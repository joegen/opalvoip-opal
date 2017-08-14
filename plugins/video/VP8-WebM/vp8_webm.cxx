/*
 * VP8 (WebM) video codec Plugin codec for OPAL
 *
 * Copyright (C) 2012 Vox Lucida Pty Ltd, All Rights Reserved
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
 * Added with funding from Requestec, Inc.
 *
 * Contributor(s): Robert Jongbloed (robertj@voxlucida.com.au)
 *
 */

#include "../common/platform.h"

#define MY_CODEC VP8  // Name of codec (use C variable characters)

#define OPAL_SIP 1

#include "../../../src/codec/vp8mf_inc.cxx"

#include <codec/opalplugin.hpp>
#include "../common/critsect.h"

#ifdef _MSC_VER
#pragma warning(disable:4505)
#define snprintf _snprintf
#endif


/* Building notes for Windows.
   As of v1.3.0, no prebuilt libraries seem to be available, you must build it
   from scratch, and to do that you must have Cygwin installed.

   After unpacking the source into opal/plugins/video/VP8-WebM/vpx-vp8, run
   cygwin and go to that directory. Then, depending on the Visual Studio
   version you with to compile against, XX=10, XX=11 or XX=12

      mkdir vsXX_32
      cd vsXX_32
      ../configure --target=x86-win32-vsXX
      make
      ./vpx.sln
   And finally, batch build vpx Debug and Release.

   And if desired (XX=11 or XX=12 only) again for 64-bit:
      cd ..
      mkdir vsXX_64
      cd vsXX_64
      ../configure --target=x86_64-win64-vsXX
      make
 */

#define VPX_CODEC_DISABLE_COMPAT 1
#include "vpx/vpx_encoder.h"
#include "vpx/vp8cx.h"
#include "vpx/vpx_decoder.h"
#include "vpx/vp8dx.h"

#include <vector>
#include <stdio.h>


/* Error concelment (handling missing data) does not appear to work. Or we are
   not using it right. Either way disable it or we eventually get a Error 5 in
   vpx_codec_decode - Bitstream not supported by this decoder.
 */
#ifdef VPX_CODEC_USE_ERROR_CONCEALMENT
#undef VPX_CODEC_USE_ERROR_CONCEALMENT
#endif

#define HAS_OUTPUT_PARTITION (defined(VPX_CODEC_CAP_OUTPUT_PARTITION) && defined(VPX_CODEC_USE_OUTPUT_PARTITION))

#define INCLUDE_OM_CUSTOM_PACKETIZATION 1

class VP8_CODEC { };

PLUGINCODEC_CONTROL_LOG_FUNCTION_DEF


PLUGINCODEC_LICENSE(
  "Robert Jongbloed, Vox Lucida Pty.Ltd.",                      // source code author
  "1.0",                                                        // source code version
  "robertj@voxlucida.com.au",                                   // source code email
  "http://www.voxlucida.com.au",                                // source code URL
  "Copyright (C) 2011 by Vox Lucida Pt.Ltd., All Rights Reserved", // source code copyright
  "MPL 1.0",                                                    // source code license
  PluginCodec_License_MPL,                                      // source code license
  
  "VP8 Video Codec",                                            // codec description
  "James Bankoski, John Koleszar, Lou Quillio, Janne Salonen, "
  "Paul Wilkins, Yaowu Xu, all from Google Inc.",               // codec author
  "8", 							        // codec version
  "jimbankoski@google.com, jkoleszar@google.com, "
  "louquillio@google.com, jsalonen@google.com, "
  "paulwilkins@google.com, yaowu@google.com",                   // codec email
  "http://google.com",                                          // codec URL
  "Copyright (c) 2010, 2011, Google Inc.",                      // codec copyright information
  "Copyright (c) 2010, 2011, Google Inc.",                      // codec license
  PluginCodec_License_BSD                                       // codec license code
);


///////////////////////////////////////////////////////////////////////////////

static struct PluginCodec_Option const MaxFR =
{
  PluginCodec_IntegerOption,          // Option type
  MaxFrameRateName,                   // User visible name
  false,                              // User Read/Only flag
  PluginCodec_MinMerge,               // Merge mode
  STRINGIZE(MAX_FR_SDP),              // Initial value
  MaxFrameRateSDP,                    // FMTP option name
  STRINGIZE(MAX_FR_SDP),              // FMTP default value
  0,                                  // H.245 generic capability code and bit mask
  STRINGIZE(MIN_FR_SDP),              // Minimum value
  STRINGIZE(MAX_FR_SDP)               // Maximum value
};

static struct PluginCodec_Option const MaxFS =
{
  PluginCodec_IntegerOption,          // Option type
  MaxFrameSizeName,                   // User visible name
  false,                              // User Read/Only flag
  PluginCodec_MinMerge,               // Merge mode
  STRINGIZE(MAX_FS_SDP),              // Initial value
  MaxFrameSizeSDP,                    // FMTP option name
  STRINGIZE(MAX_FS_SDP),              // FMTP default value
  0,                                  // H.245 generic capability code and bit mask
  STRINGIZE(MIN_FS_SDP),              // Minimum value
  STRINGIZE(MAX_FS_SDP)               // Maximum value
};

