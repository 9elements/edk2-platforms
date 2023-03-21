/**************************************************************************

Copyright (c) 2021 - 2022, Intel Corporation. All rights reserved.

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

#ifndef DYNAMIC_FORMS_H_
#define DYNAMIC_FORMS_H_
#define MAX_ONEOF_VALUE_STRING_LEN 20
#include "Ice.h"

/** Helper function that allocates and creates a pair of opcodes and
   opcode and returns handles.

   @param[in]   StartLabelNumber       Parameter to be passed to StartLabel->Number
   @param[in]   EndLabelNumber         Parameter to be passed to EndLabel->Number
   @param[out]  StartOpCodeHandle      Pointer to an opcode to be created
   @param[out]  EndOpCodeHandle        Pointer to an opcode to be created

   @return      EFI_SUCCESS            Label opcodes were created
   @return      EFI_INVALID_PARAMETER  StartOpCodeHandle or EndOpCodeHandle is NULL
   @return      EFI_OUT_OF_RESOURCES   Couldn't allocate memory
**/
EFI_STATUS
HiiCreateLabelHandlePair (
  IN  UINT16              StartLabelNumber,
  IN  UINT16              EndLabelNumber,
  OUT VOID                **StartOpCodeHandle,
  OUT VOID                **EndOpCodeHandle
  );

/** Helper function that frees a pair of opcodes basing on their handles.

   @param[in]  StartOpCodeHandle      Pointer to an opcode to be destroyed
   @param[in]  EndOpCodeHandle        Pointer to an opcode to be destroyed

   @return      EFI_SUCCESS            Label opcodes were created
   @return      EFI_INVALID_PARAMETER  StartOpCodeHandle or EndOpCodeHandle is NULL
**/
EFI_STATUS
HiiDestroyLabelHandlePair (
  IN  VOID                *StartOpCodeHandle,
  IN  VOID                *EndOpCodeHandle
  );

/** Helper function that creates a text opcode from strings.

   @param[in]    HiiPkgListHandle       Hii Handle
   @param[in]    StartOpCodeHandle      Pointer to an opcode handle for a text field
   @param[in]    DisplayString          String with a name of the field
   @param[in]    ValueString            String with a value of the field

   @retval       EFI_SUCCESS            Successfully generated description strings.
   @retval       EFI_OUT_OF_RESOURCES   Failed to allocate memory.

**/
EFI_STATUS
HiiCreateText (
  IN EFI_HII_HANDLE     HiiPkgListHandle,
  IN VOID               *StartOpCodeHandle,
  IN CHAR16             *DisplayString,
  IN CHAR16             *ValueString
  );

/** Helper function that creates a string opcode

   @param[in]    UndiPrivateData        Pointer to an UNDI_PRIVATE_DATA struct
   @param[in]    HiiPkgListHandle       Hii Handle
   @param[in]    StartOpCodeHandle      Pointer to an opcode to be created
   @param[in]    QuestionId             Question ID for the string
   @param[in]    VarstoreOffset         Offset of a value for the string
   @param[in]    NameString             Name for the string field
   @param[in]    XUefiString            x-UEFI Name for the string field
   @param[in]    VarStoreID             Storage ID
   @param[in]    MaxSize                Maximum size of the string

   @retval       EFI_SUCCESS            Port option string generated properly.
   @retval       EFI_OUT_OF_RESOURCES   Failed to allocate memory.

**/
EFI_STATUS
HiiCreateString (
  IN UNDI_PRIVATE_DATA  *UndiPrivateData,
  IN EFI_HII_HANDLE     HiiPkgListHandle,
  IN VOID               *StartOpCodeHandle,
  IN EFI_QUESTION_ID    QuestionId,
  IN UINT16             VarstoreOffset,
  IN CHAR16             *NameString,
  IN CHAR16             *XUefiString,
  IN EFI_VARSTORE_ID    VarStoreID,
  IN UINT8              MaxSize
  );

/** Helper function that creates a single oneof option.

   @param[in]      UndiPrivateData      Pointer to an UNDI_PRIVATE_DATA struct
   @param[in]      HiiPkgListHandle     The HII Handle of the form to update
   @param[in]      OptionText           Text to be displayed in an oneof
   @param[in]      Type                 Type for the option
   @param[in]      Value                Value for the option
   @param[in]      Flags                Flags for the option
   @param[in, out] OptionHandle         Oneof option opcode handle

   @return      EFI_SUCCESS            Form was updated
   @return      EFI_INVALID_PARAMETER  OptionText or HiiPkgListHandle or OptionHandle is NULL
   @return      EFI_OUT_OF_RESOURCES   Couldn't allocate memory
**/
EFI_STATUS
HiiBuildOneOfOption (
  IN      UNDI_PRIVATE_DATA   *UndiPrivateData,
  IN      EFI_HII_HANDLE      HiiPkgListHandle,
  IN      CHAR16              *OptionText,
  IN      UINT8               Type,
  IN      UINT64              Value,
  IN      UINT8               Flags,
  IN OUT  VOID                **OptionHandle
  );

/** Helper function that creates a Goto.

   @param[in]     UndiPrivateData      Pointer to an UNDI_PRIVATE_DATA struct
   @param[in]     HiiPkgListHandle     The HII Handle of the form to update
   @param[in]     OpCodeHandle         Pointer to an opcode
   @param[in]     DisplayText          Text to be displayed
   @param[in]     XUefiString          optional x-UEFI String for the goto
   @param[in]     Type                 Type for the option
   @param[in]     Flags                Flags for the option
   @param[in]     FormId               Form id to where the goto shall be created
   @param[in]     QuestionId           Target question id

   @return      EFI_SUCCESS            Form was updated
   @return      EFI_INVALID_PARAMETER  OptionText or HiiPkgListHandle or OptionHandle is NULL
   @return      EFI_OUT_OF_RESOURCES   Couldn't allocate memory
**/
EFI_STATUS
HiiBuildGoto (
  IN      UNDI_PRIVATE_DATA   *UndiPrivateData,
  IN      EFI_HII_HANDLE      HiiPkgListHandle,
  IN      VOID                *OpCodeHandle,
  IN      CHAR16              *DisplayText,
  IN      CHAR16              *XUefiString     OPTIONAL,
  IN      UINT8               Type,
  IN      UINT8               Flags,
  IN      EFI_FORM_ID         FormId,
  IN      EFI_QUESTION_ID     QuestionId
  );

#endif /* DYNAMIC_FORMS_H_ */
