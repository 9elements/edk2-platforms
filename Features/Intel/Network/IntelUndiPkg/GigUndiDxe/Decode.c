/**************************************************************************

Copyright (c) 2012 - 2021, Intel Corporation. All rights reserved.

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
#include "E1000.h"
#include <Uefi/UefiPxe.h>

/** This routine determines the operational state of the UNDI.  It updates the state flags in the
   Command Descriptor Block based on information derived from the AdapterInfo instance data.

   To ensure the command has completed successfully, CdbPtr->StatCode will contain the result of
   the command execution. The CdbPtr->StatFlags will contain a STOPPED, STARTED, or INITIALIZED
   state once the command has successfully completed. Keep in mind the AdapterInfo->State is the
   active state of the adapter (based on software interrogation), and the CdbPtr->StateFlags is
   the passed back information that is reflected to the caller of the UNDI API.

   @param[in]   CdbPtr       Pointer to the command descriptor block.
   @param[in]   AdapterInfo  Pointer to the NIC data structure information which the
                             UNDI driver is layering on..

   @retval      None
**/
VOID
E1000UndiGetState (
  IN PXE_CDB     *CdbPtr,
  IN DRIVER_DATA *AdapterInfo
  );

/** This routine is used to change the operational state of the 1-Gigabit UNDI
   from stopped to started.

   It will do this as long as the adapter's state is PXE_STATFLAGS_GET_STATE_STOPPED, otherwise
   the CdbPtr->StatFlags will reflect a command failure, and the CdbPtr->StatCode will reflect the
   UNDI as having already been started.
   This routine is modified to reflect the UNDI 1.1 specification changes. The
   changes in the spec. are mainly in the callback routines, the new spec. adds
   3 more callbacks and a unique id. Since this UNDI supports both old and new UNDI specifications,
   The NIC's data structure is filled in with the callback routines (depending
   on the version) pointed to in the caller's CpbPtr.  This seeds the Delay,
   Virt2Phys, Block, and Mem_IO for old and new versions and Map_Mem, UnMap_Mem
   and Sync_Mem routines and a unique id variable for the new version.
   This is the function which an external entity (SNP, O/S, etc) would call
   to provide it's I/O abstraction to the UNDI.
   It's final action is to change the AdapterInfo->State to PXE_STATFLAGS_GET_STATE_STARTED.

   @param[in]   CdbPtr        Pointer to the command descriptor block.
   @param[in]   AdapterInfo   Pointer to the NIC data structure information which the
                              UNDI driver is layering on..

   @retval      None
**/
VOID
E1000UndiStart (
  IN PXE_CDB     *CdbPtr,
  IN DRIVER_DATA *AdapterInfo
  );

/** This routine is used to change the operational state of the UNDI from started to stopped.

   It will not do this if the adapter's state is PXE_STATFLAGS_GET_STATE_INITIALIZED, otherwise
   the CdbPtr->StatFlags will reflect a command failure, and the CdbPtr->StatCode will reflect the
   UNDI as having already not been shut down.
   The NIC's data structure will have the Delay, Virt2Phys, and Block, pointers zero'd out..
   It's final action is to change the AdapterInfo->State to PXE_STATFLAGS_GET_STATE_STOPPED.

   @param[in]   CdbPtr        Pointer to the command descriptor block.
   @param[in]   AdapterInfo   Pointer to the NIC data structure information which the
                              UNDI driver is layering on..

   @retval      None
**/
VOID
E1000UndiStop (
  IN PXE_CDB     *CdbPtr,
  IN DRIVER_DATA *AdapterInfo
  );

/** This routine is used to retrieve the initialization information that is
   needed by drivers and applications to initialize the UNDI.

   This will fill in data in the Data Block structure that is pointed to by the
   caller's CdbPtr->DBaddr.  The fields filled in are as follows:
   MemoryRequired, FrameDataLen, LinkSpeeds[0-3], NvCount, NvWidth, MediaHeaderLen, HWaddrLen,
   MCastFilterCnt, TxBufCnt, TxBufSize, RxBufCnt, RxBufSize, IFtype, Duplex, and LoopBack.
   In addition, the CdbPtr->StatFlags ORs in that this NIC supports cable detection.  (APRIORI knowledge)

   @param[in]   CdbPtr        Pointer to the command descriptor block.
   @param[in]   AdapterInfo   Pointer to the NIC data structure information which the
                              UNDI driver is layering on..

   @retval      None
**/
VOID
E1000UndiGetInitInfo (
  IN PXE_CDB     *CdbPtr,
  IN DRIVER_DATA *AdapterInfo
  );

/** This routine is used to retrieve the configuration information about the NIC being controlled by
  this driver.

  This will fill in data in the Data Block structure that is pointed to by the caller's CdbPtr->DBaddr.
  The fields filled in are as follows:
  DbPtr->pci.BusType, DbPtr->pci.Bus, DbPtr->pci.Device, and DbPtr->pci.
  In addition, the DbPtr->pci.Config.Dword[0-63] grabs a copy of this NIC's PCI configuration space.

  @param[in]   CdbPtr        Pointer to the command descriptor block.
  @param[in]   AdapterInfo   Pointer to the NIC data structure information which the
                             UNDI driver is layering on..

  @retval      None
**/
VOID
E1000UndiGetConfigInfo (
  IN PXE_CDB     *CdbPtr,
  IN DRIVER_DATA *AdapterInfo
  );

/** This routine resets the network adapter and initializes the 1-Gigabit UNDI using the parameters
   supplied in the CPB.

   This command must be issued before the network adapter can be setup to transmit and receive packets.
   Once the memory requirements of the UNDI are obtained by using the GetInitInfo command, a block
   of non-swappable memory may need to be allocated.  The address of this memory must be passed to
   UNDI during the Initialize in the CPB.  This memory is used primarily for transmit and receive buffers.
   The fields CableDetect, LinkSpeed, Duplex, LoopBack, MemoryPtr, and MemoryLength are set with
   information that was passed in the CPB and the NIC is initialized.
   If the NIC initialization fails, the CdbPtr->StatFlags are updated with PXE_STATFLAGS_COMMAND_FAILED
   Otherwise, AdapterInfo->State is updated with PXE_STATFLAGS_GET_STATE_INITIALIZED showing the state of
   the UNDI is now initialized.

   @param[in]   CdbPtr        Pointer to the command descriptor block.
   @param[in]   AdapterInfo   Pointer to the NIC data structure information which the
                              UNDI driver is layering on..

   @retval      None
-**/
VOID
E1000UndiInitialize (
  IN PXE_CDB     *CdbPtr,
  IN DRIVER_DATA *AdapterInfo
  );

/** This routine resets the network adapter and initializes the 1-Gigabit UNDI using the
   parameters supplied in the CPB.

   The transmit and receive queues are emptied and any pending interrupts are cleared.
   If the NIC reset fails, the CdbPtr->StatFlags are updated with PXE_STATFLAGS_COMMAND_FAILED

   @param[in]   CdbPtr         Pointer to the command descriptor block.
   @param[in]   AdapterInfo    Pointer to the NIC data structure information which the
                               UNDI driver is layering on..

   @retval      None
**/
VOID
E1000UndiReset (
  IN PXE_CDB     *CdbPtr,
  IN DRIVER_DATA *AdapterInfo
  );

/** This routine resets the network adapter and leaves it in a safe state for another
   driver to initialize.

   Any pending transmits or receives are lost.  Receive filters and external
   interrupt enables are disabled.  Once the UNDI has been shutdown, it can then be stopped
   or initialized again.
   If the NIC reset fails, the CdbPtr->StatFlags are updated with PXE_STATFLAGS_COMMAND_FAILED
   Otherwise, AdapterInfo->State is updated with PXE_STATFLAGS_GET_STATE_STARTED showing
   the state of the NIC as being started.

   @param[in]   CdbPtr        Pointer to the command descriptor block.
   @param[in]   AdapterInfo   Pointer to the NIC data structure information which the
                              UNDI driver is layering on..

   @retval      None
**/
VOID
E1000UndiShutdown (
  IN PXE_CDB     *CdbPtr,
  IN DRIVER_DATA *AdapterInfo
  );

/** This routine can be used to read and/or change the current external interrupt enable
   settings.

   Disabling an external interrupt enable prevents and external (hardware)
   interrupt from being signalled by the network device.  Internally the interrupt events
   can still be polled by using the UNDI_GetState command.
   The resulting information on the interrupt state will be passed back in the CdbPtr->StatFlags.

   @param[in]   CdbPtr        Pointer to the command descriptor block.
   @param[in]   AdapterInfo   Pointer to the NIC data structure information which the
                              UNDI driver is layering on.

   @retval      None
**/
VOID
E1000UndiInterrupt (
  IN PXE_CDB     *CdbPtr,
  IN DRIVER_DATA *AdapterInfo
  );

/** This routine is used to read and change receive filters and, if supported, read
   and change multicast MAC address filter list.

   @param[in]   CdbPtr        Pointer to the command descriptor block.
   @param[in]   AdapterInfo   Pointer to the NIC data structure information which the
                              UNDI driver is layering on..

   @retval     None
**/
VOID
E1000UndiRecFilter (
  IN PXE_CDB     *CdbPtr,
  IN DRIVER_DATA *AdapterInfo
  );

/** This routine is used to get the current station and broadcast MAC addresses,
   and to change the current station MAC address.

   @param[in]   CdbPtr        Pointer to the command descriptor block.
   @param[in]   AdapterInfo   Pointer to the NIC data structure information which the
                              UNDI driver is layering on.

   @retval      None
**/
VOID
E1000UndiStnAddr (
  IN PXE_CDB     *CdbPtr,
  IN DRIVER_DATA *AdapterInfo
  );

