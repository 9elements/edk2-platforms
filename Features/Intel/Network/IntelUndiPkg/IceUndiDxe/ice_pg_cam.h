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

#ifndef _ICE_PG_CAM_H_
#define _ICE_PG_CAM_H_

#ifndef NO_FLEXP_SUPPORT
#if defined(DPDK_SUPPORT) || defined(ADV_AVF_SUPPORT)
#define ICE_PG_CAM_TABLE_SIZE		2048
#define ICE_PG_SP_CAM_TABLE_SIZE	128
#define ICE_PG_NM_CAM_TABLE_SIZE	1024
#define ICE_PG_NM_SP_CAM_TABLE_SIZE	64

struct ice_pg_cam_key {
	bool valid;
	u16 node_id;
	bool flag0;
	bool flag1;
	bool flag2;
	bool flag3;
	u8 boost_idx;
	u16 alu_reg;
	u32 next_proto;
};

struct ice_pg_nm_cam_key {
	bool valid;
	u16 node_id;
	bool flag0;
	bool flag1;
	bool flag2;
	bool flag3;
	u8 boost_idx;
	u16 alu_reg;
};

struct ice_pg_cam_action {
	u16 next_node;
	u8 next_pc;
	bool is_pg;
	u8 proto_id;
	bool is_mg;
	u8 marker_id;
	bool is_last_round;
	bool ho_polarity;
	u16 ho_inc;
};

struct ice_pg_cam_item {
	u16 idx;
	struct ice_pg_cam_key key;
	struct ice_pg_cam_action action;
};

struct ice_pg_nm_cam_item {
	u16 idx;
	struct ice_pg_nm_cam_key key;
	struct ice_pg_cam_action action;
};

void ice_pg_cam_dump(struct ice_hw *hw, struct ice_pg_cam_item *item);
void ice_pg_nm_cam_dump(struct ice_hw *hw, struct ice_pg_nm_cam_item *item);

struct ice_pg_cam_item *ice_pg_cam_table_get(struct ice_hw *hw);
struct ice_pg_cam_item *ice_pg_sp_cam_table_get(struct ice_hw *hw);

struct ice_pg_nm_cam_item *ice_pg_nm_cam_table_get(struct ice_hw *hw);
struct ice_pg_nm_cam_item *ice_pg_nm_sp_cam_table_get(struct ice_hw *hw);

struct ice_pg_cam_item *ice_pg_cam_match(struct ice_pg_cam_item *table,
					 int size, struct ice_pg_cam_key *key);
struct ice_pg_nm_cam_item *
ice_pg_nm_cam_match(struct ice_pg_nm_cam_item *table, int size,
		    struct ice_pg_cam_key *key);
#endif /* DPDK_SUPPORT || ADV_AVF_SUPPORT */
#endif /* !NO_FLEXP_SUPPORT */
#endif /* _ICE_PG_CAM_H_ */
