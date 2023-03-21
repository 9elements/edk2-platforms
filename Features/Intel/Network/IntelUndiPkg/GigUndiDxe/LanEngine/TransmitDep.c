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
  return BIT_TEST (TxDesc->upper.fields.status, E1000_TXD_STAT_DD);
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
  TxDesc->upper.fields.status = 0;
  TxDesc->lower.flags.length  = PacketLength;


  TxDesc->lower.data |= (E1000_TXD_CMD_EOP |
                         E1000_TXD_CMD_IFCS |
                         E1000_TXD_CMD_RS);
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
  E1000_WRITE_REG (&AdapterInfo->Hw, E1000_TDT (0), Index);
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
  UINT32          TempReg;

  ASSERT (AdapterInfo != NULL);

  // Set the transmit tail equal to the head pointer.
  // Makes the adapter see there is no work to be done.
  TempReg = E1000_READ_REG (&AdapterInfo->Hw, E1000_TDH (0));
  E1000_WRITE_REG (&AdapterInfo->Hw, E1000_TDT (0), TempReg);

  TxRing  = TX_RING_FROM_ADAPTER (AdapterInfo);
  MemAddr = TxRing->Descriptors.PhysicalAddress;
  MemPtr  = (UINT32 *) &MemAddr;

  E1000_WRITE_REG (&AdapterInfo->Hw, E1000_TDBAL (0), MemPtr[0]);
  E1000_WRITE_REG (&AdapterInfo->Hw, E1000_TDBAH (0), MemPtr[1]);
  E1000_WRITE_REG (
    &AdapterInfo->Hw,
    E1000_TDLEN (0),
    sizeof (TRANSMIT_DESCRIPTOR) * TxRing->BufferCount
    );

  E1000PciFlush (&AdapterInfo->Hw);

  return EFI_SUCCESS;
}

/**
  Enable Tx queue on the NIC.

  @param[in]   AdapterInfo        Pointer to the NIC data structure

  @retval EFI_SUCCESS             Tx queue has been enabled.

**/
EFI_STATUS
TransmitEnableQueue (
  IN DRIVER_DATA *AdapterInfo
  )
{
  UINT32    TempReg;
  UINTN     i;

#ifndef NO_82580_SUPPORT
  switch (AdapterInfo->Hw.mac.type) {
  case e1000_82580:
  case e1000_i350:
  case e1000_i354:
  case e1000_i210:
  case e1000_i211:

#define E1000_TXDCTL_ENABLE_TIMEOUT   1000

    E1000SetRegBits (AdapterInfo, E1000_TXDCTL (0), E1000_TXDCTL_QUEUE_ENABLE);

    for (i = 0; i < E1000_TXDCTL_ENABLE_TIMEOUT; i++) {
      TempReg = E1000_READ_REG (&AdapterInfo->Hw, E1000_TXDCTL (0));
      if ((TempReg & E1000_TXDCTL_QUEUE_ENABLE) != 0) {
        DEBUGPRINT (E1000, ("TX queue enabled, after attempt i = %d\n", i));
        break;
      }

      DelayInMicroseconds (AdapterInfo, 1);
    }

    if (i >= E1000_TXDCTL_ENABLE_TIMEOUT) {
      DEBUGPRINT (CRITICAL, ("Enable TX queue failed!\n"));
    }

    break;

  default:
    break;
  }
#endif /* !NO_82580_SUPPORT */

  E1000SetRegBits (AdapterInfo, E1000_TCTL, E1000_TCTL_EN | E1000_TCTL_PSP);
  E1000PciFlush (&AdapterInfo->Hw);

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
  UINTN     i;
  UINT32    TxdCtl;


  switch (AdapterInfo->Hw.mac.type) {
#ifndef NO_82575_SUPPORT
  case e1000_82575:
  case e1000_82576:
#endif /* NO_82575_SUPPORT */
#ifndef NO_82580_SUPPORT
  case e1000_82580:
#endif /* NO_82580_SUPPORT */
  case e1000_i350:
  case e1000_i354:
  case e1000_i210:
  case e1000_i211:

#define MAX_QUEUE_DISABLE_TIME  200

    E1000ClearRegBits (AdapterInfo, E1000_TXDCTL (0), E1000_TXDCTL_QUEUE_ENABLE);
    i = 0;
    do {
      gBS->Stall (1);
      TxdCtl = E1000_READ_REG (&AdapterInfo->Hw, E1000_TXDCTL (0));
    } while ((++i < MAX_QUEUE_DISABLE_TIME)
      && (BIT_TEST (TxdCtl, E1000_TXDCTL_QUEUE_ENABLE)));
    DEBUGPRINT (E1000, ("Tx disabled\n"));
    break;
  default:
    break;
  }

  E1000ClearRegBits (AdapterInfo, E1000_TCTL, E1000_TCTL_EN);
  E1000PciFlush (&AdapterInfo->Hw);
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
