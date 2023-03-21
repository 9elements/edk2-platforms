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

#ifndef EEPROM_CONFIG_H_
#define EEPROM_CONFIG_H_

#include "Ice.h"
#include "Hii/FormsetStd/HiiConfigData.h"

/* Typedefs */
typedef union {
  UINT8 Value;
  struct {
    BOOLEAN OverridePhyTypes                      : 1;
    BOOLEAN OverrideDisableAutomaticLinkOnStartup : 1;
    BOOLEAN OverrideEeeSetting                    : 1;
    BOOLEAN OverridePauseSetting                  : 1;
    BOOLEAN OverrideLesmEnable                    : 1;
    BOOLEAN OverrideFecSetting                    : 1;
  } Bit;
} OVERRIDE_BY_FUNCTION;

/* Defines */

#define SWAP_DWORD_WORDS(a) ((UINT32)(((a) & 0x0000FFFF) << 16) | (((a) & 0xFFFF0000) >> 16))

// EEPROM capabilities word
#define EEPROM_CAPABILITIES_WORD 0x33
#define EEPROM_CAPABILITIES_SIG  0x4000

// EEPROM size
#define EEPROM_SIZE_32K_WORDS    32768

// EMP Settings in Shared RAM mapped area
#define NVM_EMP_SR_SETTINGS_MODULE_PTR            0x48

#define ICE_VF_PER_DEVICE_MAX                 256

#define PF_MAC_ADDRESS_MODULE       0x10F
#define PF_MAC_IN_TLV_LEN_WORDS     4

#define TLV_ID_OEM_SUPPORT         0x128
#define OFFSET_CAP_DATA_FLAGS_LOW  0x000A
#define ON_CHIP_THERM_SENS_BIT     BIT (9)
#define TLV_ID_CURRENT_LLDP        0x129
#define TLV_ID_MIN_SREV            0x130
#define TLV_ID_NETLIST_MIN_SREV    0x146
#define NVM_MIN_SREV_VALID         BIT(0)
#define OROM_MIN_SREV_VALID        BIT(1)

#define RDMA_ALTRAM_CFG               19
#define RDMA_ALTRAM_CFG_DIS           0
#define RDMA_ALTRAM_CFG_DIS_OVERRIDE  1
#define RDMA_ALTRAM_CFG_ENA           2
#define RDMA_ALTRAM_CFG_ENA_OVERRIDE  3

#define RDMA_CTRL_TLV                   0x135
#define RDMA_CTRL_TLV_TOPO              BIT (0)
#define RDMA_CTRL_TLV_LOW_OVERRIDE_ENA  BIT (1)
#define RDMA_CTRL_TLV_LOW_OVERRIDE_VAL  BIT (2)
#define RDMA_CTRL_TLV_HI_OVERRIDE_ENA   BIT (3)
#define RDMA_CTRL_TLV_HI_OVERRIDE_VAL   BIT (4)

#define ALTRAM_BIOS_MODE 0
#define ALTRAM_UEFI_MODE 1

#define DEFAULT_OVERRIDE_MASK_TLV           0x134

#define TLV_ID_TST_US                       0x14B
#define TOPOLOGY_SOURCE_BIT                 BIT (4)

#define ICE_PF_SWITCH_MODE_MEM_SPACE_ID     0x0015
#define ICE_PF_SWITCH_MODE_ENABLE           0x0001
#define ICE_PF_SWITCH_MODE_DISABLE          0x0002
#define ICE_PASID_ID                        0x0019
#define ICE_PASID_ENABLE                    0x0001
#define ICE_PASID_DISABLE                   0x0002

/* Structs */
#pragma pack(1)
typedef struct LINK_DEFAULT_OVERRIDE_MASK_S {
  BOOLEAN                MediaDetectionMode              : 1; ///< TRUE = Strict, FALSE = Lenient
  BOOLEAN                EpctMediaDetectionChangeability : 1;
  BOOLEAN                PortDisable                     : 1;
  BOOLEAN                OverrideEnable                  : 1;
  BOOLEAN                DisableAutomaticLinkOnStartup   : 1;
  BOOLEAN                EeeEnable                       : 1;
  UINT8                  Reserved1                       : 2;
  UINT8                  PauseAbility                    : 2;
  UINT8                  Reserved2                       : 4;
  BOOLEAN                LesmEnable                      : 1;
  BOOLEAN                AutoFecEnable                   : 1;
  UINT8                  SetPhyConfigByte22;
  OVERRIDE_BY_FUNCTION   OverrideByFunction;
  UINT64                 PhyTypesLow;
  UINT64                 PhyTypesHigh;
} LINK_DEFAULT_OVERRIDE_MASK;
#pragma pack()

