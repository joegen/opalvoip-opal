/*
 * OpenH264 Plugin codec for OPAL
 *
 * Copyright (C) 2014 Vox Lucida Pty Ltd, All Rights Reserved
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
 */

#include "../common/platform.h"

#define MY_CODEC openH264  // Name of codec (use C variable characters)

#define OPAL_H323 1
#define OPAL_SIP 1

#include "../../../src/codec/h264mf_inc.cxx"
#include "../common/h264frame.h"
#include "svc/codec_api.h"
#include "svc/codec_ver.h"

#include <iomanip>



///////////////////////////////////////////////////////////////////////////////

class MY_CODEC { };

PLUGINCODEC_CONTROL_LOG_FUNCTION_DEF

static const char MyDescription[] = "ITU-T H.264 - Video Codec (OpenH264)";     // Human readable description of codec

PLUGINCODEC_LICENSE(
  "Robert Jongbloed, Vox Lucida Pty.Ltd.",                      // source code author
  "1.0",                                                        // source code version
  "robertj@voxlucida.com.au",                                   // source code email
  "http://www.voxlucida.com.au",                                // source code URL
  "Copyright (C) 2011 by Vox Lucida Pt.Ltd., All Rights Reserved", // source code copyright
  "MPL 1.0",                                                    // source code license
  PluginCodec_License_MPL,                                      // source code license
  
  MyDescription,                                                // codec description
  "Cisco Systems",                                              // codec author
  g_strCodecVer, 						// codec version
  "",                                                           // codec email
  "http://www.openh264.org", 				        // codec URL
  "Copyright (c) 2013, Cisco Systems. All rights reserved.",    // codec copyright information
  "BSD",                                                        // codec license
  PluginCodec_License_BSD                                       // codec license code
);


///////////////////////////////////////////////////////////////////////////////

