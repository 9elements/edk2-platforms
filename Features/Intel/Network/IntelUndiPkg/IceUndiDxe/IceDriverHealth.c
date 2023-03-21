/**************************************************************************

Copyright (c) 2020-2022, Intel Corporation. All rights reserved.

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

#define MSG_INFO_ROLLBACK_MODE "Firmware rollback mode detected. Current version is NVM:%x.%02x 0x%08x %d.%d.%d, FW:%d.%d. Device may " \
  "exhibit limited functionality. Refer to the Intel(R) Ethernet Adapters and Devices User Guide for details on firmware rollback mode."
CHAR8 mRollbackMsg[MAX_DRIVER_HEALTH_ERROR_STRING] = {'\0'}; // placeholder, will be created at runtime

typedef enum {
  ERR_FW_NEWER_THAN_EXPECTED = 0,
  ERR_FW_OLDER_THAN_EXPECTED,
  ERR_FW_INCOMPATIBLE,
  ERR_FW_ROLLBACK_MODE,
  ERR_FW_RECOVERY_MODE,
  ERR_XCEIVER_MODULE_UNQUALIFIED,
  ERR_TOPOLOGY_MEDIA_CONFLICT,
  ERR_MODULE_POWER_UNSUPPORTED,
  ERR_MODULE_NVM_INCOMPATIBLE,
  ERR_PORT_CFG_UNSUPPORTED,
  ERR_END
} DRIVER_HEALTH_ERR_INDEX;

HEALTH_MSG_ENTRY mDriverHealthEntry[] = {
  /* HII string ID,                                     Message */
  {STRING_TOKEN (STR_FW_HEALTH_MESSAGE),                "The UEFI driver for the device detected a newer version of the NVM image than expected. Please install the most recent version of the UEFI driver." },
  {STRING_TOKEN (STR_FW_HEALTH_MESSAGE),                "The UEFI driver for the device detected an older version of the NVM image than expected. Please update the NVM image."                              },
  {STRING_TOKEN (STR_FW_HEALTH_MESSAGE),                "The UEFI driver for the device stopped because the NVM image is newer than expected. You must install the most recent version of the UEFI driver."  },
  {STRING_TOKEN (STR_FW_ROLLBACK_MESSAGE),              mRollbackMsg},
  {STRING_TOKEN (STR_FW_HEALTH_MESSAGE),                "Firmware recovery mode detected. Initialization failed."                                                                                            },
  {STRING_TOKEN (STR_XCEIVER_HEALTH_MESSAGE),           "Rx/Tx is disabled on this device because an unsupported SFP+ module type was detected. Refer to the Intel(R) Network Adapters and Devices User Guide for a list of supported modules."},
  {STRING_TOKEN (STR_TOPO_MEDIA_MESSAGE),               "Possible mis-configuration of the Ethernet port detected, please use Intel (R) Ethernet Port Configuration Tool utility to address the issue."},
  {STRING_TOKEN (STR_MODULE_PWR_UNSUPPORTED_MESSAGE),   "The module's power requirements exceed the device's power supply. Cannot start link."},
  {STRING_TOKEN (STR_MODULE_NVM_INCOMPATIBLE_MESSAGE),  "The installed module is incompatible with the device's NVM image. Cannot start link."},
  {STRING_TOKEN (STR_PORT_CFG_UNSUPPORTED_MESSAGE),     "The current device configuration does not support the current port option. Please choose a valid port option."},
};


/** Checks if FW is in rollback mode & if yes, prepares informational
 * health string.

   @param[in]    UndiPrivateData    Driver private data structure

   @retval   TRUE   FW rollback detected
   @retval   FALSE  FW rollback not detected
**/
BOOLEAN
IsRollbackMode (
  IN  UNDI_PRIVATE_DATA  *UndiPrivateData
  )
{
  struct ice_hw       *Hw;

  ASSERT (UndiPrivateData != NULL);

  Hw  = &UndiPrivateData->NicInfo.Hw;

  if (ice_get_fw_mode (Hw) == ICE_FW_MODE_ROLLBACK) {
    AsciiSPrint (
      mDriverHealthEntry[ERR_FW_ROLLBACK_MODE].Msg,
      MAX_DRIVER_HEALTH_ERROR_STRING,
      MSG_INFO_ROLLBACK_MODE,
      Hw->flash.nvm.major,
      Hw->flash.nvm.minor,
      Hw->flash.nvm.eetrack,
      Hw->flash.orom.major,
      Hw->flash.orom.build,
      Hw->flash.orom.patch,
      Hw->fw_maj_ver,
      Hw->fw_min_ver
      );
    DEBUGPRINT (HEALTH, ("FW rollback detected\n"));
    return TRUE;
  }

  return FALSE;
}

