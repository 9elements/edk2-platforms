/**************************************************************************

Copyright (c) 2016 - 2023, Intel Corporation. All Rights Reserved.

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
#include "EepromConfig.h"
#include "CommonDriver.h"
#include "ice_dcb.h"

#include "Hii/Hii.h"

#include "LinkTopology.h"
#include "PortOptions.h"

/* Function definitions */

/** Write SR buffer using shared code implementation.

   @param[in]   AdapterInfo    Points to the driver information
   @param[in]   ModulePointer  Pointer to module in words with respect to NVM beginning
   @param[in]   Offset         offset in words from module start
   @param[in]   Words          Number of words to write
   @param[in]   Data           Pointer to location with data to be written

   @retval    EFI_SUCCESS        Buffer successfully written
   @retval    EFI_ACCESS_DENIED  Access to desired NVM memory range is denied
   @retval    EFI_DEVICE_ERROR   Failed to write buffer
   @retval    EFI_DEVICE_ERROR   Waiting for ARQ response timeout
**/
EFI_STATUS
IceWriteNvmBuffer (
  IN DRIVER_DATA *AdapterInfo,
  IN UINT8        ModulePointer,
  IN UINT32       Offset,
  IN UINT16       Words,
  IN VOID        *Data
  )
{
  enum ice_status            IceStatus = ICE_SUCCESS;
  EFI_STATUS                 Status = EFI_SUCCESS;

  Status = ClearAdminReceiveQueue (AdapterInfo);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  IceStatus = ice_acquire_nvm (&AdapterInfo->Hw, ICE_RES_WRITE);
  if (IceStatus != ICE_SUCCESS) {
    DEBUGPRINT (CRITICAL, ("ice_acquire_nvm returned %d\n", IceStatus));
    return EFI_DEVICE_ERROR;
  }

  IceStatus = __ice_write_sr_buf (
                &AdapterInfo->Hw,
                ModulePointer + Offset,
                Words,
                (UINT16 *) Data
              );

  ice_release_nvm (&AdapterInfo->Hw);

  if (IceStatus !=  ICE_SUCCESS) {
    DEBUGPRINT (
      CRITICAL, ("__ice_write_sr_buf (%d, %d, %x) returned: status %d, asq stat %d\n",
      ModulePointer + Offset, Words, *((UINT16 *) Data), IceStatus, AdapterInfo->Hw.adminq.sq_last_status)
    );

    if (AdapterInfo->Hw.adminq.sq_last_status == ICE_AQ_RC_EPERM) {

      // Need to detect attempts to write RO area
      DEBUGPRINT (IMAGE, ("__ice_write_sr_buf returned EPERM\n"));
      return EFI_ACCESS_DENIED;
    } else {
      return EFI_DEVICE_ERROR;
    }
  }

  DEBUGPRINT (WOL, ("__ice_write_sr_buf returned ICE_SUCCESS\n"));

  Status = AwaitReceiveQueueEvent (
             AdapterInfo,
             ice_aqc_opc_nvm_write,
             1000
           );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return EFI_SUCCESS;
}

/** Writes data buffer to nvm using IceWriteNvmBuffer shared code function.

   Function works around the situation when the buffer spreads over two sectors.
   The entire buffer must be located inside the Shared RAM.

   @param[in]   AdapterInfo   Points to the driver information
   @param[in]   Offset        Buffer offset from the start of NVM
   @param[in]   Words         Number of words to write
   @param[in]   Data          Pointer to location with data to be written

   @retval   EFI_SUCCESS       NVM buffer written successfully
   @retval   EFI_DEVICE_ERROR  Failed to write buffer (or either of the sectors)
**/
EFI_STATUS
IceWriteNvmBufferExt (
  IN DRIVER_DATA *AdapterInfo,
  IN UINT32       Offset,
  IN UINT16       Words,
  IN VOID        *Data
  )
{
  UINT16     SectorStart;
  UINT16     Margin;
  UINT16     Words1;
  UINT16     Words2;
  EFI_STATUS Status;

  DEBUGFUNC ("__ice_write_nvm_buffer");

  // Check if the buffer spreads over two sectors. Then we need to split
  // the buffer into two adjacent buffers, one for each sector and write them separatelly.
  SectorStart = (Offset / ICE_SR_SECTOR_SIZE_IN_WORDS) * ICE_SR_SECTOR_SIZE_IN_WORDS;
  Margin = (SectorStart + ICE_SR_SECTOR_SIZE_IN_WORDS) - Offset;
  if (Words > Margin) {
    Words1 = Margin;
    Words2 = Words - Margin;
  } else {
    Words1 = Words;
    Words2 = 0;
  }

  Status = IceWriteNvmBuffer (AdapterInfo, 0, Offset, Words1, Data);
  if (EFI_ERROR (Status)) {
    DEBUGPRINT (CRITICAL, ("IceWriteNvmBuffer(%x) returned %r\n", Offset, Status));
    return EFI_DEVICE_ERROR;
  }
  if (Words2 > 0) {

    // Write the remaining part of the input data to the second sector
    Status = IceWriteNvmBuffer (
               AdapterInfo,
               0,
               SectorStart + ICE_SR_SECTOR_SIZE_IN_WORDS,
               Words2,
               (UINT16 *) Data + Words1
             );
    if (EFI_ERROR (Status)) {
      DEBUGPRINT (CRITICAL, ("IceWriteNvmBuffer returned %r\n", Status));
      return EFI_DEVICE_ERROR;
    }
  }
  return EFI_SUCCESS;
}

