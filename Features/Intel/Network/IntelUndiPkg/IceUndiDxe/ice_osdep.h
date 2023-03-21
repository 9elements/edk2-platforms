/**************************************************************************

Copyright (c) 2016 - 2021, Intel Corporation. All Rights Reserved.

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
#ifndef _ICE_OSDEP_H_
#define _ICE_OSDEP_H_

#include <Uefi.h>
#include <Base.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/SynchronizationLib.h>
#include <Library/UefiLib.h>
#include <Library/PrintLib.h>
#include "Dma.h"

/* UNCOMMENT THE FOLLOWING DEFINE TO ENABLE DEBUGPRINTS IN SHARED CODE */
//#define SC_DEBUG

#ifdef SC_DEBUG
#include <Library/DebugLib.h>
#include "ice_type.h"

/* Possible DBG LEVELS from ice_type.h

  ICE_DBG_INIT               BIT_ULL(1)
  ICE_DBG_RELEASE            BIT_ULL(2)

  ICE_DBG_LINK               BIT_ULL(4)
  ICE_DBG_PHY                BIT_ULL(5)
  ICE_DBG_QCTX               BIT_ULL(6)
  ICE_DBG_NVM                BIT_ULL(7)
  ICE_DBG_LAN                BIT_ULL(8)
  ICE_DBG_FLOW               BIT_ULL(9)
  ICE_DBG_DCB                BIT_ULL(10)
  ICE_DBG_DIAG               BIT_ULL(11)
  ICE_DBG_FD                 BIT_ULL(12)
  ICE_DBG_SW                 BIT_ULL(13)
  ICE_DBG_SCHED              BIT_ULL(14)

  ICE_DBG_PKG                BIT_ULL(16)
  ICE_DBG_RES                BIT_ULL(17)

  ICE_DBG_AQ_MSG             BIT_ULL(24)
  ICE_DBG_AQ_DESC            BIT_ULL(25)
  ICE_DBG_AQ_DESC_BUF        BIT_ULL(26)
  ICE_DBG_AQ_CMD             BIT_ULL(27)
  ICE_DBG_AQ                (ICE_DBG_AQ_MSG          |
                                 ICE_DBG_AQ_DESC     |
                                 ICE_DBG_AQ_DESC_BUF |
                                 ICE_DBG_AQ_CMD)

  ICE_DBG_USER                BIT_ULL(31)
  ICE_DBG_ALL                 0xFFFFFFFFFFFFFFFFULL */

#define SC_DBG_LVL 0

#define ice_debug(hw, mask, ...)         \
  do {                                   \
    (hw);                                \
    if (((mask) & (SC_DBG_LVL)) != 0) {  \
      AsciiPrint (__VA_ARGS__);          \
    }                                    \
  } while (0)
#else /* NO SC_DEBUG */

#define ice_debug(hw, mask, ...)
#endif /* SC_DEBUG */

#define ICE_INTEL_VENDOR_ID     0x8086

#define NalMemoryCopySafe(Dest, DestSize, Src, SrcSize) CopyMem (Dest, Src, SrcSize)

#define CHAR        CHAR8
#define memcmp      CompareMem
#define memcpy      CopyMem
#define strlen      AsciiStrLen
#define strcmp      AsciiStrCmp
#define strncmp     AsciiStrnCmp

#define int32_t  INT32
#define uint32_t UINT32
#define int16_t  INT16
#define uint16_t UINT16
#define int8_t   INT8
#define uint8_t  UINT8

#define size_t UINTN

typedef UINT64  __le64;
typedef UINT64  u64;
typedef INT64   s64;
typedef UINT32  __le32;
typedef UINT32  u32;
typedef INT32   s32;
typedef UINT16  __le16;
typedef UINT16  u16;
typedef INT16   s16;
typedef UINT8   u8;
typedef INT8    s8;

typedef UINT16 __be16;
typedef UINT32 __be32;
typedef UINT64 __be64;

#define LIST_REPLACE_INIT
#define LIST_ENTRY_TYPE _LIST_ENTRY
#define LIST_HEAD_TYPE  _LIST_ENTRY
#define HLIST_NODE_TYPE _LIST_ENTRY
#define HLIST_HEAD_TYPE _LIST_ENTRY

