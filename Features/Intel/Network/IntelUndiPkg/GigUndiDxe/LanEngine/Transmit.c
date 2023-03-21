/**************************************************************************

Copyright (c) 2020-2021, Intel Corporation. All rights reserved.

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

/* Helper macros */

/** Get pointer to specific Tx buffer entry from Tx ring

   @param[in]   R   Tx ring pointer
   @param[in]   i   Desired entry index

   @return    Pointer to Tx buffer entry indexed by i
 */
#define TRANSMIT_BUFFER_ENTRY(R, i) (&((R)->BufferEntries[i]))

/** Get virtual address specific Tx descriptor from Tx ring

   @param[in]   ring  Tx ring pointer
   @param[in]   i     Desired descriptor index

   @return    Pointer to Tx descriptor indexed by i
 */
#define TRANSMIT_DESCRIPTOR_VA(ring, i) \
  (TRANSMIT_DESCRIPTOR*) ((ring)->Descriptors.UnmappedAddress + ((i) * sizeof (TRANSMIT_DESCRIPTOR)))

/* Forward declarations of driver-specific functions */

/**
  Check whether adapter has finished processing specific Tx descriptor.

  @param[in]   TxDesc             Pointer to Tx descriptor

  @retval      TRUE               Descriptor has been processed.
  @retval      FALSE              Descriptor has not been processed.

**/
BOOLEAN
TransmitIsDescriptorDone (
  IN  TRANSMIT_DESCRIPTOR    *TxDesc
  );

/**
  Reset Tx descriptor to a valid (unused) state.

  @param[in]   TxDesc             Pointer to Tx descriptor.

**/
VOID
TransmitResetDescriptor (
  IN  TRANSMIT_DESCRIPTOR    *TxDesc
  );

/**
  Setup descriptor to be ready for processing by NIC.

  @param[in]   AdapterInfo        Pointer to the NIC data structure.
  @param[in]   TxDesc             Pointer to Tx descriptor.
  @param[in]   Packet             Physical address of the buffer holding packet
                                  to be sent.
  @param[in]   PacketLength       Length of the packet to be sent.

**/
VOID
TransmitSetupDescriptor (
  IN  DRIVER_DATA             *AdapterInfo,
  IN  TRANSMIT_DESCRIPTOR     *TxDesc,
  IN  EFI_PHYSICAL_ADDRESS    Packet,
  IN  UINT16                  PacketLength
  );

/**
  Update Tx ring tail register with Tx descriptor index.

  @param[in]   AdapterInfo        Pointer to the NIC data structure
  @param[in]   Index              Tx descriptor index.

**/
VOID
TransmitUpdateRingTail (
  IN  DRIVER_DATA   *AdapterInfo,
  IN  UINT8         Index
  );

/**
  Configure NIC to be ready to use initialized Tx queue.

  @param[in]   AdapterInfo        Pointer to the NIC data structure

  @retval EFI_SUCCESS             NIC successfully configured.
  @retval EFI_INVALID_PARAMETER   Parameters were NULL/invalid.
  @retval EFI_DEVICE_ERROR        NIC operation failure.
  @retval Others                  Underlying function failure.

**/
EFI_STATUS
TransmitConfigureQueue (
  IN  DRIVER_DATA   *AdapterInfo
  );

/**
  Enable Tx queue on the NIC.

  @param[in]   AdapterInfo        Pointer to the NIC data structure

  @retval EFI_SUCCESS             Tx queue has been enabled.
  @retval EFI_INVALID_PARAMETER   Parameters were NULL/invalid.
  @retval EFI_DEVICE_ERROR        NIC operation failure.
  @retval Others                  Underlying function failure.

**/
EFI_STATUS
TransmitEnableQueue (
  IN  DRIVER_DATA   *AdapterInfo
  );

/**
  Disable Tx queue on the NIC.

  @param[in]   AdapterInfo        Pointer to the NIC data structure

  @retval EFI_SUCCESS             Tx queue has been disabled.
  @retval EFI_INVALID_PARAMETER   Parameters were NULL/invalid.
  @retval EFI_DEVICE_ERROR        NIC operation failure.
  @retval Others                  Underlying function failure.

**/
EFI_STATUS
TransmitDisableQueue (
  IN  DRIVER_DATA   *AdapterInfo
  );

