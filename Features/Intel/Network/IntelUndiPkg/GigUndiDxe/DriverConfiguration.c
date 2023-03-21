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
#include "DriverConfiguration.h"

/* Protocol structure tentative definition */
EFI_DRIVER_CONFIGURATION_PROTOCOL gGigUndiDriverConfiguration;

CHAR16 *mConfigMenu[] = {
  L"Set adapter to Autonegotiate (recommended)",
  L"Set adapter to 100Mbps full duplex",
  L"Set adapter to 100Mbps half duplex",
  L"Set adapter to 10Mbps full duplex",
  L"Set adapter to 10Mbps half duplex",
  L"Save settings to NVRAM",
  L"Exit (maintain current settings)",
  NULL
};


/** Displays the options for the configuration menu to allow the user
   to change the speed/duplex settings

   @param[in]   UndiPrivateData   Driver private data structure

   @retval   None
**/
VOID
GigUndiDriverConfigurationDisplayMenu (
  UNDI_PRIVATE_DATA *UndiPrivateData
  )
{
  UINTN            Active;
  UINT16           Selection;
  UINT16           i;
  EFI_INPUT_KEY    Key;
  UINT16           SetupOffset;
  UINT16           ConfigOffset;
  UINT16           SetupWord;
  UINT16           CustomConfigWord;
  UINT16           Word0;
  CHAR16 *         SpeedDuplexString = L"";
  struct e1000_hw *Hw = &UndiPrivateData->NicInfo.Hw;

  Selection = 0;  // Tracks which menu item is highligted
  i         = 0;
  Active    = 0;  // Tracks current speed/duplex setting so '*' can be drawn next to it

  // If the device is a dual port device then we need to use the EEPROM settings
  // for the second adapter port
  switch (UndiPrivateData->NicInfo.Function) {
  case 0:
    ConfigOffset = CONFIG_CUSTOM_WORD;
    SetupOffset  = SETUP_OPTIONS_WORD;
    break;
  case 1:
    ConfigOffset = CONFIG_CUSTOM_WORD_LANB;
    SetupOffset  = SETUP_OPTIONS_WORD_LANB;
    break;
  case 2:
    ConfigOffset = CONFIG_CUSTOM_WORD_LANC;
    SetupOffset  = SETUP_OPTIONS_WORD_LANC;
    break;
  case 3:
    ConfigOffset = CONFIG_CUSTOM_WORD_LAND;
    SetupOffset  = SETUP_OPTIONS_WORD_LAND;
    break;
  default:
    return;
    break;
  }

  e1000_read_nvm (Hw, SetupOffset, 1, &SetupWord);

  // Save the original setup word value so we can tell if the user changed it
  Word0 = SetupWord;

  // If the boot agent EEPROM signature is not set properly then we will initialize
  // the words to default values and assume a default autonegotiation setting
  e1000_read_nvm (Hw, ConfigOffset, 1, &CustomConfigWord);


  if ((CustomConfigWord & SIG_MASK) != SIG) {
    CustomConfigWord = SIG;
    SetupWord = DISPLAY_SETUP_MESSAGE;
  } else {

    // The signature bits are set so get the speed duplex settings
    // Mask of the speed and duplex setting bits so that we can determine
    // what the settings are
    switch (SetupWord & (FSP_MASK | FDP_FULL_DUPLEX_BIT)) {
    case (FDP_FULL_DUPLEX_BIT | FSP_100MBS):
      Hw->mac.autoneg              = 0;
      Hw->mac.forced_speed_duplex  = ADVERTISE_100_FULL;
      SpeedDuplexString            = L"100Mbps full duplex";
      Active = MENU_100_FULL;
      break;
    case (FDP_FULL_DUPLEX_BIT | FSP_10MBS):
      Hw->mac.autoneg              = 0;
      Hw->mac.forced_speed_duplex  = ADVERTISE_10_FULL;
      SpeedDuplexString            = L"10Mbps full duplex";
      Active = MENU_10_FULL;
      break;
    case (FSP_100MBS):
      Hw->mac.autoneg              = 0;
      Hw->mac.forced_speed_duplex  = ADVERTISE_100_HALF;
      SpeedDuplexString            = L"100Mbps half duplex";
      Active = MENU_100_HALF;
      break;
    case (FSP_10MBS):
      Hw->mac.autoneg              = 0;
      Hw->mac.forced_speed_duplex  = ADVERTISE_10_HALF;
      SpeedDuplexString            = L"10Mbps half duplex";
      Active = MENU_10_HALF;
      break;
    default:
      Hw->mac.autoneg              = 1;
      SpeedDuplexString            = L"Autonegotiation";
      Active = MENU_AUTONEG;
      break;
    }
  }

  while (1) {
    gST->ConOut->ClearScreen (gST->ConOut);
    gST->ConOut->SetAttribute (gST->ConOut, EFI_YELLOW | EFI_BACKGROUND_BLACK);
    gST->ConOut->OutputString (gST->ConOut, L"Configure adapter speed and duplex\n\r");

    gST->ConOut->OutputString (gST->ConOut, L"Current setting: ");
    gST->ConOut->OutputString (gST->ConOut, SpeedDuplexString);
    gST->ConOut->OutputString (gST->ConOut, L"\r\n");

    // Print out the menu items with the current selection highlighted
    for (i = 0; mConfigMenu[i] != NULL; i++) {
      gST->ConOut->SetAttribute (gST->ConOut, EFI_LIGHTGRAY | EFI_BACKGROUND_BLACK);
      gST->ConOut->OutputString (gST->ConOut, L"\r\n    ");
      if (i == Selection) {
        gST->ConOut->SetAttribute (gST->ConOut, EFI_BLACK | EFI_BACKGROUND_LIGHTGRAY);
      }

      // Draw an asterisk next to the current speed/duplex selection
      if (i == Active) {
        gST->ConOut->OutputString (gST->ConOut, L"*");
      } else {
        gST->ConOut->OutputString (gST->ConOut, L" ");
      }

      gST->ConOut->OutputString (gST->ConOut, mConfigMenu[i]);
    }

    gST->ConOut->SetAttribute (gST->ConOut, EFI_LIGHTGRAY | EFI_BACKGROUND_BLACK);

    // Capture the user input
    while (gST->ConIn->ReadKeyStroke (gST->ConIn, &Key) != EFI_SUCCESS) {
      ;
    }

    if (Key.UnicodeChar == CHAR_CARRIAGE_RETURN) {

      //  Check to see if user made a change to the speed/duplex settings
      switch (Selection) {
      case MENU_AUTONEG:

        // Speed mask has already been cleared
        SetupWord &= ~FSP_MASK;
        SetupWord |= (FSP_AUTONEG);
        Hw->mac.autoneg  = 1;
        Active = Selection;
        break;
      case MENU_100_FULL:
        SetupWord &= ~FSP_MASK;
        SetupWord |= (FDP_FULL_DUPLEX_BIT | FSP_100MBS);
        Hw->mac.autoneg  = 0;
        Hw->mac.forced_speed_duplex  = ADVERTISE_100_FULL;
        Active = Selection;
        break;
      case MENU_100_HALF:
        SetupWord &= ~(FSP_MASK | FDP_FULL_DUPLEX_BIT);
        SetupWord |= FSP_100MBS;
        Hw->mac.autoneg  = 0;
        Hw->mac.forced_speed_duplex  = ADVERTISE_100_HALF;
        Active = Selection;
        break;
      case MENU_10_FULL:
        SetupWord &= ~FSP_MASK;
        SetupWord |= (FDP_FULL_DUPLEX_BIT | FSP_10MBS);
        Hw->mac.autoneg  = 0;
        Hw->mac.forced_speed_duplex  = ADVERTISE_10_FULL;
        Active = Selection;
        break;
      case MENU_10_HALF:
        SetupWord &= ~(FSP_MASK | FDP_FULL_DUPLEX_BIT);
        SetupWord |= FSP_10MBS;
        Hw->mac.forced_speed_duplex  = ADVERTISE_10_HALF;
        Hw->mac.autoneg  = 0;
        Active = Selection;
        break;
      default:
        break;
      }
      gBS->Stall (1000000);

      if (Selection == MENU_SAVE) {

        // Only write the EEPROM if the speed/duplex value has changed
        if (SetupWord != Word0) {
          gST->ConOut->OutputString (gST->ConOut, L"\n\n\rSaving settings...");
          gBS->Stall (1000000);
          if (e1000_write_nvm (Hw, ConfigOffset, 1, &CustomConfigWord) != E1000_SUCCESS) {
            gST->ConOut->OutputString (gST->ConOut, L"EEPROM write error\n\r");
          } else
          if (e1000_write_nvm (Hw, SetupOffset, 1, &SetupWord) != E1000_SUCCESS) {
            gST->ConOut->OutputString (gST->ConOut, L"EEPROM write error\n\r");
          } else {

            // Success
            e1000_update_nvm_checksum (Hw);
            Word0 = SetupWord;
            gST->ConOut->OutputString (gST->ConOut, L"done\n\r");
          }
          gBS->Stall (1000000);
        } else {
          gST->ConOut->OutputString (gST->ConOut, L"\n\n\rSettings have not changed\n");
          gBS->Stall (1000000);
        }
      }

      if (Selection == MENU_EXIT) {
        if (Word0 != SetupWord) {
          gST->ConOut->OutputString (
                         gST->ConOut,
                         L"\n\n\n\n\n\n\rChanged settings have not been saved.  Do you want to exit anyway? (y/n)"
                       );
          do {
            while (gST->ConIn->ReadKeyStroke (gST->ConIn, &Key) != EFI_SUCCESS) {
              ;
            }
            if (Key.UnicodeChar == 'y'
              || Key.UnicodeChar == 'Y'
              || Key.UnicodeChar == 'n'
              || Key.UnicodeChar == 'N')
            {
              break;
            }
          } while (1);
        }

        // Always exit unless the users selects N
        if ((Key.UnicodeChar != 'n')
          && (Key.UnicodeChar != 'N'))
        {
          break;
        }
      }
    }

    if (Key.ScanCode == SCAN_UP) {
      if (Selection > 0) {
        Selection--;
      }
    } else if (Key.ScanCode == SCAN_DOWN) {
      if (mConfigMenu[Selection + 1] != NULL) {
        Selection++;
      }
    } else if (Key.ScanCode == SCAN_ESC) {
      break;
    }
  } // end while(1)
}

