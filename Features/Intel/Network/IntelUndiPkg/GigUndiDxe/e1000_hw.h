/**************************************************************************

Copyright (c) 2001 - 2021, Intel Corporation

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
#ifndef _E1000_HW_H_
#define _E1000_HW_H_

#include "DebugTools.h"
#include "e1000_osdep.h"
#include "e1000_regs.h"
#include "e1000_defines.h"

struct e1000_hw;

#ifndef NO_82571_SUPPORT
#define E1000_DEV_ID_82571EB_COPPER		0x105E
#define E1000_DEV_ID_82571EB_FIBER		0x105F
#define E1000_DEV_ID_82571EB_SERDES		0x1060
#define E1000_DEV_ID_82571EB_SERDES_DUAL	0x10D9
#define E1000_DEV_ID_82571EB_SERDES_QUAD	0x10DA
#define E1000_DEV_ID_82571EB_QUAD_COPPER	0x10A4
#define E1000_DEV_ID_82571PT_QUAD_COPPER	0x10D5
#define E1000_DEV_ID_82571EB_QUAD_FIBER		0x10A5
#define E1000_DEV_ID_82571EB_QUAD_COPPER_LP	0x10BC
#define E1000_DEV_ID_82572EI_COPPER		0x107D
#define E1000_DEV_ID_82572EI_FIBER		0x107E
#define E1000_DEV_ID_82572EI_SERDES		0x107F
#define E1000_DEV_ID_82572EI			0x10B9
#define E1000_DEV_ID_82573E			0x108B
#define E1000_DEV_ID_82573E_IAMT		0x108C
#define E1000_DEV_ID_82573L			0x109A
#endif /* NO_82571_SUPPORT */
#ifndef NO_82574_SUPPORT
#define E1000_DEV_ID_82574L			0x10D3
#define E1000_DEV_ID_82574LA			0x10F6
#define E1000_DEV_ID_82583V			0x150C
#endif /* NO_82574_SUPPORT */
#ifndef NO_80003ES2LAN_SUPPORT
#define E1000_DEV_ID_80003ES2LAN_COPPER_DPT	0x1096
#define E1000_DEV_ID_80003ES2LAN_SERDES_DPT	0x1098
#define E1000_DEV_ID_80003ES2LAN_COPPER_SPT	0x10BA
#define E1000_DEV_ID_80003ES2LAN_SERDES_SPT	0x10BB
#endif /* NO_80003ES2LAN_SUPPORT */
#ifndef NO_ICH8LAN_SUPPORT
#define E1000_DEV_ID_ICH8_82567V_3		0x1501
#define E1000_DEV_ID_ICH8_IGP_M_AMT		0x1049
#define E1000_DEV_ID_ICH8_IGP_AMT		0x104A
#define E1000_DEV_ID_ICH8_IGP_C			0x104B
#define E1000_DEV_ID_ICH8_IFE			0x104C
#define E1000_DEV_ID_ICH8_IFE_GT		0x10C4
#define E1000_DEV_ID_ICH8_IFE_G			0x10C5
#define E1000_DEV_ID_ICH8_IGP_M			0x104D
#define E1000_DEV_ID_ICH9_IGP_M			0x10BF
#define E1000_DEV_ID_ICH9_IGP_M_AMT		0x10F5
#define E1000_DEV_ID_ICH9_IGP_M_V		0x10CB
#define E1000_DEV_ID_ICH9_IGP_AMT		0x10BD
#define E1000_DEV_ID_ICH9_BM			0x10E5
#define E1000_DEV_ID_ICH9_IGP_C			0x294C
#define E1000_DEV_ID_ICH9_IFE			0x10C0
#define E1000_DEV_ID_ICH9_IFE_GT		0x10C3
#define E1000_DEV_ID_ICH9_IFE_G			0x10C2
#define E1000_DEV_ID_ICH10_R_BM_LM		0x10CC
#define E1000_DEV_ID_ICH10_R_BM_LF		0x10CD
#define E1000_DEV_ID_ICH10_R_BM_V		0x10CE
#define E1000_DEV_ID_ICH10_D_BM_LM		0x10DE
#define E1000_DEV_ID_ICH10_D_BM_LF		0x10DF
#define E1000_DEV_ID_ICH10_D_BM_V		0x1525
#define E1000_DEV_ID_PCH_M_HV_LM		0x10EA
#define E1000_DEV_ID_PCH_M_HV_LC		0x10EB
#define E1000_DEV_ID_PCH_D_HV_DM		0x10EF
#define E1000_DEV_ID_PCH_D_HV_DC		0x10F0
#define E1000_DEV_ID_PCH2_LV_LM			0x1502
#define E1000_DEV_ID_PCH2_LV_V			0x1503
#define E1000_DEV_ID_PCH_LPT_I217_LM		0x153A
#define E1000_DEV_ID_PCH_LPT_I217_V		0x153B
#define E1000_DEV_ID_PCH_LPTLP_I218_LM		0x155A
#define E1000_DEV_ID_PCH_LPTLP_I218_V		0x1559
#define E1000_DEV_ID_PCH_I218_LM2		0x15A0
#define E1000_DEV_ID_PCH_I218_V2		0x15A1
#define E1000_DEV_ID_PCH_I218_LM3		0x15A2 /* Wildcat Point PCH */
#define E1000_DEV_ID_PCH_I218_V3		0x15A3 /* Wildcat Point PCH */
#define E1000_DEV_ID_PCH_SPT_I219_LM		0x156F /* Sunrise Point PCH */
#define E1000_DEV_ID_PCH_SPT_I219_V		0x1570 /* Sunrise Point PCH */
#define E1000_DEV_ID_PCH_SPT_I219_LM2		0x15B7 /* Sunrise Point-H PCH */
#define E1000_DEV_ID_PCH_SPT_I219_V2		0x15B8 /* Sunrise Point-H PCH */
#define E1000_DEV_ID_PCH_LBG_I219_LM3		0x15B9 /* LEWISBURG PCH */
#define E1000_DEV_ID_PCH_SPT_I219_LM4		0x15D7
#define E1000_DEV_ID_PCH_SPT_I219_V4		0x15D8
#define E1000_DEV_ID_PCH_SPT_I219_LM5		0x15E3
#define E1000_DEV_ID_PCH_SPT_I219_V5		0x15D6
#define E1000_DEV_ID_PCH_CNP_I219_LM6		0x15BD
#define E1000_DEV_ID_PCH_CNP_I219_V6		0x15BE
#define E1000_DEV_ID_PCH_CNP_I219_LM7		0x15BB
#define E1000_DEV_ID_PCH_CNP_I219_V7		0x15BC
#define E1000_DEV_ID_PCH_ICP_I219_LM8		0x15DF
#define E1000_DEV_ID_PCH_ICP_I219_V8		0x15E0
#define E1000_DEV_ID_PCH_ICP_I219_LM9		0x15E1
#define E1000_DEV_ID_PCH_ICP_I219_V9		0x15E2
#define E1000_DEV_ID_PCH_CMP_I219_LM10		0x0D4E
#define E1000_DEV_ID_PCH_CMP_I219_V10		0x0D4F
#define E1000_DEV_ID_PCH_CMP_I219_LM11		0x0D4C
#define E1000_DEV_ID_PCH_CMP_I219_V11		0x0D4D
#define E1000_DEV_ID_PCH_CMP_I219_LM12		0x0D53
#define E1000_DEV_ID_PCH_CMP_I219_V12		0x0D55
#define E1000_DEV_ID_PCH_TGP_I219_LM13		0x15FB
#define E1000_DEV_ID_PCH_TGP_I219_V13		0x15FC
#define E1000_DEV_ID_PCH_TGP_I219_LM14		0x15F9
#define E1000_DEV_ID_PCH_TGP_I219_V14		0x15FA
#define E1000_DEV_ID_PCH_TGP_I219_LM15		0x15F4
#define E1000_DEV_ID_PCH_TGP_I219_V15		0x15F5
#define E1000_DEV_ID_PCH_ADL_I219_LM16		0x1A1E
#define E1000_DEV_ID_PCH_ADL_I219_V16		0x1A1F
#define E1000_DEV_ID_PCH_RPL_I219_LM23		0x0DC5
#define E1000_DEV_ID_PCH_RPL_I219_V23		0x0DC6
#define E1000_DEV_ID_PCH_ADL_I219_LM17		0x1A1C
#define E1000_DEV_ID_PCH_ADL_I219_V17		0x1A1D
#define E1000_DEV_ID_PCH_RPL_I219_LM22		0x0DC7
#define E1000_DEV_ID_PCH_RPL_I219_V22		0x0DC8
#ifdef NAHUM11_HW
#define E1000_DEV_ID_PCH_MTP_I219_LM18		0x550A
#define E1000_DEV_ID_PCH_MTP_I219_V18		0x550B
#define E1000_DEV_ID_PCH_LNL_I219_LM20		0x550E
#define E1000_DEV_ID_PCH_LNL_I219_V20		0x550F
#define E1000_DEV_ID_PCH_LNL_I219_LM21		0x5510
#define E1000_DEV_ID_PCH_LNL_I219_V21		0x5511
#ifdef ARROWLAKE_HW
#define E1000_DEV_ID_PCH_ARL_I219_LM24		0x57A0
#define E1000_DEV_ID_PCH_ARL_I219_V24		0x57A1
#endif /* ARROWLAKE_HW */
#endif /* NAHUM11_HW */
#ifdef NAHUM12_HW
#define E1000_DEV_ID_PCH_MTP_I219_LM19		0x550C
#define E1000_DEV_ID_PCH_MTP_I219_V19		0x550D
#endif /* NAHUM12_HW */
#ifdef NAHUM13_HW
#define E1000_DEV_ID_PCH_PTP_I219_LM25		0x57B3
#define E1000_DEV_ID_PCH_PTP_I219_V25		0x57B4
#define E1000_DEV_ID_PCH_PTP_I219_LM26		0x57B5
#define E1000_DEV_ID_PCH_PTP_I219_V26		0x57B6
#endif /* NAHUM13_HW */
#ifdef NAHUM14_HW
#define E1000_DEV_ID_PCH_PTP_I219_LM27		0x57B7
#define E1000_DEV_ID_PCH_PTP_I219_V27		0x57B8
#endif /* NAHUM14_HW */
#endif /* NO_ICH8LAN_SUPPORT */
#ifndef NO_82575_SUPPORT
#define E1000_DEV_ID_82576			0x10C9
#define E1000_DEV_ID_82576_FIBER		0x10E6
#define E1000_DEV_ID_82576_SERDES		0x10E7
#define E1000_DEV_ID_82576_QUAD_COPPER		0x10E8
#define E1000_DEV_ID_82576_QUAD_COPPER_ET2	0x1526
#define E1000_DEV_ID_82576_NS			0x150A
#define E1000_DEV_ID_82576_NS_SERDES		0x1518
#define E1000_DEV_ID_82576_SERDES_QUAD		0x150D
#define E1000_DEV_ID_82575EB_COPPER		0x10A7
#define E1000_DEV_ID_82575EB_FIBER_SERDES	0x10A9
#define E1000_DEV_ID_82575GB_QUAD_COPPER	0x10D6
#endif /* NO_82575_SUPPORT */
#ifndef NO_82580_SUPPORT
#define E1000_DEV_ID_82580_COPPER		0x150E
#define E1000_DEV_ID_82580_FIBER		0x150F
#define E1000_DEV_ID_82580_SERDES		0x1510
#define E1000_DEV_ID_82580_SGMII		0x1511
#define E1000_DEV_ID_82580_COPPER_DUAL		0x1516
#define E1000_DEV_ID_82580_QUAD_FIBER		0x1527
#endif /* NO_82580_SUPPORT */
#ifndef NO_82575_SUPPORT
#define E1000_DEV_ID_I350_COPPER		0x1521
#define E1000_DEV_ID_I350_FIBER			0x1522
#define E1000_DEV_ID_I350_SERDES		0x1523
#define E1000_DEV_ID_I350_SGMII			0x1524
#define E1000_DEV_ID_I350_DA4			0x1546
#ifndef NO_I210_SUPPORT
#define E1000_DEV_ID_I210_COPPER		0x1533
#define E1000_DEV_ID_I210_COPPER_OEM1		0x1534
#define E1000_DEV_ID_I210_COPPER_IT		0x1535
#define E1000_DEV_ID_I210_FIBER			0x1536
#define E1000_DEV_ID_I210_SERDES		0x1537
#define E1000_DEV_ID_I210_SGMII			0x1538
#define E1000_DEV_ID_I210_COPPER_FLASHLESS	0x157B
#define E1000_DEV_ID_I210_SERDES_FLASHLESS	0x157C
#define E1000_DEV_ID_I210_SGMII_FLASHLESS	0x15F6
#define E1000_DEV_ID_I211_COPPER		0x1539
#endif /* NO_I210_SUPPORT */
#endif /* NO_82575_SUPPORT */
#ifndef NO_82575_SUPPORT
#define E1000_DEV_ID_I354_BACKPLANE_1GBPS	0x1F40
#define E1000_DEV_ID_I354_SGMII			0x1F41
#define E1000_DEV_ID_I354_BACKPLANE_2_5GBPS	0x1F45
#endif /* NO_82575_SUPPORT */
#ifndef NO_DH89XXCC_SUPPORT
#define E1000_DEV_ID_DH89XXCC_SGMII		0x0438
#define E1000_DEV_ID_DH89XXCC_SERDES		0x043A
#define E1000_DEV_ID_DH89XXCC_BACKPLANE		0x043C
#define E1000_DEV_ID_DH89XXCC_SFP		0x0440
#endif /* NO_DH89XXCC_SUPPORT */

