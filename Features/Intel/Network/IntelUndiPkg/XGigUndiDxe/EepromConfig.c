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
#include "CommonDriver.h"
#include "ixgbe_common.h"
#include "Xgbe.h"
#include "EepromConfig.h"
#include "wol.h"
#include "DeviceSupport.h"
#include "Forms/HiiFormDefs.h"


/** Reads SR buffer.

   @param[in]   UndiPrivateData  Points to the driver information.
   @param[in]   Offset           Offset in words from module start.
   @param[in]   Length           Number of words to read.
   @param[out]  Data             Pointer to location with data to be read to.

   @retval    EFI_SUCCESS            Buffer successfully read.
   @retval    EFI_INVALID_PARAMETER  UndiPrivateData or Data is NULL.
   @retval    EFI_DEVICE_ERROR       Failed to read buffer.
**/
EFI_STATUS
ReadSrBuffer16 (
  IN  UNDI_PRIVATE_DATA  *UndiPrivateData,
  IN  UINT16             Offset,
  IN  UINT16             Length,
  OUT UINT16             *Data
  )
{
  INT32  XgbeStatus;

  if ((UndiPrivateData == NULL) ||
      (Data == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  XgbeStatus = ixgbe_read_eeprom_buffer (&UndiPrivateData->NicInfo.Hw, Offset, Length, Data);

  return (XgbeStatus == IXGBE_SUCCESS) ? EFI_SUCCESS : EFI_DEVICE_ERROR;
}

/** Reads SR word.

   @param[in]   UndiPrivateData  Points to the driver information.
   @param[in]   Offset           Offset in words from module start.
   @param[out]  Data             Pointer to location with data to be read to.

   @retval    EFI_SUCCESS     Word successfully read.
   @retval    !EFI_SUCCESS    Word not read, failure of underlying function.
**/
EFI_STATUS
ReadSr16 (
  IN  UNDI_PRIVATE_DATA  *UndiPrivateData,
  IN  UINT16             Offset,
  OUT UINT16             *Data
  )
{
  return ReadSrBuffer16 (UndiPrivateData, Offset, 1, Data);
}

/** Writes SR buffer.

   @param[in]   UndiPrivateData  Points to the driver information.
   @param[in]   Offset           Offset in words from module start.
   @param[in]   Length           Number of words to write.
   @param[in]  Data             Pointer to location with words to be written.

   @retval    EFI_SUCCESS            Buffer successfully written.
   @retval    EFI_INVALID_PARAMETER  UndiPrivateData or Data is NULL or buffer too long.
   @retval    EFI_DEVICE_ERROR       Failed to write buffer.
**/
EFI_STATUS
WriteSrBuffer16 (
  IN  UNDI_PRIVATE_DATA  *UndiPrivateData,
  IN  UINT16             Offset,
  IN  UINT16             Length,
  IN  UINT16             *Data
  )
{
  INT32  XgbeStatus;
  UINTN  i;
  UINTN  j;
  UINT16 TmpBuffer[EEPROM_WRITE_BUFFER_SIZE];
  UINT32 Timeout;

  if ((UndiPrivateData == NULL) ||
      (Data == NULL) ||
      (0 == Length) ||
      (EEPROM_WRITE_BUFFER_SIZE < Length))
  {
    return EFI_INVALID_PARAMETER;
  }
  
  // Read current contents of EEPROM area that should be updated
  XgbeStatus = ixgbe_read_eeprom_buffer (&UndiPrivateData->NicInfo.Hw, Offset, Length, TmpBuffer);
  if (XgbeStatus != IXGBE_SUCCESS) {
    return EFI_DEVICE_ERROR;
  }

  Timeout = 10;

  // Compare word by word and modify only that words that are different and should be changed in EEPROM
  for (i = 0; i < Length; i++) {
    if (TmpBuffer[i] != Data[i]) {
      for (j = 0; j < Timeout; j++) {
        XgbeStatus = ixgbe_write_eeprom (&UndiPrivateData->NicInfo.Hw, Offset + i, Data[i]);
        if (XgbeStatus == IXGBE_ERR_SWFW_SYNC) {
          DEBUGPRINT (CRITICAL, ("ixgbe_write_eeprom - semaphore error\n"));
          gBS->Stall (5000);
          continue;
        } else {
          break;
        }
      }
      if (XgbeStatus != IXGBE_SUCCESS) {
        DEBUGPRINT (CRITICAL, ("ixgbe_write_eeprom returned: %r\n", XgbeStatus));
        return EFI_DEVICE_ERROR;
      }
    }
  }
  return EFI_SUCCESS;
}

/** Writes SR word.

   @param[in]   UndiPrivateData  Points to the driver information.
   @param[in]   Offset           Offset in words from module start.
   @param[in]  Data             Word to be written.

   @retval    EFI_SUCCESS    Word successfully written.
   @retval    !EFI_SUCCESS   Word not written, failure of underlying function.
**/
EFI_STATUS
WriteSr16 (
  IN  UNDI_PRIVATE_DATA  *UndiPrivateData,
  IN  UINT16             Offset,
  IN  UINT16             Data
  )
{
  return WriteSrBuffer16 (UndiPrivateData, Offset, 1, &Data);
}



/** Gets link speed setting for adapter.

   @param[in]   UndiPrivateData        Pointer to driver private data structure
   @param[out]  LinkSpeed              Link speed setting

   @retval      EFI_SUCCESS            Successful operation
   @retval      EFI_INVALID_PARAMETER  UndiPrivateData or LinkSpeed is NULL
**/
EFI_STATUS
GetLinkSpeed (
  IN  UNDI_PRIVATE_DATA *UndiPrivateData,
  OUT UINT8             *LinkSpeed
  )
{
  *LinkSpeed = LINK_SPEED_AUTO_NEG;

  if ((UndiPrivateData == NULL) ||
      (LinkSpeed == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  //  Speed settings are currently not supported for 10 Gig driver. It's always set to autoneg to
  //  allow operation with the highest possible speed
  DEBUGPRINT (HII, ("EepromGetLanSpeedStatus\n"));
  return EFI_SUCCESS;
}

/** Sets LAN speed setting for port

   @param[in]   UndiPrivateData  Pointer to driver private data structure
   @param[in]   LinkSpeed        Link speed setting

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

/** Reads the currently assigned MAC address and factory default MAC address.

   @param[in]    UndiPrivateData     Driver private data structure
   @param[in]    LanFunction         LAN function number
   @param[out]   DefaultMacAddress   Factory default MAC address of the adapter
   @param[out]   AssignedMacAddress  CLP Assigned MAC address of the adapter,
                                     or the factory MAC address if an alternate MAC
                                     address has not been assigned.

   @retval   EFI_SUCCESS   MAC addresses successfully read.
**/
EFI_STATUS
_EepromMacAddressGet (
  IN  UNDI_PRIVATE_DATA *UndiPrivateData,
  IN  UINT32             LanFunction,
  OUT UINT16 *           DefaultMacAddress,
  OUT UINT16 *           AssignedMacAddress
  )
{
  UINT16 BackupMacOffset;
  UINT16 FactoryMacOffset;
  UINT16 BackupMacAddress[3];

  DRIVER_DATA *AdapterInfo = &UndiPrivateData->NicInfo;

  // Read in the currently assigned address from the default MAC address location
  if (LanFunction == 0) {
    ixgbe_read_eeprom (&AdapterInfo->Hw, IXGBE_CORE0_PTR, &FactoryMacOffset);
  } else {
    ixgbe_read_eeprom (&AdapterInfo->Hw, IXGBE_CORE1_PTR, &FactoryMacOffset);
  }

  // The first word of the EEPROM CORE structure is the size field.  The MAC address
  // starts after the size field.  Adjust here:
  FactoryMacOffset += 1;
  DEBUGPRINT (CLP, ("Factory MAC address at offset %X\n", FactoryMacOffset));

  ixgbe_read_eeprom (&AdapterInfo->Hw, FactoryMacOffset, &AssignedMacAddress[0]);
  ixgbe_read_eeprom (&AdapterInfo->Hw, FactoryMacOffset + 1, &AssignedMacAddress[1]);
  ixgbe_read_eeprom (&AdapterInfo->Hw, FactoryMacOffset + 2, &AssignedMacAddress[2]);

  // Check to see if the backup MAC address location is being used, otherwise the
  // factory MAC address location will be the default.
  ixgbe_read_eeprom (&AdapterInfo->Hw, IXGBE_ALT_MAC_ADDR_PTR, &BackupMacOffset);

  if (BackupMacOffset == 0xFFFF
    || BackupMacOffset == 0x0000)
  {
    DEBUGPRINT (CLP, ("Alt MAC Address feature not enabled.\n"));
    ixgbe_read_eeprom (&AdapterInfo->Hw, FactoryMacOffset, &DefaultMacAddress[0]);
    ixgbe_read_eeprom (&AdapterInfo->Hw, FactoryMacOffset + 1, &DefaultMacAddress[1]);
    ixgbe_read_eeprom (&AdapterInfo->Hw, FactoryMacOffset + 2, &DefaultMacAddress[2]);
  } else {

    // Adjust the MAC address offset if this is the second port (function 1)
    BackupMacOffset = BackupMacOffset + (UINT16) (3 * AdapterInfo->Function);
    DEBUGPRINT (CLP, ("MAC addresses at offset %X\n", BackupMacOffset));

    // Check if MAC address is backed up
    ixgbe_read_eeprom (&AdapterInfo->Hw, BackupMacOffset, &BackupMacAddress[0]);
    if (BackupMacAddress[0] == 0xFFFF) {

      // In this case the factory MAC address is not in the backup location, so the factory
      // default MAC address is the same as the address we read in from the EEPROM CORE 0/1
      // locations.
      DefaultMacAddress[0] = AssignedMacAddress[0];
      DefaultMacAddress[1] = AssignedMacAddress[1];
      DefaultMacAddress[2] = AssignedMacAddress[2];
    } else {

      // Read in the factory default Mac address.
      ixgbe_read_eeprom (&AdapterInfo->Hw, BackupMacOffset, &DefaultMacAddress[0]);
      ixgbe_read_eeprom (&AdapterInfo->Hw, BackupMacOffset + 1, &DefaultMacAddress[1]);
      ixgbe_read_eeprom (&AdapterInfo->Hw, BackupMacOffset + 2, &DefaultMacAddress[2]);
    }
  }

  return EFI_SUCCESS;
}

/** Reads factory default MAC address.

   @param[in]   UndiPrivateData      Pointer to driver private data structure
   @param[out]  FactoryMacAddress    Factory default MAC address of the adapter

   @retval      EFI_SUCCESS       MAC addresses read successfully
   @retval      !EFI_SUCCESS      Failure of underlying function
**/
EFI_STATUS
GetFactoryMacAddress (
  IN  UNDI_PRIVATE_DATA  *UndiPrivateData,
  OUT UINT8              *FactoryMacAddress
  )
{
  UINT8  AlternateMacAddress[ETH_ALEN];

  return _EepromMacAddressGet (
           UndiPrivateData,
           UndiPrivateData->NicInfo.LanFunction,
           (UINT16 *)FactoryMacAddress,
           (UINT16 *)AlternateMacAddress
           );
}

/** Gets alternate MAC address of currently managed PF.

   @param[in]   UndiPrivateData      Pointer to driver private data structure
   @param[out]  AlternateMacAddress  Pointer to buffer for resulting alternate
                                     MAC address

   @retval      EFI_SUCCESS       MAC addresses read successfully
   @retval      !EFI_SUCCESS      Failure of underlying function
**/
EFI_STATUS
GetAlternateMacAddress (
  IN   UNDI_PRIVATE_DATA  *UndiPrivateData,
  OUT  UINT8              *AlternateMacAddress
  )
{
  UINT8  FactoryMacAddress[ETH_ALEN];

  return _EepromMacAddressGet (
           UndiPrivateData,
           UndiPrivateData->NicInfo.LanFunction,
           (UINT16 *)FactoryMacAddress,
           (UINT16 *)AlternateMacAddress
           );
}




/** Programs the port with an alternate MAC address, and (in 82580-like case)
   backs up the factory default MAC address.

   @param[in]   UndiPrivateData   Pointer to driver private data structure
   @param[in]   MacAddress        Value to set the MAC address to.

   @retval   EFI_UNSUPPORTED   Alternate MAC Address feature not enabled
   @retval   EFI_SUCCESS       Default MAC address set successfully
**/
EFI_STATUS
SetAlternateMacAddress (
  IN UNDI_PRIVATE_DATA *UndiPrivateData,
  IN UINT8             *MacAddress
  )
{
  UINT16 BackupMacOffset;
  UINT16 FactoryMacOffset;
  UINT16 BackupMacAddress[3];

  DRIVER_DATA *AdapterInfo = &UndiPrivateData->NicInfo;

  // Read the address where the override MAC address is stored.
  ixgbe_read_eeprom (&AdapterInfo->Hw, IXGBE_ALT_MAC_ADDR_PTR, &BackupMacOffset);

  if (BackupMacOffset == 0xFFFF
    || BackupMacOffset == 0x0000)
  {
    DEBUGPRINT (CLP, ("Alt MAC Address feature not enabled.\n"));
    return EFI_UNSUPPORTED;
  }

  // Adjust the MAC address offset if this is the second port (function 1)
  BackupMacOffset = BackupMacOffset + (UINT16) (3 * AdapterInfo->Function);
  DEBUGPRINT (CLP, ("MAC addresses at offset %X\n", BackupMacOffset));

  if (AdapterInfo->LanFunction == 0) {
    ixgbe_read_eeprom (&AdapterInfo->Hw, IXGBE_CORE0_PTR, &FactoryMacOffset);
  } else {
    ixgbe_read_eeprom (&AdapterInfo->Hw, IXGBE_CORE1_PTR, &FactoryMacOffset);
  }

  // The first word of the EEPROM CORE structure is the size field.  The MAC address
  // starts after the size field.  Adjust here:
  FactoryMacOffset += 1;
  DEBUGPRINT (CLP, ("Factory MAC address at offset %X\n", FactoryMacOffset));


  if ((UndiPrivateData->NicInfo.Hw.mac.type != ixgbe_mac_X550)
    && (UndiPrivateData->NicInfo.Hw.mac.type != ixgbe_mac_X550EM_x)
    && (UndiPrivateData->NicInfo.Hw.mac.type != ixgbe_mac_X550EM_a)
    )
  {

  // Check if MAC address is backed up

  ixgbe_read_eeprom (&AdapterInfo->Hw, BackupMacOffset, &BackupMacAddress[0]);
  if (BackupMacAddress[0] == 0xFFFF) {

    // Read in the factory MAC address
    ixgbe_read_eeprom (&AdapterInfo->Hw, FactoryMacOffset, &BackupMacAddress[0]);
    ixgbe_read_eeprom (&AdapterInfo->Hw, FactoryMacOffset + 1, &BackupMacAddress[1]);
    ixgbe_read_eeprom (&AdapterInfo->Hw, FactoryMacOffset + 2, &BackupMacAddress[2]);

    // Now back it up
    ixgbe_write_eeprom (&AdapterInfo->Hw, BackupMacOffset, BackupMacAddress[0]);
    ixgbe_write_eeprom (&AdapterInfo->Hw, BackupMacOffset + 1, BackupMacAddress[1]);
    ixgbe_write_eeprom (&AdapterInfo->Hw, BackupMacOffset + 2, BackupMacAddress[2]);
  }
  }

  // At this point the factory MAC address should be in the backup location.  Now
  // write the new CLP assigned MAC address into the original factory location.
  ixgbe_write_eeprom (&AdapterInfo->Hw, FactoryMacOffset, ((UINT16 *) MacAddress)[0]);
  ixgbe_write_eeprom (&AdapterInfo->Hw, FactoryMacOffset + 1, ((UINT16 *) MacAddress)[1]);
  ixgbe_write_eeprom (&AdapterInfo->Hw, FactoryMacOffset + 2, ((UINT16 *) MacAddress)[2]);

  ixgbe_update_eeprom_checksum (&AdapterInfo->Hw);

  return EFI_SUCCESS;
}

/** Restores the factory default MAC address.

   @param[in]   UndiPrivateData   Driver private data structure

   @retval   EFI_UNSUPPORTED   Invalid offset for alternate MAC address
   @retval   EFI_SUCCESS       Alternate MAC Address feature not enabled
**/
EFI_STATUS
RestoreDefaultMacAddress (
  IN UNDI_PRIVATE_DATA *UndiPrivateData
  )
{
  UINT16            BackupMacOffset;
  UINT16            FactoryMacOffset;
  UINT16            BackupMacAddress[3];

  DRIVER_DATA *AdapterInfo = &UndiPrivateData->NicInfo;

  // Read the address where the override MAC address is stored.
  ixgbe_read_eeprom (&AdapterInfo->Hw, IXGBE_ALT_MAC_ADDR_PTR, &BackupMacOffset);

  if (BackupMacOffset == 0xFFFF
    || BackupMacOffset == 0x0000)
  {
    DEBUGPRINT (CLP, ("Alt MAC Address feature not enabled.\n"));
    return EFI_UNSUPPORTED;
  }

  // Adjust the MAC address offset if this is the second port (function 1)
  BackupMacOffset = BackupMacOffset + (UINT16) (3 * AdapterInfo->Function);
  DEBUGPRINT (CLP, ("MAC addresses at offset %X\n", BackupMacOffset));

  // Check if MAC address is backed up
  ixgbe_read_eeprom (&AdapterInfo->Hw, BackupMacOffset, &BackupMacAddress[0]);
  ixgbe_read_eeprom (&AdapterInfo->Hw, BackupMacOffset + 1, &BackupMacAddress[1]);
  ixgbe_read_eeprom (&AdapterInfo->Hw, BackupMacOffset + 2, &BackupMacAddress[2]);
  if (BackupMacAddress[0] == 0xFFFF) {
    DEBUGPRINT (CRITICAL, ("No backup MAC addresses\n"));
    return EFI_SUCCESS;
  }

  // Restore the factory MAC address
  if (AdapterInfo->LanFunction == 0) {
    ixgbe_read_eeprom (&AdapterInfo->Hw, IXGBE_CORE0_PTR, &FactoryMacOffset);
  } else {
    ixgbe_read_eeprom (&AdapterInfo->Hw, IXGBE_CORE1_PTR, &FactoryMacOffset);
  }

  // The first word of the EEPROM CORE structure is the size field.  The MAC address
  // starts after the size field.  Adjust here:
  FactoryMacOffset += 1;
  DEBUGPRINT (CLP, ("Factory MAC address at offset %X\n", FactoryMacOffset));

  ixgbe_write_eeprom (&AdapterInfo->Hw, FactoryMacOffset, BackupMacAddress[0]);
  ixgbe_write_eeprom (&AdapterInfo->Hw, FactoryMacOffset + 1, BackupMacAddress[1]);
  ixgbe_write_eeprom (&AdapterInfo->Hw, FactoryMacOffset + 2, BackupMacAddress[2]);

  ixgbe_update_eeprom_checksum (&AdapterInfo->Hw);

  return EFI_SUCCESS;
}

/** Returns EEPROM capabilities word (0x33) for current adapter

   @param[in]    UndiPrivateData   Points to the driver instance private data
   @param[out]   CapabilitiesWord   EEPROM capabilities word (0x33) for current adapter

   @retval   EFI_SUCCESS   Function completed successfully,
   @retval   !EFI_SUCCESS  Failed to read EEPROM capabilities word
**/
EFI_STATUS
EepromGetCapabilitiesWord (
  IN  UNDI_PRIVATE_DATA *UndiPrivateData,
  OUT UINT16 *           CapabilitiesWord
  )
{
  UINT16     Word;
  EFI_STATUS Status;

  Status = ixgbe_read_eeprom (&UndiPrivateData->NicInfo.Hw, EEPROM_CAPABILITIES_WORD, &Word);
  Word &= ~EEPROM_CAPABILITIES_SIG;
  *CapabilitiesWord = Word;

  return Status;
}

/** Checks if it is LOM device

   @param[in]   UndiPrivateData   Points to the driver instance private data

   @retval   TRUE     It is LOM device
   @retval   FALSE    It is not LOM device
   @retval   FALSE    Failed to read NVM word
**/
BOOLEAN
EepromIsLomDevice (
  IN  UNDI_PRIVATE_DATA *UndiPrivateData
  )
{
  EFI_STATUS Status;
  UINT16     SetupWord;

  if (UndiPrivateData->NicInfo.Hw.mac.type == ixgbe_mac_82598EB) {
    Status = ixgbe_read_eeprom (&UndiPrivateData->NicInfo.Hw, EEPROM_COMPATIBILITY_WORD_OPLIN, &SetupWord);
    if (SetupWord == 0xffff) {
      SetupWord = 0;
    }
  } else {
    Status = ixgbe_read_eeprom (&UndiPrivateData->NicInfo.Hw, EEPROM_COMPATIBILITY_WORD, &SetupWord);
  }
  if (Status != IXGBE_SUCCESS) {
    return FALSE;
  }

  if ((SetupWord & EEPROM_COMPATABILITY_LOM_BIT) == EEPROM_COMPATABILITY_LOM_BIT) {
    return TRUE;
  }
  return FALSE;

}

/** Updates NVM checksum

   @param[in]   UndiPrivateData   Pointer to driver private data structure

   @retval      EFI_SUCCESS       Checksum successfully updated
   @retval      EFI_DEVICE_ERROR  Failed to update NVM checksum
**/
EFI_STATUS
UpdateNvmChecksum (
  IN  UNDI_PRIVATE_DATA *UndiPrivateData
  )
{
  if (ixgbe_update_eeprom_checksum (&UndiPrivateData->NicInfo.Hw) != IXGBE_SUCCESS) {
    return EFI_DEVICE_ERROR;
  }
  return EFI_SUCCESS;
}

