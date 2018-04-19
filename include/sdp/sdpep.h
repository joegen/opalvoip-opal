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
   This flag bundles all sessions to use the same transport, e.g. UDP
   socket.

   Defaults to false.
*/
#define OPAL_OPT_AV_BUNDLE "AV-Bundle"

/**Enable multiple sync sources in single session.
   This allows multiple SSRC values to be attached to a single SDP media
   descriptor, m= line. Each SSRC must use the same media format selected for
   the session. If false, and multiple SSRC's are attached to the session,
   then each SSRC will create a separate SDP media descriptor section. Note
   in this latter case, OPAL_OPT_AV_BUNDLE must also be used.

   Defaults to false.
*/
#define OPAL_OPT_MULTI_SSRC "Multi-SSRC"

/**Indicate audio will continue to flow when audio is inactive.
   Usually, the media patch thread for received audio is halted completely
   when the remote indicates no media will flow, via the SDP "recvonly" or
   "inactive" states. If set to false, then the patch thread continues to run,
   and silence, out of the jitter buffer, is tranferred to the sink stream.

   Defaults to false.
  */
#define OPAL_OPT_INACTIVE_AUDIO_FLOW "Inactive-Audio-Flow"


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

  /**@name Overrides from OpalEndPoint */
  //@{
    /** Get available string option names.
      */
    virtual PStringList GetAvailableStringOptions() const;
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
    bool GetOfferSDP(
      SDPSessionDescription & offer,
      bool offerOpenMediaStreamsOnly = false
    );

    /// Generate an offer.
    PString GetOfferSDP(
      bool offerOpenMediaStreamsOnly = false
    );

    /// Handle an offer from remote, provide answer
    bool AnswerOfferSDP(
      const SDPSessionDescription & offer,
      SDPSessionDescription & answer
    );

    /// Handle an offer from remote, provide answer
    PString AnswerOfferSDP(
      const PString & offer
    );

    /// Handle an answer from remote
    bool HandleAnswerSDP(
      const SDPSessionDescription & answer
    );

    /// Handle an answer from remote
    bool HandleAnswerSDP(
      const PString & answer
    );

    /// Decode SDP string, creating new SDP object.
    SDPSessionDescription * CreateSDP(
      const PString & sdp
    );

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
    virtual bool OnSendOfferSDPSession(
      OpalMediaSession * mediaSession,
      SDPMediaDescription * localMedia,
      bool offerOpenMediaStreamOnly,
      RTP_SyncSourceId ssrc
    );

    virtual bool OnSendAnswerSDP(
      const SDPSessionDescription & sdpOffer,
      SDPSessionDescription & sdpAnswer,
      bool transfer = false
    );

    struct BundleMergeInfo;
    virtual SDPMediaDescription * OnSendAnswerSDPSession(
      SDPMediaDescription * incomingMedia,
      unsigned sessionId,
      bool transfer,
      SDPMediaDescription::Direction otherSidesDir,
      BundleMergeInfo & bundleMergeInfo
    );

    virtual bool OnReceivedAnswerSDP(
      const SDPSessionDescription & sdp,
      bool & multipleFormats
    );

    virtual bool OnReceivedAnswerSDPSession(
      const SDPMediaDescription * mediaDescription,
      unsigned sessionId,
      SDPMediaDescription::Direction otherSidesDir,
      bool & multipleFormats,
      BundleMergeInfo & bundleMergeInfo
    );

    virtual void FinaliseRtx(
      const OpalMediaStreamPtr & stream,
      SDPMediaDescription * sdp
    );

    virtual bool SetActiveMediaFormats(
      const OpalMediaFormatList & formats
    );

    virtual OpalMediaSession * SetUpMediaSession(
      const unsigned rtpSessionId,
      const OpalMediaType & mediaType,
      const SDPMediaDescription & mediaDescription,
      OpalTransportAddress & localAddress,
      BundleMergeInfo & bundleMergeInfo
    );

    bool PauseOrCloseMediaStream(OpalMediaStreamPtr & stream, bool changed, bool paused);

    void RetryHoldRemote(bool placeOnHold);
    virtual bool OnHoldStateChanged(bool placeOnHold);
    virtual void OnMediaStreamOpenFailed(bool rx);

    OpalSDPEndPoint & m_endpoint;

    OpalMediaFormatList m_remoteFormatList;
    OpalMediaFormatList m_activeFormatList;

    atomic<bool> m_offerPending;
    time_t       m_sdpSessionId;
    unsigned     m_sdpVersion; // Really a sequence number

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

#endif // OPAL_OPAL_SDPEP_H