static struct PluginCodec_Option const Profile =
{
  PluginCodec_EnumOption,             // Option type
  ProfileName,                        // User visible name
  false,                              // User Read/Only flag
  PluginCodec_EqualMerge,             // Merge mode
  DefaultProfileStr,                  // Initial value
  NULL,                               // FMTP option name
  NULL,                               // FMTP default value
  0,                                  // H.245 generic capability code and bit mask
  // Enum values, single string of value separated by colons
  H264_PROFILE_STR_BASELINE ":"
  H264_PROFILE_STR_MAIN     ":"
  H264_PROFILE_STR_EXTENDED ":"
  H264_PROFILE_STR_HIGH
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

static struct PluginCodec_Option const ConstraintFlags =
{
  PluginCodec_IntegerOption,          // Option type
  ConstraintFlagsName,                // User visible name
  true,                               // User Read/Only flag
  PluginCodec_AndMerge,               // Merge mode
  "0",                                // Initial value
  NULL,                               // FMTP option name
  "0",                                // FMTP default value
  0,                                  // H.245 generic capability code and bit mask
  "0",                                // Minimum value
  "255"                               // Maximum value
};

#if HAS_HI_PROFILE
static struct PluginCodec_Option const HiProfile =
{
  PluginCodec_EnumOption,             // Option type
  ProfileName,                        // User visible name
  false,                              // User Read/Only flag
  PluginCodec_EqualMerge,             // Merge mode
  H264_PROFILE_STR_HIGH,              // Initial value
  NULL,                               // FMTP option name
  NULL,                               // FMTP default value
  0,                                  // H.245 generic capability code and bit mask
  // Enum values, single string of value separated by colons
  H264_PROFILE_STR_HIGH
};                                  

static struct PluginCodec_Option const HiH241Profiles =
{
  PluginCodec_IntegerOption,          // Option type
  H241ProfilesName,                   // User visible name
  false,                              // User Read/Only flag
  PluginCodec_EqualMerge,             // Merge mode
  STRINGIZE(8),                       // Initial value
  NULL,                               // FMTP option name
  NULL,                               // FMTP default value
  H241_PROFILES,                      // H.245 generic capability code and bit mask
  "1",                                // Minimum value
  "127"                               // Maximum value
};
#endif

static struct PluginCodec_Option const H241Profiles =
{
  PluginCodec_IntegerOption,          // Option type
  H241ProfilesName,                   // User visible name
  false,                              // User Read/Only flag
  PluginCodec_EqualMerge,             // Merge mode
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
  false,                              // User Read/Only flag
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
  false,                              // User Read/Only flag
  PluginCodec_NoMerge,                // Merge mode
  DefaultSDPProfileAndLevel,          // Initial value
  SDPProfileAndLevelFMTPName,         // FMTP option name
  SDPProfileAndLevelFMTPDflt          // FMTP default value (as per RFC)
};

static struct PluginCodec_Option const MaxMBPS_SDP =
{
  PluginCodec_IntegerOption,          // Option type
  MaxMBPS_SDP_Name,                   // User visible name
  false,                              // User Read/Only flag
  PluginCodec_MinMerge,               // Merge mode
  STRINGIZE(MAX_MBPS_SDP),            // Initial value
  MaxMBPS_FMTPName,                   // FMTP option name
  STRINGIZE(MAX_MBPS_SDP),            // FMTP default value
  0,                                  // H.245 generic capability code and bit mask
  "0",                                // Minimum value
  STRINGIZE(MAX_MBPS_SDP)             // Maximum value
};

static struct PluginCodec_Option const MaxMBPS_H241 =
{
  PluginCodec_IntegerOption,          // Option type
  MaxMBPS_H241_Name,                  // User visible name
  false,                              // User Read/Only flag
  PluginCodec_MinMerge,               // Merge mode
  STRINGIZE(MAX_MBPS_H241),           // Initial value
  NULL,                               // FMTP option name
  NULL,                               // FMTP default value
  H241_CustomMaxMBPS,                 // H.245 generic capability code and bit mask
  "0",                                // Minimum value
  STRINGIZE(MAX_MBPS_H241)            // Maximum value
#ifdef PLUGIN_CODEC_VERSION_H245_DEF_GEN_PARAM
  ,
  NULL,
  NULL,
  STRINGIZE(MAX_MBPS_H241)            // H.245 default value
#endif
};

static struct PluginCodec_Option const MaxSMBPS_SDP =
{
  PluginCodec_IntegerOption,          // Option type
  MaxSMBPS_SDP_Name,                  // User visible name
  false,                              // User Read/Only flag
  PluginCodec_MinMerge,               // Merge mode
  STRINGIZE(MAX_MBPS_SDP),            // Initial value
  MaxSMBPS_FMTPName,                  // FMTP option name
  STRINGIZE(MAX_MBPS_SDP),            // FMTP default value
  0,                                  // H.245 generic capability code and bit mask
  "0",                                // Minimum value
 STRINGIZE(MAX_MBPS_SDP)              // Maximum value
};

static struct PluginCodec_Option const MaxSMBPS_H241 =
{
  PluginCodec_IntegerOption,          // Option type
  MaxSMBPS_H241_Name,                 // User visible name
  false,                              // User Read/Only flag
  PluginCodec_MinMerge,               // Merge mode
  STRINGIZE(MAX_MBPS_H241),           // Initial value
  NULL,                               // FMTP option name
  NULL,                               // FMTP default value
  H241_CustomMaxSMBPS,                // H.245 generic capability code and bit mask
  "0",                                // Minimum value
  STRINGIZE(MAX_MBPS_H241)            // Maximum value
#ifdef PLUGIN_CODEC_VERSION_H245_DEF_GEN_PARAM
  ,
  NULL,
  NULL,
  STRINGIZE(MAX_MBPS_H241)            // H.245 default value
#endif
};

static struct PluginCodec_Option const MaxFS_SDP =
{
  PluginCodec_IntegerOption,          // Option type
  MaxFS_SDP_Name,                     // User visible name
  false,                              // User Read/Only flag
  PluginCodec_MinMerge,               // Merge mode
  STRINGIZE(MAX_FS_SDP),              // Initial value
  MaxFS_FMTPName,                     // FMTP option name
  STRINGIZE(MAX_FS_SDP),              // FMTP default value
  0,                                  // H.245 generic capability code and bit mask
  "0",                                // Minimum value
  STRINGIZE(MAX_FS_SDP)               // Maximum value
};

static struct PluginCodec_Option const MaxFS_H241 =
{
  PluginCodec_IntegerOption,          // Option type
  MaxFS_H241_Name,                    // User visible name
  false,                              // User Read/Only flag
  PluginCodec_MinMerge,               // Merge mode
  STRINGIZE(MAX_FS_H241),             // Initial value
  NULL,                               // FMTP option name
  NULL,                               // FMTP default value
  H241_CustomMaxFS,                   // H.245 generic capability code and bit mask
  "0",                                // Minimum value
  STRINGIZE(MAX_FS_H241)              // Maximum value
#ifdef PLUGIN_CODEC_VERSION_H245_DEF_GEN_PARAM
  ,
  NULL,
  NULL,
  STRINGIZE(MAX_FS_H241)              // H.245 default value
#endif
};

static struct PluginCodec_Option const MaxBR_SDP =
{
  PluginCodec_IntegerOption,          // Option type
  MaxBR_SDP_Name,                     // User visible name
  false,                              // User Read/Only flag
  PluginCodec_MinMerge,               // Merge mode
  STRINGIZE(MAX_BR_SDP),              // Initial value
  MaxBR_FMTPName,                     // FMTP option name
  STRINGIZE(MAX_BR_SDP),              // FMTP default value
  0,                                  // H.245 generic capability code and bit mask
  "0",                                // Minimum value
  STRINGIZE(MAX_BR_SDP)               // Maximum value
};

static struct PluginCodec_Option const MaxBR_H241 =
{
  PluginCodec_IntegerOption,          // Option type
  MaxBR_H241_Name,                    // User visible name
  false,                              // User Read/Only flag
  PluginCodec_MinMerge,               // Merge mode
  STRINGIZE(MAX_BR_H241),             // Initial value
  NULL,                               // FMTP option name
  NULL,                               // FMTP default value
  H241_CustomMaxBRandCPB,             // H.245 generic capability code and bit mask
  "0",                                // Minimum value
  STRINGIZE(MAX_BR_H241)              // Maximum value
#ifdef PLUGIN_CODEC_VERSION_H245_DEF_GEN_PARAM
  ,
  NULL,
  NULL,
  STRINGIZE(MAX_BR_H241)              // H.245 default value
#endif
};

static struct PluginCodec_Option const H241Forced =
{
  PluginCodec_BoolOption,             // Option type
  H241ForcedName,                     // User visible name
  false,                              // User Read/Only flag
  PluginCodec_AlwaysMerge             // Merge mode
};

static struct PluginCodec_Option const SDPForced =
{
  PluginCodec_BoolOption,             // Option type
  SDPForcedName,                      // User visible name
  false,                              // User Read/Only flag
  PluginCodec_AlwaysMerge             // Merge mode
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
  OpalPluginCodec_Identifer_H264_NonInterleaved "," // Initial value
  OpalPluginCodec_Identifer_H264_Aligned
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

static struct PluginCodec_Option const * const MyOptionTable_0[] = {
  &Profile,
  &Level,
  &ConstraintFlags,
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
  &H241Forced,
  &SDPForced,
  &MaxNaluSize,
  &PacketizationModeSDP_0,
  &MediaPacketizationsH323_0,  // Note: must be last entry
  NULL
};

static struct PluginCodec_Option const * const MyOptionTable_1[] = {
  &Profile,
  &Level,
  &ConstraintFlags,
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
  &H241Forced,
  &SDPForced,
  &MaxNaluSize,
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


