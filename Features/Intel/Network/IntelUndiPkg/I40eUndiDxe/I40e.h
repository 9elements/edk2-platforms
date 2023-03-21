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
#ifndef I40E_H_
#define I40E_H_

#include "Version.h"

#include <Uefi.h>

#include <Base.h>
#include <Guid/EventGroup.h>
#include <Protocol/PciIo.h>
#include <Protocol/NetworkInterfaceIdentifier.h>
#include <Protocol/DevicePath.h>
#include <Protocol/ComponentName.h>
#include <Protocol/ComponentName2.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/DriverDiagnostics.h>
#include <Protocol/DriverBinding.h>
#include <Protocol/DriverSupportedEfiVersion.h>
#include <Protocol/PlatformToDriverConfiguration.h>
#include <Protocol/FirmwareManagement.h>
#include <Protocol/DriverHealth.h>

#include <Protocol/HiiConfigAccess.h>
#include <Protocol/HiiDatabase.h>
#include <Protocol/HiiString.h>
#include <Protocol/HiiConfigRouting.h>

#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiRuntimeLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/BaseLib.h>
#include <Library/DevicePathLib.h>
#include <Library/PrintLib.h>

#include <IndustryStandard/Pci.h>

// Debug macros are located here.
#include "DebugTools.h"

#include "i40e_type.h"
#include "i40e_prototype.h"
#include "StartStop.h"
#include "AdapterInformation.h"
#include "DriverHealth.h"
#include "Dma.h"


#include "Hii/CfgAccessProt/HiiConfigAccessInfo.h"

#include "LanEngine/Receive.h"
#include "LanEngine/Transmit.h"

/* Defines & Macros */

#ifndef I40E_INTEL_VENDOR_ID
#define I40E_INTEL_VENDOR_ID INTEL_VENDOR_ID
#endif /* I40E_INTEL_VENDOR_ID */
#define INTEL_VENDOR_ID      0x8086

/* HMC context dump related defines
   Each context sub-line consists of 128 bits (16 bytes) of data */
#define SUB_LINE_LENGTH         0x10
#define LANCTXCTL_QUEUE_TYPE_TX 0x1
#define LANCTXCTL_QUEUE_TYPE_RX 0x0


/* This is a macro to convert the preprocessor defined version number into a hex value
   that can be registered with EFI. */
#define VERSION_TO_HEX  ((MAJORVERSION << 24) + (MINORVERSION << 16) + \
          (BUILDNUMBER / 10 << 12) + (BUILDNUMBER % 10 << 8))

#define MAX_NIC_INTERFACES 256

#define EFI_NETWORK_INTERFACE_IDENTIFIER_PROTOCOL_REVISION_31 0x00010001
#define PXE_ROMID_MINORVER_31 0x10
#define PXE_STATFLAGS_DB_WRITE_TRUNCATED  0x2000

typedef struct i40e_tx_desc         TRANSMIT_DESCRIPTOR;
typedef union i40e_16byte_rx_desc   RECEIVE_DESCRIPTOR;

#define TX_RING_FROM_ADAPTER(a)   ((TRANSMIT_RING*)(&(a)->Vsi.TxRing))
#define RX_RING_FROM_ADAPTER(a)   ((RECEIVE_RING*)(&(a)->Vsi.RxRing))
#define PCI_IO_FROM_ADAPTER(a)    (EFI_PCI_IO_PROTOCOL*)((a)->PciIo)

//Default TX/RX queue size
#define I40E_DEF_NUM_TX_RX_DESCRIPTORS   16
#define I40E_TOTAL_NUM_TX_RX_DESCRIPTORS 256

// timeout for Tx/Rx queue enable/disable
#define START_RINGS_TIMEOUT 100
#define STOP_RINGS_TIMEOUT 1000

/** Retrieves RX descriptor from RX ring structure

   @param[in]   R   RX ring
   @param[in]   i   Number of descriptor

   @return   Descriptor retrieved
**/
#define I40E_RX_DESC(R, i)          \
          (&(((union i40e_16byte_rx_desc *) (UINTN) ((R)->Mapping.UnmappedAddress))[i]))

/** Retrieves TX descriptor from TX ring structure

   @param[in]   R   TX ring
   @param[in]   i   Number of descriptor

   @return   Descriptor retrieved
**/
#define I40E_TX_DESC(R, i)          \
          (&(((struct i40e_tx_desc *) (UINTN) ((R)->Mapping.UnmappedAddress))[i]))

#define I40E_MAX_PF_NUMBER   16

#define SPIN_LOCK_RELEASED          ((UINTN) 1)
#define SPIN_LOCK_ACQUIRED          ((UINTN) 2)

// Define some handy macros
#define UNDI_DEV_SIGNATURE             SIGNATURE_32 ('P', 'R', '0', '4')

/** Retrieves UNDI_PRIVATE_DATA structure using NII Protocol 3.1 instance

   @param[in]   a   Current protocol instance

   @return    UNDI_PRIVATE_DATA structure instance
**/
#define UNDI_PRIVATE_DATA_FROM_THIS(a)  \
          CR (a, UNDI_PRIVATE_DATA, NiiProtocol31, UNDI_DEV_SIGNATURE)

/** Retrieves UNDI_PRIVATE_DATA structure using DriverStop protocol instance

   @param[in]   a   Current protocol instance

   @return    UNDI_PRIVATE_DATA structure instance
**/
#define UNDI_PRIVATE_DATA_FROM_DRIVER_STOP(a) \
          CR (a, UNDI_PRIVATE_DATA, DriverStop, UNDI_DEV_SIGNATURE)

/** Retrieves UNDI_PRIVATE_DATA structure using AIP protocol instance

   @param[in]   a   Current protocol instance

   @return    UNDI_PRIVATE_DATA structure instance
**/
#define UNDI_PRIVATE_DATA_FROM_AIP(a) \
          CR (a, UNDI_PRIVATE_DATA, AdapterInformation, UNDI_DEV_SIGNATURE)


/** Retrieves UNDI_PRIVATE_DATA structure using BCF handle

   @param[in]   a   Current protocol instance

   @return    UNDI_PRIVATE_DATA structure instance
**/
#define UNDI_PRIVATE_DATA_FROM_BCF_HANDLE(a) \
          CR (a, UNDI_PRIVATE_DATA, NicInfo.BcfHandle, UNDI_DEV_SIGNATURE)

/** Retrieves UNDI_PRIVATE_DATA structure using driver data

   @param[in]   a   Current protocol instance

   @return    UNDI_PRIVATE_DATA structure instance
**/
#define UNDI_PRIVATE_DATA_FROM_DRIVER_DATA(a) \
          CR (a, UNDI_PRIVATE_DATA, NicInfo, UNDI_DEV_SIGNATURE)


/** Macro to convert byte memory requirement into pages

   @param[in]    Bytes of memory required

   @return   Number of pages returned
**/
#define UNDI_MEM_PAGES(x) (((x) - 1) / 4096 + 1)

/** Aligns number to specified granularity

   @param[in]   x   Number
   @param[in]   a   Granularity

   @return   Number aligned to Granularity
 */
#define ALIGN(x, a)    (((x) + ((UINT64) (a) - 1)) & ~((UINT64) (a) - 1))

/** Macro to return the offset of a member within a struct.  This
   looks like it dereferences a null pointer, but it doesn't really.

   @param[in]   Structure    Structure type
   @param[in]   Member       Structure member

   @return   Offset of a member within struct returned
**/
#define STRUCT_OFFSET(Structure, Member)     ((UINTN) &(((Structure *) 0)->Member))

/** Test bit mask against a value.

   @param[in]   v   Value
   @param[in]   m   Mask

   @return    TRUE when value contains whole bit mask, otherwise - FALSE.
 */
