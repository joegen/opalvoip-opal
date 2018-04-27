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
 */

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "pcss.h"
#endif

#include <opal_config.h>

#include <ep/pcss.h>

#if OPAL_HAS_PCSS

#if OPAL_VIDEO
#include <ptlib/videoio.h>
#include <codec/vidcodec.h>
#endif

#include <ptlib/sound.h>
#include <ptclib/pwavfile.h>
#include <opal/call.h>
#include <opal/manager.h>
#include <opal/patch.h>
#include <lids/lid.h>


#define new PNEW
#define PTraceModule() "PCSS"


/////////////////////////////////////////////////////////////////////////////

OpalPCSSEndPoint::OpalPCSSEndPoint(OpalManager & mgr, const char * prefix)
  : OpalLocalEndPoint(mgr, prefix, false)
  , m_localRingbackTone("440+480:2.0-4.0") // US ringback tone
  , m_soundChannelPlayDevice(PSoundChannel::GetDefaultDevice(PSoundChannel::Player))
  , m_soundChannelRecordDevice(PSoundChannel::GetDefaultDevice(PSoundChannel::Recorder))
  , m_soundChannelOnHoldDevice(P_NULL_AUDIO_DEVICE)
  , m_soundChannelBuffers(2)
#ifdef _WIN32
  // Windows MultMedia stuff seems to need greater depth due to enormous
  // latencies in its operation
  , m_soundChannelBufferTime(120)
#else
  // Should only need double buffering for Unix platforms
  , m_soundChannelBufferTime(40)
#endif
{
  PTRACE(3, "Created PC sound system endpoint.\n" << setfill('\n')
         << "Player=" << m_soundChannelPlayDevice << ", available devices:\n"
         << PSoundChannel::GetDeviceNames(PSoundChannel::Player)
         << "Recorders=" << m_soundChannelRecordDevice << ", available devices:\n"
         << PSoundChannel::GetDeviceNames(PSoundChannel::Recorder));

#if OPAL_VIDEO
  m_videoOnHoldDevice.deviceName = P_FAKE_VIDEO_TEXT "=Please Hold";
#endif

  SetDeferredAnswer(true);
  SetPauseTransmitMediaOnHold(false);
}


OpalPCSSEndPoint::~OpalPCSSEndPoint()
{
  PTRACE(4, "Deleted PC sound system endpoint.");
}


static bool SetDeviceName(const PString & name,
                          PSoundChannel::Directions dir,
                          PString & result)
{
  if (name.IsEmpty() || name == "\t")
    return true;

  // First see if the channel can be created from the name alone, this
  // picks up *.wav style names.
  PSoundChannel * channel = PSoundChannel::CreateChannelByName(name, dir);
  if (channel != NULL) {
    delete channel;
    result = name;
    return true;
  }

  // Otherwise, look for a partial match.

  // Need to create unique names sans driver
  PStringList uniqueNames;
  PStringArray devices = PSoundChannel::GetDeviceNames(dir);
  for (PINDEX i = 0; i < devices.GetSize(); ++i) {
    PCaselessString deviceName = devices[i];
    PINDEX tab = deviceName.Find('\t');
    if (tab != P_MAX_INDEX)
      deviceName.Delete(0, tab+1);
    if (uniqueNames.GetValuesIndex(deviceName) == P_MAX_INDEX)
      uniqueNames.AppendString(deviceName);
  }

  int partialMatch = -1;
  for (PINDEX index = 0; index < uniqueNames.GetSize(); index++) {
    PCaselessString deviceName = uniqueNames[index];

    // Perfect match?
    if (deviceName == name) {
      result = deviceName;
      return true;
    }

    // Partial match?
    if (deviceName.NumCompare(name) == PObject::EqualTo) {
      if (partialMatch == -1)
        partialMatch = index;
      else
        partialMatch = -2;
    }
  }

  // Not a partial match or ambiguous partial
  if (partialMatch < 0)
    return false;

  result = uniqueNames[partialMatch];
  return true;
}


