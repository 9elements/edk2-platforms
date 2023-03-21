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
#include "CommonDriver.h"
#include "Link.h"
#include "ice_type.h"
#include "EepromConfig.h"


/** Gets link status (up/down).

   @param[in]   AdapterInfo  Pointer to the NIC data structure information which
                             the UNDI driver is layering on
   @param[out]  LinkUp       BOOLEAN value that tells whether link is up

   @retval   EFI_SUCCESS   Operation successful
   @retval   !EFI_SUCCESS  Failed to retrieve link status
**/
EFI_STATUS
IsLinkUp (
  IN   DRIVER_DATA  *AdapterInfo,
  OUT  BOOLEAN      *LinkUp
  )
{
  LINK_STATE      LinkState  = 0;
  EFI_STATUS      Status;

  Status = GetLinkState (AdapterInfo, &LinkState);
  IF_RETURN (EFI_ERROR (Status), Status);

  *LinkUp = (LinkState == LINK_STATE_UP);

  return EFI_SUCCESS;
}

/** Returns information about link status and reason for it (if it's down).

   @param[in]   AdapterInfo      Pointer to the NIC data structure information which
                                 the UNDI driver is layering on
   @param[out]  LinkState  Pointer variable wih link status info

   @retval   EFI_DEVICE_ERROR        AQ command failed
   @retval   EFI_DEVICE_ERROR        Link is down for an unpredicted reason
   @retval   EFI_INVALID_PARAMETER   AdapterInfo is NULL
   @retval   EFI_INVALID_PARAMETER   LinkState is NULL
   @retval   EFI_SUCCESS             LinkState has been filled correctly
**/
EFI_STATUS
GetLinkState (
  IN DRIVER_DATA *AdapterInfo,
  OUT LINK_STATE *LinkState
  )
{
  enum ice_status                   IceStatus   = ICE_SUCCESS;
  UINT64                            PhyTypeLow  = 0;
  UINT64                            PhyTypeHigh = 0;
  UINT8                             LinkInfo    = 0;
  struct ice_aqc_get_phy_caps_data  PhyCaps     = { 0 };
  BOOLEAN                           LinkEnable  = FALSE;
  struct ice_hw                     *Hw;
  struct ice_port_info              *PortInfo;

  if (LinkState == NULL || AdapterInfo == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  PortInfo = &AdapterInfo->Hw.port_info[0];
  Hw = &AdapterInfo->Hw;


  IceStatus = ice_aq_get_link_info (
                PortInfo,
                TRUE,
                NULL,
                NULL
                );
  if (IceStatus != ICE_SUCCESS) {
    DEBUGPRINT (CRITICAL, ("ice_aq_get_link_info returned: %d\n", IceStatus));
    return EFI_DEVICE_ERROR;
  }
  PhyTypeLow  = PortInfo->phy.link_info.phy_type_low;
  PhyTypeHigh = PortInfo->phy.link_info.phy_type_high;
  LinkInfo    = PortInfo->phy.link_info.link_info;

  IceStatus = ice_aq_get_phy_caps (
                PortInfo,
                FALSE,
                ICE_AQC_REPORT_ACTIVE_CFG,
                &PhyCaps,
                NULL
                );
  if (IceStatus != ICE_SUCCESS) {
    DEBUGPRINT (CRITICAL, ("ice_aq_get_phy_caps returned: %d\n", IceStatus));
    return EFI_DEVICE_ERROR;
  }

  if (PhyCaps.caps & ICE_AQC_PHY_EN_LINK) {
    LinkEnable = TRUE;
  }
  else {
    LinkEnable = FALSE;
  }

  if ((LinkInfo & ICE_AQ_LINK_UP) && (LinkEnable == TRUE)) {
    *LinkState = LINK_STATE_UP;
    return EFI_SUCCESS;
  }
  // there is no point in trying to link up without media
  // as set PHY config AQ command on port without media will fail.
  if ((LinkInfo & ICE_AQ_MEDIA_AVAILABLE) == 0) {
    *LinkState = LINK_STATE_DOWN_NO_MODULE;
    return EFI_SUCCESS;
  }

  if (LinkEnable == FALSE) {
    // link has been disabled
    *LinkState = LINK_STATE_DOWN_SW_FORCED;
    return EFI_SUCCESS;
  }
  else if ((PhyTypeLow == 0) && (PhyTypeHigh == PHY_TYPE_AUTONEG)) {
    // if link is down and PHY type is zeroed except for "Auto-Negotiation state" bit
    // it means that FW is currently trying to brig the link up
    *LinkState = LINK_STATE_AN_IN_PROGRESS;
    return EFI_SUCCESS;
  }
  // link is down because of other reasons
  *LinkState = LINK_STATE_DOWN;
  return EFI_SUCCESS;
}

/** Returns current Phy capabilities.

   @param[in]   AdapterInfo      Pointer to the NIC data structure information which
                                 the UNDI driver is layering on
   @param[out]  PhyCaps          Pointer ice_aqc_get_phy_caps_data structure to be filled

   @retval   EFI_INVALID_PARAMETER   AdapterInfo is NULL
   @retval   EFI_INVALID_PARAMETER   PhyCaps is NULL
   @retval   EFI_DEVICE_ERROR        AQ command failed
   @retval   EFI_DEVICE_ERROR        EepromReadDefaultOverrideMaskTlv failed
   @retval   EFI_DEVICE_ERROR        Link is down for an unpredicted reason
   @retval   EFI_SUCCESS             PhyCaps has been filled correctly
**/
EFI_STATUS
GetCurrentPhyCaps (
  IN  DRIVER_DATA                       *AdapterInfo,
  OUT struct ice_aqc_get_phy_caps_data  *PhyCaps
  )
{
  enum ice_status                  IceStatus                  = ICE_SUCCESS;
  struct ice_hw                    *Hw;

  if (PhyCaps == NULL || AdapterInfo == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  Hw = &AdapterInfo->Hw;


  IceStatus = ice_aq_get_phy_caps (
                Hw->port_info,
                FALSE,
                ICE_AQC_REPORT_TOPO_CAP_MEDIA,
                PhyCaps,
                NULL
                );
  if (IceStatus != ICE_SUCCESS) {
    DEBUGPRINT (CRITICAL, ("ice_aq_get_phy_caps returned: %d\n", IceStatus));
    return EFI_DEVICE_ERROR;
  }
  return EFI_SUCCESS;
}

/** Resets phy config.

   @param[in]   AdapterInfo      Pointer to the NIC data structure information which
                                 the UNDI driver is layering on

   @retval   EFI_INVALID_PARAMETER   AdapterInfo is NULL
   @retval   EFI_DEVICE_ERROR        AQ command failed
   @retval   EFI_DEVICE_ERROR        GetLinkState failed
   @retval   EFI_DEVICE_ERROR        Link is down for an unpredicted reason
   @retval   EFI_SUCCESS             Phy has been configured
   @retval   EFI_SUCCESS             link has not been brought down by SW
**/
EFI_STATUS
ResetLinkConfig (
  IN DRIVER_DATA *AdapterInfo
  )
{
  EFI_STATUS                       Status          = EFI_SUCCESS;
  enum   ice_status                IceStatus       = ICE_SUCCESS;
  struct ice_aqc_get_phy_caps_data PhyCaps         = { 0 };
  struct ice_aqc_set_phy_cfg_data  PhyConfig       = { 0 };
  struct ice_hw                    *Hw;

  if (AdapterInfo == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  Hw = &AdapterInfo->Hw;

  Status = GetCurrentPhyCaps (AdapterInfo, &PhyCaps);
  if (EFI_ERROR (Status)) {
    DEBUGPRINT (CRITICAL, ("GetCurrentPhyCaps returned: %d\n", Status));
    return Status;
  }
  PhyConfig.phy_type_low                  = PhyCaps.phy_type_low;
  PhyConfig.phy_type_high                 = PhyCaps.phy_type_high;
  PhyConfig.caps                          = PhyCaps.caps;
  PhyConfig.low_power_ctrl_an             = PhyCaps.low_power_ctrl_an;
  PhyConfig.eee_cap                       = PhyCaps.eee_cap;
  PhyConfig.eeer_value                    = PhyCaps.eeer_value;
  PhyConfig.link_fec_opt                  = PhyCaps.link_fec_options;
  PhyConfig.module_compliance_enforcement = PhyCaps.module_compliance_enforcement;

  /* Enable link */
  PhyConfig.caps |= ICE_AQ_PHY_ENA_LINK;
  /* Auto restart link so settings take effect */
  PhyConfig.caps |= ICE_AQ_PHY_ENA_AUTO_LINK_UPDT;

  IceStatus = ice_aq_set_phy_cfg (
                Hw,
                Hw->port_info,
                &PhyConfig,
                NULL
                );
  if (IceStatus != ICE_SUCCESS) {
    DEBUGPRINT (CRITICAL, ("ice_aq_set_phy_cfg returned: %d\n", IceStatus));
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}

