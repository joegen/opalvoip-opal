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
 * $Log: pcss.cxx,v $
 * Revision 1.2018  2004/05/24 13:52:38  rjongbloed
 * Added a separate structure for the silence suppression paramters to make
 *   it easier to set from global defaults in the manager class.
 *
 * Revision 2.16  2004/05/17 13:24:18  rjongbloed
 * Added silence suppression.
 *
 * Revision 2.15  2004/04/18 13:35:28  rjongbloed
 * Fixed ability to make calls where both endpoints are specified a priori. In particular
 *   fixing the VXML support for an outgoing sip/h323 call.
 *
 * Revision 2.14  2004/02/03 12:21:30  rjongbloed
 * Fixed random destination alias from being interpreted as a sound card device name, unless it is a valid sound card device name.
 *
 * Revision 2.13  2003/03/17 22:27:36  robertj
 * Fixed multi-byte character that should have been string.
 *
 * Revision 2.12  2003/03/17 10:13:18  robertj
 * Added call back functions for creating sound channel.
 * Added video support.
 *
 * Revision 2.11  2003/03/07 08:14:50  robertj
 * Fixed support of dual address SetUpCall()
 * Fixed use of new call set up sequence, calling OnSetUp() in outgoing call.
 *
 * Revision 2.10  2003/03/06 03:57:47  robertj
 * IVR support (work in progress) requiring large changes everywhere.
 *
 * Revision 2.9  2002/07/04 07:54:53  robertj
 * Fixed GNU warning
 *
 * Revision 2.8  2002/06/16 02:19:55  robertj
 * Fixed and clarified function for initiating call, thanks Ted Szoczei
 *
 * Revision 2.7  2002/01/22 05:13:47  robertj
 * Revamp of user input API triggered by RFC2833 support
 *
 * Revision 2.6  2001/11/13 06:25:56  robertj
 * Changed SetUpConnection() so returns BOOL as returning
 *   pointer to connection is not useful.
 *
 * Revision 2.5  2001/10/15 04:33:17  robertj
 * Removed answerCall signal and replaced with state based functions.
 *
 * Revision 2.4  2001/08/23 03:15:51  robertj
 * Added missing Lock() calls in SetUpConnection
 *
 * Revision 2.3  2001/08/21 01:13:22  robertj
 * Fixed propagation of sound channel buffers through media stream.
 * Allowed setting of sound card by number using #1 etc.
 * Fixed calling OnAlerting when alerting.
 *
 * Revision 2.2  2001/08/17 08:33:50  robertj
 * More implementation.
 *
 * Revision 2.1  2001/08/01 05:45:01  robertj
 * Moved media formats list from endpoint to connection.
 *
 * Revision 2.0  2001/07/27 15:48:25  robertj
 * Conversion of OpenH323 to Open Phone Abstraction Library (OPAL)
 *
 * Revision 1.1  2000/02/19 15:34:52  robertj
 * Major upgrade to OPAL (Open Phone Abstraction Library). HUGE CHANGES!!!!
 *
 */

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "pcss.h"
#endif

#include <opal/pcss.h>

#include <ptlib/videoio.h>
#include <codec/silencedetect.h>
#include <codec/vidcodec.h>
#include <opal/call.h>
#include <opal/patch.h>
#include <opal/manager.h>


#define new PNEW


static PString MakeToken(const PString & playDevice, const PString & recordDevice)
{
  if (playDevice == recordDevice)
    return recordDevice;
  else
    return playDevice + '\\' + recordDevice;
}


/////////////////////////////////////////////////////////////////////////////

