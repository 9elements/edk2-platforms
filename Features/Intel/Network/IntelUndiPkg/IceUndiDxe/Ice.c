/**************************************************************************

Copyright (c) 2016 - 2022, Intel Corporation. All Rights Reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of Intel Corporation nor the names of its contributors
      may be used to endorse or promote products derived from this software
      without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

***************************************************************************/
#include "Ice.h"

#include "DeviceSupport.h"
#include "Hii/Hii.h"
#include "AdapterInformation.h"
#include "EepromConfig.h"
#include "ice_sched.h"
#include "Link.h"
#include "LinkTopology.h"

/* Global variables for blocking IO*/
STATIC BOOLEAN  mInitializeLock = TRUE;
STATIC EFI_LOCK gLock;
EFI_GUID gEfiUndiVarGuid = EFI_UNDI_VAR_GUID;

#ifdef SV_SUPPORT
STATIC UINT8 mDebugBuf[4096];
#endif /* SV_SUPPORT */


/** Blocking function called to assure that we are not swapped out from
   the queue while moving TX ring tail pointer.

   @param[in]   AdapterInfo   Pointer to the NIC data structure information
                             the UNDI driver is layering on
   @param[in]   Flag         Block flag

   @return   According to Flag setting (TRUE/FALSE) we're acquiring or releasing EFI lock
**/
VOID
IceBlockIt (
  IN DRIVER_DATA *AdapterInfo,
  IN UINT32       Flag
  )
{
  if (AdapterInfo->Block != NULL) {
    (*AdapterInfo->Block) (AdapterInfo->UniqueId, Flag);
  } else {
    if (mInitializeLock) {
      EfiInitializeLock (&gLock, TPL_NOTIFY);
      mInitializeLock = FALSE;
    }

    if (Flag != 0) {
      EfiAcquireLock (&gLock);
    } else {
      EfiReleaseLock (&gLock);
    }
  }
}


/** This is the drivers copy function so it does not need to rely on the
  BootServices copy which goes away at runtime.

  This copy function allows 64-bit or 32-bit copies depending on platform
  architecture. On Itanium we must check that both addresses
  are naturally aligned before attempting a 64-bit copy.

  @param[in]  Dest    Destination memory pointer.
  @param[in]  Source  Source memory pointer.
  @param[in]  Count   Number of bytes to copy.

  @return     Count bytes from Source copied to Dest
**/
VOID
IceMemCopy (
  IN  UINT8 *Dest,
  IN  UINT8 *Source,
  IN  UINT32 Count
  )
{

  UINT32 BytesToCopy;
  UINT32 IntsToCopy;
  UINTN *SourcePtr;
  UINTN *DestPtr;
  UINT8 *SourceBytePtr;
  UINT8 *DestBytePtr;

  IntsToCopy  = Count / sizeof (UINTN);
  BytesToCopy = Count % sizeof (UINTN);

  SourcePtr = (UINTN *) Source;
  DestPtr   = (UINTN *) Dest;

  while (IntsToCopy > 0) {
    *DestPtr = *SourcePtr;
    SourcePtr++;
    DestPtr++;
    IntsToCopy--;
  }

  // Copy the leftover bytes.
  SourceBytePtr = (UINT8 *) SourcePtr;
  DestBytePtr   = (UINT8 *) DestPtr;
  while (BytesToCopy > 0) {
    *DestBytePtr = *SourceBytePtr;
    SourceBytePtr++;
    DestBytePtr++;
    BytesToCopy--;
  }
}


/** Copies the frame from one of the Rx buffers to the command block
  passed in as part of the cpb parameter.

  The flow:  Ack the interrupt, setup the pointers, find where the last
  block copied is, check to make sure we have actually received something,
  and if we have then we do a lot of work. The packet is checked for errors,
  adjust the amount to copy if the buffer is smaller than the packet,
  copy the packet to the EFI buffer, and then figure out if the packet was
  targetted at us, broadcast, multicast or if we are all promiscuous.
  We then put some of the more interesting information (protocol, src and dest
  from the packet) into the db that is passed to us.  Finally we clean up
  the frame, set the return value to _SUCCESS, and inc the index, watching
  for wrapping.  Then with all the loose ends nicely wrapped up,
  fade to black and return.

  @param[in]  AdapterInfo  Pointer to the NIC data structure information which
                           the UNDI driver is layering on
  @param[in]  CpbReceive  Pointer (Ia-64 friendly) to the command parameter block.
                          The frame will be placed inside of it.
  @param[in]  DbReceive   The data buffer.  The out of band method of passing
                          pre-digested information to the protocol.

  @retval     PXE_STATCODE_NO_DATA  There is no data to receive
  @retval     PXE_STATCODE_SUCCESS  Received data passed to the protocol.
**/
UINTN
IceReceive (
  IN DRIVER_DATA     *AdapterInfo,
  IN PXE_CPB_RECEIVE *CpbReceive,
  IN PXE_DB_RECEIVE  *DbReceive
  )
{
  PXE_FRAME_TYPE            PacketType;
  union ice_16byte_rx_desc *ReceiveDescriptor;
  ETHER_HEADER             *EtherHeader;
  PXE_STATCODE              StatCode;
  ICE_RING                 *RxRing;
  UINT8                    *PacketPtr;
  UINT16                    TempLen;
  UINT16                    i;

  UINT32 RxStatus;
  UINT32 RxError;
  UINT16 RxPacketLength;
  UINT16 RxHeaderLength;
  UINT16 RxSph;
  UINT16 RxPType;

  UINT64 DescQWord;

  PacketType  = PXE_FRAME_TYPE_NONE;
  StatCode    = PXE_STATCODE_NO_DATA;
  i           = 0;

  // Get a pointer to the buffer that should have a rx in it, IF one is really there.
  RxRing = &AdapterInfo->Vsi.RxRing;
  ReceiveDescriptor = ICE_RX_DESC (RxRing, RxRing->NextToUse);

  DescQWord = ReceiveDescriptor->wb.qword1.status_error_len;
  RxStatus = (UINT32) ((DescQWord & ICE_RXD_QW1_STATUS_M) >> ICE_RXD_QW1_STATUS_S);

  if ((RxStatus & (1 << ICE_RX_DESC_STATUS_DD_S)) != 0) {
    DEBUGPRINT(RX, ("Descriptor done on Port %X, B/D/F %2X:%2X.%X\n",
                    AdapterInfo->PhysicalPortNumber,
                    AdapterInfo->Bus, AdapterInfo->Device, AdapterInfo->Function));

    if (RxRing->NextToUse != AdapterInfo->LastRxDescReported) {
      AdapterInfo->RxPacketPending   = TRUE;
    }

    RxPacketLength = (UINT16) ((DescQWord & ICE_RXD_QW1_LEN_PBUF_M) >> ICE_RXD_QW1_LEN_PBUF_S);
    RxHeaderLength = (UINT16) ((DescQWord & ICE_RXD_QW1_LEN_HBUF_M) >> ICE_RXD_QW1_LEN_HBUF_S);
    RxSph          = (UINT16) ((DescQWord & ICE_RXD_QW1_LEN_SPH_M) >> ICE_RXD_QW1_LEN_SPH_S);
    RxError        = (UINT32) ((DescQWord & ICE_RXD_QW1_ERROR_M) >> ICE_RXD_QW1_ERROR_S);
    RxPType        = (UINT16) ((DescQWord & ICE_RXD_QW1_PTYPE_M) >> ICE_RXD_QW1_PTYPE_S);

    // Just to make sure we don't try to copy a zero length, only copy a positive sized packet.
    if (RxPacketLength != 0) {
      // If the buffer passed us is smaller than the packet, only copy the size of the buffer.
      TempLen = RxPacketLength;
      if (RxPacketLength > (INT16) CpbReceive->BufferLen) {
        TempLen = (UINT16) CpbReceive->BufferLen;
      }

      // Copy the packet from our list to the EFI buffer.
      IceMemCopy (
        (UINT8 *) (UINTN) CpbReceive->BufferAddr,
        RxRing->UnmappedBuffers[RxRing->NextToUse],
        TempLen
      );

      PacketPtr = (UINT8 *) (UINTN) CpbReceive->BufferAddr;


      DEBUGDUMP (
        RX, ("%02x:%02x:%02x:%02x:%02x:%02x %02x:%02x:%02x:%02x:%02x:%02x %02x%02x %02x %02x\n",
        PacketPtr[0x0], PacketPtr[0x1], PacketPtr[0x2], PacketPtr[0x3], PacketPtr[0x4], PacketPtr[0x5],
        PacketPtr[0x6], PacketPtr[0x7], PacketPtr[0x8], PacketPtr[0x9], PacketPtr[0xA], PacketPtr[0xB],
        PacketPtr[0xC], PacketPtr[0xD], PacketPtr[0xE], PacketPtr[0xF])
      );

      // Fill the DB with needed information
      DbReceive->FrameLen = RxPacketLength;  // includes header
      DbReceive->MediaHeaderLen = PXE_MAC_HEADER_LEN_ETHER;

      EtherHeader = (ETHER_HEADER *) (UINTN) PacketPtr;

      // Figure out if the packet was meant for us, was a broadcast, multicast or we
      // recieved a frame in promiscuous mode.
      for (i = 0; i < PXE_HWADDR_LEN_ETHER; i++) {
        if (EtherHeader->DestAddr[i] != AdapterInfo->Hw.port_info[0].mac.lan_addr[i]) {
          break;
        }
      }

      // if we went the whole length of the header without breaking out then the packet is
      // directed at us.
      if (i >= PXE_HWADDR_LEN_ETHER) {
        PacketType = PXE_FRAME_TYPE_UNICAST;
      } else {

        // Compare it against our broadcast node address
        for (i = 0; i < PXE_HWADDR_LEN_ETHER; i++) {
          if (EtherHeader->DestAddr[i] != AdapterInfo->BroadcastNodeAddress[i]) {
            break;
          }
        }

        // If we went the whole length of the header without breaking out
        // then the packet is directed at us via broadcast
        if (i >= PXE_HWADDR_LEN_ETHER) {
          PacketType = PXE_FRAME_TYPE_BROADCAST;
        } else {

          // That leaves multicast or we must be in promiscuous mode. Check for the
          // Mcast bit in the address. Otherwise its a promiscuous receive.
          if ((EtherHeader->DestAddr[0] & 1) == 1) {
            PacketType = PXE_FRAME_TYPE_MULTICAST;
          } else {
            PacketType = PXE_FRAME_TYPE_PROMISCUOUS;
          }
        }
      }

      DEBUGPRINT (RX, ("Status %x, Length %d, PacketType = %d\n", RxStatus, RxPacketLength, PacketType));

      DbReceive->Type = PacketType;

      // Put the protocol (UDP, TCP/IP) in the data buffer.
      DbReceive->Protocol = EtherHeader->Type;

      for (i = 0; i < PXE_HWADDR_LEN_ETHER; i++) {
        DbReceive->SrcAddr[i]   = EtherHeader->SrcAddr[i];
        DbReceive->DestAddr[i]  = EtherHeader->DestAddr[i];
      }

      DEBUGDUMP (
        RX, ("RxRing.NextToUse: %x (Physical: %p, Unmapped: %p)\n",
        RxRing->NextToUse,
        (UINT64) (RxRing->PhysicalBuffers[RxRing->NextToUse]),
        (UINT64) (RxRing->UnmappedBuffers[RxRing->NextToUse]))
      );

      StatCode = PXE_STATCODE_SUCCESS;
    } else {
      DEBUGPRINT (CRITICAL, ("ERROR: RxPacketLength: %x, RxError: %x \n", RxPacketLength, RxError));
    }

    // Clean up the packet and restore the buffer address
    ReceiveDescriptor->wb.qword1.status_error_len = 0;
    ReceiveDescriptor->read.pkt_addr = (UINT64) RxRing->PhysicalBuffers[RxRing->NextToUse];

    // Move the current cleaned buffer pointer, being careful to wrap it as needed.  Then update the hardware,
    // so it knows that an additional buffer can be used.
    IceWrite32 (AdapterInfo, QRX_TAIL (0), RxRing->NextToUse);

    RxRing->NextToUse++;
    if (RxRing->NextToUse == RxRing->Count) {
      RxRing->NextToUse = 0;
    }
  }

  return StatCode;
}

/** Takes a command block pointer (cpb) and sends the frame.

  Takes either one fragment or many and places them onto the wire.
  Cleanup of the send happens in the function UNDI_Status in Decode.c

  @param[in]  AdapterInfo   Pointer to the NIC data structure information
                             the UNDI driver is layering on
  @param[in]  Cpb           The command parameter block address.
                            64 bits since this is Itanium(tm) processor friendly
  @param[in]  OpFlags       The operation flags, tells if there is any special
                            sauce on this transmit

  @retval     PXE_STATCODE_SUCCESS        The frame goes out
  @retval     PXE_STATCODE_DEVICE_FAILURE The frame does not go out
  @retval     PXE_STATCODE_BUSY           Need to call again later
**/
UINTN
IceTransmit (
  IN DRIVER_DATA *AdapterInfo,
  IN UINT64       Cpb,
  IN UINT16       OpFlags
  )
{
  PXE_CPB_TRANSMIT_FRAGMENTS *TxFrags;
  PXE_CPB_TRANSMIT           *TxBuffer;
  ICE_RING                   *TxRing;
  EFI_STATUS                  Status;
  struct ice_tx_desc         *TransmitDescriptor;
  UINT16                      Size;
  UINT32                      TdCommand = 0;
  UINT32                      TdOffset = 0;
  UINT32                      TdTag = 0;
  UINT32                      i;
  INT32                       WaitMsec;

  TxRing = &AdapterInfo->Vsi.TxRing;

  DEBUGPRINT (TX, ("OpFlags: %x, Port %X, B/D/F %2X:%2X.%X\n",
                   OpFlags, AdapterInfo->PhysicalPortNumber,
                   AdapterInfo->Bus, AdapterInfo->Device, AdapterInfo->Function));

  // Transmit buffers must be freed by the upper layer before we can transmit any more.
  if (TxRing->TxBufferMappings[TxRing->NextToUse].PhysicalAddress != 0) {
    DEBUGPRINT (TX | CRITICAL, ("TX buffers have all been used!\n"));
    return PXE_STATCODE_QUEUE_FULL;
  }

  // Make some short cut pointers so we don't have to worry about typecasting later.
  // If the TX has fragments we will use the
  // tx_tpr_f pointer, otherwise the tx_ptr_l (l is for linear)
  TxBuffer  = (PXE_CPB_TRANSMIT *) (UINTN) Cpb;
  TxFrags   = (PXE_CPB_TRANSMIT_FRAGMENTS *) (UINTN) Cpb;

  if (AdapterInfo->VlanEnable) {
    TdCommand |= ICE_TX_DESC_CMD_IL2TAG1;
    TdTag = AdapterInfo->VlanTag;
  }

  // quicker pointer to the next available Tx descriptor to use.
  TransmitDescriptor = ICE_TX_DESC (TxRing, TxRing->NextToUse);

  // Opflags will tell us if this Tx has fragments
  // So far the linear case (the no fragments case, the else on this if) is the majority
  // of all frames sent.
  if (OpFlags & PXE_OPFLAGS_TRANSMIT_FRAGMENTED) {

    // this count cannot be more than 8;
    DEBUGPRINT (TX, ("Fragments %x\n", TxFrags->FragCnt));

    // for each fragment, give it a descriptor, being sure to keep track of the number used.
    for (i = 0; i < TxFrags->FragCnt; i++) {

      // Put the size of the fragment in the descriptor
      TransmitDescriptor->buf_addr = TxFrags->FragDesc[i].FragAddr;
      Size = (UINT16) TxFrags->FragDesc[i].FragLen;

      TransmitDescriptor->cmd_type_offset_bsz = ICE_TX_DESC_DTYPE_DATA
                                                | ((UINT64) TdCommand << ICE_TXD_QW1_CMD_S)
                                                | ((UINT64) TdOffset << ICE_TXD_QW1_OFFSET_S)
                                                | ((UINT64) Size << ICE_TXD_QW1_TX_BUF_SZ_S)
                                                | ((UINT64) TdTag << ICE_TXD_QW1_L2TAG1_S);

      TxRing->TxBufferMappings[TxRing->NextToUse].PhysicalAddress = TxFrags->FragDesc[i].FragAddr;

      // If this is the last fragment we must also set the EOP bit
      if ((i + 1) == TxFrags->FragCnt) {
        TransmitDescriptor->cmd_type_offset_bsz |= (UINT64) ICE_TXD_CMD << ICE_TXD_QW1_CMD_S;
      }

      // move our software counter passed the frame we just used, watching for wrapping
      DEBUGPRINT (TX, ("Advancing TX pointer %x\n", AdapterInfo->Vsi.TxRing.NextToUse));
      AdapterInfo->Vsi.TxRing.NextToUse++;
      if (AdapterInfo->Vsi.TxRing.NextToUse == AdapterInfo->Vsi.TxRing.Count) {
        AdapterInfo->Vsi.TxRing.NextToUse = 0;
      }

      TransmitDescriptor = ICE_TX_DESC (&AdapterInfo->Vsi.TxRing, AdapterInfo->Vsi.TxRing.NextToUse);
    }
  } else {
    Size = (UINT16) ((UINT16) TxBuffer->DataLen + TxBuffer->MediaheaderLen);

    TxRing->TxBufferMappings[TxRing->NextToUse].UnmappedAddress = TxBuffer->FrameAddr;
    TxRing->TxBufferMappings[TxRing->NextToUse].Size = Size;

    Status = UndiDmaMapMemoryRead (
               AdapterInfo->PciIo,
               &TxRing->TxBufferMappings[TxRing->NextToUse]
               );

    if (EFI_ERROR (Status)) {
      DEBUGPRINT (CRITICAL, ("Failed to map Tx buffer: %r\n", Status));
      DEBUGWAIT (CRITICAL);
      return PXE_STATCODE_DEVICE_FAILURE;
    }

    TransmitDescriptor->buf_addr = TxRing->TxBufferMappings[TxRing->NextToUse].PhysicalAddress;

    TransmitDescriptor->cmd_type_offset_bsz = ICE_TX_DESC_DTYPE_DATA
                                              | ((UINT64) TdCommand << ICE_TXD_QW1_CMD_S)
                                              | ((UINT64) TdOffset << ICE_TXD_QW1_OFFSET_S)
                                              | ((UINT64) Size << ICE_TXD_QW1_TX_BUF_SZ_S)
                                              | ((UINT64) TdTag << ICE_TXD_QW1_L2TAG1_S);
    TransmitDescriptor->cmd_type_offset_bsz |= (UINT64) ICE_TXD_CMD << ICE_TXD_QW1_CMD_S;

    // Move our software counter passed the frame we just used, watching for wrapping
    TxRing->NextToUse++;
    if (TxRing->NextToUse == TxRing->Count) {
      TxRing->NextToUse = 0;
    }
    DEBUGDUMP (
      TX, ("Length = %d, Buffer addr %x, cmd_type_offset_bsz %x \n",
      Size,
      TransmitDescriptor->buf_addr,
      TransmitDescriptor->cmd_type_offset_bsz)
    );


#if (DBG_LVL & TX)
    UINT8 * PacketPtr = (UINT8 *) (UINTN) TransmitDescriptor->buf_addr;
    DEBUGDUMP (
      TX, ("DestMAC: %02x:%02x:%02x:%02x:%02x:%02x SrcMAC: %02x:%02x:%02x:%02x:%02x:%02x EtherType: %02x%02x Ver/LengthX32b: %02x DSCP: %02x\n",
      PacketPtr[0x0], PacketPtr[0x1], PacketPtr[0x2], PacketPtr[0x3], PacketPtr[0x4], PacketPtr[0x5],
      PacketPtr[0x6], PacketPtr[0x7], PacketPtr[0x8], PacketPtr[0x9], PacketPtr[0xA], PacketPtr[0xB],
      PacketPtr[0xC], PacketPtr[0xD], PacketPtr[0xE], PacketPtr[0xF])
    );
#endif /* (DBG_LVL & TX) */
  }

  // Turn on the blocking function so we don't get swapped out
  // Then move the Tail pointer so the HW knows to start processing the TX we just setup.
  IceBlockIt (AdapterInfo, TRUE);
  IceWrite32 (AdapterInfo, QTX_COMM_DBELL (0), TxRing->NextToUse);
  IceBlockIt (AdapterInfo, FALSE);

  //Force wait
  //OpFlags |= PXE_OPFLAGS_TRANSMIT_BLOCK;
  // If the OpFlags tells us to wait for the packet to hit the wire, we will wait.
  if ((OpFlags & PXE_OPFLAGS_TRANSMIT_BLOCK) != 0) {
    WaitMsec = 10000;

    while ((TransmitDescriptor->cmd_type_offset_bsz & ICE_TX_DESC_DTYPE_DESC_DONE) == 0) {
      DelayInMicroseconds (AdapterInfo, 10);
      WaitMsec -= 10;
      if (WaitMsec <= 0) {
        break;
      }
    }

    // If we waited for a while, and it didn't finish then the HW must be bad.
    if ((TransmitDescriptor->cmd_type_offset_bsz & ICE_TX_DESC_DTYPE_DESC_DONE) == 0) {
      DEBUGPRINT (TX | CRITICAL, ("Device failure\n"));
      return PXE_STATCODE_DEVICE_FAILURE;
    } else {
      DEBUGPRINT (TX, ("Transmit success\n"));
    }
  }

  return PXE_STATCODE_SUCCESS;
}

