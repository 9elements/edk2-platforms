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
#ifndef HII_FORM_DEFS_H_
#define HII_FORM_DEFS_H_

#include "Hii/CfgAccessProt/HiiVarStoreFieldSupport.h"

/* This file contains type/value definitions shared between HII code and VFR forms.
   Not all C language constructs can be used due to that (VFR compiler limitations).
*/

  #define HII_FORM_GUID \
    { 0xfe3423f7, 0xd04f, 0x4cde, { 0x93, 0x08, 0x74, 0x1f, 0x3c, 0x93, 0x4a, 0xbf } }

/* VarStore IDs - 0x0100+ */
#define     STORAGE_VARIABLE_ID                 0x0100

/* Form IDs - 0x0200+ */
#define     FORM_MAIN                                0x0200
#define     FORM_NIC                                 0x0201
  #define   FORM_DEVICE_LEVEL_CONFIG                 0x0207

#define     FORM_PORT_OPTION_CONFIG                  0x0212
#define     FORM_PORT_OPTION_CONFIG_DETAILS          0x0213
#define     FORM_PORT_OPTION_CONFIG_FOR_ALL_PORTS    0x0214


/* Question IDs - 0x1000+ */
#define     QUESTION_ID_NIC_CONFIG_MENU                         0x1000
#define     QUESTION_ID_EFI_DRIVER_VER                          0x1001
#define     QUESTION_ID_ADAPTER_PBA                             0x1002
#define     QUESTION_ID_CONTROLER_ID                            0x1003
#define     QUESTION_ID_PCI_BUS_DEV_FUNC                        0x1004
#define     QUESTION_ID_LINK_STATUS                             0x1005
#define     QUESTION_ID_MAC_ADDR                                0x1006
#define     QUESTION_ID_ALT_MAC_ADDR                            0x1007
#define     QUESTION_ID_LINK_SPEED                              0x1008
#define     QUESTION_ID_WOL                                     0x1009
#define     QUESTION_ID_BLINK_LED                               0x100A
#define     QUESTION_ID_DEVICE_ID                               0x100B
#define     QUESTION_ID_DEVICE_NAME                             0x100C
#define     QUESTION_ID_DEFAULT_WOL                             0x100D
#define     QUESTION_ID_LLDP_AGENT                              0x100E
#define     QUESTION_ID_LLDP_AGENT_DEAULT                       0x100F
  #define   QUESTION_ID_DEVICE_LEVEL_CONFIG_MENU                0x1100

  /* Port option QuestionIDs and labels - 0x6200+ */
  #define   QUESTION_ID_PORT_OPTION                           0x6200
  #define   QUESTION_ID_PORT_OPTION_CONFIG_MENU               0x6201
  #define   QUESTION_ID_PORT_OPTION_DETAILS_MENU              0x6202
  #define   QUESTION_ID_PORT_OPTION_OPTMENU0                  0x6203
  #define   QUESTION_ID_PORT_OPTION_OPTMENU1                  0x6204
  #define   QUESTION_ID_PORT_OPTION_OPTMENU2                  0x6205
  #define   QUESTION_ID_PORT_OPTION_OPTMENU3                  0x6206
  #define   QUESTION_ID_PORT_OPTION_OPTMENU4                  0x6207
  #define   QUESTION_ID_PORT_OPTION_OPTMENU5                  0x6208
  #define   QUESTION_ID_PORT_OPTION_OPTMENU6                  0x6209
  #define   QUESTION_ID_PORT_OPTION_OPTMENU7                  0x620A
  #define   QUESTION_ID_PORT_OPTION_OPTMENU8                  0x620B
  #define   QUESTION_ID_PORT_OPTION_OPTMENU9                  0x620C
  #define   QUESTION_ID_PORT_OPTION_OPTMENU10                 0x620D
  #define   QUESTION_ID_PORT_OPTION_OPTMENU11                 0x620E
  #define   QUESTION_ID_PORT_OPTION_OPTMENU12                 0x620F
  #define   QUESTION_ID_PORT_OPTION_OPTMENU13                 0x6210
  #define   QUESTION_ID_PORT_OPTION_OPTMENU14                 0x6211
  #define   QUESTION_ID_PORT_OPTION_OPTMENU15                 0x6212
  #define   PORT_OPTION_BEGIN_LABEL                           0x6213
  #define   PORT_OPTION_END_LABEL                             0x6214
  #define   PORT_OPTION_MENU_BEGIN_LABEL                      0x6215
  #define   PORT_OPTION_MENU_END_LABEL                        0x6216
  #define   PORT_OPTION_FORM_ENTRY_LABEL                      0x6217
  #define   PORT_OPTION_FORM_END_LABEL                        0x6218
  #define   PORT_OPTION_DETAILS_BEGIN_LABEL                   0x6219
  #define   PORT_OPTION_DETAILS_END_LABEL                     0x621A

  #define QUESTION_ID_PORT_OPTION_STRINGS_BASE                0x6220
  /* 176 Question IDs are reserved for port option strings (11 strings * 16 options).
     Next available Question ID is at 0x62D1.
  */
  #define PORT_OPTION_QID_STRING_OPTION_OFFSET                  0
  #define PORT_OPTION_QID_STRING_QUAD0_OFFSET                   1
  #define PORT_OPTION_QID_STRING_QUAD1_OFFSET                   2
  #define PORT_OPTION_QID_STRING_LANE0_OFFSET                   3
  #define PORT_OPTION_QID_STRING_LANE1_OFFSET                   4
  #define PORT_OPTION_QID_STRING_LANE2_OFFSET                   5
  #define PORT_OPTION_QID_STRING_LANE3_OFFSET                   6
  #define PORT_OPTION_QID_STRING_LANE4_OFFSET                   7
  #define PORT_OPTION_QID_STRING_LANE5_OFFSET                   8
  #define PORT_OPTION_QID_STRING_LANE6_OFFSET                   9
  #define PORT_OPTION_QID_STRING_LANE7_OFFSET                   10
  #define PORT_OPTION_QID_SPAN                                  (PORT_OPTION_QID_STRING_LANE7_OFFSET + 1)

  #define QUESTION_ID_PORT_OPTION_STRING_OPTION(o) \
    (QUESTION_ID_PORT_OPTION_STRINGS_BASE + (PORT_OPTION_QID_SPAN * (o)))

  #define QUESTION_ID_PORT_OPTION_STRING_QUAD(o, q) \
    (QUESTION_ID_PORT_OPTION_STRING_OPTION (o) + PORT_OPTION_QID_STRING_QUAD0_OFFSET + (q))

  #define QUESTION_ID_PORT_OPTION_STRING_LANE(o, l) \
    (QUESTION_ID_PORT_OPTION_STRING_OPTION (o) + PORT_OPTION_QID_STRING_LANE0_OFFSET + (l))

  #define QUESTION_ID_ALL_PORTS_FORM                              0x7000

  #define QUESTION_ID_SIOV                                        0x7100
  #define QUESTION_ID_DEFAULT_SIOV                                0x7101
  #define QUESTION_ID_TX_BALANCING                                0x7102
  #define QUESTION_ID_DEFAULT_TX_BALANCING                        0x7103

