/*
 * iSAC Plugin codec for OpenH323/OPAL
 *
 * Copyright (C) 2014 Vox Lucida Pty. Ltd., All Rights Reserved
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
 * The Initial Developer of the Original Code is Vox Lucida Pty. Ltd.
 *
 * Contributor(s): ______________________________________.
 *
 */

#define _CRT_NONSTDC_NO_DEPRECATE 1
#define _CRT_SECURE_NO_WARNINGS 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef PLUGIN_CODEC_DLL_EXPORTS
#include "plugin_config.h"
#endif

#include <codec/opalplugin.h>

#include <isac.h>


#if defined(_WIN32) || defined(_WIN32_WCE)
  #define STRCMPI  _strcmpi
#else
  #define STRCMPI  strcasecmp
#endif


/////////////////////////////////////////////////////////////////////////////

static void * CreateEncoder(const struct PluginCodec_Definition * codec)
{
  ISACStruct * context;
  if (WebRtcIsac_Create(&context) != 0)
    return 0;

  if (WebRtcIsac_EncoderInit(context, 1) == 0 &&
      WebRtcIsac_SetEncSampRate(context, codec->sampleRate) == 0 &&
      WebRtcIsac_Control(context, codec->bitsPerSec, 30) == 0)
    return context;

  WebRtcIsac_Free(context);
  return 0;
}


static void * CreateDecoder(const struct PluginCodec_Definition * codec)
{
  ISACStruct * context;
  if (WebRtcIsac_Create(&context) != 0)
    return 0;

  if (WebRtcIsac_DecoderInit(context) == 0 && WebRtcIsac_SetDecSampRate(context, codec->sampleRate) == 0)
    return context;

  WebRtcIsac_Free(context);
  return 0;
}


static void DestroyContext(const struct PluginCodec_Definition * codec, void * context)
{
  WebRtcIsac_Free((ISACStruct *)context);
}


static int EncodeAudio(const struct PluginCodec_Definition * codec, 
                                           void * context,
                                     const void * from, 
                                       unsigned * fromLen,
                                           void * to,         
                                       unsigned * toLen,
                                   unsigned int * flag)
{
  int result;
  const int16_t * samples = (const int16_t *)from;
  unsigned samples10ms = codec->sampleRate/100;
  unsigned bytes10ms = samples10ms*2;
  unsigned consumed = 0;

  do {
    if (*fromLen < bytes10ms)
      return 0;

    if ((result = WebRtcIsac_Encode((ISACStruct *)context, samples, (int16_t*)to)) < 0)
      return 0;

    samples += samples10ms;
    *fromLen -= bytes10ms;
    consumed += bytes10ms;
  } while (result == 0);

  *toLen = result;
  *fromLen = consumed;

  return 1; 
}


static int DecodeAudio(const struct PluginCodec_Definition * codec, 
                                           void * context,
                                     const void * from, 
                                       unsigned * fromLen,
                                           void * to,         
                                       unsigned * toLen,
                                   unsigned int * flag)
{
  int result;
  int16_t samples;
  int16_t speechType;

  if (WebRtcIsac_ReadFrameLen((ISACStruct *)context, (const int16_t*)from, &samples) < 0)
    return 0;

  if (*toLen < (unsigned)samples)
    return 0;

  if ((result = WebRtcIsac_Decode((ISACStruct *)context, (const uint16_t*)from, *fromLen, (int16_t*)to, &speechType)) < 0)
    return 0;

  // set output lengths
  *toLen = result*2;

  return 1;
}


/////////////////////////////////////////////////////////////////////////////

PLUGINCODEC_LICENSE(
  "Robert Jongbloed, Vox Lucida Pty. Ltd.",                    // source code author
  "1.0",                                                       // source code version
  "robertj@voxlucida.com.au",                                  // source code email
  "http://www.voxlucida.com.au",                               // source code URL
  "Copyright (C) 2014 by Vox Lucida Pty. Ltd., All Rights Reserved", // source code copyright
  "MPL 1.0",                                                   // source code license
  PluginCodec_License_MPL,                                     // source code license

  "iSAC Wideband Audio Codec",                                 // codec description
  "Global IP Sound, Inc.",                                     // codec author
  NULL,                                                        // codec version
  "info@globalipsound.com",                                    // codec email
  "http://www.isacfreeware.org",                               // codec URL
  "Global IP Sound AB. Portions Copyright (C) 1999-2002, All Rights Reserved",          // codec copyright information
  "Global IP Sound iLBC Freeware Public License, IETF Version, Limited Commercial Use", // codec license
  PluginCodec_License_Freeware                                // codec license code
);


static struct PluginCodec_Definition iSACCodecDefn[] =
{
  PLUGINCODEC_AUDIO_CODEC("iSAC-16kHz",                     /* Media Format */
                          "isac",                           /* IANA RTP payload code */
                          "iSAC Wideband Audio Codec",      /* Description text */
                          16000,                            /* Sample rate */
                          32000,                            /* Maximum bits per second */
                          480,                              /* Samples per audio frame */
                          1,                                /* Recommended frames per packet */
                          1,                                /* Maximum frames per packet */
                          PluginCodec_RTPTypeDynamic,       /* Extra flags typically if RTP payload type is fixed */
                          96,                               /* IANA RTP payload type code */
                          PluginCodec_H323Codec_NoH323,     /* h323CapabilityType enumeration */
                          NULL,                             /* Data to go with h323CapabilityType */
                          CreateEncoder,
                          DestroyContext,
                          EncodeAudio,
                          CreateDecoder,
                          DestroyContext,
                          DecodeAudio,
                          NULL
  ),
  PLUGINCODEC_AUDIO_CODEC("iSAC-32kHz",                     /* Media Format */
                          "isac",                           /* IANA RTP payload code */
                          "iSAC Wideband Audio Codec",      /* Description text */
                          32000,                            /* Sample rate */
                          56000,                            /* Maximum bits per second */
                          960,                              /* Samples per audio frame */
                          1,                                /* Recommended frames per packet */
                          1,                                /* Maximum frames per packet */
                          PluginCodec_RTPTypeDynamic,       /* Extra flags typically if RTP payload type is fixed */
                          96,                               /* IANA RTP payload type code */
                          PluginCodec_H323Codec_NoH323,     /* h323CapabilityType enumeration */
                          NULL,                             /* Data to go with h323CapabilityType */
                          CreateEncoder,
                          DestroyContext,
                          EncodeAudio,
                          CreateDecoder,
                          DestroyContext,
                          DecodeAudio,
                          NULL
  )
};


PLUGIN_CODEC_IMPLEMENT_ALL(iSAC, iSACCodecDefn, PLUGIN_CODEC_VERSION_OPTIONS)

/////////////////////////////////////////////////////////////////////////////
