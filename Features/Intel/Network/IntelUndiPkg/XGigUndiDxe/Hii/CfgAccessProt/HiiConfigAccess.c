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
#include "ErrorHandle.h"
#include "Hii/Hii.h"

#include "Hii/CfgAccessProt/HiiConfigAccessInfo.h"
#include <Library/HiiLib.h>

/** Converts pointer to EFI_HII_CONFIG_ACCESS_PROTOCOL to containing
   HII_CFG_ACCESS_INFO structure pointer.

   @param[in]  HiiCfgAccess   Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
**/
#define HII_CFG_ACCESS_INFO_FROM_HII_CONFIG_ACCESS_PROT(HiiCfgAccess) \
  CR (HiiCfgAccess, HII_CFG_ACCESS_INFO, HiiConfigAccessProt, HII_CFG_ACCESS_INFO_SIG)

/** Gets HII varstore map configuration structure for given Configuration or Request string.

  @param[in]  CfgAccessInfo    Pointer to HII_CFG_ACCESS_INFO structure
  @param[in]  ConfigOrRequest  Configuration (RouteConfig) or Request (ExtractConfig) string

  @return     Pointer to the matching varstore map configuration structure or NULL if not found
**/
HII_VARSTORE_MAP_CFG *
GetVarStoreMapCfg (
  IN        HII_CFG_ACCESS_INFO  *CfgAccessInfo,
  IN  CONST EFI_STRING           ConfigOrRequest
  )
{
  HII_VARSTORE_MAP_CFG  *VarStoreMapCfg;

  IF_NULL_RETURN (CfgAccessInfo, NULL);
  IF_RETURN (CfgAccessInfo->NumberOfVarStoreConfigs == 0, NULL);

  if (ConfigOrRequest == NULL) {
    IF_RETURN (CfgAccessInfo->DefaultVarStoreCfgIdx > (CfgAccessInfo->NumberOfVarStoreConfigs - 1), NULL);
    return &CfgAccessInfo->VarStoreConfigs[CfgAccessInfo->DefaultVarStoreCfgIdx];
  }

  for (UINTN i = 0; i < CfgAccessInfo->NumberOfVarStoreConfigs; i++) {
    VarStoreMapCfg = &CfgAccessInfo->VarStoreConfigs[i];
    if (HiiIsConfigHdrMatch (ConfigOrRequest, &VarStoreMapCfg->ConfigGuid, VarStoreMapCfg->ConfigName)) {
      return VarStoreMapCfg;
    }
  }

  DEBUGPRINT (HII, ("No supported HII configuration found for this Request\n"));
  return NULL;
}

/** Gets HII varstore map configuration structure for given Configuration or Request string.

  @param[in]   VarStoreMapCfg    HII varstore map configuration structure
  @param[in]   ConfigMapEntry    Pointer to the specific config map entry
  @param[out]  Support           Resultant FIELD_SUPPORT value for varstore field

  @retval  EFI_SUCCESS            Operation successful
  @retval  EFI_SUCCESS            No need to calculate support (VIS_NO_EVAL, support set to MODIFIABLE)
  @retval  EFI_INVALID_PARAMETER  Varstore field map entry has support flag defined,
                                  but no function to determine FIELD_SUPPORT value
  @retval  EFI_OUT_OF_RESOURCES   Unable to allocate memory for CurrentField
  @retval  !EFI_SUCCESS           One of support check functions failed
**/
EFI_STATUS
EvaluateMapEntrySupport (
  IN   HII_VARSTORE_MAP_CFG  *VarStoreMapCfg,
  IN   HII_CONFIG_MAP_ENTRY  *ConfigMapEntry,
  OUT  FIELD_SUPPORT         *Support
  )
{
  BOOLEAN           GrayOut           = FALSE;
  BOOLEAN           Suppress          = FALSE;
  BOOLEAN           Supported         = FALSE;
  EFI_STATUS        Status;

  ASSERT_IF_NULL3 (VarStoreMapCfg, ConfigMapEntry, Support);

  // is there a support flag defined at all ?
  if (ConfigMapEntry->SupportFlagIdx == VIS_NO_EVAL) {
    *Support = MODIFIABLE;
    return EFI_SUCCESS;
  }

  if ((ConfigMapEntry->CheckGrayOut == NULL) &&
      (ConfigMapEntry->CheckSuppress == NULL))
  {
    // support flag defined but no function to determine value
    return EFI_INVALID_PARAMETER;
  }

  if (ConfigMapEntry->CheckSuppress != NULL) {
    Status = ConfigMapEntry->CheckSuppress (VarStoreMapCfg->DriverContext, &Supported);
    IF_RETURN (EFI_ERROR (Status), Status);
    Suppress = !Supported;
  }

  if (!Suppress &&
      (ConfigMapEntry->CheckGrayOut != NULL))
  {
    Status = ConfigMapEntry->CheckGrayOut (VarStoreMapCfg->DriverContext, &Supported);
    IF_RETURN (EFI_ERROR (Status), Status);
    GrayOut = !Supported;
  }

  if (Suppress) {
    *Support = SUPPRESS;
  } else if (GrayOut) {
    *Support = GRAYOUT;
  } else {
    *Support = MODIFIABLE;
  }

  DEBUGPRINT (HII, ("Support == %d\n", *Support));
  return EFI_SUCCESS;
}

