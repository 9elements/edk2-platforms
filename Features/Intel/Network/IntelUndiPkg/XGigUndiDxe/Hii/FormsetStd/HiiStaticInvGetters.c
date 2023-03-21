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


extern CHAR16 mDriverNameString[];

/** Helper function that tries to retrieve inventory string from package.

  @param[in]   UndiPrivateData  Pointer to driver private data structure
  @param[out]  InventoryStr     Pointer to resultant string
  @param[in]   StringId         EFI_STRING_ID of required string
  @param[in]   Language         Language for which string should be retrieved or NULL

  @retval      EFI_SUCCESS        Operation successful
  @retval      EFI_DEVICE_ERROR   Failed to retrieve string
**/
EFI_STATUS
GetInventoryStr (
  IN   UNDI_PRIVATE_DATA  *UndiPrivateData,
  OUT  EFI_STRING         InventoryStr,
  IN   EFI_STRING_ID      StringId,
  IN   CONST CHAR8        *Language
  )
{
  EFI_STRING     String;

  ASSERT_IF_NULL2 (UndiPrivateData, InventoryStr);

  String = HiiGetString (UndiPrivateData->HiiInfo.HiiPkgListHandle, StringId, Language);
  IF_NULL_RETURN (String, EFI_DEVICE_ERROR);

  StrCpyS (InventoryStr, HII_MAX_STR_LEN, String);
  FreePool (String);

  return EFI_SUCCESS;
}



/** Gets branding string for adapter.

  @param[in]   UndiPrivateData  Pointer to driver private data structure
  @param[out]  BrandStr         Resultant string

  @retval      EFI_SUCCESS        Operation successful
**/
EFI_STATUS
GetBrandStr (
  IN   UNDI_PRIVATE_DATA  *UndiPrivateData,
  OUT  EFI_STRING         BrandStr
  )
{
  StrCpyS (BrandStr, HII_MAX_STR_LEN, UndiPrivateData->Brand);
  return EFI_SUCCESS;
}

/** Gets standard formset title string for adapter.

  @param[in]   UndiPrivateData  Pointer to driver private data structure
  @param[out]  FormSetTitleStr  Resultant string

  @retval      EFI_SUCCESS        Operation successful
**/
EFI_STATUS
GetFormSetTitleStr (
  IN   UNDI_PRIVATE_DATA  *UndiPrivateData,
  OUT  EFI_STRING         FormSetTitleStr
  )
{
  UINT8         *CurrentMacAddr;


  CurrentMacAddr = UndiPrivateData->NicInfo.Hw.mac.addr;
  UnicodeSPrint (
    FormSetTitleStr,
    HII_MAX_STR_LEN_BYTES,
    L"%s - %02x:%02x:%02x:%02x:%02x:%02x",
    UndiPrivateData->Brand,
    CurrentMacAddr[0],
    CurrentMacAddr[1],
    CurrentMacAddr[2],
    CurrentMacAddr[3],
    CurrentMacAddr[4],
    CurrentMacAddr[5]
    );
  return EFI_SUCCESS;
}

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
  )
{
  EFI_STRING_ID  StringId;

  StringId = GetFormSetHelpStringId (UndiPrivateData);
  IF_RETURN (StringId == 0, EFI_DEVICE_ERROR);

  return GetInventoryStr (UndiPrivateData, FormSetHelpStr, StringId, Language);
}


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
  )
{
  EFI_STATUS  Status;
  UINT8       FactoryMac[ETH_ALEN];

  Status = GetFactoryMacAddress (UndiPrivateData, FactoryMac);
  IF_RETURN (EFI_ERROR (Status), Status);

  SET_UNI_MAC_FROM_BIN (FactoryMacStr, FactoryMac);
  return EFI_SUCCESS;
}

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
  )
{

  UnicodeSPrint (
    PciBdfStr,
    HII_MAX_STR_LEN_BYTES,
    L"%02x:%02x:%02x",
    UndiPrivateData->NicInfo.Bus,
    UndiPrivateData->NicInfo.Device,
    UndiPrivateData->NicInfo.Function
    );

  return EFI_SUCCESS;
}

/** Gets Driver Name string in the same format as Component Name protocol.

  @param[in]   UndiPrivateData         Pointer to driver private data structure
  @param[out]  EfiDriverNameAndVerStr  Resultant string

  @retval      EFI_SUCCESS      Operation successful
**/
EFI_STATUS
GetEfiDriverNameAndVerStr (
  IN   UNDI_PRIVATE_DATA  *UndiPrivateData,
  OUT  EFI_STRING         EfiDriverNameAndVerStr
  )
{
  StrCpyS (EfiDriverNameAndVerStr, HII_MAX_STR_LEN, mDriverNameString);
  return EFI_SUCCESS;
}

/** Gets UEFI driver version string.

  @param[in]   UndiPrivateData  Pointer to driver private data structure
  @param[out]  EfiDriverVerStr  Resultant string

  @retval      EFI_SUCCESS      Operation successful
**/
EFI_STATUS
GetEfiDriverVerStr (
  IN   UNDI_PRIVATE_DATA  *UndiPrivateData,
  OUT  EFI_STRING         EfiDriverVerStr
  )
{
  UnicodeSPrint (EfiDriverVerStr, HII_MAX_STR_LEN_BYTES, L"%d.%d.%02d", MAJORVERSION, MINORVERSION, BUILDNUMBER);
  return EFI_SUCCESS;
}

/** Gets Device ID string.

  @param[in]   UndiPrivateData  Pointer to driver private data structure
  @param[out]  DeviceIdStr      Resultant string

  @retval      EFI_SUCCESS      Operation successful
**/
EFI_STATUS
GetDeviceIdStr (
  IN   UNDI_PRIVATE_DATA  *UndiPrivateData,
  OUT  EFI_STRING         DeviceIdStr
  )
{
  UnicodeSPrint (DeviceIdStr, HII_MAX_STR_LEN_BYTES, L"%04x", UndiPrivateData->NicInfo.Hw.device_id);
  return EFI_SUCCESS;
}

