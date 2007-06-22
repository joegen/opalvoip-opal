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
 * Revision 1.2047  2007/06/22 02:09:54  rjongbloed
 * Removed asserts where is a valid, though unusual, case.
 *
 * Revision 2.45  2007/06/09 07:37:59  rjongbloed
 * Fixed some bugs in the LID code so USB handsets work correctly.
 *
 * Revision 2.44  2007/04/04 02:12:00  rjongbloed
 * Reviewed and adjusted PTRACE log levels
 *   Now follows 1=error,2=warn,3=info,4+=debug
 *
 * Revision 2.43  2007/04/03 07:40:24  rjongbloed
 * Fixed CreateCall usage so correct function (with userData) is called on
 *   incoming connections.
 *
 * Revision 2.42  2007/03/29 23:55:46  rjongbloed
 * Tidied some code when a new connection is created by an endpoint. Now
 *   if someone needs to derive a connection class they can create it without
 *   needing to remember to do any more than the new.
 *
 * Revision 2.41  2007/03/29 05:16:49  csoutheren
 * Pass OpalConnection to OpalMediaSream constructor
 * Add ID to OpalMediaStreams so that transcoders can match incoming and outgoing codecs
 *
 * Revision 2.40  2007/03/13 02:17:46  csoutheren
 * Remove warnings/errors when compiling with various turned off
 *
 * Revision 2.39  2007/03/13 00:33:10  csoutheren
 * Simple but messy changes to allow compile time removal of protocol
 * options such as H.450 and H.460
 * Fix MakeConnection overrides
 *
 * Revision 2.38  2007/03/01 05:51:04  rjongbloed
 * Fixed backward compatibility of OnIncomingConnection() virtual
 *   functions on various classes. If an old override returned FALSE
 *   then it will now abort the call as it used to.
 *
 * Revision 2.37  2007/01/24 04:00:57  csoutheren
 * Arrrghh. Changing OnIncomingConnection turned out to have a lot of side-effects
 * Added some pure viritual functions to prevent old code from breaking silently
 * New OpalEndpoint and OpalConnection descendants will need to re-implement
 * OnIncomingConnection. Sorry :)
 *
 * Revision 2.36  2006/12/18 03:18:42  csoutheren
 * Messy but simple fixes
 *   - Add access to SIP REGISTER timeout
 *   - Ensure OpalConnection options are correctly progagated
 *
 * Revision 2.35  2006/10/22 12:05:56  rjongbloed
 * Fixed correct usage of read/write buffer sizes in LID endpoints.
 *
 * Revision 2.34  2006/10/15 06:23:35  rjongbloed
 * Fixed the mechanism where both A-party and B-party are indicated by the application. This now works
 *   for LIDs as well as PC endpoint, wheich is the only one that was used before.
 *
 * Revision 2.33  2006/10/02 13:30:51  rjongbloed
 * Added LID plug ins
 *
 * Revision 2.32  2006/09/28 07:42:18  csoutheren
 * Merge of useful SRTP implementation
 *
 * Revision 2.31  2006/08/29 08:47:43  rjongbloed
 * Added functions to get average audio signal level from audio streams in
 *   suitable connection types.
 *
 * Revision 2.30  2006/08/17 23:09:04  rjongbloed
 * Added volume controls
 *
 * Revision 2.29  2006/07/27 22:34:21  rjongbloed
 * Fixed compiler warning
 *
 * Revision 2.28  2006/06/27 13:50:24  csoutheren
 * Patch 1375137 - Voicetronix patches and lid enhancements
 * Thanks to Frederich Heem
 *
 * Revision 2.27  2006/03/08 10:38:01  csoutheren
 * Applied patch #1441139 - virtualise LID functions and kill monitorlines correctly
 * Thanks to Martin Yarwood
 *
 * Revision 2.26  2006/01/14 10:43:06  dsandras
 * Applied patch from Brian Lu <Brian.Lu _AT_____ sun.com> to allow compilation
 * with OpenSolaris compiler. Many thanks !!!
 *
 * Revision 2.25  2004/10/06 13:03:42  rjongbloed
 * Added "configure" support for known LIDs
 * Changed LID GetName() function to be normalised against the GetAllNames()
 *   return values and fixed the pre-factory registration system.
 * Added a GetDescription() function to do what the previous GetName() did.
 *
 * Revision 2.24  2004/08/14 07:56:31  rjongbloed
 * Major revision to utilise the PSafeCollection classes for the connections and calls.
 *
 * Revision 2.23  2004/07/11 12:42:12  rjongbloed
 * Added function on endpoints to get the list of all media formats any
 *   connection the endpoint may create can support.
 *
 * Revision 2.22  2004/05/24 13:52:30  rjongbloed
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


