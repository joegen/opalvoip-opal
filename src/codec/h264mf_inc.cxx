/*
 * h264mf_inc.cxx
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
 */

#include <codec/opalplugin.hpp>
#include <codec/known.h>

#include <stdio.h>
#include <vector>


///////////////////////////////////////////////////////////////////////////////

#ifdef MY_CODEC
  #define MY_CODEC_LOG STRINGIZE(MY_CODEC)
#else
  #define MY_CODEC_LOG "H.264"
#endif

static const char H264_Mode0_FormatName[] = OPAL_H264_MODE0;
static const char H264_Mode1_FormatName[] = OPAL_H264_MODE1;
static const char H264_High_FormatName[]  = OPAL_H264_High;
static const char H264_Flash_FormatName[] = OPAL_H264_Flash;

static const char H264EncodingName[] = "H264";
static const char H264FlashEncodingName[] = "H264F";

// Level 3 allows 4CIF at 25fps
#define DefaultProfileStr          H264_PROFILE_STR_BASELINE
#define DefaultProfileInt          H264_PROFILE_INT_BASELINE
#define DefaultProfileH241         64
#define DefaultLevelStr            H264_LEVEL_STR_4
#define DefaultLevelInt            31
#define DefaultLevelH241           71
#define DefaultSDPProfileAndLevel  "42801f"

#define DefaultSendAccessUnitDelimiters 0  // Many endpoints don't seem to like these, initially false

#define H241_MAX_NALU_SIZE   1400   // From H.241/8.3.2.10

#define H264_PROFILE_STR_BASELINE  "Baseline"
#define H264_PROFILE_STR_MAIN      "Main"
#define H264_PROFILE_STR_EXTENDED  "Extended"
#define H264_PROFILE_STR_HIGH      "High"

#define H264_PROFILE_INT_BASELINE  66
#define H264_PROFILE_INT_MAIN      77
#define H264_PROFILE_INT_EXTENDED  88
#define H264_PROFILE_INT_HIGH      100

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
#define H264_LEVEL_STR_5_2  "5.2"


enum
{
    H241_PROFILES                      = 41 | PluginCodec_H245_Collapsing | PluginCodec_H245_TCS | PluginCodec_H245_OLC | PluginCodec_H245_ReqMode | PluginCodec_H245_BooleanArray | (1 << PluginCodec_H245_PositionShift),
    H241_LEVEL                         = 42 | PluginCodec_H245_Collapsing | PluginCodec_H245_TCS | PluginCodec_H245_OLC | PluginCodec_H245_ReqMode                                 | (2 << PluginCodec_H245_PositionShift),
    H241_CustomMaxMBPS                 =  3 | PluginCodec_H245_Collapsing | PluginCodec_H245_TCS | PluginCodec_H245_OLC | PluginCodec_H245_ReqMode,
    H241_CustomMaxFS                   =  4 | PluginCodec_H245_Collapsing | PluginCodec_H245_TCS | PluginCodec_H245_OLC | PluginCodec_H245_ReqMode,
    H241_CustomMaxDPB                  =  5 | PluginCodec_H245_Collapsing | PluginCodec_H245_TCS | PluginCodec_H245_OLC | PluginCodec_H245_ReqMode,
    H241_CustomMaxBRandCPB             =  6 | PluginCodec_H245_Collapsing | PluginCodec_H245_TCS | PluginCodec_H245_OLC | PluginCodec_H245_ReqMode,
    H241_CustomMaxSMBPS                =  7 | PluginCodec_H245_Collapsing | PluginCodec_H245_TCS | PluginCodec_H245_OLC | PluginCodec_H245_ReqMode,
    H241_Max_RCMD_NALU_size            =  8 | PluginCodec_H245_Collapsing | PluginCodec_H245_TCS | PluginCodec_H245_OLC | PluginCodec_H245_ReqMode,
    H241_Max_NAL_unit_size             =  9 | PluginCodec_H245_Collapsing | PluginCodec_H245_TCS | PluginCodec_H245_OLC | PluginCodec_H245_ReqMode,
    H241_SampleAspectRatiosSupported   = 10 | PluginCodec_H245_Collapsing | PluginCodec_H245_TCS | PluginCodec_H245_OLC | PluginCodec_H245_ReqMode,
    H241_AdditionalModesSupported      = 11 | PluginCodec_H245_Collapsing | PluginCodec_H245_TCS | PluginCodec_H245_OLC | PluginCodec_H245_ReqMode | PluginCodec_H245_BooleanArray,
    H241_AdditionalDisplayCapabilities = 12 | PluginCodec_H245_Collapsing | PluginCodec_H245_TCS | PluginCodec_H245_OLC | PluginCodec_H245_ReqMode | PluginCodec_H245_BooleanArray,
};


