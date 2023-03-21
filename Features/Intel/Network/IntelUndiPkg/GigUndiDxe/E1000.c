/**************************************************************************

Copyright (c) 2012 - 2021, Intel Corporation. All rights reserved.

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
#include "E1000.h"
#include "DeviceSupport.h"

#include <Library/HiiLib.h>
#include "Hii/Hii.h"



/* Global variables for blocking IO */
STATIC BOOLEAN  mInitializeLock = TRUE;
STATIC EFI_LOCK mLock;

EFI_TIME gTime;





/** Checks if alternate MAC address is supported.

   @param[in]   UndiPrivateData    Driver instance private data structure
   @param[out]  AltMacSupport      Tells if alternate MAC address is supported

   @retval   EFI_SUCCESS    Alternate MAC address is always supported
**/
EFI_STATUS
GetAltMacAddressSupport (
  IN   UNDI_PRIVATE_DATA  *UndiPrivateData,
  OUT  BOOLEAN            *AltMacSupport
  )
{
  UINT16 BackupMacPointer;

  // Check to see if the backup MAC address location pointer is set
  e1000_read_nvm (&UndiPrivateData->NicInfo.Hw, NVM_ALT_MAC_ADDR_PTR, 1, &BackupMacPointer);
  //  Alternate Mac Address not supported if 0x37 pointer is not initialized to a value
  //  other than 0x0000 or 0xffff
  *AltMacSupport = ((BackupMacPointer != 0xFFFF) && (BackupMacPointer != 0x0000));
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
  *MaxSpeed = GIGABITS (1); // 1G available only
  return EFI_SUCCESS;
}



/** Wait for up to 15 seconds for two pair downshift to complete

   @param[in]   AdapterInfo   Pointer to the NIC data structure information
                             which the UNDI driver is layering on..

   @retval   TRUE    Two pair downshift was successful and link was established
   @retval   FALSE   Otherwise
**/
BOOLEAN
E1000DownShift (
  IN DRIVER_DATA *AdapterInfo
  )
{
  UINTN  i;
  UINT32 Status;

  DEBUGPRINT (E1000, ("E1000DownShift: Attempting downshift\n"));

  i = 0;
  for (i = 0; i < 15; i++) {
    DELAY_IN_MILLISECONDS (1000);
    Status = E1000_READ_REG (&AdapterInfo->Hw, E1000_STATUS);
    DEBUGPRINT (E1000, ("Status = %x\n", Status));
    if ((Status & E1000_STATUS_LU) != 0) {
      DEBUGPRINT (E1000, ("Successfully established link\n"));
      return TRUE;
    }
  }

  return FALSE;
}

/** Copies the stats from our local storage to the protocol storage.

   It means it will read our read and clear numbers, so some adding is required before
   we copy it over to the protocol.

   @param[in]   AdapterInfo   Pointer to the NIC data structure information
                             which the UNDI driver is layering on..
   @param[in]   DbAddr   The data Block address
   @param[in]   DbSize   The data Block size

   @retval   PXE_STATCODE_SUCCESS  Statistics copied successfully
**/
UINTN
E1000Statistics (
  IN DRIVER_DATA *AdapterInfo,
  IN UINT64       DbAddr,
  IN UINT16       DbSize
  )
{
  PXE_DB_STATISTICS *    DbPtr;
  struct e1000_hw *      Hw;
  struct e1000_hw_stats *St;
  UINTN                  Stat;

  Hw  = &AdapterInfo->Hw;
  St  = &AdapterInfo->Stats;

  {
    UPDATE_OR_RESET_STAT (crcerrs, E1000_CRCERRS);
  }

  UPDATE_OR_RESET_STAT (gprc, E1000_GPRC);
  UPDATE_OR_RESET_STAT (bprc, E1000_BPRC);
  UPDATE_OR_RESET_STAT (mprc, E1000_MPRC);
  UPDATE_OR_RESET_STAT (roc, E1000_ROC);
  UPDATE_OR_RESET_STAT (prc64, E1000_PRC64);
  UPDATE_OR_RESET_STAT (prc127, E1000_PRC127);
  UPDATE_OR_RESET_STAT (prc255, E1000_PRC255);
  UPDATE_OR_RESET_STAT (prc511, E1000_PRC511);
  UPDATE_OR_RESET_STAT (prc1023, E1000_PRC1023);
  UPDATE_OR_RESET_STAT (prc1522, E1000_PRC1522);

  UPDATE_OR_RESET_STAT (symerrs, E1000_SYMERRS);
  UPDATE_OR_RESET_STAT (mpc, E1000_MPC);
  UPDATE_OR_RESET_STAT (scc, E1000_SCC);
  UPDATE_OR_RESET_STAT (ecol, E1000_ECOL);
  UPDATE_OR_RESET_STAT (mcc, E1000_MCC);
  UPDATE_OR_RESET_STAT (latecol, E1000_LATECOL);
  UPDATE_OR_RESET_STAT (dc, E1000_DC);
  UPDATE_OR_RESET_STAT (sec, E1000_SEC);
  UPDATE_OR_RESET_STAT (rlec, E1000_RLEC);
  UPDATE_OR_RESET_STAT (xonrxc, E1000_XONRXC);
  UPDATE_OR_RESET_STAT (xontxc, E1000_XONTXC);
  UPDATE_OR_RESET_STAT (xoffrxc, E1000_XOFFRXC);
  UPDATE_OR_RESET_STAT (xofftxc, E1000_XOFFTXC);
  UPDATE_OR_RESET_STAT (fcruc, E1000_FCRUC);
  UPDATE_OR_RESET_STAT (gptc, E1000_GPTC);
  UPDATE_OR_RESET_STAT (rnbc, E1000_RNBC);
  UPDATE_OR_RESET_STAT (ruc, E1000_RUC);
  UPDATE_OR_RESET_STAT (rfc, E1000_RFC);
  UPDATE_OR_RESET_STAT (rjc, E1000_RJC);
  UPDATE_OR_RESET_STAT (tpr, E1000_TPR);
  UPDATE_OR_RESET_STAT (ptc64, E1000_PTC64);
  UPDATE_OR_RESET_STAT (ptc127, E1000_PTC127);
  UPDATE_OR_RESET_STAT (ptc255, E1000_PTC255);
  UPDATE_OR_RESET_STAT (ptc511, E1000_PTC511);
  UPDATE_OR_RESET_STAT (ptc1023, E1000_PTC1023);
  UPDATE_OR_RESET_STAT (ptc1522, E1000_PTC1522);
  UPDATE_OR_RESET_STAT (mptc, E1000_MPTC);
  UPDATE_OR_RESET_STAT (bptc, E1000_BPTC);

  // used for adaptive IFS
  Hw->mac.tx_packet_delta = E1000_READ_REG (Hw, E1000_TPT);
  St->tpt                 = DbAddr ? St->tpt + Hw->mac.tx_packet_delta : 0;
  Hw->mac.collision_delta = E1000_READ_REG (Hw, E1000_COLC);
  St->colc                = DbAddr ? St->colc + Hw->mac.collision_delta : 0;

  {
    UPDATE_OR_RESET_STAT (algnerrc, E1000_ALGNERRC);
    UPDATE_OR_RESET_STAT (rxerrc, E1000_RXERRC);
    UPDATE_OR_RESET_STAT (tncrs, E1000_TNCRS);
    UPDATE_OR_RESET_STAT (cexterr, E1000_CEXTERR);
    UPDATE_OR_RESET_STAT (tsctc, E1000_TSCTC);
    UPDATE_OR_RESET_STAT (tsctfc, E1000_TSCTFC);
  }

  if (!DbAddr) {
    return PXE_STATCODE_SUCCESS;
  }

  DbPtr = (PXE_DB_STATISTICS *) (UINTN) DbAddr;

  // Fill out the OS statistics structure
  // To Add/Subtract stats, include/delete the lines in pairs.
  // E.g., adding a new stat would entail adding these two lines:
  // stat = PXE_STATISTICS_NEW_STAT_XXX;         SET_SUPPORT;
  //     DbPtr->Data[stat] = st->xxx;
  DbPtr->Supported = 0;

  {
    UPDATE_EFI_STAT (RX_TOTAL_FRAMES, tpr);
  }

  UPDATE_EFI_STAT (RX_GOOD_FRAMES, gprc);
  UPDATE_EFI_STAT (RX_UNDERSIZE_FRAMES, ruc);
  UPDATE_EFI_STAT (RX_OVERSIZE_FRAMES, roc);
  UPDATE_EFI_STAT (RX_DROPPED_FRAMES, rnbc);
  SET_SUPPORT (RX_UNICAST_FRAMES);
  DbPtr->Data[Stat] = (St->gprc - St->bprc - St->mprc);
  UPDATE_EFI_STAT (RX_BROADCAST_FRAMES, bprc);
  UPDATE_EFI_STAT (RX_MULTICAST_FRAMES, mprc);
  SET_SUPPORT (RX_CRC_ERROR_FRAMES);
  DbPtr->Data[Stat] = (St->crcerrs + St->algnerrc);
  UPDATE_EFI_STAT (TX_TOTAL_FRAMES, tpt);
  UPDATE_EFI_STAT (TX_GOOD_FRAMES, gptc);
  SET_SUPPORT (TX_UNICAST_FRAMES);
  DbPtr->Data[Stat] = (St->gptc - St->bptc - St->mptc);
  UPDATE_EFI_STAT (TX_BROADCAST_FRAMES, bptc);
  UPDATE_EFI_STAT (TX_MULTICAST_FRAMES, mptc);
  UPDATE_EFI_STAT (COLLISIONS, colc);

  return PXE_STATCODE_SUCCESS;
}

