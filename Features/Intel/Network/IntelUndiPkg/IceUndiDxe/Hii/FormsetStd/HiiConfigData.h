/**************************************************************************

Copyright (c) 2020 - 2022, Intel Corporation. All rights reserved.

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

#ifndef HII_CONFIG_DATA_H_
#define HII_CONFIG_DATA_H_

  #define HII_DATA_GUID \
    { 0xb48f2760, 0xd5bb, 0x48ba, { 0xa9, 0xb4, 0x69, 0x15, 0x99, 0xf8, 0x42, 0x6b } }

#define  UNI_MAC_CHAR_COUNT  18 /* (12 * hex) + (5 * ":") + NULL terminator */

/* FIELD_SUPPORT Support[] array indexes  - *MUST BE* consecutive numbers, cannot be declared
   as enum because it is unsupported by VFR compiler.
*/
#define  LEGACY_BOOT_PROT  0
#define  LINK_SPEED        1
#define  PXE_VLAN          2
#define  LLDP_AGENT        3
#define  LINK_SPEED_STATUS 4
#define  ALT_MAC           6
#define  FEC_MODE          7
#define  DEV_LVL_CFG       17
#define  LINK_SPEED_METHOD 23
#define  PORT_OPTIONS      24
#define  CVL_DEV           25
#define  TX_BALANCING      26
#define  NO_FEC_IN_AUTO    27
#define  VIS_IDX_NUM       28    // *!! MUST BE !!* equal to last #define above + 1 == No. of Support indexes

#define  VIS_NO_EVAL       0xFFFFFFFF // indicates there's no Support Flag index associated with the field


#define MAX_SINGLE_SPEED_LEN  20
#define MAX_OPT_NAME_LEN      120
#define MAX_PORT_OPTIONS      16

#define ICE_QUAD_COUNT        2
#define ICE_LANES_PER_QUAD    4

typedef struct {
  CHAR16      Desc[MAX_SINGLE_SPEED_LEN];
} LANE_DESC;

typedef struct {
  CHAR16      Desc[MAX_SINGLE_SPEED_LEN];
  LANE_DESC   Lane[ICE_LANES_PER_QUAD];
} QUAD_DESC;

typedef struct {
  CHAR16      OptionName[MAX_OPT_NAME_LEN];
  QUAD_DESC   Quad[ICE_QUAD_COUNT];
} PORT_OPTION_DESC;


#pragma pack(2)
typedef struct HII_STD_VARSTORE_S {
  // ---------------------------  <"NIC Configuration"> menu -------------------------------------
  UINT8   LinkSpeed;
  UINT8   WolStatus;
  UINT8   DefaultWolStatus;
  UINT8   LldpAgentStatus;
  UINT8   DefaultLldpAgentStatus;


  UINT8   TxBalancing;
  UINT8   DefaultTxBalancing;
  UINT8   ScalableIov;
  UINT8   DefaultScalableIov;
  UINT8   ActivePortOption;
  // ---------------------------  Main HII menu -----------------------------------------------
  UINT16  BlinkLed;
  UINT8   LinkStatus;
  UINT16  AltMacAddr[UNI_MAC_CHAR_COUNT];
  FIELD_SUPPORT  Support[VIS_IDX_NUM];
  PORT_OPTION_DESC PortOptStrings[MAX_PORT_OPTIONS];
} HII_STD_VARSTORE;
#pragma pack()

#endif /* HII_CONFIG_DATA_H_ */
