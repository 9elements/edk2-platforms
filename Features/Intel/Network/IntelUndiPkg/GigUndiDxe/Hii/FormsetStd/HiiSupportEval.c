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
#include "Hii/FormsetStd/HiiCommonDep.h"





/** Checks if Alternate MAC address attribute is supported.

  @param[in]   UndiPrivateData  Pointer to driver private data structure
  @param[out]  Supported        BOOLEAN support information

  @retval    EFI_SUCCESS        Operation successful
**/
EFI_STATUS
IsAltMacAddrSupported (
  IN   UNDI_PRIVATE_DATA  *UndiPrivateData,
  OUT  BOOLEAN            *Supported
  )
{
  *Supported = UndiPrivateData->HiiInfo.AltMacAddrSupported;
  return EFI_SUCCESS;
}






/** Calculates value of support flags that are not tied to specific field in standard formset
  varstore configuration map (e.g. specify form visibility, not field).

  @param[in]   UndiPrivateData  Pointer to driver private data structure
  @param[out]  SupportTable     Pointer to support table in varstore buffer

  @retval    EFI_SUCCESS        Operation successful
  @retval    !EFI_SUCCESS       Failed to calculate support value for specific fields
**/
EFI_STATUS
EvaluateUnaffiliatedSupportFlags (
  IN  UNDI_PRIVATE_DATA  *UndiPrivateData,
  OUT FIELD_SUPPORT      *SupportTable
  )
{
  return EFI_SUCCESS;
}

/** Reads and sets support information that is static & HW dependent. Called during HII setup
   before inventory initialization.

   @param[in,out]   UndiPrivateData   Points to the driver instance private data

  @retval    EFI_SUCCESS        Operation successful
  @retval    !EFI_SUCCESS       Failure of underlying function
**/
EFI_STATUS
SetHwDependentAdapterSupportFlags (
  IN OUT  UNDI_PRIVATE_DATA  *UndiPrivateData
  )
{
  EFI_STATUS  Status;
  HII_INFO    *HiiInfo;

  HiiInfo = &UndiPrivateData->HiiInfo;




  Status = GetAltMacAddressSupport (
             UndiPrivateData,
             &HiiInfo->AltMacAddrSupported
             );
  IF_RETURN (EFI_ERROR (Status), Status);

  return EFI_SUCCESS;
}
