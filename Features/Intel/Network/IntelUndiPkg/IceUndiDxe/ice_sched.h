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

#ifndef _ICE_SCHED_H_
#define _ICE_SCHED_H_

#include "ice_common.h"

#define ICE_SCHED_5_LAYERS	5
#define ICE_SCHED_9_LAYERS	9

#define ICE_QGRP_LAYER_OFFSET	2
#define ICE_VSI_LAYER_OFFSET	4
#define ICE_AGG_LAYER_OFFSET	6
#define ICE_SCHED_INVAL_LAYER_NUM	0xFF
/* Burst size is a 12 bits register that is configured while creating the RL
 * profile(s). MSB is a granularity bit and tells the granularity type
 * 0 - LSB bits are in 64 bytes granularity
 * 1 - LSB bits are in 1K bytes granularity
 */
#define ICE_64_BYTE_GRANULARITY			0
#define ICE_KBYTE_GRANULARITY			BIT(11)
#define ICE_MIN_BURST_SIZE_ALLOWED		64 /* In Bytes */
#define ICE_MAX_BURST_SIZE_ALLOWED \
	((BIT(11) - 1) * 1024) /* In Bytes */
#define ICE_MAX_BURST_SIZE_64_BYTE_GRANULARITY \
	((BIT(11) - 1) * 64) /* In Bytes */
#define ICE_MAX_BURST_SIZE_KBYTE_GRANULARITY	ICE_MAX_BURST_SIZE_ALLOWED

#define ICE_RL_PROF_ACCURACY_BYTES 128
#define ICE_RL_PROF_MULTIPLIER 10000
#define ICE_RL_PROF_TS_MULTIPLIER 32
#define ICE_RL_PROF_FRACTION 512

#define ICE_PSM_CLK_367MHZ_IN_HZ 367647059
#define ICE_PSM_CLK_416MHZ_IN_HZ 416666667
#define ICE_PSM_CLK_446MHZ_IN_HZ 446428571
#define ICE_PSM_CLK_390MHZ_IN_HZ 390625000

#ifdef BMSM_MODE
/* bit definitions per recipe */
#define ICE_RECIPE_BIT_INCL_IPG_AND_PREAMBLE        BIT(4)
#define ICE_RECIPE_BIT_INCL_OFFSET                  BIT(3)
#define ICE_RECIPE_BIT_INCL_ESP_TRAILER             BIT(2)
#define ICE_RECIPE_BIT_INCL_L2_PADDING              BIT(1)
#define ICE_RECIPE_BIT_INCL_CRC                     BIT(0)

/* protocol IDs from factory parsing program */
#define ICE_PROT_ID_MAC_OUTER_1             0x01
#define ICE_PROT_ID_MAC_OUTER_2             0x02
#define ICE_PROT_ID_MAC_INNER_LAST          0x04
#define ICE_PROT_ID_IPV4_OUTER_1            0x20
#define ICE_PROT_ID_IPV4_INNER_LAST         0x21
#define ICE_PROT_ID_IPV6_OUTER_1            0x28
#define ICE_PROT_ID_IPV6_INNER_LAST         0x29

/* Packet adjustment profile ID */
#define ICE_ADJ_PROFILE_ID 0
#define ICE_DWORDS_PER_ADJ 8
#endif /* BMSM_MODE */

#ifndef NO_UNUSED_SCHED_CODE
struct rl_profile_params {
	u32 bw;			/* in Kbps */
	u16 rl_multiplier;
	u16 wake_up_calc;
	u16 rl_encode;
};
#endif /* !NO_UNUSED_SCHED_CODE */

/* BW rate limit profile parameters list entry along
 * with bandwidth maintained per layer in port info
 */
struct ice_aqc_rl_profile_info {
	struct ice_aqc_rl_profile_elem profile;
	struct LIST_ENTRY_TYPE list_entry;
	u32 bw;			/* requested */
	u16 prof_id_ref;	/* profile ID to node association ref count */
};