/* Interface function declarations */

/** Write SR buffer using shared code implementation.

   @param[in]   AdapterInfo    Points to the driver information
   @param[in]   ModulePointer  Pointer to module in words with respect to NVM beginning
   @param[in]   Offset         offset in words from module start
   @param[in]   Words          Number of words to write
   @param[in]   Data           Pointer to location with data to be written

   @retval    EFI_SUCCESS        Buffer successfully written
   @retval    EFI_ACCESS_DENIED  Access to desired NVM memory range is denied
   @retval    EFI_DEVICE_ERROR   Failed to write buffer
   @retval    EFI_DEVICE_ERROR   Waiting for ARQ response timeout
**/
EFI_STATUS
IceWriteNvmBuffer (
  IN DRIVER_DATA *AdapterInfo,
  IN UINT8        ModulePointer,
  IN UINT32       Offset,
  IN UINT16       Words,
  IN VOID        *Data
  );

/** Writes data buffer to nvm using __i40e_write_nvm_buffer shared code function.

   Function works around the situation when the buffer spreads over two sectors.
   The entire buffer must be located inside the Shared RAM.

   @param[in]   AdapterInfo   Points to the driver information
   @param[in]   Offset        Buffer offset from the start of NVM
   @param[in]   Words         Number of words to write
   @param[in]   Data          Pointer to location with data to be written

   @retval   EFI_SUCCESS       NVM buffer written successfully
   @retval   EFI_DEVICE_ERROR  Failed to write buffer (or either of the sectors)
**/
EFI_STATUS
IceWriteNvmBufferExt (
  IN DRIVER_DATA *AdapterInfo,
  IN UINT32       Offset,
  IN UINT16       Words,
  IN VOID        *Data
  );

/** Restores the factory default MAC address for currently managed PF.

   @param[in]   UndiPrivateData   Pointer to driver private data structure

   @retval      EFI_SUCCESS       New MAC address set successfully
   @retval      EFI_DEVICE_ERROR  Failed to invalidate Alternate RAM entry
**/
EFI_STATUS
RestoreDefaultMacAddress (
  IN  UNDI_PRIVATE_DATA  *UndiPrivateData
  );

/** Does nothing. Necessary for UndiCommon to compile

   @param[in]   UndiPrivateData  Pointer to driver private data structure

   @retval      EFI_SUCCESS      Always returned
**/
EFI_STATUS
FixFwsm31Bit (
  IN UNDI_PRIVATE_DATA *UndiPrivateData
  );

/** Gets factory MAC addresses for PF0.

   @param[in]   UndiPrivateData      Pointer to driver private data structure
   @param[out]  FactoryMacAddress    Pointer to buffer for resulting factory
                                     MAC address

   @retval      EFI_SUCCESS       MAC addresses read successfully
   @retval      !EFI_SUCCESS      Failure of underlying function
**/
EFI_STATUS
GetFactoryMacAddressForPf0 (
  IN   UNDI_PRIVATE_DATA  *UndiPrivateData,
  OUT  UINT8              *FactoryMacAddress
  );

/** Sets alternate MAC address for currently managed PF.

   @param[in]   UndiPrivateData      Pointer to driver private data structure
   @param[in]   AlternateMacAddress  Value to set the MAC address to

   @retval      EFI_SUCCESS       New MAC address set successfully
   @retval      EFI_DEVICE_ERROR  Failed to write new MAC value to alt. RAM
**/
EFI_STATUS
SetAlternateMacAddress (
  IN  UNDI_PRIVATE_DATA  *UndiPrivateData,
  IN  UINT8              *AlternateMacAddress
  );

/** Reads factory default MAC address for specified PF.

   @param[in]   UndiPrivateData      Pointer to driver private data structure
   @param[in]   PhysicalFunction     Number of PF to read the MAC Addresses of
   @param[out]  FactoryMacAddress    Factory default MAC address of the adapter

   @retval      EFI_SUCCESS    MAC addresses read successfully
   @retval      !EFI_SUCCESS   Failed to read PF_MAC_ADDRESS_MODULE TLV
**/
EFI_STATUS
GetFactoryMacAddressForPf (
  IN  UNDI_PRIVATE_DATA  *UndiPrivateData,
  IN  UINT8              PhysicalFunction,
  OUT UINT8              *FactoryMacAddress
  );

