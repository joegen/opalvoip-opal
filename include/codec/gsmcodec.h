/*
 * gsmcodec.h
 *
 * H.323 protocol handler
 *
 * Open H323 Library
 *
 * Copyright (c) 1999-2001 Equivalence Pty. Ltd.
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
 * The Original Code is Open H323 Library.
 *
 * The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * Portions of this code were written with the assisance of funding from
 * Vovida Networks, Inc. http://www.vovida.com.
 *
 * Contributor(s): ______________________________________.
 *
 * $Log: gsmcodec.h,v $
 * Revision 1.2002  2001/08/01 05:03:09  robertj
 * Changes to allow control of linking software transcoders, use macros
 *   to force linking.
 * Allowed codecs to be used without H.,323 being linked by using the
 *   new NO_H323 define.
 * Major changes to H.323 capabilities, uses OpalMediaFormat for base name.
 *
 * Revision 2.0  2001/07/27 15:48:24  robertj
 * Conversion of OpenH323 to Open Phone Abstraction Library (OPAL)
 *
 * Revision 1.10  2001/02/11 22:48:30  robertj
 * Added #pragma interface for GNU C++.
 *
 * Revision 1.9  2001/01/25 07:27:14  robertj
 * Major changes to add more flexible OpalMediaFormat class to normalise
 *   all information about media types, especially codecs.
 *
 * Revision 1.8  2000/10/13 03:43:14  robertj
 * Added clamping to avoid ever setting incorrect tx frame count.
 *
 * Revision 1.7  2000/05/10 04:05:26  robertj
 * Changed capabilities so has a function to get name of codec, instead of relying on PrintOn.
 *
 * Revision 1.6  2000/05/02 04:32:24  robertj
 * Fixed copyright notice comment.
 *
 * Revision 1.5  1999/12/31 00:05:36  robertj
 * Added Microsoft ACM G.723.1 codec capability.
 *
 * Revision 1.4  1999/12/23 23:02:34  robertj
 * File reorganision for separating RTP from H.323 and creation of LID for VPB support.
 *
 * Revision 1.3  1999/10/08 09:59:01  robertj
 * Rewrite of capability for sending multiple audio frames
 *
 * Revision 1.2  1999/10/08 04:58:37  robertj
 * Added capability for sending multiple audio frames in single RTP packet
 *
 * Revision 1.1  1999/09/08 04:05:48  robertj
 * Added support for video capabilities & codec, still needs the actual codec itself!
 *
 */

#ifndef __CODEC_GSMCODEC_H
#define __CODEC_GSMCODEC_H

#ifdef __GNUC__
#pragma interface
#endif


#include <opal/transcoders.h>


///////////////////////////////////////////////////////////////////////////////

struct gsm_state;

class Opal_GSM0610 : public OpalFramedTranscoder {
  public:
    Opal_GSM0610(
      const OpalTranscoderRegistration & registration, /// Registration for transcoder
      unsigned inputBytesPerFrame,  /// Number of bytes in an input frame
      unsigned outputBytesPerFrame  /// Number of bytes in an output frame
    );
    ~Opal_GSM0610();
  protected:
    gsm_state * gsm;
};


///////////////////////////////////////////////////////////////////////////////

class Opal_GSM0610_PCM : public Opal_GSM0610 {
  public:
    Opal_GSM0610_PCM(
      const OpalTranscoderRegistration & registration /// Registration for transcoder
    );
    virtual BOOL ConvertFrame(const BYTE * src, BYTE * dst);
};


///////////////////////////////////////////////////////////////////////////////

class Opal_PCM_GSM0610 : public Opal_GSM0610 {
  public:
    Opal_PCM_GSM0610(
      const OpalTranscoderRegistration & registration /// Registration for transcoder
    );
    virtual BOOL ConvertFrame(const BYTE * src, BYTE * dst);
};


///////////////////////////////////////////////////////////////////////////////

#define OPAL_REGISTER_GSM0610() \
          OPAL_REGISTER_TRANSCODER(Opal_GSM0610_PCM, OPAL_GSM0610, OPAL_PCM16); \
          OPAL_REGISTER_TRANSCODER(Opal_PCM_GSM0610, OPAL_PCM16, OPAL_GSM0610)



#endif // __CODEC_GSMCODEC_H


/////////////////////////////////////////////////////////////////////////////
