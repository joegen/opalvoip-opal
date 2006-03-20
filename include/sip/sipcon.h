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
 * $Log: sipcon.h,v $
 * Revision 1.2043.2.1  2006/03/20 02:25:26  csoutheren
 * Backports from CVS head
 *
 * Revision 2.45  2006/03/08 21:54:54  dsandras
 * Forgot to commit this file.
 *
 * Revision 2.44  2006/03/06 22:52:59  csoutheren
 * Reverted experimental SRV patch due to unintended side effects
 *
 * Revision 2.43  2006/03/06 12:56:02  csoutheren
 * Added experimental support for SIP SRV lookups
 *
 * Revision 2.42  2006/02/10 23:44:03  csoutheren
 * Applied fix for SetConnection and RFC2833 startup
 *
 * Revision 2.41  2006/01/02 14:47:28  dsandras
 * More code cleanups.
 *
 * Revision 2.40  2005/11/30 13:35:26  csoutheren
 * Changed tags for Doxygen
 *
 * Revision 2.39  2005/10/22 12:16:05  dsandras
 * Moved mutex preventing media streams to be opened before they are completely closed to the SIPConnection class.
 *
 * Revision 2.38  2005/10/13 19:33:50  dsandras
 * Added GetDirection to get the default direction for a media stream. Modified OnSendMediaDescription to call BuildSDP if no reverse streams can be opened.
 *
 * Revision 2.37  2005/10/04 12:57:18  rjongbloed
 * Removed CanOpenSourceMediaStream/CanOpenSinkMediaStream functions and
 *   now use overides on OpenSourceMediaStream/OpenSinkMediaStream
 *
 * Revision 2.36  2005/09/27 16:03:46  dsandras
 * Removed SendResponseToInvite and added SendPDU function that will change the transport address right before sending the PDU. This operation is atomic.
 *
 * Revision 2.35  2005/09/15 17:08:36  dsandras
 * Added support for CanOpen[Source|Sink]MediaStream.
 *
 * Revision 2.34  2005/08/25 18:49:52  dsandras
 * Added SIP Video support. Changed API of BuildSDP to allow it to be called
 * for both audio and video.
 *
 * Revision 2.33  2005/07/11 01:52:24  csoutheren
 * Extended AnsweringCall to work for SIP as well as H.323
 * Fixed problems with external RTP connection in H.323
 * Added call to OnClosedMediaStream
 *
 * Revision 2.32  2005/05/06 07:37:06  csoutheren
 * Various changed while working with SIP carrier
 *   - remove assumption that authentication realm is a domain name.
 *   - stopped rewrite of "To" field when proxy being used
 *   - fix Contact field in REGISTER to match actual port used when Symmetric NATin use
 *   - lots of formatting changes and cleanups
 *
 * Revision 2.31  2005/04/28 20:22:53  dsandras
 * Applied big sanity patch for SIP thanks to Ted Szoczei <tszoczei@microtronix.ca>.
 * Thanks a lot!
 *
 * Revision 2.30  2005/04/10 20:59:42  dsandras
 * Added call hold support (local and remote).
 *
 * Revision 2.29  2005/04/10 20:58:21  dsandras
 * Added function to handle incoming transfer (REFER).
 *
 * Revision 2.28  2005/04/10 20:57:18  dsandras
 * Added support for Blind Transfer (transfering and being transfered)
 *
 * Revision 2.27  2005/04/10 20:54:35  dsandras
 * Added function that returns the "best guess" callback URL of a connection.
 *
 * Revision 2.26  2005/02/19 22:48:48  dsandras
 * Added the possibility to register to several registrars and be able to do authenticated calls to each of them. Added SUBSCRIBE/NOTIFY support for Message Waiting Indications.
 *
 * Revision 2.25  2005/01/16 11:28:05  csoutheren
 * Added GetIdentifier virtual function to OpalConnection, and changed H323
 * and SIP descendants to use this function. This allows an application to
 * obtain a GUID for any connection regardless of the protocol used
 *
 * Revision 2.24  2004/12/25 20:43:41  dsandras
 * Attach the RFC2833 handlers when we are in connected state to ensure
 * OpalMediaPatch exist. Fixes problem for DTMF sending.
 *
 * Revision 2.23  2004/12/22 18:53:18  dsandras
 * Added definition for ForwardCall.
 *
 * Revision 2.22  2004/08/20 12:13:31  rjongbloed
 * Added correct handling of SIP 180 response
 *
 * Revision 2.21  2004/08/14 07:56:30  rjongbloed
 * Major revision to utilise the PSafeCollection classes for the connections and calls.
 *
 * Revision 2.20  2004/04/26 05:40:38  rjongbloed
 * Added RTP statistics callback to SIP
 *
 * Revision 2.19  2004/03/14 10:09:53  rjongbloed
 * Moved transport on SIP top be constructed by endpoint as any transport created on
 *   an endpoint can receive data for any connection.
 *
 * Revision 2.18  2004/03/13 06:30:03  rjongbloed
 * Changed parameter in UDP write function to void * from PObject *.
 *
 * Revision 2.17  2004/02/24 11:33:46  rjongbloed
 * Normalised RTP session management across protocols
 * Added support for NAT (via STUN)
 *
 * Revision 2.16  2004/02/07 02:20:32  rjongbloed
 * Changed to allow opening of more than just audio streams.
 *
 * Revision 2.15  2003/12/20 12:21:18  rjongbloed
 * Applied more enhancements, thank you very much Ted Szoczei
 *
 * Revision 2.14  2003/03/17 10:26:59  robertj
 * Added video support.
 *
 * Revision 2.13  2003/03/06 03:57:47  robertj
 * IVR support (work in progress) requiring large changes everywhere.
 *
 * Revision 2.12  2002/09/16 02:52:35  robertj
 * Added #define so can select if #pragma interface/implementation is used on
 *   platform basis (eg MacOS) rather than compiler, thanks Robert Monaghan.
 *
 * Revision 2.11  2002/04/16 07:53:15  robertj
 * Changes to support calls through proxies.
 *
 * Revision 2.10  2002/04/10 03:13:45  robertj
 * Moved code for handling media bypass address resolution into ancestor as
 *   now done ths same way in both SIP and H.323.
 * Major changes to RTP session management when initiating an INVITE.
 *
 * Revision 2.9  2002/04/09 01:02:14  robertj
 * Fixed problems with restarting INVITE on  authentication required response.
 *
 * Revision 2.8  2002/04/08 02:40:13  robertj
 * Fixed issues with using double originate call, eg from simple app command line.
 *
 * Revision 2.7  2002/04/05 10:42:04  robertj
 * Major changes to support transactions (UDP timeouts and retries).
 *
 * Revision 2.6  2002/03/15 10:55:28  robertj
 * Added ability to specify proxy username/password in URL.
 *
 * Revision 2.5  2002/03/08 06:28:19  craigs
 * Changed to allow Authorisation to be included in other PDUs
 *
 * Revision 2.4  2002/02/19 07:52:40  robertj
 * Restructured media bypass functions to fix problems with RFC2833.
 *
 * Revision 2.3  2002/02/13 04:55:59  craigs
 * Fixed problem with endless loop if proxy keeps failing authentication with 407
 *
 * Revision 2.2  2002/02/11 07:34:06  robertj
 * Changed SDP to use OpalTransport for hosts instead of IP addresses/ports
 * Added media bypass for streams between compatible protocols.
 *
 * Revision 2.1  2002/02/01 04:53:01  robertj
 * Added (very primitive!) SIP support.
 *
 */

