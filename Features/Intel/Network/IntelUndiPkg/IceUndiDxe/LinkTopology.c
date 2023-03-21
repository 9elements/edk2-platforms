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
#include "Ice.h"
#include "ice_type.h"
#include "LinkTopology.h"
#include "HiiCommonDep.h"
#include "Hii/Hii.h"

#include "Hii/FormsetStd/HiiConfigData.h"


const UINT8 mPmdToPfStandard2Port[]     = { 0, 1, 2, 3, 4, 5, 6, 7 };
const UINT8 mPmdToPfInverted2Port[]     = { 1, 0, 3, 2, 5, 4, 7, 6 };
const UINT8 mPmdToPfStandard4Port[]     = { 0, 2, 1, 3, 4, 6, 5, 7 };
const UINT8 mPmdToPfInverted4Port[]     = { 3, 1, 2, 0, 7, 5, 6, 4 };
const UINT8 mPmdToPfStandard8PortCpk[]  = { 0, 2, 4, 6, 7, 5, 3, 1 };
const UINT8 mPmdToPfInverted8PortCpk[]  = { 7, 5, 3, 1, 0, 2, 4, 6 };
const UINT8 mPmdToPfStandard8PortCvl[]  = { 0, 2, 4, 6, 1, 3, 5, 7 };
const UINT8 mPmdToPfInverted8PortCvl[]  = { 7, 5, 3, 1, 6, 4, 2, 0 };

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
  IN     UNDI_PRIVATE_DATA       *UndiPrivateData,
  IN OUT PORT_OPTIONS_DATA       *PortOptionsData
  )
{
  UINT8         PmdCount        = 0;
  UINT16        PfToPortMapping = 0;
  UINT8         PmdNum;
  UINT8         PfNum;
  UINT8         PortOptionNum;
  UINT8         ActivePmdNum;
  PORT_OPTION   *CurrentPortOption;

  IF_NULL2_RETURN (UndiPrivateData, PortOptionsData, EFI_INVALID_PARAMETER);

  for (PortOptionNum = 0; PortOptionNum < PortOptionsData->PortOptionsCount; PortOptionNum++) {
    CurrentPortOption = &PortOptionsData->PortOptions[PortOptionNum];
    PfToPortMapping   = CurrentPortOption->PfToPortConfigId;
    PmdCount          = CurrentPortOption->PmdCount;
    ActivePmdNum      = 0;

    IF_RETURN (PmdCount > ICE_MAX_PMD, EFI_DEVICE_ERROR);

    // invalidate all the PF entries first
    for (PmdNum = 0; PmdNum < ICE_MAX_PMD; PmdNum++) {
      CurrentPortOption->PfNum[PmdNum] = PMD_PF_INACTIVE;
    }

    if (PmdCount == 1) {
      PfNum = 0; // no mapping for a single port
      while (CurrentPortOption->PortSpeed[ActivePmdNum++] == PortOptionSpeedNA) {
        IF_RETURN (ActivePmdNum == ICE_MAX_PMD, EFI_DEVICE_ERROR);
      }
      CurrentPortOption->PfNum[ActivePmdNum - 1] = PfNum;
    } else {
      for (PmdNum = 0; PmdNum < PmdCount; PmdNum++) {
        switch (PfToPortMapping) {
        case PfToPortStandard8PortCvl:
          PfNum = mPmdToPfStandard8PortCvl[PmdNum];
          break;
        case PfToPortInverted8PortCvl:
          PfNum = mPmdToPfInverted8PortCvl[PmdNum];
          break;
        case PfToPortStandard8PortCpk:
          PfNum = mPmdToPfStandard8PortCpk[PmdNum];
          break;
        case PfToPortInverted8PortCpk:
          PfNum = mPmdToPfInverted8PortCpk[PmdNum];
          break;
        case PfToPortStandard4PortCvl:
        case PfToPortStandard4PortSplitCpk:
        case PfToPortStandard4PortBreakoutCpk:
          PfNum = mPmdToPfStandard4Port[PmdNum];
          break;
        case PfToPortInverted4PortCvl:
        case PfToPortInverted4PortSplitCpk:
        case PfToPortInverted4PortBreakoutCpk:
          PfNum = mPmdToPfInverted4Port[PmdNum];
          break;
        case PfToPortStandard2Port:
          PfNum = mPmdToPfStandard2Port[PmdNum];
          break;
        case PfToPortInverted2Port:
          PfNum = mPmdToPfInverted2Port[PmdNum];
          break;
        default:
          return EFI_UNSUPPORTED;
        }

        // PmdNum contains contiguous PMD indexes, but some of the actual 
        // port options have non-contiguous set of *active* PMD indexes.
        // Distribute ordered set of PFs across *active* PMD indexes
        while (CurrentPortOption->PortSpeed[ActivePmdNum++] == PortOptionSpeedNA) {
          IF_RETURN (ActivePmdNum == ICE_MAX_PMD, EFI_DEVICE_ERROR);
        }
        CurrentPortOption->PfNum[ActivePmdNum - 1] = PfNum;
      }
    }
  }
  return EFI_SUCCESS;
}


