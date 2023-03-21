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
#include <Library/HiiLib.h>
#include "Hii/Hii.h"

#ifndef SWITCH_MODE
#include "Hii/CfgAccessProt/HiiConfigAccessInfo.h"
#include "Hii/HiiSetup.h"
#include "Hii/FormsetStd/HiiCommonDep.h"



/* This is the generated IFR binary data for each formset defined in VFR.
  This data array is ready to be used as input of PreparePackageList() to
  create a packagelist (which contains Form packages, String packages, etc). */
extern UINT8                           InventoryBin[];
#endif /* !SWITCH_MODE */

/* This is the generated String package data for all .UNI files.
  This data array is ready to be used as input of PreparePackageList() to
  create a packagelist (which contains Form packages, String packages, etc). */
extern UINT8                           *gUndiDxeHiiStringsPkgPtr;
#ifndef SWITCH_MODE
extern EFI_HII_CONFIG_ACCESS_PROTOCOL  gHiiConfigAccessProt;

extern HII_VARSTORE_MAP_CFG            mHiiStandardMapCfg;

extern HII_STATIC_INV_STRING_ENTRY     mHiiHwStaticInvStringMap[];

extern UINTN                           mHiiHwStaticInvStringMapSize;

/** Gets the next language code from native RFC 4646 language code array.

  @param[in]      Langs      Pointer to the string containing language array
  @param[in,out]  LangsIdx   Index in the string from which to start search, on
                             output set to position after currently found language
  @param[in]      LangsSize  Size of Langs string in bytes
  @param[out]     SubLang    Pointer to string filled with currently found language

  @retval    TRUE            Operation successful
  @retval    FALSE           No language left in array
  @retval    FALSE           Invalid input parameter, LangsIdx out of range
**/
BOOLEAN
HiiGetNextLanguage (
  IN      CHAR8   *Langs,
  IN OUT  UINTN   *LangsIdx,
  IN      UINTN   LangsSize,
  OUT     CHAR8   *SubLang
  )
{
  UINTN  SubLangIdx = 0;

  IF_NULL3_RETURN (Langs, LangsIdx, SubLang, FALSE);
  IF_RETURN ((*LangsIdx + 1) > LangsSize, FALSE);

  while ((*LangsIdx + 1) < LangsSize) {
    if (Langs[*LangsIdx] == ';') {
      (*LangsIdx)++;
      break;
    }
    if (Langs[*LangsIdx] == '\0') {
      break;
    }
    SubLang[SubLangIdx++] = Langs[ (*LangsIdx)++];
  }

  SubLang[SubLangIdx] = '\0';

  return (SubLangIdx != 0); // any languages left ?
}