struct ice_sched_agg_vsi_info {
	struct LIST_ENTRY_TYPE list_entry;
	ice_declare_bitmap(tc_bitmap, ICE_MAX_TRAFFIC_CLASS);
	u16 vsi_handle;
	/* save aggregator VSI TC bitmap */
	ice_declare_bitmap(replay_tc_bitmap, ICE_MAX_TRAFFIC_CLASS);
};

struct ice_sched_agg_info {
	struct LIST_HEAD_TYPE agg_vsi_list;
	struct LIST_ENTRY_TYPE list_entry;
	ice_declare_bitmap(tc_bitmap, ICE_MAX_TRAFFIC_CLASS);
	u32 agg_id;
	enum ice_agg_type agg_type;
	/* bw_t_info saves aggregator BW information */
	struct ice_bw_type_info bw_t_info[ICE_MAX_TRAFFIC_CLASS];
	/* save aggregator TC bitmap */
	ice_declare_bitmap(replay_tc_bitmap, ICE_MAX_TRAFFIC_CLASS);
};

/* FW AQ command calls */
#ifndef NO_UNUSED_DEBUG_CODE
enum ice_status
ice_aq_get_dflt_topo(struct ice_hw *hw, u8 lport,
		     struct ice_aqc_get_topo_elem *buf, u16 buf_size,
		     u8 *num_branches, struct ice_sq_cd *cd);
#endif /* !NO_UNUSED_DEBUG_CODE */
#if !defined(NO_UNUSED_SCHED_CODE) || defined(AE_DRIVER)
enum ice_status
ice_aq_query_rl_profile(struct ice_hw *hw, u16 num_profiles,
			struct ice_aqc_rl_profile_elem *buf, u16 buf_size,
			struct ice_sq_cd *cd);
enum ice_status
ice_aq_cfg_node_attr(struct ice_hw *hw, u16 num_nodes,
		     struct ice_aqc_node_attr_elem *buf, u16 buf_size,
		     struct ice_sq_cd *cd);
enum ice_status
ice_aq_cfg_l2_node_cgd(struct ice_hw *hw, u16 num_nodes,
		       struct ice_aqc_cfg_l2_node_cgd_elem *buf, u16 buf_size,
		       struct ice_sq_cd *cd);
#endif /* !NO_UNUSED_SCHED_CODE || AE_DRIVER */
#if defined(AE_DRIVER)
enum ice_status
ice_aq_get_l2_node_phys_id(struct ice_hw *hw, u16 num_nodes,
			   struct ice_aqc_get_l2_node_phys_id_elem *buf,
			   u16 buf_size, struct ice_sq_cd *cd);
#endif /* (SWITCH_MODE && !NO_UNUSED_SCHED_CODE) || AE_DRIVER */
enum ice_status
ice_aq_move_sched_elems(struct ice_hw *hw, u16 grps_req,
			struct ice_aqc_move_elem *buf, u16 buf_size,
			u16 *grps_movd, struct ice_sq_cd *cd);
#ifdef AE_DRIVER
enum ice_status
ice_aq_suspend_sched_elems(struct ice_hw *hw, u16 elems_req, __le32 *buf,
			   u16 buf_size, u16 *elems_ret, struct ice_sq_cd *cd);
enum ice_status
ice_aq_resume_sched_elems(struct ice_hw *hw, u16 elems_req, __le32 *buf,
			  u16 buf_size, u16 *elems_ret, struct ice_sq_cd *cd);
#endif /* AE_DRIVER */
enum ice_status
ice_aq_query_sched_elems(struct ice_hw *hw, u16 elems_req,
			 struct ice_aqc_txsched_elem_data *buf, u16 buf_size,
			 u16 *elems_ret, struct ice_sq_cd *cd);
#ifdef AE_DRIVER
enum ice_status
ice_aq_add_sched_elems(struct ice_hw *hw, u16 grps_req,
		       struct ice_aqc_add_elem *buf, u16 buf_size,
		       u16 *grps_added, struct ice_sq_cd *cd);
