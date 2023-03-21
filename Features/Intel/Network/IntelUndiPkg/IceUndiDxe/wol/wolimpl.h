/**************************************************************************

Copyright (c) 2014 - 2020, Intel Corporation. All rights reserved.

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
#ifndef __WOLIMPL_H
#define __WOLIMPL_H

#include <Ice.h>
#include <EepromConfig.h>
#define WOL_ICE   5

#ifndef WOL_HAF
typedef UNDI_PRIVATE_DATA *WOL_ADAPTER_HANDLE_TYPE;
typedef EFI_STATUS WOL_STATUS;

#define WOL_SUCCESS               EFI_SUCCESS
#define WOL_FEATURE_NOT_SUPPORTED EFI_UNSUPPORTED
#define WOL_ERROR                 EFI_DEVICE_ERROR
#else /* HAF */
typedef NAL_ADAPTER_HANDLE const WOL_ADAPTER_HANDLE_TYPE;
typedef HAF_STATUS WOL_STATUS;

#define WOL_SUCCESS               HAF_SUCCESS
#define WOL_FEATURE_NOT_SUPPORTED HAF_FEATURE_NOT_SUPPORTED
#define WOL_ERROR                 HAF_ERROR
#endif


#define WOL_MAC_TYPE                          UINT32
#define WOL_MAC_TYPE_EMPTY                    0x00000000
#define WOL_MAC_TYPE_UNKNOWN                  0xFFFFFFFF
#define WOL_MAKE_MACTYPE(SpeedClass, MacType) ((SpeedClass << 16) | (MacType & 0xFFFF))

typedef struct {
  UINT16 VendorId;
  UINT16 DeviceId;
  UINT16 SubVendorId;
  UINT16 SubDeviceId;
} _WOL_DEVICE_ID_t;

typedef struct {
  UINT16 VendorId;
  UINT16 DeviceId;
  UINT16 SubVendorId;
  UINT16 SubDeviceId;
  UINT8 WolInfo;
} _WOL_DEVICE_INFO_t;

typedef struct {
  WOL_MAC_TYPE const *Family;
  WOL_STATUS (*WolGetOffsetBitmask)(WOL_ADAPTER_HANDLE_TYPE Handle, UINT16 *Offset, UINT16 *Bitmask);
} _WOL_FAMILY_INFO_t;

WOL_MAC_TYPE _WolFindMacType(WOL_MAC_TYPE MacType, WOL_MAC_TYPE *MacTypes);
UINT8 _WolGetLanPort(WOL_ADAPTER_HANDLE_TYPE Handle);
void _WolGetDeviceId(WOL_ADAPTER_HANDLE_TYPE Handle, _WOL_DEVICE_ID_t *DeviceId);
BOOLEAN _WolIsFirstController(WOL_ADAPTER_HANDLE_TYPE Handle);
WOL_MAC_TYPE _WolGetMacType(WOL_ADAPTER_HANDLE_TYPE Handle);
UINT8 _WolGetFunction(WOL_ADAPTER_HANDLE_TYPE Handle);

#ifndef WOL_HAF
WOL_STATUS _WolEepromRead16(WOL_ADAPTER_HANDLE_TYPE Handle, UINT16 Offset, UINT16 *Data);
WOL_STATUS _WolEepromWrite16(WOL_ADAPTER_HANDLE_TYPE Handle, UINT16 Offset, UINT16 Data);
WOL_STATUS _WolEepromUpdateChecksum(WOL_ADAPTER_HANDLE_TYPE Handle);
WOL_STATUS _WolReadNvmFeatureConfig(WOL_ADAPTER_HANDLE_TYPE Handle, UINT32 FcId, UINT8* ConfigData, UINT16 BufferSize, UINT16* ItemCount);
WOL_STATUS _WolWriteNvmFeatureConfig(WOL_ADAPTER_HANDLE_TYPE Handle, UINT8* ConfigData, UINT16 BufferSize, UINT16 ItemCount);
#else /* HAF */
#define _WolEepromRead16 NalReadEeprom16
#define _WolEepromWrite16 NalWriteEeprom16
#define _WolEepromUpdateChecksum NalUpdateEepromChecksum
#define _WolReadNvmFeatureConfig NalReadNvmFeatureConfig
#define _WolWriteNvmFeatureConfig NalWriteNvmFeatureConfig
#endif

#endif /* __WOLIMPL_H */
