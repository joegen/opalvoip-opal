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

#include <ptlib.h>

#include <ep/lyncep.h>

#define new PNEW
#define PTraceModule() "LyncEP"


#if OPAL_LYNC

///////////////////////////////////////////////////////////////

OpalLyncEndPoint::OpalLyncEndPoint(OpalManager & manager, const char *prefix)
  : OpalEndPoint(manager, prefix, IsNetworkEndPoint)
  , m_platform(nullptr)
{
}


OpalLyncEndPoint::~OpalLyncEndPoint()
{
}


void OpalLyncEndPoint::ShutDown()
{
  {
    PWaitAndSignal lock(m_registrationMutex);
    for (RegistrationMap::iterator it = m_registrations.begin(); it != m_registrations.end(); ++it)
      DestroyUserEndpoint(it->second);
    m_registrations.clear();
  }

  if (!DestroyPlatform(m_platform)) {
    PTRACE_IF(2, !GetLastError().empty(), "Error shutting down Lync UCMA platform: " << GetLastError());
  }

  m_platform = nullptr;
}


OpalMediaFormatList OpalLyncEndPoint::GetMediaFormats() const
{
  // just raw audio for now, then work out what Lync supports, somehow ...
  OpalMediaFormatList formats;
  formats += OpalPCM16;
  return formats;
}


PSafePtr<OpalConnection> OpalLyncEndPoint::MakeConnection(OpalCall & call,
                                                          const PString & uri,
                                                          void * userData,
                                                          unsigned int options,
                                                          OpalConnection::StringOptions * stringOptions)
{
  return AddConnection(CreateConnection(call, uri, userData, options, stringOptions));
}


OpalLyncConnection * OpalLyncEndPoint::CreateConnection(OpalCall & call,
                                                        const PString & uri,
                                                        void * userData,
                                                        unsigned int options,
                                                        OpalConnection::StringOptions * stringOptions)
{
  return new OpalLyncConnection(call, *this, uri, userData, options, stringOptions);
}


PBoolean OpalLyncEndPoint::GarbageCollection()
{
  return OpalEndPoint::GarbageCollection();
}


bool OpalLyncEndPoint::Register(const PString & uri)
{
  PWaitAndSignal lock(m_registrationMutex);

  if (m_platform == nullptr) {
    m_platform = CreatePlatform(PProcess::Current().GetName());
    if (m_platform == nullptr) {
      PTRACE(2, "Error initialising Lync UCMA platform: " << GetLastError());
      return false;
    }
  }

  RegistrationMap::iterator it = m_registrations.find(uri);
  if (it != m_registrations.end())
    return false;

  UserEndpoint * user = CreateUserEndpoint(*m_platform, uri);
  if (user == NULL) {
    PTRACE(2, "Error registering \"" << uri << "\" as Lync UCMA user: " << GetLastError());
    return false;
  }

  m_registrations[uri] = user;
  PTRACE(3, "Registered \"" << uri << "\" as Lync UCMA.");
  return true;
}


bool OpalLyncEndPoint::Unregister(const PString & uri)
{
  PWaitAndSignal lock(m_registrationMutex);

  RegistrationMap::iterator it = m_registrations.find(uri);
  if (it == m_registrations.end())
    return false;

  DestroyUserEndpoint(it->second);
  m_registrations.erase(it);
  PTRACE(3, "Unregistered \"" << uri << "\" as Lync UCMA.");
  return true;
}


OpalLyncShim::UserEndpoint * OpalLyncEndPoint::GetRegistration(const PString & uri)
{
  PWaitAndSignal lock(m_registrationMutex);

  RegistrationMap::iterator it = uri.IsEmpty() || uri == "*" ? m_registrations.begin() : m_registrations.find(uri);
  return it != m_registrations.end() ? it->second : nullptr;
}


///////////////////////////////////////////////////////////////////////

OpalLyncConnection::OpalLyncConnection(OpalCall & call,
                                       OpalLyncEndPoint & ep,
                                       const PString & /*dialNumber*/,
                                       void * /*userData*/,
                                       unsigned options,
                                       OpalConnection::StringOptions * stringOptions)
  : OpalConnection(call, ep, ep.GetManager().GetNextToken('L'), options, stringOptions)
  , m_endpoint(ep)
  , m_conversation(nullptr)
  , m_audioVideoCall(nullptr)
{
}


PBoolean OpalLyncConnection::SetUpConnection()
{
  OpalLyncShim::UserEndpoint * uep = m_endpoint.GetRegistration(GetLocalPartyURL());
  if (uep == nullptr) {
    PTRACE(2, "Cannot find registration for user: " << GetLocalPartyURL());
    return false;
  }

  m_conversation = CreateConversation(*uep);
  if (m_conversation == nullptr) {
    PTRACE(2, "Error creating Lync UCMA conversation: " << GetLastError());
    return false;
  }

  m_audioVideoCall = CreateAudioVideoCall(*m_conversation, GetRemotePartyURL());
  if (m_audioVideoCall == nullptr) {
    PTRACE(2, "Error creating Lync UCMA conversation: " << GetLastError());
    return false;
  }

  PTRACE(3, "Lync UCMA call from " << GetLocalPartyURL() << " to " << GetRemotePartyURL() << " started.");
  return true;
}


void OpalLyncConnection::OnCallStateChanged(int previousState, int newState)
{
  PTRACE(3, "Lync UCMA call stat changed from " << previousState << " to " << newState);
}


void OpalLyncConnection::OnEndCallEstablished(const std::string & error)
{
  if (error.empty())
    InternalOnEstablished();
  else {
    PTRACE(2, "Error establishing Lync UCMA call: " << error);
    Release(EndedByConnectFail);
  }
}


void OpalLyncConnection::OnReleased()
{
  DestroyAudioVideoCall(m_audioVideoCall);
  m_audioVideoCall = nullptr;

  DestroyConversation(m_conversation);
  m_conversation = nullptr;
}


OpalMediaFormatList OpalLyncConnection::GetMediaFormats() const
{
  return m_endpoint.GetMediaFormats();
}


PBoolean OpalLyncConnection::SetAlerting(const PString & /*calleeName*/, PBoolean /*withMedia*/)
{
  return false;
}


PBoolean OpalLyncConnection::SetConnected()
{
  return false;
}


OpalTransportAddress OpalLyncConnection::GetRemoteAddress() const
{
  return OpalTransportAddress();
}



#endif // OPAL_LYNC

// End of File /////////////////////////////////////////////////////////////