  virtual bool IsValidForProtocol(const char * protocol) const
  {
    return strcasecmp(protocol, PLUGINCODEC_OPTION_PROTOCOL_SIP) == 0 || m_options != MyOptionTable_0;
  }


  virtual bool ToNormalised(OptionMap & original, OptionMap & changed) const
  {
    return MyToNormalised(original, changed);
  }


  virtual bool ToCustomised(OptionMap & original, OptionMap & changed) const
  {
    return MyToCustomised(original, changed);
  }
}; 

/* SIP requires two completely independent media formats for packetisation
   modes zero and one. */
static H264_PluginMediaFormat const MyMediaFormatInfo_Mode0(OPAL_H264_MODE0, MyOptionTable_0);
static H264_PluginMediaFormat const MyMediaFormatInfo_Mode1(OPAL_H264_MODE1, MyOptionTable_1);
static H264_PluginMediaFormat const MyMediaFormatInfo_Open0("Open" OPAL_H264_MODE0, MyOptionTable_0);
static H264_PluginMediaFormat const MyMediaFormatInfo_Open1("Open" OPAL_H264_MODE1, MyOptionTable_1);


///////////////////////////////////////////////////////////////////////////////

#if PLUGINCODEC_TRACING

static void TraceCallback(void*, int welsLevel, const char* string)
{
  unsigned ptraceLevel;
  if (welsLevel <= WELS_LOG_ERROR)
    ptraceLevel = 1;
  else if (welsLevel <= WELS_LOG_WARNING)
    ptraceLevel = 2;
  else if (welsLevel <= WELS_LOG_INFO)
    ptraceLevel = 4;
  else if (welsLevel <= WELS_LOG_DEBUG)
    ptraceLevel = 5;
  else
    ptraceLevel = 6;

  if (!PTRACE_CHECK(ptraceLevel))
    return;

  const char * msg;
  if (strncmp(string, "[OpenH264]", 10) == 0 && (msg = strchr(string, ':')) != NULL)
    ++msg;
  else
    msg = string;

  size_t len = strlen(msg);
  while (len > 0 && isspace(msg[len - 1]))
    --len;
  if (len == 0)
    return;

  char * buf = (char *)alloca(len+1);
  strncpy(buf, msg, len);
  buf[len] = '\0';
  PluginCodec_LogFunctionInstance(ptraceLevel, __FILE__, __LINE__, "OpenH264-Lib", buf);
}

static int TraceLevel = WELS_LOG_DETAIL;
static WelsTraceCallback TraceCallbackPtr = TraceCallback;

#endif // PLUGINCODEC_TRACING


static void CheckVersion(bool encoder)
{
  OpenH264Version version;
  WelsGetCodecVersionEx(&version);
  if (version.uMajor == OPENH264_MAJOR && version.uMinor == OPENH264_MINOR && version.uRevision == OPENH264_REVISION) {
    PTRACE(4, MY_CODEC_LOG, "Created " << (encoder ? "encoder" : "decoder")
           << ", plugin DLL/API version " << version.uMajor << '.' << version.uMinor << '.' << version.uRevision);
  }
  else {
    PTRACE(1, MY_CODEC_LOG, "WARNING! Created " << (encoder ? "encoder" : "decoder")
           << ", plugin with DLL version " << version.uMajor << '.' << version.uMinor << '.' << version.uRevision
           << " but API version " << OPENH264_MAJOR << '.' << OPENH264_MINOR << '.' << OPENH264_REVISION);
  }
}


///////////////////////////////////////////////////////////////////////////////

class H264_Encoder : public PluginVideoEncoder<MY_CODEC>
{
    typedef PluginVideoEncoder<MY_CODEC> BaseClass;

