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

#include "DynamicForms.h"
#include "PortOptions.h"
#include "Ice.h"
#include <Library/HiiLib.h>
#include "LinkTopology.h"
#include "Hii.h"

EFI_GUID          mHiiFormGuid       = HII_FORM_GUID;


/** Creates speed string out of PORT_OPTION_SPEED.

   @param[in]    Speed         PORT_OPTION_SPEED to create speed string out of.
   @param[out]   SpeedString   pointer to a string to be generated

**/
VOID
CreateSpeedString (
  IN  PORT_OPTION_SPEED Speed,
  OUT CHAR16            *SpeedString
  )
{
  switch (Speed) {
    case PortOptionSpeed100M:
      UnicodeSPrint (SpeedString, MAX_PORT_SPEED_LEN, L"0.1G");
      break;
    case PortOptionSpeed1G:
      UnicodeSPrint (SpeedString, MAX_PORT_SPEED_LEN, L"1G");
      break;
    case PortOptionSpeed2500M:
      UnicodeSPrint (SpeedString, MAX_PORT_SPEED_LEN, L"2.5G");
      break;
    case PortOptionSpeed5G:
      UnicodeSPrint (SpeedString, MAX_PORT_SPEED_LEN, L"5G");
      break;
    case PortOptionSpeed10G:
      UnicodeSPrint (SpeedString, MAX_PORT_SPEED_LEN, L"10G");
      break;
    case PortOptionSpeed25G:
      UnicodeSPrint (SpeedString, MAX_PORT_SPEED_LEN, L"25G");
      break;
    case PortOptionSpeed50G:
      UnicodeSPrint (SpeedString, MAX_PORT_SPEED_LEN, L"50G");
      break;
    case PortOptionSpeed100G:
      UnicodeSPrint (SpeedString, MAX_PORT_SPEED_LEN, L"100G");
      break;
    default:
      UnicodeSPrint (SpeedString, MAX_PORT_SPEED_LEN, L"");
  }
}
/** Creates speed count string.

   @param[in]    SpeedCount         Number of occurences of current speed
   @param[out]   SpeedCountString   pointer to a string to be generated

**/
VOID
CreateSpeedCountString (
  IN  UINT8   SpeedCount,
  OUT CHAR16  *SpeedCountString
  )
{
  if ((SpeedCount > 4)
    || (SpeedCount == 0))
  {
    UnicodeSPrint (SpeedCountString, MAX_PORT_SPEED_LEN, L"");
  }
  else {
    UnicodeSPrint (SpeedCountString, MAX_PORT_SPEED_LEN, L"%dx", SpeedCount);
  }
}

/** Creates quad description string.

   @param[in]   SpeedCount   Number of occurrences of current speed.
   @param[in]   Speed        Current speed.
   @param[out]  String       Pointer to a string to be generated.
   @param[in]   SizeOfString Size of the string.

**/
VOID
CreateQuadDescription (
  IN  UINT8              SpeedCount,
  IN  PORT_OPTION_SPEED  Speed,
  OUT CHAR16             *String,
  IN  UINT8              SizeOfString
  )
{
  CHAR16  CurrentSpeedString[MAX_QUAD_SPEED_LEN];
  CHAR16  SpeedCountString[MAX_SUBPART_LEN];

  if (SpeedCount != 0) {
    ZeroMem (CurrentSpeedString, MAX_QUAD_SPEED_LEN * 2);
    ZeroMem (SpeedCountString, MAX_SUBPART_LEN * 2);

    if (StrLen (String) != 0) {
      // If there is some speed + count defined already add '+' before adding a new speed.
      UnicodeSPrint (
        String + StrLen (String),
        SizeOfString,
        L"+"
        );
    }

    CreateSpeedCountString (SpeedCount, SpeedCountString);
    CreateSpeedString (Speed, CurrentSpeedString);
    UnicodeSPrint (
      String + StrLen (String),
      SizeOfString,
      L"%s%s",
      SpeedCountString,
      CurrentSpeedString
      );
  }
}

