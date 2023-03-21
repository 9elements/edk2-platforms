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
#include "Hii/Hii.h"
#include "Hii/FormsetStd/HiiCommonDep.h"
#include "Hii/CfgAccessProt/HiiConfigAccessInfo.h"
#include "HiiConfigData.h"
#include "LinkTopology.h"
#include "Hii/FormsetStd/PortOptions.h"

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
  { OFFSET_WIDTH (LldpAgentStatus),            GetLldpAgentStatus,        SetLldpAgentStatus,        LLDP_AGENT,        NULL,                  IsLldpAgentSupported },
  { OFFSET_WIDTH (DefaultLldpAgentStatus),     GetDfltLldpAgentStatus,    NULL,                      LLDP_AGENT,        NULL,                  IsLldpAgentSupported },


  { OFFSET_WIDTH (ScalableIov),                GetSiovFeature,            SetSiovFeature,            CVL_DEV,           NULL,                  IsCvlDevice },
  { OFFSET_WIDTH (DefaultScalableIov),         GetSiovFeature,            NULL,                      CVL_DEV,           NULL,                  IsCvlDevice },
  { OFFSET_WIDTH (TxBalancing),                GetTransmitBalancing,      SetTransmitBalancing,      TX_BALANCING,      NULL,                  IsTxSchedulerTopologyChangeSupported },
  { OFFSET_WIDTH (DefaultTxBalancing),         GetTransmitBalancing,      NULL,                      TX_BALANCING,      NULL,                  IsTxSchedulerTopologyChangeSupported },
  // ---------------------------  Main HII menu -----------------------------------------------
  { OFFSET_WIDTH (BlinkLed),                   NULL,                      BlinkLeds,                 VIS_NO_EVAL,       NULL,                  NULL },
  { OFFSET_WIDTH (LinkStatus),                 GetLinkStatus,             NULL,                      VIS_NO_EVAL,       NULL,                  NULL },
  { OFFSET_WIDTH (AltMacAddr),                 GetAltMacAddr,             NULL,                      ALT_MAC,           NULL,                  IsAltMacAddrSupported },
  // --------------------------- Port Option attributes -----------------------------------------------
  // hidden, used while dynamically creating port option menus and to suppress/grayout HII fields
  { OFFSET_WIDTH (ActivePortOption),           GetCurrentPortOptionNum,    SetPortOption,             PORT_OPTIONS,      IsPortOptModifiable,   IsPortOptionChangeSupported },
  { OFFSET_WIDTH (PortOptStrings),             GetPortOptStrings,          NULL,                      PORT_OPTIONS,      NULL,                  IsPortOptionChangeSupported },
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
  return HiiAdapterPreRoute (UndiPrivateData, VarStoreMapCfg, HiiCfgData, Configuration);
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

  Status = HiiAdapterPostRoute (UndiPrivateData, VarStoreMapCfg);
  IF_GOTO (Status == EFI_ACCESS_DENIED, UpdateChecksum); // Port Option warning, still needs to update NVM checksum
  IF_RETURN (EFI_ERROR (Status), Status);

UpdateChecksum:
  UpdateNvmChecksumStatus = UpdateNvmChecksum (UndiPrivateData);
  IF_RETURN (EFI_ERROR (UpdateNvmChecksumStatus), UpdateNvmChecksumStatus);

  DEBUGPRINT (HII, ("RouteConfig changes committed\n"));
  return Status;
}
