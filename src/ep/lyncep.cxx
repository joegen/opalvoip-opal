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
void OpalLyncShimBase::OnTraceOutput(unsigned level, const char * file, unsigned line, const char * out)
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
}


OpalMediaFormatList OpalLyncEndPoint::GetMediaFormats() const
{
  // just raw audio for now, then work out what Lync supports, somehow ...
  OpalMediaFormatList formats;
  formats += OpalPCM16;
  return formats;
}


void OpalLyncEndPoint::OnIncomingLyncCall(const IncomingLyncCallInfo & info)
{
  PTRACE(3, "Incoming Lync call: " << info.m_remoteUri);

  PSafePtr<OpalCall> call = m_manager.InternalCreateCall();
  if (call != NULL) {
    OpalLyncConnection * connection = CreateConnection(*call, nullptr, 0, nullptr);
    if (AddConnection(connection)) {
      connection->SetUpIncomingLyncCall(info);
      return;
    }
  }

  DestroyAudioVideoCall(const_cast<IncomingLyncCallInfo&>(info).m_call);
}


PSafePtr<OpalConnection> OpalLyncEndPoint::MakeConnection(OpalCall & call,
                                                          const PString & remote,
                                                          void * userData,
                                                          unsigned int options,
                                                          OpalConnection::StringOptions * stringOptions)
{
  OpalConnection * connection = AddConnection(CreateConnection(call, userData, options, stringOptions));
  if (connection != NULL)
    connection->SetRemotePartyName(remote);
  return connection;
}


