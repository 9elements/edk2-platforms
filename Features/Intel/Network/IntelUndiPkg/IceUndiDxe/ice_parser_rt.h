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

#ifndef _ICE_PARSER_RT_H_
#define _ICE_PARSER_RT_H_

#ifndef NO_FLEXP_SUPPORT
#if defined(DPDK_SUPPORT) || defined(ADV_AVF_SUPPORT)
struct ice_parser_ctx;

#define ICE_PARSER_MAX_PKT_LEN 504
#define ICE_PARSER_GPR_NUM 128

struct ice_gpr_pu {
	bool gpr_val_upd[128]; /* flag to indicate if GRP needs to be updated */
	u16 gpr_val[128];
	u64 flg_msk;
	u64 flg_val;
	u16 err_msk;
	u16 err_val;
};

struct ice_parser_rt {
	struct ice_parser *psr;
	u16 gpr[ICE_PARSER_GPR_NUM];
	u8 pkt_buf[ICE_PARSER_MAX_PKT_LEN + 32];
	u16 pkt_len;
	u16 po;
	u8 bst_key[20];
	struct ice_pg_cam_key pg_key;
	struct ice_alu *alu0;
	struct ice_alu *alu1;
	struct ice_alu *alu2;
	struct ice_pg_cam_action *action;
	u8 pg;
	struct ice_gpr_pu pu;
	u8 markers[9]; /* 8 * 9 = 72 bits*/
	bool protocols[256];
	u16 offsets[256];
};

void ice_parser_rt_reset(struct ice_parser_rt *rt);
void ice_parser_rt_pktbuf_set(struct ice_parser_rt *rt, const u8 *pkt_buf,
			      int pkt_len);

struct ice_parser_result;
enum ice_status ice_parser_rt_execute(struct ice_parser_rt *rt,
				      struct ice_parser_result *rslt);
#endif /* DPDK_SUPPORT || ADV_AVF_SUPPORT */
#endif /* !NO_FLEXP_SUPPORT */
#endif /* _ICE_PARSER_RT_H_ */
