/**************************************************************************

Copyright (c) 2020 - 2021, Intel Corporation. All rights reserved.

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
  )
{
  ASSERT (RxDesc != NULL);
  ASSERT (RxBuffer != 0);

  ZeroMem (RxDesc, sizeof (*RxDesc));
  RxDesc->buffer_addr = (UINT64) RxBuffer;
}

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
  )
{
  ASSERT (RxDesc != NULL);

  if (!BIT_TEST (RxDesc->status, IXGBE_RXD_STAT_EOP | IXGBE_RXD_STAT_DD)) {
    return FALSE;
  }

  if (PacketLength != NULL) {
    *PacketLength = RxDesc->length;
  }
  if (HeaderLength != NULL) {
    // No header length in legacy descriptors.
    *HeaderLength = 0;
  }
  if (RxError != NULL) {
    *RxError = RxDesc->errors;
  }
  if (PacketType != NULL) {
    // No packet type in legacy descriptors.
    *PacketType = 0;
  }

  return TRUE;
}

/**
  Update device's Rx tail register with a given descriptor ID

  @param[in]   AdapterInfo        Pointer to the NIC data structure.
  @param[in]   DescId             Descriptor ID to be written.

**/
VOID
ReceiveUpdateTail (
  IN  DRIVER_DATA   *AdapterInfo,
  IN  UINT16        DescId
  )
{
  ASSERT (AdapterInfo != NULL);
  ASSERT (DescId < (RX_RING_FROM_ADAPTER (AdapterInfo))->BufferCount);
  XgbeOutDword (AdapterInfo, IXGBE_RDT (0), DescId);
}

/**
  Configure NIC to be ready to use initialized Rx queue.

  @param[in]   AdapterInfo        Pointer to the NIC data structure

  @retval EFI_SUCCESS             NIC successfully configured.

**/
EFI_STATUS
ReceiveConfigureQueue (
  IN DRIVER_DATA *AdapterInfo
  )
{
  RECEIVE_RING      *RxRing;
  UINT64            MemAddr;
  UINT32            *MemPtr;
  UINT32            SrrCtl;

  ASSERT (AdapterInfo != NULL);

  RxRing = RX_RING_FROM_ADAPTER (AdapterInfo);

  MemAddr = RxRing->Descriptors.PhysicalAddress;
  MemPtr  = (UINT32 *) &MemAddr;

  // Setup the RDBA, RDLEN
  // Write physical address of Rx descriptor buffer for HW to use
  XgbeOutDword (AdapterInfo, IXGBE_RDBAL (0), MemPtr[0]);
  XgbeOutDword (AdapterInfo, IXGBE_RDBAH (0), MemPtr[1]);

  XgbeOutDword (AdapterInfo, IXGBE_RDH (0), 0);

  // We must wait for the receive unit to be enabled before we move
  // the tail descriptor or the hardware gets confused.
  XgbeOutDword (AdapterInfo, IXGBE_RDT (0), 0);

  XgbeOutDword (
    AdapterInfo,
    IXGBE_RDLEN (0),
    sizeof (RECEIVE_DESCRIPTOR) * RxRing->BufferCount
    );

  if (IsNianticBasedDevice (AdapterInfo)) {
    SrrCtl = XgbeInDword (AdapterInfo, IXGBE_SRRCTL (0));
    // Init receive buffer size (BSIZEPACKET field)
    // BSIZEPACKET field has 1kB resolution
    ASSERT (RxRing->BufferSize >= 1024);
    ASSERT (RxRing->BufferSize % 1024 == 0);
    SrrCtl &= ~IXGBE_SRRCTL_BSIZEPKT_MASK;
    SrrCtl |= RxRing->BufferSize / 1024;

    // Setup descriptor type to legacy (bits 27:25 to 0)
    SrrCtl &= ~IXGBE_SRRCTL_DESCTYPE_MASK;
    XgbeOutDword (AdapterInfo, IXGBE_SRRCTL (0), SrrCtl);
  }

  switch (AdapterInfo->Hw.mac.type) {
  case ixgbe_mac_X540:
  case ixgbe_mac_X550:
  case ixgbe_mac_X550EM_x:
  case ixgbe_mac_X550EM_a:
    AdapterInfo->Hw.fc.pause_time = 1;
    AdapterInfo->Hw.fc.requested_mode = ixgbe_fc_none;
    AdapterInfo->Hw.fc.disable_fc_autoneg = TRUE;
    ixgbe_fc_enable (&AdapterInfo->Hw);
    break;

  default:
    break;
  }

  XgbePciFlush (AdapterInfo);
  return EFI_SUCCESS;
}