#define INIT_LIST_HEAD   InitializeListHead
#define LIST_EMPTY       IsListEmpty
#define LIST_ADD(a,b)    InsertHeadList(b,a)
#define LIST_DEL         RemoveEntryList

#define INIT_HLIST_HEAD     InitializeListHead
#define HLIST_EMPTY         IsListEmpty
#define HLIST_ADD_HEAD(a,b) InsertHeadList(b,a)
#define HLIST_DEL           RemoveEntryList

/* Retrieves base address of a structure EntryType which contains LIST_HEAD_TYPE
 * element List given forward link (FwLink) address in that element.
 *
 * @param[in]   FwLink        Address in LIST_HEAD_TYPE element
 * @param[in]   EntryType     Structure type of element to retrieve address
 * @param[in]   List          Name of LIST_HEAD_TYPE member in struct EntryType
 *
 * @return      Base address of a structure EntryType instance
 */
#define CONTAINING_RECORD(FwLink, EntryType, List)                 \
    (struct EntryType *)                                           \
    ((UINT64) FwLink                                               \
     - (UINT64) OFFSET_OF (struct LIST_HEAD_TYPE, ForwardLink) \
     - (UINT64) OFFSET_OF (struct EntryType, List)             \
    )

/* Return pointer to the entry after Pos in the List.
 *
 * @param[in]   Pos            Position in the List
 * @param[in]   EntryType      Entry type
 * @param[in]   List           Name of LIST_HEAD_TYPE member in struct EntryType
 *
 * @return      Pointer to the entry after position Pos
 */
#define LIST_NEXT_ENTRY(Pos, EntryType, List) CONTAINING_RECORD((Pos)->List.ForwardLink, EntryType, List)

/** Return pointer to the first entry in the list.

   @param[in]   Head            List pointer
   @param[in]   ContainerType   Entry type
   @param[in]   Member          Not used

   @return  Pointer to the first entry
**/
#define LIST_FIRST_ENTRY(Head, ContainerType, Member) \
    (ContainerType *) ((Head)->ForwardLink)

/* Iterates through circular forward list.
 *
 * @param[in]   Pos         Loop iterator
 * @param[in]   Head        Head element of the list
 * @param[in]   EntryType   Type of elements to iterate over
 * @param[in]   List        Name of LIST_HEAD_TYPE member in struct EntryType
 */
#define LIST_FOR_EACH_ENTRY(Pos, Head, EntryType, List)                    \
    for (Pos = CONTAINING_RECORD ((Head)->ForwardLink, EntryType, List);   \
         &Pos->List != (Head);                                             \
         Pos = CONTAINING_RECORD (Pos->List.ForwardLink, EntryType, List))

#define HLIST_FOR_EACH_ENTRY LIST_FOR_EACH_ENTRY

/* Iterates through circular forward list when deletion of currently iterated
 * element may be required. Requires additional temporary pointer.
 *
 * @param[in]   Pos         Loop iterator
 * @param[in]   Temp        Temporary iterator pointer
 * @param[in]   Head        Head element of the list
 * @param[in]   EntryType   Type of elements to iterate over
 * @param[in]   List        Name of LIST_HEAD_TYPE member in struct EntryType
 */
#define LIST_FOR_EACH_ENTRY_SAFE(Pos, Temp, Head, EntryType, List)                      \
    for (Pos = CONTAINING_RECORD ((Head)->ForwardLink, EntryType, List),                \
         Temp = CONTAINING_RECORD (Pos->List.ForwardLink, EntryType, List);             \
         &Pos->List != (Head);                                                          \
         Pos = Temp, Temp = CONTAINING_RECORD (Temp->List.ForwardLink, EntryType, List))

struct ice_dma_mem {
    void              *va;
    unsigned long long pa;
    u32                size;
    UNDI_DMA_MAPPING   Mapping;
};

struct ice_lock {
  EFI_LOCK SpinLock;
};


typedef BOOLEAN bool;

#define false FALSE
#define true  TRUE

#define ETH_ALEN 6

#define INLINE

// temporarily redefine inline keyword until the shared code fix is in place
#define inline