#if INCLUDE_OM_CUSTOM_PACKETIZATION
static struct PluginCodec_Option const MaxFrameSizeOM =
{
  PluginCodec_StringOption,           // Option type
  "SIP/SDP Max Frame Size",           // User visible name
  true,                               // User Read/Only flag
  PluginCodec_NoMerge,                // Merge mode
  "",                                 // Initial value
  "x-mx-max-size",                    // FMTP option name
  ""                                  // FMTP default value (as per RFC)
};
#endif

static struct PluginCodec_Option const TemporalSpatialTradeOff =
{
  PluginCodec_IntegerOption,          // Option type
  PLUGINCODEC_OPTION_TEMPORAL_SPATIAL_TRADE_OFF, // User visible name
  false,                              // User Read/Only flag
  PluginCodec_AlwaysMerge,            // Merge mode
  "31",                               // Initial value
  NULL,                               // FMTP option name
  NULL,                               // FMTP default value
  0,                                  // H.245 generic capability code and bit mask
  "1",                                // Minimum value
  "31"                                // Maximum value
};

static struct PluginCodec_Option const SpatialResampling =
{
  PluginCodec_BoolOption,             // Option type
  SpatialResamplingsName,             // User visible name
  false,                              // User Read/Only flag
  PluginCodec_AndMerge,               // Merge mode
  "0",                                // Initial value
  SpatialResamplingsSDP,              // FMTP option name
  "0",                                // FMTP default value
  0                                   // H.245 generic capability code and bit mask
};

static struct PluginCodec_Option const SpatialResamplingUp =
{
  PluginCodec_IntegerOption,          // Option type
  SpatialResamplingUpName,            // User visible name
  false,                              // User Read/Only flag
  PluginCodec_AlwaysMerge,            // Merge mode
  "100",                              // Initial value
  NULL,                               // FMTP option name
  NULL,                               // FMTP default value
  0,                                  // H.245 generic capability code and bit mask
  "0",                                // Minimum value
  "100"                               // Maximum value
};

static struct PluginCodec_Option const SpatialResamplingDown =
{
  PluginCodec_IntegerOption,          // Option type
  SpatialResamplingDownName,          // User visible name
  false,                              // User Read/Only flag
  PluginCodec_AlwaysMerge,            // Merge mode
  "0",                                // Initial value
  NULL,                               // FMTP option name
  NULL,                               // FMTP default value
  0,                                  // H.245 generic capability code and bit mask
  "0",                                // Minimum value
  "100"                               // Maximum value
};

static struct PluginCodec_Option const PictureIDSize =
{
  PluginCodec_EnumOption,             // Option type
  PictureIDSizeName,                  // User visible name
  false,                              // User Read/Only flag
  PluginCodec_AlwaysMerge,            // Merge mode
  "None",                             // Initial value
  NULL,                               // FMTP option name
  NULL,                               // FMTP default value
  0,                                  // H.245 generic capability code and bit mask
  "None:Byte:Word"                    // Enumeration
};

#if HAS_OUTPUT_PARTITION
static struct PluginCodec_Option const OutputPartition =
{
  PluginCodec_BoolOption,             // Option type
  "Output Partition",                 // User visible name
  false,                              // User Read/Only flag
  PluginCodec_AndMerge,               // Merge mode
  "0",                                // Initial value
  NULL,                               // FMTP option name
  NULL,                               // FMTP default value
  0                                   // H.245 generic capability code and bit mask
};
#endif

static struct PluginCodec_Option const * OptionTableRFC[] = {
  &MaxFR,
  &MaxFS,
  &PictureIDSize,
#if HAS_OUTPUT_PARTITION
  &OutputPartition,
#endif
  &TemporalSpatialTradeOff,
  &SpatialResampling,
  &SpatialResamplingUp,
  &SpatialResamplingDown,
  NULL
};


#if INCLUDE_OM_CUSTOM_PACKETIZATION
static struct PluginCodec_Option const * OptionTableOM[] = {
  &MaxFrameSizeOM,
  &TemporalSpatialTradeOff,
  &SpatialResampling,
  &SpatialResamplingUp,
  &SpatialResamplingDown,
  NULL
};
#endif


///////////////////////////////////////////////////////////////////////////////

class VP8Format : public PluginCodec_VideoFormat<VP8_CODEC>
{
  private:
    typedef PluginCodec_VideoFormat<VP8_CODEC> BaseClass;

  public:
    VP8Format(const char * formatName, const char * payloadName, const char * description, OptionsTable options)
      : BaseClass(formatName, payloadName, description, MaxBitRate, options)
    {
#ifdef VPX_CODEC_USE_ERROR_CONCEALMENT
      if ((vpx_codec_get_caps(vpx_codec_vp8_dx()) & VPX_CODEC_CAP_ERROR_CONCEALMENT) != 0)
        m_flags |= PluginCodec_ErrorConcealment; // Prevent video update request on packet loss
#endif
    }
};


class VP8FormatRFC : public VP8Format
{
  public:
    VP8FormatRFC()
      : VP8Format(VP8FormatName, VP8EncodingName, "VP8 Video Codec (RFC)", OptionTableRFC)
    {
    }


    virtual bool IsValidForProtocol(const char * protocol) const
    {
      return strcasecmp(protocol, PLUGINCODEC_OPTION_PROTOCOL_SIP) == 0;
    }


    virtual bool ToNormalised(OptionMap & original, OptionMap & changed) const
    {
      return MyToNormalised(original, changed);
    }


