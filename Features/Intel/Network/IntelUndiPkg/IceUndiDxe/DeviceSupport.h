/**************************************************************************

Copyright (c) 2015 - 2020, Intel Corporation. All rights reserved.

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
#ifndef DEVICE_SUPPORT_H_
#define DEVICE_SUPPORT_H_

#include "CommonDriver.h"


/* Types */
typedef struct BRAND_STRUCT_S {
  UINT16  VendorId;
  UINT16  SubvendorId;
  UINT16  DeviceId;
  UINT16  SubsystemId;
  CHAR16 *BrandString;
} BRAND_STRUCT;

extern BRAND_STRUCT mBrandingTable[];
extern UINTN        mBrandingTableSize;


/* Defines */
#define INVALID_VENDOR_ID     0xFFFF
#define INVALID_SUBVENDOR_ID  0xFFFF
#define INVALID_DEVICE_ID     0xFFFF
#define INVALID_SUBSYSTEM_ID  0xFFFF
#define WILD_CARD             0x0000



/* Function declarations */

/** Returns pointer to current device's branding string (looks for best match)

   @param[in]   UndiPrivateData   Points to the driver instance private data

   @return   Pointer to current device's branding string
**/
CHAR16*
GetDeviceBrandingString (
  UNDI_PRIVATE_DATA *UndiPrivateData
  );

/** Returns information whether given device ID is supported basing on branding
   table.

   @param[in]   VendorId      Device's vendor ID
   @param[in]   DeviceId      Device's device ID

   @retval   TRUE    Device ID is supported
   @retval   FALSE   Device ID is not supported
**/
BOOLEAN
IsDeviceIdSupported (
  UINT16 VendorId,
  UINT16 DeviceId
  );

/** Seeks for current device's entry in branding table

   @param[in]   UndiPrivateData   Driver private data structure
   @param[in]   ExactMatch        Indicator whether full 4-part device ID match is expected.
                                  If FALSE, function returns best matching device's info.

   @return   Device info structure pointer when match was found, NULL otherwise.
**/
BRAND_STRUCT*
FindDeviceInTable (
  UNDI_PRIVATE_DATA *UndiPrivateData,
  BOOLEAN            ExactMatch
  );

/** Sets adapter support information flags.

   @param[in]   UndiPrivateData   Points to the driver instance private data
**/
VOID
SetStaticAdapterSupportFlags (
  IN  UNDI_PRIVATE_DATA  *UndiPrivateData
  );

/** Sets adapter support information flags.

   @param[in]   UndiPrivateData   Points to the driver instance private data
**/
EFI_STATUS
SetHwDependentAdapterSupportFlags (
  IN  UNDI_PRIVATE_DATA  *UndiPrivateData
  );

#endif /* DEVICE_SUPPORT_H_ */