/** Processes static inventory strings map for specific language OR language agnostic entries.

  @param[in]   UndiPrivateData  Pointer to driver private data structure
  @param[in]   Language         Language for which map language specific entries should be processed,
                                or NULL when language agnostic entries should be processed

  @retval    EFI_SUCCESS            Operation successful
  @retval    EFI_INVALID_PARAMETER  Invalid input parameter
  @retval    EFI_INVALID_PARAMETER  Map entry has no getter
  @retval    EFI_UNSUPPORTED        Unsupported type of getter
  @retval    EFI_DEVICE_ERROR       Failed to set inventory string in package
  @retval    !EFI_SUCCESS           Failure of inventory string getter / support checker
**/
EFI_STATUS
HiiSetupStaticInventory (
  IN        UNDI_PRIVATE_DATA  *UndiPrivateData,
  IN CONST  CHAR8              *Language
  )
{
  EFI_STATUS                   Status;
  HII_STATIC_INV_STRING_ENTRY  *InvStringEntry;
  UINTN                        EntryIdx;
  BOOLEAN                      EntrySupported;
  CHAR16                       InvString[HII_MAX_STR_LEN];
  EFI_STRING_ID                InvStringId;

  DEBUGPRINT (HII, ("--> lang - %a\n", Language));

  ASSERT (UndiPrivateData != NULL);

  for (EntryIdx = 0; EntryIdx < mHiiHwStaticInvStringMapSize; EntryIdx++) {
    InvStringEntry = &mHiiHwStaticInvStringMap[EntryIdx];

    if (((Language == NULL) && (InvStringEntry->GetStringType == LANG)) ||
        ((Language == NULL) && (InvStringEntry->GetStringType == LANG_FOR_STRING_ID)) ||
        ((Language != NULL) && (InvStringEntry->GetStringType == ALL_LANG)) ||
        ((Language != NULL) && (InvStringEntry->GetStringType == ALL_LANG_FOR_STRING_ID)))
    {
      continue; // for language agnostic (NULL) omit language specific entry & vice versa
    }
    DEBUGPRINT (HII, ("Processing entry %d for lang %a\n", EntryIdx, Language));

    if (Language != NULL) {
      if (IS_UEFI_CONFIG_LANG (Language) &&
          !InvStringEntry->HasXUefi)
      {
        continue; // Don't modify x-UEFI language strings
      }
    }

    if (InvStringEntry->CheckSupport != NULL) { // evaluate support if needed
      Status = InvStringEntry->CheckSupport (UndiPrivateData, &EntrySupported);
      IF_RETURN (EFI_ERROR (Status), Status);
      if (!EntrySupported) {
        DEBUGPRINT (HII, (" - unsupported\n"));
        continue;
      }
    }

    if (InvStringEntry->GetString.Raw == NULL) {
      return EFI_INVALID_PARAMETER;
    }

    ZeroMem (InvString, sizeof (InvString));

    switch (InvStringEntry->GetStringType) {
    case ALL_LANG:
      Status = InvStringEntry->GetString.GetAllLang (UndiPrivateData, InvString);
      break;
    case LANG:
      Status = InvStringEntry->GetString.GetLang (UndiPrivateData, InvString, Language);
      break;
    case ALL_LANG_FOR_STRING_ID:
      Status = InvStringEntry->GetString.GetAllLangForStringId (UndiPrivateData, InvString, InvStringEntry->StringId);
      break;
    case LANG_FOR_STRING_ID:
      Status = InvStringEntry->GetString.GetLangForStringId (
                                           UndiPrivateData,
                                           InvString,
                                           Language,
                                           InvStringEntry->StringId
                                           );
      break;
    default:
      Status = EFI_UNSUPPORTED;
      break;
    }

    if (EFI_ERROR (Status)) {
      ZeroMem (InvString, sizeof (InvString));
      Status = GetInventoryStr (UndiPrivateData, InvString, STRING_TOKEN (STR_NA_TEXT), Language);
      IF_RETURN (EFI_ERROR (Status), Status);
    }

    DEBUGPRINT (HII, ("Setting Inv. string to:\n%s\n", InvString));

    InvStringId = HiiSetString (
                    UndiPrivateData->HiiInfo.HiiPkgListHandle,
                    InvStringEntry->StringId,
                    InvString,
                    Language
                    );
    IF_RETURN (InvStringId == 0, EFI_DEVICE_ERROR);
  }

  DEBUGPRINT (HII, ("Static Inv. processing success for lang %a\n", Language));
  return EFI_SUCCESS;
}

#define HII_VARSTORES_MAX 2

