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
 */

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "call.h"
#endif

#include <opal_config.h>

#include <opal/call.h>

#include <opal/manager.h>
#include <opal/endpoint.h>
#include <opal/patch.h>
#include <opal/transcoders.h>


#define new PNEW

#define PTraceModule() "Call"


/////////////////////////////////////////////////////////////////////////////

OpalCall::OpalCall(OpalManager & mgr)
  : m_manager(mgr)
  , m_token(mgr.GetNextToken('C'))
  , m_networkOriginated(false)
  , m_establishedTime(0)
  , m_isEstablished(false)
  , m_isClearing(false)
  , m_handlingHold(false)
  , m_isCleared(false)
  , m_callEndReason(OpalConnection::NumCallEndReasons)
#if OPAL_HAS_MIXER
  , m_recordManager(NULL)
#endif
#if OPAL_T38_CAPABILITY
  , m_T38SwitchState(e_NotSwitchingT38)
#endif
{
  PTRACE_CONTEXT_ID_NEW();

  m_manager.m_activeCalls.SetAt(m_token, this);

  m_connectionsActive.DisallowDeleteObjects();

#if OPAL_SCRIPT
  PScriptLanguage * script = m_manager.GetScript();
  if (script != NULL) {
    PString tableName = OPAL_SCRIPT_CALL_TABLE_NAME "." + GetToken();
    script->CreateComposite(tableName);
    script->SetFunction(tableName + ".Clear", PCREATE_NOTIFIER(ScriptClear));
    script->Call("OnNewCall", "s", (const char *)GetToken());
  }
#endif

  PTRACE(3, "Created " << *this << " ptr=" << this);
}


OpalCall::~OpalCall()
{
#if OPAL_HAS_MIXER
  delete m_recordManager;
#endif

#if OPAL_SCRIPT
  PScriptLanguage * script = m_manager.GetScript();
  if (script != NULL) {
    script->Call("OnDestroyCall", "s", (const char *)GetToken());
    script->ReleaseVariable(OPAL_SCRIPT_CALL_TABLE_NAME "." + GetToken());
  }
#endif

  PTRACE(3, "Destroyed " << *this << " ptr=" << this);
}


void OpalCall::PrintOn(ostream & strm) const
{
  strm << "Call[" << m_token << ']';
}


void OpalCall::OnEstablishedCall()
{
  PTRACE(3, "Established " << *this);
  m_manager.OnEstablishedCall(*this);
}


void OpalCall::SetCallEndReason(OpalConnection::CallEndReason reason)
{
  // Only set reason if not already set to something
  if (m_callEndReason == OpalConnection::NumCallEndReasons &&
     (reason != OpalConnection::EndedByCallForwarded || m_connectionsActive.GetSize() <= 1))
    m_callEndReason = reason;
}


void OpalCall::Clear(OpalConnection::CallEndReason reason, PSyncPoint * sync)
{
  PTRACE_CONTEXT_ID_PUSH_THREAD(this);

  PTRACE(3, "Clearing " << (sync != NULL ? "(sync) " : "") << *this << " reason=" << reason);

  {
    PSafeLockReadWrite lock(*this);
    if (!lock.IsLocked() || m_isClearing) {
      if (sync != NULL)
        sync->Signal();
      return;
    }

    m_isClearing = true;

    SetCallEndReason(reason);

    if (sync != NULL)
      m_endCallSyncPoint.push_back(sync);

    PINDEX i = m_connectionsActive.GetSize();
    while (i > 0) {
      PSafePtr<OpalConnection> connection = m_connectionsActive.GetAt(--i, PSafeReference);
      if (connection != NULL)
        connection->Release(reason);
    }
  }

  // Outside of lock
  InternalOnClear();
}


