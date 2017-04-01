/*
 * h263mf_inc.cxx
 *
 * H.263 Media Format descriptions
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

#include <algorithm>


///////////////////////////////////////////////////////////////////////////////

#ifdef MY_CODEC
  #define MY_CODEC_LOG STRINGIZE(MY_CODEC)
#else
  #define MY_CODEC_LOG OPAL_H263
#endif

static const char H263FormatName[] = OPAL_H263;
static const char H263EncodingName[] = "H263";

#define H263_PAYLOAD_TYPE 34

static const char H263plusFormatName[] = OPAL_H263plus;
static const char H263plusEncodingName[] = "H263-1998";

#define H263_BITRATE (16*1024*1024)

#define TIMESTAMP_29_97_FPS 3003

#define MAX_H263_CUSTOM_RESOLUTIONS 10
#define H263_CUSTOM_RESOLUTION_BUFFER_SIZE (MAX_H263_CUSTOM_RESOLUTIONS*20)
#define DEFAULT_CUSTOM_MPI "0,0," STRINGIZE(PLUGINCODEC_MPI_DISABLED)

static const char SQCIF_MPI[]  = PLUGINCODEC_SQCIF_MPI;
static const char QCIF_MPI[]   = PLUGINCODEC_QCIF_MPI;
static const char CIF_MPI[]    = PLUGINCODEC_CIF_MPI;
static const char CIF4_MPI[]   = PLUGINCODEC_CIF4_MPI;
static const char CIF16_MPI[]  = PLUGINCODEC_CIF16_MPI;

static const char RFC2190[] = "RFC2190";
static const char RFC2429[] = "RFC2429,RFC2190"; // Plus lesser included charge

static struct StandardResolution {
  enum { 
    SQCIF, 
    QCIF, 
    CIF, 
    CIF4, 
    CIF16, 
    Count
  };

  unsigned width;
  unsigned height;
  const char * optionName;
} StandardResolutions[StandardResolution::Count] = {
  { PLUGINCODEC_SQCIF_WIDTH, PLUGINCODEC_SQCIF_HEIGHT, PLUGINCODEC_SQCIF_MPI },
  {  PLUGINCODEC_QCIF_WIDTH,  PLUGINCODEC_QCIF_HEIGHT, PLUGINCODEC_QCIF_MPI  },
  {   PLUGINCODEC_CIF_WIDTH,   PLUGINCODEC_CIF_HEIGHT, PLUGINCODEC_CIF_MPI   },
  {  PLUGINCODEC_CIF4_WIDTH,  PLUGINCODEC_CIF4_HEIGHT, PLUGINCODEC_CIF4_MPI  },
  { PLUGINCODEC_CIF16_WIDTH, PLUGINCODEC_CIF16_HEIGHT, PLUGINCODEC_CIF16_MPI },
};


typedef struct CustomResoluton {
  unsigned width;
  unsigned height;
  unsigned mpi;
} CustomResolutions[MAX_H263_CUSTOM_RESOLUTIONS];

static bool GetCustomMPI(const char * str,
                         CustomResolutions res,
                         size_t & count)
{
  count = 0;
  for (;;) {
    res[count].width = res[count].height = res[count].mpi = 0;

    char * end;
    res[count].width = strtoul(str, &end, 10);
    if (*end != ',')
      return false;

    str = end+1;
    res[count].height = strtoul(str, &end, 10);
    if (*end != ',')
      return false;

    str = end+1;
    res[count].mpi = strtoul(str, &end, 10);
    if (res[count].mpi == 0 || res[count].mpi > PLUGINCODEC_MPI_DISABLED)
      return false;

    if (res[count].mpi == PLUGINCODEC_MPI_DISABLED)
      return true;

    if (res[count].width < 16 || res[count].height < 16)
      return false;

    ++count;
    if (count >= MAX_H263_CUSTOM_RESOLUTIONS || *end != ';')
      return true;

    str = end+1;
  }
}


static int MergeCustomResolution(const char * dest, const char * src, char * result)
{
  size_t resultCount = 0;
  CustomResolutions resultRes;

  size_t srcCount;
  CustomResolutions srcRes;

  if (!GetCustomMPI(src, srcRes, srcCount)) {
    PTRACE(2, "H.263", "Invalid source custom MPI format \"" << src << '"');
    return false;
  }

  size_t dstCount;
  CustomResolutions dstRes;
  if (!GetCustomMPI(dest, dstRes, dstCount)) {
    PTRACE(2, "H.263", "Invalid destination custom MPI format \"" << dest << '"');
    return false;
  }

  for (size_t s = 0; s < srcCount; ++s) {
    for (size_t d = 0; d < dstCount; ++d) {
      // The below extra test is only there for stupid GCC 5.1.0 warning
      if (d < MAX_H263_CUSTOM_RESOLUTIONS && srcRes[s].width == dstRes[d].width && srcRes[s].height == dstRes[d].height) {
        resultRes[resultCount].width = srcRes[s].width;
        resultRes[resultCount].height = srcRes[s].height;
        resultRes[resultCount].mpi = std::max(srcRes[s].mpi, dstRes[d].mpi);
        ++resultCount;
      }
    }
  }

  if (resultCount == 0)
    strcpy(result, strdup(DEFAULT_CUSTOM_MPI));
  else {
    size_t len = 0;
    for (size_t i = 0; i < resultCount; ++i)
      len += sprintf(result+len, len == 0 ? "%u,%u,%u" : ";%u,%u,%u",
                     resultRes[i].width, resultRes[i].height, resultRes[i].mpi);
  }

  return true;
}


static void ClampResolution(unsigned width, unsigned height, unsigned mpi,
                            unsigned & minWidth, unsigned & minHeight,
                            unsigned & maxWidth, unsigned & maxHeight,
                            unsigned & frameTime)
{
  if (mpi == PLUGINCODEC_MPI_DISABLED)
    return;

  if (minWidth > width)
    minWidth = width;

  if (minHeight > height)
    minHeight = height;

  if (maxWidth < width)
    maxWidth = width;

  if (maxHeight < height)
    maxHeight = height;

  unsigned thisTime = TIMESTAMP_29_97_FPS*mpi;
  if (thisTime > frameTime)
    frameTime = thisTime;
}


static bool ClampToNormalised(PluginCodec_OptionMap & original, PluginCodec_OptionMap & changed)
{
  // extract the MPI values set in the custom options, and find the min/max of them
  unsigned minWidth  = INT_MAX;
  unsigned minHeight = INT_MAX;
  unsigned maxWidth  = 0;
  unsigned maxHeight = 0;
  unsigned frameTime = 0;

  for (int i = 0; i < StandardResolution::Count; i++)
    ClampResolution(StandardResolutions[i].width,
                    StandardResolutions[i].height,
                    original.GetUnsigned(StandardResolutions[i].optionName, PLUGINCODEC_MPI_DISABLED),
                    minWidth, minHeight, maxWidth, maxHeight, frameTime);

  if (original.find(PLUGINCODEC_CUSTOM_MPI) != original.end()) {
    size_t customCount;
    CustomResolutions custom;
    if (!GetCustomMPI(original[PLUGINCODEC_CUSTOM_MPI].c_str(), custom, customCount)) {
      PTRACE(2, "H.263", "Invalid custom MPI format \"" << original[PLUGINCODEC_CUSTOM_MPI] << '"');
      return false;
    }

    for (size_t i = 0; i < customCount; ++i)
      ClampResolution(custom[i].width, custom[i].height, custom[i].mpi,
                      minWidth, minHeight, maxWidth, maxHeight, frameTime);
  }

  // if no MPIs specified, then the spec says to use QCIF
  if (frameTime == 0) {
    minWidth  = maxWidth  = PLUGINCODEC_QCIF_WIDTH;
    minHeight = maxHeight = PLUGINCODEC_QCIF_HEIGHT;
    frameTime = TIMESTAMP_29_97_FPS;
  }

  // Set min/max resolution and frame rate, derived from MPI
  changed.SetUnsigned(minWidth,  PLUGINCODEC_OPTION_MIN_RX_FRAME_WIDTH);
  changed.SetUnsigned(minHeight, PLUGINCODEC_OPTION_MIN_RX_FRAME_HEIGHT);
  changed.SetUnsigned(maxWidth,  PLUGINCODEC_OPTION_MAX_RX_FRAME_WIDTH);
  changed.SetUnsigned(maxHeight, PLUGINCODEC_OPTION_MAX_RX_FRAME_HEIGHT);
  changed.SetUnsigned(frameTime, PLUGINCODEC_OPTION_FRAME_TIME);

  // Set Max bit rate if provided
  unsigned maxBR = original.GetUnsigned("MaxBR")*100;
  if (maxBR != 0) {
    changed.SetUnsigned(maxBR, PLUGINCODEC_OPTION_MAX_BIT_RATE);
    PluginCodec_OptionMap::ClampMax(maxBR, original, changed, PLUGINCODEC_OPTION_TARGET_BIT_RATE);
  }

  return true;
}


static bool ClampToCustomised(PluginCodec_OptionMap & original, PluginCodec_OptionMap & changed)
{
  // following values will be set while scanning for options
  unsigned minWidth  = original.GetUnsigned(PLUGINCODEC_OPTION_MIN_RX_FRAME_WIDTH,  PLUGINCODEC_QCIF_WIDTH);
  unsigned minHeight = original.GetUnsigned(PLUGINCODEC_OPTION_MIN_RX_FRAME_HEIGHT, PLUGINCODEC_QCIF_HEIGHT);
  unsigned maxWidth  = original.GetUnsigned(PLUGINCODEC_OPTION_MAX_RX_FRAME_WIDTH,  PLUGINCODEC_QCIF_WIDTH);
  unsigned maxHeight = original.GetUnsigned(PLUGINCODEC_OPTION_MAX_RX_FRAME_HEIGHT, PLUGINCODEC_QCIF_HEIGHT);
  unsigned frameTime = original.GetUnsigned(PLUGINCODEC_OPTION_FRAME_TIME, TIMESTAMP_29_97_FPS);

  // turn off any MPI that are outside the final bounding box
  bool noMPI = true;
  for (int i = 0; i < StandardResolution::Count; i++) {
    unsigned mpi = PLUGINCODEC_MPI_DISABLED;
    if (StandardResolutions[i].width >= minWidth &&
        StandardResolutions[i].width <= maxWidth &&
        StandardResolutions[i].height >= minHeight &&
        StandardResolutions[i].height <= maxHeight) {
      mpi = (frameTime+TIMESTAMP_29_97_FPS-1)/TIMESTAMP_29_97_FPS;
      noMPI = false;
    }
    changed.SetUnsigned(mpi, StandardResolutions[i].optionName);
  }

  if (noMPI) {
    PTRACE(2, "H.263", "Resolution range ("
           << minWidth << 'x' << minHeight << '-' << maxWidth << 'x' << maxHeight
           << ") outside all possible fixed sizes.");
    return false;
  }

  unsigned maxBitRate = original.GetUnsigned(PLUGINCODEC_OPTION_MAX_BIT_RATE);
  if (maxBitRate == 0)
    maxBitRate = H263_BITRATE;
  changed.SetUnsigned((maxBitRate+50)/100, "MaxBR");

  return true;
}


// End of File ///////////////////////////////////////////////////////////////
