/*
 * sipep.h
 *
 * Session Initiation Protocol endpoint.
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
 * $Log: sipep.h,v $
 * Revision 1.2014  2004/04/26 06:30:33  rjongbloed
 * Added ability to specify more than one defualt listener for an endpoint,
 *   required by SIP which listens on both UDP and TCP.
 *
 * Revision 2.12  2004/04/26 05:40:38  rjongbloed
 * Added RTP statistics callback to SIP
 *
 * Revision 2.11  2004/03/14 11:32:19  rjongbloed
 * Changes to better support SIP proxies.
 *
 * Revision 2.10  2004/03/14 10:13:03  rjongbloed
 * Moved transport on SIP top be constructed by endpoint as any transport created on
 *   an endpoint can receive data for any connection.
 * Changes to REGISTER to support authentication
 *
 * Revision 2.9  2004/03/14 08:34:09  csoutheren
 * Added ability to set User-Agent string
 *
 * Revision 2.8  2004/03/13 06:51:31  rjongbloed
 * Alllowed for empty "username" in registration
 *
 * Revision 2.7  2004/03/13 06:32:17  rjongbloed
 * Fixes for removal of SIP and H.323 subsystems.
 * More registration work.
 *
 * Revision 2.6  2003/03/06 03:57:47  robertj
 * IVR support (work in progress) requiring large changes everywhere.
 *
 * Revision 2.5  2002/09/16 02:52:35  robertj
 * Added #define so can select if #pragma interface/implementation is used on
 *   platform basis (eg MacOS) rather than compiler, thanks Robert Monaghan.
 *
 * Revision 2.4  2002/07/04 07:41:47  robertj
 * Fixed memory/thread leak of transports.
 *
 * Revision 2.3  2002/04/16 08:06:35  robertj
 * Fixed GNU warnings.
 *
 * Revision 2.2  2002/04/05 10:42:04  robertj
 * Major changes to support transactions (UDP timeouts and retries).
 *
 * Revision 2.1  2002/02/01 04:53:01  robertj
 * Added (very primitive!) SIP support.
 *
 */

#ifndef __OPAL_SIPEP_H
#define __OPAL_SIPEP_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif


#include <opal/endpoint.h>
#include <sip/sippdu.h>


class SIPConnection;


/////////////////////////////////////////////////////////////////////////

/**Session Initiation Protocol endpoint.
 */
class SIPEndPoint : public OpalEndPoint
{
  PCLASSINFO(SIPEndPoint, OpalEndPoint);

  public:
  /**@name Construction */
  //@{
    /**Create a new endpoint.
     */
    SIPEndPoint(
      OpalManager & manager
    );

    /**Destroy endpoint.
     */
    ~SIPEndPoint();
  //@}

  /**@name Overrides from OpalManager */
  //@{
    /**Get the default listeners for the endpoint type.
       Overrides the default behaviour to return udp and tcp listeners.
      */
    virtual PStringArray GetDefaultListeners() const;

