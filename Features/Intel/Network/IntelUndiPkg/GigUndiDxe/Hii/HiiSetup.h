/******************************************************************************
**                                                                           **
** INTEL CONFIDENTIAL                                                        **
**                                                                           **
** Copyright (c) 2020 Intel Corporation. All rights reserved.                **
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
#ifndef HII_SETUP_H_
#define HII_SETUP_H_

#include "CommonDriver.h"

/** Gets language agnostic inventory string (the same for all languages).
  Called once for all languages. Meant for static but HW dependent strings.

  @param[in]   UndiPrivateData  Pointer to driver private data structure
  @param[out]  InvString        Pointer to resultant string

  @retval    EFI_SUCCESS       Operation successful
  @retval    !EFI_SUCCESS      Failed to retrieve string
**/
typedef
EFI_STATUS
(*HII_INV_STRING_GET_ALL_LANG) (
  IN  UNDI_PRIVATE_DATA  *UndiPrivateData,
  OUT EFI_STRING         InvString
  );

/** Gets language specific inventory string (different for every language).
  Called separately for every language. Meant for static but HW dependent strings.

  @param[in]   UndiPrivateData  Pointer to driver private data structure
  @param[out]  InvString        Pointer to resultant string
  @param[in]   Language         Language for which string should be retrieved

  @retval      EFI_SUCCESS       Operation successful
  @retval      !EFI_SUCCESS      Failed to retrieve string
**/
typedef
EFI_STATUS
(*HII_INV_STRING_GET_LANG) (
  IN        UNDI_PRIVATE_DATA  *UndiPrivateData,
  OUT       EFI_STRING         InvString,
  IN  CONST CHAR8              *Language
  );

/** Gets language agnostic inventory string (the same for all languages) when EFI_STRING_ID
  is needed to determine string contents. Called once for all languages.
  Meant for static but HW dependent strings.

  @param[in]   UndiPrivateData  Pointer to driver private data structure
  @param[out]  InvString        Pointer to resultant string
  @param[in]   StringId         EFI_STRING_ID of required string

  @retval    EFI_SUCCESS       Operation successful
  @retval    !EFI_SUCCESS      Failed to retrieve string
**/
typedef
EFI_STATUS
(*HII_INV_STRING_GET_ALL_LANG_FOR_STRING_ID) (
  IN  UNDI_PRIVATE_DATA  *UndiPrivateData,
  OUT EFI_STRING         InvString,
  IN  EFI_STRING_ID      StringId
  );

/** Gets language specific inventory string (different for every language) when EFI_STRING_ID
  is needed to determine string contents. Called separately for every language.
  Meant for static but HW dependent strings.

  @param[in]   UndiPrivateData  Pointer to driver private data structure
  @param[out]  InvString        Pointer to resultant string
  @param[in]   Language         Language for which string should be retrieved
  @param[in]   StringId         EFI_STRING_ID of required string

  @retval      EFI_SUCCESS       Operation successful
  @retval      !EFI_SUCCESS      Failed to retrieve string
**/
typedef
EFI_STATUS
(*HII_INV_STRING_GET_LANG_FOR_STRING_ID) (
  IN            UNDI_PRIVATE_DATA  *UndiPrivateData,
  IN OUT        EFI_STRING         InvString,
  IN      CONST CHAR8              *Language,
  IN            EFI_STRING_ID      StringId
  );

typedef union HII_INV_STRING_GET_U {
  HII_INV_STRING_GET_ALL_LANG                GetAllLang;
  HII_INV_STRING_GET_LANG                    GetLang;
  HII_INV_STRING_GET_ALL_LANG_FOR_STRING_ID  GetAllLangForStringId;
  HII_INV_STRING_GET_LANG_FOR_STRING_ID      GetLangForStringId;
  VOID                                       *Raw;
} HII_INV_STRING_GET;

typedef enum HII_INV_STRING_GET_TYPE_E {
  ALL_LANG = 0,
  LANG,
  ALL_LANG_FOR_STRING_ID,
  LANG_FOR_STRING_ID,
} HII_INV_STRING_GET_TYPE;

/** This structure describes Inventory string entry Getter function in static Inventory string map
**/
typedef struct HII_STATIC_INV_STRING_ENTRY_S {
  EFI_STRING_ID                   StringId;      ///< StringId of inventory string
  HII_INV_STRING_GET_TYPE         GetStringType; ///< specifies which one of 4 types of Getters is used
  BOOLEAN                         HasXUefi;      ///< specifies whether Getter should be processed for x-UEFI language
  HII_INV_STRING_GET              GetString;     ///< inventory string Getter function
  HII_CONFIG_FIELD_CHECK_SUPPORT  CheckSupport;  ///< function that checks if inv. string is supported (can be NULL)
} HII_STATIC_INV_STRING_ENTRY;

/** Shorthand macro to define static inventory map entry for HII_INV_STRING_GET_ALL_LANG

  @param[in]   StringId   EFI_STRING_ID of inventory string
  @param[out]  XUefiSupp  Tells if inv. string has x-UEFI string
  @param[in]   GetFunc    Getter of HII_INV_STRING_GET_ALL_LANG type
  @param[in]   CheckSupp  Getter is processed only if this function returns TRUE or is NULL
**/
#define ALL_LANG_ENTRY(StringId, XUefiSupp, GetFunc, CheckSupp)  \
  {StringId, ALL_LANG,                XUefiSupp, {.GetAllLang = GetFunc},            CheckSupp}

/** Shorthand macro to define static inventory map entry for HII_INV_STRING_GET_LANG

  @param[in]   StringId   EFI_STRING_ID of inventory string
  @param[out]  XUefiSupp  Tells if inv. string has x-UEFI string
  @param[in]   GetFunc    Getter of HII_INV_STRING_GET_LANG type
  @param[in]   CheckSupp  Getter is processed only if this function returns TRUE or is NULL
**/
#define LANG_ENTRY(StringId, XUefiSupp, GetFunc, CheckSupp)  \
  {StringId, LANG,                    XUefiSupp, {.GetLang = GetFunc},               CheckSupp}

/** Shorthand macro to define static inventory map entry for HII_INV_STRING_GET_ALL_LANG_FOR_STRING_ID

  @param[in]   StringId   EFI_STRING_ID of inventory string
  @param[out]  XUefiSupp  Tells if inv. string has x-UEFI string
  @param[in]   GetFunc    Getter of HII_INV_STRING_GET_ALL_LANG_FOR_STRING_ID type
  @param[in]   CheckSupp  Getter is processed only if this function returns TRUE or is NULL
**/
#define ALL_LANG_STR_ID_ENTRY(StringId, XUefiSupp, GetFunc, CheckSupp)  \
  {StringId, ALL_LANG_FOR_STRING_ID,  XUefiSupp, {.GetAllLangForStringId = GetFunc}, CheckSupp}

/** Shorthand macro to define static inventory map entry for HII_INV_STRING_GET_LANG_FOR_STRING_ID

  @param[in]   StringId   EFI_STRING_ID of inventory string
  @param[out]  XUefiSupp  Tells if inv. string has x-UEFI string
  @param[in]   GetFunc    Getter of HII_INV_STRING_GET_LANG_FOR_STRING_ID type
  @param[in]   CheckSupp  Getter is processed only if this function returns TRUE or is NULL
**/
#define LANG_STR_ID_ENTRY(StringId, XUefiSupp, GetFunc, CheckSupp)  \
  {StringId, LANG_FOR_STRING_ID,      XUefiSupp, {.GetLangForStringId = GetFunc},    CheckSupp}

#endif /* HII_SETUP_H_ */