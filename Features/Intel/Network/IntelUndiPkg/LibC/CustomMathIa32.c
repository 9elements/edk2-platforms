/** @file
  64-bit Math Worker Function.
  The 32-bit versions of C compiler generate calls to library routines
  to handle 64-bit math. These functions use non-standard calling conventions.

  Copyright (c) 2009 - 2011, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials are licensed and made available
  under the terms and conditions of the BSD License which accompanies this
  distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

UINT64
EFIAPI
CustomMathMultU64x64 (
    IN UINT64 Multiplicand,
    IN UINT64 Multiplier
    )
{
  _asm {
    mov     ebx, dword ptr [Multiplicand + 0]
    mov     edx, dword ptr [Multiplier + 0]
    mov     ecx, ebx
    mov     eax, edx
    imul    ebx, dword ptr [Multiplier + 4]
    imul    edx, dword ptr [Multiplicand + 4]
    add     ebx, edx
    mul     ecx
    add     edx, ebx
  }
}

UINT64
EFIAPI
CustomMultU64x64 (
  IN      UINT64                    Multiplicand,
  IN      UINT64                    Multiplier
  )
{
  UINT64                            Result;

  Result = CustomMathMultU64x64 (Multiplicand, Multiplier);

  return Result;
}

INT64
EFIAPI
CustomMultS64x64 (
  IN      INT64                     Multiplicand,
  IN      INT64                     Multiplier
  )
{
  return (INT64)CustomMultU64x64((UINT64)Multiplicand, (UINT64)Multiplier);
}

/*
 * Multiplies a 64-bit signed or unsigned value by a 64-bit signed or unsigned value
 * and returns a 64-bit result.
 */
__declspec(naked) void __cdecl _allmul (void)
{
  //
  // Wrapper Implementation over our CustomMultS64x64() routine
  //    INT64
  //    EFIAPI
  //    CustomMultS64x64 (
  //      IN      INT64      Multiplicand,
  //      IN      INT64      Multiplier
  //      )
  //
  _asm {
    ; Original local stack when calling _allmul
    ;               -----------------
    ;               |               |
    ;               |---------------|
    ;               |               |
    ;               |--Multiplier --|
    ;               |               |
    ;               |---------------|
    ;               |               |
    ;               |--Multiplicand-|
    ;               |               |
    ;               |---------------|
    ;               |  ReturnAddr** |
    ;       ESP---->|---------------|
    ;

    ;
    ; Set up the local stack for Multiplicand parameter
    ;
    mov  eax, [esp + 16]
    push eax
    mov  eax, [esp + 16]
    push eax

    ;
    ; Set up the local stack for Multiplier parameter
    ;
    mov  eax, [esp + 16]
    push eax
    mov  eax, [esp + 16]
    push eax

    ;
    ; Call native MulS64x64 of BaseLib
    ;
    call CustomMultS64x64

    ;
    ; Adjust stack
    ;
    add  esp, 16

    ret  16
  }
}

/*
 * Shifts a 64-bit signed value left by a particular number of bits.
 */
__declspec(naked) void __cdecl _allshl(void)
{
  _asm {
    ;
    ; Handle shifting of 64 or more bits (return 0)
    ;
    cmp     cl, 64
    jae     short ReturnZero

    ;
    ; Handle shifting of between 0 and 31 bits
    ;
    cmp     cl, 32
    jae     short More32
    shld    edx, eax, cl
    shl     eax, cl
    ret

    ;
    ; Handle shifting of between 32 and 63 bits
    ;
More32:
    mov     edx, eax
    xor     eax, eax
    and     cl, 31
    shl     edx, cl
    ret

ReturnZero:
    xor     eax,eax
    xor     edx,edx
    ret
  }
}

/*
 * Shifts a 64-bit unsigned value right by a certain number of bits.
 */
__declspec(naked) void __cdecl _aullshr(void)
{
  _asm {
    ;
    ; Checking: Only handle 64bit shifting or more
    ;
    cmp     cl, 64
    jae     _Exit

    ;
    ; Handle shifting between 0 and 31 bits
    ;
    cmp     cl, 32
    jae     More32
    shrd    eax, edx, cl
    shr     edx, cl
    ret

    ;
    ; Handle shifting of 32-63 bits
    ;
More32:
    mov     eax, edx
    xor     edx, edx
    and     cl, 31
    shr     eax, cl
    ret

    ;
    ; Invalid number (less then 32bits), return 0
    ;
_Exit:
    xor     eax, eax
    xor     edx, edx
    ret
  }
}

