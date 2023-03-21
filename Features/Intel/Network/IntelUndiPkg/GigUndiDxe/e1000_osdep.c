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

/** Reads from the PCI capabality region.

   @param[in]    Hw      Pointer to the shared code hw structure.
   @param[in]    Reg     The register offset within the cap region.
   @param[out]   Value   The value at the register offset.

   @retval   INVALID_STATUS_REGISTER_VALUE    Invalid status register value
   @retval   E1000_SUCCESS      Value read successfully
   @retval   E1000_ERR_CONFIG   The requested region was not found in PCI space
**/
INT32
e1000_read_pcie_cap_reg (
  struct e1000_hw *Hw,
  UINT32           Reg,
  UINT16 *         Value
  )
{
  // Start at the first cap pointer of PCI space
  UINT16 NextPtrOffset = PCI_CAP_PTR;
  UINT16 NextPtrValue = 0;

  DRIVER_DATA *AdapterInfo = Hw->back;

  if (IsSurpriseRemoval (AdapterInfo)) {
    return INVALID_STATUS_REGISTER_VALUE;
  }

  e1000_read_pci_cfg (Hw, NextPtrOffset, &NextPtrValue);

  // Keep only the first byte of the returned Value
  NextPtrValue &= 0xFF;

  // Traverse the capabilities linked regions until the end
  while (NextPtrValue != PCI_CAP_PTR_ENDPOINT) {
    NextPtrOffset = NextPtrValue;
    e1000_read_pci_cfg (Hw, NextPtrOffset, &NextPtrValue);

    // Check if we found the requested capabilities region
    if ((NextPtrValue & 0xFF) != PCI_EX_CAP_ID) {

      // Jump to the next capabilities region
      NextPtrValue &= 0xFF00;
      NextPtrValue = NextPtrValue >> 8;
      NextPtrValue &= 0xFF;
    } else {

      // Read the Value from the request offset
      e1000_read_pci_cfg (Hw, NextPtrOffset + Reg, &NextPtrValue);
      *Value = NextPtrValue;
      return E1000_SUCCESS;
    }
  }

  // The requested region was not found in PCI space
  DEBUGPRINT (IO, ("Cap ID 0x10 was not found in PCI space.\n"));
  return E1000_ERR_CONFIG;
}

#ifndef NO_PCIE_SUPPORT
/** Writes into the PCI capabality region.

   @param[in]   Hw      Pointer to the shared code hw structure.
   @param[in]   Reg     The register offset within the cap region.
   @param[in]   Value   The value at the register offset.

   @retval   INVALID_STATUS_REGISTER_VALUE    Invalid status register value
   @retval   E1000_SUCCESS      Value written successfully
   @retval   E1000_ERR_CONFIG   The requested region was not found in PCI space
**/
INT32
e1000_write_pcie_cap_reg (
  struct e1000_hw *Hw,
  UINT32           Reg,
  UINT16 *         Value
  )
{
  // Start at the first cap pointer of PCI space
  UINT16 NextPtrOffset = PCI_CAP_PTR;
  UINT16 NextPtrValue = 0;

  DRIVER_DATA *AdapterInfo = Hw->back;

  if (IsSurpriseRemoval (AdapterInfo)) {
    return INVALID_STATUS_REGISTER_VALUE;
  }

  e1000_read_pci_cfg (Hw, NextPtrOffset, &NextPtrValue);

  // Keep only the first byte of the returned Value
  NextPtrValue &= 0xFF;

  // Traverse the capabilities linked regions until the end
  while (NextPtrValue != PCI_CAP_PTR_ENDPOINT) {
    NextPtrOffset = NextPtrValue;
    e1000_read_pci_cfg (Hw, NextPtrOffset, &NextPtrValue);

    // Check if we found the requested capabilities region
    if ((NextPtrValue & 0xFF) != PCI_EX_CAP_ID) {

      // Jump to the next capabilities region
      NextPtrValue &= 0xFF00;
      NextPtrValue = NextPtrValue >> 8;
      NextPtrValue &= 0xFF;
    } else {

      // Write the Value from the request offset
      e1000_write_pci_cfg (Hw, NextPtrOffset + Reg, Value);
      return E1000_SUCCESS;
    }
  }

  // The requested region was not found in PCI space
  DEBUGPRINT (IO, ("Cap ID 0x10 was not found in PCI space.\n"));
  return E1000_ERR_CONFIG;
}
#endif /* NO_PCIE_SUPPORT */