/** Free TX buffers that have been transmitted by the hardware.

  @param[in]   AdapterInfo  Pointer to the NIC data structure information which
                           the UNDI driver is layering on.
  @param[in]   NumEntries   Number of entries in the array which can be freed.
  @param[out]  TxBuffer     Array to pass back free TX buffer

  @return      Number of TX buffers written.
**/
UINT16
IceFreeTxBuffers (
  IN  DRIVER_DATA *AdapterInfo,
  IN  UINT16       NumEntries,
  OUT UINT64      *TxBuffer
  )
{

  struct ice_tx_desc  *TransmitDescriptor;
  ICE_RING            *TxRing;
  UINT16               i;
  EFI_STATUS           Status;

  i =      0;
  TxRing = &AdapterInfo->Vsi.TxRing;

  do {
    if (i >= NumEntries) {
      DEBUGPRINT (TX, ("Exceeded number of DB entries, i=%d, NumEntries=%d\n", i, NumEntries));
      break;
    }

    TransmitDescriptor = ICE_TX_DESC (TxRing, TxRing->NextToClean);

    DEBUGPRINT (
      TX, ("TXDesc:%d Addr:%x, ctob: %x\n",
      TxRing->NextToClean,
      TransmitDescriptor->buf_addr,
      TransmitDescriptor->cmd_type_offset_bsz)
    );

    if ((TransmitDescriptor->cmd_type_offset_bsz & ICE_TX_DESC_DTYPE_DESC_DONE) != 0) {
      if (TxRing->TxBufferMappings[TxRing->NextToClean].PhysicalAddress == 0) {
        DEBUGPRINT (TX | CRITICAL, ("ERROR: TX buffer complete without being marked used!\n"));
        break;
      }

      DEBUGPRINT (TX, ("Cleaning buffer address %d, %x\n", i, TxBuffer[i]));

      Status = UndiDmaUnmapMemory (
                 AdapterInfo->PciIo,
                 &TxRing->TxBufferMappings[TxRing->NextToClean]
                 );

      if (EFI_ERROR (Status)) {
        DEBUGPRINT (CRITICAL, ("Failed to unmap Tx buffer: %r\n", Status));
        DEBUGWAIT (CRITICAL);
        break;
      }

      TxBuffer[i] = TxRing->TxBufferMappings[TxRing->NextToClean].UnmappedAddress;
      i++;

      TxRing->TxBufferMappings[TxRing->NextToClean].UnmappedAddress = 0;
      TxRing->TxBufferMappings[TxRing->NextToClean].Size = 0;
      TransmitDescriptor->cmd_type_offset_bsz &= ~((UINT64)ICE_TXD_QW1_DTYPE_M);

      TxRing->NextToClean++;
      if (TxRing->NextToClean >= TxRing->Count) {
        TxRing->NextToClean = 0;
      }
    } else {
      DEBUGPRINT (TX, ("TX Descriptor %d not done\n", TxRing->NextToClean));
      break;
    }
  } while (TxRing->NextToUse != TxRing->NextToClean);

  return i;
}

/** Sets receive filters.

  @param[in]  AdapterInfo  Pointer to the adapter structure
  @param[in]  NewFilter    A PXE_OPFLAGS bit field indicating what filters to use.

  @return     Broad/Multicast and promiscous settings are set according to NewFilter
**/
VOID
IceSetFilter (
  IN DRIVER_DATA *AdapterInfo,
  IN UINT16       NewFilter
  )
{
  UINT8            PromiscuousMask = 0;
#ifndef BMSM_MODE
  enum ice_status  ScStatus        = ICE_SUCCESS;
#endif /* !BMSM_MODE */

  DEBUGPRINT (
    RXFILTER, ("\n-->IceSetFilter [PF %d] - NewFilter = %x OldFilter = %x\n",
    AdapterInfo->Function,
    NewFilter,
    AdapterInfo->RxFilter)
  );

  if ((NewFilter & PXE_OPFLAGS_RECEIVE_FILTER_PROMISCUOUS) != 0) {
    PromiscuousMask |= ICE_PROMISC_UCAST_RX | ICE_PROMISC_BCAST_RX | ICE_PROMISC_MCAST_RX;
    DEBUGPRINT (RXFILTER, (" - set UCAST/BCAST/MCAST\n"));
  }

  if ((NewFilter & PXE_OPFLAGS_RECEIVE_FILTER_BROADCAST) != 0) {
    PromiscuousMask |= ICE_PROMISC_UCAST_RX | ICE_PROMISC_BCAST_RX;
    DEBUGPRINT (RXFILTER, (" - set UCAST/BCAST\n"));
  }

  if ((NewFilter & PXE_OPFLAGS_RECEIVE_FILTER_ALL_MULTICAST) != 0) {
    PromiscuousMask |= ICE_PROMISC_UCAST_RX | ICE_PROMISC_MCAST_RX;
    DEBUGPRINT (RXFILTER, (" - set UCAST/MCAST\n"));
  }

  if (PromiscuousMask == 0) {
    DEBUGPRINT (RXFILTER, ("- nothing to do\n"));
  }

#ifndef BMSM_MODE
  if (!AdapterInfo->DriverBusy &&
      (PromiscuousMask != AdapterInfo->CurrentPromiscuousMask))
  {
    AdapterInfo->CurrentPromiscuousMask = PromiscuousMask;

    ScStatus = ice_set_vsi_promisc (
                 &AdapterInfo->Hw,
                 AdapterInfo->Vsi.Id,
                 PromiscuousMask,
                 0 // no VLAN
               );

    if (ScStatus != ICE_SUCCESS) {
      DEBUGPRINT (RXFILTER, ("ERROR: Failed to set Rx filter !!!\n"));
    }
    DEBUGPRINT (
      RXFILTER, ("ice_set_vsi_promisc (vsi %d, mask %02x) returned %d\n",
      AdapterInfo->Vsi.Id,
      PromiscuousMask,
      ScStatus)
    );
  }
#endif /* !BMSM_MODE */

  AdapterInfo->RxFilter |= NewFilter;
}

/** Clears receive filters.

  @param[in]  AdapterInfo  Pointer to the adapter structure
  @param[in]  NewFilter    A PXE_OPFLAGS bit field indicating what filters to clear.

  @return     Broad/Multicast and promiscuous settings are cleared according to NewFilter
**/
VOID
IceClearFilter (
  IN DRIVER_DATA *AdapterInfo,
  IN UINT16       NewFilter
  )
{
  UINT8            PromiscuousMask = 0;
#ifndef BMSM_MODE
  enum ice_status  ScStatus = ICE_SUCCESS;
#endif /* !BMSM_MODE */

  DEBUGPRINT (
    RXFILTER, ("\n-->IceClearFilter [PF %d] - NewFilter = %x OldFilter = %x\n",
    AdapterInfo->Function,
    NewFilter,
    AdapterInfo->RxFilter)
  );

  if ((NewFilter & PXE_OPFLAGS_RECEIVE_FILTER_PROMISCUOUS) != 0) {
    PromiscuousMask |= ICE_PROMISC_UCAST_RX | ICE_PROMISC_BCAST_RX | ICE_PROMISC_MCAST_RX;
    DEBUGPRINT (RXFILTER, (" - clear UCAST/BCAST/MCAST\n"));
  }

  if ((NewFilter & PXE_OPFLAGS_RECEIVE_FILTER_BROADCAST) != 0) {
    PromiscuousMask |= ICE_PROMISC_UCAST_RX | ICE_PROMISC_BCAST_RX;
    DEBUGPRINT (RXFILTER, (" - clear UCAST/BCAST\n"));
  }

  if ((NewFilter & PXE_OPFLAGS_RECEIVE_FILTER_ALL_MULTICAST) != 0) {
    PromiscuousMask |= ICE_PROMISC_UCAST_RX | ICE_PROMISC_MCAST_RX;
    DEBUGPRINT (RXFILTER, (" - clear UCAST/MCAST\n"));
  }

  if (PromiscuousMask == 0) {
    DEBUGPRINT (RXFILTER, ("- nothing to do\n"));
  }

#ifndef BMSM_MODE
  if (!AdapterInfo->DriverBusy &&
      ((PromiscuousMask & AdapterInfo->CurrentPromiscuousMask) != 0))
  {
    AdapterInfo->CurrentPromiscuousMask &= ~PromiscuousMask;

    ScStatus = ice_clear_vsi_promisc (
                 &AdapterInfo->Hw,
                 AdapterInfo->Vsi.Id,
                 PromiscuousMask,
                 0 // no VLAN
               );

    if (ScStatus != ICE_SUCCESS) {
      DEBUGPRINT (RXFILTER, ("ERROR: Failed to clear Rx filter !!!\n"));
    }
     DEBUGPRINT (
       RXFILTER, ("ice_clear_vsi_promisc (vsi %d, mask %02x) returned %d\n",
       AdapterInfo->Vsi.Id,
       PromiscuousMask,
       ScStatus)
     );
  }
#endif /* !BMSM_MODE */

  AdapterInfo->RxFilter &= ~NewFilter;
}

/* Free multicast lists allocated for ice_add_mac and ice_remove_mac.

  param[in]      LinkedMcastList    List of MAC addresses to free.
*/
VOID
IceFreeMcastList (
  IN LIST_ENTRY       *LinkedMcastList
)
{
  struct ice_fltr_list_entry *MacListIter;
  struct ice_fltr_list_entry *TmpMacListIter;

  LIST_FOR_EACH_ENTRY_SAFE (MacListIter, TmpMacListIter, LinkedMcastList, ice_fltr_list_entry, list_entry) {
    FreePool (MacListIter);
  }
}

/* Creates and populates list of filter info entries based on MCAST_LIST structure which
  contains raw table of MAC addresses inside.

  param[in]   AdapterInfo       Driver private data structure
  param[in]   McastList         List of MAC addresses in MCAST_LIST structure format
  param[out]  LinkedMcastList   Resulting linked list of MAC addresses in ice_fltr_list_entry
                                format

  @retval    EFI_OUT_OF_RESOURCES  Failed to allocate resources for ice_fltr_list_entry entry
  @retval    EFI_SUCCESS           List of filter info entries successfully created
*/
EFI_STATUS
IceCreateFilterListFromArray (
  IN  DRIVER_DATA *AdapterInfo,
  IN  MCAST_LIST  *McastList,
  OUT LIST_ENTRY  *LinkedMcastList
  )
{
  struct ice_fltr_list_entry *MacListIter;
  UINT8                      *MacAddr;
  UINTN                      i = 0;
  UINTN                      j;

  InitializeListHead (LinkedMcastList);

  for (i = 0; i < McastList->Length; i++) {

    struct ice_fltr_list_entry *Entry;
    struct ice_fltr_info *FilterInfo;

    Entry = AllocateZeroPool (sizeof (struct ice_fltr_list_entry));

    if (Entry == NULL) {
      DEBUGPRINT (RXFILTER, ("Failed to allocate memory for Rx filter entry.\n"));
      IceFreeMcastList (LinkedMcastList);
      return EFI_OUT_OF_RESOURCES;
    }

    FilterInfo = &Entry->fltr_info;

    FilterInfo->fltr_act         = ICE_FWD_TO_VSI;
    FilterInfo->vsi_handle       = AdapterInfo->Vsi.Id;
    FilterInfo->fwd_id.hw_vsi_id = AdapterInfo->Vsi.Id;
    FilterInfo->lkup_type        = ICE_SW_LKUP_MAC;
    FilterInfo->flag             = ICE_FLTR_TX;
    FilterInfo->src              = AdapterInfo->Hw.port_info->lport;
    FilterInfo->src_id           = ICE_SRC_ID_VSI;

    CopyMem (
      FilterInfo->l_data.mac.mac_addr,
      McastList->McAddr[i],
      ETH_ALEN // PXE_MAC_LENGTH (32) ????
    );

    InsertTailList (LinkedMcastList, (LIST_ENTRY *) Entry);
  }

  // Debugprint the list
  j = 0;
  LIST_FOR_EACH_ENTRY (MacListIter, LinkedMcastList, ice_fltr_list_entry, list_entry) {
    MacAddr = MacListIter->fltr_info.l_data.mac.mac_addr;
    DEBUGPRINT (
      RXFILTER, ("%d - %02x:%02x:%02x:%02x:%02x:%02x\n",
      j++, MacAddr[0], MacAddr[1], MacAddr[2], MacAddr[3], MacAddr[4], MacAddr[5])
    );
  }

  return EFI_SUCCESS;
}

/** Adds MAC/VLAN elements to multicast list

   @param[in]   AdapterInfo   Pointer to the NIC data structure information
                              the UNDI driver is layering on

   @return  MAC/VLAN elements from adapter VSI structure are added to list
**/
VOID
IceSetMcastList (
  IN DRIVER_DATA *AdapterInfo
  )
{
  EFI_STATUS  Status;
  enum ice_status  IceStatus;
  LIST_ENTRY  CurrMcastList;
  LIST_ENTRY  NewMcastList;
  struct ice_fltr_list_entry *MacListIter;
  UINTN       i;

  DEBUGPRINT (RXFILTER, ("\n-->IceSetMcastList \n"));

  if (!AdapterInfo->DriverBusy) {

    // Remove existing elements from the Forwarding Table
    if (AdapterInfo->Vsi.CurrentMcastList.Length > 0) {

      DEBUGPRINT (RXFILTER, ("Remove MAC's list: \n"));
      Status = IceCreateFilterListFromArray (
                 AdapterInfo,
                 &AdapterInfo->Vsi.CurrentMcastList,
                 &CurrMcastList
               );
      if (EFI_ERROR (Status)) {
        DEBUGPRINT (RXFILTER, ("Error: IceCreateFilterList() returned: %d"));
      }

      IceStatus = ice_remove_mac (&AdapterInfo->Hw, &CurrMcastList);

      if (IceStatus != ICE_SUCCESS) {
        DEBUGPRINT (RXFILTER, ("ice_remove_mac() returned: %d\n", IceStatus));
        i = 0;
        LIST_FOR_EACH_ENTRY (MacListIter, &CurrMcastList, ice_fltr_list_entry, list_entry) {
          DEBUGPRINT (RXFILTER, ("ice_remove_mac() entry: %d, status: %d\n", i++, MacListIter->status));
        }
      }

      IceFreeMcastList (&CurrMcastList);
    }

    // Add new elements to the Forwarding Table
    if (AdapterInfo->Vsi.McastListToProgram.Length > 0) {

      DEBUGPRINT (RXFILTER, ("Add MAC's list: \n"));
      Status = IceCreateFilterListFromArray (
                 AdapterInfo,
                 &AdapterInfo->Vsi.McastListToProgram,
                 &NewMcastList
               );
      if (EFI_ERROR (Status)) {
        DEBUGPRINT (RXFILTER, ("Error: IceCreateFilterList() returned: %d"));
      }

      IceStatus = ice_add_mac (&AdapterInfo->Hw, &NewMcastList);

      if (IceStatus != ICE_SUCCESS) {
        DEBUGPRINT (RXFILTER, ("ice_add_mac() returned: %d\n", IceStatus));
        i = 0;
        LIST_FOR_EACH_ENTRY (MacListIter, &NewMcastList, ice_fltr_list_entry, list_entry) {
          DEBUGPRINT (
            RXFILTER, ("ice_add_mac() entry: %d, status: %d, rule id: %d\n",
            i++,
            MacListIter->status,
            MacListIter->fltr_info.fltr_rule_id)
          );
        }
      }

      IceFreeMcastList (&NewMcastList);
    }
  }

  // Update CurrentMcastList
  CopyMem(
    AdapterInfo->Vsi.CurrentMcastList.McAddr,
    AdapterInfo->Vsi.McastListToProgram.McAddr,
    AdapterInfo->Vsi.McastListToProgram.Length * PXE_MAC_LENGTH
  );
  AdapterInfo->Vsi.CurrentMcastList.Length = AdapterInfo->Vsi.McastListToProgram.Length;
}


