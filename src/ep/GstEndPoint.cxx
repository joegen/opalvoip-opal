/*
 * GstEndPoint.cxx
 *
 * GStreamer support.
 *
 * Open Phone Abstraction Library (OPAL)
 * Formally known as the Open H323 project.
 *
 * Copyright (c) 2013 Vox Lucida Pty. Ltd. and Jonathan M. Henson
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
 * The Original Code is Open Phone Abstraction Library.
 *
 * The Initial Developer of the Original Code is Jonathan M. Henson
 * jonathan.michael.henson@gmail.com, jonathan.henson@innovisit.com
 *
 * Contributor(s): Robert Jongbloed (robertj@voxlucida.com.au).
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#include <ptlib.h>

#include <ep/GstEndPoint.h>
#include <codec/vidcodec.h>

#if OPAL_GSTREAMER

#define PTraceModule() "OpalGst"


#define APP_SOURCE_SUFFIX "AppSource"
#define APP_SINK_SUFFIX   "AppSink"

// Note the start of these constanst must be same as OpalMediaType::Audio() & OpalMediaType::Video()
const PString & GstEndPoint::GetPipelineAudioSourceName() { static PConstString const s("audio" APP_SOURCE_SUFFIX); return s; }
const PString & GstEndPoint::GetPipelineAudioSinkName()   { static PConstString const s("audio" APP_SINK_SUFFIX  ); return s; }
#if OPAL_VIDEO
const PString & GstEndPoint::GetPipelineVideoSourceName() { static PConstString const s("video" APP_SOURCE_SUFFIX); return s; }
const PString & GstEndPoint::GetPipelineVideoSinkName()   { static PConstString const s("video" APP_SINK_SUFFIX  ); return s; }
#endif // OPAL_VIDEO


static PConstString const PreferredAudioSourceDevice[] = {
#if _WIN32
//  "wasapisrc",
  "directsoundsrc",
  "dshowaudiosrc",
#else
  "alsasrc",
  "oss4src",
  "osssrc",
  "pulsesrc",
#endif
  "autoaudiosrc" // Note this is in desperation only, if used cannot set rad buffer size
};


static PConstString const PreferredAudioSinkDevice[] = {
#if _WIN32
//  "wasapisink", Deos not work, no idea why
  "directsoundsink",
  "dshowaudiosink",
#else
  "alsasink",
  "oss4sink",
  "osssink",
  "pulsesink",
#endif
  "autoaudiosink"
};


#if OPAL_VIDEO
static PConstString const PreferredVideoSourceDevice[] = {
#if _WIN32
  "dshowvideosrc",
#else
  "video4linux2",
#endif
  "autovideosrc"
};


static PConstString const PreferredVideoSinkDevice[] = {
#if _WIN32
  "dshowvideosink",
  "directdrawsink",
#else
  "xvimagesink",
#endif
  "sdlvideosink",
  "autovideosink"
};
#endif // OPAL_VIDEO


static const PStringToString::Initialiser DefaultGstEncoderToMediaFormat[] = {
  { "mulawenc",   OpalG711_ULAW_64K },
  { "alawenc",    OpalG711_ALAW_64K },
  { "ffenc_g726", OpalG726_40K      },
  { "celtenc",    "CELT-32K"        },

#if OPAL_VIDEO
  { "ffenc_h261",  OpalH261         },
  { "ffenc_h263",  OpalH263         },
  { "ffenc_h263p", OpalH263plus     },
  { "ffenc_mpeg4", OpalMPEG4        },
  { "theoraenc",   "theora"         },
#endif // OPAL_VIDEO
};


static struct GstInitInfo {
  const char * m_mediaFormat;
  const char * m_depacketiser;
  const char * m_decoder;
  const char * m_packetiser;
  const char * m_encoder;
} const DefaultMediaFormatToGStreamer[] = {
  { OpalG711_ULAW_64K, "rtppcmudepay",  "mulawdec",    "rtppcmupay",         "mulawenc"   },
  { OpalG711_ALAW_64K, "rtppcmadepay",  "alawdec",     "rtppcmapay",         "alawenc"    },
  { OpalGSM0610,       "rtpgsmdepay",   "gsmdec",      "rtpgsmpay",          "gsmenc"     },
  { OpalG726_40K,      "rtpg726depay",  "ffdec_g726",  "rtpg726pay",         "ffenc_g726"
                                                                             " bitrate=40000"
  },
  { OpalG726_32K,      "rtpg726depay",  "ffdec_g726",  "rtpg726pay",         "ffenc_g726"
                                                                             " bitrate=32000"
  },
  { OpalG726_24K,      "rtpg726depay",  "ffdec_g726",  "rtpg726pay",         "ffenc_g726"
                                                                             " bitrate=24000"
  },
  { "CELT-32K",        "rtpceltdepay",  "celtdec",     "rtpceltpay",         "celtenc"    },
  { "Speex",           "rtpspeexdepay", "speexdec",    "rtpspeexpay",        "speexenc"   },
#if OPAL_VIDEO
  { OpalH261,          "",              "ffdec_h261",  "",                   "ffenc_h261" },
  { OpalH263,          "rtph263depay",  "ffdec_h263",  "rtph263pay",         "ffenc_h263"
                                                                             " rtp-payload-size=1400"
                                                                             " max-key-interval=125"
                                                                             " me-method=5"
                                                                             " max-bframes=0"
                                                                             " gop-size=125"
                                                                             " rc-buffer-size=65536000"
                                                                             " rc-min-rate=0"
                                                                             " rc-max-rate=1024000"
                                                                             " rc-qsquish=0"
                                                                             " rc-eq=1"
                                                                             " max-qdiff=10"
                                                                             " qcompress=0.5"
                                                                             " i-quant-factor=-0.6"
                                                                             " i-quant-offset=0.0"
                                                                             " me-subpel-quality=8"
                                                                             " qmin=2"
  },
  { OpalH263plus,      "rtph263pdepay", "ffdec_h263",  "rtph263ppay",        "ffenc_h263p"
                                                                             " rtp-payload-size=200"
                                                                             " max-key-interval=125"
                                                                             " me-method=5"
                                                                             " max-bframes=0"
                                                                             " gop-size=125"
                                                                             " rc-buffer-size=65536000"
                                                                             " rc-min-rate=0"
                                                                             " rc-max-rate=1024000"
                                                                             " rc-qsquish=0"
                                                                             " rc-eq=1"
                                                                             " max-qdiff=10"
                                                                             " qcompress=0.5"
                                                                             " i-quant-factor=-0.6"
                                                                             " i-quant-offset=0.0"
                                                                             " me-subpel-quality=8"
                                                                             " qmin=2"
  },
  { OpalH264,          "rtph264depay",  "ffdec_h264",  "rtph264pay"
                                                       " config-interval=5", "x264enc"
                                                                             " byte-stream=true"
                                                                             " bframes=0"
                                                                             " b-adapt=0"
                                                                             " bitrate=1024"
                                                                             " tune=0x4"
                                                                             " speed-preset=3"
                                                                             " sliced-threads=false"
                                                                             " profile=0"
                                                                             " key-int-max=30"
  },
  { "theora",         "rtptheoradepay", "theoradec",   "rtptheorapay",       "theoraenc"
                                                                             " name=videoEncoder"
                                                                             " keyframe-freq=125"
                                                                             " keyframe-force=125"
                                                                             " quality=16"
                                                                             " drop-frames=false"
                                                                             " bitrate=2048"
  },
  { OpalMPEG4,        "rtpmp4vdepay",   "ffdec_mpeg4", "rtpmp4vpay"
                                                       " send-config=true", "ffenc_mpeg4"
                                                                             " quantizer=0.8"
                                                                             " quant-type=1"
  }
#endif // OPAL_VIDEO
};


static PString GetDefaultDevice(const char * klass, PConstString const * preferred, size_t count)
{
  PTRACE(4, "Devices:\n" << setfill('\n') << PGstPluginFeature::Inspect(klass, true));

  PStringArray available = PGstPluginFeature::Inspect(klass, false);
  for (size_t i = 0; i < count; ++i) {
    if (available.GetValuesIndex(preferred[i]) != P_MAX_INDEX)
      return preferred[i];
  }
  return available[0];
}


GstEndPoint::GstEndPoint(OpalManager & manager, const char *prefix)
  : OpalLocalEndPoint(manager, prefix)
  , m_audioSourceDevice(GetDefaultDevice("Source/Audio", PreferredAudioSourceDevice, PARRAYSIZE(PreferredAudioSourceDevice)))
  , m_audioSinkDevice(GetDefaultDevice("Sink/Audio", PreferredAudioSinkDevice, PARRAYSIZE(PreferredAudioSinkDevice)))
#if OPAL_VIDEO
  , m_videoSourceDevice(GetDefaultDevice("Source/Video", PreferredVideoSourceDevice, PARRAYSIZE(PreferredVideoSourceDevice)))
  , m_videoSinkDevice(GetDefaultDevice("Sink/Video", PreferredVideoSinkDevice, PARRAYSIZE(PreferredVideoSinkDevice)))
#endif // OPAL_VIDEO
{
  for (PINDEX i = 0; i < PARRAYSIZE(DefaultMediaFormatToGStreamer); ++i) {
    OpalMediaFormat mediaFormat(DefaultMediaFormatToGStreamer[i].m_mediaFormat);
    if (mediaFormat.IsValid()) {
      GstInfo & info = m_MediaFormatToGStreamer[mediaFormat];
      info.m_encoder      = DefaultMediaFormatToGStreamer[i].m_encoder;
      info.m_packetiser   = DefaultMediaFormatToGStreamer[i].m_packetiser;
      info.m_decoder      = DefaultMediaFormatToGStreamer[i].m_decoder;
      info.m_depacketiser = DefaultMediaFormatToGStreamer[i].m_depacketiser;
    }
  }

  PStringList encoders = PGstPluginFeature::Inspect("Codec/Encoder/Audio", false);
  PStringList decoders = PGstPluginFeature::Inspect("Codec/Decoder/Audio", false);
  PTRACE(4, "Audio encoder elements:\n" << setfill('\n') << PGstPluginFeature::Inspect("Codec/Encoder/Audio", true));
  PTRACE(4, "Audio decoder elements:\n" << setfill('\n') << PGstPluginFeature::Inspect("Codec/Decoder/Audio", true));

#if OPAL_VIDEO
  encoders += PGstPluginFeature::Inspect("Codec/Encoder/Video", false);
  decoders += PGstPluginFeature::Inspect("Codec/Decoder/Video", false);
  PTRACE(4, "Video encoder elements:\n" << setfill('\n') << PGstPluginFeature::Inspect("Codec/Encoder/Video", true));
  PTRACE(4, "Video decoder elements:\n" << setfill('\n') << PGstPluginFeature::Inspect("Codec/Decoder/Video", true));
#endif // OPAL_VIDEO

  PStringToString gstEncoderToMediaFormat(PARRAYSIZE(DefaultGstEncoderToMediaFormat), DefaultGstEncoderToMediaFormat, true);
  for (PStringList::iterator enc = encoders.begin(); enc != encoders.end(); ++enc) {
    OpalMediaFormat mediaFormat = gstEncoderToMediaFormat(*enc);
    if (mediaFormat.IsValid()) {
      GstInfoMap::iterator inf = m_MediaFormatToGStreamer.find(mediaFormat);
      if (inf != m_MediaFormatToGStreamer.end()) {
        for (PStringList::iterator dec = decoders.begin(); dec != decoders.end(); ++dec) {
          if (inf->second.m_decoder.NumCompare(*dec)) {
            m_mediaFormatsAvailable += mediaFormat;
            break;
          }
        }
      }
    }
  }
}


GstEndPoint::~GstEndPoint()
{
}


OpalMediaFormatList GstEndPoint::GetMediaFormats() const
{
  return m_mediaFormatsAvailable;
}


OpalLocalConnection * GstEndPoint::CreateConnection(OpalCall & call,
                                                        void * userData,
                                                    unsigned   options,
                               OpalConnection::StringOptions * stringOptions)
{
  return new GstConnection(call, *this, userData, options, stringOptions);
}


bool GstEndPoint::BuildPipeline(ostream & description, const GstMediaStream & stream)
{
#if OPAL_VIDEO

  bool isAudio;
  {
    OpalMediaType mediaType = stream.GetMediaFormat().GetMediaType();
    if (mediaType == OpalMediaType::Audio())
      isAudio = true;
    else if (mediaType == OpalMediaType::Video())
      isAudio = false;
    else {
      PTRACE(2, "Media type \"" << mediaType << "\" is not supported.");
      return false;
    }
  }

  if (stream.IsSource()) {
    if (isAudio)
      return BuildAudioSourceDevice(description, stream) && BuildAudioSourcePipeline(description, stream);
    else
      return BuildVideoSourceDevice(description, stream) && BuildVideoSourcePipeline(description, stream);
  }
  else {
    if (isAudio)
      return BuildAudioSinkPipeline(description, stream) && BuildAudioSinkDevice(description, stream);
    else
      return BuildVideoSinkPipeline(description, stream) && BuildVideoSinkDevice(description, stream);
  }

#else //OPAL_VIDEO

  if (stream.GetMediaFormat().GetMediaType() != OpalMediaType::Audio()) {
    PTRACE(2, "Media type \"" << mediaType << "\" is not supported.");
    return false;
  }

  if (stream.IsSource())
    return BuildAudioSourceDevice(description, stream) && BuildAudioSourcePipeline(description, stream);
  else
    return BuildAudioSinkPipeline(description, stream) && BuildAudioSinkDevice(description, stream);

#endif //OPAL_VIDEO
}


bool GstEndPoint::BuildAudioSourcePipeline(ostream & desc, const GstMediaStream & stream)
{
  const OpalMediaFormat & mediaFormat = stream.GetMediaFormat();

  desc << " ! "
          "audioconvert"
          " ! "
          "audioresample"
          " ! "
          "audio/x-raw-int, "
          "depth=16, "
          "width=16, "
          "channels=" << mediaFormat.GetOptionInteger(OpalAudioFormat::ChannelsOption(), 1) << ", "
          "rate=" << mediaFormat.GetClockRate();

  if (mediaFormat != OpalPCM16) {
    desc << " ! ";
    if (!BuildEncoder(desc, stream))
      return false;
    desc << " pt=" << (unsigned)mediaFormat.GetPayloadType();
  }

//  desc << "volume name=volumeFilter ! ";

  desc << " ! appsink name=" << GetPipelineAudioSinkName() << " async=true max-buffers=2 drop=true";
  return true;
}


bool GstEndPoint::BuildAudioSinkPipeline(ostream & desc, const GstMediaStream & stream)
{
  const OpalMediaFormat & mediaFormat = stream.GetMediaFormat();

  desc << "appsrc name=" << GetPipelineAudioSourceName() << " ! ";

  if (mediaFormat == OpalPCM16)
    desc << "audio/x-raw-int, "
            "depth=16, "
            "width=16, "
            "channels=" << mediaFormat.GetOptionInteger(OpalAudioFormat::ChannelsOption(), 1) << ", "
            "rate=" << mediaFormat.GetClockRate() << ", "
            "signed=true, "
            "endianness=1234";
  else {
    desc << "application/x-rtp, "
            "media=audio, "
            "payload=" << (unsigned)mediaFormat.GetPayloadType() << ", "
            "clock-rate=" << mediaFormat.GetClockRate() << ", "
            "encoding-name=" << mediaFormat.GetEncodingName() <<
            " ! ";
    if (!BuildDecoder(desc, stream))
      return false;
  }

  desc << " ! ";

  return true;
}


bool GstEndPoint::BuildAudioSourceDevice(ostream & desc, const GstMediaStream & stream)
{
  desc << m_audioSourceDevice << " blocksize="
       << std::max(stream.GetMediaFormat().GetFrameTime()*2, 320U)
       << " name=audioSourceDevice";
  return true;
}


bool GstEndPoint::BuildAudioSinkDevice(ostream & desc, const GstMediaStream & /*stream*/)
{
  desc << m_audioSinkDevice << " name=audioSinkDevice";
  return true;
}


