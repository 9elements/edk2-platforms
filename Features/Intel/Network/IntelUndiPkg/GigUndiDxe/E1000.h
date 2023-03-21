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

#ifndef E1000_H_
#define E1000_H_

#include <Uefi.h>

#include <Base.h>
#include <Guid/EventGroup.h>
#include <Protocol/PciIo.h>
#include <Protocol/NetworkInterfaceIdentifier.h>
#include <Protocol/DevicePath.h>
#include <Protocol/ComponentName2.h>
#include <Protocol/DriverDiagnostics.h>
#include <Protocol/DriverBinding.h>
#include <Protocol/DriverSupportedEfiVersion.h>
#include <Protocol/PlatformToDriverConfiguration.h>
#include <Protocol/FirmwareManagement.h>
#include <Protocol/DriverHealth.h>

#include <Protocol/HiiConfigRouting.h>
#include <Protocol/FormBrowser2.h>
#include <Protocol/HiiConfigAccess.h>
#include <Protocol/HiiDatabase.h>
#include <Protocol/HiiString.h>

#include <Guid/MdeModuleHii.h>

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


#include "e1000_api.h"
#include "StartStop.h"
#include "Version.h"
#include "AdapterInformation.h"
#include "DriverHealth.h"
#include "Dma.h"

#include "EepromConfig.h"





// DRIVER_DATA forward declaration
typedef struct DRIVER_DATA_S DRIVER_DATA;

#include "Hii/CfgAccessProt/HiiConfigAccessInfo.h"

#include "LanEngine/Receive.h"
#include "LanEngine/Transmit.h"


#define MAX_NIC_INTERFACES  256
#define MAX_NUMBER_OF_PORTS 4

// Device and Vendor IDs
#define INTEL_VENDOR_ID         0x8086
#define E1000_VENDOR_ID         INTEL_VENDOR_ID
#define E1000_SUBVENDOR_ID      0x8086

#define EFI_NETWORK_INTERFACE_IDENTIFIER_PROTOCOL_REVISION_31 0x00010001
#define PXE_ROMID_MINORVER_31 0x10


// PCI Base Address Register Bits
#define PCI_BAR_IO_MASK             0x00000003
#define PCI_BAR_IO_MODE             0x00000001

#define PCI_BAR_MEM_MASK            0x0000000F
#define PCI_BAR_MEM_MODE            0x00000000
#define PCI_BAR_MEM_64BIT           0x00000004

// Bit fields for the PCI command register
#define PCI_COMMAND_MWI     0x10
#define PCI_COMMAND_MASTER  0x04
#define PCI_COMMAND_MEM     0x02
#define PCI_COMMAND_IO      0x01
#define PCI_COMMAND         0x04
#define PCI_LATENCY_TIMER   0x0D

// PCI Capability IDs
#define PCI_EX_CAP_ID           0x10
#define PCI_CAP_PTR_ENDPOINT    0x00

// PCI Configuration Space Register Offsets
#define PCI_CAP_PTR         0x34    /* PCI Capabilities pointer */

#define ETH_ALEN            ETH_ADDR_LEN

// PBA constants
#define E1000_PBA_16K 0x0010    /* 16KB, default TX allocation */
#define E1000_PBA_22K 0x0016
#define E1000_PBA_24K 0x0018
#define E1000_PBA_30K 0x001E
#define E1000_PBA_40K 0x0028
#define E1000_PBA_48K 0x0030    /* 48KB, default RX allocation */

// EEPROM Word Defines:
#define INIT_CONTROL_WORD_2                 0x0F
#define INIT_CONTROL_WORD_2_PWR_DOWN_BIT    BIT (6)
#define PCIE_CONTROL                        0x1B
#define FUNC_CONTROL_WORD                   0x21
#define PCIE_CONTROL_2                      0x28
#define FUNC_CONTROL_LAN_FUNCTION_SEL       (1 << 12)
#define PCIE_CONTROL_DUMMY_FUNCTION_ENABLE  (1 << 14)

/* Initialization Control Word 2 indicates flash size
 000: 64KB, 001: 128KB, 010: 256KB, 011: 512KB, 100: 1MB, 101: 2MB, 110: 4MB, 111: 8MB
 The Flash size impacts the requested memory space for the Flash and expansion ROM BARs. */
#define FLASH_SIZE_MASK     0x0700
#define FLASH_SIZE_SHIFT    8

