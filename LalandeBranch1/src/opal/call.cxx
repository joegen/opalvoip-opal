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

#include <opal/buildopts.h>

#include <opal/call.h>

#include <opal/manager.h>
#include <opal/endpoint.h>
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

  PSafePtr<OpalConnection> connection;
  while (EnumerateConnections(connection, PSafeReadWrite))
    connection->Release(reason);
}


void OpalCall::OnCleared()
{
  manager.OnClearedCall(*this);
  manager.GetRecordManager().Close(myToken);

  if (!LockReadWrite())
    return;

  if (endCallSyncPoint != NULL) {
    endCallSyncPoint->Signal();
    endCallSyncPoint = NULL;
  }

  UnlockReadWrite();
}


void OpalCall::OnNewConnection(OpalConnection & connection)
{
  manager.OnNewConnection(connection);
  SetPartyNames();
}


PBoolean OpalCall::OnSetUp(OpalConnection & connection)
{
  PTRACE(3, "Call\tOnSetUp " << connection);

  if (isClearing)
    return false;

  SetPartyNames();

  bool ok = false;

  PSafePtr<OpalConnection> otherConnection;
  while (EnumerateConnections(otherConnection, PSafeReadWrite, &connection)) {
    if (otherConnection->SetUpConnection() && otherConnection->OnSetUpConnection())
      ok = true;
  }

  return ok;
}


void OpalCall::OnProceeding(OpalConnection & PTRACE_PARAM(connection))
{
  PTRACE(3, "Call\tOnProceeding " << connection);
}


PBoolean OpalCall::OnAlerting(OpalConnection & connection)
{
  PTRACE(3, "Call\tOnAlerting " << connection);

  if (isClearing)
    return false;

  PBoolean hasMedia = connection.GetMediaStream(OpalMediaType::Audio(), true) != NULL;

  bool ok = false;

  PSafePtr<OpalConnection> otherConnection;
  while (EnumerateConnections(otherConnection, PSafeReadWrite, &connection)) {
    if (otherConnection->SetAlerting(connection.GetRemotePartyName(), hasMedia))
      ok = true;
  }

  SetPartyNames();

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

  bool ok = connectionsActive.GetSize() == 1 && !m_partyB.IsEmpty();

  UnlockReadOnly();

  if (ok) {
    if (!manager.MakeConnection(*this, m_partyB, NULL, 0,
                                const_cast<OpalConnection::StringOptions *>(&connection.GetStringOptions())))
      connection.Release(OpalConnection::EndedByNoUser);
    return OnSetUp(connection);
  }

  PSafePtr<OpalConnection> otherConnection;
  while (EnumerateConnections(otherConnection, PSafeReadWrite, &connection)) {
    if (otherConnection->GetPhase() >= OpalConnection::ConnectedPhase ||
        otherConnection->SetConnected())
      ok = true;
  }

  SetPartyNames();

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

  PSafePtr<OpalConnection> otherConnection;
  EnumerateConnections(otherConnection, PSafeReference, &connection);
  return otherConnection;
}


bool OpalCall::Hold()
{
  PTRACE(3, "Call\tSetting to On Hold");

  bool ok = false;

  PSafePtr<OpalConnection> connection;
  while (EnumerateConnections(connection, PSafeReadWrite)) {
    if (connection->HoldConnection())
      ok = true;
  }

  return ok;
}


bool OpalCall::Retrieve()
{
  PTRACE(3, "Call\tRetrieve from On Hold");

  bool ok = false;

  PSafePtr<OpalConnection> connection;
  while (EnumerateConnections(connection, PSafeReadWrite)) {
    if (connection->RetrieveConnection())
      ok = true;
  }

  return ok;
}


bool OpalCall::IsOnHold() const
{
  PSafePtr<OpalConnection> connection;
  while (EnumerateConnections(connection, PSafeReadOnly)) {
    if (connection->IsConnectionOnHold())
      return true;
  }

  return false;
}