/** This routine is used to read and clear the NIC traffic statistics.  This command is supported
   only if the !PXE structure's Implementation flags say so.

   Results will be parsed out in the following manner:
   CdbPtr->DBaddr.Data[0]   R  Total Frames (Including frames with errors and dropped frames)
   CdbPtr->DBaddr.Data[1]   R  Good Frames (All frames copied into receive buffer)
   CdbPtr->DBaddr.Data[2]   R  Undersize Frames (Frames below minimum length for media <64 for ethernet)
   CdbPtr->DBaddr.Data[4]   R  Dropped Frames (Frames that were dropped because receive buffers were full)
   CdbPtr->DBaddr.Data[8]   R  CRC Error Frames (Frames with alignment or CRC errors)
   CdbPtr->DBaddr.Data[A]   T  Total Frames (Including frames with errors and dropped frames)
   CdbPtr->DBaddr.Data[B]   T  Good Frames (All frames copied into transmit buffer)
   CdbPtr->DBaddr.Data[C]   T  Undersize Frames (Frames below minimum length for media <64 for ethernet)
   CdbPtr->DBaddr.Data[E]   T  Dropped Frames (Frames that were dropped because of collisions)
   CdbPtr->DBaddr.Data[14]  T  Total Collision Frames (Total collisions on this subnet)

   @param[in]   CdbPtr        Pointer to the command descriptor block.
   @param[in]   AdapterInfo   Pointer to the NIC data structure information which the
                              UNDI driver is layering on..

   @retval      None
**/
VOID
E1000UndiStatistics (
  IN PXE_CDB     *CdbPtr,
  IN DRIVER_DATA *AdapterInfo
  );

/** This routine is used to translate a multicast IP address to a multicast MAC address.

   This results in a MAC address composed of 25 bits of fixed data with the upper 23 bits of the IP
   address being appended to it.  Results passed back in the equivalent of CdbPtr->DBaddr->MAC[0-5].

   @param[in]   CdbPtr        Pointer to the command descriptor block.
   @param[in]   AdapterInfo   Pointer to the NIC data structure information which the
                              UNDI driver is layering on..

   @retval      None
**/
VOID
E1000UndiIp2Mac (
  IN PXE_CDB     *CdbPtr,
  IN DRIVER_DATA *AdapterInfo
  );

/** This routine is used to read and write non-volatile storage on the NIC (if supported).  The NVRAM
   could be EEPROM, FLASH, or battery backed RAM.

   This is an optional function according to the UNDI specification  (or will be......)

   @param[in]   CdbPtr        Pointer to the command descriptor block.
   @param[in]   AdapterInfo   Pointer to the NIC data structure information which the
                              UNDI driver is layering on..

   @retval      None
**/
VOID
E1000UndiNvData (
  IN PXE_CDB     *CdbPtr,
  IN DRIVER_DATA *AdapterInfo
  );

/** This routine returns the current interrupt status and/or the transmitted buffer addresses.

   If the current interrupt status is returned, pending interrupts will be acknowledged by this
   command.  Transmitted buffer addresses that are written to the DB are removed from the transmit
   buffer queue.
   Normally, this command would be polled with interrupts disabled.
   The transmit buffers are returned in CdbPtr->DBaddr->TxBufer[0 - NumEntries].
   The interrupt status is returned in CdbPtr->StatFlags.

   @param[in]   CdbPtr        Pointer to the command descriptor block.
   @param[in]   AdapterInfo   Pointer to the NIC data structure information which the 1-Gigabit
                              UNDI driver is layering on..

   @retval   None
**/
VOID
E1000UndiStatus (
  IN PXE_CDB     *CdbPtr,
  IN DRIVER_DATA *AdapterInfo
  );

/** This routine is used to fill media header(s) in transmit packet(s).

   Copies the MAC address into the media header whether it is dealing
   with fragmented or non-fragmented packets.

   @param[in]   CdbPtr        Pointer to the command descriptor block.
   @param[in]   AdapterInfo   Pointer to the NIC data structure information which the
                              UNDI driver is layering on.

   @retval      None
**/
VOID
E1000UndiFillHeader (
  IN PXE_CDB     *CdbPtr,
  IN DRIVER_DATA *AdapterInfo
  );

/** This routine is used to place a packet into the transmit queue.

   The data buffers given to this command are to be considered locked and the application or
   network driver loses ownership of these buffers and must not free or relocate them until
   the ownership returns.
   When the packets are transmitted, a transmit complete interrupt is generated (if interrupts
   are disabled, the transmit interrupt status is still set and can be checked using the UNDI_Status
   command.
   Some implementations and adapters support transmitting multiple packets with one transmit
   command.  If this feature is supported, the transmit CPBs can be linked in one transmit
   command.
   All UNDIs support fragmented frames, now all network devices or protocols do.  If a fragmented
   frame CPB is given to UNDI and the network device does not support fragmented frames
   (see !PXE.Implementation flag), the UNDI will have to copy the fragments into a local buffer
   before transmitting.

   @param[in]   CdbPtr        Pointer to the command descriptor block.
   @param[in]   AdapterInfo   Pointer to the NIC data structure information which the
                              UNDI driver is layering on..

   @retval      None
**/
VOID
E1000UndiTransmit (
  IN PXE_CDB     *CdbPtr,
  IN DRIVER_DATA *AdapterInfo
  );

/** When the network adapter has received a frame, this command is used to copy the frame
   into the driver/application storage location.

   Once a frame has been copied, it is removed from the receive queue.

   @param[in]   CdbPtr        Pointer to the command descriptor block.
   @param[in]   AdapterInfo   Pointer to the NIC data structure information which the
                              UNDI driver is layering on..

   @retval      None
**/
VOID
E1000UndiReceive (
  IN PXE_CDB     *CdbPtr,
  IN DRIVER_DATA *AdapterInfo
  );

// Global variables defined in this file
UNDI_CALL_TABLE mE1000ApiTable[PXE_OPCODE_LAST_VALID + 1] = {
  {
    PXE_CPBSIZE_NOT_USED,
    PXE_DBSIZE_NOT_USED,
    0,
    (UINT16) (ANY_STATE),
    E1000UndiGetState
  },
  {
    (UINT16) (DONT_CHECK),
    PXE_DBSIZE_NOT_USED,
    0,
    (UINT16) (ANY_STATE),
    E1000UndiStart
  },
  {
    PXE_CPBSIZE_NOT_USED,
    PXE_DBSIZE_NOT_USED,
    0,
    MUST_BE_STARTED,
    E1000UndiStop
  },
  {
    PXE_CPBSIZE_NOT_USED,
    sizeof (PXE_DB_GET_INIT_INFO),
    0,
    MUST_BE_STARTED,
    E1000UndiGetInitInfo
  },
  {
    PXE_CPBSIZE_NOT_USED,
    sizeof (PXE_DB_GET_CONFIG_INFO),
    0,
    MUST_BE_STARTED,
    E1000UndiGetConfigInfo
  },
  {
    sizeof (PXE_CPB_INITIALIZE),
    (UINT16) (DONT_CHECK),
    (UINT16) (DONT_CHECK),
    MUST_BE_STARTED,
    E1000UndiInitialize
  },
  {
    PXE_CPBSIZE_NOT_USED,
    PXE_DBSIZE_NOT_USED,
    (UINT16) (DONT_CHECK),
    MUST_BE_INITIALIZED,
    E1000UndiReset
  },
  {
    PXE_CPBSIZE_NOT_USED,
    PXE_DBSIZE_NOT_USED,
    0,
    MUST_BE_INITIALIZED,
    E1000UndiShutdown
  },
  {
    PXE_CPBSIZE_NOT_USED,
    PXE_DBSIZE_NOT_USED,
    (UINT16) (DONT_CHECK),
    MUST_BE_INITIALIZED,
    E1000UndiInterrupt
  },
  {
    (UINT16) (DONT_CHECK),
    (UINT16) (DONT_CHECK),
    (UINT16) (DONT_CHECK),
    MUST_BE_INITIALIZED,
    E1000UndiRecFilter
  },
  {
    (UINT16) (DONT_CHECK),
    (UINT16) (DONT_CHECK),
    (UINT16) (DONT_CHECK),
    MUST_BE_INITIALIZED,
    E1000UndiStnAddr
  },
  {
    PXE_CPBSIZE_NOT_USED,
    (UINT16) (DONT_CHECK),
    (UINT16) (DONT_CHECK),
    MUST_BE_INITIALIZED,
    E1000UndiStatistics
  },
  {
    sizeof (PXE_CPB_MCAST_IP_TO_MAC),
    sizeof (PXE_DB_MCAST_IP_TO_MAC),
    (UINT16) (DONT_CHECK),
    MUST_BE_INITIALIZED,
    E1000UndiIp2Mac
  },
  {
    (UINT16) (DONT_CHECK),
    (UINT16) (DONT_CHECK),
    (UINT16) (DONT_CHECK),
    MUST_BE_INITIALIZED,
    E1000UndiNvData
  },
  {
    PXE_CPBSIZE_NOT_USED,
    (UINT16) (DONT_CHECK),
    (UINT16) (DONT_CHECK),
    MUST_BE_INITIALIZED,
    E1000UndiStatus
  },
  {
    (UINT16) (DONT_CHECK),
    PXE_DBSIZE_NOT_USED,
    (UINT16) (DONT_CHECK),
    MUST_BE_INITIALIZED,
    E1000UndiFillHeader
  },
  {
    (UINT16) (DONT_CHECK),
    PXE_DBSIZE_NOT_USED,
    (UINT16) (DONT_CHECK),
    MUST_BE_INITIALIZED,
    E1000UndiTransmit
  },
  {
    sizeof (PXE_CPB_RECEIVE),
    sizeof (PXE_DB_RECEIVE),
    0,
    MUST_BE_INITIALIZED,
    E1000UndiReceive
  }
};

