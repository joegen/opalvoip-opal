/* SBC Plugin codec for OpenH323/OPAL
 *
 * Copyright (C) 2008-10 Christian Hoene, All Rights Reserved
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
 * The Initial Developer of the Original Code is Christian Hoene
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#define _CRT_NONSTDC_NO_DEPRECATE 1
#define _CRT_SECURE_NO_WARNINGS 1

#include <ptlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <err.h>
#include <string.h>
#include <assert.h>

#include "plugin_config.h"

// #include <samplerate.h>
#include <codec/opalplugin.h>
#include <codec/g711a1_plc.h>

#include "sbc.h"

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE (!FALSE)
#endif

#if defined(_WIN32) || defined(_WIN32_WCE)
  #define STRCMPI  _strcmpi
#else
  #define STRCMPI  strcasecmp
#endif

#define SBC_HEAD_FRAGMASK	0xe0	//mask over all fragmentation bits
#define SBC_HEAD_FRAG		0x80	//packet is part of fragmented frame
#define SBC_HEAD_STARTFRAG	0x40	//first packet of fragmented frame	
#define SBC_HEAD_LASTFRAG	0x20	//last packet of fragmented frame
#define SBC_HEAD_RFA		0x10	//reserved
#define SBC_HEAD_COUNT		0x0f	//count of frames / fragments left

struct PluginSbcContext {
    const PluginCodec_Definition *cdef;
    bool is_encoder;
    sbc_t sbcState;
    /* remote end capabilities as returned by a=fmtp */
    uint16_t codec_caps;
    uint8_t bitpool_min;
    uint8_t bitpool_max;
#define SBC_CAP_RATE_16000	0x8000
#define SBC_CAP_RATE_32000	0x4000
#define SBC_CAP_RATE_44100	0x2000
#define SBC_CAP_RATE_48000	0x1000
#define SBC_CAP_MONO		0x0800
#define SBC_CAP_DUAL_CHANNEL	0x0400
#define SBC_CAP_STEREO		0x0200
#define SBC_CAP_JOINT_STEREO	0x0100
#define SBC_CAP_BLK_4		0x0080
#define SBC_CAP_BLK_8		0x0040
#define SBC_CAP_BLK_12		0x0020
#define SBC_CAP_BLK_16		0x0010
#define SBC_CAP_SB_4		0x0008
#define SBC_CAP_SB_8		0x0004
#define SBC_CAP_SNR		0x0002
#define SBC_CAP_LOUDNESS	0x0001

    OpalG711_PLC *plc;

#if 0
    SRC_STATE *srcState;
    int lastPayloadSize;

    float *data1,*data2;  // do buffers to store samples
    int data1_len, data2_len; // size of the buffers measured in float variables

    short *between;                // buffer between resampling and SBC
    int between_len,between_count; // size of allocated buffer and number of sample stored in buffer

    double ratio;
#endif
};

#define channels(cdef) \
	((((cdef)->flags & PluginCodec_ChannelsMask) >> PluginCodec_ChannelsPos) + 1)

/////////////////////////////////////////////////////////////////////////////

/**********************************************************
 *                                                        *
 *   setup up resp. free structures for encoder/decoder   *
 *                                                        *
 **********************************************************/
static void *sbc_create_encoder(const struct PluginCodec_Definition *codec)
{
    PTRACE(0, codec << "sbc_create_encoder");

    struct PluginSbcContext *context = new PluginSbcContext;

    context->is_encoder = true;
    context->cdef = codec;
    /* init SBC */
    int error;
    if((error=sbc_init(&context->sbcState, 0))!=0) {
	PTRACE(0, "Codec\tSBC sbc_init failed " << error);
	free(context);

	return NULL;
    }

    return context;
}


/**
 * Destroy and delete encoder object
 */

static void sbc_destroy_encoder(const struct PluginCodec_Definition *codec, void *_context)
{
    PTRACE(0, codec << "sbc_destroy_encoder");

    PluginSbcContext *context = (PluginSbcContext *)_context;

    sbc_finish(&context->sbcState);

    free(context);
}

