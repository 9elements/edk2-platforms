/**************************************************************************

Copyright (c) 2014 - 2022, Intel Corporation. All rights reserved.

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
#include <wol.h>

#ifndef WOL_HAF
  void _WolGetDeviceId(WOL_ADAPTER_HANDLE_TYPE Handle, _WOL_DEVICE_ID_t *DeviceId)
  {
    DeviceId->VendorId = Handle->NicInfo.Hw.vendor_id;
    DeviceId->DeviceId = Handle->NicInfo.Hw.device_id;
    DeviceId->SubVendorId = Handle->NicInfo.Hw.subsystem_vendor_id;
    DeviceId->SubDeviceId = Handle->NicInfo.Hw.subsystem_device_id;
  }
#else /* HAF */
  void _WolGetDeviceId(WOL_ADAPTER_HANDLE_TYPE Handle, _WOL_DEVICE_ID_t *DeviceId)
  {
    NAL_ADAPTER_VENDOR_INFO VendorInfo;


    if (NalGetVendorInformation(Handle, &VendorInfo) == NAL_SUCCESS) {
      DeviceId->VendorId = VendorInfo.Vendor;
      DeviceId->DeviceId = VendorInfo.Device;
      DeviceId->SubVendorId = VendorInfo.SubVendor;
      DeviceId->SubDeviceId = VendorInfo.SubDevice;
    } else {
      DeviceId->VendorId = 0;
      DeviceId->DeviceId = 0;
      DeviceId->SubVendorId = 0;
      DeviceId->SubDeviceId = 0;
    }
  }
#endif

  UINT8 _WolGetLanPort(WOL_ADAPTER_HANDLE_TYPE Handle)
  {
    return Handle->NicInfo.PhysicalPortNumber;
  }

  BOOLEAN _WolIsFirstController(WOL_ADAPTER_HANDLE_TYPE Handle)
  {
    return TRUE;
  }

  WOL_MAC_TYPE _WolGetMacType(WOL_ADAPTER_HANDLE_TYPE Handle)
  {
    return WOL_MAKE_MACTYPE(WOL_40G, Handle->NicInfo.Hw.mac.type);
  }

  WOL_STATUS _WolEepromRead16(WOL_ADAPTER_HANDLE_TYPE Handle, UINT16 Offset, UINT16 *Data)
  {
    if (i40e_read_nvm_word(&Handle->NicInfo.Hw, Offset, Data) == I40E_SUCCESS) {
      return EFI_SUCCESS;
    } else {
      return EFI_DEVICE_ERROR;
    }
  }

  WOL_STATUS _WolEepromWrite16(WOL_ADAPTER_HANDLE_TYPE Handle, UINT16 Offset, UINT16 Data)
  {
    if (I40eWriteNvmBufferExt(&Handle->NicInfo, Offset, 1, &Data) == I40E_SUCCESS) {
      return EFI_SUCCESS;
    } else {
      return EFI_DEVICE_ERROR;
    }
  }

  WOL_STATUS _WolEepromUpdateChecksum(WOL_ADAPTER_HANDLE_TYPE Handle)
  {
    return UpdateNvmChecksum(Handle);
  }


  UINT8 _WolGetFunction(WOL_ADAPTER_HANDLE_TYPE Handle)
  {
    return Handle->NicInfo.Hw.pf_id;
  }

WOL_STATUS _WolSetApmRegister (WOL_ADAPTER_HANDLE_TYPE Handle, BOOLEAN Enable, UINT32 PfpmApmCtrlReg)
{
#define PFPM_APM_APME_BIT   1 << 0
  UINT8       FunctionNumber;
  UINT32      Address = 0;
  UINT32      ReadWord = 0;
  BOOLEAN     IsCurrentlySet;

  if (Handle == NULL)
  {
    return WOL_FEATURE_NOT_SUPPORTED;
  }

  FunctionNumber = _WolGetFunction (Handle);

  /* Create offset based on _PF */
  Address = PfpmApmCtrlReg + 4 * FunctionNumber;

#ifndef WOL_HAF
  ReadWord = rd32 (&Handle->NicInfo.Hw, Address);
#else /* WOL_HAF */
  if (NAL_SUCCESS != NalReadMacRegister32 (Handle, Address, &ReadWord)) {
    return WOL_ERROR;
  }
#endif /* WOL_HAF */

  /* If set to 1b, APM Wakeup is enabled */
  IsCurrentlySet = (ReadWord & PFPM_APM_APME_BIT) != 0;

  if (IsCurrentlySet != Enable)
  {
    ReadWord ^= PFPM_APM_APME_BIT;
#ifndef WOL_HAF
    wr32 (&Handle->NicInfo.Hw, Address, ReadWord);
#else /* WOL_HAF */
    if (NAL_SUCCESS != NalWriteMacRegister32 (Handle, Address, ReadWord)) {
      return WOL_ERROR;
    }
#endif /* WOL_HAF */
  }

  return WOL_SUCCESS;
}

  WOL_STATUS WolSetApmRegister (WOL_ADAPTER_HANDLE_TYPE Handle, BOOLEAN Enable)
  {
    return _WolSetApmRegister (Handle, Enable, I40E_PFPM_APM (0));
  }
