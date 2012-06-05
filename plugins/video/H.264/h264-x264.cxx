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

#include <codec/opalplugin.hpp>

#include "../common/ffmpeg.h"
#include "../common/dyna.h"

#include "shared/h264frame.h"
#include "shared/x264wrap.h"


#define MY_CODEC      x264                                  // Name of codec (use C variable characters)
#define MY_CODEC_LOG "x264"

class MY_CODEC { };

#if defined(PLUGIN_CODEC_VERSION_H245_DEF_GEN_PARAM)
static unsigned MyVersion = PLUGIN_CODEC_VERSION_H245_DEF_GEN_PARAM;
#elif defined(PLUGIN_CODEC_VERSION_INTERSECT)
static unsigned MyVersion = PLUGIN_CODEC_VERSION_INTERSECT;
#else
static unsigned MyVersion = PLUGIN_CODEC_VERSION_OPTIONS;
#endif

static const char MyDescription[] = "x264 Video Codec";     // Human readable description of codec
static const char FormatNameH323[] = "H.264";               // OpalMediaFormat name string to generate
static const char FormatNameSIP0[] = "H.264-0";             // OpalMediaFormat name string to generate
static const char FormatNameSIP1[] = "H.264-1";             // OpalMediaFormat name string to generate
static const char MyPayloadName[] = "H264";                 // RTP payload name (IANA approved)
static unsigned   MyClockRate = 90000;                      // RTP dictates 90000
static unsigned   MyMaxFrameRate = 60;                      // Maximum frame rate (per second)
static unsigned   MyMaxWidth = 2816;                        // Maximum width of frame
static unsigned   MyMaxHeight = 2304;                       // Maximum height of frame

static const char YUV420PFormatName[] = "YUV420P";          // Raw media format


static struct PluginCodec_information LicenseInfo = {
  1143692893,                                                   // timestamp = Thu 30 Mar 2006 04:28:13 AM UTC

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
};


///////////////////////////////////////////////////////////////////////////////

FFMPEGLibrary FFMPEGLibraryInstance(CODEC_ID_H264);

PLUGINCODEC_CONTROL_LOG_FUNCTION_DEF

///////////////////////////////////////////////////////////////////////////////

// Level 3 allows 4CIF at 25fps
#define DefaultProfileStr          H264_PROFILE_STR_BASELINE
#define DefaultProfileInt          H264_PROFILE_INT_BASELINE
#define DefaultProfileH241         64
#define DefaultLevelStr            H264_LEVEL_STR_3
#define DefaultLevelInt            30
#define DefaultLevelH241           64
#define DefaultSDPProfileAndLevel  "42801e"

#define DefaultSendAccessUnitDelimiters 0  // Many endpoints don't seem to like these, initially false

#define H241_MAX_NALU_SIZE   1400   // From H.241/8.3.2.10

#define H264_PROFILE_STR_BASELINE  "Baseline"
#define H264_PROFILE_STR_MAIN      "Main"
#define H264_PROFILE_STR_EXTENDED  "Extended"

#define H264_PROFILE_INT_BASELINE  66
#define H264_PROFILE_INT_MAIN      77
#define H264_PROFILE_INT_EXTENDED  88

#define H264_LEVEL_STR_1    "1"
#define H264_LEVEL_STR_1_b  "1.b"
#define H264_LEVEL_STR_1_1  "1.1"
#define H264_LEVEL_STR_1_2  "1.2"
#define H264_LEVEL_STR_1_3  "1.3"
#define H264_LEVEL_STR_2    "2"
#define H264_LEVEL_STR_2_1  "2.1"
#define H264_LEVEL_STR_2_2  "2.2"
#define H264_LEVEL_STR_3    "3"
#define H264_LEVEL_STR_3_1  "3.1"
#define H264_LEVEL_STR_3_2  "3.2"
#define H264_LEVEL_STR_4    "4"
#define H264_LEVEL_STR_4_1  "4.1"
#define H264_LEVEL_STR_4_2  "4.2"
#define H264_LEVEL_STR_5    "5"
#define H264_LEVEL_STR_5_1  "5.1"


enum
{
    H241_PROFILES                      = 41 | PluginCodec_H245_Collapsing | PluginCodec_H245_TCS | PluginCodec_H245_OLC | PluginCodec_H245_ReqMode | PluginCodec_H245_BooleanArray | (1 << PluginCodec_H245_PositionShift),
    H241_LEVEL                         = 42 | PluginCodec_H245_Collapsing | PluginCodec_H245_TCS | PluginCodec_H245_OLC | PluginCodec_H245_ReqMode                                 | (2 << PluginCodec_H245_PositionShift),
    H241_CustomMaxMBPS                 =  3 | PluginCodec_H245_Collapsing | PluginCodec_H245_TCS | PluginCodec_H245_OLC | PluginCodec_H245_ReqMode,
    H241_CustomMaxFS                   =  4 | PluginCodec_H245_Collapsing | PluginCodec_H245_TCS | PluginCodec_H245_OLC | PluginCodec_H245_ReqMode,
    H241_CustomMaxDPB                  =  5 | PluginCodec_H245_Collapsing | PluginCodec_H245_TCS | PluginCodec_H245_OLC | PluginCodec_H245_ReqMode,
    H241_CustomMaxBRandCPB             =  6 | PluginCodec_H245_Collapsing | PluginCodec_H245_TCS | PluginCodec_H245_OLC | PluginCodec_H245_ReqMode,
    H241_MaxStaticMBPS                 =  7 | PluginCodec_H245_Collapsing | PluginCodec_H245_TCS | PluginCodec_H245_OLC | PluginCodec_H245_ReqMode,
    H241_Max_RCMD_NALU_size            =  8 | PluginCodec_H245_Collapsing | PluginCodec_H245_TCS | PluginCodec_H245_OLC | PluginCodec_H245_ReqMode,
    H241_Max_NAL_unit_size             =  9 | PluginCodec_H245_Collapsing | PluginCodec_H245_TCS | PluginCodec_H245_OLC | PluginCodec_H245_ReqMode,
    H241_SampleAspectRatiosSupported   = 10 | PluginCodec_H245_Collapsing | PluginCodec_H245_TCS | PluginCodec_H245_OLC | PluginCodec_H245_ReqMode,
    H241_AdditionalModesSupported      = 11 | PluginCodec_H245_Collapsing | PluginCodec_H245_TCS | PluginCodec_H245_OLC | PluginCodec_H245_ReqMode | PluginCodec_H245_BooleanArray,
    H241_AdditionalDisplayCapabilities = 12 | PluginCodec_H245_Collapsing | PluginCodec_H245_TCS | PluginCodec_H245_OLC | PluginCodec_H245_ReqMode | PluginCodec_H245_BooleanArray,
};

