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

OpalLIDEndPoint::OpalLIDEndPoint(OpalManager & mgr,
                                 const PString & prefix,
                                 unsigned attributes)
  : OpalEndPoint(mgr, prefix, attributes),
    defaultLine("*")
{
  PTRACE(4, "LID EP\tOpalLIDEndPoint " << prefix << " created");
  monitorThread = PThread::Create(PCREATE_NOTIFIER(MonitorLines), 0,
                                  PThread::NoAutoDeleteThread,
                                  PThread::LowPriority,
                                  prefix.ToUpper() & "Line Monitor");
}


OpalLIDEndPoint::~OpalLIDEndPoint()
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
  PTRACE(4, "LID EP\tOpalLIDEndPoint " << GetPrefixName() << " destroyed");
}


PBoolean OpalLIDEndPoint::MakeConnection(OpalCall & call,
                                     const PString & remoteParty,
                                     void * userData,
                                     unsigned int /*options*/,
                                     OpalConnection::StringOptions *)
{
  PTRACE(3, "LID EP\tMakeConnection remoteParty " << remoteParty << ", prefix "<< GetPrefixName());  
  // First strip of the prefix if present
  PINDEX prefixLength = 0;
  PINDEX colonIndex = remoteParty.Find(":");
  if ((remoteParty.Find(GetPrefixName()) == 0) && (colonIndex != P_MAX_INDEX)){
    /* the remote party contains the prefix and ':' */
    prefixLength = colonIndex;
  }

  // Then see if there is a specific line mentioned in the prefix, e.g vpb@1/2:123456
  PString number, lineName;
  PINDEX at = remoteParty.Left(prefixLength).Find('@');
  if (at != P_MAX_INDEX) {
    number = remoteParty.Right(remoteParty.GetLength() - prefixLength - 1);
    lineName = remoteParty(GetPrefixName().GetLength() + 1, prefixLength - 1);
  }
  else {
    if (HasAttribute(CanTerminateCall))
      lineName = remoteParty.Mid(prefixLength + 1);
    else
      number = remoteParty.Mid(prefixLength + 1);
  }

  if (lineName.IsEmpty())
    lineName = '*';

  
  PTRACE(3,"LID EP\tMakeConnection line = \"" << lineName << "\", number = \"" << number << '"');
  
  // Locate a line
  OpalLine * line = GetLine(lineName, PTrue);
  if (line == NULL){
    PTRACE(1,"LID EP\tMakeConnection cannot find the line \"" << lineName << '"');
    line = GetLine(defaultLine, PTrue);
  }  
  if (line == NULL){
    PTRACE(1,"LID EP\tMakeConnection cannot find the default line " << defaultLine);
    return PFalse;

  }

  return AddConnection(CreateConnection(call, *line, userData, number));
}


OpalMediaFormatList OpalLIDEndPoint::GetMediaFormats() const
{
  OpalMediaFormatList mediaFormats;
#if OPAL_VIDEO
  AddVideoMediaFormats(mediaFormats);
#endif

  PWaitAndSignal mutex(linesMutex);

  for (PINDEX i = 0; i < lines.GetSize(); i++) {
    OpalMediaFormatList devFormats = lines[i].GetDevice().GetMediaFormats();
    for (PINDEX j = 0; j < devFormats.GetSize(); j++)
      mediaFormats += devFormats[j];
  }

  return mediaFormats;
}


OpalLineConnection * OpalLIDEndPoint::CreateConnection(OpalCall & call,
                                                       OpalLine & line,
                                                       void * /*userData*/,
                                                       const PString & number)
{
  PTRACE(3, "LID EP\tCreateConnection call = " << call << " line = " << line << " number = " << number);
  return new OpalLineConnection(call, *this, line, number);
}


static void InitialiseLine(OpalLine * line)
{
  PTRACE(3, "LID EP\tInitialiseLine " << *line);
  line->Ring(0, NULL);
  line->StopTone();
  line->StopReading();
  line->StopWriting();
  line->DisableAudio();

  for (unsigned lnum = 0; lnum < line->GetDevice().GetLineCount(); lnum++) {
    if (lnum != line->GetLineNumber())
      line->GetDevice().SetLineToLineDirect(lnum, line->GetLineNumber(), PFalse);
  }
}