#define BIT_TEST(v, m) (((v) & (m)) == (m))

// PCI Base Address Register Bits
#define PCI_BAR_IO_MASK         0x00000003
#define PCI_BAR_IO_MODE         0x00000001

#define PCI_BAR_MEM_MASK        0x0000000F
#define PCI_BAR_MEM_MODE        0x00000000
#define PCI_BAR_MEM_64BIT       0x00000004
#define PCI_BAR_MEM_BASE_ADDR_M 0xFFFFFFF0

#define ETHER_MAC_ADDR_LEN  6

// Definitions for Alternate RAM
#define I40E_ALT_RAM_SIZE_IN_BYTES        8192
#define I40E_ALT_RAM_SIZE_IN_DW           (I40E_ALT_RAM_SIZE_IN_BYTES / 4)
#define I40E_AQ_ALTERNATE_ADDRESS_IGNORE  0xFFFFFFFF

/* Recovery mode defines */
#define I40E_FW_RECOVERY_MODE_CORER         0x30
#define I40E_FW_RECOVERY_MODE_CORER_LEGACY  0x0B
#define I40E_FW_RECOVERY_MODE_GLOBR         0x31
#define I40E_FW_RECOVERY_MODE_GLOBR_LEGACY  0x0C
#define I40E_FW_RECOVERY_MODE_TRANSITION    0x32
#define I40E_FW_RECOVERY_MODE_NVM           0x33

#if (1)
#define I40E_ALT_RAM_LAN_MAC_ADDRESS_LOW(_PF)       (0 + 64 * (_PF))
#define I40E_ALT_RAM_LAN_MAC_ADDRESS_HIGH(_PF)      (1 + 64 * (_PF))
#define I40E_ALT_RAM_SAN_MAC_ADDRESS_LOW(_PF)       (2 + 64 * (_PF))
#define I40E_ALT_RAM_SAN_MAC_ADDRESS_HIGH(_PF)      (3 + 64 * (_PF))
#define I40E_ALT_RAM_WWNN_PREFIX(_PF)               (4 + 64 * (_PF))
#define I40E_ALT_RAM_WWPN_PREFIX(_PF)               (5 + 64 * (_PF))
#define I40E_ALT_RAM_PORT_ENABLE(_PF)               (6 + 64 * (_PF))
#define I40E_ALT_RAM_ADV_LINK_SPEED(_PF)            (7 + 64 * (_PF))
#define I40E_ALT_RAM_RX_FLOW_CONTROL(_PF)           (8 + 64 * (_PF))
#define I40E_ALT_RAM_TX_FLOW_CONTROL(_PF)           (9 + 64 * (_PF))
#define I40E_ALT_RAM_DCC_VLAN(_PF)                  (10 + 64 * (_PF))
#define I40E_ALT_RAM_PF_PROTOCOL(_PF)               (11 + 64 * (_PF))
#define I40E_ALT_RAM_USER_PRIORITY(_PF)             (12 + 64 * (_PF))
#define I40E_ALT_RAM_OUTER_VLAN_TAG(_PF)            (13 + 64 * (_PF))
#define I40E_ALT_RAM_MIN_BW(_PF)                    (14 + 64 * (_PF))
#define I40E_ALT_RAM_MAX_BW(_PF)                    (15 + 64 * (_PF))
#define I40E_ALT_RAM_BOOT(_PF)                      (16 + 64 * (_PF))
#define I40E_ALT_RAM_PF_ENABLE(_PF)                 (17 + 64 * (_PF))
#define I40E_ALT_RAM_SRIOV(_PF)                     (18 + 64 * (_PF))
#define I40E_ALT_RAM_NCSI_STATE(_PF)                (19 + 64 * (_PF))
#define I40E_ALT_RAM_NVM_UPDATE_PROCESS_STATUS(_PF) (27 + 64 * (_PF))
#define I40E_ALT_RAM_BOOT_CONFIGURATION             1280

#define ALT_RAM_VALID_PARAM_BIT_SHIFT           31
#define ALT_RAM_VALID_PARAM_BIT_MASK            (1 << ALT_RAM_VALID_PARAM_BIT_SHIFT)

#define I40E_ALT_RAM_PORT_ENABLE_BIT_SHIFT      0
#define I40E_ALT_RAM_PORT_ENABLE_BIT_MASK       (1 << I40E_ALT_RAM_PORT_ENABLE_BIT_SHIFT)

#define I40E_ALT_RAM_ADV_LINK_SPEED_BIT_SHIFT   0
#define I40E_ALT_RAM_ADV_LINK_SPEED_BIT_MASK    (3 << I40E_ALT_RAM_ADV_LINK_SPEED_BIT_SHIFT)
#define I40E_ALT_RAM_ADV_LINK_SPEED_1G          0
#define I40E_ALT_RAM_ADV_LINK_SPEED_10G         1
#define I40E_ALT_RAM_ADV_LINK_SPEED_20G         2
#define I40E_ALT_RAM_ADV_LINK_SPEED_40G         3

#define I40E_ALT_RAM_RX_FLOW_CTRL_BIT_SHIFT     0
#define I40E_ALT_RAM_RX_FLOW_CTRL_BIT_MASK      (1 << I40E_ALT_RAM_RX_FLOW_CTRL_BIT_SHIFT)

#define I40E_ALT_RAM_TX_FLOW_CTRL_BIT_SHIFT     0
#define I40E_ALT_RAM_TX_FLOW_CTRL_BIT_MASK      (1 << I40E_ALT_RAM_TX_FLOW_CTRL_BIT_SHIFT)

#define I40E_ALT_RAM_DCC_VLAN_BIT_SHIFT         0
#define I40E_ALT_RAM_DCC_VLAN_BIT_MASK          (0x0FFF << I40E_ALT_RAM_DCC_VLAN_BIT_SHIFT)

#define I40E_ALT_RAM_PF_PROTOCOL_BIT_SHIFT      0
#define I40E_ALT_RAM_PF_PROTOCOL_BIT_MASK       (0x7 << I40E_ALT_RAM_PF_PROTOCOL_BIT_SHIFT)
#define I40E_ALT_RAM_PF_NOT_ENUMERATED          0
#define I40E_ALT_RAM_PF_ETHERNET                1
#define I40E_ALT_RAM_PF_ETHERNET_RDMA           2
#define I40E_ALT_RAM_PF_ISCSI_ETHERNET          3
#define I40E_ALT_RAM_PF_FCOE                    4

#define I40E_ALT_RAM_USER_PRIORITY_BIT_SHIFT    0
#define I40E_ALT_RAM_USER_PRIORITY_BIT_MASK     (0x7 << I40E_ALT_RAM_USER_PRIORITY_BIT_SHIFT)

#define I40E_ALT_RAM_OUTER_VLAN_TAG_BIT_SHIFT   0
#define I40E_ALT_RAM_OUTER_VLAN_TAG_BIT_MASK    (0x0FFF << I40E_ALT_RAM_OUTER_VLAN_TAG_BIT_SHIFT)

#define I40E_ALT_RAM_MIN_BW_BIT_SHIFT           0
#define I40E_ALT_RAM_MIN_BW_BIT_MASK            (0x1FF << I40E_ALT_RAM_MIN_BW_BIT_SHIFT)
#define I40E_ALT_RAM_MIN_BW_RELATIVE_BIT_SHIFT  30
#define I40E_ALT_RAM_MIN_BW_RELATIVE_BIT_MASK   (1 << I40E_ALT_RAM_MIN_BW_RELATIVE_BIT_SHIFT)

