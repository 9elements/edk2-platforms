/******************************************************************************
**                                                                           **
** INTEL CONFIDENTIAL                                                        **
**                                                                           **
** Copyright (c) 2020 - 2022, Intel Corporation. All rights reserved.        **
**                                                                           **
** The source code contained or described herein and all documents related   **
** to the source code ("Material") are owned by Intel Corporation or its     **
** suppliers or licensors.  Title to the Material remains with Intel         **
** Corporation or its suppliers and licensors.  The Material contains trade  **
** secrets and proprietary and confidential information of Intel or its      **
** suppliers and licensors.  The Material is protected by worldwide          **
** copyright and trade secret laws and treaty provisions.  No part of the    **
** Material may be used, copied, reproduced, modified, published, uploaded,  **
** posted, transmitted, distributed, or disclosed in any way without Intel's **
** prior express written permission.                                         **
**                                                                           **
** No license under any patent, copyright, trade secret or other             **
** intellectual property right is granted to or conferred upon you by        **
** disclosure or delivery of the Materials, either expressly, by             **
** implication, inducement, estoppel or otherwise.  Any license under such   **
** intellectual property rights must be express and approved by Intel in     **
** writing.                                                                  **
**                                                                           **
******************************************************************************/
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
