/**************************************************************************

Copyright (c) 2012 - 2021, Intel Corporation. All rights reserved.

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
#ifndef I40E_OSDEP_H_
#define I40E_OSDEP_H_

#include <Uefi.h>
#include <Base.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/SynchronizationLib.h>
#include <Library/UefiLib.h>
#include <Library/PrintLib.h>
#include "Dma.h"


#define CHAR            CHAR8
#define memcmp          CompareMem
#define memcpy          CopyMem
#define strlen          AsciiStrLen
#define NalMemoryCopySafe(Dest, DestSize, Src, SrcSize) CopyMem (Dest, Src, SrcSize)

#define int32_t  INT32
#define uint32_t UINT32
#define int16_t  INT16
#define uint16_t UINT16

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

typedef BOOLEAN bool;

#define false FALSE
#define true  TRUE

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

/** Returns number of array elements

   @param[in]   a   Array

   @return   Number of array elements
**/
#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#endif /* ARRAY_SIZE */

#if 0
#undef ASSERT
#define ASSERT(x)
#define DEBUGOUT  AsciiPrint
#define DEBUGOUT1 AsciiPrint
#define DEBUGOUT2 AsciiPrint
#define DEBUGOUT3 AsciiPrint
#define DEBUGOUT4 AsciiPrint
#define DEBUGOUT6 AsciiPrint
#define DEBUGOUT7 AsciiPrint
#else /* 1 */

/** Macro wrapper for shared code DEBUGOUT statement,
   blank here

   @param[in]   s    String to display

   @retval   None
**/
#define DEBUGOUT(s)

/** Macro wrapper for shared code DEBUGOUT1 statement,
   blank here

   @param[in]   s    String to display
   @param[in]   a    Value to include in string

   @retval   None
**/
#define DEBUGOUT1(s, a)

/** Macro wrapper for shared code DEBUGOUT2 statement,
   blank here

   @param[in]   s    String to display
   @param[in]   a    Value to include in string
   @param[in]   b    Value to include in string

   @retval   None
**/
#define DEBUGOUT2(s, a, b)

/** Macro wrapper for shared code DEBUGOUT3 statement,
   blank here

   @param[in]   s    String to display
   @param[in]   a    Value to include in string
   @param[in]   b    Value to include in string
   @param[in]   c    Value to include in string

   @retval   None
**/
#define DEBUGOUT3(s, a, b, c)

/** Macro wrapper for shared code DEBUGOUT7 statement,
   blank here

   @param[in]   s    String to display
   @param[in]   a    Value to include in string
   @param[in]   b    Value to include in string
   @param[in]   c    Value to include in string
   @param[in]   d    Value to include in string
   @param[in]   e    Value to include in string
   @param[in]   f    Value to include in string

   @retval   None
**/
#define DEBUGOUT6(s, a, b, c, d, e, f)

/** Macro wrapper for shared code DEBUGOUT7 statement,
   blank here

   @param[in]   s    String to display
   @param[in]   a    Value to include in string
   @param[in]   b    Value to include in string
   @param[in]   c    Value to include in string
   @param[in]   d    Value to include in string
   @param[in]   e    Value to include in string
   @param[in]   f    Value to include in string
   @param[in]   g    Value to include in string

   @retval   None
**/
#define DEBUGOUT7(s, a, b, c, d, e, f, g)
#endif /* 0 */

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

/** Retrieves higher Dword from Qword

   @param[in]   x   Qword

   @return  Dword returned
**/
#define HIDWORD(x) ((UINT32)(((x) >> 32) & 0xFFFFFFFF))

/** Retrieves lower Dword from Qword

   @param[in]   x   Qword

   @return  Dword returned
**/
#define LODWORD(x) ((UINT32)((x) & 0xFFFFFFFF))

/** Retrieves higher word from Dword

   @param[in]   x   Dword

   @return  word returned
**/
#define HIWORD(x) ((UINT16)(((x) >> 16) & 0xFFFF))

