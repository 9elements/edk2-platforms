/**************************************************************************

Copyright (c) 2020 - 2022, Intel Corporation. All rights reserved.

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
#include "Hii/Hii.h"
#include "DeviceSupport.h"
#include "EepromConfig.h"

#include "wol.h"


/** Gets default WOL status.

  @param[in]   UndiPrivateData       Pointer to driver private data structure
  @param[out]  WolStatus             Default WOL status

  @retval     EFI_SUCCESS            Operation successful
**/
EFI_STATUS
GetDefaultWolStatus (
  IN   UNDI_PRIVATE_DATA   *UndiPrivateData,
  OUT  ADAPTER_WOL_STATUS  *WolStatus
  )
{
  if (WolIsWakeOnLanSupported (UndiPrivateData)) {
    *WolStatus = WOL_ENABLE;
  } else {
    *WolStatus = WOL_NA;
  }

  return EFI_SUCCESS;
}

/** Gets Alternate MAC address attribute.

  @param[in]   UndiPrivateData       Pointer to driver private data structure
  @param[out]  AltMacAddrUni         Resultant MAC address string

  @retval     EFI_SUCCESS            Operation successful
  @retval     !EFI_SUCCESS           Failed to get factory or alternate MAC address
**/
EFI_STATUS
GetAltMacAddr (
  IN   UNDI_PRIVATE_DATA  *UndiPrivateData,
  OUT  UINT16             *AltMacAddrUni
  )
{
  UINT8       FactoryMac[ETH_ALEN];
  UINT8       AlternateMac[ETH_ALEN];
  EFI_STATUS  Status;

  Status = GetFactoryMacAddress (UndiPrivateData, FactoryMac);
  IF_RETURN (EFI_ERROR (Status), Status);
  Status = GetAlternateMacAddress (UndiPrivateData, AlternateMac);
  IF_RETURN (EFI_ERROR (Status), Status);

  if (CompareMem (FactoryMac, AlternateMac, ETH_ALEN) == 0) {
    ZeroMem (AlternateMac, ETH_ALEN);
  }

  SET_UNI_MAC_FROM_BIN (AltMacAddrUni, AlternateMac);
  return EFI_SUCCESS;
}