#if OPAL_VIDEO

bool GstEndPoint::BuildVideoSourcePipeline(ostream & desc, const GstMediaStream & stream)
{
  const OpalMediaFormat & mediaFormat = stream.GetMediaFormat();

  desc << " ! ffmpegcolorspace"
          " ! video/x-raw-yuv, "
             "format=(fourcc)I420, "
             "width=" << mediaFormat.GetOptionInteger(OpalVideoFormat::FrameWidthOption()) << ", "
             "height=" << mediaFormat.GetOptionInteger(OpalVideoFormat::FrameHeightOption()) << ", "
             "framerate=(fraction)90000/" << mediaFormat.GetFrameTime()
       << " ! ";

  if (mediaFormat != OpalYUV420P) {
    if (!BuildEncoder(desc, stream))
      return false;

    desc << " mtu=" << mediaFormat.GetOptionInteger(OpalMediaFormat::MaxTxPacketSizeOption(), 1400) <<
            " pt=" << (unsigned)mediaFormat.GetPayloadType()
         << " ! ";
  }

  desc << "appsink name=" << GetPipelineVideoSinkName() << " async=true";
  return true;
}


bool GstEndPoint::BuildVideoSinkPipeline(ostream & desc, const GstMediaStream & stream)
{
  const OpalMediaFormat & mediaFormat = stream.GetMediaFormat();

  desc << "appsrc name=" << GetPipelineVideoSourceName()
       << " ! ";

  if (mediaFormat == OpalYUV420P)
    desc << "video/x-raw-yuv, "
            "format=(fourcc)I420, "
            "width=" << mediaFormat.GetOptionInteger(OpalVideoFormat::FrameWidthOption()) << ", "
            "height=" << mediaFormat.GetOptionInteger(OpalVideoFormat::FrameHeightOption()) << ", "
            "framerate=(fraction)9000/" << mediaFormat.GetFrameTime();
  else {
    desc << "application/x-rtp, "
            "media=video, "
            "payload=" << (unsigned)mediaFormat.GetPayloadType() << ", "
            "clock-rate=" << mediaFormat.GetClockRate() << ", "
            "encoding-name=" << mediaFormat.GetEncodingName()
         << " ! ";
    if (!BuildDecoder(desc, stream))
      return false;
  }

  desc << " ! ffmpegcolorspace ! ";

  return true;
}


