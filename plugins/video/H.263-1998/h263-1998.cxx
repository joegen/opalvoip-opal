/*
 * H.263 Plugin codec for OpenH323/OPAL
 *
 * This code is based on the following files from the OPAL project which
 * have been removed from the current build and distributions but are still
 * available in the CVS "attic"
 * 
 *    src/codecs/h263codec.cxx 
 *    include/codecs/h263codec.h 

 * The original files, and this version of the original code, are released under the same 
 * MPL 1.0 license. Substantial portions of the original code were contributed
 * by Salyens and March Networks and their right to be identified as copyright holders
 * of the original code portions and any parts now included in this new copy is asserted through 
 * their inclusion in the copyright notices below.
 *
 * Copyright (C) 2007 Matthias Schneider
 * Copyright (C) 2006 Post Increment
 * Copyright (C) 2005 Salyens
 * Copyright (C) 2001 March Networks Corporation
 * Copyright (C) 1999-2000 Equivalence Pty. Ltd.
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
 *                 Guilhem Tardy (gtardy@salyens.com)
 *                 Craig Southeren (craigs@postincrement.com)
 *
 */

/*
  Notes
  -----

 */


#define _CRT_SECURE_NO_DEPRECATE
#include "h263-1998.h"
#include "trace.h"
#include "dyna.h"


extern "C" {
#include "ffmpeg/avcodec.h"
};

static FFMPEGLibrary FFMPEGLibraryInstance(CODEC_ID_H263P);

/////////////////////////////////////////////////////////////////////////////

H263PEncoderContext::H263PEncoderContext() 
{ 
  if (!FFMPEGLibraryInstance.IsLoaded())
    return;

  if ((_codec = FFMPEGLibraryInstance.AvcodecFindEncoder(CODEC_ID_H263P)) == NULL) {
    TRACE(1, "H263+\tEncoder\tCodec not found for encoder");
    return;
  }

  _frameWidth  = CIF_WIDTH;
  _frameHeight = CIF_HEIGHT;
  _bitRate = 256000;

  _context = FFMPEGLibraryInstance.AvcodecAllocContext();
  if (_context == NULL) {
    TRACE(1, "H263+\tEncoder\tFailed to allocate context for encoder");
    return;
  }

  _inputFrame = FFMPEGLibraryInstance.AvcodecAllocFrame();
  if (_inputFrame == NULL) {
    TRACE(1, "H263+\tEncoder\tFailed to allocate frame for encoder");
    return;
  }

  _context->codec = NULL;

  // set some reasonable values for quality as default
  _videoQuality = 10; 
  _videoQMin = 4;
  _videoQMax = 24;

  _frameCount = 0;

  _txH263PFrame = new H263PFrame(MAX_YUV420P_FRAME_SIZE);
  _txH263PFrame->SetMaxPayloadSize(H263P_PAYLOAD_SIZE);

  TRACE(3, "Codec\tEncoder\tH263 encoder created");
}

H263PEncoderContext::~H263PEncoderContext()
{
  if (_txH263PFrame)
    delete _txH263PFrame;

  if (FFMPEGLibraryInstance.IsLoaded()) {
    CloseCodec();

    FFMPEGLibraryInstance.AvcodecFree(_context);
    FFMPEGLibraryInstance.AvcodecFree(_inputFrame);
  }
}