/**
  Enable Rx queue on the NIC.

  @param[in]   AdapterInfo        Pointer to the NIC data structure

  @retval EFI_SUCCESS             Rx queue has been enabled.
  @retval EFI_DEVICE_ERROR        NIC operation failure.

**/
EFI_STATUS
ReceiveEnableQueue (
  IN DRIVER_DATA    *AdapterInfo
  )
{
  RECEIVE_RING      *RxRing;
  UINT32            TmpReg;

  ASSERT (AdapterInfo != NULL);

  XgbeSetRegBits (AdapterInfo, IXGBE_RXDCTL (0), IXGBE_RXDCTL_ENABLE);

  if (IsNianticBasedDevice (AdapterInfo)) {

#define RXQ_ENABLE_TIMEOUT      1000
#define RXQ_ENABLE_DELAY_STEP   1

    UINTN   i = 0;

    do {
      gBS->Stall (RXQ_ENABLE_DELAY_STEP);
      TmpReg = XgbeInDword (AdapterInfo, IXGBE_RXDCTL (0));
      i++;

      if (BIT_TEST (TmpReg, IXGBE_RXDCTL_ENABLE)) {
        break;
      }

    } while (i < RXQ_ENABLE_TIMEOUT);

    if (i >= RXQ_ENABLE_TIMEOUT) {
      DEBUGPRINT (CRITICAL, ("Rx queue enable timeout.\n"));
      return EFI_DEVICE_ERROR;
    }
  }

  RxRing = RX_RING_FROM_ADAPTER (AdapterInfo);

  // Advance the tail descriptor to tell the hardware it can use the descriptors
  ReceiveUpdateTail (AdapterInfo, RxRing->BufferCount - 1);

  if (IsNianticBasedDevice (AdapterInfo)) {
    XgbeSetRegBits (AdapterInfo, IXGBE_SECRXCTRL, IXGBE_SECRXCTRL_RX_DIS);

    do {
      TmpReg = IXGBE_READ_REG (&AdapterInfo->Hw, IXGBE_SECRXSTAT);
    } while (!BIT_TEST (TmpReg, IXGBE_SECRXSTAT_SECRX_RDY));
  }

  ixgbe_enable_rx (&AdapterInfo->Hw);

  if (IsNianticBasedDevice (AdapterInfo)) {
    XgbeClearRegBits (AdapterInfo, IXGBE_SECRXCTRL, IXGBE_SECRXCTRL_RX_DIS);
  }

  return EFI_SUCCESS;
}

/**
  Disable Rx queue on the NIC.

  @param[in]   AdapterInfo        Pointer to the NIC data structure

  @retval EFI_SUCCESS             Rx queue has been disabled.
  @retval EFI_DEVICE_ERROR        NIC operation failure.
  @retval Others                  Underlying function failure.

**/
EFI_STATUS
ReceiveDisableQueue (
  IN DRIVER_DATA *AdapterInfo
  )
{
  EFI_STATUS      Status;
  RECEIVE_RING    *RxRing;

  ASSERT (AdapterInfo != NULL);

  XgbeClearRegBits (AdapterInfo, IXGBE_RXDCTL (0), IXGBE_RXDCTL_ENABLE);

  if (IsNianticBasedDevice (AdapterInfo)) {
    UINT32  RxdCtl;

    do {
      gBS->Stall (1);
      RxdCtl = XgbeInDword (AdapterInfo, IXGBE_RXDCTL (0));
    } while ((RxdCtl & IXGBE_RXDCTL_ENABLE) != 0);
  }

  ixgbe_disable_rx (&AdapterInfo->Hw);

  RxRing = RX_RING_FROM_ADAPTER (AdapterInfo);

  // Cycle through the remaining packets
  for (UINTN i = 0; i <= RxRing->BufferCount; i++) {
    Status = ReceiveGetPacket (
               AdapterInfo,
               NULL,
               NULL,
               NULL
               );

    if (Status == EFI_NOT_READY) {
      // All descriptors were cycled.
      break;
    }

    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  ASSERT (Status == EFI_NOT_READY);

  // At this point, HEAD == TAIL.
  return EFI_SUCCESS;
}

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
  )
{
  return EFI_SUCCESS;
}
