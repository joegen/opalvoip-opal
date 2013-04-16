/*
 * mpegmf_inc.cxx
 *
 * MPEG4 part 2 Media Format descriptions
 *
 * Open Phone Abstraction Library
 *
 * Copyright (c) 2012 Vox Lucida Pty. Ltd.
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
 * $Revision: 28055 $
 * $Author: rjongbloed $
 * $Date: 2012-07-18 18:02:09 +1000 (Wed, 18 Jul 2012) $
 */

#include <codec/opalplugin.hpp>
#include <codec/known.h>

#include <stdio.h>


///////////////////////////////////////////////////////////////////////////////

#ifdef MY_CODEC
  #define MY_CODEC_LOG STRINGIZE(MY_CODEC)
#else
  #define MY_CODEC_LOG "MPEG4"
#endif

static const char MPEG4FormatName[] = OPAL_MPEG4;
static const char MPEG4EncodingName[] = "MP4V-ES";
static const char ProfileAndLevel[] = "Profile & Level";
static const char ProfileAndLevel_SDP[] = "profile-level-id";
static const char Object_SDP[] = "object";
static const char H245ObjectId[] = "H.245 Object Id";
static const char DCI[] = "DCI";
static const char DCI_SDP[] = "config";

static struct mpeg4_profile_level {
    unsigned profileLevel;
    const char* profileName;
    unsigned profileNumber;
    unsigned level;
    unsigned maxQuantTables;       /* Max. unique quant. tables */
    unsigned maxVMVBufferSize;     /* max. VMV buffer size(MB units) */
    unsigned frame_size;           /* max. VCV buffer size (MB) */
    unsigned mbps;                 /* VCV decoder rate (MB/s) 4 */
    unsigned boundaryMbps;         /* VCV boundary MB decoder rate (MB/s)9 */
    unsigned maxBufferSize;        /* max. total VBV buffer size (units of 16384 bits) */
    unsigned maxVOLBufferSize;     /* max. VOL VBV buffer size (units of 16384 bits) */
    unsigned maxVideoPacketLength; /* max. video packet length (bits) */
    long unsigned bitrate;
} const mpeg4_profile_levels[] = {
    {   1, "Simple",                     1, 1, 1,   198,    99,   1485,      0,  10,  10,  2048,    64000 },
    {   2, "Simple",                     1, 2, 1,   792,   396,   5940,      0,  40,  40,  4096,   128000 },
    {   3, "Simple",                     1, 3, 1,   792,   396,  11880,      0,  40,  40,  8192,   384000 },
    {   4, "Simple",                     1, 4, 1,  2400,  1200,  36000,      0,  80,  80, 16384,  4000000 }, // is really 4a
    {   5, "Simple",                     1, 5, 1,  3240,  1620,  40500,      0, 112, 112, 16384,  8000000 },
    {   8, "Simple",                     1, 0, 1,   198,    99,   1485,      0,  10,  10,  2048,    64000 },
    {   9, "Simple",                     1, 0, 1,   198,    99,   1485,      0,  20,  20,  2048,   128000 }, // 0b
    {  17, "Simple Scalable",            2, 1, 1,  1782,   495,   7425,      0,  40,  40,  2048,   128000 },
    {  18, "Simple Scalable",            2, 2, 1,  3168,   792,  23760,      0,  40,  40,  4096,   256000 },
    {  33, "Core",                       3, 1, 4,   594,   198,   5940,   2970,  16,  16,  4096,   384000 },
    {  34, "Core",                       3, 2, 4,  2376,   792,  23760,  11880,  80,  80,  8192,  2000000 },
    {  50, "Main",                       4, 2, 4,  3960,  1188,  23760,  11880,  80,  80,  8192,  2000000 },
    {  51, "Main",                       4, 3, 4, 11304,  3240,  97200,  48600, 320, 320, 16384, 15000000 },
    {  52, "Main",                       4, 4, 4, 65344, 16320, 489600, 244800, 760, 760, 16384, 38400000 },
    {  66, "N-Bit",                      5, 2, 4,  2376,   792,  23760,  11880,  80,  80,  8192,  2000000 },
    { 145, "Advanced Real Time Simple",  6, 1, 1,   198,    99,   1485,      0,  10,  10,  8192,    64000 },
    { 146, "Advanced Real Time Simple",  6, 2, 1,   792,   396,   5940,      0,  40,  40, 16384,   128000 },
    { 147, "Advanced Real Time Simple",  6, 3, 1,   792,   396,  11880,      0,  40,  40, 16384,   384000 },
    { 148, "Advanced Real Time Simple",  6, 4, 1,   792,   396,  11880,      0,  80,  80, 16384,  2000000 },
    { 161, "Core Scalable",              7, 1, 4,  2376,   792,  14850,   7425,  64,  64,  4096,   768000 },
    { 162, "Core Scalable",              7, 2, 4,  2970,   990,  29700,  14850,  80,  80,  4096,  1500000 },
    { 163, "Core Scalable",              7, 3, 4, 12906,  4032, 120960,  60480,  80,  80, 16384,  4000000 },
    { 177, "Advanced Coding Efficiency", 8, 1, 4,  1188,   792,  11880,   5940,  40,  40,  8192,   384000 },
    { 178, "Advanced Coding Efficiency", 8, 2, 4,  2376,  1188,  23760,  11880,  80,  80,  8192,  2000000 },
    { 179, "Advanced Coding Efficiency", 8, 3, 4,  9720,  3240,  97200,  48600, 320, 320, 16384, 15000000 },
    { 180, "Advanced Coding Efficiency", 8, 4, 4, 48960, 16320, 489600, 244800, 760, 760, 16384, 38400000 },
    { 193, "Advanced Core",              9, 1, 4,   594,   198,   5940,   2970,  16,   8,  4096,   384000 },
    { 194, "Advanced Core",              9, 2, 4,  2376,   792,  23760,  11880,  80,  40,  8192,  2000000 },
    { 240, "Advanced Simple",           10, 0, 1,   297,    99,   2970,    100,  10,  10,  2048,   128000 },
    { 241, "Advanced Simple",           10, 1, 1,   297,    99,   2970,    100,  10,  10,  2048,   128000 },
    { 242, "Advanced Simple",           10, 2, 1,  1188,   396,   5940,    100,  40,  40,  4096,   384000 },
    { 243, "Advanced Simple",           10, 3, 1,  1188,   396,  11880,    100,  40,  40,  4096,   768000 },
    { 244, "Advanced Simple",           10, 4, 1,  2376,   792,  23760,     50,  80,  80,  8192,  3000000 },
    { 245, "Advanced Simple",           10, 5, 1,  4860,  1620,  48600,     25, 112, 112, 16384,  8000000 },
    { 0 }
};

