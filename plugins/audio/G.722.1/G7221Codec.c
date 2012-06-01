/****************************************************************************
 *
 * ITU G.722.1 wideband audio codec plugin for OpenH323/OPAL
 *
 * Copyright (C) 2009 Nimajin Software Consulting, All Rights Reserved
 * Copyright (C) 2010 Vox Lucida Pty. Ltd.
 *
 * Permission to copy, use, sell and distribute this file is granted
 * provided this copyright notice appears in all copies.
 * Permission to modify the code herein and to distribute modified code is
 * granted provided this copyright notice appears in all copies, and a
 * notice that the code was modified is included with the copyright notice.
 *
 * This software and information is provided "as is" without express or im-
 * plied warranty, and with no claim as to its suitability for any purpose.
 *
 ****************************************************************************
 *
 * ITU-T 7/14kHz Audio Coder (G.722.1) AKA Polycom 'Siren'
 * SDP usage described in RFC 5577
 * H.245 capabilities defined in G.722.1 Annex A & H.245 Appendix VIII
 * This implementation employs ITU-T G.722.1 (2005-05) fixed point reference 
 * code, release 2.1 (2008-06)
 * This implements only standard bit rates 24000 & 32000 at 16kHz (Siren7)
 * sampling rate, not extended modes or 32000 sampling rate (Siren14).
 * G.722.1 does not implement any silence handling (VAD/CNG)
 * Static variables are not used, so multiple instances can run simultaneously.
 * G.722.1 is patented by Polycom, but is royalty-free if you follow their 
 * license terms. See:
 * http://www.polycom.com/company/about_us/technology/siren14_g7221c/faq.html
 *
 * Initial development by: Ted Szoczei, Nimajin Software Consulting, 09-12-09
 * Portions developed by: Robert Jongbloed, Vox Lucida Pty. Ltd.
 *
 ****************************************************************************/

#define _CRT_NONSTDC_NO_DEPRECATE 1
#define _CRT_SECURE_NO_WARNINGS 1

#ifndef PLUGIN_CODEC_DLL_EXPORTS
#include "plugin-config.h"
#endif

#include <codec/opalplugin.h>

#include "../../../src/codec/g7221mf_inc.cxx"


/////////////////////////////////////////////////////////////////////////////

static const char G7221Description[] = "ITU-T G.722.1 - 7/14kHz Audio Coder";

PLUGINCODEC_LICENSE(
  "Ted Szoczei, Nimajin Software Consulting",                  // source code author
  "1.0",                                                       // source code version
  "ted.szoczei@nimajin.com",                                   // source code email
  NULL,                                                        // source code URL
  "Copyright (c) 2009 Nimajin Software Consulting",            // source code copyright
  "None",                                                      // source code license
  PluginCodec_License_None,                                    // source code license
    
  G7221Description,                                            // codec description
  "Polycom, Inc.",                                             // codec author
  "2.1  2008-06-26",				                           // codec version
  NULL,                                                        // codec email
  "http://www.itu.int/rec/T-REC-G.722.1/e",                    // codec URL
  "(c) 2005 Polycom, Inc. All rights reserved.",               // codec copyright information
  "ITU-T General Public License (G.191)",                      // codec license
  PluginCodec_License_NoRoyalties                              // codec license code
);


/////////////////////////////////////////////////////////////////////////////

#include "G722-1/defs.h"


// bits and bytes per 20 ms frame, depending on bitrate
// 60 bytes for 24000, 80 for 32000
#define G722_1_FRAME_BITS(rate) ((Word16)((rate) / 50))
#define G722_1_FRAME_BYTES(rate) ((rate) / 400)


/////////////////////////////////////////////////////////////////////////////
// Compress audio for transport


typedef struct
{
  unsigned bitsPerSec;                  // can be changed between frames
  Word16 history [G7221_SAMPLES_PER_FRAME];
  Word16 mlt_coefs [G7221_SAMPLES_PER_FRAME];
  Word16 mag_shift;
} G7221EncoderContext;


