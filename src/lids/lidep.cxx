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
 * $Revision$
 * $Author$
 * $Date$
 */

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "lidep.cxx"
#endif

#include <lids/lidep.h>

#include <opal/manager.h>
#include <opal/call.h>
#include <opal/patch.h>


#define new PNEW

/////////////////////////////////////////////////////////////////////////////

OpalLineEndPoint::OpalLineEndPoint(OpalManager & mgr)
  : OpalEndPoint(mgr, "pots", CanTerminateCall),
    defaultLine("*")
{
  PTRACE(4, "LID EP\tOpalLineEndPoint created");
  manager.AttachEndPoint(this, "pstn", false);
  monitorThread = PThread::Create(PCREATE_NOTIFIER(MonitorLines), 0,
                                  PThread::NoAutoDeleteThread,
                                  PThread::LowPriority,
                                  "Line Monitor");
}


OpalLineEndPoint::~OpalLineEndPoint()
{
  if(NULL != monitorThread)
  {
     PTRACE(4, "LID EP\tAwaiting monitor thread termination " << GetPrefixName());
     exitFlag.Signal();
     monitorThread->WaitForTermination();
     delete monitorThread;
     monitorThread = NULL;

     /* remove all lines which has been added with AddLine or AddLinesFromDevice
        RemoveAllLines can be invoked only after the monitorThread has been destroyed,
        indeed, the monitor thread may called some function such as vpb_get_event_ch_async which may
        throw an exception if the line handle is no longer valid
     */
     RemoveAllLines();
  }
  PTRACE(4, "LID EP\tOpalLineEndPoint " << GetPrefixName() << " destroyed");
}


PBoolean OpalLineEndPoint::MakeConnection(OpalCall & call,
                                     const PString & remoteParty,
                                     void * userData,
                                     unsigned int /*options*/,
                                     OpalConnection::StringOptions *)
{
  PTRACE(3, "LID EP\tMakeConnection to " << remoteParty);  

  // Then see if there is a specific line mentioned in the prefix, e.g 123456@vpb:1/2
  PINDEX prefixLength = GetPrefixName().GetLength();
  bool terminating = (remoteParty.Left(prefixLength) *= "pots");

  PString number, lineName;
  PINDEX at = remoteParty.Find('@');
  if (at != P_MAX_INDEX) {
    number = remoteParty(prefixLength+1, at-1);
    lineName = remoteParty.Mid(at + 1);
  }
  else {
    if (terminating)
      lineName = remoteParty.Mid(prefixLength + 1);
    else
      number = remoteParty.Mid(prefixLength + 1);
  }

  if (lineName.IsEmpty())
    lineName = '*';
  
  PTRACE(3,"LID EP\tMakeConnection line = \"" << lineName << "\", number = \"" << number << '"');
  
  // Locate a line
  OpalLine * line = GetLine(lineName, true, terminating);
  if (line == NULL){
    PTRACE(1,"LID EP\tMakeConnection cannot find the line \"" << lineName << '"');
    line = GetLine(defaultLine, true, terminating);
  }  
  if (line == NULL){
    PTRACE(1,"LID EP\tMakeConnection cannot find the default line " << defaultLine);
    return false;

  }

  return AddConnection(CreateConnection(call, *line, userData, number));
}


OpalMediaFormatList OpalLineEndPoint::GetMediaFormats() const
{
  OpalMediaFormatList mediaFormats;
#if OPAL_VIDEO
  AddVideoMediaFormats(mediaFormats);
#endif

  PWaitAndSignal mutex(linesMutex);

  for (OpalLineList::const_iterator line = lines.begin(); line != lines.end(); ++line)
    mediaFormats += line->GetDevice().GetMediaFormats();

  return mediaFormats;
}


OpalLineConnection * OpalLineEndPoint::CreateConnection(OpalCall & call,
                                                       OpalLine & line,
                                                       void * /*userData*/,
                                                       const PString & number)
{
  PTRACE(3, "LID EP\tCreateConnection call = " << call << " line = " << line << " number = " << number);
  return new OpalLineConnection(call, *this, line, number);
}


static bool InitialiseLine(OpalLine * line)
{
  PTRACE(3, "LID EP\tInitialiseLine " << *line);
  line->Ring(0, NULL);
  line->StopTone();
  line->StopReading();
  line->StopWriting();

  if (!line->DisableAudio())
    return false;

  for (unsigned lnum = 0; lnum < line->GetDevice().GetLineCount(); lnum++) {
    if (lnum != line->GetLineNumber())
      line->GetDevice().SetLineToLineDirect(lnum, line->GetLineNumber(), false);
  }

  return true;
}


