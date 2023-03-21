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
#ifndef XGBE_H_
#define XGBE_H_

#ifndef EFI_SPECIFICATION_VERSION
#define EFI_SPECIFICATION_VERSION 0x00020000
#endif /* EFI_SPECIFICATION_VERSION */

#ifndef TIANO_RELEASE_VERSION
#define TIANO_RELEASE_VERSION 0x00080005
#endif /* TIANO_RELEASE_VERSION */

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
#include <Library/HiiLib.h>

#include <IndustryStandard/Pci.h>

// Debug macros are located here.
#include "DebugTools.h"

#include "ixgbe_api.h"
#include "ixgbe_type.h"
#include "StartStop.h"
#include "Decode.h"
#include "Version.h"
#include "AdapterInformation.h"
#include "DriverHealth.h"
#include "Dma.h"
#include "DeviceSupport.h"

#include "EepromConfig.h"



#include "Hii/CfgAccessProt/HiiConfigAccessInfo.h"

#include "LanEngine/Receive.h"
#include "LanEngine/Transmit.h"

#ifndef IXGBE_INTEL_VENDOR_ID
#define IXGBE_INTEL_VENDOR_ID INTEL_VENDOR_ID
#endif /* IXGBE_INTEL_VENDOR_ID */

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

#define MAX_NIC_INTERFACES    256

// Device and Vendor IDs
#define INTEL_VENDOR_ID       0x8086
#define HP_SUBVENDOR_ID       0x103C

// BlinkInterval value (BlinkLeds) expressed in ms
#define BLINK_INTERVAL                      200

#define UNDI_DEV_SIGNATURE             SIGNATURE_32 ('P', 'R', '0', 'x')

// Interrupt related defines:
#define IXGBE_EICR_RTX_QUEUE_0_MASK 0x01
#define IXGBE_EICR_RTX_QUEUE_1_MASK 0x02

// Transmit/receive
#define ETH_ALEN                    6
#define RX_BUFFER_SIZE              2048

#define DEFAULT_RX_DESCRIPTORS      8
#define DEFAULT_TX_DESCRIPTORS      8

typedef struct ixgbe_legacy_tx_desc TRANSMIT_DESCRIPTOR;
typedef struct ixgbe_legacy_rx_desc RECEIVE_DESCRIPTOR;

#define TX_RING_FROM_ADAPTER(a)     ((TRANSMIT_RING*)(&(a)->TxRing))
#define RX_RING_FROM_ADAPTER(a)     ((RECEIVE_RING*)(&(a)->RxRing))
#define PCI_IO_FROM_ADAPTER(a)      (EFI_PCI_IO_PROTOCOL*)((a)->PciIo)

#define SR_DEVICE_CAPS IXGBE_DEVICE_CAPS

/** Retrieves UNDI_PRIVATE_DATA structure using NII Protocol 3.1 instance

   @param[in]   a   Current protocol instance

   @return    UNDI_PRIVATE_DATA structure instance
**/
#define UNDI_PRIVATE_DATA_FROM_THIS(a) CR (a, UNDI_PRIVATE_DATA, NiiProtocol31, UNDI_DEV_SIGNATURE)

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


/** Helper for controller private data for() iteration loop.

   @param[in]    Iter   Driver private data iteration pointer
*/
#define FOREACH_ACTIVE_CONTROLLER(Iter)                 \
  for ((Iter) = GetFirstControllerPrivateData ();       \
       (Iter) != NULL;                                  \
       (Iter) = GetNextControllerPrivateData ((Iter)))

#define EFI_NETWORK_INTERFACE_IDENTIFIER_PROTOCOL_REVISION_31   0x00010001
#define PXE_ROMID_MINORVER_31                                   0x10

typedef struct {
  UINT16 CpbSize;
  UINT16 DbSize;
  UINT16 OpFlags;
  UINT16 State;
  VOID (*ApiPtr)();
} UNDI_CALL_TABLE;

