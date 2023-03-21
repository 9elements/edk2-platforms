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
#ifndef __WOL_H
#define __WOL_H

#include <wolimpl.h>

#define WOL_DISABLE   0x00
#define WOL_ENABLE    0x01
#define WOL_NA        0x02

typedef  UINT8  ADAPTER_WOL_STATUS;


BOOLEAN
WolIsWakeOnLanSupported(
  IN    WOL_ADAPTER_HANDLE_TYPE     Handle
);

WOL_STATUS
WolGetWakeOnLanEnableStatus(
  IN    WOL_ADAPTER_HANDLE_TYPE     Handle,
  OUT   BOOLEAN                    *WolStatus
);

WOL_STATUS
WolGetWakeOnLanStatus (
  IN  WOL_ADAPTER_HANDLE_TYPE  Handle,
  OUT ADAPTER_WOL_STATUS       *WolStatus
  );

WOL_STATUS
WolEnableWakeOnLan(
  IN    WOL_ADAPTER_HANDLE_TYPE     Handle,
  IN    BOOLEAN                     Enable
);

WOL_STATUS
WolSetWakeOnLanStatus (
  IN WOL_ADAPTER_HANDLE_TYPE  Handle,
  IN ADAPTER_WOL_STATUS       *WolStatus
  );

WOL_STATUS
WolEnableApm(
  IN    WOL_ADAPTER_HANDLE_TYPE     Handle,
  IN    BOOLEAN                     Enable
);

WOL_STATUS
WolEnableApmPme(
  IN    WOL_ADAPTER_HANDLE_TYPE     Handle,
  IN    BOOLEAN                     Enable
);

WOL_STATUS
WolGetWakeOnLanStatus_Ice (
  IN    WOL_ADAPTER_HANDLE_TYPE     Handle,
  OUT   BOOLEAN                    *WolStatus
);

WOL_STATUS
WolEnableWakeOnLan_Ice (
  IN    WOL_ADAPTER_HANDLE_TYPE     Handle,
  IN    BOOLEAN                     Enable
);

WOL_STATUS
WolSetApmRegister_10G (
  IN    WOL_ADAPTER_HANDLE_TYPE Handle,
  IN    BOOLEAN Enable
  );

WOL_STATUS
_WolSetApmRegister_1G (
  IN    WOL_ADAPTER_HANDLE_TYPE Handle,
  IN    BOOLEAN Enable
  );
#endif /* __WOL_H */

