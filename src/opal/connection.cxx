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
 * Revision 1.2089.2.10  2007/08/05 13:12:18  hfriederich
 * Backport from HEAD - Changes since last commit
 *
 * Revision 2.88.2.9  2007/05/28 16:41:45  hfriederich
 * Backport from HEAD, changes since May 3, 2007
 *
 * Revision 2.88.2.8  2007/05/03 10:37:50  hfriederich
 * Backport from HEAD.
 * All changes since Apr 1, 2007
 *
 * Revision 2.88.2.7  2007/03/30 06:44:45  hfriederich
 * (Backport from HEAD)
 * Tidied some code when a new connection is created by an endpoint. Now
 *   if someone needs to derive a connectino class they can create it without
 *   needing to remember to do any more than the new.
 * Fixed various GCC warnings
 *
 * Revision 2.88.2.6  2007/03/29 21:45:56  hfriederich
 * (Backport from HEAD)
 * Pass OpalConnection to OpalMediaSream constructor
 * Add ID to OpalMediaStreams so that transcoders can match incoming and
 *   outgoing codecs
 *
 * Revision 2.88.2.5  2007/03/20 09:33:57  hfriederich
 * (Backport from HEAD)
 * Simple but messy changes to allow compile time removal of protocol options
 *   such as H.450 and H.460.
 * Fix MakeConnection overrides
 *
 * Revision 2.88.2.4  2007/03/10 07:49:09  hfriederich
 * (Backport from HEAD)
 * Fixed backward compatibility of OnIncomingConnection() virtual functions
 *   on various classes. If an old override returned FALSE then it will now
 *   abort the call as it used to.
 * Backports some other fixes from HEAD.
 *
 * Revision 2.88.2.3  2007/02/14 08:26:12  hfriederich
 * (Backport from HEAD)
 * Fix problem with using SIP connections that have no StringOptions
 *
 * Revision 2.88.2.2  2007/02/10 23:07:22  hfriederich
 * Allow to adjust media formats between connections.
 * Allow H323 capabilities to update their state based on media formats.
 *
 * Revision 2.88.2.1  2007/02/07 08:51:02  hfriederich
 * New branch with major revision of the core Opal media format handling system.
 *
 * - Session IDs have been replaced by new OpalMediaType class.
 * - The creation of H.245 TCS and SDP media descriptions have been extended
 *   to dynamically handle all available media types
 * - The H.224 code has been rewritten for better integration into the Opal
 *   system. It takes advantage of the new media type system and removes
 *   all hooks found in the core Opal classes.
 *
 * More work will follow as the current version breaks lots of important code.
 *
 * Revision 2.88  2007/01/24 04:00:57  csoutheren
 * Arrrghh. Changing OnIncomingConnection turned out to have a lot of side-effects
 * Added some pure viritual functions to prevent old code from breaking silently
 * New OpalEndpoint and OpalConnection descendants will need to re-implement
 * OnIncomingConnection. Sorry :)
 *
 * Revision 2.87  2007/01/24 00:28:28  csoutheren
 * Fixed overrides of OnIncomingConnection
 *
 * Revision 2.86  2007/01/18 12:25:33  csoutheren
 * Add ability to set H.323 call-id via options
 *
 * Revision 2.85  2007/01/18 04:45:17  csoutheren
 * Messy, but simple change to add additional options argument to OpalConnection constructor
 * This allows the provision of non-trivial arguments for connections
 *
 * Revision 2.84  2006/12/18 03:18:42  csoutheren
 * Messy but simple fixes
 *   - Add access to SIP REGISTER timeout
 *   - Ensure OpalConnection options are correctly progagated
 *
 * Revision 2.83  2006/12/08 04:22:06  csoutheren
 * Applied 1589261 - new release cause for fxo endpoints
 * Thanks to Frederic Heem
 *
 * Revision 2.82  2006/11/20 03:37:12  csoutheren
 * Allow optional inclusion of RTP aggregation
 *
 * Revision 2.81  2006/11/19 06:02:58  rjongbloed
 * Moved function that reads User Input into a destination address to
 *   OpalManager so can be easily overidden in applications.
 *
 * Revision 2.80  2006/11/11 12:23:18  hfriederich
 * Code reorganisation to improve RFC2833 handling for both SIP and H.323. Thanks Simon Zwahlen for the idea
 *
 * Revision 2.79  2006/10/28 16:40:28  dsandras
 * Fixed SIP reinvite without breaking H.323 calls.
 *
 * Revision 2.78  2006/10/25 13:02:54  rjongbloed
 * Fixed compiler error due to tone subsystem upgrade
 *
 * Revision 2.77  2006/10/10 07:18:18  csoutheren
 * Allow compilation with and without various options
 *
 * Revision 2.76  2006/09/28 07:42:18  csoutheren
 * Merge of useful SRTP implementation
 *
 * Revision 2.75  2006/09/13 00:22:53  csoutheren
 * Fix reentrancy problems in Release
 *
 * Revision 2.74  2006/08/29 08:47:43  rjongbloed
 * Added functions to get average audio signal level from audio streams in
 *   suitable connection types.
 *
 * Revision 2.73  2006/08/28 00:07:43  csoutheren
 * Applied 1545125 - SetPhase mutex protection and transition control
 * Thanks to Drazen Dimoti
 *
 * Revision 2.72  2006/08/17 23:09:04  rjongbloed
 * Added volume controls
 *
 * Revision 2.71  2006/08/10 05:10:33  csoutheren
 * Various H.323 stability patches merged in from DeimosPrePLuginBranch
 *
 * Revision 2.70  2006/08/10 04:26:36  csoutheren
 * Applied 1534434 - OpalConnection::SetAudioJitterDelay - checking of values
 * Thanks to Drazen Dimoti
 *
 * Revision 2.69  2006/08/03 04:57:12  csoutheren
 * Port additional NAT handling logic from OpenH323 and extend into OpalConnection class
 *
 * Revision 2.68  2006/07/24 14:03:40  csoutheren
 * Merged in audio and video plugins from CVS branch PluginBranch
 *
 * Revision 2.67.2.3  2006/08/09 12:49:21  csoutheren
 * Improve stablity under heavy H.323 load
 *
 * Revision 2.67.2.2  2006/08/03 08:09:20  csoutheren
 * Removed old code
 *
 * Revision 2.67.2.1  2006/08/03 08:01:15  csoutheren
 * Added additional locking for media stream list to remove crashes in very busy applications
 *
 * Revision 2.67  2006/07/21 00:42:08  csoutheren
 * Applied 1525040 - More locking when changing connection's phase
 * Thanks to Borko Jandras
 *
 * Revision 2.66  2006/07/05 05:09:12  csoutheren
 * Applied 1494937 - Allow re-opening of mediastrams
 * Thanks to mturconi
 *
 * Revision 2.65  2006/06/30 00:49:07  csoutheren
 * Applied 1469865 - remove connection from call's connection list
 * Thanks to Frederich Heem
 *
 * Revision 2.64  2006/06/27 13:07:37  csoutheren
 * Patch 1374533 - add h323 Progress handling
 * Thanks to Frederich Heem
 *
 * Revision 2.63  2006/06/09 04:22:24  csoutheren
 * Implemented mapping between SIP release codes and Q.931 codes as specified
 *  by RFC 3398
 *
 * Revision 2.62  2006/05/30 04:58:06  csoutheren
 * Added suport for SIP INFO message (untested as yet)
 * Fixed some issues with SIP state machine on answering calls
 * Fixed some formatting issues
 *
 * Revision 2.61  2006/05/23 17:26:52  dsandras
 * Reverted previous patch preventing OnEstablished to be called with H.323 calls.
 *
 * Revision 2.60  2006/04/20 16:52:22  hfriederich
 * Adding support for H.224/H.281
 *
 * Revision 2.59  2006/04/09 12:12:54  rjongbloed
 * Changed the media format option merging to include the transcoder formats.
 *
 * Revision 2.58  2006/03/29 23:57:52  csoutheren
 * Added patches from Paul Caswell to provide correct operation when using
 * external RTP with H.323
 *
 * Revision 2.57  2006/03/20 10:37:47  csoutheren
 * Applied patch #1453753 - added locking on media stream manipulation
 * Thanks to Dinis Rosario
 *
 * Revision 2.56.2.4  2006/04/11 05:12:25  csoutheren
 * Updated to current OpalMediaFormat changes
 *
 * Revision 2.56.2.3  2006/04/10 06:24:30  csoutheren
 * Backport from CVS head up to Plugin_Merge3
 *
 * Revision 2.56.2.2  2006/04/07 07:57:20  csoutheren
 * Halfway through media format changes - not working, but closer
 *
 * Revision 2.56.2.1  2006/04/06 05:33:08  csoutheren
 * Backports from CVS head up to Plugin_Merge2
 *
 * Revision 2.59  2006/04/09 12:12:54  rjongbloed
 * Changed the media format option merging to include the transcoder formats.
 *
 * Revision 2.58  2006/03/29 23:57:52  csoutheren
 * Added patches from Paul Caswell to provide correct operation when using
 * external RTP with H.323
 *
 * Revision 2.57  2006/03/20 10:37:47  csoutheren
 * Applied patch #1453753 - added locking on media stream manipulation
 * Thanks to Dinis Rosario
 *
 * Revision 2.56  2006/02/22 10:40:10  csoutheren
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
  : PSafeObject(&call), // Share the lock flag from the call
    ownerCall(call),
    endpoint(ep),
    phase(UninitialisedPhase),
    callToken(token),
    originating(FALSE),
    alertingTime(0),
    connectedTime(0),
    callEndTime(0),
    productInfo(ep.GetProductInfo()),
    localPartyName(ep.GetDefaultLocalPartyName()),
    displayName(ep.GetDefaultDisplayName()),
    remotePartyName(token),
    callEndReason(NumCallEndReasons),
    remoteIsNAT(FALSE),
    q931Cause(0x100),
    silenceDetector(NULL),
    echoCanceler(NULL),
