/*
 * MPEG4 Plugin codec for OpenH323/OPAL
 *
  This codec implements an MPEG4 encoder/decoder using the ffmpeg library
  (http://ffmpeg.mplayerhq.hu/).  Plugin infrastructure is based off of the
  H.263 plugin and MPEG4 codec functions originally created by Michael Smith.

 * The original files, and this version of the original code, are released under the same 
 * MPL 1.0 license. Substantial portions of the original code were contributed
 * by Salyens and March Networks and their right to be identified as copyright holders
 * of the original code portions and any parts now included in this new copy is asserted through 
 * their inclusion in the copyright notices below.
 *
 * Copyright (C) 2007 Canadian Bank Note, Limited
 * Copyright (C) 2006 Post Increment
 * Copyright (C) 2005 Salyens
 * Copyright (C) 2001 March Networks Corporation
 * Copyright (C) 1999-2000 Equivalence Pty. Ltd.
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
 * The Original Code is Open H323 Library.
 *
 * The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * Contributor(s): Josh Mahonin (jmahonin@cbnco.com)
 *                 Michael Smith (msmith@cbnco.com)
 *                 Guilhem Tardy (gtardy@salyens.com)
 *                 Craig Southeren (craigs@postincrement.com)
 *                 Matthias Schneider (ma30002000@yahoo.de)
 *
 * NOTES:
 * Initial implementation of MPEG4 codec plugin using ffmpeg.
 * Untested under Windows or H.323
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#define _CRT_SECURE_NO_DEPRECATE

#ifndef _MSC_VER
#include "plugin-config.h"
#endif

#define MY_CODEC FF_MP4V  // Name of codec (use C variable characters)

#define OPAL_PLUGIN_COMPILE 1
#define OPAL_H323 1
#define OPAL_SIP 1

#include <codec/opalplugin.hpp>

#include "../../common/ffmpeg.h"


// Needed C++ headers
#include <deque>
#include <vector>

using namespace std;


// FFMPEG specific headers
extern "C" {

// Public ffmpeg headers.
// We'll pull them in from their locations in the ffmpeg source tree,
// but it would be possible to get them all from /usr/include/ffmpeg
// with #include <ffmpeg/...h>.
#ifdef LIBAVCODEC_HAVE_SOURCE_DIR
#include <libavutil/common.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavutil/log.h>

// Private headers from the ffmpeg source tree.
#include <libavutil/intreadwrite.h>
#include <libavutil/bswap.h>
#include <libavcodec/mpegvideo.h>

#else /* LIBAVCODEC_HAVE_SOURCE_DIR */
#include LIBAVCODEC_HEADER
#endif /* LIBAVCODEC_HAVE_SOURCE_DIR */
}


/////////////////////////////////////////////////////////////////////////////

#define MY_CODEC_LOG "FF_MP4V"

class MY_CODEC { };

PLUGINCODEC_CONTROL_LOG_FUNCTION_DEF

static const char MyFormatName[]  = "MPEG4";
static const char MyEncodingName[] = "MP4V-ES";
static const char MyDescription[] = "FFMPEG MPEG4 part 2 Video Codec";     // Human readable description of codec

PLUGINCODEC_LICENSE(
  "Craig Southeren, Guilhem Tardy, Derek Smithies, "
  "Michael Smith, Josh Mahonin, Robert Jongbloed",              // authors
  "1.0",                                                        // source code version
  "openh323@openh323.org",                                      // source code email
  "http://sourceforge.net/projects/openh323",                   // source code URL
  "Copyright (C) 2007 Canadian Bank Note, Limited., "           // source code copyright
  ", Copyright (C) 2006 by Post Increment"           
  ", Copyright (C) 2005 Salyens"
  ", Copyright (C) 2001 March Networks Corporation"
  ", Copyright (C) 1999-2000 Equivalence Pty. Ltd."
  ", Copyright (C) 2012 Vox Lucida Pty. Ltd.",
  "MPL 1.0",                                                    // source code license
  PluginCodec_License_MPL,                                      // source code license
  
  MyDescription,                                                // codec description
  "Michael Niedermayer, Fabrice Bellard",                       // codec author
  "4.7.1",                                                      // codec version
  "ffmpeg-devel-request@ mplayerhq.hu",                         // codec email
  "http://sourceforge.net/projects/ffmpeg/",                    // codec URL
  "Copyright (c) 2000-2001 Fabrice Bellard"                     // codec copyright information
  ", Copyright (c) 2002-2003 Michael Niedermayer",
  "GNU LESSER GENERAL PUBLIC LICENSE, Version 2.1, February 1999", // codec license
  PluginCodec_License_LGPL                                         // codec license code
);


