/**************************************************************************

Copyright (c) 2012 - 2022, Intel Corporation. All rights reserved.

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
#ifndef EEPROM_CONFIG_H_
#define EEPROM_CONFIG_H_

#include "Xgbe.h"

/* Compatibility Word in EEPROM
 The only thing we need this define for it to determine if the device is a LOM */
#define EEPROM_COMPATIBILITY_WORD          0x03
#define EEPROM_COMPATIBILITY_WORD_OPLIN    0x10
#define EEPROM_COMPATABILITY_LOM_BIT       0x0800

#define XGBE_EEPROM_CORE0                 0x9
#define XGBE_EEPROM_CORE1                 0xA

#define IOV_CONTROL_WORD_1_OFFSET                   0x0C
#define IOV_CONTROL_WORD_IOVENABLE_SHIFT            0
#define IOV_CONTROL_WORD_IOVENABLE_MASK             0x0001
#define IOV_CONTROL_WORD_MAXVFS_SHIFT               5
#define IOV_CONTROL_WORD_MAXVFS_MASK                0x07E0
#define IOV_CONTROL_WORD_MAXVFS_MAX                 63

// Sageville PCIe Capabilities Support
#define PCI_CAPSUP_L_OFFSET         0x000A
#define PCI_CAPSUP_H_OFFSET         0x000B
#define PCI_CAPSUP_IOVENABLE_SHIFT  0x0005
#define PCI_CAPSUP_IOVENABLE_MASK   0x01 << PCI_CAPSUP_IOVENABLE_SHIFT /* 0x0020 */
#define PCI_CNF2_L_OFFSET           0x0000
#define PCI_CNF2_H_OFFSET           0x0001
#define PCI_CNF2_NUM_VFS_SHIFT      0x0008
#define PCI_CNF2_NUM_VFS_MASK       0x7F << PCI_CNF2_NUM_VFS_SHIFT     /* 0x7F00 */
#define PCI_CNF2_NUM_VFS_MAX        64

// EEPROM power management bit definitions
#define XGBE_EEPROM_CONTROL_WORD3         0x38
#define XGBE_EEPROM_APM_ENABLE_PORT1      0x2
#define XGBE_EEPROM_APM_ENABLE_PORT0      0x1
#define XGBE_PCIE_CONFIG0_PTR             0x07
#define XGBE_PCIE_CONFIG1_PTR             0x08
#define XGBE_FLASH_DISABLE_BIT            0x0100 /* bit 8 */

#ifndef BIT
#define BIT(a) (1UL << (a))
#endif /* BIT */

#define XGBE_NVM_CONTROL_WORD2              0x0001
#define XGBE_NVM_CONTROL_WORD2_LAN_DIS_BIT  BIT (8)

#define PF0                                 0
#define PF1                                 1

typedef enum {
  LOCATION_DIRECT,  // direct offset
  LOCATION_POINTER, // pointer
} LOCATION_TYPE;

#define  NVM_DIRECT(Offset1)           {LOCATION_DIRECT, Offset1}
#define  NVM_POINTER(Offset1, Offset2) {LOCATION_POINTER, Offset1, Offset2}

typedef struct {
  LOCATION_TYPE Type;
  UINT32        Offset1;
  UINT32        Offset2;
} NVM_LOCATION;

#define MAX_EXCLUDED_FIELDS_RECORD_COUNT  40

typedef struct {
  NVM_LOCATION Location;          // location in NVM
  UINT32       Length;
  UINT32       ModuleStartOffset;
  UINT16       BitMask;           // bits mask - applies to all words pointed by Size
} EXCLUDED_FIELDS_RECORD;

/** Reads SR buffer.

   @param[in]   UndiPrivateData  Points to the driver information.
   @param[in]   Offset           Offset in words from module start.
   @param[in]   Length           Number of words to read.
   @param[out]  Data             Pointer to location with data to be read to.

   @retval    EFI_SUCCESS            Buffer successfully read.
   @retval    EFI_INVALID_PARAMETER  UndiPrivateData or Data is NULL.
   @retval    EFI_DEVICE_ERROR       Failed to read buffer.
**/
EFI_STATUS
ReadSrBuffer16 (
  IN  UNDI_PRIVATE_DATA  *UndiPrivateData,
  IN  UINT16             Offset,
  IN  UINT16             Length,
  OUT UINT16             *Data
  );