/** This routine determines the operational state of the UNDI.  It updates the state flags in the
   Command Descriptor Block based on information derived from the AdapterInfo instance data.

   To ensure the command has completed successfully, CdbPtr->StatCode will contain the result of
   the command execution. The CdbPtr->StatFlags will contain a STOPPED, STARTED, or INITIALIZED
   state once the command has successfully completed. Keep in mind the AdapterInfo->State is the
   active state of the adapter (based on software interrogation), and the CdbPtr->StateFlags is
   the passed back information that is reflected to the caller of the UNDI API.


   @param[in]   CdbPtr       Pointer to the command descriptor block.
   @param[in]   AdapterInfo  Pointer to the NIC data structure information which the
                             UNDI driver is layering on..

   @retval      None
**/
VOID
E1000UndiGetState (
  IN PXE_CDB     *CdbPtr,
  IN DRIVER_DATA *AdapterInfo
  )
{
  DEBUGPRINT (DECODE, ("E1000UndiGetState\n"));
  DEBUGWAIT (DECODE);

  CdbPtr->StatFlags |= AdapterInfo->State;
  CdbPtr->StatFlags |= PXE_STATFLAGS_COMMAND_COMPLETE;

  CdbPtr->StatCode = PXE_STATCODE_SUCCESS;
}

/** This routine is used to change the operational state of the 1-Gigabit UNDI
   from stopped to started.

   It will do this as long as the adapter's state is PXE_STATFLAGS_GET_STATE_STOPPED, otherwise
   the CdbPtr->StatFlags will reflect a command failure, and the CdbPtr->StatCode will reflect the
   UNDI as having already been started.
   This routine is modified to reflect the UNDI 1.1 specification changes. The
   changes in the spec. are mainly in the callback routines, the new spec. adds
   3 more callbacks and a unique id. Since this UNDI supports both old and new UNDI specifications,
   The NIC's data structure is filled in with the callback routines (depending
   on the version) pointed to in the caller's CpbPtr.  This seeds the Delay,
   Virt2Phys, Block, and Mem_IO for old and new versions and Map_Mem, UnMap_Mem
   and Sync_Mem routines and a unique id variable for the new version.
   This is the function which an external entity (SNP, O/S, etc) would call
   to provide it's I/O abstraction to the UNDI.
   It's final action is to change the AdapterInfo->State to PXE_STATFLAGS_GET_STATE_STARTED.

   @param[in]   CdbPtr        Pointer to the command descriptor block.
   @param[in]   AdapterInfo   Pointer to the NIC data structure information which the
                              UNDI driver is layering on..

   @retval      None
**/
VOID
E1000UndiStart (
  IN PXE_CDB     *CdbPtr,
  IN DRIVER_DATA *AdapterInfo
  )
{
  PXE_CPB_START_31 *CpbPtr31;

  DEBUGPRINT (DECODE, ("E1000UndiStart\n"));
  DEBUGWAIT (DECODE);

  // check if it is already started.
  if (AdapterInfo->State != PXE_STATFLAGS_GET_STATE_STOPPED) {
    CdbPtr->StatFlags = PXE_STATFLAGS_COMMAND_FAILED;
    CdbPtr->StatCode = PXE_STATCODE_ALREADY_STARTED;
    return;
  }

  if (CdbPtr->CPBsize != sizeof (PXE_CPB_START_30)
    && CdbPtr->CPBsize != sizeof (PXE_CPB_START_31))
  {
    CdbPtr->StatFlags = PXE_STATFLAGS_COMMAND_FAILED;
    CdbPtr->StatCode = PXE_STATCODE_INVALID_CDB;
    return;
  }

  CpbPtr31 = (PXE_CPB_START_31 *) (UINTN) (CdbPtr->CPBaddr);

  AdapterInfo->Delay     = (BS_PTR) (UINTN) CpbPtr31->Delay;
  AdapterInfo->Virt2Phys = (VIRT_PHYS) (UINTN) CpbPtr31->Virt2Phys;
  AdapterInfo->Block     = (BLOCK) (UINTN) CpbPtr31->Block;
  AdapterInfo->MemIo     = (MEM_IO) (UINTN) CpbPtr31->Mem_IO;
  AdapterInfo->MapMem    = (MAP_MEM) (UINTN) CpbPtr31->Map_Mem;
  AdapterInfo->UnMapMem  = (UNMAP_MEM) (UINTN) CpbPtr31->UnMap_Mem;
  AdapterInfo->SyncMem   = (SYNC_MEM) (UINTN) CpbPtr31->Sync_Mem;
  AdapterInfo->UniqueId = CpbPtr31->Unique_ID;
  DEBUGPRINT (DECODE, ("CpbPtr31->Unique_ID = %x\n", CpbPtr31->Unique_ID));

  AdapterInfo->State = PXE_STATFLAGS_GET_STATE_STARTED;

  CdbPtr->StatFlags     = PXE_STATFLAGS_COMMAND_COMPLETE;
  CdbPtr->StatCode      = PXE_STATCODE_SUCCESS;
}

/** This routine is used to change the operational state of the UNDI from started to stopped.

   It will not do this if the adapter's state is PXE_STATFLAGS_GET_STATE_INITIALIZED, otherwise
   the CdbPtr->StatFlags will reflect a command failure, and the CdbPtr->StatCode will reflect the
   UNDI as having already not been shut down.
   The NIC's data structure will have the Delay, Virt2Phys, and Block, pointers zero'd out..
   It's final action is to change the AdapterInfo->State to PXE_STATFLAGS_GET_STATE_STOPPED.

   @param[in]   CdbPtr        Pointer to the command descriptor block.
   @param[in]   AdapterInfo   Pointer to the NIC data structure information which the
                              UNDI driver is layering on..

   @retval      None
**/
VOID
E1000UndiStop (
  IN PXE_CDB     *CdbPtr,
  IN DRIVER_DATA *AdapterInfo
  )
{

  DEBUGPRINT (DECODE, ("E1000UndiStop\n"));
  DEBUGWAIT (DECODE);

  if (AdapterInfo->State == PXE_STATFLAGS_GET_STATE_INITIALIZED) {
    CdbPtr->StatFlags = PXE_STATFLAGS_COMMAND_FAILED;
    CdbPtr->StatCode = PXE_STATCODE_NOT_SHUTDOWN;
    return;
  }

  AdapterInfo->Delay         = 0;
  AdapterInfo->Virt2Phys     = 0;
  AdapterInfo->Block         = 0;

  AdapterInfo->MapMem        = 0;
  AdapterInfo->UnMapMem      = 0;
  AdapterInfo->SyncMem       = 0;

  AdapterInfo->State         = PXE_STATFLAGS_GET_STATE_STOPPED;

  CdbPtr->StatFlags             = PXE_STATFLAGS_COMMAND_COMPLETE;
  CdbPtr->StatCode              = PXE_STATCODE_SUCCESS;
}

/** This routine is used to retrieve the initialization information that is
   needed by drivers and applications to initialize the UNDI.

   This will fill in data in the Data Block structure that is pointed to by the
   caller's CdbPtr->DBaddr.  The fields filled in are as follows:
   MemoryRequired, FrameDataLen, LinkSpeeds[0-3], NvCount, NvWidth, MediaHeaderLen, HWaddrLen,
   MCastFilterCnt, TxBufCnt, TxBufSize, RxBufCnt, RxBufSize, IFtype, Duplex, and LoopBack.
   In addition, the CdbPtr->StatFlags ORs in that this NIC supports cable detection.  (APRIORI knowledge)

   @param[in]   CdbPtr        Pointer to the command descriptor block.
   @param[in]   AdapterInfo   Pointer to the NIC data structure information which the
                              UNDI driver is layering on..

   @retval      None
**/
VOID
E1000UndiGetInitInfo (
  IN PXE_CDB     *CdbPtr,
  IN DRIVER_DATA *AdapterInfo
  )
{
  PXE_DB_GET_INIT_INFO *DbPtr;

  DEBUGPRINT (DECODE, ("E1000UndiGetInitInfo\n"));
  DEBUGWAIT (DECODE);

  DbPtr                 = (PXE_DB_GET_INIT_INFO *) (UINTN) (CdbPtr->DBaddr);

  DbPtr->MemoryRequired = 0;
  DbPtr->FrameDataLen   = PXE_MAX_TXRX_UNIT_ETHER;

  // First check for FIBER, Links are 1000,0,0,0
  if (AdapterInfo->Hw.phy.media_type == e1000_media_type_copper ) {
    DbPtr->LinkSpeeds[0]  = 10;
    DbPtr->LinkSpeeds[1]  = 100;
    DbPtr->LinkSpeeds[2]  = 1000;
    DbPtr->LinkSpeeds[3]  = 0;
  } else {
    DbPtr->LinkSpeeds[0]  = 1000;
    DbPtr->LinkSpeeds[1]  = 0;
    DbPtr->LinkSpeeds[2]  = 0;
    DbPtr->LinkSpeeds[3]  = 0;
  }

  DbPtr->NvCount        = MAX_EEPROM_LEN;
  DbPtr->NvWidth        = 4;
  DbPtr->MediaHeaderLen = PXE_MAC_HEADER_LEN_ETHER;
  DbPtr->HWaddrLen      = PXE_HWADDR_LEN_ETHER;
  DbPtr->MCastFilterCnt = MAX_MCAST_ADDRESS_CNT;

  DbPtr->TxBufCnt       = DEFAULT_TX_DESCRIPTORS;
  DbPtr->TxBufSize      = sizeof (E1000_TRANSMIT_DESCRIPTOR);
  DbPtr->RxBufCnt       = DEFAULT_RX_DESCRIPTORS;
  DbPtr->RxBufSize      = sizeof (E1000_RECEIVE_DESCRIPTOR) + sizeof (LOCAL_RX_BUFFER);

  DbPtr->IFtype         = PXE_IFTYPE_ETHERNET;
  DbPtr->SupportedDuplexModes         = PXE_DUPLEX_ENABLE_FULL_SUPPORTED | PXE_DUPLEX_FORCE_FULL_SUPPORTED;
  DbPtr->SupportedLoopBackModes       = 0;

  CdbPtr->StatFlags |= (PXE_STATFLAGS_CABLE_DETECT_SUPPORTED |
                        PXE_STATFLAGS_GET_STATUS_NO_MEDIA_SUPPORTED);

  CdbPtr->StatFlags |= PXE_STATFLAGS_COMMAND_COMPLETE;
  CdbPtr->StatCode = PXE_STATCODE_SUCCESS;

  if (!AdapterInfo->UndiEnabled) {
    CdbPtr->StatCode = PXE_STATCODE_BUSY;
  }
}