/** Special function - getter for SupportTable in varstore (if varstore has SupportTable).

  @param[in]   VarStoreMapCfg  HII varstore map configuration structure
  @param[out]  SupportTable    Address of FIELD_SUPPORT* SupportTable contained within varstore

  @retval  EFI_SUCCESS            Operation successful
  @retval  EFI_SUCCESS            No need to calculate support (VIS_NO_EVAL, support set to MODIFIABLE)
  @retval  EFI_INVALID_PARAMETER  Varstore field map entry has support flag defined,
                                  but no function to determine FIELD_SUPPORT value
  @retval  !EFI_SUCCESS           Failed to evaluate support for one of the fields or unaffiliated fields
**/
EFI_STATUS
EvaluateSupportFlags (
  IN  HII_VARSTORE_MAP_CFG  *VarStoreMapCfg,
  IN  FIELD_SUPPORT         *SupportTable
  )
{
  EFI_STATUS            Status;
  FIELD_SUPPORT         Support;
  HII_CONFIG_MAP_ENTRY  *ConfigMapEntry;

  ASSERT_IF_NULL2 (VarStoreMapCfg, SupportTable);

  FOR_EACH_CONFIG_MAP_ENTRY (VarStoreMapCfg, ConfigMapEntry) {

    if (ConfigMapEntry->SupportFlagIdx >= VIS_IDX_NUM) {
      continue; // don't evaluate special VIS_xx flags e.g. VIS_NO_EVAL, not being indexes
    }

    Status = EvaluateMapEntrySupport (VarStoreMapCfg, ConfigMapEntry, &Support);
    IF_RETURN (EFI_ERROR (Status), Status);

    SupportTable[ConfigMapEntry->SupportFlagIdx] = Support;
  }

  // Set Support flags that are not affiliated with specific varstore field
  if (VarStoreMapCfg->EvalUnaffiliatedSupport != NULL) {
    Status = VarStoreMapCfg->EvalUnaffiliatedSupport (VarStoreMapCfg->DriverContext, SupportTable);
    IF_RETURN (EFI_ERROR (Status), Status);
  }

  return EFI_SUCCESS;
}

/** Extracts configuration bytes of <ElementOffset, ElementOffset+ElementWidth) to varstore.

  @param[in]   VarStoreMapCfg  HII varstore map configuration structure
  @param[out]  HiiConfigData   Varstore raw buffer address
  @param[in]   ElementOffset   Offset of the element within varstore
  @param[in]   ElementWidth    Width of the element within varstore

  @retval  EFI_SUCCESS            Operation successful
  @retval  EFI_INVALID_PARAMETER  ElementOffset or ElementOffset + ElementWidth are out of range
  @retval  !EFI_SUCCESS           Failed to evaluate support of the varstore field corresponding to
                                  the requested range or failed to get value of this field
**/
EFI_STATUS
ExtractConfigRange (
  IN   HII_VARSTORE_MAP_CFG  *VarStoreMapCfg,
  OUT  UINT8                 *HiiConfigData,
  IN   UINT32                ElementOffset,
  IN   UINT32                ElementWidth
  )
{
  HII_CONFIG_MAP_ENTRY  *ConfigMapEntry;
  FIELD_SUPPORT         Support;
  UINTN                 CurrentOffset;
  UINTN                 EndRangeOffset;
  EFI_STATUS            Status;

  ASSERT_IF_NULL2 (VarStoreMapCfg, HiiConfigData);

  EndRangeOffset = ElementOffset + ElementWidth;

  IF_RETURN (EndRangeOffset > VarStoreMapCfg->ConfigSize, EFI_INVALID_PARAMETER);

  CurrentOffset = ElementOffset;

  while (CurrentOffset < EndRangeOffset) {

    FOR_EACH_CONFIG_MAP_ENTRY (VarStoreMapCfg, ConfigMapEntry) {

      if ((ConfigMapEntry->FieldOffset <= CurrentOffset) &&
          (ConfigMapEntry->FieldOffset + ConfigMapEntry->FieldWidth > CurrentOffset))
      {
        DEBUGPRINT (HII, ("Found element in map %d entry, offset: %d\n", MapIdx, ConfigMapEntry->FieldOffset));
#if DBG_LVL & HII
        DEBUGPRINT (HII, ("Field name: %a \n", ConfigMapEntry->Name));
#endif /* DBG_LVL & HII */
        if (!ConfigMapEntry->CfgExecuted) {
          if (ConfigMapEntry->Get != NULL) {
            Status = EvaluateMapEntrySupport (VarStoreMapCfg, ConfigMapEntry, &Support);
            IF_RETURN (EFI_ERROR (Status), Status);

            if (Support != SUPPRESS) {
              Status = ConfigMapEntry->Get (VarStoreMapCfg->DriverContext, HiiConfigData + ConfigMapEntry->FieldOffset);
              IF_RETURN (EFI_ERROR (Status), Status);
              DEBUGPRINT (HII, ("Extracted element\n"));
              ConfigMapEntry->CfgExecuted = TRUE;
            }
          } else if (VarStoreMapCfg->HasSupportTable &&
                    (ConfigMapEntry->FieldOffset == VarStoreMapCfg->SupportTableOffset))
          { // Support table - specific case
            Status = EvaluateSupportFlags (VarStoreMapCfg, HiiConfigData + VarStoreMapCfg->SupportTableOffset);
            IF_RETURN (EFI_ERROR (Status), Status);
            ConfigMapEntry->CfgExecuted = TRUE;
          }
        }
        CurrentOffset += ConfigMapEntry->FieldWidth - 1; // -1 due to ++ after break
        break;
      }
    }
    CurrentOffset++;
  }

  return EFI_SUCCESS;
}