/** Checks if FW is in operable state & if FW AQ version is compatible with SW AQ API version.

   @param[in]   UndiPrivateData  Driver private data structure
   @param[out]  ErrIdx           Index of FW error in global health error array. Valid only when
                                 return value is FALSE

   @retval   TRUE   FW is compatible & operable
   @retval   FALSE  FW is not operable
   @retval   FALSE  FW has incompatible AQ API version
**/
BOOLEAN
IsFirmwareCompatible (
  IN   UNDI_PRIVATE_DATA        *UndiPrivateData,
  OUT  DRIVER_HEALTH_ERR_INDEX  *ErrIdx
  )
{
  UINT16 FwAqApiMajor;
  UINT16 FwAqApiMinor;

  ASSERT (UndiPrivateData != NULL);
  ASSERT (ErrIdx != NULL);

  if (IsRecoveryMode (&UndiPrivateData->NicInfo)) {
    *ErrIdx = ERR_FW_RECOVERY_MODE;
    return FALSE;
  }

  FwAqApiMajor = UndiPrivateData->NicInfo.Hw.api_maj_ver;
  FwAqApiMinor = UndiPrivateData->NicInfo.Hw.api_min_ver;

  DEBUGPRINT (HEALTH, ("Queried   FW API : %d.%d\n", FwAqApiMajor, FwAqApiMinor));
  DEBUGPRINT (HEALTH, ("Supported SW API : %d.%d\n", EXP_FW_API_VER_MAJOR, EXP_FW_API_VER_MINOR));

  if (FwAqApiMajor > EXP_FW_API_VER_MAJOR) {
    *ErrIdx = ERR_FW_INCOMPATIBLE;
    return FALSE;
  }
  else if ((FwAqApiMajor == EXP_FW_API_VER_MAJOR) &&
          (FwAqApiMinor > (EXP_FW_API_VER_MINOR + 2)))
  {
    *ErrIdx = ERR_FW_NEWER_THAN_EXPECTED;
    return FALSE;
  }
  else if ((FwAqApiMajor < EXP_FW_API_VER_MAJOR) ||
          ((FwAqApiMinor + 2) < EXP_FW_API_VER_MINOR))
  {
    *ErrIdx = ERR_FW_OLDER_THAN_EXPECTED;
    return FALSE;
  }

  return TRUE;
}

#define TOPO_MEDIA_CONFLICT_MASK  (ICE_AQ_LINK_TOPO_CONFLICT     |  \
                                   ICE_AQ_LINK_MEDIA_CONFLICT    |  \
                                   ICE_AQ_LINK_TOPO_CORRUPT)