/** Helper function that adds varstore (config data structure) map config.

  @param[in,out]  HiiCfgAccessInfo  HII Config Access protocol info structure
  @param[in]      VarStoreMapCfg    Varstore map config structure
  @param[in]      DriverContext     Context of the driver that will be passed to varstore
                                    map getter/setters
  @param[in]      Default           Boolean value that tells if this varstore map config is default

  @retval    EFI_SUCCESS            Operation successful
  @retval    EFI_INVALID_PARAMETER  Invalid input parameter
  @retval    EFI_OUT_OF_RESOURCES   Maximum number of varstores reached
**/
EFI_STATUS
AddVarStoreConfig (
  IN OUT  HII_CFG_ACCESS_INFO   *HiiCfgAccessInfo,
  IN      HII_VARSTORE_MAP_CFG  *VarStoreMapCfg,
  IN      VOID                  *DriverContext,
  IN      BOOLEAN               Default
  )
{
  HII_VARSTORE_MAP_CFG  *VarStoreConfigDest;

  IF_NULL3_RETURN (HiiCfgAccessInfo, HiiCfgAccessInfo->VarStoreConfigs, DriverContext, EFI_INVALID_PARAMETER);
  IF_RETURN (HiiCfgAccessInfo->NumberOfVarStoreConfigs == HII_VARSTORES_MAX, EFI_OUT_OF_RESOURCES);

  VarStoreConfigDest = HiiCfgAccessInfo->VarStoreConfigs + HiiCfgAccessInfo->NumberOfVarStoreConfigs;

  CopyMem (
    VarStoreConfigDest,
    VarStoreMapCfg,
    sizeof (HII_VARSTORE_MAP_CFG)
    );
  VarStoreConfigDest->DriverContext = DriverContext;

  if (Default) {
    HiiCfgAccessInfo->DefaultVarStoreCfgIdx = HiiCfgAccessInfo->NumberOfVarStoreConfigs;
  }

  HiiCfgAccessInfo->NumberOfVarStoreConfigs++;

  return EFI_SUCCESS;
}

/** Helper function that strips x-UEFI string package.

  @param[out]   StringsPkgStripped    Pointer to a buffer containing stripped string package
                                      (allocated internally)

  @retval       EFI_SUCCESS            Operation successful
  @retval       EFI_OUT_OF_RESOURCES   Couldn't allocate buffer for stripped inventory string package
**/
EFI_STATUS
StripXUefiStringsFromPkg (
  OUT UINT8 **StringsPkgStripped
  )
{
  EFI_HII_STRING_PACKAGE_HDR  *CurrentPkg       = NULL;
  UINT8                       *StringsPkgLocal  = NULL;
  UINT32                      CurrentPkgOffset  = sizeof (UINT32);
  UINT32                      StringsPkgSize    = *((UINT32 *)gUndiDxeHiiStringsPkgPtr);
  UINT32                      CurrentPkgSize    = 0;
  UINT32                      TotalPkgsSize     = sizeof (UINT32);

  *StringsPkgStripped   = NULL;
  StringsPkgLocal       = AllocateZeroPool (StringsPkgSize);
  IF_NULL_RETURN (StringsPkgLocal, EFI_OUT_OF_RESOURCES);

  while (CurrentPkgOffset < StringsPkgSize) {
    CurrentPkg      = (EFI_HII_STRING_PACKAGE_HDR *)(&gUndiDxeHiiStringsPkgPtr[CurrentPkgOffset]);
    CurrentPkgSize  = CurrentPkg->Header.Length;

    if (AsciiStrnCmp (CurrentPkg->Language, UEFI_CONFIG_LANG, AsciiStrLen (UEFI_CONFIG_LANG)) != 0) {
      CopyMem (StringsPkgLocal + CurrentPkgOffset, gUndiDxeHiiStringsPkgPtr + CurrentPkgOffset, CurrentPkgSize);
      TotalPkgsSize += CurrentPkgSize;
    }
    CurrentPkgOffset += CurrentPkgSize;
  }
  *(UINT32 *)StringsPkgLocal = TotalPkgsSize;

  *StringsPkgStripped = StringsPkgLocal;
  return EFI_SUCCESS;
}

