/*
 * sipcon.h
 *
 * Session Initiation Protocol connection.
 *
 * Open Phone Abstraction Library (OPAL)
 * Formally known as the Open H323 project.
 *
 * Copyright (c) 2001 Equivalence Pty. Ltd.
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
 * The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * Contributor(s): ______________________________________.
 */

#ifndef OPAL_SIP_SIPCON_H
#define OPAL_SIP_SIPCON_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <opal_config.h>

#if OPAL_SIP

#include <rtp/rtpconn.h>
#include <sip/sippdu.h>
#include <sip/handlers.h>
#include <sdp/sdpep.h>


class OpalSIPIMContext;


/**OpalConnection::StringOption key to a boolean indicating that call
   deflection/forwarding should use REFER rather tha  302 response.
   Default false.
  */
#define OPAL_OPT_FORWARD_REFER   "Forward-Refer"

/**OpalConnection::StringOption key to a boolean indicating the the state
   of the "Refer-Sub" header in the REFER request. Default true.
  */
#define OPAL_OPT_REFER_SUB       "Refer-Sub"

/**OpalConnection::StringOption key to a boolean indicating whether to not 
use RFC4488 ("Refer-Sub" header) at all, in the REFER request. Default false.
*/
#define OPAL_OPT_NO_REFER_SUB    "No-Refer-Sub"

/**OpalConnection::StringOption key to an integer indicating the the mode
   for the reliable provisional response system. See PRACKMode for more
   information. Default is from SIPEndPoint::GetDefaultPRACKMode() which
   in turn defaults to e_prackSupported.
  */
#define OPAL_OPT_PRACK_MODE      "PRACK-Mode"

/**OpalConnection::StringOption key to a boolean indicating that we should
   make initial SDP offer. Default true.
  */
#define OPAL_OPT_INITIAL_OFFER "Initial-Offer"

/**Allow INVITE with Replaces to operate on "early" dialog.
   While RFC 3891 explicitly forbids and incoming INVITE with Replaces to
   replace a call which was incoming to the UA. It is allowed for a dialog
   originated by the UA receiving the INVITE with Replace, but not for ones
   where it is "ringing". Unfortunately, some systems (*cough Cisco) do it
   anyway. This option allows the replacement and dows not send the 481
   response as required by the specification.

   Defaults to false.
  */
#define OPAL_OPT_ALLOW_EARLY_REPLACE "Allow-Early-Replace"

/**OpalConnection::StringOption key to a string representing the precise SDP
   to be included in the INVITE offer. No media streams are opened or any
   checks whstsoever made on the string. It is simply included as the body of
   the INVITE.
   
   Note this options presence also prevents handling of the response SDP
   
   Defaults to empty string which generates SDP from available
   media formats and media streams.
  */
#define OPAL_OPT_EXTERNAL_SDP "External-SDP"

/**Enable SRTP when not using secure signalling (sips)
   Normally SRTP and SDES only applies to secure signalling, sips or
   transport=tls, however this flag will allow it on unsecure connections
   which means the key exchange can be observed.

   Defaults to false.
*/
#define OPAL_OPT_UNSECURE_SRTP "Unsecure-SRTP"

/**Offer/accept DTLS.
   Unlike SDES, this is more "modern" and is independent of requiring a secure
   signalling channel. However, it is not as widely supported.

   Defaults to false.
*/
#define OPAL_OPT_ENABLE_DTLS "Enable-DTLS"


#define SIP_HEADER_PREFIX      "SIP-Header:"
#define SIP_HEADER_REPLACES    SIP_HEADER_PREFIX"Replaces"
#define SIP_HEADER_REFERRED_BY SIP_HEADER_PREFIX"Referred-By"
#define SIP_HEADER_CONTACT     SIP_HEADER_PREFIX"Contact"

#define OPAL_SIP_REFERRED_CONNECTION "Referred-Connection"


/////////////////////////////////////////////////////////////////////////

/**Session Initiation Protocol connection.
 */

class SIPConnection : public OpalSDPConnection, public SIPTransactionOwner
{
    PCLASSINFO(SIPConnection, OpalSDPConnection);
  public:

  /**@name Construction */
  //@{
    struct Init {
      Init(OpalCall & call, SIPEndPoint & endpoint)
        : m_call(call)
        , m_endpoint(endpoint)
        , m_userData(NULL)
        , m_invite(NULL)
        , m_options(0)
        , m_stringOptions(NULL)
        { }