static struct PluginCodec_Option const Profile =
{
  PluginCodec_EnumOption,             // Option type
  "Profile",                          // User visible name
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
  "Level",                            // User visible name
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
  "H.241 Profile Mask",               // User visible name
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
  "H.241 Level",                      // User visible name
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
  "SIP/SDP Profile & Level",          // User visible name
  true,                               // User Read/Only flag
  PluginCodec_NoMerge,                // Merge mode
  DefaultSDPProfileAndLevel,          // Initial value
  "profile-level-id",                 // FMTP option name
  "42800A"                            // FMTP default value (as per RFC)
};

static struct PluginCodec_Option const MaxMBPS_SDP =
{
  PluginCodec_IntegerOption,          // Option type
  "SIP/SDP Max MBPS",                 // User visible name
  true,                               // User Read/Only flag
  PluginCodec_MinMerge,               // Merge mode
  "0",                                // Initial value
  "max-mbps",                         // FMTP option name
  "0",                                // FMTP default value
  0,                                  // H.245 generic capability code and bit mask
  "0",                                // Minimum value
  "983040"                            // Maximum value
};

static struct PluginCodec_Option const MaxMBPS_H241 =
{
  PluginCodec_IntegerOption,          // Option type
  "H.241 Max MBPS",                   // User visible name
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

static struct PluginCodec_Option const MaxFS_SDP =
{
  PluginCodec_IntegerOption,          // Option type
  "SIP/SDP Max FS",                   // User visible name
  true,                               // User Read/Only flag
  PluginCodec_MinMerge,               // Merge mode
  "0",                                // Initial value
  "max-fs",                           // FMTP option name
  "0",                                // FMTP default value
  0,                                  // H.245 generic capability code and bit mask
  "0",                                // Minimum value
  "36864"                             // Maximum value
};

static struct PluginCodec_Option const MaxFS_H241 =
{
  PluginCodec_IntegerOption,          // Option type
  "H.241 Max FS",                     // User visible name
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
  "SIP/SDP Max BR",                   // User visible name
  true,                               // User Read/Only flag
  PluginCodec_MinMerge,               // Merge mode
  "0",                                // Initial value
  "max-br",                           // FMTP option name
  "0",                                // FMTP default value
  0,                                  // H.245 generic capability code and bit mask
  "0",                                // Minimum value
  "240000"                            // Maximum value
};

static struct PluginCodec_Option const MaxBR_H241 =
{
  PluginCodec_IntegerOption,          // Option type
  "H.241 Max BR",                     // User visible name
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
  "Max NALU Size",                    // User visible name
  false,                              // User Read/Only flag
  PluginCodec_MinMerge,               // Merge mode
  STRINGIZE(H241_MAX_NALU_SIZE),      // Initial value
  "max-rcmd-nalu-size",               // FMTP option name
  STRINGIZE(H241_MAX_NALU_SIZE),      // FMTP default value
  H241_Max_NAL_unit_size,             // H.245 generic capability code and bit mask
  "396",                              // Minimum value - uncompressed macro block size 16*16*3+12
  "65535"                             // Maximum value
};

#ifdef PLUGIN_CODEC_VERSION_INTERSECT
static struct PluginCodec_Option const MediaPacketizations =
{
  PluginCodec_StringOption,           // Option type
  PLUGINCODEC_MEDIA_PACKETIZATIONS,   // User visible name
  false,                              // User Read/Only flag
  PluginCodec_IntersectionMerge,      // Merge mode
  OpalPluginCodec_Identifer_H264_Aligned "," // Initial value
  OpalPluginCodec_Identifer_H264_NonInterleaved
};
#endif

static const char NamePacketizationMode[] = "Packetization Mode";
static struct PluginCodec_Option const PacketizationMode_0 =
{
  PluginCodec_IntegerOption,          // Option type
  NamePacketizationMode,              // User visible name
  true,                               // User Read/Only flag
  PluginCodec_EqualMerge,             // Merge mode
  "0",                                // Initial value
  "packetization-mode",               // FMTP option name
  "0",                                // FMTP default value
  0,                                  // H.245 generic capability code and bit mask
  "0",                                // Minimum value
  "2"                                 // Maximum value
};

static struct PluginCodec_Option const PacketizationMode_1 =
{
  PluginCodec_IntegerOption,          // Option type
  NamePacketizationMode,              // User visible name
  true,                               // User Read/Only flag
  PluginCodec_EqualMerge,             // Merge mode
  "1",                                // Initial value
  "packetization-mode",               // FMTP option name
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

static struct PluginCodec_Option const * OptionTable[] = {
  &Profile,
  &Level,
  &H241Profiles,
  &H241Level,
  &MaxNaluSize,
  &MaxMBPS_H241,
  &MaxFS_H241,
  &MaxBR_H241,
  &TemporalSpatialTradeOff,
  &SendAccessUnitDelimiters,
#ifdef PLUGIN_CODEC_VERSION_INTERSECT
  &MediaPacketizations,  // Note: must be last entry
#endif
  NULL
};

static struct PluginCodec_Option const * const OptionTable_0[] = {
  &Profile,
  &Level,
  &SDPProfileAndLevel,
  &MaxNaluSize,
  &MaxMBPS_SDP,
  &MaxFS_SDP,
  &MaxBR_SDP,
  &PacketizationMode_0,
  &TemporalSpatialTradeOff,
  &SendAccessUnitDelimiters,
  NULL
};

static struct PluginCodec_Option const * const OptionTable_1[] = {
  &Profile,
  &Level,
  &SDPProfileAndLevel,
  &MaxNaluSize,
  &MaxMBPS_SDP,
  &MaxFS_SDP,
  &MaxBR_SDP,
  &PacketizationMode_1,
  &TemporalSpatialTradeOff,
  &SendAccessUnitDelimiters,
  NULL
};


static struct PluginCodec_H323GenericCodecData H323GenericData = {
  OpalPluginCodec_Identifer_H264_Generic
};


static struct {
  char     m_Name[9];
  unsigned m_H264;
  unsigned m_H241;
  unsigned m_Constraints;
} const ProfileInfo[] = {
  { H264_PROFILE_STR_BASELINE, H264_PROFILE_INT_BASELINE, 64, 0x80 },
  { H264_PROFILE_STR_MAIN,     H264_PROFILE_INT_MAIN,     32, 0x40 },
  { H264_PROFILE_STR_EXTENDED, H264_PROFILE_INT_EXTENDED, 16, 0x20 }
};

static struct LevelInfoStruct {
  char     m_Name[4];
  unsigned m_H264;
  unsigned m_constraints;
  unsigned m_H241;
  unsigned m_MaxFrameSize;   // In macroblocks
  unsigned m_MaxWidthHeight; // sqrt(m_MaxFrameSize*8)*16
  unsigned m_MaxMBPS;        // In macroblocks/second
  unsigned m_MaxBitRate;
} const LevelInfo[] = {
  // Table A-1 from H.264 specification
  { H264_LEVEL_STR_1,    10, 0x00,  15,    99,  448,   1485,     64000 },
  { H264_LEVEL_STR_1_b,  11, 0x10,  19,    99,  448,   1485,    128000 },
  { H264_LEVEL_STR_1_1,  11, 0x00,  22,   396,  896,   3000,    192000 },
  { H264_LEVEL_STR_1_2,  12, 0x00,  29,   396,  896,   6000,    384000 },
  { H264_LEVEL_STR_1_3,  13, 0x00,  36,   396,  896,  11880,    768000 },
  { H264_LEVEL_STR_2,    20, 0x00,  43,   396,  896,  11880,   2000000 },
  { H264_LEVEL_STR_2_1,  21, 0x00,  50,   792, 1264,  19800,   4000000 },
  { H264_LEVEL_STR_2_2,  22, 0x00,  57,  1620, 1808,  20250,   4000000 },
  { H264_LEVEL_STR_3,    30, 0x00,  64,  1620, 1808,  40500,  10000000 },
  { H264_LEVEL_STR_3_1,  31, 0x00,  71,  3600, 2704, 108000,  14000000 },
  { H264_LEVEL_STR_3_2,  32, 0x00,  78,  5120, 3232, 216000,  20000000 },
  { H264_LEVEL_STR_4,    40, 0x00,  85,  8192, 4096, 245760,  25000000 },
  { H264_LEVEL_STR_4_1,  41, 0x00,  92,  8292, 4112, 245760,  62500000 },
  { H264_LEVEL_STR_4_2,  42, 0x00,  99,  8704, 4208, 522340,  62500000 },
  { H264_LEVEL_STR_5,    50, 0x00, 106, 22080, 6720, 589824, 135000000 },
  { H264_LEVEL_STR_5_1,  51, 0x00, 113, 36864, 8320, 983040, 240000000 }
};

static unsigned MyMaxBitRate = LevelInfo[sizeof(LevelInfo)/sizeof(LevelInfo[0])-1].m_MaxBitRate;


static struct {
  unsigned m_width;
  unsigned m_height;
  unsigned m_macroblocks;
} MaxVideoResolutions[] = {
#define VIDEO_RESOLUTION(width, height) { width, height, ((width+15)/16) * ((height+15)/16) }
  VIDEO_RESOLUTION(2816, 2304),
  VIDEO_RESOLUTION(1920, 1080),
  VIDEO_RESOLUTION(1408, 1152),
  VIDEO_RESOLUTION(1280,  720),
  VIDEO_RESOLUTION( 704,  576),
  VIDEO_RESOLUTION( 640,  480),
  VIDEO_RESOLUTION( 352,  288),
  VIDEO_RESOLUTION( 320,  240),
  VIDEO_RESOLUTION( 176,  144),
  VIDEO_RESOLUTION( 128,   96)
};
static size_t LastMaxVideoResolutions = sizeof(MaxVideoResolutions)/sizeof(MaxVideoResolutions[0]) - 1;


static unsigned GetMacroBlocks(unsigned width, unsigned height)
{
  return ((width+15)/16) * ((height+15)/16);
}


static int hexdigit(char ch)
{
  if (ch < '0')
    return 0;
  if (ch <= '9')
    return ch -'0';
  ch = (char)tolower(ch);
  if (ch < 'a')
    return 0;
  if (ch <= 'f')
    return ch - 'a' + 10;

  return 0;
}


static unsigned hexbyte(const char * hex)
{
  return ((hexdigit(hex[0]) << 4) | hexdigit(hex[1]));
}


///////////////////////////////////////////////////////////////////////////////

class MyPluginMediaFormat : public PluginCodec_MediaFormat
{
  bool m_sipOnly;

public:
  MyPluginMediaFormat(OptionsTable options, bool sipOnly)
    : PluginCodec_MediaFormat(options)
    , m_sipOnly(sipOnly)
  {
  }


