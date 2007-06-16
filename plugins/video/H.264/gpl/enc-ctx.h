/*
 * H.264 Plugin codec for OpenH323/OPAL
 *
 * Copyright (C) Matthias Schneider, All Rights Reserved
 *
 * This code is based on the file h261codec.cxx from the OPAL project released
 * under the MPL 1.0 license which contains the following:
 *
 * Copyright (c) 1998-2000 Equivalence Pty. Ltd.
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
 * Contributor(s): Matthias Schneider (ma30002000@yahoo.de)
 *                 Michele Piccini (michele@piccini.com)
 *                 Derek Smithies (derek@indranet.co.nz)
 *
 *
 */

/*
  Notes
  -----

 */

#ifndef __ENC_CTX_H__
#define __ENC_CTX_H__ 1

#include <stdarg.h>
#include <stdint.h>
#include "../shared/h264frame.h"
//#include "critsect.h"

extern "C" {
  #include <x264.h>
};

#define CIF_WIDTH 352
#define CIF_HEIGHT 288
#define QCIF_WIDTH 176
#define QCIF_HEIGHT 144
#define IT_QCIF 0
#define IT_CIF 1

#define H264_BITRATE         768000
#define H264_PAYLOAD_SIZE      1400
#define H264_FRAME_RATE          25
#define H264_KEY_FRAME_INTERVAL 2.0
#define H264_TRACELEVEL 4

typedef unsigned char u_char;

static void logCallbackX264   (void *priv, int level, const char *fmt, va_list arg);

class X264EncoderContext 
{
  public:
    X264EncoderContext ();
    ~X264EncoderContext ();

    void SetTargetBitRate (int rate);
    void SetEncodingQuality (int quality);
    void SetFrameWidth (int width);
    void SetFrameHeight (int height);
    void SetPayloadType (int payloadType);
    void ApplyOptions ();

    int EncodeFrames (const u_char * src, unsigned & srcLen, u_char * dst, unsigned & dstLen, unsigned int & flags);

  protected:
//    CriticalSection _mutex;

    x264_t* _codec;
    x264_param_t _context;
    x264_picture_t _inputFrame;
    H264Frame* _txH264Frame;

    uint32_t _PFramesSinceLastIFrame; // counts frames since last keyframe
    uint32_t _IFrameInterval; // confd frames between keyframes
    int _frameCounter;
    int _payloadType;
} ;



#endif /* __H264-X264_H__ */
