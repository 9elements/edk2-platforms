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
#include "DriverDiagnostics.h"
#include "Xgbe.h"

UINT8 mPacket[MAX_ETHERNET_SIZE];

/* Protocol structures tentative definitions */
EFI_DRIVER_DIAGNOSTICS_PROTOCOL  gXgbeUndiDriverDiagnostics;
EFI_DRIVER_DIAGNOSTICS2_PROTOCOL gXgbeUndiDriverDiagnostics2;

/** Build a packet to transmit in the PHY loopback test.

   @param[in]   AdapterInfo   Pointer to the NIC data structure information
                            which the UNDI driver is layering on so that we can
                            get the MAC address

   @return   Sets the global array mPacket[] with the packet to send out during PHY loopback.
**/
VOID
_BuildPacket (
  IN DRIVER_DATA *AdapterInfo
  )
{
  ETHERNET_HDR *EthernetHdr;
  UINT16        Length;
  UINT16        i;

  EthernetHdr = NULL;
  Length      = 0;
  i           = 0;

  ZeroMem ((CHAR8 *) mPacket, MAX_ETHERNET_SIZE);

  // First copy the source and destination addresses
  EthernetHdr = (ETHERNET_HDR *) mPacket;
  CopyMem ((CHAR8 *) &EthernetHdr->SourceAddr, (CHAR8 *) AdapterInfo->Hw.mac.perm_addr, ETH_ALEN);
  CopyMem ((CHAR8 *) &EthernetHdr->DestAddr, (CHAR8 *) AdapterInfo->BroadcastNodeAddress, ETH_ALEN);

  // Calculate the data segment size and store it in the header big Endian style
  Length                  = TEST_PACKET_SIZE - sizeof (ETHERNET_HDR);
  EthernetHdr->Length[0]  = (UINT8) (Length >> 8);
  EthernetHdr->Length[1]  = (UINT8) Length;

  // Generate mPacket data
  for (i = 0; i < Length; i++) {
    mPacket[i + sizeof (ETHERNET_HDR)] = (UINT8) i;
  }
}