/**
  Perform actions before transmit ring resources are freed.

  @param[in]   AdapterInfo        Pointer to the NIC data structure

  @retval EFI_SUCCESS             Tx queue has been dismantled.
  @retval EFI_DEVICE_ERROR        NIC operation failure.
  @retval Others                  Underlying function failure.

**/
EFI_STATUS
TransmitDismantleQueue (
  IN  DRIVER_DATA   *AdapterInfo
  );

/* Private variables for blocking IO */
STATIC BOOLEAN  mInitializeLock = TRUE;
STATIC EFI_LOCK mLock;

/* Private receive engine functions */

/**
  Zero-init Tx buffer entries array.

  @param[in]   TxRing             Pointer to Tx ring structure.

**/
VOID
TransmitResetBufferEntries (
  IN TRANSMIT_RING   *TxRing
  )
{
  ASSERT (TxRing != NULL);
  ASSERT (TxRing->BufferEntries != NULL);
  ASSERT (TxRing->BufferCount != 0);

  ZeroMem (
    (VOID*) TxRing->BufferEntries,
    TxRing->BufferCount * sizeof (TRANSMIT_BUFFER_ENTRY)
    );
}

/**
  Obtain free Tx pair (descriptor + buffer entry).

  @param[in]   TxRing             Pointer to Tx ring structure.
  @param[out]  Desc               On output, pointer to a free Tx descriptor.
  @param[out]  BufferEntry        On output, pointer to a free Tx buffer entry.

  @retval EFI_SUCCESS             Free Tx pair obtained.
  @retval EFI_INVALID_PARAMETER   Parameters were NULL/invalid.
  @retval EFI_OUT_OF_RESOURCES    No free Tx pair available.

**/
EFI_STATUS
TransmitGetFreePair (
  IN  TRANSMIT_RING           *TxRing,
  OUT TRANSMIT_DESCRIPTOR     **Desc,
  OUT TRANSMIT_BUFFER_ENTRY   **BufferEntry
  )
{
  TRANSMIT_DESCRIPTOR     *TxDesc;
  TRANSMIT_BUFFER_ENTRY   *TxBufferEntry;

  DEBUGPRINT (TX, ("Getting free buffer entry\n"));

  if (TxRing == NULL
    || Desc == NULL
    || BufferEntry == NULL)
  {
    // Should not happen.
    DEBUGPRINT (CRITICAL, ("Invalid input parameters\n"));
    ASSERT (TxRing != NULL);
    ASSERT (Desc != NULL);
    ASSERT (BufferEntry != NULL);
    return EFI_INVALID_PARAMETER;
  }

  TxDesc        = TRANSMIT_DESCRIPTOR_VA (TxRing, TxRing->NextToUse);
  TxBufferEntry = TRANSMIT_BUFFER_ENTRY (TxRing, TxRing->NextToUse);

  if (TxBufferEntry->State != TRANSMIT_BUFFER_STATE_FREE) {
    DEBUGPRINT (TX, ("No free Tx pair\n"));
    return EFI_OUT_OF_RESOURCES;
  }

  *Desc         = TxDesc;
  *BufferEntry  = TxBufferEntry;

  DEBUGPRINT (
    TX,
    ("Free buffer found. Pair %d: DescAddr: %lX, EntryAddr: %lX\n",
      TxRing->NextToUse, TxDesc, TxBufferEntry)
    );

  return EFI_SUCCESS;
}

/* Public receive engine functions */

