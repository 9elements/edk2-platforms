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
#include "Hii/Hii.h"
#include "DeviceSupport.h"
#include "EepromConfig.h"

#include "wol.h"


/** Gets default WOL status.

  @param[in]   UndiPrivateData       Pointer to driver private data structure
  @param[out]  WolStatus             Default WOL status

  @retval     EFI_SUCCESS            Operation successful
**/
EFI_STATUS
GetDefaultWolStatus (
  IN   UNDI_PRIVATE_DATA   *UndiPrivateData,
  OUT  ADAPTER_WOL_STATUS  *WolStatus
  )
{
  if (WolIsWakeOnLanSupported (UndiPrivateData)) {
    *WolStatus = WOL_ENABLE;
  } else {
    *WolStatus = WOL_NA;
  }

  return EFI_SUCCESS;
}

/** Gets Alternate MAC address attribute.

  @param[in]   UndiPrivateData       Pointer to driver private data structure
  @param[out]  AltMacAddrUni         Resultant MAC address string

  @retval     EFI_SUCCESS            Operation successful
  @retval     !EFI_SUCCESS           Failed to get factory or alternate MAC address
**/
EFI_STATUS
GetAltMacAddr (
  IN   UNDI_PRIVATE_DATA  *UndiPrivateData,
  OUT  UINT16             *AltMacAddrUni
  )
{
  UINT8       FactoryMac[ETH_ALEN];
  UINT8       AlternateMac[ETH_ALEN];
  EFI_STATUS  Status;

  Status = GetFactoryMacAddress (UndiPrivateData, FactoryMac);
  IF_RETURN (EFI_ERROR (Status), Status);
  Status = GetAlternateMacAddress (UndiPrivateData, AlternateMac);
  IF_RETURN (EFI_ERROR (Status), Status);

  if (CompareMem (FactoryMac, AlternateMac, ETH_ALEN) == 0) {
    ZeroMem (AlternateMac, ETH_ALEN);
  }

  SET_UNI_MAC_FROM_BIN (AltMacAddrUni, AlternateMac);
  return EFI_SUCCESS;
}

