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

#ifndef HII_CONFIG_DATA_H_
#define HII_CONFIG_DATA_H_

  #define HII_DATA_GUID \
    { 0xa31abb16, 0xc627, 0x475b, { 0x98, 0x8e, 0x7e, 0xe0, 0x77, 0x67, 0x40, 0xf3 } }

#define  UNI_MAC_CHAR_COUNT  18 /* (12 * hex) + (5 * ":") + NULL terminator */

/* FIELD_SUPPORT Support[] array indexes  - *MUST BE* consecutive numbers, cannot be declared
   as enum because it is unsupported by VFR compiler.
*/
#define  LEGACY_BOOT_PROT  0
#define  LINK_SPEED        1
#define  PXE_VLAN          2
#define  LLDP_AGENT        3
#define  LINK_SPEED_STATUS 4
#define  ALT_MAC           6
#define  VIS_IDX_NUM       28    // *!! MUST BE !!* equal to last #define above + 1 == No. of Support indexes

#define  VIS_NO_EVAL       0xFFFFFFFF // indicates there's no Support Flag index associated with the field



#pragma pack(2)
typedef struct HII_STD_VARSTORE_S {
  // ---------------------------  <"NIC Configuration"> menu -------------------------------------
  UINT8   LinkSpeed;
  UINT8   WolStatus;
  UINT8   DefaultWolStatus;


  // ---------------------------  Main HII menu -----------------------------------------------
  UINT16  BlinkLed;
  UINT8   LinkStatus;
  UINT16  AltMacAddr[UNI_MAC_CHAR_COUNT];
  FIELD_SUPPORT  Support[VIS_IDX_NUM];
} HII_STD_VARSTORE;
#pragma pack()

#endif /* HII_CONFIG_DATA_H_ */
