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
 * Revision 1.9  2007/03/29 05:16:49  csoutheren
 * Pass OpalConnection to OpalMediaSream constructor
 * Add ID to OpalMediaStreams so that transcoders can match incoming and outgoing codecs
 *
 * Revision 1.8  2007/01/11 03:02:16  dereksmithies
 * Remove the previous audio buffering code, and switch to using the jitter
 * buffer provided in Opal. Reduce the verbosity of the log mesasges.
 *
 * Revision 1.7  2007/01/10 09:16:55  csoutheren
 * Allow compilation with video disabled
 *
 * Revision 1.6  2006/09/11 03:08:50  dereksmithies
 * Add fixes from Stephen Cook (sitiveni@gmail.com) for new patches to
 * improve call handling. Notably, IAX2 call transfer. Many thanks.
 * Thanks also to the Google summer of code for sponsoring this work.
 *
 * Revision 1.5  2006/08/09 03:46:39  dereksmithies
 * Add ability to register to a remote Asterisk box. The iaxProcessor class is split
 * into a callProcessor and a regProcessor class.
 * Big thanks to Stephen Cook, (sitiveni@gmail.com) for this work.
 *
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
				                                             PBoolean isSource)
  : OpalMediaStream(conn, mediaFormat, sessionID, isSource),
    connection(conn)
{
    PTRACE(6, "Media\tConstructor OpalIAX2MediaStream" << mediaFormat);
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
  PTRACE(2, "Media\tOpalMediaStream is a " << PString(IsSink() ? "sink" : "source"));

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
  PTRACE(6, "Media\tRead media comppressed audio packet from the iax2 connection");

  if (IsSink()) {
    PTRACE(1, "Media\tTried to read from sink media stream");
    return PFalse;
  }

  if (!isOpen) {
    PTRACE(3, "Media\tStream has been closed, so exit now");
    return PFalse;
  }
    
  PBoolean success = connection.ReadSoundPacket(timestamp, packet); 

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
PBoolean OpalIAX2MediaStream::WriteData(const BYTE * buffer, PINDEX length, PINDEX & written)
{
  written = 0;
  if (IsSource()) {
    PTRACE(1, "Media\tTried to write to source media stream");
    return PFalse;
  }
  PTRACE(6, "Media\tSend data to the network : have " << length << " bytes to send to remote host");
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


/////////////////////////////////////////////////////////////////////////////

