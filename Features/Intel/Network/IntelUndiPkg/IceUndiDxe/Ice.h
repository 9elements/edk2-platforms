/**************************************************************************

Copyright (c) 2016 - 2022, Intel Corporation. All Rights Reserved.

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
#ifndef ICE_H_
#define ICE_H_

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

#include <Protocol/DriverBinding.h>
#include <Protocol/DriverSupportedEfiVersion.h>
#include <Protocol/FirmwareManagement.h>
#include <Protocol/DriverHealth.h>
#include <Protocol/DriverDiagnostics.h>
#include <Protocol/PlatformToDriverConfiguration.h>
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

#include "DebugTools.h"

#include "ice_common.h"
#include "ice_switch.h"
#include "StartStop.h"
#include "AdapterInformation.h"
#include "DriverHealth.h"
#include "Dma.h"

#include "Hii/CfgAccessProt/HiiConfigAccessInfo.h"

#include "LinkTopology.h"

/* Defines & Macros */

#ifndef ICE_INTEL_VENDOR_ID
#define ICE_INTEL_VENDOR_ID INTEL_VENDOR_ID
#endif /* ICE_INTEL_VENDOR_ID */
#define INTEL_VENDOR_ID      0x8086

/* HMC context dump related defines
   Each context sub-line consists of 128 bits (16 bytes) of data */
#define SUB_LINE_LENGTH         0x10
#define LANCTXCTL_QUEUE_TYPE_TX 0x1
#define LANCTXCTL_QUEUE_TYPE_RX 0x0

// Define default port info number
#define DFLT_PORT_NUM 0

// TLV access related defines
#define NO_OFFSET               0
#define SKIP_TLV_LENGTH         1

// VSI context dump helper macros