/** Applies RouteConfig incoming field value to current field value by overwriting
  field range that is passed in Configuration and checks whether resultant field value has changed.

  @param[in]   Configuration   A null-terminated Unicode string in <ConfigResp> format.
  @param[in]   CfgAccessInfo   Instance of HII_CFG_ACCESS_INFO structure
  @param[in]   VarStoreMapCfg  HII varstore map configuration structure
  @param[in]   ConfigMapEntry  Pointer to the specific config map entry
  @param[out]  HiiConfigData   Varstore raw buffer address
  @param[out]  FieldChanged    On output tells whether incoming RouteConfig configuration
                               for field is different than current

  @retval  EFI_SUCCESS            Operation successful
  @retval  EFI_OUT_OF_RESOURCES   Failed to allocate buffer for current field data
  @retval  !EFI_SUCCESS           Failed to extract current configuration or ConfigToBlock failure
**/
EFI_STATUS
MergeFieldConfigAndDetectChange (
  IN  CONST EFI_STRING      Configuration,
  IN  HII_CFG_ACCESS_INFO   *CfgAccessInfo,
  IN  HII_VARSTORE_MAP_CFG  *VarStoreMapCfg,
  IN  HII_CONFIG_MAP_ENTRY  *ConfigMapEntry,
  IN  UINT8                 *HiiConfigData,
  OUT BOOLEAN               *FieldChanged
  )
{
  EFI_STATUS  Status;
  INTN        BufferDifference;
  UINT8       *FieldCurrent;
  UINTN       BufferSize;
  EFI_STRING  Progress;

  FieldCurrent = AllocateZeroPool (ConfigMapEntry->FieldWidth);
  IF_NULL_RETURN (FieldCurrent, EFI_OUT_OF_RESOURCES);

  Status = ExtractConfigRange (VarStoreMapCfg, HiiConfigData, ConfigMapEntry->FieldOffset, ConfigMapEntry->FieldWidth);
  ConfigMapEntry->CfgExecuted = FALSE;
  IF_GOTO (EFI_ERROR (Status), ExitFreeRes);

  // Stash extracted current configuration in comparison buffer
  CopyMem (FieldCurrent, HiiConfigData + ConfigMapEntry->FieldOffset, ConfigMapEntry->FieldWidth);

  // Merge-Overwrite current configuration with incoming configuration
  Progress = Configuration;
  BufferSize = VarStoreMapCfg->ConfigSize;
  Status = CfgAccessInfo->HiiConfigRoutingProt->ConfigToBlock (
                                                  CfgAccessInfo->HiiConfigRoutingProt,
                                                  Configuration,
                                                  HiiConfigData,
                                                  &BufferSize,
                                                  &Progress
                                                  );
  IF_GOTO (EFI_ERROR (Status), ExitFreeRes);
  IF_GOTO ((BufferSize + 1) > VarStoreMapCfg->ConfigSize, ExitFreeRes);

  // Check if incoming configuration differs from stashed current
  BufferDifference = CompareMem (
                       FieldCurrent,
                       HiiConfigData + ConfigMapEntry->FieldOffset,
                       ConfigMapEntry->FieldWidth
                       );

  *FieldChanged = (BufferDifference != 0);

ExitFreeRes:
  FreePool (FieldCurrent);
  return Status;
}

/** Routes configuration bytes of <ElementOffset, ElementOffset+ElementWidth) from varstore.

  @param[in]   Configuration   A null-terminated Unicode string in <ConfigResp> format.
  @param[in]   CfgAccessInfo   Instance of HII_CFG_ACCESS_INFO structure
  @param[in]   VarStoreMapCfg  HII varstore map configuration structure
  @param[in]   HiiConfigData   Varstore raw buffer address
  @param[in]   ElementOffset   Offset of the element within varstore
  @param[in]   ElementWidth    Width of the element within varstore

  @retval  EFI_SUCCESS            Operation successful
  @retval  EFI_INVALID_PARAMETER  ElementOffset or ElementOffset + ElementWidth are out of range
  @retval  !EFI_SUCCESS           Failed to evaluate support of the varstore field corresponding to
                                  the requested range or failed to set value of this field
**/
EFI_STATUS
RouteConfigRange (
  IN  CONST EFI_STRING      Configuration,
  IN  HII_CFG_ACCESS_INFO   *CfgAccessInfo,
  IN  HII_VARSTORE_MAP_CFG  *VarStoreMapCfg,
  IN  UINT8                 *HiiConfigData,
  IN  UINT32                ElementOffset,
  IN  UINT32                ElementWidth
  )
{
  HII_CONFIG_MAP_ENTRY  *ConfigMapEntry;
  UINT32                CurrentOffset;
  UINTN                 EndRangeOffset;
  FIELD_SUPPORT         Support;
  EFI_STATUS            Status;
  BOOLEAN               FieldChanged;

  ASSERT_IF_NULL2 (VarStoreMapCfg, HiiConfigData);

  EndRangeOffset = ElementOffset + ElementWidth;

  IF_RETURN (EndRangeOffset > VarStoreMapCfg->ConfigSize, EFI_INVALID_PARAMETER);

  CurrentOffset = ElementOffset;

  while (CurrentOffset < EndRangeOffset) {

    FOR_EACH_CONFIG_MAP_ENTRY (VarStoreMapCfg, ConfigMapEntry) {

      if ((ConfigMapEntry->FieldOffset <= CurrentOffset) &&
          (ConfigMapEntry->FieldOffset + ConfigMapEntry->FieldWidth > CurrentOffset))
      {
        DEBUGPRINT (HII, ("Found element in map %d entry, offset: %d\n", MapIdx, ConfigMapEntry->FieldOffset));
#if DBG_LVL & HII
        DEBUGPRINT (HII, ("Field name: %a \n", ConfigMapEntry->Name));
#endif /* DBG_LVL & HII */
        if ((ConfigMapEntry->Set != NULL) &&
            !ConfigMapEntry->CfgExecuted)
        {
          Status = MergeFieldConfigAndDetectChange (
                     Configuration,
                     CfgAccessInfo,
                     VarStoreMapCfg,
                     ConfigMapEntry,
                     HiiConfigData,
                     &FieldChanged
                     );
          IF_RETURN (EFI_ERROR (Status), Status);

          if (FieldChanged) {
            if (ConfigMapEntry->SetBlocked) {
              ConfigMapEntry->SetAttempted = TRUE;
              return EFI_SUCCESS;
            }
            DEBUGPRINT (HII, ("FieldChanged\n"));
            Status = EvaluateMapEntrySupport (VarStoreMapCfg, ConfigMapEntry, &Support);
            IF_RETURN (EFI_ERROR (Status), Status);
            if (Support == MODIFIABLE) {
              Status = ConfigMapEntry->Set (VarStoreMapCfg->DriverContext, HiiConfigData + ConfigMapEntry->FieldOffset);
              IF_RETURN (EFI_ERROR (Status), Status);
              DEBUGPRINT (HII, ("Routed element\n"));
              DEBUGWAIT (HII);
            }
          }
          ConfigMapEntry->CfgExecuted = TRUE;
        }
        CurrentOffset += ConfigMapEntry->FieldWidth - 1; // -1 due to ++ after break
        break;
      }
    }
    CurrentOffset++;
  }

  return EFI_SUCCESS;
}

