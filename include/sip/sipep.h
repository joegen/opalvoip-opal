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
 * $Revision$
 * $Author$
 * $Date$
 */

#ifndef OPAL_SIP_SIPEP_H
#define OPAL_SIP_SIPEP_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#ifndef _PTLIB_H
#include <ptlib.h>
#endif

#include <opal/buildopts.h>

#if OPAL_SIP

#include <opal/rtpep.h>
#include <sip/sipcon.h>
#include <sip/handlers.h> 


//
//  provide flag so applications know if presence is available
//
#define OPAL_HAS_SIP_PRESENCE   1

/////////////////////////////////////////////////////////////////////////

/**Session Initiation Protocol endpoint.
 */
class SIPEndPoint : public OpalRTPEndPoint
{
  PCLASSINFO(SIPEndPoint, OpalRTPEndPoint);

  public:
  /**@name Construction */
  //@{
    /**Create a new endpoint.
     */
    SIPEndPoint(
      OpalManager & manager,
      unsigned maxThreads = 15
    );

    /**Destroy endpoint.
     */
    ~SIPEndPoint();
  //@}

  /**@name Overrides from OpalEndPoint */
  //@{
    /**Shut down the endpoint, this is called by the OpalManager just before
       destroying the object and can be handy to make sure some things are
       stopped before the vtable gets clobbered.
      */
    virtual void ShutDown();

    /**Get the default transports for the endpoint type.
       Overrides the default behaviour to return udp and tcp.
      */
    virtual PString GetDefaultTransport() const;

    /**Build a list of network accessible URIs given a user name.
       This typically gets URI's like sip:user@interface, h323:user@interface
       etc, for each listener of each endpoint.
      */
    virtual PStringList GetNetworkURIs(
      const PString & name
    ) const;

