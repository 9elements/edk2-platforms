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


#if defined(WOL_1G)
static WOL_MAC_TYPE const _WOL_CORDOVA[] = {
  0
};

static WOL_MAC_TYPE const _WOL_KENAI[] = {
#ifndef NO_82571_SUPPORT
  WOL_MAKE_MACTYPE(WOL_1G, e1000_82571),
  WOL_MAKE_MACTYPE(WOL_1G, e1000_82572),
  WOL_MAKE_MACTYPE(WOL_1G, e1000_82573),
#ifndef NO_82574_SUPPORT
  WOL_MAKE_MACTYPE(WOL_1G, e1000_82574),
  WOL_MAKE_MACTYPE(WOL_1G, e1000_82583),
#endif
#endif
#ifndef NO_80003ES2LAN_SUPPORT
  WOL_MAKE_MACTYPE(WOL_1G, e1000_80003es2lan),
#endif
#ifndef NO_82575_SUPPORT
  WOL_MAKE_MACTYPE(WOL_1G, e1000_82575),
  WOL_MAKE_MACTYPE(WOL_1G, e1000_82576),
#endif
  0
};

static WOL_MAC_TYPE const _WOL_NAHUM[] = {
#ifndef NO_ICH8LAN_SUPPORT
  WOL_MAKE_MACTYPE(WOL_1G, e1000_ich8lan),
#endif
  0
};

static WOL_MAC_TYPE const _WOL_NAHUM2[] = {
#ifndef NO_ICH8LAN_SUPPORT
  WOL_MAKE_MACTYPE(WOL_1G, e1000_ich9lan),
  WOL_MAKE_MACTYPE(WOL_1G, e1000_ich10lan),
  WOL_MAKE_MACTYPE(WOL_1G, e1000_pchlan),
  WOL_MAKE_MACTYPE(WOL_1G, e1000_pch2lan),
#ifdef NAHUM6_HW
  WOL_MAKE_MACTYPE(WOL_1G, e1000_pch_lpt),
#endif
#endif
  0
};

static WOL_MAC_TYPE const _WOL_BARTONHILLS[] = {
#ifndef NO_82580_SUPPORT
  WOL_MAKE_MACTYPE(WOL_1G, e1000_82580),
#endif
  WOL_MAKE_MACTYPE(WOL_1G, e1000_i350),
  WOL_MAKE_MACTYPE(WOL_1G, e1000_i354),
#ifndef NO_I210_SUPPORT
  WOL_MAKE_MACTYPE(WOL_1G, e1000_i210),
  WOL_MAKE_MACTYPE(WOL_1G, e1000_i211),
#endif
  0
};
#endif



#if defined(WOL_ICE)
WOL_MAC_TYPE const _WOL_ICE[] = {
#if defined(E810C_SUPPORT) || defined(E810_XXV_SUPPORT)
  WOL_MAKE_MACTYPE(WOL_ICE, ICE_MAC_E810),
#endif /* E810C_SUPPORT || E810_XXV_SUPPORT */
  WOL_MAKE_MACTYPE(WOL_ICE, ICE_MAC_GENERIC),
  0
};
#endif


extern WOL_STATUS _WolGetOffsetBitmask_PRO100(WOL_ADAPTER_HANDLE_TYPE Handle, UINT16 *Offset, UINT16 *Bitmask);
extern WOL_STATUS _WolGetOffsetBitmask_CORDOVA(WOL_ADAPTER_HANDLE_TYPE Handle, UINT16 *Offset, UINT16 *Bitmask);
extern WOL_STATUS _WolGetOffsetBitmask_KENAI(WOL_ADAPTER_HANDLE_TYPE Handle, UINT16 *Offset, UINT16 *Bitmask);
extern WOL_STATUS _WolGetOffsetBitmask_NAHUM(WOL_ADAPTER_HANDLE_TYPE Handle, UINT16 *Offset, UINT16 *Bitmask);
extern WOL_STATUS _WolGetOffsetBitmask_NAHUM2(WOL_ADAPTER_HANDLE_TYPE Handle, UINT16 *Offset, UINT16 *Bitmask);
extern WOL_STATUS _WolGetOffsetBitmask_BARTONHILLS(WOL_ADAPTER_HANDLE_TYPE Handle, UINT16 *Offset, UINT16 *Bitmask);
extern WOL_STATUS _WolGetOffsetBitmask_IXGBE(WOL_ADAPTER_HANDLE_TYPE Handle, UINT16 *Offset, UINT16 *Bitmask);
extern WOL_STATUS _WolGetOffsetBitmask_40GBE(WOL_ADAPTER_HANDLE_TYPE Handle, UINT16 *Offset, UINT16 *Bitmask);

