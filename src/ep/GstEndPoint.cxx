/*
 * GstEndPoint.cpp
 *
 *  Created on: Jun 20, 2011
 *  Author: Jonathan M. Henson
 *  Contact: jonathan.michael.henson@gmail.com, jonathan.henson@innovisit.com
 *  Description: This is the implementation of the GstEndPoint class which is declared in GstEndPoint.h.
 *  See the header file for more information.
 */

#include <ptlib.h>

#include <ep/GstEndPoint.h>
#include <codec/vidcodec.h>

#if P_GSTREAMER

static char const PipelineSourceName[] = "PipelineSource";
static char const PipelineSinkName[] = "PipelineSink";


GstEndPoint::GstEndPoint(OpalManager & manager, const char *prefix)
  : OpalLocalEndPoint(manager, prefix)
{
}


GstEndPoint::~GstEndPoint()
{
}


OpalMediaFormatList GstEndPoint::GetMediaFormats() const
{
  OpalMediaFormatList list;

  PStringList elements = PGstPluginFeature::Inspect("Codec/Encoder/Audio", true);
  PTRACE(4, "OpalGst\tAudio elements by:\n" << setfill('\n') << elements);

  elements = PGstPluginFeature::Inspect("Codec/Encoder/Video", true);
  PTRACE(4, "OpalGst\tVideo elements by:\n" << setfill('\n') << elements);


  return list;
}


OpalLocalConnection * GstEndPoint::CreateConnection(OpalCall & call,
                                                        void * userData,
                                                    unsigned   options,
                               OpalConnection::StringOptions * stringOptions)
{
  return new GstConnection(call, *this, userData, options, stringOptions);
}


///////////////////////////////////////////////////////////////////////////////

GstConnection::GstConnection(OpalCall & call,
                             GstEndPoint & ep,
                             void * userData,
                             unsigned options,
                             OpalConnection::StringOptions * stringOptions,
                             char tokenPrefix)
  : OpalLocalConnection(call, ep, userData, options, stringOptions, tokenPrefix)
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


///////////////////////////////////////////////////////////////////////////////

GstMediaStream::GstMediaStream(GstConnection & conn,
                                 const OpalMediaFormat & mediaFormat,
                                 unsigned sessionID,
                                 bool isSource)
  : OpalMediaStream(conn, mediaFormat, sessionID, isSource)
{
}


PBoolean GstMediaStream::Open()
{
  if (IsOpen())
    return true;

  PStringStream pipeline;

  bool ok;

  OpalMediaType mediaType = mediaFormat.GetMediaType();
  if (mediaFormat == OpalMediaType::Audio())
    if (IsSource())
      ok = BuildAudioSourcePipeline(pipeline);
    else
      ok = BuildAudioSinkPipeline(pipeline);
  else if (mediaFormat == OpalMediaType::Video())
    if (IsSource())
      ok = BuildVideoSourcePipeline(pipeline);
    else
      ok = BuildVideoSinkPipeline(pipeline);
  else
    ok = BuildOtherMediaPipeline(pipeline);

  if (!ok)
    return false;

  if (m_pipeline.Parse(pipeline))
    return true;

  PTRACE(2, "GstMedia", "Failed to open gstreamer pipeline: \"" << pipeline << '"');
  return false;
}


void GstMediaStream::InternalClose()
{
  m_pipeline.SetState(PGstElement::Paused);
}


PBoolean GstMediaStream::IsSynchronous() const
{
  return false;
}


bool GstMediaStream::BuildAudioSourcePipeline(ostream & desc)
{
  if (!BuildAudioSourceDevice(desc))
    return false;

  desc << " name=soundSrc"
          "!audioconvert"
          "!audioresample"
          "!audio/x-raw-int"
          ",depth=16"
          ",width=16"
          ",channels=" << mediaFormat.GetOptionInteger(OpalAudioFormat::ChannelsOption(), 1) <<
          ",rate=" << mediaFormat.GetClockRate() <<
          '!';

  if (mediaFormat.GetName().Find("peex") != P_MAX_INDEX)
    desc << "speexenc name=audioEncoder!rtpspeexpay";
  else if (mediaFormat.GetName().Find("ELT") != P_MAX_INDEX)
    desc << "celtenc name=audioEncoder!rtpceltpay";
  else if (mediaFormat == OpalG726_40K)
    desc << "ffenc_g726 name=audioEncoder!rtpg726pay";
  else if (mediaFormat == OpalGSM0610)
    desc << "gsmenc name=audioEncoder!rtpgsmpay";
  else if (mediaFormat == OpalG711_ALAW_64K)
    desc << "alawenc name=audioEncoder!rtppcmapay";
  else if (mediaFormat == OpalG711_ULAW_64K)
    desc << "mulawenc name=audioEncoder ! rtppcmupay";
  else if (mediaFormat != OpalPCM16)
    return false;
  desc << " name=audioRtpPay pt=" << (unsigned)mediaFormat.GetPayloadType() << " !";

//  desc << "volume name=volumeFilter ! ";

  desc << " tee name=t ! queue ! appsink name=soundSink max-buffers=10 drop=true t. "
          " ! queue ! multiudpsink send-duplicates=false name=audioUdpSink";
  return true;
}