/////////////////////////////////////////////////////////////////////////////

static const char ProfileAndLevel[] = "Profile & Level";

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

static int MergeProfileAndLevelMPEG4(char ** result, const char * dest, const char * src)
{
  // Due to the "special case" where the value 8 and 9 is simple profile level zero and zero b,
  // we cannot actually use a simple min merge!
  unsigned dstPL = strtoul(dest, NULL, 10);
  unsigned srcPL = strtoul(src, NULL, 10);

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

  char buffer[10];
  
  switch (dstLevel) {
    case -10:
      sprintf(buffer, "%u", (0));
      break;
    case -2:
      sprintf(buffer, "%u", (8));
      break;
    case -1:
      sprintf(buffer, "%u", (9));
      break;
    default:
      sprintf(buffer, "%u", (dstProfile<<4)|(dstLevel));
      break;
  }

  *result = strdup(buffer);

  return true;
}

static void FreeString(char * str)
{
  free(str);
}

static struct PluginCodec_Option const H245ProfileLevelMPEG4 =
{
  PluginCodec_IntegerOption,          // Option type
  ProfileAndLevel,                    // User visible name
  false,                              // User Read/Only flag
  PluginCodec_CustomMerge,            // Merge mode
  "5",                                // Initial value (Simple Profile/Level 5)
  "profile-level-id",                 // FMTP option name
  "0",                                // FMTP default value (Simple Profile/Level 1)
  H245_ANNEX_E_PROFILE_LEVEL,         // H.245 generic capability code and bit mask
  "0",                                // Minimum value
  "245",                              // Maximum value
  MergeProfileAndLevelMPEG4,          // Function to do merge
  FreeString                          // Function to free memory in string
};

static struct PluginCodec_Option const H245ObjectMPEG4 =
{
  PluginCodec_IntegerOption,          // Option type
  "H.245 Object Id",                  // User visible name
  true,                               // User Read/Only flag
  PluginCodec_EqualMerge,             // Merge mode
  "0",                                // Initial value
  NULL,                               // FMTP option name
  NULL,                               // FMTP default value
  H245_ANNEX_E_OBJECT,                // H.245 generic capability code and bit mask
  "0",                                // Minimum value
  "15"                                // Maximum value
};

static struct PluginCodec_Option const DecoderConfigInfoMPEG4 =
{
  PluginCodec_OctetsOption,           // Option type
  "DCI",                              // User visible name
  false,                              // User Read/Only flag
  PluginCodec_NoMerge,                // Merge mode
  "",                                 // Initial value
  "config",                           // FMTP option name
  NULL,                               // FMTP default value
  H245_ANNEX_E_DCI                    // H.245 generic capability code and bit mask
};

static struct PluginCodec_Option const MediaPacketizationMPEG4 =
{
  PluginCodec_StringOption,           // Option type
  PLUGINCODEC_MEDIA_PACKETIZATION,    // User visible name
  true,                               // User Read/Only flag
  PluginCodec_EqualMerge,             // Merge mode
  OpalPluginCodec_Identifer_MPEG4     // Initial value
};


static struct PluginCodec_Option const * const OptionTable[] = {
  &H245ProfileLevelMPEG4,
  &H245ObjectMPEG4,
  &DecoderConfigInfoMPEG4,
//  &ProfileMPEG4,
//  &LevelMPEG4,
  &MediaPacketizationMPEG4,
  NULL
};

static struct PluginCodec_H323GenericCodecData H323GenericMPEG4 = {
  OpalPluginCodec_Identifer_MPEG4
};

///////////////////////////////////////////////////////////////////////////////

class MPEG4_PluginMediaFormat : public PluginCodec_VideoFormat<MY_CODEC>
{
public:
  typedef PluginCodec_VideoFormat<MY_CODEC> BaseClass;

  MPEG4_PluginMediaFormat()
    : BaseClass(MyFormatName, MyEncodingName, MyDescription, 8000000, OptionTable)
  {
    m_h323CapabilityType = PluginCodec_H323Codec_generic;
    m_h323CapabilityData = &H323GenericMPEG4;
  }


