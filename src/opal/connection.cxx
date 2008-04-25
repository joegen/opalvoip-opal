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
#include <ptclib/url.h>

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
  : PSafeObject(&call)  // Share the lock flag from the call
  , ownerCall(call)
  , endpoint(ep)
  , phase(UninitialisedPhase)
  , callToken(token)
  , originating(PFalse)
  , alertingTime(0)
  , connectedTime(0)
  , callEndTime(0)
  , productInfo(ep.GetProductInfo())
  , localPartyName(ep.GetDefaultLocalPartyName())
  , displayName(ep.GetDefaultDisplayName())
  , remotePartyName(token)
  , callEndReason(NumCallEndReasons)
  , q931Cause(0x100)
  , silenceDetector(NULL)
  , echoCanceler(NULL)
#if OPAL_T120DATA
  , t120handler(NULL)
#endif
#if OPAL_T38FAX
  , t38handler(NULL)
#endif
  , stringOptions((_stringOptions == NULL) ? NULL : new OpalConnection::StringOptions(*_stringOptions))
#ifdef OPAL_STATISTICS
  , m_VideoUpdateRequestsSent(0)
#endif
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

  dtmfScaleMultiplier = dtmfScaleDivisor = 1;

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
  
  if (stringOptions != NULL) {
    PString str((*stringOptions)("Call-Identifier"));
    if (!str.IsEmpty())
      callIdentifier = PGloballyUniqueID(str);

    str = (*stringOptions)("enableinbanddtmf");
    if (!str.IsEmpty())
      detectInBandDTMF = str *= "true";
    str = (*stringOptions)("dtmfmult");
    if (!str.IsEmpty()) {
      dtmfScaleMultiplier = str.AsInteger();
      dtmfScaleDivisor    = 1;
    }
    str = (*stringOptions)("dtmfdiv");
    if (!str.IsEmpty())
      dtmfScaleDivisor = str.AsInteger();
  }
}