/** Run the PHY loopback test for N iterations.

   This routine transmits a packet, waits a bit, and then checks to see if it was received.
   If any of the packets are not received then it will be interpreted as a failure.

   @param[in]   AdapterInfo      Pointer to the NIC data structure the PHY loopback test will be run on.
   @param[in]   PxeCpbTransmit   Pointer to the packet to transmit.

   @retval   EFI_SUCCESS            All packets were received successfully
   @retval   EFI_DEVICE_ERROR       Transmitting packet failed.
   @retval   EFI_DEVICE_ERROR       Receiving packet failed.
   @retval   EFI_DEVICE_ERROR       Transmitted and received packet data do not match.
   @retval   EFI_OUT_OF_RESOURCES   Couldn't allocate memory for RX/TX buffers.
**/
EFI_STATUS
XgbeUndiRunPhyLoopback (
  IN DRIVER_DATA      *AdapterInfo,
  IN PXE_CPB_TRANSMIT  PxeCpbTransmit
  )
{
  EFI_STATUS      Status = EFI_SUCCESS;
  PXE_DB_RECEIVE  DbReceive;
  PXE_CPB_RECEIVE CpbReceive;
  UINT64          FreeTxBuffer[DEFAULT_TX_DESCRIPTORS];
  UINT32          RxAttempt = 0;

  // This is zeroed in the loop below before first use.
  CpbReceive.BufferAddr = (PXE_UINT64) (UINTN) AllocatePool (RX_BUFFER_SIZE);
  if (CpbReceive.BufferAddr == (PXE_UINT64) (UINTN) NULL) {
    DEBUGPRINTWAIT (DIAG, ("Failed to allocate CpbReceive.BufferAddr!\n"));
    return EFI_OUT_OF_RESOURCES;
  }

  DEBUGPRINT (DIAG, ("CpbReceive.BufferAddr allocated at %x\n", (UINTN) CpbReceive.BufferAddr));
  for (UINT32 i = 0; i < PHY_LOOPBACK_ITERATIONS; i++) {
    ZeroMem ((VOID *) CpbReceive.BufferAddr, RX_BUFFER_SIZE);

    Status = XgbeTransmit (
               AdapterInfo,
               (UINT64) (UINTN) &PxeCpbTransmit,
               PXE_OPFLAGS_TRANSMIT_WHOLE
             );

    if (Status != PXE_STATCODE_SUCCESS) {
      DEBUGPRINT (DIAG, ("ixgbe_Transmit Status %X\n", Status));
      DEBUGWAIT (DIAG);
      DEBUGPRINT (DIAG, ("Transmit error\n"));
      break;
    }

    // Wait a little, then check to see if the packet has arrived
    DEBUGWAIT (DIAG);
    CpbReceive.BufferLen = RX_BUFFER_SIZE;

    for (RxAttempt = 0; RxAttempt <= 100000; RxAttempt++) {
      Status = XgbeReceive (AdapterInfo, &CpbReceive, &DbReceive);
      gBS->Stall (10);
      if (Status == PXE_STATCODE_NO_DATA) {
        continue;
      } else if (Status != PXE_STATCODE_SUCCESS) {
        break;
      }

      //
      // Packets from NCSI may be received even though internal PHY loopback
      // is set.
      // Test for packet we have just sent. If received something else, ignore
      // and continue polling for packets.
      //
      if (CompareMem ((VOID *) (UINTN) CpbReceive.BufferAddr, (VOID *) (UINTN) mPacket, TEST_PACKET_SIZE) == 0) {
        break; // Leave with PXE_STATCODE_SUCCESS
      }
    }

    if (RxAttempt > 100000) {
      DEBUGPRINT (CRITICAL, ("ERROR: Receive timeout on iteration %d\n", RxAttempt));
      Status = EFI_DEVICE_ERROR;
      break;
    } else if (Status != PXE_STATCODE_SUCCESS) {
      DEBUGPRINT (CRITICAL, ("ERROR: Receive failed with status %X\n", Status));
      Status = EFI_DEVICE_ERROR;
      break;
    }

    XgbeFreeTxBuffers (
      AdapterInfo,
      DEFAULT_TX_DESCRIPTORS,
      FreeTxBuffer
    );
  }

  gBS->FreePool ((VOID *) ((UINTN) CpbReceive.BufferAddr));
  return Status;
}