    virtual bool ToCustomised(OptionMap & original, OptionMap & changed) const
    {
      return MyToCustomised(original, changed);
    }
};

static VP8FormatRFC const VP8MediaFormatInfoRFC;


///////////////////////////////////////////////////////////////////////////////

#if INCLUDE_OM_CUSTOM_PACKETIZATION

class VP8FormatOM : public VP8Format
{
  public:
    VP8FormatOM()
      : VP8Format("VP8-OM", "X-MX-VP8", "VP8 Video Codec (Open Market)", OptionTableOM)
    {
    }


    virtual bool IsValidForProtocol(const char * protocol) const
    {
      return strcasecmp(protocol, PLUGINCODEC_OPTION_PROTOCOL_SIP) == 0;
    }


    virtual bool ToNormalised(OptionMap & original, OptionMap & changed) const
    {
      OptionMap::iterator it = original.find(MaxFrameSizeOM.m_name);
      if (it != original.end() && !it->second.empty()) {
        std::stringstream strm(it->second);
        unsigned maxWidth, maxHeight;
        char x;
        strm >> maxWidth >> x >> maxHeight;
        if (maxWidth < 32 || maxHeight < 32) {
          PTRACE(1, MY_CODEC_LOG, "Invalid " << MaxFrameSizeOM.m_name << ", was \"" << it->second << '"');
          return false;
        }
        ClampMax(maxWidth,  original, changed, PLUGINCODEC_OPTION_MAX_RX_FRAME_WIDTH);
        ClampMax(maxHeight, original, changed, PLUGINCODEC_OPTION_MAX_RX_FRAME_HEIGHT);
        ClampMax(maxWidth,  original, changed, PLUGINCODEC_OPTION_MIN_RX_FRAME_WIDTH);
        ClampMax(maxHeight, original, changed, PLUGINCODEC_OPTION_MIN_RX_FRAME_HEIGHT);
      }
      return true;
    }


    virtual bool ToCustomised(OptionMap & original, OptionMap & changed) const
    {
      unsigned maxWidth = original.GetUnsigned(PLUGINCODEC_OPTION_MAX_RX_FRAME_WIDTH);
      unsigned maxHeight = original.GetUnsigned(PLUGINCODEC_OPTION_MAX_RX_FRAME_HEIGHT);
      std::stringstream strm;
      strm << maxWidth << 'x' << maxHeight;
      Change(strm.str().c_str(), original, changed, MaxFrameSizeOM.m_name);
      return true;
    }
};

static VP8FormatOM const VP8MediaFormatInfoOM;

#endif // INCLUDE_OM_CUSTOM_PACKETIZATION


///////////////////////////////////////////////////////////////////////////////

static bool IsError(vpx_codec_err_t err, const char * fn)
{
  if (err == VPX_CODEC_OK)
    return false;

  PTRACE(1, MY_CODEC_LOG, "Error " << err << " in " << fn << " - " << vpx_codec_err_to_string(err));
  return true;
}

#define IS_ERROR(fn, args) IsError(fn args, #fn)


enum Orientation {
  PortraitLeft,
  LandscapeUp,
  PortraitRight,
  LandscapeDown,
  OrientationMask = 3,

  OrientationExtHdrShift = 4,
  OrientationPktHdrShift = 5
};


///////////////////////////////////////////////////////////////////////////////

class VP8Encoder : public PluginVideoEncoder<VP8_CODEC>
{
  private:
    typedef PluginVideoEncoder<VP8_CODEC> BaseClass;

  protected:
    vpx_codec_enc_cfg_t        m_config;
    unsigned                   m_initFlags;
    unsigned                   m_maxQ;
    unsigned                   m_rateControlPeriod;
    vpx_codec_ctx_t            m_codec;
    vpx_codec_iter_t           m_iterator;
    const vpx_codec_cx_pkt_t * m_packet;
    size_t                     m_offset;

    CriticalSection m_mutex;

  public:
    VP8Encoder(const PluginCodec_Definition * defn)
      : BaseClass(defn)
      , m_initFlags(0)
      , m_maxQ(31)
      , m_rateControlPeriod(1000)
      , m_iterator(NULL)
      , m_packet(NULL)
      , m_offset(0)
    {
      memset(&m_codec, 0, sizeof(m_codec));
    }


    ~VP8Encoder()
    {
      vpx_codec_destroy(&m_codec);
    }


    virtual bool Construct()
    {
      if (IS_ERROR(vpx_codec_enc_config_default, (vpx_codec_vp8_cx(), &m_config, 0)))
        return false;

      m_maxBitRate = m_config.rc_target_bitrate*1000;
      m_maxQ = m_config.rc_max_quantizer;

      m_config.g_w = 0; // Forces OnChangedOptions to initialise encoder
      m_config.g_h = 0;
      m_config.g_error_resilient = 1;
      m_config.g_pass = VPX_RC_ONE_PASS;
      m_config.g_lag_in_frames = 0;
      m_config.rc_end_usage = VPX_CBR;
      m_config.g_timebase.num = 1;
      m_config.g_timebase.den = PLUGINCODEC_VIDEO_CLOCK;

      if (!OnChangedOptions())
        return false;

      PTRACE(4, MY_CODEC_LOG, "Encoder opened: " << vpx_codec_version_str());
      return true;
    }


