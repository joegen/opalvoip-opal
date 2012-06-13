/*
 * H.264 Plugin codec for OPAL
 *
 * This the starting point for creating new H.264 plug in codecs for
 * OPAL in C++. The requirements of H.264 over H.323 and SIP are quite
 * complex. The mapping of width/height/frame rate/bit rate to the
 * H.264 profile/level with possible overrides (e.g. MaxMBS for maximum
 * macro blocks per second) which are not universally supported by all
 * endpoints make precise control of the video difficult and in some
 * cases impossible.
 *
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
 * Contributor(s): Matthias Schneider (ma30002000@yahoo.de)
 *                 Michele Piccini (michele@piccini.com)
 *                 Derek Smithies (derek@indranet.co.nz)
 *                 Robert Jongbloed (robertj@voxlucida.com.au)
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#ifndef PLUGIN_CODEC_DLL_EXPORTS
#include "plugin-config.h"
#endif

#define MY_CODEC x264  // Name of codec (use C variable characters)

#define OPAL_H323 1
#define OPAL_SIP 1

#include "../../../src/codec/h264mf_inc.cxx"

#include "../common/ffmpeg.h"

#include "shared/h264frame.h"
#include "shared/x264wrap.h"


///////////////////////////////////////////////////////////////////////////////

class MY_CODEC { };

PLUGINCODEC_CONTROL_LOG_FUNCTION_DEF

static const char MyDescription[] = "x264 Video Codec";     // Human readable description of codec

PLUGINCODEC_LICENSE(
  "Matthias Schneider\n"
  "Robert Jongbloed, Vox Lucida Pty.Ltd.",                      // source code author
  "1.0",                                                        // source code version
  "robertj@voxlucida.com.au",                                   // source code email
  "http://www.voxlucida.com.au",                                // source code URL
  "Copyright (C) 2006 by Matthias Schneider\n"
  "Copyright (C) 2011 by Vox Lucida Pt.Ltd., All Rights Reserved", // source code copyright
  "MPL 1.0",                                                    // source code license
  PluginCodec_License_MPL,                                      // source code license
  
  MyDescription,                                                // codec description
  "x264: Laurent Aimar, ffmpeg: Michael Niedermayer",           // codec author
  "", 							        // codec version
  "fenrir@via.ecp.fr, ffmpeg-devel-request@mplayerhq.hu",       // codec email
  "http://developers.videolan.org/x264.html, \
   http://ffmpeg.mplayerhq.hu", 				// codec URL
  "x264: Copyright (C) 2003 Laurent Aimar, \
   ffmpeg: Copyright (c) 2002-2003 Michael Niedermayer",        // codec copyright information
  "x264: GNU General Public License as published Version 2, \
   ffmpeg: GNU Lesser General Public License, Version 2.1",     // codec license
  PluginCodec_License_LGPL                                      // codec license code
);


///////////////////////////////////////////////////////////////////////////////

static struct PluginCodec_Option const Profile =
{
  PluginCodec_EnumOption,             // Option type
  ProfileName,                        // User visible name
  false,                              // User Read/Only flag
  PluginCodec_MinMerge,               // Merge mode
  DefaultProfileStr,                  // Initial value
  NULL,                               // FMTP option name
  NULL,                               // FMTP default value
  0,                                  // H.245 generic capability code and bit mask
  // Enum values, single string of value separated by colons
  H264_PROFILE_STR_BASELINE ":"
  H264_PROFILE_STR_MAIN     ":"
  H264_PROFILE_STR_EXTENDED
};                                  

static struct PluginCodec_Option const Level =
{
  PluginCodec_EnumOption,             // Option type
  LevelName,                          // User visible name
  false,                              // User Read/Only flag
  PluginCodec_MinMerge,               // Merge mode
  DefaultLevelStr,                    // Initial value
  NULL,                               // FMTP option name
  NULL,                               // FMTP default value
  0,                                  // H.245 generic capability code and bit mask
  // Enum values, single string of value separated by colons
  H264_LEVEL_STR_1   ":"
  H264_LEVEL_STR_1_b ":" 
  H264_LEVEL_STR_1_1 ":"
  H264_LEVEL_STR_1_2 ":"
  H264_LEVEL_STR_1_3 ":"
  H264_LEVEL_STR_2   ":"
  H264_LEVEL_STR_2_1 ":"
  H264_LEVEL_STR_2_2 ":"
  H264_LEVEL_STR_3   ":"
  H264_LEVEL_STR_3_1 ":"
  H264_LEVEL_STR_3_2 ":"
  H264_LEVEL_STR_4   ":"
  H264_LEVEL_STR_4_1 ":"
  H264_LEVEL_STR_4_2 ":"
  H264_LEVEL_STR_5   ":"
  H264_LEVEL_STR_5_1
};

static struct PluginCodec_Option const H241Profiles =
{
  PluginCodec_IntegerOption,          // Option type
  H241ProfilesName,                   // User visible name
  true,                               // User Read/Only flag
  PluginCodec_MinMerge,               // Merge mode
  STRINGIZE(DefaultProfileH241),      // Initial value
  NULL,                               // FMTP option name
  NULL,                               // FMTP default value
  H241_PROFILES,                      // H.245 generic capability code and bit mask
  "1",                                // Minimum value
  "127"                               // Maximum value
};

static struct PluginCodec_Option const H241Level =
{
  PluginCodec_IntegerOption,          // Option type
  H241LevelName,                      // User visible name
  true,                               // User Read/Only flag
  PluginCodec_MinMerge,               // Merge mode
  STRINGIZE(DefaultLevelH241),        // Initial value
  NULL,                               // FMTP option name
  NULL,                               // FMTP default value
  H241_LEVEL,                         // H.245 generic capability code and bit mask
  "15",                               // Minimum value
  "113"                               // Maximum value
};

static struct PluginCodec_Option const SDPProfileAndLevel =
{
  PluginCodec_OctetsOption,           // Option type
  SDPProfileAndLevelName,             // User visible name
  true,                               // User Read/Only flag
  PluginCodec_NoMerge,                // Merge mode
  DefaultSDPProfileAndLevel,          // Initial value
  SDPProfileAndLevelFMTPName,         // FMTP option name
  SDPProfileAndLevelFMTPDflt          // FMTP default value (as per RFC)
};

static struct PluginCodec_Option const MaxMBPS_SDP =
{
  PluginCodec_IntegerOption,          // Option type
  MaxMBPS_SDP_Name,                   // User visible name
  true,                               // User Read/Only flag
  PluginCodec_MinMerge,               // Merge mode
  "0",                                // Initial value
  MaxMBPS_FMTPName,                   // FMTP option name
  "0",                                // FMTP default value
  0,                                  // H.245 generic capability code and bit mask
  "0",                                // Minimum value
  "983040"                            // Maximum value
};

static struct PluginCodec_Option const MaxMBPS_H241 =
{
  PluginCodec_IntegerOption,          // Option type
  MaxMBPS_H241_Name,                  // User visible name
  true,                               // User Read/Only flag
  PluginCodec_MinMerge,               // Merge mode
  "0",                                // Initial value
  NULL,                               // FMTP option name
  NULL,                               // FMTP default value
  H241_CustomMaxMBPS,                 // H.245 generic capability code and bit mask
  "0",                                // Minimum value
  "1966"                              // Maximum value
#ifdef PLUGIN_CODEC_VERSION_H245_DEF_GEN_PARAM
  ,
  NULL,
  NULL,
  "0"                                 // H.245 default value
#endif
};

static struct PluginCodec_Option const MaxSMBPS_SDP =
{
  PluginCodec_IntegerOption,          // Option type
  MaxSMBPS_SDP_Name,                  // User visible name
  true,                               // User Read/Only flag
  PluginCodec_MinMerge,               // Merge mode
  "0",                                // Initial value
  MaxSMBPS_FMTPName,                  // FMTP option name
  "0",                                // FMTP default value
  0,                                  // H.245 generic capability code and bit mask
  "0",                                // Minimum value
  "983040"                            // Maximum value
};

static struct PluginCodec_Option const MaxSMBPS_H241 =
{
  PluginCodec_IntegerOption,          // Option type
  MaxSMBPS_H241_Name,                 // User visible name
  true,                               // User Read/Only flag
  PluginCodec_MinMerge,               // Merge mode
  "0",                                // Initial value
  NULL,                               // FMTP option name
  NULL,                               // FMTP default value
  H241_CustomMaxSMBPS,                // H.245 generic capability code and bit mask
  "0",                                // Minimum value
  "1966"                              // Maximum value
#ifdef PLUGIN_CODEC_VERSION_H245_DEF_GEN_PARAM
  ,
  NULL,
  NULL,
  "0"                                 // H.245 default value
#endif
};

static struct PluginCodec_Option const MaxFS_SDP =
{
  PluginCodec_IntegerOption,          // Option type
  MaxFS_SDP_Name,                     // User visible name
  true,                               // User Read/Only flag
  PluginCodec_MinMerge,               // Merge mode
  "0",                                // Initial value
  MaxFS_FMTPName,                     // FMTP option name
  "0",                                // FMTP default value
  0,                                  // H.245 generic capability code and bit mask
  "0",                                // Minimum value
  "36864"                             // Maximum value
};

static struct PluginCodec_Option const MaxFS_H241 =
{
  PluginCodec_IntegerOption,          // Option type
  MaxFS_H241_Name,                    // User visible name
  true,                               // User Read/Only flag
  PluginCodec_MinMerge,               // Merge mode
  "0",                                // Initial value
  NULL,                               // FMTP option name
  NULL,                               // FMTP default value
  H241_CustomMaxFS,                   // H.245 generic capability code and bit mask
  "0",                                // Minimum value
  "144"                               // Maximum value
#ifdef PLUGIN_CODEC_VERSION_H245_DEF_GEN_PARAM
  ,
  NULL,
  NULL,
  "0"                                 // H.245 default value
#endif
};

static struct PluginCodec_Option const MaxBR_SDP =
{
  PluginCodec_IntegerOption,          // Option type
  MaxBR_SDP_Name,                     // User visible name
  true,                               // User Read/Only flag
  PluginCodec_MinMerge,               // Merge mode
  "0",                                // Initial value
  MaxBR_FMTPName,                     // FMTP option name
  "0",                                // FMTP default value
  0,                                  // H.245 generic capability code and bit mask
  "0",                                // Minimum value
  "240000"                            // Maximum value
};

static struct PluginCodec_Option const MaxBR_H241 =
{
  PluginCodec_IntegerOption,          // Option type
  MaxBR_H241_Name,                    // User visible name
  true,                               // User Read/Only flag
  PluginCodec_MinMerge,               // Merge mode
  "0",                                // Initial value
  NULL,                               // FMTP option name
  NULL,                               // FMTP default value
  H241_CustomMaxBRandCPB,             // H.245 generic capability code and bit mask
  "0",                                // Minimum value
  "9600"                              // Maximum value
#ifdef PLUGIN_CODEC_VERSION_H245_DEF_GEN_PARAM
  ,
  NULL,
  NULL,
  "0"                                 // H.245 default value
#endif
};

static struct PluginCodec_Option const MaxNaluSize =
{
  PluginCodec_IntegerOption,          // Option type
  MaxNALUSizeName,                    // User visible name
  false,                              // User Read/Only flag
  PluginCodec_MinMerge,               // Merge mode
  STRINGIZE(H241_MAX_NALU_SIZE),      // Initial value
  MaxNALUSizeFMTPName,                // FMTP option name
  STRINGIZE(H241_MAX_NALU_SIZE),      // FMTP default value
  H241_Max_NAL_unit_size,             // H.245 generic capability code and bit mask
  "396",                              // Minimum value - uncompressed macro block size 16*16*3+12
  "65535"                             // Maximum value
#ifdef PLUGIN_CODEC_VERSION_H245_DEF_GEN_PARAM
  ,
  NULL,
  NULL,
  STRINGIZE(H241_MAX_NALU_SIZE)       // H.245 default value
#endif
};

static struct PluginCodec_Option const MediaPacketizationsH323_0 =
{
  PluginCodec_StringOption,           // Option type
  PLUGINCODEC_MEDIA_PACKETIZATIONS,   // User visible name
  false,                              // User Read/Only flag
  PluginCodec_IntersectionMerge,      // Merge mode
  OpalPluginCodec_Identifer_H264_Aligned // Initial value
};

static struct PluginCodec_Option const MediaPacketizationsH323_1 =
{
  PluginCodec_StringOption,           // Option type
  PLUGINCODEC_MEDIA_PACKETIZATIONS,   // User visible name
  false,                              // User Read/Only flag
  PluginCodec_IntersectionMerge,      // Merge mode
  OpalPluginCodec_Identifer_H264_Aligned "," // Initial value
  OpalPluginCodec_Identifer_H264_NonInterleaved
};

static struct PluginCodec_Option const PacketizationModeSDP_0 =
{
  PluginCodec_IntegerOption,          // Option type
  PacketizationModeName,              // User visible name
  true,                               // User Read/Only flag
  PluginCodec_EqualMerge,             // Merge mode
  "0",                                // Initial value
  PacketizationFMTPName,              // FMTP option name
  "0",                                // FMTP default value
  0,                                  // H.245 generic capability code and bit mask
  "0",                                // Minimum value
  "2"                                 // Maximum value
};

static struct PluginCodec_Option const PacketizationModeSDP_1 =
{
  PluginCodec_IntegerOption,          // Option type
  PacketizationModeName,              // User visible name
  true,                               // User Read/Only flag
  PluginCodec_EqualMerge,             // Merge mode
  "1",                                // Initial value
  PacketizationFMTPName,              // FMTP option name
  "0",                                // FMTP default value
  0,                                  // H.245 generic capability code and bit mask
  "0",                                // Minimum value
  "2"                                 // Maximum value
};

static struct PluginCodec_Option const TemporalSpatialTradeOff =
{
  PluginCodec_IntegerOption,          // Option type
  PLUGINCODEC_OPTION_TEMPORAL_SPATIAL_TRADE_OFF, // User visible name
  false,                              // User Read/Only flag
  PluginCodec_AlwaysMerge,            // Merge mode
  "31",                               // Initial value
  NULL,                               // FMTP option name
  NULL,                               // FMTP default value
  0,                                  // H.245 generic capability code and bit mask
  "1",                                // Minimum value
  "31"                                // Maximum value
};

static struct PluginCodec_Option const SendAccessUnitDelimiters =
{
  PluginCodec_BoolOption,                         // Option type
  "Send Access Unit Delimiters",                  // User visible name
  false,                                          // User Read/Only flag
  PluginCodec_AndMerge,                           // Merge mode
  STRINGIZE(DefaultSendAccessUnitDelimiters)      // Initial value
};

static struct PluginCodec_Option const * const MyOptionTable_0[] = {
  &Profile,
  &Level,
  &H241Profiles,
  &H241Level,
  &SDPProfileAndLevel,
  &MaxMBPS_H241,
  &MaxMBPS_SDP,
  &MaxSMBPS_H241,
  &MaxSMBPS_SDP,
  &MaxFS_H241,
  &MaxFS_SDP,
  &MaxBR_H241,
  &MaxBR_SDP,
  &MaxNaluSize,
  &TemporalSpatialTradeOff,
  &SendAccessUnitDelimiters,
  &PacketizationModeSDP_0,
  &MediaPacketizationsH323_0,  // Note: must be last entry
  NULL
};

static struct PluginCodec_Option const * const MyOptionTable_1[] = {
  &Profile,
  &Level,
  &H241Profiles,
  &H241Level,
  &SDPProfileAndLevel,
  &MaxMBPS_H241,
  &MaxMBPS_SDP,
  &MaxSMBPS_H241,
  &MaxSMBPS_SDP,
  &MaxFS_H241,
  &MaxFS_SDP,
  &MaxBR_H241,
  &MaxBR_SDP,
  &MaxNaluSize,
  &TemporalSpatialTradeOff,
  &SendAccessUnitDelimiters,
  &PacketizationModeSDP_1,
  &MediaPacketizationsH323_1,  // Note: must be last entry
  NULL
};


static struct PluginCodec_H323GenericCodecData MyH323GenericData = {
  OpalPluginCodec_Identifer_H264_Generic
};


///////////////////////////////////////////////////////////////////////////////

class H264_PluginMediaFormat : public PluginCodec_VideoFormat<MY_CODEC>
{
public:
  typedef PluginCodec_VideoFormat<MY_CODEC> BaseClass;

  H264_PluginMediaFormat(const char * formatName, OptionsTable options)
    : BaseClass(formatName, H264EncodingName, MyDescription, LevelInfo[sizeof(LevelInfo)/sizeof(LevelInfo[0])-1].m_MaxBitRate, options)
  {
    m_h323CapabilityType = PluginCodec_H323Codec_generic;
    m_h323CapabilityData = &MyH323GenericData;
  }


  virtual bool ToNormalised(OptionMap & original, OptionMap & changed)
  {
    return MyToNormalised(original, changed);
  }


  virtual bool ToCustomised(OptionMap & original, OptionMap & changed)
  {
    return MyToCustomised(original, changed);
  }
}; 

/* SIP requires two completely independent media formats for packetisation
   modes zero and one. */
