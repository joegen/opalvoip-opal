/*
 * h264mf.cxx
 *
 * H.264 Media Format descriptions
 *
 * Open Phone Abstraction Library
 * Formally known as the Open H323 project.
 *
 * Copyright (c) 2008 Vox Lucida
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
 * The Original Code is Open Phone Abstraction Library
 *
 * The Initial Developer of the Original Code is Vox Lucida
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision$
 * $Author$
 * $Date$
 */

// This is to get around the DevStudio precompiled headers rqeuirements.
#ifdef OPAL_PLUGIN_COMPILE
#define PTLIB_PTLIB_H
#endif

#include <ptlib.h>

#include <codec/opalplugin.hpp>
#include <codec/known.h>


///////////////////////////////////////////////////////////////////////////////

#ifdef MY_CODEC
  #define MY_CODEC_LOG STRINGIZE(MY_CODEC)
#else
  #define MY_CODEC_LOG OPAL_H264
#endif

static const char H264FormatName[]  = OPAL_H264;
static const char H2640FormatName[] = OPAL_H264_MODE0;
static const char H2641FormatName[] = OPAL_H264_MODE1;

static const char H264EncodingName[] = "H264";

// Level 3 allows 4CIF at 25fps
#define DefaultProfileStr          H264_PROFILE_STR_BASELINE
#define DefaultProfileInt          66
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

#define OpalPluginCodec_Identifer_H264_Truncated "0.0.8.241.0.0.0" // Some stupid endpoints (e.g. Polycom) use this, one zero short!


static char const ProfileName[] = "Profile";
static char const LevelName[] = "Level";
static char const MaxNALUSizeName[] = "Max NALU Size";

static char const H241ProfilesName[] = "H.241 Profile Mask";
static char const H241LevelName[] = "H.241 Level";
static char const MaxMBPS_H241_Name[] = "H.241 Max MBPS";
static char const MaxFS_H241_Name[] = "H.241 Max FS";
static char const MaxBR_H241_Name[] = "H.241 Max BR";

static char const SDPProfileAndLevelName[] = "SIP/SDP Profile & Level";
static char const SDPProfileAndLevelFMTPName[] = "profile-level-id";
static char const SDPProfileAndLevelFMTPDflt[] = "42800A";
static char const MaxMBPS_SDP_Name[] = "SIP/SDP Max MBPS";
static char const MaxMBPS_FMTPName[] = "max-mbps";
static char const MaxFS_SDP_Name[] = "SIP/SDP Max FS";
static char const MaxFS_FMTPName[] = "max-fs";
static char const MaxBR_SDP_Name[] = "SIP/SDP Max BR";
static char const MaxBR_FMTPName[] = "max-br";
static char const MaxNALUSizeFMTPName[] = "max-rcmd-nalu-size";
static const char PacketizationModeName[] = "Packetization Mode";
static const char PacketizationFMTPName[] = "packetization-mode";