/** Callback function for Driver Configuration protocol.  Finds the NII adapter handle for
   Controller handle and then calls the setup menu

   @param[in]   This              Driver configuration protocol instance
   @param[in]   ControllerHandle  The network driver controller handle
   @param[in]   ChildHandle       The NII child handle (not used)
   @param[in]   Language          Always english
   @param[out]  ActionRequired    Not used

   @retval   EFI_UNSUPPORTED   Unable to open the driver configuration protocol for ControllerHandle
   @retval   EFI_UNSUPPORTED   ControllerHandle is NULL
   @retval   EFI_UNSUPPORTED   Driver is not managing ControllerHandle
   @retval   EFI_UNSUPPORTED   It is fiber or SerDes card
   @retval   EFI_SUCCESS       Configuration was successful
**/
EFI_STATUS
EFIAPI
GigUndiDriverConfigurationSetOptions (
  IN EFI_DRIVER_CONFIGURATION_PROTOCOL *          This,
  IN EFI_HANDLE                                   ControllerHandle,
  IN EFI_HANDLE                                   ChildHandle, OPTIONAL
  IN CHAR8 *                                      Language,
  OUT EFI_DRIVER_CONFIGURATION_ACTION_REQUIRED *  ActionRequired
  )
{
  EFI_DEVICE_PATH_PROTOCOL *UndiDevicePath;
  UNDI_PRIVATE_DATA *       UndiPrivateData;
  EFI_NII_POINTER_PROTOCOL *NiiPointerProtocol;
  EFI_STATUS                Status;

  UINT8 ReceiveStarted;

  UndiPrivateData = NULL;
  *ActionRequired    = EfiDriverConfigurationActionNone;

  if (ControllerHandle == NULL) {
    DEBUGPRINT (CRITICAL, ("ControllerHandle == NULL\n"));
    DEBUGWAIT (CRITICAL);
    return EFI_UNSUPPORTED;
  }

  // Make sure this driver is currently managing ControllerHandle
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiDevicePathProtocolGuid,
                  (VOID * *) &UndiDevicePath,
                  gUndiDriverBinding.DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                );

  if (Status != EFI_ALREADY_STARTED) {
    gBS->CloseProtocol (
           ControllerHandle,
           &gEfiDevicePathProtocolGuid,
           gUndiDriverBinding.DriverBindingHandle,
           ControllerHandle
         );
    DEBUGPRINT (CRITICAL, ("OpenProtocol Status != EFI_ALREADY_STARTED %X\n", Status));
    DEBUGWAIT (CRITICAL);
    return EFI_UNSUPPORTED;
  }

  //  Open an instance for the Network Interface Identifier Protocol so we can check
  // if the child handle interface is actually valid.
  DEBUGPRINT (CFG, ("Open an instance for the Network Interface Identifier Protocol\n"));
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiNiiPointerGuid,
                  (VOID * *) &NiiPointerProtocol,
                  gUndiDriverBinding.DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                );

  if (EFI_ERROR (Status)) {
    DEBUGPRINT (CRITICAL, ("OpenProtocol error Status %X\n", Status));
    DEBUGWAIT (CRITICAL);
    return Status;
  }

  UndiPrivateData = UNDI_PRIVATE_DATA_FROM_THIS (NiiPointerProtocol->NiiProtocol31);

  // Speed duplex configuration is not supported on fiber or serdes cards.
  if (UndiPrivateData->NicInfo.Hw.phy.media_type != e1000_media_type_copper) {
    DEBUGPRINT (CRITICAL, ("Phy media type not copper\n"));
    DEBUGWAIT (CRITICAL);
    return EFI_UNSUPPORTED;
  }

  // Remember receiver state, so we can leave it in the same state as it was before settings were made.
  ReceiveStarted = UndiPrivateData->NicInfo.RxRing.IsRunning;

  GigUndiDriverConfigurationDisplayMenu (UndiPrivateData);

  // After speed/duplex setting completes we need to perform a full reset of the adapter.
  // If the adapter was initialized on entry then force a full reset of the adapter.
  // Also reenable the receive unit if it was enabled before we started the PHY loopback test.
  e1000_reset_hw (&UndiPrivateData->NicInfo.Hw);
  UndiPrivateData->NicInfo.HwInitialized = FALSE;

  if (UndiPrivateData->NicInfo.State == PXE_STATFLAGS_GET_STATE_INITIALIZED) {
    E1000Inititialize (&UndiPrivateData->NicInfo);
    DEBUGPRINT (DIAG, ("E1000Inititialize complete\n"));

    //  Restart the receive unit if it was running on entry
    if (ReceiveStarted) {
      DEBUGPRINT (DIAG, ("RESTARTING RU\n"));
      E1000ReceiveStart (&UndiPrivateData->NicInfo);
    }
  }
  DEBUGPRINT (DIAG, ("ADAPTER RESET COMPLETE\n"));

  return EFI_SUCCESS;
}

