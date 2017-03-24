/*
 * opusmf_inc.cxx
 *
 * Opus Media Format descriptions
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

#include <codec/opalplugin.h>
#include <codec/known.h>


///////////////////////////////////////////////////////////////////////////////

static const char OpusEncodingName[] = "OPUS"; // MIME name rfc's 3047, 5577

static const char UseInBandFEC_OptionName[]    = "Use In-Band FEC";
static const char UseInBandFEC_FMTPName[]      = "useinbandfec";
static const char UseDTX_OptionName[]          = "Use DTX";
static const char UseDTX_FMTPName[]            = "usedtx";
static const char ConstantBitRate_OptionName[] = "Constant Bit Rate";
static const char ConstantBitRate_FMTPName[]   = "cbr";
static const char MaxPlaybackRate_OptionName[] = "Max Playback Sample Rate";
static const char MaxPlaybackRate_FMTPName[]   = "maxplaybackrate";
static const char MaxCaptureRate_OptionName[]  = "Max Capture Sample Rate";
static const char MaxCaptureRate_FMTPName[]    = "sprop-maxcapturerate";
static const char PlaybackStereo_OptionName[]  = "Playback Stereo";
static const char PlaybackStereo_FMTPName[]    = "stereo";
static const char CaptureStereo_OptionName[]   = "Capture Stereo";
static const char CaptureStereo_FMTPName[]     = "sprop-stereo";
static const char MaxAverageBitRate_FMTPName[] = "maxaveragebitrate";

#define OPUS_FRAME_MS    20
#define OPUS_SAMPLE_RATE 48000

#define MIN_SAMPLE_RATE 8000
#define MAX_SAMPLE_RATE 48000

#define MIN_BIT_RATE 6000
#define MAX_BIT_RATE 510000

#define DEFAULT_BIT_RATE 64000
#define DEFAULT_USE_FEC  1
#define DEFAULT_USE_DTX  0
#define DEFAULT_CBR      0
#define DEFAULT_STEREO   0


// End of File ///////////////////////////////////////////////////////////////
