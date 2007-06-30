/*
 * MPEG4 Plugin codec for OpenH323/OPAL
 * This code is based on the H.263 plugin implementation in OPAL.
 *
 * The original files, and this version of the original code, are released under the same 
 * MPL 1.0 license. Substantial portions of the original code were contributed
 * by Salyens and March Networks and their right to be identified as copyright holders
 * of the original code portions and any parts now included in this new copy is asserted through 
 * their inclusion in the copyright notices below.
 *
 * Copyright (C) 2007 Canadian Bank Note, Limited
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
 * Contributor(s): Josh Mahonin (jmahonin@cbnco.com)
 *                 Michael Smith (msmith@cbnco.com)
 *                 Guilhem Tardy (gtardy@salyens.com)
 *                 Craig Southeren (craigs@postincrement.com)
 *
 * NOTES:
 * Initial implementation of MPEG4 codec plugin using ffmpeg.
 * Untested under Windows or H.323
 *
 * $Log: mpeg4.cxx,v $
 * Revision 1.8  2007/06/30 09:57:27  dsandras
 * Added patch from Matthias Schneider <ma30002000 yahoo de> to fix
 * dynamic payload type for H.264 and MPEG-4 codecs.
 * Added pipe mechanism for win32 for the H.264 codec.
 *
 * Revision 1.7  2007/06/16 21:46:40  dsandras
 * Cleanups, stability fixes, and documentation for the codec parameters.
 *
 * - document recommended encoder and decoder settings for various bitrates
 * - allow for older ffmpeg version
 * - fixes for stack alignment problems which were causing crashes with MMX
 * enabled in FFmpeg
 * - sweep "marker does not match f_code" errors under the rug to save CPU
 * - whitespace fixes: try to keep formatting consistent at least within the
 * same function :)
 * - ensure pointers are set to NULL on free to make bugs appear faster
 * - use "frame time" instead of fps, to match other video codecs
 * - don't open encoder until first frame is encoded - avoids needless reopens
 * after initial config changes
 * - always encode from _rawFrameBuffer since we know it has
 * FF_INPUT_BUFFER_PADDING_SIZE at the end
 * - remove some cerr's / cout's
 * - display I-frame even if it had errors so we aren't hosed for the rest of
 * the GOP
 * Thanks to Michael Smith <msmith cbnco com>.
 * Huge thanks for the contribution!
 *
 * Revision 1.6  2007/06/02 12:28:13  dsandras
 * Fixed various aspects of the build system for the MPEG-4 plugin thanks
 * to Michael Smith <msmith cbnco com>. Thanks a lot!
 *
 * Revision 1.5  2007/05/28 07:22:15  csoutheren
 * Changed to use compliant SDP name
 * Thanks to Matthias Schneider
 *
 */

/*
  Notes
  -----

  This codec implements an MPEG4 encoder/decoder using the ffmpeg library
  (http://ffmpeg.mplayerhq.hu/).  Plugin infrastructure is based off of the
  H.263 plugin and MPEG4 codec functions originally created by Michael Smith.

 */

// Plugin specific
#define _CRT_SECURE_NO_DEPRECATE
#include <codec/opalplugin.h>

extern "C" {
PLUGIN_CODEC_IMPLEMENT(FFMPEG_MPEG4)
};


#include <stdlib.h>

// Win32 or other
#ifdef _WIN32
#include <windows.h>
#include <malloc.h>
#define STRCMPI  _strcmpi
#else
#include <semaphore.h>
#include <dlfcn.h>
#define __STDC_CONSTANT_MACROS
#define STRCMPI  strcasecmp
#define FALSE false
#define TRUE  true
typedef unsigned char BYTE;
typedef bool BOOL;

#endif

// Needed C++ headers
#include <string.h>
#include <stdint.h>             // for uint8_t
#include <math.h>               // for rint() in dsputil.h
#include <assert.h>
#include <stdio.h>
#include <iostream>
#include <deque>

using namespace std;

// FFMPEG specific headers
extern "C" {

// Public ffmpeg headers.
// We'll pull them in from their locations in the ffmpeg source tree,
// but it would be possible to get them all from /usr/include/ffmpeg
// with #include <ffmpeg/...h>.
#include <libavutil/common.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavutil/log.h>

// Private headers from the ffmpeg source tree.
#include <libavutil/intreadwrite.h>
#include <libavutil/bswap.h>
#include <libavcodec/mpegvideo.h>
}

// Compile time version checking
// FFMPEG SVN from 20060817. This is what Packman shipped for SuSE 10.1.
// Should be recent enough.
#if LIBAVCODEC_VERSION_INT < ((51<<16)+(11<<8)+0)
#error Libavcodec too old.
#endif


/*
 * Some combination of gcc 3.3.5, glibc 2.3.5 and PWLib 1.11.3 is throwing
 * off stack alignment for ffmpeg when it is dynamically loaded by PWLib.
 * Wrapping all ffmpeg calls in this macro should ensure the stack is aligned
 * when it reaches ffmpeg. ffmpeg still needs to be compiled with a more
 * recent gcc (we used 4.1.1) to ensure it preserves stack boundaries
 * internally.
 *
 * This macro comes from FFTW 3.1.2, kernel/ifft.h. See:
 *     http://www.fftw.org/fftw3_doc/Stack-alignment-on-x86.html
 * Used with permission.
 */
#define WITH_ALIGNED_STACK(what)                                \
{                                                               \
     /*                                                         \
      * Use alloca to allocate some memory on the stack.        \
      * This alerts gcc that something funny is going           \
      * on, so that it does not omit the frame pointer          \
      * etc.                                                    \
      */                                                        \
     (void)__builtin_alloca(16);                                \
                                                                \
     /*                                                         \
      * Now align the stack pointer                             \
      */                                                        \
     __asm__ __volatile__ ("andl $-16, %esp");                  \
                                                                \
     what                                                       \
}



// WIN32 plugin path
#  ifdef  _WIN32
#    define P_DEFAULT_PLUGIN_DIR "C:\\PWLIB_PLUGINS"
#    define DIR_SEPERATOR "\\"
#    define DIR_TOKENISER ";"
#  else
#    define P_DEFAULT_PLUGIN_DIR "/usr/local/lib/pwlib"
#    define DIR_SEPERATOR "/"
#    define DIR_TOKENISER ":"
#  endif

#define RTP_DYNAMIC_PAYLOAD  96

#define MPEG4_CLOCKRATE     90000
#define MPEG4_BITRATE       3000000

#define CIF_WIDTH       352
#define CIF_HEIGHT      288

#define CIF4_WIDTH      (CIF_WIDTH*2)
#define CIF4_HEIGHT     (CIF_HEIGHT*2)

#define CIF16_WIDTH     (CIF_WIDTH*4)
#define CIF16_HEIGHT    (CIF_HEIGHT*4)

#define QCIF_WIDTH     (CIF_WIDTH/2)
#define QCIF_HEIGHT    (CIF_HEIGHT/2)

#define SQCIF_WIDTH     128
#define SQCIF_HEIGHT    96

#define MAX_MPEG4_PACKET_SIZE     2048





static void ffmpeg_printon(void * ptr, int level, const char *fmt, va_list vl)
{
    // Skip common decoder errors:
    // marker does not match f_code
    static const char * match = "marker do";
    if (strncmp(fmt, match, strlen(match)) == 0)
        return;

    char fmtbuf[256];
    if(ptr)
    {
        AVClass * avc = *(AVClass**) ptr;
        snprintf(fmtbuf, sizeof(fmtbuf), "[%s @ %p] %s",
                 avc->item_name(ptr), ptr, fmt);
    }
    else
    {
        strncpy(fmtbuf, fmt, sizeof(fmtbuf));
    }
    fmtbuf[sizeof(fmtbuf)-1] = '\0';
    vprintf(fmtbuf, vl);
}


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
    CriticalSection(const CriticalSection &)
    { }
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

class DynaLink
{
  public:
    typedef void (*Function)();

    DynaLink()
    { _hDLL = NULL; }

    ~DynaLink()
    { Close(); }

    virtual bool Open(const char *name)
    {
      // Look for the library in MPEG4_AVCODECDIR, if set.
      // (In that case, LD_LIBRARY_PATH must also be set so the linker can
      // find libavutil.so.49.)
      char * env = ::getenv("MPEG4_AVCODECDIR");
      if (env == NULL) {
        // Try to use the libavcodec.so in /usr/lib.
        return InternalOpen(NULL, name);
      }

      const char * token = strtok(env, DIR_TOKENISER);
      while (token != NULL) {
        if (InternalOpen(token, name))
          return true;
        token = strtok(NULL, DIR_TOKENISER);
      }
      return false;
    }

  // split into directories on correct seperator

    bool InternalOpen(const char * dir, const char *name)
    {
      char path[1024];
      memset(path, 0, sizeof(path));
      if (dir != NULL) {
        strcpy(path, dir);
        if (path[strlen(path)-1] != DIR_SEPERATOR[0]) 
          strcat(path, DIR_SEPERATOR);
      }
      strcat(path, name);

#ifdef _WIN32
# ifdef UNICODE
      USES_CONVERSION;
      _hDLL = LoadLibrary(A2T(path));
# else
      _hDLL = LoadLibrary(name);
# endif // UNICODE
#else
      _hDLL = dlopen((const char *)path, RTLD_NOW);
      if (_hDLL == NULL) {
        fprintf(stderr, "error loading %s", path);
        char * err = dlerror();
        if (err != NULL)
          fprintf(stderr, " - %s", err);
        fprintf(stderr, "\n");
      }
#endif // _WIN32
      return _hDLL != NULL;
    }

    virtual void Close()
    {
      if (_hDLL != NULL) {
#ifdef _WIN32
        FreeLibrary(_hDLL);
#else
        dlclose(_hDLL);
#endif // _WIN32
        _hDLL = NULL;
      }
    }


