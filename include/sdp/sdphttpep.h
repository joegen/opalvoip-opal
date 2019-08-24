/*
 * sdphttpep.h
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
 */

#ifndef OPAL_OPAL_SDPHTTPEP_H
#define OPAL_OPAL_SDPHTTPEP_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <opal_config.h>


#if OPAL_SDP_HTTP

#include <sdp/sdpep.h>
#include <opal.h>


/** This query parameter on the HTTP POST indicates the operation to execute.
    If missing then this is treated as a "connect".
  */
#define OPAL_SDP_HTTP_OP_QUERY_PARAM   "operation"

/** This operation will start a new connection. It is expected the body of the
    HTTP POST contains the offer SDP, and has content-type application/sdp.
    There should also be a "destination" query parameter for what to connect
    to within OPAL. This can be another protocol such as sip or internal
    endpoints such as "pcss" */
#define OPAL_SDP_HTTP_OP_CONNECT       "connect"

/** This operation disconnects an existing connection. The "connection-id"
    query parameter is required to identify the connection to disconnect. The
    "connect" operation will return this value in the
    "X-OPAL-Connection-Id" HTTP header. */
#define OPAL_SDP_HTTP_OP_DISCONNECT    "disconnect"

/** This operation returns a short HTML indication of the call status.
    The "connection-id" query parameter is required to identify the
    connection*/
#define OPAL_SDP_HTTP_OP_STATUS        "status"

/// THe connnection identifier query parameter used by some operations
#define OPAL_SDP_HTTP_ID_QUERY_PARAM   "connection-id"

/// The HTTP header that returns the connection identifier
#define OPAL_SDP_HTTP_ID_HEADER        "X-OPAL-Connection-Id"

/// The destination query parameter for the "connect" operation.
#define OPAL_SDP_HTTP_DEST_QUERY_PARAM "destination"


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
      const PCaselessString & prefix = OPAL_PREFIX_SDP ///<  Prefix for URL style address strings
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
      PHTTPRequest & request  ///< HTTP request information
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

    virtual PBoolean OnGETData(
      PHTTPRequest & request                      ///< request state information
    );
    virtual PBoolean OnPOSTData(
      PHTTPRequest & request,        ///< request information
      const PStringToString & data   ///< Variables in the POST data.
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

    /**Get the protocol-specific unique identifier for this connection.
    Default behaviour just returns the connection token.
    */
    virtual PString GetIdentifier() const;

    /// Get the media local interface to initialise the RTP session.
    virtual PString GetMediaInterface();

    /// Get the remote media address to initialise the RTP session on making offer.
    virtual OpalTransportAddress GetRemoteMediaAddress();
  //@}

  /**@name Actions */
  //@{
    /// Handle incoming HTTP from OpalSDPHTTPResource
    virtual bool OnReceivedHTTP(
      PHTTPRequest & request  ///< HTTP request information
    );
  //@}

  protected:
    void InternalSetMediaAddresses(PIndirectChannel & channel);

    PString                 m_mediaInterface;
    OpalTransportAddress    m_remoteAddress;
    SDPSessionDescription * m_offerSDP;
    SDPSessionDescription * m_answerSDP;
    PSyncPoint              m_connected;
    PGloballyUniqueID       m_guid;

  friend class OpalSDPHTTPEndPoint;
};

#endif // OPAL_SDP_HTTP

#endif // OPAL_OPAL_SDPHTTPEP_H