/**
  Initialize Tx ring structure of LAN engine.
  This function will allocate and initialize all the necessary resources.
  It also assumes (and checks) that TX_RING_FROM_ADAPTER (AdapterInfo)
  is zeroed.

  @param[in]   AdapterInfo        Pointer to the NIC data structure.
  @param[in]   BufferCount        Number of buffers Tx ring will accommodate.

  @retval EFI_SUCCESS             Tx ring initialized.
  @retval EFI_INVALID_PARAMETER   Parameters were NULL/invalid.
  @retval EFI_VOLUME_CORRUPTED    Tx ring structure was not zeroed.
  @retval EFI_OUT_OF_RESOURCES    Could not allocate necessary resources.
  @retval Others                  Other internal function error.

**/
EFI_STATUS
TransmitInitialize (
  IN DRIVER_DATA  *AdapterInfo,
  IN UINT16       BufferCount
  )
{
  EFI_STATUS      Status;
  TRANSMIT_RING   *TxRing;

  DEBUGPRINT (INIT, ("Initializing Tx ring.\n"));

  if (AdapterInfo == NULL
    || PCI_IO_FROM_ADAPTER (AdapterInfo) == NULL)
  {
    DEBUGPRINT (CRITICAL, ("Invalid input parameters.\n"));
    ASSERT (AdapterInfo != NULL);
    ASSERT (PCI_IO_FROM_ADAPTER (AdapterInfo) != NULL);
    return EFI_INVALID_PARAMETER;
  }

  if (BufferCount == 0) {
    DEBUGPRINT (CRITICAL, ("Invalid input parameters.\n"));
    ASSERT (BufferCount != 0);
    return EFI_INVALID_PARAMETER;
  }

  TxRing = TX_RING_FROM_ADAPTER (AdapterInfo);

  if (!IsZeroBuffer (TxRing, sizeof (TRANSMIT_RING))) {
    DEBUGPRINT (CRITICAL, ("Invalid initial contents of TRANSMIT_RING structure.\n"));
    ASSERT (IsZeroBuffer (TxRing, sizeof (TRANSMIT_RING)));
    return EFI_VOLUME_CORRUPTED;
  }

  TxRing->BufferCount = BufferCount;

  // Allocate Tx descriptors
  TxRing->Descriptors.Size = ALIGN (TxRing->BufferCount * sizeof (TRANSMIT_DESCRIPTOR), 4096);

  Status = UndiDmaAllocateCommonBuffer (
             PCI_IO_FROM_ADAPTER (AdapterInfo),
             &TxRing->Descriptors
             );

  if (EFI_ERROR (Status)) {
    DEBUGPRINT (CRITICAL, ("Failed to allocate descriptor memory via PciIo: %r\n", Status));
    ASSERT_EFI_ERROR (Status);
    goto Exit;
  }

  DEBUGPRINT (INIT, ("Allocated Tx descriptor buffer: %lX\n", TxRing->Descriptors.UnmappedAddress));

  // Allocate Tx mapping array
  TxRing->BufferEntries = AllocatePool (TxRing->BufferCount * sizeof (TRANSMIT_BUFFER_ENTRY));

  if (TxRing->BufferEntries == NULL) {
    DEBUGPRINT (CRITICAL, ("Failed to allocate buffer mapping array.\n"));
    ASSERT (TxRing->BufferEntries != NULL);
    goto ExitFreeDesc;
  }

  DEBUGPRINT (INIT, ("Allocated Tx buffer entries: %lX\n", TxRing->BufferEntries));

  // Zero-init descriptors area
  ZeroMem (
    (VOID*) TxRing->Descriptors.UnmappedAddress,
    TxRing->Descriptors.Size
    );

  // Initialize buffer entries
  TransmitResetBufferEntries (TxRing);

  DEBUGPRINT (INIT, ("Buffer entries have been reset\n"));

  // Configure device to use Tx queue
  Status = TransmitConfigureQueue (AdapterInfo);

  if (EFI_ERROR (Status)) {
    DEBUGPRINT (CRITICAL, ("Failed to configure device to use Tx queue: %r\n", Status));
    ASSERT_EFI_ERROR (Status);
    goto ExitFreeBufferEntries;
  }

  // Setup TxRing fields
  TxRing->Signature   = TRANSMIT_RING_SIGNATURE;
  TxRing->IsRunning   = FALSE;
  TxRing->NextToUse   = 0;
  TxRing->NextToUnmap = 0;
  TxRing->NextToFree  = 0;
  DEBUGPRINT (INIT, ("TxRing initialized.\n"));

  return EFI_SUCCESS;

ExitFreeBufferEntries:
  FreePool (TxRing->BufferEntries);
  TxRing->BufferEntries = NULL;

ExitFreeDesc:
  UndiDmaFreeCommonBuffer (
    PCI_IO_FROM_ADAPTER (AdapterInfo),
    &TxRing->Descriptors
    );

Exit:
  ZeroMem (TxRing, sizeof (TRANSMIT_RING));

  return Status;
}

