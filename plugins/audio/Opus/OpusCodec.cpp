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
 */

#include <codec/opalplugin.hpp>
#include <codec/known.h>

#ifndef PLUGIN_CODEC_DLL_EXPORTS
#include "plugin_config.h"
#endif

#include "../../../src/codec/opusmf_inc.cxx"

#include "opus.h"

#include <vector>

#ifdef _MSC_VER
#pragma warning(disable:4505)
#define snprintf _snprintf
#endif


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


///////////////////////////////////////////////////////////////////////////////

static struct PluginCodec_Option const UseInBandFEC =
{
  PluginCodec_BoolOption,
  UseInBandFEC_OptionName,
  false,
  PluginCodec_AndMerge,
  STRINGIZE(DEFAULT_USE_FEC),
  UseInBandFEC_FMTPName
};

static struct PluginCodec_Option const UseDTX =
{
  PluginCodec_BoolOption,
  UseDTX_OptionName,
  false,
  PluginCodec_AndMerge,
  STRINGIZE(DEFAULT_USE_DTX),
  UseDTX_FMTPName,
  "0"
};

static struct PluginCodec_Option const ConstantBitRate =
{
  PluginCodec_BoolOption,
  ConstantBitRate_OptionName,
  false,
  PluginCodec_AndMerge,
  STRINGIZE(DEFAULT_CBR),
  ConstantBitRate_FMTPName,
  "0"
};

static struct PluginCodec_Option const MaxPlaybackRate =
{
  PluginCodec_IntegerOption,
  MaxPlaybackRate_OptionName,
  true,
  PluginCodec_NoMerge,
  STRINGIZE(OPUS_SAMPLE_RATE),
  MaxPlaybackRate_FMTPName,
  STRINGIZE(MAX_SAMPLE_RATE),
  0,
  STRINGIZE(MIN_SAMPLE_RATE),
  STRINGIZE(MAX_SAMPLE_RATE)
};

static struct PluginCodec_Option const MaxCaptureRate =
{
  PluginCodec_IntegerOption,
  MaxCaptureRate_OptionName,
  true,
  PluginCodec_NoMerge,
  STRINGIZE(OPUS_SAMPLE_RATE),
  MaxCaptureRate_FMTPName,
  STRINGIZE(MAX_SAMPLE_RATE),
  0,
  STRINGIZE(MIN_SAMPLE_RATE),
  STRINGIZE(MAX_SAMPLE_RATE)
};

static struct PluginCodec_Option const PlaybackStereo =
{
  PluginCodec_BoolOption,
  PlaybackStereo_OptionName,
  true,
  PluginCodec_NoMerge,
  STRINGIZE(DEFAULT_STEREO),
  PlaybackStereo_FMTPName
};

static struct PluginCodec_Option const CaptureStereo =
{
  PluginCodec_BoolOption,
  CaptureStereo_OptionName,
  true,
  PluginCodec_NoMerge,
  STRINGIZE(DEFAULT_STEREO),
  CaptureStereo_FMTPName
};

static struct PluginCodec_Option const MaxAverageBitRate =
{
  PluginCodec_IntegerOption,
  PLUGINCODEC_OPTION_MAX_BIT_RATE,
  true,
  PluginCodec_MinMerge,
  STRINGIZE(DEFAULT_BIT_RATE),
  MaxAverageBitRate_FMTPName,
  "",
  0,
  STRINGIZE(MIN_BIT_RATE),
  STRINGIZE(MAX_BIT_RATE)
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
  "0","100" // percentage
};

static struct PluginCodec_Option const Complexity =
{
  PluginCodec_IntegerOption,
  "Complexity",
  false,
  PluginCodec_NoMerge,
  "9",
  NULL,
  NULL,
  0,
  "0","10"
};

static struct PluginCodec_Option const * const MyOptions[] = {
  &UseInBandFEC,
  &UseDTX,
  &ConstantBitRate,
  &MaxPlaybackRate,
  &MaxCaptureRate,
  &PlaybackStereo,
  &CaptureStereo,
  &MaxAverageBitRate,
  &DynamicPacketLoss,
  &Complexity,
  NULL
};


class OpusPluginMediaFormat : public PluginCodec_AudioFormat<MY_CODEC>
{
  public:
    unsigned m_actualSampleRate;
    unsigned m_actualChannels;

    OpusPluginMediaFormat(const char * formatName, const char * rawFormat, unsigned actualSampleRate, unsigned actualChannels)
      : PluginCodec_AudioFormat<MY_CODEC>(formatName, OpusEncodingName, MyDescription,
                                           960*actualChannels*actualSampleRate/OPUS_SAMPLE_RATE,
                                           MAX_BIT_RATE*OPUS_FRAME_MS/1000/8, // 20ms and bits to bytes
                                           OPUS_SAMPLE_RATE, MyOptions)
      , m_actualSampleRate(actualSampleRate)
      , m_actualChannels(actualChannels)
    {
      m_rawFormat = rawFormat;
      m_recommendedFramesPerPacket = 1; // 20ms
      m_maxFramesPerPacket = 120/OPUS_FRAME_MS; // 120ms
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
      changed[PlaybackStereo.m_name] = changed[CaptureStereo.m_name] = m_actualChannels == 1 ? "0" : "1";
      return true;
    }
};

