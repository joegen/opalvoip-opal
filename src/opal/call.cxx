/*
 * call.cxx
 *
 * Call management
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
 * $Log: call.cxx,v $
 * Revision 2.67  2007/10/12 06:07:33  csoutheren
 * Fix problem with asymmetric codecs in some SIP calls
 *
 * Revision 2.66  2007/09/20 04:32:36  rjongbloed
 * Fixed issue with clearing a call before it has finished setting up.
 *
 * Revision 2.65  2007/09/18 10:06:03  rjongbloed
 * Fixed compiler warning
 *
 * Revision 2.64  2007/09/18 09:37:52  rjongbloed
 * Propagated call backs for RTP statistics through OpalManager and OpalCall.
 *
 * Revision 2.63  2007/06/28 12:08:26  rjongbloed
 * Simplified mutex strategy to avoid some wierd deadlocks. All locking of access
 *   to an OpalConnection must be via the PSafeObject locks.
 *
 * Revision 2.62  2007/06/22 02:02:56  rjongbloed
 * Removed extraneous logging.
 *
 * Revision 2.61  2007/05/09 01:39:28  csoutheren
 * Remove redundant patch for NULL source streams
 *
 * Revision 2.60  2007/05/07 14:14:31  csoutheren
 * Add call record capability
 *
 * Revision 2.59  2007/04/26 07:01:47  csoutheren
 * Add extra code to deal with getting media formats from connections early enough to do proper
 * gatewaying between calls. The SIP and H.323 code need to have the handing of the remote
 * and local formats standardized, but this will do for now
 *
 * Revision 2.58  2007/04/04 02:12:01  rjongbloed
 * Reviewed and adjusted PTRACE log levels
 *   Now follows 1=error,2=warn,3=info,4+=debug
 *
 * Revision 2.57  2007/04/03 05:27:30  rjongbloed
 * Cleaned up somewhat confusing usage of the OnAnswerCall() virtual
 *   function. The name is innaccurate and exists as a legacy from the
 *   OpenH323 days. it now only indicates how alerting is done
 *   (with/without media) and does not actually answer the call.
 *
 * Revision 2.56  2007/03/29 05:16:50  csoutheren
 * Pass OpalConnection to OpalMediaSream constructor
 * Add ID to OpalMediaStreams so that transcoders can match incoming and outgoing codecs
 *
 * Revision 2.55  2007/03/10 17:56:33  dsandras
 * Improved locking.
 *
 * Revision 2.54  2007/03/04 19:25:00  dsandras
 * Do not use ReadWrite locks when not required. Fixes potential deadlocks
 * in weird conditions.
 *
 * Revision 2.53  2007/02/19 08:35:11  csoutheren
 * Fix typo
 *
 * Revision 2.52  2007/01/25 11:48:11  hfriederich
 * OpalMediaPatch code refactorization.
 * Split into OpalMediaPatch (using a thread) and OpalPassiveMediaPatch
 * (not using a thread). Also adds the possibility for source streams
 * to push frames down to the sink streams instead of having a patch
 * thread around.
 *
 * Revision 2.51  2006/12/31 17:00:13  dsandras
 * Do not try transcoding RTP frames if they do not correspond to the formats
 * for which the transcoder was created.
 *
 * Revision 2.50  2006/11/06 13:57:40  dsandras
 * Use readonly locks as suggested by Robert as we are not
 * writing to the collection. Fixes deadlock on SIP when
 * reinvite.
 *
 * Revision 2.49  2006/11/01 14:40:17  dsandras
 * Applied fix from Hannes Friederich to prevent deadlock. Thanks!
 *
 * Revision 2.48  2006/08/28 00:10:55  csoutheren
 * Applied 1545126 - Deadlock fix in OpalCall
 * Thanks to Drazen Dimoti
 *
 * Revision 2.47  2006/08/10 05:10:31  csoutheren
 * Various H.323 stability patches merged in from DeimosPrePLuginBranch
 *
 * Revision 2.46  2006/07/24 14:03:40  csoutheren
 * Merged in audio and video plugins from CVS branch PluginBranch
 *
 * Revision 2.45.2.1  2006/08/03 08:01:15  csoutheren
 * Added additional locking for media stream list to remove crashes in very busy applications
 *
 * Revision 2.45  2006/07/14 04:22:43  csoutheren
 * Applied 1517397 - More Phobos stability fix
 * Thanks to Dinis Rosario
 *
 * Revision 2.44  2006/07/09 10:18:28  csoutheren
 * Applied 1517393 - Opal T.38
 * Thanks to Drazen Dimoti
 *
 * Revision 2.43  2006/03/29 23:53:03  csoutheren
 * Added call to OpalCall::OnSetUpConnection
 *
 * Revision 2.42  2006/03/20 10:37:47  csoutheren
 * Applied patch #1453753 - added locking on media stream manipulation
 * Thanks to Dinis Rosario
 *
 * Revision 2.41.2.4  2006/04/25 01:06:22  csoutheren
 * Allow SIP-only codecs
 *
 * Revision 2.41.2.3  2006/04/11 05:12:25  csoutheren
 * Updated to current OpalMediaFormat changes
 *
 * Revision 2.41.2.2  2006/04/07 07:57:20  csoutheren
 * Halfway through media format changes - not working, but closer
 *
 * Revision 2.41.2.1  2006/04/06 05:33:08  csoutheren
 * Backports from CVS head up to Plugin_Merge2
 *
 * Revision 2.43  2006/03/29 23:53:03  csoutheren
 * Added call to OpalCall::OnSetUpConnection
 *
 * Revision 2.42  2006/03/20 10:37:47  csoutheren
 * Applied patch #1453753 - added locking on media stream manipulation
 * Thanks to Dinis Rosario
 *
 * Revision 2.41  2006/02/02 07:02:57  csoutheren
 * Added RTP payload map to transcoders and connections to allow remote SIP endpoints
 * to change the payload type used for outgoing RTP.
 *
 * Revision 2.40  2006/01/31 03:29:46  csoutheren
 * Removed compiler warning
 *
 * Revision 2.39  2005/11/24 20:31:55  dsandras
 * Added support for echo cancelation using Speex.
 * Added possibility to add a filter to an OpalMediaPatch for all patches of a connection.
 *
 * Revision 2.38  2005/10/22 12:16:05  dsandras
 * Moved mutex preventing media streams to be opened before they are completely closed to the SIPConnection class.
 *
 * Revision 2.37  2005/10/04 13:02:24  rjongbloed
 * Removed CanOpenSourceMediaStream/CanOpenSinkMediaStream functions and
 *   now use overides on OpenSourceMediaStream/OpenSinkMediaStream
 *
 * Revision 2.36  2005/09/22 17:08:52  dsandras
 * Added mutex to protect media streams access and prevent media streams for a call to be closed before they are all opened.
 *
 * Revision 2.35  2005/09/15 17:05:32  dsandras
 * Only open media streams for a session with media formats corresponding to that session. Check if the sink media stream can be opened on the other connection(s) before opening the source media stream.
 *
 * Revision 2.34  2005/08/04 17:22:17  dsandras
 * Added functions to close/remove the media streams of a call.
 *
 * Revision 2.33  2005/07/14 08:56:44  csoutheren
 * Fixed problem with sink streams not being created when media bypass being used
 *
 * Revision 2.32  2005/07/11 01:52:25  csoutheren
 * Extended AnsweringCall to work for SIP as well as H.323
 * Fixed problems with external RTP connection in H.323
 * Added call to OnClosedMediaStream
 *
 * Revision 2.31  2004/11/30 00:15:07  csoutheren
 * Don' t convert userIndication::signalUpdate messages into UserInputString messages
 *
 * Revision 2.30  2004/08/18 13:03:57  rjongbloed
 * Changed to make calling OPalManager::OnClearedCall() in override optional.
 * Added setting of party A and B fields in call for display purposes.
 * Fixed warning.
 *
 * Revision 2.29  2004/08/14 07:56:32  rjongbloed
 * Major revision to utilise the PSafeCollection classes for the connections and calls.
 *
 * Revision 2.28  2004/07/14 13:26:14  rjongbloed
 * Fixed issues with the propagation of the "established" phase of a call. Now
 *   calling an OnEstablished() chain like OnAlerting() and OnConnected() to
 *   finally arrive at OnEstablishedCall() on OpalManager
 *
 * Revision 2.27  2004/07/11 12:43:17  rjongbloed
 * Added function to get a list of all possible media formats that may be used given
 *   a list of media and taking into account all of the registered transcoders.
 *
 * Revision 2.26  2004/05/09 13:19:00  rjongbloed
 * Fixed issues with non fast start and non-tunnelled connections
 *
 * Revision 2.25  2004/05/02 05:18:45  rjongbloed
 * More logging
 *
 * Revision 2.24  2004/05/01 10:00:52  rjongbloed
 * Fixed ClearCallSynchronous so now is actually signalled when call is destroyed.
 *
 * Revision 2.23  2004/04/25 02:53:29  rjongbloed
 * Fixed GNU 3.4 warnings
 *
 * Revision 2.22  2004/04/18 07:09:12  rjongbloed
 * Added a couple more API functions to bring OPAL into line with similar functions in OpenH323.
 *
 * Revision 2.21  2004/02/07 00:35:47  rjongbloed
 * Fixed calls GetMediaFormats so no DOES return intersection of all connections formats.
 * Tidied some API elements to make usage more explicit.
 *
 * Revision 2.20  2004/01/18 15:36:47  rjongbloed
 * Fixed problem with symmetric codecs
 *
 * Revision 2.19  2003/06/02 03:12:56  rjongbloed
 * Made changes so that media stream in opposite direction to the one already
 *   opened will use same media format for preference. That is try and use
 *   symmetric codecs if possible.
 *
 * Revision 2.18  2003/03/07 05:53:03  robertj
 * Made sure connection is locked with all function calls that are across
 *   the "call" object.
 *
 * Revision 2.17  2003/03/06 03:57:47  robertj
 * IVR support (work in progress) requiring large changes everywhere.
 *
 * Revision 2.16  2002/11/10 11:33:19  robertj
 * Updated to OpenH323 v1.10.3
 *
 * Revision 2.15  2002/04/09 00:21:41  robertj
 * Fixed media formats list not being sorted and sifted before use in opening streams.
 *
 * Revision 2.14  2002/04/08 02:40:13  robertj
 * Fixed issues with using double originate call, eg from simple app command line.
 *
 * Revision 2.13  2002/04/05 10:38:35  robertj
 * Rearranged OnRelease to remove the connection from endpoints connection
 *   list at the end of the release phase rather than the beginning.
 *
 * Revision 2.12  2002/02/19 07:48:50  robertj
 * Restructured media bypass functions to fix problems with RFC2833.
 *
 * Revision 2.11  2002/02/11 07:41:37  robertj
 * Added media bypass for streams between compatible protocols.
 *
 * Revision 2.10  2002/01/22 05:11:34  robertj
 * Revamp of user input API triggered by RFC2833 support
 *
 * Revision 2.9  2002/01/14 02:23:00  robertj
 * Fixed problem in not getting fast started medai in Alerting
 *
 * Revision 2.8  2001/11/15 07:01:55  robertj
 * Changed OpalCall::OpenSourceMediaStreams so the connection to not open
 *   a media stream on is optional.
 *
 * Revision 2.7  2001/11/14 01:31:55  robertj
 * Corrected placement of adjusting media format list.
 *
 * Revision 2.6  2001/11/06 05:33:52  robertj
 * Added fail safe starting of media streams on CONNECT
 *
 * Revision 2.5  2001/10/15 04:33:17  robertj
 * Removed answerCall signal and replaced with state based functions.
 *
 * Revision 2.4  2001/10/04 05:43:44  craigs
 * Changed to start media patch threads in Paused state
 *
 * Revision 2.3  2001/08/22 10:20:09  robertj
 * Changed connection locking to use double mutex to guarantee that
 *   no threads can ever deadlock or access deleted connection.
 *
 * Revision 2.2  2001/08/17 08:26:59  robertj
 * Added call end reason for whole call, not just connection.
 *
 * Revision 2.1  2001/08/01 05:29:19  robertj
 * Added function to get all media formats possible in a call.
 *
 * Revision 2.0  2001/07/27 15:48:25  robertj
 * Conversion of OpenH323 to Open Phone Abstraction Library (OPAL)
 *
 */

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "call.h"
#endif