/** Creates port option string.

   @param[in]    PortOption            Pointer to a single PORT_OPTION
   @param[out]   PortOptionDataString  Pointer to a string to be generated

   @retval   EFI_SUCCESS             Port option string generated properly.
   @retval   EFI_OUT_OF_RESOURCES    Failed to allocate memory.

**/
EFI_STATUS
CreatePortOptionString (
  IN  PORT_OPTION     *PortOption,
  OUT CHAR16          *PortOptionDataString
  )
{
  UINT8              LogicalPortNum;
  CHAR16             QuadDescription[ICE_QUAD_COUNT][MAX_QUAD_SPEED_LEN];
  CHAR16             ResultString[ICE_QUAD_COUNT * MAX_QUAD_SPEED_LEN];
  UINT8              SamePortSpeedCount = 0;
  PORT_OPTION_SPEED  PortSpeed          = 0;
  PORT_OPTION_SPEED  PreviousSpeed      = PortOptionSpeedNA;
  UINT8              QuadNum            = 0;
  BOOLEAN            Update             = FALSE;

  ASSERT (PortOptionDataString != NULL);
  ASSERT (PortOption != NULL);

  ZeroMem (QuadDescription, ICE_QUAD_COUNT * MAX_QUAD_SPEED_LEN * 2);
  ZeroMem (ResultString, ICE_QUAD_COUNT * MAX_QUAD_SPEED_LEN * 2);


  for (LogicalPortNum = 0; LogicalPortNum < MAX_PMD_COUNT; LogicalPortNum++) {
    if (PortOption->PortOptionInvalid) {
      // no point in checking other parameters when port option is invalid
      break;
    }

    // For logical port 4 increase the number of quads and zero everything.
    if (LogicalPortNum == 4) {
      QuadNum = 1;
      SamePortSpeedCount = 0;
      PreviousSpeed = PortOptionSpeedNA;
    }

    PortSpeed = PortOption->PortSpeed[LogicalPortNum];

    if (PortSpeed != PortOptionSpeedNA) {
      // For logical port 0 and 4 set previous speed.
      if (PreviousSpeed == PortOptionSpeedNA) {
        PreviousSpeed = PortSpeed;
      }

      // If different speeds then update the string.
      if (PortSpeed != PreviousSpeed) {
        Update = TRUE;
      } else {
        // Count ports
        SamePortSpeedCount++;
      }
    }

    // Update the string.
    if (Update) {
      CreateQuadDescription (
        SamePortSpeedCount,
        PreviousSpeed,
        QuadDescription[QuadNum],
        sizeof (QuadDescription[QuadNum])
        );
      SamePortSpeedCount = 1;
      Update = FALSE;
      PreviousSpeed = PortSpeed;
    }

    // If it's the last port of the quad- prepare the string.
    if ((LogicalPortNum == 3) ||
        (LogicalPortNum == 7))
    {
      CreateQuadDescription (
        SamePortSpeedCount,
        PreviousSpeed,
        QuadDescription[QuadNum],
        sizeof (QuadDescription[QuadNum])
        );
    }
  } // End of loop

  if (StrCmp (QuadDescription[0], QuadDescription[1]) == 0) {
    // we've got strings for two quads. Check if they are the same
    if (StrLen (QuadDescription[0]) == 0) {
      // empty string
      UnicodeSPrint (ResultString, sizeof (ResultString), L"");
    } else {
      // this means that there are the same and are not empty so add '2x'
      UnicodeSPrint (ResultString, sizeof (ResultString), L"2x%s", QuadDescription[0]);
    }
  } else { // Quad0 != Quad1
    UnicodeSPrint (ResultString, sizeof (ResultString), L"%s", QuadDescription[0]);
    if (StrLen (QuadDescription[1]) != 0) {
      UnicodeSPrint (ResultString + StrLen (ResultString), sizeof (ResultString), L"-%s", QuadDescription[1]);
    }
  }
  StrCpyS (PortOptionDataString, StrLen (ResultString)+1, ResultString);
  return EFI_SUCCESS;
}

