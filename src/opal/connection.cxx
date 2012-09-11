/*
 * connection.cxx
 *
 * Connection abstraction
 *
 * Open Phone Abstraction Library (OPAL)
 * Formally known as the Open H323 project.
 *
 * Copyright (c) 2001 Equivalence Pty. Ltd.
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
 * The Original Code is Open Phone Abstraction Library.
 *
 * The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "connection.h"
#endif

#include <opal/buildopts.h>

#include <opal/connection.h>

#include <opal/manager.h>
#include <opal/endpoint.h>
#include <opal/call.h>
#include <opal/transcoders.h>
#include <opal/patch.h>
#include <codec/silencedetect.h>
#include <codec/echocancel.h>
#include <codec/rfc2833.h>
#include <codec/g711codec.h>
#include <codec/vidcodec.h>
#include <rtp/rtp.h>
#include <t120/t120proto.h>
#include <t38/t38proto.h>
#include <im/rfc4103.h>
#include <ptclib/url.h>


#define new PNEW


#if PTRACING

ostream & operator<<(ostream & out, OpalConnection::CallEndReason reason)
{
  if (reason.code != OpalConnection::EndedByQ931Cause)
    return out << reason.code;
  else
    return out << "Q.931[0x" << hex << reason.q931 << dec << ']';
}

#endif


static POrdinalToString::Initialiser const CallEndReasonStringsInitialiser[] = {
  { OpalConnection::EndedByLocalUser,            "Local party cleared call" },
  { OpalConnection::EndedByNoAccept,             "Local party did not accept call" },
  { OpalConnection::EndedByAnswerDenied,         "Local party declined to answer call" },
  { OpalConnection::EndedByRemoteUser,           "Remote party cleared call" },
  { OpalConnection::EndedByRefusal,              "Remote party refused call" },
  { OpalConnection::EndedByNoAnswer,             "Remote party did not answer in required time" },
  { OpalConnection::EndedByCallerAbort,          "Remote party stopped calling" },
  { OpalConnection::EndedByTransportFail,        "Call failed due to a transport error" },
  { OpalConnection::EndedByConnectFail,          "Connection to remote failed" },
  { OpalConnection::EndedByGatekeeper,           "Gatekeeper has cleared call" },
  { OpalConnection::EndedByNoUser,               "Call failed as could not find user" },
  { OpalConnection::EndedByNoBandwidth,          "Call failed due to insufficient bandwidth" },
  { OpalConnection::EndedByCapabilityExchange,   "Call failed as could not find common media capabilities" },
  { OpalConnection::EndedByCallForwarded,        "Call was forwarded" },
  { OpalConnection::EndedBySecurityDenial,       "Call failed security check" },
  { OpalConnection::EndedByLocalBusy,            "Local party busy" },
  { OpalConnection::EndedByLocalCongestion,      "Local party congested" },
  { OpalConnection::EndedByRemoteBusy,           "Remote party busy" },
  { OpalConnection::EndedByRemoteCongestion,     "Remote switch congested" },
  { OpalConnection::EndedByUnreachable,          "Remote party could not be reached" },
  { OpalConnection::EndedByNoEndPoint,           "Remote party application is not running" },
  { OpalConnection::EndedByHostOffline,          "Remote party host is off line" },
  { OpalConnection::EndedByTemporaryFailure,     "Remote system failed temporarily" },
  { OpalConnection::EndedByQ931Cause,            "Call cleared with Q.931 cause code %u" },
  { OpalConnection::EndedByDurationLimit,        "Call cleared due to an enforced duration limit" },
  { OpalConnection::EndedByInvalidConferenceID,  "Call cleared due to invalid conference ID" },
  { OpalConnection::EndedByNoDialTone,           "Call cleared due to missing dial tone" },
  { OpalConnection::EndedByNoRingBackTone,       "Call cleared due to missing ringback tone" },
  { OpalConnection::EndedByOutOfService,         "Call cleared because the line is out of service" },
  { OpalConnection::EndedByAcceptingCallWaiting, "Call cleared because another call is answered" },
  { OpalConnection::EndedByGkAdmissionFailed,    "Call cleared because gatekeeper admission request failed." },
  { OpalConnection::EndedByMediaFailed,          "Call cleared due to loss of media flow." },
};

static POrdinalToString CallEndReasonStrings(PARRAYSIZE(CallEndReasonStringsInitialiser), CallEndReasonStringsInitialiser);


/////////////////////////////////////////////////////////////////////////////

#ifdef _MSC_VER
#pragma warning(disable:4355)
#endif

OpalConnection::OpalConnection(OpalCall & call,
                               OpalEndPoint  & ep,
                               const PString & token,
                               unsigned int options,
                               OpalConnection::StringOptions * stringOptions)
  : PSafeObject(&call)  // Share the lock flag from the call
  , ownerCall(call)
  , endpoint(ep)
  , m_phase(UninitialisedPhase)
  , callToken(token)
  , m_originating(false)
  , productInfo(ep.GetProductInfo())
  , localPartyName(ep.GetDefaultLocalPartyName())
  , displayName(ep.GetDefaultDisplayName())
  , remotePartyName(token)
  , callEndReason(NumCallEndReasons)
  , silenceDetector(NULL)
#if OPAL_AEC
  , echoCanceler(NULL)
#endif
#if OPAL_PTLIB_DTMF
  , m_minAudioJitterDelay(endpoint.GetManager().GetMinAudioJitterDelay())
  , m_maxAudioJitterDelay(endpoint.GetManager().GetMaxAudioJitterDelay())
  , m_rxBandwidthAvailable(endpoint.GetInitialBandwidth(OpalBandwidth::Rx))
  , m_txBandwidthAvailable(endpoint.GetInitialBandwidth(OpalBandwidth::Tx))
  , m_dtmfScaleMultiplier(1)
  , m_dtmfScaleDivisor(1)
  , m_dtmfDetectNotifier(PCREATE_NOTIFIER(OnDetectInBandDTMF))
  , m_sendInBandDTMF(true)
  , m_emittedInBandDTMF(0)
#endif
#if OPAL_PTLIB_DTMF
  , m_dtmfSendNotifier(PCREATE_NOTIFIER(OnSendInBandDTMF))
#endif
#if OPAL_HAS_MIXER
  , m_recordAudioNotifier(PCREATE_NOTIFIER(OnRecordAudio))
#if OPAL_VIDEO
  , m_recordVideoNotifier(PCREATE_NOTIFIER(OnRecordVideo))
#endif
#endif
#if OPAL_STATISTICS
  , m_VideoUpdateRequestsSent(0)
#endif
{
  PTRACE_CONTEXT_ID_FROM(call);

  PTRACE(3, "OpalCon\tCreated connection " << *this);

  PAssert(ownerCall.SafeReference(), PLogicError);

  ownerCall.connectionsActive.Append(this);

  if (stringOptions != NULL)
    m_stringOptions = *stringOptions;

#if OPAL_PTLIB_DTMF
  switch (options&DetectInBandDTMFOptionMask) {
    case DetectInBandDTMFOptionDisable :
      m_detectInBandDTMF = false;
      break;

    case DetectInBandDTMFOptionEnable :
      m_detectInBandDTMF = true;
      break;

    default :
      m_detectInBandDTMF = !endpoint.GetManager().DetectInBandDTMFDisabled();
      break;
  }
#endif

  switch (options & SendDTMFMask) {
    case SendDTMFAsString:
      sendUserInputMode = SendUserInputAsString;
      break;
    case SendDTMFAsTone:
      sendUserInputMode = SendUserInputAsTone;
      break;
    case SendDTMFAsRFC2833:
      sendUserInputMode = SendUserInputAsRFC2833;
      break;
    case SendDTMFAsDefault:
    default:
      sendUserInputMode = ep.GetSendUserInputMode();
      break;
  }

#if 0 // OPAL_HAS_IM
  m_rfc4103Context[0].SetMediaFormat(OpalT140);
  m_rfc4103Context[1].SetMediaFormat(OpalT140);
#endif

#if OPAL_SCRIPT
  PScriptLanguage * script = endpoint.GetManager().GetScript();
  if (script != NULL) {
    m_scriptTableName  = OPAL_SCRIPT_CALL_TABLE_NAME "." + ownerCall.GetToken() + '[' + GetToken().ToLiteral() + ']';
    script->CreateComposite(m_scriptTableName);
    script->SetFunction(m_scriptTableName + ".Release", PCREATE_NOTIFIER(ScriptRelease));
    script->SetFunction(m_scriptTableName + ".SetOption", PCREATE_NOTIFIER(ScriptSetOption));
    script->SetFunction(m_scriptTableName + ".GetLocalPartyURL", PCREATE_NOTIFIER(ScriptGetLocalPartyURL));
    script->SetFunction(m_scriptTableName + ".GetRemotePartyURL", PCREATE_NOTIFIER(ScriptGetRemotePartyURL));
    script->SetFunction(m_scriptTableName + ".GetCalledPartyURL", PCREATE_NOTIFIER(ScriptGetCalledPartyURL));
    script->SetFunction(m_scriptTableName + ".GetRedirectingParty", PCREATE_NOTIFIER(ScriptGetRedirectingParty));
    script->SetString  (m_scriptTableName + ".callToken", call.GetToken());
    script->SetString  (m_scriptTableName + ".connectionToken", GetToken());
    script->SetString  (m_scriptTableName + ".prefix", GetPrefixName());
    script->SetBoolean (m_scriptTableName + ".originating", false);
    script->Call("OnNewConnection", "ss", (const char *)call.GetToken(), (const char *)GetToken());
  }
#endif // OPAL_SCRIPT

  m_phaseTime[UninitialisedPhase].SetCurrentTime();
}

#ifdef _MSC_VER
#pragma warning(default:4355)
#endif

OpalConnection::~OpalConnection()
{
  mediaStreams.RemoveAll();

#if OPAL_SCRIPT
  PScriptLanguage * script = endpoint.GetManager().GetScript();
  if (script != NULL) {
    script->Call("OnDestroyConnection", "ss", (const char *)ownerCall.GetToken(), (const char *)GetToken());
    script->ReleaseVariable(m_scriptTableName);
  }
#endif

  delete silenceDetector;
#if OPAL_AEC
  delete echoCanceler;
#endif
#if OPAL_T120DATA
  delete t120handler;
#endif

  ownerCall.connectionsActive.Remove(this);
  ownerCall.SafeDereference();

  PTRACE(3, "OpalCon\tConnection " << *this << " destroyed.");
}


void OpalConnection::PrintOn(ostream & strm) const
{
  strm << ownerCall << '-'<< endpoint << '[' << callToken << ']';
}


bool OpalConnection::GarbageCollection()
{
  return mediaStreams.DeleteObjectsToBeRemoved();
}


void OpalConnection::InternalSetAsOriginating()
{
  m_originating = true;
#if OPAL_SCRIPT
  PScriptLanguage * script = endpoint.GetManager().GetScript();
  if (script != NULL)
    script->SetBoolean(m_scriptTableName  + ".originating", true);
#endif

}


PBoolean OpalConnection::SetUpConnection()
{
  InternalSetAsOriginating();

  // Check if we are A-Party in this call, so need to do things differently
  if (ownerCall.GetConnection(0) == this) {
    SetPhase(SetUpPhase);
    if (!OnIncomingConnection(0, NULL)) {
      Release(EndedByNoUser);
      return false;
    }

    PTRACE(3, "OpalCon\tOutgoing call routed to " << ownerCall.GetPartyB() << " for " << *this);
    if (!ownerCall.OnSetUp(*this)) {
      Release(EndedByNoAccept);
      return false;
    }
  }
  else if (ownerCall.IsEstablished()) {
    PTRACE(3, "OpalCon\tTransfer of connection in call " << ownerCall);
    OnApplyStringOptions();
    AutoStartMediaStreams();
    OnConnectedInternal();
  }
  else {
    PTRACE(3, "OpalCon\tIncoming call from " << remotePartyName);

    OnApplyStringOptions();
  }

  return true;
}


PBoolean OpalConnection::OnSetUpConnection()
{
  PTRACE(3, "OpalCon\tOnSetUpConnection" << *this);
  return endpoint.OnSetUpConnection(*this);
}


bool OpalConnection::Hold(bool /*fromRemote*/, bool /*placeOnHold*/)
{
  return false;
}


