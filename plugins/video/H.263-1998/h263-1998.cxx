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

#ifndef PLUGIN_CODEC_DLL_EXPORTS
#include "plugin-config.h"
#endif

#include "h263-1998.h"
#include <limits>
#include <iomanip>
#include <stdio.h>
#include <math.h>
#include <string.h>

#include "../common/mpi.h"
#include "../common/dyna.h"
#include "rfc2190.h"
#include "rfc2429.h"


extern "C" {
#include LIBAVCODEC_HEADER
};


static const char YUV420PDesc[]  = { "YUV420P" };
static const char h263PDesc[]    = { "H.263plus" };
static const char sdpH263P[]     = { "h263-1998" };

static const char h263Desc[]     = { "H.263" };
static const char sdpH263[]      = { "h263" };


#define MAX_H263_CUSTOM_SIZES 10
#define DEFAULT_CUSTOM_MPI "0,0,"STRINGIZE(PLUGINCODEC_MPI_DISABLED)

static struct StdSizes {
  enum { 
    SQCIF, 
    QCIF, 
    CIF, 
    CIF4, 
    CIF16, 
    NumStdSizes,
    UnknownStdSize = NumStdSizes
  };

  int width;
  int height;
  const char * optionName;
} StandardVideoSizes[StdSizes::NumStdSizes] = {
  { SQCIF_WIDTH, SQCIF_HEIGHT, PLUGINCODEC_SQCIF_MPI },
  {  QCIF_WIDTH,  QCIF_HEIGHT, PLUGINCODEC_QCIF_MPI  },
  {   CIF_WIDTH,   CIF_HEIGHT, PLUGINCODEC_CIF_MPI   },
  {  CIF4_WIDTH,  CIF4_HEIGHT, PLUGINCODEC_CIF4_MPI  },
  { CIF16_WIDTH, CIF16_HEIGHT, PLUGINCODEC_CIF16_MPI },
};

static FFMPEGLibrary FFMPEGLibraryInstance(CODEC_ID_H263P);


/////////////////////////////////////////////////////////////////////////////

static char * num2str(int num)
{
  char buf[20];
  sprintf(buf, "%i", num);
  return strdup(buf);
}

#if TRACE_RTP

static void DumpRTPPayload(std::ostream & trace, const RTPFrame & rtp, unsigned max)
{
  if (max > rtp.GetPayloadSize())
    max = rtp.GetPayloadSize();
  BYTE * ptr = rtp.GetPayloadPtr();
  trace << std::hex << std::setfill('0') << std::setprecision(2);
  while (max-- > 0) 
    trace << (int) *ptr++ << ' ';
  trace << std::setfill(' ') << std::dec;
}

static void RTPDump(std::ostream & trace, const RTPFrame & rtp)
{
  trace << "seq=" << rtp.GetSequenceNumber()
       << ",ts=" << rtp.GetTimestamp()
       << ",mkr=" << rtp.GetMarker()
       << ",pt=" << (int)rtp.GetPayloadType()
       << ",ps=" << rtp.GetPayloadSize();
}

static void RFC2190Dump(std::ostream & trace, const RTPFrame & rtp)
{
  RTPDump(trace, rtp);
  if (rtp.GetPayloadSize() > 2) {
    bool iFrame = false;
    char mode;
    BYTE * payload = rtp.GetPayloadPtr();
    if ((payload[0] & 0x80) == 0) {
      mode = 'A';
      iFrame = (payload[1] & 0x10) == 0;
    }
    else if ((payload[0] & 0x40) == 0) {
      mode = 'B';
      iFrame = (payload[4] & 0x80) == 0;
    }
    else {
      mode = 'C';
      iFrame = (payload[4] & 0x80) == 0;
    }
    trace << "mode=" << mode << ",I=" << (iFrame ? "yes" : "no");
  }
  trace << ",data=";
  DumpRTPPayload(trace, rtp, 10);
}

static void RFC2429Dump(std::ostream & trace, const RTPFrame & rtp)
{
  RTPDump(trace, rtp);
  trace << ",data=";
  DumpRTPPayload(trace, rtp, 10);
}

#define CODEC_TRACER_RTP(text, rtp, func) \
  if (PTRACE_CHECK(6)) { \
    std::ostringstream strm; strm << text; func(strm, rtp); \
    PluginCodec_LogFunctionInstance(6, __FILE__, __LINE__, m_prefix, strm.str().c_str()); \
  } else (void)0

#else

#define CODEC_TRACER_RTP(text, rtp, func) 

#endif
      
/////////////////////////////////////////////////////////////////////////////
      
H263_Base_EncoderContext::H263_Base_EncoderContext(const char * prefix, Packetizer * packetizer)
  : m_prefix(prefix)
  , m_codec(NULL)
  , m_context(NULL)
  , m_inputFrame(NULL)
  , m_alignedInputYUV(NULL)
  , m_packetizer(packetizer)
{ 
  FFMPEGLibraryInstance.Load();
}

H263_Base_EncoderContext::~H263_Base_EncoderContext()
{
  WaitAndSignal m(m_mutex);

  CloseCodec();

  if (m_context != NULL)
    FFMPEGLibraryInstance.AvcodecFree(m_context);
  if (m_inputFrame != NULL)
    FFMPEGLibraryInstance.AvcodecFree(m_inputFrame);
  if (m_alignedInputYUV != NULL)
    free(m_alignedInputYUV);

  delete m_packetizer;

  PTRACE(4, m_prefix, "Encoder closed");
}

bool H263_Base_EncoderContext::Init(CodecID codecId)
{
  PTRACE(5, m_prefix, "Opening encoder");

  if (!FFMPEGLibraryInstance.IsLoaded())
    return false;

  m_codec = FFMPEGLibraryInstance.AvcodecFindEncoder(codecId);
  if (m_codec == NULL) {
    PTRACE(1, m_prefix, "Codec not found for encoder");
    return false;
  }

  m_context = FFMPEGLibraryInstance.AvcodecAllocContext();
  if (m_context == NULL) {
    PTRACE(1, m_prefix, "Failed to allocate context for encoder");
    return false;
  }

  m_inputFrame = FFMPEGLibraryInstance.AvcodecAllocFrame();
  if (m_inputFrame == NULL) {
    PTRACE(1, m_prefix, "Failed to allocate frame for encoder");
    return false;
  }

  m_context->opaque = this;

  m_context->flags = CODEC_FLAG_EMU_EDGE   // don't draw edges
                   | CODEC_FLAG_TRUNCATED  // Possible missing packets
                   ;

  m_context->pix_fmt = PIX_FMT_YUV420P;
  m_context->gop_size = H263_KEY_FRAME_INTERVAL;

  // X-Lite does not like Custom Picture frequency clocks... stick to 29.97Hz
  m_context->time_base.num = 100;
  m_context->time_base.den = 2997;

  // debugging flags
#if PLUGINCODEC_TRACING
  if (PTRACE_CHECK(4))
    m_context->debug |= FF_DEBUG_ER;
  if (PTRACE_CHECK(5))
    m_context->debug |= FF_DEBUG_PICT_INFO | FF_DEBUG_RC;
  if (PTRACE_CHECK(6))
    m_context->debug |= FF_DEBUG_BUGS | FF_DEBUG_BUFFERS;
#endif

  PTRACE(4, m_prefix, "Encoder created");

  return true;
}

bool H263_Base_EncoderContext::SetOptions(const char * const * options)
{
  Lock();
  CloseCodec();

  // get the "frame width" media format parameter to use as a hint for the encoder to start off
  for (const char * const * option = options; *option != NULL; option += 2)
    SetOption(option[0], option[1]);

  bool ok = OpenCodec();
  Unlock();
  return ok;
}


