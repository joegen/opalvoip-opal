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
 */

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "connection.h"
#endif

#include <opal_config.h>

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
#include <ptclib/url.h>


#define new PNEW

#define PTraceModule() "OpalCon"

#if PTRACING

ostream & operator<<(ostream & out, OpalConnection::CallEndReason reason)
{
  if (reason.code != OpalConnection::EndedByQ931Cause)
    return out << (OpalConnection::CallEndReasonCodes)reason.code;
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
  { OpalConnection::EndedByCertificateAuthority, "Server certificates could not be authenticated." },
  { OpalConnection::EndedByIllegalAddress,       "An illegal address was used for transport." },
};

static POrdinalToString CallEndReasonStrings(PARRAYSIZE(CallEndReasonStringsInitialiser), CallEndReasonStringsInitialiser);


/////////////////////////////////////////////////////////////////////////////

OpalConnection::OpalConnection(OpalCall & call,
                               OpalEndPoint  & ep,
                               const PString & token,
                               unsigned int options,
                               OpalConnection::StringOptions * stringOptions)
  : PSafeObject(&call)  // Share the lock flag from the call
  , m_ownerCall(call)
  , m_endpoint(ep)
  , m_phase(UninitialisedPhase)
  , m_callToken(token)
  , m_originating(false)
  , m_productInfo(ep.GetProductInfo())
  , m_localPartyName(ep.GetDefaultLocalPartyName())
  , m_displayName(ep.GetDefaultDisplayName())
  , m_remotePartyName(token)
  , m_callEndReason(NumCallEndReasons)
  , m_silenceDetector(NULL)
#if OPAL_AEC
  , m_echoCanceler(NULL)
#endif
#if OPAL_PTLIB_DTMF
  , m_jitterParams(m_endpoint.GetManager().GetJitterParameters())
  , m_rxBandwidthAvailable(m_endpoint.GetInitialBandwidth(OpalBandwidth::Rx))
  , m_txBandwidthAvailable(m_endpoint.GetInitialBandwidth(OpalBandwidth::Tx))
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
{
  PTRACE_CONTEXT_ID_FROM(call);

  PTRACE(3, "Created connection " << *this << " ptr=" << this);

  PAssert(m_ownerCall.SafeReference(), PLogicError);

  m_ownerCall.m_connectionsActive.Append(this);

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
      m_detectInBandDTMF = !m_endpoint.GetManager().DetectInBandDTMFDisabled();
      break;
  }
#endif

  switch (options & SendDTMFMask) {
    case SendDTMFAsString:
      m_sendUserInputMode = SendUserInputAsString;
      break;
    case SendDTMFAsTone:
      m_sendUserInputMode = SendUserInputAsTone;
      break;
    case SendDTMFAsRFC2833:
      m_sendUserInputMode = SendUserInputAsRFC2833;
      break;
    case SendDTMFAsDefault:
    default:
      m_sendUserInputMode = ep.GetSendUserInputMode();
      break;
  }

#if 0 // OPAL_HAS_IM
  m_rfc4103Context[0].SetMediaFormat(OpalT140);
  m_rfc4103Context[1].SetMediaFormat(OpalT140);
#endif

