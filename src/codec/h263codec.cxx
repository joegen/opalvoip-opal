/*
 * h263codec.cxx
 *
 * Open Phone Abstraction Library (OPAL)
 * Formally known as the Open H323 project.
 *
 * Copyright (c) 2005 Salyens
 * Copyright (c) 2001 March Networks Corporation
 * Copyright (c) 1999-2000 Equivalence Pty. Ltd.
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
 * The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * Contributor(s): Guilhem Tardy (gtardy@salyens.com)
 *
 * $Log: h263codec.cxx,v $
 * Revision 1.2.2.1  2006/03/20 02:25:26  csoutheren
 * Backports from CVS head
 *
 * Revision 1.3  2006/03/12 06:36:57  rjongbloed
 * Fixed DevStudio warning
 *
 * Revision 1.2  2006/02/13 03:46:17  csoutheren
 * Added initialisation stuff to make sure that everything works OK
 *
 * Revision 1.1  2006/01/01 19:19:33  dsandras
 * Added RFC2190 H.263 codec thanks to Salyens. Many thanks!
 *
 * Revision 1.1  2005/12/30 12:29:29  guilhem
 * *** empty log message ***
 *
 * Revision 1.1  2005/08/19 22:32:46  gtardy
 * Initial release of h263 codec.
 *
 */

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "h263codec.h"
#endif

#include <codec/h263codec.h>

#ifndef NO_OPAL_VIDEO

#ifdef RFC2190_AVCODEC

#ifndef NO_H323
#include <asn/h245.h>
#endif

extern "C" {
#include <avcodec.h>
extern void avcodec_set_print_fn(void (*print_fn)(char *));
};

#define new PNEW

namespace PWLibStupidLinkerHacks {
  int rfc2190h263Loader;
};

const OpalVideoFormat & GetOpalH263()
{
  static const OpalVideoFormat H263(OPAL_H263, RTP_DataFrame::H263, "H263", 352, 288, 15, 180000);
  return H263;
}

//////////////////////////////////////////////////////////////////////////////

static void h263_ffmpeg_printon(char * str)
{
  if (str) {
    PTRACE(3, "FFMPEG\t" << str);
  }
}

//////////////////////////////////////////////////////////////////////////////

class FFmpegLink : public PDynaLink
{
  PCLASSINFO(FFmpegLink, PDynaLink)
    
  public:
    FFmpegLink();
    ~FFmpegLink();

    AVCodec *AvcodecFindEncoder(enum CodecID id);
    AVCodec *AvcodecFindDecoder(enum CodecID id);
    AVCodecContext *AvcodecAllocContext(void);
    AVFrame *AvcodecAllocFrame(void);
    int AvcodecOpen(AVCodecContext *ctx, AVCodec *codec);
    int AvcodecClose(AVCodecContext *ctx);
    int AvcodecEncodeVideo(AVCodecContext *ctx, BYTE *buf, int buf_size, const AVFrame *pict);
    int AvcodecDecodeVideo(AVCodecContext *ctx, AVFrame *pict, int *got_picture_ptr, BYTE *buf, int buf_size);
    void AvcodecFree(void * ptr);

    void AvcodecSetPrintFn(void (*print_fn)(char *));

    BOOL IsLoaded();

  protected:
    void (*Favcodec_init)(void);
    AVCodec *Favcodec_h263_encoder;
    AVCodec *Favcodec_h263p_encoder;
    AVCodec *Favcodec_h263_decoder;
    void (*Favcodec_register)(AVCodec *format);
    AVCodec *(*Favcodec_find_encoder)(enum CodecID id);
    AVCodec *(*Favcodec_find_decoder)(enum CodecID id);
    AVCodecContext *(*Favcodec_alloc_context)(void);
    void (*Favcodec_free)(void *);
    AVFrame *(*Favcodec_alloc_frame)(void);
    int (*Favcodec_open)(AVCodecContext *ctx, AVCodec *codec);
    int (*Favcodec_close)(AVCodecContext *ctx);
    int (*Favcodec_encode_video)(AVCodecContext *ctx, BYTE *buf, int buf_size, const AVFrame *pict);
    int (*Favcodec_decode_video)(AVCodecContext *ctx, AVFrame *pict, int *got_picture_ptr, BYTE *buf, int buf_size);

    void (*Favcodec_set_print_fn)(void (*print_fn)(char *));

    PMutex processLock;
    BOOL isLoadedOK;
};

//////////////////////////////////////////////////////////////////////////////

