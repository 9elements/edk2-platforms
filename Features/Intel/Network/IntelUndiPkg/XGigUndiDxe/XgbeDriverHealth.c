/**************************************************************************

Copyright (c) 2020, Intel Corporation. All rights reserved.

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

typedef enum {
  ERR_FW_RECOVERY_MODE = 0,
  ERR_XCEIVER_MODULE_UNQUALIFIED,
  ERR_END
} DRIVER_HEALTH_ERR_INDEX;

HEALTH_MSG_ENTRY mDriverHealthEntry[] = {
  /* HII string ID,                           Message */
  {STRING_TOKEN (STR_FW_HEALTH_MESSAGE),      "Firmware recovery mode detected. Initialization failed."},
  {STRING_TOKEN (STR_XCEIVER_HEALTH_MESSAGE), "Rx/Tx is disabled on this device because an unsupported SFP+ module type was detected. Refer to the Intel(R) Network Adapters and Devices User Guide for a list of supported modules."}
};


/** Retrieves adapter specific health status information from SW/FW/HW.

   @param[in]   UndiPrivateData   Driver private data structure
   @param[out]  ErrorCount        On return, number of errors found, if any
   @param[out]  ErrorIndexes      On return, array that holds found health error indexes (from global array).
                                  Valid only when ErrorCount != 0. Must be allocated by caller

   @retval  EFI_SUCCESS            Adapter health information retrieved successfully
**/
EFI_STATUS
GetAdapterHealthStatus (
  IN   UNDI_PRIVATE_DATA  *UndiPrivateData,
  OUT  UINT16             *ErrorCount,
  OUT  UINT16             *ErrorIndexes
  )
{
  DRIVER_DATA      *AdapterInfo;
  struct ixgbe_hw  *Hw;

  ASSERT (UndiPrivateData != NULL);
  ASSERT (ErrorCount != NULL);

  AdapterInfo = &UndiPrivateData->NicInfo;
  Hw          = &AdapterInfo->Hw;

  *ErrorCount = 0;

  if (ixgbe_fw_recovery_mode (Hw)) {
    DEBUGPRINT (HEALTH, ("Improper FW state - recovery mode\n"));
    AddHealthError (ErrorCount, ErrorIndexes, ERR_FW_RECOVERY_MODE);
  } // Check module qualification only when FW is in good state
  else if ((Hw->phy.media_type == ixgbe_media_type_fiber) ||
           (Hw->phy.media_type == ixgbe_media_type_fiber_qsfp))
  {
    AdapterInfo->XceiverModuleQualified = GetModuleQualificationResult (AdapterInfo);
    if (!AdapterInfo->XceiverModuleQualified) {
      DEBUGPRINT (HEALTH, ("Transceiver module not qualified\n"));
      AddHealthError (ErrorCount, ErrorIndexes, ERR_XCEIVER_MODULE_UNQUALIFIED);
    }
  }

  return EFI_SUCCESS;
}