  static void ClampSizes(const LevelInfoStruct & info,
                         unsigned maxWidth,
                         unsigned maxHeight,
                         unsigned & maxFrameSize,
                         OptionMap & original,
                         OptionMap & changed)
  {
    unsigned macroBlocks = GetMacroBlocks(maxWidth, maxHeight);
    if (macroBlocks > maxFrameSize || maxWidth > info.m_MaxWidthHeight || maxHeight > info.m_MaxWidthHeight) {
      size_t i = 0;
      while (i < LastMaxVideoResolutions &&
             (MaxVideoResolutions[i].m_macroblocks > maxFrameSize ||
              MaxVideoResolutions[i].m_width > info.m_MaxWidthHeight ||
              MaxVideoResolutions[i].m_height > info.m_MaxWidthHeight))
        ++i;
      maxWidth = MaxVideoResolutions[i].m_width;
      maxHeight = MaxVideoResolutions[i].m_height;
      PTRACE(5, MY_CODEC_LOG, "Reduced max resolution to " << maxWidth << 'x' << maxHeight
                 << " (" << macroBlocks << '>' << maxFrameSize << ')');
      macroBlocks = MaxVideoResolutions[i].m_macroblocks;
    }

    maxFrameSize = macroBlocks;

    ClampMax(maxWidth,  original, changed, PLUGINCODEC_OPTION_MIN_RX_FRAME_WIDTH);
    ClampMax(maxHeight, original, changed, PLUGINCODEC_OPTION_MIN_RX_FRAME_HEIGHT);
    ClampMax(maxWidth,  original, changed, PLUGINCODEC_OPTION_MAX_RX_FRAME_WIDTH);
    ClampMax(maxHeight, original, changed, PLUGINCODEC_OPTION_MAX_RX_FRAME_HEIGHT);
    ClampMax(maxWidth,  original, changed, PLUGINCODEC_OPTION_FRAME_WIDTH);
    ClampMax(maxHeight, original, changed, PLUGINCODEC_OPTION_FRAME_HEIGHT);
  }