/** Starts Rx rings.

   Enable rings by using Queue enable registers

   @param[in]  AdapterInfo   Pointer to the NIC data structure information
                             the UNDI driver is layering on

   @retval     ICE_SUCCESS      RX rings started successfully
   @retval     ICE_ERR_TIMEOUT  Waiting for RX queue status timed out
**/
enum ice_status
IceReceiveStart (
  IN DRIVER_DATA *AdapterInfo
  )
{
  struct ice_hw  *Hw;
  UINTN           j = 0;

  Hw = &AdapterInfo->Hw;

  // Enable Rx queues by setting proper bits in QRX_CTRL
  // Wait and check if status bits are changed.
  wr32 (Hw, QRX_CTRL (0), (rd32 (Hw, QRX_CTRL (0)) | QRX_CTRL_QENA_REQ_M));

  // Wait for the Rx queue status
  for (j = 0; j < START_RINGS_TIMEOUT; j++) {
    if (rd32 (Hw, QRX_CTRL (0)) & QRX_CTRL_QENA_STAT_M) {
      break;
    }
    gBS->Stall (10);
  }
  if (j >= START_RINGS_TIMEOUT) {
    DEBUGPRINT (
      CRITICAL, ("Rx ring enable timed out, value %x\n",
      rd32 (Hw, QRX_CTRL (0)))
    );
    return ICE_ERR_NOT_READY;
  } else {
    DEBUGPRINT (INIT, ("Rx ring enabled\n"));
  }

  AdapterInfo->ReceiveStarted = TRUE;

  return ICE_SUCCESS;
}

/** Stops Rx rings.

   @param[in]   AdapterInfo   Pointer to the NIC data structure information
                              the UNDI driver is layering on

   @retval     ICE_SUCCESS      RX rings started successfully
   @retval     ICE_ERR_TIMEOUT  Waiting for RX queue status timed out
**/
enum ice_status
IceReceiveStop (
  IN DRIVER_DATA *AdapterInfo
  )
{
  struct ice_hw *Hw;
  UINTN           j;

  Hw = &AdapterInfo->Hw;

  // Disable Rx queues by setting proper bits in QRX_CTRL
  // Wait and check if status bits are changed.
  //wr32 (Hw, I40E_QTX_ENA (0), (rd32 (Hw, I40E_QTX_ENA (0)) & ~I40E_QTX_ENA_QENA_REQ_MASK));
  wr32 (Hw, QRX_CTRL (0), (rd32 (Hw, QRX_CTRL (0)) & ~QRX_CTRL_QENA_REQ_M));

  // Use the remaining delay time to check the Rx queue status
  for (j = 0; j < STOP_RINGS_TIMEOUT; j++) {
    if (!(rd32 (Hw, QRX_CTRL (0)) & QRX_CTRL_QENA_STAT_M)) {
      break;
    }
    gBS->Stall (10);
  }
  if (j >= STOP_RINGS_TIMEOUT) {
    DEBUGPRINT (
      CRITICAL, ("Rx ring disable timed out, value %x\n",
      rd32 (Hw, QRX_CTRL (0)))
    );
    return ICE_ERR_NOT_READY;
  }
  DEBUGPRINT (INIT, ("Rx ring disabled\n"));

  gBS->Stall (50000);

  AdapterInfo->ReceiveStarted = FALSE;

  return ICE_SUCCESS;
}

/** Configure transmit and receive descriptor rings

  @param[in]  AdapterInfo  Pointer to the NIC data structure information
                           the UNDI driver is layering on

  @retval     EFI_STATUS        TX queues configured successfully
  @retval     EFI_DEVICE_ERROR  Failed to clear LAN Tx queue context on Tx ring
  @retval     EFI_DEVICE_ERROR  Failed to set LAN Tx queue context on Tx ring
  @retval     EFI_DEVICE_ERROR  Failed to clear LAN Rx queue context on Rx ring
  @retval     EFI_DEVICE_ERROR  Failed to set LAN Rx queue context on Rx ring
  @retval     EFI_OUT_OF_RESOURCES  Could not allocate buffer for Rx Ring
 **/
EFI_STATUS
IceConfigureTxQueues (
  IN DRIVER_DATA *AdapterInfo
  )
{
  enum   ice_status           IceStatus;
  struct ice_aqc_add_tx_qgrp *AqBuff;
  struct ice_hw              *Hw;
  struct ice_tlan_ctx         TxContext;
  UINT16                      BufLen;
  EFI_STATUS                  Status;
  ICE_RING                   *TxRing;
  UINT16                      QueueHandle = 0;

  DEBUGPRINT (INIT, ("IceConfigureTxQueues\n"));

  Hw =        &AdapterInfo->Hw;
  IceStatus = ICE_SUCCESS;
  Status =    EFI_DEVICE_ERROR;
  BufLen =    sizeof (struct ice_aqc_add_tx_qgrp);
  TxRing =    &AdapterInfo->Vsi.TxRing;


  AqBuff = AllocateZeroPool (BufLen);

  if (AqBuff == NULL) {
    DEBUGPRINT (CRITICAL, ("Failed to allocate memory for TxQ Lan group data elem.\n"));
    return EFI_OUT_OF_RESOURCES;
  }

  // Clear the context structure before use
  ZeroMem (&TxContext, sizeof (struct ice_tlan_ctx));

  // Must be set to 0 in NIC and Switch mode
  TxContext.port_num = Hw->port_info[0].lport;


  // PF number
  TxContext.pf_num = Hw->pf_id;

  // Congestion Domain number = 0
  TxContext.cgd_num = 0; //ring->dcb_tc;

  TxContext.base = (UINT64) TxRing->Mapping.PhysicalAddress / 128;
  TxContext.qlen = TxRing->Count;
  TxContext.tsyn_ena = 0;

  // PF queue
  TxContext.vmvf_type = 2; //ICE_TX_VMVF_PF;

  // set right VSI
  TxContext.src_vsi = Hw->fw_vsi_num;

  // Legacy queue
  TxContext.tso_ena = 1;

  // TSO enabled
  TxContext.tso_qnum = 0;

  // Legacy interface
  TxContext.legacy_int = 1;

  AqBuff->txqs[0].txq_id = 0;
  AqBuff->num_txqs = 1;
  AqBuff->parent_teid   = Hw->port_info[0].last_node_teid;

  ice_set_ctx (Hw, (UINT8 *)&TxContext, AqBuff->txqs[0].txq_ctx, ice_tlan_ctx_info);

  IceStatus = ice_ena_vsi_txq (
                &Hw->port_info[0],
                Hw->fw_vsi_num,
                0,
                QueueHandle,
                1,
                AqBuff,
                sizeof (struct ice_aqc_add_tx_qgrp),
                NULL
              );

  // We need to store q_teid to use it later as a parameter in
  // ice_dis_vsi_txq
  AdapterInfo->Vsi.TxRing.TxqTeid = AqBuff->txqs[0].q_teid;

  if (IceStatus != ICE_SUCCESS) {
    DEBUGPRINT (
      CRITICAL, ("Failed to add LAN Tx queue context on Tx ring, error: %d\n",
      IceStatus)
    );
    return EFI_DEVICE_ERROR;
  }

  // Free resources
  FreePool (AqBuff);

  return EFI_SUCCESS;
}

/** Configure transmit and receive descriptor rings

  @param[in]  AdapterInfo  Pointer to the NIC data structure information
                           the UNDI driver is layering on

  @retval     EFI_STATUS        TX/RX queues configured successfully
  @retval     EFI_DEVICE_ERROR  Failed to clear LAN Tx queue context on Tx ring
  @retval     EFI_DEVICE_ERROR  Failed to set LAN Tx queue context on Tx ring
  @retval     EFI_DEVICE_ERROR  Failed to clear LAN Rx queue context on Rx ring
  @retval     EFI_DEVICE_ERROR  Failed to set LAN Rx queue context on Rx ring
  @retval     EFI_OUT_OF_RESOURCES  Could not allocate buffer for Rx Ring
 **/
EFI_STATUS
IceConfigureRxQueues (
  IN DRIVER_DATA *AdapterInfo
  )
{
  enum   ice_status           IceStatus;
  struct ice_hw              *Hw;
  struct ice_rlan_ctx         RxContext;
  union  ice_16byte_rx_desc  *RxDesc;
  UINTN                       i;
  EFI_STATUS                  Status;
  ICE_RING                   *RxRing;
  UNDI_DMA_MAPPING           *RxBufferMapping;

  DEBUGPRINT (INIT, ("IceConfigureRxQueues\n"));

  Hw =              &AdapterInfo->Hw;
  RxRing =          &AdapterInfo->Vsi.RxRing;
  RxBufferMapping = &RxRing->RxBufferMapping;
  IceStatus =       ICE_SUCCESS;
  Status =          EFI_DEVICE_ERROR;

  // Clear the context structure first
  ZeroMem (&RxContext, sizeof (struct ice_rlan_ctx));

  RxRing->RxBufLen = ICE_RXBUFFER_2048;

  // No packet split
  RxRing->RxHdrLen = 0;

  RxContext.head = 0;
  RxContext.cpuid = 0;

  RxContext.dbuf = (UINT8) (RxRing->RxBufLen >> ICE_RLAN_CTX_DBUF_S);
  RxContext.hbuf = (UINT8) (RxRing->RxHdrLen >> ICE_RLAN_CTX_HBUF_S);

  RxContext.base = (UINT64) RxRing->Mapping.PhysicalAddress / 128;
  RxContext.qlen = RxRing->Count;

  // 0 - 16 byte descriptors in use
  // 1 - 32 byte descriptors in use
  RxContext.dsize = 0;

  RxContext.dtype =    ICE_RX_DTYPE_NO_SPLIT;
  RxContext.hsplit_0 = ICE_RLAN_RX_HSPLIT_0_NO_SPLIT;
  RxContext.hsplit_1 = ICE_RLAN_RX_HSPLIT_1_NO_SPLIT;

  // vlan stripped
  RxContext.showiv = 0;

  RxContext.l2tsel = 1;

  // Max packet size
  RxContext.rxmax = 0x600;

  RxContext.tphrdesc_ena = 0;
  RxContext.tphwdesc_ena = 0;
  RxContext.tphdata_ena = 0;
  RxContext.tphhead_ena = 0;

  // low receive queue threshold
  RxContext.lrxqthresh = 0;

  // strip crc
  RxContext.crcstrip = 1;

  // clear rx  context before init
  IceStatus = ice_clear_rxq_ctx (Hw, 0);
  if (IceStatus != ICE_SUCCESS) {
    DEBUGPRINT (
      CRITICAL, ("ice_clear_rxq_context returned %r\n",
      IceStatus)
    );
    return EFI_DEVICE_ERROR;
  }

  IceStatus = ice_write_rxq_ctx (Hw, &RxContext, 0);
  if (IceStatus != ICE_SUCCESS) {
    DEBUGPRINT (
      CRITICAL, ("Failed to set LAN Rx queue context on Rx ring, error: %d\n",
      IceStatus)
    );
    return EFI_DEVICE_ERROR;
  }

  // Enable No Expire mode on RX queue
  IceWrite32 (AdapterInfo, QRX_ITR (0), QRX_ITR_NO_EXPR_M);

  // Initialize tail register
  IceWrite32 (AdapterInfo, QRX_TAIL (0), 0);
  IceWrite32 (AdapterInfo, QRX_TAIL (0), RxRing->Count - 1);
  AdapterInfo->Vsi.RxRing.NextToUse = 0;
#if (DBG_LVL & RX)
  UINT32 QRxTail = IceRead32 (AdapterInfo, QRX_TAIL (0));
  DEBUGPRINT (INIT, ("QRXTail %d\n", QRxTail));
#endif /* (DBG_LVL & RX) */

  // Determine the overall size of memory needed for receive buffers and allocate memory
  RxBufferMapping->Size = ALIGN (RxRing->Count * RxRing->RxBufLen, 4096);

  //
  // Allocate a common buffer for Rx buffers
  //
  Status = UndiDmaAllocateCommonBuffer (AdapterInfo->PciIo, RxBufferMapping);

  if (EFI_ERROR (Status)) {
    DEBUGPRINT (CRITICAL, ("Failed to allocate memory for Rx buffers: %r\n", Status));
    return Status;
  }

  // Link the RX Descriptors to the receive buffers and cleanup descriptors
  for (i = 0; i < RxRing->Count; i++) {
    RxRing->UnmappedBuffers[i] = (UINT8 *) (RxBufferMapping->UnmappedAddress + (i * RxRing->RxBufLen));
    RxRing->PhysicalBuffers[i] = (UINT8 *) (RxBufferMapping->PhysicalAddress + (i * RxRing->RxBufLen));

    RxDesc = ICE_RX_DESC (RxRing, i);
    RxDesc->read.pkt_addr = (UINTN) RxRing->PhysicalBuffers[i];
    RxDesc->read.hdr_addr = 0;
    RxDesc->wb.qword1.status_error_len = 0;
  }

  return EFI_SUCCESS;
}

/** Configure transmit and receive descriptor rings

  @param[in]  AdapterInfo  Pointer to the NIC data structure information
                           the UNDI driver is layering on

  @retval     EFI_STATUS        TX/RX queues configured successfully
  @retval     EFI_DEVICE_ERROR  Failed to clear LAN Tx queue context on Tx ring
  @retval     EFI_DEVICE_ERROR  Failed to set LAN Tx queue context on Tx ring
  @retval     EFI_DEVICE_ERROR  Failed to clear LAN Rx queue context on Rx ring
  @retval     EFI_DEVICE_ERROR  Failed to set LAN Rx queue context on Rx ring
  @retval     EFI_OUT_OF_RESOURCES  Could not allocate buffer for Rx Ring
 **/
EFI_STATUS
IceConfigureTxRxQueues (
  IN DRIVER_DATA *AdapterInfo
  )
{
  EFI_STATUS Status = EFI_SUCCESS;

  Status = IceConfigureTxQueues (AdapterInfo);
  if (EFI_ERROR (Status)) {
    DEBUGPRINT (
      CRITICAL, ("IceConfigureTxQueues returned %r\n",
      Status)
    );
    return Status;
  }

  Status = IceConfigureRxQueues (AdapterInfo);
  if (EFI_ERROR (Status)) {
    DEBUGPRINT (
      CRITICAL, ("IceConfigureRxQueues returned %r\n",
      Status)
    );
    return Status;
  }

  return EFI_SUCCESS;
}

/** Free resources allocated for transmit and receive descriptor rings

  @param[in]  AdapterInfo  Pointer to the NIC data structure information
                             the UNDI driver is layering on

  @retval     EFI_SUCCESS       TX/RX resources freed successfully
  @retval     EFI_DEVICE_ERROR  Failed to clear LAN Tx queue context on Tx ring
  @retval     EFI_DEVICE_ERROR  Failed to clear LAN Rx queue context on Rx ring
**/
EFI_STATUS
IceFreeTxRxQueues (
  IN DRIVER_DATA *AdapterInfo
  )
{
  struct ice_hw               *Hw;
  struct ice_aqc_dis_txq_item *QueueList;
  EFI_STATUS                   Status;
  enum   ice_status            IceStatus;
  UINT16                       QueueIds[] = { 0 };

  UINT16                       VsiHandle = AdapterInfo->Vsi.Id;
  UINT8                        Tc = 0;
  UINT16                       QueueHandle[] = { 0 };

  DEBUGPRINT (INIT, ("IceFreeTxRxQueues\n"));

  Hw = &AdapterInfo->Hw;
  Status = EFI_DEVICE_ERROR;

  QueueList = AllocateZeroPool (sizeof (struct ice_aqc_dis_txq_item));

  // clear rx  context
  IceStatus = ice_clear_rxq_ctx (Hw, 0);
  if (IceStatus != ICE_SUCCESS) {
    DEBUGPRINT (
      CRITICAL, ("ice_clear_rxq_ctx returned %r\n",
      IceStatus)
    );
    return EFI_DEVICE_ERROR;
  }

  // Clear tx context (remove tx queue)
  IceStatus = ice_dis_vsi_txq (
                Hw->port_info,
                VsiHandle,
                Tc,
                1,
                QueueHandle,
                QueueIds,
                &AdapterInfo->Vsi.TxRing.TxqTeid,
                ICE_NO_RESET,
                0,
                NULL
              );
  if (IceStatus != ICE_SUCCESS) {
    DEBUGPRINT (
      CRITICAL, ("ice_dis_vsi_txq returned %r\n",
      IceStatus)
    );
    return EFI_DEVICE_ERROR;
  }

  Status = UndiDmaFreeCommonBuffer (
             AdapterInfo->PciIo,
             &AdapterInfo->Vsi.RxRing.RxBufferMapping
             );

  return Status;
}

/** Allocate memory resources for the Tx and Rx descriptors

  @param[in]  AdapterInfo  Pointer to the NIC data structure information
                             the UNDI driver is layering on

  @retval     EFI_SUCCESS           Resources allocated succesfully
  @retval     EFI_OUT_OF_RESOURCES  Could not allocate buffer for Tx descriptor ring
  @retval     EFI_OUT_OF_RESOURCES  Could not allocate buffer for Rx descriptor ring
**/
EFI_STATUS
IceSetupTxRxResources (
  IN DRIVER_DATA *AdapterInfo
  )
{
  EFI_STATUS Status;
  UINTN      i;
  ICE_RING  *TxRing;
  ICE_RING  *RxRing;

  TxRing = &AdapterInfo->Vsi.TxRing;
  RxRing = &AdapterInfo->Vsi.RxRing;

  TxRing->Count = AdapterInfo->Vsi.NumDesc;
  TxRing->Size = 0;

  RxRing->Count = AdapterInfo->Vsi.NumDesc;
  RxRing->Size = 0;

  // This block is for Tx descriptiors
  // Round up to nearest 4K
  TxRing->Size = ALIGN (TxRing->Count * sizeof (struct ice_tx_desc), 4096);
  TxRing->Mapping.Size = TxRing->Size;

  // Allocate memory
  Status = UndiDmaAllocateCommonBuffer (AdapterInfo->PciIo, &TxRing->Mapping);

  if (EFI_ERROR (Status)) {
    DEBUGPRINT (CRITICAL, ("Failed to allocate memory for Tx desc ring: %r\n", Status));
    return Status;
  }

  TxRing->NextToUse = 0;
  TxRing->NextToClean = 0;

  // All available transmit descriptors are free by default
  for (i = 0; i < TxRing->Count; i++) {
    ZeroMem (&TxRing->TxBufferMappings[i], sizeof (UNDI_DMA_MAPPING));
  }

  //  This block is for Rx descriptors
  //  Use 16 byte descriptors as we are in PXE MODE.

  // Round up to nearest 4K
  RxRing->Size = ALIGN (RxRing->Count * sizeof (union ice_16byte_rx_desc), 4096);
  RxRing->Mapping.Size = RxRing->Size;

  Status = UndiDmaAllocateCommonBuffer (AdapterInfo->PciIo, &RxRing->Mapping);

  if (EFI_ERROR (Status)) {
    DEBUGPRINT (CRITICAL, ("Failed to allocate memory for Rx desc ring: %r\n", Status));

    UndiDmaFreeCommonBuffer (AdapterInfo->PciIo, &TxRing->Mapping);
    return Status;
  }

  RxRing->NextToClean = 0;
  RxRing->NextToUse = 0;

  return EFI_SUCCESS;
}

