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
 */

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "lidep.cxx"
#endif

#include <lids/lidep.h>

#if OPAL_LID

#include <opal/manager.h>
#include <opal/call.h>
#include <opal/patch.h>


#define new PNEW

static const char PrefixPSTN[] = "pstn";
static const char PrefixPOTS[] = "pots";


/////////////////////////////////////////////////////////////////////////////

OpalLineEndPoint::OpalLineEndPoint(OpalManager & mgr)
  : OpalEndPoint(mgr, PrefixPOTS, SupportsE164),
    m_defaultLine("*")
{
  PTRACE(4, "LID EP\tOpalLineEndPoint created");
  m_manager.AttachEndPoint(this, PrefixPSTN);
  m_monitorThread = PThread::Create(PCREATE_NOTIFIER(MonitorLines), "Line Monitor");
}


OpalLineEndPoint::~OpalLineEndPoint()
{
  m_exitFlag.Signal();
  PThread::WaitAndDelete(m_monitorThread);

  /* remove all lines which has been added with AddLine or AddLinesFromDevice
    RemoveAllLines can be invoked only after the monitorThread has been destroyed,
    indeed, the monitor thread may called some function such as vpb_get_event_ch_async which may
    throw an exception if the line handle is no longer valid
  */
  RemoveAllLines();

  PTRACE(4, "LID EP\tOpalLineEndPoint " << GetPrefixName() << " destroyed");
}


PSafePtr<OpalConnection> OpalLineEndPoint::MakeConnection(OpalCall & call,
                                                     const PString & remoteParty,
                                                              void * userData,
                                                        unsigned int /*options*/,
                                     OpalConnection::StringOptions * /*stringOptions*/)
{
  PTRACE(3, "LID EP\tMakeConnection to " << remoteParty);  

  // Then see if there is a specific line mentioned in the prefix, e.g 123456@vpb:1/2
  PINDEX prefixLength = GetPrefixName().GetLength();
  bool terminating = (remoteParty.Left(prefixLength) *= PrefixPOTS);

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
    lineName = m_defaultLine;
  
  PTRACE(3,"LID EP\tMakeConnection line = \"" << lineName << "\", number = \"" << number << '"');
  
  // Locate a line
  OpalLine * line = GetLine(lineName, true, terminating);
  if (line == NULL && lineName != m_defaultLine) {
    PTRACE(1,"LID EP\tMakeConnection cannot find the line \"" << lineName << '"');
    line = GetLine(m_defaultLine, true, terminating);
  }  
  if (line == NULL){
    PTRACE(1,"LID EP\tMakeConnection cannot find the default line " << m_defaultLine);
    return NULL;

  }

  return AddConnection(CreateConnection(call, *line, userData, number));
}


OpalMediaFormatList OpalLineEndPoint::GetMediaFormats() const
{
  OpalMediaFormatList mediaFormats = m_manager.GetCommonMediaFormats(false, false);

  PWaitAndSignal mutex(m_linesMutex);

  for (OpalLineList::const_iterator line = m_lines.begin(); line != m_lines.end(); ++line)
    mediaFormats += line->GetDevice().GetMediaFormats();

  return mediaFormats;
}


OpalLineConnection * OpalLineEndPoint::CreateConnection(OpalCall & call,
                                                       OpalLine & line,
                                                       void * /*userData*/,
                                                       const PString & number)
{
  PTRACE(3, "LID EP\tCreateConnection call = " << call << " line = \"" << line << "\", number = \"" << number << '"');
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

  m_linesMutex.Wait();
  m_lines.Append(line);
  m_linesMutex.Signal();

  return true;
}


void OpalLineEndPoint::RemoveLine(OpalLine * line)
{
  m_linesMutex.Wait();
  m_lines.Remove(line);
  m_linesMutex.Signal();
}


void OpalLineEndPoint::RemoveLine(const PString & token)
{
  m_linesMutex.Wait();
  OpalLineList::iterator line = m_lines.begin();
  while (line != m_lines.end()) {
    if (line->GetToken() *= token)
      m_lines.erase(line++);
    else
      ++line;
  }
  m_linesMutex.Signal();
}


void OpalLineEndPoint::RemoveAllLines()
{
  m_linesMutex.Wait();
  m_lines.RemoveAll();
  m_devices.RemoveAll();
  m_linesMutex.Signal();
}   
    