/* Values used to fill formset variables */

/* Those are used in fields representing BOOLEAN values */
#define DISABLED 0x0
#define ENABLED  0x1


#define LINK_SPEED_AUTO_NEG                   0x00
#define LINK_SPEED_10HALF                     0x01
#define LINK_SPEED_10FULL                     0x02
#define LINK_SPEED_100HALF                    0x03
#define LINK_SPEED_100FULL                    0x04
#define LINK_SPEED_1000HALF                   0x05
#define LINK_SPEED_1000FULL                   0x06
#define LINK_SPEED_2500                       0x07
#define LINK_SPEED_5000                       0x08
#define LINK_SPEED_10000HALF                  0x09
#define LINK_SPEED_10000FULL                  0x0A
#define LINK_SPEED_20000                      0x0B
#define LINK_SPEED_25000                      0x0C
#define LINK_SPEED_40000                      0x0D
#define LINK_SPEED_50000                      0x0E
#define LINK_SPEED_100000                     0x0F
#define LINK_SPEED_NO_CONFIGURE_AUTO          0x10
#define LINK_SPEED_UNKNOWN                    0x20

#define WOL_DISABLE                           0x00
#define WOL_ENABLE                            0x01
#define WOL_NA                                0x02






/** Checks if flag under support index indicates support. To use within VFR files.

  @param[in]  SupportIndex   Index in SupportTable of specific varstore
**/
#define SUPPORTED(SupportIndex)     ideqval NicCfgData.Support[SupportIndex] == MODIFIABLE

/** Checks if flag under support index indicates lack of support. To use within VFR files.

  @param[in]  SupportIndex   Index in SupportTable of specific varstore
**/
#define NOT_SUPPORTED(SupportIndex) ideqval NicCfgData.Support[SupportIndex] == SUPPRESS

/** Checks if flag under support index indicates R/O support. To use within VFR files.

  @param[in]  SupportIndex   Index in SupportTable of specific varstore
**/
#define RD_ONLY(SupportIndex)       ideqval NicCfgData.Support[SupportIndex] == GRAYOUT

#endif /* HII_FORM_DEFS_H_ */
