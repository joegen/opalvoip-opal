/*
 * H.263 Plugin codec for OpenH323/OPAL
 *
 * Copyright (C) 2006 Post Increment, All Rights Reserved
 *
 * This code is based on the file h263codec.cxx from the OPAL project released
 * under the MPL 1.0 license which contains the following:
 *
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
 * Contributor(s): Guilhem Tardy (gtardy@salyens.com)
 *                 Craig Southeren (craigs@postincrement.com)
 *
 * $Log: h263ffmpeg.cxx,v $
 * Revision 1.1.2.1  2006/04/24 09:07:34  csoutheren
 * Initial implementation of H.263 codec plugin using ffmpeg.
 * Not yet tested - decoder only implemented
 *
 */

/*
  Notes
  -----

  This codec implements a H.263 encoder and decoder with RTP packaging as per 
  RFC 2190 "RTP Payload Format for H.263 Video Streams". As per this specification,
  The RTP payload code is always set to 34

 */


#include <codec/opalplugin.h>

extern "C" {
PLUGIN_CODEC_IMPLEMENT(FFMPEG_H263)
};


#include <stdlib.h>
#ifdef _WIN32
#include <windows.h>
#include <malloc.h>
#endif
#include <string.h>

extern "C" {
#include "ffmpeg/avcodec.h"
};

// if defined, the FFMPEG code is access via another DLL
// otherwise, the FFMPEG code is assumed to be statically linked into this plugin

#ifdef _WIN32
#define USE_DLL_AVCODEC   1
#endif // _WIN32

#define RTP_RFC2190_PAYLOAD  34
#define RTP_DYNAMIC_PAYLOAD  96

#define H263_CLOCKRATE    90000
#define H263_BITRATE      180000

#define CIF_WIDTH     352
#define CIF_HEIGHT    288

#define QCIF_WIDTH     (CIF_WIDTH/2)
#define QCIF_HEIGHT    (CIF_HEIGHT/2)

/////////////////////////////////////////////////////////////////
//
// define a class to implement a critical section mutex
// based on PCriticalSection from PWLib

class CriticalSection
{
  public:
    CriticalSection()
    { 
#ifdef _WIN32
      ::InitializeCriticalSection(&criticalSection); 
#else
      ::sem_init(&sem, 0, 1);
#endif
    }

    ~CriticalSection()
    { 
#ifdef _WIN32
      ::DeleteCriticalSection(&criticalSection); 
#else
      ::sem_destroy(&sem);
#endif
    }

    void Wait()
    { 
#ifdef _WIN32
      ::EnterCriticalSection(&criticalSection); 
#else
      ::sem_wait(&sem);
#endif
    }

    void Signal()
    { 
#ifdef _WIN32
      ::LeaveCriticalSection(&criticalSection); 
#else
      ::sem_post(&sem); 
#endif
    }

  private:
    CriticalSection & operator=(const CriticalSection &) { return *this; }
#ifdef _WIN32
    mutable CRITICAL_SECTION criticalSection; 
#else
    mutable sem_t sem;
#endif
};
    
class WaitAndSignal {
  public:
    inline WaitAndSignal(const CriticalSection & cs)
      : sync((CriticalSection &)cs)
    { sync.Wait(); }

    ~WaitAndSignal()
    { sync.Signal(); }

    WaitAndSignal & operator=(const WaitAndSignal &) 
    { return *this; }

  protected:
    CriticalSection & sync;
};

/////////////////////////////////////////////////////////////////
//
// define a class to simplify handling a DLL library
// based on PDynaLink from PWLib

#if USE_DLL_AVCODEC

class DynaLink
{
  public:
    typedef void (*Function)();

    DynaLink()
#ifdef _WIN32
    { _hDLL = NULL; }
#else
      ;
#endif // _WIN32

    ~DynaLink()
#ifdef _WIN32
    { Close(); }
#else
      ;
#endif // _WIN32

    virtual bool Open(const char *name)
#ifdef _WIN32
    {
# ifdef UNICODE
      USES_CONVERSION;
      _hDLL = LoadLibrary(A2T(name));
# else
      _hDLL = LoadLibrary(name);
# endif // UNICODE
      return _hDLL != NULL;
    }
#else
      ;
#endif // _WIN32


    virtual void Close()
#ifdef _WIN32
    {
      if (_hDLL != NULL) {
        FreeLibrary(_hDLL);
        _hDLL = NULL;
      }
    }
#else
      ;
#endif // _WIN32


    virtual bool IsLoaded() const
#ifdef _WIN32
    { return _hDLL != NULL; }
#else
      ;
#endif // _WIN32


    bool GetFunction(const char * name, Function & func)
#ifdef _WIN32
    {
      if (_hDLL == NULL)
        return FALSE;

# ifdef UNICODE
      USES_CONVERSION;
      FARPROC p = GetProcAddress(_hDLL, A2T(name));
# else
      FARPROC p = GetProcAddress(_hDLL, name);
# endif // UNICODE
      if (p == NULL)
        return FALSE;

      func = (Function)p;
      return TRUE;
    }
#else
      ;
#endif // _WIN32

#if defined(_WIN32)
  protected:
    HINSTANCE _hDLL;
#endif // _WIN32
};