FFmpegLink::FFmpegLink()
{
  isLoadedOK = FALSE;

  if (!PDynaLink::Open("avcodec")
#if defined(P_LINUX) || defined(P_FREEBSD) || defined(P_OPENBSD) || defined(P_NETBSD)
      && !PDynaLink::Open("libavcodec.so")
#endif
    ) {
    cerr << "FFLINK\tFailed to load a library, some codecs won't operate correctly;" << endl;
#if defined(P_LINUX) || defined(P_FREEBSD) || defined(P_OPENBSD) || defined(P_NETBSD)
    cerr << "put libavcodec.so in the current directory (together with this program) and try again" << endl;
#else
    cerr << "put avcodec.dll in the current directory (together with this program) and try again" << endl;
#endif
    return;
  }

  if (!GetFunction("avcodec_init", (Function &)Favcodec_init)) {
    cerr << "Failed to load avcodec_int" << endl;
    return;
  }

  if (!GetFunction("h263_encoder", (Function &)Favcodec_h263_encoder)) {
    cerr << "Failed to load h263_encoder" << endl;
    return;
  }

  if (!GetFunction("h263p_encoder", (Function &)Favcodec_h263p_encoder)) {
    cerr << "Failed to load h263p_encoder" << endl;
    return;
  }

  if (!GetFunction("h263_decoder", (Function &)Favcodec_h263_decoder)) {
    cerr << "Failed to load h263_decoder" << endl;
    return;
  }

  if (!GetFunction("register_avcodec", (Function &)Favcodec_register)) {
    cerr << "Failed to load register_avcodec" << endl;
    return;
  }

  if (!GetFunction("avcodec_find_encoder", (Function &)Favcodec_find_encoder)) {
    cerr << "Failed to load avcodec_find_encoder" << endl;
    return;
  }

  if (!GetFunction("avcodec_find_decoder", (Function &)Favcodec_find_decoder)) {
    cerr << "Failed to load avcodec_find_decoder" << endl;
    return;
  }

  if (!GetFunction("avcodec_alloc_context", (Function &)Favcodec_alloc_context)) {
    cerr << "Failed to load avcodec_alloc_context" << endl;
    return;
  }

  if (!GetFunction("avcodec_alloc_frame", (Function &)Favcodec_alloc_frame)) {
    cerr << "Failed to load avcodec_alloc_frame" << endl;
    return;
  }

  if (!GetFunction("avcodec_open", (Function &)Favcodec_open)) {
    cerr << "Failed to load avcodec_open" << endl;
    return;
  }

  if (!GetFunction("avcodec_close", (Function &)Favcodec_close)) {
    cerr << "Failed to load avcodec_close" << endl;
    return;
  }

  if (!GetFunction("avcodec_encode_video", (Function &)Favcodec_encode_video)) {
    cerr << "Failed to load avcodec_encode_video" << endl;
    return;
  }

  if (!GetFunction("avcodec_decode_video", (Function &)Favcodec_decode_video)) {
    cerr << "Failed to load avcodec_decode_video" << endl;
    return;
  }

  if (!GetFunction("avcodec_set_print_fn", (Function &)Favcodec_set_print_fn)) {
    cerr << "Failed to load avcodec_set_print_fn" << endl;
    return;
  }
   
  if (!GetFunction("av_free", (Function &)Favcodec_free)) {
    cerr << "Failed to load avcodec_close" << endl;
    return;
  }

  // must be called before using avcodec lib
  Favcodec_init();

  // register only the codecs needed (to have smaller code)
  Favcodec_register(Favcodec_h263_encoder);
  Favcodec_register(Favcodec_h263p_encoder);
  Favcodec_register(Favcodec_h263_decoder);
  
  Favcodec_set_print_fn(h263_ffmpeg_printon);

  isLoadedOK = TRUE;
}

FFmpegLink::~FFmpegLink()
{
  PDynaLink::Close();
}

AVCodec *FFmpegLink::AvcodecFindEncoder(enum CodecID id)
{
  AVCodec *res = Favcodec_find_encoder(id);
  PTRACE_IF(6, res, "FFLINK\tFound encoder " << res->name << " @ " << ::hex << (int)res << ::dec);
  return res;
}

AVCodec *FFmpegLink::AvcodecFindDecoder(enum CodecID id)
{
  AVCodec *res = Favcodec_find_decoder(id);
  PTRACE_IF(6, res, "FFLINK\tFound decoder " << res->name << " @ " << ::hex << (int)res << ::dec);
  return res;
}

AVCodecContext *FFmpegLink::AvcodecAllocContext(void)
{
  AVCodecContext *res = Favcodec_alloc_context();
  PTRACE_IF(6, res, "FFLINK\tAllocated context @ " << ::hex << (int)res << ::dec);
  return res;
}

AVFrame *FFmpegLink::AvcodecAllocFrame(void)
{
  AVFrame *res = Favcodec_alloc_frame();
  PTRACE_IF(6, res, "FFLINK\tAllocated frame @ " << ::hex << (int)res << ::dec);
  return res;
}

