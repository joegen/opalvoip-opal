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
 * Revision 1.2009  2002/04/08 02:40:13  robertj
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

#ifdef __GNUC__
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
      OpalTransport * transport   /// Transport INVITE came in on
    );

    /**Destroy connection.
     */
    ~SIPConnection();
  //@}

  /**@name Overrides from OpalConnection */
  //@{
    /**Get the phase of the connection.
      */
    virtual Phases GetPhase() const;

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
      BOOL isSource,      /// Is a source stream
      unsigned sessionID  /// Session number for stream
    );

    /**See if the media can bypass the local host.

       The default behaviour returns FALSE indicating that media bypass is not
       possible.
     */
    virtual BOOL IsMediaBypassPossible(
      unsigned sessionID                  /// Session ID for media channel
    ) const;

    /**Get information on the media channel for the connection.
       The default behaviour returns TRUE and fills the info structure if
       there is a media channel active for the sessionID.
     */
    virtual BOOL GetMediaInformation(
      unsigned sessionID,     /// Session ID for media channel
      MediaInformation & info /// Information on media channel
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
    virtual void OnReceivedAuthenticationRequired(SIP_PDU & pdu);
  
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
  //@}

    void SendResponseToINVITE(SIP_PDU::StatusCodes code, const char * str = NULL);

    unsigned GetNextCSeq() { return ++lastSentCSeq; }

    void InitiateCall(
      const SIPURL & destination
    );

    SDPSessionDescription * BuildSDP();

    OpalTransportAddress GetLocalAddress(WORD port = 0) const;
    RTP_Session * UseSession(unsigned rtpSessionId);

    OpalTransport & GetTransport() const { return *transport; }

    PString GetLocalPartyAddress() const { return localPartyAddress; }
    void SetLocalPartyAddress();
    void SetLocalPartyAddress(
      const PString & addr
    ) { localPartyAddress = addr; }

    SIPEndPoint & GetEndPoint() const { return endpoint; }

    const SIPAuthentication & GetAuthentication() const { return authentication; }

    void AddTransaction(
      SIPTransaction * transaction
    ) { transactions.SetAt(transaction->GetTransactionID(), transaction); }

    void RemoveTransaction(
      SIPTransaction * transaction
    ) { transactions.SetAt(transaction->GetTransactionID(), NULL); }

  protected:
    PDECLARE_NOTIFIER(PThread, SIPConnection, HandlePDUsThreadMain);
    virtual void OnReceivedSDP(SIP_PDU & pdu);
    static BOOL WriteINVITE(OpalTransport & transport, PObject * param);

    SIPEndPoint   & endpoint;
    Phases          currentPhase;
    OpalTransport * transport;

    PString   localPartyAddress;
    SIP_PDU * originalInvite;
    SIPURL    originalDestination;
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
    RTP_SessionManager  rtpSessions;

    PDICTIONARY(MediaTransportDict, POrdinalKey, OpalTransportAddress);
    MediaTransportDict mediaTransports;
};


#endif // __OPAL_SIPCON_H


// End of File ///////////////////////////////////////////////////////////////
