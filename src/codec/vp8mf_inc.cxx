/*
 * vp8mf_inc.cxx
 *
 * Google VP8 Media Format descriptions
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
 */

#include <codec/opalplugin.hpp>
#include <codec/known.h>

#include <stdio.h>


///////////////////////////////////////////////////////////////////////////////

#ifdef MY_CODEC
  #define MY_CODEC_LOG STRINGIZE(MY_CODEC)
#else
  #define MY_CODEC_LOG "VP8"
#endif

static const char VP8FormatName[] = OPAL_VP8;
static const char VP8EncodingName[] = "VP8";
static const char MaxFrameRateName[] = "Max Frame Rate";
static const char MaxFrameRateSDP[] = "max-fr";
static const char MaxFrameSizeName[] = "Max Frame Size";
static const char MaxFrameSizeSDP[] = "max-fs";
static const char SpatialResamplingsName[] = "Spatial Resampling";
static const char SpatialResamplingsSDP[] = "dynamicres";
static const char SpatialResamplingUpName[] = "Spatial Resampling Up";
static const char SpatialResamplingDownName[] = "Spatial Resampling Down";
static const char PictureIDSizeName[] = "Picture ID Size";
static const char PictureIDSizeEnum[] = "None:Byte:Word";

const unsigned MaxBitRate = 16000000;
#define MIN_FR_SDP 1
#define MAX_FR_SDP 30
#define MIN_FS_SDP 48
#define MAX_FS_SDP 65536


/////////////////////////////////////////////////////////////////////////////

static bool MyToNormalised(PluginCodec_OptionMap & original, PluginCodec_OptionMap & changed)
{
  PluginCodec_OptionMap::iterator it = original.find(MaxFrameSizeName);
  if (it != original.end() && !it->second.empty()) {
    unsigned maxWidth = original.GetUnsigned(PLUGINCODEC_OPTION_MAX_RX_FRAME_WIDTH);
    unsigned maxHeight = original.GetUnsigned(PLUGINCODEC_OPTION_MAX_RX_FRAME_HEIGHT);
    unsigned maxFrameSize = PluginCodec_Utilities::String2Unsigned(it->second) % MAX_FS_SDP;
    if (PluginCodec_Utilities::ClampResolution(original, changed, maxWidth, maxHeight, maxFrameSize))
      PluginCodec_Utilities::Change(maxFrameSize, original, changed, MaxFrameSizeName);
  }

  it = original.find(MaxFrameRateName);
  if (it != original.end() && !it->second.empty())
    PluginCodec_Utilities::ClampMin(PLUGINCODEC_VIDEO_CLOCK/PluginCodec_Utilities::String2Unsigned(it->second), original, changed, PLUGINCODEC_OPTION_FRAME_TIME);

  return true;
}


static bool MyToCustomised(PluginCodec_OptionMap & original, PluginCodec_OptionMap & changed)
{
  PluginCodec_Utilities::Change(PluginCodec_Utilities::GetMacroBlocks(
                                    original.GetUnsigned(PLUGINCODEC_OPTION_MAX_RX_FRAME_WIDTH),
                                    original.GetUnsigned(PLUGINCODEC_OPTION_MAX_RX_FRAME_HEIGHT)),
                                original, changed, MaxFrameSizeName);

  PluginCodec_Utilities::Change(PLUGINCODEC_VIDEO_CLOCK/original.GetUnsigned(PLUGINCODEC_OPTION_FRAME_TIME),
                                original, changed, MaxFrameRateName);

  return true;
}


// End of File ///////////////////////////////////////////////////////////////