/** Return the pointer to the section of the Request string that begins just
   after the <ConfigHdr>.

   @param[in]   Request   A request string starting with "GUID="

   @return     Pointer to request string section after header or NULL if Request
               string ends before reaching it
**/
EFI_STRING
SkipConfigHeader (
  IN EFI_STRING Request
  )
{
  EFI_STRING StringPtr;

  if (Request == NULL) {
    return NULL;
  }

  StringPtr = Request;

  // Requets string must start with "GUID="
  if (StrnCmp (StringPtr, L"GUID=", StrLen (L"GUID=")) != 0) {
    return NULL;
  }

  while ((*StringPtr != 0)
    && StrnCmp (StringPtr, L"PATH=", StrLen (L"PATH=")) != 0)
  {
    StringPtr++;
  }
  if (*StringPtr == 0) {
    return NULL;
  }

  while ((*StringPtr != L'&')
    && (*StringPtr != 0))
  {
    StringPtr++;
  }
  if (*StringPtr == 0) {
    return NULL;
  }

  return StringPtr;
}

/** Get the value of <Number> in <BlockConfig> format, i.e. the value of OFFSET
  or WIDTH or VALUE.

  <BlockConfig> ::= 'OFFSET='<Number>&'WIDTH='<Number>&'VALUE'=<Number>
  This is a internal function.

  (Note - This function was taken from EDK2 Code.)

  @param[in]  StringPtr         String in <BlockConfig> format and points to the
                                first character of <Number>.
  @param[in]  Number            The output value. Caller takes the responsibility
                                to free memory.
  @param[out]  Len              Length of the <Number>, in characters.

  @retval EFI_INVALID_PARAMETER  NULL pointers to OUT or IN parameters or empty
                                 input string.
  @retval EFI_OUT_OF_RESOURCES   Insufficient resources to store neccessary
                                 structures.
  @retval EFI_SUCCESS            Value of <Number> is outputted in Number
                                 successfully.
**/
EFI_STATUS
GetValueOfNumber (
  IN  EFI_STRING  StringPtr,
  OUT UINT8       **Number,
  OUT UINTN       *Len
  )
{
  EFI_STATUS  Status     = EFI_SUCCESS;
  EFI_STRING  TmpPtr     = NULL;
  UINTN       Length     = 0;
  EFI_STRING  Str        = NULL;
  UINT8       *Buf       = NULL;
  UINT8       DigitUint8 = 0;
  UINTN       Index      = 0;
  CHAR16      TemStr[2];

  IF_NULL3_RETURN (StringPtr, Number, Len, EFI_INVALID_PARAMETER);
  IF_RETURN (*StringPtr == L'\0', EFI_INVALID_PARAMETER);

  TmpPtr = StringPtr;
  while ((*StringPtr != L'\0')
    && (*StringPtr != L'&'))
  {
    StringPtr++;
  }

  // Calculate the token size.
  *Len   = StringPtr - TmpPtr;
  Length = *Len + 1;

  Str = (EFI_STRING) AllocateZeroPool (Length * sizeof (CHAR16));
  if (Str == NULL) {
    DEBUGPRINT (CRITICAL, ("Failed to allocate Str!"));
    Status = EFI_OUT_OF_RESOURCES;
    goto Exit;
  }
  CopyMem (Str, TmpPtr, (*Len) * sizeof (CHAR16));
  *(Str + *Len) = L'\0';

  Length = (Length + 1) / 2;
  Buf = (UINT8 *) AllocateZeroPool (Length);
  if (Buf == NULL) {
    DEBUGPRINT (CRITICAL, ("Failed to allocate Buf!"));
    Status = EFI_OUT_OF_RESOURCES;
    goto Exit;
  }

  Length = *Len;
  ZeroMem (TemStr, sizeof (TemStr));
  for (Index = 0; Index < Length; Index++) {
    TemStr[0] = Str[Length - Index - 1];
    DigitUint8 = (UINT8) StrHexToUint64 (TemStr);
    if ((Index & 1) == 0) {
      Buf[Index / 2] = DigitUint8;
    } else {
      Buf[Index / 2] = (UINT8) ((DigitUint8 << 4) + Buf[Index / 2]);
    }
  }

  *Number = Buf;
  Status  = EFI_SUCCESS;

Exit:
  if (Str != NULL) {
    FreePool (Str);
  }

  return Status;
}