  virtual bool ToNormalised(OptionMap & original, OptionMap & changed)
  {
    size_t levelIndex = 0;
    size_t profileIndex = sizeof(ProfileInfo)/sizeof(ProfileInfo[0]);
    unsigned maxMBPS;
    unsigned maxFrameSizeInMB;
    unsigned maxBitRate;

    if (original["Protocol"] == "H.323") {
      unsigned h241profiles = String2Unsigned(original[H241Profiles.m_name]);
      while (--profileIndex > 0) {
        if ((h241profiles&ProfileInfo[profileIndex].m_H241) != 0)
          break;
      }

      unsigned h241level = String2Unsigned(original[H241Level.m_name]);
      for (; levelIndex < sizeof(LevelInfo)/sizeof(LevelInfo[0])-1; ++levelIndex) {
        if (h241level <= LevelInfo[levelIndex].m_H241)
          break;
      }

      maxMBPS = String2Unsigned(original[MaxMBPS_H241.m_name])*500;
      maxFrameSizeInMB = String2Unsigned(original[MaxFS_H241.m_name])*256;
      maxBitRate = String2Unsigned(original[MaxBR_H241.m_name])*25000;
    }
    else {
      std::string sdpProfLevel = original[SDPProfileAndLevel.m_name];
      if (sdpProfLevel.length() < 6) {
        PTRACE(1, MY_CODEC_LOG, "SDP profile-level-id field illegal.");
        return false;
      }

      unsigned sdpProfile = hexbyte(&sdpProfLevel[0]);
      while (--profileIndex > 0) {
        if (sdpProfile == ProfileInfo[profileIndex].m_H264)
          break;
      }

      unsigned sdpConstraints = hexbyte(&sdpProfLevel[2]);
      unsigned sdpLevel = hexbyte(&sdpProfLevel[4]);

      // convert sdpLevel to an index into LevelInfo
      for (; levelIndex < sizeof(LevelInfo)/sizeof(LevelInfo[0])-1; ++levelIndex) {

        // if not reached SDP level yet, keep looking
        if (LevelInfo[levelIndex].m_H264 < sdpLevel)
          continue;

        // if gone past the level, stop
        if (LevelInfo[levelIndex].m_H264 > sdpLevel)
          break;

        // if reached the level, stop if constraints are the same
        if ((sdpConstraints & 0x10) == LevelInfo[levelIndex].m_constraints)
          break;
      }

      maxMBPS = String2Unsigned(original[MaxMBPS_SDP.m_name]);
      maxFrameSizeInMB = String2Unsigned(original[MaxFS_SDP.m_name]);
      maxBitRate = String2Unsigned(original[MaxBR_SDP.m_name])*1000;
    }

    Change(ProfileInfo[profileIndex].m_Name, original, changed, Profile.m_name); 
    Change(LevelInfo[levelIndex].m_Name, original, changed, Level.m_name);

    if (maxFrameSizeInMB < LevelInfo[levelIndex].m_MaxFrameSize)
      maxFrameSizeInMB = LevelInfo[levelIndex].m_MaxFrameSize;
    ClampSizes(LevelInfo[levelIndex],
               String2Unsigned(original[PLUGINCODEC_OPTION_MAX_RX_FRAME_WIDTH]),
               String2Unsigned(original[PLUGINCODEC_OPTION_MAX_RX_FRAME_HEIGHT]),
               maxFrameSizeInMB,
               original, changed);

    // Frame rate
    if (maxMBPS < LevelInfo[levelIndex].m_MaxMBPS)
      maxMBPS = LevelInfo[levelIndex].m_MaxMBPS;
    ClampMin(GetMacroBlocks(String2Unsigned(original[PLUGINCODEC_OPTION_MIN_RX_FRAME_WIDTH]),
                            String2Unsigned(original[PLUGINCODEC_OPTION_MIN_RX_FRAME_HEIGHT]))*MyClockRate/maxMBPS,
             original, changed, PLUGINCODEC_OPTION_FRAME_TIME);

    // Bit rate
    if (maxBitRate < LevelInfo[levelIndex].m_MaxBitRate)
      maxBitRate = LevelInfo[levelIndex].m_MaxBitRate;
    ClampMax(maxBitRate, original, changed, PLUGINCODEC_OPTION_MAX_BIT_RATE);
    ClampMax(maxBitRate, original, changed, PLUGINCODEC_OPTION_TARGET_BIT_RATE);
    return true;
  }