UINT64
EFIAPI
CustomMathDivRemU64x64 (
  IN      UINT64                    Dividend,
  IN      UINT64                    Divisor,
  OUT     UINT64                    *Remainder OPTIONAL
  )
{
  _asm {
    mov     edx, dword ptr [Dividend + 4]
    mov     eax, dword ptr [Dividend + 0]   // edx:eax <- dividend
    mov     edi, edx
    mov     esi, eax                    // edi:esi <- dividend
    mov     ecx, dword ptr [Divisor + 4]
    mov     ebx, dword ptr [Divisor + 0]   // ecx:ebx <- divisor
BitLoop:
    shr     edx, 1
    rcr     eax, 1
    shrd    ebx, ecx, 1
    shr     ecx, 1
    jnz     BitLoop
    div     ebx
    mov     ebx, eax                    // ebx <- quotient
    mov     ecx, dword ptr [Divisor + 4]
    mul     dword ptr [Divisor]
    imul    ecx, ebx
    add     edx, ecx
    mov     ecx, Remainder
    jc      TooLarge                   // product > 2^64
    cmp     edi, edx                    // compare high 32 bits
    ja      Correct
    jb      TooLarge                   // product > dividend
    cmp     esi, eax
    jae     Correct                    // product <= dividend
TooLarge:
    dec     ebx                         // adjust quotient by -1
    jecxz   Return                     // return if Remainder == NULL
    sub     eax, dword ptr [Divisor + 0]
    sbb     edx, dword ptr [Divisor + 4]
Correct:
    jecxz   Return
    sub     esi, eax
    sbb     edi, edx                    // edi:esi <- remainder
    mov     [ecx], esi
    mov     [ecx + 4], edi
Return:
    mov     eax, ebx                    // eax <- quotient
    xor     edx, edx
  }
}

INT64
EFIAPI
CustomMathDivRemS64x64 (
  IN      INT64                     Dividend,
  IN      INT64                     Divisor,
  OUT     INT64                     *Remainder  OPTIONAL
  )
{
  INT64                             Quot;

  Quot = CustomMathDivRemU64x64 (
           (UINT64) (Dividend >= 0 ? Dividend : -Dividend),
           (UINT64) (Divisor >= 0 ? Divisor : -Divisor),
           (UINT64 *) Remainder
           );
  if (Remainder != NULL && Dividend < 0) {
    *Remainder = -*Remainder;
  }
  return (Dividend ^ Divisor) >= 0 ? Quot : -Quot;
}

INT64
EFIAPI
CustomDivS64x64Remainder (
  IN      INT64                     Dividend,
  IN      INT64                     Divisor,
  OUT     INT64                     *Remainder  OPTIONAL
  )
{
  return CustomMathDivRemS64x64 (Dividend, Divisor, Remainder);
}

/*
 * Divides a 64-bit signed value with a 64-bit signed value and returns
 * a 64-bit signed result.
 */
__declspec(naked) void __cdecl _alldiv (void)
{
  _asm {

    ;Entry:
    ;       Arguments are passed on the stack:
    ;               1st pushed: divisor (QWORD)
    ;               2nd pushed: dividend (QWORD)
    ;
    ;Exit:
    ;       EDX:EAX contains the quotient (dividend/divisor)
    ;       NOTE: this routine removes the parameters from the stack.
    ;
    ; Original local stack when calling _alldiv
    ;               -----------------
    ;               |               |
    ;               |---------------|
    ;               |               |
    ;               |--  Divisor  --|
    ;               |               |
    ;               |---------------|
    ;               |               |
    ;               |--  Dividend --|
    ;               |               |
    ;               |---------------|
    ;               |  ReturnAddr** |
    ;       ESP---->|---------------|
    ;

    ;
    ; Set up the local stack for NULL Reminder pointer
    ;
    xor  eax, eax
    push eax

    ;
    ; Set up the local stack for Divisor parameter
    ;
    mov  eax, [esp + 20]
    push eax
    mov  eax, [esp + 20]
    push eax

    ;
    ; Set up the local stack for Dividend parameter
    ;
    mov  eax, [esp + 20]
    push eax
    mov  eax, [esp + 20]
    push eax

    ;
    ; Call native DivS64x64Remainder of BaseLib
    ;
    call CustomDivS64x64Remainder

    ;
    ; Adjust stack
    ;
    add  esp, 20

    ret  16
  }
}

UINT64
EFIAPI
CustomDivU64x64Remainder (
  IN      UINT64                    Dividend,
  IN      UINT64                    Divisor,
  OUT     UINT64                    *Remainder  OPTIONAL
  )
{
  return CustomMathDivRemU64x64 (Dividend, Divisor, Remainder);
}

/*
 * Divides a 64-bit unsigned value with a 64-bit unsigned value and returns
 * a 64-bit unsigned result.
 */
__declspec(naked) void __cdecl _aulldiv (void)
{
  _asm {

    ; Original local stack when calling _aulldiv
    ;               -----------------
    ;               |               |
    ;               |---------------|
    ;               |               |
    ;               |--  Divisor  --|
    ;               |               |
    ;               |---------------|
    ;               |               |
    ;               |--  Dividend --|
    ;               |               |
    ;               |---------------|
    ;               |  ReturnAddr** |
    ;       ESP---->|---------------|
    ;

    ;
    ; Set up the local stack for NULL Reminder pointer
    ;
    xor  eax, eax
    push eax

    ;
    ; Set up the local stack for Divisor parameter
    ;
    mov  eax, [esp + 20]
    push eax
    mov  eax, [esp + 20]
    push eax

    ;
    ; Set up the local stack for Dividend parameter
    ;
    mov  eax, [esp + 20]
    push eax
    mov  eax, [esp + 20]
    push eax

    ;
    ; Call native DivU64x64Remainder of BaseLib
    ;
    call CustomDivU64x64Remainder

    ;
    ; Adjust stack
    ;
    add  esp, 20

    ret  16
  }
}