/** Retrieves lower word from Dword

   @param[in]   x   Dword

   @return  word returned
**/
#define LOWORD(x) ((UINT16)((x) & 0xFFFF))

/** Retrieves higher byte from word

   @param[in]   x   word

   @return  byte returned
**/
#define HIBYTE(x) ((UINT8)(((x) >> 8) & 0xFF))

/** Retrieves lower byte from word

   @param[in]   x   word

   @return  byte returned
**/
#define LOBYTE(x) ((UINT8)((x) & 0xFF))

/** Retrieves lower byte from word

   @param[in]   x   word

   @return  byte returned
**/
#define LOW_BYTE(word)      LOBYTE (word)

/** Retrieves higher byte from word

   @param[in]   x   word

   @return  byte returned
**/
#define HIGH_BYTE(word)     HIBYTE (word)

/** Retrieves lower word from Dword

   @param[in]   x   Dword

   @return  word returned
**/
#define LOW_WORD(dword)     LOWORD (dword)

/** Retrieves higher word from Dword

   @param[in]   x   Dword

   @return  word returned
**/
#define HIGH_WORD(dword)    HIWORD (dword)

/** Creates word from high and low bytes

    @param[in]   Hi   High byte
    @param[in]   Low     Low byte

    @return   Word created
**/
#define MAKE_WORD(Hi, Low)                  \
          ((UINT16) ((((UINT16)(Hi)) << 8) | (Low)))

struct i40e_dma_mem {
  VOID              *va;
  UINT64            pa;
  UINT32            size;
  UNDI_DMA_MAPPING  Mapping;
};

/** Wrapper for I40eAllocateDmaMem()

   @param[in]   Hw    pointer to the HW structure
   @param[out]  Mem   ptr to mem struct to fill out
   @param[in]   size  size of memory requested

   @return   Memory allocated
**/
#define i40e_allocate_dma_mem(h, m, unused, s, a) I40eAllocateDmaMem (h, m, s, a)

/** Wrapper for I40eFreeDmaMem()

   @param[in]   Hw    pointer to the HW structure
   @param[out]  Mem   ptr to mem struct to free

   @return   Memory freed
**/
#define i40e_free_dma_mem(h, m) I40eFreeDmaMem (h, m)

struct i40e_virt_mem {
  VOID   *va;
  UINT32 size;
};

/** Wrapper for I40eAllocateMem()

   @param[in]   Hw    pointer to the HW structure
   @param[out]  Mem   ptr to mem struct to fill out
   @param[in]   size  size of memory requested

   @return   Memory allocated
**/
#define i40e_allocate_virt_mem(h, m, s) I40eAllocateMem (h, m, s)

/** Wrapper for I40eFreeMem()

   @param[in]   Hw    pointer to the HW structure
   @param[out]  Mem   ptr to mem struct to free

   @return   Memory freed
**/
#define i40e_free_virt_mem(h, m) I40eFreeMem (h, m)


struct i40e_spinlock {
  EFI_LOCK SpinLock;
};

/** Wrapper for I40eInitSpinLock()

   @param[in]   Sp   Spinlock instance

   @return   Spinlock Sp initialized
**/
#define i40e_init_spinlock(Sp) I40eInitSpinLock (Sp)

/** Wrapper for I40eAcquireSpinLock()

   @param[in]   Sp   Spinlock instance

   @return   Spinlock Sp acquired
**/
#define i40e_acquire_spinlock(Sp) I40eAcquireSpinLock (Sp)

/** Wrapper for I40eReleaseSpinLock()

   @param[in]   Sp   Spinlock instance

   @return   Spinlock Sp released
**/
#define i40e_release_spinlock(Sp) I40eReleaseSpinLock (Sp)

/** Wrapper for I40eDestroySpinLock()

   @param[in]   Sp   Spinlock instance

   @return   Spinlock Sp destroyed
**/
#define i40e_destroy_spinlock(Sp) I40eDestroySpinLock (Sp)

