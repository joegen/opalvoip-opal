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
#include <math.h>
#include "trace.h"
#include "dyna.h"
#include "mpi.h"

extern "C" {
#include "libavcodec/avcodec.h"
};

static FFMPEGLibrary FFMPEGLibraryInstance(CODEC_ID_H263P);

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
    sprintf(buffer, "H263+\tFFMPEG\t");
    vsprintf(buffer + strlen(buffer), fmt, arg);
    if (strlen(buffer) > 0)
      buffer[strlen(buffer)-1] = 0;
    if (severity = 4) 
      { TRACE_UP (severity, buffer); }
    else
      { TRACE (severity, buffer); }
  }
}
/////////////////////////////////////////////////////////////////////////////
static char * num2str(int num)
{
  char buf[20];
  sprintf(buf, "%i", num);
  return strdup(buf);
}
      
/////////////////////////////////////////////////////////////////////////////
      
H263PEncoderContext::H263PEncoderContext() 
{ 
  if (!FFMPEGLibraryInstance.IsLoaded())
    return;

  _codec = FFMPEGLibraryInstance.AvcodecFindEncoder(CODEC_ID_H263P);
  if (_codec == NULL) {
    TRACE(1, "H263+\tEncoder\tCodec not found for encoder");
    return;
  }

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

  _txH263PFrame = new H263PFrame(MAX_YUV420P_FRAME_SIZE);

  InitContext();
  SetFrameWidth(CIF_WIDTH);
  SetFrameHeight(CIF_HEIGHT);
  SetTargetBitrate(256000);
  SetTSTO(31);
  DisableAnnex(D);
  DisableAnnex(F);
  DisableAnnex(I);
  DisableAnnex(K);
  DisableAnnex(J);
  DisableAnnex(S);
  SetMaxKeyFramePeriod(H263P_KEY_FRAME_INTERVAL);
  SetMaxRTPFrameSize(H263P_PAYLOAD_SIZE);

  _frameCount = 0;

  TRACE(3, "H263+\tEncoder\tH263+ encoder created");
}

H263PEncoderContext::~H263PEncoderContext()
{
  WaitAndSignal m(_mutex);

  if (_txH263PFrame)
    delete _txH263PFrame;

  if (FFMPEGLibraryInstance.IsLoaded()) {
    CloseCodec();

    FFMPEGLibraryInstance.AvcodecFree(_context);
    FFMPEGLibraryInstance.AvcodecFree(_inputFrame);
  }  
  TRACE(3, "H263+\tEncoder\tH263+ encoder closed");
}

void H263PEncoderContext::InitContext()
{
  _context->codec = NULL;
  _context->mb_decision = FF_MB_DECISION_SIMPLE; // choose only one MB type at a time
  _context->me_method = ME_EPZS;

  _context->max_b_frames = 0;
  _context->pix_fmt = PIX_FMT_YUV420P;
  _context->rtp_mode = 1;

  // X-Lite does not like Custom Picture frequency clocks...
  _context->time_base.num = 100; 
  _context->time_base.den = 2997;

//  _context->flags = 0;
  // avoid copying input/output
  _context->flags |= CODEC_FLAG_INPUT_PRESERVED; // we guarantee to preserve input for max_b_frames+1 frames
  _context->flags |= CODEC_FLAG_EMU_EDGE;        // don't draw edges
  _context->flags |= CODEC_FLAG_PASS1;

  // debugging flags
  if (Trace::CanTraceUserPlane(4)) {
    _context->debug |= FF_DEBUG_RC;
    _context->debug |= FF_DEBUG_PICT_INFO;
    _context->debug |= FF_DEBUG_MV;
    _context->debug |= FF_DEBUG_QP;
  }
}

void H263PEncoderContext::SetMaxRTPFrameSize (unsigned size)
{
   if ((size * 6 / 7) > 0)
    _context->rtp_payload_size = (size * 6 / 7);
    else  
    _context->rtp_payload_size = size;

  _txH263PFrame->SetMaxPayloadSize(size);
}

