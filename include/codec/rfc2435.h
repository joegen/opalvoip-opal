/*
 * rfc2435.h
 *
 * RFC2435 JPEG transport for uncompressed video
 *
 * Open Phone Abstraction Library
 *
 * Copyright (C) 2010 Post Increment
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
 * $Revision: 24184 $
 * $Author: csoutheren $
 * $Date: 2010-04-05 23:50:26 -0700 (Mon, 05 Apr 2010) $
 */

#ifndef OPAL_CODEC_RFC2435_H
#define OPAL_CODEC_RFC2435_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <ptlib.h>

#include <opal/buildopts.h>

#if OPAL_RFC2435

#include <ptclib/random.h>

#include <opal/transcoders.h>
#include <codec/opalplugin.h>
#include <codec/vidcodec.h>


#define OPAL_RFC2435_YCbCr420  "RFC2435"
extern const OpalVideoFormat & GetOpalRFC2435();
#define OpalRFC2435            GetOpalRFC2435();

/////////////////////////////////////////////////////////////////////////////

class OpalRFC2435Transcoder : public OpalVideoTranscoder
{
  PCLASSINFO(OpalRFC2435Transcoder, OpalVideoTranscoder);
  public:
    OpalRFC2435Transcoder(      
      const OpalMediaFormat & inputMediaFormat,  ///<  Input media format
      const OpalMediaFormat & outputMediaFormat  ///<  Output media format
    );
};

/////////////////////////////////////////////////////////////////////////////

class OpalRFC2435Encoder : public OpalRFC2435Transcoder
{
  PCLASSINFO(OpalRFC2435Encoder, OpalRFC2435Transcoder);
  public:
    OpalRFC2435Encoder(      
      const OpalMediaFormat & inputMediaFormat,  ///<  Input media format
      const OpalMediaFormat & outputMediaFormat  ///<  Output media format
    );

    bool ConvertFrames(const RTP_DataFrame & input, RTP_DataFrameList & output);
};

/////////////////////////////////////////////////////////////////////////////

class OpalRFC2435Decoder : public OpalRFC2435Transcoder
{
  PCLASSINFO(OpalRFC2435Decoder, OpalRFC2435Transcoder);
  public:
    OpalRFC2435Decoder(      
      const OpalMediaFormat & inputMediaFormat,  ///<  Input media format
      const OpalMediaFormat & outputMediaFormat  ///<  Output media format
    );
    ~OpalRFC2435Decoder();

    bool ConvertFrames(const RTP_DataFrame & input, RTP_DataFrameList & output);
};

/////////////////////////////////////////////////////////////////////////////


#define OPAL_REGISTER_RFC2435() \
  OPAL_REGISTER_TRANSCODER(Opal_RFC2435_to_YUV420P, OpalRFC2435, OpalYUV420P); \
  OPAL_REGISTER_TRANSCODER(Opal_YUV420P_to_RFC2435, OpalYUV420P, OpalRFC2435);


/////////////////////////////////////////////////////////////////////////////

#endif // OPAL_RFC2435

#endif // OPAL_CODEC_RFC2435_H
