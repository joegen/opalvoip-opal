/*
 * H.263 Plugin codec for OpenH323/OPAL
 *
 * This code is based on the following files from the OPAL project which
 * have been removed from the current build and distributions but are still
 * available in the CVS "attic"
 * 
 *    src/codecs/h263codec.cxx 
 *    include/codecs/h263codec.h 

 * The original files, and this version of the original code, are released under the same 
 * MPL 1.0 license. Substantial portions of the original code were contributed
 * by Salyens and March Networks and their right to be identified as copyright holders
 * of the original code portions and any parts now included in this new copy is asserted through 
 * their inclusion in the copyright notices below.
 *
 * Copyright (C) 2007 Matthias Schneider
 * Copyright (C) 2006 Post Increment
 * Copyright (C) 2005 Salyens
 * Copyright (C) 2001 March Networks Corporation
 * Copyright (C) 1999-2000 Equivalence Pty. Ltd.
 * Copyright (C) 2010 Vox Lucida Pty. Ltd.
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
 * Contributor(s): Matthias Schneider (ma30002000@yahoo.de)
 *                 Guilhem Tardy (gtardy@salyens.com)
 *                 Craig Southeren (craigs@postincrement.com)
 *                 Robert Jongbloed (robertj@voxlucida.com.au)
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#ifndef PLUGIN_CODEC_DLL_EXPORTS
#include "plugin-config.h"
#endif

#define MY_CODEC FF_H263  // Name of codec (use C variable characters)

#define OPAL_H323 1
#define OPAL_SIP 1

#include "../../../src/codec/h263mf_inc.cxx"


#include "rfc2190.h"
#include "rfc2429.h"

//#include <iomanip>


/////////////////////////////////////////////////////////////////////////////

class MY_CODEC { };

PLUGINCODEC_CONTROL_LOG_FUNCTION_DEF

static const char MyDescription[] = "H.263 (FFMPEG)";

PLUGINCODEC_LICENSE(
  "Matthias Schneider, Craig Southeren"                         // source code author
  "Guilhem Tardy, Derek Smithies",
  "1.0",                                                        // source code version
  "openh323@openh323.org",                                      // source code email
  "http://sourceforge.net/projects/openh323",                   // source code URL
  "Copyright (C) 2007 Matthias Schneider"                       // source code copyright
  ", Copyright (C) 2006 by Post Increment"
  ", Copyright (C) 2005 Salyens"
  ", Copyright (C) 2001 March Networks Corporation"
  ", Copyright (C) 1999-2000 Equivalence Pty. Ltd.",
  "MPL 1.0",                                                    // source code license
  PluginCodec_License_MPL,                                      // source code license
  
  MyDescription,                                                // codec description
  "Michael Niedermayer, Fabrice Bellard",                       // codec author
  "",                                                           // codec version
  "ffmpeg-devel-request@mplayerhq.hu",                          // codec email
  "http://ffmpeg.mplayerhq.hu",                                 // codec URL
  "Copyright (c) 2000-2001 Fabrice Bellard"                     // codec copyright information
  ", Copyright (c) 2002-2003 Michael Niedermayer",
  "GNU LESSER GENERAL PUBLIC LICENSE, Version 2.1, February 1999", // codec license
  PluginCodec_License_LGPL                                         // codec license code
);


static struct PluginCodec_Option const sqcifMPI =
{
  PluginCodec_IntegerOption,            // Option type
  SQCIF_MPI,                            // User visible name
  false,                                // User Read/Only flag
  PluginCodec_MaxMerge,                 // Merge mode
  "1",                                  // Initial value
  "SQCIF",                              // FMTP option name
  STRINGIZE(PLUGINCODEC_MPI_DISABLED),  // FMTP default value
  0,                                    // H.245 generic capability code and bit mask
  "1",                                  // Minimum value
  STRINGIZE(PLUGINCODEC_MPI_DISABLED)   // Maximum value
};

static struct PluginCodec_Option const qcifMPI =
{
  PluginCodec_IntegerOption,            // Option type
  QCIF_MPI,                             // User visible name
  false,                                // User Read/Only flag
  PluginCodec_MaxMerge,                 // Merge mode
  "1",                                  // Initial value
  "QCIF",                               // FMTP option name
  STRINGIZE(PLUGINCODEC_MPI_DISABLED),  // FMTP default value
  0,                                    // H.245 generic capability code and bit mask
  "1",                                  // Minimum value
  STRINGIZE(PLUGINCODEC_MPI_DISABLED)   // Maximum value
};

static struct PluginCodec_Option const cifMPI =
{
  PluginCodec_IntegerOption,            // Option type
  CIF_MPI,                              // User visible name
  false,                                // User Read/Only flag
  PluginCodec_MaxMerge,                 // Merge mode
  "1",                                  // Initial value
  "CIF",                                // FMTP option name
  STRINGIZE(PLUGINCODEC_MPI_DISABLED),  // FMTP default value
  0,                                    // H.245 generic capability code and bit mask
  "1",                                  // Minimum value
  STRINGIZE(PLUGINCODEC_MPI_DISABLED)   // Maximum value
};

static struct PluginCodec_Option const cif4MPI =
{
  PluginCodec_IntegerOption,            // Option type
  CIF4_MPI,                             // User visible name
  false,                                // User Read/Only flag
  PluginCodec_MaxMerge,                 // Merge mode
  "1",                                  // Initial value
  "CIF4",                               // FMTP option name
  STRINGIZE(PLUGINCODEC_MPI_DISABLED),  // FMTP default value
  0,                                    // H.245 generic capability code and bit mask
  "1",                                  // Minimum value
  STRINGIZE(PLUGINCODEC_MPI_DISABLED)   // Maximum value
};

static struct PluginCodec_Option const cif16MPI =
{
  PluginCodec_IntegerOption,            // Option type
  CIF16_MPI,                            // User visible name
  false,                                // User Read/Only flag
  PluginCodec_MaxMerge,                 // Merge mode
  "1",                                  // Initial value
  "CIF16",                              // FMTP option name
  STRINGIZE(PLUGINCODEC_MPI_DISABLED),  // FMTP default value
  0,                                    // H.245 generic capability code and bit mask
  "1",                                  // Minimum value
  STRINGIZE(PLUGINCODEC_MPI_DISABLED)   // Maximum value
};

static struct PluginCodec_Option const maxBR =
{
  PluginCodec_IntegerOption,          // Option type
  "MaxBR",                            // User visible name
  false,                              // User Read/Only flag
  PluginCodec_MinMerge,               // Merge mode
  "0",                                // Initial value
  "maxbr",                            // FMTP option name
  "0",                                // FMTP default value
  0,                                  // H.245 generic capability code and bit mask
  "0",                                // Minimum value
  "32767"                             // Maximum value
};

static struct PluginCodec_Option const mediaPacketization =
{
  PluginCodec_StringOption,           // Option type
  PLUGINCODEC_MEDIA_PACKETIZATION,    // User visible name
  true,                               // User Read/Only flag
  PluginCodec_EqualMerge,             // Merge mode
  RFC2190                             // Initial value
};

static struct PluginCodec_Option const mediaPacketizationPlus =
{
  PluginCodec_StringOption,           // Option type
  PLUGINCODEC_MEDIA_PACKETIZATIONS,   // User visible name
  true,                               // User Read/Only flag
  PluginCodec_IntersectionMerge,      // Merge mode
  RFC2429                             // Initial value
};


static int MergeCustomH263(char ** result, const char * dest, const char * src)
{
  char buffer[MAX_H263_CUSTOM_SIZES*20];
  if (!MergeCustomResolution(dest, src, buffer))
    return false;

  *result = strdup(buffer);
  return true;
}

static void FreeString(char * str)
{
  free(str);
}

static struct PluginCodec_Option const customMPI =
{
  PluginCodec_StringOption,           // Option type
  PLUGINCODEC_CUSTOM_MPI,             // User visible name
  false,                              // User Read/Only flag
  PluginCodec_CustomMerge,            // Merge mode
  DEFAULT_CUSTOM_MPI,                 // Initial value
  "CUSTOM",                           // FMTP option name
  DEFAULT_CUSTOM_MPI,                 // FMTP default value
  0,                                  // H.245 generic capability code and bit mask
  NULL,                               // Minimum value
  NULL,                               // Maximum value
  MergeCustomH263,
  FreeString
};

static struct PluginCodec_Option const annexF =
  { PluginCodec_BoolOption,    H263_ANNEX_F,   false, PluginCodec_AndMerge, "1", "F", "0" };

static struct PluginCodec_Option const annexI =
  { PluginCodec_BoolOption,    H263_ANNEX_I,   false, PluginCodec_AndMerge, "1", "I", "0" };

static struct PluginCodec_Option const annexJ =
  { PluginCodec_BoolOption,    H263_ANNEX_J,   false, PluginCodec_AndMerge, "1", "J", "0" };

static struct PluginCodec_Option const annexK =
  { PluginCodec_IntegerOption, H263_ANNEX_K,   false, PluginCodec_MinMerge, "0", "K", "0", 0, "0", "4" };

static struct PluginCodec_Option const annexN =
  { PluginCodec_BoolOption,    H263_ANNEX_N,   true,  PluginCodec_AndMerge, "0", "N", "0" };

static struct PluginCodec_Option const annexT =
  { PluginCodec_BoolOption,    H263_ANNEX_T,   true,  PluginCodec_AndMerge, "0", "T", "0" };

static struct PluginCodec_Option const annexD =
  { PluginCodec_BoolOption,    H263_ANNEX_D,   false, PluginCodec_AndMerge, "1", "D", "0" };

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

static struct PluginCodec_Option const * const OptionTable_RFC22429[] = {
  &mediaPacketizationPlus,
  &maxBR,
  &qcifMPI,
  &cifMPI,
  &sqcifMPI,
  &cif4MPI,
  &cif16MPI,
  &customMPI,
  &annexF,
  &annexI,
  &annexJ,
  &annexK,
  &annexN,
  &annexT,
  &annexD,
  &TemporalSpatialTradeOff,
  NULL
};


static struct PluginCodec_Option const * const OptionTable_RFC2190[] = {
  &mediaPacketization,
  &maxBR,
  &qcifMPI,
  &cifMPI,
  &sqcifMPI,
  &cif4MPI,
  &cif16MPI,
  &annexF,
  NULL
};


/////////////////////////////////////////////////////////////////////////////

class H263_PluginMediaFormat : public PluginCodec_VideoFormat<MY_CODEC>
{
public:
  typedef PluginCodec_VideoFormat<MY_CODEC> BaseClass;

  H263_PluginMediaFormat(const char * formatName, const char * encodingName, OptionsTable options)
    : BaseClass(formatName, encodingName, MyDescription, H263_BITRATE, options)
  {
    m_h323CapabilityType = PluginCodec_H323VideoCodec_h263;
  }


  virtual bool ToNormalised(OptionMap & original, OptionMap & changed)
  {
    return ClampSizes(original, changed);
  }


  virtual bool ToCustomised(OptionMap & original, OptionMap & changed)
  {
    return ClampSizes(original, changed);
  }
}; 

static H263_PluginMediaFormat MyMediaFormatInfo_RFC2190(H263FormatName,     H263EncodingName,     OptionTable_RFC2190);
static H263_PluginMediaFormat MyMediaFormatInfo_RFC2429(H263plusFormatName, H263plusEncodingName, OptionTable_RFC22429);


/////////////////////////////////////////////////////////////////////////////

#if TRACE_RTP

static void DumpRTPPayload(std::ostream & trace, const PluginCodec_RTP & rtp, unsigned max)
{
  if (max > rtp.GetPayloadSize())
    max = rtp.GetPayloadSize();
  BYTE * ptr = rtp.GetPayloadPtr();
  trace << std::hex << std::setfill('0') << std::setprecision(2);
  while (max-- > 0) 
    trace << (int) *ptr++ << ' ';
  trace << std::setfill(' ') << std::dec;
}

static void RTPDump(std::ostream & trace, const PluginCodec_RTP & rtp)
{
  trace << "seq=" << rtp.GetSequenceNumber()
       << ",ts=" << rtp.GetTimestamp()
       << ",mkr=" << rtp.GetMarker()
       << ",pt=" << (int)rtp.GetPayloadType()
       << ",ps=" << rtp.GetPayloadSize();
}

static void RFC2190Dump(std::ostream & trace, const PluginCodec_RTP & rtp)
{
  RTPDump(trace, rtp);
  if (rtp.GetPayloadSize() > 2) {
    bool iFrame = false;
    char mode;
    BYTE * payload = rtp.GetPayloadPtr();
    if ((payload[0] & 0x80) == 0) {
      mode = 'A';
      iFrame = (payload[1] & 0x10) == 0;
    }
    else if ((payload[0] & 0x40) == 0) {
      mode = 'B';
      iFrame = (payload[4] & 0x80) == 0;
    }
    else {
      mode = 'C';
      iFrame = (payload[4] & 0x80) == 0;
    }
    trace << "mode=" << mode << ",I=" << (iFrame ? "yes" : "no");
  }
  trace << ",data=";
  DumpRTPPayload(trace, rtp, 10);
}

static void RFC2429Dump(std::ostream & trace, const PluginCodec_RTP & rtp)
{
  RTPDump(trace, rtp);
  trace << ",data=";
  DumpRTPPayload(trace, rtp, 10);
}

#define CODEC_TRACER_RTP(text, rtp, func) \
  if (PTRACE_CHECK(6)) { \
    std::ostringstream strm; strm << text; func(strm, rtp); \
    PluginCodec_LogFunctionInstance(6, __FILE__, __LINE__, m_prefix, strm.str().c_str()); \
  } else (void)0

#else

#define CODEC_TRACER_RTP(text, rtp, func) 

#endif
      

/////////////////////////////////////////////////////////////////////////////

class H263_Base_Encoder : public PluginVideoEncoder<MY_CODEC>, public FFMPEGCodec
{
  typedef PluginVideoEncoder<MY_CODEC> BaseClass;

  public:
    H263_Base_Encoder(const PluginCodec_Definition * defn, const char * prefix, EncodedFrame * packetizer)
      : BaseClass(defn)
      , FFMPEGCodec(prefix, packetizer)
    { 
      PTRACE(4, m_prefix, "Created encoder $Revision$");
    }


    bool SetOption(const char * option, const char * value)
    {
      // Annex D: Unrestructed Motion Vectors
      // Level 2+ 
      // works with eyeBeam, signaled via  non-standard "D"
      if (strcasecmp(option, H263_ANNEX_D) == 0)
        return SetOptionBit(m_context->flags, CODEC_FLAG_H263P_UMV, value);

      // Annex F: Advanced Prediction Mode
      // does not work with eyeBeam
    #if 1 // DO NOT ENABLE THIS FLAG. FFMPEG IS NOT THREAD_SAFE WHEN THIS FLAG IS SET
      if (strcasecmp(option, H263_ANNEX_F) == 0)
        return SetOptionBit(m_context->flags, CODEC_FLAG_OBMC, value);
    #endif

      // Annex I: Advanced Intra Coding
      // Level 3+
      // works with eyeBeam
      if (strcasecmp(option, H263_ANNEX_I) == 0)
        return SetOptionBit(m_context->flags, CODEC_FLAG_AC_PRED, value);

      // Annex J: Deblocking Filter
      // works with eyeBeam
      if (strcasecmp(option, H263_ANNEX_J) == 0)
        return SetOptionBit(m_context->flags, CODEC_FLAG_LOOP_FILTER, value);

      // Annex K: Slice Structure
      // does not work with eyeBeam
      if (strcasecmp(option, H263_ANNEX_K) == 0)
        return SetOptionBit(m_context->flags, CODEC_FLAG_H263P_SLICE_STRUCT, value);

      // Annex S: Alternative INTER VLC mode
      // does not work with eyeBeam
      if (strcasecmp(option, H263_ANNEX_S) == 0)
        return SetOptionBit(m_context->flags, CODEC_FLAG_H263P_AIV, value);

      if (strcasecmp(option, PLUGINCODEC_MEDIA_PACKETIZATION) == 0 ||
          strcasecmp(option, PLUGINCODEC_MEDIA_PACKETIZATIONS) == 0) {
        if (strstr(value, m_fullFrame->GetName()) == NULL) {
          PTRACE(4, m_prefix, "Packetisation changed to " << value);
          delete m_fullFrame;
          if (STRCMPI(value, "RFC2429") == 0)
            m_fullFrame = new RFC2429Frame;
          else
            m_fullFrame = new RFC2190Packetizer;
        }
        return true;
      }

      return BaseClass::SetOption(option, value);
    }


    virtual bool OnChangedOptions()
    {
      CloseCodec();

      if (!m_fullFrame->SetResolution(m_width, m_height)) {
        PTRACE(1, m_prefix, "Unable to allocate memory for packet buffer");
        return false;
      }

      SetResolution(m_width, m_height);
      SetEncoderOptions(m_frameTime, m_maxBitRate, m_maxRTPSize, m_tsto, m_keyFramePeriod);
      m_fullFrame->SetMaxPayloadSize(m_context->rtp_payload_size);
      PTRACE(4, m_prefix, "Packetization is " << m_fullFrame->GetName());

      #define CODEC_TRACER_FLAG(tracer, flag) \
        PTRACE(4, m_prefix, #flag " is " << ((m_context->flags & flag) ? "enabled" : "disabled"));
      CODEC_TRACER_FLAG(tracer, CODEC_FLAG_H263P_UMV);
      CODEC_TRACER_FLAG(tracer, CODEC_FLAG_OBMC);
      CODEC_TRACER_FLAG(tracer, CODEC_FLAG_AC_PRED);
      CODEC_TRACER_FLAG(tracer, CODEC_FLAG_H263P_SLICE_STRUCT)
      CODEC_TRACER_FLAG(tracer, CODEC_FLAG_LOOP_FILTER);
      CODEC_TRACER_FLAG(tracer, CODEC_FLAG_H263P_AIV);

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


    bool GetStatistics(char * stats, unsigned maxSize)
    {
      if (m_picture == NULL)
        return false;

      snprintf(stats, maxSize, "Quality=%i\n", m_picture->quality);
      stats[maxSize-1] = '\0';
      return true;
    }
};


/////////////////////////////////////////////////////////////////////////////

class H263_RFC2190_Encoder : public H263_Base_Encoder
{
  public:
    H263_RFC2190_Encoder(const PluginCodec_Definition * defn)
      : H263_Base_Encoder(defn, "H.263-RFC2190", new RFC2190Packetizer())
    {
    }


    bool Construct()
    {
      if (!InitEncoder(CODEC_ID_H263))
        return false;

    #if LIBAVCODEC_RTP_MODE
      m_context->rtp_mode = 1;
    #endif

      m_context->rtp_payload_size = PluginCodec_RTP_MaxPayloadSize;

      m_context->flags &= ~CODEC_FLAG_H263P_UMV;
      m_context->flags &= ~CODEC_FLAG_4MV;
    #if LIBAVCODEC_RTP_MODE
      m_context->flags &= ~CODEC_FLAG_H263P_AIC;
    #endif
      m_context->flags &= ~CODEC_FLAG_H263P_AIV;
      m_context->flags &= ~CODEC_FLAG_H263P_SLICE_STRUCT;

      return true;
    }
};


/////////////////////////////////////////////////////////////////////////////

class H263_RFC2429_Encoder : public H263_Base_Encoder
{
  public:
    H263_RFC2429_Encoder(const PluginCodec_Definition * defn)
      : H263_Base_Encoder(defn, "H.263-RFC2429", new RFC2429Frame)
    {
    }


    bool Construct()
    {
      return InitEncoder(CODEC_ID_H263P);
    }
};


/////////////////////////////////////////////////////////////////////////////

class H263_Base_Decoder : public PluginVideoDecoder<MY_CODEC>, public FFMPEGCodec
{
  typedef PluginVideoDecoder<MY_CODEC> BaseClass;

  public:
    H263_Base_Decoder(const PluginCodec_Definition * defn, const char * prefix, EncodedFrame * depacketizer)
      : BaseClass(defn)
      , FFMPEGCodec(prefix, depacketizer)
    {
      PTRACE(4, m_prefix, "Created decoder $Revision$");
    }


    bool Construct()
    {
      m_fullFrame->Reset();
      return InitDecoder(CODEC_ID_H263) && OpenCodec();
    }


    bool SetOption(const char * option, const char * value)
    {
      if (strcasecmp(option, PLUGINCODEC_MEDIA_PACKETIZATION) == 0 ||
          strcasecmp(option, PLUGINCODEC_MEDIA_PACKETIZATIONS) == 0) {
        if (strstr(value, m_fullFrame->GetName()) == NULL) {
          PTRACE(4, m_prefix, "Packetisation changed to " << value);
          delete m_fullFrame;
          if (strcasecmp(value, "RFC2429") == 0)
            m_fullFrame = new RFC2429Frame;
          else
            m_fullFrame = new RFC2190Depacketizer;
        }
      }

      return BaseClass::SetOption(option, value);
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


    bool GetStatistics(char * stats, size_t maxSize)
    {
      if (m_picture == NULL)
        return false;

      snprintf(stats, maxSize, "Quality=%i\n", m_picture->quality);
      stats[maxSize-1] = '\0';
      return true;
    }
};


///////////////////////////////////////////////////////////////////////////////////

class H263_RFC2429_Decoder : public H263_Base_Decoder
{
  public:
    H263_RFC2429_Decoder(const PluginCodec_Definition * defn)
      : H263_Base_Decoder(defn, "H.263-RFC2429", new RFC2429Frame)
    {
    }
};


/////////////////////////////////////////////////////////////////////////////

class H263_RFC2190_Decoder : public H263_Base_Decoder
{
  public:
    H263_RFC2190_Decoder(const PluginCodec_Definition * defn)
      : H263_Base_Decoder(defn, "H.263-RFC2190", new RFC2190Depacketizer)
    {
    }
};


/////////////////////////////////////////////////////////////////////////////

static struct PluginCodec_Definition MyCodecDefinition[] =
{
  PLUGINCODEC_VIDEO_CODEC_CXX(MyMediaFormatInfo_RFC2190, H263_RFC2190_Encoder, H263_RFC2190_Decoder),
  PLUGINCODEC_VIDEO_CODEC_CXX(MyMediaFormatInfo_RFC2429, H263_RFC2429_Encoder, H263_RFC2429_Decoder)
};


PLUGIN_CODEC_IMPLEMENT_CXX(MY_CODEC, MyCodecDefinition);


/////////////////////////////////////////////////////////////////////////////