/** Dumps VSI context register contents

   @param[in]   RegName   RegisterName

   @return  Register contents printed
**/
#define DumpVsiCtxRegister(RegName) \
          DEBUGDUMP ( \
            INIT, (# RegName "(%x) = %x\n", \
            ## RegName ## (AdapterInfo->Vsi.Id), rd32 (&AdapterInfo->Hw, ## RegName ## (VsiId))) \
          );

/** Dumps register contents

   @param[in]   RegName   RegisterName

   @return  Register contents printed
**/
#define DumpRegister(RegName) \
          DEBUGDUMP ( \
            INIT, (# RegName "(%x) = %x\n", \
            ## RegName ## , rd32 (&AdapterInfo->Hw, ## RegName ## )) \
          );

/** Returns L2TAGSTXVALID register address

   @param[in]   VSI   VSI number

   @return   Register address returned
**/
#define ICE_VSI_L2TAGSTXVALID(VSI)    (0x00042800 + ((VSI) * 4))

/* This is a macro to convert the preprocessor defined version number into a hex value
   that can be registered with EFI. */

// The version number ends with additional .1 suffix for open source version of driver
#define VERSION_TO_HEX  ((MAJORVERSION << 24) + (MINORVERSION << 16) + \
          (BUILDNUMBER / 10 << 12) + (BUILDNUMBER % 10 << 8) + 1)

#define MAX_NIC_INTERFACES 256

#define EFI_NETWORK_INTERFACE_IDENTIFIER_PROTOCOL_REVISION_31 0x00010001
#define PXE_ROMID_MINORVER_31 0x10
#define PXE_STATFLAGS_DB_WRITE_TRUNCATED  0x2000

// By default use ring size defined in inf file
#ifdef RXTX_RING_SIZE
#define ICE_NUM_TX_RX_DESCRIPTORS RXTX_RING_SIZE
#else /* NOT RXTX_RING_SIZE */
#define ICE_NUM_TX_RX_DESCRIPTORS 16
#endif /* RXTX_RING_SIZE */

// timeout for Tx/Rx queue enable/disable
#define START_RINGS_TIMEOUT 100
#define STOP_RINGS_TIMEOUT 1000

/** Retrieves RX descriptor from RX ring structure

   @param[in]   R   RX ring
   @param[in]   i   Number of descriptor

   @return   Descriptor retrieved
**/
#define ICE_RX_DESC(R, i)          \
          (&(((union ice_16byte_rx_desc *) (UINTN) ((R)->Mapping.UnmappedAddress))[i]))

/** Retrieves TX descriptor from TX ring structure

   @param[in]   R   TX ring
   @param[in]   i   Number of descriptor

   @return   Descriptor retrieved
**/
#define ICE_TX_DESC(R, i)          \
          (&(((struct ice_tx_desc *) (UINTN) ((R)->Mapping.UnmappedAddress))[i]))

#define ICE_MAX_PF_NUMBER   16

#define SPIN_LOCK_RELEASED          ((UINTN) 1)
#define SPIN_LOCK_ACQUIRED          ((UINTN) 2)

// Define some handy macros
#define UNDI_DEV_SIGNATURE             SIGNATURE_32 ('C', 'P', 'K', '4')

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

/** Retrieves UNDI_PRIVATE_DATA structure using FMP protocol instance

   @param[in]   a   Current protocol instance

   @return    UNDI_PRIVATE_DATA structure instance
**/
#define UNDI_PRIVATE_DATA_FROM_FIRMWARE_MANAGEMENT(a) \
          CR (a, UNDI_PRIVATE_DATA, FirmwareManagement, UNDI_DEV_SIGNATURE)

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

//#define ALIGN(x, a) (((x) - 1) / a + 1)

/** Helper for controller private data for() iteration loop.

   @param[in]    Iter   Driver private data iteration pointer
*/
#define FOREACH_ACTIVE_CONTROLLER(Iter)                 \
  for ((Iter) = GetFirstControllerPrivateData ();       \
       (Iter) != NULL;                                  \
       (Iter) = GetNextControllerPrivateData ((Iter)))

#define STRUCT_OFFSET  OFFSET_OF

// Definitions for NVM
#define NVM_OPERATION_TIMEOUT_IN_1MS_UNITS  80000

// PCI Base Address Register Bits
#define PCI_BAR_IO_MASK   0x00000003
#define PCI_BAR_IO_MODE   0x00000001

#define PCI_BAR_MEM_MASK  0x0000000F
#define PCI_BAR_MEM_MODE  0x00000000
#define PCI_BAR_MEM_64BIT 0x00000004

#define ETHER_MAC_ADDR_LEN  6

// Definitions for Alternate RAM
#define ICE_ALT_RAM_SIZE_IN_BYTES        8192
#define ICE_ALT_RAM_SIZE_IN_DW           (ICE_ALT_RAM_SIZE_IN_BYTES / 4)
#define ICE_AQ_ALTERNATE_ADDRESS_IGNORE  0xFFFFFFFF

#define ALT_RAM_VALID_PARAM_BIT_SHIFT    31
#define ALT_RAM_VALID_PARAM_BIT_MASK     (1 << ALT_RAM_VALID_PARAM_BIT_SHIFT)

#define ICE_ALT_RAM_LAN_MAC_ADDRESS_LOW(_PF)   (0 + 64 * (_PF))
#define ICE_ALT_RAM_LAN_MAC_ADDRESS_HIGH(_PF)  (1 + 64 * (_PF))

#define EFI_UNDI_VAR_GUID \
  { 0xDB5D57AB, 0xDB96, 0x404B, { 0xBB, 0x97, 0xDF, 0xCB, 0x2F, 0x77, 0x8B, 0x71 } }


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

// Supported Rx Buffer Sizes
#define ICE_RXBUFFER_512   512    /* Used for packet split */
#define ICE_RXBUFFER_2048  2048
#define ICE_RXBUFFER_4096  4096
#define ICE_RXBUFFER_8192  8192
#define ICE_MAX_RXBUFFER   16384  /* largest size for single descriptor */

#define ICE_RX_DTYPE_NO_SPLIT      0
#define ICE_RX_DTYPE_HEADER_SPLIT  1
#define ICE_RX_DTYPE_SPLIT_ALWAYS  2

typedef struct {
  UINT64 Packets;
  UINT64 Bytes;
  UINT64 RestartQueue;
  UINT64 TxBusy;
  UINT64 Completed;
  UINT64 TxDoneOld;
} ICE_TX_QUEUE_STATS;

typedef struct {
  UINT64 Packets;
  UINT64 Bytes;
  UINT64 NonEopDescs;
  UINT64 AllocRxPageFailed;
  UINT64 AllocRxBuffFailed;
} ICE_RX_QUEUE_STATS;

/* struct that defines a descriptor ring, associated with a VSI */
typedef struct {
  UNDI_DMA_MAPPING  RxBufferMapping;
  UINT8             **PhysicalBuffers;
  UINT8             **UnmappedBuffers;

  UINT16            Count;                 /* Number of descriptors */
  UINT16            RegIdx;                /* HW register index of the ring */
  UINT16            RxHdrLen;
  UINT16            RxBufLen;

  /* used in interrupt processing */
  UINT16 NextToUse;
  UINT16 NextToClean;

  UNDI_DMA_MAPPING    *TxBufferMappings;

  /* stats structs */
  union {
    ICE_TX_QUEUE_STATS TxStats;
    ICE_RX_QUEUE_STATS RxStats;
  } TxRxQueues;

  UINTN               Size;       /* Length of descriptor ring in bytes */
  UNDI_DMA_MAPPING    Mapping;    /* DMA mapping for descriptors area */

  UINT32              TxqTeid;   /* Queue node teid */

} ICE_RING;

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

  // Tx Rx rings
  ICE_RING RxRing;
  ICE_RING TxRing;

  UINT16             Id; /* VSI number */

  UINT16             NumDesc;
  enum ice_vsi_type  Type; /* VSI type, e.g., LAN, FCoE, etc */

  struct ice_vsi_ctx VsiCtx;
} ICE_VSI;

