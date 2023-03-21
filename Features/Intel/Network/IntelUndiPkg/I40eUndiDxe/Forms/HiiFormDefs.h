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
    { 0xb935c15e, 0x5bf1, 0x42c9, { 0xa3, 0x12, 0x1c, 0xf7, 0x52, 0xdb, 0xf4, 0x05 } }

/* VarStore IDs - 0x0100+ */
#define     STORAGE_VARIABLE_ID                 0x0100

/* Form IDs - 0x0200+ */
#define     FORM_MAIN                                0x0200
#define     FORM_NIC                                 0x0201



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
