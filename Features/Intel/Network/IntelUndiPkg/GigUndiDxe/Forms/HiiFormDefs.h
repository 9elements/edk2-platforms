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
#ifndef HII_FORM_DEFS_H_
#define HII_FORM_DEFS_H_

#include "Hii/CfgAccessProt/HiiVarStoreFieldSupport.h"

/* This file contains type/value definitions shared between HII code and VFR forms.
   Not all C language constructs can be used due to that (VFR compiler limitations).
*/

  #define HII_FORM_GUID \
    { 0x77f2ea2f, 0x4312, 0x4569, { 0x85, 0xc4, 0x58, 0x3a, 0xcd, 0x8d, 0xb7, 0xe2 } }

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