PBoolean OpalLineEndPoint::AddLine(OpalLine * line)
{
  if (PAssertNULL(line) == NULL)
    return false;

  if (!line->GetDevice().IsOpen())
    return false;

  if (!InitialiseLine(line))
    return false;

  linesMutex.Wait();
  lines.Append(line);
  linesMutex.Signal();

  return true;
}


void OpalLineEndPoint::RemoveLine(OpalLine * line)
{
  linesMutex.Wait();
  lines.Remove(line);
  linesMutex.Signal();
}


void OpalLineEndPoint::RemoveLine(const PString & token)
{
  linesMutex.Wait();
  OpalLineList::iterator line = lines.begin();
  while (line != lines.end()) {
    if (line->GetToken() *= token)
      lines.erase(line++);
    else
      ++line;
  }
  linesMutex.Signal();
}


void OpalLineEndPoint::RemoveAllLines()
{
  linesMutex.Wait();
  lines.RemoveAll();
  devices.RemoveAll();
  linesMutex.Signal();
}   
    
PBoolean OpalLineEndPoint::AddLinesFromDevice(OpalLineInterfaceDevice & device)
{
  if (!device.IsOpen()){
    PTRACE(1, "LID EP\tAddLinesFromDevice device " << device.GetDeviceName() << "is not opened");
    return false;
  }  

  unsigned lineCount = device.GetLineCount();
  PTRACE(3, "LID EP\tAddLinesFromDevice device " << device.GetDeviceName() << " has " << lineCount << " lines");
  if (lineCount == 0)
    return false;

  bool atLeastOne = false;

  for (unsigned line = 0; line < lineCount; line++) {
    OpalLine * newLine = new OpalLine(device, line);
    if (InitialiseLine(newLine)) {
      atLeastOne = true;
      linesMutex.Wait();
      lines.Append(newLine);
      linesMutex.Signal();
      PTRACE(3, "LID EP\tAddded line  " << line << ", " << (device.IsLineTerminal(line) ? "terminal" : "network"));
    }
    else {
      delete newLine;
      PTRACE(3, "LID EP\tCould not add line  " << line << ", " << (device.IsLineTerminal(line) ? "terminal" : "network"));
    }
  }

  return atLeastOne;
}


void OpalLineEndPoint::RemoveLinesFromDevice(OpalLineInterfaceDevice & device)
{
  linesMutex.Wait();
  OpalLineList::iterator line = lines.begin();
  while (line != lines.end()) {
    if (line->GetToken().Find(device.GetDeviceName()) == 0)
      lines.erase(line++);
    else
      ++line;
  }
  linesMutex.Signal();
}


PBoolean OpalLineEndPoint::AddDeviceNames(const PStringArray & descriptors)
{
  PBoolean ok = false;

  for (PINDEX i = 0; i < descriptors.GetSize(); i++) {
    if (AddDeviceName(descriptors[i]))
      ok = true;
  }

  return ok;
}


PBoolean OpalLineEndPoint::AddDeviceName(const PString & descriptor)
{
  PString deviceType, deviceName;

  PINDEX colon = descriptor.Find(':');
  if (colon != P_MAX_INDEX) {
    deviceType = descriptor.Left(colon).Trim();
    deviceName = descriptor.Mid(colon+1).Trim();
  }

  if (deviceType.IsEmpty() || deviceName.IsEmpty()) {
    PTRACE(1, "LID EP\tInvalid device description \"" << descriptor << '"');
    return false;
  }

  // Make sure not already there.
  linesMutex.Wait();
  for (OpalLIDList::iterator iterDev = devices.begin(); iterDev != devices.end(); ++iterDev) {
    if (iterDev->GetDeviceType() == deviceType && iterDev->GetDeviceName() == deviceName) {
      linesMutex.Signal();
      PTRACE(3, "LID EP\tDevice " << deviceType << ':' << deviceName << " already loaded.");
      return true;
    }
  }
  linesMutex.Signal();

  // Not there so add it.
  OpalLineInterfaceDevice * device = OpalLineInterfaceDevice::Create(deviceType);
  if (device == NULL) {
    PTRACE(1, "LID EP\tDevice type " << deviceType << " could not be created.");
    return false;
  }

  if (device->Open(deviceName))
    return AddDevice(device);

  delete device;
  PTRACE(1, "LID EP\tDevice " << deviceType << ':' << deviceName << " could not be opened.");
  return false;
}