// "Main Setup Options Word"
#define SETUP_OPTIONS_WORD 0x30
#define SETUP_OPTIONS_WORD_LANB 0x34
#define SETUP_OPTIONS_WORD_LANC 0x38
#define SETUP_OPTIONS_WORD_LAND 0x3A

#define FDP_FULL_DUPLEX_BIT   0x1000
#define FSP_100MBS            0x0800
#define FSP_10MBS             0x0400
#define FSP_AUTONEG           0x0000
#define FSP_MASK              0x0C00
#define DISPLAY_SETUP_MESSAGE 0x0100

// "Configuration Customization Word"
#define CONFIG_CUSTOM_WORD 0x31
#define CONFIG_CUSTOM_WORD_LANB 0x35
#define CONFIG_CUSTOM_WORD_LANC 0x39
#define CONFIG_CUSTOM_WORD_LAND 0x3B

#define SIG               0x4000
#define SIG_MASK          0xC000

#define EEPROM_CAPABILITIES_WORD       0x33
#define EEPROM_CAPABILITIES_SIG        0x4000
#define EEPROM_CAPABILITIES_SIG_MASK   0xC000
#define EEPROM_BC_BIT                  0x0001
#define EEPROM_UNDI_BIT                0x0002
#define EEPROM_PXE_BIT                 (EEPROM_BC_BIT | EEPROM_UNDI_BIT)
#define EEPROM_RPL_BIT                 0x0004
#define EEPROM_EFI_BIT                 0x0008
#define EEPROM_FCOE_BIT                0x0020
#define EEPROM_ISCSI_BIT               0x0010
#define EEPROM_LEGACY_BIT              (EEPROM_PXE_BIT | EEPROM_ISCSI_BIT)
#define EEPROM_SMCLP_BIT               0x0040
#define EEPROM_TYPE_MASK               (EEPROM_BC_BIT | EEPROM_UNDI_BIT | EEPROM_EFI_BIT | EEPROM_ISCSI_BIT)
#define EEPROM_ALL_BITS                (EEPROM_TYPE_MASK | EEPROM_RPL_BIT)

#define COMPATIBILITY_WORD          0x03
#define COMPATABILITY_LOM_BIT       0x0800 /* bit 11 */

// UNDI_CALL_TABLE.state can have the following values
#define DONT_CHECK -1
#define ANY_STATE -1
#define MUST_BE_STARTED 1
#define MUST_BE_INITIALIZED 2

#define EFI_OPTIONAL_PTR                    0x00000001
#define EFI_INTERNAL_PTR                    0x00000004 /* Pointer to internal runtime data */
#define EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE   0x60000202

#define UNDI_DEV_SIGNATURE   SIGNATURE_32 ('P', 'R', '0', 'g')

typedef struct e1000_tx_desc        TRANSMIT_DESCRIPTOR;
typedef struct e1000_rx_desc        RECEIVE_DESCRIPTOR;

#define TX_RING_FROM_ADAPTER(a)     ((TRANSMIT_RING*)(&(a)->TxRing))
#define RX_RING_FROM_ADAPTER(a)     ((RECEIVE_RING*)(&(a)->RxRing))
#define PCI_IO_FROM_ADAPTER(a)      (EFI_PCI_IO_PROTOCOL*)((a)->PciIo)

/** Retrieves UNDI_PRIVATE_DATA structure using NII Protocol 3.1 instance

   @param[in]   a   Current protocol instance

   @return    UNDI_PRIVATE_DATA structure instance
**/
#define UNDI_PRIVATE_DATA_FROM_THIS(a) \
  CR (a, UNDI_PRIVATE_DATA, NiiProtocol31, UNDI_DEV_SIGNATURE)

/** Retrieves UNDI_PRIVATE_DATA structure using DevicePath instance

   @param[in]   a   Current protocol instance

   @return    UNDI_PRIVATE_DATA structure instance
**/
#define UNDI_PRIVATE_DATA_FROM_DEVICE_PATH(a) \
  CR (a, UNDI_PRIVATE_DATA, Undi32BaseDevPath, UNDI_DEV_SIGNATURE)


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


/** Macro to return the offset of a member within a struct.  This
   looks like it dereferences a null pointer, but it doesn't really.

   @param[in]   Structure    Structure type
   @param[in]   Member       Structure member

   @return    Offset of a member within a struct
**/
#define STRUCT_OFFSET(Structure, Member)     ((UINTN) &(((Structure *) 0)->Member))