/** Sets up the adapter to run the Phy loopback test and then calls
   the loop which will iterate through the test.

   @param[in]   XgbePrivate   Pointer to adapter data.

   @retval   EFI_SUCCESS            The Phy loopback test passed
   @retval   EFI_DEVICE_ERROR       Phy loopback test failed
   @retval   EFI_INVALID_PARAMETER  Some other error occurred
**/
EFI_STATUS
XgbeUndiPhyLoopback (
  UNDI_PRIVATE_DATA *XgbePrivate
  )
{
  PXE_CPB_TRANSMIT     PxeCpbTransmit;
  EFI_STATUS           Status;
  EFI_STATUS           LoopbackStatus;
  DRIVER_DATA          *AdapterInfo;
  struct ixgbe_hw      *Hw;
  UINTN                i;

  AdapterInfo = &XgbePrivate->NicInfo;
  Hw          = &AdapterInfo->Hw;

  // Uninstall NII protocol.
  // This should make network stack drivers to stop UNDI (including Tx buffer
  // retrieval).
  Status = gBS->UninstallProtocolInterface (
                  XgbePrivate->DeviceHandle,
                  &gEfiNetworkInterfaceIdentifierProtocolGuid_31,
                  &XgbePrivate->NiiProtocol31
                  );
  if (EFI_ERROR (Status)) {
    DEBUGPRINT (CRITICAL, ("Failed to uninstall NII: %r\n", Status));
    return EFI_DEVICE_ERROR;
  }

  DEBUGPRINTWAIT (DIAG, ("NII uninstalled\n"));

  DEBUGPRINT (DIAG, ("XgbePrivate->NicInfo.Block %X\n", (UINTN) XgbePrivate->NicInfo.Block));
  DEBUGPRINT (DIAG, ("XgbePrivate->NicInfo.Block30 %X\n", (UINTN) XgbePrivate->NicInfo.Block30));
  DEBUGPRINT (DIAG, ("XgbePrivate->NicInfo.MapMem %X\n", (UINTN) XgbePrivate->NicInfo.MapMem));
  DEBUGPRINT (DIAG, ("XgbePrivate->NicInfo.Delay30 %X\n", (UINTN) XgbePrivate->NicInfo.Delay30));
  DEBUGPRINT (DIAG, ("XgbePrivate->NicInfo.Delay %X\n", (UINTN) XgbePrivate->NicInfo.Delay));
  DEBUGPRINT (DIAG, ("XgbePrivate->NicInfo.MemIo %X\n", (UINTN) XgbePrivate->NicInfo.MemIo));
  DEBUGPRINT (DIAG, ("XgbePrivate->NicInfo.MemIo30 %X\n", (UINTN) XgbePrivate->NicInfo.MemIo30));
  DEBUGWAIT (DIAG);

  // Adapter is stopped at this point. Need to reinitialize it to enable Tx/Rx.
  ixgbe_reset_hw (Hw);
  AdapterInfo->HwInitialized = FALSE;
  if (XgbeInitialize (AdapterInfo) != PXE_STATCODE_SUCCESS) {
    DEBUGPRINT (CRITICAL, ("Error initializing adapter!\n"));
    LoopbackStatus = EFI_DEVICE_ERROR;
    goto ExitShutdown;
  }

  DEBUGPRINTWAIT (DIAG, ("Adapter initialized\n"));

  // Enable loopback mode on 10GbE
  XgbeSetRegBits (AdapterInfo, IXGBE_HLREG0, IXGBE_HLREG0_LPBK);

  if (Hw->mac.type == ixgbe_mac_82598EB) {
    DEBUGPRINT (DIAG, ("Enable loopback on 82598\n"));
    XgbeSetRegBits (AdapterInfo, IXGBE_AUTOC, IXGBE_AUTOC_FLU);
  }
  else if ((Hw->mac.type == ixgbe_mac_82599EB)) {
    DEBUGPRINT (DIAG, ("Enable loopback on 82599\n"));

    // Set FLU and LMS
    XgbeClearRegBits (AdapterInfo, IXGBE_AUTOC, IXGBE_AUTOC_LMS_MASK);
    XgbeSetRegBits (AdapterInfo, IXGBE_AUTOC, IXGBE_AUTOC_FLU | IXGBE_AUTOC_LMS_10G_LINK_NO_AN);
  } else if (
    (Hw->mac.type == ixgbe_mac_X540) ||
    (Hw->mac.type == ixgbe_mac_X550) ||
    (Hw->mac.type == ixgbe_mac_X550EM_x) ||
    (Hw->mac.type == ixgbe_mac_X550EM_a) ||
    FALSE)
  {
    XgbeSetRegBits (AdapterInfo, IXGBE_MACC, IXGBE_MACC_FLU);
  }

  XgbeSetRegBits (AdapterInfo, IXGBE_FCTRL, IXGBE_FCTRL_BAM);

  // Enable the receive unit
  XgbeReceiveStart (AdapterInfo);
  DelayInMicroseconds (AdapterInfo, 1000 * 100);

  // Build our packet, and send it out the door.
  DEBUGPRINT (DIAG, ("Building Packet\n"));
  _BuildPacket (AdapterInfo);

  PxeCpbTransmit.MediaheaderLen = sizeof (ETHERNET_HDR);
  PxeCpbTransmit.DataLen        = TEST_PACKET_SIZE - sizeof (ETHERNET_HDR);
  PxeCpbTransmit.FrameAddr      = (UINTN) mPacket;
  PxeCpbTransmit.reserved       = 0;
  DEBUGPRINT (DIAG, ("Packet length = %d\n", PxeCpbTransmit.DataLen));
  DEBUGPRINT (DIAG, ("Packet = %X FrameAddr = %X\n", (UINTN) mPacket, PxeCpbTransmit.FrameAddr));
  DEBUGPRINT (DIAG, ("Packet data:\n"));
  for (i = 0; i < 40; i++) {
    DEBUGPRINT (DIAG, ("%d: %x ", i, ((UINT8 *) ((UINTN) PxeCpbTransmit.FrameAddr))[i]));
  }

  DEBUGWAIT (DIAG);

  LoopbackStatus = XgbeUndiRunPhyLoopback (AdapterInfo, PxeCpbTransmit);

  DEBUGPRINT (DIAG, ("PHY Loopback test returns %r\n", LoopbackStatus));

  XgbeReceiveStop (AdapterInfo);
  DEBUGPRINT (DIAG, ("Taking PHY out of loopback mode\n"));

  // Remove loopback mode on 10GbE
  XgbeClearRegBits (AdapterInfo, IXGBE_HLREG0, IXGBE_HLREG0_LPBK);
  XgbeClearRegBits (AdapterInfo, IXGBE_AUTOC, IXGBE_AUTOC_FLU | IXGBE_AUTOC_LMS_MASK);

ExitShutdown:

  // Shutdown the adapter. Make it ready to be reinitialized via UNDI interface.
  XgbeShutdown (AdapterInfo);
  AdapterInfo->HwInitialized = FALSE;

  // Reinstall NII protocol.
  Status = gBS->InstallProtocolInterface (
                  &XgbePrivate->DeviceHandle,
                  &gEfiNetworkInterfaceIdentifierProtocolGuid_31,
                  EFI_NATIVE_INTERFACE,
                  &XgbePrivate->NiiProtocol31
                  );

  if (EFI_ERROR (Status)) {
    DEBUGPRINT (CRITICAL, ("NII installation failed: %r\n", Status));
    goto Exit;
  }

  DEBUGPRINTWAIT (DIAG, ("NII has been reinstalled.\n"));

  // Reattach network stack drivers to the child handle on which NII sits.
  Status = gBS->ConnectController (
                  XgbePrivate->DeviceHandle,
                  NULL,
                  XgbePrivate->Undi32DevPath,
                  FALSE
                  );

  if (EFI_ERROR (Status)) {
    DEBUGPRINTWAIT (CRITICAL, ("Failed to connect controller: %r\n", Status));
  }

  DEBUGPRINTWAIT (DIAG, ("Device handle has been reconnected.\n"));

Exit:
  // Report error if loopback or NII (un)install flow fails
  // (with the exception of EFI_NOT_FOUND, expected in case of SNP not loaded beforehand).
  if (EFI_ERROR (LoopbackStatus)) {
    Status = LoopbackStatus;
  } else if (Status == EFI_NOT_FOUND) {
    Status = EFI_SUCCESS;
  }

  DEBUGPRINTWAIT (DIAG, ("Returning: %r\n", Status));

  return Status;
}