#include <opal/call.h>

#include <opal/manager.h>
#include <opal/patch.h>
#include <opal/transcoders.h>


#define new PNEW


/////////////////////////////////////////////////////////////////////////////

#ifdef _MSC_VER
#pragma warning(disable:4355)
#endif

OpalCall::OpalCall(OpalManager & mgr)
  : manager(mgr)
  , myToken(mgr.GetNextCallToken())
  , isEstablished(FALSE)
  , isClearing(FALSE)
  , callEndReason(OpalConnection::NumCallEndReasons)
  , endCallSyncPoint(NULL)
{
  manager.activeCalls.SetAt(myToken, this);

  connectionsActive.DisallowDeleteObjects();

  PTRACE(3, "Call\tCreated " << *this);
}

#ifdef _MSC_VER
#pragma warning(default:4355)
#endif


OpalCall::~OpalCall()
{
  PTRACE(3, "Call\t" << *this << " destroyed.");

  manager.GetRecordManager().Close(myToken);
}


void OpalCall::PrintOn(ostream & strm) const
{
  strm << "Call[" << myToken << ']';
}


void OpalCall::OnEstablishedCall()
{
  manager.OnEstablishedCall(*this);
}


void OpalCall::SetCallEndReason(OpalConnection::CallEndReason reason)
{
  // Only set reason if not already set to something
  if (callEndReason == OpalConnection::NumCallEndReasons)
    callEndReason = reason;
}