/** Aligns number to specified granularity

   @param[in]   x   Number
   @param[in]   a   Granularity

   @return   Number aligned to Granularity
 */
#define ALIGN(x, a)    (((x) + ((UINT64) (a) - 1)) & ~((UINT64) (a) - 1))

/** Test bit mask against a value.

   @param[in]   v   Value
   @param[in]   m   Mask

   @return    TRUE when value contains whole bit mask, otherwise - FALSE.
 */
#define BIT_TEST(v, m) (((v) & (m)) == (m))

/** Macro to compare MAC addresses.  Returns true if the MAC addresses match.
   a and b must be UINT8 pointers to the first byte of MAC address.

   @param[in]   a   Pointer to MAC address to compare with b
   @param[in]   b   Pointer to MAC address to compare with a

   @retval  TRUE     if MAC addresses match
   @retval  FALSE    MAC addresses don't match
**/
#define E1000_COMPARE_MAC(a, b) \
  ( *((UINT32 *) a) == *((UINT32 *) b) ) && ( *((UINT16 *) (a + 4)) == *((UINT16 *) (b + 4)) )

/** Macro to copy MAC address b to a.
   a and b must be UINT8 pointers to the first byte of MAC address.

   @param[in]   a   Pointer to MAC address to copy to
   @param[in]   b   Pointer to MAC address to copy from

   @return   MAC b copied to a
**/
#define E1000_COPY_MAC(a, b) \
  *((UINT32 *) a) = *((UINT32 *) b);*((UINT16 *) (a + 4)) = *((UINT16 *) (b + 4))

/** Helper for controller private data for() iteration loop.

   @param[in]    Iter   Driver private data iteration pointer
*/
#define FOREACH_ACTIVE_CONTROLLER(Iter)                 \
  for ((Iter) = GetFirstControllerPrivateData ();       \
       (Iter) != NULL;                                  \
       (Iter) = GetNextControllerPrivateData ((Iter)))

/* External variables declarations */
extern PXE_SW_UNDI                 *mE1000Pxe31;
extern UNDI_PRIVATE_DATA           *mUndi32DeviceList[MAX_NIC_INTERFACES];
extern EFI_DRIVER_BINDING_PROTOCOL gUndiDriverBinding;
extern EFI_DRIVER_BINDING_PROTOCOL gGigUndiDriverBinding;
extern EFI_GUID                    gEfiNiiPointerGuid;
extern EFI_GUID                    gNiiPointerProtocolGuid;
extern EFI_SYSTEM_TABLE            *gSystemTable;
extern EFI_TIME                    gTime;
extern BOOLEAN                     mExitBootServicesTriggered;

extern UINT8                       GigUndiDxeString[];

typedef struct {
  UINT16 CpbSize;
  UINT16 DbSize;
  UINT16 OpFlags;
  UINT16 State;
  VOID (*ApiPtr)();
} UNDI_CALL_TABLE;

/* External Global Variables */
extern UNDI_CALL_TABLE                           mE1000ApiTable[];
extern EFI_COMPONENT_NAME_PROTOCOL               gUndiComponentName;
extern EFI_COMPONENT_NAME2_PROTOCOL              gUndiComponentName2;
extern EFI_DRIVER_SUPPORTED_EFI_VERSION_PROTOCOL gUndiSupportedEfiVersion;
extern EFI_DRIVER_CONFIGURATION_PROTOCOL         gGigUndiDriverConfiguration;
extern EFI_DRIVER_DIAGNOSTICS_PROTOCOL           gGigUndiDriverDiagnostics;
extern EFI_DRIVER_DIAGNOSTICS2_PROTOCOL          gGigUndiDriverDiagnostics2;
extern EFI_DRIVER_HEALTH_PROTOCOL                gUndiDriverHealthProtocol;

extern EFI_DRIVER_STOP_PROTOCOL                  gUndiDriverStop;
extern EFI_GUID                                  gEfiStartStopProtocolGuid;

extern UINT8                                     E1000UndiDxeStrings[];


typedef struct {
  EFI_NETWORK_INTERFACE_IDENTIFIER_PROTOCOL *InterfacePointer;
  EFI_DEVICE_PATH_PROTOCOL                  *DevicePathPointer;
} NII_ENTRY;