  protected:
    EProfileIdc m_profile;
    ELevelIdc   m_level;
    unsigned    m_constraints;
    unsigned    m_sdpMBPS;
    unsigned    m_h241MBPS;
    unsigned    m_maxNALUSize;
    unsigned    m_packetisationModeSDP;
    unsigned    m_packetisationModeH323;
    bool        m_isH323;

    ISVCEncoder * m_encoder;
    H264Frame     m_encapsulation;
    int           m_quality;

  public:
    H264_Encoder(const PluginCodec_Definition * defn)
      : BaseClass(defn)
      , m_profile((EProfileIdc)DefaultProfileInt)
      , m_level(LEVEL_3_1)
      , m_constraints(0)
      , m_sdpMBPS(MAX_MBPS_SDP)
      , m_h241MBPS(MAX_MBPS_H241)
      , m_maxNALUSize(H241_MAX_NALU_SIZE)
      , m_packetisationModeSDP(1)
      , m_packetisationModeH323(1)
      , m_isH323(false)
      , m_encoder(NULL)
      , m_quality(-1)
    {
      CheckVersion(true);
    }


    ~H264_Encoder()
    {
      if (m_encoder != NULL)
        WelsDestroySVCEncoder(m_encoder);
    }

    virtual bool Construct()
    {
      if (WelsCreateSVCEncoder(&m_encoder) != cmResultSuccess) {
        PTRACE(1, MY_CODEC_LOG, "Could not create encoder.");
        return false;
      }

#if PLUGINCODEC_TRACING
      m_encoder->SetOption(ENCODER_OPTION_TRACE_CALLBACK, &TraceCallbackPtr);
      m_encoder->SetOption(ENCODER_OPTION_TRACE_LEVEL, &TraceLevel);
#endif
      return true;
    }