enum ice_status
ice_aq_cfg_sched_elems(struct ice_hw *hw, u16 elems_req,
		       struct ice_aqc_txsched_elem_data *buf, u16 buf_size,
		       u16 *elems_cfgd, struct ice_sq_cd *cd);
enum ice_status
ice_aq_query_sched_res(struct ice_hw *hw, u16 buf_size,
		       struct ice_aqc_query_txsched_res_resp *buf,
		       struct ice_sq_cd *cd);
enum ice_status
ice_aq_delete_sched_elems(struct ice_hw *hw, u16 grps_req,
			  struct ice_aqc_delete_elem *buf, u16 buf_size,
			  u16 *grps_deleted, struct ice_sq_cd *cd);
enum ice_status
ice_aq_add_rl_profile(struct ice_hw *hw, u16 num_profiles,
		      struct ice_aqc_rl_profile_elem *buf, u16 buf_size,
		      u16 *num_profiles_added, struct ice_sq_cd *cd);
enum ice_status
ice_aq_remove_rl_profile(struct ice_hw *hw, u16 num_profiles,
			 struct ice_aqc_rl_profile_elem *buf, u16 buf_size,
			 u16 *num_profiles_removed, struct ice_sq_cd *cd);
#endif /* AE_DRIVER */
enum ice_status ice_sched_init_port(struct ice_port_info *pi);
#ifdef BMSM_MODE
enum ice_status ice_sched_add_dflt_l2_nodes(struct ice_port_info *pi);
enum ice_status ice_sched_clear_l2_nodes(struct ice_port_info *pi);
enum ice_status ice_sched_set_dflt_cgd_to_tc_map(struct ice_port_info *pi);
enum ice_status
ice_sched_copy_cgd(struct ice_port_info *src, struct ice_port_info *dst, u8 num_cgd);
#endif /* BMSM_MODE */
enum ice_status ice_sched_query_res_alloc(struct ice_hw *hw);
void ice_sched_get_psm_clk_freq(struct ice_hw *hw);

/* Functions to cleanup scheduler SW DB */
void ice_sched_clear_port(struct ice_port_info *pi);
void ice_sched_cleanup_all(struct ice_hw *hw);
void ice_sched_clear_agg(struct ice_hw *hw);

/* Get a scheduling node from SW DB for given TEID */
struct ice_sched_node *ice_sched_get_node(struct ice_port_info *pi, u32 teid);
#ifdef AE_DRIVER

/* Add a root node into SW DB */
enum ice_status
ice_sched_add_root_node(struct ice_port_info *port_info,
			struct ice_aqc_txsched_elem_data *info);
#endif
struct ice_sched_node *
ice_sched_find_node_by_teid(struct ice_sched_node *start_node, u32 teid);
/* Add a scheduling node into SW DB for given info */
enum ice_status
ice_sched_add_node(struct ice_port_info *pi, u8 layer,
		   struct ice_aqc_txsched_elem_data *info);
void ice_free_sched_node(struct ice_port_info *pi, struct ice_sched_node *node);
struct ice_sched_node *ice_sched_get_tc_node(struct ice_port_info *pi, u8 tc);
struct ice_sched_node *
ice_sched_get_free_qparent(struct ice_port_info *pi, u16 vsi_handle, u8 tc,
			   u8 owner);
enum ice_status
ice_sched_cfg_vsi(struct ice_port_info *pi, u16 vsi_handle, u8 tc, u16 maxqs,
		  u8 owner, bool enable);
enum ice_status ice_rm_vsi_lan_cfg(struct ice_port_info *pi, u16 vsi_handle);
#ifdef RDMA_SUPPORT
enum ice_status ice_rm_vsi_rdma_cfg(struct ice_port_info *pi, u16 vsi_handle);
#endif /* !RDMA_SUPPORT */
#ifndef NO_UNUSED_SCHED_CODE
struct ice_sched_node *
ice_sched_get_vsi_node(struct ice_port_info *pi, struct ice_sched_node *tc_node,
		       u16 vsi_handle);
