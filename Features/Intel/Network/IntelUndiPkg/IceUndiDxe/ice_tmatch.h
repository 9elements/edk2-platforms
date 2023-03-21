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

#ifndef _ICE_TMATCH_H_
#define _ICE_TMATCH_H_

#ifndef NO_FLEXP_SUPPORT
static inline
bool ice_ternary_match_byte(u8 key, u8 key_inv, u8 pat)
{
	u8 k1, k2, v;
	int i;

	for (i = 0; i < 8; i++) {
		k1 = (u8)(key & (1 << i));
		k2 = (u8)(key_inv & (1 << i));
		v = (u8)(pat & (1 << i));

		if (k1 != 0 && k2 != 0)
			continue;
		if (k1 == 0 && k2 == 0)
			return false;

		if (k1 == v)
			return false;
	}

	return true;
}

static inline
bool ice_ternary_match(const u8 *key, const u8 *key_inv,
		       const u8 *pat, int len)
{
	int i;

	for (i = 0; i < len; i++)
		if (!ice_ternary_match_byte(key[i], key_inv[i], pat[i]))
			return false;

	return true;
}

#endif /* !NO_FLEXP_SUPPORT */
#endif /* _ICE_TMATCH_H_ */
