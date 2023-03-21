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
#ifndef E1000_OSDEP_H_
#define E1000_OSDEP_H_

#include <Uefi.h>
#include <Base.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>


#ifndef EFI_SPECIFICATION_VERSION
#define EFI_SPECIFICATION_VERSION 0x00020000
#endif /* EFI_SPECIFICATION_VERSION */

#ifndef TIANO_RELEASE_VERSION
#define TIANO_RELEASE_VERSION     0x00080005
#endif /* TIANO_RELEASE_VERSION */


//#define STATIC static

#define CHAR     CHAR8
#define memcmp   CompareMem
#define memcpy   CopyMem
#define strlen   AsciiStrLen
#define NalMemoryCopySafe(Dest, DestSize, Src, SrcSize) CopyMem (Dest, Src, SrcSize)

#define int32_t  INT32;
#define uint32_t UINT32;
#define int16_t  INT16;
#define uint16_t UINT16;

typedef UINT64   __le64;
typedef UINT64   u64;
typedef INT64    s64;
typedef UINT32   __le32;
typedef UINT32   u32;
typedef INT32    s32;
typedef UINT16   __le16;
typedef UINT16   u16;
typedef INT16    s16;
typedef UINT8    u8;
typedef INT8     s8;
typedef BOOLEAN  bool;

#define true 1
#define false 0

struct e1000_hw;

/** This function calls the MemIo callback to read a dword from the device's
   address space

   Since UNDI3.0 uses the TmpMemIo function (instead of the callback routine)
   which also takes the UniqueId parameter (as in UNDI3.1 spec) we don't have
   to make undi3.0 a special case

   @param[in]   Hw     Pointer to hardware instance.
   @param[in]   Port   Which Port to read from.

   @return   Results - The data read from the port.
**/
UINT32
E1000InDword (
  struct e1000_hw *Hw,
  UINT32           Port
  );

/** This function calls the MemIo callback to write a word from the device's
   address space

   Since UNDI3.0 uses the TmpMemIo function (instead of the callback routine)
   which also takes the UniqueId parameter (as in UNDI3.1 spec) we don't have
   to make undi3.0 a special case

   @param[in]   Hw   Pointer to hardware instance.
   @param[in]   Port   Which port to write to.
   @param[in]   Data   Data to write to Port.

   @return   Word written
**/
VOID
E1000OutDword (
  IN struct e1000_hw *Hw,
  IN UINT32           Port,
  IN UINT32           Data
  );

/** This function calls the MemIo callback to read a dword from the device's
   address space

   Since UNDI3.0 uses the TmpMemIo function (instead of the callback routine)
   which also takes the UniqueId parameter (as in UNDI3.1 spec) we don't have
   to make undi3.0 a special case

   @param[in]   Hw    Pointer to hardware instance.
   @param[in]   Port  Which port to read from.

   @return   Results - The data read from the port.
**/
UINT32
E1000FlashRead (
  IN struct e1000_hw *Hw,
  IN UINT32           Port
  );

/** This function calls the MemIo callback to read a dword from the device's
   address space

   Since UNDI3.0 uses the TmpMemIo function (instead of the callback routine)
   which also takes the UniqueId parameter (as in UNDI3.1 spec) we don't have
   to make undi3.0 a special case

   @param[in]   Hw   Pointer to hardware instance.
   @param[in]   Port   Which port to read from.

   @return   Results - The data read from the port.
**/
UINT16
E1000FlashRead16 (
  IN struct e1000_hw *Hw,
  IN UINT32           Port
  );

/** This function calls the MemIo callback to write a word from the device's
   address space

   Since UNDI3.0 uses the TmpMemIo function (instead of the callback routine)
   which also takes the UniqueId parameter (as in UNDI3.1 spec) we don't have
   to make undi3.0 a special case

   @param[in]   Hw     Pointer to hardware instance.
   @param[in]   Port   Which port to write to.
   @param[in]   Data   Data to write to Port.

   @return   Word written
**/
VOID
E1000FlashWrite (
  IN struct e1000_hw *Hw,
  IN UINT32           Port,
  IN UINT32           Data
  );

