/*
* main.cxx
*
* Jester - a tester of the Opal jitter buffer
*
* Copyright (c) 2006 Derek J Smithies
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
* The Original Code is Jester
*
* The Initial Developer of the Original Code is Derek J Smithies
*
* Contributor(s): Robert Jongbloed.
*
* $Revision$
* $Author$
* $Date$
*/

#ifdef P_USE_PRAGMA
#pragma implementation "main.h"
#endif


#include <ptlib.h>

#include <opal/buildopts.h>
#include <rtp/rtp.h>

#include "main.h"
#include "../../version.h"


#define new PNEW


PCREATE_PROCESS(JesterProcess);


#define SAMPLES_PER_SECOND 8000
#define SAMPLES_PER_MILLISECOND (SAMPLES_PER_SECOND/1000)


ostream & operator<<(ostream & strm, const JitterProfileMap & profile)
{
  for (JitterProfileMap::const_iterator it = profile.begin(); it != profile.end(); ++it) {
    if (it != profile.begin())
      strm << ',';
    strm << it->first << '=' << it->second;
  }
  return strm;
}


///////////////////////////////////////////////////////////////
JesterJitterBuffer::JesterJitterBuffer()
: OpalJitterBuffer(400, 2000)
{

}


/////////////////////////////////////////////////////////////////////////////

JesterProcess::JesterProcess()
  : PProcess("Derek Smithies Code Factory", "Jester", 1, 1, ReleaseCode, 0)
  , m_generateTimestamp(0)
  , m_initialTimestamp(0)
  , m_generateSequenceNumber(0)
  , m_maxSequenceNumber(4000)
  , m_playbackTimestamp(0)
  , m_keepRunning(true)
  , m_lastFrameWasSilence(true)
  , m_talkBurstTimestamp(0)
  , m_lastSilentTimestamp(0)
  , m_lastGeneratedJitter(0)
  , m_lastFrameTime(0)
{
}


