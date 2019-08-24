/*
 * sockep.h
 *
 * Media from socket endpoint.
 *
 * Open Phone Abstraction Library (OPAL)
 *
 * Copyright (c) 2018 Vox Lucida Pty. Ltd.
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
 * The Initial Developer of the Original Code is Vox Lucdia Pty. Ltd.
 *
 * Contributor(s): Robert Jongbleod robertj@voxlucida.com.au.
 */

#ifndef OPAL_OPAL_SOCKEP_H
#define OPAL_OPAL_SOCKEP_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif


#include <opal_config.h>

#include <ep/localep.h>


class OpalSocketConnection;

#define OPAL_SOCK_PREFIX "sock"

#define OPAL_OPT_SOCK_EP_AUDIO_IP    "audio-ip"
#define OPAL_OPT_SOCK_EP_AUDIO_PORT  "audio-port"
#define OPAL_OPT_SOCK_EP_AUDIO_PROTO "audio-proto"
#define OPAL_OPT_SOCK_EP_AUDIO_CODEC "audio-codec"

#if OPAL_VIDEO

#define OPAL_OPT_SOCK_EP_VIDEO_IP    "video-ip"
#define OPAL_OPT_SOCK_EP_VIDEO_PORT  "video-port"
#define OPAL_OPT_SOCK_EP_VIDEO_PROTO "video-proto"
#define OPAL_OPT_SOCK_EP_VIDEO_CODEC "video"
#define OPAL_OPT_SOCK_EP_FRAME_RATE  "frame-rate"

#endif // OPAL_VIDEO


/** Socket media endpoint.
    This simply routes media to/from tcp/udp sockets
 */
class OpalSockEndPoint : public OpalLocalEndPoint
{
    PCLASSINFO(OpalSockEndPoint, OpalLocalEndPoint);
  public:
  /**@name Construction */
  //@{
    /**Create a new endpoint.
     */
    OpalSockEndPoint(
      OpalManager & manager,  ///<  Manager of all endpoints.
      const char * prefix = OPAL_SOCK_PREFIX ///<  Prefix for URL style address strings
    );

    /**Destroy endpoint.
     */
    ~OpalSockEndPoint();
  //@}

  /**@name Overrides from OpalLocalEndPoint */
  //@{
    /**Set up a connection to a remote party.
       This is called from the OpalManager::MakeConnection() function once
       it has determined that this is the endpoint for the protocol.

       The general form for this party parameter is:

            [proto:][alias@][transport$]address[:port]

       where the various fields will have meanings specific to the endpoint
       type. For example, with H.323 it could be "h323:Fred@site.com" which
       indicates a user Fred at gatekeeper size.com. Whereas for the PSTN
       endpoint it could be "pstn:5551234" which is to call 5551234 on the
       first available PSTN line.

       The proto field is optional when passed to a specific endpoint. If it
       is present, however, it must agree with the endpoints protocol name or
       false is returned.

       This function usually returns almost immediately with the connection
       continuing to occur in a new background thread.

       If false is returned then the connection could not be established. For
       example if a PSTN endpoint is used and the assiciated line is engaged
       then it may return immediately. Returning a non-NULL value does not
       mean that the connection will succeed, only that an attempt is being
       made.

       The default behaviour is pure.
     */
    virtual PSafePtr<OpalConnection> MakeConnection(
      OpalCall & call,           ///<  Owner of connection
      const PString & party,     ///<  Remote party to call
      void * userData = NULL,    ///<  Arbitrary data to pass to connection
      unsigned int options = 0,  ///<  options to pass to conneciton
      OpalConnection::StringOptions * stringOptions  = NULL ///< Options to pass to connection
    );


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
};


/** Socket media connection.
    This simply routes media to/from tcp/udp sockets
 */
class OpalSockConnection : public OpalLocalConnection
{
    PCLASSINFO(OpalSockConnection, OpalLocalConnection);
  public:
  /**@name Construction */
  //@{
    /**Create a new endpoint.
     */
    OpalSockConnection(
      OpalCall & call,              ///<  Owner calll for connection
      OpalSockEndPoint & endpoint,  ///<  Owner endpoint for connection
      void * userData,    ///<  Arbitrary data to pass to connection
      unsigned options = 0,                    ///< Option bit map to be passed to connection
      OpalConnection::StringOptions * stringOptions = NULL ///< Options to be passed to connection
    );

