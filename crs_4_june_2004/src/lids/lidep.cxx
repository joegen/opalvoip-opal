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
 * Revision 1.2023  2004/05/24 13:52:30  rjongbloed
 * Added a separate structure for the silence suppression paramters to make
 *   it easier to set from global defaults in the manager class.
 *
 * Revision 2.21  2004/05/17 13:24:18  rjongbloed
 * Added silence suppression.
 *
 * Revision 2.20  2004/04/25 08:34:08  rjongbloed
 * Fixed various GCC 3.4 warnings
 *
 * Revision 2.19  2004/04/18 13:35:27  rjongbloed
 * Fixed ability to make calls where both endpoints are specified a priori. In particular
 *   fixing the VXML support for an outgoing sip/h323 call.
 *
 * Revision 2.18  2003/06/02 02:56:41  rjongbloed
 * Moved LID specific media stream class to LID source file.
 *
 * Revision 2.17  2003/03/24 07:18:29  robertj
 * Added registration system for LIDs so can work with various LID types by
 *   name instead of class instance.
 *
 * Revision 2.16  2003/03/17 10:26:59  robertj
 * Added video support.
 *
 * Revision 2.15  2003/03/06 03:57:47  robertj
 * IVR support (work in progress) requiring large changes everywhere.
 *
 * Revision 2.14  2002/09/04 05:28:14  robertj
 * Added ability to set default line name to be used when the destination
 *   does not match any lines configured.
 *
 * Revision 2.13  2002/04/09 00:18:39  robertj
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
#include <opal/patch.h>


#define new PNEW

/////////////////////////////////////////////////////////////////////////////

OpalLIDEndPoint::OpalLIDEndPoint(OpalManager & mgr,
                                 const PString & prefix,
                                 unsigned attributes)
  : OpalEndPoint(mgr, prefix, attributes),
    defaultLine("*")
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


BOOL OpalLIDEndPoint::MakeConnection(OpalCall & call,
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
  OpalLine * line = GetLine(lineName, TRUE);
  if (line == NULL)
    line = GetLine(defaultLine, TRUE);
  if (line == NULL)
    return FALSE;

  OpalLineConnection * connection = CreateConnection(call, *line, userData, number);

  inUseFlag.Wait();
  connectionsActive.SetAt(connection->GetToken(), connection);
  inUseFlag.Signal();

  // If we are the A-party then need to initiate a call now in this thread. If
  // we are the B-Party then SetUpConnection() gets called in the context of
  // the A-party thread.
  if (&call.GetConnection(0) == connection)
    connection->SetUpConnection();

  return TRUE;
}