/** Takes a command Block pointer (cpb) and sends the frame.  Takes either one fragment or many
   and places them onto the wire.  Cleanup of the send happens in the function UNDI_Status in DECODE.C

   @param[in]   AdapterInfo   Pointer to the instance data
   @param[in]   Cpb       The command parameter Block address.  64 bits since this is Itanium(tm)
                          processor friendly
   @param[in]   OpFlags   The operation flags, tells if there is any special sauce on this transmit

  @retval     PXE_STATCODE_SUCCESS          Packet enqueued for transmit.
  @retval     PXE_STATCODE_DEVICE_FAILURE   AdapterInfo parameter is NULL.
  @retval     PXE_STATCODE_DEVICE_FAILURE   Failed to send packet.
  @retval     PXE_STATCODE_INVALID_CPB      CPB invalid.
  @retval     PXE_STATCODE_UNSUPPORTED      Fragmented tranmission was requested.
  @retval     PXE_STATCODE_QUEUE_FULL       Tx queue is full.
**/
UINTN
E1000Transmit (
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
      DEBUGPRINT (TX, ("Tx queue is full.\n"));
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

/** Copies the frame from our internal storage ring (As pointed to by AdapterInfo->rx_ring)
   to the command Block passed in as part of the cpb parameter.

   The flow:
   Ack the interrupt, setup the pointers, find where the last Block copied is, check to make
   sure we have actually received something, and if we have then we do a lot of work.
   The packet is checked for errors, size is adjusted to remove the CRC, adjust the amount
   to copy if the buffer is smaller than the packet, copy the packet to the EFI buffer,
   and then figure out if the packet was targetted at us, broadcast, multicast
   or if we are all promiscuous.  We then put some of the more interesting information
   (protocol, src and dest from the packet) into the db that is passed to us.
   Finally we clean up the frame, set the return value to _SUCCESS, and inc the cur_rx_ind, watching
   for wrapping.  Then with all the loose ends nicely wrapped up, fade to black and return.

   @param[in]   AdapterInfo   Pointer to the driver data
   @param[in]   Cpb           Pointer (Ia-64 friendly) to the command parameter
                              block. The frame will be placed inside of it.
   @param[out]  Db            The data buffer. The out of band method of passing
                              pre-digested information to the protocol.

  @retval     PXE_STATCODE_NO_DATA        There is no data to receive.
  @retval     PXE_STATCODE_DEVICE_FAILURE AdapterInfo is NULL.
  @retval     PXE_STATCODE_DEVICE_FAILURE Device failure on packet receive.
  @retval     PXE_STATCODE_INVALID_CPB    Invalid CPB/DB parameters.
  @retval     PXE_STATCODE_NOT_STARTED    Rx queue not started.
  @retval     PXE_STATCODE_SUCCESS        Received data passed to the protocol.
**/
UINTN
E1000Receive (
  IN  DRIVER_DATA       *AdapterInfo,
  IN  PXE_CPB_RECEIVE   *CpbReceive,
  OUT PXE_DB_RECEIVE    *DbReceive
  )
{
  PXE_STATCODE      StatCode;
  EFI_STATUS        Status;

  UINT8             *RxBuffer;
  UINT16            RxBufferSize;
  UINT16            BytesReceived;
  UINT16            PacketLength;

  ETHER_HEADER      *Header;
  PXE_FRAME_TYPE    PacketType;

  if (AdapterInfo == NULL) {
    // Should not happen
    ASSERT (AdapterInfo != NULL);
    StatCode = PXE_STATCODE_DEVICE_FAILURE;
    goto Exit;
  }

  // Acknowledge the interrupts
  E1000_READ_REG (&AdapterInfo->Hw, E1000_ICR);

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
  if (CompareMem (Header->DestAddr, AdapterInfo->Hw.mac.perm_addr, PXE_HWADDR_LEN_ETHER) == 0) {
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

/** Allows the protocol to control our interrupt behaviour.

   @param[in]   AdapterInfo   Pointer to the driver structure

   @retval   PXE_STATCODE_SUCCESS   Interrupt state set successfully
**/
UINTN
E1000SetInterruptState (
  IN DRIVER_DATA *AdapterInfo
  )
{
  UINT32 SetIntMask;

  SetIntMask = 0;

  DEBUGPRINT (E1000, ("E1000SetInterruptState\n"));

  // Start with no Interrupts.
  E1000_WRITE_REG (&AdapterInfo->Hw, E1000_IMC, 0xFFFFFFFF);

  // Mask the RX interrupts
  if (AdapterInfo->IntMask & PXE_OPFLAGS_INTERRUPT_RECEIVE) {
    SetIntMask = (E1000_ICR_RXT0 |
                  E1000_ICR_RXSEQ |
                  E1000_ICR_RXDMT0 |
                  E1000_ICR_RXO |
                  E1000_ICR_RXCFG |
                  SetIntMask);
    DEBUGPRINT (E1000, ("Mask the RX interrupts\n"));
  }

  // Mask the TX interrupts
  if (AdapterInfo->IntMask & PXE_OPFLAGS_INTERRUPT_TRANSMIT) {
    SetIntMask = (E1000_ICR_TXDW | E1000_ICR_TXQE | SetIntMask);
    DEBUGPRINT (E1000, ("Mask the TX interrupts\n"));
  }

  // Now we have all the Ints we want, so let the hardware know.
  E1000_WRITE_REG (&AdapterInfo->Hw, E1000_IMS, SetIntMask);

  return PXE_STATCODE_SUCCESS;
}

/** Stop the hardware and put it all (including the PHY) into a known good state.

   @param[in]   AdapterInfo   Pointer to the driver structure

   @retval   PXE_STATCODE_SUCCESS    Hardware stopped
**/
UINTN
E1000Shutdown (
  IN DRIVER_DATA *AdapterInfo
  )
{
  EFI_STATUS  Status;

  DEBUGPRINT (E1000, ("E1000Shutdown - adapter stop\n"));

  // Disable the transmit and receive DMA
  Status = ReceiveStop (AdapterInfo);

  switch (Status) {
  case EFI_SUCCESS:
  case EFI_NOT_STARTED:
    break;
  default:
    ASSERT_EFI_ERROR (Status);
  }

  Status = TransmitStop (AdapterInfo);

  switch (Status) {
  case EFI_SUCCESS:
  case EFI_NOT_STARTED:
    break;
  default:
    ASSERT_EFI_ERROR (Status);
  }

  // Release the software semaphore.
  E1000_WRITE_REG (&AdapterInfo->Hw, E1000_SWSM, 0);
  E1000PciFlush (&AdapterInfo->Hw);

  // This delay is to ensure in flight DMA and receive descriptor flush
  // have time to complete.
  DELAY_IN_MILLISECONDS (10);

  AdapterInfo->RxFilter = 0;

  return PXE_STATCODE_SUCCESS;
}

/** Resets the hardware and put it all (including the PHY) into a known good state.

   @param[in]   AdapterInfo   The pointer to our context data
   @param[in]   OpFlags      The information on what else we need to do.

   @retval   PXE_STATCODE_SUCCESS        Successfull hardware reset
   @retval   PXE_STATCODE_NOT_STARTED    Hardware init failed
**/
UINTN
E1000Reset (
  IN DRIVER_DATA *AdapterInfo,
  IN UINT16       OpFlags
  )
{
  UINT32 TempReg;

  DEBUGPRINT (E1000, ("E1000Reset\n"));

  TempReg = E1000_READ_REG (&AdapterInfo->Hw, E1000_STATUS);

  AdapterInfo->Hw.phy.reset_disable = TRUE;

  if ((TempReg & E1000_STATUS_LU) != 0) {
    if (((TempReg & E1000_STATUS_FD) == 0)
      && ((TempReg & E1000_STATUS_SPEED_MASK) == E1000_STATUS_SPEED_1000))
    {
      DEBUGPRINT (E1000, ("BAD LINK - 1Gig/Half - Enabling PHY reset\n"));
      AdapterInfo->Hw.phy.reset_disable = FALSE;

      // Since link is in a bad state we also need to make sure that we do a full reset down below
      AdapterInfo->HwInitialized = FALSE;
    }
  }

  // Put the E1000 into a known state by resetting the transmit
  // and receive units of the E1000 and masking/clearing all
  // interrupts.
  // If the hardware has already been started then don't bother with a reset
  // We want to make sure we do not have to restart autonegotiation and two-pair
  // downshift.
  if (!AdapterInfo->HwInitialized) {
    e1000_reset_hw (&AdapterInfo->Hw);

    // Now that the structures are in place, we can configure the hardware to use it all.
    if (e1000_init_hw (&AdapterInfo->Hw) == 0) {
      DEBUGPRINT (E1000, ("e1000_init_hw success\n"));
    } else {
      DEBUGPRINT (CRITICAL, ("Hardware Init failed\n"));
      return PXE_STATCODE_NOT_STARTED;
    }
  } else {
    DEBUGPRINT (E1000, ("Skipping adapter reset\n"));
  }

  if ((OpFlags & PXE_OPFLAGS_RESET_DISABLE_FILTERS) == 0) {
    UINT16 SaveFilter;

    SaveFilter = AdapterInfo->RxFilter;

    // if we give the filter same as Rx_Filter, this routine will not set mcast list
    // (it thinks there is no change)
    // to force it, we will reset that flag in the Rx_Filter
    AdapterInfo->RxFilter &= (~PXE_OPFLAGS_RECEIVE_FILTER_FILTERED_MULTICAST);
    E1000SetFilter (AdapterInfo, SaveFilter, (UINT64) 0, (UINT32) 0);
  }

  if (OpFlags & PXE_OPFLAGS_RESET_DISABLE_INTERRUPTS) {
    AdapterInfo->IntMask = 0; // disable the interrupts
  }

  E1000SetInterruptState (AdapterInfo);

  return PXE_STATCODE_SUCCESS;
}

/** PCIe function to LAN port mapping.

   @param[in,out]   AdapterInfo   Pointer to adapter structure

   @return   LAN function set accordingly
**/
VOID
E1000LanFunction (
  IN OUT DRIVER_DATA *AdapterInfo
  )
{
  AdapterInfo->LanFunction = (E1000_READ_REG (&AdapterInfo->Hw, E1000_STATUS) &
                            E1000_STATUS_LAN_ID_MASK)
                            >> E1000_STATUS_LAN_ID_OFFSET;
  DEBUGPRINT (INIT, ("PCI function %d is LAN port %d \n", AdapterInfo->Function, AdapterInfo->LanFunction));
  DEBUGWAIT (INIT);
}

/** Sets the force speed and duplex settings for the adapter based on
   EEPROM settings and data set by the SNP

   @param[in]   AdapterInfo   Pointer to adapter structure

   @return    Speed duplex settings set
**/
VOID
E1000SetSpeedDuplex (
  IN DRIVER_DATA *AdapterInfo
  )
{
  UINT16 SetupOffset;
  UINT16 ConfigOffset;
  UINT16 SetupWord;
  UINT16 CustomConfigWord;

  DEBUGPRINT (E1000, ("E1000SetSpeedDuplex\n"));

  // Copy forced speed and duplex settings to shared code structure
  if ((AdapterInfo->LinkSpeed == 10)
    && (AdapterInfo->DuplexMode == PXE_FORCE_HALF_DUPLEX))
  {
    DEBUGPRINT (E1000, ("Force 10-Half\n"));
    AdapterInfo->Hw.phy.reset_disable    = FALSE;
    AdapterInfo->Hw.mac.autoneg              = 0;
    AdapterInfo->Hw.mac.forced_speed_duplex  = ADVERTISE_10_HALF;
    AdapterInfo->HwInitialized = FALSE;
  }

  if ((AdapterInfo->LinkSpeed == 100)
    && (AdapterInfo->DuplexMode == PXE_FORCE_HALF_DUPLEX))
  {
    DEBUGPRINT (E1000, ("Force 100-Half\n"));
    AdapterInfo->Hw.phy.reset_disable    = FALSE;
    AdapterInfo->Hw.mac.autoneg              = 0;
    AdapterInfo->Hw.mac.forced_speed_duplex  = ADVERTISE_100_HALF;
    AdapterInfo->HwInitialized = FALSE;
  }

  if ((AdapterInfo->LinkSpeed == 10)
    && (AdapterInfo->DuplexMode == PXE_FORCE_FULL_DUPLEX))
  {
    DEBUGPRINT (E1000, ("Force 10-Full\n"));
    AdapterInfo->Hw.phy.reset_disable    = FALSE;
    AdapterInfo->Hw.mac.autoneg              = 0;
    AdapterInfo->Hw.mac.forced_speed_duplex  = ADVERTISE_10_FULL;
    AdapterInfo->HwInitialized = FALSE;
  }

  if ((AdapterInfo->LinkSpeed == 100)
    && (AdapterInfo->DuplexMode == PXE_FORCE_FULL_DUPLEX))
  {
    DEBUGPRINT (E1000, ("Force 100-Full\n"));
    AdapterInfo->Hw.phy.reset_disable    = FALSE;
    AdapterInfo->Hw.mac.autoneg              = 0;
    AdapterInfo->Hw.mac.forced_speed_duplex  = ADVERTISE_100_FULL;
    AdapterInfo->HwInitialized = FALSE;
  }

  // Check for forced speed and duplex
  // The EEPROM settings will override settings passed by the SNP

  // Configure offsets depending on function number
  switch (AdapterInfo->Function) {
  case 0:
    ConfigOffset = CONFIG_CUSTOM_WORD;
    SetupOffset  = SETUP_OPTIONS_WORD;
    break;
  case 1:
    ConfigOffset = CONFIG_CUSTOM_WORD_LANB;
    SetupOffset  = SETUP_OPTIONS_WORD_LANB;
    break;
  case 2:
    ConfigOffset = CONFIG_CUSTOM_WORD_LANC;
    SetupOffset  = SETUP_OPTIONS_WORD_LANC;
    break;
  case 3:
    ConfigOffset = CONFIG_CUSTOM_WORD_LAND;
    SetupOffset  = SETUP_OPTIONS_WORD_LAND;
    break;
  default:
    ConfigOffset = CONFIG_CUSTOM_WORD;
    SetupOffset  = SETUP_OPTIONS_WORD;
    break;
  }

  e1000_read_nvm (&AdapterInfo->Hw, SetupOffset, 1, &SetupWord);
  e1000_read_nvm (&AdapterInfo->Hw, ConfigOffset, 1, &CustomConfigWord);

  if ((CustomConfigWord & SIG_MASK) == SIG) {
    switch (SetupWord & (FSP_MASK | FDP_FULL_DUPLEX_BIT)) {
    case (FDP_FULL_DUPLEX_BIT | FSP_100MBS):
      DEBUGPRINT (E1000, ("Forcing 100 Full from EEPROM\n"));
      AdapterInfo->Hw.phy.reset_disable = FALSE;
      AdapterInfo->Hw.mac.autoneg = 0;
      AdapterInfo->Hw.mac.forced_speed_duplex = ADVERTISE_100_FULL;
      AdapterInfo->HwInitialized = FALSE;
      break;
    case (FDP_FULL_DUPLEX_BIT | FSP_10MBS):
      DEBUGPRINT (E1000, ("Forcing 10 Full from EEPROM\n"));
      AdapterInfo->Hw.phy.reset_disable = FALSE;
      AdapterInfo->Hw.mac.autoneg = 0;
      AdapterInfo->Hw.mac.forced_speed_duplex = ADVERTISE_10_FULL;
      AdapterInfo->HwInitialized = FALSE;
      break;
    case (FSP_100MBS):
      DEBUGPRINT (E1000, ("Forcing 100 Half from EEPROM\n"));
      AdapterInfo->Hw.phy.reset_disable = FALSE;
      AdapterInfo->Hw.mac.autoneg = 0;
      AdapterInfo->Hw.mac.forced_speed_duplex = ADVERTISE_100_HALF;
      AdapterInfo->HwInitialized = FALSE;
      break;
    case (FSP_10MBS):
      DEBUGPRINT (E1000, ("Forcing 10 Half from EEPROM\n"));
      AdapterInfo->Hw.phy.reset_disable = FALSE;
      AdapterInfo->Hw.mac.autoneg = 0;
      AdapterInfo->Hw.mac.forced_speed_duplex = ADVERTISE_10_HALF;
      AdapterInfo->HwInitialized = FALSE;
      break;
    default:
      AdapterInfo->Hw.mac.autoneg = 1;
      break;
    }
  }
}

/** Initializes the transmit and receive resources for the adapter.

   @param[in]   AdapterInfo   Pointer to adapter structure

   @return   TX/RX resources configured and initialized
**/
VOID
E1000TxRxConfigure (
  IN DRIVER_DATA *AdapterInfo
  )
{
  EFI_STATUS      Status;

  Status = ReceiveStop (AdapterInfo);

  switch (Status) {
  case EFI_SUCCESS:
  case EFI_NOT_STARTED:
    break;
  default:
    DEBUGPRINT (CRITICAL, ("Failed to stop Rx queue: %r\n"));
    ASSERT_EFI_ERROR (Status);
    return;
  }

  Status = TransmitStop (AdapterInfo);

  switch (Status) {
  case EFI_SUCCESS:
  case EFI_NOT_STARTED:
    break;
  default:
    DEBUGPRINT (CRITICAL, ("Failed to stop Tx queue: %r\n"));
    ASSERT_EFI_ERROR (Status);
    goto ExitStartRx;
  }

  Status = ReceiveReset (AdapterInfo);

  if (EFI_ERROR (Status)) {
    DEBUGPRINT (CRITICAL, ("Failed to reset Rx queue: %r\n"));
    ASSERT_EFI_ERROR (Status);
    goto ExitStartTx;
  }

  Status = TransmitReset (AdapterInfo);

  if (EFI_ERROR (Status)) {
    DEBUGPRINT (CRITICAL, ("Failed to reset Tx queue: %r\n"));
    ASSERT_EFI_ERROR (Status);
  }

ExitStartTx:
  Status = TransmitStart (AdapterInfo);
  ASSERT_EFI_ERROR (Status);
ExitStartRx:
  Status = ReceiveStart (AdapterInfo);
  ASSERT_EFI_ERROR (Status);
}
/** This function performs PCI-E initialization for the device.

   @param[in]   AdapterInfo   Pointer to adapter structure

   @retval   EFI_SUCCESS            PCI-E initialized successfully
   @retval   EFI_UNSUPPORTED        Failed to get supported PCI command options
   @retval   EFI_UNSUPPORTED        Failed to set PCI command options
   @retval   EFI_OUT_OF_RESOURCES   The memory pages for transmit and receive resources could
                                    not be allocated
**/
EFI_STATUS
E1000PciInit (
  IN DRIVER_DATA *AdapterInfo
  )
{
  EFI_STATUS Status;
  UINT64     Result = 0;
  BOOLEAN    PciAttributesSaved = FALSE;

  // Save original PCI attributes
  Status = AdapterInfo->PciIo->Attributes (
                                AdapterInfo->PciIo,
                                EfiPciIoAttributeOperationGet,
                                0,
                                &AdapterInfo->OriginalPciAttributes
                               );

  if (EFI_ERROR (Status)) {
    goto PciIoError;
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
                                  Result & (EFI_PCI_DEVICE_ENABLE |
                                            EFI_PCI_IO_ATTRIBUTE_DUAL_ADDRESS_CYCLE),
                                  NULL
                                );
  }

  if (EFI_ERROR (Status)) {
    DEBUGPRINT (CRITICAL, ("Attributes returns %r\n", Status));
    goto PciIoError;
  }

  return EFI_SUCCESS;

PciIoError:
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
/** This function is called as early as possible during driver start to ensure the
   hardware has enough time to autonegotiate when the real SNP device initialize call is made.

   @param[in]   AdapterInfo   Pointer to adapter structure

   @retval   EFI_SUCCESS        Hardware init success
   @retval   EFI_DEVICE_ERROR   Hardware init failed
   @retval   EFI_UNSUPPORTED    Unsupported MAC type
   @retval   EFI_UNSUPPORTED    e1000_setup_init_funcs failed
   @retval   EFI_UNSUPPORTED    Could not read bus information
   @retval   EFI_UNSUPPORTED    Could not read MAC address
   @retval   EFI_ACCESS_DENIED  iSCSI Boot detected on port
   @retval   EFI_DEVICE_ERROR   Failed to reset hardware
**/
EFI_STATUS
E1000FirstTimeInit (
  IN DRIVER_DATA *AdapterInfo
  )
{
  PCI_CONFIG_HEADER *PciConfigHeader;
  UINT32 *           TempBar;
  UINT8              BarIndex;
  EFI_STATUS         Status;
  UINT32             ScStatus;
  UINT32             Reg;
  UINT16             i;

  DEBUGPRINT (E1000, ("E1000FirstTimeInit\n"));

  AdapterInfo->DriverBusy = FALSE;

  // Read all the registers from the device's PCI Configuration space
  AdapterInfo->PciIo->Pci.Read (
                           AdapterInfo->PciIo,
                           EfiPciIoWidthUint32,
                           0,
                           MAX_PCI_CONFIG_LEN,
                           AdapterInfo->PciConfig
                         );

  PciConfigHeader = (PCI_CONFIG_HEADER *) AdapterInfo->PciConfig;

  // Enumerate through the PCI BARs for the device to determine which one is
  // the IO BAR.  Save the index of the BAR into the adapter info structure.
  TempBar = &PciConfigHeader->BaseAddressReg0;
  for (BarIndex = 0; BarIndex <= 5; BarIndex++) {
    DEBUGPRINT (E1000, ("BAR = %X\n", *TempBar));
    if ((*TempBar & PCI_BAR_MEM_MASK) == PCI_BAR_MEM_64BIT) {

      // This is a 64-bit memory bar, skip this and the
      // next bar as well.
      TempBar++;
    }

    // Find the IO BAR and save it's number into IoBar
    if ((*TempBar & PCI_BAR_IO_MASK) == PCI_BAR_IO_MODE) {

      // Here is the IO Bar - save it to the Gigabit adapter struct.
      AdapterInfo->IoBarIndex = BarIndex;
      break;
    }

    // Advance the pointer to the next bar in PCI config space
    TempBar++;
  }

  AdapterInfo->PciIo->GetLocation (
                       AdapterInfo->PciIo,
                       &AdapterInfo->Segment,
                       &AdapterInfo->Bus,
                       &AdapterInfo->Device,
                       &AdapterInfo->Function
                       );

  DEBUGPRINT (INIT, ("AdapterInfo->IoBarIndex = %X\n", AdapterInfo->IoBarIndex));
  DEBUGPRINT (INIT, ("PCI Command Register = %X\n", PciConfigHeader->Command));
  DEBUGPRINT (INIT, ("PCI Status Register = %X\n", PciConfigHeader->Status));
  DEBUGPRINT (INIT, ("PCI VendorID = %X\n", PciConfigHeader->VendorId));
  DEBUGPRINT (INIT, ("PCI DeviceID = %X\n", PciConfigHeader->DeviceId));
  DEBUGPRINT (INIT, ("PCI SubVendorID = %X\n", PciConfigHeader->SubVendorId));
  DEBUGPRINT (INIT, ("PCI SubSystemID = %X\n", PciConfigHeader->SubSystemId));
  DEBUGPRINT (INIT, ("PCI Segment = %X\n", AdapterInfo->Segment));
  DEBUGPRINT (INIT, ("PCI Bus = %X\n", AdapterInfo->Bus));
  DEBUGPRINT (INIT, ("PCI Device = %X\n", AdapterInfo->Device));
  DEBUGPRINT (INIT, ("PCI Function = %X\n", AdapterInfo->Function));

  ZeroMem (AdapterInfo->BroadcastNodeAddress, PXE_MAC_LENGTH);
  SetMem (AdapterInfo->BroadcastNodeAddress, PXE_HWADDR_LEN_ETHER, 0xFF);

  // Initialize all parameters needed for the shared code
  AdapterInfo->Hw.hw_addr                = (UINT8 *) (UINTN) PciConfigHeader->BaseAddressReg0;
  AdapterInfo->Hw.back                   = AdapterInfo;
  AdapterInfo->Hw.vendor_id              = PciConfigHeader->VendorId;
  AdapterInfo->Hw.device_id              = PciConfigHeader->DeviceId;
  AdapterInfo->Hw.subsystem_vendor_id    = PciConfigHeader->SubVendorId;
  AdapterInfo->Hw.subsystem_device_id    = PciConfigHeader->SubSystemId;
  AdapterInfo->Hw.revision_id            = PciConfigHeader->RevId;

  AdapterInfo->Hw.mac.autoneg            = TRUE;
  AdapterInfo->Hw.fc.current_mode        = e1000_fc_full;
  AdapterInfo->Hw.fc.requested_mode      = e1000_fc_full;

  AdapterInfo->Hw.phy.autoneg_wait_to_complete = FALSE;
  AdapterInfo->Hw.phy.reset_disable      = FALSE;
  AdapterInfo->Hw.phy.autoneg_advertised = E1000_ALL_SPEED_DUPLEX;
  AdapterInfo->Hw.phy.autoneg_mask       = AUTONEG_ADVERTISE_SPEED_DEFAULT;

  AdapterInfo->PciClass       = PciConfigHeader->ClassIdMain;
  AdapterInfo->PciSubClass    = PciConfigHeader->ClassIdSubclass;
  AdapterInfo->PciClassProgIf = PciConfigHeader->ClassIdProgIf;

  // We need to set the IO bar to zero for the shared code because the EFI PCI protocol
  // gets the BAR for us.
  AdapterInfo->Hw.io_base               = 0;

  //  This variable is set only to make the flash shared code work on ICH8.
  //  Set to 1 because the flash BAR will always be BAR 1.
  AdapterInfo->Hw.flash_address         = (UINT8 *) ((UINTN) 1);

  if (e1000_set_mac_type (&AdapterInfo->Hw) != E1000_SUCCESS) {
    DEBUGPRINT (CRITICAL, ("Unsupported MAC type!\n"));
    return EFI_UNSUPPORTED;
  }

  if (e1000_setup_init_funcs (&AdapterInfo->Hw, TRUE) != E1000_SUCCESS) {
    DEBUGPRINT (CRITICAL, ("e1000_setup_init_funcs failed!\n"));
    return EFI_UNSUPPORTED;
  }

  E1000LanFunction (AdapterInfo);


  DEBUGPRINT (E1000, ("Calling e1000_get_bus_info\n"));
  if (e1000_get_bus_info (&AdapterInfo->Hw) != E1000_SUCCESS) {
    DEBUGPRINT (CRITICAL, ("Could not read bus information\n"));
    return EFI_UNSUPPORTED;
  }

  DEBUGPRINT (E1000, ("Calling e1000_read_mac_addr\n"));
  if (e1000_read_mac_addr (&AdapterInfo->Hw) != E1000_SUCCESS) {
    DEBUGPRINT (CRITICAL, ("Could not read MAC address\n"));
    return EFI_UNSUPPORTED;
  }

  DEBUGPRINT (INIT, ("MAC Address: "));
  for (i = 0; i < 6; i++) {
    DEBUGPRINT (INIT, ("%2x ", AdapterInfo->Hw.mac.perm_addr[i]));
  }
  DEBUGPRINT (INIT, ("\n"));

  Reg = E1000_READ_REG (&AdapterInfo->Hw, E1000_CTRL_EXT);
  if ((Reg & E1000_CTRL_EXT_DRV_LOAD) != 0) {
    DEBUGPRINT (CRITICAL, ("iSCSI Boot detected on port!\n"));
    return EFI_ACCESS_DENIED;
  }


  ScStatus = e1000_reset_hw (&AdapterInfo->Hw);
  if (ScStatus != E1000_SUCCESS) {
    DEBUGPRINT (CRITICAL, ("e1000_reset_hw returns %d\n", ScStatus));
    return EFI_DEVICE_ERROR;
  }

  // Now that the structures are in place, we can configure the hardware to use it all.
  ScStatus = e1000_init_hw (&AdapterInfo->Hw);
  if (ScStatus == E1000_SUCCESS) {
    DEBUGPRINT (E1000, ("e1000_init_hw success\n"));
    Status = EFI_SUCCESS;
    AdapterInfo->HwInitialized = TRUE;
  } else {
    DEBUGPRINT (CRITICAL, ("Hardware Init failed status=%x\n", ScStatus));
    AdapterInfo->HwInitialized = FALSE;
    Status = EFI_DEVICE_ERROR;
  }


#ifndef NO_82571_SUPPORT

  // On 82571 based adapters if either port is reset then the
  // MAC address will be loaded into the EEPROM If the user overrides the default MAC
  // address using the StnAddr command then the 82571 will reset the MAC address
  // the next time either port is reset.  This check resets the MAC
  // address to the default value specified by the user.
  if (AdapterInfo->Hw.mac.type == e1000_82571
    && AdapterInfo->MacAddrOverride)
  {
    DEBUGPRINT (E1000, ("RESETING STATION ADDRESS\n"));
    e1000_rar_set (&AdapterInfo->Hw, AdapterInfo->Hw.mac.addr, 0);
  }
#endif /* NO_82571_SUPPORT */

  Reg = E1000_READ_REG (&AdapterInfo->Hw, E1000_CTRL_EXT);
  Reg |= E1000_CTRL_EXT_DRV_LOAD;
  E1000_WRITE_REG (&AdapterInfo->Hw, E1000_CTRL_EXT, Reg);

  return Status;
}

/** Initializes the gigabit adapter, setting up memory addresses, MAC Addresses,
   Type of card, etc.

   @param[in]   AdapterInfo   Pointer to adapter structure

   @retval   PXE_STATCODE_SUCCESS       Initialization succeeded
   @retval   PXE_STATCODE_NOT_STARTED   Hardware Init failed
**/
PXE_STATCODE
E1000Inititialize (
  IN DRIVER_DATA *AdapterInfo
  )
{
  PXE_STATCODE PxeStatcode = PXE_STATCODE_SUCCESS;

  DEBUGPRINT (E1000, ("E1000Inititialize\n"));


  DEBUGWAIT (E1000);

  E1000SetSpeedDuplex (AdapterInfo);

  // If the hardware has already been initialized then don't bother with a reset
  // We want to make sure we do not have to restart autonegotiation and two-pair
  // downshift.
  if (!AdapterInfo->HwInitialized) {
    DEBUGPRINT (E1000, ("Initializing hardware!\n"));

    if (e1000_init_hw (&AdapterInfo->Hw) == 0) {
      DEBUGPRINT (E1000, ("e1000_init_hw success\n"));
      PxeStatcode = PXE_STATCODE_SUCCESS;
      AdapterInfo->HwInitialized      = TRUE;
    } else {
      DEBUGPRINT (CRITICAL, ("Hardware Init failed\n"));
      PxeStatcode = PXE_STATCODE_NOT_STARTED;
    }
  } else {
    DEBUGPRINT (E1000, ("Skipping adapter reset\n"));
    PxeStatcode = PXE_STATCODE_SUCCESS;
  }

  if (PxeStatcode == PXE_STATCODE_SUCCESS) {
    E1000TxRxConfigure (AdapterInfo);
  }

  // Re-read the MAC address.  The CLP configured MAC address is being reset by
  // hardware to the factory address after init, so we need to reset it here.
  if (e1000_read_mac_addr (&AdapterInfo->Hw) != E1000_SUCCESS) {
    DEBUGPRINT (CRITICAL, ("Could not read MAC address.\n"));
  }

  DEBUGWAIT (E1000);

  return PxeStatcode;
}

/** Enables Rx unit.

   @param[in]   AdapterInfo   Pointer to the adapter structure

   @return   RX unit enabled
**/
VOID
RxEnable (
  IN DRIVER_DATA *AdapterInfo
  )
{
  UINT32 RctlReg;

  RctlReg = E1000_READ_REG (&AdapterInfo->Hw, E1000_RCTL);
  RctlReg |= E1000_RCTL_EN;
  E1000_WRITE_REG (&AdapterInfo->Hw, E1000_RCTL, RctlReg);
}

/** Disables Rx unit.

   @param[in]   AdapterInfo   Pointer to the adapter structure

   @return   RX unit disabled or not depending on device MAC type
**/
VOID
RxDisable (
  IN DRIVER_DATA *AdapterInfo
  )
{
  UINT32 RctlReg;

  // No need to disable RX on the following devices
  // when changing receive filters
  if ((AdapterInfo->Hw.mac.type == e1000_82574)
    || (AdapterInfo->Hw.mac.type == e1000_82575)
    || (AdapterInfo->Hw.mac.type == e1000_82583)
    || (AdapterInfo->Hw.mac.type == e1000_i350)
    || (AdapterInfo->Hw.mac.type == e1000_i354)
    || (AdapterInfo->Hw.mac.type == e1000_i210)
    || (AdapterInfo->Hw.mac.type == e1000_i211))
  {
    return;
  }

  RctlReg = E1000_READ_REG (&AdapterInfo->Hw, E1000_RCTL);
  RctlReg &= ~E1000_RCTL_EN;
  E1000_WRITE_REG (&AdapterInfo->Hw, E1000_RCTL, RctlReg);
}

/** Changes filter settings

   @param[in]   AdapterInfo  Pointer to the NIC data structure information which the
                            UNDI driver is layering on..
   @param[in]   NewFilter   A PXE_OPFLAGS bit field indicating what filters to use.
   @param[in]   Cpb         The command parameter Block address.  64 bits since this is Itanium(tm)
                            processor friendly
   @param[in]   CpbSize     Command parameter Block size

   @retval   0   Filters changed according to NewFilter settings
**/
UINTN
E1000SetFilter (
  IN DRIVER_DATA *AdapterInfo,
  IN UINT16       NewFilter,
  IN UINT64       Cpb,
  IN UINT32       CpbSize
  )
{
  PXE_CPB_RECEIVE_FILTERS *CpbReceiveFilter;
  UINT32                   UpdateRCTL;
  UINT16                   CfgFilter;
  UINT16                   OldFilter;
  UINT16                   MulticastCount;
  UINT16                   i;
  UINT16                   j;

  DEBUGPRINT (E1000, ("E1000SetFilter\n"));

  CpbReceiveFilter = (PXE_CPB_RECEIVE_FILTERS *) (UINTN) Cpb;
  OldFilter = AdapterInfo->RxFilter;

  // only these bits need a change in the configuration
  // actually change in bcast requires configure but we ignore that change
  CfgFilter = PXE_OPFLAGS_RECEIVE_FILTER_PROMISCUOUS |
              PXE_OPFLAGS_RECEIVE_FILTER_BROADCAST |
              PXE_OPFLAGS_RECEIVE_FILTER_ALL_MULTICAST;

  if ((OldFilter & CfgFilter) != (NewFilter & CfgFilter)) {

    UpdateRCTL = E1000_READ_REG (&AdapterInfo->Hw, E1000_RCTL);

    if (NewFilter & PXE_OPFLAGS_RECEIVE_FILTER_PROMISCUOUS) {

      // add the UPE bit to the variable to be written to the RCTL
      UpdateRCTL |= E1000_RCTL_UPE;
    }


    if (NewFilter & PXE_OPFLAGS_RECEIVE_FILTER_ALL_MULTICAST) {

      // add the MPE bit to the variable to be written to the RCTL
      UpdateRCTL |= E1000_RCTL_MPE;
    }

    // Put the card into the proper mode...
    // Rx unit needs to be disabled and re-enabled
    // while changing filters.
    if (AdapterInfo->RxRing.IsRunning) {
      RxDisable (AdapterInfo);
    }

    UpdateRCTL |= E1000_RCTL_BAM;
    AdapterInfo->RxFilter = NewFilter;
    E1000_WRITE_REG (&AdapterInfo->Hw, E1000_RCTL, UpdateRCTL);

    if (AdapterInfo->RxRing.IsRunning) {
      RxEnable (AdapterInfo);
    }
  }

  // check if mcast setting changed
  if (((NewFilter & PXE_OPFLAGS_RECEIVE_FILTER_FILTERED_MULTICAST) !=
    (OldFilter & PXE_OPFLAGS_RECEIVE_FILTER_FILTERED_MULTICAST))
    || (CpbReceiveFilter != NULL))
  {

    // copy the list
    if (CpbReceiveFilter != NULL) {
      UINT8 McAddrList[MAX_MCAST_ADDRESS_CNT][ETH_ALEN];

      MulticastCount = AdapterInfo->McastList.Length = (UINT16) (CpbSize / PXE_MAC_LENGTH);
      DEBUGPRINT (E1000, ("E1000: MulticastCount=%d\n", MulticastCount));

      ZeroMem (AdapterInfo->McastList.McAddr, MAX_MCAST_ADDRESS_CNT * PXE_MAC_LENGTH);
      CopyMem (
        AdapterInfo->McastList.McAddr,
        (VOID *) (UINTN) CpbReceiveFilter->MCastList,
        CpbSize
      );

      // Copy the multicast address list into a form that can be accepted by the
      // shared code.
      for (i = 0; (i < MulticastCount && i < MAX_MCAST_ADDRESS_CNT); i++) {
        DEBUGPRINT (E1000, ("E1000: MulticastAddress %d:", i));
        for (j = 0; j < ETH_ALEN; j++) {
          McAddrList[i][j] = AdapterInfo->McastList.McAddr[i][j];
          DEBUGPRINT (E1000, ("%02x", CpbReceiveFilter->MCastList[i][j]));
        }
        DEBUGPRINT (E1000, ("\n"));
      }

      TransmitLockIo (AdapterInfo, TRUE);

      //Rx unit needs to be disabled while changing filters.
      if (AdapterInfo->RxRing.IsRunning) {
        RxDisable (AdapterInfo);
      }
      e1000_update_mc_addr_list (
        &AdapterInfo->Hw,
        &McAddrList[0][0],
        MulticastCount
      );
      if (AdapterInfo->RxRing.IsRunning) {
        RxEnable (AdapterInfo);
      }
      TransmitLockIo (AdapterInfo, FALSE);
    }

    // are we setting the list or resetting??
    if ((NewFilter & PXE_OPFLAGS_RECEIVE_FILTER_FILTERED_MULTICAST) != 0) {
      DEBUGPRINT (E1000, ("E1000: Creating new multicast list.\n"));
      AdapterInfo->RxFilter |= PXE_OPFLAGS_RECEIVE_FILTER_FILTERED_MULTICAST;
    } else {
      DEBUGPRINT (E1000, ("E1000: Disabling multicast list.\n"));
      AdapterInfo->RxFilter &= (~PXE_OPFLAGS_RECEIVE_FILTER_FILTERED_MULTICAST);
    }
  }

  if (NewFilter != 0) {

    // Enable unicast and start the RU
    AdapterInfo->RxFilter |= (NewFilter | PXE_OPFLAGS_RECEIVE_FILTER_UNICAST);
    E1000ReceiveStart (AdapterInfo);
  } else {

    // may be disabling everything!
    AdapterInfo->RxFilter = NewFilter;
    E1000ReceiveStop (AdapterInfo);
  }

  return 0;
}

/** Stops the receive unit.

   @param[in]   AdapterInfo   Pointer to the NIC data structure information
                             which the UNDI driver is layering on..

   @return   Receive unit stopped
**/
VOID
E1000ReceiveStop (
  IN DRIVER_DATA *AdapterInfo
  )
{
  EFI_STATUS      Status;

  Status = ReceiveStop (AdapterInfo);

  switch (Status) {
  case EFI_NOT_STARTED:
    DEBUGPRINT (E1000, ("Rx queue already stopped.\n"));
    break;
  case EFI_SUCCESS:
    DEBUGPRINT (E1000, ("Rx queue stopped successfully.\n"));
    break;
  default:
    DEBUGPRINT (CRITICAL, ("Failed to stop Rx queue: %r\n", Status));
    ASSERT_EFI_ERROR (Status);
    break;
  }

  Status = ReceiveReset (AdapterInfo);

  if (EFI_ERROR (Status)) {
    DEBUGPRINT (CRITICAL, ("Failed to reset Rx queue: %r\n", Status));
    ASSERT_EFI_ERROR (Status);
  }
}

/** Starts the receive unit.

   @param[in]   AdapterInfo   Pointer to the NIC data structure information
                             which the UNDI driver is layering on..

   @return   Receive unit started
**/
VOID
E1000ReceiveStart (
  IN DRIVER_DATA *AdapterInfo
  )
{
  EFI_STATUS      Status;

  DEBUGPRINT (E1000, ("E1000ReceiveStart\n"));

  Status = ReceiveStart (AdapterInfo);

  switch (Status) {
  case EFI_ALREADY_STARTED:
    DEBUGPRINT (E1000, ("Rx queue already started.\n"));
    break;
  case EFI_SUCCESS:
    DEBUGPRINT (E1000, ("Rx queue enabled successfully.\n"));
    break;
  default:
    DEBUGPRINT (CRITICAL, ("Failed to start Rx queue: %r\n", Status));
    ASSERT_EFI_ERROR (Status);
    break;
  }
}

/** This routine blocks until auto-negotiation completes or times out (after 4.5 seconds).

   @param[in]   AdapterInfo   Pointer to the NIC data structure information
                             which the UNDI driver is layering on..

   @retval   TRUE   Auto-negotiation completed successfully,
   @retval   FALSE  Auto-negotiation did not complete (i.e., timed out)
**/
BOOLEAN
E1000WaitForAutoNeg (
  IN DRIVER_DATA *AdapterInfo
  )
{
  BOOLEAN AutoNegComplete;
  UINTN   i;
  UINT16  Reg;
  UINT32  Status;

  AutoNegComplete   = FALSE;
  Status            = 0;

  DEBUGPRINT (E1000, ("E1000WaitForAutoNeg\n"));

  if (!AdapterInfo->CableDetect) {

    // Caller specified not to detect cable, so we return true.
    DEBUGPRINT (E1000, ("Cable detection disabled.\n"));
    return TRUE;
  }


  for (i = 0; i < 500; i++) {
    Status = E1000_READ_REG (&AdapterInfo->Hw, E1000_STATUS);
    if ((Status & E1000_STATUS_LU) != 0) {
      if ((E1000_DEV_ID_I210_COPPER == AdapterInfo->Hw.device_id) ||
        (E1000_DEV_ID_I210_COPPER_FLASHLESS == AdapterInfo->Hw.device_id) ||
        (E1000_DEV_ID_I211_COPPER == AdapterInfo->Hw.device_id))
      {
        DELAY_IN_MILLISECONDS (1000);
      }
      DEBUGPRINT (E1000, ("Successfully established link on retry %d\n", i));
      return TRUE;
    }
    DELAY_IN_MILLISECONDS (10);
  }
  DEBUGPRINT (E1000, ("Link up not detected\n"));

  if (AdapterInfo->Hw.phy.type == e1000_phy_igp) {
    DEBUGPRINT (E1000, ("IGP PHY\n"));
    for (i = 5; i != 0; i--) {
      e1000_read_phy_reg (&AdapterInfo->Hw, PHY_1000T_STATUS, &Reg);
      if (Reg != 0) {
        AutoNegComplete = E1000DownShift (AdapterInfo);
        break;
      }
      DELAY_IN_MILLISECONDS (1000);
      Status = E1000_READ_REG (&AdapterInfo->Hw, E1000_STATUS);
      if ((Status & E1000_STATUS_LU) != 0) {
        AutoNegComplete = TRUE;
        break;
      }
    }

  } else if (AdapterInfo->Hw.phy.type == e1000_phy_m88) {

    // We are on a Marvel PHY that supports 2-pair downshift
    // Check the real time link status bit to see if there is actually a cable connected
    // If so then we will attempt to downshift, if not then we will report failure
    // Wait for up to 1 second for real time link detected
    for (i = 100; i != 0; i--) {
      DEBUGPRINT (E1000, ("."));
      e1000_read_phy_reg (&AdapterInfo->Hw, M88E1000_PHY_SPEC_STATUS, &Reg);
      if ((Reg & M88E1000_PSSR_LINK) != 0) {
        DEBUGPRINT (E1000, ("E1000DownShift - Real Time Link Detected\n"));
        AutoNegComplete = E1000DownShift (AdapterInfo);
        break;
      }

      DELAY_IN_MILLISECONDS (10);
    }
  }

  DEBUGPRINT (E1000, ("Return %d\n", AutoNegComplete));
  DEBUGWAIT (E1000);
  return AutoNegComplete;
}

/** Free TX buffers that have been transmitted by the hardware.

   @param[in]   AdapterInfo   Pointer to the NIC data structure information
                             which the UNDI driver is layering on.
   @param[in]   NumEntries   Number of entries in the array which can be freed.
   @param[out]  TxBuffer     Array to pass back free TX buffer

   @return   Number of TX buffers written.
**/
UINT16
E1000FreeTxBuffers (
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

/** Sets specified bits in a device register

   @param[in]   AdapterInfo   Pointer to the device instance
   @param[in]   Register     Register to write
   @param[in]   BitMask      Bits to set

   @return   Returns the value read from the PCI register.
**/
UINT32
E1000SetRegBits (
  IN DRIVER_DATA *AdapterInfo,
  IN UINT32       Register,
  IN UINT32       BitMask
  )
{
  UINT32 TempReg;

  TempReg = E1000_READ_REG (&AdapterInfo->Hw, Register);
  TempReg |= BitMask;
  E1000_WRITE_REG (&AdapterInfo->Hw, Register, TempReg);

  return TempReg;
}

/** Clears specified bits in a device register

   @param[in]   AdapterInfo   Pointer to the device instance
   @param[in]   Register     Register to write
   @param[in]   BitMask      Bits to clear

   @return    Returns the value read from the PCI register.
**/
UINT32
E1000ClearRegBits (
  IN DRIVER_DATA *AdapterInfo,
  IN UINT32       Register,
  IN UINT32       BitMask
  )
{
  UINT32 TempReg;

  TempReg = E1000_READ_REG (&AdapterInfo->Hw, Register);
  TempReg &= ~BitMask;
  E1000_WRITE_REG (&AdapterInfo->Hw, Register, TempReg);

  return TempReg;
}

/** Gets information on current link up/down status.

   @param[in]   UndiPrivateData    Pointer to driver private data structure
   @param[out]  LinkUp             Link up/down status

   @retval  EFI_SUCCESS            Links status retrieved successfully
   @retval  !EFI_SUCCESS           Underlying function failure
**/
EFI_STATUS
GetLinkStatus (
  IN   UNDI_PRIVATE_DATA  *UndiPrivateData,
  OUT  BOOLEAN            *LinkUp
  )
{
  UINT32 Reg;

  Reg = E1000_READ_REG (&UndiPrivateData->NicInfo.Hw, E1000_STATUS);
  *LinkUp = (Reg & E1000_STATUS_LU) != 0;
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

   @param[in]   UndiPrivateData         Pointer to driver private data structure
   @param[out]  LinkSpeedModifiable     BOOLEAN value describing support

   @retval      EFI_SUCCESS             Successful operation
**/
EFI_STATUS
IsLinkSpeedModifiable (
  IN  UNDI_PRIVATE_DATA  *UndiPrivateData,
  OUT BOOLEAN            *LinkSpeedModifiable
  )
{
  *LinkSpeedModifiable = UndiPrivateData->NicInfo.Hw.phy.media_type == e1000_media_type_copper;
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
  e1000_setup_led (&UndiPrivateData->NicInfo.Hw);

  switch (UndiPrivateData->NicInfo.Hw.mac.type) {
#ifndef NO_82574_SUPPORT
  case e1000_82574:
#endif /* NO_82574_SUPPORT */
    {
      UINT32 Miliseconds = *Time * 1000;

      while (Miliseconds > 0) {
        e1000_led_on (&UndiPrivateData->NicInfo.Hw);
        DelayInMicroseconds (&UndiPrivateData->NicInfo, 200 * 1000);
        Miliseconds -= 200;
        e1000_led_off (&UndiPrivateData->NicInfo.Hw);

        // Do not wait last 200ms with LEDs off.
        if (Miliseconds > 200) {
          DelayInMicroseconds (&UndiPrivateData->NicInfo, 200 * 1000);
          Miliseconds -= 200;
        } else {

          // Break here when the Seconds is an odd number.
          // Break on 'while (Miliseconds > 0)' when the Seconds is an even number.
          break;
        }
      }
    }
    break;

  default:
    e1000_blink_led (&UndiPrivateData->NicInfo.Hw);
    DelayInMicroseconds (&UndiPrivateData->NicInfo, *Time * 1000 * 1000);
    break;
  }

  e1000_cleanup_led (&UndiPrivateData->NicInfo.Hw);
  return EFI_SUCCESS;
}

/** Reads PBA string from NVM.

   @param[in]   UndiPrivateData  Pointer to driver private data structure
   @param[out]  PbaNumberStr     Output string buffer for PBA string

   @retval   EFI_SUCCESS            PBA string successfully read
   @retval   EFI_SUCCESS            PBA string unsupported by adapter
   @retval   EFI_DEVICE_ERROR       Failed to read PBA flags word
   @retval   EFI_DEVICE_ERROR       Failed to read PBA module pointer
   @retval   EFI_DEVICE_ERROR       Failed to read PBA length
   @retval   EFI_DEVICE_ERROR       Failed to read PBA number word
**/
EFI_STATUS
GetPbaStr (
  IN  UNDI_PRIVATE_DATA  *UndiPrivateData,
  OUT EFI_STRING         PbaNumberStr
  )
{
  INT32   Status;
  CHAR8   PbaStringAscii[HII_MAX_STR_LEN_BYTES];

  Status = e1000_read_pba_string (&UndiPrivateData->NicInfo.Hw, PbaStringAscii, HII_MAX_STR_LEN_BYTES);
  if (Status != E1000_SUCCESS) {
    return EFI_DEVICE_ERROR;
  }
  UnicodeSPrint (PbaNumberStr, HII_MAX_STR_LEN_BYTES, L"%a", PbaStringAscii);
  return EFI_SUCCESS;
}

/** Gets 1Gig chip type string.

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
  switch (UndiPrivateData->NicInfo.Hw.mac.type) {
  #ifndef NO_82571_SUPPORT
    case e1000_82571:
      UnicodeSPrint (ChipTypeStr, HII_MAX_STR_LEN_BYTES, L"Intel 82571");
      break;
    case e1000_82572:
      UnicodeSPrint (ChipTypeStr, HII_MAX_STR_LEN_BYTES, L"Intel 82572");
      break;
    case e1000_82573:
      UnicodeSPrint (ChipTypeStr, HII_MAX_STR_LEN_BYTES, L"Intel 82573");
      break;
  #ifndef NO_82574_SUPPORT
    case e1000_82574:
      UnicodeSPrint (ChipTypeStr, HII_MAX_STR_LEN_BYTES, L"Intel 82574");
      break;
    case e1000_82583:
      UnicodeSPrint (ChipTypeStr, HII_MAX_STR_LEN_BYTES, L"Intel 82583V");
      break;
  #endif /* NO_82574_SUPPORT */
  #endif /* NO_82571_SUPPORT */
  #ifndef NO_80003ES2LAN_SUPPORT
    case e1000_80003es2lan:
      UnicodeSPrint (ChipTypeStr, HII_MAX_STR_LEN_BYTES, L"Intel 80003ES2LAN");
      break;
  #endif /* NO_80003ES2LAN_SUPPORT */
  #ifndef NO_82575_SUPPORT
    case e1000_82575:
      UnicodeSPrint (ChipTypeStr, HII_MAX_STR_LEN_BYTES, L"Intel 82575");
      break;
    case e1000_82576:
      UnicodeSPrint (ChipTypeStr, HII_MAX_STR_LEN_BYTES, L"Intel 82576");
      break;
  #endif /* NO_82575_SUPPORT */
  #ifndef NO_82580_SUPPORT
    case e1000_82580:
      UnicodeSPrint (ChipTypeStr, HII_MAX_STR_LEN_BYTES, L"Intel 82580");
      break;
  #endif /* NO_82580_SUPPORT */
    case e1000_i350:
      UnicodeSPrint (ChipTypeStr, HII_MAX_STR_LEN_BYTES, L"Intel i350");
      break;
    case e1000_i354:
      UnicodeSPrint (ChipTypeStr, HII_MAX_STR_LEN_BYTES, L"Intel i354");
      break;
    case e1000_i210:
      UnicodeSPrint (ChipTypeStr, HII_MAX_STR_LEN_BYTES, L"Intel i210");
      break;
    case e1000_i211:
      UnicodeSPrint (ChipTypeStr, HII_MAX_STR_LEN_BYTES, L"Intel i211");
      break;
    default:
      UnicodeSPrint (ChipTypeStr, HII_MAX_STR_LEN_BYTES, L"Unknown");
      break;
  }

  return EFI_SUCCESS;
}

/** Gets HII formset help string ID.

   @param[in]   UndiPrivateData  Pointer to driver private data structure

   @return   EFI_STRING_ID of formset help string
**/
EFI_STRING_ID
GetFormSetHelpStringId (
  IN UNDI_PRIVATE_DATA  *UndiPrivateData
  )
{
  return STRING_TOKEN (STR_INV_FORM_SET_1_HELP);
}

/** Detects surprise removal device status in PCI controller register

   @param[in]   AdapterInfo   Pointer to the device instance

   @retval   TRUE    Surprise removal has been detected
   @retval   FALSE   Surprise removal has not been detected
**/
BOOLEAN
IsSurpriseRemoval (
  IN DRIVER_DATA *AdapterInfo
  )
{
  UINT32 Results;

  if (AdapterInfo->SurpriseRemoval) {
    return TRUE;
  }

  MemoryFence ();
  AdapterInfo->PciIo->Mem.Read (
                            AdapterInfo->PciIo,
                            EfiPciIoWidthUint32,
                            0,
                            E1000_STATUS,
                            1,
                            (VOID *) (&Results)
                          );
  MemoryFence ();

  if (Results == INVALID_STATUS_REGISTER_VALUE) {

    if ((AdapterInfo->Hw.mac.type >= e1000_i210)
      && !( e1000_get_flash_presence_i210 (&AdapterInfo->Hw)))
    {
      // during e1000_pll_workaround_i210 flow, flashless springville adapter
      // may be put in D3 power state for a milisecond. In order to avoid
      // SurpriseRemoval state need to check if the device is in D3 power state
      UINT16 PciWord;
      MemoryFence ();
      AdapterInfo->PciIo->Pci.Read (
                                AdapterInfo->PciIo,
                                EfiPciIoWidthUint16,
                                E1000_PCI_PMCSR,
                                1,
                                (VOID *) &PciWord
                              );
      MemoryFence ();

      if (PciWord == 0xFFFF) {
        // if we've read an 0xFFFF from PCI config space value
        // it still means that we're it's a surprise removal state
        AdapterInfo->SurpriseRemoval = TRUE;
        return TRUE;
      }

      PciWord &= E1000_PCI_PMCSR_D3;
      if (PciWord == E1000_PCI_PMCSR_D3) {
        return FALSE;
      }

    }
    AdapterInfo->SurpriseRemoval = TRUE;
    return TRUE;
  }
  return FALSE;
}

/** Delay a specified number of microseconds

   @param[in]   AdapterInfo    Pointer to the NIC data structure information
                               which the UNDI driver is layering on..
   @param[in]   MicroSeconds   Time to delay in Microseconds.

   @return   Execution of code delayed
**/
VOID
DelayInMicroseconds (
  IN DRIVER_DATA *AdapterInfo,
  IN UINTN        MicroSeconds
  )
{
  if (AdapterInfo->Delay != NULL) {
    (*AdapterInfo->Delay) (AdapterInfo->UniqueId, MicroSeconds);
  } else {
    gBS->Stall (MicroSeconds);
  }
}


/** Returns information whether current device supports any RDMA protocol.

  @param[in]   UndiPrivateData    Pointer to the driver private data
  @param[out]  Supported          Tells whether feature is supported

  @retval     EFI_SUCCESS    Operation successful
**/
EFI_STATUS
IsRdmaSupported (
  IN   UNDI_PRIVATE_DATA  *UndiPrivateData,
  OUT  BOOLEAN            *Supported
  )
{
  *Supported = FALSE;
  return EFI_SUCCESS;
}

