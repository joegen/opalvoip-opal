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
 * $Revision$
 * $Author$
 * $Date$
 */

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "iax2medstrm.h"
#endif

#if OPAL_VIDEO
#include <codec/vidcodec.h>
#endif

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

OpalIAX2MediaStream::OpalIAX2MediaStream(IAX2Connection & conn, 
                                  const OpalMediaFormat & mediaFormat,
				                                         unsigned sessionID,   
				                                             BOOL isSource)
  : OpalMediaStream(conn, mediaFormat, sessionID, isSource),
    connection(conn)
{
    PTRACE(6, "Media\tConstructor OpalIAX2MediaStream" << mediaFormat);
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
 
 
BOOL OpalIAX2MediaStream::ReadPacket(RTP_DataFrame & packet)
{
  PTRACE(6, "Media\tRead media comppressed audio packet from the iax2 connection");

  if (IsSink()) {
    PTRACE(1, "Media\tTried to read from sink media stream");
    return FALSE;
  }

  if (!isOpen) {
    PTRACE(3, "Media\tStream has been closed, so exit now");
    return FALSE;
  }
    
  BOOL success = connection.ReadSoundPacket(timestamp, packet); 

  return success;
}


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

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