#if OPAL_T120DATA
    t120handler(NULL),
#endif
#if OPAL_T38FAX
    t38handler(NULL),
#endif
    stringOptions((_stringOptions == NULL) ? NULL : new OpalConnection::StringOptions(*_stringOptions))
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
  
  rfc2833Handler = new OpalRFC2833Proto(*this, PCREATE_NOTIFIER(OnUserInputInlineRFC2833));
#if OPAL_T38FAX
  ciscoNSEHandler = new OpalRFC2833Proto(*this, PCREATE_NOTIFIER(OnUserInputInlineCiscoNSE));
#endif

  securityMode = ep.GetDefaultSecurityMode();

  switch (options & RTPAggregationMask) {
    case RTPAggregationDisable:
      useRTPAggregation = FALSE;
      break;
    case RTPAggregationEnable:
      useRTPAggregation = TRUE;
      break;
    default:
      useRTPAggregation = endpoint.UseRTPAggregation();
  }
  
  switch (options & AdjustMediaFormatOptionsMask) {
    case AdjustMediaFormatOptionsEnable:
      adjustMediaFormatOptions = TRUE;
      break;
    case AdjustMediaFormatOptionsDisable:
      adjustMediaFormatOptions = FALSE;
      break;
    default:
      adjustMediaFormatOptions = endpoint.GetAdjustMediaFormatOptions();
  }

  if (stringOptions != NULL) {
    PString id((*stringOptions)("Call-Identifier"));
    if (!id.IsEmpty())
      callIdentifier = PGloballyUniqueID(id);
  }
}

