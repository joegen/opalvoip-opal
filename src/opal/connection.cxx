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
#include <rtp/rtp.h>
#include <t120/t120proto.h>
#include <t38/t38proto.h>
#include <h224/h224handler.h>

#if OPAL_T38FAX
#include <t38/t38proto.h>
#endif

#define new PNEW


#if PTRACING
ostream & operator<<(ostream & out, OpalConnection::Phases phase)
{
  static const char * const names[OpalConnection::NumPhases] = {
    "UninitialisedPhase",
    "SetUpPhase",
    "AlertingPhase",
    "ConnectedPhase",
    "EstablishedPhase",
    "ReleasingPhase",
    "ReleasedPhase"
  };
  return out << names[phase];
}


ostream & operator<<(ostream & out, OpalConnection::CallEndReason reason)
{
  const char * const names[OpalConnection::NumCallEndReasons] = {
    "EndedByLocalUser",         /// Local endpoint application cleared call
    "EndedByNoAccept",          /// Local endpoint did not accept call OnIncomingCall()=PFalse
    "EndedByAnswerDenied",      /// Local endpoint declined to answer call
    "EndedByRemoteUser",        /// Remote endpoint application cleared call
    "EndedByRefusal",           /// Remote endpoint refused call
    "EndedByNoAnswer",          /// Remote endpoint did not answer in required time
    "EndedByCallerAbort",       /// Remote endpoint stopped calling
    "EndedByTransportFail",     /// Transport error cleared call
    "EndedByConnectFail",       /// Transport connection failed to establish call
    "EndedByGatekeeper",        /// Gatekeeper has cleared call
    "EndedByNoUser",            /// Call failed as could not find user (in GK)
    "EndedByNoBandwidth",       /// Call failed as could not get enough bandwidth
    "EndedByCapabilityExchange",/// Could not find common capabilities
    "EndedByCallForwarded",     /// Call was forwarded using FACILITY message
    "EndedBySecurityDenial",    /// Call failed a security check and was ended
    "EndedByLocalBusy",         /// Local endpoint busy
    "EndedByLocalCongestion",   /// Local endpoint congested
    "EndedByRemoteBusy",        /// Remote endpoint busy
    "EndedByRemoteCongestion",  /// Remote endpoint congested
    "EndedByUnreachable",       /// Could not reach the remote party
    "EndedByNoEndPoint",        /// The remote party is not running an endpoint
    "EndedByOffline",           /// The remote party is off line
    "EndedByTemporaryFailure",  /// The remote failed temporarily app may retry
    "EndedByQ931Cause",         /// The remote ended the call with unmapped Q.931 cause code
    "EndedByDurationLimit",     /// Call cleared due to an enforced duration limit
    "EndedByInvalidConferenceID",/// Call cleared due to invalid conference ID
    "EndedByNoDialTone",        /// Call cleared due to missing dial tone
    "EndedByNoRingBackTone",    /// Call cleared due to missing ringback tone    
    "EndedByOutOfService",      /// Call cleared because the line is out of service, 
    "EndedByAcceptingCallWaiting", /// Call cleared because another call is answered
  };
  PAssert((PINDEX)(reason & 0xff) < PARRAYSIZE(names), "Invalid reason");
  return out << names[reason & 0xff];
}

ostream & operator<<(ostream & o, OpalConnection::AnswerCallResponse s)
{
  static const char * const AnswerCallResponseNames[OpalConnection::NumAnswerCallResponses] = {
    "AnswerCallNow",
    "AnswerCallDenied",
    "AnswerCallPending",
    "AnswerCallDeferred",
    "AnswerCallAlertWithMedia",
    "AnswerCallDeferredWithMedia",
    "AnswerCallProgress",
    "AnswerCallNowAndReleaseCurrent"
  };
  if ((PINDEX)s >= PARRAYSIZE(AnswerCallResponseNames))
    o << "InvalidAnswerCallResponse<" << (unsigned)s << '>';
  else if (AnswerCallResponseNames[s] == NULL)
    o << "AnswerCallResponse<" << (unsigned)s << '>';
  else
    o << AnswerCallResponseNames[s];
  return o;
}

ostream & operator<<(ostream & o, OpalConnection::SendUserInputModes m)
{
  static const char * const SendUserInputModeNames[OpalConnection::NumSendUserInputModes] = {
    "SendUserInputAsQ931",
    "SendUserInputAsString",
    "SendUserInputAsTone",
    "SendUserInputAsRFC2833",
    "SendUserInputAsSeparateRFC2833",
    "SendUserInputAsProtocolDefault"
  };

  if ((PINDEX)m >= PARRAYSIZE(SendUserInputModeNames))
    o << "InvalidSendUserInputMode<" << (unsigned)m << '>';
  else if (SendUserInputModeNames[m] == NULL)
    o << "SendUserInputMode<" << (unsigned)m << '>';
  else
    o << SendUserInputModeNames[m];
  return o;
}

