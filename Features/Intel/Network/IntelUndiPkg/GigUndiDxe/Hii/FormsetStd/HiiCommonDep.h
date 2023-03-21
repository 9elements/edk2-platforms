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
#ifndef HII_COMMON_DEP_H_
#define HII_COMMON_DEP_H_

#include "DeviceSupport.h"
#include "wol.h"

/** Performs operations before starting varstore config map processing for standard formset RouteConfig().

   @param[in]   UndiPrivateData  Pointer to driver private data structure
   @param[in]   VarStoreMapCfg   HII varstore map configuration structure
   @param[in]   HiiCfgData       Pointer to raw configuration data buffer (of varstore type)
   @param[in]   Configuration    RouteConfig Configuration string

   @retval      EFI_SUCCESS      Operation successful
   @retval      !EFI_SUCCESS     Failure with reason specific to adapter
**/
EFI_STATUS
HiiConfigMapPreRoute (
  IN       UNDI_PRIVATE_DATA     *UndiPrivateData,
  IN       HII_VARSTORE_MAP_CFG  *VarStoreMapCfg,
  IN       HII_STD_VARSTORE      *HiiCfgData,
  IN CONST EFI_STRING            Configuration
  );

/** Performs operations after varstore config map processing is finished for standard formset RouteConfig().

   @param[in]   UndiPrivateData  Pointer to driver private data structure
   @param[in]   VarStoreMapCfg   HII varstore map configuration structure

   @retval      EFI_SUCCESS      Operation successful
   @retval      !EFI_SUCCESS     NVM checksum update failed
   @retval      !EFI_SUCCESS     Failure with reason specific to adapter
**/
EFI_STATUS
HiiConfigMapPostRoute (
  IN  UNDI_PRIVATE_DATA     *UndiPrivateData,
  IN  HII_VARSTORE_MAP_CFG  *VarStoreMapCfg
  );

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
  );

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
  );


/** Gets default WOL status.

  @param[in]   UndiPrivateData       Pointer to driver private data structure
  @param[out]  WolStatus             Default WOL status

  @retval     EFI_SUCCESS            Operation successful
**/
EFI_STATUS
GetDefaultWolStatus (
  IN   UNDI_PRIVATE_DATA   *UndiPrivateData,
  OUT  ADAPTER_WOL_STATUS  *WolStatus
  );

/** Gets Alternate MAC address attribute.

  @param[in]   UndiPrivateData       Pointer to driver private data structure
  @param[out]  AltMacAddrUni         Resultant MAC address string

  @retval     EFI_SUCCESS            Operation successful
  @retval     !EFI_SUCCESS           Failed to get factory or alternate MAC address
**/
EFI_STATUS
GetAltMacAddr (
  IN   UNDI_PRIVATE_DATA  *UndiPrivateData,
  OUT  UINT16             *AltMacAddrUni
  );



/** Sets Alternate MAC address attribute.

  @param[in]  UndiPrivateData       Pointer to driver private data structure
  @param[in]  NewAltMacAddrUni      MAC address string

  @retval     EFI_SUCCESS            Operation successful
  @retval     EFI_INVALID_PARAMETER  Requested MAC address is multicast
  @retval     !EFI_SUCCESS           Failed to get current MAC address, restore default or set new
  @retval     !EFI_SUCCESS           UNICODE string MAC to raw binary MAC failed
**/
EFI_STATUS
SetAltMacAddr (
  IN  UNDI_PRIVATE_DATA  *UndiPrivateData,
  IN  UINT16             *NewAltMacAddrUni
  );




/** Gets branding string for adapter.

  @param[in]   UndiPrivateData  Pointer to driver private data structure
  @param[out]  BrandStr         Resultant string

  @retval      EFI_SUCCESS        Operation successful
**/
EFI_STATUS
GetBrandStr (
  IN   UNDI_PRIVATE_DATA  *UndiPrivateData,
  OUT  EFI_STRING         BrandStr
  );

/** Gets standard formset title string for adapter.

  @param[in]   UndiPrivateData  Pointer to driver private data structure
  @param[out]  FormSetTitleStr  Resultant string

  @retval      EFI_SUCCESS        Operation successful
**/
EFI_STATUS
GetFormSetTitleStr (
  IN   UNDI_PRIVATE_DATA  *UndiPrivateData,
  OUT  EFI_STRING         FormSetTitleStr
  );

/** Gets standard formset help string for adapter.

  @param[in]   UndiPrivateData  Pointer to driver private data structure
  @param[out]  FormSetHelpStr   Resultant string
  @param[in]   Language         Language for which string should be retrieved

  @retval      EFI_SUCCESS        Operation successful
  @retval      EFI_DEVICE_ERROR   Failed to retrieve EFI_STRING_ID of help string
  @retval      !EFI_SUCCESS       Failed to retrieve string from inventory package
**/
EFI_STATUS
GetFormSetHelpStr (
  IN         UNDI_PRIVATE_DATA  *UndiPrivateData,
  OUT        EFI_STRING         FormSetHelpStr,
  IN   CONST CHAR8              *Language
  );


