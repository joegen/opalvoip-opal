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

#include <ptlib.h>

#include <opal/buildopts.h>
#include <rtp/rtp.h>

#include "main.h"
#include "../../version.h"


#define new PNEW


PCREATE_PROCESS(JesterProcess);

PSyncPoint newDataReady;
PMutex     dataFrameMutex;
RTP_DataFrame *dataFrame;
BOOL       runningOk;

///////////////////////////////////////////////////////////////

JestRTP_Session::JestRTP_Session()
    : RTP_Session(NULL, 101)
{
    PTRACE(4, "constructor of the rtp session tester");
    psuedoTimestamp = 0;
    psuedoSequenceNo = 0;
    closedDown = FALSE;
    readCount = 0;
}

BOOL JestRTP_Session::ReadData(RTP_DataFrame & frame, BOOL)
{
    if (!runningOk) {
	PTRACE(3, "End of rtp session, return false");
	return FALSE;
    }

    while(TRUE) {
	newDataReady.Wait();
	if (!runningOk)
	    return FALSE;

	dataFrameMutex.Wait();
	if (dataFrame == NULL) {
	    dataFrameMutex.Signal();
	    continue;
	}
	frame = *dataFrame;
	
	dataFrame = NULL;
	dataFrameMutex.Signal();

	return runningOk;
    }

    return runningOk;
}



void JestRTP_Session::Close(
    BOOL /*reading */   ///<  Closing the read side of the session
    ) 
{
    PTRACE(3, "Close the rtp thing down ");
    closedDown = TRUE; 
    runningOk = FALSE;
    newDataReady.Signal();
}

BOOL JestRTP_Session::WriteData(
    RTP_DataFrame & frame  
    )
{
    PTRACE(4, "WriteData has a frame of time " << (frame.GetTimestamp() >> 3));
    return TRUE;
}

BOOL JestRTP_Session::WriteControl(
    RTP_ControlFrame & /*frame*/
    )
{
    PTRACE(4, "WriteControl has a frame");
    return TRUE;
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
	     "d-duration:"
	     "i-iterations:"
	     "j-jitter:"
	     "s-silence."
             "h-help."
#if PTRACING
             "o-output:"
             "t-trace."
#endif
	     "v-version."
          , FALSE);


  if (args.HasOption('h') ) {
      cout << "Usage : " << GetName() << " [options] \n"
	  
	  "General options:\n"
	  "  -d --duration        : duration of each packet, default is 30ms. \n"
	  "  -i --iterations #    : number of packets to ask for  (default is 80)\n"
	  "  -s --silence         : simulate silence suppression. - so audio is sent in bursts.\n"
	  "  -j --jitter #        : size of the jitter buffer in ms (100) \n"
#if PTRACING
	  "  -t --trace           : Enable trace, use multiple times for more detail.\n"
	  "  -o --output          : File for trace output, default is stderr.\n"
#endif

	  "  -h --help            : This help message.\n"
	  "  -v --version         : report version and program info.\n"
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



  runningOk     = TRUE;
  iterations    = 80;
  duration      = 30;
  minJitterSize = 100;

  if (args.HasOption('i'))
      iterations = args.GetOptionString('i').AsInteger();

  if (args.HasOption('d'))
      duration = args.GetOptionString('d').AsInteger();

  if (args.HasOption('j'))
      minJitterSize = args.GetOptionString('j').AsInteger();

  testSession.SetJitterBufferSize(8 * minJitterSize, 8 * minJitterSize * 3);

  silenceSuppression = args.HasOption('s');

  PTRACE(3, "Process " << iterations << " psuedo packets");
  PTRACE(3, "Process packets of size " << duration << " ms");


  PThread * writer = PThread::Create(PCREATE_NOTIFIER(GenerateUdpPackets), 0,
				     PThread::NoAutoDeleteThread,
				     PThread::NormalPriority,
				     "generate");

  PThread::Sleep(10);

  PThread * reader = PThread::Create(PCREATE_NOTIFIER(ConsumeUdpPackets), 0,
				     PThread::NoAutoDeleteThread,
				     PThread::NormalPriority,
				     "consume");



  writer->WaitForTermination();

  reader->WaitForTermination();

  delete writer;
  delete reader;
  
}


void JesterProcess::GenerateUdpPackets(PThread &, INT )
{
    PAdaptiveDelay delay;
    BOOL lastFrameWasSilence = TRUE;
    for(PINDEX i = 0; i < iterations; i++) {
	if (silenceSuppression && ((i % 1000) > 700)) {
	    PTRACE(3, "Don't send this frame - silence period");
	    lastFrameWasSilence = TRUE;
	} else {
	    RTP_DataFrame *frame = new RTP_DataFrame;
	    frame->SetMarker(lastFrameWasSilence);
	    frame->SetPayloadType(RTP_DataFrame::PCMU);
	    frame->SetSyncSource(0x12345678);
	    frame->SetSequenceNumber((WORD)(i + 100));
	    frame->SetPayloadSize((duration/30) * 240); 
	    
	    frame->SetTimestamp( 0 + (i  * duration * 8));
	    
	    PTRACE(3, "GenerateUdpPacket    iteration " << i 
		   << " with time of " << (frame->GetTimestamp() >> 3) << " ms");

	    dataFrameMutex.Wait();
	    if (dataFrame != NULL) {
		delete frame;
	    } else {
		dataFrame = frame;
	    }
	    dataFrameMutex.Signal();
	    newDataReady.Signal();
	    lastFrameWasSilence = FALSE;
	}

	delay.Delay(duration);
	switch (i % 2) 
	{
	    case 0: 
		break;
	    case 1: PThread::Sleep(10);
		break;
	}
//	PThread::Sleep(duration + ((i % 4) * 5));
    }
    PTRACE(3, "End of generate udp packets ");
    runningOk = FALSE;
}


void JesterProcess::ConsumeUdpPackets(PThread &, INT)
{
  RTP_DataFrame readFrame;
  DWORD readTimestamp = 0;
  PAdaptiveDelay readDelay;

  for(PINDEX i = 0; i < iterations; i++) {
      BOOL success = testSession.ReadBufferedData(readTimestamp, readFrame);
      readDelay.Delay(duration);
      if (success && (readFrame.GetPayloadSize() > 0)) {
	  readTimestamp = (duration * 8) + readFrame.GetTimestamp();
	  if (readFrame.GetMarker()) {
	      PTRACE(3, "Start of speech burst");
	  }
	  PTRACE(4, "ReadBufferedData " << ::setw(4) << i << " has given us " 
		 << (readFrame.GetTimestamp() >> 3) << " on payload size " 
		 << readFrame.GetPayloadSize() << "    jitter len is "
		 << (testSession.GetJitterBufferSize() >> 3) << "ms" );
      } else {
	  PTRACE(4, "ReadBufferedData     iteration " << ::setw(4) << i << " BAD READ");
	  PTRACE(4, "Get a frame, timestamp" << (readFrame.GetTimestamp() >> 3) << "ms"
		 << " payload " << readFrame.GetPayloadSize()  << " bytes  "
		 << "    good read=" << (success? ("TRUE") : ("FALSE")));
	  readTimestamp += (duration * 8);
      }
  }

  testSession.Close(TRUE);
  PTRACE(3, "Closed the RTP Session");

  PTRACE(3, "End of consume udp packets ");
}


// End of File ///////////////////////////////////////////////////////////////
