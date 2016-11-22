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

#if PTRACING
void OpalLyncShimBase::OnTraceOutput(unsigned level, const char * file, unsigned line, const std::string & out)
{
  if (PTrace::CanTrace(level))
    PTrace::Begin(level, file, line, nullptr, "LyncShim") << out << PTrace::End;
}
#endif

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


void OpalLyncEndPoint::OnIncomingLyncCall(AudioVideoCall * avCall)
{
  PTRACE(3, "Incoming Lync call: " << avCall);
  PSafePtr<OpalCall> call = m_manager.InternalCreateCall();
  if (call == NULL) {
    DestroyAudioVideoCall(avCall);
    return;
  }
  AddConnection(CreateConnection(*call, avCall, "", nullptr, 0, nullptr));
}


PSafePtr<OpalConnection> OpalLyncEndPoint::MakeConnection(OpalCall & call,
                                                          const PString & uri,
                                                          void * userData,
                                                          unsigned int options,
                                                          OpalConnection::StringOptions * stringOptions)
{
  return AddConnection(CreateConnection(call, nullptr, uri, userData, options, stringOptions));
}


OpalLyncConnection * OpalLyncEndPoint::CreateConnection(OpalCall & call,
                                                        AudioVideoCall * avCall,
                                                        const PString & uri,
                                                        void * userData,
                                                        unsigned int options,
                                                        OpalConnection::StringOptions * stringOptions)
{
  return new OpalLyncConnection(call, *this, avCall, uri, userData, options, stringOptions);
}


PBoolean OpalLyncEndPoint::GarbageCollection()
{
  return OpalEndPoint::GarbageCollection();
}


bool OpalLyncEndPoint::Register(const RegistrationInfo & info)
{
  if (info.m_uri.IsEmpty()) {
    PTRACE(2, "Must proivide a URI for registration.");
    return false;
  }

  PWaitAndSignal lock(m_registrationMutex);

  if (m_platform == nullptr) {
    m_platform = CreatePlatform(PProcess::Current().GetName());
    if (m_platform == nullptr) {
      PTRACE(2, "Error initialising Lync UCMA platform: " << GetLastError());
      return false;
    }
    PTRACE(3, "Created Lync UCMA platform.");
  }

  RegistrationMap::iterator it = m_registrations.find(info.m_uri);
  if (it != m_registrations.end())
    return false;

  PURL url(info.m_uri, "sip");

  PString authID = info.m_authID;
  if (authID.IsEmpty())
    authID = url.GetUserName();

  PString domain = info.m_domain;
  if (domain.IsEmpty()) {
    domain = url.GetHostName();
    domain.Delete(domain.Find(".com"), P_MAX_INDEX);
  }

  UserEndpoint * user = CreateUserEndpoint(*m_platform, info.m_uri, info.m_password, authID, domain);
  if (user == NULL) {
    PTRACE(2, "Error registering \"" << info.m_uri << "\" as Lync UCMA user: " << GetLastError());
    return false;
  }

  if (m_registrations.empty())
    SetDefaultLocalPartyName(info.m_uri);

  m_registrations[info.m_uri] = user;
  PTRACE(3, "Registered \"" << info.m_uri << "\" as Lync UCMA.");
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

  RegistrationMap::iterator it = m_registrations.find(uri.IsEmpty() || uri == "*" ? GetDefaultLocalPartyName() : uri);
  return it != m_registrations.end() ? it->second : nullptr;
}


///////////////////////////////////////////////////////////////////////

OpalLyncConnection::OpalLyncConnection(OpalCall & call,
                                       OpalLyncEndPoint & ep,
                                       AudioVideoCall * avCall,
                                       const PString & uri,
                                       void * /*userData*/,
                                       unsigned options,
                                       OpalConnection::StringOptions * stringOptions)
  : OpalConnection(call, ep, ep.GetManager().GetNextToken('L'), options, stringOptions)
  , m_endpoint(ep)
  , m_conversation(nullptr)
  , m_audioVideoCall(avCall)
{
  m_remotePartyURL = uri;
}


PBoolean OpalLyncConnection::SetUpConnection()
{
  PTRACE(3, "SetUpConnection call on " << *this << " to \"" << GetRemotePartyURL() << '"');

  SetPhase(SetUpPhase);
  InternalSetAsOriginating();

  PString localParty = m_stringOptions(OPAL_OPT_CALLING_PARTY_URL, GetLocalPartyName());
  OpalLyncShim::UserEndpoint * uep = m_endpoint.GetRegistration(localParty);
  if (uep == nullptr) {
    PTRACE(2, "Cannot find registration for user: " << localParty);
    Release(EndedByGkAdmissionFailed);
    return false;
  }

  m_conversation = CreateConversation(*uep);
  if (m_conversation == nullptr) {
    PTRACE(2, "Error creating Lync UCMA conversation: " << GetLastError());
    return false;
  }

  PString otherParty = GetRemotePartyURL();
  if (otherParty.NumCompare(m_endpoint.GetPrefixName()+':') == EqualTo)
    otherParty.Replace(m_endpoint.GetPrefixName(), "sip");
  else
    otherParty.Splice("sip:", 0);

  m_audioVideoCall = CreateAudioVideoCall(*m_conversation, otherParty, false);
  if (m_audioVideoCall == nullptr) {
    PTRACE(2, "Error creating Lync UCMA conversation: " << GetLastError());
    Release(EndedByIllegalAddress);
    return false;
  }

  PTRACE(3, "Lync UCMA call from " << GetLocalPartyURL() << " to " << GetRemotePartyURL() << " started.");
  return true;
}