#define E1000_REVISION_0	0
#define E1000_REVISION_1	1
#define E1000_REVISION_2	2
#define E1000_REVISION_3	3
#define E1000_REVISION_4	4

#define E1000_FUNC_0		0
#define E1000_FUNC_1		1
#ifndef NO_82580_SUPPORT
#define E1000_FUNC_2		2
#define E1000_FUNC_3		3
#endif /* Barton Hills HW */

#define E1000_ALT_MAC_ADDRESS_OFFSET_LAN0	0
#define E1000_ALT_MAC_ADDRESS_OFFSET_LAN1	3
#ifndef NO_82580_SUPPORT
#define E1000_ALT_MAC_ADDRESS_OFFSET_LAN2	6
#define E1000_ALT_MAC_ADDRESS_OFFSET_LAN3	9
#endif /* Barton Hills HW */

enum e1000_mac_type {
	e1000_undefined = 0,
#ifndef NO_82571_SUPPORT
	e1000_82571,
	e1000_82572,
	e1000_82573,
#ifndef NO_82574_SUPPORT
	e1000_82574,
	e1000_82583,
#endif /* NO_82574_SUPPORT */
#endif
#ifndef NO_80003ES2LAN_SUPPORT
	e1000_80003es2lan,
#endif
#ifndef NO_ICH8LAN_SUPPORT
	e1000_ich8lan,
	e1000_ich9lan,
	e1000_ich10lan,
	e1000_pchlan,
	e1000_pch2lan,
	e1000_pch_lpt,
	e1000_pch_spt,
	e1000_pch_cnp,
	e1000_pch_tgp,
	e1000_pch_adp,
#ifdef NAHUM11_HW
	e1000_pch_mtp,
#endif
#ifdef NAHUM13_HW
	e1000_pch_ptp,
#endif /* NAHUM13_HW */
#endif /* NO_ICH8LAN_SUPPORT */
#ifndef NO_82575_SUPPORT
	e1000_82575,
	e1000_82576,
#ifndef NO_82580_SUPPORT
	e1000_82580,
#endif
	e1000_i350,
	e1000_i354,
#ifndef NO_I210_SUPPORT
	e1000_i210,
	e1000_i211,
#endif /* NO_I210_SUPPORT */
#endif /* NO_82575_SUPPORT */
#ifndef NO_82575_SUPPORT
#endif /* NO_82575_SUPPORT */
	e1000_num_macs  /* List is 1-based, so subtract 1 for true count. */
};

