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

#include "h264-x264.h"

#ifdef WIN32
#include "h264pipe_win32.h"
#else
#include "h264pipe_unix.h"
#endif

#ifdef _MSC_VER
 #include "../common/dyna.h"
 #include "../common/trace.h"
 #include "shared/rtpframe.h"
#else
 #include "dyna.h"
 #include "trace.h"
 #include "rtpframe.h"
#endif


#include <stdlib.h>
#ifdef _WIN32
  #include <malloc.h>
  #define STRCMPI  _strcmpi
#else
  #include <semaphore.h>
  #define STRCMPI  strcasecmp
#endif
#include <string.h>

FFMPEGLibrary FFMPEGLibraryInstance(CODEC_ID_H264);
H264EncCtx H264EncCtxInstance;

static void logCallbackFFMPEG (void* v, int level, const char* fmt , va_list arg) {
  char buffer[512];
  int severity = 0;
  if (v) {
    switch (level)
    {
      case AV_LOG_QUIET: severity = 0; break;
      case AV_LOG_ERROR: severity = 1; break;
      case AV_LOG_INFO:  severity = 4; break;
      case AV_LOG_DEBUG: severity = 4; break;
    }
    sprintf(buffer, "H264\tFFMPEG\t");
    vsprintf(buffer + strlen(buffer), fmt, arg);
    TRACE (severity, buffer);
  }
}

H264EncoderContext::H264EncoderContext()
{
  H264EncCtxInstance.call(H264ENCODERCONTEXT_CREATE);
}

H264EncoderContext::~H264EncoderContext()
{
  H264EncCtxInstance.call(H264ENCODERCONTEXT_DELETE);
}

void H264EncoderContext::ApplyOptions()
{
  H264EncCtxInstance.call(APPLY_OPTIONS);
}

void H264EncoderContext::SetMaxRTPFrameSize(int size)
{
  H264EncCtxInstance.call(SET_MAX_FRAME_SIZE, size);
}

void H264EncoderContext::SetTargetBitRate(int rate)
{
  H264EncCtxInstance.call(SET_TARGET_BITRATE, rate);
}

void H264EncoderContext::SetFrameRate(int rate)
{
  H264EncCtxInstance.call(SET_FRAME_RATE, rate);
}

void H264EncoderContext::SetFrameWidth(int width)
{
  H264EncCtxInstance.call(SET_FRAME_WIDTH, width);
}

void H264EncoderContext::SetFrameHeight(int height)
{
  H264EncCtxInstance.call(SET_FRAME_HEIGHT, height);
}

int H264EncoderContext::EncodeFrames(const u_char * src, unsigned & srcLen, u_char * dst, unsigned & dstLen, unsigned int & flags)
{
  WaitAndSignal m(_mutex);

  int ret;
  unsigned int headerLen;

  RTPFrame dstRTP(dst, dstLen);
  headerLen = dstRTP.GetHeaderSize();

  H264EncCtxInstance.call(ENCODE_FRAMES, src, srcLen, dst, dstLen, headerLen, flags, ret);

  return ret;
}

H264DecoderContext::H264DecoderContext()
{
  if (!FFMPEGLibraryInstance.IsLoaded()) return;

  _gotIFrame =0;
  _frameCounter=0; 
  _skippedFrameCounter=0;
  _rxH264Frame = new H264Frame();

  FFMPEGLibraryInstance.AvLogSetLevel(AV_LOG_DEBUG);
  FFMPEGLibraryInstance.AvLogSetCallback(&logCallbackFFMPEG);

  if ((_codec = FFMPEGLibraryInstance.AvcodecFindDecoder(CODEC_ID_H264)) == NULL) {
    TRACE(1, "H264\tDecoder\tCodec not found for decoder");
    return;
  }

  _context = FFMPEGLibraryInstance.AvcodecAllocContext();
  if (_context == NULL) {
    TRACE(1, "H264\tDecoder\tFailed to allocate context for decoder");
    return;
  }

  _outputFrame = FFMPEGLibraryInstance.AvcodecAllocFrame();
  if (_outputFrame == NULL) {
    TRACE(1, "H264\tDecoder\tFailed to allocate frame for encoder");
    return;
  }

  if (FFMPEGLibraryInstance.AvcodecOpen(_context, _codec) < 0) {
    TRACE(1, "H264\tDecoder\tFailed to open H.264 decoder");
    return;
  }
  else
  {
    TRACE(1, "H264\tDecoder\tDecoder successfully opened");
  }
}

