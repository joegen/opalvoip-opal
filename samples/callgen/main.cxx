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


PCREATE_PROCESS(CallGen);


///////////////////////////////////////////////////////////////////////////////

CallGen::CallGen()
  : PProcess("Equivalence", "CallGen", MAJOR_VERSION, MINOR_VERSION, BUILD_TYPE, BUILD_NUMBER)
  , m_interrupted(false)
  , m_totalCalls(0)
  , m_totalEstablished(0)
  , m_quietMode(false)
{
}


CallGen::~CallGen()
{
}


void CallGen::Main()
{
  PArgList & args = GetArguments();
  PArgList::ParseResult result = args.Parse("[Call Control:]"
             "l-listen.              Passive/listening mode.\n"
             "m-max:                 Maximum number of simultaneous calls\n"
             "r-repeat:              Repeat calls n times\n"
             "C-cycle.               Each simultaneous call cycles through destination list\n"
             "u-user:                Specify local username [login name]\n"
             "p-password:            Specify gatekeeper H.235 password [none]\n"

             "[Timing:]"
             "-tmaxest:              Maximum time to wait for \"Established\" [0]\n"
             "-tmincall:             Minimum call duration in seconds [10]\n"
             "-tmaxcall:             Maximum call duration in seconds [60]\n"
             "-tminwait:             Minimum interval between calls in seconds [10]\n"
             "-tmaxwait:             Maximum interval between calls in seconds [30]\n"

             "[SIP:]"
             "-no-sip.               Do not start SIP\n"
             "-sip-interface:        Specify IP address and port listen on for SIP [*:5060]\n"
             "R-registrar:           Specify address of record for registration\n"

             "[H.323:]"
             "-no-h323.              Do not start H.323\n"
             "-h323-interface:       Specify IP address and port listen on for H.323 [*:1720]\n"
             "f-fast-disable.        Disable fast start\n"
             "T-h245tunneldisable.   Disable H245 tunnelling.\n"
             "g-gatekeeper:          Specify gatekeeper host [no gatekeeper]\n"
             "G-discover-gatekeeper. Discover gatekeeper automatically [false]\n"
             "-require-gatekeeper.   Exit if gatekeeper discovery fails [false]\n"
             "a-access-token-oid:    Gatekeeper access token object identifier\n"

             "[Media:]"
             "O-out-msg:             Specify PCM16 WAV file for outgoing message [ogm.wav]\n"
             "P-prefer:              Set codec preference (use multiple times) [none]\n"
             "D-disable:             Disable codec (use multiple times) [none]\n"

             "[Network:]"
             "S-stun:                Specify Host/addres of STUN server\n"
             "-tcp-base:             Specific the base TCP port to use.\n"
             "-tcp-max:              Specific the maximum TCP port to use.\n"
             "-udp-base:             Specific the base UDP port to use.\n"
             "-udp-max:              Specific the maximum UDP port to use.\n"
             "-rtp-base:             Specific the base RTP/RTCP pair of UDP port to use.\n"
             "-rtp-max:              Specific the maximum RTP/RTCP pair of UDP port to use.\n"

             "[Reporting:]"
             "q-quiet.               Do not display call progress output.\n"
             "c-cdr:                 Specify Call Detail Record file [none]\n"
             "I-in-dir:              Specify directory for incoming WAV files [disabled]\n"
#if PTRACING
             "t-trace.               Trace enable (use multiple times for more detail)\n"
             "o-output:              Specify filename for trace output [stdout]\n"
#endif
             "h-help.                This help\n"
             , false);

  PAssert(result != PArgList::ParseInvalidOptions, PInvalidParameter);

  if (args.HasOption('h') || result < 0 || (result == PArgList::ParseNoArguments && !args.HasOption('l'))) {
    cout << "Usage:\n"
            "  " << GetFile().GetTitle() << " [options] --listen\n"
            "  " << GetFile().GetTitle() << " [options] destination [ destination ... ]\n";
    args.Usage(cout);
    cout << "\n"
            "Notes:\n"
            "  If --tmaxest is set a non-zero value then --tmincall is the time to leave\n"
            "  the call running once established. If zero (the default) then --tmincall\n"
            "  is the length of the call from initiation. The call may or may not be\n"
            "  \"answered\" within that time.\n"
            "\n";
    return;
  }
  
#if PTRACING
  PTrace::Initialise(args.GetOptionCount('t'), args.GetOptionString('o'),
		     PTrace::Blocks | PTrace::DateAndTime | PTrace::Thread | PTrace::FileAndLine);
#endif

#if OPAL_H323
  H323EndPoint * h323 = new H323EndPoint(m_manager);
#endif

  m_quietMode = args.HasOption('q');

  m_outgoingMessageFile.Parse(args.GetOptionString('O', "ogm.wav"), "file");
  if (m_outgoingMessageFile.IsEmpty())
    cout << "Not using outgoing message file." << endl;
  else if (PFile::Exists(m_outgoingMessageFile.AsFilePath()))
    cout << "Using outgoing message file: " << m_outgoingMessageFile << endl;
  else {
    cout << "Outgoing message file  \"" << m_outgoingMessageFile << "\" does not exist!" << endl;
    PTRACE(1, "CallGen\tOutgoing message file \"" << m_outgoingMessageFile << "\" does not exist");
    m_outgoingMessageFile = PString::Empty();
  }

  m_incomingAudioDirectory = args.GetOptionString('I');
  if (m_incomingAudioDirectory.IsEmpty())
    cout << "Not saving incoming audio data." << endl;
  else if (PDirectory::Exists(m_incomingAudioDirectory) ||
           PDirectory::Create(m_incomingAudioDirectory)) {
    m_incomingAudioDirectory = PDirectory(m_incomingAudioDirectory);
    cout << "Using incoming audio directory: " << m_incomingAudioDirectory << endl;
  }
  else {
    cout << "Could not create incoming audio directory \"" << m_incomingAudioDirectory << "\"!" << endl;
    PTRACE(1, "CallGen\tCould not create incoming audio directory \"" << m_incomingAudioDirectory << '"');
    m_incomingAudioDirectory = PString::Empty();
  }

  if (args.HasOption('S'))
    m_manager.SetSTUNServer(args.GetOptionString('S'));

#if OPAL_H323
  if (!args.HasOption("no-h323")) {
    PStringArray interfaces = args.GetOptionString("h323-interface").Lines();
    if (!h323->StartListeners(interfaces)) {
      cout << "Couldn't start any listeners on interfaces/ports:\n"
           << setfill('\n') << interfaces << setfill(' ') << endl;
      return;
    }
    cout << "H.323 listening on: " << setfill(',') << h323->GetListeners() << setfill(' ') << endl;
  }
#endif // OPAL_H323

#if OPAL_SIP
  if (!args.HasOption("no-sip")) {
    PStringArray interfaces = args.GetOptionString("sip-interface").Lines();
    SIPEndPoint * sip = new SIPEndPoint(m_manager);
    if (!sip->StartListeners(interfaces)) {
      cout << "Couldn't start any listeners on interfaces/ports:\n"
           << setfill('\n') << interfaces << setfill(' ') << endl;
      return;
    }
    cout << "SIP listening on: " << setfill(',') << sip->GetListeners() << setfill(' ') << endl;

    SIPRegister::Params reg;
    reg.m_addressOfRecord = args.GetOptionString('R');
    if (!reg.m_addressOfRecord.IsEmpty()) {
      PString aor;
      sip->Register(reg, aor);
    }
  }
#endif // OPAL_SIP

#if OPAL_IVR
  OpalIVREndPoint * ivr = new OpalIVREndPoint(m_manager);
  ivr->SetDefaultVXML("repeat=1000;"+m_outgoingMessageFile.AsString());
#endif // OPAL_IVR


  unsigned simultaneous = args.GetOptionString('m').AsUnsigned();
  if (simultaneous == 0)
    simultaneous = 1;

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

  if (args.HasOption("tcp-base"))
    m_manager.SetTCPPorts(args.GetOptionString("tcp-base").AsUnsigned(),
                        args.GetOptionString("tcp-max").AsUnsigned());
  if (args.HasOption("udp-base"))
    m_manager.SetUDPPorts(args.GetOptionString("udp-base").AsUnsigned(),
                        args.GetOptionString("udp-max").AsUnsigned());
  if (args.HasOption("rtp-base"))
    m_manager.SetRtpIpPorts(args.GetOptionString("rtp-base").AsUnsigned(),
                          args.GetOptionString("rtp-max").AsUnsigned());
  else {
    // Make sure that there are enough RTP ports for the simultaneous calls
    unsigned availablePorts = m_manager.GetRtpIpPortMax() - m_manager.GetRtpIpPortBase();
    if (availablePorts < simultaneous*4) {
      m_manager.SetRtpIpPorts(m_manager.GetRtpIpPortBase(), m_manager.GetRtpIpPortBase()+simultaneous*5);
      cout << "Increasing RTP ports available from " << availablePorts << " to " << simultaneous*5 << endl;
    }
  }

  if (args.HasOption('D'))
    m_manager.SetMediaFormatMask(args.GetOptionString('D').Lines());
  if (args.HasOption('P'))
    m_manager.SetMediaFormatOrder(args.GetOptionString('P').Lines());

  OpalMediaFormatList allMediaFormats;
#if OPAL_IVR
  allMediaFormats = OpalTranscoder::GetPossibleFormats(ivr->GetMediaFormats()); // Get transcoders
#endif

#if PTRACING
  ostream & traceStream = PTrace::Begin(3, __FILE__, __LINE__);
  traceStream << "Simple\tAvailable media formats:\n";
  for (PINDEX i = 0; i < allMediaFormats.GetSize(); i++)
    allMediaFormats[i].PrintOptions(traceStream);
  traceStream << PTrace::End;
#endif

  allMediaFormats.RemoveNonTransportable();
  allMediaFormats.Remove(m_manager.GetMediaFormatMask());
  allMediaFormats.Reorder(m_manager.GetMediaFormatOrder());

  cout << "Codecs removed  : " << setfill(',') << m_manager.GetMediaFormatMask() << "\n"
          "Codec order     : " << setfill(',') << m_manager.GetMediaFormatOrder() << "\n"
          "Available codecs: " << allMediaFormats << setfill(' ') << endl;

  // set local username, is necessary
  if (args.HasOption('u')) {
    PStringArray aliases = args.GetOptionString('u').Lines();
    m_manager.SetDefaultUserName(aliases[0]);
#if OPAL_H323
    for (PINDEX i = 1; i < aliases.GetSize(); ++i)
      h323->AddAliasName(aliases[i]);
#endif // OPAL_H323
  }
  cout << "Local username: \"" << m_manager.GetDefaultUserName() << '"' << endl;
  
#if OPAL_H323
  if (args.HasOption('p')) {
    h323->SetGatekeeperPassword(args.GetOptionString('p'));
    cout << "Using H.235 security." << endl;
  }

  if (args.HasOption('a')) {
    h323->SetGkAccessTokenOID(args.GetOptionString('a'));
    cout << "Set Access Token OID to \"" << h323->GetGkAccessTokenOID() << '"' << endl;
  }

  // process gatekeeper registration options
  if (args.HasOption('g')) {
    PString gkAddr = args.GetOptionString('g');
    cout << "Registering with gatekeeper \"" << gkAddr << "\" ..." << flush;
    if (!h323->UseGatekeeper(gkAddr)) {
      cout << "\nError registering with gatekeeper at \"" << gkAddr << '"' << endl;
      return;
    }
  }
  else if (args.HasOption('G')) {
    cout << "Searching for gatekeeper ..." << flush;
    if (!h323->UseGatekeeper()) {
      cout << "\nNo gatekeeper found." << endl;
      if (args.HasOption("require-gatekeeper")) 
        return;
    }
  }

  H323Gatekeeper * gk = h323->GetGatekeeper();
  if (gk != NULL) {
    H323Gatekeeper::RegistrationFailReasons reason;
    while ((reason = gk->GetRegistrationFailReason()) == H323Gatekeeper::UnregisteredLocally)
      PThread::Sleep(500);
    switch (reason) {
      case H323Gatekeeper::RegistrationSuccessful :
        cout << "\nGatekeeper set to \"" << *gk << '"' << endl;
        break;
      case H323Gatekeeper::DuplicateAlias :
        cout << "\nGatekeeper registration failed: duplicate alias" << endl;
        break;
      case H323Gatekeeper::SecurityDenied :
        cout << "\nGatekeeper registration failed: security denied" << endl;
        break;
      case H323Gatekeeper::TransportError :
        cout << "\nGatekeeper registration failed: transport error" << endl;
        break;
      default :
        cout << "\nGatekeeper registration failed: code=" << reason << endl;
    }
  }

  if (args.HasOption('f'))
    h323->DisableFastStart(true);
  if (args.HasOption('T'))
    h323->DisableH245Tunneling(true);
#endif // OPAL_H323


  if (args.HasOption('l')) {
    m_manager.AddRouteEntry(".*\t.* = ivr:"); // Everything goes to IVR
    cout << "Endpoint is listening for incoming calls, press ^C to exit.\n";
    m_signalMain.Wait();
  }
  else {
    CallParams params(*this);
    params.tmax_est .SetInterval(0, args.GetOptionString("tmaxest",  "0" ).AsUnsigned());
    params.tmin_call.SetInterval(0, args.GetOptionString("tmincall", "10").AsUnsigned());
    params.tmax_call.SetInterval(0, args.GetOptionString("tmaxcall", "60").AsUnsigned());
    params.tmin_wait.SetInterval(0, args.GetOptionString("tminwait", "10").AsUnsigned());
    params.tmax_wait.SetInterval(0, args.GetOptionString("tmaxwait", "30").AsUnsigned());

    if (params.tmin_call == 0 ||
        params.tmin_wait == 0 ||
        params.tmin_call > params.tmax_call ||
        params.tmin_wait > params.tmax_wait) {
      cerr << "Invalid times entered!\n";
      return;
    }

    cout << "Maximum time between calls: " << params.tmin_wait << '-' << params.tmax_wait << "\n"
            "Maximum total call duration: " << params.tmin_call << '-' << params.tmax_call << "\n"
            "Maximum wait for establish: " << params.tmax_est << "\n"
            "Endpoint starting " << simultaneous << " simultaneous call";
    if (simultaneous > 1)
      cout << 's';
    cout << ' ';

    params.repeat = args.GetOptionString('r', "1").AsUnsigned();
    if (params.repeat != 0)
      cout << params.repeat;
    else
      cout << "infinite";
    cout << " time";
    if (params.repeat != 1)
      cout << 's';
    if (params.repeat != 0)
      cout << ", grand total of " << simultaneous*params.repeat << " calls";
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

   for (;;) {
      m_signalMain.Wait();

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
        m_coutMutex.Wait();
        cout << "\nAborting all calls ..." << endl;
        m_coutMutex.Signal();

        // stop threads
        for (PINDEX i = 0; i < m_threadList.GetSize(); i++)
          m_threadList[i].Stop();

        // stop all calls
        m_manager.ClearAllCalls();
      }
    }
  }

  cout << "Total calls: " << m_totalCalls << " attempted, " << m_totalEstablished << " established.\n";
}