/** Not implemented

   @param[in]   This                Driver configuration protocol current instance
   @param[in]   ControllerHandle    Controller handle
   @param[in]   ChildHandle

   @retval   EFI_SUCCESS   Always returned
**/
EFI_STATUS
EFIAPI
GigUndiDriverConfigurationOptionsValid (
  IN EFI_DRIVER_CONFIGURATION_PROTOCOL *   This,
  IN EFI_HANDLE                            ControllerHandle,
  IN EFI_HANDLE                            ChildHandle OPTIONAL
  )
{
  return EFI_SUCCESS;
}

/** Restores the speed/duplex settings to the autonegotiation default value

   @param[in]   This              Driver configuration protocol instance
   @param[in]   ControllerHandle  The network driver controller handle
   @param[in]   ChildHandle       The NII child handle (not used)
   @param[in]   DefaultType       Not used
   @param[out]  ActionRequired    Not used

   @retval   EFI_UNSUPPORTED   Unable to open the driver configuration protocol for ControllerHandle
   @retval   EFI_UNSUPPORTED   ControllerHandle is NULL
   @retval   EFI_UNSUPPORTED   Driver is not managing ControllerHandle
   @retval   EFI_UNSUPPORTED   It is fiber or SerDes card
   @retval   EFI_SUCCESS       Configuration was successful
**/
EFI_STATUS
EFIAPI
GigUndiDriverConfigurationForceDefaults (
  IN EFI_DRIVER_CONFIGURATION_PROTOCOL *                   This,
  IN EFI_HANDLE                                            ControllerHandle,
  IN EFI_HANDLE                                            ChildHandle, OPTIONAL
  IN UINT32                                                DefaultType,
  OUT EFI_DRIVER_CONFIGURATION_ACTION_REQUIRED *           ActionRequired
  )
{
  EFI_DEVICE_PATH_PROTOCOL *UndiDevicePath;
  UNDI_PRIVATE_DATA *       UndiPrivateData;
  EFI_NII_POINTER_PROTOCOL *NiiPointerProtocol;
  EFI_STATUS                Status;
  struct e1000_hw *         Hw;
  UINT16                    ConfigOffset;
  UINT16                    SetupOffset;
  UINT16                    CustomConfigWord;
  UINT16                    SetupWord;

  UndiPrivateData = NULL;
  *ActionRequired    = EfiDriverConfigurationActionNone;

  if (ControllerHandle == NULL) {
    DEBUGPRINT (CRITICAL, ("ControllerHandle == NULL\n"));
    DEBUGWAIT (CRITICAL);
    return EFI_UNSUPPORTED;
  }

  // Make sure this driver is currently managing ControllerHandle
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiDevicePathProtocolGuid,
                  (VOID * *) &UndiDevicePath,
                  gUndiDriverBinding.DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                );

  if (Status != EFI_ALREADY_STARTED) {
    gBS->CloseProtocol (
           ControllerHandle,
           &gEfiDevicePathProtocolGuid,
           gUndiDriverBinding.DriverBindingHandle,
           ControllerHandle
         );
    DEBUGPRINT (CRITICAL, ("OpenProtocol Status != EFI_ALREADY_STARTED %X\n", Status));
    DEBUGWAIT (CRITICAL);
    return EFI_UNSUPPORTED;
  }

  //  Open an instance for the Network Interface Identifier Protocol so we can check
  // if the child handle interface is actually valid.
  DEBUGPRINT (CFG, ("Open an instance for the Network Interface Identifier Protocol\n"));
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiNiiPointerGuid,
                  (VOID * *) &NiiPointerProtocol,
                  gUndiDriverBinding.DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                );

  if (EFI_ERROR (Status)) {
    DEBUGPRINT (CRITICAL, ("OpenProtocol error Status %X\n", Status));
    DEBUGWAIT (CRITICAL);
    return Status;
  }

  UndiPrivateData = UNDI_PRIVATE_DATA_FROM_THIS (NiiPointerProtocol->NiiProtocol31);

  // Speed duplex configuration is not supported on fiber or serdes cards.
  if (UndiPrivateData->NicInfo.Hw.phy.media_type != e1000_media_type_copper) {
    DEBUGPRINT (CRITICAL, ("Phy media type not copper\n"));
    DEBUGWAIT (CRITICAL);
    return EFI_UNSUPPORTED;
  }

  Hw = &UndiPrivateData->NicInfo.Hw;

  // If the device is a dual port device then we need to use the EEPROM settings
  // for the second adapter port
  switch (UndiPrivateData->NicInfo.Function) {
  case 0:
    ConfigOffset = CONFIG_CUSTOM_WORD;
    SetupOffset  = SETUP_OPTIONS_WORD;
    break;
  case 1:
    ConfigOffset = CONFIG_CUSTOM_WORD_LANB;
    SetupOffset  = SETUP_OPTIONS_WORD_LANB;
    break;
  case 2:
    ConfigOffset = CONFIG_CUSTOM_WORD_LANC;
    SetupOffset  = SETUP_OPTIONS_WORD_LANC;
    break;
  case 3:
    ConfigOffset = CONFIG_CUSTOM_WORD_LAND;
    SetupOffset  = SETUP_OPTIONS_WORD_LAND;
    break;
  default:
    return EFI_INVALID_PARAMETER;
    break;
  }

  e1000_read_nvm (Hw, ConfigOffset, 1, &CustomConfigWord);
  e1000_read_nvm (Hw, SetupOffset, 1, &SetupWord);

  // If the signature word is not set then we will always assume the default values
  // so do not change anything.  If the signature bits are set then set the adapter
  // back to autonegotiate
  if ((CustomConfigWord & SIG_MASK) == SIG) {

    // Only write the setup word if the adapter is not already set to autonegotiate
    if ((SetupWord & FSP_MASK) != FSP_AUTONEG) {
      SetupWord = (UINT16) ((SetupWord & ~FSP_MASK) | FSP_AUTONEG);
      e1000_write_nvm (Hw, SetupOffset, 1, &SetupWord);
      e1000_update_nvm_checksum (Hw);
    }
  }

  return EFI_SUCCESS;
}

/* Protocol structure definition and initialization */

EFI_DRIVER_CONFIGURATION_PROTOCOL gGigUndiDriverConfiguration = {
  GigUndiDriverConfigurationSetOptions,
  GigUndiDriverConfigurationOptionsValid,
  GigUndiDriverConfigurationForceDefaults,
  "eng"
};
