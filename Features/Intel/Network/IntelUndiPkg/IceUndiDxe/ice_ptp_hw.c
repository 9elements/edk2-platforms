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

#ifndef NO_PTP_SUPPORT
#include "ice_type.h"
#include "ice_common.h"
#include "ice_ptp_hw.h"
#include "ice_ptp_consts.h"
#ifdef CGU_SUPPORT
#include "ice_cgu_regs.h"
#endif /* CGU_SUPPORT */


/* Low level functions for interacting with and managing the device clock used
 * for the Precision Time Protocol.
 *
 * The ice hardware represents the current time using three registers:
 *
 *    GLTSYN_TIME_H     GLTSYN_TIME_L     GLTSYN_TIME_R
 *  +---------------+ +---------------+ +---------------+
 *  |    32 bits    | |    32 bits    | |    32 bits    |
 *  +---------------+ +---------------+ +---------------+
 *
 * The registers are incremented every clock tick using a 40bit increment
 * value defined over two registers:
 *
 *                     GLTSYN_INCVAL_H   GLTSYN_INCVAL_L
 *                    +---------------+ +---------------+
 *                    |    8 bit s    | |    32 bits    |
 *                    +---------------+ +---------------+
 *
 * The increment value is added to the GLSTYN_TIME_R and GLSTYN_TIME_L
 * registers every clock source tick. Depending on the specific device
 * configuration, the clock source frequency could be one of a number of
 * values.
 *
 * For E810 devices, the increment frequency is 812.5 MHz
 *
 * For E822 devices the clock can be derived from different sources, and the
 * increment has an effective frequency of one of the following:
 * - 823.4375 MHz
 * - 783.36 MHz
 * - 796.875 MHz
 * - 816 MHz
 * - 830.078125 MHz
 * - 783.36 MHz
 *
 * The hardware captures timestamps in the PHY for incoming packets, and for
 * outgoing packets on request. To support this, the PHY maintains a timer
 * that matches the lower 64 bits of the global source timer.
 *
 * In order to ensure that the PHY timers and the source timer are equivalent,
 * shadow registers are used to prepare the desired initial values. A special
 * sync command is issued to trigger copying from the shadow registers into
 * the appropriate source and PHY registers simultaneously.
 *
 * The driver supports devices which have different PHYs with subtly different
 * mechanisms to program and control the timers. We divide the devices into
 * families named after the first major device, E810 and similar devices, and
 * E822 and similar devices.
 *
 * - E822 based devices have additional support for fine grained Vernier
 *   calibration which requires significant setup
 * - The layout of timestamp data in the PHY register blocks is different
 * - The way timer synchronization commands are issued is different.
 *
 * To support this, very low level functions have an e810 or e822 suffix
 * indicating what type of device they work on. Higher level abstractions for
 * tasks that can be done on both devices do not have the suffix and will
 * correctly look up the appropriate low level function when running.
 *
 * Functions which only make sense on a single device family may not have
 * a suitable generic implementation
 */

/**
 * ice_get_ptp_src_clock_index - determine source clock index
 * @hw: pointer to HW struct
 *
 * Determine the source clock index currently in use, based on device
 * capabilities reported during initialization.
 */
u8 ice_get_ptp_src_clock_index(struct ice_hw *hw)
{
	return hw->func_caps.ts_func_info.tmr_index_assoc;
}

/**
 * ice_ptp_read_src_incval - Read source timer increment value
 * @hw: pointer to HW struct
 *
 * Read the increment value of the source timer and return it.
 */
u64 ice_ptp_read_src_incval(struct ice_hw *hw)
{
	u32 lo, hi;
	u8 tmr_idx;

	tmr_idx = ice_get_ptp_src_clock_index(hw);

	lo = rd32(hw, GLTSYN_INCVAL_L(tmr_idx));
	hi = rd32(hw, GLTSYN_INCVAL_H(tmr_idx));

	return ((u64)(hi & INCVAL_HIGH_M) << 32) | lo;
}

#ifdef CGU_SUPPORT
/**
 * ice_read_cgu_reg_e822 - Read a CGU register
 * @hw: pointer to the HW struct
 * @addr: Register address to read
 * @val: storage for register value read
 *
 * Read the contents of a register of the Clock Generation Unit. Only
 * applicable to E822 devices.
 */
static enum ice_status
ice_read_cgu_reg_e822(struct ice_hw *hw, u16 addr, u32 *val)
{
	struct ice_sbq_msg_input cgu_msg;
	enum ice_status status;

	cgu_msg.opcode = ice_sbq_msg_rd;
	cgu_msg.dest_dev = cgu;
	cgu_msg.msg_addr_low = addr;
	cgu_msg.msg_addr_high = 0x0;

	status = ice_sbq_rw_reg_lp(hw, &cgu_msg, true);
	if (status) {
		ice_debug(hw, ICE_DBG_PTP, "Failed to read CGU register 0x%04x, status %d\n",
			  addr, status);
		return status;
	}

	*val = cgu_msg.data;

	return ICE_SUCCESS;
}

/**
 * ice_write_cgu_reg_e822 - Write a CGU register
 * @hw: pointer to the HW struct
 * @addr: Register address to write
 * @val: value to write into the register
 *
 * Write the specified value to a register of the Clock Generation Unit. Only
 * applicable to E822 devices.
 */
static enum ice_status
ice_write_cgu_reg_e822(struct ice_hw *hw, u16 addr, u32 val)
{
	struct ice_sbq_msg_input cgu_msg;
	enum ice_status status;

	cgu_msg.opcode = ice_sbq_msg_wr;
	cgu_msg.dest_dev = cgu;
	cgu_msg.msg_addr_low = addr;
	cgu_msg.msg_addr_high = 0x0;
	cgu_msg.data = val;

	status = ice_sbq_rw_reg_lp(hw, &cgu_msg, true);
	if (status) {
		ice_debug(hw, ICE_DBG_PTP, "Failed to write CGU register 0x%04x, status %d\n",
			  addr, status);
		return status;
	}

	return ICE_SUCCESS;
}

/**
 * ice_clk_freq_str - Convert time_ref_freq to string
 * @clk_freq: Clock frequency
 *
 * Convert the specified TIME_REF clock frequency to a string.
 */
const char *ice_clk_freq_str(u8 clk_freq)
{
	switch ((enum ice_time_ref_freq)clk_freq) {
	case ICE_TIME_REF_FREQ_25_000:
		return "25 MHz";
	case ICE_TIME_REF_FREQ_122_880:
		return "122.88 MHz";
	case ICE_TIME_REF_FREQ_125_000:
		return "125 MHz";
	case ICE_TIME_REF_FREQ_153_600:
		return "153.6 MHz";
	case ICE_TIME_REF_FREQ_156_250:
		return "156.25 MHz";
	case ICE_TIME_REF_FREQ_245_760:
		return "245.76 MHz";
	default:
		return "Unknown";
	}
}

/**
 * ice_clk_src_str - Convert time_ref_src to string
 * @clk_src: Clock source
 *
 * Convert the specified clock source to its string name.
 */
const char *ice_clk_src_str(u8 clk_src)
{
	switch ((enum ice_clk_src)clk_src) {
	case ICE_CLK_SRC_TCX0:
		return "TCX0";
	case ICE_CLK_SRC_TIME_REF:
		return "TIME_REF";
	default:
		return "Unknown";
	}
}

/**
 * ice_cfg_cgu_pll_e822 - Configure the Clock Generation Unit
 * @hw: pointer to the HW struct
 * @clk_freq: Clock frequency to program
 * @clk_src: Clock source to select (TIME_REF, or TCX0)
 *
 * Configure the Clock Generation Unit with the desired clock frequency and
 * time reference, enabling the PLL which drives the PTP hardware clock.
 */
enum ice_status
ice_cfg_cgu_pll_e822(struct ice_hw *hw, enum ice_time_ref_freq *clk_freq,
		     enum ice_clk_src *clk_src)
{
	union tspll_ro_bwm_lf bwm_lf;
	union nac_cgu_dword19 dw19;
	union nac_cgu_dword22 dw22;
	union nac_cgu_dword24 dw24;
	union nac_cgu_dword9 dw9;
	enum ice_status status;

	if (*clk_freq >= NUM_ICE_TIME_REF_FREQ) {
		ice_warn(hw, "Invalid TIME_REF frequency %u\n", *clk_freq);
		return ICE_ERR_PARAM;
	}

	if (*clk_src >= NUM_ICE_CLK_SRC) {
		ice_warn(hw, "Invalid clock source %u\n", *clk_src);
		return ICE_ERR_PARAM;
	}

	if (*clk_src == ICE_CLK_SRC_TCX0 &&
	    *clk_freq != ICE_TIME_REF_FREQ_25_000) {
		ice_warn(hw, "TCX0 only supports 25 MHz frequency\n");
		return ICE_ERR_PARAM;
	}

	status = ice_read_cgu_reg_e822(hw, NAC_CGU_DWORD9, &dw9.val);
	if (status)
		return status;

	status = ice_read_cgu_reg_e822(hw, NAC_CGU_DWORD24, &dw24.val);
	if (status)
		return status;

	status = ice_read_cgu_reg_e822(hw, TSPLL_RO_BWM_LF, &bwm_lf.val);
	if (status)
		return status;

	/* Log the current clock configuration */
	ice_debug(hw, ICE_DBG_PTP, "Current CGU configuration -- %s, clk_src %s, clk_freq %s, PLL %s\n",
		  dw24.field.ts_pll_enable ? "enabled" : "disabled",
		  ice_clk_src_str(dw24.field.time_ref_sel),
		  ice_clk_freq_str(dw9.field.time_ref_freq_sel),
		  bwm_lf.field.plllock_true_lock_cri ? "locked" : "unlocked");

	/* Disable the PLL before changing the clock source or frequency */
	if (dw24.field.ts_pll_enable) {
		dw24.field.ts_pll_enable = 0;

		status = ice_write_cgu_reg_e822(hw, NAC_CGU_DWORD24, dw24.val);
		if (status)
			return status;
	}

	/* Set the frequency */
	dw9.field.time_ref_freq_sel = *clk_freq;
	status = ice_write_cgu_reg_e822(hw, NAC_CGU_DWORD9, dw9.val);
	if (status)
		return status;

	/* Configure the TS PLL feedback divisor */
	status = ice_read_cgu_reg_e822(hw, NAC_CGU_DWORD19, &dw19.val);
	if (status)
		return status;

	dw19.field.tspll_fbdiv_intgr = e822_cgu_params[*clk_freq].feedback_div;
	dw19.field.tspll_ndivratio = 1;

	status = ice_write_cgu_reg_e822(hw, NAC_CGU_DWORD19, dw19.val);
	if (status)
		return status;

	/* Configure the TS PLL post divisor */
	status = ice_read_cgu_reg_e822(hw, NAC_CGU_DWORD22, &dw22.val);
	if (status)
		return status;

	dw22.field.time1588clk_div = e822_cgu_params[*clk_freq].post_pll_div;
	dw22.field.time1588clk_sel_div2 = 0;

	status = ice_write_cgu_reg_e822(hw, NAC_CGU_DWORD22, dw22.val);
	if (status)
		return status;

	/* Configure the TS PLL pre divisor and clock source */
	status = ice_read_cgu_reg_e822(hw, NAC_CGU_DWORD24, &dw24.val);
	if (status)
		return status;

	dw24.field.ref1588_ck_div = e822_cgu_params[*clk_freq].refclk_pre_div;
	dw24.field.tspll_fbdiv_frac = e822_cgu_params[*clk_freq].frac_n_div;
	dw24.field.time_ref_sel = *clk_src;

	status = ice_write_cgu_reg_e822(hw, NAC_CGU_DWORD24, dw24.val);
	if (status)
		return status;

	/* Finally, enable the PLL */
	dw24.field.ts_pll_enable = 1;

	status = ice_write_cgu_reg_e822(hw, NAC_CGU_DWORD24, dw24.val);
	if (status)
		return status;

	/* Wait to verify if the PLL locks */
	ice_msec_delay(1, true);

	status = ice_read_cgu_reg_e822(hw, TSPLL_RO_BWM_LF, &bwm_lf.val);
	if (status)
		return status;

	if (!bwm_lf.field.plllock_true_lock_cri) {
		ice_warn(hw, "CGU PLL failed to lock\n");
		return ICE_ERR_NOT_READY;
	}

	/* Log the current clock configuration */
	ice_debug(hw, ICE_DBG_PTP, "New CGU configuration -- %s, clk_src %s, clk_freq %s, PLL %s\n",
		  dw24.field.ts_pll_enable ? "enabled" : "disabled",
		  ice_clk_src_str(dw24.field.time_ref_sel),
		  ice_clk_freq_str(dw9.field.time_ref_freq_sel),
		  bwm_lf.field.plllock_true_lock_cri ? "locked" : "unlocked");

	*clk_freq = (enum ice_time_ref_freq)dw9.field.time_ref_freq_sel;
	*clk_src = (enum ice_clk_src)dw24.field.time_ref_sel;

	return ICE_SUCCESS;
}

/**
 * ice_init_cgu_e822 - Initialize CGU with settings from firmware
 * @hw: pointer to the HW structure
 *
 * Initialize the Clock Generation Unit of the E822 device.
 */
static enum ice_status ice_init_cgu_e822(struct ice_hw *hw)
{
	struct ice_ts_func_info *ts_info = &hw->func_caps.ts_func_info;
	union tspll_cntr_bist_settings cntr_bist;
	enum ice_time_ref_freq time_ref_freq;
	enum ice_clk_src clk_src;
	enum ice_status status;

	status = ice_read_cgu_reg_e822(hw, TSPLL_CNTR_BIST_SETTINGS,
				       &cntr_bist.val);
	if (status)
		return status;

	/* Disable sticky lock detection so lock status reported is accurate */
	cntr_bist.field.i_plllock_sel_0 = 0;
	cntr_bist.field.i_plllock_sel_1 = 0;

	status = ice_write_cgu_reg_e822(hw, TSPLL_CNTR_BIST_SETTINGS,
					cntr_bist.val);
	if (status)
		return status;

	/* Configure the CGU PLL using the parameters from the function
	 * capabilities.
	 */
	time_ref_freq = (enum ice_time_ref_freq)ts_info->time_ref;
	clk_src = (enum ice_clk_src)ts_info->clk_src;
	status = ice_cfg_cgu_pll_e822(hw, &time_ref_freq, &clk_src);
	if (status)
		return status;

	return ICE_SUCCESS;
}

#endif /* CGU_SUPPORT */
#if defined(CGU_SUPPORT) || defined(SYNCE_SUPPORT)
/**
 * ice_ptp_cgu_err_reporting - Enable/disable error reporting for CGU
 * @hw: pointer to HW struct
 * @enable: true if reporting should be enabled
 *
 * Enable or disable error events to be reported through Admin Queue.
 *
 * Return: 0 on success, error code otherwise
 */
static enum ice_status ice_ptp_cgu_err_reporting(struct ice_hw *hw, bool enable)
{
	enum ice_status status;

	status = ice_aq_cfg_cgu_err(hw, enable, enable, NULL);
	if (status) {
		ice_debug(hw, ICE_DBG_PTP,
			  "Failed to %s CGU error reporting, status %d\n",
			  enable ? "enable" : "disable", status);
		return status;
	}

	return ICE_SUCCESS;
}

/**
 * ice_ptp_process_cgu_err - Handle reported CGU error
 * @hw: pointer to HW struct
 * @event: reported CGU error descriptor
 */
void ice_ptp_process_cgu_err(struct ice_hw *hw, struct ice_rq_event_info *event)
{
	u8 err_type = event->desc.params.cgu_err.err_type;

#ifdef SYNCE_SUPPORT
	if (err_type & ICE_AQC_CGU_ERR_SYNCE_LOCK_LOSS)
		ice_warn(hw, "SyncE lock lost\n");

	if (err_type & ICE_AQC_CGU_ERR_HOLDOVER_CHNG)
		ice_warn(hw, "SyncE holdover change\n");
#endif /* SYNCE_SUPPORT */
#ifdef CGU_SUPPORT
	if (err_type & ICE_AQC_CGU_ERR_TIMESYNC_LOCK_LOSS) {
		ice_warn(hw, "TimeSync PLL lock lost. Retrying to acquire lock with default PLL configuration.\n");
		ice_init_cgu_e822(hw);
	}
#endif /* CGU_SUPPORT */

	/* Reenable CGU error reporting */
	ice_ptp_cgu_err_reporting(hw, true);
}

#endif /* CGU_SUPPORT || SYNCE_SUPPORT */

/**
 * ice_ptp_src_cmd - Prepare source timer for a timer command
 * @hw: pointer to HW structure
 * @cmd: Timer command
 *
 * Prepare the source timer for an upcoming timer sync command.
 */
void ice_ptp_src_cmd(struct ice_hw *hw, enum ice_ptp_tmr_cmd cmd)
{
	u32 cmd_val;
	u8 tmr_idx;

	tmr_idx = ice_get_ptp_src_clock_index(hw);
	cmd_val = tmr_idx << SEL_CPK_SRC;

	switch (cmd) {
	case ICE_PTP_INIT_TIME:
		cmd_val |= GLTSYN_CMD_INIT_TIME;
		break;
	case ICE_PTP_INIT_INCVAL:
		cmd_val |= GLTSYN_CMD_INIT_INCVAL;
		break;
	case ICE_PTP_ADJ_TIME:
		cmd_val |= GLTSYN_CMD_ADJ_TIME;
		break;
	case ICE_PTP_ADJ_TIME_AT_TIME:
		cmd_val |= GLTSYN_CMD_ADJ_INIT_TIME;
		break;
	case ICE_PTP_READ_TIME:
		cmd_val |= GLTSYN_CMD_READ_TIME;
		break;
	case ICE_PTP_NOP:
		break;
	default:
		ice_warn(hw, "Unknown timer command %u\n", cmd);
		return;
	}

	wr32(hw, GLTSYN_CMD, cmd_val);
}

/**
 * ice_ptp_exec_tmr_cmd - Execute all prepared timer commands
 * @hw: pointer to HW struct
 *
 * Write the SYNC_EXEC_CMD bit to the GLTSYN_CMD_SYNC register, and flush the
 * write immediately. This triggers the hardware to begin executing all of the
 * source and PHY timer commands synchronously.
 */
static void ice_ptp_exec_tmr_cmd(struct ice_hw *hw)
{
	wr32(hw, GLTSYN_CMD_SYNC, SYNC_EXEC_CMD);
	ice_flush(hw);
}

/**
 * ice_ptp_clean_cmd - Clean the timer command register
 * @hw: pointer to HW struct
 *
 * Zero out the GLTSYN_CMD to avoid any residual command execution.
 */
static void ice_ptp_clean_cmd(struct ice_hw *hw)
{
	wr32(hw, GLTSYN_CMD, 0);
	ice_flush(hw);
}

#ifdef ETH56G_SUPPORT
enum eth56g_res_type {
	ETH56G_PHY_REG,
	ETH56G_PHY_MEM,
};

/* 56G PHY access functions */
static const u32 ice_eth56g_port_base[ICE_NUM_PHY_PORTS] = {
	ICE_PHY0_BASE,
	ICE_PHY1_BASE,
	ICE_PHY2_BASE,
	ICE_PHY3_BASE,
	ICE_PHY4_BASE,
};

/**
 * ice_write_phy_eth56g_raw_lp - Write a PHY port register with lock parameter
 * @hw: pointer to the HW struct
 * @phy_index: PHY index
 * @reg_addr: PHY register address
 * @val: Value to write
 * @lock_sbq: true to lock the sideband queue
 */
static enum ice_status
ice_write_phy_eth56g_raw_lp(struct ice_hw *hw, u8 phy_index, u32 reg_addr,
			    u32 val, bool lock_sbq)
{
	struct ice_sbq_msg_input phy_msg;
	enum ice_status status;

	phy_msg.opcode = ice_sbq_msg_wr;

	phy_msg.msg_addr_low = ICE_LO_WORD(reg_addr);
	phy_msg.msg_addr_high = ICE_HI_WORD(reg_addr);

	phy_msg.data = val;
	phy_msg.dest_dev = hw->phy_addr[phy_index];

	status = ice_sbq_rw_reg_lp(hw, &phy_msg, lock_sbq);

	if (status)
		ice_debug(hw, ICE_DBG_PTP, "PTP failed to send msg to phy %d\n",
			  status);

	return status;
}

/**
 * ice_read_phy_eth56g_raw_lp - Read a PHY port register with lock parameter
 * @hw: pointer to the HW struct
 * @phy_index: PHY index
 * @reg_addr: PHY register address
 * @val: Value to write
 * @lock_sbq: true to lock the sideband queue
 */
static enum ice_status
ice_read_phy_eth56g_raw_lp(struct ice_hw *hw, u8 phy_index, u32 reg_addr,
			   u32 *val, bool lock_sbq)
{
	struct ice_sbq_msg_input phy_msg;
	enum ice_status status;

	phy_msg.opcode = ice_sbq_msg_rd;

	phy_msg.msg_addr_low = ICE_LO_WORD(reg_addr);
	phy_msg.msg_addr_high = ICE_HI_WORD(reg_addr);

	phy_msg.data = 0;
	phy_msg.dest_dev = hw->phy_addr[phy_index];

	status = ice_sbq_rw_reg_lp(hw, &phy_msg, lock_sbq);

	if (status)
		ice_debug(hw, ICE_DBG_PTP, "PTP failed to send msg to phy %d\n",
			  status);
	else
		*val = phy_msg.data;

	return status;
}

/**
 * ice_phy_port_res_address_eth56g - Calculate a PHY port register address
 * @port: Port number to be written
 * @res_type: resource type (register/memory)
 * @offset: Offset from PHY port register base
 *
 * @address: The result address
 */
static enum ice_status
ice_phy_port_res_address_eth56g(u8 port, enum eth56g_res_type res_type,
				u16 offset, u32 *address)
{
	u8 phy, lane;

	phy = port / ICE_PORTS_PER_QUAD;
	lane = port % ICE_PORTS_PER_QUAD;

	switch (res_type) {
	case ETH56G_PHY_REG:
		*address = offset + ice_eth56g_port_base[phy] +
			   PHY_PTP_LANE_ADDR_STEP * lane;
		break;
	case ETH56G_PHY_MEM:
		*address = offset + ice_eth56g_port_base[phy] +
			   PHY_PTP_MEM_START + PHY_PTP_MEM_LANE_STEP * lane;
		break;
	default:
		return ICE_ERR_PARAM;
	}

	return ICE_SUCCESS;
}

/**
 * ice_write_phy_port_eth56g_lp - Write a PHY port register with lock parameter
 * @hw: pointer to the HW struct
 * @reg_offs: PHY register offset
 * @port: Port number
 * @val: Value to write
 * @res_type: resource type (register/memory)
 * @lock_sbq: true to lock the sideband queue
 */
static enum ice_status
ice_write_phy_port_eth56g_lp(struct ice_hw *hw, u8 port, u32 reg_offs, u32 val,
			     enum eth56g_res_type res_type, bool lock_sbq)
{
	u8 phy_index = port / hw->phy_ports;
	u8 phy_port = port % hw->phy_ports;
	enum ice_status status;
	u32 reg_addr;

	if (port >= hw->phy_ports)
		return ICE_ERR_OUT_OF_RANGE;

	status = ice_phy_port_res_address_eth56g(phy_port, res_type, reg_offs,
						 &reg_addr);

	if (status)
		return status;

	return ice_write_phy_eth56g_raw_lp(hw, phy_index, reg_addr, val,
					   lock_sbq);
}

/**
 * ice_read_phy_port_eth56g_lp - Read a PHY port register with lock parameter
 * @hw: pointer to the HW struct
 * @reg_offs: PHY register offset
 * @port: Port number
 * @val: Value to write
 * @res_type: resource type (register/memory)
 * @lock_sbq: true to lock the sideband queue
 */
static enum ice_status
ice_read_phy_port_eth56g_lp(struct ice_hw *hw, u8 port, u32 reg_offs, u32 *val,
			    enum eth56g_res_type res_type, bool lock_sbq)
{
	u8 phy_index = port / hw->phy_ports;
	u8 phy_port = port % hw->phy_ports;
	enum ice_status status;
	u32 reg_addr;

	if (port >= hw->phy_ports)
		return ICE_ERR_OUT_OF_RANGE;

	status = ice_phy_port_res_address_eth56g(phy_port, res_type, reg_offs,
						 &reg_addr);

	if (status)
		return status;

	return ice_read_phy_eth56g_raw_lp(hw, phy_index, reg_addr, val,
					  lock_sbq);
}

/**
 * ice_write_phy_reg_eth56g_lp - Write a PHY port register with lock parameter
 * @hw: pointer to the HW struct
 * @port: Port number to be written
 * @offset: Offset from PHY port register base
 * @val: Value to write
 * @lock_sbq: true to lock the sideband queue
 */
static enum ice_status
ice_write_phy_reg_eth56g_lp(struct ice_hw *hw, u8 port, u16 offset, u32 val,
			    bool lock_sbq)
{
	return ice_write_phy_port_eth56g_lp(hw, port, offset, val,
					    ETH56G_PHY_REG, lock_sbq);
}

/**
 * ice_write_phy_reg_eth56g - Write a PHY port register with sbq locked
 * @hw: pointer to the HW struct
 * @port: Port number to be written
 * @offset: Offset from PHY port register base
 * @val: Value to write
 */
enum ice_status
ice_write_phy_reg_eth56g(struct ice_hw *hw, u8 port, u16 offset, u32 val)
{
	return ice_write_phy_reg_eth56g_lp(hw, port, offset, val, true);
}

/**
 * ice_read_phy_reg_eth56g_lp - Read a PHY port register with lock parameter
 * @hw: pointer to the HW struct
 * @port: Port number to be read
 * @offset: Offset from PHY port register base
 * @val: Pointer to the value to read (out param)
 * @lock_sbq: true to lock the sideband queue
 */
static enum ice_status
ice_read_phy_reg_eth56g_lp(struct ice_hw *hw, u8 port, u16 offset, u32 *val,
			   bool lock_sbq)
{
	return ice_read_phy_port_eth56g_lp(hw, port, offset, val,
					   ETH56G_PHY_REG, lock_sbq);
}

/**
 * ice_read_phy_reg_eth56g - Read a PHY port register with sbq locked
 * @hw: pointer to the HW struct
 * @port: Port number to be read
 * @offset: Offset from PHY port register base
 * @val: Pointer to the value to read (out param)
 */
enum ice_status
ice_read_phy_reg_eth56g(struct ice_hw *hw, u8 port, u16 offset, u32 *val)
{
	return ice_read_phy_reg_eth56g_lp(hw, port, offset, val, true);
}

/**
 * ice_phy_port_mem_read_eth56g_lp - Read a PHY port memory location
 * @hw: pointer to the HW struct
 * @port: Port number to be read
 * @offset: Offset from PHY port register base
 * @val: Pointer to the value to read (out param)
 * @lock_sbq: true to lock the sideband queue
 */
static enum ice_status
ice_phy_port_mem_read_eth56g_lp(struct ice_hw *hw, u8 port, u16 offset,
				u32 *val, bool lock_sbq)
{
	return ice_read_phy_port_eth56g_lp(hw, port, offset, val,
					   ETH56G_PHY_MEM, lock_sbq);
}

/**
 * ice_phy_port_mem_read_eth56g - Read a PHY port memory location
 * @hw: pointer to the HW struct
 * @port: Port number to be read
 * @offset: Offset from PHY port register base
 * @val: Pointer to the value to read (out param)
 *
 * Takes the sideband queue lock.
 */
static enum ice_status
ice_phy_port_mem_read_eth56g(struct ice_hw *hw, u8 port, u16 offset, u32 *val)
{
	return ice_phy_port_mem_read_eth56g_lp(hw, port, offset, val, true);
}

/**
 * ice_phy_port_mem_write_eth56g_lp - Write a PHY port memory location
 * @hw: pointer to the HW struct
 * @port: Port number to be read
 * @offset: Offset from PHY port register base
 * @val: Pointer to the value to read (out param)
 * @lock_sbq: true to lock the sideband queue
 */
static enum ice_status
ice_phy_port_mem_write_eth56g_lp(struct ice_hw *hw, u8 port, u16 offset,
				 u32 val, bool lock_sbq)
{
	return ice_write_phy_port_eth56g_lp(hw, port, offset, val,
					    ETH56G_PHY_MEM, lock_sbq);
}

