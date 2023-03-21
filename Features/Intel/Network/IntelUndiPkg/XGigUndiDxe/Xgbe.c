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
#include "Xgbe.h"
#include "DeviceSupport.h"

#include <Library/HiiLib.h>
#include "Hii/Hii.h"

/** Checks if alternate MAC address is supported.

   @param[in]   UndiPrivateData    Driver instance private data structure
   @param[out]  AltMacSupport      Tells if alternate mac address is supported

   @retval   EFI_SUCCESS            Alternate MAC address is always supported
   @retval   EFI_DEVICE_ERROR       Failed to read data from NVM
   @retval   EFI_INVALID_PARAMETER  UndiPrivateData or AltMacSupport is NULL.
**/
EFI_STATUS
GetAltMacAddressSupport (
  IN   UNDI_PRIVATE_DATA  *UndiPrivateData,
  OUT  BOOLEAN            *AltMacSupport
  )
{
  INT32   XgbeStatus;
  UINT16  BackupMacPointer;

  if ((UndiPrivateData == NULL) ||
      (AltMacSupport == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  // Check to see if the backup MAC address location pointer is set
  XgbeStatus = ixgbe_read_eeprom (&UndiPrivateData->NicInfo.Hw, IXGBE_ALT_MAC_ADDR_PTR, &BackupMacPointer);
  if (XgbeStatus != IXGBE_SUCCESS) {
    DEBUGPRINT (CRITICAL, ("Failed to read device alternate MAC address support from NVM.\n"));
    return EFI_DEVICE_ERROR;
  }

  *AltMacSupport = ((BackupMacPointer != 0xFFFF) &&
                    (BackupMacPointer != 0x0000));
  return EFI_SUCCESS;
}


/** Gets Max Speed of ethernet port in bits.

   @param[in]   UndiPrivateData   Points to the driver instance private data
   @param[out]  MaxSpeed          Resultant value

   @retval      EFI_SUCCESS            Operation successful.
   @retval      EFI_INVALID_PARAMETER  UndiPrivateData or MaxSpeed is NULL
**/
EFI_STATUS
GetMaxSpeed (
  IN  UNDI_PRIVATE_DATA  *UndiPrivateData,
  OUT UINT64             *MaxSpeed
  )
{
  if ((UndiPrivateData == NULL) ||
      (MaxSpeed == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  *MaxSpeed = GIGABITS (10);
  return EFI_SUCCESS;
}



/** Iterate over list of multicast MAC addresses, and gets the current
   MAC address from the first address in the list

   @param[in]   Hw          Shared code hardware structure
   @param[in]   McAddrPtr   Pointer to table of multicast addresses
   @param[in]   Vmdq        VMDQ pointer

   @retval   Pointer to current MAC address
**/
UINT8 *
_XgbeIterateMcastMacAddr (
  IN struct ixgbe_hw *Hw,
  IN UINT8 **         McAddrPtr,
  IN UINT32 *         Vmdq
  )
{
  UINT8 *CurrentMac;

  CurrentMac = *McAddrPtr;
  *McAddrPtr += PXE_MAC_LENGTH;

  DEBUGPRINT (
    XGBE, ("Current MC MAC Addr = %02x %02x %02x %02x %02x %02x",
    CurrentMac[0], CurrentMac[1], CurrentMac[2], CurrentMac[3], CurrentMac[4], CurrentMac[5])
  );
  DEBUGWAIT (XGBE);

  return CurrentMac;
}

/** Copies the stats from our local storage to the protocol storage.

   It means it will read our read and clear numbers, so some adding is required before
   we copy it over to the protocol.

   @param[in]   AdapterInfo  Pointer to the NIC data structure information
                             which the UNDI driver is layering on..
   @param[in]   DbAddr   The data Block address
   @param[in]   DbSize   The data Block size

   @retval   PXE_STATCODE_SUCCESS  Statistics copied successfully
**/
UINTN
XgbeStatistics (
  IN DRIVER_DATA *AdapterInfo,
  IN UINT64       DbAddr,
  IN UINT16       DbSize
  )
{
  PXE_DB_STATISTICS *    DbPtr;
  struct ixgbe_hw *      Hw;
  struct ixgbe_hw_stats *St;
  UINTN                  Stat;

  Hw  = &AdapterInfo->Hw;
  St  = &AdapterInfo->Stats;

  UPDATE_OR_RESET_STAT (tpr, IXGBE_TPR);
  UPDATE_OR_RESET_STAT (gprc, IXGBE_GPRC);
  UPDATE_OR_RESET_STAT (ruc, IXGBE_RUC);
  UPDATE_OR_RESET_STAT (ruc, IXGBE_ROC);
  UPDATE_OR_RESET_STAT (rnbc[0], IXGBE_RNBC (0));
  UPDATE_OR_RESET_STAT (bprc, IXGBE_BPRC);
  UPDATE_OR_RESET_STAT (mprc, IXGBE_MPRC);
  UPDATE_OR_RESET_STAT (tpt, IXGBE_TPT);
  UPDATE_OR_RESET_STAT (gptc, IXGBE_GPTC);
  UPDATE_OR_RESET_STAT (bptc, IXGBE_BPTC);
  UPDATE_OR_RESET_STAT (mptc, IXGBE_MPTC);
  UPDATE_OR_RESET_STAT (crcerrs, IXGBE_CRCERRS);

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

  UPDATE_EFI_STAT (RX_TOTAL_FRAMES, tpr);
  UPDATE_EFI_STAT (RX_GOOD_FRAMES, gprc);
  UPDATE_EFI_STAT (RX_UNDERSIZE_FRAMES, ruc);
  UPDATE_EFI_STAT (RX_OVERSIZE_FRAMES, roc);
  UPDATE_EFI_STAT (RX_DROPPED_FRAMES, rnbc[0]);
  SET_SUPPORT (RX_UNICAST_FRAMES);
  DbPtr->Data[Stat] = (St->gprc - St->bprc - St->mprc);
  UPDATE_EFI_STAT (RX_BROADCAST_FRAMES, bprc);
  UPDATE_EFI_STAT (RX_MULTICAST_FRAMES, mprc);
  UPDATE_EFI_STAT (RX_CRC_ERROR_FRAMES, crcerrs);
  UPDATE_EFI_STAT (TX_TOTAL_FRAMES, tpt);
  UPDATE_EFI_STAT (TX_GOOD_FRAMES, gptc);
  SET_SUPPORT (TX_UNICAST_FRAMES);
  DbPtr->Data[Stat] = (St->gptc - St->bptc - St->mptc);
  UPDATE_EFI_STAT (TX_BROADCAST_FRAMES, bptc);
  UPDATE_EFI_STAT (TX_MULTICAST_FRAMES, mptc);

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
XgbeTransmit (
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

   @param[in]   AdapterInfo   pointer to the driver data
   @param[in]   CpbReceive    Pointer (Ia-64 friendly) to the command parameter Block.
                              The frame will be placed inside of it.
   @param[out]  DbReceive     The data buffer.  The out of band method of passing pre-digested
                              information to the protocol.

   @retval      PXE_STATCODE_NO_DATA        There is no data to receive.
   @retval      PXE_STATCODE_DEVICE_FAILURE AdapterInfo is NULL.
   @retval      PXE_STATCODE_DEVICE_FAILURE Device failure on packet receive.
   @retval      PXE_STATCODE_INVALID_CPB    Invalid CPB/DB parameters.
   @retval      PXE_STATCODE_NOT_STARTED    Rx queue not started.
   @retval      PXE_STATCODE_SUCCESS        Received data passed to the protocol.
**/
UINTN
XgbeReceive (
  IN  DRIVER_DATA     *AdapterInfo,
  IN  PXE_CPB_RECEIVE *CpbReceive,
  OUT PXE_DB_RECEIVE  *DbReceive
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

  // Acknowledge the interrupts
  IXGBE_READ_REG (&AdapterInfo->Hw, IXGBE_EICR);

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
XgbeSetInterruptState (
  IN DRIVER_DATA *AdapterInfo
  )
{
  UINT32 SetIntMask;

  SetIntMask = 0;

  DEBUGPRINT (XGBE, ("XgbeSetInterruptState\n"));

  // Mask the RX interrupts
  if ((AdapterInfo->IntMask & PXE_OPFLAGS_INTERRUPT_RECEIVE) != 0) {
    SetIntMask |= IXGBE_EICR_RTX_QUEUE_0_MASK;
    DEBUGPRINT (XGBE, ("Mask the RX interrupts\n"));
  }

  // Mask the TX interrupts
  if ((AdapterInfo->IntMask & PXE_OPFLAGS_INTERRUPT_TRANSMIT) != 0) {
    SetIntMask |= IXGBE_EICR_RTX_QUEUE_1_MASK;
    DEBUGPRINT (XGBE, ("Mask the TX interrupts\n"));
  }

  // Now we have all the Ints we want, so let the hardware know.
  IXGBE_WRITE_REG (&AdapterInfo->Hw, IXGBE_EIMS, SetIntMask);

  return PXE_STATCODE_SUCCESS;
}

/** Stop the hardware and put it all (including the PHY) into a known good state.

   @param[in]   AdapterInfo   Pointer to the driver structure

   @retval   PXE_STATCODE_SUCCESS    Hardware stopped
**/
UINTN
XgbeShutdown (
  IN DRIVER_DATA *AdapterInfo
  )
{
  EFI_STATUS    Status;

  DEBUGPRINT (XGBE, ("XgbeShutdown - adapter stop\n"));

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

  AdapterInfo->RxFilter = 0;

  XgbeDisableInterrupts (AdapterInfo);

  return PXE_STATCODE_SUCCESS;
}

/** Resets the hardware and put it all (including the PHY) into a known good state.

   @param[in]   AdapterInfo   The pointer to our context data
   @param[in]   OpFlags       The information on what else we need to do.

   @retval   PXE_STATCODE_SUCCESS        Successful hardware reset
   @retval   PXE_STATCODE_NOT_STARTED    Hardware init failed
**/
UINTN
XgbeReset (
  IN DRIVER_DATA *AdapterInfo,
  IN UINT16       OpFlags
  )
{
  EFI_STATUS Status;

  // Put the XGBE into a known state by resetting the transmit
  // and receive units of the XGBE and masking/clearing all
  // interrupts.
  // If the hardware has already been started then don't bother with a reset.
  if (!AdapterInfo->HwInitialized) {

    // Now that the structures are in place, we can configure the hardware to use it all.
    Status = XgbeInitHw (AdapterInfo);
    if (EFI_ERROR (Status)) {
      DEBUGPRINT (CRITICAL, ("XgbeInitHw returns %r\n", Status));
      return PXE_STATCODE_NOT_STARTED;
    }
  } else {
    DEBUGPRINT (XGBE, ("Skipping adapter reset\n"));
  }

  if ((OpFlags & PXE_OPFLAGS_RESET_DISABLE_FILTERS) == 0) {
    UINT16 SaveFilter;

    SaveFilter = AdapterInfo->RxFilter;

    // if we give the filter same as RxFilter, this routine will not set mcast list
    // (it thinks there is no change)
    // to force it, we will reset that flag in the RxFilter
    AdapterInfo->RxFilter &= (~PXE_OPFLAGS_RECEIVE_FILTER_FILTERED_MULTICAST);
    XgbeSetFilter (AdapterInfo, SaveFilter);
  }

  if (OpFlags & PXE_OPFLAGS_RESET_DISABLE_INTERRUPTS) {
    AdapterInfo->IntMask = 0; // disable the interrupts
  }

  XgbeSetInterruptState (AdapterInfo);

  return PXE_STATCODE_SUCCESS;
}

/** Configures internal interrupt causes on current PF.

   @param[in]   AdapterInfo   The pointer to our context data

   @return   Interrupt causes are configured for current PF
**/
VOID
XgbeConfigureInterrupts (
  IN DRIVER_DATA *AdapterInfo
  )
{
  UINT32  RegVal = 0;

  // Map Rx queue 0 interrupt to EICR bit0 and Tx queue 0interrupt to EICR bit1
  switch (AdapterInfo->Hw.mac.type) {
  case ixgbe_mac_82598EB:
    RegVal = IXGBE_IVAR_ALLOC_VAL;
    IXGBE_WRITE_REG (&AdapterInfo->Hw, IXGBE_IVAR (0), RegVal);
    RegVal = IXGBE_IVAR_ALLOC_VAL | 0x01;
    IXGBE_WRITE_REG (&AdapterInfo->Hw, IXGBE_IVAR (16), RegVal);
    break;
  case ixgbe_mac_82599EB:
  case ixgbe_mac_X540:
  case ixgbe_mac_X550:
  case ixgbe_mac_X550EM_x:
  case ixgbe_mac_X550EM_a:
    RegVal = ((IXGBE_IVAR_ALLOC_VAL | 0x01) << 8) | IXGBE_IVAR_ALLOC_VAL;
    IXGBE_WRITE_REG (&AdapterInfo->Hw, IXGBE_IVAR (0), RegVal);
    break;
  default:
    break;
  }

}

/** Disables internal interrupt causes on current PF.

   @param[in]   AdapterInfo   The pointer to our context data

   @return   Interrupt causes are disabled for current PF
**/
VOID
XgbeDisableInterrupts (
  IN DRIVER_DATA *AdapterInfo
  )
{

  // Deconfigure Interrupt Vector Allocation Register
  switch (AdapterInfo->Hw.mac.type) {
  case ixgbe_mac_82598EB:
    IXGBE_WRITE_REG (&AdapterInfo->Hw, IXGBE_IVAR (0), 0);
    IXGBE_WRITE_REG (&AdapterInfo->Hw, IXGBE_IVAR (16), 0);
    break;
  case ixgbe_mac_82599EB:
  case ixgbe_mac_X540:
  case ixgbe_mac_X550:
  case ixgbe_mac_X550EM_x:
  case ixgbe_mac_X550EM_a:
    IXGBE_WRITE_REG (&AdapterInfo->Hw, IXGBE_IVAR (0), 0);
    break;
  default:
    break;
  }
}

/** PCIe function to LAN port mapping.

   @param[in,out]   AdapterInfo   Pointer to adapter structure

   @return   LAN function set accordingly
**/
VOID
XgbeLanFunction (
  IN DRIVER_DATA *AdapterInfo
  )
{
  AdapterInfo->LanFunction = (IXGBE_READ_REG (&AdapterInfo->Hw, IXGBE_STATUS) & IXGBE_STATUS_LAN_ID)
                             >> IXGBE_STATUS_LAN_ID_SHIFT;
  DEBUGPRINT (INIT, ("PCI function %d is LAN port %d \n", AdapterInfo->Function, AdapterInfo->LanFunction));
  DEBUGWAIT (INIT);
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
XgbePciInit (
  IN DRIVER_DATA *AdapterInfo
  )
{
  EFI_STATUS Status;
  UINTN      Seg;
  UINT64     Result;
  BOOLEAN    PciAttributesSaved = FALSE;

  Result = 0;

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

  DEBUGPRINT (XGBE, ("Attributes supported %x\n", Result));

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

  AdapterInfo->PciIo->GetLocation (
                        AdapterInfo->PciIo,
                        &Seg,
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

   @param[in]   XgbePrivate   Pointer to driver private data

   @retval   EFI_SUCCESS        Hardware init success
   @retval   EFI_UNSUPPORTED    Shared code initialization failed
   @retval   EFI_UNSUPPORTED    Could not read MAC address
   @retval   EFI_ACCESS_DENIED  iSCSI Boot detected on port
   @retval   EFI_DEVICE_ERROR   Hardware init failed
**/
EFI_STATUS
XgbeFirstTimeInit (
  UNDI_PRIVATE_DATA *XgbePrivate
  )
{
  PCI_CONFIG_HEADER *PciConfigHeader;
  EFI_STATUS         Status;
  DRIVER_DATA       *AdapterInfo;
  INT32              ScStatus;
  UINT32             Reg;
  UINT16             i;

  DEBUGPRINT (CRITICAL, ("XgbeFirstTimeInit\n"));

  AdapterInfo             = &XgbePrivate->NicInfo;

  AdapterInfo->DriverBusy = FALSE;
  PciConfigHeader         = (PCI_CONFIG_HEADER *) &AdapterInfo->PciConfig[0];

  ZeroMem (AdapterInfo->BroadcastNodeAddress, PXE_MAC_LENGTH);
  SetMem (AdapterInfo->BroadcastNodeAddress, PXE_HWADDR_LEN_ETHER, 0xFF);

  DEBUGPRINT (XGBE, ("PciConfigHeader->VendorId = %X\n", PciConfigHeader->VendorId));
  DEBUGPRINT (XGBE, ("PciConfigHeader->DeviceId = %X\n", PciConfigHeader->DeviceId));


  // Initialize all parameters needed for the shared code
  AdapterInfo->Hw.hw_addr                       = 0;
  AdapterInfo->Hw.back                          = AdapterInfo;
  AdapterInfo->Hw.vendor_id                     = PciConfigHeader->VendorId;
  AdapterInfo->Hw.device_id                     = PciConfigHeader->DeviceId;
  AdapterInfo->Hw.revision_id                   = (UINT8) PciConfigHeader->RevId;
  AdapterInfo->Hw.subsystem_vendor_id           = PciConfigHeader->SubVendorId;
  AdapterInfo->Hw.subsystem_device_id           = PciConfigHeader->SubSystemId;
  AdapterInfo->Hw.revision_id                   = (UINT8) PciConfigHeader->RevId;
  AdapterInfo->Hw.adapter_stopped               = TRUE;
  AdapterInfo->Hw.fc.requested_mode             = ixgbe_fc_default;

  AdapterInfo->PciClass    = (UINT8) ((PciConfigHeader->ClassId & PCI_CLASS_MASK) >> 8);
  AdapterInfo->PciSubClass = (UINT8) (PciConfigHeader->ClassId) & PCI_SUBCLASS_MASK;

  ScStatus = ixgbe_init_shared_code (&AdapterInfo->Hw);
  if (ScStatus != IXGBE_SUCCESS) {
    DEBUGPRINT (CRITICAL, ("Error initializing shared code! %d\n", -ScStatus));

    // This is the only condition where we will fail.  We need to support SFP hotswap
    // which may produce an error if the SFP module is missing.
    if (ScStatus == IXGBE_ERR_DEVICE_NOT_SUPPORTED ||
      ScStatus == IXGBE_ERR_SFP_NOT_SUPPORTED)
    {
      return EFI_UNSUPPORTED;
    }
  }

  XgbeLanFunction (AdapterInfo);

  DEBUGPRINT (XGBE, ("Calling ixgbe_get_mac_addr\n"));
  if (ixgbe_get_mac_addr (&AdapterInfo->Hw, AdapterInfo->Hw.mac.perm_addr) != IXGBE_SUCCESS) {
    DEBUGPRINT (CRITICAL, ("Could not read MAC address\n"));
    return EFI_UNSUPPORTED;
  }

  // Copy perm_addr to addr. Needed in HII. Print it if requested.
  DEBUGPRINT (INIT, ("MAC Address: "));
  for (i = 0; i < 6; i++) {
    AdapterInfo->Hw.mac.addr[i] = AdapterInfo->Hw.mac.perm_addr[i];
    DEBUGPRINT (INIT, ("%2x ", AdapterInfo->Hw.mac.perm_addr[i]));
  }
  DEBUGPRINT (INIT, ("\n"));

  if (ixgbe_fw_recovery_mode (&AdapterInfo->Hw)) {
    // Firmware is in recovery mode - we CANNOT touch the NIC a lot from now on.
    // Report this via the Driver Health Protocol.

    DEBUGPRINT (CRITICAL, ("NIC firmware is in Recovery Mode, skip further initialization.\n"));
    AdapterInfo->FwSupported = FALSE;
    return EFI_UNSUPPORTED;
  } else {
    AdapterInfo->FwSupported = TRUE;
  }

  Reg = IXGBE_READ_REG (&AdapterInfo->Hw, IXGBE_CTRL_EXT);
  if ((Reg & IXGBE_CTRL_EXT_DRV_LOAD) != 0) {
    DEBUGPRINT (CRITICAL, ("XgbeFirstTimeInit: iSCSI Boot detected on port!\n"));
    return EFI_ACCESS_DENIED;
  }

  // Clear the Wake-up status register in case there has been a power management event
  IXGBE_WRITE_REG (&AdapterInfo->Hw, IXGBE_WUS, 0);

  ixgbe_init_swfw_semaphore (&AdapterInfo->Hw);

  Status = XgbeInitHw (AdapterInfo);
  if (EFI_ERROR (Status)) {
    DEBUGPRINT (CRITICAL, ("XgbeInitHw returns %r\n", Status));
    return Status;
  }

  XgbeSetRegBits (AdapterInfo, IXGBE_CTRL_EXT, IXGBE_CTRL_EXT_DRV_LOAD);

  return EFI_SUCCESS;
}


/** Initializes the hardware and sets up link.

   @param[in]   AdapterInfo   Pointer to adapter structure

   @retval   EFI_SUCCESS        Hardware initialized, link set up
   @retval   EFI_DEVICE_ERROR   Failed to initialize hardware
   @retval   EFI_DEVICE_ERROR   Failed to set up link
   @retval   EFI_DEVICE_ERROR   Failed to set the PCI bus info in ixgbe_hw structure
**/
EFI_STATUS
XgbeInitHw (
  IN DRIVER_DATA *AdapterInfo
  )
{
  INT32            ScStatus;
  ixgbe_link_speed Speed;

  // Now that the structures are in place, we can configure the hardware to use it all.

  ScStatus = ixgbe_init_hw (&AdapterInfo->Hw);
  if (ScStatus == 0) {
    DEBUGPRINT (XGBE, ("ixgbe_init_hw success\n"));
    AdapterInfo->HwInitialized = TRUE;
  } else if (ScStatus == IXGBE_ERR_SFP_NOT_PRESENT) {
    DEBUGPRINT (CRITICAL, ("ixgbe_init_hw returns IXGBE_ERR_SFP_NOT_PRESENT\n"));
    AdapterInfo->HwInitialized = TRUE;
  } else {
    DEBUGPRINT (CRITICAL, ("Hardware Init failed = %d\n", -ScStatus));
    AdapterInfo->HwInitialized = FALSE;
    return EFI_DEVICE_ERROR;
  }
  DEBUGWAIT (XGBE);

  ScStatus = ixgbe_set_phy_power (&AdapterInfo->Hw, TRUE);
  if (ScStatus != IXGBE_SUCCESS
    && ScStatus != IXGBE_NOT_IMPLEMENTED)
  {
    DEBUGPRINT (CRITICAL, ("ixgbe_set_phy_power failed with Status %X\n", ScStatus));
  }

  // Set the PCI bus info in ixgbe_hw structure
  ScStatus = ixgbe_get_bus_info (&AdapterInfo->Hw);
  if (ScStatus != IXGBE_SUCCESS) {
    DEBUGPRINT (CRITICAL, ("ixgbe_get_bus_info fails\n"));
    return EFI_DEVICE_ERROR;
  }

  // 82599+ silicons support 100Mb autonegotiation which is not supported
  // with 82598. This is why we initialize Speed parameter in different way.
  if (AdapterInfo->Hw.mac.type == ixgbe_mac_82598EB) {
    Speed = IXGBE_LINK_SPEED_82598_AUTONEG;
  } else if (AdapterInfo->Hw.device_id == IXGBE_DEV_ID_X550EM_X_KX4) {
    Speed = IXGBE_LINK_SPEED_1GB_FULL;
  } else if ((AdapterInfo->Hw.device_id == IXGBE_DEV_ID_X550EM_A_KR_L ||
    AdapterInfo->Hw.device_id == IXGBE_DEV_ID_X550EM_A_KR) &&
    AdapterInfo->Hw.phy.nw_mng_if_sel & IXGBE_NW_MNG_IF_SEL_PHY_SPEED_2_5G) {
    Speed = IXGBE_LINK_SPEED_2_5GB_FULL;
  } else if (AdapterInfo->Hw.device_id == IXGBE_DEV_ID_X550EM_A_SGMII ||
             AdapterInfo->Hw.device_id == IXGBE_DEV_ID_X550EM_A_SGMII_L) {
    Speed = IXGBE_LINK_SPEED_1GB_FULL |
            IXGBE_LINK_SPEED_100_FULL |
            IXGBE_LINK_SPEED_10_FULL;
  } else {
    Speed = IXGBE_LINK_SPEED_82599_AUTONEG;
  }

  AdapterInfo->XceiverModuleQualified = GetModuleQualificationResult (AdapterInfo);

  ScStatus = ixgbe_setup_link (&AdapterInfo->Hw, Speed, FALSE);
  if (ScStatus != IXGBE_SUCCESS) {
    DEBUGPRINT (CRITICAL, ("ixgbe_setup_link fails\n"));
    return EFI_DEVICE_ERROR;
  }

  // Enable interrupt causes.
  XgbeConfigureInterrupts (AdapterInfo);

  return EFI_SUCCESS;
}

/** Initializes the transmit and receive resources for the adapter.

   @param[in]   AdapterInfo   Pointer to adapter structure

   @return   TX/RX resources configured and initialized
**/
VOID
XgbeTxRxConfigure (
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

/** Initializes the gigabit adapter, setting up memory addresses, MAC Addresses,
   Type of card, etc.

   @param[in]   AdapterInfo   Pointer to adapter structure

   @retval   PXE_STATCODE_SUCCESS       Initialization succeeded
   @retval   PXE_STATCODE_NOT_STARTED   Hardware initialization failed
**/
PXE_STATCODE
XgbeInitialize (
  IN DRIVER_DATA *AdapterInfo
  )
{
  EFI_STATUS   Status;
  PXE_STATCODE PxeStatcode = PXE_STATCODE_SUCCESS;

  // If the hardware has already been started then don't bother with a reset
  // We want to make sure we do not have to restart link negotiation.
  if (!AdapterInfo->HwInitialized) {

    // Now that the structures are in place, we can configure the hardware to use it all.
    Status = XgbeInitHw (AdapterInfo);
    if (EFI_ERROR (Status)) {
      DEBUGPRINT (CRITICAL, ("XgbeInitHw returns %r\n", Status));
      PxeStatcode = PXE_STATCODE_NOT_STARTED;
    }
  } else {
    DEBUGPRINT (XGBE, ("Skipping adapter reset\n"));
    PxeStatcode = PXE_STATCODE_SUCCESS;
  }

  // If we reset the adapter then reinitialize the TX and RX rings
  // and reconfigure interrupt causes.
  if (PxeStatcode == PXE_STATCODE_SUCCESS) {
    XgbeTxRxConfigure (AdapterInfo);
    XgbeConfigureInterrupts (AdapterInfo);
  }

  return PxeStatcode;
}

/** Disable Rx unit. Use the Shared Code implementation to
   make sure all WAs are in place.

   @param[in]   AdapterInfo   Pointer to the adapter structure

   @return   RX unit disabled
**/
VOID
RxDisable (
  IN DRIVER_DATA *AdapterInfo
  )
{
  UINT32 RxCtrl;

  RxCtrl = IXGBE_READ_REG (&AdapterInfo->Hw, IXGBE_RXCTRL);
  RxCtrl &= ~IXGBE_RXCTRL_RXEN;
  ixgbe_enable_rx_dma (&AdapterInfo->Hw, RxCtrl);
}

/** Enable Rx unit. Use the Shared Code implementation to
   make sure all WAs are in place.

   @param[in]   AdapterInfo   Pointer to the adapter structure

   @return   RX unit enabled
**/
VOID
RxEnable (
  IN DRIVER_DATA *AdapterInfo
  )
{
  UINT32 RxCtrl;

  RxCtrl = IXGBE_READ_REG (&AdapterInfo->Hw, IXGBE_RXCTRL);
  RxCtrl |= IXGBE_RXCTRL_RXEN;
  ixgbe_enable_rx_dma (&AdapterInfo->Hw, RxCtrl);
}

/** Changes filter settings

   @param[in]   AdapterInfo  Pointer to the NIC data structure information which the
                            UNDI driver is layering on..
   @param[in]   NewFilter   A PXE_OPFLAGS bit field indicating what filters to use.

   @return   Filters changed according to NewFilter settings
**/
VOID
XgbeSetFilter (
  IN DRIVER_DATA *AdapterInfo,
  IN UINT16       NewFilter
  )
{
  UINT32 Fctrl;
  UINT32 FctrlInitial;

  DEBUGPRINT (RXFILTER, ("XgbeSetFilter: "));

  Fctrl = IXGBE_READ_REG (&AdapterInfo->Hw, IXGBE_FCTRL);
  FctrlInitial = Fctrl;

  if (NewFilter & PXE_OPFLAGS_RECEIVE_FILTER_PROMISCUOUS) {
    Fctrl |= IXGBE_FCTRL_UPE;
    DEBUGPRINT (RXFILTER, ("IXGBE_FCTRL_UPE "));
  }

  if (NewFilter & PXE_OPFLAGS_RECEIVE_FILTER_BROADCAST) {
    Fctrl |= IXGBE_FCTRL_BAM;
    DEBUGPRINT (RXFILTER, ("IXGBE_FCTRL_BAM "));
  }

  if (NewFilter & PXE_OPFLAGS_RECEIVE_FILTER_ALL_MULTICAST) {
    Fctrl |= IXGBE_FCTRL_MPE;
    DEBUGPRINT (RXFILTER, ("IXGBE_FCTRL_MPE "));
  }

  AdapterInfo->RxFilter |= NewFilter;
  DEBUGPRINT (RXFILTER, (", RxFilter=%08x, FCTRL=%08x\n", AdapterInfo->RxFilter, Fctrl));

  if (Fctrl != FctrlInitial) {

    // Filter has changed - write the new value
    // Receiver must be disabled during write to IXGBE_FCTRL
    if (AdapterInfo->RxRing.IsRunning) {
      if ((AdapterInfo->Hw.mac.type != ixgbe_mac_X550) &&
        (AdapterInfo->Hw.mac.type != ixgbe_mac_X550EM_x))
      {
        RxDisable (AdapterInfo);
      }
    }

    IXGBE_WRITE_REG (&AdapterInfo->Hw, IXGBE_FCTRL, Fctrl);

    if (AdapterInfo->RxRing.IsRunning) {
      if ((AdapterInfo->Hw.mac.type != ixgbe_mac_X550) &&
        (AdapterInfo->Hw.mac.type != ixgbe_mac_X550EM_x))
      {
        RxEnable (AdapterInfo);
      }
    }
  }

  // Start/Stop Rx unit based on the updated Rx filters
  if (AdapterInfo->RxFilter != 0) {
    XgbeReceiveStart (AdapterInfo);
  } else {
    XgbeReceiveStop (AdapterInfo);
  }

  DEBUGWAIT (XGBE);
}

/** Clears receive filters.

   @param[in]   AdapterInfo   Pointer to the adapter structure
   @param[in]   NewFilter   A PXE_OPFLAGS bit field indicating what filters to clear.

   @retval   0   Filters cleared according to NewFilter settings
**/
UINTN
XgbeClearFilter (
  IN DRIVER_DATA *AdapterInfo,
  IN UINT16       NewFilter
  )
{
  UINT32 Fctrl;
  UINT32 FctrlInitial;

  DEBUGPRINT (RXFILTER, ("XgbeClearFilter %x: ", NewFilter));

  Fctrl = IXGBE_READ_REG (&AdapterInfo->Hw, IXGBE_FCTRL);
  FctrlInitial = Fctrl;

  if (NewFilter & PXE_OPFLAGS_RECEIVE_FILTER_PROMISCUOUS) {
    Fctrl &= ~IXGBE_FCTRL_UPE;
    DEBUGPRINT (RXFILTER, ("IXGBE_FCTRL_UPE "));
  }

  if (NewFilter & PXE_OPFLAGS_RECEIVE_FILTER_BROADCAST) {
    Fctrl &= ~IXGBE_FCTRL_BAM;
    DEBUGPRINT (RXFILTER, ("IXGBE_FCTRL_BAM "));
  }

  if (NewFilter & PXE_OPFLAGS_RECEIVE_FILTER_ALL_MULTICAST) {

    // add the MPE bit to the variable to be written to the RCTL
    Fctrl &= ~IXGBE_FCTRL_MPE;
    DEBUGPRINT (RXFILTER, ("IXGBE_FCTRL_MPE "));
  }

  AdapterInfo->RxFilter &= ~NewFilter;
  DEBUGPRINT (RXFILTER, (", RxFilter=%08x, FCTRL=%08x\n", AdapterInfo->RxFilter, Fctrl));

  if (Fctrl != FctrlInitial) {

    // Filter has changed - write the new value
    // Receiver must be disabled during write to IXGBE_FCTRL
    if (AdapterInfo->RxRing.IsRunning) {
      if ((AdapterInfo->Hw.mac.type != ixgbe_mac_X550) &&
        (AdapterInfo->Hw.mac.type != ixgbe_mac_X550EM_x))
      {
        RxDisable (AdapterInfo);
      }
    }

    IXGBE_WRITE_REG (&AdapterInfo->Hw, IXGBE_FCTRL, Fctrl);

    if (AdapterInfo->RxRing.IsRunning) {
      if ((AdapterInfo->Hw.mac.type != ixgbe_mac_X550) &&
        (AdapterInfo->Hw.mac.type != ixgbe_mac_X550EM_x))
      {
        RxEnable (AdapterInfo);
      }
    }
  }

  // Start/Stop Rx unit based on the updated Rx filters
  if (AdapterInfo->RxFilter != 0) {
    XgbeReceiveStart (AdapterInfo);
  } else {
    XgbeReceiveStop (AdapterInfo);
  }

  DEBUGPRINT (XGBE, ("XgbeClearFilter done.\n"));
  DEBUGWAIT (XGBE);
  return 0;
}


/** Updates multicast filters, updates MAC address list and enables multicast

   @param[in]   AdapterInfo   Pointer to the adapter structure

   @return   All operations in description completed
**/
VOID
XgbeSetMcastList (
  IN DRIVER_DATA *AdapterInfo
  )
{
  // Updating Mcast filters requires disabling Rx unit
  if (AdapterInfo->RxRing.IsRunning) {
    if ((AdapterInfo->Hw.mac.type != ixgbe_mac_X550) &&
      (AdapterInfo->Hw.mac.type != ixgbe_mac_X550EM_x)) {
      RxDisable (AdapterInfo);
    }
  }

  if (AdapterInfo->McastList.Length == 0) {
    DEBUGPRINT (RXFILTER, ("Resetting multicast list\n"));
    AdapterInfo->RxFilter &= ~PXE_OPFLAGS_RECEIVE_FILTER_FILTERED_MULTICAST;
    ixgbe_disable_mc (&AdapterInfo->Hw);
    if (AdapterInfo->RxRing.IsRunning) {
      if ((AdapterInfo->Hw.mac.type != ixgbe_mac_X550) &&
        (AdapterInfo->Hw.mac.type != ixgbe_mac_X550EM_x))
      {
        RxEnable (AdapterInfo);
      }
    }
    return;
  }

  AdapterInfo->RxFilter |= PXE_OPFLAGS_RECEIVE_FILTER_FILTERED_MULTICAST;

  DEBUGPRINT (RXFILTER, ("Update multicast list, count=%d\n", AdapterInfo->McastList.Length));

  ixgbe_update_mc_addr_list (
    &AdapterInfo->Hw,
    (UINT8 *) &AdapterInfo->McastList.McAddr[0][0],
    AdapterInfo->McastList.Length,
    _XgbeIterateMcastMacAddr,
    true
  );

  ixgbe_enable_mc (&AdapterInfo->Hw);

  // Assume that if we are updating the MC list that we want to also
  // start the receiver.
  if (AdapterInfo->RxRing.IsRunning) {
    if ((AdapterInfo->Hw.mac.type != ixgbe_mac_X550) &&
      (AdapterInfo->Hw.mac.type != ixgbe_mac_X550EM_x))
    {
      RxEnable (AdapterInfo);
    }
  } else {
    XgbeReceiveStart (AdapterInfo);
  }
}

/** Stops the receive unit. Receive queue is also reset and all existing packets are dropped.

   @param[in]   AdapterInfo   Pointer to the NIC data structure information
                              which the UNDI driver is layering on..

   @return   Receive unit stopped
**/
VOID
XgbeReceiveStop (
  IN DRIVER_DATA *AdapterInfo
  )
{
  EFI_STATUS      Status;

  Status = ReceiveStop (AdapterInfo);

  switch (Status) {
  case EFI_NOT_STARTED:
    DEBUGPRINT (XGBE, ("Rx queue already stopped.\n"));
    break;
  case EFI_SUCCESS:
    DEBUGPRINT (XGBE, ("Rx queue stopped successfully.\n"));
    break;
  default:
    DEBUGPRINT (CRITICAL, ("Failed to stop Rx queue: %r\n", Status));
    ASSERT_EFI_ERROR (Status);
    return;
  }

  Status = ReceiveReset (AdapterInfo);

  if (EFI_ERROR (Status)) {
    DEBUGPRINT (CRITICAL, ("Failed to reset Rx queue: %r\n", Status));
    ASSERT_EFI_ERROR (Status);
  }
}

/** Starts the receive unit.

   @param[in]   AdapterInfo   Pointer to the NIC data structure information which
                              the UNDI driver is layering on..

   @return   Receive unit started
**/
VOID
XgbeReceiveStart (
  IN DRIVER_DATA *AdapterInfo
  )
{
  EFI_STATUS      Status;

  DEBUGPRINT (XGBE, ("XgbeReceiveStart\n"));

  Status = ReceiveStart (AdapterInfo);

  switch (Status) {
  case EFI_ALREADY_STARTED:
    DEBUGPRINT (XGBE, ("Rx queue already started.\n"));
    break;
  case EFI_SUCCESS:
    DEBUGPRINT (XGBE, ("Rx queue enabled successfully.\n"));
    break;
  default:
    DEBUGPRINT (CRITICAL, ("Failed to start Rx queue: %r\n", Status));
    ASSERT_EFI_ERROR (Status);
    break;
  }
}

/** Delay a specified number of microseconds

   @param[in]   AdapterInfo   Pointer to the NIC data structure information
                              which the UNDI driver is layering on..
   @param[in]   MicroSeconds   Time to delay in Microseconds.

   @return   Execution of code delayed
**/
VOID
DelayInMicroseconds (
  IN DRIVER_DATA *AdapterInfo,
  IN UINT32       MicroSeconds
  )
{
  if (AdapterInfo->Delay != NULL) {
    (*AdapterInfo->Delay)(AdapterInfo->UniqueId, MicroSeconds);
  } else {
    gBS->Stall (MicroSeconds);
  }
}

/** Swaps the bytes from machine order to network order (Big Endian)

   @param[in]   Dword   32-bit input value

   @return    Big Endian swapped value
**/
UINT32
IxgbeHtonl (
  IN UINT32 Dword
  )
{
  UINT8   Buffer[4];
  UINT32 *Result;

  DEBUGPRINT (XGBE, ("IxgbeHtonl = %x\n", Dword));

  Buffer[3] = (UINT8) Dword;
  Dword     = Dword >> 8;
  Buffer[2] = (UINT8) Dword;
  Dword     = Dword >> 8;
  Buffer[1] = (UINT8) Dword;
  Dword     = Dword >> 8;
  Buffer[0] = (UINT8) Dword;

  Result    = (UINT32 *) Buffer;
  DEBUGPRINT (XGBE, ("IxgbeHtonl result %x\n", *Result));
  DEBUGWAIT (XGBE);

  return *Result;
}

/** This function calls the MemIo callback to read a dword from the device's
   address space

   @param[in]   AdapterInfo   Adapter structure
   @param[in]   Port          Address to read from

   @retval    The data read from the port.
**/
UINT32
XgbeInDword (
  IN DRIVER_DATA *AdapterInfo,
  IN UINT32       Port
  )
{
  UINT32 Results;

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
  return Results;
}

/** This function calls the MemIo callback to write a word from the device's
   address space

   @param[in]   AdapterInfo   Adapter structure
   @param[in]   Port          Address to write to
   @param[in]   Data          Data to write to Port

   @return   Word written
**/
VOID
XgbeOutDword (
  IN DRIVER_DATA *AdapterInfo,
  IN UINT32       Port,
  IN UINT32       Data
  )
{
  UINT32 Value;

  Value = Data;

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
  return;
}

/** Sets specified bits in a device register

   @param[in]   AdapterInfo   Pointer to the device instance
   @param[in]   Register      Register to write
   @param[in]   BitMask       Bits to set

   @return    Returns the value read from the PCI register with BitMask applied.
**/
UINT32
XgbeSetRegBits (
  IN DRIVER_DATA *AdapterInfo,
  IN UINT32       Register,
  IN UINT32       BitMask
  )
{
  UINT32 TempReg;

  TempReg = IXGBE_READ_REG (&AdapterInfo->Hw, Register);
  TempReg |= BitMask;
  IXGBE_WRITE_REG (&AdapterInfo->Hw, Register, TempReg);

  return TempReg;
}

/** Clears specified bits in a device register

   @param[in]   AdapterInfo   Pointer to the device instance
   @param[in]   Register      Register to write
   @param[in]   BitMask       Bits to clear

   @return    Returns the value read from the PCI register with ~BitMask applied.
**/
UINT32
XgbeClearRegBits (
  IN DRIVER_DATA *AdapterInfo,
  IN UINT32       Register,
  IN UINT32       BitMask
  )
{
  UINT32 TempReg;

  TempReg = IXGBE_READ_REG (&AdapterInfo->Hw, Register);
  TempReg &= ~BitMask;
  IXGBE_WRITE_REG (&AdapterInfo->Hw, Register, TempReg);

  return TempReg;
}


/** This function calls the EFI PCI IO protocol to read a value from the device's PCI
   register space.

   @param[in]   AdapterInfo   Pointer to the shared code hw structure.
   @param[in]   Offset        Which register to read from.

   @return     The value read from the PCI register.
**/
UINT16
XgbeReadPci16 (
  IN DRIVER_DATA *AdapterInfo,
  IN UINT32       Offset
  )
{
  UINT16 Data;

  MemoryFence ();

  AdapterInfo->PciIo->Pci.Read (
                            AdapterInfo->PciIo,
                            EfiPciIoWidthUint16,
                            Offset,
                            1,
                            (VOID *) (&Data)
                          );
  MemoryFence ();
  return Data;
}

/** This function calls the EFI PCI IO protocol to write a value to the device's PCI
   register space.

   @param[in]   AdapterInfo   Pointer to the adapter structure.
   @param[in]   Offset        Which register to read from.
   @param[in]   Data          Returns the value read from the PCI register.

   @return    Value present in Data was written
**/
VOID
XgbeWritePci16 (
  IN DRIVER_DATA *AdapterInfo,
  IN UINT32       Offset,
  IN UINT16       Data
  )
{
  MemoryFence ();

  AdapterInfo->PciIo->Pci.Write (
                            AdapterInfo->PciIo,
                            EfiPciIoWidthUint16,
                            Offset,
                            1,
                            (VOID *) (&Data)
                          );
  MemoryFence ();
}

/** Flushes a PCI write transaction to system memory.

   @param[in]   AdapterInfo   Pointer to the adapter structure.

   @return   Write transaction flushed
**/
VOID
XgbePciFlush (
  IN DRIVER_DATA *AdapterInfo
  )
{
  MemoryFence ();

  AdapterInfo->PciIo->Flush (AdapterInfo->PciIo);

  MemoryFence ();

  return;
}

/** Free TX buffers that have been transmitted by the hardware.

   @param[in]   AdapterInfo   Pointer to the NIC data structure information which
                              the UNDI driver is layering on.
   @param[in]   NumEntries    Number of entries in the array which can be freed.
   @param[out]  TxBuffer      Array to pass back free TX buffer

   @return   Number of TX buffers written.
**/
UINT16
XgbeFreeTxBuffers (
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

/** Checks if link is up

   @param[in]   UndiPrivateData  Pointer to driver private data structure
   @param[out]  LinkUp           Link up/down status

   @retval  EFI_SUCCESS       Links status retrieved successfully
   @retval  EFI_UNSUPPORTED   Transceiver module is not qualified
   @retval  EFI_DEVICE_ERROR  Failed to read link status
**/
EFI_STATUS
GetLinkStatus (
  IN   UNDI_PRIVATE_DATA  *UndiPrivateData,
  OUT  BOOLEAN            *LinkUp
  )
{
  ixgbe_link_speed Speed;
  INT32            IxgbeStatus;

  IF_RETURN (!UndiPrivateData->NicInfo.XceiverModuleQualified, EFI_UNSUPPORTED);

  IxgbeStatus = ixgbe_check_link (&UndiPrivateData->NicInfo.Hw, &Speed, LinkUp, FALSE);
  if (IxgbeStatus != IXGBE_SUCCESS) {
    return EFI_DEVICE_ERROR;
  }
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

   @retval   EFI_SUCCESS            Successful operation
   @retval   EFI_INVALID_PARAMETER  Invalid parameter passed
**/
EFI_STATUS
IsLinkSpeedModifiable (
  IN  UNDI_PRIVATE_DATA  *UndiPrivateData,
  OUT BOOLEAN            *LinkSpeedModifiable
  )
{
  if ((UndiPrivateData == NULL) ||
      (LinkSpeedModifiable == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

    *LinkSpeedModifiable = FALSE;
  return EFI_SUCCESS;
}


/** Blinks LEDs on port managed by current PF.

   @param[in]   UndiPrivateData  Pointer to driver private data structure
   @param[in]   Time             Time in seconds to blink

   @retval  EFI_SUCCESS            LEDs blinked successfully
   @retval  EFI_DEVICE_ERROR       Failed to blink PHY link LED
   @retval  EFI_INVALID_PARAMETER  Invalid parameter passed
**/
EFI_STATUS
BlinkLeds (
  IN UNDI_PRIVATE_DATA  *UndiPrivateData,
  IN UINT16             *Time
  )
{
  DRIVER_DATA  *AdapterInfo = &UndiPrivateData->NicInfo;
  INT32        IxgbeStatus;
  UINT32       LedCtl;
  BOOLEAN      LedOn = FALSE;
  UINT32       i = 0;
  UINT32       LedIndex = 2;
  UINT32       MiliSeconds = *Time * 1000;
  UINT16       PhyRegVal = 0;

  // IXGBE shared code doesn't save/restore the LEDCTL register when blinking used.
  LedCtl = IXGBE_READ_REG (&AdapterInfo->Hw, IXGBE_LEDCTL);

  if (AdapterInfo->Hw.mac.type == ixgbe_mac_X540) {

    IXGBE_WRITE_REG (&AdapterInfo->Hw, IXGBE_LEDCTL, (LedCtl & ~0xFF) | 0x4E);
  } else if (AdapterInfo->Hw.mac.type == ixgbe_mac_82599EB &&
    AdapterInfo->Hw.device_id == 0x154A)
  {
    IXGBE_WRITE_REG (&AdapterInfo->Hw, IXGBE_LEDCTL, (LedCtl & ~0xFF00) | 0x4E00);
  }

  if (AdapterInfo->Hw.mac.type == ixgbe_mac_X550EM_x) {
    LedIndex = 0;
  }
  else if (AdapterInfo->Hw.mac.type == ixgbe_mac_X550EM_a) {
    LedIndex = AdapterInfo->Hw.mac.led_link_act;
  }

  switch (AdapterInfo->Hw.device_id) {
  case IXGBE_DEV_ID_X550EM_X_10G_T:
  case IXGBE_DEV_ID_X550EM_A_10G_T:
    IxgbeStatus = ixgbe_read_phy_reg (
                    &AdapterInfo->Hw,
                    IXGBE_X557_LED_PROVISIONING + LedIndex,
                    IXGBE_MDIO_VENDOR_SPECIFIC_1_DEV_TYPE,
                    &PhyRegVal
                    );
    if (IxgbeStatus != IXGBE_SUCCESS) {
      return EFI_DEVICE_ERROR;
    }

    IxgbeStatus = ixgbe_write_phy_reg (
                    &AdapterInfo->Hw,
                    IXGBE_X557_LED_PROVISIONING + LedIndex,
                    IXGBE_MDIO_VENDOR_SPECIFIC_1_DEV_TYPE,
                    0
                    );
    if (IxgbeStatus != IXGBE_SUCCESS) {
      return EFI_DEVICE_ERROR;
    }
  }
  if (MiliSeconds > 0) {
    for (i = 0; i < MiliSeconds; i += BLINK_INTERVAL) {
      LedOn = !LedOn;
      if (LedOn) {
        IxgbeStatus = ixgbe_led_on (&AdapterInfo->Hw, LedIndex);
      } else {
        IxgbeStatus = ixgbe_led_off (&AdapterInfo->Hw, LedIndex);
      }
      if (IxgbeStatus != IXGBE_SUCCESS) {
        return EFI_DEVICE_ERROR;
      }
      DelayInMicroseconds (AdapterInfo, BLINK_INTERVAL * 1000);
    }
  }
  IXGBE_WRITE_REG (&AdapterInfo->Hw, IXGBE_LEDCTL, LedCtl);
  IXGBE_WRITE_FLUSH (&AdapterInfo->Hw);

  switch (AdapterInfo->Hw.device_id) {
  case IXGBE_DEV_ID_X550EM_X_10G_T:
  case IXGBE_DEV_ID_X550EM_A_10G_T:
    IxgbeStatus = ixgbe_write_phy_reg (
                    &AdapterInfo->Hw,
                    IXGBE_X557_LED_PROVISIONING + LedIndex,
                    IXGBE_MDIO_VENDOR_SPECIFIC_1_DEV_TYPE,
                    PhyRegVal
                    );
    if (IxgbeStatus != IXGBE_SUCCESS) {
      return EFI_DEVICE_ERROR;
    }
  }
  return EFI_SUCCESS;
}

/** Reads PBA string from NVM.

   @param[in]   UndiPrivateData  Pointer to driver private data structure
   @param[out]  PbaNumberStr     Output string buffer for PBA string

   @retval   EFI_SUCCESS            PBA string successfully read
   @retval   EFI_DEVICE_ERROR       Failed to read PBA string
**/
EFI_STATUS
GetPbaStr (
  IN  UNDI_PRIVATE_DATA  *UndiPrivateData,
  OUT EFI_STRING         PbaNumberStr
  )
{
  INT32   Status;
  CHAR8   PbaStringAscii[HII_MAX_STR_LEN_BYTES];

  Status = ixgbe_read_pba_string (&UndiPrivateData->NicInfo.Hw, PbaStringAscii, HII_MAX_STR_LEN_BYTES);
  if (Status != IXGBE_SUCCESS) {
    return EFI_DEVICE_ERROR;
  }

  UnicodeSPrint (PbaNumberStr, HII_MAX_STR_LEN_BYTES, L"%a", PbaStringAscii);
  return EFI_SUCCESS;
}

/** Gets 10Gig chip type string.

   @param[in]   UndiPrivateData   Points to the driver instance private data
   @param[out]  ChipTypeStr       Points to the output string buffer

   @retval      EFI_SUCCESS       Chip type string successfully retrieved
**/
EFI_STATUS
GetChipTypeStr (
  IN  UNDI_PRIVATE_DATA  *UndiPrivateData,
  OUT EFI_STRING         ChipTypeStr
  )
{
  switch (UndiPrivateData->NicInfo.Hw.mac.type) {
  case ixgbe_mac_82598EB:
    UnicodeSPrint (ChipTypeStr, HII_MAX_STR_LEN_BYTES, L"Intel 82598EB");
    break;
  case ixgbe_mac_82599EB:
    UnicodeSPrint (ChipTypeStr, HII_MAX_STR_LEN_BYTES, L"Intel 82599");
    break;
  case ixgbe_mac_X540:
    UnicodeSPrint (ChipTypeStr, HII_MAX_STR_LEN_BYTES, L"Intel X540");
    break;
  case ixgbe_mac_X550:
  case ixgbe_mac_X550EM_x:
  case ixgbe_mac_X550EM_a:
    UnicodeSPrint (ChipTypeStr, HII_MAX_STR_LEN_BYTES, L"Intel X550");
    break;
  default:
    UnicodeSPrint (ChipTypeStr, HII_MAX_STR_LEN_BYTES, L"Unknown");
    break;
  }
  return EFI_SUCCESS;
}

/** Gets HII formset help string ID.

   @param[in]   UndiPrivateData  Pointer to driver private data structure

   @return      EFI_STRING_ID of formset help string, or 0 on failure
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

/** Returns information whether current device supports any RDMA protocol.

  @param[in]   UndiPrivateData    Pointer to the driver private data
  @param[out]  Supported          Tells whether feature is supported

  @retval      EFI_SUCCESS    Operation successful
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

/** Reverse bytes of a word (endianness change)

   @param[in]   Word   Value to be modified

   @return   Word reversed
**/
UINT16
IxgbeReverseWord (
  IN UINT16 Word
  )
{
  UINT8  SwapBuf;
  UINT8 *Ptr;

  Ptr = (UINT8 *) &Word;
  SwapBuf = Ptr[0];
  Ptr[0] = Ptr[1];
  Ptr[1] = SwapBuf;

  return Word;
}

/** Reverse bytes of a double word (endianness change)

   @param[in]   DWord   Value to be modified

   @return   DWord reversed
**/
UINT32
IxgbeReverseDword (
  IN UINT32 Dword
  )
{
  UINT16  SwapBuf;
  UINT16 *Ptr;

  Ptr = (UINT16 *) &Dword;
  SwapBuf = Ptr[0];
  Ptr[0] = IxgbeReverseWord (Ptr[1]);
  Ptr[1] = IxgbeReverseWord (SwapBuf);

  return Dword;
}

/** Check if current module used for this port is qualified module

   @param[in]   AdapterInfo   Pointer to the NIC data structure information
                              which the UNDI driver is layering on..

   @retval   TRUE    Module is qualified
   @retval   FALSE   Module is unqualified and module
                     qualification is enabled on the port
**/
BOOLEAN
GetModuleQualificationResult (
  IN DRIVER_DATA *AdapterInfo
  )
{
  INT32 ScStatus;

  ScStatus = AdapterInfo->Hw.phy.ops.identify_sfp (&AdapterInfo->Hw);
  DEBUGPRINT (HEALTH, ("identify_sfp returns = %d\n", ScStatus));
  if (ScStatus != IXGBE_SUCCESS
    && ScStatus != IXGBE_ERR_SFP_NOT_PRESENT)
  {
    return FALSE;
  }
  // Module is qualified or qualification process is not enabled
  return TRUE;
}



/** Check if adapter is based on 82599 (register-wise)

   @param[in]   AdapterInfo   Pointer to the NIC data structure.

   @retval   TRUE    Adapter is based on 82599
   @retval   FALSE   Adapter is not based on 82599

**/
BOOLEAN
IsNianticBasedDevice (
  IN  DRIVER_DATA   *AdapterInfo
  )
{
  ASSERT (AdapterInfo != NULL);

  switch (AdapterInfo->Hw.mac.type) {
  case ixgbe_mac_82599EB:
  case ixgbe_mac_X540:
  case ixgbe_mac_X550:
  case ixgbe_mac_X550EM_x:
  case ixgbe_mac_X550EM_a:
    return TRUE;

  default:
    return FALSE;
  }
}