#endif

/////////////////////////////////////////////////////////////////////////////

OpalConnection::OpalConnection(OpalCall & call,
                               OpalEndPoint  & ep,
                               const PString & token,
                               unsigned int options,
                               OpalConnection::StringOptions * _stringOptions)
  : PSafeObject(&call), // Share the lock flag from the call
    ownerCall(call),
    endpoint(ep),
    phase(UninitialisedPhase),
    callToken(token),
    originating(PFalse),
    alertingTime(0),
    connectedTime(0),
    callEndTime(0),
    productInfo(ep.GetProductInfo()),
    localPartyName(ep.GetDefaultLocalPartyName()),
    displayName(ep.GetDefaultDisplayName()),
    remotePartyName(token),
    callEndReason(NumCallEndReasons),
    remoteIsNAT(PFalse),
    q931Cause(0x100),
    silenceDetector(NULL),
    echoCanceler(NULL),
#if OPAL_T120DATA
    t120handler(NULL),
#endif
#if OPAL_T38FAX
    t38handler(NULL),
#endif
#if OPAL_H224
    h224Handler(NULL),
#endif
    stringOptions((_stringOptions == NULL) ? NULL : new OpalConnection::StringOptions(*_stringOptions))
{
  PTRACE(3, "OpalCon\tCreated connection " << *this);

  PAssert(ownerCall.SafeReference(), PLogicError);

  // Set an initial value for the A-Party name, if we are in fact the A-Party
  if (ownerCall.connectionsActive.IsEmpty())
    ownerCall.partyA = localPartyName;

  ownerCall.connectionsActive.Append(this);

  detectInBandDTMF = !endpoint.GetManager().DetectInBandDTMFDisabled();
  minAudioJitterDelay = endpoint.GetManager().GetMinAudioJitterDelay();
  maxAudioJitterDelay = endpoint.GetManager().GetMaxAudioJitterDelay();
  bandwidthAvailable = endpoint.GetInitialBandwidth();

  switch (options & SendDTMFMask) {
    case SendDTMFAsString:
      sendUserInputMode = SendUserInputAsString;
      break;
    case SendDTMFAsTone:
      sendUserInputMode = SendUserInputAsTone;
      break;
    case SendDTMFAsRFC2833:
      sendUserInputMode = SendUserInputAsInlineRFC2833;
      break;
    case SendDTMFAsDefault:
    default:
      sendUserInputMode = ep.GetSendUserInputMode();
      break;
  }
  
  rfc2833Handler  = new OpalRFC2833Proto(*this, PCREATE_NOTIFIER(OnUserInputInlineRFC2833));
#if OPAL_T38FAX
  ciscoNSEHandler = new OpalRFC2833Proto(*this, PCREATE_NOTIFIER(OnUserInputInlineCiscoNSE));
#endif

  securityMode = ep.GetDefaultSecurityMode();

  switch (options & RTPAggregationMask) {
    case RTPAggregationDisable:
      useRTPAggregation = PFalse;
      break;
    case RTPAggregationEnable:
      useRTPAggregation = PTrue;
      break;
    default:
      useRTPAggregation = endpoint.UseRTPAggregation();
  }

  if (stringOptions != NULL) {
    PString id((*stringOptions)("Call-Identifier"));
    if (!id.IsEmpty())
      callIdentifier = PGloballyUniqueID(id);
  }

  autoStartMediaMap = ep.GetAutoStartMediaMap();
}

OpalConnection::~OpalConnection()
{
  mediaStreams.RemoveAll();

  delete silenceDetector;
  delete echoCanceler;
  delete rfc2833Handler;
#if OPAL_T120DATA
  delete t120handler;
#endif
#if OPAL_T38FAX
  delete ciscoNSEHandler;
  delete t38handler;
#endif
#if OPAL_H224
  delete h224Handler;
#endif
  delete stringOptions;

  ownerCall.connectionsActive.Remove(this);
  ownerCall.SafeDereference();

  PTRACE(3, "OpalCon\tConnection " << *this << " destroyed.");
}


void OpalConnection::PrintOn(ostream & strm) const
{
  strm << ownerCall << '-'<< endpoint << '[' << callToken << ']';
}

PBoolean OpalConnection::OnSetUpConnection()
{
  PTRACE(3, "OpalCon\tOnSetUpConnection" << *this);
  return endpoint.OnSetUpConnection(*this);
}

void OpalConnection::HoldConnection()
{
  
}


void OpalConnection::RetrieveConnection()
{
  
}


PBoolean OpalConnection::IsConnectionOnHold()
{
  return PFalse;
}

