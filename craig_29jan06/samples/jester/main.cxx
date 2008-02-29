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
 * Contributor(s): ______________________________________.
 *
 * $Log: main.cxx,v $
 * Revision 1.13  2007/01/14 22:18:35  dereksmithies
 * MOdify the period when doing silence detect, to be 69sec off,  6 sec on.
 *
 * Revision 1.12  2007/01/14 20:52:32  dereksmithies
 * Report available audio devices if it fails to open the specified device.
 *
 * Revision 1.11  2007/01/13 00:05:40  rjongbloed
 * Fixed compilation on DevStudio 2003
 *
 * Revision 1.10  2007/01/12 10:00:57  dereksmithies
 * bring it up to date so it compiles.
 *
 * Revision 1.9  2007/01/11 09:20:41  dereksmithies
 * Use the new OpalJitterBufer class, allowing easy access to the jitter buffer's internal
 * variables. Play output audio to the specified sound device.
 *
 * Revision 1.8  2006/12/08 09:00:20  dereksmithies
 * Add mutex protection of a pointer.
 * Change default to send packets at a non uniform rate, so they go at 0 20 60 80 120 140 180 etc.
 * Add ability to suppress the sending of packets, so we simulate silence suppression.
 *
 * Revision 1.7  2006/12/02 07:31:00  dereksmithies
 * Add more options - duration of each packet.
 *
 * Revision 1.6  2006/12/02 04:16:13  dereksmithies
 * Get it to terminate correctly.
 * Report on when each frame is commited, and when each frame is received.
 *
 * Revision 1.5  2006/11/23 07:55:15  rjongbloed
 * Fixed sample app build due to RTP session class API breakage.
 *
 * Revision 1.4  2006/10/02 13:30:51  rjongbloed
 * Added LID plug ins
 *
 * Revision 1.3  2006/08/25 06:04:44  dereksmithies
 * Add to the docs on the functions.  Add a new thread to generate the frames,
 * which helps make the operation of the jitterbuffer clearer.
 *
 * Revision 1.2  2006/07/29 13:42:20  rjongbloed
 * Fixed compiler warning
 *
 * Revision 1.1  2006/06/19 09:32:09  dereksmithies
 * Initial cut of a program to test the jitter buffer in OPAL.
 *
 *
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

BOOL       keepRunning;

///////////////////////////////////////////////////////////////
JesterJitterBuffer::JesterJitterBuffer():
    IAX2JitterBuffer()
{

}

JesterJitterBuffer::~JesterJitterBuffer()
{

}

void JesterJitterBuffer::Close(BOOL /*reading */ )
{
CloseDown();
}

/////////////////////////////////////////////////////////////////////////////

JesterProcess::JesterProcess()
  : PProcess("Derek Smithies Code Factory", "Jester",
             1, 1, ReleaseCode, 0)
{
}


