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
 * $Log: connection.cxx,v $
 * Revision 1.2002  2001/08/01 05:45:01  robertj
 * Moved media formats list from endpoint to connection.
 *
 * Revision 2.0  2001/07/27 15:48:25  robertj
 * Conversion of OpenH323 to Open Phone Abstraction Library (OPAL)
 *
 */

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "connection.h"
#endif

#include <opal/connection.h>

#include <opal/endpoint.h>
#include <opal/call.h>
#include <opal/transcoders.h>


#define new PNEW


#if PTRACING
const char * const OpalConnection::PhasesNames[NumPhases] = {
  "SetUpPhase",
  "AlertingPhase",
  "ConnectedPhase",
  "EstablishedPhase",
  "ReleasedPhase"
};

const char * const OpalConnection::AnswerCallResponseNames[NumAnswerCallResponses] = {
  "AnswerCallNow",
  "AnswerCallDenied",
  "AnswerCallPending",
  "AnswerCallDeferred",
  "AnswerCallAlertWithMedia"
};

const char * const OpalConnection::CallEndReasonNames[NumCallEndReasons] = {
  "EndedByLocalUser",         /// Local endpoint application cleared call
  "EndedByNoAccept",          /// Local endpoint did not accept call OnIncomingCall()=FALSE
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
};
#endif


/////////////////////////////////////////////////////////////////////////////

OpalConnection::OpalConnection(OpalCall & call,
                               OpalEndPoint  & ep,
                               const PString & token)
  : ownerCall(call),
    endpoint(ep),
    callToken(token)
{
  PTRACE(3, "OpalCon\tCreated connection " << *this);

  callAnswered = FALSE;
  callEndReason = NumCallEndReasons;
  answerResponse = AnswerCallDeferred;
  bandwidthAvailable = endpoint.GetInitialBandwidth();

  mediaStreams.DisallowDeleteObjects();

  ownerCall.AddConnection(this);
}


OpalConnection::~OpalConnection()
{
  PTRACE(3, "OpalCon\tConnection " << *this << " destroyed.");
}


void OpalConnection::PrintOn(ostream & strm) const
{
  strm << ownerCall << '-'<< endpoint << '[' << callToken << ']';
}


BOOL OpalConnection::Lock()
{
  // If shutting down don't try and lock, just return failed. If not then lock
  // it but do second test for shut down to avoid a possible race condition.

  if (GetPhase() == ReleasedPhase)
    return FALSE;

  inUseFlag.Wait();

  if (GetPhase() != ReleasedPhase)
    return TRUE;

  inUseFlag.Signal();
  return FALSE;
}


