/**************************************************************************

Copyright (c) 2020 - 2022, Intel Corporation. All rights reserved.

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
#include "DeviceSupport.h"

typedef enum {
  ERR_FW_NEWER_THAN_EXPECTED = 0,
  ERR_FW_OLDER_THAN_EXPECTED,
  ERR_FW_INCOMPATIBLE,
  ERR_FW_REPEATED_RESETS,
  ERR_FW_RECOVERY_MODE,
  ERR_XCEIVER_MODULE_UNQUALIFIED,
  ERR_END
} DRIVER_HEALTH_ERR_INDEX;

HEALTH_MSG_ENTRY mDriverHealthEntry[] = {
  /* HII string ID,                           Message */
  {STRING_TOKEN (STR_FW_HEALTH_MESSAGE),      "The UEFI driver for the device detected a newer version of the NVM image than expected. Please install the most recent version of the UEFI driver." },
  {STRING_TOKEN (STR_FW_HEALTH_MESSAGE),      "The UEFI driver for the device detected an older version of the NVM image than expected. Please update the NVM image."                              },
  {STRING_TOKEN (STR_FW_HEALTH_MESSAGE),      "The UEFI driver for the device stopped because the NVM image is newer than expected. You must install the most recent version of the UEFI driver."  },
  {STRING_TOKEN (STR_FW_HEALTH_MESSAGE),      "Entering recovery mode due to repeated FW resets. This may take several minutes."                                                                   },
  {STRING_TOKEN (STR_FW_HEALTH_MESSAGE),      "Firmware recovery mode detected. Initialization failed."                                                                                            },
  {STRING_TOKEN (STR_XCEIVER_HEALTH_MESSAGE), "Rx/Tx is disabled on this device because an unsupported SFP+ module type was detected. Refer to the Intel(R) Network Adapters and Devices User Guide for a list of supported modules."},
};


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
  UINT16 SwAqApiMajor = I40E_FW_API_VERSION_MAJOR; // For now both FVL/FPK are same
  UINT16 SwAqApiMinor;
  UINT16 FwAqApiMajor;
  UINT16 FwAqApiMinor;

  ASSERT (UndiPrivateData != NULL);
  ASSERT (ErrIdx != NULL);

  if (UndiPrivateData->NicInfo.RepeatedFwResets) {
    *ErrIdx = ERR_FW_REPEATED_RESETS;
    return FALSE;
  }

  if (UndiPrivateData->NicInfo.RecoveryMode) {
    *ErrIdx = ERR_FW_RECOVERY_MODE;
    return FALSE;
  }

  SwAqApiMinor = I40E_FW_MINOR_VERSION (&UndiPrivateData->NicInfo.Hw);
  FwAqApiMajor = UndiPrivateData->NicInfo.Hw.aq.api_maj_ver;
  FwAqApiMinor = UndiPrivateData->NicInfo.Hw.aq.api_min_ver;

  DEBUGPRINT (HEALTH, ("Queried   FW API : %d.%d\n", FwAqApiMajor, FwAqApiMinor));
  DEBUGPRINT (HEALTH, ("Supported SW API : %d.%d\n", SwAqApiMajor, SwAqApiMinor));

  if (FwAqApiMajor > SwAqApiMajor) {
    *ErrIdx = ERR_FW_INCOMPATIBLE;
    return FALSE;
  }

  if ((FwAqApiMinor > SwAqApiMinor) &&
      (FwAqApiMajor == SwAqApiMajor))
  {
    *ErrIdx = ERR_FW_NEWER_THAN_EXPECTED;
    return FALSE;
  }

  // Throw a warning message only when NVM is from FVL3 or older
  if ((FwAqApiMajor == 1) &&
      (FwAqApiMinor < 4))
  {
    *ErrIdx = ERR_FW_OLDER_THAN_EXPECTED;
    return FALSE;
  }

  return TRUE;
}