/** This routine is used to retrieve the configuration information about the NIC being controlled by
  this driver.

  This will fill in data in the Data Block structure that is pointed to by the caller's CdbPtr->DBaddr.
  The fields filled in are as follows:
  DbPtr->pci.BusType, DbPtr->pci.Bus, DbPtr->pci.Device, and DbPtr->pci.
  In addition, the DbPtr->pci.Config.Dword[0-63] grabs a copy of this NIC's PCI configuration space.

  @param[in]   CdbPtr        Pointer to the command descriptor block.
  @param[in]   AdapterInfo   Pointer to the NIC data structure information which the
                             UNDI driver is layering on..

  @retval      None
**/
VOID
E1000UndiGetConfigInfo (
  IN PXE_CDB     *CdbPtr,
  IN DRIVER_DATA *AdapterInfo
  )
{
  PXE_DB_GET_CONFIG_INFO *DbPtr;

  DEBUGPRINT (DECODE, ("E1000UndiGetConfigInfo\n"));
  DEBUGWAIT (DECODE);

  DbPtr               = (PXE_DB_GET_CONFIG_INFO *) (UINTN) (CdbPtr->DBaddr);

  DbPtr->pci.BusType  = PXE_BUSTYPE_PCI;
  DbPtr->pci.Bus      = (UINT16) AdapterInfo->Bus;
  DbPtr->pci.Device   = (UINT8) AdapterInfo->Device;
  DbPtr->pci.Function = (UINT8) AdapterInfo->Function;
  DEBUGPRINT (
    DECODE, ("Bus %x, Device %x, Function %x\n",
    AdapterInfo->Bus,
    AdapterInfo->Device,
    AdapterInfo->Function)
  );

  CopyMem (DbPtr->pci.Config.Dword, &AdapterInfo->PciConfig, MAX_PCI_CONFIG_LEN * sizeof (UINT32));

  CdbPtr->StatFlags = PXE_STATFLAGS_COMMAND_COMPLETE;
  CdbPtr->StatCode = PXE_STATCODE_SUCCESS;
}

/** This routine resets the network adapter and initializes the 1-Gigabit UNDI using the parameters
   supplied in the CPB.

   This command must be issued before the network adapter can be setup to transmit and receive packets.
   Once the memory requirements of the UNDI are obtained by using the GetInitInfo command, a block
   of non-swappable memory may need to be allocated.  The address of this memory must be passed to
   UNDI during the Initialize in the CPB.  This memory is used primarily for transmit and receive buffers.
   The fields CableDetect, LinkSpeed, Duplex, LoopBack, MemoryPtr, and MemoryLength are set with
   information that was passed in the CPB and the NIC is initialized.
   If the NIC initialization fails, the CdbPtr->StatFlags are updated with PXE_STATFLAGS_COMMAND_FAILED
   Otherwise, AdapterInfo->State is updated with PXE_STATFLAGS_GET_STATE_INITIALIZED showing the state of
   the UNDI is now initialized.

   @param[in]   CdbPtr        Pointer to the command descriptor block.
   @param[in]   AdapterInfo   Pointer to the NIC data structure information which the
                              UNDI driver is layering on..

   @retval      None
-**/
VOID
E1000UndiInitialize (
  IN PXE_CDB     *CdbPtr,
  IN DRIVER_DATA *AdapterInfo
  )
{
  PXE_CPB_INITIALIZE *CpbPtr;
  PXE_DB_INITIALIZE * DbPtr;

  DEBUGPRINT (DECODE, ("E1000UndiInitialize\n"));
  DEBUGWAIT (DECODE);

  if (AdapterInfo->DriverBusy) {
    DEBUGPRINT (DECODE, ("ERROR: E1000UndiInitialize called when driver busy\n"));
    CdbPtr->StatFlags = PXE_STATFLAGS_COMMAND_FAILED;
    CdbPtr->StatCode = PXE_STATCODE_BUSY;
    return;
  }

  if ((CdbPtr->OpFlags != PXE_OPFLAGS_INITIALIZE_DETECT_CABLE) &&
    (CdbPtr->OpFlags != PXE_OPFLAGS_INITIALIZE_DO_NOT_DETECT_CABLE))
  {
    DEBUGPRINT (CRITICAL, ("INVALID CDB\n"));
    CdbPtr->StatFlags = PXE_STATFLAGS_COMMAND_FAILED;
    CdbPtr->StatCode = PXE_STATCODE_INVALID_CDB;
    return;
  }

  // Check if it is already initialized
  if (AdapterInfo->State == PXE_STATFLAGS_GET_STATE_INITIALIZED) {
    DEBUGPRINT (DECODE, ("ALREADY INITIALIZED\n"));
    CdbPtr->StatFlags = PXE_STATFLAGS_COMMAND_FAILED;
    CdbPtr->StatCode = PXE_STATCODE_ALREADY_INITIALIZED;
    return;
  }

  CpbPtr  = (PXE_CPB_INITIALIZE *) (UINTN) CdbPtr->CPBaddr;
  DbPtr   = (PXE_DB_INITIALIZE *) (UINTN) CdbPtr->DBaddr;

  // Default behaviour is to detect the cable, if the 3rd param is 1,
  // do not do that
  if (CdbPtr->OpFlags == (UINT16) PXE_OPFLAGS_INITIALIZE_DO_NOT_DETECT_CABLE) {
    AdapterInfo->CableDetect = (UINT8) 0;
  } else {
    AdapterInfo->CableDetect = (UINT8) 1;
  }
  DEBUGPRINT (DECODE, ("CdbPtr->OpFlags = %X\n", CdbPtr->OpFlags));
  AdapterInfo->LinkSpeed     = (UINT16) CpbPtr->LinkSpeed;
  AdapterInfo->DuplexMode    = CpbPtr->DuplexMode;
  AdapterInfo->LoopBack      = CpbPtr->LoopBackMode;

  DEBUGPRINT (DECODE, ("CpbPtr->TxBufCnt = %X\n", CpbPtr->TxBufCnt));
  DEBUGPRINT (DECODE, ("CpbPtr->TxBufSize = %X\n", CpbPtr->TxBufSize));
  DEBUGPRINT (DECODE, ("CpbPtr->RxBufCnt = %X\n", CpbPtr->RxBufCnt));
  DEBUGPRINT (DECODE, ("CpbPtr->RxBufSize = %X\n", CpbPtr->RxBufSize));

  if (AdapterInfo->CableDetect != 0) {
    DEBUGPRINT (DECODE, ("Setting wait_autoneg_complete\n"));
    AdapterInfo->Hw.phy.autoneg_wait_to_complete = TRUE;
  } else {
    AdapterInfo->Hw.phy.autoneg_wait_to_complete = FALSE;
  }

  CdbPtr->StatCode = (PXE_STATCODE) E1000Inititialize (AdapterInfo);

  // We allocate our own memory for transmit and receive so set MemoryUsed to 0.
  DbPtr->MemoryUsed = 0;
  DbPtr->TxBufCnt   = DEFAULT_TX_DESCRIPTORS;
  DbPtr->TxBufSize  = sizeof (E1000_TRANSMIT_DESCRIPTOR);
  DbPtr->RxBufCnt   = DEFAULT_RX_DESCRIPTORS;
  DbPtr->RxBufSize  = sizeof (E1000_RECEIVE_DESCRIPTOR) + sizeof (LOCAL_RX_BUFFER);

  if (CdbPtr->StatCode != PXE_STATCODE_SUCCESS) {
    DEBUGPRINT (CRITICAL, ("E1000Inititialize failed! Statcode = %X\n", CdbPtr->StatCode));
    CdbPtr->StatFlags = PXE_STATFLAGS_COMMAND_FAILED;
  } else {
    AdapterInfo->State = PXE_STATFLAGS_GET_STATE_INITIALIZED;
  }

  // If no link is detected we want to set the driver state back to _GET_STATE_STARTED so
  // that the SNP will not try to restart the driver.
  if (E1000WaitForAutoNeg (AdapterInfo)) {
    CdbPtr->StatFlags = PXE_STATFLAGS_COMMAND_COMPLETE;
  } else {
    CdbPtr->StatFlags |= PXE_STATFLAGS_INITIALIZED_NO_MEDIA;
    CdbPtr->StatCode = PXE_STATCODE_NOT_STARTED;
    AdapterInfo->State = PXE_STATFLAGS_GET_STATE_STARTED;
  }

  AdapterInfo->Hw.mac.get_link_status = TRUE;
}

/** This routine resets the network adapter and initializes the 1-Gigabit UNDI using the
   parameters supplied in the CPB.

   The transmit and receive queues are emptied and any pending interrupts are cleared.
   If the NIC reset fails, the CdbPtr->StatFlags are updated with PXE_STATFLAGS_COMMAND_FAILED

   @param[in]   CdbPtr         Pointer to the command descriptor block.
   @param[in]   AdapterInfo    Pointer to the NIC data structure information which the
                               UNDI driver is layering on..

   @retval      None
**/
VOID
E1000UndiReset (
  IN PXE_CDB     *CdbPtr,
  IN DRIVER_DATA *AdapterInfo
  )
{
  DEBUGPRINT (DECODE, ("E1000UndiReset\n"));
  DEBUGWAIT (DECODE);

  if (AdapterInfo->DriverBusy) {
    DEBUGPRINT (DECODE, ("ERROR: E1000UndiReset called when driver busy\n"));
    CdbPtr->StatFlags = PXE_STATFLAGS_COMMAND_FAILED;
    CdbPtr->StatCode = PXE_STATCODE_BUSY;
    return;
  }

  if (CdbPtr->OpFlags != PXE_OPFLAGS_NOT_USED &&
    CdbPtr->OpFlags != PXE_OPFLAGS_RESET_DISABLE_INTERRUPTS &&
    CdbPtr->OpFlags != PXE_OPFLAGS_RESET_DISABLE_FILTERS)
  {
    CdbPtr->StatFlags = PXE_STATFLAGS_COMMAND_FAILED;
    CdbPtr->StatCode = PXE_STATCODE_INVALID_CDB;
    return;
  }

  CdbPtr->StatCode = PXE_STATCODE_SUCCESS;

  if (CdbPtr->StatCode != PXE_STATCODE_SUCCESS) {
    CdbPtr->StatFlags = PXE_STATFLAGS_COMMAND_FAILED;
  } else {
    CdbPtr->StatFlags = PXE_STATFLAGS_COMMAND_COMPLETE;
  }
}