static H264_PluginMediaFormat MyMediaFormatInfo_0(H264_Mode0_FormatName, MyOptionTable_0);
static H264_PluginMediaFormat MyMediaFormatInfo_1(H264_Mode1_FormatName, MyOptionTable_1);


///////////////////////////////////////////////////////////////////////////////

class H264_Encoder : public PluginVideoEncoder<MY_CODEC>
{
    typedef PluginVideoEncoder<MY_CODEC> BaseClass;

  protected:
    unsigned m_profile;
    unsigned m_level;
    unsigned m_constraints;
    unsigned m_maxMBPS;
    unsigned m_maxNALUSize;
    unsigned m_packetisationModeSDP;
    unsigned m_packetisationModeH323;
    bool     m_isH323;

    H264Encoder m_encoder;

  public:
    H264_Encoder(const PluginCodec_Definition * defn)
      : BaseClass(defn)
      , m_profile(DefaultProfileInt)
      , m_level(DefaultLevelInt)
      , m_constraints(0)
      , m_maxMBPS(0)
      , m_maxNALUSize(H241_MAX_NALU_SIZE)
      , m_packetisationModeSDP(1)
      , m_packetisationModeH323(1)
      , m_isH323(false)
    {
      PTRACE(4, MY_CODEC_LOG, "Created encoder $Revision$");
    }