void H263PEncoderContext::SetMaxKeyFramePeriod (unsigned period)
{
  _context->gop_size = period;
}

void H263PEncoderContext::SetTargetBitrate (unsigned rate)
{
  _context->bit_rate = (rate * 3) >> 2;        // average bit rate
  _context->bit_rate_tolerance = rate >> 1;
  _context->rc_min_rate = 0;                   // minimum bitrate
  _context->rc_max_rate = rate;                // maximum bitrate

  /* ratecontrol qmin qmax limiting method
     0-> clipping, 1-> use a nice continous function to limit qscale wthin qmin/qmax.
  */
  
  _context->rc_qsquish = 0;                    // limit q by clipping 
  _context->rc_eq = (char*) "1";       // rate control equation
  _context->rc_buffer_size = rate * 64;
}

void H263PEncoderContext::SetFrameWidth (unsigned width)
{
  _context->width  = width;

  _inputFrame->linesize[0] = width;
  _inputFrame->linesize[1] = width / 2;
  _inputFrame->linesize[2] = width / 2;

}

void H263PEncoderContext::SetFrameHeight (unsigned height)
{
  _context->height = height;
}

void H263PEncoderContext::SetTSTO (unsigned tsto)
{
  _inputFrame->quality = H263P_MIN_QUANT;

  _context->max_qdiff = 3; // max q difference between frames
  _context->qcompress = 0.5; // qscale factor between easy & hard scenes (0.0-1.0)
  _context->i_quant_factor = (float)-0.6; // qscale factor between p and i frames
  _context->i_quant_offset = (float)0.0; // qscale offset between p and i frames
  _context->me_subpel_quality = 8;

  _context->qmin = H263P_MIN_QUANT;
  _context->qmax = round ( (double)(31 - H263P_MIN_QUANT) / 31 * tsto + H263P_MIN_QUANT);
  _context->qmax = std::min( _context->qmax, 31);
  
  _context->mb_qmin = _context->qmin;
  _context->mb_qmax = _context->qmax;

  // Lagrange multipliers - this is how the context defaults do it:
  _context->lmin = _context->qmin * FF_QP2LAMBDA;
  _context->lmax = _context->qmax * FF_QP2LAMBDA; 
}

void H263PEncoderContext::EnableAnnex (Annex annex)
{
  switch (annex) {
    case D:
      // Annex D: Unrestructed Motion Vectors
      // Level 2+ 
      // works with eyeBeam, signaled via  non-standard "D"
      _context->flags |= CODEC_FLAG_H263P_UMV; 
      break;
    case F:
      // Annex F: Advanced Prediction Mode
      // does not work with eyeBeam
      //_context->flags |= CODEC_FLAG_OBMC; 
      break;
    case I:
      // Annex I: Advanced Intra Coding
      // Level 3+
      // works with eyeBeam
      _context->flags |= CODEC_FLAG_AC_PRED; 
      break;
    case K:
      // Annex K: 
      // does not work with eyeBeam
      //_context->flags |= CODEC_FLAG_H263P_SLICE_STRUCT;  
      break;
    case J:
      // Annex J: Deblocking Filter
      // works with eyeBeam
      _context->flags |= CODEC_FLAG_LOOP_FILTER;
      break;
    case T:
      break;
    case S:
      // Annex S: Alternative INTER VLC mode
      // does not work with eyeBeam
      //_context->flags |= CODEC_FLAG_H263P_AIV;
      break;
    case N:
    case P:
    default:
      break;
  }
}

