/**************************************************************************

Copyright (c) 2015 - 2022, Intel Corporation. All rights reserved.

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
#include "ComponentName.h"
#include "DeviceSupport.h"

/* Protocol structures tentative definitions */
EFI_COMPONENT_NAME_PROTOCOL  gUndiComponentName;
EFI_COMPONENT_NAME2_PROTOCOL gUndiComponentName2;

// The define rodeo below creates the driver component name based on the build env.
// Be careful with space placement! Add spaces after each name part put before the
// version, add spaces before parts put after the version, so that you get:
// L"Before " L"Version " L"1.2.34" L" And" L" After"
#if defined (DBG_LVL) && ((DBG_LVL) != (NONE))
  #define COMP_NAME_PREFIX L"[DEBUG] "
#else /* Production component name */
  #define COMP_NAME_PREFIX L""
#endif /* [DEBUG] prefix */

#define COMP_NAME_VENDOR L"Intel(R) "


  #define COMP_NAME_PRODUCT L"PRO/1000 "

// Extras between the product name and version.
  #define COMP_NAME_EXTRA L"Open Source "

  #define COMP_NAME_VERSION L"%1d.%1d.%02d"

    #define COMP_NAME_SUFFIX L" PCI-E"

#define COMP_NAME COMP_NAME_PREFIX COMP_NAME_VENDOR COMP_NAME_PRODUCT COMP_NAME_EXTRA COMP_NAME_VERSION COMP_NAME_SUFFIX
CHAR16 * mDriverNameFormat = COMP_NAME;


// Version and Branding Information
CHAR16 mDriverNameString[60];

STATIC EFI_UNICODE_STRING_TABLE mUndiDriverNameTable[] = {
  { "eng", mDriverNameString},
  { "en-US", mDriverNameString},
  { NULL, NULL}
};



/** Searches through the branding string list for the best possible match for the controller
   associated with UndiPrivateData.

   @param[in,out]   UndiPrivateData   Driver private data structure

   @return    Controller name initialized according to device
**/
VOID
ComponentNameInitializeControllerName (
  UNDI_PRIVATE_DATA *UndiPrivateData
  )
{
  CHAR16 *BrandingString;

  DEBUGPRINT (INIT, ("ComponentNameInitializeDriverAndControllerName\n"));

  // Initialize driver name for ComponentNameGetDriverName & HII
  UnicodeSPrint (
    mDriverNameString,
    sizeof (mDriverNameString),
    mDriverNameFormat,
    MAJORVERSION,
    MINORVERSION,
    BUILDNUMBER
    );

  BrandingString = GetDeviceBrandingString (UndiPrivateData);

  if (BrandingString != NULL) {
    UndiPrivateData->Brand = BrandingString;
    AddUnicodeString2 (
      "eng",
      gUndiComponentName.SupportedLanguages,
      &UndiPrivateData->ControllerNameTable,
      BrandingString,
      TRUE
    );

    AddUnicodeString2 (
      "en-US",
      gUndiComponentName2.SupportedLanguages,
      &UndiPrivateData->ControllerNameTable,
      BrandingString,
      FALSE
    );
  }
}

/** Retrieves a Unicode string that is the user readable name of the EFI Driver.

   @param[in]   This   A pointer to the EFI_COMPONENT_NAME_PROTOCOL instance.
   @param[in]   Language   A pointer to a three character ISO 639-2 language identifier.
                           This is the language of the driver name that that the caller
                           is requesting, and it must match one of the languages specified
                           in SupportedLanguages.  The number of languages supported by a
                           driver is up to the driver writer.
   @param[out]   DriverName   A pointer to the Unicode string to return.  This Unicode string
                              is the name of the driver specified by This in the language
                              specified by Language.

   @retval   EFI_SUCCES             The Unicode string for the Driver specified by This
                                    and the language specified by Language was returned
                                    in DriverName.
   @retval   EFI_INVALID_PARAMETER  Language is NULL.
   @retval   EFI_INVALID_PARAMETER  DriverName is NULL.
   @retval   EFI_UNSUPPORTED        The driver specified by This does not support the
                                    language specified by Language.
**/
EFI_STATUS
EFIAPI
ComponentNameGetDriverName (
  IN  EFI_COMPONENT_NAME_PROTOCOL *This,
  IN  CHAR8 *                      Language,
  OUT CHAR16 **                    DriverName
  )
{
  if (DriverName == NULL) {
    // Other vars:
    // - This can be NULL, handled as a special case below.
    // - Language can be NULL, not used for the special case.
    DEBUGPRINT (INIT, ("DriverName is NULL!\n"));
    return EFI_INVALID_PARAMETER;
  }

  return LookupUnicodeString2 (
           Language,
           This->SupportedLanguages,
           mUndiDriverNameTable,
           DriverName,
           (BOOLEAN) (This == &gUndiComponentName)
         );
}