    virtual bool SetOption(const char * optionName, const char * optionValue)
    {
      if (strcasecmp(optionName, PLUGINCODEC_OPTION_RATE_CONTROL_PERIOD) == 0)
        return SetOptionUnsigned(m_rateControlPeriod, optionValue, 100, 60000);

      if (strcasecmp(optionName, SpatialResampling.m_name) == 0)
        return SetOptionBoolean(m_config.rc_resize_allowed, optionValue);

      if (strcasecmp(optionName, SpatialResamplingUp.m_name) == 0)
        return SetOptionUnsigned(m_config.rc_resize_up_thresh, optionValue, 0, 100);

      if (strcasecmp(optionName, SpatialResamplingDown.m_name) == 0)
        return SetOptionUnsigned(m_config.rc_resize_down_thresh, optionValue, 0, 100);

      return BaseClass::SetOption(optionName, optionValue);
    }


    virtual bool OnChangedOptions()
    {
      WaitAndSignal lock(m_mutex);

      m_config.kf_mode = VPX_KF_AUTO;
      if (m_keyFramePeriod != 0)
        m_config.kf_min_dist = m_config.kf_max_dist = m_keyFramePeriod;
      else {
        m_config.kf_min_dist = 0;
        m_config.kf_max_dist = 10*PLUGINCODEC_VIDEO_CLOCK/m_frameTime;  // No slower than once every 10 seconds
      }

      m_config.rc_target_bitrate = m_maxBitRate/1000;
      m_config.rc_buf_sz = m_rateControlPeriod;
      m_config.rc_buf_optimal_sz = m_rateControlPeriod*4/5;

      // Take simple temporal/spatial trade off and set multiple variables
      m_config.rc_resize_allowed = m_tsto > 29;
      if (m_tsto < 31) {
        m_config.rc_dropframe_thresh = 60 + m_tsto;     // Range from 60% to 90%
        m_config.rc_max_quantizer = m_maxQ*m_tsto/31;
      }
      else {
        m_config.rc_dropframe_thresh = 0;   // m_tsto==31 is maintain frame rate, so threshold is zero
        m_config.rc_max_quantizer = m_maxQ; // Allow Q to get as bad as it can get
      }

      PTRACE(4, MY_CODEC_LOG, "Config: " << m_width << 'x' << m_height << ","
                              " rc_target_bitrate=" << m_config.rc_target_bitrate << ","
                              " kf_dist=" << m_config.kf_min_dist << '-' << m_config.kf_max_dist << ","
                              " rc_dropframe_thresh=" << m_config.rc_dropframe_thresh << ","
                              " rc_resize_allowed=" << m_config.rc_resize_allowed << ","
                              " rc_max_quantizer=" << m_config.rc_max_quantizer << ","
                              " rc_buf_sz=" << m_config.rc_buf_sz << ","
                              " rc_buf_optimal_sz=" << m_config.rc_buf_optimal_sz << ","
                              " rc_undershoot_pct=" << m_config.rc_undershoot_pct);

      if (m_config.g_w == m_width && m_config.g_h == m_height)
        return !IS_ERROR(vpx_codec_enc_config_set, (&m_codec, &m_config));

      m_config.g_w = m_width;
      m_config.g_h = m_height;
      vpx_codec_destroy(&m_codec);
      return !IS_ERROR(vpx_codec_enc_init, (&m_codec, vpx_codec_vp8_cx(), &m_config, m_initFlags));
    }


    bool NeedEncode()
    {
      if (m_packet != NULL)
        return false;

      WaitAndSignal lock(m_mutex);

      while ((m_packet = vpx_codec_get_cx_data(&m_codec, &m_iterator)) != NULL) {
        if (m_packet->kind == VPX_CODEC_CX_FRAME_PKT)
          return false;
      }

      m_iterator = NULL;
      return true;
    }


    virtual int GetStatistics(char * bufferPtr, unsigned bufferSize)
    {
      size_t len = BaseClass::GetStatistics(bufferPtr, bufferSize);
      len += snprintf(bufferPtr+len, bufferSize-len, "Width=%u\nHeight=%u\n", m_width, m_height);

      WaitAndSignal lock(m_mutex);

      int quality = -1;
      IS_ERROR(vpx_codec_control_VP8E_GET_LAST_QUANTIZER_64,(&m_codec, VP8E_GET_LAST_QUANTIZER_64, &quality));
      if (quality >= 0 && len < bufferSize)
        len += snprintf(bufferPtr+len, bufferSize-len, "Quality=%u\n", quality);

      return (int)len;
    }