#endif  // USE_DLL_AVCODEC 

/////////////////////////////////////////////////////////////////
//
// define a class to interface to the FFMpeg library


class FFMPEGLibrary

#if USE_DLL_AVCODEC
                   : public DynaLink
#endif // USE_DLL_AVCODEC
{
  public:
    FFMPEGLibrary();
    ~FFMPEGLibrary();

    bool Load();

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

    bool IsLoaded();
    CriticalSection processLock;

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

    bool isLoadedOK;
};

static FFMPEGLibrary FFMPEGLibraryInstance;

//////////////////////////////////////////////////////////////////////////////

#ifdef USE_DLL_AVCODEC

FFMPEGLibrary::FFMPEGLibrary()
{
  isLoadedOK = FALSE;
}

bool FFMPEGLibrary::Load()
{
  if (!DynaLink::Open("avcodec")
#if !defined(WIN32)
      && !PDynaLink::Open("libavcodec.so")
#endif
    ) {
    //cerr << "FFLINK\tFailed to load a library, some codecs won't operate correctly;" << endl;
#if !defined(WIN32)
    //cerr << "put libavcodec.so in the current directory (together with this program) and try again" << endl;
#else
    //cerr << "put avcodec.dll in the current directory (together with this program) and try again" << endl;
#endif
    return false;
  }

  if (!GetFunction("avcodec_init", (Function &)Favcodec_init)) {
    //cerr << "Failed to load avcodec_int" << endl;
    return false;
  }

  if (!GetFunction("h263_encoder", (Function &)Favcodec_h263_encoder)) {
    //cerr << "Failed to load h263_encoder" << endl;
    return false;
  }

  if (!GetFunction("h263p_encoder", (Function &)Favcodec_h263p_encoder)) {
    //cerr << "Failed to load h263p_encoder" << endl;
    return false;
  }

  if (!GetFunction("h263_decoder", (Function &)Favcodec_h263_decoder)) {
    //cerr << "Failed to load h263_decoder" << endl;
    return false;
  }

  if (!GetFunction("register_avcodec", (Function &)Favcodec_register)) {
    //cerr << "Failed to load register_avcodec" << endl;
    return false;
  }

  if (!GetFunction("avcodec_find_encoder", (Function &)Favcodec_find_encoder)) {
    //cerr << "Failed to load avcodec_find_encoder" << endl;
    return false;
  }

  if (!GetFunction("avcodec_find_decoder", (Function &)Favcodec_find_decoder)) {
    //cerr << "Failed to load avcodec_find_decoder" << endl;
    return false;
  }

  if (!GetFunction("avcodec_alloc_context", (Function &)Favcodec_alloc_context)) {
    //cerr << "Failed to load avcodec_alloc_context" << endl;
    return false;
  }

  if (!GetFunction("avcodec_alloc_frame", (Function &)Favcodec_alloc_frame)) {
    //cerr << "Failed to load avcodec_alloc_frame" << endl;
    return false;
  }

  if (!GetFunction("avcodec_open", (Function &)Favcodec_open)) {
    //cerr << "Failed to load avcodec_open" << endl;
    return false;
  }

  if (!GetFunction("avcodec_close", (Function &)Favcodec_close)) {
    //cerr << "Failed to load avcodec_close" << endl;
    return false;
  }

  if (!GetFunction("avcodec_encode_video", (Function &)Favcodec_encode_video)) {
    //cerr << "Failed to load avcodec_encode_video" << endl;
    return false;
  }

  if (!GetFunction("avcodec_decode_video", (Function &)Favcodec_decode_video)) {
    //cerr << "Failed to load avcodec_decode_video" << endl;
    return false;
  }

  if (!GetFunction("avcodec_set_print_fn", (Function &)Favcodec_set_print_fn)) {
    //cerr << "Failed to load avcodec_set_print_fn" << endl;
    return false;
  }
   
  if (!GetFunction("av_free", (Function &)Favcodec_free)) {
    //cerr << "Failed to load avcodec_close" << endl;
    return false;
  }

  // must be called before using avcodec lib
  Favcodec_init();

  // register only the codecs needed (to have smaller code)
  Favcodec_register(Favcodec_h263_encoder);
  Favcodec_register(Favcodec_h263p_encoder);
  Favcodec_register(Favcodec_h263_decoder);
  
  //Favcodec_set_print_fn(h263_ffmpeg_printon);

  isLoadedOK = TRUE;

  return true;
}

FFMPEGLibrary::~FFMPEGLibrary()
{
  DynaLink::Close();
}

AVCodec *FFMPEGLibrary::AvcodecFindEncoder(enum CodecID id)
{
  AVCodec *res = Favcodec_find_encoder(id);
  //PTRACE_IF(6, res, "FFLINK\tFound encoder " << res->name << " @ " << ::hex << (int)res << ::dec);
  return res;
}