bool OpalCall::Transfer(OpalConnection & connection, const PString & newAddress)
{
  if (newAddress.NumCompare(connection.GetPrefixName()+':') == EqualTo)
    return connection.TransferConnection(newAddress);

  PSafePtr<OpalConnection> connectionToKeep;
  EnumerateConnections(connectionToKeep, PSafeReference, &connection);

  if (!manager.MakeConnection(*this, newAddress))
    return false;

  connection.Release(OpalConnection::EndedByCallForwarded);

  // Close streams, but as we Released above should not do re-INVITE/OLC
  connection.CloseMediaStreams();

  // Restart with new connection
  return OnSetUp(*connectionToKeep);
}


OpalMediaFormatList OpalCall::GetMediaFormats(const OpalConnection & connection,
                                              PBoolean includeSpecifiedConnection)
{
  OpalMediaFormatList commonFormats;

  PBoolean first = PTrue;

  PSafePtr<OpalConnection> otherConnection;
  while (EnumerateConnections(otherConnection, PSafeReadOnly, includeSpecifiedConnection ? NULL : &connection)) {
    OpalMediaFormatList possibleFormats = OpalTranscoder::GetPossibleFormats(otherConnection->GetMediaFormats());
    otherConnection->AdjustMediaFormats(possibleFormats);
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

  connection.AdjustMediaFormats(commonFormats);

  PTRACE(4, "Call\tGetMediaFormats for " << connection << '\n'
         << setfill('\n') << commonFormats << setfill(' '));

  return commonFormats;
}


PBoolean OpalCall::OpenSourceMediaStreams(OpalConnection & connection, 
                                     const OpalMediaType & mediaType,
                                                  unsigned sessionID, 
                                   const OpalMediaFormat & preselectedFormat)
{
  PSafeLockReadWrite lock(*this);
  if (isClearing || !lock.IsLocked())
    return false;

  // Check if already done
  OpalMediaStreamPtr sinkStream;
  OpalMediaStreamPtr sourceStream = connection.GetMediaStream(sessionID, true);
  if (sourceStream != NULL) {
    OpalMediaPatch * patch = sourceStream->GetPatch();
    if (patch != NULL)
      sinkStream = patch->GetSink();
    if (sourceStream->GetMediaFormat() == preselectedFormat ||
        (sinkStream != NULL && sinkStream->GetMediaFormat() == preselectedFormat)) {
      PTRACE(3, "Call\tOpenSourceMediaStreams (already opened) for session " << sessionID << " on " << connection);
      return true;
    }
    if (sinkStream == NULL && sourceStream->IsOpen()) {
      PTRACE(3, "Call\tOpenSourceMediaStreams (is opening) for session " << sessionID << " on " << connection);
      return true;
    }
  }

  PTRACE(3, "Call\tOpenSourceMediaStreams " << (sourceStream != NULL ? "replacing" : "opening")
         << ' ' << mediaType << " session " << sessionID << " on " << connection);
  sourceStream.SetNULL();

  // Create the sinks and patch if needed
  bool startedOne = false;
  OpalMediaPatch * patch = NULL;
  OpalMediaFormat sourceFormat, sinkFormat;

  // Reorder destinations so we give preference to symmetric codecs
  if (sinkStream != NULL)
    sinkFormat = sinkStream->GetMediaFormat();

  PSafePtr<OpalConnection> otherConnection;
  while (EnumerateConnections(otherConnection, PSafeReadWrite, &connection)) {
    OpalMediaFormatList sinkMediaFormats = otherConnection->GetMediaFormats();
    if (preselectedFormat.IsValid()  && sinkMediaFormats.HasFormat(preselectedFormat))
      sinkMediaFormats = preselectedFormat;
    else
      sinkMediaFormats.Reorder(sinkFormat.GetName()); // Preferential treatment to format already in use

    OpalMediaFormatList sourceMediaFormats;
    if (sourceFormat.IsValid())
      sourceMediaFormats = sourceFormat; // Use the source format already established
    else {
      OpalMediaFormatList connectionMediaFormats = connection.GetMediaFormats();
      if (preselectedFormat.IsValid()  && connectionMediaFormats.HasFormat(preselectedFormat))
        sourceMediaFormats = preselectedFormat;
      else {
        // Only include media formats that are of the same media type
        for (OpalMediaFormatList::iterator i = connectionMediaFormats.begin(); i != connectionMediaFormats.end(); ++i) {
          if (i->GetMediaType() == mediaType)
            sourceMediaFormats += *i;
        }
      }
    }

    // Get other media directions format so we give preference to symmetric codecs
    OpalMediaStreamPtr otherDirection = otherConnection->GetMediaStream(sessionID, true);
    if (otherDirection != NULL) {
      PString priorityFormat = otherDirection->GetMediaFormat().GetName();
      sourceMediaFormats.Reorder(priorityFormat);
      sinkMediaFormats.Reorder(priorityFormat);
    }

    if (!SelectMediaFormats(sourceMediaFormats,
                            sinkMediaFormats,
                            connection.GetLocalMediaFormats(),
                            sourceFormat,
                            sinkFormat))
      return false;

    if (sessionID == 0)
      sessionID = mediaType.GetDefinition()->GetDefaultSessionId();

    // Finally have the negotiated formats, open the streams
    if (sourceStream == NULL) {
      sourceStream = connection.OpenMediaStream(sourceFormat, sessionID, true);
      if (sourceStream == NULL)
        return false;
    }

    sinkStream = otherConnection->OpenMediaStream(sinkFormat, sessionID, false);
    if (sinkStream != NULL) {
      startedOne = true;
      if (patch == NULL) {
        patch = manager.CreateMediaPatch(*sourceStream, sinkStream->RequiresPatchThread(sourceStream) &&
                                                        sourceStream->RequiresPatchThread(sinkStream));
        if (patch == NULL)
          return false;
      }
      patch->AddSink(sinkStream);
    }
  }

  if (!startedOne) {
    if (sourceStream != NULL)
      connection.RemoveMediaStream(*sourceStream);
    return false;
  }

  if (patch != NULL) {
    // if a patch was created, make sure the callback is called, just once per connection
    while (EnumerateConnections(otherConnection, PSafeReadWrite))
      otherConnection->OnPatchMediaStream(otherConnection == &connection, *patch);
  }

  return true;
}


bool OpalCall::SelectMediaFormats(const OpalMediaFormatList & srcFormats,
                                  const OpalMediaFormatList & dstFormats,
                                  const OpalMediaFormatList & allFormats,
                                  OpalMediaFormat & srcFormat,
                                  OpalMediaFormat & dstFormat) const
{
  if (OpalTranscoder::SelectFormats(srcFormats, dstFormats, allFormats, srcFormat, dstFormat)) {
    PTRACE(3, "Call\tSelected media formats " << srcFormat << " -> " << dstFormat);
    return true;
  }

  PTRACE(2, "Call\tSelectMediaFormats could not find compatible media format:\n"
            "  source formats=" << setfill(',') << srcFormats << "\n"
            "   sink  formats=" << dstFormats << setfill(' '));
  return false;
}


void OpalCall::CloseMediaStreams()
{
  PSafePtr<OpalConnection> connection;
  while (EnumerateConnections(connection, PSafeReadWrite))
    connection->CloseMediaStreams();
}


void OpalCall::OnRTPStatistics(const OpalConnection & /*connection*/, const RTP_Session & /*session*/)
{
}


PBoolean OpalCall::IsMediaBypassPossible(const OpalConnection & connection,
                                     unsigned sessionID) const
{
  PTRACE(3, "Call\tIsMediaBypassPossible " << connection << " session " << sessionID);

  PSafePtr<OpalConnection> otherConnection;
  return EnumerateConnections(otherConnection, PSafeReadOnly, &connection) &&
         manager.IsMediaBypassPossible(connection, *otherConnection, sessionID);
}


void OpalCall::OnUserInputString(OpalConnection & connection, const PString & value)
{
  PSafePtr<OpalConnection> otherConnection;
  while (EnumerateConnections(otherConnection, PSafeReadWrite)) {
    if (otherConnection != &connection)
      otherConnection->SendUserInputString(value);
    else
      connection.SetUserInput(value);
  }
}


void OpalCall::OnUserInputTone(OpalConnection & connection,
                               char tone,
                               int duration)
{
  bool reprocess = duration > 0 && tone != ' ';

  PSafePtr<OpalConnection> otherConnection;
  while (EnumerateConnections(otherConnection, PSafeReadWrite, &connection)) {
    if (otherConnection->SendUserInputTone(tone, duration))
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
  if (connectionsActive.IsEmpty() && manager.activeCalls.Contains(GetToken())) {
    OnCleared();
    manager.activeCalls.RemoveAt(GetToken());
  }
}


void OpalCall::OnHold(OpalConnection & /*connection*/,
                      bool /*fromRemote*/, 
                      bool /*onHold*/)
{
}


PBoolean OpalCall::StartRecording(const PFilePath & fn, bool mono)
{
  // create the mixer entry
  if (!manager.GetRecordManager().Open(myToken, fn, mono))
    return PFalse;

  // tell each connection to start sending data
  PSafePtr<OpalConnection> connection;
  while (EnumerateConnections(connection, PSafeReadWrite))
    connection->EnableRecording();

  return PTrue;
}


bool OpalCall::IsRecording() const
{
  return manager.GetRecordManager().IsOpen(myToken);
}


void OpalCall::StopRecording()
{
  // tell each connection to stop sending data
  PSafePtr<OpalConnection> connection;
  while (EnumerateConnections(connection, PSafeReadWrite))
    connection->DisableRecording();

  manager.GetRecordManager().Close(myToken);
}


void OpalCall::OnRecordAudio(const PString & streamId, const RTP_DataFrame & frame)
{
  if (!manager.GetRecordManager().WriteAudio(myToken, streamId, frame))
    StopRecording();
}


void OpalCall::OnStopRecordAudio(const PString & streamId)
{
  manager.GetRecordManager().CloseStream(myToken, streamId);
}


bool OpalCall::IsNetworkOriginated() const
{
  PSafePtr<OpalConnection> connection = PSafePtr<OpalConnection>(connectionsActive, PSafeReadOnly);
  return connection == NULL || connection->IsNetworkConnection();
}


void OpalCall::SetPartyNames()
{
  PSafeLockReadWrite lock(*this);
  if (!lock.IsLocked())
    return;

  PSafePtr<OpalConnection> connectionA = connectionsActive.GetAt(0, PSafeReadOnly);
  if (connectionA == NULL)
    return;

  bool networkA = connectionA->IsNetworkConnection();
  if (networkA)
    m_partyA = connectionA->GetRemotePartyURL();
  if (!networkA || m_partyA.IsEmpty())
    m_partyA = connectionA->GetLocalPartyURL();

  PSafePtr<OpalConnection> connectionB = connectionsActive.GetAt(1, PSafeReadOnly);
  if (connectionB == NULL)
    return;

  if (connectionB->IsNetworkConnection()) {
    if (!networkA)
      connectionA->CopyPartyNames(*connectionB);
    m_partyB = connectionB->GetRemotePartyURL();
  }
  else {
    if (networkA) {
      connectionB->CopyPartyNames(*connectionA);
      m_partyB = connectionA->GetCalledPartyURL();
    }
    if (m_partyB.IsEmpty())
      m_partyB = connectionB->GetLocalPartyURL();
  }
}


bool OpalCall::EnumerateConnections(PSafePtr<OpalConnection> & connection,
                                    PSafetyMode mode,
                                    const OpalConnection * skipConnection) const
{
  if (connection == NULL)
    connection = PSafePtr<OpalConnection>(connectionsActive, PSafeReference);
  else {
    connection.SetSafetyMode(PSafeReference);
    ++connection;
  }

  while (connection != NULL) {
    if (connection != skipConnection &&
        connection->GetPhase() < OpalConnection::ReleasingPhase &&
        connection.SetSafetyMode(mode))
      return true;
    ++connection;
  }

  return false;
}


/////////////////////////////////////////////////////////////////////////////