/** Check if transceiver (SFP/QSFP/fiber) module used for this port is qualified module.
    This function also checks if current port option is supported in current adapter configuration.

   @param[in]   UndiPrivateData       Driver private data structure.
   @param[out]  ErrorCount            On return, number of errors found, if any.
   @param[out]  ErrorIndexes          On return, array that holds found health error indexes (from global array).
                                      Valid only when ErrorCount != 0. Must be allocated by caller.
   @retval  EFI_SUCCESS               Qualification/Topology/Port Option information retrieved successfully.
   @retval  EFI_DEVICE_ERROR          Failed to retrieve updated link information from FW.
   @retval  !EFI_SUCCESS              One of the functions called returned an error.
**/
EFI_STATUS
CheckForLinkProblems (
  IN   UNDI_PRIVATE_DATA  *UndiPrivateData,
  OUT  UINT16             *ErrorCount,
  OUT  UINT16             *ErrorIndexes
  )
{
  struct ice_hw                       *Hw;
  struct ice_phy_info                 *Phy;
  enum ice_status                     IceStatus;
  BOOLEAN                             LinkUp;
  BOOLEAN                             ModuleQualified     = TRUE;
  BOOLEAN                             ModuleNvmCompatible = TRUE;
  EFI_STATUS                          Status;
  PORT_OPTION                         *PortOption;
  PORT_OPTIONS_DATA                   *PortOptionsData;

  ASSERT (UndiPrivateData != NULL);

  Hw  = &UndiPrivateData->NicInfo.Hw;
  Phy = &Hw->port_info->phy;

  Phy->get_link_info = TRUE;
  IceStatus = ice_get_link_status (Hw->port_info, &LinkUp);
  if (IceStatus != ICE_SUCCESS) {
    DEBUGPRINT (CRITICAL, ("ice_get_link_status failed with: %d\n", IceStatus));
    return EFI_DEVICE_ERROR;
  }

  if ((Phy->link_info.link_info & ICE_AQ_MEDIA_AVAILABLE) != 0) {
    // If link is up then module is qualified.
    // In case no link is detected, module qualification check is required.
    if (!LinkUp) {
      if ((Phy->link_info.an_info & ICE_AQ_QUALIFIED_MODULE) == 0) {
        DEBUGPRINT (HEALTH, ("Transceiver not qualified\n"));
        ModuleQualified = FALSE;
      }

      if ((Phy->link_info.link_cfg_err & ICE_AQ_LINK_MODULE_POWER_UNSUPPORTED) != 0) {
        DEBUGPRINT (HEALTH, ("The module's power requirements exceed the device's power supply. Cannot start link.\n"));
        AddHealthError (ErrorCount, ErrorIndexes, ERR_MODULE_POWER_UNSUPPORTED);
      }

      if ((Phy->link_info.link_cfg_err & ICE_AQ_LINK_INVAL_MAX_POWER_LIMIT) != 0) {
        DEBUGPRINT (HEALTH, ("The installed module is incompatible with the device's NVM image. Cannot start link.\n"));
        ModuleNvmCompatible = FALSE;
      }
    }

    if ((Phy->link_info.link_cfg_err & ICE_AQ_LINK_EXTERNAL_PHY_LOAD_FAILURE) != 0) {
      DEBUGPRINT (
        HEALTH,
        ("Device failed to load the FW for the external PHY. Please download and install the latest NVM for your device and try again.\n")
        );
      ModuleNvmCompatible = FALSE;
    }

    if ((Phy->link_info.topo_media_conflict & TOPO_MEDIA_CONFLICT_MASK) != 0) {
      DEBUGPRINT (HEALTH, ("Media/topology conflict\n"));
      AddHealthError (ErrorCount, ErrorIndexes, ERR_TOPOLOGY_MEDIA_CONFLICT);
    }

    if ((Phy->link_info.topo_media_conflict & ICE_AQ_LINK_TOPO_UNSUPP_MEDIA) != 0) {
      DEBUGPRINT (HEALTH, ("Unsupported media/transceiver\n"));
      ModuleQualified = FALSE;
    }

    if (!ModuleQualified) {
      AddHealthError (ErrorCount, ErrorIndexes, ERR_XCEIVER_MODULE_UNQUALIFIED);
    }

    if (!ModuleNvmCompatible) {
      AddHealthError (ErrorCount, ErrorIndexes, ERR_MODULE_NVM_INCOMPATIBLE);
    }
  }

  // Check if current port option is not valid (not supported by this adapter)
  Status = GetPortOptionsForAllPorts (&UndiPrivateData->NicInfo, &PortOptionsData);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  PortOption = &PortOptionsData->PortOptions[PortOptionsData->Active];
  if (PortOption->PortOptionInvalid) {
    DEBUGPRINT (HEALTH, ("The current device configuration does not support the current port option. Please choose a valid port option.\n"));
    AddHealthError (ErrorCount, ErrorIndexes, ERR_PORT_CFG_UNSUPPORTED);
  }

  FreePool (PortOptionsData);

  return EFI_SUCCESS;
}

/** Retrieves adapter specific health status information from SW/FW/HW.

   @param[in]   UndiPrivateData   Driver private data structure
   @param[out]  ErrorCount        On return, number of errors found, if any
   @param[out]  ErrorIndexes      On return, array that holds found health error indexes (from global array).
                                  Valid only when ErrorCount != 0. Must be allocated by caller

   @retval  EFI_SUCCESS            Adapter health information retrieved successfully
   @retval  EFI_DEVICE_ERROR       Failed to retrieve link/module/topology health info
**/
EFI_STATUS
GetAdapterHealthStatus (
  IN   UNDI_PRIVATE_DATA  *UndiPrivateData,
  OUT  UINT16             *ErrorCount,
  OUT  UINT16             *ErrorIndexes
  )
{
  EFI_STATUS               Status;
  DRIVER_HEALTH_ERR_INDEX  ErrIdx           = ERR_END;

  ASSERT (UndiPrivateData != NULL);
  ASSERT (ErrorCount != NULL);

  *ErrorCount = 0;

  if (!IsFirmwareCompatible (UndiPrivateData, &ErrIdx)) {
    DEBUGPRINT (HEALTH, ("Improper FW state/version, err idx: %d\n", ErrIdx, "\n"));
    AddHealthError (ErrorCount, ErrorIndexes, ErrIdx);
  }

  // Check module qualification && rollback only when FW is in good state
  if ((ErrIdx != ERR_FW_RECOVERY_MODE) &&
      (ErrIdx != ERR_FW_INCOMPATIBLE))
  {
    if (IsRollbackMode (UndiPrivateData)) {
      AddHealthError (ErrorCount, ErrorIndexes, ERR_FW_ROLLBACK_MODE);
    }
    Status = CheckForLinkProblems (
               UndiPrivateData,
               ErrorCount,
               ErrorIndexes
               );
    if (EFI_ERROR (Status)) {
      return EFI_DEVICE_ERROR;
    }
  }

  return EFI_SUCCESS;
}