/**
 * allocate and create SBC decoder object
 */
static void *sbc_create_decoder(const struct PluginCodec_Definition *codec)
{
    PTRACE(0, codec << "sbc_create_decoder");

    PluginSbcContext *context = new PluginSbcContext;

    context->is_encoder = false;
    context->cdef = codec;

    int error;
    if((error=sbc_init(&context->sbcState, 0))!=0) {
	PTRACE(0, "sbc_create_decoder failed " << error);
	free(context);

	return NULL;
    }

    context->plc = new OpalG711_PLC(context->cdef->sampleRate, channels(codec));
    if(context->plc == NULL) {
      PTRACE(0, "Codec\tSBC creatng PLC left failed " << error);
      sbc_finish(&context->sbcState);
      free(context);

      return NULL;
    }

    return context;
}

static void sbc_destroy_decoder(const struct PluginCodec_Definition *codec, void *_context)
{
    PTRACE(0, codec << "sbc_destroy_decoder");

    struct PluginSbcContext *context = (struct PluginSbcContext *)_context;

    if(!context)
	return;

    free(context->plc);

    sbc_finish(&context->sbcState);

    free(context);
}

/***************************************************
 *                                                 *
 *        encoder and decoder wrappers             *
 *                                                 *
 ***************************************************/
static int sbc_codec_encoder(const struct PluginCodec_Definition *codec,
                                           void *_context,
                                     const void *from,
                                       unsigned *fromLen,
                                           void *to,
                                       unsigned *toLen,
                                   unsigned int *flags)
{
    PTRACE(6, codec << "sbc_codec_encoder " << *fromLen << " " << *toLen << " flags: " << *flags);

    PluginSbcContext *context = (PluginSbcContext *)_context;
    if(!context)
	return 0;

    // remember position of first byte which is needed for payload header
    // and progress in output buffer by one byte
    uint8_t *head = (uint8_t *)to; // pointer to SBC payload header
    *head = 0;
    to = (uint8_t *)to + 1;	// SBC payload header takes one byte
    (*toLen)--;
    size_t written = 1;		// SBC payload haeder + encoded bytes produced by encoder
    size_t used = 0;		// raw bytes consumed by encoder

    do {
      size_t in, out;

      // encoder
      in = sbc_encode(&context->sbcState,
	    (const char*)from,
	    *fromLen,
	    (char*)to,
	    *toLen,
	    &out);
      if (in < 0) {
	PTRACE(4, codec << "sbc_codec_encoder failed " << used);
	return 0;
      }

      // fiddle with pointers to progress through the buffers
      used += in;
      from = (uint8_t *)from + in;
      *fromLen -= in;

      written += out;
      to = (uint8_t *)to + out;
      *toLen -= out;

      (*head)++; // increment frame count

      if (*fromLen == 0) break;
      // should not happen
      if (in == 0) {
	PTRACE(0, "nothing to encode");
	break;
      }
    } while (*head < SBC_HEAD_COUNT);

    *fromLen = used;
    *toLen = written;

    PTRACE(6, codec << "sbc_codec_encoder " << *fromLen << " " << *toLen);

    return 1;
}
    
