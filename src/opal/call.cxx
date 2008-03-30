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
 * $Revision$
 * $Author$
 * $Date$
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
  , isEstablished(PFalse)
  , isClearing(PFalse)
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

  isClearing = PTrue;

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

  if (!LockReadWrite())
    return;

  if (endCallSyncPoint != NULL) {
    endCallSyncPoint->Signal();
    endCallSyncPoint = NULL;
  }

  UnlockReadWrite();
}


PBoolean OpalCall::OnSetUp(OpalConnection & connection)
{
  PTRACE(3, "Call\tOnSetUp " << connection);

  PBoolean ok = PFalse;

  if (isClearing || !LockReadWrite())
    return PFalse;

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


PBoolean OpalCall::OnAlerting(OpalConnection & connection)
{
  PTRACE(3, "Call\tOnAlerting " << connection);

  if (isClearing || !LockReadWrite())
    return PFalse;

  partyB = connection.GetRemotePartyName();

  UnlockReadWrite();


  PBoolean hasMedia = connection.GetMediaStream(OpalMediaFormat::DefaultAudioSessionID, PTrue) != NULL;

  PBoolean ok = PFalse;

  for (PSafePtr<OpalConnection> conn(connectionsActive, PSafeReadOnly); conn != NULL; ++conn) {
    if (conn != &connection) {
      if (conn->SetAlerting(connection.GetRemotePartyName(), hasMedia))
        ok = PTrue;
    }
  }

  return ok;
}

OpalConnection::AnswerCallResponse OpalCall::OnAnswerCall(OpalConnection & PTRACE_PARAM(connection),
                                                          const PString & PTRACE_PARAM(caller))
{
  PTRACE(3, "Call\tOnAnswerCall " << connection << " caller \"" << caller << '"');
  return OpalConnection::AnswerCallPending;
}


PBoolean OpalCall::OnConnected(OpalConnection & connection)
{
  PTRACE(3, "Call\tOnConnected " << connection);

  if (isClearing || !LockReadOnly())
    return false;

  bool ok = connectionsActive.GetSize() == 1 && !partyB.IsEmpty();

  UnlockReadOnly();

  if (ok) {
    if (!manager.MakeConnection(*this, partyB, NULL, 0, connection.GetStringOptions()))
      connection.Release(OpalConnection::EndedByNoUser);
    return OnSetUp(connection);
  }

  if (!LockReadOnly())
    return false;

  for (PSafePtr<OpalConnection> conn(connectionsActive, PSafeReadOnly); conn != NULL; ++conn) {
    if (conn != &connection) {
      if (conn->SetConnected())
        ok = true;
    }
  }

  UnlockReadOnly();
  
  return ok;
}


PBoolean OpalCall::OnEstablished(OpalConnection & connection)
{
  PTRACE(3, "Call\tOnEstablished " << connection);

  PSafeLockReadWrite lock(*this);
  if (isClearing || !lock.IsLocked())
    return false;

  if (isEstablished)
    return true;

  if (connectionsActive.GetSize() < 2)
    return false;

  connection.StartMediaStreams();

  for (PSafePtr<OpalConnection> conn(connectionsActive, PSafeReference); conn != NULL; ++conn) {
    if (conn->GetPhase() != OpalConnection::EstablishedPhase)
      return false;
  }

  isEstablished = true;
  OnEstablishedCall();

  return true;
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


void OpalCall::Hold()
{
  PTRACE(3, "Call\tSetting to On Hold");

  for (PSafePtr<OpalConnection> conn(connectionsActive, PSafeReadOnly); conn != NULL; ++conn)
    conn->HoldConnection();
}


void OpalCall::Retrieve()
{
  PTRACE(3, "Call\tRetrieve from On Hold");

  for (PSafePtr<OpalConnection> conn(connectionsActive, PSafeReadOnly); conn != NULL; ++conn)
    conn->RetrieveConnection();
}


bool OpalCall::IsOnHold() const
{
  for (PSafePtr<OpalConnection> conn(connectionsActive, PSafeReadOnly); conn != NULL; ++conn) {
    if (conn->IsConnectionOnHold())
      return true;
  }

  return false;
}


OpalMediaFormatList OpalCall::GetMediaFormats(const OpalConnection & connection,
                                              PBoolean includeSpecifiedConnection)
{
  OpalMediaFormatList commonFormats;

  PBoolean first = PTrue;

  for (PSafePtr<OpalConnection> conn(connectionsActive, PSafeReadOnly); conn != NULL; ++conn) {
    if (includeSpecifiedConnection || conn != &connection) {
      OpalMediaFormatList possibleFormats = OpalTranscoder::GetPossibleFormats(conn->GetMediaFormats());
      if (first) {
        commonFormats = possibleFormats;
        first = PFalse;
      }
      else {
        // Want intersection of the possible formats for all connections.
        for (OpalMediaFormatList::iterator format = commonFormats.begin(); format != commonFormats.end(); ) {
          if (possibleFormats.HasFormat(*format))
            ++format;
          else
            commonFormats.erase(format++);
        }
      }
    }
  }

  connection.AdjustMediaFormats(commonFormats);

  PTRACE(4, "Call\tGetMediaFormats for " << connection << '\n'
         << setfill('\n') << commonFormats << setfill(' '));

  return commonFormats;
}

PBoolean OpalCall::OpenSourceMediaStreams(OpalConnection & connection, unsigned sessionID, const OpalMediaFormatList & preselectedFormats)
{
  PSafeLockReadOnly lock(*this);
  if (isClearing || !lock.IsLocked())
    return false;

  // Check if already done
  if (connection.GetMediaStream(sessionID, true) != NULL) {
    PTRACE(3, "Call\tOpenSourceMediaStreams (already opened) for session " << sessionID << " on " << connection);
    return true;
  }

  PTRACE(3, "Call\tOpenSourceMediaStreams for session " << sessionID << " on " << connection);

  // handle RTP payload translation
  RTP_DataFrame::PayloadMapType map;
  for (PSafePtr<OpalConnection> conn(connectionsActive, PSafeReadOnly); conn != NULL; ++conn) {
    const RTP_DataFrame::PayloadMapType & connMap = conn->GetRTPPayloadMap();
    if (conn != &connection)
      map.insert(connMap.begin(), connMap.end());
    else {
      for (RTP_DataFrame::PayloadMapType::const_iterator it = connMap.begin(); it != connMap.end(); ++it)
        map[it->second] = it->first;
    }
  }

  // Create the sinks and patch if needed
  bool startedOne = false;
  OpalMediaPatch * patch = NULL;
  OpalMediaStreamPtr source;
  OpalMediaFormat sourceFormat, sinkFormat;

  // Reorder destinations so we give preference to symmetric codecs
  OpalMediaStreamPtr otherDirection = connection.GetMediaStream(sessionID, false);
  if (otherDirection != NULL)
    sinkFormat = otherDirection->GetMediaFormat();

  for (PSafePtr<OpalConnection> conn(connectionsActive, PSafeReadOnly); conn != NULL; ++conn) {
    if (conn != &connection) {
      OpalMediaFormatList sinkMediaFormats = conn->GetMediaFormats();
      sinkMediaFormats.Reorder(sinkFormat.GetName()); // Preferential treatment to format already in use

      OpalMediaFormatList sourceMediaFormats;
      if (sourceFormat.IsValid())
        sourceMediaFormats = sourceFormat; // Use the source format already established
      else if (preselectedFormats.IsEmpty())
        sourceMediaFormats = connection.GetMediaFormats();
      else
        sourceMediaFormats = preselectedFormats;

      if (!SelectMediaFormats(sessionID,
                              sourceMediaFormats,
                              sinkMediaFormats,
                              conn->GetLocalMediaFormats(),
                              sourceFormat,
                              sinkFormat))
        return false;

      // Finally have the negotiated formats, open the streams
      if (source == NULL) {
        source = connection.OpenMediaStream(sourceFormat, sessionID, true);
        if (source == NULL)
          return false;
      }

      OpalMediaStreamPtr sink = conn->OpenMediaStream(sinkFormat, sessionID, false);
      if (sink != NULL) {
        startedOne = true;
        if (patch == NULL) {
          patch = manager.CreateMediaPatch(*source, source->RequiresPatchThread());
          if (patch == NULL)
            return false;
        }
        patch->AddSink(sink, map);
      }
    }
  }

  if (!startedOne) {
    connection.RemoveMediaStream(*source);
    return false;
  }

  if (patch != NULL) {
    // if a patch was created, make sure the callback is called, just once per connection
    for (PSafePtr<OpalConnection> conn(connectionsActive, PSafeReadOnly); conn != NULL; ++conn)
      conn->OnPatchMediaStream(conn == &connection, *patch);
  }

  return true;
}


bool OpalCall::SelectMediaFormats(unsigned sessionID,
                                  const OpalMediaFormatList & srcFormats,
                                  const OpalMediaFormatList & dstFormats,
                                  const OpalMediaFormatList & allFormats,
                                  OpalMediaFormat & srcFormat,
                                  OpalMediaFormat & dstFormat) const
{
  if (OpalTranscoder::SelectFormats(sessionID, srcFormats, dstFormats, allFormats, srcFormat, dstFormat)) {
    PTRACE(3, "Call\tSelected media formats " << srcFormat << " -> " << dstFormat);
    return true;
  }

  PTRACE(2, "Call\tSelectMediaFormats session " << sessionID
        << ", could not find compatible media format:\n"
            "  source formats=" << setfill(',') << srcFormats << "\n"
            "   sink  formats=" << dstFormats << setfill(' '));
  return false;
}


void OpalCall::CloseMediaStreams()
{
  for (PSafePtr<OpalConnection> conn(connectionsActive, PSafeReadOnly); conn != NULL; ++conn) 
    conn->CloseMediaStreams();
}


void OpalCall::OnRTPStatistics(const OpalConnection & /*connection*/, const RTP_Session & /*session*/)
{
}


PBoolean OpalCall::IsMediaBypassPossible(const OpalConnection & connection,
                                     unsigned sessionID) const
{
  PTRACE(3, "Call\tIsMediaBypassPossible " << connection << " session " << sessionID);

  PBoolean ok = PFalse;

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
  bool reprocess = duration > 0 && tone != ' ';

  for (PSafePtr<OpalConnection> conn(connectionsActive, PSafeReadOnly); conn != NULL; ++conn) {
    if (conn != &connection && conn->SendUserInputTone(tone, duration))
      reprocess = false;
  }

  if (reprocess)
    connection.OnUserInputString(tone);
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

PBoolean OpalCall::StartRecording(const PFilePath & fn)
{
  // create the mixer entry
  if (!manager.GetRecordManager().Open(myToken, fn))
    return PFalse;

  // tell each connection to start sending data
  for (PSafePtr<OpalConnection> connection(connectionsActive, PSafeReadWrite); connection != NULL; ++connection)
    connection->EnableRecording();

  return PTrue;
}


bool OpalCall::IsRecording() const
{
  return manager.GetRecordManager().IsOpen();
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