bool GstEndPoint::BuildVideoSourceDevice(ostream & desc, const GstMediaStream & /*stream*/)
{
  desc << m_videoSourceDevice << " name=videoSourceDevice";
  return true;
}


bool GstEndPoint::BuildVideoSinkDevice(ostream & desc, const GstMediaStream & /*stream*/)
{
  desc << m_videoSinkDevice << " name=videoSinkDevice";
  return true;
}

#endif // OPAL_VIDEO


bool GstEndPoint::BuildEncoder(ostream & desc, const GstMediaStream & stream)
{
  OpalMediaFormat mediaFormat = stream.GetMediaFormat();
  GstInfoMap::iterator it = m_MediaFormatToGStreamer.find(mediaFormat);
  if (it == m_MediaFormatToGStreamer.end())
    return false;

  OpalMediaType mediaType = mediaFormat.GetMediaType();
  desc << it->second.m_encoder << " name=" << mediaType << "Encoder";
  if (!it->second.m_packetiser.IsEmpty())
    desc << " ! " << it->second.m_packetiser << " name=" << mediaType << "Packetiser";

  return true;
}


bool GstEndPoint::BuildDecoder(ostream & desc, const GstMediaStream & stream)
{
  OpalMediaFormat mediaFormat = stream.GetMediaFormat();
  GstInfoMap::iterator it = m_MediaFormatToGStreamer.find(mediaFormat);
  if (it == m_MediaFormatToGStreamer.end())
    return false;

  OpalMediaType mediaType = mediaFormat.GetMediaType();
  if (!it->second.m_depacketiser.IsEmpty())
    desc << it->second.m_depacketiser << " name=" << mediaType << "Depacketiser ! ";
  desc << it->second.m_decoder << " name=" << mediaType << "Decoder";

  return true;
}