/** Find next element in the Request or Config string and return its parameters.

   The Request or Config string consists of 1 or more elements like:
   "OFFSET=hexnum&WIDTH=hexnum&VALUE=binaryhexstring...".

   @param[in,out]  CurrentReqRespElement  Request or Config string
   @param[out]     ElementOffset          Offset of the element
   @param[out]     ElementWidth           Width of the element

   @retval       EFI_SUCCESS            Operation successful
   @retval       EFI_INVALID_PARAMETER  Invalid input parameter or invalid format of input string
   @retval       !EFI_SUCCESS           Failed to convert numeric in string to numeric variable
**/
EFI_STATUS
GetNextRequestOrConfigElement (
  IN  OUT  EFI_STRING  *CurrentReqRespElement,
  OUT      UINTN       *ElementOffset,
  OUT      UINTN       *ElementWidth
  )
{
  EFI_STRING  StringPtr;
  EFI_STATUS  Status;
  UINTN       Length;
  UINT8       *TmpBuffer;
  UINTN       Offset;
  UINTN       Width;

  IF_NULL3_RETURN (CurrentReqRespElement, ElementOffset, ElementWidth, EFI_INVALID_PARAMETER);
  IF_NULL_RETURN (*CurrentReqRespElement, EFI_INVALID_PARAMETER);

  StringPtr = *CurrentReqRespElement;

  // End of the string, no more elements to parse
  if (StringPtr[0] == L'\0') {
    *CurrentReqRespElement = NULL;
    return EFI_SUCCESS;
  }

  // Unexpected content - error
  // For now only <BlockName> pairs are supported, no <NvConfig> (Name/Value)
  IF_RETURN (StrnCmp (StringPtr, L"&OFFSET=", StrLen (L"&OFFSET=")) != 0, EFI_INVALID_PARAMETER);

  StringPtr += StrLen (L"&OFFSET=");

  // Get Offset
  Status = GetValueOfNumber (StringPtr, &TmpBuffer, &Length);
  IF_RETURN (EFI_ERROR (Status), Status);

  Offset = 0;
  CopyMem (
    &Offset,
    TmpBuffer,
    (((Length + 1) / 2) < sizeof (UINTN)) ? ((Length + 1) / 2) : sizeof (UINTN)
  );
  FreePool (TmpBuffer);

  StringPtr += Length;

  IF_RETURN (StrnCmp (StringPtr, L"&WIDTH=", StrLen (L"&WIDTH=")) != 0, EFI_INVALID_PARAMETER);
  StringPtr += StrLen (L"&WIDTH=");

  // Get Width
  Status = GetValueOfNumber (StringPtr, &TmpBuffer, &Length);
  IF_RETURN (EFI_ERROR (Status), Status);
  Width = 0;
  CopyMem (
    &Width,
    TmpBuffer,
    (((Length + 1) / 2) < sizeof (UINTN)) ? ((Length + 1) / 2) : sizeof (UINTN)
  );
  FreePool (TmpBuffer);

  StringPtr += Length;

  *ElementWidth = Width;
  *ElementOffset = Offset;

  // for <BlockConfig> (in RouteConfig) skip VALUE
  if (StrnCmp (StringPtr, L"&VALUE=", StrLen (L"&VALUE=")) == 0) {
    StringPtr += StrLen (L"&VALUE=");
    while ((*StringPtr != L'\0') &&
           (*StringPtr != L'&'))
    {
      StringPtr++; // skip to the end of number
    }
  }

  *CurrentReqRespElement = StringPtr;
  return EFI_SUCCESS;
}


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
  )
{
  EFI_STATUS  Status;
  EFI_STRING  ConfigElement;
  UINTN       ElementOffset;
  UINTN       ElementWidth;

  ConfigElement = SkipConfigHeader (Configuration);
  IF_NULL_RETURN (ConfigElement, EFI_INVALID_PARAMETER);

  while (*ConfigElement != L'\0') {
    Status = GetNextRequestOrConfigElement (&ConfigElement, &ElementOffset, &ElementWidth);
    IF_RETURN (EFI_ERROR (Status), EFI_INVALID_PARAMETER);

    DEBUGPRINT (HII, ("Checking element @ OFFSET: %d, WIDTH: %d\n", ElementOffset, ElementWidth));

    if (FieldOffset == ElementOffset) {
      return EFI_SUCCESS;
    }
    if ((FieldOffset > ElementOffset) &&
        (FieldOffset < (ElementOffset + ElementWidth)))
    {
      return EFI_SUCCESS;
    }
  }

  return EFI_NOT_FOUND;
}

