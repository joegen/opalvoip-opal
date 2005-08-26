/*
 *
 * Inter Asterisk Exchange 2
 * 
 * Implementation of the class that connects media to Opal and IAX2
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
 * $Log: iax2medstrm.cxx,v $
 * Revision 1.4  2005/08/26 03:26:51  dereksmithies
 * Add some tidyups from Adrian Sietsma.  Many thanks..
 *
 * Revision 1.3  2005/08/26 03:07:38  dereksmithies
 * Change naming convention, so all class names contain the string "IAX2"
 *
 * Revision 1.2  2005/08/24 01:38:38  dereksmithies
 * Add encryption, iax2 style. Numerous tidy ups. Use the label iax2, not iax
 *
 * Revision 1.1  2005/07/30 07:01:33  csoutheren
 * Added implementation of IAX2 (Inter Asterisk Exchange 2) protocol
 * Thanks to Derek Smithies of Indranet Technologies Ltd. for
 * writing and contributing this code
 *
 *
 */

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "iax2medstrm.h"
#endif

#include <codec/vidcodec.h>
#include <iax2/frame.h>
#include <iax2/iax2con.h>
#include <iax2/iax2medstrm.h>
#include <lids/lid.h>
#include <opal/mediastrm.h>
#include <opal/patch.h>
#include <ptlib/videoio.h>
#include <rtp/rtp.h>


#define MAX_PAYLOAD_TYPE_MISMATCHES 10


#define new PNEW


///////////////////////////////////////////////////////////////////////////////

OpalIAX2MediaStream::OpalIAX2MediaStream(const OpalMediaFormat & mediaFormat,
				       unsigned sessionID,   
				       BOOL isSource,        
				       unsigned minJitterDelay,
				       unsigned maxJitterDelay,
				       IAX2Connection &con)
  : OpalMediaStream(mediaFormat, sessionID, isSource),
    connection(con),
    minAudioJitterDelay(minJitterDelay),
    maxAudioJitterDelay(maxJitterDelay)
{
  PTRACE(3, "Media\tCREATE an opal iax media audio stream ");
}
 
 
BOOL OpalIAX2MediaStream::Open()
{
  if (isOpen)
    return TRUE;

  BOOL res = OpalMediaStream::Open();
  PTRACE(3, "Media\tOpalIAX2MediaStream set to " << mediaFormat << " is now open");
 
  return res;
}
 
BOOL OpalIAX2MediaStream::Start()
{
  PTRACE(2, "Media\tOpalMediaStream is a " << PString(IsSink() ? "sink" : "source"));

  return OpalMediaStream::Start();
}
  
BOOL OpalIAX2MediaStream::Close()
{
  BOOL res = OpalMediaStream::Close();

  PTRACE(3, "Media\tOpalIAX2MediaStream of " << mediaFormat << " is now closed"); 
  return res;
}
 
 
BOOL OpalIAX2MediaStream::ReadData(BYTE * buffer, PINDEX size, PINDEX & length)
{
  PTRACE(6, "Media\tRead data of " << size << " bytes max");
  length = 0;
  
  if (IsSink()) {
    PTRACE(1, "Media\tTried to read from sink media stream");
    return FALSE;
  }

  if (!isOpen) {
    PTRACE(3, "Media\tStream has been closed, so exit now");
    return FALSE;
  }
    

  PINDEX pendingSize;
  pendingSize = pendingData.GetSize();
  if (pendingSize > 0) {
    if (size < pendingSize) {
      memcpy(buffer, pendingData.GetPointer(), size);
      length = size;
      memmove(pendingData.GetPointer(), pendingData.GetPointer() + size, pendingSize - size);
      pendingData.SetSize(pendingSize - size);
      PTRACE(6, "Media\tPending size was " << pendingSize << " and read size was " << size);
      return TRUE;
    } else {
      memcpy(buffer, pendingData.GetPointer(), pendingSize);
      length += pendingSize;
      pendingData.SetSize(0);
      PTRACE(6, "Media\tPick up "<< pendingSize << " from the pending data in our quest to reead " << size);
    }
  }
  
  IAX2Frame *res = connection.GetSoundPacketFromNetwork();
  if ((res == NULL) && (length > 0)) {
    PTRACE(3, "Finished getting media data. Send " << length);
    return TRUE;
  }

  if (res == NULL) {
    do {
      if (connection.GetPhase() == OpalConnection::ReleasedPhase) {
	PTRACE(3, "Media\tExit now from opal media stream" << *this);
	return FALSE;
      }

      PThread::Sleep(10); //Under windows may not be 10ms..
      PTRACE(6, "Media\tJust slept another 10ms cause read nothing in last iteration ");
      res = connection.GetSoundPacketFromNetwork();
      if (res != NULL) {
	PTRACE(6, "Media\tNow we have data to process " << res->IdString());
      }
    } while ((res == NULL) && isOpen);
  }

  if (res == NULL) {
    PTRACE(3, "Media\tWe have looped and looped, but still have a null");
    return FALSE;
  }

  PTRACE(6, "Media\tThis frame has " << res->GetMediaDataSize() << " bytes of media");
  if (res->GetMediaDataSize() <= (size - length)) {
    memcpy(buffer + length, res->GetMediaDataPointer(), res->GetMediaDataSize());
    length = length + res->GetMediaDataSize();
    delete res;
    PTRACE(3, "Media\t have written to supplied data array & exit");
    return TRUE;
  }

  PINDEX bytesToCopy = size - length;
  memcpy(buffer + length, res->GetMediaDataPointer(), bytesToCopy);
  pendingData.SetSize(res->GetMediaDataSize() - bytesToCopy);
  memcpy(pendingData.GetPointer(), res->GetMediaDataPointer() + bytesToCopy, pendingData.GetSize());
  length = size;
  delete res;

  PTRACE(3, "Media\tOk, we have to save some to pending... ");
  return TRUE;
}


// This routine takes data from the source (eg mic) and sends the data to the remote host.
BOOL OpalIAX2MediaStream::WriteData(const BYTE * buffer, PINDEX length, PINDEX & written)
{
  written = 0;
  if (IsSource()) {
    PTRACE(1, "Media\tTried to write to source media stream");
    return FALSE;
  }
  PTRACE(6, "Media\tSend data to the network : have " << length << " bytes to send to remote host");
  PBYTEArray *sound = new PBYTEArray(buffer, length);
  written = length;
  connection.PutSoundPacketToNetwork(sound);

  return TRUE;
}

BOOL OpalIAX2MediaStream::IsSynchronous() const
{
  if (IsSource())
    return TRUE;

  /**We are reading from a sound card, which generates a frame at a regular rate*/
  return TRUE;
}


/////////////////////////////////////////////////////////////////////////////

