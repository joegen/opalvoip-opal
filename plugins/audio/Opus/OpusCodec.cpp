/*
 * Opus Plugin codec for OPAL
 *
 * This the starting point for creating new plug in codecs for C++
 * It is generally done fom the point of view of video codecs, as
 * audio codecs are much simpler and there is a wealth of examples
 * in C available.
 *
 * Copyright (C) 2010 Vox Lucida Pty Ltd, All Rights Reserved
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
 * The Original Code is OPAL Library.
 *
 * The Initial Developer of the Original Code is Vox Lucida Pty Ltd
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#include <codec/opalplugin.hpp>
#include <codec/known.h>

#ifndef PLUGIN_CODEC_DLL_EXPORTS
#include "plugin_config.h"
#endif

#include "opus.h"


#define MY_CODEC Opus                        // Name of codec (use C variable characters)

#define MY_CODEC_LOG STRINGIZE(MY_CODEC)
class MY_CODEC { };

PLUGINCODEC_CONTROL_LOG_FUNCTION_DEF


static const char MyDescription[] = "Opus Audio Codec";     // Human readable description of codec

PLUGINCODEC_LICENSE(
  "Robert Jongbloed, Vox Lucida Pty.Ltd.",                      // source code author
  "1.0",                                                        // source code version
  "robertj@voxlucida.com.au",                                   // source code email
  "http://www.voxlucida.com.au",                                // source code URL
  "Copyright (C) 2010 by Vox Lucida Pt.Ltd., All Rights Reserved", // source code copyright
  "MPL 1.0",                                                    // source code license
  PluginCodec_License_MPL,                                      // source code license
  
  MyDescription,                                                // codec description
  "Xiph",                                                       // codec author
  "1.0.3",                                                      // codec version
  "opus@xiph.org",                                              // codec email
  "http://www.opus-codec.org",                                  // codec URL
  "Copyright 2013, Xiph.Org Foundation",                   // codec copyright information
  "Opus has a freely available specification, a BSD-licensed, "
  "high-quality reference encoder and decoder, and protective, "
  "royalty-free licenses for the required patents. The copyright "
  "and patent licenses for Opus are automatically granted to "
  "everyone and do not require application or approval.",       // codec license
  PluginCodec_License_BSD             // codec license code
);


///////////////////////////////////////////////////////////////////////////////

static struct PluginCodec_Option const UseInBandFEC =
{
  PluginCodec_BoolOption,
  "Use In-Band FEC",
  false,
  PluginCodec_AndMerge,
  "0",
  "useinbandfec"
};

static struct PluginCodec_Option const UseDTX =
{
  PluginCodec_BoolOption,
  "Use DTX",
  false,
  PluginCodec_AndMerge,
  "0",
  "usedtx"
};

static struct PluginCodec_Option const MaxPlaybackRate =
{
  PluginCodec_IntegerOption,
  "Max Playback Rate",
  true,
  PluginCodec_NoMerge,
  "48000",
  "maxplaybackrate",
  "",
  0,
  "6000",
  "510000"
};

static struct PluginCodec_Option const MaxCaptureRate =
{
  PluginCodec_IntegerOption,
  "Max Capture Rate",
  true,
  PluginCodec_NoMerge,
  "48000",
  "sprop-maxcapturerate",
  "",
  0,
  "6000",
  "510000"
};

static struct PluginCodec_Option const PlaybackStereo =
{
  PluginCodec_BoolOption,
  "Playback Stereo",
  true,
  PluginCodec_NoMerge,
  "0",
  "stereo"
};

static struct PluginCodec_Option const CaptureStereo =
{
  PluginCodec_BoolOption,
  "Capture Stereo",
  true,
  PluginCodec_NoMerge,
  "0",
  "sprop-stereo"
};

static struct PluginCodec_Option const * const MyOptions[] = {
  &UseInBandFEC,
  &UseDTX,
  &MaxPlaybackRate,
  &MaxCaptureRate,
  &PlaybackStereo,
  &CaptureStereo,
  NULL
};


class OpusPluginMediaFormat : public PluginCodec_AudioFormat<MY_CODEC>
{
  public:
    unsigned m_actualSampleRate;
    unsigned m_actualChannels;

    OpusPluginMediaFormat(const char * formatName, const char * rawFormat, unsigned actualSampleRate, unsigned actualChannels)
      : PluginCodec_AudioFormat<MY_CODEC>(formatName, "OPUS", MyDescription,
                                           actualSampleRate/50,
                                           640*actualChannels*actualSampleRate/48000,
                                           48000, MyOptions)
      , m_actualSampleRate(actualSampleRate)
      , m_actualChannels(actualChannels)
    {
      m_rawFormat = rawFormat;
      m_recommendedFramesPerPacket = 1; // 20ms
      m_maxFramesPerPacket = 6; // 120ms
      m_flags |= PluginCodec_SetChannels(2) | PluginCodec_RTPTypeShared;
    }


    virtual bool ToCustomised(OptionMap &, OptionMap & changed)
    {
      Unsigned2String(m_actualSampleRate, changed[MaxPlaybackRate.m_name]);
      changed[MaxCaptureRate.m_name] = changed[MaxPlaybackRate.m_name];
      changed[PlaybackStereo.m_name] = changed[CaptureStereo.m_name] = m_actualChannels == 1 ? "0" : "1";
      return true;
    }
};

#define CHANNEL  1
#define CHANNELS 2
#define DEF_MEDIA_FORMAT(rate, stereo) \
  static OpusPluginMediaFormat const MyMediaFormat##rate##stereo("Opus-" #rate #stereo, \
                          OPAL_PCM16##stereo##_##rate##KHZ, rate*1000, CHANNEL##stereo)
DEF_MEDIA_FORMAT( 8, );
DEF_MEDIA_FORMAT( 8,S);
DEF_MEDIA_FORMAT(12, );
DEF_MEDIA_FORMAT(12,S);
DEF_MEDIA_FORMAT(16, );
DEF_MEDIA_FORMAT(16,S);
DEF_MEDIA_FORMAT(24, );
DEF_MEDIA_FORMAT(24,S);
DEF_MEDIA_FORMAT(48, );
DEF_MEDIA_FORMAT(48,S);


///////////////////////////////////////////////////////////////////////////////

class OpusPluginCodec : public PluginCodec<MY_CODEC>
{
  protected:
    unsigned m_sampleRate;
    unsigned m_channels;

  public:
    OpusPluginCodec(const PluginCodec_Definition * defn)
      : PluginCodec<MY_CODEC>(defn)
    {
      const OpusPluginMediaFormat *mediaFormat = reinterpret_cast<const OpusPluginMediaFormat *>(m_definition->userData);
      m_sampleRate = mediaFormat->m_actualSampleRate;
      m_channels = mediaFormat->m_actualChannels;
    }
};


///////////////////////////////////////////////////////////////////////////////

class OpusPluginEncoder : public OpusPluginCodec
{
  protected:
    OpusEncoder * m_encoder;
    bool          m_useInBandFEC;
    bool          m_useDTX;
    unsigned      m_bitRate;

  public:
    OpusPluginEncoder(const PluginCodec_Definition * defn)
      : OpusPluginCodec(defn)
      , m_encoder(NULL)
      , m_useInBandFEC(false)
      , m_useDTX(false)
      , m_bitRate(12000)
    {
      PTRACE(4, MY_CODEC_LOG, "Encoder created: $Revision$, version \"" << opus_get_version_string() << '"');
    }


    ~OpusPluginEncoder()
    {
      if (m_encoder != NULL)
        opus_encoder_destroy(m_encoder);
    }


    virtual bool Construct()
    {
      int error;
      if ((m_encoder = opus_encoder_create(m_sampleRate, m_channels, OPUS_APPLICATION_VOIP, &error)) != NULL)
        return true;

      PTRACE(1, MY_CODEC_LOG, "Encoder create error " << error << ' ' << opus_strerror(error));
      return false;
    }


    virtual bool SetOption(const char * optionName, const char * optionValue)
    {
      if (strcasecmp(optionName, UseInBandFEC.m_name) == 0)
        return SetOptionBoolean(m_useInBandFEC, optionValue);

      if (strcasecmp(optionName, UseDTX.m_name) == 0)
        return SetOptionBoolean(m_useDTX, optionValue);

      if (strcasecmp(optionName, PLUGINCODEC_OPTION_TARGET_BIT_RATE) == 0)
        return SetOptionUnsigned(m_bitRate, optionValue, 6000, 510000);

      // Base class sets bit rate and frame time
      return PluginCodec<MY_CODEC>::SetOption(optionName, optionValue);
    }


    virtual bool OnChangedOptions()
    {
      if (m_encoder == NULL)
        return false;

      opus_encoder_ctl(m_encoder, OPUS_SET_INBAND_FEC(m_useInBandFEC));
      opus_encoder_ctl(m_encoder, OPUS_SET_DTX(m_useDTX));
      opus_encoder_ctl(m_encoder, OPUS_SET_BITRATE(m_bitRate));
      return true;
    }


    virtual bool Transcode(const void * fromPtr,
                             unsigned & fromLen,
                                 void * toPtr,
                             unsigned & toLen,
                             unsigned & flags)
    {
      opus_int32 result = opus_encode(m_encoder,
                                      (const opus_int16 *)fromPtr, fromLen/m_channels/2,
                                      (unsigned char *)toPtr, toLen);
      if (result < 0) {
        PTRACE(1, MY_CODEC_LOG, "Encoder error " << result << ' ' << opus_strerror(result));
        return false;
      }

      toLen = result;
      fromLen = opus_packet_get_nb_samples((const unsigned char *)toPtr, toLen, m_sampleRate)*m_channels*2;
      return true;
    }
};


///////////////////////////////////////////////////////////////////////////////

class OpusPluginDecoder : public OpusPluginCodec
{
  protected:
    OpusDecoder * m_decoder;

  public:
    OpusPluginDecoder(const PluginCodec_Definition * defn)
      : OpusPluginCodec(defn)
      , m_decoder(NULL)
    {
      PTRACE(4, MY_CODEC_LOG, "Decoder created: $Revision$, version \"" << opus_get_version_string() << '"');
    }


    ~OpusPluginDecoder()
    {
      if (m_decoder != NULL)
        opus_decoder_destroy(m_decoder);
    }


    virtual bool Construct()
    {
      int error;
      if ((m_decoder = opus_decoder_create(m_sampleRate, m_channels, &error)) != NULL)
        return true;

      PTRACE(1, MY_CODEC_LOG, "Decoder create error " << error << ' ' << opus_strerror(error));
      return false;
    }


    virtual bool Transcode(const void * fromPtr,
                             unsigned & fromLen,
                                 void * toPtr,
                             unsigned & toLen,
                             unsigned & flags)
    {
      int samples = opus_decoder_get_nb_samples(m_decoder, (const unsigned char *)fromPtr, fromLen);
      if (samples < 0) {
        PTRACE(1, MY_CODEC_LOG, "Decoding error " << samples << ' ' << opus_strerror(samples));
        return false;
      }

      if (samples*m_channels*2 > (int)toLen) {
        PTRACE(1, MY_CODEC_LOG, "Provided sample buffer too small, " << toLen << " bytes");
        return false;
      }

      int result = opus_decode(m_decoder,
		               (const unsigned char *)fromPtr, fromLen,
                               (opus_int16 *)toPtr, samples, 0);
      if (result < 0) {
        PTRACE(1, MY_CODEC_LOG, "Decoder error " << result << ' ' << opus_strerror(result));
        return false;
      }

      toLen = result*m_channels*2;
      return true;
    }
};


///////////////////////////////////////////////////////////////////////////////

static struct PluginCodec_Definition MyCodecDefinition[] =
{
  #define MEDIA_FORMAT_DEF(variant) PLUGINCODEC_AUDIO_CODEC_CXX(MyMediaFormat##variant, OpusPluginEncoder, OpusPluginDecoder),
  MEDIA_FORMAT_DEF(8)
  MEDIA_FORMAT_DEF(8S)
  MEDIA_FORMAT_DEF(12)
  MEDIA_FORMAT_DEF(12S)
  MEDIA_FORMAT_DEF(16)
  MEDIA_FORMAT_DEF(16S)
  MEDIA_FORMAT_DEF(24)
  MEDIA_FORMAT_DEF(24S)
  MEDIA_FORMAT_DEF(48)
  MEDIA_FORMAT_DEF(48S)
};

PLUGIN_CODEC_IMPLEMENT_CXX(MY_CODEC, MyCodecDefinition);


/////////////////////////////////////////////////////////////////////////////
