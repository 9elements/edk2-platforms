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
  Check whether adapter has finished processing specific Tx descriptor.

  @param[in]   TxDesc             Pointer to Tx descriptor

  @retval      TRUE               Descriptor has been processed.
  @retval      FALSE              Descriptor has not been processed.

**/
BOOLEAN
TransmitIsDescriptorDone (
  IN  TRANSMIT_DESCRIPTOR    *TxDesc
  )
{
  ASSERT (TxDesc != NULL);
  return BIT_TEST (TxDesc->cmd_type_offset_bsz, I40E_TX_DESC_DTYPE_DESC_DONE);
}

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
  IN  DRIVER_DATA            *AdapterInfo,
  IN  TRANSMIT_DESCRIPTOR    *TxDesc,
  IN  EFI_PHYSICAL_ADDRESS   Packet,
  IN  UINT16                 PacketLength
  )
{
  ASSERT (AdapterInfo != NULL);
  ASSERT (TxDesc != NULL);
  ASSERT (Packet != 0);
  ASSERT (PacketLength != 0);

  TxDesc->buffer_addr = Packet;

  TxDesc->cmd_type_offset_bsz = I40E_TX_DESC_DTYPE_DATA
                                | ((UINT64) PacketLength << I40E_TXD_QW1_TX_BUF_SZ_SHIFT);

#define I40E_TXD_CMD (I40E_TX_DESC_CMD_EOP | I40E_TX_DESC_CMD_RS | I40E_TX_DESC_CMD_ICRC)

  TxDesc->cmd_type_offset_bsz |= (UINT64) I40E_TXD_CMD << I40E_TXD_QW1_CMD_SHIFT;
}

/**
  Update Tx ring tail register with Tx descriptor index.

  @param[in]   AdapterInfo        Pointer to the NIC data structure
  @param[in]   Index              Tx descriptor index.

**/
VOID
TransmitUpdateRingTail (
  IN  DRIVER_DATA   *AdapterInfo,
  IN  UINT8         Index
  )
{
  ASSERT (AdapterInfo != NULL);

  TransmitLockIo (AdapterInfo, TRUE);
  I40eWrite32 (AdapterInfo, I40E_QTX_TAIL (0), Index);
  TransmitLockIo (AdapterInfo, FALSE);
}

/**
  Reset Tx descriptor to a valid (unused) state.

  @param[in]   TxDesc             Pointer to Tx descriptor.

**/
VOID
TransmitResetDescriptor (
  IN  TRANSMIT_DESCRIPTOR    *TxDesc
  )
{
  ASSERT (TxDesc != NULL);

  TxDesc->cmd_type_offset_bsz &= ~((UINT64) I40E_TXD_QW1_DTYPE_MASK);
}