/** Notify FW of change in alternate RAM structures.

   @param[in]   UndiPrivateData   Pointer to driver private data structure

   @retval   EFI_SUCCESS       Successfully notified of changes
   @retval   EFI_DEVICE_ERROR  Failed to notify
**/
EFI_STATUS
ApplyAlternateSettings (
  IN  UNDI_PRIVATE_DATA  *UndiPrivateData
  )
{
  enum ice_status  IceStatus;
  BOOLEAN          ResetNeeded = FALSE;

  ASSERT (UndiPrivateData != NULL);
  IceStatus = ice_aq_alternate_write_done (&UndiPrivateData->NicInfo.Hw, ALTRAM_BIOS_MODE, &ResetNeeded);
  IF_SCERR_RETURN (IceStatus, EFI_DEVICE_ERROR);

  return EFI_SUCCESS;
}

/** Gets factory MAC addresses for PF0.

   @param[in]   UndiPrivateData      Pointer to driver private data structure
   @param[out]  FactoryMacAddress    Pointer to buffer for resulting factory
                                     MAC address

   @retval      EFI_SUCCESS       MAC addresses read successfully
   @retval      !EFI_SUCCESS      Failure of underlying function
**/
EFI_STATUS
GetFactoryMacAddressForPf0 (
  IN   UNDI_PRIVATE_DATA  *UndiPrivateData,
  OUT  UINT8              *FactoryMacAddress
  )
{
  return GetFactoryMacAddressForPf (UndiPrivateData, 0, FactoryMacAddress);
}

/** Reads factory default MAC address for specified PF.

   @param[in]   UndiPrivateData      Pointer to driver private data structure
   @param[in]   PhysicalFunction     Number of PF to read the MAC Addresses of
   @param[out]  FactoryMacAddress    Factory default MAC address of the adapter

   @retval      EFI_SUCCESS    MAC addresses read successfully
   @retval      !EFI_SUCCESS   Failed to read PF_MAC_ADDRESS_MODULE TLV
**/
EFI_STATUS
GetFactoryMacAddressForPf (
  IN  UNDI_PRIVATE_DATA  *UndiPrivateData,
  IN  UINT8              PhysicalFunction,
  OUT UINT8              *FactoryMacAddress
  )
{
  EFI_STATUS      Status;
  UINT16          PermMacBuff[PF_MAC_IN_TLV_LEN_WORDS];
  UINT16          PfOffset = SKIP_TLV_LENGTH + (PF_MAC_IN_TLV_LEN_WORDS * PhysicalFunction);

  ASSERT_IF_NULL2 (UndiPrivateData, FactoryMacAddress);

  Status = ReadTlv (UndiPrivateData, PF_MAC_ADDRESS_MODULE, PfOffset, sizeof (PermMacBuff), &PermMacBuff);
  IF_RETURN (EFI_ERROR (Status), Status);

  CopyMem (FactoryMacAddress, &PermMacBuff, ETH_ALEN);

  return EFI_SUCCESS;
}

/** Reads factory default MAC address.

   @param[in]   UndiPrivateData      Pointer to driver private data structure
   @param[out]  FactoryMacAddress    Factory default MAC address of the adapter

   @retval      EFI_SUCCESS     MAC addresses read successfully
   @retval      !EFI_SUCCESS    Failure of underlying function
**/
EFI_STATUS
GetFactoryMacAddress (
  IN   UNDI_PRIVATE_DATA  *UndiPrivateData,
  OUT  UINT8              *FactoryMacAddress
  )
{
  ASSERT (UndiPrivateData != NULL);
  return GetFactoryMacAddressForPf (
           UndiPrivateData,
           UndiPrivateData->NicInfo.Hw.pf_id,
           FactoryMacAddress
           );
}

/** Gets alternate MAC address of currently managed PF.

   @param[in]   UndiPrivateData      Pointer to driver private data structure
   @param[out]  AlternateMacAddress  Pointer to buffer for resulting alternate
                                     MAC address

   @retval      EFI_SUCCESS       MAC addresses read successfully
   @retval      EFI_DEVICE_ERROR  Failed to read alternate MAC addr from Alt. RAM
**/
EFI_STATUS
GetAlternateMacAddress (
  IN   UNDI_PRIVATE_DATA  *UndiPrivateData,
  OUT  UINT8              *AlternateMacAddress
  )
{
  enum ice_status  IceStatus;
  UINT32           AltRamBuffer[2];

  ASSERT_IF_NULL2 (UndiPrivateData, AlternateMacAddress);
  IceStatus = ice_aq_alternate_read (
                &UndiPrivateData->NicInfo.Hw,
                ICE_ALT_RAM_LAN_MAC_ADDRESS_LOW (UndiPrivateData->NicInfo.Hw.pf_id),
                &AltRamBuffer[0],
                ICE_ALT_RAM_LAN_MAC_ADDRESS_HIGH (UndiPrivateData->NicInfo.Hw.pf_id),
                &AltRamBuffer[1]
                );
  IF_SCERR_RETURN (IceStatus, EFI_DEVICE_ERROR);

  // Check if this Alternate RAM entry is valid and then use it,
  // otherwise return zeros
  if ((AltRamBuffer[1] & ALT_RAM_VALID_PARAM_BIT_MASK) != 0) {
    ((UINT16 *)AlternateMacAddress)[0] = SwapBytes16 ((UINT16) (AltRamBuffer[1] & 0x0000FFFF));
    ((UINT16 *)AlternateMacAddress)[1] = SwapBytes16 ((UINT16) ((AltRamBuffer[0] & 0xFFFF0000) >> 16));
    ((UINT16 *)AlternateMacAddress)[2] = SwapBytes16 ((UINT16) (AltRamBuffer[0] & 0x0000FFFF));
  } else {
    ZeroMem (AlternateMacAddress, ETH_ALEN);
  }

  return EFI_SUCCESS;
}