typedef struct NII_CONFIG_ENTRY {
  UINT32                   NumEntries;
  UINT32                   Reserved;
  struct NII_CONFIG_ENTRY  *NextLink;
  NII_ENTRY                NiiEntry[MAX_NIC_INTERFACES];
} NII_TABLE;

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

typedef struct e1000_rx_desc E1000_RECEIVE_DESCRIPTOR;


// TX Buffer size including crc and padding
#define RX_BUFFER_SIZE 2048

#define DEFAULT_RX_DESCRIPTORS 64
#define DEFAULT_TX_DESCRIPTORS 8

#pragma pack(1)
typedef struct {
  UINT8  RxBuffer[RX_BUFFER_SIZE - (sizeof (UINT64))];
  UINT64 BufferUsed;
} LOCAL_RX_BUFFER, *PLOCAL_RX_BUFFER;
#pragma pack()

typedef struct e1000_tx_desc E1000_TRANSMIT_DESCRIPTOR;

typedef struct {
  UINT32 RegisterOffset;
  UINT32 Sck;
  UINT32 Cs;
} FLASH_CLOCK_REGISTER;

typedef struct {
  UINT32 RegisterOffset;
  UINT32 Si;
  UINT32 So;
  UINT8  SoPosition;
} FLASH_DATA_REGISTER;

/* If using a serial flash, this struct will be filled with the
 proper offsets, since I82540 and I82541 use different registers
 for Flash manipulation. */
typedef struct {
  FLASH_CLOCK_REGISTER FlashClockRegister;
  FLASH_DATA_REGISTER  FlashDataRegister;
} SERIAL_FLASH_OFFSETS;



typedef struct {
  UINT16 Length;
  UINT8  McAddr[MAX_MCAST_ADDRESS_CNT][PXE_MAC_LENGTH]; // 8*32 is the size
} MCAST_LIST;

typedef struct DRIVER_DATA_S {
  UINT16                  State; // stopped, started or initialized

  struct e1000_hw         Hw;
  struct e1000_hw_stats   Stats;
  UINTN                   Segment;
  UINTN                   Bus;
  UINTN                   Device;
  UINTN                   Function;

  UINT8                   PciClass;
  UINT8                   PciSubClass;
  UINT8                   PciClassProgIf;

  UINTN                   LanFunction;

  SERIAL_FLASH_OFFSETS    SerialFlashOffsets;

  UINT8                   BroadcastNodeAddress[PXE_MAC_LENGTH];

  UINT32                  PciConfig[MAX_PCI_CONFIG_LEN];
  UINT32                  NvData[MAX_EEPROM_LEN];

  UINTN                   HwInitialized;
  UINTN                   DriverBusy;
  UINT16                  LinkSpeed; // requested (forced) link speed
  UINT8                   DuplexMode; // requested duplex
  UINT8                   CableDetect; // 1 to detect and 0 not to detect the cable
  UINT8                   LoopBack;

  UINT8                   UndiEnabled; // When 0 only HII and FMP are avaliable,
                                     // NII is not installed on ControllerHandle
                                     // (e.g. in case iSCSI driver loaded on port)

  UINT64                  UniqueId;
  EFI_PCI_IO_PROTOCOL     *PciIo;
  UINT64                  OriginalPciAttributes;

  // UNDI callbacks
  BS_PTR                  Delay;
  VIRT_PHYS               Virt2Phys;
  BLOCK                   Block;
  MEM_IO                  MemIo;
  MAP_MEM                 MapMem;
  UNMAP_MEM               UnMapMem;
  SYNC_MEM                SyncMem;

  UINT8                   IoBarIndex;
  UINT16                  RxFilter;
  UINT8                   IntMask;

  MCAST_LIST              McastList;

  RECEIVE_RING            RxRing;
  TRANSMIT_RING           TxRing;

  BOOLEAN                 MacAddrOverride;
  BOOLEAN                 FlashWriteInProgress;
  BOOLEAN                 SurpriseRemoval;
  UINTN                   VersionFlag; // Indicates UNDI version 3.0 or 3.1
} DRIVER_DATA;

typedef struct HII_INFO_S {
  EFI_HANDLE           HiiInstallHandle;
  EFI_HII_HANDLE       HiiPkgListHandle;
  HII_CFG_ACCESS_INFO  HiiCfgAccessInfo;

  BOOLEAN              AltMacAddrSupported;
} HII_INFO;