OpalLineConnection * OpalLIDEndPoint::CreateConnection(OpalCall & call,
                                                       OpalLine & line,
                                                       void * /*userData*/,
                                                       const PString & number)
{
  return new OpalLineConnection(call, *this, line, number);
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


BOOL OpalLIDEndPoint::AddDeviceNames(const PStringArray & descriptors)
{
  BOOL ok = FALSE;

  for (PINDEX i = 0; i < descriptors.GetSize(); i++) {
    if (AddDeviceName(descriptors[i]))
      ok = TRUE;
  }

  return ok;
}


BOOL OpalLIDEndPoint::AddDeviceName(const PString & descriptor)
{
  PINDEX colon = descriptor.Find(':');
  if (colon == P_MAX_INDEX)
    return FALSE;

  PString deviceType = descriptor.Left(colon).Trim();
  PString deviceName = descriptor.Mid(colon+1).Trim();

  // Make sure not already there.
  linesMutex.Wait();
  for (PINDEX i = 0; i < devices.GetSize(); i++) {
    if (devices[i].GetName().Find(deviceName) != P_MAX_INDEX) {
      linesMutex.Signal();
      return TRUE;
    }
  }
  linesMutex.Signal();

  // Not there so add it.
  OpalLineInterfaceDevice * device = OpalLineInterfaceDevice::Create(deviceType);
  if (device == NULL)
    return FALSE;

  if (device->Open(deviceName))
    return AddDevice(device);

  delete device;
  return FALSE;
}


BOOL OpalLIDEndPoint::AddDevice(OpalLineInterfaceDevice * device)
{
  if (PAssertNULL(device) == NULL)
    return FALSE;

  linesMutex.Wait();
  devices.Append(device);
  linesMutex.Signal();
  return AddLinesFromDevice(*device);
}


void OpalLIDEndPoint::RemoveDevice(OpalLineInterfaceDevice * device)
{
  if (PAssertNULL(device) == NULL)
    return;

  RemoveLinesFromDevice(*device);
  linesMutex.Wait();
  devices.Remove(device);
  linesMutex.Signal();
}


OpalLine * OpalLIDEndPoint::GetLine(const PString & lineName, BOOL enableAudio) const
{
  PWaitAndSignal mutex(linesMutex);

  for (PINDEX i = 0; i < lines.GetSize(); i++) {
    if ((lineName == "*" || lines[i].GetDescription() == lineName) &&
        (!enableAudio || lines[i].EnableAudio()))
      return &lines[i];
  }

  return NULL;
}


void OpalLIDEndPoint::SetDefaultLine(const PString & lineName)
{
  linesMutex.Wait();
  defaultLine = lineName;
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

  // See if we can get exlusive use of the line. With something like a LineJACK
  // enabling audio on the PSTN line the POTS line will no longer be enable-able
  // so this will fail and the ringing will be ignored
  if (!line.EnableAudio())
    return;

  // Have incoming ring, create a new LID connection and let it handle it
  connection = CreateConnection(*manager.CreateCall(), line, NULL, PString::Empty());
  connectionsActive.SetAt(line.GetToken(), connection);
  ((OpalLineConnection *)connection)->StartIncoming();
}


/////////////////////////////////////////////////////////////////////////////

OpalLineConnection::OpalLineConnection(OpalCall & call,
                                       OpalLIDEndPoint & ep,
                                       OpalLine & ln,
                                       const PString & number)
  : OpalConnection(call, ep, ln.GetToken()),
    endpoint(ep),
    line(ln)
{
  remotePartyNumber = number;
  silenceDetector = new OpalLineSilenceDetector(line);

  answerRingCount = 3;
  requireTonesForDial = TRUE;
  wasOffHook = FALSE;
  handlerThread = NULL;

  PTRACE(3, "LID Con\tConnection " << callToken << " created");
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
  return ReadUserInput();
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
  OpalMediaFormatList formats = line.GetDevice().GetMediaFormats();
  AddVideoMediaFormats(formats);
  return formats;
}


OpalMediaStream * OpalLineConnection::CreateMediaStream(const OpalMediaFormat & mediaFormat,
                                                        unsigned sessionID,
                                                        BOOL isSource)
{
  if (sessionID != OpalMediaFormat::DefaultAudioSessionID)
    return OpalConnection::CreateMediaStream(mediaFormat, sessionID, isSource);

  return new OpalLineMediaStream(mediaFormat, sessionID, isSource, line);
}


BOOL OpalLineConnection::OnOpenMediaStream(OpalMediaStream & mediaStream)
{
  if (!OpalConnection::OnOpenMediaStream(mediaStream))
    return FALSE;

  if (mediaStream.IsSource()) {
    OpalMediaPatch * patch = mediaStream.GetPatch();
    if (patch != NULL) {
      silenceDetector->SetParameters(endpoint.GetManager().GetSilenceDetectParams());
      patch->AddFilter(silenceDetector->GetReceiveHandler(), line.GetReadFormat());
    }
  }

  return TRUE;
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

  phase = SetUpPhase;

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

  PTRACE(2, "LID\tIncoming call routed for " << *this);
  if (!ownerCall.OnSetUp(*this))
    Release(EndedByNoAccept);
}


BOOL OpalLineConnection::SetUpConnection()
{
  PTRACE(3, "LID Con\tHandling outgoing call on " << *this);

  phase = SetUpPhase;
  originating = TRUE;

  if (line.IsTerminal()) {
    line.SetCallerID(remotePartyNumber);
    line.Ring(TRUE);
    phase = AlertingPhase;
    OnAlerting();
  }
  else {
    switch (line.DialOut(remotePartyNumber, requireTonesForDial)) {
      case OpalLineInterfaceDevice::DialTone :
        PTRACE(3, "LID Con\tNo dial tone on " << line);
        return FALSE;

      case OpalLineInterfaceDevice::RingTone :
        PTRACE(3, "LID Con\tGot ringback on " << line);
        phase = AlertingPhase;
        OnAlerting();
        break;

      default :
        PTRACE(3, "LID Con\tError dialling " << remotePartyNumber << " on " << line);
        return FALSE;
    }
  }

  return TRUE;
}


///////////////////////////////////////////////////////////////////////////////

OpalLineMediaStream::OpalLineMediaStream(const OpalMediaFormat & mediaFormat,
                                         unsigned sessionID,
                                         BOOL isSource,
                                         OpalLine & ln)
  : OpalMediaStream(mediaFormat, sessionID, isSource),
    line(ln)
{
  useDeblocking = FALSE;
  missedCount = 0;
  lastSID[0] = 2;
  lastFrameWasSignal = TRUE;
}


BOOL OpalLineMediaStream::Open()
{
  if (isOpen)
    return TRUE;

  if (IsSource()) {
    if (!line.SetReadFormat(mediaFormat))
      return FALSE;
    useDeblocking = mediaFormat.GetFrameSize() != line.GetReadFrameSize();
  }
  else {
    if (!line.SetWriteFormat(mediaFormat))
      return FALSE;
    useDeblocking = mediaFormat.GetFrameSize() != line.GetWriteFrameSize();
  }

  PTRACE(3, "Media\tStream set to " << mediaFormat << ", frame size: rd="
         << line.GetReadFrameSize() << " wr="
         << line.GetWriteFrameSize() << ", "
         << (useDeblocking ? "needs" : "no") << " reblocking.");

  return OpalMediaStream::Open();
}


BOOL OpalLineMediaStream::Close()
{
  if (IsSource())
    line.StopReadCodec();
  else
    line.StopWriteCodec();

  return OpalMediaStream::Close();
}


BOOL OpalLineMediaStream::ReadData(BYTE * buffer, PINDEX size, PINDEX & length)
{
  length = 0;

  if (IsSink()) {
    PTRACE(1, "Media\tTried to read from sink media stream");
    return FALSE;
  }

  if (useDeblocking) {
    line.SetReadFrameSize(size);
    if (line.ReadBlock(buffer, size)) {
      length = size;
      return TRUE;
    }
  }
  else {
    if (line.ReadFrame(buffer, length)) {
      // In the case of G.723.1 remember the last SID frame we sent and send
      // it again if the hardware sends us a CNG frame.
      if (mediaFormat.GetPayloadType() == RTP_DataFrame::G7231) {
        switch (length) {
          case 1 : // CNG frame
            memcpy(buffer, lastSID, 4);
            length = 4;
            lastFrameWasSignal = FALSE;
            break;
          case 4 :
            if ((*(BYTE *)buffer&3) == 2)
              memcpy(lastSID, buffer, 4);
            lastFrameWasSignal = FALSE;
            break;
          default :
            lastFrameWasSignal = TRUE;
        }
      }
      return TRUE;
    }
  }

  PTRACE_IF(1, line.GetDevice().GetErrorNumber() != 0,
            "Media\tDevice read frame error: " << line.GetDevice().GetErrorText());

  return FALSE;
}


BOOL OpalLineMediaStream::WriteData(const BYTE * buffer, PINDEX length, PINDEX & written)
{
  written = 0;

  if (IsSource()) {
    PTRACE(1, "Media\tTried to write to source media stream");
    return FALSE;
  }

  // Check for writing silence
  PBYTEArray silenceBuffer;
  if (length != 0)
    missedCount = 0;
  else {
    switch (mediaFormat.GetPayloadType()) {
      case RTP_DataFrame::G7231 :
        if (missedCount++ < 4) {
          static const BYTE g723_erasure_frame[24] = { 0xff, 0xff, 0xff, 0xff };
          buffer = g723_erasure_frame;
          length = 24;
        }
        else {
          static const BYTE g723_cng_frame[4] = { 3 };
          buffer = g723_cng_frame;
          length = 1;
        }
        break;

      case RTP_DataFrame::PCMU :
      case RTP_DataFrame::PCMA :
        buffer = silenceBuffer.GetPointer(line.GetWriteFrameSize());
        length = silenceBuffer.GetSize();
        memset((void *)buffer, 0xff, length);
        break;

      case RTP_DataFrame::G729 :
        if (mediaFormat.Find('B') != P_MAX_INDEX) {
          static const BYTE g729_sid_frame[2] = { 1 };
          buffer = g729_sid_frame;
          length = 2;
          break;
        }
        // Else fall into default case

      default :
        buffer = silenceBuffer.GetPointer(line.GetWriteFrameSize()); // Fills with zeros
        length = silenceBuffer.GetSize();
        break;
    }
  }

  if (useDeblocking) {
    line.SetWriteFrameSize(length);
    if (line.WriteBlock(buffer, length)) {
      written = length;
      return TRUE;
    }
  }
  else {
    if (line.WriteFrame(buffer, length, written))
      return TRUE;
  }

  PTRACE_IF(1, line.GetDevice().GetErrorNumber() != 0,
            "Media\tLID write frame error: " << line.GetDevice().GetErrorText());

  return FALSE;
}


BOOL OpalLineMediaStream::IsSynchronous() const
{
  return TRUE;
}


/////////////////////////////////////////////////////////////////////////////

OpalLineSilenceDetector::OpalLineSilenceDetector(OpalLine & l)
  : line(l)
{
}


unsigned OpalLineSilenceDetector::GetAverageSignalLevel(const BYTE *, PINDEX)
{
  return line.GetAverageSignalLevel(TRUE);
}


/////////////////////////////////////////////////////////////////////////////
