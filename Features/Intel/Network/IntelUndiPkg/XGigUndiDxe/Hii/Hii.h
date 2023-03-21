/******************************************************************************
**                                                                           **
** INTEL CONFIDENTIAL                                                        **
**                                                                           **
** Copyright (c) 2020 - 2021, Intel Corporation. All rights reserved.        **
**                                                                           **
** The source code contained or described herein and all documents related   **
** to the source code ("Material") are owned by Intel Corporation or its     **
** suppliers or licensors.  Title to the Material remains with Intel         **
** Corporation or its suppliers and licensors.  The Material contains trade  **
** secrets and proprietary and confidential information of Intel or its      **
** suppliers and licensors.  The Material is protected by worldwide          **
** copyright and trade secret laws and treaty provisions.  No part of the    **
** Material may be used, copied, reproduced, modified, published, uploaded,  **
** posted, transmitted, distributed, or disclosed in any way without Intel's **
** prior express written permission.                                         **
**                                                                           **
** No license under any patent, copyright, trade secret or other             **
** intellectual property right is granted to or conferred upon you by        **
** disclosure or delivery of the Materials, either expressly, by             **
** implication, inducement, estoppel or otherwise.  Any license under such   **
** intellectual property rights must be express and approved by Intel in     **
** writing.                                                                  **
**                                                                           **
******************************************************************************/

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