OpalLyncConnection * OpalLyncEndPoint::CreateConnection(OpalCall & call,
                                                        void * userData,
                                                        unsigned int options,
                                                        OpalConnection::StringOptions * stringOptions)
{
  return new OpalLyncConnection(call, *this, userData, options, stringOptions);
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

  PURL url(info.m_uri.NumCompare(GetPrefixName()+':') == EqualTo
                   ? info.m_uri.Mid(GetPrefixName().GetLength()+1) : info.m_uri, "sip");

  RegistrationMap::iterator it = m_registrations.find(url);
  if (it != m_registrations.end())
    return false;

  PString authID = info.m_authID;
  if (authID.IsEmpty())
    authID = url.GetUserName();

  PString domain = info.m_domain;
  if (domain.IsEmpty()) {
    domain = url.GetHostName();
    domain.Delete(domain.Find(".com"), P_MAX_INDEX);
  }

  UserEndpoint * user = CreateUserEndpoint(*m_platform, url.AsString(), info.m_password, authID, domain);
  if (user == NULL) {
    PTRACE(2, "Error registering \"" << info.m_uri << "\""
              " (" << url << ","
              " authID=\"" << authID << "\","
              " domain=\"" << domain << "\")"
              " as Lync UCMA user: " << GetLastError());
    return false;
  }

  if (m_registrations.empty())
    SetDefaultLocalPartyName(url);

  m_registrations[url] = user;
  PTRACE(3, "Registered \"" << url << "\" as Lync UCMA.");
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


PStringArray OpalLyncEndPoint::GetRegisteredURIs() const
{
  PWaitAndSignal lock(m_registrationMutex);
  PStringArray uris(m_registrations.size());
  PINDEX i = 0;
  for (RegistrationMap::const_iterator it = m_registrations.begin(); it != m_registrations.end(); ++it)
    uris[i++] = it->first;
  return uris;
}


///////////////////////////////////////////////////////////////////////

OpalLyncConnection::OpalLyncConnection(OpalCall & call,
                                       OpalLyncEndPoint & ep,
                                       void * /*userData*/,
                                       unsigned options,
                                       OpalConnection::StringOptions * stringOptions)
  : OpalConnection(call, ep, ep.GetManager().GetNextToken('L'), options, stringOptions)
  , m_endpoint(ep)
  , m_conversation(nullptr)
  , m_audioVideoCall(nullptr)
  , m_flow(nullptr)
{
}


void OpalLyncConnection::SetUpIncomingLyncCall(const IncomingLyncCallInfo & info)
{
  m_audioVideoCall = info.m_call;

  if (GetPhase() == UninitialisedPhase) {
    m_remotePartyURL = info.m_remoteUri;
    m_remotePartyName = info.m_displayName;
    m_calledPartyName = info.m_destinationUri;
    m_redirectingParty = info.m_transferredBy;
    SetPhase(SetUpPhase);
    OnApplyStringOptions();
    if (OnIncomingConnection(0, NULL))
      m_ownerCall.OnSetUp(*this);
  }
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


void OpalLyncConnection::OnLyncCallStateChanged(int PTRACE_PARAM(previousState), int newState)
{
  PTRACE(3, "Lync UCMA call stat changed from " << previousState << " to " << newState);

  if (newState == CallStateEstablishing)
    OnProceeding();
  else if (newState == CallStateTerminating) {
    if (!IsOriginating() || GetPhase() > SetUpPhase)
      Release(EndedByRemoteUser);
    // else get OnLyncCallFailed() with more info on why
  }
  else if (newState == CallStateEstablished) {
      m_flow = CreateAudioVideoFlow(*m_audioVideoCall);
      InternalOnConnected();
  }
}


void OpalLyncConnection::OnLyncCallFailed(const std::string & error)
{
  PTRACE(2, "Failed to establish Lync UCMA call: " << error);

  if (error.find("ResponseCode=480") != std::string::npos)
    Release(EndedByTemporaryFailure);
  else if (error.find("ResponseCode=60") != std::string::npos)
    Release(EndedByRefusal);
  else
    Release(EndedByConnectFail);
}


void OpalLyncConnection::OnReleased()
{
  DestroyAudioVideoFlow(m_flow);
  DestroyAudioVideoCall(m_audioVideoCall);
  DestroyConversation(m_conversation);

  OpalConnection::OnReleased();
}


OpalMediaFormatList OpalLyncConnection::GetMediaFormats() const
{
  return m_endpoint.GetMediaFormats();
}


PBoolean OpalLyncConnection::SetAlerting(const PString & /*calleeName*/, PBoolean /*withMedia*/)
{
  return true;
}


PBoolean OpalLyncConnection::SetConnected()
{
  if (!PAssert(m_audioVideoCall != nullptr, PLogicError))
    return false;

  if (AcceptAudioVideoCall(*m_audioVideoCall)) {
    PTRACE(3, "Answered Lync UCMA call on " << *this);
    return true;
  }

  PTRACE(2, "Failed to accept incoming Lync UCMA call: " << GetLastError());
  return false;
}


OpalMediaStream * OpalLyncConnection::CreateMediaStream(const OpalMediaFormat & mediaFormat,
                                                        unsigned sessionID,
                                                        PBoolean isSource)
{
  return new OpalLyncMediaStream(*this, mediaFormat, sessionID, isSource);
}


void OpalLyncConnection::OnMediaFlowStateChanged(int PTRACE_PARAM(previousState), int newState)
{
  PTRACE(3, "Lync UCMA media flow state changed from " << previousState << " to " << newState);

  if (newState == MediaFlowActive) {
    AutoStartMediaStreams();
    InternalOnEstablished();
  }
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
  if (IsOpen())
    return true;

  AudioVideoFlow * flow = m_connection.GetAudioVideoFlow();
  if (flow == nullptr) {
    PTRACE(2, "Cannot open media stream " << *this << ", Lync UCMA flow not yet started.");
    return false;
  }

  if (IsSource()) {
    m_inputConnector = CreateSpeechRecognitionConnector(*flow);
    if (m_inputConnector == nullptr) {
      PTRACE(2, "Error creating Lync UCMA SpeechRecognitionConnector for " << *this << ": " << GetLastError());
      return false;
    }
    m_avStream = CreateAudioVideoStream(*m_inputConnector);
  }
  else {
    m_outputConnector = CreateSpeechSynthesisConnector(*flow);
    if (m_outputConnector == nullptr) {
      PTRACE(2, "Error creating Lync UCMA SpeechSynthesisConnector for " << *this << ": " << GetLastError());
      return false;
    }
    m_avStream = CreateAudioVideoStream(*m_outputConnector);
  }

  if (m_avStream == nullptr) {
    PTRACE(2, "Error creating Lync UCMA Stream for " << *this << ": " << GetLastError());
    return false;
  }

  PTRACE(3, "Opened Lync UCMA stream " << *this);
  return OpalMediaStream::Open();
}


void OpalLyncMediaStream::InternalClose()
{
  if (m_inputConnector != nullptr) {
    DestroyAudioVideoStream(*m_inputConnector, m_avStream);
    DestroySpeechRecognitionConnector(m_inputConnector);
  }

  if (m_outputConnector != nullptr) {
    DestroyAudioVideoStream(*m_outputConnector, m_avStream);
    DestroySpeechSynthesisConnector(m_outputConnector);
  }
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