void OpalConnection::SetCallEndReason(CallEndReason reason)
{
  // Only set reason if not already set to something
  if (callEndReason == NumCallEndReasons) {
    if ((reason & EndedWithQ931Code) != 0) {
      SetQ931Cause((int)reason >> 24);
      reason = (CallEndReason)(reason & 0xff);
    }
    PTRACE(3, "OpalCon\tCall end reason for " << GetToken() << " set to " << reason);
    callEndReason = reason;
  }
}


void OpalConnection::ClearCall(CallEndReason reason)
{
  // Now set reason for the connection close
  SetCallEndReason(reason);
  ownerCall.Clear(reason);
}


void OpalConnection::ClearCallSynchronous(PSyncPoint * sync, CallEndReason reason)
{
  // Now set reason for the connection close
  SetCallEndReason(reason);
  ownerCall.Clear(reason, sync);
}


void OpalConnection::TransferConnection(const PString & PTRACE_PARAM(remoteParty),
                                        const PString & /*callIdentity*/)
{
  PTRACE(2, "OpalCon\tCan not transfer connection to " << remoteParty);
}


void OpalConnection::Release(CallEndReason reason)
{
  {
    PWaitAndSignal m(phaseMutex);
    if (phase >= ReleasingPhase) {
      PTRACE(2, "OpalCon\tAlready released " << *this);
      return;
    }
    SetPhase(ReleasingPhase);
  }

  {
    PSafeLockReadWrite safeLock(*this);
    if (!safeLock.IsLocked()) {
      PTRACE(2, "OpalCon\tAlready released " << *this);
      return;
    }

    PTRACE(3, "OpalCon\tReleasing " << *this);

    // Now set reason for the connection close
    SetCallEndReason(reason);

    // Add a reference for the thread we are about to start
    SafeReference();
  }

  PThread::Create(PCREATE_NOTIFIER(OnReleaseThreadMain), 0,
                  PThread::AutoDeleteThread,
                  PThread::NormalPriority,
                  "OnRelease:%x");
}


void OpalConnection::OnReleaseThreadMain(PThread &, INT)
{
  OnReleased();

  PTRACE(4, "OpalCon\tOnRelease thread completed for " << GetToken());

  // Dereference on the way out of the thread
  SafeDereference();
}


void OpalConnection::OnReleased()
{
  PTRACE(3, "OpalCon\tOnReleased " << *this);

  CloseMediaStreams();

  endpoint.OnReleased(*this);
}


PBoolean OpalConnection::OnIncomingConnection()
{
  return PTrue;
}


PBoolean OpalConnection::OnIncomingConnection(unsigned int /*options*/)
{
  return PTrue;
}


PBoolean OpalConnection::OnIncomingConnection(unsigned options, OpalConnection::StringOptions * stringOptions)
{
  return OnIncomingConnection() &&
         OnIncomingConnection(options) &&
         endpoint.OnIncomingConnection(*this, options, stringOptions);
}


PString OpalConnection::GetDestinationAddress()
{
  return "*";
}


PBoolean OpalConnection::ForwardCall(const PString & /*forwardParty*/)
{
  return PFalse;
}


void OpalConnection::OnAlerting()
{
  endpoint.OnAlerting(*this);
}

OpalConnection::AnswerCallResponse OpalConnection::OnAnswerCall(const PString & callerName)
{
  return endpoint.OnAnswerCall(*this, callerName);
}

void OpalConnection::AnsweringCall(AnswerCallResponse /*response*/)
{
}

void OpalConnection::OnConnected()
{
  endpoint.OnConnected(*this);
}


void OpalConnection::OnEstablished()
{
  endpoint.OnEstablished(*this);
}


void OpalConnection::AdjustMediaFormats(OpalMediaFormatList & mediaFormats) const
{
  endpoint.AdjustMediaFormats(*this, mediaFormats);
}

