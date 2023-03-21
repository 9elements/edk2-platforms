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
#include "Hii/Hii.h"
#include "Hii/FormsetStd/HiiCommonDep.h"
#include "Hii/CfgAccessProt/HiiConfigAccessInfo.h"
#include "HiiConfigData.h"

/** Instatiates first 2 fields (offset, width) in HII_CONFIG_MAP_ENTRY structure.

  @param[in]  Field   Name of the field in structure
**/
#if DBG_LVL & HII
  #define OFFSET_WIDTH(Field) \
    STRUCT_OFFSET (HII_STD_VARSTORE, Field), sizeof (((HII_STD_VARSTORE *) 0)->Field), .Name = "" #Field ""
#else /* !(DBG_LVL & HII) */
  #define OFFSET_WIDTH(Field) \
    STRUCT_OFFSET (HII_STD_VARSTORE, Field), sizeof (((HII_STD_VARSTORE *) 0)->Field)
#endif /* DBG_LVL & HII */
HII_CONFIG_MAP_ENTRY  mHiiConfigMap[] = {
  /*                                                                                                | Last 3 fields are relevant ONLY when runtime evaluation is needed, if parameter |
                                                                                                    | visibility is hardcoded in VFR e.g. "grayoutif TRUE", determined by other       |
                                                                                                    | known values, or not needed, then mark it as VIS_NO_EVAL.                       |
   | 2 fields (offset + width)                | Getter (ExtractConfig)   | Setter (RouteConfig)     | SupportTable idx | Is Modifiable ?       | Is Supported ?          |*/

  // --------------------------- <"NIC Configuration"> menu -------------------------------------
  { OFFSET_WIDTH (LinkSpeed),                  GetLinkSpeed,              SetLinkSpeed,              LINK_SPEED,        IsLinkSpeedModifiable, IsLinkSpeedSupported },
  { OFFSET_WIDTH (WolStatus),                  WolGetWakeOnLanStatus,     WolSetWakeOnLanStatus,     VIS_NO_EVAL,       NULL,                  NULL },
  { OFFSET_WIDTH (DefaultWolStatus),           GetDefaultWolStatus,       NULL,                      VIS_NO_EVAL,       NULL,                  NULL },


  // ---------------------------  Main HII menu -----------------------------------------------
  { OFFSET_WIDTH (BlinkLed),                   NULL,                      BlinkLeds,                 VIS_NO_EVAL,       NULL,                  NULL },
  { OFFSET_WIDTH (LinkStatus),                 GetLinkStatus,             NULL,                      VIS_NO_EVAL,       NULL,                  NULL },
  { OFFSET_WIDTH (AltMacAddr),                 GetAltMacAddr,             NULL,                      ALT_MAC,           NULL,                  IsAltMacAddrSupported },
  // special SupportTable field treated differently in ExtractConfig
  { OFFSET_WIDTH (Support),                    NULL,                       NULL,                      VIS_NO_EVAL,       NULL,                  NULL }
};

HII_VARSTORE_MAP_CFG  mHiiStandardMapCfg = {

  .ConfigMap               = mHiiConfigMap,
  .NumMapEntries           = sizeof (mHiiConfigMap) / sizeof (mHiiConfigMap[0]),
  .ConfigName              = L"NicCfgData",
  .ConfigGuid              = HII_DATA_GUID,
  .ConfigSize              = sizeof (HII_STD_VARSTORE),
  .HasSupportTable         = TRUE,
  .SupportTableOffset      = OFFSET_OF (HII_STD_VARSTORE, Support),
  .PreRoute                = HiiConfigMapPreRoute,
  .PostRoute               = HiiConfigMapPostRoute,
  .EvalUnaffiliatedSupport = EvaluateUnaffiliatedSupportFlags
};

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
  )
{
  return EFI_SUCCESS;
}

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
  )
{
  EFI_STATUS Status = EFI_SUCCESS;
  EFI_STATUS UpdateNvmChecksumStatus;


  UpdateNvmChecksumStatus = UpdateNvmChecksum (UndiPrivateData);
  IF_RETURN (EFI_ERROR (UpdateNvmChecksumStatus), UpdateNvmChecksumStatus);

  DEBUGPRINT (HII, ("RouteConfig changes committed\n"));
  return Status;
}