#if OPAL_SCRIPT
  PScriptLanguage * script = m_endpoint.GetManager().GetScript();
  if (script != NULL) {
    m_scriptTableName  = OPAL_SCRIPT_CALL_TABLE_NAME "." + m_ownerCall.GetToken() + '[' + GetToken().ToLiteral() + ']';
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


OpalConnection::~OpalConnection()
{
  m_mediaStreams.RemoveAll();

#if OPAL_SCRIPT
  PScriptLanguage * script = m_endpoint.GetManager().GetScript();
  if (script != NULL) {
    script->Call("OnDestroyConnection", "ss", (const char *)m_ownerCall.GetToken(), (const char *)GetToken());
    script->ReleaseVariable(m_scriptTableName);
  }
#endif

  delete m_silenceDetector;
#if OPAL_AEC
  delete m_echoCanceler;
#endif
#if OPAL_T120DATA
  delete t120handler;
#endif

  m_ownerCall.m_connectionsActive.Remove(this);
  m_ownerCall.SafeDereference();

  PTRACE(3, "Destroyed connection " << *this << " ptr=" << this);
}


void OpalConnection::PrintOn(ostream & strm) const
{
  strm << m_ownerCall << '-'<< m_endpoint << '[' << m_callToken << ']';
}


bool OpalConnection::GarbageCollection()
{
  return m_mediaStreams.DeleteObjectsToBeRemoved();
}


void OpalConnection::SetToken(const PString & newToken)
{
  if (m_callToken == newToken)
    return;

  PTRACE(3, "Set new token from \"" << m_callToken << "\" to \"" << newToken << '"');

  m_endpoint.m_connectionsActive.DisallowDeleteObjects();
  m_endpoint.m_connectionsActive.RemoveAt(m_callToken);
  m_endpoint.m_connectionsActive.AllowDeleteObjects();
  m_callToken = newToken;
  m_endpoint.m_connectionsActive.SetAt(newToken, this);
}


void OpalConnection::InternalSetAsOriginating()
{
  PTRACE(4, "Set originating " << *this);
  m_originating = true;
#if OPAL_SCRIPT
  PScriptLanguage * script = m_endpoint.GetManager().GetScript();
  if (script != NULL)
    script->SetBoolean(m_scriptTableName  + ".originating", true);
#endif
}


PBoolean OpalConnection::SetUpConnection()
{
  // Check if we are A-Party in this call, so need to do things differently
  if (m_ownerCall.GetConnection(0) == this) {
    SetPhase(SetUpPhase);
    if (!OnIncomingConnection(0, NULL)) {
      Release(EndedByNoUser);
      return false;
    }

    PTRACE(3, "Outgoing call routed to " << m_ownerCall.GetPartyB() << " to " << *this);
    if (!m_ownerCall.OnSetUp(*this)) {
      Release(EndedByNoAccept);
      return false;
    }
  }
  else if (m_ownerCall.IsEstablished()) {
    PTRACE(3, "Transfer of connection in call " << m_ownerCall);
    OnApplyStringOptions();
    AutoStartMediaStreams(true);
    InternalOnConnected();
  }
  else {
    InternalSetAsOriginating();
    PTRACE(3, "Incoming call from " << m_remotePartyName << " to " << *this);

    OnApplyStringOptions();
  }

  return true;
}


PBoolean OpalConnection::OnSetUpConnection()
{
  PTRACE(3, "OnSetUpConnection" << *this);
  return m_endpoint.OnSetUpConnection(*this);
}


bool OpalConnection::HoldRemote(bool /*placeOnHold*/)
{
  return false;
}


PBoolean OpalConnection::IsOnHold(bool /*fromRemote*/) const
{
  return false;
}


void OpalConnection::OnHold(bool fromRemote, bool onHold)
{
  PTRACE(4, "OnHold: " << (onHold ? "on" : "off") << " hold, "
         << (fromRemote ? "from" : "to") << " remote, " << *this);
  m_endpoint.OnHold(*this, fromRemote, onHold);
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
  if (m_callEndReason == NumCallEndReasons) {
    if (m_ownerCall.GetCallEndReason() != OpalConnection::NumCallEndReasons) {
      m_callEndReason = m_ownerCall.GetCallEndReason();
      PTRACE_IF(3, m_callEndReason != reason, "Call end reason for "
                << *this << " not set to " << reason << ", using call value " << m_callEndReason);
    }
    else {
      PTRACE(3, "Call end reason for " << *this << " set to " << reason);
      m_callEndReason = reason;
      m_ownerCall.SetCallEndReason(reason);
    }
  }
}


void OpalConnection::ClearCall(CallEndReason reason, PSyncPoint * sync)
{
  SetCallEndReason(reason);
  m_ownerCall.Clear(reason, sync);
}


void OpalConnection::ClearCallSynchronous(PSyncPoint * sync, CallEndReason reason)
{
  SetCallEndReason(reason);

  PSyncPoint syncPoint;
  if (sync == NULL)
    sync = &syncPoint;

  ClearCall(reason, sync);

  PTRACE(5, "Synchronous wait for " << *this);
  sync->Wait();
}


bool OpalConnection::TransferConnection(const PString & PTRACE_PARAM(remoteParty))
{
  PTRACE(2, "Can not transfer connection to " << remoteParty);
  return false;
}


void OpalConnection::Release(CallEndReason reason, bool synchronous)
{
  if (InternalRelease(reason))
    return;

  /* If we have exactly 2 connections, then we release the other connection
     as well, which clears the entire call. */
  if (m_ownerCall.GetConnectionCount() == 2) {
    PSafePtr<OpalConnection> other = GetOtherPartyConnection();
    if (other != NULL)
      other->InternalRelease(reason); // Do not execute OnReleased() here, see OpalCall::OnReleased()
  }

  // Add a reference for the thread we are about to start
  SafeReference();

  if (synchronous) {
    PTRACE(3, "Releasing synchronously " << *this);
    InternalOnReleased();
  }
  else {
    PTRACE(3, "Releasing asynchronously " << *this);
    new PThreadObj<OpalConnection>(*this, &OpalConnection::InternalOnReleased, true, "OnRelease");
  }
}


bool OpalConnection::InternalRelease(CallEndReason reason)
{
  PWaitAndSignal mutex(m_phaseMutex);

  if (IsReleased()) {
    PTRACE(3, "Already released " << *this);
    return true;
  }

  SetPhase(ReleasingPhase);
  SetCallEndReason(reason);
  return false;
}


void OpalConnection::InternalOnReleased()
{
  PTRACE_CONTEXT_ID_TO(PThread::Current());

  /* Do a brief lock here to avoid race conditions where an operation (e.g. a
     SIP re-INVITE) was started before release, but has not yet finished. Once
     that operation has finished, no new operations should happen as they will
     check GetPhase() for connection released before proceeding.

     Note, call to UnlockReadOnly() is BEFORE the call to OnReleased(), it is
     up to OnReleased() to manage it's locking regime. */
  if (LockReadOnly()) {
    UnlockReadOnly();
    OnReleased();
  }

  PTRACE(4, "OnRelease thread completed for " << *this);

  // Dereference on the way out of the thread
  SafeDereference();
}


void OpalConnection::OnReleased()
{
  PTRACE(4, "OnReleased " << *this);

  CloseMediaStreams();

  m_endpoint.OnReleased(*this);

  SetPhase(ReleasedPhase);

#if PTRACING
  static const int Level = 3;
  if (PTrace::CanTrace(Level)) {
    ostream & trace = PTRACE_BEGIN(Level);
    trace << "Connection " << *this << " released\n"
             "        Initial Time: " << m_phaseTime[UninitialisedPhase] << '\n';
    for (Phases ph = SetUpPhase; ph < NumPhases; ph = (Phases)(ph+1)) {
      trace << setw(20) << ph << ": ";
      if (m_phaseTime[ph].IsValid())
        trace << (m_phaseTime[ph]-m_phaseTime[UninitialisedPhase]);
      else
        trace << "N/A";
      trace << '\n';
    }
    trace << "     Call end reason: " << m_callEndReason << '\n'
          << PTrace::End;
  }
#endif
}


PBoolean OpalConnection::OnIncomingConnection(unsigned options, OpalConnection::StringOptions * stringOptions)
{
  if (m_endpoint.OnIncomingConnection(*this, options, stringOptions))
    return true;

  Release(EndedByNoUser);
  return false;
}


PString OpalConnection::GetDestinationAddress()
{
  PSafePtr<OpalConnection> partyA = m_ownerCall.GetConnection(0);
  if (partyA != this)
    return partyA->GetDestinationAddress();

  if (!IsOriginating()) {
    if (!m_calledPartyNumber.IsEmpty())
      return m_calledPartyNumber;
    if (!m_calledPartyName.IsEmpty())
      return m_calledPartyName;
  }
  return '*';
}


PBoolean OpalConnection::ForwardCall(const PString & /*forwardParty*/)
{
  return false;
}


PSafePtr<OpalConnection> OpalConnection::GetOtherPartyConnection() const
{
  return GetCall().GetOtherPartyConnection(*this);
}


void OpalConnection::OnProceeding()
{
  m_endpoint.OnProceeding(*this);
}


void OpalConnection::OnAlerting(bool withMedia)
{
  m_endpoint.OnAlerting(*this, withMedia);
}


void OpalConnection::OnAlerting()
{
  m_endpoint.OnAlerting(*this);
}


PBoolean OpalConnection::SetAlerting(const PString &, PBoolean)
{
  return true;
}


OpalConnection::AnswerCallResponse OpalConnection::OnAnswerCall(const PString & callerName)
{
  return m_endpoint.OnAnswerCall(*this, callerName);
}


void OpalConnection::AnsweringCall(AnswerCallResponse response)
{
  PTRACE(3, "Answering call: " << response);

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
      SetAlerting(GetLocalPartyName(), true);
      break;

    case AnswerCallPending :
      SetAlerting(GetLocalPartyName(), false);
      break;

    case AnswerCallNow:
      PTRACE(3, "Application has answered incoming call");
      GetOtherPartyConnection()->InternalOnConnected();
      break;

    default : // AnswerCallDeferred etc
      break;
  }
}