PBoolean OpalConnection::IsOnHold(bool /*fromRemote*/)
{
  return false;
}


void OpalConnection::OnHold(bool fromRemote, bool onHold)
{
  PTRACE(4, "OpalCon\tOnHold " << *this);
  endpoint.OnHold(*this, fromRemote, onHold);
}


PString OpalConnection::GetCallEndReasonText(CallEndReason reason)
{
  return psprintf(CallEndReasonStrings(reason.code), reason.q931);
}


void OpalConnection::SetCallEndReasonText(CallEndReasonCodes reasonCode, const PString & newText)
{
  CallEndReasonStrings.SetAt(reasonCode, newText);
}


void OpalConnection::SetCallEndReason(CallEndReason reason)
{
  PWaitAndSignal mutex(m_phaseMutex);

  // Only set reason if not already set to something
  if (callEndReason == NumCallEndReasons) {
    PTRACE(3, "OpalCon\tCall end reason for " << *this << " set to " << reason);
    callEndReason = reason;
    ownerCall.SetCallEndReason(reason);
  }
}


void OpalConnection::ClearCall(CallEndReason reason, PSyncPoint * sync)
{
  SetCallEndReason(reason);
  ownerCall.Clear(reason, sync);
}


void OpalConnection::ClearCallSynchronous(PSyncPoint * sync, CallEndReason reason)
{
  SetCallEndReason(reason);

  PSyncPoint syncPoint;
  if (sync == NULL)
    sync = &syncPoint;

  ClearCall(reason, sync);

  PTRACE(5, "OpalCon\tSynchronous wait for " << *this);
  sync->Wait();
}


bool OpalConnection::TransferConnection(const PString & PTRACE_PARAM(remoteParty))
{
  PTRACE(2, "OpalCon\tCan not transfer connection to " << remoteParty);
  return false;
}


void OpalConnection::Release(CallEndReason reason, bool synchronous)
{
  {
    PWaitAndSignal mutex(m_phaseMutex);
    if (IsReleased()) {
      PTRACE(3, "OpalCon\tAlready released " << *this);
      return;
    }
    SetPhase(ReleasingPhase);
    SetCallEndReason(reason);
  }

  if (synchronous) {
    PTRACE(3, "OpalCon\tReleasing synchronously " << *this);
    OnReleased();
    return;
  }

  PTRACE(3, "OpalCon\tReleasing asynchronously " << *this);

  // Add a reference for the thread we are about to start
  SafeReference();
  PThread::Create(PCREATE_NOTIFIER(OnReleaseThreadMain), 0,
                  PThread::AutoDeleteThread,
                  PThread::NormalPriority,
                  "OnRelease");
}


void OpalConnection::OnReleaseThreadMain(PThread & PTRACE_PARAM(thread), INT)
{
  PTRACE_CONTEXT_ID_TO(thread);

  OnReleased();

  PTRACE(4, "OpalCon\tOnRelease thread completed for " << *this);

  // Dereference on the way out of the thread
  SafeDereference();
}


void OpalConnection::OnReleased()
{
  PTRACE(4, "OpalCon\tOnReleased " << *this);

  CloseMediaStreams();

  endpoint.OnReleased(*this);

  SetPhase(ReleasedPhase);

#if PTRACING
  static const int Level = 3;
  if (PTrace::CanTrace(Level)) {
    ostream & trace = PTrace::Begin(Level, __FILE__, __LINE__, this);
    trace << "OpalCon\tConnection " << *this << " released\n"
             "        Initial Time: " << m_phaseTime[UninitialisedPhase] << '\n';
    for (Phases ph = SetUpPhase; ph < NumPhases; ph = (Phases)(ph+1)) {
      trace << setw(20) << ph << ": ";
      if (m_phaseTime[ph].IsValid())
        trace << (m_phaseTime[ph]-m_phaseTime[UninitialisedPhase]);
      else
        trace << "N/A";
      trace << '\n';
    }
    trace << "     Call end reason: " << callEndReason << '\n'
          << PTrace::End;
  }
#endif
}


