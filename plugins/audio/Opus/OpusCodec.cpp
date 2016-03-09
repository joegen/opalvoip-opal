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

#include <vector>


#define MY_CODEC Opus                        // Name of codec (use C variable characters)

#define MY_CODEC_LOG STRINGIZE(MY_CODEC)
class MY_CODEC { };

PLUGINCODEC_CONTROL_LOG_FUNCTION_DEF


static const char MyDescription[] = "Opus Audio Codec (RFC6716 reference)";     // Human readable description of codec

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
  "1.1",                                                        // codec version
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


#define MAX_BIT_RATE 510000


///////////////////////////////////////////////////////////////////////////////

static struct PluginCodec_Option const UseInBandFEC =
{
  PluginCodec_BoolOption,
  "Use In-Band FEC",
  false,
  PluginCodec_AndMerge,
  "1",
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
  "64000",
  "maxplaybackrate",
  "",
  0,
  "6000",
  STRINGIZE(MAX_BIT_RATE)
};

static struct PluginCodec_Option const MaxCaptureRate =
{
  PluginCodec_IntegerOption,
  "Max Capture Rate",
  true,
  PluginCodec_NoMerge,
  "64000",
  "sprop-maxcapturerate",
  "",
  0,
  "6000",
  STRINGIZE(MAX_BIT_RATE)
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

static struct PluginCodec_Option const DynamicPacketLoss =
{
  PluginCodec_IntegerOption,
  PLUGINCODEC_OPTION_DYNAMIC_PACKET_LOSS,
  false,
  PluginCodec_NoMerge,
  "0",
  NULL,
  NULL,
  0,
  "0",
  "100"
};

static struct PluginCodec_Option const * const MyOptions[] = {
  &UseInBandFEC,
  &UseDTX,
  &MaxPlaybackRate,
  &MaxCaptureRate,
  &PlaybackStereo,
  &CaptureStereo,
  &DynamicPacketLoss,
  NULL
};


class OpusPluginMediaFormat : public PluginCodec_AudioFormat<MY_CODEC>
{
  public:
    unsigned m_actualSampleRate;
    unsigned m_actualChannels;

    OpusPluginMediaFormat(const char * formatName, const char * rawFormat, unsigned actualSampleRate, unsigned actualChannels)
      : PluginCodec_AudioFormat<MY_CODEC>(formatName, "OPUS", MyDescription,
                                           960*actualChannels*actualSampleRate/48000,
                                           MAX_BIT_RATE*20/1000/8, // 20ms and bits to bytes
                                           48000, MyOptions)
      , m_actualSampleRate(actualSampleRate)
      , m_actualChannels(actualChannels)
    {
      m_rawFormat = rawFormat;
      m_recommendedFramesPerPacket = 1; // 20ms
      m_maxFramesPerPacket = 6; // 120ms
      m_maxBandwidth = MAX_BIT_RATE;
      m_frameTime = 20000; // Rare occasion where frame time not derived from samplesPerFrame and sampleRate
      m_flags |= PluginCodec_SetChannels(2) | PluginCodec_RTPTypeShared | PluginCodec_EmptyPayload;
    }

    virtual bool IsValidForProtocol(const char * protocol) const
	{
		return strcasecmp(protocol, PLUGINCODEC_OPTION_PROTOCOL_SIP) == 0;
	}

	virtual bool ToNormalised(OptionMap & /*original*/, OptionMap & /*changed*/) const
	{
		return true;
	}

    virtual bool ToCustomised(OptionMap & original, OptionMap & changed) const
    {
      changed[MaxPlaybackRate.m_name] = original[PLUGINCODEC_OPTION_MAX_BIT_RATE];
      changed[MaxCaptureRate.m_name] = original[PLUGINCODEC_OPTION_TARGET_BIT_RATE];
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
    bool     m_useInBandFEC;
    unsigned m_channels;

  public:
    OpusPluginCodec(const PluginCodec_Definition * defn)
      : PluginCodec<MY_CODEC>(defn)
      , m_useInBandFEC(true)
    {
      const OpusPluginMediaFormat *mediaFormat = reinterpret_cast<const OpusPluginMediaFormat *>(m_definition->userData);
      m_sampleRate = mediaFormat->m_actualSampleRate;
      m_channels = mediaFormat->m_actualChannels;
    }


    virtual bool SetOption(const char * optionName, const char * optionValue)
    {
      if (strcasecmp(optionName, UseInBandFEC.m_name) == 0) {
        if (!SetOptionBoolean(m_useInBandFEC, optionValue))
          return false;
        PTRACE(4, MY_CODEC_LOG, "In band FEC set to " << std::boolalpha << m_useInBandFEC);
        return true;
      }

      // Base class sets bit rate and frame time
      return PluginCodec<MY_CODEC>::SetOption(optionName, optionValue);
    }


    bool PacketHasFec(const opus_uint8 * payload, unsigned length)
    {
      if (payload == NULL || length == 0)
        return false;

      /* In CELT_ONLY mode, packets should not have FEC. */
      if (payload[0] & 0x80)
        return false;

      int frames;
      switch (opus_packet_get_samples_per_frame(payload, m_sampleRate)) {
        case 480:
        case 960:
          frames = 1;
          break;
        case 960*2:
          frames = 2;
          break;
        case 960*3:
          frames = 3;
          break;
        default:
          return false; // invalid packet.
      }

      /* The following is to parse the LBRR flags. */
      opus_int16 frame_sizes[48];
      const opus_uint8 *frame_data[48];
      if (opus_packet_parse(payload, length, NULL, frame_data, frame_sizes, NULL) < 0)
        return false;

      if (frame_sizes[0] <= 1)
        return false;

      int channels = opus_packet_get_nb_channels(payload);
      for (int n = 0; n < channels; n++) {
        if (frame_data[0][0] & (0x80 >> ((n + 1) * (frames + 1) - 1))) {
          PTRACE(5, MY_CODEC_LOG, "FEC packet detected");
          return true;
        }
      }

      return false;
    }
};


///////////////////////////////////////////////////////////////////////////////

class OpusPluginEncoder : public OpusPluginCodec
{
  protected:
    OpusEncoder * m_encoder;
    unsigned      m_dynamicPacketLoss;
    bool          m_useDTX;
    unsigned      m_bitRate;