PBoolean OpalConnection::OpenSourceMediaStream(const OpalMediaFormatList & mediaFormats, const OpalMediaSessionId & sessionID)
{
  PSafeLockReadWrite safeLock(*this);
  if (!safeLock.IsLocked())
    return PFalse;

  // See if already opened
  OpalMediaStream * stream = GetMediaStream(sessionID, PTrue);

  if (stream != NULL && stream->IsOpen()) {
    PTRACE(3, "OpalCon\tOpenSourceMediaStream (already opened) for session " << sessionID.sessionId << " on " << *this);
    return PTrue;
  }

    // see if sink stream already opened, so we can ensure symmetric codecs
    OpalMediaFormatList toFormats = mediaFormats;
    {
      OpalMediaStream * sink = GetMediaStream(sessionID, PFalse);
      if (sink != NULL) {
        PTRACE(3, "OpalCon\tOpenSourceMediaStream reordering codec for sink stream format " << sink->GetMediaFormat());
        toFormats.Reorder(sink->GetMediaFormat().GetName());
      }
    }

  OpalMediaFormat sourceFormat, destinationFormat;
  if (!OpalTranscoder::SelectFormats(sessionID.mediaType,
                                    GetMediaFormats(),
                                    toFormats,
                                    sourceFormat,
                                    destinationFormat)) {
    PTRACE(2, "OpalCon\tOpenSourceMediaStream session " << sessionID.mediaType
          << ", could not find compatible media format:\n"
              "  source formats=" << setfill(',') << GetMediaFormats() << "\n"
              "   sink  formats=" << mediaFormats << setfill(' '));
    return PFalse;
  }

  PTRACE(3, "OpalCon\tSelected media stream " << sourceFormat << " -> " << destinationFormat);
  
  if (stream == NULL)
    stream = InternalCreateMediaStream(sourceFormat, sessionID, PTrue);

  if (stream == NULL) {
    PTRACE(1, "OpalCon\tCreateMediaStream returned NULL for session "
          << sessionID.sessionId << " on " << *this);
    return PFalse;
  }

  if (stream->Open()) {
    if (OnOpenMediaStream(*stream)) {
      PTRACE(3, "OpalCon\tOpened source stream " << stream->GetID());
      return PTrue;
    }
    PTRACE(2, "OpalCon\tSource media OnOpenMediaStream open of " << sourceFormat << " failed.");
  }
  else {
    PTRACE(2, "OpalCon\tSource media stream open of " << sourceFormat << " failed.");
  }

  if (!RemoveMediaStream(stream))
    delete stream;

  return PFalse;
}


OpalMediaStream * OpalConnection::OpenSinkMediaStream(OpalMediaStream & source)
{
  OpalMediaSessionId sessionID = source.GetSessionID();
  
  PTRACE(3, "OpalCon\tOpenSinkMediaStream " << *this << " session=" << sessionID.sessionId);

  OpalMediaFormat sourceFormat = source.GetMediaFormat();

  // Reorder the media formats from this protocol so we give preference
  // to what has been selected in the source media stream.
  OpalMediaFormatList destinationFormats = GetMediaFormats();
  PString format = sourceFormat;
  PStringArray order = format;
  // Second preference is given to the previous media stream already
  // opened to maintain symmetric codecs, if possible.

  PSafeLockReadWrite safeLock(*this);
  if (!safeLock.IsLocked())
    return NULL;

  OpalMediaStream * otherStream = GetMediaStream(sessionID, PTrue);
  if (otherStream != NULL)
    order += otherStream->GetMediaFormat();
  destinationFormats.Reorder(order);

  OpalMediaFormat destinationFormat;
  if (!OpalTranscoder::SelectFormats(sourceFormat.GetMediaType(),
                                    sourceFormat, // Only use selected format on source
                                    destinationFormats,
                                    sourceFormat,
                                    destinationFormat)) {
    PTRACE(2, "OpalCon\tOpenSinkMediaStream, could not find compatible media format:\n"
              "  source formats=" << setfill(',') << sourceFormat << "\n"
              "   sink  formats=" << destinationFormats << setfill(' '));
    return NULL;
  }

  PTRACE(3, "OpalCon\tOpenSinkMediaStream, selected " << sourceFormat << " -> " << destinationFormat);

  OpalMediaStream * stream = InternalCreateMediaStream(destinationFormat, sessionID, PFalse);
  if (stream == NULL) {
    PTRACE(1, "OpalCon\tCreateMediaStream " << *this << " returned NULL");
    return NULL;
  }

  if (stream->Open()) {
    if (OnOpenMediaStream(*stream)) {
      PTRACE(3, "OpalCon\tOpened sink stream " << stream->GetID());
      return stream;
    }
    PTRACE(2, "OpalCon\tSink media stream OnOpenMediaStream of " << destinationFormat << " failed.");
  }
  else {
    PTRACE(2, "OpalCon\tSink media stream open of " << destinationFormat << " failed.");
  }

  if (!RemoveMediaStream(stream))
    delete stream;

  return NULL;
}


void OpalConnection::StartMediaStreams()
{
  if (!LockReadWrite())
    return;

  for (PINDEX i = 0; i < mediaStreams.GetSize(); i++) {
    if (mediaStreams[i].IsOpen()) {
      OpalMediaStream & strm = mediaStreams[i];
      strm.Start();
    }
  }

  UnlockReadWrite();
  
  PTRACE(3, "OpalCon\tMedia stream threads started.");
}


void OpalConnection::CloseMediaStreams()
{
  GetCall().OnStopRecordAudio(callIdentifier.AsString());

  if (!LockReadWrite())
    return;

  for (PINDEX i = 0; i < mediaStreams.GetSize(); i++) {
    OpalMediaStream & strm = mediaStreams[i];
    if (strm.IsOpen()) {
      OnClosedMediaStream(strm);
      strm.Close();
    }
  }

  UnlockReadWrite();
  
  PTRACE(3, "OpalCon\tMedia stream threads closed.");
}