PBoolean OpalConnection::OnIncomingConnection(unsigned options, OpalConnection::StringOptions * stringOptions)
{
  return endpoint.OnIncomingConnection(*this, options, stringOptions);
}


PString OpalConnection::GetDestinationAddress()
{
  return '*';
}


PBoolean OpalConnection::ForwardCall(const PString & /*forwardParty*/)
{
  return PFalse;
}


PSafePtr<OpalConnection> OpalConnection::GetOtherPartyConnection() const
{
  return GetCall().GetOtherPartyConnection(*this);
}


void OpalConnection::OnProceeding()
{
  endpoint.OnProceeding(*this);
}

void OpalConnection::OnAlerting()
{
  endpoint.OnAlerting(*this);
}


PBoolean OpalConnection::SetAlerting(const PString &, PBoolean)
{
  return true;
}


OpalConnection::AnswerCallResponse OpalConnection::OnAnswerCall(const PString & callerName)
{
  return endpoint.OnAnswerCall(*this, callerName);
}


void OpalConnection::AnsweringCall(AnswerCallResponse response)
{
  PTRACE(3, "OpalCon\tAnswering call: " << response);

  PSafeLockReadWrite safeLock(*this);
  // Can only answer call in UninitialisedPhase, SetUpPhase and AlertingPhase phases
  if (!safeLock.IsLocked() || GetPhase() > AlertingPhase)
    return;

  switch (response) {
    case AnswerCallDenied :
      // If response is denied, abort the call
      Release(EndedByAnswerDenied);
      break;

    case AnswerCallAlertWithMedia :
      SetAlerting(GetLocalPartyName(), PTrue);
      break;

    case AnswerCallPending :
      SetAlerting(GetLocalPartyName(), PFalse);
      break;

    case AnswerCallNow: 
      PTRACE(3, "OpalCon\tApplication has answered incoming call");
      GetOtherPartyConnection()->OnConnectedInternal();
      break;

    default : // AnswerCallDeferred etc
      break;
  }
}


PBoolean OpalConnection::SetConnected()
{
  PTRACE(3, "OpalCon\tSetConnected for " << *this);

  if (GetPhase() < ConnectedPhase)
    SetPhase(ConnectedPhase);

  if (!mediaStreams.IsEmpty() && GetPhase() < EstablishedPhase) {
    SetPhase(EstablishedPhase);
    OnEstablished();
  }

  return true;
}


void OpalConnection::OnConnectedInternal()
{
  if (GetPhase() < ConnectedPhase) {
    SetPhase(ConnectedPhase);
    OnConnected();
  }

  if (!mediaStreams.IsEmpty() && GetPhase() < EstablishedPhase) {
    SetPhase(EstablishedPhase);
    OnEstablished();
  }
}


void OpalConnection::OnConnected()
{
  PTRACE(3, "OpalCon\tOnConnected for " << *this);
  endpoint.OnConnected(*this);
}


void OpalConnection::OnEstablished()
{
  PTRACE(3, "OpalCon\tOnEstablished " << *this);
  ownerCall.StartMediaStreams();
  endpoint.OnEstablished(*this);
}


bool OpalConnection::OnTransferNotify(const PStringToString & info,
                                      const OpalConnection * transferringConnection)
{
  if (transferringConnection == this) {
    PSafePtr<OpalConnection> otherConnection = GetOtherPartyConnection();
    if (otherConnection != NULL)
      otherConnection->OnTransferNotify(info, transferringConnection);
  }

  return endpoint.OnTransferNotify(*this, info);

}


void OpalConnection::AdjustMediaFormats(bool   local,
                        const OpalConnection * otherConnection,
                         OpalMediaFormatList & mediaFormats) const
{
  if (otherConnection != NULL)
    return;

  mediaFormats.Remove(m_stringOptions(OPAL_OPT_REMOVE_CODEC).Lines());

  if (local) {
    for (PStringToString::const_iterator it = m_stringOptions.begin(); it != m_stringOptions.end(); ++it) {
      PString key = it->first;
      PString fmtName, optName;
      if (key.Split(':', fmtName, optName) && !fmtName.IsEmpty() && !optName.IsEmpty()) {
        PString optValue = it->second;
        OpalMediaFormatList::const_iterator iterFormat;
        while ((iterFormat = mediaFormats.FindFormat(fmtName, iterFormat)) != mediaFormats.end()) {
          OpalMediaFormat & format = const_cast<OpalMediaFormat &>(*iterFormat);
          if (format.SetOptionValue(optName, optValue)) {
            PTRACE(4, "OpalCon\tSet media format " << format
                    << " option " << optName << " to \"" << optValue << '"');
          }
          else {
            PTRACE(2, "OpalCon\tFailed to set media format " << format
                    << " option " << optName << " to \"" << optValue << '"');
          }
        }
      }
    }
  }

  endpoint.AdjustMediaFormats(local, *this, mediaFormats);
}


unsigned OpalConnection::GetNextSessionID(const OpalMediaType & /*mediaType*/, bool /*isSource*/)
{
  return 0;
}


void OpalConnection::AutoStartMediaStreams(bool force)
{
  PTRACE(4, "OpalCon\tAutoStartMediaStreams(" << force << ") on " << *this);
  OpalMediaTypeList mediaTypes = OpalMediaType::GetList();
  for (OpalMediaTypeList::iterator iter = mediaTypes.begin(); iter != mediaTypes.end(); ++iter) {
    OpalMediaType mediaType = *iter;
    if ((GetAutoStart(mediaType)&OpalMediaType::Transmit) != 0 && (force || GetMediaStream(mediaType, true) == NULL))
      ownerCall.OpenSourceMediaStreams(*this, mediaType, mediaType->GetDefaultSessionId());
  }
  StartMediaStreams();
}


#if OPAL_T38_CAPABILITY
bool OpalConnection::SwitchT38(bool)
{
  PAssertAlways(PUnimplementedFunction);
  return false;
}


void OpalConnection::OnSwitchedT38(bool toT38, bool success)
{
  if (!PAssert(ownerCall.IsSwitchingT38(), PLogicError))
    return;

  ownerCall.ResetSwitchingT38();

  PTRACE(3, "OpalCon\tSwitch of media streams to "
         << (toT38 ? "T.38" : "audio") << ' '
         << (success ? "succeeded" : "failed")
         << " on " << *this);

  PSafePtr<OpalConnection> other = GetOtherPartyConnection();
  if (other != NULL)
    other->OnSwitchedT38(toT38, success);

  if (success || IsReleased())
    return;

  if (toT38) {
    PTRACE(4, "OpalCon\tSwitch request to fax failed, falling back to audio mode");
    SwitchT38(false);
  }
  else {
    PTRACE(3, "OpalCon\tSwitch request back to audio mode failed.");
    Release();
  }
}

void OpalConnection::OnSwitchingT38(bool toT38)
{
  if (ownerCall.IsSwitchingT38())
    return;

  PTRACE(3, "OpalCon\tRemote requests switching media streams to "
         << (toT38 ? "T.38" : "audio") << " on " << *this);

  ownerCall.SetSwitchingT38(toT38);

  PSafePtr<OpalConnection> other = GetOtherPartyConnection();
  if (other != NULL)
    other->OnSwitchingT38(toT38);
}
#endif // OPAL_T38_CAPABILITY


