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
 * Revision 1.2005  2001/08/23 03:15:51  robertj
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

#include <opal/call.h>


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


OpalConnection * OpalPCSSEndPoint::SetUpConnection(OpalCall & call,
                                                   const PString & remoteParty,
                                                   void * userData)
{
  // First strip of the prefix if present
  PINDEX prefixLength = 0;
  if (remoteParty.Find(GetPrefixName()+":") == 0)
    prefixLength = GetPrefixName().GetLength()+1;

  PString playDevice;
  PString recordDevice;
  PINDEX separator = remoteParty.Find('\\', prefixLength);
  if (separator == P_MAX_INDEX)
    playDevice = recordDevice = remoteParty.Mid(prefixLength);
  else {
    playDevice = remoteParty(prefixLength+1, separator-1);
    recordDevice = remoteParty.Mid(separator+1);
  }

  if (playDevice == "*")
    playDevice = soundChannelPlayDevice;
  if (recordDevice == "*")
    recordDevice = soundChannelRecordDevice;

  OpalPCSSConnection * connection;
  {
    PWaitAndSignal mutex(inUseFlag);

    if (connectionsActive.Contains(MakeToken(playDevice, recordDevice)))
      return NULL;

    connection = CreateConnection(call, playDevice, recordDevice, userData);
    if (connection == NULL)
      return NULL;

    connectionsActive.SetAt(connection->GetToken(), connection);
  }

  connection->Lock();

  // See if we are initiating or answering call.
  OpalConnection & caller = call.GetConnection(0);
  if (connection == &caller)
    connection->StartOutgoing();
  else
    connection->StartIncoming(caller.GetRemotePartyName());

  return connection;
}


OpalPCSSConnection * OpalPCSSEndPoint::CreateConnection(OpalCall & call,
                                                        const PString & playDevice,
                                                        const PString & recordDevice,
                                                        void * /*userData*/)
{
  return new OpalPCSSConnection(call, playDevice, recordDevice, *this);
}


void OpalPCSSEndPoint::AcceptIncomingConnection(const PString & token)
{
  OpalPCSSConnection * connection = (OpalPCSSConnection *)GetConnectionWithLock(token);
  if (connection != NULL) {
    connection->AcceptIncoming();
    connection->Unlock();
  }
}


BOOL OpalPCSSEndPoint::OnShowUserIndication(const OpalPCSSConnection &, const PString &)
{
  return TRUE;
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
                                       const PString & playDevice,
                                       const PString & recordDevice,
                                       OpalPCSSEndPoint & ep)
  : OpalConnection(call, ep, MakeToken(playDevice, recordDevice)),
    endpoint(ep),
    soundChannelPlayDevice(playDevice),
    soundChannelRecordDevice(recordDevice)
{
  phase = SetUpPhase;
  PTRACE(3, "PCSS\tCreated PC sound system connection.");
}


OpalPCSSConnection::~OpalPCSSConnection()
{
  PTRACE(3, "PCSS\tDeleted PC sound system connection.");
}


OpalConnection::Phases OpalPCSSConnection::GetPhase() const
{
  return phase;
}


BOOL OpalPCSSConnection::SetAlerting(const PString & calleeName)
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
  // Sound card can only do 16 bit PCM
  OpalMediaFormatList formatNames;
  formatNames += OpalPCM16;
  return formatNames;
}


OpalMediaStream * OpalPCSSConnection::CreateMediaStream(BOOL isSource, unsigned sessionID)
{
  PString deviceName;
  if (isSource)
    deviceName = soundChannelRecordDevice;
  else
    deviceName = soundChannelPlayDevice;

  PSoundChannel * soundChannel = new PSoundChannel;

  if (soundChannel->Open(deviceName,
                         isSource ? PSoundChannel::Recorder
                                  : PSoundChannel::Player,
                         1, 8000, 16)) {
    PTRACE(3, "PCSS\tOpened sound channel \"" << deviceName
           << "\" for " << (isSource ? "record" : "play") << "ing.");
    return new OpalAudioMediaStream(isSource,
                                    sessionID,
                                    endpoint.GetSoundChannelBufferDepth(),
                                    soundChannel);
  }

  PTRACE(1, "PCSS\tCould not open sound channel \"" << deviceName
         << "\" for " << (isSource ? "record" : "play")
         << "ing: " << soundChannel->GetErrorText());

  delete soundChannel;
  return NULL;
}


BOOL OpalPCSSConnection::SendUserIndicationString(const PString & value)
{
  PTRACE(3, "PCSS\tSendUserIndicationString(" << value << ')');
  return endpoint.OnShowUserIndication(*this, value);
}


void OpalPCSSConnection::StartOutgoing()
{
  phase = EstablishedPhase;
  OnEstablished();
}


void OpalPCSSConnection::StartIncoming(const PString & callerName)
{
  PTRACE(3, "PCSS\tStartIncomingCall(" << callerName << ')');
  phase = AlertingPhase;
  remotePartyName = callerName;
  endpoint.OnShowIncoming(*this);
  OnAlerting();
}


void OpalPCSSConnection::AcceptIncoming()
{
  if (phase == AlertingPhase) {
    phase = ConnectedPhase;
    OnConnected();
  }
}


/////////////////////////////////////////////////////////////////////////////
