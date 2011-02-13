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

#define OVERRUN_AT_START_PACKETS 0
#define INITIAL_NOJITTER_TIME 1000 /*timestamp units*/


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
  , m_generateSequenceNumber(0)
  , m_playbackTimestamp(0)
  , m_keepRunning(true)
  , m_lastFrameWasSilence(true)
  , m_talkBurstTimestamp(0)
  , m_lastSilentTimestamp(0)
  , m_lastGeneratedJitter(0)
{
}


void JesterProcess::Main()
{
  // Get and parse all of the command line arguments.
  PArgList & args = GetArguments();
  args.Parse(
    "a-audiodevice:"
    "D-start-delta:"
    "d-drop."
    "j-jitter:"
    "g-generate:"
    "I-init-play-ts:"
    "i-init-gen-ts:"
    "s-silence."
    "S-size:"
    "h-help."
    "m-marker."
    "R."
#if PTRACING
    "o-output:"
    "t-trace."
#endif
    "v-version."
    "w-wavfile:"
    , false);


  if (args.HasOption('h') ) {
    cerr << "Usage : " << GetName() << " [options] \n"
            "General options:\n"
            "  -w --wavfile          : audio file from which the source data is read from \n"
            "  -a --audiodevice      : audio device to play the output on\n"
            "  -s --silence          : simulate silence suppression. - so audio is sent in bursts.\n"
            "  -d --drop             : simulate dropped packets.\n"
            "  -j --jitter [min-]max : size of the jitter buffer in ms (100-1000)\n"
            "  -g --generate profile : amount of jitter to simulate (see below)\n"
            "  -S --size max         : size of each RTP packet in ms\n"
            "  -m --marker           : turn some of the marker bits off, that indicate speech bursts\n"
            "  -D --start-delta n    : Start delta time between generator and playback (ms)\n"
            "  -I --init-play-ts n   : Initial timestamp value for generator\n"
            "  -i --init-gen-ts n    : Initial timestamp value for playback\n"
#if PTRACING
            "  -t --trace            : Enable trace, use multiple times for more detail.\n"
            "  -o --output           : File for trace output, default is stderr.\n"
#endif
            "  -h --help             : This help message.\n"
            "  -v --version          : report version and program info.\n"
            "\n"
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

#if PTRACING
  PTrace::Initialise(args.GetOptionCount('t'),
    args.HasOption('o') ? (const char *)args.GetOptionString('o') : NULL,
    PTrace::Timestamp|PTrace::Thread|PTrace::FileAndLine);
#endif

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

  cout << "Jitter buffer size: " << minJitterSize << ".." << maxJitterSize << " ms\n"
          "Packet size       : " << m_bytesPerBlock << " bytes (" << m_bytesPerBlock/SAMPLES_PER_MILLISECOND/2 << "ms)\n"
          "Generated jitter  : " << m_generateJitter << "\n"
          "Silence periods   : " << (m_silenceSuppression ? "yes" : "no") << "\n"
          "Suppress markers  : " << (m_markerSuppression ? "yes" : "no") << "\n"
          "Audio device      : " << m_player.GetName() << "\n"
          "WAV file          : " << m_wavFile.GetName() << '\n'
       << endl;

  if (args.HasOption('R')) {
    RTP_DataFrame readFrame(m_bytesPerBlock);
    PTimeInterval genTick;
    PTimeInterval outTick = m_startTimeDelta;
    DWORD initialTimestamp = m_generateTimestamp;
    while (m_generateSequenceNumber < 4000) {
      while (genTick < outTick) {
        RTP_DataFrame frame;
        if (GenerateFrame(frame)) {
          PTRACE(5, "Jester\tInserting frame : "
                    "ts=" << setw(5) << frame.GetTimestamp() << " "
                    "tick=" << setw(7) << genTick << " "
                    "(" << PTimeInterval(m_generateTimestamp/SAMPLES_PER_MILLISECOND) << ')');
          if (!m_jitterBuffer.WriteData(frame, genTick))
            break;
        }

        m_generateTimestamp += m_bytesPerBlock/2;
        if (m_generateSequenceNumber < OVERRUN_AT_START_PACKETS)
          initialTimestamp = m_generateTimestamp;
        else {
          DWORD elapsedTimestamp = m_generateTimestamp-initialTimestamp;
          unsigned newTick = elapsedTimestamp/SAMPLES_PER_MILLISECOND;
          if (m_generateTimestamp > INITIAL_NOJITTER_TIME) {
            JitterProfileMap::const_iterator it = m_generateJitter.upper_bound(elapsedTimestamp);
            --it;
            PTRACE_IF(3, m_lastGeneratedJitter != it->second,
                         "Jester\tGenerated jitter changed to " << it->second << "ms");
            m_lastGeneratedJitter = it->second;
            newTick += PRandom::Number(m_lastGeneratedJitter);
          }
          if (genTick < newTick)
            genTick = newTick;
        }
      }

      readFrame.SetTimestamp(m_playbackTimestamp);
      if (!m_jitterBuffer.ReadData(readFrame, outTick))
        break;

      PTRACE(5, "Jester\tExtracted frame : "
                "ts=" << setw(5) << readFrame.GetTimestamp() << " "
                "tick=" << setw(7) << outTick << " "
                "sz=" << readFrame.GetPayloadSize());
      outTick += m_bytesPerBlock/SAMPLES_PER_MILLISECOND/2;
      if (readFrame.GetPayloadSize() != 0)
        m_playbackTimestamp = readFrame.GetTimestamp() + readFrame.GetPayloadSize()/2;
      else
        m_playbackTimestamp += m_bytesPerBlock/2;
    }
  }
  else {
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

  PTimeInterval startTick = PTimer::Tick();
  DWORD initialTimestamp = m_generateTimestamp;

  while (m_keepRunning) {
    DWORD elapsedTimestamp = m_generateTimestamp - initialTimestamp;
    JitterProfileMap::const_iterator it = m_generateJitter.upper_bound(elapsedTimestamp);
    --it;
    PTRACE_IF(3, m_lastGeneratedJitter != it->second,
                 "Jester\tGenerated jitter changed to " << it->second << "ms");
    m_lastGeneratedJitter = it->second;
    int delay = (int)(elapsedTimestamp/SAMPLES_PER_MILLISECOND
                      - (PTimer::Tick() - startTick).GetMilliSeconds()
                      + PRandom::Number(m_lastGeneratedJitter));
    if (delay > 0)
      PThread::Sleep(delay);
    else
      delay = 0;

    RTP_DataFrame frame;
    if (GenerateFrame(frame)) {
      PTRACE(5, "Jester\tInserting frame : ts=" << m_generateTimestamp << ", delay=" << delay << "ms");
      m_jitterBuffer.WriteData(frame);
    }

    m_generateTimestamp += m_bytesPerBlock/2;
  }

  PTRACE(3, "Jester\tEnd of generate packets ");
}


bool JesterProcess::GenerateFrame(RTP_DataFrame & frame)
{
  if (m_silenceSuppression && !m_lastFrameWasSilence &&
          (m_generateTimestamp - m_talkBurstTimestamp) > (10000*SAMPLES_PER_MILLISECOND)) {
    PTRACE(3, "Jester\tStarted silence : ts=" << m_generateTimestamp);
    m_lastFrameWasSilence = true;
    m_lastSilentTimestamp = m_generateTimestamp;
    return false;
  }

  if (m_lastSilentTimestamp != 0 && (m_generateTimestamp - m_lastSilentTimestamp) < (2000*SAMPLES_PER_MILLISECOND)) {
    PTRACE(5, "Jester\tSilence active  : ts=" << setw(5) << m_generateTimestamp);
    return false;
  }

  if (m_dropPackets && m_generateSequenceNumber%100 == 0) {
    PTRACE(4, "Jester\tDropped packet  : ts=" << setw(5) << m_generateTimestamp);
    return false;
  }

  frame.SetPayloadSize(m_bytesPerBlock);
  frame.SetPayloadType(RTP_DataFrame::L16_Mono);
  frame.SetSequenceNumber(++m_generateSequenceNumber);
  frame.SetTimestamp(m_generateTimestamp);
  frame.SetMarker(m_lastFrameWasSilence && (!m_markerSuppression || (m_generateSequenceNumber&1) == 0));

  if (m_lastFrameWasSilence) {
    PTRACE(3, "Jester\tBegin talk burst: ts=" << setw(5) << m_generateTimestamp);
    m_talkBurstTimestamp = m_generateTimestamp;
    m_lastFrameWasSilence = false;
  }

  if (!m_wavFile.Read(frame.GetPayloadPtr(), m_bytesPerBlock)) {
    m_wavFile.Close();
    m_wavFile.Open();
    PTRACE(3, "Jester\tRestarted sound file");
  }

  return true;
}


void JesterProcess::ConsumePackets(PThread &, INT)
{
  if (m_startTimeDelta < 0)
    PThread::Sleep(-m_startTimeDelta);

  RTP_DataFrame readFrame(0, m_bytesPerBlock);
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