OpalMediaStreamPtr OpalConnection::OpenMediaStream(const OpalMediaFormat & mediaFormat, unsigned sessionID, bool isSource)
{
  PSafeLockReadWrite safeLock(*this);
  if (!safeLock.IsLocked())
    return NULL;

  // See if already opened
  OpalMediaStreamPtr stream = GetMediaStream(sessionID, isSource);
  if (stream != NULL && stream->IsOpen()) {
    if (stream->GetMediaFormat() == mediaFormat) {
      PTRACE(3, "OpalCon\tOpenMediaStream (already opened) for session " << sessionID << " on " << *this);
      return stream;
    }
    // Changing the media format, needs to close and re-open the stream
    stream->Close();
    stream.SetNULL();
  }

  if (stream == NULL) {
    stream = CreateMediaStream(mediaFormat, sessionID, isSource);
    if (stream == NULL) {
      PTRACE(1, "OpalCon\tCreateMediaStream returned NULL for session " << sessionID << " on " << *this);
      return NULL;
    }
    mediaStreams.Append(stream);
  }

  if (stream->Open()) {
    if (OnOpenMediaStream(*stream)) {
      PTRACE(3, "OpalCon\tOpened " << (isSource ? "source" : "sink") << " stream " << stream->GetID() << " with format " << mediaFormat);
      return stream;
    }
    PTRACE(2, "OpalCon\tOnOpenMediaStream failed for " << mediaFormat << ", closing " << *stream);
    stream->Close();
  }
  else {
    PTRACE(2, "OpalCon\tSource media stream open failed for " << *stream << " (" << mediaFormat << ')');
  }

  mediaStreams.Remove(stream);

  return NULL;
}


bool OpalConnection::CloseMediaStream(unsigned sessionId, bool source)
{
  OpalMediaStreamPtr stream = GetMediaStream(sessionId, source);
  return stream != NULL && stream->IsOpen() && CloseMediaStream(*stream);
}


bool OpalConnection::CloseMediaStream(OpalMediaStream & stream)
{
  return stream.Close();
}


PBoolean OpalConnection::RemoveMediaStream(OpalMediaStream & stream)
{
  stream.Close();
  PTRACE(3, "OpalCon\tRemoved media stream " << stream);
  return mediaStreams.Remove(&stream);
}


void OpalConnection::StartMediaStreams()
{
  for (OpalMediaStreamPtr mediaStream = mediaStreams; mediaStream != NULL; ++mediaStream)
    mediaStream->Start();

  PTRACE(3, "OpalCon\tMedia stream threads started for " << *this);
}


void OpalConnection::CloseMediaStreams()
{
  // Do this double loop as while closing streams, the instance may disappear from the
  // mediaStreams list, prematurely stopping the for loop.
  bool someOpen = true;
  while (someOpen) {
    someOpen = false;
    for (OpalMediaStreamPtr mediaStream(mediaStreams, PSafeReference); mediaStream != NULL; ++mediaStream) {
      if (mediaStream->IsOpen()) {
        someOpen = true;
        CloseMediaStream(*mediaStream);
      }
    }
  }

  PTRACE(3, "OpalCon\tMedia streams closed.");
}


void OpalConnection::PauseMediaStreams(bool paused)
{
  for (OpalMediaStreamPtr mediaStream = mediaStreams; mediaStream != NULL; ++mediaStream)
    mediaStream->SetPaused(paused);
}


void OpalConnection::OnPauseMediaStream(OpalMediaStream & /*strm*/, bool /*paused*/)
{
}


#if OPAL_VIDEO
OpalMediaStream * OpalConnection::CreateMediaStream(const OpalMediaFormat & mediaFormat,
                                                    unsigned sessionID,
                                                    PBoolean isSource)
{
  if (mediaFormat.GetMediaType() == OpalMediaType::Video()) {
    if (isSource) {
      PVideoInputDevice * videoDevice;
      PBoolean autoDeleteGrabber;
      if (CreateVideoInputDevice(mediaFormat, videoDevice, autoDeleteGrabber)) {
        PTRACE(4, "OpalCon\tCreated capture device \"" << videoDevice->GetDeviceName() << '"');

        PVideoOutputDevice * previewDevice;
        PBoolean autoDeletePreview;
        if (CreateVideoOutputDevice(mediaFormat, PTrue, previewDevice, autoDeletePreview))
          PTRACE(4, "OpalCon\tCreated preview device \"" << previewDevice->GetDeviceName() << '"');
        else
          previewDevice = NULL;

        return new OpalVideoMediaStream(*this, mediaFormat, sessionID, videoDevice, previewDevice, autoDeleteGrabber, autoDeletePreview);
      }
    }
    else {
      PVideoOutputDevice * videoDevice;
      PBoolean autoDelete;
      if (CreateVideoOutputDevice(mediaFormat, PFalse, videoDevice, autoDelete)) {
        PTRACE(4, "OpalCon\tCreated display device \"" << videoDevice->GetDeviceName() << '"');
        return new OpalVideoMediaStream(*this, mediaFormat, sessionID, NULL, videoDevice, false, autoDelete);
      }
    }
  }

#if OPAL_HAS_RFC4103
  if (mediaFormat.GetMediaType() == OpalT140.GetMediaType())
    return new OpalT140MediaStream(*this, mediaFormat, sessionID, isSource);
#endif

  return NULL;
}
#else
OpalMediaStream * OpalConnection::CreateMediaStream(const OpalMediaFormat &, unsigned, PBoolean)
{
  return NULL;
}
#endif


PBoolean OpalConnection::OnOpenMediaStream(OpalMediaStream & stream)
{
  if (!endpoint.OnOpenMediaStream(*this, stream))
    return PFalse;

  if (!LockReadWrite())
    return PFalse;

  if (GetPhase() == ConnectedPhase) {
    SetPhase(EstablishedPhase);
    OnEstablished();
  }

  UnlockReadWrite();

  return PTrue;
}


void OpalConnection::OnClosedMediaStream(const OpalMediaStream & stream)
{
  OpalMediaPatchPtr patch = stream.GetPatch();
  if (patch != NULL) {
#if OPAL_HAS_MIXER
    OnStopRecording(patch);
#endif

    if (silenceDetector != NULL && patch->RemoveFilter(silenceDetector->GetReceiveHandler(), m_filterMediaFormat)) {
      PTRACE(4, "OpalCon\tRemoved silence detect filter on connection " << *this << ", patch " << patch);
    }

#if OPAL_AEC
    if (echoCanceler != NULL && patch->RemoveFilter(stream.IsSource() ? echoCanceler->GetReceiveHandler()
                                                  : echoCanceler->GetSendHandler(), m_filterMediaFormat)) {
      PTRACE(4, "OpalCon\tRemoved echo canceler filter on connection " << *this << ", patch " << patch);
    }
#endif

#if OPAL_PTLIB_DTMF
    if (patch->RemoveFilter(m_dtmfDetectNotifier, OpalPCM16)) {
      PTRACE(4, "OpalCon\tRemoved detect DTMF filter on connection " << *this << ", patch " << patch);
    }
    if (!m_dtmfSendFormat.IsEmpty() && patch->RemoveFilter(m_dtmfSendNotifier, m_dtmfSendFormat)) {
      PTRACE(4, "OpalCon\tRemoved DTMF send filter on connection " << *this << ", patch " << patch);
    }
#endif
  }

  endpoint.OnClosedMediaStream(stream);
}

#if OPAL_HAS_MIXER

static PString MakeRecordingKey(const OpalMediaPatch & patch)
{
  return psprintf("%08x", &patch);
}


void OpalConnection::OnStartRecording(OpalMediaPatch * patch)
{
  if (patch == NULL)
    return;

  if (!ownerCall.OnStartRecording(MakeRecordingKey(*patch), patch->GetSource().GetMediaFormat())) {
    PTRACE(4, "OpalCon\tNo record filter added on connection " << *this << ", patch " << *patch);
    return;
  }

  patch->AddFilter(m_recordAudioNotifier, OpalPCM16);
#if OPAL_VIDEO
  patch->AddFilter(m_recordVideoNotifier, OPAL_YUV420P);
#endif

  PTRACE(4, "OpalCon\tAdded record filter on connection " << *this << ", patch " << *patch);
}


void OpalConnection::OnStopRecording(OpalMediaPatch * patch)
{
  if (patch == NULL)
    return;

  ownerCall.OnStopRecording(MakeRecordingKey(*patch));

  patch->RemoveFilter(m_recordAudioNotifier, OpalPCM16);
#if OPAL_VIDEO
  patch->RemoveFilter(m_recordVideoNotifier, OPAL_YUV420P);
#endif

  PTRACE(4, "OpalCon\tRemoved record filter on " << *patch);
}

