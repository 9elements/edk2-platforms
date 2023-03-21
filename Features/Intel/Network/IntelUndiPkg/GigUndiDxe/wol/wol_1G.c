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
_WolGetOffsetBitmask_CORDOVA (
  IN    WOL_ADAPTER_HANDLE_TYPE     Handle,
  OUT   UINT16                      *Offset,
  OUT   UINT16                      *Bitmask
  )
{
  UINT16  LanPort = 0;
  LanPort = _WolGetLanPort(Handle);

  if (LanPort == 0) {
    /* Initialization Control Word 2        */
    *Offset = 0x000F;
    /* APM Enable                           */
    *Bitmask = 0x0004;
    return WOL_SUCCESS;
  }
  return WOL_FEATURE_NOT_SUPPORTED;
}

WOL_STATUS
_WolGetOffsetBitmask_KENAI (
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
      /* Initialization Control 3 (port 0)  */
      *Offset = 0x0024;
      /* APM Enable                         */
      *Bitmask = 0x0400;
      return WOL_SUCCESS;
    case 1:
      /* Initialization Control 3 (port 1)  */
      *Offset = 0x0014;
      /* APM Enable                         */
      *Bitmask = 0x0400;
      return WOL_SUCCESS;
  }
  return WOL_FEATURE_NOT_SUPPORTED;
}

WOL_STATUS
_WolGetOffsetBitmask_NAHUM (
  IN    WOL_ADAPTER_HANDLE_TYPE     Handle,
  OUT   UINT16                      *Offset,
  OUT   UINT16                      *Bitmask
  )
{
  UINT16  LanPort = 0;
  LanPort = _WolGetLanPort(Handle);

  if (LanPort == 0) {
    *Offset = 0x000A;
    *Bitmask = 0x0004;
    return WOL_SUCCESS;
  }
  return WOL_FEATURE_NOT_SUPPORTED;
}

WOL_STATUS
_WolGetOffsetBitmask_NAHUM2 (
  IN    WOL_ADAPTER_HANDLE_TYPE     Handle,
  OUT   UINT16                      *Offset,
  OUT   UINT16                      *Bitmask
  )
{
  UINT16  LanPort = 0;
  LanPort = _WolGetLanPort(Handle);

  if (LanPort == 0) {
    *Offset = 0x001A;
    *Bitmask = 0x0001;
    return WOL_SUCCESS;
  }
  return WOL_FEATURE_NOT_SUPPORTED;
}

WOL_STATUS
_WolGetOffsetBitmask_BARTONHILLS (
  IN    WOL_ADAPTER_HANDLE_TYPE     Handle,
  OUT   UINT16                      *Offset,
  OUT   UINT16                      *Bitmask
  )
{
  UINT16  LanPort = 0;
  LanPort = _WolGetLanPort(Handle);

  /* Initialization Control 3   */
  /* plus offset for given port */
  switch (LanPort) {
    case 0:
      /* Initialization Control 3   */
      /* plus offset for given port */
      *Offset  = 0x0024 + 0x0000;
      /* APM Enable                 */
      *Bitmask = 0x0400;
      return WOL_SUCCESS;
    case 1:
      *Offset  = 0x0024 + 0x0080;
      *Bitmask = 0x0400;
      return WOL_SUCCESS;
    case 2:
      *Offset  = 0x0024 + 0x00C0;
      *Bitmask = 0x0400;
      return WOL_SUCCESS;
    case 3:
      *Offset  = 0x0024 + 0x0100;
      *Bitmask = 0x0400;
      return WOL_SUCCESS;
  }
  return WOL_FEATURE_NOT_SUPPORTED;
}

WOL_STATUS _WolEnableLaser(WOL_ADAPTER_HANDLE_TYPE Handle, BOOLEAN Enable)
{
  extern WOL_MAC_TYPE const WOL_LASER_TABLE[];
  if (_WolFindMacType(_WolGetMacType(Handle), (WOL_MAC_TYPE *)WOL_LASER_TABLE)) {
    WOL_STATUS Status;
    UINT16 Value = 0;
    if (!(E1000_HW_PTR(Handle)->phy.media_type == e1000_media_type_fiber)) {
      return WOL_FEATURE_NOT_SUPPORTED;
    }
    
    /* Read Software Defined Pins Control */
    Status = _WolEepromRead16(Handle, 0x20, &Value);
    if (Status != WOL_SUCCESS) {
      return Status;
    }

    /* Set/reset Software Defined Pin 7 to enable or disable the laser */
    if (Enable) {
      Value |= 0x0080;
    } else {
      Value &= ~0x0080;
    }

    /* Write Software Defined Pins Control back and update checksum */
    Status = _WolEepromWrite16(Handle, 0x20, Value);
    if (Status != WOL_SUCCESS) {
      return Status;
    }

    return _WolEepromUpdateChecksum(Handle);
  }

  return WOL_FEATURE_NOT_SUPPORTED;
}

WOL_STATUS _WolSetApmRegister_1G (WOL_ADAPTER_HANDLE_TYPE Handle, BOOLEAN Enable)
{
  return WOL_SUCCESS;
}

