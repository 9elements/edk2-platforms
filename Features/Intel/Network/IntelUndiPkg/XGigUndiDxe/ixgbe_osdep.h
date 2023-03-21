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
#ifndef IXGBE_OSDEP_H_
#define IXGBE_OSDEP_H_


#ifndef EFI_SPECIFICATION_VERSION
#define EFI_SPECIFICATION_VERSION 0x00020000
#endif /* EFI_SPECIFICATION_VERSION */

#ifndef TIANO_RELEASE_VERSION
#define TIANO_RELEASE_VERSION 0x00080005
#endif /* TIANO_RELEASE_VERSION */

#include <Uefi.h>
#include <Base.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PrintLib.h>
#include <Library/DebugLib.h>

#define CHAR            CHAR8
#define memcmp          CompareMem
#define memcpy          CopyMem
#define strlen          AsciiStrLen
#define NalMemoryCopySafe(Dest, DestSize, Src, SrcSize) CopyMem (Dest, Src, SrcSize)

typedef UINT64  u64;
typedef INT64   s64;
typedef UINT32  u32;
typedef INT32   s32;
typedef UINT16  u16;
typedef INT16   s16;
typedef UINT8   u8;
typedef INT8    s8;
typedef BOOLEAN bool;

#define false FALSE
#define true  TRUE

typedef struct DRIVER_DATA_S DRIVER_DATA;

/** Delay a specified number of microseconds

   @param[in]   AdapterInfo   Pointer to the NIC data structure information
                              which the UNDI driver is layering on..
   @param[in]   MicroSeconds   Time to delay in Microseconds.

   @return   Execution of code delayed
**/
extern
VOID
DelayInMicroseconds (
  IN DRIVER_DATA *AdapterInfo,
  IN UINT32       MicroSeconds
  );

/** This function calls the MemIo callback to read a dword from the device's
   address space

   @param[in]   AdapterInfo   Adapter structure
   @param[in]   Port          Address to read from

   @retval    The data read from the port.
**/
extern
UINT32
XgbeInDword (
  IN DRIVER_DATA *AdapterInfo,
  IN UINT32       Port
  );

/** This function calls the MemIo callback to write a word from the device's
   address space

   @param[in]   AdapterInfo   Adapter structure
   @param[in]   Port          Address to write to
   @param[in]   Data          Data to write to Port

   @return   Word written
**/
extern
VOID
XgbeOutDword (
  IN DRIVER_DATA *AdapterInfo,
  IN UINT32       Port,
  IN UINT32       Data
  );

/** This function calls the EFI PCI IO protocol to read a value from the device's PCI
   register space.

   @param[in]   AdapterInfo   Pointer to the shared code hw structure.
   @param[in]   Offset        Which register to read from.

   @return     The value read from the PCI register.
**/
UINT16
XgbeReadPci16 (
  IN DRIVER_DATA *AdapterInfo,
  IN UINT32       Offset
  );

/** This function calls the EFI PCI IO protocol to write a value to the device's PCI
   register space.

   @param[in]   AdapterInfo   Pointer to the adapter structure.
   @param[in]   Offset        Which register to read from.
   @param[in]   Data          Returns the value read from the PCI register.

   @return    Value present in Data was written
**/
VOID
XgbeWritePci16 (
  IN DRIVER_DATA *AdapterInfo,
  IN UINT32       Offset,
  IN UINT16       Data
  );

/** Flushes a PCI write transaction to system memory.

   @param[in]   AdapterInfo   Pointer to the adapter structure.

   @return   Write transaction flushed
**/
extern
VOID
XgbePciFlush (
  IN DRIVER_DATA *AdapterInfo
  );

/** Swaps the bytes from machine order to network order (Big Endian)

   @param[in]   Dword   32-bit input value

   @return    Big Endian swapped value
**/
UINT32
IxgbeHtonl (
  IN UINT32 Dword
  );

/** Reverse bytes of a word (endianness change)

   @param[in]   Word   Value to be modified

   @return   Word reversed
**/
UINT16
IxgbeReverseWord (
  IN UINT16 Word
  );

/** Reverse bytes of a double word (endianness change)

   @param[in]   DWord   Value to be modified

   @return   DWord reversed
**/
UINT32
IxgbeReverseDword (
  IN UINT32 Dword
  );