#endif

void OpalConnection::OnPatchMediaStream(PBoolean isSource, OpalMediaPatch & patch)
{
  OpalMediaFormat mediaFormat = isSource ? patch.GetSource().GetMediaFormat() : patch.GetSink()->GetMediaFormat();

  if (mediaFormat.GetMediaType() == OpalMediaType::Audio()) {
    if (!mediaFormat.IsTransportable()) {
      m_filterMediaFormat = mediaFormat;

      if (isSource && silenceDetector != NULL) {
        silenceDetector->SetParameters(endpoint.GetManager().GetSilenceDetectParams(), mediaFormat.GetClockRate());
        patch.AddFilter(silenceDetector->GetReceiveHandler(), mediaFormat);
        PTRACE(4, "OpalCon\tAdded silence detect filter on connection " << *this << ", patch " << patch);
      }

#if OPAL_AEC
      if (echoCanceler) {
        echoCanceler->SetParameters(endpoint.GetManager().GetEchoCancelParams());
        echoCanceler->SetClockRate(mediaFormat.GetClockRate());
        patch.AddFilter(isSource ? echoCanceler->GetReceiveHandler()
                                 : echoCanceler->GetSendHandler(), mediaFormat);
        PTRACE(4, "OpalCon\tAdded echo canceler filter on connection " << *this << ", patch " << patch);
      }
#endif
    }

#if OPAL_PTLIB_DTMF
    if (m_detectInBandDTMF && isSource) {
      patch.AddFilter(m_dtmfDetectNotifier, OpalPCM16);
      PTRACE(4, "OpalCon\tAdded detect DTMF filter on connection " << *this << ", patch " << patch);
    }

    if (m_sendInBandDTMF && !isSource) {
      if (mediaFormat == OpalG711_ULAW_64K || mediaFormat == OpalG711_ALAW_64K)
        m_dtmfSendFormat = mediaFormat;
      else
        m_dtmfSendFormat = OpalPCM16;
      patch.AddFilter(m_dtmfSendNotifier, mediaFormat);
      PTRACE(4, "OpalCon\tAdded send DTMF filter on connection " << *this << ", patch " << patch);
    }
#endif
  }

#if OPAL_HAS_MIXER
  if (!m_recordingFilename.IsEmpty())
    ownerCall.StartRecording(m_recordingFilename);
  else if (ownerCall.IsRecording())
    OnStartRecording(&patch);
#endif

  PTRACE(3, "OpalCon\t" << (isSource ? "Source" : "Sink") << " stream of connection " << *this << " uses patch " << patch);
}


#if OPAL_HAS_MIXER
void OpalConnection::EnableRecording()
{
  OpalMediaStreamPtr stream = GetMediaStream(OpalMediaType::Audio(), true);
  if (stream != NULL)
    OnStartRecording(stream->GetPatch());

#if OPAL_VIDEO
  stream = GetMediaStream(OpalMediaType::Video(), true);
  if (stream != NULL)
    OnStartRecording(stream->GetPatch());
#endif
}


void OpalConnection::DisableRecording()
{
  OpalMediaStreamPtr stream = GetMediaStream(OpalMediaType::Audio(), true);
  if (stream != NULL)
    OnStopRecording(stream->GetPatch());

#if OPAL_VIDEO
  stream = GetMediaStream(OpalMediaType::Video(), true);
  if (stream != NULL)
    OnStopRecording(stream->GetPatch());
#endif
}


void OpalConnection::OnRecordAudio(RTP_DataFrame & frame, INT param)
{
  const OpalMediaPatch * patch = (const OpalMediaPatch *)param;
  ownerCall.OnRecordAudio(MakeRecordingKey(*patch), frame);
}


#if OPAL_VIDEO

void OpalConnection::OnRecordVideo(RTP_DataFrame & frame, INT param)
{
  const OpalMediaPatch * patch = (const OpalMediaPatch *)param;
  ownerCall.OnRecordVideo(MakeRecordingKey(*patch), frame);
}

#endif

#endif


OpalMediaStreamPtr OpalConnection::GetMediaStream(const PString & streamID, bool source) const
{
  for (PSafePtr<OpalMediaStream> mediaStream(mediaStreams, PSafeReference); mediaStream != NULL; ++mediaStream) {
    if ((streamID.IsEmpty() || mediaStream->GetID() == streamID) && mediaStream->IsSource() == source)
      return mediaStream;
  }

  return NULL;
}


OpalMediaStreamPtr OpalConnection::GetMediaStream(unsigned sessionId, bool source) const
{
  for (OpalMediaStreamPtr mediaStream(mediaStreams, PSafeReference); mediaStream != NULL; ++mediaStream) {
    if (mediaStream->GetSessionID() == sessionId && mediaStream->IsSource() == source)
      return mediaStream;
  }

  return NULL;
}


OpalMediaStreamPtr OpalConnection::GetMediaStream(const OpalMediaType & mediaType,
                                                  bool source,
                                                  OpalMediaStreamPtr mediaStream) const
{
  if (mediaStream == NULL)
    mediaStream = OpalMediaStreamPtr(mediaStreams, PSafeReference);
  else
    ++mediaStream;

  while (mediaStream != NULL) {
    if ((mediaType.empty() || mediaStream->GetMediaFormat().GetMediaType() == mediaType) && mediaStream->IsSource() == source)
      return mediaStream;
    ++mediaStream;
  }

  return NULL;
}


#if P_NAT
PNatMethod * OpalConnection::GetNatMethod (const PIPSocket::Address & remoteAddress) const
{
  return endpoint.GetNatMethod(remoteAddress);
}
#endif


bool OpalConnection::GetMediaTransportAddresses(const OpalMediaType & PTRACE_PARAM(mediaType),
                                          OpalTransportAddressArray &) const
{
  PTRACE(4, "OpalCon\tGetMediaTransportAddresses for " << mediaType << " not allowed.");
  return false;
}


#if OPAL_VIDEO

PBoolean OpalConnection::CreateVideoInputDevice(const OpalMediaFormat & mediaFormat,
                                            PVideoInputDevice * & device,
                                            PBoolean & autoDelete)
{
  return endpoint.CreateVideoInputDevice(*this, mediaFormat, device, autoDelete);
}


PBoolean OpalConnection::CreateVideoOutputDevice(const OpalMediaFormat & mediaFormat,
                                             PBoolean preview,
                                             PVideoOutputDevice * & device,
                                             PBoolean & autoDelete)
{
  return endpoint.CreateVideoOutputDevice(*this, mediaFormat, preview, device, autoDelete);
}


bool OpalConnection::SendVideoUpdatePicture(unsigned sessionID, bool force) const
{
  if (!ExecuteMediaCommand(force ? OpalVideoUpdatePicture() : OpalVideoPictureLoss(), sessionID, OpalMediaType::Video()))
    return false;

  PTRACE(3, "OpalCon\tVideo update picture (I-Frame) requested on " << *this);
  return true;
}


void OpalConnection::OnRxIntraFrameRequest(const OpalMediaSession & session, bool force)
{
  GetEndPoint().GetManager().QueueDecoupledEvent(new PSafeWorkArg2<OpalConnection, unsigned, bool>(
              this, session.GetSessionID(), force, &OpalConnection::SendVideoUpdatePictureCallback));
}

#endif // OPAL_VIDEO


PBoolean OpalConnection::SetAudioVolume(PBoolean /*source*/, unsigned /*percentage*/)
{
  return PFalse;
}

PBoolean OpalConnection::GetAudioVolume(PBoolean /*source*/, unsigned & /*percentage*/)
{
  return false;
}


bool OpalConnection::SetAudioMute(bool /*source*/, bool /*mute*/)
{
  return false;
}


bool OpalConnection::GetAudioMute(bool /*source*/, bool & /*mute*/)
{
  return false;
}