void H263PEncoderContext::DisableAnnex (Annex annex)
{
  switch (annex) {
    case D:
      // Annex D: Unrestructed Motion Vectors
      // Level 2+ 
      // works with eyeBeam, signaled via  non-standard "D"
      _context->flags &= ~CODEC_FLAG_H263P_UMV; 
      break;
    case F:
      // Annex F: Advanced Prediction Mode
      // does not work with eyeBeam
      _context->flags &= ~CODEC_FLAG_OBMC; 
      break;
    case I:
      // Annex I: Advanced Intra Coding
      // Level 3+
      // works with eyeBeam
      _context->flags &= ~CODEC_FLAG_AC_PRED; 
      break;
    case K:
      // Annex K: 
      // does not work with eyeBeam
      _context->flags &= ~CODEC_FLAG_H263P_SLICE_STRUCT;  
      break;
    case J:
      // Annex J: Deblocking Filter
      // works with eyeBeam
      _context->flags &= ~CODEC_FLAG_LOOP_FILTER;  
      break;
    case T:
      break;
    case S:
      // Annex S: Alternative INTER VLC mode
      // does not work with eyeBeam
      _context->flags &= ~CODEC_FLAG_H263P_AIV;
      break;
    case N:
    case P:
    default:
      break;
  }
}

bool H263PEncoderContext::OpenCodec()
{
  if (_codec == NULL) {
    TRACE(1, "H263+\tEncoder\tCodec not initialized");
    return false;
  }

  return FFMPEGLibraryInstance.AvcodecOpen(_context, _codec) == 0;
}

void H263PEncoderContext::CloseCodec()
{
  if (_context != NULL) {
    if (_context->codec != NULL) {
      FFMPEGLibraryInstance.AvcodecClose(_context);
    }
  }
}

int H263PEncoderContext::EncodeFrames(const BYTE * src, unsigned & srcLen, BYTE * dst, unsigned & dstLen, unsigned int & flags)
{
  WaitAndSignal m(_mutex);

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
  if ((_frameCount == 0) || 
      ((unsigned) _context->width !=  header->width) || 
      ((unsigned) _context->height != header->height)) {

    TRACE(4,  "H263+\tEncoder\tFirst frame received or resolution has changed - reopening codec");
    CloseCodec();
    SetFrameWidth(header->width);
    SetFrameHeight(header->height);
    if (!OpenCodec()) {
      TRACE(1,  "H263+\tEncoder\tReopening codec failed");
      return false;
    }
  }

  int size = _context->width * _context->height;
  int frameSize = (size * 3) >> 1;
 
  // we need FF_INPUT_BUFFER_PADDING_SIZE allocated bytes after the YVU420P image for the encoder
  memset (_inputFrameBuffer, 0 , FF_INPUT_BUFFER_PADDING_SIZE);
  memcpy (_inputFrameBuffer + FF_INPUT_BUFFER_PADDING_SIZE, OPAL_VIDEO_FRAME_DATA_PTR(header), frameSize);
  memset (_inputFrameBuffer + FF_INPUT_BUFFER_PADDING_SIZE + frameSize, 0 , FF_INPUT_BUFFER_PADDING_SIZE);

  _inputFrame->data[0] = _inputFrameBuffer + FF_INPUT_BUFFER_PADDING_SIZE;
  _inputFrame->data[1] = _inputFrame->data[0] + size;
  _inputFrame->data[2] = _inputFrame->data[1] + (size / 4);
  _inputFrame->pict_type = (flags && forceIFrame) ? FF_I_TYPE : 0;
 
  _txH263PFrame->BeginNewFrame();
  _txH263PFrame->SetTimestamp(srcRTP.GetTimestamp());
  _txH263PFrame->SetFrameSize (FFMPEGLibraryInstance.AvcodecEncodeVideo(_context, _txH263PFrame->GetFramePtr(), frameSize, _inputFrame));  
  _frameCount++; 

  if (_txH263PFrame->GetFrameSize() == 0) {
    TRACE(1, "H263+\tEncoder internal error - there should be outstanding packets at this point");
    return 1;
  }

  if (_txH263PFrame->HasRTPFrames())
  {
    _txH263PFrame->GetRTPFrame(dstRTP, flags);
    dstLen = dstRTP.GetFrameLen();
    return true;
  }
  return 1;
}

void H263PEncoderContext::Lock()
{
  _mutex.Wait();
}