/** This function allows a caller to extract the current configuration for one
   or more named elements from the target driver.

   @param[in]   This       Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
   @param[in]   Request    A null-terminated Unicode string in <ConfigRequest> format.
   @param[out]  Progress   On return, points to a character in the Request string.
                           Points to the string's null terminator if request was successful.
                           Points to the most recent '&' before the first failing name/value
                           pair (or the beginning of the string if the failure is in the
                           first name/value pair) if the request was not successful.
   @param[out]   Results   A null-terminated Unicode string in <ConfigAltResp> format which
                           has all values filled in for the names in the Request string.
                           String to be allocated by the called function.

   @retval   EFI_SUCCESS            The Results string is filled with the values
                                    corresponding to all requested names.
   @retval   EFI_OUT_OF_RESOURCES   Not enough memory to store the parts of the results
                                    that must be stored awaiting possible future protocols
   @retval   EFI_NOT_FOUND          A configuration element matching the routing data is
                                    not found. Progress set to the first character in the
                                    routing header.
   @retval   EFI_INVALID_PARAMETER  Illegal syntax. Progress set to most recent "&" before
                                    the error or the beginning of the string.
   @retval   EFI_INVALID_PARAMETER  If This or Results or Progress is NULL
   @retval   EFI_ACCESS_DENIED      The action violated a system policy.
   @retval   EFI_DEVICE_ERROR       Failed to extract the current configuration for one or
                                    more named elements.
   @retval   EFI_DEVICE_ERROR       Failed to construct <ConfigHdr> template.
**/
EFI_STATUS
EFIAPI
ExtractConfig (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL  *This,
  IN  CONST EFI_STRING                      Request,
  OUT       EFI_STRING                      *Progress,
  OUT       EFI_STRING                      *Results
  )
{
  EFI_STRING            ConfigRequestHdr  = NULL;
  EFI_STRING            ConfigRequest     = NULL;
  EFI_STRING            RequestElement    = NULL;
  EFI_STRING            BlockToConfigProgress;
  BOOLEAN               OriginalRequestUsed  = TRUE;
  UINTN                 RequestSize;
  UINTN                 ElementOffset     = 0;
  UINTN                 ElementWidth      = 0;
  HII_CFG_ACCESS_INFO   *CfgAccessInfo;
  HII_VARSTORE_MAP_CFG  *VarStoreMapCfg;
  HII_CONFIG_MAP_ENTRY  *ConfigMapEntry;
  EFI_STATUS            Status            = EFI_INVALID_PARAMETER;
  UINT8                 *HiiConfigData;

  DEBUGPRINT (HII, ("=== ExtractConfig: Incoming Request  ===\n%s\n=== End Request ===\n", Request));
  DEBUGWAIT (HII);

  IF_NULL3_RETURN (This, Progress, Results, EFI_INVALID_PARAMETER);

  *Progress = Request;

  CfgAccessInfo = HII_CFG_ACCESS_INFO_FROM_HII_CONFIG_ACCESS_PROT (This);

  VarStoreMapCfg = GetVarStoreMapCfg (CfgAccessInfo, Request);
  IF_NULL_RETURN (VarStoreMapCfg, EFI_NOT_FOUND);
  ASSERT (VarStoreMapCfg->DriverContext != NULL);

  DEBUGPRINT (HII, ("VarStoreMapCfg name: %s, size: %d\n", VarStoreMapCfg->ConfigName, VarStoreMapCfg->ConfigSize));

  HiiConfigData = AllocateZeroPool (VarStoreMapCfg->ConfigSize);
  IF_NULL_RETURN (HiiConfigData, EFI_OUT_OF_RESOURCES);

  if ((Request == NULL) ||
      (StrStr (Request, L"OFFSET") == NULL)) // <ConfigRequest> has no <RequestElement>
  {
    // Request is NULL or incomplete, construct full request string.
    ConfigRequestHdr = HiiConstructConfigHdr (
                         &VarStoreMapCfg->ConfigGuid,
                         VarStoreMapCfg->ConfigName,
                         CfgAccessInfo->InstallationHandle
                         );
    if (ConfigRequestHdr == NULL) {
      Status = EFI_DEVICE_ERROR;
      goto ExitExtractError;
    }

    // Allocate buffer for <ConfigHdr> + 32(<RequestElement> == "&OFFSET=0&WIDTH=WWWWWWWWWWWWWWWW") + NULL.
    RequestSize = (StrLen (ConfigRequestHdr) + 32 + 1) * sizeof (CHAR16);
    ConfigRequest = AllocateZeroPool (RequestSize);
    if (ConfigRequest == NULL) {
      DEBUGPRINT (CRITICAL, ("Failed to allocate ConfigRequest!\n"));
      FreePool (ConfigRequestHdr);
      return EFI_OUT_OF_RESOURCES;
    }

    OriginalRequestUsed = FALSE;
    UnicodeSPrint (ConfigRequest, RequestSize, L"%s&OFFSET=0&WIDTH=%016LX", ConfigRequestHdr, VarStoreMapCfg->ConfigSize);
    FreePool (ConfigRequestHdr);
  } else {
    ConfigRequest = Request;
  }

  // At this point Request has a correct <ConfigHdr>. Jump over the header and
  // start parsing <RequestElement>'s
  *Progress = RequestElement = SkipConfigHeader (ConfigRequest);
  IF_NULL_RETURN (RequestElement, EFI_INVALID_PARAMETER);

  FOR_EACH_CONFIG_MAP_ENTRY (VarStoreMapCfg, ConfigMapEntry) {
    ConfigMapEntry->CfgExecuted = FALSE;
  }

  if (VarStoreMapCfg->PreExtract != NULL) {
    Status = VarStoreMapCfg->PreExtract (VarStoreMapCfg->DriverContext, VarStoreMapCfg, HiiConfigData, Request);
    IF_GOTO (EFI_ERROR (Status), ExitExtractError);
  }

  // Don't read all parameters from NVM, only those included in the Request string.
  // On TM UEFI calls ExtractConfig for each individual parameter, so it would
  // cost a lot of time to read all parameters during each ExtractConfig execution.
  // Each <RequestElement> consists of a OFFSET, WIDTH pair.
  while (*RequestElement != L'\0') {

    Status = GetNextRequestOrConfigElement (&RequestElement, &ElementOffset, &ElementWidth);
    IF_GOTO (EFI_ERROR (Status), ExitExtractError);

    DEBUGPRINT (HII, ("Processing req. element %s, OFFSET: %d, WIDTH: %d\n", RequestElement, ElementOffset, ElementWidth));

    Status = ExtractConfigRange (VarStoreMapCfg, HiiConfigData, ElementOffset, ElementWidth);
    IF_GOTO (EFI_ERROR (Status), ExitExtractError);

    DEBUGPRINT (HII, ("Processed req. element\n"));
    if (OriginalRequestUsed) {
      *Progress = RequestElement;
    }
  }

  if (VarStoreMapCfg->PostExtract != NULL) {
    Status = VarStoreMapCfg->PostExtract (VarStoreMapCfg->DriverContext, VarStoreMapCfg);
    IF_GOTO (EFI_ERROR (Status), ExitExtractError);
  }

  // Configuration has been correctly extracted from adapter.
  // Create Result string using helper function from UEFI BIOS and exit.
  Status = CfgAccessInfo->HiiConfigRoutingProt->BlockToConfig (
                                                  CfgAccessInfo->HiiConfigRoutingProt,
                                                  ConfigRequest,
                                                  HiiConfigData,
                                                  VarStoreMapCfg->ConfigSize,
                                                  Results,
                                                  &BlockToConfigProgress
                                                  );
  if (OriginalRequestUsed) {
    *Progress = BlockToConfigProgress;
  }
  if (EFI_ERROR (Status)) {
    DEBUGPRINT (CRITICAL, ("BlockToConfig failed with %r\n", Status));
  }

ExitExtractError:
  if (!OriginalRequestUsed) {
    FreePool (ConfigRequest);
    ConfigRequest = NULL;
  }

  FreePool (HiiConfigData);

  DEBUGPRINT (HII, ("=== ExtractConfig: End Progress  ===\n%s\n=== ===\n", *Progress));
  DEBUGPRINT (HII, ("=== ExtractConfig: End Results  ===\n%s\n=== ===\n",  *Results));
  DEBUGPRINT (HII, ("=== ExtractConfig: End Status  - %r \n",  Status));
  DEBUGWAIT (HII);
  return Status;
}