unsigned OpalConnection::GetAudioSignalLevel(PBoolean /*source*/)
{
    return UINT_MAX;
}


OpalBandwidth OpalConnection::GetBandwidthAvailable(OpalBandwidth::Direction dir) const
{
  switch (dir) {
    case OpalBandwidth::Rx :
      return m_rxBandwidthAvailable;
    case OpalBandwidth::Tx :
      return m_txBandwidthAvailable;
    default :
      return m_rxBandwidthAvailable+m_txBandwidthAvailable;
  }
}


bool OpalConnection::SetBandwidthAvailable(OpalBandwidth::Direction dir, OpalBandwidth newBandwidth)
{
  OpalBandwidth used = GetBandwidthUsed(dir);
  if (used > newBandwidth) {
    PTRACE(2, "OpalCon\tCannot set " << dir << " bandwidth to " << newBandwidth
           << ", currently using " << used << " on connection " << *this);
    return false;
  }

  PTRACE(3, "OpalCon\tSetting " << dir << " bandwidth to " << newBandwidth << " on connection " << *this);

  OpalBandwidth available = newBandwidth - used;
  switch (dir) {
    case OpalBandwidth::Rx :
      m_rxBandwidthAvailable = available;
      break;

    case OpalBandwidth::Tx :
      m_txBandwidthAvailable = available;
      break;

    default :
      OpalBandwidth rx = (PUInt64)(unsigned)available*m_rxBandwidthAvailable/(m_rxBandwidthAvailable+m_txBandwidthAvailable);
      OpalBandwidth tx = (PUInt64)(unsigned)available*m_txBandwidthAvailable/(m_rxBandwidthAvailable+m_txBandwidthAvailable);
      m_rxBandwidthAvailable = rx;
      m_txBandwidthAvailable = tx;
  }

  return true;
}


OpalBandwidth OpalConnection::GetBandwidthUsed(OpalBandwidth::Direction dir) const
{
  OpalBandwidth used = 0;
  OpalMediaStreamPtr stream;

  switch (dir) {
    case OpalBandwidth::Rx :
      for (stream = GetMediaStream(PString::Empty(), true); stream != NULL; ++stream)
        used += stream->GetMediaFormat().GetUsedBandwidth();
      break;

    case OpalBandwidth::Tx :
      for (stream = GetMediaStream(PString::Empty(), false); stream != NULL; ++stream)
        used += stream->GetMediaFormat().GetUsedBandwidth();
      break;

    default :
      for (stream = GetMediaStream(PString::Empty(), true); stream != NULL; ++stream)
        used += stream->GetMediaFormat().GetUsedBandwidth();
      for (stream = GetMediaStream(PString::Empty(), false); stream != NULL; ++stream)
        used += stream->GetMediaFormat().GetUsedBandwidth();
  }

  PTRACE(4, "OpalCon\tUsing " << dir << " bandwidth of " << used << " for " << *this);

  return used;
}


bool OpalConnection::SetBandwidthUsed(OpalBandwidth::Direction dir,
                                      OpalBandwidth releasedBandwidth,
                                      OpalBandwidth requiredBandwidth)
{
  PTRACE_IF(3, releasedBandwidth > 0, "OpalCon\tReleasing " << dir << " bandwidth of " << releasedBandwidth);

  OpalBandwidth bandwidthAvailable = GetBandwidthAvailable(dir);

  bandwidthAvailable += releasedBandwidth;

  PTRACE_IF(3, requiredBandwidth > 0, "OpalCon\tRequesting " << dir << " bandwidth of "
            << requiredBandwidth << ", available: " << bandwidthAvailable);

  return SetBandwidthAvailable(dir, OpalBandwidth(bandwidthAvailable - requiredBandwidth));
}


void OpalConnection::SetSendUserInputMode(SendUserInputModes mode)
{
  PTRACE(3, "OPAL\tSetting default User Input send mode to " << mode);
  sendUserInputMode = mode;
}


PBoolean OpalConnection::SendUserInputString(const PString & value)
{
  for (const char * c = value; *c != '\0'; c++) {
    if (!SendUserInputTone(*c, 0))
      return PFalse;
  }
  return PTrue;
}


#if OPAL_PTLIB_DTMF
PBoolean OpalConnection::SendUserInputTone(char tone, unsigned duration)
{
  if (m_dtmfSendFormat.IsEmpty())
    return false;

  if (duration <= 0)
    duration = PDTMFEncoder::DefaultToneLen;

  PTRACE(3, "OPAL\tSending in-band DTMF tone '" << tone << "', duration=" << duration);

  PDTMFEncoder dtmfSamples;
  dtmfSamples.AddTone(tone, duration);
  PINDEX size = dtmfSamples.GetSize();

  m_inBandMutex.Wait();

  switch (m_dtmfSendFormat.GetPayloadType()) {
    case RTP_DataFrame::PCMU :
      if (m_inBandDTMF.SetSize(size)) {
        for (PINDEX i = 0; i < size; ++i)
          m_inBandDTMF[i] = (BYTE)Opal_PCM_G711_uLaw::ConvertSample(dtmfSamples[i]);
      }
      break;

    case RTP_DataFrame::PCMA :
      if (m_inBandDTMF.SetSize(size)) {
        for (PINDEX i = 0; i < size; ++i)
          m_inBandDTMF[i] = (BYTE)Opal_PCM_G711_ALaw::ConvertSample(dtmfSamples[i]);
      }
      break;

    default :
      size *= sizeof(short);
      if (m_inBandDTMF.SetSize(size))
        memcpy(m_inBandDTMF.GetPointer(), dtmfSamples, size);
  }

  m_inBandMutex.Signal();

  return true;
}
#else
PBoolean OpalConnection::SendUserInputTone(char /*tone*/, unsigned /*duration*/)
{
  return false;
}
#endif


void OpalConnection::OnUserInputString(const PString & value)
{
  endpoint.OnUserInputString(*this, value);
}


void OpalConnection::OnUserInputTone(char tone, unsigned duration)
{
  endpoint.OnUserInputTone(*this, tone, duration);
}


PString OpalConnection::GetUserInput(unsigned timeout)
{
  PString reply;
  if (userInputAvailable.Wait(PTimeInterval(0, timeout)) && !IsReleased() && LockReadWrite()) {
    reply = userInputString;
    userInputString = PString();
    UnlockReadWrite();
  }
  return reply;
}


void OpalConnection::SetUserInput(const PString & input)
{
  if (LockReadWrite()) {
    userInputString += input;
    userInputAvailable.Signal();
    UnlockReadWrite();
  }
}


PString OpalConnection::ReadUserInput(const char * terminators,
                                      unsigned lastDigitTimeout,
                                      unsigned firstDigitTimeout)
{
  return endpoint.ReadUserInput(*this, terminators, lastDigitTimeout, firstDigitTimeout);
}


PBoolean OpalConnection::PromptUserInput(PBoolean /*play*/)
{
  return PTrue;
}


#if OPAL_PTLIB_DTMF
void OpalConnection::OnDetectInBandDTMF(RTP_DataFrame & frame, INT)
{
  // This function is set up as an 'audio filter'.
  // This allows us to access the 16 bit PCM audio (at 8Khz sample rate)
  // before the audio is passed on to the sound card (or other output device)

  // Pass the 16 bit PCM audio through the DTMF decoder   
  PString tones = m_dtmfDecoder.Decode((const short *)frame.GetPayloadPtr(),
                                       frame.GetPayloadSize()/sizeof(short),
                                       m_dtmfScaleMultiplier,
                                       m_dtmfScaleDivisor);
  if (!tones.IsEmpty()) {
    PTRACE(3, "OPAL\tDTMF detected: \"" << tones << '"');
    for (PINDEX i = 0; i < tones.GetLength(); i++)
      GetEndPoint().GetManager().QueueDecoupledEvent(new PSafeWorkArg2<OpalConnection, char, unsigned>(
                            this, tones[i], PDTMFDecoder::DetectTime, &OpalConnection::OnUserInputTone));
  }
}