static void * G7221EncoderCreate (const struct PluginCodec_Definition * codec)
{
  int i;
  G7221EncoderContext * Context = (G7221EncoderContext *) malloc (sizeof(G7221EncoderContext));
  if (Context == NULL)
    return NULL;

  Context->bitsPerSec = codec->bitsPerSec;

  // initialize the mlt history buffer
  for (i = 0; i < G7221_SAMPLES_PER_FRAME; i++)
    Context->history[i] = 0;

  return Context;
}


static void G7221EncoderDestroy (const struct PluginCodec_Definition * codec, void * context)
{
  free (context);
}


static int G7221Encode (const struct PluginCodec_Definition * codec, 
                                                       void * context,
                                                 const void * fromPtr, 
                                                   unsigned * fromLen,
                                                       void * toPtr,         
                                                   unsigned * toLen,
                                               unsigned int * flag)
{
  G7221EncoderContext * Context = (G7221EncoderContext *) context;
  if (Context == NULL)
    return 0;

  if (*fromLen < G7221_SAMPLES_PER_FRAME * sizeof(Word16))
    return 0;                           // Source is not a full frame

  if (*toLen < G722_1_FRAME_BYTES (Context->bitsPerSec))
    return 0;                           // Destination buffer not big enough

  // Convert input samples to rmlt coefs
  Context->mag_shift = samples_to_rmlt_coefs ((Word16 *) fromPtr, Context->history, Context->mlt_coefs, G7221_SAMPLES_PER_FRAME);

  // Encode the mlt coefs
  encoder (G722_1_FRAME_BITS (Context->bitsPerSec), NUMBER_OF_REGIONS, Context->mlt_coefs, Context->mag_shift, (Word16 *) toPtr);

  // return the number of encoded bytes to the caller
  *fromLen = G7221_SAMPLES_PER_FRAME * sizeof(Word16);
  *toLen = G722_1_FRAME_BYTES (Context->bitsPerSec);
  return 1;
}


/////////////////////////////////////////////////////////////////////////////
// Convert encoded source to audio


typedef struct
{
  unsigned bitsPerSec;                  // can be changed between frames
  Bit_Obj bitobj;
  Rand_Obj randobj;
  Word16 decoder_mlt_coefs [G7221_SAMPLES_PER_FRAME];
  Word16 mag_shift;
  Word16 old_samples [G7221_SAMPLES_PER_FRAME / 2];
  Word16 old_decoder_mlt_coefs [G7221_SAMPLES_PER_FRAME];
  Word16 old_mag_shift;
  Word16 frame_error_flag;
} G7221DecoderContext;


static void * G7221DecoderCreate (const struct PluginCodec_Definition * codec)
{
  int i;
  G7221DecoderContext * Context = (G7221DecoderContext *) malloc (sizeof(G7221DecoderContext));
  if (Context == NULL)
    return NULL;

  Context->bitsPerSec = codec->bitsPerSec;

  Context->old_mag_shift = 0;
  Context->frame_error_flag = 0;

  // initialize the coefs history
  for (i = 0; i < G7221_SAMPLES_PER_FRAME; i++)
    Context->old_decoder_mlt_coefs[i] = 0;    

  for (i = 0; i < (G7221_SAMPLES_PER_FRAME >> 1); i++)
    Context->old_samples[i] = 0;
    
  // initialize the random number generator
  Context->randobj.seed0 =
  Context->randobj.seed1 =
  Context->randobj.seed2 =
  Context->randobj.seed3 = 1;

  return Context;
}


static void G7221DecoderDestroy (const struct PluginCodec_Definition * codec, void * context)
{
  free (context);
}