AVCodec *FFMPEGLibrary::AvcodecFindDecoder(enum CodecID id)
{
  AVCodec *res = Favcodec_find_decoder(id);
  //PTRACE_IF(6, res, "FFLINK\tFound decoder " << res->name << " @ " << ::hex << (int)res << ::dec);
  return res;
}

AVCodecContext *FFMPEGLibrary::AvcodecAllocContext(void)
{
  AVCodecContext *res = Favcodec_alloc_context();
  //PTRACE_IF(6, res, "FFLINK\tAllocated context @ " << ::hex << (int)res << ::dec);
  return res;
}

AVFrame *FFMPEGLibrary::AvcodecAllocFrame(void)
{
  AVFrame *res = Favcodec_alloc_frame();
  //PTRACE_IF(6, res, "FFLINK\tAllocated frame @ " << ::hex << (int)res << ::dec);
  return res;
}

int FFMPEGLibrary::AvcodecOpen(AVCodecContext *ctx, AVCodec *codec)
{
  WaitAndSignal m(processLock);

  //PTRACE(6, "FFLINK\tNow open context @ " << ::hex << (int)ctx << ", codec @ " << (int)codec << ::dec);
  return Favcodec_open(ctx, codec);
}

int FFMPEGLibrary::AvcodecClose(AVCodecContext *ctx)
{
  //PTRACE(6, "FFLINK\tNow close context @ " << ::hex << (int)ctx << ::dec);
  return Favcodec_close(ctx);
}

int FFMPEGLibrary::AvcodecEncodeVideo(AVCodecContext *ctx, BYTE *buf, int buf_size, const AVFrame *pict)
{
  WaitAndSignal m(processLock);

  //PTRACE(6, "FFLINK\tNow encode video for ctxt @ " << ::hex << (int)ctx << ", pict @ " << (int)pict
	// << ", buf @ " << (int)buf << ::dec << " (" << buf_size << " bytes)");
  int res = Favcodec_encode_video(ctx, buf, buf_size, pict);

  //PTRACE(6, "FFLINK\tEncoded video into " << res << " bytes");
  return res;
}

int FFMPEGLibrary::AvcodecDecodeVideo(AVCodecContext *ctx, AVFrame *pict, int *got_picture_ptr, BYTE *buf, int buf_size)
{
  WaitAndSignal m(processLock);

  //PTRACE(6, "FFLINK\tNow decode video for ctxt @ " << ::hex << (int)ctx << ", pict @ " << (int)pict
	// << ", buf @ " << (int)buf << ::dec << " (" << buf_size << " bytes)");
  int res = Favcodec_decode_video(ctx, pict, got_picture_ptr, buf, buf_size);

  //PTRACE(6, "FFLINK\tDecoded video of " << res << " bytes, got_picture=" << *got_picture_ptr);
  return res;
}

void FFMPEGLibrary::AvcodecSetPrintFn(void (*print_fn)(char *))
{
  Favcodec_set_print_fn(print_fn);
}

void FFMPEGLibrary::AvcodecFree(void * ptr)
{
  Favcodec_free(ptr);
}

bool FFMPEGLibrary::IsLoaded()
{
  WaitAndSignal m(processLock);

  return isLoadedOK;
}

#else

#error "Not yet able to use statically linked libavcodec"

#endif // USE_DLL_AVCODEC

/////////////////////////////////////////////////////////////////////////////
//
// define some simple RTP packet routines
//

#define RTP_MIN_HEADER_SIZE 12

class RTPFrame
{
  public:
    RTPFrame(const unsigned char * _packet, int _packetLen)
      : packet((unsigned char *)_packet), packetLen(_packetLen)
    {
    }

    RTPFrame(unsigned char * _packet, int _packetLen, unsigned char payloadType)
      : packet(_packet), packetLen(_packetLen)
    { 
      if (packetLen > 0)
        packet[0] = 0x80;    // set version, no extensions, zero contrib count
      SetPayloadType(payloadType);
    }

    inline unsigned long GetLong(int offs) const
    {
      if (offs + 4 > packetLen)
        return 0;
      return (packet[offs + 0] << 24) + (packet[offs+1] << 16) + (packet[offs+2] << 8) + packet[offs+3]; 
    }

    inline void SetLong(int offs, unsigned long n)
    {
      if (offs + 4 <= packetLen) {
        packet[offs + 0] = (BYTE)((n >> 24) & 0xff);
        packet[offs + 1] = (BYTE)((n >> 16) & 0xff);
        packet[offs + 2] = (BYTE)((n >> 8) & 0xff);
        packet[offs + 3] = (BYTE)(n & 0xff);
      }
    }

    inline unsigned short GetShort(int offs) const
    { 
      if (offs + 2 > packetLen)
        return 0;
      return (packet[offs + 0] << 8) + packet[offs + 1]; 
    }

    inline void SetShort(int offs, unsigned short n) 
    { 
      if (offs + 2 <= packetLen) {
        packet[offs + 0] = (BYTE)((n >> 8) & 0xff);
        packet[offs + 1] = (BYTE)(n & 0xff);
      }
    }