    virtual bool IsLoaded() const
    { return _hDLL != NULL; }

    bool GetFunction(const char * name, Function & func)
    {
      if (_hDLL == NULL)
        return FALSE;
#ifdef _WIN32

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
#else
      void * p = dlsym(_hDLL, (const char *)name);
      if (p == NULL)
        return FALSE;
      func = (Function &)p;
      return TRUE;
#endif // _WIN32
    }

  protected:
#if defined(_WIN32)
    HINSTANCE _hDLL;
#else
    void * _hDLL;
#endif // _WIN32
};

/////////////////////////////////////////////////////////////////
//
// define a class to interface to the FFMpeg library


class FFMPEGLibrary : public DynaLink
{
  public:
    FFMPEGLibrary();
    ~FFMPEGLibrary();

    bool Load();

    AVCodecContext *AvcodecAllocContext(void);
    AVFrame *AvcodecAllocFrame(void);
    int AvcodecOpen(AVCodecContext *ctx, AVCodec *codec);
    int AvcodecClose(AVCodecContext *ctx);
    int AvcodecEncodeVideo(AVCodecContext *ctx, BYTE *buf, int buf_size, const AVFrame *pict);
    int AvcodecDecodeVideo(AVCodecContext *ctx, AVFrame *pict, int *got_picture_ptr, BYTE *buf, int buf_size);
    void AvcodecFree(void * ptr);
    AVCodec *mpeg4_encoder;
    AVCodec *mpeg4_decoder;

    void AvcodecSetLogCallback(void (*print_fn)(void *, int, const char*, va_list));
    void AvcodecSetLogLevel(int);
    int (*Fff_check_alignment)(void);

    bool IsLoaded();
    CriticalSection processLock;

  protected:
    void (*Favcodec_init)(void);
    void (*Favcodec_register)(AVCodec *format);
    AVCodecContext *(*Favcodec_alloc_context)(void);
    void (*Favcodec_free)(void *);
    AVFrame *(*Favcodec_alloc_frame)(void);
    int (*Favcodec_open)(AVCodecContext *ctx, AVCodec *codec);
    int (*Favcodec_close)(AVCodecContext *ctx);
    int (*Favcodec_encode_video)(AVCodecContext *ctx, BYTE *buf, int buf_size, const AVFrame *pict);
    int (*Favcodec_decode_video)(AVCodecContext *ctx, AVFrame *pict, int *got_picture_ptr, BYTE *buf, int buf_size);
    void (*Favcodec_set_log_callback)(void (*log_callback)(void *, int, const char*, va_list));
    void (*Favcodec_set_log_level)(int);
    unsigned (*Favcodec_version)(void);
    unsigned (*Favcodec_build)(void);
    bool isLoadedOK;
};

static FFMPEGLibrary FFMPEGLibraryInstance;

//////////////////////////////////////////////////////////////////////////////

FFMPEGLibrary::FFMPEGLibrary()
:	Favcodec_init(NULL),
	Favcodec_register(NULL),
	Favcodec_alloc_context(NULL),
	Favcodec_free(NULL),
	Favcodec_alloc_frame(NULL),
	Favcodec_close(NULL),
	Favcodec_encode_video(NULL),
	Favcodec_decode_video(NULL),
	Favcodec_set_log_callback(NULL),
	Favcodec_set_log_level(NULL),
	Favcodec_version(NULL),
	Favcodec_build(NULL)
{
  isLoadedOK = FALSE;
}


// This must be called when the processLock mutex is held 
// Called by PLUGIN_CODEC_GET_CODEC_FN from the plugin DLL

bool FFMPEGLibrary::Load()
{
    // try open
  if (!DynaLink::Open("libavcodec.so")
      && !DynaLink::Open("libavcodec.so.51")
      && !DynaLink::Open("libavcodec")
      && !DynaLink::Open("avcodec")) {
    cerr << "MPEG4\tFailed to load ffmpeg library." << endl;
    cerr << "Ensure that the MPEG4_AVCODECDIR and LD_LIBRARY_PATH environment variables are set to point to a recent libavcodec.so." << endl;
    return false;
  }
 
  // load functions

  if(!GetFunction("avcodec_version", (Function &)Favcodec_version)){
    cerr << "Failed to load avcodec_version" << endl;
    return false;
  }
  
  if(!GetFunction("avcodec_build", (Function &)Favcodec_build)){
    cerr << "Failed to load avcodec_build" << endl;
    return false;
  }

  // make sure version's OK
  unsigned libVer = Favcodec_version();
  unsigned libBuild = Favcodec_build();
  if (libVer < LIBAVCODEC_VERSION_INT || libBuild < LIBAVCODEC_BUILD) {
    cerr << "Version mismatch: compiled against headers from ver/build "
         << std::hex << LIBAVCODEC_VERSION_INT << "/"
         << std::dec << LIBAVCODEC_BUILD
         << ", loaded " << std::hex << libVer << "/"
         << std::dec << libBuild << "." << endl;
    return false;
  }

  // continue loading
  if (!GetFunction("avcodec_init", (Function &)Favcodec_init)) {
    cerr << "Failed to load avcodec_int" << endl;
    return false;
  }

  if (!GetFunction("mpeg4_encoder", (Function &)mpeg4_encoder)) {
    cerr << "Failed to load mpeg4_encoder" << endl;
    return false;
  }

  if (!GetFunction("mpeg4_decoder", (Function &)mpeg4_decoder)) {
    cerr << "Failed to load mpeg4_decoder" << endl;
    return false;
  }

  if (!GetFunction("register_avcodec", (Function &)Favcodec_register)) {
    cerr << "Failed to load register_avcodec" << endl;
    return false;
  }

  if (!GetFunction("avcodec_alloc_context", (Function &)Favcodec_alloc_context))
  {
    cerr << "Failed to load avcodec_alloc_context" << endl;
    return false;
  }

  if (!GetFunction("avcodec_alloc_frame", (Function &)Favcodec_alloc_frame)) {
    cerr << "Failed to load avcodec_alloc_frame" << endl;
    return false;
  }

  if (!GetFunction("avcodec_open", (Function &)Favcodec_open)) {
    cerr << "Failed to load avcodec_open" << endl;
    return false;
  }

  if (!GetFunction("avcodec_close", (Function &)Favcodec_close)) {
    cerr << "Failed to load avcodec_close" << endl;
    return false;
  }

  if (!GetFunction("avcodec_encode_video", (Function &)Favcodec_encode_video)) {
    cerr << "Failed to load avcodec_encode_video" << endl;
    return false;
  }

  if (!GetFunction("avcodec_decode_video", (Function &)Favcodec_decode_video)) {
    cerr << "Failed to load avcodec_decode_video" << endl;
    return false;
  }


  if (!GetFunction("av_log_set_callback",
                   (Function &)Favcodec_set_log_callback))
  {
    cerr << "Failed to load av_log_set_callback" << endl;
    return false;
  }

  if (!GetFunction("av_log_set_level", (Function &)Favcodec_set_log_level)) {
    cerr << "Failed to load av_log_set_level" << endl;
    return false;
  }
   
  if (!GetFunction("av_free", (Function &)Favcodec_free)) {
    cerr << "Failed to load avcodec_close" << endl;
    return false;
  }

  if (!GetFunction("ff_check_alignment", (Function &) Fff_check_alignment)) {
    cerr << "Failed to load ff_check_alignment" << endl;
    return false;
  }

  WITH_ALIGNED_STACK({
    // must be called before using avcodec lib
    Favcodec_init();

    // register only the codecs needed (to have smaller code)
    Favcodec_register(mpeg4_encoder);
    Favcodec_register(mpeg4_decoder);

    Favcodec_set_log_callback(ffmpeg_printon);
    Favcodec_set_log_level(AV_LOG_QUIET); // QUIET, INFO, ERROR, DEBUG 
  })

  isLoadedOK = TRUE;

  return true;
}

FFMPEGLibrary::~FFMPEGLibrary()
{
  DynaLink::Close();
}

AVCodecContext *FFMPEGLibrary::AvcodecAllocContext(void)
{
  WITH_ALIGNED_STACK({
    return Favcodec_alloc_context();
  })
}

AVFrame *FFMPEGLibrary::AvcodecAllocFrame(void)
{
  WITH_ALIGNED_STACK({
    return Favcodec_alloc_frame();
  })
}

int FFMPEGLibrary::AvcodecOpen(AVCodecContext *ctx, AVCodec *codec)
{
  WaitAndSignal m(processLock);
  WITH_ALIGNED_STACK({
    return Favcodec_open(ctx, codec);
  })
}

int FFMPEGLibrary::AvcodecClose(AVCodecContext *ctx)
{
  WaitAndSignal m(processLock);
  WITH_ALIGNED_STACK({
    return Favcodec_close(ctx);
  })
}

int FFMPEGLibrary::AvcodecEncodeVideo(AVCodecContext *ctx, BYTE *buf,
                                      int buf_size, const AVFrame *pict)
{
  WITH_ALIGNED_STACK({
    return Favcodec_encode_video(ctx, buf, buf_size, pict);
  })
}

int FFMPEGLibrary::AvcodecDecodeVideo(AVCodecContext *ctx, AVFrame *pict,
                                      int *got_picture_ptr,
                                      BYTE *buf, int buf_size)
{
  WITH_ALIGNED_STACK({
    return Favcodec_decode_video(ctx, pict, got_picture_ptr, buf, buf_size);
  })
}

void FFMPEGLibrary::AvcodecSetLogCallback
                      (void (*log_callback)(void *, int, const char*, va_list))
{
  WITH_ALIGNED_STACK({
    Favcodec_set_log_callback(log_callback);
  })
}

void FFMPEGLibrary::AvcodecSetLogLevel(int log_level)
{
  WITH_ALIGNED_STACK({
    Favcodec_set_log_level(log_level);
  })
}

