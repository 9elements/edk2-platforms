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
  return BIT_TEST (TxDesc->upper.fields.status, IXGBE_TXD_STAT_DD);
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

  TxDesc->buffer_addr         = Packet;
  TxDesc->lower.data          = (IXGBE_TXD_CMD_EOP |
                                 IXGBE_TXD_CMD_IFCS |
                                 IXGBE_TXD_CMD_RS);
  TxDesc->upper.fields.status = 0;
  TxDesc->lower.flags.length  = PacketLength;
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
  XgbeOutDword (AdapterInfo, IXGBE_TDT (0), Index);
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

  TxDesc->upper.fields.status = 0;
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
  TRANSMIT_RING   *TxRing;
  UINT64          MemAddr;
  UINT32          *MemPtr;

  ASSERT (AdapterInfo != NULL);

  XgbeOutDword (AdapterInfo, IXGBE_TDH (0), 0);
  XgbeOutDword (AdapterInfo, IXGBE_TDT (0), 0);

  TxRing  = TX_RING_FROM_ADAPTER (AdapterInfo);
  MemAddr = TxRing->Descriptors.PhysicalAddress;
  MemPtr  = (UINT32 *) &MemAddr;

  XgbeOutDword (AdapterInfo, IXGBE_TDBAL (0), MemPtr[0]);
  XgbeOutDword (AdapterInfo, IXGBE_TDBAH (0), MemPtr[1]);
  XgbeOutDword (
    AdapterInfo,
    IXGBE_TDLEN (0),
    sizeof (TRANSMIT_DESCRIPTOR) * TxRing->BufferCount
    );

  if (IsNianticBasedDevice (AdapterInfo)) {
    XgbeSetRegBits (AdapterInfo, IXGBE_DMATXCTL, IXGBE_DMATXCTL_TE);
  }

  XgbePciFlush (AdapterInfo);

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
  XgbeSetRegBits (
    AdapterInfo,
    IXGBE_TXDCTL (0),
    IXGBE_TXDCTL_ENABLE | IXGBE_TX_PAD_ENABLE
    );

  if (IsNianticBasedDevice (AdapterInfo)) {
    UINTN   i = 0;
    UINT32  TempReg;

    do {
      gBS->Stall (1);

      TempReg = XgbeInDword (AdapterInfo, IXGBE_TXDCTL (0));
      if (BIT_TEST (TempReg, IXGBE_TXDCTL_ENABLE)) {
        DEBUGPRINT (XGBE, ("TX queue enabled, after attempt i = %d\n", i));
        break;
      }
      i++;
    } while (i < 1000);

    if (i >= 1000) {
      DEBUGPRINT (CRITICAL, ("Tx queue enable timeout.\n"));
      return EFI_DEVICE_ERROR;
    }
  }

  return EFI_SUCCESS;
}

/**
  Disable Tx queue on the NIC.

  @param[in]   AdapterInfo        Pointer to the NIC data structure

  @retval EFI_SUCCESS             Tx queue has been disabled.

**/
EFI_STATUS
TransmitDisableQueue (
  IN DRIVER_DATA *AdapterInfo
  )
{
  XgbeClearRegBits (AdapterInfo, IXGBE_TXDCTL (0), IXGBE_TXDCTL_ENABLE);
  XgbePciFlush (AdapterInfo);
  return EFI_SUCCESS;
}

/**
  Perform actions before transmit ring resources are freed.

  @param[in]   AdapterInfo        Pointer to the NIC data structure

  @retval EFI_SUCCESS             Tx queue has been dismantled.

**/
EFI_STATUS
TransmitDismantleQueue (
  IN  DRIVER_DATA   *AdapterInfo
  )
{
  return EFI_SUCCESS;
}