void OpalCall::Clear(OpalConnection::CallEndReason reason, PSyncPoint * sync)
{
  PTRACE(3, "Call\tClearing " << (sync != NULL ? "(sync) " : "") << *this << " reason=" << reason);

  if (!LockReadWrite())
    return;

  isClearing = TRUE;

  SetCallEndReason(reason);

  if (sync != NULL && !connectionsActive.IsEmpty()) {
    // only set the sync point if it is NULL
    if (endCallSyncPoint == NULL)
      endCallSyncPoint = sync;
    else {
      PAssertAlways("Can only have one thread doing ClearCallSynchronous");
    }
  }

  UnlockReadWrite();

  for (PSafePtr<OpalConnection> connection(connectionsActive, PSafeReadOnly); connection != NULL; ++connection)
    connection->Release(reason);
}


void OpalCall::OnCleared()
{
  manager.OnClearedCall(*this);

  if (endCallSyncPoint != NULL)
    endCallSyncPoint->Signal();
}


BOOL OpalCall::OnSetUp(OpalConnection & connection)
{
  PTRACE(3, "Call\tOnSetUp " << connection);

  BOOL ok = FALSE;

  if (isClearing || !LockReadWrite())
    return FALSE;

  partyA = connection.GetRemotePartyName();

  UnlockReadWrite();

  for (PSafePtr<OpalConnection> conn(connectionsActive, PSafeReadOnly); conn != NULL; ++conn) {
    if (conn != &connection) {
      if (conn->SetUpConnection()) {
        ok = conn->OnSetUpConnection();
      }
    }
  }

  return ok;
}


