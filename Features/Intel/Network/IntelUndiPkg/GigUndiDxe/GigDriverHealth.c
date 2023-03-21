/**************************************************************************

Copyright (c) 2020, Intel Corporation. All rights reserved.

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
#include "E1000.h"

HEALTH_MSG_ENTRY mDriverHealthEntry[] = { {0, NULL} };


/** Retrieves adapter specific health status information from SW/FW/HW.

   @param[in]   UndiPrivateData   Driver private data structure
   @param[out]  ErrorCount        On return, number of errors found, if any
   @param[out]  ErrorIndexes      On return, array that holds found health error indexes (from global array).
                                  Valid only when ErrorCount != 0. Must be allocated by caller

   @retval  EFI_SUCCESS            Adapter health information retrieved successfully
**/
EFI_STATUS
GetAdapterHealthStatus (
  IN   UNDI_PRIVATE_DATA  *UndiPrivateData,
  OUT  UINT16             *ErrorCount,
  OUT  UINT16             *ErrorIndexes
  )
{
  ASSERT (ErrorCount != NULL);

  *ErrorCount = 0; // E1000 driver always reports being healthy

  return EFI_SUCCESS;
}