void OpalConnection::OnSendInBandDTMF(RTP_DataFrame & frame, INT)
{
  if (m_inBandDTMF.IsEmpty())
    return;

  // Can't do the usual LockReadWrite() as deadlocks
  m_inBandMutex.Wait();

  PINDEX bytes = m_inBandDTMF.GetSize() - m_emittedInBandDTMF;
  if (bytes > frame.GetPayloadSize())
    bytes = frame.GetPayloadSize();
  memcpy(frame.GetPayloadPtr(), &m_inBandDTMF[m_emittedInBandDTMF], bytes);

  m_emittedInBandDTMF += bytes;

  if (m_emittedInBandDTMF >= m_inBandDTMF.GetSize()) {
    PTRACE(4, "OPAL\tSent in-band DTMF tone, " << m_inBandDTMF.GetSize() << " bytes");
    m_inBandDTMF.SetSize(0);
    m_emittedInBandDTMF = 0;
  }

  m_inBandMutex.Signal();
}
#endif


PString OpalConnection::GetPrefixName() const
{
  return endpoint.GetPrefixName();
}


void OpalConnection::SetLocalPartyName(const PString & name)
{
  localPartyName = name;
}


PString OpalConnection::GetLocalPartyURL() const
{
  return GetPrefixName() + ':' + PURL::TranslateString(GetLocalPartyName(), PURL::LoginTranslation);
}


bool OpalConnection::IsPresentationBlocked() const
{
  return m_stringOptions.GetBoolean(OPAL_OPT_PRESENTATION_BLOCK);
}


static PString MakeURL(const PString & prefix, const PString & partyName)
{
  if (partyName.IsEmpty())
    return PString::Empty();

  PINDEX colon = partyName.Find(':');
  if (colon != P_MAX_INDEX && partyName.FindSpan("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789") == colon)
    return partyName;

  PStringStream url;
  url << prefix << ':' << partyName;
  return url;
}


PString OpalConnection::GetRemotePartyURL() const
{
  if (!remotePartyURL.IsEmpty())
    return remotePartyURL;

  return MakeURL(GetPrefixName(), GetRemotePartyAddress());
}


PString OpalConnection::GetCalledPartyURL()
{
  return MakeURL(GetPrefixName(), GetDestinationAddress());
}


void OpalConnection::CopyPartyNames(const OpalConnection & other)
{
  remotePartyName     = other.remotePartyName;
  remotePartyNumber   = other.remotePartyNumber;
  remotePartyAddress  = other.remotePartyAddress;
  remotePartyURL      = other.remotePartyURL;
  m_calledPartyName   = other.m_calledPartyName;
  m_calledPartyNumber = other.m_calledPartyNumber;
  remoteProductInfo   = other.remoteProductInfo;
}


PString OpalConnection::GetAlertingType() const
{
  return PString::Empty();
}


bool OpalConnection::SetAlertingType(const PString & /*info*/)
{
  return false;
}


PString OpalConnection::GetCallInfo() const
{
  return PString::Empty();
}


bool OpalConnection::GetConferenceState(OpalConferenceState *) const
{
  return false;
}


void OpalConnection::SetAudioJitterDelay(unsigned minDelay, unsigned maxDelay)
{
  if (minDelay != 0 || maxDelay != 0) {
    minDelay = std::max(10U, std::min(minDelay, 999U));
    maxDelay = std::max(minDelay, std::min(maxDelay, 999U));
  }

  m_minAudioJitterDelay = minDelay;
  m_maxAudioJitterDelay = maxDelay;
}


PString OpalConnection::GetIdentifier() const
{
  return GetToken();
}


PINDEX OpalConnection::GetMaxRtpPayloadSize() const
{ 
  return endpoint.GetManager().GetMaxRtpPayloadSize(); 
}


void OpalConnection::SetPhase(Phases phaseToSet)
{
  PTRACE(3, "OpalCon\tSetPhase from " << m_phase << " to " << phaseToSet << " for " << *this);

  PWaitAndSignal mutex(m_phaseMutex);

  // With next few lines we will prevent phase to ever go down when it
  // reaches ReleasingPhase - end result - once you call Release you never
  // go back.
  if (m_phase < ReleasingPhase || (m_phase == ReleasingPhase && phaseToSet == ReleasedPhase)) {
    m_phase = phaseToSet;
    if (!m_phaseTime[m_phase].IsValid())
      m_phaseTime[m_phase].SetCurrentTime();
  }
}


void OpalConnection::SetStringOptions(const StringOptions & options, bool overwrite)
{
  if (overwrite)
    m_stringOptions = options;
  else {
    for (PStringToString::const_iterator it = options.begin(); it != options.end(); ++it)
      m_stringOptions.SetAt(it->first, it->second);
  }

  OnApplyStringOptions();
}

void OpalConnection::OnApplyStringOptions()
{
  endpoint.GetManager().OnApplyStringOptions(*this, m_stringOptions);

  PTRACE_IF(4, !m_stringOptions.IsEmpty(), "OpalCon\tApplying string options:\n" << m_stringOptions);

  if (LockReadWrite()) {
    PCaselessString str;

    str = m_stringOptions(IsOriginating() ? OPAL_OPT_CALLING_PARTY_NAME : OPAL_OPT_CALLED_PARTY_NAME);
    if (!str.IsEmpty())
      SetLocalPartyName(str);

    // Allow for explicitly haveing empty string for display name
    str = IsOriginating() ? OPAL_OPT_CALLING_DISPLAY_NAME : OPAL_OPT_CALLED_DISPLAY_NAME;
    if (m_stringOptions.Contains(str))
      SetDisplayName(m_stringOptions[str]);

    str = m_stringOptions(OPAL_OPT_USER_INPUT_MODE);
    if (str == "RFC2833")
      SetSendUserInputMode(SendUserInputAsRFC2833);
    else if (str == "String")
      SetSendUserInputMode(SendUserInputAsString);
    else if (str == "Tone")
      SetSendUserInputMode(SendUserInputAsTone);
    else if (str == "Q.931")
      SetSendUserInputMode(SendUserInputAsQ931);
#if OPAL_PTLIB_DTMF
    else if (str == "InBand") {
      SetSendUserInputMode(SendUserInputInBand);
      m_sendInBandDTMF = true;
    }
#endif

#if OPAL_PTLIB_DTMF
    m_sendInBandDTMF   = m_stringOptions.GetBoolean(OPAL_OPT_ENABLE_INBAND_DTMF, m_sendInBandDTMF);
    m_detectInBandDTMF = m_stringOptions.GetBoolean(OPAL_OPT_DETECT_INBAND_DTMF, m_detectInBandDTMF);
    m_sendInBandDTMF   = m_stringOptions.GetBoolean(OPAL_OPT_SEND_INBAND_DTMF,   m_sendInBandDTMF);

    m_dtmfScaleMultiplier = m_stringOptions.GetInteger(OPAL_OPT_DTMF_MULT, m_dtmfScaleMultiplier);
    m_dtmfScaleDivisor    = m_stringOptions.GetInteger(OPAL_OPT_DTMF_DIV,  m_dtmfScaleDivisor);
#endif

    m_autoStartInfo.Initialise(m_stringOptions);

    if (m_stringOptions.GetBoolean(OPAL_OPT_DISABLE_JITTER))
      SetAudioJitterDelay(0, 0);
    else
      SetAudioJitterDelay(m_stringOptions.GetInteger(OPAL_OPT_MIN_JITTER, GetMinAudioJitterDelay()),
                          m_stringOptions.GetInteger(OPAL_OPT_MAX_JITTER, GetMaxAudioJitterDelay()));

#if OPAL_HAS_MIXER
    if (m_stringOptions.Contains(OPAL_OPT_RECORD_AUDIO))
      m_recordingFilename = m_stringOptions(OPAL_OPT_RECORD_AUDIO);
#endif

    str = m_stringOptions(OPAL_OPT_ALERTING_TYPE);
    if (!str.IsEmpty())
      SetAlertingType(str);

    UnlockReadWrite();
  }
}


OpalMediaFormatList OpalConnection::GetMediaFormats() const
{
  return endpoint.GetMediaFormats();
}