static int sbc_codec_decoder(const struct PluginCodec_Definition *codec,
                                           void *_context,
                                     const void *from,
                                       unsigned *fromLen,
                                           void *to,
                                       unsigned *toLen,
                                   unsigned int *flags)
{
    struct PluginSbcContext *context = (struct PluginSbcContext *)_context;

    if(!context)
	return 0;

    if(fromLen)
	PTRACE(6, codec << "sbc_codec_decoder " << *fromLen << " " << *toLen << " flags: " << *flags);
    else
	PTRACE(6, codec << "sbc_codec_decoder " << "NULL" << " " << *toLen << " flags: " << *flags);


    /*
     * Package loss concealment
     */
    if (*flags & PluginCodec_CoderSilenceFrame) {
      unsigned length = *toLen / 2 / channels(codec); // plc takes size in samples, NOT bytes
      PTRACE(4, " concealing");
      context->plc->dofe((short*)to, length);
    }
    /*
     * SBC decoder
     */
    else {
      //we do not yet handle fragmented frames
      if (*(uint8_t *)from & SBC_HEAD_FRAGMASK) return 0;

      // i is number of frames in packet
      int i = *(uint8_t *)from & SBC_HEAD_COUNT;
      from = (uint8_t *)from + 1; //consume one byte for payload header
      (*fromLen)--;
      size_t used = 1;		//payload header + encoded bytes consumed by decoder
      size_t written = 0;	//raw bytes produced by decoder

      /* see sbc_codec_encoder for format of SBC payload header */
      while (i-- > 0) {
	size_t in, out;

	// decoder
	in = sbc_decode(&context->sbcState,
	      (const char*)from,
	      *fromLen,
	      (char*)to,
	      *toLen,
	      &out);
	if (in < 0) {
	  PTRACE(4, codec << "sbc_codec_encoder failed " << used);
	  return 0;
	}

	// fiddle with pointers to progress through the buffers
	used += in;
	from = (uint8_t *)from + in;
	*fromLen -= in;

	written += out;
	to = (uint8_t *)to + out;
	*toLen -= out;

	// this should actually not happen
	if (in == 0) {
	  PTRACE(0, " missed a frame");
	  break;
	}
      }

      // return by reference
      *toLen = written;
      *fromLen = used;

      // tell PLC about decoded samples and possibly do OLA
      unsigned length = written / 2 / channels(codec); // plc takes size in samples, NOT bytes
      context->plc->addtohistory((short*)to, length);
    }

    if(fromLen)
	PTRACE(6, codec << "sbc_codec_decoder fin " << *fromLen << " " << *toLen);
    else
	PTRACE(6, codec << "sbc_codec_decoder fin " << "NULL" << " " << *toLen);

    return 1;
}

/***************************************************
 *                                                 *
 *    fmtp: capabilities=0c,XX,XX,XX,XX option     *
 *                                                 *
 ***************************************************/

/* takes string of exactly 14 characters and returns array of exactly 5 octets. */
static uint8_t *parse_fmtp(const char *fmtp) {
  uint8_t *octets = (uint8_t *)malloc(5 * sizeof(uint8_t)); /* XXX why does compiler want me to cast ?!? */

  PAssert(fmtp != NULL, "unsupported number of channels");

  /* parse and check for correct syntax */
  if (fmtp[14] != '\0') goto bad_format;
  for (int i=0; i < 5; i++) {
    char *endptr;

    errno = 0;
    octets[i] = strtol(fmtp, &endptr, 16);
    if (errno) {
      PTRACE(0, "strtol failed on \"" << fmtp << '"');
      goto bad_format;
    }

    if (*endptr != ','&& *endptr != '\0') goto bad_format;
    if (endptr - fmtp != 2) goto bad_format;
    fmtp += 3;
  }

  if (octets[0] != 0x9C) goto bad_format;

  return octets;

bad_format:
  PTRACE(0, "unknown a=fmtp format for SBC");
  free(octets);
  return NULL;
}

/* takes array of exactly 5 octets and returns string of exactly 14 characters. */
char *create_fmtp(const uint8_t *octets) {
  char *buf = (char *)malloc(15);
  
  snprintf(buf, 15, "%02.2hhX,%02.2hhX,%02.2hhX,%02.2hhX,%02.2hhX",
      octets[0],
      octets[1],
      octets[2],
      octets[3],
      octets[4]
  );

  return buf;
}

static int mergeCapabilities(char **merged, const char *_fst, const char *_snd)
{
  uint8_t res[5];
  uint8_t *fst = parse_fmtp(_fst);
  uint8_t *snd = parse_fmtp(_snd);
  if (fst == NULL || snd == NULL) return 0;

  PTRACE(0, "mergeCapabilities");

  res[0] = 0x9C;
  res[1] = fst[1] & snd[1];
  res[2] = fst[2] & snd[2];
  res[3] = fst[3] > snd[3] ? fst[3] : snd[3];
  res[4] = fst[4] < snd[4] ? fst[4] : snd[4];

  *merged = create_fmtp(res);

  if (res[3] < 2 || res[3] > res[4] || res[4] > 250) {
    PTRACE(0, " Bad bit-pool values: " << _fst << " ~ " << _snd << " -> " << *merged);
    return 0;
  }

  PTRACE(0, " " << _snd << " ~ " << _fst << " -> " << *merged);

  return 1;
}