PBoolean OpalConnection::SetConnected()
{
  PTRACE(3, "SetConnected for " << *this);

  if (GetPhase() < ConnectedPhase)
    SetPhase(ConnectedPhase);

  InternalOnEstablished();
  return true;
}


bool OpalConnection::InternalOnConnected()
{
  if (GetPhase() >= ConnectedPhase)
    return false;

  SetPhase(ConnectedPhase);
  OnConnected();
  InternalOnEstablished();
  return true;
}


bool OpalConnection::InternalOnEstablished()
{
  if (GetPhase() != ConnectedPhase) {
    PTRACE(5, "Not in ConnectedPhase, cannot move to EstablishedPhase on " << *this);
    return false;
  }

  if (m_mediaStreams.IsEmpty()) {
    PTRACE(5, "No media streams, cannot move to EstablishedPhase on " << *this);
    return false;
  }

  for (StreamDict::iterator it = m_mediaStreams.begin(); it != m_mediaStreams.end(); ++it) {
    OpalMediaStreamPtr mediaStream = it->second;
    if (mediaStream.SetSafetyMode(PSafeReadOnly) && !mediaStream->IsEstablished()) {
      PTRACE(5, "Media stream " << *it->second << " is not established, cannot move to EstablishedPhase on " << *this);
      return false;
    }
  }

  SetPhase(EstablishedPhase);
  OnEstablished();
  return true;
}


void OpalConnection::OnConnected()
{
  PTRACE(3, "OnConnected for " << *this);
  m_endpoint.OnConnected(*this);
}


void OpalConnection::OnEstablished()
{
  PTRACE(3, "OnEstablished " << *this);
  StartMediaStreams();
  m_endpoint.OnEstablished(*this);
}


bool OpalConnection::OnTransferNotify(const PStringToString & info,
                                      const OpalConnection * transferringConnection)
{
  return m_endpoint.OnTransferNotify(*this, info, transferringConnection);
}


void OpalConnection::AdjustMediaFormats(bool   local,
                        const OpalConnection * otherConnection,
                         OpalMediaFormatList & mediaFormats) const
{
  if (otherConnection != NULL)
    return;

  mediaFormats.Remove(m_stringOptions(OPAL_OPT_REMOVE_CODEC).Lines());

  if (!local)
    return m_endpoint.AdjustMediaFormats(local, *this, mediaFormats);

  for (PStringToString::const_iterator it = m_stringOptions.begin(); it != m_stringOptions.end(); ++it) {
    PString key = it->first;
    PString fmtName, optName;
    if (key.Split(':', fmtName, optName, PString::SplitTrimBefore|PString::SplitNonEmpty)) {
      PString optValue = it->second;
      OpalMediaFormatList::const_iterator iterFormat;
      while ((iterFormat = mediaFormats.FindFormat(fmtName, iterFormat)) != mediaFormats.end()) {
        OpalMediaFormat & format = const_cast<OpalMediaFormat &>(*iterFormat);
        if (format.SetOptionValue(optName, optValue)) {
          PTRACE(4, "Set media format " << format
                  << " option " << optName << " to \"" << optValue << '"');
        }
        else {
          PTRACE(2, "Failed to set media format " << format
                  << " option " << optName << " to \"" << optValue << '"');
        }
      }
    }
  }

  m_endpoint.AdjustMediaFormats(local, *this, mediaFormats);

  // See if we can assign more reasonable payload type numbers to use
  std::vector<OpalMediaFormat *> usedFormats(RTP_DataFrame::MaxPayloadType+2);
  for (OpalMediaFormatList::iterator it = mediaFormats.begin(); it != mediaFormats.end(); ++it)
    usedFormats[it->GetPayloadType()] = &*it;

  RTP_DataFrame::PayloadTypes unusedPT = RTP_DataFrame::DynamicBase;
  for (RTP_DataFrame::PayloadTypes checkPT = (RTP_DataFrame::PayloadTypes)(RTP_DataFrame::LastKnownPayloadType + 1);
                          checkPT < RTP_DataFrame::DynamicBase; checkPT = (RTP_DataFrame::PayloadTypes)(checkPT + 1)) {
    if (checkPT == RTP_DataFrame::StartConflictRTCP)
      checkPT = RTP_DataFrame::EndConflictRTCP;
    else if (usedFormats[checkPT] != NULL) {
      while (usedFormats[unusedPT] != NULL) {
        if (unusedPT < RTP_DataFrame::DynamicBase) {
          unusedPT = (RTP_DataFrame::PayloadTypes)(unusedPT - 1);
          if (unusedPT <= checkPT)
            return; // We are full
        }
        else {
          unusedPT = (RTP_DataFrame::PayloadTypes)(unusedPT + 1);
          if (unusedPT >= RTP_DataFrame::MaxPayloadType)
            unusedPT = (RTP_DataFrame::PayloadTypes)(RTP_DataFrame::DynamicBase - 1);
        }
      }
      usedFormats[checkPT]->SetPayloadType(unusedPT);
      std::swap(usedFormats[unusedPT], usedFormats[checkPT]);
    }
  }
}