  virtual bool ToCustomised(OptionMap & original, OptionMap & changed)
  {
    // Determine the profile
    std::string str = original[Profile.m_name];
    if (str.empty())
      str = H264_PROFILE_STR_BASELINE;

    size_t profileIndex = sizeof(ProfileInfo)/sizeof(ProfileInfo[0]);
    while (--profileIndex > 0) {
      if (str == ProfileInfo[profileIndex].m_Name)
        break;
    }

    Change(ProfileInfo[profileIndex].m_H241, original, changed, H241Profiles.m_name);

    // get the current level 
    str = original[Level.m_name];
    if (str.empty())
      str = H264_LEVEL_STR_1_3;

    size_t levelIndex = sizeof(LevelInfo)/sizeof(LevelInfo[0]);
    while (--levelIndex > 0) {
      if (str == LevelInfo[levelIndex].m_Name)
        break;
    }
    PTRACE(5, MY_CODEC_LOG, "Level \"" << str << "\" selected index " << levelIndex);

    /* While we have selected the desired level, we may need to adjust it
       further due to resolution restrictions. This is due to the fact that
       we have no other mechnism to prevent the remote from sending, say
       CIF when we only support QCIF.
     */
    unsigned maxWidth = String2Unsigned(original[PLUGINCODEC_OPTION_MAX_RX_FRAME_WIDTH]);
    unsigned maxHeight = String2Unsigned(original[PLUGINCODEC_OPTION_MAX_RX_FRAME_HEIGHT]);
    unsigned maxFrameSizeInMB = GetMacroBlocks(maxWidth, maxHeight);
    if (maxFrameSizeInMB > 0) {
      while (levelIndex > 0 && maxFrameSizeInMB < LevelInfo[levelIndex].m_MaxFrameSize)
        --levelIndex;
    }
    PTRACE(5, MY_CODEC_LOG, "Max resolution " << maxWidth << 'x' << maxHeight << " selected index " << levelIndex);

    // set the new level
    Change(LevelInfo[levelIndex].m_H241, original, changed, H241Level.m_name);

    // Calculate SDP parameters from the adjusted profile/level
    char sdpProfLevel[7];
    sprintf(sdpProfLevel, "%02x%02x%02x",
            ProfileInfo[profileIndex].m_H264,
            ProfileInfo[profileIndex].m_Constraints | LevelInfo[levelIndex].m_constraints,
            LevelInfo[levelIndex].m_H264);
    Change(sdpProfLevel, original, changed, SDPProfileAndLevel.m_name);

    // Clamp other variables (width/height etc) according to the adjusted profile/level
    ClampSizes(LevelInfo[levelIndex], maxWidth, maxHeight, maxFrameSizeInMB, original, changed);

    // Do this afer the clamping, maxFrameSizeInMB may change
    if (maxFrameSizeInMB > LevelInfo[levelIndex].m_MaxFrameSize) {
      Change(maxFrameSizeInMB, original, changed, MaxFS_SDP.m_name);
      Change((maxFrameSizeInMB+255)/256, original, changed, MaxFS_H241.m_name);
    }

    // Set exception to bit rate if necessary
    unsigned bitRate = String2Unsigned(original[PLUGINCODEC_OPTION_MAX_BIT_RATE]);
    if (bitRate > LevelInfo[levelIndex].m_MaxBitRate) {
      Change((bitRate+999)/1000, original, changed, MaxBR_SDP.m_name);
      Change((bitRate+24999)/25000, original, changed, MaxBR_H241.m_name);
    }

    // Set exception to frame rate if necessary
    unsigned mbps = maxFrameSizeInMB*MyClockRate/String2Unsigned(original[PLUGINCODEC_OPTION_FRAME_TIME]);
    if (mbps > LevelInfo[levelIndex].m_MaxMBPS) {
      Change(mbps, original, changed, MaxMBPS_SDP.m_name);
      Change((mbps+499)/500, original, changed, MaxMBPS_H241.m_name);
    }

    return true;
  }


  virtual bool IsValidForProtocol(const char * protocol)
  {
    return (strcasecmp(protocol, "SIP") == 0) == m_sipOnly;
  }
}; 

/* SIP requires two completely independent media formats for packetisation
   modes zero and one. */
static MyPluginMediaFormat MyMediaFormatInfo  (OptionTable  , false);
static MyPluginMediaFormat MyMediaFormatInfo_0(OptionTable_0, true );
static MyPluginMediaFormat MyMediaFormatInfo_1(OptionTable_1, true );


///////////////////////////////////////////////////////////////////////////////

class MyEncoder : public PluginCodec<MY_CODEC>
{
  protected:
    unsigned m_width;
    unsigned m_height;
    unsigned m_frameRate;
    unsigned m_bitRate;
    unsigned m_profile;
    unsigned m_level;
    unsigned m_constraints;
    unsigned m_packetisationMode;
    unsigned m_maxRTPSize;
    unsigned m_maxNALUSize;
    unsigned m_tsto;
    unsigned m_keyFramePeriod;

    H264Encoder m_encoder;

  public:
    MyEncoder(const PluginCodec_Definition * defn)
      : PluginCodec<MY_CODEC>(defn)
      , m_width(352)
      , m_height(288)
      , m_frameRate(15)
      , m_bitRate(512000)
      , m_profile(DefaultProfileInt)
      , m_level(DefaultLevelInt)
      , m_constraints(0)
      , m_packetisationMode(1)
      , m_maxRTPSize(PluginCodec_RTP_MaxPayloadSize)
      , m_maxNALUSize(1400)
      , m_tsto(31)
      , m_keyFramePeriod(0)
    {
    }


    virtual bool Construct()
    {
      /* Complete construction of object after it has been created. This
         allows you to fail the create operation (return false), which cannot
         be done in the normal C++ constructor. */

      // ABR with bit rate tolerance = 1 is CBR...
      if (FFMPEGLibraryInstance.Load() && m_encoder.Load(this)) {
        PTRACE(4, MY_CODEC_LOG, "Opened encoder (SVN $Revision$)");
        return true;
      }

      PTRACE(1, MY_CODEC_LOG, "Could not open encoder.");
      return false;
    }