// This table is used in order to select a different resolution if the desired one
// consists of more macroblocks than the level limit
static struct mpeg4_resolution {
    unsigned width;
    unsigned height;
    unsigned macroblocks;
} const mpeg4_resolutions[] = {
    { 1920, 1088, 8160 },
    { 1600, 1200, 7500 },
    { 1408, 1152, 6336 },
    { 1280, 1024, 5120 },
    { 1280,  720, 3600 },
    { 1024,  768, 3072 },
    {  800,  600, 1900 },
    {  704,  576, 1584 },
    {  640,  480, 1200 },
    {  352,  288,  396 },
    {  320,  240,  300 },
    {  176,  144,   99 },
    {  128,   96,   48 },
    { 0 }
};


/////////////////////////////////////////////////////////////////////////////

enum
{
    H245_ANNEX_E_PROFILE_LEVEL = 0 | PluginCodec_H245_NonCollapsing | PluginCodec_H245_TCS | PluginCodec_H245_OLC | PluginCodec_H245_ReqMode,
    H245_ANNEX_E_OBJECT        = 1 | PluginCodec_H245_NonCollapsing |                        PluginCodec_H245_OLC | PluginCodec_H245_ReqMode,
    H245_ANNEX_E_DCI           = 2 | PluginCodec_H245_NonCollapsing |                        PluginCodec_H245_OLC,
    H245_ANNEX_E_DRAWING_ORDER = 3 | PluginCodec_H245_NonCollapsing |                        PluginCodec_H245_OLC,
    H245_ANNEX_E_VBCH          = 4 | PluginCodec_H245_Collapsing    | PluginCodec_H245_TCS | PluginCodec_H245_OLC | PluginCodec_H245_ReqMode,
};