typedef struct DRIVER_DATA_S {
  UINT16                    State;  // stopped, started or initialized
  struct ice_hw            Hw;
  struct ice_hw_port_stats Stats;
  ICE_VSI                  Vsi;

  UINTN                     Segment;
  UINTN                     Bus;
  UINTN                     Device;
  UINTN                     Function;

  UINT8                     PciClass;
  UINT8                     PciSubClass;

  UINTN                     PhysicalPortNumber;
  UINT8                     PfPerPortMaxNumber;
  BOOLEAN                   PartitionEnabled[ICE_MAX_PF_NUMBER];
  UINT8                     PartitionPfNumber[ICE_MAX_PF_NUMBER];

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
  UINT8                     FwSupported;        // FW is not supported, AQ operations are prohibited

  BOOLEAN                   MediaStatusChecked;
  BOOLEAN                   LastMediaStatus;

  BOOLEAN                   VlanEnable;
  UINT16                    VlanTag;

  UINT64                    UniqueId;
  EFI_PCI_IO_PROTOCOL      *PciIo;
  UINT64                    OriginalPciAttributes;
  BOOLEAN                   AriCapabilityEnabled;

  BOOLEAN                   NvmAcquired; // Field specific for NUL semaphore management.

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
  UINT8  CurrentPromiscuousMask;

  // Store info if packets are pending report in UndiStatus
  BOOLEAN RxPacketPending;
  BOOLEAN TxPacketPending;
  UINT16  LastRxDescReported;
  UINT16  LastTxDescReported;

  UINT16 CurRxInd;
  UINT16 CurTxInd;
  UINT8  ReceiveStarted;

  UINT16             XmitDoneHead;
  BOOLEAN            MacAddrOverride;
  UINTN              VersionFlag; // Indicates UNDI version 3.0 or 3.1
  UINT16             TxRxDescriptorCount;
} DRIVER_DATA;