    virtual bool Transcode(const void * fromPtr,
                             unsigned & fromLen,
                                 void * toPtr,
                             unsigned & toLen,
                             unsigned & flags)
    {
      if (NeedEncode()) {
        PluginCodec_RTP srcRTP(fromPtr, fromLen);
        PluginCodec_Video_FrameHeader * video = srcRTP.GetVideoHeader();

        if (m_width != video->width || m_height != video->height) {
          PTRACE(4, MY_CODEC_LOG, "Changing resolution from " <<
                 m_width << 'x' << m_height << " to " << video->width << 'x' << video->height);
          m_width = video->width;
          m_height = video->height;
          if (!OnChangedOptions())
            return false;
        }

        vpx_image_t image;
        vpx_img_wrap(&image, VPX_IMG_FMT_I420, video->width, video->height, 2, srcRTP.GetVideoFrameData());

        {
          WaitAndSignal lock(m_mutex);

          if (IS_ERROR(vpx_codec_encode, (&m_codec, &image,
                                          srcRTP.GetTimestamp(), m_frameTime,
                                          (flags&PluginCodec_CoderForceIFrame) != 0 ? VPX_EFLAG_FORCE_KF : 0,
                                          VPX_DL_REALTIME)))
            return false;
        }

        if (NeedEncode()) {
          // Skipping frame completely.
          flags = PluginCodec_ReturnCoderLastFrame;
          toLen = 0;
          return true;
        }
      }

      flags = 0;
      if ((m_packet->data.frame.flags&VPX_FRAME_IS_KEY) != 0)
        flags |= PluginCodec_ReturnCoderIFrame;

      PluginCodec_RTP dstRTP(toPtr, toLen);
      Packetise(dstRTP);
      toLen = (unsigned)dstRTP.GetPacketSize();

      if (m_offset >= m_packet->data.frame.sz) {
        flags |= PluginCodec_ReturnCoderLastFrame;
        dstRTP.SetMarker(true);
        m_packet = NULL;
        m_offset = 0;
      }

      return true;
    }


    virtual void Packetise(PluginCodec_RTP & rtp) = 0;
};


class VP8EncoderRFC : public VP8Encoder
{
  protected:
    unsigned m_pictureId;
    unsigned m_pictureIdSize;

  public:
    VP8EncoderRFC(const PluginCodec_Definition * defn)
      : VP8Encoder(defn)
      , m_pictureId(rand()&0x7fff)
      , m_pictureIdSize(0)
    {
    }


    virtual bool SetOption(const char * optionName, const char * optionValue)
    {
      if (strcasecmp(optionName, PictureIDSize.m_name) == 0) {
        if (strcasecmp(optionValue, "Byte") == 0) {
          if (m_pictureIdSize != 0x80) {
            m_pictureIdSize = 0x80;
            m_optionsSame = false;
          }
          return true;
        }

        if (strcasecmp(optionValue, "Word") == 0) {
          if (m_pictureIdSize != 0x8000) {
            m_pictureIdSize = 0x8000;
            m_optionsSame = false;
          }
          return true;
        }

        if (strcasecmp(optionValue, "None") == 0) {
          if (m_pictureIdSize != 0) {
            m_pictureIdSize = 0;
            m_optionsSame = false;
          }
          return true;
        }

        PTRACE(2, MY_CODEC_LOG, "Unknown picture ID size: \"" << optionValue << '"');
        return false;
      }

#if HAS_OUTPUT_PARTITION
      if (strcasecmp(optionName, OutputPartition.m_name) == 0 &&
                    (vpx_codec_get_caps(vpx_codec_vp8_dx()) & VPX_CODEC_CAP_OUTPUT_PARTITION) != 0)
        return SetOptionBit(m_initFlags, VPX_CODEC_USE_OUTPUT_PARTITION, optionValue);
#endif

      return VP8Encoder::SetOption(optionName, optionValue);
    }


    virtual void Packetise(PluginCodec_RTP & rtp)
    {
      size_t headerSize = 1;

      rtp[0] = 0;

      if (m_offset == 0)
        rtp[0] |= 0x10; // Add S bit if start of partition

#if HAS_OUTPUT_PARTITION
      if (m_packet->data.frame.partition_id > 0 && m_packet->data.frame.partition_id <= 8)
        rtp[0] |= (uint8_t)m_packet->data.frame.partition_id;
#endif

      if ((m_packet->data.frame.flags&VPX_FRAME_IS_DROPPABLE) != 0)
        rtp[0] |= 0x20; // Add N bit for non-reference frame

      if (m_pictureIdSize > 0) {
        headerSize += 2;
        rtp[0] |= 0x80; // Add X bit for X (extension) bit mask byte
        rtp[1] |= 0x80; // Add I bit for picture ID
        if (m_pictureId < 128)
          rtp[2] = (uint8_t)m_pictureId;
        else {
          ++headerSize;
          rtp[2] = (uint8_t)(0x80 | (m_pictureId >> 8));
          rtp[3] = (uint8_t)m_pictureId;
        }

        if (m_offset == 0 && ++m_pictureId >= m_pictureIdSize)
          m_pictureId = 0;
      }

      size_t fragmentSize = GetPacketSpace(rtp, m_packet->data.frame.sz - m_offset + headerSize) - headerSize;
      rtp.CopyPayload((char *)m_packet->data.frame.buf+m_offset, fragmentSize, headerSize);
      m_offset += fragmentSize;
    }
};


#if INCLUDE_OM_CUSTOM_PACKETIZATION

class VP8EncoderOM : public VP8Encoder
{
  protected:
    unsigned m_currentGID;

  public:
    VP8EncoderOM(const PluginCodec_Definition * defn)
      : VP8Encoder(defn)
      , m_currentGID(0)
    {
    }