    virtual bool Construct()
    {
      /* Complete construction of object after it has been created. This
         allows you to fail the create operation (return false), which cannot
         be done in the normal C++ constructor. */

      // ABR with bit rate tolerance = 1 is CBR...
      if (m_encoder.Load(this))
        return true;

      PTRACE(1, MY_CODEC_LOG, "Could not open encoder.");
      return false;
    }


    virtual bool SetOption(const char * optionName, const char * optionValue)
    {
      if (strcasecmp(optionName, MaxNaluSize.m_name) == 0)
        return SetOptionUnsigned(m_maxNALUSize, optionValue, 256, 8192);

      if (strcasecmp(optionName, MaxMBPS_SDP.m_name) == 0)
        return SetOptionUnsigned(m_maxMBPS, optionValue, 0);

      if (strcasecmp(optionName, Profile.m_name) == 0) {
        for (size_t i = 0; i < sizeof(ProfileInfo)/sizeof(ProfileInfo[0]); ++i) {
          if (strcasecmp(optionValue, ProfileInfo[i].m_Name) == 0) {
            m_profile = ProfileInfo[i].m_H264;
            m_optionsSame = false;
            return true;
          }
        }
        return false;
      }

      if (strcasecmp(optionName, Level.m_name) == 0) {
        for (size_t i = 0; i < sizeof(LevelInfo)/sizeof(LevelInfo[0]); i++) {
          if (strcasecmp(optionValue, LevelInfo[i].m_Name) == 0) {
            m_level = LevelInfo[i].m_H264;
            m_optionsSame = false;
            return true;
          }
        }
        return false;
      }

      if (
#ifdef PLUGIN_CODEC_VERSION_INTERSECT
          strcasecmp(optionName, PLUGINCODEC_MEDIA_PACKETIZATIONS) == 0 ||
#endif
          strcasecmp(optionName, PLUGINCODEC_MEDIA_PACKETIZATION ) == 0) {
        if (strstr(optionValue, OpalPluginCodec_Identifer_H264_Interleaved) != NULL) {
          m_packetisationModeH323 = 2;
          m_optionsSame = false;
          return true;
        }

        if (strstr(optionValue, OpalPluginCodec_Identifer_H264_NonInterleaved) != NULL) {
          m_packetisationModeH323 = 1;
          m_optionsSame = false;
          return true;
        }

        if (*optionValue != '\0' && strstr(optionValue, OpalPluginCodec_Identifer_H264_Aligned) == NULL)
          PTRACE(2, MY_CODEC_LOG, "Unknown packetisation mode: \"" << optionValue << '"');

        m_packetisationModeH323 = 0;
        m_optionsSame = false;
        return true;
      }

      if (strcasecmp(optionName, PacketizationModeName) == 0) {
        m_packetisationModeSDP = atoi(optionValue);
        m_optionsSame = false;
        return true;
      }

      if (strcasecmp(optionName, "Protocol") == 0) {
        m_isH323 = strstr(optionValue, "323") != NULL;
        return true;
      }

      // Base class sets bit rate and frame time
      return BaseClass::SetOption(optionName, optionValue);
    }