H264DecoderContext::~H264DecoderContext()
{
  if (FFMPEGLibraryInstance.IsLoaded())
  {
    if (_context != NULL)
    {
      if (_context->codec != NULL)
      {
        FFMPEGLibraryInstance.AvcodecClose(_context);
        TRACE(4, "H264\tDecoder\tClosed H.264 decoder, decoded " << _frameCounter << " Frames, skipped " << _skippedFrameCounter << " Frames" );
      }
    }

    FFMPEGLibraryInstance.AvcodecFree(_context);
    FFMPEGLibraryInstance.AvcodecFree(_outputFrame);
  }
  if (_rxH264Frame) delete _rxH264Frame;
}

int H264DecoderContext::DecodeFrames(const u_char * src, unsigned & srcLen, u_char * dst, unsigned & dstLen, unsigned int & flags)
{
  if (!FFMPEGLibraryInstance.IsLoaded()) return 0;

  // create RTP frame from source buffer
  RTPFrame srcRTP(src, srcLen);

  // create RTP frame from destination buffer
  RTPFrame dstRTP(dst, dstLen, 0);
  dstLen = 0;
  flags = 0;

  _rxH264Frame->SetFromRTPFrame(srcRTP, flags);
  if (srcRTP.GetMarker()==0)
  {
     return 1;
  } 

  if (_rxH264Frame->GetFrameSize()==0)
  {
    _rxH264Frame->BeginNewFrame();
    TRACE(4, "H264\tDecoder\tGot an empty frame - skipping");
    _skippedFrameCounter++;
    return 0;
  }

  TRACE(4, "H264\tDecoder\tDecoding " << _rxH264Frame->GetFrameSize()  << " bytes");

  // look and see if we have read an I frame.
  if (_gotIFrame == 0)
  {
    if (!_rxH264Frame->IsSync())
    {
      TRACE(1, "H264\tDecoder\tWaiting for an I-Frame");
      _rxH264Frame->BeginNewFrame();
      return 0;
    }
    _gotIFrame = 1;
  }

  int gotPicture = 0;
  uint32_t bytesUsed = 0;
  int bytesDecoded = FFMPEGLibraryInstance.AvcodecDecodeVideo(_context, _outputFrame, &gotPicture, _rxH264Frame->GetFramePtr() + bytesUsed, _rxH264Frame->GetFrameSize() - bytesUsed);

  _rxH264Frame->BeginNewFrame();
  if (!gotPicture) 
  {
    TRACE(1, "H264\tDecoder\tDecoded "<< bytesDecoded << " bytes without getting a Picture..."); 
    _skippedFrameCounter++;
    return 0;
  }

  TRACE(4, "H264\tDecoder\tDecoded " << bytesDecoded << " bytes"<< ", Resolution: " << _context->width << "x" << _context->height);
  int frameBytes = (_context->width * _context->height * 3) / 2;
  PluginCodec_Video_FrameHeader * header = (PluginCodec_Video_FrameHeader *)dstRTP.GetPayloadPtr();
  header->x = header->y = 0;
  header->width = _context->width;
  header->height = _context->height;

  int size = _context->width * _context->height;
  if (_outputFrame->data[1] == _outputFrame->data[0] + size
      && _outputFrame->data[2] == _outputFrame->data[1] + (size >> 2))
  {
    memcpy(OPAL_VIDEO_FRAME_DATA_PTR(header), _outputFrame->data[0], frameBytes);
  }
  else 
  {
    unsigned char *dst = OPAL_VIDEO_FRAME_DATA_PTR(header);
    for (int i=0; i<3; i ++)
    {
      unsigned char *src = _outputFrame->data[i];
      int dst_stride = i ? _context->width >> 1 : _context->width;
      int src_stride = _outputFrame->linesize[i];
      int h = i ? _context->height >> 1 : _context->height;

      if (src_stride==dst_stride)
      {
        memcpy(dst, src, dst_stride*h);
        dst += dst_stride*h;
      }
      else
      {
        while (h--)
        {
          memcpy(dst, src, dst_stride);
          dst += dst_stride;
          src += src_stride;
        }
      }
    }
  }

  dstRTP.SetPayloadSize(sizeof(PluginCodec_Video_FrameHeader) + frameBytes);
  dstRTP.SetTimestamp(srcRTP.GetTimestamp());
  dstRTP.SetMarker(1);
  dstLen = dstRTP.GetFrameLen();

  flags = PluginCodec_ReturnCoderLastFrame;
  _frameCounter++;
  return 1;
}

/////////////////////////////////////////////////////////////////////////////

