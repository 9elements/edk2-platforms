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
#include "DriverDiagnostics.h"

// tentative definition needed by RunDiagnostics() function
EFI_DRIVER_DIAGNOSTICS_PROTOCOL    gUndiDriverDiagnostics;

/* Global Variables */

UINT8  mPacket[MAX_ETHERNET_SIZE]; // our loopback test packet

/* Function definitions */

/** Build a packet to transmit in the MAC loopback test.

  @param[in]   AdapterInfo   Pointer to the NIC data structure information
                             which the UNDI driver is layering on so that we can
                             get the MAC address

  @return      Sets the global array Packet[] with the packet to send out during MAC loopback.
**/
VOID
_BuildPacket (
  IN DRIVER_DATA *AdapterInfo
  )
{
  ETHERNET_HDR  *EthernetHdr;
  UINT16        Length;
  UINT16        i;

  EthernetHdr = NULL;
  Length      = 0;
  i           = 0;

  ZeroMem ((CHAR8 *) mPacket, MAX_ETHERNET_SIZE);

  // First copy the source and destination addresses
  EthernetHdr = (ETHERNET_HDR *) mPacket;
  CopyMem (
    (CHAR8 *) &EthernetHdr->SourceAddr,
    (CHAR8 *) AdapterInfo->Hw.port_info[0].mac.perm_addr,
    ETH_ALEN
  );
  CopyMem (
    (CHAR8 *) &EthernetHdr->DestAddr,
    (CHAR8 *) AdapterInfo->BroadcastNodeAddress,
    ETH_ALEN
  );

  // Source address must be different than the station address
  // otherwise packet will be dropped
  EthernetHdr->SourceAddr[5] = 255 - EthernetHdr->SourceAddr[5];

  // Calculate the data segment size and store it in the header Big Endian style
  Length                  = TEST_PACKET_SIZE - sizeof (ETHERNET_HDR);
  EthernetHdr->Length[0]  = (UINT8) (Length >> 8);
  EthernetHdr->Length[1]  = (UINT8) Length;

  // Generate Packet data
  for (i = 0; i < Length; i++) {
    mPacket[i + sizeof (ETHERNET_HDR)] = (UINT8) i;
  }
}

#if (DBG_LVL & DIAG)
/** Helper debug function to display Rx descriptors, and respective packet and
   header addresses

  @param[in]   AdapterInfo   Pointer to the NIC data structure information
                             which the UNDI driver is layering

  @retval      None
**/
VOID
_DisplayBuffersAndDescriptors (
  IN DRIVER_DATA *AdapterInfo
  )
{
  union ice_16byte_rx_desc  *ReceiveDesc;
  UINT32                    j;

  DEBUGDUMP (DIAG, ("Receive Descriptor\n"));
  DEBUGDUMP (DIAG, ("QRX_TAIL=%X ", rd32 (&AdapterInfo->Hw, QRX_TAIL (0))));
  DEBUGDUMP (DIAG, ("RxRing.NextToUse=%X\n", AdapterInfo->Vsi.RxRing.NextToUse));

  for (j = 0; j < AdapterInfo->Vsi.RxRing.Count; j++) {
    ReceiveDesc = ICE_RX_DESC (&AdapterInfo->Vsi.RxRing, j);
    DEBUGDUMP (DIAG, ("QWORD1=%p, ", ReceiveDesc));
    DEBUGDUMP (DIAG, ("QWORD1=%LX, ", ReceiveDesc->read.pkt_addr));
    DEBUGDUMP (DIAG, ("QWORD2=%LX\n", ReceiveDesc->read.hdr_addr));
  }
}
#endif /* (DBG_LVL & DIAG) */