_WOL_FAMILY_INFO_t const WOL_FAMILY_TABLE[] = {
#if defined(WOL_1G)
  { _WOL_CORDOVA,       _WolGetOffsetBitmask_CORDOVA        },
  { _WOL_KENAI,         _WolGetOffsetBitmask_KENAI          },
  { _WOL_NAHUM,         _WolGetOffsetBitmask_NAHUM          },
  { _WOL_NAHUM2,        _WolGetOffsetBitmask_NAHUM2         },
  { _WOL_BARTONHILLS,   _WolGetOffsetBitmask_BARTONHILLS    },
#endif /* WOL_1G */
  { 0,                  0                                   }
};

WOL_MAC_TYPE const WOL_APMPME_TABLE[] = {
#if defined(WOL_1G)
  /*
   * Wainwright doesn't seem to be supported
   * neither by QV nor by shared code
   */
#ifndef NO_82571_SUPPORT
  WOL_MAKE_MACTYPE(WOL_1G, e1000_82571),
  WOL_MAKE_MACTYPE(WOL_1G, e1000_82572),
  WOL_MAKE_MACTYPE(WOL_1G, e1000_82573),
#ifndef NO_82574_SUPPORT
  WOL_MAKE_MACTYPE(WOL_1G, e1000_82574),
  WOL_MAKE_MACTYPE(WOL_1G, e1000_82583),
#endif
#endif
#ifndef NO_80003ES2LAN_SUPPORT
  WOL_MAKE_MACTYPE(WOL_1G, e1000_80003es2lan),
#endif
#ifndef NO_82575_SUPPORT
  WOL_MAKE_MACTYPE(WOL_1G, e1000_82575),
  WOL_MAKE_MACTYPE(WOL_1G, e1000_82576),
#endif
#ifndef NO_82580_SUPPORT
  WOL_MAKE_MACTYPE(WOL_1G, e1000_82580),
#endif
  WOL_MAKE_MACTYPE(WOL_1G, e1000_i350),
  WOL_MAKE_MACTYPE(WOL_1G, e1000_i354),
#ifndef NO_I210_SUPPORT
  WOL_MAKE_MACTYPE(WOL_1G, e1000_i210),
  WOL_MAKE_MACTYPE(WOL_1G, e1000_i211),
#endif
#endif /* WOL_1G */
  0
};

WOL_MAC_TYPE const WOL_LASER_TABLE[] = {
#if defined(WOL_1G)
#ifndef NO_82571_SUPPORT
  WOL_MAKE_MACTYPE(WOL_1G, e1000_82571),
  WOL_MAKE_MACTYPE(WOL_1G, e1000_82572),
  WOL_MAKE_MACTYPE(WOL_1G, e1000_82573),
#ifndef NO_82574_SUPPORT
  WOL_MAKE_MACTYPE(WOL_1G, e1000_82574),
  WOL_MAKE_MACTYPE(WOL_1G, e1000_82583),
#endif
#endif
#ifndef NO_80003ES2LAN_SUPPORT
  WOL_MAKE_MACTYPE(WOL_1G, e1000_80003es2lan),
#endif
#endif /* WOL_1G */
  0
};