enum e1000_media_type {
	e1000_media_type_unknown = 0,
	e1000_media_type_copper = 1,
	e1000_media_type_fiber = 2,
	e1000_media_type_internal_serdes = 3,
	e1000_num_media_types
};

enum e1000_nvm_type {
	e1000_nvm_unknown = 0,
	e1000_nvm_none,
	e1000_nvm_eeprom_spi,
#ifndef NO_MICROWIRE_SUPPORT
	e1000_nvm_eeprom_microwire,
#endif
	e1000_nvm_flash_hw,
#if !defined(NO_I210_SUPPORT) 
	e1000_nvm_invm,
#endif /* NO_I210_SUPPORT || NO_I225_SUPPORT */
	e1000_nvm_flash_sw
};

enum e1000_nvm_override {
	e1000_nvm_override_none = 0,
	e1000_nvm_override_spi_small,
	e1000_nvm_override_spi_large,
#ifndef NO_MICROWIRE_SUPPORT
	e1000_nvm_override_microwire_small,
	e1000_nvm_override_microwire_large
#endif
};

enum e1000_phy_type {
	e1000_phy_unknown = 0,
	e1000_phy_none,
	e1000_phy_m88,
	e1000_phy_igp,
	e1000_phy_igp_2,
	e1000_phy_gg82563,
	e1000_phy_igp_3,
	e1000_phy_ife,
#ifndef NO_ICH8LAN_SUPPORT
	e1000_phy_bm,
	e1000_phy_82578,
	e1000_phy_82577,
	e1000_phy_82579,
	e1000_phy_i217,
#endif /* NO_ICH8LAN_SUPPORT */
#ifndef NO_82580_SUPPORT
	e1000_phy_82580,
#endif /* NO_82580_SUPPORT */
#ifndef NO_82575_SUPPORT
	e1000_phy_vf,
#endif /* NO_82575_SUPPORT */
#ifndef NO_I210_SUPPORT
	e1000_phy_i210,
#endif /* NO_I210_SUPPORT */
};

