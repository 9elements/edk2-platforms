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


extern WOL_STATUS _WolEnableLaser(WOL_ADAPTER_HANDLE_TYPE Handle, BOOLEAN Enable);
extern BOOLEAN _WolGetInfoFromEeprom_10G(WOL_ADAPTER_HANDLE_TYPE Handle);
extern BOOLEAN _WolGetInfoFromEeprom_40G(WOL_ADAPTER_HANDLE_TYPE Handle);
extern BOOLEAN _WolGetInfoFromEeprom_100G(WOL_ADAPTER_HANDLE_TYPE Handle);
extern WOL_STATUS WolSetApmRegister(WOL_ADAPTER_HANDLE_TYPE Handle, BOOLEAN Enable);

static BOOLEAN _WolIsDevInfoEmpty(_WOL_DEVICE_INFO_t const *DeviceInfo)
{
  return DeviceInfo->VendorId == 0 && DeviceInfo->DeviceId == 0 &&
         DeviceInfo->SubVendorId == 0 && DeviceInfo->SubDeviceId == 0;
}

static BOOLEAN _WolMatchId(UINT16 Id, UINT16 Pattern)
{
  return Id == Pattern || Pattern == 0xFFFF;
}

static BOOLEAN _WolMatchDeviceId(
  _WOL_DEVICE_ID_t *DeviceId,
  _WOL_DEVICE_ID_t *Pattern
) {
  return _WolMatchId(DeviceId->VendorId, Pattern->VendorId) &&
         _WolMatchId(DeviceId->DeviceId, Pattern->DeviceId) &&
         _WolMatchId(DeviceId->SubVendorId, Pattern->SubVendorId) &&
         _WolMatchId(DeviceId->SubDeviceId, Pattern->SubDeviceId);
}

static _WOL_DEVICE_INFO_t *_WolFindDeviceInfo(
  _WOL_DEVICE_ID_t *DeviceId,
  _WOL_DEVICE_INFO_t *DeviceInfoTable
) {
  while (!_WolIsDevInfoEmpty(DeviceInfoTable)) {
    if (_WolMatchDeviceId(DeviceId, (_WOL_DEVICE_ID_t *)DeviceInfoTable)) {
      return DeviceInfoTable;
    }
    ++DeviceInfoTable;
  }

  return NULL;
}

static BOOLEAN _WolGetInfo(WOL_ADAPTER_HANDLE_TYPE Handle, _WOL_DEVICE_INFO_t const *DeviceInfo)
{
  switch (DeviceInfo->WolInfo) {
    case 1: /* WOL is supported on the first port on the NIC. */
      /* The first port on the NIC means the first port
       * on the first port controller. As many 4-port legacy 1G NICs are
       * equipped with two port controllers we have to account for that.
       */
      if (_WolIsFirstController(Handle)) {
        return _WolGetFunction(Handle) == 0;
      } else {
        return FALSE;
      }

    case 0xF: /* WOL is supported on all ports. */
      #if defined(WOL_10G)
      {
        /* For TVL even if the WoL is supported on all ports as indicated in INF,
         * it still can be disabled in EEPROM.
         */
        if (_WolGetMacType(Handle) == WOL_MAKE_MACTYPE(WOL_10G, ixgbe_mac_X540)) {
          return _WolGetInfoFromEeprom_10G(Handle);
        } else {
            return TRUE;
        }
      }
      #else
        return TRUE;
      #endif

    case 0: /* WOL is not supported at all. */
      return FALSE;
  }

  return FALSE;
}

WOL_MAC_TYPE _WolFindMacType(WOL_MAC_TYPE MacType, WOL_MAC_TYPE *MacTypes)
{
  while (WOL_MAC_TYPE_EMPTY != *MacTypes) {
    if (MacType == *MacTypes) {
      return MacType;
    }
    ++MacTypes;
  }

  return WOL_MAC_TYPE_EMPTY;
}

