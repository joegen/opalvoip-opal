/*
 * capi_linux.h
 *
 * ISDN via CAPI Functions
 *
 * Open Phone Abstraction Library
 *
 * Copyright (c) 2010 Vox Lucida Pty. Ltd.
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.0 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
 * the License for the specific language governing rights and limitations
 * under the License.
 *
 * The Original Code is Open H323 Library.
 *
 * The Initial Developer of the Original Code is Vox Lucida Pty. Ltd.
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision$
 * $Author$
 * $Date$
 */


#include <sys/types.h>
#include <capi20.h> // Install isdn4k-utils-devel to get this


class OpalCapiFunctions
{
public:
  typedef unsigned Result;
  typedef unsigned ApplID;
  typedef unsigned UInt;

  Result REGISTER(UInt bufferSize, UInt maxLogicalConnection, UInt maxBDataBlocks, UInt maxBDataLen, ApplID* pApplID)
  {
    return capi20_register(maxLogicalConnection, maxBDataBlocks, maxBDataLen, pApplID);
  }

  Result RELEASE(ApplID ApplID)
  {
    return capi20_release(ApplID);
  }

  Result PUT_MESSAGE(ApplID ApplID, void * pCAPIMessage)
  {
    return capi20_put_message(ApplID, (unsigned char *)pCAPIMessage);
  }

  Result GET_MESSAGE(ApplID ApplID, void ** ppCAPIMessage)
  {
    return capi20_get_message(ApplID, (unsigned char **)ppCAPIMessage);
  }

  Result WAIT_FOR_SIGNAL(ApplID ApplID)
  {
    return capi20_waitformessage(ApplID, NULL);
  }

  Result GET_MANUFACTURER(char * szBuffer)
  {
    capi20_get_manufacturer(0, (unsigned char *)szBuffer);
    return 0;
  }

  Result GET_VERSION(UInt *pCAPIMajor, UInt *pCAPIMinor, UInt *pManufacturerMajor, UInt *pManufacturerMinor)
  {
    unsigned char buffer[4*sizeof(uint32_t)];
    if (capi20_get_version(0, buffer) == NULL)
      return 0x100;

    if (pCAPIMajor) *pCAPIMajor = (UInt)(((uint32_t *)buffer)[0]);
    if (pCAPIMinor) *pCAPIMinor = (UInt)(((uint32_t *)buffer)[1]);
    if (pManufacturerMajor) *pManufacturerMajor = (UInt)(((uint32_t *)buffer)[2]);
    if (pManufacturerMinor) *pManufacturerMinor = (UInt)(((uint32_t *)buffer)[3]);

    return 0;
  }

  Result GET_SERIAL_NUMBER(char * szBuffer)
  {
      return capi20_get_serial_number(0, (unsigned char *)szBuffer) == NULL ? 0x100 : 0;
  }

  Result GET_PROFILE(void * pBuffer, UInt CtrlNr)
  {
    return capi20_get_profile(CtrlNr, (unsigned char *)pBuffer);
  }

  Result INSTALLED()
  {
    return capi20_isinstalled();
  }
};


// End of File ///////////////////////////////////////////////////////////////