PStringArray OpalConnection::GetMediaCryptoSuites() const
{
  PStringArray overrides = m_stringOptions(OPAL_OPT_CRYPTO_SUITES).Lines();
  return overrides.IsEmpty() ? m_endpoint.GetMediaCryptoSuites() : overrides;
}


unsigned OpalConnection::GetNextSessionID(const OpalMediaType & /*mediaType*/, bool /*isSource*/)
{
  return 0;
}


void OpalConnection::AutoStartMediaStreams(bool transfer)
{
  PTRACE(4, "AutoStartMediaStreams("
         << (transfer ? "transfer": (GetPhase() < AlertingPhase ? "alerting" : "normal"))
         << ") on " << *this);

  PSafePtr<OpalConnection> otherConnection = GetOtherPartyConnection();
  if (otherConnection == NULL)
    return;

  OpalMediaTypeList mediaTypes = OpalMediaType::GetList();
  for (OpalMediaTypeList::iterator iter = mediaTypes.begin(); iter != mediaTypes.end(); ++iter) {
    OpalMediaType mediaType = *iter;
    if ((otherConnection->GetAutoStart(mediaType)&OpalMediaType::Receive) &&
                         (GetAutoStart(mediaType)&OpalMediaType::Transmit) &&
                        (transfer || GetMediaStream(mediaType, true) == NULL))
      m_ownerCall.OpenSourceMediaStreams(*this,
                                       mediaType,
                                       mediaType->GetDefaultSessionId(),
                                       OpalMediaFormat(),
#if OPAL_VIDEO
                                       OpalVideoFormat::eNoRole,
#endif
                                       transfer);
  }

  if (!transfer && GetPhase() >= ConnectedPhase)
    StartMediaStreams();
}


#if OPAL_T38_CAPABILITY
bool OpalConnection::SwitchFaxMediaStreams(bool)
{
  PAssertAlways(PUnimplementedFunction);
  return false;
}


void OpalConnection::OnSwitchedFaxMediaStreams(bool toT38, bool success)
{
  if (!PAssert(m_ownerCall.IsSwitchingT38(), PLogicError))
    return;

  m_ownerCall.ResetSwitchingT38();

  PTRACE(3, "Switch of media streams to "
         << (toT38 ? "T.38" : "audio") << ' '
         << (success ? "succeeded" : "failed")
         << " on " << *this);

  PSafePtr<OpalConnection> other = GetOtherPartyConnection();
  if (other != NULL)
    other->OnSwitchedFaxMediaStreams(toT38, success);

  if (success || IsReleased())
    return;

  if (toT38) {
    PTRACE(4, "Switch request to fax failed, falling back to audio mode");
    SwitchFaxMediaStreams(false);
  }
  else {
    PTRACE(3, "Switch request back to audio mode failed.");
    Release();
  }
}

bool OpalConnection::OnSwitchingFaxMediaStreams(bool toT38)
{
  if (!m_ownerCall.IsSwitchingT38()) {
    PTRACE(3, "Remote requests switching media streams to "
           << (toT38 ? "T.38" : "audio") << " on " << *this);

    m_ownerCall.SetSwitchingT38(toT38);

    PSafePtr<OpalConnection> other = GetOtherPartyConnection();
    if (other != NULL)
      return other->OnSwitchingFaxMediaStreams(toT38);
  }

  return true;
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
      PTRACE(3, "OpenMediaStream (already opened) for session " << sessionID << " on " << *this);
      return stream;
    }
    // Changing the media format, needs to close and re-open the stream
    stream->Close();
    stream.SetNULL();
  }

  if (stream == NULL) {
    stream = CreateMediaStream(mediaFormat, sessionID, isSource);
    if (stream == NULL) {
      PTRACE(1, "CreateMediaStream returned NULL for session " << sessionID << " on " << *this);
      return NULL;
    }
    m_mediaStreams.SetAt(*stream, stream);

    m_mediaSessionFailedMutex.Wait();
    m_mediaSessionFailed.erase(sessionID*2 + isSource);
    m_mediaSessionFailedMutex.Signal();
  }

  if (stream->Open()) {
    if (OnOpenMediaStream(*stream)) {
      PTRACE(3, "Opened " << (isSource ? "source" : "sink") << " stream " << stream->GetID() << " with format " << mediaFormat);
      return stream;
    }
    PTRACE(2, "OnOpenMediaStream failed for " << mediaFormat << ", closing " << *stream);
    stream->Close();
  }
  else {
    PTRACE(2, "Source media stream open failed for " << *stream << " (" << mediaFormat << ')');
  }

  m_mediaStreams.RemoveAt(*stream);

  return NULL;
}


bool OpalConnection::CloseMediaStream(unsigned sessionId, bool source)
{
  PSafeLockReadWrite safeLock(*this);
  OpalMediaStreamPtr stream = GetMediaStream(sessionId, source);
  return stream != NULL && stream->Close();
}


bool OpalConnection::CloseMediaStream(OpalMediaStreamPtr stream)
{
  PSafeLockReadWrite safeLock(*this);
  return stream != NULL && stream->Close();
}


PBoolean OpalConnection::RemoveMediaStream(OpalMediaStream & stream)
{
  stream.Close();
  PTRACE(3, "Removed media stream " << stream);
  return m_mediaStreams.RemoveAt(stream);
}