/** Writes a value to one of the devices registers using port I/O (as opposed to
   memory mapped I/O). Only 82544 and newer devices support port I/O.

   @param[in]   Hw      Pointer to the shared code hw structure.
   @param[in]   Offset  The register offset to write.
   @param[in]   Value   The value to write to the register.

   @return   Value written to register offset
**/
VOID
E1000WriteRegIo (
  struct e1000_hw *Hw,
  UINT32           Offset,
  UINT32           Value
  )
{
  DRIVER_DATA *AdapterInfo = Hw->back;

  if (IsSurpriseRemoval (AdapterInfo)) {
    return;
  }

  DEBUGPRINT (IO, ("e1000_write_reg_io\n"));
  DEBUGPRINT (IO, ("IO bAR INDEX = %d\n", AdapterInfo->IoBarIndex));
  DEBUGWAIT (IO);

  MemoryFence ();
  AdapterInfo->PciIo->Io.Write (
                           AdapterInfo->PciIo,
                           EfiPciIoWidthUint32,
                           AdapterInfo->IoBarIndex,
                           0,                          // IO location offset
                           1,
                           (VOID *) (&Offset)
                         );
  MemoryFence ();
  AdapterInfo->PciIo->Io.Write (
                           AdapterInfo->PciIo,
                           EfiPciIoWidthUint32,
                           AdapterInfo->IoBarIndex,
                           4,                          // IO data offset
                           1,
                           (VOID *) (&Value)
                         );
  MemoryFence ();
  return;
}

/** This function calls the EFI PCI IO protocol to read a value from the device's PCI
   register space.

   @param[in]    Hw      Pointer to the shared code hw structure.
   @param[in]    Port    Which register to read from
   @param[out]   Value   Returns the value read from the PCI register.

   @return   Value from register read successfully
**/
VOID
e1000_read_pci_cfg (
  struct e1000_hw *Hw,
  UINT32           Port,
  UINT16 *         Value
  )
{
  DRIVER_DATA *AdapterInfo = Hw->back;

  if (IsSurpriseRemoval (AdapterInfo)) {
    return;
  }

  MemoryFence ();

  AdapterInfo->PciIo->Pci.Read (
                            AdapterInfo->PciIo,
                            EfiPciIoWidthUint16,
                            Port,
                            1,
                            (VOID *) Value
                          );
  MemoryFence ();
  return;
}

/** This function calls the EFI PCI IO protocol to write a value to the device's PCI
   register space.

   @param[in]   Hw      Pointer to the shared code hw structure.
   @param[in]   Port    Which register to write to.
   @param[out]  Value   Value to write to the PCI register.

   @return   Value written to register successfully
**/
VOID
e1000_write_pci_cfg (
  struct e1000_hw *Hw,
  UINT32           Port,
  UINT16 *         Value
  )
{
  DRIVER_DATA *AdapterInfo = Hw->back;

  if (IsSurpriseRemoval (AdapterInfo)) {
    return;
  }

  MemoryFence ();
  AdapterInfo->PciIo->Pci.Write (
                            AdapterInfo->PciIo,
                            EfiPciIoWidthUint16,
                            Port,
                            1,
                            (VOID *) Value
                          );
  MemoryFence ();

  return;
}

/** This function calls the EFI PCI IO protocol to write a value to the device's PCI
   register space, omitting IsSurpriseRemoval check.

   @param[in]   Hw      Pointer to the shared code hw structure.
   @param[in]   Port    Which register to write to.
   @param[out]  Value   Value to write to the PCI register.

   @return   Value written to register successfully
**/
VOID
e1000_write_pci_cfg_pb (
  struct e1000_hw *Hw,
  UINT32          Port,
  UINT16          *Value
  )
{
  DRIVER_DATA *AdapterInfo = Hw->back;

  MemoryFence ();
  AdapterInfo->PciIo->Pci.Write (
                            AdapterInfo->PciIo,
                            EfiPciIoWidthUint16,
                            Port,
                            1,
                            (VOID *) Value
                            );
  MemoryFence ();

  return;
}

/** Delay a specified number of microseconds.

   @param[in]   Hw   Pointer to hardware instance.
   @param[in]   usecs   Number of microseconds to delay

   @return   Execution of code delayed
**/
VOID
USecDelay (
  struct e1000_hw *Hw,
  UINTN            Usecs
  )
{
  DelayInMicroseconds (Hw->back, Usecs);
}