enum e1000_bus_type {
	e1000_bus_type_unknown = 0,
	e1000_bus_type_pci,
	e1000_bus_type_pcix,
	e1000_bus_type_pci_express,
	e1000_bus_type_reserved
};

enum e1000_bus_speed {
	e1000_bus_speed_unknown = 0,
	e1000_bus_speed_33,
	e1000_bus_speed_66,
	e1000_bus_speed_100,
	e1000_bus_speed_120,
	e1000_bus_speed_133,
	e1000_bus_speed_2500,
	e1000_bus_speed_5000,
	e1000_bus_speed_reserved
};

enum e1000_bus_width {
	e1000_bus_width_unknown = 0,
	e1000_bus_width_pcie_x1,
	e1000_bus_width_pcie_x2,
	e1000_bus_width_pcie_x4 = 4,
	e1000_bus_width_pcie_x8 = 8,
	e1000_bus_width_32,
	e1000_bus_width_64,
	e1000_bus_width_reserved
};

enum e1000_1000t_rx_status {
	e1000_1000t_rx_status_not_ok = 0,
	e1000_1000t_rx_status_ok,
	e1000_1000t_rx_status_undefined = 0xFF
};

enum e1000_rev_polarity {
	e1000_rev_polarity_normal = 0,
	e1000_rev_polarity_reversed,
	e1000_rev_polarity_undefined = 0xFF
};

enum e1000_fc_mode {
	e1000_fc_none = 0,
	e1000_fc_rx_pause,
	e1000_fc_tx_pause,
	e1000_fc_full,
	e1000_fc_default = 0xFF
};

enum e1000_ms_type {
	e1000_ms_hw_default = 0,
	e1000_ms_force_primary,
	e1000_ms_force_secondary,
	e1000_ms_auto
};

enum e1000_smart_speed {
	e1000_smart_speed_default = 0,
	e1000_smart_speed_on,
	e1000_smart_speed_off
};

enum e1000_serdes_link_state {
	e1000_serdes_link_down = 0,
	e1000_serdes_link_autoneg_progress,
	e1000_serdes_link_autoneg_complete,
	e1000_serdes_link_forced_up
};

#ifndef __le16
#define __le16 u16
#endif
#ifndef __le32
#define __le32 u32
#endif
#ifndef __le64
#define __le64 u64
#endif
#ifndef NO_LEGACY_RX_DESC_SUPPORT
/* Receive Descriptor */
struct e1000_rx_desc {
	__le64 buffer_addr; /* Address of the descriptor's data buffer */
	__le16 length;      /* Length of data DMAed into data buffer */
	__le16 csum; /* Packet checksum */
	u8  status;  /* Descriptor status */
	u8  errors;  /* Descriptor Errors */
	__le16 special;
};