int FFmpegLink::AvcodecOpen(AVCodecContext *ctx, AVCodec *codec)
{
  PWaitAndSignal m(processLock);

  PTRACE(6, "FFLINK\tNow open context @ " << ::hex << (int)ctx << ", codec @ " << (int)codec << ::dec);
  return Favcodec_open(ctx, codec);
}

int FFmpegLink::AvcodecClose(AVCodecContext *ctx)
{
  PTRACE(6, "FFLINK\tNow close context @ " << ::hex << (int)ctx << ::dec);
  return Favcodec_close(ctx);
}

int FFmpegLink::AvcodecEncodeVideo(AVCodecContext *ctx, BYTE *buf, int buf_size, const AVFrame *pict)
{
  PWaitAndSignal m(processLock);

  PTRACE(6, "FFLINK\tNow encode video for ctxt @ " << ::hex << (int)ctx << ", pict @ " << (int)pict
	 << ", buf @ " << (int)buf << ::dec << " (" << buf_size << " bytes)");
  int res = Favcodec_encode_video(ctx, buf, buf_size, pict);

  PTRACE(6, "FFLINK\tEncoded video into " << res << " bytes");
  return res;
}

int FFmpegLink::AvcodecDecodeVideo(AVCodecContext *ctx, AVFrame *pict, int *got_picture_ptr, BYTE *buf, int buf_size)
{
  PWaitAndSignal m(processLock);

  PTRACE(6, "FFLINK\tNow decode video for ctxt @ " << ::hex << (int)ctx << ", pict @ " << (int)pict
	 << ", buf @ " << (int)buf << ::dec << " (" << buf_size << " bytes)");
  int res = Favcodec_decode_video(ctx, pict, got_picture_ptr, buf, buf_size);

  PTRACE(6, "FFLINK\tDecoded video of " << res << " bytes, got_picture=" << *got_picture_ptr);
  return res;
}

void FFmpegLink::AvcodecSetPrintFn(void (*print_fn)(char *))
{
  Favcodec_set_print_fn(print_fn);
}

void FFmpegLink::AvcodecFree(void * ptr)
{
  Favcodec_free(ptr);
}

BOOL FFmpegLink::IsLoaded()
{
  PWaitAndSignal m(processLock);

  return isLoadedOK;
}

FFmpegLink ff;

//////////////////////////////////////////////////////////////////////////////

#ifndef NO_H323

H323_H263Capability::H323_H263Capability(unsigned _sqcifMPI,
					     unsigned _qcifMPI,
					     unsigned _cifMPI,
					     unsigned _cif4MPI,
					     unsigned _cif16MPI,
					     unsigned _maxBitRate,
					     BOOL _unrestrictedVector,
					     BOOL _arithmeticCoding,
					     BOOL _advancedPrediction,
					     BOOL _pbFrames,
					     BOOL _temporalSpatialTradeOff,
					     unsigned _hrd_B,
					     unsigned _bppMaxKb,
					     unsigned _slowSqcifMPI,
					     unsigned _slowQcifMPI,
					     unsigned _slowCifMPI,
					     unsigned _slowCif4MPI,
					     unsigned _slowCif16MPI,
					     BOOL _errorCompensation)
{
  sqcifMPI = (_sqcifMPI>0?_sqcifMPI:-(signed)_slowSqcifMPI);
  qcifMPI = (_qcifMPI>0?_qcifMPI:-(signed)_slowQcifMPI);
  cifMPI = (_cifMPI>0?_cifMPI:-(signed)_slowCifMPI);
  cif4MPI = (_cif4MPI>0?_cif4MPI:-(signed)_slowCif4MPI);
  cif16MPI = (_cif16MPI>0?_cif16MPI:-(signed)_slowCif16MPI);

  maxBitRate = _maxBitRate;

  unrestrictedVector = _unrestrictedVector;
  arithmeticCoding = _arithmeticCoding;
  advancedPrediction = _advancedPrediction;
  pbFrames = _pbFrames;

  temporalSpatialTradeOff = _temporalSpatialTradeOff;

  hrd_B = _hrd_B;
  bppMaxKb = _bppMaxKb;

  errorCompensation = _errorCompensation;
}

PObject * H323_H263Capability::Clone() const
{
  return new H323_H263Capability(*this);
}

