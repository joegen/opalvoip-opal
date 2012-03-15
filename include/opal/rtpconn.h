/*
 * rtpconn.h
 *
 * Connection abstraction
 *
 * Open Phone Abstraction Library (OPAL)
 *
 * Copyright (C) 2007 Post Increment
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

#ifndef OPAL_OPAL_RTPCONN_H
#define OPAL_OPAL_RTPCONN_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <opal/buildopts.h>

#include <opal/connection.h>
#include <opal/mediatype.h>
#include <opal/mediasession.h>
#include <rtp/rtp_session.h>
#include <ptlib/bitwise_enum.h>


class OpalRTPEndPoint;


/**This is the base class for OpalConnections that use RTP sessions, 
   such as H.323 and SIPconnections to an endpoint.
 */
class OpalRTPConnection : public OpalConnection
{
    PCLASSINFO(OpalRTPConnection, OpalConnection);
  public:
  /**@name Construction */
  //@{
    /**Create a new connection.
     */
    OpalRTPConnection(
      OpalCall & call,                         ///<  Owner calll for connection
      OpalRTPEndPoint & endpoint,              ///<  Owner endpoint for connection
      const PString & token,                   ///<  Token to identify the connection
      unsigned options = 0,                    ///<  Connection options
      OpalConnection::StringOptions * stringOptions = NULL     ///< more complex options
    );  

    /**Destroy connection.
     */
    ~OpalRTPConnection();

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
  //@}


  /**@name Overrides from OpalConnection */
  //@{
    /**Create a new media stream.
       This will create a media stream of an appropriate subclass as required
       by the underlying connection protocol. For instance H.323 would create
       an OpalRTPStream.

       The sessionID parameter may not be needed by a particular media stream
       and may be ignored. In the case of an OpalRTPStream it us used.

       Note that media streams may be created internally to the underlying
       protocol. This function is not the only way a stream can come into
       existance.
     */
    virtual OpalMediaStream * CreateMediaStream(
      const OpalMediaFormat & mediaFormat, ///<  Media format for stream
      unsigned sessionID,                  ///<  Session number for stream
      PBoolean isSource                        ///<  Is a source stream
    );

    /**Get transports for the media session on the connection.
       This is primarily used by the media bypass feature controlled by the
       OpalManager::AllowMediaBypass() function. It allows one side of the
       call to get the transport address of the media on the other side, so it
       can pass it on, bypassing the local host.

       @return true if a transport address is available and may be used to pass
               on to a remote system for direct access.
     */
    virtual bool GetMediaTransportAddresses(
      const OpalMediaType & mediaType,       ///< Media type for session to return information
      OpalTransportAddressArray & transports ///<  Information on media session
    ) const;

    /**Adjust media formats available on a connection.
       This is called by a connection after it has called
       OpalCall::GetMediaFormats() to get all media formats that it can use so
       that an application may remove or reorder the media formats before they
       are used to open media streams.

       This function may also be executed by other connections in the call. If
       this happens then the "otherConnection" parameter will be non-NULL. The
       "local" parameter sense is relative to the "otherConnection" parameter,
       if NULL then it is relative to "this".

       The default behaviour calls the OpalEndPoint function of the same name.
      */
    virtual void AdjustMediaFormats(
      bool local,                             ///<  Media formats a local ones to be presented to remote
      const OpalConnection * otherConnection, ///<  Other connection we are adjusting media for
      OpalMediaFormatList & mediaFormats      ///<  Media formats to use
    ) const;

    /**Call back when patching a media stream.
       This function is called when a connection has created a new media
       patch between two streams. This is usually called twice per media patch,
       once for the source stream and once for the sink stream.

       Note this is not called within the context of the patch thread and is
       called before that thread has started.
      */
    virtual void OnPatchMediaStream(
      PBoolean isSource,        ///< Is source/sink call
      OpalMediaPatch & patch    ///<  New patch
    );

    /** Callback for media commands.
        Calls the SendIntraFrameRequest on the rtp session

       @returns true if command is handled.
      */
    virtual bool OnMediaCommand(
      OpalMediaStream & stream,         ///< Stream command executed on
      const OpalMediaCommand & command  ///< Media command being executed
    );

    virtual PBoolean SendUserInputTone(
      char tone,        ///<  DTMF tone code
      unsigned duration = 0  ///<  Duration of tone in milliseconds
    );
  //@}


  /**@name RTP Session Management */
  //@{
    /**Get next available session ID for the media type.
      */
    virtual unsigned GetNextSessionID(
      const OpalMediaType & mediaType,   ///< Media type of stream being opened
      bool isSource                      ///< Stream is a source/sink
    );