#endif /* NO_LEGACY_RX_DESC_SUPPORT */
/* Receive Descriptor - Extended */
union e1000_rx_desc_extended {
	struct {
		__le64 buffer_addr;
		__le64 reserved;
	} read;
	struct {
		struct {
			__le32 mrq; /* Multiple Rx Queues */
			union {
				__le32 rss; /* RSS Hash */
				struct {
					__le16 ip_id;  /* IP id */
					__le16 csum;   /* Packet Checksum */
				} csum_ip;
			} hi_dword;
		} lower;
		struct {
			__le32 status_error;  /* ext status/error */
			__le16 length;
			__le16 vlan; /* VLAN tag */
		} upper;
	} wb;  /* writeback */
};

#define MAX_PS_BUFFERS 4

/* Number of packet split data buffers (not including the header buffer) */
#define PS_PAGE_BUFFERS	(MAX_PS_BUFFERS - 1)

/* Receive Descriptor - Packet Split */
union e1000_rx_desc_packet_split {
	struct {
		/* one buffer for protocol header(s), three data buffers */
		__le64 buffer_addr[MAX_PS_BUFFERS];
	} read;
	struct {
		struct {
			__le32 mrq;  /* Multiple Rx Queues */
			union {
				__le32 rss; /* RSS Hash */
				struct {
					__le16 ip_id;    /* IP id */
					__le16 csum;     /* Packet Checksum */
				} csum_ip;
			} hi_dword;
		} lower;
		struct {
			__le32 status_error;  /* ext status/error */
			__le16 length0;  /* length of buffer 0 */
			__le16 vlan;  /* VLAN tag */
		} middle;
		struct {
			__le16 header_status;
			/* length of buffers 1-3 */
			__le16 length[PS_PAGE_BUFFERS];
		} upper;
		__le64 reserved;
	} wb; /* writeback */
};

/* Transmit Descriptor */
struct e1000_tx_desc {
	__le64 buffer_addr;   /* Address of the descriptor's data buffer */
	union {
		__le32 data;
		struct {
			__le16 length;  /* Data buffer length */
			u8 cso;  /* Checksum offset */
			u8 cmd;  /* Descriptor control */
		} flags;
	} lower;
	union {
		__le32 data;
		struct {
			u8 status; /* Descriptor status */
			u8 css;  /* Checksum start */
			__le16 special;
		} fields;
	} upper;
};

/* Offload Context Descriptor */
struct e1000_context_desc {
	union {
		__le32 ip_config;
		struct {
			u8 ipcss;  /* IP checksum start */
			u8 ipcso;  /* IP checksum offset */
			__le16 ipcse;  /* IP checksum end */
		} ip_fields;
	} lower_setup;
	union {
		__le32 tcp_config;
		struct {
			u8 tucss;  /* TCP checksum start */
			u8 tucso;  /* TCP checksum offset */
			__le16 tucse;  /* TCP checksum end */
		} tcp_fields;
	} upper_setup;
	__le32 cmd_and_length;
	union {
		__le32 data;
		struct {
			u8 status;  /* Descriptor status */
			u8 hdr_len;  /* Header length */
			__le16 mss;  /* Maximum segment size */
		} fields;
	} tcp_seg_setup;
};

/* Offload data descriptor */
struct e1000_data_desc {
	__le64 buffer_addr;  /* Address of the descriptor's buffer address */
	union {
		__le32 data;
		struct {
			__le16 length;  /* Data buffer length */
			u8 typ_len_ext;
			u8 cmd;
		} flags;
	} lower;
	union {
		__le32 data;
		struct {
			u8 status;  /* Descriptor status */
			u8 popts;  /* Packet Options */
			__le16 special;
		} fields;
	} upper;
};

/* Statistics counters collected by the MAC */
struct e1000_hw_stats {
	u64 crcerrs;
	u64 algnerrc;
	u64 symerrs;
	u64 rxerrc;
	u64 mpc;
	u64 scc;
	u64 ecol;
	u64 mcc;
	u64 latecol;
	u64 colc;
	u64 dc;
	u64 tncrs;
	u64 sec;
	u64 cexterr;
	u64 rlec;
	u64 xonrxc;
	u64 xontxc;
	u64 xoffrxc;
	u64 xofftxc;
	u64 fcruc;
	u64 prc64;
	u64 prc127;
	u64 prc255;
	u64 prc511;
	u64 prc1023;
	u64 prc1522;
	u64 gprc;
	u64 bprc;
	u64 mprc;
	u64 gptc;
	u64 gorc;
	u64 gotc;
	u64 rnbc;
	u64 ruc;
	u64 rfc;
	u64 roc;
	u64 rjc;
	u64 mgprc;
	u64 mgpdc;
	u64 mgptc;
	u64 tor;
	u64 tot;
	u64 tpr;
	u64 tpt;
	u64 ptc64;
	u64 ptc127;
	u64 ptc255;
	u64 ptc511;
	u64 ptc1023;
	u64 ptc1522;
	u64 mptc;
	u64 bptc;
	u64 tsctc;
	u64 tsctfc;
	u64 iac;
	u64 icrxptc;
	u64 icrxatc;
	u64 ictxptc;
	u64 ictxatc;
	u64 ictxqec;
	u64 ictxqmtc;
	u64 icrxdmtc;
	u64 icrxoc;
	u64 cbtmpc;
	u64 htdpmc;
	u64 cbrdpc;
	u64 cbrmpc;
	u64 rpthc;
	u64 hgptc;
	u64 htcbdpc;
	u64 hgorc;
	u64 hgotc;
	u64 lenerrs;
	u64 scvpc;
	u64 hrmpc;
#if !defined(NO_82575_SUPPORT) 
	u64 doosync;
	u64 o2bgptc;
	u64 o2bspc;
	u64 b2ospc;
	u64 b2ogprc;
#endif /* NO_82575_SUPPORT || NO_I225_SUPPORT */
};