/** This function calls the MemIo callback to write a word from the device's
   address space

   Since UNDI3.0 uses the TmpMemIo function (instead of the callback routine)
   which also takes the UniqueId parameter (as in UNDI3.1 spec) we don't have
   to make undi3.0 a special case

   @param[in]   Hw     Pointer to hardware instance.
   @param[in]   Port   Which port to write to.
   @param[in]   Data   Data to write to Port.

   @return   Word written
**/
VOID
E1000FlashWrite16 (
  IN struct e1000_hw *Hw,
  IN UINT32           Port,
  IN UINT16           Data
  );

/** Flushes a PCI write transaction to system memory.

   @param[in]   Hw   Pointer to hardware structure.

   @return  PCI write transaction flushed
**/
VOID
E1000PciFlush (
  struct e1000_hw *Hw
  );

/** Delay a specified number of microseconds.

   @param[in]   Hw   Pointer to hardware instance.
   @param[in]   usecs   Number of microseconds to delay

   @return   Execution of code delayed
**/
VOID
USecDelay (
  struct e1000_hw *Hw,
  UINTN            Usecs
  );

/** Writes a value to one of the devices registers using port I/O (as opposed to
   memory mapped I/O). Only 82544 and newer devices support port I/O.

   @param[in]   Hw      Pointer to the shared code hw structure.
   @param[in]   Offset  The register offset to write.
   @param[in]   Value   The value to write to the register.

   @return   Value written to register offset
**/
VOID
E1000WriteRegIo (
  struct e1000_hw *Hw,
  UINT32           Offset,
  UINT32           Value
  );

/** Wrapper macro for shared code usec_delay statement
   with USecDelay function

   @param[in]   x   Time to wait in microseconds

   @return   USecDelay called
**/
#define usec_delay(x)     USecDelay (hw, x)

/** Wrapper macro for shared code msec_delay statement
   with USecDelay function

   @param[in]   x   Time to wait in milliseconds

   @return   USecDelay called
**/
#define msec_delay(x)     USecDelay (hw, x * 1000)

/** Wrapper macro for shared code usec_delay_irq statement
   with USecDelay function

   @param[in]   x   Time to wait in microseconds

   @return   USecDelay called
**/
#define usec_delay_irq(x) USecDelay (hw, x)

/** Wrapper macro for shared code msec_delay_irq statement
   with USecDelay function

   @param[in]   x   Time to wait in milliseconds

   @return   USecDelay called
**/
#define msec_delay_irq(x) USecDelay (hw, x * 1000)

/** Shared code uses memset(), this macro wraps SetMem to fullfill this need

   @param[in]    Buffer         Buffer to set its contents
   @param[in]    BufferLength   Length of the buffer
   @param[in]    Value          Value to set buffer contents to

   @return   Buffer contents set to Value
**/
#define memset(Buffer, Value, BufferLength) SetMem (Buffer, BufferLength, Value)

#define CMD_MEM_WRT_INVALIDATE EFI_PCI_COMMAND_MEMORY_WRITE_AND_INVALIDATE

typedef BOOLEAN boolean_t;


#if (0)
#define DEBUGFUNC(F)
#define DEBUGOUT(s) Aprint (s);
#define DEBUGOUT1(s, a) Aprint (s, a);
#define DEBUGOUT2(s, a, b) Aprint (s, a, b);
#define DEBUGOUT3(s, a, b, c) Aprint (s, a, b, c);
#define DEBUGOUT7(s, a, b, c, d, e, f, g) Aprint (s, a, b, c, d, e, f, g);
#else /* NOT (0) */

/** Macro wrapper for shared code, blank here

   @param[in]   F    String to display

   @retval  None
**/
#define DEBUGFUNC(F)

/** Macro wrapper for shared code DEBUGOUT statement
   with UNREFERENCED_XPARAMETER (resulting in DEBUGOUT being unused)

   @param[in]   s    String to display

   @retval   None
**/
#define DEBUGOUT(s) \
  do { \
  } while (0)

/** Macro wrapper for shared code DEBUGOUT1 statement
   with UNREFERENCED_1PARAMETER (resulting in DEBUGOUT being unused)

   @param[in]   s    String to display
   @param[in]   a    Value to include in string

   @retval   None
**/
#define DEBUGOUT1(s, a) \
  do { \
    UNREFERENCED_1PARAMETER (a); \
  } while (0)