      OpalCall    & m_call;      ///< Owner call for connection
      SIPEndPoint & m_endpoint;  ///< SIP endpoint that controls the connection
      PString       m_token;     ///< Token to identify the connection
      SIPURL        m_address;   ///< Destination address for outgoing call
      void        * m_userData;  ///< User data
      SIP_PDU     * m_invite;    ///< Invite packet
      unsigned m_options;        ///< Connection options
      OpalConnection::StringOptions * m_stringOptions;  ///<  complex string options
    };

    /**Create a new connection.
     */
    SIPConnection(
      const Init & init         ///< Initialisation parameters
    );

    /**Destroy connection.
     */
    ~SIPConnection();
  //@}

  /**@name Overrides from OpalConnection */
  //@{
    /**Get this connections protocol prefix for URLs.
      */
    virtual PString GetPrefixName() const;

    /**Get the protocol-specific unique identifier for this connection.
     */
    virtual PString GetIdentifier() const;

    /// Call back for connection to act on changed string options
    virtual void OnApplyStringOptions();

    /**Start an outgoing connection.
       This function will initiate the connection to the remote entity, for
       example in H.323 it sends a SETUP, in SIP it sends an INVITE etc.

       The default behaviour is .
      */
    virtual PBoolean SetUpConnection();

    /** Get the remote transport address
      */
    virtual OpalTransportAddress GetRemoteAddress() const { return m_remoteAddress; }

    /** Get the remote identity.
        Under some circumstances the "identity" of the remote party, may be
        different from the name, number or URL for that user. For example, this
        would be the P-Asserted-Identity field in SIP.
      */
    virtual PString GetRemoteIdentity() const;

    /**Get the destination address of an incoming connection.
       This will, for example, collect a phone number from a POTS line, or
       get the fields from the H.225 SETUP pdu in a H.323 connection.

       The default behaviour for sip returns the request URI in the INVITE.
      */
    virtual PString GetDestinationAddress();

    /**Get the fulll URL being indicated by the remote for incoming calls. This may
       not have any relation to the local name of the endpoint.

       The default behaviour returns GetDestinationAddress() normalised to a URL.
       The remote may provide a full URL, if it does not then the prefix for the
       endpoint is prepended to the destination address.
      */
    virtual PString GetCalledPartyURL();

    /**Get any extra call information.
      */
    virtual PMultiPartList GetExtraCallInfo() const { return m_multiPartMIME; }

    /**Get the local name/alias.
      */
    virtual PString GetLocalPartyURL() const;

    /**Get alerting type information of an incoming call.
       The type of "distinctive ringing" for the call. The string is protocol
       dependent, so the caller would need to be aware of the type of call
       being made. Some protocols may ignore the field completely.

       For SIP this corresponds to the string contained in the "Alert-Info"
       header field of the INVITE. This is typically a URI for the ring file.

       For H.323 this must be a string representation of an integer from 0 to 7
       which will be contained in the Q.931 SIGNAL (0x34) Information Element.

       Default behaviour returns an empty string.
      */
    virtual PString GetAlertingType() const;

    /**Set alerting type information for outgoing call.
       The type of "distinctive ringing" for the call. The string is protocol
       dependent, so the caller would need to be aware of the type of call
       being made. Some protocols may ignore the field completely.

       For SIP this corresponds to the string contained in the "Alert-Info"
       header field of the INVITE. This is typically a URI for the ring file.

       For H.323 this must be a string representation of an integer from 0 to 7
       which will be contained in the Q.931 SIGNAL (0x34) Information Element.

       Default behaviour returns false.
      */
    virtual bool SetAlertingType(const PString & info);

    /**Get call information of an incoming call.
       This is protocol dependent information provided about the call. The
       details are outside the scope of this help.

       For SIP this corresponds to the string contained in the "Call-Info"
       header field of the INVITE.
      */
    virtual PString GetCallInfo() const;

    /**Get supported features for an incoming call.
       A '\n' separated list of protocol dependent names about the supported
       features of the remote endpoint. The details are outside the scope of
       this help.

       For SIP this corresponds to the contents of the "Supported" header field
       of the INVITE.
      */
    virtual PString GetSupportedFeatures() const;

