/*
 * lidep.cxx
 *
 * Line Interface Device EndPoint
 *
 * Open Phone Abstraction Library
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
 * The Original Code is Open H323 Library.
 *
 * The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * Contributor(s): ______________________________________.
 *
 * $Log: lidep.cxx,v $
 * Revision 1.2001  2001/07/27 15:48:25  robertj
 * Conversion of OpenH323 to Open Phone Abstraction Library (OPAL)
 *
 */

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "lidep.cxx"
#endif

#include <lids/lidep.h>

#include <opal/manager.h>


#define new PNEW

/////////////////////////////////////////////////////////////////////////////

OpalLIDEndPoint::OpalLIDEndPoint(OpalManager & mgr,
                                 const PString & prefix,
                                 unsigned attributes)
  : OpalEndPoint(mgr, prefix, attributes)
{
  monitorThread = PThread::Create(PCREATE_NOTIFIER(MonitorLines), 0,
                                  PThread::NoAutoDeleteThread,
                                  PThread::LowPriority,
                                  prefix.ToUpper() & "Line Monitor");
}


OpalLIDEndPoint::~OpalLIDEndPoint()
{
  PTRACE(3, "LID EP\tAwaiting monitor thread termination " << GetPrefixName());
  exitFlag.Signal();
  monitorThread->WaitForTermination();
  delete monitorThread;
  monitorThread = NULL;
}


OpalConnection * OpalLIDEndPoint::SetUpConnection(OpalCall & call,
                                                  const PString & remoteParty,
                                                  void * userData)
{
  // First strip of the prefix if present
  PINDEX prefixLength = 0;
  if (remoteParty.Find(GetPrefixName()+":") == 0)
    prefixLength = GetPrefixName().GetLength()+1;

  // Then see if there is a specific line mentioned
  PString number, lineName;
  PINDEX at = remoteParty.Find('@', prefixLength);
  if (at != P_MAX_INDEX) {
    number = remoteParty(prefixLength, at);
    lineName = remoteParty.Mid(at+1);
  }
  else {
    if (HasAttribute(CanTerminateCall))
      lineName = remoteParty.Mid(prefixLength);
    else
      number = remoteParty.Mid(prefixLength);
  }

  if (lineName.IsEmpty())
    lineName = '*';

  // Locate a line
  OpalLineConnection * connection = NULL;

  inUseFlag.Wait();

  for (PINDEX i = 0; i < lines.GetSize(); i++) {
    if ((lineName == "*"  || lines[i].GetDescription() == lineName) && lines[i].EnableAudio()) {
      connection = CreateConnection(call, lines[i], userData);
      PAssertNULL(connection);
      connectionsActive.SetAt(connection->GetToken(), connection);
      break;
    }
  }

  inUseFlag.Signal();

  if (connection != NULL)
    connection->StartOutgoing(lineName);

  return connection;
}


OpalMediaFormat::List OpalLIDEndPoint::GetMediaFormats() const
{
  OpalMediaFormat::List formats;

  PWaitAndSignal wait(((OpalLIDEndPoint*)this)->inUseFlag);

  for (PINDEX i = 0; i < lines.GetSize(); i++) {
    OpalMediaFormat::List devFormats = lines[i].GetDevice().GetMediaFormats();
    for (PINDEX f = 0; f < devFormats.GetSize(); f++)
      formats += devFormats[f];
  }

  return formats;
}


OpalLineConnection * OpalLIDEndPoint::CreateConnection(OpalCall & call,
                                                       OpalLine & line,
                                                       void * /*userData*/)
{
  return new OpalLineConnection(call, *this, line);
}


static void InitialiseLine(OpalLine * line)
{
  line->StopTone();
  line->StopRawCodec();
  line->StopReadCodec();
  line->StopWriteCodec();
  line->DisableAudio();

  for (unsigned lnum = 0; lnum < line->GetDevice().GetLineCount(); lnum++) {
    if (lnum != line->GetLineNumber())
      line->GetDevice().SetLineToLineDirect(lnum, line->GetLineNumber(), FALSE);
  }
}


BOOL OpalLIDEndPoint::AddLine(OpalLine * line)
{
  if (PAssertNULL(line) == NULL)
    return FALSE;

  if (!line->GetDevice().IsOpen())
    return FALSE;

  if (line->IsTerminal() != HasAttribute(CanTerminateCall))
    return FALSE;

  InitialiseLine(line);

  inUseFlag.Wait();
  lines.Append(line);
  inUseFlag.Signal();

  return TRUE;
}


void OpalLIDEndPoint::RemoveLine(OpalLine * line)
{
  inUseFlag.Wait();
  lines.Remove(line);
  inUseFlag.Signal();
}


void OpalLIDEndPoint::RemoveLine(const PString & token)
{
  inUseFlag.Wait();
  for (PINDEX i = 0; i < lines.GetSize(); i++) {
    if (lines[i].GetToken() *= token)
      lines.RemoveAt(i--);
  }
  inUseFlag.Signal();
}


