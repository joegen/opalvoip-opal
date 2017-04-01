/*
 * main.cxx
 *
 * OPAL call generator
 *
 * Copyright (c) 2007 Equivalence Pty. Ltd.
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
 * The Original Code is CallGen.
 *
 * Contributor(s): Equivalence Pty. Ltd.
 *
 */

#include "precompile.h"
#include "main.h"
#include "version.h"

#include <ptclib/pvidfile.h>
#include <opal/transcoders.h>


#define PTraceModule() "CallGen"


extern const char Manufacturer[] = "Equivalence";
extern const char Application[] = "CallGen";
typedef OpalConsoleProcess<MyManager, Manufacturer, Application> MyApp;
PCREATE_PROCESS(MyApp);

static PMutex g_coutMutex;

#define START_OUTPUT(index, token) \
  { \
    g_coutMutex.Wait(); \
    if (index > 0) std::cout << setw(3) << index << ": "; \
    std::cout << setw(10) << token.Left(10) << ": "

#define END_OUTPUT() \
    std::cout << std::endl; \
    g_coutMutex.Signal(); \
  }

#define OUTPUT(index, token, info) START_OUTPUT(index, token) << info; END_OUTPUT()



///////////////////////////////////////////////////////////////////////////////

MyManager::MyManager()
  : m_totalCalls(0)
  , m_totalEstablished(0)
{
}


PString MyManager::GetArgumentSpec() const
{
  PString spec = OpalManagerConsole::GetArgumentSpec();
  spec.Replace("r-register", "R-register");
  return "[Call Control:]"
         "l-listen.              Passive/listening mode.\n"
         "m-max:                 Maximum number of simultaneous calls\n"
         "r-repeat:              Repeat calls n times\n"
         "C-cycle.               Each simultaneous call cycles through destination list\r"
                                "otherwise it only calls the destination for its position\r"
                                "in the list.\n"
         "A-audio-file:          Audio file to play (.wav, .raw, .pcap).\n"
         "-audio-format:         If audio file .pcap, indicate media format\n"
#if OPAL_VIDEO
         "Y-video-file:          Video file to play (.yuv, .pcap).\n"
         "-video-format:         If audio file .pcap, indicate media format\n"
#endif
         "[Timing:]"
         "-tmaxest:              Maximum time to wait for \"Established\" [0]\n"
         "-tmincall:             Minimum call duration in seconds [10]\n"
         "-tmaxcall:             Maximum call duration in seconds [60]\n"
         "-tminwait:             Minimum interval between calls in seconds [10]\n"
         "-tmaxwait:             Maximum interval between calls in seconds [30]\n"
         "[Reporting:]"
         "q-quiet.               Do not display call progress output.\n"
         "c-cdr:                 Specify Call Detail Record file [none]\n"
         "I-in-dir:              Specify directory for incoming media (.pcap) files [disabled]\n"
       + spec;
}


void MyManager::Usage(ostream & strm, const PArgList & args)
{
  args.Usage(strm,
             "[options] --listen\n[options] destination [ destination ... ]") << "\n"
             "Notes:\n"
             "  If --tmaxest is set a non-zero value then --tmincall is the time to leave\n"
             "  the call running once established. If zero (the default) then --tmincall\n"
             "  is the length of the call from initiation. The call may or may not be\n"
             "  \"answered\" within that time.\n"
             "\n";
}


