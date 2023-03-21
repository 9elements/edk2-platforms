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

#ifndef RECEIVE_H_
#define RECEIVE_H_

#include "CommonDriver.h"
#include "Dma.h"

#define RECEIVE_RING_SIGNATURE       0x80865278    /* Intel vendor + 'Rx' */

typedef struct _RECEIVE_RING {
  UINT32              Signature;
  BOOLEAN             IsRunning;
  UINT16              BufferCount;
  UINT16              BufferSize;
  UNDI_DMA_MAPPING    Descriptors;
  UNDI_DMA_MAPPING    Buffers;
  UINT16              NextToUse;
} RECEIVE_RING;

/** Check whether Rx ring structure is in initialized state.

   @param[in]   ring  Rx ring pointer

   @return    TRUE if ring is initialized, FALSE otherwise
 */
#define IS_RX_RING_INITIALIZED(r)   ((r)->Signature == RECEIVE_RING_SIGNATURE)

#define MIN_ETHERNET_PACKET_LENGTH  60

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
  );

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
  );

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
  );

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
  );

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
  );

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
  );

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
  OUT     UINT8         *Buffer,
  IN OUT  UINT16        *BufferSize,
  OUT     UINT16        *PacketLength
  );

#endif /* RECEIVE_H_ */