bool CallGen::OnInterrupt(bool)
{
  m_interrupted = true;
  m_signalMain.Signal();
  return true;
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


#define START_OUTPUT(index, token) \
  if (CallGen::Current().m_quietMode) ; else { \
    CallGen::Current().m_coutMutex.Wait(); \
    if (index > 0) cout << setw(3) << index << ": "; \
    cout << setw(10) << token.Left(10) << ": "

#define END_OUTPUT() \
    cout << endl; \
    CallGen::Current().m_coutMutex.Signal(); \
  }

static bool generateOutput = false;

#define OUTPUT(index, token, info) START_OUTPUT(index, token) << info; END_OUTPUT()


void CallThread::Main()
{
  PTRACE(2, "CallGen\tStarted thread " << m_index);

  CallGen & callgen = CallGen::Current();
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
      unsigned totalAttempts = ++callgen.m_totalCalls;
      if (!callgen.m_manager.SetUpCall("ivr:*", destination, token, this))
        OUTPUT(m_index, token, "Failed to start call to " << destination)
      else {
        bool stopping = false;

        delay = RandomRange(rand, m_params.tmin_call, m_params.tmax_call);
        PTRACE(1, "CallGen\tMax call time " << delay);

        START_OUTPUT(m_index, token) << "Making call " << count;
        if (m_params.repeat)
          cout << " of " << m_params.repeat;
        cout << " (total=" << totalAttempts<< ") for " << delay << " seconds to " << destination;
        END_OUTPUT();

        if (m_params.tmax_est > 0) {
          OUTPUT(m_index, token, "Waiting " << m_params.tmax_est << " seconds for establishment");
          PTRACE(1, "CallGen\tWaiting " << m_params.tmax_est << " seconds for establishment");

          PTimer timeout = m_params.tmax_est;
          while (!callgen.m_manager.IsCallEstablished(token)) {
            stopping = m_exit.Wait(100);
            if (stopping || !timeout.IsRunning() || !callgen.m_manager.HasCall(token)) {
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

        callgen.m_manager.ClearCallSynchronous(token);

        if (stopping)
          break;
      }

      count++;
      if (m_params.repeat > 0 && count > m_params.repeat)
        break;

      // wait for a random delay
      delay = RandomRange(rand, m_params.tmin_wait, m_params.tmax_wait);
      OUTPUT(m_index, PString::Empty(), "Delaying for " << delay << " seconds");

      PTRACE(1, "CallGen\tDelaying for " << delay);
      // wait for a random time
    } while (!m_exit.Wait(delay));

    OUTPUT(m_index, PString::Empty(), "Completed call set.");
    PTRACE(2, "CallGen\tFinished thread " << m_index);
  }

  m_running = false;
  callgen.m_signalMain.Signal();
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
                                     " total=" << ++CallGen::Current().m_totalCalls);
    }
  }
}


void MyCall::OnEstablishedCall()
{
  OUTPUT(m_index, GetToken(), "Established \"" << GetRemoteName() << "\""
                                           " " << GetRemoteParty() <<
                                    " active=" << m_manager.GetActiveCalls() <<
                                     " total=" << ++CallGen::Current().m_totalEstablished);
  OpalCall::OnEstablishedCall();
}


void MyCall::OnCleared()
{
  OUTPUT(m_index, GetToken(), "Cleared \"" << GetRemoteName() << "\""
                                       " " << GetRemoteParty() <<
                                " reason=" << GetCallEndReason() <<
                                " active=" << (m_manager.GetActiveCalls()-1) <<
                                 " total=" << CallGen::Current().m_totalEstablished);

  PTextFile & cdrFile = CallGen::Current().m_cdrFile;

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


// End of file ////////////////////////////////////////////////////////////////