BOOLEAN WolIsWakeOnLanSupported(WOL_ADAPTER_HANDLE_TYPE Handle)
{
#if defined(WOL_ICE)
{
  extern WOL_MAC_TYPE const _WOL_ICE[];
#ifndef WOL_HAF
  DEBUGPRINT (WOL, ("--> WolIsWakeOnLanSupported"));
#endif
  /* In case of CVL read WOL configuration from EEPROM */
  if (_WolFindMacType(_WolGetMacType(Handle), (WOL_MAC_TYPE *)_WOL_ICE)) {
#ifndef WOL_HAF
    DEBUGPRINT (WOL, ("- found MAC type\n"));
#endif
    return _WolGetInfoFromEeprom_100G(Handle);
  }
}
#endif
#if defined(WOL_10G)
{
    /* In case of post TVL 10G devices read WOL configuration from EEPROM */
    extern WOL_MAC_TYPE const _WOL_IXGBE[];
    if (_WolFindMacType(_WolGetMacType(Handle), (WOL_MAC_TYPE *)_WOL_IXGBE) &&
        _WolGetMacType(Handle) > WOL_MAKE_MACTYPE(WOL_10G, ixgbe_mac_X540)) {
        return _WolGetInfoFromEeprom_10G(Handle);
    }
}
#endif
  {
    extern _WOL_DEVICE_INFO_t const WOL_DEVICE_INFO_TABLE[];
    _WOL_DEVICE_ID_t DeviceId;
    _WOL_DEVICE_INFO_t *DeviceInfo;

    _WolGetDeviceId(Handle, &DeviceId);


    /* Handle devices listed in the WOL_DEVICE_INFO_TABLE generated based on INF
     * files from NDIS drivers.
     */
    DeviceInfo = _WolFindDeviceInfo(&DeviceId, (_WOL_DEVICE_INFO_t *)WOL_DEVICE_INFO_TABLE);

    if (NULL != DeviceInfo) {
      return _WolGetInfo(Handle, DeviceInfo);
    }
  }

  return FALSE;
}

WOL_STATUS
_WolGetOffsetBitmask(
  IN    WOL_ADAPTER_HANDLE_TYPE     Handle,
  OUT   UINT16                      *Offset,
  OUT   UINT16                      *Bitmask
  )
{
  extern _WOL_FAMILY_INFO_t const WOL_FAMILY_TABLE[];
  _WOL_FAMILY_INFO_t const *FamilyTable;
#ifndef WOL_HAF
  DEBUGPRINT (WOL, ("-->_WolGetOffsetBitmask\n"));
#endif
  for (FamilyTable = WOL_FAMILY_TABLE; FamilyTable->Family; ++FamilyTable) {
    if (_WolFindMacType(_WolGetMacType(Handle), (WOL_MAC_TYPE *)FamilyTable->Family)) {
#ifndef WOL_HAF
      DEBUGPRINT (WOL, ("Found FamilyTable\n"));
#endif
      return FamilyTable->WolGetOffsetBitmask(Handle, Offset, Bitmask);
    }
  }

  return WOL_FEATURE_NOT_SUPPORTED;
}

WOL_STATUS
WolGetWakeOnLanEnableStatus(
  IN    WOL_ADAPTER_HANDLE_TYPE     Handle,
  OUT   BOOLEAN                    *WolStatus
  )
{
  WOL_STATUS  Status;
  UINT16      Offset;
  UINT16      Value  = 0;
  UINT16      Mask   = 0;
#ifdef WOL_ICE
  extern WOL_MAC_TYPE const _WOL_ICE[];
#endif
#ifndef WOL_HAF
  DEBUGPRINT (WOL, ("--> WolGetWakeOnLanStatus\n"));
#endif
  
  if (!WolIsWakeOnLanSupported(Handle)) {
    return WOL_FEATURE_NOT_SUPPORTED;
  }
#if defined (WOL_ICE)
  if (_WolFindMacType(_WolGetMacType(Handle), (WOL_MAC_TYPE *)_WOL_ICE)) {
    /* For 100G we use ANVM */
    Status = WolGetWakeOnLanStatus_Ice(Handle, WolStatus);
    if (Status != WOL_SUCCESS) {
      return Status;
    }
  }
  else
  {
#endif /* WOL_ICE */
    /* For other families we use EEPROM reading */
      Status = _WolGetOffsetBitmask(Handle, &Offset, &Mask);
      if (Status != WOL_SUCCESS) {
        return Status;
      }

      Status = _WolEepromRead16(Handle, Offset, &Value);
      if (Status != WOL_SUCCESS) {
        return Status;
      }

      *WolStatus = (Value & Mask) != 0;
#if defined (WOL_ICE)
  }
#endif /* WOL_ICE */

  return WOL_SUCCESS;
}

