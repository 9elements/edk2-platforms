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
#include "CommonDriver.h"

#include "Hii/Hii.h"
#include "Hii/FormsetStd/HiiCommonDep.h"

#include <Library/HiiLib.h>


STATIC BOOLEAN mActionChangingBlinked = FALSE;

/** HII on action changing callback - validates values passed to forms by user.

   @param[in]      UndiPrivateData  Pointer to driver private data structure
   @param[in]      QuestionId       Question ID related to specific HII content
   @param[in]      Type             Type specifying question value
   @param[in,out]  Value            Question value
   @param[out]     ActionRequest    On return, action requested by the callback function. (unused here)

   @retval   EFI_SUCCESS             Callback completed successfully / user selection can be applied
   @retval   EFI_DEVICE_ERROR        Failed to get uncommited data from form browser
   @retval   EFI_DEVICE_ERROR        Failed to set uncommited data in form browser
   @retval   EFI_DEVICE_ERROR        EFI_STRING_ID specified by Value->string field is not
                                     present in string package or action is not permitted
   @retval   EFI_DEVICE_ERROR        Failed to verify whether user selection can be applied
   @retval   EFI_DEVICE_ERROR        User selection cannot be applied
**/
EFI_STATUS
HiiOnActionChanging (
  IN     UNDI_PRIVATE_DATA           *UndiPrivateData,
  IN     EFI_QUESTION_ID             QuestionId,
  IN     UINT8                       Type,
  IN OUT EFI_IFR_TYPE_VALUE          *Value,
  OUT    EFI_BROWSER_ACTION_REQUEST  *ActionRequest
  )
{
  HII_STD_VARSTORE          HiiDrvConfig;
  CHAR8                     AsciiString[256];
  EFI_STRING                UnicodeString;
  BOOLEAN                   ValueValid         = TRUE;

  ZeroMem (&HiiDrvConfig, sizeof (HiiDrvConfig));

  if (!HiiGetBrowserData (NULL, NULL, sizeof (HiiDrvConfig), (UINT8 *) &HiiDrvConfig)) {
    return EFI_DEVICE_ERROR;
  }

  if (Type == EFI_IFR_TYPE_STRING) {
    UnicodeString = HiiGetString (UndiPrivateData->HiiInfo.HiiPkgListHandle, Value->string, NULL);
    IF_NULL_RETURN (UnicodeString, EFI_DEVICE_ERROR);

    UnicodeStrToAsciiStrS (UnicodeString, AsciiString, sizeof (AsciiString));
  }

  switch (QuestionId) {
  case QUESTION_ID_BLINK_LED:
    if (Type == EFI_IFR_TYPE_NUM_SIZE_16) {
      BlinkLeds (UndiPrivateData, &Value->u16);

      // After blinking the LED, always clear the Blink LEDs question back to 0.
      Value->u16 = 0;
      HiiDrvConfig.BlinkLed = 0;
      mActionChangingBlinked = TRUE;
      if (!HiiSetBrowserData (NULL, NULL, sizeof (HiiDrvConfig), (UINT8 *) &HiiDrvConfig, NULL)) {
        return EFI_DEVICE_ERROR;
      }
    }
    break;

  default:
    ValueValid = TRUE;
    break;
  }

  return ValueValid ? EFI_SUCCESS : EFI_DEVICE_ERROR;
}

/** HII on action changed callback - updates fields in uncommited browser configuration
   in case it's needed.

   @param[in]      UndiPrivateData  Pointer to driver private data structure
   @param[in]      QuestionId       Question ID related to specific HII content
   @param[in]      Type             Type specifying question value
   @param[in,out]  Value            Question value
   @param[out]     ActionRequest    On return, action requested by the callback function. (unused here)

   @retval   EFI_SUCCESS       Callback completed successfully
   @retval   EFI_DEVICE_ERROR  Failed to get uncommited data from form browser
   @retval   EFI_DEVICE_ERROR  Failed to set uncommited data in form browser
**/
EFI_STATUS
HiiOnActionChanged (
  IN     UNDI_PRIVATE_DATA           *UndiPrivateData,
  IN     EFI_QUESTION_ID             QuestionId,
  IN     UINT8                       Type,
  IN OUT EFI_IFR_TYPE_VALUE          *Value,
  OUT    EFI_BROWSER_ACTION_REQUEST  *ActionRequest
  )
{
  HII_STD_VARSTORE   HiiDrvConfig;
  BOOLEAN            UpdateBrowserData = FALSE;

  ZeroMem (&HiiDrvConfig, sizeof (HiiDrvConfig));

  if (!HiiGetBrowserData (NULL, NULL, sizeof (HiiDrvConfig), (UINT8 *) &HiiDrvConfig)) {
    return EFI_DEVICE_ERROR;
  }

  switch (QuestionId) {
  case QUESTION_ID_BLINK_LED:
    if (Type == EFI_IFR_TYPE_NUM_SIZE_16) {
      if (!mActionChangingBlinked) {
        BlinkLeds (UndiPrivateData, &Value->u16);
      }

      // After blinking the LED, always clear the Blink LEDs question back to 0.
      Value->u16 = 0;
      HiiDrvConfig.BlinkLed = 0;
      UpdateBrowserData = TRUE;
    }
    break;


  default:
    break;
  }

  if (UpdateBrowserData) {
    if (!HiiSetBrowserData (NULL, NULL, sizeof (HiiDrvConfig), (UINT8 *) &HiiDrvConfig, NULL)) {
      return EFI_DEVICE_ERROR;
    }
  }

  return EFI_SUCCESS;
}