void JesterProcess::Main()
{
  // Get and parse all of the command line arguments.
  PArgList & args = GetArguments();
  if (args.Parse("a-audiodevice: audio device to play the output on\n"
                 "D-start-delta: Start delta time between generator and playback (ms)\n"
                 "d-drop. simulate dropped packets.\n"
                 "j-jitter: size of the jitter buffer in ms (100-1000)\n"
                 "g-generate: amount of jitter to simulate (see below)\n"
                 "I-init-play-ts: Initial timestamp value for generator\n"
                 "i-init-gen-ts: Initial timestamp value for playback\n"
                 "s-silence. simulate silence suppression. - so audio is sent in bursts.\n"
                 "S-size: size of each RTP packet in ms\n"
                 "m-marker. turn some of the marker bits off, that indicate speech bursts\n"
                 "P-pcap: Read RTP data from PCAP file\n"
                 "R. Non real time test\n"
                 "v-version. report version and program info.\n"
                 "w-wavfile:audio file from which the source data is read from\n"
                 PTRACE_ARGLIST
                 "h-help. This help message.\n"
                 , false) < PArgList::ParseNoArguments || args.HasOption('h') ) {
    args.Usage(cerr, "[options]") << "\n"
            "The jitter generate profile is a comma separated list of timestamps and\n"
            "jitter levels. e.g. \"0=30,16000=60,48000=120,64000=30\" would start at\n"
            "30ms, thena 2 seconds in generate 60ms of jitter, at 6 seconds 120ms,\n"
            "finally at 8 seconds back to 30ms for the remainder of the test.\n"
            "\n";
    return;
  }

  if (args.HasOption('v')) {
    cout << GetName()  << endl
         << " Version " << GetVersion(true) << endl
         << " by " << GetManufacturer() << endl
         << " on " << GetOSClass() << ' ' << GetOSName() << endl
         << " (" << GetOSVersion() << '-' << GetOSHardware() << ")\n\n";
    return;
  }

  PTRACE_INITIALISE(args);

  unsigned minJitterSize = 50;
  unsigned maxJitterSize = 250;

  if (args.HasOption('j')) {
    unsigned minJitterNew;
    unsigned maxJitterNew;
    PStringArray delays = args.GetOptionString('j').Tokenise(",-");

    if (delays.GetSize() > 1) {
      minJitterNew = delays[0].AsUnsigned();
      maxJitterNew = delays[1].AsUnsigned();
    } else {
      maxJitterNew = delays[0].AsUnsigned();
      minJitterNew = maxJitterNew;
    }

    if (minJitterNew >= 20 && minJitterNew <= maxJitterNew && maxJitterNew <= 1000) {
      minJitterSize = minJitterNew;
      maxJitterSize = maxJitterNew;
    } else {
      cerr << "Jitter should be between 20 milliseconds and 1 seconds, is "
        << 20 << '-' << 1000 << endl;
    }
  } 
  m_jitterBuffer.SetDelay(SAMPLES_PER_MILLISECOND * minJitterSize, SAMPLES_PER_MILLISECOND * maxJitterSize);

  m_silenceSuppression = args.HasOption('s');
  m_dropPackets = args.HasOption('d');
  m_markerSuppression = args.HasOption('m');
  m_bytesPerBlock = args.GetOptionString('S', "20").AsUnsigned()*SAMPLES_PER_MILLISECOND*2;
  m_startTimeDelta = args.GetOptionString('D', "0").AsInteger();
  m_generateTimestamp = args.GetOptionString('i', "0").AsUnsigned();
  m_playbackTimestamp = args.GetOptionString('I', "0").AsUnsigned();

  m_generateJitter[0] = 20;
  PStringArray profile = args.GetOptionString('g', "0=100").Tokenise(',',false);
  for (PINDEX i = 0; i < profile.GetSize(); ++i) {
    PStringArray change = profile[i].Tokenise('=');
    if (change.GetSize() != 2) {
      cerr << "Illegal format for jitter generation profile" << endl;
      return;
    }
    m_generateJitter[change[0].AsUnsigned()] = change[1].AsUnsigned();
  }

  if (!m_wavFile.Open(args.GetOptionString('w', "../callgen/ogm.wav"), PFile::ReadOnly)) {
    cerr << "the audio file " << m_wavFile.GetName() << " does not exist." << endl;
    return;
  }

  PString audioDevice = args.GetOptionString('a', PSoundChannel::GetDefaultDevice(PSoundChannel::Player));
  if (!m_player.Open(audioDevice, PSoundChannel::Player, 1, SAMPLES_PER_SECOND, 16)) {
    cerr << "Failed to open the sound device \"" << audioDevice 
         << "\", available devices:\n";
    PStringList namesPlay = PSoundChannel::GetDeviceNames(PSoundChannel::Player);
    for (PINDEX i = 0; i < namesPlay.GetSize(); i++)
      cerr << i << "  " << namesPlay[i] << endl;
    cerr << endl;
    return;
  }
  m_player.SetBuffers(m_bytesPerBlock, 160/SAMPLES_PER_MILLISECOND/2);

  if (args.HasOption('P')) {
    if (!m_pcap.Open(args.GetOptionString('P'))) {
      cerr << "Could not open PCAP file \"" << args.GetOptionString('P') << '"' << endl;
      return;
    }

    cout << "Analysing PCAP file ... " << flush;
    OpalPCAPFile::DiscoveredRTPMap discoveredRTPMap;
    if (!m_pcap.DiscoverRTP(discoveredRTPMap)) {
      cerr << "error: no RTP sessions found" << endl;
      return;
    }

    cout << "\nSelect one of the following sessions:\n" << discoveredRTPMap << endl;
    for (;;) {
      cout << "Session? " << flush;
      size_t session;
      cin >> session;
      if (m_pcap.SetFilters(discoveredRTPMap, session))
        break;
      cout << "Session " << session << " is not valid" << endl;
    }
    cout << "\nPCAP file         : " << m_pcap.GetFilePath() << endl;
  }
  else {
    cout << "Packet size       : " << m_bytesPerBlock << " bytes (" << m_bytesPerBlock/SAMPLES_PER_MILLISECOND/2 << "ms)\n"
            "Generated jitter  : " << m_generateJitter << "\n"
            "Silence periods   : " << (m_silenceSuppression ? "yes" : "no") << "\n"
            "Suppress markers  : " << (m_markerSuppression ? "yes" : "no") << "\n"
            "WAV file          : " << m_wavFile.GetName() << '\n'
         << endl;
  }

  cout << "Jitter buffer size: " << minJitterSize << ".." << maxJitterSize << " ms" << endl;

  if (args.HasOption('R')) {
    PTimeInterval genTick = m_startTimeDelta;
    PTimeInterval outTick;
    PINDEX lastReadTimeDelta = 80;
    while (m_pcap.IsOpen() ? !m_pcap.IsEndOfFile() : (m_generateSequenceNumber < m_maxSequenceNumber)) {
      RTP_DataFrame writeFrame;
      PTimeInterval delay;
      bool gotFrame = GenerateFrame(writeFrame, delay);
      genTick += delay;

      while (outTick <= genTick) {
        RTP_DataFrame readFrame(m_bytesPerBlock);
        readFrame.SetTimestamp(m_playbackTimestamp);
        if (!m_jitterBuffer.ReadData(readFrame, outTick))
          break;

        DWORD ts = readFrame.GetTimestamp();
        PINDEX sz = readFrame.GetPayloadSize();
        PTRACE(5, "Jester\tExtracted frame : "
                  "ts=" << setw(8) << ts << " "
                  "tick=" << setw(7) << outTick << " "
                  "sz=" << sz);
        if (sz == 0)
          m_playbackTimestamp += lastReadTimeDelta;
        else {
          switch (readFrame.GetPayloadType()) {
            case RTP_DataFrame::PCMA :
            case RTP_DataFrame::PCMU :
              lastReadTimeDelta = sz;
              break;
            case RTP_DataFrame::G729 :
              lastReadTimeDelta = sz/10*80;
              break;
            case RTP_DataFrame::G723 :
              lastReadTimeDelta = sz/24*240;
              break;
            default :
              lastReadTimeDelta = 160;
          }
          m_playbackTimestamp = ts + lastReadTimeDelta;
        }
        outTick += lastReadTimeDelta/SAMPLES_PER_MILLISECOND;
#if 0
        static unsigned adjust = 0;
        outTick -= ++adjust % 3 > 0;
#endif
      }

      if (gotFrame) {
        DWORD ts = writeFrame.GetTimestamp();
        PTRACE(5, "Jester\tInserting frame : "
                  "ts=" << setw(8) << ts << " "
                  "tick=" << setw(7) << genTick << " "
                  "(" << PTimeInterval(ts/SAMPLES_PER_MILLISECOND) << ')');
        if (!m_jitterBuffer.WriteData(writeFrame, genTick))
          break;
      }
    }
  }
  else {
    cout << "Audio device      : " << m_player.GetName() << endl;

    PThread * writer = PThread::Create(PCREATE_NOTIFIER(GeneratePackets), 0,
                                       PThread::NoAutoDeleteThread,
                                       PThread::NormalPriority,
                                       "generate");

    PThread * reader = PThread::Create(PCREATE_NOTIFIER(ConsumePackets), 0,
                                       PThread::NoAutoDeleteThread,
                                       PThread::NormalPriority,
                                       "consume");


    ManageUserInput();

    writer->WaitForTermination();
    reader->WaitForTermination();

    delete writer;
    delete reader;
  }

  Report();
}


