/**************************************************************************

Copyright (c) 2018 - 2019, Intel Corporation. All rights reserved.

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
#ifndef _DMA_H_
#define _DMA_H_

#include <Uefi.h>
#include <Protocol/PciIo.h>

// Structure for DMA common buffer mapping
typedef struct _UNDI_DMA_MAPPING {
  EFI_VIRTUAL_ADDRESS     UnmappedAddress;
  EFI_PHYSICAL_ADDRESS    PhysicalAddress;
  UINTN                   Size;
  VOID                    *Mapping;
} UNDI_DMA_MAPPING;

/** Allocate DMA common buffer (aligned to the page)

    @param[in]  PciIo         Pointer to PCI IO protocol installed on controller
                              handle.
    @param[in]  DmaMapping    Pointer to DMA mapping structure. Size must be filled in.

    @retval     EFI_INVALID_PARAMETER   Bad arguments provided.
    @retval     EFI_OUT_OF_RESOURCES    Failed to map whole requested area
    @retval     EFI_SUCCESS             Allocation succeeded.
**/
EFI_STATUS
UndiDmaAllocateCommonBuffer (
  EFI_PCI_IO_PROTOCOL       *PciIo,
  UNDI_DMA_MAPPING          *DmaMapping
  );

/** Free DMA common buffer

    @param[in]  PciIo         Pointer to PCI IO protocol installed on controller
                              handle.
    @param[in]  DmaMapping    Pointer to DMA mapping structure (previously
                              filled by allocation function.

    @retval     EFI_INVALID_PARAMETER   Bad arguments provided.
    @retval     EFI_SUCCESS             Deallocation succeeded.
**/
EFI_STATUS
UndiDmaFreeCommonBuffer (
  EFI_PCI_IO_PROTOCOL       *PciIo,
  UNDI_DMA_MAPPING          *DmaMapping
  );

/** Map DMA buffer as common buffer (read/write)

    @param[in]  PciIo         Pointer to PCI IO protocol installed on controller
                              handle.
    @param[in]  DmaMapping    Pointer to DMA mapping structure (previously
                              filled by allocation function)

    @retval     EFI_INVALID_PARAMETER   Bad arguments provided.
    @retval     EFI_SUCCESS             Mapping succeeded.
**/
EFI_STATUS
UndiDmaMapCommonBuffer (
  EFI_PCI_IO_PROTOCOL       *PciIo,
  UNDI_DMA_MAPPING          *DmaMapping
  );

/** Map DMA buffer for read operations

    @param[in]  PciIo         Pointer to PCI IO protocol installed on controller
                              handle.
    @param[in]  DmaMapping    Pointer to DMA mapping structure (previously
                              filled by allocation function)

    @retval     EFI_INVALID_PARAMETER   Bad arguments provided.
    @retval     EFI_SUCCESS             Mapping succeeded.
**/
EFI_STATUS
UndiDmaMapMemoryRead (
  EFI_PCI_IO_PROTOCOL       *PciIo,
  UNDI_DMA_MAPPING          *DmaMapping
  );

/** Unmap DMA buffer

    @param[in]  PciIo         Pointer to PCI IO protocol installed on controller
                              handle.
    @param[in]  DmaMapping    Pointer to DMA mapping structure (previously
                              filled by allocation function)

    @retval     EFI_INVALID_PARAMETER   Bad arguments provided.
    @retval     EFI_SUCCESS             Unmapping succeeded.
**/
EFI_STATUS
UndiDmaUnmapMemory (
  EFI_PCI_IO_PROTOCOL       *PciIo,
  UNDI_DMA_MAPPING          *DmaMapping
  );

#endif /* _DMA_H_ */
