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
 * Revision 1.2002  2001/08/01 05:29:19  robertj
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
        activeConnections[idx].SetAnswerResponse(OpalConnection::AnswerCallNow);
      default :
        return;
    }
  }

  OnEstablished();
}


void OpalCall::OnEstablished()
{
  manager.OnEstablished(*this);
}


void OpalCall::Clear(OpalConnection::CallEndReason reason, PSyncPoint * sync)
{
  PTRACE(3, "Call\tClearing " << *this << " reason=" << reason);

  inUseFlag.Wait();

  while (activeConnections.GetSize() > 0) {
    activeConnections[0].SetCallEndReason(reason);
    garbageConnections.Append(activeConnections.RemoveAt(0));
  }

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

  inUseFlag.Wait();

  for (PINDEX i = 0; i < activeConnections.GetSize(); i++) {
    OpalConnection & conn = activeConnections[i];
    if (&connection != &conn)
      conn.SetAlerting(connection.GetRemotePartyName());
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

  OpenSourceMediaStreams(connection, connection.GetMediaFormats(), OpalMediaFormat::DefaultAudioSessionID);
  if (manager.CanAutoStartTransmitVideo())
    OpenSourceMediaStreams(connection, connection.GetMediaFormats(), OpalMediaFormat::DefaultVideoSessionID);
}


void OpalCall::OnReleased(OpalConnection & connection)
{
  PTRACE(3, "Call\tOnReleased " << connection);

  inUseFlag.Wait();
  if (activeConnections.GetSize() == 1) {
    activeConnections[0].SetCallEndReason(connection.GetCallEndReason());
    garbageConnections.Append(activeConnections.RemoveAt(0));
  }
  inUseFlag.Signal();

  // Signal the background threads that there is some stuff to process.
  manager.SignalGarbageCollector();
}


void OpalCall::Release(OpalConnection * connection)
{
  PAssertNULL(connection);

  PTRACE(3, "Call\tReleasing connection " << *connection);

  inUseFlag.Wait();
  if (activeConnections.Remove(connection))
    garbageConnections.Append(connection);
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

  PTRACE(3, "Call\tGetMediaFormats " << connection << '\n'
         << setfill('\n') << formats << setfill(' '));

  return formats;
}


BOOL OpalCall::OpenSourceMediaStreams(const OpalConnection & connection,
                                      const OpalMediaFormatList & mediaFormats,
                                      unsigned sessionID)
{
  PTRACE(3, "Call\tOpenSourceMediaStreams " << connection);

  BOOL startedOne = FALSE;

  inUseFlag.Wait();

  for (PINDEX i = 0; i < activeConnections.GetSize(); i++) {
    OpalConnection & conn = activeConnections[i];
    if (&connection != &conn) {
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

  if (patchedOne)
    patch->Resume();

  return patchedOne;
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