bool MyManager::Initialise(PArgList & args, bool, const PString &)
{
  args.Parse(GetArgumentSpec());

  if (args.GetCount() == 0 && !args.HasOption('l')) {
    cerr << "\nNeed either --listen or a destination argument\n\n";
    Usage(cerr, args);
    return false;
  }

  if (args.HasOption('q'))
    cout.rdbuf(NULL);

  if (!OpalManagerConsole::Initialise(args, !args.HasOption('q'), "local:"))
    return false;

  MyLocalEndPoint * localEP = new MyLocalEndPoint(*this);
  if (!localEP->Initialise(args))
    return false;

  unsigned simultaneous = args.GetOptionString('m').AsUnsigned();
  if (simultaneous == 0)
    simultaneous = args.HasOption('C') ? 1 : args.GetCount();

  if (args.HasOption('c')) {
    if (m_cdrFile.Open(args.GetOptionString('c'), PFile::WriteOnly, PFile::Create)) {
      m_cdrFile.SetPosition(0, PFile::End);
      PTRACE(1, "Setting CDR to \"" << m_cdrFile.GetFilePath() << '"');
      cout << "Sending Call Detail Records to \"" << m_cdrFile.GetFilePath() << '"' << endl;
    }
    else {
      cout << "Could not open \"" << m_cdrFile.GetFilePath() << "\"!" << endl;
    }
  }

  if (args.HasOption('l')) {
    cout << "Endpoint is listening for incoming calls, press ^C to exit.\n";
    return true;
  }

  CallParams params(*this);
  params.m_tmax_est .SetInterval(0, args.GetOptionString("tmaxest",  "0" ).AsUnsigned());
  params.m_tmin_call.SetInterval(0, args.GetOptionString("tmincall", "10").AsUnsigned());
  params.m_tmax_call.SetInterval(0, args.GetOptionString("tmaxcall", "60").AsUnsigned());
  params.m_tmin_wait.SetInterval(0, args.GetOptionString("tminwait", "10").AsUnsigned());
  params.m_tmax_wait.SetInterval(0, args.GetOptionString("tmaxwait", "30").AsUnsigned());

  if (params.m_tmin_call == 0 ||
      params.m_tmin_wait == 0 ||
      params.m_tmin_call > params.m_tmax_call ||
      params.m_tmin_wait > params.m_tmax_wait) {
    cerr << "Invalid times entered!\n";
    return false;
  }

  cout << "Maximum time between calls: " << params.m_tmin_wait << '-' << params.m_tmax_wait << "\n"
          "Maximum total call duration: " << params.m_tmin_call << '-' << params.m_tmax_call << "\n"
          "Maximum wait for establish: " << params.m_tmax_est << "\n"
          "Endpoint starting " << simultaneous << " simultaneous call";
  if (simultaneous > 1)
    cout << 's';
  cout << ' ';

  params.m_repeat = args.GetOptionString('r', "1").AsUnsigned();
  if (params.m_repeat != 0)
    cout << params.m_repeat;
  else
    cout << "infinite";
  cout << " time";
  if (params.m_repeat != 1)
    cout << 's';
  if (params.m_repeat != 0)
    cout << ", grand total of " << simultaneous*params.m_repeat << " calls";
  cout << ".\n\n"
          "Press ^C at any time to quit.\n\n"
        << endl;

  // create some threads to do calls, but start them randomly
  for (unsigned idx = 0; idx < simultaneous; idx++) {
    if (args.HasOption('C'))
      m_threadList.Append(new CallThread(idx+1, args.GetParameters(), params));
    else {
      PINDEX arg = idx%args.GetCount();
      m_threadList.Append(new CallThread(idx+1, args.GetParameters(arg, arg), params));
    }
  }

  return true;
}


void MyManager::Run()
{
  for (;;) {
    m_endRun.Wait();

    bool finished = true;
    for (PINDEX i = 0; i < m_threadList.GetSize(); i++) {
      if (m_threadList[i].m_running) {
        finished = false;
        break;
      }
    }

    if (finished) {
      cout << "\nAll call sets completed." << endl;
      break;
    }

    if (m_interrupted) {
      g_coutMutex.Wait();
      cout << "\nAborting all calls ..." << endl;
      g_coutMutex.Signal();

      // stop threads
      for (PINDEX i = 0; i < m_threadList.GetSize(); i++)
        m_threadList[i].Stop();

      // stop all calls
      ClearAllCalls();
    }
  }

  cout << "Total calls: " << m_totalCalls << " attempted, " << m_totalEstablished << " established.\n";
}


///////////////////////////////////////////////////////////////////////////////

CallThread::CallThread(unsigned index, const PStringArray & destinations, const CallParams & params)
  : PThread(1000, NoAutoDeleteThread, NormalPriority, psprintf("CallGen %u", index))
  , m_destinations(destinations)
  , m_index(index)
  , m_params(params)
  , m_running(true)
{
  Resume();
}


static unsigned RandomRange(PRandom & rand,
                            const PTimeInterval & tmin,
                            const PTimeInterval & tmax)
{
  unsigned umax = tmax.GetInterval();
  unsigned umin = tmin.GetInterval();
  return rand.Generate() % (umax - umin + 1) + umin;
}


