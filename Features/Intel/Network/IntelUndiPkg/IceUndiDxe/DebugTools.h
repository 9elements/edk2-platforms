/**************************************************************************

Copyright (c) 2018 - 2020, Intel Corporation. All rights reserved.

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

/** @file DebugTools.h Debug macros and utilities. */
#ifndef DEBUG_TOOLS_H_
#define DEBUG_TOOLS_H_

#include <Uefi.h>

// Debug levels for driver DEBUG_PRINT statements
#define NONE        0
#define INIT        (1 << 0)
#define DECODE      (1 << 1)

#define E1000       (1 << 2)
#define XGBE        (1 << 2)
#define I40E        (1 << 2)
#define ICE         (1 << 2)

#define SHARED      (1 << 3)
#define DIAG        (1 << 4)
#define CFG         (1 << 5)
#define IO          (1 << 6)
#define IMAGE       (1 << 7)
#define RX          (1 << 8)
#define TX          (1 << 9)
#define CRITICAL    (1 << 10)
#define HII         (1 << 11)
#define RXFILTER    (1 << 12)
#define WAIT        (1 << 13)
#define FLASH       (1 << 14)
#define BINDSUPPORT (1 << 15)
#define HEALTH      (1 << 16)
#define ADAPTERINFO (1 << 17)
#define DMA         (1 << 18)
#define WOL         (1 << 19)
#define HW          (1 << 20)
#define CLP         (1 << 21)
#define VLAN        (1 << 22)
#define HOSTIF      (1 << 24)
#define VPDUPD      (1 << 25)
#define VPD_DBG     (1 << 26)
//error : inserted by coan: "#define BOFM        (1 << 27)" contradicts -U or --implicit at C:\Sandbox\1074855\IntelUndiPkg\IceUndiDxe\DebugTools.h(66)
#define FOD         (1 << 28)
#define USER        (1 << 29)
#define INTERRUPT   (1 << 30)

// Enable debug levels you want here.
#define DBG_LVL (NONE)
#define OPENSRC_DBG_LVL (INIT | DIAG | CRITICAL | HII | HEALTH | ADAPTERINFO | DMA | WOL)


#if defined (DBG_LVL)
    /** Macro that unwraps all arguments in (parentheses) as variadic arguments.

      @param[in]   ...   variadic arguments
    **/
    #define NO_PARENTH(...) __VA_ARGS__
#endif /* DBG_LVL */


#if !defined (DBG_LVL) || (DBG_LVL) == (NONE)
    // If all debug macros are disabled, make sure these are not present
    // in the final binary at all.

    #define DEBUGPRINT(Lvl, Msg)
    #define DEBUGWAIT(Lvl)
    #define DEBUGPRINTTIME(Lvl)
    #define DEBUGDUMP(Lvl, Msg)

#elif defined (DBG_LVL) /* !defined(DBG_LOG_ENABLED) */
    // Debug macros enabled, output goes to the standard UEFI debug macros.
    #include <Library/DebugLib.h>

    /** When specific debug level is currently set this macro
       prints debug message.

       @param[in]   Lvl   Debug level
       @param[in]   Msg   Debug message
    **/
    #define DEBUGDUMP(Lvl, Msg) \
              if ((DBG_LVL & (Lvl)) != 0) { \
                DEBUG ((EFI_D_INFO, NO_PARENTH Msg)); \
              }
#endif /* DBG_LVL/DBG_LOG_ENABLED */


#if defined (DBG_LVL) && !defined (DEBUGPRINT)
    /** Print "Function[Line]: debug message" if a given debug level is set.

       This is a generic DEBUGPRINT implementation for most output methods.

       @param[in]   Lvl   Debug level
       @param[in]   Msg   Debug message
    **/
    #define DEBUGPRINT(Lvl, Msg) \
              DEBUGDUMP (Lvl, ("%a[%d]: ", __FUNCTION__, __LINE__)); \
              DEBUGDUMP (Lvl, Msg)
#endif /* End generic DEBUGPRINT */

#if defined (DBG_LVL) && !defined (DEBUGPRINTTIME)
    #include <Library/UefiRuntimeLib.h>

    /** Print the current timestamp if a given debug level is set.

       This is a generic DEBUGPRINTTIME implementation for most output methods.

       @param[in]   Lvl   Debug level
       @param[in]   Msg   Debug message
    **/
    #define DEBUGPRINTTIME(Lvl) \
              if ((DBG_LVL & (Lvl)) != 0) { \
                gRT->GetTime (&gTime, NULL); \
                DEBUGPRINT (Lvl, \
                  ("Timestamp - %dH:%dM:%dS:%dNS\n", \
                   gTime.Hour, gTime.Minute, gTime.Second, gTime.Nanosecond) \
                ); \
              }
#endif /* End generic DEBUGPRINTTIME */

#if defined (DBG_LVL) && !defined (DEBUGWAIT)
    /** Wait for the user to press Enter if a given debug level is set.

       This is a generic DEBUGWAIT implementation for most output methods.

       @param[in]   Lvl   Debug level
    **/
    #define DEBUGWAIT(Lvl) \
              if ((DBG_LVL & (Lvl)) != 0) { \
                EFI_INPUT_KEY Key; \
                DEBUGPRINT (Lvl, ("Press Enter to continue...\n")); \
                do { \
                  gST->ConIn->ReadKeyStroke (gST->ConIn, &Key); \
                } while (Key.UnicodeChar != 0xD); \
              }
#else /* !DBG_LVL || DEBUGWAIT */
    #undef  DEBUGWAIT
    #define DEBUGWAIT(Lvl)
#endif /* End generic DEBUGWAIT */

#if defined (DBG_LVL) && !defined (DEBUGPRINTWAIT)
    /** Print a message and wait for the user to press any button.
    
       @param[in]   Lvl   Debug level.
       @param[in]   Msg   Message to pass to DEBUGPRINT.
    **/
    #define DEBUGPRINTWAIT(Lvl, Msg) \
              { DEBUGPRINT (Lvl, Msg); DEBUGWAIT (Lvl); }
#endif /* End generic DEBUGPRINTWAIT */

#if defined (DBG_LVL) && !defined (UNIMPLEMENTED)
    #define UNIMPLEMENTED(Lvl) \
              DEBUGPRINT(Lvl, ("Unimplemented.\n")); \
              DEBUGWAIT(Lvl)
#endif /* End generic UNIMPLEMENTED */

// STATIC_ASSERT macro borrowed from edk2 master branch.
// Can be removed from driver once that macro gets integrated
// into a edk2 release.
#ifdef MDE_CPU_EBC
  #define STATIC_ASSERT(Expression, Message)
#elif _MSC_EXTENSIONS
  #define STATIC_ASSERT static_assert
#else /* !MDE_CPU_EBC && !_MSC_EXTENSIONS */
  #define STATIC_ASSERT _Static_assert
#endif /* STATIC_ASSERT */

#endif /* DEBUG_TOOLS_H_ */
