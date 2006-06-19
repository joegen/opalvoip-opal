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
 * Revision 1.1  2006/06/19 09:32:09  dereksmithies
 * Initial cut of a program to test the jitter buffer in OPAL.
 *
 *
 */

#include <ptlib.h>

#include <opal/buildopts.h>

#include "main.h"
#include "../../version.h"


#ifdef OPAL_STATIC_LINK
#define H323_STATIC_LIB
#include <codec/allcodecs.h>
#include <lids/alllids.h>
#endif


#define new PNEW


PCREATE_PROCESS(JesterProcess);


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
    if (closedDown) {
	PTRACE(3, "No more reading of data, we have to close down ");
	return FALSE;
    }

    frame.SetSize(100);
    frame.SetPayloadType(RTP_DataFrame::GSM);
    frame.SetTimestamp(psuedoTimestamp);
    frame.SetSequenceNumber(psuedoSequenceNo);
    frame.SetSyncSource(123456789);

    readCount++;
    PTimeInterval delta = PTime() - startJester;
    int delay = delta.GetMilliSeconds() - (readCount * 30);

    if (delay < 0) {
	delay = 0 - delay;
	PINDEX perturbation = variation.Number() % 20;
	if (delay > 20) 
	    delay = delay - perturbation;
//	PTRACE(3, "Delay is randomized to " << delay);
	PThread::Sleep(delay);
    }
    PTRACE(3, "Supply a frame of timestamp " 
	   << ::setw(7) << (psuedoTimestamp >> 3) 
	   << "    reality is " << (readCount * 30));
    psuedoTimestamp += 240;
    psuedoSequenceNo++;
			
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
    RTP_ControlFrame & frame 
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

  JestRTP_Session testSession;
  testSession.SetJitterBufferSize(8 * 100,8 * 1000);
  PThread::Sleep(50);

  RTP_DataFrame readFrame;
  DWORD readTimestamp = 480;
  PAdaptiveDelay readDelay;

  PINDEX iterations = 40;
  if (args.HasOption('i'))
      iterations = args.GetOptionString('i').AsInteger();

  PTRACE(3, "Run for " << iterations << " psuedo packets");

  for(PINDEX i = 0; i < iterations; i++) {
      testSession.ReadBufferedData(readTimestamp, readFrame);
      readDelay.Delay(30);
      PTRACE(4, "ReadBufferedData " << ::setw(4) << i << " has given us " << (readFrame.GetTimestamp() >> 3));
      
      readTimestamp += 240;
  }

  cout << "Finished experiment" << endl;
  testSession.Close(TRUE);

  PThread::Sleep(1000);
}


// End of File ///////////////////////////////////////////////////////////////