void OpalCall::OnReleased(OpalConnection & connection)
{
  PTRACE(3, "OnReleased " << connection);
  m_connectionsActive.Remove(&connection);

  PSafePtr<OpalConnection> other = m_connectionsActive.GetAt(0, PSafeReference);
  if (other != NULL && other->GetPhase() == OpalConnection::ReleasingPhase) {
    PTRACE(4, "Follow on OnReleased to " << *other);
    other->OnReleased();
  }

  InternalOnClear();
}


void OpalCall::InternalOnClear()
{
  if (!m_connectionsActive.IsEmpty())
    return;

  if (m_isCleared.exchange(true))
    return;

  OnCleared();

#if OPAL_HAS_MIXER
  StopRecording();
#endif

  if (LockReadWrite()) {
    while (!m_endCallSyncPoint.empty()) {
      PTRACE(5, "Signalling end call.");
      m_endCallSyncPoint.front()->Signal();
      m_endCallSyncPoint.pop_front();
    }
    UnlockReadWrite();
  }

  m_manager.m_activeCalls.RemoveAt(GetToken());
}


void OpalCall::OnCleared()
{
  m_manager.OnClearedCall(*this);
}


void OpalCall::OnNewConnection(OpalConnection & connection)
{
  if (m_connectionsActive.GetSize() == 1)
    m_networkOriginated = connection.IsNetworkConnection();

  m_manager.OnNewConnection(connection);
  SetPartyNames();
}


PBoolean OpalCall::OnSetUp(OpalConnection & connection)
{
  PTRACE(3, "OnSetUp " << connection);

  if (m_isClearing)
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
  PTRACE(3, "OnProceeding " << connection);
}


void OpalCall::OnAlerting(OpalConnection & connection, bool withMedia)
{
  PTRACE(3, "OnAlerting " << connection);

  if (m_isClearing)
    return;

  PSafePtr<OpalConnection> otherConnection;
  while (EnumerateConnections(otherConnection, PSafeReadWrite, &connection)) {
    if (!otherConnection->SetAlerting(connection.GetRemotePartyName(), withMedia))
      return;
  }

  SetPartyNames();
}


PBoolean OpalCall::OnAlerting(OpalConnection & connection)
{
  connection.OnAlerting(connection.GetMediaStream(OpalMediaType(), true) != NULL);
  return true;
}


OpalConnection::AnswerCallResponse OpalCall::OnAnswerCall(OpalConnection & PTRACE_PARAM(connection),
                                                          const PString & PTRACE_PARAM(caller))
{
  PTRACE(3, "OnAnswerCall " << connection << " caller \"" << caller << '"');
  return OpalConnection::AnswerCallDeferred;
}


PBoolean OpalCall::OnConnected(OpalConnection & connection)
{
  PTRACE(3, "OnConnected " << connection);

  if (m_isClearing || !LockReadOnly())
    return false;

  bool havePartyB = m_connectionsActive.GetSize() == 1 && !m_partyB.IsEmpty();

  UnlockReadOnly();

  if (havePartyB) {
    if (m_manager.MakeConnection(*this, m_partyB, NULL, 0,
                                const_cast<OpalConnection::StringOptions *>(&connection.GetStringOptions())) != NULL)
      return OnSetUp(connection);

    connection.Release(OpalConnection::EndedByNoUser);
    return false;
  }

  bool ok = false;
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
  PTRACE(4, "OnEstablished " << connection);

  PSafeLockReadWrite lock(*this);
  if (m_isClearing || !lock.IsLocked()) {
    PTRACE(3, "OnEstablished " << connection << ", ignored as clearing call.");
    return false;
  }

  if (m_isEstablished)
    return true;

  if (m_connectionsActive.GetSize() < 2) {
    PTRACE(3, "OnEstablished " << connection << ", not enough connections.");
    return false;
  }

  connection.StartMediaStreams();

  for (PSafePtr<OpalConnection> conn(m_connectionsActive, PSafeReference); conn != NULL; ++conn) {
    if (conn->GetPhase() != OpalConnection::EstablishedPhase) {
      PTRACE(3, "OnEstablished " << connection << ", other side not established yet.");
      return false;
  }
  }

  m_isEstablished = true;
  m_establishedTime.SetCurrentTime();
  OnEstablishedCall();

  return true;
}