/** Cleans up the receive queue if not empty.

   This routine repeatedly calls IceReceive() until there is no data left in the queue.

   @param[in]   AdapterInfo      Pointer to the NIC data structure.

   @retval      EFI_SUCCESS         Receive queue is cleaned.
   @retval      EFI_OUT_OF_MEMORY   No memory left to allocate Cpb receive buffer.
**/
EFI_STATUS
_CleanUpReceiveQueue (
  IN DRIVER_DATA *AdapterInfo
  )
{
  PXE_CPB_RECEIVE CpbReceive;
  PXE_DB_RECEIVE  DbReceive;
  EFI_STATUS      Status;
  UINTN           PxeStatCode;

  // Wait a little, then check to see if the packet has arrived
  Status = gBS->AllocatePool (
                  EfiBootServicesData,
                  ICE_RXBUFFER_2048,
                  (VOID **) &CpbReceive.BufferAddr
                );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  CpbReceive.BufferLen = ICE_RXBUFFER_2048;

  do {
    PxeStatCode = IceReceive (
                    AdapterInfo,
                    &CpbReceive,
                    &DbReceive
                  );
  } while (PxeStatCode != PXE_STATCODE_NO_DATA);

  gBS->FreePool ((VOID *) ((UINTN) CpbReceive.BufferAddr));

  return Status;
}

/** Run the MAC loopback test for N iterations.

   This routine transmits a packet, waits a bit, and then checks to see if it was received.
   If any of the packets are not received then it will be interpreted as a failure.

   @param[in]   AdapterInfo      Pointer to the NIC data structure the MAC loopback test will be run on.
   @param[in]   PxeCpbTransmit   Pointer to the packet to transmit.


   @retval      EFI_SUCCESS        All packets were received successfully
   @retval      EFI_DEVICE_ERROR   Transmitting packet failed.
   @retval      EFI_DEVICE_ERROR   Receiving packet failed.
   @retval      EFI_DEVICE_ERROR   Transmitted and received packet data do not match.
**/
EFI_STATUS
IceUndiRunMacLoopback (
  IN DRIVER_DATA      *AdapterInfo,
  IN PXE_CPB_TRANSMIT  PxeCpbTransmit
  )
{
  PXE_CPB_RECEIVE CpbReceive;
  PXE_DB_RECEIVE  DbReceive;
  UINT64          FreeTxBuffer[ICE_NUM_TX_RX_DESCRIPTORS];
  UINTN           PxeStatCode;
  EFI_STATUS      Status;
  UINT32          j;
  UINT32          i;

  Status = EFI_SUCCESS;

  DEBUGPRINT (DIAG, ("Running MAC loopback test on MAC port %X, B/D/F %2X:%2X.%X\n",
                     AdapterInfo->PhysicalPortNumber,
                     AdapterInfo->Bus, AdapterInfo->Device, AdapterInfo->Function));

  // Clean up Rx/Tx queues
  _CleanUpReceiveQueue (AdapterInfo);
  IceFreeTxBuffers (
    AdapterInfo,
    ICE_NUM_TX_RX_DESCRIPTORS,
    FreeTxBuffer
  );

  j = 0;
  while (j < MAC_LOOPBACK_ITERATIONS) {
    PxeStatCode = IceTransmit (
                    AdapterInfo,
                    (UINT64) (UINTN) &PxeCpbTransmit,
                    PXE_OPFLAGS_TRANSMIT_WHOLE | PXE_OPFLAGS_TRANSMIT_BLOCK
                  );

    if (PxeStatCode != PXE_STATCODE_SUCCESS) {
      DEBUGPRINT (CRITICAL, ("IceTransmit returned error code %X\n", PxeStatCode));
      DEBUGWAIT (CRITICAL);
      Status = EFI_DEVICE_ERROR;
      break;
    }

    // Wait a little, then check to see if the packet has arrived
    Status = gBS->AllocatePool (
                    EfiBootServicesData,
                    ICE_RXBUFFER_2048,
                    (VOID * *) &CpbReceive.BufferAddr
                  );
    if (EFI_ERROR (Status)) {
      DEBUGPRINT (CRITICAL, ("AllocatePool returned %X\n", Status));
      DEBUGWAIT (CRITICAL);
      break;
    }

    DEBUGPRINT (DIAG, ("CpbReceive.BufferAddr allocated at %x\n", (UINTN) CpbReceive.BufferAddr));
    DEBUGWAIT (DIAG);
    CpbReceive.BufferLen = ICE_RXBUFFER_2048;

    i = 0;
    do {
      PxeStatCode = IceReceive (
                      AdapterInfo,
                      &CpbReceive,
                      &DbReceive
                    );
      gBS->Stall (1);
      i++;
      if (i > 100000) {
        break;
      }
    } while (PxeStatCode == PXE_STATCODE_NO_DATA);

#if (DBG_LVL & DIAG)
    _DisplayBuffersAndDescriptors (AdapterInfo);
#endif /* (DBG_LVL & DIAG) */

    if (PxeStatCode != PXE_STATCODE_SUCCESS) {
      DEBUGPRINT (CRITICAL, ("IceReceive returned error code %X\n", PxeStatCode));
      DEBUGWAIT (CRITICAL);
      Status = EFI_DEVICE_ERROR;
      break;
    }

    if (CompareMem (
          (VOID *) (UINTN) CpbReceive.BufferAddr,
          (VOID *) (UINTN) mPacket, TEST_PACKET_SIZE
        ) == 0)
    {
      DEBUGPRINT (DIAG, ("receive packet verified\n"));
      DEBUGWAIT (DIAG);
      Status = EFI_SUCCESS;
    } else {
      DEBUGPRINT (CRITICAL, ("Lopback test failed - transmited and received packets don't match\n"));
      DEBUGWAIT (CRITICAL);
      Status = EFI_DEVICE_ERROR;
      break;
    }

    IceFreeTxBuffers (
      AdapterInfo,
      ICE_NUM_TX_RX_DESCRIPTORS,
      FreeTxBuffer
    );

    j++;
    gBS->FreePool ((VOID *) ((UINTN) CpbReceive.BufferAddr));
  }

  return Status;
}