/** Reads factory default MAC address.

   @param[in]   UndiPrivateData      Pointer to driver private data structure
   @param[out]  FactoryMacAddress    Factory default MAC address of the adapter

   @retval      EFI_SUCCESS     MAC addresses read successfully
   @retval      !EFI_SUCCESS    Failure of underlying function
**/
EFI_STATUS
GetFactoryMacAddress (
  IN   UNDI_PRIVATE_DATA  *UndiPrivateData,
  OUT  UINT8              *FactoryMacAddress
  );

/** Gets alternate MAC address of currently managed PF.

   @param[in]   UndiPrivateData      Pointer to driver private data structure
   @param[out]  AlternateMacAddress  Pointer to buffer for resulting alternate
                                     MAC address

   @retval      EFI_SUCCESS       MAC addresses read successfully
   @retval      EFI_DEVICE_ERROR  Failed to read alternate MAC addr from Alt. RAM
**/
EFI_STATUS
GetAlternateMacAddress (
  IN   UNDI_PRIVATE_DATA  *UndiPrivateData,
  OUT  UINT8              *AlternateMacAddress
  );

/** Returns EEPROM capabilities word (0x33) for current adapter

   @param[in]   UndiPrivateData    Points to the driver instance private data
   @param[out]  CapabilitiesWord   EEPROM capabilities word (0x33) for current adapter

   @retval   EFI_SUCCESS       Capabilities word successfully read
   @retval   EFI_DEVICE_ERROR  Failed to read capabilities word
**/
EFI_STATUS
EepromGetCapabilitiesWord (
  IN  UNDI_PRIVATE_DATA *UndiPrivateData,
  OUT UINT16            *CapabilitiesWord
  );

/** Updates NVM checksum.

   @param[in]   UndiPrivateData   Pointer to driver private data structure

   @retval      EFI_SUCCESS       Checksum successfully updated
   @retval      EFI_DEVICE_ERROR  Failed to acquire NVM
   @retval      EFI_DEVICE_ERROR  Failed to update NVM checksum
   @retval      !EFI_SUCCESS      Failure of underlying function
**/
EFI_STATUS
UpdateNvmChecksum (
  IN  UNDI_PRIVATE_DATA  *UndiPrivateData
  );

/** Reads PBA string from NVM.

   @param[in]   UndiPrivateData  Pointer to driver private data structure
   @param[out]  PbaNumberStr     Output string buffer for PBA string

   @retval   EFI_SUCCESS            PBA string successfully read
   @retval   EFI_SUCCESS            PBA string is unsupported by the adapter
   @retval   EFI_DEVICE_ERROR       Failure of underlying shared code function
**/
EFI_STATUS
GetPbaStr (
  IN  UNDI_PRIVATE_DATA  *UndiPrivateData,
  OUT EFI_STRING         PbaNumberStr
  );


/** Checks if FW LLDP Agent status is supported.

   @param[in]   UndiPrivateData  Pointer to driver private data structure
   @param[out]  Supported        Tells whether LLDP Agent is supported

   @retval      EFI_SUCCESS  LLDP Agent is supported.
**/
EFI_STATUS
IsLldpAgentSupported (
  IN   UNDI_PRIVATE_DATA  *UndiPrivateData,
  OUT  BOOLEAN            *Supported
  );

/** Reads current FW LLDP Agent status.

   @param[in]   UndiPrivateData   Pointer to driver private data structure
   @param[out]  LldpAgentEna      Pointer to variable which will store read LLDP Admin status

   @retval      EFI_SUCCESS            LLDP Agent status read successfully.
   @retval      EFI_DEVICE_ERROR       Failed to read LLDP Agent status.
   @retval      EFI_DEVICE_ERROR       Out of range value read from NVM.
**/
EFI_STATUS
GetLldpAgentStatus (
  IN   UNDI_PRIVATE_DATA  *UndiPrivateData,
  OUT  BOOLEAN            *LldpAgentEna
  );

/** Sets FW LLDP Agent status.

   @param[in]   UndiPrivateData  Pointer to driver private data structure
   @param[in]   LldpAgentEna     Requested LLDP Agent status

   @retval      EFI_SUCCESS         LLDP Agent Status written successfully
   @retval      EFI_DEVICE_ERROR    Failed to read current LLDP Agent status
   @retval      EFI_SUCCESS         Requested LLDP agent status matches current
   @retval      EFI_DEVICE_ERROR    Failed to start or stop LLDP Agent
   @retval      EFI_DEVICE_ERROR    Failed to set DCB parameters
**/
EFI_STATUS
SetLldpAgentStatus (
  IN  UNDI_PRIVATE_DATA  *UndiPrivateData,
  IN  BOOLEAN            *LldpAgentEna
  );

