/******************************************************************************
**                                                                           **
** INTEL CONFIDENTIAL                                                        **
**                                                                           **
** Copyright (c) 2020 - 2022 Intel Corporation. All rights reserved.         **
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
#include "Hii/Hii.h"
#include "Hii/HiiSetup.h"
#include "Hii/FormsetStd/HiiCommonDep.h"


HII_STATIC_INV_STRING_ENTRY  mHiiHwStaticInvStringMap[] = {


  // ---------------------------  Main HII Configuration Page -----------------------------------------------
  ALL_LANG_ENTRY    (STRING_TOKEN (STR_INV_FORM_SET_TITLE),            FALSE, GetFormSetTitleStr,         NULL),
  LANG_ENTRY        (STRING_TOKEN (STR_INV_FORM_SET_HELP),             FALSE, GetFormSetHelpStr,          NULL),  // language specific
  ALL_LANG_ENTRY    (STRING_TOKEN (STR_MAC_ADDR_TEXT),                 FALSE, GetFactoryMacStr,           NULL),
  ALL_LANG_ENTRY    (STRING_TOKEN (STR_PCI_BUS_DEV_FUNC_TEXT),         FALSE, GetPciBdfStr,               NULL),
  ALL_LANG_ENTRY    (STRING_TOKEN (STR_EFI_DRIVER_VER_TEXT),           FALSE, GetEfiDriverNameAndVerStr,  NULL),
  ALL_LANG_ENTRY    (STRING_TOKEN (STR_DEVICE_NAME_TEXT),              FALSE, GetBrandStr,                NULL),
  ALL_LANG_ENTRY    (STRING_TOKEN (STR_DEVICE_ID_TEXT),                FALSE, GetDeviceIdStr,             NULL),
  ALL_LANG_ENTRY    (STRING_TOKEN (STR_CONTROLER_ID_TEXT),             FALSE, GetChipTypeStr,             NULL),
  ALL_LANG_ENTRY    (STRING_TOKEN (STR_ADAPTER_PBA_TEXT),              FALSE, GetPbaStr,                  NULL),
};

UINTN mHiiHwStaticInvStringMapSize = sizeof (mHiiHwStaticInvStringMap) / sizeof (mHiiHwStaticInvStringMap[0]);