struct e1000_phy_stats {
	u32 idle_errors;
	u32 receive_errors;
};

struct e1000_host_mng_dhcp_cookie {
	u32 signature;
	u8  status;
	u8  reserved0;
	u16 vlan_id;
	u32 reserved1;
	u16 reserved2;
	u8  reserved3;
	u8  checksum;
};

/* Host Interface "Rev 1" */
struct e1000_host_command_header {
	u8 command_id;
	u8 command_length;
	u8 command_options;
	u8 checksum;
};

#define E1000_HI_MAX_DATA_LENGTH	252
struct e1000_host_command_info {
	struct e1000_host_command_header command_header;
	u8 command_data[E1000_HI_MAX_DATA_LENGTH];
};

/* Host Interface "Rev 2" */
struct e1000_host_mng_command_header {
	u8  command_id;
	u8  checksum;
	u16 reserved1;
	u16 reserved2;
	u16 command_length;
};

#define E1000_HI_MAX_MNG_DATA_LENGTH	0x6F8
struct e1000_host_mng_command_info {
	struct e1000_host_mng_command_header command_header;
	u8 command_data[E1000_HI_MAX_MNG_DATA_LENGTH];
};

#include "e1000_mac.h"
#include "e1000_phy.h"
#include "e1000_nvm.h"
#include "e1000_manage.h"

#ifdef E1000_NVMUPD_SUPPORT
/* NVM Update commands */
#define E1000_NVMUPD_CMD_REG_READ	0x0000000B
#define E1000_NVMUPD_CMD_REG_WRITE	0x0000000C

/* NVM Update features API */
#define E1000_NVMUPD_FEATURES_API_VER_MAJOR		0
#define E1000_NVMUPD_FEATURES_API_VER_MINOR		0
#define E1000_NVMUPD_FEATURES_API_FEATURES_ARRAY_LEN	12
#define E1000_NVMUPD_EXEC_FEATURES			0xE
#define E1000_NVMUPD_FEATURE_FLAT_NVM_SUPPORT		(1 << 0)
#define E1000_NVMUPD_FEATURE_REGISTER_ACCESS_SUPPORT	(1 << 1)

#define E1000_NVMUPD_MOD_PNT_MASK			0xFF

struct e1000_nvm_access {
	u32 command;
	u32 config;
	u32 offset;	/* in bytes */
	u32 data_size;	/* in bytes */
	u8 data[1];
};

struct e1000_nvm_features {
	u8 major;
	u8 minor;
	u16 size;
	u8 features[E1000_NVMUPD_FEATURES_API_FEATURES_ARRAY_LEN];
};
#endif

/* Function pointers for the MAC. */
struct e1000_mac_operations {
	s32  (*init_params)(struct e1000_hw *);
	s32  (*id_led_init)(struct e1000_hw *);
	s32  (*blink_led)(struct e1000_hw *);
	bool (*check_mng_mode)(struct e1000_hw *);
	s32  (*check_for_link)(struct e1000_hw *);
	s32  (*cleanup_led)(struct e1000_hw *);
	void (*clear_hw_cntrs)(struct e1000_hw *);
	void (*clear_vfta)(struct e1000_hw *);
	s32  (*get_bus_info)(struct e1000_hw *);
	void (*set_lan_id)(struct e1000_hw *);
	s32  (*get_link_up_info)(struct e1000_hw *, u16 *, u16 *);
	s32  (*led_on)(struct e1000_hw *);
	s32  (*led_off)(struct e1000_hw *);
	void (*update_mc_addr_list)(struct e1000_hw *, u8 *, u32);
	s32  (*reset_hw)(struct e1000_hw *);
	s32  (*init_hw)(struct e1000_hw *);
#ifndef NO_82575_SUPPORT
	void (*shutdown_serdes)(struct e1000_hw *);
	void (*power_up_serdes)(struct e1000_hw *);
#endif /* NO_82575_SUPPORT */
	s32  (*setup_link)(struct e1000_hw *);
	s32  (*setup_physical_interface)(struct e1000_hw *);
	s32  (*setup_led)(struct e1000_hw *);
	void (*write_vfta)(struct e1000_hw *, u32, u32);
	void (*config_collision_dist)(struct e1000_hw *);
	int  (*rar_set)(struct e1000_hw *, u8*, u32);
	s32  (*read_mac_addr)(struct e1000_hw *);
	s32  (*validate_mdi_setting)(struct e1000_hw *);
#if !defined(NO_82575_SUPPORT) 
	s32  (*acquire_swfw_sync)(struct e1000_hw *, u16);
	void (*release_swfw_sync)(struct e1000_hw *, u16);
#endif /* NO_82575_SUPPORT or NO_I225_SUPPORT */
};

