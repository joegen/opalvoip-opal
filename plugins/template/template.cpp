/*
 * Template Plugin codec for OPAL
 *
 * This the starting point for creating new plug in codecs for C++
 * It is generally done fom the point of view of video codecs, as
 * audio codecs are much simpler and there is a wealth of examples
 * in C available.
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
 * Contributor(s): ______________________________________.
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#include <codec/opalplugin.hpp>

#define MY_CODEC   mycodec                        // Name of codec (use C variable characters)

static unsigned   MyVersion = PLUGIN_CODEC_VERSION_OPTIONS; // Minimum version for codec (should never change)
static const char MyDescription[] = "My Video Codec";       // Human readable description of codec
static const char MyFormatName[] = "MyCodec";               // OpalMediaFormat name string to generate
static const char MyPayloadName[] = "mycod";                // RTP payload name (IANA approved)
static unsigned   MyClockRate = 90000;                      // RTP dictates 90000
static unsigned   MyMaxBitRate = 1000000;                   // Maximum bit rate
static unsigned   MyMaxFrameRate = 60;                      // Maximum frame rate (per second)
static unsigned   MyMaxWidth = 352;                         // Maximum width of frame
static unsigned   MyMaxHeight = 288;                        // Maximum height of frame

static const char YUV420PFormatName[] = "YUV420P";          // Raw media format


static struct PluginCodec_information LicenseInfo = {
  1143692893,                                                   // timestamp = Thu 30 Mar 2006 04:28:13 AM UTC

  "Robert Jongbloed, Vox Lucida Pty.Ltd.",                      // source code author
  "1.0",                                                        // source code version
  "robertj@voxlucida.com.au",                                   // source code email
  "http://www.voxlucida.com.au",                                // source code URL
  "Copyright (C) 2010 by Vox Lucida Pt.Ltd., All Rights Reserved", // source code copyright
  "MPL 1.0",                                                    // source code license
  PluginCodec_License_MPL,                                      // source code license
  
  MyDescription,                                                // codec description
  "Professor Dumbledore",                                       // codec author
  "1.0",                                                        // codec version
  "headmaster@hogwarts.ac.uk",                                  // codec email
  "http://hogwarts.ac.uk",                                      // codec URL
  "Copyright, Ministry of Magic",                               // codec copyright information
  NULL,                                                         // codec license
  PluginCodec_License_BSD                                       // codec license code
};


///////////////////////////////////////////////////////////////////////////////

static struct PluginCodec_Option const MinRxFrameWidth =
{
  PluginCodec_IntegerOption,
  PLUGINCODEC_OPTION_MIN_RX_FRAME_WIDTH,
  true,
  PluginCodec_NoMerge,
  "176",
  NULL,
  NULL,
  0,
  "176",
  "352"
};

static struct PluginCodec_Option const MinRxFrameHeight =
{
  PluginCodec_IntegerOption,
  PLUGINCODEC_OPTION_MIN_RX_FRAME_HEIGHT,
  true,
  PluginCodec_NoMerge,
  "144",
  NULL,
  NULL,
  0,
  "144",
  "288"
};

static struct PluginCodec_Option const MaxRxFrameWidth =
{
  PluginCodec_IntegerOption,
  PLUGINCODEC_OPTION_MAX_RX_FRAME_WIDTH,
  true,
  PluginCodec_NoMerge,
  "352",
  NULL,
  NULL,
  0,
  "176",
  "352"
};

static struct PluginCodec_Option const MaxRxFrameHeight =
{
  PluginCodec_IntegerOption,
  PLUGINCODEC_OPTION_MAX_RX_FRAME_HEIGHT,
  true,
  PluginCodec_NoMerge,
  "288",
  NULL,
  NULL,
  0,
  "144",
  "288"
};


static struct PluginCodec_Option const * const MyOptions[] = {
  &MinRxFrameWidth,
  &MinRxFrameHeight,
  &MaxRxFrameWidth,
  &MaxRxFrameHeight,
  NULL
};


///////////////////////////////////////////////////////////////////////////////

class MyPluginMediaFormat : public PluginCodec_MediaFormat
{
  public:
    MyPluginMediaFormat()
      : PluginCodec_MediaFormat(MyOptions)
    {
    }


    virtual bool ToNormalised(OptionMap & original, OptionMap & changed)
    {
      return true;
    }


    virtual bool ToCustomised(OptionMap & original, OptionMap & changed)
    {
      return true;
    }
};

static MyPluginMediaFormat MyMediaFormat;


///////////////////////////////////////////////////////////////////////////////

class MyEncoder : public PluginCodec
{
  protected:
    unsigned m_width;
    unsigned m_height;
    unsigned m_frameRate;
    unsigned m_bitRate;

  public:
    MyEncoder(const PluginCodec_Definition * defn)
      : PluginCodec(defn)
      , m_width(352)
      , m_height(288)
      , m_frameRate(15)
      , m_bitRate(512000)
    {
    }


    virtual bool Construct()
    {
      /* Complete construction of object after it has been created. This
         allows you to fail the create operation (return false), which cannot
         be done in the normal C++ constructor. */
      return true;
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

      // Base class sets bit rate and frame time
      return PluginCodec::SetOption(optionName, optionValue);
    }


    virtual bool OnChangedOptions()
    {
      /* After all the options are set via SetOptions() this is called if any
         of them changed. This would do whatever is needed to set parmaeters
         for the actual codec. */
      return true;
    }


    virtual bool Transcode(const void * fromPtr,
                             unsigned & fromLen,
                                 void * toPtr,
                             unsigned & toLen,
                             unsigned & flags)
    {
      // At this point fromPtr and toPtr are validated as non NULL

      const PluginCodec_Video_FrameHeader * videoHeader =
                (const PluginCodec_Video_FrameHeader *)PluginCodec_RTP_GetPayloadPtr(fromPtr);
      const unsigned char * videoBuffer = OPAL_VIDEO_FRAME_DATA_PTR(videoHeader);

      /* Check for catastrophic failures, if return false, aborts the codec
        conversion completely, and will be closed and re-opened. */
      if (fromLen < videoHeader->width*videoHeader->height*3/2 + 
                    sizeof(PluginCodec_Video_FrameHeader) +
                    PluginCodec_RTP_GetHeaderLength(fromPtr))
        return false;

      /* Do conversion of source YUV420 video frame to RTP packets, do not exceed
         the size indicated in toLen for any of the output packets. If there are
         no packets generated, set the toLen to zero and no packets will be sent
         over the wire. This allows for various bit rate control algorithms. */

      // Set output packet size, including RTP header
      toLen = 1400;

      // If we determine this is an Intra (reference) frame, set flag
      if (true)
        flags |= PluginCodec_ReturnCoderIFrame;

      // If we determine this is the last RTP packet of video frame, set flag
      if (true)
        flags |= PluginCodec_ReturnCoderLastFrame;

      return true;
    }
};