/** Sets alternate MAC address for currently managed PF.

   @param[in]   UndiPrivateData      Pointer to driver private data structure
   @param[in]   AlternateMacAddress  Value to set the MAC address to

   @retval      EFI_SUCCESS       New MAC address set successfully
   @retval      EFI_DEVICE_ERROR  Failed to write new MAC value to alt. RAM
**/
EFI_STATUS
SetAlternateMacAddress (
  IN  UNDI_PRIVATE_DATA  *UndiPrivateData,
  IN  UINT8              *AlternateMacAddress
  )
{
  enum ice_status  IceStatus;
  UINT32           AltRamBuffer[2];

  ASSERT_IF_NULL2 (UndiPrivateData, AlternateMacAddress);
  AltRamBuffer[0] = SwapBytes16 (((UINT16 *)AlternateMacAddress)[2]) +
                    ((UINT32) SwapBytes16 (((UINT16 *)AlternateMacAddress)[1]) << 16);
  AltRamBuffer[1] = SwapBytes16 (((UINT16 *)AlternateMacAddress)[0]);

  AltRamBuffer[1] |= ALT_RAM_VALID_PARAM_BIT_MASK;

  IceStatus = ice_aq_alternate_write (
                &UndiPrivateData->NicInfo.Hw,
                ICE_ALT_RAM_LAN_MAC_ADDRESS_LOW (UndiPrivateData->NicInfo.Hw.pf_id),
                AltRamBuffer[0],
                ICE_ALT_RAM_LAN_MAC_ADDRESS_HIGH (UndiPrivateData->NicInfo.Hw.pf_id),
                AltRamBuffer[1]
                );
  IF_SCERR_RETURN (IceStatus, EFI_DEVICE_ERROR);

  UndiPrivateData->HiiInfo.EmprRequired = TRUE;
  return EFI_SUCCESS;
}

/** Restores the factory default MAC address for currently managed PF.

   @param[in]   UndiPrivateData   Pointer to driver private data structure

   @retval      EFI_SUCCESS       New MAC address set successfully
   @retval      EFI_DEVICE_ERROR  Failed to invalidate Alternate RAM entry
**/
EFI_STATUS
RestoreDefaultMacAddress (
  IN  UNDI_PRIVATE_DATA  *UndiPrivateData
  )
{
  enum ice_status  IceStatus;
  UINT32           AltRamBuffer[2];

  // Invalidate Alternate RAM entry by writing zeros
  AltRamBuffer[0] = 0;
  AltRamBuffer[1] = 0;

  ASSERT (UndiPrivateData != NULL);
  IceStatus = ice_aq_alternate_write (
                &UndiPrivateData->NicInfo.Hw,
                ICE_ALT_RAM_LAN_MAC_ADDRESS_LOW (UndiPrivateData->NicInfo.Hw.pf_id),
                AltRamBuffer[0],
                ICE_ALT_RAM_LAN_MAC_ADDRESS_HIGH (UndiPrivateData->NicInfo.Hw.pf_id),
                AltRamBuffer[1]
                );
  IF_SCERR_RETURN (IceStatus, EFI_DEVICE_ERROR);

  UndiPrivateData->HiiInfo.EmprRequired = TRUE;
  return EFI_SUCCESS;
}

/** Returns EEPROM capabilities word (0x33) for current adapter

   @param[in]   UndiPrivateData    Points to the driver instance private data
   @param[out]  CapabilitiesWord   EEPROM capabilities word (0x33) for current adapter

   @retval   EFI_SUCCESS       Capabilities word successfully read
   @retval   EFI_DEVICE_ERROR  Failed to read capabilities word
**/
EFI_STATUS
EepromGetCapabilitiesWord (
  IN  UNDI_PRIVATE_DATA *UndiPrivateData,
  OUT UINT16            *CapabilitiesWord
  )
{
  enum ice_status IceStatus;
  UINT16          Word;

  IceStatus = ice_read_sr_word (
                &UndiPrivateData->NicInfo.Hw,
                EEPROM_CAPABILITIES_WORD,
                &Word
              );
  if (IceStatus != ICE_SUCCESS) {
    return EFI_DEVICE_ERROR;
  }

  Word &= ~EEPROM_CAPABILITIES_SIG;
  *CapabilitiesWord = Word;

  return EFI_SUCCESS;
}

