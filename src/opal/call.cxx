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
 * Revision 1.2010  2002/01/14 02:23:00  robertj
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

OpalCall::OpalCall(OpalManager & mgr)
  : manager(mgr),
    myToken(manager.GetNextCallToken())
{
  manager.AttachCall(this);

  callEndReason = OpalNumCallEndReasons;

  activeConnections.DisallowDeleteObjects();
  garbageConnections.DisallowDeleteObjects();

  PTRACE(3, "Call\tCreated " << *this);
}


OpalCall::~OpalCall()
{
  PTRACE(3, "Call\t" << *this << " destroyed.");
}


void OpalCall::PrintOn(ostream & strm) const
{
  strm << "Call[" << myToken << ']';
}


void OpalCall::AddConnection(OpalConnection * connection)
{
  PAssertNULL(connection);

  inUseFlag.Wait();
  activeConnections.Append(connection);
  inUseFlag.Signal();
}


void OpalCall::CheckEstablished()
{
  PINDEX idx = activeConnections.GetSize();
  if (idx < 2)
    return;

  while (idx-- > 0) {
    switch (activeConnections[idx].GetPhase()) {
      case OpalConnection::EstablishedPhase :
        break;
      case OpalConnection::AlertingPhase :
        activeConnections[idx].SetConnected();
      default :
        return;
    }
  }

  OnEstablished();
}


void OpalCall::OnEstablished()
{
  manager.OnEstablishedCall(*this);
}


void OpalCall::SetCallEndReason(OpalCallEndReason reason)
{
  // Only set reason if not already set to something
  if (callEndReason == OpalNumCallEndReasons)
    callEndReason = reason;
}


void OpalCall::Clear(OpalCallEndReason reason, PSyncPoint * sync)
{
  PTRACE(3, "Call\tClearing " << *this << " reason=" << reason);

  inUseFlag.Wait();

  SetCallEndReason(reason);

  while (activeConnections.GetSize() > 0)
    InternalReleaseConnection(0, reason);

  inUseFlag.Signal();

  // Signal the background threads that there is some stuff to process.
  manager.SignalGarbageCollector();

  if (sync != NULL)
    sync->Wait();
}


void OpalCall::OnCleared()
{
  manager.OnClearedCall(*this);
}


void OpalCall::OnAlerting(OpalConnection & connection)
{
  PTRACE(3, "Call\tOnAlerting " << connection);

  BOOL hasMedia = connection.GetMediaStream(OpalMediaFormat::DefaultAudioSessionID, TRUE) != NULL;

  inUseFlag.Wait();

  for (PINDEX i = 0; i < activeConnections.GetSize(); i++) {
    OpalConnection & conn = activeConnections[i];
    if (&connection != &conn)
      conn.SetAlerting(connection.GetRemotePartyName(), hasMedia);
  }

  inUseFlag.Signal();
}


void OpalCall::OnConnected(OpalConnection & connection)
{
  PTRACE(3, "Call\tOnConnected " << connection);

  inUseFlag.Wait();

  for (PINDEX i = 0; i < activeConnections.GetSize(); i++) {
    OpalConnection & conn = activeConnections[i];
    if (&connection != &conn)
      conn.SetConnected();
  }

  inUseFlag.Signal();

  BOOL createdOne = OpenSourceMediaStreams(connection.GetMediaFormats(), OpalMediaFormat::DefaultAudioSessionID);
  if (manager.CanAutoStartTransmitVideo()) {
    if (OpenSourceMediaStreams(connection.GetMediaFormats(), OpalMediaFormat::DefaultVideoSessionID))
      createdOne = TRUE;
  }

  if (createdOne) {
    inUseFlag.Wait();

    for (PINDEX i = 0; i < activeConnections.GetSize(); i++)
      activeConnections[i].StartMediaStreams();

    inUseFlag.Signal();
  }
}


void OpalCall::OnReleased(OpalConnection & connection)
{
  PTRACE(3, "Call\tOnReleased " << connection);

  inUseFlag.Wait();

  SetCallEndReason(connection.GetCallEndReason());

  // A call will evaporate when one connection left, at some point this is
  // to be changes so can have "parked" connections.
  if (activeConnections.GetSize() == 1)
    InternalReleaseConnection(0, connection.GetCallEndReason());

  inUseFlag.Signal();

  // Signal the background threads that there is some stuff to process.
  manager.SignalGarbageCollector();
}