#endif /* !NO_UNUSED_SCHED_CODE */
#ifndef NO_UNUSED_SCHED_CODE
bool ice_sched_is_tree_balanced(struct ice_hw *hw, struct ice_sched_node *node);
#endif /* !NO_UNUSED_SCHED_CODE */
#ifndef NO_UNUSED_SCHED_CODE
enum ice_status
ice_aq_query_node_to_root(struct ice_hw *hw, u32 node_teid,
			  struct ice_aqc_txsched_elem_data *buf, u16 buf_size,
			  struct ice_sq_cd *cd);
#endif /* NO_UNUSED_SCHED_CODE */

/* Tx scheduler rate limiter functions */
enum ice_status
ice_cfg_agg(struct ice_port_info *pi, u32 agg_id,
	    enum ice_agg_type agg_type, u8 tc_bitmap);
enum ice_status
ice_move_vsi_to_agg(struct ice_port_info *pi, u32 agg_id, u16 vsi_handle,
		    u8 tc_bitmap);
#ifndef NO_UNUSED_SCHED_CODE
enum ice_status ice_rm_agg_cfg(struct ice_port_info *pi, u32 agg_id);
#endif /* !NO_UNUSED_SCHED_CODE */
enum ice_status
ice_cfg_q_bw_lmt(struct ice_port_info *pi, u16 vsi_handle, u8 tc,
		 u16 q_handle, enum ice_rl_type rl_type, u32 bw);
enum ice_status
ice_cfg_q_bw_dflt_lmt(struct ice_port_info *pi, u16 vsi_handle, u8 tc,
		      u16 q_handle, enum ice_rl_type rl_type);
#ifndef NO_UNUSED_SCHED_CODE
enum ice_status
ice_cfg_tc_node_bw_lmt(struct ice_port_info *pi, u8 tc,
		       enum ice_rl_type rl_type, u32 bw);
enum ice_status
ice_cfg_tc_node_bw_dflt_lmt(struct ice_port_info *pi, u8 tc,
			    enum ice_rl_type rl_type);
#endif /* !NO_UNUSED_SCHED_CODE */
enum ice_status
ice_cfg_vsi_bw_lmt_per_tc(struct ice_port_info *pi, u16 vsi_handle, u8 tc,
			  enum ice_rl_type rl_type, u32 bw);
enum ice_status
ice_cfg_vsi_bw_dflt_lmt_per_tc(struct ice_port_info *pi, u16 vsi_handle, u8 tc,
			       enum ice_rl_type rl_type);
#ifndef NO_UNUSED_SCHED_CODE
enum ice_status
ice_cfg_agg_bw_lmt_per_tc(struct ice_port_info *pi, u32 agg_id, u8 tc,
			  enum ice_rl_type rl_type, u32 bw);
enum ice_status
ice_cfg_agg_bw_dflt_lmt_per_tc(struct ice_port_info *pi, u32 agg_id, u8 tc,
			       enum ice_rl_type rl_type);
enum ice_status
ice_cfg_vsi_bw_shared_lmt(struct ice_port_info *pi, u16 vsi_handle, u32 min_bw,
			  u32 max_bw, u32 shared_bw);
enum ice_status
ice_cfg_vsi_bw_no_shared_lmt(struct ice_port_info *pi, u16 vsi_handle);
enum ice_status
ice_cfg_agg_bw_shared_lmt(struct ice_port_info *pi, u32 agg_id, u32 min_bw,
			  u32 max_bw, u32 shared_bw);
enum ice_status
ice_cfg_agg_bw_no_shared_lmt(struct ice_port_info *pi, u32 agg_id);
enum ice_status
ice_cfg_agg_bw_shared_lmt_per_tc(struct ice_port_info *pi, u32 agg_id, u8 tc,
				 u32 min_bw, u32 max_bw, u32 shared_bw);
enum ice_status
ice_cfg_agg_bw_no_shared_lmt_per_tc(struct ice_port_info *pi, u32 agg_id,
				    u8 tc);
enum ice_status
ice_cfg_vsi_q_priority(struct ice_port_info *pi, u16 num_qs, u32 *q_ids,
		       u8 *q_prio);
