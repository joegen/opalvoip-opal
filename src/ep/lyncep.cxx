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
  , m_applicationRegistration(nullptr)
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

  DestroyApplicationEndpoint(m_applicationRegistration);

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
  PSafePtr<OpalCall> call = m_manager.InternalCreateCall();
  if (call != NULL) {
    OpalLyncConnection * connection = CreateConnection(*call, nullptr, 0, nullptr);
    if (AddConnection(connection)) {
      connection->SetUpIncomingLyncCall(info);
      return;
    }
  }

  PTRACE(2, "Incoming Lync call unexpectedly failed.");

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


bool OpalLyncEndPoint::OnApplicationProvisioning(ApplicationEndpoint * aep)
{
  if (m_applicationRegistration != nullptr) {
    PTRACE(2, "Cannot provision, already registered for application.");
    return false;
  }

  if (!OpalLyncShim::OnApplicationProvisioning(aep))
    return false;

  m_applicationRegistration = aep;
  return true;
}


bool OpalLyncEndPoint::Register(const PString & provisioningID)
{
  if (m_applicationRegistration != nullptr) {
    PTRACE(2, "Already registered for application.");
    return false;
  }

  if (m_platform != nullptr) {
    PTRACE(2, "Already registered for users.");
    return false;
  }

  m_platform = CreatePlatform(PProcess::Current().GetName(), provisioningID);
  if (m_platform == nullptr) {
    PTRACE(2, "Error initialising Lync UCMA platform: " << GetLastError());
    return false;
  }

  PTRACE(3, "Created Lync UCMA platform.");
  return true;
}


bool OpalLyncEndPoint::RegisterApplication(const PlatformParams & platformParams, const ApplicationParams & appParams)
{
  if (m_applicationRegistration != nullptr) {
    PTRACE(2, "Already registered for application.");
    return false;
  }

  if (m_platform != nullptr) {
    PTRACE(2, "Already registered for users.");
    return false;
  }

  m_platform = CreatePlatform(PProcess::Current().GetName(), platformParams);
  if (m_platform == nullptr) {
    PTRACE(2, "Error initialising Lync UCMA platform: " << GetLastError());
    return false;
  }

  PTRACE(3, "Created Lync UCMA platform.");

  m_applicationRegistration = CreateApplicationEndpoint(*m_platform, appParams);
  if (m_applicationRegistration == nullptr) {
    PTRACE(2, "Error initialising Lync UCMA application endpoint: " << GetLastError());
    DestroyPlatform(m_platform);
    return false;
  }

  PTRACE(3, "Created Lync UCMA application endpoint.");
  return true;
}


PString OpalLyncEndPoint::RegisterUser(const UserParams & info)
{
  if (info.m_uri.IsEmpty()) {
    PTRACE(2, "Must proivide a URI for registration.");
    return PString::Empty();
  }

  PWaitAndSignal lock(m_registrationMutex);

  if (m_platform == nullptr) {
    PlatformParams params;
    m_platform = CreatePlatform(PProcess::Current().GetName());
    if (m_platform == nullptr) {
      PTRACE(2, "Error initialising Lync UCMA platform: " << GetLastError());
      return PString::Empty();
    }
    PTRACE(3, "Created Lync UCMA platform.");
  }

  PString uriStr = info.m_uri;
  AdjustLyncURI(uriStr);
  PURL url(uriStr);

  RegistrationMap::iterator it = m_registrations.find(url);
  if (it != m_registrations.end())
    return PString::Empty();

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
    return PString::Empty();
  }

  if (m_registrations.empty())
    SetDefaultLocalPartyName(url);

  m_registrations[url] = user;
  PTRACE(3, "Registered \"" << url << "\" as Lync UCMA.");
  return url;
}


bool OpalLyncEndPoint::UnregisterUser(const PString & uri)
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


OpalLyncShim::UserEndpoint * OpalLyncEndPoint::GetRegisteredUser(const PString & uri) const
{
  PWaitAndSignal lock(m_registrationMutex);

  RegistrationMap::const_iterator it = m_registrations.find(uri.IsEmpty() || uri == "*" ? GetDefaultLocalPartyName() : uri);
  return it != m_registrations.end() ? it->second : nullptr;
}