/**
 * ice_phy_port_mem_write_eth56g - Write a PHY port memory location
 * @hw: pointer to the HW struct
 * @port: Port number to be read
 * @offset: Offset from PHY port register base
 * @val: Pointer to the value to read (out param)
 *
 * Takes the sideband queue lock.
 */
static enum ice_status
ice_phy_port_mem_write_eth56g(struct ice_hw *hw, u8 port, u16 offset, u32 val)
{
	return ice_phy_port_mem_write_eth56g_lp(hw, port, offset, val, true);
}

/**
 * ice_is_64b_phy_reg_eth56g - Check if this is a 64bit PHY register
 * @low_addr: the low address to check
 * @high_addr: on return, contains the high address of the 64bit register
 *
 * Checks if the provided low address is one of the known 64bit PHY values
 * represented as two 32bit registers. If it is, return the appropriate high
 * register offset to use.
 */
static bool ice_is_64b_phy_reg_eth56g(u16 low_addr, u16 *high_addr)
{
	switch (low_addr) {
	case PHY_REG_TX_TIMER_INC_PRE_L:
		*high_addr = PHY_REG_TX_TIMER_INC_PRE_U;
		return true;
	case PHY_REG_RX_TIMER_INC_PRE_L:
		*high_addr = PHY_REG_RX_TIMER_INC_PRE_U;
		return true;
	case PHY_REG_TX_CAPTURE_L:
		*high_addr = PHY_REG_TX_CAPTURE_U;
		return true;
	case PHY_REG_RX_CAPTURE_L:
		*high_addr = PHY_REG_RX_CAPTURE_U;
		return true;
	case PHY_REG_TOTAL_TX_OFFSET_L:
		*high_addr = PHY_REG_TOTAL_TX_OFFSET_U;
		return true;
	case PHY_REG_TOTAL_RX_OFFSET_L:
		*high_addr = PHY_REG_TOTAL_RX_OFFSET_U;
		return true;
	case PHY_REG_TX_MEMORY_STATUS_L:
		*high_addr = PHY_REG_TX_MEMORY_STATUS_U;
		return true;
	default:
		return false;
	}
}

/**
 * ice_is_40b_phy_reg_eth56g - Check if this is a 40bit PHY register
 * @low_addr: the low address to check
 * @high_addr: on return, contains the high address of the 40bit value
 *
 * Checks if the provided low address is one of the known 40bit PHY values
 * split into two registers with the lower 8 bits in the low register and the
 * upper 32 bits in the high register. If it is, return the high register
 * offset to use.
 */
static bool ice_is_40b_phy_reg_eth56g(u16 low_addr, u16 *high_addr)
{
	switch (low_addr) {
	case PHY_REG_TIMETUS_L:
		*high_addr = PHY_REG_TIMETUS_U;
		return true;
	default:
		return false;
	}
}

/**
 * ice_read_40b_phy_reg_eth56g - Read a 40bit value from PHY registers
 * @hw: pointer to the HW struct
 * @port: PHY port to read from
 * @low_addr: offset of the lower register to read from
 * @val: on return, the contents of the 40bit value from the PHY registers
 *
 * Reads the two registers associated with a 40bit value and returns it in the
 * val pointer.
 * This function checks that the caller has specified a known 40 bit register
 * offset
 */
static enum ice_status
ice_read_40b_phy_reg_eth56g(struct ice_hw *hw, u8 port, u16 low_addr, u64 *val)
{
	enum ice_status status;
	u16 high_addr;
	u32 lo, hi;

	if (!ice_is_40b_phy_reg_eth56g(low_addr, &high_addr))
		return ICE_ERR_PARAM;

	status = ice_read_phy_reg_eth56g(hw, port, low_addr, &lo);
	if (status) {
		ice_debug(hw, ICE_DBG_PTP, "Failed to read from low register %#08x\n, status %d",
			  (int)low_addr, status);
		return status;
	}

	status = ice_read_phy_reg_eth56g(hw, port, low_addr + sizeof(u32), &hi);
	if (status) {
		ice_debug(hw, ICE_DBG_PTP, "Failed to read from high register %08x\n, status %d",
			  high_addr, status);
		return status;
	}

	*val = ((u64)hi << P_REG_40B_HIGH_S) | (lo & P_REG_40B_LOW_M);

	return ICE_SUCCESS;
}

/**
 * ice_read_64b_phy_reg_eth56g - Read a 64bit value from PHY registers
 * @hw: pointer to the HW struct
 * @port: PHY port to read from
 * @low_addr: offset of the lower register to read from
 * @val: on return, the contents of the 64bit value from the PHY registers
 *
 * Reads the two registers associated with a 64bit value and returns it in the
 * val pointer.
 * This function checks that the caller has specified a known 64 bit register
 * offset
 */
static enum ice_status
ice_read_64b_phy_reg_eth56g(struct ice_hw *hw, u8 port, u16 low_addr, u64 *val)
{
	enum ice_status status;
	u16 high_addr;
	u32 lo, hi;

	if (!ice_is_64b_phy_reg_eth56g(low_addr, &high_addr))
		return ICE_ERR_PARAM;

	status = ice_read_phy_reg_eth56g(hw, port, low_addr, &lo);
	if (status) {
		ice_debug(hw, ICE_DBG_PTP, "Failed to read from low register %#08x\n, status %d",
			  low_addr, status);
		return status;
	}

	status = ice_read_phy_reg_eth56g(hw, port, high_addr, &hi);
	if (status) {
		ice_debug(hw, ICE_DBG_PTP, "Failed to read from high register %#08x\n, status %d",
			  high_addr, status);
		return status;
	}

	*val = ((u64)hi << 32) | lo;

	return ICE_SUCCESS;
}

/**
 * ice_write_40b_phy_reg_eth56g - Write a 40b value to the PHY
 * @hw: pointer to the HW struct
 * @port: port to write to
 * @low_addr: offset of the low register
 * @val: 40b value to write
 *
 * Write the provided 40b value to the two associated registers by splitting
 * it up into two chunks, the lower 8 bits and the upper 32 bits.
 * This function checks that the caller has specified a known 40 bit register
 * offset
 */
static enum ice_status
ice_write_40b_phy_reg_eth56g(struct ice_hw *hw, u8 port, u16 low_addr, u64 val)
{
	enum ice_status status;
	u16 high_addr;
	u32 lo, hi;

	if (!ice_is_40b_phy_reg_eth56g(low_addr, &high_addr))
		return ICE_ERR_PARAM;

	lo = (u32)(val & P_REG_40B_LOW_M);
	hi = (u32)(val >> P_REG_40B_HIGH_S);

	status = ice_write_phy_reg_eth56g(hw, port, low_addr, lo);
	if (status) {
		ice_debug(hw, ICE_DBG_PTP, "Failed to write to low register 0x%08x\n, status %d",
			  low_addr, status);
		return status;
	}

	status = ice_write_phy_reg_eth56g(hw, port, high_addr, hi);
	if (status) {
		ice_debug(hw, ICE_DBG_PTP, "Failed to write to high register 0x%08x\n, status %d",
			  high_addr, status);
		return status;
	}

	return ICE_SUCCESS;
}

/**
 * ice_write_64b_phy_reg_eth56g - Write a 64bit value to PHY registers
 * @hw: pointer to the HW struct
 * @port: PHY port to read from
 * @low_addr: offset of the lower register to read from
 * @val: the contents of the 64bit value to write to PHY
 *
 * Write the 64bit value to the two associated 32bit PHY registers.
 * This function checks that the caller has specified a known 64 bit register
 * offset
 */
static enum ice_status
ice_write_64b_phy_reg_eth56g(struct ice_hw *hw, u8 port, u16 low_addr, u64 val)
{
	enum ice_status status;
	u16 high_addr;
	u32 lo, hi;

	if (!ice_is_64b_phy_reg_eth56g(low_addr, &high_addr))
		return ICE_ERR_PARAM;

	lo = ICE_LO_DWORD(val);
	hi = ICE_HI_DWORD(val);

	status = ice_write_phy_reg_eth56g(hw, port, low_addr, lo);
	if (status) {
		ice_debug(hw, ICE_DBG_PTP, "Failed to write to low register 0x%08x\n, status %d",
			  low_addr, status);
		return status;
	}

	status = ice_write_phy_reg_eth56g(hw, port, high_addr, hi);
	if (status) {
		ice_debug(hw, ICE_DBG_PTP, "Failed to write to high register 0x%08x\n, status %d",
			  high_addr, status);
		return status;
	}

	return ICE_SUCCESS;
}

/**
 * ice_read_phy_tstamp_eth56g - Read a PHY timestamp out of the port memory
 * @hw: pointer to the HW struct
 * @port: the port to read from
 * @idx: the timestamp index to read
 * @tstamp: on return, the 40bit timestamp value
 *
 * Read a 40bit timestamp value out of the two associated entries in the
 * port memory block of the internal PHYs of the 56G devices.
 */
static enum ice_status
ice_read_phy_tstamp_eth56g(struct ice_hw *hw, u8 port, u8 idx, u64 *tstamp)
{
	enum ice_status status;
	u16 lo_addr, hi_addr;
	u32 lo, hi;

	lo_addr = (u16)PHY_TSTAMP_L(idx);
	hi_addr = (u16)PHY_TSTAMP_U(idx);

	status = ice_phy_port_mem_read_eth56g(hw, port, lo_addr, &lo);
	if (status) {
		ice_debug(hw, ICE_DBG_PTP, "Failed to read low PTP timestamp register, status %d\n",
			  status);
		return status;
	}

	status = ice_phy_port_mem_read_eth56g(hw, port, hi_addr, &hi);
	if (status) {
		ice_debug(hw, ICE_DBG_PTP, "Failed to read high PTP timestamp register, status %d\n",
			  status);
		return status;
	}

	/* For 56G based internal PHYs, the timestamp is reported with the
	 * lower 8 bits in the low register, and the upper 32 bits in the high
	 * register.
	 */
	*tstamp = ((u64)hi) << TS_PHY_HIGH_S | ((u64)lo & TS_PHY_LOW_M);

	return ICE_SUCCESS;
}

/**
 * ice_clear_phy_tstamp_eth56g - Clear a timestamp from the quad block
 * @hw: pointer to the HW struct
 * @port: the quad to read from
 * @idx: the timestamp index to reset
 *
 * Read and then forcibly clear the timestamp index to ensure the valid bit is
 * cleared and the timestamp status bit is reset in the PHY port memory of
 * internal PHYs of the 56G devices.
 *
 * This function should only be called on an idx whose bit is set according to
 * ice_get_phy_tx_tstamp_ready.
 */
static enum ice_status
ice_clear_phy_tstamp_eth56g(struct ice_hw *hw, u8 port, u8 idx)
{
	enum ice_status status;
	u64 unused_tstamp;
	u16 lo_addr;

	/* Read the timestamp register to ensure the timestamp status bit is
	 * cleared.
	 */
	status = ice_read_phy_tstamp_eth56g(hw, port, idx, &unused_tstamp);
	if (status) {
		ice_debug(hw, ICE_DBG_PTP, "Failed to read the PHY timestamp register for port %u, idx %u, status %d\n",
			  port, idx, status);
	}

	lo_addr = (u16)PHY_TSTAMP_L(idx);

	status = ice_phy_port_mem_write_eth56g(hw, port, lo_addr, 0);
	if (status) {
		ice_debug(hw, ICE_DBG_PTP, "Failed to clear low PTP timestamp register for port %u, idx %u, status %d\n",
			  port, idx, status);
		return status;
	}

	return ICE_SUCCESS;
}

/**
 * ice_ptp_reset_ts_memory_eth56g - Clear all timestamps from the port block
 * @hw: pointer to the HW struct
 */
static void ice_ptp_reset_ts_memory_eth56g(struct ice_hw *hw)
{
	unsigned int port;

	for (port = 0; port < ICE_NUM_EXTERNAL_PORTS; port++) {
		ice_write_phy_reg_eth56g(hw, port, PHY_REG_TX_MEMORY_STATUS_L,
					 0);
		ice_write_phy_reg_eth56g(hw, port, PHY_REG_TX_MEMORY_STATUS_U,
					 0);
	}
}

/**
 * ice_ptp_prep_port_phy_time_eth56g - Prepare one PHY port with initial time
 * @hw: pointer to the HW struct
 * @port: port number
 * @phy_time: time to initialize the PHY port clocks to
 *
 * Write a new initial time value into registers of a specific PHY port.
 */
static enum ice_status
ice_ptp_prep_port_phy_time_eth56g(struct ice_hw *hw, u8 port, u64 phy_time)
{
	enum ice_status status;

	/* Tx case */
	status = ice_write_64b_phy_reg_eth56g(hw, port,
					      PHY_REG_TX_TIMER_INC_PRE_L,
					      phy_time);
	if (status)
		return status;

	/* Rx case */
	return ice_write_64b_phy_reg_eth56g(hw, port,
					    PHY_REG_RX_TIMER_INC_PRE_L,
					    phy_time);
}

/**
 * ice_ptp_prep_phy_time_eth56g - Prepare PHY port with initial time
 * @hw: pointer to the HW struct
 * @time: Time to initialize the PHY port clocks to
 *
 * Program the PHY port registers with a new initial time value. The port
 * clock will be initialized once the driver issues an ICE_PTP_INIT_TIME sync
 * command. The time value is the upper 32 bits of the PHY timer, usually in
 * units of nominal nanoseconds.
 */
static enum ice_status
ice_ptp_prep_phy_time_eth56g(struct ice_hw *hw, u32 time)
{
	u64 phy_time;
	u8 port;

	/* The time represents the upper 32 bits of the PHY timer, so we need
	 * to shift to account for this when programming.
	 */
	phy_time = (u64)time << 32;

	for (port = 0; port < ICE_NUM_EXTERNAL_PORTS; port++) {
		enum ice_status status;

		status = ice_ptp_prep_port_phy_time_eth56g(hw, port,
							   phy_time);

		if (status) {
			ice_debug(hw, ICE_DBG_PTP, "Failed to write init time for port %u, status %d\n",
				  port, status);
			return status;
		}
	}

	return ICE_SUCCESS;
}

#ifdef SIMICS_SUPPORT
/*
 * ice_ptp_prep_port_adj_eth56g - Prepare a single port for time adjust
 * @hw: pointer to HW struct
 * @port: Port number to be programmed
 * @time_rx: time in cycles to adjust the port Rx clocks
 * @time_tx: time in cycles to adjust the port Tx clocks
 * @lock_sbq: true to lock the sbq sq_lock (the usual case); false if the
 *            sq_lock has already been locked at a higher level
 *
 * Program the port for an atomic adjustment by writing the Tx and Rx timer
 * registers. The atomic adjustment won't be completed until the driver issues
 * an ICE_PTP_ADJ_TIME command.
 *
 * Note that time is not in units of nanoseconds. It is in clock time
 * including the lower sub-nanosecond portion of the port timer.
 *
 * Negative adjustments are supported using 2s complement arithmetic.
 */
enum ice_status
ice_ptp_prep_port_adj_eth56g(struct ice_hw *hw, u8 port, s64 time_rx,
			     s64 time_tx, bool lock_sbq)
#else /* SIMICS_SUPPORT */
/**
 * ice_ptp_prep_port_adj_eth56g - Prepare a single port for time adjust
 * @hw: pointer to HW struct
 * @port: Port number to be programmed
 * @time: time in cycles to adjust the port clocks
 * @lock_sbq: true to lock the sbq sq_lock (the usual case); false if the
 *            sq_lock has already been locked at a higher level
 *
 * Program the port for an atomic adjustment by writing the Tx and Rx timer
 * registers. The atomic adjustment won't be completed until the driver issues
 * an ICE_PTP_ADJ_TIME command.
 *
 * Note that time is not in units of nanoseconds. It is in clock time
 * including the lower sub-nanosecond portion of the port timer.
 *
 * Negative adjustments are supported using 2s complement arithmetic.
 */
enum ice_status
ice_ptp_prep_port_adj_eth56g(struct ice_hw *hw, u8 port, s64 time,
			     bool lock_sbq)
#endif /* SIMICS_SUPPORT */
{
	enum ice_status status;
	u32 l_time, u_time;

#ifdef SIMICS_SUPPORT
	l_time = ICE_LO_DWORD(time_tx);
	u_time = ICE_HI_DWORD(time_tx);
#else /* SIMICS_SUPPORT */
	l_time = ICE_LO_DWORD(time);
	u_time = ICE_HI_DWORD(time);
#endif /* SIMICS_SUPPORT */

	/* Tx case */
	status = ice_write_phy_reg_eth56g_lp(hw, port,
					     PHY_REG_TX_TIMER_INC_PRE_L,
					     l_time, lock_sbq);
	if (status)
		goto exit_err;

	status = ice_write_phy_reg_eth56g_lp(hw, port,
					     PHY_REG_TX_TIMER_INC_PRE_U,
					     u_time, lock_sbq);
	if (status)
		goto exit_err;

#ifdef SIMICS_SUPPORT
	l_time = ICE_LO_DWORD(time_rx);
	u_time = ICE_HI_DWORD(time_rx);
#endif /* SIMICS_SUPPORT */

	/* Rx case */
	status = ice_write_phy_reg_eth56g_lp(hw, port,
					     PHY_REG_RX_TIMER_INC_PRE_L,
					     l_time, lock_sbq);
	if (status)
		goto exit_err;

	status = ice_write_phy_reg_eth56g_lp(hw, port,
					     PHY_REG_RX_TIMER_INC_PRE_U,
					     u_time, lock_sbq);
	if (status)
		goto exit_err;

	return ICE_SUCCESS;

exit_err:
	ice_debug(hw, ICE_DBG_PTP, "Failed to write time adjust for port %u, status %d\n",
		  port, status);
	return status;
}

/**
 * ice_ptp_prep_phy_adj_eth56g - Prep PHY ports for a time adjustment
 * @hw: pointer to HW struct
 * @adj: adjustment in nanoseconds
 * @lock_sbq: true to lock the sbq sq_lock (the usual case); false if the
 *            sq_lock has already been locked at a higher level
 *
 * Prepare the PHY ports for an atomic time adjustment by programming the PHY
 * Tx and Rx port registers. The actual adjustment is completed by issuing an
 * ICE_PTP_ADJ_TIME or ICE_PTP_ADJ_TIME_AT_TIME sync command.
 */
static enum ice_status
ice_ptp_prep_phy_adj_eth56g(struct ice_hw *hw, s32 adj, bool lock_sbq)
{
	enum ice_status status = ICE_SUCCESS;
	s64 cycles;
	u8 port;

	/* The port clock supports adjustment of the sub-nanosecond portion of
	 * the clock. We shift the provided adjustment in nanoseconds to
	 * calculate the appropriate adjustment to program into the PHY ports.
	 */
	cycles = (s64)adj << 32;

	for (port = 0; port < ICE_NUM_EXTERNAL_PORTS; port++) {

#ifdef SIMICS_SUPPORT
		status = ice_ptp_prep_port_adj_eth56g(hw, port, cycles,
						      cycles, lock_sbq);
#else /* SIMICS_SUPPORT */
		status = ice_ptp_prep_port_adj_eth56g(hw, port, cycles,
						      lock_sbq);
#endif /* SIMICS_SUPPORT */
		if (status)
			break;
	}

	return status;
}

/**
 * ice_ptp_prep_phy_incval_eth56g - Prepare PHY ports for time adjustment
 * @hw: pointer to HW struct
 * @incval: new increment value to prepare
 *
 * Prepare each of the PHY ports for a new increment value by programming the
 * port's TIMETUS registers. The new increment value will be updated after
 * issuing an ICE_PTP_INIT_INCVAL command.
 */
static enum ice_status
ice_ptp_prep_phy_incval_eth56g(struct ice_hw *hw, u64 incval)
{
	u8 port;

	for (port = 0; port < ICE_NUM_EXTERNAL_PORTS; port++) {
		enum ice_status status;
		status = ice_write_40b_phy_reg_eth56g(hw, port,
						      PHY_REG_TIMETUS_L,
						      incval);
		if (status) {
			ice_debug(hw, ICE_DBG_PTP, "Failed to write incval for port %u, status %d\n",
				  port, status);
			return status;
		}
	}

	return ICE_SUCCESS;
}

/**
 * ice_ptp_read_phy_incval_eth56g - Read a PHY port's current incval
 * @hw: pointer to the HW struct
 * @port: the port to read
 * @incval: on return, the time_clk_cyc incval for this port
 *
 * Read the time_clk_cyc increment value for a given PHY port.
 */
enum ice_status
ice_ptp_read_phy_incval_eth56g(struct ice_hw *hw, u8 port, u64 *incval)
{
	enum ice_status status;

	status = ice_read_40b_phy_reg_eth56g(hw, port, PHY_REG_TIMETUS_L,
					     incval);
	if (status) {
		ice_debug(hw, ICE_DBG_PTP, "Failed to read TIMETUS_L, status %d\n",
			  status);
		return status;
	}

	ice_debug(hw, ICE_DBG_PTP, "read INCVAL = 0x%016llx\n",
		  (unsigned long long)*incval);

	return ICE_SUCCESS;
}

/**
 * ice_ptp_prep_phy_adj_target_eth56g - Prepare PHY for adjust at target time
 * @hw: pointer to HW struct
 * @target_time: target time to program
 *
 * Program the PHY port Tx and Rx TIMER_CNT_ADJ registers used for the
 * ICE_PTP_ADJ_TIME_AT_TIME command. This should be used in conjunction with
 * ice_ptp_prep_phy_adj_eth56g to program an atomic adjustment that is
 * delayed until a specified target time.
 *
 * Note that a target time adjustment is not currently supported on E810
 * devices.
 */
static enum ice_status
ice_ptp_prep_phy_adj_target_eth56g(struct ice_hw *hw, u32 target_time)
{
	enum ice_status status;
	u8 port;

	for (port = 0; port < ICE_NUM_EXTERNAL_PORTS; port++) {
		/* Tx case */
		/* No sub-nanoseconds data */
		status = ice_write_phy_reg_eth56g_lp(hw, port,
						     PHY_REG_TX_TIMER_CNT_ADJ_L,
						     0, true);
		if (status)
			goto exit_err;

		status = ice_write_phy_reg_eth56g_lp(hw, port,
						     PHY_REG_TX_TIMER_CNT_ADJ_U,
						     target_time, true);
		if (status)
			goto exit_err;

		/* Rx case */
		/* No sub-nanoseconds data */
		status = ice_write_phy_reg_eth56g_lp(hw, port,
						     PHY_REG_RX_TIMER_CNT_ADJ_L,
						     0, true);
		if (status)
			goto exit_err;

		status = ice_write_phy_reg_eth56g_lp(hw, port,
						     PHY_REG_RX_TIMER_CNT_ADJ_U,
						     target_time, true);
		if (status)
			goto exit_err;
	}

	return ICE_SUCCESS;

exit_err:
	ice_debug(hw, ICE_DBG_PTP, "Failed to write target time for port %u, status %d\n",
		  port, status);

	return status;
}

/**
 * ice_ptp_read_port_capture_eth56g - Read a port's local time capture
 * @hw: pointer to HW struct
 * @port: Port number to read
 * @tx_ts: on return, the Tx port time capture
 * @rx_ts: on return, the Rx port time capture
 *
 * Read the port's Tx and Rx local time capture values.
 */
enum ice_status
ice_ptp_read_port_capture_eth56g(struct ice_hw *hw, u8 port, u64 *tx_ts,
				 u64 *rx_ts)
{
	enum ice_status status;

	/* Tx case */
	status = ice_read_64b_phy_reg_eth56g(hw, port, PHY_REG_TX_CAPTURE_L,
					     tx_ts);
	if (status) {
		ice_debug(hw, ICE_DBG_PTP, "Failed to read REG_TX_CAPTURE, status %d\n",
			  status);
		return status;
	}

	ice_debug(hw, ICE_DBG_PTP, "tx_init = %#016llx\n",
		  (unsigned long long)*tx_ts);

	/* Rx case */
	status = ice_read_64b_phy_reg_eth56g(hw, port, PHY_REG_RX_CAPTURE_L,
					     rx_ts);
	if (status) {
		ice_debug(hw, ICE_DBG_PTP, "Failed to read RX_CAPTURE, status %d\n",
			  status);
		return status;
	}

	ice_debug(hw, ICE_DBG_PTP, "rx_init = %#016llx\n",
		  (unsigned long long)*rx_ts);

	return ICE_SUCCESS;
}

/**
 * ice_ptp_one_port_cmd_eth56g - Prepare a single PHY port for a timer command
 * @hw: pointer to HW struct
 * @port: Port to which cmd has to be sent
 * @cmd: Command to be sent to the port
 * @lock_sbq: true if the sideband queue lock must be acquired
 *
 * Prepare the requested port for an upcoming timer sync command.
 */
enum ice_status
ice_ptp_one_port_cmd_eth56g(struct ice_hw *hw, u8 port,
			    enum ice_ptp_tmr_cmd cmd, bool lock_sbq)
{
	enum ice_status status;
	u32 cmd_val, val;
	u8 tmr_idx;

	tmr_idx = ice_get_ptp_src_clock_index(hw);
	cmd_val = tmr_idx << SEL_PHY_SRC;
	switch (cmd) {
	case ICE_PTP_INIT_TIME:
		cmd_val |= PHY_CMD_INIT_TIME;
		break;
	case ICE_PTP_INIT_INCVAL:
		cmd_val |= PHY_CMD_INIT_INCVAL;
		break;
	case ICE_PTP_ADJ_TIME:
		cmd_val |= PHY_CMD_ADJ_TIME;
		break;
	case ICE_PTP_ADJ_TIME_AT_TIME:
		cmd_val |= PHY_CMD_ADJ_TIME_AT_TIME;
		break;
	case ICE_PTP_READ_TIME:
		cmd_val |= PHY_CMD_READ_TIME;
		break;
	default:
		ice_warn(hw, "Unknown timer command %u\n", cmd);
		return ICE_ERR_PARAM;
	}

	/* Tx case */
	/* Read, modify, write */
	status = ice_read_phy_reg_eth56g_lp(hw, port, PHY_REG_TX_TMR_CMD, &val,
					    lock_sbq);
	if (status) {
		ice_debug(hw, ICE_DBG_PTP, "Failed to read TX_TMR_CMD, status %d\n",
			  status);
		return status;
	}

	/* Modify necessary bits only and perform write */
	val &= ~TS_CMD_MASK;
	val |= cmd_val;

	status = ice_write_phy_reg_eth56g_lp(hw, port, PHY_REG_TX_TMR_CMD, val,
					     lock_sbq);
	if (status) {
		ice_debug(hw, ICE_DBG_PTP, "Failed to write back TX_TMR_CMD, status %d\n",
			  status);
		return status;
	}

	/* Rx case */
	/* Read, modify, write */
	status = ice_read_phy_reg_eth56g_lp(hw, port, PHY_REG_RX_TMR_CMD, &val,
					    lock_sbq);
	if (status) {
		ice_debug(hw, ICE_DBG_PTP, "Failed to read RX_TMR_CMD, status %d\n",
			  status);
		return status;
	}

	/* Modify necessary bits only and perform write */
	val &= ~TS_CMD_MASK;
	val |= cmd_val;

	status = ice_write_phy_reg_eth56g_lp(hw, port, PHY_REG_RX_TMR_CMD, val,
					     lock_sbq);
	if (status) {
		ice_debug(hw, ICE_DBG_PTP, "Failed to write back RX_TMR_CMD, status %d\n",
			  status);
		return status;
	}

	return ICE_SUCCESS;
}

/**
 * ice_ptp_port_cmd_eth56g - Prepare all ports for a timer command
 * @hw: pointer to the HW struct
 * @cmd: timer command to prepare
 * @lock_sbq: true if the sideband queue lock must  be acquired
 *
 * Prepare all ports connected to this device for an upcoming timer sync
 * command.
 */
static enum ice_status
ice_ptp_port_cmd_eth56g(struct ice_hw *hw, enum ice_ptp_tmr_cmd cmd,
			bool lock_sbq)
{
	u8 port;

	for (port = 0; port < ICE_NUM_EXTERNAL_PORTS; port++) {
		enum ice_status status;
		status = ice_ptp_one_port_cmd_eth56g(hw, port, cmd, lock_sbq);
		if (status)
			return status;
	}

	return ICE_SUCCESS;
}

