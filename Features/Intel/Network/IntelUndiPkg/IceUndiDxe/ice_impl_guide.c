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

/*! \mainpage Intel FIXME Shared Code Implementation Guide
 *
 *\section secA  Operating System Interface
 * The Shared Code is common code designed to coordinate, and
 * make common, the initialization and other hardware tasks.
 *
 * \section sec2 Operating System Dependent Files
 * Each driver is required to implement one or two files, ice_osdep.c and
 * ice_osdep.h for the operating system dependent portions of the shared code.
 * The following are required in the osdep file(s) (in header file if
 * implemented as a macro/inline-function or in the C file if implemented as a
 * function with a prototype in the header file).
 *
 * \section sec3 Data Types/structures
 * \htmlonly <br>
 * __le16<br>
 * __le32<br>
 * __le64<br>
 * <br>
 * struct ice_dma_mem {<br>
 * &emsp;void *va;<br>
 * &emsp;&lt;os-specific physical address type&gt; pa;<br>
 * &emsp;&lt;os-specific size type&gt; size;<br>
 * &emsp;&lt;other OS-specific data...&gt;<br>
 * }<br>
 * <br>
 * struct ice_lock {<br>
 * &emsp;&lt;os specific lock type&gt; lock;<br>
 * }<br>
 * <br>
 * LIST_ENTRY_TYPE	(list entry, e.g. list_head on Linux, _LIST_ENTRY on Windows)<br>
 * LIST_HEAD_TYPE	(list head, e.g. list_head on Linux, _LIST_ENTRY on Windows)<br>
 * \endhtmlonly
 *
 * \section sec4 Functions/macros
 * \htmlonly <br>
 * <bold>See ice_common.c:ice_init_hw() for some examples</bold><br>
 * <br>
 * STATIC<br>
 * CPU_TO_BE64(a)<br>
 * CPU_TO_BE32(a)<br>
 * CPU_TO_BE16(a)<br>
 * CPU_TO_LE64(a)<br>
 * CPU_TO_LE32(a)<br>
 * CPU_TO_LE16(a)<br>
 * LE64_TO_CPU(a)<br>
 * LE32_TO_CPU(a)<br>
 * LE16_TO_CPU(a)<br>
 * offsetof(_type, _field)<br>
 * FIELD_SIZEOF(_type, _field)<br>
 * ARRAY_SIZE(_array)<br>
 * NTOHL(a)<br>
 * NTOHS(a)<br>
 * HTONL(a)<br>
 * HTONS(a)<br>
 * SNPRINTF(buf, size, fmt, ...)<br>
 * <br>
 * u32 rd32(struct ice_hw *, reg_offset)<br>
 * void wr32(struct ice_hw *, reg_offset, u32 value)<br>
 * u64 rd64(struct ice_hw *, reg_offset)<br>
 * void wr64(struct ice_hw *, reg_offset, u64 value)<br>
 * <br>
 * void ice_flush(struct ice_hw *)<br>
 * <br>
 * void ice_debug(struct ice_hw *hw, u32 mask, char *format, ...)<br>
 * void ice_debug_array(struct ice_hw *hw, u32 mask, u32 rowsize, u32 groupsize, char *buf, size_t len)<br>
 * void ice_debug_fw_log(struct ice_hw *hw, u32 mask, u32 rowsize, u32 groupsize, char *buf, size_t len)<br>
 * <br>
 * void ice_info(struct ice_hw *hw, char *format, ...)<br>
 * void ice_info_fwlog(struct ice_hw *hw, u32 rowsize, u32 groupsize, char *buf, size_t len)<br>
 * <br>
 * void ice_warn(struct ice_hw *hw, char *format, ...)<br>
 * Like ice_info but may log the message at a higher warning level<br>
 * <br>
 * Delay functions - bool sleep indicates sleep (true) or busy-wait (false)<br>
 * void ice_usec_delay(unsigned long usecs, bool sleep)<br>
 * void ice_msec_delay(unsigned long msecs, bool sleep)<br>
 * <br>
 * void *ice_memset(void *addr, int c, size_t n, ice_memset_type direction)<br>
 * void *ice_memcpy(void *d, const void *s, size_t n, ice_memcpy_type dir)<br>
 * void *ice_memdup(struct ice_hw *hw, const void *s, size_t n, ice_memcpy_type dir)<br>
 * <br>
 * Memory allocation functions - expected to provide zero'ed memory<br>
 * void *ice_alloc_dma_mem(struct ice_hw *hw, struct ice_dma_mem *m, u64 size)<br>
 * void *ice_malloc(struct ice_hw *hw, size)<br>
 * void *ice_calloc(struct ice_hw *hw, cnt, size)<br>
 * <br>
 * void ice_free_dma_mem(struct ice_hw *hw, struct ice_dma_mem *m)<br>
 * void ice_free(struct ice_hw *, void *) - should not fail if void pointer is NULL<br>
 * <br>
 * void ice_init_lock(struct ice_lock *lock);<br>
 * void ice_acquire_lock(struct ice_lock *lock);<br>
 * void ice_release_lock(struct ice_lock *lock);<br>
 * void ice_destroy_lock(struct ice_lock *lock);<br>
 * <br>
 * void ice_declare_bitmap(name, u16 size);<br>
 * void ice_is_bit_set(ice_bitmap_t *bmp, u16 bit);<br>
 * void ice_clear_bit(u16 bit, ice_bitmap_t *bmp);<br>
 * void ice_set_bit(u16 bit, ice_bitmap_t *bmp);<br>
 * bool ice_test_and_clear_bit(u16 bit, ice_bitmap_t *bmp);<br>
 * bool ice_test_and_set_bit(u16 bit, ice_bitmap_t *bmp);<br>
 * void ice_zero_bitmap(ice_bitmap_t *bmp, u16 size);<br>
 * int ice_and_bitmap(ice_bitmap_t *dst, ice_bitmap_t *bmp1, ice_bitmap_t *bmp2, u16 size);<br>
 * void ice_or_bitmap(ice_bitmap_t *dst, ice_bitmap_t *bmp1, ice_bitmap_t *bmp2, u16 size);<br>
 * u16 ice_find_next_bit(ice_bitmap_t *bmp, u16 size, u16 offset);<br>
 * u16 ice_find_first_bit(ice_bitmap_t *bmp, u16 size);<br>
 * bool ice_is_any_bit_set(ice_bitmap_t *bmp, u16 size);<br>
 * void ice_cp_bitmap(ice_bitmap_t *dst, ice_bitmap_t *src, u16 size);<br>
 * bool ice_cmp_bitmap(ice_bitmap_t *bmp1, ice_bitmap_t *bmp2, u16 size);<br>
 * Note that while most of the bitmap functions are careful to avoid accessing
 * bits beyond the specified size, ice_zero_bitmap and ice_cp_bitmap will not.
 * If this behavior is not desirable for certain code locations, consider
 * implementing ice_clear_bitmap and ice_cp_bitmap_clear_tail.<br>
 * <br>
 * u8 ice_hweight8(u8 weight) - determine hamming weight of an 8-bit value<br>
 * u8 ice_hweight32(u32 weight) - determine hamming weight of an 32-bit value<br>
 * <br>
 * bool ice_is_pow2(u64 val) - check val is power of 2 (zero is not) [OPTIONAL]<br>
 * int ice_ilog2(u64 val) - calculates integer log base 2 of val (rounds down) [OPTIONAL]<br>
 * <br>
 * unsigned int ice_fls(unsigned int val) - return the 1 based index of the MSB [OPTIONAL]<br>
 * <br>
 * <bold>doubly-linked list management macros:</bold><br>
 * INIT_LIST_HEAD(struct LIST_HEAD_TYPE *head)<br>
 * LIST_EMPTY(const struct LIST_HEAD_TYPE *head)<br>
 * LIST_ADD(struct LIST_ENTRY_TYPE *entry, struct LIST_HEAD_TYPE *head)<br>
 * LIST_ADD_AFTER(struct LIST_ENTRY_TYPE *entry, struct LIST_ENTRY_TYPE *elem)<br>
 * LIST_ADD_TAIL(struct LIST_ENTRY_TYPE *entry, struct LIST_HEAD_TYPE *head)<br>
 * LIST_FIRST_ENTRY(struct LIST_HEAD_TYPE *head, &lt;name of struct&gt;, &lt;name of LIST_ENTRY_TYPE member in struct&gt;)<br>
 * LIST_NEXT_ENTRY(struct LIST_ENTRY_TYPE *entry, &lt;name of struct&gt;, &lt;name of LIST_ENTRY_TYPE member in struct&gt;)<br>
 * LIST_LAST_ENTRY(struct LIST_HEAD_TYPE *head, &lt;name of struct&gt;, &lt;name of LIST_ENTRY_TYPE member in struct&gt;)<br>
 * LIST_DEL(struct LIST_ENTRY_TYPE *entry)<br>
 * LIST_FOR_EACH_ENTRY(&lt;&amp;struct used as iterator&gt;, struct LIST_HEAD_TYPE *head, &lt;name of struct&gt;, &lt;name of LIST_ENTRY_TYPE member in struct&gt;)<br>
 * Note: it is not safe to remove list entries in a LIST_FOR_EACH_ENTRY() loop<br>
 * LIST_FOR_EACH_ENTRY_SAFE(&lt;&amp;struct used as iterator&gt;, &lt;&amp;struct used as iterator&gt;struct LIST_ENTRY_TYPE *entry;, struct LIST_HEAD_TYPE *head, &lt;name of struct&gt;, &lt;name of LIST_ENTRY_TYPE member in struct&gt;)<br>
 * LIST_REPLACE_INIT(struct LIST_HEAD_TYPE *old_head, struct LIST_HEAD_TYPE *new_head)<br>
 * <br>
 * <bold>unused function parameter macros</bold><br>
 * __ALWAYS_UNUSED<br>
 * Define to a compiler attribute to mark an unused parameter<br>
 * <br>
 * <bold>array initialization with indexes macro</bold><br>
 * ice_arr_elem_idx(idx, const_val)<br>
 * <br>
 *
 * \section sec5 List implementation details
 * The LIST macros are defined to implement a doubly-linked list which embeds
 * the LIST_ENTRY structures as elements of the items linked to the list. The
 * macros assume that pointer arithmetic can be used to extract the container
 * structure from the LIST_ENTRY element and the structure type.
 * <br>
 * INIT_LIST_HEAD is expected to be able to initialize a pointer to a new
 * list.
 * <br>
 * LIST_EMPTY is called to determine if a list pointed to by a given list head
 * contains any elements. Calling LIST_EMPTY on an uninitialized list head
 * results in undefined implementation specific behavior.
 * <br>
 * LIST_ADD is called to add an element to the front of a list pointed to by
 * a given list head. It is assumed that LIST_ADD will perform any required
 * initialization for the LIST_ENTRY_TYPE structure.
 * <br>
 * LIST_ADD_AFTER is called to insert a new element into the list after the
 * given element. It is assumed that LIST_ADD_AFTER will perform any required
 * initialization for the LIST_ENTRY_TYPE structure.
 * <br>
 * LIST_FIRST_ENTRY is called to obtain a pointer to the structure containing
 * the first LIST_ENTRY_TYPE element of a list pointed to by the given list
 * head. Calling LIST_FIRST_ENTRY with an empty or uninitialized list results
 * in undefined implementation specific behavior.
 * <br>
 * LIST_LAST_ENTRY is called to obtain a pointer to the structure containing
 * the last LIST_ENTRY_TYPE element of a list pointed to by the given list
 * head. Calling LIST_LAST_ENTRY with an empty or uninitialized list results
 * in undefined implementation specific behavior.
 * <br>
 * LIST_ADD_TAIL is called to insert a new element to the end of list pointed
 * by the given head. It is assumed that LIST_ADD_TAIL will perform any
 * required initialization for the LIST_ENTRY_TYPE structure.
 * <br>
 * LIST_NEXT_ENTRY is called to obtain a pointer to the structure containing
 * the next LIST_ENTRY_TYPE element in the list, given a pointer to the current
 * structure.
 * <br>
 * LIST_DEL is called to remove an element from its associated list.
 * <br>
 * LIST_FOR_EACH_ENTRY is used to loop through every element of a list, using
 * a pointer of the containing type as an iterator. It is expected to have
 * semantics similar to for loops, and can take either a single or block
 * statement following it. Calling LIST_DEL on the iterator is not safe and
 * results in undefined implementation specific behavior. If deleting elements
 * from the list while iterating is required, use LIST_FOR_EACH_ENTRY_SAFE
 * instead.
 * <br>
 * LIST_FOR_EACH_ENTRY_SAFE is used to loop through every element of a list
 * guaranteeing safety to delete the iterator element even during the
 * iteration. It requires two temporary pointers both of the struct type used
 * as the iterator. If the ability to remove the iterated element from the
 * list is required, then LIST_FOR_EACH_ENTRY_SAFE must be used instead of
 * LIST_FOR_EACH_ENTRY.
 * <br>
 * LIST_REPLACE_INIT is used to replace old head by a new head. This will also
 * reinitialize the old head to make it a empty list head. The new list does
 * not have to be initialized for this function.
 * <br>
 * \endhtmlonly
 */
