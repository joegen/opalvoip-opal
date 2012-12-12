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
 * $Revision$
 * $Author$
 * $Date$
 */

#include "precompile.h"
#include "main.h"
#include "version.h"

#include <opal/transcoders.h>


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
         "W-wav:                 Audio WAV file to play.\n"
         "Y-yuv:                 Video YUV file to play.\n"
         "[Timing:]"
         "-tmaxest:              Maximum time to wait for \"Established\" [0]\n"
         "-tmincall:             Minimum call duration in seconds [10]\n"
         "-tmaxcall:             Maximum call duration in seconds [60]\n"
         "-tminwait:             Minimum interval between calls in seconds [10]\n"
         "-tmaxwait:             Maximum interval between calls in seconds [30]\n"
         "[Reporting:]"
         "q-quiet.               Do not display call progress output.\n"
         "c-cdr:                 Specify Call Detail Record file [none]\n"
         "I-in-dir:              Specify directory for incoming media files [disabled]\n"
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


bool MyManager::Initialise(PArgList & args, bool verbose, const PString &)
{
  args.Parse(GetArgumentSpec());

  if (args.HasOption('q'))
    cout.rdbuf(NULL);

  if (!OpalManagerConsole::Initialise(args, !args.HasOption('q'), "pc:"))
    return false;

  if (args.GetCount() == 0 && !args.HasOption('l')) {
    cerr << "\nNeed either --listen or a destination argument\n\n";
    Usage(cerr, args);
    return false;
  }

  OpalPCSSEndPoint * pcss = new MyPCSSEndPoint(*this);
  pcss->SetSoundChannelRecordDevice("Null Audio");
  pcss->SetSoundChannelPlayDevice("Null Audio");

  videoInputDevice.deviceName = "Fake/NTSCTest";
  videoOutputDevice.deviceName = "NULL";
  videoPreviewDevice.deviceName.MakeEmpty();  // Don't want any preview for video, there could be ... lots

  if (!args.HasOption('W'))
    cout << "Not using outgoing audio file." << endl;
  else {
    PString wavFile = args.GetOptionString('W');
    PINDEX last = wavFile.GetLength()-1;
    PWAVFile check;
    if (check.Open(wavFile[last] == '*' ? wavFile.Left(last) : wavFile, PFile::ReadOnly)) {
      cout << "Using outgoing audio file: " << wavFile << endl;
      pcss->SetSoundChannelRecordDevice(wavFile);
    }
    else {
      cout << "Outgoing audio file  \"" << wavFile << "\" does not exist!" << endl;
      PTRACE(1, "CallGen\tOutgoing audio file \"" << wavFile << "\" does not exist");
    }
  }

  if (!args.HasOption('Y'))
    cout << "Not using outgoing video file." << endl;
  else {
    PString yuvFile = args.GetOptionString('Y');
    PINDEX last = yuvFile.GetLength()-1;
    if (PFile::Exists(yuvFile[last] == '*' ? yuvFile.Left(last) : yuvFile)) {
      cout << "Using outgoing video file: " << yuvFile << endl;
      videoInputDevice.deviceName = yuvFile;
    }
    else {
      cout << "Outgoing video file  \"" << yuvFile << "\" does not exist!" << endl;
      PTRACE(1, "CallGen\tOutgoing video file \"" << yuvFile << "\" does not exist");
    }
  }

  {
    PString incomingMediaDirectory = args.GetOptionString('I');
    if (incomingMediaDirectory.IsEmpty())
      cout << "Not saving incoming media data." << endl;
    else if (PDirectory::Exists(incomingMediaDirectory) ||
             PDirectory::Create(incomingMediaDirectory)) {
      incomingMediaDirectory = PDirectory(incomingMediaDirectory);
      cout << "Using incoming media directory: " << incomingMediaDirectory << endl;
      pcss->SetSoundChannelPlayDevice(incomingMediaDirectory + "*.wav");
      videoOutputDevice.deviceName = incomingMediaDirectory + "*.yuv";
    }
    else {
      cout << "Could not create incoming media directory \"" << incomingMediaDirectory << "\"!" << endl;
      PTRACE(1, "CallGen\tCould not create incoming media directory \"" << incomingMediaDirectory << '"');
    }
  }

  unsigned simultaneous = args.GetOptionString('m').AsUnsigned();
  if (simultaneous == 0)
    simultaneous = args.HasOption('C') ? 1 : args.GetCount();

  if (args.HasOption('c')) {
    if (m_cdrFile.Open(args.GetOptionString('c'), PFile::WriteOnly, PFile::Create)) {
      m_cdrFile.SetPosition(0, PFile::End);
      PTRACE(1, "CallGen\tSetting CDR to \"" << m_cdrFile.GetFilePath() << '"');
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
  PTRACE(2, "CallGen\tStarted thread " << m_index);

  PRandom rand(PRandom::Number());

  PTimeInterval delay = RandomRange(rand, (m_index-1)*500, (m_index+1)*500);
  OUTPUT(m_index, PString::Empty(), "Initial delay of " << delay << " seconds");

  if (m_exit.Wait(delay)) {
    PTRACE(2, "CallGen\tAborted thread " << m_index);
  }
  else {
    // Loop "repeat" times for (repeat > 0), or loop forever for (repeat == 0)
    unsigned count = 1;
    do {
      PString destination = m_destinations[(m_index-1 + count-1)%m_destinations.GetSize()];

      // trigger a call
      PString token;
      PTRACE(1, "CallGen\tMaking call to " << destination);
      unsigned totalAttempts = ++m_params.m_callgen.m_totalCalls;
      if (!m_params.m_callgen.SetUpCall("pc:*", destination, token, this))
        OUTPUT(m_index, token, "Failed to start call to " << destination)
      else {
        bool stopping = false;

        delay = RandomRange(rand, m_params.m_tmin_call, m_params.m_tmax_call);
        PTRACE(1, "CallGen\tMax call time " << delay);

        START_OUTPUT(m_index, token) << "Making call " << count;
        if (m_params.m_repeat)
          cout << " of " << m_params.m_repeat;
        cout << " (total=" << totalAttempts<< ") for " << delay << " seconds to " << destination;
        END_OUTPUT();

        if (m_params.m_tmax_est > 0) {
          OUTPUT(m_index, token, "Waiting " << m_params.m_tmax_est << " seconds for establishment");
          PTRACE(1, "CallGen\tWaiting " << m_params.m_tmax_est << " seconds for establishment");

          PTimer timeout = m_params.m_tmax_est;
          while (!m_params.m_callgen.IsCallEstablished(token)) {
            stopping = m_exit.Wait(100);
            if (stopping || !timeout.IsRunning() || !m_params.m_callgen.HasCall(token)) {
              PTRACE(1, "CallGen\tTimeout/Stopped on establishment");
              delay = 0;
              break;
            }
          }
        }

        if (delay > 0) {
          // wait for a random time
          PTRACE(1, "CallGen\tWaiting for " << delay);
          stopping = m_exit.Wait(delay);
        }

        // end the call
        OUTPUT(m_index, token, "Clearing call");
        PTRACE(1, "CallGen\tClearing call");

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

      PTRACE(1, "CallGen\tDelaying for " << delay);
      // wait for a random time
    } while (!m_exit.Wait(delay));

    OUTPUT(m_index, PString::Empty(), "Completed call set.");
    PTRACE(2, "CallGen\tFinished thread " << m_index);
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


PBoolean MyPCSSEndPoint::OnShowIncoming(const OpalPCSSConnection & connection)
{
  AcceptIncomingConnection(connection.GetToken());
  return true;
}


PBoolean MyPCSSEndPoint::OnShowOutgoing(const OpalPCSSConnection &)
{
  return true;
}


// End of file ////////////////////////////////////////////////////////////////