static unsigned MergeProfileAndLevelOption(unsigned dstPL, unsigned srcPL)
{
  // Due to the "special case" where the value 8 and 9 is simple profile level zero and zero b,
  // we cannot actually use a simple min merge!
  unsigned dstProfile;
  int dstLevel;
  unsigned srcProfile;
  int srcLevel;

  switch (dstPL) {
    case 0:
      dstProfile = 0;
      dstLevel = -10;
      break;
    case 8:
      dstProfile = 0;
      dstLevel = -2;
      break;
    case 9:
      dstProfile = 0;
      dstLevel = -1;
      break;
    default:
      dstProfile = (dstPL>>4)&7;
      dstLevel = dstPL&7;
      break;
  }

  switch (srcPL) {
    case 0:
      srcProfile = 0;
      srcLevel = -10;
      break;
    case 8:
      srcProfile = 0;
      srcLevel = -2;
      break;
    case 9:
      srcProfile = 0;
      srcLevel = -1;
      break;
    default:
      srcProfile = (srcPL>>4)&7;
      srcLevel = srcPL&7;
      break;
  }


  if (dstProfile > srcProfile)
    dstProfile = srcProfile;
  if (dstLevel > srcLevel)
    dstLevel = srcLevel;

  switch (dstLevel) {
    case -10:
      return 0;
    case -2:
      return 8;
    case -1:
      return 9;
    default:
      return (dstProfile<<4)|dstLevel;
  }
}

///////////////////////////////////////////////////////////////////////////////

static bool MyToNormalised(PluginCodec_OptionMap & original, PluginCodec_OptionMap & changed)
{
    unsigned profileLevel = original.GetUnsigned(ProfileAndLevel, 1);
    unsigned width = original.GetUnsigned(PLUGINCODEC_OPTION_FRAME_WIDTH, 352);
    unsigned height = original.GetUnsigned(PLUGINCODEC_OPTION_FRAME_HEIGHT, 288);
    unsigned frameTime = original.GetUnsigned(PLUGINCODEC_OPTION_FRAME_TIME, 3000);

    // Though this is not a strict requirement we enforce 
    //it here in order to obtain optimal compression results
    width -= width % 16;
    height -= height % 16;

    int i = 0;
    while (mpeg4_profile_levels[i].profileLevel) {
      if (mpeg4_profile_levels[i].profileLevel == profileLevel)
        break;
      i++; 
    }

    if (!mpeg4_profile_levels[i].profileLevel) {
      PTRACE(1, MY_CODEC_LOG, "tIllegal Level negotiated");
      return false;
    }

  // Correct max. number of macroblocks per frame
    uint32_t nbMBsPerFrame = width * height / 256;
    unsigned j = 0;
    PTRACE(4, MY_CODEC_LOG, "Frame Size: " << nbMBsPerFrame << "(" << mpeg4_profile_levels[i].frame_size << ")");
    if    ( (nbMBsPerFrame          > mpeg4_profile_levels[i].frame_size) ) {

      while (mpeg4_resolutions[j].width) {
        if  ( (mpeg4_resolutions[j].macroblocks <= mpeg4_profile_levels[i].frame_size) )
            break;
        j++; 
      }
      if (!mpeg4_resolutions[j].width) {
        PTRACE(1, MY_CODEC_LOG, "No Resolution found that has number of macroblocks <=" << mpeg4_profile_levels[i].frame_size);
        return false;
      }

      width  = mpeg4_resolutions[j].width;
      height = mpeg4_resolutions[j].height;
    }

  // Correct macroblocks per second
    uint32_t nbMBsPerSecond = width * height / 256 * (90000 / frameTime);
    PTRACE(4, MY_CODEC_LOG, "MBs/s: " << nbMBsPerSecond << "(" << mpeg4_profile_levels[i].mbps << ")");
    if (nbMBsPerSecond > mpeg4_profile_levels[i].mbps)
      changed.SetUnsigned(PLUGINCODEC_VIDEO_CLOCK / 256 * width  * height / mpeg4_profile_levels[i].mbps, PLUGINCODEC_OPTION_FRAME_TIME);

    return true;
}


static bool MyToCustomised(PluginCodec_OptionMap & /*original*/, PluginCodec_OptionMap & /*changed*/)
{
  return true;
}


// End of File ///////////////////////////////////////////////////////////////
