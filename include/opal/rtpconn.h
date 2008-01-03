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
 * $Revision: 19146 $
 * $Author: csoutheren $
 * $Date: 2007-12-21 11:57:11 +1100 (Fri, 21 Dec 2007) $
 */

#ifndef __OPAL_RTPCONN_H
#define __OPAL_RTPCONN_H

#include <opal/buildopts.h>

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#ifdef __GNUC__
#pragma implementation "rtpconn.h"
#endif

#include <opal/connection.h>
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

  /**@name RTP Session Management */
  //@{
    /**Get an RTP session for the specified ID.
       If there is no session of the specified ID, NULL is returned.
      */
    virtual RTP_Session * GetSession(
      unsigned sessionID    ///<  RTP session number
    ) const;

    /**Use an RTP session for the specified ID.
       This will find a session of the specified ID and increment its
       reference count. Multiple OpalRTPStreams use this to indicate their
       usage of the RTP session.

       If this function is used, then the ReleaseSession() function MUST be
       called or the session is never deleted for the lifetime of the Opal
       connection.

       If there is no session of the specified ID one is created.

       The type of RTP session that is created will be compatible with the
       transport. At this time only IP (RTp over UDP) is supported.
      */
    virtual RTP_Session * UseSession(
      unsigned sessionID
    );
    virtual RTP_Session * UseSession(
      const OpalTransport & transport,  ///<  Transport of signalling
      unsigned sessionID,               ///<  RTP session number
      RTP_QOS * rtpqos = NULL           ///<  Quiality of Service information
    );

    /**Release the session.
       If the session ID is not being used any more any clients via the
       UseSession() function, then the session is deleted.
     */
    virtual void ReleaseSession(
      unsigned sessionID,    ///<  RTP session number
      PBoolean clearAll = PFalse  ///<  Clear all sessions
    );

    /**Create and open a new RTP session.
       The type of RTP session that is created will be compatible with the
       transport. At this time only IP (RTp over UDP) is supported.
      */
    virtual RTP_Session * CreateSession(
      const OpalTransport & transport,
      unsigned sessionID,
      RTP_QOS * rtpqos
    );
  //@}

  //@{
    /** Return PTrue if the remote appears to be behind a NAT firewall
    */
    PBoolean RemoteIsNAT() const
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

    /**Attaches the RFC 2833 handler to the media patch
       This method may be called from subclasses, e.g. within
       OnPatchMediaStream()
      */
    virtual void AttachRFC2833HandlerToPatch(PBoolean isSource, OpalMediaPatch & patch);

    virtual PBoolean SendUserInputTone(
      char tone,        ///<  DTMF tone code
      unsigned duration = 0  ///<  Duration of tone in milliseconds
    );

    /**Meda information structure for GetMediaInformation() function.
      */
    struct MediaInformation {
      MediaInformation() { 
        rfc2833  = RTP_DataFrame::IllegalPayloadType; 
#if OPAL_T38FAX
        ciscoNSE = RTP_DataFrame::IllegalPayloadType; 
#endif
      }

      OpalTransportAddress data;           ///<  Data channel address
      OpalTransportAddress control;        ///<  Control channel address
      RTP_DataFrame::PayloadTypes rfc2833; ///<  Payload type for RFC2833
#if OPAL_T38FAX
      RTP_DataFrame::PayloadTypes ciscoNSE; ///<  Payload type for RFC2833
#endif
    };

    /**Get information on the media channel for the connection.
       The default behaviour checked the mediaTransportAddresses dictionary
       for the session ID and returns information based on that. It also uses
       the rfc2833Handler variable for that part of the info.

       It is up to the descendant class to assure that the mediaTransportAddresses
       dictionary is set correctly before OnIncomingCall() is executed.
     */
    virtual PBoolean GetMediaInformation(
      unsigned sessionID,     ///<  Session ID for media channel
      MediaInformation & info ///<  Information on media channel
    ) const;

    /**See if the media can bypass the local host.

       The default behaviour returns PTrue if the session is audio or video.
     */
    virtual PBoolean IsMediaBypassPossible(
      unsigned sessionID                  ///<  Session ID for media channel
    ) const;

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

    /**Overrides from OpalConnection
      */
    virtual void OnPatchMediaStream(PBoolean isSource, OpalMediaPatch & patch);

    virtual bool CanAutoStartMedia(const OpalMediaType & mediaType, bool rx);

    virtual void SetSecurityMode(const PString & v)
    { securityMode = v; }

    virtual PString GetSecurityMode() const 
    { return securityMode; }

  protected:
    PString securityMode;

    OpalRFC2833Proto    * rfc2833Handler;
#if OPAL_T38FAX
    OpalRFC2833Proto    * ciscoNSEHandler;
#endif

    RTP_SessionManager            rtpSessions;

  public:
    /** Class for storing media channel start information
      */
    struct ChannelInfo {
      public:
        ChannelInfo(const OpalMediaType & _mediaType);

        OpalMediaType mediaType;
        bool autoStartReceive;
        bool autoStartTransmit;
        bool assigned;

        unsigned protocolSpecificSessionId;
        unsigned channelId;

        PString channelName;
    };

    class ChannelInfoMap : public std::map<unsigned, ChannelInfo> 
    {
      public:
        ChannelInfoMap();
        void Initialise(OpalRTPConnection & conn, OpalConnection::StringOptions * stringOptions);
        unsigned AddChannel(ChannelInfo & info);

        ChannelInfo * AssignAndLockChannel(const OpalMediaSessionId & id, bool assigned);
        ChannelInfo * AssignAndLockChannel(const OpalMediaType & mediaType, bool assigned);
        bool CanAutoStartMedia(const OpalMediaType & mediaType, bool rx);

        OpalMediaSessionId GetSessionOfType(const OpalMediaType & type) const;
        OpalMediaType ChannelInfoMap::GetTypeOfSession(unsigned sessionId) const;

        void Unlock() { mutex.Signal(); }

        mutable PMutex mutex;

      protected:
        void SetOldOptions(unsigned channelID, const OpalMediaType & mediaType, bool rx, bool tx);
        mutable bool initialised;
    };

    const ChannelInfoMap & GetChannelInfoMap() const { return channelInfoMap; }

  protected:
    ChannelInfoMap channelInfoMap; 
};

#endif // __OPAL_RTPCONN_H
