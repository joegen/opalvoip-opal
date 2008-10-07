/*
 * pcss.cxx
 *
 * PC Sound system support.
 *
 * Open Phone Abstraction Library (OPAL)
 *
 * Copyright (c) 2000 Equivalence Pty. Ltd.
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
#pragma implementation "pcss.h"
#endif

#include <opal/buildopts.h>
#include <opal/pcss.h>

#if OPAL_VIDEO
#include <ptlib/videoio.h>
#include <codec/vidcodec.h>
#endif

#include <ptlib/sound.h>
#include <codec/silencedetect.h>
#include <codec/echocancel.h>
#include <opal/call.h>
#include <opal/patch.h>
#include <opal/manager.h>


#define new PNEW


/////////////////////////////////////////////////////////////////////////////

OpalPCSSEndPoint::OpalPCSSEndPoint(OpalManager & mgr, const char * prefix)
  : OpalEndPoint(mgr, prefix, CanTerminateCall),
    soundChannelPlayDevice(PSoundChannel::GetDefaultDevice(PSoundChannel::Player)),
    soundChannelRecordDevice(PSoundChannel::GetDefaultDevice(PSoundChannel::Recorder))
{
#ifdef _WIN32
  // Windows MultMedia stuff seems to need greater depth due to enormous
  // latencies in its operation
  soundChannelBuffers = 6;
#else
  // Should only need double buffering for Unix platforms
  soundChannelBuffers = 2;
#endif

  PTRACE(3, "PCSS\tCreated PC sound system endpoint.\n" << setfill('\n')
         << "Players:\n"   << PSoundChannel::GetDeviceNames(PSoundChannel::Player)
         << "Recorders:\n" << PSoundChannel::GetDeviceNames(PSoundChannel::Recorder));
}


OpalPCSSEndPoint::~OpalPCSSEndPoint()
{
  PTRACE(4, "PCSS\tDeleted PC sound system endpoint.");
}


static PBoolean SetDeviceName(const PString & name,
                          PSoundChannel::Directions dir,
                          PString & result)
{
  if (name[0] == '#') {
    PStringArray devices = PSoundChannel::GetDeviceNames(dir);
    PINDEX id = name.Mid(1).AsUnsigned();
    if (id == 0 || id > devices.GetSize())
      return PFalse;
    result = devices[id-1];
  }
  else {
    PSoundChannel * pChannel = PSoundChannel::CreateChannelByName(name, dir);
    if (pChannel == NULL)
      return PFalse;
    delete pChannel;
    result = name;
  }

  return PTrue;
}


PBoolean OpalPCSSEndPoint::MakeConnection(OpalCall & call,
                                      const PString & remoteParty,
                                      void * userData,
                               unsigned int /*options*/,
                               OpalConnection::StringOptions *)
{
  // First strip of the prefix if present
  PINDEX prefixLength = 0;
  if (remoteParty.Find(GetPrefixName()+":") == 0)
    prefixLength = GetPrefixName().GetLength()+1;

  PString playDevice;
  PString recordDevice;
  PINDEX separator = remoteParty.FindOneOf("\n\t\\", prefixLength);
  if (separator == P_MAX_INDEX)
    playDevice = recordDevice = remoteParty.Mid(prefixLength);
  else {
    playDevice = remoteParty(prefixLength+1, separator-1);
    recordDevice = remoteParty.Mid(separator+1);
  }

  if (!SetDeviceName(playDevice, PSoundChannel::Player, playDevice))
    playDevice = soundChannelPlayDevice;
  if (!SetDeviceName(recordDevice, PSoundChannel::Recorder, recordDevice))
    recordDevice = soundChannelRecordDevice;

  return AddConnection(CreateConnection(call, playDevice, recordDevice, userData));
}