/** Wrapper for AsciiSPrint()

   @param[in]   a   Buffer to print to
   @param[in]   b   Buffer size
   @param[in]   c   String to write

   @return   String printed to buffer
**/
#define sprintf(a, b, c) AsciiSPrint ((a), sizeof(a), (b), (c))

/** Wrapper for AsciiSPrint()

   @param[in]   a    Buffer to print to
   @param[in]   b    Buffer size
   @param[in]   c    Format string
   @param[in]   ...  Additional arguments

   @return   String printed to buffer
**/
#define snprintf AsciiSPrint
#define SNPRINTF AsciiSPrint

#ifndef ARRAY_SIZE
/** Returns number of array elements

   @param[in]   a   Array

   @return   Number of array elements
**/
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#endif /* ARRAY_SIZE */

#undef ASSERT

/** ASSERT macro left blank

   @param[in]   x    Assert condition

   @return   None
**/
#define ASSERT(x)

/** Macro wrapper for shared code DEBUGOUT statement,
   blank here

   @param[in]   s    String to display

   @retval   None
**/
//#define DEBUGOUT(s)

#define DEBUGOUT(fmt, ...)

#ifndef ice_info
/** Macro wrapper for SC ice_info debug function

   @param[in]  Hw       pointer to the HW structure

   @retval   None
**/
#define ice_info(Hw, ...) do { (Hw); } while (0)
#endif /* ice_info */

#ifndef ice_warn
/** Macro wrapper for SC ice_warn debug function

   @param[in]  Hw       pointer to the HW structure

   @retval   None
**/
#define ice_warn(Hw, ...) do { (Hw); } while (0)
#endif /* ice_warn */

/** Macros to swap bytes in Dword

   @param[in]   Val   value to swap

   @return  Value swapped
**/
#define NTOHL(Val) ( \
  (((u32) (Val)  & 0xFF000000) >> 24) | \
   ((u32) ((Val) & 0x00FF0000) >> 8)  | \
   ((u32) ((Val) & 0x0000FF00) << 8)  | \
   ((u32) ((Val) & 0x000000FF) << 24))

/** Macros to swap bytes in word

   @param[in]   Val   value to swap

   @return  Value swapped
**/
#define NTOHS(Val) ( \
  (u16) (((u16) ((Val) & 0xFF00) >> 8) | \
  ((u16) ((Val) & 0x00FF) << 8)))


#define HTONS(a) NTOHS(a)
#define HTONL(a) NTOHL(a)

/** Macro wrapper for shared code debug function

   @param[in]   Hw         Pointer to the HW structure
   @param[in]   Type       Debug type
   @param[in]   Rowsize    Size of the row
   @param[in]   Groupsize  Group size
   @param[in]   Buf        Buffer pointer
   @param[in]   Len        Lenght

   @retval   None
**/
#define ice_debug_array(Hw, Type, Rowsize, Groupsize, Buf, Len)

/** Macro wrapper for shared code, blank here

   @param[in]   F    String to display

   @return None
**/
#define DEBUGFUNC(F)                        DEBUGOUT (F)

/** Macro for word conversion from CPU native
   to Little Endian

   @param[in]   a    Value to reverse

   @return   Value is converted
**/
#define CPU_TO_LE16(a) ((u16)(a))

/** Macro for Dword conversion from CPU native
   to Little Endian

   @param[in]   a    Value to reverse

   @return   Value is converted
**/
#define CPU_TO_LE32(a) ((u32)(a))

/** Macro for Qword conversion from CPU native
   to Little Endian

   @param[in]   a    Value to reverse

   @return   Value is converted
**/
#define CPU_TO_LE64(a) ((u64)(a))

/** Macro for Word conversion from Little Endian
   to CPU native

   @param[in]   a    Value to reverse

   @return   Value is converted
**/
#define LE16_TO_CPU(a) ((u16)(a))

/** Macro for Dword conversion from Little Endian
   to CPU native

   @param[in]   a    Value to reverse

   @return   Value is converted
**/
#define LE32_TO_CPU(a) ((u32)(a))

/** Macro for Qword conversion from Little Endian
   to CPU native

   @param[in]   a    Value to reverse

   @return   Value is converted
**/
#define LE64_TO_CPU(a) ((u64)(a))