void OpalCall::Release(OpalConnection * connection)
{
  PAssertNULL(connection);

  PTRACE(3, "Call\tReleasing connection " << *connection);

  inUseFlag.Wait();
  InternalReleaseConnection(activeConnections.GetObjectsIndex(connection), EndedByLocalUser);
  inUseFlag.Signal();

  // Signal the background threads that there is some stuff to process.
  manager.SignalGarbageCollector();
}


OpalMediaFormatList OpalCall::GetMediaFormats(const OpalConnection & connection)
{
  PINDEX i;
  OpalMediaFormatList formats;

  inUseFlag.Wait();

  BOOL first = TRUE;

  for (i = 0; i < activeConnections.GetSize(); i++) {
    OpalConnection & conn = activeConnections[i];
    if (&connection != &conn) {
      OpalMediaFormatList list = conn.GetMediaFormats();
      if (first) {
        formats = list;
        first = FALSE;
      }
      else {
        formats.MakeUnique();
        for (PINDEX j = 0; j < formats.GetSize(); j++) {
          if (list.GetValuesIndex(formats[j]) == P_MAX_INDEX)
            formats.RemoveAt(j--);
        }
      }
    }
  }

  inUseFlag.Signal();

  for (i = 0; i < formats.GetSize(); i++) {
    OpalMediaFormatList srcFormats = OpalTranscoder::GetSourceFormats(formats[i]);
    for (PINDEX j = 0; j < srcFormats.GetSize(); j++) {
      if (OpalTranscoder::GetDestinationFormats(srcFormats[j]).GetSize() > 0)
        formats += srcFormats[j];
    }
  }

  connection.AdjustMediaFormats(formats);

  PTRACE(3, "Call\tGetMediaFormats for " << connection << '\n'
         << setfill('\n') << formats << setfill(' '));

  return formats;
}


BOOL OpalCall::OpenSourceMediaStreams(const OpalMediaFormatList & mediaFormats,
                                      unsigned sessionID,
                                      const OpalConnection * connection)
{
  PTRACE(2, "Call\tOpenSourceMediaStreams for session " << sessionID
         << " with media " << setfill(',') << mediaFormats << setfill(' '));
  PTRACE_IF(3, connection != NULL, "Call\tExcluding " << *connection);

  BOOL startedOne = FALSE;

  inUseFlag.Wait();

  for (PINDEX i = 0; i < activeConnections.GetSize(); i++) {
    OpalConnection & conn = activeConnections[i];
    if (connection != &conn) {
      if (conn.OpenSourceMediaStream(mediaFormats, sessionID))
        startedOne = TRUE;
    }
  }

  inUseFlag.Signal();

  return startedOne;
}


BOOL OpalCall::PatchMediaStreams(const OpalConnection & connection,
                                 OpalMediaStream & source)
{
  PTRACE(3, "Call\tPatchMediaStreams " << connection);

  BOOL patchedOne = FALSE;

  OpalMediaPatch * patch = manager.CreateMediaPatch(source);

  inUseFlag.Wait();

  for (PINDEX i = 0; i < activeConnections.GetSize(); i++) {
    OpalConnection & conn = activeConnections[i];
    if (&connection != &conn) {
      OpalMediaStream * sink = conn.OpenSinkMediaStream(source);
      if (sink != NULL) {
        patchedOne = TRUE;
        patch->AddSink(sink);
      }
    }
  }

  inUseFlag.Signal();

  return patchedOne;
}


void OpalCall::InternalReleaseConnection(PINDEX activeIndex, OpalCallEndReason reason)
{
  if (activeIndex >= activeConnections.GetSize())
    return;

  OpalConnection * connection = (OpalConnection *)activeConnections.RemoveAt(activeIndex);
  garbageConnections.Append(connection);
  connection->GetEndPoint().RemoveConnection(connection);
  connection->SetCallEndReason(reason);
}


BOOL OpalCall::GarbageCollection()
{
  inUseFlag.Wait();

  while (garbageConnections.GetSize() > 0) {
    OpalConnection * connection = (OpalConnection *)garbageConnections.RemoveAt(0);

    inUseFlag.Signal();

    // Now, outside the callsMutex, we can clean up calls
    if (connection->OnReleased())
      delete connection;

    inUseFlag.Wait();
  }

  inUseFlag.Signal();

  // Going out of scope does the delete of all the calls
  return activeConnections.IsEmpty();
}


/////////////////////////////////////////////////////////////////////////////