bool H263PEncoderContext::OpenCodec()
{
  if (_codec == NULL) {
    TRACE(1, "H263+\tEncoder\tCodec not initialized");
    return false;
  }

  _context->width  = _frameWidth;
  _context->height = _frameHeight;

  _inputFrame->linesize[0] = _frameWidth;
  _inputFrame->linesize[1] = _frameWidth / 2;
  _inputFrame->linesize[2] = _frameWidth / 2;
  _inputFrame->quality = _videoQuality;

  _context->bit_rate = (_bitRate * 3) >> 2; // average bit rate
  _context->bit_rate_tolerance = _bitRate << 3;
  _context->rc_min_rate = 0; // minimum bitrate
  _context->rc_max_rate = _bitRate; // maximum bitrate
  _context->rc_qsquish = 0; // limit q by clipping
  _context->rc_eq= "tex^qComp"; // rate control equation
  _context->rc_buffer_size = _bitRate * 64;

  _context->mb_qmin = _context->qmin = _videoQMin;
  _context->mb_qmax = _context->qmax = _videoQMax;
  _context->max_qdiff = 3; // max q difference between frames
  _context->qcompress = 0.5; // qscale factor between easy & hard scenes (0.0-1.0)
  _context->i_quant_factor = (float)-0.6; // qscale factor between p and i frames
  _context->i_quant_offset = (float)0.0; // qscale offset between p and i frames

  _context->mb_decision = FF_MB_DECISION_SIMPLE; // choose only one MB type at a time
  _context->me_method = ME_EPZS;
  _context->me_subpel_quality = 8;

  _context->time_base.num = 100; // X-Lite does not like Custom Picture frequency clocks...
  _context->time_base.den = 2997;
  _context->gop_size = (int)(H263P_FRAME_RATE * H263P_KEY_FRAME_INTERVAL);

  _context->max_b_frames = 0;
  _context->pix_fmt = PIX_FMT_YUV420P;
  _context->rtp_mode = 1;
   if ((H263P_PAYLOAD_SIZE * 6 / 7) > 0)  
    _context->rtp_payload_size = (H263P_PAYLOAD_SIZE * 6 / 7);
    else  
    _context->rtp_payload_size = H263P_PAYLOAD_SIZE;

  // avoid copying input/output
  _context->flags |= CODEC_FLAG_INPUT_PRESERVED; // we guarantee to preserve input for max_b_frames+1 frames
  _context->flags |= CODEC_FLAG_EMU_EDGE; // don't draw edges
  _context->flags |= CODEC_FLAG_PASS1;

  // possibly the next? , is compliant to level 1
  _context->flags &= ~CODEC_FLAG_AC_PRED; // advanced intra coding signaled via annex I, level3+ eyeBeam
  _context->flags &= ~CODEC_FLAG_H263P_SLICE_STRUCT;  // has to be signaled via annex K eyeBeam   // does not work with eyeBeam
  _context->flags &= ~CODEC_FLAG_OBMC; // advanced prediction mode signaled via annex F // does not work with eyeBeam
  _context->flags &= ~CODEC_FLAG_LOOP_FILTER;  // Annex J: deblocking filter    eyeBeam

  //not signaled via capability
  _context->flags &= ~CODEC_FLAG_H263P_AIV;    // Annex S - Alternative INTER VLC mode // does not work with eyeBeam
  _context->flags &= ~CODEC_FLAG_H263P_UMV; // Annex D: Unrestructed Motion Vectors level2+ works with eyeBeam signal via D??

// "F", "I", "K", "J"
//      "T", "N", and "P".
  return FFMPEGLibraryInstance.AvcodecOpen(_context, _codec) == 0;
}

void H263PEncoderContext::CloseCodec()
{
  if (_context != NULL) {
    if (_context->codec != NULL) {
      FFMPEGLibraryInstance.AvcodecClose(_context);
      TRACE(3, "H263+\tEncoder\tClosed H.263 encoder" );
    }
  }
}