    virtual void Packetise(PluginCodec_RTP & rtp)
    {
      size_t headerSize;
      if (m_offset != 0) {
        headerSize = 1;
        rtp[0] = (uint8_t)m_currentGID; // No start bit or header extension bit
      }
      else {
        headerSize = 2;

        rtp[0] = 0x40; // Start bit

        unsigned type = UINT_MAX;
        size_t len;
        unsigned char * ext = rtp.GetExtendedHeader(type, len);
        Orientation orientation = type == 0x10001 ? (Orientation)(*ext >> OrientationExtHdrShift) : LandscapeUp;
        rtp[1] = (unsigned char)(orientation << OrientationPktHdrShift);

        if ((m_packet->data.frame.flags&VPX_FRAME_IS_KEY) != 0) {
          rtp[1] |= 0x80; // Indicate is golden frame
          m_currentGID = (m_currentGID+1)&0x3f;
        }

        rtp[0] |= m_currentGID;
      }

      size_t fragmentSize = GetPacketSpace(rtp, m_packet->data.frame.sz - m_offset + headerSize) - headerSize;
      rtp.CopyPayload((char *)m_packet->data.frame.buf+m_offset, fragmentSize, headerSize);
      m_offset += fragmentSize;
    }
};

#endif //INCLUDE_OM_CUSTOM_PACKETIZATION


///////////////////////////////////////////////////////////////////////////////

class VP8Decoder : public PluginVideoDecoder<VP8_CODEC>
{
  private:
    typedef PluginVideoDecoder<VP8_CODEC> BaseClass;

  protected:
    vpx_codec_iface_t  * m_iface;
    vpx_codec_ctx_t      m_codec;
    vpx_codec_flags_t    m_flags;
    vpx_codec_iter_t     m_iterator;
    std::vector<uint8_t> m_fullFrame;
    bool                 m_firstFrame;
    bool                 m_intraFrame;
    bool                 m_ignoreTillKeyFrame;
    unsigned             m_consecutiveErrors;

  public:
    VP8Decoder(const PluginCodec_Definition * defn)
      : BaseClass(defn)
      , m_iface(vpx_codec_vp8_dx())
      , m_flags(0)
      , m_iterator(NULL)
      , m_firstFrame(true)
      , m_intraFrame(false)
      , m_ignoreTillKeyFrame(false)
      , m_consecutiveErrors(0)
    {
      memset(&m_codec, 0, sizeof(m_codec));
      m_fullFrame.reserve(10000);

#ifdef VPX_CODEC_USE_ERROR_CONCEALMENT
      if ((vpx_codec_get_caps(m_iface) & VPX_CODEC_CAP_ERROR_CONCEALMENT) != 0)
        m_flags |= VPX_CODEC_USE_ERROR_CONCEALMENT;
#endif
    }


    ~VP8Decoder()
    {
      vpx_codec_destroy(&m_codec);
    }


    virtual bool Construct()
    {
      if (IS_ERROR(vpx_codec_dec_init, (&m_codec, m_iface, NULL, m_flags)))
        return false;

      PTRACE(4, MY_CODEC_LOG, "Decoder opened: " << vpx_codec_version_str());
      return true;
    }


    virtual int GetStatistics(char * bufferPtr, unsigned bufferSize)
    {
      size_t len = BaseClass::GetStatistics(bufferPtr, bufferSize);
      len += snprintf(bufferPtr+len, bufferSize-len, "Width=%u\nHeight=%u\n", m_width, m_height);
      return (int)len;
    }


    bool BadDecode(unsigned & flags, bool ok)
    {
      if (ok)
        return false;

      flags = PluginCodec_ReturnCoderRequestIFrame;
      m_ignoreTillKeyFrame = true;
      m_fullFrame.clear();
      return true;
    }


    virtual bool Transcode(const void * fromPtr,
                             unsigned & fromLen,
                                 void * toPtr,
                             unsigned & toLen,
                             unsigned & flags)
    {
      vpx_image_t * image;

      bool noLostPackets = (flags & PluginCodec_CoderPacketLoss) == 0;

      flags = m_intraFrame ? PluginCodec_ReturnCoderIFrame : 0;

      if (m_firstFrame || (image = vpx_codec_get_frame(&m_codec, &m_iterator)) == NULL) {
        /* Unless error concealment implemented, decoder has a problems with
           missing data in the frame and can gets it's knickers thorougly
           twisted, so just ignore everything till next I-Frame. */
        if (BadDecode(flags,
#ifdef VPX_CODEC_USE_ERROR_CONCEALMENT
                             (m_flags & VPX_CODEC_USE_ERROR_CONCEALMENT) != 0 ||
#endif
                             noLostPackets))
          return true;

        PluginCodec_RTP srcRTP(fromPtr, fromLen);
        if (BadDecode(flags, Unpacketise(srcRTP)))
          return true;

        if (!srcRTP.GetMarker() || m_fullFrame.empty())
          return true;

        vpx_codec_err_t err = vpx_codec_decode(&m_codec, &m_fullFrame[0], (unsigned)m_fullFrame.size(), NULL, 0);
        switch (err) {
          case VPX_CODEC_OK :
            m_consecutiveErrors = 0;
            break;

          case VPX_CODEC_UNSUP_BITSTREAM :
            if (m_consecutiveErrors++ > 10) {
              IsError(err, "vpx_codec_decode");
              return false;
            }

          // Non fatal errors
          case VPX_CODEC_UNSUP_FEATURE :
          case VPX_CODEC_CORRUPT_FRAME :
            PTRACE(3, MY_CODEC_LOG, "Decoder reported non-fatal error: " << vpx_codec_err_to_string(err));
            BadDecode(flags, false);
            return true;

          default:
            IsError(err, "vpx_codec_decode");
            return false;
        }

#if 1
        /* Prefer to use vpx_codec_get_stream_info() here, but it doesn't
           work, it always returns key frame! The vpx_codec_peek_stream_info()
           function is also useless, it return VPX_CODEC_UNSUP_FEATURE for
           anything that isn't an I-Frame. Luckily it appears that the low bit
           of the first byte is the I-Frame indicator.
        */
        if ((m_fullFrame[0]&1) == 0)
          m_intraFrame = true;
#else
        vpx_codec_stream_info_t info;
        info.sz = sizeof(info);
        if (IS_ERROR(vpx_codec_get_stream_info, (&m_codec, &info)))
          flags |= PluginCodec_ReturnCoderRequestIFrame;

        if (info.is_kf)
          m_intraFrame = true;
#endif

        if (m_intraFrame)
          flags |= PluginCodec_ReturnCoderIFrame;

        m_fullFrame.clear();

        m_iterator = NULL;
        if ((image = vpx_codec_get_frame(&m_codec, &m_iterator)) == NULL)
          return true;

        m_firstFrame = false;
      }

      if (image->fmt != VPX_IMG_FMT_I420) {
        PTRACE(1, MY_CODEC_LOG, "Unsupported image format from decoder.");
        return false;
      }

      m_width = image->d_w;
      m_height = image->d_h;

      PluginCodec_RTP dstRTP(toPtr, toLen);
      toLen = OutputImage(image->planes, image->stride, image->d_w, image->d_h, dstRTP, flags);

      if ((flags & PluginCodec_ReturnCoderLastFrame) != 0)
        m_intraFrame = false;

      return true;
    }