PBoolean OpalLineEndPoint::AddLinesFromDevice(OpalLineInterfaceDevice & device)
{
  if (!device.IsOpen()){
    PTRACE(1, "LID EP\tAddLinesFromDevice device " << device.GetDeviceName() << " is not opened");
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
      m_linesMutex.Wait();
      m_lines.Append(newLine);
      m_linesMutex.Signal();
      if (!device.IsLineTerminal(line))
        m_attributes |= IsNetworkEndPoint;
      PTRACE(3, "LID EP\tAdded line  " << line << ", " << (device.IsLineTerminal(line) ? "terminal" : "network"));
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
  m_linesMutex.Wait();
  OpalLineList::iterator line = m_lines.begin();
  while (line != m_lines.end()) {
    if (line->GetToken().Find(device.GetDeviceName()) == 0)
      m_lines.erase(line++);
    else
      ++line;
  }
  m_linesMutex.Signal();
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


const OpalLineInterfaceDevice * OpalLineEndPoint::GetDeviceByName(const PString & descriptor)
{
  PString deviceType, deviceName;
  if (!descriptor.Split(':', deviceType, deviceName, PString::SplitTrim|PString::SplitNonEmpty)) {
    PTRACE(1, "LID EP\tInvalid device description \"" << descriptor << '"');
    return NULL;
  }

  // Make sure not already there.
  PWaitAndSignal mutex(m_linesMutex);
  for (OpalLIDList::iterator iterDev = m_devices.begin(); iterDev != m_devices.end(); ++iterDev) {
    if (iterDev->GetDeviceType() == deviceType && iterDev->GetDeviceName() == deviceName) {
      PTRACE(3, "LID EP\tDevice " << deviceType << ':' << deviceName << " is loaded.");
      return &*iterDev;
    }
  }

  return NULL;
}


PBoolean OpalLineEndPoint::AddDeviceName(const PString & descriptor)
{
  if (GetDeviceByName(descriptor) != NULL)
    return true;

  // Not there so add it.
  OpalLineInterfaceDevice * device = OpalLineInterfaceDevice::CreateAndOpen(descriptor);
  if (device != NULL)
    return AddDevice(device);

  PTRACE(1, "LID EP\tDevice " << descriptor << " could not be created or opened.");
  return false;
}


PBoolean OpalLineEndPoint::AddDevice(OpalLineInterfaceDevice * device)
{
  if (PAssertNULL(device) == NULL)
    return false;

  m_linesMutex.Wait();
  m_devices.Append(device);
  m_linesMutex.Signal();
  return AddLinesFromDevice(*device);
}


void OpalLineEndPoint::RemoveDevice(OpalLineInterfaceDevice * device)
{
  if (PAssertNULL(device) == NULL)
    return;

  RemoveLinesFromDevice(*device);
  m_linesMutex.Wait();
  m_devices.Remove(device);
  m_linesMutex.Signal();
}


OpalLine * OpalLineEndPoint::GetLine(const PString & lineName, bool enableAudio, bool terminating)
{
  PWaitAndSignal mutex(m_linesMutex);

  PTRACE(4, "LID EP\tGetLine " << lineName << ", enableAudio=" << enableAudio << ", terminating=" << terminating);
  for (OpalLineList::iterator line = m_lines.begin(); line != m_lines.end(); ++line) {
    PString lineToken = line->GetToken();
    if (lineName != m_defaultLine && lineToken != lineName)
      PTRACE(4, "LID EP\tNo match to line name=\"" << lineToken << "\", default=" << m_defaultLine);
    else if (line->IsTerminal() != terminating)
      PTRACE(4, "LID EP\tNo match to line name=\"" << lineToken << "\", terminating=" << line->IsTerminal());
    else if (!line->IsPresent())
      PTRACE(4, "LID EP\tNo match to line name=\"" << lineToken << "\", not present");
    else if (enableAudio && (line->IsAudioEnabled() || !line->EnableAudio()))
      PTRACE(4, "LID EP\tNo match to line name=\"" << lineToken << "\", enableAudio=" << line->IsAudioEnabled());
    else {
      PTRACE(3, "LID EP\tGetLine found the line \"" << lineName << '"');
      return &*line;
    }  
  }

  PTRACE(3, "LID EP\tGetLine could not find/enable \"" << lineName << '"');
  return NULL;
}


void OpalLineEndPoint::SetDefaultLine(const PString & lineName)
{
  PTRACE(3, "LID EP\tSetDefaultLine " << lineName);
  m_linesMutex.Wait();
  m_defaultLine = lineName;
  m_linesMutex.Signal();
}


bool OpalLineEndPoint::SetCountryCode(OpalLineInterfaceDevice::T35CountryCodes country)
{
  PWaitAndSignal mutex(m_linesMutex);
  for (OpalLIDList::iterator iterDev = m_devices.begin(); iterDev != m_devices.end(); ++iterDev) {
    if (!iterDev->SetCountryCode(country))
      return false;
  }

  return true;
}


bool OpalLineEndPoint::SetCountryCodeName(const PString & countryName)
{
  PWaitAndSignal mutex(m_linesMutex);
  for (OpalLIDList::iterator iterDev = m_devices.begin(); iterDev != m_devices.end(); ++iterDev) {
    if (!iterDev->SetCountryCodeName(countryName))
      return false;
  }

  return true;
}


void OpalLineEndPoint::MonitorLines(PThread &, P_INT_PTR)
{
  PTRACE(4, "LID EP\tMonitor thread started for " << GetPrefixName());

  while (!m_exitFlag.Wait(100)) {
    m_linesMutex.Wait();
    for (OpalLineList::iterator line = m_lines.begin(); line != m_lines.end(); ++line)
      MonitorLine(*line);
    m_linesMutex.Signal();
  }

  PTRACE(4, "LID EP\tMonitor thread stopped for " << GetPrefixName());
}


void OpalLineEndPoint::MonitorLine(OpalLine & line)
{
  PSafePtr<OpalLineConnection> connection = GetLIDConnectionWithLock(line.GetToken(), PSafeReference);
  if (connection != NULL) {
    // Are still in a call, pass hook state it to the connection object for handling
    connection->Monitor();
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
  OpalCall * call = m_manager.InternalCreateCall();
  if (call == NULL) {
    line.DisableAudio();
    return;
  }

  // Have incoming ring, create a new LID connection and let it handle it
  connection = CreateConnection(*call, line, NULL, "Unknown");
  if (AddConnection(connection))
    connection->StartIncoming();
}


/////////////////////////////////////////////////////////////////////////////

OpalLineConnection::OpalLineConnection(OpalCall & call,
                                       OpalLineEndPoint & ep,
                                       OpalLine & ln,
                                       const PString & number)
  : OpalConnection(call, ep, ln.GetToken())
  , m_endpoint(ep)
  , m_line(ln)
  , m_wasOffHook(false)
  , m_minimumRingCount(2)
  , m_promptTone(OpalLineInterfaceDevice::DialTone)
  , m_handlerThread(NULL)
{
  m_localPartyName = ln.GetToken();
  m_remotePartyNumber = number.Right(number.FindSpan("0123456789*#,"));
  m_remotePartyName = number;
  m_remotePartyURL = GetPrefixName() + ':';
  if (!number.IsEmpty())
    m_remotePartyURL += number + '@';
  m_remotePartyURL += GetToken();

  m_silenceDetector = new OpalLineSilenceDetector(m_line, (m_endpoint.GetManager().GetSilenceDetectParams()));

  PTRACE(3, "LID Con\tConnection " << m_callToken << " created to " << (number.IsEmpty() ? "local" : number));
  
}


void OpalLineConnection::OnReleased()
{
  PTRACE(3, "LID Con\tOnReleased " << *this);

  // Stop the signalling handler thread
  SetUserInput(PString()); // Break out of ReadUserInput
  PThread::WaitAndDelete(m_handlerThread);

  if (m_line.IsTerminal()) {
    if (m_line.IsOffHook()) {
      if (m_line.PlayTone(OpalLineInterfaceDevice::ClearTone))
        PTRACE(3, "LID Con\tPlaying clear tone until handset onhook");
      else
        PTRACE(2, "LID Con\tCould not play clear tone!");
    }
    m_line.Ring(0, NULL);
  }
  else
    m_line.SetOnHook();

  OpalConnection::OnReleased();
}


PString OpalLineConnection::GetDestinationAddress()
{
  PTRACE(2, "OpalLineConnection::GetDestinationAddress called: " << m_line.IsTerminal() << " " << m_dialedNumber);
  return m_line.IsTerminal() ? m_dialedNumber : GetLocalPartyName();
}


PBoolean OpalLineConnection::SetAlerting(const PString & /*calleeName*/, PBoolean /*withMedia*/)
{
  PTRACE(3, "LID Con\tSetAlerting " << *this);

  if (GetPhase() >= AlertingPhase) 
    return false;

  // switch phase 
  SetPhase(AlertingPhase);

  if (m_line.IsTerminal() && GetMediaStream(OpalMediaType::Audio(), false) == NULL) {
    // Start ringing if we don't have an audio media stream
    if (m_line.PlayTone(OpalLineInterfaceDevice::RingTone))
      PTRACE(3, "LID Con\tPlaying ring tone");
    else
      PTRACE(2, "LID Con\tCould not play ring tone");
  }

  return true;
}


PBoolean OpalLineConnection::SetConnected()
{
  PTRACE(3, "LID Con\tSetConnected " << *this);

  if (!m_line.StopTone()) {
    PTRACE(1, "LID Con\tCould not stop tone on " << *this);
    return false;
  }

  if (m_line.IsTerminal()) {
    if (!m_line.SetConnected()) {
      PTRACE(1, "LID Con\tCould not set line to connected mode on " << *this);
      return false;
    }
  }
  else {
    if (!m_line.SetOffHook()) {
      PTRACE(1, "LID Con\tCould not set line off hook on " << *this);
      return false;
    }
    PTRACE(4, "LID Con\tAnswered call - gone off hook.");
    m_wasOffHook = true;
  }

  AutoStartMediaStreams();
  return OpalConnection::SetConnected();
}


OpalMediaFormatList OpalLineConnection::GetMediaFormats() const
{
  OpalMediaFormatList formats = m_endpoint.GetManager().GetCommonMediaFormats(false, false);
  formats += m_line.GetDevice().GetMediaFormats();
  return formats;
}


OpalMediaStream * OpalLineConnection::CreateMediaStream(const OpalMediaFormat & mediaFormat,
                                                        unsigned sessionID,
                                                        PBoolean isSource)
{
  OpalMediaFormatList formats = m_line.GetDevice().GetMediaFormats();
  if (formats.FindFormat(mediaFormat) == formats.end())
    return OpalConnection::CreateMediaStream(mediaFormat, sessionID, isSource);

  return new OpalLineMediaStream(*this, mediaFormat, sessionID, isSource, m_line);
}


PBoolean OpalLineConnection::OnOpenMediaStream(OpalMediaStream & mediaStream)
{
  if (!OpalConnection::OnOpenMediaStream(mediaStream))
    return false;

  if (mediaStream.IsSource())
    mediaStream.AddFilter(m_silenceDetector->GetReceiveHandler(), m_line.GetReadFormat());

  m_line.StopTone(); // In case a RoutingTone or RingTone is going
  return true;
}


void OpalLineConnection::OnClosedMediaStream(const OpalMediaStream & mediaStream)
{
  mediaStream.RemoveFilter(m_silenceDetector->GetReceiveHandler(), m_line.GetReadFormat());

  OpalConnection::OnClosedMediaStream(mediaStream);
}


PBoolean OpalLineConnection::SetAudioVolume(PBoolean source, unsigned percentage)
{
  PSafePtr<OpalLineMediaStream> stream = PSafePtrCast<OpalMediaStream, OpalLineMediaStream>(GetMediaStream(OpalMediaType::Audio(), source));
  if (stream == NULL)
    return false;

  OpalLine & line = stream->GetLine();
  return source ? line.SetRecordVolume(percentage) : line.SetPlayVolume(percentage);
}


unsigned OpalLineConnection::GetAudioSignalLevel(PBoolean source)
{
  PSafePtr<OpalLineMediaStream> stream = PSafePtrCast<OpalMediaStream, OpalLineMediaStream>(GetMediaStream(OpalMediaType::Audio(), source));
  if (stream == NULL)
    return UINT_MAX;

  OpalLine & line = stream->GetLine();
  return line.GetAverageSignalLevel(!source);
}


PBoolean OpalLineConnection::SendUserInputString(const PString & value)
{
  return m_line.PlayDTMF(value);
}


PBoolean OpalLineConnection::SendUserInputTone(char tone, int duration)
{
  if (duration <= 0)
    return m_line.PlayDTMF(&tone);
  else
    return m_line.PlayDTMF(&tone, duration);
}


PBoolean OpalLineConnection::PromptUserInput(PBoolean play)
{
  PTRACE(3, "LID Con\tConnection " << m_callToken << " dial tone " << (play ? "started" : "stopped"));

  if (play) {
    if (m_line.PlayTone(m_promptTone)) {
      PTRACE(3, "LID Con\tPlaying dial tone");
      return true;
    }
    PTRACE(2, "LID Con\tCould not start dial tone");
    return false;
  }

  m_line.StopTone();
  return true;
}


void OpalLineConnection::StartIncoming()
{
  if (m_handlerThread == NULL)
    m_handlerThread = PThread::Create(PCREATE_NOTIFIER(HandleIncoming), "Line Connection");
}


void OpalLineConnection::Monitor()
{
  bool offHook = !m_line.IsDisconnected();
  if (m_wasOffHook != offHook) {
    PSafeLockReadWrite mutex(*this);

    m_wasOffHook = offHook;
    PTRACE(3, "LID Con\tConnection " << m_callToken << " " << (offHook ? "off" : "on") << " hook: phase=" << GetPhase());

    if (!offHook) {
      Release(EndedByRemoteUser);
      return;
    }

    if (IsOriginating() && m_line.IsTerminal()) {
      // Ok, they went off hook, stop ringing
      m_line.Ring(0, NULL);

      if (GetPhase() != AlertingPhase)
        StartIncoming(); // We are A-party
      else {
        // If we are in alerting state then we are B-Party
        AutoStartMediaStreams();
        InternalOnConnected();
      }
    }
  }

  // If we are off hook, we continually suck out DTMF tones plus various other tones and signals and pass them up
  if (offHook) {
    switch (m_line.IsToneDetected()) {
      case OpalLineInterfaceDevice::CNGTone :
        OnUserInputTone('X', 100);
        break;

      case OpalLineInterfaceDevice::CEDTone :
        OnUserInputTone('Y', 100);
        break;

      default :
        break;
    }

    if (m_line.HasHookFlash())
      OnUserInputTone('!', 100);

    char tone;
    while ((tone = m_line.ReadDTMF()) != '\0')
      OnUserInputTone(tone, 180);
  }
  else {
    // Check for incoming PSTN ring stopping
    if (GetPhase() == AlertingPhase && !m_line.IsTerminal() && m_line.GetRingCount() == 0)
      Release(EndedByCallerAbort);
  }
}


void OpalLineConnection::HandleIncoming(PThread &, P_INT_PTR)
{
  PTRACE(3, "LID Con\tHandling incoming call on " << *this);

  SetPhase(SetUpPhase);

  if (m_line.IsTerminal()) {
    PTRACE(3, "LID Con\tWaiting for number to be entered");
    m_wasOffHook = true;
    m_dialedNumber = ReadUserInput();
    PTRACE(3, "LID Con\tEntered number is '" << m_dialedNumber << "'");
  }
  else {
    PTRACE(4, "LID Con\tCounting rings.");
    // Count incoming rings
    unsigned count;
    do {
      count = m_line.GetRingCount();
      if (count == 0) {
        PTRACE(3, "LID Con\tIncoming PSTN call stopped.");
        Release(EndedByCallerAbort);
        return;
      }
      PThread::Sleep(100);
      if (IsReleased())
        return;
    } while (count < m_minimumRingCount); // Wait till we have CLID

    // Get caller ID
    PString callerId;
    if (m_line.GetCallerID(callerId, true)) {
      PStringArray words = callerId.Tokenise('\t', true);
      if (words.GetSize() < 3) {
        PTRACE(2, "LID Con\tMalformed caller ID \"" << callerId << '"');
      }
      else {
        PTRACE(3, "LID Con\tDetected Caller ID \"" << callerId << '"');
        m_remotePartyNumber = words[0].Trim();
        m_remotePartyName = words[2].Trim();
        if (m_remotePartyName.IsEmpty())
          m_remotePartyName = m_remotePartyNumber;
      }
    }
    else {
      PTRACE(3, "LID Con\tNo caller ID available.");
    }
    if (m_remotePartyName.IsEmpty())
      m_remotePartyName = "Unknown";

    // switch phase 
    SetPhase(AlertingPhase);
  }

  if (!OnIncomingConnection(0, NULL)) {
    PTRACE(3, "LID\tWaiting for RING to stop on " << *this);
    while (m_line.GetRingCount() > 0) {
      if (IsReleased())
        return;
      PThread::Sleep(100);
    }
    Release(EndedByCallerAbort);
    return;
  }

  PTRACE(3, "LID Con\tRouted to \"" << m_ownerCall.GetPartyB() << "\", "
         << (IsOriginating() ? "outgo" : "incom") << "ing connection " << *this);
  if (m_ownerCall.OnSetUp(*this) && m_line.IsTerminal() && GetPhase() < AlertingPhase)
    m_line.PlayTone(OpalLineInterfaceDevice::RoutingTone);
}


PString OpalLineConnection::GetPrefixName() const
{
  return m_line.IsTerminal() ? PrefixPOTS : PrefixPSTN;
}


PBoolean OpalLineConnection::SetUpConnection()
{
  PTRACE(3, "LID Con\tSetUpConnection call on " << *this << " to \"" << m_remotePartyNumber << '"');

  SetPhase(SetUpPhase);
  InternalSetAsOriginating();

  if (m_line.IsTerminal()) {
    PSafePtr<OpalConnection> partyA = m_ownerCall.GetConnection(0);
    if (partyA != this) {
      // Are B-Party, so set caller ID and move to alerting state
      m_line.SetCallerID(partyA->GetRemotePartyNumber() + "\t\t" + partyA->GetRemotePartyName());
      SetPhase(AlertingPhase);
      OnAlerting();
    }
    return m_line.Ring(1, NULL);
  }

  if (m_remotePartyNumber.IsEmpty()) {
    if (!m_line.SetOffHook()) {
      PTRACE(1, "LID Con\tCould not go off hook");
      return false;
    }

    PTRACE(3, "LID Con\tNo remote party indicated, going off hook without dialing.");
    AutoStartMediaStreams();
    InternalOnConnected();
    return true;
  }

  switch (m_line.DialOut(m_remotePartyNumber, m_dialParams)) {
    case OpalLineInterfaceDevice::RingTone :
      break;

    case OpalLineInterfaceDevice::DialTone :
      PTRACE(3, "LID Con\tNo dial tone on " << m_line);
      return false;

    case OpalLineInterfaceDevice::BusyTone :
      PTRACE(3, "LID Con\tBusy tone on " << m_line);
      Release(OpalConnection::EndedByRemoteBusy);
      return false;

    default :
      PTRACE(1, "LID Con\tError dialling " << m_remotePartyNumber << " on " << m_line);
      Release(OpalConnection::EndedByConnectFail);
      return false;
  }

  PTRACE(3, "LID Con\tGot ring back on " << m_line);
  // Start media before the OnAlerting to get progress tones (e.g. SIP 183 response)
  AutoStartMediaStreams();
  SetPhase(AlertingPhase);
  OnAlerting();

  // Wait for connection
  if (m_dialParams.m_progressTimeout == 0) {
    InternalOnConnected();
    return true;
  }

  PTRACE(3, "LID Con\tWaiting " << m_dialParams.m_progressTimeout << "ms for connection on line " << m_line);
  PTimer timeout(m_dialParams.m_progressTimeout);
  while (timeout.IsRunning()) {
    if (GetPhase() != AlertingPhase) // Check for external connection release
      return false;

    if (m_line.IsConnected()) {
      InternalOnConnected();
      return true;
    }

    if (m_line.IsToneDetected() == OpalLineInterfaceDevice::BusyTone) {
      Release(OpalConnection::EndedByRemoteBusy);
      return false;
    }

    PThread::Sleep(100);
  }

  PTRACE(2, "LID Con\tConnection not detected ("
         << (m_dialParams.m_requireTones ? "required" : "optional")
         << ") on line " << m_line);

  if (m_dialParams.m_requireTones) {
    Release(OpalConnection::EndedByRemoteBusy);
    return false;
  }

  // Connect anyway
  InternalOnConnected();
  return true;
}


///////////////////////////////////////////////////////////////////////////////

OpalLineMediaStream::OpalLineMediaStream(OpalLineConnection & conn, 
                                  const OpalMediaFormat & mediaFormat,
                                                 unsigned sessionID,
                                                     PBoolean isSource,
                                               OpalLine & ln)
  : OpalMediaStream(conn, mediaFormat, sessionID, isSource)
  , m_line(ln)
  , m_notUsingRTP(!ln.GetDevice().UsesRTP())
  , m_useDeblocking(false)
  , m_missedCount(0)
  , m_lastFrameWasSignal(true)
  , m_directLineNumber(UINT_MAX)
{
  m_lastSID[0] = 2;
}

OpalLineMediaStream::~OpalLineMediaStream()
{
  Close();
}


PBoolean OpalLineMediaStream::Open()
{
  if (m_isOpen)
    return true;

  P_INSTRUMENTED_LOCK_READ_WRITE(return false);

  if (IsSource()) {
    if (!m_line.SetReadFormat(m_mediaFormat))
      return false;
  }
  else {
    if (!m_line.SetWriteFormat(m_mediaFormat))
      return false;
  }

  SetDataSize(GetDataSize(), GetDataSize()/2);
  PTRACE(3, "LineMedia\tStream opened for " << m_mediaFormat << ", using "
         << (m_notUsingRTP ? (m_useDeblocking ? "reblocked audio" : "audio frames") : "direct RTP"));

  return OpalMediaStream::Open();
}


void OpalLineMediaStream::InternalClose()
{
  if (m_directLineNumber != UINT_MAX)
    m_line.GetDevice().SetLineToLineDirect(m_line.GetLineNumber(), m_directLineNumber, false);
  else if (IsSource())
    m_line.StopReading();
  else
    m_line.StopWriting();
}


PBoolean OpalLineMediaStream::ReadPacket(RTP_DataFrame & packet)
{
  if (m_notUsingRTP)
    return OpalMediaStream::ReadPacket(packet);

  if (!packet.SetMinSize(RTP_DataFrame::MinHeaderSize+GetDataSize()))
    return false;

  PINDEX count = packet.GetSize();
  if (!m_line.ReadFrame(packet.GetPointer(), count))
    return false;

  packet.SetPayloadSize(count-packet.GetHeaderSize());
  return true;
}


PBoolean OpalLineMediaStream::WritePacket(RTP_DataFrame & packet)
{
  if (m_notUsingRTP)
    return OpalMediaStream::WritePacket(packet);

  PINDEX written = 0;
  return m_line.WriteFrame(packet.GetPointer(), packet.GetPacketSize(), written);
}


PBoolean OpalLineMediaStream::ReadData(BYTE * buffer, PINDEX size, PINDEX & length)
{
  PAssert(m_notUsingRTP, PLogicError);

  length = 0;

  if (IsSink()) {
    PTRACE(1, "LineMedia\tTried to read from sink media stream");
    return false;
  }

  if (m_useDeblocking) {
    m_line.SetReadFrameSize(size);
    if (m_line.ReadBlock(buffer, size)) {
      length = size;
      return true;
    }
  }
  else {
    length = size;
    if (m_line.ReadFrame(buffer, length)) {
      // In the case of G.723.1 remember the last SID frame we sent and send
      // it again if the hardware sends us a DTX frame.
      if (m_mediaFormat.GetPayloadType() == RTP_DataFrame::G7231) {
        switch (length) {
          case 1 : // DTX frame
            memcpy(buffer, m_lastSID, 4);
            length = 4;
            m_lastFrameWasSignal = false;
            break;
          case 4 :
            if ((*(BYTE *)buffer&3) == 2)
              memcpy(m_lastSID, buffer, 4);
            m_lastFrameWasSignal = false;
            break;
          default :
            m_lastFrameWasSignal = true;
        }
      }
      return true;
    }
  }

  PTRACE_IF(1, m_line.GetDevice().GetErrorNumber() != 0,
            "LineMedia\tDevice read frame error: " << m_line.GetDevice().GetErrorText());

  return false;
}


PBoolean OpalLineMediaStream::WriteData(const BYTE * buffer, PINDEX length, PINDEX & written)
{
  PAssert(m_notUsingRTP, PLogicError);

  written = 0;

  if (IsSource()) {
    PTRACE(1, "LineMedia\tTried to write to source media stream");
    return false;
  }

  // Check for writing silence
  PBYTEArray silenceBuffer;
  if (length != 0)
    m_missedCount = 0;
  else {
    switch (m_mediaFormat.GetPayloadType()) {
      case RTP_DataFrame::G7231 :
        if (m_missedCount++ < 4) {
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
        buffer = silenceBuffer.GetPointer(m_line.GetWriteFrameSize());
        length = silenceBuffer.GetSize();
        memset((void *)buffer, 0xff, length);
        break;

      case RTP_DataFrame::G729 :
        if (m_mediaFormat.GetName().Find('B') != P_MAX_INDEX) {
          static const BYTE g729_sid_frame[2] = { 1 };
          buffer = g729_sid_frame;
          length = 2;
          break;
        }
        // Else fall into default case

      default :
        buffer = silenceBuffer.GetPointer(m_line.GetWriteFrameSize()); // Fills with zeros
        length = silenceBuffer.GetSize();
        break;
    }
  }

  if (m_useDeblocking) {
    m_line.SetWriteFrameSize(length);
    if (m_line.WriteBlock(buffer, length)) {
      written = length;
      return true;
    }
  }
  else {
    if (m_line.WriteFrame(buffer, length, written))
      return true;
  }

  PTRACE_IF(1, m_line.GetDevice().GetErrorNumber() != 0,
            "LineMedia\tLID write frame error: " << m_line.GetDevice().GetErrorText());

  return false;
}


PBoolean OpalLineMediaStream::SetDataSize(PINDEX dataSize, PINDEX frameTime)
{
  if (m_notUsingRTP) {
    if (IsSource())
      m_useDeblocking = !m_line.SetReadFrameSize(dataSize) || m_line.GetReadFrameSize() != dataSize;
    else
      m_useDeblocking = !m_line.SetWriteFrameSize(dataSize) || m_line.GetWriteFrameSize() != dataSize;

    PTRACE(3, "LineMedia\tStream "
           << (IsSource() ? "read" : "write")
           << " frame size " << dataSize << " : rd="
           << m_line.GetReadFrameSize() << " wr="
           << m_line.GetWriteFrameSize() << ", "
           << (m_useDeblocking ? "needs" : "no") << " reblocking.");
  }

  // Yes we set BOTH to frameSize, and ignore the dataSize parameter
  return OpalMediaStream::SetDataSize(dataSize, frameTime);
}


PBoolean OpalLineMediaStream::IsSynchronous() const
{
  return true;
}


PBoolean OpalLineMediaStream::RequiresPatchThread(OpalMediaStream * stream) const
{
  OpalLineMediaStream * lineStream = dynamic_cast<OpalLineMediaStream *>(stream);
  if (lineStream != NULL && &m_line.GetDevice() == &lineStream->m_line.GetDevice()) {
    if (m_line.GetDevice().SetLineToLineDirect(m_line.GetLineNumber(), lineStream->m_line.GetLineNumber(), true)) {
      PTRACE(3, "LineMedia\tDirect line connection between "
             << m_line.GetLineNumber() << " and " << lineStream->m_line.GetLineNumber()
             << " on device " << m_line.GetDevice());
      const_cast<OpalLineMediaStream *>(this)->m_directLineNumber = lineStream->m_line.GetLineNumber();
      lineStream->m_directLineNumber = m_line.GetLineNumber();
      return false; // Do not start threads
    }
    PTRACE(2, "LineMedia\tCould not do direct line connection between "
           << m_line.GetLineNumber() << " and " << lineStream->m_line.GetLineNumber()
           << " on device " << m_line.GetDevice());
  }
  return OpalMediaStream::RequiresPatchThread(stream);
}


/////////////////////////////////////////////////////////////////////////////

OpalLineSilenceDetector::OpalLineSilenceDetector(OpalLine & theLine, const Params & theParam)
  : OpalSilenceDetector(theParam)
  , m_line(theLine)
{
}


unsigned OpalLineSilenceDetector::GetAverageSignalLevel(const BYTE *, PINDEX)
{
  return m_line.GetAverageSignalLevel(true);
}
#endif // OPAL_LID