#define I40E_ALT_RAM_MAX_BW_BIT_SHIFT           0
#define I40E_ALT_RAM_MAX_BW_BIT_MASK            (0x1FF << I40E_ALT_RAM_MAX_BW_BIT_SHIFT)
#define I40E_ALT_RAM_MAX_BW_RELATIVE_BIT_SHIFT  30
#define I40E_ALT_RAM_MAX_BW_RELATIVE_BIT_MASK   (1 << I40E_ALT_RAM_MAX_BW_RELATIVE_BIT_SHIFT)

#define I40E_ALT_RAM_BOOT_ENABLE_BIT_SHIFT      0
#define I40E_ALT_RAM_BOOT_ENABLE_BIT_MASK       (1 << I40E_ALT_RAM_BOOT_ENABLE_BIT_SHIFT)

#define I40E_ALT_RAM_PF_ENABLE_BIT_SHIFT        0
#define I40E_ALT_RAM_PF_ENABLE_BIT_MASK         (1 << I40E_ALT_RAM_PF_ENABLE_BIT_SHIFT)

#define I40E_ALT_RAM_SRIOV_VFS_BIT_SHIFT        0
#define I40E_ALT_RAM_SRIOV_VFS_BIT_MASK         (0xFF << I40E_ALT_RAM_SRIOV_VFS_BIT_SHIFT)
#define I40E_ALT_RAM_SRIOV_ENABLE_BIT_SHIFT     29
#define I40E_ALT_RAM_SRIOV_ENABLE_BIT_MASK      (0x01 << I40E_ALT_RAM_SRIOV_ENABLE_BIT_SHIFT)
#define I40E_ALT_RAM_SRIOV_AUTO_BIT_SHIFT       30
#define I40E_ALT_RAM_SRIOV_AUTO_BIT_MASK        (1 << I40E_ALT_RAM_SRIOV_AUTO_BIT_SHIFT)
#else /* 0 */
#define I40E_EMP_RESERVED_AREA                  32
#define I40E_ALT_RAM_LAN_MAC_ADDRESS_LOW(_PF)   (I40E_EMP_RESERVED_AREA + 7 + 30 * (_PF))
#define I40E_ALT_RAM_LAN_MAC_ADDRESS_HIGH(_PF)  (I40E_EMP_RESERVED_AREA + 8 + 30 * (_PF))
//#define I40E_ALT_RAM_SAN_MAC_ADDRESS_LOW(_PF)   (I40E_EMP_RESERVED_AREA + 2 + 30 * (_PF))
//#define I40E_ALT_RAM_SAN_MAC_ADDRESS_HIGH(_PF)  (I40E_EMP_RESERVED_AREA + 3 + 30 * (_PF))
//#define I40E_ALT_RAM_WWNN_PREFIX(_PF)           (I40E_EMP_RESERVED_AREA + 4 + 30 * (_PF))
//#define I40E_ALT_RAM_WWPN_PREFIX(_PF)           (I40E_EMP_RESERVED_AREA + 5 + 30 * (_PF))
#define I40E_ALT_RAM_PORT_ENABLE(_PF)           (I40E_EMP_RESERVED_AREA + 0 + 30 * (_PF))
#define I40E_ALT_RAM_ADV_LINK_SPEED(_PF)        (I40E_EMP_RESERVED_AREA + 1 + 30 * (_PF))
#define I40E_ALT_RAM_RX_FLOW_CONTROL(_PF)       (I40E_EMP_RESERVED_AREA + 2 + 30 * (_PF))
#define I40E_ALT_RAM_TX_FLOW_CONTROL(_PF)       (I40E_EMP_RESERVED_AREA + 3 + 30 * (_PF))
#define I40E_ALT_RAM_DCC_VLAN(_PF)              (I40E_EMP_RESERVED_AREA + 4 + 30 * (_PF))
#define I40E_ALT_RAM_PF_PROTOCOL(_PF)           (I40E_EMP_RESERVED_AREA + 5 + 30 * (_PF))
#define I40E_ALT_RAM_USER_PRIORITY(_PF)         (I40E_EMP_RESERVED_AREA + 6 + 30 * (_PF))
#define I40E_ALT_RAM_OUTER_VLAN_TAG(_PF)        (I40E_EMP_RESERVED_AREA + 9 + 30 * (_PF))
#define I40E_ALT_RAM_MIN_BW(_PF)                (I40E_EMP_RESERVED_AREA + 10 + 30 * (_PF))
#define I40E_ALT_RAM_MAX_BW(_PF)                (I40E_EMP_RESERVED_AREA + 11 + 30 * (_PF))
#define I40E_ALT_RAM_BOOT(_PF)                  (I40E_EMP_RESERVED_AREA + 12 + 30 * (_PF))
#define I40E_ALT_RAM_PF_ENABLE(_PF)             (I40E_EMP_RESERVED_AREA + 13 + 30 * (_PF))
#define I40E_ALT_RAM_SRIOV(_PF)                 (I40E_EMP_RESERVED_AREA + 14 + 30 * (_PF))
//#define I40E_ALT_RAM_NCSI_STATE(_PF)            (I40E_EMP_RESERVED_AREA + 15 + 30 * (_PF))
#define I40E_ALT_RAM_BOOT_CONFIGURATION         1280

#define ALT_RAM_VALID_PARAM_BIT_SHIFT           31
#define ALT_RAM_VALID_PARAM_BIT_MASK            (1 << ALT_RAM_VALID_PARAM_BIT_SHIFT)

#define I40E_ALT_RAM_PORT_ENABLE_BIT_SHIFT      0
#define I40E_ALT_RAM_PORT_ENABLE_BIT_MASK       (1 << I40E_ALT_RAM_PORT_ENABLE_BIT_SHIFT)

#define I40E_ALT_RAM_ADV_LINK_SPEED_BIT_SHIFT   0
#define I40E_ALT_RAM_ADV_LINK_SPEED_BIT_MASK    (3 << I40E_ALT_RAM_ADV_LINK_SPEED_BIT_SHIFT)
#define I40E_ALT_RAM_ADV_LINK_SPEED_1G          0
#define I40E_ALT_RAM_ADV_LINK_SPEED_10G         1
#define I40E_ALT_RAM_ADV_LINK_SPEED_20G         2
#define I40E_ALT_RAM_ADV_LINK_SPEED_40G         3

#define I40E_ALT_RAM_RX_FLOW_CTRL_BIT_SHIFT     0
#define I40E_ALT_RAM_RX_FLOW_CTRL_BIT_MASK      (1 << I40E_ALT_RAM_RX_FLOW_CTRL_BIT_SHIFT)

#define I40E_ALT_RAM_TX_FLOW_CTRL_BIT_SHIFT     0
#define I40E_ALT_RAM_TX_FLOW_CTRL_BIT_MASK      (1 << I40E_ALT_RAM_TX_FLOW_CTRL_BIT_SHIFT)

#define I40E_ALT_RAM_DCC_VLAN_BIT_SHIFT         0
#define I40E_ALT_RAM_DCC_VLAN_BIT_MASK          (0x0FFF << I40E_ALT_RAM_DCC_VLAN_BIT_SHIFT)

#define I40E_ALT_RAM_PF_PROTOCOL_BIT_SHIFT      0
#define I40E_ALT_RAM_PF_PROTOCOL_BIT_MASK       (0x7 << I40E_ALT_RAM_PF_PROTOCOL_BIT_SHIFT)
#define I40E_ALT_RAM_PF_NOT_ENUMERATED          0
#define I40E_ALT_RAM_PF_ETHERNET                1
#define I40E_ALT_RAM_PF_ETHERNET_RDMA           2
#define I40E_ALT_RAM_PF_ISCSI                   3
#define I40E_ALT_RAM_PF_FCOE                    4

#define I40E_ALT_RAM_USER_PRIORITY_BIT_SHIFT    0
#define I40E_ALT_RAM_USER_PRIORITY_BIT_MASK     (0x7 << I40E_ALT_RAM_USER_PRIORITY_BIT_SHIFT)

