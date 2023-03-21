/**************************************************************************

Copyright (c) 2014 - 2020, Intel Corporation. All rights reserved.

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
#include <wol.h>

#ifndef WOL_HAF
  void _WolGetDeviceId(WOL_ADAPTER_HANDLE_TYPE Handle, _WOL_DEVICE_ID_t *DeviceId)
  {
    DeviceId->VendorId = Handle->NicInfo.Hw.vendor_id;
    DeviceId->DeviceId = Handle->NicInfo.Hw.device_id;
    DeviceId->SubVendorId = Handle->NicInfo.Hw.subsystem_vendor_id;
    DeviceId->SubDeviceId = Handle->NicInfo.Hw.subsystem_device_id;
  }
#else /* HAF */
  void _WolGetDeviceId(WOL_ADAPTER_HANDLE_TYPE Handle, _WOL_DEVICE_ID_t *DeviceId)
  {
    NAL_ADAPTER_VENDOR_INFO VendorInfo;


    if (NalGetVendorInformation(Handle, &VendorInfo) == NAL_SUCCESS) {
      DeviceId->VendorId = VendorInfo.Vendor;
      DeviceId->DeviceId = VendorInfo.Device;
      DeviceId->SubVendorId = VendorInfo.SubVendor;
      DeviceId->SubDeviceId = VendorInfo.SubDevice;
    } else {
      DeviceId->VendorId = 0;
      DeviceId->DeviceId = 0;
      DeviceId->SubVendorId = 0;
      DeviceId->SubDeviceId = 0;
    }
  }
#endif

  UINT8 _WolGetLanPort(WOL_ADAPTER_HANDLE_TYPE Handle)
  {
    return Handle->NicInfo.LanFunction;
  }

  BOOLEAN _WolIsFirstController(WOL_ADAPTER_HANDLE_TYPE Handle)
  {
    extern EFI_DRIVER_BINDING_PROTOCOL gUndiDriverBinding;

    EFI_STATUS             Status;
    UINTN                  i, NumHandles = 0;
    EFI_HANDLE            *HandleBuf;
    BOOLEAN                IsFirstController = TRUE;

    switch (Handle->NicInfo.Hw.device_id) {
    case E1000_DEV_ID_82571EB_QUAD_COPPER:
    case E1000_DEV_ID_82571EB_QUAD_COPPER_LP:
    case E1000_DEV_ID_82571EB_QUAD_FIBER:
    case E1000_DEV_ID_82575GB_QUAD_COPPER:
    case E1000_DEV_ID_82576_QUAD_COPPER:
    case E1000_DEV_ID_82576_QUAD_COPPER_ET2:
    case E1000_DEV_ID_82571PT_QUAD_COPPER:

        Status = gBS->LocateHandleBuffer(
          ByProtocol,
          &gEfiPciIoProtocolGuid,
          NULL,
          &NumHandles,
          &HandleBuf
          );

        if (EFI_ERROR(Status)) {
          return FALSE;
        }

        for (i = 0; i < NumHandles; i++) {

          UINTN Seg, Bus, Device, Function;
          EFI_PCI_IO_PROTOCOL *PciIo;

          Status = gBS->OpenProtocol (
            HandleBuf[i],
            &gEfiPciIoProtocolGuid,
            (VOID **) &PciIo,
            gUndiDriverBinding.DriverBindingHandle,
            Handle->ControllerHandle,
            EFI_OPEN_PROTOCOL_GET_PROTOCOL
            );

          if (EFI_ERROR(Status)) {
            continue;
          }

          // No need to check status code as it can only return EFI_INVALID_PARAMETER
          // only when any of the last four parameters is NULL.
          PciIo->GetLocation (PciIo, &Seg, &Bus, &Device, &Function);

          // Only PCI function 0 on the first device supports WOL. On PCIe
          // cards each device will be on its own secondary bus as device 0.
          // If we can read device 0 at the next lower bus number and it's
          // the same ID, then we are looking at the second device on the
          // card and WOL is not supported. Otherwise it must be the PCIe
          // switch and therefore this is the first device.

          if ((Seg == Handle->NicInfo.Segment) && (Bus == Handle->NicInfo.Bus - 1)) {

            UINT16 DeviceId, VendorId;

            Status = PciIo->Pci.Read (PciIo, EfiPciIoWidthUint16, 2, 1, &DeviceId);
            if (EFI_ERROR(Status)) {
                break;
            }

            Status = PciIo->Pci.Read (PciIo, EfiPciIoWidthUint16, 0, 1, &VendorId);
            if (EFI_ERROR(Status)) {
                break;
            }

            if ((DeviceId == Handle->NicInfo.Hw.device_id) ||
                (VendorId == Handle->NicInfo.Hw.vendor_id)) {
                IsFirstController = FALSE;
            }

            break;
          }

        }
        gBS->FreePool(HandleBuf);
    }

    return IsFirstController;
  }

  WOL_MAC_TYPE _WolGetMacType(WOL_ADAPTER_HANDLE_TYPE Handle)
  {
    return WOL_MAKE_MACTYPE(WOL_1G, Handle->NicInfo.Hw.mac.type);
  }

  WOL_STATUS _WolEepromRead16(WOL_ADAPTER_HANDLE_TYPE Handle, UINT16 Offset, UINT16 *Data)
  {
    if (e1000_read_nvm(&Handle->NicInfo.Hw, Offset, 1, Data) == E1000_SUCCESS) {
      return EFI_SUCCESS;
    } else {
      return EFI_DEVICE_ERROR;
    }
  }

  WOL_STATUS _WolEepromWrite16(WOL_ADAPTER_HANDLE_TYPE Handle, UINT16 Offset, UINT16 Data)
  {
    if (e1000_write_nvm(&Handle->NicInfo.Hw, Offset, 1, &Data) == E1000_SUCCESS) {
      return EFI_SUCCESS;
    } else {
      return EFI_DEVICE_ERROR;
    }
  }

  WOL_STATUS _WolEepromUpdateChecksum(WOL_ADAPTER_HANDLE_TYPE  Handle)
  {
    if (e1000_update_nvm_checksum(&Handle->NicInfo.Hw) == E1000_SUCCESS) {
      return EFI_SUCCESS;
    } else {
      return EFI_DEVICE_ERROR;
    }
  }


  UINT8 _WolGetFunction(WOL_ADAPTER_HANDLE_TYPE Handle)
  {
    return Handle->NicInfo.Function;
  }


  WOL_STATUS WolSetApmRegister (WOL_ADAPTER_HANDLE_TYPE Handle, BOOLEAN Enable)
  {
    return _WolSetApmRegister_1G (Handle, Enable);
  }
