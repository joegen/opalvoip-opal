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
 * Revision 1.2057  2006/02/22 10:40:10  csoutheren
 * Added patch #1374583 from Frederic Heem
 * Added additional H.323 virtual function
 *
 * Revision 2.55  2005/12/06 21:32:25  dsandras
 * Applied patch from Frederic Heem <frederic.heem _Atttt_ telsey.it> to fix
 * assert in PSyncPoint when OnReleased is called twice from different threads.
 * Thanks! (Patch #1374240)
 *
 * Revision 2.54  2005/11/30 13:44:20  csoutheren
 * Removed compile warnings on Windows
 *
 * Revision 2.53  2005/11/24 20:31:55  dsandras
 * Added support for echo cancelation using Speex.
 * Added possibility to add a filter to an OpalMediaPatch for all patches of a connection.
 *
 * Revision 2.52  2005/10/04 20:31:30  dsandras
 * Minor code cleanup.
 *
 * Revision 2.51  2005/10/04 12:59:28  rjongbloed
 * Removed CanOpenSourceMediaStream/CanOpenSinkMediaStream functions and
 *   now use overides on OpenSourceMediaStream/OpenSinkMediaStream
 * Moved addition of a media stream to list in OpalConnection to OnOpenMediaStream
 *   so is consistent across protocols.
 *
 * Revision 2.50  2005/09/15 17:02:40  dsandras
 * Added the possibility for a connection to prevent the opening of a sink/source media stream.
 *
 * Revision 2.49  2005/09/06 12:44:49  rjongbloed
 * Many fixes to finalise the video processing: merging remote media
 *
 * Revision 2.48  2005/08/24 10:43:51  rjongbloed
 * Changed create video functions yet again so can return pointers that are not to be deleted.
 *
 * Revision 2.47  2005/08/23 12:45:09  rjongbloed
 * Fixed creation of video preview window and setting video grab/display initial frame size.
 *
 * Revision 2.46  2005/08/04 17:20:22  dsandras
 * Added functions to close/remove the media streams of a connection.
 *
 * Revision 2.45  2005/07/14 08:51:19  csoutheren
 * Removed CreateExternalRTPAddress - it's not needed because you can override GetMediaAddress
 * to do the same thing
 * Fixed problems with logic associated with media bypass
 *
 * Revision 2.44  2005/07/11 06:52:16  csoutheren
 * Added support for outgoing calls using external RTP
 *
 * Revision 2.43  2005/07/11 01:52:26  csoutheren
 * Extended AnsweringCall to work for SIP as well as H.323
 * Fixed problems with external RTP connection in H.323
 * Added call to OnClosedMediaStream
 *
 * Revision 2.42  2005/06/02 13:18:02  rjongbloed
 * Fixed compiler warnings
 *
 * Revision 2.41  2005/04/10 21:12:59  dsandras
 * Added function to put the OpalMediaStreams on pause.
 *
 * Revision 2.40  2005/04/10 21:12:12  dsandras
 * Added support for Blind Transfer.
 *
 * Revision 2.39  2005/04/10 21:11:25  dsandras
 * Added support for call hold.
 *
 * Revision 2.38  2004/08/14 07:56:35  rjongbloed
 * Major revision to utilise the PSafeCollection classes for the connections and calls.
 *
 * Revision 2.37  2004/07/14 13:26:14  rjongbloed
 * Fixed issues with the propagation of the "established" phase of a call. Now
 *   calling an OnEstablished() chain like OnAlerting() and OnConnected() to
 *   finally arrive at OnEstablishedCall() on OpalManager
 *
 * Revision 2.36  2004/07/11 12:42:13  rjongbloed
 * Added function on endpoints to get the list of all media formats any
 *   connection the endpoint may create can support.
 *
 * Revision 2.35  2004/05/17 13:24:18  rjongbloed
 * Added silence suppression.
 *
 * Revision 2.34  2004/05/15 12:53:03  rjongbloed
 * Added default username and display name to manager, for all endpoints.
 *
 * Revision 2.33  2004/05/01 10:00:52  rjongbloed
 * Fixed ClearCallSynchronous so now is actually signalled when call is destroyed.
 *
 * Revision 2.32  2004/04/26 04:33:06  rjongbloed
 * Move various call progress times from H.323 specific to general conenction.
 *
 * Revision 2.31  2004/04/18 13:31:28  rjongbloed
 * Added new end call value from OpenH323.
 *
 * Revision 2.30  2004/03/13 06:25:54  rjongbloed
 * Slight rearrangement of local party name and alias list to beter match common
 *   behaviour in ancestor.
 * Abstracted local party name for endpoint into ancestor from H.,323.
 *
 * Revision 2.29  2004/03/02 09:57:54  rjongbloed
 * Fixed problems with recent changes to RTP UseSession() function which broke it for H.323
 *
 * Revision 2.28  2004/02/24 11:30:11  rjongbloed
 * Normalised RTP session management across protocols
 *
 * Revision 2.27  2004/01/18 15:36:47  rjongbloed
 * Fixed problem with symmetric codecs
 *
 * Revision 2.26  2003/06/02 03:11:01  rjongbloed
 * Fixed start media streams function actually starting the media streams.
 * Fixed problem where a sink stream should be opened (for preference) using
 *   the source streams media format. That is no transcoder is used.
 *
 * Revision 2.25  2003/03/18 06:42:39  robertj
 * Fixed incorrect return value, thanks gravsten
 *
 * Revision 2.24  2003/03/17 10:27:00  robertj
 * Added video support.
 *
 * Revision 2.23  2003/03/07 08:12:54  robertj
 * Changed DTMF entry so single # returns itself instead of an empty string.
 *
 * Revision 2.22  2003/03/06 03:57:47  robertj
 * IVR support (work in progress) requiring large changes everywhere.
 *
 * Revision 2.21  2003/01/07 04:39:53  robertj
 * Updated to OpenH323 v1.11.2
 *
 * Revision 2.20  2002/11/10 11:33:19  robertj
 * Updated to OpenH323 v1.10.3
 *
 * Revision 2.19  2002/07/01 04:56:33  robertj
 * Updated to OpenH323 v1.9.1
 *
 * Revision 2.18  2002/06/16 02:24:05  robertj
 * Fixed memory leak of media streams in non H323 protocols, thanks Ted Szoczei
 *
 * Revision 2.17  2002/04/10 03:09:01  robertj
 * Moved code for handling media bypass address resolution into ancestor as
 *   now done ths same way in both SIP and H.323.
 *
 * Revision 2.16  2002/04/09 00:19:06  robertj
 * Changed "callAnswered" to better description of "originating".
 *
 * Revision 2.15  2002/02/19 07:49:23  robertj
 * Added OpalRFC2833 as a OpalMediaFormat variable.
 * Restructured media bypass functions to fix problems with RFC2833.
 *
 * Revision 2.14  2002/02/13 02:31:13  robertj
 * Added trace for default CanDoMediaBypass
 *
 * Revision 2.13  2002/02/11 09:32:13  robertj
 * Updated to openH323 v1.8.0
 *
 * Revision 2.12  2002/02/11 07:41:58  robertj
 * Added media bypass for streams between compatible protocols.
 *
 * Revision 2.11  2002/01/22 05:12:12  robertj
 * Revamp of user input API triggered by RFC2833 support
 *
 * Revision 2.10  2001/11/15 06:56:54  robertj
 * Added session ID to trace log in OenSourceMediaStreams
 *
 * Revision 2.9  2001/11/14 01:31:55  robertj
 * Corrected placement of adjusting media format list.
 *
 * Revision 2.8  2001/11/02 10:45:19  robertj
 * Updated to OpenH323 v1.7.3
 *
 * Revision 2.7  2001/10/15 04:34:02  robertj
 * Added delayed start of media patch threads.
 * Removed answerCall signal and replaced with state based functions.
 *
 * Revision 2.6  2001/10/04 00:44:51  robertj
 * Removed GetMediaFormats() function as is not useful.
 *
 * Revision 2.5  2001/10/03 05:56:15  robertj
 * Changes abndwidth management API.
 *
 * Revision 2.4  2001/08/22 10:20:09  robertj
 * Changed connection locking to use double mutex to guarantee that
 *   no threads can ever deadlock or access deleted connection.
 *
 * Revision 2.3  2001/08/17 08:26:26  robertj
 * Moved call end reasons enum from OpalConnection to global.
 *
 * Revision 2.2  2001/08/13 05:10:40  robertj
 * Updates from OpenH323 v1.6.0 release.
 *
 * Revision 2.1  2001/08/01 05:45:01  robertj
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

#include <opal/manager.h>
#include <opal/endpoint.h>
#include <opal/call.h>
#include <opal/transcoders.h>
#include <codec/silencedetect.h>
#include <codec/echocancel.h>
#include <codec/rfc2833.h>
#include <rtp/rtp.h>
#include <t120/t120proto.h>
#include <t38/t38proto.h>


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
    "EndedByTemporaryFailure",  /// The remote failed temporarily app may retry
    "EndedByQ931Cause",         /// The remote ended the call with unmapped Q.931 cause code
    "EndedByDurationLimit",     /// Call cleared due to an enforced duration limit
    "EndedByInvalidConferenceID",/// Call cleared due to invalid conference ID
  };
  return out << names[reason];
}

