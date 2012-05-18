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
 * $Revision$
 * $Author$
 * $Date$
 */

#include <codec/opalplugin.hpp>
#include <codec/known.h>

#include <stdio.h>

#undef min
#undef max

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

#define H263_BITRATE 327600

#define MAX_H263_CUSTOM_SIZES 10
#define DEFAULT_CUSTOM_MPI "0,0,"STRINGIZE(PLUGINCODEC_MPI_DISABLED)

static const char SQCIF_MPI[]  = PLUGINCODEC_SQCIF_MPI;
static const char QCIF_MPI[]   = PLUGINCODEC_QCIF_MPI;
static const char CIF_MPI[]    = PLUGINCODEC_CIF_MPI;
static const char CIF4_MPI[]   = PLUGINCODEC_CIF4_MPI;
static const char CIF16_MPI[]  = PLUGINCODEC_CIF16_MPI;

static const char RFC2190[] = "RFC2190";
static const char RFC2429[] = "RFC2429,RFC2190"; // Plus lesser included charge

static struct StdSizes {
  enum { 
    SQCIF, 
    QCIF, 
    CIF, 
    CIF4, 
    CIF16, 
    NumStdSizes,
    UnknownStdSize = NumStdSizes
  };

  int width;
  int height;
  const char * optionName;
} StandardVideoSizes[StdSizes::NumStdSizes] = {
  { PLUGINCODEC_SQCIF_WIDTH, PLUGINCODEC_SQCIF_HEIGHT, PLUGINCODEC_SQCIF_MPI },
  {  PLUGINCODEC_QCIF_WIDTH,  PLUGINCODEC_QCIF_HEIGHT, PLUGINCODEC_QCIF_MPI  },
  {   PLUGINCODEC_CIF_WIDTH,   PLUGINCODEC_CIF_HEIGHT, PLUGINCODEC_CIF_MPI   },
  {  PLUGINCODEC_CIF4_WIDTH,  PLUGINCODEC_CIF4_HEIGHT, PLUGINCODEC_CIF4_MPI  },
  { PLUGINCODEC_CIF16_WIDTH, PLUGINCODEC_CIF16_HEIGHT, PLUGINCODEC_CIF16_MPI },
};


static bool GetCustomMPI(const char * str,
                         unsigned width[MAX_H263_CUSTOM_SIZES],
                         unsigned height[MAX_H263_CUSTOM_SIZES],
                         unsigned mpi[MAX_H263_CUSTOM_SIZES],
                         size_t & count)
{
  count = 0;
  for (;;) {
    width[count] = height[count] = mpi[count] = 0;

    char * end;
    width[count] = strtoul(str, &end, 10);
    if (*end != ',')
      return false;

    str = end+1;
    height[count] = strtoul(str, &end, 10);
    if (*end != ',')
      return false;

    str = end+1;
    mpi[count] = strtoul(str, &end, 10);
    if (mpi[count] == 0 || mpi[count] > PLUGINCODEC_MPI_DISABLED)
      return false;

    if (mpi[count] < PLUGINCODEC_MPI_DISABLED && (width[count] < 16 || height[count] < 16))
      return false;

    if (mpi[count] == 0 || mpi[count] > PLUGINCODEC_MPI_DISABLED)
      return false;
    ++count;
    if (count >= MAX_H263_CUSTOM_SIZES || *end != ';')
      return true;

    str = end+1;
  }
}