typedef struct UNDI_PRIVATE_DATA_S {
  UINTN                                     Signature;
  UINTN                                     IfId;
  EFI_NETWORK_INTERFACE_IDENTIFIER_PROTOCOL NiiProtocol31;
  EFI_NII_POINTER_PROTOCOL                  NIIPointerProtocol;
  EFI_HANDLE                                ControllerHandle;
  EFI_HANDLE                                DeviceHandle;
  EFI_HANDLE                                FmpInstallHandle;
  EFI_DEVICE_PATH_PROTOCOL                  *Undi32BaseDevPath;
  EFI_DEVICE_PATH_PROTOCOL                  *Undi32DevPath;
  DRIVER_DATA                               NicInfo;
  CHAR16                                    *Brand;
  EFI_UNICODE_STRING_TABLE                  *ControllerNameTable;
  BOOLEAN                                   IsChildInitialized;
  HII_INFO                                  HiiInfo;
  EFI_DRIVER_STOP_PROTOCOL                  DriverStop;


  EFI_ADAPTER_INFORMATION_PROTOCOL          AdapterInformation;
  UINT32                                    LastAttemptVersion;
  UINT32                                    LastAttemptStatus;
} UNDI_PRIVATE_DATA;

typedef struct {
  E1000_RECEIVE_DESCRIPTOR  RxRing[DEFAULT_RX_DESCRIPTORS];
  E1000_TRANSMIT_DESCRIPTOR TxRing[DEFAULT_TX_DESCRIPTORS];
  LOCAL_RX_BUFFER           RxBuffer[DEFAULT_RX_DESCRIPTORS];
} GIG_UNDI_DMA_RESOURCES;

typedef struct {
  UINT8                     Pf0:1;
  UINT8                     Pf1:1;
  UINT8                     Pf2:1;
  UINT8                     Pf3:1;
} ACTIVE_PHYSICAL_FUNCTIONS;

typedef union {
  ACTIVE_PHYSICAL_FUNCTIONS   PhysicalFunction;
  UINT8                       Mask;
} ALLOWED_PFS_STATE_VALUES;

#define BYTE_ALIGN_64    0x7F

/* We need enough space to store TX descriptors, RX descriptors,
 RX buffers, and enough left over to do a 64 byte alignment. */
#define RX_RING_SIZE    sizeof (((GIG_UNDI_DMA_RESOURCES*) 0)->RxRing)
#define TX_RING_SIZE    sizeof (((GIG_UNDI_DMA_RESOURCES*) 0)->TxRing)
#define RX_BUFFERS_SIZE sizeof (((GIG_UNDI_DMA_RESOURCES*) 0)->RxBuffer)

#define FOUR_GIGABYTE (UINT64) 0x100000000

/* If the surprise removal has been detected,
 Device Status Register returns 0xFFFFFFFF */
#define INVALID_STATUS_REGISTER_VALUE  0xFFFFFFFF

/** This function performs PCI-E initialization for the device.

   @param[in]   AdapterInfo   Pointer to adapter structure

   @retval   EFI_SUCCESS            PCI-E initialized successfully
   @retval   EFI_UNSUPPORTED        Failed to get supported PCI command options
   @retval   EFI_UNSUPPORTED        Failed to set PCI command options
   @retval   EFI_OUT_OF_RESOURCES   The memory pages for transmit and receive resources could
                                    not be allocated
**/
EFI_STATUS
E1000PciInit (
  IN DRIVER_DATA *AdapterInfo
  );


/** Gets Max Speed of ethernet port in bits.

   @param[in]   UndiPrivateData   Points to the driver instance private data
   @param[out]  MaxSpeed          Resultant value

   @retval      EFI_SUCCESS       Operation successful.
**/
EFI_STATUS
GetMaxSpeed (
  IN  UNDI_PRIVATE_DATA  *UndiPrivateData,
  OUT UINT64             *MaxSpeed
  );


/** Initializes the gigabit adapter, setting up memory addresses, MAC Addresses,
   Type of card, etc.

   @param[in]   AdapterInfo   Pointer to adapter structure

   @retval   PXE_STATCODE_SUCCESS       Initialization succeeded
   @retval   PXE_STATCODE_NOT_STARTED   Hardware Init failed
**/
PXE_STATCODE
E1000Inititialize (
  IN DRIVER_DATA *AdapterInfo
  );