void FFMPEGLibrary::AvcodecFree(void * ptr)
{
  WITH_ALIGNED_STACK({
    Favcodec_free(ptr);
  })
}

bool FFMPEGLibrary::IsLoaded()
{
  return isLoadedOK;
}

/////////////////////////////////////////////////////////////////////////////
//
// define some simple RTP packet routines
//

#define RTP_MIN_HEADER_SIZE 12

class RTPFrame
{
  public:
    RTPFrame(const unsigned char * _packet, int _maxPacketLen)
      : packet((unsigned char *)_packet), maxPacketLen(_maxPacketLen), packetLen(_maxPacketLen)
    {
    }

    RTPFrame(unsigned char * _packet, int _maxPacketLen, unsigned char payloadType)
      : packet(_packet), maxPacketLen(_maxPacketLen), packetLen(_maxPacketLen)
    { 
      if (packetLen > 0)
        packet[0] = 0x80;    // set version, no extensions, zero contrib count
      SetPayloadType(payloadType);
    }

    inline unsigned long GetLong(unsigned offs) const
    {
      if (offs + 4 > packetLen)
        return 0;
      return (packet[offs + 0] << 24) + (packet[offs+1] << 16) + (packet[offs+2] << 8) + packet[offs+3]; 
    }

    inline void SetLong(unsigned offs, unsigned long n)
    {
      if (offs + 4 <= packetLen) {
        packet[offs + 0] = (BYTE)((n >> 24) & 0xff);
        packet[offs + 1] = (BYTE)((n >> 16) & 0xff);
        packet[offs + 2] = (BYTE)((n >> 8) & 0xff);
        packet[offs + 3] = (BYTE)(n & 0xff);
      }
    }

    inline unsigned short GetShort(unsigned offs) const
    { 
      if (offs + 2 > packetLen)
        return 0;
      return (packet[offs + 0] << 8) + packet[offs + 1]; 
    }

    inline void SetShort(unsigned offs, unsigned short n) 
    { 
      if (offs + 2 <= packetLen) {
        packet[offs + 0] = (BYTE)((n >> 8) & 0xff);
        packet[offs + 1] = (BYTE)(n & 0xff);
      }
    }

    inline int GetPacketLen() const                    { return packetLen; }
    inline int GetMaxPacketLen() const                 { return maxPacketLen; }
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
    inline unsigned char * GetPayloadPtr() const       { return packet + GetHeaderSize(); }

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

    inline bool SetPayloadSize(int payloadSize)      
    { 
      if (GetHeaderSize() + payloadSize > maxPacketLen)
        return true; 
      packetLen = GetHeaderSize() + payloadSize;
      return true;
    }

  protected:
    unsigned char * packet;
    unsigned maxPacketLen;
    unsigned packetLen;
};

/////////////////////////////////////////////////////////////////////////////
// Beginning of codec options, H323 untested

static const char * default_cif_mpeg4_options[][3] = {
//  { "h323_cifMPI",                               "<2" ,      "i" },
//  { "Max Bit Rate",                              "<327600" , "i" },
  { NULL, NULL, NULL }
};

static const char * default_qcif_mpeg4_options[][3] = {
//  { "h323_qcifMPI",                              "<1" ,      "i" },
//  { "Max Bit Rate",                              "<327600" , "i" },
  { NULL, NULL, NULL }
};

static const char * default_sip_mpeg4_options[][3] = {
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
  return get_xcif_options(context, parm, parmLen, &default_cif_mpeg4_options[0][0]);
}

static int coder_get_qcif_options(
      const PluginCodec_Definition * , 
      void * context, 
      const char * , 
      void * parm, 
      unsigned * parmLen)
{
  return get_xcif_options(context, parm, parmLen, &default_qcif_mpeg4_options[0][0]);
}

static int coder_get_sip_options(
      const PluginCodec_Definition *, 
      void * context, 
      const char * , 
      void * parm, 
      unsigned * parmLen)
{
  return get_xcif_options(context, parm, parmLen, &default_sip_mpeg4_options[0][0]);
}

static int valid_for_sip(
      const PluginCodec_Definition * , 
      void * context , 
      const char * , 
      void * parm , 
      unsigned * parmLen)
{
  if (parmLen == NULL || parm == NULL || *parmLen != sizeof(char *))
    return 0;

  return (STRCMPI((const char *)parm, "sip") == 0) ? 1 : 0;
}

static int valid_for_h323(
      const PluginCodec_Definition * , 
      void * , 
      const char * , 
      void * parm , 
      unsigned * parmLen)
{
  if (parmLen == NULL || parm == NULL || *parmLen != sizeof(char *))
    return 0;

  return (STRCMPI((const char *)parm, "h.323") == 0 ||
          STRCMPI((const char *)parm, "h323") == 0) ? 1 : 0;
}

/////////////////////////////////////////////////////////////////////////////
//
// define the encoding context
//

class MPEG4EncoderContext
{
  public:

    MPEG4EncoderContext();
    ~MPEG4EncoderContext();

    int EncodeFrames(const BYTE * src, unsigned & srcLen,
                     BYTE * dst, unsigned & dstLen, unsigned int & flags);
    static void RtpCallback(AVCodecContext *ctx, void *data, int data_size,
                            int num_mb);
    
    void SetIQuantFactor(float newFactor);
    void SetThrottle(bool enable);
    void SetForceKeyframeUpdate(bool enable);
    void SetKeyframeUpdatePeriod(int interval);
    void SetMaxBitrate(int max);
    void SetFPS(int frameTime);
    void SetFrameHeight(int height);
    void SetFrameWidth(int width);
    void SetQMin(int qmin);
    void SetQMax(int qmax);
    void SetQuality(int qual);
    void SetDynamicVideo(bool enable);

    int GetFrameBytes();

  protected:
    BOOL OpenCodec();
    void CloseCodec();

    // sets encoding paramters
    void SetDynamicEncodingParams(bool restartOnResize);
    void SetStaticEncodingParams();
    void ResizeEncodingFrame(bool restartCodec);

    // reset libavcodec rate control
    void ResetBitCounter(int spread);

    // Modifiable quantization factor.  Defaults to -0.8
    float _iQuantFactor;

    // Automatic IFrame updates.  Defaults to false
    BOOL _forceKeyframeUpdate;

    // Interval in seconds between forced IFrame updates if enabled
    int _keyframeUpdatePeriod;


    // Bandwidth throttling.  Defaults to false.
    BOOL _doThrottle;

    // Modifiable upper limit for bits/s transferred
    int _bitRateHighLimit;

    // Frames per second.  Defaults to 24.
    int _targetFPS;

    // Let mpeg4 decide video quality?
    bool _dynamicVideo;
    
    // packet sizes generating in RtpCallback
    deque<unsigned> _packetSizes;
    unsigned _lastPktOffset;

    // raw and encoded frame buffers
    BYTE * _encFrameBuffer;
    unsigned int _encFrameLen;
    BYTE * _rawFrameBuffer;
    unsigned int _rawFrameLen;

    // codec, context and picture
    AVCodec        *_avcodec;
    AVCodecContext *_avcontext;
    AVFrame        *_avpicture;

    // encoding and frame settings
    int _videoQMax, _videoQMin; // dynamic video quality min/max limits, 1..31
    int _videoQuality; // current video encode quality setting, 1..31

    int _frameNum;
    unsigned int _frameWidth;
    unsigned int _frameHeight;

    unsigned long _lastTimeStamp;
    time_t _lastKeyframe;

    enum StdSize { 
      SQCIF, 
      QCIF, 
      CIF, 
      CIF4, 
      CIF16, 
      NumStdSizes,
      UnknownStdSize = NumStdSizes
    };

    // simple bandwidth throttler
    class Throttle {
        public:
            Throttle(int slots) : _slots(NULL) {
                reset(slots);
            }
            ~Throttle(){
                delete[] _slots;
            }
            BOOL throttle(int maxbytes) {
                if(_total > maxbytes) {
                    // Don't drop two in a row
                    if (_numSlots == 1 || _slots[(_index - 1) % _numSlots] != 0)
                    {
                        record(0);
                        return TRUE;
                    }
                }
                return FALSE;
            }
            void record(int bytes){
                _total -= _slots[_index];
                _slots[_index] = bytes;
                _total += bytes;
                _index = (_index + 1) % _numSlots;
            }
            void reset(int slots){
                _total = _index = 0;
                if(slots < 1)
                    slots = 1;
                _numSlots = slots;
                if(_slots){
                    delete _slots;
                }
                _slots = new int[slots];
                for(int i = 0; i < slots; ++i)
                    _slots[i] = 0;
            }
        private:
            int _total;
            int _numSlots;
            int _index;
            int * _slots;    
    } * _throttle; 

    static int GetStdSize(int width, int height)
    {
      static struct { 
        int width; 
        int height; 
      } StandardVideoSizes[NumStdSizes] = {
        {  128,   96}, // SQCIF
        {  176,  144}, // QCIF
        {  352,  288}, // CIF
        {  704,  576}, // 4CIF
        { 1408, 1152}, // 16CIF
      };

      int sizeIndex;
      for (sizeIndex = 0; sizeIndex < NumStdSizes; ++sizeIndex )
        if (StandardVideoSizes[sizeIndex].width == width
            && StandardVideoSizes[sizeIndex].height == height)
          return sizeIndex;
      return UnknownStdSize;
    }
};

/////////////////////////////////////////////////////////////////////////////
//
//  Encoding context constructor.  Sets some encoding parameters, registers the
//  codec and allocates the context and frame.
//