    virtual bool SetOption(const char * optionName, const char * optionValue)
    {
      if (strcasecmp(optionName, MaxNaluSize.m_name) == 0)
        return SetOptionUnsigned(m_maxNALUSize, optionValue, 256, 8192);

      if (strcasecmp(optionName, MaxMBPS_H241.m_name) == 0)
        return SetOptionUnsigned(m_h241MBPS, optionValue, 0);

      if (strcasecmp(optionName, MaxMBPS_SDP.m_name) == 0)
        return SetOptionUnsigned(m_sdpMBPS, optionValue, 0);

      if (strcasecmp(optionName, Profile.m_name) == 0) {
        for (size_t i = 0; i < sizeof(ProfileInfo)/sizeof(ProfileInfo[0]); ++i) {
          if (strcasecmp(optionValue, ProfileInfo[i].m_Name) == 0) {
            m_profile = (EProfileIdc)ProfileInfo[i].m_H264;
            m_optionsSame = false;
            return true;
          }
        }
        return false;
      }

      if (strcasecmp(optionName, Level.m_name) == 0) {
        for (size_t i = 0; i < sizeof(LevelInfo)/sizeof(LevelInfo[0]); i++) {
          if (strcasecmp(optionValue, LevelInfo[i].m_Name) == 0) {
            m_level = (ELevelIdc)(i+1);
            m_optionsSame = false;
            return true;
          }
        }
        return false;
      }

      if (strcasecmp(optionName, ConstraintFlags.m_name) == 0)
        return SetOptionUnsigned(m_constraints, optionValue, 0, 255);

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

        if (*optionValue != '\0' && strstr(optionValue, OpalPluginCodec_Identifer_H264_Aligned) == NULL) {
          PTRACE(2, MY_CODEC_LOG, "Unknown packetisation mode: \"" << optionValue << '"');
        }

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
      m_encoder->Uninitialize();

      SEncParamExt param;
      m_encoder->GetDefaultParams(&param);
      param.iUsageType = CAMERA_VIDEO_REAL_TIME;
      param.iPicWidth = m_width;
      param.iPicHeight = m_height;
      param.iTargetBitrate = param.iMaxBitrate = m_maxBitRate;
      param.iRCMode = RC_BITRATE_MODE;
      param.fMaxFrameRate = (float)PLUGINCODEC_VIDEO_CLOCK/m_frameTime;
      param.uiIntraPeriod = m_keyFramePeriod;
      param.bPrefixNalAddingCtrl = false;

      param.sSpatialLayers[0].uiProfileIdc = m_profile;
      param.sSpatialLayers[0].uiLevelIdc = m_level;
      param.sSpatialLayers[0].iVideoWidth = m_width;
      param.sSpatialLayers[0].iVideoHeight = m_height;
      param.sSpatialLayers[0].fFrameRate = param.fMaxFrameRate;
      param.sSpatialLayers[0].iMaxSpatialBitrate = param.sSpatialLayers[0].iSpatialBitrate = m_maxBitRate;

      unsigned mode = m_isH323 ? m_packetisationModeH323 : m_packetisationModeSDP;
      switch (mode) {
        case 0 :
          param.sSpatialLayers[0].sSliceArgument.uiSliceMode = SM_SIZELIMITED_SLICE;
          param.sSpatialLayers[0].sSliceArgument.uiSliceSizeConstraint = param.uiMaxNalSize = m_maxNALUSize;
          break;

        case 1 :
          param.sSpatialLayers[0].sSliceArgument.uiSliceMode = SM_SINGLE_SLICE;
          break;

        default :
          PTRACE(1, MY_CODEC_LOG, "Unsupported packetisation mode: " << mode);
          return false;
      }

      m_encapsulation.SetPacketisationMode(mode);
      m_encapsulation.SetMaxPayloadSize(m_maxRTPSize);

      int err = m_encoder->InitializeExt(&param);
      switch (err) {
        case cmResultSuccess :
          PTRACE(4, MY_CODEC_LOG, "Initialised encoder: " << m_width <<'x' << m_height << '@' << param.fMaxFrameRate << ", "
                 << m_maxBitRate << "bps, ""NALU=" << m_maxNALUSize << ", profile=" << m_profile << ", level=" << m_level);
          return true;

        case cmInitParaError :
          PTRACE(1, MY_CODEC_LOG, "Invalid parameter to encoder:"
                    " profile=" << m_profile << " level=" << LevelInfo[m_level - 1].m_Name << '(' << m_level << ")"
                    " " << m_width << 'x' << m_height << '@' << std::fixed << std::setprecision(1) << param.fMaxFrameRate <<
                    " br=" << m_maxBitRate << " mode=" << mode);
          return false;

        default :
          PTRACE(1, MY_CODEC_LOG, "Could not initialise encoder: error=" << err);
          return false;
      }
    }


