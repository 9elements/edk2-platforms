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

/** Get virtual address of specific Rx descriptor from Rx ring

   @param[in]   ring  Rx ring pointer
   @param[in]   i     Desired descriptor index

   @return    Pointer to Rx descriptor indexed by i
 */
#define RECEIVE_DESCRIPTOR_VA(ring, i) \
  (RECEIVE_DESCRIPTOR*) ((ring)->Descriptors.UnmappedAddress + ((i) * sizeof (RECEIVE_DESCRIPTOR)))

/** Get virtual address of specific Rx buffer from Rx ring

   @param[in]   ring  Rx ring pointer
   @param[in]   i     Desired buffer index

   @return    Pointer to Rx buffer indexed by i
 */
#define RECEIVE_BUFFER_VA(ring, i) \
  (UINT8*) ((ring)->Buffers.UnmappedAddress + ((i) * (ring)->BufferSize))

/** Get physical address of specific Rx buffer from Rx ring

   @param[in]   ring  Rx ring pointer
   @param[in]   i     Desired buffer index

   @return    Pointer to Rx buffer indexed by i
 */
#define RECEIVE_BUFFER_PA(ring, i) \
  (EFI_PHYSICAL_ADDRESS) ((ring)->Buffers.PhysicalAddress + ((i) * (ring)->BufferSize))


/* Forward declarations of driver-specific functions */

/**
  Write physical address of the Rx buffer to a specific field within
  Rx descriptor.

  @param[in]   RxDesc             Pointer to Rx descriptor
  @param[in]   RxBuffer           Physical address of Rx buffer

**/
VOID
ReceiveAttachBufferToDescriptor (
  IN  RECEIVE_DESCRIPTOR    *RxDesc,
  IN  EFI_PHYSICAL_ADDRESS  RxBuffer
  );

/**
  Check whether adapter has finished processing specific Rx descriptor.
  Optional parameters can be provided to fill in additional information on
  received packet.

  @param[in]   RxDesc             Pointer to Rx descriptor.
  @param[out]  PacketLength       On output, length of received packet.
  @param[out]  HeaderLength       On output, length of received packet's header.
  @param[out]  RxError            On output, descriptor's RXERROR field content.
  @param[out]  PacketType         On output, descriptor's PTYPE field content.

  @retval      TRUE               Descriptor has been processed.
  @retval      FALSE              Descriptor has not been processed.

**/
BOOLEAN
ReceiveIsDescriptorDone (
  IN  RECEIVE_DESCRIPTOR  *RxDesc,
  OUT UINT16              *PacketLength   OPTIONAL,
  OUT UINT16              *HeaderLength   OPTIONAL,
  OUT UINT8               *RxError        OPTIONAL,
  OUT UINT8               *PacketType     OPTIONAL
  );

/**
  Update device's Rx tail register with a given descriptor ID.

  @param[in]   AdapterInfo        Pointer to the NIC data structure.
  @param[in]   DescId             Descriptor ID to be written.

**/
VOID
ReceiveUpdateTail (
  IN  DRIVER_DATA   *AdapterInfo,
  IN  UINT16        DescId
  );

/**
  Configure NIC to be ready to use initialized Rx queue.

  @param[in]   AdapterInfo        Pointer to the NIC data structure

  @retval EFI_SUCCESS             NIC successfully configured.
  @retval EFI_INVALID_PARAMETER   Parameters were NULL/invalid.
  @retval EFI_DEVICE_ERROR        NIC operation failure.
  @retval Others                  Underlying function failure.

**/
EFI_STATUS
ReceiveConfigureQueue (
  IN  DRIVER_DATA   *AdapterInfo
  );

/**
  Enable Rx queue on the NIC.

  @param[in]   AdapterInfo        Pointer to the NIC data structure

  @retval EFI_SUCCESS             Rx queue has been enabled.
  @retval EFI_INVALID_PARAMETER   Parameters were NULL/invalid.
  @retval EFI_DEVICE_ERROR        NIC operation failure.
  @retval Others                  Underlying function failure.

**/
EFI_STATUS
ReceiveEnableQueue (
  IN  DRIVER_DATA   *AdapterInfo
  );