/** This function processes the results of changes in configuration for the driver that
   published this protocol.

   @param[in]   This           Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
   @param[in]   Configuration  A null-terminated Unicode string in <ConfigResp> format.
   @param[out]  Progress       A pointer to a string filled in with the offset of the most
                               recent '&' before the first failing name/value pair (or the
                               beginning of the string if the failure is in the first
                               name/value pair) or the terminating NULL if all was successful.

   @retval   EFI_SUCCESS            The results have been distributed or are awaiting distribution
   @retval   EFI_OUT_OF_RESOURCES   Not enough memory to store the parts of the results
                                    that must be stored awaiting possible future protocols
   @retval   EFI_INVALID_PARAMETER  If This or Configuration or Progress is NULL
   @retval   EFI_NOT_FOUND          Target for the specified routing data was not found
   @retval   EFI_ACCESS_DENIED      The action violated a system policy.
   @retval   EFI_DEVICE_ERROR       Failed to route the configuration for one or
                                    more named elements.
**/
EFI_STATUS
EFIAPI
RouteConfig (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL  *This,
  IN  CONST EFI_STRING                      Configuration,
  OUT       EFI_STRING                      *Progress
  )
{
  EFI_STATUS            Status;
  HII_CFG_ACCESS_INFO   *CfgAccessInfo;
  HII_VARSTORE_MAP_CFG  *VarStoreMapCfg;
  HII_CONFIG_MAP_ENTRY  *ConfigMapEntry;
  UINTN                 BufferSize;
  UINT8                 *HiiConfigData;
  EFI_STRING            ConfigElement     = NULL;
  UINTN                 ElementOffset     = 0;
  UINTN                 ElementWidth      = 0;

  IF_NULL3_RETURN (This, Configuration, Progress, EFI_INVALID_PARAMETER);

  DEBUGPRINT (HII, ("=== RouteConfig: Configuration ===\n%s\n=== End Configuration ===\n", Configuration));
  DEBUGWAIT (HII);

  *Progress = Configuration;

  CfgAccessInfo = HII_CFG_ACCESS_INFO_FROM_HII_CONFIG_ACCESS_PROT (This);

  VarStoreMapCfg = GetVarStoreMapCfg (CfgAccessInfo, Configuration);
  IF_NULL_RETURN (VarStoreMapCfg, EFI_NOT_FOUND);
  ASSERT (VarStoreMapCfg->DriverContext != NULL);

  HiiConfigData = AllocateZeroPool (VarStoreMapCfg->ConfigSize);
  IF_NULL_RETURN (HiiConfigData, EFI_OUT_OF_RESOURCES);

  BufferSize = VarStoreMapCfg->ConfigSize;
  Status = CfgAccessInfo->HiiConfigRoutingProt->ConfigToBlock (
                                                  CfgAccessInfo->HiiConfigRoutingProt,
                                                  Configuration,
                                                  HiiConfigData,
                                                  &BufferSize,
                                                  Progress
                                                  );
  IF_GOTO (EFI_ERROR (Status), ExitRouteError);

  // On return BufferSize should contain last modified byte index in config data structure
  IF_GOTO (BufferSize + 1 > VarStoreMapCfg->ConfigSize, ExitRouteError);

  // Reset Progress after ConfigToBlock successfull call
  *Progress = ConfigElement = SkipConfigHeader (Configuration);
  IF_NULL_RETURN (ConfigElement, EFI_INVALID_PARAMETER);

  FOR_EACH_CONFIG_MAP_ENTRY (VarStoreMapCfg, ConfigMapEntry) {
    ConfigMapEntry->CfgExecuted  = FALSE;
    ConfigMapEntry->SetBlocked   = FALSE;
    ConfigMapEntry->SetAttempted = FALSE;
  }

  if (VarStoreMapCfg->PreRoute != NULL) {
    Status = VarStoreMapCfg->PreRoute (VarStoreMapCfg->DriverContext, VarStoreMapCfg, HiiConfigData, Configuration);
    IF_GOTO (EFI_ERROR (Status), ExitRouteError);
  }

  while (*ConfigElement != L'\0') {

    Status = GetNextRequestOrConfigElement (&ConfigElement, &ElementOffset, &ElementWidth);
    IF_GOTO (EFI_ERROR (Status), ExitRouteError);

    DEBUGPRINT (HII, ("Processing cfg element @ OFFSET: %d, WIDTH: %d\n", ElementOffset, ElementWidth));

    Status = RouteConfigRange (
               Configuration,
               CfgAccessInfo,
               VarStoreMapCfg,
               HiiConfigData,
               ElementOffset,
               ElementWidth
               );
    IF_GOTO (EFI_ERROR (Status), ExitRouteError);

    DEBUGPRINT (HII, ("Processed cfg element\n"));
    *Progress = ConfigElement;
  }

  if (VarStoreMapCfg->PostRoute != NULL) {
    Status = VarStoreMapCfg->PostRoute (VarStoreMapCfg->DriverContext, VarStoreMapCfg);
    IF_GOTO (EFI_ERROR (Status), ExitRouteError);
  }

ExitRouteError:
  if (HiiConfigData != NULL) {
    FreePool (HiiConfigData);
  }

  DEBUGPRINT (HII, ("=== RouteConfig: End Progress  ===\n%s\n=== ===\n", *Progress));
  DEBUGPRINT (HII, ("=== RouteConfig: End Status  - %r \n",  Status));
  DEBUGWAIT (HII);
  return Status;
}