/** This routine resets the network adapter and leaves it in a safe state for another
   driver to initialize.

   Any pending transmits or receives are lost.  Receive filters and external
   interrupt enables are disabled.  Once the UNDI has been shutdown, it can then be stopped
   or initialized again.
   If the NIC reset fails, the CdbPtr->StatFlags are updated with PXE_STATFLAGS_COMMAND_FAILED
   Otherwise, AdapterInfo->State is updated with PXE_STATFLAGS_GET_STATE_STARTED showing
   the state of the NIC as being started.

   @param[in]   CdbPtr        Pointer to the command descriptor block.
   @param[in]   AdapterInfo   Pointer to the NIC data structure information which the
                              UNDI driver is layering on..

   @retval      None
**/
VOID
E1000UndiShutdown (
  IN PXE_CDB     *CdbPtr,
  IN DRIVER_DATA *AdapterInfo
  )
{
  // do the shutdown stuff here
  DEBUGPRINT (DECODE, ("E1000UndiShutdown\n"));
  DEBUGWAIT (DECODE);

  if (AdapterInfo->DriverBusy) {
    DEBUGPRINT (DECODE, ("ERROR: E1000UndiShutdown called when driver busy\n"));
    CdbPtr->StatFlags = PXE_STATFLAGS_COMMAND_FAILED;
    CdbPtr->StatCode = PXE_STATCODE_BUSY;
    return;
  }

  CdbPtr->StatCode =  (UINT16) E1000Shutdown (AdapterInfo);

  if (CdbPtr->StatCode != PXE_STATCODE_SUCCESS) {
    CdbPtr->StatFlags = PXE_STATFLAGS_COMMAND_FAILED;
  } else {
    AdapterInfo->State = PXE_STATFLAGS_GET_STATE_STARTED;
    CdbPtr->StatFlags     = PXE_STATFLAGS_COMMAND_COMPLETE;
  }
}

/** This routine can be used to read and/or change the current external interrupt enable
   settings.

   Disabling an external interrupt enable prevents and external (hardware)
   interrupt from being signalled by the network device.  Internally the interrupt events
   can still be polled by using the UNDI_GetState command.
   The resulting information on the interrupt state will be passed back in the CdbPtr->StatFlags.

   @param[in]   CdbPtr        Pointer to the command descriptor block.
   @param[in]   AdapterInfo   Pointer to the NIC data structure information which the
                              UNDI driver is layering on.

   @retval      None
**/
VOID
E1000UndiInterrupt (
  IN PXE_CDB     *CdbPtr,
  IN DRIVER_DATA *AdapterInfo
  )
{
  UINT8 IntMask;

  DEBUGPRINT (DECODE, ("E1000UndiInterrupt\n"));

  IntMask = (UINT8) (UINTN) (CdbPtr->OpFlags &
                            (PXE_OPFLAGS_INTERRUPT_RECEIVE |
                             PXE_OPFLAGS_INTERRUPT_TRANSMIT));

  switch (CdbPtr->OpFlags & PXE_OPFLAGS_INTERRUPT_OPMASK) {
  case PXE_OPFLAGS_INTERRUPT_READ:
    break;

  case PXE_OPFLAGS_INTERRUPT_ENABLE:
    if (IntMask == 0) {
      CdbPtr->StatFlags = PXE_STATFLAGS_COMMAND_FAILED;
      CdbPtr->StatCode = PXE_STATCODE_INVALID_CDB;
      return;
    }

    AdapterInfo->IntMask = IntMask;
    E1000SetInterruptState (AdapterInfo);
    break;

  case PXE_OPFLAGS_INTERRUPT_DISABLE:
    if (IntMask != 0) {
      AdapterInfo->IntMask &= ~(IntMask);
      E1000SetInterruptState (AdapterInfo);
      break;
    }

  default:
    CdbPtr->StatFlags = PXE_STATFLAGS_COMMAND_FAILED;
    CdbPtr->StatCode = PXE_STATCODE_INVALID_CDB;
    return;
    break;
  }

  if ((AdapterInfo->IntMask & PXE_OPFLAGS_INTERRUPT_RECEIVE) != 0) {
    CdbPtr->StatFlags |= PXE_STATFLAGS_INTERRUPT_RECEIVE;
  }

  if ((AdapterInfo->IntMask & PXE_OPFLAGS_INTERRUPT_TRANSMIT) != 0) {
    CdbPtr->StatFlags |= PXE_STATFLAGS_INTERRUPT_TRANSMIT;
  }

}

/** This routine is used to read and change receive filters and, if supported, read
   and change multicast MAC address filter list.

   @param[in]   CdbPtr        Pointer to the command descriptor block.
   @param[in]   AdapterInfo   Pointer to the NIC data structure information which the
                              UNDI driver is layering on..

   @retval     None
**/
VOID
E1000UndiRecFilter (
  IN PXE_CDB     *CdbPtr,
  IN DRIVER_DATA *AdapterInfo
  )
{

  UINT16                  NewFilter;
  UINT16                  OpFlags;
  PXE_DB_RECEIVE_FILTERS *DbPtr;

  DEBUGPRINT (DECODE, ("E1000UndiRecFilter\n"));

  if (AdapterInfo->DriverBusy) {
    DEBUGPRINT (DECODE, ("ERROR: E1000UndiRecFilter called when driver busy\n"));
    CdbPtr->StatFlags = PXE_STATFLAGS_COMMAND_FAILED;
    CdbPtr->StatCode = PXE_STATCODE_BUSY;
    return;
  }

  OpFlags = CdbPtr->OpFlags;
  NewFilter = (UINT16) (OpFlags & 0x1F);

  switch (OpFlags & PXE_OPFLAGS_RECEIVE_FILTER_OPMASK) {
  case PXE_OPFLAGS_RECEIVE_FILTER_READ:

    // not expecting a cpb, not expecting any filter bits
    if ((NewFilter != 0)
      || (CdbPtr->CPBsize != 0))
    {
      goto BadCdb;
    }

    goto JustRead;

    break;

  case PXE_OPFLAGS_RECEIVE_FILTER_ENABLE:

    // there should be atleast one other filter bit set.
    if (NewFilter == 0) {

      // nothing to enable
      goto BadCdb;
    }

    if (CdbPtr->CPBsize != 0) {

      // this must be a multicast address list!
      // don't accept the list unless selective_mcast is set
      // don't accept confusing mcast settings with this
      if (((OpFlags & PXE_OPFLAGS_RECEIVE_FILTER_FILTERED_MULTICAST) == 0) ||
        ((OpFlags & PXE_OPFLAGS_RECEIVE_FILTER_RESET_MCAST_LIST) != 0) ||
        ((OpFlags & PXE_OPFLAGS_RECEIVE_FILTER_ALL_MULTICAST) != 0))
      {
        goto BadCdb;
      }
    }

    // check selective mcast case enable case
    if ((OpFlags & PXE_OPFLAGS_RECEIVE_FILTER_FILTERED_MULTICAST) != 0) {
      if (((OpFlags & PXE_OPFLAGS_RECEIVE_FILTER_RESET_MCAST_LIST) != 0) ||
        ((OpFlags & PXE_OPFLAGS_RECEIVE_FILTER_ALL_MULTICAST) != 0))
      {
        goto BadCdb;
      }

      // if no cpb, make sure we have an old list
      if ((CdbPtr->CPBsize == 0)
        && (AdapterInfo->McastList.Length == 0))
      {
        goto BadCdb;
      }
    }

    // if you want to enable anything, you got to have unicast
    // and you have what you already enabled!
    NewFilter |= (PXE_OPFLAGS_RECEIVE_FILTER_UNICAST | AdapterInfo->RxFilter);

    break;

  case PXE_OPFLAGS_RECEIVE_FILTER_DISABLE:

    // mcast list not expected, i.e. no cpb here!
    if (CdbPtr->CPBsize != PXE_CPBSIZE_NOT_USED) {
      goto BadCdb;  // db with all_multi??
    }

    NewFilter = (UINT16) ((~(CdbPtr->OpFlags & 0x1F)) & AdapterInfo->RxFilter);

    break;

  default:
    goto BadCdb;
    break;
  }

  if ((OpFlags & PXE_OPFLAGS_RECEIVE_FILTER_RESET_MCAST_LIST) != 0) {
    AdapterInfo->McastList.Length = 0;
    NewFilter &= (~PXE_OPFLAGS_RECEIVE_FILTER_FILTERED_MULTICAST);
  }

  E1000SetFilter (AdapterInfo, NewFilter, CdbPtr->CPBaddr, CdbPtr->CPBsize);

JustRead:
  DEBUGPRINT (DECODE, ("Read current filter\n"));

  // give the current mcast list
  if ((CdbPtr->DBsize != 0)
    && (AdapterInfo->McastList.Length != 0))
  {

    // copy the mc list to db
    UINT16 i;
    UINT16 CopyLen;
    UINT8 *Ptr1;
    UINT8 *Ptr2;

    DbPtr = (PXE_DB_RECEIVE_FILTERS *) (UINTN) CdbPtr->DBaddr;
    Ptr1  = (UINT8 *) (&DbPtr->MCastList[0]);

    CopyLen = (UINT16) (AdapterInfo->McastList.Length * PXE_MAC_LENGTH);

    if (CopyLen > CdbPtr->DBsize) {
      CopyLen = CdbPtr->DBsize;

    }

    Ptr2 = (UINT8 *) (&AdapterInfo->McastList.McAddr[0]);
    for (i = 0; i < CopyLen; i++) {
      Ptr1[i] = Ptr2[i];
    }
  }

  // give the stat flags here
  if (AdapterInfo->RxRing.IsRunning) {
    CdbPtr->StatFlags |= (AdapterInfo->RxFilter | PXE_STATFLAGS_COMMAND_COMPLETE);
  }

  return;

BadCdb:
  DEBUGPRINT (CRITICAL, ("ERROR: Bad CDB!\n"));
  CdbPtr->StatFlags = PXE_STATFLAGS_COMMAND_FAILED;
  CdbPtr->StatCode = PXE_STATCODE_INVALID_CDB;
}

