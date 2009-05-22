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

#if OPAL_PTLIB_AUDIO

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

#if OPAL_HAS_IM

#include <im/im.h>

class PCSSIMStream : public OpalIMMediaStream
{
  public:
    PCSSIMStream(OpalConnection & conn,
               const OpalMediaFormat & mediaFormat, ///<  Media format for stream
                              unsigned sessionID,   ///<  Session number for stream
                                  bool isSource)     ///<  Is a source stream
      : OpalIMMediaStream(conn, mediaFormat, sessionID, isSource)
    {
    }

    virtual PBoolean ReadData(
      BYTE * /*data*/,      ///<  Data buffer to read to
      PINDEX /*size*/,      ///<  Size of buffer
      PINDEX & /*length*/   ///<  Length of data actually read
    )
    {
      PAssertAlways("Cannot ReadData from OpalSIPIMMediaStream");
      return false;
    }

    /**Write raw media data to the sink media stream.
       The default behaviour writes to the PChannel object.
      */
    virtual PBoolean WritePacket(RTP_DataFrame & packet)
    {
      if (!IsOpen())
        return false;

      OpalConnection::IMInfo info;
      info.sessionId   = sessionID;
      info.mediaFormat = mediaFormat;
      info.body        = T140String(packet.GetPayloadPtr(), packet.GetPayloadSize());

      connection.OnReceiveIM(info);

      return true;
    }

    virtual PBoolean RequiresPatchThread() const   { return !isSource; }

};

#endif  // OPAL_HAS_IM

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

OpalMediaFormatList OpalPCSSEndPoint::GetMediaFormats() const
{
  OpalMediaFormatList list = OpalEndPoint::GetMediaFormats();
  list += OpalL16_STEREO_48KHZ;
  return list;
}

