/*
 *
 * Inter Asterisk Exchange 2
 * 
 * Class to implement a jitter buffer for the iax media packets.
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
 *
 * $Log: iax2jitter.cxx,v $
 * Revision 1.3  2007/01/14 21:06:07  dereksmithies
 * Fix a memory leak. The RTP_DataFrame class does need to be handled more
 * efficiently, but this will do for now.
 *
 * Revision 1.2  2007/01/13 00:04:57  rjongbloed
 * Fixed compilation on DevStudio 2003
 *
 * Revision 1.1  2007/01/11 03:02:16  dereksmithies
 * Remove the previous audio buffering code, and switch to using the jitter
 * buffer provided in Opal. Reduce the verbosity of the log mesasges.
 *
 *
 *
 *
 */

#include <ptlib.h>

#ifdef P_USE_PRAGMA
#pragma implementation "iax2jitter.h"
#endif


#include <iax2/iax2jitter.h>
#include <rtp/rtp.h>

#define new PNEW  
////////////////////////////////////////////////////////////////////////////////
PendingRtpDataFrames::PendingRtpDataFrames()
{ 
    keepGoing = TRUE;
    AllowDeleteObjects(FALSE);
}

PendingRtpDataFrames::~PendingRtpDataFrames()
{
    AllowDeleteObjects();
}

RTP_DataFrame * PendingRtpDataFrames::InternalGetLastFrame() 
{
  PWaitAndSignal m(mutex);

  if (!keepGoing)
      return NULL;
  
  PINDEX size = PAbstractList::GetSize();
  
  if (size == 0)
    return NULL;
  
  return (RTP_DataFrame *) RemoveAt(size - 1);
}

RTP_DataFrame * PendingRtpDataFrames::GetLastFrame() 
{
  while (keepGoing) {
    RTP_DataFrame * answer = InternalGetLastFrame();
    if (answer != NULL)
      return answer;

    if (!keepGoing)
      return NULL;  

    activate.Wait();
  }
  return NULL;
}

void PendingRtpDataFrames::AddNewFrame(RTP_DataFrame *newElem)
{
  mutex.Wait();
  InsertAt(0, newElem);
  mutex.Signal();

  activate.Signal();
}

void PendingRtpDataFrames::CloseDown()
{
    keepGoing = FALSE;
    activate.Signal();
}

/////////////////////////////////////////////////////////////////////////////

IAX2JitterBuffer::IAX2JitterBuffer()
    : OpalJitterBuffer(400, 2000, 8, 30000)
{
    PTRACE(6, "IAX2\tConstruct iax2 jitter buffer");
}

IAX2JitterBuffer::~IAX2JitterBuffer()
{
    receivedFrames.CloseDown();
    WaitForTermination(1000);
}

BOOL IAX2JitterBuffer::OnReadPacket(RTP_DataFrame & frame,
				    BOOL /*loop*/)
{
    RTP_DataFrame *oldestFrame = receivedFrames.GetLastFrame();

    if (oldestFrame == NULL)
	return FALSE;

    frame = *oldestFrame;
    delete oldestFrame;

    return TRUE;
}

BOOL IAX2JitterBuffer::IsEmpty()
{
    PWaitAndSignal m(bufferMutex);

    return oldestFrame == NULL;
}



////////////////////////////////////////////////////////////////////////////////
/* The comment below is magic for those who use emacs to edit this file. */
/* With the comment below, the tab key does auto indent to 4 spaces.     */

/*
 * Local Variables:
 * mode:c
 * c-file-style:linux
 * c-basic-offset:2
 * End:
 */
