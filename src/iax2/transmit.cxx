/*
 *
 * Inter Asterisk Exchange 2
 * 
 * Class to implement the transmitter, which sends all packets for all calls.
 * 
 * Open Phone Abstraction Library (OPAL)
 *
 * Copyright (c) 2005 Indranet Technologies Ltd.
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
 * The Original Code is Open Phone Abstraction Library.
 *
 * The Initial Developer of the Original Code is Indranet Technologies Ltd.
 *
 * The author of this code is Derek J Smithies
 *
 *
 *  $Log: transmit.cxx,v $
 *  Revision 1.1  2005/07/30 07:01:33  csoutheren
 *  Added implementation of IAX2 (Inter Asterisk Exchange 2) protocol
 *  Thanks to Derek Smithies of Indranet Technologies Ltd. for
 *  writing and contributing this code
 *
 *
 *
 *
 */

#include <ptlib.h>

#ifdef P_USE_PRAGMA
#pragma implementation "transmit.h"
#endif

#include <iax2/transmit.h>

#define new PNEW

Transmit::Transmit(IAX2EndPoint & _newEndpoint, PUDPSocket & _newSocket)
  : PThread(1000, NoAutoDeleteThread),
     ep(_newEndpoint),
     sock(_newSocket)
{
  sendNowFrames.Initialise();
  ackingFrames.Initialise();
  
  keepGoing = TRUE;
  
  PTRACE(6,"Successfully constructed");
  Resume();
}

Transmit::~Transmit()
{
  keepGoing = FALSE;
  activate.Signal();
  
  if(WaitForTermination(1000))
    PTRACE(1, "Has Terminated just FINE");
  else
    PTRACE(1, "ERROR Did not terminate");
  
  sendNowFrames.AllowDeleteObjects();
  ackingFrames.AllowDeleteObjects();
  PTRACE(3,"Destructor finished");
}


void Transmit::SendFrame(Frame *newFrame)
{
  PTRACE(5,"Process request to send frame " << newFrame->IdString());
  
  sendNowFrames.AddNewFrame(newFrame);
  PTRACE(5, "Transmit, sendNowFrames has " << sendNowFrames.GetSize() << " entries");
  
  activate.Signal();
}

void Transmit::PurgeMatchingFullFrames(Frame *newFrame)
{
  if (!PIsDescendant(newFrame, FullFrame))
    return;
  
  PTRACE(4, "Purge frames matching  received " << newFrame->IdString());
  ackingFrames.DeleteMatchingSendFrame((FullFrame *)newFrame);
}

void Transmit::Main()
{
  SetThreadName("Transmit");
  while(keepGoing) {
    activate.Wait();

    ReportLists();
    ProcessAckingList();
    
    ProcessSendList();
  }
  PTRACE(3, " End of the Transmit thread.");
  
}

void Transmit::ProcessAckingList()
{
  PTRACE(3,"TASK 1 of 2: ackingFrameList");
  
  FrameList framesToSend;
  framesToSend.Initialise();
  
  ackingFrames.GetResendFramesDeleteOldFrames(framesToSend);
  
  framesToSend.MarkAllAsResent();
  
  sendNowFrames.GrabContents(framesToSend);
}

void Transmit::ReportLists()
{
  PTRACE(3, "Transmit\tSend now frames is: ");
  sendNowFrames.ReportList();
  PTRACE(3, "Transmit\tAckingFrames is:");
  ackingFrames.ReportList();
}

void Transmit::ProcessSendList()
{
  PTRACE(3,"TASK 2 of 2: ProcessSendList");
  PTRACE(3,"SendList has " << sendNowFrames.GetSize() << " elements");
  
  for(;;) {
    Frame * active = sendNowFrames.GetLastFrame();
    if (active == NULL) {
      PTRACE(3, "Transmit has emptied the sendNowFrames list, so finish (for now)");
      break;
    }
    PTRACE(3, "Transmit\tProcess (or send) frame " << active->IdString());

    BOOL isFullFrame = FALSE;
    if (PIsDescendant(active, FullFrame)) {
      isFullFrame = TRUE;
      FullFrame *f= (FullFrame *)active;
      if (f->DeleteFrameNow()) {
	PTRACE(6, "This frame has timed out, so do not transmit" <<  f->IdString());
	delete f;
	continue;
      }
    }
    
    if (!active->TransmitPacket(sock)) {
      PTRACE(3,"Delete  " << active->IdString() << " as transmit failed.");
      delete active;
      continue;
    }
    
    if (!isFullFrame) {
      PTRACE(3, "Delete this frame as it is a mini frame, and continue" << active->IdString());
      delete active;
      continue;
    }
    
    FullFrame *f= (FullFrame *)active;
    if (f->IsAckFrame()) {
      PTRACE(3, "Delete this frame as it is an ack frame, and continue" << f->IdString());
      delete f;
      continue;
    }
    
    if (!active->CanRetransmitFrame()) {
      PTRACE(3, "Delete this frame now as it does not need to be retransmitted." << f->IdString());
      delete f;
      continue;
    }
    
    PTRACE(3, "Put " << f->IdString() << " onto acking list");
    ackingFrames.AddNewFrame(active);
    PTRACE(3, "Acking frames has " << ackingFrames.GetSize() << " elements");
  }
}

/* The comment below is magic for those who use emacs to edit this file. */
/* With the comment below, the tab key does auto indent to 4 spaces.     */

/*
 * Local Variables:
 * mode:c
 * c-file-style:linux
 * c-basic-offset:2
 * End:
 */