/** Retrieves higher byte from word

   @param[in]   x   word

   @return  byte returned
**/
#define HIGH_BYTE(x) ((UINT8)(((x) >> 8) & 0xFF))

/** Retrieves lower byte from word

   @param[in]   x   word

   @return  byte returned
**/
#define LOW_BYTE(x) ((UINT8)((x) & 0xFF))

/** Creates word from high and low bytes

    @param[in]   Hi   High byte
    @param[in]   Low     Low byte

    @return   Word created
**/
#define MAKE_WORD(Hi, Low)                  \
          ((UINT16) ((((UINT16)(Hi)) << 8) | (Low)))

struct ice_hw;

VOID * IceAllocateMem (struct ice_hw *Hw, UINT32 Size);
VOID IceFreeMem (VOID *MemPtr);

/** Wrapper for IceAllocateDmaMem()

   @param[in]   Hw    pointer to the HW structure
   @param[out]  Mem   ptr to mem struct to fill out
   @param[in]   size  size of memory requested

   @return   Memory allocated
**/
#define ice_alloc_dma_mem(h, m, s) IceAllocateDmaMemWrap (h, m, s)

/** Wrapper for IceFreeDmaMem()

   @param[in]   Hw    pointer to the HW structure
   @param[out]  Mem   ptr to mem struct to free

   @return   Memory freed
**/
#define ice_free_dma_mem(h, m) IceFreeDmaMemWrap (h, m)

/** Wrapper for IceAllocateMem()

   @param[in]   Hw    pointer to the HW structure
   @param[out]  Mem   ptr to mem struct to fill out
   @param[in]   size  size of memory requested

   @return   Memory allocated
**/
#define ice_malloc(h, s)    IceAllocateMem (h, s)
#define ice_calloc(h, n, s) IceAllocateMem (h, ((n) * (s)))

/** Wrapper for IceFreeMem()

   @param[in]   Hw    pointer to the HW structure
   @param[out]  Mem   ptr to mem struct to free

   @return   Memory freed
**/
#define ice_free(Hw, Mem) IceFreeMem (Mem)

/** Shared code uses ice_memset(), this macro wraps SetMem to fullfill this need

   @param[in]    a   Buffer to set its contents
   @param[in]    b   Length of the buffer
   @param[in]    c   Value to set buffer contents to
   @param[in]    d   Unused

   @return    Buffer contents set to Value
**/
#define ice_memset(a,b,c,d)  SetMem ((a),(c),(b))

/** Shared code uses ice_memcpy(), this macro wraps CopyMem to fullfill this need

   @param[in]    a   Destination
   @param[in]    b   Source
   @param[in]    c   Size
   @param[in]    d   Unused

   @return    Size bytes from Source copied to Destination
**/
#define ice_memcpy(a,b,c,d)  CopyMem ((a),(b),(c))

/** Shared code uses memset(), this macro wraps SetMem to fullfill this need

   @param[in]    Buffer         Buffer to set its contents
   @param[in]    BufferLength   Length of the buffer
   @param[in]    Value          Value to set buffer contents to

   @return    Buffer contents set to Value
**/
#define memset(Buffer, Value, BufferLength) SetMem (Buffer, BufferLength, Value)

typedef struct DRIVER_DATA_S DRIVER_DATA;

/** This function calls the MemIo callback to read a dword from the device's
   address space

   @param[in]   AdapterInfo   Pointer to the NIC data structure information
                             the UNDI driver is layering on
   @param[in]   Port         Address to read from

   @return      The data read from the port.
**/
UINT32
IceRead32 (
  IN DRIVER_DATA *AdapterInfo,
  IN UINT32       Port
  );

/** This function calls the MemIo callback to write a word from the device's
   address space

   @param[in]   AdapterInfo   Pointer to the NIC data structure information
                             the UNDI driver is layering on
   @param[in]   Port         Address to write to
   @param[in]   Data         Data to write to Port

   @return    Data written to address in device's space
**/
VOID
IceWrite32 (
  IN DRIVER_DATA *AdapterInfo,
  IN UINT32       Port,
  IN UINT32       Data
  );

