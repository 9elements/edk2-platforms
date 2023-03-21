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