/**
 * ice_calc_fixed_tx_offset_eth56g - Calculated Fixed Tx offset for a port
 * @hw: pointer to the HW struct
 * @link_spd: the Link speed to calculate for
 *
 * Calculate the fixed offset due to known static latency data.
 */
static u64
ice_calc_fixed_tx_offset_eth56g(struct ice_hw *hw,
				enum ice_ptp_link_spd link_spd)
{
	u64 fixed_offset = 0;
	return fixed_offset;
}

/**
 * ice_phy_cfg_tx_offset_eth56g - Configure total Tx timestamp offset
 * @hw: pointer to the HW struct
 * @port: the PHY port to configure
 *
 * Program the PHY_REG_TOTAL_TX_OFFSET register with the total number of TUs to
 * adjust Tx timestamps by.
 *
 * To avoid overflow, when calculating the offset based on the known static
 * latency values, we use measurements in 1/100th of a nanosecond, and divide
 * the TUs per second up front. This avoids overflow while allowing
 * calculation of the adjustment using integer arithmetic.
 */
enum ice_status ice_phy_cfg_tx_offset_eth56g(struct ice_hw *hw, u8 port)
{
	enum ice_ptp_link_spd link_spd = ICE_PTP_LNK_SPD_10G;
	enum ice_status status;
	u64 total_offset;

	total_offset = ice_calc_fixed_tx_offset_eth56g(hw, link_spd);

	/* Now that the total offset has been calculated, program it to the
	 * PHY and indicate that the Tx offset is ready. After this,
	 * timestamps will be enabled.
	 */
	status = ice_write_64b_phy_reg_eth56g(hw, port,
					      PHY_REG_TOTAL_TX_OFFSET_L,
					      total_offset);
	if (status)
		return status;

	return ice_write_phy_reg_eth56g(hw, port, PHY_REG_TX_OFFSET_READY, 1);
}

/**
 * ice_calc_fixed_rx_offset_eth56g - Calculated the fixed Rx offset for a port
 * @hw: pointer to HW struct
 * @link_spd: The Link speed to calculate for
 *
 * Determine the fixed Rx latency for a given link speed.
 */
static u64
ice_calc_fixed_rx_offset_eth56g(struct ice_hw *hw,
				enum ice_ptp_link_spd link_spd)
{
	u64 fixed_offset = 0;
	return fixed_offset;
}

/**
 * ice_phy_cfg_rx_offset_eth56g - Configure total Rx timestamp offset
 * @hw: pointer to the HW struct
 * @port: the PHY port to configure
 *
 * Program the PHY_REG_TOTAL_RX_OFFSET register with the number of Time Units to
 * adjust Rx timestamps by. This combines calculations from the Vernier offset
 * measurements taken in hardware with some data about known fixed delay as
 * well as adjusting for multi-lane alignment delay.
 *
 * This function must be called only after the offset registers are valid,
 * i.e. after the Vernier calibration wait has passed, to ensure that the PHY
 * has measured the offset.
 *
 * To avoid overflow, when calculating the offset based on the known static
 * latency values, we use measurements in 1/100th of a nanosecond, and divide
 * the TUs per second up front. This avoids overflow while allowing
 * calculation of the adjustment using integer arithmetic.
 */
enum ice_status ice_phy_cfg_rx_offset_eth56g(struct ice_hw *hw, u8 port)
{
	enum ice_status status;
	u64 total_offset;

	total_offset = ice_calc_fixed_rx_offset_eth56g(hw, 0);

	/* Now that the total offset has been calculated, program it to the
	 * PHY and indicate that the Rx offset is ready. After this,
	 * timestamps will be enabled.
	 */
	status = ice_write_64b_phy_reg_eth56g(hw, port,
					      PHY_REG_TOTAL_RX_OFFSET_L,
					      total_offset);
	if (status)
		return status;

	return ice_write_phy_reg_eth56g(hw, port, PHY_REG_RX_OFFSET_READY, 1);
}

/**
 * ice_phy_cfg_intr_eth56g - Configure TX timestamp interrupt
 * @hw: pointer to the HW struct
 * @port: the timestamp port
 * @ena: enable or disable interrupt
 * @threshold: interrupt threshold
 *
 * Configure TX timestamp interrupt for the specified port
 */

enum ice_status
ice_phy_cfg_intr_eth56g(struct ice_hw *hw, u8 port, bool ena, u8 threshold)
{
	enum ice_status err;
	u32 val;

	err = ice_read_phy_reg_eth56g(hw, port, PHY_REG_TS_INT_CONFIG,
				      &val);
	if (err)
		return err;

	if (ena) {
		val |= PHY_TS_INT_CONFIG_ENA_M;
		val &= ~PHY_TS_INT_CONFIG_THRESHOLD_M;
		val |= ((threshold << PHY_TS_INT_CONFIG_THRESHOLD_S) &
			PHY_TS_INT_CONFIG_THRESHOLD_M);
	} else {
		val &= ~PHY_TS_INT_CONFIG_ENA_M;
	}

	err = ice_write_phy_reg_eth56g(hw, port, PHY_REG_TS_INT_CONFIG,
				       val);
	return err;
}

/**
 * ice_ptp_clear_phy_offset_ready_eth56g - Clear PHY OFFSET_READY registers
 * @hw: pointer to the HW struct
 *
 * Clear PHY TX_/RX_OFFSET_READY registers, effectively marking all transmitted
 * and received timestamps as invalid.
 */
static enum ice_status ice_ptp_clear_phy_offset_ready_eth56g(struct ice_hw *hw)
{
	u8 port;

	for (port = 0; port < ICE_NUM_EXTERNAL_PORTS; port++) {
		enum ice_status status;

		status = ice_write_phy_reg_eth56g(hw, port,
						  PHY_REG_TX_OFFSET_READY, 0);
		if (status) {
			ice_warn(hw, "Failed to clear PHY TX_OFFSET_READY register\n");
			return status;
		}

		status = ice_write_phy_reg_eth56g(hw, port,
						  PHY_REG_RX_OFFSET_READY, 0);
		if (status) {
			ice_warn(hw, "Failed to clear PHY RX_OFFSET_READY register\n");
			return status;
		}
	}

	return ICE_SUCCESS;
}

/**
 * ice_read_phy_and_phc_time_eth56g - Simultaneously capture PHC and PHY time
 * @hw: pointer to the HW struct
 * @port: the PHY port to read
 * @phy_time: on return, the 64bit PHY timer value
 * @phc_time: on return, the lower 64bits of PHC time
 *
 * Issue a ICE_PTP_READ_TIME timer command to simultaneously capture the PHY
 * and PHC timer values.
 */
#ifndef SIMICS_SUPPORT
static enum ice_status
ice_read_phy_and_phc_time_eth56g(struct ice_hw *hw, u8 port, u64 *phy_time,
				 u64 *phc_time)
#else /* SIMICS_SUPPORT */
enum ice_status
ice_read_phy_and_phc_time_eth56g(struct ice_hw *hw, u8 port, u64 *phy_time_rx,
				 u64 *phy_time_tx, u64 *phc_time)
#endif /* SIMICS_SUPPORT */
{
	enum ice_status status;
	u64 tx_time, rx_time;
	u32 zo, lo;
	u8 tmr_idx;

	tmr_idx = ice_get_ptp_src_clock_index(hw);

	/* Prepare the PHC timer for a ICE_PTP_READ_TIME capture command */
	ice_ptp_src_cmd(hw, ICE_PTP_READ_TIME);

	/* Prepare the PHY timer for a ICE_PTP_READ_TIME capture command */
	status = ice_ptp_one_port_cmd_eth56g(hw, port, ICE_PTP_READ_TIME, true);
	if (status)
		return status;

	/* Issue the sync to start the ICE_PTP_READ_TIME capture */
	ice_ptp_exec_tmr_cmd(hw);
	ice_ptp_clean_cmd(hw);

	/* Read the captured PHC time from the shadow time registers */
	zo = rd32(hw, GLTSYN_SHTIME_0(tmr_idx));
	lo = rd32(hw, GLTSYN_SHTIME_L(tmr_idx));
	*phc_time = (u64)lo << 32 | zo;

	/* Read the captured PHY time from the PHY shadow registers */
	status = ice_ptp_read_port_capture_eth56g(hw, port, &tx_time, &rx_time);
	if (status)
		return status;

	/* If the PHY Tx and Rx timers don't match, log a warning message.
	 * Note that this should not happen in normal circumstances since the
	 * driver always programs them together.
	 */
	if (tx_time != rx_time)
		ice_warn(hw, "PHY port %u Tx and Rx timers do not match, tx_time 0x%016llX, rx_time 0x%016llX\n",
			 port, (unsigned long long)tx_time,
			 (unsigned long long)rx_time);

#ifdef SIMICS_SUPPORT
	*phy_time_tx = tx_time;
	*phy_time_rx = rx_time;
#else /* SIMICS_SUPPORT */
	*phy_time = tx_time;
#endif /* SIMICS_SUPPORT */

	return ICE_SUCCESS;
}

/**
 * ice_sync_phy_timer_eth56g - Synchronize the PHY timer with PHC timer
 * @hw: pointer to the HW struct
 * @port: the PHY port to synchronize
 *
 * Perform an adjustment to ensure that the PHY and PHC timers are in sync.
 * This is done by issuing a ICE_PTP_READ_TIME command which triggers a
 * simultaneous read of the PHY timer and PHC timer. Then we use the
 * difference to calculate an appropriate 2s complement addition to add
 * to the PHY timer in order to ensure it reads the same value as the
 * primary PHC timer.
 */
static enum ice_status ice_sync_phy_timer_eth56g(struct ice_hw *hw, u8 port)
{
#ifdef SIMICS_SUPPORT
	u64 phc_time, phy_time_rx, phy_time_tx, diff_rx, diff_tx;
#else /* SIMICS_SUPPORT */
	u64 phc_time, phy_time, difference;
#endif /* SIMICS_SUPPORT */
	enum ice_status status;

	if (!ice_ptp_lock(hw)) {
		ice_debug(hw, ICE_DBG_PTP, "Failed to acquire PTP semaphore\n");
		return ICE_ERR_NOT_READY;
	}

#ifdef SIMICS_SUPPORT
	status = ice_read_phy_and_phc_time_eth56g(hw, port, &phy_time_rx,
						  &phy_time_tx, &phc_time);
#else /* SIMICS_SUPPORT */
	status = ice_read_phy_and_phc_time_eth56g(hw, port, &phy_time,
						  &phc_time);
#endif /* SIMICS_SUPPORT */
	if (status)
		goto err_unlock;

	/* Calculate the amount required to add to the port time in order for
	 * it to match the PHC time.
	 *
	 * Note that the port adjustment is done using 2s complement
	 * arithmetic. This is convenient since it means that we can simply
	 * calculate the difference between the PHC time and the port time,
	 * and it will be interpreted correctly.
	 */

	ice_ptp_src_cmd(hw, ICE_PTP_NOP);
#ifdef SIMICS_SUPPORT
	diff_rx = phc_time - phy_time_rx;
	diff_tx = phc_time - phy_time_tx;

	status = ice_ptp_prep_port_adj_eth56g(hw, port, (s64)diff_rx,
					      (s64)diff_tx, true);
#else /* SIMICS_SUPPORT */
	difference = phc_time - phy_time;

	status = ice_ptp_prep_port_adj_eth56g(hw, port, (s64)difference, true);
#endif /* SIMICS_SUPPORT */
	if (status)
		goto err_unlock;

	status = ice_ptp_one_port_cmd_eth56g(hw, port, ICE_PTP_ADJ_TIME, true);
	if (status)
		goto err_unlock;

	/* Issue the sync to activate the time adjustment */
	ice_ptp_exec_tmr_cmd(hw);
	ice_ptp_clean_cmd(hw);

	/* Re-capture the timer values to flush the command registers and
	 * verify that the time was properly adjusted.
	 */
#ifdef SIMICS_SUPPORT
	status = ice_read_phy_and_phc_time_eth56g(hw, port, &phy_time_rx,
						  &phy_time_tx, &phc_time);
#else /* SIMICS_SUPPORT */

	status = ice_read_phy_and_phc_time_eth56g(hw, port, &phy_time,
						  &phc_time);
#endif /* SIMICS_SUPPORT */
	if (status)
		goto err_unlock;

#ifdef SIMICS_SUPPORT
	ice_info(hw, "Port %u PHY time synced to PHC: 0x%016llX/0x%016llX, 0x%016llX\n",
		 port, phy_time_rx, phy_time_tx,
		 (unsigned long long)phc_time);
#else /* SIMICS_SUPPORT */
	ice_info(hw, "Port %u PHY time synced to PHC: 0x%016llX, 0x%016llX\n",
		 port, (unsigned long long)phy_time,
		 (unsigned long long)phc_time);
#endif /* SIMICS_SUPPORT */

err_unlock:
	ice_ptp_unlock(hw);
	return status;
}

/**
 * ice_stop_phy_timer_eth56g - Stop the PHY clock timer
 * @hw: pointer to the HW struct
 * @port: the PHY port to stop
 * @soft_reset: if true, hold the SOFT_RESET bit of PHY_REG_PS
 *
 * Stop the clock of a PHY port. This must be done as part of the flow to
 * re-calibrate Tx and Rx timestamping offsets whenever the clock time is
 * initialized or when link speed changes.
 */
enum ice_status
ice_stop_phy_timer_eth56g(struct ice_hw *hw, u8 port, bool soft_reset)
{
	enum ice_status status;

	status = ice_write_phy_reg_eth56g(hw, port, PHY_REG_TX_OFFSET_READY, 0);
	if (status)
		return status;

	status = ice_write_phy_reg_eth56g(hw, port, PHY_REG_RX_OFFSET_READY, 0);
	if (status)
		return status;

	ice_debug(hw, ICE_DBG_PTP, "Disabled clock on PHY port %u\n", port);

	return ICE_SUCCESS;
}

/**
 * ice_start_phy_timer_eth56g - Start the PHY clock timer
 * @hw: pointer to the HW struct
 * @port: the PHY port to start
 *
 * Start the clock of a PHY port. This must be done as part of the flow to
 * re-calibrate Tx and Rx timestamping offsets whenever the clock time is
 * initialized or when link speed changes.
 *
 */
enum ice_status
ice_start_phy_timer_eth56g(struct ice_hw *hw, u8 port)
{
	enum ice_status status;
	u32 lo, hi;
	u64 incval;
	u8 tmr_idx;

	tmr_idx = ice_get_ptp_src_clock_index(hw);

	status = ice_stop_phy_timer_eth56g(hw, port, false);
	if (status)
		return status;

	ice_ptp_src_cmd(hw, ICE_PTP_NOP);

	lo = rd32(hw, GLTSYN_INCVAL_L(tmr_idx));
	hi = rd32(hw, GLTSYN_INCVAL_H(tmr_idx));
	incval = (u64)hi << 32 | lo;

	status = ice_write_40b_phy_reg_eth56g(hw, port, PHY_REG_TIMETUS_L,
					      incval);
	if (status)
		return status;

	status = ice_ptp_one_port_cmd_eth56g(hw, port, ICE_PTP_INIT_INCVAL,
					     true);
	if (status)
		return status;

	ice_ptp_exec_tmr_cmd(hw);

	status = ice_sync_phy_timer_eth56g(hw, port);
	if (status)
		return status;

	/* Program the Tx offset */
	status = ice_phy_cfg_tx_offset_eth56g(hw, port);
	if (status)
		return status;

	/* Program the Rx offset */
	status = ice_phy_cfg_rx_offset_eth56g(hw, port);
	if (status)
		return status;

	ice_debug(hw, ICE_DBG_PTP, "Enabled clock on PHY port %u\n", port);

	return ICE_SUCCESS;
}

/**
 * ice_sb_access_ena_eth56g - Enable SB devices (PHY and others) access
 * @hw: pointer to HW struct
 * @enable: Enable or disable access
 *
 * Enable sideband devices (PHY and others) access.
 */
static void ice_sb_access_ena_eth56g(struct ice_hw *hw, bool enable)
{
	u32 regval;

	/* Enable reading and writing switch and PHY registers over the
	 * sideband queue.
	 */
#define PF_SB_REM_DEV_CTL_SWITCH_READ BIT(1)
#define PF_SB_REM_DEV_CTL_PHY0 BIT(2)
	regval = rd32(hw, PF_SB_REM_DEV_CTL);
	if (enable)
		regval |= (PF_SB_REM_DEV_CTL_SWITCH_READ |
			   PF_SB_REM_DEV_CTL_PHY0);
	else
		regval &= ~(PF_SB_REM_DEV_CTL_SWITCH_READ |
			    PF_SB_REM_DEV_CTL_PHY0);

	wr32(hw, PF_SB_REM_DEV_CTL, regval);
}

/**
 * ice_ptp_init_phc_eth56g - Perform E822 specific PHC initialization
 * @hw: pointer to HW struct
 *
 * Perform PHC initialization steps specific to E822 devices.
 */
static enum ice_status ice_ptp_init_phc_eth56g(struct ice_hw *hw)
{
	enum ice_status status = ICE_SUCCESS;

#ifdef CGU_SUPPORT
	/* Initialize the Clock Generation Unit */
	status = ice_init_cgu_e822(hw);
#endif /* CGU_SUPPORT */

	return status;
}

/**
 * ice_ptp_read_tx_hwtstamp_status_eth56g - Get TX timestamp status
 * @hw: pointer to the HW struct
 * @ts_status: the timestamp mask pointer
 *
 * Read the PHY Tx timestamp status mask indicating which ports have Tx
 * timestamps available.
 */
enum ice_status
ice_ptp_read_tx_hwtstamp_status_eth56g(struct ice_hw *hw, u32 *ts_status)
{
	u32 curr_status;
	u8 phy, mask;

	mask = (1 << hw->phy_ports) - 1;
	*ts_status = 0;

	for (phy = 0; phy < hw->num_phys; phy++) {
		enum ice_status status;

		status = ice_read_phy_eth56g_raw_lp(hw, phy, PHY_PTP_INT_STATUS,
						    &curr_status, true);

		if (status)
			return status;

		*ts_status |= (curr_status & mask) << (phy * hw->phy_ports);
	}

	ice_debug(hw, ICE_DBG_PTP, "PHY interrupt status: %x\n", *ts_status);

	return ICE_SUCCESS;
}

/**
 * ice_get_phy_tx_tstamp_ready_eth56g - Read the Tx memory status register
 * @hw: pointer to the HW struct
 * @port: the PHY port to read from
 * @tstamp_ready: contents of the Tx memory status register
 *
 * Read the PHY_REG_TX_MEMORY_STATUS register indicating which timestamps in
 * the PHY are ready. A set bit means the corresponding timestamp is valid and
 * ready to be captured from the PHY timestamp block.
 */
static enum ice_status
ice_get_phy_tx_tstamp_ready_eth56g(struct ice_hw *hw, u8 port,
				   u64 *tstamp_ready)
{
	enum ice_status status;

	status = ice_read_64b_phy_reg_eth56g(hw, port,
					     PHY_REG_TX_MEMORY_STATUS_L,
					     tstamp_ready);
	if (status) {
		ice_debug(hw, ICE_DBG_PTP, "Failed to read TX_MEMORY_STATUS for port %u, status %d\n",
			  port, status);
		return status;
	}

	return ICE_SUCCESS;
}
#endif /* ETH56G_SUPPORT */

/* ----------------------------------------------------------------------------
 * E822 family functions
 *
 * The following functions operate on the E822 family of devices.
 */

/**
 * ice_fill_phy_msg_e822 - Fill message data for a PHY register access
 * @msg: the PHY message buffer to fill in
 * @port: the port to access
 * @offset: the register offset
 */
static void
ice_fill_phy_msg_e822(struct ice_sbq_msg_input *msg, u8 port, u16 offset)
{
	int phy_port, phy, quadtype;

	phy_port = port % ICE_PORTS_PER_PHY_E822;
	phy = port / ICE_PORTS_PER_PHY_E822;
	quadtype = (port / ICE_PORTS_PER_QUAD) % ICE_QUADS_PER_PHY_E822;

	if (quadtype == 0) {
		msg->msg_addr_low = P_Q0_L(P_0_BASE + offset, phy_port);
		msg->msg_addr_high = P_Q0_H(P_0_BASE + offset, phy_port);
	} else {
		msg->msg_addr_low = P_Q1_L(P_4_BASE + offset, phy_port);
		msg->msg_addr_high = P_Q1_H(P_4_BASE + offset, phy_port);
	}

	if (phy == 0)
		msg->dest_dev = rmn_0;
	else if (phy == 1)
		msg->dest_dev = rmn_1;
	else
		msg->dest_dev = rmn_2;
}

/**
 * ice_is_64b_phy_reg_e822 - Check if this is a 64bit PHY register
 * @low_addr: the low address to check
 * @high_addr: on return, contains the high address of the 64bit register
 *
 * Checks if the provided low address is one of the known 64bit PHY values
 * represented as two 32bit registers. If it is, return the appropriate high
 * register offset to use.
 */
static bool ice_is_64b_phy_reg_e822(u16 low_addr, u16 *high_addr)
{
	switch (low_addr) {
	case P_REG_PAR_PCS_TX_OFFSET_L:
		*high_addr = P_REG_PAR_PCS_TX_OFFSET_U;
		return true;
	case P_REG_PAR_PCS_RX_OFFSET_L:
		*high_addr = P_REG_PAR_PCS_RX_OFFSET_U;
		return true;
	case P_REG_PAR_TX_TIME_L:
		*high_addr = P_REG_PAR_TX_TIME_U;
		return true;
	case P_REG_PAR_RX_TIME_L:
		*high_addr = P_REG_PAR_RX_TIME_U;
		return true;
	case P_REG_TOTAL_TX_OFFSET_L:
		*high_addr = P_REG_TOTAL_TX_OFFSET_U;
		return true;
	case P_REG_TOTAL_RX_OFFSET_L:
		*high_addr = P_REG_TOTAL_RX_OFFSET_U;
		return true;
	case P_REG_UIX66_10G_40G_L:
		*high_addr = P_REG_UIX66_10G_40G_U;
		return true;
	case P_REG_UIX66_25G_100G_L:
		*high_addr = P_REG_UIX66_25G_100G_U;
		return true;
	case P_REG_TX_CAPTURE_L:
		*high_addr = P_REG_TX_CAPTURE_U;
		return true;
	case P_REG_RX_CAPTURE_L:
		*high_addr = P_REG_RX_CAPTURE_U;
		return true;
	case P_REG_TX_TIMER_INC_PRE_L:
		*high_addr = P_REG_TX_TIMER_INC_PRE_U;
		return true;
	case P_REG_RX_TIMER_INC_PRE_L:
		*high_addr = P_REG_RX_TIMER_INC_PRE_U;
		return true;
	default:
		return false;
	}
}

/**
 * ice_is_40b_phy_reg_e822 - Check if this is a 40bit PHY register
 * @low_addr: the low address to check
 * @high_addr: on return, contains the high address of the 40bit value
 *
 * Checks if the provided low address is one of the known 40bit PHY values
 * split into two registers with the lower 8 bits in the low register and the
 * upper 32 bits in the high register. If it is, return the appropriate high
 * register offset to use.
 */
static bool ice_is_40b_phy_reg_e822(u16 low_addr, u16 *high_addr)
{
	switch (low_addr) {
	case P_REG_TIMETUS_L:
		*high_addr = P_REG_TIMETUS_U;
		return true;
	case P_REG_PAR_RX_TUS_L:
		*high_addr = P_REG_PAR_RX_TUS_U;
		return true;
	case P_REG_PAR_TX_TUS_L:
		*high_addr = P_REG_PAR_TX_TUS_U;
		return true;
	case P_REG_PCS_RX_TUS_L:
		*high_addr = P_REG_PCS_RX_TUS_U;
		return true;
	case P_REG_PCS_TX_TUS_L:
		*high_addr = P_REG_PCS_TX_TUS_U;
		return true;
	case P_REG_DESK_PAR_RX_TUS_L:
		*high_addr = P_REG_DESK_PAR_RX_TUS_U;
		return true;
	case P_REG_DESK_PAR_TX_TUS_L:
		*high_addr = P_REG_DESK_PAR_TX_TUS_U;
		return true;
	case P_REG_DESK_PCS_RX_TUS_L:
		*high_addr = P_REG_DESK_PCS_RX_TUS_U;
		return true;
	case P_REG_DESK_PCS_TX_TUS_L:
		*high_addr = P_REG_DESK_PCS_TX_TUS_U;
		return true;
	default:
		return false;
	}
}

/**
 * ice_read_phy_reg_e822_lp - Read a PHY register
 * @hw: pointer to the HW struct
 * @port: PHY port to read from
 * @offset: PHY register offset to read
 * @val: on return, the contents read from the PHY
 * @lock_sbq: true if the sideband queue lock must be acquired
 *
 * Read a PHY register for the given port over the device sideband queue.
 */
static enum ice_status
ice_read_phy_reg_e822_lp(struct ice_hw *hw, u8 port, u16 offset, u32 *val,
			 bool lock_sbq)
{
	struct ice_sbq_msg_input msg = {0};
	enum ice_status status;


	ice_fill_phy_msg_e822(&msg, port, offset);
	msg.opcode = ice_sbq_msg_rd;

	status = ice_sbq_rw_reg_lp(hw, &msg, lock_sbq);
	if (status) {
		ice_debug(hw, ICE_DBG_PTP, "Failed to send message to phy, status %d\n",
			  status);
		return status;
	}

	*val = msg.data;

	return ICE_SUCCESS;
}

enum ice_status
ice_read_phy_reg_e822(struct ice_hw *hw, u8 port, u16 offset, u32 *val)
{
	return ice_read_phy_reg_e822_lp(hw, port, offset, val, true);
}

/**
 * ice_read_40b_phy_reg_e822 - Read a 40bit value from PHY registers
 * @hw: pointer to the HW struct
 * @port: PHY port to read from
 * @low_addr: offset of the lower register to read from
 * @val: on return, the contents of the 40bit value from the PHY registers
 *
 * Reads the two registers associated with a 40bit value and returns it in the
 * val pointer. The offset always specifies the lower register offset to use.
 * The high offset is looked up. This function only operates on registers
 * known to be split into a lower 8 bit chunk and an upper 32 bit chunk.
 */
static enum ice_status
ice_read_40b_phy_reg_e822(struct ice_hw *hw, u8 port, u16 low_addr, u64 *val)
{
	enum ice_status status;
	u32 low, high;
	u16 high_addr;

	/* Only operate on registers known to be split into two 32bit
	 * registers.
	 */
	if (!ice_is_40b_phy_reg_e822(low_addr, &high_addr)) {
		ice_debug(hw, ICE_DBG_PTP, "Invalid 64b register addr 0x%08x\n",
			  low_addr);
		return ICE_ERR_PARAM;
	}

	status = ice_read_phy_reg_e822(hw, port, low_addr, &low);
	if (status) {
		ice_debug(hw, ICE_DBG_PTP, "Failed to read from low register 0x%08x\n, status %d",
			  low_addr, status);
		return status;
	}

	status = ice_read_phy_reg_e822(hw, port, high_addr, &high);
	if (status) {
		ice_debug(hw, ICE_DBG_PTP, "Failed to read from high register 0x%08x\n, status %d",
			  high_addr, status);
		return status;
	}

	*val = (u64)high << P_REG_40B_HIGH_S | (low & P_REG_40B_LOW_M);

	return ICE_SUCCESS;
}

/**
 * ice_read_64b_phy_reg_e822 - Read a 64bit value from PHY registers
 * @hw: pointer to the HW struct
 * @port: PHY port to read from
 * @low_addr: offset of the lower register to read from
 * @val: on return, the contents of the 64bit value from the PHY registers
 *
 * Reads the two registers associated with a 64bit value and returns it in the
 * val pointer. The offset always specifies the lower register offset to use.
 * The high offset is looked up. This function only operates on registers
 * known to be two parts of a 64bit value.
 */