bool GstMediaStream::BuildAudioSinkPipeline(ostream & desc)
{
  desc <<  "appsrc is-live=true name=" << PipelineSourceName <<
          '!';

  if (mediaFormat != OpalPCM16)
    desc <<  "audio/x-raw-int"
            ",depth=16"
            ",width=16"
            ",channels=" << mediaFormat.GetOptionInteger(OpalAudioFormat::ChannelsOption(), 1) <<
            ",rate=" << mediaFormat.GetClockRate() <<
            ",signed=true"
            ",endianness=1234";
  else {
    desc <<  "application/x-rtp"
            ",media=audio"
            ",payload=" << (unsigned)mediaFormat.GetPayloadType() <<
            ",clock-rate=" << mediaFormat.GetClockRate() <<
            ",encoding-name=" << mediaFormat.GetEncodingName() <<
            '!';
    if (mediaFormat.GetName().Find("peex") != P_MAX_INDEX)
      desc << "rtpspeexdepay name=audioRtpDepay!speexdec";
    else if(mediaFormat.GetName().Find("ELT") != P_MAX_INDEX)
      desc <<  "rtpceltdepay name=audioRtpDepay"
              "!celtdec";
    else if (mediaFormat == OpalG726_40K)
      desc << "rtpg726depay name=audioRtpDepay!ffdec_g726";
    else if (mediaFormat == OpalGSM0610)
      desc << "rtpgsmdepay name=audioRtpDepay!gsmdec";
    else if (mediaFormat == OpalG711_ALAW_64K)
      desc << "rtppcmadepay name=audioRtpDepay!alawdec";
    else if (mediaFormat == OpalG711_ULAW_64K)
      desc << "rtppcmudepay name=audioRtpDepay!mulawdec";
    else
      return false;
  }

  desc << '|';

  if (!BuildAudioSinkDevice(desc))
    return false;

  desc << " name=" << PipelineSinkName;
  return true;
}


bool GstMediaStream::BuildVideoSourcePipeline(ostream & desc)
{
  if (!BuildVideoSourceDevice(desc))
    return false;

  desc << " name=videoSrc"
          "!video/x-raw-yuv"
          ",format=(fourcc)I420"
          ",width=" << mediaFormat.GetOptionInteger(OpalVideoFormat::FrameWidthOption()) <<
          ",height=" << mediaFormat.GetOptionInteger(OpalVideoFormat::FrameHeightOption()) <<
          ",framerate=(fraction)90000/" << mediaFormat.GetFrameTime() <<
          "!videobalance name=VideoBalance"
          "!textoverlay name=chanNameFilter"
          "!textoverlay name=osdMessageFilter"
          "!textoverlay name=sessionTimerOverlay"
          "!";

  if (mediaFormat != OpalYUV420P) {
    if (mediaFormat == OpalH261)
      desc << "ffenc_h261 name=videoRtpPay";
    else if (mediaFormat == OpalH263)
      desc << "ffenc_h263 name=videoEncoder"
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
              "!rtph263pay";
    else if (mediaFormat == OpalH263plus)
      desc << "ffenc_h263p name=videoEncoder"
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
              "!rtph263ppay";
    else if (mediaFormat == OpalH264)
      desc << "x264enc name=videoEncoder"
              " byte-stream=true"
              " bframes=0"
              " b-adapt=0"
              " bitrate=1024"
              " tune=0x4"
              " speed-preset=3"
              " sliced-threads=false"
              " profile=0"
              " key-int-max=30"
              "!rtph264pay config-interval=5";
    else if (mediaFormat.GetName().Find("heora") != P_MAX_INDEX)
      desc << "theoraenc name=videoEncoder"
              " keyframe-freq=125"
              " keyframe-force=125"
              " quality=16"
              " drop-frames=false"
              " bitrate=2048"
              "!rtptheorapay";
    else if (mediaFormat == OpalMPEG4)
      desc << "ffenc_mpeg4 name=videoEncoder"
              " quantizer=0.8"
              " quant-type=1"
              "!rtpmp4vpay send-config=true config-interval=5";
    else
      return false;
    desc << " name=videoRtpPay"
            " mtu=" << mediaFormat.GetOptionInteger(OpalMediaFormat::MaxTxPacketSizeOption(), 1400) <<
            " pt=" << (unsigned)mediaFormat.GetPayloadType();
  }

  desc << "!tee name=t"
          "!queue"
          "!appsink name=videoAppSink max-buffers=10 drop=true sync=false t."
          "!queue"
          "!multiudpsink send-duplicates=false name=videoUdpSink sync=false";
  return true;
}