void H263_Base_EncoderContext::SetOption(const char * option, const char * value)
{
  if (STRCMPI(option, PLUGINCODEC_OPTION_FRAME_TIME) == 0) {
    m_context->time_base.den = 2997;
    m_context->time_base.num = atoi(value)*m_context->time_base.den/VIDEO_CLOCKRATE;
    return;
  }

  if (STRCMPI(option, PLUGINCODEC_OPTION_FRAME_WIDTH) == 0) {
    FFMPEGLibraryInstance.AvSetDimensions(m_context, atoi(value), m_context->height);
    return;
  }

  if (STRCMPI(option, PLUGINCODEC_OPTION_FRAME_HEIGHT) == 0) {
    FFMPEGLibraryInstance.AvSetDimensions(m_context, m_context->width, atoi(value));
    return;
  }

  if (STRCMPI(option, PLUGINCODEC_OPTION_MAX_TX_PACKET_SIZE) == 0) {
    m_context->rtp_payload_size = atoi(value);
    m_packetizer->SetMaxPayloadSize(m_context->rtp_payload_size);
    return;
  }

  if (STRCMPI(option, PLUGINCODEC_OPTION_TARGET_BIT_RATE) == 0) {
    m_context->bit_rate = atoi(value);
    return;
  }

  if (STRCMPI(option, PLUGINCODEC_OPTION_TEMPORAL_SPATIAL_TRADE_OFF) == 0) {
    m_context->qmax = atoi(value);
    return;
  }

  if (STRCMPI(option, PLUGINCODEC_OPTION_TX_KEY_FRAME_PERIOD) == 0) {
    m_context->gop_size = atoi(value);
    return;
  }

  if (STRCMPI(option, H263_ANNEX_D) == 0) {
    // Annex D: Unrestructed Motion Vectors
    // Level 2+ 
    // works with eyeBeam, signaled via  non-standard "D"
    if (atoi(value) == 1)
      m_context->flags |= CODEC_FLAG_H263P_UMV; 
    else
      m_context->flags &= ~CODEC_FLAG_H263P_UMV; 
    return;
  }

#if 0 // DO NOT ENABLE THIS FLAG. FFMPEG IS NOT THREAD_SAFE WHEN THIS FLAG IS SET
  if (STRCMPI(option, H263_ANNEX_F) == 0) {
    // Annex F: Advanced Prediction Mode
    // does not work with eyeBeam
    if (atoi(value) == 1)
      m_context->flags |= CODEC_FLAG_OBMC; 
    else
      m_context->flags &= ~CODEC_FLAG_OBMC; 
    return;
  }
#endif

  if (STRCMPI(option, H263_ANNEX_I) == 0) {
    // Annex I: Advanced Intra Coding
    // Level 3+
    // works with eyeBeam
    if (atoi(value) == 1)
      m_context->flags |= CODEC_FLAG_AC_PRED; 
    else
      m_context->flags &= ~CODEC_FLAG_AC_PRED; 
    return;
  }

  if (STRCMPI(option, H263_ANNEX_J) == 0) {
    // Annex J: Deblocking Filter
    // works with eyeBeam
    if (atoi(value) == 1)
      m_context->flags |= CODEC_FLAG_LOOP_FILTER; 
    else
      m_context->flags &= ~CODEC_FLAG_LOOP_FILTER; 
    return;
  }

  if (STRCMPI(option, H263_ANNEX_K) == 0) {
    // Annex K: Slice Structure
    // does not work with eyeBeam
    if (atoi(value) != 0)
      m_context->flags |= CODEC_FLAG_H263P_SLICE_STRUCT; 
    else
      m_context->flags &= ~CODEC_FLAG_H263P_SLICE_STRUCT; 
    return;
  }

  if (STRCMPI(option, H263_ANNEX_S) == 0) {
    // Annex S: Alternative INTER VLC mode
    // does not work with eyeBeam
    if (atoi(value) == 1)
      m_context->flags |= CODEC_FLAG_H263P_AIV; 
    else
      m_context->flags &= ~CODEC_FLAG_H263P_AIV; 
    return;
  }
}


