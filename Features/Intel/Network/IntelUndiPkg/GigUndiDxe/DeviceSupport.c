/**************************************************************************

Copyright (c) 2015 - 2022, Intel Corporation. All rights reserved.

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

#include "DeviceSupport.h"

#if !defined (UNDI_IDPF) && !defined (SWITCH_MODE)
#include "EepromConfig.h"
#endif /* !defined (UNDI_IAVF) && !defined (UNDI_IDPF) && !defined (SWITCH_MODE) */


/** Seeks for current device's entry in branding table

   @param[in]   VendorId      Device's vendor ID
   @param[in]   DeviceId      Device's device ID
   @param[in]   SubvendorId   Device's subvendor ID
   @param[in]   SubdeviceId   Device's subdevice ID
   @param[in]   ExactMatch    Indicator whether full 4-part device ID match is expected.
                              If FALSE, function returns best matching device's info.

   @return   Device info structure pointer when match was found, NULL otherwise.
**/
BRAND_STRUCT*
FindDeviceInTableByIds (
  UINT16  VendorId,
  UINT16  DeviceId,
  UINT16  SubvendorId,
  UINT16  SubdeviceId,
  BOOLEAN ExactMatch
  )
{
  UINTN         i;
  BRAND_STRUCT *Device = NULL;

  if (ExactMatch) {
    for (i = 0; i < mBrandingTableSize; i++) {
      if ((VendorId == mBrandingTable[i].VendorId) &&
        (DeviceId == mBrandingTable[i].DeviceId) &&
        (SubvendorId == mBrandingTable[i].SubvendorId) &&
        (SubdeviceId == mBrandingTable[i].SubsystemId))
      {
        Device = &mBrandingTable[i];
        break;
      }
    }
  } else {
    INTN SubsystemMatch  = -1;
    INTN SubvendorMatch  = -1;
    INTN DeviceMatch = -1;
    INTN VendorMatch = -1;

    for (i = 0; i < mBrandingTableSize; i++) {
      if (VendorId == mBrandingTable[i].VendorId) {
        if (DeviceId == mBrandingTable[i].DeviceId) {
          if (SubvendorId == mBrandingTable[i].SubvendorId) {
            if (SubdeviceId == mBrandingTable[i].SubsystemId) {
              SubsystemMatch = i;
              break;
            } else if (mBrandingTable[i].SubsystemId == WILD_CARD) {
              SubvendorMatch = i;
            }
          } else if (mBrandingTable[i].SubvendorId == WILD_CARD) {
            DeviceMatch = i;
          }
        } else if (mBrandingTable[i].DeviceId == WILD_CARD) {
          VendorMatch = i;
        }
      }
    }
    do {
      if (SubsystemMatch != -1) {
        Device = &mBrandingTable[SubsystemMatch];
        break;
      }
      if (SubvendorMatch != -1) {
        Device = &mBrandingTable[SubvendorMatch];
        break;
      }
      if (DeviceMatch != -1) {
        Device = &mBrandingTable[DeviceMatch];
        break;
      }
      if (VendorMatch != -1) {
        Device = &mBrandingTable[VendorMatch];
        break;
      }
    } while (0);

  }
  return Device;
}

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
  )
{
  SHARED_HW *Nic = &UndiPrivateData->NicInfo.Hw;

  return FindDeviceInTableByIds (
           Nic->vendor_id,
           Nic->device_id,
           Nic->subsystem_vendor_id,
           Nic->subsystem_device_id,
           ExactMatch
         );
}


/** Returns pointer to current device's branding string (looks for best match)

   @param[in]   UndiPrivateData   Points to the driver instance private data

   @return   Pointer to current device's branding string
**/
CHAR16*
GetDeviceBrandingString (
  UNDI_PRIVATE_DATA *UndiPrivateData
  )
{
  BRAND_STRUCT *Device = NULL;
  CHAR16 *      BrandingString = NULL;


  // It will return last, INVALID entry at least
  Device = FindDeviceInTable (UndiPrivateData, FALSE);
  if (Device != NULL) {
    BrandingString = Device->BrandString;
  }

  return BrandingString;
}

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
  )
{
  UINTN i;

  if (VendorId == INTEL_VENDOR_ID) {
    for (i = 0; i < mBrandingTableSize; i++) {
      if (mBrandingTable[i].DeviceId != INVALID_DEVICE_ID) {
        if (DeviceId == mBrandingTable[i].DeviceId) {
          return TRUE;
        }
      }
    }
  }
  return FALSE;
}

/** Sets adapter support information flags.

   @param[in]   UndiPrivateData   Points to the driver instance private data
**/
VOID
SetStaticAdapterSupportFlags (
  IN  UNDI_PRIVATE_DATA  *UndiPrivateData
  )
{
}