/** This routine is used to get the current station and broadcast MAC addresses,
   and to change the current station MAC address.

   @param[in]   CdbPtr        Pointer to the command descriptor block.
   @param[in]   AdapterInfo   Pointer to the NIC data structure information which the
                              UNDI driver is layering on.

   @retval      None
**/
VOID
E1000UndiStnAddr (
  IN PXE_CDB     *CdbPtr,
  IN DRIVER_DATA *AdapterInfo
  )
{

  PXE_CPB_STATION_ADDRESS *CpbPtr;
  PXE_DB_STATION_ADDRESS * DbPtr;
  UINT16                   i;

  DbPtr = NULL;
  DEBUGPRINT (DECODE, ("E1000UndiStnAddr\n"));

  if (CdbPtr->OpFlags == PXE_OPFLAGS_STATION_ADDRESS_RESET) {

    // configure the permanent address.
    // change the AdapterInfo->CurrentNodeAddress field.
    if (CompareMem (
        AdapterInfo->Hw.mac.addr,
        AdapterInfo->Hw.mac.perm_addr,
        PXE_HWADDR_LEN_ETHER
        ) != 0)
    {
      CopyMem (
        AdapterInfo->Hw.mac.addr,
        AdapterInfo->Hw.mac.perm_addr,
        PXE_HWADDR_LEN_ETHER
      );
      e1000_rar_set (&AdapterInfo->Hw, AdapterInfo->Hw.mac.addr, 0);
    }
  }

  if (CdbPtr->CPBaddr != (UINT64) 0) {
    CpbPtr = (PXE_CPB_STATION_ADDRESS *) (UINTN) (CdbPtr->CPBaddr);
    AdapterInfo->MacAddrOverride = TRUE;

    // configure the new address
    CopyMem (
      AdapterInfo->Hw.mac.addr,
      CpbPtr->StationAddr,
      PXE_HWADDR_LEN_ETHER
    );

    DEBUGPRINT (DECODE, ("Reassigned address:\n"));
    for (i = 0; i < 6; i++) {
      DEBUGPRINT (DECODE, ("%2x ", CpbPtr->StationAddr[i]));
    }

    e1000_rar_set (&AdapterInfo->Hw, AdapterInfo->Hw.mac.addr, 0);
  }

  if (CdbPtr->DBaddr != (UINT64) 0) {
    DbPtr = (PXE_DB_STATION_ADDRESS *) (UINTN) (CdbPtr->DBaddr);

    // fill it with the new values
    ZeroMem (DbPtr->StationAddr, PXE_MAC_LENGTH);
    ZeroMem (DbPtr->PermanentAddr, PXE_MAC_LENGTH);
    ZeroMem (DbPtr->BroadcastAddr, PXE_MAC_LENGTH);
    CopyMem (DbPtr->StationAddr, AdapterInfo->Hw.mac.addr, PXE_HWADDR_LEN_ETHER);
    CopyMem (DbPtr->PermanentAddr, AdapterInfo->Hw.mac.perm_addr, PXE_HWADDR_LEN_ETHER);
    CopyMem (DbPtr->BroadcastAddr, AdapterInfo->BroadcastNodeAddress, PXE_MAC_LENGTH);
  }

  DEBUGPRINT (DECODE, ("DbPtr->BroadcastAddr ="));
  for (i = 0; i < PXE_MAC_LENGTH; i++) {
    DEBUGPRINT (DECODE, (" %x", DbPtr->BroadcastAddr[i]));
  }

  DEBUGPRINT (DECODE, ("\n"));
  DEBUGWAIT (DECODE);
  CdbPtr->StatFlags = PXE_STATFLAGS_COMMAND_COMPLETE;
  CdbPtr->StatCode  = PXE_STATCODE_SUCCESS;
}

/** This routine is used to read and clear the NIC traffic statistics.  This command is supported
   only if the !PXE structure's Implementation flags say so.

   Results will be parsed out in the following manner:
   CdbPtr->DBaddr.Data[0]   R  Total Frames (Including frames with errors and dropped frames)
   CdbPtr->DBaddr.Data[1]   R  Good Frames (All frames copied into receive buffer)
   CdbPtr->DBaddr.Data[2]   R  Undersize Frames (Frames below minimum length for media <64 for ethernet)
   CdbPtr->DBaddr.Data[4]   R  Dropped Frames (Frames that were dropped because receive buffers were full)
   CdbPtr->DBaddr.Data[8]   R  CRC Error Frames (Frames with alignment or CRC errors)
   CdbPtr->DBaddr.Data[A]   T  Total Frames (Including frames with errors and dropped frames)
   CdbPtr->DBaddr.Data[B]   T  Good Frames (All frames copied into transmit buffer)
   CdbPtr->DBaddr.Data[C]   T  Undersize Frames (Frames below minimum length for media <64 for ethernet)
   CdbPtr->DBaddr.Data[E]   T  Dropped Frames (Frames that were dropped because of collisions)
   CdbPtr->DBaddr.Data[14]  T  Total Collision Frames (Total collisions on this subnet)

   @param[in]   CdbPtr        Pointer to the command descriptor block.
   @param[in]   AdapterInfo   Pointer to the NIC data structure information which the
                              UNDI driver is layering on..

   @retval      None
**/
VOID
E1000UndiStatistics (
  IN PXE_CDB     *CdbPtr,
  IN DRIVER_DATA *AdapterInfo
  )
{
  DEBUGPRINT (DECODE, ("E1000UndiStatistics\n"));

  if ((CdbPtr->OpFlags & ~(PXE_OPFLAGS_STATISTICS_RESET)) != 0) {
    CdbPtr->StatFlags = PXE_STATFLAGS_COMMAND_FAILED;
    CdbPtr->StatCode  = PXE_STATCODE_INVALID_CDB;
    return;
  }

  if ((CdbPtr->OpFlags & PXE_OPFLAGS_STATISTICS_RESET) != 0) {

    // Reset the statistics
    CdbPtr->StatCode = (UINT16) E1000Statistics (AdapterInfo, 0, 0);
  } else {
    CdbPtr->StatCode = (UINT16) E1000Statistics (AdapterInfo, CdbPtr->DBaddr, CdbPtr->DBsize);
  }
}

/** This routine is used to translate a multicast IP address to a multicast MAC address.

   This results in a MAC address composed of 25 bits of fixed data with the upper 23 bits of the IP
   address being appended to it.  Results passed back in the equivalent of CdbPtr->DBaddr->MAC[0-5].

   @param[in]   CdbPtr        Pointer to the command descriptor block.
   @param[in]   AdapterInfo   Pointer to the NIC data structure information which the
                              UNDI driver is layering on..

   @retval      None
**/
VOID
E1000UndiIp2Mac (
  IN PXE_CDB     *CdbPtr,
  IN DRIVER_DATA *AdapterInfo
  )
{
  PXE_CPB_MCAST_IP_TO_MAC *CpbPtr;
  PXE_DB_MCAST_IP_TO_MAC * DbPtr;
  UINT32                   IpAddr;
  UINT8 *                  TmpPtr;

  CpbPtr  = (PXE_CPB_MCAST_IP_TO_MAC *) (UINTN) CdbPtr->CPBaddr;
  DbPtr   = (PXE_DB_MCAST_IP_TO_MAC *) (UINTN) CdbPtr->DBaddr;

  DEBUGPRINT (DECODE, ("E1000UndiIp2Mac\n"));

  if ((CdbPtr->OpFlags & PXE_OPFLAGS_MCAST_IPV6_TO_MAC) != 0) {
    UINT8 *Ipv6Ptr;

    Ipv6Ptr    = (UINT8 *) &CpbPtr->IP.IPv6;

    DbPtr->MAC[0] = 0x33;
    DbPtr->MAC[1] = 0x33;
    DbPtr->MAC[2] = *(Ipv6Ptr + 12);
    DbPtr->MAC[3] = *(Ipv6Ptr + 13);
    DbPtr->MAC[4] = *(Ipv6Ptr + 14);
    DbPtr->MAC[5] = *(Ipv6Ptr + 15);
    return;
  }

  // Take the last 23 bits of IP to generate a multicase IP address.
  IpAddr        = CpbPtr->IP.IPv4;
  TmpPtr        = (UINT8 *) (&IpAddr);

  DbPtr->MAC[0] = 0x01;
  DbPtr->MAC[1] = 0x00;
  DbPtr->MAC[2] = 0x5E;
  DbPtr->MAC[3] = (UINT8) (TmpPtr[1] & 0x7F);
  DbPtr->MAC[4] = (UINT8) TmpPtr[2];
  DbPtr->MAC[5] = (UINT8) TmpPtr[3];
}