///////////////////////////////////////////////////////////////////////////////

GstConnection::GstConnection(OpalCall & call,
                             GstEndPoint & ep,
                             void * userData,
                             unsigned options,
                             OpalConnection::StringOptions * stringOptions,
                             char tokenPrefix)
  : OpalLocalConnection(call, ep, userData, options, stringOptions, tokenPrefix)
  , m_endpoint(ep)
{
}


void GstConnection::OnReleased()
{
  //we want to release the main window in case the user is using it for other purposes.
  //stream_engine->EndVideoOutput();

  OpalLocalConnection::OnReleased();
}


OpalMediaStream* GstConnection::CreateMediaStream(const OpalMediaFormat & mediaFormat,
                                                  unsigned sessionID,
                                                  PBoolean isSource)
{
  return new GstMediaStream(*this, mediaFormat, sessionID, isSource);
}


bool GstConnection::OpenPipeline(PGstPipeline & pipeline, const GstMediaStream & stream)
{
  PStringStream description;
  if (!m_endpoint.BuildPipeline(description, stream))
    return false;

  if (pipeline.IsValid()) {
    PTRACE(4, "Reconfiguring gstreamer pipeline for " << stream);
    if (!pipeline.SetState(PGstPipeline::Null, 1000)) {
      PTRACE(2, "Could not stop gstreamer pipeline for " << stream);
    }
  }

  bool isSource = stream.IsSource();
  bool isAudio = stream.GetMediaFormat().GetMediaType() == OpalMediaType::Audio();

  OpalMediaStreamPtr otherStream = GetMediaStream(isAudio ? OpalMediaType::Video() : OpalMediaType::Audio(), isSource);
  if (otherStream != NULL) {
    const GstMediaStream & audioStream = isAudio ? stream : dynamic_cast<const GstMediaStream &>(*otherStream);
    const GstMediaStream & videoStream = isAudio ? dynamic_cast<const GstMediaStream &>(*otherStream) : stream;
    PStringStream audioDevice, videoDevice;
    if (isSource) {
      m_endpoint.BuildAudioSourceDevice(audioDevice, audioStream);
      m_endpoint.BuildVideoSourceDevice(videoDevice, videoStream);
    }
    else {
      m_endpoint.BuildAudioSinkDevice(audioDevice, audioStream);
      m_endpoint.BuildVideoSinkDevice(videoDevice, videoStream);
    }
    if (audioDevice == videoDevice) {
      // Parallel pipelines
      PTRACE(2, "Common audio/video source not yet supported");
      return false;
    }
  }

  if (!pipeline.Parse(description)) {
    PTRACE(2, "Failed to parse gstreamer pipeline: \"" << description << '"');
    return false;
  }

  pipeline.SetName(isSource ? "SourcePipeline" : "SinkPipeline");

#if PTRACING
  if (PTrace::CanTrace(5)) {
    ostream & trace = PTRACE_BEGIN(5);
    trace << "Pipeline elements:";
    PGstIterator<PGstElement> it;
    if (pipeline.GetElements(it, true)) {
      do {
        trace << ' ' << it->GetName();
      } while (++it);
    }
    trace << PTrace::End;
  }
#endif // PTRACING

  return true;
}


