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

#ifndef _ICE_DEFS_H_
#define _ICE_DEFS_H_

#ifndef ETH_ALEN
#define ETH_ALEN	6
#endif /* ETH_ALEN */

#ifndef ETH_HEADER_LEN
#define ETH_HEADER_LEN	14
#endif /* ETH_HEADER_LEN */

#ifndef BIT
#define BIT(a) (1UL << (a))
#endif /* BIT */
#ifndef BIT_ULL
#define BIT_ULL(a) (1ULL << (a))
#endif /* BIT_ULL */

#ifndef BITS_PER_BYTE
#define BITS_PER_BYTE	8
#endif

#ifndef _FORCE_
#define _FORCE_
#endif

#define ICE_BYTES_PER_WORD	2
#define ICE_BYTES_PER_DWORD	4
#define ICE_MAX_TRAFFIC_CLASS	8
#ifdef ADQ_SUPPORT
#define ICE_CHNL_MAX_TC		16
#endif /* ADQ_SUPPORT */

#ifndef DIVIDE_AND_ROUND_UP
#define DIVIDE_AND_ROUND_UP(a, b) (((a) + (b) - 1) / (b))
#endif

#ifndef ROUND_UP
/**
 * ROUND_UP - round up to next arbitrary multiple (not a power of 2)
 * @a: value to round up
 * @b: arbitrary multiple
 *
 * Round up to the next multiple of the arbitrary b.
 * Note, when b is a power of 2 use ICE_ALIGN() instead.
 */
#define ROUND_UP(a, b)	((b) * DIVIDE_AND_ROUND_UP((a), (b)))
#endif

#ifndef MIN_T
#define MIN_T(_t, _a, _b)	min((_t)(_a), (_t)(_b))
#endif

#ifndef IS_ASCII
#define IS_ASCII(_ch)	((_ch) < 0x80)
#endif

#ifdef C99
#ifdef STRUCT_HACK_VAR_LEN
#undef STRUCT_HACK_VAR_LEN
#endif /* STRUCT_HACK_VAR_LEN */
#define STRUCT_HACK_VAR_LEN
#ifndef ice_struct_size
/**
 * ice_struct_size - size of struct with C99 flexible array member
 * @ptr: pointer to structure
 * @field: flexible array member (last member of the structure)
 * @num: number of elements of that flexible array member
 */
#define ice_struct_size(ptr, field, num) \
	(sizeof(*(ptr)) + sizeof(*(ptr)->field) * (num))
#endif /* ice_struct_size */
#else /* !C99 */
#ifndef STRUCT_HACK_VAR_LEN
#define STRUCT_HACK_VAR_LEN     1
#elif !((STRUCT_HACK_VAR_LEN == 0) || (STRUCT_HACK_VAR_LEN == 1))
#error STRUCT_HACK_VAR_LEN must be 0 or 1
#endif /* STRUCT_HACK_VAR_LEN */
#ifndef ice_struct_size
/**
 * ice_struct_size - size of struct with variable-length object as last member
 * @ptr: pointer to structure
 * @field: variable-length object (last member of the structure)
 * @num: number of elements of that variable-length object (array)
 */
#define ice_struct_size(ptr, field, num) \
	(sizeof(*(ptr)) + sizeof(*(ptr)->field) * ((num) - STRUCT_HACK_VAR_LEN))
#endif /* !ice_struct_size */
#endif /* !C99 */

#ifndef FLEX_ARRAY_SIZE
#define FLEX_ARRAY_SIZE(_ptr, _mem, cnt) ((cnt) * sizeof(_ptr->_mem[0]))
#endif /* FLEX_ARRAY_SIZE */

#endif /* _ICE_DEFS_H_ */