void OpalConnection::StartMediaStreams()
{
  for (StreamDict::iterator it = m_mediaStreams.begin(); it != m_mediaStreams.end(); ++it) {
    OpalMediaStreamPtr mediaStream = it->second;
    if (mediaStream.SetSafetyMode(PSafeReadWrite))
      mediaStream->Start();
  }

  PTRACE(3, "Media stream threads started for " << *this);
}


void OpalConnection::CloseMediaStreams()
{
  // Do this double loop as while closing streams, the instance may disappear from the
  // mediaStreams list, prematurely stopping the for loop.
  bool someOpen = true;
  while (someOpen) {
    someOpen = false;
    for (StreamDict::iterator it = m_mediaStreams.begin(); it != m_mediaStreams.end(); ++it) {
      OpalMediaStreamPtr mediaStream = it->second;
      if (mediaStream != NULL && mediaStream->IsOpen()) {
        someOpen = true;
        mediaStream->Close();
      }
    }
  }

  PTRACE(3, "Media streams closed.");
}


void OpalConnection::PauseMediaStreams(bool paused)
{
  for (StreamDict::iterator it = m_mediaStreams.begin(); it != m_mediaStreams.end(); ++it) {
    OpalMediaStreamPtr mediaStream = it->second;
    if (mediaStream.SetSafetyMode(PSafeReadWrite))
      mediaStream->SetPaused(paused);
  }
}


void OpalConnection::OnPauseMediaStream(OpalMediaStream & /*strm*/, bool /*paused*/)
{
}


OpalMediaStream * OpalConnection::CreateMediaStream(const OpalMediaFormat &, unsigned, PBoolean)
{
  return NULL;
}


PBoolean OpalConnection::OnOpenMediaStream(OpalMediaStream & stream)
{
  if (!m_endpoint.OnOpenMediaStream(*this, stream))
    return false;

  if (!LockReadWrite())
    return false;

  InternalOnEstablished();

  UnlockReadWrite();

  return true;
}


void OpalConnection::OnClosedMediaStream(const OpalMediaStream & stream)
{
  OpalMediaPatchPtr patch = stream.GetPatch();
  if (patch != NULL) {
#if OPAL_HAS_MIXER
    OnStopRecording(patch);
#endif

    if (m_silenceDetector != NULL && patch->RemoveFilter(m_silenceDetector->GetReceiveHandler(), m_filterMediaFormat)) {
      PTRACE(4, "Removed silence detect filter on connection " << *this << ", patch " << patch);
    }

#if OPAL_AEC
    if (m_echoCanceler != NULL && patch->RemoveFilter(stream.IsSource() ? m_echoCanceler->GetReceiveHandler()
                                                  : m_echoCanceler->GetSendHandler(), m_filterMediaFormat)) {
      PTRACE(4, "Removed echo canceler filter on connection " << *this << ", patch " << patch);
    }
#endif

#if OPAL_PTLIB_DTMF
    if (patch->RemoveFilter(m_dtmfDetectNotifier, OpalPCM16)) {
      PTRACE(4, "Removed detect DTMF filter on connection " << *this << ", patch " << patch);
    }
    if (!m_dtmfSendFormat.IsEmpty() && patch->RemoveFilter(m_dtmfSendNotifier, m_dtmfSendFormat)) {
      PTRACE(4, "Removed DTMF send filter on connection " << *this << ", patch " << patch);
    }
#endif
  }

  m_endpoint.OnClosedMediaStream(stream);
}


void OpalConnection::OnFailedMediaStream(bool fromRemote, const PString & reason)
{
  m_endpoint.OnFailedMediaStream(*this, fromRemote, reason);
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

  if (!m_ownerCall.OnStartRecording(MakeRecordingKey(*patch), patch->GetSource().GetMediaFormat())) {
    PTRACE(4, "No record filter added on connection " << *this << ", patch " << *patch);
    return;
  }

  patch->AddFilter(m_recordAudioNotifier, OpalPCM16);
#if OPAL_VIDEO
  patch->AddFilter(m_recordVideoNotifier, OPAL_YUV420P);
#endif

  PTRACE(4, "Added record filter on connection " << *this << ", patch " << *patch);
}


void OpalConnection::OnStopRecording(OpalMediaPatch * patch)
{
  if (patch == NULL)
    return;

  m_ownerCall.OnStopRecording(MakeRecordingKey(*patch));

  patch->RemoveFilter(m_recordAudioNotifier, OpalPCM16);
#if OPAL_VIDEO
  patch->RemoveFilter(m_recordVideoNotifier, OPAL_YUV420P);
#endif

  PTRACE(4, "Removed record filter on " << *patch);
}

#endif

void OpalConnection::OnPatchMediaStream(PBoolean isSource, OpalMediaPatch & patch)
{
  OpalMediaFormat mediaFormat = isSource ? patch.GetSource().GetMediaFormat() : patch.GetSink()->GetMediaFormat();

  if (mediaFormat.GetMediaType() == OpalMediaType::Audio()) {
    if (!mediaFormat.IsTransportable()) {
      m_filterMediaFormat = mediaFormat;

      if (isSource && m_silenceDetector != NULL) {
        m_silenceDetector->SetParameters(m_endpoint.GetManager().GetSilenceDetectParams(), mediaFormat.GetClockRate());
        patch.AddFilter(m_silenceDetector->GetReceiveHandler(), mediaFormat);
        PTRACE(4, "Added silence detect filter on connection " << *this << ", patch " << patch);
      }

#if OPAL_AEC
      if (m_echoCanceler) {
        m_echoCanceler->SetParameters(m_endpoint.GetManager().GetEchoCancelParams());
        m_echoCanceler->SetClockRate(mediaFormat.GetClockRate());
        patch.AddFilter(isSource ? m_echoCanceler->GetReceiveHandler()
                                 : m_echoCanceler->GetSendHandler(), mediaFormat);
        PTRACE(4, "Added echo canceler filter on connection " << *this << ", patch " << patch);
      }
#endif
    }

#if OPAL_PTLIB_DTMF
    if (m_detectInBandDTMF && isSource) {
      patch.AddFilter(m_dtmfDetectNotifier, OpalPCM16);
      PTRACE(4, "Added detect DTMF filter on connection " << *this << ", patch " << patch);
    }

    if (m_sendInBandDTMF && !isSource) {
      if (mediaFormat == OpalG711_ULAW_64K || mediaFormat == OpalG711_ALAW_64K)
        m_dtmfSendFormat = mediaFormat;
      else
        m_dtmfSendFormat = OpalPCM16;
      patch.AddFilter(m_dtmfSendNotifier, mediaFormat);
      PTRACE(4, "Added send DTMF filter on connection " << *this << ", patch " << patch);
    }
#endif
  }

#if OPAL_HAS_MIXER
  if (!m_recordingFilename.IsEmpty())
    m_ownerCall.StartRecording(m_recordingFilename);
  else if (m_ownerCall.IsRecording())
    OnStartRecording(&patch);
#endif

  PTRACE(3, (isSource ? "Source" : "Sink") << " stream of connection " << *this << " uses patch " << patch);
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
#endif // OPAL_VIDEO
}