/** This function is called to provide results data to the driver.

   @param[in]      This            Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
   @param[in]      Action          Specifies the type of action taken by the browser.
   @param[in]      QuestionId      A unique value which is sent to the original exporting driver
                                   so that it can identify the type of data to expect.
   @param[in]      Type            The type of value for the question.
   @param[in,out]  Value           A pointer to the data being sent to the original exporting driver.
   @param[out]     ActionRequest   On return, points to the action requested by the callback function.

   @retval   EFI_SUCCESS           The callback successfully handled the action.
   @retval   EFI_OUT_OF_RESOURCES  Not enough storage is available to hold the variable and its data.
   @retval   EFI_DEVICE_ERROR      The variable could not be saved.
   @retval   EFI_UNSUPPORTED       The specified Action is not supported by the callback.
**/
EFI_STATUS
EFIAPI
CallBack (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL  *This,
  IN        EFI_BROWSER_ACTION              Action,
  IN        EFI_QUESTION_ID                 QuestionId,
  IN        UINT8                           Type,
  IN OUT    EFI_IFR_TYPE_VALUE              *Value,
  OUT       EFI_BROWSER_ACTION_REQUEST      *ActionRequest
  )
{
  EFI_STATUS                       Status;
  HII_CFG_ACCESS_INFO              *CfgAccessInfo;
  VOID                             *CallBackContext;
  HII_ACCESS_FORM_ACTION_CALLBACK  ActionCallBack;

  IF_NULL3_RETURN (This, Value, ActionRequest, EFI_INVALID_PARAMETER);

  CfgAccessInfo   = HII_CFG_ACCESS_INFO_FROM_HII_CONFIG_ACCESS_PROT (This);
  CallBackContext = CfgAccessInfo->CallBackDriverContext;
  ASSERT (CallBackContext != NULL);

  *ActionRequest = EFI_BROWSER_ACTION_REQUEST_NONE;

  switch (Action) {
  case EFI_BROWSER_ACTION_CHANGING:
    ActionCallBack = CfgAccessInfo->OnActionChanging;
    Status = EFI_SUCCESS;
    break;
  case EFI_BROWSER_ACTION_CHANGED:
    ActionCallBack = CfgAccessInfo->OnActionChanged;
    Status = EFI_SUCCESS;
    break;
  case EFI_BROWSER_ACTION_FORM_OPEN:
    ActionCallBack = CfgAccessInfo->OnActionFormOpen;
    Status = EFI_SUCCESS;
  case EFI_BROWSER_ACTION_FORM_CLOSE:
    ActionCallBack = CfgAccessInfo->OnActionFormClose;
    Status = EFI_SUCCESS;
    break;
  case EFI_BROWSER_ACTION_DEFAULT_STANDARD:
    ActionCallBack = CfgAccessInfo->OnActionDefaultStandard;
    Status = EFI_UNSUPPORTED;
    break;
  case EFI_BROWSER_ACTION_DEFAULT_MANUFACTURING:
    ActionCallBack = CfgAccessInfo->OnActionDefaultManufacturing;
    Status = EFI_UNSUPPORTED;
    break;
  case EFI_BROWSER_ACTION_DEFAULT_SAFE:
    ActionCallBack = CfgAccessInfo->OnActionDefaultSafe;
    Status = EFI_UNSUPPORTED;
    break;
  case EFI_BROWSER_ACTION_DEFAULT_PLATFORM:
    ActionCallBack = CfgAccessInfo->OnActionDefaultPlatform;
    Status = EFI_UNSUPPORTED;
    break;
  case EFI_BROWSER_ACTION_DEFAULT_HARDWARE:
    ActionCallBack = CfgAccessInfo->OnActionDefaultHardware;
    Status = EFI_UNSUPPORTED;
    break;
  case EFI_BROWSER_ACTION_DEFAULT_FIRMWARE:
    ActionCallBack = CfgAccessInfo->OnActionDefaultFirmware;
    Status = EFI_UNSUPPORTED;
    break;
  case EFI_BROWSER_ACTION_RETRIEVE:
    ActionCallBack = CfgAccessInfo->OnActionRetrieve;
    Status = EFI_UNSUPPORTED;
    break;
  case EFI_BROWSER_ACTION_SUBMITTED:
    ActionCallBack = CfgAccessInfo->OnActionSubmitted;
    Status = EFI_UNSUPPORTED;
    break;
  default:
    ActionCallBack = NULL;
    Status = EFI_UNSUPPORTED;
    break;
  }

  if (ActionCallBack != NULL) {
    Status = ActionCallBack (CallBackContext, QuestionId, Type, Value, ActionRequest);
    DEBUGPRINT (HII, ("Action %x callback finished with status: %r\n", Action, Status));
  }

  return Status;
}

EFI_HII_CONFIG_ACCESS_PROTOCOL gHiiConfigAccessProt = {
  ExtractConfig,
  RouteConfig,
  CallBack
};
