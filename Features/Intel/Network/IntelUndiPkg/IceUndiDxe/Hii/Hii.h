/**************************************************************************

Copyright (c) 2020 - 2021, Intel Corporation. All rights reserved.

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

#ifndef HII_H_
#define HII_H_

#include "CommonDriver.h"
#include "Forms/HiiFormDefs.h"
#include "Hii/FormsetStd/HiiConfigData.h"

#define HII_MAX_STR_LEN         1024
#define HII_MAX_STR_LEN_BYTES   (HII_MAX_STR_LEN * sizeof (CHAR16))

#define MAX_PBA_STR_LENGTH  15 // normally it is 10 chars string

/** Checks if language is x-UEFIxxx configuration language.

  @param[in]  Language  HII language in ASCII format
**/
#define IS_UEFI_CONFIG_LANG(Lang) \
  (AsciiStrnCmp (Lang, UEFI_CONFIG_LANG, 6) == 0)

/** Converts binary 6 byte MAC addr to UNICODE string with ':' separators.

  @param[out]  UniMac   Resultant UNICODE MAC string
  @param[in]   BinMac   Input binary MAC
**/
#define SET_UNI_MAC_FROM_BIN(UniMac, BinMac) \
  UnicodeSPrint (                            \
    UniMac,                                  \
    UNI_MAC_CHAR_COUNT * 2,                  \
    L"%02x:%02x:%02x:%02x:%02x:%02x",        \
    BinMac[0],                               \
    BinMac[1],                               \
    BinMac[2],                               \
    BinMac[3],                               \
    BinMac[4],                               \
    BinMac[5]                                \
    )

/** Converts Number of Gigabits to bits number describing speed of the port.

  @param[in]   Gigs   Number of Gigabits
**/
#define GIGABITS(Gigs) (1024ULL * 1024ULL * 1024ULL * ((UINT64)Gigs))

/** Checks whether MAC address is all 0's.

  @param[in]  Mac   MAC address
**/
#define IS_ZERO_MAC_ADDR(Mac)     \
  ((((UINT16 *) (Mac))[0] == 0) && \
   (((UINT16 *) (Mac))[1] == 0) && \
   (((UINT16 *) (Mac))[2] == 0))

/** Checks whether MAC address is broadcast.

  @param[in]  Mac   MAC address
**/
#define IS_BROADCAST_MAC_ADDR(Mac)     \
  ((((UINT16 *) (Mac))[0] == 0xFFFF) && \
   (((UINT16 *) (Mac))[1] == 0xFFFF) && \
   (((UINT16 *) (Mac))[2] == 0xFFFF))

/** Checks whether MAC address is multicast.

  @param[in]  Mac   MAC address
**/
#define IS_MULTICAST_MAC_ADDR(Mac) ((((UINT8 *)(Mac))[0] & 0x01) != 0)

/** Helper function that tries to retrieve inventory string from package.

  @param[in]   UndiPrivateData  Pointer to driver private data structure
  @param[out]  InventoryStr     Pointer to resultant string
  @param[in]   StringId         EFI_STRING_ID of required string
  @param[in]   Language         Language for which string should be retrieved or NULL

  @retval      EFI_SUCCESS        Operation successful
  @retval      EFI_DEVICE_ERROR   Failed to retrieve string
**/
EFI_STATUS
GetInventoryStr (
  IN   UNDI_PRIVATE_DATA  *UndiPrivateData,
  OUT  EFI_STRING         InventoryStr,
  IN   EFI_STRING_ID      StringId,
  IN   CONST CHAR8        *Language
  );


/** Uninstalls HII protocol & package related resources, frees memory allocations.
   (Resources previously obtained by HiiInit ()).

   @param[in,out]   UndiPrivateData   Points to the driver instance private data.

   @retval   EFI_SUCCESS    HII resources uninstalled correctly
   @retval   !EFI_SUCCESS   Failed to uninstall HII resources
**/
EFI_STATUS
HiiUnload (
  IN OUT  UNDI_PRIVATE_DATA  *UndiPrivateData
  );

/** Installs HII Config Access interfaces required by driver instance -
   protocol & package related resources.

   @param[in,out]   UndiPrivateData   Points to the driver instance private data.

   @retval   EFI_SUCCESS          Successful operation
   @retval   EFI_ALREADY_STARTED  HII interfaces are already installed or weren't properly uninstalled
   @retval   !EFI_SUCCESS         Failed to setup Config Access protocol/package resources or
                                  failed to locate required protocols
**/
EFI_STATUS
HiiInit (
  IN OUT  UNDI_PRIVATE_DATA  *UndiPrivateData
  );

/** Uninstalls HII packages installed in partial init flow.
   Reverts HiiAddStringPkgOnly () operations.

   @param[in,out]   UndiPrivateData   Points to the driver instance private data

   @retval   EFI_SUCCESS          Always returned
**/
EFI_STATUS
HiiRemoveStringPkgOnly (
  IN OUT  UNDI_PRIVATE_DATA  *UndiPrivateData
  );

/** Used instead of HiiInit() in partial init flow. Installs only string packages
   to allow Driver Health protocol reporting.

   @param[in,out]   UndiPrivateData   Points to the driver instance private data

   @retval   EFI_SUCCESS           Successful operation
   @retval   EFI_ALREADY_STARTED   HII string packages are already installed or weren't properly uninstalled
   @retval   EFI_OUT_OF_RESOURCES  Failed to register string packages
**/
EFI_STATUS
HiiAddStringPkgOnly (
  IN OUT  UNDI_PRIVATE_DATA  *UndiPrivateData
  );

#endif /* HII_H_ */
