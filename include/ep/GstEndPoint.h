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
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#ifndef GSTREAMER_GSTENDPOINT_H
#define GSTREAMER_GSTENDPOINT_H

#include <ep/localep.h>

#include <ptclib/gstreamer.h>

#if OPAL_GSTREAMER

class GstMediaStream;


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

      */
    virtual bool BuildPipeline(
      ostream & newDescription,
      const PString & existingDescription,
      const GstMediaStream & stream
    );

    virtual bool BuildAudioSourcePipeline(ostream & desc, const GstMediaStream & stream);
    virtual bool BuildAudioSinkPipeline(ostream & desc, const GstMediaStream & stream);
    virtual bool BuildAudioSourceDevice(ostream & desc, const GstMediaStream & stream);
    virtual bool BuildAudioSinkDevice(ostream & desc, const GstMediaStream & stream);

    virtual bool BuildEncoder(ostream & desc, const GstMediaStream & stream);
    virtual bool BuildDecoder(ostream & desc, const GstMediaStream & stream);

    static const PString & GetPipelineAudioSourceName();
    static const PString & GetPipelineAudioSinkName();

    void SetAudioSourceDevice(const PString & dev) { m_audioSourceDevice = dev; }
    void SetAudioSinkDevice  (const PString & dev) { m_audioSinkDevice = dev; }
    const PString & GetAudioSourceDevice() const { return m_audioSourceDevice; }
    const PString & GetAudioSinkDevice()   const { return m_audioSinkDevice; }

#if OPAL_VIDEO
    virtual bool BuildVideoSourcePipeline(ostream & desc, const GstMediaStream & stream);
    virtual bool BuildVideoSinkPipeline(ostream & desc, const GstMediaStream & stream);
    virtual bool BuildVideoSourceDevice(ostream & desc, const GstMediaStream & stream);
    virtual bool BuildVideoSinkDevice(ostream & desc, const GstMediaStream & stream);
    static const PString & GetPipelineVideoSourceName();
    static const PString & GetPipelineVideoSinkName();
    void SetVideoSourceDevice(const PString & dev) { m_videoSourceDevice = dev; }
    void SetVideoSinkDevice  (const PString & dev) { m_videoSinkDevice = dev; }
    const PString & GetVideoSourceDevice() const { return m_videoSourceDevice; }
    const PString & GetVideoSinkDevice()   const { return m_videoSinkDevice; }
#endif // OPAL_VIDEO
  //@}

  protected:
    PString m_audioSourceDevice;
    PString m_audioSinkDevice;
#if OPAL_VIDEO
    PString m_videoSourceDevice;
    PString m_videoSinkDevice;
#endif // OPAL_VIDEO

    // Translation tables
    PStringToString     m_MediaFormatToGstEncoder;
    PStringToString     m_MediaFormatToGstDecoder;
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

    virtual void OnReleased();
  //@}

  /**@name Customisation call backs for building GStreamer pipeline */
  //@{
    /**Build the GStreamer pipeline.
       This will call GstEndPoint::BuildPipeline() for each of the source/sink
       pipelines.
      */
    virtual bool BuildPipeline(const GstMediaStream & stream);

    /**DEstroy the GStreamer pipeline.
      */
    virtual void DestroyPipeline(const GstMediaStream & stream);

    /// Get the source pipeline
    PGstPipeline & GetPipeline(bool isSource) { return m_pipeline[isSource]; }
  //@}

  protected:
    GstEndPoint & m_endpoint;
    PGstPipeline  m_pipeline[2]; // The GStreamer pipeline(s) for media: sink=0, source=1
    PString       m_pipedesc[2];
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
  //@}

  /**@name Overrides of OpalMediaStream class */
  //@{
    /**Open the media stream using the media format.
       This calls GstConenction::BuildPipeline() and creates the GStreamer
       pipeline from the resultant desciption string.
      */
    virtual PBoolean Open();

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
  //@}

  protected:
    /**Close any internal components of the stream.
       This should be used in preference to overriding the Close() function as
       it is guaranteed to be called exactly once and avoids race conditions in
       the shut down sequence of a media stream.
      */
    virtual void InternalClose();

    // Member variables.
    GstConnection & m_connection;
    PGstAppSrc      m_pipeSource;
    PGstAppSink     m_pipeSink;
};


#endif // OPAL_GSTREAMER

#endif // GSTREAMER_GSTENDPOINT_H
