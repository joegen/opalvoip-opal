/*
 * sdpep.h
 *
 * Open Phone Abstraction Library (OPAL)
 *
 * Copyright (C) 2014 Vox Lucida Pty. Ltd.
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
 * The Initial Developer of the Original Code is Post Increment
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#ifndef OPAL_OPAL_SDPEP_H
#define OPAL_OPAL_SDPEP_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <opal_config.h>

#if OPAL_SDP

#include <rtp/rtpep.h>
#include <rtp/rtpconn.h>
#include <sdp/sdp.h>


class OpalSDPConnection;
class OpalSDPHTTPConnection;


/**Enable audio/video grouping in SDP.
   This add various identifiers to the SDP to link the audio and video
   medis sessions together. For example, to do lip sync.

   Defaults to true.
*/
#define OPAL_OPT_AV_GROUPING "AV-Grouping"

/**OpalConnection::StringOption key to a string for a regular expression to
   match the product information, which if matching the remote system, will
   indicate the remote does not support asymmetric hold as required by the
   standard.
   
   This fault is when SDP sendonly is sent (us putting them on hold), and
   they reply inactive, which implies them putting us on hold. When we
   subsequently send recvonly to release our hold to them, they continue to
   send inactive, and hold is never released.

   Note the OpalProductInfo vendor, name & version strings are concatenated
   before comparison with the regular expression.

   Defaults to empty string.
  */
#define OPAL_OPT_SYMMETRIC_HOLD_PRODUCT "Symmetric-Hold-Product"


/**Base class for endpoint types that use SDP for media transport.
   Protocols such as SIP, RTSP or WebRTC.
  */
class OpalSDPEndPoint : public OpalRTPEndPoint
{
    PCLASSINFO(OpalSDPEndPoint, OpalRTPEndPoint);
  public:
    static const PCaselessString & ContentType();


  /**@name Construction */
  //@{
    /**Create a new endpoint.
     */
    OpalSDPEndPoint(
      OpalManager & manager,          ///<  Manager of all endpoints.
      const PCaselessString & prefix, ///<  Prefix for URL style address strings
      Attributes attributes           ///<  Bit mask of attributes endpoint has
    );

    /**Destroy the endpoint.
     */
    ~OpalSDPEndPoint();
  //@}

  /**@name Actions */
    /**Create a SDP instance for the SIP packet.
       The default implementation is to create a SDPSessionDescription.
      */
    virtual SDPSessionDescription * CreateSDP(
      time_t sessionId,
      unsigned version,
      const OpalTransportAddress & address
    );
  //@}

  /**@name Member variables */
    void SetHoldTimeout(
      const PTimeInterval & t
    ) { m_holdTimeout = t; }
    const PTimeInterval & GetHoldTimeout() const { return m_holdTimeout; }
  //@}

  protected:
    PTimeInterval m_holdTimeout;
};


///////////////////////////////////////////////////////////////////////////////

/**This is the base class for OpalConnections that use SDP sessions, 
   such as SIP, RTSP or WebRTC.
 */
class OpalSDPConnection : public OpalRTPConnection
{
    PCLASSINFO(OpalSDPConnection, OpalRTPConnection);
  public:

  /**@name Construction */
  //@{
  /**Create a new connection.
   */
  OpalSDPConnection(
    OpalCall & call,                         ///<  Owner calll for connection
    OpalSDPEndPoint & endpoint,              ///<  Owner endpoint for connection
    const PString & token,                   ///<  Token to identify the connection
    unsigned options = 0,                    ///<  Connection options
    OpalConnection::StringOptions * stringOptions = NULL     ///< more complex options
    );

  /**Destroy connection.
   */
  ~OpalSDPConnection();
  //@}

  /**@name Overrides from OpalConnection */
  //@{
    /**Get indication of connection being to a "network".
       This indicates the if the connection may be regarded as a "network"
       connection. The distinction is about if there is a concept of a "remote"
       party being connected to and is best described by example: sip, h323,
       iax and pstn are all "network" connections as they connect to something
       "remote". While pc, pots and ivr are not as the entity being connected
       to is intrinsically local.
      */
    virtual bool IsNetworkConnection() const { return true; }

    /**Get the data formats this endpoint is capable of operating in.
      */
    virtual OpalMediaFormatList GetMediaFormats() const;

    /**Put the current connection on hold, suspending media streams.
       The streams from the remote are always paused. The streams from the
       local to the remote are conditionally paused depending on underlying
       logic for "music on hold" functionality.

       The \p fromRemote parameter indicates if we a putting the remote on
       hold (false) or it is a request for the remote to put us on hold (true).

       The /p placeOnHold parameter indicates of the command/request is for
       going on hold or retrieving from hold.

       @return true if hold request successfully initiated. The OnHold() call
               back must be monitored for final confirmation of hold state.
     */
    virtual bool HoldRemote(
      bool placeOnHold  ///< Flag for setting on or off hold
    );