BOOL OpalLIDEndPoint::MakeConnection(OpalCall & call,
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
  OpalLine * line = GetLine(lineName, TRUE);
  if (line == NULL){
    PTRACE(1,"LID EP\tMakeConnection cannot find the line \"" << lineName << '"');
    line = GetLine(defaultLine, TRUE);
  }  
  if (line == NULL){
    PTRACE(1,"LID EP\tMakeConnection cannot find the default line " << defaultLine);
    return FALSE;

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
  if (!device.IsOpen()){
    PTRACE(1, "LID EP\tAddLinesFromDevice device " << device.GetDeviceName() << "is not opened");
    return FALSE;
  }  

  BOOL atLeastOne = FALSE;

  linesMutex.Wait();

  
  PTRACE(3, "LID EP\tAddLinesFromDevice device " << device.GetDeviceName() << 
      " has " << device.GetLineCount() << " lines, CanTerminateCall = " << HasAttribute(CanTerminateCall));
  for (unsigned line = 0; line < device.GetLineCount(); line++) {
    PTRACE(3, "LID EP\tAddLinesFromDevice line  " << line << ", terminal = " << device.IsLineTerminal(line));
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
    if (lines[i].GetToken().Find(device.GetDeviceName()) == 0)
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
    if (devices[i].GetDeviceType() == deviceType && devices[i].GetDeviceName() == deviceName) {
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
BOOL OpalLIDEndPoint::OnSetUpConnection(OpalLineConnection & PTRACE_PARAM(connection))
{
  PTRACE(3, "LID EP\tOnSetUpConnection" << connection);
  return TRUE;
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
  requireTonesForDial = TRUE;
  wasOffHook = FALSE;
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

BOOL OpalLineConnection::OnSetUpConnection()
{
  PTRACE(3, "LID Con\tOnSetUpConnection");
  return endpoint.OnSetUpConnection(*this);
}

BOOL OpalLineConnection::SetAlerting(const PString & calleeName, BOOL)
{
  PTRACE(3, "LID Con\tSetAlerting " << *this);

  if (GetPhase() != SetUpPhase) 
    return FALSE;

  // switch phase 
  phase = AlertingPhase;
  alertingTime = PTime();

  line.SetCallerID(calleeName);
  return line.PlayTone(OpalLineInterfaceDevice::RingTone);
}


BOOL OpalLineConnection::SetConnected()
{
  PTRACE(3, "LID Con\tSetConnected " << *this);

  if (GetPhase() >= ConnectedPhase)
    return FALSE;

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
                                                        BOOL isSource)
{
  if (sessionID != OpalMediaFormat::DefaultAudioSessionID)
    return OpalConnection::CreateMediaStream(mediaFormat, sessionID, isSource);

  return new OpalLineMediaStream(*this, mediaFormat, sessionID, isSource, line);
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


BOOL OpalLineConnection::SetAudioVolume(BOOL source, unsigned percentage)
{
  OpalLineMediaStream * stream = dynamic_cast<OpalLineMediaStream *>(GetMediaStream(OpalMediaFormat::DefaultAudioSessionID, source));
  if (stream == NULL)
    return FALSE;

  OpalLine & line = stream->GetLine();
  return source ? line.SetRecordVolume(percentage) : line.SetPlayVolume(percentage);
}


unsigned OpalLineConnection::GetAudioSignalLevel(BOOL source)
{
  OpalLineMediaStream * stream = dynamic_cast<OpalLineMediaStream *>(GetMediaStream(OpalMediaFormat::DefaultAudioSessionID, source));
  if (stream == NULL)
    return UINT_MAX;

  OpalLine & line = stream->GetLine();
  return line.GetAverageSignalLevel(!source);
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

  if (!OnIncomingConnection(0, NULL)) {
    Release(EndedByCallerAbort);
    return;
  }

  PTRACE(3, "LID\tIncoming call routed for " << *this);
  if (!ownerCall.OnSetUp(*this))
    Release(EndedByNoAccept);
}

BOOL OpalLineConnection::SetUpConnection()
{
  PTRACE(3, "LID Con\tSetUpConnection call on " << *this << " to \"" << remotePartyNumber << '"');

  phase = SetUpPhase;
  originating = TRUE;

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
        return FALSE;

      case OpalLineInterfaceDevice::RingTone :
        PTRACE(3, "LID Con\tGot ringback on " << line);
        phase = AlertingPhase;
        OnAlerting();
        break;

      default :
        PTRACE(1, "LID Con\tError dialling " << remotePartyNumber << " on " << line);
        return FALSE;
    }
  }

  return TRUE;
}


///////////////////////////////////////////////////////////////////////////////

OpalLineMediaStream::OpalLineMediaStream(OpalLineConnection & conn, 
                                  const OpalMediaFormat & mediaFormat,
                                                 unsigned sessionID,
                                                     BOOL isSource,
                                               OpalLine & ln)
  : OpalMediaStream(conn, mediaFormat, sessionID, isSource),
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
    useDeblocking = !line.SetReadFrameSize(GetDataSize()) || line.GetReadFrameSize() != GetDataSize();
  }
  else {
    if (!line.SetWriteFormat(mediaFormat))
      return FALSE;
    useDeblocking = !line.SetWriteFrameSize(GetDataSize()) || line.GetWriteFrameSize() != GetDataSize();
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
    line.StopReading();
  else
    line.StopWriting();

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


BOOL OpalLineMediaStream::SetDataSize(PINDEX dataSize)
{
  if (IsSource())
    useDeblocking = !line.SetReadFrameSize(dataSize) || line.GetReadFrameSize() != dataSize;
  else
    useDeblocking = !line.SetWriteFrameSize(dataSize) || line.GetWriteFrameSize() != dataSize;
  return OpalMediaStream::SetDataSize(dataSize);
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