PSafePtr<OpalConnection> OpalCall::GetOtherPartyConnection(const OpalConnection & connection) const
{
  PSafePtr<OpalConnection> otherConnection;
  EnumerateConnections(otherConnection, PSafeReference, &connection);
#if PTRACING
  static unsigned Level = 4;
  if (PTrace::CanTrace(Level)) {
      ostream & trace = PTRACE_BEGIN(4);
      trace << "GetOtherPartyConnection: connection=" << connection << ", other=";
      if (otherConnection == NULL)
          trace << "<null>";
      else
          trace << *otherConnection;
      trace << PTrace::End;
  }
#endif // PTRACING
  return otherConnection;
}


bool OpalCall::Hold(bool placeOnHold)
{
  PTRACE_CONTEXT_ID_PUSH_THREAD(this);

  PTRACE(3, (placeOnHold ? "Setting to" : "Retrieving from") << " On Hold");

  bool ok = false;

  PSafePtr<OpalConnection> connection;
  while (EnumerateConnections(connection, PSafeReadWrite)) {
    if (!connection->IsNetworkConnection())
      connection->HoldRemote(placeOnHold);
  }
  while (EnumerateConnections(connection, PSafeReadWrite)) {
    if (connection->IsNetworkConnection() && connection->HoldRemote(placeOnHold))
      ok = true;
  }

  return ok;
}


bool OpalCall::IsOnHold(bool fromRemote) const
{
  PSafePtr<OpalConnection> connection;
  while (EnumerateConnections(connection, PSafeReadOnly)) {
    if (connection->IsNetworkConnection() && connection->IsOnHold(fromRemote))
      return true;
  }

  return false;
}


bool OpalCall::Transfer(const PString & newAddress, OpalConnection * connection)
{
  PTRACE_CONTEXT_ID_PUSH_THREAD(this);

  PSafeLockReadWrite lock(*this);
  if (!lock.IsLocked())
    return false;

  PCaselessString prefix;
  PINDEX colon = newAddress.Find(':');
  if (colon != P_MAX_INDEX)
    prefix = newAddress.Left(colon);

  if (connection == NULL) {
    for (PSafePtr<OpalConnection> conn = GetConnection(0); conn != NULL; ++conn) {
      if (prefix == conn->GetPrefixName() && !conn->IsReleased())
        return conn->TransferConnection(newAddress);
    }

    PTRACE(2, "Unable to resolve transfer to \"" << newAddress << '"');
    return false;
  }

  if (connection->IsReleased()) {
    PTRACE(2, "Cannot transfer to released connection " << *connection);
    return false;
  }

  if (prefix == "*")
    return connection->TransferConnection(connection->GetPrefixName() + newAddress.Mid(1));

  if (prefix.IsEmpty() || prefix == connection->GetPrefixName() || m_manager.HasCall(newAddress))
    return connection->TransferConnection(newAddress);

  PTRACE(3, "Transferring " << *connection << " to \"" << newAddress << '"');

  PSafePtr<OpalConnection> connectionToKeep = GetOtherPartyConnection(*connection);
  if (connectionToKeep == NULL)
    return false;

  PSafePtr<OpalConnection> newConnection = m_manager.MakeConnection(*this, newAddress);
  if (newConnection == NULL)
    return false;

  OpalConnection::Phases oldPhase = connection->GetPhase();
  connection->SetPhase(OpalConnection::ForwardingPhase);

  // Restart with new connection
  if (newConnection->SetUpConnection() && newConnection->OnSetUpConnection()) {
    connectionToKeep->AutoStartMediaStreams(true);
    connection->Release(OpalConnection::EndedByCallForwarded, true);
    newConnection->StartMediaStreams();
    connectionToKeep->StartMediaStreams();
    return true;
  }

  newConnection->Release(OpalConnection::EndedByTemporaryFailure);
  connection->SetPhase(oldPhase);
  return false;
}