/** This routine is used to read and write non-volatile storage on the NIC (if supported).  The NVRAM
   could be EEPROM, FLASH, or battery backed RAM.

   This is an optional function according to the UNDI specification  (or will be......)

   @param[in]   CdbPtr        Pointer to the command descriptor block.
   @param[in]   AdapterInfo   Pointer to the NIC data structure information which the
                              UNDI driver is layering on..

   @retval      None
**/
VOID
E1000UndiNvData (
  IN PXE_CDB     *CdbPtr,
  IN DRIVER_DATA *AdapterInfo
  )
{
  PXE_DB_NVDATA *      DbPtr;
  PXE_CPB_NVDATA_BULK *PxeCpbNvdata;
  UINT32               Result;

  DEBUGPRINT (DECODE, ("E1000UndiNvData\n"));

  if ((AdapterInfo->State != PXE_STATFLAGS_GET_STATE_STARTED) &&
    (AdapterInfo->State != PXE_STATFLAGS_GET_STATE_INITIALIZED))
  {
    CdbPtr->StatFlags = PXE_STATFLAGS_COMMAND_FAILED;
    CdbPtr->StatCode = PXE_STATCODE_NOT_STARTED;
    return;
  }

  if ((CdbPtr->OpFlags == PXE_OPFLAGS_NVDATA_READ) != 0) {
    DbPtr = (PXE_DB_NVDATA *) (UINTN) CdbPtr->DBaddr;
    Result = e1000_read_nvm (&AdapterInfo->Hw, 0, 256, &DbPtr->Data.Word[0]);
  } else {
    PxeCpbNvdata = (PXE_CPB_NVDATA_BULK *) (UINTN) CdbPtr->CPBaddr;
    Result        = e1000_write_nvm (&AdapterInfo->Hw, 0x40, 0xBF, &PxeCpbNvdata->Word[0x40]);
  }

  if (Result == E1000_SUCCESS) {
    CdbPtr->StatFlags = PXE_STATFLAGS_COMMAND_COMPLETE;
    CdbPtr->StatCode = PXE_STATCODE_SUCCESS;
  } else {
    CdbPtr->StatFlags = PXE_STATFLAGS_COMMAND_FAILED;
    CdbPtr->StatCode = PXE_STATCODE_SUCCESS;
  }
}

/** This routine returns the current interrupt status and/or the transmitted buffer addresses.

   If the current interrupt status is returned, pending interrupts will be acknowledged by this
   command.  Transmitted buffer addresses that are written to the DB are removed from the transmit
   buffer queue.
   Normally, this command would be polled with interrupts disabled.
   The transmit buffers are returned in CdbPtr->DBaddr->TxBufer[0 - NumEntries].
   The interrupt status is returned in CdbPtr->StatFlags.

   @param[in]   CdbPtr        Pointer to the command descriptor block.
   @param[in]   AdapterInfo   Pointer to the NIC data structure information which the 1-Gigabit
                             UNDI driver is layering on..

   @retval   None
**/
VOID
E1000UndiStatus (
  IN PXE_CDB     *CdbPtr,
  IN DRIVER_DATA *AdapterInfo
  )
{
  EFI_STATUS                Status;
  BOOLEAN                   LinkUp;
  PXE_DB_GET_STATUS         *DbPtr;
  UINT16                    IntStatus;
  UINT16                    NumEntries;
  UINT16                    RxPacketLength;

  DEBUGPRINT (DECODE, ("E1000UndiStatus\n"));

  if (AdapterInfo->DriverBusy) {
    DEBUGPRINT (DECODE, ("ERROR: E1000UndiStatus called when driver busy\n"));
    CdbPtr->StatFlags = PXE_STATFLAGS_COMMAND_FAILED;
    CdbPtr->StatCode = PXE_STATCODE_BUSY;
    return;
  }

  // If the size of the DB is not large enough to store at least one 64 bit
  // complete transmit buffer address and size of the next available receive
  // packet we will return an error.  Per E.4.16 of the EFI spec the DB should
  // have enough space for at least 1 completed transmit buffer.
  if (CdbPtr->DBsize < (sizeof (UINT64) * 2)) {
    CdbPtr->StatFlags = PXE_STATFLAGS_COMMAND_FAILED;
    CdbPtr->StatCode = PXE_STATCODE_INVALID_CDB;
    DEBUGPRINT (CRITICAL, ("Invalid CDB\n"));
    if ((CdbPtr->OpFlags & PXE_OPFLAGS_GET_TRANSMITTED_BUFFERS) != 0) {
      CdbPtr->StatFlags |= PXE_STATFLAGS_GET_STATUS_NO_TXBUFS_WRITTEN;
    }

    return;
  }

  DbPtr = (PXE_DB_GET_STATUS *) (UINTN) CdbPtr->DBaddr;

  // Fill in size of next available receive packet and
  // reserved field in caller's DB storage.
  Status = ReceiveIsPacketReady (
             AdapterInfo,
             &RxPacketLength,
             NULL,
             NULL,
             NULL
             );

  if (Status == EFI_SUCCESS) {
    DbPtr->RxFrameLen = RxPacketLength;
  } else {
    DbPtr->RxFrameLen = 0;
  }

  DbPtr->reserved = 0;

  // Fill in the completed transmit buffer addresses so they can be freed by
  // the calling application or driver
  if ((CdbPtr->OpFlags & PXE_OPFLAGS_GET_TRANSMITTED_BUFFERS) != 0) {

    // Calculate the number of entries available in the DB to save the addresses
    // of completed transmit buffers.
    NumEntries = (UINT16) ((CdbPtr->DBsize - sizeof (UINT64)) / sizeof (UINT64));
    DEBUGPRINT (DECODE, ("CdbPtr->DBsize = %d\n", CdbPtr->DBsize));
    DEBUGPRINT (DECODE, ("NumEntries in DbPtr = %d\n", NumEntries));

    // On return NumEntries will be the number of TX buffers written into the DB
    NumEntries = E1000FreeTxBuffers (AdapterInfo, NumEntries, DbPtr->TxBuffer);
    if (NumEntries == 0) {
      CdbPtr->StatFlags |= PXE_STATFLAGS_GET_STATUS_NO_TXBUFS_WRITTEN;
    }

    // The receive buffer size and reserved fields take up the first 64 bits of the DB
    // The completed transmit buffers take up the rest
    CdbPtr->DBsize = (UINT16) (sizeof (UINT64) + NumEntries * sizeof (UINT64));
    DEBUGPRINT (DECODE, ("Return DBsize = %d\n", CdbPtr->DBsize));
  }

  if ((CdbPtr->OpFlags & PXE_OPFLAGS_GET_INTERRUPT_STATUS) != 0) {
    IntStatus = (UINT16) E1000_READ_REG (&AdapterInfo->Hw, E1000_ICR);

    // Report all the outstanding interrupts.
    if ((IntStatus & (E1000_ICR_RXT0 | E1000_ICR_RXSEQ | E1000_ICR_RXDMT0 | E1000_ICR_RXO | E1000_ICR_RXCFG)) != 0) {
      CdbPtr->StatFlags |= PXE_STATFLAGS_GET_STATUS_RECEIVE;
    }

    if ((IntStatus & (E1000_ICR_TXDW | E1000_ICR_TXQE)) != 0) {
      CdbPtr->StatFlags |= PXE_STATFLAGS_GET_STATUS_TRANSMIT;
    }

    // Acknowledge the interrupts.
    E1000_WRITE_REG (&AdapterInfo->Hw, E1000_ICR, IntStatus);
  }

  // Return current media status
  if ((CdbPtr->OpFlags & PXE_OPFLAGS_GET_MEDIA_STATUS) != 0) {
    Status = GetLinkStatus (UNDI_PRIVATE_DATA_FROM_DRIVER_DATA (AdapterInfo), &LinkUp);
    if (EFI_ERROR (Status)) {
      CdbPtr->StatCode = PXE_STATFLAGS_COMMAND_FAILED;
      return;
    }
    if (!LinkUp) {
      CdbPtr->StatFlags |= PXE_STATFLAGS_GET_STATUS_NO_MEDIA;
    }
  }

  CdbPtr->StatFlags |= PXE_STATFLAGS_COMMAND_COMPLETE;
  CdbPtr->StatCode = PXE_STATCODE_SUCCESS;
}

/** This routine is used to fill media header(s) in transmit packet(s).

   Copies the MAC address into the media header whether it is dealing
   with fragmented or non-fragmented packets.

   @param[in]   CdbPtr        Pointer to the command descriptor block.
   @param[in]   AdapterInfo   Pointer to the NIC data structure information which the
                              UNDI driver is layering on.

   @retval      None
**/
VOID
E1000UndiFillHeader (
  IN PXE_CDB     *CdbPtr,
  IN DRIVER_DATA *AdapterInfo
  )
{
  PXE_CPB_FILL_HEADER *           Cpb;
  PXE_CPB_FILL_HEADER_FRAGMENTED *Cpbf;
  ETHER_HEADER *                  MacHeader;
  UINTN                           i;

  DEBUGPRINT (DECODE, ("E1000UndiFillHeader\n"));

  if (CdbPtr->CPBsize == PXE_CPBSIZE_NOT_USED) {
    CdbPtr->StatFlags = PXE_STATFLAGS_COMMAND_FAILED;
    CdbPtr->StatCode = PXE_STATCODE_INVALID_CDB;
    return;
  }

  if ((CdbPtr->OpFlags & PXE_OPFLAGS_FILL_HEADER_FRAGMENTED) != 0) {
    Cpbf = (PXE_CPB_FILL_HEADER_FRAGMENTED *) (UINTN) CdbPtr->CPBaddr;

    // Assume 1st fragment is big enough for the mac header.
    if ((Cpbf->FragCnt == 0)
      || (Cpbf->FragDesc[0].FragLen < PXE_MAC_HEADER_LEN_ETHER))
    {

      // No buffers given.
      CdbPtr->StatFlags = PXE_STATFLAGS_COMMAND_FAILED;
      CdbPtr->StatCode = PXE_STATCODE_INVALID_CDB;
      return;
    }

    MacHeader = (ETHER_HEADER *) (UINTN) Cpbf->FragDesc[0].FragAddr;

    // We don't swap the protocol bytes.
    MacHeader->Type = Cpbf->Protocol;

    DEBUGPRINT (DECODE, ("MacHeader->SrcAddr = "));
    for (i = 0; i < PXE_HWADDR_LEN_ETHER; i++) {
      MacHeader->DestAddr[i] = Cpbf->DestAddr[i];
      MacHeader->SrcAddr[i] = Cpbf->SrcAddr[i];
      DEBUGPRINT (DECODE, ("%x ", MacHeader->SrcAddr[i]));
    }

    DEBUGPRINT (DECODE, ("\n"));
  } else {
    Cpb       = (PXE_CPB_FILL_HEADER *) (UINTN) CdbPtr->CPBaddr;
    MacHeader = (ETHER_HEADER *) (UINTN) Cpb->MediaHeader;

    // We don't swap the protocol bytes.
    MacHeader->Type = Cpb->Protocol;

    DEBUGPRINT (DECODE, ("MacHeader->SrcAddr = "));
    for (i = 0; i < PXE_HWADDR_LEN_ETHER; i++) {
      MacHeader->DestAddr[i] = Cpb->DestAddr[i];
      MacHeader->SrcAddr[i] = Cpb->SrcAddr[i];
      DEBUGPRINT (DECODE, ("%x ", MacHeader->SrcAddr[i]));
    }

    DEBUGPRINT (DECODE, ("\n"));
  }

  DEBUGWAIT (DECODE);
}