/** Creates Oneof and its options in FORM_PORT_OPTION_CONFIG.

   @param[in]    UndiPrivateData       Pointer to an UNDI_PRIVATE_DATA struct
   @param[in]    OptionStrings         Pointer to port option Strings
   @param[in]    OptCount              Number of port options to generate oneof for
   @param[in]    PortOptionsData       Pointer to Port Options struct

   @retval   EFI_SUCCESS             Port option string generated properly.
   @retval   EFI_ERROR               One of a functions called here failed.
   @retval   EFI_OUT_OF_RESOURCES    Failed to allocate memory

**/
EFI_STATUS
CreatePortOptionsOneOf (
  IN UNDI_PRIVATE_DATA  *UndiPrivateData,
  IN CHAR16             **OptionStrings,
  IN UINT8              OptCount,
  IN PORT_OPTIONS_DATA  *PortOptionsData
  )
{
  EFI_STATUS          Status          = EFI_SUCCESS;
  VOID                *StartHandle    = NULL;
  VOID                *EndHandle      = NULL;
  VOID                *OptionHandle   = NULL;
  VOID                *OneofOpCode    = NULL;
  VOID                *DefaultHandle  = NULL;
  VOID                *DefaultOpcode  = NULL;
  UINT8               DefaultPortOpt;
  UINT8               i;
  PORT_OPTION         *PortOption;

  ASSERT (UndiPrivateData != NULL);
  ASSERT (OptionStrings != NULL);
  ASSERT (OptCount <= MAX_PORT_OPTIONS);

  Status = HiiCreateLabelHandlePair (
             PORT_OPTION_BEGIN_LABEL,
             PORT_OPTION_END_LABEL,
             &StartHandle,
             &EndHandle
             );
  IF_RETURN (EFI_ERROR (Status), Status);

  for (i = 0; i < OptCount; i++) {
    // Check if requested port configuration is supported by this adapter
    // If this is inactive port option then it should be not available as OneOf Option
    PortOption = &PortOptionsData->PortOptions[i];
    if (!PortOption->PortOptionInvalid) {
      Status = HiiBuildOneOfOption (
                 UndiPrivateData,
                 UndiPrivateData->HiiInfo.HiiPkgListHandle,
                 OptionStrings[i],
                 EFI_IFR_NUMERIC_SIZE_1,
                 i,
                 0,
                 &OptionHandle
                 );
      IF_GOTO (EFI_ERROR (Status), ExitCleanPair);
    }
  }

  Status = GetCurrentPortOptionNum (
             UndiPrivateData,
             &DefaultPortOpt
             );
  IF_GOTO (EFI_ERROR (Status), ExitCleanPair);

  DefaultHandle = HiiAllocateOpCodeHandle ();
  if (DefaultHandle == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ExitCleanOption;
  }
  DefaultOpcode = HiiCreateDefaultOpCode (
                    DefaultHandle,
                    EFI_HII_DEFAULT_CLASS_STANDARD,
                    EFI_IFR_NUMERIC_SIZE_1,
                    DefaultPortOpt
                    );

  if (DefaultOpcode == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ExitCleanOption;
  }
  OneofOpCode = HiiCreateOneOfOpCode (
                  StartHandle,
                  QUESTION_ID_PORT_OPTION,                           /* QuestionId */
                  STORAGE_VARIABLE_ID,                               /* VarStoreId */
                  OFFSET_OF (HII_STD_VARSTORE, ActivePortOption),    /* VarOffset */
                  STRING_TOKEN (STR_PORT_OPTION_ONEOF_PROMPT),       /* Prompt */
                  STRING_TOKEN (STR_PORT_OPTION_ONEOF_HELP),         /* Help */
                  EFI_IFR_FLAG_RESET_REQUIRED,                       /* QuestionFlags */
                  EFI_IFR_NUMERIC_SIZE_1,                            /* OneOfFlags */
                  OptionHandle,                                      /* OptionsOpCodeHandle */
                  DefaultHandle                                      /* DefaultsOpCodeHandle */
                  );

  if (OneofOpCode == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ExitCleanOption;
  }

  Status = HiiUpdateForm (
             UndiPrivateData->HiiInfo.HiiPkgListHandle,
             &mHiiFormGuid,
             FORM_PORT_OPTION_CONFIG,
             StartHandle,
             EndHandle
             );

ExitCleanOption:
  if (OptionHandle != NULL) {
    HiiFreeOpCodeHandle (OptionHandle);
  }
  if (DefaultHandle != NULL) {
    HiiFreeOpCodeHandle (DefaultHandle);
  }
ExitCleanPair:
  HiiDestroyLabelHandlePair (StartHandle, EndHandle);
  return Status;
}


/** Creates port option gotos.

   @param[in]    UndiPrivateData       Pointer to an UNDI_PRIVATE_DATA struct
   @param[in]    OptionStrings         Pointer to port option Strings
   @param[in]    OptCount              Number of port options to generate gotos for
   @param[in]    PortOptionsData       Pointer to Port Options struct

   @retval   EFI_SUCCESS             Port option string generated properly.
   @retval   EFI_ERROR               One of a functions called here failed.

**/
EFI_STATUS
CreatePortOptionGotos (
  IN UNDI_PRIVATE_DATA  *UndiPrivateData,
  IN CHAR16             **OptionStrings,
  IN UINT8              OptCount,
  IN PORT_OPTIONS_DATA  *PortOptionsData
  )
{
  VOID         *StartOpCodeHandle;
  VOID         *EndOpCodeHandle;
  EFI_STATUS   Status;
  UINT8        i;
  CHAR16       TempXUefiString[MAX_XUEFI_SPEED_LEN];
  PORT_OPTION  *PortOption;

  Status = HiiCreateLabelHandlePair (
             PORT_OPTION_MENU_BEGIN_LABEL,
             PORT_OPTION_MENU_END_LABEL,
             &StartOpCodeHandle,
             &EndOpCodeHandle
             );
  IF_RETURN (EFI_ERROR (Status), Status);

  for (i = 0; i < OptCount; i++) {
    PortOption = &PortOptionsData->PortOptions[i];
    if (!PortOption->PortOptionInvalid) {
      ZeroMem (TempXUefiString, MAX_XUEFI_SPEED_LEN * 2);
      UnicodeSPrint (TempXUefiString, MAX_XUEFI_SPEED_LEN, L"INTEL_PortOption%dRef", i);
      Status = HiiBuildGoto (
                 UndiPrivateData,
                 UndiPrivateData->HiiInfo.HiiPkgListHandle,
                 StartOpCodeHandle,
                 OptionStrings[i],
                 TempXUefiString,
                 EFI_IFR_TYPE_NUM_SIZE_8,
                 EFI_IFR_FLAG_CALLBACK,
                 FORM_PORT_OPTION_CONFIG_DETAILS,
                 (UINT16) (QUESTION_ID_PORT_OPTION_OPTMENU0 + i)
                 );
      IF_GOTO (EFI_ERROR (Status), ExitCleanPair);
    }
  }
  Status = HiiUpdateForm (
             UndiPrivateData->HiiInfo.HiiPkgListHandle,
             &mHiiFormGuid,
             FORM_PORT_OPTION_CONFIG,
             StartOpCodeHandle,
             EndOpCodeHandle
             );

ExitCleanPair:
  HiiDestroyLabelHandlePair (StartOpCodeHandle, EndOpCodeHandle);
  return Status;
}

