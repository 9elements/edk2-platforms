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

#ifdef SRIOV_SUPPORT
#ifndef _ICE_VF_MBX_H_
#define _ICE_VF_MBX_H_

#ifndef ICE_TDD
#include "ice_type.h"
#include "ice_controlq.h"

/* Defining the mailbox message threshold as 63 asynchronous
 * pending messages. Normal VF functionality does not require
 * sending more than 63 asynchronous pending message.
 */
#define ICE_ASYNC_VF_MSG_THRESHOLD	63

#ifndef LINUX_SUPPORT
enum ice_status
ice_aq_send_msg_to_pf(struct ice_hw *hw, enum virtchnl_ops v_opcode,
		      enum ice_status v_retval, u8 *msg, u16 msglen,
		      struct ice_sq_cd *cd);
#endif /* !LINUX_SUPPORT */
#ifdef LINUX_SUPPORT
/* #ifdef CONFIG_PCI_IOV */
#endif /* LINUX_SUPPORT */
#ifdef AE_DRIVER
enum ice_status
ice_aq_send_msg_to_pf(struct ice_hw *hw, u32 v_opcode, u32 v_retval, u8 *msg,
		      u16 msglen, struct ice_sq_cd *cd);

#endif /* AE_DRIVER */
enum ice_status
ice_aq_send_msg_to_vf(struct ice_hw *hw, u16 vfid, u32 v_opcode, u32 v_retval,
		      u8 *msg, u16 msglen, struct ice_sq_cd *cd);

u32 ice_conv_link_speed_to_virtchnl(bool adv_link_support, u16 link_speed);
enum ice_status
ice_mbx_vf_state_handler(struct ice_hw *hw, struct ice_mbx_data *mbx_data,
			 u16 vf_id, bool *is_mal_vf);
enum ice_status
ice_mbx_clear_malvf(struct ice_mbx_snapshot *snap, ice_bitmap_t *all_malvfs,
		    u16 bitmap_len, u16 vf_id);
enum ice_status ice_mbx_init_snapshot(struct ice_hw *hw, u16 vf_count);
void ice_mbx_deinit_snapshot(struct ice_hw *hw);
enum ice_status
ice_mbx_report_malvf(struct ice_hw *hw, ice_bitmap_t *all_malvfs,
		     u16 bitmap_len, u16 vf_id, bool *report_malvf);
#ifdef LINUX_SUPPORT
/* #else CONFIG_PCI_IOV */
static inline enum ice_status
ice_aq_send_msg_to_vf(struct ice_hw __always_unused *hw,
		      u16 __always_unused vfid, u32 __always_unused v_opcode,
		      u32 __always_unused v_retval, u8 __always_unused *msg,
		      u16 __always_unused msglen,
		      struct ice_sq_cd __always_unused *cd)
{
	return ICE_SUCCESS;
}

static inline u32
ice_conv_link_speed_to_virtchnl(bool __always_unused adv_link_support,
				u16 __always_unused link_speed)
{
	return 0;
}

/* #endif CONFIG_PCI_IOV */
#endif /* LINUX_SUPPORT */
#endif /* !ICE_TDD */
#endif /* _ICE_VF_MBX_H_ */
#endif /* SRIOV_SUPPORT */