typedef struct {
  EFI_NETWORK_INTERFACE_IDENTIFIER_PROTOCOL  *InterfacePointer;
  EFI_DEVICE_PATH_PROTOCOL                   *DevicePathPointer;
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

/* External Global Variables */
extern EFI_DRIVER_BINDING_PROTOCOL                gUndiDriverBinding;
extern EFI_DRIVER_BINDING_PROTOCOL                gXgbeUndiDriverBinding;
extern EFI_COMPONENT_NAME_PROTOCOL                gXgbeUndiComponentName;

extern UNDI_CALL_TABLE                            mIxgbeApiTable[];
extern EFI_COMPONENT_NAME_PROTOCOL                gUndiComponentName;
extern EFI_COMPONENT_NAME2_PROTOCOL               gUndiComponentName2;
extern EFI_DRIVER_SUPPORTED_EFI_VERSION_PROTOCOL  gUndiSupportedEfiVersion;
extern EFI_DRIVER_DIAGNOSTICS_PROTOCOL            gXgbeUndiDriverDiagnostics;
extern EFI_DRIVER_DIAGNOSTICS2_PROTOCOL           gXgbeUndiDriverDiagnostics2;
extern EFI_DRIVER_HEALTH_PROTOCOL                 gUndiDriverHealthProtocol;
extern EFI_DRIVER_STOP_PROTOCOL                   gUndiDriverStop;


extern EFI_GUID                                   gEfiStartStopProtocolGuid;
extern EFI_GUID                                   gEfiNiiPointerGuid;


extern PXE_SW_UNDI                                *mIxgbePxe31;
extern UNDI_PRIVATE_DATA                          *mUndi32DeviceList[MAX_NIC_INTERFACES];
extern BOOLEAN                                    mExitBootServicesTriggered;

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
  UINT16 RevId;
  UINT16 ClassId;
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
  UINT16 Length;
  UINT8  McAddr[MAX_MCAST_ADDRESS_CNT][PXE_MAC_LENGTH]; // 8*32 is the size
} MCAST_LIST;

typedef struct DRIVER_DATA_S {
  UINT16                   State; // stopped, started or initialized
  struct ixgbe_hw          Hw;
  struct ixgbe_hw_stats    Stats;
  UINTN                 Bus;
  UINTN                 Device;
  UINTN                 Function;

  UINT8                 PciClass;
  UINT8                 PciSubClass;

  UINTN                 LanFunction; // LAN function to determine port 0 or port 1
                                     // when LAN function select is enabled
  UINT8                 BroadcastNodeAddress[PXE_MAC_LENGTH];

  UINT32                PciConfig[MAX_PCI_CONFIG_LEN];
  UINT32                NvData[MAX_EEPROM_LEN];

  UINTN                       HwReset;
  UINTN                       HwInitialized;
  BOOLEAN                     XceiverModuleQualified;
  UINTN                       DriverBusy;
  UINT16                      LinkSpeed; // requested (forced) link speed
  UINT8                       DuplexMode; // requested duplex
  UINT8                       CableDetect; // 1 to detect and 0 not to detect the cable
  UINT8                       LoopBack;

  UINT8                 UndiEnabled; // When 0 only HII and FMP are avaliable, NII
                                     // is not installed on ControllerHandle
                                     // (e.g. in case iSCSI driver loaded on port)

  UINT8                FwSupported;
  UINT64               UniqueId;
  EFI_PCI_IO_PROTOCOL *PciIo;
  UINT64               OriginalPciAttributes;

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

  RECEIVE_RING         RxRing;
  TRANSMIT_RING        TxRing;
  UINT16               RxFilter;
  UINT8                IntMask;
  MCAST_LIST           McastList;
  BOOLEAN              MacAddrOverride;

  UINTN                VersionFlag;  // Indicates UNDI version 3.0 or 3.1
} DRIVER_DATA, *PADAPTER_STRUCT;

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

  HII_INFO                                  HiiInfo;
  EFI_DRIVER_STOP_PROTOCOL                  DriverStop;

  EFI_ADAPTER_INFORMATION_PROTOCOL          AdapterInformation;
  BOOLEAN                                   IsChildInitialized;

  UINT32                                    LastAttemptVersion;
  UINT32                                    LastAttemptStatus;
} UNDI_PRIVATE_DATA;

