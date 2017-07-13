/*
 * GstEndPoint.h
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
 */

#ifndef GSTREAMER_GSTENDPOINT_H
#define GSTREAMER_GSTENDPOINT_H

#include <ep/localep.h>

#include <ptclib/gstreamer.h>

#if OPAL_GSTREAMER

class GstMediaStream;


#define OPAL_GST_STRINTF_FMT "{%s}"
#define OPAL_GST_NAME        "name"
#define OPAL_GST_SAMPLE_RATE "sample-rate"
#define OPAL_GST_PT          "pt"
#define OPAL_GST_MTU         "mtu"
#define OPAL_GST_WIDTH       "width"
#define OPAL_GST_HEIGHT      "height"
#define OPAL_GST_FRAME_RATE  "frame-rate"
#define OPAL_GST_BIT_RATE    "bit-rate"
#define OPAL_GST_BIT_RATE_K  "bit-rate-kbps"
#define OPAL_GST_BLOCK_SIZE  "blocksize"
#define OPAL_GST_LATENCY     "latency"
#define OPAL_GST_REMOTE_IP   "remote-ip"
#define OPAL_GST_REMOTE_PPRT "remote-port"


/**Endpoint for performing OPAL media via gstreamer.
   This class exposes a gstreamer media stream which supports multiple encoded
   formats as well as raw audio and video. To use this end point, create an
   instance in your OpalManager descendant. Then route the connections like so:

      AddRouteEntry("gst:.* = sip:<da>");
      AddRouteEntry("sip:.* = gst:");
      AddRouteEntry("gst:.* = h323:<da>");
      AddRouteEntry("h323:.* = gst:");

   To create your own descendant of GSTMediaStream which is more specific to
   your platform and specific needs, create a descendant of this class and
   override the various BuildXXXXPipeline() functions.
 */
class GstEndPoint : public OpalLocalEndPoint
{
    PCLASSINFO(GstEndPoint, OpalLocalEndPoint);
  public:
  /**@name Construction */
  //@{
    /**Create a new endpoint.
     */
    GstEndPoint(
      OpalManager & manager,
      const char *prefix = "gst"
    );

    /**Destroy endpoint.
     */
    virtual ~GstEndPoint();
  //@}

  /**@name Overrides from OpalEndPoint */
  //@{
    /**Get the data formats this endpoint is capable of operating.
       This provides a list of media data format names that may be used by an
       OpalMediaStream may be created by a connection from this endpoint.

       Note that a specific connection may not actually support all of the
       media formats returned here, but should return no more.

       The default behaviour returns the most basic media formats, PCM audio
       and YUV420P video.
      */
    virtual OpalMediaFormatList GetMediaFormats() const;

    /**Create a connection for the PCSS endpoint.
       The default implementation is to create a OpalLocalConnection.
      */
    virtual OpalLocalConnection * CreateConnection(
      OpalCall & call,    ///<  Owner of connection
      void * userData,    ///<  Arbitrary data to pass to connection
      unsigned options,   ///< Option bit mask to pass to connection
      OpalConnection::StringOptions * stringOptions ///< Options to pass to connection
    );
  //@}

  /**@name Customisation call backs for building GStreamer pipeline */
  //@{
    /**Build the GStreamer pipeline description string.
       Note that this may be called twice, once for audio and once for video,
       and not necessarily in that order. This mecahnism is to allow for
       things like media files where the source (or sink) for audio and video
       is the same and must be "teed" into two sub-pipes.

       Thus, if \p existingDescription is non-empty, its source (or sink)
       element is checked for if is identical to the other of audio/video as
       returned by BuildAudioSourceDevice()/BuildVideoSourceDevice() and a
       split pipeline formed.

       The OPAL "end" of each pipline is always appsrc (or appsink) and must
       have the names defined by GetPipelineAudioSourceName(),
       GetPipelineAudioSinkName(), GetPipelineVideoSourceName() and
       GetPipelineVideoSinkName().

       If the BuildEncoder() and BuildDecoder() functions are used (default)
       then strings with the format {XXX} are substituted for dynbamic values.
       XXX may be:
          name          Name of element
          pt            Payload type number
          mtu           Maximum transmission unit size
          sample-rate   Audio sample rate
          width         Video width
          height        Video hight
          frame-rate    Video frames/second
      */
    virtual bool BuildPipeline(
      ostream & description,
      const GstMediaStream * audioStream
#if OPAL_VIDEO
      , const GstMediaStream * videoStream
#endif
    );