PObject::Comparison H323_H263Capability::Compare(const PObject & obj) const
{
  if (!PIsDescendant(&obj, H323_H263Capability))
    return LessThan;

  Comparison result = H323Capability::Compare(obj);
  if (result != EqualTo) 
    return result;

  const H323_H263Capability & other = (const H323_H263Capability &)obj;

  if ((sqcifMPI && other.sqcifMPI) ||
      (qcifMPI && other.qcifMPI) ||
      (cifMPI && other.cifMPI) ||
      (cif4MPI && other.cif4MPI) ||
      (cif16MPI && other.cif16MPI))
    return EqualTo;

  if ((!cif16MPI && other.cif16MPI) ||
      (!cif4MPI && other.cif4MPI) ||
      (!cifMPI && other.cifMPI) ||
      (!qcifMPI && other.qcifMPI))
    return LessThan;

  return GreaterThan;
}

PString H323_H263Capability::GetFormatName() const
{
  PString ret = OpalH263;

  if (sqcifMPI)
    ret += "-SQCIF";

  if (qcifMPI)
    ret += "-QCIF";

  if (cifMPI)
    ret += "-CIF";

  if (cif4MPI)
    ret += "-4CIF";

  if (cif16MPI)
    ret += "-16CIF";

  return ret;
}

unsigned H323_H263Capability::GetSubType() const
{
  return H245_VideoCapability::e_h263VideoCapability;
}

BOOL H323_H263Capability::OnSendingPDU(H245_VideoCapability & cap) const
{
  cap.SetTag(H245_VideoCapability::e_h263VideoCapability);

  H245_H263VideoCapability & h263 = cap;
  if (sqcifMPI > 0) {
    h263.IncludeOptionalField(H245_H263VideoCapability::e_sqcifMPI);
    h263.m_sqcifMPI = sqcifMPI;
  }
  if (qcifMPI > 0) {
    h263.IncludeOptionalField(H245_H263VideoCapability::e_qcifMPI);
    h263.m_qcifMPI = qcifMPI;
  }
  if (cifMPI > 0) {
    h263.IncludeOptionalField(H245_H263VideoCapability::e_cifMPI);
    h263.m_cifMPI = cifMPI;
  }
  if (cif4MPI > 0) {
    h263.IncludeOptionalField(H245_H263VideoCapability::e_cif4MPI);
    h263.m_cif4MPI = cif4MPI;
  }
  if (cif16MPI > 0) {
    h263.IncludeOptionalField(H245_H263VideoCapability::e_cif16MPI);
    h263.m_cif16MPI = cif16MPI;
  }
  h263.m_temporalSpatialTradeOffCapability = temporalSpatialTradeOff;
  h263.m_maxBitRate = maxBitRate;
  if (sqcifMPI < 0) {
    h263.IncludeOptionalField(H245_H263VideoCapability::e_slowSqcifMPI);
    h263.m_slowSqcifMPI = -sqcifMPI;
  }
  if (qcifMPI < 0) {
    h263.IncludeOptionalField(H245_H263VideoCapability::e_slowQcifMPI);
    h263.m_slowQcifMPI = -qcifMPI;
  }
  if (cifMPI < 0) {
    h263.IncludeOptionalField(H245_H263VideoCapability::e_slowCifMPI);
    h263.m_slowCifMPI = -cifMPI;
  }
  if (cif4MPI < 0) {
    h263.IncludeOptionalField(H245_H263VideoCapability::e_slowCif4MPI);
    h263.m_slowCif4MPI = -cif4MPI;
  }
  if (cif16MPI < 0) {
    h263.IncludeOptionalField(H245_H263VideoCapability::e_slowCif16MPI);
    h263.m_slowCif16MPI = -cif16MPI;
  }

  return TRUE;
}

BOOL H323_H263Capability::OnSendingPDU(H245_VideoMode & pdu) const
{
  pdu.SetTag(H245_VideoMode::e_h263VideoMode);
  H245_H263VideoMode & mode = pdu;
  mode.m_resolution.SetTag(cif16MPI ? H245_H263VideoMode_resolution::e_cif16
			  :(cif4MPI ? H245_H263VideoMode_resolution::e_cif4
			   :(cifMPI ? H245_H263VideoMode_resolution::e_cif
			    :(qcifMPI ? H245_H263VideoMode_resolution::e_qcif
			     : H245_H263VideoMode_resolution::e_sqcif))));
  mode.m_bitRate = maxBitRate;
  mode.m_unrestrictedVector = unrestrictedVector;
  mode.m_arithmeticCoding = arithmeticCoding;
  mode.m_advancedPrediction = advancedPrediction;
  mode.m_pbFrames = pbFrames;
  mode.m_errorCompensation = errorCompensation;

  return TRUE;
}