#ifdef DPDK_SUPPORT
enum ice_status
ice_sched_cfg_sibl_node_prio_lock(struct ice_port_info *pi,
				  struct ice_sched_node *node, u8 priority);
enum ice_status
ice_cfg_q_bw_alloc(struct ice_port_info *pi, u16 vsi_handle, u8 tc,
		   u16 q_handle, enum ice_rl_type rl_type, u32 bw_alloc);
#endif /* DPDK_SUPPORT */
enum ice_status
ice_cfg_vsi_bw_alloc(struct ice_port_info *pi, u16 vsi_handle, u8 ena_tcmap,
		     enum ice_rl_type rl_type, u8 *bw_alloc);
enum ice_status
ice_cfg_agg_vsi_priority_per_tc(struct ice_port_info *pi, u32 agg_id,
				u16 num_vsis, u16 *vsi_handle_arr,
				u8 *node_prio, u8 tc);
enum ice_status
ice_cfg_agg_bw_alloc(struct ice_port_info *pi, u32 agg_id, u8 ena_tcmap,
		     enum ice_rl_type rl_type, u8 *bw_alloc);
bool
ice_sched_find_node_in_subtree(struct ice_hw *hw, struct ice_sched_node *base,
			       struct ice_sched_node *node);
#ifdef AE_DRIVER
enum ice_status
ice_sched_bw_to_rl_profile(struct ice_hw *hw, u32 bw,
			   struct ice_aqc_rl_profile_elem *profile);
#endif
enum ice_status
ice_sched_set_agg_bw_dflt_lmt(struct ice_port_info *pi, u16 vsi_handle);
#endif /* !NO_UNUSED_SCHED_CODE */
enum ice_status
ice_sched_set_node_bw_lmt_per_tc(struct ice_port_info *pi, u32 id,
				 enum ice_agg_type agg_type, u8 tc,
				 enum ice_rl_type rl_type, u32 bw);
#ifndef NO_UNUSED_SCHED_CODE
enum ice_status
ice_sched_set_vsi_bw_shared_lmt(struct ice_port_info *pi, u16 vsi_handle,
				u32 min_bw, u32 max_bw, u32 shared_bw);
enum ice_status
ice_sched_set_agg_bw_shared_lmt(struct ice_port_info *pi, u32 agg_id, u32 min_bw,
				u32 max_bw, u32 shared_bw);
enum ice_status
ice_sched_set_agg_bw_shared_lmt_per_tc(struct ice_port_info *pi, u32 agg_id,
				       u8 tc, u32 min_bw, u32 max_bw,
				       u32 shared_bw);
enum ice_status
ice_sched_cfg_sibl_node_prio(struct ice_port_info *pi,
			     struct ice_sched_node *node, u8 priority);
#ifdef BMSM_MODE
enum ice_status
ice_cfg_root_node_bw_lmt(struct ice_port_info *pi, u32 bw,
			 enum ice_rl_type rl_type);
#endif /* BMSM_MODE */
enum ice_status
ice_cfg_tc_node_bw_alloc(struct ice_port_info *pi, u8 tc,
			 enum ice_rl_type rl_type, u8 bw_alloc);
#endif /* !NO_UNUSED_SCHED_CODE */
enum ice_status ice_cfg_rl_burst_size(struct ice_hw *hw, u32 bytes);
void ice_sched_replay_agg_vsi_preinit(struct ice_hw *hw);
void ice_sched_replay_agg(struct ice_hw *hw);
enum ice_status ice_sched_replay_tc_node_bw(struct ice_port_info *pi);
enum ice_status ice_replay_vsi_agg(struct ice_hw *hw, u16 vsi_handle);
enum ice_status ice_sched_replay_root_node_bw(struct ice_port_info *pi);
enum ice_status
ice_sched_replay_q_bw(struct ice_port_info *pi, struct ice_q_ctx *q_ctx);

#ifdef BMSM_MODE
void ice_cfg_pkt_len_adj_profiles(struct ice_hw *hw);
#endif /* BMSM_MODE */
#endif /* _ICE_SCHED_H_ */