    inline int GetPacketLen() const                    { return packetLen; }
    inline unsigned GetVersion() const                 { return (packetLen < 1) ? 0 : (packet[0]>>6)&3; }
    inline bool GetExtension() const                   { return (packetLen < 1) ? 0 : (packet[0]&0x10) != 0; }
    inline bool GetMarker()  const                     { return (packetLen < 2) ? FALSE : ((packet[1]&0x80) != 0); }
    inline unsigned char GetPayloadType() const        { return (packetLen < 2) ? FALSE : (packet[1] & 0x7f);  }
    inline unsigned short GetSequenceNumber() const    { return GetShort(2); }
    inline unsigned long GetTimestamp() const          { return GetLong(4); }
    inline unsigned long GetSyncSource() const         { return GetLong(8); }
    inline int GetContribSrcCount() const              { return (packetLen < 1) ? 0  : (packet[0]&0xf); }
    inline int GetExtensionSize() const                { return !GetExtension() ? 0  : GetShort(RTP_MIN_HEADER_SIZE + 4*GetContribSrcCount() + 2); }
    inline int GetExtensionType() const                { return !GetExtension() ? -1 : GetShort(RTP_MIN_HEADER_SIZE + 4*GetContribSrcCount()); }
    inline int GetPayloadSize() const                  { return packetLen - GetHeaderSize(); }
    inline const unsigned char * GetPayloadPtr() const { return packet + GetHeaderSize(); }

    inline unsigned int GetHeaderSize() const    
    { 
      unsigned int sz = RTP_MIN_HEADER_SIZE + 4*GetContribSrcCount();
      if (GetExtension())
        sz += 4 + GetExtensionSize();
      return sz;
    }

    inline void SetMarker(bool m)                    { if (packetLen >= 2) packet[1] = (packet[1] & 0x7f) | (m ? 0x80 : 0x00); }
    inline void SetPayloadType(unsigned char t)      { if (packetLen >= 2) packet[1] = (packet[1] & 0x80) | (t & 0x7f); }
    inline void SetSequenceNumber(unsigned short v)  { SetShort(2, v); }
    inline void SetTimestamp(unsigned long n)        { SetLong(4, n); }
    inline void SetSyncSource(unsigned long n)       { SetLong(8, n); }
    inline void SetPayloadSize(int payloadSize)      { packetLen = GetHeaderSize() + payloadSize; }

  protected:
    unsigned char * packet;
    int packetLen;
};

/////////////////////////////////////////////////////////////////////////////

static const char * default_cif_h263_options[][3] = {
  { "h323_cifMPI",                               "<2" ,    "i" },
  { "h323_maxBitRate",                           "<6217" , "i" },
  { "h323_temporalSpatialTradeOffCapability",    "<f" ,    "b" },
  { NULL, NULL, NULL }
};

static const char * default_qcif_h263_options[][3] = {
  { "h323_qcifMPI",                              "<1" ,    "i" },
  { "h323_maxBitRate",                           "<6217" , "i" },
  { "h323_temporalSpatialTradeOffCapability",    "<f" ,    "b" },
  { NULL, NULL, NULL }
};

static const char * default_sip_h263_options[][3] = {
  { "h323_qcifMPI",                              "<1" ,    "i" },
  { "h323_maxBitRate",                           "<6217" , "i" },
  { "h323_temporalSpatialTradeOffCapability",    "<f" ,    "b" },
  { NULL, NULL, NULL }
};

static int get_xcif_options(void * context, void * parm, unsigned * parmLen, const char ** default_parms)
{
  if (parmLen == NULL || parm == NULL || *parmLen != sizeof(char **))
    return 0;

  const char ***options = (const char ***)parm;

  if (context == NULL) {
    *options = default_parms;
    return 1;
  }

  return 0;
}

static int coder_get_cif_options(
      const PluginCodec_Definition * , 
      void * context, 
      const char * , 
      void * parm, 
      unsigned * parmLen)
{
  return get_xcif_options(context, parm, parmLen, &default_cif_h263_options[0][0]);
}

static int coder_get_qcif_options(
      const PluginCodec_Definition * , 
      void * context, 
      const char * , 
      void * parm, 
      unsigned * parmLen)
{
  return get_xcif_options(context, parm, parmLen, &default_qcif_h263_options[0][0]);
}

static int coder_get_sip_options(
      const PluginCodec_Definition *, 
      void * context, 
      const char * , 
      void * parm, 
      unsigned * parmLen)
{
  return get_xcif_options(context, parm, parmLen, &default_sip_h263_options[0][0]);
}

/////////////////////////////////////////////////////////////////////////////

class H263EncoderContext
{
  public:
    H263EncoderContext() 
    { }
};


static void * create_encoder(const struct PluginCodec_Definition * /*codec*/)
{
  return new H263EncoderContext;
}

static int encoder_set_options(
      const PluginCodec_Definition * , 
      void *, 
      const char * , 
      void *, 
      unsigned * )
{
  //H263EncoderContext * context = (H263EncoderContext *)_context;
  return 1;
}