#define I40E_ALT_RAM_OUTER_VLAN_TAG_BIT_SHIFT   0
#define I40E_ALT_RAM_OUTER_VLAN_TAG_BIT_MASK    (0x0FFF << I40E_ALT_RAM_OUTER_VLAN_TAG_BIT_SHIFT)

#define I40E_ALT_RAM_MIN_BW_BIT_SHIFT           0
#define I40E_ALT_RAM_MIN_BW_BIT_MASK            (0x1F << I40E_ALT_RAM_MIN_BW_BIT_SHIFT)
#define I40E_ALT_RAM_MIN_BW_RELATIVE_BIT_SHIFT  30
#define I40E_ALT_RAM_MIN_BW_RELATIVE_BIT_MASK   (1 << I40E_ALT_RAM_MIN_BW_RELATIVE_BIT_SHIFT)

#define I40E_ALT_RAM_MAX_BW_BIT_SHIFT           0
#define I40E_ALT_RAM_MAX_BW_BIT_MASK            (0x1F << I40E_ALT_RAM_MAX_BW_BIT_SHIFT)
#define I40E_ALT_RAM_MAX_BW_RELATIVE_BIT_SHIFT  30
#define I40E_ALT_RAM_MAX_BW_RELATIVE_BIT_MASK   (1 << I40E_ALT_RAM_MAX_BW_RELATIVE_BIT_SHIFT)

#define I40E_ALT_RAM_BOOT_ENABLE_BIT_SHIFT      0
#define I40E_ALT_RAM_BOOT_ENABLE_BIT_MASK       (1 << I40E_ALT_RAM_BOOT_ENABLE_BIT_SHIFT)

#define I40E_ALT_RAM_PF_ENABLE_BIT_SHIFT        0
#define I40E_ALT_RAM_PF_ENABLE_BIT_MASK         (1 << I40E_ALT_RAM_PF_ENABLE_BIT_SHIFT)

#define I40E_ALT_RAM_SRIOV_VFS_BIT_SHIFT        0
#define I40E_ALT_RAM_SRIOV_VFS_BIT_MASK         (0xFF << I40E_ALT_RAM_SRIOV_VFS_BIT_SHIFT)
#define I40E_ALT_RAM_SRIOV_AUTO_BIT_SHIFT       30
#define I40E_ALT_RAM_SRIOV_AUTO_BIT_MASK        (1 << I40E_ALT_RAM_SRIOV_AUTO_BIT_SHIFT)
#endif /* 1 */

#if (1)
#define I40E_PRTDCB_TC2PFC_RCB                  0x00122140 /* Reset: CORER */
#define I40E_PRTDCB_TC2PFC_RCB_TC2PFC_SHIFT     0
#define I40E_PRTDCB_TC2PFC_RCB_TC2PFC_MASK      I40E_MASK(0xFF, I40E_PRTDCB_TC2PFC_RCB_TC2PFC_SHIFT)
#endif

/** Helper for controller private data for() iteration loop.

   @param[in]    Iter   Driver private data iteration pointer
*/
#define FOREACH_ACTIVE_CONTROLLER(Iter)                 \
  for ((Iter) = GetFirstControllerPrivateData ();       \
       (Iter) != NULL;                                  \
       (Iter) = GetNextControllerPrivateData ((Iter)))

/* Function and structure typedefs */

typedef struct {
  EFI_NETWORK_INTERFACE_IDENTIFIER_PROTOCOL *NiiProtocol31;
} EFI_NII_POINTER_PROTOCOL;

/* UNDI callback functions typedefs */
typedef
VOID
(EFIAPI * PTR) (
  VOID
  );

typedef
VOID
(EFIAPI * BS_PTR_30) (
  UINTN   MicroSeconds
  );

typedef
VOID
(EFIAPI * VIRT_PHYS_30) (
  UINT64   VirtualAddr,
  UINT64   PhysicalPtr
  );

typedef
VOID
(EFIAPI * BLOCK_30) (
  UINT32   Enable
  );

typedef
VOID
(EFIAPI * MEM_IO_30) (
  UINT8   ReadWrite,
  UINT8   Len,
  UINT64  Port,
  UINT64  BufAddr
  );

typedef
VOID
(EFIAPI * BS_PTR) (
  UINT64  UnqId,
  UINTN   MicroSeconds
  );

typedef
VOID
(EFIAPI * VIRT_PHYS) (
  UINT64  UnqId,
  UINT64  VirtualAddr,
  UINT64  PhysicalPtr
  );

typedef
VOID
(EFIAPI * BLOCK) (
  UINT64  UnqId,
  UINT32  Enable
  );

typedef
VOID
(EFIAPI * MEM_IO) (
  UINT64  UnqId,
  UINT8   ReadWrite,
  UINT8   Len,
  UINT64  Port,
  UINT64  BufAddr
  );

typedef
VOID
(EFIAPI * MAP_MEM) (
  UINT64  UnqId,
  UINT64  VirtualAddr,
  UINT32  Size,
  UINT32  Direction,
  UINT64  MappedAddr
  );

typedef
VOID
(EFIAPI * UNMAP_MEM) (
  UINT64  UnqId,
  UINT64  VirtualAddr,
  UINT32  Size,
  UINT32  Direction,
  UINT64  MappedAddr
  );

typedef
VOID
(EFIAPI * SYNC_MEM) (
  UINT64  UnqId,
  UINT64  VirtualAddr,
  UINT32  Size,
  UINT32  Direction,
  UINT64  MappedAddr
  );


typedef struct {
  UINT16 CpbSize;
  UINT16 DbSize;
  UINT16 OpFlags;

// UNDI_CALL_TABLE.State can have the following values
#define DONT_CHECK -1
#define ANY_STATE -1
#define MUST_BE_STARTED 1
#define MUST_BE_INITIALIZED 2
  UINT16 State;
  VOID (*ApiPtr)();
} UNDI_CALL_TABLE;

/* External Variables */
extern EFI_DRIVER_DIAGNOSTICS_PROTOCOL           gUndiDriverDiagnostics;
extern EFI_DRIVER_DIAGNOSTICS2_PROTOCOL          gUndiDriverDiagnostics2;
extern EFI_DRIVER_STOP_PROTOCOL                  gUndiDriverStop;
extern EFI_DRIVER_SUPPORTED_EFI_VERSION_PROTOCOL gUndiSupportedEfiVersion;
extern EFI_DRIVER_HEALTH_PROTOCOL                gUndiDriverHealthProtocol;
extern EFI_COMPONENT_NAME2_PROTOCOL              gUndiComponentName2;
extern EFI_COMPONENT_NAME_PROTOCOL               gUndiComponentName;
extern EFI_DRIVER_BINDING_PROTOCOL               gUndiDriverBinding;
extern EFI_GUID                                  gEfiStartStopProtocolGuid;

extern EFI_GUID                    gEfiNiiPointerGuid;

extern PXE_SW_UNDI            *mPxe31;
extern UNDI_PRIVATE_DATA      *mUndi32DeviceList[MAX_NIC_INTERFACES];
extern BOOLEAN                mExitBootServicesTriggered;

extern UINT8 I40eUndiDxeStrings[];

#pragma pack(1)
typedef struct {
  UINT8  DestAddr[PXE_HWADDR_LEN_ETHER];
  UINT8  SrcAddr[PXE_HWADDR_LEN_ETHER];
  UINT16 Type;
} ETHER_HEADER;