int H263PEncoderContext::EncodeFrames(const BYTE * src, unsigned & srcLen, BYTE * dst, unsigned & dstLen, unsigned int & flags)
{
  if (!FFMPEGLibraryInstance.IsLoaded())
    return 0;

  if (_codec == NULL) {
    TRACE(1, "H263+\tEncoder\tCodec not initialized");
    return 0;
  }

  // create RTP frame from source buffer
  RTPFrame srcRTP(src, srcLen);

  // create RTP frame from destination buffer
  RTPFrame dstRTP(dst, dstLen);
  dstLen = 0;
  flags = 0;

  // if there are RTP packets to return, return them
  if  (_txH263PFrame->HasRTPFrames())
  {
    _txH263PFrame->GetRTPFrame(dstRTP, flags);
    dstLen = dstRTP.GetFrameLen();
    return 1;
  }

  if (srcRTP.GetPayloadSize() < sizeof(PluginCodec_Video_FrameHeader)) {
    TRACE(1,"H263+\tEncoder\tVideo grab too small, closing down video transmission thread.");
    return 0;
  }

  PluginCodec_Video_FrameHeader * header = (PluginCodec_Video_FrameHeader *)srcRTP.GetPayloadPtr();
  if (header->x != 0 || header->y != 0) {
    TRACE(1, "H263+\tEncoder\tVideo grab of partial frame unsupported, closing down video transmission thread.");
    return false;
  }

  // if this is the first frame, or the frame size has changed, deal wth it
  if (_frameCount == 0 || 
      _frameWidth != header->width || 
      _frameHeight != header->height) {

    _frameWidth  = header->width;
    _frameHeight = header->height;

 
    CloseCodec();
    if (!OpenCodec()) {
      TRACE(1,  "H263+\tEncoder\tReopening codec failed");
      return false;
    }
  }

  int size = _frameWidth * _frameHeight;
  int frameSize = (size * 3) >> 1;
 
  // we need FF_INPUT_BUFFER_PADDING_SIZE allocated bytes after the YVU420P image for the encoder
  memcpy (_inputFrameBuffer, OPAL_VIDEO_FRAME_DATA_PTR(header), frameSize);
  memset (_inputFrameBuffer + frameSize, 0 , FF_INPUT_BUFFER_PADDING_SIZE);

  _inputFrame->data[0] = _inputFrameBuffer;
  _inputFrame->data[1] = _inputFrame->data[0] + size;
  _inputFrame->data[2] = _inputFrame->data[1] + (size / 4);

  _txH263PFrame->BeginNewFrame();
  _txH263PFrame->SetTimestamp(srcRTP.GetTimestamp());
  _txH263PFrame->SetFrameSize (FFMPEGLibraryInstance.AvcodecEncodeVideo(_context, _txH263PFrame->GetFramePtr(), frameSize, _inputFrame));
  _frameCount++; 

  if (_txH263PFrame->GetFrameSize() == 0) {
    TRACE(1, "H263+\tEncoder internal error - there should be outstanding packets at this point");
    return 1;
  }

  TRACE(4, "H263+\tEncoded " << frameSize << " bytes of YUV420P raw data into " << _txH263PFrame->GetFrameSize() << " bytes");

  if (_txH263PFrame->HasRTPFrames())
  {
    _txH263PFrame->GetRTPFrame(dstRTP, flags);
    dstLen = dstRTP.GetFrameLen();
    return true;
  }
  return 1;
}

void H263PEncoderContext::SetMaxRTPFrameSize (int size)
{
  _txH263PFrame->SetMaxPayloadSize(size);
}


static void * create_encoder(const struct PluginCodec_Definition * /*codec*/)
{
  return new H263PEncoderContext;
}

static int encoder_set_options(const PluginCodec_Definition *, 
                               void * _context,
                               const char * , 
                               void * parm, 
                               unsigned * parmLen)
{
  H263PEncoderContext * context = (H263PEncoderContext *)_context;
  if (parmLen == NULL || *parmLen != sizeof(const char **) || parm == NULL)
    return 0;

  // get the "frame width" media format parameter to use as a hint for the encoder to start off
  for (const char * const * option = (const char * const *)parm; *option != NULL; option += 2) {
    if (STRCMPI(option[0], "Frame Width") == 0)
      context->_frameWidth = atoi(option[1]);
    if (STRCMPI(option[0], "Frame Height") == 0)
      context->_frameHeight = atoi(option[1]);
    if (STRCMPI(option[0], "Max Frame Size") == 0)
       context->SetMaxRTPFrameSize (atoi(option[1]));
    TRACE (4, "H263+\tEncoder\tOption " << option[0] << " = " << option[1]);
  }
  return 1;
}

static void destroy_encoder(const struct PluginCodec_Definition * /*codec*/, void * _context)
{
  H263PEncoderContext * context = (H263PEncoderContext *)_context;
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
  H263PEncoderContext * context = (H263PEncoderContext *)_context;
  return context->EncodeFrames((const BYTE *)from, *fromLen, (BYTE *)to, *toLen, *flag);
}


/////////////////////////////////////////////////////////////////////////////


H263PDecoderContext::H263PDecoderContext()
{
  if (!FFMPEGLibraryInstance.IsLoaded())
    return;

  if ((_codec = FFMPEGLibraryInstance.AvcodecFindDecoder(CODEC_ID_H263)) == NULL) {
    TRACE(1, "H263+\tDecoder\tCodec not found for decoder");
    return;
  }

  _context = FFMPEGLibraryInstance.AvcodecAllocContext();
  if (_context == NULL) {
    TRACE(1, "H263+\tDecoder\tFailed to allocate context for decoder");
    return;
  }

  _outputFrame = FFMPEGLibraryInstance.AvcodecAllocFrame();
  if (_outputFrame == NULL) {
    TRACE(1, "H263+\tDecoder\tFailed to allocate frame for decoder");
    return;
  }

  if (!OpenCodec()) { // decoder will re-initialise context with correct frame size
    TRACE(1, "H263+\tDecoder\tFailed to open codec for decoder");
    return;
  }

  _frameCount = 0;
  _rxH263PFrame = new H263PFrame(MAX_YUV420P_FRAME_SIZE);
  _skippedFrameCounter = 0;
  _gotIFrame = false;

  TRACE(4,  "H263+\tDecoder\tH263 decoder created");
}