    P_DECLARE_BITWISE_ENUM(CreateMediaSessionsSecurity, 2, (e_NoMediaSessions, e_ClearMediaSession, e_SecureMediaSession));

    /**Create all media sessions for available media types.
       Note that the sessions are not opened, just created.

       Sessions using media security will alsoe be created if \p secure is
       true. This indicates the media key exchange can be done securely,
       typically it means signaling channel is also secure.

       @returns a list of the media types for which sessions where created.
      */
    vector<bool> CreateAllMediaSessions(
      CreateMediaSessionsSecurity security  ///< Wse secure media, or not
    );

    /**Get an RTP session for the specified ID.
       If there is no session of the specified ID, NULL is returned.
      */
    virtual OpalMediaSession * GetMediaSession(
      unsigned sessionID    ///<  RTP session number
    ) const;

    /**Find an RTP session for the specified local port.
       If there is no session, NULL is returned.
      */
    virtual OpalMediaSession * FindSessionByLocalPort(
      WORD port    ///< Local port number
    ) const;

    /**Use an RTP session for the specified ID.
       This will find a session of the specified ID and uses it if available.

       If there is no session of the specified ID one is created.

       The type of RTP session that is created will be compatible with the
       transport. At this time only IP (RTP over UDP) is supported.
      */
    virtual OpalMediaSession * UseMediaSession(
      unsigned sessionId,               ///< Unique (in connection) session ID for session
      const OpalMediaType & mediaType,  ///< Media type for session
      const PString & sessionType = PString::Empty() ///< Type of session to create
    );

    /**Release the session.
     */
    virtual void ReleaseMediaSession(
      unsigned sessionID    ///<  RTP session number
    );

    /**Change the sessionID for an existing session.
       This will adjust the RTP session and media streams.

       Return false if no such session exists.
      */
    virtual bool ChangeSessionID(
      unsigned fromSessionID,   ///< Session ID to search for
      unsigned toSessionID      ///< Session ID to change to
    );

    /**Replace existing session with new one, exchanging transports.
      */
    virtual void ReplaceMediaSession(
      unsigned sessionId,             ///< Session ID to replace
      OpalMediaSession * mediaSession ///< New session
    );

    /**Set QoS on session.
      */
    virtual bool SetSessionQoS(
      OpalRTPSession * session
    );
  //@}

  /**@name NAT Management */
  //@{
    /** Return true if the remote appears to be behind a NAT firewall
    */
    virtual PBoolean RemoteIsNAT() const
    { return remoteIsNAT; }

    /**Determine if the RTP session needs to accommodate a NAT router.
       For endpoints that do not use STUN or something similar to set up all the
       correct protocol embeddded addresses correctly when a NAT router is between
       the endpoints, it is possible to still accommodate the call, with some
       restrictions. This function determines if the RTP can proceed with special
       NAT allowances.

       The special allowance is that the RTP code will ignore whatever the remote
       indicates in the protocol for the address to send RTP data and wait for
       the first packet to arrive from the remote and will then proceed to send
       all RTP data back to that address AND port.

       The default behaviour checks the values of the physical link
       (localAddr/peerAddr) against the signaling address the remote indicated in
       the protocol, eg H.323 SETUP sourceCallSignalAddress or SIP "To" or
       "Contact" fields, and makes a guess that the remote is behind a NAT router.
     */
    virtual PBoolean IsRTPNATEnabled(
      const PIPSocket::Address & localAddr,   ///< Local physical address of connection
      const PIPSocket::Address & peerAddr,    ///< Remote physical address of connection
      const PIPSocket::Address & signalAddr,  ///< Remotes signaling address as indicated by protocol of connection
      PBoolean incoming                       ///< Incoming/outgoing connection
    );
  //@}

    class SessionMap : public map<unsigned, OpalMediaSession *>
    {
      public:
        SessionMap();
        ~SessionMap();
        void Assign(SessionMap & other, bool move = true);
      protected:
        bool m_deleteSessions;
      private:
        SessionMap(const SessionMap & obj) : map<unsigned, OpalMediaSession *>(obj) { }
        void operator=(const SessionMap &) { }
    };

  protected:
    PDECLARE_NOTIFIER(OpalRFC2833Info, OpalRTPConnection, OnUserInputInlineRFC2833);

    void CheckForMediaBypass(OpalMediaSession & session);

    SessionMap m_sessions;

    OpalRFC2833Proto * rfc2833Handler;
#if OPAL_T38_CAPABILITY
    OpalRFC2833Proto * ciscoNSEHandler;
#endif

    PBoolean remoteIsNAT;
    PBoolean useRTPAggregation;

#if OPAL_VIDEO
    PSimpleTimer m_rtcpIntraFrameRequestTimer;
#endif
};


#endif // OPAL_OPAL_RTPCONN_H