    /**Handle new incoming connection from listener.

       The default behaviour does nothing.
      */
    virtual BOOL NewIncomingConnection(
      OpalTransport * transport  /// Transport connection came in on
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
       FALSE is returned.

       This function usually returns almost immediately with the connection
       continuing to occur in a new background thread.

       If FALSE is returned then the connection could not be established. For
       example if a PSTN endpoint is used and the assiciated line is engaged
       then it may return immediately. Returning a non-NULL value does not
       mean that the connection will succeed, only that an attempt is being
       made.

       The default behaviour is pure.
     */
    virtual BOOL MakeConnection(
      OpalCall & call,        /// Owner of connection
      const PString & party,  /// Remote party to call
      void * userData = NULL  /// Arbitrary data to pass to connection
    );
  //@}

  /**@name Customisation call backs */
  //@{
    /**Create a connection for the SIP endpoint.
       The default implementation is to create a OpalSIPConnection.
      */
    virtual SIPConnection * CreateConnection(
      OpalCall & call,            /// Owner of connection
      const PString & token,      /// token used to identify connection
      void * userData,            /// User data for connection
      const SIPURL & destination, /// Destination for outgoing call
      OpalTransport * transport,  /// Transport INVITE has been received on
      SIP_PDU * invite            /// Original INVITE pdu
    );
  //@}
  
  /**@name Protocol handling routines */
  //@{
    OpalTransport * CreateTransport(
      const OpalTransportAddress & address
    );

    virtual void HandlePDU(
      OpalTransport & transport
    );

    /**Handle an incoming SIP PDU that has been full decoded
      */
    virtual BOOL OnReceivedPDU(
      OpalTransport & transport,
      SIP_PDU * pdu
    );

    /**Handle an incoming response PDU.
      */
    virtual void OnReceivedResponse(
      SIPTransaction & transaction,
      SIP_PDU & response
    );

    /**Handle an incoming INVITE request.
      */
    virtual BOOL OnReceivedINVITE(
      OpalTransport & transport,
      SIP_PDU * pdu
    );
  
    /**Handle an incoming Proxy Authentication Required response PDU
      */
    virtual void OnReceivedAuthenticationRequired(
      SIPTransaction & transaction,
      SIP_PDU & response
    );

    /**Handle an incoming OK response PDU.
       This actually gets any PDU of the class 2xx not just 200.
      */
    virtual void OnReceivedOK(
      SIPTransaction & transaction,
      SIP_PDU & response
    );

    /**Callback from the RTP session for statistics monitoring.
       This is called every so many packets on the transmitter and receiver
       threads of the RTP session indicating that the statistics have been
       updated.

       The default behaviour does nothing.
      */
    virtual void OnRTPStatistics(
      const SIPConnection & connection,  /// Connection for the channel
      const RTP_Session & session         /// Session with statistics
    ) const;
  //@}
 

    SIPConnection * GetSIPConnectionWithLock(const PString & str)
      { return (SIPConnection *)GetConnectionWithLock(str); }

    void AddNewConnection(SIPConnection * conn);

    virtual BOOL IsAcceptedAddress(const SIPURL & toAddr);

    BOOL Register(
      const PString & hostname,
      const PString & username = PString::Empty(),
      const PString & password = PString::Empty()
    );

    void SetMIMEForm(BOOL v) { mimeForm = v; }
    BOOL GetMIMEForm() const { return mimeForm; }

    void SetMaxRetries(unsigned r) { maxRetries = r; }
    unsigned GetMaxRetries() const { return maxRetries; }

    void SetRetryTimeouts(
      const PTimeInterval & t1,
      const PTimeInterval & t2
    ) { retryTimeoutMin = t1; retryTimeoutMax = t2; }
    const PTimeInterval & GetRetryTimeoutMin() const { return retryTimeoutMin; }
    const PTimeInterval & GetRetryTimeoutMax() const { return retryTimeoutMax; }

    void SetNonInviteTimeout(
      const PTimeInterval & t
    ) { nonInviteTimeout = t; }
    const PTimeInterval & GetNonInviteTimeout() const { return nonInviteTimeout; }

    void SetPduCleanUpTimeout(
      const PTimeInterval & t
    ) { pduCleanUpTimeout = t; }
    const PTimeInterval & GetPduCleanUpTimeout() const { return pduCleanUpTimeout; }

    void SetInviteTimeout(
      const PTimeInterval & t
    ) { inviteTimeout = t; }
    const PTimeInterval & GetInviteTimeout() const { return inviteTimeout; }

    void SetAckTimeout(
      const PTimeInterval & t
    ) { ackTimeout = t; }
    const PTimeInterval & GetAckTimeout() const { return ackTimeout; }

    void AddTransaction(
      SIPTransaction * transaction
    ) { transactions.SetAt(transaction->GetTransactionID(), transaction); }

    void RemoveTransaction(
      SIPTransaction * transaction
    ) { transactions.SetAt(transaction->GetTransactionID(), NULL); }

    unsigned GetNextCSeq() { return ++lastSentCSeq; }

    const SIPAuthentication & GetAuthentication() const { return authentication; }

    const SIPURL & GetProxy() const { return proxy; }
    void SetProxy(const SIPURL & url) { proxy = url; }

    virtual PString GetUserAgent() const;
    void SetUserAgent(const PString & str) { userAgentString = str; }

  protected:
    PDECLARE_NOTIFIER(PThread, SIPEndPoint, TransportThreadMain);
    static BOOL WriteREGISTER(OpalTransport & transport, void * param);

  protected:
    SIPURL            proxy;
    PString           userAgentString;
    SIPAuthentication authentication;

    BOOL          mimeForm;
    unsigned      maxRetries;
    PTimeInterval retryTimeoutMin;   // T1
    PTimeInterval retryTimeoutMax;   // T2
    PTimeInterval nonInviteTimeout;  // T3
    PTimeInterval pduCleanUpTimeout; // T4
    PTimeInterval inviteTimeout;
    PTimeInterval ackTimeout;

    OpalTransport    * registrarTransport;
    SIPURL             registrationAddress;
    PString            registrationID;
    SIPTransactionList registrations;
    SIPTransactionDict transactions;
    unsigned           lastSentCSeq;
};

#endif // __OPAL_SIPEP_H


// End of File ///////////////////////////////////////////////////////////////