/** Macro wrapper for shared code DEBUGOUT2 statement
   with UNREFERENCED_2PARAMETER (resulting in DEBUGOUT being unused)

   @param[in]   s    String to display
   @param[in]   a    Value to include in string
   @param[in]   b    Value to include in string

   @retval   None
**/
#define DEBUGOUT2(s, a, b) \
  do { \
    UNREFERENCED_2PARAMETER (a, b); \
  } while (0)

/** Macro wrapper for shared code DEBUGOUT3 statement
   with UNREFERENCED_3PARAMETER (resulting in DEBUGOUT being unused)

   @param[in]   s    String to display
   @param[in]   a    Value to include in string
   @param[in]   b    Value to include in string
   @param[in]   c    Value to include in string

   @retval   None
**/
#define DEBUGOUT3(s, a, b, c) \
  do { \
    UNREFERENCED_3PARAMETER (a, b, c); \
  } while (0)

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
#endif /* (0) */

/** E1000_WRITE_REG wrapper macro for shared code

   @param[in]   a        Pointer to hardware instance.
   @param[in]   Reg      Which port to write to.
   @param[in]   Value    Data to write to Port.

   @return   E1000OutDword called
**/
#define E1000_WRITE_REG(a, Reg, Value) \
  E1000OutDword (a, (UINT32) (Reg), Value)

/** E1000_READ_REG wrapper macro for shared code

   @param[in]   a        Pointer to hardware instance.
   @param[in]   Reg      Which port to read from.

   @return   E1000InDword called
**/
#define E1000_READ_REG(a, Reg)  \
  E1000InDword (a, (UINT32) (Reg)) \

/** E1000_WRITE_REG_ARRAY wrapper macro for shared code

   @param[in]   a        Pointer to hardware instance.
   @param[in]   Reg      Which port to write to.
   @param[in]   Offset   Offset from Reg.
   @param[in]   Value    Data to write to Port.

   @return   E1000OutDword called
**/
#define E1000_WRITE_REG_ARRAY(a, Reg, Offset, Value) \
  E1000OutDword (a, (UINT32) (Reg + ((Offset) << 2)), Value)

/** E1000_WRITE_REG_ARRAY_BYTE wrapper macro for shared code

   @param[in]   a        Pointer to hardware instance.
   @param[in]   Reg      Which port to write to.
   @param[in]   Offset   Offset from Reg.
   @param[in]   Value    Data to write to Port.

   @return   E1000OutDword called
**/
#define E1000_WRITE_REG_ARRAY_BYTE(a, Reg, Offset, Value)  \
  E1000OutDword (a, (UINT32) (Reg + ((Offset) << 2)), Value) \

/** E1000_WRITE_REG_ARRAY_DWORD wrapper macro for shared code

   @param[in]   a        Pointer to hardware instance.
   @param[in]   Reg      Which port to write to.
   @param[in]   Offset   Offset from Reg.
   @param[in]   Value    Data to write to Port.

   @return   E1000OutDword called
**/
#define E1000_WRITE_REG_ARRAY_DWORD(a, Reg, Offset, Value)  \
  E1000OutDword (a, (UINT32) (Reg + ((Offset) << 2)), Value)  \

/** E1000_READ_REG_ARRAY wrapper macro for shared code

   @param[in]   a        Pointer to hardware instance.
   @param[in]   Reg      Which port to read from.
   @param[in]   Offset   Offset from Reg.

   @return   E1000InDword called
**/
#define E1000_READ_REG_ARRAY(a, Reg, Offset)  \
  E1000InDword (a, (UINT32) (Reg + ((Offset) << 2)))

/** E1000_READ_REG_ARRAY_BYTE wrapper macro for shared code

   @param[in]   a        Pointer to hardware instance.
   @param[in]   Reg      Which port to read from.
   @param[in]   Offset   Offset from Reg.

   @return   E1000InDword called
**/
#define E1000_READ_REG_ARRAY_BYTE(a, Reg, Offset)  \
  E1000InDword (a, (UINT32) (Reg + ((Offset) << 2)))

/** E1000_READ_REG_ARRAY_DWORD wrapper macro for shared code

   @param[in]   a        Pointer to hardware instance.
   @param[in]   Reg      Which port to read from.
   @param[in]   Offset   Offset from Reg.

   @return   E1000InDword called
**/
#define E1000_READ_REG_ARRAY_DWORD(a, Reg, Offset)  \
  E1000InDword (a, (UINT32) (Reg + ((Offset) << 2)))