static void destroy_encoder(const struct PluginCodec_Definition * /*codec*/, void * _context)
{
  H263EncoderContext * context = (H263EncoderContext *)_context;
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
  H263EncoderContext * context = (H263EncoderContext *)_context;
  return 0;
}

static int encoder_get_output_data_size(const PluginCodec_Definition * codec, void *, const char *, void *, unsigned *)
{
  // this is really frame height * frame width;
  return codec->samplesPerFrame * codec->bytesPerFrame * 3 / 2;
}

static PluginCodec_ControlDefn cifEncoderControls[] = {
  { "get_codec_options",    coder_get_cif_options },
  { "set_codec_options",    encoder_set_options },
  { "get_output_data_size", encoder_get_output_data_size },
  { NULL }
};

static PluginCodec_ControlDefn qcifEncoderControls[] = {
  { "get_codec_options",    coder_get_qcif_options },
  { "set_codec_options",    encoder_set_options },
  { "get_output_data_size", encoder_get_output_data_size },
  { NULL }
};

static PluginCodec_ControlDefn sipEncoderControls[] = {
  { "get_codec_options",    coder_get_sip_options },
  { "set_codec_options",    encoder_set_options },
  { "get_output_data_size", encoder_get_output_data_size },
  { NULL }
};

/////////////////////////////////////////////////////////////////////////////

class H263DecoderContext
{
  public:
    H263DecoderContext();
    ~H263DecoderContext();

    bool DecodeFrames(const BYTE * src, unsigned & srcLen, BYTE * dst, unsigned & dstLen, unsigned int & flags);

  protected:
    bool OpenCodec();
    void CloseCodec();

    unsigned char encFrameBuffer[1500];

    AVCodec        *avcodec;
    AVCodecContext *avcontext;
    AVFrame        *picture;

    int frameNum;
    unsigned int frameWidth;
    unsigned int frameHeight;
};

H263DecoderContext::H263DecoderContext()
{
  if (!FFMPEGLibraryInstance.IsLoaded())
    return;

  if ((avcodec = FFMPEGLibraryInstance.AvcodecFindDecoder(CODEC_ID_H263)) == NULL) {
    //PTRACE(1, "H263\tCodec not found for decoder");
    return;
  }

  frameWidth  = CIF_WIDTH;
  frameHeight = CIF_HEIGHT;

  avcontext = FFMPEGLibraryInstance.AvcodecAllocContext();
  if (avcontext == NULL) {
    //PTRACE(1, "H263\tFailed to allocate context for decoder");
    return;
  }

  picture = FFMPEGLibraryInstance.AvcodecAllocFrame();
  if (picture == NULL) {
    //PTRACE(1, "H263\tFailed to allocate frame for decoder");
    return;
  }

  if (!OpenCodec()) { // decoder will re-initialise context with correct frame size
    //PTRACE(1, "H263\tFailed to open codec for decoder");
    return;
  }

  frameNum = 0;

  //PTRACE(3, "Codec\tH263 decoder created");
}

H263DecoderContext::~H263DecoderContext()
{
  if (FFMPEGLibraryInstance.IsLoaded()) {
    CloseCodec();

    FFMPEGLibraryInstance.AvcodecFree(avcontext);
    FFMPEGLibraryInstance.AvcodecFree(picture);
  }
}

bool H263DecoderContext::OpenCodec()
{
  // avoid copying input/output
  avcontext->flags |= CODEC_FLAG_INPUT_PRESERVED; // we guarantee to preserve input for max_b_frames+1 frames
  avcontext->flags |= CODEC_FLAG_EMU_EDGE; // don't draw edges

  avcontext->width  = frameWidth;
  avcontext->height = frameHeight;

  avcontext->workaround_bugs = 0; // no workaround for buggy H.263 implementations
  avcontext->error_concealment = FF_EC_GUESS_MVS | FF_EC_DEBLOCK;
  avcontext->error_resilience = FF_ER_CAREFULL;

  if (FFMPEGLibraryInstance.AvcodecOpen(avcontext, avcodec) < 0) {
    //PTRACE(1, "H263\tFailed to open H.263 decoder");
    return FALSE;
  }

  return TRUE;
}

void H263DecoderContext::CloseCodec()
{
  if (avcontext != NULL) {
    if (avcontext->codec != NULL) {
      FFMPEGLibraryInstance.AvcodecClose(avcontext);
      //PTRACE(5, "H263\tClosed H.263 decoder" );
    }
  }
}

