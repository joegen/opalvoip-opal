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

    newDataReady.Wait();
    if (!runningOk)
	return FALSE;

    frame = *dataFrame;
    dataFrame = NULL;

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
             MAJOR_VERSION, MINOR_VERSION, BUILD_TYPE, BUILD_NUMBER)
{
}


void JesterProcess::Main()
{
  // Get and parse all of the command line arguments.
  PArgList & args = GetArguments();
  args.Parse(
             "h-help."
	     "i-iterations:"
#if PTRACING
             "o-output:"
             "t-trace."
#endif
	     "v-version."
          , FALSE);


  if (args.HasOption('h') ) {
      cout << "Usage : " << GetName() << " [options] \n"
	  
	  "General options:\n"
	  " -i --iterations #        : number of packets to ask for \n"
#if PTRACING
	  "  -t --trace              : Enable trace, use multiple times for more detail.\n"
	  "  -o --output             : File for trace output, default is stderr.\n"
#endif

	  "  -h --help               : This help message.\n"
	  "  -v --version            : report version and program info.\n"
	  "\n"
	  "\n";
      return;
  }

  if (args.HasOption('v')) {
      cout << GetName()
	   << " Version " << GetVersion(TRUE)
	   << " by " << GetManufacturer()
	   << " on " << GetOSClass() << ' ' << GetOSName()
	   << " (" << GetOSVersion() << '-' << GetOSHardware() << ")\n\n";
      return;
  }

#if PTRACING
  PTrace::Initialise(args.GetOptionCount('t'),
                     args.HasOption('o') ? (const char *)args.GetOptionString('o') : NULL,
                     PTrace::Timestamp|PTrace::Thread|PTrace::FileAndLine);
#endif

  testSession.SetJitterBufferSize(8 * 100,8 * 1000);

  runningOk = TRUE;
  iterations = 80;
  PThread * writer = PThread::Create(PCREATE_NOTIFIER(GenerateUdpPackets), 0,
				     PThread::NoAutoDeleteThread,
				     PThread::NormalPriority);

  PThread::Sleep(50);

  RTP_DataFrame readFrame;
  DWORD readTimestamp = 0;
  PAdaptiveDelay readDelay;


  if (args.HasOption('i'))
      iterations = args.GetOptionString('i').AsInteger();

  PTRACE(3, "Read in " << iterations << " psuedo packets");

  for(PINDEX i = 0; i < iterations; i++) {
      BOOL success = testSession.ReadBufferedData(readTimestamp, readFrame);
      readDelay.Delay(40);
      if (success) {
	  readTimestamp += 320;
	  PTRACE(4, "ReadBufferedData " << ::setw(4) << i << " has given us " 
		 << (readFrame.GetTimestamp() >> 3) << " on payload size " 
		 << readFrame.GetPayloadSize());
      } else {
	  PTRACE(4, "ReadBufferedData " << ::setw(4) << i << " BAD READ");
      }
  }

  cout << "Finished reading packets " << endl;
  testSession.Close(TRUE);
  PTRACE(3, "Closed the RTP Session");

  writer->WaitForTermination();
}

void JesterProcess::GenerateUdpPackets(PThread &, INT )
{
    for(PINDEX i = 0; i < iterations; i++) {
	RTP_DataFrame *frame = new RTP_DataFrame;
	
	frame->SetPayloadType(RTP_DataFrame::GSM);
	frame->SetSyncSource(0x12345678);
	frame->SetSequenceNumber((WORD)(i + 100));
	frame->SetPayloadSize(66);

	frame->SetTimestamp( i  * 320);

	PTRACE(3, "Send a new frame at iteration " << i 
	       << " with time of " << (frame->GetTimestamp() >> 3) << " ms");
	dataFrame = frame;
	newDataReady.Signal();

	PThread::Sleep(30 + ((i % 4) * 5));
    }
    PTRACE(3, "End of generate udp packets ");
    runningOk = FALSE;
}
// End of File ///////////////////////////////////////////////////////////////