H263PDecoderContext::~H263PDecoderContext()
{
  if (_rxH263PFrame) delete _rxH263PFrame;

  if (FFMPEGLibraryInstance.IsLoaded()) {
    CloseCodec();

    FFMPEGLibraryInstance.AvcodecFree(_context);
    FFMPEGLibraryInstance.AvcodecFree(_outputFrame);
  }
}

bool H263PDecoderContext::OpenCodec()
{
  if (_codec == NULL) {
    TRACE(1, "H263+\tDecoder\tCodec not initialized");
    return 0;
  }

  // avoid copying input/output
  _context->flags |= CODEC_FLAG_INPUT_PRESERVED; // we guarantee to preserve input for max_b_frames+1 frames
  _context->flags |= CODEC_FLAG_EMU_EDGE; // don't draw edges

  _context->workaround_bugs = 0; // no workaround for buggy H.263 implementations
  _context->error_concealment = FF_EC_GUESS_MVS | FF_EC_DEBLOCK;

  if (FFMPEGLibraryInstance.AvcodecOpen(_context, _codec) < 0) {
    TRACE(1, "H263+\tDecoder\tFailed to open H.263 decoder");
    return false;
  }

  return true;
}

void H263PDecoderContext::CloseCodec()
{
  if (_context != NULL) {
    if (_context->codec != NULL) {
      FFMPEGLibraryInstance.AvcodecClose(_context);
      TRACE(4, "H263+\tDecoder\tClosed H.263 decoder" );
    }
  }
}

bool H263PDecoderContext::DecodeFrames(const BYTE * src, unsigned & srcLen, BYTE * dst, unsigned & dstLen, unsigned int & flags)
{
  if (!FFMPEGLibraryInstance.IsLoaded())
    return 0;

  // create RTP frame from source buffer
  RTPFrame srcRTP(src, srcLen);

  // create RTP frame from destination buffer
  RTPFrame dstRTP(dst, dstLen, 0);
  dstLen = 0;
  flags = 0;

  _rxH263PFrame->SetFromRTPFrame(srcRTP, flags);
  if (srcRTP.GetMarker()==0)
  {
     return 1;
  } 

  if (_rxH263PFrame->GetFrameSize()==0)
  {
    _rxH263PFrame->BeginNewFrame();
    TRACE(4, "H263+\tDecoder\tGot an empty frame - skipping");
    _skippedFrameCounter++;
    return 0;
  }

  if (!_rxH263PFrame->hasPicHeader()) {
    TRACE(1, "H263+\tDecoder\tReceived frame has no picture header - dropping");
    _rxH263PFrame->BeginNewFrame();
    return 0;
  }

  // look and see if we have read an I frame.
  if (!_gotIFrame)
  {
    if (!_rxH263PFrame->isIFrame())
    {
      TRACE(1, "H263+\tDecoder\tWating for an I-Frame");
      _rxH263PFrame->BeginNewFrame();
      return 0;
    }
    _gotIFrame = true;
  }

  int gotPicture = 0;

  TRACE(4, "H263+\tDecoder\tDecoding " << _rxH263PFrame->GetFrameSize()  << " bytes");
  int bytesDecoded = FFMPEGLibraryInstance.AvcodecDecodeVideo(_context, _outputFrame, &gotPicture, _rxH263PFrame->GetFramePtr(), _rxH263PFrame->GetFrameSize());

  _rxH263PFrame->BeginNewFrame();

  if (!gotPicture) 
  {
    TRACE(1, "H263+\tDecoder\tDecoded "<< bytesDecoded << " bytes without getting a Picture..."); 
    _skippedFrameCounter++;
    return 0;
  }

  TRACE(4, "H263+\tDecoder\tDecoded " << bytesDecoded << " bytes"<< ", Resolution: " << _context->width << "x" << _context->height);

  // if error occurred, tell the other end to send another I-frame and hopefully we can resync
  if (bytesDecoded < 0) {
    TRACE(1, "H263+\tDecoder\tDecoded 0 bytes, requesting I frame");
    flags = PluginCodec_ReturnCoderRequestIFrame;
    return 1;
  }

  // if decoded frame size is not legal, request an I-Frame
  if (_context->width == 0 || _context->height == 0) {
    TRACE(1, "H263+\tDecoder\tReceived frame with invalid size, requesting I frame");
    flags = PluginCodec_ReturnCoderRequestIFrame;
    return 1;
  }

  int frameBytes = (_context->width * _context->height * 12) / 8;
  PluginCodec_Video_FrameHeader * header = (PluginCodec_Video_FrameHeader *)dstRTP.GetPayloadPtr();
  header->x = header->y = 0;
  header->width = _context->width;
  header->height = _context->height;
  int size = _context->width * _context->height;
  if (_outputFrame->data[1] == _outputFrame->data[0] + size
      && _outputFrame->data[2] == _outputFrame->data[1] + (size >> 2)) {
    memcpy(OPAL_VIDEO_FRAME_DATA_PTR(header), _outputFrame->data[0], frameBytes);
  } else {
    unsigned char *dst = OPAL_VIDEO_FRAME_DATA_PTR(header);
    for (int i=0; i<3; i ++) {
      unsigned char *src = _outputFrame->data[i];
      int dst_stride = i ? _context->width >> 1 : _context->width;
      int src_stride = _outputFrame->linesize[i];
      int h = i ? _context->height >> 1 : _context->height;

      if (src_stride==dst_stride) {
        memcpy(dst, src, dst_stride*h);
        dst += dst_stride*h;
      } else {
        while (h--) {
          memcpy(dst, src, dst_stride);
          dst += dst_stride;
          src += src_stride;
        }
      }
    }
  }

  dstRTP.SetPayloadSize(sizeof(PluginCodec_Video_FrameHeader) + frameBytes);
  dstRTP.SetTimestamp(srcRTP.GetTimestamp());
  dstRTP.SetMarker(true);

  dstLen = dstRTP.GetFrameLen();

  flags = PluginCodec_ReturnCoderLastFrame ;   // TODO: THIS NEEDS TO BE CHANGED TO DO CORRECT IFRAME DETECTION

  _frameCount++;

  return 1;
}