static bool SetDeviceNames(const PString & remoteParty,
                           PString & playResult,
                           PString & recordResult
                           PTRACE_PARAM(, const char * operation))
{
  PINDEX prefixLength = remoteParty.Find(':');
  if (prefixLength == P_MAX_INDEX)
    prefixLength = 0;
  else
    ++prefixLength;

  PString playDevice, recordDevice;
  PINDEX separator = remoteParty.Find('|', prefixLength);
  if (separator == P_MAX_INDEX)
    separator = remoteParty.Find('\\', prefixLength);
  if (separator == P_MAX_INDEX)
    playDevice = recordDevice = remoteParty.Mid(prefixLength);
  else {
    playDevice = remoteParty(prefixLength, separator-1);
    recordDevice = remoteParty.Mid(separator+1);
  }

  if (playDevice.IsEmpty() || playDevice == "*")
    playDevice = playResult;
  if (!SetDeviceName(playDevice, PSoundChannel::Player, playResult)) {
    PTRACE(2, "Sound player device \"" << playDevice << "\" does not exist, " << operation << " aborted.");
    return false;
  }
  PTRACE(4, "Sound player device is \"" << playDevice << '"');

  if (recordDevice.IsEmpty() || recordDevice == "*")
    recordDevice = recordResult;
  if (!SetDeviceName(recordDevice, PSoundChannel::Recorder, recordResult)) {
    PTRACE(2, "Sound recording device \"" << recordDevice << "\" does not exist, " << operation << " aborted.");
    return false;
  }
  PTRACE(4, "Sound recording device is \"" << recordDevice << '"');

  return true;
}


PSafePtr<OpalConnection> OpalPCSSEndPoint::MakeConnection(OpalCall & call,
                                                     const PString & remoteParty,
                                                              void * userData,
                                                        unsigned int options,
                                     OpalConnection::StringOptions * stringOptions)
{
  PString deviceNames = remoteParty;
  OpalConnection::StringOptions localStringOptions;
  if (stringOptions == NULL)
    stringOptions = &localStringOptions;
  stringOptions->ExtractFromString(deviceNames);

  PString playDevice = m_soundChannelPlayDevice;
  PString recordDevice = m_soundChannelRecordDevice;
  if (!SetDeviceNames(deviceNames, playDevice, recordDevice PTRACE_PARAM(, "call"))) {
    call.Clear(OpalConnection::EndedByLocalBusy);
    return NULL;
  }

  return AddConnection(CreateConnection(call, playDevice, recordDevice, userData, options, stringOptions));
}


OpalPCSSConnection * OpalPCSSEndPoint::CreateConnection(OpalCall & call,
                                                        const PString & playDevice,
                                                        const PString & recordDevice,
                                                        void * /*userData*/,
                                                        unsigned options,
                                                        OpalConnection::StringOptions * stringOptions)
{
  return new OpalPCSSConnection(call, *this, playDevice, recordDevice, options, stringOptions);
}


PSoundChannel * OpalPCSSEndPoint::CreateSoundChannel(const OpalPCSSConnection & connection,
                                                        const OpalMediaFormat & mediaFormat,
                                                                     unsigned   sessionID,
                                                                         bool   isSource)
{
  if (!isSource)
    return CreateSoundChannel(connection, mediaFormat, sessionID, isSource, connection.GetSoundChannelPlayDevice());

  if (connection.GetCall().IsOnHold())
    return CreateSoundChannel(connection, mediaFormat, sessionID, isSource, connection.GetSoundChannelOnHoldDevice());

  if (connection.GetPhase() < OpalConnection::AlertingPhase && !connection.GetSoundChannelOnRingDevice().IsEmpty())
    return CreateSoundChannel(connection, mediaFormat, sessionID, isSource, connection.GetSoundChannelOnRingDevice());

  return CreateSoundChannel(connection, mediaFormat, sessionID, isSource, connection.GetSoundChannelRecordDevice());
}


PSoundChannel * OpalPCSSEndPoint::CreateSoundChannel(const OpalPCSSConnection & connection,
                                                        const OpalMediaFormat & mediaFormat,
                                                                     unsigned   sessionID,
                                                                         bool   isSource,
                                                                const PString & device)
{
  PSoundChannel::Params params;
  params.m_device = device;
  params.m_direction = isSource ? PSoundChannel::Recorder : PSoundChannel::Player;
  params.m_channels = mediaFormat.GetOptionInteger(OpalAudioFormat::ChannelsOption());
  params.m_sampleRate = mediaFormat.GetClockRate();

  params.m_device.Replace("%token%", connection.GetCall().GetToken(), true);
  params.m_device.Replace("%session%", sessionID, true);

  PSoundChannel * soundChannel = PSoundChannel::CreateOpenedChannel(params);
  if (soundChannel == NULL)
    return NULL;

  PTRACE_CONTEXT_ID_SET(*soundChannel, connection);

  PTRACE(3, "Opened "
              << ((params.m_channels == 1) ? "mono" : ((params.m_channels == 2) ? "stereo" : "multi-channel"))
              << " sound channel \"" << params.m_device
              << "\" for " << (isSource ? "record" : "play") << "ing at "
              << params.m_sampleRate/1000 << '.' << (params.m_sampleRate%1000)/100 << " kHz.");

  return soundChannel;
}


