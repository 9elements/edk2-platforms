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

#ifndef _ICE_XLT_KB_H_
#define _ICE_XLT_KB_H_

#ifndef NO_FLEXP_SUPPORT
#if defined(DPDK_SUPPORT) || defined(ADV_AVF_SUPPORT)
#define ICE_XLT_KB_TBL_CNT 8
#define ICE_XLT_KB_FLAG0_14_CNT 15

struct ice_xlt_kb_entry {
	u8 xlt1_ad_sel;
	u8 xlt2_ad_sel;
	u16 flg0_14_sel[ICE_XLT_KB_FLAG0_14_CNT];
	u8 xlt1_md_sel;
	u8 xlt2_md_sel;
};

struct ice_xlt_kb {
	u8 xlt1_pm;
	u8 xlt2_pm;
	u8 prof_id_pm;
	u64 flag15;

	struct ice_xlt_kb_entry entries[ICE_XLT_KB_TBL_CNT];
};

void ice_xlt_kb_dump(struct ice_hw *hw, struct ice_xlt_kb *kb);
struct ice_xlt_kb *ice_xlt_kb_get_sw(struct ice_hw *hw);
struct ice_xlt_kb *ice_xlt_kb_get_acl(struct ice_hw *hw);
struct ice_xlt_kb *ice_xlt_kb_get_fd(struct ice_hw *hw);
struct ice_xlt_kb *ice_xlt_kb_get_rss(struct ice_hw *hw);
u16 ice_xlt_kb_flag_get(struct ice_xlt_kb *kb, u64 pkt_flag);
#endif /* DPDK_SUPPORT || ADV_AVF_SUPPORT */
#endif /* !NO_FLEXP_SUPPORT */
#endif /* _ICE_XLT_KB_H */