MPEG4EncoderContext::MPEG4EncoderContext() 
:   _encFrameBuffer(NULL),
    _rawFrameBuffer(NULL), 
    _avcodec(NULL),
    _avcontext(NULL),
    _avpicture(NULL),
    _doThrottle(false),
    _forceKeyframeUpdate(false),
    _dynamicVideo(false),
    _throttle(new Throttle(1))
{ 

  // Some sane default video settings
  _targetFPS = 24;
  _videoQMin = 2;
  _videoQMax = 20;
  _videoQuality = 12;
  _iQuantFactor = -0.8f;

  _keyframeUpdatePeriod = 3; // 3 seconds between forced keyframes, if enabled

  _frameNum = 0;
  _lastPktOffset = 0;
  _lastKeyframe = time(NULL);
  

  if (!FFMPEGLibraryInstance.IsLoaded()){
    return;
  }


  // Default frame size.  These may change after encoder_set_options
  _frameWidth  = CIF_WIDTH;
  _frameHeight = CIF_HEIGHT;
  _rawFrameLen = (_frameWidth * _frameHeight * 3) / 2;
}

/////////////////////////////////////////////////////////////////////////////
//
//  Close the codec and free our pointers
//

MPEG4EncoderContext::~MPEG4EncoderContext()
{
  if (FFMPEGLibraryInstance.IsLoaded()) {
    CloseCodec();
  }
  if (_rawFrameBuffer) {
    delete[] _rawFrameBuffer;
    _rawFrameBuffer = NULL;
  }
  if (_encFrameBuffer) {
    delete[] _encFrameBuffer;
    _encFrameBuffer = NULL;
  }
  if (_throttle) {
    delete _throttle;
    _throttle = NULL;
  }
}

/////////////////////////////////////////////////////////////////////////////
//
// Returns the number of bytes / frame.  This is called from 
// encoder_get_output_data_size to get an accurate frame size
//

int MPEG4EncoderContext::GetFrameBytes() {
    return _frameWidth * _frameHeight;
}


/////////////////////////////////////////////////////////////////////////////
//
// Setter function for _iQuantFactor.  This may be called from 
// encoder_set_options if the "IQuantFactor" real option has been passed 
//

void MPEG4EncoderContext::SetIQuantFactor(float newFactor) {
    _iQuantFactor = newFactor;
}

/////////////////////////////////////////////////////////////////////////////
//
// Setter function for _doThrottle. This is called from encoder_set_options
// if the "Bandwidth Throttling" boolean option is passed 

void MPEG4EncoderContext::SetThrottle(bool throttle) {
    _doThrottle = throttle;
}

/////////////////////////////////////////////////////////////////////////////
//
// Setter function for _forceKeyframeUpdate.  This is called from 
// encoder_set_options if the "Force Keyframe Update" boolean option is passed

void MPEG4EncoderContext::SetForceKeyframeUpdate(bool enable) {
    _forceKeyframeUpdate = enable;
}

/////////////////////////////////////////////////////////////////////////////
//
// Setter function for _keyframeUpdatePeriod.  This is called from 
// encoder_set_options if the "Keyframe Update Period" integer option is passed

void MPEG4EncoderContext::SetKeyframeUpdatePeriod(int interval) {
    _keyframeUpdatePeriod = interval;
}

/////////////////////////////////////////////////////////////////////////////
//
// Setter function for _maxBitRateLimit. This is called from encoder_set_options
// if the "MaxBitrate" integer option is passed 

void MPEG4EncoderContext::SetMaxBitrate(int max) {
    _bitRateHighLimit = max;
}

/////////////////////////////////////////////////////////////////////////////
//
// Setter function for _targetFPS. This is called from encoder_set_options
// if the "FPS" integer option is passed 

void MPEG4EncoderContext::SetFPS(int frameTime) {
    _targetFPS = (MPEG4_CLOCKRATE / frameTime);
}


/////////////////////////////////////////////////////////////////////////////
//
// Setter function for _frameWidth. This is called from encoder_set_options
// when the "Frame Width" integer option is passed 

void MPEG4EncoderContext::SetFrameWidth(int width) {
    _frameWidth = width;
}

/////////////////////////////////////////////////////////////////////////////
//
// Setter function for _frameHeight. This is called from encoder_set_options
// when the "Frame Height" integer option is passed 

void MPEG4EncoderContext::SetFrameHeight(int height) {
    _frameHeight = height;
}

/////////////////////////////////////////////////////////////////////////////
//
// Setter function for _videoQMin. This is called from encoder_set_options
// when the "Minimum Quality" integer option is passed

void MPEG4EncoderContext::SetQMin(int qmin) {
    _videoQMin = qmin;
}

/////////////////////////////////////////////////////////////////////////////
//
// Setter function for _videoQMax. This is called from encoder_set_options
// when the "Maximum Quality" integer option is passed 

void MPEG4EncoderContext::SetQMax(int qmax) {
    _videoQMax = qmax;
}

/////////////////////////////////////////////////////////////////////////////
//
// Setter function for _frameHeight. This is called from encoder_set_options
// when the "Encoding Quality" integer option is passed 

void MPEG4EncoderContext::SetQuality(int qual) {
    _videoQuality = qual;
}

/////////////////////////////////////////////////////////////////////////////
//
// Setter function for _dynamicQuality. This is called from encoder_set_options
// when the "Dynamic Video Quality" boolean option is passed 

void MPEG4EncoderContext::SetDynamicVideo(bool enable) {
    _dynamicVideo = enable;
}

/////////////////////////////////////////////////////////////////////////////
//
//  The ffmpeg rate control averages bit usage over an entire file.
//  We're only interested in instantaneous usage. Past periods of
//  over-bandwidth or under-bandwidth shouldn't affect the next
//  frame. So we reset the total_bits counter toward what it should be,
//  picture_number * bit_rate / fps,
//  bit by bit as we go.
//
//  We also reset the vbv buffer toward the value where it doesn't
//  affect quantization - rc_buffer_size/2.
//

void MPEG4EncoderContext::ResetBitCounter(int spread) {
    MpegEncContext *s = (MpegEncContext *) _avcontext->priv_data;
    int64_t wanted_bits
        = int64_t(s->bit_rate * double(s->picture_number)
                  * av_q2d(_avcontext->time_base));
    s->total_bits += (wanted_bits - s->total_bits) / int64_t(spread);

    double want_buffer = double(_avcontext->rc_buffer_size / 2);
    s->rc_context.buffer_index
        += (want_buffer - s->rc_context.buffer_index) / double(spread);
}


/////////////////////////////////////////////////////////////////////////////
//
// Set static encoding parameters.  These should be values that remain
// unchanged through the duration of the encoding context.
//

void MPEG4EncoderContext::SetStaticEncodingParams(){
    _avcontext->pix_fmt = PIX_FMT_YUV420P;
    _avcontext->mb_decision = FF_MB_DECISION_SIMPLE;    // high quality off
    _avcontext->rtp_mode = 1;                           // use RTP packetization
    _avcontext->rtp_payload_size = 750;                 // ffh263 uses 750
    _avcontext->rtp_callback = &MPEG4EncoderContext::RtpCallback;

    // Reduce the difference in quantization between frames.
    _avcontext->qblur = 0.3;
    // default is tex^qComp; 1 is constant bitrate
    _avcontext->rc_eq = "1";
    //avcontext->rc_eq = "tex^qComp";
    // These ones technically could be dynamic, I think
    _avcontext->rc_min_rate = 0;
    // This is set to 0 in ffmpeg.c, the command-line utility.
    _avcontext->rc_initial_cplx = 0.0f;

    // And this is set to 1.
    // It seems to affect how aggressively the library will raise and lower
    // quantization to keep bandwidth constant. Except it's in reference to
    // the "vbv buffer", not bits per second, so nobody really knows how
    // it works.
    _avcontext->rc_buffer_aggressivity = 1.0f;

    // Ratecontrol buffer size, in bits. Usually 0.5-1 second worth.
    // 224 kbyte is what VLC uses, and it seems to fix the quantization pulse.
    _avcontext->rc_buffer_size = 224*1024*8;

    // In MEncoder this defaults to 1/4 buffer size, but in ffmpeg.c it
    // defaults to 3/4. I think the buffer is supposed to stabilize at
    // about half full. Note that setting this after avcodec_open() has
    // no effect.
    _avcontext->rc_initial_buffer_occupancy = _avcontext->rc_buffer_size * 1/2;

    // Defaults to -0.8, which is good for high bandwidth and not negative
    // enough for low bandwidth.
    _avcontext->i_quant_factor = _iQuantFactor;
    _avcontext->i_quant_offset = 0.0;

    // Set our initial target FPS, gop_size and create throttler
    _avcontext->time_base.num = 1;
    _avcontext->time_base.den = _targetFPS;

    // Number of frames for a group of pictures
    _avcontext->gop_size = _avcontext->time_base.den * 8;
    _throttle->reset(_avcontext->time_base.den / 2);
    if (_dynamicVideo) {
        // Set the initial frame quality to something sane
        _avpicture->quality = _videoQuality;  
    }
    else {
        // Adjust bitrate to get quantizer
        // Frame quality will be set to _videoQuality on every
        // 'SetDynamicEncodingParams()'
        _avcontext->flags |= CODEC_FLAG_QSCALE;
    }

    _avcontext->flags |= CODEC_FLAG_PART;   // data partitioning
    _avcontext->flags |= CODEC_FLAG_4MV;    // 4 motion vectors
    _avcontext->opaque = this;              // for use in RTP callback
}

/////////////////////////////////////////////////////////////////////////////
//
// Set dynamic encoding parameters.  These are values that may change over the
// encoding context's lifespan
//