    /// Get options that are "active" and may be different from the last SetOptions() call.
    virtual bool GetActiveOptions(PluginCodec_OptionMap & options)
    {
      options.SetUnsigned(m_frameTime, PLUGINCODEC_OPTION_FRAME_TIME);
      return true;
    }


    virtual int GetStatistics(char * bufferPtr, unsigned bufferSize)
    {
      size_t len = BaseClass::GetStatistics(bufferPtr, bufferSize);
      len += snprintf(bufferPtr+len, bufferSize-len, "Width=%u\nHeight=%u\n", m_width, m_height);

      if (m_quality >= 0 && len < bufferSize)
        len += snprintf(bufferPtr+len, bufferSize-len, "Quality=%u\n", m_quality);

      return (int)len;
    }


    virtual bool Transcode(const void * fromPtr,
                             unsigned & fromLen,
                                 void * toPtr,
                             unsigned & toLen,
                             unsigned & flags)
    {
      bool forceIntraFrame = (flags&PluginCodec_CoderForceIFrame) != 0;
      flags = 0;

      if (!m_encapsulation.HasRTPFrames()) {
        PluginCodec_RTP from(fromPtr, fromLen);
        PluginCodec_Video_FrameHeader * header = from.GetVideoHeader();

        unsigned planeWidth = (header->width+1)&~1;
        unsigned planeHeight = (header->height+1)&~1;

        SSourcePicture picture;
        picture.iColorFormat = videoFormatI420;	// color space type
        picture.iPicWidth = m_width = header->width;
        picture.iPicHeight = m_height = header->height;
        picture.iStride[0] = planeWidth;
        picture.iStride[1] = picture.iStride[2] = planeWidth/2;
        picture.pData[0] = from.GetVideoFrameData();
        picture.pData[1] = picture.pData[0] + planeWidth*planeHeight;
        picture.pData[2] = picture.pData[1] + planeWidth*planeHeight/4;
        picture.uiTimeStamp = 0;

        if (forceIntraFrame)
          m_encoder->ForceIntraFrame(true);

        SFrameBSInfo  bitstream;
        memset(&bitstream, 0, sizeof(bitstream));
        if (m_encoder->EncodeFrame(&picture, &bitstream) != cmResultSuccess) {
          PTRACE(1, MY_CODEC_LOG, "Fatal error encoding frame.");
          return false;
        }

        switch (bitstream.eFrameType) {
          case videoFrameTypeInvalid :
            PTRACE(1, MY_CODEC_LOG, "Fatal error encoding frame.");
            return false;

          case videoFrameTypeSkip :
            PTRACE(5, MY_CODEC_LOG, "Output frame skipped.");
            toLen = 0;
            return true;

          case videoFrameTypeIDR :
          case videoFrameTypeI :
            flags |= PluginCodec_ReturnCoderIFrame;
          case videoFrameTypeP :
          case videoFrameTypeIPMixed :
            uint32_t numberOfNALUs = 0;
            for (int layer = 0; layer < bitstream.iLayerNum; ++layer) {
              for (int nalu = 0; nalu < bitstream.sLayerInfo[layer].iNalCount; ++nalu)
                ++numberOfNALUs;
            }

            m_encapsulation.Reset();
            m_encapsulation.Allocate(numberOfNALUs);
            m_encapsulation.SetTimestamp(from.GetTimestamp());

            m_quality = -1;
            for (int layer = 0; layer < bitstream.iLayerNum; ++layer) {
              for (int nalu = 0; nalu < bitstream.sLayerInfo[layer].iNalCount; ++nalu) {
                size_t len = bitstream.sLayerInfo[layer].pNalLengthInByte[nalu];
                m_encapsulation.AddNALU(bitstream.sLayerInfo[layer].pBsBuf[4], len, bitstream.sLayerInfo[layer].pBsBuf);
                bitstream.sLayerInfo[layer].pBsBuf += len;
                if (bitstream.sLayerInfo[layer].uiQualityId > 0 && m_quality < bitstream.sLayerInfo[layer].uiQualityId)
                  m_quality = bitstream.sLayerInfo[layer].uiQualityId;
              }
            }
        }
      }

      // create RTP frame from destination buffer
      PluginCodec_RTP to(toPtr, toLen);
      if (!m_encapsulation.GetPacket(to, flags))
        return false;

      toLen = (unsigned)to.GetPacketSize();
      return true;
    }
};


///////////////////////////////////////////////////////////////////////////////

class H264_Decoder : public PluginVideoDecoder<MY_CODEC>
{
    typedef PluginVideoDecoder<MY_CODEC> BaseClass;