bool H263DecoderContext::DecodeFrames(const BYTE * src, unsigned & srcLen, BYTE * dst, unsigned & dstLen, unsigned int & flags)
{
  if (!FFMPEGLibraryInstance.IsLoaded())
    return 0;

  dstLen = 0;
  flags = 0;

  // create RTP frame from source buffer
  RTPFrame srcRTP(src, srcLen);

  // create RTP frame from destination buffer
  RTPFrame dstRTP(dst, dstLen, 0);

  int srcPayloadSize = srcRTP.GetPayloadSize();
  unsigned char * payload;

  // copy payload to a temporary buffer if there are not enough bytes after the end of the payload
  if (srcRTP.GetHeaderSize() + srcPayloadSize + FF_INPUT_BUFFER_PADDING_SIZE > srcLen) {
    if (srcPayloadSize + FF_INPUT_BUFFER_PADDING_SIZE > sizeof(encFrameBuffer))
      return 0; 

    memcpy(encFrameBuffer, srcRTP.GetPayloadPtr(), srcPayloadSize);
    payload = encFrameBuffer;
  }
  else
    payload = (unsigned char *) srcRTP.GetPayloadPtr();

  // ensure the first 24 bits past the end of the payload are all zero
  {
    unsigned char * padding = payload + srcPayloadSize;
    padding[0] = padding[1] = padding[2] = 0;
  }

  // only accept RFC 2190 for now
  switch (srcRTP.GetPayloadType()) {
    case RTP_RFC2190_PAYLOAD:
      avcontext->flags |= CODEC_FLAG_RFC2190;
      break;

    //case RTP_DYNAMIC_PAYLOAD:
    //  avcontext->flags |= RTPCODEC_FLAG_RFC2429
    //  break;
    default:
      return 1;
  }

  // decode the frame
  int got_picture;
  int len = FFMPEGLibraryInstance.AvcodecDecodeVideo(avcontext, picture, &got_picture, payload, srcPayloadSize);

  // if that was not the last packet for the frame, keep going
  if (!srcRTP.GetMarker()) {
    return 1;
  }

  // cause decoder to end the frame
  len = FFMPEGLibraryInstance.AvcodecDecodeVideo(avcontext, picture, &got_picture, NULL, -1);

  // if error occurred, tell the other end to send another I-frame and hopefully we can resync
  if (len < 0) {
    flags = PluginCodec_ReturnCoderRequestIFrame;
    return 1;
  }

  // no picture was decoded - shrug and do nothing
  if (!got_picture)
    return 1;

  // if decoded frame size is not legal, request an I-Frame
  if (avcontext->width == 0 || avcontext->height == 0) {
    flags = PluginCodec_ReturnCoderRequestIFrame;
    return 1;
  }

  // see if frame size has changed
  if (frameWidth != (unsigned)avcontext->width || frameHeight != (unsigned)avcontext->height) {
    frameWidth  = avcontext->width;
    frameHeight = avcontext->height;
  }

  int frameBytes = (frameWidth * frameHeight * 12) / 8;
  PluginCodec_Video_FrameHeader * header = (PluginCodec_Video_FrameHeader *)dstRTP.GetPayloadPtr();
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

  dstRTP.SetPayloadSize(frameBytes);
  dstRTP.SetPayloadType(RTP_DYNAMIC_PAYLOAD);
  dstRTP.SetTimestamp(srcRTP.GetTimestamp());
  dstRTP.SetMarker(TRUE);

  frameNum++;

  return TRUE;
}


static void * create_decoder(const struct PluginCodec_Definition *)
{
  return new H263DecoderContext;
}

static int decoder_set_options(
      const struct PluginCodec_Definition *, 
      void * _context, 
      const char *, 
      void * parm, 
      unsigned * parmLen)
{
  H263DecoderContext * context = (H263DecoderContext *)_context;

  if (parmLen == NULL || *parmLen != sizeof(const char **))
    return 0;

  // get the "frame width" media format parameter to use as a hint for the encoder to start off
  if (parm != NULL) {
    const char ** options = (const char **)parm;
    int i;
    int frameWidth = 0;
    for (i = 0; options[i] != NULL; i += 2) {
      if (strcmpi(options[i], "Frame Width") == 0)
        frameWidth = atoi(options[i+1]);
    }
    //context->videoDecoder->fmt_ = (frameWidth == QCIF_WIDTH) ? IT_QCIF : IT_CIF;
    //context->videoDecoder->init();
  }

  return 1;
}

static void destroy_decoder(const struct PluginCodec_Definition * /*codec*/, void * _context)
{
  H263DecoderContext * context = (H263DecoderContext *)_context;
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
  H263DecoderContext * context = (H263DecoderContext *)_context;
  return context->DecodeFrames((const BYTE *)from, *fromLen, (BYTE *)to, *toLen, *flag);
}

static int decoder_get_output_data_size(const PluginCodec_Definition * codec, void *, const char *, void *, unsigned *)
{
  // this is really frame height * frame width;
  return sizeof(PluginCodec_Video_FrameHeader) + ((codec->samplesPerFrame * codec->bytesPerFrame * 3) / 2);
}

static PluginCodec_ControlDefn cifDecoderControls[] = {
  { "get_codec_options",    coder_get_cif_options },
  { "set_codec_options",    decoder_set_options },
  { "get_output_data_size", decoder_get_output_data_size },
  { NULL }
};

static PluginCodec_ControlDefn qcifDecoderControls[] = {
  { "get_codec_options",    coder_get_qcif_options },
  { "set_codec_options",    decoder_set_options },
  { "get_output_data_size", decoder_get_output_data_size },
  { NULL }
};