/** Returns number of active lanes in a specified quad for specified port option.

   @param[in]    PortOption     Port option for which active lanes count shall be returned
   @param[in]    QuadNum        Quad for wchich active lanes count shall be returned

   @return       Number of active lanes

**/
UINT8
GetActiveLanesCount (
  IN PORT_OPTION    *PortOption,
  IN UINT8          QuadNum
  )
{
  UINT8 ActiveLanesCount = 0;

  ASSERT (PortOption != NULL);
  ASSERT (Quadnum < ICE_QUAD_COUNT);

  for (UINT8 i = QuadNum * 4; i < ((QuadNum * 4) + 4); i++) {
    if (PortOption->PortSpeed[i] != PortOptionSpeedNA) {
      ActiveLanesCount++;
    }
  }

  return ActiveLanesCount;
}

/** Returns varstore offset of OptionName for a port option.

   @param[in]  PortOptionNum     Index of a port option.

   @return     Varstore offset of a OptionName field.

**/
UINTN
GetPortOptionStringVarstoreOffset (
  UINTN     PortOptionNum
  )
{
  UINTN   BaseOffset;
  UINTN   OptionOffset;

  BaseOffset = OFFSET_OF (HII_STD_VARSTORE, PortOptStrings);
  OptionOffset = PortOptionNum * sizeof (PORT_OPTION_DESC);

  return BaseOffset + OptionOffset + OFFSET_OF (PORT_OPTION_DESC, OptionName);
}

/** Returns varstore offset of Quad Description of a quad for a port option.

   @param[in]  PortOptionNum     Index of a port option.
   @param[in]  QuadNum           Index of a quad.

   @return     Varstore offset of a Quad.Desc field

**/
UINTN
GetQuadStringVarstoreOffset (
  UINTN     PortOptionNum,
  UINTN     QuadNum
  )
{
  UINTN   BaseOffset;
  UINTN   OptionOffset;

  BaseOffset = OFFSET_OF (HII_STD_VARSTORE, PortOptStrings);
  OptionOffset = PortOptionNum * sizeof (PORT_OPTION_DESC);

  return BaseOffset + OptionOffset + OFFSET_OF (PORT_OPTION_DESC, Quad[QuadNum].Desc);
}

/** Returns varstore offset of Lane Description of a lane of a quad for a port option.

   @param[in]  PortOptionNum     Index of a port option.
   @param[in]  QuadNum           Index of a quad.
   @param[in]  LaneNum           Index of a lane.

   @return     Varstore offset of a Quad.Lane.Desc field

**/
UINTN
GetLaneStringVarstoreOffset (
  UINTN     PortOptionNum,
  UINTN     QuadNum,
  UINTN     LaneNum
  )
{
  UINTN   BaseOffset;
  UINTN   OptionOffset;

  BaseOffset = OFFSET_OF (HII_STD_VARSTORE, PortOptStrings);
  OptionOffset = PortOptionNum * sizeof (PORT_OPTION_DESC);

  return BaseOffset + OptionOffset + OFFSET_OF (PORT_OPTION_DESC, Quad[QuadNum].Lane[LaneNum].Desc);
}

