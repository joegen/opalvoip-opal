/*
 * lyncep.cxx
 *
 * Header file for Lync (UCMA) interface
 *
 * Copyright (C) 2016 Vox Lucida Pty. Ltd.
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
 * The Original Code is Portable Windows Library.
 *
 * The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * Contributor(s): ______________________________________.
 */

#define OPAL_LYNC_SHIM 1
#include <ep/lyncep.h>

#if OPAL_LYNC

#include <vcclr.h>
#using OPAL_LYNC_LIBRARY

using namespace Microsoft::Rtc;

struct OpalLyncShim::UserEndpoint : gcroot<Collaboration::UserEndpoint^>
{
  UserEndpoint(Collaboration::UserEndpoint^ ep) : gcroot<Collaboration::UserEndpoint^>(ep) { }
};

struct OpalLyncShim::Platform : gcroot<Collaboration::CollaborationPlatform^>
{
  Platform(Collaboration::CollaborationPlatform^ p) : gcroot<Collaboration::CollaborationPlatform^>(p) { }
};


///////////////////////////////////////////////////////////////////////////////

OpalLyncShim::OpalLyncShim(const char * appName)
{
  System::String^ userAgent = nullptr;
  if (appName != NULL && *appName != '\0')
    userAgent = gcnew System::String(appName);

  Collaboration::ClientPlatformSettings^ cps = gcnew Collaboration::ClientPlatformSettings(userAgent, Signaling::SipTransportType::Tcp);
  m_platform = new Platform(gcnew Collaboration::CollaborationPlatform(cps));
}


OpalLyncShim::~OpalLyncShim()
{
  delete m_platform;
}


OpalLyncShim::UserEndpoint * OpalLyncShim::CreateUserEndpoint(const char * uri)
{
    Collaboration::UserEndpointSettings^ ues = gcnew Collaboration::UserEndpointSettings(gcnew System::String(uri));
    Collaboration::UserEndpoint^ uep = gcnew Collaboration::UserEndpoint(*m_platform, ues);
    if (uep != nullptr)
      return new UserEndpoint(uep);
    return NULL;
}


void OpalLyncShim::DestroyUserEndpoint(UserEndpoint * user)
{
  delete user;
}



#endif // OPAL_LYNC

// End of File /////////////////////////////////////////////////////////////