#pragma pack(1)
typedef struct {
  UINT16 VendorId;
  UINT16 DeviceId;
  UINT16 Command;
  UINT16 Status;
  UINT8  RevId;
  UINT8  ClassIdProgIf;
  UINT8  ClassIdSubclass;
  UINT8  ClassIdMain;
  UINT8  CacheLineSize;
  UINT8  LatencyTimer;
  UINT8  HeaderType;
  UINT8  Bist;
  UINT32 BaseAddressReg0;
  UINT32 BaseAddressReg1;
  UINT32 BaseAddressReg2;
  UINT32 BaseAddressReg3;
  UINT32 BaseAddressReg4;
  UINT32 BaseAddressReg5;
  UINT32 CardBusCisPtr;
  UINT16 SubVendorId;
  UINT16 SubSystemId;
  UINT32 ExpansionRomBaseAddr;
  UINT8  CapabilitiesPtr;
  UINT8  Reserved1;
  UINT16 Reserved2;
  UINT32 Reserved3;
  UINT8  IntLine;
  UINT8  IntPin;
  UINT8  MinGnt;
  UINT8  MaxLat;
} PCI_CONFIG_HEADER;
#pragma pack()

// Supported Rx Buffer Sizes
#define I40E_RXBUFFER_512   512    /* Used for packet split */
#define I40E_RXBUFFER_2048  2048
#define I40E_RXBUFFER_3072  3072   /* For FCoE MTU of 2158 */
#define I40E_RXBUFFER_4096  4096
#define I40E_RXBUFFER_8192  8192
#define I40E_MAX_RXBUFFER   16384  /* largest size for single descriptor */

typedef struct {
  UINT64 Packets;
  UINT64 Bytes;
  UINT64 RestartQueue;
  UINT64 TxBusy;
  UINT64 Completed;
  UINT64 TxDoneOld;
} I40E_TX_QUEUE_STATS;

typedef struct {
  UINT64 Packets;
  UINT64 Bytes;
  UINT64 NonEopDescs;
  UINT64 AllocRxPageFailed;
  UINT64 AllocRxBuffFailed;
} I40E_RX_QUEUE_STATS;

#define I40E_RX_DTYPE_NO_SPLIT      0
#define I40E_RX_DTYPE_SPLIT_ALWAYS  1
#define I40E_RX_DTYPE_HEADER_SPLIT  2

#define I40E_RX_SPLIT_L2            0x1
#define I40E_RX_SPLIT_IP            0x2
#define I40E_RX_SPLIT_TCP_UDP       0x4
#define I40E_RX_SPLIT_SCTP          0x8

typedef struct {
  UINT16 Length;
  UINT8  McAddr[MAX_MCAST_ADDRESS_CNT][PXE_MAC_LENGTH];
} MCAST_LIST;


/* struct that defines a VSI */
typedef struct {

  UINT16     Flags;

  SPIN_LOCK  MacFilterLock;

  MCAST_LIST CurrentMcastList;
  MCAST_LIST McastListToProgram;

  BOOLEAN    EnablePromiscuous;
  BOOLEAN    EnableBroadcast;
  BOOLEAN    EnableMulticastPromiscuous;

  // Tx Rx rings
  TRANSMIT_RING                       TxRing;
  RECEIVE_RING                        RxRing;

  UINT16                              Seid; /* HW index of this VSI (absolute index) */
  UINT16                              Id; /* VSI number */

  UINT16                              BaseQueue; /* vsi's first queue in hw array */
  UINT16                              NumQueuePairs; /* tx and rx pairs */
  UINT16                              NumDesc;
  enum i40e_vsi_type                  Type; /* VSI type, e.g., LAN, FCoE, etc */

  struct i40e_aqc_vsi_properties_data Info;
} I40E_VSI;


typedef struct DRIVER_DATA_S {
  UINT16                    State;  // stopped, started or initialized
  struct i40e_hw            Hw;
  struct i40e_hw_port_stats Stats;
  I40E_VSI                  Vsi;

  UINTN                     Segment;
  UINTN                     Bus;
  UINTN                     Device;
  UINTN                     Function;

  UINT8                     PciClass;
  UINT8                     PciSubClass;
  UINT8                     PciClassProgIf;

  UINTN                     PhysicalPortNumber;
  UINT8                     PfPerPortMaxNumber;
  BOOLEAN                   PartitionEnabled[I40E_MAX_PF_NUMBER];  // both valid only for those enabled
  UINT8                     PartitionPfNumber[I40E_MAX_PF_NUMBER]; // on port managed by current PF

  UINT8                     BroadcastNodeAddress[PXE_MAC_LENGTH];

  UINT32                    PciConfig[MAX_PCI_CONFIG_LEN];

  UINTN                     HwReset;
  UINTN                     HwInitialized;
  UINTN                     DriverBusy;
  UINT16                    LinkSpeed;     // requested (forced) link speed
  UINT8                     DuplexMode;     // requested duplex
  UINT8                     CableDetect;    // 1 to detect and 0 not to detect the cable
  UINT8                     LoopBack;

  UINT8                     UndiEnabled;        // When false only configuration protocols are avaliable
                                                // (e.g. iSCSI driver loaded on port)
  UINT8                     FwSupported; // FW is not supported, AQ operations are prohibited

  BOOLEAN                   MediaStatusChecked;
  BOOLEAN                   LastMediaStatus;
  BOOLEAN                   WaitingForLinkUp;


  UINT64                    UniqueId;
  EFI_PCI_IO_PROTOCOL      *PciIo;
  UINT64                    OriginalPciAttributes;

  BOOLEAN                   NvmAcquired; // Field specific for NUL semaphore management.
  BOOLEAN                   RepeatedFwResets;
  BOOLEAN                   RecoveryMode;

  // UNDI callbacks
  BS_PTR_30            Delay30;
  VIRT_PHYS_30         Virt2Phys30;
  BLOCK_30             Block30;
  MEM_IO_30            MemIo30;

  BS_PTR               Delay;
  VIRT_PHYS            Virt2Phys;
  BLOCK                Block;
  MEM_IO               MemIo;
  MAP_MEM              MapMem;
  UNMAP_MEM            UnMapMem;
  SYNC_MEM             SyncMem;

  UINT64 MemoryPtr;
  UINT32 MemoryLength;

  UINT16 RxFilter;
  UINT16 IntMask;
  UINT32 IntStatus;

  UINT16 CurRxInd;
  UINT16 CurTxInd;

  UINT16 NumLanQps;        /* num lan queues this pf has set up */

  /* switch config info */
  UINT16             PfSeid;
  UINT16             MainVsiSeid;
  UINT16             VebSeid;
  UINT16             MacSeid;

  UINT16             XmitDoneHead;
  BOOLEAN            MacAddrOverride;
  UINTN              VersionFlag; // Indicates UNDI version 3.0 or 3.1
  UINT16 TxRxDescriptorCount;

} DRIVER_DATA;

typedef struct HII_INFO_S {
  EFI_HANDLE                       HiiInstallHandle;
  EFI_HII_HANDLE                   HiiPkgListHandle;
  HII_CFG_ACCESS_INFO              HiiCfgAccessInfo;

  BOOLEAN    AltMacAddrSupported;
} HII_INFO;

typedef struct UNDI_PRIVATE_DATA_S {
  UINTN                                     Signature;
  UINTN                                     IfId;
  EFI_NETWORK_INTERFACE_IDENTIFIER_PROTOCOL NiiProtocol31;
  EFI_NII_POINTER_PROTOCOL                  NiiPointerProtocol;
  EFI_HANDLE                                ControllerHandle;
  EFI_HANDLE                                DeviceHandle;
  EFI_HANDLE                                FmpInstallHandle;
  EFI_DEVICE_PATH_PROTOCOL                 *Undi32BaseDevPath;
  EFI_DEVICE_PATH_PROTOCOL                 *Undi32DevPath;
  DRIVER_DATA                               NicInfo;
  CHAR16                                   *Brand;
  EFI_UNICODE_STRING_TABLE                 *ControllerNameTable;

  HII_INFO                                 HiiInfo;


  EFI_DRIVER_STOP_PROTOCOL         DriverStop;
  EFI_ADAPTER_INFORMATION_PROTOCOL AdapterInformation;

  BOOLEAN                                   IsChildInitialized;

  UINT32                                    LastAttemptVersion;
  UINT32                                    LastAttemptStatus;
} UNDI_PRIVATE_DATA;