    /* Configure the pipeline when it is already in NULL state */
    virtual bool ConfigurePipeline(PGstPipeline & pipeline, const GstMediaStream & stream);

    virtual bool BuildAudioSourcePipeline(ostream & desc, const GstMediaStream & stream, int rtpIndex);
    virtual bool BuildAudioSinkPipeline(ostream & desc, const GstMediaStream & stream, int rtpIndex);

    virtual bool OutputRTPSource(ostream & desc, const GstMediaStream & stream, int rtpIndex);
    virtual bool OutputRTPSink(ostream & desc, const GstMediaStream & stream, int rtpIndex);

    bool SetAudioSourceDevice(const PString & elementName);
    virtual bool BuildAudioSourceDevice(ostream & desc, const GstMediaStream & stream);
    const PString & GetAudioSourceDevice() const { return m_audioSourceDevice; }

    bool SetAudioSinkDevice(const PString & elementName);
    const PString & GetAudioSinkDevice() const { return m_audioSinkDevice; }
    virtual bool BuildAudioSinkDevice(ostream & desc, const GstMediaStream & stream);

    bool SetJitterBufferPipeline(const PString & elementName);
    const PString & GetJitterBufferPipeline()   const { return m_jitterBuffer; }
    static const PString & GetPipelineJitterBufferName();
    virtual bool BuildJitterBufferPipeline(ostream & desc, const GstMediaStream & stream);

    bool SetRTPPipeline(const PString & elementName);
    const PString & GetRTPPipeline()   const { return m_rtpbin; }
    static const PString & GetPipelineRTPName();
    virtual bool BuildRTPPipeline(ostream & desc, const GstMediaStream & stream, unsigned index);

    static const PString & GetPipelineAudioSourceName();
    virtual bool BuildAppSource(ostream & desc, const PString & name);

    static const PString & GetPipelineAudioSinkName();
    virtual bool BuildAppSink(ostream & desc, const PString & name, int rtpIndex);

    static const PString & GetPipelineVolumeName();
    virtual bool BuildVolume(ostream & desc, const PString & name);

#if OPAL_VIDEO
    virtual bool BuildVideoSourcePipeline(ostream & desc, const GstMediaStream & stream, int rtpIndex);
    virtual bool BuildVideoSinkPipeline(ostream & desc, const GstMediaStream & stream, int rtpIndex);

    bool SetVideoSourceDevice(const PString & elementName);
    const PString & GetVideoSourceDevice() const { return m_videoSourceDevice; }
    virtual bool BuildVideoSourceDevice(ostream & desc, const GstMediaStream & stream);

    bool SetVideoSinkDevice(const PString & elementName);
    const PString & GetVideoSinkDevice() const { return m_videoSinkDevice; }
    virtual bool BuildVideoSinkDevice(ostream & desc, const GstMediaStream & stream);

    bool SetVideoSourceColourConverter(const PString & elementName);
    const PString & GetVideoSourceColourConverter()   const { return m_videoSourceColourConverter; }

    bool SetVideoSinkColourConverter(const PString & elementName);
    const PString & GetVideoSinkColourConverter()   const { return m_videoSinkColourConverter; }

    static const PString & GetPipelineVideoSourceName();
    static const PString & GetPipelineVideoSinkName();
#endif // OPAL_VIDEO

    /// GStreamer codec partial pipelines
    struct CodecPipelines {
      PString m_encoder;      ///< Encoder pipeline element, e.g. "ffenc_g726 bitrate=32000"
      PString m_packetiser;   ///< RTP packetisation element e.g. "rtph264pay config-interval=5"
      PString m_decoder;      ///< Decoder pipeline element, e.g. "ffdec_h263"
      PString m_depacketiser; ///< RTP depacketisation element e.g. "rtpgsmdepay"
    };

    /** Set the mapping between an OpalMediaFormat and the GStreamer pipeline text.

        Returns: false if the pipeline elements are not available in the
                 gstreamer run time.
      */
    bool SetMapping(
      const OpalMediaFormat & mediaFormat,  ///< Media format to map
      const CodecPipelines & info           ///< GStreamer partial pipelines to map
    );