BOOL OpalCall::OnAlerting(OpalConnection & connection)
{
  PTRACE(3, "Call\tOnAlerting " << connection);

  if (isClearing || !LockReadWrite())
    return FALSE;

  partyB = connection.GetRemotePartyName();

  UnlockReadWrite();


  BOOL hasMedia = connection.GetMediaStream(OpalMediaFormat::DefaultAudioSessionID, TRUE) != NULL;

  BOOL ok = FALSE;

  for (PSafePtr<OpalConnection> conn(connectionsActive, PSafeReadOnly); conn != NULL; ++conn) {
    if (conn != &connection) {
      if (conn->SetAlerting(connection.GetRemotePartyName(), hasMedia))
        ok = TRUE;
    }
  }

  return ok;
}

OpalConnection::AnswerCallResponse OpalCall::OnAnswerCall(OpalConnection & /*connection*/,
                                                          const PString & /*caller*/)
{
  return OpalConnection::AnswerCallPending;
}


BOOL OpalCall::OnConnected(OpalConnection & connection)
{
  PTRACE(3, "Call\tOnConnected " << connection);

  if (isClearing || !LockReadOnly())
    return FALSE;

  BOOL ok = connectionsActive.GetSize() == 1 && !partyB.IsEmpty();

  UnlockReadOnly();

  if (ok) {
    if (!manager.MakeConnection(*this, partyB))
      connection.Release(OpalConnection::EndedByNoUser);
    return OnSetUp(connection);
  }

  BOOL createdOne = FALSE;

  if (!LockReadOnly())
    return FALSE;

  for (PSafePtr<OpalConnection> conn(connectionsActive, PSafeReadOnly); conn != NULL; ++conn) {
    if (conn != &connection) {
      if (conn->SetConnected())
        ok = TRUE;
    }

    OpalMediaFormatList formats = GetMediaFormats(*conn, TRUE);
    if (OpenSourceMediaStreams(*conn, formats, OpalMediaFormat::DefaultAudioSessionID))
      createdOne = TRUE;
    if (OpenSourceMediaStreams(*conn, formats, OpalMediaFormat::DefaultVideoSessionID))
      createdOne = TRUE;
    if (OpenSourceMediaStreams(*conn, formats, OpalMediaFormat::DefaultDataSessionID))
      createdOne = TRUE;
  }

  UnlockReadOnly();
  
  if (ok && createdOne) {
    for (PSafePtr<OpalConnection> conn(connectionsActive, PSafeReadOnly); conn != NULL; ++conn)
      conn->StartMediaStreams();
  }

  return ok;
}