BOOL H323_H263Capability::OnReceivedPDU(const H245_VideoCapability & cap)
{
  if (cap.GetTag() != H245_VideoCapability::e_h263VideoCapability)
    return FALSE;

  const H245_H263VideoCapability & h263 = cap;
  if (h263.HasOptionalField(H245_H263VideoCapability::e_sqcifMPI))
    sqcifMPI = h263.m_sqcifMPI;
  else if (h263.HasOptionalField(H245_H263VideoCapability::e_slowSqcifMPI))
    sqcifMPI = -(signed)h263.m_slowSqcifMPI;
  else
    sqcifMPI = 0;
  if (h263.HasOptionalField(H245_H263VideoCapability::e_qcifMPI))
    qcifMPI = h263.m_qcifMPI;
  else if (h263.HasOptionalField(H245_H263VideoCapability::e_slowQcifMPI))
    qcifMPI = -(signed)h263.m_slowQcifMPI;
  else
    qcifMPI = 0;
  if (h263.HasOptionalField(H245_H263VideoCapability::e_cifMPI))
    cifMPI = h263.m_cifMPI;
  else if (h263.HasOptionalField(H245_H263VideoCapability::e_slowCifMPI))
    cifMPI = -(signed)h263.m_slowCifMPI;
  else
    cifMPI = 0;
  if (h263.HasOptionalField(H245_H263VideoCapability::e_cif4MPI))
    cif4MPI = h263.m_cif4MPI;
  else if (h263.HasOptionalField(H245_H263VideoCapability::e_slowCif4MPI))
    cif4MPI = -(signed)h263.m_slowCif4MPI;
  else
    cif4MPI = 0;
  if (h263.HasOptionalField(H245_H263VideoCapability::e_cif16MPI))
    cif16MPI = h263.m_cif16MPI;
  else if (h263.HasOptionalField(H245_H263VideoCapability::e_slowCif16MPI))
    cif16MPI = -(signed)h263.m_slowCif16MPI;
  else
    cif16MPI = 0;
  maxBitRate = h263.m_maxBitRate;
  unrestrictedVector = h263.m_unrestrictedVector;
  arithmeticCoding = h263.m_arithmeticCoding;
  advancedPrediction = h263.m_advancedPrediction;
  pbFrames = h263.m_pbFrames;
  temporalSpatialTradeOff = h263.m_temporalSpatialTradeOffCapability;
  hrd_B = h263.m_hrd_B;
  bppMaxKb = h263.m_bppMaxKb;
  errorCompensation = h263.m_errorCompensation;

  return TRUE;
}

#endif // NO_H323

//////////////////////////////////////////////////////////////////////

void H263Packet::Store(void *_data, int _data_size, void *_hdr, int _hdr_size)
{
  data = _data;
  data_size = _data_size;
  hdr = _hdr;
  hdr_size = _hdr_size;
}

BOOL H263Packet::Read(unsigned & length, RTP_DataFrame & frame)
{
  length = (unsigned) (hdr_size + data_size);
  if (!frame.SetPayloadSize(length)) {
    PTRACE(1, "H263Pck\tNot enough memory for packet of " << length << " bytes");
    length = 0;
    return FALSE;
  }
  memcpy(frame.GetPayloadPtr(), hdr, hdr_size);
  memcpy(frame.GetPayloadPtr() + hdr_size, data, data_size);
  return TRUE;
}

//////////////////////////////////////////////////////////////////////////////

Opal_H263_YUV420P::Opal_H263_YUV420P()
  : OpalVideoTranscoder(OpalH263, OpalYUV420P)
{
  if (!ff.IsLoaded())
    return;

  if ((codec = ff.AvcodecFindDecoder(CODEC_ID_H263)) == NULL) {
    PTRACE(1, "H263\tCodec not found for decoder");
    return;
  }

  frameWidth = 352;
  frameHeight = 288;

  context = ff.AvcodecAllocContext();
  if (context == NULL) {
    PTRACE(1, "H263\tFailed to allocate context for decoder");
    return;
  }

  picture = ff.AvcodecAllocFrame();
  if (picture == NULL) {
    PTRACE(1, "H263\tFailed to allocate frame for decoder");
    return;
  }

  if (!OpenCodec()) { // decoder will re-initialise context with correct frame size
    PTRACE(1, "H263\tFailed to open codec for decoder");
    return;
  }

  frameNum = 0;

  PTRACE(3, "Codec\tH263 decoder created");
}

Opal_H263_YUV420P::~Opal_H263_YUV420P()
{
  if (ff.IsLoaded()) {
    CloseCodec();

    ff.AvcodecFree(context);
    ff.AvcodecFree(picture);
  }
}

BOOL Opal_H263_YUV420P::OpenCodec()
{
  // avoid copying input/output
  context->flags |= CODEC_FLAG_INPUT_PRESERVED; // we guarantee to preserve input for max_b_frames+1 frames
  context->flags |= CODEC_FLAG_EMU_EDGE; // don't draw edges

  context->width  = frameWidth;
  context->height = frameHeight;

  context->workaround_bugs = 0; // no workaround for buggy H.263 implementations
  context->error_concealment = FF_EC_GUESS_MVS | FF_EC_DEBLOCK;
  context->error_resilience = FF_ER_CAREFULL;

  if (ff.AvcodecOpen(context, codec) < 0) {
    PTRACE(1, "H263\tFailed to open H.263 decoder");
    return FALSE;
  }

  return TRUE;
}

