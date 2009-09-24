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
 * $Revision$
 * $Author$
 * $Date$
 */

#include <ptlib.h>
#include <opal/buildopts.h>

#if OPAL_IAX2

#ifdef __GNUC__
#pragma implementation "iax2medstrm.h"
#endif

#include <iax2/iax2medstrm.h>

#include <iax2/frame.h>
#include <iax2/iax2con.h>
#include <lids/lid.h>
#include <opal/mediastrm.h>
#include <opal/patch.h>
#include <ptlib/videoio.h>
#include <rtp/rtp.h>
#include <codec/vidcodec.h>


#define MAX_PAYLOAD_TYPE_MISMATCHES 10


#define new PNEW


///////////////////////////////////////////////////////////////////////////////

OpalIAX2MediaStream::OpalIAX2MediaStream(IAX2Connection & conn, 
                                  const OpalMediaFormat & mediaFormat,
					 unsigned sessionID,   
					 PBoolean isSource)
  : OpalMediaStream(conn, mediaFormat, sessionID, isSource),
    connection(conn)
{
  PTRACE(6, "Media\tConstructor OpalIAX2MediaStream" << mediaFormat);
    connection.SafeReference();
}
 
OpalIAX2MediaStream::~OpalIAX2MediaStream()
{
  PTRACE(6, "Media\tDestructor OpalIAX2MediaStream");
  connection.SafeDereference();
}
 
PBoolean OpalIAX2MediaStream::Open()
{
  if (isOpen)
    return PTrue;

  PBoolean res = OpalMediaStream::Open();
  PTRACE(3, "Media\tOpalIAX2MediaStream set to " << mediaFormat << " is now open");
 
  return res;
}
 
PBoolean OpalIAX2MediaStream::Start()
{
  PTRACE(2, "Media\tOpalIAX2MediaStream is a " << PString(IsSink() ? "sink" : "source"));

  return OpalMediaStream::Start();
}
  
PBoolean OpalIAX2MediaStream::Close()
{
  PBoolean res = OpalMediaStream::Close();

  PTRACE(3, "Media\tOpalIAX2MediaStream of " << mediaFormat << " is now closed"); 
  return res;
}
 
 
PBoolean OpalIAX2MediaStream::ReadPacket(RTP_DataFrame & packet)
{
  PTRACE(3, "Media\tRead media comppressed audio packet from the iax2 connection");

  if (IsSink()) {
    PTRACE(1, "Media\tTried to read from sink media stream");
    return PFalse;
  }

  if (!isOpen) {
    PTRACE(3, "Media\tStream has been closed, so exit now");
    return PFalse;
  }
    
  PBoolean success = connection.ReadSoundPacket(packet); 

  return success;
}


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

// This routine takes data from the source (eg mic) and sends the data to the remote host.
PBoolean OpalIAX2MediaStream::WriteData(const BYTE * buffer, PINDEX length, PINDEX & written)
{
  written = 0;
  if (IsSource()) {
    PTRACE(1, "Media\tTried to write to source media stream");
    return PFalse;
  }
  PTRACE(3, "Media\tSend data to the network : have " 
	 << length << " bytes to send to remote host");
  PBYTEArray *sound = new PBYTEArray(buffer, length);
  written = length;
  connection.PutSoundPacketToNetwork(sound);

  return PTrue;
}

PBoolean OpalIAX2MediaStream::IsSynchronous() const
{
  if (IsSource())
    return PTrue;

  /**We are reading from a sound card, which generates a frame at a regular rate*/
  return PTrue;
}


#endif // OPAL_IAX2

/////////////////////////////////////////////////////////////////////////////