BOOL OpalLIDEndPoint::AddLinesFromDevice(OpalLineInterfaceDevice & device)
{
  if (!device.IsOpen())
    return FALSE;

  BOOL atLeastOne = FALSE;

  inUseFlag.Wait();

  for (unsigned line = 0; line < device.GetLineCount(); line++) {
    if (device.IsLineTerminal(line) == HasAttribute(CanTerminateCall)) {
      OpalLine * newLine = new OpalLine(device, line);
      InitialiseLine(newLine);
      lines.Append(newLine);
      atLeastOne = TRUE;
    }
  }

  inUseFlag.Signal();

  return atLeastOne;
}


void OpalLIDEndPoint::RemoveLinesFromDevice(OpalLineInterfaceDevice & device)
{
  inUseFlag.Wait();
  for (PINDEX i = 0; i < lines.GetSize(); i++) {
    if (lines[i].GetToken().Find(device.GetName()) == 0)
      lines.RemoveAt(i--);
  }
  inUseFlag.Signal();
}


void OpalLIDEndPoint::MonitorLines(PThread &, INT)
{
  PTRACE(3, "LID EP\tMonitor thread started for " << GetPrefixName());

  while (!exitFlag.Wait(100)) {
    inUseFlag.Wait();
    for (PINDEX i = 0; i < lines.GetSize(); i++)
      MonitorLine(lines[i]);
    inUseFlag.Signal();
  }

  PTRACE(3, "LID EP\tMonitor thread stopped for " << GetPrefixName());
}


void OpalLIDEndPoint::MonitorLine(OpalLine & line)
{
  OpalConnection * connection = GetConnectionWithLock(line.GetToken());
  if (connection != NULL) {
    // Are still in a call, pass hook state it to the connetion object for handling
    ((OpalLineConnection *)connection)->Monitor(!line.IsDisconnected());
    connection->Unlock();
    return;
  }

  if (line.IsAudioEnabled()) {
    // Are still in previous call, wait for them to hang up
    if (line.IsDisconnected()) {
      PTRACE(3, "LID EP\tLine " << line << " has disconnected.");
      line.StopTone();
      line.DisableAudio();
    }
    return;
  }

  if (line.IsTerminal()) {
    // Not off hook, so nothing happening, just return
    if (!line.IsOffHook())
      return;
    PTRACE(3, "LID EP\tLine " << line << " has gone off hook.");
  }
  else {
    // Not ringing, so nothing happening, just return
    if (!line.IsRinging())
      return;
    PTRACE(3, "LID EP\tLine " << line << " is ringing.");
  }

  // See if we can get exlusive use of the line. With soemthing like a LinJACK
  // enableing audio on the PSTN line the POTS line will no longer be enableable
  // so this will fail and the ringing will be ignored
  if (!line.EnableAudio())
    return;

  // Have incoming ring, create a new LID connection and let it handle it
  connection = CreateConnection(*manager.CreateCall(), line, NULL);
  connectionsActive.SetAt(line.GetToken(), connection);
  ((OpalLineConnection *)connection)->StartIncoming();
}


/////////////////////////////////////////////////////////////////////////////

OpalLineConnection::OpalLineConnection(OpalCall & call,
                                       OpalLIDEndPoint & ep,
                                       OpalLine & ln)
  : OpalConnection(call, ep, ln.GetToken()),
    endpoint(ep),
    line(ln)
{
  phase = SetUpPhase;
  answerRingCount = 3;
  requireTonesForDial = TRUE;
  wasOffHook = FALSE;
  handlerThread = NULL;

  PTRACE(3, "LID Con\tConnection " << callToken << " created");
}


OpalConnection::Phases OpalLineConnection::GetPhase() const
{
  return phase;
}


BOOL OpalLineConnection::OnReleased()
{
  PTRACE(2, "LID Con\tOnReleased " << *this);

  // A kludge to avoid a deadlock, grab the lock, set the connectionState to
  // indicate we are shutting down then release the lock with a short sleep
  // to assure all threads waiting on that lock have time to get scheduled.
  // When they are they see the connection state and exit immediately.

  inUseFlag.Wait();
  phase = ReleasedPhase;
  inUseFlag.Signal();
  PThread::Current()->Sleep(1);
  inUseFlag.Wait();

  if (handlerThread != NULL) {
    PTRACE(3, "LID Con\tAwaiting handler thread termination " << *this);
    // Stop the signalling handler thread
    SetUserIndication(PString()); // Break out of ReadUserInput
    SetAnswerResponse(AnswerCallDenied); // Break out of answer loop
    handlerThread->WaitForTermination();
    delete handlerThread;
    handlerThread = NULL;
  }

  PTRACE(3, "LID Con\tPlaying clear tone until handset onhook");
  line.PlayTone(OpalLineInterfaceDevice::ClearTone);
  line.Ring(FALSE);
  line.SetOnHook();

  return OpalConnection::OnReleased();
}