/** Main function that sets up inventory packages. Adds configuration for supported varstores &
  adds required packages, runs static inventory map processing.

  @param[in,out]   UndiPrivateData  Pointer to driver private data structure

  @retval    EFI_SUCCESS            Operation successful
  @retval    EFI_INVALID_PARAMETER  Invalid input parameter
  @retval    EFI_OUT_OF_RESOURCES   Failed to add packages
  @retval    EFI_OUT_OF_RESOURCES   Memory allocation failed
  @retval    EFI_OUT_OF_RESOURCES   Failed to get supported languages
  @retval    !EFI_SUCCESS           Failure of one of the other underlying functions
**/
EFI_STATUS
HiiSetupInventoryPackages (
  IN OUT  UNDI_PRIVATE_DATA  *UndiPrivateData
  )
{
  EFI_STATUS                   Status;
  CHAR8                        SubLang[HII_MAX_STR_LEN];
  UINTN                        LanguagesSize;
  UINTN                        LanguagesIdx               = 0;
  BOOLEAN                      InvSetupSuccess            = FALSE;
  CHAR8                        *Languages                 = NULL;
  EFI_GUID                     HiiFormGuid                = HII_FORM_GUID;
  HII_INFO                     *HiiInfo;
  UINT8                        *StringsPkg                = gUndiDxeHiiStringsPkgPtr;

  ASSERT (UndiPrivateData != NULL);

  HiiInfo = &UndiPrivateData->HiiInfo;

  Status = SetHwDependentAdapterSupportFlags (UndiPrivateData);
  IF_RETURN (EFI_ERROR (Status), Status);

  HiiInfo->HiiCfgAccessInfo.VarStoreConfigs = AllocateZeroPool (HII_VARSTORES_MAX * sizeof (HII_VARSTORE_MAP_CFG));
  IF_NULL_RETURN (HiiInfo->HiiCfgAccessInfo.VarStoreConfigs, EFI_OUT_OF_RESOURCES);

  HiiInfo->HiiCfgAccessInfo.OnActionChanging      = HiiOnActionChanging;
  HiiInfo->HiiCfgAccessInfo.OnActionChanged       = HiiOnActionChanged;
  HiiInfo->HiiCfgAccessInfo.CallBackDriverContext = UndiPrivateData;

  Status = AddVarStoreConfig (&HiiInfo->HiiCfgAccessInfo, &mHiiStandardMapCfg, UndiPrivateData, TRUE);
  IF_GOTO (EFI_ERROR (Status), ExitFreeRes);

    HiiInfo->HiiPkgListHandle = HiiAddPackages (
                                  &HiiFormGuid,
                                  HiiInfo->HiiInstallHandle,
                                  StringsPkg,
                                  InventoryBin,
                                  NULL
                                  );

  IF_GOTO (HiiInfo->HiiPkgListHandle == NULL, ExitFreeRes);

  Languages = HiiGetSupportedLanguages (HiiInfo->HiiPkgListHandle);
  IF_GOTO (Languages == NULL, ExitFreeRes);

  DEBUGPRINT (HII, ("Package list languages - %a\n"));

  // Setup language agnostic inventory
  Status = HiiSetupStaticInventory (UndiPrivateData, NULL);
  IF_GOTO (EFI_ERROR (Status), ExitFreeRes);

  // assume size of returned languages will never be longer than HII_MAX_STR_LEN
  LanguagesSize = AsciiStrnLenS (Languages, HII_MAX_STR_LEN) + 1;

  // Setup language specific inventory
  while (HiiGetNextLanguage (Languages, &LanguagesIdx, LanguagesSize, SubLang)) {
    Status = HiiSetupStaticInventory (UndiPrivateData, SubLang);
    IF_GOTO (EFI_ERROR (Status), ExitFreeRes);
  }

  InvSetupSuccess = TRUE;
ExitFreeRes:
  if (Languages != NULL) {
    FreePool (Languages);
  }

  if (!InvSetupSuccess) {
    HiiUnload (UndiPrivateData);
    if (!EFI_ERROR (Status)) {
      Status = EFI_OUT_OF_RESOURCES;
    }
  }


  return Status;
}

