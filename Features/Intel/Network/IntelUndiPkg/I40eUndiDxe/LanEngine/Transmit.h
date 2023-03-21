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

#ifndef TRANSMIT_H_
#define TRANSMIT_H_

#include "CommonDriver.h"
#include "Dma.h"

#define TRANSMIT_RING_SIGNATURE       0x80865478    /* Intel vendor + 'Tx' */

typedef enum _TRANSMIT_BUFFER_STATE {
  TRANSMIT_BUFFER_STATE_FREE = 0,
  TRANSMIT_BUFFER_STATE_IN_QUEUE,
  TRANSMIT_BUFFER_STATE_UNMAPPED
} TRANSMIT_BUFFER_STATE;

typedef struct _TRANSMIT_BUFFER_ENTRY {
  TRANSMIT_BUFFER_STATE   State;
  UNDI_DMA_MAPPING        Mapping;
} TRANSMIT_BUFFER_ENTRY;

typedef struct _TRANSMIT_RING {
  UINT32                Signature;
  BOOLEAN               IsRunning;
  UINT16                BufferCount;
  UNDI_DMA_MAPPING      Descriptors;
  TRANSMIT_BUFFER_ENTRY *BufferEntries;
  UINT16                NextToUse;
  UINT16                NextToUnmap;
  UINT16                NextToFree;
} TRANSMIT_RING;

/** Check whether Tx ring structure is in initialized state.

   @param[in]   ring  Tx ring pointer

   @return    TRUE if ring is initialized, FALSE otherwise
 */
#define IS_TX_RING_INITIALIZED(r)   ((r)->Signature == TRANSMIT_RING_SIGNATURE)

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
  );

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
  );

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
  );

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
  );

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
  );

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
  );

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
  );

/**
  Retrieve Tx buffer for which Tx operations were completed, from Tx ring.

  @param[in]   AdapterInfo        Pointer to the NIC data structure.
  @param[out]  Buffer             Pointer to memory to hold Tx buffer address

  @retval EFI_SUCCESS             Packet has been retrieved.
  @retval EFI_INVALID_PARAMETER   Parameters were NULL/invalid.
  @retval EFI_NOT_READY           No packets to retrieve.

**/
EFI_STATUS
TransmitReleaseBuffer (
  IN    DRIVER_DATA           *AdapterInfo,
  OUT   EFI_VIRTUAL_ADDRESS   *Buffer
  );

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
  );

#endif /* TRANSMIT_H_ */
