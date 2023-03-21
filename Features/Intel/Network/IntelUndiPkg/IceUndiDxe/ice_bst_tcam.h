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

#ifndef _ICE_BST_TCAM_H_
#define _ICE_BST_TCAM_H_

#ifndef NO_FLEXP_SUPPORT
#if defined(DPDK_SUPPORT) || defined(ADV_AVF_SUPPORT)
#include "ice_imem.h"

struct ice_bst_tcam_item {
	u16 address;
	u8 key[20];
	u8 key_inv[20];
	u8 hit_idx_grp;
	u8 pg_pri;
	struct ice_np_keybuilder np_kb;
	struct ice_pg_keybuilder pg_kb;
	struct ice_alu alu0;
	struct ice_alu alu1;
	struct ice_alu alu2;
};

void ice_bst_tcam_dump(struct ice_hw *hw, struct ice_bst_tcam_item *item);

struct ice_bst_tcam_item *ice_bst_tcam_table_get(struct ice_hw *hw);

struct ice_lbl_item *ice_bst_lbl_table_get(struct ice_hw *hw);

struct ice_bst_tcam_item *
ice_bst_tcam_match(struct ice_bst_tcam_item *tcam_table, u8 *pat);
struct ice_bst_tcam_item *
ice_bst_tcam_search(struct ice_bst_tcam_item *tcam_table,
		    struct ice_lbl_item *lbl_table,
		    const char *prefix, u16 *start);
#endif /* DPDK_SUPPORT || ADV_AVF_SUPPORT */
#endif /* !NO_FLEXP_SUPPORT */
#endif /*_ICE_BST_TCAM_H_ */