/** Get default LLDP Agent status.

   @param[in]   UndiPrivateData      Pointer to driver private data structure.
   @param[out]  DefaultLldpAgentEna  Pointer to variable which should store default value for LLDP Agent.

   @retval      EFI_SUCCESS          LLDP Agent get default successful.
   @retval      EFI_DEVICE_ERROR     Out of range value read from NVM.
   @retval      !EFI_SUCCESS         Failed to get default LLDP Agent.
**/
EFI_STATUS
GetDfltLldpAgentStatus (
  IN  UNDI_PRIVATE_DATA  *UndiPrivateData,
  OUT BOOLEAN            *DefaultLldpAgentEna
  );








/** Operations executed pre RouteConfig() map processing, needed for 100G driver.

   @param[in]   UndiPrivateData  Pointer to driver private data structure
   @param[in]   VarStoreMapCfg   HII varstore map configuration structure
   @param[in]   HiiCfgData       Pointer to configuration data buffer (of varstore type)
   @param[in]   Configuration    RouteConfig Configuration string

   @retval      EFI_SUCCESS      Operation successful
   @retval      !EFI_SUCCESS     Failed to get current active port option number
   @retval      !EFI_SUCCESS     Failed to get information if active port option field is present
                                 in configuration request
**/
EFI_STATUS
HiiAdapterPreRoute (
  IN       UNDI_PRIVATE_DATA     *UndiPrivateData,
  IN       HII_VARSTORE_MAP_CFG  *VarStoreMapCfg,
  IN       HII_STD_VARSTORE      *HiiCfgData,
  IN CONST EFI_STRING            Configuration
  );

/** Operations executed post RouteConfig() map processing, needed for 100G driver.

   @param[in]   UndiPrivateData  Pointer to driver private data structure
   @param[in]   VarStoreMapCfg   HII varstore map configuration structure

   @retval      EFI_SUCCESS      Operation successful
**/
EFI_STATUS
HiiAdapterPostRoute (
  IN  UNDI_PRIVATE_DATA     *UndiPrivateData,
  IN  HII_VARSTORE_MAP_CFG  *VarStoreMapCfg
  );

/** Gets S-IOV feature setting.

   @param[in]   UndiPrivateData      Pointer to driver private data structure.
   @param[out]  SiovEnabled          S-IOV feature setting.

   @retval   EFI_SUCCESS             S-IOV feature successfully returned.
   @retval   EFI_DEVICE_ERROR        Failed to read S-IOV feature configuration from NVM.
   @retval   EFI_NOT_READY           Failed to acquire NVM.
**/
EFI_STATUS
GetSiovFeature (
  IN  UNDI_PRIVATE_DATA  *UndiPrivateData,
  OUT BOOLEAN            *SiovEnabled
  );

/** Sets S-IOV feature.

   @param[in]   UndiPrivateData     Pointer to driver private data structure.
   @param[in]   SiovEnabled         Desired S-IOV feature setting.

   @retval   EFI_SUCCESS            S-IOV feature successfully set.
   @retval   EFI_DEVICE_ERROR       Failed to write S-IOV feature configuration to NVM.
   @retval   EFI_NOT_READY          Failed to acquire NVM.
**/
EFI_STATUS
SetSiovFeature (
  IN  UNDI_PRIVATE_DATA  *UndiPrivateData,
  IN  BOOLEAN            *SiovEnabled
  );

/** Sets Transmit Balancing.

   @param[in]   UndiPrivateData           Pointer to driver private data structure.
   @param[in]   TransmitBalancingEnabled  Desired Transmit Balancing setting.

   @retval   EFI_SUCCESS       Transmit Balancing setting successfully set.
   @retval   EFI_DEVICE_ERROR  Failed to set Transmit Balancing in NVM.
**/
EFI_STATUS
SetTransmitBalancing (
  IN  UNDI_PRIVATE_DATA  *UndiPrivateData,
  IN  BOOLEAN            *TransmitBalancingEnabled
  );

/** Gets Transmit Balancing setting.

   @param[in]   UndiPrivateData           Pointer to driver private data structure.
   @param[out]  TransmitBalancingEnabled  Transmit Balancing setting.

   @retval   EFI_SUCCESS       Transmit Balancing setting successfully gotten.
   @retval   EFI_DEVICE_ERROR  Failed to get Transmit Balancing from NVM.
**/
EFI_STATUS
GetTransmitBalancing (
  IN  UNDI_PRIVATE_DATA  *UndiPrivateData,
  OUT BOOLEAN            *TransmitBalancingEnabled
  );
#endif /* EEPROM_CONFIG_H_ */
