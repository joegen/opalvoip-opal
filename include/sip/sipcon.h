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
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#ifndef __OPAL_SIPCON_H
#define __OPAL_SIPCON_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <opal/buildopts.h>
#include <opal/rtpconn.h>
#include <sip/sippdu.h>
#if OPAL_VIDEO
#include <opal/pcss.h>                  // for OpalPCSSConnection
#include <codec/vidcodec.h>             // for OpalVideoUpdatePicture command
#endif

class OpalCall;
class SIPEndPoint;


/////////////////////////////////////////////////////////////////////////

/**Session Initiation Protocol connection.
 */
class SIPConnection : public OpalRTPConnection
{
  PCLASSINFO(SIPConnection, OpalRTPConnection);
  public:

  /**@name Construction */
  //@{
    /**Create a new connection.
     */
    SIPConnection(
      OpalCall & call,                          ///<  Owner call for connection
      SIPEndPoint & endpoint,                   ///<  Owner endpoint for connection
      const PString & token,                    ///<  token to identify the connection
      const SIPURL & address,                   ///<  Destination address for outgoing call
      OpalTransport * transport,                ///<  Transport INVITE came in on
      unsigned int options = 0,                 ///<  Connection options
      OpalConnection::StringOptions * stringOptions = NULL  ///<  complex string options
    );

    /**Destroy connection.
     */
    ~SIPConnection();
  //@}

  /**@name Overrides from OpalConnection */
  //@{
    /**Start an outgoing connection.
       This function will initiate the connection to the remote entity, for
       example in H.323 it sends a SETUP, in SIP it sends an INVITE etc.

       The default behaviour is .
      */
    virtual PBoolean SetUpConnection();

    /**Get the destination address of an incoming connection.
       This will, for example, collect a phone number from a POTS line, or
       get the fields from the H.225 SETUP pdu in a H.323 connection.

       The default behaviour returns "*", which by convention means any
       address the endpoint/connection can get to.
      */
    virtual PString GetDestinationAddress();

    /**Initiate the transfer of an existing call (connection) to a new remote 
       party.
     */
    virtual void TransferConnection(
      const PString & remoteParty,   ///<  Remote party to transfer the existing call to
      const PString & callIdentity = PString::Empty()
                                    ///<  Call Identity of secondary call if present
    );

    /**Put the current connection on hold, suspending all media streams.
     */
    virtual void HoldConnection();

    /**Retrieve the current connection from hold, activating all media 
     * streams.
     */
    virtual void RetrieveConnection();