/** Free memory resources for the Tx and Rx descriptors

 @param[in]  AdapterInfo  Pointer to the NIC data structure information
                             the UNDI driver is layering on

 @retval     EFI_SUCCESS            Tx/Rx ring descriptor resources freed successfully
 @retval     EFI_INVALID_PARAMETER  Memory pages count was not allocated with Allocate
                                    Buffer() on specified Tx desc. address
 @retval     EFI_INVALID_PARAMETER  Memory pages count was not allocated with Allocate
                                    Buffer() on specified Rx desc. address
**/
EFI_STATUS
IceFreeTxRxResources (
  IN DRIVER_DATA *AdapterInfo
  )
{
  EFI_STATUS Status;

  Status = UndiDmaFreeCommonBuffer (
             AdapterInfo->PciIo,
             &AdapterInfo->Vsi.TxRing.Mapping
             );

  if (EFI_ERROR (Status)) {
    DEBUGPRINT (
      CRITICAL, ("Unable to free memory for the Tx descriptor ring: %r\n",
      Status)
    );
    return Status;
  }

  Status = UndiDmaFreeCommonBuffer (
             AdapterInfo->PciIo,
             &AdapterInfo->Vsi.RxRing.Mapping
             );

  if (EFI_ERROR (Status)) {
    DEBUGPRINT (
      CRITICAL, ("Unable to free memory for the Rx descriptor ring: %r\n",
      Status)
    );
    return Status;
  }

  return Status;
}

/** Setup the initial LAN and VMDq switch.

   This adds the VEB into the internal switch, makes sure the main
   LAN VSI is connected correctly, allocates and connects all the
   VMDq VSIs, and sets the base queue index for each VSI.

   @param[in]   AdapterInfo   Pointer to the NIC data structure information
                             the UNDI driver is layering on

   @retval  EFI_SUCCESS        Successfull LAN and VMDq setup
   @retval  EFI_DEVICE_ERROR   Failed to get VSI params
   @retval  EFI_DEVICE_ERROR   VSI has not enough queue pairs
   @retval  EFI_DEVICE_ERROR   Failed to set filter control settings
   @retval  EFI_DEVICE_ERROR   add_macvlan AQ cmd failed
   @retval  EFI_DEVICE_ERROR   Failed to disable VLAN stripping
**/
EFI_STATUS
IceSetupPFSwitch (
  IN DRIVER_DATA *AdapterInfo
  )
{
  EFI_STATUS              Status;
  enum ice_status         IceStatus;
  struct ice_hw           *Hw;
  struct ice_port_info    *PortInfo;

  // 1 LAN queue for Traffic class 0
  UINT16                       LanQueue[ICE_MAX_TRAFFIC_CLASS] = {1};
  UINT8                        TcBitmap = 1;

  PortInfo = &AdapterInfo->Hw.port_info[0];
  Hw = &AdapterInfo->Hw;

  IceStatus = ICE_SUCCESS;
  Status = EFI_SUCCESS;


  // Get main VSI parameters
  IceStatus = IceGetVsiParams (AdapterInfo, &AdapterInfo->Vsi.VsiCtx);
  if (IceStatus != ICE_SUCCESS) {
    DEBUGPRINT (
      CRITICAL, ("IceGetVsiParams returned %d\n",
      IceStatus)
    );
    return EFI_DEVICE_ERROR;
  }

  DEBUGPRINT (
    INIT, ("VSI params: vsi_num=%d, sw_id=%d\n",
    AdapterInfo->Vsi.VsiCtx.vsi_num,
    AdapterInfo->Vsi.VsiCtx.info.sw_id)
  );

  AdapterInfo->Vsi.Id = AdapterInfo->Vsi.VsiCtx.vsi_num;

  // Store VSI parameters in VSI structure
  AdapterInfo->Vsi.Type = ICE_VSI_PF;
  AdapterInfo->Vsi.Flags = 0;
  AdapterInfo->Vsi.NumDesc = ICE_NUM_TX_RX_DESCRIPTORS;

  // Set pointer to the vsi context
  AdapterInfo->Hw.vsi_ctx[Hw->fw_vsi_num] = &AdapterInfo->Vsi.VsiCtx;

  // Update VSI
  AdapterInfo->Vsi.VsiCtx.vsi_num               = Hw->fw_vsi_num;
  AdapterInfo->Vsi.VsiCtx.info.sw_id            = Hw->port_info[0].sw_id;
  AdapterInfo->Vsi.VsiCtx.info.sw_flags         = ICE_AQ_VSI_SW_FLAG_SRC_PRUNE;
  AdapterInfo->Vsi.VsiCtx.info.sw_flags2        = ICE_AQ_VSI_SW_FLAG_LAN_ENA;
  AdapterInfo->Vsi.VsiCtx.info.inner_vlan_flags = ((ICE_AQ_VSI_INNER_VLAN_TX_MODE_ALL &
                                                    ICE_AQ_VSI_INNER_VLAN_TX_MODE_M) >>
                                                    ICE_AQ_VSI_INNER_VLAN_TX_MODE_S);
  AdapterInfo->Vsi.VsiCtx.info.valid_sections   |= ICE_AQ_VSI_PROP_VLAN_VALID;
  AdapterInfo->Vsi.VsiCtx.info.valid_sections   |= ICE_AQ_VSI_PROP_SW_VALID;
  AdapterInfo->Vsi.VsiCtx.info.inner_vlan_flags |= ICE_AQ_VSI_INNER_VLAN_TX_MODE_ALL;
  AdapterInfo->Vsi.VsiCtx.info.inner_vlan_flags |= ICE_AQ_VSI_INNER_VLAN_EMODE_NOTHING;

  IceStatus = ice_aq_update_vsi (Hw, &AdapterInfo->Vsi.VsiCtx, NULL);
  if (IceStatus != ICE_SUCCESS) {
    DEBUGPRINT (
      CRITICAL, ("ice_aq_update_vsi returned %d\n",
      IceStatus)
    );
    return EFI_DEVICE_ERROR;
  }

  IceStatus = ice_cfg_vsi_lan (
                Hw->port_info,
                Hw->fw_vsi_num,
                TcBitmap,
                LanQueue
              );
  if (IceStatus != ICE_SUCCESS) {
    DEBUGPRINT (
      CRITICAL, ("ice_cfg_vsi_lan returned %d\n",
      IceStatus)
    );
    return EFI_DEVICE_ERROR;
  }

  // Add RX rule to set FW VSI as default

  // Add TX rule to set FW VSI as default
  IceStatus = ice_cfg_dflt_vsi (
                PortInfo,
                AdapterInfo->Vsi.Id,
                TRUE,
                ICE_FLTR_TX
              );
  if (IceStatus != ICE_SUCCESS) {
    DEBUGPRINT (CRITICAL, ("ice_cfg_dflt_vsi ICE_FLTR_TX returned %d\n", IceStatus));
    return EFI_DEVICE_ERROR;
  }

  // Add RX rule to set FW VSI as default
  IceStatus = ice_cfg_dflt_vsi (
                PortInfo,
                AdapterInfo->Vsi.Id,
                TRUE,
                ICE_FLTR_RX
              );
  if (IceStatus != ICE_SUCCESS) {
    DEBUGPRINT (CRITICAL, ("ice_cfg_dflt_vsi ICE_FLTR_RX returned %d\n", IceStatus));
    return EFI_DEVICE_ERROR;
  }
#ifdef BMSM_MODE
  IceStatus = ice_set_vsi_promisc_on_port (
                Hw,
                AdapterInfo->Vsi.Id,
                ICE_PROMISC_UCAST_RX | ICE_PROMISC_UCAST_TX | ICE_PROMISC_BCAST_RX | ICE_PROMISC_BCAST_TX,
                0,
                Hw->port_info[0].lport // logical port
                );
  if (IceStatus != ICE_SUCCESS) {
    DEBUGPRINT (
      CRITICAL, ("ice_set_vsi_promisc on logical port %d returned %d\n",
      Hw->port_info[0].lport,
      IceStatus)
    );
    return EFI_DEVICE_ERROR;
  }
#else /* !BMSM_MODE */
  IceStatus = ice_set_vsi_promisc (Hw,
                AdapterInfo->Vsi.Id,
                ICE_PROMISC_UCAST_RX | ICE_PROMISC_UCAST_TX | ICE_PROMISC_BCAST_RX | ICE_PROMISC_BCAST_TX,
                0
              );

  if (IceStatus != ICE_SUCCESS) {
    DEBUGPRINT (
      CRITICAL, ("ice_set_vsi_promisc returned %d\n",
      IceStatus)
    );

    // Ignore status if switch rule is already set in FW VSI
    if (IceStatus != ICE_ERR_AQ_ERROR) {
      return EFI_DEVICE_ERROR;
    }
  }
#endif /* BMSM_MODE */
  AdapterInfo->CurrentPromiscuousMask = ICE_PROMISC_UCAST_RX | ICE_PROMISC_BCAST_RX;

#if !defined (SIMICS_SUPPORT)
#ifdef BMSM_MODE
  IceStatus = ice_aq_set_port_params (PortInfo, AdapterInfo->Vsi.Id, TRUE, TRUE, FALSE, NULL, NULL);
#else /* !BMSM_MODE */
  IceStatus = ice_aq_set_port_params (Hw->port_info, AdapterInfo->Vsi.Id, TRUE, TRUE, FALSE, NULL);
#endif /* BMSM_MODE */
  if (IceStatus != ICE_SUCCESS) {
    DEBUGPRINT (
      CRITICAL, ("ice_aq_set_port_params returned %d\n",
      IceStatus)
    );
    return EFI_DEVICE_ERROR;
  }
#endif /* !defined (SIMICS_SUPPORT) */
  return Status;
}