    virtual bool SetOption(const char * optionName, const char * optionValue)
    {
      if (strcasecmp(optionName, PLUGINCODEC_OPTION_FRAME_WIDTH) == 0)
        return SetOptionUnsigned(m_width, optionValue, 16, MyMaxWidth);

      if (strcasecmp(optionName, PLUGINCODEC_OPTION_FRAME_HEIGHT) == 0)
        return SetOptionUnsigned(m_height, optionValue, 16, MyMaxHeight);

      if (strcasecmp(optionName, PLUGINCODEC_OPTION_FRAME_TIME) == 0) {
        unsigned frameTime = MyClockRate/m_frameRate;
        if (!SetOptionUnsigned(frameTime, optionValue, MyClockRate/MyMaxFrameRate, MyClockRate))
          return false;
        m_frameRate = MyClockRate/frameTime;
        return true;
      }

      if (strcasecmp(optionName, PLUGINCODEC_OPTION_TARGET_BIT_RATE) == 0)
        return SetOptionUnsigned(m_bitRate, optionValue, 1000);

      if (strcasecmp(optionName, PLUGINCODEC_OPTION_MAX_TX_PACKET_SIZE) == 0)
        return SetOptionUnsigned(m_maxRTPSize, optionValue, 256, 8192);

      if (strcasecmp(optionName, MaxNaluSize.m_name) == 0)
        return SetOptionUnsigned(m_maxNALUSize, optionValue, 256, 8192);

      if (STRCMPI(optionName, PLUGINCODEC_OPTION_TEMPORAL_SPATIAL_TRADE_OFF) == 0)
        return SetOptionUnsigned(m_tsto, optionValue, 1, 31);

      if (strcasecmp(optionName, PLUGINCODEC_OPTION_TX_KEY_FRAME_PERIOD) == 0)
        return SetOptionUnsigned(m_keyFramePeriod, optionValue, 0);

      if (strcasecmp(optionName, Level.m_name) == 0) {
        for (size_t i = 0; i < sizeof(LevelInfo)/sizeof(LevelInfo[0]); i++) {
          if (strcasecmp(optionValue, LevelInfo[i].m_Name) == 0) {
            m_level = LevelInfo[i].m_H264;
            return true;
          }
        }
        return false;
      }

      if (strcasecmp(optionName, Profile.m_name) == 0) {
        for (size_t i = 0; i < sizeof(ProfileInfo)/sizeof(ProfileInfo[0]); ++i) {
          if (strcasecmp(optionValue, ProfileInfo[i].m_Name) == 0) {
            m_profile = ProfileInfo[i].m_H264;
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
        if (strstr(optionValue, OpalPluginCodec_Identifer_H264_Interleaved) != NULL)
          return SetPacketisationMode(2);
        if (strstr(optionValue, OpalPluginCodec_Identifer_H264_NonInterleaved) != NULL)
          return SetPacketisationMode(1);
        if (*optionValue != '\0' && strstr(optionValue, OpalPluginCodec_Identifer_H264_Aligned) == NULL)
          PTRACE(2, MY_CODEC_LOG, "Unknown packetisation mode: \"" << optionValue << '"');
        return SetPacketisationMode(0);
      }

      if (strcasecmp(optionName, NamePacketizationMode) == 0)
        return SetPacketisationMode(atoi(optionValue));

      // Base class sets bit rate and frame time
      return PluginCodec<MY_CODEC>::SetOption(optionName, optionValue);
    }


    bool SetPacketisationMode(unsigned mode)
    {
      PTRACE(4, MY_CODEC_LOG, "Setting NALU " << (mode == 0 ? "aligned" : "fragmentation") << " mode.");

      if (mode > 2) // Or 3 if support interleaved mode
        return false; // Unknown/unsupported packetization mode

      if (m_packetisationMode != mode) {
        m_packetisationMode = mode;
        m_optionsSame = false;
      }

      return true;
    }


    virtual bool OnChangedOptions()
    {
      /* After all the options are set via SetOptions() this is called if any
         of them changed. This would do whatever is needed to set parmaeters
         for the actual codec. */

      m_encoder.SetProfileLevel(m_profile, m_level, m_constraints);
      m_encoder.SetFrameWidth(m_width);
      m_encoder.SetFrameHeight(m_height);
      m_encoder.SetFrameRate(m_frameRate);
      m_encoder.SetTargetBitrate(m_bitRate/1000);
      m_encoder.SetTSTO(m_tsto);
      m_encoder.SetMaxKeyFramePeriod(m_keyFramePeriod);

      if (m_packetisationMode == 0) {
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
                              "fps=" << m_frameRate << " "
                              "bps=" << m_bitRate << " "
                              "RTP=" << m_maxRTPSize << " "
                              "NALU=" << m_maxNALUSize << " "
                              "TSTO=" << m_tsto);
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

class MyDecoder : public PluginCodec<MY_CODEC>
{
    AVCodec        * m_codec;
    AVCodecContext * m_context;
    AVFrame        * m_picture;
    H264Frame        m_fullFrame;
    size_t           m_outputSize;

  public:
    MyDecoder(const PluginCodec_Definition * defn)
      : PluginCodec<MY_CODEC>(defn)
      , m_codec(NULL)
      , m_context(NULL)
      , m_picture(NULL)
      , m_outputSize(352*288*3/2 + sizeof(PluginCodec_Video_FrameHeader) + PluginCodec_RTP_MinHeaderSize)
    {
    }


    ~MyDecoder()
    {
      if (m_context != NULL) {
        if (m_context->codec != NULL)
          FFMPEGLibraryInstance.AvcodecClose(m_context);
        FFMPEGLibraryInstance.AvcodecFree(m_context);
      }

      if (m_picture != NULL)
        FFMPEGLibraryInstance.AvcodecFree(m_picture);
    }


    virtual bool Construct()
    {
      if (!FFMPEGLibraryInstance.Load())
        return false;

      /* Complete construction of object after it has been created. This
         allows you to fail the create operation (return false), which cannot
         be done in the normal C++ constructor. */

      if ((m_codec = FFMPEGLibraryInstance.AvcodecFindDecoder(CODEC_ID_H264)) == NULL)
        return false;

      if ((m_context = FFMPEGLibraryInstance.AvcodecAllocContext()) == NULL)
        return false;

      m_context->workaround_bugs = FF_BUG_AUTODETECT;
      m_context->error_recognition = FF_ER_AGGRESSIVE;
      m_context->idct_algo = FF_IDCT_H264;
      m_context->error_concealment = FF_EC_GUESS_MVS | FF_EC_DEBLOCK;
      m_context->flags = CODEC_FLAG_INPUT_PRESERVED | CODEC_FLAG_EMU_EDGE;
      m_context->flags2 = CODEC_FLAG2_BRDO |
                          CODEC_FLAG2_MEMC_ONLY |
                          CODEC_FLAG2_DROP_FRAME_TIMECODE |
                          CODEC_FLAG2_SKIP_RD |
                          CODEC_FLAG2_CHUNKS;

      if ((m_picture = FFMPEGLibraryInstance.AvcodecAllocFrame()) == NULL)
        return false;

      if (FFMPEGLibraryInstance.AvcodecOpen(m_context, m_codec) < 0)
        return false;

      PTRACE(4, MY_CODEC_LOG, "Opened decoder (SVN $Revision$)");
      return true;
    }


    virtual size_t GetOutputDataSize()
    {
      return m_outputSize;
    }


    virtual bool Transcode(const void * fromPtr,
                             unsigned & fromLen,
                                 void * toPtr,
                             unsigned & toLen,
                             unsigned & flags)
    {
      if (!FFMPEGLibraryInstance.IsLoaded())
        return false;

      RTPFrame srcRTP((const unsigned char *)fromPtr, fromLen);
      if (!m_fullFrame.SetFromRTPFrame(srcRTP, flags))
        return true;

      // Wait for marker to indicate end of frame
      if (!srcRTP.GetMarker())
        return true;

      if (m_fullFrame.GetFrameSize() == 0) {
        m_fullFrame.BeginNewFrame();
        PTRACE(3, MY_CODEC_LOG, "Got an empty video frame - skipping");
        return true;
      }

      int bytesToDecode = m_fullFrame.GetFrameSize();

      if (m_fullFrame.GetProfile() == H264_PROFILE_INT_BASELINE)
        m_context->has_b_frames = 0;

      /* Do conversion of RTP packet. Note that typically many are received
         before an output frame is generated. If no output fram is available
         yet, simply return true and more will be sent. */

      int gotPicture = false;
      int bytesDecoded = FFMPEGLibraryInstance.AvcodecDecodeVideo(m_context,
                                                                  m_picture,
                                                                  &gotPicture,
                                                                  m_fullFrame.GetFramePtr(),
                                                                  bytesToDecode);
      m_fullFrame.BeginNewFrame();

      if (bytesDecoded <= 0) {
        // Should have output log error in logging callback dyna.cxx
        flags |= PluginCodec_ReturnCoderRequestIFrame;
        return true;
      }

      if (!gotPicture) {
        PTRACE(3, MY_CODEC_LOG, "Decoded " << bytesDecoded<< " of " << bytesToDecode 
               << " bytes without a picture.");
        flags |= PluginCodec_ReturnCoderRequestIFrame;
        return true;
      }

      if (bytesDecoded == bytesToDecode)
        PTRACE(5, MY_CODEC_LOG, "Decoded " << bytesToDecode << " byte "
               << (m_picture->key_frame ? 'I' : 'P') << "-Frame");
      else
        PTRACE(4, MY_CODEC_LOG, "Decoded only " << bytesDecoded << " of " << bytesToDecode << " byte "
               << (m_picture->key_frame ? 'I' : 'P') << "-Frame");

      // If we determine this is an Intra (reference) frame, set flag
      if (m_picture->key_frame)
        flags |= PluginCodec_ReturnCoderIFrame;

      PluginCodec_Video_FrameHeader * videoHeader =
                (PluginCodec_Video_FrameHeader *)PluginCodec_RTP_GetPayloadPtr(toPtr);
      videoHeader->width = m_context->width;
      videoHeader->height = m_context->height;

      /* If we determine via toLen the output is not big enough, set flag.
         Make sure GetOutputDataSize() then returns correct larger size. */
      size_t ySize = m_context->width * m_context->height;
      size_t uvSize = ySize/4;
      size_t newToLen = ySize+uvSize+uvSize+sizeof(PluginCodec_Video_FrameHeader)+PluginCodec_RTP_MinHeaderSize;
      if (newToLen > toLen) {
        m_outputSize = newToLen;
        flags |= PluginCodec_ReturnCoderBufferTooSmall;
      }
      else {
        flags |= PluginCodec_ReturnCoderLastFrame;

        uint8_t * src[3] = { m_picture->data[0], m_picture->data[1], m_picture->data[2] };
        uint8_t * dst[3] = { OPAL_VIDEO_FRAME_DATA_PTR(videoHeader), dst[0] + ySize, dst[1] + uvSize };
        size_t dstLineSize[3] = { m_context->width, m_context->width/2, m_context->width/2 };

        for (int y = 0; y < m_context->height; ++y) {
          for (int plane = 0; plane < 3; ++plane) {
            if ((y&1) == 0 || plane == 0) {
              memcpy(dst[plane], src[plane], dstLineSize[plane]);
              src[plane] += m_picture->linesize[plane];
              dst[plane] += dstLineSize[plane];
            }
          }
        }

        PluginCodec_RTP_SetMarker(toPtr, true);
      }

      toLen = newToLen;
      return true;
    }
};


///////////////////////////////////////////////////////////////////////////////

static struct PluginCodec_Definition MyCodecDefinition[] =
{
  {
    // Encoder H.323
    MyVersion,                          // codec API version
    &LicenseInfo,                       // license information

    PluginCodec_MediaTypeVideo |        // audio codec
    PluginCodec_RTPTypeDynamic,         // dynamic RTP type

    MyDescription,                      // text decription
    YUV420PFormatName,                  // source format
    FormatNameH323,                     // destination format

    &MyMediaFormatInfo,                 // user data 

    MyClockRate,                        // samples per second
    MyMaxBitRate,                       // raw bits per second
    1000000/MyMaxFrameRate,             // microseconds per frame

    {{
      MyMaxWidth,                       // frame width
      MyMaxHeight,                      // frame height
      MyMaxFrameRate,                   // recommended frame rate
      MyMaxFrameRate                    // maximum frame rate
    }},
    
    0,                                  // IANA RTP payload code
    MyPayloadName,                      // IANA RTP payload name

    PluginCodec<MY_CODEC>::Create<MyEncoder>,     // create codec function
    PluginCodec<MY_CODEC>::Destroy,               // destroy codec
    PluginCodec<MY_CODEC>::Transcode,             // encode/decode
    PluginCodec<MY_CODEC>::GetControls(),         // codec controls

    PluginCodec_H323Codec_generic,      // h323CapabilityType 
    &H323GenericData                    // h323CapabilityData
  },
  { 
    // Decoder H.323
    MyVersion,                          // codec API version
    &LicenseInfo,                       // license information

    PluginCodec_MediaTypeVideo |        // audio codec
    PluginCodec_RTPTypeDynamic,         // dynamic RTP type

    MyDescription,                      // text decription
    FormatNameH323,                     // source format
    YUV420PFormatName,                  // destination format

    &MyMediaFormatInfo,                 // user data 

    MyClockRate,                        // samples per second
    MyMaxBitRate,                       // raw bits per second
    1000000/MyMaxFrameRate,             // microseconds per frame

    {{
      MyMaxWidth,                       // frame width
      MyMaxHeight,                      // frame height
      MyMaxFrameRate,                   // recommended frame rate
      MyMaxFrameRate                    // maximum frame rate
    }},
    
    0,                                  // IANA RTP payload code
    MyPayloadName,                      // IANA RTP payload name

    PluginCodec<MY_CODEC>::Create<MyDecoder>,     // create codec function
    PluginCodec<MY_CODEC>::Destroy,               // destroy codec
    PluginCodec<MY_CODEC>::Transcode,             // encode/decode
    PluginCodec<MY_CODEC>::GetControls(),         // codec controls

    PluginCodec_H323Codec_generic,      // h323CapabilityType 
    &H323GenericData                    // h323CapabilityData
  },
  {
    // Encoder SIP mode 1
    MyVersion,                          // codec API version
    &LicenseInfo,                       // license information

    PluginCodec_MediaTypeVideo |        // audio codec
    PluginCodec_RTPTypeDynamic,         // dynamic RTP type

    MyDescription,                      // text decription
    YUV420PFormatName,                  // source format
    FormatNameSIP1,                     // destination format

    &MyMediaFormatInfo_1,               // user data 

    MyClockRate,                        // samples per second
    MyMaxBitRate,                       // raw bits per second
    1000000/MyMaxFrameRate,             // microseconds per frame

    {{
      MyMaxWidth,                       // frame width
      MyMaxHeight,                      // frame height
      MyMaxFrameRate,                   // recommended frame rate
      MyMaxFrameRate                    // maximum frame rate
    }},
    
    0,                                  // IANA RTP payload code
    MyPayloadName,                      // IANA RTP payload name

    PluginCodec<MY_CODEC>::Create<MyEncoder>,     // create codec function
    PluginCodec<MY_CODEC>::Destroy,               // destroy codec
    PluginCodec<MY_CODEC>::Transcode,             // encode/decode
    PluginCodec<MY_CODEC>::GetControls(),         // codec controls

    PluginCodec_H323Codec_NoH323,       // h323CapabilityType 
    NULL                                // h323CapabilityData
  },
  { 
    // Decoder SIP mode 1
    MyVersion,                          // codec API version
    &LicenseInfo,                       // license information

    PluginCodec_MediaTypeVideo |        // audio codec
    PluginCodec_RTPTypeDynamic,         // dynamic RTP type

    MyDescription,                      // text decription
    FormatNameSIP1,                     // source format
    YUV420PFormatName,                  // destination format

    &MyMediaFormatInfo_1,               // user data 

    MyClockRate,                        // samples per second
    MyMaxBitRate,                       // raw bits per second
    1000000/MyMaxFrameRate,             // microseconds per frame

    {{
      MyMaxWidth,                       // frame width
      MyMaxHeight,                      // frame height
      MyMaxFrameRate,                   // recommended frame rate
      MyMaxFrameRate                    // maximum frame rate
    }},
    
    0,                                  // IANA RTP payload code
    MyPayloadName,                      // IANA RTP payload name

    PluginCodec<MY_CODEC>::Create<MyDecoder>,     // create codec function
    PluginCodec<MY_CODEC>::Destroy,               // destroy codec
    PluginCodec<MY_CODEC>::Transcode,             // encode/decode
    PluginCodec<MY_CODEC>::GetControls(),         // codec controls

    PluginCodec_H323Codec_NoH323,       // h323CapabilityType 
    NULL                                // h323CapabilityData
  },
  {
    // Encoder SIP mode 0
    MyVersion,                          // codec API version
    &LicenseInfo,                       // license information

    PluginCodec_MediaTypeVideo |        // audio codec
    PluginCodec_RTPTypeDynamic,         // dynamic RTP type

    MyDescription,                      // text decription
    YUV420PFormatName,                  // source format
    FormatNameSIP0,                     // destination format

    &MyMediaFormatInfo_0,               // user data 

    MyClockRate,                        // samples per second
    MyMaxBitRate,                       // raw bits per second
    1000000/MyMaxFrameRate,             // microseconds per frame

    {{
      MyMaxWidth,                       // frame width
      MyMaxHeight,                      // frame height
      MyMaxFrameRate,                   // recommended frame rate
      MyMaxFrameRate                    // maximum frame rate
    }},
    
    0,                                  // IANA RTP payload code
    MyPayloadName,                      // IANA RTP payload name

    PluginCodec<MY_CODEC>::Create<MyEncoder>,     // create codec function
    PluginCodec<MY_CODEC>::Destroy,               // destroy codec
    PluginCodec<MY_CODEC>::Transcode,             // encode/decode
    PluginCodec<MY_CODEC>::GetControls(),         // codec controls

    PluginCodec_H323Codec_NoH323,       // h323CapabilityType 
    NULL                                // h323CapabilityData
  },
  { 
    // Decoder SIP mode 0
    MyVersion,                          // codec API version
    &LicenseInfo,                       // license information

    PluginCodec_MediaTypeVideo |        // audio codec
    PluginCodec_RTPTypeDynamic,         // dynamic RTP type

    MyDescription,                      // text decription
    FormatNameSIP0,                     // source format
    YUV420PFormatName,                  // destination format

    &MyMediaFormatInfo_0,               // user data 

    MyClockRate,                        // samples per second
    MyMaxBitRate,                       // raw bits per second
    1000000/MyMaxFrameRate,             // microseconds per frame

    {{
      MyMaxWidth,                       // frame width
      MyMaxHeight,                      // frame height
      MyMaxFrameRate,                   // recommended frame rate
      MyMaxFrameRate                    // maximum frame rate
    }},
    
    0,                                  // IANA RTP payload code
    MyPayloadName,                      // IANA RTP payload name

    PluginCodec<MY_CODEC>::Create<MyDecoder>,     // create codec function
    PluginCodec<MY_CODEC>::Destroy,               // destroy codec
    PluginCodec<MY_CODEC>::Transcode,             // encode/decode
    PluginCodec<MY_CODEC>::GetControls(),         // codec controls

    PluginCodec_H323Codec_NoH323,       // h323CapabilityType 
    NULL                                // h323CapabilityData
  }
};

static size_t const MyCodecDefinitionSize = sizeof(MyCodecDefinition)/sizeof(MyCodecDefinition[0]);

extern "C"
{
  PLUGIN_CODEC_IMPLEMENT(MY_CODEC)
  PLUGIN_CODEC_DLL_API
  struct PluginCodec_Definition * PLUGIN_CODEC_GET_CODEC_FN(unsigned * count, unsigned version)
  {
    if (version < PLUGIN_CODEC_VERSION_OPTIONS)
      return NULL;

    PluginCodec_MediaFormat::AdjustAllForVersion(version, MyCodecDefinition, MyCodecDefinitionSize);

    *count = MyCodecDefinitionSize;
    return MyCodecDefinition;
  }
};

/////////////////////////////////////////////////////////////////////////////