    /**Return PTrue if the current connection is on hold.
     */
    virtual PBoolean IsConnectionOnHold();

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
      PBoolean withMedia
    );

    /**Indicate to remote endpoint we are connected.

       The default behaviour does nothing.
      */
    virtual PBoolean SetConnected();

    /**Get the data formats this endpoint is capable of operating in.
      */
    virtual OpalMediaFormatList GetMediaFormats() const;
    
    /**Open source or sink media stream for session.
      */
    virtual OpalMediaStreamPtr OpenMediaStream(
      const OpalMediaFormat & mediaFormat, ///<  Media format to open
      unsigned sessionID,                  ///<  Session to start stream on
      bool isSource                        ///< Stream is a source/sink
    );

    /**Request close of a specific media stream.
       Note that this is usually asymchronous, the OnClosedMediaStream() function is
       called when the stream is really closed.
      */
    virtual bool CloseMediaStream(
      OpalMediaStream & stream  ///< Stream to close
    );

    /**Indicate the result of answering an incoming call.
       This should only be called if the OnAnswerCall() callback function has
       returned a AnswerCallPending or AnswerCallDeferred response.

       Note sending further AnswerCallPending responses via this function will
       have the result of an 180 PDU being sent to the remote endpoint.
       In this way multiple Alerting PDUs may be sent.

       Sending a AnswerCallDeferred response would have no effect.
      */
    void AnsweringCall(
      AnswerCallResponse response ///<  Answer response to incoming call
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

    /**Handle an incoming REFER PDU
      */
    virtual void OnReceivedREFER(SIP_PDU & pdu);
  
    /**Handle an incoming INFO PDU
      */
    virtual void OnReceivedINFO(SIP_PDU & pdu);

    /**Handle an incoming PING PDU
      */
    virtual void OnReceivedPING(SIP_PDU & pdu);

    /**Handle an incoming BYE PDU
      */
    virtual void OnReceivedBYE(SIP_PDU & pdu);
  
    /**Handle an incoming CANCEL PDU
      */
    virtual void OnReceivedCANCEL(SIP_PDU & pdu);
  
    /**Handle an incoming response PDU.
      */
    virtual void OnReceivedResponse(
      SIPTransaction & transaction,
      SIP_PDU & response
    );

    /**Handle an incoming Trying response PDU
      */
    virtual void OnReceivedTrying(SIP_PDU & pdu);
  
    /**Handle an incoming Ringing response PDU
      */
    virtual void OnReceivedRinging(SIP_PDU & pdu);
  
    /**Handle an incoming Session Progress response PDU
      */
    virtual void OnReceivedSessionProgress(SIP_PDU & pdu);
  
    /**Handle an incoming Proxy Authentication Required response PDU
       Returns: PTrue if handled, if PFalse is returned connection is released.
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
    virtual void OnCreatingINVITE(SIP_PDU & pdu);

    /**Callback from the RTP session for statistics monitoring.
       This is called every so many packets on the transmitter and receiver
       threads of the RTP session indicating that the statistics have been
       updated.

       The default behaviour does nothing.
      */
    virtual void OnRTPStatistics(
      const RTP_Session & session         ///<  Session with statistics
    ) const;
  //@}


    /**Forward incoming connection to the specified address.
       This would typically be called from within the OnIncomingConnection()
       function when an application wishes to redirect an unwanted incoming
       call.

       The return value is PTrue if the call is to be forwarded, PFalse 
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
    
    /**Send a "200 OK" response for the received INVITE message.
     */
    virtual PBoolean SendInviteOK(const SDPSessionDescription & sdp);
	
    /**Send a response for the received INVITE message.
     */
    virtual PBoolean SendInviteResponse(
      SIP_PDU::StatusCodes code,
      const char * contact = NULL,
      const char * extra = NULL,
      const SDPSessionDescription * sdp = NULL
    );

    /**Send a PDU using the connection transport.
     * The PDU is sent to the address given as argument.
     */
    virtual PBoolean SendPDU(SIP_PDU &, const OpalTransportAddress &);

    unsigned GetNextCSeq() { return ++lastSentCSeq; }

    OpalTransportAddress GetLocalAddress(WORD port = 0) const;

    OpalTransport & GetTransport() const { return *transport; }

    virtual PString GetLocalPartyAddress() const { return localPartyAddress; }
    virtual PString GetExplicitFrom() const;

    /** Create full SIPURI - with display name, URL in <> and tag, suitable for From:
      */
    virtual void SetLocalPartyAddress();
    void SetLocalPartyAddress(
      const PString & addr
    ) { localPartyAddress = addr; }

    /**Get the remote party address.
       This will return the "best guess" at an address to use in a
       to call the user again later.
      */
    const PString GetRemotePartyCallbackURL() const;

    SIPEndPoint & GetEndPoint() const { return endpoint; }
    const SIPURL & GetTargetAddress() const { return targetAddress; }
    const PStringList & GetRouteSet() const { return routeSet; }
    const SIPAuthentication & GetAuthenticator() const { return authentication; }

    PBoolean OnOpenIncomingMediaChannels();

#if OPAL_VIDEO
    /**Call when SIP INFO of type application/media_control+xml is received.

       Return PFalse if default reponse of Failure_UnsupportedMediaType is to be returned

      */
    virtual PBoolean OnMediaControlXML(SIP_PDU & pdu);
#endif

  protected:
    PDECLARE_NOTIFIER(PTimer, SIPConnection, OnInviteResponseRetry);
    PDECLARE_NOTIFIER(PTimer, SIPConnection, OnAckTimeout);

    virtual RTP_UDP *OnUseRTPSession(
      const unsigned rtpSessionId,
      const OpalTransportAddress & mediaAddress,
      OpalTransportAddress & localAddress
    );

    virtual bool OnSendSDP(
      bool isAnswerSDP,
      RTP_SessionManager & rtpSessions,
      SDPSessionDescription & sdpOut
    );
    virtual bool OfferSDPMediaDescription(
             const OpalMediaType & mediaType,
                          unsigned channelId,
              RTP_SessionManager & rtpSessions,
           SDPSessionDescription & sdpOut
    );
    virtual bool AnswerSDPMediaDescription(
      const SDPSessionDescription & sdpIn,
      PINDEX idx,
      SDPSessionDescription & sdpOut
    );

    virtual void OnReceivedSDP(
      SIP_PDU & pdu
    );
    virtual bool OnReceivedSDPMediaDescription(
      SDPSessionDescription & sdp,
      const OpalMediaType & mediaType,
      unsigned sessionId
    );
    friend class SIPInvite;
    static PBoolean WriteINVITE(OpalTransport & transport, void * param);

    OpalTransport * CreateTransport(const OpalTransportAddress & address, PBoolean isLocalAddress = PFalse);

    void UpdateRemotePartyNameAndNumber();

    SIPEndPoint         & endpoint;
    OpalTransport       * transport;

    PMutex                transportMutex;
    bool                  local_hold;
    bool                  remote_hold;
    PString               localPartyAddress;
    PString               forwardParty;

    SIP_PDU               * originalInvite;
    PTime                 originalInviteTime;

    bool                  needReINVITE;
    PStringList           routeSet;
    SIPURL                targetAddress;
    SIPAuthentication     authentication;

    PTimer                    ackTimer;
    PTimer                    ackRetry;
    SIP_PDU                   ackPacket;
    bool                      ackReceived;
    PSafePtr<SIPTransaction>  referTransaction;
    PSafeList<SIPTransaction> forkedInvitations; // Not for re-INVITE
    PAtomicInteger            lastSentCSeq;

    enum {
      ReleaseWithBYE,
      ReleaseWithCANCEL,
      ReleaseWithResponse,
      ReleaseWithNothing,
    } releaseMethod;

    OpalMediaFormatList remoteFormatList;

    PString explicitFrom;
};