/* Function declarations */

/** This function performs PCI-E initialization for the device.

   @param[]   AdapterInfo   Pointer to the NIC data structure information
                             the UNDI driver is layering on

   @retval   EFI_SUCCESS            PCI-E initialized successfully
   @retval   EFI_INVALID_PARAMETER  Failed to get original PCI attributes to save locally
   @retval   EFI_UNSUPPORTED        Failed to get original PCI attributes to save locally
   @retval   EFI_INVALID_PARAMETER  Failed to get supported PCI command options
   @retval   EFI_UNSUPPORTED        Failed to get supported PCI command options
   @retval   EFI_INVALID_PARAMETER  Failed to set PCI command options
   @retval   EFI_UNSUPPORTED        Failed to set PCI command options
**/
EFI_STATUS
I40ePciInit (
  IN DRIVER_DATA *AdapterInfo
  );

/** Performs HW initialization from child side

   Initializes HMC structure, sets flow control, setups PF switch,
   setups and configures Tx/Rx resources and queues, enables Tx/Rx rings

   @param[in]   AdapterInfo   Pointer to the NIC data structure information
                              the UNDI driver is layering on

   @retval    EFI_SUCCESS       HW initialized successfully
   @retval    EFI_DEVICE_ERROR  Failed to initialize HMC structure for LAN function
   @retval    EFI_DEVICE_ERROR  Failed to configure HMC
   @retval    EFI_DEVICE_ERROR  Failed to setup PF switch
   @retval    EFI_OUT_OF_RESOURCES  Failed to setup Tx/Rx resources
   @retval    EFI_DEVICE_ERROR  Failed to configure Tx/Rx queues
   @retval    EFI_OUT_OF_RESOURCES  Failed to configure Tx/Rx queues
**/
EFI_STATUS
I40eInitHw (
  IN DRIVER_DATA *AdapterInfo
  );

/** Performs I40eInitHw function for UNDI interface

   @param[in]   AdapterInfo   Pointer to the NIC data structure information
                             the UNDI driver is layering on

   @retval    PXE_STATCODE_SUCCESS   HW initialized successfully
   @retval    PXE_STATCODE_NOT_STARTED  Failed to initialize HW
**/
PXE_STATCODE
I40eInitialize (
  IN DRIVER_DATA *AdapterInfo
  );

/** Reverts the operations performed in I40eInitHw. Stops HW from child side

   @param[in]   AdapterInfo   Pointer to the NIC data structure information
                              the UNDI driver is layering on

   @retval   PXE_STATCODE_SUCCESS   HW is already not initialized
   @retval   PXE_STATCODE_SUCCESS   HW successfully stopped
**/
PXE_STATCODE
I40eShutdown (
  IN DRIVER_DATA *AdapterInfo
  );

/** Performs HW reset by reinitialization

   @param[in]   AdapterInfo   Pointer to the NIC data structure information
                              the UNDI driver is layering on

   @retval   PXE_STATCODE_SUCCESS      Successfull HW reset
   @retval   PXE_STATCODE_NOT_STARTED  Failed to initialize HW
**/
PXE_STATCODE
I40eReset (
  IN DRIVER_DATA *AdapterInfo
  );

/** Configures internal interrupt causes on current PF.

   @param[in]   AdapterInfo  Pointer to the NIC data structure information which
                             the UNDI driver is layering on

   @return   Interrupt causes are configured for current PF
**/
VOID
I40eConfigureInterrupts (
  IN DRIVER_DATA *AdapterInfo
  );

/** Disables internal interrupt causes on current PF.

   @param[in]   AdapterInfo  Pointer to the NIC data structure information which
                             the UNDI driver is layering on

   @return   Interrupt causes are disabled for current PF
**/
VOID
I40eDisableInterrupts (
  IN DRIVER_DATA *AdapterInfo
  );

/** Free TX buffers that have been transmitted by the hardware.

  @param[in]   AdapterInfo  Pointer to the NIC data structure information which
                           the UNDI driver is layering on.
  @param[in]   NumEntries   Number of entries in the array which can be freed.
  @param[out]  TxBuffer     Array to pass back free TX buffer

  @return      Number of TX buffers written.
**/
UINT16
I40eFreeTxBuffers (
  IN  DRIVER_DATA *AdapterInfo,
  IN  UINT16       NumEntries,
  OUT UINT64      *TxBuffer
  );

#define PCI_CLASS_MASK          0xFF00
#define PCI_SUBCLASS_MASK       0x00FF

/** This function is called as early as possible during driver start to ensure the
   hardware has enough time to autonegotiate when the real SNP device initialize call
   is made.

   @param[in]   AdapterInfo   Pointer to the NIC data structure information
                              the UNDI driver is layering on

   @retval   EFI_SUCCESS            First time init end up successfully
   @retval   EFI_INVALID_PARAMETER  Firmware version is newer than expected
   @retval   EFI_DEVICE_ERROR       Failed to init shared code
   @retval   EFI_DEVICE_ERROR       PF reset failed
   @retval   EFI_DEVICE_ERROR       Init Admin Queue failed
   @retval   EFI_NOT_FOUND          Failed reading MFP configuration
   @retval   EFI_DEVICE_ERROR       Failed reading MFP configuration
   @retval   EFI_INVALID_PARAMETER  Failed to discover (read) capabilities
   @retval   EFI_OUT_OF_RESOURCES   Failed to discover (read) capabilities
   @retval   EFI_DEVICE_ERROR       Failed to discover (read) capabilities
   @retval   EFI_DEVICE_ERROR       Failed to read MAC address
   @retval   EFI_ACCESS_DENIED      UNDI is not enabled
**/
EFI_STATUS
I40eFirstTimeInit (
  IN DRIVER_DATA *AdapterInfo
  );

#define IOADDR 0x98
#define IODATA 0x9C

/** This function calls the MemIo callback to read a dword from the device's
   address space

   @param[in]   AdapterInfo   Pointer to the NIC data structure information
                             the UNDI driver is layering on
   @param[in]   Port         Address to read from

   @return      The data read from the port.
**/
UINT32
I40eRead32 (
  IN DRIVER_DATA *AdapterInfo,
  IN UINT32       Port
  );

/** This function calls the MemIo callback to write a word from the device's
   address space

   @param[in]   AdapterInfo   Pointer to the NIC data structure information
                             the UNDI driver is layering on
   @param[in]   Port         Address to write to
   @param[in]   Data         Data to write to Port

   @return    Data written to address in device's space
**/
VOID
I40eWrite32 (
  IN DRIVER_DATA *AdapterInfo,
  IN UINT32       Port,
  IN UINT32       Data
  );

