/** @file
    LibC-style memcpy/memset wrappers over EFI memory functions.

    Copyright (c) 2010-2018, Intel Corporation. All rights reserved.<BR>
    This program and the accompanying materials are licensed and made available under
    the terms and conditions of the BSD License that accompanies this distribution.
    The full text of the license may be found at
    http://opensource.org/licenses/bsd-license.php.

    THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
    WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/
#ifdef _SIZE_T_NOTDEFINED
typedef unsigned int size_t;
#endif

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>

/* Do not define memcpy for IPF+GCC builds. For IPF, GCC compiler
 * converts memcpy to CopyMem by objcpy during build. */
#if  !(defined(MDE_CPU_IPF) && defined(__GNUC__))

#ifdef _SIZE_T_NOTDEFINED
typedef unsigned int size_t;
#endif

/** The memcpy function copies n characters from the object pointed to by s2
    into the object pointed to by s1.

    The implementation is reentrant and handles the case where s2 overlaps s1.

    @return   The memcpy function returns the value of s1.
**/
void *
memcpy(void * __restrict s1, const void * __restrict s2, size_t n)
{
  return CopyMem( s1, s2, n);
}
#endif  /* !(defined(MDE_CPU_IPF) && defined(__GCC)) */


/** The memset function copies the value of c (converted to an unsigned char)
    into each of the first n characters of the object pointed to by s.

    @return   The memset function returns the value of s.
**/
void *
memset(void *s, int c, size_t n)
{
    return SetMem(s, (UINTN)n, (UINT8)c);
}
