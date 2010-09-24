/*
 * Skype SILK Plugin codec for OPAL
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

#include "SILK_SDK/interface/SKP_Silk_SDK_API.h"


#define MY_CODEC   silk                        // Name of codec (use C variable characters)

static unsigned   MyVersion = PLUGIN_CODEC_VERSION_OPTIONS; // Minimum version for codec (should never change)
static const char MyDescription[] = "SILK Audio Codec";     // Human readable description of codec
static const char MyFormatName8k[] = "SILK-8";              // OpalMediaFormat name string to generate
static const char MyFormatName16k[] = "SILK-16";            // OpalMediaFormat name string to generate
static const char MyPayloadName[] = "SILK";                 // RTP payload name (IANA approved)
static unsigned   MyFrameTime = 20000;                      // Frame time in microseconds

static const char RawFormatName[] = "L16";                  // Raw media format


static struct PluginCodec_information LicenseInfo = {
  1143692893,                                                   // timestamp = Thu 30 Mar 2006 04:28:13 AM UTC

  "Robert Jongbloed, Vox Lucida Pty.Ltd.",                      // source code author
  "1.0",                                                        // source code version
  "robertj@voxlucida.com.au",                                   // source code email
  "http://www.voxlucida.com.au",                                // source code URL
  "Copyright (C) 2010 by Vox Lucida Pt.Ltd., All Rights Reserved", // source code copyright
  "MPL 1.0",                                                    // source code license
  PluginCodec_License_MPL,                                      // source code license
  
  MyDescription,                                                // codec description
  "Skype",                                                      // codec author
  "1.07",                                                       // codec version
  "silksupport@skype.net",                                      // codec email
  "http://developer.skype.com/silk",                            // codec URL
  "Copyright 2009-2010, Skype Limited",                         // codec copyright information
  "Skype Limited hereby separately grants to you a license "
  "under its patents to use this software for internal "
  "evaluation and testing purposes only. This license "
  "expressly excludes use of this software for distribution "
  "or use in any commercial product or any commercial or "
  "production use whatsoever.",                                 // codec license
  PluginCodec_License_ResearchAndDevelopmentUseOnly             // codec license code
};


///////////////////////////////////////////////////////////////////////////////

static struct PluginCodec_Option const UseInBandFEC =
{
  PluginCodec_BoolOption,
  "Use In-Band FEC",
  true,
  PluginCodec_AndMerge,
  "1",
  "useinbandfec",
  "1",
};

static struct PluginCodec_Option const UseDTX =
{
  PluginCodec_BoolOption,
  "Use DTX",
  true,
  PluginCodec_AndMerge,
  "0",
  "usedtx",
  "0",
};

static struct PluginCodec_Option const Complexity =
{
  PluginCodec_IntegerOption,
  "Complexity",
  true,
  PluginCodec_NoMerge,
  "1",
  NULL,
  NULL,
  0,
  "0",
  "2"
};

static struct PluginCodec_Option const PacketLossPercentage =
{
  PluginCodec_IntegerOption,
  "Packet Loss Percentage",
  true,
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
  &Complexity,
  NULL
};


///////////////////////////////////////////////////////////////////////////////

class MyPluginMediaFormat : public PluginCodec_MediaFormat
{
  public:
    MyPluginMediaFormat()
      : PluginCodec_MediaFormat(MyOptions)
    {
    }


    virtual bool ToNormalised(OptionMap & original, OptionMap & changed)
    {
      return true;
    }


    virtual bool ToCustomised(OptionMap & original, OptionMap & changed)
    {
      return true;
    }
};

static MyPluginMediaFormat MyMediaFormat;


///////////////////////////////////////////////////////////////////////////////

class MyEncoder : public PluginCodec
{
  protected:
    void * m_state;
    SKP_SILK_SDK_EncControlStruct m_control;

  public:
    MyEncoder(const PluginCodec_Definition * defn)
      : PluginCodec(defn)
      , m_state(NULL)
    {
    }


    ~MyEncoder()
    {
      if (m_state != NULL)
        free(m_state);
    }


    virtual bool Construct()
    {
      SKP_int32 sizeBytes = 0;
      if (SKP_Silk_SDK_Get_Encoder_Size(&sizeBytes) != 0)
        return false;

      m_state = malloc(sizeBytes);
      if (m_state == NULL)
        return false;

      if (SKP_Silk_SDK_InitEncoder(m_state, &m_control) != 0)
        return false;

      m_control.API_sampleRate = m_control.maxInternalSampleRate = m_definition->sampleRate;
      return true;
    }


    virtual bool SetOption(const char * optionName, const char * optionValue)
    {
      if (strcasecmp(optionName, UseInBandFEC.m_name) == 0)
        return SetOptionBoolean(m_control.useInBandFEC, optionValue);

      if (strcasecmp(optionName, UseDTX.m_name) == 0)
        return SetOptionBoolean(m_control.useDTX, optionValue);

      if (strcasecmp(optionName, Complexity.m_name) == 0)
        return SetOptionUnsigned(m_control.complexity, optionValue, 0, 2);

      if (strcasecmp(optionName, PacketLossPercentage.m_name) == 0)
        return SetOptionUnsigned(m_control.packetLossPercentage, optionValue, 0, 100);

      if (strcasecmp(optionName, PLUGINCODEC_OPTION_TARGET_BIT_RATE) == 0)
        return SetOptionUnsigned(m_control.bitRate, optionValue, 5000, 40000);

      if (strcasecmp(optionName, PLUGINCODEC_OPTION_TX_FRAMES_PER_PACKET) == 0) {
        unsigned frames = m_control.packetSize/m_definition->parm.audio.samplesPerFrame;
        if (!SetOptionUnsigned(frames, optionValue, 1, m_definition->parm.audio.maxFramesPerPacket))
          return false;
        m_control.packetSize = frames*m_definition->parm.audio.samplesPerFrame;
        return true;
      }

      // Base class sets bit rate and frame time
      return PluginCodec::SetOption(optionName, optionValue);
    }


    virtual bool Transcode(const void * fromPtr,
                             unsigned & fromLen,
                                 void * toPtr,
                             unsigned & toLen,
                             unsigned & flags)
    {
      if (fromLen/2 < m_definition->parm.audio.samplesPerFrame) {
        PTRACE(1, "SILK", "Provided samples too small, " << fromLen << " bytes");
        return false;
      }

      SKP_int16 nBytesOut = toLen;
      SKP_int error = SKP_Silk_SDK_Encode(m_state, &m_control,
                          (const SKP_int16 *)fromPtr, m_definition->parm.audio.samplesPerFrame,
                          (SKP_uint8 *)toPtr, &nBytesOut);
      fromLen = m_definition->parm.audio.samplesPerFrame*2;
      toLen = nBytesOut;

      if (error == 0)
        return true;

      PTRACE(1, "SILK", "Encoder error " << error);
      return false;
    }
};


///////////////////////////////////////////////////////////////////////////////

class MyDecoder : public PluginCodec
{
  protected:
    void * m_state;

  public:
    MyDecoder(const PluginCodec_Definition * defn)
      : PluginCodec(defn)
    {
    }


    ~MyDecoder()
    {
      if (m_state != NULL)
        free(m_state);
    }


    virtual bool Construct()
    {
      SKP_int32 sizeBytes = 0;
      if (SKP_Silk_SDK_Get_Decoder_Size(&sizeBytes) != 0)
        return false;

      m_state = malloc(sizeBytes);
      if (m_state == NULL)
        return false;

      return SKP_Silk_SDK_InitDecoder(m_state) == 0;
    }


    virtual bool Transcode(const void * fromPtr,
                             unsigned & fromLen,
                                 void * toPtr,
                             unsigned & toLen,
                             unsigned & flags)
    {
      SKP_SILK_SDK_DecControlStruct status;
      status.API_sampleRate = m_definition->sampleRate;

      SKP_int16 nSamplesOut = toLen/2;
      SKP_int error = SKP_Silk_SDK_Decode(m_state, &status,
                                          false,
                                          (const SKP_uint8 *)fromPtr, fromLen,
                                          (SKP_int16 *)toPtr, &nSamplesOut);
      toLen = nSamplesOut*2;

      if (status.moreInternalDecoderFrames)
        fromLen = 0;

      if (error == 0)
        return true;

      PTRACE(1, "SILK", "Decoder error " << error);
      return false;
    }
};


///////////////////////////////////////////////////////////////////////////////

PLUGINCODEC_DEFINE_CONTROL_TABLE(MyControls);

static struct PluginCodec_Definition MyCodecDefinition[] =
{
  {
    // Encoder 8kHz
    MyVersion,                          // codec API version
    &LicenseInfo,                       // license information

    PluginCodec_MediaTypeAudio |        // audio codec
    PluginCodec_RTPTypeDynamic,         // dynamic RTP type

    MyDescription,                      // text decription
    RawFormatName,                      // source format
    MyFormatName8k,                     // destination format

    &MyMediaFormat,                     // user data 

    8000,                               // samples per second
    20000,                              // raw bits per second
    MyFrameTime,                        // microseconds per frame

    {{
      160,                              // samples per frame
      50,                               // max bytes per frame
      2,                                // recommended number of frames per packet
      5,                                // maximum number of frames per packet
    }},
    
    0,                                  // IANA RTP payload code
    MyPayloadName,                      // IANA RTP payload name

    PluginCodec::Create<MyEncoder>,     // create codec function
    PluginCodec::Destroy,               // destroy codec
    PluginCodec::Transcode,             // encode/decode
    MyControls,                         // codec controls

    PluginCodec_H323Codec_NoH323,       // h323CapabilityType 
    NULL                                // h323CapabilityData
  },
  { 
    // Decoder 8kHz
    MyVersion,                          // codec API version
    &LicenseInfo,                       // license information

    PluginCodec_MediaTypeAudio |        // audio codec
    PluginCodec_RTPTypeDynamic,         // dynamic RTP type

    MyDescription,                      // text decription
    MyFormatName8k,                     // source format
    RawFormatName,                      // destination format

    &MyMediaFormat,                     // user data 

    8000,                               // samples per second
    20000,                              // raw bits per second
    MyFrameTime,                        // microseconds per frame

    {{
      160,                              // samples per frame
      50,                               // max bytes per frame
      2,                                // recommended number of frames per packet
      5,                                // maximum number of frames per packet
    }},
    
    0,                                  // IANA RTP payload code
    MyPayloadName,                      // IANA RTP payload name

    PluginCodec::Create<MyDecoder>,     // create codec function
    PluginCodec::Destroy,               // destroy codec
    PluginCodec::Transcode,             // encode/decode
    MyControls,                         // codec controls

    PluginCodec_H323Codec_NoH323,       // h323CapabilityType 
    NULL                                // h323CapabilityData
  },
  {
    // Encoder 16kHz
    MyVersion,                          // codec API version
    &LicenseInfo,                       // license information

    PluginCodec_MediaTypeAudio |        // audio codec
    PluginCodec_RTPTypeDynamic,         // dynamic RTP type

    MyDescription,                      // text decription
    RawFormatName,                      // source format
    MyFormatName16k,                    // destination format

    &MyMediaFormat,                     // user data 

    16000,                              // samples per second
    30000,                              // raw bits per second
    MyFrameTime,                        // microseconds per frame

    {{
      320,                              // samples per frame
      75,                               // max bytes per frame
      2,                                // recommended number of frames per packet
      5,                                // maximum number of frames per packet
    }},
    
    0,                                  // IANA RTP payload code
    MyPayloadName,                      // IANA RTP payload name

    PluginCodec::Create<MyEncoder>,     // create codec function
    PluginCodec::Destroy,               // destroy codec
    PluginCodec::Transcode,             // encode/decode
    MyControls,                         // codec controls

    PluginCodec_H323Codec_NoH323,       // h323CapabilityType 
    NULL                                // h323CapabilityData
  },
  { 
    // Decoder 16kHz
    MyVersion,                          // codec API version
    &LicenseInfo,                       // license information

    PluginCodec_MediaTypeAudio |        // audio codec
    PluginCodec_RTPTypeDynamic,         // dynamic RTP type

    MyDescription,                      // text decription
    MyFormatName16k,                    // source format
    RawFormatName,                      // destination format

    &MyMediaFormat,                     // user data 

    16000,                              // samples per second
    30000,                              // raw bits per second
    MyFrameTime,                        // microseconds per frame

    {{
      320,                              // samples per frame
      75,                               // max bytes per frame
      2,                                // recommended number of frames per packet
      5,                                // maximum number of frames per packet
    }},
    
    0,                                  // IANA RTP payload code
    MyPayloadName,                      // IANA RTP payload name

    PluginCodec::Create<MyDecoder>,     // create codec function
    PluginCodec::Destroy,               // destroy codec
    PluginCodec::Transcode,             // encode/decode
    MyControls,                         // codec controls

    PluginCodec_H323Codec_NoH323,       // h323CapabilityType 
    NULL                                // h323CapabilityData
  }
};

extern "C"
{
  PLUGIN_CODEC_IMPLEMENT_ALL(MY_CODEC, MyCodecDefinition, MyVersion)
};

/////////////////////////////////////////////////////////////////////////////
