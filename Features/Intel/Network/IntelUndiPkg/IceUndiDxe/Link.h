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

#ifndef LINK_H_
#define LINK_H_


#define PHY_TYPE_LINK_DISABLED BIT_ULL(63)
#define PHY_TYPE_AUTONEG BIT_ULL(62)

typedef enum {
  LINK_STATE_UP = 0,
  LINK_STATE_AN_IN_PROGRESS,
  LINK_STATE_DOWN,
  LINK_STATE_DOWN_NO_MODULE,
  LINK_STATE_DOWN_SW_FORCED,
} LINK_STATE;

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
  );

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
  );

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
  );
#endif /* LINK_H_ */