    /**Return true if the current connection is on hold.
       The \p fromRemote parameter indicates if we are testing if the remote
       system has us on hold, or we have them on hold.
     */
    virtual bool IsOnHold(
      bool fromRemote  ///< Flag for if remote has us on hold, or we have them
    ) const;
  //@}

  /**@name Actions */
  //@{
    /// Generate an offer.
    PString GetOfferSDP(
      bool offerOpenMediaStreamsOnly = false
    );

    /// Handle an offer from remote, provide answer
    PString AnswerOfferSDP(
      const PString & offer
    );

    /// Handle an answer from remote
    bool HandleAnswerSDP(
      const PString & answer
    );

#if OPAL_SRTP
    /// Indicate if SRTP is available to be sued in this connection.
    virtual bool CanDoSRTP() const;
#endif

    /// Get the media local interface to initialise the RTP session.
    virtual PString GetMediaInterface() = 0;

    /// Get the remote media address to initialise the RTP session on making offer.
    virtual OpalTransportAddress GetRemoteMediaAddress() = 0;
  //@}

  protected:
    virtual bool OnSendOfferSDP(
      SDPSessionDescription & sdpOut,
      bool offerOpenMediaStreamsOnly
    );
    virtual bool OnSendOfferSDPSession(
      unsigned sessionID,
      SDPSessionDescription & sdpOut,
      bool offerOpenMediaStreamOnly
    );

    virtual bool OnSendAnswerSDP(
      const SDPSessionDescription & sdpOffer,
      SDPSessionDescription & sdpAnswer
    );
    virtual SDPMediaDescription * OnSendAnswerSDPSession(
      SDPMediaDescription * incomingMedia,
      unsigned sessionId,
      SDPMediaDescription::Direction otherSidesDir
    );

    virtual bool OnReceivedAnswerSDP(
      const SDPSessionDescription & sdp,
      bool & multipleFormats
    );
    virtual bool OnReceivedAnswerSDPSession(
      const SDPSessionDescription & sdp,
      unsigned sessionId,
      bool & multipleFormats
    );

    virtual OpalMediaSession * SetUpMediaSession(
      const unsigned rtpSessionId,
      const OpalMediaType & mediaType,
      const SDPMediaDescription & mediaDescription,
      OpalTransportAddress & localAddress,
      bool & remoteChanged
    );
#if OPAL_VIDEO
    void SetAudioVideoGroup();
#endif

    void RetryHoldRemote(bool placeOnHold);
    virtual bool OnHoldStateChanged(bool placeOnHold);
    virtual void OnMediaStreamOpenFailed(bool rx);

    OpalSDPEndPoint & m_endpoint;

    OpalMediaFormatList m_remoteFormatList;
    OpalMediaFormatList m_answerFormatList;

    enum HoldState {
      eHoldOff,
      eRetrieveInProgress,

      // Order is important!
      eHoldOn,
      eHoldInProgress
    };
    HoldState m_holdToRemote;
    bool      m_holdFromRemote;
};

#endif // OPAL_SDP


///////////////////////////////////////////////////////////////////////////////

#if OPAL_SDP_HTTP

/**Endpoint for SDP over HTTP POST command (used for WebRTC).
  */
class OpalSDPHTTPEndPoint : public OpalSDPEndPoint
{
    PCLASSINFO(OpalSDPHTTPEndPoint, OpalSDPEndPoint);
  public:
    enum
    {
      DefaultPort = 5080,
      DefaultSecurePort = 5081
    };

  /**@name Construction */
  //@{
    /**Create a new endpoint.
     */
    OpalSDPHTTPEndPoint(
      OpalManager & manager,          ///<  Manager of all endpoints.
      const PCaselessString & prefix = "sdp" ///<  Prefix for URL style address strings
    );

    /**Destroy the endpoint.
     */
    ~OpalSDPHTTPEndPoint();
  //@}

  /**@name Overrides from OpalEndPoint */
  //@{
    /**Get comma separated list of transport protocols to create if
       no explicit listeners started.
       Transport protocols are as per OpalTransportAddress, e.g. "udp$". It
       may also have a ":port" after it if that transport type does not use
       the value from GetDefaultSignalPort().
      */
    virtual PString GetDefaultTransport() const;

    /**Get the default signal port for this endpoint.
     */
    virtual WORD GetDefaultSignalPort() const;

