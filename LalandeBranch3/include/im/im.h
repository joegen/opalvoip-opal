/*
 * im_mf.h
 *
 * Media formats for Instant Messaging
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

#ifndef OPAL_IM_IM_H
#define OPAL_IM_IM_H

#include <ptlib.h>
#include <opal/buildopts.h>

#if OPAL_HAS_IM

#include <opal/mediastrm.h>

#include <im/rfc4103.h>

class OpalIMMediaType : public OpalMediaTypeDefinition 
{
  public:
    OpalIMMediaType(
      const char * mediaType,          ///< name of the media type (audio, video etc)
      const char * sdpType,            ///< name of the SDP type 
      unsigned     preferredSessionId  ///< preferred session ID
    )
      : OpalMediaTypeDefinition(mediaType, sdpType, preferredSessionId, OpalMediaType::DontOffer)
    { }

    PString GetRTPEncoding() const { return PString::Empty(); }
    RTP_UDP * CreateRTPSession(OpalRTPConnection & , unsigned , bool ) { return NULL; }
    virtual bool UsesRTP() const { return false; }
};


class OpalIMMediaStream : public OpalMediaStream
{
  public:
    OpalIMMediaStream(
      OpalConnection & conn,
      const OpalMediaFormat & mediaFormat, ///<  Media format for stream
      unsigned sessionID,                  ///<  Session number for stream
      bool isSource                        ///<  Is a source stream
    );

    virtual PBoolean IsSynchronous() const         { return false; }
    virtual PBoolean RequiresPatchThread() const   { return false; }

    /*
     * called to send IM to remote connection
     */
    virtual bool PushIM(const T140String & text);
    virtual bool PushIM(RTP_DataFrame & frame);

  protected:
    RFC4103Context rfc4103;
};


#endif // OPAL_HAS_IM

#endif // OPAL_IM_IM_H
