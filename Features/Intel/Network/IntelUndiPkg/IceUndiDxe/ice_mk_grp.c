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

#ifndef NO_FLEXP_SUPPORT
#if defined(DPDK_SUPPORT) || defined(ADV_AVF_SUPPORT)
#include "ice_common.h"
#include "ice_parser_util.h"

#define ICE_MK_GRP_TABLE_SIZE 128
#define ICE_MK_COUNT_PER_GRP 8

/**
 * ice_mk_grp_dump - dump an marker group item info
 * @hw: pointer to the hardware structure
 * @item: marker group item to dump
 */
void ice_mk_grp_dump(struct ice_hw *hw, struct ice_mk_grp_item *item)
{
	int i;

	ice_info(hw, "index = %d\n", item->idx);
	ice_info(hw, "markers: ");
	for (i = 0; i < ICE_MK_COUNT_PER_GRP; i++)
		ice_info(hw, "%d ", item->markers[i]);
	ice_info(hw, "\n");
}

static void _mk_grp_parse_item(struct ice_hw *hw, u16 idx, void *item,
			       void *data, int size)
{
	struct ice_mk_grp_item *grp = (struct ice_mk_grp_item *)item;
	u8 *buf = (u8 *)data;
	int i;

	grp->idx = idx;

	for (i = 0; i < ICE_MK_COUNT_PER_GRP; i++)
		grp->markers[i] = buf[i];

	if (hw->debug_mask & ICE_DBG_PARSER)
		ice_mk_grp_dump(hw, grp);
}

/**
 * ice_mk_grp_table_get - create a marker group table
 * @hw: pointer to the hardware structure
 */
struct ice_mk_grp_item *ice_mk_grp_table_get(struct ice_hw *hw)
{
	return (struct ice_mk_grp_item *)
		ice_parser_create_table(hw, ICE_SID_RXPARSER_MARKER_GRP,
					sizeof(struct ice_mk_grp_item),
					ICE_MK_GRP_TABLE_SIZE,
					ice_parser_sect_item_get,
					_mk_grp_parse_item, false);
}
#endif /* DPDK_SUPPORT || ADV_AVF_SUPPORT */
#endif /* !NO_FLEXP_SUPPORT */