void OpalConnection::RemoveMediaStreams()
{
  if (!LockReadWrite())
    return;

  CloseMediaStreams();
  mediaStreams.RemoveAll();

  UnlockReadWrite();
  
  PTRACE(3, "OpalCon\tMedia stream threads removed from session.");
}

void OpalConnection::PauseMediaStreams(PBoolean paused)
{
  if (!LockReadWrite())
    return;

  for (PINDEX i = 0; i < mediaStreams.GetSize(); i++)
    mediaStreams[i].SetPaused(paused);

  UnlockReadWrite();
}


OpalMediaStream * OpalConnection::CreateMediaStream(
  const OpalMediaFormat & mediaFormat,
  const OpalMediaSessionId & sessionID,
  PBoolean isSource)
{
  return InternalCreateMediaStream(mediaFormat, sessionID, isSource);
}


PBoolean OpalConnection::OnOpenMediaStream(OpalMediaStream & stream)
{
  if (!endpoint.OnOpenMediaStream(*this, stream))
    return PFalse;

  if (!LockReadWrite())
    return PFalse;

  if (mediaStreams.GetObjectsIndex(&stream) == P_MAX_INDEX)
    mediaStreams.Append(&stream);

  if (GetPhase() == ConnectedPhase) {
    SetPhase(EstablishedPhase);
    OnEstablished();
  }

  UnlockReadWrite();

  return PTrue;
}


void OpalConnection::OnClosedMediaStream(const OpalMediaStream & stream)
{
  endpoint.OnClosedMediaStream(stream);
}


void OpalConnection::OnPatchMediaStream(PBoolean /*isSource*/, OpalMediaPatch & /*patch*/)
{
  if (!recordAudioFilename.IsEmpty())
    GetCall().StartRecording(recordAudioFilename);

  // TODO - add autorecord functions here
  PTRACE(3, "OpalCon\tNew patch created");
}

void OpalConnection::EnableRecording()
{
  if (!LockReadWrite())
    return;

  OpalMediaStream * stream = GetMediaStream(OpalMediaSessionId("audio"), PTrue);
  if (stream != NULL) {
    OpalMediaPatch * patch = stream->GetPatch();
    if (patch != NULL)
      patch->AddFilter(PCREATE_NOTIFIER(OnRecordAudio), OPAL_PCM16);
  }
  
  UnlockReadWrite();
}

void OpalConnection::DisableRecording()
{
  if (!LockReadWrite())
    return;

  OpalMediaStream * stream = GetMediaStream(OpalMediaSessionId("audio"), PTrue);
  if (stream != NULL) {
    OpalMediaPatch * patch = stream->GetPatch();
    if (patch != NULL)
      patch->RemoveFilter(PCREATE_NOTIFIER(OnRecordAudio), OPAL_PCM16);
  }
  
  UnlockReadWrite();
}

void OpalConnection::OnRecordAudio(RTP_DataFrame & frame, INT)
{
  GetCall().GetManager().GetRecordManager().WriteAudio(GetCall().GetToken(), callIdentifier.AsString(), frame);
}

void OpalConnection::AttachRFC2833HandlerToPatch(PBoolean isSource, OpalMediaPatch & patch)
{
  if (rfc2833Handler != NULL) {
    if(isSource) {
      PTRACE(3, "OpalCon\tAdding RFC2833 receive handler");
      OpalMediaStream & mediaStream = patch.GetSource();
      patch.AddFilter(rfc2833Handler->GetReceiveHandler(), mediaStream.GetMediaFormat());
    } 
  }

#if OPAL_T38FAX
  if (ciscoNSEHandler != NULL) {
    if(isSource) {
      PTRACE(3, "OpalCon\tAdding Cisco NSE receive handler");
      OpalMediaStream & mediaStream = patch.GetSource();
      patch.AddFilter(ciscoNSEHandler->GetReceiveHandler(), mediaStream.GetMediaFormat());
    } 
  }
#endif
}


OpalMediaStream * OpalConnection::GetMediaStream(const OpalMediaSessionId & sessionId, PBoolean source) const
{
  PSafeLockReadWrite safeLock(*this);
  if (!safeLock.IsLocked())
    return NULL;

  for (PINDEX i = 0; i < mediaStreams.GetSize(); i++) {
    if (mediaStreams[i].GetMediaFormat().GetMediaType() == sessionId.mediaType &&
        mediaStreams[i].IsSource() == source)
      return &mediaStreams[i];
  }

  return NULL;
}

bool OpalConnection::HasMedia() const
{
  PSafeLockReadWrite safeLock(*this);
  if (!safeLock.IsLocked())
    return NULL;

  for (PINDEX i = 0; i < mediaStreams.GetSize(); i++) {
    if (mediaStreams[i].IsSource())
      return true;
  }

  return false;
}