ostream & operator<<(ostream & o, OpalConnection::AnswerCallResponse s)
{
  static const char * const AnswerCallResponseNames[OpalConnection::NumAnswerCallResponses] = {
    "AnswerCallNow",
    "AnswerCallDenied",
    "AnswerCallPending",
    "AnswerCallDeferred",
    "AnswerCallAlertWithMedia",
    "AnswerCallDeferredWithMedia"
  };
  if ((PINDEX)s >= PARRAYSIZE(AnswerCallResponseNames))
    o << "InvalidAnswerCallResponse<" << (unsigned)s << '>';
  else if (AnswerCallResponseNames[s] == NULL)
    o << "AnswerCallResponse<" << (unsigned)s << '>';
  else
    o << AnswerCallResponseNames[s];
  return o;
}

#endif

/////////////////////////////////////////////////////////////////////////////

OpalConnection::OpalConnection(OpalCall & call,
                               OpalEndPoint  & ep,
                               const PString & token)
  : ownerCall(call),
    endpoint(ep),
    callToken(token),
    alertingTime(0),
    connectedTime(0),
    callEndTime(0),
    localPartyName(ep.GetDefaultLocalPartyName()),
    displayName(ep.GetDefaultDisplayName()),
    remotePartyName(token)
{
  PTRACE(3, "OpalCon\tCreated connection " << *this);

  PAssert(ownerCall.SafeReference(), PLogicError);
  ownerCall.connectionsActive.Append(this);

  phase = UninitialisedPhase;
  originating = FALSE;
  callEndReason = NumCallEndReasons;
  detectInBandDTMF = !endpoint.GetManager().DetectInBandDTMFDisabled();
  minAudioJitterDelay = endpoint.GetManager().GetMinAudioJitterDelay();
  maxAudioJitterDelay = endpoint.GetManager().GetMaxAudioJitterDelay();
  bandwidthAvailable = endpoint.GetInitialBandwidth();

  silenceDetector = NULL;
  echoCanceler = NULL;
  
  rfc2833Handler = new OpalRFC2833Proto(PCREATE_NOTIFIER(OnUserInputInlineRFC2833));

  t120handler = NULL;
  t38handler = NULL;
}


