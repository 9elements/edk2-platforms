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
    { 0x159c873b, 0x3dba, 0x4469, { 0x94, 0x90, 0x01, 0x3c, 0x28, 0x6c, 0x84, 0x8f } }

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
#define  VIS_IDX_NUM       28    // *!! MUST BE !!* equal to last #define above + 1 == No. of Support indexes

#define  VIS_NO_EVAL       0xFFFFFFFF // indicates there's no Support Flag index associated with the field



#pragma pack(2)
typedef struct HII_STD_VARSTORE_S {
  // ---------------------------  <"NIC Configuration"> menu -------------------------------------
  UINT8   LinkSpeed;
  UINT8   WolStatus;
  UINT8   DefaultWolStatus;
  UINT8   LldpAgentStatus;
  UINT8   DefaultLldpAgentStatus;


  // ---------------------------  Main HII menu -----------------------------------------------
  UINT16  BlinkLed;
  UINT8   LinkStatus;
  UINT16  AltMacAddr[UNI_MAC_CHAR_COUNT];
  FIELD_SUPPORT  Support[VIS_IDX_NUM];
} HII_STD_VARSTORE;
#pragma pack()

#endif /* HII_CONFIG_DATA_H_ */