static int G7221Decode (const struct PluginCodec_Definition * codec, 
                                                       void * context,
                                                 const void * fromPtr, 
                                                   unsigned * fromLen,
                                                       void * toPtr,         
                                                   unsigned * toLen,
                                               unsigned int * flag)
{
  int i;
  G7221DecoderContext * Context = (G7221DecoderContext *) context;
  if (Context == NULL)
    return 0;

  if (*fromLen < G722_1_FRAME_BYTES (Context->bitsPerSec))
    return 0;                           // Source is not a full frame

  if (*toLen < G7221_SAMPLES_PER_FRAME * sizeof(Word16))
    return 0;                           // Destination buffer not big enough

  // reinit the current word to point to the start of the buffer
  Context->bitobj.code_word_ptr = (Word16 *) fromPtr;
  Context->bitobj.current_word = *((Word16 *) fromPtr);
  Context->bitobj.code_bit_count = 0;
  Context->bitobj.number_of_bits_left = G722_1_FRAME_BITS(Context->bitsPerSec);
        
  // process the out_words into decoder_mlt_coefs
  decoder (&Context->bitobj, &Context->randobj, NUMBER_OF_REGIONS, Context->decoder_mlt_coefs, &Context->mag_shift, &Context->old_mag_shift, Context->old_decoder_mlt_coefs, Context->frame_error_flag);
  
  // convert the decoder_mlt_coefs to samples
  rmlt_coefs_to_samples (Context->decoder_mlt_coefs, Context->old_samples, (Word16 *) toPtr, G7221_SAMPLES_PER_FRAME, Context->mag_shift);

  // For ITU testing, off the 2 lsbs.
  for (i = 0; i < G7221_SAMPLES_PER_FRAME; i++)
    ((Word16 *) toPtr) [i] &= 0xFFFC;
      
  // return the number of decoded bytes to the caller
  *fromLen = G722_1_FRAME_BYTES (Context->bitsPerSec);
  *toLen = G7221_SAMPLES_PER_FRAME * sizeof(Word16);
  return 1;
}


/////////////////////////////////////////////////////////////////////////////

// bitrate is a required SDP parameter in RFC 3047/5577

static struct PluginCodec_Option BitRateOption24k =
{
  PluginCodec_IntegerOption,        // PluginCodec_OptionTypes
  G7221BitRateOptionName,           // Generic (human readable) option name
  1,                                // Read Only flag
  PluginCodec_EqualMerge,           // Merge mode
  STRINGIZE(G7221_24K_BIT_RATE),    // Initial value
  G7221BitRateFMTPName,             // SIP/SDP FMTP name
  "0",                              // SIP/SDP FMTP default value (option not included in FMTP if have this value)
  0,                                // H.245 Generic Capability number and scope bits
  STRINGIZE(G7221_24K_BIT_RATE),    // Minimum value (enum values separated by ':')
  STRINGIZE(G7221_24K_BIT_RATE)     // Maximum value
};

static struct PluginCodec_Option BitRateOption32k =
{
  PluginCodec_IntegerOption,        // PluginCodec_OptionTypes
  G7221BitRateOptionName,           // Generic (human readable) option name
  1,                                // Read Only flag
  PluginCodec_EqualMerge,           // Merge mode
  STRINGIZE(G7221_32K_BIT_RATE),    // Initial value
  G7221BitRateFMTPName,             // SIP/SDP FMTP name
  "0",                              // SIP/SDP FMTP default value (option not included in FMTP if have this value)
  0,                                // H.245 Generic Capability number and scope bits
  STRINGIZE(G7221_32K_BIT_RATE),    // Minimum value (enum values separated by ':')
  STRINGIZE(G7221_32K_BIT_RATE)     // Maximum value
};