    /**Destroy endpoint.
     */
    ~OpalSockConnection();
  //@}

  /**@name Overrides from OpalConnection */
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

    /**Clean up the termination of the connection.
       This function can do any internal cleaning up and waiting on background
       threads that may be using the connection object.

       Note that there is not a one to one relationship with the
       OnEstablishedConnection() function. This function may be called without
       that function being called. For example if SetUpConnection() was used
       but the call never completed.

       Classes that override this function should make sure they call the
       ancestor version for correct operation.

       An application will not typically call this function as it is used by
       the OpalManager during a release of the connection.

       The default behaviour calls the OpalEndPoint function of the same name.
      */
    virtual void OnReleased();

    /**Initiate the transfer of an existing call (connection) to a new remote 
       party.

       If remoteParty is a valid call token, then the remote party is transferred
       to that party (consultation transfer) and both calls are cleared.
     */
    virtual bool TransferConnection(
      const PString & remoteParty   ///<  Remote party to transfer the existing call to
    );

    /** Callback for media commands.
        Executes OnMediaCommand ont he other connection in call.

       @returns true if command is handled.
      */
    virtual bool OnMediaCommand(
      OpalMediaStream & stream,         ///< Stream command executed on
      const OpalMediaCommand & command  ///< Media command being executed
    );
  //@}

  /**@name Overrides from OpalLocalConnection */
  //@{
    /**Call back to indicate that remote is ringing.

       The default implementation call OpalLocalEndPoint::OnOutgoingCall().

       @return false if the call is to be aborted with EndedByNoAccept.
      */
    virtual bool OnOutgoing();

    /**Call back to indicate that there is an incoming call.
       Note this function should not block or it will impede the operation of
       the stack.

       The default implementation call OpalLocalEndPoint::OnIncomingCall().

       @return false if the call is to be aborted with status of EndedByLocalBusy.
      */
    virtual bool OnIncoming();

    /**Call back to get media data for transmission.
       If false is returned the media stream will be closed.

       Care with the handling of real time is required, see GetSynchronicity
       for more details.

       The default implementation fills the buffer with zeros and returns true.
      */
    virtual bool OnReadMediaData(
      OpalMediaStream & mediaStream, ///<  Media stream data is required for
      void * data,                   ///<  Data to send
      PINDEX size,                   ///<  Maximum size of data buffer
      PINDEX & length                ///<  Number of bytes placed in buffer
    );

    /**Call back to handle received media data.
       If false is returned the media stream will be closed.

       It is expected that this function be real time. That is if 320 bytes of
       PCM-16 are written, this function should take 20ms to execute. If not
       then the jitter buffer will not operate correctly and audio will not be
       of high quality. This timing can be simulated if required, see
       GetSynchronicity for more details.

       Note: For encoded audio media, if \p data is NULL then that indicates
       there is no incoming audio available from the jitter buffer. The
       application should output silence for a time. The \p written value
       should still contain the bytes of silence emitted, even though it will
       be larger that \p length. This does not occcur for raw (PCM-16) audio.

       The default implementation ignores the media data and returns true.
      */
    virtual bool OnWriteMediaData(
      const OpalMediaStream & mediaStream,    ///<  Media stream data is required for
      const void * data,                      ///<  Data received
      PINDEX length,                          ///<  Amount of data available to write
      PINDEX & written                        ///<  Amount of data written
    );
  //@}


#pragma pack(1)
    /** Over the wire header for the socket interface.
      */
    struct MediaHeader {
      uint8_t  m_headerSize;
      enum {
        Marker = 0x01,
        Update = 0x02,
        Bitrate = 0x04
      };
      uint8_t  m_flags;
      PUInt16b m_length; // Big endian
    };

    struct MediaBitrate : MediaHeader {
      PUInt32b rate;
    };
#pragma pack()


  protected:
    bool OpenMediaSockets();
    bool OpenMediaSocket(
      PIPSocket * & socket,
      const PString & addrKey,
      const PString & portKey,
      const PCaselessString & protoKey
    );

    OpalSockEndPoint & m_endpoint;
    PIPSocket        * m_audioSocket;
#if OPAL_VIDEO
    PIPSocket        * m_videoSocket;
    PDECLARE_MUTEX(m_writeMutex);
#endif
};

#endif // OPAL_OPAL_SOCKEP_H
