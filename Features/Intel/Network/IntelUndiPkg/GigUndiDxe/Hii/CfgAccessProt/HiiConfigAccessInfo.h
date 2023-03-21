/******************************************************************************
**                                                                           **
** INTEL CONFIDENTIAL                                                        **
**                                                                           **
** Copyright (c) 2020 - 2021, Intel Corporation. All rights reserved.        **
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
#ifndef HII_CONFIG_ACCESS_INFO_H_
#define HII_CONFIG_ACCESS_INFO_H_

#include <Uefi.h>

#include <Library/HiiLib.h>

#include <Protocol/HiiConfigAccess.h>
#include <Protocol/HiiDatabase.h>
#include <Protocol/HiiString.h>
#include <Protocol/HiiConfigRouting.h>

#include "HiiVarStoreFieldSupport.h"

/** Utility macro to ease traversing varstore configuration map.

   @param[in]      VarStoreMapCfg   HII_VARSTORE_MAP_CFG instance pointer
   @param[in,out]  MapEntry         HII_CONFIG_MAP_ENTRY pointer used to iterate over map
**/
#define FOR_EACH_CONFIG_MAP_ENTRY(VarStoreMapCfg, MapEntry)                                              \
  for (UINTN MapIdx = 0;                                                                                 \
       (MapIdx < VarStoreMapCfg->NumMapEntries) ? (MapEntry = &(VarStoreMapCfg->ConfigMap[MapIdx])) : 0; \
       MapIdx++)


/** Helper function to return information if specific field is included in Configuration string.

   @param[in]   Configuration  A null-terminated Unicode string in <ConfigResp> format.
   @param[in]   FieldOffset    Offset of the searched field.

   @retval   EFI_SUCCESS            Field found in the Configuration string
   @retval   EFI_INVALID_PARAMETER  Error when parsing Configuration string
   @retval   EFI_NOT_FOUND          Field was not found in the Configuration string
**/
EFI_STATUS
IsFieldPresentInConfiguration (
  IN  CONST EFI_STRING  Configuration,
  IN  UINTN             FieldOffset
  );

/** Returns support information.
   Used to check whether varstore field is modifiable or supported at all.

   @param[in]   DriverContext   Pointer to driver private data structure
   @param[out]  Supported       Calculated support value

   @retval      EFI_SUCCESS      Operation successful.
   @retval      !EFI_SUCCESS     Failure with reason specific to varstore config map entry
**/
typedef
EFI_STATUS
(*HII_CONFIG_FIELD_CHECK_SUPPORT) (
  IN   VOID     *DriverContext,
  OUT  BOOLEAN  *Supported
  );

/** Used to get or set varstore field in Extract/RouteConfig.

   @param[in]      DriverContext   Pointer to driver private data structure
   @param[in,out]  HiiConfigField  Varstore field pointer

   @retval      EFI_SUCCESS      Operation successful.
   @retval      !EFI_SUCCESS     Failure with reason specific to varstore config map entry
**/
typedef
EFI_STATUS
(*HII_CONFIG_FIELD_GET_SET) (
  IN      VOID  *DriverContext,
  IN OUT  VOID  *HiiConfigField
  );

/** Structure which describes single varstore field in varstore configuration map.
**/
typedef struct HII_CONFIG_MAP_ENTRY_S {
  UINTN                           FieldOffset;
  UINTN                           FieldWidth;
#if DBG_LVL & HII
  CONST CHAR8                     *Name;
#endif /* DBG_LVL & HII */
  HII_CONFIG_FIELD_GET_SET        Get;
  HII_CONFIG_FIELD_GET_SET        Set;
  UINT32                          SupportFlagIdx; ///< index of FIELD_SUPPORT value in SupportTable if that exists
  HII_CONFIG_FIELD_CHECK_SUPPORT  CheckGrayOut;
  HII_CONFIG_FIELD_CHECK_SUPPORT  CheckSuppress;
  BOOLEAN                         CfgExecuted;    ///< used to avoid running Get/Set in Extract/Route multiple times
                                                  ///< for every array element for fields which Get/Sets
                                                  ///< support whole arrays of configuration
  BOOLEAN                         SetBlocked;     ///< used to block Set execution if there's a need to
  BOOLEAN                         SetAttempted;   ///< informs that Set was attempted when SetBlocked was TRUE
} HII_CONFIG_MAP_ENTRY;

typedef struct HII_VARSTORE_MAP_CFG_S HII_VARSTORE_MAP_CFG;

/** Performs operations before starting varstore config map processing (OPTIONAL).

   @param[in]   DriverContext    Pointer to driver private data structure
   @param[in]   VarStoreMapCfg   HII varstore map configuration structure
   @param[in]   HiiCfgData       Pointer to raw configuration data buffer (of varstore type)
   @param[in]   ConfigOrRequest  Configuration string for RouteConfig, Request string for ExtractConfig

   @retval      EFI_SUCCESS      Operation successful.
   @retval      !EFI_SUCCESS     Failure with reason specific to varstore config map
**/
typedef
EFI_STATUS
(*HII_CONFIG_PRE_EXTRACT_ROUTE) (
  IN       VOID                  *DriverContext,
  IN       HII_VARSTORE_MAP_CFG  *VarStoreMapCfg,
  IN       VOID                  *HiiCfgData,
  IN CONST EFI_STRING            ConfigOrRequest
  );

/** Performs operations after varstore config map processing (OPTIONAL).

   @param[in]   DriverContext    Pointer to driver private data structure
   @param[in]   VarStoreMapCfg   HII varstore map configuration structure

   @retval      EFI_SUCCESS      Operation successful.
   @retval      !EFI_SUCCESS     Failure with reason specific to varstore config map
**/
typedef
EFI_STATUS
(*HII_CONFIG_POST_EXTRACT_ROUTE) (
  IN      VOID                  *DriverContext,
  IN      HII_VARSTORE_MAP_CFG  *VarStoreMapCfg
  );