BOOL OpalCall::OnEstablished(OpalConnection & PTRACE_PARAM(connection))
{
  PTRACE(3, "Call\tOnEstablished " << connection);

  PSafeLockReadWrite lock(*this);
  if (isClearing || !lock.IsLocked())
    return FALSE;

  if (isEstablished)
    return TRUE;

  if (connectionsActive.GetSize() < 2)
    return FALSE;

  for (PSafePtr<OpalConnection> conn(connectionsActive, PSafeReference); conn != NULL; ++conn) {
    if (conn->GetPhase() != OpalConnection::EstablishedPhase)
      return FALSE;
  }

  isEstablished = TRUE;
  OnEstablishedCall();

  return TRUE;
}


PSafePtr<OpalConnection> OpalCall::GetOtherPartyConnection(const OpalConnection & connection) const
{
  PTRACE(3, "Call\tGetOtherPartyConnection " << connection);

  for (PSafePtr<OpalConnection> conn(connectionsActive, PSafeReadOnly); conn != NULL; ++conn) {
    if (conn != &connection)
      return conn;
  }

  return NULL;
}


OpalMediaFormatList OpalCall::GetMediaFormats(const OpalConnection & connection,
                                              BOOL includeSpecifiedConnection)
{
  OpalMediaFormatList commonFormats;

  BOOL first = TRUE;

  for (PSafePtr<OpalConnection> conn(connectionsActive, PSafeReadOnly); conn != NULL; ++conn) {
    if (includeSpecifiedConnection || conn != &connection) {
      OpalMediaFormatList possibleFormats = OpalTranscoder::GetPossibleFormats(conn->GetMediaFormats());
      if (first) {
        commonFormats = possibleFormats;
        first = FALSE;
      }
      else {
        // Want intersection of the possible formats for all connections.
        for (PINDEX i = 0; i < commonFormats.GetSize(); i++) {
          if (possibleFormats.GetValuesIndex(commonFormats[i]) == P_MAX_INDEX)
            commonFormats.RemoveAt(i--);
        }
      }
    }
  }

  connection.AdjustMediaFormats(commonFormats);

  PTRACE(4, "Call\tGetMediaFormats for " << connection << '\n'
         << setfill('\n') << commonFormats << setfill(' '));

  return commonFormats;
}