/** These are wrapper macros for shared code usec/msec_delay macros
   with DelayInMicroseconds() function

   @param[in]   x   Time to wait in microseconds

   @return   DelayInMicroseconds called
**/
#define usec_delay(x)                       DelayInMicroseconds ((DRIVER_DATA *) (hw->back), x)

/** These are wrapper macros for shared code usec/msec_delay macros
   with DelayInMicroseconds() function

   @param[in]   x   Time to wait in milliseconds

   @return   DelayInMicroseconds called
**/
#define msec_delay(x)                       DelayInMicroseconds ((DRIVER_DATA *) (hw->back), x * 1000)

/** Shared code uses memset(), this macro wraps SetMem to fullfill this need

   @param[in]    Buffer         Buffer to set its contents
   @param[in]    BufferLength   Length of the buffer
   @param[in]    Value          Value to set buffer contents to

   @return   Buffer contents set to Value
**/
#define memset(Buffer, Value, BufferLength) SetMem (Buffer, BufferLength, Value)

#define PCI_COMMAND_REGISTER                PCI_COMMAND_REGISTER_OFFSET
#define CMD_MEM_WRT_INVALIDATE              EFI_PCI_COMMAND_MEMORY_WRITE_AND_INVALIDATE

typedef BOOLEAN boolean_t;

#if 0
#undef ASSERT
#define ASSERT(x)
#define DEBUGOUT  Aprint
#define DEBUGOUT1 Aprint
#define DEBUGOUT2 Aprint
#define DEBUGOUT3 Aprint
#define DEBUGOUT6 Aprint
#define DEBUGOUT7 Aprint
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

/** Macro wrapper for shared code EWARN macro,
   blank here

   @param[in]   hw   Pointer to HW structure
   @param[in]   s    Value to include in string

   @retval   None
**/
#define EWARN(hw, s)

#define DEBUGFUNCXX AsciiPrint

/** Macro wrapper for shared code DEBUGFUNC macro,
   assigned to DEBUGOUT (results in being blank)

   @param[in]   F,    String to display

   @retval   None
**/
#define DEBUGFUNC(F)                        DEBUGOUT (F)

/** Wrapper macro for shared code IXGBE_WRITE_REG statement

   @param[in]   a        Pointer to hardware instance.
   @param[in]   Reg      Which port to write to.
   @param[in]   Value    Data to write to Port.

   @return   XgbeOutDword called
**/
#define IXGBE_WRITE_REG(a, Reg, Value)      XgbeOutDword ((DRIVER_DATA *) ((a)->back), Reg, Value)

/** Wrapper macro for shared code IXGBE_READ_REG statement

   @param[in]   a        Pointer to hardware instance.
   @param[in]   Reg      Which port to read from.

   @return   XgbeInDword called
**/
#define IXGBE_READ_REG(a, Reg)              XgbeInDword ((DRIVER_DATA *) ((a)->back), (UINT32) (Reg))

/** Wrapper macro for shared code IXGBE_WRITE_REG_ARRAY statement

   @param[in]   a        Pointer to hardware instance.
   @param[in]   Reg      Which port to write to.
   @param[in]   Offset   Offset from Reg
   @param[in]   Value    Data to write to Port.

   @return   XgbeOutDword called
**/
#define IXGBE_WRITE_REG_ARRAY(a, Reg, Offset, Value)  \
   XgbeOutDword ((DRIVER_DATA *) ((a)->back), Reg + ((Offset) << 2), Value)

/** Wrapper macro for shared code IXGBE_READ_REG_ARRAY statement

   @param[in]   a        Pointer to hardware instance.
   @param[in]   Reg      Which port to read from.
   @param[in]   Offset   Offset from Reg

   @return   XgbeInDword called
**/
#define IXGBE_READ_REG_ARRAY(a, Reg, Offset)  \
  XgbeInDword ((DRIVER_DATA *) ((a)->back), Reg + ((Offset) << 2))

/** Wrapper macro for shared code IXGBE_WRITE_FLUSH statement

   @param[in]   a        Pointer to hardware instance.

   @return   XgbePciFlush called
**/
#define IXGBE_WRITE_FLUSH(a)                XgbePciFlush ((DRIVER_DATA *) ((a)->back));

/** Wrapper macro for shared code IXGBE_READ_PCIE_WORD statement

   @param[in]   a        Pointer to hardware instance.
   @param[in]   Reg      Which port to read from.

   @return   XgbeReadPci16 called
**/
#define IXGBE_READ_PCIE_WORD(a, Reg)        XgbeReadPci16 (a->back, Reg)