    virtual bool OnChangedOptions()
    {
      /* After all the options are set via SetOptions() this is called if any
         of them changed. This would do whatever is needed to set parmaeters
         for the actual codec. */

      // do some checks on limits
      size_t levelIndex = sizeof(LevelInfo)/sizeof(LevelInfo[0]);
      while (--levelIndex > 0) {
        if (m_level == LevelInfo[levelIndex].m_H264)
          break;
      }

      unsigned minFrameTime = PLUGINCODEC_VIDEO_CLOCK*GetMacroBlocks(m_width, m_height)/std::max(LevelInfo[levelIndex].m_MaxMBPS, m_maxMBPS);
      if (m_frameTime < minFrameTime) {
        PTRACE(3, MY_CODEC_LOG, "Selected resolution " << m_width << 'x' << m_height
               << " lowering frame rate from " << (PLUGINCODEC_VIDEO_CLOCK/m_frameTime) << " to " << (PLUGINCODEC_VIDEO_CLOCK/minFrameTime));
        m_frameTime = minFrameTime;
      }
      else
        PTRACE(4, MY_CODEC_LOG, "Selected resolution " << m_width << 'x' << m_height
               << " unchanged frame rate " << (PLUGINCODEC_VIDEO_CLOCK/m_frameTime) << " <= " << (PLUGINCODEC_VIDEO_CLOCK/minFrameTime));

      m_encoder.SetProfileLevel(m_profile, m_level, m_constraints);
      m_encoder.SetFrameWidth(m_width);
      m_encoder.SetFrameHeight(m_height);
      m_encoder.SetFrameRate(PLUGINCODEC_VIDEO_CLOCK/m_frameTime);
      m_encoder.SetTargetBitrate(m_maxBitRate/1000);
      m_encoder.SetTSTO(m_tsto);
      m_encoder.SetMaxKeyFramePeriod(m_keyFramePeriod != 0 ? m_keyFramePeriod : 10*PLUGINCODEC_VIDEO_CLOCK/m_frameTime); // Every 10 seconds

      unsigned mode = m_isH323 ? m_packetisationModeH323 : m_packetisationModeSDP;
      if (mode == 0) {
        unsigned size = std::min(m_maxRTPSize, m_maxNALUSize);
        m_encoder.SetMaxRTPPayloadSize(size);
        m_encoder.SetMaxNALUSize(size);
      }
      else {
        m_encoder.SetMaxRTPPayloadSize(m_maxRTPSize);
        m_encoder.SetMaxNALUSize(m_maxNALUSize);
      }

      m_encoder.ApplyOptions();

      PTRACE(3, MY_CODEC_LOG, "Applied options: "
                              "prof=" << m_profile << " "
                              "lev=" << m_level << " "
                              "res=" << m_width << 'x' << m_height << " "
                              "fps=" << (PLUGINCODEC_VIDEO_CLOCK/m_frameTime) << " "
                              "bps=" << m_maxBitRate << " "
                              "RTP=" << m_maxRTPSize << " "
                              "NALU=" << m_maxNALUSize << " "
                              "TSTO=" << m_tsto << " "
                              "Mode=" << mode);
      return true;
    }


