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


static void _proto_off_dump(struct ice_hw *hw, struct ice_proto_off *po,
			    int idx)
{
	ice_info(hw, "proto %d\n", idx);
	ice_info(hw, "\tpolarity = %d\n", po->polarity);
	ice_info(hw, "\tproto_id = %d\n", po->proto_id);
	ice_info(hw, "\toffset = %d\n", po->offset);
}

/**
 * ice_proto_grp_dump - dump a proto group item info
 * @hw: pointer to the hardware structure
 * @item: proto group item to dump
 */
void ice_proto_grp_dump(struct ice_hw *hw, struct ice_proto_grp_item *item)
{
	int i;

	ice_info(hw, "index = %d\n", item->idx);

	for (i = 0; i < ICE_PROTO_COUNT_PER_GRP; i++)
		_proto_off_dump(hw, &item->po[i], i);
}

/** The function parses a 22 bits Protocol entry with below format:
 *  BIT 0:	Polarity of Protocol Offset (po->polarity)
 *  BIT 1-8:	Protocol ID (po->proto_id)
 *  BIT 9-11:	reserved
 *  BIT 12-21:	Protocol Offset (po->offset)
 */
static void _proto_off_parse(struct ice_proto_off *po, u32 data)
{
	po->polarity = (data & 0x1) != 0;
	po->proto_id = (u8)((data >> 1) & 0xff);
	po->offset = (u16)((data >> 12) & 0x3ff);
}

/** The function parses a 192 bits Protocol Group Table entry with below
 *  format:
 *  BIT 0-21:	Protocol 0 (grp->po[0])
 *  BIT 22-43:	Protocol 1 (grp->po[1])
 *  BIT 44-65:	Protocol 2 (grp->po[2])
 *  BIT 66-87:	Protocol 3 (grp->po[3])
 *  BIT 88-109:	Protocol 4 (grp->po[4])
 *  BIT 110-131:Protocol 5 (grp->po[5])
 *  BIT 132-153:Protocol 6 (grp->po[6])
 *  BIT 154-175:Protocol 7 (grp->po[7])
 *  BIT 176-191:reserved
 */
static void _proto_grp_parse_item(struct ice_hw *hw, u16 idx, void *item,
				  void *data, int size)
{
	struct ice_proto_grp_item *grp = (struct ice_proto_grp_item *)item;
	u8 *buf = (u8 *)data;
	u32 d32;

	grp->idx = idx;

	d32 = *(u32 *)buf;
	_proto_off_parse(&grp->po[0], d32);

	d32 = (*(u32 *)&buf[2] >> 6);
	_proto_off_parse(&grp->po[1], d32);

	d32 = (*(u32 *)&buf[5] >> 4);
	_proto_off_parse(&grp->po[2], d32);

	d32 = (*(u32 *)&buf[8] >> 2);
	_proto_off_parse(&grp->po[3], d32);

	d32 = *(u32 *)&buf[11];
	_proto_off_parse(&grp->po[4], d32);

	d32 = (*(u32 *)&buf[13] >> 6);
	_proto_off_parse(&grp->po[5], d32);

	d32 = (*(u32 *)&buf[16] >> 4);
	_proto_off_parse(&grp->po[6], d32);

	d32 = (*(u32 *)&buf[19] >> 2);
	_proto_off_parse(&grp->po[7], d32);

	if (hw->debug_mask & ICE_DBG_PARSER)
		ice_proto_grp_dump(hw, grp);
}

/**
 * ice_proto_grp_table_get - create a proto group table
 * @hw: pointer to the hardware structure
 */
struct ice_proto_grp_item *ice_proto_grp_table_get(struct ice_hw *hw)
{
	return (struct ice_proto_grp_item *)
		ice_parser_create_table(hw, ICE_SID_RXPARSER_PROTO_GRP,
					sizeof(struct ice_proto_grp_item),
					ICE_PROTO_GRP_TABLE_SIZE,
					ice_parser_sect_item_get,
					_proto_grp_parse_item, false);
}
#endif /* DPDK_SUPPORT || ADV_AVF_SUPPORT */
#endif /* !NO_FLEXP_SUPPORT */