bool OpalPCSSEndPoint::OnOutgoingCall(const OpalLocalConnection & connection)
{
  return OnShowOutgoing(dynamic_cast<const OpalPCSSConnection &>(connection));
}


bool OpalPCSSEndPoint::OnIncomingCall(OpalLocalConnection & connection)
{
  if (m_deferredAnswer)
    return OnShowIncoming(dynamic_cast<const OpalPCSSConnection &>(connection));

  return OpalLocalEndPoint::OnIncomingCall(connection);
}


bool OpalPCSSEndPoint::OnUserInput(const OpalLocalConnection & connection, const PString & indication)
{
  return OnShowUserInput(dynamic_cast<const OpalPCSSConnection &>(connection), indication);
}


PBoolean OpalPCSSEndPoint::OnShowOutgoing(const OpalPCSSConnection & connection)
{
  return OpalLocalEndPoint::OnOutgoingCall(connection);
}


PBoolean OpalPCSSEndPoint::OnShowIncoming(const OpalPCSSConnection & connection)
{
  return OpalLocalEndPoint::OnIncomingCall(const_cast<OpalPCSSConnection &>(connection));
}


PBoolean OpalPCSSEndPoint::AcceptIncomingConnection(const PString & token)
{
  return AcceptIncomingCall(token);
}


PBoolean OpalPCSSEndPoint::RejectIncomingConnection(const PString & token, const OpalConnection::CallEndReason & reason)
{
  return RejectIncomingCall(token, reason);
}


PBoolean OpalPCSSEndPoint::OnShowUserInput(const OpalPCSSConnection &, const PString &)
{
  return true;
}


bool OpalPCSSEndPoint::SetLocalRingbackTone(const PString & tone)
{
  if (tone.IsEmpty())
    return false;

#if P_WAVFILE
  PWAVFile wavFile(tone, PFile::ReadOnly);
  if (wavFile.IsOpen()) {
    m_localRingbackTone = tone;
    return true;
  }
#endif

#if OPAL_LID
  OpalLineInterfaceDevice::T35CountryCodes code = OpalLineInterfaceDevice::GetCountryCode(tone);
  if (code != OpalLineInterfaceDevice::UnknownCountry) {
    PString toneSpec = OpalLineInterfaceDevice::GetCountryInfo(code).m_tone[OpalLineInterfaceDevice::RingTone];
    if (toneSpec.IsEmpty()) {
      PTRACE(2, "Country code \"" << tone << "\" does not have a ringback tone specified");
      return false;
    }

    if (!PTones().Generate(toneSpec)) {
      PTRACE(2, "Country code \"" << tone << "\" had illegal tone specification \"" << toneSpec << '"');
      return false;
    }

    m_localRingbackTone = toneSpec;
    return true;
  }
#endif // OPAL_LID

  if (!PTones().Generate(tone)) {
    PTRACE(2, "Illegal tone specification \"" << tone << '"');
    return false;
  }

  m_localRingbackTone = tone;
  return true;
}


PBoolean OpalPCSSEndPoint::SetSoundChannelPlayDevice(const PString & name)
{
  return SetDeviceName(name, PSoundChannel::Player, m_soundChannelPlayDevice);
}


PBoolean OpalPCSSEndPoint::SetSoundChannelRecordDevice(const PString & name)
{
  return SetDeviceName(name, PSoundChannel::Recorder, m_soundChannelRecordDevice);
}


bool OpalPCSSEndPoint::SetSoundChannelOnHoldDevice(const PString & name)
{
  return SetDeviceName(name, PSoundChannel::Recorder, m_soundChannelOnHoldDevice);
}


bool OpalPCSSEndPoint::SetSoundChannelOnRingDevice(const PString & name)
{
  if (!name.IsEmpty())
    return SetDeviceName(name, PSoundChannel::Recorder, m_soundChannelOnRingDevice);

  m_soundChannelOnRingDevice.MakeEmpty();
  return true;
}