/**This class is for encpsulating the IETF Real Time Protocol interface.
 */
class SIP_RTP_Session : public RTP_UserData
{
  PCLASSINFO(SIP_RTP_Session, RTP_UserData);

  /**@name Construction */
  //@{
    /**Create a new channel.
     */
    SIP_RTP_Session(
      const SIPConnection & connection  ///<  Owner of the RTP session
    );
  //@}

  /**@name Overrides from RTP_UserData */
  //@{
    /**Callback from the RTP session for transmit statistics monitoring.
       This is called every RTP_Session::senderReportInterval packets on the
       transmitter indicating that the statistics have been updated.

       The default behaviour calls H323Connection::OnRTPStatistics().
      */
    virtual void OnTxStatistics(
      const RTP_Session & session   ///<  Session with statistics
    ) const;

    /**Callback from the RTP session for receive statistics monitoring.
       This is called every RTP_Session::receiverReportInterval packets on the
       receiver indicating that the statistics have been updated.

       The default behaviour calls H323Connection::OnRTPStatistics().
      */
    virtual void OnRxStatistics(
      const RTP_Session & session   ///<  Session with statistics
    ) const;

#if OPAL_VIDEO
    /**Callback from the RTP session after an IntraFrameRequest is receieved.
       The default behaviour executes an OpalVideoUpdatePicture command on the
       connection's source video stream if it exists.
      */
    virtual void OnRxIntraFrameRequest(
      const RTP_Session & session   ///<  Session with statistics
    ) const;

    /**Callback from the RTP session after an IntraFrameRequest is sent.
       The default behaviour does nothing.
      */
    virtual void OnTxIntraFrameRequest(
      const RTP_Session & session   ///<  Session with statistics
    ) const;
#endif
  //@}

  protected:
    const SIPConnection & connection; /// Owner of the RTP session
};


#endif // __OPAL_SIPCON_H


// End of File ///////////////////////////////////////////////////////////////
