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


#ifdef OPAL_STATIC_LINK
#define H323_STATIC_LIB
#include <codec/allcodecs.h>
#include <lids/alllids.h>
#endif


#define new PNEW


PCREATE_PROCESS(JesterProcess);

PSyncPoint newDataReady;
RTP_DataFrame *dataFrame;

///////////////////////////////////////////////////////////////

JestRTP_Session::JestRTP_Session()
    : RTP_Session(101)
{
    PTRACE(4, "constructor of the rtp session tester");
    psuedoTimestamp = 480;
    psuedoSequenceNo = 5023;
    closedDown = FALSE;
    readCount = 0;
}

BOOL JestRTP_Session::ReadData(
    RTP_DataFrame & frame   ///<  Frame read from the RTP session
    )
{
    newDataReady.Wait();
    frame = *dataFrame;
    dataFrame = NULL;

    PTRACE(4, "ReadData has a frame of time " << (frame.GetTimestamp() >> 3));
    return TRUE;
}

void JestRTP_Session::Close(
    BOOL /*reading */   ///<  Closing the read side of the session
    ) 
{
    PTRACE(3, "Close the rtp thing down ");
    closedDown = TRUE; 
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

  iterations = 80;
  PThread * writer = PThread::Create(PCREATE_NOTIFIER(GenerateUdpPackets), 0,
				     PThread::NoAutoDeleteThread,
				     PThread::NormalPriority);

  PThread::Sleep(50);

  RTP_DataFrame readFrame;
  DWORD readTimestamp = 480;
  PAdaptiveDelay readDelay;


  if (args.HasOption('i'))
      iterations = args.GetOptionString('i').AsInteger();

  PTRACE(3, "Run for " << iterations << " psuedo packets");

  for(PINDEX i = 0; i < iterations; i++) {
      testSession.ReadBufferedData(readTimestamp, readFrame);
      readDelay.Delay(40);
      PTRACE(4, "ReadBufferedData " << ::setw(4) << i << " has given us " << (readFrame.GetTimestamp() >> 3));
      
      readTimestamp += 480;
  }

  cout << "Finished experiment" << endl;
  testSession.Close(TRUE);
  writer->WaitForTermination();
}

void JesterProcess::GenerateUdpPackets(PThread &, INT )
{
    for(PINDEX i = 0; i < iterations; i++) {
	RTP_DataFrame *frame = new RTP_DataFrame;
	
	frame->SetPayloadType(RTP_DataFrame::GSM);
	frame->SetSyncSource(0x12345678);
	frame->SetSequenceNumber(i + 100);
	frame->SetPayloadSize(66);

	frame->SetTimestamp( (i + 100) * 8 * 20);

	PTRACE(3, "Send a new frame at iteration " << i);
	PTRACE(3, "The pin is " << ::hex << dataFrame << ::dec);
	dataFrame = frame;
	newDataReady.Signal();

	PThread::Sleep(35 + ((i % 4) * 5));
    }
}
// End of File ///////////////////////////////////////////////////////////////