static void freeString(char * string) { free(string); }

static struct PluginCodec_Option const capabilitiesOpt =
{
  PluginCodec_StringOption, // PluginCodec_OptionTypes
  "FMTP",                   // Generic (human readable) option name
  0,                        // Read Only flag
  PluginCodec_CustomMerge,  // Merge mode
  "9C,8F,FF,02,FA",         // Initial value
  "capabilities",           // SIP/SDP FMTP name
  NULL,                     // SIP/SDP FMTP default value (option not included in FMTP if have this value)
  0,                        // H.245 Generic Capability number and scope bits
  NULL,                     // Minimum value
  NULL,                     // Maximum value
  mergeCapabilities,        // Used if m_merge==PluginCodec_CustomMerge
  freeString,
};

static struct PluginCodec_Option const monoOpt =
{
  PluginCodec_IntegerOption,   // PluginCodec_OptionTypes
  PLUGINCODEC_OPTION_CHANNELS, // Generic (human readable) option name
  0,                           // Read Only flag
  PluginCodec_MinMerge,        // Merge mode
  "1",                         // Initial value
  NULL,                        // SIP/SDP FMTP name
  NULL,                        // SIP/SDP FMTP default value (option not included in FMTP if have this value)
  0,                           // H.245 Generic Capability number and scope bits
  "1",                         // Minimum value
  "2",                         // Maximum value
  NULL,                        // Used if m_merge==PluginCodec_CustomMerge
  NULL,
};

static struct PluginCodec_Option const stereoOpt =
{
  PluginCodec_IntegerOption,   // PluginCodec_OptionTypes
  PLUGINCODEC_OPTION_CHANNELS, // Generic (human readable) option name
  0,                           // Read Only flag
  PluginCodec_MinMerge,        // Merge mode
  "2",                         // Initial value
  NULL,                        // SIP/SDP FMTP name
  NULL,                        // SIP/SDP FMTP default value (option not included in FMTP if have this value)
  0,                           // H.245 Generic Capability number and scope bits
  "1",                         // Minimum value
  "2",                         // Maximum value
  NULL,                        // Used if m_merge==PluginCodec_CustomMerge
  NULL,
};

static struct PluginCodec_Option const * const OptionTableMono[] = {
  &capabilitiesOpt,
  &monoOpt,
  NULL
};

static struct PluginCodec_Option const * const OptionTableStereo[] = {
  &capabilitiesOpt,
  &stereoOpt,
  NULL
};

static int get_codec_options(const struct PluginCodec_Definition * codec,
                                                            void * _context,
                                                      const char * name,
                                                            void * parm,
                                                        unsigned * parmLen)
{
  struct PluginSbcContext *context = (struct PluginSbcContext *)_context;

  if (parm == NULL || parmLen == NULL || *parmLen != sizeof(struct PluginCodec_Option **))
    return 0;

  PTRACE(0, "get_codec_options");

  switch(channels(codec)) {
    case 1:
      *(struct PluginCodec_Option const * const * *)parm = OptionTableMono;
      break;
    case 2:
      *(struct PluginCodec_Option const * const * *)parm = OptionTableStereo;
      break;
    default:
      PAssert(false, "Wrong number of channels");
  }

  /* *parmLen = 0; XXX why this??? (taken from iLBC) */

  return 1;
}