#define PCI_CLASS_MASK          0xFF00
#define PCI_SUBCLASS_MASK       0x00FF

/** This function is called as early as possible during driver start to ensure the
   hardware has enough time to autonegotiate when the real SNP device initialize call is made.

   @param[in]   AdapterInfo   Pointer to adapter structure

   @retval   EFI_SUCCESS        Hardware init success
   @retval   EFI_DEVICE_ERROR   Hardware init failed
   @retval   EFI_UNSUPPORTED    Unsupported MAC type
   @retval   EFI_UNSUPPORTED    e1000_setup_init_funcs failed
   @retval   EFI_UNSUPPORTED    Could not read bus information
   @retval   EFI_UNSUPPORTED    Could not read MAC address
   @retval   EFI_ACCESS_DENIED  iSCSI Boot detected on port
   @retval   EFI_DEVICE_ERROR   Failed to reset hardware
**/
EFI_STATUS
E1000FirstTimeInit (
  IN DRIVER_DATA *AdapterInfo
  );

#define MAX_QUEUE_ENABLE_TIME   200

/** Starts the receive unit.

   @param[in]   AdapterInfo   Pointer to the NIC data structure information
                              which the UNDI driver is layering on..

   @return   Receive unit started
**/
VOID
E1000ReceiveStart (
  IN DRIVER_DATA *AdapterInfo
  );

/** Stops the receive unit.

   @param[in]   AdapterInfo   Pointer to the NIC data structure information
                              which the UNDI driver is layering on..

   @return   Receive unit stopped
**/
VOID
E1000ReceiveStop (
  IN DRIVER_DATA *AdapterInfo
  );

/** Takes a command Block pointer (cpb) and sends the frame.  Takes either one fragment or many
   and places them onto the wire.  Cleanup of the send happens in the function UNDI_Status in DECODE.C

   @param[in]   AdapterInfo   Pointer to the instance data
   @param[in]   Cpb       The command parameter Block address.  64 bits since this is Itanium(tm)
                          processor friendly
   @param[in]   OpFlags   The operation flags, tells if there is any special sauce on this transmit

  @retval     PXE_STATCODE_SUCCESS          Packet enqueued for transmit.
  @retval     PXE_STATCODE_DEVICE_FAILURE   AdapterInfo parameter is NULL.
  @retval     PXE_STATCODE_DEVICE_FAILURE   Failed to send packet.
  @retval     PXE_STATCODE_INVALID_CPB      CPB invalid.
  @retval     PXE_STATCODE_UNSUPPORTED      Fragmented tranmission was requested.
  @retval     PXE_STATCODE_QUEUE_FULL       Tx queue is full.
**/
UINTN
E1000Transmit (
  IN DRIVER_DATA *AdapterInfo,
  IN UINT64       Cpb,
  IN UINT16       OpFlags
  );

/** Copies the frame from our internal storage ring (As pointed to by AdapterInfo->rx_ring)
   to the command Block passed in as part of the cpb parameter.

   The flow:
   Ack the interrupt, setup the pointers, find where the last Block copied is, check to make
   sure we have actually received something, and if we have then we do a lot of work.
   The packet is checked for errors, size is adjusted to remove the CRC, adjust the amount
   to copy if the buffer is smaller than the packet, copy the packet to the EFI buffer,
   and then figure out if the packet was targetted at us, broadcast, multicast
   or if we are all promiscuous.  We then put some of the more interesting information
   (protocol, src and dest from the packet) into the db that is passed to us.
   Finally we clean up the frame, set the return value to _SUCCESS, and inc the cur_rx_ind, watching
   for wrapping.  Then with all the loose ends nicely wrapped up, fade to black and return.

   @param[in]   AdapterInfo   Pointer to the driver data
   @param[in]   Cpb           Pointer (Ia-64 friendly) to the command parameter
                              block. The frame will be placed inside of it.
   @param[out]  Db            The data buffer. The out of band method of passing
                              pre-digested information to the protocol.

  @retval     PXE_STATCODE_NO_DATA        There is no data to receive.
  @retval     PXE_STATCODE_DEVICE_FAILURE AdapterInfo is NULL.
  @retval     PXE_STATCODE_DEVICE_FAILURE Device failure on packet receive.
  @retval     PXE_STATCODE_INVALID_CPB    Invalid CPB/DB parameters.
  @retval     PXE_STATCODE_NOT_STARTED    Rx queue not started.
  @retval     PXE_STATCODE_SUCCESS        Received data passed to the protocol.
**/
UINTN
E1000Receive (
  IN  DRIVER_DATA       *AdapterInfo,
  IN  PXE_CPB_RECEIVE   *CpbReceive,
  OUT PXE_DB_RECEIVE    *DbReceive
  );