/** Enables link events (LSE) reporting on ARQ by FW for desired event causes.

   @param[in]   AdapterInfo   Pointer to the NIC data structure information
                             the UNDI driver is layering on

   @retval   EFI_SUCCESS             Link events enabled
   @retval   EFI_DEVICE_ERROR        Failed to set event mask
   @retval   EFI_DEVICE_ERROR        Failed to get link info & enable LSE
**/
EFI_STATUS
IceConfigureLinkEvents (
  IN DRIVER_DATA *AdapterInfo
  )
{
  enum ice_status       IceStatus;
  struct ice_hw        *Hw;
  UINT16                EventMask;
  struct ice_port_info *PortInfo;

  Hw = &AdapterInfo->Hw;
  PortInfo = &Hw->port_info[0];


  // currently we use only link up/down notifications
  EventMask = (UINT16) ~((UINT16) ICE_AQ_LINK_EVENT_UPDOWN);

  IceStatus = ice_aq_set_event_mask (
                Hw,
                PortInfo->lport,
                EventMask,
                NULL
              );
  if (IceStatus != ICE_SUCCESS) {
    DEBUGPRINT (CRITICAL, ("ice_aq_set_event_mask failed: %d\n", IceStatus));
    return EFI_DEVICE_ERROR;
  }

  // Call get_link_info AQ with LSE enable flag set, to enable Link Status Events on ARQ
  IceStatus = ice_aq_get_link_info (
                PortInfo,
                TRUE,
                NULL,
                NULL
              );
  if (IceStatus != ICE_SUCCESS) {
    DEBUGPRINT (CRITICAL, ("ice_aq_get_link_info failed: %d\n", IceStatus));
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}

/** This function performs PCI-E initialization for the device.

   @param[in]   AdapterInfo   Pointer to the NIC data structure information
                             the UNDI driver is layering on

   @retval   EFI_SUCCESS            PCI-E initialized successfully
   @retval   EFI_UNSUPPORTED        Failed to get original PCI attributes to save locally
   @retval   EFI_UNSUPPORTED        Failed to get supported PCI command options
   @retval   EFI_UNSUPPORTED        Failed to set PCI command options
**/
EFI_STATUS
IcePciInit (
  IN DRIVER_DATA *AdapterInfo
  )
{
  EFI_STATUS Status;
  UINT64     NewCommand;
  UINT64     Result;
  BOOLEAN    PciAttributesSaved;

  NewCommand = 0;
  Result = 0;

  PciAttributesSaved = FALSE;

  // Save original PCI attributes
  Status = AdapterInfo->PciIo->Attributes (
                                 AdapterInfo->PciIo,
                                 EfiPciIoAttributeOperationGet,
                                 0,
                                 &AdapterInfo->OriginalPciAttributes
                               );

  if (EFI_ERROR (Status)) {
    goto Error;
  }
  PciAttributesSaved = TRUE;

  // Get the PCI Command options that are supported by this controller.
  Status = AdapterInfo->PciIo->Attributes (
                                 AdapterInfo->PciIo,
                                 EfiPciIoAttributeOperationSupported,
                                 0,
                                 &Result
                               );

  DEBUGPRINT (INIT, ("Attributes supported %x\n", Result));

  if (!EFI_ERROR (Status)) {

    // Set the PCI Command options to enable device memory mapped IO,
    // port IO, and bus mastering.
    Status = AdapterInfo->PciIo->Attributes (
                                   AdapterInfo->PciIo,
                                   EfiPciIoAttributeOperationEnable,
                                   Result & (EFI_PCI_DEVICE_ENABLE | EFI_PCI_IO_ATTRIBUTE_DUAL_ADDRESS_CYCLE),
                                   &NewCommand
                                 );
  }
  if (EFI_ERROR (Status)) {
    DEBUGPRINT (CRITICAL, ("PciIo->Attributes returned %r\n", Status));
    goto Error;
  }

  AdapterInfo->PciIo->GetLocation (
                        AdapterInfo->PciIo,
                        &AdapterInfo->Segment,
                        &AdapterInfo->Bus,
                        &AdapterInfo->Device,
                        &AdapterInfo->Function
                      );

#if 0

  // Do not try to initialize hw on other than 0 function
  if (AdapterInfo->Function > 0) {
    Status = EFI_DEVICE_ERROR;
    goto Error;
  }
#endif /* 0 */

  // Read all the registers from the device's PCI Configuration space
  AdapterInfo->PciIo->Pci.Read (
                            AdapterInfo->PciIo,
                            EfiPciIoWidthUint32,
                            0,
                            MAX_PCI_CONFIG_LEN,
                            AdapterInfo->PciConfig
                          );

  return Status;

Error:
  if (PciAttributesSaved) {

    // Restore original PCI attributes
    AdapterInfo->PciIo->Attributes (
                          AdapterInfo->PciIo,
                          EfiPciIoAttributeOperationSet,
                          AdapterInfo->OriginalPciAttributes,
                          NULL
                        );
  }

  return Status;
}

/** Reads MAC address into port_info->mac.lan_addr structure

   @param[in]   AdapterInfo   Pointer to the NIC data structure information
                              the UNDI driver is layering on

   @retval    EFI_SUCCESS       MAC address read successfully
   @retval    EFI_DEVICE_ERROR  Failed to get MAC address
**/
EFI_STATUS
IceReadMacAddress (
  IN DRIVER_DATA *AdapterInfo
  )
{
  enum ice_status      IceStatus;
  struct ice_hw       *Hw = &AdapterInfo->Hw;
  struct ice_aqc_manage_mac_read_resp MacReadResponse[2];
  UINT16 ResponseLength = sizeof (MacReadResponse);

  // Get current MAC Address using the shared code function
  IceStatus = ice_aq_manage_mac_read (Hw, MacReadResponse, ResponseLength, NULL);
  if (IceStatus != ICE_SUCCESS) {
    DEBUGPRINT (CRITICAL, ("ice_aq_manage_mac_read returned %d\n", IceStatus));
    return EFI_DEVICE_ERROR;
  }

  DEBUGPRINT (
    INIT, ("MAC Address = %02x:%02x:%02x:%02x:%02x:%02x\n",
    Hw->port_info->mac.lan_addr[0],
    Hw->port_info->mac.lan_addr[1],
    Hw->port_info->mac.lan_addr[2],
    Hw->port_info->mac.lan_addr[3],
    Hw->port_info->mac.lan_addr[4],
    Hw->port_info->mac.lan_addr[5])
  );

  return EFI_SUCCESS;
}
/** Performs HW initialization from child side

   Initializes HMC structure, sets flow control, setups PF switch,
   setups and configures Tx/Rx resources and queues, enables Tx/Rx rings

   @param[in]   AdapterInfo   Pointer to the NIC data structure information
                              the UNDI driver is layering on

   @retval    EFI_SUCCESS       HW initialized successfully
   @retval    EFI_DEVICE_ERROR  Failed to initialize HMC structure for LAN function
   @retval    EFI_DEVICE_ERROR  Failed to configure HMC
   @retval    EFI_DEVICE_ERROR  Failed to setup PF switch
   @retval    EFI_OUT_OF_RESOURCES  Failed to setup Tx/Rx resources
   @retval    EFI_DEVICE_ERROR  Failed to configure Tx/Rx queues
   @retval    EFI_OUT_OF_RESOURCES  Failed to configure Tx/Rx queues
   @retval    EFI_DEVICE_ERROR  Failed to configure link events
**/
EFI_STATUS
IceInitHw (
  IN DRIVER_DATA *AdapterInfo
  )
{

  EFI_STATUS            Status;
  struct ice_hw         *Hw;

  Hw = &AdapterInfo->Hw;


  Status = IceSetupPFSwitch (AdapterInfo);
  if (EFI_ERROR (Status)) {
    DEBUGPRINT (CRITICAL, ("IceSetupPFSwitch returned %r\n", Status));
    return Status;
  }

  Status = IceSetupTxRxResources (AdapterInfo);
  if (EFI_ERROR (Status)) {
    DEBUGPRINT (CRITICAL, ("IceSetupTxRxResources returned %r\n", Status));
    return Status;
  }

  Status = IceConfigureTxRxQueues (AdapterInfo);
  if (EFI_ERROR (Status)) {
    DEBUGPRINT (CRITICAL, ("IceConfigureTxRxQueues returned %r\n", Status));
    return Status;
  }

  IceReceiveStart (AdapterInfo);

  // Initialize flags for GET_STATUS_RECEIVE/TRANSMIT reporting in UndiStatus
  AdapterInfo->RxPacketPending = FALSE;
  AdapterInfo->TxPacketPending = FALSE;
  AdapterInfo->LastTxDescReported = AdapterInfo->Vsi.TxRing.Count - 1;
  AdapterInfo->LastRxDescReported = AdapterInfo->Vsi.RxRing.Count - 1;

  Status = IceConfigureLinkEvents (AdapterInfo);
  if (EFI_ERROR (Status)) {
    DEBUGPRINT (CRITICAL, ("IceConfigureLinkEvents returned %r\n", Status));
    return Status;
  }


  AdapterInfo->HwInitialized = TRUE;

  return Status;

}

/** Performs IceInitHw function for UNDI interface

   @param[in]   AdapterInfo   Pointer to the NIC data structure information
                             the UNDI driver is layering on

   @retval    PXE_STATCODE_SUCCESS   HW initialized successfully
   @retval    PXE_STATCODE_NOT_STARTED  Failed to initialize HW
**/
PXE_STATCODE
IceInitialize (
  IN DRIVER_DATA *AdapterInfo
  )
{

#ifndef AVOID_HW_REINITIALIZATION
  EFI_STATUS Status;
#endif /* AVOID_HW_REINITIALIZATION */
  PXE_STATCODE PxeStatcode;

  DEBUGPRINT (INIT, ("Entering IceInitialize\n"));

  PxeStatcode = PXE_STATCODE_SUCCESS;

  // Do not try to initialize hw again when it is already initialized
  if (AdapterInfo->HwInitialized == FALSE) {
    DEBUGPRINT (INIT, ("Hw is not initialized, calling IceInitHw\n"));
#ifndef AVOID_HW_REINITIALIZATION
    Status = IceInitHw (AdapterInfo);
    if (EFI_ERROR (Status)) {
      DEBUGPRINT (CRITICAL, ("IceInitHw returns %r\n", Status));
      PxeStatcode = PXE_STATCODE_NOT_STARTED;
    }
#endif /* AVOID_HW_REINITIALIZATION */
  }
  AdapterInfo->DriverBusy = FALSE;
  return PxeStatcode;
}

/** Reverts the operations performed in IceInitHw. Stops HW from child side

   @param[in]   AdapterInfo   Pointer to the NIC data structure information
                              the UNDI driver is layering on

   @retval   PXE_STATCODE_SUCCESS   HW is already not initialized
   @retval   PXE_STATCODE_SUCCESS   HW successfully stopped
**/
PXE_STATCODE
IceShutdown (
  IN DRIVER_DATA *AdapterInfo
  )
{

  PXE_STATCODE PxeStatcode;
#ifndef AVOID_HW_REINITIALIZATION
  enum ice_status       IceStatus = ICE_SUCCESS;
  struct ice_hw         *Hw;
  struct ice_port_info  *PortInfo;
#endif  /* AVOID_HW_REINITIALIZATION */
  DEBUGPRINT (INIT, ("Entering IceShutdown\n"));

#if (0)
  if (AdapterInfo->Hw.bus.func == 1) {
    DumpInternalFwHwData (AdapterInfo);
  }

#endif /* (0) */

  if (!AdapterInfo->HwInitialized) {
    PxeStatcode = PXE_STATCODE_SUCCESS;
    return PxeStatcode;
  }

#ifndef AVOID_HW_REINITIALIZATION
  Hw = &AdapterInfo->Hw;
  PortInfo = &AdapterInfo->Hw.port_info[0];

  // For proper shutdown first we need to clear
  // specified promiscuous mode(s) for given VSI.
  // Then remove TX and RX rules.
#ifdef BMSM_MODE
  IceStatus = ice_clear_vsi_promisc_on_port (
                Hw,
                AdapterInfo->Vsi.Id,
                ICE_PROMISC_UCAST_RX | ICE_PROMISC_UCAST_TX | ICE_PROMISC_BCAST_RX | ICE_PROMISC_BCAST_TX,
                0,
                Hw->port_info[0].lport
                );
  if (IceStatus != ICE_SUCCESS) {
    DEBUGPRINT (CRITICAL, ("ice_clear_vsi_promisc on logical port %d returned %d\n", Hw->port_info[0].lport, IceStatus));
  }
#else /* !BMSM_MODE */
  IceStatus = ice_clear_vsi_promisc (
                Hw,
                AdapterInfo->Vsi.Id,
                ICE_PROMISC_UCAST_RX | ICE_PROMISC_UCAST_TX | ICE_PROMISC_BCAST_RX | ICE_PROMISC_BCAST_TX,
                0
              );
  if (IceStatus != ICE_SUCCESS) {
    DEBUGPRINT (CRITICAL, ("ice_clear_vsi_promisc returned %d\n", IceStatus));
  }
#endif /* BMSM_MODE */

  // Remove RX rule to set FW VSI as default
  IceStatus = ice_cfg_dflt_vsi (
                PortInfo,
                AdapterInfo->Vsi.Id,
                FALSE,
                ICE_FLTR_TX
              );
  if (IceStatus != ICE_SUCCESS) {
    DEBUGPRINT (CRITICAL, ("ice_cfg_dflt_vsi ICE_FLTR_TX FALSE returned %d\n", IceStatus));
  }

  IceStatus = ice_cfg_dflt_vsi (
                PortInfo,
                AdapterInfo->Vsi.Id,
                FALSE,
                ICE_FLTR_RX
              );
  if (IceStatus != ICE_SUCCESS) {
    DEBUGPRINT (CRITICAL, ("ice_cfg_dflt_vsi ICE_FLTR_RX FALSE returned %d\n", IceStatus));
  }

  IceStatus = IceReceiveStop (AdapterInfo);
  if (IceStatus != ICE_SUCCESS) {
    DEBUGPRINT (CRITICAL, ("IceReceiveStop returned %d\n", IceStatus));
  }

  IceStatus = IceFreeTxRxQueues (AdapterInfo);
  if (IceStatus != ICE_SUCCESS) {
    DEBUGPRINT (CRITICAL, ("IceFreeTxRxQueues returned %d\n", IceStatus));
  }

  IceStatus = IceFreeTxRxResources (AdapterInfo);
  if (IceStatus != ICE_SUCCESS) {
    DEBUGPRINT (CRITICAL, ("IceFreeTxRxResources returned %d\n", IceStatus));
  }

  //We need to remove VSI in order to clean scheduler node structure leftovers
  IceStatus = ice_rm_vsi_lan_cfg (Hw->port_info, AdapterInfo->Vsi.Id);
  if (IceStatus != ICE_SUCCESS) {
    DEBUGPRINT (CRITICAL, ("ice_rm_vsi_lan_cfg returned %d\n", IceStatus));
  }

  AdapterInfo->HwInitialized = FALSE;
#endif  /* AVOID_HW_REINITIALIZATION */
  PxeStatcode = PXE_STATCODE_SUCCESS;
  return PxeStatcode;
}

/** Performs HW reset by reinitialization

   @param[in]   AdapterInfo   Pointer to the NIC data structure information
                              the UNDI driver is layering on

   @retval   PXE_STATCODE_SUCCESS      Successfull HW reset
   @retval   PXE_STATCODE_NOT_STARTED  Failed to initialize HW
**/
PXE_STATCODE
IceReset (
  IN DRIVER_DATA *AdapterInfo
  )
{

  PXE_STATCODE PxeStatcode;
#ifndef AVOID_HW_REINITIALIZATION
  EFI_STATUS Status;
#endif /* AVOID_HW_REINITIALIZATION */

  DEBUGPRINT (INIT, ("Entering IceReset\n"));

  // Do not reinitialize the adapter when it has already been initialized
  // This saves the time required for initialization
  if (!AdapterInfo->HwInitialized) {
#ifndef AVOID_HW_REINITIALIZATION
    Status = IceInitHw (AdapterInfo);
    if (EFI_ERROR (Status)) {
      DEBUGPRINT (CRITICAL, ("XgbeInitHw returns %r\n", Status));
      return PXE_STATCODE_NOT_STARTED;
    }
#endif /* AVOID_HW_REINITIALIZATION */
  } else {
    DEBUGPRINT (ICE, ("Skipping adapter reset\n"));
  }

  PxeStatcode = PXE_STATCODE_SUCCESS;
  return PxeStatcode;
}

#ifdef LEGACY_IN_BACKGROUND
/** This function checks if any  other instance of driver is loaded on this PF by
   reading PFGEN_DRUN1 CPK register.

   If not it writes the bit in the register to let know other components that
   the PF is in use.

   @param[in]   AdapterInfo   Pointer to the NIC data structure information
                             the UNDI driver is layering on

   @retval   TRUE   The PF is free to use for Tx/Rx
   @retval   FALSE  The PF cannot be used for Tx/Rx
**/
BOOLEAN
IceAquireControllerHw (
  IN DRIVER_DATA *AdapterInfo
  )
{

  UINT32          RegValue;
  struct  ice_hw  *Hw;

  Hw = &AdapterInfo->Hw;


  RegValue = rd32 (&AdapterInfo->Hw, PFGEN_DRUN + 4 * Hw->bus.func);

  if (RegValue & PFGEN_DRUN_DRVUNLD_M) {

    // bit set means other driver is loaded on this pf
    return FALSE;
  }

  RegValue |= PFGEN_DRUN_DRVUNLD_M;
  wr32 (&AdapterInfo->Hw, PFGEN_DRUN + 4 * Hw->bus.func, RegValue);

  return TRUE;
}

/** Release this PF by clearing the bit in PFGEN_DRUN CPK register.

   @param[in]   AdapterInfo   Pointer to the NIC data structure information
                             the UNDI driver is layering on

   @return   PFGEN_DRUN driver unload bit is cleared
**/
VOID
IceReleaseControllerHw (
  IN DRIVER_DATA *AdapterInfo
  )
{
  UINT32         RegValue;
  struct ice_hw  *Hw;

  Hw = &AdapterInfo->Hw;

  RegValue = rd32 (&AdapterInfo->Hw, PFGEN_DRUN + 4 * Hw->bus.func);
  RegValue &= ~PFGEN_DRUN_DRVUNLD_M;
  wr32 (&AdapterInfo->Hw, PFGEN_DRUN + 4 * Hw->bus.func, RegValue);
}
#endif /* LEGACY_IN_BACKGROUND */

/** Wait for the Firmware to initialize

   @param[in]  AdapterInfo   Pointer to the driver data
   @param[in]  Timeout       Timeout in 1s units

   @retval     EFI_TIMEOUT   Firmware initialization timeout
   @retval     EFI_SUCCESS   Successful wait
**/
EFI_STATUS
AwaitFwInit (
  IN DRIVER_DATA  *AdapterInfo,
  IN UINT8        Timeout
  )
{
  UINT32          RegisterValue = 0;
  UINT8           WaitCnt = 0;

#define PHY_FW_DOWNLOAD_STARTED BIT (30)

  ASSERT (AdapterInfo != NULL);

  do {
    RegisterValue = IceRead32 (AdapterInfo, GL_MNG_FWSM);
    if ((RegisterValue & PHY_FW_DOWNLOAD_STARTED) == 0) {
      return EFI_SUCCESS;
    }
    // Wait 1 second.
    gBS->Stall (1000000);
    WaitCnt++;
  } while (WaitCnt <= Timeout);

  DEBUGPRINT (CRITICAL, ("Timeout waiting for the FW to initialize\n"));

  return EFI_TIMEOUT;
}

/** This function is called as early as possible during driver start to ensure the
   hardware has enough time to autonegotiate when the real SNP device initialize call
   is made.

   @param[in]   AdapterInfo   Pointer to the NIC data structure information
                              the UNDI driver is layering on

   @retval   EFI_SUCCESS            First time init end up successfully
   @retval   EFI_OUT_OF_RESOURCES   Not enough memory to initialize driver data.
   @retval   EFI_DEVICE_ERROR       Failed to init hw
   @retval   EFI_ACCESS_DENIED      UNDI is not enabled
   @retval   EFI_OUT_OF_RESOURCES   Failed to allocate memory for PortInfo
**/
EFI_STATUS
IceFirstTimeInit (
  IN DRIVER_DATA *AdapterInfo
  )
{
  PCI_CONFIG_HEADER *PciConfigHeader;
  enum ice_status   IceStatus;
  EFI_STATUS        Status;
  ICE_RING          *RxRing;
  ICE_RING          *TxRing;
  struct ice_hw     *Hw;
  UINT32            PortNumber = 0;
  LINK_STATE        LinkState;

  Hw = &AdapterInfo->Hw;
  Hw->back = AdapterInfo;

  AdapterInfo->DriverBusy         = FALSE;
  AdapterInfo->MediaStatusChecked = FALSE;
  AdapterInfo->LastMediaStatus    = FALSE;
  AdapterInfo->FwSupported        = TRUE;

  Hw->bus.device = (UINT16) AdapterInfo->Device;
  Hw->bus.func =   (UINT16) AdapterInfo->Function;

  PciConfigHeader = (PCI_CONFIG_HEADER *) &AdapterInfo->PciConfig[0];

  DEBUGPRINT (INIT, ("PCI Command Register = %X\n", PciConfigHeader->Command));
  DEBUGPRINT (INIT, ("PCI Status Register = %X\n", PciConfigHeader->Status));
  DEBUGPRINT (INIT, ("PCI VendorID = %X\n", PciConfigHeader->VendorId));
  DEBUGPRINT (INIT, ("PCI DeviceID = %X\n", PciConfigHeader->DeviceId));
  DEBUGPRINT (INIT, ("PCI SubVendorID = %X\n", PciConfigHeader->SubVendorId));
  DEBUGPRINT (INIT, ("PCI SubSystemID = %X\n", PciConfigHeader->SubSystemId));
  DEBUGPRINT (INIT, ("PCI Bus = %X\n", AdapterInfo->Bus));
  DEBUGPRINT (INIT, ("PCI Device = %X\n", AdapterInfo->Device));
  DEBUGPRINT (INIT, ("PCI Function = %X\n", AdapterInfo->Function));

  ZeroMem (AdapterInfo->BroadcastNodeAddress, PXE_MAC_LENGTH);
  SetMem (AdapterInfo->BroadcastNodeAddress, PXE_HWADDR_LEN_ETHER, 0xFF);
  ZeroMem (&AdapterInfo->Vsi.CurrentMcastList, sizeof (AdapterInfo->Vsi.CurrentMcastList));

  // Initialize all parameters needed for the shared code
  Hw->hw_addr                = (UINT8 *) (UINTN) PciConfigHeader->BaseAddressReg0;
  Hw->vendor_id              = PciConfigHeader->VendorId;
  Hw->device_id              = PciConfigHeader->DeviceId;
  Hw->revision_id            = (UINT8) PciConfigHeader->RevId;
  Hw->subsystem_vendor_id    = PciConfigHeader->SubVendorId;
  Hw->subsystem_device_id    = PciConfigHeader->SubSystemId;
  Hw->revision_id            = (UINT8) PciConfigHeader->RevId;

  AdapterInfo->PciClass    = (UINT8) ((PciConfigHeader->ClassId & PCI_CLASS_MASK) >> 8);
  AdapterInfo->PciSubClass = (UINT8) (PciConfigHeader->ClassId) & PCI_SUBCLASS_MASK;

  // Find out if this function is already used by legacy component
#ifdef LEGACY_IN_BACKGROUND
  AdapterInfo->UndiEnabled = IceAquireControllerHw (AdapterInfo);
  DEBUGPRINT (INIT, ("IceAquireControllerHw returned %d\n", AdapterInfo->UndiEnabled));
#else /* NOT LEGACY_IN_BACKGROUND */
  AdapterInfo->UndiEnabled = TRUE;
#endif /* LEGACY_IN_BACKGROUND */

  // Setup AQ initialization parameters: 32 descriptor rings, 4kB buffers
  Hw->adminq.num_sq_entries = 32;
  Hw->adminq.num_rq_entries = 32;
  Hw->adminq.rq_buf_size = 4096;
  Hw->adminq.sq_buf_size = 4096;

  DEBUGPRINT (INIT, ("Initializing PF: %d\n", Hw->bus.func));

  if (IsRecoveryMode (AdapterInfo)) {
    // Firmware is in recovery mode. Refrain from further initialization
    // and report error status thru the Driver Health Protocol
    DEBUGPRINT (CRITICAL, ("FW is in recovery mode, skip further initialization\n"));
    AdapterInfo->FwSupported = FALSE;
    return EFI_UNSUPPORTED;
  }

  // Wait up to 120 seconds for the initialization
  Status = AwaitFwInit (AdapterInfo, 120);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  IceStatus = ice_init_hw (Hw);

  if (IceStatus != ICE_SUCCESS) {

    DEBUGPRINT (CRITICAL, ("ice_init_hw returned: %d\n", IceStatus));
    if (IceStatus == ICE_ERR_FW_API_VER) {

      // Firmware version is newer then expected. Refrain from further initialization
      // and report error status thru the Driver Health Protocol
      DEBUGPRINT (CRITICAL, ("Incompatible firmware - driver stops initialization\n"));
      AdapterInfo->FwSupported = FALSE;
      Status = EFI_UNSUPPORTED;
    } else {
      Status = EFI_DEVICE_ERROR;
    }
    goto ErrorReleaseController;
  }

  Status = GetLinkState (AdapterInfo, &LinkState);
  if (EFI_ERROR (Status)) {
    DEBUGPRINT (CRITICAL, ("GetLinkState returned %r\n", Status));
    goto ErrorReleaseController;
  }

  if (LinkState == LINK_STATE_DOWN_SW_FORCED) {
    Status = ResetLinkConfig (AdapterInfo);
    if (EFI_ERROR (Status)) {
      DEBUGPRINT (CRITICAL, ("ResetLinkConfig returned %r\n", Status));
      goto ErrorReleaseController;
    }
  }

  EnablePxeModeDescFetch (AdapterInfo);


  // Set Port addr
  CopyMem (Hw->port_info[0].mac.port_addr, Hw->port_info[0].mac.perm_addr, 6);

  DEBUGPRINT (
    INIT, ("Mac %02x:%02x:%02x:%02x:%02x:%02x\n",
    Hw->port_info[0].mac.perm_addr[0],
    Hw->port_info[0].mac.perm_addr[1],
    Hw->port_info[0].mac.perm_addr[2],
    Hw->port_info[0].mac.perm_addr[3],
    Hw->port_info[0].mac.perm_addr[4],
    Hw->port_info[0].mac.perm_addr[5])
  );

  DEBUGPRINT (
    INIT, ("FwVer: %x.%x.%x\n API: %x.%x\n",
    Hw->fw_maj_ver,
    Hw->fw_min_ver,
    Hw->fw_build,
    Hw->api_maj_ver,
    Hw->api_min_ver)
  );

  AdapterInfo->TxRxDescriptorCount = IceGetTxRxDescriptorsCount (Hw);
  DEBUGPRINT (INIT, ("IceGetTxRxDescriptorsCount returned %d\n", AdapterInfo->TxRxDescriptorCount));

  RxRing = &AdapterInfo->Vsi.RxRing;

  RxRing->PhysicalBuffers = AllocateZeroPool (sizeof (*RxRing->PhysicalBuffers) * AdapterInfo->TxRxDescriptorCount);
  if (RxRing->PhysicalBuffers == NULL) {
    DEBUGPRINT (CRITICAL, ("AllocateZeroPool for RxRing->PhysicalBuffers failed.\n"));
    Status = EFI_OUT_OF_RESOURCES;
    goto ErrorReleaseController;
  }

  RxRing->UnmappedBuffers = AllocateZeroPool (sizeof (*RxRing->UnmappedBuffers) * AdapterInfo->TxRxDescriptorCount);
  if (RxRing->UnmappedBuffers == NULL) {
    DEBUGPRINT (CRITICAL, ("AllocateZeroPool for RxRing->UnmappedBuffers failed.\n"));
    Status = EFI_OUT_OF_RESOURCES;
    goto ErrorReleaseController;
  }

  // For RX buffers we don't allocate RxBufferMapping pool for received data here,
  // but in IceConfigureRxQueues, and assign proper resources to each descriptor.

  TxRing = &AdapterInfo->Vsi.TxRing;

  TxRing->TxBufferMappings = AllocateZeroPool (sizeof (*TxRing->TxBufferMappings) * AdapterInfo->TxRxDescriptorCount);
  if (TxRing->TxBufferMappings == NULL) {
    DEBUGPRINT (CRITICAL, ("AllocateZeroPool for TxRing->TxBufferMappings failed.\n"));
    Status = EFI_OUT_OF_RESOURCES;
    goto ErrorReleaseController;
  }

  // Read port number for current PF and save its value
  PortNumber = IceRead32 (
                 AdapterInfo,
                 PFGEN_PORTNUM
               );
  PortNumber &= PFGEN_PORTNUM_PORT_NUM_M;

  AdapterInfo->PhysicalPortNumber = PortNumber;

  if (AdapterInfo->UndiEnabled) {
    DEBUGPRINT (INIT, ("UNDI Enabled\n"));
    return EFI_SUCCESS;
  } else {
    DEBUGPRINT (CRITICAL, ("UNDI is not enabled!\n"));
    return EFI_ACCESS_DENIED;
  }

ErrorReleaseController:
  if (AdapterInfo->Vsi.RxRing.PhysicalBuffers != NULL) {
    gBS->FreePool ((VOID *) AdapterInfo->Vsi.RxRing.PhysicalBuffers);
  }
  if (AdapterInfo->Vsi.RxRing.UnmappedBuffers != NULL) {
    gBS->FreePool ((VOID *) AdapterInfo->Vsi.RxRing.UnmappedBuffers);
  }
  if (AdapterInfo->Vsi.TxRing.TxBufferMappings != NULL) {
    gBS->FreePool ((VOID *) AdapterInfo->Vsi.TxRing.TxBufferMappings);
  }

#ifdef LEGACY_IN_BACKGROUND
  if (AdapterInfo->UndiEnabled) {
    IceReleaseControllerHw (AdapterInfo);
    AdapterInfo->UndiEnabled = FALSE;
  }
#endif /* LEGACY_IN_BACKGROUND */
  return Status;
}

#ifdef VPD_CONFIG_ACCESS
/** Reads VPD region

   @param[in]   AdapterInfo   Pointer to the NIC data structure information
                              the UNDI driver is layering on
   @param[out]  Buffer       Pointer to buffer for resulting VPD data
   @param[in]   BufferSize   Size of passed buffer

   @retval  EFI_SUCCESS  VPD region read successfully
**/
EFI_STATUS
ReadVpdRegion (
  IN  DRIVER_DATA *AdapterInfo,
  OUT UINT32      *Buffer,
  IN  UINTN        BufferSize
  )
{
#if 0
  UINTN  i;
  UINT16 Address;
  UINT32 Data;

  for (i = 0; i < BufferSize; i++) {
    Address = VPD_READ_MASK;
    Address |= sizeof (UINT32) * i;
    AdapterInfo->PciIo->Pci.Write (
                              AdapterInfo->PciIo,
                              EfiPciIoWidthUint16,
                              VPDADDR,
                              1,
                              &Address
                            );
    do {
      AdapterInfo->PciIo->Pci.Read (
                                AdapterInfo->PciIo,
                                EfiPciIoWidthUint16,
                                VPDADDR,
                                1,
                                &Address
                              );
    } while ((Address & VPD_FLAG_MASK) != VPD_WRITE_MASK);

    AdapterInfo->PciIo->Pci.Read (
                              AdapterInfo->PciIo,
                              EfiPciIoWidthUint32,
                              VPDDATA,
                              1,
                              &Data
                            );
    Buffer[i] = Data;
  }
#endif
  return EFI_SUCCESS;
}

/** Writes VPD region

   @param[in]   AdapterInfo   Pointer to the NIC data structure information
                             the UNDI driver is layering on
   @param[in]   Buffer       Pointer to buffer with VPD data to write
   @param[in]   BufferSize   Size of buffer

   @retval  EFI_SUCCESS   VPD region written successfully
**/
EFI_STATUS
WriteVpdRegion (
  IN DRIVER_DATA *AdapterInfo,
  IN UINT32      *Buffer,
  IN UINTN        BufferSize
  )
{
#if 0
  UINTN  i;
  UINT16 Address;
  UINT32 Data;

  for (i = 0; i < BufferSize; i++) {

    Address = VPD_WRITE_MASK;
    Address |= sizeof (UINT32) * i;
    Data = Buffer[i];
    AdapterInfo->PciIo->Pci.Write (
                              AdapterInfo->PciIo,
                              EfiPciIoWidthUint32,
                              VPDDATA,
                              1,
                              &Data
                            );


    AdapterInfo->PciIo->Pci.Write (
                              AdapterInfo->PciIo,
                              EfiPciIoWidthUint16,
                              VPDADDR,
                              1,
                              &Address
                            );
    do {
      AdapterInfo->PciIo->Pci.Read (
                                AdapterInfo->PciIo,
                                EfiPciIoWidthUint16,
                                VPDADDR,
                                1,
                                &Address
                              );
    } while ((Address & VPD_FLAG_MASK) != VPD_READ_MASK);
  }
#endif
  return EFI_SUCCESS;
}
#endif /* VPD_CONFIG_ACCESS */

/** This function calls the MemIo callback to read a dword from the device's
   address space

   @param[in]   AdapterInfo   Pointer to the NIC data structure information
                             the UNDI driver is layering on
   @param[in]   Port         Address to read from

   @return      The data read from the port.
**/
UINT32
IceRead32 (
  IN DRIVER_DATA *AdapterInfo,
  IN UINT32       Port
  )
{
  UINT32 Results;

#ifdef CONFIG_ACCESS_TO_CSRS
  {
    UINT32 ConfigSpaceAddress;

    ConfigSpaceAddress = Port | 0x80000000;
    AdapterInfo->PciIo->Pci.Write (
                              AdapterInfo->PciIo,
                              EfiPciIoWidthUint32,
                              IOADDR,
                              1,
                              &ConfigSpaceAddress
                            );
    AdapterInfo->PciIo->Pci.Read (
                              AdapterInfo->PciIo,
                              EfiPciIoWidthUint32,
                              IODATA,
                              1,
                              &Results
                            );
    ConfigSpaceAddress = 0;
    AdapterInfo->PciIo->Pci.Write (
                              AdapterInfo->PciIo,
                              EfiPciIoWidthUint32,
                              IOADDR,
                              1,
                              &ConfigSpaceAddress
                            );
  }
#else /* NOT CONFIG_ACCESS_TO_CSRS */
  MemoryFence ();

  AdapterInfo->PciIo->Mem.Read (
                            AdapterInfo->PciIo,
                            EfiPciIoWidthUint32,
                            0,
                            Port,
                            1,
                            (VOID *) (&Results)
                          );
  MemoryFence ();
#endif  /* CONFIG_ACCESS_TO_CSRS */
  return Results;
}

/** This function calls the MemIo callback to write a word from the device's
   address space

   @param[in]   AdapterInfo   Pointer to the NIC data structure information
                             the UNDI driver is layering on
   @param[in]   Port         Address to write to
   @param[in]   Data         Data to write to Port

   @return    Data written to address in device's space
**/
VOID
IceWrite32 (
  IN DRIVER_DATA *AdapterInfo,
  IN UINT32       Port,
  IN UINT32       Data
  )
{
  UINT32 Value;

  Value = Data;

#ifdef CONFIG_ACCESS_TO_CSRS
  {
    UINT32 ConfigSpaceAddress;

    ConfigSpaceAddress = Port | 0x80000000;
    AdapterInfo->PciIo->Pci.Write (
                              AdapterInfo->PciIo,
                              EfiPciIoWidthUint32,
                              IOADDR,
                              1,
                              &ConfigSpaceAddress
                            );
    AdapterInfo->PciIo->Pci.Write (
                              AdapterInfo->PciIo,
                              EfiPciIoWidthUint32,
                              IODATA,
                              1,
                              &Value
                            );
    ConfigSpaceAddress = 0;
    AdapterInfo->PciIo->Pci.Write (
                              AdapterInfo->PciIo,
                              EfiPciIoWidthUint32,
                              IOADDR,
                              1,
                              &ConfigSpaceAddress
                            );
  }
#else /* NOT CONFIG_ACCESS_TO_CSRS */
  MemoryFence ();

  AdapterInfo->PciIo->Mem.Write (
                            AdapterInfo->PciIo,
                            EfiPciIoWidthUint32,
                            0,
                            Port,
                            1,
                            (VOID *) (&Value)
                          );

  MemoryFence ();
#endif /* CONFIG_ACCESS_TO_CSRS */
}

/** This function calls the IceRead32 twice to read a qword from the device's
   address space

   @param[in]   AdapterInfo   Pointer to the NIC data structure information
                             the UNDI driver is layering on
   @param[in]   Port         Address to read from

   @return      The data read from the port.
**/
UINT64
IceRead64 (
  IN DRIVER_DATA *AdapterInfo,
  IN UINT32       Port
  )
{
  UINT64 Results;
  Results = (UINT64)IceRead32 (AdapterInfo, Port);
  Results += (UINT64)IceRead32 (AdapterInfo, Port+1) << 32;
  return Results;
}

/** This function calls the IceWrite32 twice to write a qword to the device's
   address space

   @param[in]   AdapterInfo   Pointer to the NIC data structure information
                             the UNDI driver is layering on
   @param[in]   Port         Address to write to
   @param[in]   Data         Data to write to Port

   @return    Data written to address in device's space
**/
VOID
IceWrite64 (
  IN DRIVER_DATA *AdapterInfo,
  IN UINT32       Port,
  IN UINT64       Data
  )
{
  UINT32 DataLow;
  UINT32 DataHigh;

  DataLow = (UINT32)(Data & 0xFFFFFFFF);
  IceWrite32 (AdapterInfo, Port, DataLow);

  DataHigh = (UINT32)((Data >> 32) & 0xFFFFFFFF);
  IceWrite32 (AdapterInfo, Port+1, DataHigh);
}

/** Delays execution of next instructions for MicroSeconds microseconds

   @param[in]   AdapterInfo   Pointer to the NIC data structure information
                             the UNDI driver is layering on
   @param[in]   MicroSeconds   Time to delay in Microseconds.

   @retval   NONE
**/
VOID
DelayInMicroseconds (
  IN DRIVER_DATA *AdapterInfo,
  IN UINT32       MicroSeconds
  )
{
  if (AdapterInfo->Delay != NULL) {
    (*AdapterInfo->Delay) (AdapterInfo->UniqueId, MicroSeconds);
  } else {
    gBS->Stall (MicroSeconds);
  }
}

/** OS specific memory alloc for shared code

   @param[in]   Hw    pointer to the HW structure
   @param[in]   size  size of memory requested

   @retval   Pointer to allocated memory (0 if failed)
**/
void *
IceAllocateMem (
  struct ice_hw          *Hw,
  UINT32                  Size
  )
{
  VOID * MemPtr = NULL;
  if (Size == 0) {
    return 0;
  }
  MemPtr = AllocatePool (Size);

  if (MemPtr != NULL)
  {
    ZeroMem (MemPtr, Size);
    return MemPtr;
  } else {
    DEBUGPRINT (CRITICAL, ("Error: Requested: %d\n", Size));
    return NULL;
  }
}

/** OS specific memory free for shared code

   @param[in]   Mem   ptr to mem struct to free

   @retval   none
**/
VOID
IceFreeMem (
  VOID *MemPtr
  )
{

  if (MemPtr == NULL) {
    return;
  }
  if (!mExitBootServicesTriggered) {
    FreePool (MemPtr);
  }
  return;
}

/** OS specific spinlock init for shared code.

   @param[in]   Sp   pointer to a spinlock declared in driver space
**/
VOID
IceInitSpinLock (
  struct ice_lock *Sp
  )
{
  EfiInitializeLock (&Sp->SpinLock, TPL_NOTIFY);
}

/** OS specific spinlock acquire for shared code.

   @param[in]   Sp   pointer to a spinlock declared in driver space
**/
VOID
IceAcquireSpinLock (
  struct ice_lock *Sp
  )
{
  EfiAcquireLockOrFail (&Sp->SpinLock);
}

/** OS specific spinlock release for shared code.

   @param[in]   Sp   pointer to a spinlock declared in driver space
**/
VOID
IceReleaseSpinLock (
  struct ice_lock *Sp
  )
{
  EfiReleaseLock (&Sp->SpinLock);
}

/** OS specific spinlock destroy for shared code.

   @param[in]   Sp   pointer to a spinlock declared in driver space
**/
VOID
IceDestroySpinLock (
  struct ice_lock *Sp
  )
{
}

/** OS specific DMA memory alloc for shared code

   @param[in]   Hw         pointer to the HW structure
   @param[out]  Mem        ptr to mem struct to fill out
   @param[in]   Size       size of memory requested
   @param[in]   Alignment  byte boundary to which we must align

   @retval   ICE_SUCCESS        Memory allocated successfully
   @retval   I40E_ERR_NO_MEMORY  Failed to allocate memory
**/
enum ice_status
IceAllocateDmaMem (
  struct ice_hw      *Hw,
  struct ice_dma_mem *Mem,
  UINT64               Size,
  UINT32               Alignment
  )
{
  EFI_STATUS  Status;
  DRIVER_DATA *AdapterInfo = (DRIVER_DATA *) Hw->back;

  if (Mem == NULL) {
    return ICE_ERR_PARAM;
  }

  Mem->Mapping.Size = (UINT32) ALIGN (Size, Alignment);

  Status = UndiDmaAllocateCommonBuffer (AdapterInfo->PciIo, &Mem->Mapping);

  Mem->va     = (VOID*) Mem->Mapping.UnmappedAddress;
  Mem->pa     = Mem->Mapping.PhysicalAddress;
  Mem->size   = Mem->Mapping.Size;

  if ((Mem->va != NULL)
    && (Status == EFI_SUCCESS))
  {
    return ICE_SUCCESS;
  } else {
    DEBUGPRINT (
      CRITICAL, ("Error: Requested: %d, Allocated size: %d\n",
      Size, Mem->size)
    );
    return ICE_ERR_NO_MEMORY;
  }
}

void *
IceAllocateDmaMemWrap (
  struct ice_hw      *Hw,
  struct ice_dma_mem *Mem,
  UINT64               Size
  )
{
  IceAllocateDmaMem (Hw,Mem,Size,4);
  ZeroMem (Mem->va, (UINT32) Size);
  return Mem->va;
}

/** OS specific DMA memory free for shared code

   @param[in]   Hw         pointer to the HW structure
   @param[out]  Mem        ptr to mem struct to free

   @retval  ICE_SUCCESS     Memory successfully freed
   @retval  I40E_ERR_BAD_PTR Failed to free buffer
   @retval  I40E_ERR_PARAM   Mem is NULL
**/
enum ice_status
IceFreeDmaMem (
  struct ice_hw *     Hw,
  struct ice_dma_mem *Mem
  )
{
  EFI_STATUS  Status;
  DRIVER_DATA *AdapterInfo = (DRIVER_DATA *) Hw->back;

  if (NULL == Mem) {
    return ICE_ERR_PARAM;
  }

  // Free memory allocated for transmit and receive resources.
  Status = UndiDmaFreeCommonBuffer (AdapterInfo->PciIo, &Mem->Mapping);

  if (EFI_ERROR (Status)) {
    DEBUGPRINT (CRITICAL, ("Failed to free ICE DMA buffer: %r\n", Status));
    return ICE_ERR_BAD_PTR;
  }
  return ICE_SUCCESS;
}

void
IceFreeDmaMemWrap (
  struct ice_hw *     Hw,
  struct ice_dma_mem *Mem
  )
{
  IceFreeDmaMem (Hw, Mem);
  Mem->va = NULL;
  Mem->pa = 0;
  Mem->size = 0;
}

/** Gets information on current link up/down status.

   @param[in]   UndiPrivateData  Pointer to driver private data structure
   @param[out]  LinkUp           Link up/down status

   @retval  EFI_SUCCESS   Links status retrieved successfully
   @retval  !EFI_SUCCESS  Underlying function failure
**/
EFI_STATUS
GetLinkStatus (
  IN   UNDI_PRIVATE_DATA  *UndiPrivateData,
  OUT  BOOLEAN            *LinkUp
  )
{
  return IsLinkUp (&UndiPrivateData->NicInfo, LinkUp);
}

/** Gets link speed setting for adapter.

   @param[in]   UndiPrivateData   Pointer to driver private data structure
   @param[out]  LinkSpeed         Link speed setting

   @retval      EFI_SUCCESS       Successfull operation
**/
EFI_STATUS
GetLinkSpeed (
  IN   UNDI_PRIVATE_DATA  *UndiPrivateData,
  OUT  UINT8              *LinkSpeed
  )
{
  // Speed settings are currently not supported for 100 Gig driver. It's always set to autoneg to
  // allow operation with the highest possible speed
  *LinkSpeed = LINK_SPEED_AUTO_NEG;
  return EFI_SUCCESS;
}

/** Sets link speed setting for adapter (unsupported).

   @param[in]   UndiPrivateData  Pointer to driver private data structure
   @param[in]   LinkSpeed        Lan speed setting - unused

   @retval      EFI_SUCCESS      Successfull operation
**/
EFI_STATUS
SetLinkSpeed (
  IN  UNDI_PRIVATE_DATA  *UndiPrivateData,
  IN  UINT8              *LinkSpeed
  )
{
  return EFI_SUCCESS;
}

/** Returns information whether Link Speed attribute is supported.

   @param[in]   UndiPrivateData     Pointer to driver private data structure
   @param[out]  LinkSpeedSupported  BOOLEAN value describing support

   @retval      EFI_SUCCESS         Successfull operation
**/
EFI_STATUS
IsLinkSpeedSupported (
  IN  UNDI_PRIVATE_DATA  *UndiPrivateData,
  OUT BOOLEAN            *LinkSpeedSupported
  )
{
  *LinkSpeedSupported = TRUE;
  return EFI_SUCCESS;
}

/** Returns information whether Link Speed attribute is modifiable.

   @param[in]   UndiPrivateData      Pointer to driver private data structure
   @param[out]  LinkSpeedModifiable  BOOLEAN value describing support

   @retval      EFI_SUCCESS          Successfull operation
**/
EFI_STATUS
IsLinkSpeedModifiable (
  IN  UNDI_PRIVATE_DATA  *UndiPrivateData,
  OUT BOOLEAN            *LinkSpeedModifiable
  )
{
  *LinkSpeedModifiable = FALSE;
  return EFI_SUCCESS;
}



/** Blinks LEDs on port managed by current PF.

   @param[in]   UndiPrivateData  Pointer to driver private data structure
   @param[in]   Time             Time in seconds to blink

   @retval  EFI_SUCCESS       LEDs blinked successfully
   @retval  EFI_DEVICE_ERROR  Failed to set LED state
**/
EFI_STATUS
BlinkLeds (
  IN  UNDI_PRIVATE_DATA  *UndiPrivateData,
  IN  UINT16             *Time
  )
{
  enum ice_status                  IceStatus;

  IceStatus = ice_aq_set_port_id_led (UndiPrivateData->NicInfo.Hw.port_info, FALSE, NULL);
  IF_RETURN (IceStatus != ICE_SUCCESS, EFI_DEVICE_ERROR);

  DelayInMicroseconds (&UndiPrivateData->NicInfo, *Time * 1000 * 1000);

  IceStatus = ice_aq_set_port_id_led (UndiPrivateData->NicInfo.Hw.port_info, TRUE, NULL);
  IF_RETURN (IceStatus != ICE_SUCCESS, EFI_DEVICE_ERROR);

  return EFI_SUCCESS;
}


/** Read VSI parameters

  @param[in]    AdapterInfo         Pointer to the NIC data structure information
                                    which the UNDI driver is layerin

  @param[out]   VsiCtx             resulting VSI context

  @retval       EFI_SUCCESS         VSI context successfully read
  @retval       EFI_DEVICE_ERROR    VSI context read error
**/
EFI_STATUS
IceGetVsiParams (
  IN  DRIVER_DATA        *AdapterInfo,
  OUT struct ice_vsi_ctx *VsiCtx
  )
{
  struct ice_hw  *Hw;
  EFI_STATUS IceStatus;

  Hw = &AdapterInfo->Hw;

  ZeroMem (VsiCtx, sizeof (struct ice_vsi_ctx));

  VsiCtx->vsi_num = Hw->fw_vsi_num;

  IceStatus = ice_aq_get_vsi_params (&AdapterInfo->Hw, VsiCtx, NULL);
  if (IceStatus != ICE_SUCCESS) {
    DEBUGPRINT (
      CRITICAL, ("ice_aq_get_vsi_params returned %r\n",
      IceStatus)
    );
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}

/** Gets Max Speed of ethernet port in bits.

   @param[in]   UndiPrivateData   Points to the driver instance private data
   @param[out]  MaxSpeed          Resultant value

   @retval      EFI_SUCCESS       Operation successful.
   @retval      EFI_DEVICE_ERROR  Matching PMD for current PF not found.
   @retval      EFI_UNSUPPORTED   Speed for current adapter not recognized.
   @retval      Others            Failure of underlying function.
**/
EFI_STATUS
GetMaxSpeed (
  IN  UNDI_PRIVATE_DATA  *UndiPrivateData,
  OUT UINT64             *MaxSpeed
  )
{
  EFI_STATUS           Status;
  PORT_OPTIONS_DATA    *PortOptionsData;
  PORT_OPTION          *ActivePortOption;
  UINT8                PmdNum;

  // After first call result should be stored in UndiPrivateData to avoid calling
  // Get Port Option AQ ICE_MAX_PMD times for every language in HII setup code.
  if (UndiPrivateData->HiiInfo.MaxPortSpeed == PortOptionSpeedNA) {

    Status = GetPortOptionsForAllPorts (&UndiPrivateData->NicInfo, &PortOptionsData);
    IF_RETURN (EFI_ERROR (Status), Status);

    ActivePortOption = &PortOptionsData->PortOptions[PortOptionsData->Active];

    for (PmdNum = 0; PmdNum < ICE_MAX_PMD; PmdNum++) {
      if (ActivePortOption->PfNum[PmdNum] == UndiPrivateData->NicInfo.Function) {
        UndiPrivateData->HiiInfo.MaxPortSpeed = ActivePortOption->PortSpeed[PmdNum];
        DEBUGPRINT (
          HII, ("found matching PF: %d, on PMD: %d, speed: %d\n",
          UndiPrivateData->NicInfo.Function,
          PmdNum,
          UndiPrivateData->HiiInfo.MaxPortSpeed)
          );
        break;
      }
    }

    FreePool (PortOptionsData);
    IF_RETURN (PmdNum == ICE_MAX_PMD, EFI_DEVICE_ERROR);
  }

  switch (UndiPrivateData->HiiInfo.MaxPortSpeed) {
    case ICE_AQC_PORT_OPT_MAX_LANE_100G:
      *MaxSpeed = GIGABITS (100);
      break;
    case ICE_AQC_PORT_OPT_MAX_LANE_50G:
      *MaxSpeed = GIGABITS (50);
      break;
    case ICE_AQC_PORT_OPT_MAX_LANE_25G:
      *MaxSpeed = GIGABITS (25);
      break;
    case ICE_AQC_PORT_OPT_MAX_LANE_10G:
      *MaxSpeed = GIGABITS (10);
      break;
    case ICE_AQC_PORT_OPT_MAX_LANE_5G:
      *MaxSpeed = GIGABITS (5);
      break;
    case ICE_AQC_PORT_OPT_MAX_LANE_2500M:
      *MaxSpeed = GIGABITS (2.5);
      break;
    case ICE_AQC_PORT_OPT_MAX_LANE_1G:
      *MaxSpeed = GIGABITS (1);
      break;
    case ICE_AQC_PORT_OPT_MAX_LANE_100M:
      *MaxSpeed = GIGABITS (0.1);
      break;
    default:
      return EFI_UNSUPPORTED;
  }

  return EFI_SUCCESS;
}

/** Gets HII formset help string ID.

   @param[in]   UndiPrivateData  Pointer to driver private data structure

   @return   EFI_STRING_ID of formset help string or 0 if unknown speed
**/
EFI_STRING_ID
GetFormSetHelpStringId (
  IN UNDI_PRIVATE_DATA  *UndiPrivateData
  )
{
  EFI_STATUS  Status;
  UINT64      EthPortSpeedBits = 0;

  Status = GetMaxSpeed (UndiPrivateData, &EthPortSpeedBits);
  IF_RETURN (EFI_ERROR (Status), 0);

  switch (EthPortSpeedBits) {
    case GIGABITS (100):
      return STRING_TOKEN (STR_INV_FORM_SET_100_HELP);
    case GIGABITS (50):
      return STRING_TOKEN (STR_INV_FORM_SET_50_HELP);
    case GIGABITS (25):
      return STRING_TOKEN (STR_INV_FORM_SET_25_HELP);
    case GIGABITS (10):
      return STRING_TOKEN (STR_INV_FORM_SET_10_HELP);
    case GIGABITS (5):
      return STRING_TOKEN (STR_INV_FORM_SET_5_HELP);
    case GIGABITS (2.5):
      return STRING_TOKEN (STR_INV_FORM_SET_2_5_HELP);
    case GIGABITS (1):
      return STRING_TOKEN (STR_INV_FORM_SET_1_HELP);
    default:
      return 0;
  }
}


/** Get supported Tx/Rx descriptor count for a given device

   @param[in]    Hw         Pointer to the HW Structure

   @return       Supported Tx/RX descriptors count
**/
UINT16
IceGetTxRxDescriptorsCount (
  IN struct ice_hw *Hw
  )
{
  // Return default value for now. Need to optimize Tx/Rx descriptor count
  // based on number of PFs.

  return ICE_NUM_TX_RX_DESCRIPTORS;
}

/** Duplicates memory block

   @param[in]  Hw    pointer to the HW structure
   @param[in]  Mem   ptr to mem struct to fill out
   @param[in]  Size  size of memory duplicated

   @return   Memory duplicated
**/
VOID
*IceMemDup(
  IN struct ice_hw *Hw,
  IN const void *MemPtr,
  IN size_t Size
  )
{
    VOID *Ptr;

    Ptr = IceAllocateMem(Hw, Size);
    if (Ptr) {
      CopyMem(Ptr, MemPtr, Size);
    }

    return Ptr;
}

/** Gets ETRACKID

   @param[in]   UndiPrivateData   Pointer to the driver data
   @param[out]  EtrackId          Unique NVM identifier

   @retval      EFI_SUCCESS            EtrackId returned correctly
   @retval      EFI_DEVICE_ERROR       HW is not initialized
   @retval      EFI_INVALID_PARAMETER  Invalid input parameter
**/
EFI_STATUS
GetEtrackId (
  IN  UNDI_PRIVATE_DATA  *UndiPrivateData,
  OUT UINT32             *EtrackId
  )
{
  if (UndiPrivateData == NULL
    || EtrackId == NULL)
  {
    return EFI_INVALID_PARAMETER;
  }

  *EtrackId = UndiPrivateData->NicInfo.Hw.flash.nvm.eetrack;

  if (*EtrackId == 0) {
    return EFI_UNSUPPORTED;
  }

  return EFI_SUCCESS;
}

/** Gets CVL chip type string.

   @param[in]   UndiPrivateData   Points to the driver instance private data
   @param[out]  ChipTypeStr       Points to the output string buffer

   @retval   EFI_SUCCESS   Chip type string successfully retrieved
**/
EFI_STATUS
GetChipTypeStr (
  IN  UNDI_PRIVATE_DATA  *UndiPrivateData,
  OUT EFI_STRING         ChipTypeStr
  )
{
  switch (UndiPrivateData->NicInfo.Hw.device_id) {
#ifdef SIMICS_SUPPORT
    case ICE_DEV_ID_SIMICS_NIC_MODE:
      UnicodeSPrint (ChipTypeStr, HII_MAX_STR_LEN_BYTES, L"SIMICS");
      break;
#endif /* SIMICS_SUPPORT */
#ifdef E810C_SUPPORT
    case ICE_DEV_ID_E810C_BACKPLANE:
    case ICE_DEV_ID_E810C_QSFP:
    case ICE_DEV_ID_E810C_SFP:
      UnicodeSPrint (ChipTypeStr, HII_MAX_STR_LEN_BYTES, L"Intel E810-C");
      break;
#endif /* E810C_SUPPORT */
#ifdef E810_XXV_SUPPORT
    case ICE_DEV_ID_E810_XXV_BACKPLANE:
    case ICE_DEV_ID_E810_XXV_QSFP:
    case ICE_DEV_ID_E810_XXV_SFP:
      UnicodeSPrint (ChipTypeStr, HII_MAX_STR_LEN_BYTES, L"Intel E810-XXV");
      break;
#endif /* E810_XXV_SUPPORT */
#ifdef E822_SUPPORT
    case ICE_DEV_ID_E822C_10G_BASE_T:
    case ICE_DEV_ID_E822C_BACKPLANE:
    case ICE_DEV_ID_E822C_QSFP:
    case ICE_DEV_ID_E822C_SFP:
    case ICE_DEV_ID_E822C_SGMII:
      UnicodeSPrint (ChipTypeStr, HII_MAX_STR_LEN_BYTES, L"Intel E822-C");
      break;
    case ICE_DEV_ID_E822L_10G_BASE_T:
    case ICE_DEV_ID_E822L_BACKPLANE:
    case ICE_DEV_ID_E822L_SFP:
    case ICE_DEV_ID_E822L_SGMII:
      UnicodeSPrint (ChipTypeStr, HII_MAX_STR_LEN_BYTES, L"Intel E822-L");
      break;
#endif /* E822_SUPPORT */
#ifdef E823_SUPPORT
    case ICE_DEV_ID_E823L_10G_BASE_T:
    case ICE_DEV_ID_E823L_1GBE:
    case ICE_DEV_ID_E823L_BACKPLANE:
    case ICE_DEV_ID_E823L_QSFP:
    case ICE_DEV_ID_E823L_SFP:
      UnicodeSPrint (ChipTypeStr, HII_MAX_STR_LEN_BYTES, L"Intel E823-L");
      break;
#endif /* E823_SUPPORT */
#ifdef E823C_SUPPORT
    case ICE_DEV_ID_E823C_10G_BASE_T:
    case ICE_DEV_ID_E823C_BACKPLANE:
    case ICE_DEV_ID_E823C_QSFP:
    case ICE_DEV_ID_E823C_SFP:
    case ICE_DEV_ID_E823C_SGMII:
      UnicodeSPrint (ChipTypeStr, HII_MAX_STR_LEN_BYTES, L"Intel E823-C");
      break;
#endif /* E823C_SUPPORT */
#ifdef E825C_SUPPORT
    case ICE_DEV_ID_E825C_BACKPLANE:
    case ICE_DEV_ID_E825C_QSFP:
    case ICE_DEV_ID_E825C_SFP:
      UnicodeSPrint (ChipTypeStr, HII_MAX_STR_LEN_BYTES, L"Intel E825-C");
      break;
#endif /* E825C_SUPPORT */
    default:
      UnicodeSPrint (ChipTypeStr, HII_MAX_STR_LEN_BYTES, L"unknown");
      break;
  }

  return EFI_SUCCESS;
}

/** Checks if alternate MAC address is supported.

   @param[in]   UndiPrivateData    Driver instance private data structure
   @param[out]  AltMacSupport      Tells if alternate mac address is supported

   @retval   EFI_SUCCESS    Alternate MAC address support retrieved successfully
**/
EFI_STATUS
GetAltMacAddressSupport (
  IN   UNDI_PRIVATE_DATA  *UndiPrivateData,
  OUT  BOOLEAN            *AltMacSupport
  )
{
  *AltMacSupport = TRUE;
  return EFI_SUCCESS;
}

/** Checks if FMP update needs to be done EEPROM only

   @param[in]   UndiPrivateData   Points to the driver instance private data.

   @retval   TRUE    FMP update needs to be done EEPROM only (Flashless device)
   @retval   FALSE    FMP update does not need to be done EEPROM only
**/
BOOLEAN
IsFlashlessSku (
  IN UNDI_PRIVATE_DATA *UndiPrivateData
  )
{
  return FALSE;
}

/** Stop all drivers managing the current adapter except the calling instance of driver

   @param[in]   UndiPrivateData   Points to the driver instance private data.
   @param[in]   StartDrivers      Flag to choose between start/stop PF

   @retval    EFI_SUCCESS     PFs stopped successfully
   @retval    EFI_SUCCESS     No driver instances found to be stoped
   @retval    EFI_OUT_OF_RESOURCES   Failed to find DriverStop protocol instance
   @retval    EFI_NOT_FOUND   Failed to find NII pointer protocol instance
   @retval    EFI_OUT_OF_RESOURCES   Failed to find NII pointer protocol instance
   @retval    EFI_UNSUPPORTED   Testing child handle failed
   @retval    EFI_ACCESS_DENIED  Failed to open PCI IO Protocol
   @retval    EFI_ALREADY_STARTED  Failed to open PCI IO Protocol
   @retval    EFI_UNSUPPORTED  Failed to open PCI IO Protocol
   @retval    EFI_ACCESS_DENIED  Failed to open DriverStop Protocol
   @retval    EFI_ALREADY_STARTED  Failed to open DriverStop Protocol
   @retval    EFI_UNSUPPORTED  Failed to open DriverStop Protocol
**/
EFI_STATUS
StartStopRemainingPFsOnAdapter (
  UNDI_PRIVATE_DATA *UndiPrivateData,
  BOOLEAN            StartDrivers
  )
{
  EFI_STATUS  Status;
  UINTN       StartStopHandleCount;
  EFI_HANDLE *StartStopHandleBuffer;
  UINTN       NiiPointerHandleCount;
  EFI_HANDLE *NiiPointerHandleBuffer;
  EFI_HANDLE  ControllerHandle;
  UINTN       i;
  UINTN       j;

  Status = EFI_DEVICE_ERROR;
  StartStopHandleBuffer = NULL;
  NiiPointerHandleBuffer = NULL;
  ControllerHandle = (EFI_HANDLE) 0;

  // Find all instances of DriverStop protocol
  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiStartStopProtocolGuid,
                  NULL,
                  &StartStopHandleCount,
                  &StartStopHandleBuffer
                );
  if (EFI_ERROR (Status)) {
    DEBUGPRINT (CRITICAL, ("LocateHandleBuffer returned %r\n", Status));
    if (Status == EFI_NOT_FOUND) {

      // Not an error. No driver instances found that need to be stopped
      Status = EFI_SUCCESS;
    }
    goto ExitFreePools;
  }

  // Find all instances of NiiPointer protocol
  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiNiiPointerGuid,
                  NULL,
                  &NiiPointerHandleCount,
                  &NiiPointerHandleBuffer
                );
  if (EFI_ERROR (Status)) {
    DEBUGPRINT (CRITICAL, ("LocateHandleBuffer returned %r\n", Status));
    goto ExitFreePools;
  }

  for (i = 0; i < StartStopHandleCount; i++) {

    // Find parrent Controller Handle
    for (j = 0; j < NiiPointerHandleCount; j++) {
      Status = EfiTestChildHandle (
                 NiiPointerHandleBuffer[j],
                 StartStopHandleBuffer[i],
                 &gEfiPciIoProtocolGuid
               );
      if (Status == EFI_SUCCESS) {
        ControllerHandle = NiiPointerHandleBuffer[j];
        break;
      }
    }
    if ((Status == EFI_SUCCESS)
      && (ControllerHandle != UndiPrivateData->ControllerHandle))
    {
      UINTN                     Seg, Bus, Device, Function;
      EFI_PCI_IO_PROTOCOL      *PciIo;
      EFI_DRIVER_STOP_PROTOCOL *StartStopProtocol;

      // Check if ControllerHandle belongs to the same adapter
      Status = gBS->OpenProtocol (
                      ControllerHandle,
                      &gEfiPciIoProtocolGuid,
                      (VOID * *) &PciIo,
                      gUndiDriverBinding.DriverBindingHandle,
                      UndiPrivateData->ControllerHandle,
                      EFI_OPEN_PROTOCOL_GET_PROTOCOL
                    );
      if (EFI_ERROR (Status)) {
        DEBUGPRINT (CRITICAL, ("LocateHandleBuffer returned %r\n", Status));
        continue;
      }

      // Check if the bus number matches
      PciIo->GetLocation (PciIo, &Seg, &Bus, &Device, &Function);
      if ((Seg == UndiPrivateData->NicInfo.Segment)
        && (Bus == UndiPrivateData->NicInfo.Bus))
      {

        // Call Driver Stop
        Status = gBS->OpenProtocol (
                        StartStopHandleBuffer[i],
                        &gEfiStartStopProtocolGuid,
                        (VOID * *) &StartStopProtocol,
                        gUndiDriverBinding.DriverBindingHandle,
                        UndiPrivateData->ControllerHandle,
                        EFI_OPEN_PROTOCOL_GET_PROTOCOL
                      );
        if (EFI_ERROR (Status)) {
          DEBUGPRINT (CRITICAL, ("OpenProtocol returned %r\n", Status));
          goto ExitFreePools;
        }
        if (StartDrivers) {
          DEBUGPRINT (IMAGE, ("Calling driver START on BDF: %d, %d, %d\n", Bus, Device, Function));
          StartStopProtocol->StartDriver (StartStopProtocol);
        } else {
          DEBUGPRINT (IMAGE, ("Calling driver STOP on BDF: %d, %d, %d\n", Bus, Device, Function));
          StartStopProtocol->StopDriver (StartStopProtocol, TRUE);
        }
      }
    }
  }

ExitFreePools:

  // Free the buffer containing the list of handles from the handle database
  if (NiiPointerHandleBuffer != NULL) {
    FreePool (NiiPointerHandleBuffer);
  }
  if (StartStopHandleBuffer != NULL) {
    FreePool (StartStopHandleBuffer);
  }

  return Status;
}