static enum ice_status
ice_read_64b_phy_reg_e822(struct ice_hw *hw, u8 port, u16 low_addr, u64 *val)
{
	enum ice_status status;
	u32 low, high;
	u16 high_addr;

	/* Only operate on registers known to be split into two 32bit
	 * registers.
	 */
	if (!ice_is_64b_phy_reg_e822(low_addr, &high_addr)) {
		ice_debug(hw, ICE_DBG_PTP, "Invalid 64b register addr 0x%08x\n",
			  low_addr);
		return ICE_ERR_PARAM;
	}

	status = ice_read_phy_reg_e822(hw, port, low_addr, &low);
	if (status) {
		ice_debug(hw, ICE_DBG_PTP, "Failed to read from low register 0x%08x\n, status %d",
			  low_addr, status);
		return status;
	}

	status = ice_read_phy_reg_e822(hw, port, high_addr, &high);
	if (status) {
		ice_debug(hw, ICE_DBG_PTP, "Failed to read from high register 0x%08x\n, status %d",
			  high_addr, status);
		return status;
	}

	*val = (u64)high << 32 | low;

	return ICE_SUCCESS;
}

/**
 * ice_write_phy_reg_e822_lp - Write a PHY register
 * @hw: pointer to the HW struct
 * @port: PHY port to write to
 * @offset: PHY register offset to write
 * @val: The value to write to the register
 * @lock_sbq: true if the sideband queue lock must be acquired
 *
 * Write a PHY register for the given port over the device sideband queue.
 */
static enum ice_status
ice_write_phy_reg_e822_lp(struct ice_hw *hw, u8 port, u16 offset, u32 val,
			  bool lock_sbq)
{
	struct ice_sbq_msg_input msg = {0};
	enum ice_status status;


	ice_fill_phy_msg_e822(&msg, port, offset);
	msg.opcode = ice_sbq_msg_wr;
	msg.data = val;

	status = ice_sbq_rw_reg_lp(hw, &msg, lock_sbq);
	if (status) {
		ice_debug(hw, ICE_DBG_PTP, "Failed to send message to phy, status %d\n",
			  status);
		return status;
	}

	return ICE_SUCCESS;
}

enum ice_status
ice_write_phy_reg_e822(struct ice_hw *hw, u8 port, u16 offset, u32 val)
{
	return ice_write_phy_reg_e822_lp(hw, port, offset, val, true);
}

/**
 * ice_write_40b_phy_reg_e822 - Write a 40b value to the PHY
 * @hw: pointer to the HW struct
 * @port: port to write to
 * @low_addr: offset of the low register
 * @val: 40b value to write
 *
 * Write the provided 40b value to the two associated registers by splitting
 * it up into two chunks, the lower 8 bits and the upper 32 bits.
 */
static enum ice_status
ice_write_40b_phy_reg_e822(struct ice_hw *hw, u8 port, u16 low_addr, u64 val)
{
	enum ice_status status;
	u32 low, high;
	u16 high_addr;

	/* Only operate on registers known to be split into a lower 8 bit
	 * register and an upper 32 bit register.
	 */
	if (!ice_is_40b_phy_reg_e822(low_addr, &high_addr)) {
		ice_debug(hw, ICE_DBG_PTP, "Invalid 40b register addr 0x%08x\n",
			  low_addr);
		return ICE_ERR_PARAM;
	}

	low = (u32)(val & P_REG_40B_LOW_M);
	high = (u32)(val >> P_REG_40B_HIGH_S);

	status = ice_write_phy_reg_e822(hw, port, low_addr, low);
	if (status) {
		ice_debug(hw, ICE_DBG_PTP, "Failed to write to low register 0x%08x\n, status %d",
			  low_addr, status);
		return status;
	}

	status = ice_write_phy_reg_e822(hw, port, high_addr, high);
	if (status) {
		ice_debug(hw, ICE_DBG_PTP, "Failed to write to high register 0x%08x\n, status %d",
			  high_addr, status);
		return status;
	}

	return ICE_SUCCESS;
}

/**
 * ice_write_64b_phy_reg_e822 - Write a 64bit value to PHY registers
 * @hw: pointer to the HW struct
 * @port: PHY port to read from
 * @low_addr: offset of the lower register to read from
 * @val: the contents of the 64bit value to write to PHY
 *
 * Write the 64bit value to the two associated 32bit PHY registers. The offset
 * is always specified as the lower register, and the high address is looked
 * up. This function only operates on registers known to be two parts of
 * a 64bit value.
 */
static enum ice_status
ice_write_64b_phy_reg_e822(struct ice_hw *hw, u8 port, u16 low_addr, u64 val)
{
	enum ice_status status;
	u32 low, high;
	u16 high_addr;

	/* Only operate on registers known to be split into two 32bit
	 * registers.
	 */
	if (!ice_is_64b_phy_reg_e822(low_addr, &high_addr)) {
		ice_debug(hw, ICE_DBG_PTP, "Invalid 64b register addr 0x%08x\n",
			  low_addr);
		return ICE_ERR_PARAM;
	}

	low = ICE_LO_DWORD(val);
	high = ICE_HI_DWORD(val);

	status = ice_write_phy_reg_e822(hw, port, low_addr, low);
	if (status) {
		ice_debug(hw, ICE_DBG_PTP, "Failed to write to low register 0x%08x\n, status %d",
			  low_addr, status);
		return status;
	}

	status = ice_write_phy_reg_e822(hw, port, high_addr, high);
	if (status) {
		ice_debug(hw, ICE_DBG_PTP, "Failed to write to high register 0x%08x\n, status %d",
			  high_addr, status);
		return status;
	}

	return ICE_SUCCESS;
}

/**
 * ice_fill_quad_msg_e822 - Fill message data for quad register access
 * @msg: the PHY message buffer to fill in
 * @quad: the quad to access
 * @offset: the register offset
 *
 * Fill a message buffer for accessing a register in a quad shared between
 * multiple PHYs.
 */
static enum ice_status
#ifndef QV_SUPPORT
ice_fill_quad_msg_e822(struct ice_sbq_msg_input *msg, u8 quad, u16 offset)
#else /* !QV_SUPPORT */
ice_fill_quad_msg_e822(struct ice_hw *hw, struct ice_sbq_msg_input *msg,
		       u8 quad, u16 offset)
#endif /* QV_SUPPORT */
{
	u32 addr;
#if defined(QV_SUPPORT)
	u8 phy;

#ifdef QV_SUPPORT
	if (!hw->is_switch_mode || hw->is_bmsm)
		goto skip_simple_switch_mode;
#endif /* QV_SUPPORT */
	if (quad >= ICE_MAX_QUAD)
		return ICE_ERR_PARAM;

	phy = quad / ICE_QUADS_PER_PHY_E822;
	switch (phy) {
	case ICE_PTP_PHY_0:
		msg->dest_dev = rmn_0;
		break;
	case ICE_PTP_PHY_1:
		msg->dest_dev = rmn_1;
		break;
	case ICE_PTP_PHY_2:
		msg->dest_dev = rmn_2;
		break;
	default:
		return ICE_ERR_PARAM;
	}
#ifdef QV_SUPPORT
skip_simple_switch_mode:
	if (!hw->is_switch_mode || hw->is_bmsm) {
		UNUSED_VAR(phy);
		if (quad >= ICE_MAX_QUAD)
			return ICE_ERR_PARAM;

		msg->dest_dev = rmn_0;
	}
#endif /* QV_SUPPORT */
#else /* (!SWITCH_MODE || BMSM_MODE) && !QV_SUPPORT */

	if (quad >= ICE_MAX_QUAD)
		return ICE_ERR_PARAM;

	msg->dest_dev = rmn_0;
#endif /* (SWITCH_MODE && !BMSM_MODE) || QV_SUPPORT */

	if ((quad % ICE_QUADS_PER_PHY_E822) == 0)
		addr = Q_0_BASE + offset;
	else
		addr = Q_1_BASE + offset;

	msg->msg_addr_low = ICE_LO_WORD(addr);
	msg->msg_addr_high = ICE_HI_WORD(addr);

	return ICE_SUCCESS;
}

/**
 * ice_read_quad_reg_e822_lp - Read a PHY quad register
 * @hw: pointer to the HW struct
 * @quad: quad to read from
 * @offset: quad register offset to read
 * @val: on return, the contents read from the quad
 * @lock_sbq: true if the sideband queue lock must be acquired
 *
 * Read a quad register over the device sideband queue. Quad registers are
 * shared between multiple PHYs.
 */
static enum ice_status
ice_read_quad_reg_e822_lp(struct ice_hw *hw, u8 quad, u16 offset, u32 *val,
			  bool lock_sbq)
{
	struct ice_sbq_msg_input msg = {0};
	enum ice_status status;

#ifdef QV_SUPPORT
	status = ice_fill_quad_msg_e822(hw, &msg, quad, offset);
#else /* !QV_SUPPORT */
	status = ice_fill_quad_msg_e822(&msg, quad, offset);
#endif /* QV_SUPPORT */
	if (status)
		goto exit_err;

	msg.opcode = ice_sbq_msg_rd;

	status = ice_sbq_rw_reg_lp(hw, &msg, lock_sbq);
exit_err:
	if (status)
		ice_debug(hw, ICE_DBG_PTP, "Failed to send message to phy, status %d\n",
			  status);
	else
		*val = msg.data;

	return status;
}

enum ice_status
ice_read_quad_reg_e822(struct ice_hw *hw, u8 quad, u16 offset, u32 *val)
{
	return ice_read_quad_reg_e822_lp(hw, quad, offset, val, true);
}

/**
 * ice_write_quad_reg_e822_lp - Write a PHY quad register
 * @hw: pointer to the HW struct
 * @quad: quad to write to
 * @offset: quad register offset to write
 * @val: The value to write to the register
 * @lock_sbq: true if the sideband queue lock must be acquired
 *
 * Write a quad register over the device sideband queue. Quad registers are
 * shared between multiple PHYs.
 */
static enum ice_status
ice_write_quad_reg_e822_lp(struct ice_hw *hw, u8 quad, u16 offset, u32 val,
			   bool lock_sbq)
{
	struct ice_sbq_msg_input msg = {0};
	enum ice_status status;

#ifdef QV_SUPPORT
	status = ice_fill_quad_msg_e822(hw, &msg, quad, offset);
#else /* !QV_SUPPORT */
	status = ice_fill_quad_msg_e822(&msg, quad, offset);
#endif /* QV_SUPPORT */
	if (status)
		goto exit_err;

	msg.opcode = ice_sbq_msg_wr;
	msg.data = val;

	status = ice_sbq_rw_reg_lp(hw, &msg, lock_sbq);
exit_err:
	if (status)
		ice_debug(hw, ICE_DBG_PTP, "Failed to send message to phy, status %d\n",
			  status);

	return status;
}

enum ice_status
ice_write_quad_reg_e822(struct ice_hw *hw, u8 quad, u16 offset, u32 val)
{
	return ice_write_quad_reg_e822_lp(hw, quad, offset, val, true);
}

/**
 * ice_read_phy_tstamp_e822 - Read a PHY timestamp out of the quad block
 * @hw: pointer to the HW struct
 * @quad: the quad to read from
 * @idx: the timestamp index to read
 * @tstamp: on return, the 40bit timestamp value
 *
 * Read a 40bit timestamp value out of the two associated registers in the
 * quad memory block that is shared between the internal PHYs of the E822
 * family of devices.
 */
static enum ice_status
ice_read_phy_tstamp_e822(struct ice_hw *hw, u8 quad, u8 idx, u64 *tstamp)
{
	enum ice_status status;
	u16 lo_addr, hi_addr;
	u32 lo, hi;

	lo_addr = (u16)TS_L(Q_REG_TX_MEMORY_BANK_START, idx);
	hi_addr = (u16)TS_H(Q_REG_TX_MEMORY_BANK_START, idx);

	status = ice_read_quad_reg_e822(hw, quad, lo_addr, &lo);
	if (status) {
		ice_debug(hw, ICE_DBG_PTP, "Failed to read low PTP timestamp register, status %d\n",
			  status);
		return status;
	}

	status = ice_read_quad_reg_e822(hw, quad, hi_addr, &hi);
	if (status) {
		ice_debug(hw, ICE_DBG_PTP, "Failed to read high PTP timestamp register, status %d\n",
			  status);
		return status;
	}

	/* For E822 based internal PHYs, the timestamp is reported with the
	 * lower 8 bits in the low register, and the upper 32 bits in the high
	 * register.
	 */
	*tstamp = ((u64)hi) << TS_PHY_HIGH_S | ((u64)lo & TS_PHY_LOW_M);

	return ICE_SUCCESS;
}

/**
 * ice_clear_phy_tstamp_e822 - Drop a timestamp from the quad block
 * @hw: pointer to the HW struct
 * @quad: the quad to read from
 * @idx: the timestamp index to reset
 *
 * Read the timetamp out of the quad to clear its timestamp status bit from
 * the PHY quad block that is shared between the internal PHYs of the E822
 * devices.
 *
 * Note that software cannot directly write the quad memory bank registers,
 * and must use ice_ptp_reset_ts_memory_quad_e822 for that purpose.
 *
 * This function should only be called on an idx whose bit is set according to
 * ice_get_phy_tx_tstamp_ready.
 */
static enum ice_status
ice_clear_phy_tstamp_e822(struct ice_hw *hw, u8 quad, u8 idx)
{
	enum ice_status status;
	u64 unused_tstamp;

	status = ice_read_phy_tstamp_e822(hw, quad, idx, &unused_tstamp);
	if (status) {
		ice_debug(hw, ICE_DBG_PTP, "Failed to read the timestamp register for quad %u, idx %u, status %d\n",
			  quad, idx, status);
		return status;
	}

	return ICE_SUCCESS;
}

/**
 * ice_ptp_reset_ts_memory_quad_e822 - Clear all timestamps from the quad block
 * @hw: pointer to the HW struct
 * @quad: the quad to read from
 *
 * Clear all timestamps from the PHY quad block that is shared between the
 * internal PHYs on the E822 devices.
 */
void ice_ptp_reset_ts_memory_quad_e822(struct ice_hw *hw, u8 quad)
{
	ice_write_quad_reg_e822(hw, quad, Q_REG_TS_CTRL, Q_REG_TS_CTRL_M);
	ice_write_quad_reg_e822(hw, quad, Q_REG_TS_CTRL, ~(u32)Q_REG_TS_CTRL_M);
}

/**
 * ice_ptp_reset_ts_memory_e822 - Clear all timestamps from all quad blocks
 * @hw: pointer to the HW struct
 */
static void ice_ptp_reset_ts_memory_e822(struct ice_hw *hw)
{
	u8 quad;

	for (quad = 0; quad < ICE_MAX_QUAD; quad++) {
		ice_ptp_reset_ts_memory_quad_e822(hw, quad);
	}
}

/**
 * ice_ptp_set_vernier_wl - Set the window length for vernier calibration
 * @hw: pointer to the HW struct
 *
 * Set the window length used for the vernier port calibration process.
 */
enum ice_status ice_ptp_set_vernier_wl(struct ice_hw *hw)
{
	u8 port;

	for (port = 0; port < ICE_NUM_EXTERNAL_PORTS; port++) {
		enum ice_status status;


		status = ice_write_phy_reg_e822_lp(hw, port, P_REG_WL,
						   PTP_VERNIER_WL, true);
		if (status) {
			ice_debug(hw, ICE_DBG_PTP, "Failed to set vernier window length for port %u, status %d\n",
				  port, status);
			return status;
		}
	}

	return ICE_SUCCESS;
}

/**
 * ice_ptp_init_phc_e822 - Perform E822 specific PHC initialization
 * @hw: pointer to HW struct
 *
 * Perform PHC initialization steps specific to E822 devices.
 */
static enum ice_status ice_ptp_init_phc_e822(struct ice_hw *hw)
{
#ifdef CGU_SUPPORT
	enum ice_status status;
#endif
	u32 regval;

	/* Enable reading switch and PHY registers over the sideband queue */
#define PF_SB_REM_DEV_CTL_SWITCH_READ BIT(1)
#define PF_SB_REM_DEV_CTL_PHY0 BIT(2)
	regval = rd32(hw, PF_SB_REM_DEV_CTL);
	regval |= (PF_SB_REM_DEV_CTL_SWITCH_READ |
		   PF_SB_REM_DEV_CTL_PHY0);
	wr32(hw, PF_SB_REM_DEV_CTL, regval);

#ifdef CGU_SUPPORT
	/* Initialize the Clock Generation Unit */
	status = ice_init_cgu_e822(hw);
	if (status)
		return status;

	/* Enable CGU error reporting */
	status = ice_ptp_cgu_err_reporting(hw, true);
	if (status)
		return status;
#endif

	/* Set window length for all the ports */
	return ice_ptp_set_vernier_wl(hw);
}

/**
 * ice_ptp_prep_phy_time_e822 - Prepare PHY port with initial time
 * @hw: pointer to the HW struct
 * @time: Time to initialize the PHY port clocks to
 *
 * Program the PHY port registers with a new initial time value. The port
 * clock will be initialized once the driver issues an ICE_PTP_INIT_TIME sync
 * command. The time value is the upper 32 bits of the PHY timer, usually in
 * units of nominal nanoseconds.
 */
static enum ice_status
ice_ptp_prep_phy_time_e822(struct ice_hw *hw, u32 time)
{
	enum ice_status status;
	u64 phy_time;
	u8 port;

	/* The time represents the upper 32 bits of the PHY timer, so we need
	 * to shift to account for this when programming.
	 */
	phy_time = (u64)time << 32;

	for (port = 0; port < ICE_NUM_EXTERNAL_PORTS; port++) {

		/* Tx case */
		status = ice_write_64b_phy_reg_e822(hw, port,
						    P_REG_TX_TIMER_INC_PRE_L,
						    phy_time);
		if (status)
			goto exit_err;

		/* Rx case */
		status = ice_write_64b_phy_reg_e822(hw, port,
						    P_REG_RX_TIMER_INC_PRE_L,
						    phy_time);
		if (status)
			goto exit_err;
	}

	return ICE_SUCCESS;

exit_err:
	ice_debug(hw, ICE_DBG_PTP, "Failed to write init time for port %u, status %d\n",
		  port, status);

	return status;
}

/**
 * ice_ptp_prep_port_adj_e822 - Prepare a single port for time adjust
 * @hw: pointer to HW struct
 * @port: Port number to be programmed
 * @time: time in cycles to adjust the port Tx and Rx clocks
 * @lock_sbq: true to lock the sbq sq_lock (the usual case); false if the
 *            sq_lock has already been locked at a higher level
 *
 * Program the port for an atomic adjustment by writing the Tx and Rx timer
 * registers. The atomic adjustment won't be completed until the driver issues
 * an ICE_PTP_ADJ_TIME command.
 *
 * Note that time is not in units of nanoseconds. It is in clock time
 * including the lower sub-nanosecond portion of the port timer.
 *
 * Negative adjustments are supported using 2s complement arithmetic.
 */
enum ice_status
ice_ptp_prep_port_adj_e822(struct ice_hw *hw, u8 port, s64 time,
			   bool lock_sbq)
{
	enum ice_status status;
	u32 l_time, u_time;

	l_time = ICE_LO_DWORD(time);
	u_time = ICE_HI_DWORD(time);

	/* Tx case */
	status = ice_write_phy_reg_e822_lp(hw, port, P_REG_TX_TIMER_INC_PRE_L,
					   l_time, lock_sbq);
	if (status)
		goto exit_err;

	status = ice_write_phy_reg_e822_lp(hw, port, P_REG_TX_TIMER_INC_PRE_U,
					   u_time, lock_sbq);
	if (status)
		goto exit_err;

	/* Rx case */
	status = ice_write_phy_reg_e822_lp(hw, port, P_REG_RX_TIMER_INC_PRE_L,
					   l_time, lock_sbq);
	if (status)
		goto exit_err;

	status = ice_write_phy_reg_e822_lp(hw, port, P_REG_RX_TIMER_INC_PRE_U,
					   u_time, lock_sbq);
	if (status)
		goto exit_err;

	return ICE_SUCCESS;

exit_err:
	ice_debug(hw, ICE_DBG_PTP, "Failed to write time adjust for port %u, status %d\n",
		  port, status);
	return status;
}

/**
 * ice_ptp_prep_phy_adj_e822 - Prep PHY ports for a time adjustment
 * @hw: pointer to HW struct
 * @adj: adjustment in nanoseconds
 * @lock_sbq: true to lock the sbq sq_lock (the usual case); false if the
 *            sq_lock has already been locked at a higher level
 *
 * Prepare the PHY ports for an atomic time adjustment by programming the PHY
 * Tx and Rx port registers. The actual adjustment is completed by issuing an
 * ICE_PTP_ADJ_TIME or ICE_PTP_ADJ_TIME_AT_TIME sync command.
 */
static enum ice_status
ice_ptp_prep_phy_adj_e822(struct ice_hw *hw, s32 adj, bool lock_sbq)
{
	s64 cycles;
	u8 port;

	/* The port clock supports adjustment of the sub-nanosecond portion of
	 * the clock. We shift the provided adjustment in nanoseconds to
	 * calculate the appropriate adjustment to program into the PHY ports.
	 */
	if (adj > 0)
		cycles = (s64)adj << 32;
	else
		cycles = -(((s64)-adj) << 32);

	for (port = 0; port < ICE_NUM_EXTERNAL_PORTS; port++) {
		enum ice_status status;


		status = ice_ptp_prep_port_adj_e822(hw, port, cycles,
						    lock_sbq);
		if (status)
			return status;
	}

	return ICE_SUCCESS;
}

/**
 * ice_ptp_prep_phy_incval_e822 - Prepare PHY ports for time adjustment
 * @hw: pointer to HW struct
 * @incval: new increment value to prepare
 *
 * Prepare each of the PHY ports for a new increment value by programming the
 * port's TIMETUS registers. The new increment value will be updated after
 * issuing an ICE_PTP_INIT_INCVAL command.
 */
static enum ice_status
ice_ptp_prep_phy_incval_e822(struct ice_hw *hw, u64 incval)
{
	enum ice_status status;
	u8 port;

	for (port = 0; port < ICE_NUM_EXTERNAL_PORTS; port++) {
		status = ice_write_40b_phy_reg_e822(hw, port, P_REG_TIMETUS_L,
						    incval);
		if (status)
			goto exit_err;
	}

	return ICE_SUCCESS;

exit_err:
	ice_debug(hw, ICE_DBG_PTP, "Failed to write incval for port %u, status %d\n",
		  port, status);

	return status;
}

/**
 * ice_ptp_read_phy_incval_e822 - Read a PHY port's current incval
 * @hw: pointer to the HW struct
 * @port: the port to read
 * @incval: on return, the time_clk_cyc incval for this port
 *
 * Read the time_clk_cyc increment value for a given PHY port.
 */
enum ice_status
ice_ptp_read_phy_incval_e822(struct ice_hw *hw, u8 port, u64 *incval)
{
	enum ice_status status;

	status = ice_read_40b_phy_reg_e822(hw, port, P_REG_TIMETUS_L, incval);
	if (status) {
		ice_debug(hw, ICE_DBG_PTP, "Failed to read TIMETUS_L, status %d\n",
			  status);
		return status;
	}

	ice_debug(hw, ICE_DBG_PTP, "read INCVAL = 0x%016llx\n",
		  (unsigned long long)*incval);

	return ICE_SUCCESS;
}

/**
 * ice_ptp_prep_phy_adj_target_e822 - Prepare PHY for adjust at target time
 * @hw: pointer to HW struct
 * @target_time: target time to program
 *
 * Program the PHY port Tx and Rx TIMER_CNT_ADJ registers used for the
 * ICE_PTP_ADJ_TIME_AT_TIME command. This should be used in conjunction with
 * ice_ptp_prep_phy_adj_e822 to program an atomic adjustment that is
 * delayed until a specified target time.
 *
 * Note that a target time adjustment is not currently supported on E810
 * devices.
 */
static enum ice_status
ice_ptp_prep_phy_adj_target_e822(struct ice_hw *hw, u32 target_time)
{
	enum ice_status status;
	u8 port;

	for (port = 0; port < ICE_NUM_EXTERNAL_PORTS; port++) {

		/* Tx case */
		/* No sub-nanoseconds data */
		status = ice_write_phy_reg_e822_lp(hw, port,
						   P_REG_TX_TIMER_CNT_ADJ_L,
						   0, true);
		if (status)
			goto exit_err;

		status = ice_write_phy_reg_e822_lp(hw, port,
						   P_REG_TX_TIMER_CNT_ADJ_U,
						   target_time, true);
		if (status)
			goto exit_err;

		/* Rx case */
		/* No sub-nanoseconds data */
		status = ice_write_phy_reg_e822_lp(hw, port,
						   P_REG_RX_TIMER_CNT_ADJ_L,
						   0, true);
		if (status)
			goto exit_err;

		status = ice_write_phy_reg_e822_lp(hw, port,
						   P_REG_RX_TIMER_CNT_ADJ_U,
						   target_time, true);
		if (status)
			goto exit_err;
	}

	return ICE_SUCCESS;

exit_err:
	ice_debug(hw, ICE_DBG_PTP, "Failed to write target time for port %u, status %d\n",
		  port, status);

	return status;
}

/**
 * ice_ptp_read_port_capture_e822 - Read a port's local time capture
 * @hw: pointer to HW struct
 * @port: Port number to read
 * @tx_ts: on return, the Tx port time capture
 * @rx_ts: on return, the Rx port time capture
 *
 * Read the port's Tx and Rx local time capture values.
 *
 * Note this has no equivalent for the E810 devices.
 */
enum ice_status
ice_ptp_read_port_capture_e822(struct ice_hw *hw, u8 port, u64 *tx_ts,
			       u64 *rx_ts)
{
	enum ice_status status;

	/* Tx case */
	status = ice_read_64b_phy_reg_e822(hw, port, P_REG_TX_CAPTURE_L, tx_ts);
	if (status) {
		ice_debug(hw, ICE_DBG_PTP, "Failed to read REG_TX_CAPTURE, status %d\n",
			  status);
		return status;
	}

	ice_debug(hw, ICE_DBG_PTP, "tx_init = 0x%016llx\n",
		  (unsigned long long)*tx_ts);

	/* Rx case */
	status = ice_read_64b_phy_reg_e822(hw, port, P_REG_RX_CAPTURE_L, rx_ts);
	if (status) {
		ice_debug(hw, ICE_DBG_PTP, "Failed to read RX_CAPTURE, status %d\n",
			  status);
		return status;
	}

	ice_debug(hw, ICE_DBG_PTP, "rx_init = 0x%016llx\n",
		  (unsigned long long)*rx_ts);

	return ICE_SUCCESS;
}

/**
 * ice_ptp_one_port_cmd_e822 - Prepare a single PHY port for a timer command
 * @hw: pointer to HW struct
 * @port: Port to which cmd has to be sent
 * @cmd: Command to be sent to the port
 * @lock_sbq: true if the sideband queue lock must be acquired
 *
 * Prepare the requested port for an upcoming timer sync command.
 *
 * Note there is no equivalent of this operation on E810, as that device
 * always handles all external PHYs internally.
 */
enum ice_status
ice_ptp_one_port_cmd_e822(struct ice_hw *hw, u8 port, enum ice_ptp_tmr_cmd cmd,
			  bool lock_sbq)
{
	enum ice_status status;
	u32 cmd_val, val;
	u8 tmr_idx;

	tmr_idx = ice_get_ptp_src_clock_index(hw);
	cmd_val = tmr_idx << SEL_PHY_SRC;
	switch (cmd) {
	case ICE_PTP_INIT_TIME:
		cmd_val |= PHY_CMD_INIT_TIME;
		break;
	case ICE_PTP_INIT_INCVAL:
		cmd_val |= PHY_CMD_INIT_INCVAL;
		break;
	case ICE_PTP_ADJ_TIME:
		cmd_val |= PHY_CMD_ADJ_TIME;
		break;
	case ICE_PTP_ADJ_TIME_AT_TIME:
		cmd_val |= PHY_CMD_ADJ_TIME_AT_TIME;
		break;
	case ICE_PTP_READ_TIME:
		cmd_val |= PHY_CMD_READ_TIME;
		break;
	default:
		ice_warn(hw, "Unknown timer command %u\n", cmd);
		return ICE_ERR_PARAM;
	}

	/* Tx case */
	/* Read, modify, write */
	status = ice_read_phy_reg_e822_lp(hw, port, P_REG_TX_TMR_CMD, &val,
					  lock_sbq);
	if (status) {
		ice_debug(hw, ICE_DBG_PTP, "Failed to read TX_TMR_CMD, status %d\n",
			  status);
		return status;
	}

	/* Modify necessary bits only and perform write */
	val &= ~TS_CMD_MASK;
	val |= cmd_val;

	status = ice_write_phy_reg_e822_lp(hw, port, P_REG_TX_TMR_CMD, val,
					   lock_sbq);
	if (status) {
		ice_debug(hw, ICE_DBG_PTP, "Failed to write back TX_TMR_CMD, status %d\n",
			  status);
		return status;
	}

	/* Rx case */
	/* Read, modify, write */
	status = ice_read_phy_reg_e822_lp(hw, port, P_REG_RX_TMR_CMD, &val,
					  lock_sbq);
	if (status) {
		ice_debug(hw, ICE_DBG_PTP, "Failed to read RX_TMR_CMD, status %d\n",
			  status);
		return status;
	}

	/* Modify necessary bits only and perform write */
	val &= ~TS_CMD_MASK;
	val |= cmd_val;

	status = ice_write_phy_reg_e822_lp(hw, port, P_REG_RX_TMR_CMD, val,
					   lock_sbq);
	if (status) {
		ice_debug(hw, ICE_DBG_PTP, "Failed to write back RX_TMR_CMD, status %d\n",
			  status);
		return status;
	}

	return ICE_SUCCESS;
}

