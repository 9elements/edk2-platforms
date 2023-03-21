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

#define ICE_PTYPE_MK_TCAM_TABLE_SIZE 1024

/**
 * ice_ptype_mk_tcam_dump - dump an ptype marker tcam info_
 * @hw: pointer to the hardware structure
 * @item: ptype marker tcam to dump
 */
void ice_ptype_mk_tcam_dump(struct ice_hw *hw,
			    struct ice_ptype_mk_tcam_item *item)
{
	int i;

	ice_info(hw, "address = %d\n", item->address);
	ice_info(hw, "ptype = %d\n", item->ptype);
	ice_info(hw, "key    :");
	for (i = 0; i < 10; i++)
		ice_info(hw, "%02x ", item->key[i]);
	ice_info(hw, "\n");
	ice_info(hw, "key_inv:");
	for (i = 0; i < 10; i++)
		ice_info(hw, "%02x ", item->key_inv[i]);
	ice_info(hw, "\n");
}

static void _parse_ptype_mk_tcam_item(struct ice_hw *hw, u16 idx, void *item,
				      void *data, int size)
{
	ice_parse_item_dflt(hw, idx, item, data, size);

	if (hw->debug_mask & ICE_DBG_PARSER)
		ice_ptype_mk_tcam_dump(hw,
				       (struct ice_ptype_mk_tcam_item *)item);
}

/**
 * ice_ptype_mk_tcam_table_get - create a ptype marker tcam table
 * @hw: pointer to the hardware structure
 */
struct ice_ptype_mk_tcam_item *ice_ptype_mk_tcam_table_get(struct ice_hw *hw)
{
	return (struct ice_ptype_mk_tcam_item *)
		ice_parser_create_table(hw, ICE_SID_RXPARSER_MARKER_PTYPE,
					sizeof(struct ice_ptype_mk_tcam_item),
					ICE_PTYPE_MK_TCAM_TABLE_SIZE,
					ice_parser_sect_item_get,
					_parse_ptype_mk_tcam_item, true);
}

/**
 * ice_ptype_mk_tcam_match - match a pattern on a ptype marker tcam table
 * @table: ptype marker tcam table to search
 * @pat: pattern to match
 * @len: length of the pattern
 */
struct ice_ptype_mk_tcam_item *
ice_ptype_mk_tcam_match(struct ice_ptype_mk_tcam_item *table,
			u8 *pat, int len)
{
	int i;

	for (i = 0; i < ICE_PTYPE_MK_TCAM_TABLE_SIZE; i++) {
		struct ice_ptype_mk_tcam_item *item = &table[i];

		if (ice_ternary_match(item->key, item->key_inv, pat, len))
			return item;
	}

	return NULL;
}
#endif /* DPDK_SUPPORT || ADV_AVF_SUPPORT */
#endif /* !NO_FLEXP_SUPPORT */