    /// Get options that are "active" and may be different from the last SetOptions() call.
    virtual bool GetActiveOptions(PluginCodec_OptionMap & options)
    {
      options.SetUnsigned(m_frameTime, PLUGINCODEC_OPTION_FRAME_TIME);
      return true;
    }


    virtual bool Transcode(const void * fromPtr,
                             unsigned & fromLen,
                                 void * toPtr,
                             unsigned & toLen,
                             unsigned & flags)
    {
      return m_encoder.EncodeFrames((const unsigned char *)fromPtr, fromLen,
                                    (unsigned char *)toPtr, toLen,
                                     PluginCodec_RTP_GetHeaderLength(toPtr),
                                     flags);
    }
};


///////////////////////////////////////////////////////////////////////////////

class H264_Decoder : public PluginVideoDecoder<MY_CODEC>, public FFMPEGCodec
{
    typedef PluginVideoDecoder<MY_CODEC> BaseClass;

  public:
    H264_Decoder(const PluginCodec_Definition * defn)
      : BaseClass(defn)
      , FFMPEGCodec(MY_CODEC_LOG, new H264Frame)
    {
      PTRACE(4, MY_CODEC_LOG, "Created decoder $Revision$");
    }


    virtual bool Construct()
    {
      if (!InitDecoder(CODEC_ID_H264))
        return false;

      m_context->idct_algo = FF_IDCT_H264;
      m_context->flags = CODEC_FLAG_INPUT_PRESERVED | CODEC_FLAG_EMU_EDGE;
      m_context->flags2 = CODEC_FLAG2_BRDO |
                          CODEC_FLAG2_MEMC_ONLY |
                          CODEC_FLAG2_DROP_FRAME_TIMECODE |
                          CODEC_FLAG2_SKIP_RD |
                          CODEC_FLAG2_CHUNKS;

      if (!OpenCodec())
        return false;

      return true;
    }