/** Resets the hardware and put it all (including the PHY) into a known good state.

   @param[in]   AdapterInfo   The pointer to our context data
   @param[in]   OpFlags       The information on what else we need to do.

   @retval   PXE_STATCODE_SUCCESS        Successfull hardware reset
   @retval   PXE_STATCODE_NOT_STARTED    Hardware init failed
**/
UINTN
E1000Reset (
  IN DRIVER_DATA *AdapterInfo,
  IN UINT16       OpFlags
  );

/** Stop the hardware and put it all (including the PHY) into a known good state.

   @param[in]   AdapterInfo   Pointer to the driver structure

   @retval   PXE_STATCODE_SUCCESS    Hardware stopped
**/
UINTN
E1000Shutdown (
  IN DRIVER_DATA *AdapterInfo
  );

/** Free TX buffers that have been transmitted by the hardware.

   @param[in]   AdapterInfo   Pointer to the NIC data structure information
                              which the UNDI driver is layering on.
   @param[in]   NumEntries    Number of entries in the array which can be freed.
   @param[out]  TxBuffer      Array to pass back free TX buffer

   @return   Number of TX buffers written.
**/
UINT16
E1000FreeTxBuffers (
  IN  DRIVER_DATA *AdapterInfo,
  IN  UINT16       NumEntries,
  OUT UINT64      *TxBuffer
  );

/** Sets specified bits in a device register

   @param[in]   AdapterInfo   Pointer to the device instance
   @param[in]   Register      Register to write
   @param[in]   BitMask       Bits to set

   @return   Returns the value read from the PCI register.
**/
UINT32
E1000SetRegBits (
  IN DRIVER_DATA *AdapterInfo,
  IN UINT32       Register,
  IN UINT32       BitMask
  );

/** Clears specified bits in a device register

   @param[in]   AdapterInfo   Pointer to the device instance
   @param[in]   Register      Register to write
   @param[in]   BitMask       Bits to clear

   @return    Returns the value read from the PCI register.
**/
UINT32
E1000ClearRegBits (
  IN DRIVER_DATA *AdapterInfo,
  IN UINT32       Register,
  IN UINT32       BitMask
  );

/** Checks if link is up

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

/** Returns information whether Link Speed attribute is supported.

   @param[in]   UndiPrivateData     Pointer to driver private data structure
   @param[out]  LinkSpeedSupported  BOOLEAN value describing support

   @retval      EFI_SUCCESS         Successful operation
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

/** Reads PBA string from NVM.

   @param[in]   UndiPrivateData  Pointer to driver private data structure
   @param[out]  PbaNumberStr     Output string buffer for PBA string

   @retval   EFI_SUCCESS            PBA string successfully read
   @retval   EFI_SUCCESS            PBA string unsupported by adapter
   @retval   EFI_DEVICE_ERROR       Failed to read PBA flags word
   @retval   EFI_DEVICE_ERROR       Failed to read PBA module pointer
   @retval   EFI_DEVICE_ERROR       Failed to read PBA length
   @retval   EFI_DEVICE_ERROR       Failed to read PBA number word
**/
EFI_STATUS
GetPbaStr (
  IN  UNDI_PRIVATE_DATA  *UndiPrivateData,
  OUT EFI_STRING         PbaNumberStr
  );

/** Gets 1Gig chip type string.

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

/** Detects surprise removal device status in PCI controller register

   @param[in]   Adapter   Pointer to the device instance

   @retval   TRUE    Surprise removal has been detected
   @retval   FALSE   Surprise removal has not been detected
**/
BOOLEAN
IsSurpriseRemoval (
  IN DRIVER_DATA *AdapterInfo
  );

/** Stop the hardware and put it all (including the PHY) into a known good state.

   @param[in]   AdapterInfo   Pointer to the driver structure

   @retval   PXE_STATCODE_SUCCESS    Hardware stopped
**/
UINTN
E1000Shutdown (
  IN DRIVER_DATA *AdapterInfo
  );