OpalPCSSEndPoint::OpalPCSSEndPoint(OpalManager & mgr, const char * prefix)
  : OpalEndPoint(mgr, prefix, CanTerminateCall),
    soundChannelPlayDevice(PSoundChannel::GetDefaultDevice(PSoundChannel::Player)),
    soundChannelRecordDevice(PSoundChannel::GetDefaultDevice(PSoundChannel::Recorder))
{
#ifdef _WIN32
  // Windows MultMedia stuff seems to need greater depth due to enormous
  // latencies in its operation, need to use DirectSound maybe?
  soundChannelBuffers = 3;
#else
  // Should only need double buffering for Unix platforms
  soundChannelBuffers = 2;
#endif

  PTRACE(3, "PCSS\tCreated PC sound system endpoint.");
}


OpalPCSSEndPoint::~OpalPCSSEndPoint()
{
  PTRACE(3, "PCSS\tDeleted PC sound system endpoint.");
}


static BOOL SetDeviceName(const PString & name,
                          PSoundChannel::Directions dir,
                          PString & result)
{
  PStringArray devices = PSoundChannel::GetDeviceNames(dir);

  if (name[0] == '#') {
    PINDEX id = name.Mid(1).AsUnsigned();
    if (id == 0 || id > devices.GetSize())
      return FALSE;
    result = devices[id-1];
  }
  else {
    if (devices.GetValuesIndex(name) == P_MAX_INDEX)
      return FALSE;
    result = name;
  }

  return TRUE;
}


BOOL OpalPCSSEndPoint::MakeConnection(OpalCall & call,
                                      const PString & remoteParty,
                                      void * userData)
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

  OpalPCSSConnection * connection;
  {
    PWaitAndSignal mutex(inUseFlag);

    if (connectionsActive.Contains(MakeToken(playDevice, recordDevice)))
      return FALSE;

    connection = CreateConnection(call, playDevice, recordDevice, userData);
    if (connection == NULL)
      return FALSE;

    connectionsActive.SetAt(connection->GetToken(), connection);
  }

  // If we are the A-party then need to initiate a call now in this thread and
  // go through the routing engine via OnIncomingConnection. If we are the
  // B-Party then SetUpConnection() gets called in the context of the A-party
  // thread.
  if (&call.GetConnection(0) == connection)
    connection->InitiateCall();

  return TRUE;
}


OpalPCSSConnection * OpalPCSSEndPoint::CreateConnection(OpalCall & call,
                                                        const PString & playDevice,
                                                        const PString & recordDevice,
                                                        void * /*userData*/)
{
  return new OpalPCSSConnection(call, *this, playDevice, recordDevice);
}