WOL_STATUS
WolGetWakeOnLanStatus(
  IN  WOL_ADAPTER_HANDLE_TYPE  Handle,
  OUT ADAPTER_WOL_STATUS       *WolStatus
  )
{
  WOL_STATUS  Status;
  BOOLEAN     WolEnabled;
#ifndef WOL_HAF
  DEBUGPRINT (WOL, ("--> WolGetWakeOnLanStatus\n"));
#endif

  Status = WolGetWakeOnLanEnableStatus (Handle, &WolEnabled);
  if (Status == WOL_SUCCESS) {
    *WolStatus = WolEnabled ? WOL_ENABLE : WOL_DISABLE;
  } else if (Status == WOL_FEATURE_NOT_SUPPORTED) {
    *WolStatus = WOL_NA;
  } else {
    return WOL_ERROR;
  }

  return WOL_SUCCESS;
}

WOL_STATUS
WolEnableApm(
  IN    WOL_ADAPTER_HANDLE_TYPE     Handle,
  IN    BOOLEAN                     Enable
  )
{
  WOL_STATUS  Status;
  UINT16      Offset;
  UINT16      Value  = 0;
  UINT16      Mask   = 0;

#ifndef WOL_HAF
  DEBUGPRINT (WOL, ("--> WolEnableApm\n"));
#endif
  Status = _WolGetOffsetBitmask(Handle, &Offset, &Mask);
  if (Status != WOL_SUCCESS) {
    return Status;
  }

  Status = _WolEepromRead16(Handle, Offset, &Value);
  if (Status != WOL_SUCCESS) {
    return Status;
  }
#ifndef WOL_HAF
  DEBUGPRINT (WOL, ("Before enable Value: %x Offset: %x\n", Value, Offset));
#endif
  if (Enable) {
    Value |= Mask;
  } else {
    Value &= ~Mask;
  }

  Status = _WolEepromWrite16(Handle, Offset, Value);
  if (Status != WOL_SUCCESS) {
#ifndef WOL_HAF
    DEBUGPRINT (WOL, ("WolEnableApm failed\n"));
#endif
    return Status;
  }

#ifndef WOL_HAF
  DEBUGPRINT (WOL, ("WolEnableApm success !\n"));
#endif
  return _WolEepromUpdateChecksum(Handle);
}

WOL_STATUS
WolEnableApmPme(
  IN    WOL_ADAPTER_HANDLE_TYPE     Handle,
  IN    BOOLEAN                     Enable
  )
{

  return WOL_FEATURE_NOT_SUPPORTED;
}

WOL_STATUS
WolEnableWakeOnLan (
  IN    WOL_ADAPTER_HANDLE_TYPE     Handle,
  IN    BOOLEAN                     Enable
  )
{
#ifdef WOL_ICE
  extern WOL_MAC_TYPE const _WOL_ICE[];
#endif
#ifndef WOL_HAF
  DEBUGPRINT (WOL, ("--> WolEnableWakeOnLan\n"));
#endif
  if (WolIsWakeOnLanSupported(Handle)) {
    WOL_STATUS Status;
#ifndef WOL_HAF
    DEBUGPRINT (WOL, ("WolEnableWakeOnLan - Supported\n"));
#endif
#if defined (WOL_ICE)
    if (_WolFindMacType(_WolGetMacType(Handle), (WOL_MAC_TYPE *)_WOL_ICE)) {
      /* For 100G we use ANVM */
      Status = WolEnableWakeOnLan_Ice(Handle, Enable);
      if (Status != WOL_SUCCESS) {
        return Status;
      }
    }
    else
    {
#endif /* WOL_ICE */
      /* For other families we use APM enabling/disabling */
      Status = WolEnableApm(Handle, Enable);
      if (Status != WOL_SUCCESS) {
        return Status;
      }
#if defined (WOL_ICE)
    }
#endif /* WOL_ICE */

    Status = WolSetApmRegister (Handle, Enable);
    if (Status != WOL_SUCCESS) {
      return Status;
    }


    return Status;
  }
  return WOL_FEATURE_NOT_SUPPORTED;
}

WOL_STATUS
WolSetWakeOnLanStatus (
  IN WOL_ADAPTER_HANDLE_TYPE  Handle,
  IN ADAPTER_WOL_STATUS       *WolStatus
  )
{
  WOL_STATUS          Status;
  ADAPTER_WOL_STATUS  CurrentWolStatus;

  if (*WolStatus == WOL_NA) {
    return WOL_SUCCESS;
  }

  /* Process the change only when it differs from current adapter WOL status */
  Status = WolGetWakeOnLanStatus (Handle, &CurrentWolStatus);
  if (Status != WOL_SUCCESS) {
    return Status;
  }

  if (*WolStatus == CurrentWolStatus) {
    return WOL_SUCCESS;
  }

  return WolEnableWakeOnLan (Handle, *WolStatus == WOL_ENABLE);
}