#define CHANNEL  1
#define CHANNELS 2
#define DEF_MEDIA_FORMAT(rate, stereo) \
  static OpusPluginMediaFormat const MyMediaFormat##rate##stereo(OPAL_OPUS##rate##stereo, \
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
    int      m_countFEC;
    unsigned m_channels;

  public:
    OpusPluginCodec(const PluginCodec_Definition * defn)
      : PluginCodec<MY_CODEC>(defn)
      , m_useInBandFEC(true)
      , m_countFEC(0)
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
        if (!m_useInBandFEC)
          m_countFEC = -1;
        PTRACE(4, MY_CODEC_LOG, "In band FEC set to " << std::boolalpha << m_useInBandFEC);
        return true;
      }

      // Base class sets bit rate and frame time
      return PluginCodec<MY_CODEC>::SetOption(optionName, optionValue);
    }


    virtual int GetStatistics(char * bufferPtr, unsigned bufferSize)
    {
      return snprintf(bufferPtr, bufferSize, "FEC=%u\n", m_countFEC);
    }


    bool PacketHasFec(const opus_uint8 * payload, unsigned length)
    {
      if (payload == NULL || length == 0)
        return false;

      // In CELT_ONLY mode, packets does not have FEC.
      if (payload[0] & 0x80)
        return false;

      // Get the individual SILK frames in the packet.
      opus_int16 frame_sizes[48];
      const opus_uint8 *frame_data[48];
      int frames = opus_packet_parse(payload, length, NULL, frame_data, frame_sizes, NULL);
      if (frames < 0) {
        PTRACE(1, MY_CODEC_LOG, "Packet parse error " << frames << ' ' << opus_strerror(frames));
        return false;
      }

      // "frames" is over used. We have the frames in the packet from above, then we
      // have the SILK encoded 20ms of audio frame, I'm calling them a sub-Frames.
      int subFrames = (opus_packet_get_samples_per_frame(payload, m_sampleRate)*1000/m_sampleRate+19)/20;
      int channels = opus_packet_get_nb_channels(payload);

      // The following is to parse the LBRR flags.
      for (int frame = 0; frame < frames; ++frame) {
        if (frame_sizes[frame] > 0) {
          for (int chan = 0; chan < channels; chan++) {
            // Highest "subFrames" bits are VAD, next bit is LBRR flag, repeated for each channel
            if (frame_data[frame][0] & (0x80 >> ((subFrames+1)*chan + subFrames))) {
              PTRACE(6, MY_CODEC_LOG, "FEC packet detected");
              ++m_countFEC;
              return true;
            }
          }
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
    opus_int32    m_complexity;

  public:
    OpusPluginEncoder(const PluginCodec_Definition * defn)
      : OpusPluginCodec(defn)
      , m_encoder(NULL)
      , m_dynamicPacketLoss(0)
      , m_useDTX(false)
      , m_bitRate(12000)
      , m_complexity(0)
    {
      PTRACE(4, MY_CODEC_LOG, "Encoder created: version \"" << opus_get_version_string() << '"');
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

      if (strcasecmp(optionName, Complexity.m_name) == 0)
          return SetOptionUnsigned(m_complexity, optionValue, 0, 10);

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
      opus_encoder_ctl(m_encoder, OPUS_SET_COMPLEXITY(m_complexity));
      PTRACE(4, MY_CODEC_LOG, "Encoder options set:"
                              " fec=" << std::boolalpha << m_useInBandFEC << ","
                              " pkt-loss=" << m_dynamicPacketLoss << "%,"
                              " dtx=" << m_useDTX << ","
                              " bitrate=" << m_bitRate << ","
                              " complexity=" << m_complexity);
      return true;
    }


    virtual bool Transcode(const void * fromPtr,
                             unsigned & fromLen,
                                 void * toPtr,
                             unsigned & toLen,
                             unsigned & flags)
    {
      if (fromLen == 0) {
        static const short silence[20 * 48] = { };
        fromPtr = silence;
        fromLen = sizeof(silence);;
      }

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
      PacketHasFec((opus_uint8 *)toPtr, toLen);
      return true;
    }
};


///////////////////////////////////////////////////////////////////////////////

class OpusPluginDecoder : public OpusPluginCodec
{
  protected:
    OpusDecoder * m_decoder;

    enum {
      AwaitingInitialPacket,
      NoLostPackets,
      UseFEC
    } m_lostPacketState;

  public:
    OpusPluginDecoder(const PluginCodec_Definition * defn)
      : OpusPluginCodec(defn)
      , m_decoder(NULL)
      , m_lostPacketState(AwaitingInitialPacket)
    {
      PTRACE(4, MY_CODEC_LOG, "Decoder created: version \"" << opus_get_version_string() << '"');
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
        switch (m_lostPacketState) {
          default :
            break;

          case NoLostPackets :
            if (!m_useInBandFEC)
              break;

            m_lostPacketState = UseFEC;
            // Do next case

          case AwaitingInitialPacket :
            toLen = 0;
            return true;
        }

        packet = NULL; // As per opus_decode() API
        opus_decoder_ctl(m_decoder, OPUS_GET_LAST_PACKET_DURATION(&samples));
      }
      else {
        if (m_lostPacketState == AwaitingInitialPacket) {
          PTRACE(4, MY_CODEC_LOG, "First non-empty packet received for decoding.");
          m_lostPacketState = NoLostPackets;
        }

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
      toLen = outputBytes;

      if (m_lostPacketState == NoLostPackets)
        return DecodeFrame(packet, fromLen, toPtr, samples, false);

      m_lostPacketState = NoLostPackets;
      toLen = outputBytes*2;

      if (PacketHasFec(packet, fromLen)) {
        if (!DecodeFrame(packet, fromLen, toPtr, samples, true))
          return false;
      }
      else {
        if (!DecodeFrame(NULL, fromLen, toPtr, samples, false))
          return false;
      }

      return DecodeFrame(packet, fromLen, ((char *)toPtr)+outputBytes, samples, false);
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