/** Counts the number of '1's in the 8-bit number

   @param[in]   Number    Number of which we count the '1's.

   @return                Number of '1's in the number.
**/
UINT8
IceHweight8 (
  UINT8 Number
  )
{
  UINT8 Bit;
  UINT8 Weight = 0;

  for (Bit = 0; Bit < 8; ++Bit) {
    if ((1 << Bit) & Number) {
      ++Weight;
    }
  }

  return Weight;
}

/** Handles events found on ARQ during scanning the queue.

  Called for events found during clearing the queue or awaiting for specific event,
  to process events that require it before their disposal.
  Currently only LSE is checked.

  @param[in]  AdapterInfo            Pointer to the driver data
  @param[in]  EventInfo              ARQ event info
**/
VOID
HandleArqEvent (
  IN DRIVER_DATA              *AdapterInfo,
  IN struct ice_rq_event_info *EventInfo
  )
{
  if (EventInfo == NULL) {
    return;
  }

  if (EventInfo->desc.opcode == ice_aqc_opc_get_link_status) {
    DEBUGPRINT (
      DECODE, ("LSE detected. PF [ %d ] lport: %d\n",
      AdapterInfo->Hw.pf_id,
      AdapterInfo->Hw.port_info->lport)
    );
    AdapterInfo->MediaStatusChecked = FALSE;
  }

  return;
}