void OpalConnection::OnRecordAudio(RTP_DataFrame & frame, P_INT_PTR param)
{
  if (frame.GetPayloadSize() == 0)
    return;

  const OpalMediaPatch * patch = (const OpalMediaPatch *)param;
  std::auto_ptr<RTP_DataFrame> copyFrame(new RTP_DataFrame(frame.GetPointer(), frame.GetPacketSize()));
  GetEndPoint().GetManager().QueueDecoupledEvent(new PSafeWorkArg2<OpalConnection, PString, std::auto_ptr<RTP_DataFrame> >(
                   this, MakeRecordingKey(*patch), copyFrame, &OpalConnection::InternalOnRecordAudio), psprintf("%p", this));
}


void OpalConnection::InternalOnRecordAudio(PString key, std::auto_ptr<RTP_DataFrame> frame)
{
  m_ownerCall.OnRecordAudio(key, *frame);
}


#if OPAL_VIDEO

void OpalConnection::OnRecordVideo(RTP_DataFrame & frame, P_INT_PTR param)
{
  const OpalMediaPatch * patch = (const OpalMediaPatch *)param;
  std::auto_ptr<RTP_DataFrame> copyFrame(new RTP_DataFrame(frame.GetPointer(), frame.GetPacketSize()));
  GetEndPoint().GetManager().QueueDecoupledEvent(new PSafeWorkArg2<OpalConnection, PString, std::auto_ptr<RTP_DataFrame> >(
                   this, MakeRecordingKey(*patch), copyFrame, &OpalConnection::InternalOnRecordVideo), psprintf("%p", this));
}


void OpalConnection::InternalOnRecordVideo(PString key, std::auto_ptr<RTP_DataFrame> frame)
{
  m_ownerCall.OnRecordVideo(key, *frame);
}

#endif // OPAL_VIDEO

#endif // OPAL_HAS_MIXER


OpalMediaStreamPtr OpalConnection::GetMediaStream(const PString & streamID, bool source) const
{
  for (StreamDict::const_iterator it = m_mediaStreams.begin(); it != m_mediaStreams.end(); ++it) {
    OpalMediaStreamPtr mediaStream = it->second;
    if (mediaStream != NULL &&
        mediaStream->IsSource() == source &&
        (streamID.IsEmpty() || mediaStream->GetID() == streamID))
      return mediaStream;
  }

  return NULL;
}


OpalMediaStreamPtr OpalConnection::GetMediaStream(unsigned sessionId, bool source) const
{
  StreamDict::const_iterator it = m_mediaStreams.find(StreamKey(sessionId, source));
  return it != m_mediaStreams.end() ? it->second : OpalMediaStreamPtr();
}


OpalMediaStreamPtr OpalConnection::GetMediaStream(const OpalMediaType & mediaType,
                                                  bool source,
                                                  OpalMediaStreamPtr mediaStream) const
{
  if (mediaStream == NULL)
    mediaStream = OpalMediaStreamPtr(m_mediaStreams, PSafeReference);
  else
    ++mediaStream;

  while (mediaStream != NULL) {
    if ((mediaType.empty() || mediaStream->GetMediaFormat().IsMediaType(mediaType)) && mediaStream->IsSource() == source)
      return mediaStream;
    ++mediaStream;
  }

  return NULL;
}


bool OpalConnection::GetMediaTransportAddresses(OpalConnection & otherConnection,
                                                      unsigned   sessionId,
                                           const OpalMediaType & mediaType,
                                     OpalTransportAddressArray & transports) const
{
  return m_endpoint.GetMediaTransportAddresses(*this, otherConnection, sessionId, mediaType, transports);
}


