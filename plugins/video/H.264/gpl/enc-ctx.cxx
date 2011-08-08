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

#include "plugin-config.h"

#include "enc-ctx.h"
#include "shared/rtpframe.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#if defined(_WIN32) || defined(_WIN32_WCE)
  #include <malloc.h>
  #define STRCMPI  _strcmpi
#else
  #include <semaphore.h>
  #define STRCMPI  strcasecmp
#endif
#include <string.h>

#if PTRACING
  static void logCallbackX264 (void * /*priv*/, int level, const char *fmt, va_list arg) {
    int severity = 4;
    switch (level) {
      case X264_LOG_NONE:    severity = 1; break;
      case X264_LOG_ERROR:   severity = 1; break;
      case X264_LOG_WARNING: severity = 3; break;
      case X264_LOG_INFO:    severity = 4; break;
      case X264_LOG_DEBUG:   severity = 4; break;
    }

    char buffer[512];
    vsnprintf(buffer, sizeof(buffer), fmt, arg);
    PTRACE(severity, "x264", buffer);
  }
#endif



X264EncoderContext::X264EncoderContext()
{
  // Default
  x264_param_default_preset(&m_context, "veryfast", "fastdecode,zerolatency");

  m_context.b_annexb = false;  // Do not do markers, we are RTP so don't need them

  // ABR with bit rate tolerance = 1 is CBR...
  m_context.rc.i_rc_method = X264_RC_ABR;
  m_context.rc.f_rate_tolerance = 1;

  // No aspect ratio correction
  m_context.vui.i_sar_width = 0;
  m_context.vui.i_sar_height = 0;

#if PTRACING
  // Enable logging
  m_context.pf_log = logCallbackX264;
  m_context.i_log_level = X264_LOG_DEBUG;
  m_context.p_log_private = NULL;
#endif

  // Auto detect number of CPUs
  m_context.i_threads = 0;  

  SetFrameWidth       (CIF_WIDTH);
  SetFrameHeight      (CIF_HEIGHT);
  SetFrameRate        (H264_FRAME_RATE);
  SetTargetBitrate    ((unsigned)(H264_BITRATE / 1000));
  SetProfileLevel     (H264_PROFILE_LEVEL);
  SetTSTO             (H264_TSTO);
  SetMaxKeyFramePeriod(H264_KEY_FRAME_INTERVAL);

  m_codec = x264_encoder_open(&m_context);
  
  if (m_codec == NULL) {
    PTRACE(1, "x264", "Couldn't open encoder");
  } 
  else
  {
    PTRACE(4, "x264", "Encoder successfully opened");
  }
}

X264EncoderContext::~X264EncoderContext()
{
  if (m_codec != NULL)
  {
    x264_encoder_close(m_codec);
    PTRACE(5, "x264", "Closed H.264 encoder");
  }
}

void X264EncoderContext::SetMaxRTPFrameSize(unsigned size)
{
  m_context.i_slice_max_size = size;
  m_encapsulation.SetMaxPayloadSize(size);
}

void X264EncoderContext::SetMaxKeyFramePeriod (unsigned period)
{
  m_context.i_keyint_max = period;
}

void X264EncoderContext::SetTargetBitrate(unsigned rate)
{
  m_context.rc.i_vbv_max_bitrate = m_context.rc.i_bitrate = rate;
}

void X264EncoderContext::SetFrameRate(unsigned rate)
{
  m_context.i_fps_num = (int)((rate + .5) * 1000);
  m_context.i_fps_den = 1000;
}

void X264EncoderContext::SetTSTO (unsigned tsto)
{
  m_context.rc.i_qp_min = H264_MIN_QUANT;
  m_context.rc.i_qp_max = round ( (double)(51 - H264_MIN_QUANT) / 31 * tsto + H264_MIN_QUANT);
  m_context.rc.i_qp_step = 4;	    
}