static int MergeCustomResolution(const char * dest, const char * src, char * result)
{
  unsigned resultWidth[MAX_H263_CUSTOM_SIZES];
  unsigned resultHeight[MAX_H263_CUSTOM_SIZES];
  unsigned resultMPI[MAX_H263_CUSTOM_SIZES];
  size_t resultCount = 0;

  size_t srcCount;
  unsigned srcWidth[MAX_H263_CUSTOM_SIZES], srcHeight[MAX_H263_CUSTOM_SIZES], srcMPI[MAX_H263_CUSTOM_SIZES];

  if (!GetCustomMPI(src, srcWidth, srcHeight, srcMPI, srcCount)) {
    PTRACE(2, "IPP-H.263", "Invalid source custom MPI format \"" << src << '"');
    return false;
  }

  size_t dstCount;
  unsigned dstWidth[MAX_H263_CUSTOM_SIZES], dstHeight[MAX_H263_CUSTOM_SIZES], dstMPI[MAX_H263_CUSTOM_SIZES];
  if (!GetCustomMPI(dest, dstWidth, dstHeight, dstMPI, dstCount)) {
    PTRACE(2, "IPP-H.263", "Invalid destination custom MPI format \"" << dest << '"');
    return false;
  }

  for (size_t s = 0; s < srcCount; ++s) {
    for (size_t d = 0; d < dstCount; ++d) {
      if (srcWidth[s] == dstWidth[d] && srcHeight[s] == dstHeight[d]) {
        resultWidth[resultCount] = srcWidth[s];
        resultHeight[resultCount] = srcHeight[s];
        resultMPI[resultCount] = std::max(srcMPI[s], dstMPI[d]);
        ++resultCount;
      }
    }
  }

  if (resultCount == 0)
    strcpy(result, strdup(DEFAULT_CUSTOM_MPI));
  else {
    size_t len = 0;
    for (size_t i = 0; i < resultCount; ++i)
      len += sprintf(result+len, len == 0 ? "%u,%u,%u" : ";%u,%u,%u", resultWidth[i], resultHeight[i], resultMPI[i]);
  }

  return true;
}


static void FindBoundingBox(const PluginCodec_OptionMap & options, 
                            int * mpi,
                            int & minWidth,
                            int & minHeight,
                            int & maxWidth,
                            int & maxHeight,
                            int & frameTime,
                            int & targetBitRate,
                            int & maxBitRate)
{
  // initialise the MPI values to disabled
  int i;
  for (i = 0; i < 5; i++)
    mpi[i] = PLUGINCODEC_MPI_DISABLED;

  // following values will be set while scanning for options
  minWidth      = INT_MAX;
  minHeight     = INT_MAX;
  maxWidth      = 0;
  maxHeight     = 0;
  int rxMinWidth    = options.GetUnsigned(PLUGINCODEC_OPTION_MIN_RX_FRAME_WIDTH,  PLUGINCODEC_QCIF_WIDTH);
  int rxMinHeight   = options.GetUnsigned(PLUGINCODEC_OPTION_MIN_RX_FRAME_HEIGHT, PLUGINCODEC_QCIF_HEIGHT);
  int rxMaxWidth    = options.GetUnsigned(PLUGINCODEC_OPTION_MAX_RX_FRAME_WIDTH,  PLUGINCODEC_QCIF_WIDTH);
  int rxMaxHeight   = options.GetUnsigned(PLUGINCODEC_OPTION_MAX_RX_FRAME_HEIGHT, PLUGINCODEC_QCIF_HEIGHT);
  int frameRate     = 10;      // 10 fps
  int origFrameTime = options.GetUnsigned(PLUGINCODEC_OPTION_FRAME_TIME, 900);     // 10 fps in video RTP timestamps
  int maxBR         = options.GetUnsigned("MaxBR");
  maxBitRate        = options.GetUnsigned(PLUGINCODEC_OPTION_MAX_BIT_RATE);
  targetBitRate     = options.GetUnsigned(PLUGINCODEC_OPTION_TARGET_BIT_RATE);

  // extract the MPI values set in the custom options, and find the min/max of them
  frameTime = 0;

  for (i = 0; i < 5; i++) {
    mpi[i] = options.GetUnsigned(StandardVideoSizes[i].optionName, PLUGINCODEC_MPI_DISABLED);
    if (mpi[i] != PLUGINCODEC_MPI_DISABLED) {
      int thisTime = 3003*mpi[i];
      if (minWidth > StandardVideoSizes[i].width)
        minWidth = StandardVideoSizes[i].width;
      if (minHeight > StandardVideoSizes[i].height)
        minHeight = StandardVideoSizes[i].height;
      if (maxWidth < StandardVideoSizes[i].width)
        maxWidth = StandardVideoSizes[i].width;
      if (maxHeight < StandardVideoSizes[i].height)
        maxHeight = StandardVideoSizes[i].height;
      if (thisTime > frameTime)
        frameTime = thisTime;
    }
  }

  // if no MPIs specified, then the spec says to use QCIF
  if (frameTime == 0) {
    int ft;
    if (frameRate != 0) 
      ft = 90000 / frameRate;
    else 
      ft = origFrameTime;
    mpi[1] = (ft + 1502) / 3003;

#ifdef DEFAULT_TO_FULL_CAPABILITIES
    minWidth  = PLUGINCODEC_QCIF_WIDTH;
    maxWidth  = PLUGINCODEC_CIF16_WIDTH;
    minHeight = PLUGINCODEC_QCIF_HEIGHT;
    maxHeight = PLUGINCODEC_CIF16_HEIGHT;
#else
    minWidth  = maxWidth  = PLUGINCODEC_QCIF_WIDTH;
    minHeight = maxHeight = PLUGINCODEC_QCIF_HEIGHT;
#endif
  }

  // find the smallest MPI size that is larger than the min frame size
  for (i = 0; i < 5; i++) {
    if (StandardVideoSizes[i].width >= rxMinWidth && StandardVideoSizes[i].height >= rxMinHeight) {
      rxMinWidth = StandardVideoSizes[i].width;
      rxMinHeight = StandardVideoSizes[i].height;
      break;
    }
  }

  // find the largest MPI size that is smaller than the max frame size
  for (i = 4; i >= 0; i--) {
    if (StandardVideoSizes[i].width <= rxMaxWidth && StandardVideoSizes[i].height <= rxMaxHeight) {
      rxMaxWidth  = StandardVideoSizes[i].width;
      rxMaxHeight = StandardVideoSizes[i].height;
      break;
    }
  }

  // the final min/max is the smallest bounding box that will enclose both the MPI information and the min/max information
  minWidth  = std::max(rxMinWidth, minWidth);
  maxWidth  = std::min(rxMaxWidth, maxWidth);
  minHeight = std::max(rxMinHeight, minHeight);
  maxHeight = std::min(rxMaxHeight, maxHeight);

  // turn off any MPI that are outside the final bounding box
  for (i = 0; i < 5; i++) {
    if (StandardVideoSizes[i].width < minWidth || 
        StandardVideoSizes[i].width > maxWidth ||
        StandardVideoSizes[i].height < minHeight || 
        StandardVideoSizes[i].height > maxHeight)
     mpi[i] = PLUGINCODEC_MPI_DISABLED;
  }

  // find an appropriate max bit rate
  if (maxBitRate == 0) {
    if (maxBR != 0)
      maxBitRate = maxBR * 100;
    else if (targetBitRate != 0)
      maxBitRate = targetBitRate;
    else
      maxBitRate = 327000;
  }
  else if (maxBR > 0)
    maxBitRate = std::min(maxBR * 100, maxBitRate);

  if (targetBitRate == 0)
    targetBitRate = 327000;
}