#ifndef __OPAL_SIPCON_H
#define __OPAL_SIPCON_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif


#include <opal/connection.h>
#include <sip/sippdu.h>


class OpalCall;
class SIPEndPoint;


/////////////////////////////////////////////////////////////////////////

/**Session Initiation Protocol connection.
 */
class SIPConnection : public OpalConnection
{
  PCLASSINFO(SIPConnection, OpalConnection);
  public:

  /**@name Construction */
  //@{
    /**Create a new connection.
     */
    SIPConnection(
      OpalCall & call,            ///<  Owner call for connection
      SIPEndPoint & endpoint,     ///<  Owner endpoint for connection
      const PString & token,      ///<  token to identify the connection
      const SIPURL & address,     ///<  Destination address for outgoing call
      OpalTransport * transport   ///<  Transport INVITE came in on
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
    virtual BOOL SetUpConnection();

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

    /**Return TRUE if the current connection is on hold.
     */
    virtual BOOL IsConnectionOnHold();

    /**Indicate to remote endpoint an alert is in progress.
       If this is an incoming connection and the AnswerCallResponse is in a
       AnswerCallDeferred or AnswerCallPending state, then this function is
       used to indicate to that endpoint that an alert is in progress. This is
       usually due to another connection which is in the call (the B party)
       has received an OnAlerting() indicating that its remote endpoint is
       "ringing".

       The default behaviour does nothing.
      */
    virtual BOOL SetAlerting(
      const PString & calleeName,   ///<  Name of endpoint being alerted.
      BOOL withMedia
    );

    /**Indicate to remote endpoint we are connected.

       The default behaviour does nothing.
      */
    virtual BOOL SetConnected();

    /**Get the data formats this endpoint is capable of operating in.
      */
    virtual OpalMediaFormatList GetMediaFormats() const;
    
    /**Open source transmitter media stream for session.
      */
    virtual BOOL OpenSourceMediaStream(
      const OpalMediaFormatList & mediaFormats, ///<  Optional media format to open
      unsigned sessionID                   ///<  Session to start stream on
    );

    /**Open source transmitter media stream for session.
      */
    virtual OpalMediaStream * OpenSinkMediaStream(
      OpalMediaStream & source    ///<  Source media sink format to open to
    );

    /**Open a new media stream.
       This will create a media stream of an appropriate subclass as required
       by the underlying connection protocol. For instance H.323 would create
       an OpalRTPStream.

       The sessionID parameter may not be needed by a particular media stream
       and may be ignored. In the case of an OpalRTPStream it us used.

       Note that media streams may be created internally to the underlying
       protocol. This function is not the only way a stream can come into
       existance.

       The default behaviour is pure.
     */
    virtual OpalMediaStream * CreateMediaStream(
      const OpalMediaFormat & mediaFormat, ///<  Media format for stream
      unsigned sessionID,                  ///<  Session number for stream
      BOOL isSource                        ///<  Is a source stream
    );


    /**Call back for answering an incoming call.
       This function is called from the OnReceivedSignalSetup() function
       before it sends the 200 OK response. 

       It also gives an application time to wait for some event before
       signalling to the endpoint that the connection is to proceed. For
       example the user pressing an "Answer call" button.

       If AnswerCallDenied is returned the connection is aborted and a 200 OK 
       is sent. If AnswerCallNow is returned then the SIP protocol proceeds. 
       Finally if AnswerCallPending is returned then the protocol negotiations 
       are paused until the AnsweringCall() function is called.

       The default behaviour simply returns AnswerNow.
     */
    virtual OpalConnection::AnswerCallResponse OnAnswerCall(
      const PString & callerName      ///<  Name of caller
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


    /**A call back function whenever a connection is "connected".
       This indicates that a connection to an endpoint was connected. That
       is the endpoint received acknowledgement via whatever protocol it uses
       that the connection may now start media streams.

       In the context of H.323 this means that the CONNECT pdu has been
       received.

       The default behaviour calls the OpalConnection function of the same 
       name after having connected the RFC2833 handler to the OpalPatch.
      */
    virtual void OnConnected();

    /**See if the media can bypass the local host.

       The default behaviour returns FALSE indicating that media bypass is not
       possible.
     */
    virtual BOOL IsMediaBypassPossible(
      unsigned sessionID                  ///<  Session ID for media channel
    ) const;

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
      */
    virtual void OnReceivedAuthenticationRequired(
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
  
    /**Queue a PDU for the PDU handler thread to handle.
       Any listener threads on the endpoint upon receiving a PDU and
       determining the SIPConnection to use from the Call-ID field queues up
       the PDU so that it can get back to getting PDU's as quickly as
       possible. All time consuming operations on the PDU are done in the
       separate thread.
      */
    void QueuePDU(
      SIP_PDU * pdu
    );

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

       The return value is TRUE if the call is to be forwarded, FALSE 
       otherwise. Note that if the call is forwarded, the current connection
       is cleared with the ended call code set to EndedByCallForwarded.
      */
    virtual BOOL ForwardCall(
      const PString & forwardParty   ///<  Party to forward call to.
    );
    
    /**Send a PDU using the connection transport.
     * The PDU is sent to the address given as argument.
     */
    BOOL SendPDU(SIP_PDU &, const OpalTransportAddress &);

    unsigned GetNextCSeq() { return ++lastSentCSeq; }

    BOOL BuildSDP(
      SDPSessionDescription * &,			     
      RTP_SessionManager & rtpSessions,
      unsigned rtpSessionId
    );

	SIPTransaction * GetTransaction (PString transactionID) { return transactions.GetAt(transactionID); }

    void AddTransaction(
      SIPTransaction * transaction
    ) { transactions.SetAt(transaction->GetTransactionID(), transaction); }

    void RemoveTransaction(
      SIPTransaction * transaction
    ) { transactions.SetAt(transaction->GetTransactionID(), NULL); }


    OpalTransportAddress GetLocalAddress(WORD port = 0) const;

    OpalTransport & GetTransport() const { return *transport; }

    PString GetLocalPartyAddress() const { return localPartyAddress; }

    /** Create full SIPURI - with display name, URL in <> and tag, suitable for From:
      */
    void SetLocalPartyAddress();
    void SetLocalPartyAddress(
      const PString & addr
    ) { localPartyAddress = addr; }

    /**Get the remote party address.
       This will return the "best guess" at an address to use in a
       to call the user again later.
      */
    const PString GetRemotePartyCallbackURL() const;

    PString GetTag() const { return GetIdentifier().AsString(); }
    SIPEndPoint & GetEndPoint() const { return endpoint; }
    const SIPURL & GetTargetAddress() const { return targetAddress; }
    const PStringList & GetRouteSet() const { return routeSet; }
    const SIPAuthentication & GetAuthenticator() const { return authentication; }

  protected:
    PDECLARE_NOTIFIER(PThread, SIPConnection, HandlePDUsThreadMain);
    virtual RTP_UDP *OnUseRTPSession(
      const unsigned rtpSessionId,
      const OpalTransportAddress & mediaAddress,
      OpalTransportAddress & localAddress
    );
    virtual void OnReceivedSDP(SIP_PDU & pdu);
    virtual BOOL OnReceivedSDPMediaDescription(
      SDPSessionDescription & sdp,
      SDPMediaDescription::MediaType mediaType,
      unsigned sessionId
    );
    virtual BOOL OnSendSDPMediaDescription(
      const SDPSessionDescription & sdpIn,
      SDPMediaDescription::MediaType mediaType,
      unsigned sessionId,
      SDPSessionDescription & sdpOut
    );
    virtual BOOL OnOpenSourceMediaStreams(
      const OpalMediaFormatList & remoteFormatList,
      unsigned sessionId,
      SDPMediaDescription *localMedia
    );
    SDPMediaDescription::Direction GetDirection(unsigned sessionId);
    static BOOL WriteINVITE(OpalTransport & transport, void * param);

    void InitRFC2833Handler();

    SIPEndPoint         & endpoint;
    OpalTransport       * transport;
    OpalTransportAddress  lastTransportAddress;

    PMutex		  transportMutex;
    PMutex                streamsMutex;
    BOOL	          local_hold;
    BOOL	          remote_hold;
    PString               localPartyAddress;
    PString	          forwardParty;
    SIP_PDU             * originalInvite;
    SDPSessionDescription remoteSDP;
    PStringList           routeSet;
    SIPURL                targetAddress;
    SIPAuthentication     authentication;

    SIP_PDU_Queue pduQueue;
    PSemaphore    pduSemaphore;
    PThread     * pduHandler;

    SIPTransaction   * referTransaction;
    SIPTransactionList invitations;
    SIPTransactionDict transactions;
    unsigned           lastSentCSeq;

    enum {
      ReleaseWithBYE,
      ReleaseWithCANCEL,
      ReleaseWithResponse,
      ReleaseWithNothing,
    } releaseMethod;

    OpalMediaFormatList remoteFormatList;
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
  //@}


  protected:
    const SIPConnection & connection; /// Owner of the RTP session
};


#endif // __OPAL_SIPCON_H


// End of File ///////////////////////////////////////////////////////////////
