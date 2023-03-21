/**************************************************************************

Copyright (c) 2014 - 2019, Intel Corporation. All rights reserved.

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

#define SOFTWARE_RESERVED_WORD_2    ICE_SR_NVM_WOL_CFG
#define ICE_WOL_FEATURE_DISABLE         0x01
#define ICE_WOL_FEATURE_ENABLE          0x02
#define ICE_WOL_FEATURE(PF)         (0x0E + (PF)*0x100)

BOOLEAN _WolGetInfoFromEeprom_100G(WOL_ADAPTER_HANDLE_TYPE Handle)
{
  UINT16      WolSupportMask = 0;
  WOL_STATUS  Status;
  UINT8       LanPort;
  UINT8       WolSupport;

  Status = _WolEepromRead16(Handle,
                            SOFTWARE_RESERVED_WORD_2,
                            &WolSupportMask);

  if (Status != WOL_SUCCESS)
  {
#ifndef WOL_HAF
    DEBUGPRINT (WOL, ("Read Eeprom failed \n"));
#endif
    return FALSE;
  }
  else
  {
    LanPort = _WolGetLanPort(Handle);

    /* WolSupportMask: 1b = unsupported, 0b = supported */
    WolSupport = ((~WolSupportMask) & (1 << LanPort));

#ifndef WOL_HAF
    DEBUGPRINT (WOL, ("SW rsv word 2: %x LanPort: %d WolSupport: %d\n", WolSupportMask, LanPort, WolSupport));
#endif
    return (BOOLEAN)WolSupport;
  }
}

WOL_STATUS WolGetWakeOnLanStatus_Ice (
    IN    WOL_ADAPTER_HANDLE_TYPE   Handle,
    OUT   BOOLEAN                   *WolStatus
)
{
  WOL_STATUS Status;
  UINT8 FunctionNumber = _WolGetFunction (Handle);
  UINT16 FieldId = ICE_WOL_FEATURE(FunctionNumber);
  struct ice_aqc_nvm_cfg_data ReadBuffer = {0};
  UINT16 ElemCount = 1;

  Status = _WolReadNvmFeatureConfig (Handle, FieldId, (UINT8 *)&ReadBuffer, sizeof (ReadBuffer), &ElemCount);
  if (Status != WOL_SUCCESS) {
    return Status;
  }

  if (ReadBuffer.field_value == ICE_WOL_FEATURE_ENABLE) {
    *WolStatus = TRUE;
  } else if (ReadBuffer.field_value == ICE_WOL_FEATURE_DISABLE) {
    *WolStatus = FALSE;
  } else {
#ifndef WOL_HAF
    DEBUGPRINT (WOL, ("WOL config is incorrect!\n"));
#endif
    return WOL_FEATURE_NOT_SUPPORTED;
  }

  return WOL_SUCCESS;
}

WOL_STATUS WolEnableWakeOnLan_Ice (
  IN    WOL_ADAPTER_HANDLE_TYPE     Handle,
  IN    BOOLEAN                     Enable
)
{
  WOL_STATUS Status;
  UINT8 FunctionNumber = _WolGetFunction (Handle);
  UINT16 FieldId = ICE_WOL_FEATURE(FunctionNumber);
  UINT16 ElemCount = 1;
  struct ice_aqc_nvm_cfg_data WriteBuffer = {0};

  WriteBuffer.field_id = FieldId;
  WriteBuffer.field_value = (Enable ? ICE_WOL_FEATURE_ENABLE : ICE_WOL_FEATURE_DISABLE);

  Status = _WolWriteNvmFeatureConfig (Handle, (UINT8 *)&WriteBuffer, sizeof(WriteBuffer), ElemCount);
  if (Status != WOL_SUCCESS)
  {
    _WolEepromUpdateChecksum(Handle);
  }

  return _WolEepromUpdateChecksum (Handle);
}