/** Checks if alternate MAC address is supported.

   @param[in]   UndiPrivateData    Driver instance private data structure
   @param[out]  AltMacSupport      Tells if alternate mac address is supported

   @retval   EFI_SUCCESS            Alternate MAC address is always supported
   @retval   EFI_DEVICE_ERROR       Failed to read data from NVM
   @retval   EFI_INVALID_PARAMETER  UndiPrivateData or AltMacSupport is NULL.
**/
EFI_STATUS
GetAltMacAddressSupport (
  IN   UNDI_PRIVATE_DATA  *UndiPrivateData,
  OUT  BOOLEAN            *AltMacSupport
  );


/** Gets Max Speed of ethernet port in bits.

   @param[in]   UndiPrivateData   Points to the driver instance private data
   @param[out]  MaxSpeed          Resultant value

   @retval      EFI_SUCCESS            Operation successful.
   @retval      EFI_INVALID_PARAMETER  UndiPrivateData or MaxSpeed is NULL
**/
EFI_STATUS
GetMaxSpeed (
  IN  UNDI_PRIVATE_DATA  *UndiPrivateData,
  OUT UINT64             *MaxSpeed
  );



/** Clears receive filters.

   @param[in]   AdapterInfo   Pointer to the adapter structure
   @param[in]   NewFilter   A PXE_OPFLAGS bit field indicating what filters to clear.

   @retval   0   Filters cleared according to NewFilter settings
**/
UINTN
XgbeClearFilter (
  IN DRIVER_DATA *AdapterInfo,
  IN UINT16       NewFilter
  );

/** Initializes the gigabit adapter, setting up memory addresses, MAC Addresses,
   Type of card, etc.

   @param[in]   AdapterInfo   Pointer to adapter structure

   @retval   PXE_STATCODE_SUCCESS       Initialization succeeded
   @retval   PXE_STATCODE_NOT_STARTED   Hardware initialization failed
**/
PXE_STATCODE
XgbeInitialize (
  IN DRIVER_DATA *AdapterInfo
  );

/** Initializes the hardware and sets up link.

   @param[in]   AdapterInfo   Pointer to adapter structure

   @retval   EFI_SUCCESS        Hardware initialized, link set up
   @retval   EFI_DEVICE_ERROR   Failed to initialize hardware
   @retval   EFI_DEVICE_ERROR   Failed to set up link
**/
EFI_STATUS
XgbeInitHw (
  IN DRIVER_DATA *AdapterInfo
  );

#define PCI_CLASS_MASK          0xFF00
#define PCI_SUBCLASS_MASK       0x00FF

/** This function is called as early as possible during driver start to ensure the
   hardware has enough time to autonegotiate when the real SNP device initialize call is made.

   @param[in]   XgbePrivate   Pointer to driver private data

   @retval   EFI_SUCCESS        Hardware init success
   @retval   EFI_UNSUPPORTED    Shared code initialization failed
   @retval   EFI_UNSUPPORTED    Could not read MAC address
   @retval   EFI_ACCESS_DENIED  iSCSI Boot detected on port
   @retval   EFI_DEVICE_ERROR   Hardware init failed
**/
EFI_STATUS
XgbeFirstTimeInit (
  IN UNDI_PRIVATE_DATA *XgbePrivate
  );