void X264EncoderContext::SetProfileLevel (unsigned profileLevel)
{
  const static struct h264_level {
      unsigned level_idc;
      unsigned mbps;        /* max macroblock processing rate (macroblocks/sec) */
      unsigned frame_size;  /* max frame size (macroblocks) */
      unsigned dpb;         /* max decoded picture buffer (bytes) */
      long unsigned bitrate;/* max bitrate (kbit/sec) */
      unsigned cpb;         /* max vbv buffer (kbit) */
      unsigned mv_range;    /* max vertical mv component range (pixels) */
      unsigned mvs_per_2mb; /* max mvs per 2 consecutive mbs. */
      unsigned slice_rate;
      unsigned bipred8x8;   /* limit bipred to >=8x8 */
      unsigned direct8x8;   /* limit b_direct to >=8x8 */
      unsigned frame_only;  /* forbid interlacing */
  } h264_levels[] = {
      { 10,   1485,    99,   152064,     64000,    175,  64, 64,  0, 0, 0, 1 },
      {  9,   1485,    99,   152064,    128000,    350,  64, 64,  0, 0, 0, 1 },
      { 11,   3000,   396,   345600,    192000,    500, 128, 64,  0, 0, 0, 1 },
      { 12,   6000,   396,   912384,    384000,   1000, 128, 64,  0, 0, 0, 1 },
      { 13,  11880,   396,   912384,    768000,   2000, 128, 64,  0, 0, 0, 1 },
      { 20,  11880,   396,   912384,   2000000,   2000, 128, 64,  0, 0, 0, 1 },
      { 21,  19800,   792,  1824768,   4000000,   4000, 256, 64,  0, 0, 0, 0 },
      { 22,  20250,  1620,  3110400,   4000000,   4000, 256, 64,  0, 0, 0, 0 },
      { 30,  40500,  1620,  3110400,  10000000,  10000, 256, 32, 22, 0, 1, 0 },
      { 31, 108000,  3600,  6912000,  14000000,  14000, 512, 16, 60, 1, 1, 0 },
      { 32, 216000,  5120,  7864320,  20000000,  20000, 512, 16, 60, 1, 1, 0 },
      { 40, 245760,  8192, 12582912,  20000000,  25000, 512, 16, 60, 1, 1, 0 },
      { 41, 245760,  8192, 12582912,  50000000,  62500, 512, 16, 24, 1, 1, 0 },
      { 42, 522240,  8704, 13369344,  50000000,  62500, 512, 16, 24, 1, 1, 1 },
      { 50, 589824, 22080, 42393600, 135000000, 135000, 512, 16, 24, 1, 1, 1 },
      { 51, 983040, 36864, 70778880, 240000000, 240000, 512, 16, 24, 1, 1, 1 },
      { 0 }
  };

//  unsigned profile = (profileLevel & 0xff0000) >> 16;
//  bool constraint0 = (profileLevel & 0x008000) ? true : false;
//  bool constraint1 = (profileLevel & 0x004000) ? true : false;
//  bool constraint2 = (profileLevel & 0x002000) ? true : false;
//  bool constraint3 = (profileLevel & 0x001000) ? true : false;
  unsigned level   = (profileLevel & 0x0000ff);

  int i = 0;
  while (h264_levels[i].level_idc) {
    if (h264_levels[i].level_idc == level)
      break;
   i++; 
  }

  if (!h264_levels[i].level_idc) {
    PTRACE(1, "x264", "Illegal Level negotiated");
    return;
  }

  // We make use of the baseline profile, that means:
  // no B-Frames (too much latency in interactive video)
  // CBR (we want to get the max. quality making use of all the bitrate that is available)
  // baseline profile begin
  m_context.b_cabac = 0;  // Only >= MAIN LEVEL
  m_context.i_bframe = 0; // Only >= MAIN LEVEL

  // Level:
  m_context.i_level_idc = level;
  
  // DPB from Level by default
  m_context.rc.i_vbv_buffer_size = h264_levels[i].cpb;
  // MV Range from Level by default  
}