PBoolean OpalConnection::RemoveMediaStream(OpalMediaStream * strm)
{
  PSafeLockReadWrite safeLock(*this);
  if (!safeLock.IsLocked())
    return PFalse;

  PINDEX index = mediaStreams.GetObjectsIndex(strm);
  if (index == P_MAX_INDEX) 
    return PFalse;

  OpalMediaStream & s = mediaStreams[index];
  if (s.IsOpen()) {
    OnClosedMediaStream(s);
    s.Close();
  }
  mediaStreams.RemoveAt(index);

  return PTrue;
}


PBoolean OpalConnection::IsMediaBypassPossible(const OpalMediaSessionId & sessionID) const
{
  OpalMediaTypeDefinition * mediaDef = OpalMediaTypeFactory::CreateInstance(sessionID.mediaType);
  return (mediaDef != NULL) && mediaDef->IsMediaBypassPossible();
}

PBoolean OpalConnection::GetMediaInformation(const OpalMediaSessionId & sessionID,
                                                     MediaInformation & info) const
{
  if (!mediaTransportAddresses.Contains(sessionID.sessionId)) {
    PTRACE(2, "OpalCon\tGetMediaInformation for session " << sessionID.sessionId << " - no channel.");
    return PFalse;
  }

  OpalTransportAddress & address = mediaTransportAddresses[sessionID.sessionId];

  PIPSocket::Address ip;
  WORD port;
  if (address.GetIpAndPort(ip, port)) {
    info.data    = OpalTransportAddress(ip, (WORD)(port&0xfffe));
    info.control = OpalTransportAddress(ip, (WORD)(port|0x0001));
  }
  else
    info.data = info.control = address;

  info.rfc2833 = rfc2833Handler->GetPayloadType();
  PTRACE(3, "OpalCon\tGetMediaInformation for session " << sessionID.sessionId
         << " data=" << info.data << " rfc2833=" << info.rfc2833);
  return PTrue;
}

#if OPAL_VIDEO

void OpalConnection::AddVideoMediaFormats(OpalMediaFormatList & mediaFormats) const
{
  endpoint.AddVideoMediaFormats(mediaFormats, this);
}


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

#endif // OPAL_VIDEO


PBoolean OpalConnection::SetAudioVolume(PBoolean /*source*/, unsigned /*percentage*/)
{
  return PFalse;
}


unsigned OpalConnection::GetAudioSignalLevel(PBoolean /*source*/)
{
    return UINT_MAX;
}


RTP_Session * OpalConnection::GetSession(const OpalMediaSessionId & sessionID) const
{
  return rtpSessions.GetSession(sessionID);
}

RTP_Session * OpalConnection::UseSession(const OpalMediaSessionId & sessionID)
{
  return rtpSessions.UseSession(sessionID);
}

RTP_Session * OpalConnection::UseSession(const OpalTransport & transport,
                                         const OpalMediaSessionId & sessionID,
                                         RTP_QOS * rtpqos)
{
  RTP_Session * rtpSession = rtpSessions.UseSession(sessionID);
  if (rtpSession == NULL) {
    rtpSession = CreateSession(transport, sessionID, rtpqos);
    rtpSessions.AddSession(rtpSession);
  }

  return rtpSession;
}


void OpalConnection::ReleaseSession(const OpalMediaSessionId & sessionID,
                                    PBoolean clearAll)
{
  rtpSessions.ReleaseSession(sessionID, clearAll);
}


RTP_Session * OpalConnection::CreateSession(const OpalTransport & transport,
                                       const OpalMediaSessionId & sessionID,
                                                        RTP_QOS * rtpqos)
{
  OpalMediaTypeDefinition * mediaDefn = OpalMediaTypeFactory::CreateInstance(sessionID.mediaType);
  if (mediaDefn == NULL) {
    PTRACE(1, "OpalCon\tCannot create session for media type " << sessionID.mediaType);
    return NULL;
  }

  // We only support RTP over UDP at this point in time ...
  if (!transport.IsCompatibleTransport("ip$127.0.0.1")) {
    PTRACE(1, "OpalCon\tCannot create session for media type " << sessionID.mediaType);
    return NULL;
  }

  PIPSocket::Address localAddress;
  transport.GetLocalAddress().GetIpAddress(localAddress);

  OpalManager & manager = GetEndPoint().GetManager();

  PIPSocket::Address remoteAddress;
  transport.GetRemoteAddress().GetIpAddress(remoteAddress);
  PSTUNClient * stun = manager.GetSTUN(remoteAddress);

  // create an (S)RTP session or T38 pseudo-session as appropriate
  RTP_UDP * rtpSession = NULL;

  if (!securityMode.IsEmpty()) {
    OpalSecurityMode * parms = PFactory<OpalSecurityMode>::CreateInstance(securityMode);
    if (parms == NULL) {
      PTRACE(1, "OpalCon\tSecurity mode " << securityMode << " unknown");
      return NULL;
    }
    rtpSession = parms->CreateRTPSession(
                  useRTPAggregation ? endpoint.GetRTPAggregator() : NULL, 
                  sessionID.sessionId, remoteIsNAT);
    if (rtpSession == NULL) {
      PTRACE(1, "OpalCon\tCannot create RTP session for security mode " << securityMode);
      delete parms;
      return NULL;
    }
  }
  else
  {
    rtpSession = mediaDefn->CreateNonSecureSession(*this, 
                                                   useRTPAggregation ? endpoint.GetRTPAggregator() : NULL, 
                                                   sessionID, 
                                                   remoteIsNAT);
  }

  WORD firstPort = manager.GetRtpIpPortPair();
  WORD nextPort = firstPort;
  while (!rtpSession->Open(localAddress,
                           nextPort, nextPort,
                           manager.GetRtpIpTypeofService(),
                           stun,
                           rtpqos)) {
    nextPort = manager.GetRtpIpPortPair();
    if (nextPort == firstPort) {
      PTRACE(1, "OpalCon\tNo ports available for RTP session " << sessionID.sessionId << " for " << *this);
      delete rtpSession;
      return NULL;
    }
  }

  localAddress = rtpSession->GetLocalAddress();
  if (manager.TranslateIPAddress(localAddress, remoteAddress))
    rtpSession->SetLocalAddress(localAddress);
  return rtpSession;
}