/** Fill up FORM_PORT_OPTION_CONFIG_FOR_ALL_PORTS.

   @param[in]    UndiPrivateData       Pointer to an UNDI_PRIVATE_DATA struct
   @param[in]    OptCount              Number of port options to generate oneof for
   @param[in]    PortOptionsData       Pointer to Port Options struct

   @retval   EFI_SUCCESS             Port option string generated properly.
   @retval   EFI_ERROR               One of a functions called here failed.

**/
EFI_STATUS
CreateDetailsStringsForAllOptions (
  IN UNDI_PRIVATE_DATA  *UndiPrivateData,
  IN UINT8              OptCount,
  IN PORT_OPTIONS_DATA  *PortOptionsData
)
{
  VOID         *StartOpCodeHandle;
  VOID         *EndOpCodeHandle;
  CHAR16       NameString[MAX_OPT_NAME_LEN];
  CHAR16       XUefiName[MAX_OPT_NAME_LEN];
  EFI_STATUS   Status;
  PORT_OPTION  *PortOption;

  Status = HiiCreateLabelHandlePair (
             PORT_OPTION_DETAILS_BEGIN_LABEL,
             PORT_OPTION_DETAILS_END_LABEL,
             &StartOpCodeHandle,
             &EndOpCodeHandle
             );
  IF_RETURN (EFI_ERROR (Status), Status);

  for (UINT8 PortOptionNum = 0; PortOptionNum < OptCount; PortOptionNum++) {
    PortOption = &PortOptionsData->PortOptions[PortOptionNum];
    if (!PortOption->PortOptionInvalid) {
      // create Port Option description
      UnicodeSPrint (NameString, (MAX_OPT_NAME_LEN), L"Option Name");
      UnicodeSPrint (XUefiName, (MAX_OPT_NAME_LEN), L"INTEL_PortOption%d", PortOptionNum);

      Status = HiiCreateString (
                 UndiPrivateData,
                 UndiPrivateData->HiiInfo.HiiPkgListHandle,
                 StartOpCodeHandle,
                 QUESTION_ID_PORT_OPTION_STRING_OPTION (PortOptionNum),
                 GetPortOptionStringVarstoreOffset (PortOptionNum),
                 NameString,
                 XUefiName,
                 STORAGE_VARIABLE_ID,
                 (MAX_OPT_NAME_LEN * 2)
                 );
      IF_GOTO (EFI_ERROR (Status), ExitCleanPair);

      for (UINT8 QuadNum = 0; QuadNum < ICE_QUAD_COUNT; QuadNum++) {
        // create Quad description
        ZeroMem (NameString, MAX_OPT_NAME_LEN * 2);
        ZeroMem (XUefiName, MAX_OPT_NAME_LEN * 2);
        UnicodeSPrint (NameString, (MAX_OPT_NAME_LEN), L"Quad%d", QuadNum);
        UnicodeSPrint (XUefiName, (MAX_OPT_NAME_LEN), L"INTEL_PortOption%d_Quad%d", PortOptionNum, QuadNum);
        Status = HiiCreateString (
                   UndiPrivateData,
                   UndiPrivateData->HiiInfo.HiiPkgListHandle,
                   StartOpCodeHandle,
                   QUESTION_ID_PORT_OPTION_STRING_QUAD (PortOptionNum, QuadNum),
                   GetQuadStringVarstoreOffset (PortOptionNum, QuadNum),
                   NameString,
                   XUefiName,
                   STORAGE_VARIABLE_ID,
                   (MAX_OPT_NAME_LEN * 2)
                   );
        IF_GOTO (EFI_ERROR (Status), ExitCleanPair);
        for (UINT8 LaneNum = 0; LaneNum < ICE_LANES_PER_QUAD; LaneNum++) {
          // create Lane description
          ZeroMem (NameString, MAX_OPT_NAME_LEN * 2);
          ZeroMem (XUefiName, MAX_OPT_NAME_LEN * 2);
          UnicodeSPrint (NameString, (MAX_OPT_NAME_LEN), L"Lane%d", LaneNum);
          UnicodeSPrint (XUefiName, (MAX_OPT_NAME_LEN), L"INTEL_PortOption%d_Quad%d_Lane%d", PortOptionNum, QuadNum, LaneNum);
          Status = HiiCreateString (
                     UndiPrivateData,
                     UndiPrivateData->HiiInfo.HiiPkgListHandle,
                     StartOpCodeHandle,
                     QUESTION_ID_PORT_OPTION_STRING_LANE (PortOptionNum, (QuadNum * 4) + LaneNum),
                     GetLaneStringVarstoreOffset (PortOptionNum, QuadNum, LaneNum),
                     NameString,
                     XUefiName,
                     STORAGE_VARIABLE_ID,
                     (MAX_OPT_NAME_LEN * 2)
                     );
          IF_GOTO (EFI_ERROR (Status), ExitCleanPair);
        }
      }
    }
  }

  Status = HiiUpdateForm (
             UndiPrivateData->HiiInfo.HiiPkgListHandle,
             &mHiiFormGuid,
             FORM_PORT_OPTION_CONFIG_FOR_ALL_PORTS,
             StartOpCodeHandle,
             EndOpCodeHandle
             );

ExitCleanPair:
  HiiDestroyLabelHandlePair (StartOpCodeHandle, EndOpCodeHandle);
  return Status;
}
/** Creates Port Option description strings for all available port options.

   @param[in]       UndiPrivateData            Pointer to driver data structure
   @param[out]      *PortOptionStrings         Pointer to PORT_OPTION_DESC structure

   @retval          EFI_SUCCESS                 Successfully generated description strings
   @retval          !EFI_SUCCESS                One of the functions called returned an error

**/
EFI_STATUS
GetPortOptStrings (
  IN  UNDI_PRIVATE_DATA      *UndiPrivateData,
  OUT PORT_OPTION_DESC       *PortOptionStrings
)
{
  EFI_STATUS            Status;
  PORT_OPTIONS_DATA     *OptionData;
  PORT_OPTION_DESC      *OptionDesc;

  ASSERT (UndiPrivateData != NULL);
  ASSERT (PortOptionStrings != NULL);

  Status = GetPortOptionsForAllPorts (&UndiPrivateData->NicInfo, &OptionData);
  IF_RETURN (EFI_ERROR (Status), Status);

  for (UINTN i = 0; i < OptionData->PortOptionsCount; i++) {
    OptionDesc = &PortOptionStrings[i];

    Status = BuildPortOptionDescription (
               UndiPrivateData,
               &OptionData->PortOptions[i],
               OptionDesc
               );
    IF_GOTO (EFI_ERROR (Status), Exit);
  }

Exit:
  FreePool (OptionData);
  return EFI_SUCCESS;
}