    /**Handle new incoming connection from listener.
      */
    virtual void NewIncomingConnection(
      OpalListener & listener,            ///<  Listner that created transport
      const OpalTransportPtr & transport  ///<  Transport connection came in on
    );

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
      OpalCall & call,                         ///<  Owner of connection
      const PString & party,                   ///<  Remote party to call
      void * userData,                         ///<  Arbitrary data to pass to connection
      unsigned int options,                    ///<  options to pass to conneciton
      OpalConnection::StringOptions * stringOptions  ///<  complex string options
    );
  //@}

  /**@name Customisation call backs */
  //@{
    /**Create a connection for the SDP over HTTP endpoint.
       The default implementation is to create a OpalSDPHTTPConnection.
      */
    virtual OpalSDPHTTPConnection * CreateConnection(
      OpalCall & call,
      void * userData,
      unsigned options,
      OpalConnection::StringOptions * stringOptions
    );

    /// Handle incoming HTTP from OpalSDPHTTPResource
    virtual bool OnReceivedHTTP(
      PHTTPServer & server,         ///< HTTP server that received the request
      const PHTTPConnectionInfo & conInfo  ///< HTTP connection information
    );
  //@}

  protected:
    PHTTPSpace m_httpSpace;
};


///////////////////////////////////////////////////////////////////////////////

/**HTTP resource for SDP over HTTP POST command (used for WebRTC).
   This ised added to a PHTTSpace and used by PHTTPServer.
  */
class OpalSDPHTTPResource : public PHTTPResource
{
    PCLASSINFO(OpalSDPHTTPResource, PHTTPResource)
  public:
    OpalSDPHTTPResource(
      OpalSDPHTTPEndPoint & ep,   ///< Opal endpoint
      const PURL & url            ///< Name of the resource in URL space.
    );
    OpalSDPHTTPResource(
      OpalSDPHTTPEndPoint & ep,   ///< Opal endpoint
      const PURL & url,           ///< Name of the resource in URL space.
      const PHTTPAuthority & auth ///< Authorisation for the resource.
    );

    virtual bool OnGET(
      PHTTPServer & server,         ///< HTTP server that received the request
      const PHTTPConnectionInfo & conInfo  ///< HTTP connection information
    );
    virtual bool OnPOST(
      PHTTPServer & server,         ///< HTTP server that received the request
      const PHTTPConnectionInfo & conInfo  ///< HTTP connection information
    );

  protected:
    OpalSDPHTTPEndPoint & m_endpoint;
};


///////////////////////////////////////////////////////////////////////////////

/**Connection for SDP over HTTP POST command (used for WebRTC).
  */
class OpalSDPHTTPConnection : public OpalSDPConnection
{
    PCLASSINFO(OpalSDPHTTPConnection, OpalSDPConnection);
  public:
    /**@name Construction */
    //@{
      /**Create a new connection.
       */
      OpalSDPHTTPConnection(
        OpalCall & call,                         ///<  Owner calll for connection
        OpalSDPHTTPEndPoint & endpoint,          ///<  Owner endpoint for connection
        unsigned options = 0,                    ///<  Connection options
        OpalConnection::StringOptions * stringOptions = NULL     ///< more complex options
      );

      /**Destroy connection.
       */
      ~OpalSDPHTTPConnection();
  //@}

  /**@name Overrides from OpalConnection/OpalSDPConnection */
  //@{
    /**Start an outgoing connection.
       This function will initiate the connection to the remote entity, for
       example in H.323 it sends a SETUP, in SIP it sends an INVITE etc.

       The default behaviour calls OnIncomingConnection() and OpalCall::OnSetUp()
       if it is first conenction in the call.
      */
    virtual PBoolean SetUpConnection();

    /**Indicate to remote endpoint we are connected.

       The default behaviour sets the phase to ConnectedPhase, sets the
       connection start time and then checks if there is any media channels
       opened and if so, moves on to the established phase, calling
       OnEstablished().

       In other words, this method is used to handle incoming calls,
       and is an indication that we have accepted the incoming call.
      */
    virtual PBoolean SetConnected();

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

    /**Get the destination address of an incoming connection.
       This will, for example, collect a phone number from a POTS line, or
       get the fields from the H.225 SETUP pdu in a H.323 connection, or
       INVITE for SIP connection.

       The default behaviour returns "*", which by convention means any
       address the endpoint/connection can get to.
      */
    virtual PString GetDestinationAddress();

    /// Get the media local interface to initialise the RTP session.
    virtual PString GetMediaInterface();

    /// Get the remote media address to initialise the RTP session on making offer.
    virtual OpalTransportAddress GetRemoteMediaAddress();
  //@}

  /**@name Actions */
  //@{
    /// Handle incoming HTTP from OpalSDPHTTPResource
    virtual bool OnReceivedHTTP(
      PHTTPServer & server,         ///< HTTP server that received the request
      const PHTTPConnectionInfo & conInfo  ///< HTTP connection information
    );
  //@}

  protected:
    void InternalSetMediaAddresses(PIndirectChannel & channel);

    PString                 m_destination;
    PString                 m_mediaInterface;
    OpalTransportAddress    m_remoteAddress;
    SDPSessionDescription * m_offerSDP;
    SDPSessionDescription * m_answerSDP;
    PSyncPoint              m_connected;

  friend class OpalSDPHTTPEndPoint;
};

#endif // OPAL_SDP_HTTP

#endif // OPAL_OPAL_SDPEP_H
