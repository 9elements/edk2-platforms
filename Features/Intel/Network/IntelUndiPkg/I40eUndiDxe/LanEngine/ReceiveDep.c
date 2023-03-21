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

  RxDesc->read.pkt_addr = (UINT64) RxBuffer;
  RxDesc->read.hdr_addr = 0;
  RxDesc->wb.qword1.status_error_len = 0;
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
  UINT64    DescQWord;
  UINT32    RxStatus;

  ASSERT (RxDesc != NULL);

  DescQWord = RxDesc->wb.qword1.status_error_len;
  RxStatus  = (UINT32) ((DescQWord & I40E_RXD_QW1_STATUS_MASK) >> I40E_RXD_QW1_STATUS_SHIFT);

  if (!BIT_TEST (RxStatus, 1 << I40E_RX_DESC_STATUS_DD_SHIFT)) {
    return FALSE;
  }

  if (PacketLength != NULL) {
    *PacketLength = (UINT16) ((DescQWord & I40E_RXD_QW1_LENGTH_PBUF_MASK) >> I40E_RXD_QW1_LENGTH_PBUF_SHIFT);
  }
  if (HeaderLength != NULL) {
    *HeaderLength = (UINT16) ((DescQWord & I40E_RXD_QW1_LENGTH_HBUF_MASK) >> I40E_RXD_QW1_LENGTH_HBUF_SHIFT);
  }
  if (RxError != NULL) {
    *RxError = (UINT8) ((DescQWord & I40E_RXD_QW1_ERROR_MASK) >> I40E_RXD_QW1_ERROR_SHIFT);
  }
  if (PacketType != NULL) {
    *PacketType = (UINT8) ((DescQWord & I40E_RXD_QW1_PTYPE_MASK) >> I40E_RXD_QW1_PTYPE_SHIFT);
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
  I40eWrite32 (AdapterInfo, I40E_QRX_TAIL (0), DescId);
}

/**
  Configure NIC to be ready to use initialized Rx queue.

  @param[in]   AdapterInfo        Pointer to the NIC data structure

  @retval EFI_SUCCESS             NIC successfully configured.
  @retval EFI_DEVICE_ERROR        NIC operation failure.

**/
EFI_STATUS
ReceiveConfigureQueue (
  IN DRIVER_DATA *AdapterInfo
  )
{
  RECEIVE_RING              *RxRing;
  struct i40e_hw            *Hw;
  struct i40e_hmc_obj_rxq   RxHmcContext;
  enum i40e_status_code     I40eStatus;

  ASSERT (AdapterInfo != NULL);

  RxRing  = RX_RING_FROM_ADAPTER (AdapterInfo);
  Hw      = &AdapterInfo->Hw;

  // Clear the context structure first
  ZeroMem (&RxHmcContext, sizeof (struct i40e_hmc_obj_rxq));

  RxHmcContext.head         = 0;
  RxHmcContext.cpuid        = 0;
  RxHmcContext.dbuff        = (UINT8) (RxRing->BufferSize >> I40E_RXQ_CTX_DBUFF_SHIFT);
  RxHmcContext.hbuff        = 0;  // No packet split
  RxHmcContext.base         = (UINT64) RxRing->Descriptors.PhysicalAddress / 128;
  RxHmcContext.qlen         = RxRing->BufferCount;
  RxHmcContext.dsize        = 0;   // 16 byte descriptors in use
  RxHmcContext.dtype        = I40E_RX_DTYPE_NO_SPLIT;
  RxHmcContext.hsplit_0     = I40E_HMC_OBJ_RX_HSPLIT_0_NO_SPLIT;
  RxHmcContext.rxmax        = RxRing->BufferSize;
  RxHmcContext.tphrdesc_ena = 0;
  RxHmcContext.tphwdesc_ena = 0;
  RxHmcContext.tphdata_ena  = 0;
  RxHmcContext.tphhead_ena  = 0;
  RxHmcContext.lrxqthresh   = 0;
  RxHmcContext.crcstrip     = 1;
  RxHmcContext.fc_ena       = 0;    // No FCoE

#ifndef DIRECT_QUEUE_CTX_PROGRAMMING

  // Clear the context in the HMC
  I40eStatus = i40e_clear_lan_rx_queue_context (Hw, 0);
  if (I40eStatus != I40E_SUCCESS) {
    DEBUGPRINT (
      CRITICAL,
      ("Failed to clear LAN Rx queue context on Rx ring, error: %d\n",
        I40eStatus)
      );
    return EFI_DEVICE_ERROR;
  }
#endif /* DIRECT_QUEUE_CTX_PROGRAMMING */

  // Set the context in the HMC
#ifdef DIRECT_QUEUE_CTX_PROGRAMMING
  I40eStatus = i40e_set_lan_rx_queue_context_directly (
                 Hw,
                 AdapterInfo->Vsi.BaseQueue,
                 &RxHmcContext
                 );
#else /* NOT DIRECT_QUEUE_CTX_PROGRAMMING */
  I40eStatus = i40e_set_lan_rx_queue_context (Hw, 0, &RxHmcContext);
#endif /* DIRECT_QUEUE_CTX_PROGRAMMING */

  if (I40eStatus != I40E_SUCCESS) {
    DEBUGPRINT (
      CRITICAL,
      ("Failed to set LAN Rx queue context on Rx ring, error: %d\n",
        I40eStatus)
      );
    return EFI_DEVICE_ERROR;
  }

  // Initialize tail register
  ReceiveUpdateTail (AdapterInfo, 0);
  ReceiveUpdateTail (AdapterInfo, RxRing->BufferCount - 1);

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
  UINT32    Reg;
  UINTN     j;

  ASSERT (AdapterInfo != NULL);

  // Enable Rx queue, wait and check if status bits are changed.
  Reg = I40eRead32 (AdapterInfo, I40E_QRX_ENA (0));

  I40eWrite32 (
    AdapterInfo,
    I40E_QRX_ENA (0),
    Reg | I40E_QRX_ENA_QENA_REQ_MASK
    );

  for (j = 0; j < START_RINGS_TIMEOUT; j++) {
    Reg = I40eRead32 (AdapterInfo, I40E_QRX_ENA (0));
    if (BIT_TEST (Reg, I40E_QRX_ENA_QENA_STAT_MASK)) {
      break;
    }
    gBS->Stall (10);
  }
  if (j >= START_RINGS_TIMEOUT) {
    DEBUGPRINT (
      CRITICAL,
      ("Rx ring enable timed out, value %x\n",
        I40eRead32 (AdapterInfo, I40E_QRX_ENA (0)))
      );
    return EFI_DEVICE_ERROR;
  }

  DEBUGPRINT (INIT, ("Rx ring enabled\n"));
  return EFI_SUCCESS;
}