/* When to use various PHY register access functions:
 *
 *                 Func   Caller
 *   Function      Does   Does    When to use
 *   ~~~~~~~~~~~~  ~~~~~  ~~~~~~  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *   X_reg         L,P,A  n/a     for simple PHY reg accesses
 *   X_reg_locked  P,A    L       for multiple accesses of different regs
 *                                on different pages
 *   X_reg_page    A      L,P     for multiple accesses of different regs
 *                                on the same page
 *
 * Where X=[read|write], L=locking, P=sets page, A=register access
 *
 */
struct e1000_phy_operations {
	s32  (*init_params)(struct e1000_hw *);
	s32  (*acquire)(struct e1000_hw *);
#ifndef NO_80003ES2LAN_SUPPORT
	s32  (*cfg_on_link_up)(struct e1000_hw *);
#endif
	s32  (*check_polarity)(struct e1000_hw *);
	s32  (*check_reset_block)(struct e1000_hw *);
	s32  (*commit)(struct e1000_hw *);
	s32  (*force_speed_duplex)(struct e1000_hw *);
	s32  (*get_cfg_done)(struct e1000_hw *hw);
	s32  (*get_cable_length)(struct e1000_hw *);
	s32  (*get_info)(struct e1000_hw *);
	s32  (*set_page)(struct e1000_hw *, u16);
	s32  (*read_reg)(struct e1000_hw *, u32, u16 *);
	s32  (*read_reg_locked)(struct e1000_hw *, u32, u16 *);
	s32  (*read_reg_page)(struct e1000_hw *, u32, u16 *);
	void (*release)(struct e1000_hw *);
	s32  (*reset)(struct e1000_hw *);
	s32  (*set_d0_lplu_state)(struct e1000_hw *, bool);
	s32  (*set_d3_lplu_state)(struct e1000_hw *, bool);
	s32  (*write_reg)(struct e1000_hw *, u32, u16);
	s32  (*write_reg_locked)(struct e1000_hw *, u32, u16);
	s32  (*write_reg_page)(struct e1000_hw *, u32, u16);
	void (*power_up)(struct e1000_hw *);
	void (*power_down)(struct e1000_hw *);
};

/* Function pointers for the NVM. */
struct e1000_nvm_operations {
	s32  (*init_params)(struct e1000_hw *);
	s32  (*acquire)(struct e1000_hw *);
	s32  (*read)(struct e1000_hw *, u16, u16, u16 *);
	void (*release)(struct e1000_hw *);
	void (*reload)(struct e1000_hw *);
	s32  (*update)(struct e1000_hw *);
	s32  (*valid_led_default)(struct e1000_hw *, u16 *);
	s32  (*validate)(struct e1000_hw *);
	s32  (*write)(struct e1000_hw *, u16, u16, u16 *);
};

struct e1000_mac_info {
	struct e1000_mac_operations ops;
	u8 addr[ETH_ADDR_LEN];
	u8 perm_addr[ETH_ADDR_LEN];

	enum e1000_mac_type type;

	u32 collision_delta;
	u32 ledctl_default;
	u32 ledctl_mode1;
	u32 ledctl_mode2;
	u32 mc_filter_type;
	u32 tx_packet_delta;
	u32 txcw;

	u16 current_ifs_val;
	u16 ifs_max_val;
	u16 ifs_min_val;
	u16 ifs_ratio;
	u16 ifs_step_size;
	u16 mta_reg_count;
#if !defined(NO_82575_SUPPORT) 
	u16 uta_reg_count;
#endif /* NO_82575_SUPPORT || NO-I225_SUPPORT */

	/* Maximum size of the MTA register table in all supported adapters */
#define MAX_MTA_REG 128
	u32 mta_shadow[MAX_MTA_REG];
	u16 rar_entry_count;

	u8  forced_speed_duplex;

	bool adaptive_ifs;
	bool has_fwsm;
	bool arc_subsystem_valid;
#ifndef NO_ASF_FIRMWARE_CHECK
	bool asf_firmware_present;
#endif
	bool autoneg;
	bool autoneg_failed;
	bool get_link_status;
	bool in_ifs_mode;
	enum e1000_serdes_link_state serdes_link_state;
	bool serdes_has_link;
	bool tx_pkt_filtering;
};

struct e1000_phy_info {
	struct e1000_phy_operations ops;
	enum e1000_phy_type type;

	enum e1000_1000t_rx_status local_rx;
	enum e1000_1000t_rx_status remote_rx;
	enum e1000_ms_type ms_type;
	enum e1000_ms_type original_ms_type;
	enum e1000_rev_polarity cable_polarity;
	enum e1000_smart_speed smart_speed;

	u32 addr;
	u32 id;
	u32 reset_delay_us; /* in usec */
	u32 revision;

	enum e1000_media_type media_type;

	u16 autoneg_advertised;
	u16 autoneg_mask;
	u16 cable_length;
	u16 max_cable_length;
	u16 min_cable_length;

	u8 mdix;

	bool disable_polarity_correction;
	bool is_mdix;
	bool polarity_correction;
#ifndef NO_PHY_RESET_DISABLE
	bool reset_disable;
#endif
	bool speed_downgraded;
	bool autoneg_wait_to_complete;
};

struct e1000_nvm_info {
	struct e1000_nvm_operations ops;
	enum e1000_nvm_type type;
	enum e1000_nvm_override override;

	u32 flash_bank_size;
	u32 flash_base_addr;

	u16 word_size;
	u16 delay_usec;
	u16 address_bits;
	u16 opcode_bits;
	u16 page_size;
};

