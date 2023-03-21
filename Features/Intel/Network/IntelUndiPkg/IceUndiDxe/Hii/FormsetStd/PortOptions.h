/**************************************************************************

Copyright (c) 2021, Intel Corporation. All rights reserved.

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
#ifndef PORT_OPTIONS_H_
#define PORT_OPTIONS_H_
#include "Ice.h"
#include "LinkTopology.h"
#include "FormsetStd/HiiConfigData.h"
#include "Hii/Hii.h"

#define MAX_QUAD_SPEED_LEN        100
#define MAX_PORT_SPEED_LEN        50
#define MAX_PORT_OPT_LEN          200
#define MAX_QUAD_SPEED_SIZE       100
#define MAX_XUEFI_SPEED_LEN       100
#define MAX_SUBPART_LEN           20
#define MAX_PMD_COUNT             8
#define QUESTION_IDS_PER_PORT_OPT 11

/** Creates Port option menus.

   @param[in]    UndiPrivateData      pointer to an UNDI_PRIVATE_DATA struct

   @retval   EFI_SUCCESS             Port option string generated properly.
   @retval   EFI_ERROR               One of a functions called here failed.

**/
EFI_STATUS
CreatePortOptionMenus (
  IN UNDI_PRIVATE_DATA *UndiPrivateData
  );

/** Main function for port options called during Hii init.

   @param[in]    UndiPrivateData      pointer to an UNDI_PRIVATE_DATA struct

   @retval   EFI_SUCCESS             Port option string generated properly.
   @retval   EFI_ERROR               One of a functions called here failed.

**/
EFI_STATUS
CreateDynamicVfrContent (
  IN UNDI_PRIVATE_DATA *UndiPrivateData
  );

/** Creates Port Option description strings for all available port options.

   @param[in]       UndiPrivateData            Pointer to driver data structure
   @param[out]      *PortOptionStrings         Pointer to PORT_OPTION_DESC structure

   @retval          EFI_SUCCESS                 Successfully returned port options data
   @retval          EFI_DEVICE_ERROR            AQ command failed
   @retval          EFI_INVALID_PARAMETER       AdapterInfo or ActivePortOption is NULL
   @retval          !EFI_SUCCESS                One of the functions called returned an error
**/
EFI_STATUS
GetPortOptStrings (
  IN  UNDI_PRIVATE_DATA      *UndiPrivateData,
  OUT PORT_OPTION_DESC       *PortOptionStrings
  );

/** Fill up FORM_PORT_OPTION_CONFIG_DETAILS on callback from FORM_PORT_OPTION_CONFIG.

   @param[in]    UndiPrivateData      pointer to an UNDI_PRIVATE_DATA struct
   @param[in]    QuestionId           Target question id

   @retval   EFI_SUCCESS             Port option string generated properly.
   @retval   EFI_ERROR               One of a functions called here failed.

**/
EFI_STATUS
PortOptionsCallback (
  IN UNDI_PRIVATE_DATA *UndiPrivateData,
  IN EFI_QUESTION_ID   QuestionId
  );

/** Creates a Port option pending Popup.

   @param[in]    UndiPrivateData       pointer to an UNDI_PRIVATE_DATA struct

**/
VOID
CreatePortOptPendingWarning (
  IN UNDI_PRIVATE_DATA *UndiPrivateData
  );

/** Fills PORT_OPTION_DESC structure with description strings.

   @param[in]    UndiPrivateData      Pointer to an UNDI_PRIVATE_DATA struct
   @param[in]    Option               Pointer a filled port option struct
   @param[out]   OptionDesc           PORT_OPTION_DESC struct for the strings

   @retval       EFI_SUCCESS          Description string generated properly.
   @retval       !EFI_SUCCESS         One of a functions called here failed.

**/
EFI_STATUS
BuildPortOptionDescription (
  IN    UNDI_PRIVATE_DATA *UndiPrivateData,
  IN    PORT_OPTION       *Option,
  OUT   PORT_OPTION_DESC  *OptionDesc
  );

#endif /* PORT_OPTIONS_H_ */