PSoundChannel * OpalPCSSEndPoint::CreateSoundChannel(const OpalPCSSConnection & connection,
                                                     BOOL isSource)
{
  PString deviceName;
  if (isSource)
    deviceName = connection.GetSoundChannelRecordDevice();
  else
    deviceName = connection.GetSoundChannelPlayDevice();

  PSoundChannel * soundChannel = new PSoundChannel;

  if (soundChannel->Open(deviceName,
                         isSource ? PSoundChannel::Recorder
                                  : PSoundChannel::Player,
                         1, 8000, 16)) {
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


void OpalPCSSEndPoint::AcceptIncomingConnection(const PString & token)
{
  OpalPCSSConnection * connection = (OpalPCSSConnection *)GetConnectionWithLock(token);
  if (connection != NULL) {
    connection->AcceptIncoming();
    connection->Unlock();
  }
}


BOOL OpalPCSSEndPoint::OnShowUserInput(const OpalPCSSConnection &, const PString &)
{
  return TRUE;
}


BOOL OpalPCSSEndPoint::SetSoundChannelPlayDevice(const PString & name)
{
  return SetDeviceName(name, PSoundChannel::Player, soundChannelPlayDevice);
}


BOOL OpalPCSSEndPoint::SetSoundChannelRecordDevice(const PString & name)
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
                                       const PString & recordDevice)
  : OpalConnection(call, ep, MakeToken(playDevice, recordDevice)),
    endpoint(ep),
    soundChannelPlayDevice(playDevice),
    soundChannelRecordDevice(recordDevice),
    soundChannelBuffers(ep.GetSoundChannelBufferDepth())
{
  silenceDetector = new OpalPCM16SilenceDetector;

  PTRACE(3, "PCSS\tCreated PC sound system connection.");
}


OpalPCSSConnection::~OpalPCSSConnection()
{
  PTRACE(3, "PCSS\tDeleted PC sound system connection.");
}


BOOL OpalPCSSConnection::SetUpConnection()
{
  remotePartyName = ownerCall.GetConnection(0).GetRemotePartyName();

  PTRACE(3, "PCSS\tSetUpConnection(" << remotePartyName << ')');
  phase = AlertingPhase;
  endpoint.OnShowIncoming(*this);
  OnAlerting();

  return TRUE;
}


BOOL OpalPCSSConnection::SetAlerting(const PString & calleeName, BOOL)
{
  PTRACE(3, "PCSS\tSetAlerting(" << calleeName << ')');
  phase = AlertingPhase;
  remotePartyName = calleeName;
  return endpoint.OnShowOutgoing(*this);
}


BOOL OpalPCSSConnection::SetConnected()
{
  PTRACE(3, "PCSS\tSetConnected()");
  phase = ConnectedPhase;
  return TRUE;
}


PString OpalPCSSConnection::GetDestinationAddress()
{
  return endpoint.OnGetDestination(*this);
}


OpalMediaFormatList OpalPCSSConnection::GetMediaFormats() const
{
 
  OpalMediaFormatList formats;

  formats += OpalPCM16; // Sound card can only do 16 bit PCM

  AddVideoMediaFormats(formats);

  return formats;
}


OpalMediaStream * OpalPCSSConnection::CreateMediaStream(const OpalMediaFormat & mediaFormat,
                                                        unsigned sessionID,
                                                        BOOL isSource)
{
  if (sessionID != OpalMediaFormat::DefaultAudioSessionID)
    return OpalConnection::CreateMediaStream(mediaFormat, sessionID, isSource);

  PSoundChannel * soundChannel = CreateSoundChannel(isSource);
  if (soundChannel == NULL)
    return NULL;

  return new OpalAudioMediaStream(mediaFormat, sessionID, isSource, soundChannelBuffers, soundChannel);
}


BOOL OpalPCSSConnection::OnOpenMediaStream(OpalMediaStream & mediaStream)
{
  if (!OpalConnection::OnOpenMediaStream(mediaStream))
    return FALSE;

  if (mediaStream.IsSource()) {
    OpalMediaPatch * patch = mediaStream.GetPatch();
    if (patch != NULL) {
      silenceDetector->SetParameters(endpoint.GetManager().GetSilenceDetectParams());
      patch->AddFilter(silenceDetector->GetReceiveHandler(), OpalPCM16);
    }
  }

  return TRUE;
}


PSoundChannel * OpalPCSSConnection::CreateSoundChannel(BOOL isSource)
{
  return endpoint.CreateSoundChannel(*this, isSource);
}


BOOL OpalPCSSConnection::SendUserInputString(const PString & value)
{
  PTRACE(3, "PCSS\tSendUserInputString(" << value << ')');
  return endpoint.OnShowUserInput(*this, value);
}


void OpalPCSSConnection::InitiateCall()
{
  if (!Lock())
    return;

  phase = SetUpPhase;
  if (!OnIncomingConnection())
    Release(EndedByCallerAbort);
  else {
    PTRACE(2, "PCSS\tOutgoing call routed to " << ownerCall.GetPartyB() << " for " << *this);
    if (!ownerCall.OnSetUp(*this))
      Release(EndedByNoAccept);
  }

  Unlock();
}


void OpalPCSSConnection::AcceptIncoming()
{
  if (!Lock())
    return;

  if (phase == AlertingPhase) {
    phase = ConnectedPhase;
    OnConnected();
  }

  Unlock();
}


/////////////////////////////////////////////////////////////////////////////