/** Updates NVM checksum.

   @param[in]   UndiPrivateData   Pointer to driver private data structure

   @retval      EFI_SUCCESS       Checksum successfully updated
   @retval      EFI_DEVICE_ERROR  Failed to acquire NVM
   @retval      EFI_DEVICE_ERROR  Failed to update NVM checksum
   @retval      !EFI_SUCCESS      Failure of underlying function
**/
EFI_STATUS
UpdateNvmChecksum (
  IN  UNDI_PRIVATE_DATA  *UndiPrivateData
  )
{
  enum ice_status IceStatus;
  EFI_STATUS      Status;
  struct ice_hw   *Hw;
  UINT16          CmdFlags;

  ASSERT (UndiPrivateData != NULL);
  Hw = &UndiPrivateData->NicInfo.Hw;

  Status = ClearAdminReceiveQueue (&UndiPrivateData->NicInfo);
  IF_RETURN (EFI_ERROR (Status), Status);

  // Software takes ownership over the NVM resource for activate
  IceStatus = ice_acquire_nvm (Hw, ICE_RES_WRITE);
  IF_SCERR_RETURN (IceStatus, EFI_DEVICE_ERROR);

  // Determine whether EMPR is requested instead of the next PCIR
  // and set cmd_flags of AQ command accordingly
  CmdFlags = UndiPrivateData->HiiInfo.EmprRequired ? ICE_AQC_NVM_ACTIV_REQ_EMPR : 0;
  UndiPrivateData->HiiInfo.EmprRequired = FALSE;

  // Run NVM Activate command with no banks specified to perform SR dump
  IceStatus = ice_nvm_write_activate (Hw, CmdFlags, NULL);

  if (IceStatus != ICE_SUCCESS) {
    DEBUGPRINT (CRITICAL, ("ice_nvm_write_activate returned %d\n", IceStatus));
    Status = EFI_DEVICE_ERROR;
    goto ExitError;
  }

  Status = AwaitReceiveQueueEvent (
             &UndiPrivateData->NicInfo,
             ice_aqc_opc_nvm_write_activate,
             NVM_OPERATION_TIMEOUT_IN_1MS_UNITS
             );

ExitError:
  // Release NVM ownership
  ice_release_nvm (Hw);
  return Status;
}

/** Reads PBA string from NVM.

   @param[in]   UndiPrivateData  Pointer to driver private data structure
   @param[out]  PbaNumberStr     Output string buffer for PBA string

   @retval   EFI_SUCCESS            PBA string successfully read
   @retval   EFI_SUCCESS            PBA string is unsupported by the adapter
   @retval   EFI_DEVICE_ERROR       Failure of underlying shared code function
**/
EFI_STATUS
GetPbaStr (
  IN  UNDI_PRIVATE_DATA  *UndiPrivateData,
  OUT EFI_STRING         PbaNumberStr
  )
{
  enum ice_status  IceStatus;
  CHAR8            PbaStringAscii[MAX_PBA_STR_LENGTH];

  IceStatus = ice_read_pba_string (&UndiPrivateData->NicInfo.Hw, PbaStringAscii, MAX_PBA_STR_LENGTH);
  if (IceStatus == ICE_ERR_DOES_NOT_EXIST) {
    UnicodeSPrint (PbaNumberStr, HII_MAX_STR_LEN_BYTES, L"N/A");
  } else {
    IF_SCERR_RETURN (IceStatus, EFI_DEVICE_ERROR);

    UnicodeSPrint (PbaNumberStr, HII_MAX_STR_LEN_BYTES, L"%a", PbaStringAscii);
  }

  return EFI_SUCCESS;
}


/** Checks if FW LLDP Agent status is supported.

   @param[in]   UndiPrivateData  Pointer to driver private data structure
   @param[out]  Supported        Tells whether LLDP Agent is supported

   @retval      EFI_SUCCESS  LLDP Agent is supported.
**/
EFI_STATUS
IsLldpAgentSupported (
  IN   UNDI_PRIVATE_DATA  *UndiPrivateData,
  OUT  BOOLEAN            *Supported
  )
{
  *Supported = TRUE;
  return EFI_SUCCESS;
}

/** Get current LLDP Admin Status per Lan Port.

   @param[in]   PortNumber  LAN port number for which conversion is done.
   @param[in]   RawToRead   Variable which we use to convert from.

   @retval      LLDP Admin Status for selected port.
**/
UINT8
GetLLDPAdminForPort (
  IN   UINT8    PortNumber,
  IN   UINT32   RawToRead
  )
{
  return (UINT8) ((RawToRead >> PortNumber * 4) & 0xF);
}

/** Reads current FW LLDP Agent status.

   @param[in]   UndiPrivateData   Pointer to driver private data structure
   @param[out]  LldpAgentEna      Pointer to variable which will store read LLDP Admin status

   @retval      EFI_SUCCESS            LLDP Agent status read successfully.
   @retval      EFI_DEVICE_ERROR       Failed to read LLDP Agent status.
   @retval      EFI_DEVICE_ERROR       Out of range value read from NVM.
**/
EFI_STATUS
GetLldpAgentStatus (
  IN   UNDI_PRIVATE_DATA  *UndiPrivateData,
  OUT  BOOLEAN            *LldpAgentEna
  )
{
  EFI_STATUS           EfiStatus;
  UINT32               CurrentLLDP;
  UINT8                PhysicalPortNr;
  UINT8                LldpAdminStatus;

  PhysicalPortNr = UndiPrivateData->NicInfo.PhysicalPortNumber;

  EfiStatus = ReadTlv (
                UndiPrivateData,
                TLV_ID_CURRENT_LLDP,
                SKIP_TLV_LENGTH,
                sizeof (CurrentLLDP),
                &CurrentLLDP
              );
  IF_RETURN (EFI_ERROR (EfiStatus), EfiStatus);

  // Extract value for current LAN port
  LldpAdminStatus = GetLLDPAdminForPort (PhysicalPortNr, CurrentLLDP);

  // If LLDP Admin Status is invalid = 0xF then read default value
  if (LldpAdminStatus == 0xF) {
    return GetDfltLldpAgentStatus (UndiPrivateData, LldpAgentEna);
  } else if (LldpAdminStatus > 3) {
    DEBUGPRINT (CRITICAL, ("Wrong LLDP value read from NVM\n"));
    return EFI_DEVICE_ERROR;
  }

  *LldpAgentEna = (LldpAdminStatus != 0);

  return EFI_SUCCESS;
}