/** Clear the admin receive queue.

   @param[in]   AdapterInfo   Pointer to the driver data

   @retval     EFI_DEVICE_ERROR   Waiting for ARQ clear timeout
   @retval     EFI_SUCCESS        Receive queue cleared successfully
**/
EFI_STATUS
ClearAdminReceiveQueue (
  IN DRIVER_DATA *AdapterInfo
  )
{
  enum ice_status            IceStatus = ICE_SUCCESS;
  struct ice_hw             *Hw;
  struct ice_rq_event_info   EventInfo;
  UINT16                     Pending;
  UINT16                     RqCnt;

  Hw = &AdapterInfo->Hw;
  IceStatus = ICE_SUCCESS;
  ZeroMem (&EventInfo, sizeof (EventInfo));

  // Use RqCnt as a safety loop delimiter
  RqCnt = Hw->adminq.num_rq_entries;
  if (RqCnt == 0) {
    DEBUGPRINT (CRITICAL, ("Admin Receive Queue uninitialized.\n"));
    return EFI_DEVICE_ERROR;
  }

  do {
    IceStatus = ice_clean_rq_elem (
                  Hw,
                  &Hw->adminq,
                  &EventInfo,
                  &Pending
                );
    if (IceStatus == ICE_SUCCESS) {
      HandleArqEvent (AdapterInfo, &EventInfo);
    }

    RqCnt--;

  } while ((Pending > 0) && (RqCnt > 0));

  return EFI_SUCCESS;
}