  virtual bool ToNormalised(OptionMap & original, OptionMap & changed)
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

#if 0
  virtual bool ToCustomised(OptionMap & original, OptionMap & changed)
  {
    return true;
  }
#endif
};

static MPEG4_PluginMediaFormat MyMediaFormatInfo;


/////////////////////////////////////////////////////////////////////////////

class MPEG4_EncodedFrame : public FFMPEGCodec::EncodedFrame
{
  protected:
    // packet sizes generating in RtpCallback
    deque<unsigned> m_packetSizes;
    unsigned m_fragmentationOffset;

  public:
    MPEG4_EncodedFrame()
      : m_fragmentationOffset(0)
    {
    }


    virtual bool GetPacket(PluginCodec_RTP & rtp, unsigned & flags)
    {
      if (m_packetSizes.empty()) {
        m_fragmentationOffset = 0;
        return false;
      }

      if (!rtp.SetPayloadSize(std::min(m_packetSizes.front(), m_maxPayloadSize)))
        rtp.SetPayloadSize(rtp.GetMaxSize()-rtp.GetHeaderSize());
      size_t payloadSize = rtp.GetPayloadSize();

      // Remove, or adjust the current packet size
      if (payloadSize == m_packetSizes.front())
        m_packetSizes.pop_front();
      else
        m_packetSizes.front() -= payloadSize;

      // Copy the encoded data from the buffer into the outgoign RTP 
      memcpy(rtp.GetPayloadPtr(), GetBuffer()+m_fragmentationOffset, payloadSize);
      m_fragmentationOffset += payloadSize;

      // If there are no more packet sizes left, we've reached the last packet
      // for the frame, set the marker bit and flags
      if (m_packetSizes.empty()) {
       rtp.SetMarker(true);
        flags |= PluginCodec_ReturnCoderLastFrame;
      }

      return true;
    }


    virtual bool AddPacket(const PluginCodec_RTP & rtp, unsigned & flags)
    {
      if (!Append(rtp.GetPayloadPtr(), rtp.GetPayloadSize()))
        return false;

      if (rtp.GetMarker())
        flags |= PluginCodec_ReturnCoderLastFrame;

      return true;
    }


    virtual bool IsIntraFrame() const
    {
      unsigned i = 0;
      while ((i+4)<= m_length) {
        if ((m_buffer[i] == 0) && (m_buffer[i+1] == 0) && (m_buffer[i+2] == 1)) {
          if (m_buffer[i+3] == 0xb0)
            PTRACE(4, MY_CODEC_LOG, "Found visual_object_sequence_start_code, Profile/Level is " << (unsigned) m_buffer[i+4]);
          if (m_buffer[i+3] == 0xb6) {
            unsigned vop_coding_type = (unsigned) ((m_buffer[i+4] & 0xC0) >> 6);
            PTRACE(4, MY_CODEC_LOG, "Found vop_start_code, is vop_coding_type is " << vop_coding_type );
            if (vop_coding_type == 0)
              return true;
          }
        }
        i++;	
      }

      return false;
    }


    virtual void RTPCallBack(void *, int size, int)
    {
      m_packetSizes.push_back(size);
    }
};


/////////////////////////////////////////////////////////////////////////////
//
// define the encoding context
//

class MPEG4_Encoder : public PluginVideoEncoder<MY_CODEC>, public FFMPEGCodec
{
    typedef PluginVideoEncoder<MY_CODEC> BaseClass;

  public:
    MPEG4_Encoder(const PluginCodec_Definition * defn)
      : BaseClass(defn)
      , FFMPEGCodec(MY_CODEC_LOG, new MPEG4_EncodedFrame)
    { 
      PTRACE(4, m_prefix, "Created encoder $Revision$");
    }


    virtual bool Construct()
    {
      return InitEncoder(CODEC_ID_MPEG4);
    }


    virtual bool SetOption(const char * option, const char * value)
    {
      if (strcasecmp(option, ProfileAndLevel) == 0) {
        unsigned profileLevel = atoi(value);
        for (int i = 0; mpeg4_profile_levels[i].profileLevel != profileLevel; ++i) {
          if (mpeg4_profile_levels[i].profileLevel == 0) {
            PTRACE(1, m_prefix, "Illegal Profle-Level: " << profileLevel);
            return false;
          }
        }
        m_context->profile = profileLevel >> 4;
        m_context->level = profileLevel & 7;
      }

      return BaseClass::SetOption(option, value);
    }