/**
 * ice_ptp_port_cmd_e822 - Prepare all ports for a timer command
 * @hw: pointer to the HW struct
 * @cmd: timer command to prepare
 * @lock_sbq: true if the sideband queue lock must  be acquired
 *
 * Prepare all ports connected to this device for an upcoming timer sync
 * command.
 */
static enum ice_status
ice_ptp_port_cmd_e822(struct ice_hw *hw, enum ice_ptp_tmr_cmd cmd,
		      bool lock_sbq)
{
	u8 port;

	for (port = 0; port < ICE_NUM_EXTERNAL_PORTS; port++) {
		enum ice_status status;


		status = ice_ptp_one_port_cmd_e822(hw, port, cmd, lock_sbq);
		if (status)
			return status;
	}

	return ICE_SUCCESS;
}

/* E822 Vernier calibration functions
 *
 * The following functions are used as part of the vernier calibration of
 * a port. This calibration increases the precision of the timestamps on the
 * port.
 */

/**
 * ice_phy_get_speed_and_fec_e822 - Get link speed and FEC based on serdes mode
 * @hw: pointer to HW struct
 * @port: the port to read from
 * @link_out: if non-NULL, holds link speed on success
 * @fec_out: if non-NULL, holds FEC algorithm on success
 *
 * Read the serdes data for the PHY port and extract the link speed and FEC
 * algorithm.
 */
enum ice_status
ice_phy_get_speed_and_fec_e822(struct ice_hw *hw, u8 port,
			       enum ice_ptp_link_spd *link_out,
			       enum ice_ptp_fec_mode *fec_out)
{
	enum ice_ptp_link_spd link;
	enum ice_ptp_fec_mode fec;
	enum ice_status status;
	u32 serdes;

	status = ice_read_phy_reg_e822(hw, port, P_REG_LINK_SPEED, &serdes);
	if (status) {
		ice_debug(hw, ICE_DBG_PTP, "Failed to read serdes info\n");
		return status;
	}

	/* Determine the FEC algorithm */
	fec = (enum ice_ptp_fec_mode)P_REG_LINK_SPEED_FEC_MODE(serdes);

	serdes &= P_REG_LINK_SPEED_SERDES_M;

	/* Determine the link speed */
	if (fec == ICE_PTP_FEC_MODE_RS_FEC) {
		switch (serdes) {
		case ICE_PTP_SERDES_25G:
			link = ICE_PTP_LNK_SPD_25G_RS;
			break;
		case ICE_PTP_SERDES_50G:
			link = ICE_PTP_LNK_SPD_50G_RS;
			break;
		case ICE_PTP_SERDES_100G:
			link = ICE_PTP_LNK_SPD_100G_RS;
			break;
		default:
			return ICE_ERR_OUT_OF_RANGE;
		}
	} else {
		switch (serdes) {
		case ICE_PTP_SERDES_1G:
			link = ICE_PTP_LNK_SPD_1G;
			break;
		case ICE_PTP_SERDES_10G:
			link = ICE_PTP_LNK_SPD_10G;
			break;
		case ICE_PTP_SERDES_25G:
			link = ICE_PTP_LNK_SPD_25G;
			break;
		case ICE_PTP_SERDES_40G:
			link = ICE_PTP_LNK_SPD_40G;
			break;
		case ICE_PTP_SERDES_50G:
			link = ICE_PTP_LNK_SPD_50G;
			break;
		default:
			return ICE_ERR_OUT_OF_RANGE;
		}
	}

	if (link_out)
		*link_out = link;
	if (fec_out)
		*fec_out = fec;

	return ICE_SUCCESS;
}

/**
 * ice_phy_cfg_lane_e822 - Configure PHY quad for single/multi-lane timestamp
 * @hw: pointer to HW struct
 * @port: to configure the quad for
 */
void ice_phy_cfg_lane_e822(struct ice_hw *hw, u8 port)
{
	enum ice_ptp_link_spd link_spd;
	enum ice_status status;
	u32 val;
	u8 quad;

	status = ice_phy_get_speed_and_fec_e822(hw, port, &link_spd, NULL);
	if (status) {
		ice_debug(hw, ICE_DBG_PTP, "Failed to get PHY link speed, status %d\n",
			  status);
		return;
	}

	quad = port / ICE_PORTS_PER_QUAD;

	status = ice_read_quad_reg_e822(hw, quad, Q_REG_TX_MEM_GBL_CFG, &val);
	if (status) {
		ice_debug(hw, ICE_DBG_PTP, "Failed to read TX_MEM_GLB_CFG, status %d\n",
			  status);
		return;
	}

	if (link_spd >= ICE_PTP_LNK_SPD_40G)
		val &= ~Q_REG_TX_MEM_GBL_CFG_LANE_TYPE_M;
	else
		val |= Q_REG_TX_MEM_GBL_CFG_LANE_TYPE_M;

	status = ice_write_quad_reg_e822(hw, quad, Q_REG_TX_MEM_GBL_CFG, val);
	if (status) {
		ice_debug(hw, ICE_DBG_PTP, "Failed to write back TX_MEM_GBL_CFG, status %d\n",
			  status);
		return;
	}
}

/**
 * ice_phy_cfg_uix_e822 - Configure Serdes UI to TU conversion for E822
 * @hw: pointer to the HW structure
 * @port: the port to configure
 *
 * Program the conversion ration of Serdes clock "unit intervals" (UIs) to PHC
 * hardware clock time units (TUs). That is, determine the number of TUs per
 * serdes unit interval, and program the UIX registers with this conversion.
 *
 * This conversion is used as part of the calibration process when determining
 * the additional error of a timestamp vs the real time of transmission or
 * receipt of the packet.
 *
 * Hardware uses the number of TUs per 66 UIs, written to the UIX registers
 * for the two main serdes clock rates, 10G/40G and 25G/100G serdes clocks.
 *
 * To calculate the conversion ratio, we use the following facts:
 *
 * a) the clock frequency in Hz (cycles per second)
 * b) the number of TUs per cycle (the increment value of the clock)
 * c) 1 second per 1 billion nanoseconds
 * d) the duration of 66 UIs in nanoseconds
 *
 * Given these facts, we can use the following table to work out what ratios
 * to multiply in order to get the number of TUs per 66 UIs:
 *
 * cycles |   1 second   | incval (TUs) | nanoseconds
 * -------+--------------+--------------+-------------
 * second | 1 billion ns |    cycle     |   66 UIs
 *
 * To perform the multiplication using integers without too much loss of
 * precision, we can take use the following equation:
 *
 * (freq * incval * 6600 LINE_UI ) / ( 100 * 1 billion)
 *
 * We scale up to using 6600 UI instead of 66 in order to avoid fractional
 * nanosecond UIs (66 UI at 10G/40G is 6.4 ns)
 *
 * The increment value has a maximum expected range of about 34 bits, while
 * the frequency value is about 29 bits. Multiplying these values shouldn't
 * overflow the 64 bits. However, we must then further multiply them again by
 * the Serdes unit interval duration. To avoid overflow here, we split the
 * overall divide by 1e11 into a divide by 256 (shift down by 8 bits) and
 * a divide by 390,625,000. This does lose some precision, but avoids
 * miscalculation due to arithmetic overflow.
 */
static enum ice_status ice_phy_cfg_uix_e822(struct ice_hw *hw, u8 port)
{
	u64 cur_freq, clk_incval, tu_per_sec, uix;
	enum ice_status status;

	cur_freq = ice_e822_pll_freq(ice_e822_time_ref(hw));
	clk_incval = ice_ptp_read_src_incval(hw);

	/* Calculate TUs per second divided by 256 */
	tu_per_sec = (cur_freq * clk_incval) >> 8;

#define LINE_UI_10G_40G 640 /* 6600 UIs is 640 nanoseconds at 10Gb/40Gb */
#define LINE_UI_25G_100G 256 /* 6600 UIs is 256 nanoseconds at 25Gb/100Gb */

	/* Program the 10Gb/40Gb conversion ratio */
	uix = DIV_U64(tu_per_sec * LINE_UI_10G_40G, 390625000);

	status = ice_write_64b_phy_reg_e822(hw, port, P_REG_UIX66_10G_40G_L,
					    uix);
	if (status) {
		ice_debug(hw, ICE_DBG_PTP, "Failed to write UIX66_10G_40G, status %d\n",
			  status);
		return status;
	}

	/* Program the 25Gb/100Gb conversion ratio */
	uix = DIV_U64(tu_per_sec * LINE_UI_25G_100G, 390625000);

	status = ice_write_64b_phy_reg_e822(hw, port, P_REG_UIX66_25G_100G_L,
					    uix);
	if (status) {
		ice_debug(hw, ICE_DBG_PTP, "Failed to write UIX66_25G_100G, status %d\n",
			  status);
		return status;
	}

	return ICE_SUCCESS;
}

/**
 * ice_phy_cfg_parpcs_e822 - Configure TUs per PAR/PCS clock cycle
 * @hw: pointer to the HW struct
 * @port: port to configure
 *
 * Configure the number of TUs for the PAR and PCS clocks used as part of the
 * timestamp calibration process. This depends on the link speed, as the PHY
 * uses different markers depending on the speed.
 *
 * 1Gb/10Gb/25Gb:
 * - Tx/Rx PAR/PCS markers
 *
 * 25Gb RS:
 * - Tx/Rx Reed Solomon gearbox PAR/PCS markers
 *
 * 40Gb/50Gb:
 * - Tx/Rx PAR/PCS markers
 * - Rx Deskew PAR/PCS markers
 *
 * 50G RS and 100GB RS:
 * - Tx/Rx Reed Solomon gearbox PAR/PCS markers
 * - Rx Deskew PAR/PCS markers
 * - Tx PAR/PCS markers
 *
 * To calculate the conversion, we use the PHC clock frequency (cycles per
 * second), the increment value (TUs per cycle), and the related PHY clock
 * frequency to calculate the TUs per unit of the PHY link clock. The
 * following table shows how the units convert:
 *
 * cycles |  TUs  | second
 * -------+-------+--------
 * second | cycle | cycles
 *
 * For each conversion register, look up the appropriate frequency from the
 * e822 PAR/PCS table and calculate the TUs per unit of that clock. Program
 * this to the appropriate register, preparing hardware to perform timestamp
 * calibration to calculate the total Tx or Rx offset to adjust the timestamp
 * in order to calibrate for the internal PHY delays.
 *
 * Note that the increment value ranges up to ~34 bits, and the clock
 * frequency is ~29 bits, so multiplying them together should fit within the
 * 64 bit arithmetic.
 */
static enum ice_status ice_phy_cfg_parpcs_e822(struct ice_hw *hw, u8 port)
{
	u64 cur_freq, clk_incval, tu_per_sec, phy_tus;
	enum ice_ptp_link_spd link_spd;
	enum ice_ptp_fec_mode fec_mode;
	enum ice_status status;

	status = ice_phy_get_speed_and_fec_e822(hw, port, &link_spd, &fec_mode);
	if (status)
		return status;

	cur_freq = ice_e822_pll_freq(ice_e822_time_ref(hw));
	clk_incval = ice_ptp_read_src_incval(hw);

	/* Calculate TUs per cycle of the PHC clock */
	tu_per_sec = cur_freq * clk_incval;

	/* For each PHY conversion register, look up the appropriate link
	 * speed frequency and determine the TUs per that clock's cycle time.
	 * Split this into a high and low value and then program the
	 * appropriate register. If that link speed does not use the
	 * associated register, write zeros to clear it instead.
	 */

	/* P_REG_PAR_TX_TUS */
	if (e822_vernier[link_spd].tx_par_clk)
		phy_tus = DIV_U64(tu_per_sec,
				  e822_vernier[link_spd].tx_par_clk);
	else
		phy_tus = 0;

	status = ice_write_40b_phy_reg_e822(hw, port, P_REG_PAR_TX_TUS_L,
					    phy_tus);
	if (status)
		return status;

	/* P_REG_PAR_RX_TUS */
	if (e822_vernier[link_spd].rx_par_clk)
		phy_tus = DIV_U64(tu_per_sec,
				  e822_vernier[link_spd].rx_par_clk);
	else
		phy_tus = 0;

	status = ice_write_40b_phy_reg_e822(hw, port, P_REG_PAR_RX_TUS_L,
					    phy_tus);
	if (status)
		return status;

	/* P_REG_PCS_TX_TUS */
	if (e822_vernier[link_spd].tx_pcs_clk)
		phy_tus = DIV_U64(tu_per_sec,
				  e822_vernier[link_spd].tx_pcs_clk);
	else
		phy_tus = 0;

	status = ice_write_40b_phy_reg_e822(hw, port, P_REG_PCS_TX_TUS_L,
					    phy_tus);
	if (status)
		return status;

	/* P_REG_PCS_RX_TUS */
	if (e822_vernier[link_spd].rx_pcs_clk)
		phy_tus = DIV_U64(tu_per_sec,
				  e822_vernier[link_spd].rx_pcs_clk);
	else
		phy_tus = 0;

	status = ice_write_40b_phy_reg_e822(hw, port, P_REG_PCS_RX_TUS_L,
					    phy_tus);
	if (status)
		return status;

	/* P_REG_DESK_PAR_TX_TUS */
	if (e822_vernier[link_spd].tx_desk_rsgb_par)
		phy_tus = DIV_U64(tu_per_sec,
				  e822_vernier[link_spd].tx_desk_rsgb_par);
	else
		phy_tus = 0;

	status = ice_write_40b_phy_reg_e822(hw, port, P_REG_DESK_PAR_TX_TUS_L,
					    phy_tus);
	if (status)
		return status;

	/* P_REG_DESK_PAR_RX_TUS */
	if (e822_vernier[link_spd].rx_desk_rsgb_par)
		phy_tus = DIV_U64(tu_per_sec,
				  e822_vernier[link_spd].rx_desk_rsgb_par);
	else
		phy_tus = 0;

	status = ice_write_40b_phy_reg_e822(hw, port, P_REG_DESK_PAR_RX_TUS_L,
					    phy_tus);
	if (status)
		return status;

	/* P_REG_DESK_PCS_TX_TUS */
	if (e822_vernier[link_spd].tx_desk_rsgb_pcs)
		phy_tus = DIV_U64(tu_per_sec,
				  e822_vernier[link_spd].tx_desk_rsgb_pcs);
	else
		phy_tus = 0;

	status = ice_write_40b_phy_reg_e822(hw, port, P_REG_DESK_PCS_TX_TUS_L,
					    phy_tus);
	if (status)
		return status;

	/* P_REG_DESK_PCS_RX_TUS */
	if (e822_vernier[link_spd].rx_desk_rsgb_pcs)
		phy_tus = DIV_U64(tu_per_sec,
				  e822_vernier[link_spd].rx_desk_rsgb_pcs);
	else
		phy_tus = 0;

	return ice_write_40b_phy_reg_e822(hw, port, P_REG_DESK_PCS_RX_TUS_L,
					  phy_tus);
}

/**
 * ice_calc_fixed_tx_offset_e822 - Calculated Fixed Tx offset for a port
 * @hw: pointer to the HW struct
 * @link_spd: the Link speed to calculate for
 *
 * Calculate the fixed offset due to known static latency data.
 */
static u64
ice_calc_fixed_tx_offset_e822(struct ice_hw *hw, enum ice_ptp_link_spd link_spd)
{
	u64 cur_freq, clk_incval, tu_per_sec, fixed_offset;

	cur_freq = ice_e822_pll_freq(ice_e822_time_ref(hw));
	clk_incval = ice_ptp_read_src_incval(hw);

	/* Calculate TUs per second */
	tu_per_sec = cur_freq * clk_incval;

	/* Calculate number of TUs to add for the fixed Tx latency. Since the
	 * latency measurement is in 1/100th of a nanosecond, we need to
	 * multiply by tu_per_sec and then divide by 1e11. This calculation
	 * overflows 64 bit integer arithmetic, so break it up into two
	 * divisions by 1e4 first then by 1e7.
	 */
	fixed_offset = DIV_U64(tu_per_sec, 10000);
	fixed_offset *= e822_vernier[link_spd].tx_fixed_delay;
	fixed_offset = DIV_U64(fixed_offset, 10000000);

	return fixed_offset;
}

/**
 * ice_phy_cfg_tx_offset_e822 - Configure total Tx timestamp offset
 * @hw: pointer to the HW struct
 * @port: the PHY port to configure
 *
 * Program the P_REG_TOTAL_TX_OFFSET register with the total number of TUs to
 * adjust Tx timestamps by. This is calculated by combining some known static
 * latency along with the Vernier offset computations done by hardware.
 *
 * This function will not return successfully until the Tx offset calculations
 * have been completed, which requires waiting until at least one packet has
 * been transmitted by the device. It is safe to call this function
 * periodically until calibration succeeds, as it will only program the offset
 * once.
 *
 * To avoid overflow, when calculating the offset based on the known static
 * latency values, we use measurements in 1/100th of a nanosecond, and divide
 * the TUs per second up front. This avoids overflow while allowing
 * calculation of the adjustment using integer arithmetic.
 *
 * Returns zero on success, ICE_ERR_NOT_READY if the hardware vernier offset
 * calibration has not completed, or another error code on failure.
 */
enum ice_status ice_phy_cfg_tx_offset_e822(struct ice_hw *hw, u8 port)
{
	enum ice_ptp_link_spd link_spd;
	enum ice_ptp_fec_mode fec_mode;
	enum ice_status status;
	u64 total_offset, val;
	u32 reg;

	/* Nothing to do if we've already programmed the offset */
	status = ice_read_phy_reg_e822(hw, port, P_REG_TX_OR, &reg);
	if (status) {
		ice_debug(hw, ICE_DBG_PTP, "Failed to read TX_OR for port %u, status %d\n",
			  port, status);
		return status;
	}

	if (reg)
		return ICE_SUCCESS;

	status = ice_read_phy_reg_e822(hw, port, P_REG_TX_OV_STATUS, &reg);
	if (status) {
		ice_debug(hw, ICE_DBG_PTP, "Failed to read TX_OV_STATUS for port %u, status %d\n",
			  port, status);
		return status;
	}

	if (!(reg & P_REG_TX_OV_STATUS_OV_M))
		return ICE_ERR_NOT_READY;

	status = ice_phy_get_speed_and_fec_e822(hw, port, &link_spd, &fec_mode);
	if (status)
		return status;

	total_offset = ice_calc_fixed_tx_offset_e822(hw, link_spd);

	/* Read the first Vernier offset from the PHY register and add it to
	 * the total offset.
	 */
	if (link_spd == ICE_PTP_LNK_SPD_1G ||
	    link_spd == ICE_PTP_LNK_SPD_10G ||
	    link_spd == ICE_PTP_LNK_SPD_25G ||
	    link_spd == ICE_PTP_LNK_SPD_25G_RS ||
	    link_spd == ICE_PTP_LNK_SPD_40G ||
	    link_spd == ICE_PTP_LNK_SPD_50G) {
		status = ice_read_64b_phy_reg_e822(hw, port,
						   P_REG_PAR_PCS_TX_OFFSET_L,
						   &val);
		if (status)
			return status;

		total_offset += val;
	}

	/* For Tx, we only need to use the second Vernier offset for
	 * multi-lane link speeds with RS-FEC. The lanes will always be
	 * aligned.
	 */
	if (link_spd == ICE_PTP_LNK_SPD_50G_RS ||
	    link_spd == ICE_PTP_LNK_SPD_100G_RS) {
		status = ice_read_64b_phy_reg_e822(hw, port,
						   P_REG_PAR_TX_TIME_L,
						   &val);
		if (status)
			return status;

		total_offset += val;
	}

	/* Now that the total offset has been calculated, program it to the
	 * PHY and indicate that the Tx offset is ready. After this,
	 * timestamps will be enabled.
	 */
	status = ice_write_64b_phy_reg_e822(hw, port, P_REG_TOTAL_TX_OFFSET_L,
					    total_offset);
	if (status)
		return status;

	status = ice_write_phy_reg_e822(hw, port, P_REG_TX_OR, 1);
	if (status)
		return status;

	ice_info(hw, "Port=%d Tx vernier offset calibration complete\n", port);

	return ICE_SUCCESS;
}

/**
 * ice_phy_calc_pmd_adj_e822 - Calculate PMD adjustment for Rx
 * @hw: pointer to the HW struct
 * @port: the PHY port to adjust for
 * @link_spd: the current link speed of the PHY
 * @fec_mode: the current FEC mode of the PHY
 * @pmd_adj: on return, the amount to adjust the Rx total offset by
 *
 * Calculates the adjustment to Rx timestamps due to PMD alignment in the PHY.
 * This varies by link speed and FEC mode. The value calculated accounts for
 * various delays caused when receiving a packet.
 */
static enum ice_status
ice_phy_calc_pmd_adj_e822(struct ice_hw *hw, u8 port,
			  enum ice_ptp_link_spd link_spd,
			  enum ice_ptp_fec_mode fec_mode, u64 *pmd_adj)
{
	u64 cur_freq, clk_incval, tu_per_sec, mult, adj;
	u32 pmd_adj_divisor, val;
	enum ice_status status;
	u8 pmd_align;

	status = ice_read_phy_reg_e822(hw, port, P_REG_PMD_ALIGNMENT, &val);
	if (status) {
		ice_debug(hw, ICE_DBG_PTP, "Failed to read PMD alignment, status %d\n",
			  status);
		return status;
	}

	pmd_align = (u8)val;

	cur_freq = ice_e822_pll_freq(ice_e822_time_ref(hw));
	clk_incval = ice_ptp_read_src_incval(hw);

	/* Calculate TUs per second */
	tu_per_sec = cur_freq * clk_incval;

	/* Get the link speed dependent PMD adjustment divisor */
	pmd_adj_divisor = e822_vernier[link_spd].pmd_adj_divisor;

	/* The PMD alignment adjustment measurement depends on the link speed,
	 * and whether FEC is enabled. For each link speed, the alignment
	 * adjustment is calculated by dividing a value by the length of
	 * a Time Unit in nanoseconds.
	 *
	 * 1G: align == 4 ? 10 * 0.8 : (align + 6 % 10) * 0.8
	 * 10G: align == 65 ? 0 : (align * 0.1 * 32/33)
	 * 10G w/FEC: align * 0.1 * 32/33
	 * 25G: align == 65 ? 0 : (align * 0.4 * 32/33)
	 * 25G w/FEC: align * 0.4 * 32/33
	 * 40G: align == 65 ? 0 : (align * 0.1 * 32/33)
	 * 40G w/FEC: align * 0.1 * 32/33
	 * 50G: align == 65 ? 0 : (align * 0.4 * 32/33)
	 * 50G w/FEC: align * 0.8 * 32/33
	 *
	 * For RS-FEC, if align is < 17 then we must also add 1.6 * 32/33.
	 *
	 * To allow for calculating this value using integer arithmetic, we
	 * instead start with the number of TUs per second, (inverse of the
	 * length of a Time Unit in nanoseconds), multiply by a value based
	 * on the PMD alignment register, and then divide by the right value
	 * calculated based on the table above. To avoid integer overflow this
	 * division is broken up into a step of dividing by 125 first.
	 */
	if (link_spd == ICE_PTP_LNK_SPD_1G) {
		if (pmd_align == 4)
			mult = 10;
		else
			mult = (pmd_align + 6) % 10;
	} else if (link_spd == ICE_PTP_LNK_SPD_10G ||
		   link_spd == ICE_PTP_LNK_SPD_25G ||
		   link_spd == ICE_PTP_LNK_SPD_40G ||
		   link_spd == ICE_PTP_LNK_SPD_50G) {
		/* If Clause 74 FEC, always calculate PMD adjust */
		if (pmd_align != 65 || fec_mode == ICE_PTP_FEC_MODE_CLAUSE74)
			mult = pmd_align;
		else
			mult = 0;
	} else if (link_spd == ICE_PTP_LNK_SPD_25G_RS ||
		   link_spd == ICE_PTP_LNK_SPD_50G_RS ||
		   link_spd == ICE_PTP_LNK_SPD_100G_RS) {
		if (pmd_align < 17)
			mult = pmd_align + 40;
		else
			mult = pmd_align;
	} else {
		ice_debug(hw, ICE_DBG_PTP, "Unknown link speed %d, skipping PMD adjustment\n",
			  link_spd);
		mult = 0;
	}

	/* In some cases, there's no need to adjust for the PMD alignment */
	if (!mult) {
		*pmd_adj = 0;
		return ICE_SUCCESS;
	}

	/* Calculate the adjustment by multiplying TUs per second by the
	 * appropriate multiplier and divisor. To avoid overflow, we first
	 * divide by 125, and then handle remaining divisor based on the link
	 * speed pmd_adj_divisor value.
	 */
	adj = DIV_U64(tu_per_sec, 125);
	adj *= mult;
	adj = DIV_U64(adj, pmd_adj_divisor);

	/* Finally, for 25G-RS and 50G-RS, a further adjustment for the Rx
	 * cycle count is necessary.
	 */
	if (link_spd == ICE_PTP_LNK_SPD_25G_RS) {
		u64 cycle_adj;
		u8 rx_cycle;

		status = ice_read_phy_reg_e822(hw, port, P_REG_RX_40_TO_160_CNT,
					       &val);
		if (status) {
			ice_debug(hw, ICE_DBG_PTP, "Failed to read 25G-RS Rx cycle count, status %d\n",
				  status);
			return status;
		}

		rx_cycle = val & P_REG_RX_40_TO_160_CNT_RXCYC_M;
		if (rx_cycle) {
			mult = (4 - rx_cycle) * 40;

			cycle_adj = DIV_U64(tu_per_sec, 125);
			cycle_adj *= mult;
			cycle_adj = DIV_U64(cycle_adj, pmd_adj_divisor);

			adj += cycle_adj;
		}
	} else if (link_spd == ICE_PTP_LNK_SPD_50G_RS) {
		u64 cycle_adj;
		u8 rx_cycle;

		status = ice_read_phy_reg_e822(hw, port, P_REG_RX_80_TO_160_CNT,
					       &val);
		if (status) {
			ice_debug(hw, ICE_DBG_PTP, "Failed to read 50G-RS Rx cycle count, status %d\n",
				  status);
			return status;
		}

		rx_cycle = val & P_REG_RX_80_TO_160_CNT_RXCYC_M;
		if (rx_cycle) {
			mult = rx_cycle * 40;

			cycle_adj = DIV_U64(tu_per_sec, 125);
			cycle_adj *= mult;
			cycle_adj = DIV_U64(cycle_adj, pmd_adj_divisor);

			adj += cycle_adj;
		}
	}

	/* Return the calculated adjustment */
	*pmd_adj = adj;

	return ICE_SUCCESS;
}

/**
 * ice_calc_fixed_rx_offset_e822 - Calculated the fixed Rx offset for a port
 * @hw: pointer to HW struct
 * @link_spd: The Link speed to calculate for
 *
 * Determine the fixed Rx latency for a given link speed.
 */
static u64
ice_calc_fixed_rx_offset_e822(struct ice_hw *hw, enum ice_ptp_link_spd link_spd)
{
	u64 cur_freq, clk_incval, tu_per_sec, fixed_offset;

	cur_freq = ice_e822_pll_freq(ice_e822_time_ref(hw));
	clk_incval = ice_ptp_read_src_incval(hw);

	/* Calculate TUs per second */
	tu_per_sec = cur_freq * clk_incval;

	/* Calculate number of TUs to add for the fixed Rx latency. Since the
	 * latency measurement is in 1/100th of a nanosecond, we need to
	 * multiply by tu_per_sec and then divide by 1e11. This calculation
	 * overflows 64 bit integer arithmetic, so break it up into two
	 * divisions by 1e4 first then by 1e7.
	 */
	fixed_offset = DIV_U64(tu_per_sec, 10000);
	fixed_offset *= e822_vernier[link_spd].rx_fixed_delay;
	fixed_offset = DIV_U64(fixed_offset, 10000000);

	return fixed_offset;
}

