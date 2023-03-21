/**************************************************************************

Copyright (c) 2013 - 2020, Intel Corporation. All rights reserved.

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
#include "DriverHealth.h"

#include <Protocol/HiiString.h>
#include <Library/HiiLib.h>

#define MAX_DRIVER_HEALTH_ERRORS             15

extern HEALTH_MSG_ENTRY      mDriverHealthEntry[];


/** Helper safe function to add health error (exceeding number will be handled in
   GetControllerHealthStatus()).

   @param[out]  ErrorCount        Pointer to variable that holds error count
   @param[out]  ErrorIndexes      Pointer to array that holds error indexes
   @param[in]   ErrIdx            Index of health error in global arrays
**/
VOID
AddHealthError (
  OUT  UINT16  *ErrorCount,
  OUT  UINT16  *ErrorIndexes,
  IN   UINT16  ErrIdx
  )
{
  ASSERT (ErrorCount != NULL);
  ASSERT (ErrorIndexes != NULL);

  if (*ErrorCount < MAX_DRIVER_HEALTH_ERRORS) {
    ErrorIndexes[*ErrorCount] = ErrIdx;
  }

  (*ErrorCount)++;
}

/** Return the health status of the controller.

   @param[in]      UndiPrivateData      Driver private data structure
   @param[out]     DriverHealthStatus   EfiDriverHealthStatusHealthy/Failed, depending if errors are reported
   @param[in,out]  MessageList          Pointer to pointer of the message list to be returned (only when Unhealthy),
                                        NULL on input if MessageList is not requested

   @retval   EFI_SUCCESS              Health status retrieved successfully
   @retval   EFI_SUCCESS              Unhealthy, but MessageList is NULL or HII is not supported on this port
   @retval   EFI_INVALID_PARAMETER    Invalid parameter passed
   @retval   EFI_OUT_OF_RESOURCES     We are out of resources either for allocating MessageList
                                      or setting HII string
   @retval   EFI_OUT_OF_RESOURCES     Number of adapter reported errors exceeds MAX_DRIVER_HEALTH_ERRORS
   @retval   EFI_DEVICE_ERROR         Failed to retrieve health status from adapter HW
**/
EFI_STATUS
GetControllerHealthStatus (
  IN      UNDI_PRIVATE_DATA              *UndiPrivateData,
  OUT     EFI_DRIVER_HEALTH_STATUS       *DriverHealthStatus,
  IN OUT  EFI_DRIVER_HEALTH_HII_MESSAGE  **MessageList          OPTIONAL
  )
{
  CHAR16                         ErrorString[MAX_DRIVER_HEALTH_ERROR_STRING];
  UINT16                         ErrorIndexes[MAX_DRIVER_HEALTH_ERRORS];

  EFI_DRIVER_HEALTH_HII_MESSAGE  *MessageListArray;
  UINT16                         ErrorCount     = 0;
  EFI_STRING_ID                  StringId;
  UINT64                         MsgCode;
  EFI_STATUS                     Status;
  UINT16                         ErrIdx;


  if ((UndiPrivateData == NULL) ||
      (DriverHealthStatus == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  Status = GetAdapterHealthStatus (UndiPrivateData, &ErrorCount, ErrorIndexes);
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  if (ErrorCount == 0) {
    DEBUGPRINT (HEALTH, ("Controller is healthy\n"));
    *DriverHealthStatus = EfiDriverHealthStatusHealthy;
    return EFI_SUCCESS;
  } else {
    DEBUGPRINT (HEALTH, ("Health error count: %d\n", ErrorCount));
    *DriverHealthStatus = EfiDriverHealthStatusFailed;
    if (ErrorCount > MAX_DRIVER_HEALTH_ERRORS) {
      return EFI_OUT_OF_RESOURCES;
    }
  }

  // Create error message string
  if ((MessageList == NULL) ||
      (UndiPrivateData->HiiInfo.HiiPkgListHandle == NULL))
  {
    DEBUGPRINT (HEALTH, ("Text messages are not requested or HII is not supported on this port\n"));
    return EFI_SUCCESS;
  }


  // Need to allocate space for error count + 1 message entries:
  // - error count for the message we need to pass to UEFI BIOS
  // - one for NULL entry indicating the end of list
  MessageListArray = AllocateZeroPool ((ErrorCount + 1) * sizeof (EFI_DRIVER_HEALTH_HII_MESSAGE));
  if (MessageListArray == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  for (UINT16 MsgIdx = 0; MsgIdx < ErrorCount; MsgIdx++) {
    ErrIdx = ErrorIndexes[MsgIdx];
    UnicodeSPrintAsciiFormat (ErrorString, sizeof (ErrorString), "%a", mDriverHealthEntry[ErrIdx].Msg);
    MsgCode = 0;
    StringId = HiiSetString (
                 UndiPrivateData->HiiInfo.HiiPkgListHandle,
                 mDriverHealthEntry[ErrIdx].StringId,
                 ErrorString,
                 NULL
                 );
    if (StringId == 0) {
      FreePool (MessageListArray);
      *MessageList = NULL;
      return EFI_OUT_OF_RESOURCES;
    }
    MessageListArray[MsgIdx].HiiHandle   = UndiPrivateData->HiiInfo.HiiPkgListHandle;
    MessageListArray[MsgIdx].StringId    = mDriverHealthEntry[ErrIdx].StringId;
    MessageListArray[MsgIdx].MessageCode = MsgCode;
  }

  // Indicate the end of list by setting HiiHandle to NULL
  MessageListArray[ErrorCount].HiiHandle   = NULL;
  MessageListArray[ErrorCount].StringId    = 0;
  MessageListArray[ErrorCount].MessageCode = 0;

  *MessageList = MessageListArray;

  return EFI_SUCCESS;
}

/** Return the cumulative health status of all controllers
   managed by the driver image.

   @param[out]   DriverHealthStatus   Controller health status to be returned

   @retval   EFI_SUCCESS              Procedure returned successfully
   @retval   EFI_INVALID_PARAMETER    DriverHealthStatus is NULL
   @retval   EFI_DEVICE_ERROR         Failed to get controller health status
**/
EFI_STATUS
GetCumulativeHealthStatus (
  OUT EFI_DRIVER_HEALTH_STATUS *DriverHealthStatus
  )
{
  EFI_STATUS                Status;
  EFI_DRIVER_HEALTH_STATUS  HealthStatus;
  UNDI_PRIVATE_DATA         *Device;

  if (DriverHealthStatus == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  *DriverHealthStatus = EfiDriverHealthStatusHealthy;

  // Iterate through all controllers managed by this instance of driver and
  // ask them about their health status
  FOREACH_ACTIVE_CONTROLLER (Device) {
    if (Device->NicInfo.Hw.device_id != 0) {
      Status = GetControllerHealthStatus (Device, &HealthStatus, NULL);
      if (EFI_ERROR (Status)) {
        DEBUGPRINT (CRITICAL, ("GetControllerHealthStatus failed - %d\n", Status));
        return Status;
      }
      if (HealthStatus != EfiDriverHealthStatusHealthy) {
        *DriverHealthStatus = EfiDriverHealthStatusFailed;
        break;
      }
    }
  }

  return EFI_SUCCESS;
}

/** Retrieves the health status of a controller in the platform.

   @param[in]   This               Driver health protocol instance
   @param[in]   ControllerHandle   Controller to retrieve the health status on
   @param[in]   ChildHandle        Child to retrieve the health status on
   @param[out]  HealthStatus       Pointer to resultant health status
   @param[out]  MessageList        List of resultant error messages
   @param[out]  FormHiiHandle      Hii handle containing HII form used when
                                   configuration is required

   @retval      EFI_SUCCESS            Health status successfully retrieved
   @retval      EFI_INVALID_PARAMETER  HealthStatus is NULL
   @retval      !EFI_SUCCESS           Failure to retrieve health status
**/
EFI_STATUS
EFIAPI
GetHealthStatus (
  IN  EFI_DRIVER_HEALTH_PROTOCOL     *This,
  IN  EFI_HANDLE                     ControllerHandle,  OPTIONAL
  IN  EFI_HANDLE                     ChildHandle,       OPTIONAL
  OUT EFI_DRIVER_HEALTH_STATUS       *HealthStatus,
  OUT EFI_DRIVER_HEALTH_HII_MESSAGE  **MessageList,     OPTIONAL
  OUT EFI_HII_HANDLE                 *FormHiiHandle     OPTIONAL
  )
{
  EFI_STATUS                Status;
  EFI_NII_POINTER_PROTOCOL  *NiiPointerProtocol;
  UNDI_PRIVATE_DATA         *UndiPrivateData;

  if (HealthStatus == NULL) {
    DEBUGPRINT (CRITICAL, ("HealthStatus is NULL\n"));
    return EFI_INVALID_PARAMETER;
  }

  // Assume the message list is empty
  if (MessageList != NULL) {
    *MessageList = NULL;
  }

  // Assume no Form HII Handle is returned by the function
  if (FormHiiHandle != NULL) {
    *FormHiiHandle = NULL;
  }

  // Check if ControllerHandle is valid: should be NULL or
  // value managed by our driver. The same for ChildHandle
  if (ControllerHandle != NULL) {
    Status = EfiTestManagedDevice (
               ControllerHandle,
               gUndiDriverBinding.DriverBindingHandle,
               &gEfiDevicePathProtocolGuid
               );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    if (ChildHandle != NULL) {
      // check if ChildHandle matches ControllerHandle
      Status = EfiTestChildHandle (
                 ControllerHandle,
                 ChildHandle,
                 &gEfiPciIoProtocolGuid
                 );
      if (EFI_ERROR (Status)) {
        return Status;
      }
    }

    DEBUGPRINT (HEALTH, ("EFI_DRIVER_HEALTH_PROTOCOL.GetHealthStatus() - Single controller\n"));
    //  Open an instance for the NiiPointerProtocol to get the UNDI_PRIVATE_DATA pointer
    Status = gBS->OpenProtocol (
                    ControllerHandle,
                    &gEfiNiiPointerGuid,
                    (VOID * *) &NiiPointerProtocol,
                    gUndiDriverBinding.DriverBindingHandle,
                    ControllerHandle,
                    EFI_OPEN_PROTOCOL_GET_PROTOCOL
                    );
    if (EFI_ERROR (Status)) {
      DEBUGPRINT (CRITICAL, ("OpenProtocol(NII pointer) failed - %r\n", Status));
      return Status;
    }

    UndiPrivateData = UNDI_PRIVATE_DATA_FROM_THIS (NiiPointerProtocol->NiiProtocol31);

    Status = GetControllerHealthStatus (UndiPrivateData, HealthStatus, MessageList);
    if (EFI_ERROR (Status)) {
      DEBUGPRINT (CRITICAL, ("UndiGetControllerHealthStatus - %r\n", Status));
      return Status;
    }
  } else {
    DEBUGPRINT (HEALTH, ("EFI_DRIVER_HEALTH_PROTOCOL.GetHealthStatus() - Cumulative\n"));

    Status = GetCumulativeHealthStatus (HealthStatus);
    if (EFI_ERROR (Status)) {
      DEBUGPRINT (CRITICAL, ("GetCumulativeHealthStatus - %r\n", Status));
      return Status;
    }
  }

  return EFI_SUCCESS;
}

/** Functionality is unsupported

   @param[in]   This                  Driver Health Protocol instance
   @param[in]   ControllerHandle      Controller to repair handle
   @param[in]   ChildHandle           Child to repair handle
   @param[in]   ProgressNotification  Notification function to report progress

   @retval   EFI_UNSUPPORTED   This function is unsupported
**/
EFI_STATUS
EFIAPI
Repair (
  IN  EFI_DRIVER_HEALTH_PROTOCOL                *This,
  IN  EFI_HANDLE                                ControllerHandle,
  IN  EFI_HANDLE                                ChildHandle,          OPTIONAL
  IN  EFI_DRIVER_HEALTH_REPAIR_NOTIFY           ProgressNotification  OPTIONAL
  )
{
  DEBUGPRINT (HEALTH, ("EFI_DRIVER_HEALTH_PROTOCOL.Repair() called, but unsupported\n"));
  return EFI_UNSUPPORTED;
}

EFI_DRIVER_HEALTH_PROTOCOL gUndiDriverHealthProtocol = {
  GetHealthStatus,
  Repair
};