OpalMediaFormatList OpalConnection::GetLocalMediaFormats()
{
  if (m_localMediaFormats.IsEmpty())
    m_localMediaFormats = ownerCall.GetMediaFormats(*this);
  return m_localMediaFormats;
}


void OpalConnection::OnStartMediaPatch(OpalMediaPatch & patch)
{
  GetEndPoint().GetManager().OnStartMediaPatch(*this, patch);
}


void OpalConnection::OnStopMediaPatch(OpalMediaPatch & patch)
{
  GetEndPoint().GetManager().OnStopMediaPatch(*this, patch);
}


bool OpalConnection::OnMediaFailed(unsigned sessionId, bool source)
{
  return GetEndPoint().GetManager().OnMediaFailed(*this, sessionId, source);
}


bool OpalConnection::OnMediaCommand(OpalMediaStream & stream, const OpalMediaCommand & command)
{
  PTRACE(3, "OpalCon\tOnMediaCommand \"" << command << "\" on " << stream << " for " << *this);
  if (&stream.GetConnection() != this)
    return false;

  PSafePtr<OpalConnection> other = GetOtherPartyConnection();
  return other != NULL && other->OnMediaCommand(stream, command);
}


bool OpalConnection::ExecuteMediaCommand(const OpalMediaCommand & command,
                                         unsigned sessionID,
                                         const OpalMediaType & mediaType) const
{
  PSafeLockReadOnly safeLock(*this);
  if (!safeLock.IsLocked())
    return false;

  if (IsReleased())
    return false;

  OpalMediaStreamPtr stream = sessionID != 0 ? GetMediaStream(sessionID, false)
                                             : GetMediaStream(mediaType, false);
  if (stream == NULL) {
    PTRACE(3, "OpalCon\tNo " << mediaType << " stream to do " << command << " in connection " << *this);
    return false;
  }

  return stream->ExecuteCommand(command);
}


bool OpalConnection::RequireSymmetricMediaStreams() const
{
  return false;
}


OpalMediaType::AutoStartMode OpalConnection::GetAutoStart(const OpalMediaType & mediaType) const
{
  return m_autoStartInfo.GetAutoStart(mediaType);
}


OpalConnection::AutoStartMap::AutoStartMap()
{
  m_initialised = false;
}


void OpalConnection::AutoStartMap::Initialise(const OpalConnection::StringOptions & stringOptions)
{
  PWaitAndSignal m(m_mutex);

  // make function idempotent
  if (m_initialised)
    return;

  m_initialised = true;

  // get autostart option as lines
  PStringArray lines = stringOptions(OPAL_OPT_AUTO_START).Lines();
  for (PINDEX i = 0; i < lines.GetSize(); ++i) {
    PString line = lines[i];
    PINDEX colon = line.Find(':');
    OpalMediaType mediaType = line.Left(colon);

    // see if media type is known, and if it is, enable it
    OpalMediaTypeDefinition * def = mediaType.GetDefinition();
    if (def != NULL) {
      if (colon == P_MAX_INDEX) 
        SetAutoStart(mediaType, OpalMediaType::ReceiveTransmit);
      else {
        PStringArray tokens = line.Mid(colon+1).Tokenise(";", FALSE);
        PINDEX j;
        for (j = 0; j < tokens.GetSize(); ++j) {
          if ((tokens[i] *= "no") || (tokens[i] *= "false") || (tokens[i] *= "0"))
            SetAutoStart(mediaType, OpalMediaType::DontOffer);
          else if ((tokens[i] *= "yes") || (tokens[i] *= "true") || (tokens[i] *= "1") || (tokens[i] *= "sendrecv"))
            SetAutoStart(mediaType, OpalMediaType::ReceiveTransmit);
          else if (tokens[i] *= "recvonly")
            SetAutoStart(mediaType, OpalMediaType::Receive);
          else if (tokens[i] *= "sendonly")
            SetAutoStart(mediaType, OpalMediaType::Transmit);
          else if ((tokens[i] *= "offer") || (tokens[i] *= "inactive"))
            SetAutoStart(mediaType, OpalMediaType::OfferInactive);
          else if (tokens[i] *= "exclusive") {
            OpalMediaTypeList types = OpalMediaType::GetList();
            for (OpalMediaTypeList::iterator r = types.begin(); r != types.end(); ++r)
              SetAutoStart(*r, *r == mediaType ? OpalMediaType::ReceiveTransmit : OpalMediaType::DontOffer);
          }
        }
      }
    }
  }
}


void OpalConnection::AutoStartMap::SetAutoStart(const OpalMediaType & mediaType, OpalMediaType::AutoStartMode autoStart)
{
  PWaitAndSignal m(m_mutex);
  m_initialised = true;

  // deconflict session ID
  unsigned sessionID = mediaType->GetDefaultSessionId();
  if (size() == 0) {
    if (sessionID == 0) 
      sessionID = 1;
  }
  else {
    iterator r = begin();
    while (r != end()) {
      if (r->second.preferredSessionId != sessionID) 
        ++r;
      else {
        ++sessionID;
        r = begin();
      }
    }
  }

  AutoStartInfo info;
  info.autoStart = autoStart;
  info.preferredSessionId = sessionID;

  insert(value_type(mediaType, info));
}


OpalMediaType::AutoStartMode OpalConnection::AutoStartMap::GetAutoStart(const OpalMediaType & mediaType) const
{
  PWaitAndSignal m(m_mutex);
  const_iterator r = find(mediaType);
  return r == end() ? mediaType.GetAutoStart() : r->second.autoStart;
}


void OpalConnection::StringOptions::ExtractFromURL(PURL & url)
{
  PStringToString params = url.GetParamVars();
  params.MakeUnique();
  for (PStringToString::iterator it = params.begin(); it != params.end(); ++it) {
    PCaselessString key = it->first;
    if (key.NumCompare(OPAL_URL_PARAM_PREFIX) == EqualTo) {
      SetAt(key.Mid(5), it->second);
      url.SetParamVar(key, PString::Empty());
    }
  }
}


#if OPAL_SCRIPT

void OpalConnection::ScriptRelease(PScriptLanguage &, PScriptLanguage::Signature & sig)
{
  OpalConnection::CallEndReason reason;

  switch (sig.m_arguments.size()) {
    default :
    case 2 :
      reason.q931 = sig.m_arguments[1].AsInteger();
      // Next case

    case 1 :
      reason.code = (OpalConnection::CallEndReasonCodes)sig.m_arguments[0].AsInteger();
      // Next case

    case 0 :
      break;
  }

  Release(reason);
}


void OpalConnection::ScriptSetOption(PScriptLanguage &, PScriptLanguage::Signature & sig)
{
  for (size_t i = 0; i < sig.m_arguments.size(); ++i) {
    PString key = sig.m_arguments[i++].AsString();
    if (i < sig.m_arguments.size())
      m_stringOptions.SetAt(key, sig.m_arguments[i].AsString());
    else
      m_stringOptions.SetAt(key, PString::Empty());
  }
}


void OpalConnection::ScriptGetLocalPartyURL(PScriptLanguage &, PScriptLanguage::Signature & sig)
{
  sig.m_results.resize(1);
  sig.m_results[0].SetDynamicString(GetLocalPartyURL());
}


void OpalConnection::ScriptGetRemotePartyURL(PScriptLanguage &, PScriptLanguage::Signature & sig)
{
  sig.m_results.resize(1);
  sig.m_results[0].SetDynamicString(GetRemotePartyURL());
}


void OpalConnection::ScriptGetCalledPartyURL(PScriptLanguage &, PScriptLanguage::Signature & sig)
{
  sig.m_results.resize(1);
  sig.m_results[0].SetDynamicString(GetCalledPartyURL());
}


void OpalConnection::ScriptGetRedirectingParty(PScriptLanguage &, PScriptLanguage::Signature & sig)
{
  sig.m_results.resize(1);
  sig.m_results[0].SetDynamicString(GetRedirectingParty());
}

#endif // OPAL_SCRIPT


/////////////////////////////////////////////////////////////////////////////