/**
  Disable Rx queue on the NIC.

  @param[in]   AdapterInfo        Pointer to the NIC data structure

  @retval EFI_SUCCESS             Rx queue has been disabled.
  @retval EFI_INVALID_PARAMETER   Parameters were NULL/invalid.
  @retval EFI_DEVICE_ERROR        NIC operation failure.
  @retval Others                  Underlying function failure.

**/
EFI_STATUS
ReceiveDisableQueue (
  IN  DRIVER_DATA   *AdapterInfo
  );

/**
  Perform actions before receive ring resources are freed.

  @param[in]   AdapterInfo        Pointer to the NIC data structure

  @retval EFI_SUCCESS             Rx queue has been dismantled.
  @retval EFI_DEVICE_ERROR        NIC operation failure.
  @retval Others                  Underlying function failure.

**/
EFI_STATUS
ReceiveDismantleQueue (
  IN  DRIVER_DATA   *AdapterInfo
  );

/* Private receive engine functions */

/**
  Initialize all Rx descriptors (init fields, write Rx buffer addresses, etc.).

  @param[in]   RxRing             Pointer to Rx ring structure.

  @retval EFI_SUCCESS             Rx descriptors successfully initialized.

**/
EFI_STATUS
ReceiveInitializeDescriptors (
  RECEIVE_RING    *RxRing
  )
{
  UINT16    i;

  ASSERT (RxRing != NULL);

  // Are descriptors & buffers allocated and mapped?
  // EFI_SUCCESS from UndiDmaAllocateCommonBuffer also ensures that
  // memory area sizes for descriptors and Rx buffers are correct.
  ASSERT (RxRing->Descriptors.PhysicalAddress != 0);
  ASSERT (RxRing->Buffers.PhysicalAddress != 0);

  for (i = 0; i < RxRing->BufferCount; i++) {
    DEBUGPRINT (
      INIT,
      ("Pair %d. Attaching buffer (PA: %lX) to descriptor (VA: %lX)\n", i,
        RECEIVE_BUFFER_PA (RxRing, i), RECEIVE_DESCRIPTOR_VA (RxRing, i))
      );

    ReceiveAttachBufferToDescriptor (
      RECEIVE_DESCRIPTOR_VA (RxRing, i),
      RECEIVE_BUFFER_PA (RxRing, i)
      );
  }

  return EFI_SUCCESS;
}

/* Public receive engine functions */