static char const ProfileName[] = PLUGINCODEC_OPTION_PROFILE;
static char const LevelName[] = PLUGINCODEC_OPTION_LEVEL;
static char const MaxNALUSizeName[] = PLUGINCODEC_OPTION_MAX_NALU_SIZE;
static char const ConstraintFlagsName[] = "Constraint Flags";

static char const H241ProfilesName[] = "H.241 Profile Mask";
static char const H241LevelName[] = "H.241 Level";
static char const MaxMBPS_H241_Name[] = "H.241 Max MBPS";
static char const MaxSMBPS_H241_Name[] = "H.241 Max SMBPS";
static char const MaxFS_H241_Name[] = "H.241 Max FS";
static char const MaxBR_H241_Name[] = "H.241 Max BR";
static char const H241ForcedName[] = "H.241 Forced";

static char const SDPProfileAndLevelName[] = "SIP/SDP Profile & Level";
static char const SDPProfileAndLevelFMTPName[] = "profile-level-id";
static char const SDPProfileAndLevelFMTPDflt[] = "42000A";
static char const MaxMBPS_SDP_Name[] = "SIP/SDP Max MBPS";
static char const MaxMBPS_FMTPName[] = "max-mbps";
static char const MaxSMBPS_SDP_Name[] = "SIP/SDP Max SMBPS";
static char const MaxSMBPS_FMTPName[] = "max-smbps";
static char const MaxFS_SDP_Name[] = "SIP/SDP Max FS";
static char const MaxFS_FMTPName[] = "max-fs";
static char const MaxBR_SDP_Name[] = "SIP/SDP Max BR";
static char const MaxBR_FMTPName[] = "max-br";
static char const SDPForcedName[] = "SDP Forced";

static char const MaxNALUSizeFMTPName[] = "max-rcmd-nalu-size";
static const char PacketizationModeName[] = PLUGINCODEC_OPTION_H264_PACKET_MODE;
static const char PacketizationFMTPName[] = "packetization-mode";


/* Need to make missing MBPS parameter larger than all possible values so when
   "min merge" is done a real value always take precedence. Thus the max
   value defined here is actually one larger than the real max value */
#define MAX_MBPS_SDP  983041
#define MAX_MBPS_H241 1967
#define MAX_FS_SDP    36865
#define MAX_FS_H241   145
#define MAX_BR_SDP    240001
#define MAX_BR_H241   9601

#define SCALE_MBPS_H241 500
#define SCALE_FS_H241   256
#define SCALE_BR_H241   30000
#define SCALE_BR_SDP    1000