PBoolean OpalConnection::SetBandwidthAvailable(unsigned newBandwidth, PBoolean force)
{
  PTRACE(3, "OpalCon\tSetting bandwidth to " << newBandwidth << "00b/s on connection " << *this);

  unsigned used = GetBandwidthUsed();
  if (used > newBandwidth) {
    if (!force)
      return PFalse;

#if 0
    // Go through media channels and close down some.
    PINDEX chanIdx = GetmediaStreams->GetSize();
    while (used > newBandwidth && chanIdx-- > 0) {
      OpalChannel * channel = logicalChannels->GetChannelAt(chanIdx);
      if (channel != NULL) {
        used -= channel->GetBandwidthUsed();
        const H323ChannelNumber & number = channel->GetNumber();
        CloseLogicalChannel(number, number.IsFromRemote());
      }
    }
#endif
  }

  bandwidthAvailable = newBandwidth - used;
  return PTrue;
}


unsigned OpalConnection::GetBandwidthUsed() const
{
  unsigned used = 0;

#if 0
  for (PINDEX i = 0; i < logicalChannels->GetSize(); i++) {
    OpalChannel * channel = logicalChannels->GetChannelAt(i);
    if (channel != NULL)
      used += channel->GetBandwidthUsed();
  }
#endif

  PTRACE(3, "OpalCon\tBandwidth used is "
         << used << "00b/s for " << *this);

  return used;
}


