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
#include "trace.h"
#include "dyna.h"

//////////////////////////////////////////////////////////////////////////////
#ifdef USE_DLL_AVCODEC

FFMPEGLibrary::FFMPEGLibrary()
{
  isLoadedOK = FALSE;
}

bool FFMPEGLibrary::Load()
{
  WaitAndSignal m(processLock);
  if (IsLoaded())
    return true;
  if (!DynaLink::Open("avcodec")
#if defined(WIN32)
      && !DynaLink::Open("libavcodec")
#else
      && !DynaLink::Open("libavcodec.so")
#endif
    ) {
    TRACE (1, "FFLINK\tFailed to load a library, some codecs won't operate correctly;");
#if !defined(WIN32)
    TRACE (1, "put libavcodec.so in the current directory (together with this program) and try again");
#else
    TRACE (1, "put avcodec.dll in the current directory (together with this program) and try again");
#endif
    return false;
  }

  if (!GetFunction("avcodec_init", (Function &)Favcodec_init)) {
    TRACE (1, "Failed to load avcodec_init");
    return false;
  }

  if (!GetFunction("h263_encoder", (Function &)Favcodec_h263_encoder)) {
    TRACE (1, "Failed to load h263_encoder" );
    return false;
  }

  if (!GetFunction("h263p_encoder", (Function &)Favcodec_h263p_encoder)) {
    TRACE (1, "Failed to load h263p_encoder" );
    return false;
  }

  if (!GetFunction("h263_decoder", (Function &)Favcodec_h263_decoder)) {
    TRACE (1, "Failed to load h263_decoder" );
    return false;
  }

  if (!GetFunction("register_avcodec", (Function &)Favcodec_register)) {
    TRACE (1, "Failed to load register_avcodec");
    return false;
  }

  if (!GetFunction("avcodec_find_encoder", (Function &)Favcodec_find_encoder)) {
    TRACE (1, "Failed to load avcodec_find_encoder");
    return false;
  }

  if (!GetFunction("avcodec_find_decoder", (Function &)Favcodec_find_decoder)) {
    TRACE (1, "Failed to load avcodec_find_decoder");
    return false;
  }

  if (!GetFunction("avcodec_alloc_context", (Function &)Favcodec_alloc_context)) {
    TRACE (1, "Failed to load avcodec_alloc_context");
    return false;
  }

  if (!GetFunction("avcodec_alloc_frame", (Function &)Favcodec_alloc_frame)) {
    TRACE (1, "Failed to load avcodec_alloc_frame");
    return false;
  }

  if (!GetFunction("avcodec_open", (Function &)Favcodec_open)) {
    TRACE (1, "Failed to load avcodec_open");
    return false;
  }

  if (!GetFunction("avcodec_close", (Function &)Favcodec_close)) {
    TRACE (1, "Failed to load avcodec_close");
    return false;
  }

  if (!GetFunction("avcodec_encode_video", (Function &)Favcodec_encode_video)) {
    TRACE (1, "Failed to load avcodec_encode_video" );
    return false;
  }

  if (!GetFunction("avcodec_decode_video", (Function &)Favcodec_decode_video)) {
    TRACE (1, "Failed to load avcodec_decode_video");
    return false;
  }

  if (!GetFunction("av_free", (Function &)Favcodec_free)) {
    TRACE (1, "Failed to load avcodec_close");
    return false;
  }

  if (!GetFunction("av_log_set_level", (Function &)FAv_log_set_level)) {
    TRACE (1, "Failed to load av_log_set_level");
    return false;
  }

  if (!GetFunction("av_log_set_callback", (Function &)FAv_log_set_callback)) {
    TRACE (1, "Failed to load av_log_set_callback");
    return false;
  }
 
  // must be called before using avcodec lib
  Favcodec_init();

  // register only the codecs needed (to have smaller code)
  Favcodec_register(Favcodec_h263_encoder);
  Favcodec_register(Favcodec_h263p_encoder);
  Favcodec_register(Favcodec_h263_decoder);
  
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
//  TRACE_IF(6, res, "FFLINK\tFound encoder " << res->name << " @ " << ::hex << (int)res << ::dec);
  return res;
}

AVCodec *FFMPEGLibrary::AvcodecFindDecoder(enum CodecID id)
{
  AVCodec *res = Favcodec_find_decoder(id);
  //TRACE_IF(6, res, "FFLINK\tFound decoder " << res->name << " @ " << ::hex << (int)res << ::dec);
  return res;
}

AVCodecContext *FFMPEGLibrary::AvcodecAllocContext(void)
{
  AVCodecContext *res = Favcodec_alloc_context();
  //TRACE_IF(6, res, "FFLINK\tAllocated context @ " << ::hex << (int)res << ::dec);
  return res;
}

AVFrame *FFMPEGLibrary::AvcodecAllocFrame(void)
{
  AVFrame *res = Favcodec_alloc_frame();
  //TRACE_IF(6, res, "FFLINK\tAllocated frame @ " << ::hex << (int)res << ::dec);
  return res;
}

int FFMPEGLibrary::AvcodecOpen(AVCodecContext *ctx, AVCodec *codec)
{
  WaitAndSignal m(processLock);

  //TRACE(6, "FFLINK\tNow open context @ " << ::hex << (int)ctx << ", codec @ " << (int)codec << ::dec);
  return Favcodec_open(ctx, codec);
}

int FFMPEGLibrary::AvcodecClose(AVCodecContext *ctx)
{
  //TRACE(6, "FFLINK\tNow close context @ " << ::hex << (int)ctx << ::dec);
  return Favcodec_close(ctx);
}

int FFMPEGLibrary::AvcodecEncodeVideo(AVCodecContext *ctx, BYTE *buf, int buf_size, const AVFrame *pict)
{
  WaitAndSignal m(processLock);

//  TRACE(4, "FFLINK\tNow encode video for ctxt @ " << ::hex << (int)ctx << ", pict @ " << (int)pict
//	    << ", buf @ " << (int)buf << ::dec << " (" << buf_size << " bytes)");
  int res = Favcodec_encode_video(ctx, buf, buf_size, pict);

  TRACE(4, "FFLINK\tEncoded video into " << res << " bytes");
  return res;
}

int FFMPEGLibrary::AvcodecDecodeVideo(AVCodecContext *ctx, AVFrame *pict, int *got_picture_ptr, BYTE *buf, int buf_size)
{
  WaitAndSignal m(processLock);

//  TRACE(6, "FFLINK\tNow decode video for ctxt @ " << ::hex << (int)ctx << ", pict @ " << (int)pict
//	 << ", buf @ " << (int)buf << ::dec << " (" << buf_size << " bytes)");
  int res = Favcodec_decode_video(ctx, pict, got_picture_ptr, buf, buf_size);

  TRACE(6, "FFLINK\tDecoded video of " << res << " bytes, got_picture=" << *got_picture_ptr);
  return res;
}

void FFMPEGLibrary::AvcodecFree(void * ptr)
{
  Favcodec_free(ptr);
}

void FFMPEGLibrary::AvLogSetLevel(int level)
{
  FAv_log_set_level(level);
}

void FFMPEGLibrary::AvLogSetCallback(void (*callback)(void*, int, const char*, va_list))
{
  FAv_log_set_callback(callback);
}

bool FFMPEGLibrary::IsLoaded()
{
  return isLoadedOK;
}

#else

#error "Not yet able to use statically linked libavcodec"

#endif // USE_DLL_AVCODEC