void JesterProcess::Main()
{
  // Get and parse all of the command line arguments.
  PArgList & args = GetArguments();
  args.Parse(
             "a-audiodevice:"
	     "j-jitter:"
	     "s-silence."
             "h-help."
#if PTRACING
             "o-output:"
             "t-trace."
#endif
	     "v-version."
	     "w-wavfile:"
          , FALSE);


  if (args.HasOption('h') ) {
      cout << "Usage : " << GetName() << " [options] \n"
	  
	  "General options:\n"
	  "  -a --audiodevice     : audio device to play the output on\n"
	  "  -i --iterations #    : number of packets to ask for  (default is 80)\n"
	  "  -s --silence         : simulate silence suppression. - so audio is sent in bursts.\n"
	  "  -j --jitter #        : size of the jitter buffer in ms (100) \n"
#if PTRACING
	  "  -t --trace           : Enable trace, use multiple times for more detail.\n"
	  "  -o --output          : File for trace output, default is stderr.\n"
#endif

	  "  -h --help            : This help message.\n"
	  "  -v --version         : report version and program info.\n"
	  "  -w --wavfile         : audio file from which the source data is read from \n"
	  "\n"
	  "\n";
      return;
  }

  if (args.HasOption('v')) {
      cout << GetName()  << endl
	   << " Version " << GetVersion(TRUE) << endl
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

  minJitterSize = 100;

  if (args.HasOption('a'))
      audioDevice = args.GetOptionString('a');
  else
      audioDevice =  PSoundChannel::GetDefaultDevice(PSoundChannel::Player);

  if (args.HasOption('j'))
      minJitterSize = args.GetOptionString('j').AsInteger();

  silenceSuppression = args.HasOption('s');

  if (args.HasOption('w'))
      wavFile = args.GetOptionString('w');
  else {
      wavFile = "../../../contrib/openam/sample_message.wav";
  }
  
  bytesPerBlock = 480;

  cerr << "Jitter size is set to " << minJitterSize << " ms" << endl;

  if (!PFile::Exists(wavFile)) {
      cerr << "the audio file " << wavFile << " does not exist." << endl;
      cerr << "Terminating now" << endl;
      return;
  }

  if (!player.Open(audioDevice, PSoundChannel::Player)) {
      cerr <<  "Failed to open the sound device " << audioDevice 
	   << " to write the jittered audio to"  << endl;
      cerr << endl 
	   << "available devices are " << endl;
      PStringList namesPlay = PSoundChannel::GetDeviceNames(PSoundChannel::Player);
      for (PINDEX i = 0; i < namesPlay.GetSize(); i++)
	  cerr << i << "  " << namesPlay[i] << endl;

      cerr << endl;
      cerr << "Terminating now" << endl;      
      return;
  }

  jitterBuffer.SetDelay(8 * minJitterSize, 8 * minJitterSize * 3);
  jitterBuffer.Resume();

  keepRunning = TRUE;
  generateTimestamp = FALSE;
  consumeTimestamp = FALSE;

  PThread * writer = PThread::Create(PCREATE_NOTIFIER(GenerateUdpPackets), 0,
				     PThread::NoAutoDeleteThread,

				     PThread::NormalPriority,
				     "generate");

  PThread::Sleep(10);

  PThread * reader = PThread::Create(PCREATE_NOTIFIER(ConsumeUdpPackets), 0,
				     PThread::NoAutoDeleteThread,
				     PThread::NormalPriority,
				     "consume");


  ManageUserInput();

  writer->WaitForTermination();

  reader->WaitForTermination();

  delete writer;
  delete reader;  
}


void JesterProcess::GenerateUdpPackets(PThread &, INT )
{
    PAdaptiveDelay delay;
    BOOL lastFrameWasSilence = TRUE;
    PWAVFile soundFile(wavFile);
    generateIndex = 0;
    
    while(keepRunning) {
	generateTimestamp =  960 + (generateIndex  * 240);
	//Silence period, 45 seconds cycle, with 3 second on time.
	if (silenceSuppression && ((generateIndex % 2500) > 200)) {
	    PTRACE(3, "Don't send this frame - silence period");
	    if (lastFrameWasSilence == FALSE) {
		PTRACE(3, "Stop Audio here");
		cout << "Stop audio at " << PTime() << endl;
	    }
	    lastFrameWasSilence = TRUE;
	} else {
	    RTP_DataFrame *frame = new RTP_DataFrame;
	    if (lastFrameWasSilence) {
		PTRACE(3, "StartAudio here");
		cout << "Start Audio at " << PTime() << endl;
	    }
	    frame->SetMarker(lastFrameWasSilence);
	    lastFrameWasSilence = FALSE;
	    frame->SetPayloadType(RTP_DataFrame::L16_Mono);
	    frame->SetSyncSource(0x12345678);
	    frame->SetSequenceNumber((WORD)(generateIndex + 100));
	    frame->SetPayloadSize(bytesPerBlock);
	    
	    frame->SetTimestamp(generateTimestamp);
	    
	    PTRACE(3, "GenerateUdpPacket    iteration " << generateIndex
		   << " with time of " << frame->GetTimestamp() << " rtp time units");
	    memset(frame->GetPayloadPtr(), 0, frame->GetPayloadSize());
	    if (!soundFile.Read(frame->GetPayloadPtr(), frame->GetPayloadSize())) {
		soundFile.Close();
		soundFile.Open();
		PTRACE(3, "Reopen the sound file, as have reached the end of it");
	    }
	    jitterBuffer.NewFrameFromNetwork(frame);
	}

	delay.Delay(30);
#if 1
	switch (generateIndex % 2) 
	{
	    case 0: 
		break;
	    case 1: PThread::Sleep(30);
		break;
	}
#endif
	generateIndex++;
    }
    PTRACE(3, "End of generate udp packets ");
}


void JesterProcess::ConsumeUdpPackets(PThread &, INT)
{
  RTP_DataFrame readFrame;
  PAdaptiveDelay readDelay;
  PBYTEArray silence(bytesPerBlock);
  consumeTimestamp = 0;
  consumeIndex = 0;
  while(keepRunning) {

      BOOL success = jitterBuffer.ReadData(consumeTimestamp, readFrame);
      if (success && (readFrame.GetPayloadSize() > 0)) {
	  consumeTimestamp = readFrame.GetTimestamp();
	  player.Write(readFrame.GetPayloadPtr(), readFrame.GetPayloadSize());
	  PTRACE(3, "Play audio from the  buffer to sound device, ts=" << consumeTimestamp);
      }
      else {
	  player.Write(silence, bytesPerBlock);
	  PTRACE(3, "Play audio from silence buffer to sound device, ts=" << consumeTimestamp);	 
      }
      consumeTimestamp += 240;
      consumeIndex++;
  }

  jitterBuffer.CloseDown();

  PTRACE(3, "End of consume udp packets ");
}

void JesterProcess::ManageUserInput()
{
   PConsoleChannel console(PConsoleChannel::StandardInput);

   PStringStream help;
   help << "Select:\n";
   help << "  X   : Exit program\n"
        << "  Q   : Exit program\n"
	<< "  T   : Read and write process report their current timestamps\n"
	<< "  R   : Report iteration counts\n"
	<< "  J   : Report some of the internal variables in the jitter buffer\n"
        << "  H   : Write this help out\n";

   PThread::Sleep(100);


 for (;;) {
    // display the prompt
    cout << "(Jester) Command ? " << flush;

    // terminate the menu loop if console finished
    char ch = (char)console.peek();
    if (console.eof()) {
      cout << "\nConsole gone - menu disabled" << endl;
      goto endAudioTest;
    }

    console >> ch;
    PTRACE(3, "console in audio test is " << ch);
    switch (tolower(ch)) {
        case 'q' :
        case 'x' :
            goto endAudioTest;
	case 'r' :
	    cout << "        generate thread=" << generateIndex << "    consume thread=" << consumeIndex << endl;
	    break;
        case 'h' :
            cout << help ;
            break;
	case 't' :
	    cerr << "        Timestamps are " << generateTimestamp << "/" << consumeTimestamp << " (generate/consume)" << endl;
	    DWORD answer;
	    if (generateTimestamp > consumeTimestamp)
		answer = generateTimestamp - consumeTimestamp;
	    else
		answer = consumeTimestamp - generateTimestamp;
	    cerr << "        RTP difference " << answer << "          Milliseconds difference is "  << (answer/8) << endl;
	    break;
	case 'j' :
	    cerr << "        Target Jitter Time is  " << jitterBuffer.GetTargetJitterTime() << endl;
	    cerr << "        Current depth is       " << jitterBuffer.GetCurrentDepth() << endl;
	    cerr << "        Current Jitter Time is " << jitterBuffer.GetCurrentJitterTime() << endl;
	    break;
        default:
            ;
    }
  }

endAudioTest:
  keepRunning = FALSE;
  cout  << "end audio test" << endl;
}



// End of File ///////////////////////////////////////////////////////////////