static bool ClampSizes(PluginCodec_OptionMap & original, PluginCodec_OptionMap & changed)
{
  // find bounding box enclosing all MPI values
  int mpi[5];
  int minWidth, minHeight, maxHeight, maxWidth, frameTime, targetBitRate, maxBitRate;
  FindBoundingBox(original, mpi, minWidth, minHeight, maxWidth, maxHeight, frameTime, targetBitRate, maxBitRate);

  changed.SetUnsigned(minWidth,  PLUGINCODEC_OPTION_MIN_RX_FRAME_WIDTH);
  changed.SetUnsigned(minHeight, PLUGINCODEC_OPTION_MIN_RX_FRAME_HEIGHT);
  changed.SetUnsigned(maxWidth,  PLUGINCODEC_OPTION_MAX_RX_FRAME_WIDTH);
  changed.SetUnsigned(maxHeight, PLUGINCODEC_OPTION_MAX_RX_FRAME_HEIGHT);
  changed.SetUnsigned(frameTime, PLUGINCODEC_OPTION_FRAME_TIME);
  changed.SetUnsigned(maxBitRate, PLUGINCODEC_OPTION_MAX_BIT_RATE);
  changed.SetUnsigned(targetBitRate, PLUGINCODEC_OPTION_TARGET_BIT_RATE);
  changed.SetUnsigned((maxBitRate+50)/100, "MaxBR");
  for (int i = 0; i < 5; i++)
    changed.SetUnsigned(mpi[i], StandardVideoSizes[i].optionName);

  return true;
}


// End of File ///////////////////////////////////////////////////////////////