PBoolean OpalLineEndPoint::AddDevice(OpalLineInterfaceDevice * device)
{
  if (PAssertNULL(device) == NULL)
    return false;

  linesMutex.Wait();
  devices.Append(device);
  linesMutex.Signal();
  return AddLinesFromDevice(*device);
}


void OpalLineEndPoint::RemoveDevice(OpalLineInterfaceDevice * device)
{
  if (PAssertNULL(device) == NULL)
    return;

  RemoveLinesFromDevice(*device);
  linesMutex.Wait();
  devices.Remove(device);
  linesMutex.Signal();
}


OpalLine * OpalLineEndPoint::GetLine(const PString & lineName, bool enableAudio, bool terminating)
{
  PWaitAndSignal mutex(linesMutex);

  PTRACE(3, "LID EP\tGetLine " << lineName << ", enableAudio=" << enableAudio << ", terminating=" << terminating);
  for (OpalLineList::iterator line = lines.begin(); line != lines.end(); ++line) {
    PString lineDescription = line->GetDescription();

    PTRACE(3, "LID EP\tGetLine description = \"" << lineDescription << "\", enableAudio = " << line->IsAudioEnabled());

    /* if the line description contains the endpoint prefix, strip it*/
    if (lineDescription.Find(GetPrefixName()) == 0)
      lineDescription.Delete(0, GetPrefixName().GetLength() + 1);

    if ((lineName == defaultLine || lineDescription == lineName) &&
        (line->IsTerminal() == terminating) &&
        (!enableAudio || line->EnableAudio())){
      PTRACE(3, "LID EP\tGetLine found the line \"" << lineName << '"');
      return &*line;
    }  
  }

  return NULL;
}


void OpalLineEndPoint::SetDefaultLine(const PString & lineName)
{
  PTRACE(3, "LID EP\tSetDefaultLine " << lineName);
  linesMutex.Wait();
  defaultLine = lineName;
  linesMutex.Signal();
}


void OpalLineEndPoint::MonitorLines(PThread &, INT)
{
  PTRACE(4, "LID EP\tMonitor thread started for " << GetPrefixName());

  while (!exitFlag.Wait(100)) {
    linesMutex.Wait();
    for (OpalLineList::iterator line = lines.begin(); line != lines.end(); ++line)
      MonitorLine(*line);
    linesMutex.Signal();
  }

  PTRACE(4, "LID EP\tMonitor thread stopped for " << GetPrefixName());
}