void H263PEncoderContext::Unlock()
{
  _mutex.Signal();
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

  // debugging flags
  if (Trace::CanTrace(4)) {
    _context->debug |= FF_DEBUG_RC;
    _context->debug |= FF_DEBUG_PICT_INFO;
    _context->debug |= FF_DEBUG_MV;
  }

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

  if (!_rxH263PFrame->SetFromRTPFrame(srcRTP, flags)) {
    _rxH263PFrame->BeginNewFrame();
    flags = PluginCodec_ReturnCoderRequestIFrame;
    return 1;
  }
  
  if (srcRTP.GetMarker()==0)
  {
     return 1;
  } 

  if (_rxH263PFrame->GetFrameSize()==0)
  {
    _rxH263PFrame->BeginNewFrame();
    TRACE(4, "H263+\tDecoder\tGot an empty frame - skipping");
    _skippedFrameCounter++;
    return 1;
  }

  if (!_rxH263PFrame->hasPicHeader()) {
    TRACE(1, "H263+\tDecoder\tReceived frame has no picture header - dropping");
    _rxH263PFrame->BeginNewFrame();
    flags = PluginCodec_ReturnCoderRequestIFrame;
    return 1;
  }

  // look and see if we have read an I frame.
  if (!_gotIFrame)
  {
    if (!_rxH263PFrame->IsIFrame())
    {
      TRACE(1, "H263+\tDecoder\tWaiting for an I-Frame");
      _rxH263PFrame->BeginNewFrame();
      flags = PluginCodec_ReturnCoderRequestIFrame;
      return 1;
    }
    _gotIFrame = true;
  }

  int gotPicture = 0;

  TRACE_UP(4, "H263+\tDecoder\tDecoding " << _rxH263PFrame->GetFrameSize()  << " bytes");
  int bytesDecoded = FFMPEGLibraryInstance.AvcodecDecodeVideo(_context, _outputFrame, &gotPicture, _rxH263PFrame->GetFramePtr(), _rxH263PFrame->GetFrameSize());

  _rxH263PFrame->BeginNewFrame();

  if (!gotPicture) 
  {
    TRACE(1, "H263+\tDecoder\tDecoded "<< bytesDecoded << " bytes without getting a Picture, requesting I frame"); 
    _skippedFrameCounter++;
    flags = PluginCodec_ReturnCoderRequestIFrame;
    return 1;
  }

  TRACE_UP(4, "H263+\tDecoder\tDecoded " << bytesDecoded << " bytes"<< ", Resolution: " << _context->width << "x" << _context->height);

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

  flags = PluginCodec_ReturnCoderLastFrame ;

  _frameCount++;

  return 1;
}

/////////////////////////////////////////////////////////////////////////////

static int get_codec_options(const struct PluginCodec_Definition * codec,
                                                  void *,
                                                  const char *,
                                                  void * parm,
                                                  unsigned * parmLen)
{
    if (parmLen == NULL || parm == NULL || *parmLen != sizeof(struct PluginCodec_Option **))
        return 0;

    *(const void **)parm = codec->userData;
    *parmLen = 0; //FIXME
    return 1;
}

static int free_codec_options ( const struct PluginCodec_Definition *, void *, const char *, void * parm, unsigned * parmLen)
{
  if (parmLen == NULL || parm == NULL || *parmLen != sizeof(char ***))
    return 0;

  char ** strings = (char **) parm;
  for (char ** string = strings; *string != NULL; string++)
    free(*string);
  free(strings);
  return 1;
}

static int valid_for_protocol ( const struct PluginCodec_Definition *, void *, const char *, void * parm, unsigned * parmLen)
{
  if (parmLen == NULL || parm == NULL || *parmLen != sizeof(char *))
    return 0;

  return (STRCMPI((const char *)parm, "sip") == 0) ? 1 : 0;
}

/////////////////////////////////////////////////////////////////////////////