PBoolean OpalConnection::SetBandwidthUsed(unsigned releasedBandwidth,
                                      unsigned requiredBandwidth)
{
  PTRACE_IF(3, releasedBandwidth > 0, "OpalCon\tBandwidth release of "
            << releasedBandwidth/10 << '.' << releasedBandwidth%10 << "kb/s");

  bandwidthAvailable += releasedBandwidth;

  PTRACE_IF(3, requiredBandwidth > 0, "OpalCon\tBandwidth request of "
            << requiredBandwidth/10 << '.' << requiredBandwidth%10
            << "kb/s, available: "
            << bandwidthAvailable/10 << '.' << bandwidthAvailable%10
            << "kb/s");

  if (requiredBandwidth > bandwidthAvailable) {
    PTRACE(2, "OpalCon\tAvailable bandwidth exceeded on " << *this);
    return PFalse;
  }

  bandwidthAvailable -= requiredBandwidth;

  return PTrue;
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


PBoolean OpalConnection::SendUserInputTone(char tone, unsigned duration)
{
  if (duration == 0)
    duration = 180;

  return rfc2833Handler->SendToneAsync(tone, duration);
}


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
  if (userInputAvailable.Wait(PTimeInterval(0, timeout)) && LockReadWrite()) {
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


void OpalConnection::OnUserInputInlineRFC2833(OpalRFC2833Info & info, INT)
{
  if (!info.IsToneStart())
    OnUserInputTone(info.GetTone(), info.GetDuration()/8);
}

void OpalConnection::OnUserInputInlineCiscoNSE(OpalRFC2833Info & /*info*/, INT)
{
  cout << "Received NSE event" << endl;
  //if (!info.IsToneStart())
  //  OnUserInputTone(info.GetTone(), info.GetDuration()/8);
}

#if P_DTMF
void OpalConnection::OnUserInputInBandDTMF(RTP_DataFrame & frame, INT)
{
  // This function is set up as an 'audio filter'.
  // This allows us to access the 16 bit PCM audio (at 8Khz sample rate)
  // before the audio is passed on to the sound card (or other output device)

  // Pass the 16 bit PCM audio through the DTMF decoder   
  PString tones = dtmfDecoder.Decode((const short *)frame.GetPayloadPtr(), frame.GetPayloadSize()/sizeof(short));
  if (!tones.IsEmpty()) {
    PTRACE(3, "OPAL\tDTMF detected. " << tones);
    PINDEX i;
    for (i = 0; i < tones.GetLength(); i++) {
      OnUserInputTone(tones[i], 0);
    }
  }
}
#endif

#if OPAL_T120DATA

OpalT120Protocol * OpalConnection::CreateT120ProtocolHandler()
{
  if (t120handler == NULL)
    t120handler = endpoint.CreateT120ProtocolHandler(*this);
  return t120handler;
}

#endif

void OpalConnection::SetLocalPartyName(const PString & name)
{
  localPartyName = name;
}


void OpalConnection::SetAudioJitterDelay(unsigned minDelay, unsigned maxDelay)
{
  maxDelay = PMAX(10, PMIN(maxDelay, 999));
  minDelay = PMAX(10, PMIN(minDelay, 999));

  if (maxDelay < minDelay)
    maxDelay = minDelay;

  minAudioJitterDelay = minDelay;
  maxAudioJitterDelay = maxDelay;
}

void OpalConnection::SetPhase(Phases phaseToSet)
{
  PTRACE(3, "OpalCon\tSetPhase from " << phase << " to " << phaseToSet);

  PWaitAndSignal m(phaseMutex);

  // With next few lines we will prevent phase to ever go down when it
  // reaches ReleasingPhase - end result - once you call Release you never
  // go back.
  if (phase < ReleasingPhase) {
    phase = phaseToSet;
  } else if (phase == ReleasingPhase && phaseToSet == ReleasedPhase) {
    phase = phaseToSet;
  }
}

PBoolean OpalConnection::OnOpenIncomingMediaChannels()
{
  return PTrue;
}

void OpalConnection::SetStringOptions(StringOptions * options)
{
  if (LockReadWrite()) {
    if (stringOptions != NULL)
      delete stringOptions;
    stringOptions = options;
    UnlockReadWrite();
  }
}


void OpalConnection::ApplyStringOptions()
{
  if (stringOptions != NULL && LockReadWrite()) {
    if (stringOptions->Contains("Disable-Jitter"))
      maxAudioJitterDelay = minAudioJitterDelay = 0;
    PString str = (*stringOptions)("Max-Jitter");
    if (!str.IsEmpty())
      maxAudioJitterDelay = str.AsUnsigned();
    str = (*stringOptions)("Min-Jitter");
    if (!str.IsEmpty())
      minAudioJitterDelay = str.AsUnsigned();
    if (stringOptions->Contains("Record-Audio"))
      recordAudioFilename = (*stringOptions)("Record-Audio");
    UnlockReadWrite();
  }
}

void OpalConnection::PreviewPeerMediaFormats(const OpalMediaFormatList & /*fmts*/)
{
}

PBoolean OpalConnection::IsRTPNATEnabled(const PIPSocket::Address & localAddr, 
                           const PIPSocket::Address & peerAddr,
                           const PIPSocket::Address & sigAddr,
                                                 PBoolean incoming)
{
  return endpoint.IsRTPNATEnabled(*this, localAddr, peerAddr, sigAddr, incoming);
}

OpalMediaStream * OpalConnection::InternalCreateMediaStream(const OpalMediaFormat & mediaFormat, const OpalMediaSessionId & sessionID, PBoolean isSource)
{
  OpalMediaTypeDefinition * defn = OpalMediaTypeFactory::CreateInstance(mediaFormat.GetMediaType());
  if (defn == NULL) {
    PTRACE(2, "OpalCon\tCannot create media stream for unknown media type " << mediaFormat.GetMediaType());
    return NULL;
  }

  return defn->CreateMediaStream(*this, mediaFormat, sessionID, isSource);
}

OpalMediaFormatList OpalConnection::GetLocalMediaFormats()
{
  return ownerCall.GetMediaFormats(*this, FALSE);
}

/////////////////////////////////////////////////////////////////////////////

bool OpalAutoStartMediaMap::IsMediaAutoStart(const OpalMediaType & mediaType, PBoolean rx) const
{
  PWaitAndSignal m(mutex);
  const_iterator r = find(mediaType);
  return (r != end()) && (rx ? r->second.autoStartReceive : r->second.autoStartTransmit);
}

void OpalAutoStartMediaMap::SetMediaAutoStart(const OpalMediaType & mediaType, PBoolean rx, PBoolean v)
{
  PWaitAndSignal m(mutex);
  OpalAutoStartMediaMap::iterator r = find(mediaType);
  OpalMediaAutoStartInfo info;
  if (r != end())
    info = r->second;

  if (rx)
    info.autoStartReceive = v;
  else
    info.autoStartTransmit = v;

  insert(value_type(mediaType, info));
}


/////////////////////////////////////////////////////////////////////////////
