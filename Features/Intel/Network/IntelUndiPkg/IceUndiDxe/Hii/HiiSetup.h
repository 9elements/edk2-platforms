/**************************************************************************

Copyright (c) 2020 Intel Corporation. All rights reserved.

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