/**
  Configure NIC to be ready to use initialized Tx queue.

  @param[in]   AdapterInfo        Pointer to the NIC data structure

  @retval EFI_SUCCESS             NIC successfully configured.
  @retval EFI_DEVICE_ERROR        NIC operation failure.

**/
EFI_STATUS
TransmitConfigureQueue (
  IN  DRIVER_DATA   *AdapterInfo
  )
{
  struct i40e_hmc_obj_txq   TxHmcContext;
  TRANSMIT_RING             *TxRing;
  struct i40e_hw            *Hw;
  UINT32                    QTxCtrl;
  enum i40e_status_code     I40eStatus;

  ASSERT (AdapterInfo != NULL);

  TxRing  = TX_RING_FROM_ADAPTER (AdapterInfo);
  Hw      = &AdapterInfo->Hw;

  // Initialize ring tail
  TransmitUpdateRingTail (AdapterInfo, (UINT8) TxRing->NextToUse);

  // Now associate the queue with the PCI function
  QTxCtrl = I40E_QTX_CTL_PF_QUEUE;
  QTxCtrl |= ((Hw->pf_id << I40E_QTX_CTL_PF_INDX_SHIFT) & I40E_QTX_CTL_PF_INDX_MASK);
  I40eWrite32 (AdapterInfo, I40E_QTX_CTL (0), QTxCtrl);

  ZeroMem (&TxHmcContext, sizeof (TxHmcContext));

  TxHmcContext.new_context  = 1;
  TxHmcContext.base         = (UINT64) TxRing->Descriptors.PhysicalAddress / 128;
  TxHmcContext.qlen         = TxRing->BufferCount;
  TxHmcContext.fc_ena       = 0;    // Disable FCoE
  TxHmcContext.timesync_ena = 0;
  TxHmcContext.fd_ena       = 0;
  TxHmcContext.alt_vlan_ena = 0;

  // By default all traffic is assigned to TC0
  TxHmcContext.rdylist      = AdapterInfo->Vsi.Info.qs_handle[0];
  TxHmcContext.rdylist_act  = 0;

#ifndef DIRECT_QUEUE_CTX_PROGRAMMING

  // Clear the context in the HMC
  I40eStatus = i40e_clear_lan_tx_queue_context (Hw, 0);
  if (I40eStatus != I40E_SUCCESS) {
    DEBUGPRINT (
      CRITICAL,
      ("Failed to clear LAN Tx queue context on Tx ring, error: %d\n",
        I40eStatus)
      );
    return EFI_DEVICE_ERROR;
  }
#endif /* DIRECT_QUEUE_CTX_PROGRAMMING */

  // Set the context in the HMC
#ifdef DIRECT_QUEUE_CTX_PROGRAMMING
  I40eStatus = i40e_set_lan_tx_queue_context_directly (
                 Hw,
                 AdapterInfo->Vsi.BaseQueue,
                 &TxHmcContext
                 );
#else /* NOT DIRECT_QUEUE_CTX_PROGRAMMING */
  I40eStatus = i40e_set_lan_tx_queue_context (Hw, 0, &TxHmcContext);
#endif /* DIRECT_QUEUE_CTX_PROGRAMMING */
  if (I40eStatus != I40E_SUCCESS) {
    DEBUGPRINT (
      CRITICAL,
      ("Failed to set LAN Tx queue context on Tx ring, error: %d\n",
        I40eStatus)
      );
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}

/**
  Enable Tx queue on the NIC.

  @param[in]   AdapterInfo        Pointer to the NIC data structure

  @retval EFI_SUCCESS             Tx queue has been enabled.
  @retval EFI_DEVICE_ERROR        NIC operation failure.

**/
EFI_STATUS
TransmitEnableQueue (
  IN DRIVER_DATA *AdapterInfo
  )
{
  struct i40e_hw    *Hw;
  UINTN             Reg;
  UINTN             j;

  ASSERT (AdapterInfo != NULL);

  Hw = &AdapterInfo->Hw;

  // Tell HW that we intend to enable the Tx queue
  i40e_pre_tx_queue_cfg (Hw, 0, TRUE);

  // Enable Tx queue, wait and check if status bits are changed.
  Reg = I40eRead32 (AdapterInfo, I40E_QTX_ENA (0));
  I40eWrite32 (
    AdapterInfo,
    I40E_QTX_ENA (0),
    Reg | I40E_QTX_ENA_QENA_REQ_MASK
    );

  for (j = 0; j < START_RINGS_TIMEOUT; j++) {
    Reg = I40eRead32 (AdapterInfo, I40E_QTX_ENA (0));

    if (BIT_TEST (Reg, I40E_QTX_ENA_QENA_STAT_MASK)) {
      break;
    }
    gBS->Stall (10);
  }
  if (j >= START_RINGS_TIMEOUT) {
    DEBUGPRINT (
      CRITICAL,
      ("Tx ring enable timed out, value %x\n",
        I40eRead32 (AdapterInfo, I40E_QTX_ENA (0)))
      );
    return EFI_DEVICE_ERROR;
  }

  DEBUGPRINT (INIT, ("Tx ring enabled\n"));
  return EFI_SUCCESS;
}

/**
  Disable Tx queue on the NIC.

  @param[in]   AdapterInfo        Pointer to the NIC data structure

  @retval EFI_SUCCESS             Tx queue has been disabled.
  @retval EFI_DEVICE_ERROR        NIC operation failure.

**/
EFI_STATUS
TransmitDisableQueue (
  IN DRIVER_DATA *AdapterInfo
  )
{
  struct i40e_hw  *Hw;
  UINT32          Reg;
  UINTN           j;

  ASSERT (AdapterInfo != NULL);

  Hw = &AdapterInfo->Hw;

  // Tell HW that we intend to disable the Tx queue
  i40e_pre_tx_queue_cfg (Hw, 0, FALSE);

  Reg = I40eRead32 (AdapterInfo, I40E_QTX_ENA (0));
  I40eWrite32 (
    AdapterInfo,
    I40E_QTX_ENA (0),
    Reg & ~I40E_QTX_ENA_QENA_REQ_MASK
    );

  for (j = 0; j < STOP_RINGS_TIMEOUT; j++) {
    Reg = I40eRead32 (AdapterInfo, I40E_QTX_ENA (0));

    if (!BIT_TEST (Reg, I40E_QTX_ENA_QENA_STAT_MASK)) {
      break;
    }
    gBS->Stall (10);
  }
  if (j >= STOP_RINGS_TIMEOUT) {
    DEBUGPRINT (
      CRITICAL,
      ("Tx ring disable timed out, value %x\n",
        I40eRead32 (AdapterInfo, I40E_QTX_ENA (0)))
      );
    return EFI_DEVICE_ERROR;
  }
  DEBUGPRINT (INIT, ("Tx ring disabled\n"));

  return EFI_SUCCESS;
}

/**
  Perform actions before transmit ring resources are freed.

  @param[in]   AdapterInfo        Pointer to the NIC data structure

  @retval EFI_SUCCESS             Tx queue has been dismantled.
  @retval EFI_DEVICE_ERROR        NIC operation failure.

**/
EFI_STATUS
TransmitDismantleQueue (
  IN  DRIVER_DATA   *AdapterInfo
  )
{
#ifndef DIRECT_QUEUE_CTX_PROGRAMMING
  struct i40e_hw            *Hw;
  enum i40e_status_code     I40eStatus;

  ASSERT (AdapterInfo != NULL);

  Hw = &AdapterInfo->Hw;

  // Clear the context in the HMC
  I40eStatus = i40e_clear_lan_tx_queue_context (Hw, 0);
  if (I40eStatus != I40E_SUCCESS) {
    DEBUGPRINT (
      CRITICAL,
      ("Failed to clear LAN Tx queue context on Tx ring, error: %d\n",
        I40eStatus)
      );
    return EFI_DEVICE_ERROR;
  }
#endif /* DIRECT_QUEUE_CTX_PROGRAMMING */
  return EFI_SUCCESS;
}