/** Sets FW LLDP Agent status.

   @param[in]   UndiPrivateData  Pointer to driver private data structure
   @param[in]   LldpAgentEna     Requested LLDP Agent status

   @retval      EFI_SUCCESS         LLDP Agent Status written successfully
   @retval      EFI_DEVICE_ERROR    Failed to read current LLDP Agent status
   @retval      EFI_SUCCESS         Requested LLDP agent status matches current
   @retval      EFI_DEVICE_ERROR    Failed to start or stop LLDP Agent
   @retval      EFI_DEVICE_ERROR    Failed to set DCB parameters
**/
EFI_STATUS
SetLldpAgentStatus (
  IN  UNDI_PRIVATE_DATA  *UndiPrivateData,
  IN  BOOLEAN            *LldpAgentEna
  )
{
  enum ice_status_code  IceStatus;
  EFI_STATUS            Status;
  BOOLEAN               CurrentLldpAgentEna;

  Status = GetLldpAgentStatus (UndiPrivateData, &CurrentLldpAgentEna);
  IF_RETURN (EFI_ERROR (Status), Status);

  DEBUGPRINT (HII, ("New LLDP agent %d current %d\n", *LldpAgentEna, CurrentLldpAgentEna));
  DEBUGWAIT (HII);

  if (CurrentLldpAgentEna == *LldpAgentEna) {
    return EFI_SUCCESS;
  }

  if (*LldpAgentEna) {
    IceStatus = ice_aq_start_lldp (&UndiPrivateData->NicInfo.Hw, TRUE, NULL);
    IF_SCERR_RETURN (IceStatus, EFI_DEVICE_ERROR);
  } else {
    IceStatus = ice_aq_stop_lldp (&UndiPrivateData->NicInfo.Hw, TRUE, TRUE, NULL);
    IF_SCERR_RETURN (IceStatus, EFI_DEVICE_ERROR);

    IceStatus = ice_aq_set_dcb_parameters (&UndiPrivateData->NicInfo.Hw, TRUE, NULL);
    IF_SCERR_RETURN (IceStatus, EFI_DEVICE_ERROR);
  }

  return EFI_SUCCESS;
}

/** Read default LLDP Admin Status.

   @param[in]   UndiPrivateData  Pointer to driver private data structure.
   @param[in]   OffsetInModule  Offset in words from module beginning to default LLDP Admin Status
   @param[out]  DefaultValue  Pointer to variable which should store default value for LLDP Agent.

   @retval      EFI_SUCCESS   LLDP Admin get default/restore successful.
   @retval      EFI_DEVICE_ERROR  Failed to get default/restore of LLDP Admin.
**/
EFI_STATUS
ReadDefaultLLDPAdminStatus (
  IN  UNDI_PRIVATE_DATA *UndiPrivateData,
  IN  UINT16             OffsetInModule,
  OUT UINT16            *DefaultValue
  )
{
#define DEFAULT_LLDP_MODULE_TYPE_ID  0x0000
#define LLDP_CONF_POINTER_MOD_OFFSET 0x0023

  EFI_STATUS      EfiStatus;
  UINT16          DataPtr1;
  UINT16          DataPtr2;

  //First we need to get EMP SR Settings Pointer
  EfiStatus = ReadTlv (
                UndiPrivateData,
                DEFAULT_LLDP_MODULE_TYPE_ID,
                NVM_EMP_SR_SETTINGS_MODULE_PTR,
                sizeof (DataPtr1),
                &DataPtr1
              );
  if (EFI_ERROR (EfiStatus)) {
    goto ExitError;
  }

  //If MSB = 1 then offset is in 4KB sector units
  if ((DataPtr1 & 0x8000) != 0) {
    //To get offset in words we need to multiply it
    DataPtr1 = DataPtr1 * 0x800;
  }

  DataPtr1 += LLDP_CONF_POINTER_MOD_OFFSET;

  //Now we can read LLDP configuration pointer
  EfiStatus = ReadTlv (
                UndiPrivateData,
                DEFAULT_LLDP_MODULE_TYPE_ID,
                DataPtr1,
                sizeof (DataPtr2),
                &DataPtr2
              );
  if (EFI_ERROR (EfiStatus)) {
    goto ExitError;
  }

  //Finally we can read default LLDP value
  EfiStatus = ReadTlv (
                UndiPrivateData,
                DEFAULT_LLDP_MODULE_TYPE_ID,
                DataPtr1 + DataPtr2 + OffsetInModule,
                sizeof (*DefaultValue),
                DefaultValue
              );
  if (EFI_ERROR (EfiStatus)) {
    goto ExitError;
  }

  return EFI_SUCCESS;

  ExitError:
  DEBUGPRINT (CRITICAL, ("ReadTlv returned %d\n", EfiStatus));

  return EFI_DEVICE_ERROR;
}