OpalMediaFormatList OpalPCSSEndPoint::GetMediaFormats() const
{
  OpalMediaFormatList formats;

  // Sound cards can only do 16 bit PCM, but at various sample rates
  // The following will be in order of preference, so lets do wideband first
  formats += OpalPCM16_16KHZ;
  formats += OpalPCM16;

#if OPAL_VIDEO
  AddVideoMediaFormats(formats);
#endif

  return formats;
}


OpalPCSSConnection * OpalPCSSEndPoint::CreateConnection(OpalCall & call,
                                                        const PString & playDevice,
                                                        const PString & recordDevice,
                                                        void * /*userData*/)
{
  return new OpalPCSSConnection(call, *this, playDevice, recordDevice);
}


PSoundChannel * OpalPCSSEndPoint::CreateSoundChannel(const OpalPCSSConnection & connection,
						     const OpalMediaFormat & mediaFormat,
                                                     PBoolean isSource)
{
  PString deviceName;
  PSoundChannel::Directions dir;
  if (isSource) {
    deviceName = connection.GetSoundChannelRecordDevice();
    dir = PSoundChannel::Recorder;
  }
  else {
    deviceName = connection.GetSoundChannelPlayDevice();
    dir = PSoundChannel::Player;
  }

  PSoundChannel * soundChannel = PSoundChannel::CreateChannelByName(deviceName, dir);
  if (soundChannel == NULL) {
    PTRACE(1, "PCSS\tCould not create sound channel \"" << deviceName
           << "\" for " << (isSource ? "record" : "play") << "ing.");
    return NULL;
  }

  if (soundChannel->Open(deviceName, dir, 1, mediaFormat.GetClockRate(), 16)) {
    PTRACE(3, "PCSS\tOpened sound channel \"" << deviceName
           << "\" for " << (isSource ? "record" : "play") << "ing.");
    return soundChannel;
  }

  PTRACE(1, "PCSS\tCould not open sound channel \"" << deviceName
         << "\" for " << (isSource ? "record" : "play")
         << "ing: " << soundChannel->GetErrorText());

  delete soundChannel;
  return NULL;
}


PSafePtr<OpalPCSSConnection> OpalPCSSEndPoint::GetPCSSConnectionWithLock(const PString & token, PSafetyMode mode)
{
  PSafePtr<OpalPCSSConnection> connection = PSafePtrCast<OpalConnection, OpalPCSSConnection>(GetConnectionWithLock(token, mode));
  if (connection == NULL) {
    PSafePtr<OpalCall> call = manager.FindCallWithLock(token, PSafeReadOnly);
    if (call != NULL) {
      connection = PSafePtrCast<OpalConnection, OpalPCSSConnection>(call->GetConnection(0));
      if (connection == NULL)
        connection = PSafePtrCast<OpalConnection, OpalPCSSConnection>(call->GetConnection(1));
    }
  }

  return connection;
}


PBoolean OpalPCSSEndPoint::AcceptIncomingConnection(const PString & token)
{
  PSafePtr<OpalPCSSConnection> connection = GetPCSSConnectionWithLock(token, PSafeReadOnly);
  if (connection == NULL) {
    PTRACE(2, "PCSS\tCould not find connection using token \"" << token << '"');
    return false;
  }

  connection->AcceptIncoming();
  return true;
}


PBoolean OpalPCSSEndPoint::RejectIncomingConnection(const PString & token)
{
  PSafePtr<OpalPCSSConnection> connection = GetPCSSConnectionWithLock(token, PSafeReadOnly);
  if (connection == NULL)
    return false;

  connection->Release(OpalConnection::EndedByAnswerDenied);
  return true;
}


PBoolean OpalPCSSEndPoint::OnShowUserInput(const OpalPCSSConnection &, const PString &)
{
  return PTrue;
}


void OpalPCSSEndPoint::OnPatchMediaStream(const OpalPCSSConnection & /*connection*/, PBoolean /*isSource*/, OpalMediaPatch & /*patch*/)
{
}