void OpalPCSSEndPoint::SetSoundChannelBufferDepth(unsigned depth)
{
  PAssert(depth > 1, PInvalidParameter);
  m_soundChannelBuffers = depth;
}


void OpalPCSSEndPoint::SetSoundChannelBufferTime(unsigned depth)
{
  PAssert(depth >= 20, PInvalidParameter);
  m_soundChannelBufferTime = depth;
}


#if OPAL_VIDEO
bool OpalPCSSEndPoint::CreateVideoInputDevice(const OpalConnection & connection,
                                              const OpalMediaFormat & mediaFormat,
                                              PVideoInputDevice * & device,
                                              bool & autoDelete)
{
  if (connection.GetCall().IsNetworkOriginated() && connection.GetPhase() < OpalConnection::AlertingPhase) {
    const OpalPCSSConnection * pcss = dynamic_cast<const OpalPCSSConnection *>(&connection);
    if (pcss != NULL) {
      PVideoDevice::OpenArgs args = pcss->GetVideoOnRingDevice();
      if (!args.deviceName.IsEmpty()) {
        PTRACE(3, "Using Ring On Video device for " << *this);
        mediaFormat.AdjustVideoArgs(args);
        return m_manager.CreateVideoInputDevice(connection, args, device, autoDelete);
      }
    }
  }

  PSafePtr<OpalConnection> other = connection.GetOtherPartyConnection();
  if (other == NULL)
    return false;

  if (connection.GetPhase() < OpalConnection::ConnectedPhase &&
          other->GetPhase() < OpalConnection::ConnectedPhase)
    return false;

  return OpalLocalEndPoint::CreateVideoInputDevice(connection, mediaFormat, device, autoDelete);
}


bool OpalPCSSEndPoint::SetVideoGrabberDevice(const PVideoDevice::OpenArgs & args)
{
  return GetManager().SetVideoInputDevice(args) && GetManager().SetVideoInputDevice(args, OpalVideoFormat::eMainRole);
}


bool OpalPCSSEndPoint::SetVideoPreviewDevice(const PVideoDevice::OpenArgs & args)
{
  return GetManager().SetVideoPreviewDevice(args) && GetManager().SetVideoPreviewDevice(args, OpalVideoFormat::eMainRole);
}


bool OpalPCSSEndPoint::SetVideoDisplayDevice(const PVideoDevice::OpenArgs & args)
{
  return GetManager().SetVideoOutputDevice(args) && GetManager().SetVideoOutputDevice(args, OpalVideoFormat::eMainRole);
}


bool OpalPCSSEndPoint::SetVideoOnHoldDevice(const PVideoDevice::OpenArgs & args)
{
  return args.Validate<PVideoInputDevice>(m_videoOnHoldDevice);
}


bool OpalPCSSEndPoint::SetVideoOnRingDevice(const PVideoDevice::OpenArgs & args)
{
  return args.Validate<PVideoInputDevice>(m_videoOnRingDevice);
}
#endif // OPAL_VIDEO


/////////////////////////////////////////////////////////////////////////////

OpalPCSSConnection::OpalPCSSConnection(OpalCall & call,
                                       OpalPCSSEndPoint & ep,
                                       const PString & playDevice,
                                       const PString & recordDevice,
                                       unsigned options,
                          OpalConnection::StringOptions * stringOptions)
  : OpalLocalConnection(call, ep, NULL, options, stringOptions, 'P')
  , m_endpoint(ep)
  , m_localRingbackTone(ep.GetLocalRingbackTone())
  , m_soundChannelPlayDevice(playDevice)
  , m_soundChannelRecordDevice(recordDevice)
  , m_soundChannelOnHoldDevice(ep.GetSoundChannelOnHoldDevice())
  , m_soundChannelOnRingDevice(ep.GetSoundChannelOnRingDevice())
  , m_soundChannelBuffers(ep.GetSoundChannelBufferDepth())
  , m_soundChannelBufferTime(ep.GetSoundChannelBufferTime())
#if OPAL_VIDEO
  , m_videoOnHoldDevice(ep.GetVideoOnHoldDevice())
  , m_videoOnRingDevice(ep.GetVideoOnRingDevice())