/** Reads SR word.

   @param[in]   UndiPrivateData  Points to the driver information.
   @param[in]   Offset           Offset in words from module start.
   @param[out]  Data             Pointer to location with data to be read to.

   @retval    EFI_SUCCESS     Word successfully read.
   @retval    !EFI_SUCCESS    Word not read, failure of underlying function.
**/
EFI_STATUS
ReadSr16 (
  IN  UNDI_PRIVATE_DATA  *UndiPrivateData,
  IN  UINT16             Offset,
  OUT UINT16             *Data
  );

// Assume the maximum buffer size that can be changed at one time is 512 bytes
#define EEPROM_WRITE_BUFFER_SIZE 512

/** Writes SR buffer.

   @param[in]   UndiPrivateData  Points to the driver information.
   @param[in]   Offset           Offset in words from module start.
   @param[in]   Length           Number of words to write.
   @param[in]   Data             Pointer to location with words to be written.

   @retval    EFI_SUCCESS            Buffer successfully written.
   @retval    EFI_INVALID_PARAMETER  UndiPrivateData or Data is NULL.
   @retval    EFI_DEVICE_ERROR       Failed to write buffer.
**/
EFI_STATUS
WriteSrBuffer16 (
  IN  UNDI_PRIVATE_DATA  *UndiPrivateData,
  IN  UINT16             Offset,
  IN  UINT16             Length,
  IN  UINT16             *Data
  );

/** Writes SR word.

   @param[in]   UndiPrivateData  Points to the driver information.
   @param[in]   Offset           Offset in words from module start.
   @param[in]  Data             Word to be written.

   @retval    EFI_SUCCESS    Word successfully written.
   @retval    !EFI_SUCCESS   Word not written, failure of underlying function.
**/
EFI_STATUS
WriteSr16 (
  IN  UNDI_PRIVATE_DATA  *UndiPrivateData,
  IN  UINT16             Offset,
  IN  UINT16             Data
  );



/** Gets link speed setting for adapter.

   @param[in]   UndiPrivateData        Pointer to driver private data structure
   @param[out]  LinkSpeed              Link speed setting

   @retval      EFI_SUCCESS            Successful operation
   @retval      EFI_INVALID_PARAMETER  UndiPrivateData or LinkSpeed is NULL
**/
EFI_STATUS
GetLinkSpeed (
  IN  UNDI_PRIVATE_DATA *UndiPrivateData,
  OUT UINT8             *LinkSpeed
  );

/** Sets link speed setting for adapter.

   @param[in]   UndiPrivateData  Pointer to driver private data structure
   @param[in]   LinkSpeed        Link speed setting

   @retval      EFI_SUCCESS      Successful operation
**/
EFI_STATUS
SetLinkSpeed (
  IN  UNDI_PRIVATE_DATA  *UndiPrivateData,
  IN  UINT8              *LinkSpeed
  );

/** Restores the factory default MAC address.

   @param[in]   UndiPrivateData   Driver private data structure

   @retval   EFI_UNSUPPORTED   Invalid offset for alternate MAC address
   @retval   EFI_SUCCESS       Alternate MAC Address feature not enabled
**/
EFI_STATUS
RestoreDefaultMacAddress (
  IN UNDI_PRIVATE_DATA *UndiPrivateData
  );



/** Programs the port with an alternate MAC address, and (in 82580-like case)
   backs up the factory default MAC address.

   @param[in]   UndiPrivateData   Pointer to driver private data structure
   @param[in]   MacAddress        Value to set the MAC address to.

   @retval   EFI_UNSUPPORTED   Alternate MAC Address feature not enabled
   @retval   EFI_SUCCESS       Default MAC address set successfully
**/
EFI_STATUS
SetAlternateMacAddress (
  IN UNDI_PRIVATE_DATA *UndiPrivateData,
  IN UINT8             *MacAddress
  );

/** Reads factory default MAC address.

   @param[in]   UndiPrivateData      Pointer to driver private data structure
   @param[out]  FactoryMacAddress    Factory default MAC address of the adapter

   @retval      EFI_SUCCESS       MAC addresses read successfully
   @retval      !EFI_SUCCESS      Failure of underlying function
**/
EFI_STATUS
GetFactoryMacAddress (
  IN  UNDI_PRIVATE_DATA  *UndiPrivateData,
  OUT UINT8              *FactoryMacAddress
  );