static PluginCodec_ControlDefn sipDecoderControls[] = {
  { "get_codec_options",    coder_get_sip_options },
  { "set_codec_options",    decoder_set_options },
  { "get_output_data_size", decoder_get_output_data_size },
  { NULL }
};

/////////////////////////////////////////////////////////////////////////////


static struct PluginCodec_information licenseInfo = {
  1145863600,                                                   // timestamp =  Mon 24 Apr 2006 07:26:40 AM UTC

  "Craig Southeren, Post Increment",                            // source code author
  "1.0",                                                        // source code version
  "craigs@postincrement.com",                                   // source code email
  "http://www.postincrement.com",                               // source code URL
  "Copyright (C) 2006 by Post Increment, All Rights Reserved ",  // source code copyright
  ", Copyright (C) 2005 Salyens"
  ", Copyright (C) 2001 March Networks Corporation"
  ", Copyright (C) 1999-2000 Equivalence Pty. Ltd."
  "MPL 1.0",                                                    // source code license
  PluginCodec_License_MPL,                                      // source code license
  
  "FFMPEG",                                                        // codec description
  "Fabrice Bellard",                                               // codec author
  "4.7.1",                                                         // codec version
  "bellard@users.sourceforge.net",                                 // codec email
  "http://sourceforge.net/projects/ffmpeg/",                       // codec URL
  "Copyright (c) 2001 Fabrice Bellard.",                           // codec copyright information
  "GNU LESSER GENERAL PUBLIC LICENSE, Version 2.1, February 1999", // codec license
  PluginCodec_License_LGPL                                         // codec license code
};

/////////////////////////////////////////////////////////////////////////////

static const char YUV420PDesc[]  = { "YUV420P" };

static const char h263QCIFDesc[]  = { "H.263-QCIF" };
static const char h263CIFDesc[]   = { "H.263-CIF" };
static const char h263Desc[]      = { "H.263" };

static const char sdpH263[]   = { "h263" };

/////////////////////////////////////////////////////////////////////////////

static struct PluginCodec_Definition h263CodecDefn[6] = {

{ 
  // H.323 CIF encoder
  PLUGIN_CODEC_VERSION_VIDEO,         // codec API version
  &licenseInfo,                       // license information

  PluginCodec_MediaTypeVideo |        // audio codec
  PluginCodec_RTPTypeExplicit,        // specified RTP type

  h263CIFDesc,                        // text decription
  YUV420PDesc,                        // source format
  h263CIFDesc,                        // destination format

  0,                                  // user data 

  H263_CLOCKRATE,                     // samples per second
  H263_BITRATE,                       // raw bits per second
  20000,                              // nanoseconds per frame

  CIF_WIDTH,                          // frame width
  CIF_HEIGHT,                         // frame height
  10,                                 // recommended frame rate
  60,                                 // maximum frame rate
  RTP_RFC2190_PAYLOAD,                // IANA RTP payload code
  NULL,                               // RTP payload name

  create_encoder,                     // create codec function
  destroy_encoder,                    // destroy codec
  codec_encoder,                      // encode/decode
  cifEncoderControls,                 // codec controls