BOOL OpalCall::OpenSourceMediaStreams(const OpalConnection & connection,
                                      const OpalMediaFormatList & mediaFormats,
                                      unsigned sessionID)
{
  PTRACE(3, "Call\tOpenSourceMediaStreams for session " << sessionID
         << " with media " << setfill(',') << mediaFormats << setfill(' '));

  BOOL startedOne = FALSE;

  OpalMediaFormatList adjustableMediaFormats;
  // Keep the media formats for the session ID
  for (PINDEX i = 0; i < mediaFormats.GetSize(); i++) {
    if (mediaFormats[i].GetDefaultSessionID() == sessionID)
      adjustableMediaFormats += mediaFormats[i];
  }

  if (adjustableMediaFormats.GetSize() == 0)
    return FALSE;

  // if there is already a connection with an open stream then reorder the 
  // media formats to match that connection so we get symmetric codecs
  if (connectionsActive.GetSize() > 0) {
    OpalMediaStream * strm = connection.GetMediaStream(sessionID, TRUE);
    if (strm != NULL)
      adjustableMediaFormats.Reorder(strm->GetMediaFormat().GetName());
  }
  
  for (PSafePtr<OpalConnection> conn(connectionsActive, PSafeReadOnly); conn != NULL; ++conn) {
    if (conn != &connection) {
      if (conn->OpenSourceMediaStream(adjustableMediaFormats, sessionID)) 
        startedOne = TRUE;
    }
  }

  return startedOne;
}


void OpalCall::CloseMediaStreams()
{
  for (PSafePtr<OpalConnection> conn(connectionsActive, PSafeReadOnly); conn != NULL; ++conn) 
    conn->CloseMediaStreams();
}


void OpalCall::RemoveMediaStreams()
{
  for (PSafePtr<OpalConnection> conn(connectionsActive, PSafeReadOnly); conn != NULL; ++conn) 
    conn->RemoveMediaStreams();
}


