/*7
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

#include "enc-ctx.h"
#include "../shared/trace.h"
#include "../shared/rtpframe.h"

#include <stdlib.h>
#ifdef _WIN32
  #include <malloc.h>
  #define STRCMPI  _strcmpi
#else
  #include <semaphore.h>
  #define STRCMPI  strcasecmp
#endif
#include <string.h>

static void logCallbackX264 (void *priv, int level, const char *fmt, va_list arg) {
  char buffer[512];
  int severity = 0;
  switch (level) {
    case X264_LOG_NONE:    severity = 1; break;
    case X264_LOG_ERROR:   severity = 1; break;
    case X264_LOG_WARNING: severity = 3; break;
    case X264_LOG_INFO:    severity = 4; break;
    case X264_LOG_DEBUG:   severity = 4; break;
  }
  sprintf(buffer, "H264\tx264\t"); 
  vsprintf(buffer + strlen (buffer), fmt, arg);
  buffer[strlen(buffer) - 1] = 0;
  TRACE(severity, buffer);
}

X264EncoderContext::X264EncoderContext()
{
//  if (!X264LibraryInstance.IsLoaded()) return;

  _frameCounter=0;
  _PFramesSinceLastIFrame = 0;
  _IFrameInterval = _context.i_keyint_max = (int)(H264_FRAME_RATE * H264_KEY_FRAME_INTERVAL);
  _PFramesSinceLastIFrame = _IFrameInterval + 1; // force a keyframe on the first frame

  _txH264Frame = new H264Frame();
  _txH264Frame->SetMaxPayloadSize(H264_PAYLOAD_SIZE);
  _payloadType = 99;

  _inputFrame.i_type = X264_TYPE_AUTO;
  _inputFrame.i_qpplus1 = 0;
  _inputFrame.img.i_csp = X264_CSP_I420;
 
 // X264LibraryInstance.X264ParamDefault(&_context);
   x264_param_default(&_context);

  // We make use of the baseline profile, that means:
  // no B-Frames (too much latency in interactive video)
  // CBR (we want to get the max. quality making use of all the bitrate that is available)
  // We allow one exeption: configuring a bitrate of > 786 kbit/s
  // baseline profile begin
  _context.b_cabac = 0;
  _context.i_bframe = 0;
  _context.rc.i_vbv_max_bitrate = (int)(H264_BITRATE/1024);
  _context.rc.i_vbv_buffer_size = 2000;
  _context.i_level_idc = 13;
  _context.rc.i_bitrate = (int)(H264_BITRATE/1024);
  // baseline profile end

  _context.i_width = CIF_WIDTH;
  _context.i_height = CIF_HEIGHT;
  _context.vui.i_sar_width = 0;
  _context.vui.i_sar_height = 0;
  _context.i_fps_num = (int)((H264_FRAME_RATE + .5) * 1000);
  _context.i_fps_den = 1000;
  //_context.i_maxframes = 0;

  // ABR with bit rate tolerance = 1 is CBR...
  _context.rc.i_rc_method = X264_RC_ABR;
  _context.rc.f_rate_tolerance = 1;

  //_context.rc.b_stat_write = 0;
  //_context.analyse.inter = 0;
  _context.analyse.b_psnr = 0;

  // enable logging
  _context.pf_log = logCallbackX264;
  _context.i_log_level = X264_LOG_DEBUG;
  _context.p_log_private = NULL;

  // auto detect number of CPUs
  _context.i_threads = 0;  

  _codec = x264_encoder_open(&_context);

  if (_codec == NULL) {
    TRACE(1, "H264\tEncoder\tCouldn't init x264 encoder");
  } 
  else
  {
    TRACE(4, "H264\tEncoder\tx264 encoder successfully opened");
  }
}

X264EncoderContext::~X264EncoderContext()
{

    if (_codec != NULL)
    {
      x264_encoder_close(_codec);
      TRACE(4, "H264\tEncoder\tClosed H.264 encoder, encoded " << _frameCounter << " Frames" );
    }
  if (_txH264Frame) delete _txH264Frame;
}

void X264EncoderContext::SetTargetBitRate(int rate)
{
  _context.rc.i_vbv_max_bitrate = rate;
  _context.rc.i_bitrate = rate;
}

void X264EncoderContext::SetEncodingQuality(int quality)
{
// Quality is from 1 (worst) to 25 (best)
}

void X264EncoderContext::SetFrameWidth(int width)
{
  _context.i_width = width;
}

void X264EncoderContext::SetFrameHeight(int height)
{
  _context.i_height = height;
}

void X264EncoderContext::SetPayloadType(int payloadType)
{
  _payloadType = payloadType;
}

void X264EncoderContext::ApplyOptions()
{
  x264_encoder_close(_codec);
  _codec = x264_encoder_open(&_context);
  if (_codec == NULL) {
    TRACE(1, "H264\tEncoder\tCouldn't init x264 encoder");
  } 
  else
  {
    TRACE(4, "H264\tEncoder\tx264 encoder successfully opened");
  }
}

int X264EncoderContext::EncodeFrames(const u_char * src, unsigned & srcLen, u_char * dst, unsigned & dstLen, unsigned int & flags)
{
//  if (!x64LibraryInstance.IsLoaded()) return 0;

//  WaitAndSignal m(_mutex);

  // create RTP frame from source buffer
  RTPFrame srcRTP(src, srcLen);

  // create RTP frame from destination buffer
  RTPFrame dstRTP(dst, dstLen, _payloadType);
  dstLen = 0;

  // from here, we are encoding a new frame
  if ((!_codec) || (!_txH264Frame))
  {
    TRACE(1, "H264\tEncoder\tEncoder not opened ");
    return 0;
  }

  // if there are RTP packets to return, return them
  if  (_txH264Frame->HasRTPFrames())
  {
    _txH264Frame->GetRTPFrame(dstRTP, flags);
    dstLen = dstRTP.GetFrameLen();
    return 1;
  }

  if (srcRTP.GetPayloadSize() < sizeof(frameHeader))
  {
   TRACE(1, "H264\tEncoder\tVideo grab too small, Close down video transmission thread");
   return 0;
  }

  frameHeader * header = (frameHeader *)srcRTP.GetPayloadPtr();
  if (header->x != 0 || header->y != 0)
  {
    TRACE(1, "H264\tEncoder\tVideo grab of partial frame unsupported, Close down video transmission thread");
    return 0;
  }

  // do a validation of size
  // if the incoming data has changed size, tell the encoder
  if (_context.i_width != header->width || _context.i_height != header->height)
  {
    x264_encoder_close(_codec);
    _context.i_width = header->width;
    _context.i_height = header->height;
    _codec = x264_encoder_open(&_context);
  } 

  bool wantIFrame = false;
  x264_nal_t* NALs;
  int numberOfNALs = 0;
  x264_picture_t dummyOutput;

  // Check whether to insert a keyframe 
  // (On the first frame and every_IFrameInterval)
  _PFramesSinceLastIFrame++;
  if (_PFramesSinceLastIFrame >= _IFrameInterval)
  {
    wantIFrame = true;
    _PFramesSinceLastIFrame = 0;
  }
  // Prepare the frame to be encoded
  _inputFrame.img.plane[0] = (uint8_t *)(((unsigned char *)header) + sizeof(frameHeader));
  _inputFrame.img.i_stride[0] = header->width;
  _inputFrame.img.plane[1] = (uint8_t *)((((unsigned char *)header) + sizeof(frameHeader)) 
                           + (int)(_inputFrame.img.i_stride[0]*header->height));
  _inputFrame.img.i_stride[1] = 
  _inputFrame.img.i_stride[2] = (int) ( header->width / 2 );
  _inputFrame.img.plane[2] = (uint8_t *)(_inputFrame.img.plane[1] + (int)(_inputFrame.img.i_stride[1] *header->height/2));
  _inputFrame.i_type = wantIFrame ? X264_TYPE_IDR : X264_TYPE_AUTO;

  while (numberOfNALs==0) { // workaround for first 2 packets being 0
    if (x264_encoder_encode(_codec, &NALs, &numberOfNALs, &_inputFrame, &dummyOutput) < 0) {
      TRACE (1,"H264\tEncoder\tEncoding failed");
      return 0;
    } 
  }
  
  _txH264Frame->BeginNewFrame();
  _txH264Frame->SetFromFrame(NALs, numberOfNALs);
  _txH264Frame->SetTimestamp(srcRTP.GetTimestamp());
  _frameCounter++; 

  if (_txH264Frame->HasRTPFrames())
  {
    _txH264Frame->GetRTPFrame(dstRTP, flags);
    dstLen = dstRTP.GetFrameLen();
    return 1;
  }
  return 1;
}
