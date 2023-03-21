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

#include "CommonDriver.h"
#include "DynamicForms.h"
#include <Library/UefiLib.h>
#include <Guid/MdeModuleHii.h>

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
  )
{
  EFI_STATUS            Status;
  EFI_IFR_GUID_LABEL    *StartLabel;
  EFI_IFR_GUID_LABEL    *EndLabel;

  if ((StartOpCodeHandle == NULL)
    || (EndOpCodeHandle == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  // Create opcode handles for start/end labels
  *StartOpCodeHandle = HiiAllocateOpCodeHandle ();
  if (*StartOpCodeHandle == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  *EndOpCodeHandle = HiiAllocateOpCodeHandle ();
  if (*EndOpCodeHandle == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ExitFreeSOCH;
  }

  // Create HII Extend Label opcodes for start and end
  // Other items will be put in between start and end
  StartLabel = (EFI_IFR_GUID_LABEL *) HiiCreateGuidOpCode (
                                        *StartOpCodeHandle,
                                        &gEfiIfrTianoGuid,
                                        NULL,
                                        sizeof (EFI_IFR_GUID_LABEL)
                                        );

  if (StartLabel == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ExitFreeEOCH;
  }

  StartLabel->ExtendOpCode  = EFI_IFR_EXTEND_OP_LABEL;
  StartLabel->Number        = StartLabelNumber;

  EndLabel = (EFI_IFR_GUID_LABEL *) HiiCreateGuidOpCode (
                                      *EndOpCodeHandle,
                                      &gEfiIfrTianoGuid,
                                      NULL,
                                      sizeof (EFI_IFR_GUID_LABEL)
                                      );

  if (StartLabel == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ExitFreeEOCH;
  }

  EndLabel->ExtendOpCode  = EFI_IFR_EXTEND_OP_LABEL;
  EndLabel->Number        = EndLabelNumber;

  Status = EFI_SUCCESS;
  goto Exit;

ExitFreeEOCH:
  HiiFreeOpCodeHandle (*EndOpCodeHandle);
ExitFreeSOCH:
  HiiFreeOpCodeHandle (*StartOpCodeHandle);
Exit:
  return Status;
}


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
  )
{
  if ((StartOpCodeHandle == NULL)
    || (EndOpCodeHandle == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  HiiFreeOpCodeHandle (StartOpCodeHandle);
  HiiFreeOpCodeHandle (EndOpCodeHandle);

  return EFI_SUCCESS;
}

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
  )
{
  EFI_STRING_ID DisplayStringId;
  EFI_STRING_ID ValueStringId;
  VOID          *TextOpCode;

  ASSERT (HiiPkgListHandle != NULL);
  ASSERT (StartOpCodeHandle != NULL);
  ASSERT (DisplayString != NULL);
  ASSERT (ValueString != NULL);

  DisplayStringId = HiiSetString (
                      HiiPkgListHandle,
                      0,
                      DisplayString,
                      NULL
                      );
  IF_RETURN ((DisplayStringId == 0), EFI_OUT_OF_RESOURCES);

  ValueStringId = HiiSetString (
                    HiiPkgListHandle,
                    0,
                    ValueString,
                    NULL
                    );
  IF_RETURN ((ValueStringId == 0), EFI_OUT_OF_RESOURCES);

  TextOpCode = HiiCreateTextOpCode (
                 StartOpCodeHandle,
                 DisplayStringId,
                 STRING_TOKEN (STR_INV_EMPTY_STRING),
                 ValueStringId
                 );
  IF_NULL_RETURN (TextOpCode, EFI_OUT_OF_RESOURCES);

  return EFI_SUCCESS;
}

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
  )
{
  EFI_STATUS     Status;
  UINT8          *StringOpcode;
  EFI_STRING_ID  HiiStringId;

  ASSERT (HiiPkgListHandle != NULL);
  ASSERT (StartOpCodeHandle != NULL);
  ASSERT (NameString != NULL);
  ASSERT (XUefiString != NULL);

  Status = EFI_SUCCESS;

  HiiStringId = HiiSetString (
                  HiiPkgListHandle,
                  0,
                  NameString,
                  NULL
                  );
  IF_RETURN ((HiiStringId == 0), EFI_OUT_OF_RESOURCES);


  StringOpcode = HiiCreateStringOpCode (
                   StartOpCodeHandle,                             // *OpCodeHandle,
                   QuestionId,                                    // QuestionId,
                   VarStoreID,                                    // VarStoreId
                   VarstoreOffset,                                // VarOffset,
                   HiiStringId,                                   // Prompt,
                   STRING_TOKEN (STR_INV_EMPTY_STRING),           // Help,
                   EFI_IFR_FLAG_READ_ONLY,                        // QuestionFlags,
                   0,                                             // StringFlags,
                   0,                                             // MinSize,
                   MaxSize,                                       // MaxSize,
                   NULL                                           // *DefaultsOpCodeHandle  OPTIONAL
                   );
  IF_RETURN ((StringOpcode == NULL), EFI_OUT_OF_RESOURCES);

  return Status;
}

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
  )
{
  EFI_STATUS          Status;
  EFI_STRING_ID       OptionTextId;
  VOID                *OptionOpCode;
  VOID                *InternalOptionHandle;

  if ((HiiPkgListHandle == NULL)
    || (OptionText == NULL)
    || (OptionHandle == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  // Create option opcode handle
  if (*OptionHandle == NULL) {
    InternalOptionHandle = HiiAllocateOpCodeHandle ();
    IF_NULL_RETURN (InternalOptionHandle, EFI_OUT_OF_RESOURCES)
  } else {
    InternalOptionHandle = *OptionHandle;
  }

  // Create option string
  OptionTextId = HiiSetString (
                   HiiPkgListHandle,
                   0,
                   OptionText,
                   NULL
                   );
  if (OptionTextId == 0) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ExitFreeOpCode;
  }

  // Create Hii OneOf Option opcode
  OptionOpCode = HiiCreateOneOfOptionOpCode (
                   InternalOptionHandle,
                   OptionTextId,
                   Flags,
                   Type,
                   Value
                   );

  if (OptionOpCode == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ExitFreeOpCode;
  }

  if (*OptionHandle == NULL) {
    *OptionHandle = InternalOptionHandle;
  }
  return EFI_SUCCESS;

ExitFreeOpCode:
  if (*OptionHandle == NULL) {
    HiiFreeOpCodeHandle (InternalOptionHandle);
  }

  return Status;
}

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
  )
{
  EFI_STATUS          Status = EFI_SUCCESS;
  EFI_STRING_ID       GotoTextId;
  VOID                *FormOpCode;

  if ((HiiPkgListHandle == NULL)
    || (DisplayText == NULL)
    || (OpCodeHandle == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  // Create goto string
  GotoTextId = HiiSetString (
                 HiiPkgListHandle,
                 0,
                 DisplayText,
                 NULL
                 );
  IF_RETURN ((GotoTextId == 0), EFI_OUT_OF_RESOURCES);


  // Create Hii goto opcode
  FormOpCode = HiiCreateGotoOpCode (
                 OpCodeHandle,
                 FormId,
                 GotoTextId,
                 0,
                 Flags,
                 QuestionId
                 );
  IF_NULL_RETURN (FormOpCode, EFI_OUT_OF_RESOURCES);

  return Status;
}