#endif
  , m_ringbackThread(NULL)
  , m_userInputThread(NULL)
  , m_userInputChannel(NULL)
  , m_userInputAutoDelete(false)
{
  m_silenceDetector = new OpalPCM16SilenceDetector(m_endpoint.GetManager().GetSilenceDetectParams());
  PTRACE_CONTEXT_ID_TO(m_silenceDetector);

#if OPAL_AEC
  m_echoCanceler = new OpalEchoCanceler;
  PTRACE_CONTEXT_ID_TO(m_echoCanceler);
#endif

  PTRACE(4, "Created PC sound system connection: token=\"" << m_callToken << "\" "
            "player=\"" << playDevice << "\" recorder=\"" << recordDevice << '"');
}


OpalPCSSConnection::~OpalPCSSConnection()
{
  PTRACE(4, "Deleted PC sound system connection.");
}


void OpalPCSSConnection::OnReleased()
{
  m_ringbackStop.Signal();
  PThread::WaitAndDelete(m_ringbackThread);

  StopReadUserInput();

  OpalLocalConnection::OnReleased();
}


void OpalPCSSConnection::OnApplyStringOptions()
{
  PString device = m_stringOptions("PCSS-Player", m_soundChannelPlayDevice);
  if (m_soundChannelPlayDevice != device) {
    m_soundChannelPlayDevice = device;
    ChangeSoundChannel(m_soundChannelPlayDevice, false);
  }

  device = m_stringOptions("PCSS-Recorder", m_soundChannelRecordDevice);
  if (m_soundChannelRecordDevice != device) {
    m_soundChannelRecordDevice = device;
    ChangeSoundChannel(m_soundChannelRecordDevice, true);
  }

  OpalLocalConnection::OnApplyStringOptions();
}


PBoolean OpalPCSSConnection::SetAlerting(const PString & calleeName, PBoolean withMedia)
{
  if (!OpalLocalConnection::SetAlerting(calleeName, withMedia))
    return false;

  if (!m_localRingbackTone.IsEmpty() && m_ringbackThread == NULL && GetMediaStream(OpalMediaType::Audio(), false) == NULL)
    m_ringbackThread = new PThreadObj<OpalPCSSConnection>(*this, &OpalPCSSConnection::RingbackMain, false, "Ringback");

  return true;
}


void OpalPCSSConnection::OnStartMediaPatch(OpalMediaPatch & patch)
{
  if (&patch.GetSource().GetConnection() != this) {
    PTRACE(4, "Stopping ringback due to starting media");
    m_ringbackStop.Signal();
  }

  OpalLocalConnection::OnStartMediaPatch(patch);
}


void OpalPCSSConnection::RingbackMain()
{
  PTRACE(4, "Started ringback thread");

  PSoundChannel * channel = CreateSoundChannel(OpalPCM16, 0, false);

  if (PFile::Exists(m_localRingbackTone)) {
    do {
      if (channel->HasPlayCompleted())
        channel->PlayFile(m_localRingbackTone, false);
    } while (!m_ringbackStop.Wait(200));
  }
  else {
    PTones tone(m_localRingbackTone);
    PTimeInterval delay;
    do {
      if (channel->HasPlayCompleted()) {
        tone.Write(*channel);
        delay = tone.GetSize()*1000/tone.GetSampleRate()/2;
      }
      else
        delay = 10;
    } while (!m_ringbackStop.Wait(delay));
  }

  delete channel;

  PTRACE(4, "Ended ringback thread");
}


bool OpalPCSSConnection::TransferConnection(const PString & remoteParty)
{
  PString deviceNames = remoteParty;
  m_stringOptions.ExtractFromString(deviceNames);

  PString playDevice = m_endpoint.GetSoundChannelPlayDevice();
  PString recordDevice = m_endpoint.GetSoundChannelRecordDevice();
  if (!SetDeviceNames(deviceNames, playDevice, recordDevice PTRACE_PARAM(, "transfer")))
    return false;

  OnApplyStringOptions();

  PTRACE(3, "Transfer to sound devices: " "play=\"" << playDevice << "\", " "record=\"" << recordDevice << '"');

  m_soundChannelPlayDevice = playDevice;
  m_soundChannelRecordDevice = recordDevice;

  // Now we be really sneaky and just change the sound devices in the OpalAudioMediaStream
  bool ok = ChangeSoundChannel(playDevice, false);
  return ChangeSoundChannel(recordDevice, true) || ok;
}