void MPEG4EncoderContext::SetDynamicEncodingParams(bool restartOnResize) {
    // If no bitrate limit is set, max out at 3 mbit
    // Use 75% of available bandwidth so not as many frames are dropped
    unsigned bitRate
        = (_bitRateHighLimit ? 3*_bitRateHighLimit/4 : MPEG4_BITRATE);
    _avcontext->bit_rate = bitRate;

    // In ffmpeg this is the tolerance over the entire file. We reset the
    // bit counter toward the expected value a little bit every frame,
    // so this probably needs to be readjusted.
    _avcontext->bit_rate_tolerance = bitRate;

    // The rc_* values affect the quantizer. Not even sure what bit_rate does.
    _avcontext->rc_max_rate = bitRate;

    // Update the quantization factor
    _avcontext->i_quant_factor = _iQuantFactor;

    // Set full-frame min/max quantizers.
    // Note: mb_qmin, mb_max don't seem to be used in the libavcodec code.

    _avcontext->qmin = _videoQMin;
    _avcontext->qmax = _videoQMax;

    // Lagrange multipliers - this is how the context defaults do it:
    _avcontext->lmin = _videoQMin * FF_QP2LAMBDA;
    _avcontext->lmax = _videoQMax * FF_QP2LAMBDA;

    if(!_dynamicVideo){
        // Force the quantizer to be _videoQuality
        _avpicture->quality = _videoQuality;
    }

    // If framesize has changed or is not yet initialized, fix it up
    if(_avcontext->width != _frameWidth || _avcontext->height != _frameHeight) {
        ResizeEncodingFrame(restartOnResize);
    }
}

/////////////////////////////////////////////////////////////////////////////
//
// Updates the context's frame size and creates new buffers
//

void MPEG4EncoderContext::ResizeEncodingFrame(bool restartCodec) {
    _avcontext->width = _frameWidth;
    _avcontext->height = _frameHeight;

    // Restart to force avcodec to use the new frame sizes
    if (restartCodec) {
        CloseCodec();
        OpenCodec();
    }

    _rawFrameLen = (_frameWidth * _frameHeight * 3) / 2;
    if (_rawFrameBuffer)
    {
        delete[] _rawFrameBuffer;
    }
    _rawFrameBuffer = new BYTE[_rawFrameLen + FF_INPUT_BUFFER_PADDING_SIZE];

    if (_encFrameBuffer)
    {
        delete[] _encFrameBuffer;
    }
    _encFrameLen = _rawFrameLen/2;         // assume at least 50% compression...
    _encFrameBuffer = new BYTE[_encFrameLen];

    // Clear the back padding
    memset(_rawFrameBuffer + _rawFrameLen, 0, FF_INPUT_BUFFER_PADDING_SIZE);
    const unsigned fsz = _frameWidth * _frameHeight;
    _avpicture->data[0] = _rawFrameBuffer;              // luminance
    _avpicture->data[1] = _rawFrameBuffer + fsz;        // first chroma channel
    _avpicture->data[2] = _avpicture->data[1] + fsz/4;  // second
    _avpicture->linesize[0] = _frameWidth;
    _avpicture->linesize[1] = _avpicture->linesize[2] = _frameWidth/2;
}

/////////////////////////////////////////////////////////////////////////////
//
// Initializes codec parameters and opens the encoder
//