typedef struct HII_INFO_S {
  EFI_HANDLE                       HiiInstallHandle;
  EFI_HII_HANDLE                   HiiPkgListHandle;
  HII_CFG_ACCESS_INFO              HiiCfgAccessInfo;

  BOOLEAN    AltMacAddrSupported;
  BOOLEAN    EmprRequired;         ///< Synchronization flag for PostRoute for settings that require EMPR.
  BOOLEAN    PendingPortOptionValid;
  BOOLEAN    PortOptionChangeRequested; ///< Required for Port Option to set defaults
  PORT_OPTION_SPEED    MaxPortSpeed;
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
extern EFI_GUID                                  gEfiNiiPointerGuid;

extern PXE_SW_UNDI                              *mPxe31;
extern UNDI_PRIVATE_DATA                        *mUndi32DeviceList[MAX_NIC_INTERFACES];
extern BOOLEAN                                   mExitBootServicesTriggered;

extern UINT8 IceUndiDxeStrings[];


/* Function declarations */

/** This function performs PCI-E initialization for the device.

   @param[in]   AdapterInfo  Pointer to the NIC data structure information
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
IcePciInit (
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
IceInitHw (
  IN DRIVER_DATA *AdapterInfo
  );

/** Stops Rx and Tx rings.

   @param[in]   AdapterInfo   Pointer to the NIC data structure information
                              the UNDI driver is layering on

   @retval     ICE_SUCCESS      TX/RX rings started successfully
   @retval     ICE_ERR_TIMEOUT  Waiting for TX queue status timed out
   @retval     ICE_ERR_TIMEOUT  Waiting for RX queue status timed out
**/
enum ice_status
IceReceiveStop (
  IN DRIVER_DATA *AdapterInfo
  );

/** Performs IceInitHw function for UNDI interface

   @param[in]   AdapterInfo   Pointer to the NIC data structure information
                             the UNDI driver is layering on

   @retval    PXE_STATCODE_SUCCESS   HW initialized successfully
   @retval    PXE_STATCODE_NOT_STARTED  Failed to initialize HW
**/
PXE_STATCODE
IceInitialize (
  IN DRIVER_DATA *AdapterInfo
  );

/** Reverts the operations performed in IceInitHw. Stops HW from child side

   @param[in]   AdapterInfo   Pointer to the NIC data structure information
                              the UNDI driver is layering on

   @retval   PXE_STATCODE_SUCCESS   HW is already not initialized
   @retval   PXE_STATCODE_SUCCESS   HW successfully stopped
**/
PXE_STATCODE
IceShutdown (
  IN DRIVER_DATA *AdapterInfo
  );

/** Performs HW reset by reinitialization

   @param[in]   AdapterInfo   Pointer to the NIC data structure information
                              the UNDI driver is layering on

   @retval   PXE_STATCODE_SUCCESS      Successfull HW reset
   @retval   PXE_STATCODE_NOT_STARTED  Failed to initialize HW
**/
PXE_STATCODE
IceReset (
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
IceFreeTxBuffers (
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
   @retval   EFI_OUT_OF_RESOURCES   Failed to allocate memory for PortInfo
**/
EFI_STATUS
IceFirstTimeInit (
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
IceRead32 (
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
IceWrite32 (
  IN DRIVER_DATA *AdapterInfo,
  IN UINT32       Port,
  IN UINT32       Data
  );

/** This function calls the IceRead32 twice to read a qword from the device's
   address space

   @param[in]   AdapterInfo   Pointer to the NIC data structure information
                             the UNDI driver is layering on
   @param[in]   Port         Address to read from

   @return      The data read from the port.
**/
UINT64
IceRead64 (
  IN DRIVER_DATA *AdapterInfo,
  IN UINT32       Port
  );

/** This function calls the IceWrite32 twice to write a qword to the device's
   address space

   @param[in]   AdapterInfo   Pointer to the NIC data structure information
                             the UNDI driver is layering on
   @param[in]   Port         Address to write to
   @param[in]   Data         Data to write to Port

   @return    Data written to address in device's space
**/
VOID
IceWrite64 (
  IN DRIVER_DATA *AdapterInfo,
  IN UINT32       Port,
  IN UINT64       Data
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

  @retval     PXE_STATCODE_NO_DATA  There is no data to receive
  @retval     PXE_STATCODE_SUCCESS  Received data passed to the protocol.
**/
UINTN
IceReceive (
  IN DRIVER_DATA     *AdapterInfo,
  IN PXE_CPB_RECEIVE *CpbReceive,
  IN PXE_DB_RECEIVE  *DbReceive
  );

#define ICE_TXD_CMD (ICE_TX_DESC_CMD_EOP | ICE_TX_DESC_CMD_RS)

/** Takes a command block pointer (cpb) and sends the frame.

  Takes either one fragment or many and places them onto the wire.
  Cleanup of the send happens in the function UNDI_Status in Decode.c

  @param[in]  AdapterInfo   Pointer to the NIC data structure information
                             the UNDI driver is layering on
  @param[in]  cpb           The command parameter block address.
                            64 bits since this is Itanium(tm) processor friendly
  @param[in]  OpFlags       The operation flags, tells if there is any special
                            sauce on this transmit

  @retval     PXE_STATCODE_SUCCESS        The frame goes out
  @retval     PXE_STATCODE_DEVICE_FAILURE The frame does not go out
  @retval     PXE_STATCODE_BUSY           Need to call again later
**/
UINTN
IceTransmit (
  IN DRIVER_DATA *AdapterInfo,
  IN UINT64       Cpb,
  IN UINT16       OpFlags
  );

#define ANY_PROMISCUOUS_FILTER_SET_MASK  (PXE_OPFLAGS_RECEIVE_FILTER_UNICAST       | \
                                          PXE_OPFLAGS_RECEIVE_FILTER_BROADCAST     | \
                                          PXE_OPFLAGS_RECEIVE_FILTER_PROMISCUOUS   | \
                                          PXE_OPFLAGS_RECEIVE_FILTER_ALL_MULTICAST)

/** Sets receive filters.

  @param[in]  AdapterInfo  Pointer to the adapter structure
  @param[in]  NewFilter    A PXE_OPFLAGS bit field indicating what filters to use.

  @return     Broad/Multicast and promiscous settings are set according to NewFilter
**/
VOID
IceSetFilter (
  IN DRIVER_DATA *AdapterInfo,
  IN UINT16       NewFilter
  );

/** Clears receive filters.

  @param[in]  AdapterInfo  Pointer to the adapter structure
  @param[in]  NewFilter    A PXE_OPFLAGS bit field indicating what filters to clear.

  @return     Broad/Multicast and promiscous settings are cleared according to NewFilter
**/
VOID
IceClearFilter (
  IN DRIVER_DATA *AdapterInfo,
  IN UINT16       NewFilter
  );

/** Adds MAC/VLAN elements to multicast list

   @param[in]   AdapterInfo   Pointer to the NIC data structure information
                              the UNDI driver is layering on

   @return  MAC/VLAN elements from adapter VSI structure are added to list
**/
VOID
IceSetMcastList (
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
   @retval  EFI_DEVICE_ERROR  Failed to set LED state
**/
EFI_STATUS
BlinkLeds (
  IN  UNDI_PRIVATE_DATA  *UndiPrivateData,
  IN  UINT16             *Time
  );

/** Gets HII formset help string ID.

   @param[in]   UndiPrivateData  Pointer to driver private data structure

   @return   EFI_STRING_ID of formset help string
**/
EFI_STRING_ID
GetFormSetHelpStringId (
  IN UNDI_PRIVATE_DATA  *UndiPrivateData
  );


/** Read VSI parameters

  @param[in]    AdapterInfo         Pointer to the NIC data structure information
                                    which the UNDI driver is layerin

  @param[out]   VsiCtx             resulting VSI context

  @retval       EFI_SUCCESS         VSI context successfully read
  @retval       EFI_DEVICE_ERROR    VSI context read error
**/
EFI_STATUS
IceGetVsiParams (
  IN  DRIVER_DATA        *AdapterInfo,
  OUT struct ice_vsi_ctx *VsiCtx
  );

#define ICE_GLNVM_ULD_TIMEOUT    1000000

/** Checks if reset is done.

   @param[in]   AdapterInfo   Pointer to the NIC data structure information
                              the UNDI driver is layering on
   @param[in]   ResetMask     Mask to compare with read reg. value if reset was done

   @retval    EFI_SUCCESS       Function ended successfully
   @retval    EFI_DEVICE_ERROR  Timeout when waiting for device to become active
   @retval    EFI_DEVICE_ERROR  Timeout when waiting for reset done
**/
EFI_STATUS
IceCheckResetDone (
  IN DRIVER_DATA *AdapterInfo,
  IN UINT32       ResetMask
  );

#ifdef LEGACY_IN_BACKGROUND
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
IceAquireControllerHw (
  IN DRIVER_DATA *AdapterInfo
  );

/** Release this PF by clearing the bit in PFGEN_DRUN register.

   @param[in]   AdapterInfo   Pointer to the NIC data structure information
                             the UNDI driver is layering on

   @return   PFGEN_DRUN driver unload bit is cleared
**/
VOID
IceReleaseControllerHw (
  IN DRIVER_DATA *AdapterInfo
  );
#endif /* LEGACY_IN_BACKGROUND */

/** Reads MAC address into port_info->mac.lan_addr structure

   @param[in]   AdapterInfo   Pointer to the NIC data structure information
                              the UNDI driver is layering on

   @retval    EFI_SUCCESS       MAC address read successfully
   @retval    EFI_DEVICE_ERROR  Failed to get MAC address
**/
EFI_STATUS
IceReadMacAddress (
  IN DRIVER_DATA *AdapterInfo
  );

/** Gets CVL chip type string.

   @param[in]   UndiPrivateData   Points to the driver instance private data
   @param[out]  ChipTypeStr       Points to the output string buffer

   @retval   EFI_SUCCESS   Chip type string successfully retrieved
**/
EFI_STATUS
GetChipTypeStr (
  IN  UNDI_PRIVATE_DATA  *UndiPrivateData,
  OUT EFI_STRING         ChipTypeStr
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

/** Checks if FMP update needs to be done EEPROM only

   @param[in]   UndiPrivateData   Points to the driver instance private data.

   @retval   FALSE    No 40 gig device has EEPROM only FMP
**/
BOOLEAN
IsFlashlessSku (
  IN UNDI_PRIVATE_DATA *UndiPrivateData
  );

#ifdef VPD_CONFIG_ACCESS

#define VPDADDR 0xE2
#define VPDDATA 0xE4
#define VPD_READ_MASK  0x0000
#define VPD_WRITE_MASK  0x8000
#define VPD_FLAG_MASK  0x8000

/** Reads VPD region

   @param[in]   AdapterInfo   Pointer to the NIC data structure information
                              the UNDI driver is layering on
   @param[out]  Buffer       Pointer to buffer for resulting VPD data
   @param[in]   BufferSize   Size of passed buffer

   @retval  EFI_SUCCESS  VPD region read successfully
**/
EFI_STATUS
ReadVpdRegion (
  IN  DRIVER_DATA *AdapterInfo,
  OUT UINT32      *Buffer,
  IN  UINTN        BufferSize
  );

/** Writes VPD region

   @param[in]   AdapterInfo   Pointer to the NIC data structure information
                             the UNDI driver is layering on
   @param[in]   Buffer       Pointer to buffer with VPD data to write
   @param[in]   BufferSize   Size of buffer

   @retval  EFI_SUCCESS   VPD region written successfully
**/
EFI_STATUS
WriteVpdRegion (
  IN DRIVER_DATA *AdapterInfo,
  IN UINT32      *Buffer,
  IN UINTN        BufferSize
  );
#endif /* VPD_CONFIG_ACCESS */

/** Stop all drivers managing the current adapter except the calling instance of driver

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

/** Enables PXE mode descriptor fetch policy (one at a time).

   @param[in]  AdapterInfo   Driver private data structure
 **/
VOID
EnablePxeModeDescFetch (
  IN DRIVER_DATA  *AdapterInfo
  );

/** Get supported Tx/Rx descriptor count for a given device.

   @param[in]    Hw         Pointer to the HW Structure

   @return       Supported Tx/RX descriptors count
**/
UINT16
IceGetTxRxDescriptorsCount (
  IN struct ice_hw *Hw
  );

/** Gets ETRACKID.

   @param[in]   UndiPrivateData   Pointer to the driver data
   @param[out]  EtrackId          Unique NVM identifier

   @retval      EFI_SUCCESS            EtrackId returned correctly
   @retval      EFI_DEVICE_ERROR       HW is not initialized
   @retval      EFI_INVALID_PARAMETER  Invalid input parameter
**/
EFI_STATUS
GetEtrackId (
  IN  UNDI_PRIVATE_DATA *UndiPrivateData,
  OUT UINT32            *EtrackId
  );

/** Clear the admin receive queue.

   @param[in]   AdapterInfo   Pointer to the driver data

   @retval     EFI_DEVICE_ERROR   Waiting for ARQ clear timeout
   @retval     EFI_SUCCESS        Receive queue cleared successfully
**/
EFI_STATUS
ClearAdminReceiveQueue (
  IN DRIVER_DATA *AdapterInfo
  );

/** Wait for the specific admin queue event to occure.

   @param[in]   AdapterInfo   Pointer to the driver data
   @param[in]   Opcode        Opcode of the event
   @param[in]   Timeout       Timeout in 1ms units

   @retval     EFI_DEVICE_ERROR   Waiting for ARQ NVM erase response timeout
   @retval     EFI_SUCCESS        Successfull wait
**/
EFI_STATUS
AwaitReceiveQueueEvent (
  IN DRIVER_DATA         *AdapterInfo,
  IN enum ice_adminq_opc  Opcode,
  IN UINT32               Timeout
  );

/** Blocking function called to assure that we are not swapped out from
   the queue while moving TX ring tail pointer.

   @param[in]   AdapterInfo   Pointer to the NIC data structure information
                             the UNDI driver is layering on
   @param[in]   Flag         Block flag

   @return   According to Flag setting (TRUE/FALSE) we're acquiring or releasing EFI lock
**/
VOID
IceBlockIt (
  IN DRIVER_DATA *AdapterInfo,
  IN UINT32       Flag
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

/** Checks if Firmware is able to change Tx scheduler topology.

   @param[in]   UndiPrivateData  Pointer to the driver private data.
   @param[out]  Supported        Tells whether the feature is supported.

   @retval   EFI_SUCCESS       Successfully retrieved the information about support.
   @retval   EFI_DEVICE_ERROR  Failed to retrieve the information about support.
**/
EFI_STATUS
IsTxSchedulerTopologyChangeSupported (
  IN   UNDI_PRIVATE_DATA  *UndiPrivateData,
  OUT  BOOLEAN            *Supported
  );

/** Checks if Firmware is in pending reboot state.

   @param[in]   AdapterInfo  Pointer to the NIC data structure information which
                             the UNDI driver is layering on

   @retval   TRUE   Firmware is in pending reboot state
   @retval   FALSE  Firmware is not in pending reboot state
**/
BOOLEAN
IsPendingRebootState (
  IN DRIVER_DATA *AdapterInfo
  );

/** Checks if Firmware is in lockdown state.

   @param[in]   AdapterInfo  Pointer to the NIC data structure information which
                             the UNDI driver is layering on

   @retval   TRUE   Firmware is in lockdown state
   @retval   FALSE  Firmware is not in lockdown state
**/
BOOLEAN
IsFwLockdownState (
  IN DRIVER_DATA *AdapterInfo
  );

/** Read TLV module located in the PFA.

   @param[in]  UndiPrivateData    Pointer to the driver data
   @param[in]  ModuleTypeId       The pointer to the module
   @param[in]  Offset             Word offset within module
   @param[in]  Length             The length of data to be read (in bytes)
   @param[in]  Data               Pointer to data buffer

   @retval     EFI_SUCCESS            TLV module read successfully
   @retval     EFI_INVALID_PARAMETER  ModuleData is NULL
   @retval     EFI_DEVICE_ERROR       Failed to read TLV module
   @retval     EFI_DEVICE_ERROR       Failed to acquire NVM resource
   @retval     EFI_NOT_FOUND          EPERM status returned due to invalid TLV (TLV does not exist)
**/
EFI_STATUS
ReadTlv (
  IN UNDI_PRIVATE_DATA *UndiPrivateData,
  IN UINT16             ModuleTypeId,
  IN UINT16             Offset,
  IN UINT32             Length,
  IN VOID              *Data
  );

/** Write to TLV module located in the PFA.

   @param[in]  UndiPrivateData    Pointer to the driver data
   @param[in]  ModuleTypeId       The pointer to the module
   @param[in]  Offset             Word offset within module
   @param[in]  Length             The length of data to be written (in bytes)
   @param[in]  Data               Pointer to data buffer

   @retval     EFI_SUCCESS            TLV module written successfully
   @retval     EFI_INVALID_PARAMETER  Invalid ModulePointer
   @retval     EFI_INVALID_PARAMETER  ModuleData is NULL
   @retval     EFI_DEVICE_ERROR       Failed to write TLV module
   @retval     EFI_DEVICE_ERROR       Failed to acquire NVM resource
**/
EFI_STATUS
WriteTlv (
  IN UNDI_PRIVATE_DATA *UndiPrivateData,
  IN UINT16             ModuleTypeId,
  IN UINT16             Offset,
  IN UINT32             Length,
  IN VOID              *Data
  );

/** Read VPD TLV module to preallocated buffer.

   @param[in]  UndiPrivateData        Pointer to the driver data
   @param[out] VpdBuffer              Pointer to the output buffer
   @param[out] VpdSizeInWords         Length of read VPD TLV

   @retval     EFI_SUCCESS            VPD written to buffer successfully
   @retval     EFI_INVALID_PARAMETER  UndiPrivateData is NULL or VpdBuffer is null
   @retval     EFI_DEVICE_ERROR       Failed to read TLV module
   @retval     EFI_DEVICE_ERROR       Failed to acquire NVM resource
   @retval     EFI_DEVICE_ERROR       Failed to find VPD TLV
**/
EFI_STATUS
IceReadVpdBuffer (
  IN  UNDI_PRIVATE_DATA *UndiPrivateData,
  OUT UINT16            *VpdBuffer,
  OUT UINT16            *VpdSizeInWords
  );

/** Write VPD buffer into TLV module.

   @param[in]  UndiPrivateData        Pointer to the driver data
   @param[out] VpdBuffer              Pointer to the output buffer
   @param[in]  VpdSizeInWords         Length VPD buffer to be written into TLV

   @retval     EFI_SUCCESS            VPD written to buffer successfully
   @retval     EFI_INVALID_PARAMETER  UndiPrivateData is NULL or VpdBuffer is null
   @retval     EFI_DEVICE_ERROR       Failed to write TLV module
   @retval     EFI_DEVICE_ERROR       Failed to acquire NVM resource
   @retval     EFI_DEVICE_ERROR       Failed to find VPD TLV
**/
EFI_STATUS
IceWriteVpdBuffer (
  IN  UNDI_PRIVATE_DATA *UndiPrivateData,
  OUT UINT16            *VpdBuffer,
  IN  UINT16            *VpdSizeInWords
  );

/** Checks if device is Columbiaville.

   @param[in]   UndiPrivateData    Pointer to the driver private data.
   @param[out]  Supported          Tells whether the feature is supported.

   @retval   EFI_SUCCESS     Successfully retrieved the information about support.
**/
EFI_STATUS
IsCvlDevice (
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


#endif /* ICE_H_ */