///////////////////////////////////////////////////////////////////////////////

class MyDecoder : public PluginCodec
{
  public:
    MyDecoder(const PluginCodec_Definition * defn)
      : PluginCodec(defn)
    {
    }


    virtual bool Construct()
    {
      /* Complete construction of object after it has been created. This
         allows you to fail the create operation (return false), which cannot
         be done in the normal C++ constructor. */
      return true;
    }


    virtual size_t GetOutputDataSize()
    {
      /* Example just uses maximums. If they are very large, e.g. 4096x4096
         then a VERY large buffer will be created by OPAL. It is more sensible
         to use a "typical" size here and allow it to get bigger via the
         PluginCodec_ReturnCoderBufferTooSmall mechanism if needed. */

      return MyMaxWidth*MyMaxHeight*3/2 + 
             sizeof(PluginCodec_Video_FrameHeader) +
             PluginCodec_RTP_MinHeaderSize;
    }


    virtual bool Transcode(const void * fromPtr,
                             unsigned & fromLen,
                                 void * toPtr,
                             unsigned & toLen,
                             unsigned & flags)
    {
      // At this point fromPtr and toPtr are validated as non NULL

      PluginCodec_Video_FrameHeader * videoHeader =
                (PluginCodec_Video_FrameHeader *)PluginCodec_RTP_GetPayloadPtr(toPtr);
      unsigned char * videoBuffer = OPAL_VIDEO_FRAME_DATA_PTR(videoHeader);

      /* Do conversion of RTP packet. Note that typically many are received
         before an output frame is generated. If no output fram is available
         yet, simply return true and more will be sent. */

      // Test if we MUST output an Intra frame;
      if ((flags&PluginCodec_CoderForceIFrame) != 0)
        /*Force reference frame */;

      /* If we determine there is some non-fatal decoding error that will
         cause the output to be corrupted, set flag to get new Intra frame */
      if (true)
        flags |= PluginCodec_ReturnCoderRequestIFrame;

      /* If we determine via toLen the output is not big enough, set flag.
         Make sure GetOutputDataSize() then returns correct larger size. */
      if (true)
        flags |= PluginCodec_ReturnCoderBufferTooSmall;

      // If we determine this is an Intra (reference) frame, set flag
      if (true)
        flags |= PluginCodec_ReturnCoderIFrame;

      // If we determine we have enough input to generate output, set flag
      if (true)
        flags |= PluginCodec_ReturnCoderLastFrame;

      return true;
    }
};


///////////////////////////////////////////////////////////////////////////////

static struct PluginCodec_Definition MyCodecDefinition[] =
{
  {
    // Encoder
    MyVersion,                          // codec API version
    &LicenseInfo,                       // license information

    PluginCodec_MediaTypeVideo |        // audio codec
    PluginCodec_RTPTypeDynamic,         // dynamic RTP type

    MyDescription,                      // text decription
    YUV420PFormatName,                  // source format
    MyFormatName,                       // destination format

    &MyMediaFormat,                     // user data 

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

    PluginCodec::Create<MyEncoder>,     // create codec function
    PluginCodec::Destroy,               // destroy codec
    PluginCodec::Transcode,             // encode/decode
    PluginCodec::GetControls(),         // codec controls

    PluginCodec_H323Codec_generic,      // h323CapabilityType 
    NULL                                // h323CapabilityData
  },
  { 
    // Decoder
    MyVersion,                          // codec API version
    &LicenseInfo,                       // license information

    PluginCodec_MediaTypeVideo |        // audio codec
    PluginCodec_RTPTypeDynamic,         // dynamic RTP type

    MyDescription,                      // text decription
    MyFormatName,                       // source format
    YUV420PFormatName,                  // destination format

    &MyMediaFormat,                     // user data 

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

    PluginCodec::Create<MyDecoder>,     // create codec function
    PluginCodec::Destroy,               // destroy codec
    PluginCodec::Transcode,             // encode/decode
    PluginCodec::GetControls(),         // codec controls

    PluginCodec_H323Codec_generic,      // h323CapabilityType 
    NULL                                // h323CapabilityData
  }
};

extern "C"
{
  PLUGIN_CODEC_IMPLEMENT_ALL(MY_CODEC, MyCodecDefinition, MyVersion)
};

/////////////////////////////////////////////////////////////////////////////
