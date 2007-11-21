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
 * $Id$
 */

#ifndef __OPAL_SIPEP_H
#define __OPAL_SIPEP_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#ifndef _PTLIB_H
#include <ptlib.h>
#endif

#include <opal/endpoint.h>
#include <sip/sipcon.h>
#include <sip/sippdu.h>
#include <sip/handlers.h> 

//
//  provide flag so applications know if presence is available
//
#define OPAL_HAS_SIP_PRESENCE   1
    

/////////////////////////////////////////////////////////////////////////

/** Class to hold authentication information 
  */

class SIPAuthInfo : public PObject
{
  public:
    SIPAuthInfo()
    { }

    SIPAuthInfo(const PString & u, const PString & p)
    { username = u; password = p; }
    PString username;
    PString password;
};

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

  /**@name Overrides from OpalEndPoint */
  //@{
    /**Get the default transports for the endpoint type.
       Overrides the default behaviour to return udp and tcp.
      */
    virtual PString GetDefaultTransport() const {  return "udp$,tcp$"; }

    /**Handle new incoming connection from listener.

       The default behaviour does nothing.
      */
    virtual BOOL NewIncomingConnection(
      OpalTransport * transport  ///<  Transport connection came in on
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
      OpalCall & call,                         ///<  Owner of connection
      const PString & party,                   ///<  Remote party to call
      void * userData,                         ///<  Arbitrary data to pass to connection
      unsigned int options,                    ///<  options to pass to conneciton
      OpalConnection::StringOptions * stringOptions  ///<  complex string options
    );

    /**Get the data formats this endpoint is capable of operating.
       This provides a list of media data format names that may be used by an
       OpalMediaStream may be created by a connection from this endpoint.

       Note that a specific connection may not actually support all of the
       media formats returned here, but should return no more.

       The default behaviour is pure.
      */
    virtual OpalMediaFormatList GetMediaFormats() const;

    /** Execute garbage collection for endpoint.
        Returns TRUE if all garbage has been collected.
        Default behaviour deletes the objects in the connectionsActive list.
      */
    virtual BOOL GarbageCollection();
  //@}

  /**@name Customisation call backs */
  //@{
    /**Create a connection for the SIP endpoint.
       The default implementation is to create a OpalSIPConnection.
      */
    virtual SIPConnection * CreateConnection(
      OpalCall & call,                         ///<  Owner of connection
      const PString & token,                   ///<  token used to identify connection
      void * userData,                         ///<  User data for connection
      const SIPURL & destination,              ///<  Destination for outgoing call
      OpalTransport * transport,               ///<  Transport INVITE has been received on
      SIP_PDU * invite,                        ///<  Original INVITE pdu
      unsigned int options = 0,                ///<  connection options
      OpalConnection::StringOptions * stringOptions = NULL ///<  complex string options

    );
    
    /**Setup a connection transfer a connection for the SIP endpoint.
      */
    virtual BOOL SetupTransfer(
      const PString & token,        ///<  Existing connection to be transferred
      const PString & callIdentity, ///<  Call identity of the secondary call (if it exists)
      const PString & remoteParty,  ///<  Remote party to transfer the existing call to
      void * userData = NULL        ///<  user data to pass to CreateConnection
    );
    
    /**Forward the connection using the same token as the specified connection.
       Return TRUE if the connection is being redirected.
      */
    virtual BOOL ForwardConnection(
      SIPConnection & connection,     ///<  Connection to be forwarded
      const PString & forwardParty    ///<  Remote party to forward to
    );

  //@}
  
  /**@name Protocol handling routines */
  //@{

    /**Creates an OpalTransport instance, based on the
       address is interpreted as the remote address to which the transport should connect
      */
    OpalTransport * CreateTransport(
      const OpalTransportAddress & remoteAddress,
      const OpalTransportAddress & localAddress = OpalTransportAddress()
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

    /**Handle an incoming IntervalTooBrief response PDU
      */
    virtual void OnReceivedIntervalTooBrief(
      SIPTransaction & transaction, 
      SIP_PDU & response)
    ;
  
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
    
    /**Handle an incoming NOTIFY PDU.
      */
    virtual BOOL OnReceivedNOTIFY(
      OpalTransport & transport,
      SIP_PDU & response
    );

    /**Handle an incoming REGISTER PDU.
      */
    virtual BOOL OnReceivedREGISTER(
      OpalTransport & transport, 
      SIP_PDU & pdu
    );

    /**Handle an incoming SUBSCRIBE PDU.
      */
    virtual BOOL OnReceivedSUBSCRIBE(
      OpalTransport & transport, 
      SIP_PDU & pdu
    );

    /**Handle an incoming MESSAGE PDU.
      */
    virtual void OnReceivedMESSAGE(
      OpalTransport & transport,
      SIP_PDU & response
    );
    
    /**Handle a timeout
      */
    virtual void OnTransactionTimeout(
      SIPTransaction & transaction
    );
    
    /**Callback from the RTP session for statistics monitoring.
       This is called every so many packets on the transmitter and receiver
       threads of the RTP session indicating that the statistics have been
       updated.

       The default behaviour does nothing.
      */
    virtual void OnRTPStatistics(
      const SIPConnection & connection,  ///<  Connection for the channel
      const RTP_Session & session         ///<  Session with statistics
    ) const;
  //@}
 

    /**Find a connection that uses the specified token.
       This searches the endpoint for the connection that contains the token
       as provided by functions such as MakeConnection().
      */
    PSafePtr<SIPConnection> GetSIPConnectionWithLock(
      const PString & token,     ///<  Token to identify connection
      PSafetyMode mode = PSafeReadWrite
    ) { return PSafePtrCast<OpalConnection, SIPConnection>(GetConnectionWithLock(token, mode)); }

    virtual BOOL IsAcceptedAddress(const SIPURL & toAddr);


    /**Callback for SIP message received
     */
    virtual void OnMessageReceived (const SIPURL & from,
                    const PString & body);

                
    /**Register to a registrar. This function is asynchronous to permit
     * several registrations to occur at the same time. It can be
     * called several times for different hosts and users.
     * 
     * The username can be of the form user@domain. In that case,
     * the From field will be set to that value.
     * 
     * The realm can be specified when registering, this will
     * allow to find the correct authentication information when being
     * requested. If no realm is specified, authentication will
     * occur with the "best guess" of authentication parameters.
     */
    BOOL Register(
      unsigned expire,
      const PString & aor = PString::Empty(),                 ///< user@host
      const PString & authName = PString::Empty(),            ///< authentication user name
      const PString & password = PString::Empty(),            ///< authentication password
      const PString & authRealm = PString::Empty(),           ///< authentication realm
      const PTimeInterval & minRetryTime = PMaxTimeInterval, 
      const PTimeInterval & maxRetryTime = PMaxTimeInterval
    );
    BOOL Register(
      const PString & host,
      const PString & user = PString::Empty(),
      const PString & autName = PString::Empty(),
      const PString & password = PString::Empty(),
      const PString & authRealm = PString::Empty(),
      unsigned expire = 0,
      const PTimeInterval & minRetryTime = PMaxTimeInterval, 
      const PTimeInterval & maxRetryTime = PMaxTimeInterval
    );
    

    /**Unregister from a registrar. 
     */
    BOOL Unregister(const PString & aor);

    
    /**Subscribe to a notifier. This function is asynchronous to permit
     * several subscriptions to occur at the same time.
     */
    BOOL Subscribe(
      SIPSubscribe::SubscribeType & type,
      unsigned expire,
      const PString & to
    );


    BOOL Unsubscribe(
      SIPSubscribe::SubscribeType & type,
      const PString & to
    );


    /**Send a message to the given URL.
     */
    BOOL Message (
      const PString & to, 
      const PString & body
    );
    

    /**Publish new state information.
     * Only the basic & note fields of the PIDF+xml are supported for now.
     */
    BOOL Publish(
      const PString & to,
      const PString & body,
      unsigned expire = 0
    );
    

    /**Send a SIP PING to the remote host
      */
    BOOL Ping(
      const PString & to
    );

    /**Callback called when MWI is received
     */
    virtual void OnMWIReceived (
      const PString & to,
      SIPSubscribe::MWIType type,
      const PString & msgs);
    
    
    /**Callback called when MWI is received
     */
    virtual void OnPresenceInfoReceived (
      const PString & user,
      const PString & basic,
      const PString & note);

    
    /**Callback called when a registration to a SIP registrar status.
     * The BOOL indicates if the operation that failed was a REGISTER or 
     * an (UN)REGISTER.
     */
    virtual void OnRegistrationStatus(
      const PString & aor,
      BOOL wasRegistering,
      BOOL reRegistering,
      SIP_PDU::StatusCodes reason
    );

    /**Callback called when a registration to a SIP registrars fails.
     * The BOOL indicates if the operation that failed was a REGISTER or 
     * an (UN)REGISTER.
     */
    virtual void OnRegistrationFailed(
      const PString & aor,
      SIP_PDU::StatusCodes reason,
      BOOL wasRegistering
    );

    /**Callback called when a registration or an unregistration is successful.
     * The BOOL indicates if the operation that failed was a registration or
     * not.
     */
    virtual void OnRegistered(
      const PString & aor,
      BOOL wasRegistering
    );

    
    /**Returns TRUE if the given URL has been registered 
     * (e.g.: 6001@seconix.com).
     */
    BOOL IsRegistered(const PString & aor);

    /**Returns TRUE if the endpoint is subscribed to some
     * event for the given to address.
     */
    BOOL IsSubscribed(
      SIPSubscribe::SubscribeType type,
      const PString & to); 


    /** Returns the number of registered accounts.
     */
    unsigned GetRegistrationsCount () { return activeSIPHandlers.GetRegistrationsCount (); }
    

    /**Callback called when a message sent by the endpoint didn't reach
     * its destination or when the proxy or remote endpoint returns
     * an error code.
     */
    virtual void OnMessageFailed(
      const SIPURL & messageUrl,
      SIP_PDU::StatusCodes reason);
    

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

    void SetRegistrarTimeToLive(
      const PTimeInterval & t
    ) { registrarTimeToLive = t; }
    const PTimeInterval & GetRegistrarTimeToLive() const { return registrarTimeToLive; }
    
    void SetNotifierTimeToLive(
      const PTimeInterval & t
    ) { notifierTimeToLive = t; }
    const PTimeInterval & GetNotifierTimeToLive() const { return notifierTimeToLive; }
    
    void SetNATBindingTimeout(
      const PTimeInterval & t
    ) { natBindingTimeout = t; natBindingTimer.RunContinuous (natBindingTimeout); }
    const PTimeInterval & GetNATBindingTimeout() const { return natBindingTimeout; }

    void AddTransaction(
      SIPTransaction * transaction
    ) { transactions.SetAt(transaction->GetTransactionID(), transaction); }

    PSafePtr<SIPTransaction> GetTransaction(const PString & transactionID, PSafetyMode mode = PSafeReadWrite)
    { return transactions.FindWithLock(transactionID, mode); }
    
    /**Return the next CSEQ for the next transaction.
     */
    unsigned GetNextCSeq() { return ++lastSentCSeq; }

    
    /**Return the SIPAuthentication for a specific realm.
     */
    BOOL GetAuthentication(const PString & authRealm, SIPAuthentication &); 
    

    /**Return the registered party name URL for the given host.
     *
     * That URL can be used in the FORM field of the PDU's. 
     * The host part can be different from the registration domain.
     */
    virtual SIPURL GetRegisteredPartyName(const SIPURL &);


    /**Return the default registered party name URL.
     */
    virtual SIPURL GetDefaultRegisteredPartyName();
    

    /**Return the contact URL for the given host and user name
     * based on the listening port of the registration to that host.
     * 
     * That URL can be used as as contact field in outgoing
     * requests.
     *
     * The URL is translated if required.
     *
     * If no active registration is used, return the result of GetLocalURL
     * on the given transport.
     */
    SIPURL GetContactURL(const OpalTransport &transport, const PString & userName, const PString & host);


    /**Return the local URL for the given transport and user name.
     * That URL can be used as via address, and as contact field in outgoing
     * requests.
     *
     * The URL is translated if required.
     *
     * If the transport is not running, the first listener transport
     * will be used, if any.
     */
    virtual SIPURL GetLocalURL(
       const OpalTransport & transport,             ///< Transport on which we can receive new requests
       const PString & userName = PString::Empty()  ///< The user name part of the contact field
    );
    
    
    /**Return the outbound proxy URL, if any.
     */
    const SIPURL & GetProxy() const { return proxy; }

    
    /**Set the outbound proxy URL.
     */
    void SetProxy(const SIPURL & url);
    
    
    /**Set the outbound proxy URL.
     */
    void SetProxy(
      const PString & hostname,
      const PString & username,
      const PString & password
    );

    
    /**Get the User Agent for this endpoint.
       Default behaviour returns an empty string so the SIPConnection builds
       a valid string from the productInfo data.

       These semantics are for backward compatibility.
     */
    virtual PString GetUserAgent() const;
        
    /**Set the User Agent for the endpoint.
     */
    void SetUserAgent(const PString & str) { userAgentString = str; }


    BOOL SendResponse(
      SIP_PDU::StatusCodes code, 
      OpalTransport & transport, 
      SIP_PDU & pdu
    );

    /**NAT Binding Refresh Method
     */
    enum NATBindingRefreshMethod{
      None,
      Options,
      EmptyRequest,
      NumMethods
    };


    /**Set the NAT Binding Refresh Method
     */
    void SetNATBindingRefreshMethod(const NATBindingRefreshMethod m) { natMethod = m; }

    virtual SIPRegisterHandler * CreateRegisterHandler(const PString & to,
                                                       const PString & authName, 
                                                       const PString & password, 
                                                       const PString & realm,
                                                                   int expire,
                                                 const PTimeInterval & minRetryTime, 
                                                 const PTimeInterval & maxRetryTime);

  protected:
    PDECLARE_NOTIFIER(PThread, SIPEndPoint, TransportThreadMain);
    PDECLARE_NOTIFIER(PTimer, SIPEndPoint, NATBindingRefresh);


    void ParsePartyName(
      const PString & remoteParty,     ///<  Party name string.
      PString & party                  ///<  Parsed party name, after e164 lookup
    );


    SIPURL        proxy;
    PString       userAgentString;

    BOOL          mimeForm;
    unsigned      maxRetries;
    PTimeInterval retryTimeoutMin;   // T1
    PTimeInterval retryTimeoutMax;   // T2
    PTimeInterval nonInviteTimeout;  // T3
    PTimeInterval pduCleanUpTimeout; // T4
    PTimeInterval inviteTimeout;
    PTimeInterval ackTimeout;
    PTimeInterval registrarTimeToLive;
    PTimeInterval notifierTimeToLive;
    PTimeInterval natBindingTimeout;
    
    SIPHandlersList   activeSIPHandlers;

    PSafeDictionary<PString, SIPTransaction> transactions;

    PTimer                  natBindingTimer;
    NATBindingRefreshMethod natMethod;
    
    PAtomicInteger          lastSentCSeq;    
};

#endif // __OPAL_SIPEP_H


// End of File ///////////////////////////////////////////////////////////////