  protected:
    ISVCDecoder * m_decoder;
    SBufferInfo   m_bufferInfo;
    uint8_t     * m_bufferData[3];
    int           m_lastId;
    H264Frame     m_encapsulation;

  public:
    H264_Decoder(const PluginCodec_Definition * defn)
      : BaseClass(defn)
      , m_decoder(NULL)
      , m_lastId(-1)
    {
      memset(&m_bufferInfo, 0, sizeof(m_bufferInfo));
      memset(&m_bufferData, 0, sizeof(m_bufferData));
      CheckVersion(false);
    }


    ~H264_Decoder()
    {
      if (m_decoder != NULL)
        WelsDestroyDecoder(m_decoder);
    }


    virtual bool Construct()
    {
      if (WelsCreateDecoder(&m_decoder) != cmResultSuccess) {
        PTRACE(1, MY_CODEC_LOG, "Could not create decoder.");
        return false;
      }

#if PLUGINCODEC_TRACING
      m_decoder->SetOption(DECODER_OPTION_TRACE_CALLBACK, &TraceCallbackPtr);
      m_decoder->SetOption(DECODER_OPTION_TRACE_LEVEL, &TraceLevel);
#endif

      SDecodingParam param;
      memset(&param, 0, sizeof(param));
      param.eEcActiveIdc = ERROR_CON_DISABLE;		// Whether active error concealment feature in decoder
      param.sVideoProperty.size = sizeof(param.sVideoProperty);
      param.sVideoProperty.eVideoBsType = VIDEO_BITSTREAM_DEFAULT;

      int err = m_decoder->Initialize(&param);
      if (err != cmResultSuccess) {
        PTRACE(1, MY_CODEC_LOG, "Could not initialise decoder: error=" << err);
        return false;
      }

      PTRACE(4, MY_CODEC_LOG, "Opened decoder.");
      return true;
    }


    virtual int GetStatistics(char * bufferPtr, unsigned bufferSize)
    {
      size_t len = BaseClass::GetStatistics(bufferPtr, bufferSize);
      len += snprintf(bufferPtr+len, bufferSize-len, "Width=%u\nHeight=%u\n", m_width, m_height);
      return (int)len;
    }