    /// Get the mapping between an OpalMediaFormat and the GStreamer pipeline text
    bool GetMapping(
      const OpalMediaFormat & mediaFormat,  ///< Media format to map
      CodecPipelines & info                 ///< GStreamer partial pipelines to map
    ) const;

    virtual bool BuildEncoder(ostream & desc, const GstMediaStream & stream);
    virtual bool BuildDecoder(ostream & desc, const GstMediaStream & stream);
  //@}

  protected:
    PString m_rtpbin;
    PString m_jitterBuffer;
    PString m_audioSourceDevice;
    PString m_audioSinkDevice;
#if OPAL_VIDEO
    PString m_videoSourceDevice;
    PString m_videoSinkDevice;
    PString m_videoSourceColourConverter;
    PString m_videoSinkColourConverter;
#endif // OPAL_VIDEO

    // Translation table
    typedef map<OpalMediaFormat, CodecPipelines> CodecPipelineMap;

    CodecPipelineMap    m_MediaFormatToGStreamer;
    OpalMediaFormatList m_mediaFormatsAvailable;
};


/**Connection for performing OPAL media via gstreamer.
  */
class GstConnection : public OpalLocalConnection
{
    PCLASSINFO(GstConnection, OpalLocalConnection);
  public:
  /**@name Construction */
  //@{
    /**Create a new connection.
     */
    GstConnection(
      OpalCall & call,
      GstEndPoint & ep,
      void * /*userData*/,
      unsigned options,
      OpalConnection::StringOptions * stringOptions,
      char tokenPrefix = 'G'
    );
  //@}

  /**@name Overrides from OpalConnection */
  //@{
    /**Open a new media stream.
       This will create a media stream of an appropriate subclass as required
       by the underlying connection protocol. For instance H.323 would create
       an OpalRTPStream.

       The sessionID parameter may not be needed by a particular media stream
       and may be ignored. In the case of an OpalRTPStream it us used.

       Note that media streams may be created internally to the underlying
       protocol. This function is not the only way a stream can come into
       existance.

       The default behaviour creates a GstMediaStream instance.
     */
    virtual OpalMediaStream * CreateMediaStream(
      const OpalMediaFormat & mediaFormat,
      unsigned sessionID,
      PBoolean isSource
    );

    /**Set the volume (gain) for the audio media channel.
       The volume range is 0 == muted, 100 == LOUDEST.
      */
    virtual PBoolean SetAudioVolume(
      PBoolean source,        ///< true for source (microphone), false for sink (speaker)
      unsigned percentage     ///< Gain, 0=silent, 100=maximun
    );

    /**Get the volume (gain) for the audio media channel.
       The volume range is 0 == muted, 100 == LOUDEST.
      */
    virtual PBoolean GetAudioVolume(
      PBoolean source,       ///< true for source (microphone), false for sink (speaker)
      unsigned & percentage  ///< Gain, 0=silent, 100=maximun
    );

    /**Set the mute state for the audio media channel.
      */
    virtual bool SetAudioMute(
      bool source,        ///< true for source (microphone), false for sink (speaker)
      bool mute           ///< Flag for muted audio
    );

    /**Get the mute state for the audio media channel.
      */
    virtual bool GetAudioMute(
      bool source,        ///< true for source (microphone), false for sink (speaker)
      bool & mute         ///< Flag for muted audio
    );
  //@}

  /**@name Customisation call backs for building GStreamer pipeline */
  //@{
    /**Open the GStreamer pipeline.
       This will call GstEndPoint::BuildPipeline() for each of the source/sink
       pipelines.
      */
    virtual bool OpenPipeline(
      PGstPipeline & pipeline,
      const GstMediaStream & stream
    );

    virtual bool ConfigurePipeline(PGstPipeline & pipeline, const GstMediaStream & stream);
  //@}

  protected:
    GstEndPoint & m_endpoint;
};


/**Media stream for performing OPAL media via gstreamer.
  */
class GstMediaStream : public OpalMediaStream
{
    PCLASSINFO(GstMediaStream, OpalMediaStream);
  public:
  /**@name Construction */
  //@{
    /**Construct a new media stream for GStreamer session.
      */
    GstMediaStream(
      GstConnection & conn,                ///<  Connection that owns the stream
      const OpalMediaFormat & mediaFormat, ///<  Media format for stream
      unsigned sessionID,                  ///<  Session number for stream
      bool isSource                        ///<  Is a source stream
    );
    ~GstMediaStream();
  //@}