/** Retrieves a Unicode string that is the user readable name of the controller
   that is being managed by an EFI Driver.

   @param[in]   This   A pointer to the EFI_COMPONENT_NAME_PROTOCOL instance.
   @param[in]   ControllerHandle   The handle of a controller that the driver specified by
                                   This is managing. This handle specifies the controller
                                   whose name is to be returned.
   @param[in]   ChildHandle   The handle of the child controller to retrieve the name
                              of. This is an optional parameter that may be NULL. It
                              will be NULL for device drivers. It will also be NULL
                              for a bus drivers that wish to retrieve the name of the
                              bus controller. It will not be NULL for a bus driver
                              that wishes to retrieve the name of a child controller.
   @param[in]   Language  A pointer to a three character ISO 639-2 language
                          identifier. This is the language of the controller name
                          that the caller is requesting, and it must match one
                          of the languages specified in SupportedLanguages. The
                          number of languages supported by a driver is up to the
                          driver writer.
   @param[out]  ControllerName   A pointer to the Unicode string to return. This Unicode
                                  string is the name of the controller specified by
                                  ControllerHandle and ChildHandle in the language specified
                                  by Language from the point of view of the driver specified
                                  by This.

   @retval   EFI_SUCCESS            The Unicode string for the user readable name in the
                                    language specified by Language for the driver
                                    specified by This was returned in DriverName.
   @retval   EFI_INVALID_PARAMETER  ControllerHandle is not a valid EFI_HANDLE.
   @retval   EFI_INVALID_PARAMETER  The pointer to the EFI_COMPONENT_NAME_PROTOCOL instance is NULL.
   @retval   EFI_INVALID_PARAMETER  Language is NULL.
   @retval   EFI_INVALID_PARAMETER  ControllerName is NULL.
   @retval   EFI_UNSUPPORTED        ChildHandle is not NULL and is not a child of ControllerHandle.
   @retval   EFI_UNSUPPORTED        The driver specified by This does not support the
                                    language specified by Language.
**/
EFI_STATUS
EFIAPI
ComponentNameGetControllerName (
  IN  EFI_COMPONENT_NAME_PROTOCOL   *This,
  IN  EFI_HANDLE                    ControllerHandle,
  IN  EFI_HANDLE                    ChildHandle OPTIONAL,
  IN  CHAR8                         *Language,
  OUT CHAR16                        **ControllerName
  )
{
  EFI_NII_POINTER_PROTOCOL *NiiPointerProtocol;
  UNDI_PRIVATE_DATA        *UndiPrivateData;
  EFI_STATUS               Status;

  if (This == NULL || Language == NULL || ControllerHandle == NULL || ControllerName == NULL) {
    DEBUGPRINT (INIT, ("One of the passed, required parameters was NULL.\n"));
    return EFI_INVALID_PARAMETER;
  }

  // Make sure this driver is currently managing ControllerHandle
  Status = EfiTestManagedDevice (
             ControllerHandle,
             gUndiDriverBinding.DriverBindingHandle,
             &gEfiDevicePathProtocolGuid
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // Retrieve an instance of a produced protocol from ControllerHandle
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiNiiPointerGuid,
                  (VOID * *) &NiiPointerProtocol,
                  gUndiDriverBinding.DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // Retrieve the private context data structure for ControllerHandle
  UndiPrivateData = UNDI_PRIVATE_DATA_FROM_THIS (NiiPointerProtocol->NiiProtocol31);

  // If ChildHandle is not NULL, then make sure this driver produced ChildHandle
  if (ChildHandle != NULL) {
    Status = EfiTestChildHandle (
               ControllerHandle,
               ChildHandle,
               &gEfiPciIoProtocolGuid
               );
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  // Look up the controller name from a dynamic table of controller names
  return LookupUnicodeString2 (
           Language,
           This->SupportedLanguages,
           UndiPrivateData->ControllerNameTable,
           ControllerName,
           (BOOLEAN) (This == &gUndiComponentName)
           );
}

/* Component Name protocol structures definition and initialization */

EFI_COMPONENT_NAME_PROTOCOL gUndiComponentName = {
  ComponentNameGetDriverName,
  ComponentNameGetControllerName,
  "eng"
};

EFI_COMPONENT_NAME2_PROTOCOL gUndiComponentName2 = {
  (EFI_COMPONENT_NAME2_GET_DRIVER_NAME) ComponentNameGetDriverName,
  (EFI_COMPONENT_NAME2_GET_CONTROLLER_NAME) ComponentNameGetControllerName,
  "en-US"
};

/* Supported EFI version protocol structures definition and initialization */

EFI_DRIVER_SUPPORTED_EFI_VERSION_PROTOCOL gUndiSupportedEfiVersion = {
  sizeof (EFI_DRIVER_SUPPORTED_EFI_VERSION_PROTOCOL),
  EFI_2_80_SYSTEM_TABLE_REVISION
};