/** This function calls the MemIo callback to read a dword from the device's
   address space

   Since UNDI3.0 uses the TmpMemIo function (instead of the callback routine)
   which also takes the UniqueId parameter (as in UNDI3.1 spec) we don't have
   to make undi3.0 a special case

   @param[in]   Hw     Pointer to hardware instance.
   @param[in]   Port   Which Port to read from.

   @return   Results - The data read from the port.
**/
UINT32
E1000InDword (
  IN struct e1000_hw *Hw,
  IN UINT32           Port
  )
{
  UINT32       Results;
  DRIVER_DATA *AdapterInfo = Hw->back;

  if (IsSurpriseRemoval (AdapterInfo)) {
    return INVALID_STATUS_REGISTER_VALUE;
  }

  MemoryFence ();
  AdapterInfo->PciIo->Mem.Read (
                            AdapterInfo->PciIo,
                            EfiPciIoWidthUint32,
                            0,
                            Port,
                            1,
                            (VOID *) (&Results)
                          );
  MemoryFence ();

  return Results;
}

/** This function calls the MemIo callback to write a word from the device's
   address space

   Since UNDI3.0 uses the TmpMemIo function (instead of the callback routine)
   which also takes the UniqueId parameter (as in UNDI3.1 spec) we don't have
   to make undi3.0 a special case

   @param[in]   Hw   Pointer to hardware instance.
   @param[in]   Port   Which port to write to.
   @param[in]   Data   Data to write to Port.

   @return   Word written
**/
VOID
E1000OutDword (
  IN struct e1000_hw *Hw,
  IN UINT32           Port,
  IN UINT32           Data
  )
{
  UINT32       Value;
  DRIVER_DATA *AdapterInfo;

  AdapterInfo = Hw->back;
  Value = Data;

  if (IsSurpriseRemoval (AdapterInfo)) {
    return;
  }

  MemoryFence ();

  AdapterInfo->PciIo->Mem.Write (
                            AdapterInfo->PciIo,
                            EfiPciIoWidthUint32,
                            0,
                            Port,
                            1,
                            (VOID *) (&Value)
                          );

  MemoryFence ();

  return;
}

/** This function calls the MemIo callback to read a dword from the device's
   address space

   Since UNDI3.0 uses the TmpMemIo function (instead of the callback routine)
   which also takes the UniqueId parameter (as in UNDI3.1 spec) we don't have
   to make undi3.0 a special case

   @param[in]   Hw    Pointer to hardware instance.
   @param[in]   Port  Which port to read from.

   @return   Results - The data read from the port.
**/
UINT32
E1000FlashRead (
  IN struct e1000_hw *Hw,
  IN UINT32           Port
  )
{
  UINT32       Results;
  DRIVER_DATA *AdapterInfo = Hw->back;

  if (IsSurpriseRemoval (AdapterInfo)) {
    return INVALID_STATUS_REGISTER_VALUE;
  }

  MemoryFence ();
  AdapterInfo->PciIo->Mem.Read (
                            AdapterInfo->PciIo,
                            EfiPciIoWidthUint32,
                            1,
                            Port,
                            1,
                            (VOID *) (&Results)
                          );
  MemoryFence ();

  return Results;
}


/** This function calls the MemIo callback to read a dword from the device's
   address space

   Since UNDI3.0 uses the TmpMemIo function (instead of the callback routine)
   which also takes the UniqueId parameter (as in UNDI3.1 spec) we don't have
   to make undi3.0 a special case

   @param[in]   Hw   Pointer to hardware instance.
   @param[in]   Port   Which port to read from.

   @return   Results - The data read from the port.
**/
UINT16
E1000FlashRead16 (
  IN struct e1000_hw *Hw,
  IN UINT32           Port
  )
{
  UINT16       Results;
  DRIVER_DATA *AdapterInfo = Hw->back;

  if (IsSurpriseRemoval (AdapterInfo)) {
    return 0xFFFF;
  }

  MemoryFence ();
  AdapterInfo->PciIo->Mem.Read (
                            AdapterInfo->PciIo,
                            EfiPciIoWidthUint16,
                            1,
                            Port,
                            1,
                            (VOID *) (&Results)
                          );
  MemoryFence ();

  return Results;
}


