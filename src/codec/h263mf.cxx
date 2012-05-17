/*
 * h263mf.cxx
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

// This is to get around the DevStudio precompiled headers rqeuirements.
#ifdef OPAL_PLUGIN_COMPILE
#define PTLIB_PTLIB_H
#endif

#include <ptlib.h>

#include <codec/opalplugin.hpp>
#include <codec/known.h>


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


///////////////////////////////////////////////////////////////////////////////

#ifndef OPAL_PLUGIN_COMPILE

// Now put the tracing  back again
#undef PTRACE
#define PTRACE(level, args) PTRACE2(level, PTraceObjectInstance(), args)


#include <opal/mediafmt.h>
#include <codec/opalpluginmgr.h>
#include <asn/h245.h>


/////////////////////////////////////////////////////////////////////////////

#if OPAL_H323

class H323H263baseCapability : public H323H263Capability
{
  public:
    H323H263baseCapability()
      : H323H263Capability(H263FormatName)
    {
    }

    virtual PObject * Clone() const
    {
      return new H323H263baseCapability(*this);
    }
};

class H323H263plusCapability : public H323H263Capability
{
  public:
    H323H263plusCapability()
      : H323H263Capability(H263plusFormatName)
    {
    }

    virtual PObject * Clone() const
    {
      return new H323H263plusCapability(*this);
    }
};

#endif // OPAL_H323


class OpalCustomSizeOption : public OpalMediaOptionString
{
    PCLASSINFO(OpalCustomSizeOption, OpalMediaOptionString)
  public:
    OpalCustomSizeOption(
      const char * name,
      bool readOnly
    ) : OpalMediaOptionString(name, readOnly)
    {
    }

    virtual bool Merge(const OpalMediaOption & option)
    {
      char buffer[MAX_H263_CUSTOM_SIZES*20];
      if (!MergeCustomResolution(m_value, option.AsString(), buffer))
        return false;

      m_value = buffer;
      return true;
    }
};


class OpalH263Format : public OpalVideoFormatInternal
{
  public:
    OpalH263Format(const char * formatName, const char * encodingName)
      : OpalVideoFormatInternal(formatName, RTP_DataFrame::DynamicBase, encodingName,
                                PVideoFrameInfo::CIF16Width, PVideoFrameInfo::CIF16Height, 30, H263_BITRATE)
    {
      OpalMediaOption * option;

      option = new OpalMediaOptionInteger(SQCIF_MPI, false, OpalMediaOption::MaxMerge, 1, 1, PLUGINCODEC_MPI_DISABLED);
      OPAL_SET_MEDIA_OPTION_FMTP(option, "SQCIF", STRINGIZE(PLUGINCODEC_MPI_DISABLED));
      AddOption(option);

      option = new OpalMediaOptionInteger(QCIF_MPI, false, OpalMediaOption::MaxMerge, 1, 1, PLUGINCODEC_MPI_DISABLED);
      OPAL_SET_MEDIA_OPTION_FMTP(option, "QCIF", STRINGIZE(PLUGINCODEC_MPI_DISABLED));
      AddOption(option);

      option = new OpalMediaOptionInteger(CIF_MPI, false, OpalMediaOption::MaxMerge, 1, 1, PLUGINCODEC_MPI_DISABLED);
      OPAL_SET_MEDIA_OPTION_FMTP(option, "CIF", STRINGIZE(PLUGINCODEC_MPI_DISABLED));
      AddOption(option);

      option = new OpalMediaOptionInteger(CIF4_MPI, false, OpalMediaOption::MaxMerge, 1, 1, PLUGINCODEC_MPI_DISABLED);
      OPAL_SET_MEDIA_OPTION_FMTP(option, "CIF4", STRINGIZE(PLUGINCODEC_MPI_DISABLED));
      AddOption(option);

      option = new OpalMediaOptionInteger(CIF16_MPI, false, OpalMediaOption::MaxMerge, 1, 1, PLUGINCODEC_MPI_DISABLED);
      OPAL_SET_MEDIA_OPTION_FMTP(option, "CIF16", STRINGIZE(PLUGINCODEC_MPI_DISABLED));
      AddOption(option);

      option = new OpalMediaOptionInteger("MaxBR", false, OpalMediaOption::MinMerge, 0, 0, 32767);
      OPAL_SET_MEDIA_OPTION_FMTP(option, "maxbr", "0");
      AddOption(option);

      if (GetName() == H263FormatName)
        AddOption(new OpalMediaOptionString(PLUGINCODEC_MEDIA_PACKETIZATION, false, RFC2190));
      else {
        option = new OpalMediaOptionString(PLUGINCODEC_MEDIA_PACKETIZATIONS, false, RFC2429);
        option->SetMerge(OpalMediaOption::IntersectionMerge);
        AddOption(option);

        option = new OpalCustomSizeOption(PLUGINCODEC_CUSTOM_MPI, false);
        OPAL_SET_MEDIA_OPTION_FMTP(option, "CUSTOM", DEFAULT_CUSTOM_MPI);
        AddOption(option);

        option = new OpalMediaOptionBoolean(H263_ANNEX_D, false, OpalMediaOption::AndMerge);
        OPAL_SET_MEDIA_OPTION_FMTP(option, "D", "0");
        AddOption(option);

        option = new OpalMediaOptionBoolean(H263_ANNEX_F, false, OpalMediaOption::AndMerge);
        OPAL_SET_MEDIA_OPTION_FMTP(option, "F", "0");
        AddOption(option);

        option = new OpalMediaOptionBoolean(H263_ANNEX_I, false, OpalMediaOption::AndMerge);
        OPAL_SET_MEDIA_OPTION_FMTP(option, "I", "0");
        AddOption(option);

        option = new OpalMediaOptionBoolean(H263_ANNEX_J, false, OpalMediaOption::AndMerge);
        OPAL_SET_MEDIA_OPTION_FMTP(option, "J", "0");
        AddOption(option);

        option = new OpalMediaOptionBoolean(H263_ANNEX_K, false, OpalMediaOption::AndMerge);
        OPAL_SET_MEDIA_OPTION_FMTP(option, "K", "0");
        AddOption(option);

        option = new OpalMediaOptionBoolean(H263_ANNEX_N, false, OpalMediaOption::AndMerge);
        OPAL_SET_MEDIA_OPTION_FMTP(option, "N", "0");
        AddOption(option);

        option = new OpalMediaOptionBoolean(H263_ANNEX_T, false, OpalMediaOption::AndMerge);
        OPAL_SET_MEDIA_OPTION_FMTP(option, "T", "0");
        AddOption(option);
      }
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
      if (!ClampSizes(original, changed))
        return false;
      SetChangedOptions(changed);
      return OpalVideoFormatInternal::ToNormalisedOptions();
    }

    virtual bool ToCustomisedOptions()
    {
      PluginCodec_OptionMap original, changed;
      GetOriginalOptions(original);
      if (!ClampSizes(original, changed))
        return false;
      SetChangedOptions(changed);
      return OpalVideoFormatInternal::ToCustomisedOptions();
    }
};


const OpalVideoFormat & GetOpalH263()
{
  static OpalVideoFormat const format(new OpalH263Format(H263FormatName, H263EncodingName));

#if OPAL_H323
  static H323CapabilityFactory::Worker<H323H263baseCapability> capability(H263FormatName, true);
#endif // OPAL_H323

  return format;
}


const OpalVideoFormat & GetOpalH263plus()
{
  static OpalVideoFormat const format(new OpalH263Format(H263plusFormatName, H263plusEncodingName));

#if OPAL_H323
  static H323CapabilityFactory::Worker<H323H263plusCapability> capability(H263plusFormatName, true);
#endif // OPAL_H323

  return format;
}


#endif // OPAL_PLUGIN_COMPILE


// End of File ///////////////////////////////////////////////////////////////