  public:
    OpusPluginEncoder(const PluginCodec_Definition * defn)
      : OpusPluginCodec(defn)
      , m_encoder(NULL)
      , m_dynamicPacketLoss(0)
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
      if (strcasecmp(optionName, DynamicPacketLoss.m_name) == 0) {
        if (!SetOptionUnsigned(m_dynamicPacketLoss, optionValue, 0, 100))
          return false;
        PTRACE(4, MY_CODEC_LOG, "Dynamic packet loss set to " << m_dynamicPacketLoss << '%');
        return true;
      }

      if (strcasecmp(optionName, UseDTX.m_name) == 0)
        return SetOptionBoolean(m_useDTX, optionValue);

      if (strcasecmp(optionName, PLUGINCODEC_OPTION_TARGET_BIT_RATE) == 0)
        return SetOptionUnsigned(m_bitRate, optionValue, 6000, 510000);

      // Base class sets bit rate and frame time
      return OpusPluginCodec::SetOption(optionName, optionValue);
    }


    virtual bool OnChangedOptions()
    {
      if (m_encoder == NULL)
        return false;

      //opus_encoder_ctl(m_encoder, OPUS_SET_MAX_BANDWIDTH(m_definition->sampleRate));
      opus_encoder_ctl(m_encoder, OPUS_SET_INBAND_FEC(m_useInBandFEC));
      opus_encoder_ctl(m_encoder, OPUS_SET_PACKET_LOSS_PERC(m_dynamicPacketLoss));
      opus_encoder_ctl(m_encoder, OPUS_SET_DTX(m_useDTX));
      opus_encoder_ctl(m_encoder, OPUS_SET_BITRATE(m_bitRate));
      PTRACE(4, MY_CODEC_LOG, "Encoder options set:"
                              " fec=" << std::boolalpha << m_useInBandFEC << ","
                              " pkt-loss=" << m_dynamicPacketLoss << "%,"
                              " dtx=" << m_useDTX << ","
                              " bitrate=" << m_bitRate);
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
                                      (opus_uint8 *)toPtr, toLen);
      if (result < 0) {
        PTRACE(1, MY_CODEC_LOG, "Encoder error " << result << ' ' << opus_strerror(result));
        return false;
      }

      toLen = result;
      fromLen = opus_packet_get_samples_per_frame((const opus_uint8 *)toPtr, m_sampleRate) *
                opus_packet_get_nb_frames((const opus_uint8 *)toPtr, toLen) * m_channels * 2;
      return true;
    }
};


///////////////////////////////////////////////////////////////////////////////

class OpusPluginDecoder : public OpusPluginCodec
{
  protected:
    OpusDecoder           * m_decoder;
    unsigned                m_lostPackets;
    std::vector<opus_int16> m_previousFrame;

  public:
    OpusPluginDecoder(const PluginCodec_Definition * defn)
      : OpusPluginCodec(defn)
      , m_decoder(NULL)
      , m_lostPackets(0)
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
      int samples;
      const opus_uint8 * packet;
      if (fromLen == 0) {
        if (m_useInBandFEC && m_lostPackets++ == 0) {
          toLen = 0;
          return true;
        }
        packet = NULL; // As per opus_decode() API
        opus_decoder_ctl(m_decoder, OPUS_GET_LAST_PACKET_DURATION(&samples));
      }
      else {
        packet = (const opus_uint8 *)fromPtr;
        samples = opus_decoder_get_nb_samples(m_decoder, packet, fromLen);
        if (samples < 0) {
          PTRACE(1, MY_CODEC_LOG, "Decoding error " << samples << ' ' << opus_strerror(samples));
          return false;
        }
      }

      unsigned outputBytes = samples*m_channels*2U;
      if (outputBytes*2 > toLen) {
        PTRACE(1, MY_CODEC_LOG, "Provided sample buffer too small, " << toLen << " bytes, need " << outputBytes);
        return false;
      }

      if (!m_useInBandFEC) {
        toLen = outputBytes;
        return DecodeFrame(packet, fromLen, toPtr, samples, false);
      }

      if (m_previousFrame.empty()) {
        m_previousFrame.resize(samples*m_channels);
        toLen = 0;
      }
      else {
        toLen = outputBytes;
        memcpy(toPtr, m_previousFrame.data(), outputBytes);

        if (m_lostPackets > 0 && --m_lostPackets == 0) {
          if (!DecodeFrame(packet, fromLen, ((char *)toPtr)+outputBytes, samples, PacketHasFec(packet, fromLen)))
            return false;
          toLen += outputBytes;
        }
      }

      return DecodeFrame(packet, fromLen, m_previousFrame.data(), samples, false);
    }

    bool DecodeFrame(const void * packet, unsigned bytes, void * pcm, unsigned samples, bool fec)
    {
      int result = opus_decode(m_decoder, (const opus_uint8 *)packet, bytes, (opus_int16 *)pcm, samples, fec);
      if (result > 0)
        return true;

      PTRACE(1, MY_CODEC_LOG, "Decoder error " << result << ' ' << opus_strerror(result));
      return false;
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