static int set_codec_options(const struct PluginCodec_Definition * codec,
                                                            void * _context,
                                                      const char * name,
                                                            void * parm,
                                                        unsigned * parmLen)
{
  struct PluginSbcContext *context = (struct PluginSbcContext *)_context;

  const char * const * option;
  uint8_t *octets;
  unsigned rate = context->cdef->sampleRate;
  unsigned channels = channels(codec);

  PTRACE(0, "set_codec_options for " << (context->is_encoder ? "encoder" : "decoder"));

  if (context == NULL || parm == NULL || parmLen == NULL || *parmLen != sizeof(const char **))
    return 0;

  for (option = (const char * const *)parm; *option != NULL; option += 2) {
    PTRACE(0, " comparing '" << option[0] << "'");
    if (STRCMPI(option[0], PLUGINCODEC_OPTION_CLOCK_RATE) == 0) {
      rate = atoi(option[1]);
      PTRACE(0, "  got rate: " << rate);
      PAssert(rate == context->cdef->sampleRate, "Channels number mismatch");
    }

    if (STRCMPI(option[0], PLUGINCODEC_OPTION_CHANNELS) == 0) {
      channels = atoi(option[1]);
      PTRACE(0, "  got channels: " << channels);
      PAssert(channels == channels(codec), "Channels number mismatch");
    }

    if (STRCMPI(option[0], "FMTP") == 0) {
      PTRACE(0, "  got fmtp: '" << option[1] << "'");
      octets = parse_fmtp(option[1]);
    }
  }

  if (octets == NULL) return 0;

  context->codec_caps = octets[1] << 8;
  context->codec_caps |= octets[2];

  context->bitpool_min = octets[3];
  context->bitpool_max = octets[4];

  switch(channels) {
    case 1:
      if (context->codec_caps & SBC_CAP_MONO) {
	context->sbcState.mode = SBC_MODE_MONO;
	PTRACE(0, " SBC_MODE_MONO");
  }
      else {
	PTRACE(0, " mono maybe not supported.");
      }
      break;
    case 2:
      if (context->codec_caps & SBC_CAP_JOINT_STEREO) {
	context->sbcState.mode = SBC_MODE_JOINT_STEREO;
	PTRACE(0, " SBC_MODE_JOINT_STEREO");
      }
      else if(context->codec_caps & SBC_CAP_STEREO) {
	context->sbcState.mode = SBC_MODE_STEREO;
	PTRACE(0, " SBC_MODE_STEREO");
      }
      else if(context->codec_caps & SBC_CAP_DUAL_CHANNEL) {
	context->sbcState.mode = SBC_MODE_DUAL_CHANNEL;
	PTRACE(0, " SBC_MODE_DUAL_CHANNEL");
      }
      else {
	PTRACE(0, " stereo not supported.");
	return 0;
      }
      break;
    default:
      PTRACE(0, " unsupported number of channels");
      return 0;
  }

  switch(rate) {
    case 48000:
      context->sbcState.frequency = SBC_FREQ_48000;
      PTRACE(0, " SBC_FREQ_48000");
      break;
    case 44100:
      context->sbcState.frequency = SBC_FREQ_44100;
      PTRACE(0, " SBC_FREQ_44100");
      break;
    case 32000:
      context->sbcState.frequency = SBC_FREQ_32000;
      PTRACE(0, " SBC_FREQ_32000");
      break;
    case 16000:
      context->sbcState.frequency = SBC_FREQ_16000;
      PTRACE(0, " SBC_FREQ_16000");
      break;
    default:
      PAssert(0, " Unsupported sample rate");
  }

  if (context->codec_caps & SBC_CAP_SB_8) {
    context->sbcState.subbands = SBC_SB_8;
    PTRACE(0, " SBC_SB_8");
  }
  else if (context->codec_caps & SBC_CAP_SB_4) {
    context->sbcState.subbands = SBC_SB_4;
    PTRACE(0, " SBC_SB_4");
  }
  else {
    PTRACE(0, " no supported number of subbands supplied.");
    return 0;
  }

  if (context->codec_caps & SBC_CAP_BLK_16) {
    context->sbcState.blocks = SBC_BLK_16;
    PTRACE(0, " SBC_BLK_16");
  }
  else if (context->codec_caps & SBC_CAP_BLK_12) {
    context->sbcState.blocks = SBC_BLK_12;
    PTRACE(0, " SBC_BLK_12");
  }
  else if (context->codec_caps & SBC_CAP_BLK_8) {
    context->sbcState.blocks = SBC_BLK_8;
    PTRACE(0, " SBC_BLK_8");
  }
  else if (context->codec_caps & SBC_CAP_BLK_4) {
    context->sbcState.blocks = SBC_BLK_4;
    PTRACE(0, " SBC_BLK_4");
  }
  else {
    PTRACE(0, " no supported block length supplied.");
    return 0;
  }

  if (context->codec_caps & SBC_CAP_SNR) {
    context->sbcState.allocation = SBC_AM_SNR;
    PTRACE(0, " SBC_AM_SNR");
  }
  else if (context->codec_caps & SBC_CAP_LOUDNESS) {
    context->sbcState.allocation = SBC_AM_LOUDNESS;
    PTRACE(0, " SBC_AM_LOUDNESS");
  }
  else {
    PTRACE(0, " no supported allocation mode supplied.");
    return 0;
  }

  context->sbcState.bitpool =
    (context->bitpool_max + context->bitpool_min) / 2; /* thats just arbitrary */

  return 1;
}

