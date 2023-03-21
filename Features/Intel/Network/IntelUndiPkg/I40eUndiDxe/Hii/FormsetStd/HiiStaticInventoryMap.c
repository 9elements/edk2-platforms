/**************************************************************************

Copyright (c) 2020 - 2022 Intel Corporation. All rights reserved.

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
#include "Hii/Hii.h"
#include "Hii/HiiSetup.h"
#include "Hii/FormsetStd/HiiCommonDep.h"

#define PARTITION_STR_ENTRIES(PartitionId)                                                                                                                \
  LANG_STR_ID_ENTRY     (STR_PARTITION_ ## PartitionId ## _STATE_PROMPT,             TRUE,  GetDellPartitionIdStrForStrId,      IsPartitioningSupported), \
  LANG_STR_ID_ENTRY     (STR_PARTITION_ ## PartitionId ## _CONFIGURATION_MENU_REF,   TRUE,  GetDellPartitionIdStrForStrId,      IsPartitioningSupported), \
  LANG_STR_ID_ENTRY     (STR_PARTITION_ ## PartitionId ## _CONFIGURATION_MENU,       TRUE,  GetDellPartitionIdStrForStrId,      IsPartitioningSupported), \
  LANG_STR_ID_ENTRY     (STR_PARTITION_ ## PartitionId ## _NIC_MODE_PROMPT,          TRUE,  GetDellPartitionIdStrForStrId,      IsPartitioningSupported), \
  LANG_STR_ID_ENTRY     (STR_PARTITION_ ## PartitionId ## _PCI_VF_ADVERTISED_PROMPT, FALSE, GetDellPartitionIdStrForStrId,      IsPartitioningSupported), \
  LANG_STR_ID_ENTRY     (STR_PARTITION_ ## PartitionId ## _DEVICE_ID_PROMPT,         TRUE,  GetDellPartitionIdStrForStrId,      IsPartitioningSupported), \
  ALL_LANG_ENTRY        (STR_PARTITION_ ## PartitionId ## _DEVICE_ID_TEXT,           FALSE, GetDeviceIdStr,                     IsPartitioningSupported), \
  LANG_STR_ID_ENTRY     (STR_PARTITION_ ## PartitionId ## _PCI_BUS_DEV_FUNC_PROMPT,  TRUE,  GetDellPartitionIdStrForStrId,      IsPartitioningSupported), \
  ALL_LANG_STR_ID_ENTRY (STR_PARTITION_ ## PartitionId ## _PCI_BUS_DEV_FUNC_TEXT,    FALSE, GetDellPartitionPciBdfForStrId,     IsPartitioningSupported), \
  LANG_STR_ID_ENTRY     (STR_PARTITION_ ## PartitionId ## _MAC_ADDR_PROMPT,          TRUE,  GetDellPartitionIdStrForStrId,      IsPartitioningSupported), \
  ALL_LANG_STR_ID_ENTRY (STR_PARTITION_ ## PartitionId ## _MAC_ADDR_TEXT,            FALSE, GetDellPartitionFactoryMacForStrId, IsPartitioningSupported), \
  LANG_STR_ID_ENTRY     (STR_PARTITION_ ## PartitionId ## _VIRT_MAC_ADDR_PROMPT,     TRUE,  GetDellPartitionIdStrForStrId,      IsPartitioningSupported), \
  LANG_STR_ID_ENTRY     (STR_PARTITION_ ## PartitionId ## _PORT_NUMBER,              TRUE,  GetDellPartitionIdStrForStrId,      IsPartitioningSupported), \
  LANG_STR_ID_ENTRY     (STR_PARTITION_ ## PartitionId ## _INSTANCE_NUMBER,          TRUE,  GetDellPartitionIdStrForStrId,      IsPartitioningSupported), \
  LANG_STR_ID_ENTRY     (STR_PARTITION_ ## PartitionId ## _MIN_TX_BW_PROMPT,         TRUE,  GetDellPartitionIdStrForStrId,      IsPartitioningSupported), \
  LANG_STR_ID_ENTRY     (STR_PARTITION_ ## PartitionId ## _MAX_TX_BW_PROMPT,         TRUE,  GetDellPartitionIdStrForStrId,      IsPartitioningSupported)

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