void OpalPCSSConnection::OnHold(bool fromRemote, bool onHold)
{
  if (!fromRemote) {
    if (!onHold)
      ChangeSoundChannel(m_soundChannelRecordDevice, true);
    else if (!m_soundChannelOnHoldDevice.IsEmpty() && m_soundChannelOnHoldDevice != P_NULL_AUDIO_DEVICE)
      ChangeSoundChannel(m_soundChannelOnHoldDevice, true);
    else {
      OpalMediaStreamPtr stream = GetMediaStream(OpalMediaType::Audio(), true);
      if (stream != NULL)
        stream->InternalSetPaused(true, false, false);
    }

#if OPAL_VIDEO
    ChangeVideoInputDevice(onHold ? m_videoOnHoldDevice : m_endpoint.GetManager().GetVideoInputDevice());
#endif // OPAL_VIDEO
  }

  OpalLocalConnection::OnHold(fromRemote, onHold);
}


OpalMediaStream * OpalPCSSConnection::CreateMediaStream(const OpalMediaFormat & mediaFormat,
                                                        unsigned sessionID,
                                                        PBoolean isSource)
{
  if (mediaFormat.GetMediaType() != OpalMediaType::Audio() || m_endpoint.UseCallback(mediaFormat, isSource))
    return OpalLocalConnection::CreateMediaStream(mediaFormat, sessionID, isSource);

  PSoundChannel * soundChannel = CreateSoundChannel(mediaFormat, sessionID, isSource);
  if (soundChannel == NULL)
    return NULL;

  PTRACE_CONTEXT_ID_TO(soundChannel);
  return new OpalAudioMediaStream(*this, mediaFormat, sessionID, isSource, m_soundChannelBuffers, m_soundChannelBufferTime, soundChannel);
}


PBoolean OpalPCSSConnection::SetAudioVolume(PBoolean source, unsigned percentage)
{
  bool oneOK = false;
  for (StreamDict::iterator it = m_mediaStreams.begin(); it != m_mediaStreams.end(); ++it) {
    OpalAudioMediaStream * stream = dynamic_cast<OpalAudioMediaStream *>(&*it->second);
    if (stream != NULL && stream->IsSource() == source) {
      PSoundChannel * channel = dynamic_cast<PSoundChannel *>(stream->GetChannel());
      if (channel != NULL && channel->SetVolume(percentage))
        oneOK = true;
    }
  }
  return oneOK;
}

PBoolean OpalPCSSConnection::GetAudioVolume(PBoolean source, unsigned & percentage)
{
  PSafePtr<OpalAudioMediaStream> stream = PSafePtrCast<OpalMediaStream, OpalAudioMediaStream>(GetMediaStream(OpalMediaType::Audio(), source));
  if (stream == NULL)
    return false;

  PSoundChannel * channel = dynamic_cast<PSoundChannel *>(stream->GetChannel());
  if (channel == NULL)
    return false;

  return channel->GetVolume(percentage);
}


bool OpalPCSSConnection::SetAudioMute(bool source, bool mute)
{
  bool oneOK = false;
  for (StreamDict::iterator it = m_mediaStreams.begin(); it != m_mediaStreams.end(); ++it) {
    OpalAudioMediaStream * stream = dynamic_cast<OpalAudioMediaStream *>(&*it->second);
    if (stream != NULL && stream->IsSource() == source) {
      PSoundChannel * channel = dynamic_cast<PSoundChannel *>(stream->GetChannel());
      if (channel != NULL && channel->SetMute(mute))
        oneOK = true;
    }
  }
  return oneOK;
}


bool OpalPCSSConnection::GetAudioMute(bool source, bool & mute)
{
  PSafePtr<OpalAudioMediaStream> stream = PSafePtrCast<OpalMediaStream, OpalAudioMediaStream>(GetMediaStream(OpalMediaType::Audio(), source));
  if (stream == NULL)
    return false;

  PSoundChannel * channel = dynamic_cast<PSoundChannel *>(stream->GetChannel());
  if (channel == NULL)
    return false;

  return channel->GetMute(mute);
}


unsigned OpalPCSSConnection::GetAudioSignalLevel(PBoolean source)
{
  PSafePtr<OpalAudioMediaStream> stream = PSafePtrCast<OpalMediaStream, OpalAudioMediaStream>(GetMediaStream(OpalMediaType::Audio(), source));
  if (stream == NULL)
    return UINT_MAX;

  return stream->GetAverageSignalLevel();
}


PSoundChannel * OpalPCSSConnection::CreateSoundChannel(const OpalMediaFormat & mediaFormat, unsigned sessionID, bool isSource)
{
  return m_endpoint.CreateSoundChannel(*this, mediaFormat, sessionID, isSource);
}


