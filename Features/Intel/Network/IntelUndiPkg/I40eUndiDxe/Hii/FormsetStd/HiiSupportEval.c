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
