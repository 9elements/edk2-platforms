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
#include <wol.h>

WOL_STATUS
_WolGetOffsetBitmask_IXGBE (
  IN    WOL_ADAPTER_HANDLE_TYPE     Handle,
  OUT   UINT16                      *Offset,
  OUT   UINT16                      *Bitmask
  )
{
  UINT16  LanPort = 0;
  LanPort = _WolGetLanPort(Handle);

  switch (LanPort)
  {
    case 0:
      /* EEPROM Control Word 3 */
      *Offset = 0x0038;
      /* APM Enable Port 0     */
      *Bitmask = 0x0001;
      return WOL_SUCCESS;
    case 1:
      /* EEPROM Control Word 3 */
      *Offset = 0x0038;
      /* APM Enable Port 1     */
      *Bitmask = 0x0002;
      return WOL_SUCCESS;
  }
  return WOL_FEATURE_NOT_SUPPORTED;
}

BOOLEAN _WolGetInfoFromEeprom_10G(WOL_ADAPTER_HANDLE_TYPE Handle)
{
  UINT16 Caps;
  _WolEepromRead16(Handle, 0x2C, &Caps);
  switch (Caps & 0x000C) {      /* Get WOL bits                     */
    case 0x0004:                /* WOL supported on both ports      */
      return TRUE;
    case 0x0008:                /* WOL supported on the first port  */
      return _WolGetFunction(Handle) == 0;
  }
  return FALSE;
}

WOL_STATUS WolSetApmRegister_10G (WOL_ADAPTER_HANDLE_TYPE Handle, BOOLEAN Enable)
{
  UINT32        Address = 0;
  UINT32        ReadWord = 0;
  BOOLEAN       IsCurrentlySet;

  if (Handle == NULL) {
    return WOL_FEATURE_NOT_SUPPORTED;
  }

  if (_WolGetMacType (Handle) < WOL_MAKE_MACTYPE (WOL_10G, ixgbe_mac_X540)) {
    Address = IXGBE_GRC;
  } else {
#ifndef WOL_HAF
    Address = IXGBE_GRC_BY_MAC (&Handle->NicInfo.Hw);
#else /* WOL_HAF */
    Address = IXGBE_GRC_BY_MAC (IXGBE_HW_PTR (Handle));
#endif /* !WOL_HAF */
  }

#ifndef WOL_HAF
  ReadWord = IXGBE_READ_REG (&Handle->NicInfo.Hw, Address);
#else /* WOL_HAF */
  if (NAL_SUCCESS != NalReadMacRegister32 (Handle, Address, &ReadWord)) {
    return WOL_ERROR;
  }
#endif /* !WOL_HAF */

  /* If set to 10b, APM Wakeup is enabled */
  IsCurrentlySet = (ReadWord & IXGBE_GRC_APME) != 0;

  if (IsCurrentlySet != Enable) {
    ReadWord ^= IXGBE_GRC_APME;
#ifndef WOL_HAF
    IXGBE_WRITE_REG (&Handle->NicInfo.Hw, Address, ReadWord);
#else /* WOL_HAF */
    if (NAL_SUCCESS != NalWriteMacRegister32 (Handle, Address, ReadWord)) {
      return WOL_ERROR;
    }
#endif /* !WOL_HAF */
  }

  return WOL_SUCCESS;
}

