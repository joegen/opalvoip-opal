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
 * Revision 1.2021  2004/04/26 05:40:38  rjongbloed
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
      OpalCall & call,            /// Owner calll for connection
      SIPEndPoint & endpoint,     /// Owner endpoint for connection
      const PString & token,      /// token to identify the connection
      const SIPURL & address,     /// Destination address for outgoing call
      OpalTransport * transport   /// Transport INVITE came in on
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
      const PString & calleeName,   /// Name of endpoint being alerted.
      BOOL withMedia
    );

    /**Indicate to remote endpoint we are connected.

       The default behaviour does nothing.
      */
    virtual BOOL SetConnected();

    /**Get the data formats this endpoint is capable of operating in.
      */
    virtual OpalMediaFormatList GetMediaFormats() const;

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
      const OpalMediaFormat & mediaFormat, /// Media format for stream
      unsigned sessionID,                  /// Session number for stream
      BOOL isSource                        /// Is a source stream
    );

    /**See if the media can bypass the local host.

       The default behaviour returns FALSE indicating that media bypass is not
       possible.
     */
    virtual BOOL IsMediaBypassPossible(
      unsigned sessionID                  /// Session ID for media channel
    ) const;

    /**Clean up the termination of the connection.
       This function can do any internal cleaning up and waiting on background
       threads that may be using the connection object.

       Note that there is not a one to one relationship with the
       OnEstablishedConnection() function. This function may be called without
       that function being called. For example if SetUpConnection() was used
       but the call never completed.

       The return value indicates if the connection object is to be deleted. A
       value of FALSE can be returned and it then someone elses responsibility
       to free the memory used.

       Classes that override this function should make sure they call the
       ancestor version for correct operation.

       An application will not typically call this function as it is used by
       the OpalManager during a release of the connection.

       The default behaviour calls the OpalEndPoint function of the same name.
      */
    virtual BOOL OnReleased();
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
      const RTP_Session & session         /// Session with statistics
    ) const;
  //@}

    void SendResponseToINVITE(SIP_PDU::StatusCodes code, const char * str = NULL);

    unsigned GetNextCSeq() { return ++lastSentCSeq; }

    SDPSessionDescription * BuildSDP(
      RTP_SessionManager & rtpSessions,
      unsigned rtpSessionId
    );

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

    const PString & GetTag() const { return tag; }
    SIPEndPoint & GetEndPoint() const { return endpoint; }
    const SIPURL & GetTargetAddress() const { return targetAddress; }
    const PStringList & GetRouteSet() const { return routeSet; }

    const SIPAuthentication & GetAuthentication() const { return authentication; }

  protected:
    PDECLARE_NOTIFIER(PThread, SIPConnection, HandlePDUsThreadMain);
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
    static BOOL WriteINVITE(OpalTransport & transport, void * param);

    SIPEndPoint   & endpoint;
    OpalTransport * transport;

    PString           localPartyAddress;
    PString           tag;              // local dialog tag
    SIP_PDU         * originalInvite;
    PStringList       routeSet;
    SIPURL            targetAddress;
    SIPAuthentication authentication;

    SIP_PDU_Queue pduQueue;
    PSemaphore    pduSemaphore;
    PThread     * pduHandler;

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
      const SIPConnection & connection  /// Owner of the RTP session
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
      const RTP_Session & session   /// Session with statistics
    ) const;

    /**Callback from the RTP session for receive statistics monitoring.
       This is called every RTP_Session::receiverReportInterval packets on the
       receiver indicating that the statistics have been updated.

       The default behaviour calls H323Connection::OnRTPStatistics().
      */
    virtual void OnRxStatistics(
      const RTP_Session & session   /// Session with statistics
    ) const;
  //@}


  protected:
    const SIPConnection & connection; /// Owner of the RTP session
};


#endif // __OPAL_SIPCON_H


// End of File ///////////////////////////////////////////////////////////////