struct e1000_bus_info {
	enum e1000_bus_type type;
	enum e1000_bus_speed speed;
	enum e1000_bus_width width;

	u16 func;
	u16 pci_cmd_word;
};

struct e1000_fc_info {
	u32 high_water;  /* Flow control high-water mark */
	u32 low_water;  /* Flow control low-water mark */
	u16 pause_time;  /* Flow control pause timer */
	u16 refresh_time;  /* Flow control refresh timer */
	bool send_xon;  /* Flow control send XON */
	bool strict_ieee;  /* Strict IEEE mode */
	enum e1000_fc_mode current_mode;  /* FC mode in effect */
	enum e1000_fc_mode requested_mode;  /* FC mode requested by caller */
};

#ifndef NO_82571_SUPPORT
struct e1000_dev_spec_82571 {
	bool laa_is_present;
	u32 smb_counter;
	E1000_MUTEX swflag_mutex;
};

#endif /* NO_82571_SUPPORT */
#ifndef NO_80003ES2LAN_SUPPORT
struct e1000_dev_spec_80003es2lan {
	bool  mdic_wa_enable;
};

#endif /* NO_80003ES2LAN_SUPPORT */
#ifndef NO_ICH8LAN_SUPPORT
struct e1000_shadow_ram {
	u16  value;
	bool modified;
};

#define E1000_SHADOW_RAM_WORDS		2048

struct e1000_dev_spec_ich8lan {
	bool kmrn_lock_loss_workaround_enabled;
	struct e1000_shadow_ram shadow_ram[E1000_SHADOW_RAM_WORDS];
	E1000_MUTEX nvm_mutex;
	E1000_MUTEX swflag_mutex;
	bool nvm_k1_enabled;
	bool disable_k1_off;
	bool eee_disable;
	u16 eee_lp_ability;
#ifdef S0_IDLE_SUPPORT
	bool smbus_disable;
#endif
};

#endif /* NO_ICH8LAN_SUPPORT */
#ifndef NO_82575_SUPPORT
struct e1000_dev_spec_82575 {
	bool sgmii_active;
	bool global_device_reset;
	bool eee_disable;
	bool module_plugged;
#ifndef NO_I210_SUPPORT
	bool clear_semaphore_once;
#endif /* NO_I210_SUPPORT */
	u32 mtu;
	struct sfp_e1000_flags eth_flags;
	u8 media_port;
	bool media_changed;
};

struct e1000_dev_spec_vf {
	u32 vf_number;
	u32 v2p_mailbox;
};

#endif /* NO_82575_SUPPORT */
struct e1000_hw {
	void *back;

	u8 *hw_addr;
	u8 *flash_address;
	unsigned long io_base;

	struct e1000_mac_info  mac;
	struct e1000_fc_info   fc;
	struct e1000_phy_info  phy;
	struct e1000_nvm_info  nvm;
	struct e1000_bus_info  bus;
	struct e1000_host_mng_dhcp_cookie mng_cookie;

	union {
#ifndef NO_82571_SUPPORT
		struct e1000_dev_spec_82571 _82571;
#endif
#ifndef NO_80003ES2LAN_SUPPORT
		struct e1000_dev_spec_80003es2lan _80003es2lan;
#endif
#ifndef NO_ICH8LAN_SUPPORT
		struct e1000_dev_spec_ich8lan ich8lan;
#endif
#ifndef NO_82575_SUPPORT
		struct e1000_dev_spec_82575 _82575;
		struct e1000_dev_spec_vf vf;
#endif /* NO_82575_SUPPORT */
	} dev_spec;

	u16 device_id;
	u16 subsystem_vendor_id;
	u16 subsystem_device_id;
	u16 vendor_id;

	u8  revision_id;
#ifdef E1000_NVMUPD_SUPPORT
	/* NVM Update features */
	struct e1000_nvm_features nvmupd_features;
#endif
};

#ifndef NO_82571_SUPPORT
#include "e1000_82571.h"
#endif
#ifndef NO_80003ES2LAN_SUPPORT
#include "e1000_80003es2lan.h"
#endif
#ifndef NO_ICH8LAN_SUPPORT
#include "e1000_ich8lan.h"
#endif
#ifndef NO_82575_SUPPORT
#include "e1000_82575.h"
#endif
#ifndef NO_I210_SUPPORT
#include "e1000_i210.h"
#endif
#if !defined(NO_82575_SUPPORT) 
#include "e1000_base.h"
#endif

/* These functions must be implemented by drivers */
#ifndef NO_PCIE_SUPPORT
s32  e1000_read_pcie_cap_reg(struct e1000_hw *hw, u32 reg, u16 *value);
s32  e1000_write_pcie_cap_reg(struct e1000_hw *hw, u32 reg, u16 *value);
#endif
#if !defined(NO_PCI_SUPPORT) || !defined(NO_PCIX_SUPPORT)
void e1000_read_pci_cfg(struct e1000_hw *hw, u32 reg, u16 *value);
#endif
#ifndef NO_PCIX_SUPPORT
void e1000_write_pci_cfg(struct e1000_hw *hw, u32 reg, u16 *value);
#ifdef PREBOOT_SUPPORT
void e1000_write_pci_cfg_pb(struct e1000_hw *hw, u32 reg, u16 *value);
#endif /* PREBOOT_SUPPORT */
#endif

#endif