/** This function calls the IceRead32 twice to read a qword from the device's
   address space

   @param[in]   AdapterInfo   Pointer to the NIC data structure information
                             the UNDI driver is layering on
   @param[in]   Port         Address to read from

   @return      The data read from the port.
**/
UINT64
IceRead64 (
  IN DRIVER_DATA *AdapterInfo,
  IN UINT32       Port
  );

/** This function calls the IceWrite32 twice to write a qword to the device's
   address space

   @param[in]   AdapterInfo   Pointer to the NIC data structure information
                             the UNDI driver is layering on
   @param[in]   Port         Address to write to
   @param[in]   Data         Data to write to Port

   @return    Data written to address in device's space
**/
VOID
IceWrite64 (
  IN DRIVER_DATA *AdapterInfo,
  IN UINT32       Port,
  IN UINT64       Data
  );

/** Delays execution of next instructions for MicroSeconds microseconds

   @param[in]   AdapterInfo   Pointer to the NIC data structure information
                             the UNDI driver is layering on
   @param[in]   MicroSeconds   Time to delay in Microseconds.

   @retval   NONE
**/
VOID
DelayInMicroseconds (
  IN DRIVER_DATA *AdapterInfo,
  IN UINT32       MicroSeconds
  );

/** Wrapper for IceWrite32

   @param[in]   a        Pointer to hardware instance.
   @param[in]   Reg      Which port to write to.
   @param[in]   Value    Data to write to register.

   @return      Value written to Reg
**/
#define wr32(a, Reg, Value) IceWrite32 ((DRIVER_DATA *) ((a)->back), Reg, Value)

/** Wrapper for IceRead32

   @param[in]   a        Pointer to hardware instance.
   @param[in]   Reg      Which port to read from.

   @return      Value from Reg returned
**/
#define rd32(a, Reg)        IceRead32 ((DRIVER_DATA *) ((a)->back), (UINT32) (Reg))

/** Wrapper for IceWrite64

   @param[in]   a        Pointer to hardware instance.
   @param[in]   Reg      Which port to write to.
   @param[in]   Value    Data to write to register.

   @return      Value written to Reg
**/
#define wr64(a, Reg, Value) IceWrite64 ((DRIVER_DATA *) ((a)->back), Reg, Value)

/** Wrapper for IceRead64

   @param[in]   a        Pointer to hardware instance.
   @param[in]   Reg      Which port to read from.

   @return      Value from Reg returned
**/
#define rd64(a, Reg)        IceRead64 ((DRIVER_DATA *) ((a)->back), (UINT64) (Reg))

#define min(a,b) MIN (a,b)

/** Returns higher value from the two passed

   @param[in]   a    Value to compare
   @param[in]   a    Value to compare

   @return    Higher value returned
**/
#define max(a,b) MAX (a,b)

/** Returns offset of member in structure

   @param[in]   st   Structure type
   @param[in]   m    Structure member

   @return    Offset of member from structure in bytes
**/
#define offsetof(st, m) \
          ((size_t) ((char *)&((st *)(0))->m - (char *)0 ))

/** Returns offset of member in structure

   @param[in]   t   Structure type
   @param[in]   f   Structure field

   @return    Sizeof field in structure
**/
#define FIELD_SIZEOF(t, f) (sizeof(((t*)0)->f))

#define MAKEMASK(m, s) ((m) << (s))


/** Wrapper for IceInitSpinLock().

   @param[in]   Sp   Spinlock instance
**/
#define ice_init_lock(Sp) IceInitSpinLock (Sp)

/** Wrapper for IceAcquireSpinLock().

   @param[in]   Sp   Spinlock instance
**/
#define ice_acquire_lock(Sp) IceAcquireSpinLock (Sp)

/** Wrapper for IceReleaseSpinLock().

   @param[in]   Sp   Spinlock instance
**/
#define ice_release_lock(Sp) IceReleaseSpinLock (Sp)

/** Wrapper for IceDestroySpinLock().

   @param[in]   Sp   Spinlock instance
**/
#define ice_destroy_lock(Sp) IceDestroySpinLock (Sp)

/** Delays execution of code for time given in milliseconds

   @param[in]   x   Time in milliseconds
   @param[in]   y   Flag to indicate sleep or busy-wait (not used)
**/
#define ice_msec_delay(x, y) DelayInMicroseconds ((DRIVER_DATA *) (hw->back), x * 1000)