bool H263_Base_EncoderContext::OpenCodec()
{
  if (m_codec == NULL) {
    PTRACE(1, m_prefix, "Codec not initialized");
    return false;
  }

  m_context->rc_min_rate = m_context->bit_rate;
  m_context->rc_max_rate = m_context->bit_rate;
  m_context->rc_buffer_size = m_context->bit_rate*4; // Enough bits for 4 seconds
  m_context->bit_rate_tolerance = m_context->bit_rate/10;

  m_context->i_quant_factor = (float)-0.6;  // qscale factor between p and i frames
  m_context->i_quant_offset = (float)0.0;   // qscale offset between p and i frames
  m_context->max_qdiff = 10;  // was 3      // max q difference between frames
  m_context->qcompress = 0.5;               // qscale factor between easy & hard scenes (0.0-1.0)

  // Lagrange multipliers - this is how the context defaults do it:
  m_context->lmin = m_context->qmin * FF_QP2LAMBDA;
  m_context->lmax = m_context->qmax * FF_QP2LAMBDA; 

  // YUV420P input
  m_inputFrame->linesize[0] = m_context->width;
  m_inputFrame->linesize[1] = m_context->width / 2;
  m_inputFrame->linesize[2] = m_context->width / 2;

  if (m_alignedInputYUV != NULL)
    free(m_alignedInputYUV);

  size_t planeSize = m_context->width*m_context->height;
  m_alignedInputYUV = (uint8_t *)malloc(planeSize*3/2+16);

  m_inputFrame->data[0] = m_alignedInputYUV + 16 - (((size_t)m_alignedInputYUV) & 0xf);
  m_inputFrame->data[1] = m_inputFrame->data[0] + planeSize;
  m_inputFrame->data[2] = m_inputFrame->data[1] + (planeSize / 4);

  // Dump info
  PTRACE(5, m_prefix, "Size is " << m_context->width << "x" << m_context->height);
  PTRACE(5, m_prefix, "GOP is " << m_context->gop_size);
  PTRACE(5, m_prefix, "time_base set to " << m_context->time_base.num << '/' << m_context->time_base.den
         << " (" << std::setprecision(2) << ((double)m_context->time_base.den/m_context->time_base.num) << "fps)");
  PTRACE(5, m_prefix, "bit_rate set to " << m_context->bit_rate);
  PTRACE(5, m_prefix, "rc_max_rate is " <<  m_context->rc_max_rate);
  PTRACE(5, m_prefix, "rc_min_rate set to " << m_context->rc_min_rate);
  PTRACE(5, m_prefix, "bit_rate_tolerance set to " <<m_context->bit_rate_tolerance);
  PTRACE(5, m_prefix, "qmin set to " << m_context->qmin);
  PTRACE(5, m_prefix, "qmax set to " << m_context->qmax);
  PTRACE(5, m_prefix, "payload size set to " << m_context->rtp_payload_size);

  #define CODEC_TRACER_FLAG(tracer, flag) \
    PTRACE(4, m_prefix, #flag " is " << ((m_context->flags & flag) ? "enabled" : "disabled"));
  CODEC_TRACER_FLAG(tracer, CODEC_FLAG_H263P_UMV);
  CODEC_TRACER_FLAG(tracer, CODEC_FLAG_OBMC);
  CODEC_TRACER_FLAG(tracer, CODEC_FLAG_AC_PRED);
  CODEC_TRACER_FLAG(tracer, CODEC_FLAG_H263P_SLICE_STRUCT)
  CODEC_TRACER_FLAG(tracer, CODEC_FLAG_LOOP_FILTER);
  CODEC_TRACER_FLAG(tracer, CODEC_FLAG_H263P_AIV);

  return FFMPEGLibraryInstance.AvcodecOpen(m_context, m_codec) == 0;
}

void H263_Base_EncoderContext::CloseCodec()
{
  if (m_context != NULL && m_context->codec != NULL)
    FFMPEGLibraryInstance.AvcodecClose(m_context);
}

void H263_Base_EncoderContext::Lock()
{
  m_mutex.Wait();
}

void H263_Base_EncoderContext::Unlock()
{
  m_mutex.Signal();
}


bool H263_Base_EncoderContext::EncodeFrames(const BYTE * src, unsigned & srcLen, BYTE * dst, unsigned & dstLen, unsigned int & flags)
{
  WaitAndSignal m(m_mutex);

  if (m_codec == NULL) {
    PTRACE(1, m_prefix, "Encoder did not open");
    return false;
  }

  // create RTP frame from source buffer
  RTPFrame srcRTP(src, srcLen);

  // create RTP frame from destination buffer
  RTPFrame dstRTP(dst, dstLen);
  dstLen = 0;

  // if still running out packets from previous frame, then return it
  if (!m_packetizer->GetPacket(dstRTP, flags)) {
    PluginCodec_Video_FrameHeader * header = (PluginCodec_Video_FrameHeader *)srcRTP.GetPayloadPtr();
    if (header->x != 0 || header->y != 0) {
      PTRACE(2, m_prefix, "Video grab of partial frame unsupported, closing down video transmission thread.");
      return 0;
    }

    // if this is the first frame, or the frame size has changed, deal wth it
    if (m_context->width !=  (int)header->width || m_context->height != (int)header->height) {
      PTRACE(3, m_prefix, "Resolution has changed - reopening codec");
      CloseCodec();
      FFMPEGLibraryInstance.AvSetDimensions(m_context, header->width, header->height);
      if (!OpenCodec()) {
        PTRACE(1, m_prefix, "Reopening codec failed");
        return false;
      }
    }

    if (!m_packetizer->Reset(header->width, header->height)) {
      PTRACE(1, m_prefix, "Unable to allocate memory for packet buffer");
      return 0;
    }

    // Need to copy to local buffer to guarantee 16 byte alignment
    memcpy(m_inputFrame->data[0], OPAL_VIDEO_FRAME_DATA_PTR(header), header->width*header->height*3/2);
    m_inputFrame->pict_type = (flags & PluginCodec_CoderForceIFrame) ? FF_I_TYPE : AV_PICTURE_TYPE_NONE;

    /*
    m_inputFrame->pts = (int64_t)srcRTP.GetTimestamp()*m_context->time_base.den/m_context->time_base.num/VIDEO_CLOCKRATE;

    It would be preferable to use the above line which would get the correct bit rate if
    the grabber is actually slower that the desired frame rate. But due to rounding
    errors, this could end up with two consecutive frames with the same pts and FFMPEG
    then spits the dummy and returns -1 error, which kills the stream. Sigh.

    So, we just assume the frame rate is actually correct and use it for bit rate control.
    */
    m_inputFrame->pts = AV_NOPTS_VALUE;

    int encodedLen = FFMPEGLibraryInstance.AvcodecEncodeVideo(m_context,
                                                              m_packetizer->GetBuffer(),
                                                              m_packetizer->GetMaxSize(),
                                                              m_inputFrame);  
    if (encodedLen < 0) {
      PTRACE(1, m_prefix, "Encoder failed");
      return false;
    }

    if (encodedLen == 0) {
      PTRACE(1, m_prefix, "Encoder returned no data");
      dstRTP.SetPayloadSize(0);
      dstLen = dstRTP.GetHeaderSize();
      flags |= PluginCodec_ReturnCoderLastFrame;
      return true;
    }

    // push the encoded frame through the m_packetizer
    if (!m_packetizer->SetLength(encodedLen)) {
      PTRACE(1, m_prefix, "Packetizer failed");
      return false;
    }

    // return the first encoded block of data
    if (!m_packetizer->GetPacket(dstRTP, flags)) {
      PTRACE(1, m_prefix, "Packetizer failed");
      return false; // Huh?
    }
  }

  dstRTP.SetTimestamp(srcRTP.GetTimestamp());

  CODEC_TRACER_RTP("Tx frame:", dstRTP, RFC2190Dump);
  dstLen = dstRTP.GetHeaderSize() + dstRTP.GetPayloadSize();

  return true;
}


/////////////////////////////////////////////////////////////////////////////

H263_RFC2190_EncoderContext::H263_RFC2190_EncoderContext()
  : H263_Base_EncoderContext("H.263-RFC2190", new RFC2190Packetizer())
{
}


//s->avctx->rtp_callback(s->avctx, s->ptr_lastgob, current_packet_size, number_mb)
void H263_RFC2190_EncoderContext::RTPCallBack(AVCodecContext *avctx, void * data, int size, int mb_nb)
{
  ((RFC2190Packetizer *)((H263_RFC2190_EncoderContext *)avctx->opaque)->m_packetizer)->RTPCallBack(data, size, mb_nb);
}


bool H263_RFC2190_EncoderContext::Init()
{
  if (!H263_Base_EncoderContext::Init(CODEC_ID_H263))
    return false;

#if LIBAVCODEC_RTP_MODE
  m_context->rtp_mode = 1;
#endif

  m_context->rtp_payload_size = PluginCodec_RTP_MaxPayloadSize;
  m_context->rtp_callback = &H263_RFC2190_EncoderContext::RTPCallBack;
  m_context->opaque = this; // used to separate out packets from different encode threads

  m_context->flags &= ~CODEC_FLAG_H263P_UMV;
  m_context->flags &= ~CODEC_FLAG_4MV;
#if LIBAVCODEC_RTP_MODE
  m_context->flags &= ~CODEC_FLAG_H263P_AIC;
#endif
  m_context->flags &= ~CODEC_FLAG_H263P_AIV;
  m_context->flags &= ~CODEC_FLAG_H263P_SLICE_STRUCT;

  return true;
}


/////////////////////////////////////////////////////////////////////////////

H263_RFC2429_EncoderContext::H263_RFC2429_EncoderContext()
  : H263_Base_EncoderContext("H.263-RFC2429", new RFC2429Frame)
{
}

H263_RFC2429_EncoderContext::~H263_RFC2429_EncoderContext()
{
}


bool H263_RFC2429_EncoderContext::Init()
{
  return H263_Base_EncoderContext::Init(CODEC_ID_H263P);
}


/////////////////////////////////////////////////////////////////////////////

Packetizer::Packetizer()
  : m_maxPayloadSize(PluginCodec_RTP_MaxPayloadSize)
{
}


/////////////////////////////////////////////////////////////////////////////

H263_Base_DecoderContext::H263_Base_DecoderContext(const char * prefix, Depacketizer * depacketizer)
  : m_prefix(prefix)
  , m_codec(NULL)
  , m_context(NULL)
  , m_outputFrame(NULL)
  , m_depacketizer(depacketizer)
{
  if (!FFMPEGLibraryInstance.Load())
    return;

  if ((m_codec = FFMPEGLibraryInstance.AvcodecFindDecoder(CODEC_ID_H263)) == NULL) {
    PTRACE(1, m_prefix, "Codec not found for decoder");
    return;
  }

  m_context = FFMPEGLibraryInstance.AvcodecAllocContext();
  if (m_context == NULL) {
    PTRACE(1, m_prefix, "Failed to allocate context for decoder");
    return;
  }

  m_outputFrame = FFMPEGLibraryInstance.AvcodecAllocFrame();
  if (m_outputFrame == NULL) {
    PTRACE(1, m_prefix, "Failed to allocate frame for decoder");
    return;
  }

  // debugging flags
#if PLUGINCODEC_TRACING
  if (PTRACE_CHECK(4))
    m_context->debug |= FF_DEBUG_ER;
  if (PTRACE_CHECK(5))
    m_context->debug |= FF_DEBUG_PICT_INFO;
  if (PTRACE_CHECK(6))
    m_context->debug |= FF_DEBUG_BUGS | FF_DEBUG_BUFFERS;
#endif

  m_depacketizer->NewFrame();

  PTRACE(4, m_prefix, "Decoder created");
}

H263_Base_DecoderContext::~H263_Base_DecoderContext()
{
  CloseCodec();

  if (m_context != NULL)
    FFMPEGLibraryInstance.AvcodecFree(m_context);
  if (m_outputFrame != NULL)
    FFMPEGLibraryInstance.AvcodecFree(m_outputFrame);

  delete m_depacketizer;
}

bool H263_Base_DecoderContext::OpenCodec()
{
  if (m_codec == NULL || m_context == NULL || m_outputFrame == NULL) {
    PTRACE(1, m_prefix, "Codec not initialized");
    return 0;
  }

  if (FFMPEGLibraryInstance.AvcodecOpen(m_context, m_codec) < 0) {
    PTRACE(1, m_prefix, "Failed to open H.263 decoder");
    return false;
  }

  PTRACE(4, m_prefix, "Codec opened");

  return true;
}

void H263_Base_DecoderContext::CloseCodec()
{
  if (m_context != NULL) {
    if (m_context->codec != NULL) {
      FFMPEGLibraryInstance.AvcodecClose(m_context);
      PTRACE(4, m_prefix, "Closed decoder" );
    }
  }
}

bool H263_Base_DecoderContext::DecodeFrames(const BYTE * src, unsigned & srcLen, BYTE * dst, unsigned & dstLen, unsigned int & flags)
{
  // create RTP frame from source buffer
  RTPFrame srcRTP(src, srcLen);

  CODEC_TRACER_RTP("Tx frame:", srcRTP, DumpPacket);

  // create RTP frame from destination buffer
  RTPFrame dstRTP(dst, dstLen, 0);
  dstLen = 0;

  if (!m_depacketizer->AddPacket(srcRTP)) {
    m_depacketizer->NewFrame();
    flags = PluginCodec_ReturnCoderRequestIFrame;
    return true;
  }
  
  if (!srcRTP.GetMarker())
    return true;

  if (!m_depacketizer->IsValid()) {
    m_depacketizer->NewFrame();
    PTRACE(4, m_prefix, "Got an invalid frame - skipping");
    flags = PluginCodec_ReturnCoderRequestIFrame;
    return true;
  }

  if (m_depacketizer->IsIntraFrame())
    flags |= PluginCodec_ReturnCoderIFrame;

#if FFMPEG_HAS_DECODE_ERROR_COUNT
  unsigned error_before = m_context->decode_error_count;
#endif

  PTRACE(5, m_prefix, "Decoding " << m_depacketizer->GetLength()  << " bytes");
  int gotPicture = 0;
  int bytesDecoded = FFMPEGLibraryInstance.AvcodecDecodeVideo(m_context,
                                                              m_outputFrame,
                                                              &gotPicture,
                                                              m_depacketizer->GetBuffer(),
                                                              m_depacketizer->GetLength());

  m_depacketizer->NewFrame();

  // if error occurred, tell the other end to send another I-frame and hopefully we can resync
  if (bytesDecoded < 0
#if FFMPEG_HAS_DECODE_ERROR_COUNT
      || error_before != m_context->decode_error_count
#endif
  ) {
    flags = PluginCodec_ReturnCoderRequestIFrame;
    return true;
  }

  if (!gotPicture || bytesDecoded == 0) {
    PTRACE(3, m_prefix, "Decoded "<< bytesDecoded << " bytes without getting a Picture"); 
    return true;
  }

  PTRACE(5, m_prefix, "Decoded " << bytesDecoded << " bytes"<< ", Resolution: " << m_context->width << "x" << m_context->height);

  // if decoded frame size is not legal, request an I-Frame
  if (m_context->width <= 0 || m_context->height <= 0) {
    PTRACE(1, m_prefix, "Received frame with invalid size");
    flags = PluginCodec_ReturnCoderRequestIFrame;
    return true;
  }

  size_t frameBytes = (m_context->width * m_context->height * 12) / 8;
  if (dstRTP.GetPayloadSize() - sizeof(PluginCodec_Video_FrameHeader) < frameBytes) {
    PTRACE(2, m_prefix, "Destination buffer size " << dstRTP.GetPayloadSize() << " too small for frame of size " << m_context->width  << "x" <<  m_context->height);
    flags = PluginCodec_ReturnCoderBufferTooSmall;
    return true;
  }

  PluginCodec_Video_FrameHeader * header = (PluginCodec_Video_FrameHeader *)dstRTP.GetPayloadPtr();
  header->x = header->y = 0;
  header->width = m_context->width;
  header->height = m_context->height;
  int size = m_context->width * m_context->height;
  if (m_outputFrame->data[1] == m_outputFrame->data[0] + size
      && m_outputFrame->data[2] == m_outputFrame->data[1] + (size >> 2)) {
    memcpy(OPAL_VIDEO_FRAME_DATA_PTR(header), m_outputFrame->data[0], frameBytes);
  } else {
    BYTE *dst = OPAL_VIDEO_FRAME_DATA_PTR(header);
    for (int i=0; i<3; i ++) {
      BYTE *src = m_outputFrame->data[i];
      int dst_stride = i ? m_context->width >> 1 : m_context->width;
      int src_stride = m_outputFrame->linesize[i];
      int h = i ? m_context->height >> 1 : m_context->height;

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

  flags |= PluginCodec_ReturnCoderLastFrame;

  return true;
}


///////////////////////////////////////////////////////////////////////////////////

H263_RFC2429_DecoderContext::H263_RFC2429_DecoderContext()
  : H263_Base_DecoderContext("H.263-RFC2429", new RFC2429Frame)
{
}

/////////////////////////////////////////////////////////////////////////////

H263_RFC2190_DecoderContext::H263_RFC2190_DecoderContext()
  : H263_Base_DecoderContext("H.263-RFC2190", new RFC2190Depacketizer)
{
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


/////////////////////////////////////////////////////////////////////////////

static void * create_encoder(const struct PluginCodec_Definition * codec)
{
  H263_Base_EncoderContext * context;

  if (strcmp(codec->destFormat, h263Desc) == 0)
    context = new H263_RFC2190_EncoderContext();
  else
    context = new H263_RFC2429_EncoderContext();

  if (context->Init()) 
    return context;

  delete context;
  return NULL;
}

static void destroy_encoder(const struct PluginCodec_Definition * /*codec*/, void * context)
{
  delete (H263_Base_EncoderContext *)context;
}

static int codec_encoder(const struct PluginCodec_Definition * , 
                                           void * context,
                                     const void * from, 
                                       unsigned * fromLen,
                                           void * to,
                                       unsigned * toLen,
                                   unsigned int * flag)
{
  return ((H263_Base_EncoderContext *)context)->EncodeFrames((const BYTE *)from, *fromLen, (BYTE *)to, *toLen, *flag);
}

#define PMAX(a,b) ((a)>=(b)?(a):(b))
#define PMIN(a,b) ((a)<=(b)?(a):(b))

static void FindBoundingBox(const char * const * * parm, 
                                             int * mpi,
                                             int & minWidth,
                                             int & minHeight,
                                             int & maxWidth,
                                             int & maxHeight,
                                             int & frameTime,
                                             int & targetBitRate,
                                             int & maxBitRate)
{
  // initialise the MPI values to disabled
  int i;
  for (i = 0; i < 5; i++)
    mpi[i] = PLUGINCODEC_MPI_DISABLED;

  // following values will be set while scanning for options
  minWidth      = INT_MAX;
  minHeight     = INT_MAX;
  maxWidth      = 0;
  maxHeight     = 0;
  int rxMinWidth    = QCIF_WIDTH;
  int rxMinHeight   = QCIF_HEIGHT;
  int rxMaxWidth    = QCIF_WIDTH;
  int rxMaxHeight   = QCIF_HEIGHT;
  int frameRate     = 10;      // 10 fps
  int origFrameTime = 900;     // 10 fps in video RTP timestamps
  int maxBR = 0;
  maxBitRate = 0;
  targetBitRate = 0;

  // extract the MPI values set in the custom options, and find the min/max of them
  frameTime = 0;

  for (const char * const * option = *parm; *option != NULL; option += 2) {
    if (STRCMPI(option[0], "MaxBR") == 0)
      maxBR = atoi(option[1]);
    else if (STRCMPI(option[0], PLUGINCODEC_OPTION_MAX_BIT_RATE) == 0)
      maxBitRate = atoi(option[1]);
    else if (STRCMPI(option[0], PLUGINCODEC_OPTION_TARGET_BIT_RATE) == 0)
      targetBitRate = atoi(option[1]);
    else if (STRCMPI(option[0], PLUGINCODEC_OPTION_MIN_RX_FRAME_WIDTH) == 0)
      rxMinWidth  = atoi(option[1]);
    else if (STRCMPI(option[0], PLUGINCODEC_OPTION_MIN_RX_FRAME_HEIGHT) == 0)
      rxMinHeight = atoi(option[1]);
    else if (STRCMPI(option[0], PLUGINCODEC_OPTION_MAX_RX_FRAME_WIDTH) == 0)
      rxMaxWidth  = atoi(option[1]);
    else if (STRCMPI(option[0], PLUGINCODEC_OPTION_MAX_RX_FRAME_HEIGHT) == 0)
      rxMaxHeight = atoi(option[1]);
    else if (STRCMPI(option[0], PLUGINCODEC_OPTION_FRAME_TIME) == 0)
      origFrameTime = atoi(option[1]);
    else {
      for (i = 0; i < 5; i++) {
        if (STRCMPI(option[0], StandardVideoSizes[i].optionName) == 0) {
          mpi[i] = atoi(option[1]);
          if (mpi[i] != PLUGINCODEC_MPI_DISABLED) {
            int thisTime = 3003*mpi[i];
            if (minWidth > StandardVideoSizes[i].width)
              minWidth = StandardVideoSizes[i].width;
            if (minHeight > StandardVideoSizes[i].height)
              minHeight = StandardVideoSizes[i].height;
            if (maxWidth < StandardVideoSizes[i].width)
              maxWidth = StandardVideoSizes[i].width;
            if (maxHeight < StandardVideoSizes[i].height)
              maxHeight = StandardVideoSizes[i].height;
            if (thisTime > frameTime)
              frameTime = thisTime;
          }
        }
      }
    }
  }

  // if no MPIs specified, then the spec says to use QCIF
  if (frameTime == 0) {
    int ft;
    if (frameRate != 0) 
      ft = 90000 / frameRate;
    else 
      ft = origFrameTime;
    mpi[1] = (ft + 1502) / 3003;

#ifdef DEFAULT_TO_FULL_CAPABILITIES
    minWidth  = QCIF_WIDTH;
    maxWidth  = CIF16_WIDTH;
    minHeight = QCIF_HEIGHT;
    maxHeight = CIF16_HEIGHT;
#else
    minWidth  = maxWidth  = QCIF_WIDTH;
    minHeight = maxHeight = QCIF_HEIGHT;
#endif
  }

  // find the smallest MPI size that is larger than the min frame size
  for (i = 0; i < 5; i++) {
    if (StandardVideoSizes[i].width >= rxMinWidth && StandardVideoSizes[i].height >= rxMinHeight) {
      rxMinWidth = StandardVideoSizes[i].width;
      rxMinHeight = StandardVideoSizes[i].height;
      break;
    }
  }

  // find the largest MPI size that is smaller than the max frame size
  for (i = 4; i >= 0; i--) {
    if (StandardVideoSizes[i].width <= rxMaxWidth && StandardVideoSizes[i].height <= rxMaxHeight) {
      rxMaxWidth  = StandardVideoSizes[i].width;
      rxMaxHeight = StandardVideoSizes[i].height;
      break;
    }
  }

  // the final min/max is the smallest bounding box that will enclose both the MPI information and the min/max information
  minWidth  = PMAX(rxMinWidth, minWidth);
  maxWidth  = PMIN(rxMaxWidth, maxWidth);
  minHeight = PMAX(rxMinHeight, minHeight);
  maxHeight = PMIN(rxMaxHeight, maxHeight);

  // turn off any MPI that are outside the final bounding box
  for (i = 0; i < 5; i++) {
    if (StandardVideoSizes[i].width < minWidth || 
        StandardVideoSizes[i].width > maxWidth ||
        StandardVideoSizes[i].height < minHeight || 
        StandardVideoSizes[i].height > maxHeight)
     mpi[i] = PLUGINCODEC_MPI_DISABLED;
  }

  // find an appropriate max bit rate
  if (maxBitRate == 0) {
    if (maxBR != 0)
      maxBitRate = maxBR * 100;
    else if (targetBitRate != 0)
      maxBitRate = targetBitRate;
    else
      maxBitRate = 327000;
  }
  else if (maxBR > 0)
    maxBitRate = PMIN(maxBR * 100, maxBitRate);

  if (targetBitRate == 0)
    targetBitRate = 327000;
}

static int to_normalised_options(const struct PluginCodec_Definition *, void *, const char *, void * parm, unsigned * parmLen)
{
  if (parmLen == NULL || parm == NULL || *parmLen != sizeof(char ***))
    return 0;

  // find bounding box enclosing all MPI values
  int mpi[5];
  int minWidth, minHeight, maxHeight, maxWidth, frameTime, targetBitRate, maxBitRate;
  FindBoundingBox((const char * const * *)parm, mpi, minWidth, minHeight, maxWidth, maxHeight, frameTime, targetBitRate, maxBitRate);

  char ** options = (char **)calloc(16+(5*2)+2, sizeof(char *));
  *(char ***)parm = options;
  if (options == NULL)
    return 0;

  options[ 0] = strdup(PLUGINCODEC_OPTION_MIN_RX_FRAME_WIDTH);
  options[ 1] = num2str(minWidth);
  options[ 2] = strdup(PLUGINCODEC_OPTION_MIN_RX_FRAME_HEIGHT);
  options[ 3] = num2str(minHeight);
  options[ 4] = strdup(PLUGINCODEC_OPTION_MAX_RX_FRAME_WIDTH);
  options[ 5] = num2str(maxWidth);
  options[ 6] = strdup(PLUGINCODEC_OPTION_MAX_RX_FRAME_HEIGHT);
  options[ 7] = num2str(maxHeight);
  options[ 8] = strdup(PLUGINCODEC_OPTION_FRAME_TIME);
  options[ 9] = num2str(frameTime);
  options[10] = strdup(PLUGINCODEC_OPTION_MAX_BIT_RATE);
  options[11] = num2str(maxBitRate);
  options[12] = strdup(PLUGINCODEC_OPTION_TARGET_BIT_RATE);
  options[13] = num2str(targetBitRate);
  options[14] = strdup("MaxBR");
  options[15] = num2str((maxBitRate+50)/100);
  for (int i = 0; i < 5; i++) {
    options[16+i*2] = strdup(StandardVideoSizes[i].optionName);
    options[16+i*2+1] = num2str(mpi[i]);
  }

  return 1;
}

static int to_customised_options(const struct PluginCodec_Definition *, void *, const char *, void * parm, unsigned * parmLen)
{
  if (parmLen == NULL || parm == NULL || *parmLen != sizeof(char ***))
    return 0;

  // find bounding box enclosing all MPI values
  int mpi[5];
  int minWidth, minHeight, maxHeight, maxWidth, frameTime, targetBitRate, maxBitRate;
  FindBoundingBox((const char * const * *)parm, mpi, minWidth, minHeight, maxWidth, maxHeight, frameTime, targetBitRate, maxBitRate);

  char ** options = (char **)calloc(14+5*2+2, sizeof(char *));
  *(char ***)parm = options;
  if (options == NULL)
    return 0;

  options[ 0] = strdup(PLUGINCODEC_OPTION_MIN_RX_FRAME_WIDTH);
  options[ 1] = num2str(minWidth);
  options[ 2] = strdup(PLUGINCODEC_OPTION_MIN_RX_FRAME_HEIGHT);
  options[ 3] = num2str(minHeight);
  options[ 4] = strdup(PLUGINCODEC_OPTION_MAX_RX_FRAME_WIDTH);
  options[ 5] = num2str(maxWidth);
  options[ 6] = strdup(PLUGINCODEC_OPTION_MAX_RX_FRAME_HEIGHT);
  options[ 7] = num2str(maxHeight);
  options[ 8] = strdup(PLUGINCODEC_OPTION_MAX_BIT_RATE);
  options[ 9] = num2str(maxBitRate);
  options[10] = strdup(PLUGINCODEC_OPTION_TARGET_BIT_RATE);
  options[11] = num2str(targetBitRate);
  options[12] = strdup("MaxBR");
  options[13] = num2str((maxBitRate+50)/100);
  for (int i = 0; i < 5; i++) {
    options[14+i*2] = strdup(StandardVideoSizes[i].optionName);
    options[14+i*2+1] = num2str(mpi[i]);
  }

  return 1;
}

static int encoder_set_options(const PluginCodec_Definition *, 
                               void * context,
                               const char * , 
                               void * parm, 
                               unsigned * parmLen)
{
  if (parmLen == NULL || *parmLen != sizeof(const char **) || parm == NULL)
    return 0;

  return ((H263_Base_EncoderContext *)context)->SetOptions((const char * const *)parm);
}

static int encoder_get_output_data_size(const PluginCodec_Definition *, void *, const char *, void *, unsigned *)
{
  return PluginCodec_RTP_MaxPayloadSize;
}


static bool GetCustomMPI(const char * str,
                         unsigned width[MAX_H263_CUSTOM_SIZES],
                         unsigned height[MAX_H263_CUSTOM_SIZES],
                         unsigned mpi[MAX_H263_CUSTOM_SIZES],
                         size_t & count)
{
  count = 0;
  for (;;) {
    width[count] = height[count] = mpi[count] = 0;

    char * end;
    width[count] = strtoul(str, &end, 10);
    if (*end != ',')
      return false;

    str = end+1;
    height[count] = strtoul(str, &end, 10);
    if (*end != ',')
      return false;

    str = end+1;
    mpi[count] = strtoul(str, &end, 10);
    if (mpi[count] == 0 || mpi[count] > PLUGINCODEC_MPI_DISABLED)
      return false;

    if (mpi[count] < PLUGINCODEC_MPI_DISABLED && (width[count] < 16 || height[count] < 16))
      return false;

    if (mpi[count] == 0 || mpi[count] > PLUGINCODEC_MPI_DISABLED)
      return false;
    ++count;
    if (count >= MAX_H263_CUSTOM_SIZES || *end != ';')
      return true;

    str = end+1;
  }
}


static int MergeCustomH263(char ** result, const char * dest, const char * src)
{
  size_t srcCount;
  unsigned srcWidth[MAX_H263_CUSTOM_SIZES], srcHeight[MAX_H263_CUSTOM_SIZES], srcMPI[MAX_H263_CUSTOM_SIZES];
  if (!GetCustomMPI(src, srcWidth, srcHeight, srcMPI, srcCount)) {
    PTRACE(2, "IPP-H.263", "Invalid source custom MPI format \"" << src << '"');
    return false;
  }

  size_t dstCount;
  unsigned dstWidth[MAX_H263_CUSTOM_SIZES], dstHeight[MAX_H263_CUSTOM_SIZES], dstMPI[MAX_H263_CUSTOM_SIZES];
  if (!GetCustomMPI(dest, dstWidth, dstHeight, dstMPI, dstCount)) {
    PTRACE(2, "IPP-H.263", "Invalid destination custom MPI format \"" << dest << '"');
    return false;
  }

  size_t resultCount = 0;
  unsigned resultWidth[MAX_H263_CUSTOM_SIZES];
  unsigned resultHeight[MAX_H263_CUSTOM_SIZES];
  unsigned resultMPI[MAX_H263_CUSTOM_SIZES];

  for (size_t s = 0; s < srcCount; ++s) {
    for (size_t d = 0; d < dstCount; ++d) {
      if (srcWidth[s] == dstWidth[d] && srcHeight[s] == dstHeight[d]) {
        resultWidth[resultCount] = srcWidth[s];
        resultHeight[resultCount] = srcHeight[s];
        resultMPI[resultCount] = std::max(srcMPI[s], dstMPI[d]);
        ++resultCount;
      }
    }
  }

  if (resultCount == 0)
    *result = strdup(DEFAULT_CUSTOM_MPI);
  else {
    size_t len = 0;
    char buffer[MAX_H263_CUSTOM_SIZES*20];
    for (size_t i = 0; i < resultCount; ++i)
      len += sprintf(&buffer[len], len == 0 ? "%u,%u,%u" : ";%u,%u,%u", resultWidth[i], resultHeight[i], resultMPI[i]);
    *result = strdup(buffer);
  }

  return true;
}


static void FreeString(char * str)
{
  free(str);
}


/////////////////////////////////////////////////////////////////////////////

static void * create_decoder(const struct PluginCodec_Definition * codec)
{
  H263_Base_DecoderContext * decoder;
  if (strcmp(codec->sourceFormat, h263Desc) == 0)
    decoder = new H263_RFC2190_DecoderContext();
  else
    decoder = new H263_RFC2429_DecoderContext();

  if (decoder->OpenCodec()) // decoder will re-initialise context with correct frame size
    return decoder;

  delete decoder;
  return NULL;
}

static void destroy_decoder(const struct PluginCodec_Definition * /*codec*/, void * context)
{
  delete (H263_Base_DecoderContext *)context;
}

static int codec_decoder(const struct PluginCodec_Definition *, 
                                           void * context,
                                     const void * from, 
                                       unsigned * fromLen,
                                           void * to, 
                                       unsigned * toLen,
                                   unsigned int * flag)
{
  return ((H263_Base_DecoderContext *)context)->DecodeFrames((const BYTE *)from, *fromLen, (BYTE *)to, *toLen, *flag) ? 1 : 0;
}

static int decoder_get_output_data_size(const PluginCodec_Definition * codec, void *, const char *, void *, unsigned *)
{
  return sizeof(PluginCodec_Video_FrameHeader) + ((codec->parm.video.maxFrameWidth * codec->parm.video.maxFrameHeight * 3) / 2);
}

PLUGINCODEC_CONTROL_LOG_FUNCTION_DEF

/////////////////////////////////////////////////////////////////////////////

static struct PluginCodec_information licenseInfo = {
  1145863600,                                                   // timestamp =  Mon 24 Apr 2006 07:26:40 AM UTC

  "Matthias Schneider, Craig Southeren"                         // source code author
  "Guilhem Tardy, Derek Smithies",
  "1.0",                                                        // source code version
  "openh323@openh323.org",                                      // source code email
  "http://sourceforge.net/projects/openh323",                   // source code URL
  "Copyright (C) 2007 Matthias Schneider"                       // source code copyright
  ", Copyright (C) 2006 by Post Increment"
  ", Copyright (C) 2005 Salyens"
  ", Copyright (C) 2001 March Networks Corporation"
  ", Copyright (C) 1999-2000 Equivalence Pty. Ltd.",
  "MPL 1.0",                                                    // source code license
  PluginCodec_License_MPL,                                      // source code license
  
  "FFMPEG",                                                     // codec description
  "Michael Niedermayer, Fabrice Bellard",                       // codec author
  "",                                                           // codec version
  "ffmpeg-devel-request@mplayerhq.hu",                          // codec email
  "http://ffmpeg.mplayerhq.hu",                                 // codec URL
  "Copyright (c) 2000-2001 Fabrice Bellard"                     // codec copyright information
  ", Copyright (c) 2002-2003 Michael Niedermayer",
  "GNU LESSER GENERAL PUBLIC LICENSE, Version 2.1, February 1999", // codec license
  PluginCodec_License_LGPL                                         // codec license code
};

static const char SQCIF_MPI[]  = PLUGINCODEC_SQCIF_MPI;
static const char QCIF_MPI[]   = PLUGINCODEC_QCIF_MPI;
static const char CIF_MPI[]    = PLUGINCODEC_CIF_MPI;
static const char CIF4_MPI[]   = PLUGINCODEC_CIF4_MPI;
static const char CIF16_MPI[]  = PLUGINCODEC_CIF16_MPI;

static PluginCodec_ControlDefn EncoderControls[] = {
  { PLUGINCODEC_CONTROL_GET_CODEC_OPTIONS,     get_codec_options },
  { PLUGINCODEC_CONTROL_FREE_CODEC_OPTIONS,    free_codec_options },
  { PLUGINCODEC_CONTROL_TO_NORMALISED_OPTIONS, to_normalised_options },
  { PLUGINCODEC_CONTROL_TO_CUSTOMISED_OPTIONS, to_customised_options },
  { PLUGINCODEC_CONTROL_SET_CODEC_OPTIONS,     encoder_set_options },
  { PLUGINCODEC_CONTROL_GET_OUTPUT_DATA_SIZE,  encoder_get_output_data_size },
  PLUGINCODEC_CONTROL_LOG_FUNCTION_INC
  { NULL }
};

static PluginCodec_ControlDefn DecoderControls[] = {
  { PLUGINCODEC_CONTROL_GET_CODEC_OPTIONS,     get_codec_options },
  { PLUGINCODEC_CONTROL_GET_OUTPUT_DATA_SIZE,  decoder_get_output_data_size },
  { NULL }
};

static struct PluginCodec_Option const sqcifMPI =
{
  PluginCodec_IntegerOption,            // Option type
  SQCIF_MPI,                            // User visible name
  false,                                // User Read/Only flag
  PluginCodec_MaxMerge,                 // Merge mode
  "1",                                  // Initial value
  "SQCIF",                              // FMTP option name
  STRINGIZE(PLUGINCODEC_MPI_DISABLED),  // FMTP default value
  0,                                    // H.245 generic capability code and bit mask
  "1",                                  // Minimum value
  STRINGIZE(PLUGINCODEC_MPI_DISABLED)   // Maximum value
};

static struct PluginCodec_Option const qcifMPI =
{
  PluginCodec_IntegerOption,            // Option type
  QCIF_MPI,                             // User visible name
  false,                                // User Read/Only flag
  PluginCodec_MaxMerge,                 // Merge mode
  "1",                                  // Initial value
  "QCIF",                               // FMTP option name
  STRINGIZE(PLUGINCODEC_MPI_DISABLED),  // FMTP default value
  0,                                    // H.245 generic capability code and bit mask
  "1",                                  // Minimum value
  STRINGIZE(PLUGINCODEC_MPI_DISABLED)   // Maximum value
};

static struct PluginCodec_Option const cifMPI =
{
  PluginCodec_IntegerOption,            // Option type
  CIF_MPI,                              // User visible name
  false,                                // User Read/Only flag
  PluginCodec_MaxMerge,                 // Merge mode
  "1",                                  // Initial value
  "CIF",                                // FMTP option name
  STRINGIZE(PLUGINCODEC_MPI_DISABLED),  // FMTP default value
  0,                                    // H.245 generic capability code and bit mask
  "1",                                  // Minimum value
  STRINGIZE(PLUGINCODEC_MPI_DISABLED)   // Maximum value
};

static struct PluginCodec_Option const cif4MPI =
{
  PluginCodec_IntegerOption,            // Option type
  CIF4_MPI,                             // User visible name
  false,                                // User Read/Only flag
  PluginCodec_MaxMerge,                 // Merge mode
  "1",                                  // Initial value
  "CIF4",                               // FMTP option name
  STRINGIZE(PLUGINCODEC_MPI_DISABLED),  // FMTP default value
  0,                                    // H.245 generic capability code and bit mask
  "1",                                  // Minimum value
  STRINGIZE(PLUGINCODEC_MPI_DISABLED)   // Maximum value
};

static struct PluginCodec_Option const cif16MPI =
{
  PluginCodec_IntegerOption,            // Option type
  CIF16_MPI,                            // User visible name
  false,                                // User Read/Only flag
  PluginCodec_MaxMerge,                 // Merge mode
  "1",                                  // Initial value
  "CIF16",                              // FMTP option name
  STRINGIZE(PLUGINCODEC_MPI_DISABLED),  // FMTP default value
  0,                                    // H.245 generic capability code and bit mask
  "1",                                  // Minimum value
  STRINGIZE(PLUGINCODEC_MPI_DISABLED)   // Maximum value
};

static struct PluginCodec_Option const maxBR =
{
  PluginCodec_IntegerOption,          // Option type
  "MaxBR",                            // User visible name
  false,                              // User Read/Only flag
  PluginCodec_MinMerge,               // Merge mode
  "0",                                // Initial value
  "maxbr",                            // FMTP option name
  "0",                                // FMTP default value
  0,                                  // H.245 generic capability code and bit mask
  "0",                                // Minimum value
  "32767"                             // Maximum value
};

static struct PluginCodec_Option const mediaPacketization =
{
  PluginCodec_StringOption,           // Option type
  PLUGINCODEC_MEDIA_PACKETIZATION,    // User visible name
  true,                               // User Read/Only flag
  PluginCodec_EqualMerge,             // Merge mode
  "RFC2190"                           // Initial value
};

static struct PluginCodec_Option const mediaPacketizationPlus =
{
  PluginCodec_StringOption,           // Option type
  PLUGINCODEC_MEDIA_PACKETIZATION,    // User visible name
  true,                               // User Read/Only flag
  PluginCodec_EqualMerge,             // Merge mode
  "RFC2429"                           // Initial value
};

static struct PluginCodec_Option const customMPI =
{
  PluginCodec_StringOption,           // Option type
  PLUGINCODEC_CUSTOM_MPI,             // User visible name
  false,                              // User Read/Only flag
  PluginCodec_CustomMerge,            // Merge mode
  DEFAULT_CUSTOM_MPI,                 // Initial value
  "CUSTOM",                           // FMTP option name
  NULL,                               // FMTP default value
  0,                                  // H.245 generic capability code and bit mask
  NULL,                               // Minimum value
  NULL,                               // Maximum value
  MergeCustomH263,
  FreeString
};

static struct PluginCodec_Option const annexF =
  { PluginCodec_BoolOption,    H263_ANNEX_F,   false,  PluginCodec_AndMerge, "1", "F", "0" };

static struct PluginCodec_Option const annexI =
  { PluginCodec_BoolOption,    H263_ANNEX_I,   false,  PluginCodec_AndMerge, "1", "I", "0" };

static struct PluginCodec_Option const annexJ =
  { PluginCodec_BoolOption,    H263_ANNEX_J,   true,  PluginCodec_AndMerge, "1", "J", "0" };

static struct PluginCodec_Option const annexK =
  { PluginCodec_IntegerOption, H263_ANNEX_K,   true,  PluginCodec_MinMerge, "0", "K", "0", 0, "0", "4" };

static struct PluginCodec_Option const annexN =
  { PluginCodec_BoolOption,    H263_ANNEX_N,   true,  PluginCodec_AndMerge, "0", "N", "0" };

static struct PluginCodec_Option const annexT =
  { PluginCodec_BoolOption,    H263_ANNEX_T,   true,  PluginCodec_AndMerge, "0", "T", "0" };

static struct PluginCodec_Option const annexD =
  { PluginCodec_BoolOption,    H263_ANNEX_D,   true,  PluginCodec_AndMerge, "1", "D", "0" };

static struct PluginCodec_Option const TemporalSpatialTradeOff =
{
  PluginCodec_IntegerOption,          // Option type
  PLUGINCODEC_OPTION_TEMPORAL_SPATIAL_TRADE_OFF, // User visible name
  false,                              // User Read/Only flag
  PluginCodec_AlwaysMerge,            // Merge mode
  "31",                               // Initial value
  NULL,                               // FMTP option name
  NULL,                               // FMTP default value
  0,                                  // H.245 generic capability code and bit mask
  "1",                                // Minimum value
  "31"                                // Maximum value
};

static struct PluginCodec_Option const * const h263POptionTable[] = {
  &mediaPacketizationPlus,
  &maxBR,
  &qcifMPI,
  &cifMPI,
  &sqcifMPI,
  &cif4MPI,
  &cif16MPI,
  &customMPI,
  &annexF,
  &annexI,
  &annexJ,
  &annexK,
  &annexN,
  &annexT,
  &annexD,
  &TemporalSpatialTradeOff,
  NULL
};


static struct PluginCodec_Option const * const h263OptionTable[] = {
  &mediaPacketization,
  &maxBR,
  &qcifMPI,
  &cifMPI,
  &sqcifMPI,
  &cif4MPI,
  &cif16MPI,
  &annexF,
  NULL
};

/////////////////////////////////////////////////////////////////////////////

static struct PluginCodec_Definition h263CodecDefn[] = {
{ 
  // All frame sizes (dynamic) encoder
  PLUGIN_CODEC_VERSION_OPTIONS,       // codec API version
  &licenseInfo,                       // license information

  PluginCodec_MediaTypeVideo |        // audio codec
  PluginCodec_RTPTypeShared |         // specified RTP type
  PluginCodec_RTPTypeDynamic,         // specified RTP type

  h263PDesc,                          // text decription
  YUV420PDesc,                        // source format
  h263PDesc,                          // destination format

  h263POptionTable,                   // user data 

  VIDEO_CLOCKRATE,                    // samples per second
  H263_BITRATE,                       // raw bits per second
  20000,                              // nanoseconds per frame

  CIF4_WIDTH,                         // frame width
  CIF4_HEIGHT,                        // frame height
  10,                                 // recommended frame rate
  60,                                 // maximum frame rate
  0,                                  // IANA RTP payload code
  sdpH263P,                           // RTP payload name

  create_encoder,                     // create codec function
  destroy_encoder,                    // destroy codec
  codec_encoder,                      // encode/decode
  EncoderControls,                    // codec controls

  PluginCodec_H323VideoCodec_h263,    // h323CapabilityType 
  NULL                                // h323CapabilityData
},
{ 
  // All frame sizes (dynamic) decoder
  PLUGIN_CODEC_VERSION_OPTIONS,       // codec API version
  &licenseInfo,                       // license information

  PluginCodec_MediaTypeVideo |        // audio codec
  PluginCodec_RTPTypeShared |         // specified RTP type
  PluginCodec_RTPTypeDynamic,         // specified RTP type

  h263PDesc,                          // text decription
  h263PDesc,                          // source format
  YUV420PDesc,                        // destination format

  h263POptionTable,                   // user data 

  VIDEO_CLOCKRATE,                    // samples per second
  H263_BITRATE,                       // raw bits per second
  20000,                              // nanoseconds per frame

  CIF4_WIDTH,                         // frame width
  CIF4_HEIGHT,                        // frame height
  10,                                 // recommended frame rate
  60,                                 // maximum frame rate
  0,                                  // IANA RTP payload code
  sdpH263P,                           // RTP payload name

  create_decoder,                     // create codec function
  destroy_decoder,                    // destroy codec
  codec_decoder,                      // encode/decode
  DecoderControls,                    // codec controls

  PluginCodec_H323VideoCodec_h263,    // h323CapabilityType 
  NULL                                // h323CapabilityData
},

{ 
  // H.263 (RFC 2190) encoder
  PLUGIN_CODEC_VERSION_OPTIONS,       // codec API version
  &licenseInfo,                       // license information

  PluginCodec_MediaTypeVideo |        // video codec
  PluginCodec_MediaTypeExtVideo |     // Extended video codec
  PluginCodec_RTPTypeExplicit,        // specified RTP type

  h263Desc,                           // text decription
  YUV420PDesc,                        // source format
  h263Desc,                           // destination format

  h263OptionTable,                    // user data 

  VIDEO_CLOCKRATE,                    // samples per second
  H263_BITRATE,                       // raw bits per second
  20000,                              // nanoseconds per frame

  {{
    CIF16_WIDTH,                        // frame width
    CIF16_HEIGHT,                       // frame height
    10,                                 // recommended frame rate
    60,                                 // maximum frame rate
  }},

  RTP_RFC2190_PAYLOAD,                // IANA RTP payload code
  sdpH263,                            // RTP payload name

  create_encoder,                     // create codec function
  destroy_encoder,                    // destroy codec
  codec_encoder,                      // encode/decode
  EncoderControls,                    // codec controls

  PluginCodec_H323VideoCodec_h263,    // h323CapabilityType 
  NULL                                // h323CapabilityData
},
{ 
  // H.263 (RFC 2190) decoder
  PLUGIN_CODEC_VERSION_OPTIONS,       // codec API version
  &licenseInfo,                       // license information

  PluginCodec_MediaTypeVideo |        // video codec
  PluginCodec_MediaTypeExtVideo |     // Extended video codec
  PluginCodec_RTPTypeExplicit,        // specified RTP type

  h263Desc,                           // text decription
  h263Desc,                           // source format
  YUV420PDesc,                        // destination format

  h263OptionTable,                    // user data 

  VIDEO_CLOCKRATE,                    // samples per second
  H263_BITRATE,                       // raw bits per second
  20000,                              // nanoseconds per frame

  {{
    CIF16_WIDTH,                        // frame width
    CIF16_HEIGHT,                       // frame height
    10,                                 // recommended frame rate
    60,                                 // maximum frame rate
  }},
  RTP_RFC2190_PAYLOAD,                // IANA RTP payload code
  sdpH263,                            // RTP payload name

  create_decoder,                     // create codec function
  destroy_decoder,                    // destroy codec
  codec_decoder,                      // encode/decode
  DecoderControls,                    // codec controls

  PluginCodec_H323VideoCodec_h263,    // h323CapabilityType 
  NULL                                // h323CapabilityData
}

};

/////////////////////////////////////////////////////////////////////////////

extern "C" {
  PLUGIN_CODEC_IMPLEMENT(FFMPEG_H263P)

  PLUGIN_CODEC_DLL_API struct PluginCodec_Definition * PLUGIN_CODEC_GET_CODEC_FN(unsigned * count, unsigned version)
  {
    if (version < PLUGIN_CODEC_VERSION_OPTIONS) {
      *count = 0;
      return NULL;
    }

    *count = sizeof(h263CodecDefn) / sizeof(struct PluginCodec_Definition);
    return h263CodecDefn;
  }

};