/** E1000_WRITE_FLUSH wrapper macro for shared code

   @param[in]   a        Pointer to hardware instance.

   @return   E1000PciFlush called
**/
#define E1000_WRITE_FLUSH(a) E1000PciFlush (a);

/** E1000_WRITE_REG_IO wrapper macro for shared code

   @param[in]   a       Pointer to the shared code hw structure.
   @param[in]   Reg     The register offset to write.
   @param[in]   Value   The value to write to the register.

   @return   E1000WriteRegIo called
**/
#define E1000_WRITE_REG_IO(a, Reg, Value) \
  E1000WriteRegIo (a, (UINT32) (Reg), Value)

/** E1000_READ_FLASH_REG wrapper macro for shared code

   @param[in]   a    Pointer to hardware instance.
   @param[in]   Reg  Which port to read from.

   @return   E1000FlashRead called
**/
#define E1000_READ_FLASH_REG(a, Reg) \
  E1000FlashRead (a, (UINT32) (Reg))

/** E1000_WRITE_FLASH_REG wrapper macro for shared code

   @param[in]   a       Pointer to the shared code hw structure.
   @param[in]   Reg     The register offset to write.
   @param[in]   Data    The value to write to the register.

   @return   E1000FlashWrite called
**/
#define E1000_WRITE_FLASH_REG(a, Reg, Data) \
  E1000FlashWrite (a, (UINT32) (Reg), Data)

/** E1000_READ_FLASH_REG16 wrapper macro for shared code

   @param[in]   a       Pointer to the shared code hw structure.
   @param[in]   Reg     The register offset to read from.

   @return   E1000FlashRead16 called
**/
#define E1000_READ_FLASH_REG16(a, Reg) \
  E1000FlashRead16 (a, (UINT32) (Reg))

/** E1000_WRITE_FLASH_REG16 wrapper macro for shared code

   @param[in]   a       Pointer to the shared code hw structure.
   @param[in]   Reg     The register offset to write.
   @param[in]   Data    The value to write to the register.

   @return   E1000FlashWrite16 called
**/
#define E1000_WRITE_FLASH_REG16(a, Reg, Data) \
  E1000FlashWrite16 (a, (UINT32) (Reg), Data)


#define E1000_MUTEX                       u8

/** Macro wrapper for shared code E1000_MUTEX_INIT statement
   with UNREFERENCED_1PARAMETER (resulting in mutex being unused)

  @param[in]   Mutex   The Mutex to perform operations on

   @return None
**/
#define E1000_MUTEX_INIT(Mutex)           UNREFERENCED_1PARAMETER (Mutex)

/** Macro wrapper for shared code E1000_MUTEX_DESTROY statement
   with UNREFERENCED_1PARAMETER (resulting in mutex being unused)

  @param[in]   Mutex   The Mutex to perform operations on

   @return None
**/
#define E1000_MUTEX_DESTROY(Mutex)        UNREFERENCED_1PARAMETER (Mutex)

/** Macro wrapper for shared code E1000_MUTEX_LOCK statement
   with UNREFERENCED_1PARAMETER (resulting in mutex being unused)

   @param[in]   Mutex   The Mutex to perform operations on

   @return None
**/
#define E1000_MUTEX_LOCK(Mutex)           UNREFERENCED_1PARAMETER (Mutex)

/** Macro wrapper for shared code E1000_MUTEX_TRYLOCK statement
   with UNREFERENCED_1PARAMETER (resulting in mutex being unused)

   @param[in]   Mutex   The Mutex to perform operations on

   @return None
**/
#define E1000_MUTEX_TRYLOCK(Mutex)        UNREFERENCED_1PARAMETER (Mutex)

/** Macro wrapper for shared code E1000_MUTEX_UNLOCK statement
   with UNREFERENCED_1PARAMETER (resulting in mutex being unused)

   @param[in]   Mutex   The Mutex to perform operations on

   @return    None
**/
#define E1000_MUTEX_UNLOCK(Mutex)         UNREFERENCED_1PARAMETER (Mutex)

#endif /* E1000_OSDEP_H_ */