static bool SetDeviceName(const PString & name,
                          PSoundChannel::Directions dir,
                          PString & result)
{
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


static bool SetDeviceNames(const PString & remoteParty, PString & playResult, PString & recordResult, const char * PTRACE_PARAM(operation))
{
  PINDEX prefixLength = remoteParty.Find(':')+1;

  PString playDevice, recordDevice;
  PINDEX separator = remoteParty.FindOneOf("\n\\", prefixLength);
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

  if (recordDevice.IsEmpty() || recordDevice == "*")
    recordDevice = recordResult;
  if (!SetDeviceName(recordDevice, PSoundChannel::Recorder, recordResult)) {
    PTRACE(2, "PCSS\tSound recording device \"" << recordDevice << "\" does not exist, " << operation << " aborted.");
    return false;
  }

  return true;
}


PBoolean OpalPCSSEndPoint::MakeConnection(OpalCall & call,
                                      const PString & remoteParty,
                                      void * userData,
                               unsigned int options,
                               OpalConnection::StringOptions * stringOptions)
{
  PString playDevice = soundChannelPlayDevice;
  PString recordDevice = soundChannelRecordDevice;
  if (!SetDeviceNames(remoteParty, playDevice, recordDevice, "call")) {
    call.Clear(OpalConnection::EndedByLocalBusy);
    return false;
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

  unsigned channels = mediaFormat.GetOptionInteger(OpalAudioFormat::ChannelsOption());
  unsigned clockRate = mediaFormat.GetClockRate();

  if (soundChannel->Open(deviceName, dir, channels, clockRate, 16)) {
    PTRACE(3, "PCSS\tOpened " 
               << ((channels == 1) ? "mono" : ((channels == 2) ? "stereo" : "multi-channel")) 
               << " sound channel \"" << deviceName
               << "\" for " << (isSource ? "record" : "play") << "ing at "
               << clockRate/1000 << '.' << (clockRate%1000)/100 << " kHz.");
    return soundChannel;
  }

  PTRACE(1, "PCSS\tCould not open sound channel \"" << deviceName
         << "\" for " << (isSource ? "record" : "play")
         << "ing: " << soundChannel->GetErrorText());

  delete soundChannel;
  return NULL;
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

OpalPCSSConnection::OpalPCSSConnection(OpalCall & call,
                                       OpalPCSSEndPoint & ep,
                                       const PString & playDevice,
                                       const PString & recordDevice,
                                       unsigned options,
                          OpalConnection::StringOptions * stringOptions)
  : OpalConnection(call, ep, ep.GetManager().GetNextCallToken(), options, stringOptions),
    endpoint(ep),
    soundChannelPlayDevice(playDevice),
    soundChannelRecordDevice(recordDevice),
    soundChannelBuffers(ep.GetSoundChannelBufferDepth())
{
  silenceDetector = new OpalPCM16SilenceDetector(endpoint.GetManager().GetSilenceDetectParams());
  echoCanceler = new OpalEchoCanceler;

  PTRACE(4, "PCSS\tCreated PC sound system connection: token=\"" << callToken << "\" "
            "player=\"" << playDevice << "\" recorder=\"" << recordDevice << '"');
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
    SetPhase(SetUpPhase);
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

  PTRACE(3, "PCSS\tSetUpConnection(" << remotePartyName << ')');
  SetPhase(AlertingPhase);
  OnAlerting();

  return endpoint.OnShowIncoming(*this);
}


PBoolean OpalPCSSConnection::SetAlerting(const PString & calleeName, PBoolean)
{
  PTRACE(3, "PCSS\tSetAlerting(" << calleeName << ')');
  SetPhase(AlertingPhase);
  remotePartyName = calleeName;
  return endpoint.OnShowOutgoing(*this);
}


bool OpalPCSSConnection::TransferConnection(const PString & remoteParty)
{
  PString playDevice = soundChannelPlayDevice;
  PString recordDevice = soundChannelRecordDevice;
  if (!SetDeviceNames(remoteParty, playDevice, recordDevice, "transfer"))
    return false;

  if ((playDevice *= soundChannelPlayDevice) &&
      (recordDevice *= soundChannelRecordDevice)) {
    PTRACE(2, "PCSS\tTransfer to same sound devices, ignoring.");
    return true;
  }

  soundChannelPlayDevice = playDevice;
  soundChannelRecordDevice = recordDevice;

  PTRACE(3, "PCSS\tTransfer to sound devices: play=\"" << playDevice << "\", record=\"" << recordDevice << '"');

  // Now we be really sneaky and just change the sound devices in the OpalAudioMediaStream
  for (OpalMediaStreamPtr mediaStream = mediaStreams; mediaStream != NULL; ++mediaStream) {
    OpalRawMediaStream * rawStream = dynamic_cast<OpalRawMediaStream *>(&*mediaStream);
    if (rawStream != NULL)
      rawStream->SetChannel(CreateSoundChannel(rawStream->GetMediaFormat(), rawStream->IsSource()));
  }
  return true;
}


OpalMediaStream * OpalPCSSConnection::CreateMediaStream(const OpalMediaFormat & mediaFormat,
                                                        unsigned sessionID,
                                                        PBoolean isSource)
{
  if (mediaFormat.GetMediaType() == OpalMediaType::Audio()) {
    PSoundChannel * soundChannel = CreateSoundChannel(mediaFormat, isSource);
    if (soundChannel == NULL)
      return NULL;

    return new OpalAudioMediaStream(*this, mediaFormat, sessionID, isSource, soundChannelBuffers, soundChannel);
  }

#if OPAL_HAS_IM
  if (mediaFormat.GetMediaType() == "msrp" || mediaFormat.GetMediaType() == "sip-im" || mediaFormat.GetMediaType() == "t140") {
    return new PCSSIMStream(*this, mediaFormat, sessionID, isSource);
  }
#endif

  return OpalConnection::CreateMediaStream(mediaFormat, sessionID, isSource);
}


void OpalPCSSConnection::OnPatchMediaStream(PBoolean isSource,
					    OpalMediaPatch & patch)
{
  endpoint.OnPatchMediaStream(*this, isSource, patch);

  int clockRate;
  if (patch.GetSource().GetMediaFormat().GetMediaType() == OpalMediaType::Audio()) {
    PTRACE(3, "PCSS\tAdding filters to patch");
    if (isSource)
      patch.AddFilter(silenceDetector->GetReceiveHandler(), OpalPCM16);
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
       mediaFormat.GetMediaType() == OpalMediaType::Video() &&
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
  PSafePtr<OpalAudioMediaStream> stream = PSafePtrCast<OpalMediaStream, OpalAudioMediaStream>(GetMediaStream(OpalMediaType::Audio(), source));
  if (stream == NULL)
    return PFalse;

  PSoundChannel * channel = dynamic_cast<PSoundChannel *>(stream->GetChannel());
  if (channel == NULL)
    return PFalse;

  return channel->SetVolume(percentage);
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
  return endpoint.CreateSoundChannel(*this, mediaFormat, isSource);
}


PBoolean OpalPCSSConnection::SendUserInputString(const PString & value)
{
  PTRACE(3, "PCSS\tSendUserInputString(" << value << ')');
  return endpoint.OnShowUserInput(*this, value);
}


void OpalPCSSConnection::AcceptIncoming()
{
  if (LockReadWrite()) {
    OnConnectedInternal();
    UnlockReadWrite();
  }
}

#endif // OPAL_PTLIB_AUDIO


/////////////////////////////////////////////////////////////////////////////