/** Locates required HII protocols (Database, String, FormBrowser2 and ConfigRouting).

   @param[in,out]   HiiInfo      Hii info structure

   @retval   EFI_SUCCESS     All protocols located successfully
   @retval   EFI_NOT_FOUND   No instances of one of above protocols were found
**/
EFI_STATUS
HiiLocateProtocols (
  IN OUT  HII_INFO  *HiiInfo
  )
{
  EFI_STATUS                      Status;
  EFI_HII_DATABASE_PROTOCOL       *HiiDatabaseProt;
  EFI_HII_STRING_PROTOCOL         *HiiStringProt;
  EFI_FORM_BROWSER2_PROTOCOL      *FormBrowser2Prot;

  ASSERT (HiiInfo != NULL);

  Status = gBS->LocateProtocol (
                  &gEfiHiiDatabaseProtocolGuid,
                  NULL,
                  (VOID **) &HiiDatabaseProt
                  );
  IF_RETURN (EFI_ERROR (Status), Status);

  Status = gBS->LocateProtocol (
                  &gEfiHiiStringProtocolGuid,
                  NULL,
                  (VOID **) &HiiStringProt
                  );
  IF_RETURN (EFI_ERROR (Status), Status);

  Status = gBS->LocateProtocol (
                  &gEfiFormBrowser2ProtocolGuid,
                  NULL,
                  (VOID **) &FormBrowser2Prot
                  );
  IF_RETURN (EFI_ERROR (Status), Status);

  Status = gBS->LocateProtocol (
                  &gEfiHiiConfigRoutingProtocolGuid,
                  NULL,
                  (VOID **) &HiiInfo->HiiCfgAccessInfo.HiiConfigRoutingProt
                  );
  IF_RETURN (EFI_ERROR (Status), Status);

  return EFI_SUCCESS;
}

/** Uninstalls HII protocol & package related resources, frees memory allocations.
   (Resources previously obtained by HiiInit ()).

   @param[in,out]   UndiPrivateData   Points to the driver instance private data.

   @retval   EFI_SUCCESS    HII resources uninstalled correctly
   @retval   !EFI_SUCCESS   Failed to uninstall HII resources
**/
EFI_STATUS
HiiUnload (
  IN OUT  UNDI_PRIVATE_DATA  *UndiPrivateData
  )
{
  EFI_STATUS           Status;
  HII_INFO             *HiiInfo;

  DEBUGPRINT (HII, ("--> ()\n"));
  ASSERT (UndiPrivateData != NULL);

  HiiInfo = &UndiPrivateData->HiiInfo;

  if (HiiInfo->HiiPkgListHandle != NULL) {
    HiiRemovePackages (HiiInfo->HiiPkgListHandle);
    HiiInfo->HiiPkgListHandle = NULL;
  }


  if (HiiInfo->HiiInstallHandle != NULL) {
    if (HiiInfo->HiiCfgAccessInfo.VarStoreConfigs != NULL) {
      FreePool (HiiInfo->HiiCfgAccessInfo.VarStoreConfigs);
      HiiInfo->HiiCfgAccessInfo.VarStoreConfigs = NULL;
    }

    Status = gBS->UninstallProtocolInterface (
                    HiiInfo->HiiInstallHandle,
                    &gEfiHiiConfigAccessProtocolGuid,
                    &HiiInfo->HiiCfgAccessInfo.HiiConfigAccessProt
                    );
    IF_RETURN (EFI_ERROR (Status), Status);

    HiiInfo->HiiInstallHandle = NULL;
  }

  ZeroMem (HiiInfo, sizeof (HII_INFO));

  return EFI_SUCCESS;
}