/** Creates a Port option pending Popup.

   @param[in]    UndiPrivateData       pointer to an UNDI_PRIVATE_DATA struct

**/
VOID
CreatePortOptPendingWarning (
  IN UNDI_PRIVATE_DATA  *UndiPrivateData
  )
{
  EFI_INPUT_KEY Key;

  // CreatePopUp does not automatically wrap lines, it has to be done manually
  CreatePopUp (
    EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE,
    &Key,
    HiiGetString (UndiPrivateData->HiiInfo.HiiPkgListHandle, STRING_TOKEN (STR_PORT_OPTION_PENDING1), NULL),
    HiiGetString (UndiPrivateData->HiiInfo.HiiPkgListHandle, STRING_TOKEN (STR_PORT_OPTION_PENDING2), NULL),
    HiiGetString (UndiPrivateData->HiiInfo.HiiPkgListHandle, STRING_TOKEN (STR_PORT_OPTION_PENDING3), NULL),
    NULL
    );
}

/** Fills a form with strings from PORT_OPTION_DESC struct.

   @param[in]    UndiPrivateData        Pointer to an UNDI_PRIVATE_DATA struct
   @param[in]    StartOpCodeHandle      Pointer to an opcode handle for the strings
   @param[in]    OptionDesc             Structure filled with port option description strings

   @retval       EFI_SUCCESS            Successfully generated description strings.
   @retval       !EFI_SUCCESS           One of a functions called here failed.
   @retval       EFI_OUT_OF_RESOURCES   Failed to allocate memory.

**/
EFI_STATUS
CreatePortOptDescMenu (
  IN UNDI_PRIVATE_DATA  *UndiPrivateData,
  IN VOID               *StartOpCodeHandle,
  IN PORT_OPTION_DESC   *OptionDesc
  )
{
  CHAR16      DisplayString[MAX_OPT_NAME_LEN];
  EFI_STATUS  Status;
  VOID        *TextOpCode;

  ASSERT (UndiPrivateData != NULL);
  ASSERT (StartOpCodeHandle != NULL);
  ASSERT (OptionDesc != NULL);

  // print option name string
  UnicodeSPrint (DisplayString, MAX_OPT_NAME_LEN, L"Option Name" );
  Status = HiiCreateText (
             UndiPrivateData->HiiInfo.HiiPkgListHandle,
             StartOpCodeHandle,
             DisplayString,
             OptionDesc->OptionName
             );
  IF_RETURN (EFI_ERROR (Status), Status);

  for (UINT8 QuadNum = 0; QuadNum < ICE_QUAD_COUNT; QuadNum++) {
     // print separator
    TextOpCode = HiiCreateTextOpCode (
                   StartOpCodeHandle,
                   STRING_TOKEN (STR_INV_EMPTY_STRING),
                   STRING_TOKEN (STR_INV_EMPTY_STRING),
                   STRING_TOKEN (STR_INV_EMPTY_STRING)
                   );
    IF_NULL_RETURN (TextOpCode, EFI_OUT_OF_RESOURCES);

    ZeroMem (DisplayString, MAX_OPT_NAME_LEN * 2);
    UnicodeSPrint (DisplayString, MAX_OPT_NAME_LEN, L"Quad %d", QuadNum);
    // Quad description
    Status = HiiCreateText (
               UndiPrivateData->HiiInfo.HiiPkgListHandle,
               StartOpCodeHandle,
               DisplayString,
               OptionDesc->Quad[QuadNum].Desc
               );
    IF_RETURN (EFI_ERROR (Status), Status);

    for (UINT8 LaneNum = 0; LaneNum < ICE_LANES_PER_QUAD; LaneNum++) {
      ZeroMem (DisplayString, MAX_OPT_NAME_LEN * 2);
      UnicodeSPrint (DisplayString, MAX_OPT_NAME_LEN, L"  Lane %d", LaneNum);
      // Lane description
      Status = HiiCreateText (
                 UndiPrivateData->HiiInfo.HiiPkgListHandle,
                 StartOpCodeHandle,
                 DisplayString,
                 OptionDesc->Quad[QuadNum].Lane[LaneNum].Desc
                 );
      IF_RETURN (EFI_ERROR (Status), Status);
    }
  }
  return EFI_SUCCESS;
}