///////////////////////////////////////////////////////////////////////////////

GstMediaStream::GstMediaStream(GstConnection & conn,
                                 const OpalMediaFormat & mediaFormat,
                                 unsigned sessionID,
                                 bool isSource)
  : OpalMediaStream(conn, mediaFormat, sessionID, isSource)
  , m_connection(conn)
{
}


PBoolean GstMediaStream::Open()
{
  if (IsOpen())
    return true;

  if (!m_connection.OpenPipeline(m_pipeline, *this))
    return false;

  // Our source, their sink, and vice versa
  PString name = mediaFormat.GetMediaType();
  if (IsSource()) {
    name += APP_SINK_SUFFIX;
    if (!m_pipeline.GetByName(name, m_pipeSink)) {
      PTRACE(2, "Could not find " << name << " in pipeline for " << *this);
      return false;
    }
  }
  else {
    name += APP_SOURCE_SUFFIX;
    if (!m_pipeline.GetByName(name, m_pipeSource)) {
      PTRACE(2, "Could not find " << name << " in pipeline for " << *this);
      return false;
    }
    m_pipeSource.SetType(PGstAppSrc::Stream);
  }

  if (!m_pipeline.SetState(PGstElement::Playing)) {
    PTRACE(2, "Failed to start gstreamer pipeline for " << *this);
    m_pipeline.SetNULL();
    return false;
  }

  PTRACE(4, "Opened gstreamer pipeline for " << *this);
  return OpalMediaStream::Open();
}


