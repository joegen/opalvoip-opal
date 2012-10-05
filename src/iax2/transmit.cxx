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
 * $Revision$
 * $Author$
 * $Date$
 */

#include <ptlib.h>
#include <opal/buildopts.h>

#if OPAL_IAX2

#ifdef P_USE_PRAGMA
#pragma implementation "transmit.h"
#endif

#include <iax2/transmit.h>

#define new PNEW

IAX2Transmit::IAX2Transmit(IAX2EndPoint & _newEndpoint, PUDPSocket & _newSocket)
  : PThread(1000, NoAutoDeleteThread, NormalPriority, "IAX2 Transmitter"),
     ep(_newEndpoint),
     sock(_newSocket)
{
  sendNowFrames.Initialise();
  ackingFrames.Initialise();
  
  keepGoing = true;
  
  PTRACE(6,"IAX2Transmit\tConstructor - IAX2 Transmitter");
  Resume();
}

IAX2Transmit::~IAX2Transmit()
{
  Terminate();
  WaitForTermination();
  sendNowFrames.AllowDeleteObjects();
  
  IAX2FrameList notUsed; /* These frames will be destroyed at method end. */
  ackingFrames.GrabContents(notUsed);
  PTRACE(5, "IAX2Transmit\tDestructor finished");
}

void IAX2Transmit::Terminate()
{
  keepGoing = false;
  activate.Signal();
}

void IAX2Transmit::SendFrame(IAX2Frame *newFrame)
{
  sendNowFrames.AddNewFrame(newFrame);
  
  activate.Signal();
}

void IAX2Transmit::PurgeMatchingFullFrames(IAX2Frame *newFrame)
{
  if (!PIsDescendant(newFrame, IAX2FullFrame))
    return;

  PTRACE(5, "IAX2Transmit\tPurgeMatchingFullFrames to " << *newFrame);

  ackingFrames.DeleteMatchingSendFrame((IAX2FullFrame *)newFrame);
}

void IAX2Transmit::SendVnakRequestedFrames(IAX2FullFrameProtocol &src)
{
  PTRACE(4, "IAX2Transmit\tSendVnakRequestedFrames to " << src);
  ackingFrames.SendVnakRequestedFrames(src);
}

void IAX2Transmit::Main()
{
  SetThreadName("IAX2Transmit");
  while(keepGoing) {
    if (!keepGoing)
      break;

    activate.Wait();
    
    if (!keepGoing)
      break;

    ProcessAckingList();
    
    ProcessSendList();
  }
  PTRACE(6, "IAX2Transmit\tEnd of the Transmit thread.");  
}

void IAX2Transmit::ProcessAckingList()
{
  IAX2ActiveFrameList framesToSend;
  
  PTRACE(5, "IAX2Transmit\tGetResendFramesDeleteOldFrames");
  ackingFrames.GetResendFramesDeleteOldFrames(framesToSend);
  
  framesToSend.MarkAllAsResent();
  
  sendNowFrames.GrabContents(framesToSend);
}

void IAX2Transmit::ReportLists(PString & answer, bool getFullReport)
{
  PStringStream reply;
  PString aList;

  reply << "\n"
	<< PString("   SendNowFrames = ") << sendNowFrames.GetSize() << "\n";
  if (getFullReport) {
    sendNowFrames.ReportList(aList);
    reply << aList;
  }
  reply << PString("   AckingFrames  = ") << ackingFrames.GetSize() << "\n";
  if (getFullReport) {
    ackingFrames.ReportList(aList);
    reply << aList;
  }
  answer = reply;
}

void IAX2Transmit::ProcessSendList()
{
  for(;;) {
    IAX2Frame * active = sendNowFrames.GetLastFrame();
    if (active == NULL) 
      break;
    
    PBoolean isFullFrame = false;
    if (PIsDescendant(active, IAX2FullFrame)) {
      isFullFrame = true;
      IAX2FullFrame *f= (IAX2FullFrame *)active;
      if (f->DeleteFrameNow()) {
	PTRACE(6, "IAX2Transmit\tFrame timed out, do not transmit" 
	       <<  f->GetRemoteInfo());
	delete f;
	continue;
      }
    }
    
    if (!active->TransmitPacket(sock)) {
      PTRACE(4, "IAX2Transmit\tDelete  " << active->IdString() 
	     << " as transmit failed.");
      delete active;
      continue;
    }
    
    if (!isFullFrame) {
      PTRACE(5, "IAX2Transmit\tDelete this frame as it is a mini frame, and continue" << active->IdString());
      delete active;
      continue;
    }
    
    IAX2FullFrame *f= (IAX2FullFrame *)active;
    if (f->IsAckFrame() || f->IsVnakFrame()) {
      delete f;
      continue;
    }
    
    if (!active->CanRetransmitFrame()) {
      delete f;
      continue;
    }
    
    PTRACE(5, "IAX2Transmit\tAdd frame " << *active 
	   << " to list of frames waiting on acks");
    ackingFrames.AddNewFrame(active);
  }
}


#endif // OPAL_IAX2

/* The comment below is magic for those who use emacs to edit this file.
 * With the comment below, the tab key does auto indent to 2 spaces.    
 *
 * Local Variables:
 * mode:c
 * c-basic-offset:2
 * End:
 */