OpalConnection::~OpalConnection()
{
  mediaStreams.RemoveAll();

  delete silenceDetector;
  delete echoCanceler;
  delete rfc2833Handler;
#if OPAL_T120DATA
  delete t120handler;
#endif
#if OPAL_T38FAX
  delete t38handler;
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
    if ((reason & EndedWithQ931Code) != 0) {
      SetQ931Cause((int)reason >> 24);
      reason = (CallEndReason)(reason & 0xff);
    }
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
  PTRACE(2, "OpalCon\tCan not transfer connection to " << remoteParty);
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
  return TRUE;
}

BOOL OpalConnection::OnIncomingConnection(unsigned int /*options*/)
{
  return TRUE;
}


BOOL OpalConnection::OnIncomingConnection(unsigned options, OpalConnection::StringOptions * stringOptions)
{
  return OnIncomingConnection() && 
         OnIncomingConnection(options) && 
         endpoint.OnIncomingConnection(*this, options, stringOptions);
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

BOOL OpalConnection::OpenSourceMediaStream(const OpalMediaFormatList & mediaFormats, const OpalMediaType & mediaType)
{
  PSafeLockReadWrite safeLock(*this);
  if (!safeLock.IsLocked())
    return NULL;
  
  // See if already opened
  OpalMediaStream * stream = GetMediaStream(mediaType, TRUE);

  if (stream != NULL && stream->IsOpen()) {
    PTRACE(3, "OpalCon\tOpenSourceMediaStream (already opened) for media type "
               << mediaType << " on " << *this);
    return TRUE;
  }

  PTRACE(3, "OpalCon\tOpenSourceMediaStream for media type " << mediaType << " on " << *this);

  OpalMediaFormat sourceFormat, destinationFormat;
  if (!OpalTranscoder::SelectFormats(mediaType,
                                     GetMediaFormats(),
                                     mediaFormats,
                                     sourceFormat,
                                     destinationFormat)) {
    PTRACE(2, "OpalCon\tOpenSourceMediaStream media type " << mediaType
           << ", could not find compatible media format:\n"
              "  source formats=" << setfill(',') << GetMediaFormats() << "\n"
              "   sink  formats=" << mediaFormats << setfill(' '));
    return FALSE;
  }

  PTRACE(3, "OpalCon\tSelected media stream " << sourceFormat << " -> " << destinationFormat);
    
  if (stream == NULL)
    stream = CreateMediaStream(sourceFormat, TRUE);

  if (stream == NULL) {
    PTRACE(1, "OpalCon\tCreateMediaStream returned NULL for session "
          << mediaType << " on " << *this);
    return FALSE;
  }

  if (stream->Open()) {
    if (OnOpenMediaStream(*stream)) {
      PTRACE(3, "OpalCon\tOpened source stream " << stream->GetID());
      return TRUE;
    }
    PTRACE(2, "OpalCon\tSource media OnOpenMediaStream open of " << sourceFormat << " failed.");
  }
  else {
    PTRACE(2, "OpalCon\tSource media stream open of " << sourceFormat << " failed.");
  }

  if (!RemoveMediaStream(stream))
    delete stream;

  return FALSE;
}


OpalMediaStream * OpalConnection::OpenSinkMediaStream(OpalMediaStream & source)
{
  const OpalMediaType & mediaType = source.GetMediaType();
  
  PTRACE(3, "OpalCon\tOpenSinkMediaStream " << *this << " session=" << mediaType);

  OpalMediaFormat sourceFormat = source.GetMediaFormat();

  // Reorder the media formats from this protocol so we give preference
  // to what has been selected in the source media stream.
  OpalMediaFormatList destinationFormats = GetMediaFormats();
  PStringArray order = sourceFormat;
  // Second preference is given to the previous media stream already
  // opened to maintain symmetric codecs, if possible.

  PSafeLockReadWrite safeLock(*this);
  if (!safeLock.IsLocked())
    return NULL;
  
  OpalMediaStream * otherStream = GetMediaStream(mediaType, TRUE);
  if (otherStream != NULL)
    order += otherStream->GetMediaFormat();
  destinationFormats.Reorder(order);

  OpalMediaFormat destinationFormat;
  if (!OpalTranscoder::SelectFormats(mediaType,
                                     sourceFormat, // Only use selected format on source
                                     destinationFormats,
                                     sourceFormat,
                                     destinationFormat)) {
    PTRACE(2, "OpalCon\tOpenSinkMediaStream, could not find compatible media format:\n"
              "  source formats=" << setfill(',') << sourceFormat << "\n"
              "   sink  formats=" << destinationFormats << setfill(' '));
    return NULL;
  }

  PTRACE(3, "OpalCon\tOpenSinkMediaStream, selected " << sourceFormat << " -> " << destinationFormat);

  OpalMediaStream * stream = CreateMediaStream(destinationFormat, FALSE);
  if (stream == NULL) {
    PTRACE(1, "OpalCon\tCreateMediaStream " << *this << " returned NULL");
    return NULL;
  }

  if (stream->Open()) {
    if (OnOpenMediaStream(*stream)) {
      PTRACE(3, "OpalCon\tOpened sink stream " << stream->GetID());
      return stream;
    }
    PTRACE(2, "OpalCon\tSink media stream OnOpenMediaStream of " << destinationFormat << " failed.");
  }
  else {
    PTRACE(2, "OpalCon\tSink media stream open of " << destinationFormat << " failed.");
  }
  if (!RemoveMediaStream(stream))
    delete stream;

  return NULL;
}


void OpalConnection::StartMediaStreams()
{
  if (!LockReadWrite())
    return;
  
  for (PINDEX i = 0; i < mediaStreams.GetSize(); i++) {
    if (mediaStreams[i].IsOpen()) {
      OpalMediaStream & strm = mediaStreams[i];
      strm.Start();
    }
  }
  
  UnlockReadWrite();
  
  PTRACE(3, "OpalCon\tMedia stream threads started.");
}


void OpalConnection::CloseMediaStreams()
{
  GetCall().OnStopRecordAudio(callIdentifier.AsString());
    
  if (!LockReadWrite())
    return;
  
  for (PINDEX i = 0; i < mediaStreams.GetSize(); i++) {
    OpalMediaStream & strm = mediaStreams[i];
    if (strm.IsOpen()) {
      OnClosedMediaStream(strm);
      strm.Close();
    }
  }
  
  UnlockReadWrite();
  
  PTRACE(3, "OpalCon\tMedia stream threads closed.");
}


void OpalConnection::RemoveMediaStreams()
{
  if (!LockReadWrite())
    return;
  
  CloseMediaStreams();
  mediaStreams.RemoveAll();
  
  UnlockReadWrite();
  
  PTRACE(3, "OpalCon\tMedia stream threads removed from session.");
}

void OpalConnection::PauseMediaStreams(BOOL paused)
{
  if (!LockReadWrite())
    return;
  
  for (PINDEX i = 0; i < mediaStreams.GetSize(); i++)
    mediaStreams[i].SetPaused(paused);
  
  UnlockReadWrite();
}


OpalMediaStream * OpalConnection::CreateMediaStream(
#if OPAL_VIDEO
  const OpalMediaFormat & mediaFormat,
  BOOL isSource
#else
  const OpalMediaFormat & ,
  BOOL
#endif
)
{
#if OPAL_VIDEO
  const OpalMediaType & mediaType = mediaFormat.GetMediaType();
  if (mediaType == OpalDefaultVideoMediaType) {
    if (isSource) {
      PVideoInputDevice * videoDevice;
      BOOL autoDelete;
      if (CreateVideoInputDevice(mediaFormat, videoDevice, autoDelete)) {
        PVideoOutputDevice * previewDevice;
        if (!CreateVideoOutputDevice(mediaFormat, TRUE, previewDevice, autoDelete))
          previewDevice = NULL;
        return new OpalVideoMediaStream(*this, mediaFormat, videoDevice, previewDevice, autoDelete);
      }
    }
    else {
      PVideoOutputDevice * videoDevice;
      BOOL autoDelete;
      if (CreateVideoOutputDevice(mediaFormat, FALSE, videoDevice, autoDelete))
        return new OpalVideoMediaStream(*this, mediaFormat, NULL, videoDevice, autoDelete);
    }
  }
#endif

  return NULL;
}


BOOL OpalConnection::OnOpenMediaStream(OpalMediaStream & stream)
{
  if (!endpoint.OnOpenMediaStream(*this, stream))
    return FALSE;

  if (!LockReadWrite())
    return FALSE;

  if (mediaStreams.GetObjectsIndex(&stream) == P_MAX_INDEX)
    mediaStreams.Append(&stream);

  if (GetPhase() == ConnectedPhase) {
    SetPhase(EstablishedPhase);
    OnEstablished();
  }

  UnlockReadWrite();

  return TRUE;
}


void OpalConnection::OnClosedMediaStream(const OpalMediaStream & stream)
{
  endpoint.OnClosedMediaStream(stream);
}


void OpalConnection::OnPatchMediaStream(BOOL /*isSource*/, OpalMediaPatch & /*patch*/)
{
  if (!recordAudioFilename.IsEmpty())
    GetCall().StartRecording(recordAudioFilename);
    
  // TODO - add autorecord functions here
  PTRACE(4, "OpalCon\tNew patch created");
}

void OpalConnection::EnableRecording()
{
  if (!LockReadWrite())
    return;
  
  OpalMediaStream * stream = GetMediaStream(OpalDefaultAudioMediaType, TRUE);
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
  
  OpalMediaStream * stream = GetMediaStream(OpalDefaultAudioMediaType, TRUE);
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

void OpalConnection::AttachRFC2833HandlerToPatch(BOOL isSource, OpalMediaPatch & patch)
{
  if(rfc2833Handler != NULL) {
    if(isSource) {
      PTRACE(3, "OpalCon\tAdding RFC2833 receive handler");
      OpalMediaStream & mediaStream = patch.GetSource();
      patch.AddFilter(rfc2833Handler->GetReceiveHandler(), mediaStream.GetMediaFormat());
    }
  }
    
#if OPAL_T38FAX
  if (ciscoNSEHandler != NULL) {
    if(isSource) {
      PTRACE(3, "OpalCon\tAdding Cisco NSE receive handler");
      OpalMediaStream & mediaStream = patch.GetSource();
      patch.AddFilter(ciscoNSEHandler->GetReceiveHandler(), mediaStream.GetMediaFormat());
    }
  }
#endif
}


OpalMediaStream * OpalConnection::GetMediaStream(const OpalMediaType & mediaType, BOOL source) const
{
  PSafeLockReadWrite safeLock(*this);
  if (!safeLock.IsLocked())
    return NULL;
  
  for (PINDEX i = 0; i < mediaStreams.GetSize(); i++) {
    if (mediaStreams[i].GetMediaType() == mediaType &&
        mediaStreams[i].IsSource() == source)
      return &mediaStreams[i];
  }

  return NULL;
}

BOOL OpalConnection::RemoveMediaStream(OpalMediaStream * strm)
{
  PSafeLockReadWrite safeLock(*this);
  if (!safeLock.IsLocked())
    return FALSE;
  
  PINDEX index = mediaStreams.GetObjectsIndex(strm);
  if (index == P_MAX_INDEX) 
    return FALSE;

  OpalMediaStream & s = mediaStreams[index];
  if (s.IsOpen()) {
    OnClosedMediaStream(s);
    s.Close();
  }
  mediaStreams.RemoveAt(index);

  return TRUE;
}



BOOL OpalConnection::IsMediaBypassPossible(const OpalMediaType & /* media type */) const
{
  PTRACE(4, "OpalCon\tIsMediaBypassPossible: default returns FALSE");
  return FALSE;
}


BOOL OpalConnection::GetMediaInformation(const OpalMediaType & mediaType,
                                         MediaInformation & info) const
{
  if (!mediaTransportAddresses.Contains(mediaType.GetUniqueID())) {
    PTRACE(2, "OpalCon\tGetMediaInformation for session " << mediaType << " - no channel.");
    return FALSE;
  }

  OpalTransportAddress & address = mediaTransportAddresses[mediaType.GetUniqueID()];

  PIPSocket::Address ip;
  WORD port;
  if (address.GetIpAndPort(ip, port)) {
    info.data    = OpalTransportAddress(ip, (WORD)(port&0xfffe));
    info.control = OpalTransportAddress(ip, (WORD)(port|0x0001));
  }
  else
    info.data = info.control = address;

  // fill payloadType with payload type for RFC2833
  info.payloadType = rfc2833Handler->GetPayloadType();
  PTRACE(3, "OpalCon\tGetMediaInformation for session " << mediaType
         << " data=" << info.data << " rfc2833=" << info.payloadType);
  return TRUE;
}

#if OPAL_VIDEO

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

#endif // OPAL_VIDEO


BOOL OpalConnection::SetAudioVolume(BOOL /*source*/, unsigned /*percentage*/)
{
  return FALSE;
}


unsigned OpalConnection::GetAudioSignalLevel(BOOL /*source*/)
{
    return UINT_MAX;
}


RTP_Session * OpalConnection::GetSession(const OpalMediaType & mediaType) const
{
  return rtpSessions.GetSession(mediaType);
}


RTP_Session * OpalConnection::UseSession(const OpalMediaType & mediaType)
{
  return rtpSessions.UseSession(mediaType);
}


RTP_Session * OpalConnection::UseSession(const OpalTransport & transport,
                                         const OpalMediaType & mediaType,
                                         RTP_QOS * rtpqos)
{
  RTP_Session * rtpSession = rtpSessions.UseSession(mediaType);
  if (rtpSession == NULL) {
    rtpSession = CreateSession(transport, mediaType, rtpqos);
    rtpSessions.AddSession(rtpSession);
  }

  return rtpSession;
}


void OpalConnection::ReleaseSession(const OpalMediaType & mediaType,
                                    BOOL clearAll)
{
  rtpSessions.ReleaseSession(mediaType, clearAll);
}


RTP_Session * OpalConnection::CreateSession(const OpalTransport & transport,
                                            const OpalMediaType & mediaType,
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

  // create an (S)RTP session or T38 pseudo-session as appropriate
  RTP_UDP * rtpSession = NULL;

  if (!securityMode.IsEmpty()) {
    OpalSecurityMode * parms = PFactory<OpalSecurityMode>::CreateInstance(securityMode);
    if (parms == NULL) {
      PTRACE(1, "OpalCon\tSecurity mode " << securityMode << " unknown");
      return NULL;
    }
    rtpSession = parms->CreateRTPSession(
                  useRTPAggregation ? endpoint.GetRTPAggregator() : NULL, 
                  mediaType, remoteIsNAT);
    if (rtpSession == NULL) {
      PTRACE(1, "OpalCon\tCannot create RTP session for security mode " << securityMode);
      delete parms;
      return NULL;
    }
  }
  else
  {
    rtpSession = new RTP_UDP(
                   useRTPAggregation ? endpoint.GetRTPAggregator() : NULL, 
                   mediaType, remoteIsNAT);
  }

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
  PTRACE(3, "OpalCon\tSetting bandwidth to " << newBandwidth << "00b/s on connection " << *this);

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

void OpalConnection::SetSendUserInputMode(SendUserInputModes mode)
{
  PTRACE(3, "OPAL\tSetting default User Input send mode to " << mode);
  sendUserInputMode = mode;
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

  return rfc2833Handler->SendToneAsync(tone, duration);
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


BOOL OpalConnection::PromptUserInput(BOOL /*play*/)
{
  return TRUE;
}


void OpalConnection::OnUserInputInlineRFC2833(OpalRFC2833Info & info, INT)
{
  if (!info.IsToneStart())
    OnUserInputTone(info.GetTone(), info.GetDuration()/8);
}


void OpalConnection::OnUserInputInlineCiscoNSE(OpalRFC2833Info & /*info*/, INT)
{
    cout << "Received NSE event" << endl;
    //if (!info.IsToneStart())
    //  OnUserInputTone(info.GetTone(), info.GetDuration()/8);
}

#if P_DTMF
void OpalConnection::OnUserInputInBandDTMF(RTP_DataFrame & frame, INT)
{
  // This function is set up as an 'audio filter'.
  // This allows us to access the 16 bit PCM audio (at 8Khz sample rate)
  // before the audio is passed on to the sound card (or other output device)

  // Pass the 16 bit PCM audio through the DTMF decoder   
  PString tones = dtmfDecoder.Decode((const short *)frame.GetPayloadPtr(), frame.GetPayloadSize()/sizeof(short));
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


void OpalConnection::SetAudioJitterDelay(unsigned minDelay, unsigned maxDelay)
{
  maxDelay = PMAX(10, PMIN(maxDelay, 999));
  minDelay = PMAX(10, PMIN(minDelay, 999));

  if (maxDelay < minDelay)
    maxDelay = minDelay;

  minAudioJitterDelay = minDelay;
  maxAudioJitterDelay = maxDelay;
}

void OpalConnection::SetPhase(Phases phaseToSet)
{
  PTRACE(3, "OpalCon\tSetPhase from " << phase << " to " << phaseToSet);

  PWaitAndSignal m(phaseMutex);

  // With next few lines we will prevent phase to ever go down when it
  // reaches ReleasingPhase - end result - once you call Release you never
  // go back.
  if (phase < ReleasingPhase) {
    phase = phaseToSet;
  } else if (phase == ReleasingPhase && phaseToSet == ReleasedPhase) {
    phase = phaseToSet;
  }
}


BOOL OpalConnection::OnOpenIncomingMediaChannels()
{
  return TRUE;
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

BOOL OpalConnection::IsRTPNATEnabled(const PIPSocket::Address & localAddr,
                                     const PIPSocket::Address & peerAddr,
                                     const PIPSocket::Address & sigAddr,
                                     BOOL incoming)
{
  return endpoint.IsRTPNATEnabled(*this, localAddr, peerAddr, sigAddr, incoming);
}


/////////////////////////////////////////////////////////////////////////////
