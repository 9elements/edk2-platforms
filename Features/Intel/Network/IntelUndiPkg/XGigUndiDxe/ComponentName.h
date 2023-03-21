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
#ifndef COMPONENT_NAME_H_
#define COMPONENT_NAME_H_

#define EFI_2_80_SYSTEM_TABLE_REVISION  ((2 << 16) | (80))

#if UNDI_10G
#endif /*UNDI_10G*/

/** Searches through the branding string list for the best possible match for the controller
   associated with UndiPrivateData.

   @param[in,out]   UndiPrivateData   Driver private data structure

   @return    Controller name initialized according to device
**/
VOID
ComponentNameInitializeControllerName (
  UNDI_PRIVATE_DATA *UndiPrivateData
  );

/** Retrieves a Unicode string that is the user readable name of the EFI Driver.

   @param[in]   This   A pointer to the EFI_COMPONENT_NAME_PROTOCOL instance.
   @param[in]   Language   A pointer to a three character ISO 639-2 language identifier.
                           This is the language of the driver name that that the caller
                           is requesting, and it must match one of the languages specified
                           in SupportedLanguages.  The number of languages supported by a
                           driver is up to the driver writer.
   @param[out]   DriverName   A pointer to the Unicode string to return.  This Unicode string
                              is the name of the driver specified by This in the language
                              specified by Language.

   @retval   EFI_SUCCES             The Unicode string for the Driver specified by This
                                    and the language specified by Language was returned
                                    in DriverName.
   @retval   EFI_INVALID_PARAMETER  Language is NULL.
   @retval   EFI_INVALID_PARAMETER  DriverName is NULL.
   @retval   EFI_UNSUPPORTED        The driver specified by This does not support the
                                    language specified by Language.
**/
EFI_STATUS
EFIAPI
ComponentNameGetDriverName (
  IN  EFI_COMPONENT_NAME_PROTOCOL *This,
  IN  CHAR8 *                      Language,
  OUT CHAR16 **                    DriverName
  );

#endif /* COMPONENT_NAME_H_ */