/* taken from Speex */
static int valid_for_sip(
      const struct PluginCodec_Definition *codec,
      void *context,
      const char *key,
      void *parm,
      unsigned *parmLen)
{
    PTRACE(0, codec << "valid_for_sip" << key << ":" << parm);

    if (parmLen == NULL || parm == NULL || *parmLen != sizeof(char *))
	return 0;

    return (STRCMPI((const char *)parm, "sip") == 0) ? 1 : 0;
}

/////////////////////////////////////////////////////////////////////////////

static struct PluginCodec_ControlDefn sbc_codec_controls[] = {
  { PLUGINCODEC_CONTROL_VALID_FOR_PROTOCOL,    valid_for_sip },
  //{ PLUGINCODEC_CONTROL_TO_NORMALISED_OPTIONS, to_normalised_options },
  //{ PLUGINCODEC_CONTROL_FREE_CODEC_OPTIONS,    free_codec_options },
  { PLUGINCODEC_CONTROL_SET_CODEC_OPTIONS,     set_codec_options },
  { PLUGINCODEC_CONTROL_GET_CODEC_OPTIONS,     get_codec_options },
  { NULL }
};

static struct PluginCodec_information licenseInfo = {
 1205422679,                              // timestamp = Thu Mar 13 16:38 CET 2008
  "Christian Hoene, University of Tübingen",                   // source code author
  "0.2",                                                       // source code version
  "hoene@ieee.org",                                            // source code email
  "http://www.uni-tuebingen.de",                               // source code URL
  "Copyright (C) 2008 by Christian Hoene, All Rights Reserved",// source code copyright
   "MPL 1.0",                                                  // source code license
  PluginCodec_License_MPL,                                     // source code license

  "Subband Coding (SBC)",                                      // codec description
  "Bluetooth SIG, Frans De Bont",                              // codec author
  "1.2",                                                       // codec version
  "frans.de.bont@philips.com",                                 // codec email
  "http://www.bluetooth.com/",                                 // codec URL
  "unknown",                                                   // codec copyright information
  "please contact Wim van der Kruk, Philips.com",              // codec license
  PluginCodec_License_ResearchAndDevelopmentUseOnly            // codec license code
};

