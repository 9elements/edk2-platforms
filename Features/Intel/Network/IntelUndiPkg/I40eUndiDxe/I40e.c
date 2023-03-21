/**************************************************************************

Copyright (c) 2012 - 2022, Intel Corporation. All rights reserved.

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
#include "I40e.h"
#include "EepromConfig.h"
#include "DeviceSupport.h"


#include <Library/HiiLib.h>
#include "Hii/Hii.h"


/** Dumps LAN context and HMC related info

   @param[in]   AdapterInfo   Pointer to the NIC data structure information
                              the UNDI driver is layering on
   @param[in]   QueueNumber   Number of queue
   @param[in]   HmcType       Type of HMC (only LAN_TX and LAN_RX matters)

   @return   Information dumped to standard output
**/
EFI_STATUS
HmcDump (
  IN DRIVER_DATA                 *AdapterInfo,
  IN UINT16                       QueueNumber,
  IN enum i40e_hmc_lan_rsrc_type  HmcType
  )
{
  UINT32 ControlReg;
  UINT32 ByteLength = 0;
  UINT32 QueueType  = 0;
  UINT32 SubLine    = 0;

  switch (HmcType) {
  case I40E_HMC_LAN_RX:
    ByteLength = I40E_HMC_OBJ_SIZE_RXQ;
    QueueType  = LANCTXCTL_QUEUE_TYPE_RX;
    break;
  case I40E_HMC_LAN_TX:
    ByteLength = I40E_HMC_OBJ_SIZE_TXQ;
    QueueType  = LANCTXCTL_QUEUE_TYPE_TX;
    break;
  default:
    return EFI_DEVICE_ERROR;
    break;
  }


  for (SubLine = 0; SubLine < (ByteLength / SUB_LINE_LENGTH); SubLine++)
  {
    ControlReg = ((UINT32) QueueNumber << I40E_PFCM_LANCTXCTL_QUEUE_NUM_SHIFT) |
                 ((UINT32) QueueType << I40E_PFCM_LANCTXCTL_QUEUE_TYPE_SHIFT) |
                 ((UINT32) SubLine << I40E_PFCM_LANCTXCTL_SUB_LINE_SHIFT) |
                 ((UINT32) 0 << I40E_PFCM_LANCTXCTL_OP_CODE_SHIFT);

    I40eWrite32 (AdapterInfo, I40E_PFCM_LANCTXCTL, ControlReg);
    Print (L"I40E_PFCM_LANCTXCTL = %x\n", ControlReg);
    while ((I40eRead32 (
              AdapterInfo,
              I40E_PFCM_LANCTXSTAT
            ) & I40E_PFCM_LANCTXSTAT_CTX_DONE_MASK) == 0)
    {
      ;
    }
    Print (
      L"HMC function %d, Queue %d, Type %d, SubLine %x: %x %x %x %x\n",
      AdapterInfo->Function,
      QueueNumber,
      QueueType,
      SubLine,
      I40eRead32 (AdapterInfo, I40E_PFCM_LANCTXDATA (0)),
      I40eRead32 (AdapterInfo, I40E_PFCM_LANCTXDATA (1)),
      I40eRead32 (AdapterInfo, I40E_PFCM_LANCTXDATA (2)),
      I40eRead32 (AdapterInfo, I40E_PFCM_LANCTXDATA (3))
    );
  }


  return EFI_SUCCESS;
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

  @retval     PXE_STATCODE_NO_DATA        There is no data to receive.
  @retval     PXE_STATCODE_DEVICE_FAILURE AdapterInfo is NULL.
  @retval     PXE_STATCODE_DEVICE_FAILURE Device failure on packet receive.
  @retval     PXE_STATCODE_INVALID_CPB    Invalid CPB/DB parameters.
  @retval     PXE_STATCODE_NOT_STARTED    Rx queue not started.
  @retval     PXE_STATCODE_SUCCESS        Received data passed to the protocol.
**/
UINTN
I40eReceive (
  IN DRIVER_DATA     *AdapterInfo,
  IN PXE_CPB_RECEIVE *CpbReceive,
  IN PXE_DB_RECEIVE  *DbReceive
  )
{
  PXE_STATCODE                StatCode;
  EFI_STATUS                  Status;

  UINT8                       *RxBuffer;
  UINT16                      RxBufferSize;
  UINT16                      BytesReceived;
  UINT16                      PacketLength;

  ETHER_HEADER                *Header;
  PXE_FRAME_TYPE              PacketType;

  if (AdapterInfo == NULL) {
    // Should not happen
    ASSERT (AdapterInfo != NULL);
    StatCode = PXE_STATCODE_DEVICE_FAILURE;
    goto Exit;
  }

  if ((CpbReceive == NULL)
    || (CpbReceive->BufferLen == 0)
    || (CpbReceive->BufferLen > 0xFFFF)
    || (CpbReceive->BufferAddr == 0))
  {
    StatCode = PXE_STATCODE_INVALID_CPB;
    goto Exit;
  }

  if (DbReceive == NULL) {
    StatCode = PXE_STATCODE_INVALID_CDB;
    goto Exit;
  }

  // Try to get packet from Rx ring
  RxBuffer      = (UINT8*) (UINTN) CpbReceive->BufferAddr;
  RxBufferSize  = (UINT16) CpbReceive->BufferLen;
  BytesReceived = RxBufferSize;

  Status = ReceiveGetPacket (
             AdapterInfo,
             RxBuffer,
             &BytesReceived,
             &PacketLength
             );

  switch (Status) {
  case EFI_SUCCESS:
    // Packet received successfully
    DEBUGPRINT (RX, ("Packet received successfully.\n"));
    break;

  case EFI_NOT_STARTED:
    // Rx ring not started yet
    StatCode = PXE_STATCODE_NOT_STARTED;
    DEBUGPRINT (RX, ("Ring not started.\n"));
    goto Exit;

  case EFI_DEVICE_ERROR:
  case EFI_NOT_READY:
    // No data in case device fails or no packet is received
    StatCode = PXE_STATCODE_NO_DATA;
    goto Exit;

  default:
    // Other possible funny cases
    ASSERT_EFI_ERROR (Status);
    StatCode = PXE_STATCODE_DEVICE_FAILURE;
    goto Exit;
  }

  PacketType  = PXE_FRAME_TYPE_NONE;
  Header      = (ETHER_HEADER*) RxBuffer;

  // Fill the DB with information about the packet
  DbReceive->FrameLen         = PacketLength;
  DbReceive->MediaHeaderLen   = PXE_MAC_HEADER_LEN_ETHER;

  // Obtain packet type from MAC address
  if (CompareMem (Header->DestAddr, AdapterInfo->Hw.mac.addr, PXE_HWADDR_LEN_ETHER) == 0) {
    DEBUGPRINT (RX, ("Unicast packet\n"));
    PacketType = PXE_FRAME_TYPE_UNICAST;
  } else if (CompareMem (Header->DestAddr, AdapterInfo->BroadcastNodeAddress, PXE_HWADDR_LEN_ETHER) == 0) {
    DEBUGPRINT (RX, ("Broadcast packet\n"));
    PacketType = PXE_FRAME_TYPE_BROADCAST;
  } else if (BIT_TEST (Header->DestAddr[0], 1)) {
    DEBUGPRINT (RX, ("Multicast packet\n"));
    PacketType = PXE_FRAME_TYPE_MULTICAST;
  } else {
    DEBUGPRINT (RX, ("Promiscuous packet\n"));
    PacketType = PXE_FRAME_TYPE_PROMISCUOUS;
  }

  DbReceive->Type     = PacketType;
  DbReceive->Protocol = Header->Type;
  CopyMem (DbReceive->SrcAddr, Header->SrcAddr, PXE_HWADDR_LEN_ETHER);
  CopyMem (DbReceive->DestAddr, Header->DestAddr, PXE_HWADDR_LEN_ETHER);
  StatCode = PXE_STATCODE_SUCCESS;

Exit:
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

  @retval     PXE_STATCODE_SUCCESS          Packet enqueued for transmit.
  @retval     PXE_STATCODE_DEVICE_FAILURE   AdapterInfo parameter is NULL.
  @retval     PXE_STATCODE_DEVICE_FAILURE   Failed to send packet.
  @retval     PXE_STATCODE_INVALID_CPB      CPB invalid.
  @retval     PXE_STATCODE_UNSUPPORTED      Fragmented tranmission was requested.
  @retval     PXE_STATCODE_QUEUE_FULL       Tx queue is full.
**/
UINTN
I40eTransmit (
  IN DRIVER_DATA *AdapterInfo,
  IN UINT64       Cpb,
  IN UINT16       OpFlags
  )
{
  PXE_STATCODE                StatCode;
  EFI_STATUS                  Status;

  PXE_CPB_TRANSMIT            *TxBuffer;

  UINT16                      PacketLength;
  BOOLEAN                     IsBlocking;

  if (AdapterInfo == NULL) {
    // Should not happen
    ASSERT (AdapterInfo != NULL);
    StatCode = PXE_STATCODE_DEVICE_FAILURE;
    goto Exit;
  }

  if (Cpb == 0) {
    StatCode = PXE_STATCODE_INVALID_CPB;
    goto Exit;
  }

  if (BIT_TEST (OpFlags, PXE_OPFLAGS_TRANSMIT_FRAGMENTED)) {
    // Fragmented transmit
    // Not necessary for Windows boot case. Nevertheless, this needs to be
    // implemented.
    DEBUGPRINT (TX, ("Fragmented transmit\n"));
    StatCode = PXE_STATCODE_UNSUPPORTED;
    goto Exit;
  } else {
    // Single transmit
    DEBUGPRINT (TX, ("Single transmit\n"));

    TxBuffer      = (PXE_CPB_TRANSMIT *) (UINTN) Cpb;
    PacketLength  = (UINT16) ((UINT16) TxBuffer->DataLen + TxBuffer->MediaheaderLen);

    if (TxBuffer->FrameAddr == 0
      || PacketLength == 0)
    {
      StatCode = PXE_STATCODE_INVALID_CPB;
      goto Exit;
    }

    IsBlocking = BIT_TEST (OpFlags, PXE_OPFLAGS_TRANSMIT_BLOCK);

    Status = TransmitSend (
               AdapterInfo,
               TxBuffer->FrameAddr,
               PacketLength,
               IsBlocking
               );

    switch (Status) {
    case EFI_SUCCESS:
      StatCode = PXE_STATCODE_SUCCESS;
      break;

    case EFI_OUT_OF_RESOURCES:
      StatCode = PXE_STATCODE_QUEUE_FULL;
      goto Exit;

    default:
      ASSERT_EFI_ERROR (Status);
      StatCode = PXE_STATCODE_DEVICE_FAILURE;
      goto Exit;
    }
  }

Exit:
  return StatCode;
}

/** Free TX buffers that have been transmitted by the hardware.

  @param[in]   AdapterInfo  Pointer to the NIC data structure information which
                           the UNDI driver is layering on.
  @param[in]   NumEntries   Number of entries in the array which can be freed.
  @param[out]  TxBuffer     Array to pass back free TX buffer

  @return      Number of TX buffers written.
**/
UINT16
I40eFreeTxBuffers (
  IN  DRIVER_DATA *AdapterInfo,
  IN  UINT16       NumEntries,
  OUT UINT64      *TxBuffer
  )
{
  TRANSMIT_RING         *TxRing;
  UINT16                i;
  EFI_STATUS            Status;
  EFI_VIRTUAL_ADDRESS   FreeTxBuffer;

  if (AdapterInfo == NULL
    || TxBuffer == NULL
    || NumEntries == 0)
  {
    ASSERT (AdapterInfo != NULL);
    ASSERT (TxBuffer != NULL);
    ASSERT (NumEntries != 0);
    return 0;
  }

  i       = 0;
  TxRing  = TX_RING_FROM_ADAPTER (AdapterInfo);

  Status = TransmitScanDescriptors (AdapterInfo);

  switch (Status) {
  case EFI_SUCCESS:
  case EFI_NOT_READY:
    break;

  default:
    ASSERT_EFI_ERROR (Status);
    return 0;
  }

  do {
    if (i >= NumEntries) {
      // TxBuffer is 100% filled with packets
      break;
    }

    Status = TransmitReleaseBuffer (
               AdapterInfo,
               &FreeTxBuffer
               );

    if (Status == EFI_SUCCESS) {
      TxBuffer[i++] = FreeTxBuffer;
    }

  } while (!EFI_ERROR (Status));

  return i;
}

/** Sets receive filters.

  @param[in]  AdapterInfo  Pointer to the adapter structure
  @param[in]  NewFilter    A PXE_OPFLAGS bit field indicating what filters to use.

  @return     Broad/Multicast and promiscous settings are set according to NewFilter
**/
VOID
I40eSetFilter (
  IN DRIVER_DATA *AdapterInfo,
  IN UINT16       NewFilter
  )
{
  BOOLEAN                 ChangedPromiscuousFlag;
  BOOLEAN                 ChangedMulticastPromiscuousFlag;
  BOOLEAN                 ChangedBroadcastFlag;
  enum i40e_status_code   I40eStatus;

  DEBUGPRINT (RXFILTER, ("NewFilter %x= \n", NewFilter));

  ChangedPromiscuousFlag = FALSE;
  ChangedMulticastPromiscuousFlag = FALSE;
  ChangedBroadcastFlag = FALSE;

  if (NewFilter & PXE_OPFLAGS_RECEIVE_FILTER_PROMISCUOUS) {
    if (!AdapterInfo->Vsi.EnablePromiscuous) {
      ChangedPromiscuousFlag = TRUE;
    }
    AdapterInfo->Vsi.EnablePromiscuous = TRUE;
    DEBUGPRINT (RXFILTER, ("  Promiscuous\n"));
  }

  if (NewFilter & PXE_OPFLAGS_RECEIVE_FILTER_BROADCAST) {
    if (!AdapterInfo->Vsi.EnableBroadcast) {
      ChangedBroadcastFlag = TRUE;
    }
    AdapterInfo->Vsi.EnableBroadcast = TRUE;
    DEBUGPRINT (RXFILTER, ("  Broadcast\n"));
  }

  if (NewFilter & PXE_OPFLAGS_RECEIVE_FILTER_ALL_MULTICAST) {
    if (!AdapterInfo->Vsi.EnableMulticastPromiscuous) {
      ChangedMulticastPromiscuousFlag = TRUE;
    }
    AdapterInfo->Vsi.EnableMulticastPromiscuous = TRUE;
    DEBUGPRINT (RXFILTER, ("  MulticastPromiscuous\n"));
  }

  if (!AdapterInfo->DriverBusy) {
    if (ChangedPromiscuousFlag
      || ChangedMulticastPromiscuousFlag
      || ChangedBroadcastFlag)
    {
      I40eStatus = i40e_aq_set_vsi_unicast_promiscuous(
                     &AdapterInfo->Hw, AdapterInfo->Vsi.Seid,
                     AdapterInfo->Vsi.EnablePromiscuous,
                     NULL,
                     TRUE
                     );
      if (I40eStatus != I40E_SUCCESS) {
        DEBUGPRINT (CRITICAL, ("i40e_aq_set_vsi_unicast_promiscuous returned %d\n", I40eStatus));
      }

      I40eStatus = i40e_aq_set_vsi_multicast_promiscuous(
                     &AdapterInfo->Hw, AdapterInfo->Vsi.Seid,
                     AdapterInfo->Vsi.EnablePromiscuous || AdapterInfo->Vsi.EnableMulticastPromiscuous,
                     NULL
                     );
      if (I40eStatus != I40E_SUCCESS) {
        DEBUGPRINT (CRITICAL, ("i40e_aq_set_vsi_multicast_promiscuous returned %d\n", I40eStatus));
      }

      I40eStatus = i40e_aq_set_vsi_broadcast(
                     &AdapterInfo->Hw,
                     AdapterInfo->Vsi.Seid,
                     AdapterInfo->Vsi.EnablePromiscuous || AdapterInfo->Vsi.EnableBroadcast,
                     NULL
                     );
      if (I40eStatus != I40E_SUCCESS) {
        DEBUGPRINT (CRITICAL, ("i40e_aq_set_vsi_broadcast returned %d\n", I40eStatus));
      }
    }
  }

  AdapterInfo->RxFilter |= NewFilter;
}

/** Clears receive filters.

  @param[in]  AdapterInfo  Pointer to the adapter structure
  @param[in]  NewFilter    A PXE_OPFLAGS bit field indicating what filters to clear.

  @return     Broad/Multicast and promiscous settings are cleared according to NewFilter
**/
VOID
I40eClearFilter (
  IN DRIVER_DATA *AdapterInfo,
  IN UINT16       NewFilter
  )
{
  BOOLEAN                 ChangedPromiscuousFlag;
  BOOLEAN                 ChangedMulticastPromiscuousFlag;
  BOOLEAN                 ChangedBroadcastFlag;
  enum i40e_status_code   I40eStatus;

  ChangedPromiscuousFlag = FALSE;
  ChangedMulticastPromiscuousFlag = FALSE;
  ChangedBroadcastFlag = FALSE;

  DEBUGPRINT (RXFILTER, ("NewFilter %x= \n", NewFilter));

  if (NewFilter & PXE_OPFLAGS_RECEIVE_FILTER_PROMISCUOUS) {
    if (AdapterInfo->Vsi.EnablePromiscuous) {
      ChangedPromiscuousFlag = TRUE;
    }
    AdapterInfo->Vsi.EnablePromiscuous = FALSE;
    DEBUGPRINT (RXFILTER, ("  Promiscuous\n"));
  }

  if (NewFilter & PXE_OPFLAGS_RECEIVE_FILTER_BROADCAST) {
    if (AdapterInfo->Vsi.EnableBroadcast) {
      ChangedBroadcastFlag = TRUE;
    }
    AdapterInfo->Vsi.EnableBroadcast = FALSE;
    DEBUGPRINT (RXFILTER, ("  Broadcast\n"));
  }

  if (NewFilter & PXE_OPFLAGS_RECEIVE_FILTER_ALL_MULTICAST) {
    if (AdapterInfo->Vsi.EnableMulticastPromiscuous) {
      ChangedMulticastPromiscuousFlag = TRUE;
    }
    AdapterInfo->Vsi.EnableMulticastPromiscuous = FALSE;
    DEBUGPRINT (RXFILTER, ("  MulticastPromiscuous\n"));
  }

  if (!AdapterInfo->DriverBusy) {
    if (ChangedPromiscuousFlag
      || ChangedMulticastPromiscuousFlag
      || ChangedBroadcastFlag)
    {
      I40eStatus = i40e_aq_set_vsi_unicast_promiscuous(
                     &AdapterInfo->Hw, AdapterInfo->Vsi.Seid,
                     AdapterInfo->Vsi.EnablePromiscuous,
                     NULL,
                     TRUE
                   );
      if (I40eStatus != I40E_SUCCESS) {
        DEBUGPRINT (CRITICAL, ("i40e_aq_set_vsi_unicast_promiscuous returned %d\n", I40eStatus));
      }

      I40eStatus = i40e_aq_set_vsi_multicast_promiscuous(
                     &AdapterInfo->Hw, AdapterInfo->Vsi.Seid,
                     AdapterInfo->Vsi.EnableMulticastPromiscuous || AdapterInfo->Vsi.EnablePromiscuous,
                     NULL
                   );
      if (I40eStatus != I40E_SUCCESS) {
        DEBUGPRINT (CRITICAL, ("i40e_aq_set_vsi_multicast_promiscuous returned %d\n", I40eStatus));
      }

      I40eStatus = i40e_aq_set_vsi_broadcast(
                     &AdapterInfo->Hw,
                     AdapterInfo->Vsi.Seid,
                     AdapterInfo->Vsi.EnableBroadcast || AdapterInfo->Vsi.EnablePromiscuous,
                     NULL
                   );
      if (I40eStatus != I40E_SUCCESS) {
        DEBUGPRINT (CRITICAL, ("i40e_aq_set_vsi_broadcast returned %d\n", I40eStatus));
      }
    }
  }

  AdapterInfo->RxFilter &= ~NewFilter;
}

/** Adds MAC/VLAN elements to multicast list

   @param[in]   AdapterInfo   Pointer to the NIC data structure information
                              the UNDI driver is layering on

   @return  MAC/VLAN elements from adapter VSI structure are added to list
**/
VOID
I40eSetMcastList (
  IN DRIVER_DATA *AdapterInfo
  )
{
  struct i40e_aqc_remove_macvlan_element_data MacVlanElementsToRemove[MAX_MCAST_ADDRESS_CNT];
  struct i40e_aqc_add_macvlan_element_data    MacVlanElementsToAdd[MAX_MCAST_ADDRESS_CNT];
  enum i40e_status_code                       I40eStatus = I40E_SUCCESS;
  UINTN                                       i;
  UINT32                                      Reg = 0;

  DEBUGDUMP(
    INIT, ("SM(%d,%d):",
    AdapterInfo->Vsi.McastListToProgram.Length, AdapterInfo->Vsi.CurrentMcastList.Length)
  );

  DEBUGPRINT (
    RXFILTER, ("McastListToProgram.Length = %d\n",
    AdapterInfo->Vsi.McastListToProgram.Length)
  );
  DEBUGPRINT (
    RXFILTER, ("CurrentMcastList.Length = %d\n",
    AdapterInfo->Vsi.CurrentMcastList.Length)
  );

  if (!AdapterInfo->DriverBusy) {

    // Remove existing elements from the Forwarding Table
    if (AdapterInfo->Vsi.CurrentMcastList.Length > 0) {
      for (i = 0; i < AdapterInfo->Vsi.CurrentMcastList.Length; i++) {
        DEBUGPRINT (
          RXFILTER, ("Remove MAC %d, %x:%x:%x:%x:%x:%x\n",
          i,
          AdapterInfo->Vsi.CurrentMcastList.McAddr[i][0],
          AdapterInfo->Vsi.CurrentMcastList.McAddr[i][1],
          AdapterInfo->Vsi.CurrentMcastList.McAddr[i][2],
          AdapterInfo->Vsi.CurrentMcastList.McAddr[i][3],
          AdapterInfo->Vsi.CurrentMcastList.McAddr[i][4],
          AdapterInfo->Vsi.CurrentMcastList.McAddr[i][5])
        );
        CopyMem(
          MacVlanElementsToRemove[i].mac_addr,
          &AdapterInfo->Vsi.CurrentMcastList.McAddr[i],
          6
        );
        MacVlanElementsToRemove[i].vlan_tag = 0;
        MacVlanElementsToRemove[i].flags = I40E_AQC_MACVLAN_DEL_IGNORE_VLAN | I40E_AQC_MACVLAN_DEL_PERFECT_MATCH;
      }

      // For FPK - switch Rx drop policy when removing macvlan filter
      if (AdapterInfo->Hw.mac.type == I40E_MAC_X722) {
        Reg = I40eRead32 (AdapterInfo, I40E_PRTDCB_TC2PFC_RCB);
        I40eWrite32 (AdapterInfo, I40E_PRTDCB_TC2PFC_RCB, 0);
      }

      I40eStatus = i40e_aq_remove_macvlan(
                     &AdapterInfo->Hw,
                     AdapterInfo->Vsi.Seid,
                     MacVlanElementsToRemove,
                     AdapterInfo->Vsi.CurrentMcastList.Length,
                     NULL
                   );

      if (AdapterInfo->Hw.mac.type == I40E_MAC_X722) {
        I40eWrite32 (AdapterInfo, I40E_PRTDCB_TC2PFC_RCB, Reg);
      }

      if (I40eStatus != I40E_SUCCESS) {
        DEBUGPRINT (
          CRITICAL, ("i40e_aq_remove_macvlan returned %d, aq error = %d\n",
          I40eStatus,
          AdapterInfo->Hw.aq.asq_last_status)
        );
        for (i = 0; i < AdapterInfo->Vsi.CurrentMcastList.Length; i++) {
          DEBUGPRINT (CRITICAL, ("i40e_aq_remove_macvlan %d, %d\n", i, MacVlanElementsToRemove[i].error_code));
        }
      }
    }

    // Add new elements to the Forwarding Table
    if (AdapterInfo->Vsi.McastListToProgram.Length > 0) {
      for (i = 0; i < AdapterInfo->Vsi.McastListToProgram.Length; i++) {
        DEBUGPRINT (RXFILTER, ("Add MAC %d, %x:%x:%x:%x:%x:%x\n",
          i,
          AdapterInfo->Vsi.McastListToProgram.McAddr[i][0],
          AdapterInfo->Vsi.McastListToProgram.McAddr[i][1],
          AdapterInfo->Vsi.McastListToProgram.McAddr[i][2],
          AdapterInfo->Vsi.McastListToProgram.McAddr[i][3],
          AdapterInfo->Vsi.McastListToProgram.McAddr[i][4],
          AdapterInfo->Vsi.McastListToProgram.McAddr[i][5])
        );
        CopyMem(
          MacVlanElementsToAdd[i].mac_addr,
          &AdapterInfo->Vsi.McastListToProgram.McAddr[i],
          6
        );
        MacVlanElementsToAdd[i].vlan_tag = 0;
        MacVlanElementsToAdd[i].flags = I40E_AQC_MACVLAN_ADD_IGNORE_VLAN | I40E_AQC_MACVLAN_ADD_PERFECT_MATCH;
      }

      I40eStatus = i40e_aq_add_macvlan(
                     &AdapterInfo->Hw,
                     AdapterInfo->Vsi.Seid,
                     MacVlanElementsToAdd,
                     AdapterInfo->Vsi.McastListToProgram.Length,
                     NULL
                   );
      if (I40eStatus != I40E_SUCCESS) {
        DEBUGPRINT (
          CRITICAL, ("i40e_aq_add_macvlan returned %d, aq error = %d\n",
          I40eStatus,
          AdapterInfo->Hw.aq.asq_last_status)
        );
        for (i = 0; i < AdapterInfo->Vsi.McastListToProgram.Length; i++) {
          DEBUGPRINT (RXFILTER, ("i40e_aq_add_macvlan %d, %d\n", i, MacVlanElementsToAdd[i].match_method ));
        }
      }
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

/** Sets base queue in VSI structure to first queue from PF queue allocation
   register

   @param[in]   AdapterInfo  Pointer to the NIC data structure information
                             the UNDI driver is layering on

   @retval    EFI_SUCCESS   Base queue set successfully (always returned)
**/
EFI_STATUS
I40eSetupPfQueues (
  IN DRIVER_DATA *AdapterInfo
  )
{
  UINT32 Reg;
  UINT16 FirstQueue;
#if (DBG_LVL & INIT)
  UINT16 LastQueue;
#endif /* (DBG_LVL & INIT) */

  Reg = i40e_read_rx_ctl (&AdapterInfo->Hw, I40E_PFLAN_QALLOC);
  FirstQueue = (Reg & I40E_PFLAN_QALLOC_FIRSTQ_MASK) >> I40E_PFLAN_QALLOC_FIRSTQ_SHIFT;

#if (DBG_LVL & INIT)
  LastQueue = (Reg & I40E_PFLAN_QALLOC_LASTQ_MASK) >> I40E_PFLAN_QALLOC_LASTQ_SHIFT;
  DEBUGPRINT (INIT, ("PF Queues - first: %x, last: %x\n", FirstQueue, LastQueue));
#endif /* (DBG_LVL & INIT) */

  AdapterInfo->Vsi.BaseQueue = FirstQueue;
  return EFI_SUCCESS;
}

/** Get the current switch configuration from the device and
 extract a few useful SEID values.

 @param[in]  AdapterInfo  Pointer to the NIC data structure information
                          the UNDI driver is layering on

 @retval     EFI_SUCCESS            Switch configuration read successfully
 @retval     EFI_INVALID_PARAMETER  AdapterInfo is NULL
 @retval     EFI_DEVICE_ERROR       get_switch_config AQ cmd failed
 @retval     EFI_DEVICE_ERROR       No data returned in Switch Config
**/
EFI_STATUS
I40eReadSwitchConfiguration (
  IN DRIVER_DATA *AdapterInfo
  )
{
  struct i40e_aqc_get_switch_config_resp *SwitchConfig;
  UINT8                                   AqBuffer[I40E_AQ_LARGE_BUF];
  enum i40e_status_code                   I40eStatus;
  UINTN                                   i;
  UINT16                                  StartSeid;

  if (AdapterInfo == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  SwitchConfig = (struct i40e_aqc_get_switch_config_resp *) AqBuffer;
  StartSeid = 0;
  I40eStatus = i40e_aq_get_switch_config (
                 &AdapterInfo->Hw,
                 SwitchConfig,
                 sizeof (AqBuffer),
                 &StartSeid,
                 NULL
               );
  if (I40eStatus != I40E_SUCCESS) {
    DEBUGPRINT (
      CRITICAL, ("get switch config failed %d aq_err=%x\n",
      I40eStatus, AdapterInfo->Hw.aq.asq_last_status)
    );
    return EFI_DEVICE_ERROR;
  }

  if (SwitchConfig->header.num_reported == 0) {
    DEBUGPRINT (CRITICAL, ("No data returned in the SwitchConfig \n"));
    return EFI_DEVICE_ERROR;
  }

  for (i = 0; i < SwitchConfig->header.num_reported; i++) {
    DEBUGPRINT (
      INIT, ("type=%d seid=%d uplink=%d downlink=%d\n",
      SwitchConfig->element[i].element_type,
      SwitchConfig->element[i].seid,
      SwitchConfig->element[i].uplink_seid,
      SwitchConfig->element[i].downlink_seid)
    );

    switch (SwitchConfig->element[i].element_type) {
    case I40E_SWITCH_ELEMENT_TYPE_MAC:
      AdapterInfo->MacSeid = SwitchConfig->element[i].seid;
      break;
    case I40E_SWITCH_ELEMENT_TYPE_VEB:
      AdapterInfo->VebSeid = SwitchConfig->element[i].seid;
      break;
    case I40E_SWITCH_ELEMENT_TYPE_PF:
      AdapterInfo->PfSeid = SwitchConfig->element[i].seid;
      AdapterInfo->MainVsiSeid = SwitchConfig->element[i].uplink_seid;
      break;
    case I40E_SWITCH_ELEMENT_TYPE_VSI:
      AdapterInfo->MainVsiSeid = SwitchConfig->element[i].seid;
      AdapterInfo->PfSeid = SwitchConfig->element[i].downlink_seid;
      AdapterInfo->MacSeid = SwitchConfig->element[i].uplink_seid;
      break;

    // ignore these for now
    case I40E_SWITCH_ELEMENT_TYPE_VF:
    case I40E_SWITCH_ELEMENT_TYPE_EMP:
    case I40E_SWITCH_ELEMENT_TYPE_BMC:
    case I40E_SWITCH_ELEMENT_TYPE_PE:
    case I40E_SWITCH_ELEMENT_TYPE_PA:
      break;
    default:
      DEBUGPRINT (
        CRITICAL, ("Unknown element type=%d seid=%d\n",
        SwitchConfig->element[i].element_type,
        SwitchConfig->element[i].seid)
      );
      break;
    }
  }

  return EFI_SUCCESS;
}

/** Turn off VLAN stripping for the VSI

  @param[in]  AdapterInfo  Pointer to the NIC data structure information
                             the UNDI driver is layering on

  @retval    EFI_SUCCESS       VLAN stripping successfully disabled
  @retval    EFI_SUCCESS       VLAN stripping already disabled
  @retval    EFI_DEVICE_ERROR  Failed to update VSI param
**/
EFI_STATUS
I40eDisableVlanStripping (
  IN DRIVER_DATA *AdapterInfo
  )
{
  struct i40e_vsi_context VsiContext;
  enum i40e_status_code   I40eStatus;

  ZeroMem (&VsiContext, sizeof (VsiContext));


  if ((AdapterInfo->Vsi.Info.valid_sections & I40E_AQ_VSI_PROP_VLAN_VALID) == I40E_AQ_VSI_PROP_VLAN_VALID) {
    if ((AdapterInfo->Vsi.Info.port_vlan_flags & I40E_AQ_VSI_PVLAN_EMOD_MASK) == I40E_AQ_VSI_PVLAN_EMOD_MASK) {
      DEBUGPRINT (INIT, ("VLAN stripping already disabled\n"));
      return EFI_SUCCESS;
    }
  }

  AdapterInfo->Vsi.Info.valid_sections |= I40E_AQ_VSI_PROP_VLAN_VALID;
  AdapterInfo->Vsi.Info.port_vlan_flags = I40E_AQ_VSI_PVLAN_MODE_ALL |
                                          I40E_AQ_VSI_PVLAN_EMOD_NOTHING;

  VsiContext.seid = AdapterInfo->MainVsiSeid;
  CopyMem (&VsiContext.info, &AdapterInfo->Vsi.Info, sizeof (AdapterInfo->Vsi.Info));

  I40eStatus = i40e_aq_update_vsi_params (&AdapterInfo->Hw, &VsiContext, NULL);
  if (I40eStatus != I40E_SUCCESS) {
    DEBUGPRINT (
      CRITICAL, ("Update vsi failed, aq_err=%d\n",
      AdapterInfo->Hw.aq.asq_last_status)
    );
    return EFI_DEVICE_ERROR;
  }
  return EFI_SUCCESS;
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
I40eSetupPFSwitch (
  IN DRIVER_DATA *AdapterInfo
  )
{
  EFI_STATUS                               Status;
  struct i40e_vsi_context                  VsiCtx;
  struct i40e_aqc_add_macvlan_element_data MacVlan;
  enum i40e_status_code                    I40eStatus;

  I40eStatus = I40E_SUCCESS;
  Status = EFI_SUCCESS;
  ZeroMem (&VsiCtx, sizeof (VsiCtx));

  // Read the default switch configuration
  Status = I40eReadSwitchConfiguration (AdapterInfo);
  if (EFI_ERROR (Status)) {
    DEBUGPRINT (CRITICAL, ("I40eReadSwitchConfiguration returned %r\n", Status));
    return Status;
  }

  // Get main VSI parameters
  I40eStatus = I40eGetVsiParams (AdapterInfo, &VsiCtx);
  if (I40eStatus != I40E_SUCCESS) {
    DEBUGPRINT (
      CRITICAL, ("I40e_aq_get_vsi_params returned %d, aq_err %d\n",
      I40eStatus, AdapterInfo->Hw.aq.asq_last_status)
    );
    return EFI_DEVICE_ERROR;
  }

  DEBUGPRINT (
    INIT, ("VSI params: vsi_number=%d, VsiCtx.info.qs_handle[0]=%d\n",
    VsiCtx.vsi_number,
    VsiCtx.info.qs_handle[0])
  );

  AdapterInfo->Vsi.Id = VsiCtx.vsi_number;

  // Determnine which queues are used by this PF
  I40eSetupPfQueues (AdapterInfo);

  //  Set number of queue paires we use to 1
  AdapterInfo->NumLanQps = 1;

  // Check if VSI has enough queue pairs
  if ((AdapterInfo->Hw.func_caps.num_tx_qp < AdapterInfo->NumLanQps)
    || (AdapterInfo->Hw.func_caps.num_rx_qp < AdapterInfo->NumLanQps))
  {
    DEBUGPRINT (CRITICAL, ("Not enough qps available\n"));
    return EFI_DEVICE_ERROR;
  }

  // Store VSI parameters in VSI structure
  AdapterInfo->Vsi.Type = I40E_VSI_MAIN;
  AdapterInfo->Vsi.Flags = 0;
  AdapterInfo->Vsi.NumQueuePairs = AdapterInfo->NumLanQps;
  AdapterInfo->Vsi.NumDesc = AdapterInfo->TxRxDescriptorCount;
  AdapterInfo->Vsi.Seid = AdapterInfo->MainVsiSeid;
  CopyMem (&AdapterInfo->Vsi.Info, &VsiCtx.info, sizeof (VsiCtx.info));

  {
    struct i40e_filter_control_settings FilterControlSettings;

    ZeroMem (&FilterControlSettings, sizeof (FilterControlSettings));

    FilterControlSettings.hash_lut_size = I40E_HASH_LUT_SIZE_128;
    FilterControlSettings.enable_ethtype = TRUE;
    FilterControlSettings.enable_macvlan = TRUE;
    I40eStatus = i40e_set_filter_control (&AdapterInfo->Hw, &FilterControlSettings);
    if (I40eStatus != I40E_SUCCESS) {
      DEBUGPRINT (
        CRITICAL, ("i40e_set_filter_control returned %d, aq_err %d\n",
        I40eStatus, AdapterInfo->Hw.aq.asq_last_status)
      );
      return EFI_DEVICE_ERROR;
    }
  }

  SetMem (&MacVlan, sizeof (struct i40e_aqc_add_macvlan_element_data), 0);
  MacVlan.flags = I40E_AQC_MACVLAN_ADD_IGNORE_VLAN | I40E_AQC_MACVLAN_ADD_PERFECT_MATCH;
  CopyMem (MacVlan.mac_addr, &AdapterInfo->Hw.mac.addr, sizeof (MacVlan.mac_addr));

  I40eStatus = i40e_aq_add_macvlan (
                 &AdapterInfo->Hw,
                 AdapterInfo->MainVsiSeid,
                 &MacVlan,
                 1,
                 NULL
               );
  if (I40eStatus != I40E_SUCCESS) {
    DEBUGPRINT (
      CRITICAL, ("i40e_aq_add_macvlan returned %d, aq_err %d\n",
      I40eStatus,
      AdapterInfo->Hw.aq.asq_last_status)
    );
    return EFI_DEVICE_ERROR;
  }

  // Configure VLAN stripping on Rx packets
  Status = I40eDisableVlanStripping (AdapterInfo);
  if (EFI_ERROR (Status)) {
    DEBUGPRINT (CRITICAL, ("I40eDisableVlanStripping returned %r\n", Status));
    return Status;
  }

  return Status;
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
I40ePciInit (
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

/** Reads and prints adapter MAC address

   @param[in]   AdapterInfo   Pointer to the NIC data structure information
                             the UNDI driver is layering on

   @retval    EFI_SUCCESS       MAC address read successfully
   @retval    EFI_DEVICE_ERROR  Failed to get MAC address
**/
EFI_STATUS
I40eReadMacAddress (
  IN DRIVER_DATA *AdapterInfo
  )
{
  enum i40e_status_code I40eStatus;
  struct i40e_hw       *Hw;

  Hw = &AdapterInfo->Hw;

  // Get current MAC Address using the shared code function
  I40eStatus = i40e_get_mac_addr (Hw, Hw->mac.addr);
  if (I40eStatus != I40E_SUCCESS) {
    DEBUGPRINT (CRITICAL, ("i40e_get_mac_addr returned %d\n", I40eStatus));
    return EFI_DEVICE_ERROR;
  }

  // Assume this is also a permanent address and save it for the future
  CopyMem (Hw->mac.perm_addr, Hw->mac.addr, ETHER_MAC_ADDR_LEN);

  DEBUGPRINT (
    INIT, ("MAC Address = %02x:%02x:%02x:%02x:%02x:%02x\n",
    AdapterInfo->Hw.mac.addr[0],
    AdapterInfo->Hw.mac.addr[1],
    AdapterInfo->Hw.mac.addr[2],
    AdapterInfo->Hw.mac.addr[3],
    AdapterInfo->Hw.mac.addr[4],
    AdapterInfo->Hw.mac.addr[5])
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
**/
EFI_STATUS
I40eInitHw (
  IN DRIVER_DATA *AdapterInfo
  )
{
  enum i40e_status_code I40eStatus;
  EFI_STATUS            Status;
  struct i40e_hw *      Hw;
  UINT32                TmpReg0 = 0;

  Hw = &AdapterInfo->Hw;


  //  Initialize HMC structure for this Lan function. We need 1 Tx and 1 Rx queue.
  //  FCoE parameters are zeroed
#ifdef DIRECT_QUEUE_CTX_PROGRAMMING
  UNREFERENCED_1PARAMETER (I40eStatus);
#else /* NOT DIRECT_QUEUE_CTX_PROGRAMMING */
  I40eStatus = i40e_init_lan_hmc (Hw, 1, 1, 0, 0);
  if (I40eStatus != I40E_SUCCESS) {
    DEBUGPRINT (CRITICAL, ("i40e_init_lan_hmc returned %d\n", I40eStatus));
    return EFI_DEVICE_ERROR;
  }

  I40eStatus = i40e_configure_lan_hmc (Hw, I40E_HMC_MODEL_DIRECT_ONLY);
  if (I40eStatus != I40E_SUCCESS) {
    DEBUGPRINT (CRITICAL, ("i40e_configure_lan_hmc returned %d\n", I40eStatus));
    return EFI_DEVICE_ERROR;
  }
#endif /* DIRECT_QUEUE_CTX_PROGRAMMING */

  Status = GetLinkStatus (UNDI_PRIVATE_DATA_FROM_DRIVER_DATA (AdapterInfo), &AdapterInfo->WaitingForLinkUp);
  IF_RETURN (EFI_ERROR (Status), Status);

  Status = I40eSetupPFSwitch (AdapterInfo);
  if (EFI_ERROR (Status)) {
    DEBUGPRINT (CRITICAL, ("I40eSetupPFSwitch returned %r\n", Status));
    return Status;
  }

  Status = TransmitInitialize (
             AdapterInfo,
             AdapterInfo->TxRxDescriptorCount
             );

  if (EFI_ERROR (Status)) {
    DEBUGPRINT (CRITICAL, ("Failed to initialize Tx ring\n"));
    goto Exit;
  }

  DEBUGPRINT (INIT, ("Tx initialized.\n"));

  Status = ReceiveInitialize (
             AdapterInfo,
             AdapterInfo->TxRxDescriptorCount,
             I40E_RXBUFFER_2048
             );

  if (EFI_ERROR (Status)) {
    DEBUGPRINT (CRITICAL, ("Failed to initialize Rx ring\n"));
    goto ExitCleanTxRing;
  }

  DEBUGPRINT (INIT, ("Rx initialized.\n"));

  // Enable interrupt causes.
  I40eConfigureInterrupts (AdapterInfo);

  if (AdapterInfo->Hw.mac.type == I40E_MAC_X722) {
    TmpReg0 = I40eRead32 (AdapterInfo, I40E_PRTDCB_TC2PFC_RCB);
    I40eWrite32 (AdapterInfo, I40E_PRTDCB_TC2PFC_RCB, 0);
  }

  Status = TransmitStart (AdapterInfo);

  if (EFI_ERROR (Status)) {
    DEBUGPRINT (CRITICAL, ("Failed to start Tx ring\n"));
    goto ExitCleanRxRing;
  }

  DEBUGPRINT (INIT, ("Tx started.\n"));

  Status = ReceiveStart (AdapterInfo);

  if (EFI_ERROR (Status)) {
    DEBUGPRINT (CRITICAL, ("Failed to start Rx ring\n"));
    goto ExitStopTxRing;
  }

  DEBUGPRINT (INIT, ("Rx started\n"));

  if (AdapterInfo->Hw.mac.type == I40E_MAC_X722) {
    I40eWrite32 (AdapterInfo, I40E_PRTDCB_TC2PFC_RCB, TmpReg0);
  }

  AdapterInfo->HwInitialized = TRUE;
  goto Exit;

ExitStopTxRing:
  TransmitStop (AdapterInfo);

ExitCleanRxRing:
  ReceiveCleanup (AdapterInfo);

ExitCleanTxRing:
  TransmitCleanup (AdapterInfo);

Exit:
  return Status;
}

/** Performs I40eInitHw function for UNDI interface

   @param[in]   AdapterInfo   Pointer to the NIC data structure information
                             the UNDI driver is layering on

   @retval    PXE_STATCODE_SUCCESS   HW initialized successfully
   @retval    PXE_STATCODE_NOT_STARTED  Failed to initialize HW
**/
PXE_STATCODE
I40eInitialize (
  IN DRIVER_DATA *AdapterInfo
  )
{
#ifndef AVOID_HW_REINITIALIZATION
  EFI_STATUS Status;
#endif /* AVOID_HW_REINITIALIZATION */
  PXE_STATCODE PxeStatcode;

  DEBUGPRINT (INIT, ("Entering I40eInitialize\n"));

  PxeStatcode = PXE_STATCODE_SUCCESS;

  // Do not try to initialize hw again when it is already initialized
  if (AdapterInfo->HwInitialized == FALSE) {
    DEBUGPRINT (INIT, ("Hw is not initialized, calling I40eInitHw\n"));
#ifndef AVOID_HW_REINITIALIZATION
    Status = I40eInitHw (AdapterInfo);
    if (EFI_ERROR (Status)) {
      DEBUGPRINT (CRITICAL, ("I40eInitHw returns %r\n", Status));
      PxeStatcode = PXE_STATCODE_NOT_STARTED;
    }
#endif /* AVOID_HW_REINITIALIZATION */
  }
  AdapterInfo->DriverBusy = FALSE;
  return PxeStatcode;
}

/** Reverts the operations performed in I40eInitHw. Stops HW from child side

   @param[in]   AdapterInfo   Pointer to the NIC data structure information
                              the UNDI driver is layering on

   @retval   PXE_STATCODE_SUCCESS   HW is already not initialized
   @retval   PXE_STATCODE_SUCCESS   HW successfully stopped
**/
PXE_STATCODE
I40eShutdown (
  IN DRIVER_DATA *AdapterInfo
  )
{
  PXE_STATCODE PxeStatcode;
#ifndef AVOID_HW_REINITIALIZATION
  enum i40e_status_code   I40eStatus = I40E_SUCCESS;
  EFI_STATUS              Status;
#endif  /* AVOID_HW_REINITIALIZATION */
  DEBUGPRINT (INIT, ("Entering I40eShutdown\n"));

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
  Status = TransmitStop (AdapterInfo);
  if (EFI_ERROR (Status)) {
    DEBUGPRINT (CRITICAL, ("TransmitStop returned %r\n", Status));
    goto Exit;
  }

  Status = ReceiveStop (AdapterInfo);
  if (EFI_ERROR (Status)) {
    DEBUGPRINT (CRITICAL, ("ReceiveStop returned %r\n", Status));
    goto Exit;
  }

  Status = TransmitCleanup (AdapterInfo);
  if (EFI_ERROR (Status)) {
    DEBUGPRINT (CRITICAL, ("TransmitCleanup returned %r\n", Status));
    goto Exit;
  }

  Status = ReceiveCleanup (AdapterInfo);
  if (EFI_ERROR (Status)) {
    DEBUGPRINT (CRITICAL, ("ReceiveCleanup returned %r\n", Status));
    goto Exit;
  }

#ifndef DIRECT_QUEUE_CTX_PROGRAMMING
  I40eStatus = i40e_shutdown_lan_hmc (&AdapterInfo->Hw);
  if (I40eStatus != I40E_SUCCESS) {
    DEBUGPRINT (CRITICAL, ("i40e_shutdown_lan_hmc returned %d\n", I40eStatus));
  }

#endif  /* DIRECT_QUEUE_CTX_PROGRAMMING */
  // Disable all interrupt causes.
  I40eDisableInterrupts (AdapterInfo);

  AdapterInfo->HwInitialized = FALSE;

Exit:
  switch (Status) {
  case EFI_SUCCESS:
    PxeStatcode = PXE_STATCODE_SUCCESS;
    break;

  case EFI_ACCESS_DENIED:
    // Shutdown when packets are present
    PxeStatcode = PXE_STATCODE_BUSY;
    DEBUGPRINT (INIT, ("Shutdown requested where Tx packets were not freed.\n"));
    break;

  default:
    ASSERT_EFI_ERROR (Status);
    PxeStatcode = PXE_STATCODE_DEVICE_FAILURE;
    break;
  }
#else /* AVOID_HW_REINITIALIZATION */
  PxeStatcode = PXE_STATCODE_SUCCESS;
#endif  /* !AVOID_HW_REINITIALIZATION */
  return PxeStatcode;
}

/** Performs HW reset by reinitialization

   @param[in]   AdapterInfo   Pointer to the NIC data structure information
                              the UNDI driver is layering on

   @retval   PXE_STATCODE_SUCCESS      Successfull HW reset
   @retval   PXE_STATCODE_NOT_STARTED  Failed to initialize HW
**/
PXE_STATCODE
I40eReset (
  IN DRIVER_DATA *AdapterInfo
  )
{
  PXE_STATCODE PxeStatcode;
#ifndef AVOID_HW_REINITIALIZATION
  EFI_STATUS Status;
#endif /* AVOID_HW_REINITIALIZATION */

  DEBUGPRINT (INIT, ("Entering I40eReset\n"));

  // Do not reinitialize the adapter when it has already been initialized
  // This saves the time required for initialization
  if (!AdapterInfo->HwInitialized) {
#ifndef AVOID_HW_REINITIALIZATION
    Status = I40eInitHw (AdapterInfo);
    if (EFI_ERROR (Status)) {
      DEBUGPRINT (CRITICAL, ("I40eInitHw returns %r\n", Status));
      return PXE_STATCODE_NOT_STARTED;
    }
#endif /* AVOID_HW_REINITIALIZATION */
  } else {
    DEBUGPRINT (I40E, ("Skipping adapter reset\n"));
  }

  PxeStatcode = PXE_STATCODE_SUCCESS;
  return PxeStatcode;
}

/** Configures internal interrupt causes on current PF.

   @param[in]   AdapterInfo  Pointer to the NIC data structure information which
                             the UNDI driver is layering on

   @return   Interrupt causes are configured for current PF
**/
VOID
I40eConfigureInterrupts (
  IN DRIVER_DATA *AdapterInfo
  )
{
  UINT32  RegVal = 0;

  I40eWrite32 (AdapterInfo, I40E_PFINT_ITR0(0), 0);
  I40eWrite32 (AdapterInfo, I40E_PFINT_ITR0(1), 0);
  I40eWrite32 (AdapterInfo, I40E_PFINT_LNKLST0, 0);

  RegVal = I40E_PFINT_ICR0_ENA_ADMINQ_MASK | I40E_PFINT_ICR0_ENA_LINK_STAT_CHANGE_MASK;
  I40eWrite32 (AdapterInfo, I40E_PFINT_ICR0_ENA, RegVal);

  // Enable Queue 0 for receive interrupt
  RegVal = I40E_QINT_RQCTL_CAUSE_ENA_MASK | I40E_QINT_RQCTL_ITR_INDX_MASK |
    0x1 << I40E_QINT_RQCTL_NEXTQ_INDX_SHIFT |
    I40E_QUEUE_TYPE_TX << I40E_QINT_RQCTL_NEXTQ_TYPE_SHIFT;
  I40eWrite32 (AdapterInfo, I40E_QINT_RQCTL(0), RegVal);

  // Enable Queue 1 for transmit interrupt
  RegVal = I40E_QINT_TQCTL_CAUSE_ENA_MASK | I40E_QINT_TQCTL_ITR_INDX_MASK |
    0x1 << I40E_QINT_RQCTL_MSIX0_INDX_SHIFT | I40E_QINT_TQCTL_NEXTQ_INDX_MASK;
  I40eWrite32 (AdapterInfo, I40E_QINT_TQCTL(0), RegVal);
}

/** Disables internal interrupt causes on current PF.

   @param[in]   AdapterInfo  Pointer to the NIC data structure information which
                             the UNDI driver is layering on

   @return   Interrupt causes are disabled for current PF
**/
VOID
I40eDisableInterrupts (
  IN DRIVER_DATA *AdapterInfo
  )
{
  // Disable all non-queue interrupt causes
  I40eWrite32 (AdapterInfo, I40E_PFINT_ICR0_ENA, 0);

  // Disable receive queue interrupt causes
  I40eWrite32 (AdapterInfo, I40E_QINT_RQCTL(0), 0);

  // Disable transmit queue interrupt
  I40eWrite32 (AdapterInfo, I40E_QINT_TQCTL(0), 0);
}

/** Read function capabilities using AQ command.

   @param[in]   AdapterInfo   Pointer to the NIC data structure information
                             the UNDI driver is layering on

   @retval   EFI_INVALID_PARAMETER   Failed to allocate memory for function
                                     capabilities buffer
   @retval   EFI_OUT_OF_RESOURCES    Failed to allocate memory for function
                                     capabilities buffer
   @retval   EFI_DEVICE_ERROR        Discover capabilities AQ cmd failed
**/
EFI_STATUS
I40eDiscoverCapabilities (
  IN DRIVER_DATA *AdapterInfo
  )
{
  enum i40e_status_code                           I40eStatus;
  UINT16                                          BufferSize;
  UINT16                                          BufferSizeNeeded;
  struct i40e_aqc_list_capabilities_element_resp *CapabilitiesBuffer;
  UINT32                                          i = 0;

  BufferSize = 40 * sizeof (struct i40e_aqc_list_capabilities_element_resp);

  do {
    i++;

    CapabilitiesBuffer = AllocateZeroPool (BufferSize);
    if (CapabilitiesBuffer == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    I40eStatus = i40e_aq_discover_capabilities (
                   &AdapterInfo->Hw,
                   CapabilitiesBuffer,
                   BufferSize,
                   &BufferSizeNeeded,
                   i40e_aqc_opc_list_func_capabilities,
                   NULL
                 );

    // Free memory that was required only internally
    // in i40e_aq_discover_capabilities
    gBS->FreePool (CapabilitiesBuffer);

    if (AdapterInfo->Hw.aq.asq_last_status == I40E_AQ_RC_ENOMEM) {

      // Buffer passed was to small, use buffer size returned by the function
      BufferSize = BufferSizeNeeded;
    } else if (AdapterInfo->Hw.aq.asq_last_status != I40E_AQ_RC_OK) {
      return EFI_DEVICE_ERROR;
    }

    // Infinite loop protection
    if (i > 100) {
      DEBUGPRINT (CRITICAL, ("i40e_aq_discover_capabilities returns %x\n", I40eStatus));
      return EFI_DEVICE_ERROR;
    }
  } while (I40eStatus != I40E_SUCCESS);

  i = 0;
  do {
      i++;
      CapabilitiesBuffer = AllocateZeroPool (BufferSize);
      if (CapabilitiesBuffer == NULL) {
        return EFI_OUT_OF_RESOURCES;
      }

      I40eStatus = i40e_aq_discover_capabilities (
                     &AdapterInfo->Hw,
                     CapabilitiesBuffer,
                     BufferSize,
                     &BufferSizeNeeded,
                     i40e_aqc_opc_list_dev_capabilities,
                     NULL
                   );

      // Free memory that was required only internally
      // in i40e_aq_discover_capabilities
      gBS->FreePool (CapabilitiesBuffer);

      if (AdapterInfo->Hw.aq.asq_last_status == I40E_AQ_RC_ENOMEM) {

        // Buffer passed was to small, use buffer size returned by the function
        BufferSize = BufferSizeNeeded;
      } else if (AdapterInfo->Hw.aq.asq_last_status != I40E_AQ_RC_OK) {
        return EFI_DEVICE_ERROR;
      }

      // Endless loop protection
      if (i > 100) {
        DEBUGPRINT (CRITICAL, ("i40e_aq_discover_capabilities returns %x\n", I40eStatus));
        return EFI_DEVICE_ERROR;
      }
    } while (I40eStatus != I40E_SUCCESS);

  return EFI_SUCCESS;
}

/** Checks if reset is done.

   @param[in]   AdapterInfo   Pointer to the NIC data structure information
                              the UNDI driver is layering on
   @param[in]   ResetMask     Mask to compare with read reg. value if reset was done

   @retval    EFI_SUCCESS       Function ended successfully
   @retval    EFI_DEVICE_ERROR  Timeout when waiting for device to become active
   @retval    EFI_DEVICE_ERROR  Timeout when waiting for reset done
**/
EFI_STATUS
I40eCheckResetDone (
  IN DRIVER_DATA *AdapterInfo,
  IN UINT32       ResetMask
  )
{
  EFI_STATUS       Status;
  UINT32           Reg;
  UINTN            i = 0;
  struct  i40e_hw *Hw;

  Hw = &AdapterInfo->Hw;
  Status = EFI_SUCCESS;


  // First wait until device becomes active.
  while (1) {
    Reg = rd32 (Hw, I40E_GLGEN_RSTAT);
    if ((Reg & I40E_GLGEN_RSTAT_DEVSTATE_MASK) == 0) {
      break;
    }
    DelayInMicroseconds (AdapterInfo, 100);
    if (i++ > I40E_GLNVM_ULD_TIMEOUT) {
      DEBUGPRINT (CRITICAL, ("Device activation error\n"));
      Status = EFI_DEVICE_ERROR;
      break;
    }
  }

  i = 0;

  // Now wait for reset done indication.
  Reg = rd32 (Hw, I40E_GLNVM_ULD);
  while (1) {
    Reg = rd32 (Hw, I40E_GLNVM_ULD);
    if ((Reg & ResetMask) == ResetMask) {
      break;
    }
    DelayInMicroseconds (AdapterInfo, 100);
    if (i++ > I40E_GLNVM_ULD_TIMEOUT) {
      DEBUGPRINT (CRITICAL, ("Timeout waiting for reset done\n"));
      Status = EFI_DEVICE_ERROR;
      break;
    }
  }

  return Status;
}

/** Triggers global reset

   @param[in]   AdapterInfo   Pointer to the NIC data structure information
                              the UNDI driver is layering on

   @retval   EFI_SUCCESS   Reset triggered successfully
**/
EFI_STATUS
I40eTriggerGlobalReset (
  IN DRIVER_DATA *AdapterInfo
  )
{
  UINT32           Reg;
  struct i40e_hw  *Hw;

  Hw = &AdapterInfo->Hw;

  Reg = 0x1 << I40E_GLGEN_RTRIG_GLOBR_SHIFT;
  wr32 (Hw, I40E_GLGEN_RTRIG, Reg);


  return EFI_SUCCESS;
}


/** This function checks if any  other instance of driver is loaded on this PF by
   reading PFGEN_DRUN register.

   If not it writes the bit in the register to let know other components that
   the PF is in use.

   @param[in]   AdapterInfo   Pointer to the NIC data structure information
                             the UNDI driver is layering on

   @retval   TRUE   The PF is free to use for Tx/Rx
   @retval   FALSE  The PF cannot be used for Tx/Rx
**/
BOOLEAN
I40eAquireControllerHw (
  IN DRIVER_DATA *AdapterInfo
  )
{
  UINT32 RegValue;

  RegValue = rd32 (&AdapterInfo->Hw, I40E_PFGEN_DRUN);

  if (RegValue & I40E_PFGEN_DRUN_DRVUNLD_MASK) {

    // bit set means other driver is loaded on this pf
    return FALSE;
  }

  RegValue |= I40E_PFGEN_DRUN_DRVUNLD_MASK;
  wr32 (&AdapterInfo->Hw, I40E_PFGEN_DRUN, RegValue);
  return TRUE;
}

/** Release this PF by clearing the bit in PFGEN_DRUN register.

   @param[in]   AdapterInfo   Pointer to the NIC data structure information
                             the UNDI driver is layering on

   @return   PFGEN_DRUN driver unload bit is cleared
**/
VOID
I40eReleaseControllerHw (
  IN DRIVER_DATA *AdapterInfo
  )
{
  UINT32 RegValue;

  RegValue = rd32 (&AdapterInfo->Hw, I40E_PFGEN_DRUN);
  RegValue &= ~I40E_PFGEN_DRUN_DRVUNLD_MASK;
  wr32 (&AdapterInfo->Hw, I40E_PFGEN_DRUN, RegValue);
}

/** This function is called as early as possible during driver start to ensure the
   hardware has enough time to autonegotiate when the real SNP device initialize call
   is made.

   @param[in]   AdapterInfo   Pointer to the NIC data structure information
                              the UNDI driver is layering on

   @retval   EFI_SUCCESS            First time init end up successfully
   @retval   EFI_INVALID_PARAMETER  Firmware version is newer than expected
   @retval   EFI_DEVICE_ERROR       Failed to init shared code
   @retval   EFI_DEVICE_ERROR       PF reset failed
   @retval   EFI_DEVICE_ERROR       Init Admin Queue failed
   @retval   EFI_NOT_FOUND          Failed reading MFP configuration
   @retval   EFI_DEVICE_ERROR       Failed reading MFP configuration
   @retval   EFI_INVALID_PARAMETER  Failed to discover (read) capabilities
   @retval   EFI_OUT_OF_RESOURCES   Failed to discover (read) capabilities
   @retval   EFI_DEVICE_ERROR       Failed to discover (read) capabilities
   @retval   EFI_DEVICE_ERROR       Failed to read MAC address
   @retval   EFI_ACCESS_DENIED      UNDI is not enabled
**/
EFI_STATUS
I40eFirstTimeInit (
  IN DRIVER_DATA *AdapterInfo
  )
{
  EFI_STATUS             Status;
  enum i40e_status_code  I40eStatus;

  PCI_CONFIG_HEADER     *PciConfigHeader;
  UINT8                  PciBarConfiguration;
  BOOLEAN                Pci64Bit;

  UINT32                 Reg;
  struct i40e_hw        *Hw;

  Hw = &AdapterInfo->Hw;
  Hw->back = AdapterInfo;

  AdapterInfo->DriverBusy = FALSE;
  AdapterInfo->LastMediaStatus = FALSE;
  AdapterInfo->RepeatedFwResets = FALSE;
  AdapterInfo->MediaStatusChecked = FALSE;
  AdapterInfo->FwSupported = TRUE;
  AdapterInfo->RecoveryMode = IsRecoveryMode (AdapterInfo);

  PciConfigHeader     = (PCI_CONFIG_HEADER *) &AdapterInfo->PciConfig[0];
  PciBarConfiguration = PciConfigHeader->BaseAddressReg0 & PCI_BAR_MEM_MASK;
  Pci64Bit            = (PciBarConfiguration & 0x6) == PCI_BAR_MEM_64BIT;

  if (Pci64Bit) {
    // On 64-bit BAR, device address claims two slots in the PCI header.
    Hw->hw_addr = (UINT8 *) (UINTN) ((UINT64) (PciConfigHeader->BaseAddressReg0 & PCI_BAR_MEM_BASE_ADDR_M) +
                                    (((UINT64) PciConfigHeader->BaseAddressReg1) << 32));
    DEBUGPRINT (INIT, ("PCI Base Address Register (64-bit) = %8X:%8X\n",
                       PciConfigHeader->BaseAddressReg1,
                       PciConfigHeader->BaseAddressReg0));
  } else {
    Hw->hw_addr = (UINT8 *) (UINTN) (PciConfigHeader->BaseAddressReg0 & PCI_BAR_MEM_BASE_ADDR_M);
    DEBUGPRINT (INIT, ("PCI Base Address Register (32-bit) = %8X\n", PciConfigHeader->BaseAddressReg0));
  }

  if (Hw->hw_addr == NULL) {
    DEBUGPRINT (CRITICAL, ("NIC Hardware Address is NULL - expect issues!\n"));
    DEBUGPRINT (CRITICAL, ("Basic networking should work, advanced features will fail.\n"));
    DEBUGWAIT (CRITICAL);
  }

  DEBUGPRINT (INIT, ("PCI Command Register = %X\n", PciConfigHeader->Command));
  DEBUGPRINT (INIT, ("PCI Status Register = %X\n", PciConfigHeader->Status));
  DEBUGPRINT (INIT, ("PCI VendorID = %X\n", PciConfigHeader->VendorId));
  DEBUGPRINT (INIT, ("PCI DeviceID = %X\n", PciConfigHeader->DeviceId));
  DEBUGPRINT (INIT, ("PCI SubVendorID = %X\n", PciConfigHeader->SubVendorId));
  DEBUGPRINT (INIT, ("PCI SubSystemID = %X\n", PciConfigHeader->SubSystemId));

  // Initialize all parameters needed for the shared code
  Hw->vendor_id              = PciConfigHeader->VendorId;
  Hw->device_id              = PciConfigHeader->DeviceId;
  Hw->subsystem_vendor_id    = PciConfigHeader->SubVendorId;
  Hw->subsystem_device_id    = PciConfigHeader->SubSystemId;
  Hw->revision_id            = PciConfigHeader->RevId;

  Hw->adapter_stopped        = TRUE;

  AdapterInfo->PciClass       = PciConfigHeader->ClassIdMain;
  AdapterInfo->PciSubClass    = PciConfigHeader->ClassIdSubclass;
  AdapterInfo->PciClassProgIf = PciConfigHeader->ClassIdProgIf;

  if (Hw->subsystem_device_id == 0) {
    // Read Subsystem ID from PFPCI_SUBSYSID
    Hw->subsystem_device_id = (UINT16) (rd32 (Hw, 0x000BE100) & 0xFFFF);
  }


  if (AdapterInfo->RecoveryMode) {
    // Firmware is in recovery mode. Refrain from further initialization
    // and report error status thru the Driver Health Protocol
    DEBUGPRINT (CRITICAL, ("FW is in recovery mode, skip further initialization\n"));
    AdapterInfo->FwSupported = FALSE;
    return EFI_UNSUPPORTED;
  }

  Hw->bus.device = (UINT16) AdapterInfo->Device;
  Hw->bus.func   = (UINT16) AdapterInfo->Function;

  // DEBUGPRINT (INIT, ("PCI Segment = %X\n", AdapterInfo->Segment));
  DEBUGPRINT (INIT, ("PCI Bus = %X\n", AdapterInfo->Bus));
  DEBUGPRINT (INIT, ("PCI Device = %X\n", AdapterInfo->Device));
  DEBUGPRINT (INIT, ("PCI Function = %X\n", AdapterInfo->Function));

  ZeroMem (AdapterInfo->BroadcastNodeAddress, PXE_MAC_LENGTH);
  SetMem (AdapterInfo->BroadcastNodeAddress, PXE_HWADDR_LEN_ETHER, 0xFF);
  ZeroMem (&AdapterInfo->Vsi.CurrentMcastList, sizeof (AdapterInfo->Vsi.CurrentMcastList));

  // Find out if this function is already used by legacy component
  AdapterInfo->UndiEnabled = I40eAquireControllerHw (AdapterInfo);
  DEBUGPRINT (INIT, ("I40eAquireControllerHw returned %d\n", AdapterInfo->UndiEnabled));

  // Setup AQ initialization parameters: 32 descriptor rings, 4kB buffers
  Hw->aq.num_arq_entries = 32;
  Hw->aq.num_asq_entries = 32;
  Hw->aq.arq_buf_size = 4096;
  Hw->aq.asq_buf_size = 4096;

  I40eStatus = i40e_init_shared_code (Hw);
  if (I40eStatus != I40E_SUCCESS) {
    DEBUGPRINT (CRITICAL, ("i40e_init_shared_code returned %d\n", I40eStatus));
    Status = EFI_DEVICE_ERROR;
    goto ErrorReleaseController;
  }

  DEBUGPRINT (INIT, ("Initializing PF %d, PCI Func %d\n", Hw->pf_id, Hw->bus.func));

  if (AdapterInfo->UndiEnabled) {
    i40e_clear_hw (Hw);

    if (AdapterInfo->Hw.mac.type == I40E_MAC_X722) {
      Reg = I40eRead32 (AdapterInfo, I40E_PRTDCB_TC2PFC_RCB);
      I40eWrite32 (AdapterInfo, I40E_PRTDCB_TC2PFC_RCB, 0);
    }
    I40eStatus = i40e_pf_reset (Hw);

    if (AdapterInfo->Hw.mac.type == I40E_MAC_X722) {
      I40eWrite32 (AdapterInfo, I40E_PRTDCB_TC2PFC_RCB, Reg);
    }

    if (I40eStatus != I40E_SUCCESS) {

      // Check for repeated FW resets.
      Reg = I40eRead32 (AdapterInfo, I40E_GL_FWSTS);
      Reg &= I40E_GL_FWSTS_FWS1B_MASK;
      if (Reg > I40E_GL_FWSTS_FWS1B_EMPR_0 &&
          Reg <= I40E_GL_FWSTS_FWS1B_EMPR_10)
      {
        DEBUGPRINT (CRITICAL, ("Initialization failure due to repeated FW resets %x.\n", Reg));
        AdapterInfo->FwSupported = FALSE;
        AdapterInfo->RepeatedFwResets = TRUE;
        Status = EFI_UNSUPPORTED;
      } else {
        Status = EFI_DEVICE_ERROR;
      }
      DEBUGPRINT (CRITICAL, ("i40e_pf_reset failed %d\n", I40eStatus));
      goto ErrorReleaseController;
    }
  }

  I40eStatus = i40e_init_adminq (Hw);
  if (I40eStatus == I40E_ERR_FIRMWARE_API_VERSION) {

    // Firmware version is newer then expected. Refrain from further initialization
    // and report error status thru the Driver Health Protocol
    AdapterInfo->FwSupported = FALSE;
    return EFI_UNSUPPORTED;
  }
  if (I40eStatus != I40E_SUCCESS) {
    DEBUGPRINT (CRITICAL, ("i40e_init_adminq returned %d\n", I40eStatus));
    Status = EFI_DEVICE_ERROR;
    goto ErrorReleaseController;
  }

  DEBUGPRINT (INIT, ("FW API Info: api_maj_ver: %x, api_min_ver: %x\n", Hw->aq.api_maj_ver, Hw->aq.api_min_ver));

  Status = I40eDiscoverCapabilities (AdapterInfo);
  if (EFI_ERROR (Status)) {
    DEBUGPRINT (CRITICAL, ("I40eDiscoverCapabilities failed: %r\n", Status));
    goto ErrorReleaseController;
  }

  AdapterInfo->TxRxDescriptorCount = I40eGetTxRxDescriptorsCount (Hw);
  DEBUGPRINT (INIT, ("I40eGetTxRxDescriptorsCount: %d\n", AdapterInfo->TxRxDescriptorCount));

  gBS->Stall (100 * 1000);

  // Determine existing PF/Port configuration
  // This is to correctly match partitions, pfs and ports
  Status = ReadMfpConfiguration (AdapterInfo);
  if (EFI_ERROR (Status)) {
    DEBUGPRINT (CRITICAL, ("ReadMfpConfiguration returned %r\n", Status));
    goto ErrorReleaseController;
  }

  // Read MAC address.
  Status = I40eReadMacAddress (AdapterInfo);
  if (EFI_ERROR (Status)) {
    DEBUGPRINT (CRITICAL, ("I40eReadMacAddress failed with %r\n", Status));
    goto ErrorReleaseController;
  }

  if (AdapterInfo->UndiEnabled) {
    return EFI_SUCCESS;
  } else {
    return EFI_ACCESS_DENIED;
  }

ErrorReleaseController:
  I40eReleaseControllerHw (AdapterInfo);
  return Status;
}


/** This function calls the MemIo callback to read a dword from the device's
   address space

   @param[in]   AdapterInfo   Pointer to the NIC data structure information
                             the UNDI driver is layering on
   @param[in]   Port         Address to read from

   @return      The data read from the port.
**/
UINT32
I40eRead32 (
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
I40eWrite32 (
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
   @param[out]  Mem   ptr to mem struct to fill out
   @param[in]   size  size of memory requested

   @retval   I40E_SUCCESS        Memory allocated successfully
   @retval   I40E_ERR_NO_MEMORY  Failed to allocate memory
**/
enum i40e_status_code
I40eAllocateMem (
  struct i40e_hw          *Hw,
  struct i40e_virt_mem    *Mem,
  UINT32                   Size
  )
{
  if (Mem == NULL) {
    return I40E_ERR_PARAM;
  }

  Mem->size = Size;
  Mem->va = AllocateZeroPool (Size);
  if (Mem->va == NULL) {
    DEBUGPRINT (CRITICAL, ("Error: Requested: %d, Allocated size: %d\n", Size, Mem->size));
    return I40E_ERR_NO_MEMORY;
  }

  return I40E_SUCCESS;
}

/** OS specific memory free for shared code

   @param[in]   Hw    pointer to the HW structure
   @param[in]   Mem   ptr to mem struct to free

   @retval   I40E_SUCCESS    There is nothing to free
   @retval   I40E_SUCCESS    Memory successfully freed
   @retval   I40E_ERR_PARAM  Mem is NULL
**/
enum i40e_status_code
I40eFreeMem (
  struct i40e_hw       *Hw,
  struct i40e_virt_mem *Mem
  )
{
  if (Mem == NULL) {
    return I40E_ERR_PARAM;
  }

  if (Mem->va == NULL) {
    // Nothing to free
    return I40E_SUCCESS;
  }

  if (!mExitBootServicesTriggered) {
    gBS->FreePool ((VOID *) Mem->va);
  }

  // Always return I40E_SUCCESS, no need for enhanced error handling here
  return I40E_SUCCESS;
}

/** OS specific spinlock init for shared code

   @param[in]   Sp   pointer to a spinlock declared in driver space

   @return    Spinlock pointed by Sp is initialized
**/
VOID
I40eInitSpinLock (struct i40e_spinlock *Sp)
{
  EfiInitializeLock (&Sp->SpinLock, TPL_NOTIFY);
}

/** OS specific spinlock acquire for shared code

   @param[in]   Sp   pointer to a spinlock declared in driver space

   @return    Spinlock pointed by Sp is acquired
**/
VOID
I40eAcquireSpinLock (struct i40e_spinlock *Sp)
{
  EfiAcquireLockOrFail (&Sp->SpinLock);
}

/** OS specific spinlock release for shared code

   @param[in]   Sp   pointer to a spinlock declared in driver space

   @return    Spinlock pointed by Sp is released
**/
VOID
I40eReleaseSpinLock (struct i40e_spinlock *Sp)
{
  EfiReleaseLock (&Sp->SpinLock);
}

/** OS specific spinlock destroy for shared code

   @param[in]   Sp   pointer to a spinlock declared in driver space

   @return    Nothing is done
**/
VOID
I40eDestroySpinLock (struct i40e_spinlock *Sp)
{
}

/** OS specific DMA memory alloc for shared code

   @param[in]   Hw         pointer to the HW structure
   @param[out]  Mem        ptr to mem struct to fill out
   @param[in]   Size       size of memory requested
   @param[in]   Alignment  byte boundary to which we must align

   @retval   I40E_SUCCESS        Memory allocated successfully
   @retval   I40E_ERR_NO_MEMORY  Failed to allocate memory
**/
enum i40e_status_code
I40eAllocateDmaMem (
  IN  struct i40e_hw      *Hw,
  OUT struct i40e_dma_mem *Mem,
  IN  UINT64               Size,
  IN  UINT32               Alignment
  )
{
  EFI_STATUS   Status;
  DRIVER_DATA *AdapterInfo = (DRIVER_DATA *) Hw->back;

  if (Mem == NULL) {
    return I40E_ERR_PARAM;
  }

  Mem->Mapping.Size = (UINT32) ALIGN (Size, Alignment);

  Status = UndiDmaAllocateCommonBuffer (AdapterInfo->PciIo, &Mem->Mapping);

  Mem->va     = (VOID*) Mem->Mapping.UnmappedAddress;
  Mem->pa     = Mem->Mapping.PhysicalAddress;
  Mem->size   = Mem->Mapping.Size;

  if ((Mem->va != NULL)
    && (Status == EFI_SUCCESS))
  {
    return I40E_SUCCESS;
  } else {
    DEBUGPRINT (
      CRITICAL, ("Error: Requested: %d, Allocated size: %d\n",
      Size, Mem->Mapping.Size)
    );
    return I40E_ERR_NO_MEMORY;
  }
}

/** OS specific DMA memory free for shared code

   @param[in]   Hw         pointer to the HW structure
   @param[out]  Mem        ptr to mem struct to free

   @retval  I40E_SUCCESS     Memory successfully freed
   @retval  I40E_ERR_BAD_PTR Failed to free buffer
   @retval  I40E_ERR_PARAM   Mem is NULL
**/
enum i40e_status_code
I40eFreeDmaMem (
  struct i40e_hw *     Hw,
  struct i40e_dma_mem *Mem
  )
{
  EFI_STATUS   Status;
  DRIVER_DATA *AdapterInfo = (DRIVER_DATA *) Hw->back;

  if (NULL == Mem) {
    return I40E_ERR_PARAM;
  }

  // Free memory allocated for transmit and receive resources.
  Status = UndiDmaFreeCommonBuffer (AdapterInfo->PciIo, &Mem->Mapping);

  if (EFI_ERROR (Status)) {
    DEBUGPRINT (CRITICAL, ("Failed to free I40E DMA buffer: %r\n", Status));
    return I40E_ERR_BAD_PTR;
  }
  return I40E_SUCCESS;
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
  UINT32  RegVal;

  RegVal = I40eRead32 (AdapterInfo, I40E_GL_FWSTS);
  RegVal &= I40E_GL_FWSTS_FWS1B_MASK;
  RegVal >>= I40E_GL_FWSTS_FWS1B_SHIFT;

  if ((RegVal == I40E_FW_RECOVERY_MODE_CORER)
      || (RegVal == I40E_FW_RECOVERY_MODE_CORER_LEGACY)
      || (RegVal == I40E_FW_RECOVERY_MODE_GLOBR)
      || (RegVal == I40E_FW_RECOVERY_MODE_GLOBR_LEGACY)
      || (RegVal == I40E_FW_RECOVERY_MODE_TRANSITION)
      || (RegVal == I40E_FW_RECOVERY_MODE_NVM))
  {
    return TRUE;
  }

  return FALSE;
}

/** Updates link information in family specific Hw structure.

   @param[in]   AdapterInfo  Pointer to the NIC data structure information which
                             the UNDI driver is layering on

   @retval   EFI_SUCCESS        Link information updated successfully
   @retval   EFI_DEVICE_ERROR   get_link_info AQ cmd failed
**/
EFI_STATUS
UpdateLinkInfo (
  IN DRIVER_DATA *AdapterInfo
  )
{
  enum i40e_status_code I40eStatus;

  ASSERT (AdapterInfo != NULL);

  I40eStatus = i40e_aq_get_link_info (&AdapterInfo->Hw, TRUE, NULL, NULL);
  return (I40eStatus == I40E_SUCCESS) ? EFI_SUCCESS : EFI_DEVICE_ERROR;
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
  EFI_STATUS Status;

  Status = UpdateLinkInfo (&UndiPrivateData->NicInfo);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  *LinkUp = UndiPrivateData->NicInfo.Hw.phy.link_info.link_info & I40E_AQ_LINK_UP;

  return EFI_SUCCESS;
}

/** Gets link speed capability

   @param[in]    AdapterInfo        Pointer to the NIC data structure information which
                                    the UNDI driver is layering on
   @param[out]   AllowedLinkSpeeds  Pointer to resulting link spedd capability

   @retval    EFI_SUCCESS       Link speed capability setting successfully read
   @retval    EFI_DEVICE_ERROR  Get phy capabilities AQ cmd failed
**/
EFI_STATUS
GetLinkSpeedCapability (
  IN  DRIVER_DATA *AdapterInfo,
  OUT UINT8       *AllowedLinkSpeeds
  )
{
  enum i40e_status_code                 I40eStatus;
  struct i40e_aq_get_phy_abilities_resp PhyAbilites;

  ZeroMem (&PhyAbilites, sizeof (PhyAbilites));

  // Use AQ command to retrieve phy capabilities
  I40eStatus = i40e_aq_get_phy_capabilities (
                 &AdapterInfo->Hw,
                 FALSE,
                 FALSE,
                 &PhyAbilites,
                 NULL
               );

  if (I40eStatus != I40E_SUCCESS) {
    return EFI_DEVICE_ERROR;
  }

  *AllowedLinkSpeeds = PhyAbilites.link_speed;

  return EFI_SUCCESS;
}

/** Gets link speed setting for adapter.

   @param[in]   UndiPrivateData   Pointer to driver private data structure
   @param[out]  LinkSpeed         Link speed setting

   @retval      EFI_SUCCESS       Successful operation
**/
EFI_STATUS
GetLinkSpeed (
  IN   UNDI_PRIVATE_DATA  *UndiPrivateData,
  OUT  UINT8              *LinkSpeed
  )
{
  // Speed settings are currently not supported for 40 Gig driver. It's always set to autoneg to
  // allow operation with the highest possible speed
  *LinkSpeed = LINK_SPEED_AUTO_NEG;
  return EFI_SUCCESS;
}

/** Sets link speed setting for adapter (unsupported).

   @param[in]   UndiPrivateData  Pointer to driver private data structure
   @param[in]   LinkSpeed        Lan speed setting - unused

   @retval      EFI_SUCCESS      Successful operation
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

   @retval      EFI_SUCCESS         Successful operation
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

   @retval      EFI_SUCCESS          Successful operation
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
   @retval  EFI_DEVICE_ERROR  Failed to blink PHY link LED
**/
EFI_STATUS
BlinkLeds (
  IN  UNDI_PRIVATE_DATA  *UndiPrivateData,
  IN  UINT16             *Time
  )
{
  UINT32                 LedState;
  enum i40e_status_code  I40eStatus;
  DRIVER_DATA            *AdapterInfo = &UndiPrivateData->NicInfo;

  if (AdapterInfo->Hw.device_id == I40E_DEV_ID_10G_BASE_T
#ifdef X722_SUPPORT
    || AdapterInfo->Hw.device_id == I40E_DEV_ID_10G_BASE_T_X722
#endif /* X722_SUPPORT */
    || AdapterInfo->Hw.device_id == I40E_DEV_ID_10G_BASE_T4)
  {
    I40eStatus = i40e_blink_phy_link_led (&AdapterInfo->Hw, *Time, 500);
    IF_SCERR_RETURN (I40eStatus, EFI_DEVICE_ERROR);
  } else {

    // Get current LED state
    LedState = i40e_led_get (&AdapterInfo->Hw);

    // Turn on blinking LEDs and wait required ammount of time
    i40e_led_set (&AdapterInfo->Hw, 0xF, TRUE);
    DelayInMicroseconds (AdapterInfo, *Time * 1000 * 1000);

    // Restore LED state
    i40e_led_set (&AdapterInfo->Hw, LedState, FALSE);
  }
  return EFI_SUCCESS;
}

/** This functions initialize table containing PF numbers for partitions
   related to this port.

   @param[in]   AdapterInfo   Pointer to the NIC data structure information
                              which the UNDI driver is layering on..

   @retval      EFI_SUCCESS       PF/partition relations saved successfully.
   @retval      EFI_NOT_FOUND     EepromGetMaxPfPerPortNumber failed
   @retval      EFI_DEVICE_ERROR  EepromGetMaxPfPerPortNumber failed
**/
EFI_STATUS
ReadMfpConfiguration (
  IN DRIVER_DATA *AdapterInfo
  )
{
  UINT32     i;
  UINT32     PortNumber;
  UINT8      PartitionCount = 0;
  UINT32     PortNum;
  UINT32     FunctionStatus;
  UINT16     PortNumValues[16];
  EFI_STATUS Status;

  Status = GetMaxPfPerPortNumber (
             AdapterInfo,
             &AdapterInfo->PfPerPortMaxNumber
           );
  if (EFI_ERROR (Status)) {
    DEBUGPRINT (CRITICAL, ("EepromGetMaxPfPerPortNumber returned %r\n", Status));
    return Status;
  }

  // Read port number for current PF and save its value
  PortNumber = I40eRead32 (
                 AdapterInfo,
                 I40E_PFGEN_PORTNUM
               );
  PortNumber &= I40E_PFGEN_PORTNUM_PORT_NUM_MASK;

  AdapterInfo->PhysicalPortNumber = PortNumber;

  DEBUGPRINT (INIT, ("PhysicalPortNumber: %d\n", PortNumber));

  EepromReadPortnumValues (AdapterInfo, &PortNumValues[0], 16);

  // Run through all pfs and collect information on pfs (partitions) associated
  // to the same port as our base partition
  for (i = 0; i < I40E_MAX_PF_NUMBER; i++) {
    PortNum = PortNumValues[i];

    PortNum &= I40E_PFGEN_PORTNUM_PORT_NUM_MASK;

    if (AdapterInfo->Hw.func_caps.valid_functions & (1 << i)) {
      FunctionStatus = 1;
    } else {
      FunctionStatus = 0;
    }

    if (PortNum == PortNumber) {

      // Partition is connected to the same port as the base partition
      // Save information on the status of this partition
      if (FunctionStatus != 0) {
        AdapterInfo->PartitionEnabled[PartitionCount] = TRUE;
      } else {
        AdapterInfo->PartitionEnabled[PartitionCount] = FALSE;
      }

      // Save PCI function number
      AdapterInfo->PartitionPfNumber[PartitionCount] = i;
      DEBUGPRINT (
        INIT, ("Partition %d, PF:%d, enabled:%d\n",
        PartitionCount,
        AdapterInfo->PartitionPfNumber[PartitionCount],
        AdapterInfo->PartitionEnabled[PartitionCount])
      );

      PartitionCount++;
    }
  }
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
I40eGetVsiParams (
  IN  DRIVER_DATA             *AdapterInfo,
  OUT struct i40e_vsi_context *VsiCtx
  )
{
  EFI_STATUS I40eStatus;

  ZeroMem (VsiCtx, sizeof (struct i40e_vsi_context));

  VsiCtx->seid = AdapterInfo->MainVsiSeid;
  VsiCtx->pf_num = AdapterInfo->Hw.pf_id;
  VsiCtx->uplink_seid = AdapterInfo->MacSeid;
  VsiCtx->vf_num = 0;

  I40eStatus = i40e_aq_get_vsi_params (&AdapterInfo->Hw, VsiCtx, NULL);
  if (I40eStatus != I40E_SUCCESS) {
    DEBUGPRINT (
      CRITICAL, ("i40e_aq_get_vsi_params returned %d, aq_err %d\n",
      I40eStatus, AdapterInfo->Hw.aq.asq_last_status)
    );
    return EFI_DEVICE_ERROR;
  }
  return EFI_SUCCESS;
}


/** Gets Max Speed of ethernet port in bits.

   @param[in]   UndiPrivateData   Points to the driver instance private data
   @param[out]  MaxSpeed          Resultant value

   @retval      EFI_SUCCESS       Operation successful.
**/
EFI_STATUS
GetMaxSpeed (
  IN  UNDI_PRIVATE_DATA  *UndiPrivateData,
  OUT UINT64             *MaxSpeed
  )
{
  UINT16 DeviceId = UndiPrivateData->NicInfo.Hw.device_id;

  if (i40e_is_40G_device (DeviceId)) {
    *MaxSpeed = GIGABITS (40);
  } else if (i40e_is_25G_device (DeviceId)) {
    *MaxSpeed = GIGABITS (25);
  } else {
    switch (DeviceId) {
      case I40E_DEV_ID_5G_BASE_T_BC:
        *MaxSpeed = GIGABITS (5);
        break;
      case I40E_DEV_ID_1G_BASE_T_BC:
      case I40E_DEV_ID_1G_BASE_T_X722:
        *MaxSpeed = GIGABITS (1);
        break;
      default:
        *MaxSpeed = GIGABITS (10); // Return 10G for all others
        break;
    }
  }
  return EFI_SUCCESS;
}




/** Get supported Tx/Rx descriptor count for a given device

   @param[in]    Hw         Pointer to the HW Structure

   @return       Supported Tx/RX descriptors count
**/
UINT16
I40eGetTxRxDescriptorsCount (
  IN struct i40e_hw *Hw
  )
{
  // I40eDiscoverCapabilities must be called prior to this function.

  if (Hw->num_ports != 0 && Hw->num_partitions != 0) {
    return I40E_TOTAL_NUM_TX_RX_DESCRIPTORS / (Hw->num_ports * Hw->num_partitions);
  } else {
    return I40E_DEF_NUM_TX_RX_DESCRIPTORS;
  }
}

/** Checks if Firmware is in lockdown state.

   @param[in]   AdapterInfo  Pointer to the NIC data structure information which
                             the UNDI driver is layering on

   @retval   TRUE   Firmware is in lockdown state
   @retval   FALSE  Firmware is not in lockdown state
   @retval   FALSE  Invalid input parameter
**/
BOOLEAN
IsFwLockdownState (
  IN DRIVER_DATA *AdapterInfo
  )
{
  if (AdapterInfo == NULL) {
    return FALSE;
  }

  if (AdapterInfo->Hw.dev_caps.fw_lockdown_support) {
    //Check for FW Lockdown Status
    return AdapterInfo->Hw.dev_caps.fw_lockdown_status;
  }

  return FALSE;
}

/** Gets HII formset help string ID.

   @param[in]   UndiPrivateData  Pointer to driver private data structure

   @return   EFI_STRING_ID of formset help string, or 0 on failure
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
    case GIGABITS (40):
      return STRING_TOKEN (STR_INV_FORM_SET_40_HELP);
    case GIGABITS (25):
      return STRING_TOKEN (STR_INV_FORM_SET_25_HELP);
    case GIGABITS (10):
      return STRING_TOKEN (STR_INV_FORM_SET_10_HELP);
    case GIGABITS (5):
      return STRING_TOKEN (STR_INV_FORM_SET_5_HELP);
    case GIGABITS (1):
      return STRING_TOKEN (STR_INV_FORM_SET_1_HELP);
    default:
      return 0;
  }
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
  case I40E_DEV_ID_KX_B:
  case I40E_DEV_ID_QSFP_A:
  case I40E_DEV_ID_QSFP_B:
  case I40E_DEV_ID_QSFP_C:
  case I40E_DEV_ID_20G_KR2:
  case I40E_DEV_ID_20G_KR2_A:
    UnicodeSPrint (ChipTypeStr, HII_MAX_STR_LEN_BYTES, L"Intel XL710");
    break;
  case I40E_DEV_ID_25G_B:
  case I40E_DEV_ID_25G_SFP28:
    UnicodeSPrint (ChipTypeStr, HII_MAX_STR_LEN_BYTES, L"Intel XXV710");
    break;
  case I40E_DEV_ID_SFP_XL710:
  case I40E_DEV_ID_KX_C:
  case I40E_DEV_ID_10G_BASE_T:
  case I40E_DEV_ID_10G_BASE_T4:
  case I40E_DEV_ID_10G_BASE_T_BC:
  case I40E_DEV_ID_10G_B:
  case I40E_DEV_ID_10G_SFP:
    UnicodeSPrint (ChipTypeStr, HII_MAX_STR_LEN_BYTES, L"Intel X710");
    break;
  case I40E_DEV_ID_5G_BASE_T_BC:
    UnicodeSPrint (ChipTypeStr, HII_MAX_STR_LEN_BYTES, L"Intel V710");
    break;
  case I40E_DEV_ID_1G_BASE_T_BC:
    UnicodeSPrint (ChipTypeStr, HII_MAX_STR_LEN_BYTES, L"Intel I710");
    break;
  case I40E_DEV_ID_KX_X722:
  case I40E_DEV_ID_QSFP_X722:
  case I40E_DEV_ID_SFP_X722:
  case I40E_DEV_ID_1G_BASE_T_X722:
  case I40E_DEV_ID_10G_BASE_T_X722:
  case I40E_DEV_ID_SFP_I_X722:
  case I40E_DEV_ID_SFP_X722_A:
    UnicodeSPrint (ChipTypeStr, HII_MAX_STR_LEN_BYTES, L"Intel X722");
    break;
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