static void * create_encoder(const struct PluginCodec_Definition * /*codec*/)
{
  return new H264EncoderContext;
}

static int encoder_set_options(
      const struct PluginCodec_Definition *, 
      void * _context, 
      const char *, 
      void * parm, 
      unsigned * parmLen)
{
  H264EncoderContext * context = (H264EncoderContext *)_context;

  if (parmLen == NULL || *parmLen != sizeof(const char **)) return 0;

  if (parm != NULL) {
    const char ** options = (const char **)parm;
    int i;
    for (i = 0; options[i] != NULL; i += 2) {
      if (STRCMPI(options[i], "Target Bit Rate") == 0)
         context->SetTargetBitRate((int) (atoi(options[i+1]) / 1024) );
      if (STRCMPI(options[i], "Frame Time") == 0)
         context->SetFrameRate((int)(H264_CLOCKRATE / atoi(options[i+1])));
      if (STRCMPI(options[i], "Frame Height") == 0)
         context->SetFrameHeight(atoi(options[i+1]));
      if (STRCMPI(options[i], "Frame Width") == 0)
         context->SetFrameWidth(atoi(options[i+1]));
      if (STRCMPI(options[i], "Max Frame Size") == 0)
         context->SetMaxRTPFrameSize(atoi(options[i+1]));
      TRACE (4, "H264\tEncoder\tOption " << options[i] << " = " << atoi(options[i+1]));
    }
    context->ApplyOptions();

  }
  return 1;
}

static void destroy_encoder(const struct PluginCodec_Definition * /*codec*/, void * _context)
{
  H264EncoderContext * context = (H264EncoderContext *)_context;
  delete context;
}

static int codec_encoder(const struct PluginCodec_Definition * ,
                                           void * _context,
                                     const void * from,
                                       unsigned * fromLen,
                                           void * to,
                                       unsigned * toLen,
                                   unsigned int * flag)
{
  H264EncoderContext * context = (H264EncoderContext *)_context;
  return context->EncodeFrames((const u_char *)from, *fromLen, (u_char *)to, *toLen, *flag);
}

/////////////////////////////////////////////////////////////////////////////


static void * create_decoder(const struct PluginCodec_Definition *)
{
  return new H264DecoderContext;
}

static int get_codec_options(const struct PluginCodec_Definition * codec,
                                                  void *,
                                                  const char *,
                                                  void * parm,
                                                  unsigned * parmLen)
{
    if (parmLen == NULL || parm == NULL || *parmLen != sizeof(struct PluginCodec_Option **))
        return 0;

    *(const void **)parm = codec->userData;
    return 1;
}

static void destroy_decoder(const struct PluginCodec_Definition * /*codec*/, void * _context)
{
  H264DecoderContext * context = (H264DecoderContext *)_context;
  delete context;
}

static int codec_decoder(const struct PluginCodec_Definition *, 
                                           void * _context,
                                     const void * from, 
                                       unsigned * fromLen,
                                           void * to,
                                       unsigned * toLen,
                                   unsigned int * flag)
{
  H264DecoderContext * context = (H264DecoderContext *)_context;
  return context->DecodeFrames((const u_char *)from, *fromLen, (u_char *)to, *toLen, *flag);
}

static int decoder_get_output_data_size(const PluginCodec_Definition * codec, void *, const char *, void *, unsigned *)
{
  return sizeof(PluginCodec_Video_FrameHeader) + ((codec->parm.video.maxFrameWidth * codec->parm.video.maxFrameHeight * 3) / 2);
}

/////////////////////////////////////////////////////////////////////////////

extern "C" {

PLUGIN_CODEC_IMPLEMENT(H264)

PLUGIN_CODEC_DLL_API struct PluginCodec_Definition * PLUGIN_CODEC_GET_CODEC_FN(unsigned * count, unsigned version)
{
  char * debug_level = getenv ("PWLIB_TRACE_CODECS");
  if (debug_level!=NULL) {
    Trace::SetLevel(atoi(debug_level));
  } 
  else {
    Trace::SetLevel(0);
  }

  if (!FFMPEGLibraryInstance.Load()) {
    *count = 0;
    return NULL;
  }

  if (!H264EncCtxInstance.isLoaded()) {
    if (!H264EncCtxInstance.Load()) {
      *count = 0;
      return NULL;
    }
  }

  if (version < PLUGIN_CODEC_VERSION_VIDEO) {
    *count = 0;
    return NULL;
  }
  else {
    *count = sizeof(h264CodecDefn) / sizeof(struct PluginCodec_Definition);
    return h264CodecDefn;
  }
  
}
};