void JesterProcess::GeneratePackets(PThread &, INT)
{
  if (m_startTimeDelta > 0)
    PThread::Sleep(m_startTimeDelta);

  m_initialTick = PTimer::Tick();

  while (m_keepRunning) {
    RTP_DataFrame frame;
    PTimeInterval delay;
    bool gotFrame = GenerateFrame(frame, delay);

    if (delay > 0)
      PThread::Sleep(delay);

    if (gotFrame) {
      PTRACE(5, "Jester\tInserting frame : ts=" << frame.GetTimestamp() << ", delay=" << delay);
      m_jitterBuffer.WriteData(frame);
    }
  }

  PTRACE(3, "Jester\tEnd of generate packets ");
}


bool JesterProcess::GenerateFrame(RTP_DataFrame & frame, PTimeInterval & delay)
{
  if (m_pcap.IsOpen()) {
    RTP_DataFrame rtp;
    while (m_pcap.GetRTP(rtp) < 0) {
      if (m_pcap.IsEndOfFile())
        return false;
    }
    frame = rtp;
    frame.MakeUnique();

    if (m_lastFrameTime.IsValid())
      delay = m_pcap.GetPacketTime() - m_lastFrameTime;
    m_lastFrameTime = m_pcap.GetPacketTime();
    ++m_generateSequenceNumber;
    return true;
  }

  if (m_silenceSuppression && !m_lastFrameWasSilence &&
          (m_generateTimestamp - m_talkBurstTimestamp) > (10000*SAMPLES_PER_MILLISECOND)) {
    PTRACE(3, "Jester\tStarted silence : ts=" << m_generateTimestamp);
    m_lastFrameWasSilence = true;
    m_lastSilentTimestamp = m_generateTimestamp;
    delay = 10;
    return false;
  }

  if (m_lastSilentTimestamp != 0 && (m_generateTimestamp - m_lastSilentTimestamp) < (2000*SAMPLES_PER_MILLISECOND)) {
    PTRACE(5, "Jester\tSilence active  : ts=" << setw(8) << m_generateTimestamp);
    delay = 10;
    return false;
  }

  if (m_dropPackets && m_generateSequenceNumber%100 == 0) {
    PTRACE(4, "Jester\tDropped packet  : ts=" << setw(8) << m_generateTimestamp);
    delay = 10;
    return false;
  }

  frame.SetPayloadSize(m_bytesPerBlock);
  frame.SetPayloadType(RTP_DataFrame::L16_Mono);
  frame.SetSequenceNumber(++m_generateSequenceNumber);
  frame.SetTimestamp(m_generateTimestamp);
  frame.SetMarker(m_lastFrameWasSilence && (!m_markerSuppression || (m_generateSequenceNumber&1) == 0));

  if (m_lastFrameWasSilence) {
    PTRACE(3, "Jester\tBegin talk burst: ts=" << setw(8) << m_generateTimestamp);
    m_talkBurstTimestamp = m_generateTimestamp;
    m_lastFrameWasSilence = false;
  }

  if (!m_wavFile.Read(frame.GetPayloadPtr(), m_bytesPerBlock)) {
    m_wavFile.Close();
    m_wavFile.Open();
    PTRACE(3, "Jester\tRestarted sound file");
  }

  DWORD elapsedTimestamp = m_generateTimestamp - m_initialTimestamp;
  JitterProfileMap::const_iterator it = m_generateJitter.upper_bound(elapsedTimestamp);
  --it;
  PTRACE_IF(3, m_lastGeneratedJitter != it->second,
               "Jester\tGenerated jitter changed to " << it->second << "ms");
  m_lastGeneratedJitter = it->second;
  delay.SetInterval(elapsedTimestamp/SAMPLES_PER_MILLISECOND
                          - (PTimer::Tick() - m_initialTick).GetMilliSeconds()
                          + PRandom::Number(m_lastGeneratedJitter));

  m_generateTimestamp += m_bytesPerBlock/2;

  return true;
}