    /**Initiate the transfer of an existing call (connection) to a new remote 
       party.

       A REFER command is sent to the remote endpoint to cause it to move the
       call it has with this endpoint to a new address. This call will, in the
       end, be cleared. The OnTransferNotify() function can be used to monitor
       the progress of the transfer in case it fails.

       If remoteParty is a valid call token, then this is short hand for
       the REFER to the remote endpoint of this call to do an INVITE with
       Replaces header to the remote party of the supplied tokens call. This
       is used for consultation transfer where A calls B, B holds A, B calls C,
       B transfers A to C. The last step is a REFER to A with call details of
       C that are extracted from the B to C call leg. This short cut is
       possible because A nd C may be other endpoints but both B's are in this
       instance of OPAL.
       
       In the end, both calls are cleared. The OnTransferNotify() function can
       be used to monitor the progress of the transfer in case it fails.
     */
    virtual bool TransferConnection(
      const PString & remoteParty   ///<  Remote party to transfer the existing call to
    );

    /**Indicate to remote endpoint an alert is in progress.
       If this is an incoming connection and the AnswerCallResponse is in a
       AnswerCallDeferred or AnswerCallPending state, then this function is
       used to indicate to that endpoint that an alert is in progress. This is
       usually due to another connection which is in the call (the B party)
       has received an OnAlerting() indicating that its remote endpoint is
       "ringing".

       The default behaviour does nothing.
      */
    virtual PBoolean SetAlerting(
      const PString & calleeName,   ///<  Name of endpoint being alerted.
      PBoolean withMedia            ///<  Flag to alert with/without media
    );

    /**Indicate to remote endpoint we are connected.

       The default behaviour does nothing.
      */
    virtual PBoolean SetConnected();

    /**Get the data formats this endpoint is capable of operating in.
      */
    virtual OpalMediaFormatList GetMediaFormats() const;

    /**Indicate connection requires symmetric media.
       Default behaviour returns false.
      */
    virtual bool RequireSymmetricMediaStreams() const;

#if OPAL_T38_CAPABILITY
    /**Switch to/from T.38 fax mode.
      */
    virtual bool SwitchFaxMediaStreams(
      bool toT38  ///< T.38 or return to audio mode
    );
#endif

    /**Indicate security modes available in media negotiation.
    */
    virtual OpalMediaCryptoSuite::KeyExchangeModes GetMediaCryptoKeyExchangeModes() const;

    /**Create a new media stream.
     */
    virtual OpalMediaStream * CreateMediaStream(
      const OpalMediaFormat & mediaFormat, ///<  Media format for stream
      unsigned sessionID,                  ///<  Session number for stream
      PBoolean isSource                        ///<  Is a source stream
    );

    /**Open source or sink media stream for session.
      */
    virtual OpalMediaStreamPtr OpenMediaStream(
      const OpalMediaFormat & mediaFormat, ///<  Media format to open
      unsigned sessionID,                  ///<  Session to start stream on
      bool isSource                        ///< Stream is a source/sink
    );

    /**Call back for closed a media stream.

       The default behaviour calls the OpalEndPoint function of the same name.
      */
    virtual void OnClosedMediaStream(
      const OpalMediaStream & stream     ///<  Media stream being closed
    );