PString OpalLineConnection::GetDestinationAddress()
{
  PString str = ReadUserIndication();
  if (phase == SetUpPhase)
    return str;

  return PString();
}


BOOL OpalLineConnection::SetAlerting(const PString & calleeName)
{
  line.SetCallerID(calleeName);
  return line.PlayTone(OpalLineInterfaceDevice::RingTone);
}


BOOL OpalLineConnection::SetConnected()
{
  PTRACE(3, "LID Con\tSetConnected " << *this);

  return line.StopTone();
}


OpalMediaStream * OpalLineConnection::CreateMediaStream(BOOL isSource, unsigned sessionID)
{
  return new OpalLineMediaStream(isSource, sessionID, line);
}


BOOL OpalLineConnection::SendUserIndicationString(const PString & value)
{
  return line.PlayDTMF(value);
}


BOOL OpalLineConnection::SendUserIndicationTone(char tone, int duration)
{
  if (duration <= 0)
    return line.PlayDTMF(&tone);
  else
    return line.PlayDTMF(&tone, duration);
}


BOOL OpalLineConnection::PromptUserIndication(BOOL play)
{
  PTRACE(3, "LID Con\tConnection " << callToken << " dial tone " << (play ? "on" : "off"));

  if (play)
    return line.PlayTone(OpalLineInterfaceDevice::DialTone);
  else
    return line.StopTone();
}


void OpalLineConnection::StartIncoming()
{
  if (handlerThread == NULL)
    handlerThread = PThread::Create(PCREATE_NOTIFIER(HandleIncoming), 0,
                                    PThread::NoAutoDeleteThread,
                                    PThread::LowPriority,
                                    "Line Connection:%x");
}


void OpalLineConnection::Monitor(BOOL offHook)
{
  if (offHook) {
    PTRACE_IF(3, !wasOffHook, "LID Con\tConnection " << callToken << " off hook: phase=" << phase);

    if (phase == AlertingPhase) {
      phase = ConnectedPhase;
      OnConnected();
    }

    char tone;
    while ((tone = line.ReadDTMF()) != '\0')
      OnUserIndicationTone(tone, 0);

    wasOffHook = TRUE;
  }
  else if (wasOffHook) {
    PTRACE(3, "LID Con\tConnection " << callToken << " on hook: phase=" << phase);
    Release(OpalConnection::EndedByRemoteUser);
    wasOffHook = FALSE;
  }
}


void OpalLineConnection::HandleIncoming(PThread & thread, INT)
{
  PTRACE(3, "LID Con\tHandling incoming call on " << *this);

  if (line.IsTerminal())
    remotePartyName = line.GetDescription();
  else {
    // Count incoming rings
    unsigned count;
    do {
      count = line.GetRingCount();
      if (count == 0) {
        PTRACE(2, "LID Con\tIncoming PSTN call stopped.");
        Release(OpalConnection::EndedByCallerAbort);
        return;
      }
      thread.Sleep(100);
    } while (count < answerRingCount);

    // Get caller ID
    PString callerId;
    if (line.GetCallerID(callerId, TRUE)) {
      PStringArray words = callerId.Tokenise('\t', TRUE);
      if (words.GetSize() < 3) {
        PTRACE(2, "LID Con\tMalformed caller ID \"" << callerId << '"');
      }
      else {
        remotePartyNumber = words[0].Trim();
        remotePartyName = words[1].Trim();
        if (remotePartyName.IsEmpty())
          remotePartyName = remotePartyNumber;
      }
    }

    line.SetOffHook();
  }

  wasOffHook = TRUE;

  if (!OnIncomingConnection()) {
    Release(EndedByCallerAbort);
    return;
  }

  // If pending response wait
  while (answerResponse == AnswerCallPending || answerResponse == AnswerCallDeferred) {
    PTRACE(3, "LID Con\tAwaiting answer call response, is " << answerResponse);
    if (answerResponse == AnswerCallPending)
      SetAlerting(remotePartyName);
    answerWaitFlag.Wait();
  }
  PTRACE(3, "LID Con\tAnswer call response: " << answerResponse);

  // If response is denied, abort the call
  if (answerResponse == AnswerCallDenied)
    Release(EndedByNoAnswer);
}


BOOL OpalLineConnection::StartOutgoing(const PString & number)
{
  PTRACE(3, "LID Con\tHandling outgoing call on " << *this);

  if (line.IsTerminal()) {
    line.SetCallerID(number);
    line.Ring(TRUE);
    phase = AlertingPhase;
    OnAlerting();
  }
  else {
    switch (line.DialOut(number, requireTonesForDial)) {
      case OpalLineInterfaceDevice::DialTone :
        PTRACE(3, "LID Con\tNo dial tone on " << line);
        return FALSE;

      case OpalLineInterfaceDevice::RingTone :
        PTRACE(3, "LID Con\tGot ringback on " << line);
        phase = AlertingPhase;
        OnAlerting();
        break;

      default :
        PTRACE(3, "LID Con\tError dialling " << number << " on " << line);
        return FALSE;
    }
  }

  return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