/** Fill up FORM_PORT_OPTION_CONFIG_DETAILS on callback from FORM_PORT_OPTION_CONFIG.

   @param[in]    UndiPrivateData      Pointer to an UNDI_PRIVATE_DATA struct
   @param[in]    QuestionId           Question ID of a caller

   @retval       EFI_SUCCESS          Port option string generated properly.
   @retval       !EFI_SUCCESS         One of a functions called here failed.

**/
EFI_STATUS
PortOptionsCallback (
  IN UNDI_PRIVATE_DATA  *UndiPrivateData,
  IN EFI_QUESTION_ID    QuestionId
  )
{
  VOID                *StartOpCodeHandle;
  VOID                *EndOpCodeHandle;
  PORT_OPTIONS_DATA   *PortOptionsData;
  EFI_STATUS          Status;
  UINT8               PortOptionNum;
  PORT_OPTION_DESC    OptionDesc;

  Status = HiiCreateLabelHandlePair (
             PORT_OPTION_FORM_ENTRY_LABEL,
             PORT_OPTION_FORM_END_LABEL,
             &StartOpCodeHandle,
             &EndOpCodeHandle
             );
  IF_RETURN (EFI_ERROR (Status), Status);

  PortOptionNum = (UINT8) (QuestionId - QUESTION_ID_PORT_OPTION_OPTMENU0);

  // get port options data
  Status = GetPortOptionsForAllPorts (
             &UndiPrivateData->NicInfo,
             &PortOptionsData
             );
  IF_GOTO (EFI_ERROR (Status), ExitCleanPair);

  // fill PORT_OPTION_DESC with strings
  Status = BuildPortOptionDescription (
             UndiPrivateData,
             &PortOptionsData->PortOptions[PortOptionNum],
             &OptionDesc
             );
  IF_GOTO (EFI_ERROR (Status), ExitCleanPair);

  // fill form with port option description
  Status = CreatePortOptDescMenu (
             UndiPrivateData,
             StartOpCodeHandle,
             &OptionDesc
             );
  IF_GOTO (EFI_ERROR (Status), ExitCleanPair);

  Status = HiiUpdateForm (
             UndiPrivateData->HiiInfo.HiiPkgListHandle,
             &mHiiFormGuid,
             FORM_PORT_OPTION_CONFIG_DETAILS,
             StartOpCodeHandle,
             EndOpCodeHandle
             );

ExitCleanPair:
  if (PortOptionsData != NULL) {
    FreePool (PortOptionsData);
  }
  HiiDestroyLabelHandlePair (StartOpCodeHandle, EndOpCodeHandle);
  return Status;
}

/** Creates Port option menus. Should be called during Hii Initialization.

   @param[in]    UndiPrivateData     Pointer to an UNDI_PRIVATE_DATA struct

   @retval      EFI_SUCCESS          Port option string generated properly.
   @retval      !EFI_SUCCESS         One of a functions called here failed.

**/
EFI_STATUS
CreatePortOptionMenus (
  IN UNDI_PRIVATE_DATA *UndiPrivateData
  )
{
  EFI_STATUS         Status;
  PORT_OPTIONS_DATA  *PortOptionsData;
  UINT8              OptCount;
  UINT8              OptNum;
  UINT8              Index;
  CHAR16             *PortOptionStrings[MAX_PORT_OPTIONS];
  CHAR16             PortOptionString[MAX_PORT_OPT_LEN];

  OptNum = 0; // Initial, displayed option number

  Status = GetPortOptionsForAllPorts (
             &UndiPrivateData->NicInfo,
             &PortOptionsData
             );
  IF_RETURN (EFI_ERROR (Status), Status);

  ZeroMem (PortOptionStrings, sizeof (PortOptionStrings));

  OptCount = PortOptionsData->PortOptionsCount;

  if (OptCount == 0) {
    Status = EFI_UNSUPPORTED;
    goto ExitClearMem;
  }

  for (Index = 0; Index < OptCount; Index++) {
    if (!PortOptionsData->PortOptions[Index].PortOptionInvalid) {
      PortOptionStrings[Index] = AllocateZeroPool (MAX_PORT_OPT_LEN);

      if (PortOptionStrings[Index] == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        goto ExitClearMem;
      }

      Status = CreatePortOptionString (
                 &PortOptionsData->PortOptions[Index],
                 PortOptionString
                 );
      IF_GOTO (EFI_ERROR (Status), ExitClearMem);

      UnicodeSPrint (
        PortOptionStrings[Index],
        MAX_PORT_OPT_LEN,
        L"Option %d: %s",
        OptNum++,
        PortOptionString
        );
    } else {
      PortOptionStrings[Index] = NULL;
    }
  }
  Status = CreatePortOptionsOneOf (UndiPrivateData, PortOptionStrings, OptCount, PortOptionsData);
  IF_GOTO (EFI_ERROR (Status), ExitClearMem);

  Status = CreatePortOptionGotos (UndiPrivateData, PortOptionStrings, OptCount, PortOptionsData);
  IF_GOTO (EFI_ERROR (Status), ExitClearMem);

  Status = CreateDetailsStringsForAllOptions (UndiPrivateData, OptCount, PortOptionsData);

ExitClearMem:
  FreePool (PortOptionsData);
  for (Index = 0; Index < OptCount; Index++) {
    if (PortOptionStrings[Index] != NULL) {
      FreePool (PortOptionStrings[Index]);
    }
  }

  return Status;
}
/** Main function for port options called during Hii init.

   @param[in]    UndiPrivateData      pointer to an UNDI_PRIVATE_DATA struct

   @retval   EFI_SUCCESS             Port option string generated properly.
   @retval   !EFI_SUCCESS            One of a functions called here failed.

**/
EFI_STATUS
CreateDynamicVfrContent (
  IN UNDI_PRIVATE_DATA *UndiPrivateData
  )
{
  EFI_STATUS Status = EFI_SUCCESS;
  if (UndiPrivateData->NicInfo.Function == 0) {
    Status = CreatePortOptionMenus (UndiPrivateData);
  }
  return Status;
}

