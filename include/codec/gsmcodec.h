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
 * Revision 1.2006  2002/11/10 11:33:16  robertj
 * Updated to OpenH323 v1.10.3
 *
 * Revision 2.4  2002/09/16 02:52:33  robertj
 * Added #define so can select if #pragma interface/implementation is used on
 *   platform basis (eg MacOS) rather than compiler, thanks Robert Monaghan.
 *
 * Revision 2.3  2002/09/04 06:01:46  robertj
 * Updated to OpenH323 v1.9.6
 *
 * Revision 2.2  2001/11/02 10:45:19  robertj
 * Updated to OpenH323 v1.7.3
 *
 * Revision 2.1  2001/08/01 05:03:09  robertj
 * Changes to allow control of linking software transcoders, use macros
 *   to force linking.
 * Allowed codecs to be used without H.,323 being linked by using the
 *   new NO_H323 define.
 * Major changes to H.323 capabilities, uses OpalMediaFormat for base name.
 *
 * Revision 2.0  2001/07/27 15:48:24  robertj
 * Conversion of OpenH323 to Open Phone Abstraction Library (OPAL)
 *
 * Revision 1.14  2002/09/16 01:14:15  robertj
 * Added #define so can select if #pragma interface/implementation is used on
 *   platform basis (eg MacOS) rather than compiler, thanks Robert Monaghan.
 *
 * Revision 1.13  2002/09/03 06:19:36  robertj
 * Normalised the multi-include header prevention ifdef/define symbol.
 *
 * Revision 1.12  2002/08/05 10:03:47  robertj
 * Cosmetic changes to normalise the usage of pragma interface/implementation.
 *
 * Revision 1.11  2001/10/24 01:20:34  robertj
 * Added code to help with static linking of H323Capability names database.
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

#ifndef __OPAL_GSMCODEC_H
#define __OPAL_GSMCODEC_H

#ifdef P_USE_PRAGMA
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



#endif // __OPAL_GSMCODEC_H


/////////////////////////////////////////////////////////////////////////////