/** This function calls the MemIo callback to write a word from the device's
   address space

   Since UNDI3.0 uses the TmpMemIo function (instead of the callback routine)
   which also takes the UniqueId parameter (as in UNDI3.1 spec) we don't have
   to make undi3.0 a special case

   @param[in]   Hw     Pointer to hardware instance.
   @param[in]   Port   Which port to write to.
   @param[in]   Data   Data to write to Port.

   @return   Word written
**/
VOID
E1000FlashWrite (
  IN struct e1000_hw *Hw,
  IN UINT32           Port,
  IN UINT32           Data
  )
{
  UINT32       Value;
  DRIVER_DATA *AdapterInfo;

  AdapterInfo = Hw->back;
  Value = Data;

  if (IsSurpriseRemoval (AdapterInfo)) {
    return;
  }

  MemoryFence ();

  AdapterInfo->PciIo->Mem.Write (
                            AdapterInfo->PciIo,
                            EfiPciIoWidthUint32,
                            1,
                            Port,
                            1,
                            (VOID *) (&Value)
                          );

  MemoryFence ();
  return;
}

/** This function calls the MemIo callback to write a word from the device's
   address space

   Since UNDI3.0 uses the TmpMemIo function (instead of the callback routine)
   which also takes the UniqueId parameter (as in UNDI3.1 spec) we don't have
   to make undi3.0 a special case

   @param[in]   Hw     Pointer to hardware instance.
   @param[in]   Port   Which port to write to.
   @param[in]   Data   Data to write to Port.

   @return   Word written
**/
VOID
E1000FlashWrite16 (
  IN struct e1000_hw *Hw,
  IN UINT32           Port,
  IN UINT16           Data
  )
{
  DRIVER_DATA *AdapterInfo = Hw->back;

  if (IsSurpriseRemoval (AdapterInfo)) {
    return;
  }

  MemoryFence ();

  AdapterInfo->PciIo->Mem.Write (
                            AdapterInfo->PciIo,
                            EfiPciIoWidthUint16,
                            1,
                            Port,
                            1,
                            (VOID *) (&Data)
                          );

  MemoryFence ();
}

/** Flushes a PCI write transaction to system memory.

   @param[in]   Hw   Pointer to hardware structure.

   @return  PCI write transaction flushed
**/
VOID
E1000PciFlush (
  IN struct e1000_hw *Hw
  )
{
  DRIVER_DATA *AdapterInfo = Hw->back;

  if (IsSurpriseRemoval (AdapterInfo)) {
    return;
  }

  MemoryFence ();

  AdapterInfo->PciIo->Flush (AdapterInfo->PciIo);

  MemoryFence ();
}

/** Sets the memory write and invalidate bit in the device's PCI command register.

   @param[in]   Hw   Pointer to the shared code hw structure.

   @retval   None
**/
VOID
e1000_pci_set_mwi (
  struct e1000_hw *Hw
  )
{
  DRIVER_DATA *AdapterInfo;
  UINT32       CommandReg;

  AdapterInfo = Hw->back;

  MemoryFence ();
  AdapterInfo->PciIo->Pci.Read (
                            AdapterInfo->PciIo,
                            EfiPciIoWidthUint16,
                            PCI_COMMAND,
                            1,
                            (VOID *) (&CommandReg)
                          );

  CommandReg |= PCI_COMMAND_MWI;

  AdapterInfo->PciIo->Pci.Write (
                            AdapterInfo->PciIo,
                            EfiPciIoWidthUint16,
                            PCI_COMMAND,
                            1,
                            (VOID *) (&CommandReg)
                          );
  MemoryFence ();
}

/** Clears the memory write and invalidate bit in the device's PCI command register.

   @param[in]   Hw   Pointer to the shared code hw structure.

   @retval   None
**/
VOID
e1000_pci_clear_mwi (
  struct e1000_hw *Hw
  )
{
  DRIVER_DATA *AdapterInfo;
  UINT32       CommandReg;

  AdapterInfo = Hw->back;

  AdapterInfo->PciIo->Pci.Read (
                            AdapterInfo->PciIo,
                            EfiPciIoWidthUint16,
                            PCI_COMMAND,
                            1,
                            (VOID *) (&CommandReg)
                          );

  CommandReg &= ~PCI_COMMAND_MWI;

  AdapterInfo->PciIo->Pci.Write (
                            AdapterInfo->PciIo,
                            EfiPciIoWidthUint16,
                            PCI_COMMAND,
                            1,
                            (VOID *) (&CommandReg)
                          );
}


