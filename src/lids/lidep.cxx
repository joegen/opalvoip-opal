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
 * Revision 1.2014  2002/04/09 00:18:39  robertj
 * Added correct setting of originating flag when originating the connection.
 *
 * Revision 2.12  2002/01/22 05:16:59  robertj
 * Revamp of user input API triggered by RFC2833 support
 *
 * Revision 2.11  2001/11/14 01:31:55  robertj
 * Corrected placement of adjusting media format list.
 *
 * Revision 2.10  2001/11/13 06:25:56  robertj
 * Changed SetUpConnection() so returns BOOL as returning
 *   pointer to connection is not useful.
 *
 * Revision 2.9  2001/10/15 04:31:56  robertj
 * Removed answerCall signal and replaced with state based functions.
 *
 * Revision 2.8  2001/10/04 00:47:02  robertj
 * Removed redundent parameter.
 *
 * Revision 2.7  2001/08/23 05:51:17  robertj
 * Completed implementation of codec reordering.
 *
 * Revision 2.6  2001/08/23 03:15:51  robertj
 * Added missing Lock() calls in SetUpConnection
 *
 * Revision 2.5  2001/08/22 10:20:09  robertj
 * Changed connection locking to use double mutex to guarantee that
 *   no threads can ever deadlock or access deleted connection.
 *
 * Revision 2.4  2001/08/17 08:36:48  robertj
 * Moved call end reasons enum from OpalConnection to global.
 * Fixed missing mutex on connections structure..
 *
 * Revision 2.3  2001/08/17 01:11:17  robertj
 * Added ability to add whole LID's to LID endpoint.
 *
 * Revision 2.2  2001/08/01 06:23:55  robertj
 * Changed to use separate mutex for LIDs structure to avoid Unix nested mutex problem.
 *
 * Revision 2.1  2001/08/01 05:24:01  robertj
 * Made OpalMediaFormatList class global to help with documentation.
 * Made the connections media format list to come from LID.
 *
 * Revision 2.0  2001/07/27 15:48:25  robertj
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


BOOL OpalLIDEndPoint::SetUpConnection(OpalCall & call,
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

  linesMutex.Wait();

  for (PINDEX i = 0; i < lines.GetSize(); i++) {
    if ((lineName == "*"  || lines[i].GetDescription() == lineName) && lines[i].EnableAudio()) {
      connection = CreateConnection(call, lines[i], userData);
      inUseFlag.Wait();
      connectionsActive.SetAt(connection->GetToken(), connection);
      inUseFlag.Signal();
      break;
    }
  }

  linesMutex.Signal();

  if (connection == NULL)
    return FALSE;

  connection->Lock();
  connection->StartOutgoing(lineName);
  connection->Unlock();

  return TRUE;
}


OpalLineConnection * OpalLIDEndPoint::CreateConnection(OpalCall & call,
                                                       OpalLine & line,
                                                       void * /*userData*/)
{
  return new OpalLineConnection(call, *this, line);
}


static void InitialiseLine(OpalLine * line)
{
  line->Ring(0);
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

  linesMutex.Wait();
  lines.Append(line);
  linesMutex.Signal();

  return TRUE;
}


void OpalLIDEndPoint::RemoveLine(OpalLine * line)
{
  linesMutex.Wait();
  lines.Remove(line);
  linesMutex.Signal();
}


void OpalLIDEndPoint::RemoveLine(const PString & token)
{
  linesMutex.Wait();
  for (PINDEX i = 0; i < lines.GetSize(); i++) {
    if (lines[i].GetToken() *= token)
      lines.RemoveAt(i--);
  }
  linesMutex.Signal();
}


void OpalLIDEndPoint::RemoveAllLines()
{
  linesMutex.Wait();
  lines.RemoveAll();
  devices.RemoveAll();
  linesMutex.Signal();
}