/**
  Start Tx ring.

  @param[in]   AdapterInfo        Pointer to the NIC data structure.

  @retval EFI_SUCCESS             Tx ring started.
  @retval EFI_INVALID_PARAMETER   Parameters were NULL/invalid.
  @retval EFI_VOLUME_CORRUPTED    Tx ring was not initialized.
  @retval EFI_ALREADY_STARTED     Tx ring was already started before.
  @retval EFI_DEVICE_ERROR        NIC operation failure.
  @retval Others                  Underlying function failure.

**/
EFI_STATUS
TransmitStart (
  IN DRIVER_DATA  *AdapterInfo
  )
{
  EFI_STATUS      Status;
  TRANSMIT_RING   *TxRing;

  if (AdapterInfo == NULL) {
    DEBUGPRINT (CRITICAL, ("Invalid input parameters\n"));
    ASSERT (AdapterInfo != NULL);
    return EFI_INVALID_PARAMETER;
  }

  TxRing = TX_RING_FROM_ADAPTER (AdapterInfo);

  if (!IS_TX_RING_INITIALIZED (TxRing)) {
    DEBUGPRINT (CRITICAL, ("Tx ring is not initialized.\n"));
    ASSERT (IS_TX_RING_INITIALIZED (TxRing));
    return EFI_VOLUME_CORRUPTED;
  }

  if (TxRing->IsRunning) {
    DEBUGPRINT (CRITICAL, ("Tx ring is already started.\n"));
    return EFI_ALREADY_STARTED;
  }

  Status = TransmitEnableQueue (AdapterInfo);

  if (Status == EFI_SUCCESS) {
    DEBUGPRINT (TX, ("Tx ring is now running.\n"));
    TxRing->IsRunning = TRUE;
  }

  return Status;
}

/**
  Stop Tx ring.

  @param[in]   AdapterInfo        Pointer to the NIC data structure.

  @retval EFI_SUCCESS             Tx ring stopped.
  @retval EFI_INVALID_PARAMETER   Parameters were NULL/invalid.
  @retval EFI_VOLUME_CORRUPTED    Tx ring was not initialized.
  @retval EFI_NOT_STARTED         Tx ring was already stopped before.
  @retval EFI_DEVICE_ERROR        NIC operation failure.
  @retval Others                  Underlying function failure.

**/
EFI_STATUS
TransmitStop (
  IN DRIVER_DATA  *AdapterInfo
  )
{
  EFI_STATUS      Status;
  TRANSMIT_RING   *TxRing;

  if (AdapterInfo == NULL) {
    DEBUGPRINT (CRITICAL, ("Invalid input parameters\n"));
    ASSERT (AdapterInfo != NULL);
    return EFI_INVALID_PARAMETER;
  }

  TxRing = TX_RING_FROM_ADAPTER (AdapterInfo);

  if (!IS_TX_RING_INITIALIZED (TxRing)) {
    DEBUGPRINT (CRITICAL, ("Tx ring is not initialized.\n"));
    ASSERT (IS_TX_RING_INITIALIZED (TxRing));
    return EFI_VOLUME_CORRUPTED;
  }

  if (!TxRing->IsRunning) {
    DEBUGPRINT (CRITICAL, ("Tx ring is already stopped.\n"));
    return EFI_NOT_STARTED;
  }

  Status = TransmitDisableQueue (AdapterInfo);

  if (Status == EFI_SUCCESS) {
    DEBUGPRINT (TX, ("Tx ring is now stopped.\n"));
    TxRing->IsRunning = FALSE;
  }

  return Status;
}

/**
  Reset the transmit ring. Ring must be stopped first to call this function.

  @param[in]   AdapterInfo        Pointer to the NIC data structure.

  @retval EFI_SUCCESS             Tx ring has been reset.
  @retval EFI_VOLUME_CORRUPTED    Tx ring was not initialized.
  @retval EFI_ACCESS_DENIED       Tx ring is still running.
  @retval EFI_INVALID_PARAMETER   Parameters were NULL/invalid.
  @retval Others                  Underlying function error.

**/
EFI_STATUS
TransmitReset (
  IN  DRIVER_DATA   *AdapterInfo
  )
{
  EFI_STATUS      Status;
  TRANSMIT_RING   *TxRing;

  if (AdapterInfo == NULL) {
    DEBUGPRINT (CRITICAL, ("Invalid input parameters\n"));
    ASSERT (AdapterInfo != NULL);
    return EFI_INVALID_PARAMETER;
  }

  TxRing = TX_RING_FROM_ADAPTER (AdapterInfo);

  if (!IS_TX_RING_INITIALIZED (TxRing)) {
    DEBUGPRINT (CRITICAL, ("Tx ring not initialized (or pointer is invalid).\n"));
    ASSERT (IS_TX_RING_INITIALIZED (TxRing));
    return EFI_VOLUME_CORRUPTED;
  }

  if (TxRing->IsRunning) {
    DEBUGPRINT (CRITICAL, ("Tx ring is still running.\n"));
    return EFI_ACCESS_DENIED;
  }

  // Destroy adapter's queue context
  Status = TransmitDismantleQueue (AdapterInfo);

  if (EFI_ERROR (Status)) {
    DEBUGPRINT (CRITICAL, ("Failed to dismantle Tx queue.\n"));
    return Status;
  }

  // Reset descriptors
  ZeroMem (
    (VOID*) TxRing->Descriptors.UnmappedAddress,
    TxRing->Descriptors.Size
    );

  // Reconfigure queue
  Status = TransmitConfigureQueue (AdapterInfo);

  if (EFI_ERROR (Status)) {
    DEBUGPRINT (CRITICAL, ("Failed to reconfigure Tx queue.\n"));
    return Status;
  }

  TxRing->NextToUse   = 0;
  TxRing->NextToUnmap = 0;
  TxRing->NextToFree  = 0;

  return EFI_SUCCESS;
}

