/*
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
 * $Revision$
 * $Author$
 * $Date$
 */

#ifndef OPAL_CODEC_RFC2435_H
#define OPAL_CODEC_RFC2435_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <ptlib.h>

#include <opal_config.h>

#if OPAL_RFC2435

#include <ptclib/random.h>

#include <opal/transcoders.h>
#include <codec/opalplugin.h>
#include <codec/vidcodec.h>

#include <jpeglib.h>

#define OPAL_RFC2435_JPEG      "RFC2435_JPEG"
extern const OpalVideoFormat & GetOpalRFC2435_JPEG();
#define OpalRFC2435_JPEG       GetOpalRFC2435_JPEG()


/////////////////////////////////////////////////////////////////////////////

class OpalRFC2435Encoder : public OpalVideoTranscoder
{
  PCLASSINFO(OpalRFC2435Encoder, OpalVideoTranscoder);
  public:
    OpalRFC2435Encoder();
    bool ConvertFrames(const RTP_DataFrame & input, RTP_DataFrameList & output);

  public:
    struct jpeg_compress_struct m_jpegCompressor;
};

/////////////////////////////////////////////////////////////////////////////

class OpalRFC2435Decoder : public OpalVideoTranscoder
{
  PCLASSINFO(OpalRFC2435Decoder, OpalVideoTranscoder);
  public:
    OpalRFC2435Decoder();

    bool ConvertFrames(const RTP_DataFrame & input, RTP_DataFrameList & output);
};

/////////////////////////////////////////////////////////////////////////////

#define OPAL_REGISTER_RFC2435_JPEG() \
  OPAL_REGISTER_TRANSCODER(OpalRFC2435Decoder, OpalRFC2435_JPEG, OpalYUV420P); \
  OPAL_REGISTER_TRANSCODER(OpalRFC2435Encoder, OpalYUV420P, OpalRFC2435_JPEG);

/////////////////////////////////////////////////////////////////////////////

#endif // OPAL_RFC2435

#endif // OPAL_CODEC_RFC2435_H