OpalConnection::~OpalConnection()
{
  mediaStreams.RemoveAll();

  delete silenceDetector;
  delete echoCanceler;
#if OPAL_T120DATA
  delete t120handler;
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


bool OpalConnection::GarbageCollection()
{
  return mediaStreams.DeleteObjectsToBeRemoved();
}


PBoolean OpalConnection::OnSetUpConnection()
{
  PTRACE(3, "OpalCon\tOnSetUpConnection" << *this);
  return endpoint.OnSetUpConnection(*this);
}


bool OpalConnection::HoldConnection()
{
  return false;
}


bool OpalConnection::RetrieveConnection()
{
  return false;
}


PBoolean OpalConnection::IsConnectionOnHold()
{
  return PFalse;
}


void OpalConnection::OnHold(bool fromRemote, bool onHold)
{
  PTRACE(4, "OpalCon\tOnHold " << *this);
  endpoint.OnHold(*this, fromRemote, onHold);
}


void OpalConnection::SetCallEndReason(CallEndReason reason)
{
  // Only set reason if not already set to something
  if (callEndReason == NumCallEndReasons) {
    if ((reason & EndedWithQ931Code) != 0) {
      SetQ931Cause((int)reason >> 24);
      reason = (CallEndReason)(reason & 0xff);
    }
    PTRACE(3, "OpalCon\tCall end reason for " << *this << " set to " << reason);
    callEndReason = reason;
    ownerCall.SetCallEndReason(reason);
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


bool OpalConnection::TransferConnection(const PString & PTRACE_PARAM(remoteParty))
{
  PTRACE(2, "OpalCon\tCan not transfer connection to " << remoteParty);
  return false;
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
                  "OnRelease");
}


void OpalConnection::OnReleaseThreadMain(PThread &, INT)
{
  OnReleased();

  PTRACE(4, "OpalCon\tOnRelease thread completed for " << *this);

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
  StartMediaStreams();
  endpoint.OnEstablished(*this);
}


void OpalConnection::AdjustMediaFormats(OpalMediaFormatList & mediaFormats) const
{
  endpoint.AdjustMediaFormats(*this, mediaFormats);
}


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
  if (!stream.Close())
    return false;

  if (stream.IsSource())
    return true;

  PSafePtr<OpalConnection> otherConnection = GetCall().GetOtherPartyConnection(*this);
  if (otherConnection == NULL)
    return true;

  OpalMediaStreamPtr otherStream = otherConnection->GetMediaStream(stream.GetSessionID(), true);
  if (otherStream == NULL)
    return true;

  return otherStream->Close();
}


PBoolean OpalConnection::RemoveMediaStream(OpalMediaStream & stream)
{
  stream.Close();
  return mediaStreams.Remove(&stream);
}


void OpalConnection::StartMediaStreams()
{
  for (OpalMediaStreamPtr mediaStream = mediaStreams; mediaStream != NULL; ++mediaStream)
    mediaStream->Start();

  PTRACE(3, "OpalCon\tMedia stream threads started.");
}


void OpalConnection::CloseMediaStreams()
{
  GetCall().OnStopRecordAudio(callIdentifier.AsString());

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


void OpalConnection::PauseMediaStreams(PBoolean paused)
{
  for (OpalMediaStreamPtr mediaStream = mediaStreams; mediaStream != NULL; ++mediaStream)
    mediaStream->SetPaused(paused);
}


OpalMediaStream * OpalConnection::CreateMediaStream(
#if OPAL_VIDEO
  const OpalMediaFormat & mediaFormat,
  unsigned sessionID,
  PBoolean isSource
#else
  const OpalMediaFormat & ,
  unsigned ,
  PBoolean 
#endif
  )
{
#if OPAL_VIDEO
  if (sessionID == OpalMediaFormat::DefaultVideoSessionID) {
    if (isSource) {
      PVideoInputDevice * videoDevice;
      PBoolean autoDelete;
      if (CreateVideoInputDevice(mediaFormat, videoDevice, autoDelete)) {
        PVideoOutputDevice * previewDevice;
        if (!CreateVideoOutputDevice(mediaFormat, PTrue, previewDevice, autoDelete))
          previewDevice = NULL;
        return new OpalVideoMediaStream(*this, mediaFormat, sessionID, videoDevice, previewDevice, autoDelete);
      }
    }
    else {
      PVideoOutputDevice * videoDevice;
      PBoolean autoDelete;
      if (CreateVideoOutputDevice(mediaFormat, PFalse, videoDevice, autoDelete))
        return new OpalVideoMediaStream(*this, mediaFormat, sessionID, NULL, videoDevice, autoDelete);
    }
  }
#endif

  return NULL;
}


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
  endpoint.OnClosedMediaStream(stream);
}


void OpalConnection::OnPatchMediaStream(PBoolean PTRACE_PARAM(isSource), OpalMediaPatch & PTRACE_PARAM(patch))
{
  if (!recordAudioFilename.IsEmpty())
    GetCall().StartRecording(recordAudioFilename);

  // TODO - add autorecord functions here
  PTRACE(3, "OpalCon\t" << (isSource ? "Source" : "Sink") << " stream of connection " << *this << " uses patch " << patch);
}

void OpalConnection::EnableRecording()
{
  if (!LockReadWrite())
    return;

  OpalMediaStreamPtr stream = GetMediaStream(OpalMediaFormat::DefaultAudioSessionID, PTrue);
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

  OpalMediaStreamPtr stream = GetMediaStream(OpalMediaFormat::DefaultAudioSessionID, PTrue);
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

void OpalConnection::AttachRFC2833HandlerToPatch(PBoolean /*isSource*/, OpalMediaPatch & /*patch*/)
{
}


OpalMediaStreamPtr OpalConnection::GetMediaStream(unsigned sessionId, bool source) const
{
  for (OpalMediaStreamPtr mediaStream(mediaStreams, PSafeReference); mediaStream != NULL; ++mediaStream) {
    if (mediaStream->GetSessionID() == sessionId && mediaStream->IsSource() == source)
      return mediaStream;
  }

  return NULL;
}



PBoolean OpalConnection::IsMediaBypassPossible(unsigned /*sessionID*/) const
{
  PTRACE(4, "OpalCon\tIsMediaBypassPossible: default returns false");
  return false;
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


PBoolean OpalConnection::SendUserInputTone(char /*tone*/, unsigned /*duration*/)
{
  return false;
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


#if P_DTMF
void OpalConnection::OnUserInputInBandDTMF(RTP_DataFrame & frame, INT)
{
  // This function is set up as an 'audio filter'.
  // This allows us to access the 16 bit PCM audio (at 8Khz sample rate)
  // before the audio is passed on to the sound card (or other output device)

  // Pass the 16 bit PCM audio through the DTMF decoder   
  PString tones = dtmfDecoder.Decode((const short *)frame.GetPayloadPtr(), frame.GetPayloadSize()/sizeof(short), dtmfScaleMultiplier, dtmfScaleDivisor);
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

#if OPAL_T38FAX

OpalT38Protocol * OpalConnection::CreateT38ProtocolHandler()
{
  if (t38handler == NULL)
    t38handler = endpoint.CreateT38ProtocolHandler(*this);
  return t38handler;
}

#endif

void OpalConnection::SetLocalPartyName(const PString & name)
{
  localPartyName = name;
}


PString OpalConnection::GetLocalPartyURL() const
{
  return endpoint.GetPrefixName() + ':' + PURL::TranslateString(GetLocalPartyName(), PURL::LoginTranslation);
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

PINDEX OpalConnection::GetMaxRtpPayloadSize() const
{ 
  return endpoint.GetManager().GetMaxRtpPayloadSize(); 
}


void OpalConnection::SetPhase(Phases phaseToSet)
{
  PTRACE(3, "OpalCon\tSetPhase from " << phase << " to " << phaseToSet << " for " << *this);

  PWaitAndSignal m(phaseMutex);

  // With next few lines we will prevent phase to ever go down when it
  // reaches ReleasingPhase - end result - once you call Release you never
  // go back.
  if (phase < ReleasingPhase || (phase == ReleasingPhase && phaseToSet == ReleasedPhase))
    phase = phaseToSet;
}


PBoolean OpalConnection::OnOpenIncomingMediaChannels()
{
  return true;
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

OpalMediaFormatList OpalConnection::GetLocalMediaFormats()
{
  if (localMediaFormats.IsEmpty())
    localMediaFormats = ownerCall.GetMediaFormats(*this, FALSE);
  return localMediaFormats;
}

void OpalConnection::OnMediaPatchStart(unsigned, bool)
{ }

void OpalConnection::OnMediaPatchStop(unsigned,  bool )
{ }

void OpalConnection::OnMediaCommand(OpalMediaCommand & /*command*/, INT /*extra*/)
{
}


/////////////////////////////////////////////////////////////////////////////