static struct {
  char     m_Name[9];
  unsigned m_H264;
  unsigned m_H241;
} const ProfileInfo[] = {
  { H264_PROFILE_STR_BASELINE, H264_PROFILE_INT_BASELINE, 64 },
  { H264_PROFILE_STR_MAIN,     H264_PROFILE_INT_MAIN,     32 },
  { H264_PROFILE_STR_EXTENDED, H264_PROFILE_INT_EXTENDED, 16 },
  { H264_PROFILE_STR_HIGH,     H264_PROFILE_INT_HIGH,      8 }
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
  //                     Lev  Con  H241    FS   W/H     MBPS             BR
  { H264_LEVEL_STR_1,    10, 0x00,  15,    99,  448,    1485,     64 * 1200 },
  { H264_LEVEL_STR_1_b,  11, 0x10,  19,    99,  448,    1485,    128 * 1200 },
  { H264_LEVEL_STR_1_1,  11, 0x00,  22,   396,  896,    3000,    192 * 1200 },
  { H264_LEVEL_STR_1_2,  12, 0x00,  29,   396,  896,    6000,    384 * 1200 },
  { H264_LEVEL_STR_1_3,  13, 0x00,  36,   396,  896,   11880,    768 * 1200 },
  { H264_LEVEL_STR_2,    20, 0x00,  43,   396,  896,   11880,   2000 * 1200 },
  { H264_LEVEL_STR_2_1,  21, 0x00,  50,   792, 1264,   19800,   4000 * 1200 },
  { H264_LEVEL_STR_2_2,  22, 0x00,  57,  1620, 1808,   20250,   4000 * 1200 },
  { H264_LEVEL_STR_3,    30, 0x00,  64,  1620, 1808,   40500,  10000 * 1200 },
  { H264_LEVEL_STR_3_1,  31, 0x00,  71,  3600, 2704,  108000,  14000 * 1200 },
  { H264_LEVEL_STR_3_2,  32, 0x00,  78,  5120, 3232,  216000,  20000 * 1200 },
  { H264_LEVEL_STR_4,    40, 0x00,  85,  8192, 4096,  245760,  20000 * 1200 },
  { H264_LEVEL_STR_4_1,  41, 0x00,  92,  8192, 4096,  245760,  50000 * 1200 },
  { H264_LEVEL_STR_4_2,  42, 0x00,  99,  8704, 4208,  522240,  50000 * 1200 },
  { H264_LEVEL_STR_5,    50, 0x00, 106, 22080, 6720,  589824, 135000 * 1200 },
  { H264_LEVEL_STR_5_1,  51, 0x00, 113, 36864, 8320,  983040, 240000 * 1200 },
  { H264_LEVEL_STR_5_2,  52, 0x00, 120, 36864, 8320, 2073600, 240000 * 1200 },
};


static const size_t FlashHeaderSize = 5;
static const uint8_t FlashSPS_PPS[FlashHeaderSize] = { 0x17, 0, 0, 0, 0 };


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