void OpalLyncConnection::OnLyncCallStateChanged(int previousState, int newState)
{
  PTRACE(3, "Lync UCMA call stat changed from " << previousState << " to " << newState);

  if (newState == CallStateEstablishing)
    OnProceeding();
  else if (newState == CallStateTerminating)
    Release(EndedByRemoteUser);
  else if (newState == CallStateEstablished) {
      m_flow = CreateAudioVideoFlow(*m_audioVideoCall);
      InternalOnEstablished();
  }
}


void OpalLyncConnection::OnLyncCallFailed(const std::string & PTRACE_PARAM(error))
{
  PTRACE(2, "Error establishing Lync UCMA call: " << error);
  Release(EndedByConnectFail);
}


void OpalLyncConnection::OnReleased()
{
  DestroyAudioVideoFlow(m_flow);
  m_flow = nullptr;

  DestroyAudioVideoCall(m_audioVideoCall);
  m_audioVideoCall = nullptr;

  DestroyConversation(m_conversation);
  m_conversation = nullptr;

  OpalConnection::OnReleased();
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


OpalMediaStream * OpalLyncConnection::CreateMediaStream(const OpalMediaFormat & mediaFormat,
                                                        unsigned sessionID,
                                                        PBoolean isSource)
{
  return new OpalLyncMediaStream(*this, mediaFormat, sessionID, isSource);
}


void OpalLyncConnection::OnMediaFlowStateChanged(int previousState, int newState)
{
  PTRACE(3, "Lync UCMA media flow state changed from " << previousState << " to " << newState);
}


///////////////////////////////////////////////////////////////////////

OpalLyncMediaStream::OpalLyncMediaStream(OpalLyncConnection & conn,
                                         const OpalMediaFormat & mediaFormat,
                                         unsigned sessionID,
                                         bool isSource)
  : OpalMediaStream(conn, mediaFormat, sessionID, isSource)
  , m_connection(conn)
  , m_inputConnector(nullptr)
  , m_outputConnector(nullptr)
  , m_avStream(nullptr)
{
}


OpalLyncMediaStream::~OpalLyncMediaStream()
{
  Close();
}


PBoolean OpalLyncMediaStream::Open()
{
  AudioVideoFlow * flow = m_connection.GetAudioVideoFlow();
  if (flow == nullptr)
    return false;

  if (IsSource()) {
    m_inputConnector = CreateSpeechRecognitionConnector(*flow);
    if (m_inputConnector == nullptr) {
      PTRACE(2, "Error creating Lync UCMA SpeechRecognitionConnector: " << GetLastError());
      return false;
    }
    m_avStream = CreateAudioVideoStream(*m_inputConnector);
  }
  else {
    m_outputConnector = CreateSpeechSynthesisConnector(*flow);
    if (m_outputConnector == nullptr) {
      PTRACE(2, "Error creating Lync UCMA SpeechSynthesisConnector: " << GetLastError());
      return false;
    }
    m_avStream = CreateAudioVideoStream(*m_outputConnector);
  }

  if (m_avStream == nullptr) {
    PTRACE(2, "Error creating Lync UCMA Stream: " << GetLastError());
    return false;
  }

  PTRACE(3, "Opened Lync UCMA stream.");
  return true;
}


void OpalLyncMediaStream::InternalClose()
{
  DestroyAudioVideoStream(m_avStream);
  m_avStream = nullptr;

  DestroySpeechRecognitionConnector(m_inputConnector);
  m_inputConnector = nullptr;

  DestroySpeechSynthesisConnector(m_outputConnector);
  m_outputConnector = nullptr;
}


PBoolean OpalLyncMediaStream::ReadData(BYTE * data, PINDEX size, PINDEX & length)
{
  if (m_avStream == NULL)
    return false;
  
  int result = ReadAudioVideoStream(*m_avStream, data, size);
  if (result < 0) {
    PTRACE(2, "Error reading Lync UCMA Stream: " << GetLastError());
    return false;
  }

  length = result;
  return true;
}


PBoolean OpalLyncMediaStream::WriteData(const BYTE * data, PINDEX length, PINDEX & written)
{
  if (m_avStream == NULL)
    return false;
  
  int result = WriteAudioVideoStream(*m_avStream, data, length);
  if (result < 0) {
    PTRACE(2, "Error writing Lync UCMA Stream: " << GetLastError());
    return false;
  }

  written = result;
  return true;
}


PBoolean OpalLyncMediaStream::IsSynchronous() const
{
  return IsSource();
}


#endif // OPAL_LYNC

// End of File /////////////////////////////////////////////////////////////