/**
 * ice_phy_cfg_rx_offset_e822 - Configure total Rx timestamp offset
 * @hw: pointer to the HW struct
 * @port: the PHY port to configure
 *
 * Program the P_REG_TOTAL_RX_OFFSET register with the number of Time Units to
 * adjust Rx timestamps by. This combines calculations from the Vernier offset
 * measurements taken in hardware with some data about known fixed delay as
 * well as adjusting for multi-lane alignment delay.
 *
 * This function will not return successfully until the Rx offset calculations
 * have been completed, which requires waiting until at least one packet has
 * been received by the device. It is safe to call this function periodically
 * until calibration succeeds, as it will only program the offset once.
 *
 * This function must be called only after the offset registers are valid,
 * i.e. after the Vernier calibration wait has passed, to ensure that the PHY
 * has measured the offset.
 *
 * To avoid overflow, when calculating the offset based on the known static
 * latency values, we use measurements in 1/100th of a nanosecond, and divide
 * the TUs per second up front. This avoids overflow while allowing
 * calculation of the adjustment using integer arithmetic.
 *
 * Returns zero on success, ICE_ERR_NOT_READY if the hardware vernier offset
 * calibration has not completed, or another error code on failure.
 */
enum ice_status ice_phy_cfg_rx_offset_e822(struct ice_hw *hw, u8 port)
{
	enum ice_ptp_link_spd link_spd;
	enum ice_ptp_fec_mode fec_mode;
	u64 total_offset, pmd, val;
	enum ice_status status;
	u32 reg;

	/* Nothing to do if we've already programmed the offset */
	status = ice_read_phy_reg_e822(hw, port, P_REG_RX_OR, &reg);
	if (status) {
		ice_debug(hw, ICE_DBG_PTP, "Failed to read RX_OR for port %u, status %d\n",
			  port, status);
		return status;
	}

	if (reg)
		return ICE_SUCCESS;

	status = ice_read_phy_reg_e822(hw, port, P_REG_RX_OV_STATUS, &reg);
	if (status) {
		ice_debug(hw, ICE_DBG_PTP, "Failed to read RX_OV_STATUS for port %u, status %d\n",
			  port, status);
		return status;
	}

	if (!(reg & P_REG_RX_OV_STATUS_OV_M))
		return ICE_ERR_NOT_READY;

	status = ice_phy_get_speed_and_fec_e822(hw, port, &link_spd, &fec_mode);
	if (status)
		return status;

	total_offset = ice_calc_fixed_rx_offset_e822(hw, link_spd);

	/* Read the first Vernier offset from the PHY register and add it to
	 * the total offset.
	 */
	status = ice_read_64b_phy_reg_e822(hw, port,
					   P_REG_PAR_PCS_RX_OFFSET_L,
					   &val);
	if (status)
		return status;

	total_offset += val;

	/* For Rx, all multi-lane link speeds include a second Vernier
	 * calibration, because the lanes might not be aligned.
	 */
	if (link_spd == ICE_PTP_LNK_SPD_40G ||
	    link_spd == ICE_PTP_LNK_SPD_50G ||
	    link_spd == ICE_PTP_LNK_SPD_50G_RS ||
	    link_spd == ICE_PTP_LNK_SPD_100G_RS) {
		status = ice_read_64b_phy_reg_e822(hw, port,
						   P_REG_PAR_RX_TIME_L,
						   &val);
		if (status)
			return status;

		total_offset += val;
	}

	/* In addition, Rx must account for the PMD alignment */
	status = ice_phy_calc_pmd_adj_e822(hw, port, link_spd, fec_mode, &pmd);
	if (status)
		return status;

	/* For RS-FEC, this adjustment adds delay, but for other modes, it
	 * subtracts delay.
	 */
	if (fec_mode == ICE_PTP_FEC_MODE_RS_FEC)
		total_offset += pmd;
	else
		total_offset -= pmd;

	/* Now that the total offset has been calculated, program it to the
	 * PHY and indicate that the Rx offset is ready. After this,
	 * timestamps will be enabled.
	 */
	status = ice_write_64b_phy_reg_e822(hw, port, P_REG_TOTAL_RX_OFFSET_L,
					    total_offset);
	if (status)
		return status;

	status = ice_write_phy_reg_e822(hw, port, P_REG_RX_OR, 1);
	if (status)
		return status;

	ice_info(hw, "Port=%d Rx vernier offset calibration complete\n", port);

	return ICE_SUCCESS;
}

/**
 * ice_ptp_clear_phy_offset_ready_e822 - Clear PHY TX_/RX_OFFSET_READY registers
 * @hw: pointer to the HW struct
 *
 * Clear PHY TX_/RX_OFFSET_READY registers, effectively marking all transmitted
 * and received timestamps as invalid.
 */
static enum ice_status ice_ptp_clear_phy_offset_ready_e822(struct ice_hw *hw)
{
	enum ice_status status = ICE_SUCCESS;
	u8 port;

	for (port = 0; port < ICE_NUM_EXTERNAL_PORTS; port++) {
		status = ice_write_phy_reg_e822(hw, port, P_REG_TX_OR, 0);
		if (status) {
			ice_warn(hw, "Failed to clear PHY TX_OFFSET_READY register\n");
			break;
		}

		status = ice_write_phy_reg_e822(hw, port, P_REG_RX_OR, 0);
		if (status) {
			ice_warn(hw, "Failed to clear PHY RX_OFFSET_READY register\n");
			break;
		}
	}

	return status;
}

/**
 * ice_read_phy_and_phc_time_e822 - Simultaneously capture PHC and PHY time
 * @hw: pointer to the HW struct
 * @port: the PHY port to read
 * @phy_time: on return, the 64bit PHY timer value
 * @phc_time: on return, the lower 64bits of PHC time
 *
 * Issue a ICE_PTP_READ_TIME timer command to simultaneously capture the PHY
 * and PHC timer values.
 */
#ifndef SIMICS_SUPPORT
static enum ice_status
ice_read_phy_and_phc_time_e822(struct ice_hw *hw, u8 port, u64 *phy_time,
			       u64 *phc_time)
#else /* SIMICS_SUPPORT */
enum ice_status
ice_read_phy_and_phc_time_e822(struct ice_hw *hw, u8 port, u64 *phy_time_rx,
			       u64 *phy_time_tx,  u64 *phc_time)
#endif /* SIMICS_SUPPORT */
{
	enum ice_status status;
	u64 tx_time, rx_time;
	u32 zo, lo;
	u8 tmr_idx;

	tmr_idx = ice_get_ptp_src_clock_index(hw);

	/* Prepare the PHC timer for a ICE_PTP_READ_TIME capture command */
	ice_ptp_src_cmd(hw, ICE_PTP_READ_TIME);

	/* Prepare the PHY timer for a ICE_PTP_READ_TIME capture command */
	status = ice_ptp_one_port_cmd_e822(hw, port, ICE_PTP_READ_TIME, true);
	if (status)
		return status;

	/* Issue the sync to start the ICE_PTP_READ_TIME capture */
	ice_ptp_exec_tmr_cmd(hw);
	ice_ptp_clean_cmd(hw);

	/* Read the captured PHC time from the shadow time registers */
	zo = rd32(hw, GLTSYN_SHTIME_0(tmr_idx));
	lo = rd32(hw, GLTSYN_SHTIME_L(tmr_idx));
	*phc_time = (u64)lo << 32 | zo;

	/* Read the captured PHY time from the PHY shadow registers */
	status = ice_ptp_read_port_capture_e822(hw, port, &tx_time, &rx_time);
	if (status)
		return status;

	/* If the PHY Tx and Rx timers don't match, log a warning message.
	 * Note that this should not happen in normal circumstances since the
	 * driver always programs them together.
	 */
	if (tx_time != rx_time)
		ice_warn(hw, "PHY port %u Tx and Rx timers do not match, tx_time 0x%016llX, rx_time 0x%016llX\n",
			 port, (unsigned long long)tx_time,
			 (unsigned long long)rx_time);

#ifdef SIMICS_SUPPORT
	*phy_time_tx = tx_time;
	*phy_time_rx = tx_time;
#else /* SIMICS_SUPPORT */
	*phy_time = tx_time;
#endif /* SIMICS_SUPPORT */

	return ICE_SUCCESS;
}

/**
 * ice_sync_phy_timer_e822 - Synchronize the PHY timer with PHC timer
 * @hw: pointer to the HW struct
 * @port: the PHY port to synchronize
 *
 * Perform an adjustment to ensure that the PHY and PHC timers are in sync.
 * This is done by issuing a ICE_PTP_READ_TIME command which triggers a
 * simultaneous read of the PHY timer and PHC timer. Then we use the
 * difference to calculate an appropriate 2s complement addition to add
 * to the PHY timer in order to ensure it reads the same value as the
 * primary PHC timer.
 */
static enum ice_status ice_sync_phy_timer_e822(struct ice_hw *hw, u8 port)
{
#ifdef SIMICS_SUPPORT
	u64 phc_time, phy_time_rx, phy_time_tx, difference;
#else /* SIMICS_SUPPORT */
	u64 phc_time, phy_time, difference;
#endif /* SIMICS_SUPPORT */
	enum ice_status status;

	if (!ice_ptp_lock(hw)) {
		ice_debug(hw, ICE_DBG_PTP, "Failed to acquire PTP semaphore\n");
		return ICE_ERR_NOT_READY;
	}

#ifdef SIMICS_SUPPORT
	status = ice_read_phy_and_phc_time_e822(hw, port, &phy_time_rx,
						&phy_time_tx, &phc_time);
#else /* SIMICS_SUPPORT */
	status = ice_read_phy_and_phc_time_e822(hw, port, &phy_time,
						&phc_time);
#endif /* SIMICS_SUPPORT */
	if (status)
		goto err_unlock;

	/* Calculate the amount required to add to the port time in order for
	 * it to match the PHC time.
	 *
	 * Note that the port adjustment is done using 2s complement
	 * arithmetic. This is convenient since it means that we can simply
	 * calculate the difference between the PHC time and the port time,
	 * and it will be interpreted correctly.
	 */
#ifdef SIMICS_SUPPORT
	difference = phc_time - phy_time_tx;
#else /* SIMICS_SUPPORT */
	difference = phc_time - phy_time;
#endif /* SIMICS_SUPPORT */

	status = ice_ptp_prep_port_adj_e822(hw, port, (s64)difference, true);
	if (status)
		goto err_unlock;

	status = ice_ptp_one_port_cmd_e822(hw, port, ICE_PTP_ADJ_TIME, true);
	if (status)
		goto err_unlock;

	/* Init PHC mstr/src cmd for exec during sync */
	ice_ptp_src_cmd(hw, ICE_PTP_READ_TIME);

	/* Issue the sync to activate the time adjustment */
	ice_ptp_exec_tmr_cmd(hw);
	ice_ptp_clean_cmd(hw);

	/* Re-capture the timer values to flush the command registers and
	 * verify that the time was properly adjusted.
	 */
#ifdef SIMICS_SUPPORT
	status = ice_read_phy_and_phc_time_e822(hw, port, &phy_time_rx,
						&phy_time_tx, &phc_time);
#else /* SIMICS_SUPPORT */
	status = ice_read_phy_and_phc_time_e822(hw, port, &phy_time, &phc_time);
#endif /* SIMICS_SUPPORT */
	if (status)
		goto err_unlock;

#ifdef SIMICS_SUPPORT
	ice_info(hw, "Port %u PHY time synced to PHC: 0x%016llX/0x%016llX, 0x%016llX\n",
		 port, phy_time_rx, phy_time_tx, phc_time);
#else /* SIMICS_SUPPORT */
	ice_info(hw, "Port %u PHY time synced to PHC: 0x%016llX, 0x%016llX\n",
		 port, (unsigned long long)phy_time,
		 (unsigned long long)phc_time);
#endif /* SIMICS_SUPPORT */

	ice_ptp_unlock(hw);

	return ICE_SUCCESS;

err_unlock:
	ice_ptp_unlock(hw);
	return status;
}

/**
 * ice_stop_phy_timer_e822 - Stop the PHY clock timer
 * @hw: pointer to the HW struct
 * @port: the PHY port to stop
 * @soft_reset: if true, hold the SOFT_RESET bit of P_REG_PS
 *
 * Stop the clock of a PHY port. This must be done as part of the flow to
 * re-calibrate Tx and Rx timestamping offsets whenever the clock time is
 * initialized or when link speed changes.
 */
enum ice_status
ice_stop_phy_timer_e822(struct ice_hw *hw, u8 port, bool soft_reset)
{
	enum ice_status status;
	u32 val;

	status = ice_write_phy_reg_e822(hw, port, P_REG_TX_OR, 0);
	if (status)
		return status;

	status = ice_write_phy_reg_e822(hw, port, P_REG_RX_OR, 0);
	if (status)
		return status;

	status = ice_read_phy_reg_e822(hw, port, P_REG_PS, &val);
	if (status)
		return status;

	val &= ~P_REG_PS_START_M;
	status = ice_write_phy_reg_e822(hw, port, P_REG_PS, val);
	if (status)
		return status;

	val &= ~P_REG_PS_ENA_CLK_M;
	status = ice_write_phy_reg_e822(hw, port, P_REG_PS, val);
	if (status)
		return status;

	if (soft_reset) {
		val |= P_REG_PS_SFT_RESET_M;
		status = ice_write_phy_reg_e822(hw, port, P_REG_PS, val);
		if (status)
			return status;
	}

	ice_debug(hw, ICE_DBG_PTP, "Disabled clock on PHY port %u\n", port);

	return ICE_SUCCESS;
}

/**
 * ice_start_phy_timer_e822 - Start the PHY clock timer
 * @hw: pointer to the HW struct
 * @port: the PHY port to start
 *
 * Start the clock of a PHY port. This must be done as part of the flow to
 * re-calibrate Tx and Rx timestamping offsets whenever the clock time is
 * initialized or when link speed changes.
 *
 * Hardware will take Vernier measurements on Tx or Rx of packets.
 */
enum ice_status
ice_start_phy_timer_e822(struct ice_hw *hw, u8 port)
{
	enum ice_status status;
	u32 lo, hi, val;
	u64 incval;
	u8 tmr_idx;

	ice_ptp_clean_cmd(hw);
	tmr_idx = ice_get_ptp_src_clock_index(hw);

	status = ice_stop_phy_timer_e822(hw, port, false);
	if (status)
		return status;

	ice_phy_cfg_lane_e822(hw, port);

	status = ice_phy_cfg_uix_e822(hw, port);
	if (status)
		return status;

	status = ice_phy_cfg_parpcs_e822(hw, port);
	if (status)
		return status;

	lo = rd32(hw, GLTSYN_INCVAL_L(tmr_idx));
	hi = rd32(hw, GLTSYN_INCVAL_H(tmr_idx));
	incval = (u64)hi << 32 | lo;

	status = ice_write_40b_phy_reg_e822(hw, port, P_REG_TIMETUS_L, incval);
	if (status)
		return status;

	status = ice_ptp_one_port_cmd_e822(hw, port, ICE_PTP_INIT_INCVAL, true);
	if (status)
		return status;

	/* Init PHC mstr/src cmd for exec during sync */
	ice_ptp_src_cmd(hw, ICE_PTP_READ_TIME);

	ice_ptp_exec_tmr_cmd(hw);

	status = ice_read_phy_reg_e822(hw, port, P_REG_PS, &val);
	if (status)
		return status;

	val |= P_REG_PS_SFT_RESET_M;
	status = ice_write_phy_reg_e822(hw, port, P_REG_PS, val);
	if (status)
		return status;

	val |= P_REG_PS_START_M;
	status = ice_write_phy_reg_e822(hw, port, P_REG_PS, val);
	if (status)
		return status;

	val &= ~P_REG_PS_SFT_RESET_M;
	status = ice_write_phy_reg_e822(hw, port, P_REG_PS, val);
	if (status)
		return status;

	status = ice_ptp_one_port_cmd_e822(hw, port, ICE_PTP_INIT_INCVAL, true);
	if (status)
		return status;

	ice_ptp_exec_tmr_cmd(hw);

	val |= P_REG_PS_ENA_CLK_M;
	status = ice_write_phy_reg_e822(hw, port, P_REG_PS, val);
	if (status)
		return status;

	val |= P_REG_PS_LOAD_OFFSET_M;
	status = ice_write_phy_reg_e822(hw, port, P_REG_PS, val);
	if (status)
		return status;

	ice_ptp_exec_tmr_cmd(hw);

	status = ice_sync_phy_timer_e822(hw, port);
	if (status)
		return status;

	ice_debug(hw, ICE_DBG_PTP, "Enabled clock on PHY port %u\n", port);

	return ICE_SUCCESS;
}

/**
 * ice_get_phy_tx_tstamp_ready_e822 - Read Tx memory status register
 * @hw: pointer to the HW struct
 * @quad: the timestamp quad to read from
 * @tstamp_ready: contents of the Tx memory status register
 *
 * Read the Q_REG_TX_MEMORY_STATUS register indicating which timestamps in
 * the PHY are ready. A set bit means the corresponding timestamp is valid and
 * ready to be captured from the PHY timestamp block.
 */
static enum ice_status
ice_get_phy_tx_tstamp_ready_e822(struct ice_hw *hw, u8 quad, u64 *tstamp_ready)
{
	enum ice_status status;
	u32 hi, lo;

	status = ice_read_quad_reg_e822(hw, quad, Q_REG_TX_MEMORY_STATUS_U,
					&hi);
	if (status) {
		ice_debug(hw, ICE_DBG_PTP, "Failed to read TX_MEMORY_STATUS_U for quad %u, status %d\n",
			  quad, status);
		return status;
	}

	status = ice_read_quad_reg_e822(hw, quad, Q_REG_TX_MEMORY_STATUS_L,
					&lo);
	if (status) {
		ice_debug(hw, ICE_DBG_PTP, "Failed to read TX_MEMORY_STATUS_L for quad %u, status %d\n",
			  quad, status);
		return status;
	}

	*tstamp_ready = (u64)hi << 32 | (u64)lo;

	return ICE_SUCCESS;
}

/**
 * ice_phy_cfg_intr_e822 - Configure TX timestamp interrupt
 * @hw: pointer to the HW struct
 * @quad: the timestamp quad
 * @ena: enable or disable interrupt
 * @threshold: interrupt threshold
 *
 * Configure TX timestamp interrupt for the specified quad
 */

enum ice_status
ice_phy_cfg_intr_e822(struct ice_hw *hw, u8 quad, bool ena, u8 threshold)
{
	enum ice_status err;
	u32 val;

	err = ice_read_quad_reg_e822(hw, quad,
				     Q_REG_TX_MEM_GBL_CFG,
				     &val);
	if (err)
		return err;

	if (ena) {
		val |= Q_REG_TX_MEM_GBL_CFG_INTR_ENA_M;
		val &= ~Q_REG_TX_MEM_GBL_CFG_INTR_THR_M;
		val |= ((threshold << Q_REG_TX_MEM_GBL_CFG_INTR_THR_S) &
			Q_REG_TX_MEM_GBL_CFG_INTR_THR_M);
	} else {
		val &= ~Q_REG_TX_MEM_GBL_CFG_INTR_ENA_M;
	}

	err = ice_write_quad_reg_e822(hw, quad,
				      Q_REG_TX_MEM_GBL_CFG,
				      val);

	return err;
}

#if defined(E810C_SUPPORT) || defined(E810_XXV_SUPPORT)
/* E810 functions
 *
 * The following functions operate on the E810 series devices which use
 * a separate external PHY.
 */

/**
 * ice_read_phy_reg_e810_lp - Read register from external PHY on E810
 * @hw: pointer to the HW struct
 * @addr: the address to read from
 * @val: On return, the value read from the PHY
 * @lock_sbq: true if the sideband queue lock must be acquired
 *
 * Read a register from the external PHY on the E810 device.
 */
static enum ice_status
ice_read_phy_reg_e810_lp(struct ice_hw *hw, u32 addr, u32 *val, bool lock_sbq)
{
	struct ice_sbq_msg_input msg = {0};
	enum ice_status status;

	msg.msg_addr_low = ICE_LO_WORD(addr);
	msg.msg_addr_high = ICE_HI_WORD(addr);
	msg.opcode = ice_sbq_msg_rd;
	msg.dest_dev = rmn_0;

	status = ice_sbq_rw_reg_lp(hw, &msg, lock_sbq);
	if (status) {
		ice_debug(hw, ICE_DBG_PTP, "Failed to send message to phy, status %d\n",
			  status);
		return status;
	}

	*val = msg.data;

	return ICE_SUCCESS;
}

static enum ice_status
ice_read_phy_reg_e810(struct ice_hw *hw, u32 addr, u32 *val)
{
	return ice_read_phy_reg_e810_lp(hw, addr, val, true);
}

/**
 * ice_write_phy_reg_e810_lp - Write register on external PHY on E810
 * @hw: pointer to the HW struct
 * @addr: the address to writem to
 * @val: the value to write to the PHY
 * @lock_sbq: true if the sideband queue lock must be acquired
 *
 * Write a value to a register of the external PHY on the E810 device.
 */
static enum ice_status
ice_write_phy_reg_e810_lp(struct ice_hw *hw, u32 addr, u32 val, bool lock_sbq)
{
	struct ice_sbq_msg_input msg = {0};
	enum ice_status status;

	msg.msg_addr_low = ICE_LO_WORD(addr);
	msg.msg_addr_high = ICE_HI_WORD(addr);
	msg.opcode = ice_sbq_msg_wr;
	msg.dest_dev = rmn_0;
	msg.data = val;

	status = ice_sbq_rw_reg_lp(hw, &msg, lock_sbq);
	if (status) {
		ice_debug(hw, ICE_DBG_PTP, "Failed to send message to phy, status %d\n",
			  status);
		return status;
	}

	return ICE_SUCCESS;
}

static enum ice_status
ice_write_phy_reg_e810(struct ice_hw *hw, u32 addr, u32 val)
{
	return ice_write_phy_reg_e810_lp(hw, addr, val, true);
}

/**
 * ice_read_phy_tstamp_ll_e810 - Read a PHY timestamp registers through the FW
 * @hw: pointer to the HW struct
 * @idx: the timestamp index to read
 * @hi: 8 bit timestamp high value
 * @lo: 32 bit timestamp low value
 *
 * Read a 8bit timestamp high value and 32 bit timestamp low value out of the
 * timestamp block of the external PHY on the E810 device using the low latency
 * timestamp read.
 */
static enum ice_status
ice_read_phy_tstamp_ll_e810(struct ice_hw *hw, u8 idx, u8 *hi, u32 *lo)
{
	u8 i;

	/* Write TS index to read to the PF register so the FW can read it */
	wr32(hw, PF_SB_ATQBAL, TS_LL_READ_TS_IDX(idx));

	/* Read the register repeatedly until the FW provides us the TS */
	for (i = TS_LL_READ_RETRIES; i > 0; i--) {
		u32 val = rd32(hw, PF_SB_ATQBAL);

		/* When the bit is cleared, the TS is ready in the register */
		if (!(val & TS_LL_READ_TS)) {
			/* High 8 bit value of the TS is on the bits 16:23 */
			*hi = (u8)(val >> TS_LL_READ_TS_HIGH_S);

			/* Read the low 32 bit value and set the TS valid bit */
			*lo = rd32(hw, PF_SB_ATQBAH) | TS_VALID;
			return ICE_SUCCESS;
		}

		ice_usec_delay(10, false);
	}

	/* FW failed to provide the TS in time */
	ice_debug(hw, ICE_DBG_PTP, "Failed to read PTP timestamp using low latency read\n");
	return ICE_ERR_NOT_READY;
}

/**
 * ice_read_phy_tstamp_sbq_e810 - Read a PHY timestamp registers through the sbq
 * @hw: pointer to the HW struct
 * @lport: the lport to read from
 * @idx: the timestamp index to read
 * @hi: 8 bit timestamp high value
 * @lo: 32 bit timestamp low value
 *
 * Read a 8bit timestamp high value and 32 bit timestamp low value out of the
 * timestamp block of the external PHY on the E810 device using sideband queue.
 */
static enum ice_status
ice_read_phy_tstamp_sbq_e810(struct ice_hw *hw, u8 lport, u8 idx, u8 *hi,
			     u32 *lo)
{
	u32 hi_addr = TS_EXT(HIGH_TX_MEMORY_BANK_START, lport, idx);
	u32 lo_addr = TS_EXT(LOW_TX_MEMORY_BANK_START, lport, idx);
	enum ice_status status;
	u32 lo_val, hi_val;

	status = ice_read_phy_reg_e810(hw, lo_addr, &lo_val);
	if (status) {
		ice_debug(hw, ICE_DBG_PTP, "Failed to read low PTP timestamp register, status %d\n",
			  status);
		return status;
	}

	status = ice_read_phy_reg_e810(hw, hi_addr, &hi_val);
	if (status) {
		ice_debug(hw, ICE_DBG_PTP, "Failed to read high PTP timestamp register, status %d\n",
			  status);
		return status;
	}

	*lo = lo_val;
	*hi = (u8)hi_val;

	return ICE_SUCCESS;
}

/**
 * ice_read_phy_tstamp_e810 - Read a PHY timestamp out of the external PHY
 * @hw: pointer to the HW struct
 * @lport: the lport to read from
 * @idx: the timestamp index to read
 * @tstamp: on return, the 40bit timestamp value
 *
 * Read a 40bit timestamp value out of the timestamp block of the external PHY
 * on the E810 device.
 */
static enum ice_status
ice_read_phy_tstamp_e810(struct ice_hw *hw, u8 lport, u8 idx, u64 *tstamp)
{
	enum ice_status status;
	u32 lo = 0;
	u8 hi = 0;

	if (hw->dev_caps.ts_dev_info.ts_ll_read)
		status = ice_read_phy_tstamp_ll_e810(hw, idx, &hi, &lo);
	else
		status = ice_read_phy_tstamp_sbq_e810(hw, lport, idx, &hi, &lo);

	if (status)
		return status;

	/* For E810 devices, the timestamp is reported with the lower 32 bits
	 * in the low register, and the upper 8 bits in the high register.
	 */
	*tstamp = ((u64)hi) << TS_HIGH_S | ((u64)lo & TS_LOW_M);

	return ICE_SUCCESS;
}

/**
 * ice_clear_phy_tstamp_e810 - Clear a timestamp from the external PHY
 * @hw: pointer to the HW struct
 * @lport: the lport to read from
 * @idx: the timestamp index to reset
 *
 * Read the timestamp and then forcibly overwrite its value to clear the valid
 * bit from the timestamp block of the external PHY on the E810 device.
 *
 * This function should only be called on an idx whose bit is set according to
 * ice_get_phy_tx_tstamp_ready.
 */
static enum ice_status
ice_clear_phy_tstamp_e810(struct ice_hw *hw, u8 lport, u8 idx)
{
	enum ice_status status;
	u32 lo_addr, hi_addr;
	u64 unused_tstamp;

	status = ice_read_phy_tstamp_e810(hw, lport, idx, &unused_tstamp);
	if (status) {
		ice_debug(hw, ICE_DBG_PTP, "Failed to read the timestamp register for lport %u, idx %u, status %d\n",
			  lport, idx, status);
		return status;
	}

	lo_addr = TS_EXT(LOW_TX_MEMORY_BANK_START, lport, idx);
	hi_addr = TS_EXT(HIGH_TX_MEMORY_BANK_START, lport, idx);

	status = ice_write_phy_reg_e810(hw, lo_addr, 0);
	if (status) {
		ice_debug(hw, ICE_DBG_PTP, "Failed to clear low PTP timestamp register for lport %u, idx %u, status %d\n",
			  lport, idx, status);
		return status;
	}

	status = ice_write_phy_reg_e810(hw, hi_addr, 0);
	if (status) {
		ice_debug(hw, ICE_DBG_PTP, "Failed to clear high PTP timestamp register for lport %u, idx %u, status %d\n",
			  lport, idx, status);
		return status;
	}

	return ICE_SUCCESS;
}

