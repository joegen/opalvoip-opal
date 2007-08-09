/*
 * H.264 Plugin codec for OpenH323/OPAL
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
 * Contributor(s): Guilhem Tardy (gtardy@salyens.com)
 *                 Craig Southeren (craigs@postincrement.com)
 *                 Matthias Schneider (ma30002000@yahoo.de)
 */

#ifndef __H264DYNA_H__
#define __H264DYNA_H__ 1
#include <codec/opalplugin.h>

#include <stdlib.h>
#ifdef _WIN32
#include <windows.h>
#include <malloc.h>
#define STRCMPI  _strcmpi
#else
#include <semaphore.h>
#include <dlfcn.h>
#define STRCMPI  strcasecmp
typedef unsigned char BYTE;
//typedef bool BOOL;
#define FALSE false
#define TRUE  true

#endif

#include <string.h>
#include "critsect.h"

extern "C" {
#include "ffmpeg/avcodec.h"
};

#include "shared/trace.h"

#  ifdef  _WIN32
#    define P_DEFAULT_PLUGIN_DIR "C:\\PWLIB_PLUGINS"
#    define DIR_SEPERATOR "\\"
#    define DIR_TOKENISER ";"
#  else
#    define P_DEFAULT_PLUGIN_DIR "/usr/lib/pwlib"
#    define DIR_SEPERATOR "/"
#    define DIR_TOKENISER ":"
#  endif

#include <vector>

// if defined, the FFMPEG code is access via another DLL
// otherwise, the FFMPEG code is assumed to be statically linked into this plugin

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
      char * env = ::getenv("PWLIBPLUGINDIR");
      if (env != NULL) {
        const char * token = strtok(env, DIR_TOKENISER);
        while (token != NULL) {
          if (InternalOpen(token, name))
            return true;
          token = strtok(NULL, DIR_TOKENISER);
        }
      }
      return InternalOpen(".", name);
    }

  // split into directories on correct seperator

    bool InternalOpen(const char * dir, const char *name)
    {
      char path[1024];
      memset(path, 0, sizeof(path));
      strcpy(path, dir);
      if (path[strlen(path)-1] != DIR_SEPERATOR[0]) 
        strcat(path, DIR_SEPERATOR);
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
        char * err = dlerror();
        if (err != NULL)
          TRACE(1, "H264\tDYNA\tError loading " << path << " - " << err)
         else
          TRACE(1, "H264\tDYNA\tError loading " << path);
      }
#endif // _WIN32
      if (_hDLL != NULL) TRACE(1, "H264\tDYNA\tSuccessfully loaded " << path);
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

    AVCodec *AvcodecFindDecoder(enum CodecID id);
    AVCodecContext *AvcodecAllocContext(void);
    AVFrame *AvcodecAllocFrame(void);
    int AvcodecOpen(AVCodecContext *ctx, AVCodec *codec);
    int AvcodecClose(AVCodecContext *ctx);
    int AvcodecDecodeVideo(AVCodecContext *ctx, AVFrame *pict, int *got_picture_ptr, BYTE *buf, int buf_size);
    void AvcodecFree(void * ptr);

    void AvLogSetLevel(int level);
    void AvLogSetCallback(void (*callback)(void*, int, const char*, va_list));

    bool IsLoaded();
    CriticalSection processLock;

  protected:
    void (*Favcodec_init)(void);
    AVCodec *Favcodec_h264_decoder;
    void (*Favcodec_register)(AVCodec *format);
    AVCodec *(*Favcodec_find_decoder)(enum CodecID id);
    AVCodecContext *(*Favcodec_alloc_context)(void);
    void (*Favcodec_free)(void *);
    AVFrame *(*Favcodec_alloc_frame)(void);
    int (*Favcodec_open)(AVCodecContext *ctx, AVCodec *codec);
    int (*Favcodec_close)(AVCodecContext *ctx);
    int (*Favcodec_decode_video)(AVCodecContext *ctx, AVFrame *pict, int *got_picture_ptr, BYTE *buf, int buf_size);

    void (*FAv_log_set_level)(int level);
    void (*FAv_log_set_callback)(void (*callback)(void*, int, const char*, va_list));

    bool isLoadedOK;
};

//////////////////////////////////////////////////////////////////////////////

#endif /* __H264DYNA_H__ */