static struct {
  char     m_Name[9];
  unsigned m_H264;
  unsigned m_H241;
  unsigned m_Constraints;
} const ProfileInfo[] = {
  { H264_PROFILE_STR_BASELINE, 66, 64, 0x80 },
  { H264_PROFILE_STR_MAIN,     77, 32, 0x40 },
  { H264_PROFILE_STR_EXTENDED, 88, 16, 0x20 }
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

static void ClampSizes(const LevelInfoStruct & info,
                        unsigned maxWidth,
                        unsigned maxHeight,
                        unsigned & maxFrameSize,
                        PluginCodec_OptionMap & original,
                        PluginCodec_OptionMap & changed)
{
  if (PluginCodec_Utilities::ClampResolution(maxWidth, maxHeight, maxFrameSize, info.m_MaxWidthHeight, info.m_MaxWidthHeight))
    PTRACE(5, MY_CODEC_LOG, "Reduced max resolution to " << maxWidth << 'x' << maxHeight);

  PluginCodec_Utilities::ClampMax(maxWidth,  original, changed, PLUGINCODEC_OPTION_MIN_RX_FRAME_WIDTH);
  PluginCodec_Utilities::ClampMax(maxHeight, original, changed, PLUGINCODEC_OPTION_MIN_RX_FRAME_HEIGHT);
  PluginCodec_Utilities::ClampMax(maxWidth,  original, changed, PLUGINCODEC_OPTION_MAX_RX_FRAME_WIDTH);
  PluginCodec_Utilities::ClampMax(maxHeight, original, changed, PLUGINCODEC_OPTION_MAX_RX_FRAME_HEIGHT);
  PluginCodec_Utilities::ClampMax(maxWidth,  original, changed, PLUGINCODEC_OPTION_FRAME_WIDTH);
  PluginCodec_Utilities::ClampMax(maxHeight, original, changed, PLUGINCODEC_OPTION_FRAME_HEIGHT);
}


static bool MyToNormalised(PluginCodec_OptionMap & original, PluginCodec_OptionMap & changed)
{
  size_t levelIndex = 0;
  size_t profileIndex = sizeof(ProfileInfo)/sizeof(ProfileInfo[0]);
  unsigned maxMBPS;
  unsigned maxFrameSizeInMB;
  unsigned maxBitRate;

  if (original["Protocol"] == "H.323") {
    unsigned h241profiles = original.GetUnsigned(H241ProfilesName);
    while (--profileIndex > 0) {
      if ((h241profiles&ProfileInfo[profileIndex].m_H241) != 0)
        break;
    }

    unsigned h241level = original.GetUnsigned(H241LevelName);
    for (; levelIndex < sizeof(LevelInfo)/sizeof(LevelInfo[0])-1; ++levelIndex) {
      if (h241level <= LevelInfo[levelIndex].m_H241)
        break;
    }

    maxMBPS = original.GetUnsigned(MaxMBPS_H241_Name)*500;
    maxFrameSizeInMB = original.GetUnsigned(MaxFS_H241_Name)*256;
    maxBitRate = original.GetUnsigned(MaxBR_H241_Name)*25000;
  }
  else {
    std::string sdpProfLevel = original[SDPProfileAndLevelName];
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

    maxMBPS = original.GetUnsigned(MaxMBPS_SDP_Name);
    maxFrameSizeInMB = original.GetUnsigned(MaxFS_SDP_Name);
    maxBitRate = original.GetUnsigned(MaxBR_SDP_Name)*1000;
  }

  PluginCodec_Utilities::Change(ProfileInfo[profileIndex].m_Name, original, changed, ProfileName); 
  PluginCodec_Utilities::Change(LevelInfo[levelIndex].m_Name, original, changed, LevelName);

  if (maxFrameSizeInMB < LevelInfo[levelIndex].m_MaxFrameSize)
    maxFrameSizeInMB = LevelInfo[levelIndex].m_MaxFrameSize;
  ClampSizes(LevelInfo[levelIndex],
              original.GetUnsigned(PLUGINCODEC_OPTION_MAX_RX_FRAME_WIDTH),
              original.GetUnsigned(PLUGINCODEC_OPTION_MAX_RX_FRAME_HEIGHT),
              maxFrameSizeInMB,
              original, changed);

  // Frame rate
  if (maxMBPS < LevelInfo[levelIndex].m_MaxMBPS)
    maxMBPS = LevelInfo[levelIndex].m_MaxMBPS;
  PluginCodec_Utilities::ClampMin(PluginCodec_Utilities::GetMacroBlocks(original.GetUnsigned(PLUGINCODEC_OPTION_MIN_RX_FRAME_WIDTH),
                                      original.GetUnsigned(PLUGINCODEC_OPTION_MIN_RX_FRAME_HEIGHT))*PLUGINCODEC_VIDEO_CLOCK/maxMBPS,
            original, changed, PLUGINCODEC_OPTION_FRAME_TIME);

  // Bit rate
  if (maxBitRate < LevelInfo[levelIndex].m_MaxBitRate)
    maxBitRate = LevelInfo[levelIndex].m_MaxBitRate;
  PluginCodec_Utilities::ClampMax(maxBitRate, original, changed, PLUGINCODEC_OPTION_MAX_BIT_RATE);
  PluginCodec_Utilities::ClampMax(maxBitRate, original, changed, PLUGINCODEC_OPTION_TARGET_BIT_RATE);
  return true;
}


static bool MyToCustomised(PluginCodec_OptionMap & original, PluginCodec_OptionMap & changed)
{
  // Determine the profile
  std::string str = original[ProfileName];
  if (str.empty())
    str = H264_PROFILE_STR_BASELINE;

  size_t profileIndex = sizeof(ProfileInfo)/sizeof(ProfileInfo[0]);
  while (--profileIndex > 0) {
    if (str == ProfileInfo[profileIndex].m_Name)
      break;
  }

  PluginCodec_Utilities::Change(ProfileInfo[profileIndex].m_H241, original, changed, H241ProfilesName);

  // get the current level 
  str = original[LevelName];
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
  unsigned maxWidth = original.GetUnsigned(PLUGINCODEC_OPTION_MAX_RX_FRAME_WIDTH);
  unsigned maxHeight = original.GetUnsigned(PLUGINCODEC_OPTION_MAX_RX_FRAME_HEIGHT);
  unsigned maxFrameSizeInMB = PluginCodec_Utilities::GetMacroBlocks(maxWidth, maxHeight);
  if (maxFrameSizeInMB > 0) {
    while (levelIndex > 0 && maxFrameSizeInMB < LevelInfo[levelIndex].m_MaxFrameSize)
      --levelIndex;
  }
  PTRACE(5, MY_CODEC_LOG, "Max resolution " << maxWidth << 'x' << maxHeight << " selected index " << levelIndex);

  // set the new level
  PluginCodec_Utilities::Change(LevelInfo[levelIndex].m_H241, original, changed, H241LevelName);

  // Calculate SDP parameters from the adjusted profile/level
  char sdpProfLevel[7];
  sprintf(sdpProfLevel, "%02x%02x%02x",
          ProfileInfo[profileIndex].m_H264,
          ProfileInfo[profileIndex].m_Constraints | LevelInfo[levelIndex].m_constraints,
          LevelInfo[levelIndex].m_H264);
  PluginCodec_Utilities::Change(sdpProfLevel, original, changed, SDPProfileAndLevelName);

  // Clamp other variables (width/height etc) according to the adjusted profile/level
  ClampSizes(LevelInfo[levelIndex], maxWidth, maxHeight, maxFrameSizeInMB, original, changed);

  // Do this afer the clamping, maxFrameSizeInMB may change
  if (maxFrameSizeInMB > LevelInfo[levelIndex].m_MaxFrameSize) {
    PluginCodec_Utilities::Change(maxFrameSizeInMB, original, changed, MaxFS_SDP_Name);
    PluginCodec_Utilities::Change((maxFrameSizeInMB+255)/256, original, changed, MaxFS_H241_Name);
  }

  // Set exception to bit rate if necessary
  unsigned bitRate = original.GetUnsigned(PLUGINCODEC_OPTION_MAX_BIT_RATE);
  if (bitRate > LevelInfo[levelIndex].m_MaxBitRate) {
    PluginCodec_Utilities::Change((bitRate+999)/1000, original, changed, MaxBR_SDP_Name);
    PluginCodec_Utilities::Change((bitRate+24999)/25000, original, changed, MaxBR_H241_Name);
  }

  // Set exception to frame rate if necessary
  unsigned mbps = maxFrameSizeInMB*PLUGINCODEC_VIDEO_CLOCK/original.GetUnsigned(PLUGINCODEC_OPTION_FRAME_TIME);
  if (mbps > LevelInfo[levelIndex].m_MaxMBPS) {
    PluginCodec_Utilities::Change(mbps, original, changed, MaxMBPS_SDP_Name);
    PluginCodec_Utilities::Change((mbps+499)/500, original, changed, MaxMBPS_H241_Name);
  }

  return true;
}


/////////////////////////////////////////////////////////////////////////////

#ifndef OPAL_PLUGIN_COMPILE

// Now put the tracing  back again
#undef PTRACE
#define PTRACE(level, args) PTRACE2(level, PTraceObjectInstance(), args)

#include <opal/mediafmt.h>
#include <h323/h323caps.h>


class OpalH264Format : public OpalVideoFormatInternal
{
  public:
    OpalH264Format(const char * formatName, const char * mediaPacketizations)
      : OpalVideoFormatInternal(formatName, RTP_DataFrame::DynamicBase, H264EncodingName,
                                PVideoFrameInfo::MaxWidth, PVideoFrameInfo::MaxHeight, 30, 16000000)
    {
      OpalMediaOption * option;
#if OPAL_H323
      OpalMediaOption::H245GenericInfo info;
#endif

      static const char * const profiles[] = {
        H264_PROFILE_STR_BASELINE,
        H264_PROFILE_STR_MAIN,
        H264_PROFILE_STR_EXTENDED
      };
      AddOption(option = new OpalMediaOptionEnum(ProfileName, false, profiles, PARRAYSIZE(profiles), OpalMediaOption::MinMerge));

      static const char * const levels[] = {
        H264_LEVEL_STR_1,
        H264_LEVEL_STR_1_b,
        H264_LEVEL_STR_1_1,
        H264_LEVEL_STR_1_2,
        H264_LEVEL_STR_1_3,
        H264_LEVEL_STR_2,
        H264_LEVEL_STR_2_1,
        H264_LEVEL_STR_2_2,
        H264_LEVEL_STR_3,
        H264_LEVEL_STR_3_1,
        H264_LEVEL_STR_3_2,
        H264_LEVEL_STR_4,
        H264_LEVEL_STR_4_1,
        H264_LEVEL_STR_4_2,
        H264_LEVEL_STR_5,
        H264_LEVEL_STR_5_1
      };
      AddOption(option = new OpalMediaOptionEnum(LevelName, false, levels, PARRAYSIZE(levels), OpalMediaOption::MinMerge, PARRAYSIZE(levels)-1));

#if OPAL_H323
      option = new OpalMediaOptionUnsigned(H241ProfilesName, true, OpalMediaOption::MinMerge, DefaultProfileH241, 1, 127);
      option->SetH245Generic(OpalMediaOption::H245GenericInfo(H241_PROFILES));
      AddOption(option);

      option = new OpalMediaOptionUnsigned(H241LevelName, true, OpalMediaOption::MinMerge, DefaultLevelH241, 15, 113);
      option->SetH245Generic(OpalMediaOption::H245GenericInfo(H241_LEVEL));
      AddOption(option);

      option = new OpalMediaOptionUnsigned(MaxMBPS_H241_Name, true, OpalMediaOption::MinMerge, 0, 0, 1966);
      option->SetH245Generic(OpalMediaOption::H245GenericInfo(H241_CustomMaxMBPS));
      AddOption(option);

      option = new OpalMediaOptionUnsigned(MaxFS_H241_Name, true, OpalMediaOption::MinMerge, 0, 0, 144);
      option->SetH245Generic(OpalMediaOption::H245GenericInfo(H241_CustomMaxFS));
      AddOption(option);

      option = new OpalMediaOptionUnsigned(MaxBR_H241_Name, true, OpalMediaOption::MinMerge, 0, 0, 9600);
      option->SetH245Generic(OpalMediaOption::H245GenericInfo(H241_CustomMaxBRandCPB));
      AddOption(option);
#endif

#if OPAL_SIP
      option = new OpalMediaOptionOctets(SDPProfileAndLevelName, true, OpalMediaOption::NoMerge, false);
      option->SetFMTP(SDPProfileAndLevelFMTPName, SDPProfileAndLevelFMTPDflt);
      option->FromString(DefaultSDPProfileAndLevel);
      AddOption(option);

      option = new OpalMediaOptionUnsigned(MaxMBPS_SDP_Name, true, OpalMediaOption::MinMerge, 0, 0, 983040);
      option->SetFMTP(MaxMBPS_FMTPName, "0");
      AddOption(option);

      option = new OpalMediaOptionUnsigned(MaxFS_SDP_Name, true, OpalMediaOption::MinMerge, 0, 0, 36864);
      option->SetFMTP(MaxFS_FMTPName, "0");
      AddOption(option);

      option = new OpalMediaOptionUnsigned(MaxBR_SDP_Name, true, OpalMediaOption::MinMerge, 0, 0, 240000);
      option->SetFMTP(MaxBR_FMTPName, "0");
      AddOption(option);
#endif

      option = new OpalMediaOptionUnsigned(MaxNALUSizeName, false, OpalMediaOption::MinMerge, H241_MAX_NALU_SIZE, 256, 240000);
      OPAL_SET_MEDIA_OPTION_H245(option, H241_Max_NAL_unit_size);
      OPAL_SET_MEDIA_OPTION_FMTP(option, MaxNALUSizeFMTPName, STRINGIZE(H241_MAX_NALU_SIZE));
      AddOption(option);

      if (mediaPacketizations[1] != '\0')
        AddOption(new OpalMediaOptionString(PLUGINCODEC_MEDIA_PACKETIZATIONS, false, mediaPacketizations));
#if OPAL_SIP
      else {
        option = new OpalMediaOptionUnsigned(PacketizationModeName, true, OpalMediaOption::EqualMerge, atoi(mediaPacketizations), 0, 2);
        option->SetFMTP(PacketizationFMTPName, "0");
        AddOption(option);
      }
#endif
    }

    void GetOriginalOptions(PluginCodec_OptionMap & original)
    {
      PWaitAndSignal m1(media_format_mutex);
      for (PINDEX i = 0; i < options.GetSize(); i++)
        original[options[i].GetName()] = options[i].AsString().GetPointer();
    }

    void SetChangedOptions(const PluginCodec_OptionMap & changed)
    {
      for (PluginCodec_OptionMap::const_iterator it = changed.begin(); it != changed.end(); ++it)
        SetOptionValue(it->first, it->second);
    }

    virtual bool ToNormalisedOptions()
    {
      PluginCodec_OptionMap original, changed;
      GetOriginalOptions(original);
      if (!MyToNormalised(original, changed))
        return false;
      SetChangedOptions(changed);
      return OpalVideoFormatInternal::ToNormalisedOptions();
    }

    virtual bool ToCustomisedOptions()
    {
      PluginCodec_OptionMap original, changed;
      GetOriginalOptions(original);
      if (!MyToCustomised(original, changed))
        return false;
      SetChangedOptions(changed);
      return OpalVideoFormatInternal::ToCustomisedOptions();
    }
};


#if OPAL_H323
extern const char H264_Identifer[] = OpalPluginCodec_Identifer_H264_Generic;
#endif



#if OPAL_H323
const OpalVideoFormat & GetOpalH264()
{
  static OpalVideoFormat const format(new OpalH264Format(
    H264FormatName,
    OpalPluginCodec_Identifer_H264_Aligned","
    OpalPluginCodec_Identifer_H264_NonInterleaved","
    OpalPluginCodec_Identifer_H264_Truncated // Some stupid endpoints (e.g. Polycom) use this, one zero short!
  ));

  static H323CapabilityFactory::Worker<
    H323GenericVideoCapabilityTemplate<H264_Identifer, GetOpalH264>
  > capability(H264FormatName, true);

  return format;
}
#endif // OPAL_H323


#if OPAL_SIP
const OpalVideoFormat & GetOpalH264_MODE0()
{
  static OpalVideoFormat const format(new OpalH264Format(H2640FormatName, "0"));
  return format;
}


const OpalVideoFormat & GetOpalH264_MODE1()
{
  static OpalVideoFormat const format(new OpalH264Format(H2641FormatName, "1"));
  return format;
}
#endif // OPAL_SIP


#endif // OPAL_PLUGIN_COMPILE


// End of File ///////////////////////////////////////////////////////////////