  PluginCodec_H323VideoCodec_h261,    // h323CapabilityType 
  NULL                                // h323CapabilityData
},
{ 
  // H.323 CIF decoder
  PLUGIN_CODEC_VERSION_VIDEO,         // codec API version
  &licenseInfo,                       // license information

  PluginCodec_MediaTypeVideo |        // audio codec
  PluginCodec_RTPTypeExplicit,        // specified RTP type

  h263CIFDesc,                        // text decription
  h263CIFDesc,                        // source format
  YUV420PDesc,                        // destination format

  0,                                  // user data 

  H263_CLOCKRATE,                     // samples per second
  H263_BITRATE,                       // raw bits per second
  20000,                              // nanoseconds per frame

  CIF_WIDTH,                          // frame width
  CIF_HEIGHT,                         // frame height
  10,                                 // recommended frame rate
  60,                                 // maximum frame rate
  RTP_RFC2190_PAYLOAD,                // IANA RTP payload code
  NULL,                               // RTP payload name

  create_decoder,                     // create codec function
  destroy_decoder,                    // destroy codec
  codec_decoder,                      // encode/decode
  cifDecoderControls,                 // codec controls

  PluginCodec_H323VideoCodec_h261,    // h323CapabilityType 
  NULL                                // h323CapabilityData
},

{ 
  // H.323 QCIF encoder
  PLUGIN_CODEC_VERSION_VIDEO,         // codec API version
  &licenseInfo,                       // license information

  PluginCodec_MediaTypeVideo |        // audio codec
  PluginCodec_RTPTypeExplicit,        // specified RTP type

  h263QCIFDesc,                       // text decription
  YUV420PDesc,                        // source format
  h263QCIFDesc,                       // destination format

  0,                                  // user data 

  H263_CLOCKRATE,                     // samples per second
  H263_BITRATE,                       // raw bits per second
  20000,                              // nanoseconds per frame

  QCIF_WIDTH,                         // frame width
  QCIF_HEIGHT,                        // frame height
  10,                                 // recommended frame rate
  60,                                 // maximum frame rate
  RTP_RFC2190_PAYLOAD,                // IANA RTP payload code
  NULL,                               // RTP payload name

  create_encoder,                     // create codec function
  destroy_encoder,                    // destroy codec
  codec_encoder,                      // encode/decode
  qcifEncoderControls,                // codec controls

  PluginCodec_H323VideoCodec_h261,    // h323CapabilityType 
  NULL                                // h323CapabilityData
},
{ 
  // H.323 QCIF decoder
  PLUGIN_CODEC_VERSION_VIDEO,         // codec API version
  &licenseInfo,                       // license information

  PluginCodec_MediaTypeVideo |        // audio codec
  PluginCodec_RTPTypeExplicit,        // specified RTP type

  h263QCIFDesc,                       // text decription
  h263QCIFDesc,                       // source format
  YUV420PDesc,                        // destination format

  0,                                  // user data 

  H263_CLOCKRATE,                     // samples per second
  H263_BITRATE,                       // raw bits per second
  20000,                              // nanoseconds per frame

  QCIF_WIDTH,                         // frame width
  QCIF_HEIGHT,                        // frame height
  10,                                 // recommended frame rate
  60,                                 // maximum frame rate
  RTP_RFC2190_PAYLOAD,                // IANA RTP payload code
  NULL,                               // RTP payload name

  create_decoder,                     // create codec function
  destroy_decoder,                    // destroy codec
  codec_decoder,                      // encode/decode
  qcifDecoderControls,                // codec controls

  PluginCodec_H323VideoCodec_h261,    // h323CapabilityType 
  NULL                                // h323CapabilityData
},

{ 
  // SIP encoder
  PLUGIN_CODEC_VERSION_VIDEO,         // codec API version
  &licenseInfo,                       // license information

  PluginCodec_MediaTypeVideo |        // audio codec
  PluginCodec_RTPTypeExplicit,        // specified RTP type

  h263Desc,                           // text decription
  YUV420PDesc,                        // source format
  h263Desc,                           // destination format

  0,                                  // user data 

  H263_CLOCKRATE,                     // samples per second
  H263_BITRATE,                       // raw bits per second
  20000,                              // nanoseconds per frame

  CIF_WIDTH,                          // frame width
  CIF_HEIGHT,                         // frame height
  10,                                 // recommended frame rate
  60,                                 // maximum frame rate
  RTP_RFC2190_PAYLOAD,                // IANA RTP payload code
  sdpH263,                            // RTP payload name

  create_encoder,                     // create codec function
  destroy_encoder,                    // destroy codec
  codec_encoder,                      // encode/decode
  sipEncoderControls,                 // codec controls

  PluginCodec_H323Codec_NoH323,       // h323CapabilityType 
  NULL                                // h323CapabilityData
},
{ 
  // SIP decoder
  PLUGIN_CODEC_VERSION_VIDEO,         // codec API version
  &licenseInfo,                       // license information

  PluginCodec_MediaTypeVideo |        // audio codec
  PluginCodec_RTPTypeExplicit,        // specified RTP type

  h263Desc,                           // text decription
  h263Desc,                           // source format
  YUV420PDesc,                        // destination format

  0,                                  // user data 

  H263_CLOCKRATE,                     // samples per second
  H263_BITRATE,                       // raw bits per second
  20000,                              // nanoseconds per frame

  CIF_WIDTH,                          // frame width
  CIF_HEIGHT,                         // frame height
  10,                                 // recommended frame rate
  60,                                 // maximum frame rate
  RTP_RFC2190_PAYLOAD,                // IANA RTP payload code
  sdpH263,                            // RTP payload name

  create_decoder,                     // create codec function
  destroy_decoder,                    // destroy codec
  codec_decoder,                      // encode/decode
  sipDecoderControls,                 // codec controls

  PluginCodec_H323Codec_NoH323,       // h323CapabilityType 
  NULL                                // h323CapabilityData
},

};

#define NUM_DEFNS   (sizeof(h263CodecDefn) / sizeof(struct PluginCodec_Definition))

/////////////////////////////////////////////////////////////////////////////

extern "C" {
PLUGIN_CODEC_DLL_API struct PluginCodec_Definition * PLUGIN_CODEC_GET_CODEC_FN(unsigned * count, unsigned version)
{
  // load the DLL
  {
    WaitAndSignal m(FFMPEGLibraryInstance.processLock);
    if (!FFMPEGLibraryInstance.IsLoaded()) {
      if (!FFMPEGLibraryInstance.Load()) {
        *count = 0;
        return NULL;
      }
    }
  }

  // check version numbers etc
  if (version < PLUGIN_CODEC_VERSION_VIDEO) {
    *count = 0;
    return NULL;
  }
  else {
    *count = NUM_DEFNS;
    return h263CodecDefn;
  }
}
};
