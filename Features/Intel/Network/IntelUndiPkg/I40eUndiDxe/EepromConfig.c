/**************************************************************************

Copyright (c) 2012 - 2022, Intel Corporation. All rights reserved.

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
#include "EepromConfig.h"
#include "wol.h"



#include "Hii/Hii.h"

/* Function definitions */

/** Write SR buffer using shared code implementation.

   @param[in]   AdapterInfo    Points to the driver information
   @param[in]   ModulePointer  Pointer to module in words with respect to NVM beginning
   @param[in]   Offset         offset in words from module start
   @param[in]   Words          Number of words to write
   @param[in]   Data           Pointer to location with data to be written

   @retval    EFI_SUCCESS        Buffer successfully written
   @retval    EFI_ACCESS_DENIED  Access to desired NVM memory range is denied
   @retval    EFI_DEVICE_ERROR   Failed to write buffer
   @retval    EFI_DEVICE_ERROR   Waiting for ARQ response timeout
**/
EFI_STATUS
I40eWriteNvmBuffer (
  IN DRIVER_DATA *AdapterInfo,
  IN UINT8        ModulePointer,
  IN UINT32       Offset,
  IN UINT16       Words,
  IN VOID        *Data
  )
{
  enum i40e_status_code      I40eStatus;
  struct i40e_arq_event_info EventInfo;
  UINT16                     Pending;
  UINT32                     WaitCnt;

  ZeroMem (&EventInfo, sizeof(EventInfo));

  i40e_acquire_nvm (&AdapterInfo->Hw, I40E_RESOURCE_WRITE);
  while (i40e_clean_arq_element (
           &AdapterInfo->Hw,
           &EventInfo,
           &Pending
         ) != I40E_ERR_ADMIN_QUEUE_NO_WORK)
  {
    ;
  }

  I40eStatus = __i40e_write_nvm_buffer (&AdapterInfo->Hw, ModulePointer, Offset, Words, Data);
  if (I40eStatus != I40E_SUCCESS) {
    enum i40e_admin_queue_err LastStatus;

    DEBUGPRINT (
      CRITICAL, ("__i40e_write_nvm_buffer (%d, %d) returned: %d, %d\n",
      Offset, Words, I40eStatus, AdapterInfo->Hw.aq.asq_last_status)
    );
    LastStatus = AdapterInfo->Hw.aq.asq_last_status;
    i40e_release_nvm (&AdapterInfo->Hw);
    if (LastStatus == I40E_AQ_RC_EPERM) {

      // Need to detect attempts to write RO area
      DEBUGPRINT (IMAGE, ("__i40e_write_nvm_buffer returned EPERM\n"));
      return EFI_ACCESS_DENIED;
    } else {
      return EFI_DEVICE_ERROR;
    }
  }

  WaitCnt = 0;
  do {
    I40eStatus = i40e_clean_arq_element (&AdapterInfo->Hw, &EventInfo, &Pending);
    if (I40eStatus == I40E_SUCCESS) {
      if (EventInfo.desc.opcode != i40e_aqc_opc_nvm_update) {
        I40eStatus = I40E_ERR_ADMIN_QUEUE_NO_WORK;
      } else {
        break;
      }
    }

    DelayInMicroseconds (AdapterInfo, 1000);
    WaitCnt++;
    if (WaitCnt > 1000) {
      i40e_release_nvm (&AdapterInfo->Hw);
      DEBUGPRINT (CRITICAL, ("Timeout waiting for ARQ response\n"));
      return EFI_DEVICE_ERROR;
    }

    // Wait until we get response to i40e_aqc_opc_nvm_update
  } while (I40eStatus != I40E_SUCCESS);
  i40e_release_nvm (&AdapterInfo->Hw);

  return EFI_SUCCESS;
}

/** Writes data buffer to nvm using __i40e_write_nvm_buffer shared code function.

   Function works around the situation when the buffer spreads over two sectors.
   The entire buffer must be located inside the Shared RAM.

   @param[in]   AdapterInfo   Points to the driver information
   @param[in]   Offset        Buffer offset from the start of NVM
   @param[in]   Words         Number of words to write
   @param[in]   Data          Pointer to location with data to be written

   @retval   EFI_SUCCESS       NVM buffer written successfully
   @retval   EFI_DEVICE_ERROR  Failed to write buffer (or either of the sectors)
**/
EFI_STATUS
I40eWriteNvmBufferExt (
  IN DRIVER_DATA *AdapterInfo,
  IN UINT32       Offset,
  IN UINT16       Words,
  IN VOID        *Data
  )
{
  UINT16     SectorStart;
  UINT16     Margin;
  UINT16     Words1;
  UINT16     Words2;
  EFI_STATUS Status;

  DEBUGFUNC ("__i40e_write_nvm_buffer");

  // Check if the buffer spreads over two sectors. Then we need to split
  // the buffer into two adjacent buffers, one for each sector and write them separatelly.
  SectorStart = (Offset / I40E_SR_SECTOR_SIZE_IN_WORDS) * I40E_SR_SECTOR_SIZE_IN_WORDS;
  Margin = (SectorStart + I40E_SR_SECTOR_SIZE_IN_WORDS) - Offset;
  if (Words > Margin) {
    Words1 = Margin;
    Words2 = Words - Margin;
  } else {
    Words1 = Words;
    Words2 = 0;
  }

  Status = I40eWriteNvmBuffer (AdapterInfo, 0, Offset, Words1, Data);
  if (EFI_ERROR (Status)) {
    DEBUGPRINT (CRITICAL, ("I40eWriteNvmBuffer returned %r\n", Status));
    return EFI_DEVICE_ERROR;
  }
  if (Words2 > 0) {

    // Write the remaining part of the input data to the second sector
    Status = I40eWriteNvmBuffer (
               AdapterInfo,
               0,
               SectorStart + I40E_SR_SECTOR_SIZE_IN_WORDS,
               Words2,
               (UINT16 *) Data + Words1
             );
    if (EFI_ERROR (Status)) {
      DEBUGPRINT (CRITICAL, ("I40eWriteNvmBuffer returned %r\n", Status));
      return EFI_DEVICE_ERROR;
    }
  }
  return EFI_SUCCESS;
}