void CallThread::Main()
{
  PTRACE(2, "Started thread " << m_index);

  PRandom rand(PRandom::Number());

  PTimeInterval delay = RandomRange(rand, (m_index-1)*500, (m_index+1)*500);
  OUTPUT(m_index, PString::Empty(), "Initial delay of " << delay << " seconds");

  if (m_exit.Wait(delay)) {
    PTRACE(2, "Aborted thread " << m_index);
  }
  else {
    // Loop "repeat" times for (repeat > 0), or loop forever for (repeat == 0)
    unsigned count = 1;
    do {
      PString destination = m_destinations[(m_index-1 + count-1)%m_destinations.GetSize()];

      // trigger a call
      PString token;
      PTRACE(1, "Making call to " << destination);
      unsigned totalAttempts = ++m_params.m_callgen.m_totalCalls;
      if (!m_params.m_callgen.SetUpCall("local:*", destination, token, this))
        OUTPUT(m_index, token, "Failed to start call to " << destination)
      else {
        bool stopping = false;

        delay = RandomRange(rand, m_params.m_tmin_call, m_params.m_tmax_call);
        PTRACE(1, "Max call time " << delay);

        START_OUTPUT(m_index, token) << "Making call " << count;
        if (m_params.m_repeat)
          cout << " of " << m_params.m_repeat;
        cout << " (total=" << totalAttempts<< ") for " << delay << " seconds to " << destination;
        END_OUTPUT();

        if (m_params.m_tmax_est > 0) {
          OUTPUT(m_index, token, "Waiting " << m_params.m_tmax_est << " seconds for establishment");
          PTRACE(1, "Waiting " << m_params.m_tmax_est << " seconds for establishment");

          PTimer timeout = m_params.m_tmax_est;
          while (!m_params.m_callgen.IsCallEstablished(token)) {
            stopping = m_exit.Wait(100);
            if (stopping || !timeout.IsRunning() || !m_params.m_callgen.HasCall(token)) {
              PTRACE(1, "Timeout/Stopped on establishment");
              delay = 0;
              break;
            }
          }
        }

        if (delay > 0) {
          // wait for a random time
          PTRACE(1, "Waiting for " << delay);
          stopping = m_exit.Wait(delay);
        }

        // end the call
        OUTPUT(m_index, token, "Clearing call");
        PTRACE(1, "Clearing call");

        m_params.m_callgen.ClearCallSynchronous(token);

        if (stopping)
          break;
      }

      count++;
      if (m_params.m_repeat > 0 && count > m_params.m_repeat)
        break;

      // wait for a random delay
      delay = RandomRange(rand, m_params.m_tmin_wait, m_params.m_tmax_wait);
      OUTPUT(m_index, PString::Empty(), "Delaying for " << delay << " seconds");

      PTRACE(1, "Delaying for " << delay);
      // wait for a random time
    } while (!m_exit.Wait(delay));

    OUTPUT(m_index, PString::Empty(), "Completed call set.");
    PTRACE(2, "Finished thread " << m_index);
  }

  m_running = false;
  m_params.m_callgen.EndRun();
}


void CallThread::Stop()
{
  if (!IsTerminated()) {
    OUTPUT(m_index, PString::Empty(), "Stopping.");
  }

  m_exit.Signal();
}


///////////////////////////////////////////////////////////////////////////////

MyManager::~MyManager()
{
  ShutDownEndpoints();
}


OpalCall * MyManager:: CreateCall(void * userData)
{
  return new MyCall(*this, (CallThread *)userData);
}


PBoolean MyManager::OnOpenMediaStream(OpalConnection & connection, OpalMediaStream & stream)
{
  dynamic_cast<MyCall &>(connection.GetCall()).OnOpenMediaStream(stream);
  return OpalManager::OnOpenMediaStream(connection, stream);
}


///////////////////////////////////////////////////////////////////////////////

MyCall::MyCall(MyManager & mgr, CallThread * caller)
  : OpalCall(mgr)
  , m_manager(mgr)
  , m_index(caller != NULL ? caller->m_index : 0)
  , m_openedTransmitMedia(0)
  , m_openedReceiveMedia(0)
  , m_receivedMedia(0)
{
}


void MyCall::OnNewConnection(OpalConnection & connection)
{
  OpalCall::OnNewConnection(connection);

  if (connection.IsNetworkConnection()) {
    m_callIdentifier = connection.GetIdentifier();
    if (IsNetworkOriginated()) {
      OUTPUT(m_index, GetToken(), "Started \"" << GetRemoteName() << "\""
                                           " " << GetRemoteParty() <<
                                    " active=" << m_manager.GetActiveCalls() <<
                                     " total=" << ++m_manager.m_totalCalls);
    }
  }
}