BOOL OpalLIDEndPoint::AddLinesFromDevice(OpalLineInterfaceDevice & device)
{
  if (!device.IsOpen())
    return FALSE;

  BOOL atLeastOne = FALSE;

  linesMutex.Wait();

  for (unsigned line = 0; line < device.GetLineCount(); line++) {
    if (device.IsLineTerminal(line) == HasAttribute(CanTerminateCall)) {
      OpalLine * newLine = new OpalLine(device, line);
      InitialiseLine(newLine);
      lines.Append(newLine);
      atLeastOne = TRUE;
    }
  }

  linesMutex.Signal();

  return atLeastOne;
}


void OpalLIDEndPoint::RemoveLinesFromDevice(OpalLineInterfaceDevice & device)
{
  linesMutex.Wait();
  for (PINDEX i = 0; i < lines.GetSize(); i++) {
    if (lines[i].GetToken().Find(device.GetName()) == 0)
      lines.RemoveAt(i--);
  }
  linesMutex.Signal();
}


BOOL OpalLIDEndPoint::AddDevice(OpalLineInterfaceDevice * device)
{
  PAssertNULL(device);
  linesMutex.Wait();
  devices.Append(device);
  linesMutex.Signal();
  return AddLinesFromDevice(*device);
}


void OpalLIDEndPoint::RemoveDevice(OpalLineInterfaceDevice * device)
{
  PAssertNULL(device);
  RemoveLinesFromDevice(*device);
  linesMutex.Wait();
  devices.Remove(device);
  linesMutex.Signal();
}


void OpalLIDEndPoint::MonitorLines(PThread &, INT)
{
  PTRACE(3, "LID EP\tMonitor thread started for " << GetPrefixName());

  while (!exitFlag.Wait(100)) {
    linesMutex.Wait();
    for (PINDEX i = 0; i < lines.GetSize(); i++)
      MonitorLine(lines[i]);
    linesMutex.Signal();
  }

  PTRACE(3, "LID EP\tMonitor thread stopped for " << GetPrefixName());
}


void OpalLIDEndPoint::MonitorLine(OpalLine & line)
{
  OpalConnection * connection = GetConnectionWithLock(line.GetToken());
  if (connection != NULL) {
    // Are still in a call, pass hook state it to the connection object for handling
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

  LockOnRelease();
  phase = ReleasedPhase;

  if (handlerThread != NULL) {
    PTRACE(3, "LID Con\tAwaiting handler thread termination " << *this);
    // Stop the signalling handler thread
    SetUserInput(PString()); // Break out of ReadUserInput
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
  PString str = ReadUserInput();
  if (phase == SetUpPhase)
    return str;

  return PString();
}


BOOL OpalLineConnection::SetAlerting(const PString & calleeName, BOOL)
{
  line.SetCallerID(calleeName);
  return line.PlayTone(OpalLineInterfaceDevice::RingTone);
}


BOOL OpalLineConnection::SetConnected()
{
  PTRACE(3, "LID Con\tSetConnected " << *this);

  return line.StopTone();
}


OpalMediaFormatList OpalLineConnection::GetMediaFormats() const
{
  return line.GetDevice().GetMediaFormats();
}


OpalMediaStream * OpalLineConnection::CreateMediaStream(BOOL isSource, unsigned sessionID)
{
  return new OpalLineMediaStream(isSource, sessionID, line);
}


BOOL OpalLineConnection::SendUserInputString(const PString & value)
{
  return line.PlayDTMF(value);
}


BOOL OpalLineConnection::SendUserInputTone(char tone, int duration)
{
  if (duration <= 0)
    return line.PlayDTMF(&tone);
  else
    return line.PlayDTMF(&tone, duration);
}


BOOL OpalLineConnection::PromptUserInput(BOOL play)
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
      OnUserInputTone(tone, 180);

    wasOffHook = TRUE;
  }
  else if (wasOffHook) {
    PTRACE(3, "LID Con\tConnection " << callToken << " on hook: phase=" << phase);
    Release(EndedByRemoteUser);
    wasOffHook = FALSE;
  }
}


void OpalLineConnection::HandleIncoming(PThread &, INT)
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
        Release(EndedByCallerAbort);
        return;
      }
      PThread::Sleep(100);
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
}


BOOL OpalLineConnection::StartOutgoing(const PString & number)
{
  PTRACE(3, "LID Con\tHandling outgoing call on " << *this);

  originating = TRUE;

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