static bool MyToNormalised(PluginCodec_OptionMap & original, PluginCodec_OptionMap & changed)
{
  size_t levelIndex = 0;
  size_t profileIndex = sizeof(ProfileInfo)/sizeof(ProfileInfo[0]);
  unsigned maxMBPS;
  unsigned maxSMBPS;
  unsigned maxFrameSizeInMB;
  unsigned maxBitRate;

  if (original[PLUGINCODEC_OPTION_PROTOCOL] == PLUGINCODEC_OPTION_PROTOCOL_H323) {
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

    maxMBPS = (original.GetUnsigned(MaxMBPS_H241_Name)%MAX_MBPS_H241)*SCALE_MBPS_H241;
    maxSMBPS = (original.GetUnsigned(MaxSMBPS_H241_Name)%MAX_MBPS_H241)*SCALE_MBPS_H241;
    maxFrameSizeInMB = (original.GetUnsigned(MaxFS_H241_Name)%MAX_FS_H241)*SCALE_FS_H241;
    maxBitRate = (original.GetUnsigned(MaxBR_H241_Name)%MAX_BR_H241)*SCALE_BR_H241;

    if (maxMBPS > 0)
      PluginCodec_Utilities::Change(maxMBPS, original, changed, MaxMBPS_SDP_Name); 
    if (maxSMBPS > 0)
      PluginCodec_Utilities::Change(maxSMBPS, original, changed, MaxSMBPS_SDP_Name);
    if (maxFrameSizeInMB > 0)
      PluginCodec_Utilities::Change(maxFrameSizeInMB, original, changed, MaxFS_SDP_Name);
    if (maxBitRate > 0)
      PluginCodec_Utilities::Change((maxBitRate+SCALE_BR_SDP-1)/SCALE_BR_SDP, original, changed, MaxBR_SDP_Name); 
  }
  else if (original[PLUGINCODEC_OPTION_PROTOCOL] == PLUGINCODEC_OPTION_PROTOCOL_SIP) {
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
    PluginCodec_Utilities::Change(sdpConstraints, original, changed, ConstraintFlagsName);

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
      if ((sdpConstraints & LevelInfo[levelIndex].m_constraints) == LevelInfo[levelIndex].m_constraints)
        break;
    }
    maxMBPS = original.GetUnsigned(MaxMBPS_SDP_Name)%MAX_MBPS_SDP;
    maxSMBPS = original.GetUnsigned(MaxSMBPS_SDP_Name)%MAX_MBPS_SDP;
    maxFrameSizeInMB = original.GetUnsigned(MaxFS_SDP_Name)%MAX_FS_SDP;
    maxBitRate = (original.GetUnsigned(MaxBR_SDP_Name)%MAX_BR_SDP)*SCALE_BR_SDP;

    if (maxMBPS > 0)
      PluginCodec_Utilities::Change((maxMBPS+SCALE_MBPS_H241-1)/SCALE_MBPS_H241, original, changed, MaxMBPS_H241_Name); 
    if (maxSMBPS > 0)
      PluginCodec_Utilities::Change((maxSMBPS+SCALE_MBPS_H241-1)/SCALE_MBPS_H241, original, changed, MaxSMBPS_H241_Name);
    if (maxFrameSizeInMB > 0)
      PluginCodec_Utilities::Change((maxFrameSizeInMB+SCALE_FS_H241-1)/SCALE_FS_H241, original, changed, MaxFS_H241_Name); 
    if (maxBitRate > 0)
      PluginCodec_Utilities::Change((maxBitRate+SCALE_BR_H241-1)/SCALE_BR_H241, original, changed, MaxBR_H241_Name); 
  }
  else {
    std::string profileName = original[ProfileName];
    while (--profileIndex > 0) {
      if (profileName == ProfileInfo[profileIndex].m_Name)
        break;
    }

    std::string levelName = original[LevelName];
    for (; levelIndex < sizeof(LevelInfo)/sizeof(LevelInfo[0])-1; ++levelIndex) {
      if (levelName == LevelInfo[levelIndex].m_Name)
        break;
    }

    maxMBPS = 0;
    maxFrameSizeInMB = 0;
    maxBitRate = 0;
  }

  PluginCodec_Utilities::Change(ProfileInfo[profileIndex].m_Name, original, changed, ProfileName); 
  PluginCodec_Utilities::Change(LevelInfo[levelIndex].m_Name, original, changed, LevelName);

  if (maxFrameSizeInMB < LevelInfo[levelIndex].m_MaxFrameSize)
    maxFrameSizeInMB = LevelInfo[levelIndex].m_MaxFrameSize;
  PluginCodec_Utilities::ClampResolution(original, changed,
                                         original.GetUnsigned(PLUGINCODEC_OPTION_MAX_RX_FRAME_WIDTH),
                                         original.GetUnsigned(PLUGINCODEC_OPTION_MAX_RX_FRAME_HEIGHT),
                                         maxFrameSizeInMB);

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
  if ((original.GetUnsigned(SDPForcedName)  != 0 && original[PLUGINCODEC_OPTION_PROTOCOL] == PLUGINCODEC_OPTION_PROTOCOL_SIP) ||
      (original.GetUnsigned(H241ForcedName) != 0 && original[PLUGINCODEC_OPTION_PROTOCOL] == PLUGINCODEC_OPTION_PROTOCOL_H323)) {
    PTRACE(4, MY_CODEC_LOG, "Forcing " << original[PLUGINCODEC_OPTION_PROTOCOL] << " parameters, not customising");
    return true;
  }

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

  size_t levelIndex = sizeof(LevelInfo)/sizeof(LevelInfo[0])-1;
  while (levelIndex > 0 && str != LevelInfo[levelIndex].m_Name)
    --levelIndex;

  /* While we have selected the desired level, we may need to adjust it
      further due to resolution restrictions. This is due to the fact that
      we have no other mechnism to prevent the remote from sending, say,
      CIF when we only support QCIF.
    */
  unsigned maxWidth = original.GetUnsigned(PLUGINCODEC_OPTION_MAX_RX_FRAME_WIDTH);
  unsigned maxHeight = original.GetUnsigned(PLUGINCODEC_OPTION_MAX_RX_FRAME_HEIGHT);
  unsigned frameTime = original.GetUnsigned(PLUGINCODEC_OPTION_FRAME_TIME);
  unsigned maxMacroBlocks = PluginCodec_Utilities::GetMacroBlocks(maxWidth, maxHeight);
  unsigned macroBlocksPerSecond = maxMacroBlocks*PLUGINCODEC_VIDEO_CLOCK/frameTime;
  if (maxMacroBlocks > 0) {
    while (levelIndex > 0 &&
            (LevelInfo[levelIndex].m_MaxFrameSize > maxMacroBlocks ||
             LevelInfo[levelIndex].m_MaxMBPS > macroBlocksPerSecond))
      --levelIndex;
    PTRACE(4, MY_CODEC_LOG, "Max resolution " << maxWidth << 'x' << maxHeight
           << " and frame rate " << frameTime << " (" << (PLUGINCODEC_VIDEO_CLOCK/frameTime) << "fps)"
              " selected index " << levelIndex);
  }
  else {
    PTRACE(4, MY_CODEC_LOG, "Level \"" << str << "\" selected index " << levelIndex);
  }

  // set the new level
  PluginCodec_Utilities::Change(LevelInfo[levelIndex].m_H241, original, changed, H241LevelName);

  // Calculate SDP parameters from the adjusted profile/level
  char sdpProfLevel[3*8*2+1];
  sprintf(sdpProfLevel, "%02x%02x%02x",
          ProfileInfo[profileIndex].m_H264,
          original.GetUnsigned(ConstraintFlagsName) | LevelInfo[levelIndex].m_constraints,
          LevelInfo[levelIndex].m_H264);
  PluginCodec_Utilities::Change(sdpProfLevel, original, changed, SDPProfileAndLevelName);

  // Clamp other variables (width/height etc) according to the adjusted profile/level
  PluginCodec_Utilities::ClampResolution(original, changed, maxWidth, maxHeight, maxMacroBlocks);

  // Do this afer the clamping, maxFrameSizeInMB may change
  if (maxMacroBlocks > LevelInfo[levelIndex].m_MaxFrameSize) {
    PluginCodec_Utilities::ClampMax(maxMacroBlocks, original, changed, MaxFS_SDP_Name, true);
    PluginCodec_Utilities::ClampMax((maxMacroBlocks+SCALE_FS_H241-1)/SCALE_FS_H241, original, changed, MaxFS_H241_Name, true);
  }
  else {
    // Use max value to prevent inclusion of parameter
    PluginCodec_Utilities::Change(MAX_FS_SDP, original, changed, MaxFS_SDP_Name);
    PluginCodec_Utilities::Change(MAX_FS_H241, original, changed, MaxFS_H241_Name);
  }

  // Set exception to bit rate if necessary
  unsigned bitRate = original.GetUnsigned(PLUGINCODEC_OPTION_MAX_BIT_RATE);
  if (bitRate > LevelInfo[levelIndex].m_MaxBitRate) {
    PluginCodec_Utilities::ClampMax((bitRate+SCALE_BR_SDP-1)/SCALE_BR_SDP, original, changed, MaxBR_SDP_Name, true);
    PluginCodec_Utilities::ClampMax((bitRate+SCALE_BR_H241-1)/SCALE_BR_H241, original, changed, MaxBR_H241_Name, true);
  }
  else {
    // Use max value to prevent inclusion of parameter
    PluginCodec_Utilities::Change(MAX_BR_SDP, original, changed, MaxBR_SDP_Name);
    PluginCodec_Utilities::Change(MAX_BR_H241, original, changed, MaxBR_H241_Name);
  }

  // Set exception to frame rate if necessary
  if (macroBlocksPerSecond > LevelInfo[levelIndex].m_MaxMBPS) {
    PluginCodec_Utilities::ClampMax(macroBlocksPerSecond, original, changed, MaxMBPS_SDP_Name, true);
    PluginCodec_Utilities::ClampMax((macroBlocksPerSecond+SCALE_MBPS_H241-1)/SCALE_MBPS_H241, original, changed, MaxMBPS_H241_Name, true);
  }
  else {
    // Use max value to prevent inclusion of parameter
    PluginCodec_Utilities::Change(MAX_MBPS_SDP, original, changed, MaxMBPS_SDP_Name);
    PluginCodec_Utilities::Change(MAX_MBPS_H241, original, changed, MaxMBPS_H241_Name);
  }

  return true;
}


