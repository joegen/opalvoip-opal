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
#include <msclr\marshal_cppstd.h>

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

OpalLyncShim::OpalLyncShim()
  : m_platform(nullptr)
{
}


OpalLyncShim::~OpalLyncShim()
{
  ShutdownPlatform();
}


bool OpalLyncShim::StartPlatform(const char * appName)
{
  m_lastError.clear();

  if (m_platform != nullptr)
    return true;

  System::String^ userAgent = nullptr;
  if (appName != NULL && *appName != '\0')
    userAgent = gcnew System::String(appName);

  Collaboration::CollaborationPlatform^ platform;
  try {
    Collaboration::ClientPlatformSettings^ cps = gcnew Collaboration::ClientPlatformSettings(userAgent, Signaling::SipTransportType::Tls);
    platform = gcnew Collaboration::CollaborationPlatform(cps);
    platform->EndStartup(platform->BeginStartup(nullptr, nullptr));
  }
  catch (System::Exception^ err) {
    m_lastError = msclr::interop::marshal_as<std::string>(err->ToString());
    return false;
  }

  m_platform = new Platform(platform);
  return true;
}


bool OpalLyncShim::ShutdownPlatform()
{
  m_lastError.clear();

  if (m_platform == nullptr)
    return false;

  Collaboration::CollaborationPlatform^ platform = *m_platform;
  if (platform == nullptr)
    return false;

  bool ok = true;
  try {
    platform->EndShutdown(platform->BeginShutdown(nullptr, nullptr));
  }
  catch (System::Exception^ err) {
    m_lastError = msclr::interop::marshal_as<std::string>(err->ToString());
    ok = false;
  }

  delete m_platform;
  m_platform = nullptr;
  return ok;
}


OpalLyncShim::UserEndpoint * OpalLyncShim::CreateUserEndpoint(const char * uri)
{
  m_lastError.clear();

  if (m_platform == nullptr)
    return nullptr;

  Collaboration::UserEndpoint^ uep;
  try {
    Collaboration::UserEndpointSettings^ ues = gcnew Collaboration::UserEndpointSettings(gcnew System::String(uri));
    uep = gcnew Collaboration::UserEndpoint(*m_platform, ues);
    uep->EndEstablish(uep->BeginEstablish(nullptr, nullptr));
  }
  catch (System::Exception^ err) {
    m_lastError = msclr::interop::marshal_as<std::string>(err->ToString());
    return nullptr;
  }

  return new UserEndpoint(uep);
}


void OpalLyncShim::DestroyUserEndpoint(UserEndpoint * user)
{
  delete user;
}



#endif // OPAL_LYNC

// End of File /////////////////////////////////////////////////////////////