    void Accumulate(const unsigned char * fragmentPtr, size_t fragmentLen)
    {
      if (fragmentLen == 0)
        return;

      size_t size = m_fullFrame.size();
      m_fullFrame.reserve(size+fragmentLen*2);
      m_fullFrame.resize(size+fragmentLen);
      memcpy(&m_fullFrame[size], fragmentPtr, fragmentLen);
    }


    virtual bool Unpacketise(const PluginCodec_RTP & rtp) = 0;
};


class VP8DecoderRFC : public VP8Decoder
{
  protected:
    unsigned m_partitionID;

  public:
    VP8DecoderRFC(const PluginCodec_Definition * defn)
      : VP8Decoder(defn)
      , m_partitionID(0)
    {
    }


    virtual bool Unpacketise(const PluginCodec_RTP & rtp)
    {
      if (rtp.GetPayloadSize() == 0)
        return true;

      if (rtp.GetPayloadSize() < 2) {
        PTRACE(3, MY_CODEC_LOG, "RTP packet far too small.");
        return false;
      }

      size_t headerSize = 1;
      if ((rtp[0]&0x80) != 0) { // Check X bit
        ++headerSize;           // Allow for X byte

        if ((rtp[1]&0x80) != 0) { // Check I bit
          ++headerSize;           // Allow for I field
          if ((rtp[2]&0x80) != 0) // > 7 bit picture ID
            ++headerSize;         // Allow for extra bits of I field
        }

        if ((rtp[1]&0x40) != 0) // Check L bit
          ++headerSize;         // Allow for L byte

        if ((rtp[1]&0x30) != 0) // Check T or K bit
          ++headerSize;         // Allow for T/K byte
      }

      if (rtp.GetPayloadSize() <= headerSize) {
        PTRACE(3, MY_CODEC_LOG, "RTP packet too small.");
        return false;
      }

      if (m_ignoreTillKeyFrame) {
        // Key frame is S bit == 1, partID == 0 and P bit == 0
        if ((rtp[0]&0x1f) != 0x10 || (rtp[headerSize]&0x01) != 0)
          return false;
        m_ignoreTillKeyFrame =  false;
        PTRACE(3, MY_CODEC_LOG, "Found next start of key frame.");
      }

      if ((rtp[0]&0x10) != 0) { // Check S bit
        unsigned partitionID = rtp[0] & 0xf;
        if (partitionID != 0) {
          ++m_partitionID;
          if (m_partitionID != partitionID) {
            PTRACE(3, MY_CODEC_LOG, "Missing partition "
                   "(expected " << m_partitionID << " , got " << partitionID << "),"
                   " ignoring till next key frame.");
            return false;
          }
        }
        else {
          m_partitionID = 0;
          if (!m_fullFrame.empty()) {
            if ((rtp[headerSize] & 0x01) != 0) {
              PTRACE(3, MY_CODEC_LOG, "Start bit seen, but not completed previous frame, ignoring till next key frame.");
              return false;
            }

            PTRACE(3, MY_CODEC_LOG, "Start bit seen, but not completed previous frame, restarting key frame.");
            m_fullFrame.clear();
          }
        }
      }

      Accumulate(rtp.GetPayloadPtr()+headerSize, rtp.GetPayloadSize()-headerSize);
      return true;
    }
};


#if INCLUDE_OM_CUSTOM_PACKETIZATION

class VP8DecoderOM : public VP8Decoder
{
  protected:
    unsigned m_expectedGID;
    Orientation m_orientation;

  public:
    VP8DecoderOM(const PluginCodec_Definition * defn)
      : VP8Decoder(defn)
      , m_expectedGID(UINT_MAX)
      , m_orientation(LandscapeUp)
    {
    }