void Opal_H263_YUV420P::CloseCodec()
{
  if (context != NULL) {
    if (context->codec != NULL) {
      ff.AvcodecClose(context);
      PTRACE(5, "H263\tClosed H.263 decoder" );
    }
  }
}

PINDEX Opal_H263_YUV420P::GetOptimalDataFrameSize(BOOL input) const
{
  return input ? maxOutputSize : ((frameWidth * frameHeight * 12) / 8);
}

BOOL Opal_H263_YUV420P::ConvertFrames(const RTP_DataFrame & src, RTP_DataFrameList & dst)
{
  if (!ff.IsLoaded())
    return FALSE;

  PWaitAndSignal mutex(updateMutex);

  dst.RemoveAll();

  int payload_size = src.GetPayloadSize();
  unsigned char * payload;

  // get payload and ensure correct padding
  if (src.GetHeaderSize() + payload_size + FF_INPUT_BUFFER_PADDING_SIZE > src.GetSize()) {
    payload = (unsigned char *) encFrameBuffer.GetPointer(payload_size + FF_INPUT_BUFFER_PADDING_SIZE);
    memcpy(payload, src.GetPayloadPtr(), payload_size);
  }
  else
    payload = (unsigned char *) src.GetPayloadPtr();

  // some decoders might overread/segfault if the first 23 bits of padding are not 0
  unsigned char * padding = payload + payload_size;
  padding[0] = 0;
  padding[1] = 0;
  padding[2] = 0;

  // decode values from the RTP H263 header
  if (src.GetPayloadType() == RTP_DataFrame::H263) { // RFC 2190
    context->flags |= CODEC_FLAG_RFC2190;
  }
  else
    return FALSE;

  int got_picture, len;

  len = ff.AvcodecDecodeVideo(context, picture, &got_picture, payload, payload_size);

  if (!src.GetMarker()) // Have not built an entire frame yet
    return TRUE;

  len = ff.AvcodecDecodeVideo(context, picture, &got_picture, NULL, -1);

  if (len < 0) {
    PTRACE(1, "H263\tError while decoding frame");
    return TRUE; // And hope the error condition will fix itself
  }

  if (got_picture) {
    PTRACE(5, "H263\tDecoded frame (" << len << " bytes) into image "
           << context->width << "x" << context->height);
  
    // H.263 could change picture size at any time
    if (context->width == 0 || context->height == 0) {
      PTRACE(1,"H263\tImage dimension is 0");
      return TRUE; // And hope the error condition will fix itself
    }

    if (frameWidth != (unsigned)context->width || frameHeight != (unsigned)context->height) {
      frameWidth = context->width;
      frameHeight = context->height;
    }

    PINDEX frameBytes = (frameWidth * frameHeight * 12) / 8;
    RTP_DataFrame * pkt = new RTP_DataFrame(sizeof(FrameHeader)+frameBytes);
    FrameHeader * header = (FrameHeader *)pkt->GetPayloadPtr();
    header->x = header->y = 0;
    header->width = frameWidth;
    header->height = frameHeight;
    int size = frameWidth * frameHeight;
    if (picture->data[1] == picture->data[0] + size
        && picture->data[2] == picture->data[1] + (size >> 2))
      memcpy(header->data, picture->data[0], frameBytes);
    else {
      unsigned char *dst = header->data;
      for (int i=0; i<3; i ++) {
        unsigned char *src = picture->data[i];
        int dst_stride = i ? frameWidth >> 1 : frameWidth;
        int src_stride = picture->linesize[i];
        int h = i ? frameHeight >> 1 : frameHeight;

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
    pkt->SetPayloadSize(frameBytes);
    pkt->SetPayloadType(RTP_DataFrame::MaxPayloadType);
    pkt->SetTimestamp(src.GetTimestamp());
    pkt->SetMarker(TRUE);

    dst.Append(pkt);

    frameNum++;
  }

  return TRUE;
}

//////////////////////////////////////////////////////////////////////////////

Opal_YUV420P_H263::Opal_YUV420P_H263()
  : OpalVideoTranscoder(OpalYUV420P, OpalH263)
{
  if (!ff.IsLoaded())
    return;

  encodedPackets.DisallowDeleteObjects();
  unusedPackets.DisallowDeleteObjects();

  if ((codec = ff.AvcodecFindEncoder(CODEC_ID_H263)) == NULL) {
    PTRACE(1, "H263\tCodec not found for encoder");
    return;
  }

  frameWidth = 352;
  frameHeight = 288;

  context = ff.AvcodecAllocContext();
  if (context == NULL) {
    PTRACE(1, "H263\tFailed to allocate context for encoder");
    return;
  }

  picture = ff.AvcodecAllocFrame();
  if (picture == NULL) {
    PTRACE(1, "H263\tFailed to allocate frame for encoder");
    return;
  }

  context->codec = NULL;

  // set some reasonable values for quality as default
  videoQuality = 10; 
  videoQMin = 4;
  videoQMax = 24;

  frameNum = 0;

  PTRACE(3, "Codec\tH263 encoder created");
}

Opal_YUV420P_H263::~Opal_YUV420P_H263()
{
  if (ff.IsLoaded()) {
    CloseCodec();

    ff.AvcodecFree(context);
    ff.AvcodecFree(picture);

    encodedPackets.AllowDeleteObjects(TRUE);
    unusedPackets.AllowDeleteObjects(TRUE);
  }
}

BOOL Opal_YUV420P_H263::OpenCodec()
{
  // avoid copying input/output
  context->flags |= CODEC_FLAG_INPUT_PRESERVED; // we guarantee to preserve input for max_b_frames+1 frames
  context->flags |= CODEC_FLAG_EMU_EDGE; // don't draw edges

  context->width  = frameWidth;
  context->height = frameHeight;

  picture->linesize[0] = frameWidth;
  picture->linesize[1] = frameWidth / 2;
  picture->linesize[2] = frameWidth / 2;
  picture->quality = (float)videoQuality;

  int bitRate = 256000;
  context->bit_rate = (bitRate * 3) >> 2; // average bit rate
  context->bit_rate_tolerance = bitRate << 3;
  context->rc_min_rate = 0; // minimum bitrate
  context->rc_max_rate = bitRate; // maximum bitrate
  context->mb_qmin = context->qmin = videoQMin;
  context->mb_qmax = context->qmax = videoQMax;
  context->max_qdiff = 3; // max q difference between frames
  context->rc_qsquish = 0; // limit q by clipping
  context->rc_eq= "tex^qComp"; // rate control equation
  context->qcompress = 0.5; // qscale factor between easy & hard scenes (0.0-1.0)
  context->i_quant_factor = (float)-0.6; // qscale factor between p and i frames
  context->i_quant_offset = (float)0.0; // qscale offset between p and i frames
  // context->b_quant_factor = (float)1.25; // qscale factor between ip and b frames
  // context->b_quant_offset = (float)1.25; // qscale offset between ip and b frames

  context->flags |= CODEC_FLAG_PASS1;

  context->mb_decision = FF_MB_DECISION_SIMPLE; // choose only one MB type at a time
  context->me_method = ME_EPZS;
  context->me_subpel_quality = 8;

  context->frame_rate_base = 1;
  context->frame_rate = 15;

  context->gop_size = 64;

  context->flags &= ~CODEC_FLAG_H263P_UMV;
  context->flags &= ~CODEC_FLAG_4MV;
  context->max_b_frames = 0;
  context->flags &= ~CODEC_FLAG_H263P_AIC; // advanced intra coding (not handled by H323_FFH263Capability)

  context->flags |= CODEC_FLAG_RFC2190;

  context->rtp_mode = 1;
  context->rtp_payload_size = 750;
  context->rtp_callback = &Opal_YUV420P_H263::RtpCallback;
  context->opaque = this; // used to separate out packets from different encode threads

  if (ff.AvcodecOpen(context, codec) < 0) {
    PTRACE(1, "H263\tFailed to open H.263 encoder");
    return FALSE;
  }

  return TRUE;
}

void Opal_YUV420P_H263::CloseCodec()
{
  if (context != NULL) {
    if (context->codec != NULL) {
      ff.AvcodecClose(context);
      PTRACE(5, "H263\tClosed H.263 encoder" );
    }
  }
}

void Opal_YUV420P_H263::RtpCallback(void *data, int data_size, void *hdr, int hdr_size, void *priv_data)
{
  Opal_YUV420P_H263 *c = (Opal_YUV420P_H263 *) priv_data;
  H263Packet *p = c->unusedPackets.GetSize() > 0 ? (H263Packet *) c->unusedPackets.RemoveAt(0) : new H263Packet();

  p->Store(data, data_size, hdr, hdr_size);
  
  c->encodedPackets.Append(p);
}

static struct { int width; int height; } s_vidFrameSize[] = {
  {    0,    0}, // forbidden
  {  128,   96}, // SQCIF
  {  176,  144}, // QCIF
  {  352,  288}, // CIF
  {  704,  576}, // 4CIF
  { 1408, 1152}, // 16CIF
  {    0,    0}, // reserved
  {    0,    0}, // extended PTYPE
};

int Opal_YUV420P_H263::GetStdSize(int _width, int _height)
{
  int sizeIndex;

  for ( sizeIndex = SQCIF; sizeIndex < NumStdSizes; ++sizeIndex ) {
    if ( s_vidFrameSize[sizeIndex].width == _width && s_vidFrameSize[sizeIndex].height == _height )
      return sizeIndex;
  }

  return UnknownStdSize;
}

PINDEX Opal_YUV420P_H263::GetOptimalDataFrameSize(BOOL input) const
{
  return input ? ((frameWidth * frameHeight * 12) / 8) : maxOutputSize;
}

BOOL Opal_YUV420P_H263::ConvertFrames(const RTP_DataFrame & src, RTP_DataFrameList & dst)
{
  if (!ff.IsLoaded())
    return FALSE;

  PWaitAndSignal mutex(updateMutex);

  dst.RemoveAll();

  if (src.GetPayloadSize() < (PINDEX)sizeof(FrameHeader)) {
    PTRACE(1,"H263\tVideo grab too small, Close down video transmission thread.");
    return FALSE;
  }

  FrameHeader * header = (FrameHeader *)src.GetPayloadPtr();

  if (header->x != 0 || header->y != 0) {
    PTRACE(1,"H263\tVideo grab of partial frame unsupported, Close down video transmission thread.");
    return FALSE;
  }

  if (frameNum == 0 || frameWidth != header->width || frameHeight != header->height) {
    int sizeIndex = GetStdSize(header->width, header->height);
    if (sizeIndex == UnknownStdSize) {
      PTRACE(3, "H263\tCannot resize to " << header->width << "x" << header->height << " (non-standard format), Close down video transmission thread.");
      return FALSE;
    }

    frameWidth = header->width;
    frameHeight = header->height;

    rawFrameLen = (frameWidth * frameHeight * 12) / 8;
    rawFrameBuffer.SetSize(rawFrameLen + FF_INPUT_BUFFER_PADDING_SIZE);

    memset(rawFrameBuffer.GetPointer() + rawFrameLen, 0, FF_INPUT_BUFFER_PADDING_SIZE);

    encFrameLen = rawFrameLen; // this could be set to some lower value
    encFrameBuffer.SetSize(encFrameLen); // encoded video frame

    CloseCodec();
    if (!OpenCodec())
      return FALSE;
  }

  unsigned char * payload;

  // get payload and ensure correct padding
  if (src.GetHeaderSize() + src.GetPayloadSize() + FF_INPUT_BUFFER_PADDING_SIZE > src.GetSize()) {
    payload = (unsigned char *) rawFrameBuffer.GetPointer();
    memcpy(payload, header->data, rawFrameLen);
  }
  else
    payload = header->data;

  int size = frameWidth * frameHeight;
  picture->data[0] = payload;
  picture->data[1] = picture->data[0] + size;
  picture->data[2] = picture->data[1] + (size / 4);

PTRACE(1, "Now encode video for ctxt @ " << ::hex << (int)context << ", pict @ " << (int)picture << ", buf @ " << (int)encFrameBuffer.GetPointer() << ::dec << " (" << encFrameLen << " bytes)");
#if PTRACING
  PTime encTime;
  int out_size =  
#endif
  ff.AvcodecEncodeVideo(context, encFrameBuffer.GetPointer(), encFrameLen, picture);

#if PTRACING
  PTRACE(5, "H263\tEncoded " << out_size << " bytes from " << frameWidth << "x" << frameHeight
         << " in " << (PTime() - encTime).GetMilliSeconds() << " ms");
#endif

  if (encodedPackets.GetSize() == 0) {
    PTRACE(1, "H263\tEncoder internal error - there should be outstanding packets at this point");
    return TRUE; // And hope the error condition will fix itself
  }

  while (encodedPackets.GetSize() > 0) {
    RTP_DataFrame * pkt = new RTP_DataFrame(2048);
    unsigned length = maxOutputSize;
    H263Packet *p = (H263Packet *) encodedPackets.RemoveAt(0);
    if (p->Read(length, *pkt)) {
      pkt->SetPayloadType(RTP_DataFrame::H263);
      pkt->SetTimestamp(src.GetTimestamp());
      dst.Append(pkt);
    }
    unusedPackets.Append(p);
  }

  dst[dst.GetSize()-1].SetMarker(TRUE);

  frameNum++; // increment the number of frames encoded

  PTRACE(6, "H263\tEncoded " << src.GetPayloadSize() << " bytes of YUV420P raw data into " << dst.GetSize() << " RTP frame(s)");

  return TRUE;
}

#endif // RFC2190_AVCODEC

#endif // NO_OPAL_VIDEO