void GstMediaStream::InternalClose()
{
  PTRACE(4, "Stopping gstreamer pipeline.");
  m_pipeline.SetState(PGstPipeline::Null);
  m_pipeline.SetNULL();
}


PBoolean GstMediaStream::SetDataSize(PINDEX dataSize, PINDEX frameTime)
{
  if (IsSink() || mediaFormat.GetMediaType() != OpalMediaType::Audio())
    return OpalMediaStream::SetDataSize(dataSize, frameTime);

  PINDEX oldSize = GetDataSize();
  if (!OpalMediaStream::SetDataSize(frameTime*2, frameTime))
    return false;

  if (GetDataSize() == oldSize)
    return true;

#if 0
  // For some reason, doing this stops the pipeline dead in it's tracks.
  PGstElement srcDev;
  if (m_connection.GetPipeline(true).GetByName("audioSourceDevice", srcDev))
    srcDev.Set("blocksize", PString(GetDataSize()));
#endif

  return true;
}


PBoolean GstMediaStream::IsSynchronous() const
{
  return IsSource();
}


PBoolean GstMediaStream::ReadPacket(RTP_DataFrame & packet)
{
  if (!IsOpen() || IsSink() || !m_pipeSink.IsValid())
    return false;

  PINDEX size = m_connection.GetEndPoint().GetManager().GetMaxRtpPacketSize();
  if (m_pipeSink.Pull(packet.GetPointer(size), size)) {
    packet.SetPacketSize(size);
    PTRACE(5, "Pulled " << size << " bytes");
    return true;
  }

  PTRACE(2, "Error pulling data to pipeline for " << *this);
  return false;
}


PBoolean GstMediaStream::WritePacket(RTP_DataFrame & packet)
{
  if (!IsOpen() || IsSource() || !m_pipeSource.IsValid())
    return false;

  PINDEX size = packet.GetHeaderSize() + packet.GetPayloadSize();
  PTRACE(5, "Pushing " << size << " bytes");
  if (m_pipeSource.Push(packet.GetPointer(), size))
    return true;

  PTRACE(2, "Error pushing " << size <<" bytes of data to pipeline for " << *this);
  return false;
}


#else // OPAL_GSTREAMER

  #ifdef _MSC_VER
    #pragma message("GStreamer support DISABLED")
  #endif

#endif // OPAL_GSTREAMER
