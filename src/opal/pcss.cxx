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
 * Revision 1.2002  2001/08/01 05:45:01  robertj
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


/////////////////////////////////////////////////////////////////////////////

OpalPCSSEndPoint::OpalPCSSEndPoint(OpalManager & mgr)
  : OpalEndPoint(mgr, "pc", CanTerminateCall),
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
  PINDEX proto = remoteParty.Find("pc:") == 0 ? 5 : 0;

  /* At present we only accept wildcard, eventually this would have the ability
     to specify a "line" which the PC could emulate with separate windows or
     something.
   */
  if (remoteParty.Mid(proto) != "*")
    return NULL;

  OpalPCSSConnection * connection = CreateConnection(call, userData);
  if (connection == NULL)
    return NULL;

  // See if we are initiating or answering call.
  OpalConnection & caller = call.GetConnection(0);
  if (connection == &caller)
    connection->StartOutgoingCall();
  else
    connection->StartIncomingCall(caller.GetRemotePartyName());

  return connection;
}


OpalPCSSConnection * OpalPCSSEndPoint::CreateConnection(OpalCall & call,
                                                        void * /*userData*/)
{
  return new OpalPCSSConnection(call, *this);
}


BOOL OpalPCSSEndPoint::SetSoundChannelPlayDevice(const PString & name)
{
  if (PSoundChannel::GetDeviceNames(PSoundChannel::Player).GetValuesIndex(name) == P_MAX_INDEX)
    return FALSE;

  soundChannelPlayDevice = name;
  return TRUE;
}


BOOL OpalPCSSEndPoint::SetSoundChannelRecordDevice(const PString & name)
{
  if (PSoundChannel::GetDeviceNames(PSoundChannel::Recorder).GetValuesIndex(name) == P_MAX_INDEX)
    return FALSE;

  soundChannelRecordDevice = name;
  return TRUE;
}


void OpalPCSSEndPoint::SetSoundChannelBufferDepth(unsigned depth)
{
  PAssert(depth > 1, PInvalidParameter);
  soundChannelBuffers = depth;
}


/////////////////////////////////////////////////////////////////////////////

OpalPCSSConnection::OpalPCSSConnection(OpalCall & call, OpalPCSSEndPoint & ep)
  : OpalConnection(call, ep, "xxx"),
    endpoint(ep)
{
  currentPhase = SetUpPhase;
  PTRACE(3, "PCSS\tCreated PC sound system connection.");
}


OpalPCSSConnection::~OpalPCSSConnection()
{
  PTRACE(3, "PCSS\tDeleted PC sound system connection.");
}


OpalConnection::Phases OpalPCSSConnection::GetPhase() const
{
  return currentPhase;
}


BOOL OpalPCSSConnection::SetAlerting(const PString & /*calleeName*/)
{
  return TRUE;
}


BOOL OpalPCSSConnection::SetConnected()
{
  return TRUE;
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
    deviceName = endpoint.GetSoundChannelRecordDevice();
  else
    deviceName = endpoint.GetSoundChannelPlayDevice();

  PSoundChannel * soundChannel = new PSoundChannel;
//  soundChannel->SetBuffers(format.GetFrameSize(), 2);

  if (soundChannel->Open(deviceName,
                         isSource ? PSoundChannel::Recorder
                                  : PSoundChannel::Player,
                         1, 8000, 16)) {
    PTRACE(3, "PCSS\tOpened sound channel \"" << deviceName
           << "\" for " << (isSource ? "record" : "play") << "ing.");
    return new OpalAudioMediaStream(isSource, sessionID, soundChannel);
  }

  PTRACE(1, "PCSS\tCould not open sound channel \"" << deviceName
         << "\" for " << (isSource ? "record" : "play")
         << "ing: " << soundChannel->GetErrorText());

  delete soundChannel;
  return NULL;
}


void OpalPCSSConnection::AnsweringCall(AnswerCallResponse response)
{
  if (currentPhase != AlertingPhase)
    return;

  switch (response) {
    case AnswerCallNow :
      currentPhase = EstablishedPhase;
      OnEstablished();
      break;

    case AnswerCallDenied :
      SetCallEndReason(EndedByAnswerDenied);
      currentPhase = ReleasedPhase;
      break;

    case AnswerCallPending :
      endpoint.OnShowRinging(remotePartyName);

    default :
      break;
  }
}


BOOL OpalPCSSConnection::SendConnectionAlert(const PString & calleeName)
{
  currentPhase = AlertingPhase;
  remotePartyName = calleeName;
  return endpoint.OnShowAlerting(calleeName);
}


void OpalPCSSConnection::StartOutgoingCall()
{
  currentPhase = EstablishedPhase;
  OnEstablished();
}


void OpalPCSSConnection::StartIncomingCall(const PString & callerName)
{
  currentPhase = AlertingPhase;
  remotePartyName = callerName;
  endpoint.OnShowRinging(callerName);
}


/////////////////////////////////////////////////////////////////////////////