void OpalConnection::SetCallEndReason(CallEndReason reason)
{
  // Only set reason if not already set to something
  if (callEndReason == NumCallEndReasons)
    callEndReason = reason;
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


void OpalConnection::Release(CallEndReason reason)
{
  // Now set reason for the connection close
  SetCallEndReason(reason);
  ownerCall.Release(this);
}


BOOL OpalConnection::OnReleased()
{
  for (PINDEX i = 0; i < mediaStreams.GetSize(); i++)
    mediaStreams[i].Close();

  return endpoint.OnReleased(*this);
}


BOOL OpalConnection::OnIncomingConnection()
{
  return endpoint.OnIncomingConnection(*this);
}


PString OpalConnection::GetDestinationAddress()
{
  return "*";
}


BOOL OpalConnection::ForwardCall(const PString & /*forwardParty*/)
{
  return FALSE;
}


void OpalConnection::SetAnswerResponse(AnswerCallResponse response)
{
  if (response == AnswerCallDeferred)
    return;

  answerResponse = response;
  answerWaitFlag.Signal();
}


void OpalConnection::OnAlerting()
{
  endpoint.OnAlerting(*this);
}


void OpalConnection::OnConnected()
{
  endpoint.OnConnected(*this);
}


void OpalConnection::OnEstablished()
{
  endpoint.OnEstablished(*this);
}


BOOL OpalConnection::OpenSourceMediaStream(const OpalMediaFormatList & mediaFormats,
                                           unsigned sessionID)
{
  PTRACE(3, "OpalCon\tOpenSourceMediaStream " << *this);

  // See if already opened
  if (GetMediaStream(sessionID, TRUE) != NULL)
    return TRUE;

  OpalMediaStream * stream = CreateMediaStream(TRUE, sessionID);
  if (stream == NULL) {
    PTRACE(1, "OpalCon\tCreateMediaStream " << *this << " returned NULL");
    return FALSE;
  }

  OpalMediaFormat sourceFormat, destinationFormat;
  if (OpalTranscoder::SelectFormats(sessionID,
                                    stream->GetMediaFormats(),
                                    mediaFormats,
                                    sourceFormat,
                                    destinationFormat)) {
    PTRACE(3, "OpalCon\tOpenSourceMediaStream, selected "
           << sourceFormat << " -> " << destinationFormat);
    if (stream->Open(sourceFormat)) {
      if (OnOpenMediaStream(*stream)) {
        mediaStreams.Append(stream);
        return TRUE;
      }
    }
    else {
      PTRACE(2, "OpalCon\tSource media stream open of " << sourceFormat << " failed.");
    }
  }
  else {
    PTRACE(2, "OpalCon\tOpenSourceMediaStream, could not find compatible media format:\n"
              "  source formats=" << setfill(',') << stream->GetMediaFormats() << "\n"
              "   sink  formats=" << mediaFormats << setfill(' '));
  }

  delete stream;
  return FALSE;
}


OpalMediaStream * OpalConnection::OpenSinkMediaStream(OpalMediaStream & source)
{
  unsigned sessionID = source.GetSessionID();
  PTRACE(3, "OpalCon\tOpenSinkMediaStream " << *this << " session=" << sessionID);

  OpalMediaStream * stream = CreateMediaStream(FALSE, sessionID);
  if (stream == NULL) {
    PTRACE(1, "OpalCon\tCreateMediaStream " << *this << " returned NULL");
    return FALSE;
  }

  OpalMediaFormat sourceFormat, destinationFormat;
  if (OpalTranscoder::SelectFormats(sessionID,
                                    source.GetMediaFormats(),
                                    stream->GetMediaFormats(),
                                    sourceFormat,
                                    destinationFormat)) {
    PTRACE(3, "OpalCon\tOpenSinkMediaStream, selected "
           << sourceFormat << " -> " << destinationFormat);
    if (stream->Open(destinationFormat)) {
      if (OnOpenMediaStream(*stream)) {
        mediaStreams.Append(stream);
        return stream;
      }
    }
    else {
      PTRACE(2, "OpalCon\tSink media stream open of " << destinationFormat << " failed.");
    }
  }
  else {
    PTRACE(2, "OpalCon\tOpenSinkMediaStream, could not find compatible media format:\n"
              "  source formats=" << setfill(',') << source.GetMediaFormats() << "\n"
              "   sink  formats=" << stream->GetMediaFormats() << setfill(' '));
  }

  delete stream;
  return NULL;
}


BOOL OpalConnection::OnOpenMediaStream(OpalMediaStream & stream)
{
  return endpoint.OnOpenMediaStream(*this, stream);
}


void OpalConnection::OnClosedMediaStream(const OpalMediaStream & stream)
{
  endpoint.OnClosedMediaStream(stream);
}


OpalMediaStream * OpalConnection::GetMediaStream(unsigned sessionId, BOOL source) const
{
  for (PINDEX i = 0; i < mediaStreams.GetSize(); i++) {
    if (mediaStreams[i].GetSessionID() == sessionId &&
        mediaStreams[i].IsSource() == source)
      return &mediaStreams[i];
  }

  return NULL;
}


RTP_Session * OpalConnection::GetSession(unsigned sessionID) const
{
  return rtpSessions.GetSession(sessionID);
}


RTP_Session * OpalConnection::UseSession(unsigned sessionID)
{
  return rtpSessions.UseSession(sessionID);
}


void OpalConnection::ReleaseSession(unsigned sessionID)
{
  rtpSessions.ReleaseSession(sessionID);
}


BOOL OpalConnection::SetBandwidthAvailable(unsigned newBandwidth, BOOL force)
{
  PTRACE(3, "OpalCon\tSetting bandwidth to "
         << newBandwidth << "00b/s on connection " << *this);

  unsigned used = GetBandwidthUsed();
  if (used > newBandwidth) {
    if (!force)
      return FALSE;

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
  return TRUE;
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


BOOL OpalConnection::RequestBandwidth(unsigned bandwidth)
{
  if (bandwidth == 0)
    return TRUE;

  PTRACE(3, "OpalCon\tBandwidth request of "
         << bandwidth << "00b/s for " << *this);

  if (bandwidth > bandwidthAvailable) {
    PTRACE(2, "OpalCon\tAvailable bandwidth exceeded on " << *this);
    return FALSE;
  }

  bandwidthAvailable -= bandwidth;

  return TRUE;
}


void OpalConnection::ReleaseBandwidth(unsigned bandwidth)
{
  if (bandwidth == 0)
    return;

  PTRACE(3, "OpalCon\tBandwidth release of "
         << bandwidth << "00b/s for " << *this);

  bandwidthAvailable += bandwidth;
}


BOOL OpalConnection::SendUserIndicationString(const PString & value)
{
  for (const char * c = value; *c != '\0'; c++) {
    if (!SendUserIndicationTone(*c, 0))
      return FALSE;
  }
  return TRUE;
}


BOOL OpalConnection::SendUserIndicationTone(char /*tone*/, int /*duration*/)
{
  return TRUE;
}


void OpalConnection::OnUserIndicationString(const PString & value)
{
  endpoint.OnUserIndicationString(*this, value);
}


void OpalConnection::OnUserIndicationTone(char tone, int duration)
{
  endpoint.OnUserIndicationTone(*this, tone, duration);
}


PString OpalConnection::GetUserIndication(unsigned timeout)
{
  PString reply;
  if (userIndicationAvailable.Wait(PTimeInterval(0, timeout))) {
    userIndicationMutex.Wait();
    reply = userIndicationString;
    userIndicationString = PString();
    userIndicationMutex.Signal();
  }
  return reply;
}


void OpalConnection::SetUserIndication(const PString & input)
{
  userIndicationMutex.Wait();
  userIndicationString += input;
  userIndicationMutex.Signal();
  userIndicationAvailable.Signal();
}


PString OpalConnection::ReadUserIndication(const char * terminators,
                                           unsigned lastDigitTimeout,
                                           unsigned firstDigitTimeout)
{
  PTRACE(3, "OpalCon\tReadUserIndication from " << *this);

  PromptUserIndication(TRUE);
  PString input = GetUserIndication(firstDigitTimeout);
  PromptUserIndication(FALSE);

  if (!input) {
    for (;;) {
      PString next = GetUserIndication(lastDigitTimeout);
      if (next.IsEmpty()) {
        PTRACE(3, "OpalCon\tReadUserIndication last character timeout on " << *this);
        break;
      }
      if (next.FindOneOf(terminators) != P_MAX_INDEX)
        break;
      input += next;
    }
  }
  else {
    PTRACE(3, "OpalCon\tReadUserIndication first character timeout on " << *this);
  }

  return input;
}


BOOL OpalConnection::PromptUserIndication(BOOL /*play*/)
{
  return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