BOOL MPEG4EncoderContext::OpenCodec()
{
  _avcontext = FFMPEGLibraryInstance.AvcodecAllocContext();
  if (_avcontext == NULL) {
    return FALSE;
  }

  _avpicture = FFMPEGLibraryInstance.AvcodecAllocFrame();
  if (_avpicture == NULL) {
    return FALSE;
  }

  if((_avcodec = FFMPEGLibraryInstance.mpeg4_encoder) == NULL){
    return FALSE;
  }

  // debugging flags
  //_avcontext->debug |= FF_DEBUG_RC;
  //_avcontext->debug |=  FF_DEBUG_PICT_INFO;
  //_avcontext->debug |=  FF_DEBUG_MV;

  SetStaticEncodingParams();
  SetDynamicEncodingParams(false);    // don't force a restart, it's not open
  if (FFMPEGLibraryInstance.AvcodecOpen(_avcontext, _avcodec) < 0)
  {
    return FALSE;
  }
  return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
//
// Close the codec
//

void MPEG4EncoderContext::CloseCodec()
{
  if (_avcontext != NULL) {
    if(_avcontext->codec != NULL)
      FFMPEGLibraryInstance.AvcodecClose(_avcontext);
    FFMPEGLibraryInstance.AvcodecFree(_avcontext);
    _avcontext = NULL;
  }
  if (_avpicture != NULL) {
    FFMPEGLibraryInstance.AvcodecFree(_avpicture);
    _avpicture = NULL;
  }
}

/////////////////////////////////////////////////////////////////////////////
//
// This is called after an Favcodec_encode_frame() call.  This populates the 
// _packetSizes deque with offsets, no greated than max_rtp which we will use
// to create RTP packets.
//

void MPEG4EncoderContext::RtpCallback(AVCodecContext *priv_data, void *data,
                                      int size, int num_mb)
{
    const int max_rtp = 1350;
    while (size > 0)
    {
        MPEG4EncoderContext *c
            = static_cast<MPEG4EncoderContext *>(priv_data->opaque);
        c->_packetSizes.push_back((size < max_rtp ? size : max_rtp));
        size -= max_rtp;
    }
}

/////////////////////////////////////////////////////////////////////////////
//
// The main encoding loop.  If there are no packets ready to be sent, generate
// them by encoding a frame.  When there are, create packets from the encoded
// frame buffer and send them out
//

int MPEG4EncoderContext::EncodeFrames(const BYTE * src, unsigned & srcLen,
                                      BYTE * dst, unsigned & dstLen,
                                      unsigned int & flags)
{
    if (!FFMPEGLibraryInstance.IsLoaded()){
        return 0;
    }

    // create frames frame from their respective buffers
    RTPFrame srcRTP(src, srcLen);
    RTPFrame dstRTP(dst, MAX_MPEG4_PACKET_SIZE);

    // create the video frame header from the source and update video size
    PluginCodec_Video_FrameHeader * header
        = (PluginCodec_Video_FrameHeader *)srcRTP.GetPayloadPtr();
    _frameWidth = header->width;
    _frameHeight = header->height;
    dstLen = 0;

    // Check if we're throttling bandwidth.  Throttle over bitrate/8 (bytes)
    // then divide by 2 for a half second period
    if (_doThrottle && _throttle->throttle(_bitRateHighLimit >> 4)) {
        // Throttled, don't generate packets
        fprintf(stderr, "mpeg4: throttling frame\n");
    }
    else if (_packetSizes.empty()) {
        if (_avcontext == NULL) {
            OpenCodec();
        }
        else {
            // set our dynamic parameters, restart the codec if we need to
            SetDynamicEncodingParams(true);
        }
        _lastTimeStamp = srcRTP.GetTimestamp();
        _lastPktOffset = 0; 

        // generate the raw picture
        memcpy(_rawFrameBuffer, OPAL_VIDEO_FRAME_DATA_PTR(header),
              _rawFrameLen);

        time_t now = time(NULL);
        // Should the next frame be an I-Frame?
        if ((_forceKeyframeUpdate
             && (now - _lastKeyframe > _keyframeUpdatePeriod))
            || (flags & PluginCodec_CoderForceIFrame) || (_frameNum == 0))
        {
            _lastKeyframe = now;
            _avpicture->pict_type = FF_I_TYPE;
            flags = PluginCodec_ReturnCoderIFrame;
        }
        else // No IFrame requested, let avcodec decide what to do
        {
            _avpicture->pict_type = 0;
        }

        // Encode a frame
        int total = FFMPEGLibraryInstance.AvcodecEncodeVideo
                            (_avcontext, _encFrameBuffer, _encFrameLen,
                             _avpicture);
        if (total > 0) {
            _frameNum++; // increment the number of frames encoded
            ResetBitCounter(8); // Fix ffmpeg rate control
            _throttle->record(total); // record frames for throttler
        }
    }

    // _packetSizes should not be empty unless we've been throttled
    if(_packetSizes.empty() == false)
    {
        // Grab the next payload size and store it in dstLen
        dstLen = _packetSizes.front();
        _packetSizes.pop_front();
        dstRTP.SetPayloadSize(dstLen); 

        // Copy the encoded data from the buffer into the outgoign RTP 
        memcpy(dstRTP.GetPayloadPtr(), &_encFrameBuffer[_lastPktOffset],
               dstLen);
        _lastPktOffset += dstLen;

        // If there are no more packet sizes left, we've reached the last packet
        // for the frame, set the marker bit and flags
        if (_packetSizes.empty()) {
            dstRTP.SetMarker(TRUE);
            flags |= PluginCodec_ReturnCoderLastFrame;
        }

        // set timestamp and adjust dstLen to include header size
        dstRTP.SetTimestamp(_lastTimeStamp);
        dstLen+= dstRTP.GetHeaderSize();
    }
    else
    {
        // throttled, tell OPAL to send a 0 length packet
        dstLen = 0;
        dstRTP.SetPayloadSize(0);
        dstRTP.SetMarker(TRUE);
    }
    return 1;
}

/////////////////////////////////////////////////////////////////////////////
//
// OPAL plugin encoder functions
//

static void * create_encoder(const struct PluginCodec_Definition * /*codec*/)
{
  return new MPEG4EncoderContext;
}

// This is assuming the second param is the context...
static int encoder_set_options(
      const PluginCodec_Definition * , 
      void * _context, 
      const char * , 
      void *parm, 
      unsigned *parmLen )
{
  if (parmLen == NULL || *parmLen != sizeof(const char **))
    return 0;

  MPEG4EncoderContext * context = (MPEG4EncoderContext *)_context;
  // Scan the options passed in and check for:
  // 1) "Frame Width" sets the encoding frame width
  // 2) "Frame Height" sets the encoding frame height
  // 3) "Max Bit Rate" sets the bitrate upper limit
  // 4) "Minimum Quality" sets the minimum encoding quality
  // 5) "Maximum Quality" sets the maximum encoding quality
  // 6) "Encoding Quality" sets the default encoding quality
  // 7) "Dynamic Encoding Quality" enables dynamic adjustment of encoding quality
  // 8) "Bandwidth Throttling" will turn on bandwidth throttling for the encoder
  // 9) "IQuantFactor" will update the quantization factor to a float value
  // 10) "Force Keyframe Update": force the encoder to make an IFrame every 3s
  // 11) "Keyframe Update Period" interval in seconds before an IFrame is sent
  // 12) "Frame Time" lets the encoder and throttler know target frames per second 

  if (parm != NULL) {
    const char ** options = (const char **)parm;
    for (int i = 0; options[i] != NULL; i += 2) {
      if (STRCMPI(options[i], "Frame Width") == 0)
        context->SetFrameWidth(atoi(options[i+1]));
      else if(STRCMPI(options[i], "Frame Height") == 0)
        context->SetFrameHeight(atoi(options[i+1]));
      else if(STRCMPI(options[i], "Max Bit Rate") == 0)
        context->SetMaxBitrate(atoi(options[i+1]));
      else if(STRCMPI(options[i], "Minimum Quality") == 0)
        context->SetQMin(atoi(options[i+1]));
      else if(STRCMPI(options[i], "Maximum Quality") == 0)
        context->SetQMax(atoi(options[i+1]));
      else if(STRCMPI(options[i], "Encoding Quality") == 0)
        context->SetQuality(atoi(options[i+1]));
      else if(STRCMPI(options[i], "Dynamic Video Quality") == 0)
        context->SetDynamicVideo(atoi(options[i+1]));
      else if(STRCMPI(options[i], "Bandwidth Throttling") == 0)
        context->SetThrottle(atoi(options[i+1]));
      else if(STRCMPI(options[i], "IQuantFactor") == 0)
        context->SetIQuantFactor(atof(options[i+1]));
      else if(STRCMPI(options[i], "Force Keyframe Update") == 0)
        context->SetForceKeyframeUpdate(atoi(options[i+1]));
      else if(STRCMPI(options[i], "Keyframe Update Period") == 0)
        context->SetKeyframeUpdatePeriod(atoi(options[i+1]));
      else if(STRCMPI(options[i], "Frame Time") == 0)
        context->SetFPS(atoi(options[i+1]));
    }
  }
  return 1;
}

static void destroy_encoder(const struct PluginCodec_Definition * /*codec*/,
                            void * _context)
{
  MPEG4EncoderContext * context = (MPEG4EncoderContext *)_context;
  delete context;
}

static int codec_encoder(const struct PluginCodec_Definition * , 
                                           void * _context,
                                     const void * from, 
                                       unsigned * fromLen,
                                           void * to,         
                                       unsigned * toLen,
                                   unsigned int * flags)
{
  MPEG4EncoderContext * context = (MPEG4EncoderContext *)_context;
  return context->EncodeFrames((const BYTE *)from, *fromLen, (BYTE *)to, *toLen,
                               *flags);
}

static int encoder_get_output_data_size(const PluginCodec_Definition * codec, void *_context, const char *, void *, unsigned *)
{
  MPEG4EncoderContext * context = (MPEG4EncoderContext *)_context;
  return context->GetFrameBytes() * 3 / 2;
}

static PluginCodec_ControlDefn cifEncoderControls[] = {
  { "valid_for_protocol",       valid_for_h323 },
  { "get_codec_options",    coder_get_cif_options },
  { "set_codec_options",    encoder_set_options },
  { "get_output_data_size", encoder_get_output_data_size },
  { NULL }
};

static PluginCodec_ControlDefn qcifEncoderControls[] = {
  { "valid_for_protocol",       valid_for_h323 },
  { "get_codec_options",    coder_get_qcif_options },
  { "set_codec_options",    encoder_set_options },
  { "get_output_data_size", encoder_get_output_data_size },
  { NULL }
};

static PluginCodec_ControlDefn sipEncoderControls[] = {
  { "valid_for_protocol",       valid_for_sip },
  { "get_codec_options",    coder_get_sip_options },
  { "set_codec_options",    encoder_set_options },
  { "get_output_data_size", encoder_get_output_data_size },
  { NULL }
};

/////////////////////////////////////////////////////////////////////////////
//
// Define the decoder context
//

class MPEG4DecoderContext
{
  public:
    MPEG4DecoderContext();
    ~MPEG4DecoderContext();

    bool DecodeFrames(const BYTE * src, unsigned & srcLen,
                      BYTE * dst, unsigned & dstLen, unsigned int & flags);

    bool DecoderError(int threshold);
    void SetErrorRecovery(BOOL error);
    void SetErrorThresh(int thresh);
    void SetDisableResize(bool disable);

    void SetFrameHeight(int height);
    void SetFrameWidth(int width);

    int GetFrameBytes();

  protected:

    bool OpenCodec();
    void CloseCodec();

    void SetStaticDecodingParams();
    void SetDynamicDecodingParams(bool restartOnResize);
    void ResizeDecodingFrame(bool restartCodec);

    BYTE * _encFrameBuffer;
    unsigned int _encFrameLen;

    AVCodec        *_avcodec;
    AVCodecContext *_avcontext;
    AVFrame        *_avpicture;

    int _frameNum;

    bool _doError;
    int _keyRefreshThresh;

    bool _disableResize;

    unsigned _lastPktOffset;
    unsigned int _frameWidth;
    unsigned int _frameHeight;
};

/////////////////////////////////////////////////////////////////////////////
//
// Constructor.  Register the codec, allocate the context and frame and set
// some parameters
//

MPEG4DecoderContext::MPEG4DecoderContext() 
:   _encFrameBuffer(NULL),
    _doError(true),
    _disableResize(false), 
    _keyRefreshThresh(1),
    _frameNum(0),
    _lastPktOffset(0),
    _frameWidth(0),
    _frameHeight(0)
{
  if (!FFMPEGLibraryInstance.IsLoaded()) {
    return;
  }

  // Default frame sizes.  These may change after decoder_set_options is called
  _frameWidth  = CIF_WIDTH;
  _frameHeight = CIF_HEIGHT;

  OpenCodec();
}

/////////////////////////////////////////////////////////////////////////////
//
// Close the codec, free the context, frame and buffer
//

MPEG4DecoderContext::~MPEG4DecoderContext()
{
  if (FFMPEGLibraryInstance.IsLoaded()) {
    CloseCodec();
  }
  if(_encFrameBuffer) {
    delete[] _encFrameBuffer;
    _encFrameBuffer = NULL;
  }
}

/////////////////////////////////////////////////////////////////////////////
//
// Returns the number of bytes / frame.  This is called from 
// decoder_get_output_data_size to get an accurate frame size
//

int MPEG4DecoderContext::GetFrameBytes() {
    return _frameWidth * _frameHeight;
}



/////////////////////////////////////////////////////////////////////////////
//
// Setter for decoding error recovery.  Called from decoder_set_options if
// the user passes the "Error Recovery" option.
//

void MPEG4DecoderContext::SetErrorRecovery(BOOL error) {
    _doError = error;
}

/////////////////////////////////////////////////////////////////////////////
//
// Setter for DecoderError threshold.  Called from decoder_set_options if
// the user passes "Error Threshold" option.
//

void MPEG4DecoderContext::SetErrorThresh(int thresh) {
    _keyRefreshThresh = thresh;
}

/////////////////////////////////////////////////////////////////////////////
//
// Setter for the automatic resize boolean.  Called from decoder_set_options if
// the user passes "Disable Resize" option.
//

void MPEG4DecoderContext::SetDisableResize(bool disable) {
    _disableResize = disable;
}


/////////////////////////////////////////////////////////////////////////////
//
// Setter function for _frameWidth. This is called from decoder_set_options
// when the "Frame Width" integer option is passed 

void MPEG4DecoderContext::SetFrameWidth(int width) {
    _frameWidth = width;
}

/////////////////////////////////////////////////////////////////////////////
//
// Setter function for _frameHeight. This is called from decoder_set_options
// when the "Frame Height" integer option is passed 

void MPEG4DecoderContext::SetFrameHeight(int height) {
    _frameHeight = height;
}

/////////////////////////////////////////////////////////////////////////////
//
// Sets static decoding parameters.
//

void MPEG4DecoderContext::SetStaticDecodingParams() {
    _avcontext->flags |= CODEC_FLAG_4MV; 
    _avcontext->flags |= CODEC_FLAG_PART;
    _avcontext->workaround_bugs = 0; // no workaround for buggy implementations
}


/////////////////////////////////////////////////////////////////////////////
//
// Check for errors on I-Frames.  If we found one, ask for another.
//

bool MPEG4DecoderContext::DecoderError(int threshold) {
    if (_doError) {
        int errors = 0;
        MpegEncContext *s = (MpegEncContext *) _avcontext->priv_data;
        if (s->error_count && _avcontext->coded_frame->pict_type == FF_I_TYPE) {
            const uint8_t badflags = AC_ERROR | DC_ERROR | MV_ERROR;
            for (int i = 0; i < s->mb_num && errors < threshold; ++i) {
                if (s->error_status_table[s->mb_index2xy[i]] & badflags)
                    ++errors;
            }
        }
        return (errors >= threshold);
    }
    return false;
}


/////////////////////////////////////////////////////////////////////////////
//
// If there's any other dynamic decoder updates to do, put them here.  For
// now it just checks to see if the frame needs resizing.
//

void MPEG4DecoderContext::SetDynamicDecodingParams(bool restartOnResize) {
    if (_frameWidth != _avcontext->width || _frameHeight != _avcontext->height)
    {
        ResizeDecodingFrame(restartOnResize);
    }
}

/////////////////////////////////////////////////////////////////////////////
//
// Resizes the decoding frame, updates the buffer
//

void MPEG4DecoderContext::ResizeDecodingFrame(bool restartCodec) {
    _avcontext->width = _frameWidth;
    _avcontext->height = _frameHeight;
    unsigned _rawFrameLen = (_frameWidth * _frameHeight * 3) / 2;
    if (_encFrameBuffer)
    {
        delete[] _encFrameBuffer;
    }
    _encFrameLen = _rawFrameLen/2;         // assume at least 50% compression...
    _encFrameBuffer = new BYTE[_encFrameLen];
    // Restart to force avcodec to use the new frame sizes
    if (restartCodec) {
        CloseCodec();
        OpenCodec();
    }
}

/////////////////////////////////////////////////////////////////////////////
//
// Set our parameters and open the decoder
//

bool MPEG4DecoderContext::OpenCodec()
{
    if ((_avcodec = FFMPEGLibraryInstance.mpeg4_decoder) == NULL) {
        return FALSE;
    }
        
    _avcontext = FFMPEGLibraryInstance.AvcodecAllocContext();
    if (_avcontext == NULL) {
        return FALSE;
    }

    _avpicture = FFMPEGLibraryInstance.AvcodecAllocFrame();
    if (_avpicture == NULL) {
        return FALSE;
    }

    _avcontext->codec = NULL;

    SetStaticDecodingParams();
    SetDynamicDecodingParams(false);    // don't force a restart, it's not open
    if (FFMPEGLibraryInstance.AvcodecOpen(_avcontext, _avcodec) < 0) {
        return FALSE;
    }
    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
//
// Closes the decoder
//

void MPEG4DecoderContext::CloseCodec()
{
  if (_avcontext != NULL) {
    if(_avcontext->codec != NULL)
      FFMPEGLibraryInstance.AvcodecClose(_avcontext);
    FFMPEGLibraryInstance.AvcodecFree(_avcontext);
    _avcontext = NULL;
  }
  if(_avpicture != NULL) {
    FFMPEGLibraryInstance.AvcodecFree(_avpicture);    
    _avpicture = NULL;
  }
}

/////////////////////////////////////////////////////////////////////////////
//
// Frame decoding loop.  Copies incoming RTP payloads into a buffer, when a
// marker frame is received, send the completed frame into the decoder then
// display.
//

bool MPEG4DecoderContext::DecodeFrames(const BYTE * src, unsigned & srcLen,
                                       BYTE * dst, unsigned & dstLen,
                                       unsigned int & flags)
{
    if (!FFMPEGLibraryInstance.IsLoaded())
        return 0;

    // Creates our frames
    RTPFrame srcRTP(src, srcLen);
    RTPFrame dstRTP(dst, dstLen, RTP_DYNAMIC_PAYLOAD);
    dstLen = 0;
    
    int srcPayloadSize = srcRTP.GetPayloadSize();
    SetDynamicDecodingParams(true); // Adjust dynamic settings, restart allowed
    
    // Don't exceed buffer limits.  _encFrameLen set by ResizeDecodingFrame
    if(_lastPktOffset + srcPayloadSize < _encFrameLen)
    {
        // Copy the payload data into the buffer and update the offset
        memcpy(_encFrameBuffer + _lastPktOffset, srcRTP.GetPayloadPtr(),
               srcPayloadSize);
        _lastPktOffset += srcPayloadSize;
    }
    else {

        // Likely we dropped the marker packet, so at this point we have a
        // full buffer with some of the frame we wanted and some of the next
        // frame.  

        //I'm on the fence about whether to send the data to the
        // decoder and hope for the best, or to throw it all away and start 
        // again.


        // throw the data away and ask for an IFrame
        flags |= PluginCodec_ReturnCoderRequestIFrame;
        _lastPktOffset = 0;
        return 0;
    }

    // decode the frame if we got the marker packet
    int got_picture = 0;
    if (srcRTP.GetMarker()) {
        _frameNum++;
        int len = FFMPEGLibraryInstance.AvcodecDecodeVideo
                        (_avcontext, _avpicture, &got_picture,
                         _encFrameBuffer, _lastPktOffset);

        if (len >= 0 && got_picture) {
            if (DecoderError(_keyRefreshThresh)) {
                // ask for an IFrame update, but still show what we've got
                flags |= PluginCodec_ReturnCoderRequestIFrame;
            }
            // If the decoding size changes on us, we can catch it and resize
            if (!_disableResize
                && (_frameWidth != (unsigned)_avcontext->width
                   || _frameHeight != (unsigned)_avcontext->height))
            {
                // Set the decoding width to what avcodec says it is
                _frameWidth  = _avcontext->width;
                _frameHeight = _avcontext->height;
                // Set dynamic settings (framesize), restart as needed
                SetDynamicDecodingParams(true);
                return true;
            }

            // it's stride time
            int frameBytes = (_frameWidth * _frameHeight * 3) / 2;
            PluginCodec_Video_FrameHeader * header
                = (PluginCodec_Video_FrameHeader *)dstRTP.GetPayloadPtr();
            header->x = header->y = 0;
            header->width = _frameWidth;
            header->height = _frameHeight;
            int size = _frameWidth * _frameHeight;
            unsigned char *dst = OPAL_VIDEO_FRAME_DATA_PTR(header);
            for (int i=0; i<3; i ++) {
                unsigned char *src = _avpicture->data[i];
                int dst_stride = i ? _frameWidth >> 1 : _frameWidth;
                int src_stride = _avpicture->linesize[i];
                int h = i ? _frameHeight >> 1 : _frameHeight;
                if (src_stride==dst_stride) {
                    memcpy(dst, src, dst_stride*h);
                    dst += dst_stride*h;
                } 
                else 
                {
                    while (h--) {
                        memcpy(dst, src, dst_stride);
                        dst += dst_stride;
                        src += src_stride;
                    }
                }
            }
            // Treating the screen as an RTP is weird
            dstRTP.SetPayloadSize(sizeof(PluginCodec_Video_FrameHeader)
                                  + frameBytes);
            dstRTP.SetPayloadType(RTP_DYNAMIC_PAYLOAD);
            dstRTP.SetTimestamp(srcRTP.GetTimestamp());
            dstRTP.SetMarker(TRUE);
            dstLen = dstRTP.GetPacketLen();
            flags |= PluginCodec_ReturnCoderLastFrame;
        }
        else {
            // decoding error, ask for an IFrame update
            flags |= PluginCodec_ReturnCoderRequestIFrame;
        }
        _lastPktOffset = 0;
    }
    return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
//
// OPAL plugin decoder functions
//

static void * create_decoder(const struct PluginCodec_Definition *)
{
  return new MPEG4DecoderContext;
}

// This is assuming the second param is the context...
static int decoder_set_options(
      const struct PluginCodec_Definition *, 
      void * _context , 
      const char *, 
      void * parm, 
      unsigned * parmLen)
{
  if (parmLen == NULL || *parmLen != sizeof(const char **))
    return 0;

  MPEG4DecoderContext * context = (MPEG4DecoderContext *)_context;

  // This checks if any of the following options have been passed:
  // 1) get the "Frame Width" media format parameter and let the decoder know
  // 2) get the "Frame Height" media format parameter and let the decoder know
  // 3) Check if "Error Recovery" boolean option has been passed, then sets the
  //    value in the decoder context
  // 4) Check if "Error Threshold" integer has been passed, then sets the value
  // 5) Check if "Disable Resize" boolean has been passed, then sets the value

  if (parm != NULL) {
    const char ** options = (const char **)parm;
    int i;
    for (i = 0; options[i] != NULL; i += 2) {
      if (STRCMPI(options[i], "Frame Width") == 0)
        context->SetFrameWidth(atoi(options[i+1]));
      else if(STRCMPI(options[i], "Frame Height") == 0)
        context->SetFrameHeight(atoi(options[i+1]));
      else if(STRCMPI(options[i], "Error Recovery") == 0)
        context->SetErrorRecovery(atoi(options[i+1]));
      else if(STRCMPI(options[i], "Error Threshold") == 0)
        context->SetErrorThresh(atoi(options[i+1]));
      else if(STRCMPI(options[i], "Disable Resize") == 0)
        context->SetDisableResize(atoi(options[i+1]));
    }
  }
  return 1;
}

static void destroy_decoder(const struct PluginCodec_Definition * /*codec*/, void * _context)
{
  MPEG4DecoderContext * context = (MPEG4DecoderContext *)_context;
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
  MPEG4DecoderContext * context = (MPEG4DecoderContext *)_context;
  return context->DecodeFrames((const BYTE *)from, *fromLen, (BYTE *)to, *toLen, *flag);
}

static int decoder_get_output_data_size(const PluginCodec_Definition * codec, void * _context, const char *, void *, unsigned *)
{
  MPEG4DecoderContext * context = (MPEG4DecoderContext *) _context;
  return sizeof(PluginCodec_Video_FrameHeader) + (context->GetFrameBytes() * 3 / 2);
}

// Plugin Codec Definitions

static PluginCodec_ControlDefn cifDecoderControls[] = {
  { "valid_for_protocol",       valid_for_h323 },
  { "get_codec_options",    coder_get_cif_options },
  { "set_codec_options",    decoder_set_options },
  { "get_output_data_size", decoder_get_output_data_size },
  { NULL }
};

static PluginCodec_ControlDefn qcifDecoderControls[] = {
  { "valid_for_protocol",       valid_for_h323 },
  { "get_codec_options",    coder_get_qcif_options },
  { "set_codec_options",    decoder_set_options },
  { "get_output_data_size", decoder_get_output_data_size },
  { NULL }
};

static PluginCodec_ControlDefn sipDecoderControls[] = {
  { "valid_for_protocol",       valid_for_sip },
  { "get_codec_options",    coder_get_sip_options },
  { "set_codec_options",    decoder_set_options },
  { "get_output_data_size", decoder_get_output_data_size },
  { NULL }
};

/////////////////////////////////////////////////////////////////////////////


static struct PluginCodec_information licenseInfo = {
1168031935,                                                       // timestamp = Fri 5 Jan 2006 21:19:40 PM UTC
  
  "Craig Southeren, Guilhem Tardy, Derek Smithies, Michael Smith, Josh Mahonin", // authors
  "1.0",                                                        // source code version
  "openh323@openh323.org",                                      // source code email
  "http://sourceforge.net/projects/openh323",                   // source code URL
  "Copyright (C) 2007 Canadian Bank Note, Limited.",
  "Copyright (C) 2006 by Post Increment"                       // source code copyright
  ", Copyright (C) 2005 Salyens"
  ", Copyright (C) 2001 March Networks Corporation"
  ", Copyright (C) 1999-2000 Equivalence Pty. Ltd."
  "MPL 1.0",                                                    // source code license
  PluginCodec_License_MPL,                                      // source code license
  
  "FFMPEG",                                                     // codec description
  "Michael Niedermayer, Fabrice Bellard",                       // codec author
  "4.7.1",                                                      // codec version
  "ffmpeg-devel-request@ mplayerhq.hu",                         // codec email
  "http://sourceforge.net/projects/ffmpeg/",                    // codec URL
  "Copyright (c) 2000-2001 Fabrice Bellard"                     // codec copyright information
  ", Copyright (c) 2002-2003 Michael Niedermayer",
  "GNU LESSER GENERAL PUBLIC LICENSE, Version 2.1, February 1999", // codec license
  PluginCodec_License_LGPL                                         // codec license code
};

/////////////////////////////////////////////////////////////////////////////

static const char YUV420PDesc[]  = { "YUV420P" };

static const char mpeg4QCIFDesc[]  = { "MPEG4-QCIF" };
static const char mpeg4CIFDesc[]   = { "MPEG4-CIF" };
static const char mpeg4Desc[]      = { "MPEG4" };

static const char sdpMPEG4[]   = { "MP4V-ES" };

/////////////////////////////////////////////////////////////////////////////

static struct PluginCodec_Definition mpeg4CodecDefn[6] = {

{ 
  // H.323 CIF encoder
  PLUGIN_CODEC_VERSION_VIDEO,         // codec API version
  &licenseInfo,                       // license information

  PluginCodec_MediaTypeVideo |        // video codec
  PluginCodec_RTPTypeShared |
  PluginCodec_RTPTypeDynamic,         // specified RTP type

  mpeg4CIFDesc,                       // text decription
  YUV420PDesc,                        // source format
  mpeg4CIFDesc,                       // destination format

  0,                                  // user data 

  MPEG4_CLOCKRATE,                    // samples per second
  MPEG4_BITRATE,                      // raw bits per second
  20000,                              // nanoseconds per frame

  CIF_WIDTH,                          // frame width
  CIF_HEIGHT,                         // frame height
  10,                                 // recommended frame rate
  60,                                 // maximum frame rate
  0,                                  // IANA RTP payload code
  sdpMPEG4,                           // RTP payload name

  create_encoder,                     // create codec function
  destroy_encoder,                    // destroy codec
  codec_encoder,                      // encode/decode
  cifEncoderControls,                 // codec controls

  PluginCodec_H323VideoCodec_h263,    // h323CapabilityType 
  NULL                                // h323CapabilityData
},
{ 
  // H.323 CIF decoder
  PLUGIN_CODEC_VERSION_VIDEO,         // codec API version
  &licenseInfo,                       // license information

  PluginCodec_MediaTypeVideo |        // video codec
  PluginCodec_RTPTypeShared |
  PluginCodec_RTPTypeDynamic,         // specified RTP type

  mpeg4CIFDesc,                       // text decription
  mpeg4CIFDesc,                       // source format
  YUV420PDesc,                        // destination format

  0,                                  // user data 

  MPEG4_CLOCKRATE,                    // samples per second
  MPEG4_BITRATE,                      // raw bits per second
  20000,                              // nanoseconds per frame

  CIF_WIDTH,                          // frame width
  CIF_HEIGHT,                         // frame height
  10,                                 // recommended frame rate
  60,                                 // maximum frame rate
  0,                                  // IANA RTP payload code
  sdpMPEG4,                           // RTP payload name

  create_decoder,                     // create codec function
  destroy_decoder,                    // destroy codec
  codec_decoder,                      // encode/decode
  cifDecoderControls,                 // codec controls

  PluginCodec_H323VideoCodec_h263,    // h323CapabilityType 
  NULL                                // h323CapabilityData
},

{ 
  // H.323 QCIF encoder
  PLUGIN_CODEC_VERSION_VIDEO,         // codec API version
  &licenseInfo,                       // license information

  PluginCodec_MediaTypeVideo |        // video codec
  PluginCodec_RTPTypeShared |
  PluginCodec_RTPTypeDynamic,         // specified RTP type

  mpeg4QCIFDesc,                      // text decription
  YUV420PDesc,                        // source format
  mpeg4QCIFDesc,                      // destination format

  0,                                  // user data 

  MPEG4_CLOCKRATE,                    // samples per second
  MPEG4_BITRATE,                      // raw bits per second
  20000,                              // nanoseconds per frame

  QCIF_WIDTH,                         // frame width
  QCIF_HEIGHT,                        // frame height
  10,                                 // recommended frame rate
  60,                                 // maximum frame rate
  0,                                  // IANA RTP payload code
  sdpMPEG4,                           // RTP payload name

  create_encoder,                     // create codec function
  destroy_encoder,                    // destroy codec
  codec_encoder,                      // encode/decode
  qcifEncoderControls,                // codec controls

  PluginCodec_H323VideoCodec_h263,    // h323CapabilityType 
  NULL                                // h323CapabilityData
},
{ 
  // H.323 QCIF decoder
  PLUGIN_CODEC_VERSION_VIDEO,         // codec API version
  &licenseInfo,                       // license information

  PluginCodec_MediaTypeVideo |        // video codec
  PluginCodec_RTPTypeShared |
  PluginCodec_RTPTypeDynamic,         // specified RTP type

  mpeg4QCIFDesc,                      // text decription
  mpeg4QCIFDesc,                      // source format
  YUV420PDesc,                        // destination format

  0,                                  // user data 

  MPEG4_CLOCKRATE,                    // samples per second
  MPEG4_BITRATE,                      // raw bits per second
  20000,                              // nanoseconds per frame

  QCIF_WIDTH,                         // frame width
  QCIF_HEIGHT,                        // frame height
  10,                                 // recommended frame rate
  60,                                 // maximum frame rate
  0,                                  // IANA RTP payload code
  sdpMPEG4,                           // RTP payload name

  create_decoder,                     // create codec function
  destroy_decoder,                    // destroy codec
  codec_decoder,                      // encode/decode
  qcifDecoderControls,                // codec controls

  PluginCodec_H323VideoCodec_h263,    // h323CapabilityType 
  NULL                                // h323CapabilityData
},

{ 
  // SIP encoder
  PLUGIN_CODEC_VERSION_VIDEO,         // codec API version
  &licenseInfo,                       // license information

  PluginCodec_MediaTypeVideo |        // video codec
  PluginCodec_RTPTypeShared |
  PluginCodec_RTPTypeDynamic,         // specified RTP type

  mpeg4Desc,                          // text decription
  YUV420PDesc,                        // source format
  mpeg4Desc,                          // destination format

  0,                                  // user data 

  MPEG4_CLOCKRATE,                    // samples per second
  MPEG4_BITRATE,                      // raw bits per second
  20000,                              // nanoseconds per frame

  CIF_WIDTH,                          // frame width
  CIF_HEIGHT,                         // frame height
  10,                                 // recommended frame rate
  60,                                 // maximum frame rate
  0,                                  // IANA RTP payload code
  sdpMPEG4,                           // RTP payload name

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

  PluginCodec_MediaTypeVideo |        // video codec
  PluginCodec_RTPTypeShared |
  PluginCodec_RTPTypeDynamic,         // specified RTP type

  mpeg4Desc,                          // text decription
  mpeg4Desc,                          // source format
  YUV420PDesc,                        // destination format

  0,                                  // user data 

  MPEG4_CLOCKRATE,                    // samples per second
  MPEG4_BITRATE,                      // raw bits per second
  20000,                              // nanoseconds per frame

  CIF_WIDTH,                          // frame width
  CIF_HEIGHT,                         // frame height
  10,                                 // recommended frame rate
  60,                                 // maximum frame rate
  0,                                  // IANA RTP payload code
  sdpMPEG4,                           // RTP payload name

  create_decoder,                     // create codec function
  destroy_decoder,                    // destroy codec
  codec_decoder,                      // encode/decode
  sipDecoderControls,                 // codec controls

  PluginCodec_H323Codec_NoH323,       // h323CapabilityType 
  NULL                                // h323CapabilityData
},

};

#define NUM_DEFNS   (sizeof(mpeg4CodecDefn) / sizeof(struct PluginCodec_Definition))

/////////////////////////////////////////////////////////////////////////////

extern "C" {
PLUGIN_CODEC_DLL_API struct PluginCodec_Definition *
PLUGIN_CODEC_GET_CODEC_FN(unsigned * count, unsigned version)
{
  // load the library.
  WITH_ALIGNED_STACK({
    WaitAndSignal m(FFMPEGLibraryInstance.processLock);
    if (!FFMPEGLibraryInstance.IsLoaded()) {
      if (!FFMPEGLibraryInstance.Load()) {
        *count = 0;
        return NULL;
      }
      if (FFMPEGLibraryInstance.Fff_check_alignment() != 0) {
        fprintf(stderr, "MPEG4 plugin: ff_check_alignment() reports failure; "
                "stack alignment is not correct\n");
      }
    }
  })

  // check version numbers etc
  if (version < PLUGIN_CODEC_VERSION_VIDEO) {
    *count = 0;
    return NULL;
  }
  else {
    *count = NUM_DEFNS;
    return mpeg4CodecDefn;
  }
}
};
