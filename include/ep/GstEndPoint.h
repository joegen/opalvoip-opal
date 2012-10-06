/*
 * GstEndPoint.h
 *
 *  Created on: Jun 20, 2011
 *  Author: Jonathan M. Henson
 *  Contact: jonathan.michael.henson@gmail.com, jonathan.henson@innovisit.com
 *  Description: This class exposes a gstreamer media stream which supports multiple encoded formats as well as raw audio and video.
 *  To use this end point, create an instance in your OpalManager descendant.
 *  Then route the connections like so:
 *
 *  AddRouteEntry("gst:.* = sip:<da>");
 *  AddRouteEntry("sip:.* = gst:");
 *  AddRouteEntry("gst:.* = h323:<da>");
 *  AddRouteEntry("h323:.* = gst:");
 *
 *  It depends on GSTMediaStream. See GSTMediaStream.h for more information.
 *
 *  To create your own descendant of GSTMediaStream which is more specific to your platform and specific needs,
 *  create a descendant of this class and override CreateGstMediaStream() returning your GSTMediaStream descendant.
 */

#ifndef GSTREAMER_GSTENDPOINT_H
#define GSTREAMER_GSTENDPOINT_H

#include <ep/localep.h>

#include <ptclib/gstreamer.h>

#if P_GSTREAMER

/**Endpoint for access via gstreamer.
  */
class GstEndPoint : public OpalLocalEndPoint
{
    PCLASSINFO(GstEndPoint, OpalLocalEndPoint);
  public:
    GstEndPoint(
      OpalManager & manager,
      const char *prefix = "gst"
    );
    virtual ~GstEndPoint();

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
};


class GstConnection : public OpalLocalConnection
{
    PCLASSINFO(GstConnection, OpalLocalConnection);
  public:
    GstConnection(
      OpalCall & call,
      GstEndPoint & ep,
      void * /*userData*/,
      unsigned options,
      OpalConnection::StringOptions * stringOptions,
      char tokenPrefix = 'G'
    );

    virtual OpalMediaStream * CreateMediaStream(
      const OpalMediaFormat & mediaFormat,
      unsigned sessionID,
      PBoolean isSource
    );

    virtual void OnReleased();
};


class GstMediaStream : public OpalMediaStream
{
    PCLASSINFO(GstMediaStream, OpalMediaStream);
  public:
    GstMediaStream(
      GstConnection & conn,                ///<  Connection that owns the stream
      const OpalMediaFormat & mediaFormat, ///<  Media format for stream
      unsigned sessionID,                  ///<  Session number for stream
      bool isSource                        ///<  Is a source stream
    );

    /**Open the media stream using the media format.

       The default behaviour simply sets the isOpen variable to true.
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

    /**Indicate if the media stream is synchronous.
       If this returns true then the media stream will block of the amount of
       time it takes to annunciate the data. For example if the media stream
       is over a sound card, and 480 bytes of data are to be written it will
       take 30 milliseconds to complete.
      */
    virtual PBoolean IsSynchronous() const;

    virtual bool BuildAudioSourcePipeline(ostream & desc);
    virtual bool BuildAudioSinkPipeline(ostream & desc);
    virtual bool BuildVideoSourcePipeline(ostream & desc);
    virtual bool BuildVideoSinkPipeline(ostream & desc);
    virtual bool BuildOtherMediaPipeline(ostream & desc);

    virtual bool BuildAudioSourceDevice(ostream & desc);
    virtual bool BuildAudioSinkDevice(ostream & desc);
    virtual bool BuildVideoSourceDevice(ostream & desc);
    virtual bool BuildVideoSinkDevice(ostream & desc);

  protected:
    /**Close any internal components of the stream.
       This should be used in preference to overriding the Close() function as
       it is guaranteed to be called exactly once and avoids race conditions in
       the shut down sequence of a media stream.
      */
    virtual void InternalClose();


    PGstPipeline m_pipeline;
};

#endif // P_GSTREAMER

#endif // GSTREAMER_GSTENDPOINT_H