/**
  Clean up Tx ring structure of LAN engine.
  This function will release all the resources used by Tx ring.

  @param[in]   AdapterInfo        Pointer to the NIC data structure.

  @retval EFI_SUCCESS             Tx ring cleaned up.
  @retval EFI_INVALID_PARAMETER   Parameters were NULL/invalid.
  @retval EFI_VOLUME_CORRUPTED    Tx ring was not initialized.
  @retval EFI_ACCESS_DENIED       Tx ring is still running.
  @retval EFI_ACCESS_DENIED       Tx ring still holds unreleased packets.
  @retval Others                  Underlying function error.

**/
EFI_STATUS
TransmitCleanup (
  IN DRIVER_DATA  *AdapterInfo
  )
{
  EFI_STATUS            Status;
  TRANSMIT_RING         *TxRing;
  TRANSMIT_BUFFER_ENTRY *BufferEntry;

  DEBUGPRINT (INIT, ("Cleaning up Tx ring.\n"));

  if (AdapterInfo == NULL
    || PCI_IO_FROM_ADAPTER (AdapterInfo) == NULL)
  {
    DEBUGPRINT (CRITICAL, ("Invalid input parameters\n"));
    ASSERT (AdapterInfo != NULL);
    ASSERT (PCI_IO_FROM_ADAPTER (AdapterInfo) != NULL);
    return EFI_INVALID_PARAMETER;
  }

  TxRing = TX_RING_FROM_ADAPTER (AdapterInfo);

  if (!IS_TX_RING_INITIALIZED (TxRing)) {
    DEBUGPRINT (CRITICAL, ("Tx ring is not initialized.\n"));
    ASSERT (IS_TX_RING_INITIALIZED (TxRing));
    return EFI_VOLUME_CORRUPTED;
  }

  if (TxRing->IsRunning) {
    DEBUGPRINT (CRITICAL, ("Tx ring is still running.\n"));
    return EFI_ACCESS_DENIED;
  }

  ASSERT (TxRing->BufferCount != 0);
  ASSERT (TxRing->BufferEntries != NULL);

  // Are there any mapped transmit buffers?
  BufferEntry = TRANSMIT_BUFFER_ENTRY (TxRing, TxRing->NextToUnmap);

  if (BufferEntry->State == TRANSMIT_BUFFER_STATE_IN_QUEUE) {
    DEBUGPRINT (CRITICAL, ("There are still mapped transmit buffers!\n"));
    return EFI_ACCESS_DENIED;
  }

  BufferEntry = TRANSMIT_BUFFER_ENTRY (TxRing, TxRing->NextToFree);

  if (BufferEntry->State == TRANSMIT_BUFFER_STATE_UNMAPPED) {
    DEBUGPRINT (CRITICAL, ("There are still some transmit buffers owned by UNDI!\n"));
    return EFI_ACCESS_DENIED;
  }

  Status = TransmitDismantleQueue (AdapterInfo);

  if (EFI_ERROR (Status)) {
    DEBUGPRINT (CRITICAL, ("Failed to dismantle Tx queue: %r\n", Status));
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  Status = UndiDmaFreeCommonBuffer (
             PCI_IO_FROM_ADAPTER (AdapterInfo),
             &TxRing->Descriptors
             );

  if (EFI_ERROR (Status)) {
    DEBUGPRINT (CRITICAL, ("Failed to deallocate Tx descriptors: %r\n", Status));
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  DEBUGPRINT (INIT, ("Tx descriptors freed\n"));

  FreePool (TxRing->BufferEntries);

  DEBUGPRINT (INIT, ("Tx buffer entries freed\n"));

  ZeroMem (TxRing, sizeof (TRANSMIT_RING));

  DEBUGPRINT (INIT, ("Tx ring resources have been successfully freed\n"));

  return EFI_SUCCESS;
}

/**
  Traverse from NextToUnmap to NextToUse in order to find descriptors indicating
  finished Tx operation. If found, unmaps the packet buffer associated with that
  descriptor.

  @param[in]   AdapterInfo        Pointer to the NIC data structure.

  @retval EFI_SUCCESS             Some buffers were unmapped.
  @retval EFI_INVALID_PARAMETER   Parameters were NULL/invalid.
  @retval EFI_VOLUME_CORRUPTED    Tx ring was not initialized.
  @retval EFI_NOT_READY           No buffers were unmapped.
  @retval Others                  Unmap operation failure.

**/
EFI_STATUS
TransmitScanDescriptors (
  IN    DRIVER_DATA   *AdapterInfo
  )
{
  EFI_STATUS              Status;
  TRANSMIT_RING           *TxRing;
  TRANSMIT_DESCRIPTOR     *TxDesc;
  TRANSMIT_BUFFER_ENTRY   *BufferEntry;

  DEBUGPRINT (TX, ("Scanning descriptors for finished transmits...\n"));

  if (AdapterInfo == NULL) {
    ASSERT (AdapterInfo != NULL);
    return EFI_INVALID_PARAMETER;
  }

  TxRing = TX_RING_FROM_ADAPTER (AdapterInfo);

  if (!IS_TX_RING_INITIALIZED (TxRing)) {
    ASSERT (IS_TX_RING_INITIALIZED (TxRing));
    return EFI_VOLUME_CORRUPTED;
  }

  ASSERT (PCI_IO_FROM_ADAPTER (AdapterInfo) != NULL);

  Status = EFI_NOT_READY;

  do {
    TxDesc      = TRANSMIT_DESCRIPTOR_VA (TxRing, TxRing->NextToUnmap);
    BufferEntry = TRANSMIT_BUFFER_ENTRY (TxRing, TxRing->NextToUnmap);

    ASSERT (TxDesc != NULL);
    ASSERT (BufferEntry != NULL);

    DEBUGPRINT (TX, ("Checking pair %d...\n", TxRing->NextToUnmap));

    if (BufferEntry->State != TRANSMIT_BUFFER_STATE_IN_QUEUE) {
      // Nothing to do.
      break;
    }

    if (!TransmitIsDescriptorDone (TxDesc)) {
      // Transmission not done yet.
      DEBUGPRINT (TX, ("Pair %d - descriptor not done\n", TxRing->NextToUnmap));
      break;
    }

    DEBUGPRINT (TX, ("Pair %d - descriptor done. VA: %lX\n", TxRing->NextToUnmap, TxDesc));

    // Clear up descriptor
    TransmitResetDescriptor (TxDesc);

    // Unmap buffer
    ASSERT (BufferEntry->Mapping.PhysicalAddress != 0);

    DEBUGPRINT (
      TX,
      ("Unmapping buffer %d. Entry address: %lX\n",
        TxRing->NextToUnmap, BufferEntry)
      );

    Status = UndiDmaUnmapMemory (
               PCI_IO_FROM_ADAPTER (AdapterInfo),
               &BufferEntry->Mapping
               );

    if (EFI_ERROR (Status)) {
      ASSERT_EFI_ERROR (Status);
      break;
    }

    DEBUGPRINT (
      TX,
      ("Pair %d - Entry->State = TRANSMIT_BUFFER_STATE_UNMAPPED\n",
        TxRing->NextToUnmap)
      );
    BufferEntry->State = TRANSMIT_BUFFER_STATE_UNMAPPED;

    if (++TxRing->NextToUnmap == TxRing->BufferCount) {
      TxRing->NextToUnmap = 0;
    }
    DEBUGPRINT (TX, ("TxRing->NextToUnmap is now %d\n", TxRing->NextToUnmap));

  } while (TxRing->NextToUnmap != TxRing->NextToUse);

  return Status;
}

/**
  Enqueue the packet in Tx queue.

  @param[in]   AdapterInfo        Pointer to the NIC data structure.
  @param[in]   Packet             Address of packet buffer
  @param[in]   PacketLength       Length of packet to be sent
  @param[in]   IsBlocking         Control whether function should wait for
                                  Tx operation completion.

  @retval EFI_SUCCESS             Packet successfully enqueued/sent.
  @retval EFI_INVALID_PARAMETER   Parameters were NULL/invalid.
  @retval EFI_VOLUME_CORRUPTED    Tx ring was not initialized.
  @retval EFI_NOT_STARTED         Tx ring was not started.
  @retval Others                  Underlying function error.

**/
EFI_STATUS
TransmitSend (
  IN  DRIVER_DATA           *AdapterInfo,
  IN  EFI_VIRTUAL_ADDRESS   Packet,
  IN  UINT16                PacketLength,
  IN  BOOLEAN               IsBlocking
  )
{
  TRANSMIT_RING           *TxRing;
  TRANSMIT_DESCRIPTOR     *TxDesc;
  TRANSMIT_BUFFER_ENTRY   *BufferEntry;
  EFI_STATUS              Status;

  DEBUGPRINT (TX, ("Putting packet for sending\n"));

  if (AdapterInfo == NULL
    || Packet == 0
    || PacketLength == 0)
  {
    DEBUGPRINT (CRITICAL, ("Invalid input parameters\n"));
    ASSERT (AdapterInfo != NULL);
    ASSERT (Packet != 0);
    ASSERT (PacketLength != 0);
    return EFI_INVALID_PARAMETER;
  }

  TxRing = TX_RING_FROM_ADAPTER (AdapterInfo);

  if (!IS_TX_RING_INITIALIZED (TxRing)) {
    DEBUGPRINT (CRITICAL, ("Tx ring is not initialized.\n"));
    ASSERT (IS_TX_RING_INITIALIZED (TxRing));
    return EFI_VOLUME_CORRUPTED;
  }

  if (!TxRing->IsRunning) {
    DEBUGPRINT (CRITICAL, ("Tx ring is not running.\n"));
    return EFI_NOT_STARTED;
  }

  // Test if there is a free buffer mapping
  // This equals checking whether we have a free Tx descriptor
  Status = TransmitGetFreePair (
             TxRing,
             &TxDesc,
             &BufferEntry
             );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  BufferEntry->Mapping.UnmappedAddress  = Packet;
  BufferEntry->Mapping.Size             = PacketLength;

  // Map the buffer via PciIo
  ASSERT (PCI_IO_FROM_ADAPTER (AdapterInfo) != NULL);
  Status = UndiDmaMapMemoryRead (
             PCI_IO_FROM_ADAPTER (AdapterInfo),
             &BufferEntry->Mapping
             );

  if (EFI_ERROR (Status)) {
    DEBUGPRINT (CRITICAL, ("Failed to map Tx buffer\n"));
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  DEBUGPRINT (TX, ("Buffer VA: %lX\n", BufferEntry->Mapping.UnmappedAddress));
  DEBUGPRINT (TX, ("Buffer PA: %lX\n", BufferEntry->Mapping.PhysicalAddress));

  TxDesc = TRANSMIT_DESCRIPTOR_VA (TxRing, TxRing->NextToUse);

  // Advance Tx ring tail
  if (++TxRing->NextToUse == TxRing->BufferCount) {
    TxRing->NextToUse = 0;
  }

  DEBUGPRINT (TX, ("TxRing->NextToUse is now %d\n", TxRing->NextToUse));

  // Insert buffer's physical address into descriptor and mark descriptor to send
  // (via driver-specific function)
  TransmitSetupDescriptor (
    AdapterInfo,
    TxDesc,
    BufferEntry->Mapping.PhysicalAddress,
    PacketLength
    );

  DEBUGPRINT (TX, ("Packet has been bound to desc\n"));

  BufferEntry->State = TRANSMIT_BUFFER_STATE_IN_QUEUE;

  // Move the ring tail to make adapter initiate transmit
  TransmitUpdateRingTail (AdapterInfo, (UINT8) TxRing->NextToUse);

  DEBUGPRINT (TX, ("Tx tail updated\n"));

#define TX_RING_SEND_TIMEOUT      10000
#define TX_RING_SEND_WAIT_PERIOD  1

  if (IsBlocking) {
    INT32  WaitTime = TX_RING_SEND_TIMEOUT;

    DEBUGPRINT (TX, ("Blocking call\n"));

    // Wait for descriptor done
    while (!TransmitIsDescriptorDone (TxDesc)) {
      gBS->Stall (TX_RING_SEND_WAIT_PERIOD);
      WaitTime -= TX_RING_SEND_WAIT_PERIOD;

      if (WaitTime <= 0) {
        DEBUGPRINT (TX | CRITICAL, ("Tx overall failure.\n"));
        return PXE_STATCODE_DEVICE_FAILURE;
      }
    }

    // Go through the descriptor cleanup
    // This should clean at least the descriptor that was tied by this function
    TransmitScanDescriptors (AdapterInfo);
  }

  return EFI_SUCCESS;
}

/**
  Retrieve Tx buffer for which Tx operations were completed, from Tx ring.

  @param[in]   AdapterInfo        Pointer to the NIC data structure.
  @param[out]  Buffer             Pointer to memory to hold Tx buffer address

  @retval EFI_SUCCESS             Packet has been retrieved.
  @retval EFI_INVALID_PARAMETER   Parameters were NULL/invalid.
  @retval EFI_VOLUME_CORRUPTED    Tx ring was not initialized.
  @retval EFI_NOT_READY           No packets to retrieve.

**/
EFI_STATUS
TransmitReleaseBuffer (
  IN    DRIVER_DATA           *AdapterInfo,
  OUT   EFI_VIRTUAL_ADDRESS   *Buffer
  )
{
  TRANSMIT_RING           *TxRing;
  TRANSMIT_BUFFER_ENTRY   *BufferEntry;

  DEBUGPRINT (TX, ("Trying to release Tx buffer\n"));

  if (AdapterInfo == NULL
    || Buffer == NULL)
  {
    DEBUGPRINT (CRITICAL, ("Invalid input parameters.\n"));
    ASSERT (AdapterInfo != NULL);
    ASSERT (Buffer != NULL);
    return EFI_INVALID_PARAMETER;
  }

  TxRing = TX_RING_FROM_ADAPTER (AdapterInfo);

  if (!IS_TX_RING_INITIALIZED (TxRing)) {
    DEBUGPRINT (CRITICAL, ("Tx ring is not initialized.\n"));
    ASSERT (IS_TX_RING_INITIALIZED (TxRing));
    return EFI_VOLUME_CORRUPTED;
  }

  BufferEntry = TRANSMIT_BUFFER_ENTRY (TxRing, TxRing->NextToFree);

  DEBUGPRINT (TX, ("Pair %d\n", TxRing->NextToFree));

  ASSERT (BufferEntry != NULL);

  if (BufferEntry->State != TRANSMIT_BUFFER_STATE_UNMAPPED) {
    DEBUGPRINT (TX, ("No buffers to be freed\n"));
    return EFI_NOT_READY;
  }

  ASSERT (BufferEntry->Mapping.PhysicalAddress == 0);

  // Provide the virtual address
  *Buffer = BufferEntry->Mapping.UnmappedAddress;

  DEBUGPRINT (
    TX,
    ("Pair %d - Entry->State = TRANSMIT_BUFFER_STATE_FREE\n",
      TxRing->NextToFree)
    );
  BufferEntry->State = TRANSMIT_BUFFER_STATE_FREE;

  if (++TxRing->NextToFree == TxRing->BufferCount) {
    TxRing->NextToFree = 0;
  }

  DEBUGPRINT (TX, ("TxRing->NextToFree is now %d\n", TxRing->NextToFree));
  DEBUGPRINT (TX, ("Returning buffer %lX\n", *Buffer));

  return EFI_SUCCESS;
}

/** Blocking function called to assure that we are not swapped out from
   the queue while moving TX ring tail pointer.

   @param[in]   AdapterInfo   Pointer to the NIC data structure information
                              the UNDI driver is layering on
   @param[in]   Flag          Block flag

   @return   According to Flag setting (TRUE/FALSE) we're acquiring or releasing EFI lock
**/
VOID
TransmitLockIo (
  IN DRIVER_DATA    *AdapterInfo,
  IN UINT32         Flag
  )
{
  if (AdapterInfo->Block != NULL) {
    (*AdapterInfo->Block) (AdapterInfo->UniqueId, Flag);
  } else {
    if (mInitializeLock) {
      EfiInitializeLock (&mLock, TPL_NOTIFY);
      mInitializeLock = FALSE;
    }

    if (Flag != 0) {
      EfiAcquireLock (&mLock);
    } else {
      EfiReleaseLock (&mLock);
    }
  }
}