/**
 * ice_ptp_init_phy_e810 - Enable PTP function on the external PHY
 * @hw: pointer to HW struct
 *
 * Enable the timesync PTP functionality for the external PHY connected to
 * this function.
 *
 * Note there is no equivalent function needed on E822 based devices.
 */
enum ice_status ice_ptp_init_phy_e810(struct ice_hw *hw)
{
	enum ice_status status;
	u8 tmr_idx;

	tmr_idx = hw->func_caps.ts_func_info.tmr_index_owned;
	status = ice_write_phy_reg_e810(hw, ETH_GLTSYN_ENA(tmr_idx),
					GLTSYN_ENA_TSYN_ENA_M);
	if (status)
		ice_debug(hw, ICE_DBG_PTP, "PTP failed in ena_phy_time_syn %d\n",
			  status);

	return status;
}

/**
 * ice_ptp_init_phc_e810 - Perform E810 specific PHC initialization
 * @hw: pointer to HW struct
 *
 * Perform E810-specific PTP hardware clock initialization steps.
 */
static enum ice_status ice_ptp_init_phc_e810(struct ice_hw *hw)
{
	/* Ensure synchronization delay is zero */
	wr32(hw, GLTSYN_SYNC_DLAY, 0);

	/* Initialize the PHY */
	return ice_ptp_init_phy_e810(hw);
}

/**
 * ice_ptp_prep_phy_time_e810 - Prepare PHY port with initial time
 * @hw: Board private structure
 * @time: Time to initialize the PHY port clock to
 *
 * Program the PHY port ETH_GLTSYN_SHTIME registers in preparation setting the
 * initial clock time. The time will not actually be programmed until the
 * driver issues an ICE_PTP_INIT_TIME command.
 *
 * The time value is the upper 32 bits of the PHY timer, usually in units of
 * nominal nanoseconds.
 */
static enum ice_status ice_ptp_prep_phy_time_e810(struct ice_hw *hw, u32 time)
{
	enum ice_status status;
	u8 tmr_idx;

	tmr_idx = hw->func_caps.ts_func_info.tmr_index_owned;
	status = ice_write_phy_reg_e810(hw, ETH_GLTSYN_SHTIME_0(tmr_idx), 0);
	if (status) {
		ice_debug(hw, ICE_DBG_PTP, "Failed to write SHTIME_0, status %d\n",
			  status);
		return status;
	}

	status = ice_write_phy_reg_e810(hw, ETH_GLTSYN_SHTIME_L(tmr_idx), time);
	if (status) {
		ice_debug(hw, ICE_DBG_PTP, "Failed to write SHTIME_L, status %d\n",
			  status);
		return status;
	}

	return ICE_SUCCESS;
}

/**
 * ice_ptp_prep_phy_adj_e810 - Prep PHY port for a time adjustment
 * @hw: pointer to HW struct
 * @adj: adjustment value to program
 * @lock_sbq: true if the sideband queue luck must be acquired
 *
 * Prepare the PHY port for an atomic adjustment by programming the PHY
 * ETH_GLTSYN_SHADJ_L and ETH_GLTSYN_SHADJ_H registers. The actual adjustment
 * is completed by issuing an ICE_PTP_ADJ_TIME sync command.
 *
 * The adjustment value only contains the portion used for the upper 32bits of
 * the PHY timer, usually in units of nominal nanoseconds. Negative
 * adjustments are supported using 2s complement arithmetic.
 */
static enum ice_status
ice_ptp_prep_phy_adj_e810(struct ice_hw *hw, s32 adj, bool lock_sbq)
{
	enum ice_status status;
	u8 tmr_idx;

	tmr_idx = hw->func_caps.ts_func_info.tmr_index_owned;

	/* Adjustments are represented as signed 2's complement values in
	 * nanoseconds. Sub-nanosecond adjustment is not supported.
	 */
	status = ice_write_phy_reg_e810_lp(hw, ETH_GLTSYN_SHADJ_L(tmr_idx),
					   0, lock_sbq);
	if (status) {
		ice_debug(hw, ICE_DBG_PTP, "Failed to write adj to PHY SHADJ_L, status %d\n",
			  status);
		return status;
	}

	status = ice_write_phy_reg_e810_lp(hw, ETH_GLTSYN_SHADJ_H(tmr_idx),
					   adj, lock_sbq);
	if (status) {
		ice_debug(hw, ICE_DBG_PTP, "Failed to write adj to PHY SHADJ_H, status %d\n",
			  status);
		return status;
	}

	return ICE_SUCCESS;
}

/**
 * ice_ptp_prep_phy_incval_e810 - Prep PHY port increment value change
 * @hw: pointer to HW struct
 * @incval: The new 40bit increment value to prepare
 *
 * Prepare the PHY port for a new increment value by programming the PHY
 * ETH_GLTSYN_SHADJ_L and ETH_GLTSYN_SHADJ_H registers. The actual change is
 * completed by issuing an ICE_PTP_INIT_INCVAL command.
 */
static enum ice_status
ice_ptp_prep_phy_incval_e810(struct ice_hw *hw, u64 incval)
{
	enum ice_status status;
	u32 high, low;
	u8 tmr_idx;

	tmr_idx = hw->func_caps.ts_func_info.tmr_index_owned;
	low = ICE_LO_DWORD(incval);
	high = ICE_HI_DWORD(incval);

	status = ice_write_phy_reg_e810(hw, ETH_GLTSYN_SHADJ_L(tmr_idx), low);
	if (status) {
		ice_debug(hw, ICE_DBG_PTP, "Failed to write incval to PHY SHADJ_L, status %d\n",
			  status);
		return status;
	}

	status = ice_write_phy_reg_e810(hw, ETH_GLTSYN_SHADJ_H(tmr_idx), high);
	if (status) {
		ice_debug(hw, ICE_DBG_PTP, "Failed to write incval PHY SHADJ_H, status %d\n",
			  status);
		return status;
	}

	return ICE_SUCCESS;
}

/**
 * ice_ptp_prep_phy_adj_target_e810 - Prepare PHY port with adjust target
 * @hw: Board private structure
 * @target_time: Time to trigger the clock adjustment at
 *
 * Program the PHY port ETH_GLTSYN_SHTIME registers in preparation for
 * a target time adjust, which will trigger an adjustment of the clock in the
 * future. The actual adjustment will occur the next time the PHY port timer
 * crosses over the provided value after the driver issues an
 * ICE_PTP_ADJ_TIME_AT_TIME command.
 *
 * The time value is the upper 32 bits of the PHY timer, usually in units of
 * nominal nanoseconds.
 */
static enum ice_status
ice_ptp_prep_phy_adj_target_e810(struct ice_hw *hw, u32 target_time)
{
	enum ice_status status;
	u8 tmr_idx;

	tmr_idx = hw->func_caps.ts_func_info.tmr_index_owned;
	status = ice_write_phy_reg_e810(hw, ETH_GLTSYN_SHTIME_0(tmr_idx), 0);
	if (status) {
		ice_debug(hw, ICE_DBG_PTP, "Failed to write target time to SHTIME_0, status %d\n",
			  status);
		return status;
	}

	status = ice_write_phy_reg_e810(hw, ETH_GLTSYN_SHTIME_L(tmr_idx),
					target_time);
	if (status) {
		ice_debug(hw, ICE_DBG_PTP, "Failed to write target time to SHTIME_L, status %d\n",
			  status);
		return status;
	}

	return ICE_SUCCESS;
}

/**
 * ice_ptp_port_cmd_e810 - Prepare all external PHYs for a timer command
 * @hw: pointer to HW struct
 * @cmd: Command to be sent to the port
 * @lock_sbq: true if the sideband queue lock must be acquired
 *
 * Prepare the external PHYs connected to this device for a timer sync
 * command.
 */
static enum ice_status
ice_ptp_port_cmd_e810(struct ice_hw *hw, enum ice_ptp_tmr_cmd cmd,
		      bool lock_sbq)
{
	enum ice_status status;
	u32 cmd_val, val;

	switch (cmd) {
	case ICE_PTP_INIT_TIME:
		cmd_val = GLTSYN_CMD_INIT_TIME;
		break;
	case ICE_PTP_INIT_INCVAL:
		cmd_val = GLTSYN_CMD_INIT_INCVAL;
		break;
	case ICE_PTP_ADJ_TIME:
		cmd_val = GLTSYN_CMD_ADJ_TIME;
		break;
	case ICE_PTP_ADJ_TIME_AT_TIME:
		cmd_val = GLTSYN_CMD_ADJ_INIT_TIME;
		break;
	case ICE_PTP_READ_TIME:
		cmd_val = GLTSYN_CMD_READ_TIME;
		break;
	default:
		ice_warn(hw, "Unknown timer command %u\n", cmd);
		return ICE_ERR_PARAM;
	}

	/* Read, modify, write */
	status = ice_read_phy_reg_e810_lp(hw, ETH_GLTSYN_CMD, &val, lock_sbq);
	if (status) {
		ice_debug(hw, ICE_DBG_PTP, "Failed to read GLTSYN_CMD, status %d\n",
			  status);
		return status;
	}

	/* Modify necessary bits only and perform write */
	val &= ~TS_CMD_MASK_E810;
	val |= cmd_val;

	status = ice_write_phy_reg_e810_lp(hw, ETH_GLTSYN_CMD, val, lock_sbq);
	if (status) {
		ice_debug(hw, ICE_DBG_PTP, "Failed to write back GLTSYN_CMD, status %d\n",
			  status);
		return status;
	}

	return ICE_SUCCESS;
}

/**
 * ice_get_phy_tx_tstamp_ready_e810 - Read Tx memory status register
 * @hw: pointer to the HW struct
 * @port: the PHY port to read
 * @tstamp_ready: contents of the Tx memory status register
 *
 * E810 devices do not use a Tx memory status register. Instead simply
 * indicate that all timestamps are currently ready.
 */
static enum ice_status
ice_get_phy_tx_tstamp_ready_e810(struct ice_hw *hw, u8 port, u64 *tstamp_ready)
{
	*tstamp_ready = 0xFFFFFFFFFFFFFFFF;
	return ICE_SUCCESS;
}

/* E810T SMA functions
 *
 * The following functions operate specifically on E810T hardware and are used
 * to access the extended GPIOs available.
 */

/**
 * ice_get_pca9575_handle
 * @hw: pointer to the hw struct
 * @pca9575_handle: GPIO controller's handle
 *
 * Find and return the GPIO controller's handle by checking what drives clock
 * mux pin. When found - the value will be cached in the hw structure and
 * following calls will return cached value.
 */
static enum ice_status
ice_get_pca9575_handle(struct ice_hw *hw, u16 *pca9575_handle)
{
	u8 node_part_number, idx, node_type_ctx_clk_mux, node_part_num_clk_mux;
	struct ice_aqc_get_link_topo_pin cmd_pin;
	u16 node_handle, clock_mux_handle;
	struct ice_aqc_get_link_topo cmd;
	enum ice_status status;

	if (!hw || !pca9575_handle)
		return ICE_ERR_PARAM;

	/* If handle was read previously return cached value */
	if (hw->io_expander_handle) {
		*pca9575_handle = hw->io_expander_handle;
		return ICE_SUCCESS;
	}

	memset(&cmd, 0, sizeof(cmd));
	memset(&cmd_pin, 0, sizeof(cmd_pin));

	node_type_ctx_clk_mux = (ICE_AQC_LINK_TOPO_NODE_TYPE_CLK_MUX <<
				 ICE_AQC_LINK_TOPO_NODE_TYPE_S);
	node_type_ctx_clk_mux |= (ICE_AQC_LINK_TOPO_NODE_CTX_GLOBAL <<
				  ICE_AQC_LINK_TOPO_NODE_CTX_S);
	node_part_num_clk_mux = ICE_ACQ_GET_LINK_TOPO_NODE_NR_GEN_CLK_MUX;

	/* Look for CLOCK MUX handle in the netlist */
	status = ice_find_netlist_node(hw, node_type_ctx_clk_mux,
				       node_part_num_clk_mux,
				       &clock_mux_handle);
	if (status)
		return ICE_ERR_NOT_SUPPORTED;

	/* Take CLOCK MUX GPIO pin */
	cmd_pin.input_io_params = (ICE_AQC_LINK_TOPO_INPUT_IO_TYPE_GPIO <<
				   ICE_AQC_LINK_TOPO_INPUT_IO_TYPE_S);
	cmd_pin.input_io_params |= (ICE_AQC_LINK_TOPO_IO_FUNC_CLK_IN <<
				    ICE_AQC_LINK_TOPO_INPUT_IO_FUNC_S);
	cmd_pin.addr.handle = CPU_TO_LE16(clock_mux_handle);
	cmd_pin.addr.topo_params.node_type_ctx =
		(ICE_AQC_LINK_TOPO_NODE_TYPE_CLK_MUX <<
		 ICE_AQC_LINK_TOPO_NODE_TYPE_S);
	cmd_pin.addr.topo_params.node_type_ctx |=
		(ICE_AQC_LINK_TOPO_NODE_CTX_PROVIDED <<
		 ICE_AQC_LINK_TOPO_NODE_CTX_S);

	status = ice_aq_get_netlist_node_pin(hw, &cmd_pin, &node_handle);
	if (status)
		return ICE_ERR_NOT_SUPPORTED;

	/* Check what is driving the pin */
	cmd.addr.topo_params.node_type_ctx =
		(ICE_AQC_LINK_TOPO_NODE_TYPE_GPIO_CTRL <<
		 ICE_AQC_LINK_TOPO_NODE_TYPE_S);
	cmd.addr.topo_params.node_type_ctx |=
		(ICE_AQC_LINK_TOPO_NODE_CTX_GLOBAL <<
		 ICE_AQC_LINK_TOPO_NODE_CTX_S);
	cmd.addr.handle = CPU_TO_LE16(node_handle);

#define SW_PCA9575_SFP_TOPO_IDX		2
#define SW_PCA9575_QSFP_TOPO_IDX	1

	/* Check if the SW IO expander controlling SMA exists in the netlist. */
	if (hw->device_id == ICE_DEV_ID_E810C_SFP)
		idx = SW_PCA9575_SFP_TOPO_IDX;
	else if (hw->device_id == ICE_DEV_ID_E810C_QSFP)
		idx = SW_PCA9575_QSFP_TOPO_IDX;
	else
		return ICE_ERR_NOT_SUPPORTED;

	cmd.addr.topo_params.index = idx;
	status = ice_aq_get_netlist_node(hw, &cmd, &node_part_number,
					 &node_handle);
	if (status)
		return ICE_ERR_NOT_SUPPORTED;

	/* Verify if PCA9575 drives the pin */
	if (node_part_number != ICE_ACQ_GET_LINK_TOPO_NODE_NR_PCA9575)
		return ICE_ERR_NOT_SUPPORTED;

	/* If present save the handle and return it */
	hw->io_expander_handle = node_handle;
	*pca9575_handle = hw->io_expander_handle;

	return ICE_SUCCESS;
}

#ifdef SYNCE_SUPPORT
/**
 * ice_is_phy_rclk_present
 * @hw: pointer to the hw struct
 *
 * Check if the PHY Recovered Clock device is present in the netlist
 */
bool ice_is_phy_rclk_present(struct ice_hw *hw)
{
	if (ice_find_netlist_node(hw, ICE_AQC_LINK_TOPO_NODE_TYPE_CLK_CTRL,
				  ICE_ACQ_GET_LINK_TOPO_NODE_NR_C827, NULL) &&
	    ice_find_netlist_node(hw, ICE_AQC_LINK_TOPO_NODE_TYPE_CLK_CTRL,
				  ICE_ACQ_GET_LINK_TOPO_NODE_NR_E822_PHY, NULL))
		return false;

	return true;
}

/**
 * ice_is_clock_mux_present_e810t
 * @hw: pointer to the hw struct
 *
 * Check if the Clock Multiplexer device is present in the netlist
 */
bool ice_is_clock_mux_present_e810t(struct ice_hw *hw)
{
	if (ice_find_netlist_node(hw, ICE_AQC_LINK_TOPO_NODE_TYPE_CLK_MUX,
				  ICE_ACQ_GET_LINK_TOPO_NODE_NR_GEN_CLK_MUX,
				  NULL))
		return false;

	return true;
}

/**
 * ice_get_pf_c827_idx - find and return the C827 index for the current pf
 * @hw: pointer to the hw struct
 * @idx: index of the found C827 PHY
 */
enum ice_status ice_get_pf_c827_idx(struct ice_hw *hw, u8 *idx)
{
	struct ice_aqc_get_link_topo cmd;
	enum ice_status status;
	u8 node_part_number;
	u16 node_handle;
	u8 ctx;

	if (hw->mac_type != ICE_MAC_E810)
		return ICE_ERR_DEVICE_NOT_SUPPORTED;

	if (hw->device_id != ICE_DEV_ID_E810C_QSFP) {
		*idx = C827_0;
		return ICE_SUCCESS;
	}

	memset(&cmd, 0, sizeof(cmd));

	ctx = ICE_AQC_LINK_TOPO_NODE_TYPE_PHY << ICE_AQC_LINK_TOPO_NODE_TYPE_S;
	ctx |= ICE_AQC_LINK_TOPO_NODE_CTX_PORT << ICE_AQC_LINK_TOPO_NODE_CTX_S;
	cmd.addr.topo_params.node_type_ctx = ctx;
	cmd.addr.topo_params.index = 0;

	status = ice_aq_get_netlist_node(hw, &cmd, &node_part_number,
					 &node_handle);
	if (status || node_part_number != ICE_ACQ_GET_LINK_TOPO_NODE_NR_C827)
		return ICE_ERR_DOES_NOT_EXIST;

	if (node_handle == E810C_QSFP_C827_0_HANDLE)
		*idx = C827_0;
	else if (node_handle == E810C_QSFP_C827_1_HANDLE)
		*idx = C827_1;
	else
		return ICE_ERR_OUT_OF_RANGE;

	return ICE_SUCCESS;
}

#endif /* SYNCE_SUPPORT */
#ifdef GNSS_SUPPORT
/**
 * ice_is_gps_present_e810t
 * @hw: pointer to the hw struct
 *
 * Check if the GPS generic device is present in the netlist
 */
bool ice_is_gps_present_e810t(struct ice_hw *hw)
{
	if (ice_find_netlist_node(hw, ICE_AQC_LINK_TOPO_NODE_TYPE_GPS,
				  ICE_ACQ_GET_LINK_TOPO_NODE_NR_GEN_GPS, NULL))
		return false;

	return true;
}

#endif /* GNSS_SUPPORT */
/**
 * ice_read_pca9575_reg_e810t
 * @hw: pointer to the hw struct
 * @offset: GPIO controller register offset
 * @data: pointer to data to be read from the GPIO controller
 *
 * Read the register from the GPIO controller
 */
enum ice_status
ice_read_pca9575_reg_e810t(struct ice_hw *hw, u8 offset, u8 *data)
{
	struct ice_aqc_link_topo_addr link_topo;
	enum ice_status status;
	__le16 addr;
	u16 handle;

	memset(&link_topo, 0, sizeof(link_topo));

	status = ice_get_pca9575_handle(hw, &handle);
	if (status)
		return status;

	link_topo.handle = CPU_TO_LE16(handle);
	link_topo.topo_params.node_type_ctx =
		(ICE_AQC_LINK_TOPO_NODE_CTX_PROVIDED <<
		 ICE_AQC_LINK_TOPO_NODE_CTX_S);

	addr = CPU_TO_LE16((u16)offset);

	return ice_aq_read_i2c(hw, link_topo, 0, addr, 1, data, NULL);
}

/**
 * ice_write_pca9575_reg_e810t
 * @hw: pointer to the hw struct
 * @offset: GPIO controller register offset
 * @data: data to be written to the GPIO controller
 *
 * Write the data to the GPIO controller register
 */
enum ice_status
ice_write_pca9575_reg_e810t(struct ice_hw *hw, u8 offset, u8 data)
{
	struct ice_aqc_link_topo_addr link_topo;
	enum ice_status status;
	__le16 addr;
	u16 handle;

	memset(&link_topo, 0, sizeof(link_topo));

	status = ice_get_pca9575_handle(hw, &handle);
	if (status)
		return status;

	link_topo.handle = CPU_TO_LE16(handle);
	link_topo.topo_params.node_type_ctx =
		(ICE_AQC_LINK_TOPO_NODE_CTX_PROVIDED <<
		 ICE_AQC_LINK_TOPO_NODE_CTX_S);

	addr = CPU_TO_LE16((u16)offset);

	return ice_aq_write_i2c(hw, link_topo, 0, addr, 1, &data, NULL);
}

/**
 * ice_read_sma_ctrl_e810t
 * @hw: pointer to the hw struct
 * @data: pointer to data to be read from the GPIO controller
 *
 * Read the SMA controller state. Only bits 3-7 in data are valid.
 */
enum ice_status ice_read_sma_ctrl_e810t(struct ice_hw *hw, u8 *data)
{
	enum ice_status status;
	u16 handle;
	u8 i;

	status = ice_get_pca9575_handle(hw, &handle);
	if (status)
		return status;

	*data = 0;

	for (i = ICE_SMA_MIN_BIT_E810T; i <= ICE_SMA_MAX_BIT_E810T; i++) {
		bool pin;

		status = ice_aq_get_gpio(hw, handle, i + ICE_PCA9575_P1_OFFSET,
					 &pin, NULL);
		if (status)
			break;
		*data |= (u8)(!pin) << i;
	}

	return status;
}

/**
 * ice_write_sma_ctrl_e810t
 * @hw: pointer to the hw struct
 * @data: data to be written to the GPIO controller
 *
 * Write the data to the SMA controller. Only bits 3-7 in data are valid.
 */
enum ice_status ice_write_sma_ctrl_e810t(struct ice_hw *hw, u8 data)
{
	enum ice_status status;
	u16 handle;
	u8 i;

	status = ice_get_pca9575_handle(hw, &handle);
	if (status)
		return status;

	for (i = ICE_SMA_MIN_BIT_E810T; i <= ICE_SMA_MAX_BIT_E810T; i++) {
		bool pin;

		pin = !(data & (1 << i));
		status = ice_aq_set_gpio(hw, handle, i + ICE_PCA9575_P1_OFFSET,
					 pin, NULL);
		if (status)
			break;
	}

	return status;
}

/**
 * ice_is_pca9575_present
 * @hw: pointer to the hw struct
 *
 * Check if the SW IO expander is present in the netlist
 */
bool ice_is_pca9575_present(struct ice_hw *hw)
{
	enum ice_status status;
	u16 handle = 0;

	status = ice_get_pca9575_handle(hw, &handle);
	if (!status && handle)
		return true;

	return false;
}

#endif /* E810C_SUPPORT || E810_XXV_SUPPORT */
#ifdef SYNCE_SUPPORT
/**
 * ice_is_cgu_present
 * @hw: pointer to the hw struct
 *
 * Check if the Clock Generation Unit (CGU) device is present in the netlist
 */
bool ice_is_cgu_present(struct ice_hw *hw)
{
	if (!ice_find_netlist_node(hw, ICE_AQC_LINK_TOPO_NODE_TYPE_CLK_CTRL,
				   ICE_ACQ_GET_LINK_TOPO_NODE_NR_ZL30632_80032,
				   NULL)) {
		hw->cgu_part_number =
			ICE_ACQ_GET_LINK_TOPO_NODE_NR_ZL30632_80032;
		return true;
	} else if (!ice_find_netlist_node(hw,
					  ICE_AQC_LINK_TOPO_NODE_TYPE_CLK_CTRL,
					  ICE_ACQ_GET_LINK_TOPO_NODE_NR_SI5383_5384,
					  NULL)) {
		hw->cgu_part_number = ICE_ACQ_GET_LINK_TOPO_NODE_NR_SI5383_5384;
		return true;
	}

	return false;
}

/**
 * ice_cgu_state_to_name - get the name of CGU state
 * @state: state of the CGU
 *
 * Return: name of CGU state
 */
const char *ice_cgu_state_to_name(int state)
{
	switch (state) {
	case ICE_CGU_STATE_INVALID:
		return "invalid";
	case ICE_CGU_STATE_FREERUN:
		return "freerun";
	case ICE_CGU_STATE_LOCKED:
		return "locked";
	case ICE_CGU_STATE_LOCKED_HO_ACQ:
		return "locked_ho_acq";
	case ICE_CGU_STATE_HOLDOVER:
		return "holdover";
	case ICE_CGU_STATE_UNKNOWN:
	default:
		return "unknown";
	}
}

/**
 * ice_get_cgu_state - get the state of the DPLL
 * @hw: pointer to the hw struct
 * @dpll_idx: Index of internal DPLL unit
 * @pin: pointer to a buffer for returning currently active pin
 * @phase_offset: pointer to a buffer for returning phase offset
 * @last_dpll_state: last known state of DPLL
 *
 * This function will read the state of the DPLL(dpll_idx). Non-null
 * 'pin' and 'phase_offset' parameters are used to retrieve currently
 * active pin and phase_offset respectively.
 *
 * Return: state of the DPLL
 */
enum ice_cgu_state
ice_get_cgu_state(struct ice_hw *hw, u8 dpll_idx, u8 *pin, s64 *phase_offset,
		  enum ice_cgu_state last_dpll_state)
{
	enum ice_status status;
	u16 dpll_state;
	s64 ph_offset;
	u8 ref_state;
	u8 eec_mode;

	if (dpll_idx >= ICE_CGU_DPLL_MAX)
		return ICE_CGU_STATE_INVALID;

	status = ice_aq_get_cgu_dpll_status(hw, dpll_idx, &ref_state,
					    &dpll_state, &ph_offset,
					    &eec_mode);
	if (status)
		return ICE_CGU_STATE_INVALID;

	if (pin) {
		/* current ref pin in dpll_state_refsel_status_X register */
		*pin = (dpll_state &
			ICE_AQC_GET_CGU_DPLL_STATUS_STATE_CLK_REF_SEL) >>
		       ICE_AQC_GET_CGU_DPLL_STATUS_STATE_CLK_REF_SHIFT;
	}

	if (phase_offset)
		*phase_offset = ph_offset;

	if (dpll_state & ICE_AQC_GET_CGU_DPLL_STATUS_STATE_LOCK) {
		if (dpll_state & ICE_AQC_GET_CGU_DPLL_STATUS_STATE_HO_READY)
			return ICE_CGU_STATE_LOCKED_HO_ACQ;
		else
			return ICE_CGU_STATE_LOCKED;
	}

	/* According to ZL DPLL documentation once it goes to LOCKED_HO_ACQ it
	 * never return to FREERUN. This aligns to ITU-T G.781 Recommendation.
	 * But we cannot report HOLDOVER while HO memory is cleared while
	 * switching to another reference (so in LOCKED without HO_ACQ) or
	 * holdover timeouts (not implemented yet) - in those two situations
	 * (only!) we actually back to FREERUN.
	 */
	if (last_dpll_state == ICE_CGU_STATE_LOCKED_HO_ACQ ||
	    last_dpll_state == ICE_CGU_STATE_HOLDOVER)
		return ICE_CGU_STATE_HOLDOVER;

	return ICE_CGU_STATE_FREERUN;
}

#if defined(E810C_SUPPORT) || defined(E810_XXV_SUPPORT)
static const struct ice_cgu_pin_desc ice_e810t_cgu_inputs[] = {
	/* name		  idx */
	{ "CVL-SDP22",    ZL_REF0P },
	{ "CVL-SDP20",    ZL_REF0N },
	{ "C827_0-RCLKA", ZL_REF1P },
	{ "C827_0-RCLKB", ZL_REF1N },
	{ "C827_1-RCLKA", ZL_REF2P },
	{ "C827_1-RCLKB", ZL_REF2N },
	{ "SMA1",         ZL_REF3P },
	{ "SMA2/U.FL2",   ZL_REF3N },
	{ "GNSS-1PPS",    ZL_REF4P },
	{ "OCXO",         ZL_REF4N },
};

/**
 * ice_zl_pin_idx_to_name_e810t - get the name of E810T CGU pin
 * @pin: pin number
 *
 * Return: name of E810T CGU pin
 */