/** Copies the frame from one of the Rx buffers to the command block
  passed in as part of the cpb parameter.

  The flow:  Ack the interrupt, setup the pointers, find where the last
  block copied is, check to make sure we have actually received something,
  and if we have then we do a lot of work. The packet is checked for errors,
  adjust the amount to copy if the buffer is smaller than the packet,
  copy the packet to the EFI buffer, and then figure out if the packet was
  targetted at us, broadcast, multicast or if we are all promiscuous.
  We then put some of the more interesting information (protocol, src and dest
  from the packet) into the db that is passed to us.  Finally we clean up
  the frame, set the return value to _SUCCESS, and inc the index, watching
  for wrapping.  Then with all the loose ends nicely wrapped up,
  fade to black and return.

  @param[in]  AdapterInfo  Pointer to the NIC data structure information which
                           the UNDI driver is layering on
  @param[in]  CpbReceive  Pointer (Ia-64 friendly) to the command parameter block.
                          The frame will be placed inside of it.
  @param[in]  DbReceive   The data buffer.  The out of band method of passing
                          pre-digested information to the protocol.

  @retval     PXE_STATCODE_NO_DATA        There is no data to receive.
  @retval     PXE_STATCODE_DEVICE_FAILURE AdapterInfo is NULL.
  @retval     PXE_STATCODE_DEVICE_FAILURE Device failure on packet receive.
  @retval     PXE_STATCODE_INVALID_CPB    Invalid CPB/DB parameters.
  @retval     PXE_STATCODE_NOT_STARTED    Rx queue not started.
  @retval     PXE_STATCODE_SUCCESS        Received data passed to the protocol.
**/
UINTN
I40eReceive (
  IN DRIVER_DATA     *AdapterInfo,
  IN PXE_CPB_RECEIVE *CpbReceive,
  IN PXE_DB_RECEIVE  *DbReceive
  );

/** Takes a command block pointer (cpb) and sends the frame.

  Takes either one fragment or many and places them onto the wire.
  Cleanup of the send happens in the function UNDI_Status in Decode.c

  @param[in]  AdapterInfo   Pointer to the NIC data structure information
                             the UNDI driver is layering on
  @param[in]  Cpb           The command parameter block address.
                            64 bits since this is Itanium(tm) processor friendly
  @param[in]  OpFlags       The operation flags, tells if there is any special
                            sauce on this transmit

  @retval     PXE_STATCODE_SUCCESS          Packet enqueued for transmit.
  @retval     PXE_STATCODE_DEVICE_FAILURE   AdapterInfo parameter is NULL.
  @retval     PXE_STATCODE_DEVICE_FAILURE   Failed to send packet.
  @retval     PXE_STATCODE_INVALID_CPB      CPB invalid.
  @retval     PXE_STATCODE_UNSUPPORTED      Fragmented tranmission was requested.
  @retval     PXE_STATCODE_QUEUE_FULL       Tx queue is full.
**/
UINTN
I40eTransmit (
  IN DRIVER_DATA *AdapterInfo,
  IN UINT64       Cpb,
  IN UINT16       OpFlags
  );

/** Sets receive filters.

  @param[in]  AdapterInfo  Pointer to the adapter structure
  @param[in]  NewFilter    A PXE_OPFLAGS bit field indicating what filters to use.

  @return     Broad/Multicast and promiscous settings are set according to NewFilter
**/
VOID
I40eSetFilter (
  IN DRIVER_DATA *AdapterInfo,
  IN UINT16       NewFilter
  );

/** Clears receive filters.

  @param[in]  AdapterInfo  Pointer to the adapter structure
  @param[in]  NewFilter    A PXE_OPFLAGS bit field indicating what filters to clear.

  @return     Broad/Multicast and promiscous settings are cleared according to NewFilter
**/
VOID
I40eClearFilter (
  IN DRIVER_DATA *AdapterInfo,
  IN UINT16       NewFilter
  );

/** Adds MAC/VLAN elements to multicast list

   @param[in]   AdapterInfo   Pointer to the NIC data structure information
                              the UNDI driver is layering on

   @return  MAC/VLAN elements from adapter VSI structure are added to list
**/
VOID
I40eSetMcastList (
  IN DRIVER_DATA *AdapterInfo
  );

/** Checks if Firmware is in recovery mode.

   @param[in]   AdapterInfo  Pointer to the NIC data structure information which
                             the UNDI driver is layering on

   @retval   TRUE   Firmware is in recovery mode
   @retval   FALSE  Firmware is not in recovery mode
**/
BOOLEAN
IsRecoveryMode (
  IN DRIVER_DATA *AdapterInfo
  );

/** Gets information on current link up/down status.

   @param[in]   UndiPrivateData  Pointer to driver private data structure
   @param[out]  LinkUp           Link up/down status

   @retval  EFI_SUCCESS   Links status retrieved successfully
   @retval  !EFI_SUCCESS  Underlying function failure
**/
EFI_STATUS
GetLinkStatus (
  IN   UNDI_PRIVATE_DATA  *UndiPrivateData,
  OUT  BOOLEAN            *LinkUp
  );

/** Gets link speed capability

   @param[in]    AdapterInfo        Pointer to the NIC data structure information which
                                    the UNDI driver is layering on
   @param[out]   AllowedLinkSpeeds  Pointer to resulting link spedd capability

   @retval    EFI_SUCCESS       Link speed capability setting successfully read
   @retval    EFI_DEVICE_ERROR  Get phy capabilities AQ cmd failed
**/
EFI_STATUS
GetLinkSpeedCapability (
  IN  DRIVER_DATA *AdapterInfo,
  OUT UINT8       *AllowedLinkSpeeds
  );

/** Gets link speed setting for adapter.

   @param[in]   UndiPrivateData   Pointer to driver private data structure
   @param[out]  LinkSpeed         Link speed setting

   @retval      EFI_SUCCESS       Successfull operation
**/
EFI_STATUS
GetLinkSpeed (
  IN   UNDI_PRIVATE_DATA  *UndiPrivateData,
  OUT  UINT8              *LinkSpeed
  );

/** Sets link speed setting for adapter (unsupported).

   @param[in]   UndiPrivateData  Pointer to driver private data structure
   @param[in]   LinkSpeed        Lan speed setting - unused

   @retval      EFI_SUCCESS      Successfull operation
**/
EFI_STATUS
SetLinkSpeed (
  IN  UNDI_PRIVATE_DATA  *UndiPrivateData,
  IN  UINT8              *LinkSpeed
  );

/** Returns information whether Link Speed attribute is supported.

   @param[in]   UndiPrivateData     Pointer to driver private data structure
   @param[out]  LinkSpeedSupported  BOOLEAN value describing support

   @retval      EFI_SUCCESS         Successfull operation
**/
EFI_STATUS
IsLinkSpeedSupported (
  IN  UNDI_PRIVATE_DATA  *UndiPrivateData,
  OUT BOOLEAN            *LinkSpeedSupported
  );

/** Returns information whether Link Speed attribute is modifiable.

   @param[in]   UndiPrivateData      Pointer to driver private data structure
   @param[out]  LinkSpeedModifiable  BOOLEAN value describing support

   @retval      EFI_SUCCESS          Successfull operation
**/
EFI_STATUS
IsLinkSpeedModifiable (
  IN  UNDI_PRIVATE_DATA  *UndiPrivateData,
  OUT BOOLEAN            *LinkSpeedModifiable
  );




/** Blinks LEDs on port managed by current PF.

   @param[in]   UndiPrivateData  Pointer to driver private data structure
   @param[in]   Time             Time in seconds to blink

   @retval  EFI_SUCCESS       LEDs blinked successfully
   @retval  EFI_DEVICE_ERROR  Failed to blink PHY link LED
**/
EFI_STATUS
BlinkLeds (
  IN  UNDI_PRIVATE_DATA  *UndiPrivateData,
  IN  UINT16             *Time
  );

/** Gets FVL chip type string.

   @param[in]   UndiPrivateData   Points to the driver instance private data
   @param[out]  ChipTypeStr       Points to the output string buffer

   @retval   EFI_SUCCESS   Chip type string successfully retrieved
**/
EFI_STATUS
GetChipTypeStr (
  IN  UNDI_PRIVATE_DATA  *UndiPrivateData,
  OUT EFI_STRING         ChipTypeStr
  );