PStringArray OpalLyncEndPoint::GetRegisteredUsers() const
{
  PWaitAndSignal lock(m_registrationMutex);
  PStringArray uris(m_registrations.size());
  PINDEX i = 0;
  for (RegistrationMap::const_iterator it = m_registrations.begin(); it != m_registrations.end(); ++it)
    uris[i++] = it->first;
  return uris;
}


void OpalLyncEndPoint::AdjustLyncURI(PString & uri)
{
  if (uri.NumCompare(GetPrefixName()+':') == EqualTo)
    uri.Splice("sip", 0, GetPrefixName().GetLength());
  else if (uri.NumCompare("sip:") != EqualTo)
    uri.Splice("sip:", 0);
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
  , m_toneController(nullptr)
  , m_mediaActive(false)
{
}


static PString ToOpalLyncURI(const PString & uri, const PString & prefix)
{
  if (uri.NumCompare("sip:") == PObject::EqualTo)
    return prefix + uri.Mid(3);
  
  return prefix + ':' + uri;
}

void OpalLyncConnection::SetUpIncomingLyncCall(const IncomingLyncCallInfo & info)
{
  if (GetPhase() != UninitialisedPhase) {
    PTRACE(2, "Unexpected SetUpIncomingLyncCall for " << *this);
    return;
  }

  PTRACE(3, "Incoming Lync call:"
            " from=\"" << info.m_remoteUri << "\","
            " name=\"" << info.m_displayName << "\","
            " dest=\"" << info.m_destinationUri << "\","
            " by=\"" << info.m_transferredBy << "\","
            " conn=" << *this);

  m_audioVideoCall = info.m_call;

  m_remotePartyURL = ToOpalLyncURI(info.m_remoteUri, GetPrefixName());
  m_remotePartyNumber = m_remotePartyURL(m_remotePartyURL.Find(':')+1, m_remotePartyURL.Find('@')-1);
  if (!OpalIsE164(m_remotePartyNumber))
    m_remotePartyNumber.MakeEmpty();
  m_remotePartyName = info.m_displayName;
  m_calledPartyName = info.m_destinationUri;
  m_redirectingParty = ToOpalLyncURI(info.m_transferredBy, GetPrefixName());

  SetPhase(SetUpPhase);
  OnApplyStringOptions();

  if (OnIncomingConnection(0, NULL))
    m_ownerCall.OnSetUp(*this);
}


PBoolean OpalLyncConnection::SetUpConnection()
{
  PTRACE(3, "SetUpConnection call on " << *this << " to \"" << GetRemotePartyURL() << '"');

  SetPhase(SetUpPhase);
  InternalSetAsOriginating();

  PString localParty = m_stringOptions(OPAL_OPT_CALLING_PARTY_URL, GetLocalPartyName());
  m_endpoint.AdjustLyncURI(localParty);
  if (OpalIsE164(localParty(localParty.Find(':')+1, localParty.Find('@')-1)))
    localParty += ";user=phone";

  ApplicationEndpoint * aep = m_endpoint.GetRegisteredApplication();
  if (aep != NULL)
    m_conversation = CreateConversation(*aep, localParty, nullptr, m_displayName);
  else {
    OpalLyncShim::UserEndpoint * uep = m_endpoint.GetRegisteredUser(localParty);
    if (uep == nullptr) {
      PTRACE(2, "Cannot find registration for user: " << localParty);
      Release(EndedByGkAdmissionFailed);
      return false;
    }

    m_conversation = CreateConversation(*uep);
  }

  if (m_conversation == nullptr) {
    PTRACE(2, "Error creating Lync UCMA conversation: " << GetLastError());
    return false;
  }

  PString otherParty = GetRemotePartyURL();
  m_endpoint.AdjustLyncURI(otherParty);

  m_audioVideoCall = CreateAudioVideoCall(*m_conversation, otherParty);
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
  PTRACE(3, "Lync UCMA call state changed from " << GetCallStateName(previousState) << " to " << GetCallStateName(newState));

  if (newState == CallStateEstablishing) {
    if (IsOriginating())
      OnProceeding();
    return;
  }

  if (newState == CallStateEstablished) {
    m_flow = CreateAudioVideoFlow(*m_audioVideoCall);
    if (m_flow != NULL) {
      InternalOnConnected();
      if (m_mediaActive) {
        AutoStartMediaStreams();
        InternalOnEstablished();
      }
    }
    else {
      PTRACE(2, "Error creating Lync UCMA A/V flow: " << GetLastError());
      Release(EndedByCallerAbort);
    }
    return;
  }

  if (newState == CallStateTerminating && (!IsOriginating() || GetPhase() > SetUpPhase))
    Release(EndedByRemoteUser);
  // else get OnLyncCallFailed() with more info on why
}


void OpalLyncConnection::OnLyncCallFailed(const std::string & error)
{
  PTRACE(2, "Failed to establish Lync UCMA call: " << error);

  if (error.find("ResponseCode=404") != std::string::npos)
    Release(EndedByNoUser);
  else if (error.find("ResponseCode=480") != std::string::npos)
    Release(EndedByTemporaryFailure);
  else if (error.find("ResponseCode=60") != std::string::npos)
    Release(EndedByRefusal);
  else
    Release(EndedByConnectFail);
}


void OpalLyncConnection::OnReleased()
{
  OpalConnection::OnReleased();

  PTRACE(4, "DestroyToneController " << *this);
  DestroyToneController(m_toneController);

  PTRACE(4, "DestroyAudioVideoFlow " << *this);
  DestroyAudioVideoFlow(m_flow);

  PTRACE(4, "DestroyAudioVideoCall " << *this);
  DestroyAudioVideoCall(m_audioVideoCall);

  PTRACE(4, "DestroyConversation " << *this);
  DestroyConversation(m_conversation);
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


bool OpalLyncConnection::TransferConnection(const PString & remoteParty)
{
  if (!PAssert(m_audioVideoCall != nullptr, PLogicError))
    return false;

  if (!IsEstablished()) {
    PTRACE(4, "Not yet established, cannot transfer Lync UCMA call on " << *this << " to \"" << remoteParty << '"');
    return false;
  }

  PSafePtr<OpalLyncConnection> otherConnection = m_endpoint.GetConnectionWithLockAs<OpalLyncConnection>(remoteParty, PSafeReadOnly);
  if (otherConnection != NULL) {
    if (TransferAudioVideoCall(*m_audioVideoCall, *otherConnection->m_audioVideoCall)) {
      PTRACE(3, "Transferred Lync UCMA call on " << *this << " to " << *otherConnection);
      otherConnection->Release(EndedByCallForwarded);
      Release(EndedByCallForwarded);
      return true;
    }
  }
  else {
    PString uri = remoteParty;
    m_endpoint.AdjustLyncURI(uri);
    if (TransferAudioVideoCall(*m_audioVideoCall, uri)) {
      PTRACE(3, "Transferred Lync UCMA call on " << *this << " to \"" << remoteParty << '"');
      Release(EndedByCallForwarded);
      return true;
    }
  }

  PTRACE(2, "Failed to transfer Lync UCMA call to \"" << remoteParty << "\" : " << GetLastError());
  return false;
}


void OpalLyncConnection::OnLyncCallTransferReceived(const std::string & transferDestination, const std::string & tansferredBy) 
{
  PTRACE(3, "Lync UCMA call Transfer Received - destination " << transferDestination << " by " << tansferredBy);
}


void OpalLyncConnection::OnLyncCallTransferStateChanged(int previousState, int newState) 
{
  PTRACE(3, "Lync UCMA call transfer state changed from " << GetTransferStateName(previousState) << " to " << GetTransferStateName(newState));
}


OpalMediaStream * OpalLyncConnection::CreateMediaStream(const OpalMediaFormat & mediaFormat,
                                                        unsigned sessionID,
                                                        PBoolean isSource)
{
  return new OpalLyncMediaStream(*this, mediaFormat, sessionID, isSource);
}


void OpalLyncConnection::OnMediaFlowStateChanged(int previousState, int newState)
{
  PTRACE(3, "Lync UCMA media flow state changed from " << GetMediaFlowName(previousState) << " to " << GetMediaFlowName(newState));

  m_mediaActive = newState == MediaFlowActive;
  if (m_mediaActive && previousState != MediaFlowActive && !IsReleased()) {
    m_toneController = CreateToneController(*m_flow);
    PTRACE_IF(2, m_toneController == nullptr, "Error creating Lync UCMA ToneController for " << *this << ": " << GetLastError());

    AutoStartMediaStreams();
    InternalOnEstablished();
  }
}


static PConstString const ToneChars("0123456789*#ABCD!");

void OpalLyncConnection::OnLyncCallToneReceived(int toneId, float volume)
{
  PTRACE(4, "OnLyncToneReceived: toneId=" << toneId << ", DTMF=" << GetToneIdName(toneId) << ", volume=" << volume);
  if (toneId >= 0 && toneId < ToneChars.GetLength())
    OnUserInputTone(ToneChars[toneId], 100);
}


void OpalLyncConnection::OnLyncCallIncomingFaxDetected()
{
  PTRACE(4, "OnLyncIncomingFaxDetected");
}


PBoolean OpalLyncConnection::SendUserInputTone(char tone, unsigned /*duration*/)
{
  if (m_toneController == nullptr) {
    PTRACE(2, "Tone Controller not available yet.");
    return false;
  }

  PINDEX toneId = ToneChars.Find(tone);
  if (!PAssert(toneId != P_MAX_INDEX, PInvalidParameter))
    return false;

  if (SendTone(*m_toneController, toneId))
    return true;

  PTRACE(2, "Could not send tone to Lync UCMA connectiobn " << *this << " : " << GetLastError());
  return false;
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
  , m_silence(10*sizeof(short)*mediaFormat.GetTimeUnits()) // At least 10ms
  , m_closing(false)
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

  if (m_connection.IsReleased())
    return false;

  if (!m_connection.m_mediaActive || m_connection.m_flow == nullptr) {
    PTRACE(2, "Cannot open media stream " << *this << ", Lync UCMA flow not yet active.");
    /* Return true so does not fail setting up the stream, but as we are not really
       open, a later call to Open () will trya again and maybe succeed. */
    return true;
  }

  if (IsSource()) {
    m_inputConnector = CreateSpeechRecognitionConnector(*m_connection.m_flow);
    if (m_inputConnector == nullptr) {
      PTRACE(2, "Error creating Lync UCMA SpeechRecognitionConnector for " << *this << ": " << GetLastError());
      return false;
    }

    m_avStream = CreateAudioVideoStream(*m_inputConnector);
  }
  else {
    m_outputConnector = CreateSpeechSynthesisConnector(*m_connection.m_flow);
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


bool OpalLyncMediaStream::IsEstablished() const
{
  return m_avStream != nullptr;
}


void OpalLyncMediaStream::InternalClose()
{
  m_closing = true;

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
  if (m_closing || m_connection.IsReleased())
    return false;

  length = 0;
  if (!m_connection.m_mediaActive || m_connection.m_flow == nullptr)
    return true;

  if (!Open())
    return false;

  int result = ReadAudioVideoStream(*m_avStream, data, size);
  if (result < 0) {
    PTRACE(2, "Lync UCMA Stream (" << *this << ") read error: " << GetLastError());
    return false;
  }

  length = result;
  return true;
}


PBoolean OpalLyncMediaStream::WriteData(const BYTE * data, PINDEX length, PINDEX & written)
{
  if (m_closing || m_connection.IsReleased())
    return false;

  written = 0;
  if (!m_connection.m_mediaActive || m_connection.m_flow == nullptr) {
    PThread::Sleep(10);
    return true;
  }

  if (!Open())
    return false;

  if (data != NULL && length != 0)
    m_silence.SetMinSize(length);
  else {
    length = m_silence.GetSize();
    data = m_silence;
    PTRACE(6, "Playing silence " << length << " bytes");
  }

  int result = WriteAudioVideoStream(*m_avStream, data, length);
  if (result < 0) {
    PTRACE(2, "Lync UCMA Stream (" << *this << ") write error: " << GetLastError());
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