/** Reads NVM Buffer to specified memory location

   @param[in]   AdapterInfo      Points to the driver information
   @param[in]   ModulePtr        Pointer to module in words with respect to NVM beginning
   @param[in]   Offset           offset in words from module start
   @param[in]   DataSizeInWords  Words to read from NVM
   @param[out]  DataPointer      Pointer to memory location where resulting buffer will
                                 be stored

   @retval      EFI_SUCCESS       Buffer successfully read
   @retval      EFI_NOT_FOUND     Pointer located under ModulePtr is not initialized
   @retval      EFI_DEVICE_ERROR  Failed to read NVM word with module pointer
   @retval      EFI_DEVICE_ERROR  Failed to read buffer from Shadow RAM
   @retval      EFI_DEVICE_ERROR  Failed to read buffer outside Shadow RAM
**/
EFI_STATUS
ReadDataFromNvmModule (
  IN  DRIVER_DATA *AdapterInfo,
  IN  UINT8        ModulePtr,
  IN  UINT32       Offset,
  IN  UINT16       DataSizeInWords,
  OUT UINT16      *DataPointer
  )
{
  enum i40e_status_code I40eStatus = I40E_SUCCESS;
  UINT16                PtrValue = 0;
  UINT32                FlatOffset;

  if (ModulePtr != 0) {
    I40eStatus = i40e_read_nvm_word (&AdapterInfo->Hw, ModulePtr, &PtrValue);
    if (I40eStatus != I40E_SUCCESS) {
      DEBUGPRINT (CRITICAL, ("i40e_read_nvm_word returned error\n"));
      return EFI_DEVICE_ERROR;
    }
  } else {

    // Reading from the flat memory
    PtrValue = 0;
  }

  // Pointer not initialized?
  if ((PtrValue == 0xFFFF)
    || (PtrValue == 0x7FFF))
  {
    return EFI_NOT_FOUND;
  }

  // Check whether the module is in SR mapped area or outside
  if ((PtrValue & 0x8000) == 0x8000) {

    // Pointer points outside of the Shared RAM mapped area
    PtrValue &= ~0x8000;

    // PtrValue in 4kB units, need to convert to words
    PtrValue /= 2;
    FlatOffset = ((UINT32) PtrValue * 0x1000) + (UINT32) Offset;
    I40eStatus = i40e_acquire_nvm (&AdapterInfo->Hw, I40E_RESOURCE_READ);
    if (I40eStatus == I40E_SUCCESS) {
      I40eStatus = i40e_aq_read_nvm (
                     &AdapterInfo->Hw,
                     0,
                     2 * FlatOffset,
                     2 * DataSizeInWords,
                     DataPointer,
                     TRUE,
                     NULL
                   );
      i40e_release_nvm (&AdapterInfo->Hw);
    }
    if (I40eStatus != I40E_SUCCESS) {
      DEBUGPRINT (
        CRITICAL, ("i40e_aq_read_nvm (%d, %d) returned: %d, %d\n",
        FlatOffset, DataSizeInWords, I40eStatus, AdapterInfo->Hw.aq.asq_last_status)
      );
      DEBUGPRINT (CRITICAL, ("i40e_read_nvm_aq returned %d\n", I40eStatus));
      return EFI_DEVICE_ERROR;
    }
  } else {

    // Read from the Shadow RAM
    I40eStatus = i40e_read_nvm_buffer (
                   &AdapterInfo->Hw,
                   PtrValue + Offset,
                   &DataSizeInWords,
                   DataPointer
                 );
    if (I40eStatus != I40E_SUCCESS) {
      DEBUGPRINT (CRITICAL, ("i40e_read_nvm_buffer returned %d\n", I40eStatus));
    }
  }
  if (I40eStatus != I40E_SUCCESS) {
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}

/** Retrieves pointer to NVM module location from specified location and
   writes given data to this module.

   @param[in]   AdapterInfo      Points to the driver information
   @param[in]   ModulePtr        Pointer to module in words with respect to NVM beginning
   @param[in]   Offset           Offset in words from module start
   @param[in]   DataSizeInWords  Number of words to write
   @param[in]   DataPointer      Pointer to buffer with data to write

   @retval      EFI_SUCCESS        Data successfully written to NVM module
   @retval      EFI_DEVICE_ERROR   Failed to read module pointer
   @retval      EFI_DEVICE_ERROR   Failed to write NVM buffer
   @retval      EFI_NOT_FOUND      Pointer location specified by ModulePtr is not
                                   initialized
   @retval      EFI_ACCESS_DENIED  Access to desired NVM memory range is denied
**/
EFI_STATUS
WriteDataToNvmModule (
  IN DRIVER_DATA *AdapterInfo,
  IN UINT8        ModulePtr,
  IN UINT16       Offset,
  IN UINT16       DataSizeInWords,
  IN UINT16      *DataPointer
  )
{
  enum i40e_status_code I40eStatus;
  UINT16                PtrValue = 0;

  if (ModulePtr != 0) {
    I40eStatus = i40e_read_nvm_word (&AdapterInfo->Hw, ModulePtr, &PtrValue);
    if (I40eStatus != I40E_SUCCESS) {
      return EFI_DEVICE_ERROR;
    }
  } else {

    // Reading from the flat memory
    PtrValue = 0;
  }

  // Pointer not initialized?
  if ((PtrValue == 0xFFFF)
    || (PtrValue == 0x7FFF))
  {
    return EFI_NOT_FOUND;
  }

  return I40eWriteNvmBuffer (AdapterInfo, 0, PtrValue + Offset, DataSizeInWords, DataPointer);
}

/** Reads SR buffer.

   @param[in]   UndiPrivateData  Points to the driver information.
   @param[in]   Offset           Offset in words from module start.
   @param[in]   Length           Number of words to read.
   @param[out]  Data             Pointer to location with data to be read to.

   @retval    EFI_SUCCESS    Buffer successfully read.
   @retval    !EFI_SUCCESS   Buffer not read, failure of underlying function.
**/
EFI_STATUS
ReadSrBuffer16 (
  IN  UNDI_PRIVATE_DATA  *UndiPrivateData,
  IN  UINT16             Offset,
  IN  UINT16             Length,
  OUT UINT16             *Data
  )
{
  return ReadDataFromNvmModule (&UndiPrivateData->NicInfo, 0, Offset, Length, Data);
}

/** Reads SR word.

   @param[in]   UndiPrivateData  Points to the driver information.
   @param[in]   Offset           Offset in words from module start.
   @param[out]  Data             Pointer to location with data to be read to.

   @retval    EFI_SUCCESS     Word successfully read.
   @retval    !EFI_SUCCESS    Word not read, failure of underlying function.
**/
EFI_STATUS
ReadSr16 (
  IN  UNDI_PRIVATE_DATA  *UndiPrivateData,
  IN  UINT16             Offset,
  OUT UINT16             *Data
  )
{
  return ReadSrBuffer16 (UndiPrivateData, Offset, 1, Data);
}

/** Writes SR buffer.

   @param[in]   UndiPrivateData  Points to the driver information.
   @param[in]   Offset           Offset in words from module start.
   @param[in]   Length           Number of words to write.
   @param[in]   Data             Pointer to location with words to be written.

   @retval    EFI_SUCCESS    Buffer successfully written.
   @retval    !EFI_SUCCESS   Buffer not written, failure of underlying function.
**/
EFI_STATUS
WriteSrBuffer16 (
  IN  UNDI_PRIVATE_DATA  *UndiPrivateData,
  IN  UINT16             Offset,
  IN  UINT16             Length,
  IN  UINT16             *Data
  )
{
  return I40eWriteNvmBufferExt (&UndiPrivateData->NicInfo, Offset, Length, Data);
}

/** Writes SR word.

   @param[in]   UndiPrivateData  Points to the driver information.
   @param[in]   Offset           Offset in words from module start.
   @param[in]   Data             Word to be written.

   @retval    EFI_SUCCESS    Word successfully written.
   @retval    !EFI_SUCCESS   Word not written, failure of underlying function.
**/
EFI_STATUS
WriteSr16 (
  IN  UNDI_PRIVATE_DATA  *UndiPrivateData,
  IN  UINT16             Offset,
  IN  UINT16             Data
  )
{
  return WriteSrBuffer16 (UndiPrivateData, Offset, 1, &Data);
}

/** Returns the offset in the NVM of the CSR register initialization data
   in the module pointed by the ModulePointer.

   Function looks for the register based on its address in CSR register space.

   @param[in]   AdapterInfo           Points to the driver information
   @param[in]   AutoGeneratedSection  Offset of module pointer in autogenerated
                                      pointer section
   @param[in]   AutoGeneratedOffset   Offset of module length pointer in
                                      autogenerated pointer section

   @return      offset of register data in NVM
   @return      0 when register is not found
**/
UINT16
GetRegisterInitializationDataOffset (
  IN DRIVER_DATA *AdapterInfo,
  IN UINT16       AutoGeneratedSection,
  IN UINT16       AutoGeneratedOffset
  )
{
  UINT16                AutoGeneratedSectionPointer;
  UINT16                ModuleOffset;
  UINT16                OffsetWithinModule;
  enum i40e_status_code I40eStatus;

  // Read pointer to the auto generated pointers section
  I40eStatus = i40e_read_nvm_word (
                 &AdapterInfo->Hw,
                 I40E_SR_AUTO_GENERATED_POINTERS_PTR,
                 &AutoGeneratedSectionPointer
               );
  if (I40eStatus != I40E_SUCCESS) {
    return 0;
  }

  // Read Nvm Module offset in SR
  I40eStatus = i40e_read_nvm_word (
                 &AdapterInfo->Hw,
                 AutoGeneratedSectionPointer + AutoGeneratedSection,
                 &ModuleOffset
               );
  if (I40eStatus != I40E_SUCCESS) {
    return 0;
  }

  // Read the length of the module
  I40eStatus = i40e_read_nvm_word (
                 &AdapterInfo->Hw,
                 AutoGeneratedSectionPointer + AutoGeneratedOffset,
                 &OffsetWithinModule
               );
  if (I40eStatus != I40E_SUCCESS) {
    return 0;
  }

  if ((ModuleOffset == 0x7FFF)
    || (ModuleOffset == 0xFFFF))
  {
    return 0;
  }

  if (OffsetWithinModule == 0x0000) {
    return 0;
  }

  return ModuleOffset + OffsetWithinModule;
}

/** Notify FW of change in alternate RAM structures.

   @param[in]   UndiPrivateData   Pointer to driver private data structure

   @retval   EFI_SUCCESS       Successfully notified of changes
   @retval   EFI_DEVICE_ERROR  Failed to notify
**/
EFI_STATUS
ApplyAlternateSettings (
  IN  UNDI_PRIVATE_DATA *UndiPrivateData
  )
{
  enum i40e_status_code I40eStatus;
  BOOLEAN               ResetNeeded;

  I40eStatus = i40e_aq_alternate_write_done (&UndiPrivateData->NicInfo.Hw, 0, &ResetNeeded);
  IF_SCERR_RETURN (I40eStatus, EFI_DEVICE_ERROR);

  return EFI_SUCCESS;
}

/** Reads factory default MAC address for specified PF.

   @param[in]   UndiPrivateData      Pointer to driver private data structure
   @param[in]   PhysicalFunction     Number of PF to read the MAC Addresses of
   @param[out]  FactoryMacAddress    Factory default MAC address of the adapter

   @retval      EFI_SUCCESS    MAC addresses read successfully
   @retval      !EFI_SUCCESS   Failed to read MAC address pointer or value
**/
EFI_STATUS
GetFactoryMacAddressForPf (
  IN  UNDI_PRIVATE_DATA  *UndiPrivateData,
  IN  UINT8              PhysicalFunction,
  OUT UINT8              *FactoryMacAddress
  )
{
  EFI_STATUS             Status;
  UINT16                PfMacAddressesPointer;

  Status = ReadDataFromNvmModule (
             &UndiPrivateData->NicInfo,
             NVM_EMP_SR_SETTINGS_MODULE_PTR,
             NVM_EMP_SR_PF_MAC_PTR,
             1,
             &PfMacAddressesPointer
             );
  IF_RETURN (EFI_ERROR (Status), Status);

  // Read from the PF MAC Address subsection of the EMP SR Settings module
  Status = ReadDataFromNvmModule (
             &UndiPrivateData->NicInfo,
             NVM_EMP_SR_SETTINGS_MODULE_PTR,
             PfMacAddressesPointer + NVM_EMP_SR_PF_MAC_PTR +
             NVM_EMP_SR_PF_MAC_SAL0 (PhysicalFunction),
             ETH_ALEN / sizeof (UINT16),
             (UINT16 *) FactoryMacAddress
             );
  IF_RETURN (EFI_ERROR (Status), Status);

  return EFI_SUCCESS;
}

/** Gets factory MAC addresses for PF0.

   @param[in]   UndiPrivateData      Pointer to driver private data structure
   @param[out]  FactoryMacAddress    Pointer to buffer for resulting factory
                                     MAC address

   @retval      EFI_SUCCESS       MAC addresses read successfully
   @retval      !EFI_SUCCESS      Failure of underlying function
**/
EFI_STATUS
GetFactoryMacAddressForPf0 (
  IN   UNDI_PRIVATE_DATA  *UndiPrivateData,
  OUT  UINT8              *FactoryMacAddress
  )
{
  return GetFactoryMacAddressForPf (UndiPrivateData, 0, FactoryMacAddress);
}

/** Gets factory MAC address for specified partition.

   @param[in]   UndiPrivateData      Pointer to driver private data structure
   @param[out]  FactoryMacAddress    Pointer to buffer for resulting factory
                                     MAC address
   @param[in]   Partition            Partition index

   @retval      EFI_SUCCESS       MAC addresses read successfully
   @retval      EFI_NOT_FOUND     Partition number exceeds maximum supported on port
   @retval      !EFI_SUCCESS      Failed to read factory MAC address
**/
EFI_STATUS
GetPartitionFactoryMacAddress (
  IN  UNDI_PRIVATE_DATA  *UndiPrivateData,
  OUT UINT8              *FactoryMacAddress,
  IN  UINT8              Partition
  )
{
  // Check if the partition wih this number exists
  // Partition numbers are 0 - based internally
  IF_RETURN (Partition >= UndiPrivateData->NicInfo.PfPerPortMaxNumber, EFI_NOT_FOUND);

  return GetFactoryMacAddressForPf (
           UndiPrivateData,
           UndiPrivateData->NicInfo.PartitionPfNumber[Partition],
           FactoryMacAddress
           );
}

/** Gets alternate MAC address for specified partition.

   @param[in]   UndiPrivateData      Pointer to driver private data structure
   @param[out]  AlternateMacAddress  Pointer to buffer for resulting alternate
                                     MAC address
   @param[in]   Partition            Partition index

   @retval      EFI_SUCCESS       MAC addresses read successfully
   @retval      EFI_NOT_FOUND     Partition number exceeds maximum supported on port
   @retval      EFI_DEVICE_ERROR  Failed to read alternate MAC addr from Alt. RAM
**/
EFI_STATUS
GetPartitionAlternateMacAddress (
  IN  UNDI_PRIVATE_DATA  *UndiPrivateData,
  OUT UINT8              *AlternateMacAddress,
  IN  UINT8              Partition
  )
{
  enum i40e_status_code I40eStatus;
  UINT32                AltRamBuffer[2];

  // Check if the partition wih this number exsist
  // Partition numbers are 0 - based internally
  IF_RETURN (Partition >= UndiPrivateData->NicInfo.PfPerPortMaxNumber, EFI_NOT_FOUND);

  // In Fortville Alternate MAC Address is stored in Alternate RAM
  I40eStatus = i40e_aq_alternate_read_indirect (
                 &UndiPrivateData->NicInfo.Hw,
                 I40E_ALT_RAM_LAN_MAC_ADDRESS_LOW (UndiPrivateData->NicInfo.PartitionPfNumber[Partition]),
                 2,
                 AltRamBuffer
                 );
  IF_SCERR_RETURN (I40eStatus, EFI_DEVICE_ERROR);

  // Check if this Alternate RAM entry is valid and then use it,
  // otherwise return zeros
  if ((AltRamBuffer[1] & ALT_RAM_VALID_PARAM_BIT_MASK) != 0) {
    ((UINT16 *)AlternateMacAddress)[0] = SwapBytes16 ((UINT16) (AltRamBuffer[1] & 0x0000FFFF));
    ((UINT16 *)AlternateMacAddress)[1] = SwapBytes16 ((UINT16) ((AltRamBuffer[0] & 0xFFFF0000) >> 16));
    ((UINT16 *)AlternateMacAddress)[2] = SwapBytes16 ((UINT16) (AltRamBuffer[0] & 0x0000FFFF));
  } else {
    ZeroMem (AlternateMacAddress, ETH_ALEN);
  }

  return EFI_SUCCESS;
}

/** Sets alternate MAC address for specified partition.

   @param[in]   UndiPrivateData      Pointer to driver private data structure
   @param[in]   AlternateMacAddress  Value to set MAC address to
   @param[in]   Partition            Partition index

   @retval      EFI_SUCCESS       New MAC address set successfully
   @retval      EFI_SUCCESS       Setting unsupported by SW
   @retval      EFI_DEVICE_ERROR  Failed to write new MAC value to Alt. RAM
**/
EFI_STATUS
SetPartitionAlternateMacAddress (
  IN UNDI_PRIVATE_DATA  *UndiPrivateData,
  IN UINT8              *AlternateMacAddress,
  IN UINT8              Partition
  )
{
  EFI_STATUS             Status;
  enum i40e_status_code  I40eStatus;
  UINT32                 AltRamBuffer[2];

  // Now that we have Factory MAC Address read, look for the Alternate MAC Address
  // In Fortville it is stored in Alternate RAM
  // Prepare Alternate RAM entry structure

  AltRamBuffer[0] = SwapBytes16 (((UINT16 *)AlternateMacAddress)[2]) +
                    ((UINT32) SwapBytes16 (((UINT16 *)AlternateMacAddress)[1]) << 16);
  AltRamBuffer[1] = SwapBytes16 (((UINT16 *)AlternateMacAddress)[0]);

  AltRamBuffer[1] |= ALT_RAM_VALID_PARAM_BIT_MASK;

  I40eStatus = i40e_aq_alternate_write_indirect (
                 &UndiPrivateData->NicInfo.Hw,
                 I40E_ALT_RAM_LAN_MAC_ADDRESS_LOW (UndiPrivateData->NicInfo.PartitionPfNumber[Partition]),
                 2,
                 AltRamBuffer
                 );
  IF_SCERR_RETURN (I40eStatus, EFI_DEVICE_ERROR);

  Status = ApplyAlternateSettings (UndiPrivateData);
  IF_RETURN (EFI_ERROR (Status), Status);

  return EFI_SUCCESS;
}

/** Restores the factory default MAC address for partition for Alt MAC address.

   @param[in]   UndiPrivateData  Pointer to driver private data structure
   @param[in]   Partition        Partition index

   @retval      EFI_SUCCESS       MAC address restored successfully
   @retval      EFI_SUCCESS       Setting unsupported by SW
   @retval      EFI_DEVICE_ERROR  Failed to invalidate Alt. RAM entry
**/
EFI_STATUS
RestorePartitionDefaultAltMacAddress (
  IN  UNDI_PRIVATE_DATA  *UndiPrivateData,
  IN  UINT8              Partition
  )
{
  EFI_STATUS             Status;
  enum i40e_status_code  I40eStatus;
  UINT32                 AltRamBuffer[2];


  // Invalidate Alternate RAM entry by writing zeros
  AltRamBuffer[0] = 0;
  AltRamBuffer[1] = 0;

  I40eStatus = i40e_aq_alternate_write_indirect (
                 &UndiPrivateData->NicInfo.Hw,
                 I40E_ALT_RAM_LAN_MAC_ADDRESS_LOW (UndiPrivateData->NicInfo.PartitionPfNumber[Partition]),
                 2,
                 AltRamBuffer
                 );
  IF_SCERR_RETURN (I40eStatus, EFI_DEVICE_ERROR);

  Status = ApplyAlternateSettings (UndiPrivateData);
  IF_RETURN (EFI_ERROR (Status), Status);

  return EFI_SUCCESS;
}

/** Reads factory default MAC address.

   @param[in]   UndiPrivateData      Pointer to driver private data structure
   @param[out]  FactoryMacAddress    Factory default MAC address of the adapter

   @retval      EFI_SUCCESS       MAC addresses read successfully
   @retval      !EFI_SUCCESS      Failure of underlying function
**/
EFI_STATUS
GetFactoryMacAddress (
  IN  UNDI_PRIVATE_DATA  *UndiPrivateData,
  OUT UINT8              *FactoryMacAddress
  )
{
  return GetPartitionFactoryMacAddress (UndiPrivateData, FactoryMacAddress, 0);
}

/** Gets alternate MAC address of currently managed PF.

   @param[in]   UndiPrivateData      Pointer to driver private data structure
   @param[out]  AlternateMacAddress  Pointer to buffer for resulting alternate
                                     MAC address

   @retval      EFI_SUCCESS       MAC addresses read successfully
   @retval      !EFI_SUCCESS      Failure of underlying function
**/
EFI_STATUS
GetAlternateMacAddress (
  IN   UNDI_PRIVATE_DATA  *UndiPrivateData,
  OUT  UINT8              *AlternateMacAddress
  )
{
  return GetPartitionAlternateMacAddress (UndiPrivateData, AlternateMacAddress, 0);
}


/** Sets alternate MAC address for currently managed PF.

   @param[in]   UndiPrivateData      Pointer to driver private data structure
   @param[in]   AlternateMacAddress  Value to set the MAC address to

   @retval      EFI_SUCCESS          New MAC address set successfully
   @retval      !EFI_SUCCESS         Failure to set alternate MAC address
**/
EFI_STATUS
SetAlternateMacAddress (
  IN  UNDI_PRIVATE_DATA  *UndiPrivateData,
  IN  UINT8              *AlternateMacAddress
  )
{
  // Call SetPartitionAlternateMacAddress with partition number 0
  return SetPartitionAlternateMacAddress (UndiPrivateData, AlternateMacAddress, 0);
}

/** Restores the factory default MAC address for currently managed PF.

   @param[in]   UndiPrivateData   Pointer to driver private data structure

   @retval      EFI_SUCCESS       New MAC address set successfully
   @retval      !EFI_SUCCESS      Failed to restore factory default MAC address
**/
EFI_STATUS
RestoreDefaultMacAddress (
  IN  UNDI_PRIVATE_DATA  *UndiPrivateData
  )
{
  // Call RestorePartitionDefaultAltMacAddress with partition number 0
  return RestorePartitionDefaultAltMacAddress (UndiPrivateData, 0);
}

/** Returns Flat NVM status.

   @param[in]   UndiPrivateData    Points to the driver instance private data
   @param[out]  IsFlatNvm          FlatNvm Status:
                                   0 - Structured NVM or N/A (FVL)
                                   1 - FPK Flat NVM

   @retval   EFI_SUCCESS       Ctrl word successfully read
   @retval   EFI_DEVICE_ERROR  Failed to read ctrl word
**/
EFI_STATUS
EepromIsFlatNvm (
  IN  UNDI_PRIVATE_DATA *UndiPrivateData,
  OUT UINT8             *IsFlatNvm
  )
{
  enum i40e_status_code I40eStatus;
  UINT16                Word = 0;

  // Set default value (0)
  *IsFlatNvm = 0;

  // Only FPK supports Flat NVM
  if (UndiPrivateData->NicInfo.Hw.mac.type != I40E_MAC_X722) {
    return EFI_SUCCESS;
  }

  // Read FPK Ctrl Word 1
  I40eStatus = i40e_read_nvm_word (
                 &UndiPrivateData->NicInfo.Hw,
                 EEPROM_FPK_CTRL_WORD_1,
                 &Word
               );
  if (I40eStatus != I40E_SUCCESS) {
    return EFI_DEVICE_ERROR;
  }

  // Check Flat Map Bit
  if ((Word & EEPROM_FPK_CTRL_WORD_1_FLAT_MAP_MASK) != 0) {
    *IsFlatNvm = 1;
  }

  return EFI_SUCCESS;
}

/** Returns EEPROM capabilities word (0x33) for current adapter

   @param[in]   UndiPrivateData    Points to the driver instance private data
   @param[out]  CapabilitiesWord   EEPROM capabilities word (0x33) for current adapter

   @retval   EFI_SUCCESS       Capabilities word successfully read
   @retval   EFI_DEVICE_ERROR  Failed to read capabilities word
**/
EFI_STATUS
EepromGetCapabilitiesWord (
  IN  UNDI_PRIVATE_DATA *UndiPrivateData,
  OUT UINT16            *CapabilitiesWord
  )
{
  enum i40e_status_code I40eStatus;
  UINT16                Word;

  I40eStatus = i40e_read_nvm_word (
                 &UndiPrivateData->NicInfo.Hw,
                 EEPROM_CAPABILITIES_WORD,
                 &Word
               );
  if (I40eStatus != I40E_SUCCESS) {
    return EFI_DEVICE_ERROR;
  }

  Word &= ~EEPROM_CAPABILITIES_SIG;
  *CapabilitiesWord = Word;

  return EFI_SUCCESS;
}

/** Checks if Phy Family Id is supported by Device

   @param[in]  UndiPrivateData    Points to the driver instance private data
   @param[in]  PhyFamilyId        PhyFamilyId

   @retval   EFI_SUCCESS        Phy Family Id is supported
   @retval   EFI_UNSUPPORTED  Phy Family Id is not supported
   @retval   EFI_DEVICE_ERROR   Failed to read capabilities word
**/
EFI_STATUS
EepromCheckPhyFamilyId (
  IN  UNDI_PRIVATE_DATA *UndiPrivateData,
  IN  UINT16            PhyFamilyId
  )
{
  enum i40e_status_code I40eStatus;
  UINT16                Word;

  I40eStatus = i40e_read_nvm_word (
                 &UndiPrivateData->NicInfo.Hw,
                 EEPROM_PHY_FAMILY_ID_WORD,
                 &Word
               );
  if (I40eStatus != I40E_SUCCESS) {
    return EFI_DEVICE_ERROR;
  }

  if (Word != 0xFFFF &&        // ignore 0xFFFF from EEPROM
      PhyFamilyId != 0xFFFF && // ignore 0xFFFF
      PhyFamilyId != 0x0000 && // ignore NULL
      PhyFamilyId != 0x0001 && // ignore 0x1
      PhyFamilyId != Word) {
     return EFI_UNSUPPORTED;
  }

  return EFI_SUCCESS;
}

/** Returns NVM version

   @param[in]   UndiPrivateData  Points to the driver instance private data
   @param[out]  MajorVer         NVM major version
   @param[out]  MinorVer         NVM minor version

   @retval   EFI_SUCCESS       NVM version read successfully
   @retval   EFI_DEVICE_ERROR  Failed to read EEPROM version word
**/
EFI_STATUS
EepromGetNvmVersion (
  IN  UNDI_PRIVATE_DATA *UndiPrivateData,
  OUT UINT16            *MajorVer,
  OUT UINT16            *MinorVer
  )
{
  enum i40e_status_code I40eStatus;
  UINT16                Word;

  I40eStatus = i40e_read_nvm_word (
                 &UndiPrivateData->NicInfo.Hw,
                 EEPROM_VERSION_WORD,
                 &Word
               );
  if (I40eStatus != I40E_SUCCESS) {
    return EFI_DEVICE_ERROR;
  }

  *MajorVer = (Word >> 12);
  *MinorVer = (Word & 0xFF);

  return EFI_SUCCESS;
}

/** Gets total number of ports

   @param[in]   UndiPrivateData  Pointer to driver private data structure

   @return      Number of ports
**/


/** Gets maximum number of PFs per port.

   @param[in]   AdapterInfo      Points to the driver information
   @param[out]  PfPerPortNumber  Max. number of PFs per port

   @retval      EFI_SUCCESS       PFs per port number successfully returned
   @retval      EFI_DEVICE_ERROR  Failed to read NVM word with max PF per port number
   @retval      EFI_DEVICE_ERROR  Invalid configuration read from NVM
**/
EFI_STATUS
GetMaxPfPerPortNumber (
  IN  DRIVER_DATA *AdapterInfo,
  OUT UINT8       *PfPerPortNumber
  )
{
  EFI_STATUS Status;
  UINT16     DataWord;

  // This parameter is stored in EMP Settings module in Shadow RAM
  Status = ReadDataFromNvmModule (
             AdapterInfo,
             NVM_EMP_SR_SETTINGS_MODULE_PTR,
             NVM_EMP_SR_MAX_PF_VF_PER_PORT_WORD_OFFSET,
             1,
             &DataWord
           );
  IF_RETURN (EFI_ERROR (Status), Status);

  DataWord &= NVM_EMP_SR_MAX_PF_PER_PORT_MASK;
  DataWord <<= NVM_EMP_SR_MAX_PF_PER_PORT_OFFSET;

  // Max number of pfs per port is not written directly, need to do conversion
  switch (DataWord) {
  case NVM_1_PF_PER_PORT:
    *PfPerPortNumber = 1;
    break;
  case NVM_2_PF_PER_PORT:
    *PfPerPortNumber = 2;
    break;
  case NVM_4_PF_PER_PORT:
    *PfPerPortNumber = 4;
    break;
  case NVM_8_PF_PER_PORT:
    *PfPerPortNumber = 8;
    break;
  case NVM_16_PF_PER_PORT:
    *PfPerPortNumber = 16;
    break;
  default:

    // Any other values are reserved so return error status
    return EFI_DEVICE_ERROR;
    break;
  }

  return EFI_SUCCESS;
}

/** Checks if is LOM device

   @param[in]   UndiPrivateData   Pointer to driver private data structure

   @retval      FALSE   Is not LOM device (always returned)
**/
BOOLEAN
EepromIsLomDevice (
  IN  UNDI_PRIVATE_DATA *UndiPrivateData
  )
{
  return FALSE;
}

/** Updates NVM checksum.

   @param[in]   UndiPrivateData   Pointer to driver private data structure

   @retval      EFI_SUCCESS       Checksum successfully updated
   @retval      EFI_DEVICE_ERROR  Failed to update NVM checksum
**/
EFI_STATUS
UpdateNvmChecksum (
  IN  UNDI_PRIVATE_DATA *UndiPrivateData
  )
{
  enum i40e_status_code      I40eStatus;
  struct i40e_arq_event_info EventInfo;
  UINT16                     Pending;
  UINT16                     ChecksumBeforeUpdate;
  UINT16                     ChecksumAfterUpdate;

  ZeroMem (&EventInfo, sizeof(EventInfo));

  i40e_acquire_nvm (&UndiPrivateData->NicInfo.Hw, I40E_RESOURCE_WRITE);
  while (i40e_clean_arq_element (
           &UndiPrivateData->NicInfo.Hw,
           &EventInfo,
           &Pending
         ) != I40E_ERR_ADMIN_QUEUE_NO_WORK)
  {
    ;
  }

  i40e_calc_nvm_checksum (&UndiPrivateData->NicInfo.Hw, &ChecksumBeforeUpdate);
  if (i40e_update_nvm_checksum (&UndiPrivateData->NicInfo.Hw) != I40E_SUCCESS) {
    i40e_release_nvm (&UndiPrivateData->NicInfo.Hw);
    return EFI_DEVICE_ERROR;
  }

  do {
    I40eStatus = i40e_clean_arq_element (&UndiPrivateData->NicInfo.Hw, &EventInfo, &Pending);
    if ((I40eStatus == I40E_SUCCESS)
      && (EventInfo.desc.opcode != i40e_aqc_opc_nvm_update))
    {
      I40eStatus = I40E_ERR_ADMIN_QUEUE_NO_WORK;
    }

    // Wait until we get response to i40e_aqc_opc_nvm_update
  } while (I40eStatus != I40E_SUCCESS);
  i40e_calc_nvm_checksum (&UndiPrivateData->NicInfo.Hw, &ChecksumAfterUpdate);

  if (ChecksumBeforeUpdate != ChecksumAfterUpdate) {

    // update checksum again
    if (i40e_update_nvm_checksum (&UndiPrivateData->NicInfo.Hw) != I40E_SUCCESS) {
      i40e_release_nvm (&UndiPrivateData->NicInfo.Hw);
      return EFI_DEVICE_ERROR;
    }

    do {
      I40eStatus = i40e_clean_arq_element (&UndiPrivateData->NicInfo.Hw, &EventInfo, &Pending);
      if ((I40eStatus == I40E_SUCCESS)
        && (EventInfo.desc.opcode != i40e_aqc_opc_nvm_update))
      {
        I40eStatus = I40E_ERR_ADMIN_QUEUE_NO_WORK;
      }
      // Wait until we get response to i40e_aqc_opc_nvm_update
    } while (I40eStatus != I40E_SUCCESS);
  }
  i40e_release_nvm (&UndiPrivateData->NicInfo.Hw);

  return EFI_SUCCESS;
}

#define PBA_NUMBER_MAX_CHARS 20 // normally 11 (10+null) but add some buffer

/** Reads PBA string from NVM.

   @param[in]   UndiPrivateData  Pointer to driver private data structure
   @param[out]  PbaNumberStr     Output string buffer for PBA string

   @retval   EFI_SUCCESS            PBA string successfully read
   @retval   EFI_SUCCESS            PBA string unsupported by adapter
   @retval   EFI_DEVICE_ERROR       Failed to read PBA flags word
   @retval   EFI_DEVICE_ERROR       Failed to read PBA module pointer
   @retval   EFI_DEVICE_ERROR       Failed to read PBA length
   @retval   EFI_DEVICE_ERROR       PBA length read from NVM won't fit buffer
   @retval   EFI_DEVICE_ERROR       Failed to read PBA number word
**/
EFI_STATUS
GetPbaStr (
  IN  UNDI_PRIVATE_DATA  *UndiPrivateData,
  OUT EFI_STRING         PbaNumberStr
  )
{
  UINT16                PbaFlags;
  UINT16                Data;
  UINT16                PbaPtr;
  UINT16                Offset;
  UINT16                Length;
  enum i40e_status_code I40eStatus;
  CHAR8                 PbaNumber[PBA_NUMBER_MAX_CHARS];

  I40eStatus = i40e_read_nvm_word (
                 &UndiPrivateData->NicInfo.Hw,
                 NVM_PBA_FLAGS_OFFSET,
                 &PbaFlags
                 );
  IF_SCERR_RETURN (I40eStatus, EFI_DEVICE_ERROR);

  I40eStatus = i40e_read_nvm_word (
                 &UndiPrivateData->NicInfo.Hw,
                 NVM_PBA_BLOCK_MODULE_POINTER,
                 &PbaPtr
                 );
  IF_SCERR_RETURN (I40eStatus, EFI_DEVICE_ERROR);

  // If PbaFlags is not 0xFAFA the PBA must be in legacy format which
  // means PbaPtr is actually our second data word for the PBA number
  // and we can decode it into an ascii string
  if (PbaFlags != NVM_PBA_FLAG_STRING_MODE) {

    // Extract hex string from data and pba_ptr
    PbaNumber[0] = (PbaFlags >> 12) & 0xF;
    PbaNumber[1] = (PbaFlags >> 8) & 0xF;
    PbaNumber[2] = (PbaFlags >> 4) & 0xF;
    PbaNumber[3] = PbaFlags & 0xF;
    PbaNumber[4] = (PbaPtr >> 12) & 0xF;
    PbaNumber[5] = (PbaPtr >> 8) & 0xF;
    PbaNumber[6] = '-';
    PbaNumber[7] = 0;
    PbaNumber[8] = (PbaPtr >> 4) & 0xF;
    PbaNumber[9] = PbaPtr & 0xF;

    // Put a null character on the end of our string
    PbaNumber[10] = '\0';

    // switch all the data but the '-' to hex char
    for (Offset = 0; Offset < 10; Offset++) {
      if (PbaNumber[Offset] < 0xA) {
        PbaNumber[Offset] += '0';
      } else if (PbaNumber[Offset] < 0x10) {
        PbaNumber[Offset] += 'A' - 0xA;
      }
    }

    goto ConvertAsciiPbaToUni;
  }

  I40eStatus = i40e_read_nvm_word (
                 &UndiPrivateData->NicInfo.Hw,
                 PbaPtr,
                 &Length
                 );
  IF_SCERR_RETURN (I40eStatus, EFI_DEVICE_ERROR);

  if ((Length == 0xFFFF)
    || (Length == 0))
  { // Fill PBA String with "N/A" in case it's not in NVM
    StrCpyS (PbaNumberStr, HII_MAX_STR_LEN, L"N/A");
    return EFI_SUCCESS;
  }

  // trim pba length from start of string
  PbaPtr++;
  Length--;

  IF_RETURN ((Length * 2) > (PBA_NUMBER_MAX_CHARS - 1), EFI_DEVICE_ERROR);

  for (Offset = 0; Offset < Length; Offset++) {
    I40eStatus = i40e_read_nvm_word (
                   &UndiPrivateData->NicInfo.Hw,
                   PbaPtr + Offset,
                   &Data
                   );
    IF_SCERR_RETURN (I40eStatus, EFI_DEVICE_ERROR);

    PbaNumber[Offset * 2] = (UINT8) (Data >> 8);
    PbaNumber[(Offset * 2) + 1] = (UINT8) (Data & 0xFF);
  }
  PbaNumber[Offset * 2] = '\0';

ConvertAsciiPbaToUni:
  UnicodeSPrint (PbaNumberStr, HII_MAX_STR_LEN, L"%a", PbaNumber);
  return EFI_SUCCESS;
}

/** Reads port numbers

   @param[in]   AdapterInfo   Points to the driver information
   @param[out]  PortNumbers   Pointer to buffer for resultant port numberes
   @param[in]   ArraySize     Size of buffer with port numbers

   @retval   EFI_SUCCESS        Port numbers successfully read
   @retval   EFI_DEVICE_ERROR   Failed to get port numbers section address with
                                offset
   @retval   EFI_DEVICE_ERROR   Failed to read port number value
**/
EFI_STATUS
EepromReadPortnumValues (
  IN  DRIVER_DATA *AdapterInfo,
  OUT UINT16      *PortNumbers,
  IN  UINT16       ArraySize
  )
{
  UINT16                RegAddress;
  UINTN                 i;
  enum i40e_status_code I40eStatus;

  RegAddress = GetRegisterInitializationDataOffset (
                 AdapterInfo,
                 I40E_AUTOGEN_PTR_PFGEN_PORTNUM_SECTION,
                 I40E_AUTOGEN_PTR_PFGEN_PORTNUM_OFFSET
               );
  if (RegAddress == 0) {
    return EFI_DEVICE_ERROR;
  }

  for (i = 0; i < ArraySize; i++) {
    I40eStatus = i40e_read_nvm_word (
                   &AdapterInfo->Hw,
                   RegAddress + i * 2,
                   &PortNumbers[i]
                 );
    if (I40eStatus != I40E_SUCCESS) {
      return EFI_DEVICE_ERROR;
    }
  }
  return EFI_SUCCESS;
}


/** Checks if FW LLDP Agent status is supported.

   @param[in]   UndiPrivateData  Pointer to driver private data structure
   @param[out]  Supported        Tells whether LLDP Agent is supported

   @retval      EFI_SUCCESS  LLDP Agent support information retrieved.
**/
EFI_STATUS
IsLldpAgentSupported (
  IN   UNDI_PRIVATE_DATA  *UndiPrivateData,
  OUT  BOOLEAN            *Supported
  )
{
  *Supported = ((UndiPrivateData->NicInfo.Hw.flags & I40E_HW_FLAG_FW_LLDP_PERSISTENT) != 0);
  return EFI_SUCCESS;
}

/** Reads current FW LLDP Agent status.

   @param[in]   UndiPrivateData  Pointer to driver private data structure
   @param[out]  LldpAgentEna     Pointer to variable which will store read LLDP Admin status

   @retval      EFI_SUCCESS       LLDP Agent status read successfully
   @retval      EFI_DEVICE_ERROR  Failed to read LLDP Agent status
   @retval      EFI_DEVICE_ERROR  Out of range value read from NVM
**/
EFI_STATUS
GetLldpAgentStatus (
  IN   UNDI_PRIVATE_DATA  *UndiPrivateData,
  OUT  BOOLEAN            *LldpAgentEna
  )
{
  enum i40e_status_code I40eStatus;
  UINT16                Offset = 0;
  UINT16                SettingOffset;
  UINT16                LldpAdminStatusWord;
  UINT8                 LldpAdminStatus;

  //For FVL and FPK in EMP SR Settings there are two different offsets
  if (UndiPrivateData->NicInfo.Hw.mac.type != I40E_MAC_X722) {
    Offset = NVM_EMP_SR_CURRENT_SETTING_PTR_FVL;
  } else {
    Offset = NVM_EMP_SR_CURRENT_SETTING_PTR_FPK;
  }

  //First we need to get an offset to Current Setting Section of LLDP
  I40eStatus = ReadDataFromNvmModule (
                 &UndiPrivateData->NicInfo,
                 NVM_EMP_SR_SETTINGS_MODULE_PTR,
                 Offset,
                 1,
                 &SettingOffset
                 );
  IF_SCERR_RETURN (I40eStatus, EFI_DEVICE_ERROR);

  //Now we can read actual value of LLDP which also has an offset of one byte
  I40eStatus = ReadDataFromNvmModule (
                 &UndiPrivateData->NicInfo,
                 NVM_EMP_SR_SETTINGS_MODULE_PTR,
                 Offset + SettingOffset + CURRENT_SETTINGS_LLDP_ADMIN_STATUS,
                 1,
                 &LldpAdminStatusWord
                 );
  IF_SCERR_RETURN (I40eStatus, EFI_DEVICE_ERROR);

  LldpAdminStatus = (LldpAdminStatusWord >> (4 * UndiPrivateData->NicInfo.PhysicalPortNumber)) & 0xF;

  // If LLDP Admin Status is invalid == 0xF then read default value
  if (LldpAdminStatus == 0xF) {
    return GetDfltLldpAgentStatus (UndiPrivateData, LldpAgentEna);
  } else if (LldpAdminStatus > 3) {
    DEBUGPRINT (CRITICAL, ("Wrong LLDP value read from NVM\n"));
    return EFI_DEVICE_ERROR;
  }

  *LldpAgentEna = (LldpAdminStatus != 0);

  return EFI_SUCCESS;
}

/** Sets FW LLDP Agent status.

   @param[in]   UndiPrivateData  Pointer to driver private data structure
   @param[in]   LldpAgentEna     Requested LLDP Agent status

   @retval      EFI_SUCCESS         LLDP Agent Status written successfully
   @retval      EFI_DEVICE_ERROR    Failed to read current LLDP Agent status
   @retval      EFI_SUCCESS         Requested LLDP agent status matches current
   @retval      EFI_DEVICE_ERROR    Failed to start or stop LLDP Agent
   @retval      EFI_DEVICE_ERROR    Failed to set DCB parameters
**/
EFI_STATUS
SetLldpAgentStatus (
  IN  UNDI_PRIVATE_DATA  *UndiPrivateData,
  IN  BOOLEAN            *LldpAgentEna
  )
{
  EFI_STATUS             Status;
  enum i40e_status_code  I40eStatus;
  BOOLEAN                CurrentLldpAgentEna;

  Status = GetLldpAgentStatus (UndiPrivateData, &CurrentLldpAgentEna);
  IF_RETURN (EFI_ERROR (Status), Status);

  if (CurrentLldpAgentEna == *LldpAgentEna) {
    return EFI_SUCCESS;
  }

  if (*LldpAgentEna) {
    I40eStatus = i40e_aq_start_lldp (&UndiPrivateData->NicInfo.Hw, TRUE, NULL);
    IF_SCERR_RETURN (I40eStatus, EFI_DEVICE_ERROR);
  } else {
    I40eStatus = i40e_aq_stop_lldp (&UndiPrivateData->NicInfo.Hw, TRUE, TRUE, NULL);
    IF_SCERR_RETURN (I40eStatus, EFI_DEVICE_ERROR);
    I40eStatus = i40e_aq_set_dcb_parameters (&UndiPrivateData->NicInfo.Hw, TRUE, NULL);
    IF_SCERR_RETURN (I40eStatus, EFI_DEVICE_ERROR);
  }

  return EFI_SUCCESS;
}

/** Restore factory LLDP Agent status.

   @param[in]   UndiPrivateData  Pointer to driver private data structure.

   @retval      EFI_SUCCESS      LLDP Agent reset successful.
   @retval      EFI_DEVICE_ERROR Failed to reset LLDP Agent.
**/
EFI_STATUS
ResetLldpAgentStatus (
  IN  UNDI_PRIVATE_DATA *UndiPrivateData
  )
{
  enum i40e_status_code I40eStatus;

  I40eStatus = i40e_aq_restore_lldp (&UndiPrivateData->NicInfo.Hw, NULL, TRUE, NULL);
  IF_SCERR_RETURN (I40eStatus, EFI_DEVICE_ERROR);

  return EFI_SUCCESS;
}

/** Get default LLDP Agent status.

   @param[in]   UndiPrivateData      Pointer to driver private data structure.
   @param[out]  DefaultLldpAgentEna  Pointer to variable which should store default value for LLDP Agent.

   @retval      EFI_SUCCESS          LLDP Agent get default successful.
   @retval      EFI_DEVICE_ERROR     Failed to get default LLDP Agent.
**/
EFI_STATUS
GetDfltLldpAgentStatus (
  IN  UNDI_PRIVATE_DATA  *UndiPrivateData,
  OUT BOOLEAN            *DefaultLldpAgentEna
  )
{
  enum i40e_status_code I40eStatus;

  I40eStatus = i40e_aq_restore_lldp (&UndiPrivateData->NicInfo.Hw, DefaultLldpAgentEna, FALSE, NULL);
  IF_SCERR_RETURN (I40eStatus, EFI_DEVICE_ERROR);

  return EFI_SUCCESS;
}