bool GstMediaStream::BuildVideoSinkPipeline(ostream & desc)
{
  desc << "appsrc is-live=true name=" << PipelineSourceName <<
          '!';

  if (mediaFormat == OpalYUV420P)
    desc << "video/x-raw-yuv"
            ",format=(fourcc)I420"
            ",width=" << mediaFormat.GetOptionInteger(OpalVideoFormat::FrameWidthOption()) <<
            ",height=" << mediaFormat.GetOptionInteger(OpalVideoFormat::FrameHeightOption()) <<
            ",framerate=(fraction)90000/" << mediaFormat.GetFrameTime();
  else if (mediaFormat == OpalH261)
    desc << "video/x-h261"
            ",width=" << mediaFormat.GetOptionInteger(OpalVideoFormat::FrameWidthOption()) <<
            ",height=" << mediaFormat.GetOptionInteger(OpalVideoFormat::FrameHeightOption()) <<
            ",framerate=(fraction)90000/" << mediaFormat.GetFrameTime() <<
            "!ffdec_h261";
  else {
    desc << "application/x-rtp"
            ",media=video"
            ",payload=" << (unsigned)mediaFormat.GetPayloadType() <<
            ",clock-rate=" << mediaFormat.GetClockRate() <<
            ",encoding-name=" << mediaFormat.GetEncodingName() <<
            '!';
    if (mediaFormat == OpalH263)
      desc << "rtph263depay name=videoRtpDepay!ffdec_h263";
    else if (mediaFormat == OpalH263plus)
      desc << "rtph263pdepay name=videoRtpDepay!ffdec_h263p";
    else if (mediaFormat == OpalH264)
      desc << "rtph264depay name=videoRtpDepay!ffdec_h264";
    else if (mediaFormat.GetName().Find("heora") != P_MAX_INDEX)
      desc << "rtptheoradepay name=videoRtpDepay!theoradec";
    else if (mediaFormat == OpalMPEG4)
      desc << "rtpmp4vdepay name=videoRtpDepay!ffdec_mpeg4";
  }

  desc << '!';

  if (!BuildVideoSinkDevice(desc))
    return false;

  desc << " name=" << PipelineSinkName;
  return true;
}


bool GstMediaStream::BuildOtherMediaPipeline(ostream &)
{
  return false;
}


bool GstMediaStream::BuildAudioSourceDevice(ostream & desc)
{
  desc << "autoaudiosrc";
  return true;
}


bool GstMediaStream::BuildAudioSinkDevice(ostream & desc)
{
  desc << "autoaudiosink";
  return true;
}


bool GstMediaStream::BuildVideoSourceDevice(ostream & desc)
{
  desc << "autovideosrc";
  return true;
}


bool GstMediaStream::BuildVideoSinkDevice(ostream & desc)
{
  desc << "autovideosink";
  return true;
}


PBoolean GstMediaStream::ReadPacket(RTP_DataFrame & packet)
{
  if (!IsOpen() || IsSink())
    return false;

  PGstAppSink pipeSink(m_pipeline, PipelineSinkName);
  if (!pipeSink.IsValid()) {
    PTRACE(2, "OpalGst\tError getting sink " << PipelineSinkName << " from pipeline");
    return false;
  }

  PINDEX size = packet.GetSize();
  if (pipeSink.Pull(packet.GetPointer(), size)) {
    packet.SetSize(size);
    return true;
  }

  PTRACE(2, "OpalGst\tError pulling data to pipeline");
  return false;
}


PBoolean GstMediaStream::WritePacket(RTP_DataFrame & packet)
{
  if (!IsOpen() || IsSource())
    return false;

  PGstAppSrc pipeSrc(m_pipeline, PipelineSourceName);
  if (!pipeSrc.IsValid()) {
    PTRACE(2, "OpalGst\tError getting source " << PipelineSourceName << " from pipeline");
    return false;
  }

  if (pipeSrc.Push(packet.GetPointer(), packet.GetSize()))
    return true;

  PTRACE(2, "OpalGst\tError pushing data to pipeline");
  return false;
}

#endif // P_GSTREAMER