void MyCall::OnEstablishedCall()
{
  OUTPUT(m_index, GetToken(), "Established \"" << GetRemoteName() << "\""
                                           " " << GetRemoteParty() <<
                                    " active=" << m_manager.GetActiveCalls() <<
                                     " total=" << ++m_manager.m_totalEstablished);
  OpalCall::OnEstablishedCall();
}


void MyCall::OnCleared()
{
  OUTPUT(m_index, GetToken(), "Cleared \"" << GetRemoteName() << "\""
                                       " " << GetRemoteParty() <<
                                " reason=" << GetCallEndReason() <<
                                " active=" << (m_manager.GetActiveCalls()-1) <<
                                 " total=" << m_manager.m_totalEstablished);

  PTextFile & cdrFile = m_manager.m_cdrFile;

  if (cdrFile.IsOpen()) {
    static PMutex cdrMutex;
    cdrMutex.Wait();

    if (cdrFile.GetLength() == 0)
      cdrFile << "Call Start Time,"
                  "Total duration,"
                  "Establishment delay,"
                  "Media open transmit delay,"
                  "Media open received delay,"
                  "Media received delay,"
                  "Call End Reason,"
                  "Remote party,"
                  "Signalling gateway,"
                  "Media gateway,"
                  "Call Id,"
                  "Call Token\n";

    cdrFile << m_startTime.AsString("yyyy/M/d hh:mm:ss") << ','
            << setprecision(1) << (PTime() - m_startTime) << ',';

    if (m_establishedTime.IsValid())
      cdrFile << (m_establishedTime - m_startTime);
    cdrFile << ',';

    if (m_openedTransmitMedia.IsValid())
      cdrFile << (m_openedTransmitMedia - m_startTime);
    cdrFile << ',';

    if (m_openedReceiveMedia.IsValid())
      cdrFile << (m_openedReceiveMedia - m_startTime);
    cdrFile << ',';

    if (m_receivedMedia.IsValid())
      cdrFile << (m_receivedMedia - m_startTime);
    cdrFile << ',';

    cdrFile << GetCallEndReason() << ','
            << GetRemoteName() << ','
            << GetRemoteParty() << ','
            << m_mediaGateway << ','
            << m_callIdentifier << ','
            << GetToken()
            << endl;

    cdrMutex.Signal();
  }

  OpalCall::OnCleared();
}


void MyCall::OnOpenMediaStream(OpalMediaStream & stream)
{
  (stream.IsSink() ? m_openedTransmitMedia : m_openedReceiveMedia) = PTime();

  OUTPUT(m_index, GetToken(),
         "Opened " << (stream.IsSink() ? "transmitter" : "receiver")
                   << " for " << stream.GetMediaFormat());
}


MyLocalEndPoint::MyLocalEndPoint(OpalManager & mgr)
  : OpalLocalEndPoint(mgr)
  , m_incomingMediaDir(PString::Empty())
{
  SetDefaultAudioSynchronicity(e_SimulateSynchronous);
#if OPAL_VIDEO
  SetDefaultVideoSourceSynchronicity(e_SimulateSynchronous);
#endif
}


static bool SetMediaFormat(OpalMediaFormat & format, const PString & str)
{
  if (str.IsEmpty()) {
    cout << "Outgoing format must be set for PCAP file!" << endl;
    PTRACE(1, "Outgoing format must be set for PCAP file");
    return false;
  }

  format = str;

  if (format.IsEmpty()) {
    cout << "Outgoing format  \"" << str << "\" is unknown!" << endl;
    PTRACE(1, "Outgoing file \"" << str << "\" is unknown");
    return false;
  }

  return true;
}