/** If varstore contains support table this optional function evaluates support indexes that are
   not related to specific field (OPTIONAL). It's not required even if SupportTable exists.

   @param[in]   DriverContext   Pointer to driver private data structure
   @param[out]  SupportTable    Address of FIELD_SUPPORT* SupportTable contained within varstore

   @retval      EFI_SUCCESS      Operation successful.
   @retval      !EFI_SUCCESS     Failure with reason specific to varstore config map
**/
typedef
EFI_STATUS
(*HII_EVAL_UNAFFILIATED_SUPPORT_FLAGS) (
  IN   VOID           *DriverContext,
  OUT  FIELD_SUPPORT  *SupportTable
  );

/** Structure which describes single varstore configuration map.
**/
typedef struct HII_VARSTORE_MAP_CFG_S {
  HII_CONFIG_MAP_ENTRY                 *ConfigMap;    ///< Pointer to varstore config map describing all fields
  UINTN                                NumMapEntries; ///< Number of entries in the map
  EFI_STRING                           ConfigName;    ///< varstore name (matches <ConfigHdr> in Extract/RouteConfig)
  EFI_GUID                             ConfigGuid;    ///< varstore GUID (matches <ConfigHdr> in Extract/RouteConfig)
  UINTN                                ConfigSize;    ///< Size of the varstore
  BOOLEAN                              HasSupportTable;
  UINTN                                SupportTableOffset;

  // Optional functions, can be NULL, see typedef description
  HII_CONFIG_PRE_EXTRACT_ROUTE         PreExtract;
  HII_CONFIG_POST_EXTRACT_ROUTE        PostExtract;
  HII_CONFIG_PRE_EXTRACT_ROUTE         PreRoute;
  HII_CONFIG_POST_EXTRACT_ROUTE        PostRoute;

  // Optional function, can be NULL, see typedef description
  HII_EVAL_UNAFFILIATED_SUPPORT_FLAGS  EvalUnaffiliatedSupport;

  VOID                                 *DriverContext;
} HII_VARSTORE_MAP_CFG;

#define HII_CFG_ACCESS_INFO_SIG SIGNATURE_32 ('H', 'C', 'I', 'S')

/** Callback handler for specific Action used inside ConfigAccess.CallBack.

   @param[in]      DriverContext    Pointer to driver private data structure
   @param[in]      QuestionId       Question ID related to specific HII content
   @param[in]      Type             Type specifying question value
   @param[in,out]  Value            Question value
   @param[out]     ActionRequest    On return, action requested by the callback function.

   @retval      EFI_SUCCESS      Operation successful.
   @retval      !EFI_SUCCESS     Failure with reason specific to specific Action
**/
typedef
EFI_STATUS
(* HII_ACCESS_FORM_ACTION_CALLBACK) (
  IN     VOID                        *DriverContext,
  IN     EFI_QUESTION_ID             QuestionId,
  IN     UINT8                       Type,
  IN OUT EFI_IFR_TYPE_VALUE          *Value,
  OUT    EFI_BROWSER_ACTION_REQUEST  *ActionRequest
  );

/** Structure which describes whole HII Config Access protocol accessible configuration
 * supported by this driver (all varstores).
**/
typedef struct HII_CFG_ACCESS_INFO_S {
  UINTN                            Signature;               ///< Must be set to HII_CFG_ACCESS_INFO_SIG
  EFI_HANDLE                       InstallationHandle;

  EFI_HII_CONFIG_ROUTING_PROTOCOL  *HiiConfigRoutingProt;

  EFI_HII_CONFIG_ACCESS_PROTOCOL   HiiConfigAccessProt;     ///< Driver context to be used in callback

  UINTN                            NumberOfVarStoreConfigs; ///< Number of varstore map configs supported by driver
  UINTN                            DefaultVarStoreCfgIdx;   ///< Index used when NULL passed to ExtractConfig
  HII_VARSTORE_MAP_CFG             *VarStoreConfigs;        ///< Points to array of size NumberOfVarStoreConfigs

  // CallBack handlers for specific actions. Can be NULL.
  HII_ACCESS_FORM_ACTION_CALLBACK  OnActionChanging;
  HII_ACCESS_FORM_ACTION_CALLBACK  OnActionChanged;
  HII_ACCESS_FORM_ACTION_CALLBACK  OnActionRetrieve;
  HII_ACCESS_FORM_ACTION_CALLBACK  OnActionFormOpen;
  HII_ACCESS_FORM_ACTION_CALLBACK  OnActionFormClose;
  HII_ACCESS_FORM_ACTION_CALLBACK  OnActionSubmitted;
  HII_ACCESS_FORM_ACTION_CALLBACK  OnActionDefaultStandard;
  HII_ACCESS_FORM_ACTION_CALLBACK  OnActionDefaultManufacturing;
  HII_ACCESS_FORM_ACTION_CALLBACK  OnActionDefaultSafe;
  HII_ACCESS_FORM_ACTION_CALLBACK  OnActionDefaultPlatform;
  HII_ACCESS_FORM_ACTION_CALLBACK  OnActionDefaultHardware;
  HII_ACCESS_FORM_ACTION_CALLBACK  OnActionDefaultFirmware;

  VOID                             *CallBackDriverContext; ///< Driver context to be used in callback
} HII_CFG_ACCESS_INFO;

#endif /* HII_CONFIG_ACCESS_INFO_H_ */