/** Check if transceiver (SFP/QSFP/fiber) module used for this port is qualified module.

   @param[in]   UndiPrivateData   Driver private data structure
   @param[out]  ModuleQualified   Tells whether module is qualified
   @param[out]  ErrIdx            Index of transceiver module error in global health error array.
                                  Valid only when ModuleQualified == FALSE

   @retval  EFI_SUCCESS       Qualification information retrieved successfully.
   @retval  EFI_DEVICE_ERROR  Failed to retrieve updated link information from FW.
**/
EFI_STATUS
IsXceiverModuleQualified (
  IN   UNDI_PRIVATE_DATA        *UndiPrivateData,
  OUT  BOOLEAN                  *ModuleQualified,
  OUT  DRIVER_HEALTH_ERR_INDEX  *ErrIdx
  )
{
  EFI_STATUS               Status;
  struct i40e_link_status  *LinkInfo;

  ASSERT (UndiPrivateData != NULL);
  ASSERT (ModuleQualified != NULL);
  ASSERT (ErrIdx != NULL);

  LinkInfo = &UndiPrivateData->NicInfo.Hw.phy.link_info;

  Status = UpdateLinkInfo (&UndiPrivateData->NicInfo);
  if (EFI_ERROR (Status)) {
    DEBUGPRINT (CRITICAL, ("UpdateLinkInfo failed\n"));
    return EFI_DEVICE_ERROR;
  }

  // Assume TRUE (Link is down /OR module is qualified /OR qualification process is not available)
  *ModuleQualified = TRUE;

  if ((LinkInfo->link_info & I40E_AQ_MEDIA_AVAILABLE) &&
    (!(LinkInfo->link_info & I40E_AQ_LINK_UP)) &&
    (!(LinkInfo->an_info & I40E_AQ_QUALIFIED_MODULE)))
  {
    *ErrIdx = ERR_XCEIVER_MODULE_UNQUALIFIED;
    *ModuleQualified = FALSE;
  }

  return EFI_SUCCESS;
}

/** Retrieves adapter specific health status information from SW/FW/HW.

   @param[in]   UndiPrivateData   Driver private data structure
   @param[out]  ErrorCount        On return, number of errors found, if any
   @param[out]  ErrorIndexes      On return, array that holds found health error indexes (from global array).
                                  Valid only when ErrorCount != 0. Must be allocated by caller

   @retval  EFI_SUCCESS            Adapter health information retrieved successfully.
   @retval  EFI_DEVICE_ERROR       Failed to retrieve module qualification info.
**/
EFI_STATUS
GetAdapterHealthStatus (
  IN   UNDI_PRIVATE_DATA  *UndiPrivateData,
  OUT  UINT16             *ErrorCount,
  OUT  UINT16             *ErrorIndexes
  )
{
  BOOLEAN                  ModuleQualified;
  EFI_STATUS               Status;
  DRIVER_HEALTH_ERR_INDEX  ErrIdx           = ERR_END;

  ASSERT (UndiPrivateData != NULL);
  ASSERT (ErrorCount != NULL);

  *ErrorCount = 0;

  if (!IsFirmwareCompatible (UndiPrivateData, &ErrIdx)) {
    DEBUGPRINT (HEALTH, ("Improper FW state/version, err idx: %d\n", ErrIdx, "\n"));
    AddHealthError (ErrorCount, ErrorIndexes, ErrIdx);
  }

  // Check module qualification only when FW is in good state
  if ((ErrIdx != ERR_FW_RECOVERY_MODE) &&
      (ErrIdx != ERR_FW_INCOMPATIBLE))
  {
    Status = IsXceiverModuleQualified (UndiPrivateData, &ModuleQualified, &ErrIdx);
    if (EFI_ERROR (Status)) {
      return EFI_DEVICE_ERROR;
    }
    if (!ModuleQualified) {
      DEBUGPRINT (HEALTH, ("Transceiver module not qualified, err idx: %d\n", ErrIdx, "\n"));
      AddHealthError (ErrorCount, ErrorIndexes, ErrIdx);
    }
  }

  return EFI_SUCCESS;
}
