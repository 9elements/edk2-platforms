/**************************************************************************

Copyright (c) 2016 - 2022, Intel Corporation. All rights reserved.

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
#include "StartStop.h"

/* Global variables */

EFI_GUID gEfiStartStopProtocolGuid = EFI_DRIVER_STOP_PROTOCOL_GUID;

/* Function definitions */

/** Issues a call to stop the driver so diagnostic application can access the hardware.

   @param[in]   This   pointer to the EFI_DRIVER_STOP_PROTOCOL instance.

   @retval      EFI_SUCCESS   driver is stopped successfully
**/
EFI_STATUS
EFIAPI
StopDriver (
  IN EFI_DRIVER_STOP_PROTOCOL *This,
  IN BOOLEAN                  StopAq
  )
{
  EFI_STATUS         Status = EFI_SUCCESS;
  UNDI_PRIVATE_DATA  *UndiPrivateData;

  DEBUGPRINT (INIT, ("Entering StopDriver\n"));

  UndiPrivateData = UNDI_PRIVATE_DATA_FROM_DRIVER_STOP (This);

  if (UndiPrivateData->NicInfo.DriverBusy) {
    // Already stopped
    return Status;
  }

  UndiPrivateData->NicInfo.DriverBusy = TRUE;

  if (UndiPrivateData->IsChildInitialized) {
    IceShutdown (&UndiPrivateData->NicInfo);
  }
  if (StopAq) {
    //Shutdown AQ
    ice_shutdown_all_ctrlq (&UndiPrivateData->NicInfo.Hw, TRUE);
  }
  DEBUGPRINT (INIT, ("Exit\n"));
  return Status;
}

/** Issues a call to start the driver after diagnostic application has completed.

   @param[in]   This   Pointer to the EFI_DRIVER_STOP_PROTOCOL instance.

   @retval      EFI_SUCCESS       If driver has restarted successfully
   @retval      EFI_DEVICE_ERROR  Hw initialization failed
   @retval      EFI_DEVICE_ERROR  PF reset failed
**/
EFI_STATUS
EFIAPI
StartDriver (
  IN EFI_DRIVER_STOP_PROTOCOL *This
  )
{
  EFI_STATUS             Status = EFI_SUCCESS;
  UNDI_PRIVATE_DATA      *UndiPrivateData;
  enum                   ice_status IceStatus;

  DEBUGPRINT (INIT, ("Entering StartDriver\n"));

  UndiPrivateData = UNDI_PRIVATE_DATA_FROM_DRIVER_STOP (This);

  if (!UndiPrivateData->NicInfo.DriverBusy) {
    // Already started
    return Status;
  }

  UndiPrivateData->NicInfo.HwInitialized = FALSE;


  // Reset PF and reinitialize AQ
  IceStatus = ice_reset (&UndiPrivateData->NicInfo.Hw, ICE_RESET_PFR);
  if (IceStatus != ICE_SUCCESS) {
    DEBUGPRINT (CRITICAL, ("ice_reset returned %d\n", IceStatus));
    return EFI_DEVICE_ERROR;
  }
  IceStatus = ice_init_all_ctrlq (&UndiPrivateData->NicInfo.Hw);
  if (IceStatus != ICE_SUCCESS) {
    DEBUGPRINT (CRITICAL, ("ice_init_all_ctrlq returned %d\n", IceStatus));
    return EFI_DEVICE_ERROR;
  }

  if (UndiPrivateData->IsChildInitialized) {
    Status = IceInitHw (&UndiPrivateData->NicInfo);
    if (EFI_ERROR (Status)) {
      DEBUGPRINT (CRITICAL, ("IceInitHw returned %r\n", Status));
      return Status;
    }

  IceSetMcastList (&UndiPrivateData->NicInfo);
  }

  UndiPrivateData->NicInfo.DriverBusy = FALSE;

  DEBUGPRINT (INIT, ("Exit\n"));

  return Status;
}

/* Protocol structure definition */

EFI_DRIVER_STOP_PROTOCOL gUndiDriverStop = {
  StopDriver,
  StartDriver
};