/** Sets up the adapter to run the MAC loopback test and then calls
   the loop which will iterate through the test.

   @param[in]   UndiPrivateData   Pointer to adapter data.

   @retval      EFI_SUCCESS             The MAC loopback test passed.
   @retval      EFI_DEVICE_ERROR        MAC loopback test failed
   @retval      EFI_INVALID_PARAMETER   Some other error occured.
**/
EFI_STATUS
IceExecuteMacLoopbackDiagnostics (
  IN UNDI_PRIVATE_DATA *UndiPrivateData
  )
{
  PXE_CPB_TRANSMIT        PxeCpbTransmit;
  EFI_STATUS              DiagnosticsStatus = EFI_DEVICE_ERROR;
  UINTN                   i;
  EFI_STATUS              Status;
  enum ice_status IceStatus = ICE_SUCCESS;

  // Uninstall NII protocol.
  // This should make network stack drivers to stop UNDI (including Tx buffer
  // retrieval).
  Status = gBS->UninstallProtocolInterface (
                  UndiPrivateData->DeviceHandle,
                  &gEfiNetworkInterfaceIdentifierProtocolGuid_31,
                  &UndiPrivateData->NiiProtocol31
                  );
  if (EFI_ERROR (Status)) {
    DEBUGPRINT (CRITICAL | DIAG, ("Failed to uninstall NII: %r\n", Status));
    goto Exit;
  }
  DEBUGPRINT (DIAG, ("NII uninstalled\n"));

  // Initialize and start the UNDI driver if it has not already been done
  if (IceInitialize (&UndiPrivateData->NicInfo) != PXE_STATCODE_SUCCESS) {
    DEBUGPRINT (CRITICAL, ("IceInitialize returned error\n"));
    goto ExitShutdown;
  }

  UndiPrivateData->NicInfo.DriverBusy = TRUE;

  DEBUGPRINT (DIAG, ("Calling function ice_aq_set_mac_loopback\n"));
  // Enable MAC loopback mode
  IceStatus = ice_aq_set_mac_loopback (&UndiPrivateData->NicInfo.Hw, TRUE, NULL);
  if (IceStatus != ICE_SUCCESS) {
    DEBUGPRINT (CRITICAL, ("ice_aq_set_mac_loopback returned %r\n", IceStatus));
    goto ExitShutdown;
  }

  // Build our packet, and send it out the door.
  DEBUGPRINT (DIAG, ("Building Packet\n"));
  _BuildPacket (&UndiPrivateData->NicInfo);

  PxeCpbTransmit.MediaheaderLen = sizeof (ETHERNET_HDR);
  PxeCpbTransmit.DataLen        = TEST_PACKET_SIZE - sizeof (ETHERNET_HDR);
  PxeCpbTransmit.FrameAddr      = (UINTN) mPacket;
  PxeCpbTransmit.reserved       = 0;
  DEBUGPRINT (DIAG, ("Packet length = %d\n", PxeCpbTransmit.DataLen));
  DEBUGPRINT (DIAG, ("Packet = %X FrameAddr = %X\n", (UINTN) mPacket, PxeCpbTransmit.FrameAddr));
  DEBUGPRINT (DIAG, ("Packet data:\n"));
  for (i = 0; i < 40; i++) {
    DEBUGDUMP (DIAG, ("%d: %x ", i, ((UINT8 *) ((UINTN) PxeCpbTransmit.FrameAddr))[i]));
  }

  DEBUGWAIT (DIAG);

  DiagnosticsStatus = IceUndiRunMacLoopback (&UndiPrivateData->NicInfo, PxeCpbTransmit);
  DEBUGPRINT (DIAG, ("MAC Loopback test returns %r\n", DiagnosticsStatus));

  // Disable MAC loopback mode
  IceStatus = ice_aq_set_mac_loopback(&UndiPrivateData->NicInfo.Hw, FALSE, NULL);
  if (IceStatus != ICE_SUCCESS) {
    DEBUGPRINT (CRITICAL, ("ice_aq_set_mac_loopback returned %r\n", IceStatus));
    DiagnosticsStatus = EFI_DEVICE_ERROR;
  }

ExitShutdown:
  UndiPrivateData->NicInfo.DriverBusy = FALSE;
  // Reinstall NII protocol.
  Status = gBS->InstallProtocolInterface (
                  &UndiPrivateData->DeviceHandle,
                  &gEfiNetworkInterfaceIdentifierProtocolGuid_31,
                  EFI_NATIVE_INTERFACE,
                  &UndiPrivateData->NiiProtocol31
                  );
  if (EFI_ERROR (Status)) {
    DEBUGPRINT (CRITICAL | DIAG, ("NII installation failed: %r\n", Status));
    goto Exit;
  }
  DEBUGPRINT (DIAG, ("NII has been reinstalled.\n"));

  // Reattach network stack drivers to the child handle on which NII sits.
  Status = gBS->ConnectController (
                  UndiPrivateData->DeviceHandle,
                  NULL,
                  UndiPrivateData->Undi32DevPath,
                  FALSE
                  );
  if (EFI_ERROR (Status)) {
    DEBUGPRINTWAIT (CRITICAL, ("Failed to connect controller: %r\n", Status));
  }
  DEBUGPRINTWAIT (DIAG, ("Device handle has been reconnected.\n"));

Exit:
  // Report error if loopback or NII (un)install flow fails
  // (with the exception of EFI_NOT_FOUND, expected in case of SNP not loaded beforehand).
  if (EFI_ERROR (DiagnosticsStatus)) {
    Status = DiagnosticsStatus;
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
IceUndiDriverDiagnosticsRunDiagnostics (
  IN EFI_DRIVER_DIAGNOSTICS_PROTOCOL                         *This,
  IN EFI_HANDLE                                              ControllerHandle,
  IN EFI_HANDLE                                              ChildHandle OPTIONAL,
  IN EFI_DRIVER_DIAGNOSTIC_TYPE                              DiagnosticType,
  IN CHAR8                                                   *Language,
  OUT EFI_GUID                                               **ErrorType,
  OUT UINTN                                                  *BufferSize,
  OUT CHAR16                                                 **Buffer
  )
{
  EFI_DEVICE_PATH_PROTOCOL  *UndiDevicePath;
  UNDI_PRIVATE_DATA         *UndiPrivateData;
  EFI_NII_POINTER_PROTOCOL  *NiiPointerProtocol;
  EFI_STATUS                Status;
  CHAR8                     *SupportedLanguages;
  BOOLEAN                   Iso639Language;
  BOOLEAN                   Found;
  UINTN                     Index;

  Status = EFI_SUCCESS;
  UndiPrivateData = NULL;

  // Validate input parameters

  // Check against invalid NULL parameters
  if ((NULL == Language)
    || (NULL == ErrorType)
    || (NULL == BufferSize)
    || (NULL == Buffer)
    || (NULL == ControllerHandle))
  {
    return EFI_INVALID_PARAMETER;
  }

  // The following implementation is taken from UDK2014
  SupportedLanguages = This->SupportedLanguages;
  Iso639Language = (BOOLEAN) (This == &gUndiDriverDiagnostics);

  // Make sure Language is in the set of Supported Languages
  Found = FALSE;

  while (*SupportedLanguages != 0) {
    if (Iso639Language) {
      if (CompareMem (Language, SupportedLanguages, 3) == 0) {
        Found = TRUE;
        break;
      }
      SupportedLanguages += 3;
    } else {
      for (Index = 0; SupportedLanguages[Index] != 0
        && SupportedLanguages[Index] != ';'; Index++)
      {
        ;
      }
      if ((AsciiStrnCmp (SupportedLanguages, Language, Index) == 0)
        && (Language[Index] == 0))
      {
        Found = TRUE;
        break;
      }
      SupportedLanguages += Index;
      for (; *SupportedLanguages != 0
        && *SupportedLanguages == ';'; SupportedLanguages++)
      {
        ;
      }
    }
  }

  // If Language is not a member of SupportedLanguages, then return EFI_UNSUPPORTED
  if (!Found) {
    DEBUGPRINT (CRITICAL, ("Driver Diagnostics: Unsupported Language: %a\n", Language));
    return EFI_UNSUPPORTED;
  }

  // Make sure this driver is currently managing ControllerHandle
  // This satisfies the ControllerHandle validation requirement in scope of detection of invalid EFI handle
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

  //  Open an instance for the gEfiPro1000Comp protocol so we can check
  //  if the child handle interface is actually supported and calculate the pointer to GigUndiPrivateData.
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

  UndiPrivateData = UNDI_PRIVATE_DATA_FROM_THIS (NiiPointerProtocol->NiiProtocol31);

  // ChildHandle input parameter can be NULL. If it is not NULL we have to validate it.
  if (ChildHandle != NULL) {

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
      DEBUGPRINT (DIAG, (" OpenProtocol returned %r\n", Status));
      return Status;
    }

    // Now we know the ChildHandle is a valid EFI handle.
    // Let's check if current ControllerHandle supports ChildHandle
    if (ChildHandle != UndiPrivateData->DeviceHandle) {
      DEBUGPRINT (CRITICAL, ("Driver Diagnostics: Unsupported Child handle: %x\n", ChildHandle));
      DEBUGPRINT (CRITICAL, ("XgbePrivate->DeviceHandle: %x\n", UndiPrivateData->DeviceHandle));
      return EFI_UNSUPPORTED;
    }
  }

  if (!UndiPrivateData->NicInfo.FwSupported) {
    return EFI_UNSUPPORTED;
  }

  // Perform required type of diagnostics
  switch (DiagnosticType) {
  case EfiDriverDiagnosticTypeStandard:
    Status = ice_nvm_validate_checksum(&UndiPrivateData->NicInfo.Hw);
    if (Status != ICE_SUCCESS) {
      return EFI_DEVICE_ERROR;
    }
    break;
  case EfiDriverDiagnosticTypeExtended:
    if ((UndiPrivateData->NicInfo.UndiEnabled)
      && (UndiPrivateData->IsChildInitialized)
      )
    {
      Status = IceExecuteMacLoopbackDiagnostics (UndiPrivateData);
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

// Diagnostics protocols structures definition and initialization

EFI_DRIVER_DIAGNOSTICS_PROTOCOL gUndiDriverDiagnostics = {
  IceUndiDriverDiagnosticsRunDiagnostics,
  "eng"
};

EFI_DRIVER_DIAGNOSTICS2_PROTOCOL gUndiDriverDiagnostics2 = {
  (EFI_DRIVER_DIAGNOSTICS2_RUN_DIAGNOSTICS) IceUndiDriverDiagnosticsRunDiagnostics,
  "en-US"
};