    struct RotatePlaneInfo : public OutputImagePlaneInfo
    {
      RotatePlaneInfo(unsigned width, unsigned height, int raster, unsigned char * src, unsigned char * dst)
      {
        m_width = width;
        m_height = height;
        m_raster = raster;
        m_source = src;
        m_destination = dst;
      }

      void CopyPortraitLeft()
      {
        for (int y = m_height-1; y >= 0; --y) {
          unsigned char * src = m_source;
          unsigned char * dst = m_destination + y;
          for (int x = m_width; x > 0; --x) {
            *dst = *src++;
            dst += m_height;
          }
          m_source += m_raster;
        }
      }

      void CopyPortraitRight()
      {
        m_destination += m_width*m_height;
        for (int y = m_height; y > 0; --y) {
          unsigned char * src = m_source;
          unsigned char * dst = m_destination - y;
          for (int x = m_width; x > 0; --x) {
            *dst = *src++;
            dst -= m_height;
          }
          m_source += m_raster;
        }
      }

      void CopyLandscapeDown()
      {
        m_destination += m_width*(m_height-1);
        for (int y = m_height; y > 0; --y) {
          memcpy(m_destination, m_source, m_width);
          m_source += m_raster;
          m_destination -= m_width;
        }
      }
    };

    virtual unsigned OutputImage(unsigned char * planes[3], int raster[3],
                                 unsigned width, unsigned height, PluginCodec_RTP & rtp, unsigned & flags)
    {
      unsigned type;
      size_t len;
      unsigned char * ext = rtp.GetExtendedHeader(type, len);
      if (ext != NULL && type == 0x10001) {
        *ext = (unsigned char)(m_orientation << OrientationExtHdrShift);
        return VP8Decoder::OutputImage(planes, raster, width, height, rtp, flags);
      }

      switch (m_orientation) {
        case PortraitLeft :
        case PortraitRight :
          if (CanOutputImage(height, width, rtp, flags))
            break;
          return 0;

        case LandscapeUp :
        case LandscapeDown :
          if (CanOutputImage(width, height, rtp, flags))
            break;
          return 0;

        default :
          return 0;
      }

      RotatePlaneInfo planeInfo[3] = {
        RotatePlaneInfo(width,   height,   raster[0], planes[0], rtp.GetVideoFrameData()),
        RotatePlaneInfo(width/2, height/2, raster[1], planes[1], planeInfo[0].m_destination + width*height),
        RotatePlaneInfo(width/2, height/2, raster[2], planes[2], planeInfo[1].m_destination + width*height/4)
      };

      for (unsigned p = 0; p < 3; ++p) {
        switch (m_orientation) {
          case PortraitLeft :
            planeInfo[p].CopyPortraitLeft();
            break;

          case LandscapeUp :
            planeInfo[p].Copy();
            break;

          case PortraitRight :
            planeInfo[p].CopyPortraitRight();
            break;

          case LandscapeDown :
            planeInfo[p].CopyLandscapeDown();
            break;

          default :
            return 0;
        }
      }

      return (unsigned)rtp.GetPacketSize();
    }


    virtual bool Unpacketise(const PluginCodec_RTP & rtp)
    {
      size_t payloadSize = rtp.GetPayloadSize();
      if (payloadSize < 2) {
        if (payloadSize == 0)
          return true;
        PTRACE(3, MY_CODEC_LOG, "RTP packet too small.");
        return false;
      }

      bool first = (rtp[0]&0x40) != 0;
      if (first)
        m_orientation = (Orientation)((rtp[1] >> OrientationPktHdrShift) & OrientationMask);

      size_t headerSize = first ? 2 : 1;
      if ((rtp[0]&0x80) != 0) {
        while ((rtp[headerSize]&0x80) != 0)
          ++headerSize;
        ++headerSize;
      }

      if (m_ignoreTillKeyFrame) {
        if (!first || (rtp[headerSize]&1) != 0)
          return false;
        m_ignoreTillKeyFrame =  false;
        PTRACE(3, MY_CODEC_LOG, "Found next start of key frame.");
      }
      else {
        if (!first && m_fullFrame.empty()) {
          PTRACE(3, MY_CODEC_LOG, "Missing start to frame, ignoring till next key frame.");
          return false;
        }
      }

      Accumulate(rtp.GetPayloadPtr()+headerSize, payloadSize-headerSize);

      unsigned gid = rtp[0]&0x3f;
      bool expected = m_expectedGID == UINT_MAX || m_expectedGID == gid;
      m_expectedGID = gid;
      if (expected || first)
        return true;

      PTRACE(3, MY_CODEC_LOG, "Unexpected GID " << gid);
      return false;
    }
};

#endif //INCLUDE_OM_CUSTOM_PACKETIZATION


///////////////////////////////////////////////////////////////////////////////

static struct PluginCodec_Definition VP8CodecDefinition[] =
{
  PLUGINCODEC_VIDEO_CODEC_CXX(VP8MediaFormatInfoRFC, VP8EncoderRFC, VP8DecoderRFC),
#if INCLUDE_OM_CUSTOM_PACKETIZATION
  PLUGINCODEC_VIDEO_CODEC_CXX(VP8MediaFormatInfoOM,  VP8EncoderOM,  VP8DecoderOM)
#endif
};

PLUGIN_CODEC_IMPLEMENT_CXX(VP8_CODEC, VP8CodecDefinition);


/////////////////////////////////////////////////////////////////////////////