/**
  Initialize Rx ring structure of LAN engine.
  This function will allocate and initialize all the necessary resources.
  It also assumes (and checks) that RX_RING_FROM_ADAPTER (AdapterInfo)
  is zeroed.

  @param[in]   AdapterInfo        Pointer to the NIC data structure.
  @param[in]   BufferCount        Number of Rx buffers to be allocated within
                                  the ring.
  @param[in]   BufferSize         Target Rx buffer size.

  @retval EFI_SUCCESS             Rx ring initialized.
  @retval EFI_INVALID_PARAMETER   Parameters were NULL/invalid.
  @retval EFI_VOLUME_CORRUPTED    Rx ring structure was not zeroed.
  @retval EFI_OUT_OF_RESOURCES    Could not allocate necessary resources.
  @retval Others                  Underlying function error.

**/
EFI_STATUS
ReceiveInitialize (
  IN DRIVER_DATA  *AdapterInfo,
  IN UINT16       BufferCount,
  IN UINT16       BufferSize
  )
{
  EFI_STATUS    Status;
  RECEIVE_RING  *RxRing;

  DEBUGPRINT (INIT, ("Initializing Rx ring.\n"));

  if (AdapterInfo == NULL
    || PCI_IO_FROM_ADAPTER (AdapterInfo) == NULL)
  {
    DEBUGPRINT (CRITICAL, ("Invalid input parameters.\n"));
    ASSERT (AdapterInfo != NULL);
    ASSERT (PCI_IO_FROM_ADAPTER (AdapterInfo) != NULL);
    return EFI_INVALID_PARAMETER;
  }

  if (BufferCount == 0
    || BufferSize == 0)
  {
    DEBUGPRINT (CRITICAL, ("Invalid input parameters.\n"));
    ASSERT (BufferCount != 0);
    ASSERT (BufferSize != 0);
    return EFI_INVALID_PARAMETER;
  }

  RxRing = RX_RING_FROM_ADAPTER (AdapterInfo);

  if (!IsZeroBuffer (RxRing, sizeof (RECEIVE_RING))) {
    DEBUGPRINT (CRITICAL, ("Invalid initial contents of RECEIVE_RING structure.\n"));
    ASSERT (IsZeroBuffer (RxRing, sizeof (RECEIVE_RING)));
    return EFI_VOLUME_CORRUPTED;
  }

  RxRing->BufferCount  = BufferCount;
  RxRing->BufferSize   = BufferSize;

  // Allocate Rx descriptors
  RxRing->Descriptors.Size = ALIGN (RxRing->BufferCount * sizeof (RECEIVE_DESCRIPTOR), 4096);

  Status = UndiDmaAllocateCommonBuffer (
             PCI_IO_FROM_ADAPTER (AdapterInfo),
             &RxRing->Descriptors
             );

  if (EFI_ERROR (Status)) {
    DEBUGPRINT (CRITICAL, ("Failed to allocate descriptor memory via PciIo: %r\n", Status));
    ASSERT_EFI_ERROR (Status);
    goto Exit;
  }

  DEBUGPRINT (
    INIT,
    ("Allocated Rx descriptors. Count: %d, Total size: %d\n",
      RxRing->BufferCount,
      RxRing->Descriptors.Size)
    );

  // Allocate Rx buffers
  RxRing->Buffers.Size = ALIGN (RxRing->BufferCount * RxRing->BufferSize, 4096);

  Status = UndiDmaAllocateCommonBuffer (
             PCI_IO_FROM_ADAPTER (AdapterInfo),
             &RxRing->Buffers
             );

  if (EFI_ERROR (Status)) {
    DEBUGPRINT (CRITICAL, ("Failed to allocate Rx buffer memory via PciIo: %r\n", Status));
    ASSERT_EFI_ERROR (Status);
    goto ExitFreeDesc;
  }

  // Zero-init buffer and descriptor area
  ZeroMem (
    (VOID*) RxRing->Descriptors.UnmappedAddress,
    RxRing->Descriptors.Size
    );

  ZeroMem (
    (VOID *) RxRing->Buffers.UnmappedAddress,
    RxRing->Buffers.Size
    );

  DEBUGPRINT (
    INIT,
    ("Allocated Rx buffers. Count: %d, Total size: %d\n",
      RxRing->BufferCount,
      RxRing->Buffers.Size)
    );

  // Tie buffers to descriptors
  Status = ReceiveInitializeDescriptors (RxRing);

  if (EFI_ERROR (Status)) {
    DEBUGPRINT (CRITICAL, ("Failed to attach Rx buffers to descriptors: %r\n", Status));
    ASSERT_EFI_ERROR (Status);
    goto ExitFreeBufs;
  }

  DEBUGPRINT (INIT, ("Rx buffers attached to Rx buffers.\n"));

  // Setup device to use Rx queue
  Status = ReceiveConfigureQueue (AdapterInfo);

  if (EFI_ERROR (Status)) {
    DEBUGPRINT (CRITICAL, ("Failed to configure device to use Rx queue: %r\n", Status));
    ASSERT_EFI_ERROR (Status);
    goto ExitFreeBufs;
  }

  // Setup Rx ring fields
  RxRing->NextToUse     = 0;
  RxRing->IsRunning     = FALSE;
  RxRing->Signature     = RECEIVE_RING_SIGNATURE;

  DEBUGPRINT (INIT, ("Rx ring has been successfully initialized.\n"));

  return EFI_SUCCESS;

ExitFreeBufs:
  UndiDmaFreeCommonBuffer (
    PCI_IO_FROM_ADAPTER (AdapterInfo),
    &RxRing->Buffers
    );

ExitFreeDesc:
  UndiDmaFreeCommonBuffer (
    PCI_IO_FROM_ADAPTER (AdapterInfo),
    &RxRing->Descriptors
    );

Exit:
  ZeroMem (RxRing, sizeof (RECEIVE_RING));
  return Status;
}