/** Wrapper macro for shared code IXGBE_WRITE_PCIE_WORD statement

   @param[in]   a        Pointer to hardware instance.
   @param[in]   Reg      Which port to write to.
   @param[in]   Data     Data to write to Port.

   @return   XgbeWritePci16 called
**/
#define IXGBE_WRITE_PCIE_WORD(a, Reg, Data) ; \
  XgbeWritePci16 (a->back, Reg, Data)

//#define IXGBE_HTONL(x)                      IxgbeHtonl (x)

/** Macros to swap bytes in word

   @param[in]   Val   value to swap

   @return  Value swapped
**/
#define IXGBE_NTOHS(Val) ((u16) (((u16) ((Val) & 0xFF00) >> 8) | ((u16) ((Val) & 0x00FF) << 8)))

/** Macros to swap bytes in Dword

   @param[in]   Val   value to swap

   @return  Value swapped
**/
#define IXGBE_NTOHL(Val) ( \
  (((u32)(Val)  & 0xFF000000) >> 24) | \
   ((u32)((Val) & 0x00FF0000) >> 8)  | \
   ((u32)((Val) & 0x0000FF00) << 8)  | \
   ((u32)((Val) & 0x000000FF) << 24))

#define IXGBE_HTONL IXGBE_NTOHL

/** Macro for word conversion from CPU native
   to Little Endian

   @param[in]   a    Value to reverse

   @return   Value is converted
**/
#define IXGBE_CPU_TO_LE16(a) ((UINT16) (a))

/** Macro for Dword conversion from CPU native
   to Little Endian

   @param[in]   a    Value to reverse

   @return   Value is converted
**/
#define IXGBE_CPU_TO_LE32(a) ((UINT32) (a))

/** Macro for Dword conversion from Little Endian
   to CPU native

   @param[in]   a    Value to reverse

   @return   Value is converted
**/
#define IXGBE_LE32_TO_CPU(a) ((UINT32) (a))

/** Macro for word conversion, unused here

   @param[in]   a    Value to reverse

   @return   Value is converted
**/
#define IXGBE_LE32_TO_CPUS(a) do {} while (0)

/** ERROR_REPORT wrapper macro for shared code,
   blank here

   @param[in]   S    IXGBE error type
   @param[in]   A    String to display

   @retval  None
**/
#define ERROR_REPORT(S,A) UNREFERENCED_1PARAMETER(A)

/** ERROR_REPORT1 wrapper macro for shared code,
   blank here

   @param[in]   S    IXGBE error type
   @param[in]   A    String to display

   @retval  None
**/
#define ERROR_REPORT1(S,A) UNREFERENCED_1PARAMETER(A)

/** ERROR_REPORT2 wrapper macro for shared code,
   blank here

   @param[in]   S    IXGBE error type
   @param[in]   A    String to display
   @param[in]   B    Value to include in string

   @retval  None
**/
#define ERROR_REPORT2(S,A,B) UNREFERENCED_2PARAMETER(A,B)

/** ERROR_REPORT3 wrapper macro for shared code,
   blank here

   @param[in]   S    IXGBE error type
   @param[in]   A    String to display
   @param[in]   B    Value to include in string
   @param[in]   C    Value to include in string

   @retval  None
**/
#define ERROR_REPORT3(S,A,B,C) UNREFERENCED_3PARAMETER(A,B,C)

/** Macro for word conversion from CPU native
   to Big Endian

   @param[in]   a    Value to reverse

   @return   Value is converted
**/
#define IXGBE_CPU_TO_BE16(a) IxgbeReverseWord (a)

/** Macro for Dword conversion from CPU native
   to Big Endian

   @param[in]   a    Value to reverse

   @return   Value is converted
**/
#define IXGBE_CPU_TO_BE32(a) IxgbeReverseDword (a)

/** Macro for Dword conversion from Big Endian
   to CPU native

   @param[in]   a    Value to reverse

   @return   Value is converted
**/
#define IXGBE_BE32_TO_CPU(a) IxgbeReverseDword (a)

/** Macro for word conversion from Big Endian
   to CPU native

   @param[in]   a    Value to reverse

   @return   Value is converted
**/
#define IXGBE_BE16_TO_CPU(a) IxgbeReverseWord (a)

#endif /* IXGBE_OSDEP_H_ */
