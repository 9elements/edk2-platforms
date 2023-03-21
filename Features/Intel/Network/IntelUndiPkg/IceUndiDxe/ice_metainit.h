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

#ifndef _ICE_METAINIT_H_
#define _ICE_METAINIT_H_

#ifndef NO_FLEXP_SUPPORT
#if defined(DPDK_SUPPORT) || defined(ADV_AVF_SUPPORT)
struct ice_metainit_item {
	u16 idx;

	u8 tsr;
	u16 ho;
	u16 pc;
	u16 pg_rn;
	u8 cd;

	bool gpr_a_ctrl;
	u8 gpr_a_data_mdid;
	u8 gpr_a_data_start;
	u8 gpr_a_data_len;
	u8 gpr_a_id;

	bool gpr_b_ctrl;
	u8 gpr_b_data_mdid;
	u8 gpr_b_data_start;
	u8 gpr_b_data_len;
	u8 gpr_b_id;

	bool gpr_c_ctrl;
	u8 gpr_c_data_mdid;
	u8 gpr_c_data_start;
	u8 gpr_c_data_len;
	u8 gpr_c_id;

	bool gpr_d_ctrl;
	u8 gpr_d_data_mdid;
	u8 gpr_d_data_start;
	u8 gpr_d_data_len;
	u8 gpr_d_id;

	u64 flags;
};

void ice_metainit_dump(struct ice_hw *hw, struct ice_metainit_item *item);
struct ice_metainit_item *ice_metainit_table_get(struct ice_hw *hw);
#endif /* DPDK_SUPPORT || ADV_AVF_SUPPORT */
#endif /* !NO_FLEXP_SUPPORT */
#endif /*_ICE_METAINIT_H_ */