/** This function performs PCI-E initialization for the device.

   @param[in]   AdapterInfo   Pointer to adapter structure

   @retval   EFI_SUCCESS            PCI-E initialized successfully
   @retval   EFI_UNSUPPORTED        Failed to get supported PCI command options
   @retval   EFI_UNSUPPORTED        Failed to set PCI command options
   @retval   EFI_OUT_OF_RESOURCES   The memory pages for transmit and receive resources could
                                    not be allocated
**/
EFI_STATUS
XgbePciInit (
  IN DRIVER_DATA *AdapterInfo
  );

/** Starts the receive unit.

   @param[in]   AdapterInfo   Pointer to the NIC data structure information which
                              the UNDI driver is layering on..

   @return   Receive unit started
**/
VOID
XgbeReceiveStart (
  IN DRIVER_DATA *AdapterInfo
  );

/** Stops the receive unit. Receive queue is also reset and all existing packets are dropped.

   @param[in]   AdapterInfo   Pointer to the NIC data structure information
                              which the UNDI driver is layering on..

   @return   Receive unit stopped
**/
VOID
XgbeReceiveStop (
  IN DRIVER_DATA *AdapterInfo
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

   @param[in]   AdapterInfo   pointer to the driver data
   @param[in]   CpbReceive    Pointer (Ia-64 friendly) to the command parameter Block.
                              The frame will be placed inside of it.
   @param[out]  DbReceive     The data buffer.  The out of band method of passing pre-digested
                              information to the protocol.

   @retval      PXE_STATCODE_NO_DATA        There is no data to receive.
   @retval      PXE_STATCODE_DEVICE_FAILURE AdapterInfo is NULL.
   @retval      PXE_STATCODE_DEVICE_FAILURE Device failure on packet receive.
   @retval      PXE_STATCODE_INVALID_CPB    Invalid CPB/DB parameters.
   @retval      PXE_STATCODE_NOT_STARTED    Rx queue not started.
   @retval      PXE_STATCODE_SUCCESS        Received data passed to the protocol.
**/
UINTN
XgbeReceive (
  IN  DRIVER_DATA     *AdapterInfo,
  IN  PXE_CPB_RECEIVE *CpbReceive,
  OUT PXE_DB_RECEIVE  *DbReceive
  );

/** Resets the hardware and put it all (including the PHY) into a known good state.

   @param[in]   AdapterInfo   The pointer to our context data
   @param[in]   OpFlags       The information on what else we need to do.

   @retval   PXE_STATCODE_SUCCESS        Successful hardware reset
   @retval   PXE_STATCODE_NOT_STARTED    Hardware init failed
**/
UINTN
XgbeReset (
  IN DRIVER_DATA *AdapterInfo,
  IN UINT16       OpFlags
  );

/** Configures internal interrupt causes on current PF.

   @param[in]   AdapterInfo   The pointer to our context data

   @return   Interrupt causes are configured for current PF
**/
VOID
XgbeConfigureInterrupts (
  IN DRIVER_DATA *AdapterInfo
  );

/** Disables internal interrupt causes on current PF.

   @param[in]   AdapterInfo   The pointer to our context data

   @return   Interrupt causes are disabled for current PF
**/
VOID
XgbeDisableInterrupts (
  IN DRIVER_DATA *AdapterInfo
  );

/** Changes filter settings

   @param[in]   AdapterInfo  Pointer to the NIC data structure information which the
                            UNDI driver is layering on..
   @param[in]   NewFilter   A PXE_OPFLAGS bit field indicating what filters to use.

   @return   Filters changed according to NewFilter settings
**/
VOID
XgbeSetFilter (
  IN DRIVER_DATA *AdapterInfo,
  IN UINT16       NewFilter
  );

/** Allows the protocol to control our interrupt behaviour.

   @param[in]   AdapterInfo   Pointer to the driver structure

   @retval   PXE_STATCODE_SUCCESS   Interrupt state set successfully
**/
UINTN
XgbeSetInterruptState (
  IN DRIVER_DATA *AdapterInfo
  );

/** Stop the hardware and put it all (including the PHY) into a known good state.

   @param[in]   AdapterInfo   Pointer to the driver structure

   @retval   PXE_STATCODE_SUCCESS    Hardware stopped
**/
UINTN
XgbeShutdown (
  IN DRIVER_DATA *AdapterInfo
  );

/** Updates multicast filters, updates MAC address list and enables multicast

   @param[in]   AdapterInfo   Pointer to the adapter structure

   @return   All operations in description completed
**/
VOID
XgbeSetMcastList (
  IN DRIVER_DATA *AdapterInfo
  );

/** Updates or resets field in E1000 HW statistics structure

   @param[in]   SwReg   Structure field mapped to HW register
   @param[in]   HwReg   HW register to read from

   @return   Stats reset or updated
**/
#define UPDATE_OR_RESET_STAT(SwReg, HwReg) \
  St->SwReg = (DbAddr ? (St->SwReg + (IXGBE_READ_REG (Hw, HwReg))) : 0)

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

   @param[in]   AdapterInfo  Pointer to the NIC data structure information
                             which the UNDI driver is layering on..
   @param[in]   DbAddr   The data Block address
   @param[in]   DbSize   The data Block size

   @retval   PXE_STATCODE_SUCCESS  Statistics copied successfully
**/
UINTN
XgbeStatistics (
  IN DRIVER_DATA *AdapterInfo,
  IN UINT64       DbAddr,
  IN UINT16       DbSize
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
XgbeTransmit (
  IN DRIVER_DATA *AdapterInfo,
  IN UINT64       Cpb,
  IN UINT16       OpFlags
  );

/** This function calls the MemIo callback to read a dword from the device's
   address space

   @param[in]   AdapterInfo   Adapter structure
   @param[in]   Port          Address to read from

   @retval    The data read from the port.
**/
UINT32
XgbeInDword (
  IN DRIVER_DATA *AdapterInfo,
  IN UINT32       Port
  );

/** This function calls the MemIo callback to write a word from the device's
   address space

   @param[in]   AdapterInfo   Adapter structure
   @param[in]   Port          Address to write to
   @param[in]   Data          Data to write to Port

   @return   Word written
**/
VOID
XgbeOutDword (
  IN DRIVER_DATA *AdapterInfo,
  IN UINT32       Port,
  IN UINT32       Data
  );

/** Sets specified bits in a device register

   @param[in]   AdapterInfo   Pointer to the device instance
   @param[in]   Register      Register to write
   @param[in]   BitMask       Bits to set

   @return    Returns the value read from the PCI register with BitMask applied.
**/
UINT32
XgbeSetRegBits (
  IN DRIVER_DATA *AdapterInfo,
  IN UINT32       Register,
  IN UINT32       BitMask
  );

/** Clears specified bits in a device register

   @param[in]   AdapterInfo   Pointer to the device instance
   @param[in]   Register      Register to write
   @param[in]   BitMask       Bits to clear

   @return    Returns the value read from the PCI register with ~BitMask applied.
**/
UINT32
XgbeClearRegBits (
  IN DRIVER_DATA *AdapterInfo,
  IN UINT32       Register,
  IN UINT32       BitMask
  );

/** Free TX buffers that have been transmitted by the hardware.

   @param[in]   AdapterInfo   Pointer to the NIC data structure information which
                              the UNDI driver is layering on.
   @param[in]   NumEntries    Number of entries in the array which can be freed.
   @param[out]  TxBuffer      Array to pass back free TX buffer

   @return   Number of TX buffers written.
**/
UINT16
XgbeFreeTxBuffers (
  IN  DRIVER_DATA *AdapterInfo,
  IN  UINT16       NumEntries,
  OUT UINT64      *TxBuffer
  );

/** This is the drivers copy function so it does not need to rely on the BootServices
   copy which goes away at runtime.

   This copy function allows 64-bit or 32-bit copies
   depending on platform architecture.  On Itanium we must check that both addresses
   are naturally aligned before attempting a 64-bit copy.

   @param[in]   Dest     Destination memory pointer to copy data to.
   @param[in]   Source   Source memory pointer.
   @param[in]   Count    Number of bytes to copy

   @return    Memory copied from source to destination
**/
VOID
XgbeMemCopy (
  IN UINT8  *Dest,
  IN UINT8  *Source,
  IN UINT32  Count
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

   @retval   EFI_SUCCESS            Successful operation
   @retval   EFI_INVALID_PARAMETER  Invalid parameter passed
**/
EFI_STATUS
IsLinkSpeedModifiable (
  IN  UNDI_PRIVATE_DATA  *UndiPrivateData,
  OUT BOOLEAN            *LinkSpeedModifiable
  );



/** Blinks LEDs on port managed by current PF.

   @param[in]   UndiPrivateData  Pointer to driver private data structure
   @param[in]   Time             Time in seconds to blink

   @retval  EFI_SUCCESS            LEDs blinked successfully
   @retval  EFI_DEVICE_ERROR       Failed to blink PHY link LED
   @retval  EFI_INVALID_PARAMETER  Invalid parameter passed
**/
EFI_STATUS
BlinkLeds (
  IN UNDI_PRIVATE_DATA *UndiPrivateData,
  IN UINT16            *Time
  );

/** Reads PBA string from NVM.

   @param[in]   UndiPrivateData  Pointer to driver private data structure
   @param[out]  PbaNumberStr     Output string buffer for PBA string

   @retval   EFI_SUCCESS            PBA string successfully read
   @retval   EFI_DEVICE_ERROR       Failed to read PBA flags word
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

/** Returns information whether current device supports any RDMA protocol.

  @param[in]   UndiPrivateData    Pointer to the driver private data
  @param[out]  Supported          Tells whether feature is supported

  @retval      EFI_SUCCESS        Operation successful
**/
EFI_STATUS
IsRdmaSupported (
  IN   UNDI_PRIVATE_DATA  *UndiPrivateData,
  OUT  BOOLEAN            *Supported
  );

/** Check if current module used for this port is qualified module

   @param[in]   AdapterInfo   Pointer to the NIC data structure information
                              which the UNDI driver is layering on..

   @retval   TRUE    Module is qualified
   @retval   FALSE   Module is unqualified and module
                     qualification is enabled on the port
**/
BOOLEAN
GetModuleQualificationResult (
  IN DRIVER_DATA *AdapterInfo
  );

/** Delay a specified number of microseconds

   @param[in]   AdapterInfo   Pointer to the NIC data structure information
                              which the UNDI driver is layering on..
   @param[in]   MicroSeconds   Time to delay in Microseconds.

   @return   Execution of code delayed
**/
VOID
DelayInMicroseconds (
  IN DRIVER_DATA *AdapterInfo,
  IN UINT32       MicroSeconds
  );

/** Delays code execution for specified time in milliseconds

   @param[in]   x   Time in milliseconds

   @return   Execution of code delayed
**/
#define DELAY_IN_MILLISECONDS(X)  DelayInMicroseconds (AdapterInfo, X * 1000)

/* This is a macro to convert the preprocessor defined version number into a hex value
 that can be registered with EFI. */
#define VERSION_TO_HEX  ((MAJORVERSION << 24) + (MINORVERSION << 16) + \
                        (BUILDNUMBER / 10 << 12) + (BUILDNUMBER % 10 << 8))

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

/** Check if adapter is based on 82599 (register-wise)

   @param[in]   AdapterInfo   Pointer to the NIC data structure.

   @retval   TRUE    Adapter is based on 82599
   @retval   FALSE   Adapter is not based on 82599

**/
BOOLEAN
IsNianticBasedDevice (
  IN  DRIVER_DATA   *AdapterInfo
  );

#endif /* XGBE_H_ */