/** Gets alternate MAC address of currently managed PF.

   @param[in]   UndiPrivateData      Pointer to driver private data structure
   @param[out]  AlternateMacAddress  Pointer to buffer for resulting alternate
                                     MAC address

   @retval      EFI_SUCCESS          MAC addresses read successfully
   @retval      !EFI_SUCCESS         Failure of underlying function
**/
EFI_STATUS
GetAlternateMacAddress (
  IN   UNDI_PRIVATE_DATA  *UndiPrivateData,
  OUT  UINT8              *AlternateMacAddress
  );


#define EEPROM_CAPABILITIES_WORD 0x33
#define EEPROM_CAPABILITIES_SIG  0x4000

/** Returns EEPROM capabilities word (0x33) for current adapter

   @param[in]    UndiPrivateData   Points to the driver instance private data
   @param[out]   CapabilitiesWord   EEPROM capabilities word (0x33) for current adapter

   @retval   EFI_SUCCESS   Function completed successfully,
   @retval   !EFI_SUCCESS  Failed to read EEPROM capabilities word
**/
EFI_STATUS
EepromGetCapabilitiesWord (
  IN  UNDI_PRIVATE_DATA *UndiPrivateData,
  OUT UINT16 *           CapabilitiesWord
  );

/** Checks if it is LOM device

   @param[in]   UndiPrivateData   Points to the driver instance private data

   @retval   TRUE     It is LOM device
   @retval   FALSE    It is not LOM device
   @retval   FALSE    Failed to read NVM word
**/
BOOLEAN
EepromIsLomDevice (
  IN  UNDI_PRIVATE_DATA *UndiPrivateData
  );

/** Updates NVM checksum

   @param[in]   UndiPrivateData   Pointer to driver private data structure

   @retval      EFI_SUCCESS       Checksum successfully updated
   @retval      EFI_DEVICE_ERROR  Failed to update NVM checksum
**/
EFI_STATUS
UpdateNvmChecksum (
  IN  UNDI_PRIVATE_DATA *UndiPrivateData
  );




/** Returns information on number of physical functions supported by the adapter.

   @param[in]   UndiPrivateData Points to the driver instance private data.
   @param[out]  PfsNum          Default number of PFs supported by adapter

   @retval   EFI_SUCCESS            Number of PFs returned successfully
   @retval   EFI_INVALID_PARAMETER  UndiPrivateData is NULL
   @retval   EFI_DEVICE_ERROR       Failed to check if Lenovo port disable is supported

**/
EFI_STATUS
GetNumberOfPfs (
  IN  UNDI_PRIVATE_DATA *UndiPrivateData,
  OUT UINT8             *PfsNum
  );

/** Creates a bitmask which stores information on available physical functions.

   @param[in]   UndiPrivateData Points to the driver instance private data.
   @param[out]  BitMask         Pointer to a variable which should store the bitmask.
                                Each set bit corresponds to enabled PF.

   @retval   EFI_SUCCESS             Created the bitmask successfully
   @retval   EFI_INVALID_PARAMETER   UndiPrivateData is NULL
   @retval   EFI_INVALID_PARAMETER   BitMask is NULL
   @retval   EFI_DEVICE_ERROR        Failed to create the bitmask

**/
EFI_STATUS
GetPfsActiveState (
  IN  UNDI_PRIVATE_DATA *UndiPrivateData,
  OUT UINT8             *BitMask
  );

/** Disables/enables physical functions according to values set in bitmask.
    Each bit represents one PF. Set bit means enabled, opposite disabled.

   @param[in]   UndiPrivateData Points to the driver instance private data.
   @param[in]   BitMask         Input bitmask which defines disabled/enabled PFs.

   @retval   EFI_SUCCESS             Disabled/enabled successfully
   @retval   EFI_INVALID_PARAMETER   UndiPrivateData is NULL
   @retval   EFI_INVALID_PARAMETER   BitMask is 0
   @retval   EFI_DEVICE_ERROR        Failed to disable/enable functions

**/
EFI_STATUS
SetPfsActiveState (
  IN UNDI_PRIVATE_DATA *UndiPrivateData,
  IN UINT8             *BitMask
  );
#endif /* EEPROM_CONFIG_H_ */