PBoolean OpalConnection::SetAudioVolume(PBoolean /*source*/, unsigned /*percentage*/)
{
  return false;
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


bool OpalConnection::SetBandwidthAvailable(OpalBandwidth::Direction dir, OpalBandwidth available)
{
  PTRACE(3, "Setting " << dir << " bandwidth available to " << available << " on connection " << *this);

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


bool OpalConnection::SetBandwidthAllocated(OpalBandwidth::Direction dir, OpalBandwidth newBandwidth)
{
  OpalBandwidth used = GetBandwidthUsed(dir);
  if (used <= newBandwidth)
    return SetBandwidthAvailable(dir, newBandwidth - used);

  PTRACE(2, "Cannot set " << dir << " bandwidth to " << newBandwidth
          << ", currently using " << used << " on connection " << *this);
  return false;
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

  PTRACE(4, "Using " << dir << " bandwidth of " << used << " for " << *this);

  return used;
}


bool OpalConnection::SetBandwidthUsed(OpalBandwidth::Direction dir,
                                      OpalBandwidth releasedBandwidth,
                                      OpalBandwidth requiredBandwidth)
{
  PTRACE_IF(3, releasedBandwidth > 0, "Releasing " << dir << " bandwidth of " << releasedBandwidth);

  OpalBandwidth bandwidthAvailable = GetBandwidthAvailable(dir) + releasedBandwidth;
  if (requiredBandwidth > bandwidthAvailable) {
    PTRACE(2, "Insufficient " << dir << " bandwidth request of "
            << requiredBandwidth << ", available: " << bandwidthAvailable);
    return false;
  }

  PTRACE_IF(3, requiredBandwidth > 0, "Requesting " << dir << " bandwidth of "
            << requiredBandwidth << ", available: " << bandwidthAvailable);

  return SetBandwidthAvailable(dir, bandwidthAvailable - requiredBandwidth);
}


void OpalConnection::SetSendUserInputMode(SendUserInputModes mode)
{
  PTRACE(3, "Setting default User Input send mode to " << mode);
  m_sendUserInputMode = mode;
}


PBoolean OpalConnection::SendUserInputString(const PString & value)
{
  for (const char * c = value; *c != '\0'; c++) {
    if (!SendUserInputTone(*c, 0))
      return false;
  }
  return true;
}


#if OPAL_PTLIB_DTMF
PBoolean OpalConnection::SendUserInputTone(char tone, unsigned duration)
{
  if (m_dtmfSendFormat.IsEmpty())
    return false;

  if (duration <= 0)
    duration = PDTMFEncoder::DefaultToneLen;

  PTRACE(3, "Sending in-band DTMF tone '" << tone << "', duration=" << duration);

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
  m_endpoint.OnUserInputString(*this, value);
}


void OpalConnection::OnUserInputTone(char tone, unsigned duration)
{
  m_endpoint.OnUserInputTone(*this, tone, duration);
}


PString OpalConnection::GetUserInput(unsigned timeout)
{
  PString reply;
  if (m_userInputAvailable.Wait(PTimeInterval(0, timeout)) && !IsReleased() && LockReadWrite()) {
    reply = m_userInputString;
    m_userInputString = PString();
    UnlockReadWrite();
  }
  return reply;
}


void OpalConnection::SetUserInput(const PString & input)
{
  if (LockReadWrite()) {
    m_userInputString += input;
    m_userInputAvailable.Signal();
    UnlockReadWrite();
  }
}


PString OpalConnection::ReadUserInput(const char * terminators,
                                      unsigned lastDigitTimeout,
                                      unsigned firstDigitTimeout)
{
  return m_endpoint.ReadUserInput(*this, terminators, lastDigitTimeout, firstDigitTimeout);
}


PBoolean OpalConnection::PromptUserInput(PBoolean /*play*/)
{
  return true;
}


#if OPAL_PTLIB_DTMF
void OpalConnection::OnDetectInBandDTMF(RTP_DataFrame & frame, P_INT_PTR)
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
    PTRACE(3, "DTMF detected: \"" << tones << '"');
    for (PINDEX i = 0; i < tones.GetLength(); i++)
      GetEndPoint().GetManager().QueueDecoupledEvent(new PSafeWorkArg2<OpalConnection, char, unsigned>(
                            this, tones[i], PDTMFDecoder::DetectTime, &OpalConnection::OnUserInputTone));
  }
}

void OpalConnection::OnSendInBandDTMF(RTP_DataFrame & frame, P_INT_PTR)
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
    PTRACE(4, "Sent in-band DTMF tone, " << m_inBandDTMF.GetSize() << " bytes");
    m_inBandDTMF.SetSize(0);
    m_emittedInBandDTMF = 0;
  }

  m_inBandMutex.Signal();
}
#endif


PString OpalConnection::GetPrefixName() const
{
  return m_endpoint.GetPrefixName();
}


void OpalConnection::SetLocalPartyName(const PString & name)
{
  m_localPartyName = name;
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
  if (!m_remotePartyURL.IsEmpty())
    return m_remotePartyURL;

  if (!m_remotePartyNumber.IsEmpty())
    return MakeURL(GetPrefixName(), m_remotePartyNumber);

  if (!m_remotePartyName.IsEmpty())
    return MakeURL(GetPrefixName(), m_remotePartyName);

  return MakeURL(GetPrefixName(), "*");
}


PString OpalConnection::GetCalledPartyURL()
{
  return MakeURL(GetPrefixName(), GetDestinationAddress());
}


