/*
 * rfc4103.h
 *
 * Implementation of RFC 4103 RTP Payload for Text Conversation
 *
 * Open Phone Abstraction Library (OPAL)
 *
 * Copyright (c) 2008 Post Increment
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
 * The Initial Developer of the Original Code is Post Increment
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#ifndef OPAL_IM_RFC4103_H
#define OPAL_IM_RFC4103_H

#include <opal_config.h>

#if OPAL_HAS_RFC4103

#include <ptclib/url.h>

#include <opal/mediastrm.h>
#include <rtp/rtp.h>


class T140String;


/** Media stream for carrying RFC 4103 (T.140) instant message over RTP
  */
class OpalT140MediaStream : public OpalMediaStream
{
  public:
    OpalT140MediaStream(
      OpalConnection & conn,
      const OpalMediaFormat & mediaFormat, ///<  Media format for stream
      unsigned sessionID,                  ///<  Session number for stream
      bool isSource                        ///<  Is a source stream
    );

    virtual PBoolean IsSynchronous() const         { return false; }
    virtual PBoolean RequiresPatchThread() const   { return false; }

    bool ReadPacket(RTP_DataFrame & packet);
    bool WritePacket(RTP_DataFrame & packet);

  protected:
    virtual void InternalClose() { }
};


#endif // OPAL_HAS_RFC4103

#endif // OPAL_IM_RFC4103_H
