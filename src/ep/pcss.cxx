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

#include <opal_config.h>

#include <ep/pcss.h>

#if OPAL_HAS_PCSS

#pragma message("PC Sound System support enabled")

#if OPAL_VIDEO
#include <ptlib/videoio.h>
#include <codec/vidcodec.h>
#endif

#include <ptlib/sound.h>
#include <opal/call.h>
#include <opal/manager.h>


#define new PNEW

/////////////////////////////////////////////////////////////////////////////

OpalPCSSEndPoint::OpalPCSSEndPoint(OpalManager & mgr, const char * prefix)
  : OpalLocalEndPoint(mgr, prefix, false)
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
  PTRACE(3, "PCSS\tCreated PC sound system endpoint.\n" << setfill('\n')
         << "Player=" << m_soundChannelPlayDevice << ", available devices:\n"
         << PSoundChannel::GetDeviceNames(PSoundChannel::Player)
         << "Recorders=" << m_soundChannelRecordDevice << ", available devices:\n"
         << PSoundChannel::GetDeviceNames(PSoundChannel::Recorder));

#if OPAL_VIDEO
  m_videoOnHoldDevice.deviceName = P_FAKE_VIDEO_TEXT;
#endif
}


OpalPCSSEndPoint::~OpalPCSSEndPoint()
{
  PTRACE(4, "PCSS\tDeleted PC sound system endpoint.");
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
    PTRACE(2, "PCSS\tSound player device \"" << playDevice << "\" does not exist, " << operation << " aborted.");
    return false;
  }
  PTRACE(4, "PCSS\tSound player device is \"" << playDevice << '"');

  if (recordDevice.IsEmpty() || recordDevice == "*")
    recordDevice = recordResult;
  if (!SetDeviceName(recordDevice, PSoundChannel::Recorder, recordResult)) {
    PTRACE(2, "PCSS\tSound recording device \"" << recordDevice << "\" does not exist, " << operation << " aborted.");
    return false;
  }
  PTRACE(4, "PCSS\tSound recording device is \"" << recordDevice << '"');

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
                                                                     PBoolean   isSource)
{
  PSoundChannel::Params params;
  if (isSource) {
    if (connection.GetCall().IsOnHold())
      params.m_device = connection.GetSoundChannelOnHoldDevice();
    else
      params.m_device = connection.GetSoundChannelRecordDevice();
    params.m_direction = PSoundChannel::Recorder;
  }
  else {
    params.m_device = connection.GetSoundChannelPlayDevice();
    params.m_direction = PSoundChannel::Player;
  }

  params.m_channels = mediaFormat.GetOptionInteger(OpalAudioFormat::ChannelsOption());
  params.m_sampleRate = mediaFormat.GetClockRate();

  PSoundChannel * soundChannel = PSoundChannel::CreateOpenedChannel(params);
  if (soundChannel == NULL)
    return NULL;

  PTRACE_CONTEXT_ID_SET(*soundChannel, connection);

  PTRACE(3, "PCSS\tOpened "
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
  return OnShowIncoming(dynamic_cast<const OpalPCSSConnection &>(connection));
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
  return OpalPCSSEndPoint::OnIncomingCall(const_cast<OpalPCSSConnection &>(connection));
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


PBoolean OpalPCSSEndPoint::SetSoundChannelPlayDevice(const PString & name)
{
  return SetDeviceName(name, PSoundChannel::Player, m_soundChannelPlayDevice);
}


PBoolean OpalPCSSEndPoint::SetSoundChannelRecordDevice(const PString & name)
{
  return SetDeviceName(name, PSoundChannel::Recorder, m_soundChannelRecordDevice);
}


PBoolean OpalPCSSEndPoint::SetSoundChannelOnHoldDevice(const PString & name)
{
  return SetDeviceName(name, PSoundChannel::Recorder, m_soundChannelOnHoldDevice);
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
PBoolean OpalPCSSEndPoint::SetVideoOnHoldDevice(const PVideoDevice::OpenArgs & args)
{
  return args.Validate<PVideoInputDevice>(m_videoOnHoldDevice);
}
#endif


/////////////////////////////////////////////////////////////////////////////

OpalPCSSConnection::OpalPCSSConnection(OpalCall & call,
                                       OpalPCSSEndPoint & ep,
                                       const PString & playDevice,
                                       const PString & recordDevice,
                                       unsigned options,
                          OpalConnection::StringOptions * stringOptions)
  : OpalLocalConnection(call, ep, NULL, options, stringOptions, 'P')
  , m_endpoint(ep)
  , m_soundChannelPlayDevice(playDevice)
  , m_soundChannelRecordDevice(recordDevice)
  , m_soundChannelOnHoldDevice(ep.GetSoundChannelOnHoldDevice())
  , m_soundChannelBuffers(ep.GetSoundChannelBufferDepth())
  , m_soundChannelBufferTime(ep.GetSoundChannelBufferTime())
#if OPAL_VIDEO
  , m_videoOnHoldDevice(ep.GetVideoOnHoldDevice())
#endif
{
  silenceDetector = new OpalPCM16SilenceDetector(endpoint.GetManager().GetSilenceDetectParams());
  PTRACE_CONTEXT_ID_TO(silenceDetector);

#if OPAL_AEC
  echoCanceler = new OpalEchoCanceler;
  PTRACE_CONTEXT_ID_TO(echoCanceler);
#endif

  PTRACE(4, "PCSS\tCreated PC sound system connection: token=\"" << callToken << "\" "
            "player=\"" << playDevice << "\" recorder=\"" << recordDevice << '"');
}


OpalPCSSConnection::~OpalPCSSConnection()
{
  PTRACE(4, "PCSS\tDeleted PC sound system connection.");
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

  bool samePlayer = playDevice *= m_soundChannelPlayDevice;
  bool sameRecorder = recordDevice *= m_soundChannelRecordDevice;

  PTRACE(3, "PCSS\tTransfer to sound devices: " "play=\"" << playDevice << "\", " "record=\"" << recordDevice << '"');

  m_soundChannelPlayDevice = playDevice;
  m_soundChannelRecordDevice = recordDevice;

  // Now we be really sneaky and just change the sound devices in the OpalAudioMediaStream
  for (OpalMediaStreamPtr mediaStream = mediaStreams; mediaStream != NULL; ++mediaStream) {
    OpalRawMediaStream * rawStream = dynamic_cast<OpalRawMediaStream *>(&*mediaStream);
    if (rawStream != NULL && !(mediaStream->IsSource() ? sameRecorder : samePlayer))
      rawStream->SetChannel(CreateSoundChannel(rawStream->GetMediaFormat(), rawStream->IsSource()));
  }

  return true;
}


void OpalPCSSConnection::OnHold(bool fromRemote, bool onHold)
{
  if (!fromRemote) {
    OpalMediaStreamPtr mediaStream = GetMediaStream(OpalMediaType::Audio(), true);
    if (mediaStream != NULL) {
      OpalRawMediaStream * rawStream = dynamic_cast<OpalRawMediaStream *>(&*mediaStream);
      if (rawStream != NULL)
        rawStream->SetChannel(CreateSoundChannel(rawStream->GetMediaFormat(), rawStream->IsSource()));
    }

#if OPAL_VIDEO
    PVideoDevice::OpenArgs args = onHold ? m_videoOnHoldDevice : m_endpoint.GetManager().GetVideoInputDevice();
    if (!args.deviceName.IsEmpty())
      ChangeVideoInputDevice(args);
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

  PSoundChannel * soundChannel = CreateSoundChannel(mediaFormat, isSource);
  if (soundChannel == NULL)
    return NULL;

  PTRACE_CONTEXT_ID_TO(soundChannel);
  return new OpalAudioMediaStream(*this, mediaFormat, sessionID, isSource, m_soundChannelBuffers, m_soundChannelBufferTime, soundChannel);
}


PBoolean OpalPCSSConnection::SetAudioVolume(PBoolean source, unsigned percentage)
{
  PSafePtr<OpalAudioMediaStream> stream = PSafePtrCast<OpalMediaStream, OpalAudioMediaStream>(GetMediaStream(OpalMediaType::Audio(), source));
  if (stream == NULL)
    return false;

  PSoundChannel * channel = dynamic_cast<PSoundChannel *>(stream->GetChannel());
  if (channel == NULL)
    return false;

  return channel->SetVolume(percentage);
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
  PSafePtr<OpalAudioMediaStream> stream = PSafePtrCast<OpalMediaStream, OpalAudioMediaStream>(GetMediaStream(OpalMediaType::Audio(), source));
  if (stream == NULL)
    return false;

  PSoundChannel * channel = dynamic_cast<PSoundChannel *>(stream->GetChannel());
  if (channel == NULL)
    return false;

  return channel->SetMute(mute);
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


PSoundChannel * OpalPCSSConnection::CreateSoundChannel(const OpalMediaFormat & mediaFormat, PBoolean isSource)
{
  return m_endpoint.CreateSoundChannel(*this, mediaFormat, isSource);
}


#else
  #pragma message("PC Sound System support DISABLED")
#endif // OPAL_HAS_PCSS


/////////////////////////////////////////////////////////////////////////////
