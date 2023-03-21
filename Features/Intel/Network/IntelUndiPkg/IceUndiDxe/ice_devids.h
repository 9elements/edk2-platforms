/**************************************************************************

Copyright (c) 2016 - 2021, Intel Corporation

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

#ifndef _ICE_DEVIDS_H_
#define _ICE_DEVIDS_H_


/* Device IDs */
#define ICE_DEV_ID_E822_SI_DFLT		0x1888
#ifdef FPGA_SUPPORT
#define ICE_DEV_ID_I200_FPGA		0xF0B5
#define ICE_DEV_ID_I200_FPGA_VF		0xF0A6
#elif defined(SIMICS_SUPPORT) 
#define ICE_DEV_ID_SIMICS_NIC_MODE	0x18E4
#endif
#ifdef E823_SUPPORT
/* Intel(R) Ethernet Connection E823-L for backplane */
#define ICE_DEV_ID_E823L_BACKPLANE	0x124C
/* Intel(R) Ethernet Connection E823-L for SFP */
#define ICE_DEV_ID_E823L_SFP		0x124D
/* Intel(R) Ethernet Connection E823-L/X557-AT 10GBASE-T */
#define ICE_DEV_ID_E823L_10G_BASE_T	0x124E
/* Intel(R) Ethernet Connection E823-L 1GbE */
#define ICE_DEV_ID_E823L_1GBE		0x124F
/* Intel(R) Ethernet Connection E823-L for QSFP */
#define ICE_DEV_ID_E823L_QSFP		0x151D
#endif /* E823_SUPPORT */
#ifdef E822S_SUPPORT
/* Intel(R) Ethernet Connection E822-S */
#define ICE_DEV_ID_E822S		0x1896
#endif /* E822S_SUPPORT */
#ifdef E810C_SUPPORT
#ifdef FPGA_SUPPORT
/* Intel(R) Ethernet Controller E810-C */
#define ICE_DEV_ID_E810C_FPGA_SI_DFLT	0x1590
#endif /* FPGA_SUPPORT */
/* Intel(R) Ethernet Controller E810-C for backplane */
#define ICE_DEV_ID_E810C_BACKPLANE	0x1591
/* Intel(R) Ethernet Controller E810-C for QSFP */
#define ICE_DEV_ID_E810C_QSFP		0x1592
/* Intel(R) Ethernet Controller E810-C for SFP */
#define ICE_DEV_ID_E810C_SFP		0x1593
#endif /* E810C_SUPPORT */
#define ICE_SUBDEV_ID_E810T		0x000E
#define ICE_SUBDEV_ID_E810T2		0x000F
#define ICE_SUBDEV_ID_E810T3		0x02E9
#define ICE_SUBDEV_ID_E810T4		0x02EA
#define ICE_SUBDEV_ID_E810T5		0x0010
#define ICE_SUBDEV_ID_E810T6		0x0012
#define ICE_SUBDEV_ID_E810T7		0x0011
#ifdef E810_XXV_SUPPORT
#ifdef QV_SUPPORT
/* Intel(R) Ethernet Controller E810-XXV default */
#define ICE_DEV_ID_E810_XXV_DEFAULT	0x1598
#endif /* QV_SUPPORT */
/* Intel(R) Ethernet Controller E810-XXV for backplane */
#define ICE_DEV_ID_E810_XXV_BACKPLANE	0x1599
/* Intel(R) Ethernet Controller E810-XXV for QSFP */
#define ICE_DEV_ID_E810_XXV_QSFP	0x159A
/* Intel(R) Ethernet Controller E810-XXV for SFP */
#define ICE_DEV_ID_E810_XXV_SFP		0x159B
#endif /* E810_XXV_SUPPORT */
#ifdef E823C_SUPPORT
/* Intel(R) Ethernet Connection E823-C for backplane */
#define ICE_DEV_ID_E823C_BACKPLANE	0x188A
/* Intel(R) Ethernet Connection E823-C for QSFP */
#define ICE_DEV_ID_E823C_QSFP		0x188B
/* Intel(R) Ethernet Connection E823-C for SFP */
#define ICE_DEV_ID_E823C_SFP		0x188C
/* Intel(R) Ethernet Connection E823-C/X557-AT 10GBASE-T */
#define ICE_DEV_ID_E823C_10G_BASE_T	0x188D
/* Intel(R) Ethernet Connection E823-C 1GbE */
#define ICE_DEV_ID_E823C_SGMII		0x188E
#endif /* E823C_SUPPORT */
#ifdef E822_SUPPORT
/* Intel(R) Ethernet Connection E822-C for backplane */
#define ICE_DEV_ID_E822C_BACKPLANE	0x1890
/* Intel(R) Ethernet Connection E822-C for QSFP */
#define ICE_DEV_ID_E822C_QSFP		0x1891
/* Intel(R) Ethernet Connection E822-C for SFP */
#define ICE_DEV_ID_E822C_SFP		0x1892
/* Intel(R) Ethernet Connection E822-C/X557-AT 10GBASE-T */
#define ICE_DEV_ID_E822C_10G_BASE_T	0x1893
/* Intel(R) Ethernet Connection E822-C 1GbE */
#define ICE_DEV_ID_E822C_SGMII		0x1894
/* Intel(R) Ethernet Connection E822-L for backplane */
#define ICE_DEV_ID_E822L_BACKPLANE	0x1897
/* Intel(R) Ethernet Connection E822-L for SFP */
#define ICE_DEV_ID_E822L_SFP		0x1898
/* Intel(R) Ethernet Connection E822-L/X557-AT 10GBASE-T */
#define ICE_DEV_ID_E822L_10G_BASE_T	0x1899
/* Intel(R) Ethernet Connection E822-L 1GbE */
#define ICE_DEV_ID_E822L_SGMII		0x189A
#endif /* E822_SUPPORT */
#ifdef E825C_SUPPORT
/* Intel(R) Ethernet Connection E825-C for backplane */
#define ICE_DEV_ID_E825C_BACKPLANE	0x579C
/* Intel(R) Ethernet Connection E825-C for QSFP */
#define ICE_DEV_ID_E825C_QSFP		0x579D
/* Intel(R) Ethernet Connection E825-C for SFP */
#define ICE_DEV_ID_E825C_SFP		0x579E
/* Intel(R) Ethernet Connection E825-C 1GbE */
#define ICE_DEV_ID_E825C_SGMII		0x579F
#endif /* E825C_SUPPORT */
#if defined(BMSM_MODE) || defined(QV_SUPPORT)
#ifdef C823X_SUPPORT
#define ICE_DEV_ID_C823X		0x188F
#endif /* C823X_SUPPORT */
#ifdef C825X_SUPPORT
#define ICE_DEV_ID_C825X		0x0DCD
#endif /* C825X_SUPPORT */
#ifdef E822X_SUPPORT
/* Intel(R) Ethernet Connection E822-X */
#define ICE_DEV_ID_E822X		0x1895
#endif /* E822X_SUPPORT */
#endif /* BMSM_MODE || QV_SUPPORT */
#if defined(QV_SUPPORT)
#ifdef E824S_SUPPORT
#define ICE_DEV_ID_E824S		0x0DBD
#endif /* E824S_SUPPORT */
#endif /* (SWITCH_MODE && !BMSM_MODE) || QV_SUPPORT */
#ifdef INTEGRATED_VF
#define ICE_DEV_ID_ADAPTIVE_VF		0x1889

#endif /* INTEGRATED_VF */
#endif /* _ICE_DEVIDS_H_ */
