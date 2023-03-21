/**************************************************************************

Copyright (c) 2012 - 2020, Intel Corporation. All rights reserved.

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
#include "StartStop.h"

EFI_GUID gEfiStartStopProtocolGuid = EFI_DRIVER_STOP_PROTOCOL_GUID;

/** Issues a call to stop the driver so diagnostic application can access the hardware.

   @param[in]   This       Pointer to the EFI_DRIVER_STOP_PROTOCOL instance.

   @retval   EFI_SUCCESS   Driver is stopped successfully
**/
EFI_STATUS
EFIAPI
StopDriver (
  IN EFI_DRIVER_STOP_PROTOCOL *This
  )
{
  EFI_STATUS         Status = EFI_SUCCESS;
  UNDI_PRIVATE_DATA *XgbePrivate;

  DEBUGPRINT (DIAG, ("Entering StopDriver\n"));
  DEBUGWAIT (DIAG);

  XgbePrivate = UNDI_PRIVATE_DATA_FROM_DRIVER_STOP (This);

  XgbePrivate->NicInfo.DriverBusy = TRUE;

  return Status;
}

/** Issues a call to start the driver after diagnostic application has completed.

   @param[in]   This       Pointer to the EFI_DRIVER_STOP_PROTOCOL instance.

   @retval   EFI_SUCCESS       If driver has restarted successfully
   @retval   EFI_DEVICE_ERROR  Failed to initialize hardware
**/
EFI_STATUS
EFIAPI
StartDriver (
  IN EFI_DRIVER_STOP_PROTOCOL *This
  )
{
  EFI_STATUS         Status = EFI_SUCCESS;
  UNDI_PRIVATE_DATA *XgbePrivate;
  BOOLEAN            ReceiveStarted;

  DEBUGPRINT (DIAG, ("Entering StartDriver\n"));
  DEBUGWAIT (DIAG);

  XgbePrivate = UNDI_PRIVATE_DATA_FROM_DRIVER_STOP (This);

  // Save off the value of ReceiveStarted as it will be reset by XgbeInitialize
  ReceiveStarted = XgbePrivate->NicInfo.RxRing.IsRunning;

  if ((XgbePrivate->NicInfo.State == PXE_STATFLAGS_GET_STATE_INITIALIZED)
    && XgbePrivate->IsChildInitialized)
  {
    XgbePrivate->NicInfo.HwInitialized = FALSE;
    XgbeInitialize (&XgbePrivate->NicInfo);
    DEBUGPRINT (DIAG, ("ixgbe_Inititialize complete\n"));

    //  Restart the receive unit if it was running on entry
    if (ReceiveStarted) {
      DEBUGPRINT (DIAG, ("RESTARTING RU\n"));
      DEBUGWAIT (DIAG);
      XgbeSetFilter (&XgbePrivate->NicInfo, XgbePrivate->NicInfo.RxFilter);
    }
  } else {

    // If the driver is not in the state INITIALIZED, we will still re-init the hardware
    // and bring link up so the device is ready to use.
    Status = XgbeInitHw (&XgbePrivate->NicInfo);
    if (EFI_ERROR (Status)) {
      DEBUGPRINT (CRITICAL, ("XgbeInitHw returns %r\n", Status));
    }
  }

  DEBUGPRINT (DIAG, ("ADAPTER RESET COMPLETE\n"));
  XgbePrivate->NicInfo.DriverBusy = FALSE;

  return Status;
}

/* Protocol structure definition and initialization */
EFI_DRIVER_STOP_PROTOCOL gUndiDriverStop = {
  StopDriver,
  StartDriver
};