    virtual bool Transcode(const void * fromPtr,
                             unsigned & fromLen,
                                 void * toPtr,
                             unsigned & toLen,
                             unsigned & flags)
    {
      if (!DecodeVideoPacket(PluginCodec_RTP(fromPtr, fromLen), flags))
        return false;

      if ((flags&PluginCodec_ReturnCoderLastFrame) == 0)
        return true;

      PluginCodec_RTP out(toPtr, toLen);
      toLen = OutputImage(m_picture->data, m_picture->linesize, m_context->width, m_context->height, out, flags);

      return true;
    }


    bool DecodeVideoFrame(const uint8_t * frame, size_t length, unsigned & flags)
    {
      if (((H264Frame *)m_fullFrame)->GetProfile() == H264_PROFILE_INT_BASELINE && m_context->has_b_frames > 0) {
        PTRACE(5, MY_CODEC_LOG, "Resetting B-Frame count to zero as Baseline profile");
        m_context->has_b_frames = 0;
      }

      return FFMPEGCodec::DecodeVideoFrame(frame, length, flags);
    }
};


///////////////////////////////////////////////////////////////////////////////

static struct PluginCodec_Definition CodecDefinition[] =
{
  PLUGINCODEC_VIDEO_CODEC_CXX(MyMediaFormatInfo_0, H264_Encoder, H264_Decoder),
  PLUGINCODEC_VIDEO_CODEC_CXX(MyMediaFormatInfo_1, H264_Encoder, H264_Decoder)
};


PLUGIN_CODEC_IMPLEMENT_CXX(MY_CODEC, CodecDefinition);


/////////////////////////////////////////////////////////////////////////////
