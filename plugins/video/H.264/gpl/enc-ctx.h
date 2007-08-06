/*
 * H.264 Plugin codec for OpenH323/OPAL
 *
 * Copyright (C) 2007 Matthias Schneider, All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#ifndef __ENC_CTX_H__
#define __ENC_CTX_H__ 1

#include <stdarg.h>
#include <stdint.h>
#include "../shared/h264frame.h"

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

static void logCallbackX264   (void *priv, int level, const char *fmt, va_list arg);

class X264EncoderContext 
{
  public:
    X264EncoderContext ();
    ~X264EncoderContext ();

    void SetTargetBitRate (int rate);
    void SetFrameRate (int rate);
    void SetFrameWidth (int width);
    void SetFrameHeight (int height);
    void ApplyOptions ();

    int EncodeFrames (const unsigned char * src, unsigned & srcLen, unsigned char * dst, unsigned & dstLen, unsigned int & flags);

  protected:

    x264_t* _codec;
    x264_param_t _context;
    x264_picture_t _inputFrame;
    H264Frame* _txH264Frame;

    uint32_t _PFramesSinceLastIFrame; // counts frames since last keyframe
    uint32_t _IFrameInterval; // confd frames between keyframes
    int _frameCounter;
} ;

#endif /* __H264-X264_H__ */