static void * create_decoder(const struct PluginCodec_Definition *)
{
  return new H263PDecoderContext;
}

static void destroy_decoder(const struct PluginCodec_Definition * /*codec*/, void * _context)
{
  H263PDecoderContext * context = (H263PDecoderContext *)_context;
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
  H263PDecoderContext * context = (H263PDecoderContext *)_context;
  return context->DecodeFrames((const BYTE *)from, *fromLen, (BYTE *)to, *toLen, *flag);
}

static int decoder_get_output_data_size(const PluginCodec_Definition * codec, void *, const char *, void *, unsigned *)
{
  return sizeof(PluginCodec_Video_FrameHeader) + ((codec->parm.video.maxFrameWidth * codec->parm.video.maxFrameHeight * 3) / 2);
}


static int get_codec_options(const struct PluginCodec_Definition * codec,
                             void *, 
                             const char *,
                             void * parm,
                             unsigned * parmLen)
{
  if (parmLen == NULL || parm == NULL || *parmLen != sizeof(struct PluginCodec_Option **))
    return 0;
  for (const char * const * option = (const char * const *)parm; *option != NULL; option += 2) {
      TRACE (4, "H263+\tDecoder\tGetting Option " << option[0] << " = " << option[1]);
  }
  *(const void **)parm = codec->userData;
  return 1;
}



extern "C" {
  PLUGIN_CODEC_IMPLEMENT(FFMPEG_H263P)

  PLUGIN_CODEC_DLL_API struct PluginCodec_Definition * PLUGIN_CODEC_GET_CODEC_FN(unsigned * count, unsigned version)
  {
    char * debug_level = getenv ("PWLIB_TRACE_CODECS");
    if (debug_level!=NULL) {
      Trace::SetLevel(atoi(debug_level));
    } 
    else {
      Trace::SetLevel(0);
    }
		    
#ifdef WITH_STACKALIGN_HACK
  STACKALIGN_HACK()
#endif
    if (!FFMPEGLibraryInstance.Load()) {
      *count = 0;
      return NULL;
    }

    *count = sizeof(h263CodecDefn) / sizeof(struct PluginCodec_Definition);
    return h263CodecDefn;
  }

};