void JesterProcess::ConsumePackets(PThread &, INT)
{
  if (m_startTimeDelta < 0)
    PThread::Sleep(-m_startTimeDelta);

  RTP_DataFrame readFrame((PINDEX)0, m_bytesPerBlock);
  PBYTEArray silence(m_bytesPerBlock);

  while (m_keepRunning) {
    m_jitterBuffer.ReadData(readFrame);

    PINDEX bytesInPacket = readFrame.GetPayloadSize();

    DWORD oldTimestamp;
    PSimpleTimer writeTime;
    if (bytesInPacket > 0) {
      PTRACE_IF(2, bytesInPacket != m_bytesPerBlock,
                "Jester\tPacket has " << bytesInPacket << " bytes and should have " << m_bytesPerBlock);

      m_player.Write(readFrame.GetPayloadPtr(), bytesInPacket);
      PTRACE_IF(2, bytesInPacket != m_player.GetLastWriteCount(),
                "Jester\tPacket was not correctly written to audio device.");

      oldTimestamp = readFrame.GetTimestamp();
      m_playbackTimestamp = oldTimestamp + bytesInPacket/2;
    }
    else {
      m_player.Write(silence, m_bytesPerBlock);
      oldTimestamp = m_playbackTimestamp;
      m_playbackTimestamp += m_bytesPerBlock/2;
    }
    PTRACE(5, "Jester\tWritten " << (bytesInPacket > 0 ? "audio" : "silence") << " to sound device, "
              " ts=" << oldTimestamp << ", real=" << writeTime.GetElapsed() << 's');

    readFrame.SetTimestamp(m_playbackTimestamp);
  }

  PTRACE(3, "Jester\tEnd of consume packets ");
}


void JesterProcess::ManageUserInput()
{
  for (;;) {
    // display the prompt
    cout << "(Jester) Command ? " << flush;

    int ch = cin.get();
    switch (tolower(ch)) {
        case EOF :
        case 'q' :
        case 'x' :
          m_keepRunning = false;
          cout << "Ending jitter test." << endl;
          return;

        case 'h' :
          cout << "Select:\n"
            "  X   : Exit program\n"
            "  Q   : Exit program\n"
            "  R   : Report status\n"
            "  J   : Set simulated jitter level\n"
            "  H   : Write this help out\n";
          break;

        case 'r' :
          Report();
          break;

        case 'j' :
          m_generateJitter.clear();
          cin >> m_generateJitter[0];
          cout << "Generated jitter set to " << m_generateJitter << "ms\n";
          PTRACE(3, "Jester\tGenerated jitter set to " << m_generateJitter << "ms");
          break;
    }
    cin.ignore(10000, '\n');
  }
}


void JesterProcess::Report()
{
  cout << "  Generated jitter      = " << m_lastGeneratedJitter << "ms\n"
          "  Total packets created = " << m_generateSequenceNumber << "\n"
          "  Current Jitter Delay  = " << m_jitterBuffer.GetCurrentJitterDelay() << " ("
       <<      m_jitterBuffer.GetCurrentJitterDelay()/SAMPLES_PER_MILLISECOND << "ms)\n"
          "  Current frame period  = " << m_jitterBuffer.GetAverageFrameTime() << " ("
       <<      m_jitterBuffer.GetAverageFrameTime()/SAMPLES_PER_MILLISECOND << "ms)\n"
          "  Current packet depth  = " << m_jitterBuffer.GetCurrentDepth() << "\n"
          "  Too late packet count = " << m_jitterBuffer.GetPacketsTooLate() << "\n"
          "  Packet overrun count  = " << m_jitterBuffer.GetBufferOverruns() << "\n"
       << endl;
}


// End of File ///////////////////////////////////////////////////////////////