/** Delays execution of code for time given in microseconds

   @param[in]   x   Time in microseconds
   @param[in]   y   Flag to indicate sleep or busy-wait (not used)
**/
#define ice_usec_delay(x, y) DelayInMicroseconds ((DRIVER_DATA *) (hw->back), x)

/** Shifts a number left by the given number of bytes

   @param[in]   a   Number to shift left
   @param[in]   b   Bytes count
**/
#define SHIFT_LEFT(a, b) ((a) << (8*(b)))

/** Shifts a number right by the given number of bytes

   @param[in]   a   Number to shift left
   @param[in]   b   Bytes count
**/
#define SHIFT_RIGHT(a, b) ((a) >> (8*(b)))

/** Gets a single byte of a number

   @param[in]   a   Number to get the byte from
   @param[in]   b   Byte index
**/
#define GET_BYTE(a, b) (UINT8) (SHIFT_RIGHT((a) & SHIFT_LEFT(0xFF, (b)), (b)))

/** Converts a 16b integer from native endianness to big endian

   @param[in]   a   Number to convert endianness
**/
#define CPU_TO_BE16(a) (UINT16) ((SHIFT_LEFT(GET_BYTE((a), 0), 1)) | \
                                 (SHIFT_LEFT(GET_BYTE((a), 1), 0)))

/** Converts a 32b integer from native endianness to big endian

   @param[in]   a   Number to convert endianness
**/
#define CPU_TO_BE32(a) (UINT32) ((SHIFT_LEFT(GET_BYTE((a), 0), 3)) | \
                                 (SHIFT_LEFT(GET_BYTE((a), 1), 2)) | \
                                 (SHIFT_LEFT(GET_BYTE((a), 2), 1)) | \
                                 (SHIFT_LEFT(GET_BYTE((a), 3), 0)))

/** Converts a 64b integer from native endianness to big endian

   @param[in]   a   Number to convert endianness
**/
#define CPU_TO_BE64(a) (UINT64) ((SHIFT_LEFT(GET_BYTE((a), 0), 7)) | \
                                 (SHIFT_LEFT(GET_BYTE((a), 1), 6)) | \
                                 (SHIFT_LEFT(GET_BYTE((a), 2), 5)) | \
                                 (SHIFT_LEFT(GET_BYTE((a), 3), 4)) | \
                                 (SHIFT_LEFT(GET_BYTE((a), 4), 3)) | \
                                 (SHIFT_LEFT(GET_BYTE((a), 5), 2)) | \
                                 (SHIFT_LEFT(GET_BYTE((a), 6), 1)) | \
                                 (SHIFT_LEFT(GET_BYTE((a), 7), 0)))

#define ice_flush(a)    ((a)->hw_addr + GLGEN_STAT)

/** Duplicates memory block

   @param[in]  Hw    pointer to the HW structure
   @param[in]  Mem   ptr to mem struct to fill out
   @param[in]  Size  size of memory duplicated

   @return   Memory duplicated
**/
VOID *
IceMemDup (
  IN struct ice_hw *Hw,
  IN const void    *MemPtr,
  IN size_t         Size
  );

/** Wrapper for IceMemDup()

   @param[in]  Hw       pointer to the HW structure
   @param[in]  Mem      ptr to mem struct to fill out
   @param[in]  Size     size of memory duplicated
   @param[in]  MemType  not used

   @return   Memory duplicated
**/
#define ice_memdup(Hw,Mem,Size,MemType) IceMemDup(Hw,Mem,Size)

/** Counts the number of '1's in the 8-bit number

   @param[in]   Number    Number of which we count the '1's.

   @return                Number of '1's in the number.
**/
UINT8
IceHweight8 (
  IN UINT8 Number
  );
#define ice_hweight8(a) IceHweight8(a)

#define ice_hweight32(a) (ice_hweight8(GET_BYTE(a, 0)) + \
                          ice_hweight8(GET_BYTE(a, 1)) + \
                          ice_hweight8(GET_BYTE(a, 2)) + \
                          ice_hweight8(GET_BYTE(a, 3)))

#endif /* ICE_OSDEP_H_ */