class H264FlashPacketizer
{
  protected:
    std::vector<unsigned char> m_naluBuffer;
    bool m_firstFrame;

    H264FlashPacketizer()
      : m_firstFrame(true)
    {
    }

    virtual ~H264FlashPacketizer()
    {
    }


    virtual bool GetNALU(const void * fromPtr, unsigned & fromLen, const uint8_t * & naluPtr, unsigned & naluLen, unsigned & flags) = 0;

    static const size_t HeaderSize = 5+5+6+4;

    bool FlashTranscode(const void * fromPtr,
                          unsigned & fromLen,
                              void * toPtr,
                          unsigned & toLen,
                          unsigned & flags)
    {
      PluginCodec_RTP rtp(toPtr, toLen);
      uint8_t * pBuffer = rtp.GetPayloadPtr();

      const uint8_t * naluPtr;
      unsigned naluLen;
      if (!GetNALU(fromPtr, fromLen, naluPtr, naluLen, flags))
        return false;

      bool bKey = (flags&PluginCodec_ReturnCoderIFrame) != 0;

      if (bKey && (naluPtr[0] & 0x1f) == 0x07) { // SPS
        toLen -= 5+5+3+3;
        if (naluLen > toLen)
          return false;

        /* Pre-amble */
        *pBuffer++ = 0x17;
        *pBuffer++ = 0x00;
        *pBuffer++ = 0x00;
        *pBuffer++ = 0x00;
        *pBuffer++ = 0x00;

        /* Start header */
        *pBuffer++ = 0x01; /* version */
        *pBuffer++ = naluPtr[1]; /* profile */
        *pBuffer++ = naluPtr[2]; /* profile compat */
        *pBuffer++ = naluPtr[3]; /* level */
        *pBuffer++ = 0xFF; /* 6 bits reserved (111111) + 2 bits nal size length - 1 (11) */

        /* Write SPSs */
        *pBuffer++ = 0xE1; // Only 1 for now
        *pBuffer++ = (uint8_t)(naluLen >> 8);
        *pBuffer++ = (uint8_t) naluLen;
        memcpy(pBuffer, naluPtr, naluLen);
        pBuffer += naluLen;
        toLen -= naluLen;

        // We assume next thing is SPS
        if (!GetNALU(fromPtr, fromLen, naluPtr, naluLen, flags))
            return false;

        if (naluLen > toLen)
          return false;

        /* Write PPSs */
        *pBuffer++ = 0x01; // Only 1 for now
        *pBuffer++ = (uint8_t)(naluLen >> 8);
        *pBuffer++ = (uint8_t) naluLen;
        memcpy(pBuffer, naluPtr, naluLen);
        pBuffer += naluLen;
      }
      else {
        toLen -= HeaderSize;
        if (naluLen > toLen)
          return false;

        /* Pre-amble */
        *pBuffer++ = bKey ? 0x17 : 0x27;
        *pBuffer++ = 0x01;
        *pBuffer++ = 0x00;
        *pBuffer++ = 0x00;
        *pBuffer++ = 0x00;

        if (bKey && !m_firstFrame) {
          /* End of previous GOP */
          *pBuffer++ = 0x00;
          *pBuffer++ = 0x00;
          *pBuffer++ = 0x00;
          *pBuffer++ = 0x01;
          *pBuffer++ = 0x0A;
        }

        /* Access unit delimiter */
        *pBuffer++ = 0x00;
        *pBuffer++ = 0x00;
        *pBuffer++ = 0x00;
        *pBuffer++ = 0x02;
        *pBuffer++ = 0x09;
        *pBuffer++ = bKey ? 0x10 : 0x30;

        for (;;) {
          *pBuffer++ = (uint8_t)(naluLen >> 24);
          *pBuffer++ = (uint8_t)(naluLen >> 16);
          *pBuffer++ = (uint8_t)(naluLen >>  8);
          *pBuffer++ = (uint8_t) naluLen;
          memcpy(pBuffer, naluPtr, naluLen);
          pBuffer += naluLen;
          toLen -= naluLen+4;

          if ((flags&PluginCodec_ReturnCoderLastFrame) != 0)
            break;

          if (!GetNALU(fromPtr, fromLen, naluPtr, naluLen, flags))
            return false;
          if (naluLen+4 > toLen)
            return false;
        }

        m_firstFrame = false;
      }

      if (!rtp.SetPayloadSize(pBuffer - rtp.GetPayloadPtr()))
        return false;

      toLen = (unsigned)rtp.GetPacketSize();
      return true;
    }
};


// End of File ///////////////////////////////////////////////////////////////
