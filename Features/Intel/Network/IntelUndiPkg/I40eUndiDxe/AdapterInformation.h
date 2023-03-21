/**************************************************************************

Copyright (c) 2013 - 2020, Intel Corporation. All rights reserved.

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

#ifndef ADAPTER_INFORMATION_H_
#define ADAPTER_INFORMATION_H_

#include <Protocol/AdapterInformation.h>

typedef struct UNDI_PRIVATE_DATA_S UNDI_PRIVATE_DATA;


typedef
EFI_STATUS
(* GET_INFORMATION_BLOCK) (
  EFI_ADAPTER_INFORMATION_PROTOCOL *This,
  VOID                             **InformationBlock,
  UINTN                            *InformationBlockSize
  );

typedef
EFI_STATUS
(* SET_INFORMATION_BLOCK) (
  EFI_ADAPTER_INFORMATION_PROTOCOL *This,
  VOID                             *InformationBlock,
  UINTN                            InformationBlockSize
  );

typedef struct {
  EFI_GUID    Guid;

  GET_INFORMATION_BLOCK   GetInformationBlock;
  SET_INFORMATION_BLOCK   SetInformationBlock;
} EFI_ADAPTER_INFORMATION_TYPE_DESCRIPTOR;

#define MAX_SUPPORTED_INFORMATION_TYPE 20

/** Adds supported Information Type Descriptor to the list.
  Call before protocol installation.

  @param[in]   InformationDescriptor  Supported Information Descriptor

  @retval    EFI_SUCCESS             Descriptor added successfully
  @retval    EFI_INVALID_PARAMETER   InformationDescriptor is NULL
  @retval    EFI_OUT_OF_RESOURCES    Existent information count is maximum
  @retval    EFI_ALREADY_STARTED     Specified InformationDescriptor already added
**/
EFI_STATUS
AddSupportedInformationType (
  IN EFI_ADAPTER_INFORMATION_TYPE_DESCRIPTOR  *InformationDescriptor
  );

/** Initializes and installs Adapter Info Protocol on adapter

   @param[in]   UndiPrivateData   Driver private data structure

   @retval    EFI_SUCCESS   Protocol installed successfully
   @retval    !EFI_SUCCESS  Failed to install and initialize protocol
**/
EFI_STATUS
InitAdapterInformationProtocol (
  IN UNDI_PRIVATE_DATA *UndiPrivateData
  );

/** Uninstalls Adapter Info Protocol

   @param[in]   UndiPrivateData   Driver private data structure

   @retval     EFI_SUCCESS    Protocol uninstalled successfully
   @retval     !EFI_SUCCESS   Failed to uninstall protocol
**/
EFI_STATUS
UninstallAdapterInformationProtocol (
  IN UNDI_PRIVATE_DATA *UndiPrivateData
  );

#endif /* ADAPTER_INFORMATION_H_ */