/** Runs diagnostics on a controller.

    @param[in]   This               A pointer to the EFI_DRIVER_DIAGNOSTICS_PROTOCOL instance.
    @param[in]   ControllerHandle   The handle of the controller to run diagnostics on.
    @param[in]   ChildHandle        The handle of the child controller to run diagnostics on
                                    This is an optional parameter that may be NULL.  It will
                                    be NULL for device drivers.  It will also be NULL for a
                                    bus drivers that wish to run diagnostics on the bus
                                    controller.  It will not be NULL for a bus driver that
                                    wishes to run diagnostics on one of its child controllers.
    @param[in]   DiagnosticType     Indicates type of diagnostics to perform on the controller
                                    specified by ControllerHandle and ChildHandle.   See
                                    "Related Definitions" for the list of supported types.
    @param[in]   Language           A pointer to a three character ISO 639-2 language
                                    identifier.  This is the language in which the optional
                                    error message should be returned in Buffer, and it must
                                    match one of the languages specified in SupportedLanguages.
                                    The number of languages supported by a driver is up to
                                    the driver writer.
    @param[out]  ErrorType          A GUID that defines the format of the data returned in
                                    Buffer.
    @param[out]  BufferSize         The size, in bytes, of the data returned in Buffer.
    @param[out]  Buffer             A buffer that contains a Null-terminated Unicode string
                                    plus some additional data whose format is defined by
                                    ErrorType.  Buffer is allocated by this function with
                                    AllocatePool(), and it is the caller's responsibility
                                    to free it with a call to FreePool().

    @retval      EFI_SUCCESS             The controller specified by ControllerHandle and
                                         ChildHandle passed the diagnostic.
    @retval      EFI_INVALID_PARAMETER   ControllerHandle is not a valid EFI_HANDLE.
    @retval      EFI_INVALID_PARAMETER   ChildHandle is not NULL and it is not a valid
                                         EFI_HANDLE.
    @retval      EFI_INVALID_PARAMETER   Language is NULL.
    @retval      EFI_INVALID_PARAMETER   ErrorType is NULL.
    @retval      EFI_INVALID_PARAMETER   BufferType is NULL.
    @retval      EFI_INVALID_PARAMETER   Buffer is NULL.
    @retval      EFI_UNSUPPORTED         The driver specified by This does not support
                                         running diagnostics for the controller specified
                                         by ControllerHandle and ChildHandle.
    @retval      EFI_UNSUPPORTED         The driver specified by This does not support the
                                         type of diagnostic specified by DiagnosticType.
    @retval      EFI_UNSUPPORTED         The driver specified by This does not support the
                                         language specified by Language.
    @retval      EFI_OUT_OF_RESOURCES    There are not enough resources available to complete
                                         the diagnostics.
    @retval      EFI_OUT_OF_RESOURCES    There are not enough resources available to return
                                         the status information in ErrorType, BufferSize,
                                         and Buffer.
    @retval      EFI_DEVICE_ERROR        The controller specified by ControllerHandle and
                                         ChildHandle did not pass the diagnostic.
**/
EFI_STATUS
EFIAPI
XgbeUndiDriverDiagnosticsRunDiagnostics (
  IN EFI_DRIVER_DIAGNOSTICS_PROTOCOL *                                   This,
  IN EFI_HANDLE                                                          ControllerHandle,
  IN EFI_HANDLE                                                          ChildHandle OPTIONAL,
  IN EFI_DRIVER_DIAGNOSTIC_TYPE                                          DiagnosticType,
  IN CHAR8 *                                                             Language,
  OUT EFI_GUID **                                                        ErrorType,
  OUT UINTN *                                                            BufferSize,
  OUT CHAR16 **                                                          Buffer
  )
{
  EFI_DEVICE_PATH_PROTOCOL *UndiDevicePath;
  UNDI_PRIVATE_DATA *       XgbePrivate;
  EFI_NII_POINTER_PROTOCOL *NiiPointerProtocol;
  EFI_STATUS                Status;
  UINT16                    CheckSum;

  Status      = EFI_SUCCESS;
  XgbePrivate = NULL;

  // Validate input parameters

  // Check against invalid NULL parameters
  if (NULL == Language
    || NULL == ErrorType
    || NULL == BufferSize
    || NULL == Buffer
    || NULL == ControllerHandle)
  {
    return EFI_INVALID_PARAMETER;
  }

  // Check against unsupported languages
  if (((&gXgbeUndiDriverDiagnostics == This) &&
    (CompareMem ("eng", Language, 4) != 0))
    || (((EFI_DRIVER_DIAGNOSTICS_PROTOCOL *) &gXgbeUndiDriverDiagnostics2 == This) &&
    (CompareMem ("en-US", Language, 6) != 0)))
  {
    DEBUGPRINT (CRITICAL, ("Driver Diagnostics: Unsupported Language\n"));
    return EFI_UNSUPPORTED;
  }

  // Make sure this driver is currently managing ControllerHandle
  // This satisfies the ControllerHandle validation requirement in scope of
  // detecion of invalid EFI handle
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiDevicePathProtocolGuid,
                  (VOID * *) &UndiDevicePath,
                  gUndiDriverBinding.DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_TEST_PROTOCOL
                );

  if (EFI_ERROR (Status)) {
    DEBUGPRINT (DIAG, (" OpenProtocol Status = %8X\n", Status));
    return Status;
  }

  //  Open an instance for the NiiPointerProtocol protocol so we can check
  //  if the child handle interface is actually supported and calculate the pointer
  //  to XgbePrivate.
  DEBUGPRINT (DIAG, ("Open an instance for the gEfiPro1000Com Protocol\n"));
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiNiiPointerGuid,
                  (VOID * *) &NiiPointerProtocol,
                  gUndiDriverBinding.DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                );

  if (EFI_ERROR (Status)) {
    DEBUGPRINT (CRITICAL, ("OpenProtocol error Status %X\n", Status));
    DEBUGWAIT (CRITICAL);
    return Status;
  }

  XgbePrivate = UNDI_PRIVATE_DATA_FROM_THIS (NiiPointerProtocol->NiiProtocol31);

  // ChildHandle input parameter can be NULL. If it is not NULL we have to validate it.
  if (NULL != ChildHandle) {

    // Make sure this ChildHandle is a valid EFI handle with NII protocol support
    // This satisfies the ChildHandle validation requirement in scope of detecion of invalid EFI handle
    Status = gBS->OpenProtocol (
                    ChildHandle,
                    &gEfiNetworkInterfaceIdentifierProtocolGuid_31,
                    NULL,
                    gUndiDriverBinding.DriverBindingHandle,
                    ControllerHandle,
                    EFI_OPEN_PROTOCOL_TEST_PROTOCOL
                  );

    if (EFI_ERROR (Status)) {
      DEBUGPRINT (DIAG, (" OpenProtocol Status = %8X\n", Status));
      return Status;
    }

    //  Now we know the ChildHandle is a valid EFI handle. Let's check if current ControllerHandle supports ChildHandle
    if (ChildHandle != XgbePrivate->DeviceHandle) {
      DEBUGPRINT (CRITICAL, ("Driver Diagnostics: Unsupported Child handle: %x\n", ChildHandle));
      DEBUGPRINT (CRITICAL, ("XgbePrivate->DeviceHandle: %x\n", XgbePrivate->DeviceHandle));
      return EFI_UNSUPPORTED;
    }
  }

  if (!XgbePrivate->NicInfo.FwSupported) {
    return EFI_UNSUPPORTED;
  }

  switch (DiagnosticType) {
  case EfiDriverDiagnosticTypeStandard:
    if (ixgbe_validate_eeprom_checksum (&XgbePrivate->NicInfo.Hw, &CheckSum) != IXGBE_SUCCESS) {
      Status = EFI_DEVICE_ERROR;
    }
    break;

  case EfiDriverDiagnosticTypeExtended:
    if (XgbePrivate->NicInfo.UndiEnabled
      && XgbePrivate->IsChildInitialized)
    {
      Status = XgbeUndiPhyLoopback (XgbePrivate);
    } else {
      Status = EFI_UNSUPPORTED;
    }
    break;

  case EfiDriverDiagnosticTypeManufacturing:
    DEBUGPRINT (CRITICAL, ("Driver Diagnostics: EfiDriverDiagnosticTypeManufacturing not supported\n"));
    DEBUGWAIT (CRITICAL);
    Status = EFI_UNSUPPORTED;
    break;

  default:
    DEBUGPRINT (DIAG, ("Unsupported diagnostic mode %x\n", DiagnosticType));
    DEBUGWAIT (DIAG);
    Status = EFI_UNSUPPORTED;
    break;
  }

  return Status;
}

/* Protocols structures definition and initialization */

EFI_DRIVER_DIAGNOSTICS_PROTOCOL gXgbeUndiDriverDiagnostics = {
  XgbeUndiDriverDiagnosticsRunDiagnostics,
  "eng"
};

EFI_DRIVER_DIAGNOSTICS2_PROTOCOL gXgbeUndiDriverDiagnostics2 = {
  (EFI_DRIVER_DIAGNOSTICS2_RUN_DIAGNOSTICS) XgbeUndiDriverDiagnosticsRunDiagnostics,
  "en-US"
};