/** Wait for the specific admin queue event to occure.

   @param[in]  AdapterInfo   Pointer to the driver data
   @param[in]  Opcode        Opcode of the event
   @param[in]  Timeout       Timeout in 1ms units

   @retval     EFI_DEVICE_ERROR   Waiting for ARQ NVM erase response timeout
   @retval     EFI_SUCCESS        Successfull wait
**/
EFI_STATUS
AwaitReceiveQueueEvent (
  IN DRIVER_DATA         *AdapterInfo,
  IN enum ice_adminq_opc  Opcode,
  IN UINT32               Timeout
  )
{
  enum ice_status            IceStatus = ICE_SUCCESS;
  struct ice_hw             *Hw;
  struct ice_rq_event_info   EventInfo;
  UINT16                     Pending;
  UINT32                     WaitCnt;

  Hw = &AdapterInfo->Hw;
  IceStatus = ICE_SUCCESS;
  ZeroMem (&EventInfo, sizeof (EventInfo));

  // Free AQ receive queue
  WaitCnt = 0;
  do {
    IceStatus = ice_clean_rq_elem (
                  Hw,
                  &Hw->adminq,
                  &EventInfo,
                  &Pending
                );
    if (IceStatus == ICE_SUCCESS) {
      if (EventInfo.desc.opcode != Opcode) {
        HandleArqEvent (AdapterInfo, &EventInfo);
        IceStatus = ICE_ERR_AQ_NO_WORK;
      } else {
        break;
      }
    }

    gBS->Stall (1000);
    WaitCnt++;
    if (WaitCnt > Timeout) {
      DEBUGPRINT (CRITICAL, ("Timeout waiting for ARQ response\n"));
      return EFI_DEVICE_ERROR;
    }
  } while (IceStatus != ICE_SUCCESS);
  return EFI_SUCCESS;
}

/** Checks if Firmware is in recovery mode.

   @param[in]   AdapterInfo  Pointer to the NIC data structure information which
                             the UNDI driver is layering on

   @retval   TRUE   Firmware is in recovery mode
   @retval   FALSE  Firmware is not in recovery mode
**/
BOOLEAN
IsRecoveryMode (
  IN DRIVER_DATA *AdapterInfo
  )
{
  enum ice_fw_modes FwMode;
  FwMode = ice_get_fw_mode (&AdapterInfo->Hw);
  if (FwMode == ICE_FW_MODE_REC) {
    return TRUE;
  }
  return FALSE;
}

/** Checks if Firmware is able to change Tx scheduler topology.

   @param[in]   UndiPrivateData  Pointer to the driver private data.
   @param[out]  Supported        Tells whether the feature is supported.

   @retval   EFI_SUCCESS       Successfully retrieved the information about support.
   @retval   EFI_DEVICE_ERROR  Failed to retrieve the information about support.
**/
EFI_STATUS
IsTxSchedulerTopologyChangeSupported (
  IN   UNDI_PRIVATE_DATA  *UndiPrivateData,
  OUT  BOOLEAN            *Supported
  )
{
  EFI_STATUS  Status;
  BOOLEAN     CvlDevice;

  Status = IsCvlDevice (UndiPrivateData, &CvlDevice);
  IF_RETURN (EFI_ERROR (Status), EFI_DEVICE_ERROR);

  *Supported = CvlDevice &&
               UndiPrivateData->NicInfo.Hw.dev_caps.common_cap.tx_sched_topo_comp_mode_en;

  return EFI_SUCCESS;
}

/** Checks if Firmware is in pending reboot state.

   @param[in]   AdapterInfo  Pointer to the NIC data structure information which
                             the UNDI driver is layering on

   @retval   TRUE   Firmware is in pending reboot state
   @retval   FALSE  Firmware is not in pending reboot state
**/
BOOLEAN
IsPendingRebootState (
  IN DRIVER_DATA *AdapterInfo
  )
{
  if ((AdapterInfo->Hw.dev_caps.common_cap.nvm_update_pending_netlist == TRUE)
     || (AdapterInfo->Hw.dev_caps.common_cap.nvm_update_pending_orom == TRUE)
     || (AdapterInfo->Hw.dev_caps.common_cap.nvm_update_pending_nvm == TRUE))
  {
    return TRUE;
  }
  return FALSE;
}

/** Checks if Firmware is in lockdown state.

   @param[in]   AdapterInfo  Pointer to the NIC data structure information which
                             the UNDI driver is layering on

   @retval   TRUE   Firmware is in lockdown state
   @retval   FALSE  Firmware is not in lockdown state
**/
BOOLEAN
IsFwLockdownState (
  IN DRIVER_DATA *AdapterInfo
  )
{
  if ((AdapterInfo->Hw.dev_caps.common_cap.fw_lockdown_support == TRUE)
     && (AdapterInfo->Hw.dev_caps.common_cap.fw_lockdown_status == TRUE))
  {
    return TRUE;
  }
  return FALSE;
}

/** Read TLV module located in the PFA.

   @param[in]  UndiPrivateData    Pointer to the driver data
   @param[in]  ModuleTypeId       The pointer to the module
   @param[in]  Offset             Word offset within module
   @param[in]  Length             The length of data to be read (in bytes)
   @param[in]  Data               Pointer to data buffer

   @retval     EFI_SUCCESS            TLV module read successfully
   @retval     EFI_INVALID_PARAMETER  ModuleData is NULL
   @retval     EFI_DEVICE_ERROR       Failed to read TLV module
   @retval     EFI_DEVICE_ERROR       Failed to acquire NVM resource
   @retval     EFI_NOT_FOUND          EPERM status returned due to invalid TLV (TLV does not exist)
**/
EFI_STATUS
ReadTlv (
  IN  UNDI_PRIVATE_DATA *UndiPrivateData,
  IN  UINT16             ModuleTypeId,
  IN  UINT16             Offset,
  IN  UINT32             Length,
  IN  VOID              *Data
  )
{
  EFI_STATUS        Status    = EFI_SUCCESS;
  enum ice_status   IceStatus = ICE_SUCCESS;
  struct ice_hw    *Hw        = &UndiPrivateData->NicInfo.Hw;

  if (Data == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  IceBlockIt (&UndiPrivateData->NicInfo, TRUE);

  IceStatus = ice_acquire_nvm (Hw, ICE_RES_READ);
  if (IceStatus != ICE_SUCCESS) {
    DEBUGPRINT (CRITICAL, ("ice_acquire_nvm failed with status %d\n", IceStatus));
    Status = EFI_DEVICE_ERROR;
    goto CleanUp;
  }

  IceStatus = ice_aq_read_nvm (Hw, ModuleTypeId, 2 * Offset, Length, Data, FALSE, TRUE, NULL);

  if (IceStatus != ICE_SUCCESS) {
    if (UndiPrivateData->NicInfo.Hw.adminq.sq_last_status == ICE_AQ_RC_EPERM) {
      Status = EFI_NOT_FOUND; // EPERM status returned due to invalid TLV (TLV does not exist)
    } else {
      Status = EFI_DEVICE_ERROR;
    }
    DEBUGPRINT (CRITICAL, ("ice_aq_read_nvm failed with status %d\n", IceStatus));
  }
  ice_release_nvm (Hw);

CleanUp:
  IceBlockIt (&UndiPrivateData->NicInfo, FALSE);
  return Status;
}

/** Write to TLV module located in the PFA.

   @param[in]  UndiPrivateData    Pointer to the driver data
   @param[in]  ModuleTypeId       The pointer to the module
   @param[in]  Offset             Word offset within module
   @param[in]  Length             The length of data to be written (in bytes)
   @param[in]  Data               Pointer to data buffer

   @retval     EFI_SUCCESS            TLV module written successfully
   @retval     EFI_INVALID_PARAMETER  Invalid ModulePointer
   @retval     EFI_INVALID_PARAMETER  ModuleData is NULL
   @retval     EFI_DEVICE_ERROR       Failed to write TLV module
   @retval     EFI_DEVICE_ERROR       Failed to acquire NVM resource
**/
EFI_STATUS
WriteTlv (
  IN  UNDI_PRIVATE_DATA *UndiPrivateData,
  IN  UINT16             ModuleTypeId,
  IN  UINT16             Offset,
  IN  UINT32             Length,
  IN  VOID              *Data
  )
{
  EFI_STATUS        Status    = EFI_SUCCESS;
  enum ice_status   IceStatus = ICE_SUCCESS;
  struct ice_hw    *Hw        = &UndiPrivateData->NicInfo.Hw;

  if (Data == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  IceBlockIt (&UndiPrivateData->NicInfo, TRUE);

  Status = ClearAdminReceiveQueue (&UndiPrivateData->NicInfo);
  if (EFI_ERROR (Status)) {
    DEBUGPRINT (CRITICAL, ("ClearAdminReceiveQueue failed with status %d\n", Status));
    goto CleanUp;
  }

  IceStatus = ice_acquire_nvm (Hw, ICE_RES_WRITE);
  if (IceStatus != ICE_SUCCESS) {
    DEBUGPRINT (CRITICAL, ("ice_acquire_nvm failed with status %d\n", IceStatus));
    Status = EFI_DEVICE_ERROR;
    goto CleanUp;
  }

  IceStatus = ice_aq_update_nvm (Hw, ModuleTypeId, 2 * Offset, Length, Data, FALSE, 0, NULL);
  ice_release_nvm (Hw);
  if (IceStatus != ICE_SUCCESS) {
    DEBUGPRINT (CRITICAL, ("ice_aq_read_nvm failed with status %d\n", IceStatus));
    Status = EFI_DEVICE_ERROR;
    goto CleanUp;
  }

  Status = AwaitReceiveQueueEvent (
             &UndiPrivateData->NicInfo,
             ice_aqc_opc_nvm_write,
             NVM_OPERATION_TIMEOUT_IN_1MS_UNITS
           );

  if (EFI_ERROR (Status)) {
    DEBUGPRINT (CRITICAL, ("ClearAdminReceiveQueue failed with status %d\n", Status));
  }

CleanUp:
  IceBlockIt (&UndiPrivateData->NicInfo, FALSE);
  return Status;
}

/** Read VPD TLV module to preallocated buffer.

   @param[in]  UndiPrivateData        Pointer to the driver data
   @param[out] VpdBuffer              Pointer to the output buffer
   @param[out] VpdSizeInWords         Length of read VPD TLV

   @retval     EFI_SUCCESS            VPD written to buffer successfully
   @retval     EFI_INVALID_PARAMETER  UndiPrivateData is NULL or VpdBuffer is null
   @retval     EFI_DEVICE_ERROR       Failed to read TLV module
   @retval     EFI_DEVICE_ERROR       Failed to acquire NVM resource
   @retval     EFI_DEVICE_ERROR       Failed to find VPD TLV
**/
EFI_STATUS
IceReadVpdBuffer (
  IN  UNDI_PRIVATE_DATA *UndiPrivateData,
  OUT UINT16            *VpdBuffer,
  OUT UINT16            *VpdSizeInWords
  )
{
  enum ice_status IceStatus;
  UINT16 VpdTlvOffset;

  if ((UndiPrivateData == NULL) ||
    (VpdBuffer == NULL) ||
    (VpdSizeInWords == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  IceStatus = ice_get_pfa_module_tlv (&UndiPrivateData->NicInfo.Hw, &VpdTlvOffset, VpdSizeInWords, ICE_SR_VPD_PTR);
  if (IceStatus != ICE_SUCCESS) {
    return EFI_DEVICE_ERROR;
  }

  //We have to skip 2 words, containing TLV type and length
  VpdTlvOffset += 2;

  IceStatus = ice_acquire_nvm (&UndiPrivateData->NicInfo.Hw, ICE_RES_READ);
  if (IceStatus != ICE_SUCCESS) {
    return EFI_DEVICE_ERROR;
  }

  IceStatus = ice_aq_read_nvm (
                &UndiPrivateData->NicInfo.Hw,
                0,
                VpdTlvOffset * 2,
                *VpdSizeInWords * 2,
                VpdBuffer,
                TRUE,
                TRUE,
                NULL
              );
  if (IceStatus != ICE_SUCCESS) {
    return EFI_DEVICE_ERROR;
  }

  ice_release_nvm (&UndiPrivateData->NicInfo.Hw);
  return EFI_SUCCESS;
}

/** Write VPD buffer into TLV module.

   @param[in]  UndiPrivateData        Pointer to the driver data
   @param[out] VpdBuffer              Pointer to the output buffer
   @param[in]  VpdSizeInWords         Length VPD buffer to be written into TLV

   @retval     EFI_SUCCESS            VPD written to buffer successfully
   @retval     EFI_INVALID_PARAMETER  UndiPrivateData is NULL or VpdBuffer is null
   @retval     EFI_DEVICE_ERROR       Failed to write TLV module
   @retval     EFI_DEVICE_ERROR       Failed to acquire NVM resource
   @retval     EFI_DEVICE_ERROR       Failed to find VPD TLV
**/
EFI_STATUS
IceWriteVpdBuffer (
  IN  UNDI_PRIVATE_DATA *UndiPrivateData,
  OUT UINT16            *VpdBuffer,
  IN  UINT16            *VpdSizeInWords
  )
{
  enum ice_status IceStatus;
  UINT16 VpdTlvOffset;
  EFI_STATUS Status;

  if (UndiPrivateData == NULL || VpdBuffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  IceStatus = ice_get_pfa_module_tlv (&UndiPrivateData->NicInfo.Hw, &VpdTlvOffset, VpdSizeInWords, ICE_SR_VPD_PTR);
  if (IceStatus != ICE_SUCCESS) {
    DEBUGPRINT (CRITICAL, ("ice_get_pfa_module_tlv failed with status %d\n", IceStatus));
    return EFI_DEVICE_ERROR;
  }

  //We have to skip 2 words, containing TLV type and length
  VpdTlvOffset += 2;

  IceStatus = ice_acquire_nvm (&UndiPrivateData->NicInfo.Hw, ICE_RES_WRITE);
  if (IceStatus != ICE_SUCCESS) {
    return EFI_DEVICE_ERROR;
  }
  IceBlockIt (&UndiPrivateData->NicInfo, TRUE);
  IceStatus = ice_aq_update_nvm (
                &UndiPrivateData->NicInfo.Hw,
                0,
                VpdTlvOffset * 2,
                *VpdSizeInWords * 2,
                VpdBuffer,
                TRUE,
                0,
                NULL
                );
  if (IceStatus != ICE_SUCCESS) {
    return EFI_DEVICE_ERROR;
  }
  Status = AwaitReceiveQueueEvent (
             &UndiPrivateData->NicInfo,
             ice_aqc_opc_nvm_write,
             NVM_OPERATION_TIMEOUT_IN_1MS_UNITS
             );
  if (EFI_ERROR (Status)) {
    DEBUGPRINT (CRITICAL, ("AwaitReceiveQueueEvent returned %d\n", Status));
  }

  ice_release_nvm (&UndiPrivateData->NicInfo.Hw);

  // Software takes ownership over the NVM resource for activate
  IceStatus = ice_acquire_nvm (&UndiPrivateData->NicInfo.Hw, ICE_RES_WRITE);
  if (IceStatus != ICE_SUCCESS) {
    return EFI_DEVICE_ERROR;
  }

  IceStatus = ice_nvm_write_activate (&UndiPrivateData->NicInfo.Hw, 0, NULL);
  if (IceStatus != ICE_SUCCESS) {
    DEBUGPRINT (CRITICAL, ("ice_nvm_write_activate returned %d\n", IceStatus));
    return EFI_DEVICE_ERROR;
  }

  Status = AwaitReceiveQueueEvent (
             &UndiPrivateData->NicInfo,
             ice_aqc_opc_nvm_write_activate,
             NVM_OPERATION_TIMEOUT_IN_1MS_UNITS
             );
  ice_release_nvm (&UndiPrivateData->NicInfo.Hw);
  IceBlockIt (&UndiPrivateData->NicInfo, FALSE);

  return EFI_SUCCESS;
}

/** Checks if device is Columbiaville.

   @param[in]   UndiPrivateData    Pointer to the driver private data.
   @param[out]  Supported          Tells whether the feature is supported.

   @retval   EFI_SUCCESS     Successfully retrieved the information about support.
**/
EFI_STATUS
IsCvlDevice (
  IN   UNDI_PRIVATE_DATA  *UndiPrivateData,
  OUT  BOOLEAN            *Supported
  )
{
  *Supported = (UndiPrivateData->NicInfo.Hw.mac_type == ICE_MAC_E810);

  return EFI_SUCCESS;
}