bool MyLocalEndPoint::Initialise(PArgList & args)
{
#if P_WAVFILE
  if (!args.HasOption('A'))
    cout << "Not using outgoing audio file." << endl;
  else {
    m_audioFilePath = args.GetOptionString('A');
    std::auto_ptr<PFile> file(OpenAudioFile());
    if (file.get() == NULL) {
      cout << "Outgoing audio file  \"" << m_audioFilePath << "\" does not exist!" << endl;
      return false;
    }

    PWAVFile * wavFile = dynamic_cast<PWAVFile *>(file.get());
    if (wavFile != NULL) {
      if (!SetMediaFormat(m_audioFileFormat, wavFile->GetFormatString()))
        return false;
    }
    else if (dynamic_cast<OpalPCAPFile *>(file.get()) != NULL) {
      if (!SetMediaFormat(m_audioFileFormat, args.GetOptionString("audio-format")))
        return false;
    }
    else
      m_audioFileFormat = OpalPCM16;

    cout << "Using outgoing audio file \"" << m_audioFilePath << "\" as " << m_audioFileFormat << endl;
  }
#endif // P_WAVFILE

#if OPAL_VIDEO
  if (!args.HasOption('Y'))
    cout << "Not using outgoing video file." << endl;
  else {
    m_videoFilePath = args.GetOptionString('Y');
    std::auto_ptr<PFile> file(OpenVideoFile());
    if (file.get() == NULL) {
      cout << "Outgoing video file  \"" << m_audioFilePath << "\" does not exist!" << endl;
      return false;
    }

    if (dynamic_cast<OpalPCAPFile *>(file.get()) != NULL) {
      if (!SetMediaFormat(m_videoFileFormat, args.GetOptionString("video-format")))
        return false;
    }
    else
      m_audioFileFormat = OpalYUV420P;

    cout << "Using outgoing video file \"" << m_videoFilePath << "\" as " << m_videoFileFormat << endl;
  }
#endif // OPAL_VIDEO

  if (!args.HasOption('I')) 
    cout << "Not saving incoming media data." << endl;
  else {
    m_incomingMediaDir = args.GetOptionString('I');
    if (m_incomingMediaDir.Exists() || m_incomingMediaDir.Create(PFileInfo::DefaultDirPerms, true))
      cout << "Using incoming media directory \"" << m_incomingMediaDir << '"' << endl;
    else {
      cout << "Could not create incoming media directory \"" << m_incomingMediaDir << "\"!" << endl;
      PTRACE(1, "Could not create incoming media directory \"" << m_incomingMediaDir << '"');
      return false;
    }
  }

  return true;
}


PFile * MyLocalEndPoint::OpenAudioFile() const
{
  if (m_audioFilePath.IsEmpty())
    return NULL;

  PFile * file;
  if (m_audioFilePath.GetType() == ".raw")
    file = new PFile();
  else if (m_audioFilePath.GetType() == ".wav")
    file = new PWAVFile();
  else
    file = new OpalPCAPFile();

  if (file->Open(m_audioFilePath, PFile::ReadOnly))
    return file;

  PTRACE(2, "Could not open audio file \"" << m_audioFilePath << '"');
  delete file;
  return NULL;
}


#if OPAL_VIDEO
PFile * MyLocalEndPoint::OpenVideoFile() const
{
  if (m_videoFilePath.IsEmpty())
    return NULL;

  PFile * file = PVideoFileFactory::CreateInstance(m_videoFilePath.GetType());
  if (file == NULL)
    file = new OpalPCAPFile();

  if (file->Open(m_videoFilePath, PFile::ReadOnly))
    return file;

  PTRACE(2, "Could not open video file \"" << m_videoFilePath << '"');
  delete file;
  return NULL;
}
#endif // OPAL_VIDEO


OpalLocalConnection * MyLocalEndPoint::CreateConnection(OpalCall & call, void * userData, unsigned options, OpalConnection::StringOptions * stringOptions)
{
  return new MyLocalConnection(call, *this, userData, options, stringOptions);
}


OpalMediaFormatList MyLocalEndPoint::GetMediaFormats() const
{
  OpalMediaFormatList mediaFormats = OpalLocalEndPoint::GetMediaFormats();

  if (!m_audioFilePath.IsEmpty() && m_audioFileFormat.IsTransportable()) {
    mediaFormats.Remove("@audio");
    mediaFormats += m_audioFileFormat;
  }
#if OPAL_VIDEO
  if (!m_videoFilePath.IsEmpty() && m_videoFileFormat.IsTransportable()) {
    mediaFormats.Remove("@video");
    mediaFormats += m_videoFileFormat;
  }
#endif

  return mediaFormats;
}


MyLocalConnection::MyLocalConnection(OpalCall & call, MyLocalEndPoint & ep, void * userData, unsigned options, OpalConnection::StringOptions * stringOptions)
  : OpalLocalConnection(call, ep, userData, options, stringOptions)
  , m_endpoint(ep)
  , m_audioFile(ep.OpenAudioFile())
  , m_toneOffset(0)
#if OPAL_VIDEO
  , m_videoFile(ep.OpenVideoFile())
#endif
{
}


