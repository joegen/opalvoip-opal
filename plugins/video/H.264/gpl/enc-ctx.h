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
#ifdef _MSC_VER
  #include "../shared/vs-stdint.h"
#else
  #include <stdint.h>
#endif

extern "C" {
#ifdef _MSC_VER
  #include "../x264/x264.h"
#else
  #include <x264.h>
#endif
};


#include "../shared/h264frame.h"


#if PTRACING
  #include <iostream>
  #define PTRACE_CHECK(level) ((level) < 4)
  #define PTRACE(level, section, expr) std::cerr << section << '\t' << expr << std::endl
#else
  #define PTRACE_CHECK(level) true
  #define PTRACE(level, section, expr)
#endif


#define CIF_WIDTH 352
#define CIF_HEIGHT 288
#define QCIF_WIDTH 176
#define QCIF_HEIGHT 144
#define IT_QCIF 0
#define IT_CIF 1

#define H264_BITRATE         768000
#define H264_PAYLOAD_SIZE      1400
#define H264_FRAME_RATE          25
#define H264_KEY_FRAME_INTERVAL 125
#define H264_PROFILE_LEVEL       ((66 << 16) + (0xC0 << 8) +  30)
#define H264_TSTO                31
#define H264_MIN_QUANT           10


class X264EncoderContext 
{
  public:
    X264EncoderContext();
    ~X264EncoderContext();

    int EncodeFrames(const unsigned char * src, unsigned & srcLen, unsigned char * dst, unsigned & dstLen, unsigned int & flags);

    void SetFrameWidth(unsigned width) { m_context.i_width = width; }
    unsigned GetFrameWidth() const { return m_context.i_width; }

    void SetFrameHeight(unsigned height) { m_context.i_height = height; }
    unsigned GetFrameHeight() const { return m_context.i_height; }

    void SetMaxRTPFrameSize(unsigned size);
    void SetMaxKeyFramePeriod(unsigned period);
    void SetTargetBitrate(unsigned rate);
    void SetFrameRate(unsigned rate);
    void SetTSTO(unsigned tsto);
    void SetProfileLevel(unsigned profileLevel);
    void ApplyOptions();

  protected:
    x264_param_t   m_context;
    x264_t       * m_codec;
    H264Frame      m_encapsulation;
};


#endif /* __ENC_CTX_H__ */