/** Restore factory LLDP Agent status.

   @param[in]   UndiPrivateData  Pointer to driver private data structure.

   @retval      EFI_SUCCESS      LLDP Admin reset successful.
   @retval      EFI_DEVICE_ERROR Failed to reset LLDP Admin.
**/
EFI_STATUS
ResetLLDPAdminStatus (
  IN  UNDI_PRIVATE_DATA *UndiPrivateData
  )
{
  EFI_STATUS       EfiStatus;
  UINT8            PhysicalPortNr;
  UINT32           Mask;
  UINT32           Temp;

  PhysicalPortNr = UndiPrivateData->NicInfo.PhysicalPortNumber;

  //First we need to read current value
  EfiStatus = ReadTlv (
                UndiPrivateData,
                TLV_ID_CURRENT_LLDP,
                SKIP_TLV_LENGTH,
                sizeof (Temp),
                &Temp
              );
  if (EFI_ERROR (EfiStatus)) {
    DEBUGPRINT (CRITICAL, ("Failed to read current LLDP AdminStatus"));
    return EFI_DEVICE_ERROR;
  }

  //Then set 0xF for a corresponding port in variable, we want to change value only for a port
  Mask = 0xF << PhysicalPortNr * 4 ;
  Temp |= Mask;

  //Finally we can write entire double word to NVM
  EfiStatus = WriteTlv (
                UndiPrivateData,
                TLV_ID_CURRENT_LLDP,
                SKIP_TLV_LENGTH,
                sizeof (Temp),
                &Temp
              );
  if (EFI_ERROR (EfiStatus)) {
    DEBUGPRINT (CRITICAL, ("Failed to reset current LLDP AdminStatus"));
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}

/** Get default LLDP Agent status.

   @param[in]   UndiPrivateData      Pointer to driver private data structure.
   @param[out]  DefaultLldpAgentEna  Pointer to variable which should store default value for LLDP Agent.

   @retval      EFI_SUCCESS          LLDP Agent get default successful.
   @retval      EFI_DEVICE_ERROR     Out of range value read from NVM.
   @retval      !EFI_SUCCESS         Failed to get default LLDP Agent.
**/
EFI_STATUS
GetDfltLldpAgentStatus (
  IN  UNDI_PRIVATE_DATA  *UndiPrivateData,
  OUT BOOLEAN            *DefaultLldpAgentEna
  )
{
  EFI_STATUS      EfiStatus;
  UINT32          Offset = 0x0001;
  UINT8           PhysicalPortNr;
  UINT16          Buffer;
  UINT8           LldpAdminStatus;

  PhysicalPortNr = UndiPrivateData->NicInfo.PhysicalPortNumber;

  // When physical port greater than 3 we need to read second word from module
  if (PhysicalPortNr > 3) {
    PhysicalPortNr -= 4;
    Offset = 0x0002;
  }

  EfiStatus = ReadDefaultLLDPAdminStatus (UndiPrivateData, Offset, &Buffer);
  IF_RETURN (EFI_ERROR (EfiStatus), EfiStatus);

  LldpAdminStatus = GetLLDPAdminForPort (PhysicalPortNr, Buffer);
  IF_RETURN (LldpAdminStatus > 3, EFI_DEVICE_ERROR);

  *DefaultLldpAgentEna = (LldpAdminStatus != 0);

  return EFI_SUCCESS;
}










/** Gets S-IOV feature setting.

   @param[in]   UndiPrivateData      Pointer to driver private data structure.
   @param[out]  SiovEnabled          S-IOV feature setting.

   @retval   EFI_SUCCESS             S-IOV feature successfully returned.
   @retval   EFI_DEVICE_ERROR        Failed to read S-IOV feature configuration from NVM.
   @retval   EFI_NOT_READY           Failed to acquire NVM.
**/
EFI_STATUS
GetSiovFeature (
  IN  UNDI_PRIVATE_DATA  *UndiPrivateData,
  OUT BOOLEAN            *SiovEnabled
  )
{
  enum ice_status             IceStatus = ICE_SUCCESS;
  struct ice_hw               *Hw;
  struct ice_aqc_nvm_cfg_data ConfigData;
  UINT16                      PasidState;
  UINT16                      PfSwitchMode;

  *SiovEnabled = FALSE;
  Hw           = &UndiPrivateData->NicInfo.Hw;
  ZeroMem (&ConfigData, sizeof (ConfigData));

  IceStatus = ice_acquire_nvm (Hw, ICE_RES_READ);
  IF_SCERR_RETURN (IceStatus, EFI_NOT_READY);

  IceStatus = ice_aq_read_nvm_cfg (Hw, 0, ICE_PASID_ID, &ConfigData, sizeof (ConfigData), NULL, NULL);
  if (IceStatus != ICE_SUCCESS) {
    ice_release_nvm (Hw);
    DEBUGPRINT (CRITICAL, ("ice_aq_read_nvm_cfg failed for ICE_PASID_ID with %d\n", IceStatus));
    return EFI_DEVICE_ERROR;
  }

  PasidState = ConfigData.field_value;
  ZeroMem (&ConfigData, sizeof (ConfigData));

  IceStatus = ice_aq_read_nvm_cfg (
                Hw,
                0,
                ICE_PF_SWITCH_MODE_MEM_SPACE_ID,
                &ConfigData,
                sizeof (ConfigData),
                NULL,
                NULL
                );
  ice_release_nvm (Hw);
  if (IceStatus != ICE_SUCCESS) {
    DEBUGPRINT (CRITICAL, ("ice_aq_read_nvm_cfg failed for ICE_PF_SWITCH_MODE_MEM_SPACE_ID with %d\n", IceStatus));
    return EFI_DEVICE_ERROR;
  }

  PfSwitchMode = ConfigData.field_value;
  if ((PfSwitchMode == ICE_PF_SWITCH_MODE_ENABLE) &&
      (PasidState == ICE_PASID_ENABLE))
  {
    *SiovEnabled = TRUE;
  }

  return EFI_SUCCESS;
}

/** Sets S-IOV feature.

   @param[in]   UndiPrivateData     Pointer to driver private data structure.
   @param[in]   SiovEnabled         Desired S-IOV feature setting.

   @retval   EFI_SUCCESS            S-IOV feature successfully set.
   @retval   EFI_DEVICE_ERROR       Failed to write S-IOV feature configuration to NVM.
   @retval   EFI_NOT_READY          Failed to acquire NVM.
**/
EFI_STATUS
SetSiovFeature (
  IN  UNDI_PRIVATE_DATA  *UndiPrivateData,
  IN  BOOLEAN            *SiovEnabled
  )
{
  enum ice_status             IceStatus = ICE_SUCCESS;
  struct ice_hw               *Hw;
  struct ice_aqc_nvm_cfg_data ConfigData;
  UINT16                      PasidState;
  UINT16                      PreviousPasidState;
  UINT16                      PfSwitchMode;

  Hw = &UndiPrivateData->NicInfo.Hw;
  ZeroMem (&ConfigData, sizeof (ConfigData));

  if (*SiovEnabled) {
    PasidState   = ICE_PASID_ENABLE;
    PfSwitchMode = ICE_PF_SWITCH_MODE_ENABLE;
  } else {
    PasidState   = ICE_PASID_DISABLE;
    PfSwitchMode = ICE_PF_SWITCH_MODE_DISABLE;
  }

  IceStatus = ice_acquire_nvm (Hw, ICE_RES_WRITE);
  IF_SCERR_RETURN (IceStatus, EFI_NOT_READY);

  // Try to memorize the original value, we may need this if the second transaction fails.
  IceStatus = ice_aq_read_nvm_cfg (Hw, 0, ICE_PASID_ID, &ConfigData, sizeof (ConfigData), NULL, NULL);
  if (IceStatus != ICE_SUCCESS) {
    DEBUGPRINT (CRITICAL, ("Failed to read PASID capability with %d\n", IceStatus));
    goto ExitReleaseNvm;
  }
  PreviousPasidState = ConfigData.field_value;
  ZeroMem (&ConfigData, sizeof (ConfigData));

  // First transaction
  ConfigData.field_id    = ICE_PASID_ID;
  ConfigData.field_value = PasidState;
  IceStatus = ice_aq_write_nvm_cfg (Hw, 0, &ConfigData, sizeof (ConfigData), 1, NULL);
  if (IceStatus != ICE_SUCCESS) {
    DEBUGPRINT (CRITICAL, ("Couldn't set PASID capability: %d\n", IceStatus));
    goto ExitReleaseNvm;
  }
  ZeroMem (&ConfigData, sizeof (ConfigData));

  // Second transaction
  ConfigData.field_id    = ICE_PF_SWITCH_MODE_MEM_SPACE_ID;
  ConfigData.field_value = PfSwitchMode;
  IceStatus = ice_aq_write_nvm_cfg (Hw, 0, &ConfigData, sizeof (ConfigData), 1, NULL);
  if (IceStatus != ICE_SUCCESS) {
    DEBUGPRINT (CRITICAL, ("Couldn't set PF Switch mode memory space: %d\n", IceStatus));
    goto ExitRevertPasid;
  }

  ice_release_nvm (Hw);
  return EFI_SUCCESS;

ExitRevertPasid:
  // Revert the first transaction
  ZeroMem (&ConfigData, sizeof (ConfigData));

  ConfigData.field_id    = ICE_PASID_ID;
  ConfigData.field_value = PreviousPasidState;
  IceStatus = ice_aq_write_nvm_cfg (Hw, 0, &ConfigData, sizeof (ConfigData), 1, NULL);
  if (IceStatus != ICE_SUCCESS) {
    DEBUGPRINT (CRITICAL, ("Couldn't restore PASID capability: %d\n", IceStatus));
  }

ExitReleaseNvm:
  ice_release_nvm (Hw);
  return EFI_DEVICE_ERROR;
}

/** Sets Transmit Balancing.

   @param[in]   UndiPrivateData           Pointer to driver private data structure.
   @param[in]   TransmitBalancingEnabled  Desired Transmit Balancing setting.

   @retval   EFI_SUCCESS       Transmit Balancing setting successfully set.
   @retval   EFI_DEVICE_ERROR  Failed to set Transmit Balancing in NVM.
**/
EFI_STATUS
SetTransmitBalancing (
  IN  UNDI_PRIVATE_DATA  *UndiPrivateData,
  IN  BOOLEAN            *TransmitBalancingEnabled
  )
{
  EFI_STATUS             Status;
  UINT16                 WordToSet;
  UINT16                 CurrentWord;

  Status = ReadTlv (
             UndiPrivateData,
             TLV_ID_TST_US,
             SKIP_TLV_LENGTH,
             sizeof (CurrentWord),
             &CurrentWord
             );
  IF_RETURN (EFI_ERROR (Status), EFI_DEVICE_ERROR);

  if (*TransmitBalancingEnabled) {
    WordToSet = CurrentWord | TOPOLOGY_SOURCE_BIT;
  } else {
    WordToSet = CurrentWord & ~TOPOLOGY_SOURCE_BIT;
  }

  if (WordToSet == CurrentWord) {
    return EFI_SUCCESS;
  }

  Status = WriteTlv (
             UndiPrivateData,
             TLV_ID_TST_US,
             SKIP_TLV_LENGTH,
             sizeof (WordToSet),
             &WordToSet
             );
  IF_RETURN (EFI_ERROR (Status), EFI_DEVICE_ERROR);

  return EFI_SUCCESS;
}

/** Gets Transmit Balancing setting.

   @param[in]   UndiPrivateData           Pointer to driver private data structure.
   @param[out]  TransmitBalancingEnabled  Transmit Balancing setting.

   @retval   EFI_SUCCESS       Transmit Balancing setting successfully gotten.
   @retval   EFI_DEVICE_ERROR  Failed to get Transmit Balancing from NVM.
**/
EFI_STATUS
GetTransmitBalancing (
  IN  UNDI_PRIVATE_DATA  *UndiPrivateData,
  OUT BOOLEAN            *TransmitBalancingEnabled
  )
{
  EFI_STATUS             Status;
  UINT16                 Word;

  Status = ReadTlv (
             UndiPrivateData,
             TLV_ID_TST_US,
             SKIP_TLV_LENGTH,
             sizeof (Word),
             &Word
             );
  IF_RETURN (EFI_ERROR (Status), EFI_DEVICE_ERROR);

  *TransmitBalancingEnabled = ((Word & TOPOLOGY_SOURCE_BIT) != 0);

  return EFI_SUCCESS;
}

/** Operations executed pre RouteConfig() map processing, needed for 100G driver.

   @param[in]   UndiPrivateData  Pointer to driver private data structure
   @param[in]   VarStoreMapCfg   HII varstore map configuration structure
   @param[in]   HiiCfgData       Pointer to configuration data buffer (of varstore type)
   @param[in]   Configuration    RouteConfig Configuration string

   @retval      EFI_SUCCESS      Operation successful
   @retval      !EFI_SUCCESS     Failed to get current active port option number
   @retval      !EFI_SUCCESS     Failed to get information if active port option field is present
                                 in configuration request
**/
EFI_STATUS
HiiAdapterPreRoute (
  IN       UNDI_PRIVATE_DATA     *UndiPrivateData,
  IN       HII_VARSTORE_MAP_CFG  *VarStoreMapCfg,
  IN       HII_STD_VARSTORE      *HiiCfgData,
  IN CONST EFI_STRING            Configuration
  )
{
  EFI_STATUS            Status;
  UINTN                 ActivePortOptionOffset = STRUCT_OFFSET (HII_STD_VARSTORE, ActivePortOption);
  UINT8                 CurrentActivePortOption;
  HII_CONFIG_MAP_ENTRY  *ConfigMapEntry;

  if (UndiPrivateData->NicInfo.Function == 0) {
    Status = GetCurrentPortOptionNum (UndiPrivateData, &CurrentActivePortOption);
    IF_RETURN (EFI_ERROR (Status), Status);

    Status = IsFieldPresentInConfiguration (Configuration, ActivePortOptionOffset);
    IF_RETURN (Status == EFI_INVALID_PARAMETER, Status);

    if (Status == EFI_SUCCESS) {
      // Also required for PostRoute
      UndiPrivateData->HiiInfo.PortOptionChangeRequested = (HiiCfgData->ActivePortOption != CurrentActivePortOption);
    }
  }

  // Block every other HII field setting when:
  // - Port Option is present in Configuration string & is different from current, or
  // - there is pending Port Option from previous RouteConfig call
  if (UndiPrivateData->HiiInfo.PortOptionChangeRequested ||
      UndiPrivateData->HiiInfo.PendingPortOptionValid)
  {
    DEBUGPRINT (HII, (" Port Option changed - set SetBlocked\n"));
    FOR_EACH_CONFIG_MAP_ENTRY (VarStoreMapCfg, ConfigMapEntry) {
      if (ConfigMapEntry->FieldOffset != ActivePortOptionOffset) {
        ConfigMapEntry->SetBlocked = TRUE;
      }
    }
  }

  return EFI_SUCCESS;
}

/** Operations executed post RouteConfig() map processing, needed for 100G driver.

   @param[in]   UndiPrivateData  Pointer to driver private data structure
   @param[in]   VarStoreMapCfg   HII varstore map configuration structure

   @retval      EFI_SUCCESS            Operation successful
   @retval      EFI_ACCESS_DENIED      Update blocked, port option change pending
**/
EFI_STATUS
HiiAdapterPostRoute (
  IN  UNDI_PRIVATE_DATA     *UndiPrivateData,
  IN  HII_VARSTORE_MAP_CFG  *VarStoreMapCfg
  )
{
  HII_CONFIG_MAP_ENTRY  *ConfigMapEntry;
  EFI_STATUS            Status;

  if (UndiPrivateData->HiiInfo.PortOptionChangeRequested) {
    Status = PortConfigDefault (UndiPrivateData); // Set to default all Port Option related items
    IF_RETURN (EFI_ERROR (Status), Status);
    UndiPrivateData->HiiInfo.PortOptionChangeRequested = FALSE;
  }

  FOR_EACH_CONFIG_MAP_ENTRY (VarStoreMapCfg, ConfigMapEntry) {
    if (ConfigMapEntry->SetBlocked &&
        ConfigMapEntry->SetAttempted)
    {
#if DBG_LVL & HII
      DEBUGPRINT (HII, (" SetAttempted at field %a\n", ConfigMapEntry->Name));
#endif
      CreatePortOptPendingWarning (UndiPrivateData);
      return EFI_ACCESS_DENIED;
    }
  }

  return EFI_SUCCESS;
}