    virtual bool OnChangedOptions()
    {
      CloseCodec();

      SetResolution(m_width, m_height);
      SetEncoderOptions(m_frameTime, m_maxBitRate, m_maxRTPSize, m_tsto, m_keyFramePeriod);

      m_context->max_b_frames = 0; /*don't use b frames*/
      m_context->flags |= CODEC_FLAG_AC_PRED
                       |  CODEC_FLAG_H263P_UMV
                    /* | CODEC_FLAG_QPEL */ // don't enable this one: this forces profile_level to advanced simple profile
                       | CODEC_FLAG_4MV
                       | CODEC_FLAG_GMC
                       | CODEC_FLAG_LOOP_FILTER
                       | CODEC_FLAG_H263P_SLICE_STRUCT;

      return OpenCodec();
    }


    virtual bool Transcode(const void * fromPtr,
                             unsigned & fromLen,
                                 void * toPtr,
                             unsigned & toLen,
                             unsigned & flags)
    {
      PluginCodec_RTP dstRTP(toPtr, toLen);
      if (!EncodeVideo(PluginCodec_RTP(fromPtr, fromLen), dstRTP, flags))
        return false;

      toLen = dstRTP.GetHeaderSize() + dstRTP.GetPayloadSize();
      return true;
    }


#ifdef LIBAVCODEC_HAVE_SOURCE_DIR
    //  The ffmpeg rate control averages bit usage over an entire file.
    //  We're only interested in instantaneous usage. Past periods of
    //  over-bandwidth or under-bandwidth shouldn't affect the next
    //  frame. So we reset the total_bits counter toward what it should be,
    //  picture_number * bit_rate / fps,
    //  bit by bit as we go.
    //
    //  We also reset the vbv buffer toward the value where it doesn't
    //  affect quantization - rc_buffer_size/2.
    //
    void ResetBitCounter(int spread);
    {
        MpegEncContext *s = (MpegEncContext *) m_context->priv_data;
        int64_t wanted_bits
            = int64_t(s->bit_rate * double(s->picture_number)
                      * av_q2d(m_context->time_base));
        s->total_bits += (wanted_bits - s->total_bits) / int64_t(spread);

        double want_buffer = double(m_context->rc_buffer_size / 2);
        s->rc_context.buffer_index
            += (want_buffer - s->rc_context.buffer_index) / double(spread);
    }
#endif
};


/////////////////////////////////////////////////////////////////////////////
//
// Define the decoder context
//

class MPEG4_Decoder : public PluginVideoDecoder<MY_CODEC>, public FFMPEGCodec
{
  typedef PluginVideoDecoder<MY_CODEC> BaseClass;

  public:
    MPEG4_Decoder(const PluginCodec_Definition * defn)
      : BaseClass(defn)
      , FFMPEGCodec(MY_CODEC_LOG, new MPEG4_EncodedFrame)
    {
    }


    bool Construct()
    {
      if (!InitDecoder(CODEC_ID_MPEG4))
        return false;

      m_context->flags |= CODEC_FLAG_4MV
                       |  CODEC_FLAG_PART;

      return OpenCodec();
    }


    virtual bool Transcode(const void * fromPtr,
                             unsigned & fromLen,
                                 void * toPtr,
                             unsigned & toLen,
                             unsigned & flags)
    {
      if (!DecodeVideo(PluginCodec_RTP(fromPtr, fromLen), flags))
        return false;

      if ((flags&PluginCodec_ReturnCoderLastFrame) == 0)
        return true;

      PluginCodec_RTP out(toPtr, toLen);
      toLen = OutputImage(m_picture->data, m_picture->linesize, m_context->width, m_context->height, out, flags);

      return true;
    }
};


/////////////////////////////////////////////////////////////////////////////

static struct PluginCodec_Definition CodecDefinition[] =
{
  PLUGINCODEC_VIDEO_CODEC_CXX(MyMediaFormatInfo, MPEG4_Encoder, MPEG4_Decoder),
};


PLUGIN_CODEC_IMPLEMENT_CXX(MY_CODEC, CodecDefinition);


/////////////////////////////////////////////////////////////////////////////