bool OpalPCSSConnection::ChangeSoundChannel(const PString & device, bool isSource, unsigned sessionID)
{
  if (sessionID != 0)
    return ChangeSoundChannel(device, PSafePtrCast<OpalMediaStream, OpalAudioMediaStream>(GetMediaStream(sessionID, isSource)));

  bool allOK = true;
  for (StreamDict::iterator it = m_mediaStreams.begin(); it != m_mediaStreams.end(); ++it) {
    OpalAudioMediaStream * stream = dynamic_cast<OpalAudioMediaStream *>(&*it->second);
    if (stream != NULL && stream->IsSource() == isSource && !ChangeSoundChannel(device, stream))
      allOK = false;
  }
  return allOK;
}


bool OpalPCSSConnection::ChangeSoundChannel(const PString & device, OpalAudioMediaStream * stream)
{
  if (stream == NULL) {
    PTRACE(3, "No audio stream for change of sound channel to " << device);
    return false;
  }

  PSoundChannel * channel = dynamic_cast<PSoundChannel *>(stream->GetChannel());
  if (channel != NULL && channel->GetName() == device) {
    PTRACE(4, "No change of sound channel required to " << device);
    return true;
  }

  if (stream->SetChannel(m_endpoint.CreateSoundChannel(*this, stream->GetMediaFormat(), stream->GetSessionID(), stream->IsSource(), device)))
    return true;

  PTRACE(2, "Could not change sound channel to " << device);
  return false;
}


bool OpalPCSSConnection::StartReadUserInput(PChannel * channel, bool autoDelete)
{
  PSafeLockReadWrite lock(*this);
  if (!lock.IsLocked())
    return false;

  if (m_userInputThread != NULL) {
    PTRACE(2, "Cannot start user input thread as already started.");
    return channel == m_userInputChannel;
  }

  if (!PAssert(channel != NULL, PInvalidParameter))
    return false;

  m_userInputChannel = channel;
  m_userInputAutoDelete = autoDelete;
  m_userInputThread = new PThreadObj<OpalPCSSConnection>(*this, &OpalPCSSConnection::UserInputMain, false, "PCSS-UI");
  PTRACE(3, "Starting user input thread.");
  return true;
}


void OpalPCSSConnection::StopReadUserInput()
{
  PSafeLockReadWrite lock(*this);
  if (!lock.IsLocked())
    return;

  if (m_userInputThread == NULL)
    return;

  PTRACE(3, "Stopping user input thread.");
  m_userInputChannel->Close();
  PThread::WaitAndDelete(m_userInputThread);
  if (m_userInputAutoDelete)
    delete m_userInputChannel;
  m_userInputChannel = NULL;
}


void OpalPCSSConnection::UserInputMain()
{
  SafeReference();

  PTRACE(4, "Started user input thread.");
  int input;
  while ((input = m_userInputChannel->ReadChar()) >= 0) {
    char c = (char)toupper(input);
    switch (c) {
      case '0' :
      case '1' :
      case '2' :
      case '3' :
      case '4' :
      case '5' :
      case '6' :
      case '7' :
      case '8' :
      case '9' :
      case 'A' :
      case 'a' :
      case 'B' :
      case 'b' :
      case 'C' :
      case 'c' :
      case 'D' :
      case 'd' :
      case '!' :
        PTRACE(4, "Emulating user input '" << c << '\'');
        OnUserInputTone(c, 100);
        break;

      case 'H' :
      case 'h' :
        PTRACE(4, "Releasing connection due to hang up from user input.");
        Release();
    }
  }
  PTRACE(4, "Stopped user input thread.");

  SafeDereference();
}


void OpalPCSSConnection::AlertingIncoming(bool withMedia)
{
  OpalLocalConnection::AlertingIncoming(withMedia || !m_soundChannelOnRingDevice.IsEmpty());
}


void OpalPCSSConnection::AcceptIncoming()
{
  if (!m_soundChannelOnRingDevice.IsEmpty()) {
    ChangeSoundChannel(m_soundChannelRecordDevice, true);
#if OPAL_VIDEO
    ChangeVideoInputDevice(m_endpoint.GetManager().GetVideoInputDevice());
#endif // OPAL_VIDEO
  }

  OpalLocalConnection::AcceptIncoming();
}
#endif // OPAL_HAS_PCSS