/** Provides data for each aviable port option.

   @param[in]       AdapterInfo                 Pointer to driver data structure
   @param[out]      ResultOptionsData           Pointer for the Port Options Data struct

   @retval          EFI_SUCCESS                 Successfully returned port options data
   @retval          EFI_OUT_OF_RESOURCES        Memory allocation failed
   @retval          EFI_INVALID_PARAMETER       AdapterInfo or ResultOptionsData is NULL
   @retval          !EFI_SUCCESS                One of the functions called returned an error
**/
EFI_STATUS
GetPortOptionsForAllPorts (
  IN  DRIVER_DATA            *AdapterInfo,
  OUT PORT_OPTIONS_DATA      **ResultOptionsData
  )
{
  PORT_OPTIONS_AQ_RSP                   *PortOptionsDataForEachPort;
  UINT8                                 LogicalPortNum;
  UINT8                                 PortOptionsCount               = 0;
  UINT8                                 PortOptionNum                  = 0;
  EFI_STATUS                            Status                         = EFI_SUCCESS;
  struct ice_aqc_get_port_options_elem  CurrentPortOptElem;
  PORT_OPTION                           *CurrentResultPortOpt;
  PORT_OPTIONS_DATA                     *PortOptionsDataBuffer;
  PORT_OPTIONS_DATA                     CurrentPortOpt;
  UINT8                                 CurrentPmdCount                = 0;
  UINT16                                CurrentPfToPortConfigId        = 0;
  UINT8                                 CurrentMaxPmdSpeed             = 0;
  UINTN                                 ResultOptionsDataLength;
  UNDI_PRIVATE_DATA                     *UndiPrivateData;

  IF_NULL2_RETURN (AdapterInfo, ResultOptionsData, EFI_INVALID_PARAMETER);

  PortOptionsDataForEachPort = AllocateZeroPool (ICE_MAX_PMD * sizeof (PORT_OPTIONS_AQ_RSP));
  if (PortOptionsDataForEachPort == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  UndiPrivateData = UNDI_PRIVATE_DATA_FROM_DRIVER_DATA (AdapterInfo);

  // Get AQ get_port_options command response for each logical port
  for (LogicalPortNum = 0; LogicalPortNum < ICE_MAX_PMD; LogicalPortNum++) {
    Status = GetPortOptions (
               AdapterInfo,
               LogicalPortNum,
               &(PortOptionsDataForEachPort[LogicalPortNum]),
               &CurrentPortOpt
             );
    IF_GOTO (EFI_ERROR (Status), ExitCleanMem);
  }

  // PortOptionsCount, Active, Valid and Forced fields are the same for all of the logical ports
  PortOptionsCount = CurrentPortOpt.PortOptionsCount;

  ResultOptionsDataLength = (sizeof (PORT_OPTIONS_DATA) + ((PortOptionsCount - 1) * sizeof (PORT_OPTION)));
  PortOptionsDataBuffer = AllocateZeroPool (ResultOptionsDataLength);
  if (PortOptionsDataBuffer == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ExitCleanMem;
  }

  PortOptionsDataBuffer->Active           = CurrentPortOpt.Active;
  PortOptionsDataBuffer->Forced           = CurrentPortOpt.Forced;
  PortOptionsDataBuffer->Valid            = CurrentPortOpt.Valid;
  PortOptionsDataBuffer->PortOptionsCount = CurrentPortOpt.PortOptionsCount;
  PortOptionsDataBuffer->PendingValid     = CurrentPortOpt.PendingValid;
  if (PortOptionsDataBuffer->PendingValid) {
    PortOptionsDataBuffer->Pending        = CurrentPortOpt.Pending;
  }
  // Fill PORT_OPTIONS_DATA structures
  for (PortOptionNum = 0; PortOptionNum < PortOptionsCount; PortOptionNum++) {
    for (LogicalPortNum = 0; LogicalPortNum < ICE_MAX_PMD; LogicalPortNum++) {
      CurrentPortOptElem = PortOptionsDataForEachPort[LogicalPortNum].Options[PortOptionNum];
      CurrentPmdCount = (CurrentPortOptElem.pmd & ICE_AQC_PORT_OPT_PMD_COUNT_M);
      CurrentMaxPmdSpeed = (CurrentPortOptElem.max_lane_speed & ICE_AQC_PORT_OPT_MAX_LANE_M);
      CurrentResultPortOpt = &(PortOptionsDataBuffer->PortOptions[PortOptionNum]);
      CurrentPfToPortConfigId = 0;
      CurrentPfToPortConfigId |= (UINT16)CurrentPortOptElem.pf2port_cid[1] << 8;
      CurrentPfToPortConfigId |= (UINT16)CurrentPortOptElem.pf2port_cid[0] << 0;
      if (CurrentResultPortOpt->PmdCount == 0) {
        if (CurrentMaxPmdSpeed != PortOptionSpeedNA) {
          CurrentResultPortOpt->PmdCount = CurrentPmdCount;
        }
      }

      if (CurrentResultPortOpt->PfToPortConfigId == 0) {
        if (CurrentMaxPmdSpeed != PortOptionSpeedNA) {
          CurrentResultPortOpt->PfToPortConfigId = CurrentPfToPortConfigId;
        }
      }
      CurrentResultPortOpt->PortSpeed[LogicalPortNum] = CurrentMaxPmdSpeed;
      CurrentResultPortOpt->PortOptionInvalid = (CurrentPortOptElem.pmd & ICE_AQC_PORT_OPT_PMD_INVALID_M) != 0;
    }
  }

  Status = GetPfNumsFromPmds (UndiPrivateData, PortOptionsDataBuffer);
  IF_GOTO (EFI_ERROR (Status), ExitCleanMem);


ExitCleanMem:
  if (Status == EFI_SUCCESS) {
    *ResultOptionsData = PortOptionsDataBuffer;
  } else {
    FreePool (PortOptionsDataBuffer);
  }
  FreePool (PortOptionsDataForEachPort);
  return Status;
}

/** Provides data from AQ response to PORT_OPTIONS_DATA struct.

   @param[in]       PortOptionsCmd              Pointer to AQ response struct
   @param[out]      ParsedOption                Pointer for the Port Options Data struct

   @retval          EFI_SUCCESS                 Successfully returned port options data
   @retval          EFI_INVALID_PARAMETER       ParsedOption or PortOptionsCmd is NULL
**/
EFI_STATUS
ParsePortOptionsCmd (
  IN struct ice_aqc_get_port_options       *PortOptionsCmd,
  OUT PORT_OPTIONS_DATA                    *ParsedOption
  )
{

  IF_NULL2_RETURN (ParsedOption, PortOptionsCmd, EFI_INVALID_PARAMETER);

  ParsedOption->Active = (PortOptionsCmd->port_options & ICE_AQC_PORT_OPT_ACTIVE_M);
  ParsedOption->Forced = (PortOptionsCmd->port_options & ICE_AQC_PORT_OPT_FORCED) != 0;
  ParsedOption->Valid  = (PortOptionsCmd->port_options & ICE_AQC_PORT_OPT_VALID) != 0;

  ParsedOption->PortOptionsCount      = PortOptionsCmd->port_options_count;
  ParsedOption->PendingValid = (PortOptionsCmd->pending_port_option_status & ICE_AQC_PENDING_PORT_OPT_VALID) != 0;
  if (ParsedOption->PendingValid) {
    ParsedOption->Pending = (PortOptionsCmd->pending_port_option_status & ICE_AQC_PENDING_PORT_OPT_IDX_M);
  }
  return EFI_SUCCESS;
}

/** Brings Port Configuration to default.

   @param[in]       UndiPrivateData             Pointer to driver private data structure

   @retval          EFI_SUCCESS                 Successfully changed port configuration to default
   @retval          EFI_INVALID_PARAMETER       AdapterInfo or ResultOptionsData is NULL
   @retval          !EFI_SUCCESS                One of the functions called returned an error
**/
EFI_STATUS
PortConfigDefault (
  IN  UNDI_PRIVATE_DATA      *UndiPrivateData
  )
{
  WOL_STATUS          WolStatus = WOL_SUCCESS;
  UINT8               AdapterWolStatus;


  IF_NULL_RETURN (UndiPrivateData, EFI_INVALID_PARAMETER);



  if (WolIsWakeOnLanSupported (UndiPrivateData)) {
    AdapterWolStatus = WOL_DISABLE;
    WolStatus = WolSetWakeOnLanStatus (UndiPrivateData, &AdapterWolStatus);
    IF_RETURN (WolStatus != WOL_SUCCESS, EFI_DEVICE_ERROR);
  }

  return EFI_SUCCESS;
}

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
  )
{
  struct ice_aq_desc                    AqDesc;
  struct ice_aqc_get_port_options       *PortOptionsCmd;
  UINTN                                 PortOptionsDataLength;
  enum ice_status                       IceStatus;
  EFI_STATUS                            Status;

  IF_NULL_RETURN (AdapterInfo, EFI_INVALID_PARAMETER);
  IF_RETURN ((LogicalPortNumber >= ICE_MAX_PMD), EFI_INVALID_PARAMETER);

  // Obtain available port options (Get Port Options AQ)
  PortOptionsCmd                    = &AqDesc.params.get_port_options;
  ice_fill_dflt_direct_cmd_desc (&AqDesc, ice_aqc_opc_get_port_options);
  PortOptionsCmd->lport_num         = LogicalPortNumber;
  PortOptionsCmd->lport_num_valid   = ICE_AQC_PORT_OPT_PORT_NUM_VALID;

  PortOptionsDataLength = (sizeof (PORT_OPTIONS_AQ_RSP));
  IceStatus = ice_aq_send_cmd (
                &AdapterInfo->Hw,
                &AqDesc,
                PortOptionsAqResp,
                PortOptionsDataLength,
                NULL
                );

  if (IceStatus != ICE_SUCCESS) {
    DEBUGPRINT (CRITICAL, ("ice_aq_send_cmd returned: %d\n", IceStatus));
    return EFI_DEVICE_ERROR;
  }

  // Fill Active, Forced, Valid and PortOptionsCount fields
  Status = ParsePortOptionsCmd (PortOptionsCmd, ParsedOption);
  IF_RETURN (EFI_ERROR (Status), Status);


  return EFI_SUCCESS;
}