PBoolean OpalPCSSEndPoint::SetSoundChannelPlayDevice(const PString & name)
{
  return SetDeviceName(name, PSoundChannel::Player, soundChannelPlayDevice);
}


PBoolean OpalPCSSEndPoint::SetSoundChannelRecordDevice(const PString & name)
{
  return SetDeviceName(name, PSoundChannel::Recorder, soundChannelRecordDevice);
}


void OpalPCSSEndPoint::SetSoundChannelBufferDepth(unsigned depth)
{
  PAssert(depth > 1, PInvalidParameter);
  soundChannelBuffers = depth;
}


/////////////////////////////////////////////////////////////////////////////

static unsigned LastConnectionTokenID;

OpalPCSSConnection::OpalPCSSConnection(OpalCall & call,
                                       OpalPCSSEndPoint & ep,
                                       const PString & playDevice,
                                       const PString & recordDevice)
  : OpalConnection(call, ep, psprintf("%u", ++LastConnectionTokenID)),
    endpoint(ep),
    soundChannelPlayDevice(playDevice),
    soundChannelRecordDevice(recordDevice),
    soundChannelBuffers(ep.GetSoundChannelBufferDepth())
{
  silenceDetector = new OpalPCM16SilenceDetector;
  echoCanceler = new OpalEchoCanceler;

  PTRACE(4, "PCSS\tCreated PC sound system connection with token '" << callToken << "'");
}


OpalPCSSConnection::~OpalPCSSConnection()
{
  PTRACE(4, "PCSS\tDeleted PC sound system connection.");
}


PBoolean OpalPCSSConnection::SetUpConnection()
{
  originating = true;

  // Check if we are A-Party in thsi call, so need to do things differently
  if (ownerCall.GetConnection(0) == this) {
    phase = SetUpPhase;
    if (!OnIncomingConnection(0, NULL)) {
      Release(EndedByCallerAbort);
      return PFalse;
    }

    PTRACE(3, "PCSS\tOutgoing call routed to " << ownerCall.GetPartyB() << " for " << *this);
    if (!ownerCall.OnSetUp(*this)) {
      Release(EndedByNoAccept);
      return PFalse;
    }

    return PTrue;
  }

  {
    PSafePtr<OpalConnection> otherConn = ownerCall.GetOtherPartyConnection(*this);
    if (otherConn == NULL)
      return PFalse;

    remotePartyName    = otherConn->GetRemotePartyName();
    remotePartyAddress = otherConn->GetRemotePartyAddress();
    remoteProductInfo  = otherConn->GetRemoteProductInfo();
  }

  PTRACE(3, "PCSS\tSetUpConnection(" << remotePartyName << ')');
  phase = AlertingPhase;
  OnAlerting();

  return endpoint.OnShowIncoming(*this);
}


PBoolean OpalPCSSConnection::SetAlerting(const PString & calleeName, PBoolean)
{
  PTRACE(3, "PCSS\tSetAlerting(" << calleeName << ')');
  phase = AlertingPhase;
  remotePartyName = calleeName;
  return endpoint.OnShowOutgoing(*this);
}


PBoolean OpalPCSSConnection::SetConnected()
{
  PTRACE(3, "PCSS\tSetConnected()");

  if (mediaStreams.IsEmpty())
    phase = ConnectedPhase;
  else {
    phase = EstablishedPhase;
    OnEstablished();
  }

  return PTrue;
}


OpalMediaFormatList OpalPCSSConnection::GetMediaFormats() const
{
  return endpoint.GetMediaFormats();
}


OpalMediaStream * OpalPCSSConnection::CreateMediaStream(const OpalMediaFormat & mediaFormat,
                                                        unsigned sessionID,
                                                        PBoolean isSource)
{
  if (sessionID != OpalMediaFormat::DefaultAudioSessionID)
    return OpalConnection::CreateMediaStream(mediaFormat, sessionID, isSource);

  PSoundChannel * soundChannel = CreateSoundChannel(mediaFormat, isSource);
  if (soundChannel == NULL)
    return NULL;

  return new OpalAudioMediaStream(*this, mediaFormat, sessionID, isSource, soundChannelBuffers, soundChannel);
}