PBoolean OpalLIDEndPoint::AddLine(OpalLine * line)
{
  if (PAssertNULL(line) == NULL)
    return PFalse;

  if (!line->GetDevice().IsOpen())
    return PFalse;

  if (line->IsTerminal() != HasAttribute(CanTerminateCall))
    return PFalse;

  InitialiseLine(line);

  linesMutex.Wait();
  lines.Append(line);
  linesMutex.Signal();

  return PTrue;
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
    
PBoolean OpalLIDEndPoint::AddLinesFromDevice(OpalLineInterfaceDevice & device)
{
  if (!device.IsOpen()){
    PTRACE(1, "LID EP\tAddLinesFromDevice device " << device.GetDeviceName() << "is not opened");
    return PFalse;
  }  

  PBoolean atLeastOne = PFalse;

  linesMutex.Wait();

  
  PTRACE(3, "LID EP\tAddLinesFromDevice device " << device.GetDeviceName() << 
      " has " << device.GetLineCount() << " lines, CanTerminateCall = " << HasAttribute(CanTerminateCall));
  for (unsigned line = 0; line < device.GetLineCount(); line++) {
    PTRACE(3, "LID EP\tAddLinesFromDevice line  " << line << ", terminal = " << device.IsLineTerminal(line));
    if (device.IsLineTerminal(line) == HasAttribute(CanTerminateCall)) {
      OpalLine * newLine = new OpalLine(device, line);
      InitialiseLine(newLine);
      lines.Append(newLine);
      atLeastOne = PTrue;
    }
  }

  linesMutex.Signal();

  return atLeastOne;
}


void OpalLIDEndPoint::RemoveLinesFromDevice(OpalLineInterfaceDevice & device)
{
  linesMutex.Wait();
  for (PINDEX i = 0; i < lines.GetSize(); i++) {
    if (lines[i].GetToken().Find(device.GetDeviceName()) == 0)
      lines.RemoveAt(i--);
  }
  linesMutex.Signal();
}


PBoolean OpalLIDEndPoint::AddDeviceNames(const PStringArray & descriptors)
{
  PBoolean ok = PFalse;

  for (PINDEX i = 0; i < descriptors.GetSize(); i++) {
    if (AddDeviceName(descriptors[i]))
      ok = PTrue;
  }

  return ok;
}


PBoolean OpalLIDEndPoint::AddDeviceName(const PString & descriptor)
{
  PINDEX colon = descriptor.Find(':');
  if (colon == P_MAX_INDEX)
    return PFalse;

  PString deviceType = descriptor.Left(colon).Trim();
  PString deviceName = descriptor.Mid(colon+1).Trim();

  // Make sure not already there.
  linesMutex.Wait();
  for (PINDEX i = 0; i < devices.GetSize(); i++) {
    if (devices[i].GetDeviceType() == deviceType && devices[i].GetDeviceName() == deviceName) {
      linesMutex.Signal();
      return PTrue;
    }
  }
  linesMutex.Signal();

  // Not there so add it.
  OpalLineInterfaceDevice * device = OpalLineInterfaceDevice::Create(deviceType);
  if (device == NULL)
    return PFalse;

  if (device->Open(deviceName))
    return AddDevice(device);

  delete device;
  return PFalse;
}


PBoolean OpalLIDEndPoint::AddDevice(OpalLineInterfaceDevice * device)
{
  if (PAssertNULL(device) == NULL)
    return PFalse;

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


OpalLine * OpalLIDEndPoint::GetLine(const PString & lineName, PBoolean enableAudio) const
{
  PWaitAndSignal mutex(linesMutex);

  PTRACE(3, "LID EP\tGetLine " << lineName << ", enableAudio = " << enableAudio);
  for (PINDEX i = 0; i < lines.GetSize(); i++) {
    PTRACE(3, "LID EP\tGetLine line " << i << ", description = \"" << lines[i].GetDescription() <<
        "\", enableAudio = " << lines[i].EnableAudio());
    PString lineDescription = lines[i].GetDescription();
    /* if the line description contains the endpoint prefix, strip it*/
    if(lineDescription.Find(GetPrefixName()) == 0){
      lineDescription.Delete(0, GetPrefixName().GetLength() + 1);
    }
    if ((lineName == defaultLine ||  lineDescription == lineName) &&
         (!enableAudio || lines[i].EnableAudio())){
      PTRACE(3, "LID EP\tGetLine found the line \"" << lineName << '"');
      return &lines[i];
    }  
  }

  return NULL;
}


void OpalLIDEndPoint::SetDefaultLine(const PString & lineName)
{
  PTRACE(3, "LID EP\tSetDefaultLine " << lineName);
  linesMutex.Wait();
  defaultLine = lineName;
  linesMutex.Signal();
}


void OpalLIDEndPoint::MonitorLines(PThread &, INT)
{
  PTRACE(4, "LID EP\tMonitor thread started for " << GetPrefixName());

  while (!exitFlag.Wait(100)) {
    linesMutex.Wait();
    for (PINDEX i = 0; i < lines.GetSize(); i++)
      MonitorLine(lines[i]);
    linesMutex.Signal();
  }

  PTRACE(4, "LID EP\tMonitor thread stopped for " << GetPrefixName());
}


void OpalLIDEndPoint::MonitorLine(OpalLine & line)
{
  PSafePtr<OpalLineConnection> connection = GetLIDConnectionWithLock(line.GetToken());
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

  // Have incoming ring, create a new LID connection and let it handle it
  connection = CreateConnection(*manager.CreateCall(NULL), line, NULL, PString::Empty());
  if (AddConnection(connection))
    connection->StartIncoming();
}


#if 1
PBoolean OpalLIDEndPoint::OnSetUpConnection(OpalLineConnection & PTRACE_PARAM(connection))
{
  PTRACE(3, "LID EP\tOnSetUpConnection" << connection);
  return PTrue;
}
#endif


/////////////////////////////////////////////////////////////////////////////

OpalLineConnection::OpalLineConnection(OpalCall & call,
                                       OpalLIDEndPoint & ep,
                                       OpalLine & ln,
                                       const PString & number)
  : OpalConnection(call, ep, ln.GetToken()),
    endpoint(ep),
    line(ln)
{
  remotePartyNumber = number.Right(number.Find(':'));
  silenceDetector = new OpalLineSilenceDetector(line);

  answerRingCount = 3;
  requireTonesForDial = PTrue;
  wasOffHook = PFalse;
  handlerThread = NULL;

  m_uiDialDelay = 0;
  PTRACE(3, "LID Con\tConnection " << callToken << " created to " << number << 
            " remotePartyNumber = " << remotePartyNumber);
  
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

  PTRACE(3, "LID Con\tPlaying clear tone until handset onhook");
  line.PlayTone(OpalLineInterfaceDevice::ClearTone);
  line.Ring(0, NULL);
  line.SetOnHook();

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
    return PFalse;

  // switch phase 
  phase = AlertingPhase;
  alertingTime = PTime();

  line.SetCallerID(calleeName);
  return line.PlayTone(OpalLineInterfaceDevice::RingTone);
}


PBoolean OpalLineConnection::SetConnected()
{
  PTRACE(3, "LID Con\tSetConnected " << *this);

  if (GetPhase() >= ConnectedPhase)
    return PFalse;

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
    return PFalse;

  if (mediaStream.IsSource()) {
    OpalMediaPatch * patch = mediaStream.GetPatch();
    if (patch != NULL) {
      silenceDetector->SetParameters(endpoint.GetManager().GetSilenceDetectParams());
      patch->AddFilter(silenceDetector->GetReceiveHandler(), line.GetReadFormat());
    }
  }

  return PTrue;
}


PBoolean OpalLineConnection::SetAudioVolume(PBoolean source, unsigned percentage)
{
  PSafePtr<OpalLineMediaStream> stream = PSafePtrCast<OpalMediaStream, OpalLineMediaStream>(GetMediaStream(OpalMediaFormat::DefaultAudioSessionID, source));
  if (stream == NULL)
    return PFalse;

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


void OpalLineConnection::Monitor(PBoolean offHook)
{
  if (wasOffHook != offHook) {
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
    if (line.GetCallerID(callerId, PTrue)) {
      PStringArray words = callerId.Tokenise('\t', PTrue);
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

  wasOffHook = PTrue;

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
  originating = PTrue;

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
        return PFalse;

      case OpalLineInterfaceDevice::RingTone :
        PTRACE(3, "LID Con\tGot ringback on " << line);
        phase = AlertingPhase;
        OnAlerting();
        break;

      default :
        PTRACE(1, "LID Con\tError dialling " << remotePartyNumber << " on " << line);
        return PFalse;
    }
  }

  return PTrue;
}


///////////////////////////////////////////////////////////////////////////////

OpalLineMediaStream::OpalLineMediaStream(OpalLineConnection & conn, 
                                  const OpalMediaFormat & mediaFormat,
                                                 unsigned sessionID,
                                                     PBoolean isSource,
                                               OpalLine & ln)
  : OpalMediaStream(conn, mediaFormat, sessionID, isSource),
    line(ln)
{
  useDeblocking = PFalse;
  missedCount = 0;
  lastSID[0] = 2;
  lastFrameWasSignal = PTrue;
}


PBoolean OpalLineMediaStream::Open()
{
  if (isOpen)
    return PTrue;

  if (IsSource()) {
    if (!line.SetReadFormat(mediaFormat))
      return PFalse;
    useDeblocking = !line.SetReadFrameSize(GetDataSize()) || line.GetReadFrameSize() != GetDataSize();
  }
  else {
    if (!line.SetWriteFormat(mediaFormat))
      return PFalse;
    useDeblocking = !line.SetWriteFrameSize(GetDataSize()) || line.GetWriteFrameSize() != GetDataSize();
  }

  PTRACE(3, "Media\tStream set to " << mediaFormat << ", frame size: rd="
         << line.GetReadFrameSize() << " wr="
         << line.GetWriteFrameSize() << ", "
         << (useDeblocking ? "needs" : "no") << " reblocking.");

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


PBoolean OpalLineMediaStream::ReadData(BYTE * buffer, PINDEX size, PINDEX & length)
{
  length = 0;

  if (IsSink()) {
    PTRACE(1, "Media\tTried to read from sink media stream");
    return PFalse;
  }

  if (useDeblocking) {
    line.SetReadFrameSize(size);
    if (line.ReadBlock(buffer, size)) {
      length = size;
      return PTrue;
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
            lastFrameWasSignal = PFalse;
            break;
          case 4 :
            if ((*(BYTE *)buffer&3) == 2)
              memcpy(lastSID, buffer, 4);
            lastFrameWasSignal = PFalse;
            break;
          default :
            lastFrameWasSignal = PTrue;
        }
      }
      return PTrue;
    }
  }

  PTRACE_IF(1, line.GetDevice().GetErrorNumber() != 0,
            "Media\tDevice read frame error: " << line.GetDevice().GetErrorText());

  return PFalse;
}


PBoolean OpalLineMediaStream::WriteData(const BYTE * buffer, PINDEX length, PINDEX & written)
{
  written = 0;

  if (IsSource()) {
    PTRACE(1, "Media\tTried to write to source media stream");
    return PFalse;
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
      return PTrue;
    }
  }
  else {
    if (line.WriteFrame(buffer, length, written))
      return PTrue;
  }

  PTRACE_IF(1, line.GetDevice().GetErrorNumber() != 0,
            "Media\tLID write frame error: " << line.GetDevice().GetErrorText());

  return PFalse;
}


PBoolean OpalLineMediaStream::SetDataSize(PINDEX dataSize)
{
  if (IsSource())
    useDeblocking = !line.SetReadFrameSize(dataSize) || line.GetReadFrameSize() != dataSize;
  else
    useDeblocking = !line.SetWriteFrameSize(dataSize) || line.GetWriteFrameSize() != dataSize;
  return OpalMediaStream::SetDataSize(dataSize);
}


PBoolean OpalLineMediaStream::IsSynchronous() const
{
  return PTrue;
}


/////////////////////////////////////////////////////////////////////////////

OpalLineSilenceDetector::OpalLineSilenceDetector(OpalLine & l)
  : line(l)
{
}


unsigned OpalLineSilenceDetector::GetAverageSignalLevel(const BYTE *, PINDEX)
{
  return line.GetAverageSignalLevel(PTrue);
}


/////////////////////////////////////////////////////////////////////////////