typedef struct DRIVER_DATA_S DRIVER_DATA;

/** This function calls the MemIo callback to read a dword from the device's
   address space

   @param[in]   AdapterInfo   Pointer to the NIC data structure information
                             the UNDI driver is layering on
   @param[in]   Port         Address to read from

   @return      The data read from the port.
**/
UINT32
I40eRead32(
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
I40eWrite32(
  IN DRIVER_DATA *AdapterInfo,
  IN UINT32       Port,
  IN UINT32       Data
  );

/** Delays execution of next instructions for MicroSeconds microseconds

   @param[in]   AdapterInfo   Pointer to the NIC data structure information
                             the UNDI driver is layering on
   @param[in]   MicroSeconds   Time to delay in Microseconds.

   @retval   NONE
**/
VOID
DelayInMicroseconds (
  IN DRIVER_DATA  *AdapterInfo,
  UINT32           MicroSeconds
  );

/** Wrapper for I40eWrite32

   @param[in]   a        Pointer to hardware instance.
   @param[in]   Reg      Which port to write to.
   @param[in]   Value    Data to write to register.

   @return      Value written to Reg
**/
#define wr32(a, Reg, Value) I40eWrite32 ((DRIVER_DATA *) ((a)->back), Reg, Value)

/** Wrapper for I40eRead32

   @param[in]   a        Pointer to hardware instance.
   @param[in]   Reg      Which port to read from.

   @return      Value from Reg returned
**/
#define rd32(a, Reg)        I40eRead32 ((DRIVER_DATA *) ((a)->back), (UINT32) (Reg))

/** Delays execution of code for time given in microseconds

   @param[in]   x   TIme in microseconds

   @return      Code execution delayed
**/
#define i40e_usec_delay(x) DelayInMicroseconds ((DRIVER_DATA *) (hw->back), x)
#define i40e_usec_stall(x) DelayInMicroseconds ((DRIVER_DATA *) (hw->back), x)

/** Delays execution of code for time given in milliseconds

   @param[in]   x   TIme in milliseconds

   @return      Code execution delayed
**/
#define i40e_msec_delay(x) DelayInMicroseconds ((DRIVER_DATA *) (hw->back), x * 1000)
#define i40e_msec_stall(x) DelayInMicroseconds ((DRIVER_DATA *) (hw->back), x * 1000)


/** Shared code uses i40e_memset(), this macro wraps SetMem to fullfill this need

   @param[in]    a   Buffer to set its contents
   @param[in]    b   Length of the buffer
   @param[in]    c   Value to set buffer contents to
   @param[in]    d   Unused

   @return    Buffer contents set to Value
**/
#define i40e_memset(a,b,c,d)  SetMem ((a),(c),(b))

/** Shared code uses i40e_memcpy(), this macro wraps CopyMem to fullfill this need

   @param[in]    a   Destination
   @param[in]    b   Source
   @param[in]    c   Size
   @param[in]    d   Unused

   @return    Size bytes from Source copied to Destination
**/
#define i40e_memcpy(a,b,c,d)  CopyMem ((a),(b),(c))

/** Shared code uses memset(), this macro wraps SetMem to fullfill this need

   @param[in]    Buffer         Buffer to set its contents
   @param[in]    BufferLength   Length of the buffer
   @param[in]    Value          Value to set buffer contents to

   @return    Buffer contents set to Value
**/
#define memset(Buffer, Value, BufferLength) SetMem (Buffer, BufferLength, Value)

/** Returns lower value from the two passed

   @param[in]   a    Value to compare
   @param[in]   a    Value to compare

   @return    Lower value returned
**/
#define min(a,b) MIN (a,b)

/** Returns higher value from the two passed

   @param[in]   a    Value to compare
   @param[in]   a    Value to compare

   @return    Higher value returned
**/
#define max(a,b) MAX (a,b)

#define i40e_debug


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


#endif /* I40E_OSDEP_H_ */
