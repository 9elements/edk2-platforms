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

#include "CommonDriver.h"
#include "Dma.h"

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
  )
{
  EFI_STATUS    Status;
  UINTN         RequestedSize;

  if (PciIo == NULL || DmaMapping == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (DmaMapping->Size == 0) {
    return EFI_INVALID_PARAMETER;
  }

  Status = PciIo->AllocateBuffer (
             PciIo,
             AllocateAnyPages,
             EfiBootServicesData,
             EFI_SIZE_TO_PAGES (DmaMapping->Size),
             (VOID **) &DmaMapping->UnmappedAddress,
             0
             );

  if (EFI_ERROR (Status)) {
    DEBUGPRINT (CRITICAL, ("Failed to allocate DMA buffer: %r\n", Status));
    DmaMapping->Size = 0;
  }

  RequestedSize = DmaMapping->Size;

  Status = UndiDmaMapCommonBuffer (PciIo, DmaMapping);

  if (EFI_ERROR (Status)) {
    DEBUGPRINT (CRITICAL, ("DMA buffer mapping failed with: %r\n", Status));
    DEBUGWAIT (CRITICAL);
    goto FREE_BUF_ON_ERROR;
  }

  if (DmaMapping->Size != RequestedSize) {
    DEBUGPRINT (CRITICAL, ("Failed to map whole DMA area. Requested: %d, Obtained: %d\n", RequestedSize, DmaMapping->Size));
    DEBUGWAIT (CRITICAL);
    Status = EFI_OUT_OF_RESOURCES;
    goto UNMAP_ON_ERROR;
  }

  DEBUGPRINT (DMA, ("DMA allocation OK. VA: %lX, PA: %lX, Size: %d, Mapping: %x\n",
    DmaMapping->UnmappedAddress,
    DmaMapping->PhysicalAddress,
    DmaMapping->Size,
    DmaMapping->Mapping
    ));
  DEBUGWAIT (DMA);

  return EFI_SUCCESS;

UNMAP_ON_ERROR:
  UndiDmaUnmapMemory (PciIo, DmaMapping);

FREE_BUF_ON_ERROR:
  PciIo->FreeBuffer (
           PciIo,
           EFI_SIZE_TO_PAGES (DmaMapping->Size),
           (VOID *) (UINTN) DmaMapping->UnmappedAddress
           );
  DmaMapping->Size = 0;
  DmaMapping->UnmappedAddress = 0;

  return Status;
}

/** Free DMA common buffer

    @param[in]  PciIo         Pointer to PCI IO protocol installed on controller
                              handle.
    @param[in]  DmaMapping    Pointer to DMA mapping structure (previously
                              filled by allocation function)

    @retval     EFI_INVALID_PARAMETER   Bad arguments provided.
    @retval     EFI_SUCCESS             Deallocation succeeded.
**/
EFI_STATUS
UndiDmaFreeCommonBuffer (
  EFI_PCI_IO_PROTOCOL       *PciIo,
  UNDI_DMA_MAPPING          *DmaMapping
  )
{
  EFI_STATUS    Status;

  if (PciIo == NULL || DmaMapping == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (DmaMapping->Size == 0 || DmaMapping->UnmappedAddress == 0 ||
      DmaMapping->PhysicalAddress == 0) {
    return EFI_INVALID_PARAMETER;
  }

  Status = UndiDmaUnmapMemory (PciIo, DmaMapping);

  if (EFI_ERROR (Status)) {
    DEBUGPRINT (CRITICAL, ("Could not unmap DMA memory: %r\n", Status));
    DEBUGWAIT (CRITICAL);
    return Status;
  }

  if (!mExitBootServicesTriggered) {
    PciIo->FreeBuffer (
             PciIo,
             EFI_SIZE_TO_PAGES (DmaMapping->Size),
             (VOID *) (UINTN) DmaMapping->UnmappedAddress
             );
  }

  DmaMapping->UnmappedAddress = 0;
  DmaMapping->Size = 0;

  return EFI_SUCCESS;
}

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
  )
{
  if (PciIo == NULL || DmaMapping == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (DmaMapping->Size == 0 || DmaMapping->UnmappedAddress == 0) {
    return EFI_INVALID_PARAMETER;
  }

  return PciIo->Map (
                  PciIo,
                  EfiPciIoOperationBusMasterCommonBuffer,
                  (VOID *) (UINTN) DmaMapping->UnmappedAddress,
                  &DmaMapping->Size,
                  &DmaMapping->PhysicalAddress,
                  &DmaMapping->Mapping
                  );
}

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
  )
{
  if (PciIo == NULL || DmaMapping == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (DmaMapping->Size == 0 || DmaMapping->UnmappedAddress == 0) {
    return EFI_INVALID_PARAMETER;
  }

  return PciIo->Map (
                  PciIo,
                  EfiPciIoOperationBusMasterRead,
                  (VOID *) (UINTN) DmaMapping->UnmappedAddress,
                  &DmaMapping->Size,
                  &DmaMapping->PhysicalAddress,
                  &DmaMapping->Mapping
                  );
}

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
  )
{
  EFI_STATUS    Status;

  if (PciIo == NULL || DmaMapping == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status = PciIo->Unmap (PciIo, DmaMapping->Mapping);

  if (Status == EFI_SUCCESS) {
    DmaMapping->PhysicalAddress = 0;
    DmaMapping->Mapping = NULL;
  }

  return Status;
}