void OpalLineEndPoint::MonitorLine(OpalLine & line)
{
  PSafePtr<OpalLineConnection> connection = GetLIDConnectionWithLock(line.GetToken(), PSafeReference);
  if (connection != NULL) {
    // Are still in a call, pass hook state it to the connection object for handling
    connection->Monitor(!line.IsDisconnected());
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

  // Get new instance of a call, abort if none created
  OpalCall * call = manager.InternalCreateCall();
  if (call == NULL) {
    line.DisableAudio();
    return;
  }

  // Have incoming ring, create a new LID connection and let it handle it
  connection = CreateConnection(*call, line, NULL, PString::Empty());
  if (AddConnection(connection))
    connection->StartIncoming();
}


PBoolean OpalLineEndPoint::OnSetUpConnection(OpalLineConnection & PTRACE_PARAM(connection))
{
  PTRACE(3, "LID EP\tOnSetUpConnection" << connection);
  return true;
}


/////////////////////////////////////////////////////////////////////////////

OpalLineConnection::OpalLineConnection(OpalCall & call,
                                       OpalLineEndPoint & ep,
                                       OpalLine & ln,
                                       const PString & number)
  : OpalConnection(call, ep, ln.GetToken()),
    endpoint(ep),
    line(ln)
{
  localPartyName = ln.GetToken();
  remotePartyNumber = number.Right(number.Find(':'));
  silenceDetector = new OpalLineSilenceDetector(line);

  answerRingCount = 3;
  requireTonesForDial = true;
  wasOffHook = false;
  handlerThread = NULL;

  m_uiDialDelay = 0;
  PTRACE(3, "LID Con\tConnection " << callToken << " created to " << (number.IsEmpty() ? "local" : number));
  
}


void OpalLineConnection::OnReleased()
{
  PTRACE(3, "LID Con\tOnReleased " << *this);

  if (handlerThread != NULL) {
    PTRACE(4, "LID Con\tAwaiting handler thread termination " << *this);
    // Stop the signalling handler thread
    SetUserInput(PString()); // Break out of ReadUserInput
    handlerThread->WaitForTermination();
    delete handlerThread;
    handlerThread = NULL;
  }

  if (line.IsOffHook()) {
    if (line.PlayTone(OpalLineInterfaceDevice::ClearTone))
      PTRACE(3, "LID Con\tPlaying clear tone until handset onhook");
    else
      PTRACE(2, "LID Con\tCould not play clear tone until handset onhook");
    line.SetOnHook();
  }
  line.Ring(0, NULL);

  phase = ReleasedPhase;

  OpalConnection::OnReleased();
}


PString OpalLineConnection::GetDestinationAddress()
{
  return ReadUserInput();
}

PBoolean OpalLineConnection::OnSetUpConnection()
{
  PTRACE(3, "LID Con\tOnSetUpConnection");
  return endpoint.OnSetUpConnection(*this);
}

PBoolean OpalLineConnection::SetAlerting(const PString & calleeName, PBoolean)
{
  PTRACE(3, "LID Con\tSetAlerting " << *this);

  if (GetPhase() != SetUpPhase) 
    return false;

  // switch phase 
  phase = AlertingPhase;
  alertingTime = PTime();

  line.SetCallerID(calleeName);

  if (line.PlayTone(OpalLineInterfaceDevice::RingTone))
    PTRACE(3, "LID Con\tPlaying ring tone");
  else
    PTRACE(2, "LID Con\tCould not play ring tone");

  return true;
}


PBoolean OpalLineConnection::SetConnected()
{
  PTRACE(3, "LID Con\tSetConnected " << *this);

  if (GetPhase() >= ConnectedPhase)
    return false;

  // switch phase 
  phase = ConnectedPhase;
  connectedTime = PTime();

  return line.StopTone();
}


OpalMediaFormatList OpalLineConnection::GetMediaFormats() const
{
  OpalMediaFormatList formats = line.GetDevice().GetMediaFormats();
#if OPAL_VIDEO
  AddVideoMediaFormats(formats);
#endif
  return formats;
}


OpalMediaStream * OpalLineConnection::CreateMediaStream(const OpalMediaFormat & mediaFormat,
                                                        unsigned sessionID,
                                                        PBoolean isSource)
{
  if (sessionID != OpalMediaFormat::DefaultAudioSessionID)
    return OpalConnection::CreateMediaStream(mediaFormat, sessionID, isSource);

  return new OpalLineMediaStream(*this, mediaFormat, sessionID, isSource, line);
}


PBoolean OpalLineConnection::OnOpenMediaStream(OpalMediaStream & mediaStream)
{
  if (!OpalConnection::OnOpenMediaStream(mediaStream))
    return false;

  if (mediaStream.IsSource()) {
    OpalMediaPatch * patch = mediaStream.GetPatch();
    if (patch != NULL) {
      silenceDetector->SetParameters(endpoint.GetManager().GetSilenceDetectParams());
      patch->AddFilter(silenceDetector->GetReceiveHandler(), line.GetReadFormat());
    }
  }

  return true;
}


PBoolean OpalLineConnection::SetAudioVolume(PBoolean source, unsigned percentage)
{
  PSafePtr<OpalLineMediaStream> stream = PSafePtrCast<OpalMediaStream, OpalLineMediaStream>(GetMediaStream(OpalMediaFormat::DefaultAudioSessionID, source));
  if (stream == NULL)
    return false;

  OpalLine & line = stream->GetLine();
  return source ? line.SetRecordVolume(percentage) : line.SetPlayVolume(percentage);
}


unsigned OpalLineConnection::GetAudioSignalLevel(PBoolean source)
{
  PSafePtr<OpalLineMediaStream> stream = PSafePtrCast<OpalMediaStream, OpalLineMediaStream>(GetMediaStream(OpalMediaFormat::DefaultAudioSessionID, source));
  if (stream == NULL)
    return UINT_MAX;

  OpalLine & line = stream->GetLine();
  return line.GetAverageSignalLevel(!source);
}


PBoolean OpalLineConnection::SendUserInputString(const PString & value)
{
  return line.PlayDTMF(value);
}


PBoolean OpalLineConnection::SendUserInputTone(char tone, int duration)
{
  if (duration <= 0)
    return line.PlayDTMF(&tone);
  else
    return line.PlayDTMF(&tone, duration);
}


PBoolean OpalLineConnection::PromptUserInput(PBoolean play)
{
  PTRACE(3, "LID Con\tConnection " << callToken << " dial tone " << (play ? "started" : "stopped"));

  if (play) {
    if (line.PlayTone(OpalLineInterfaceDevice::DialTone)) {
      PTRACE(3, "LID Con\tPlaying dial tone");
      return true;
    }
    PTRACE(2, "LID Con\tCould not dial ring tone");
    return false;
  }

  line.StopTone();
  return true;
}


void OpalLineConnection::StartIncoming()
{
  if (handlerThread == NULL)
    handlerThread = PThread::Create(PCREATE_NOTIFIER(HandleIncoming), 0,
                                    PThread::NoAutoDeleteThread,
                                    PThread::LowPriority,
                                    "Line Connection:%x");
}


void OpalLineConnection::Monitor(PBoolean offHook)
{
  if (wasOffHook != offHook) {
    PSafeLockReadWrite mutex(*this);

    wasOffHook = offHook;
    PTRACE(3, "LID Con\tConnection " << callToken << " " << (offHook ? "off" : "on") << " hook: phase=" << phase);

    if (!offHook) {
      Release(EndedByRemoteUser);
      return;
    }

    if (IsOriginating()) {
      // Ok, they went off hook, stop ringing
      line.Ring(0, NULL);

      // If we are in alerting state then we are B-Party
      if (phase == AlertingPhase) {
        phase = ConnectedPhase;
        OnConnected();
      }
      else {
        // Otherwise we are A-Party
        if (!OnIncomingConnection(0, NULL)) {
          Release(EndedByCallerAbort);
          return;
        }

        PTRACE(3, "LID Con\tOutgoing connection " << *this << " routed to \"" << ownerCall.GetPartyB() << '"');
        if (!ownerCall.OnSetUp(*this)) {
          Release(EndedByNoAccept);
          return;
        }
      }
    }
  }

  // If we are off hook, we continually suck out DTMF tones and pass them up
  if (offHook) {
    char tone;
    while ((tone = line.ReadDTMF()) != '\0')
      OnUserInputTone(tone, 180);
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
        PTRACE(3, "LID Con\tIncoming PSTN call stopped.");
        Release(EndedByCallerAbort);
        return;
      }
      PThread::Sleep(100);
      if (phase >= ReleasingPhase)
        return;
    } while (count < answerRingCount);

    // Get caller ID
    PString callerId;
    if (line.GetCallerID(callerId, true)) {
      PStringArray words = callerId.Tokenise('\t', true);
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

  wasOffHook = true;

  if (!OnIncomingConnection(0, NULL)) {
    Release(EndedByCallerAbort);
    return;
  }

  PTRACE(3, "LID\tIncoming call routed for " << *this);
  if (!ownerCall.OnSetUp(*this))
    Release(EndedByNoAccept);
}

PBoolean OpalLineConnection::SetUpConnection()
{
  PTRACE(3, "LID Con\tSetUpConnection call on " << *this << " to \"" << remotePartyNumber << '"');

  phase = SetUpPhase;
  originating = true;

  if (line.IsTerminal()) {
    line.SetCallerID(remotePartyNumber);
    line.Ring(1, NULL);
    if (ownerCall.GetConnection(0) != this) {
      // Are B-Party, so move to alerting state
      phase = AlertingPhase;
      OnAlerting();
    }
  }
  else {
    switch (line.DialOut(remotePartyNumber, requireTonesForDial, getDialDelay())) {
      case OpalLineInterfaceDevice::DialTone :
        PTRACE(3, "LID Con\tdial tone on " << line);
        return false;

      case OpalLineInterfaceDevice::RingTone :
        PTRACE(3, "LID Con\tGot ringback on " << line);
        phase = AlertingPhase;
        OnAlerting();
        break;

      default :
        PTRACE(1, "LID Con\tError dialling " << remotePartyNumber << " on " << line);
        return false;
    }
  }

  return true;
}


///////////////////////////////////////////////////////////////////////////////

OpalLineMediaStream::OpalLineMediaStream(OpalLineConnection & conn, 
                                  const OpalMediaFormat & mediaFormat,
                                                 unsigned sessionID,
                                                     PBoolean isSource,
                                               OpalLine & ln)
  : OpalMediaStream(conn, mediaFormat, sessionID, isSource)
  , line(ln)
  , notUsingRTP(!ln.GetDevice().UsesRTP())
  , useDeblocking(false)
  , missedCount(0)
  , lastFrameWasSignal(true)
{
  lastSID[0] = 2;
}


PBoolean OpalLineMediaStream::Open()
{
  if (isOpen)
    return true;

  if (IsSource()) {
    if (!line.SetReadFormat(mediaFormat))
      return false;
  }
  else {
    if (!line.SetWriteFormat(mediaFormat))
      return false;
  }

  SetDataSize(GetDataSize());
  PTRACE(3, "LineMedia\tStream opened for " << mediaFormat << ", using "
         << (notUsingRTP ? (useDeblocking ? "reblocked audio" : "audio frames") : "direct RTP"));

  return OpalMediaStream::Open();
}


PBoolean OpalLineMediaStream::Close()
{
  if (IsSource())
    line.StopReading();
  else
    line.StopWriting();

  return OpalMediaStream::Close();
}


PBoolean OpalLineMediaStream::ReadPacket(RTP_DataFrame & packet)
{
  if (notUsingRTP)
    return OpalMediaStream::ReadPacket(packet);

  PINDEX count = packet.GetSize();
  if (!line.ReadFrame(packet.GetPointer(), count))
    return false;

  packet.SetPayloadSize(count-packet.GetHeaderSize());
  return true;
}


PBoolean OpalLineMediaStream::WritePacket(RTP_DataFrame & packet)
{
  if (notUsingRTP)
    return OpalMediaStream::WritePacket(packet);

  PINDEX written = 0;
  return line.WriteFrame(packet.GetPointer(), packet.GetHeaderSize()+packet.GetPayloadSize(), written);
}


PBoolean OpalLineMediaStream::ReadData(BYTE * buffer, PINDEX size, PINDEX & length)
{
  PAssert(notUsingRTP, PLogicError);

  length = 0;

  if (IsSink()) {
    PTRACE(1, "LineMedia\tTried to read from sink media stream");
    return false;
  }

  if (useDeblocking) {
    line.SetReadFrameSize(size);
    if (line.ReadBlock(buffer, size)) {
      length = size;
      return true;
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
            lastFrameWasSignal = false;
            break;
          case 4 :
            if ((*(BYTE *)buffer&3) == 2)
              memcpy(lastSID, buffer, 4);
            lastFrameWasSignal = false;
            break;
          default :
            lastFrameWasSignal = true;
        }
      }
      return true;
    }
  }

  PTRACE_IF(1, line.GetDevice().GetErrorNumber() != 0,
            "LineMedia\tDevice read frame error: " << line.GetDevice().GetErrorText());

  return false;
}