/** Gets factory MAC address string.

  @param[in]   UndiPrivateData  Pointer to driver private data structure
  @param[out]  FactoryMacStr    Resultant string

  @retval      EFI_SUCCESS      Operation successful
  @retval      !EFI_SUCCESS     Failed to retrieve factory MAC address
**/
EFI_STATUS
GetFactoryMacStr (
  IN   UNDI_PRIVATE_DATA  *UndiPrivateData,
  OUT  EFI_STRING         FactoryMacStr
  );

/** Gets PCI Bus:Device:Function string.

  @param[in]   UndiPrivateData  Pointer to driver private data structure
  @param[out]  PciBdfStr        Resultant string

  @retval      EFI_SUCCESS      Operation successful
  @retval      !EFI_SUCCESS     Underlying function error
**/
EFI_STATUS
GetPciBdfStr (
  IN   UNDI_PRIVATE_DATA  *UndiPrivateData,
  OUT  EFI_STRING         PciBdfStr
  );

/** Gets Driver Name string in the same format as Component Name protocol.

  @param[in]   UndiPrivateData         Pointer to driver private data structure
  @param[out]  EfiDriverNameAndVerStr  Resultant string

  @retval      EFI_SUCCESS      Operation successful
**/
EFI_STATUS
GetEfiDriverNameAndVerStr (
  IN   UNDI_PRIVATE_DATA  *UndiPrivateData,
  OUT  EFI_STRING         EfiDriverNameAndVerStr
  );

/** Gets UEFI driver version string.

  @param[in]   UndiPrivateData  Pointer to driver private data structure
  @param[out]  EfiDriverVerStr  Resultant string

  @retval      EFI_SUCCESS      Operation successful
**/
EFI_STATUS
GetEfiDriverVerStr (
  IN   UNDI_PRIVATE_DATA  *UndiPrivateData,
  OUT  EFI_STRING         EfiDriverVerStr
  );

/** Gets Device ID string.

  @param[in]   UndiPrivateData  Pointer to driver private data structure
  @param[out]  DeviceIdStr      Resultant string

  @retval      EFI_SUCCESS      Operation successful
**/
EFI_STATUS
GetDeviceIdStr (
  IN   UNDI_PRIVATE_DATA  *UndiPrivateData,
  OUT  EFI_STRING         DeviceIdStr
  );


/** Checks if port option is modifiable.

  @param[in]   UndiPrivateData  Pointer to driver private data structure
  @param[out]  Supported        BOOLEAN support information

  @retval    EFI_SUCCESS        Operation successful
**/
EFI_STATUS
IsPortOptModifiable (
  IN   UNDI_PRIVATE_DATA  *UndiPrivateData,
  OUT  BOOLEAN            *Supported
  );

/** Checks if port option change is supported.

  @param[in]   UndiPrivateData  Pointer to driver private data structure
  @param[out]  Supported        BOOLEAN support information

  @retval    EFI_SUCCESS        Operation successful
**/
EFI_STATUS
IsPortOptionChangeSupported (
  IN   UNDI_PRIVATE_DATA  *UndiPrivateData,
  OUT  BOOLEAN            *Supported
  );




/** Checks if Alternate MAC address attribute is supported.

  @param[in]   UndiPrivateData  Pointer to driver private data structure
  @param[out]  Supported        BOOLEAN support information

  @retval    EFI_SUCCESS        Operation successful
**/
EFI_STATUS
IsAltMacAddrSupported (
  IN   UNDI_PRIVATE_DATA  *UndiPrivateData,
  OUT  BOOLEAN            *Supported
  );






/** Calculates value of support flags that are not tied to specific field in standard formset
  varstore configuration map (e.g. specify form visibility, not field).

  @param[in]   UndiPrivateData  Pointer to driver private data structure
  @param[out]  SupportTable     Pointer to support table in varstore buffer

  @retval    EFI_SUCCESS        Operation successful
  @retval    !EFI_SUCCESS       Failed to calculate support value for specific fields
**/
EFI_STATUS
EvaluateUnaffiliatedSupportFlags (
  IN  UNDI_PRIVATE_DATA  *UndiPrivateData,
  OUT FIELD_SUPPORT      *SupportTable
  );

/** Reads and sets support information that is static & HW dependent. Called during HII setup
   before inventory initialization.

   @param[in,out]   UndiPrivateData   Points to the driver instance private data

  @retval    EFI_SUCCESS        Operation successful
  @retval    !EFI_SUCCESS       Failure of underlying function
**/
EFI_STATUS
SetHwDependentAdapterSupportFlags (
  IN OUT  UNDI_PRIVATE_DATA  *UndiPrivateData
  );

#endif /* HII_COMMON_DEP_H_ */