    /**Handle new incoming connection from listener.

       The default behaviour does nothing.
      */
    virtual PBoolean NewIncomingConnection(
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
       false is returned.

       This function usually returns almost immediately with the connection
       continuing to occur in a new background thread.

       If false is returned then the connection could not be established. For
       example if a PSTN endpoint is used and the assiciated line is engaged
       then it may return immediately. Returning a non-NULL value does not
       mean that the connection will succeed, only that an attempt is being
       made.

       The default behaviour is pure.
     */
    virtual PSafePtr<OpalConnection> MakeConnection(
      OpalCall & call,                         ///<  Owner of connection
      const PString & party,                   ///<  Remote party to call
      void * userData,                         ///<  Arbitrary data to pass to connection
      unsigned int options,                    ///<  options to pass to conneciton
      OpalConnection::StringOptions * stringOptions  ///<  complex string options
    );

    /**A call back function whenever a connection is broken.
       This function can do any internal cleaning up and waiting on background
       threads that may be using the connection object.

       Note that there is not a one to one relationship with the
       OnEstablishedConnection() function. This function may be called without
       that function being called. For example if MakeConnection() was used
       but the call never completed.

       Classes that override this function should make sure they call the
       ancestor version for correct operation.

       An application will not typically call this function as it is used by
       the OpalManager during a release of the connection.

       The default behaviour removes the connection from the internal database
       and calls the OpalManager function of the same name.
      */
    virtual void OnReleased(
      OpalConnection & connection   ///<  Connection that was established
    );

    /**Call back when conferencing state information changes.
       If a conferencing endpoint type detects a change in a conference nodes
       state, as would be returned by GetConferenceStatus() then this function
       will be called on all endpoints in the OpalManager.

       The \p uri parameter is as is the internal URI for the conference.
      */
    virtual void OnConferenceStatusChanged(
      OpalEndPoint & endpoint,  /// < Endpoint sending state change
      const PString & uri,      ///< Internal URI of conference node that changed
      OpalConferenceState::ChangeType change ///< Change that occurred
    );

    /** Execute garbage collection for endpoint.
        Returns true if all garbage has been collected.
        Default behaviour deletes the objects in the connectionsActive list.
      */
    virtual PBoolean GarbageCollection();
  //@}

  /**@name Customisation call backs */
  //@{
    /**Create a connection for the SIP endpoint.
       The default implementation is to create a OpalSIPConnection.
      */
    virtual SIPConnection * CreateConnection(
      const SIPConnection::Init & init     ///< Initialisation parameters

    );
    
    /**Setup a connection transfer a connection for the SIP endpoint.
      */
    virtual PBoolean SetupTransfer(
      const PString & token,        ///<  Existing connection to be transferred
      const PString & callIdentity, ///<  Call identity of the secondary call (if it exists)
      const PString & remoteParty,  ///<  Remote party to transfer the existing call to
      void * userData = NULL        ///<  user data to pass to CreateConnection
    );
    
    /**Forward the connection using the same token as the specified connection.
       Return true if the connection is being redirected.
      */
    virtual PBoolean ForwardConnection(
      SIPConnection & connection,     ///<  Connection to be forwarded
      const PString & forwardParty    ///<  Remote party to forward to
    );

    /**Clear a SIP connection by dialog identifer informataion.
       This function does not require an OPAL connection to operate, it will
       attempt to send a BYE to the dialog identified by the information in
       the SIPDialogContext structure.

       This feature can be useful for servers that had an "unexpected exit"
       and various clients it was talking to at the time do not implement the
       RFC4028 session timers, so continue to try and send media forever. They
       need to be told to cease and desist.
      */
    bool ClearDialogContext(
      const PString & descriptor  ///< Descriptor string for call clearance
    );
    bool ClearDialogContext(
      SIPDialogContext & context  ///< Context for call clearance
    );
  //@}
  
  /**@name Protocol handling routines */
  //@{

    /**Creates an OpalTransport instance, based on the
       address is interpreted as the remote address to which the transport should connect
      */
    OpalTransport * CreateTransport(
      const SIPURL & remoteURL,
      const PString & localInterface = PString::Empty(),
      SIP_PDU::StatusCodes * reason = NULL
    );

    virtual void HandlePDU(
      OpalTransport & transport
    );

    /**Handle an incoming SIP PDU that has been full decoded
      */
    virtual PBoolean OnReceivedPDU(
      OpalTransport & transport,
      SIP_PDU * pdu
    );

    /**Handle an incoming SIP PDU not assigned to any connection
      */
    virtual bool OnReceivedConnectionlessPDU(
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
    virtual PBoolean OnReceivedINVITE(
      OpalTransport & transport,
      SIP_PDU * pdu
    );

    /**Handle an incoming NOTIFY PDU.
      */
    virtual PBoolean OnReceivedNOTIFY(
      OpalTransport & transport,
      SIP_PDU & response
    );

    /**Handle an incoming REGISTER PDU.
      */
    virtual PBoolean OnReceivedREGISTER(
      OpalTransport & transport, 
      SIP_PDU & pdu
    );

    /**Handle an incoming SUBSCRIBE PDU.
      */
    virtual PBoolean OnReceivedSUBSCRIBE(
      OpalTransport & transport, 
      SIP_PDU & pdu,
      SIPDialogContext * dialog
    );

    /**Handle an incoming MESSAGE PDU.
      */
    virtual bool OnReceivedMESSAGE(
      OpalTransport & transport,
      SIP_PDU & response
    );

    /**Handle an incoming OPTIONS PDU.
      */
    virtual bool OnReceivedOPTIONS(
      OpalTransport & transport,
      SIP_PDU & response
    );
    
    /**Handle a SIP packet transaction failure
      */
    virtual void OnTransactionFailed(
      SIPTransaction & transaction
    );
  //@}
 

    /**Find a connection that uses the specified token.
       This searches the endpoint for the connection that contains the token
       as provided by functions such as MakeConnection().

       Note the token may be a "replaces" style value of the form:
         "callid;to-tag=tag;from-tag=tag"
      */
    PSafePtr<SIPConnection> GetSIPConnectionWithLock(
      const PString & token,     ///<  Token to identify connection
      PSafetyMode mode = PSafeReadWrite,  ///< Lock mode
      SIP_PDU::StatusCodes * errorCode = NULL ///< Error code if lock fails
    );

    virtual PBoolean IsAcceptedAddress(const SIPURL & toAddr);


    /**Register an entity to a registrar.
       This function is asynchronous to permit several registrations to occur
       at the same time. It can be called several times for different hosts
       and users.

       The params.m_addressOfRecord field is the only field required, though
       typically params.m_password is also required. A registration for the
       user part of params.m_addressOfRecord is made to the a registrar
       associated with the domain part of the field. The authentication
       identity is the same as the user field, though this may be set to
       soemthing different via the params.m_authID field.

       The params.m_registrarAddress may indicate the specific hostname to use
       for the registrar rather than using the domain part of
       params.m_addressOfRecord field.

       To aid in flexbility if the params.m_addressOfRecord does not contain
       a domain and the params.m_registrarAddress does, the the AOR is
       constructed from them.

       The params.m_realm can be specified when registering, this will
       allow to find the correct authentication information when being
       requested. If no realm is specified, authentication will
       occur with the "best guess" of authentication parameters.

       The Contact address is normally constructed from the listeners active
       on the SIPEndPoint. This may be overridden to an explicit value via the
       params.m_contactAddress field.

       The returned "token" is a string that can be used in functions
       such as Unregister() or IsRegistered(). While it possible to use the
       AOR for those functions, it is not recommended as a) there may be more
       than one registration for an AOR and b) the AOR may be constructed from
     */
    bool Register(
      const SIPRegister::Params & params, ///< Registration parameters
      PString & aor,                      ///< Resultant address-of-record for unregister
      SIP_PDU::StatusCodes * reason = NULL ///< If not null, wait for completion, may take some time
    );

    // For backward compatibility
    bool Register(
      const SIPRegister::Params & params, ///< Registration parameters
      PString & aor,                      ///< Resultant address-of-record for unregister
      bool asynchronous                   ///< Wait for completion, may take some time
    );

    /// Registration function for backward compatibility.
    bool P_DEPRECATED Register(
      const PString & host,
      const PString & user = PString::Empty(),
      const PString & autName = PString::Empty(),
      const PString & password = PString::Empty(),
      const PString & authRealm = PString::Empty(),
      unsigned expire = 0,
      const PTimeInterval & minRetryTime = PMaxTimeInterval, 
      const PTimeInterval & maxRetryTime = PMaxTimeInterval
    );

    /**Determine if there is a registration for the entity.
       The "token" parameter string is typically the string returned by the
       Register() function which is guaranteed to uniquely identify the
       specific registration.

       For backward compatibility, the AOR can also be used, but as it is
       possible to have two registrations to the same AOR, this should be
       avoided.

       The includeOffline parameter indicates if the caller is interested in
       if we are, to the best of our knowledge, currently registered (have
       had recent confirmation) or we are not sure if we are registered or
       not, but are continually re-trying.
     */
    PBoolean IsRegistered(
      const PString & aor,          ///< AOR returned by Register()
      bool includeOffline = false   ///< Include offline registrations
    );

    /**Unregister the address-of-record from a registrar.
       The "token" parameter string is typically the string returned by the
       Register() function which is guaranteed to uniquely identify the
       specific registration.

       For backward compatibility, the AOR can also be used, but as it is
       possible to have two registrations to the same AOR, this should be
       avoided.
     */
    bool Unregister(
      const PString & aor    ///< AOR returned by Register()
    );

    /**Unregister all current registrations.
       Returns true if at least one registrar is unregistered.
      */
    bool UnregisterAll();

    /** Returns the number of registered accounts.
     */
    unsigned GetRegistrationsCount() const { return activeSIPHandlers.GetCount(SIP_PDU::Method_REGISTER); }

    /** Returns a list of registered accounts.
     */
    PStringList GetRegistrations(
      bool includeOffline = false   ///< Include offline registrations
    ) const { return activeSIPHandlers.GetAddresses(includeOffline, SIP_PDU::Method_REGISTER); }

    /** Information provided on the registration status. */
    struct RegistrationStatus {
      SIPRegisterHandler * m_handler;           ///< Handler for registration
      PString              m_addressofRecord;   ///< Address of record for registration
      bool                 m_wasRegistering;    ///< Was registering or unregistering
      bool                 m_reRegistering;     ///< Was a registration refresh
      SIP_PDU::StatusCodes m_reason;            ///< Reason for status change
      OpalProductInfo      m_productInfo;       ///< Server product info from registrar if available.
      void               * m_userData;          ///< User data corresponding to this registration
    };

    /** Get current registration status.
        Returns false if there is no registration that match the aor/call-id
      */
    bool GetRegistrationStatus(
      const PString & token,          /// Address-of-record, or call-id
      RegistrationStatus & status     ///< Returned status
    );

    /**Callback called when a registration to a SIP registrar status.
     */
    virtual void OnRegistrationStatus(
      const RegistrationStatus & status   ///< Status of registration request
    );

    // For backward compatibility
    virtual void OnRegistrationStatus(
      const PString & aor,
      PBoolean wasRegistering,
      PBoolean reRegistering,
      SIP_PDU::StatusCodes reason
    );

    /**Callback called when a registration to a SIP registrars fails.
       Deprecated, maintained for backward compatibility, use OnRegistrationStatus().
     */
    virtual void OnRegistrationFailed(
      const PString & aor,
      SIP_PDU::StatusCodes reason,
      PBoolean wasRegistering
    );

    /**Callback called when a registration or an unregistration is successful.
       Deprecated, maintained for backward compatibility, use OnRegistrationStatus().
     */
    virtual void OnRegistered(
      const PString & aor,
      PBoolean wasRegistering
    );


    /**Subscribe to an agent to get event notifications.
       This function is asynchronous to permit several subscriptions to occur
       at the same time. It can be called several times for different hosts
       and users.

       The params.m_eventPackage and params.m_addressOfRecord field are the
       only field required, though typically params.m_password is also
       required. A subscription for the user part of params.m_addressOfRecord
       is made to the an agent associated with the domain part of the field.
       The authentication identity is the same as the user field, though this
       may be set to soemthing different via the params.m_authID field.

       The params.m_agentAddress may indicate the specific hostname to use
       for the registrar rather than using the domain part of
       params.m_addressOfRecord field.

       To aid in flexbility if the params.m_addressOfRecord does not contain
       a domain and the params.m_agentAddress does, the the AOR is
       constructed from them.

       The params.m_realm can be specified when subcribing, this will
       allow to find the correct authentication information when being
       requested. If no realm is specified, authentication will
       occur with the "best guess" of authentication parameters.

       The Contact address is normally constructed from the SIPEndPoint local
       identity.

       The returned "token" is a string that can be used in functions
       such as Unregister() or IsRegistered(). While it possible to use the
       user supplied AOR for those functions, it is not recommended as a) there
       may be more than one registration for an AOR and b) the AOR may be
       constructed from fields in the params argument and not be what the user
       expects.

       The tokenIsAOR can also be used, if false, to return the globally unique
       ID used for the handler. This is the preferred method, even though the
       default is to not use it, that is for backward compatibility reasons.
     */
    bool Subscribe(
      const SIPSubscribe::Params & params, ///< Subscription parameters
      PString & token,                     ///< Resultant token for unsubscribe
      bool tokenIsAOR = true               ///< Token is the address-of-record
    );

    // For backward compatibility
    bool Subscribe(
      SIPSubscribe::PredefinedPackages eventPackage, ///< Event package being unsubscribed
      unsigned expire,                               ///< Expiry time in seconds
      const PString & aor                            ///< Address-of-record for subscription
    );

    /**Returns true if the endpoint is subscribed to some
       event for the given to address. The includeOffline parameter indicates
       if the caller is interested in if we are, to the best of our knowlegde,
       currently subscribed (have had recent confirmation) or we are not sure
       if we are subscribed or not, but are continually re-trying.
     */
    bool IsSubscribed(
      const PString & aor,           ///< AOR returned by Subscribe()
      bool includeOffline = false    ///< Include offline subscription
    );
    bool IsSubscribed(
      const PString & eventPackage,  ///< Event package being unsubscribed
      const PString & aor,           ///< Address-of-record for subscription
      bool includeOffline = false    ///< Include offline subscription
    );

    /**Unsubscribe a current subscriptions.
       The "token" parameter string is typically the string returned by the
       Subscribe() function which is guaranteed to uniquely identify the
       specific registration.

       For backward compatibility, the AOR can also be used, but as it is
       possible to have two susbcriptions to the same AOR, this should be
       avoided.
      */
    bool Unsubscribe(
      const PString & aor,             ///< Unique token for registration
      bool invalidateNotifiers = false ///< Notifiers in SIPSubscribe::Params are to be reset
    );
    bool Unsubscribe(
      SIPSubscribe::PredefinedPackages eventPackage,  ///< Event package being unsubscribed
      const PString & aor,                            ///< Address-of-record for subscription
      bool invalidateNotifiers = false                ///< Notifiers in SIPSubscribe::Params are to be reset
    );
    bool Unsubscribe(
      const PString & eventPackage,  ///< Event package being unsubscribed
      const PString & aor,             ///< Address-of-record for subscription
      bool invalidateNotifiers = false ///< Notifiers in SIPSubscribe::Params are to be reset
    );

    /**Unsubscribe all current subscriptions.
       Returns true if at least one subscription is unsubscribed.
      */
    bool UnsubcribeAll(
      SIPSubscribe::PredefinedPackages eventPackage  ///< Event package being unsubscribed
    );
    bool UnsubcribeAll(
      const PString & eventPackage  ///< Event package being unsubscribed
    );

    /** Returns the number of registered accounts.
     */
    unsigned GetSubscriptionCount(
      const SIPSubscribe::EventPackage & eventPackage  ///< Event package of subscription
    ) { return activeSIPHandlers.GetCount(SIP_PDU::Method_SUBSCRIBE, eventPackage); }

    /** Returns a list of subscribed accounts for package.
     */
    PStringList GetSubscriptions(
      const SIPSubscribe::EventPackage & eventPackage, ///< Event package of subscription
      bool includeOffline = false   ///< Include offline subscriptions
    ) const { return activeSIPHandlers.GetAddresses(includeOffline, SIP_PDU::Method_REGISTER, eventPackage); }

    /** Information provided on the subscription status. */
    typedef SIPSubscribe::SubscriptionStatus SubscriptionStatus;

    /** Get current registration status.
        Returns false if there is no registration that match the aor/call-id
      */
    bool GetSubscriptionStatus(
      const PString & token,          /// Address-of-record, or call-id
      const PString & eventPackage,   ///< Event package being unsubscribed
      SubscriptionStatus & status     ///< Returned status
    );

    /**Callback called when a subscription to a SIP UA status changes.
     */
    virtual void OnSubscriptionStatus(
      const SubscriptionStatus & status   ///< Status of subscription request
    );

    /**Callback called when a subscription to a SIP UA status changes.
       Now deprecated - called by OnSubscriptionStatus that accepts SIPHandler
     */
    virtual void OnSubscriptionStatus(
      const PString & eventPackage, ///< Event package subscribed to
      const SIPURL & uri,           ///< Target URI for the subscription.
      bool wasSubscribing,          ///< Indication the subscribing or unsubscribing
      bool reSubscribing,           ///< If subscribing then indication was refeshing subscription
      SIP_PDU::StatusCodes reason   ///< Status of subscription
    );

    virtual void OnSubscriptionStatus(
      SIPSubscribeHandler & handler, ///< Event subscription paramaters
      const SIPURL & uri,            ///< Target URI for the subscription.
      bool wasSubscribing,           ///< Indication the subscribing or unsubscribing
      bool reSubscribing,            ///< If subscribing then indication was refeshing subscription
      SIP_PDU::StatusCodes reason    ///< Status of subscription
    );

    /** Indicate notifications for the specified event package are supported.
      */
    virtual bool CanNotify(
      const PString & eventPackage ///< Event package we support
    );

    enum CanNotifyResult {
      CannotNotify,
      CanNotifyImmediate,
      CanNotifyDeferrred
    };
    virtual CanNotifyResult CanNotify(
      const PString & eventPackage, ///< Event package we support
      const SIPURL & aor
    );

    /** Send notification to all remotes that are subcribed to the event package.
      */
    bool Notify(
      const SIPURL & targetAddress, ///< Address that was subscribed
      const PString & eventPackage, ///< Event package for notification
      const PObject & body          ///< Body of notification
    );


    /**Callback called when dialog NOTIFY message is received
     */
    virtual void OnDialogInfoReceived(
      const SIPDialogNotification & info  ///< Information on dialog state change
    );

    void SendNotifyDialogInfo(
      const SIPDialogNotification & info  ///< Information on dialog state change
    );


    /**Callback called when reg NOTIFY message is received
     */
    virtual void OnRegInfoReceived(
      const SIPRegNotification & info  ///< Information on dialog state change
    );


    /**Send SIP message
     */
    bool SendMESSAGE(
      SIPMessage::Params & params
    );

    /**Callback called when a message completes in any manner
     */
    virtual void OnMESSAGECompleted(
      const SIPMessage::Params & params,
      SIP_PDU::StatusCodes reason
    );

    struct ConnectionlessMessageInfo {
      ConnectionlessMessageInfo(OpalTransport & transport, SIP_PDU & pdu)
        : m_pdu(pdu), m_transport(transport), m_status(ResponseSent)
      { }

      SIP_PDU & m_pdu;
      OpalTransport & m_transport;
      enum {
        NotHandled,
        SendOK,
        MethodNotAllowed,
        ResponseSent
      } m_status;
    };

    typedef PNotifierTemplate<ConnectionlessMessageInfo &> ConnectionlessMessageNotifier;
    #define PDECLARE_ConnectionlessMessageNotifier(cls, fn) PDECLARE_NOTIFIER2(SIPEndPoint, cls, fn, SIPEndPoint::ConnectionlessMessageInfo &)
    #define PCREATE_ConnectionlessMessageNotifier(fn) PCREATE_NOTIFIER2(fn, SIPEndPoint::ConnectionlessMessageInfo &)

    void SetConnectionlessMessageNotifier(
      const ConnectionlessMessageNotifier & notifier
    )
    { m_onConnectionlessMessage = notifier; }


    /**Send SIP OPTIONS
     */
    virtual bool SendOPTIONS(
      const SIPOptions::Params & params
    );

    /**Callback called when an OPTIONS command is completed, either
       successfully or with error.
     */
    virtual void OnOptionsCompleted(
      const SIPOptions::Params & params,    ///< Original parameter for SendOPTIONS() call
      const SIP_PDU & response              ///< Response packet, check GetStatusCode() for result
    );


    /**Publish new state information.
       A expire time of zero will cease to automatically update the publish.
     */
    bool Publish(
      const SIPSubscribe::Params & params,
      const PString & body,
      PString & aor
    );
    bool Publish(
      const PString & to,   ///< Address to send PUBLISH
      const PString & body, ///< Body of PUBLISH
      unsigned expire = 300 ///< Time between automatic updates in seconds.
    );

    /** Returns a list of published entities.
     */
    PStringList GetPublications(
      const SIPSubscribe::EventPackage & eventPackage, ///< Event package for publication
      bool includeOffline = false   ///< Include offline publications
    ) const { return activeSIPHandlers.GetAddresses(includeOffline, SIP_PDU::Method_PUBLISH, eventPackage); }


    /**Publish new state information.
     * Only the basic & note fields of the PIDF+xml are supported for now.
     */
    bool PublishPresence(
      const SIPPresenceInfo & info,  ///< Presence info to publish
      unsigned expire = 300          ///< Refresh time
    );
    
    /**Callback called when presence is received
     */
    virtual void OnPresenceInfoReceived (
      const SIPPresenceInfo & info  ///< Presence info publicised
    );
    virtual void OnPresenceInfoReceived (
      const PString & identity,
      const PString & basic,
      const PString & note
    );


    /**Send a SIP PING to the remote host
      */
    PBoolean Ping(
      const PURL & to
    );

    /** Get the allowed events for SUBSCRIBE commands.
      */
    const PStringSet & GetAllowedEvents() const { return m_allowedEvents; }

    /**Get default mode for PRACK support.
      */
    SIPConnection::PRACKMode GetDefaultPRACKMode() const { return m_defaultPrackMode; }

    /**Set default mode for PRACK support.
      */
    void SetDefaultPRACKMode(SIPConnection::PRACKMode mode) { m_defaultPrackMode = mode; }

    void SetMIMEForm(PBoolean v) { mimeForm = v; }
    PBoolean GetMIMEForm() const { return mimeForm; }

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

    void SetProgressTimeout(
      const PTimeInterval & t
    ) { m_progressTimeout = t; }
    const PTimeInterval & GetProgressTimeout() const { return m_progressTimeout; }

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

    void GetKeepAlive(
      PTimeInterval & keepAliveTimeout,
      PBYTEArray & keepAliveData
    ) { keepAliveTimeout = m_keepAliveTimeout; keepAliveData = m_keepAliveData; }
    void SetKeepAlive(
      const PTimeInterval & keepAliveTimeout,
      const PBYTEArray & keepAliveData
    ) { m_keepAliveTimeout = keepAliveTimeout; m_keepAliveData = keepAliveData; }


    void AddTransaction(
      SIPTransaction * transaction
    ) { m_transactions.Append(transaction); }

    PSafePtr<SIPTransaction> GetTransaction(const PString & transactionID, PSafetyMode mode = PSafeReadWrite)
    { return PSafePtrCast<SIPTransactionBase, SIPTransaction>(m_transactions.FindWithLock(transactionID, mode)); }
    
    /**Return the next CSEQ for the next transaction.
     */
    unsigned GetNextCSeq() { return ++lastSentCSeq; }

    /**Set registration search mode.
       If true then only the user indicated as "local" address
       (e.g. from OPAL_OPT_CALLING_PARTY_NAME etc) by the will be used
       in searches of the registrations. No default to first user of the
       same domain will be performed. */
    void SetRegisteredUserMode(bool v) { m_registeredUserMode = v; }
    bool GetRegisteredUserMode() const { return m_registeredUserMode; }

    /**Return the SIPAuthentication for a specific realm.
     */
    bool GetAuthentication(const PString & authRealm, PString & user, PString & password); 
    
    /**Return the default Contact URL.
     */
    virtual SIPURL GetDefaultLocalURL(const OpalTransport & transport);
    
    /**Adjust the outgoing PDU to registered information.
       Various header fields of the PDU must be adjusted to agree with values
       provided to/from the active registration for the domain the call is being
       made to. For example the "From" field must agree exactly with
     
       If no active registration is available, the result of GetLocalURL() on
       the given transport is set to the Contact field.
     */
    void AdjustToRegistration(
      SIP_PDU & pdu,
      const OpalTransport & transport,
      const SIPConnection * connection
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

    
    /**Get the default line appearance code for new connections.
      */
    int GetDefaultAppearanceCode() const { return m_defaultAppearanceCode; }

    /**Set the default line appearance code for new connections.
      */
    void SetDefaultAppearanceCode(int code) { m_defaultAppearanceCode = code; }

    /**Get the User Agent for this endpoint.
       Default behaviour returns an empty string so the SIPConnection builds
       a valid string from the productInfo data.

       These semantics are for backward compatibility.
     */
    virtual PString GetUserAgent() const;
        
    /**Set the User Agent for the endpoint.
     */
    void SetUserAgent(const PString & str) { userAgentString = str; }


    /** Return a bit mask of the allowed SIP methods.
      */
    virtual unsigned GetAllowedMethods() const;


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

    virtual SIPRegisterHandler * CreateRegisterHandler(const SIPRegister::Params & params);

    virtual void OnStartTransaction(SIPConnection & conn, SIPTransaction & transaction);


    PSafePtr<SIPHandler> FindSIPHandlerByCallID(const PString & callID, PSafetyMode m)
      { return activeSIPHandlers.FindSIPHandlerByCallID(callID, m); }

    void UpdateHandlerIndexes(SIPHandler * handler)
      { activeSIPHandlers.Update(handler); }


    SIPThreadPool & GetThreadPool() { return m_threadPool; }


  protected:
    PDECLARE_NOTIFIER(PThread, SIPEndPoint, TransportThreadMain);
    PDECLARE_NOTIFIER(PTimer, SIPEndPoint, NATBindingRefresh);

    SIPURL        proxy;
    PString       userAgentString;
    PStringSet    m_allowedEvents;

    SIPConnection::PRACKMode m_defaultPrackMode;

    bool          mimeForm;
    unsigned      maxRetries;
    PTimeInterval retryTimeoutMin;   // T1
    PTimeInterval retryTimeoutMax;   // T2
    PTimeInterval nonInviteTimeout;  // T3
    PTimeInterval pduCleanUpTimeout; // T4
    PTimeInterval inviteTimeout;
    PTimeInterval m_progressTimeout;
    PTimeInterval ackTimeout;
    PTimeInterval registrarTimeToLive;
    PTimeInterval notifierTimeToLive;
    PTimeInterval natBindingTimeout;
    PTimeInterval m_keepAliveTimeout;
    PBYTEArray    m_keepAliveData;
    bool          m_registeredUserMode;

    bool              m_shuttingDown;

    SIPHandlersList   activeSIPHandlers;
    PSafePtr<SIPHandler> FindHandlerByPDU(const SIP_PDU & pdu, PSafetyMode mode);

    PStringToString   m_receivedConnectionTokens;
    PMutex            m_receivedConnectionMutex;

    PSafeSortedList<SIPTransactionBase> m_transactions;

    PTimer                  natBindingTimer;
    NATBindingRefreshMethod natMethod;
    PAtomicInteger          lastSentCSeq;
    int                     m_defaultAppearanceCode;

    struct RegistrationCompletion {
      PSyncPoint           m_sync;
      SIP_PDU::StatusCodes m_reason;
      RegistrationCompletion() : m_reason(SIP_PDU::Information_Trying) { }
    };
    std::map<PString, RegistrationCompletion> m_registrationComplete;

    ConnectionlessMessageNotifier m_onConnectionlessMessage;
    typedef std::multimap<PString, SIPURL> ConferenceMap;
    ConferenceMap m_conferenceAOR;

    // Thread pooling
    SIPThreadPool m_threadPool;

    // Network interface checking
    enum {
      HighPriority = 80,
      LowPriority  = 30,
    };
    class InterfaceMonitor : public PInterfaceMonitorClient
    {
        PCLASSINFO(InterfaceMonitor, PInterfaceMonitorClient);
      public:
        InterfaceMonitor(SIPEndPoint & manager, PINDEX priority);
        
        virtual void OnAddInterface(const PIPSocket::InterfaceEntry & entry);
        virtual void OnRemoveInterface(const PIPSocket::InterfaceEntry & entry);

      protected:
        SIPEndPoint & m_endpoint;
    };
    InterfaceMonitor m_highPriorityMonitor;
    InterfaceMonitor m_lowPriorityMonitor;

    friend void InterfaceMonitor::OnAddInterface(const PIPSocket::InterfaceEntry & entry);
    friend void InterfaceMonitor::OnRemoveInterface(const PIPSocket::InterfaceEntry & entry);

    bool m_disableTrying;

    P_REMOVE_VIRTUAL_VOID(OnReceivedIntervalTooBrief(SIPTransaction &, SIP_PDU &));
    P_REMOVE_VIRTUAL_VOID(OnReceivedAuthenticationRequired(SIPTransaction &, SIP_PDU &));
    P_REMOVE_VIRTUAL_VOID(OnReceivedOK(SIPTransaction &, SIP_PDU &));
    P_REMOVE_VIRTUAL_VOID(OnMessageFailed(const SIPURL &, SIP_PDU::StatusCodes));
    P_REMOVE_VIRTUAL(SIPConnection *,CreateConnection(OpalCall &, const PString &, void *, const SIPURL &, OpalTransport *, SIP_PDU *, unsigned, OpalConnection::StringOptions *), NULL);
};


#endif // OPAL_SIP

#endif // OPAL_SIP_SIPEP_H


// End of File ///////////////////////////////////////////////////////////////