BOOL OpalCall::PatchMediaStreams(const OpalConnection & connection,
                                 OpalMediaStream & source)
{
  PTRACE(3, "Call\tPatchMediaStreams " << connection);

  PSafeLockReadOnly lock(*this);
  if (isClearing || !lock.IsLocked())
    return FALSE;

  OpalMediaPatch * patch = NULL;

  {
    // handle RTP payload translation
    RTP_DataFrame::PayloadMapType map;
    for (PSafePtr<OpalConnection> conn(connectionsActive, PSafeReadOnly); conn != NULL; ++conn) {
      if (conn != &connection) {
        map = conn->GetRTPPayloadMap();
      }
    }
    if (map.size() == 0)
      map = connection.GetRTPPayloadMap();

    for (PSafePtr<OpalConnection> conn(connectionsActive, PSafeReadOnly); conn != NULL; ++conn) {
      if (conn != &connection) {
        OpalMediaStream * sink = conn->OpenSinkMediaStream(source);
        if (sink == NULL)
          return FALSE;
        if (source.RequiresPatch()) {
          if (patch == NULL) {
            patch = manager.CreateMediaPatch(source, source.RequiresPatchThread());
            if (patch == NULL)
              return FALSE;
          }
          patch->AddSink(sink, map);
        }
      }
    }
  }

  // if a patch was created, make sure the callback is called
  if (patch) {
    for (PSafePtr<OpalConnection> conn(connectionsActive, PSafeReadOnly); conn != NULL; ++conn) 
      conn->OnPatchMediaStream(conn == &connection, *patch);
  }
  
  return TRUE;
}


void OpalCall::OnRTPStatistics(const OpalConnection & /*connection*/, const RTP_Session & /*session*/)
{
}


BOOL OpalCall::IsMediaBypassPossible(const OpalConnection & connection,
                                     unsigned sessionID) const
{
  PTRACE(3, "Call\tIsMediaBypassPossible " << connection << " session " << sessionID);

  BOOL ok = FALSE;

  for (PSafePtr<OpalConnection> conn(connectionsActive, PSafeReadOnly); conn != NULL; ++conn) {
    if (conn != &connection) {
      ok = manager.IsMediaBypassPossible(connection, *conn, sessionID);
      break;
    }
  }

  return ok;
}


void OpalCall::OnUserInputString(OpalConnection & connection, const PString & value)
{
  for (PSafePtr<OpalConnection> conn(connectionsActive, PSafeReadOnly); conn != NULL; ++conn) {
    if (conn != &connection)
      conn->SendUserInputString(value);
    else
      connection.SetUserInput(value);
  }
}


void OpalCall::OnUserInputTone(OpalConnection & connection,
                               char tone,
                               int duration)
{
  for (PSafePtr<OpalConnection> conn(connectionsActive, PSafeReadOnly); conn != NULL; ++conn) {
    if (conn != &connection)
      conn->SendUserInputTone(tone, duration);
    else {
      if (duration > 0 && tone != ' ')
        connection.OnUserInputString(tone);
    }
  }
}


void OpalCall::OnReleased(OpalConnection & connection)
{
  PTRACE(3, "Call\tOnReleased " << connection);

  SetCallEndReason(connection.GetCallEndReason());

  connectionsActive.Remove(&connection);

  // A call will evaporate when one connection left, at some point this is
  // to be changes so can have "parked" connections. 
  if(connectionsActive.GetSize() == 1)
  {
    PSafePtr<OpalConnection> last = connectionsActive.GetAt(0, PSafeReference);
    if (last != NULL)
      last->Release(connection.GetCallEndReason());
  }
  if (connectionsActive.IsEmpty()) {
    OnCleared();
    manager.activeCalls.RemoveAt(GetToken());
  }
}

BOOL OpalCall::StartRecording(const PFilePath & fn)
{
  // create the mixer entry
  if (!manager.GetRecordManager().Open(myToken, fn))
    return FALSE;

  // tell each connection to start sending data
  for (PSafePtr<OpalConnection> connection(connectionsActive, PSafeReadWrite); connection != NULL; ++connection)
    connection->EnableRecording();

  return TRUE;
}

void OpalCall::StopRecording()
{
  // tell each connection to stop sending data
  for (PSafePtr<OpalConnection> connection(connectionsActive, PSafeReadWrite); connection != NULL; ++connection)
    connection->DisableRecording();

  manager.GetRecordManager().Close(myToken);
}

void OpalCall::OnStopRecordAudio(const PString & callToken)
{
  manager.GetRecordManager().CloseStream(myToken, callToken);
}

/////////////////////////////////////////////////////////////////////////////