const char *ice_zl_pin_idx_to_name_e810t(u8 pin)
{
	if (pin < NUM_ZL_CGU_PINS)
		return ice_e810t_cgu_inputs[pin].name;

	return "invalid";
}
#endif /* E810C_SUPPORT || E810_XXV_SUPPORT */
#if defined(E823_SUPPORT) || defined(E823C_SUPPORT)
static const struct ice_cgu_pin_desc ice_e823_si_cgu_inputs[] = {
	/* name		  idx */
	{ "NONE",         SI_REF0P },
	{ "NONE",         SI_REF0N },
	{ "SYNCE0_DP",    SI_REF1P },
	{ "SYNCE0_DN",    SI_REF1N },
	{ "EXT_CLK_SYNC", SI_REF2P },
	{ "NONE",         SI_REF2N },
	{ "EXT_PPS_OUT",  SI_REF3  },
	{ "INT_PPS_OUT",  SI_REF4  },
};

static const struct ice_cgu_pin_desc ice_e823_zl_cgu_inputs[] = {
	/* name		  idx */
	{ "NONE",         ZL_REF0P },
	{ "INT_PPS_OUT",  ZL_REF0N },
	{ "SYNCE0_DP",    ZL_REF1P },
	{ "SYNCE0_DN",    ZL_REF1N },
	{ "NONE",         ZL_REF2P },
	{ "NONE",         ZL_REF2N },
	{ "EXT_CLK_SYNC", ZL_REF3P },
	{ "NONE",         ZL_REF3N },
	{ "EXT_PPS_OUT",  ZL_REF4P },
	{ "OCXO",         ZL_REF4N },
};

/**
 * ice_pin_idx_to_name_e823 - get the name of E823 CGU pin
 * @hw: pointer to the hw struct
 * @pin: pin number
 *
 * Return: name of E823 CGU pin
 */
const char *ice_pin_idx_to_name_e823(struct ice_hw *hw, u8 pin)
{
	if (hw->cgu_part_number ==
	    ICE_ACQ_GET_LINK_TOPO_NODE_NR_ZL30632_80032 &&
	    pin < NUM_ZL_CGU_PINS)
		return ice_e823_zl_cgu_inputs[pin].name;
	else if (hw->cgu_part_number ==
		 ICE_ACQ_GET_LINK_TOPO_NODE_NR_SI5383_5384 &&
		 pin < NUM_SI_CGU_PINS)
		return ice_e823_si_cgu_inputs[pin].name;
	else
		return "invalid";
}

#endif /* E823_SUPPORT || E823C_SUPPORT */
#endif /* SYNCE_SUPPORT */
/* Device agnostic functions
 *
 * The following functions implement shared behavior common to both E822/E823
 * and E810 devices, possibly calling a device specific implementation where
 * necessary.
 */

/**
 * ice_ptp_lock - Acquire PTP global semaphore register lock
 * @hw: pointer to the HW struct
 *
 * Acquire the global PTP hardware semaphore lock. Returns true if the lock
 * was acquired, false otherwise.
 *
 * The PFTSYN_SEM register sets the busy bit on read, returning the previous
 * value. If software sees the busy bit cleared, this means that this function
 * acquired the lock (and the busy bit is now set). If software sees the busy
 * bit set, it means that another function acquired the lock.
 *
 * Software must clear the busy bit with a write to release the lock for other
 * functions when done.
 */
bool ice_ptp_lock(struct ice_hw *hw)
{
	u32 hw_lock;
	int i;

#define MAX_TRIES 15

	for (i = 0; i < MAX_TRIES; i++) {
		hw_lock = rd32(hw, PFTSYN_SEM + (PFTSYN_SEM_BYTES * hw->pf_id));
		hw_lock = hw_lock & PFTSYN_SEM_BUSY_M;
		if (hw_lock) {
			/* Somebody is holding the lock */
#ifdef LINUX_SUPPORT
			usleep_range(5000, 6000);
#else
			ice_msec_delay(5, true);
#endif /* LINUX_SUPPORT */
			continue;
		}

		break;
	}

	return !hw_lock;
}

/**
 * ice_ptp_unlock - Release PTP global semaphore register lock
 * @hw: pointer to the HW struct
 *
 * Release the global PTP hardware semaphore lock. This is done by writing to
 * the PFTSYN_SEM register.
 */
void ice_ptp_unlock(struct ice_hw *hw)
{
	wr32(hw, PFTSYN_SEM + (PFTSYN_SEM_BYTES * hw->pf_id), 0);
}

#define ICE_DEVID_MASK 0xFFF8

/**
 * ice_ptp_init_phy_model - Initialize hw->phy_model based on device type
 * @hw: pointer to the HW structure
 *
 * Determine the PHY model for the device, and initialize hw->phy_model
 * for use by other functions.
 */
enum ice_status ice_ptp_init_phy_model(struct ice_hw *hw)
{
#ifdef ETH56G_SUPPORT
	unsigned int phy;
	u32 phy_rev;

	for (phy = 0; phy < MAX_PHYS_PER_ICE; phy++)
		hw->phy_addr[phy] = 0;

	switch (hw->device_id & ICE_DEVID_MASK) {
	case ICE_DEV_ID_E825C_BACKPLANE & ICE_DEVID_MASK:
		hw->phy_addr[0] = eth56g_dev_0;
		hw->phy_addr[1] = eth56g_dev_1;
		hw->num_phys = ICE_PHYS_PER_CPLX_C825X;
		hw->phy_ports = ICE_PORTS_PER_PHY_C825X;
		break;
	default:
		goto e8xx;
	}

	for (phy = 0; phy < hw->num_phys; phy++)
		if (hw->phy_addr[phy]) {
			enum ice_status status;

			ice_sb_access_ena_eth56g(hw, true);

			status = ice_read_phy_eth56g_raw_lp(hw, phy,
							    PHY_REG_REVISION,
							    &phy_rev, true);
			if (status)
				return status;

			if (phy_rev != PHY_REVISION_ETH56G) {
				hw->phy_model = ICE_PHY_UNSUP;
				return ICE_ERR_NOT_SUPPORTED;
			}
		}

	return ICE_SUCCESS;
e8xx:
#endif /* ETH56G_SUPPORT */

#if defined(E810C_SUPPORT) || defined(E810_XXV_SUPPORT)
	if (ice_is_e810(hw))
		hw->phy_model = ICE_PHY_E810;
	else
		hw->phy_model = ICE_PHY_E822;
#else
	hw->phy_model = ICE_PHY_E822;
#endif /* E810C_SUPPORT || E810_XXV_SUPPORT */

	return ICE_SUCCESS;
}

/**
 * ice_ptp_tmr_cmd - Prepare and trigger a timer sync command
 * @hw: pointer to HW struct
 * @cmd: the command to issue
 * @lock_sbq: true if the sideband queue lock must be acquired
 *
 * Prepare the source timer and PHY timers and then trigger the requested
 * command. This causes the shadow registers previously written in preparation
 * for the command to be synchronously applied to both the source and PHY
 * timers.
 */
static enum ice_status
ice_ptp_tmr_cmd(struct ice_hw *hw, enum ice_ptp_tmr_cmd cmd, bool lock_sbq)
{
	enum ice_status status;

	/* First, prepare the source timer */
	ice_ptp_src_cmd(hw, cmd);

	/* Next, prepare the ports */
	switch (hw->phy_model) {
#ifdef ETH56G_SUPPORT
	case ICE_PHY_ETH56G:
		status = ice_ptp_port_cmd_eth56g(hw, cmd, lock_sbq);
		break;
#endif
#if defined(E810C_SUPPORT) || defined(E810_XXV_SUPPORT)
	case ICE_PHY_E810:
		status = ice_ptp_port_cmd_e810(hw, cmd, lock_sbq);
		break;
#endif
	case ICE_PHY_E822:
		status = ice_ptp_port_cmd_e822(hw, cmd, lock_sbq);
		break;
	default:
		status = ICE_ERR_NOT_SUPPORTED;
	}
	if (status) {
		ice_debug(hw, ICE_DBG_PTP, "Failed to prepare PHY ports for timer command %u, status %d\n",
			  cmd, status);
		return status;
	}

	/* Write the sync command register to drive both source and PHY timer
	 * commands synchronously
	 */
	ice_ptp_exec_tmr_cmd(hw);
	ice_ptp_clean_cmd(hw);

	return ICE_SUCCESS;
}

/**
 * ice_ptp_init_time - Initialize device time to provided value
 * @hw: pointer to HW struct
 * @time: 64bits of time (GLTSYN_TIME_L and GLTSYN_TIME_H)
 * @wr_main_tmr: program the main timer
 *
 * Initialize the device to the specified time provided. This requires a three
 * step process:
 *
 * 1) write the new init time to the source timer shadow registers
 * 2) write the new init time to the phy timer shadow registers
 * 3) issue an init_time timer command to synchronously switch both the source
 *    and port timers to the new init time value at the next clock cycle.
 */
enum ice_status ice_ptp_init_time(struct ice_hw *hw, u64 time,
				  bool wr_main_tmr)
{
	enum ice_status status;
	u8 tmr_idx;

	tmr_idx = hw->func_caps.ts_func_info.tmr_index_owned;

	/* Source timers */
	if (wr_main_tmr) {
		wr32(hw, GLTSYN_SHTIME_L(tmr_idx), ICE_LO_DWORD(time));
		wr32(hw, GLTSYN_SHTIME_H(tmr_idx), ICE_HI_DWORD(time));
		wr32(hw, GLTSYN_SHTIME_0(tmr_idx), 0);
	}

	/* PHY Clks */
	/* Fill Rx and Tx ports and send msg to PHY */
	switch (hw->phy_model) {
#ifdef ETH56G_SUPPORT
	case ICE_PHY_ETH56G:
		status = ice_ptp_prep_phy_time_eth56g(hw, time & 0xFFFFFFFF);
		break;
#endif
#if defined(E810C_SUPPORT) || defined(E810_XXV_SUPPORT)
	case ICE_PHY_E810:
		status = ice_ptp_prep_phy_time_e810(hw, time & 0xFFFFFFFF);
		break;
#endif
	case ICE_PHY_E822:
		status = ice_ptp_prep_phy_time_e822(hw, time & 0xFFFFFFFF);
		break;
	default:
		status = ICE_ERR_NOT_SUPPORTED;
	}

	if (status)
		return status;

	return ice_ptp_tmr_cmd(hw, ICE_PTP_INIT_TIME, true);
}

/**
 * ice_ptp_write_incval - Program PHC with new increment value
 * @hw: pointer to HW struct
 * @incval: Source timer increment value per clock cycle
 * @wr_main_tmr: Program the main timer
 *
 * Program the timers with a new increment value. This requires a three-step
 * process:
 *
 * 1) Write the increment value to the source timer shadow registers
 * 2) Write the increment value to the PHY timer shadow registers
 * 3) Issue an ICE_PTP_INIT_INCVAL timer command to synchronously switch both
 *    the source and port timers to the new increment value at the next clock
 *    cycle.
 */
enum ice_status ice_ptp_write_incval(struct ice_hw *hw, u64 incval,
				     bool wr_main_tmr)
{
	enum ice_status status;
	u8 tmr_idx;

	tmr_idx = hw->func_caps.ts_func_info.tmr_index_owned;

	if (wr_main_tmr) {
		/* Shadow Adjust */
		wr32(hw, GLTSYN_SHADJ_L(tmr_idx), ICE_LO_DWORD(incval));
		wr32(hw, GLTSYN_SHADJ_H(tmr_idx), ICE_HI_DWORD(incval));
	}

	switch (hw->phy_model) {
#ifdef ETH56G_SUPPORT
	case ICE_PHY_ETH56G:
		status = ice_ptp_prep_phy_incval_eth56g(hw, incval);
		break;
#endif
#if defined(E810C_SUPPORT) || defined(E810_XXV_SUPPORT)
	case ICE_PHY_E810:
		status = ice_ptp_prep_phy_incval_e810(hw, incval);
		break;
#endif
	case ICE_PHY_E822:
		status = ice_ptp_prep_phy_incval_e822(hw, incval);
		break;
	default:
		status = ICE_ERR_NOT_SUPPORTED;
	}

	if (status)
		return status;

	return ice_ptp_tmr_cmd(hw, ICE_PTP_INIT_INCVAL, true);
}

/**
 * ice_ptp_write_incval_locked - Program new incval while holding semaphore
 * @hw: pointer to HW struct
 * @incval: Source timer increment value per clock cycle
 * @wr_main_tmr: Program the main timer
 *
 * Program a new PHC incval while holding the PTP semaphore.
 */
enum ice_status ice_ptp_write_incval_locked(struct ice_hw *hw, u64 incval,
					    bool wr_main_tmr)
{
	enum ice_status status;

	if (!ice_ptp_lock(hw))
		return ICE_ERR_NOT_READY;

	status = ice_ptp_write_incval(hw, incval, wr_main_tmr);

	ice_ptp_unlock(hw);

	return status;
}

/**
 * ice_ptp_adj_clock - Adjust PHC clock time atomically
 * @hw: pointer to HW struct
 * @adj: Adjustment in nanoseconds
 * @lock_sbq: true to lock the sbq sq_lock (the usual case); false if the
 *            sq_lock has already been locked at a higher level
 *
 * Perform an atomic adjustment of the PHC time by the specified number of
 * nanoseconds. This requires a three-step process:
 *
 * 1) Write the adjustment to the source timer shadow registers
 * 2) Write the adjustment to the PHY timer shadow registers
 * 3) Issue an ICE_PTP_ADJ_TIME timer command to synchronously apply the
 *    adjustment to both the source and port timers at the next clock cycle.
 */
enum ice_status ice_ptp_adj_clock(struct ice_hw *hw, s32 adj, bool lock_sbq)
{
	enum ice_status status;
	u8 tmr_idx;

	tmr_idx = hw->func_caps.ts_func_info.tmr_index_owned;

	/* Write the desired clock adjustment into the GLTSYN_SHADJ register.
	 * For an ICE_PTP_ADJ_TIME command, this set of registers represents
	 * the value to add to the clock time. It supports subtraction by
	 * interpreting the value as a 2's complement integer.
	 */
	wr32(hw, GLTSYN_SHADJ_L(tmr_idx), 0);
	wr32(hw, GLTSYN_SHADJ_H(tmr_idx), adj);

	switch (hw->phy_model) {
#ifdef ETH56G_SUPPORT
	case ICE_PHY_ETH56G:
		status = ice_ptp_prep_phy_adj_eth56g(hw, adj, lock_sbq);
		break;
#endif
#if defined(E810C_SUPPORT) || defined(E810_XXV_SUPPORT)
	case ICE_PHY_E810:
		status = ice_ptp_prep_phy_adj_e810(hw, adj, lock_sbq);
		break;
#endif
	case ICE_PHY_E822:
		status = ice_ptp_prep_phy_adj_e822(hw, adj, lock_sbq);
		break;
	default:
		status = ICE_ERR_NOT_SUPPORTED;
	}

	if (status)
		return status;

	return ice_ptp_tmr_cmd(hw, ICE_PTP_ADJ_TIME, lock_sbq);
}

/**
 * ice_ptp_adj_clock_at_time - Adjust PHC atomically at specified time
 * @hw: pointer to HW struct
 * @at_time: Time in nanoseconds at which to perform the adjustment
 * @adj: Adjustment in nanoseconds
 *
 * Perform an atomic adjustment to the PHC clock at the specified time. This
 * requires a five-step process:
 *
 * 1) Write the adjustment to the source timer shadow adjust registers
 * 2) Write the target time to the source timer shadow time registers
 * 3) Write the adjustment to the PHY timers shadow adjust registers
 * 4) Write the target time to the PHY timers shadow adjust registers
 * 5) Issue an ICE_PTP_ADJ_TIME_AT_TIME command to initiate the atomic
 *    adjustment.
 */
enum ice_status
ice_ptp_adj_clock_at_time(struct ice_hw *hw, u64 at_time, s32 adj)
{
	enum ice_status status;
	u32 time_lo, time_hi;
	u8 tmr_idx;

	tmr_idx = hw->func_caps.ts_func_info.tmr_index_owned;
	time_lo = ICE_LO_DWORD(at_time);
	time_hi = ICE_HI_DWORD(at_time);

	/* Write the desired clock adjustment into the GLTSYN_SHADJ register.
	 * For an ICE_PTP_ADJ_TIME_AT_TIME command, this set of registers
	 * represents the value to add to the clock time. It supports
	 * subtraction by interpreting the value as a 2's complement integer.
	 */
	wr32(hw, GLTSYN_SHADJ_L(tmr_idx), 0);
	wr32(hw, GLTSYN_SHADJ_H(tmr_idx), adj);

	/* Write the target time to trigger the adjustment for source clock */
	wr32(hw, GLTSYN_SHTIME_0(tmr_idx), 0);
	wr32(hw, GLTSYN_SHTIME_L(tmr_idx), time_lo);
	wr32(hw, GLTSYN_SHTIME_H(tmr_idx), time_hi);

	/* Prepare PHY port adjustments */
	switch (hw->phy_model) {
#ifdef ETH56G_SUPPORT
	case ICE_PHY_ETH56G:
		status = ice_ptp_prep_phy_adj_eth56g(hw, adj, true);
		break;
#endif
#if defined(E810C_SUPPORT) || defined(E810_XXV_SUPPORT)
	case ICE_PHY_E810:
		status = ice_ptp_prep_phy_adj_e810(hw, adj, true);
		break;
#endif
	case ICE_PHY_E822:
		status = ice_ptp_prep_phy_adj_e822(hw, adj, true);
		break;
	default:
		status = ICE_ERR_NOT_SUPPORTED;
	}

	if (status)
		return status;

	/* Set target time for each PHY port */
	switch (hw->phy_model) {
#ifdef ETH56G_SUPPORT
	case ICE_PHY_ETH56G:
		status = ice_ptp_prep_phy_adj_target_eth56g(hw, time_lo);
		break;
#endif
#if defined(E810C_SUPPORT) || defined(E810_XXV_SUPPORT)
	case ICE_PHY_E810:
		status = ice_ptp_prep_phy_adj_target_e810(hw, time_lo);
		break;
#endif
	case ICE_PHY_E822:
		status = ice_ptp_prep_phy_adj_target_e822(hw, time_lo);
		break;
	default:
		status = ICE_ERR_NOT_SUPPORTED;
	}

	if (status)
		return status;

	return ice_ptp_tmr_cmd(hw, ICE_PTP_ADJ_TIME_AT_TIME, true);
}

/**
 * ice_ptp_clear_phy_offset_ready - Clear PHY TX_/RX_OFFSET_READY registers
 * @hw: pointer to the HW struct
 *
 * Clear PHY TX_/RX_OFFSET_READY registers, effectively marking all transmitted
 * and received timestamps as invalid.
 */
enum ice_status ice_ptp_clear_phy_offset_ready(struct ice_hw *hw)
{
	switch (hw->phy_model) {
#ifdef ETH56G_SUPPORT
	case ICE_PHY_ETH56G:
		return ice_ptp_clear_phy_offset_ready_eth56g(hw);
#endif
#if defined(E810C_SUPPORT) || defined(E810_XXV_SUPPORT)
	case ICE_PHY_E810:
		return ICE_SUCCESS;
#endif
	case ICE_PHY_E822:
		return ice_ptp_clear_phy_offset_ready_e822(hw);
	default:
		return ICE_ERR_NOT_SUPPORTED;
	}
}

/**
 * ice_read_phy_tstamp - Read a PHY timestamp from the timestamp block
 * @hw: pointer to the HW struct
 * @block: the block/port to read from
 * @idx: the timestamp index to read
 * @tstamp: on return, the 40bit timestamp value
 *
 * Read a 40bit timestamp value out of the timestamp block. For E822 devices,
 * the block is the quad to read from. For E810 devices, the block is the
 * logical port to read from.
 */
enum ice_status
ice_read_phy_tstamp(struct ice_hw *hw, u8 block, u8 idx, u64 *tstamp)
{
	switch (hw->phy_model) {
#ifdef ETH56G_SUPPORT
	case ICE_PHY_ETH56G:
		return ice_read_phy_tstamp_eth56g(hw, block, idx, tstamp);
#endif
#if defined(E810C_SUPPORT) || defined(E810_XXV_SUPPORT)
	case ICE_PHY_E810:
		return ice_read_phy_tstamp_e810(hw, block, idx, tstamp);
#endif
	case ICE_PHY_E822:
		return ice_read_phy_tstamp_e822(hw, block, idx, tstamp);
	default:
		return ICE_ERR_NOT_SUPPORTED;
	}
}

/**
 * ice_clear_phy_tstamp - Drop a timestamp from the timestamp block
 * @hw: pointer to the HW struct
 * @block: the block to read from
 * @idx: the timestamp index to reset
 *
 * Drop a timestamp from the timestamp block by reading it. This will reset
 * the memory status bit allowing the timestamp index to be reused. For E822
 * devices, the block is the quad to clear from. For E810 devices, the block
 * is the logical port to clear from.
 *
 * This function should only be called on a timestamp index whose valid bit
 * is set according to ice_get_phy_tx_tstamp_ready.
 */
enum ice_status
ice_clear_phy_tstamp(struct ice_hw *hw, u8 block, u8 idx)
{
	switch (hw->phy_model) {
#ifdef ETH56G_SUPPORT
	case ICE_PHY_ETH56G:
		return ice_clear_phy_tstamp_eth56g(hw, block, idx);
#endif
#if defined(E810C_SUPPORT) || defined(E810_XXV_SUPPORT)
	case ICE_PHY_E810:
		return ice_clear_phy_tstamp_e810(hw, block, idx);
#endif
	case ICE_PHY_E822:
		return ice_clear_phy_tstamp_e822(hw, block, idx);
	default:
		return ICE_ERR_NOT_SUPPORTED;
	}
}

/**
 * ice_ptp_reset_ts_memory - Reset timestamp memory for all blocks
 * @hw: pointer to the HW struct
 */
void ice_ptp_reset_ts_memory(struct ice_hw *hw)
{
	switch (hw->phy_model) {
#ifdef ETH56G_SUPPORT
	case ICE_PHY_ETH56G:
		ice_ptp_reset_ts_memory_eth56g(hw);
		break;
#endif
	case ICE_PHY_E822:
		ice_ptp_reset_ts_memory_e822(hw);
		break;
#if defined(E810C_SUPPORT) || defined(E810_XXV_SUPPORT)
	case ICE_PHY_E810:
#endif
	default:
		return;
	}
}

#ifdef SIMICS_SUPPORT
/**
 * ice_ptp_sync_phy_timer - Synchronize PHY timers for a specific port
 * @hw: pointer to the HW struct
 * @port: port number
 *
 * Perform the steps required to resynchronize the PTP hardware clock.
 */

enum ice_status ice_ptp_sync_phy_timer(struct ice_hw *hw, u8 port)
{
	switch (hw->phy_model) {
#ifdef ETH56G_SUPPORT
	case ICE_PHY_ETH56G:
		return ice_sync_phy_timer_eth56g(hw, port);
		break;
#endif
#if defined(E810C_SUPPORT) || defined(E810_XXV_SUPPORT)
	case ICE_PHY_E810:
#endif
	case ICE_PHY_E822:
		return ice_sync_phy_timer_e822(hw, port);
		break;
	default:
		return ICE_ERR_NOT_SUPPORTED;
	}
}
#endif /* SIMICS_SUPPORT */

/**
 * ice_ptp_init_phc - Initialize PTP hardware clock
 * @hw: pointer to the HW struct
 *
 * Perform the steps required to initialize the PTP hardware clock.
 */
enum ice_status ice_ptp_init_phc(struct ice_hw *hw)
{
	u8 src_idx = hw->func_caps.ts_func_info.tmr_index_owned;

	/* Enable source clocks */
	wr32(hw, GLTSYN_ENA(src_idx), GLTSYN_ENA_TSYN_ENA_M);

	/* Clear event status indications for auxiliary pins */
	(void)rd32(hw, GLTSYN_STAT(src_idx));

	switch (hw->phy_model) {
#ifdef ETH56G_SUPPORT
	case ICE_PHY_ETH56G:
		return ice_ptp_init_phc_eth56g(hw);
#endif
#if defined(E810C_SUPPORT) || defined(E810_XXV_SUPPORT)
	case ICE_PHY_E810:
		return ice_ptp_init_phc_e810(hw);
#endif
	case ICE_PHY_E822:
		return ice_ptp_init_phc_e822(hw);
	default:
		return ICE_ERR_NOT_SUPPORTED;
	}
}

/**
 * ice_get_phy_tx_tstamp_ready - Read PHY Tx memory status indication
 * @hw: pointer to the HW struct
 * @block: the timestamp block to check
 * @tstamp_ready: storage for the PHY Tx memory status information
 *
 * Check the PHY for Tx timestamp memory status. This reports a 64 bit value
 * which indicates which timestamps in the block may be captured. A set bit
 * means the timestamp can be read. An unset bit means the timestamp is not
 * ready and software should avoid reading the register.
 */
enum ice_status
ice_get_phy_tx_tstamp_ready(struct ice_hw *hw, u8 block, u64 *tstamp_ready)
{
	switch (hw->phy_model) {
#ifdef ETH56G_SUPPORT
	case ICE_PHY_ETH56G:
		return ice_get_phy_tx_tstamp_ready_eth56g(hw, block,
							  tstamp_ready);
#endif
#if defined(E810C_SUPPORT) || defined(E810_XXV_SUPPORT)
	case ICE_PHY_E810:
		return ice_get_phy_tx_tstamp_ready_e810(hw, block,
							tstamp_ready);
#endif
	case ICE_PHY_E822:
		return ice_get_phy_tx_tstamp_ready_e822(hw, block,
							tstamp_ready);
		break;
	default:
		return ICE_ERR_NOT_SUPPORTED;
	}
}

/**
 * ice_ptp_one_port_cmd - Prepare a single PHY port for a timer command
 * @hw: pointer to HW struct
 * @port: Port to which cmd has to be sent
 * @cmd: Command to be sent to the port
 * @lock_sbq: true if the sideband queue lock must be acquired
 *
 * Prepare the requested port for an upcoming timer sync command.
 */
enum ice_status
ice_ptp_one_port_cmd(struct ice_hw *hw, u8 port, enum ice_ptp_tmr_cmd cmd,
		     bool lock_sbq)
{
	switch (hw->phy_model) {
#ifdef ETH56G_SUPPORT
	case ICE_PHY_ETH56G:
		return ice_ptp_one_port_cmd_eth56g(hw, port,
						   ICE_PTP_READ_TIME,
						   true);
#endif /* ETH56G_SUPPORT */
	case ICE_PHY_E822:
		return ice_ptp_one_port_cmd_e822(hw, port,
						 ICE_PTP_READ_TIME,
						 true);
	default:
		return ICE_ERR_NOT_SUPPORTED;
	}
}

/**
 * ice_ptp_read_port_capture - Read a port's local time capture
 * @hw: pointer to HW struct
 * @port: Port number to read
 * @tx_ts: on return, the Tx port time capture
 * @rx_ts: on return, the Rx port time capture
 *
 * Read the port's Tx and Rx local time capture values.
 */
enum ice_status
ice_ptp_read_port_capture(struct ice_hw *hw, u8 port, u64 *tx_ts,
			  u64 *rx_ts)
{
	switch (hw->phy_model) {
#ifdef ETH56G_SUPPORT
	case ICE_PHY_ETH56G:
		return ice_ptp_read_port_capture_eth56g(hw, port,
							tx_ts, rx_ts);
#endif /* ETH56G_SUPPORT */
	case ICE_PHY_E822:
		return ice_ptp_read_port_capture_e822(hw, port,
						      tx_ts, rx_ts);
	default:
		return ICE_ERR_NOT_SUPPORTED;
	}
}

/**
 * ice_ptp_read_phy_incval - Read a PHY port's current incval
 * @hw: pointer to the HW struct
 * @port: the port to read
 * @incval: on return, the time_clk_cyc incval for this port
 *
 * Read the time_clk_cyc increment value for a given PHY port.
 */
enum ice_status
ice_ptp_read_phy_incval(struct ice_hw *hw, u8 port, u64 *incval)
{
	switch (hw->phy_model) {
#ifdef ETH56G_SUPPORT
	case ICE_PHY_ETH56G:
		return ice_ptp_read_phy_incval_eth56g(hw, port, incval);
#endif /* ETH56G_SUPPORT */
	case ICE_PHY_E822:
		return ice_ptp_read_phy_incval_e822(hw, port, incval);
	default:
		return ICE_ERR_NOT_SUPPORTED;
	}
}

#endif /* !NO_PTP_SUPPORT */