#define DECLARE_SBC_CODEC(name, rate, channels, framelength) \
  { \
    /* encoder */ \
    PLUGIN_CODEC_VERSION_OPTIONS,       /* codec API version */ \
    &licenseInfo,                       /* license information */ \
\
    PluginCodec_MediaTypeAudio |        /* audio codec */ \
    PluginCodec_InputTypeRaw |          /* raw input data */ \
    PluginCodec_OutputTypeRaw |         /* raw output data */ \
    PluginCodec_RTPTypeShared |         \
    PluginCodec_RTPTypeDynamic |        /* dynamic RTP type */ \
    (((channels-1) << PluginCodec_ChannelsPos) & PluginCodec_ChannelsMask), \
\
    name,                               /* text decription */ \
    "L16",                             /* source format */ \
    name,                               /* destination format */ \
\
    (void *)NULL,                       /* user data */ \
\
    rate,                               /* samples per second */ \
    rate * channels * 2 * 8,            /* raw bits per second - TODO: is dynamic */ \
    1000 * 1000 * framelength / rate,   /* mikroseconds per frame - TODO: is dynamic */ \
    {{ \
    framelength,                        /* samples per frame - TODO: is dynamic */ \
    framelength * channels * 2,         /* bytes per frame - TODO: is dynamic */ \
    1,					/* recommended number of frames per packet */ \
    1300 / framelength / channels / 2,  /* maximum number of frames per packet TODO: just a guess that there are 1300 bytes per packet available */ \
    }}, \
    0,                                  /* IANA RTP payload code */ \
    name,                               /* RTP payload name */ \
\
    sbc_create_encoder,                 /* create codec function */ \
    sbc_destroy_encoder,                /* destroy codec */ \
    sbc_codec_encoder,                  /* encode/decode */ \
    sbc_codec_controls,			/* codec controls */ \
\
    PluginCodec_H323Codec_NoH323,       /* h323CapabilityType */ \
    NULL                                \
  }, \
  { \
    /* decoder */ \
    PLUGIN_CODEC_VERSION_OPTIONS,       /* codec API version */ \
    NULL,                               /* license information */ \
\
    PluginCodec_MediaTypeAudio |        /* audio codec */ \
    PluginCodec_InputTypeRaw |          /* raw input data */ \
    PluginCodec_OutputTypeRaw |         /* raw output data */ \
    PluginCodec_RTPTypeShared |         \
    PluginCodec_RTPTypeDynamic |        /* dynamic RTP type */ \
    PluginCodec_DecodeSilence | \
    PluginCodec_EmptyPayload | \
    (((channels-1) << PluginCodec_ChannelsPos) & PluginCodec_ChannelsMask), \
\
    name,                               /* text decription */ \
    name,                               /* source format */ \
    "L16",                             /* destination format */ \
\
    (const void *)NULL,                 /* user data */ \
\
    rate,                               /* samples per second */ \
    rate * channels * 2 * 8,            /* raw bits per second - TODO: is dynamic */ \
    1000 * 1000 * framelength / rate,   /* mikroseconds per frame - TODO: is dynamic */ \
    {{ \
    framelength,                        /* samples per frame - TODO: is dynamic */ \
    framelength * channels * 2,         /* bytes per frame - TODO: is dynamic */ \
    1,					/* recommended number of frames per packet */ \
    1300 / framelength / channels / 2,  /* maximum number of frames per packet TODO: just a guess that there are 1300 bytes per packet available */ \
    }}, \
    0,                                  /* IANA RTP payload code */ \
    name,                               /* RTP payload name */ \
\
    sbc_create_decoder,			/* create codec function */ \
    sbc_destroy_decoder,		/* destroy codec */ \
    sbc_codec_decoder,			/* encode/decode */ \
    sbc_codec_controls,                               /* codec controls */ \
\
    PluginCodec_H323Codec_NoH323,       /* h323CapabilityType */ \
    NULL                                /* h323CapabilityData */ \
  }

static struct PluginCodec_Definition SBCCodecDefn[] =
{
    DECLARE_SBC_CODEC("SBC48-2", 48000, 2, 384),
    DECLARE_SBC_CODEC("SBC32-2", 32000, 2, 128),
    DECLARE_SBC_CODEC("SBC16-2", 16000, 2, 128),
    DECLARE_SBC_CODEC("SBC48-1", 48000, 1, 384),
    DECLARE_SBC_CODEC("SBC32-1", 32000, 1, 128),
    DECLARE_SBC_CODEC("SBC16-1", 16000, 1, 128),
};

extern "C" {
PLUGIN_CODEC_IMPLEMENT_ALL(SBC, SBCCodecDefn, PLUGIN_CODEC_VERSION_OPTIONS)
}
/////////////////////////////////////////////////////////////////////////////