/** Changes filter settings

   @param[in]   AdapterInfo   Pointer to the NIC data structure information which the UNDI driver is layering on..
   @param[in]   NewFilter     A PXE_OPFLAGS bit field indicating what filters to use.
   @param[in]   Cpb           The command parameter Block address.  64 bits since this is Itanium(tm)
                              processor friendly
   @param[in]   CpbSize       Command parameter Block size

   @retval   0   Filters changed according to NewFilter settings
**/
UINTN
E1000SetFilter (
  DRIVER_DATA *AdapterInfo,
  UINT16       NewFilter,
  UINT64       Cpb,
  UINT32       CpbSize
  );

/** Updates or resets field in E1000 HW statistics structure

   @param[in]   SwReg   Structure field mapped to HW register
   @param[in]   HwReg   HW register to read from

   @return   Stats reset or updated
**/
#define UPDATE_OR_RESET_STAT(SwReg, HwReg) \
  do { \
    St->SwReg = DbAddr ? St->SwReg + E1000_READ_REG (Hw, HwReg) : 0; \
  } while (0)

/** Updates Supported PXE_DB_STATISTICS structure field which indicates
   which statistics data are collected

   @param[in]   S   PXE_STATISTICS type

   @return   Supported field updated
**/
#define SET_SUPPORT(S) \
  do { \
    Stat = PXE_STATISTICS_ ## S; \
    DbPtr->Supported |= (UINT64) (1 << Stat); \
  } while (0)

/** Sets support and updates Data[] PXE_DB_STATISTICS structure field with specific
   field from E1000 HW statistics structure

   @param[in]   S   PXE_STATISTICS type
   @param[in]   B   Field from E1000 HW statistics structure

   @return   EFI statistics updated
**/
#define UPDATE_EFI_STAT(S, B) \
  do { \
    SET_SUPPORT (S); \
    DbPtr->Data[Stat] = St->B; \
  } while (0)

/** Copies the stats from our local storage to the protocol storage.

   It means it will read our read and clear numbers, so some adding is required before
   we copy it over to the protocol.

   @param[in]   AdapterInfo   Pointer to the NIC data structure information
                              which the UNDI driver is layering on..
   @param[in]   DbAddr        The data Block address
   @param[in]   DbSize        The data Block size

   @retval   PXE_STATCODE_SUCCESS  Statistics copied successfully
**/
UINTN
E1000Statistics (
  IN DRIVER_DATA *AdapterInfo,
  IN UINT64       DbAddr,
  IN UINT16       DbSize
  );

/** Allows the protocol to control our interrupt behaviour.

   @param[in]   AdapterInfo   Pointer to the driver structure

   @retval   PXE_STATCODE_SUCCESS   Interrupt state set successfully
**/
UINTN
E1000SetInterruptState (
  IN DRIVER_DATA *AdapterInfo
  );

/** This routine blocks until auto-negotiation completes or times out (after 4.5 seconds).

   @param[in]   AdapterInfo   Pointer to the NIC data structure information
                              which the UNDI driver is layering on..

   @retval   TRUE   Auto-negotiation completed successfully,
   @retval   FALSE  Auto-negotiation did not complete (i.e., timed out)
**/
BOOLEAN
E1000WaitForAutoNeg (
  IN DRIVER_DATA *AdapterInfo
  );

/** Delay a specified number of microseconds

   @param[in]   Adapter        Pointer to the NIC data structure information
                               which the UNDI driver is layering on..
   @param[in]   MicroSeconds   Time to delay in Microseconds.

   @return   Execution of code delayed
**/
VOID
DelayInMicroseconds (
  IN DRIVER_DATA *AdapterInfo,
  IN UINTN        MicroSeconds
  );

// Time translations.
/** Delays code execution for specified time in milliseconds

   @param[in]   x   Time in milliseconds

   @return   Execution of code delayed
**/
#define DELAY_IN_MILLISECONDS(x)  DelayInMicroseconds (AdapterInfo, x * 1000)

/** Returns information whether current device supports any RDMA protocol.

  @param[in]   UndiPrivateData    Pointer to the driver private data
  @param[out]  Supported          Tells whether feature is supported

  @retval     EFI_SUCCESS    Operation successful
**/
EFI_STATUS
IsRdmaSupported (
  IN   UNDI_PRIVATE_DATA  *UndiPrivateData,
  OUT  BOOLEAN            *Supported
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
#endif /* E1000_H_ */