/** Returns index of current port option.
   If no option is pending returns number of an active one.

   @param[in]       UndiPrivateData            Pointer to driver data structure
   @param[out]      CurrentPortOption          buffer for active port option number

   @retval          EFI_SUCCESS                 Successfully returned port options data
   @retval          EFI_DEVICE_ERROR            AQ command failed
   @retval          EFI_INVALID_PARAMETER       AdapterInfo or ActivePortOption is NULL
   @retval          !EFI_SUCCESS                One of the functions called returned an error
**/
EFI_STATUS
GetCurrentPortOptionNum (
  IN  UNDI_PRIVATE_DATA      *UndiPrivateData,
  OUT UINT8                  *CurrentPortOption
  )
{
  EFI_STATUS         Status = EFI_SUCCESS;
  PORT_OPTIONS_DATA  *PortOptionsData;

  IF_NULL2_RETURN (UndiPrivateData, CurrentPortOption, EFI_INVALID_PARAMETER);

  Status = GetPortOptionsForAllPorts (&UndiPrivateData->NicInfo, &PortOptionsData);
  IF_RETURN (EFI_ERROR (Status), Status);

  UndiPrivateData->HiiInfo.PendingPortOptionValid = PortOptionsData->PendingValid;

  if (PortOptionsData->PendingValid) {
    *CurrentPortOption = PortOptionsData->Pending;
  }
  else {
    *CurrentPortOption = PortOptionsData->Active;
  }
  if (PortOptionsData != NULL) {
    FreePool (PortOptionsData);
  }
  return EFI_SUCCESS;
}

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
  IN  UNDI_PRIVATE_DATA     *UndiPrivateData,
  IN  UINT8                 *Option
  )
{
  enum ice_status                 IceStatus;
  struct ice_aq_desc              AqDesc;
  struct ice_aqc_set_port_option  *PortOptionCmd;
  struct ice_hw                   *Hw;
  EFI_STATUS                      Status;
  UINT8                           CurrentOption;
  PORT_OPTION                     *PortOption;
  PORT_OPTIONS_DATA               *PortOptionsData;
  BOOLEAN                         PortOptionInvalid;

  IF_NULL_RETURN (UndiPrivateData, EFI_INVALID_PARAMETER);
  IF_RETURN ((*Option >= ICE_MAX_PORT_OPT), EFI_INVALID_PARAMETER);

  // Check if requested port configuration is supported by this adapter
  Status = GetPortOptionsForAllPorts (&UndiPrivateData->NicInfo, &PortOptionsData);
  IF_RETURN (EFI_ERROR (Status), Status);
  PortOption = &PortOptionsData->PortOptions[*Option];
  PortOptionInvalid = PortOption->PortOptionInvalid;
  FreePool (PortOptionsData);
  IF_RETURN (PortOptionInvalid, EFI_INVALID_PARAMETER);

  // No point in overwriting current pending/active port option with the same one
  Status = GetCurrentPortOptionNum (UndiPrivateData, &CurrentOption);
  IF_RETURN (EFI_ERROR (Status), Status);

  if (CurrentOption != *Option) {
    Hw = &UndiPrivateData->NicInfo.Hw;

    PortOptionCmd = &AqDesc.params.set_port_option;
    ice_fill_dflt_direct_cmd_desc (&AqDesc, ice_aqc_opc_set_port_option);

    // Only use port 0 to set Port option for all ports
    PortOptionCmd->lport_num            = 0;
    PortOptionCmd->lport_num_valid      = ICE_AQC_PORT_OPT_PORT_NUM_VALID;
    PortOptionCmd->selected_port_option = *Option;
    IceStatus = ice_aq_send_cmd (
                  Hw,
                  &AqDesc,
                  NULL,
                  0,
                  NULL
                  );
    IF_SCERR_RETURN (IceStatus, EFI_DEVICE_ERROR);
  }

  return EFI_SUCCESS;
}