  /**@name Overrides of OpalMediaStream class */
  //@{
    /**Open the media stream using the media format.
       This calls GstConenction::BuildPipeline() and creates the GStreamer
       pipeline from the resultant desciption string.
      */
    virtual PBoolean Open();

    /**Start the media stream.

       The default behaviour calls Resume() on the associated OpalMediaPatch
       thread if it was suspended.
      */
    virtual PBoolean Start();

    /**Read an RTP frame of data from the source media stream.
       The default behaviour simply calls ReadData() on the data portion of the
       RTP_DataFrame and sets the frames timestamp and marker from the internal
       member variables of the media stream class.
      */
    virtual PBoolean ReadPacket(
      RTP_DataFrame & packet
    );

    /**Write an RTP frame of data to the sink media stream.
       The default behaviour simply calls WriteData() on the data portion of the
       RTP_DataFrame and and sets the internal timestamp and marker from the
       member variables of the media stream class.
      */
    virtual PBoolean WritePacket(
      RTP_DataFrame & packet
    );

    /**Set the data size in bytes that is expected to be used. Some media
       streams can make use of this information to perform optimisations.

       The default behaviour does nothing.
      */
    virtual PBoolean SetDataSize(
      PINDEX dataSize,  ///< New data size (in total)
      PINDEX frameTime  ///< Individual frame time (if applicable)
    );

    /**Indicate if the media stream is synchronous.
       If this returns true then the media stream will block of the amount of
       time it takes to annunciate the data. For example if the media stream
       is over a sound card, and 480 bytes of data are to be written it will
       take 30 milliseconds to complete.
      */
    virtual PBoolean IsSynchronous() const;

    /**Indicate if the media stream requires a OpalMediaPatch thread (active patch).
       This is called on the source/sink stream and is passed the sink/source
       stream that the patch will initially be using. The function could
       conditionally require the patch thread to execute a thread reading and
       writing data, or prevent  it from doing so as it can do so in hardware
       in some way.
      */
    virtual PBoolean RequiresPatchThread(
      OpalMediaStream * stream  ///< Other stream in patch
    ) const;

    /**Indicate media transport is required.
       One of the two streams in the patch can indicate that media transport is
       not required as it is somehow being bypassed.

       In this case we are bypassing the media transport read thread as the
       gstreamer piipeline is reading the socket directly itself.
      */
    virtual bool RequireMediaTransportThread(
      OpalMediaStream & stream  ///< Other stream in patch
    ) const;

    /**Set the volume (gain) for the audio media channel.
       The volume range is 0 == muted, 100 == LOUDEST.
      */
    virtual bool SetAudioVolume(
      unsigned percentage     ///< Gain, 0=silent, 100=maximun
    );

    /**Get the volume (gain) for the audio media channel.
       The volume range is 0 == muted, 100 == LOUDEST.
      */
    virtual bool GetAudioVolume(
      unsigned & percentage  ///< Gain, 0=silent, 100=maximun
    );

    /**Set the mute state for the audio media channel.
      */
    virtual bool SetAudioMute(
      bool mute           ///< Flag for muted audio
    );

    /**Get the mute state for the audio media channel.
      */
    virtual bool GetAudioMute(
      bool & mute         ///< Flag for muted audio
    );
  //@}

  protected:
    /**Close any internal components of the stream.
       This should be used in preference to overriding the Close() function as
       it is guaranteed to be called exactly once and avoids race conditions in
       the shut down sequence of a media stream.
      */
    virtual void InternalClose();
    virtual bool InternalSetPaused(bool pause, bool fromUser, bool fromPatch);

    bool StartPlaying(
      PGstElement::States & state
    );

    // Member variables.
    GstConnection & m_connection;
    PGstPipeline    m_pipeline;
    PGstAppSrc      m_pipeSource; // Must be after m_pipeline so is destroyed before it
    PGstAppSink     m_pipeSink;   // Ditto
    PGstElement     m_pipeVolume;
};


#endif // OPAL_GSTREAMER

#endif // GSTREAMER_GSTENDPOINT_H
