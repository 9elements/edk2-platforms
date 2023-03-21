/**************************************************************************

Copyright (c) 2021 - 2022, Intel Corporation. All Rights Reserved.

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

#ifndef LINK_TOPOLOGY_H_
#define LINK_TOPOLOGY_H_

#define ICE_MAX_PMD           8
#define ICE_MAX_PORT_OPT      16
#define ICE_MAX_VPD_PORT_NUM  16

typedef enum {
  PortOptionSpeed100M   = 0,
  PortOptionSpeed1G     = 1,
  PortOptionSpeed2500M  = 2,
  PortOptionSpeed5G     = 3,
  PortOptionSpeed10G    = 4,
  PortOptionSpeed25G    = 5,
  PortOptionSpeed50G    = 6,
  PortOptionSpeed100G   = 7,
  PortOptionSpeedNA     = 15
} PORT_OPTION_SPEED;

/* used to mark PORT_OPTION.PfNum entries as inactive */
#define PMD_PF_INACTIVE 0xFF

/* used to mark Bit 4 from PMD (Invalid field). This bit indicates that the port option is indicated as not valid */
#define ICE_AQC_PORT_OPT_PMD_INVALID_S	4
#define ICE_AQC_PORT_OPT_PMD_INVALID_M	(0x1 << ICE_AQC_PORT_OPT_PMD_INVALID_S)

typedef struct {
  UINT8             PmdCount;
  UINT16            PfToPortConfigId;
  PORT_OPTION_SPEED PortSpeed[ICE_MAX_PMD];
  UINT8             PfNum[ICE_MAX_PMD];
  BOOLEAN           PortOptionInvalid;
} PORT_OPTION;

typedef struct {
  UINT8       Active;
  BOOLEAN     Forced;
  BOOLEAN     Valid;
  UINT8       PortOptionsCount;
  BOOLEAN     PendingValid;
  UINT8       Pending;
  PORT_OPTION PortOptions[1];
} PORT_OPTIONS_DATA;

typedef struct {
  struct ice_aqc_get_port_options_elem Options[ICE_MAX_PORT_OPT];
} PORT_OPTIONS_AQ_RSP;

typedef enum {
  PfToPortSwitchMode               = 0x0,
  PfToPortStandard2Port            = 0x1,
  PfToPortInverted2Port            = 0x101,
  PfToPortStandard4PortSplitCpk    = 0x2,
  PfToPortInverted4PortSplitCpk    = 0x102,
  PfToPortStandard4PortBreakoutCpk = 0x202,
  PfToPortInverted4PortBreakoutCpk = 0x302,
  PfToPortStandard4PortCvl         = 0x402,
  PfToPortInverted4PortCvl         = 0x502,
  PfToPortStandard8PortCpk         = 0x3,
  PfToPortInverted8PortCpk         = 0x103,
  PfToPortStandard8PortCvl         = 0x4,
  PfToPortInverted8PortCvl         = 0x104
} PF_TO_PORT_MAPPING;

/** AQ command wrapper for Get Port Options.

   @param[in]       AdapterInfo                 Pointer to driver data structure
   @param[in]       LogicalPortNumber           Port number for which aviable port options are returned
   @param[out]      PortOptionsAqResp           pointer to AQ response struct
   @param[out]      ParsedOption                pointer to parsed port option

   @retval          EFI_SUCCESS                 Successfully returned port options data
   @retval          EFI_DEVICE_ERROR            AQ command failed
   @retval          EFI_INVALID_PARAMETER       AdapterInfo is NULL or LogicalPortNumber > 7
   @retval          !EFI_SUCCESS                One of the functions called returned an error
**/
EFI_STATUS
GetPortOptions (
  IN  DRIVER_DATA           *AdapterInfo,
  IN  UINT8                 LogicalPortNumber,
  OUT PORT_OPTIONS_AQ_RSP   *PortOptionsAqResp,
  OUT PORT_OPTIONS_DATA     *ParsedOption
  );

/** AQ command wrapper for Set Port Options.

   @param[in]       UndiPrivateData             Pointer to driver data structure
   @param[in]       Option                      Port option index to be set as active

   @retval          EFI_SUCCESS                 Successfully set port option
   @retval          EFI_DEVICE_ERROR            AQ command failed
   @retval          EFI_INVALID_PARAMETER       AdapterInfo is NULL or Option is invalid
   @retval          !EFI_SUCCESS                One of the functions called returned an error
**/
EFI_STATUS
SetPortOption (
  IN  UNDI_PRIVATE_DATA      *UndiPrivateData,
  IN  UINT8                  *Option
  );

/** Provides data for each aviable port option.

   @param[in]       AdapterInfo                 Pointer to driver data structure
   @param[out]      ResultPortOptionsData       Pointer for the Port Options Data struct

   @retval          EFI_SUCCESS                 Successfully returned port options data
   @retval          EFI_OUT_OF_RESOURCES        Memory allocation failed
   @retval          EFI_INVALID_PARAMETER       AdapterInfo or ResultOptionsData is NULL
   @retval          !EFI_SUCCESS                One of the functions called returned an error
**/
EFI_STATUS
GetPortOptionsForAllPorts (
  IN  DRIVER_DATA            *AdapterInfo,
  OUT PORT_OPTIONS_DATA      **ResultPortOptionsData
  );

/** Brings Port Configuration to default.

   @param[in]       UndiPrivateData             Pointer to driver private data structure

   @retval          EFI_SUCCESS                 Successfully changed port configuration to default
   @retval          EFI_INVALID_PARAMETER       AdapterInfo or ResultOptionsData is NULL
   @retval          !EFI_SUCCESS                One of the functions called returned an error
**/
EFI_STATUS
PortConfigDefault (
  IN  UNDI_PRIVATE_DATA      *UndiPrivateData
  );

/** Returns index of current port option. If no option is pending returns number of an active one.

   @param[in]       UndiPrivateData             Pointer to driver data structure
   @param[out]      CurrentPortOption           buffer for active port option number

   @retval          EFI_SUCCESS                 Successfully returned port options data
   @retval          EFI_DEVICE_ERROR            AQ command failed
   @retval          EFI_INVALID_PARAMETER       AdaperInfo or ActivePortOption is NULL
   @retval          !EFI_SUCCESS                One of the functions called returned an error
**/
EFI_STATUS
GetCurrentPortOptionNum (
  IN  UNDI_PRIVATE_DATA      *UndiPrivateData,
  OUT UINT8                  *CurrentPortOption
  );

/** Provides PMD to PF mapping to the PORT_OPTIONS_DATA.

   @param[in]       UndiPrivateData             Pointer to driver private data structure
   @param[in, out]  PortOptionsData             Pointer to Port Options struct

   @retval          EFI_SUCCESS                 Successfully returned PMD to PF mapping
   @retval          EFI_INVALID_PARAMETER       UndiPrivateData or PortOptionsData is NULL
   @retval          EFI_UNSUPPORTED             Invalid PmdCount field in PORT_OPTIONS_DATA struct
   @retval          EFI_UNSUPPORTED             Invalid PfToPortMapping ID
   @retval          EFI_DEVICE_ERROR            Number of active PMDs is less than PMD count reported by AQ

**/
EFI_STATUS
GetPfNumsFromPmds (
  IN UNDI_PRIVATE_DATA       *UndiPrivateData,
  IN OUT PORT_OPTIONS_DATA   *PortOptionsData
  );


#endif /* LINK_TOPOLOGY_H_ */