    virtual bool Transcode(const void * fromPtr,
                             unsigned & fromLen,
                                 void * toPtr,
                             unsigned & toLen,
                             unsigned & flags)
    {
      PluginCodec_RTP from(fromPtr, fromLen);

      if (from.GetPayloadSize() > 0 && !m_encapsulation.AddPacket(from, flags))
        return false;

      if (!from.GetMarker())
        return true;

      if (m_encapsulation.GetLength() > 0) {
        DECODING_STATE status = m_decoder->DecodeFrame2(m_encapsulation.GetBuffer(),
                                                        (int)m_encapsulation.GetLength(),
                                                        m_bufferData,
                                                        &m_bufferInfo);

        if (status != dsErrorFree) {
          if (status >= dsInvalidArgument) {
            PTRACE(1, MY_CODEC_LOG, "Fatal error decoding frame: status=0x" << std::hex << status);
            return false;
          }

          if (status & dsRefLost) {
            PTRACE(5, MY_CODEC_LOG, "Reference frame lost");
          }
          if (status & dsBitstreamError) {
            PTRACE(3, MY_CODEC_LOG, "Bit stream error decoding frame");
          }
          if (status & dsDepLayerLost) {
            PTRACE(5, MY_CODEC_LOG, "Dependent layer lost");
          }
          if (status & dsNoParamSets) {
            PTRACE(3, MY_CODEC_LOG, "No parameter sets received");
          }
          if (status & dsDataErrorConcealed) {
            PTRACE(4, MY_CODEC_LOG, "Data error concealed");
          }
          if (status >= dsDataErrorConcealed*2) {
            PTRACE(4, MY_CODEC_LOG, "Unknown error: status=0x" << std::hex << status);
          }
          if (status != dsDataErrorConcealed)
            flags = PluginCodec_ReturnCoderRequestIFrame;
        }

        m_encapsulation.Reset();
      }

      if (m_bufferInfo.iBufferStatus != 0) {
        PluginCodec_RTP out(toPtr, toLen);

        m_width = m_bufferInfo.UsrData.sSystemBuffer.iWidth;
        m_height = m_bufferInfo.UsrData.sSystemBuffer.iHeight;

        // Why make it so hard?
        int raster[3] = {
          m_bufferInfo.UsrData.sSystemBuffer.iStride[0],
          m_bufferInfo.UsrData.sSystemBuffer.iStride[1],
          m_bufferInfo.UsrData.sSystemBuffer.iStride[1]
        };
        toLen = OutputImage(m_bufferData,
                            raster,
                            m_bufferInfo.UsrData.sSystemBuffer.iWidth,
                            m_bufferInfo.UsrData.sSystemBuffer.iHeight,
                            out,
                            flags);

        if ((flags & PluginCodec_ReturnCoderBufferTooSmall) == 0) {
          int id = 0;
          if (m_decoder->GetOption(DECODER_OPTION_IDR_PIC_ID, &id) == cmResultSuccess && m_lastId != id) {
            m_lastId = id;
            flags |= PluginCodec_ReturnCoderIFrame;
          }

          m_bufferInfo.iBufferStatus = 0;
        }
      }

      return true;
    }
};


///////////////////////////////////////////////////////////////////////////////

static struct PluginCodec_Definition CodecDefinition[] =
{
  PLUGINCODEC_VIDEO_CODEC_CXX(MyMediaFormatInfo_Mode0, H264_Encoder, H264_Decoder),
  PLUGINCODEC_VIDEO_CODEC_CXX(MyMediaFormatInfo_Mode1, H264_Encoder, H264_Decoder),
  PLUGINCODEC_VIDEO_CODEC_CXX(MyMediaFormatInfo_Open0, H264_Encoder, H264_Decoder),
  PLUGINCODEC_VIDEO_CODEC_CXX(MyMediaFormatInfo_Open1, H264_Encoder, H264_Decoder),
};


PLUGIN_CODEC_IMPLEMENT_CXX(MY_CODEC, CodecDefinition);


/////////////////////////////////////////////////////////////////////////////