/**
  Disable Rx queue on the NIC.

  @param[in]   AdapterInfo        Pointer to the NIC data structure

  @retval EFI_SUCCESS             Rx queue has been disabled.
  @retval EFI_DEVICE_ERROR        NIC operation failure.

**/
EFI_STATUS
ReceiveDisableQueue (
  IN DRIVER_DATA *AdapterInfo
  )
{
  UINT32        Reg;
  UINTN         j;
  EFI_STATUS    Status;

  ASSERT (AdapterInfo != NULL);

  // For FPK - switch Rx drop policy when stopping Rx rings
  if (AdapterInfo->Hw.mac.type == I40E_MAC_X722) {
    Reg = I40eRead32 (AdapterInfo, I40E_PRTDCB_TC2PFC_RCB);
    I40eWrite32 (AdapterInfo, I40E_PRTDCB_TC2PFC_RCB, 0);
  }

  Reg = I40eRead32 (AdapterInfo, I40E_QRX_ENA (0));
  I40eWrite32 (
    AdapterInfo,
    I40E_QRX_ENA (0),
    Reg & ~I40E_QRX_ENA_QENA_REQ_MASK
    );

  for (j = 0; j < STOP_RINGS_TIMEOUT; j++) {
    Reg = I40eRead32 (AdapterInfo, I40E_QRX_ENA (0));

    if (!BIT_TEST (Reg, I40E_QRX_ENA_QENA_STAT_MASK)) {
      break;
    }
    gBS->Stall (10);
  }

  if (j >= STOP_RINGS_TIMEOUT) {
    DEBUGPRINT (
      CRITICAL,
      ("Rx ring disable timed out, value %x\n",
        I40eRead32 (AdapterInfo, I40E_QRX_ENA (0)))
      );
    Status = EFI_DEVICE_ERROR;
    goto Exit;
  }

  DEBUGPRINT (INIT, ("Rx ring disabled\n"));

  gBS->Stall (50000);

  Status = EFI_SUCCESS;

Exit:
  if (AdapterInfo->Hw.mac.type == I40E_MAC_X722) {
    I40eWrite32 (AdapterInfo, I40E_PRTDCB_TC2PFC_RCB, Reg);
  }
  return Status;
}

/**
  Perform actions before receive ring resources are freed.

  @param[in]   AdapterInfo        Pointer to the NIC data structure

  @retval EFI_SUCCESS             Rx queue has been dismantled.
  @retval EFI_DEVICE_ERROR        NIC operation failure.

**/
EFI_STATUS
ReceiveDismantleQueue (
  IN  DRIVER_DATA   *AdapterInfo
  )
{
#ifndef DIRECT_QUEUE_CTX_PROGRAMMING
  struct i40e_hw            *Hw;
  enum i40e_status_code     I40eStatus;

  ASSERT (AdapterInfo != NULL);

  Hw = &AdapterInfo->Hw;

  // Clear the context in the HMC
  I40eStatus = i40e_clear_lan_rx_queue_context (Hw, 0);
  if (I40eStatus != I40E_SUCCESS) {
    DEBUGPRINT (
      CRITICAL,
      ("Failed to clear LAN Rx queue context on Rx ring, error: %d\n",
        I40eStatus)
      );
    return EFI_DEVICE_ERROR;
  }
#endif /* DIRECT_QUEUE_CTX_PROGRAMMING */
  return EFI_SUCCESS;
}