void OpalPCSSConnection::OnPatchMediaStream(PBoolean isSource,
					    OpalMediaPatch & patch)
{
  endpoint.OnPatchMediaStream(*this, isSource, patch);

  int clockRate;
  if (patch.GetSource().GetSessionID() == OpalMediaFormat::DefaultAudioSessionID) {
    PTRACE(3, "PCSS\tAdding filters to patch");
    if (isSource) {
      silenceDetector->SetParameters(endpoint.GetManager().GetSilenceDetectParams());
      patch.AddFilter(silenceDetector->GetReceiveHandler(), OpalPCM16);
    }
    clockRate = patch.GetSource().GetMediaFormat().GetClockRate();
    echoCanceler->SetParameters(endpoint.GetManager().GetEchoCancelParams());
    echoCanceler->SetClockRate(clockRate);
    patch.AddFilter(isSource?echoCanceler->GetReceiveHandler():echoCanceler->GetSendHandler(), OpalPCM16);
  }
}


OpalMediaStreamPtr OpalPCSSConnection::OpenMediaStream(const OpalMediaFormat & mediaFormat, unsigned sessionID, bool isSource)
{
#if OPAL_VIDEO
  if ( isSource &&
       sessionID == OpalMediaFormat::DefaultVideoSessionID &&
      !ownerCall.IsEstablished() &&
      !endpoint.GetManager().CanAutoStartTransmitVideo()) {
    PTRACE(3, "PCSS\tOpenMediaStream auto start disabled, refusing video open");
    return NULL;
  }
#endif

  return OpalConnection::OpenMediaStream(mediaFormat, sessionID, isSource);
}


PBoolean OpalPCSSConnection::SetAudioVolume(PBoolean source, unsigned percentage)
{
  PSafePtr<OpalAudioMediaStream> stream = PSafePtrCast<OpalMediaStream, OpalAudioMediaStream>(GetMediaStream(OpalMediaFormat::DefaultAudioSessionID, source));
  if (stream == NULL)
    return PFalse;

  PSoundChannel * channel = dynamic_cast<PSoundChannel *>(stream->GetChannel());
  if (channel == NULL)
    return PFalse;

  return channel->SetVolume(percentage);
}


unsigned OpalPCSSConnection::GetAudioSignalLevel(PBoolean source)
{
  PSafePtr<OpalAudioMediaStream> stream = PSafePtrCast<OpalMediaStream, OpalAudioMediaStream>(GetMediaStream(OpalMediaFormat::DefaultAudioSessionID, source));
  if (stream == NULL)
    return UINT_MAX;

  return stream->GetAverageSignalLevel();
}


PSoundChannel * OpalPCSSConnection::CreateSoundChannel(const OpalMediaFormat & mediaFormat, PBoolean isSource)
{
  return endpoint.CreateSoundChannel(*this, mediaFormat, isSource);
}


PBoolean OpalPCSSConnection::SendUserInputString(const PString & value)
{
  PTRACE(3, "PCSS\tSendUserInputString(" << value << ')');
  return endpoint.OnShowUserInput(*this, value);
}


void OpalPCSSConnection::AcceptIncoming()
{
  if (!LockReadOnly())
    return;

  if (phase != AlertingPhase) {
    UnlockReadOnly();
    return;
  }

  LockReadWrite();
  phase = ConnectedPhase;
  UnlockReadWrite();
  UnlockReadOnly();

  OnConnected();

  if (!LockReadOnly())
    return;

  if (mediaStreams.IsEmpty()) {
    UnlockReadOnly();
    return;
  }

  LockReadWrite();
  phase = EstablishedPhase;
  UnlockReadWrite();
  UnlockReadOnly();

  OnEstablished();
}

/////////////////////////////////////////////////////////////////////////////