PBoolean OpalLineMediaStream::WriteData(const BYTE * buffer, PINDEX length, PINDEX & written)
{
  PAssert(notUsingRTP, PLogicError);

  written = 0;

  if (IsSource()) {
    PTRACE(1, "LineMedia\tTried to write to source media stream");
    return false;
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
        if (mediaFormat.GetName().Find('B') != P_MAX_INDEX) {
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
      return true;
    }
  }
  else {
    if (line.WriteFrame(buffer, length, written))
      return true;
  }

  PTRACE_IF(1, line.GetDevice().GetErrorNumber() != 0,
            "LineMedia\tLID write frame error: " << line.GetDevice().GetErrorText());

  return false;
}


PBoolean OpalLineMediaStream::SetDataSize(PINDEX dataSize)
{
  if (notUsingRTP) {
    if (IsSource())
      useDeblocking = !line.SetReadFrameSize(dataSize) || line.GetReadFrameSize() != dataSize;
    else
      useDeblocking = !line.SetWriteFrameSize(dataSize) || line.GetWriteFrameSize() != dataSize;

    PTRACE(3, "LineMedia\tStream frame size: rd="
           << line.GetReadFrameSize() << " wr="
           << line.GetWriteFrameSize() << ", "
           << (useDeblocking ? "needs" : "no") << " reblocking.");
  }

  return OpalMediaStream::SetDataSize(dataSize);
}


PBoolean OpalLineMediaStream::IsSynchronous() const
{
  return true;
}


/////////////////////////////////////////////////////////////////////////////

OpalLineSilenceDetector::OpalLineSilenceDetector(OpalLine & l)
  : line(l)
{
}


unsigned OpalLineSilenceDetector::GetAverageSignalLevel(const BYTE *, PINDEX)
{
  return line.GetAverageSignalLevel(true);
}


/////////////////////////////////////////////////////////////////////////////