void X264EncoderContext::ApplyOptions()
{
  if (m_codec != NULL)
    x264_encoder_close(m_codec);

  m_codec = x264_encoder_open(&m_context);
  if (m_codec == NULL) {
    PTRACE(1, "x264", "Couldn't re-open encoder");
  } 
  else
  {
    PTRACE(5, "x264", "Encoder successfully re-opened");
  }
}

int X264EncoderContext::EncodeFrames(const unsigned char * src, unsigned & srcLen, unsigned char * dst, unsigned & dstLen, unsigned int & flags)
{
  if (m_codec == NULL) {
    PTRACE(1, "x264", "Encoder not open");
    return 0;
  }

  // if there are NALU's encoded, return them
  if (!m_encapsulation.HasRTPFrames()) {
    // create RTP frame from source buffer
    RTPFrame srcRTP(src, srcLen);

    // do a validation of size
    size_t payloadSize = srcRTP.GetPayloadSize();
    if (payloadSize < sizeof(frameHeader)) {
      PTRACE(1, "x264", "Video grab far too small, Close down video transmission thread");
      return 0;
    }

    frameHeader * header = (frameHeader *)srcRTP.GetPayloadPtr();
    if (header->x != 0 || header->y != 0) {
      PTRACE(1, "x264", "Video grab of partial frame unsupported, Close down video transmission thread");
      return 0;
    }

    if (payloadSize < sizeof(frameHeader)+header->width*header->height*3/2) {
      PTRACE(1, "x264", "Video grab far too small, Close down video transmission thread");
      return 0;
    }

    // if the incoming data has changed size, tell the encoder
    if ((unsigned)m_context.i_width != header->width || (unsigned)m_context.i_height != header->height) {
      PTRACE(4, "x264", "Detected resolution change " << m_context.i_width << 'x' << m_context.i_height
                                                    << " to " << header->width << 'x' << header->height);
      x264_encoder_close(m_codec);
      m_context.i_width = header->width;
      m_context.i_height = header->height;
      m_codec = x264_encoder_open(&m_context);
      if (m_codec == NULL) {
        PTRACE(1, "x264", "Couldn't re-open encoder");
        return 0;
      }
      PTRACE(4, "x264", "Encoder successfully re-opened");
    } 

    // Prepare the frame to be encoded
    x264_picture_t inputPicture;
    x264_picture_init(&inputPicture);
    inputPicture.i_qpplus1 = 0;
    inputPicture.img.i_csp = X264_CSP_I420;
    inputPicture.img.i_stride[0] = header->width;
    inputPicture.img.i_stride[1] = inputPicture.img.i_stride[2] = header->width/2;
    inputPicture.img.plane[0] = (uint8_t *)(((unsigned char *)header) + sizeof(frameHeader));
    inputPicture.img.plane[1] = inputPicture.img.plane[0] + header->width*header->height;
    inputPicture.img.plane[2] = inputPicture.img.plane[1] + header->width*header->height/4;
    inputPicture.i_type = flags != 0 ? X264_TYPE_IDR : X264_TYPE_AUTO;

    x264_nal_t *NALs;
    int numberOfNALs = 0;
    while (numberOfNALs == 0) { // workaround for first 2 packets being 0
      x264_picture_t outputPicture;
      if (x264_encoder_encode(m_codec, &NALs, &numberOfNALs, &inputPicture, &outputPicture) < 0) {
        PTRACE(1, "x264", "x264_encoder_encode failed");
        return 0;
      }
    }

    m_encapsulation.BeginNewFrame();
    m_encapsulation.SetFromFrame(NALs, numberOfNALs);
    m_encapsulation.SetTimestamp(srcRTP.GetTimestamp());
  }

  // create RTP frame from destination buffer
  RTPFrame dstRTP(dst, dstLen);
  m_encapsulation.GetRTPFrame(dstRTP, flags);
  dstLen = dstRTP.GetFrameLen();
  return 1;
}