/** This routine is used to place a packet into the transmit queue.

   The data buffers given to this command are to be considered locked and the application or
   network driver loses ownership of these buffers and must not free or relocate them until
   the ownership returns.
   When the packets are transmitted, a transmit complete interrupt is generated (if interrupts
   are disabled, the transmit interrupt status is still set and can be checked using the UNDI_Status
   command.
   Some implementations and adapters support transmitting multiple packets with one transmit
   command.  If this feature is supported, the transmit CPBs can be linked in one transmit
   command.
   All UNDIs support fragmented frames, now all network devices or protocols do.  If a fragmented
   frame CPB is given to UNDI and the network device does not support fragmented frames
   (see !PXE.Implementation flag), the UNDI will have to copy the fragments into a local buffer
   before transmitting.

   @param[in]   CdbPtr        Pointer to the command descriptor block.
   @param[in]   AdapterInfo   Pointer to the NIC data structure information which the
                              UNDI driver is layering on..

   @retval      None
**/
VOID
E1000UndiTransmit (
  IN PXE_CDB     *CdbPtr,
  IN DRIVER_DATA *AdapterInfo
  )
{
  if (AdapterInfo->DriverBusy) {
    DEBUGPRINT (DECODE, ("ERROR: E1000UndiTransmit called when driver busy\n"));
    CdbPtr->StatFlags = PXE_STATFLAGS_COMMAND_FAILED;
    CdbPtr->StatCode = PXE_STATCODE_BUSY;
    return;
  }

  if (CdbPtr->CPBsize == PXE_CPBSIZE_NOT_USED) {
    CdbPtr->StatFlags = PXE_STATFLAGS_COMMAND_FAILED;
    CdbPtr->StatCode = PXE_STATCODE_INVALID_CDB;
    return;
  }


  CdbPtr->StatCode = (PXE_STATCODE) E1000Transmit (AdapterInfo, CdbPtr->CPBaddr, CdbPtr->OpFlags);

  if (CdbPtr->StatCode == PXE_STATCODE_SUCCESS) {
    CdbPtr->StatFlags = PXE_STATFLAGS_COMMAND_COMPLETE;
  } else {
    CdbPtr->StatFlags = PXE_STATFLAGS_COMMAND_FAILED;
  }
}

/** When the network adapter has received a frame, this command is used to copy the frame
   into the driver/application storage location.

   Once a frame has been copied, it is removed from the receive queue.

   @param[in]   CdbPtr        Pointer to the command descriptor block.
   @param[in]   AdapterInfo   Pointer to the NIC data structure information which the
                              UNDI driver is layering on..

   @retval      None
**/
VOID
E1000UndiReceive (
  IN PXE_CDB     *CdbPtr,
  IN DRIVER_DATA *AdapterInfo
  )
{

  if (AdapterInfo->DriverBusy) {
    DEBUGPRINT (DECODE, ("ERROR: E1000UndiReceive called while driver busy\n"));
    CdbPtr->StatFlags = PXE_STATFLAGS_COMMAND_FAILED;
    CdbPtr->StatCode = PXE_STATCODE_BUSY;
    return;
  }


  CdbPtr->StatCode = (UINT16) E1000Receive (
                                AdapterInfo,
                                (PXE_CPB_RECEIVE *) (UINTN) CdbPtr->CPBaddr,
                                (PXE_DB_RECEIVE *) (UINTN) CdbPtr->DBaddr
                                );

  if (CdbPtr->StatCode == PXE_STATCODE_SUCCESS) {
    CdbPtr->StatFlags = PXE_STATFLAGS_COMMAND_COMPLETE;
  } else {
    CdbPtr->StatFlags = PXE_STATFLAGS_COMMAND_FAILED;
  }
}

/** This is the main SW UNDI API entry using the newer nii protocol.
   The parameter passed in is a 64 bit flat model virtual
   address of the Cdb.  We then jump into the common routine for both old and
   new nii protocol entries.

   @param[in]   Cdb   Pointer to the command descriptor block.

   @retval   None
**/
VOID
EFIAPI
E1000UndiApiEntry (
  IN  UINT64 Cdb
  )
{
  PXE_CDB         *CdbPtr;
  DRIVER_DATA     *AdapterInfo;
  UNDI_CALL_TABLE *TabPtr;

  if (Cdb == (UINT64) 0) {
    return;
  }

  CdbPtr = (PXE_CDB *) (UINTN) Cdb;

  if (CdbPtr->IFnum > mE1000Pxe31->IFcnt) {
    DEBUGPRINT (DECODE, ("Invalid IFnum %d\n", CdbPtr->IFnum));
    CdbPtr->StatFlags = PXE_STATFLAGS_COMMAND_FAILED;
    CdbPtr->StatCode  = PXE_STATCODE_INVALID_CDB;
    return;
  }

  AdapterInfo = &(mUndi32DeviceList[CdbPtr->IFnum]->NicInfo);

  // Check if InitUndiNotifyExitBs was called before
  if (mExitBootServicesTriggered) {
    DEBUGPRINT (CRITICAL, ("Exit Boot Services triggered prior to entering UNDI API entry.\n"));
    CdbPtr->StatFlags = PXE_STATFLAGS_COMMAND_FAILED;
    CdbPtr->StatCode  = PXE_STATCODE_NOT_INITIALIZED;
    return;
  }

  AdapterInfo->VersionFlag = 0x31; // entering from new entry point

  // Check the OPCODE range.
  if ((CdbPtr->OpCode > PXE_OPCODE_LAST_VALID) ||
    (CdbPtr->StatCode != PXE_STATCODE_INITIALIZE) ||
    (CdbPtr->StatFlags != PXE_STATFLAGS_INITIALIZE))
  {
    DEBUGPRINT (DECODE, ("Invalid StatCode, OpCode, or StatFlags.\n", CdbPtr->IFnum));
    goto BadCdb;
  }

  if (CdbPtr->CPBsize == PXE_CPBSIZE_NOT_USED) {
    if (CdbPtr->CPBaddr != PXE_CPBADDR_NOT_USED) {
      goto BadCdb;
    }
  } else if (CdbPtr->CPBaddr == PXE_CPBADDR_NOT_USED) {
    goto BadCdb;
  }

  if (CdbPtr->DBsize == PXE_DBSIZE_NOT_USED) {
    if (CdbPtr->DBaddr != PXE_DBADDR_NOT_USED) {
      goto BadCdb;
    }
  } else if (CdbPtr->DBaddr == PXE_DBADDR_NOT_USED) {
    goto BadCdb;
  }

  // Check if cpbsize and dbsize are as needed.
  // Check if opflags are as expected.
  TabPtr = &mE1000ApiTable[CdbPtr->OpCode];

  if (TabPtr->CpbSize != (UINT16) (DONT_CHECK)
    && TabPtr->CpbSize != CdbPtr->CPBsize)
  {
    goto BadCdb;
  }

  if (TabPtr->DbSize != (UINT16) (DONT_CHECK)
    && TabPtr->DbSize != CdbPtr->DBsize)
  {
    goto BadCdb;
  }

  if (TabPtr->OpFlags != (UINT16) (DONT_CHECK)
    && TabPtr->OpFlags != CdbPtr->OpFlags)
  {
    goto BadCdb;
  }

  // Check if UNDI_State is valid for this call.
  if (TabPtr->State != (UINT16) (-1)) {

    // Should atleast be started.
    if (AdapterInfo->State == PXE_STATFLAGS_GET_STATE_STOPPED) {
      CdbPtr->StatFlags = PXE_STATFLAGS_COMMAND_FAILED;
      CdbPtr->StatCode  = PXE_STATCODE_NOT_STARTED;
      return;
    }

    // Check if it should be initialized.
    if (TabPtr->State == 2) {
      if (AdapterInfo->State != PXE_STATFLAGS_GET_STATE_INITIALIZED) {
        CdbPtr->StatCode  = PXE_STATCODE_NOT_INITIALIZED;
        CdbPtr->StatFlags = PXE_STATFLAGS_COMMAND_FAILED;
        return;
      }
    }
  }

  // Set the return variable for success case here.
  CdbPtr->StatFlags = PXE_STATFLAGS_COMMAND_COMPLETE;
  CdbPtr->StatCode  = PXE_STATCODE_SUCCESS;

  TabPtr->ApiPtr (CdbPtr, AdapterInfo);
  return;

BadCdb:
  CdbPtr->StatFlags = PXE_STATFLAGS_COMMAND_FAILED;
  CdbPtr->StatCode  = PXE_STATCODE_INVALID_CDB;
}