static void * create_encoder(const struct PluginCodec_Definition * /*codec*/)
{
  return new H263PEncoderContext;
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

static int to_normalised_options(const struct PluginCodec_Definition *, void *, const char *, void * parm, unsigned * parmLen)
{
  if (parmLen == NULL || parm == NULL || *parmLen != sizeof(char ***))
    return 0;

  MPIList h263MPIList;
  for (const char * const * option = *(const char * const * *)parm; *option != NULL; option += 2) {
      if (STRCMPI(option[0], PLUGINCODEC_OPTION_FRAME_WIDTH) == 0)
        h263MPIList.setDesiredWidth(atoi(option[1]));
      if (STRCMPI(option[0], PLUGINCODEC_OPTION_FRAME_HEIGHT) == 0)
        h263MPIList.setDesiredHeight(atoi(option[1]));
      if (STRCMPI(option[0], PLUGINCODEC_OPTION_FRAME_TIME) == 0)
        h263MPIList.setFrameTime( atoi(option[1]));
      if (STRCMPI(option[0], PLUGINCODEC_SQCIF_MPI) == 0)
        h263MPIList.addMPI(SQCIF_WIDTH, SQCIF_HEIGHT, atoi(option[1]) );
      if (STRCMPI(option[0], PLUGINCODEC_QCIF_MPI) == 0)
        h263MPIList.addMPI(QCIF_WIDTH, QCIF_HEIGHT, atoi(option[1]) );
      if (STRCMPI(option[0], PLUGINCODEC_CIF_MPI) == 0)
        h263MPIList.addMPI(CIF_WIDTH, CIF_HEIGHT, atoi(option[1]) );
      if (STRCMPI(option[0], PLUGINCODEC_CIF4_MPI) == 0)
        h263MPIList.addMPI(CIF4_WIDTH, CIF4_HEIGHT, atoi(option[1]) );
      if (STRCMPI(option[0], PLUGINCODEC_CIF16_MPI) == 0)
        h263MPIList.addMPI(CIF16_WIDTH, CIF16_HEIGHT, atoi(option[1]) );
  }

  // Defaul value(s)
  if (h263MPIList.size() == 0) {
#ifdef WITH_RFC_COMPLIANT_DEFAULTS
    h263MPIList.addMPI(QCIF_WIDTH, QCIF_HEIGHT, 2 );
#else
    h263MPIList.addMPI(SQCIF_WIDTH, SQCIF_HEIGHT, 1);
    h263MPIList.addMPI(QCIF_WIDTH,  QCIF_HEIGHT,  1);
    h263MPIList.addMPI(CIF_WIDTH,   CIF_HEIGHT,   1);
    h263MPIList.addMPI(CIF4_WIDTH,  CIF4_HEIGHT, 1);
    h263MPIList.addMPI(CIF16_WIDTH, CIF16_HEIGHT, 1);
#endif 
  }
  
  char ** options = (char **)calloc(7, sizeof(char *));
  *(char ***)parm = options;
  if (options == NULL)
    return 0;

  unsigned width, height, fps;
  h263MPIList.getNegotiatedMPI(&width, &height, &fps);
  options[0] = strdup(PLUGINCODEC_OPTION_FRAME_WIDTH);
  options[1] = num2str(width);
  options[2] = strdup(PLUGINCODEC_OPTION_FRAME_HEIGHT);
  options[3] = num2str(height);
  options[4] = strdup(PLUGINCODEC_OPTION_FRAME_TIME);
  options[5] = num2str(fps);

  return 1;
}

static int to_customised_options(const struct PluginCodec_Definition *, void *, const char *, void * parm, unsigned * parmLen)
{
  if (parmLen == NULL || parm == NULL || *parmLen != sizeof(char ***))
    return 0;

  MPIList h263MPIList;
  for (const char * const * option = *(const char * const * *)parm; *option != NULL; option += 2) {
      if (STRCMPI(option[0], PLUGINCODEC_OPTION_MIN_RX_FRAME_WIDTH) == 0)
        h263MPIList.setMinWidth(atoi(option[1]));
      if (STRCMPI(option[0], PLUGINCODEC_OPTION_MIN_RX_FRAME_HEIGHT) == 0)
        h263MPIList.setMinHeight(atoi(option[1]));
      if (STRCMPI(option[0], PLUGINCODEC_OPTION_MAX_RX_FRAME_WIDTH) == 0)
        h263MPIList.setMaxWidth(atoi(option[1]));
      if (STRCMPI(option[0], PLUGINCODEC_OPTION_MAX_RX_FRAME_HEIGHT) == 0)
        h263MPIList.setMaxHeight(atoi(option[1]));
      if (STRCMPI(option[0], PLUGINCODEC_SQCIF_MPI) == 0)
        h263MPIList.addMPI(SQCIF_WIDTH, SQCIF_HEIGHT, atoi(option[1]) );
      if (STRCMPI(option[0], PLUGINCODEC_QCIF_MPI) == 0)
        h263MPIList.addMPI(QCIF_WIDTH, QCIF_HEIGHT, atoi(option[1]) );
      if (STRCMPI(option[0], PLUGINCODEC_CIF_MPI) == 0)
        h263MPIList.addMPI(CIF_WIDTH, CIF_HEIGHT, atoi(option[1]));
      if (STRCMPI(option[0], PLUGINCODEC_CIF4_MPI) == 0)
        h263MPIList.addMPI(CIF4_WIDTH, CIF4_HEIGHT, atoi(option[1]));
      if (STRCMPI(option[0], PLUGINCODEC_CIF16_MPI) == 0)
        h263MPIList.addMPI(CIF16_WIDTH, CIF16_HEIGHT, atoi(option[1]));
      if (STRCMPI(option[0], PLUGINCODEC_OPTION_FRAME_TIME) == 0)
        h263MPIList.setFrameTime( atoi(option[1]) );
  }

  unsigned qcif_mpi = h263MPIList.getSupportedMPI(QCIF_WIDTH, QCIF_HEIGHT);
  unsigned cif_mpi = h263MPIList.getSupportedMPI(CIF_WIDTH, CIF_HEIGHT);
  unsigned sqcif_mpi = h263MPIList.getSupportedMPI(SQCIF_WIDTH, SQCIF_HEIGHT);
  unsigned cif4_mpi = h263MPIList.getSupportedMPI(CIF4_WIDTH, CIF4_HEIGHT);
  unsigned cif16_mpi = h263MPIList.getSupportedMPI(CIF16_WIDTH, CIF16_HEIGHT);

  if ((qcif_mpi == PLUGINCODEC_MPI_DISABLED) 
   && (cif_mpi == PLUGINCODEC_MPI_DISABLED)
   && (sqcif_mpi == PLUGINCODEC_MPI_DISABLED)
   && (cif4_mpi == PLUGINCODEC_MPI_DISABLED)
   && (cif16_mpi == PLUGINCODEC_MPI_DISABLED)) {
    TRACE(1, "H.263+\tNeg\tNo MPI was about to be set - illegal");
    return 0; // illegal
  }

  char ** options = (char **)calloc(11, sizeof(char *));
  *(char ***)parm = options;
  if (options == NULL)
    return 0;

  options[0] = strdup(PLUGINCODEC_QCIF_MPI);
  options[1] = num2str(qcif_mpi);
  options[2] = strdup(PLUGINCODEC_CIF_MPI);
  options[3] = num2str(cif_mpi);
  options[4] = strdup(PLUGINCODEC_SQCIF_MPI);
  options[5] = num2str(sqcif_mpi);
  options[6] = strdup(PLUGINCODEC_CIF4_MPI);
  options[7] = num2str(cif4_mpi);
  options[8] = strdup(PLUGINCODEC_CIF16_MPI);
  options[9] = num2str(cif16_mpi);

  return 1;
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

  context->Lock();
  context->CloseCodec();

  // get the "frame width" media format parameter to use as a hint for the encoder to start off
  for (const char * const * option = (const char * const *)parm; *option != NULL; option += 2) {
    if (STRCMPI(option[0], PLUGINCODEC_OPTION_FRAME_WIDTH) == 0)
      context->SetFrameWidth (atoi(option[1]));
    if (STRCMPI(option[0], PLUGINCODEC_OPTION_FRAME_HEIGHT) == 0)
      context->SetFrameHeight (atoi(option[1]));
/*    if (STRCMPI(option[0], PLUGINCODEC_OPTION_MAX_FRAME_SIZE) == 0)
      context->SetMaxRTPFrameSize (atoi(option[1]));
    if (STRCMPI(option[0], PLUGINCODEC_OPTION_TARGET_BIT_RATE) == 0)
       context->SetTargetBitrate(atoi(option[1]));
    if (STRCMPI(option[0], PLUGINCODEC_OPTION_TX_KEY_FRAME_PERIOD) == 0)
      context->SetMaxKeyFramePeriod (atoi(option[1]));
    if (STRCMPI(option[0], PLUGINCODEC_OPTION_TEMPORAL_SPATIAL_TRADE_OFF) == 0)
       context->SetTSTO (atoi(option[1]));

    if (STRCMPI(option[0], "Annex D") == 0)
      if (atoi(option[1]) == 1)
        context->EnableAnnex (D);
       else
        context->DisableAnnex (D);
    if (STRCMPI(option[0], "Annex F") == 0)
      if (atoi(option[1]) == 1)
        context->EnableAnnex (F);
       else
        context->DisableAnnex (F);
    if (STRCMPI(option[0], "Annex I") == 0)
      if (atoi(option[1]) == 1)
        context->EnableAnnex (I);
       else
        context->DisableAnnex (I);
    if (STRCMPI(option[0], "Annex K") == 0)
      if (atoi(option[1]) == 1)
        context->EnableAnnex (K);
       else
        context->DisableAnnex (K);
    if (STRCMPI(option[0], "Annex J") == 0)
      if (atoi(option[1]) == 1)
        context->EnableAnnex (J);
       else
        context->DisableAnnex (J);
    if (STRCMPI(option[0], "Annex S") == 0)
      if (atoi(option[1]) == 1)
        context->EnableAnnex (S);
       else
        context->DisableAnnex (S);
*/
  }

  context->OpenCodec();
  context->Unlock();
  return 1;
}

static int encoder_get_output_data_size(const PluginCodec_Definition *, void *, const char *, void *, unsigned *)
{
  return 2000; //FIXME
}

/////////////////////////////////////////////////////////////////////////////

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

/////////////////////////////////////////////////////////////////////////////

extern "C" {
  PLUGIN_CODEC_IMPLEMENT(FFMPEG_H263P)

  PLUGIN_CODEC_DLL_API struct PluginCodec_Definition * PLUGIN_CODEC_GET_CODEC_FN(unsigned * count, unsigned version)
  {
    char * debug_level = getenv ("PTLIB_TRACE_CODECS");
    if (debug_level!=NULL) {
      Trace::SetLevel(atoi(debug_level));
    } 
    else {
      Trace::SetLevel(0);
    }

    debug_level = getenv ("PTLIB_TRACE_CODECS_USER_PLANE");
    if (debug_level!=NULL) {
      Trace::SetLevelUserPlane(atoi(debug_level));
    } 
    else {
      Trace::SetLevelUserPlane(0);
    }

    if (!FFMPEGLibraryInstance.Load()) {
      *count = 0;
      TRACE(1, "H263+\tCodec\tDisabled");
      return NULL;
    }

    FFMPEGLibraryInstance.AvLogSetLevel(AV_LOG_DEBUG);
    FFMPEGLibraryInstance.AvLogSetCallback(&logCallbackFFMPEG);

    if (version < PLUGIN_CODEC_VERSION_OPTIONS) {
      *count = 0;
      TRACE(1, "H263+\tCodec\tDisabled - plugin version mismatch");
      return NULL;
    }
    else {
      *count = sizeof(h263PCodecDefn) / sizeof(struct PluginCodec_Definition);
      TRACE(1, "H263+\tCodec\tEnabled");
      return h263PCodecDefn;
    }
  }

};