void OpalConnection::CopyPartyNames(const OpalConnection & other)
{
  if (IsNetworkConnection()) {
    m_localPartyName = other.GetRemoteIdentity();
    if (m_localPartyName.NumCompare(other.GetPrefixName()+':') == EqualTo)
      m_localPartyName.Delete(0, other.GetPrefixName().GetLength()+1);
    if (m_localPartyName.NumCompare(GetPrefixName()+':') != EqualTo)
      m_localPartyName.Splice(GetPrefixName()+':', 0);
    m_displayName = other.GetRemotePartyName();
  }
  else {
    m_remotePartyName = other.GetRemotePartyName();
    m_remotePartyNumber = other.GetRemotePartyNumber();
    m_remotePartyURL = other.GetRemotePartyURL();
    m_calledPartyName = other.GetCalledPartyName();
    m_calledPartyNumber = other.GetCalledPartyNumber();
    m_remoteProductInfo = other.GetRemoteProductInfo();
  }
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


PString OpalConnection::GetSupportedFeatures() const
{
  return PString::Empty();
}


bool OpalConnection::GetConferenceState(OpalConferenceState *) const
{
  return false;
}


bool OpalConnection::RequestPresentationRole(bool)
{
  return false;
}


bool OpalConnection::OnChangedPresentationRole(const PString & newChairURI, bool request)
{
  return m_endpoint.GetManager().OnChangedPresentationRole(*this, newChairURI, request);
}


bool OpalConnection::HasPresentationRole() const
{
  return false;
}


void OpalConnection::SetAudioJitterDelay(unsigned minDelay, unsigned maxDelay)
{
  if (minDelay != 0 || maxDelay != 0) {
    minDelay = std::max(10U, std::min(minDelay, 999U));
    maxDelay = std::max(minDelay, std::min(maxDelay, 999U));
  }

  m_jitterParams.m_minJitterDelay = minDelay;
  m_jitterParams.m_maxJitterDelay = maxDelay;
}


PString OpalConnection::GetIdentifier() const
{
  return GetToken();
}


PINDEX OpalConnection::GetMaxRtpPayloadSize() const
{
  return m_endpoint.GetManager().GetMaxRtpPayloadSize();
}


void OpalConnection::SetPhase(Phases phaseToSet)
{
  PWaitAndSignal mutex(m_phaseMutex);

  // With next few lines we will prevent phase to ever go down when it
  // reaches ReleasingPhase - end result - once you call Release you never
  // go back.
  if (m_phase < ReleasingPhase || (m_phase == ReleasingPhase && phaseToSet == ReleasedPhase)) {
    PTRACE(3, "Setting phase from " << m_phase << " to " << phaseToSet << " for " << *this);
    m_phase = phaseToSet;
    if (!m_phaseTime[m_phase].IsValid())
      m_phaseTime[m_phase].SetCurrentTime();
  }
  else {
    PTRACE(2, "Cannot set phase from " << m_phase << " to " << phaseToSet << " for " << *this);
  }
}


void OpalConnection::SetStringOptions(const StringOptions & options, bool overwrite)
{
  if (overwrite)
    m_stringOptions = options;
  else {
    if (options.IsEmpty())
      return;
    for (PStringToString::const_iterator it = options.begin(); it != options.end(); ++it)
      m_stringOptions.SetAt(it->first, it->second);
  }

  OnApplyStringOptions();
}

void OpalConnection::OnApplyStringOptions()
{
  m_endpoint.GetManager().OnApplyStringOptions(*this, m_stringOptions);

  PTRACE_IF(4, !m_stringOptions.IsEmpty(), "Applying string options:\n" << m_stringOptions);

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

    m_autoStartInfo.Add(m_stringOptions(OPAL_OPT_AUTO_START));

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

    if (m_silenceDetector != NULL && !(str = m_stringOptions(OPAL_OPT_SILENCE_DETECT_MODE)).IsEmpty()) {
      OpalSilenceDetector::Params params;
      m_silenceDetector->GetParameters(params);
      params.FromString(str);
      m_silenceDetector->SetParameters(params);
    }

    UnlockReadWrite();
  }
}


OpalMediaFormatList OpalConnection::GetMediaFormats() const
{
  return m_endpoint.GetMediaFormats();
}


OpalMediaFormatList OpalConnection::GetLocalMediaFormats()
{
  if (m_localMediaFormats.IsEmpty())
    m_localMediaFormats = m_ownerCall.GetMediaFormats(*this);
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


bool OpalConnection::OnMediaFailed(unsigned sessionId)
{
  if (IsReleased())
    return false;

  m_mediaSessionFailedMutex.Wait();
  m_mediaSessionFailed.insert(sessionId);
  m_mediaSessionFailedMutex.Signal();

  return GetEndPoint().GetManager().OnMediaFailed(*this, sessionId);
}


bool OpalConnection::AllMediaFailed() const
{
  for (StreamDict::const_iterator it = m_mediaStreams.begin(); it != m_mediaStreams.end(); ++it) {
    OpalMediaStreamPtr mediaStream = it->second;
    if (mediaStream != NULL) {
      PWaitAndSignal lock(m_mediaSessionFailedMutex);
      if (m_mediaSessionFailed.find(mediaStream->GetSessionID()) == m_mediaSessionFailed.end()) {
        PTRACE(3, "Checking for all media failed: no, still have media stream " << *mediaStream << " for " << *this);
        return false;
      }
    }
  }

  return true;
}


bool OpalConnection::OnMediaCommand(OpalMediaStream & stream, const OpalMediaCommand & command)
{
  if (&stream.GetConnection() != this) {
    PTRACE(4, "Ended processing OnMediaCommand \"" << command << "\" on " << stream << " for " << *this);
    return false;
  }

  PSafePtr<OpalConnection> other = GetOtherPartyConnection();
  if (other == NULL)
    return false;

  PTRACE(4, "Passing on OnMediaCommand \"" << command << "\" on " << stream << " to " << *other);
  return other->OnMediaCommand(stream, command);
}


void OpalConnection::InternalExecuteMediaCommand(OpalMediaCommand * command)
{
  ExecuteMediaCommand(*command, false);
  delete command;
}


bool OpalConnection::ExecuteMediaCommand(const OpalMediaCommand & command, bool async) const
{
  if (async) {
    GetEndPoint().GetManager().QueueDecoupledEvent(
                    new PSafeWorkArg1<OpalConnection, OpalMediaCommand *>(
                            const_cast<OpalConnection *>(this),
                            command.CloneAs<OpalMediaCommand>(),
                            &OpalConnection::InternalExecuteMediaCommand));
    return true;
  }

  PSafeLockReadOnly safeLock(*this);
  if (!safeLock.IsLocked())
    return false;

  if (IsReleased())
    return false;

  OpalMediaStreamPtr stream = command.GetSessionID() != 0 ? GetMediaStream(command.GetSessionID(), false)
                                                          : GetMediaStream(command.GetMediaType(), false);
  if (stream == NULL) {
    PTRACE(3, "No " << command.GetMediaType() << " stream to do " << command << " in connection " << *this);
    return false;
  }

  PTRACE(5, "Execute " << command.GetMediaType() << " stream command " << command << " in connection " << *this);
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


void OpalConnection::StringOptions::ExtractFromString(PString & str)
{
  PINDEX semicolon = str.Find(';');
  if (semicolon == P_MAX_INDEX)
    return;

  PStringToString params;
  PURL::SplitVars(str.Mid(semicolon), params, ';', '=');

  for (PStringToString::iterator it = params.begin(); it != params.end(); ++it) {
    PString key = it->first;
    if (key.NumCompare(OPAL_URL_PARAM_PREFIX) == EqualTo)
      key.Delete(0, 5);
    SetAt(key, it->second);
  }

  str.Delete(semicolon, P_MAX_INDEX);
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
