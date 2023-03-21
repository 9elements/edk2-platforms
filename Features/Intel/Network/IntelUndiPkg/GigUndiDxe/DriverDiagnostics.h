/**************************************************************************

Copyright (c) 2016 - 2021, Intel Corporation. All rights reserved.

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
#ifndef DRIVER_DIAGNOSTICS_H_
#define DRIVER_DIAGNOSTICS_H_

#define MAX_ETHERNET_SIZE       1518
#define TEST_PACKET_SIZE        1024

#if (DBG_LVL & DIAG)
#define PHY_LOOPBACK_ITERATIONS 10
#else /* NOT (DBG_LVL & DIAG) */
#define PHY_LOOPBACK_ITERATIONS 10000
#endif /* (DBG_LVL & DIAG) */

#define PHY_PHLBKC              19
#define PHY_PHCTRL1             23
#define PHY_PHSTAT              26

#define E1000_RCTL_LBM_MASK   (0x000000C0) /* bitmask to retrieve LBM bits */

#define E1000_EEER_EEE_FRC_AN   0x10000000 /* Force EEE Auto-negotiation */

/** Delays execution of further code for time given in microseconds

   @param[in]   X   Time in microseconds

   @return   Execution of code delayed
**/
#define USEC_DELAY(X)     USecDelay (Hw, X)

/** Delays execution of further code for time given in milliseconds

   @param[in]   X   Time in milliseconds

   @return   Execution of code delayed
**/
#define MSEC_DELAY(X)     USecDelay (Hw, X * 1000)

typedef enum {
  E1000_LBM_NONE        = 0,
  E1000_LBM_MAC,
  E1000_LBM_PHY_1000,
  E1000_LBM_PHY_100,
  E1000_LBM_PHY_10,
  E1000_LBM_TRANSCEIVER,
  E1000_LBM_COUNT,
  E1000_LBM_INVALID     = 0xFF
} E1000_LBM_TYPE;

#pragma pack(1)
typedef struct {
  UINT8 DestAddr[6];
  UINT8 SourceAddr[6];
  UINT8 Length[2];
} ETHERNET_HDR;
#pragma pack()

#endif /* DRIVER_DIAGNOSTICS_H_ */