OpalConnection::~OpalConnection()
{
  delete silenceDetector;
  delete echoCanceler;
  delete rfc2833Handler;
  delete t120handler;
  delete t38handler;

  ownerCall.SafeDereference();

  PTRACE(3, "OpalCon\tConnection " << *this << " destroyed.");
}


void OpalConnection::PrintOn(ostream & strm) const
{
  strm << ownerCall << '-'<< endpoint << '[' << callToken << ']';
}

BOOL OpalConnection::OnSetUpConnection()
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


BOOL OpalConnection::IsConnectionOnHold()
{
  return FALSE;
}

void OpalConnection::SetCallEndReason(CallEndReason reason)
{
  // Only set reason if not already set to something
  if (callEndReason == NumCallEndReasons) {
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
  PTRACE(3, "OpalCon\tCan not transfer connection to " << remoteParty);
}


void OpalConnection::Release(CallEndReason reason)
{
  PSafeLockReadWrite safeLock(*this);
  if (!safeLock.IsLocked() || phase >= ReleasingPhase) {
    PTRACE(3, "OpalCon\tAlready released " << *this);
    return;
  }

  PTRACE(3, "OpalCon\tReleasing " << *this);

  // Now set reason for the connection close
  SetCallEndReason(reason);
  SetPhase(ReleasingPhase);

  // Add a reference for the thread we are about to start
  SafeReference();
  PThread::Create(PCREATE_NOTIFIER(OnReleaseThreadMain), 0,
                  PThread::AutoDeleteThread,
                  PThread::NormalPriority,
                  "OnRelease:%x");
}


void OpalConnection::OnReleaseThreadMain(PThread &, INT)
{
  OnReleased();

  PTRACE(3, "OpalCon\tOnRelease thread completed for " << GetToken());

  // Dereference on the way out of the thread
  SafeDereference();
}


void OpalConnection::OnReleased()
{
  PTRACE(3, "OpalCon\tOnReleased " << *this);

  CloseMediaStreams();

  endpoint.OnReleased(*this);
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


BOOL OpalConnection::OpenSourceMediaStream(const OpalMediaFormatList & mediaFormats,
                                           unsigned sessionID)
{
  // See if already opened
  if (GetMediaStream(sessionID, TRUE) != NULL) {
    PTRACE(3, "OpalCon\tOpenSourceMediaStream (already opened) for session "
           << sessionID << " on " << *this);
    return TRUE;
  }

  PTRACE(3, "OpalCon\tOpenSourceMediaStream for session " << sessionID << " on " << *this);

  OpalMediaFormat sourceFormat, destinationFormat;
  if (!OpalTranscoder::SelectFormats(sessionID,
                                     GetMediaFormats(),
                                     mediaFormats,
                                     sourceFormat,
                                     destinationFormat)) {
    PTRACE(2, "OpalCon\tOpenSourceMediaStream session " << sessionID
           << ", could not find compatible media format:\n"
              "  source formats=" << setfill(',') << GetMediaFormats() << "\n"
              "   sink  formats=" << mediaFormats << setfill(' '));
    return FALSE;
  }

  if (!sourceFormat.Merge(destinationFormat)) {
    PTRACE(2, "OpalCon\tOpenSourceMediaStream session " << sessionID
           << ", could not merge destination media format " << destinationFormat
           << " into source " << sourceFormat);
    return FALSE;
  }

  PTRACE(3, "OpalCon\tSelected media stream "
         << sourceFormat << " -> " << destinationFormat);
  
  OpalMediaStream *stream = CreateMediaStream(sourceFormat, sessionID, TRUE);
  if (stream == NULL) {
    PTRACE(1, "OpalCon\tCreateMediaStream returned NULL for session "
           << sessionID << " on " << *this);
    return FALSE;
  }

  if (stream->Open()) {
    if (OnOpenMediaStream(*stream))
      return TRUE;
    PTRACE(2, "OpalCon\tSource media OnOpenMediaStream open of " << sourceFormat << " failed.");
  }
  else {
    PTRACE(2, "OpalCon\tSource media stream open of " << sourceFormat << " failed.");
  }

  delete stream;
  return FALSE;
}


OpalMediaStream * OpalConnection::OpenSinkMediaStream(OpalMediaStream & source)
{
  unsigned sessionID = source.GetSessionID();
  
  PTRACE(3, "OpalCon\tOpenSinkMediaStream " << *this << " session=" << sessionID);

  OpalMediaFormat sourceFormat = source.GetMediaFormat();

  // Reorder the media formats from this protocol so we give preference
  // to what has been selected in the source media stream.
  OpalMediaFormatList destinationFormats = GetMediaFormats();
  PStringArray order = sourceFormat;
  // Second preference is given to the previous media stream already
  // opened to maintain symmetric codecs, if possible.
  OpalMediaStream * otherStream = GetMediaStream(sessionID, TRUE);
  if (otherStream != NULL)
    order += otherStream->GetMediaFormat();
  destinationFormats.Reorder(order);

  OpalMediaFormat destinationFormat;
  if (!OpalTranscoder::SelectFormats(sessionID,
                                     sourceFormat, // Only use selected format on source
                                     destinationFormats,
                                     sourceFormat,
                                     destinationFormat)) {
    PTRACE(2, "OpalCon\tOpenSinkMediaStream, could not find compatible media format:\n"
              "  source formats=" << setfill(',') << source.GetMediaFormat() << "\n"
              "   sink  formats=" << destinationFormats << setfill(' '));
    return NULL;
  }

  PTRACE(3, "OpalCon\tOpenSinkMediaStream, selected "
         << sourceFormat << " -> " << destinationFormat);

  OpalMediaStream * stream = CreateMediaStream(destinationFormat, sessionID, FALSE);
  if (stream == NULL) {
    PTRACE(1, "OpalCon\tCreateMediaStream " << *this << " returned NULL");
    return NULL;
  }

  if (stream->Open()) {
    if (OnOpenMediaStream(*stream))
      return stream;
    PTRACE(2, "OpalCon\tSink media stream OnOpenMediaStream of " << destinationFormat << " failed.");
  }
  else {
    PTRACE(2, "OpalCon\tSink media stream open of " << destinationFormat << " failed.");
  }

  delete stream;
  return NULL;
}


void OpalConnection::StartMediaStreams()
{
  for (PINDEX i = 0; i < mediaStreams.GetSize(); i++)
    mediaStreams[i].Start();
  PTRACE(2, "OpalCon\tMedia stream threads started.");
}


void OpalConnection::CloseMediaStreams()
{
  for (PINDEX i = 0; i < mediaStreams.GetSize(); i++) {
    if (mediaStreams[i].IsOpen()) {
      OnClosedMediaStream(mediaStreams[i]);
      mediaStreams[i].Close();
    }
  }
  
  PTRACE(2, "OpalCon\tMedia stream threads closed.");
}


void OpalConnection::RemoveMediaStreams()
{
  CloseMediaStreams();
  mediaStreams.RemoveAll();
  
  PTRACE(2, "OpalCon\tMedia stream threads removed from session.");
}


void OpalConnection::PauseMediaStreams(BOOL paused)
{
  for (PINDEX i = 0; i < mediaStreams.GetSize(); i++)
    mediaStreams[i].SetPaused(paused);
}


OpalMediaStream * OpalConnection::CreateMediaStream(const OpalMediaFormat & mediaFormat,
                                                    unsigned sessionID,
                                                    BOOL isSource)
{
  if (sessionID == OpalMediaFormat::DefaultVideoSessionID) {
    if (isSource) {
      PVideoInputDevice * videoDevice;
      BOOL autoDelete;
      if (CreateVideoInputDevice(mediaFormat, videoDevice, autoDelete)) {
        PVideoOutputDevice * previewDevice;
        if (!CreateVideoOutputDevice(mediaFormat, TRUE, previewDevice, autoDelete))
          previewDevice = NULL;
        return new OpalVideoMediaStream(mediaFormat, sessionID, videoDevice, previewDevice, autoDelete);
      }
    }
    else {
      PVideoOutputDevice * videoDevice;
      BOOL autoDelete;
      if (CreateVideoOutputDevice(mediaFormat, FALSE, videoDevice, autoDelete))
        return new OpalVideoMediaStream(mediaFormat, sessionID, NULL, videoDevice, autoDelete);
    }
  }

  return NULL;
}


BOOL OpalConnection::OnOpenMediaStream(OpalMediaStream & stream)
{
  if (!endpoint.OnOpenMediaStream(*this, stream))
    return FALSE;

  mediaStreams.Append(&stream);

  if (phase == ConnectedPhase) {
    SetPhase(EstablishedPhase);
    OnEstablished();
  }

  return TRUE;
}


void OpalConnection::OnClosedMediaStream(const OpalMediaStream & stream)
{
  endpoint.OnClosedMediaStream(stream);
}


void OpalConnection::OnPatchMediaStream(BOOL /*isSource*/, OpalMediaPatch & /*patch*/)
{
  PTRACE(3, "OpalCon\tNew patch created");
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


BOOL OpalConnection::IsMediaBypassPossible(unsigned /*sessionID*/) const
{
  PTRACE(3, "OpalCon\tIsMediaBypassPossible: default returns FALSE");
  return FALSE;
}


BOOL OpalConnection::GetMediaInformation(unsigned sessionID,
                                         MediaInformation & info) const
{
  if (!mediaTransportAddresses.Contains(sessionID)) {
    PTRACE(3, "OpalCon\tGetMediaInformation for session " << sessionID << " - no channel.");
    return FALSE;
  }

  OpalTransportAddress & address = mediaTransportAddresses[sessionID];

  PIPSocket::Address ip;
  WORD port;
  if (address.GetIpAndPort(ip, port)) {
    info.data    = OpalTransportAddress(ip, (WORD)(port&0xfffe));
    info.control = OpalTransportAddress(ip, (WORD)(port|0x0001));
  }
  else
    info.data = info.control = address;

  info.rfc2833 = rfc2833Handler->GetPayloadType();
  PTRACE(3, "OpalCon\tGetMediaInformation for session " << sessionID
         << " data=" << info.data << " rfc2833=" << info.rfc2833);
  return TRUE;
}


void OpalConnection::AddVideoMediaFormats(OpalMediaFormatList & mediaFormats) const
{
  endpoint.AddVideoMediaFormats(mediaFormats, this);
}


BOOL OpalConnection::CreateVideoInputDevice(const OpalMediaFormat & mediaFormat,
                                            PVideoInputDevice * & device,
                                            BOOL & autoDelete)
{
  return endpoint.CreateVideoInputDevice(*this, mediaFormat, device, autoDelete);
}


BOOL OpalConnection::CreateVideoOutputDevice(const OpalMediaFormat & mediaFormat,
                                             BOOL preview,
                                             PVideoOutputDevice * & device,
                                             BOOL & autoDelete)
{
  return endpoint.CreateVideoOutputDevice(*this, mediaFormat, preview, device, autoDelete);
}


RTP_Session * OpalConnection::GetSession(unsigned sessionID) const
{
  return rtpSessions.GetSession(sessionID);
}


RTP_Session * OpalConnection::UseSession(const OpalTransport & transport,
                                         unsigned sessionID,
                                         RTP_QOS * rtpqos)
{
  RTP_Session * rtpSession = rtpSessions.UseSession(sessionID);
  if (rtpSession == NULL) {
    rtpSession = CreateSession(transport, sessionID, rtpqos);
    rtpSessions.AddSession(rtpSession);
  }

  return rtpSession;
}


void OpalConnection::ReleaseSession(unsigned sessionID)
{
  rtpSessions.ReleaseSession(sessionID);
}


RTP_Session * OpalConnection::CreateSession(const OpalTransport & transport,
                                            unsigned sessionID,
                                            RTP_QOS * rtpqos)
{
  // We only support RTP over UDP at this point in time ...
  if (!transport.IsCompatibleTransport("ip$127.0.0.1"))
    return NULL;
                                        
  PIPSocket::Address localAddress;
  transport.GetLocalAddress().GetIpAddress(localAddress);

  OpalManager & manager = GetEndPoint().GetManager();

  PIPSocket::Address remoteAddress;
  transport.GetRemoteAddress().GetIpAddress(remoteAddress);
  PSTUNClient * stun = manager.GetSTUN(remoteAddress);

  // create an RTP session
  RTP_UDP * rtpSession = new RTP_UDP(sessionID);

  WORD firstPort = manager.GetRtpIpPortPair();
  WORD nextPort = firstPort;
  while (!rtpSession->Open(localAddress,
                           nextPort, nextPort,
                           manager.GetRtpIpTypeofService(),
                           stun,
                           rtpqos)) {
    nextPort = manager.GetRtpIpPortPair();
    if (nextPort == firstPort) {
      delete rtpSession;
      return NULL;
    }
  }

  localAddress = rtpSession->GetLocalAddress();
  if (manager.TranslateIPAddress(localAddress, remoteAddress))
    rtpSession->SetLocalAddress(localAddress);
  return rtpSession;
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


BOOL OpalConnection::SetBandwidthUsed(unsigned releasedBandwidth,
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
    return FALSE;
  }

  bandwidthAvailable -= requiredBandwidth;

  return TRUE;
}


BOOL OpalConnection::SendUserInputString(const PString & value)
{
  for (const char * c = value; *c != '\0'; c++) {
    if (!SendUserInputTone(*c, 0))
      return FALSE;
  }
  return TRUE;
}


BOOL OpalConnection::SendUserInputTone(char tone, unsigned duration)
{
  if (duration == 0)
    duration = 180;

  return rfc2833Handler->SendTone(tone, duration);
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
  if (userInputAvailable.Wait(PTimeInterval(0, timeout))) {
    userInputMutex.Wait();
    reply = userInputString;
    userInputString = PString();
    userInputMutex.Signal();
  }
  return reply;
}


void OpalConnection::SetUserInput(const PString & input)
{
  userInputMutex.Wait();
  userInputString += input;
  userInputMutex.Signal();
  userInputAvailable.Signal();
}


PString OpalConnection::ReadUserInput(const char * terminators,
                                      unsigned lastDigitTimeout,
                                      unsigned firstDigitTimeout)
{
  PTRACE(3, "OpalCon\tReadUserInput from " << *this);

  PromptUserInput(TRUE);
  PString input = GetUserInput(firstDigitTimeout);
  PromptUserInput(FALSE);

  if (!input) {
    for (;;) {
      PString next = GetUserInput(lastDigitTimeout);
      if (next.IsEmpty()) {
        PTRACE(3, "OpalCon\tReadUserInput last character timeout on " << *this);
        break;
      }
      if (next.FindOneOf(terminators) != P_MAX_INDEX) {
        if (input.IsEmpty())
          input = next;
        break;
      }
      input += next;
    }
  }
  else {
    PTRACE(3, "OpalCon\tReadUserInput first character timeout on " << *this);
  }

  return input;
}


BOOL OpalConnection::PromptUserInput(BOOL /*play*/)
{
  return TRUE;
}


void OpalConnection::OnUserInputInlineRFC2833(OpalRFC2833Info & info, INT)
{
  if (!info.IsToneStart())
    OnUserInputTone(info.GetTone(), info.GetDuration()/8);
}


void OpalConnection::OnUserInputInBandDTMF(RTP_DataFrame & frame, INT)
{
  // This function is set up as an 'audio filter'.
  // This allows us to access the 16 bit PCM audio (at 8Khz sample rate)
  // before the audio is passed on to the sound card (or other output device)

  // Pass the 16 bit PCM audio through the DTMF decoder   
  PString tones = dtmfDecoder.Decode(frame.GetPayloadPtr(), frame.GetPayloadSize());
  if (!tones.IsEmpty()) {
    PTRACE(1, "DTMF detected. " << tones);
    PINDEX i;
    for (i = 0; i < tones.GetLength(); i++) {
      OnUserInputTone(tones[i], 0);
    }
  }
}


OpalT120Protocol * OpalConnection::CreateT120ProtocolHandler()
{
  if (t120handler == NULL)
    t120handler = endpoint.CreateT120ProtocolHandler(*this);
  return t120handler;
}


OpalT38Protocol * OpalConnection::CreateT38ProtocolHandler()
{
  if (t38handler == NULL)
    t38handler = endpoint.CreateT38ProtocolHandler(*this);
  return t38handler;
}


void OpalConnection::SetLocalPartyName(const PString & name)
{
  localPartyName = name;
}


void OpalConnection::SetAudioJitterDelay(unsigned minDelay, unsigned maxDelay)
{
  PAssert(minDelay <= 1000 && maxDelay <= 1000, PInvalidParameter);

  if (minDelay < 10)
    minDelay = 10;
  minAudioJitterDelay = minDelay;

  if (maxDelay < minDelay)
    maxDelay = minDelay;
  maxAudioJitterDelay = maxDelay;
}

void OpalConnection::SetPhase(Phases phaseToSet)
{
    PTRACE(3, "OpalCon\tSetPhase from " << phase << " to " << phaseToSet);
    phase = phaseToSet;
}
/////////////////////////////////////////////////////////////////////////////