/**
  Clean up Rx ring structure of LAN engine.
  This function will release all the resources used by Rx ring.

  @param[in]   AdapterInfo        Pointer to the NIC data structure.

  @retval EFI_SUCCESS             Rx ring cleaned up.
  @retval EFI_INVALID_PARAMETER   Parameters were NULL/invalid.
  @retval EFI_VOLUME_CORRUPTED    Rx ring was not initialized.
  @retval EFI_ACCESS_DENIED       Rx ring is still running.
  @retval Others                  Underlying function error.

**/
EFI_STATUS
ReceiveCleanup (
  IN DRIVER_DATA  *AdapterInfo
  )
{
  EFI_STATUS    Status;
  RECEIVE_RING  *RxRing;

  if (AdapterInfo == NULL
    || PCI_IO_FROM_ADAPTER (AdapterInfo) == NULL)
  {
    DEBUGPRINT (CRITICAL, ("Invalid input parameters.\n"));
    ASSERT (AdapterInfo != NULL);
    ASSERT (PCI_IO_FROM_ADAPTER (AdapterInfo) != NULL);
    return EFI_INVALID_PARAMETER;
  }

  RxRing = RX_RING_FROM_ADAPTER (AdapterInfo);

  if (!IS_RX_RING_INITIALIZED (RxRing)) {
    DEBUGPRINT (CRITICAL, ("Rx ring not initialized (or pointer is invalid).\n"));
    ASSERT (IS_RX_RING_INITIALIZED (RxRing));
    return EFI_VOLUME_CORRUPTED;
  }

  if (RxRing->IsRunning) {
    DEBUGPRINT (CRITICAL, ("Rx ring is still running.\n"));
    return EFI_ACCESS_DENIED;
  }

  DEBUGPRINT (INIT, ("Cleaning up receive engine.\n"));

  Status = ReceiveDismantleQueue (AdapterInfo);

  if (EFI_ERROR (Status)) {
    DEBUGPRINT (CRITICAL, ("Failed to dismantle Rx queue: %r\n", Status));
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  // Free Rx descriptor DMA region
  Status = UndiDmaFreeCommonBuffer (
             PCI_IO_FROM_ADAPTER (AdapterInfo),
             &RxRing->Descriptors
             );

  if (EFI_ERROR (Status)) {
    DEBUGPRINT (CRITICAL, ("Failed to deallocate Rx descriptors: %r\n", Status));
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  // Free Rx buffers DMA region
  Status = UndiDmaFreeCommonBuffer (
             PCI_IO_FROM_ADAPTER (AdapterInfo),
             &RxRing->Buffers
             );

  if (EFI_ERROR (Status)) {
    DEBUGPRINT (CRITICAL, ("Failed to deallocate Rx buffers: %r\n", Status));
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  ZeroMem (RxRing, sizeof (RECEIVE_RING));

  DEBUGPRINT (INIT, ("Rx ring resources have been successfully freed\n"));

  return EFI_SUCCESS;
}

/**
  Reset the receive ring. Ring must be stopped first to call this function.

  @param[in]   AdapterInfo        Pointer to the NIC data structure.

  @retval EFI_SUCCESS             Rx ring has been reset.
  @retval EFI_VOLUME_CORRUPTED    Rx ring was not initialized.
  @retval EFI_ACCESS_DENIED       Rx ring is still running.
  @retval EFI_INVALID_PARAMETER   Parameters were NULL/invalid.
  @retval Others                  Underlying function error.

**/
EFI_STATUS
ReceiveReset (
  IN  DRIVER_DATA   *AdapterInfo
  )
{
  EFI_STATUS      Status;
  RECEIVE_RING    *RxRing;

  if (AdapterInfo == NULL) {
    DEBUGPRINT (CRITICAL, ("Invalid input parameters\n"));
    ASSERT (AdapterInfo != NULL);
    return EFI_INVALID_PARAMETER;
  }

  RxRing = RX_RING_FROM_ADAPTER (AdapterInfo);

  if (!IS_RX_RING_INITIALIZED (RxRing)) {
    DEBUGPRINT (CRITICAL, ("Rx ring not initialized (or pointer is invalid).\n"));
    ASSERT (IS_RX_RING_INITIALIZED (RxRing));
    return EFI_VOLUME_CORRUPTED;
  }

  if (RxRing->IsRunning) {
    DEBUGPRINT (CRITICAL, ("Rx ring is still running.\n"));
    return EFI_ACCESS_DENIED;
  }

  // Destroy adapter's queue context
  Status = ReceiveDismantleQueue (AdapterInfo);

  if (EFI_ERROR (Status)) {
    DEBUGPRINT (CRITICAL, ("Failed to dismantle Rx queue.\n"));
    return Status;
  }

  // Reinitialize descriptors
  ZeroMem (
    (VOID*) RxRing->Descriptors.UnmappedAddress,
    RxRing->Descriptors.Size
    );

  ReceiveInitializeDescriptors (RxRing);

  // Clear Rx buffers
  ZeroMem (
    (VOID*) RxRing->Buffers.UnmappedAddress,
    RxRing->Buffers.Size
    );

  // Reconfigure queue
  Status = ReceiveConfigureQueue (AdapterInfo);

  if (EFI_ERROR (Status)) {
    DEBUGPRINT (CRITICAL, ("Failed to reconfigure Rx queue.\n"));
    return Status;
  }

  RxRing->NextToUse = 0;

  return EFI_SUCCESS;
}

/**
  Check whether Rx ring has a packet ready to be obtained.

  @param[in]   AdapterInfo        Pointer to the NIC data structure.
  @param[out]  PacketLength       On output, length of received packet.
  @param[out]  HeaderLength       On output, length of received packet's header.
  @param[out]  RxError            On output, descriptor's RXERROR field content.
  @param[out]  PacketType         On output, descriptor's PTYPE field content.

  @retval EFI_SUCCESS             Packet received and ready to be obtained.
  @retval EFI_NOT_READY           No packet has been received.
  @retval EFI_INVALID_PARAMETER   Parameters were NULL/invalid.
  @retval EFI_VOLUME_CORRUPTED    Rx ring was not initialized.
  @retval EFI_NOT_STARTED         Rx ring was not started.

**/
EFI_STATUS
ReceiveIsPacketReady (
  IN  DRIVER_DATA   *AdapterInfo,
  OUT UINT16        *PacketLength   OPTIONAL,
  OUT UINT16        *HeaderLength   OPTIONAL,
  OUT UINT8         *RxError        OPTIONAL,
  OUT UINT8         *PacketType     OPTIONAL
  )
{
  RECEIVE_RING          *RxRing;
  RECEIVE_DESCRIPTOR    *RxDesc;
  BOOLEAN               GotPacket;

  if (AdapterInfo == NULL) {
    DEBUGPRINT (CRITICAL, ("Invalid input parameters\n"));
    ASSERT (AdapterInfo != NULL);
    return EFI_INVALID_PARAMETER;
  }

  RxRing = RX_RING_FROM_ADAPTER (AdapterInfo);

  if (!IS_RX_RING_INITIALIZED (RxRing)) {
    DEBUGPRINT (CRITICAL, ("Rx ring not initialized (or pointer is invalid).\n"));
    ASSERT (IS_RX_RING_INITIALIZED (RxRing));
    return EFI_VOLUME_CORRUPTED;
  }

  if (!RxRing->IsRunning) {
    DEBUGPRINT (CRITICAL, ("Rx ring is not running.\n"));
    return EFI_NOT_STARTED;
  }

  RxDesc = RECEIVE_DESCRIPTOR_VA (RxRing, RxRing->NextToUse);
  ASSERT (RxDesc != NULL);

  GotPacket = ReceiveIsDescriptorDone (
                RxDesc,
                PacketLength,
                HeaderLength,
                RxError,
                PacketType
                );

  return GotPacket ? EFI_SUCCESS : EFI_NOT_READY;
}

/**
  Try to obtain the packet from Rx ring.
  If no buffer is provided, ring will cycle through one descriptor.
  If provided buffer cannot hold the whole packet, data that could not be
  copied to that buffer will be lost. To identify this case, PacketLength value
  can be compared with BufferSize.

  @param[in]      AdapterInfo     Pointer to the NIC data structure.
  @param[out]     Buffer          Buffer to hold received packet.
  @param[in,out]  BufferSize      On input, length of provided buffer.
                                  On output, number of bytes transferred
                                  from packet to target buffer.
  @param[out]     PacketLength    On output, full length of received packet.

  @retval EFI_SUCCESS             Packet received and ready to be obtained.
  @retval EFI_INVALID_PARAMETER   Parameters were NULL/invalid.
  @retval EFI_VOLUME_CORRUPTED    Rx ring was not initialized.
  @retval EFI_NOT_STARTED         Rx ring was not started.
  @retval EFI_NOT_READY           No packet has been received.
  @retval EFI_DEVICE_ERROR        Error has been reported via Rx descriptor.
  @retval Others                  Underlying function error.

**/
EFI_STATUS
ReceiveGetPacket (
  IN      DRIVER_DATA   *AdapterInfo,
  OUT     UINT8         *Buffer         OPTIONAL,
  IN OUT  UINT16        *BufferSize     OPTIONAL,
  OUT     UINT16        *PacketLength   OPTIONAL
  )
{
  EFI_STATUS          Status;
  RECEIVE_RING        *RxRing;
  UINT16              HeaderLength;
  UINT8               RxError;
  UINT16              LengthToCopy;

  if (AdapterInfo == NULL) {
    DEBUGPRINT (CRITICAL, ("Invalid input parameters.\n"));
    ASSERT (AdapterInfo != NULL);
    return EFI_INVALID_PARAMETER;
  }

  if (Buffer != NULL) {
    if ((PacketLength == NULL)
      || (BufferSize == NULL)
      || (*BufferSize == 0))
    {
      DEBUGPRINT (CRITICAL, ("Invalid input parameters.\n"));
      ASSERT (PacketLength != NULL);
      ASSERT (BufferSize != NULL);
      ASSERT (*BufferSize != 0);
      return EFI_INVALID_PARAMETER;
    }
  }

  RxRing = RX_RING_FROM_ADAPTER (AdapterInfo);

  if (!IS_RX_RING_INITIALIZED (RxRing)) {
    DEBUGPRINT (CRITICAL, ("Rx ring not initialized (or pointer is invalid).\n"));
    ASSERT (IS_RX_RING_INITIALIZED (RxRing));
    return EFI_VOLUME_CORRUPTED;
  }

  if (!RxRing->IsRunning) {
    DEBUGPRINT (CRITICAL, ("Rx ring is not running.\n"));
    return EFI_NOT_STARTED;
  }

  Status = ReceiveIsPacketReady (
             AdapterInfo,
             PacketLength,
             &HeaderLength,
             &RxError,
             NULL
             );

  if (EFI_ERROR (Status)) {
    // Failure or packet not ready
    goto Exit;
  }

  if (RxError != 0) {
    // Receive error occured
    DEBUGPRINT (RX, ("Receive error. RxError = %d\n", RxError));
    Status = EFI_DEVICE_ERROR;
    goto ExitAdvanceDesc;
  }

  if (Buffer == NULL) {
    // Caller is only interested in cycling the queue. Advance.
    goto ExitAdvanceDesc;
  }

  if (*PacketLength < MIN_ETHERNET_PACKET_LENGTH
    || *PacketLength > RxRing->BufferSize)
  {
    // Descriptor done but no/insufficient data or device screw-up
    DEBUGPRINT (RX, ("Descriptor done but no/insufficient data. PacketLenght = %d\n", *PacketLength));
    Status = EFI_DEVICE_ERROR;
    goto ExitAdvanceDesc;
  }

  // Copy packet to provided buffer
  LengthToCopy = MIN (*PacketLength, *BufferSize);

  DEBUGPRINT (
    RX,
    ("Copying packet from buffer %d, VA: %lX, byte count: %d\n",
      RxRing->NextToUse, RECEIVE_BUFFER_VA (RxRing, RxRing->NextToUse),
      LengthToCopy)
    );

  ASSERT (RxRing->NextToUse < RxRing->BufferCount);

  CopyMem (
    Buffer,
    RECEIVE_BUFFER_VA (RxRing, RxRing->NextToUse),
    LengthToCopy
    );

  Status = EFI_SUCCESS;

ExitAdvanceDesc:

  DEBUGPRINT (
    RX,
    ("Attaching buffer %d (PA: %lX) to descriptor %d (VA: %lX)\n",
      RxRing->NextToUse, RECEIVE_BUFFER_PA (RxRing, RxRing->NextToUse),
      RxRing->NextToUse, RECEIVE_DESCRIPTOR_VA (RxRing, RxRing->NextToUse))
    );

  // Rewrite buffer address to Rx descriptor
  ReceiveAttachBufferToDescriptor (
    RECEIVE_DESCRIPTOR_VA (RxRing, RxRing->NextToUse),
    RECEIVE_BUFFER_PA (RxRing, RxRing->NextToUse)
    );

  DEBUGPRINT (RX, ("Advancing Rx tail to %d\n", RxRing->NextToUse));

  ReceiveUpdateTail (AdapterInfo, RxRing->NextToUse);

  if (++RxRing->NextToUse == RxRing->BufferCount) {
    RxRing->NextToUse = 0;
  }

  DEBUGPRINT (RX, ("RxRing->NextToUse = %d\n", RxRing->NextToUse));

Exit:
  return Status;
}

/**
  Start Rx ring.

  @param[in]   AdapterInfo        Pointer to the NIC data structure.

  @retval EFI_SUCCESS             Rx ring started.
  @retval EFI_INVALID_PARAMETER   Parameters were NULL/invalid.
  @retval EFI_VOLUME_CORRUPTED    Rx ring was not initialized.
  @retval EFI_ALREADY_STARTED     Rx ring was already started.
  @retval EFI_DEVICE_ERROR        NIC operation failure.
  @retval Others                  Underlying function failure.

**/
EFI_STATUS
ReceiveStart (
  IN DRIVER_DATA  *AdapterInfo
  )
{
  EFI_STATUS      Status;
  RECEIVE_RING    *RxRing;

  if (AdapterInfo == NULL) {
    DEBUGPRINT (CRITICAL, ("Invalid input parameters.\n"));
    ASSERT (AdapterInfo != NULL);
    return EFI_INVALID_PARAMETER;
  }

  RxRing = RX_RING_FROM_ADAPTER (AdapterInfo);

  if (!IS_RX_RING_INITIALIZED (RxRing)) {
    DEBUGPRINT (CRITICAL, ("Rx ring not initialized (or pointer is invalid).\n"));
    ASSERT (IS_RX_RING_INITIALIZED (RxRing));
    return EFI_VOLUME_CORRUPTED;
  }

  if (RxRing->IsRunning) {
    DEBUGPRINT (CRITICAL, ("Rx ring is already started.\n"));
    return EFI_ALREADY_STARTED;
  }

  Status = ReceiveEnableQueue (AdapterInfo);

  if (Status == EFI_SUCCESS) {
    DEBUGPRINT (RX, ("Rx ring is now running.\n"));
    RxRing->IsRunning = TRUE;
  }

  ASSERT_EFI_ERROR (Status);

  return Status;
}

/**
  Stop Rx ring.

  @param[in]   AdapterInfo        Pointer to the NIC data structure.

  @retval EFI_SUCCESS             Rx ring stopped.
  @retval EFI_INVALID_PARAMETER   Parameters were NULL/invalid.
  @retval EFI_VOLUME_CORRUPTED    Rx ring was not initialized.
  @retval EFI_NOT_STARTED         Rx ring was already stopped.
  @retval EFI_DEVICE_ERROR        NIC operation failure.
  @retval Others                  Underlying function failure.

**/
EFI_STATUS
ReceiveStop (
  IN DRIVER_DATA  *AdapterInfo
  )
{
  EFI_STATUS      Status;
  RECEIVE_RING    *RxRing;

  if (AdapterInfo == NULL) {
    DEBUGPRINT (CRITICAL, ("Invalid input parameters.\n"));
    ASSERT (AdapterInfo != NULL);
    return EFI_INVALID_PARAMETER;
  }

  RxRing = RX_RING_FROM_ADAPTER (AdapterInfo);

  if (!IS_RX_RING_INITIALIZED (RxRing)) {
    DEBUGPRINT (CRITICAL, ("Rx ring not initialized (or pointer is invalid).\n"));
    ASSERT (IS_RX_RING_INITIALIZED (RxRing));
    return EFI_VOLUME_CORRUPTED;
  }

  if (!RxRing->IsRunning) {
    DEBUGPRINT (CRITICAL, ("Rx ring is already stopped.\n"));
    return EFI_NOT_STARTED;
  }

  Status = ReceiveDisableQueue (AdapterInfo);

  if (Status == EFI_SUCCESS) {
    DEBUGPRINT (RX, ("Rx ring is now stopped.\n"));
    RxRing->IsRunning = FALSE;
  }

  ASSERT_EFI_ERROR (Status);

  return Status;
}