MyLocalConnection::~MyLocalConnection()
{
  delete m_audioFile;
#if OPAL_VIDEO
  delete m_videoFile;
#endif
}


void MyLocalConnection::AdjustMediaFormats(bool local, const OpalConnection * otherConnection, OpalMediaFormatList & mediaFormats) const
{
  if (local) {
    if (!m_endpoint.m_audioFilePath.IsEmpty() && m_endpoint.m_audioFileFormat.IsTransportable()) {
      mediaFormats.Remove("@audio");
      mediaFormats += m_endpoint.m_audioFileFormat;
    }
#if OPAL_VIDEO
    if (!m_endpoint.m_videoFilePath.IsEmpty() && m_endpoint.m_videoFileFormat.IsTransportable()) {
      mediaFormats.Remove("@video");
      mediaFormats += m_endpoint.m_videoFileFormat;
    }
#endif
  }

  OpalConnection::AdjustMediaFormats(local, otherConnection, mediaFormats);
}


bool MyLocalConnection::OnReadMediaData(const OpalMediaStream & mediaStream, void * data, PINDEX size, PINDEX & length)
{
  if (m_audioFile != NULL) {
    // WAV or RAW file
    if (!m_audioFile->Read(data, size))
      return false;
    length = m_audioFile->GetLastReadCount();
    return true;
  }

  if (m_tone.IsEmpty()) {
    OpalMediaFormat mediaFormat = mediaStream.GetMediaFormat();
    m_tone.SetSampleRate(mediaFormat.GetClockRate());
    m_tone.Generate('-', 440, 0, std::max(mediaFormat.GetFrameTime()/mediaFormat.GetTimeUnits(), 20U)); // at least 20ms of middle A
  }

  PINDEX bytesLeft = (m_tone.GetSize() - m_toneOffset) / 2;
  if (bytesLeft >= size)
    memcpy(data, &m_tone[m_toneOffset], size);
  else {
    memcpy(data, &m_tone[m_toneOffset], bytesLeft);
    memcpy((char *)data + bytesLeft, &m_tone[0], size - bytesLeft);
  }
  m_toneOffset = (m_toneOffset + size / 2) % m_tone.GetSize();
  length = size;
  return true;
}


bool MyLocalConnection::OnReadMediaFrame(const OpalMediaStream & mediaStream, RTP_DataFrame & frame)
{
  OpalPCAPFile * pcapFile;
  if (mediaStream.GetMediaFormat().GetMediaType() == OpalMediaType::Audio()) {
    pcapFile = dynamic_cast<OpalPCAPFile *>(m_audioFile);
    if (pcapFile == NULL)
      return OpalLocalConnection::OnReadMediaFrame(mediaStream, frame);
  }

#if OPAL_VIDEO
  else {
    PVideoFile * videoFile = dynamic_cast<PVideoFile *>(m_videoFile);
    if (videoFile != NULL) {
      frame.SetPayloadSize(videoFile->GetFrameBytes() + sizeof(OpalVideoTranscoder::FrameHeader));
      OpalVideoTranscoder::FrameHeader * hdr = (OpalVideoTranscoder::FrameHeader *)frame.GetPayloadPtr();
      hdr->x = hdr->y = 0;
      videoFile->GetFrameSize(hdr->width, hdr->height);
      videoFile->ReadFrame(OpalVideoFrameDataPtr(hdr));
      return true;
    }

    if ((pcapFile = dynamic_cast<OpalPCAPFile *>(m_videoFile)) == NULL)
      return true;
  }
#endif

  if (pcapFile->GetRTP(frame) < 0) {
    PTRACE(3, "Could not get RTP from PCAP file.");
  }
  return true;
}


bool MyLocalConnection::OnWriteMediaFrame(const OpalMediaStream & mediaStream, RTP_DataFrame & frame)
{
  if (!mediaStream.GetMediaFormat().IsTransportable())
    return true;

  if (!m_savedMedia.IsOpen()) {
    if (m_endpoint.m_incomingMediaDir.IsEmpty())
      return true;
    if (!m_savedMedia.Open(m_endpoint.m_incomingMediaDir + GetToken() + ".pcap", PFile::WriteOnly))
      return true;
  }

  if (m_savedMedia.IsOpen())
    m_savedMedia.WriteRTP(frame, mediaStream.GetMediaFormat().GetMediaType() == OpalMediaType::Audio() ? 5000 : 5002);
  return true;
}


// End of file ////////////////////////////////////////////////////////////////