static struct PluginCodec_Option RxFramesPerPacket =
{
  PluginCodec_IntegerOption,        // PluginCodec_OptionTypes
  PLUGINCODEC_OPTION_RX_FRAMES_PER_PACKET, // Generic (human readable) option name
  0,                                // Read Only flag
  PluginCodec_MinMerge,             // Merge mode
  "2",                              // Initial value
  NULL,                             // SIP/SDP FMTP name
  NULL,                             // SIP/SDP FMTP default value (option not included in FMTP if have this value)
  G7221_H241_RxFramesPerPacket,     // H.245 Generic Capability number and scope bits
  "1",                              // Minimum value (enum values separated by ':')
  "8"                               // Maximum value
};

static struct PluginCodec_Option const * const OptionTable24k[] =
{
  &BitRateOption24k,
  &RxFramesPerPacket,
  NULL
};

static struct PluginCodec_Option const * const OptionTable32k[] =
{
  &BitRateOption32k,
  &RxFramesPerPacket,
  NULL
};


static int get_codec_options (const struct PluginCodec_Definition * defn,
                                                             void * context, 
                                                       const char * name,
                                                             void * parm,
                                                         unsigned * parmLen)
{
  if (parm == NULL || parmLen == NULL || *parmLen != sizeof(struct PluginCodec_Option **))
    return 0;

  *(struct PluginCodec_Option const * const * *)parm = (defn->bitsPerSec == G7221_24K_BIT_RATE)? OptionTable24k : OptionTable32k;
  *parmLen = 0;
  return 1;
}


// Options are read-only, so set_codec_options not implemented
// get_codec_options returns pointers to statics, and toCustomized and 
// toNormalized are not implemented, so free_codec_options is not necessary

static struct PluginCodec_ControlDefn G7221Controls[] =
{
  { PLUGINCODEC_CONTROL_GET_CODEC_OPTIONS, get_codec_options },
  { NULL }
};


/////////////////////////////////////////////////////////////////////////////


static const struct PluginCodec_H323GenericCodecData G722124kCap =
{
  OpalPluginCodec_Identifer_G7221, // capability identifier (Ref: G.722.1 (05/2005) Table A.2)
  G7221_24K_BIT_RATE               // Must always be this regardless of "Max Bit Rate" option
};

static const struct PluginCodec_H323GenericCodecData G722132kCap =
{
  OpalPluginCodec_Identifer_G7221, // capability identifier (Ref: G.722.1 (05/2005) Table A.2)
  G7221_32K_BIT_RATE               // Must always be this regardless of "Max Bit Rate" option
};


/////////////////////////////////////////////////////////////////////////////


static struct PluginCodec_Definition G7221CodecDefn[] =
{
  PLUGINCODEC_AUDIO_CODEC(
    G7221FormatName24K,
    G7221EncodingName,
    G7221Description,
    G7221_SAMPLE_RATE,
    G7221_24K_BIT_RATE,
    G7221_SAMPLES_PER_FRAME,
    1,1,
    PluginCodec_RTPTypeShared,
    0, // IANA RTP payload code -  dynamic
    PluginCodec_H323Codec_generic, &G722124kCap,
    G7221EncoderCreate, G7221EncoderDestroy, G7221Encode,
    G7221DecoderCreate, G7221DecoderDestroy, G7221Decode,
    G7221Controls
  ),
  PLUGINCODEC_AUDIO_CODEC(
    G7221FormatName32K,
    G7221EncodingName,
    G7221Description,
    G7221_SAMPLE_RATE,
    G7221_32K_BIT_RATE,
    G7221_SAMPLES_PER_FRAME,
    1,1,
    PluginCodec_RTPTypeShared,
    0, // IANA RTP payload code -  dynamic
    PluginCodec_H323Codec_generic, &G722132kCap,
    G7221EncoderCreate, G7221EncoderDestroy, G7221Encode,
    G7221DecoderCreate, G7221DecoderDestroy, G7221Decode,
    G7221Controls
  )
};


PLUGIN_CODEC_IMPLEMENT_ALL(G7221, G7221CodecDefn, PLUGIN_CODEC_VERSION_OPTIONS)


/////////////////////////////////////////////////////////////////////////////