OpalMediaFormatList OpalCall::GetMediaFormats(const OpalConnection & connection)
{
  OpalMediaFormatList commonFormats;

  bool first = true;
  const OpalMediaTypeList allMediaTypes = OpalMediaType::GetList();

  PSafePtr<OpalConnection> otherConnection;
  while (EnumerateConnections(otherConnection, PSafeReadOnly, &connection)) {
    const OpalMediaFormatList otherConnectionsFormats = otherConnection->GetMediaFormats();
    OpalMediaFormatList possibleFormats;
    for (OpalMediaTypeList::const_iterator iterMediaType = allMediaTypes.begin(); iterMediaType != allMediaTypes.end(); ++iterMediaType) {
      OpalMediaFormatList formatsOfMediaType;
      for (OpalMediaFormatList::const_iterator it = otherConnectionsFormats.begin(); it != otherConnectionsFormats.end(); ++it) {
        if (it->GetMediaType() == *iterMediaType)
          formatsOfMediaType += *it;
      }
      if (!formatsOfMediaType.IsEmpty()) {
        if (connection.IsNetworkConnection() && otherConnection->IsNetworkConnection() &&
              m_manager.GetMediaTransferMode(connection, *otherConnection, *iterMediaType) != OpalManager::MediaTransferTranscode)
          possibleFormats += formatsOfMediaType;
        else
          possibleFormats += OpalTranscoder::GetPossibleFormats(formatsOfMediaType);
      }
    }
    if (first) {
      commonFormats = possibleFormats;
      first = false;
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

  connection.AdjustMediaFormats(true, NULL, commonFormats);

  PTRACE(4, "GetMediaFormats for " << connection << "\n    "
         << setfill(',') << commonFormats << setfill(' '));

  return commonFormats;
}


void OpalCall::AdjustMediaFormats(bool local, const OpalConnection & connection, OpalMediaFormatList & mediaFormats) const
{
  PSafePtr<OpalConnection> otherConnection;
  while (EnumerateConnections(otherConnection, PSafeReadOnly, &connection))
    otherConnection->AdjustMediaFormats(local, &connection, mediaFormats);
}


PBoolean OpalCall::OpenSourceMediaStreams(OpalConnection & connection, 
                                     const OpalMediaType & mediaType,
                                                  unsigned requestedSessionID, 
                                   const OpalMediaFormat & preselectedFormat,
#if OPAL_VIDEO
                              OpalVideoFormat::ContentRole contentRole,
#endif
                                                      bool transfer,
                                                      bool startPaused)
{
  PTRACE_CONTEXT_ID_PUSH_THREAD(this);

  PSafeLockReadWrite lock(*this);
  if (m_isClearing || !lock.IsLocked())
    return false;

  unsigned sessionID = requestedSessionID > 0 ? requestedSessionID : connection.GetNextSessionID(mediaType, true);

#if PTRACING
  PStringStream traceText;
  if (PTrace::CanTrace(2)) {
    traceText << " for ";
    if (sessionID == 0)
      traceText << "unallocated ";
    else if (requestedSessionID == 0)
      traceText << "allocated ";
    traceText << mediaType << " session";
    if (sessionID != 0)
      traceText << ' ' << sessionID;
    if (preselectedFormat.IsValid())
      traceText << " (" << preselectedFormat << ')';
    traceText << " on " << connection;
  }
#endif

  // Check if already done
  OpalMediaStreamPtr sinkStream;
  OpalMediaStreamPtr sourceStream = connection.GetMediaStream(sessionID, true);
  if (sourceStream != NULL && !transfer) {
    OpalMediaPatchPtr patch = sourceStream->GetPatch();
    if (patch != NULL)
      sinkStream = patch->GetSink();
    if (preselectedFormat.IsEmpty() ||
        sourceStream->GetMediaFormat() == preselectedFormat ||
        (sinkStream != NULL && sinkStream->GetMediaFormat() == preselectedFormat)) {
      if (sourceStream->IsPaused()) {
        sourceStream->SetPaused(false);
        PTRACE(3, "OpenSourceMediaStreams (un-pausing)" << traceText);
      }
      else {
        PTRACE(3, "OpenSourceMediaStreams (already opened)" << traceText);
      }
      return true;
    }
    if (sinkStream == NULL && sourceStream->IsOpen()) {
      PTRACE(3, "OpenSourceMediaStreams (is opening)" << traceText);
      return true;
    }
  }

  PTRACE(3, "OpenSourceMediaStreams " << (sourceStream != NULL ? "replace" : "open")
         << (startPaused ? " (paused)" : "") << (transfer ? " (transfer)" : "") << traceText);
  sourceStream.SetNULL();

  // Create the sinks and patch if needed
  bool startedOne = false;
  OpalMediaFormat sourceFormat, sinkFormat;

  // Reorder destinations so we give preference to symmetric codecs
  if (sinkStream != NULL)
    sinkFormat = sinkStream->GetMediaFormat();

  PSafePtr<OpalConnection> otherConnection;
  while (EnumerateConnections(otherConnection, PSafeReadWrite, &connection)) {
    PStringArray order;
    if (sinkFormat.IsValid())
      order += sinkFormat.GetName(); // Preferential treatment to format already in use
    order += '@' + mediaType;        // And media of the same type

    OpalMediaFormatList sinkMediaFormats = otherConnection->GetMediaFormats();
    if (sinkMediaFormats.IsEmpty()) {
      PTRACE(2, "OpenSourceMediaStreams failed with no sink formats" << traceText);
      return false;
    }

    if (preselectedFormat.IsValid() && sinkMediaFormats.HasFormat(preselectedFormat))
      sinkMediaFormats = preselectedFormat;
    else
      sinkMediaFormats.Reorder(order);

    OpalMediaFormatList sourceMediaFormats;
    if (sourceFormat.IsValid())
      sourceMediaFormats = sourceFormat; // Use the source format already established
    else {
      sourceMediaFormats = connection.GetMediaFormats();
      if (sourceMediaFormats.IsEmpty()) {
        PTRACE(2, "OpenSourceMediaStreams failed with no source formats" << traceText);
        return false;
      }

      if (preselectedFormat.IsValid() && sourceMediaFormats.HasFormat(preselectedFormat))
        sourceMediaFormats = preselectedFormat;
      else
        sourceMediaFormats.Reorder(order);
    }

    // Get other media directions format so we give preference to symmetric codecs
    OpalMediaStreamPtr otherDirection = sessionID != 0 ? otherConnection->GetMediaStream(sessionID, true)
                                                       : otherConnection->GetMediaStream(mediaType, true);
    if (otherDirection != NULL) {
      PString priorityFormat = otherDirection->GetMediaFormat().GetName();
      sourceMediaFormats.Reorder(priorityFormat);
      sinkMediaFormats.Reorder(priorityFormat);
    }

#if OPAL_VIDEO
    if (contentRole == OpalVideoFormat::eNoRole && preselectedFormat.IsValid())
      contentRole = preselectedFormat.GetOptionEnum(OpalVideoFormat::ContentRoleOption(), OpalVideoFormat::eNoRole);
    if (contentRole != OpalVideoFormat::eNoRole) {
      PTRACE(4, "Content Role " << contentRole << traceText);

      bool noDefaultMainRole = contentRole != OpalVideoFormat::eMainRole;

      // Remove all media formats not supporting the role
      OpalMediaFormatList * oldLists[2] = { &sourceMediaFormats, &sinkMediaFormats };
      OpalMediaFormatList newLists[2];
      for (PINDEX i = 0; i < 2; i++) {
        for (OpalMediaFormatList::iterator format = oldLists[i]->begin(); format != oldLists[i]->end(); ++format) {
          if (format->GetMediaType() != mediaType)
            continue;

          if (!format->IsTransportable()) {
            newLists[i] += *format;
            continue;
          }

          unsigned mask = format->GetOptionInteger(OpalVideoFormat::ContentRoleMaskOption());
          if (mask != 0)
            noDefaultMainRole = true;

          if ((mask&OpalVideoFormat::ContentRoleBit(contentRole)) != 0)
            newLists[i] += *format;
        }
      }

      if (noDefaultMainRole) {
        if (newLists[0].IsEmpty() || newLists[1].IsEmpty()) {
          PTRACE(2, "Unsupported Content Role " << contentRole << traceText);
          return false;
        }

        sourceMediaFormats = newLists[0];
        sinkMediaFormats = newLists[1];
      }
      else {
        PTRACE(4, "Default main content role used " << traceText);
        contentRole = OpalVideoFormat::eNoRole;
      }
    }
#endif

    if (!SelectMediaFormats(mediaType,
                            sourceMediaFormats,
                            sinkMediaFormats,
                            connection.GetLocalMediaFormats(),
                            sourceFormat,
                            sinkFormat))
      return false;

#if OPAL_VIDEO
    sourceFormat.SetOptionEnum(OpalVideoFormat::ContentRoleOption(), contentRole);
    sinkFormat.SetOptionEnum(OpalVideoFormat::ContentRoleOption(), contentRole);
#endif

    if (sessionID == 0) {
      sessionID = otherConnection->GetNextSessionID(mediaType, false);
      PTRACE(3, "OpenSourceMediaStreams using session " << sessionID << " on " << connection);
    }

    // Finally have the negotiated formats, open the streams
    if (sourceStream == NULL) {
      sourceStream = connection.OpenMediaStream(sourceFormat, sessionID, true);
      if (sourceStream == NULL)
        return false;
      // If re-opening, need to disconnect from old patch. If new then this sets NULL to NULL.
      sourceStream->SetPatch(NULL);
    }

    sinkStream = otherConnection->OpenMediaStream(sinkFormat, sessionID, false);
    if (sinkStream != NULL) {
      if (!sourceStream.SetSafetyMode(PSafeReadOnly))
        return false;

      OpalMediaPatchPtr patch = sourceStream->GetPatch();
      if (patch == NULL) {
        patch = m_manager.CreateMediaPatch(*sourceStream, sinkStream->RequiresPatchThread(sourceStream) &&
                                                        sourceStream->RequiresPatchThread(sinkStream));
        if (patch == NULL) {
          PTRACE(3, "OpenSourceMediaStreams could not patch session " << sessionID << " on " << connection);
          return false;
        }
      }

      sourceStream.SetSafetyMode(PSafeReference);

      if (patch->AddSink(sinkStream))
        startedOne = true;
      else
        sinkStream->Close();
    }
  }

  if (!startedOne) {
    if (sourceStream != NULL)
      sourceStream->Close();
    PTRACE(3, "OpenSourceMediaStreams did not start session " << sessionID << " on " << connection);
    return false;
  }

  if (!sourceStream.SetSafetyMode(PSafeReadOnly))
    return false;

  if (startPaused) {
    sourceStream->InternalSetPaused(true, false, false);
    sinkStream->InternalSetPaused(true, false, false);
  }

  OpalMediaPatchPtr patch = sourceStream->GetPatch();
  if (patch == NULL) {
    PTRACE(3, "OpenSourceMediaStreams patch failed session " << sessionID << " on " << connection);
    return false;
  }

  // If a patch was created, make sure the callback is called once per connection.
  while (EnumerateConnections(otherConnection, PSafeReadWrite))
    otherConnection->OnPatchMediaStream(otherConnection == &connection, *patch);

  PTRACE(4, "OpenSourceMediaStreams completed session " << sessionID << " on " << connection);
  return true;
}


bool OpalCall::SelectMediaFormats(const OpalMediaType & mediaType,
                                  const OpalMediaFormatList & srcFormats,
                                  const OpalMediaFormatList & dstFormats,
                                  const OpalMediaFormatList & masterFormats,
                                  OpalMediaFormat & srcFormat,
                                  OpalMediaFormat & dstFormat) const
{
  if (OpalTranscoder::SelectFormats(mediaType, srcFormats, dstFormats, masterFormats, srcFormat, dstFormat)) {
    PTRACE(4, "SelectMediaFormats:\n"
              "  source formats=" << setfill(',') << srcFormats << "\n"
              "   sink  formats=" << dstFormats << "\n"
              "  master formats=" << masterFormats << setfill(' '));
    PTRACE(3, "Selected media formats " << srcFormat << " -> " << dstFormat);
    return true;
  }

  PTRACE(2, "SelectMediaFormats could not find compatible " << mediaType << " format:\n"
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

  PTRACE(4, "OnUserInputTone: '" << tone << "', duration=" << duration << ", " << (reprocess ? "reprocessing as string" : "handled"));

  if (reprocess)
    connection.OnUserInputString(tone);
}


void OpalCall::OnHold(OpalConnection & connection, bool fromRemote, bool onHold)
{
  if (m_handlingHold)
    m_manager.OnHold(connection, fromRemote, onHold);
  else {
    m_handlingHold = true;
    PSafePtr<OpalConnection> otherConnection;
    while (EnumerateConnections(otherConnection, PSafeReadWrite, &connection)) {
      if (!otherConnection->IsNetworkConnection())
        otherConnection->OnHold(fromRemote, onHold);
      else if (fromRemote)
        otherConnection->HoldRemote(onHold);
    }
    m_handlingHold = false;
  }
}

#if OPAL_HAS_MIXER

bool OpalCall::StartRecording(const PFilePath & fn, const OpalRecordManager::Options & options)
{
  StopRecording();

  OpalRecordManager * newManager = OpalRecordManager::Factory::CreateInstance(fn.GetType());
  if (newManager == NULL) {
    PTRACE(2, "Cannot record to file type " << fn);
    return false;
  }

  PTRACE_CONTEXT_ID_TO(*newManager);

  // create the mixer entry
  if (!newManager->Open(fn, options)) {
    delete newManager;
    return false;
  }

  PSafeLockReadWrite lock(*this);
  if (!lock.IsLocked())
    return false;

  m_recordManager = newManager;

  // tell each connection to start sending data
  PSafePtr<OpalConnection> connection;
  while (EnumerateConnections(connection, PSafeReadWrite))
    connection->EnableRecording();

  return true;
}

bool OpalCall::IsRecording() const
{
  PSafeLockReadOnly lock(*this);
  return lock.IsLocked() && m_recordManager != NULL && m_recordManager->IsOpen();
}


bool OpalCall::StopRecording()
{
  PSafeLockReadWrite lock(*this);
  if (!lock.IsLocked() || m_recordManager == NULL)
    return false;

  // tell each connection to stop sending data
  PSafePtr<OpalConnection> connection;
  while (EnumerateConnections(connection, PSafeReadWrite))
    connection->DisableRecording();

  m_recordManager->Close();
  delete m_recordManager;
  m_recordManager = NULL;
  return true;
}


bool OpalCall::OnStartRecording(const PString & streamId, const OpalMediaFormat & format)
{
  PSafeLockReadOnly lock(*this);
  return lock.IsLocked() && m_recordManager != NULL && m_recordManager->OpenStream(streamId, format);
}


void OpalCall::OnStopRecording(const PString & streamId)
{
  PSafeLockReadOnly lock(*this);
  if (lock.IsLocked() && m_recordManager != NULL)
    m_recordManager->CloseStream(streamId);
}


void OpalCall::OnRecordAudio(const PString & streamId, const RTP_DataFrame & frame)
{
  PSafeLockReadOnly lock(*this);
  if (lock.IsLocked() && m_recordManager != NULL && !m_recordManager->WriteAudio(streamId, frame))
    m_recordManager->CloseStream(streamId);
}


#if OPAL_VIDEO

void OpalCall::OnRecordVideo(const PString & streamId, const RTP_DataFrame & frame)
{
  PSafeLockReadOnly lock(*this);
  if (lock.IsLocked() && m_recordManager != NULL && !m_recordManager->WriteVideo(streamId, frame))
    m_recordManager->CloseStream(streamId);
}

#endif

#endif


void OpalCall::SetPartyNames()
{
  PSafeLockReadWrite lock(*this);
  if (!lock.IsLocked())
    return;

  PSafePtr<OpalConnection> connectionA = m_connectionsActive.GetAt(0, PSafeReadOnly);
  if (connectionA == NULL)
    return;

  bool networkA = connectionA->IsNetworkConnection();
  if (networkA) {
    m_partyA = connectionA->GetRemotePartyURL();
    m_nameA = connectionA->GetRemotePartyName();
    m_identityA = connectionA->GetRemoteIdentity();
  }
  if (!networkA || m_partyA.IsEmpty()) {
    m_partyA = connectionA->GetLocalPartyURL();
    m_nameA = connectionA->GetDisplayName();
    m_identityA = connectionA->GetLocalPartyName();
  }

  PSafePtr<OpalConnection> connectionB = m_connectionsActive.GetAt(1, PSafeReadOnly);
  if (connectionB == NULL)
    return;

  if (connectionB->IsNetworkConnection()) {
    if (networkA)
      connectionB->CopyPartyNames(*connectionA);
    else
      connectionA->CopyPartyNames(*connectionB);
    m_partyB = connectionB->GetRemotePartyURL();
    m_nameB = connectionB->GetRemotePartyName();
    m_identityB = connectionB->GetRemoteIdentity();
  }
  else {
    if (networkA) {
      connectionB->CopyPartyNames(*connectionA);
      PString dest = connectionA->GetDestinationAddress();
      if (!dest.IsEmpty() && dest != "*")
        m_partyB = connectionA->GetCalledPartyURL();
    }
    if (m_partyB.IsEmpty())
      m_partyB = connectionB->GetLocalPartyURL();
    m_nameB = connectionB->GetDisplayName();
    m_identityB = connectionB->GetLocalPartyName();
  }
}


bool OpalCall::EnumerateConnections(PSafePtr<OpalConnection> & connection,
                                    PSafetyMode mode,
                                    const OpalConnection * skipConnection) const
{
  if (connection == NULL)
    connection = PSafePtr<OpalConnection>(m_connectionsActive, PSafeReference);
  else {
    connection.SetSafetyMode(PSafeReference);
    ++connection;
  }

  while (connection != NULL) {
    if (connection != skipConnection &&
        connection->GetPhase() <= OpalConnection::EstablishedPhase &&
        connection.SetSafetyMode(mode))
      return true;
    ++connection;
  }

  return false;
}


#if OPAL_SCRIPT

void OpalCall::ScriptClear(PScriptLanguage &, PScriptLanguage::Signature & sig)
{
  OpalConnection::CallEndReason reason;

  switch (sig.m_arguments.size()) {
    default :
    case 2 :
      reason.q931 = sig.m_arguments[1].AsInteger();
      // Next case

    case 1 :
      reason.code = (OpalConnection::CallEndReasonCodes)sig.m_arguments[0].AsInteger();
      // Next case

    case 0 :
      break;
  }

  Clear(reason);
}

#endif // OPAL_SCRIPT


/////////////////////////////////////////////////////////////////////////////