    /**Get transports for the media session on the connection.
       This is primarily used by the media bypass feature controlled by the
       OpalManager::GetMediaTransferMode() function. It allows one side of the
       call to get the transport address of the media on the other side, so it
       can pass it on, bypassing the local host.

       @return true if a transport address is available and may be used to pass
               on to a remote system for direct access.
     */
    virtual bool GetMediaTransportAddresses(
      OpalConnection & otherConnection,      ///< Other half of call needing media transport addresses
      unsigned sessionId,                    ///< Session identifier
      const OpalMediaType & mediaType,       ///< Media type for session to return information
      OpalTransportAddressArray & transports ///<  Information on media session
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

    /**Pause media streams for connection.
      */
    virtual void OnPauseMediaStream(
      OpalMediaStream & strm,     ///< Media stream paused/un-paused
      bool paused                 ///< Flag for pausing/un-pausing
    );

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

    /**Forward incoming connection to the specified address.
       This would typically be called from within the OnIncomingConnection()
       function when an application wishes to redirect an unwanted incoming
       call.

       The return value is true if the call is to be forwarded, false 
       otherwise. Note that if the call is forwarded, the current connection
       is cleared with the ended call code set to EndedByCallForwarded.
      */
    virtual PBoolean ForwardCall(
      const PString & forwardParty   ///<  Party to forward call to.
    );

    /**Get the real user input indication transmission mode.
       This will return the user input mode that will actually be used for
       transmissions. It will be the value of GetSendUserInputMode() provided
       the remote endpoint is capable of that mode.
      */
    virtual SendUserInputModes GetRealSendUserInputMode() const;

    /**Send a user input indication to the remote endpoint.
       This is for sending arbitrary strings as user indications.

       The default behaviour is to call SendUserInputTone() for each character
       in the string.
      */
    virtual PBoolean SendUserInputString(
      const PString & value                   ///<  String value of indication
    );

    /**Send a user input indication to the remote endpoint.
       This sends DTMF emulation user input. If something more sophisticated
       than the simple tones that can be sent using the SendUserInput()
       function.

       A duration of zero indicates that no duration is to be indicated.
       A non-zero logical channel indicates that the tone is to be syncronised
       with the logical channel at the rtpTimestamp value specified.

       The tone parameter must be one of "0123456789#*ABCD!" where '!'
       indicates a hook flash. If tone is a ' ' character then a
       signalUpdate PDU is sent that updates the last tone indication
       sent. See the H.245 specifcation for more details on this.

       The default behaviour sends the tone using RFC2833.
      */
    PBoolean SendUserInputTone(char tone, unsigned duration);
  //@}

  /**@name Protocol handling functions */
  //@{
    /**Handle the fail of a transaction we initiated.
      */
    virtual void OnTransactionFailed(
      SIPTransaction & transaction
    );

    /**Handle an incoming SIP PDU that has been full decoded
      */
    virtual void OnReceivedPDU(SIP_PDU & pdu);

    /**Handle an incoming INVITE request
      */
    virtual void OnReceivedINVITE(SIP_PDU & pdu);

    /**Handle an incoming Re-INVITE request
      */
    virtual void OnReceivedReINVITE(SIP_PDU & pdu);

    /**Handle an incoming ACK PDU
      */
    virtual void OnReceivedACK(SIP_PDU & pdu);
  
    /**Handle an incoming OPTIONS PDU
      */
    virtual void OnReceivedOPTIONS(SIP_PDU & pdu);

    /**Handle an incoming NOTIFY PDU
      */
    virtual void OnReceivedNOTIFY(SIP_PDU & pdu);

    /**Callback function on receipt of an allowed NOTIFY message.
       Allowed events are determined by the m_allowedEvents member variable.
      */
    virtual void OnAllowedEventNotify(
      const PString & eventName  ///< Name of event
    );

    /**Handle an incoming REFER PDU
      */
    virtual void OnReceivedREFER(SIP_PDU & pdu);
  
    /**Handle an incoming INFO PDU
      */
    virtual void OnReceivedINFO(SIP_PDU & pdu);

    /**Handle an incoming PING PDU
      */
    virtual void OnReceivedPING(SIP_PDU & pdu);

    /**Handle an incoming PRACK PDU
      */
    virtual void OnReceivedPRACK(SIP_PDU & pdu);

    /**Handle an incoming BYE PDU
      */
    virtual void OnReceivedBYE(SIP_PDU & pdu);
  
    /**Handle an incoming CANCEL PDU
      */
    virtual void OnReceivedCANCEL(SIP_PDU & pdu);
  
    /**Handle an incoming response PDU to our INVITE.
       Note this is called before th ACK is sent and thus should do as little as possible.
       All the hard work (SDP processing etc) should be in the usual OnReceivedResponse().
      */
    virtual bool OnReceivedResponseToINVITE(
      SIPTransaction & transaction,
      SIP_PDU & response
    );

    /**Handle an incoming response PDU.
      */
    virtual void OnReceivedResponse(
      SIPTransaction & transaction,
      SIP_PDU & response
    );

    /**Handle an incoming Trying response PDU
      */
    virtual void OnReceivedTrying(
      SIPTransaction & transaction,
      SIP_PDU & response
    );
  
    /**Handle an incoming Ringing response PDU
      */
    virtual void OnReceivedRinging(SIPTransaction & transaction, SIP_PDU & pdu);
  
    /**Handle an incoming Session Progress response PDU
      */
    virtual void OnReceivedSessionProgress(SIPTransaction & transaction, SIP_PDU & pdu);
  
    /**Handle an incoming Proxy Authentication Required response PDU
       Returns: true if handled, if false is returned connection is released.
      */
    virtual PBoolean OnReceivedAuthenticationRequired(
      SIPTransaction & transaction,
      SIP_PDU & response
    );
  
    /**Handle an incoming redirect response PDU
      */
    virtual void OnReceivedRedirection(SIP_PDU & pdu);

    /**Handle an incoming OK response PDU.
       This actually gets any PDU of the class 2xx not just 200.
      */
    virtual void OnReceivedOK(
      SIPTransaction & transaction,
      SIP_PDU & response
    );
  
    /**Handle a sending INVITE request
      */
    virtual void OnCreatingINVITE(SIPInvite & pdu);

    enum TypeOfINVITE {
      IsNewINVITE,
      IsDuplicateINVITE,
      IsReINVITE,
      IsLoopedINVITE
    };

    /// Indicate if this is a duplicate or multi-path INVITE
    TypeOfINVITE CheckINVITE(
      const SIP_PDU & pdu
    ) const;

    /**Send an OPTIONS command within this calls dialog.
       Note if \p reply is non-NULL, this function will block until the
       transaction completes. Care must be executed in this case that
       no deadlocks occur.
      */
    bool SendOPTIONS(
      const SIPOptions::Params & params,  ///< Parameters for OPTIONS command
      SIP_PDU * reply = NULL              ///< Reply to message
    );

    /**Send an INFO command within this calls dialog.
       Note if \p reply is non-NULL, this function will block until the
       transaction completes. Care must be executed in this case that
       no deadlocks occur.
      */
    bool SendINFO(
      const SIPInfo::Params & params,  ///< Parameters for OPTIONS command
      SIP_PDU * reply = NULL              ///< Reply to message
    );
  //@}

    OpalTransportAddress GetDefaultSDPConnectAddress(WORD port = 0) const;

    SIPEndPoint & GetEndPoint() const { return m_sipEndpoint; }
    SIPAuthentication * GetAuthenticator() const { return m_authentication; }

    /// Mode for reliable provisional responses.
    enum PRACKMode {
      e_prackDisabled,  /**< Do not use PRACK if remote asks for 100rel in Supported
                             field, refuse call with 420 Bad Extension if 100rel is
                             in Require header field. */
                             
      e_prackSupported, /** Add 100rel to Supported header in outgoing INVITE. For
                            incoming INVITE enable PRACK is either Supported or
                            Require headers include 100rel. */
      e_prackRequired   /** Add 100rel to Require header in outgoing INVITE. For
                            incoming INVITE enable PRACK is either Supported or
                            Require headers include 100rel, fail the call with
                            a 421 Extension Required if missing. */
    };
    /**Get active PRACK mode. See PRACKMode enum for details.
      */
    PRACKMode GetPRACKMode() const { return m_prackMode; }

    /** Return a bit mask of the allowed local SIP methods.
      */
    virtual unsigned GetAllowedMethods() const;

    /** REturn true if remote allows the method.
     */
    bool DoesRemoteAllowMethod(SIP_PDU::Methods method) const { return (m_allowedMethods&(1<<method)) != 0; }

#if OPAL_VIDEO
    /**Call when SIP INFO of type application/media_control+xml is received.

       Return false if default reponse of Failure_UnsupportedMediaType is to be returned

      */
    virtual PBoolean OnMediaControlXML(SIP_PDU & pdu);
#endif

    /** Callback for media commands.
        Calls the SendIntraFrameRequest on the rtp session

       @returns true if command is handled.
      */
    virtual bool OnMediaCommand(
      OpalMediaStream & stream,         ///< Stream command executed on
      const OpalMediaCommand & command  ///< Media command being executed
    );

    // Overrides from SIPTransactionOwner
    virtual PString GetAuthID() const;


    virtual void OnStartTransaction(SIPTransaction & transaction);
    virtual void OnReceivedMESSAGE(SIP_PDU & pdu);
    virtual void OnReceivedSUBSCRIBE(SIP_PDU & pdu);

    virtual PString GetMediaInterface();
    virtual OpalTransportAddress GetRemoteMediaAddress();


  protected:
    virtual bool GarbageCollection();
    void OnSessionTimeout();
    void OnInviteResponseRetry();
    void OnInviteResponseTimeout();
    void OnInviteCollision();
    void OnDelayedRefer();
    virtual bool OnHoldStateChanged(bool placeOnHold);
    virtual void OnMediaStreamOpenFailed(bool rx);

    void OnReceivedAlertingResponse(
      SIPTransaction & transaction,
      SIP_PDU & response
    );
    bool InternalSetConnected(bool transfer);
    virtual bool OnSendAnswer(
      SIP_PDU::StatusCodes response,
      bool transfer
    );
    virtual bool OnReceivedAnswer(
      SIP_PDU & response,
      SIPTransaction * transaction
    );

    bool SendReINVITE(PTRACE_PARAM(const char * msg, ) int operation = 0);
    bool StartPendingReINVITE();

    friend class SIPInvite;
    PDECLARE_WriteConnectCallback(SIPConnection, WriteINVITE);

    virtual bool SendDelayedACK(bool force);
    void OnDelayedAckTimeout();

    virtual PBoolean SendInviteResponse(
      SIP_PDU::StatusCodes code,
      const SDPSessionDescription * sdp = NULL
    );
    virtual void AdjustInviteResponse(
      SIP_PDU & response
    );

    void UpdateRemoteAddresses();

    void NotifyDialogState(
      SIPDialogNotification::States state,
      SIPDialogNotification::Events eventType = SIPDialogNotification::NoEvent,
      unsigned eventCode = 0
    );

    virtual bool InviteConferenceParticipant(const PString & conf, const PString & dest);
    PSafePtr<SIPConnection> GetB2BUA();


    typedef SIPPoolTimer<SIPConnection> PoolTimer;

    // Member variables
    unsigned              m_allowedMethods;
    PStringSet            m_allowedEvents;

    PString               m_forwardParty;
    OpalTransportAddress  m_remoteAddress;
    SIPURL                m_assertedIdentity;
    SIPURL                m_remoteIdentity;
    SIPURL                m_contactAddress;
    SIPURL                m_ciscoRemotePartyID;
    PMultiPartList        m_multiPartMIME;

    SIP_PDU             * m_lastReceivedINVITE;
    SIP_PDU             * m_delayedAckInviteResponse;
    PoolTimer             m_delayedAckTimer;
    PTimeInterval         m_delayedAckTimeout1;
    PTimeInterval         m_delayedAckTimeout2;
    SIP_PDU             * m_delayedAckPDU;
    bool                  m_needReINVITE;
    bool                  m_handlingINVITE;
    bool                  m_resolveMultipleFormatReINVITE;
    bool                  m_symmetricOpenStream;
    OpalGloballyUniqueID  m_dialogNotifyId;
    int                   m_appearanceCode;
    PString               m_alertInfo;
#if OPAL_VIDEO
    bool                  m_canDoVideoFastUpdateINFO;
#endif
    PoolTimer             m_sessionTimer;
    PRACKMode             m_prackMode;
    bool                  m_prackEnabled;
    unsigned              m_prackSequenceNumber;
    std::queue<SIP_PDU>   m_responsePackets;
    PoolTimer             m_responseFailTimer;
    PoolTimer             m_responseRetryTimer;
    unsigned              m_responseRetryCount;
    PoolTimer             m_inviteCollisionTimer;
    PoolTimer             m_delayedReferTimer;
    SIPURL                m_delayedReferTo;
    SIPURL                m_sentReferTo;

    PSafeArray<SIPTransaction> m_forkedInvitations; // Not for re-INVITE
    PSafeArray<SIPTransaction> m_pendingInvitations; // For re-INVITE

    enum {
      ReleaseWithBYE,
      ReleaseWithCANCEL,
      ReleaseWithResponse,
      ReleaseWithNothing,
    } m_releaseMethod;

    enum {
      eNoRemoteRefer,
      eReferStarted,
      eReferNotifyConfirmed
    } m_referOfRemoteState;

    int SetRemoteMediaFormats(SIP_PDU & pdu);

    std::map<std::string, SIP_PDU *> m_responses;

#if OPAL_HAS_SIPIM
    PSafePtr<OpalSIPIMContext> m_imContext;
#endif

    enum {
      UserInputMethodUnknown,
      ReceivedRFC2833,
      ReceivedINFO
    } m_receivedUserInputMethod;
    void OnUserInputInlineRFC2833(OpalRFC2833Info & info, INT type);


  private:
    P_REMOVE_VIRTUAL_VOID(OnCreatingINVITE(SIP_PDU&));
    P_REMOVE_VIRTUAL_VOID(OnReceivedTrying(SIP_PDU &));
    P_REMOVE_VIRTUAL_VOID(OnMessageReceived(const SIPURL & /*from*/, const SIP_PDU & /*pdu*/));
    P_REMOVE_VIRTUAL_VOID(OnMessageReceived(const SIP_PDU & /*pdu*/));

  friend class SIPTransaction;
  friend class SIP_RTP_Session;
};


#endif // OPAL_SIP

#endif // OPAL_SIP_SIPCON_H


// End of File ///////////////////////////////////////////////////////////////
