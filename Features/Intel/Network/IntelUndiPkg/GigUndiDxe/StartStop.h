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
#ifndef START_STOP_H_
#define START_STOP_H_

#include "E1000.h"

#define EFI_DRIVER_STOP_PROTOCOL_GUID \
  { 0x34d59603, 0x1428, 0x4429, { 0xa4, 0x14, 0xe6, 0xb3, 0xb5, 0xfd, 0x7d, 0xc1 } }

typedef struct EFI_DRIVER_STOP_PROTOCOL_S  EFI_DRIVER_STOP_PROTOCOL;

/** Issues a call to stop the driver so diagnostic application can access the hardware.

   @param[in]   This       Pointer to the EFI_DRIVER_STOP_PROTOCOL instance.

   @retval   EFI_SUCCESS   Driver is stopped successfully
**/
typedef
EFI_STATUS
(EFIAPI *EFI_DRIVER_STOP_PROTOCOL_STOP_DRIVER) (
  IN EFI_DRIVER_STOP_PROTOCOL *This
  );

/** Issues a call to start the driver after diagnostic application has completed.

   @param[in]   This       Pointer to the EFI_DRIVER_STOP_PROTOCOL instance.

   @retval   EFI_SUCCESS   If driver has restarted successfully
**/
typedef
EFI_STATUS
(EFIAPI *EFI_DRIVER_STOP_PROTOCOL_START_DRIVER) (
  IN EFI_DRIVER_STOP_PROTOCOL *This
  );

struct EFI_DRIVER_STOP_PROTOCOL_S {
  EFI_DRIVER_STOP_PROTOCOL_STOP_DRIVER StopDriver;
  EFI_DRIVER_STOP_PROTOCOL_START_DRIVER StartDriver;
};

#endif /* START_STOP_H_ */