/** Installs HII Config Access interfaces required by driver instance -
   protocol & package related resources.

   @param[in,out]   UndiPrivateData   Points to the driver instance private data.

   @retval   EFI_SUCCESS          Successful operation
   @retval   EFI_ALREADY_STARTED  HII interfaces are already installed or weren't properly uninstalled
   @retval   !EFI_SUCCESS         Failed to setup Config Access protocol/package resources or
                                  failed to locate required protocols
**/
EFI_STATUS
HiiInit (
  IN OUT  UNDI_PRIVATE_DATA  *UndiPrivateData
  )
{
  EFI_STATUS               Status;
  HII_INFO                 *HiiInfo;
  HII_CFG_ACCESS_INFO      *HiiCfgAccessInfo;

  DEBUGPRINT (HII, ("--> ()\n"));
  ASSERT (UndiPrivateData != NULL);

  HiiInfo          = &UndiPrivateData->HiiInfo;
  HiiCfgAccessInfo = &HiiInfo->HiiCfgAccessInfo;

  IF_RETURN (HiiInfo->HiiInstallHandle != NULL, EFI_ALREADY_STARTED);
  IF_RETURN (HiiInfo->HiiPkgListHandle != NULL, EFI_ALREADY_STARTED);

  ZeroMem (HiiInfo, sizeof (HII_INFO));

  // 1. Locate required HII protocols in the platform BIOS
  Status = HiiLocateProtocols (HiiInfo);
  IF_RETURN (EFI_ERROR (Status), Status);

  // 2. Install HII Config Access protocol instance

  HiiInfo->HiiInstallHandle = UndiPrivateData->DeviceHandle;

  HiiCfgAccessInfo->Signature          = HII_CFG_ACCESS_INFO_SIG;
  HiiCfgAccessInfo->InstallationHandle = HiiInfo->HiiInstallHandle;

  CopyMem (
    &HiiCfgAccessInfo->HiiConfigAccessProt,
    &gHiiConfigAccessProt,
    sizeof (EFI_HII_CONFIG_ACCESS_PROTOCOL)
    );

  Status = gBS->InstallProtocolInterface (
                  &HiiInfo->HiiInstallHandle,
                  &gEfiHiiConfigAccessProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &HiiCfgAccessInfo->HiiConfigAccessProt
                  );
  IF_GOTO (EFI_ERROR (Status), ConfigAccessInstallError);

  // 3. Setup & add packages related to HII Config Access protocol instance
  Status = HiiSetupInventoryPackages (UndiPrivateData);
  IF_RETURN (EFI_ERROR (Status), Status);


  DEBUGPRINT (HII, ("HiiInit() - EFI_SUCCESS\n"));
  return EFI_SUCCESS;

ConfigAccessInstallError:
  HiiInfo->HiiInstallHandle = NULL;
  HiiUnload (UndiPrivateData);
  return Status;
}
#endif /* !SWITCH_MODE */

/** Uninstalls HII packages installed in partial init flow.
   Reverts HiiAddStringPkgOnly () operations.

   @param[in,out]   UndiPrivateData   Points to the driver instance private data

   @retval   EFI_SUCCESS          Always returned
**/
EFI_STATUS
HiiRemoveStringPkgOnly (
  IN OUT  UNDI_PRIVATE_DATA  *UndiPrivateData
  )
{
  HII_INFO  *HiiInfo;

  ASSERT (UndiPrivateData != NULL);

  HiiInfo = &UndiPrivateData->HiiInfo;

  if (HiiInfo->HiiPkgListHandle != NULL) {
    HiiRemovePackages (HiiInfo->HiiPkgListHandle);
    HiiInfo->HiiPkgListHandle = NULL;
  }

  return EFI_SUCCESS;
}

/** Used instead of HiiInit() in partial init flow. Installs only string packages
   to allow Driver Health protocol reporting.

   @param[in,out]   UndiPrivateData   Points to the driver instance private data

   @retval   EFI_SUCCESS           Successful operation
   @retval   EFI_ALREADY_STARTED   HII string packages are already installed or weren't properly uninstalled
   @retval   EFI_OUT_OF_RESOURCES  Failed to register string packages
**/
EFI_STATUS
HiiAddStringPkgOnly (
  IN OUT  UNDI_PRIVATE_DATA  *UndiPrivateData
  )
{
  HII_INFO  *HiiInfo;
  EFI_GUID  HiiFormGuid = HII_FORM_GUID;

  ASSERT (UndiPrivateData != NULL);

  HiiInfo = &UndiPrivateData->HiiInfo;

  IF_RETURN (HiiInfo->HiiPkgListHandle != NULL, EFI_ALREADY_STARTED);

  HiiInfo->HiiInstallHandle = UndiPrivateData->ControllerHandle;
  HiiInfo->HiiPkgListHandle = HiiAddPackages (
                                &HiiFormGuid,
                                HiiInfo->HiiInstallHandle,
                                gUndiDxeHiiStringsPkgPtr,
                                NULL
                                );
  IF_NULL_RETURN (HiiInfo->HiiPkgListHandle, EFI_OUT_OF_RESOURCES);

  return EFI_SUCCESS;
}