/** Creates lane description string.

   @param[in]    UndiPrivateData      Pointer to an UNDI_PRIVATE_DATA struct
   @param[in]    Option               Pointer a filled port option struct
   @param[in]    PmdNum               Number of a lane in a quad
   @param[in]    Buffer               Buffer for a string
   @param[in]    BufferLength         Length of a buffer for a string

   @retval       EFI_SUCCESS          Description string generated properly.

**/
EFI_STATUS
BuildLaneString (
  IN  UNDI_PRIVATE_DATA   *UndiPrivateData,
  IN  PORT_OPTION         *Option,
  IN  UINTN               PmdNum,
  OUT CHAR16              *Buffer,
  IN  UINTN               BufferLength
  )
{
  CHAR16              TempString[64];
  PORT_OPTION_SPEED   PortSpeed;
  UINTN               PfNum;

  ASSERT (UndiPrivateData != NULL);
  ASSERT (Buffer != NULL);

  PortSpeed = Option->PortSpeed[PmdNum];
  PfNum     = Option->PfNum[PmdNum];
  ASSERT (PortSpeed != PortOptionSpeedNA);

  CreateSpeedString (PortSpeed, TempString);
    UnicodeSPrint (
      Buffer,
      BufferLength,
      L"%s, PF %d",
      TempString,
      PfNum
      );

  return EFI_SUCCESS;
}

/** Fills PORT_OPTION_DESC structure with description strings.

   @param[in]    UndiPrivateData      Pointer to an UNDI_PRIVATE_DATA struct
   @param[in]    Option               Pointer a filled port option struct
   @param[in]    OptionDesc           PORT_OPTION_DESC struct for the strings

   @retval       EFI_SUCCESS          Description string generated properly.
   @retval      !EFI_SUCCESS          One of a functions called here failed.

**/
EFI_STATUS
BuildPortOptionDescription (
  IN    UNDI_PRIVATE_DATA *UndiPrivateData,
  IN    PORT_OPTION       *Option,
  OUT   PORT_OPTION_DESC  *OptionDesc
  )
{
  EFI_STATUS          Status;
  UINTN               QuadNum;
  UINTN               ActiveLaneCount;
  UINTN               LaneNum;
  PORT_OPTION_SPEED   PortSpeed;
  QUAD_DESC           *QuadDesc;
  LANE_DESC           *LaneDesc;

  ASSERT (Option != NULL);
  ASSERT (OptionDesc != NULL);

  ZeroMem (OptionDesc, sizeof (*OptionDesc));

  // Build Option string
  Status = CreatePortOptionString (Option, OptionDesc->OptionName);
  ASSERT_EFI_ERROR (Status);

  // Build strings for quads and lanes
  for (UINTN PmdNum = 0; PmdNum < MAX_PMD_COUNT; PmdNum++) {
    if ((PmdNum == 0)
      || (PmdNum == 4))
    {
      // Process quad
      if (PmdNum == 0) {
        QuadNum = 0;
      }
      else {
        QuadNum = 1;
      }

      QuadDesc = &OptionDesc->Quad[QuadNum];

      // Get active lanes on a quad
      ActiveLaneCount = GetActiveLanesCount (Option, QuadNum);
      if (ActiveLaneCount == 0) {
        // Inactive quad
        UnicodeSPrint (
          QuadDesc->Desc,
          sizeof (QuadDesc->Desc),
          L"N/A"
          );
      } else if (ActiveLaneCount > 1) {
        // More than 1 lane active on a quad - breakout
        UnicodeSPrint (
          QuadDesc->Desc,
          sizeof (QuadDesc->Desc),
          L"Breakout"
          );
      } else if (ActiveLaneCount == 1) {
        // Exactly one lane on a quad
        // Find the active lane, its speed and mapping
        for (LaneNum = (QuadNum * ICE_LANES_PER_QUAD);
             LaneNum < ((QuadNum * ICE_LANES_PER_QUAD) + ICE_LANES_PER_QUAD);
             LaneNum++
             )
        {
          PortSpeed = Option->PortSpeed[LaneNum];
          if (PortSpeed != PortOptionSpeedNA) {
            break;
          }
        }
        ASSERT (LaneNum < ((QuadNum * ICE_LANES_PER_QUAD) + ICE_LANES_PER_QUAD));

        // Create description string for quad
        BuildLaneString (
          UndiPrivateData,
          Option,
          LaneNum,
          QuadDesc->Desc,
          sizeof (QuadDesc->Desc)
          );
      }
    }

    PortSpeed = Option->PortSpeed[PmdNum];
    LaneDesc = &OptionDesc->Quad[QuadNum].Lane[PmdNum % ICE_LANES_PER_QUAD];

    // Prepare lane string
    if ((PortSpeed != PortOptionSpeedNA) &&
        (ActiveLaneCount > 1))
    {
      BuildLaneString (
        UndiPrivateData,
        Option,
        PmdNum,
        LaneDesc->Desc,
        sizeof (LaneDesc->Desc)
        );
    } else {
      UnicodeSPrint (
        LaneDesc->Desc,
        sizeof (LaneDesc->Desc),
        L"--"
        );
    }
  }

  return EFI_SUCCESS;
}