/** Gets HII formset help string ID.

   @param[in]   UndiPrivateData  Pointer to driver private data structure

   @return   EFI_STRING_ID of formset help string
**/
EFI_STRING_ID
GetFormSetHelpStringId (
  IN UNDI_PRIVATE_DATA  *UndiPrivateData
  );

/** Checks if alternate MAC address is supported.

   @param[in]   UndiPrivateData    Driver instance private data structure
   @param[out]  AltMacSupport      Tells if alternate mac address is supported

   @retval   EFI_SUCCESS    Alternate MAC address is always supported
**/
EFI_STATUS
GetAltMacAddressSupport (
  IN   UNDI_PRIVATE_DATA  *UndiPrivateData,
  OUT  BOOLEAN            *AltMacSupport
  );

/** This functions initialize table containing PF numbers for partitions
   related to this port.

   @param[in]   AdapterInfo   Pointer to the NIC data structure information
                              which the UNDI driver is layering on..

   @retval      EFI_SUCCESS       PF/partition relations saved successfully.
   @retval      EFI_NOT_FOUND     EepromGetMaxPfPerPortNumber failed
   @retval      EFI_DEVICE_ERROR  EepromGetMaxPfPerPortNumber failed
**/
EFI_STATUS
ReadMfpConfiguration (
  IN DRIVER_DATA *AdapterInfo
  );

/** Read VSI parameters

  @param[in]    AdapterInfo         Pointer to the NIC data structure information
                                    which the UNDI driver is layerin

  @param[out]   VsiCtx             resulting VSI context

  @retval       EFI_SUCCESS         VSI context successfully read
  @retval       EFI_DEVICE_ERROR    VSI context read error
**/
EFI_STATUS
I40eGetVsiParams (
  IN  DRIVER_DATA             *AdapterInfo,
  OUT struct i40e_vsi_context *VsiCtx
  );

#define I40E_GLNVM_ULD_TIMEOUT    1000000

/** Checks if reset is done.

   @param[in]   AdapterInfo   Pointer to the NIC data structure information
                              the UNDI driver is layering on
   @param[in]   ResetMask     Mask to compare with read reg. value if reset was done

   @retval    EFI_SUCCESS       Function ended successfully
   @retval    EFI_DEVICE_ERROR  Timeout when waiting for device to become active
   @retval    EFI_DEVICE_ERROR  Timeout when waiting for reset done
**/
EFI_STATUS
I40eCheckResetDone (
  IN DRIVER_DATA *AdapterInfo,
  IN UINT32       ResetMask
  );


/** This function checks if any  other instance of driver is loaded on this PF by
   reading PFGEN_DRUN register.

   If not it writes the bit in the register to let know other components that
   the PF is in use.

   @param[in]   AdapterInfo   Pointer to the NIC data structure information
                             the UNDI driver is layering on

   @retval   TRUE   The PF is free to use for Tx/Rx
   @retval   FALSE  The PF cannot be used for Tx/Rx
**/
BOOLEAN
I40eAquireControllerHw (
  IN DRIVER_DATA *AdapterInfo
  );

/** Release this PF by clearing the bit in PFGEN_DRUN register.

   @param[in]   AdapterInfo   Pointer to the NIC data structure information
                             the UNDI driver is layering on

   @return   PFGEN_DRUN driver unload bit is cleared
**/
VOID
I40eReleaseControllerHw (
  IN DRIVER_DATA *AdapterInfo
  );

/** Reads and prints adapter MAC address

   @param[in]   AdapterInfo   Pointer to the NIC data structure information
                             the UNDI driver is layering on

   @retval    EFI_SUCCESS       MAC address read successfully
   @retval    EFI_DEVICE_ERROR  Failed to get MAC address
**/
EFI_STATUS
I40eReadMacAddress (
  IN DRIVER_DATA *AdapterInfo
  );



/** Stop all drivers managing the current adapter accept the calling instance of driver

   @param[in]   UndiPrivateData   Points to the driver instance private data.
   @param[in]   StartDrivers      Flag to choose between start/stop PF

   @retval    EFI_SUCCESS     PFs stopped successfully
   @retval    EFI_SUCCESS     No driver instances found to be stoped
   @retval    EFI_OUT_OF_RESOURCES   Failed to find DriverStop protocol instance
   @retval    EFI_NOT_FOUND   Failed to find NII pointer protocol instance
   @retval    EFI_OUT_OF_RESOURCES   Failed to find NII pointer protocol instance
   @retval    EFI_UNSUPPORTED   Testing child handle failed
   @retval    EFI_ACCESS_DENIED  Failed to open PCI IO Protocol
   @retval    EFI_ALREADY_STARTED  Failed to open PCI IO Protocol
   @retval    EFI_UNSUPPORTED  Failed to open PCI IO Protocol
   @retval    EFI_ACCESS_DENIED  Failed to open DriverStop Protocol
   @retval    EFI_ALREADY_STARTED  Failed to open DriverStop Protocol
   @retval    EFI_UNSUPPORTED  Failed to open DriverStop Protocol
**/
EFI_STATUS
StartStopRemainingPFsOnAdapter (
  IN UNDI_PRIVATE_DATA *UndiPrivateData,
  IN BOOLEAN            StartDrivers
  );

/** Get supported Tx/Rx descriptor count for a given device.

   @param[in]    Hw         Pointer to the HW Structure

   @return       Supported Tx/RX descriptors count
**/
UINT16
I40eGetTxRxDescriptorsCount (
  IN struct i40e_hw *Hw
  );

/** Checks if Firmware is in lockdown state.

   @param[in]   AdapterInfo  Pointer to the NIC data structure information which
                             the UNDI driver is layering on

   @retval   TRUE   Firmware is in lockdown state
   @retval   FALSE  Firmware is not in lockdown state
   @retval   FALSE  Invalid input parameter
**/
BOOLEAN
IsFwLockdownState (
  IN DRIVER_DATA *AdapterInfo
  );

/** Read function capabilities using AQ command.

   @param[in]   AdapterInfo   Pointer to the NIC data structure information
                             the UNDI driver is layering on

   @retval   EFI_INVALID_PARAMETER   Failed to allocate memory for function
                                     capabilities buffer
   @retval   EFI_OUT_OF_RESOURCES    Failed to allocate memory for function
                                     capabilities buffer
   @retval   EFI_DEVICE_ERROR        Discover capabilities AQ cmd failed
**/
EFI_STATUS
I40eDiscoverCapabilities (
  IN DRIVER_DATA *AdapterInfo
  );

/** Iteration helper. Get first controller private data structure
    within mUndi32DeviceList global array.

   @return     UNDI_PRIVATE_DATA    Pointer to Private Data Structure.
   @return     NULL                 No controllers within the array
**/
UNDI_PRIVATE_DATA*
GetFirstControllerPrivateData (
  );

/** Iteration helper. Get controller private data structure standing
    next to UndiPrivateData within mUndi32DeviceList global array.

   @param[in]  UndiPrivateData        Pointer to Private Data Structure.

   @return     UNDI_PRIVATE_DATA    Pointer to Private Data Structure.
   @return     NULL                 No controllers within the array
**/
UNDI_PRIVATE_DATA*
GetNextControllerPrivateData (
  IN  UNDI_PRIVATE_DATA     *UndiPrivateData
  );

/** Updates link information in family specific Hw structure.

   @param[in]   AdapterInfo  Pointer to the NIC data structure information which
                             the UNDI driver is layering on

   @retval   EFI_SUCCESS        Link information updated successfully
   @retval   EFI_DEVICE_ERROR   get_link_info AQ cmd failed
**/
EFI_STATUS
UpdateLinkInfo (
  IN DRIVER_DATA *AdapterInfo
  );


#endif /* I40E_H_ */
