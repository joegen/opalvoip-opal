/*
 * h323ep.h
 *
 * H.323 protocol handler
 *
 * Open H323 Library
 *
 * Copyright (c) 1998-2001 Equivalence Pty. Ltd.
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
 * The Original Code is Open H323 Library.
 *
 * The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * Portions of this code were written with the assisance of funding from
 * Vovida Networks, Inc. http://www.vovida.com.
 *
 * Contributor(s): ______________________________________.
 */

#ifndef OPAL_H323_H323EP_H
#define OPAL_H323_H323EP_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <opal_config.h>

#if OPAL_H323

#include <rtp/rtpep.h>
#include <opal/manager.h>
#include <opal/call.h>
#include <opal/transports.h>
#include <h323/h323con.h>
#include <h323/h323caps.h>
#include <h323/h235auth.h>
#include <h323/gkclient.h>
#include <asn/h225.h>
#include <h460/h4601.h>


class H225_EndpointType;
class H225_VendorIdentifier;
class H225_H221NonStandard;
class H225_ServiceControlDescriptor;
class H225_FeatureSet;

class H235SecurityInfo;

class H323Gatekeeper;
class H323SignalPDU;
class H323ServiceControlSession;

class H46019Server;


///////////////////////////////////////////////////////////////////////////////

/**This class manages the H323 endpoint.
   An endpoint may have zero or more listeners to create incoming connections
   or zero or more outgoing connections initiated via the MakeCall() function.
   Once a conection exists it is managed by this class instance.

   The main thing this class embodies is the capabilities of the application,
   that is the codecs and protocols it is capable of.

   An application may create a descendent off this class and overide the
   CreateConnection() function, if they require a descendent of H323Connection
   to be created. This would be quite likely in most applications.
 */
class H323EndPoint : public OpalRTPEndPoint
{
  PCLASSINFO(H323EndPoint, OpalRTPEndPoint);

  public:
    enum {
      DefaultTcpSignalPort = 1720
    };
    static const OpalProductInfo & AvayaPhone();

  /**@name Construction */
  //@{
    /**Create a new endpoint.
     */
    H323EndPoint(
      OpalManager & manager
    );

    /**Destroy endpoint.
     */
    ~H323EndPoint();
  //@}

  /**@name Overrides from OpalEndPoint */
  //@{
    /**Shut down the endpoint, this is called by the OpalManager just before
       destroying the object and can be handy to make sure some things are
       stopped before the vtable gets clobbered.
      */
    virtual void ShutDown();

    /** Execute garbage collection for endpoint.
    Returns true if all garbage has been collected.
    Default behaviour deletes the objects in the connectionsActive list.
    */
    virtual PBoolean GarbageCollection();

    /**Set up a connection to a remote party.
       This is called from the OpalManager::SetUpConnection() function once
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
      OpalCall & call,                  ///<  Owner of connection
      const PString & party,            ///<  Remote party to call
      void * userData  = NULL,          ///<  Arbitrary data to pass to connection
      unsigned int options = 0,         ///<  options to pass to connection
      OpalConnection::StringOptions * stringOptions = NULL
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

    /** Get available string option names.
    */
    virtual PStringList GetAvailableStringOptions() const;
    //@}

  /**@name Set up functions */
  //@{
    /**Set the endpoint information in H225 PDU's.
      */
    virtual void SetEndpointTypeInfo(
      H225_EndpointType & info
    ) const;

    /**Set the vendor information in H225 PDU's.
      */
    virtual void SetVendorIdentifierInfo(
      H225_VendorIdentifier & info
    ) const;

    /**Set the H221NonStandard information in H225 PDU's.
      */
    virtual void SetH221NonStandardInfo(
      H225_H221NonStandard & info
    ) const;

    /**Set the Gateway supported protocol default always H.323
      */
    virtual bool SetGatewaySupportedProtocol(
      H225_ArrayOf_SupportedProtocols & protocols
    ) const;

    /**Set the gateway prefixes 
       Override this to set the acceptable prefixes to the gatekeeper
      */
    virtual bool OnSetGatewayPrefixes(
      PStringList & prefixes
    ) const;
  //@}


  /**@name Gatekeeper management */
  //@{
    /**Use and register with an explicit gatekeeper.
       This will call other functions according to the following table:

           address    identifier   function
           empty      empty        DiscoverGatekeeper()
           non-empty  empty        SetGatekeeper()
           empty      non-empty    LocateGatekeeper()
           non-empty  non-empty    SetGatekeeperZone()

       The localAddress field, if non-empty, indicates the interface on which
       to look for the gatekeeper. An empty string is equivalent to "ip$*:*"
       which is any interface or port.

       If the endpoint is already registered with a gatekeeper that meets
       the same criteria then the gatekeeper is not changed, otherwise it is
       deleted (with unregistration) and new one created and registered to.

       Note that a gatekeeper address of "*" is treated like an empty string
       resulting in gatekeeper discovery.
     */
    bool UseGatekeeper(
      const PString & address = PString::Empty(),     ///<  Address of gatekeeper to use.
      const PString & identifier = PString::Empty(),  ///<  Identifier of gatekeeper to use.
      const PString & localAddress = PString::Empty() ///<  Local interface to use.
    );

    /**Select and register with an explicit gatekeeper.
       This will use the specified transport and a string giving a transport
       dependent address to locate a specific gatekeeper. The endpoint will
       register with that gatekeeper and, if successful, set it as the current
       gatekeeper used by this endpoint.

       Note the transport being passed in will be deleted by this function or
       the H323Gatekeeper object it becomes associated with. Also if transport
       is NULL then a H323TransportUDP is created.
     */
    bool SetGatekeeper(
      const PString & address,          ///<  Address of gatekeeper to use.
      const PString & localAddress = PString::Empty() ///<  Local interface to use.
    );

    /**Select and register with an explicit gatekeeper and zone.
       This will use the specified transport and a string giving a transport
       dependent address to locate a specific gatekeeper. The endpoint will
       register with that gatekeeper and, if successful, set it as the current
       gatekeeper used by this endpoint.

       The gatekeeper identifier is set to the spplied parameter to allow the
       gatekeeper to either allocate a zone or sub-zone, or refuse to register
       if the zones do not match.

       Note the transport being passed in will be deleted by this function or
       the H323Gatekeeper object it becomes associated with. Also if transport
       is NULL then a H323TransportUDP is created.
     */
    bool SetGatekeeperZone(
      const PString & address,          ///<  Address of gatekeeper to use.
      const PString & identifier,       ///<  Identifier of gatekeeper to use.
      const PString & localAddress = PString::Empty() ///<  Local interface to use.
    );

    /**Locate and select gatekeeper.
       This function will use the automatic gatekeeper discovery methods to
       locate the gatekeeper on the particular transport that has the specified
       gatekeeper identifier name. This is often the "Zone" for the gatekeeper.

       Note the transport being passed in will be deleted becomes owned by the
       H323Gatekeeper created by this function and will be deleted by it. Also
       if transport is NULL then a H323TransportUDP is created.
     */
    bool LocateGatekeeper(
      const PString & identifier,       ///<  Identifier of gatekeeper to locate.
      const PString & localAddress = PString::Empty() ///<  Local interface to use.
    );

    /**Discover and select gatekeeper.
       This function will use the automatic gatekeeper discovery methods to
       locate the first gatekeeper on a particular transport.

       Note the transport being passed in will be deleted becomes owned by the
       H323Gatekeeper created by this function and will be deleted by it. Also
       if transport is NULL then a H323TransportUDP is created.
     */
    bool DiscoverGatekeeper(
      const PString & localAddress = PString::Empty() ///<  Local interface to use.
    );

    /**Create a gatekeeper.
       This allows the application writer to have the gatekeeper as a
       descendent of the H323Gatekeeper in order to add functionality to the
       base capabilities in the library.

       The default creates an instance of the H323Gatekeeper class.
     */
    virtual H323Gatekeeper * CreateGatekeeper(
      H323Transport * transport  ///<  Transport over which gatekeepers communicates.
    );

    /**Get the gatekeeper we are registered with.
     */
    H323Gatekeeper * GetGatekeeper(
      const PString & alias = PString::Empty()
    ) const;

    typedef PList<H323Gatekeeper> GatekeeperList;

    /**Get all the gatekeepers we are registered with.
    */
    const GatekeeperList & GetGatekeepers() const { return m_gatekeepers; }

    /**Return if endpoint is registered with any/all gatekeepers.
      */
    PBoolean IsRegisteredWithGatekeeper(bool all = false) const;

    /**Unregister and delete the gatekeeper we are registered with.
       The return value indicates false if there was an error during the
       unregistration. However the gatekeeper is still removed and its
       instance deleted regardless of this error.
     */
    PBoolean RemoveGatekeeper(
      int reason = -1   ///<  Reason for gatekeeper removal
    );

    /**Set the H.235 password for the gatekeeper.
      */
    virtual void SetGatekeeperPassword(
      const PString & password,
      const PString & username = PString::Empty()
    );

    /** Set the password for specific alias.
    */
    void SetAliasPasswords(
      const PStringToString & aliasPasswords,
      const PString & defaultAddress
    );
    
    /**Get the H.235 username for the gatekeeper.
      */
    virtual const PString & GetGatekeeperUsername() const { return m_gatekeeperUsername; }

    /**Get the H.235 password for the gatekeeper.
      */
    virtual const PString & GetGatekeeperPassword() const { return m_gatekeeperPassword; }

    enum { MaxGatekeeperAliasLimit = 1000000 };
    /** Set gatekeeper alias limit in single RRQ.
      */
    void SetGatekeeperAliasLimit(
      PINDEX limit   ///< New limit for aliases
    );

    /** Get gatekeeper alias limit in single RRQ.
      */
    PINDEX GetGatekeeperAliasLimit() const { return m_gatekeeperAliasLimit; }

    /** Set gatekeeper flag to simulate alias pattern with aliases.
        This will generate separate individual aliases for ranges in the pattern list.
    */
    void SetGatekeeperSimulatePattern(bool sim) { m_gatekeeperSimulatePattern = sim; }

    /** Get gatekeeper flag to simulate alias pattern with aliases.
    */
    bool GetGatekeeperSimulatePattern() const { return m_gatekeeperSimulatePattern; }

    /** Set gatekeeper flag to redirect to new gatekeeper based on rasAddres field.
    */
    void SetGatekeeperRasRedirect(bool redir) { m_gatekeeperRasRedirect = redir; }

    /** Get gatekeeper flag to redirect to new gatekeeper based on rasAddres field.
    */
    bool GetGatekeeperRasRedirect() const { return m_gatekeeperRasRedirect; }

    /**Create a list of authenticators for gatekeeper.
      */
    virtual H235Authenticators CreateAuthenticators();

    /**Called when the gatekeeper status changes.
      */
    virtual void OnGatekeeperStatus(
      H323Gatekeeper & gk,
      H323Gatekeeper::RegistrationFailReasons status
    );
    // For backward compatibility
    virtual void OnGatekeeperStatus(
      H323Gatekeeper::RegistrationFailReasons status
    );
  //@}

  /**@name Connection management */
  //@{
    /**Handle new incoming connetion from listener.
      */
    virtual void NewIncomingConnection(
      OpalListener & listener,            ///<  Listner that created transport
      const OpalTransportPtr & transport  ///<  Transport connection came in on
    );

    void InternalNewIncomingConnection(
      OpalTransportPtr transport,   ///< Transport connection came in on
      bool reused = false
    );

    /**Create a connection that uses the specified call.
      */
    virtual H323Connection * CreateConnection(
      OpalCall & call,                         ///<  Call object to attach the connection to
      const PString & token,                   ///<  Call token for new connection
      void * userData,                         ///<  Arbitrary user data from MakeConnection
      OpalTransport & transport,               ///<  Transport for connection
      const PString & alias,                   ///<  Alias for outgoing call
      const H323TransportAddress & address,    ///<  Address for outgoing call
      H323SignalPDU * setupPDU,                ///<  Setup PDU for incoming call
      unsigned options = 0,
      OpalConnection::StringOptions * stringOptions = NULL ///<  complex string options
    );

    /**Setup the transfer of an existing call (connection) to a new remote party
       using H.450.2.  This sends a Call Transfer Setup Invoke message from the
       B-Party (transferred endpoint) to the C-Party (transferred-to endpoint).

       If the transport parameter is NULL the transport is determined from the
       remoteParty description. The general form for this parameter is
       [alias@][transport$]host[:port] where the default alias is the same as
       the host, the default transport is "ip" and the default port is 1720.

       This function returns almost immediately with the transfer occurring in a
       new background thread.

       This function is declared virtual to allow an application to override
       the function and get the new call token of the forwarded call.
     */
    virtual PBoolean SetupTransfer(
      const PString & token,        ///<  Existing connection to be transferred
      const PString & callIdentity, ///<  Call identity of the secondary call (if it exists)
      const PString & remoteParty,  ///<  Remote party to transfer the existing call to
      void * userData = NULL        ///<  user data to pass to CreateConnection
    );

    /**Initiate the transfer of an existing call (connection) to a new remote
       party using H.450.2.  This sends a Call Transfer Initiate Invoke
       message from the A-Party (transferring endpoint) to the B-Party
       (transferred endpoint).
     */
    void TransferCall(
      const PString & token,        ///<  Existing connection to be transferred
      const PString & remoteParty,  ///<  Remote party to transfer the existing call to
      const PString & callIdentity = PString::Empty()
                                    ///<  Call Identity of secondary call if present
    );

    /**Transfer the call through consultation so the remote party in the
       primary call is connected to the called party in the second call
       using H.450.2.  This sends a Call Transfer Identify Invoke message
       from the A-Party (transferring endpoint) to the C-Party
       (transferred-to endpoint).
     */
    void ConsultationTransfer(
      const PString & primaryCallToken,   ///<  Token of primary call
      const PString & secondaryCallToken  ///<  Token of secondary call
    );

    /** Initiate Call intrusion
        Designed similar to MakeCall function
      */
    PBoolean IntrudeCall(
      const PString & remoteParty,  ///<  Remote party to intrude call
      unsigned capabilityLevel,     ///<  Capability level
      void * userData = NULL        ///<  user data to pass to CreateConnection
    );

    /**Parse a party address into alias and transport components.
       An appropriate transport is determined from the remoteParty parameter.
       The general form for this parameter is [alias@][transport$]host[:port]
       where the default alias is the same as the host, the default transport
       is "ip" and the default port is 1720.
      */
    PBoolean ParsePartyName(
      const PString & party,          ///<  Party name string.
      PString & alias,                ///<  Parsed alias name
      H323TransportAddress & address, ///<  Parsed transport address
      OpalConnection::StringOptions * stringOptions = NULL ///< String options parsed from party name
    );

    /**Find a connection that uses the specified token.
       This searches the endpoint for the connection that contains the token
       as provided by functions such as MakeCall(). if not found it will then search for
       the string representation of the CallIdentifier for the connection, and
       finally try for the string representation of the ConferenceIdentifier.

       Note the caller of this function MUSt call the H323Connection::Unlock()
       function if this function returns a non-NULL pointer. If it does not
       then a deadlock can occur.
      */
    PSafePtr<H323Connection> FindConnectionWithLock(
      const PString & token,     ///<  Token to identify connection
      PSafetyMode mode = PSafeReadWrite
    );

    /** OnSendSignalSetup is a hook for the appliation to attach H450 info in setup, 
        for instance, H450.7 Activate or Deactivate 
        @param connection the connection associated to the setup
        @param pduSetup the setup message to modify
        @return if false, do no send the setup pdu
      */
      
    virtual PBoolean OnSendSignalSetup(H323Connection & connection,
                                   H323SignalPDU & setupPDU);

    /**Adjust call proceeding PDU being sent. This function is called from
       the OnReceivedSignalSetup() function before it sends the Call
       Proceeding PDU. It gives an opportunity for an application to alter
       the request before transmission to the other endpoint. If this function
       returns false then the Call Proceeding PDU is not sent at all.

       The default behaviour simply returns true.
       @param connection the connection associated to the call proceeding
       @param callProceedingPDU the call processding to modify
       @return if false, do no send the connect pdu  
     */
    virtual PBoolean OnSendCallProceeding(
      H323Connection & connection,
      H323SignalPDU & callProceedingPDU
    );

    /**Adjust call connect PDU being sent. This function is called from
       the H323Connection::SetConnected function before it sends the connect  PDU. 
       It gives an opportunity for an application to alter
       the request before transmission to the other endpoint. If this function
       returns false then the connect PDU is not sent at all.

       The default behaviour simply returns true.
       @param connection the connection associated to the connect
       @param connectPDU the connect to modify
       @return if false, do no send the connect pdu  
     */
    virtual PBoolean OnSendConnect(
      H323Connection & connection,
      H323SignalPDU & connectPDU
    );
    
    /**Call back for incoming call.
       This function is called from the OnReceivedSignalSetup() function
       before it sends the Alerting PDU. It gives an opportunity for an
       application to alter the reply before transmission to the other
       endpoint.

       If false is returned the connection is aborted and a Release Complete
       PDU is sent.

       The default behaviour simply returns true.
     */
    virtual PBoolean OnIncomingCall(
      H323Connection & connection,    ///<  Connection that was established
      const H323SignalPDU & setupPDU,   ///<  Received setup PDU
      H323SignalPDU & alertingPDU       ///<  Alerting PDU to send
    );

    /**Called when an outgoing call connects
       If false is returned the connection is aborted and a Release Complete
       PDU is sent.

       The default behaviour simply returns true.
      */
    virtual PBoolean OnOutgoingCall(
      H323Connection & conn, 
      const H323SignalPDU & connectPDU
    );

    /**Handle a connection transfer.
       This gives the application an opportunity to abort the transfer.
       The default behaviour just returns true.
      */
    virtual PBoolean OnCallTransferInitiate(
      H323Connection & connection,    ///<  Connection to transfer
      const PString & remoteParty     ///<  Party transferring to.
    );

    /**Handle a transfer via consultation.
       This gives the transferred-to user an opportunity to abort the transfer.
       The default behaviour just returns true.
      */
    virtual PBoolean OnCallTransferIdentify(
      H323Connection & connection    ///<  Connection to transfer
    );

    /**
      * Callback for ARQ send to a gatekeeper. This allows the endpoint
      * to change or check fields in the ARQ before it is sent.
      */
    virtual void OnSendARQ(
      H323Connection & conn,
      H225_AdmissionRequest & arq
    );

    /**Call back for answering an incoming call.
       This function is a H.323 specific version of OpalEndPoint::OnAnswerCall
       that contains additional information that applies only to H.323.

       By default this calls OpalEndPoint::OnAnswerCall, which returns
     */
    virtual OpalConnection::AnswerCallResponse OnAnswerCall(
      H323Connection & connection,    ///< Connection that was established
      const PString & callerName,       ///< Name of caller
      const H323SignalPDU & setupPDU,   ///< Received setup PDU
      H323SignalPDU & connectPDU,       ///< Connect PDU to send. 
      H323SignalPDU & progressPDU        ///< Progress PDU to send. 
    );
    virtual OpalConnection::AnswerCallResponse OnAnswerCall(
       OpalConnection & connection,
       const PString & caller
    );

    /**Call back for remote party being alerted.
       This function is called from the SendSignalSetup() function after it
       receives the optional Alerting PDU from the remote endpoint. That is
       when the remote "phone" is "ringing".

       If false is returned the connection is aborted and a Release Complete
       PDU is sent.

       The default behaviour simply returns true.
     */
    virtual PBoolean OnAlerting(
      H323Connection & connection,    ///<  Connection that was established
      const H323SignalPDU & alertingPDU,  ///<  Received Alerting PDU
      const PString & user                ///<  Username of remote endpoint
    );

    /** A call back function when the alerting is about to be sent, 
        can be used by the application to alter the alerting Pdu
        @return if false, then the alerting is not sent
     */
    virtual PBoolean OnSendAlerting(
      H323Connection & connection,  ///< onnection that was established
      H323SignalPDU & alerting,     ///< Alerting PDU to modify
      const PString & calleeName,   ///< Name of endpoint being alerted.
      PBoolean withMedia                ///< Open media with alerting
    );

    /** A call back function when the alerting has been sent, can be used by the application 
        to send the connect as soon as the alerting has been sent.
     */
    virtual PBoolean OnSentAlerting(
      H323Connection & connection
    );

    /**A call back function when a connection indicates it is to be forwarded.
       An H323 application may handle this call back so it can make
       complicated decisions on if the call forward ius to take place. If it
       decides to do so it must call MakeCall() and return true.

       The default behaviour simply returns false and that the automatic
       call forwarding should take place. See ForwardConnection()
      */
    virtual PBoolean OnConnectionForwarded(
      H323Connection & connection,    ///<  Connection to be forwarded
      const PString & forwardParty,   ///<  Remote party to forward to
      const H323SignalPDU & pdu       ///<  Full PDU initiating forwarding
    );

    /**Forward the call using the same token as the specified connection.
       Return true if the call is being redirected.

       The default behaviour will replace the current call in the endpoints
       call list using the same token as the call being redirected. Not that
       even though the same token is being used the actual object is
       completely mad anew.
      */
    virtual PBoolean ForwardConnection(
      H323Connection & connection,    ///<  Connection to be forwarded
      const PString & forwardParty,   ///<  Remote party to forward to
      const H323SignalPDU & pdu       ///<  Full PDU initiating forwarding
    );

    /**A call back function whenever a connection is established.
       This indicates that a connection to a remote endpoint was established
       with a control channel and zero or more logical channels.

       The default behaviour does nothing.
      */
    virtual void OnConnectionEstablished(
      H323Connection & connection,    ///<  Connection that was established
      const PString & token           ///<  Token for identifying connection
    );

    /**Determine if a connection is established.
      */
    virtual PBoolean IsConnectionEstablished(
      const PString & token   ///<  Token for identifying connection
    );
  //@}


  /**@name Logical Channels management */
  //@{
    /**Call back for opening a logical channel.

       The default behaviour simply returns true.
      */
    virtual PBoolean OnStartLogicalChannel(
      H323Connection & connection,    ///<  Connection for the channel
      H323Channel & channel           ///<  Channel being started
    );

    /**Call back for closed a logical channel.

       The default behaviour does nothing.
      */
    virtual void OnClosedLogicalChannel(
      H323Connection & connection,    ///<  Connection for the channel
      const H323Channel & channel     ///<  Channel being started
    );

    /**Call back from GK admission confirm to notify the 
     * Endpoint it is behind a NAT (GNUGK Gatekeeper).
     * The default does nothing. 
     * Override this to notify the user they are behind a NAT.
     */
    virtual void OnGatekeeperNATDetect(
      const PIPSocket::Address & publicAddr, ///> Public address as returned by the Gatekeeper
      H323TransportAddress & gkRouteAddress  ///> Gatekeeper Route Address
    );
  //@}

  /**@name Service Control */
  //@{
    /**Call back for HTTP based Service Control.
       An application may override this to use an HTTP based channel using a
       resource designated by the session ID. For example the session ID can
       correspond to a browser window and the 

       The default behaviour does nothing.
      */
    virtual void OnHTTPServiceControl(
      unsigned operation,  ///<  Control operation
      unsigned sessionId,  ///<  Session ID for HTTP page
      const PString & url  ///<  URL to use.
    );

    /**Call back for call credit information.
       An application may override this to display call credit information
       on registration, or when a call is started.

       The canDisplayAmountString member variable must also be set to true
       for this to operate.

       The default behaviour does nothing.
      */
    virtual void OnCallCreditServiceControl(
      const PString & amount,  ///<  UTF-8 string for amount, including currency.
      PBoolean mode          ///<  Flag indicating that calls will debit the account.
    );

    /**Handle incoming service control session information.
       Default behaviour calls session.OnChange()
     */
    virtual void OnServiceControlSession(
      unsigned type,
      unsigned sessionid,
      const H323ServiceControlSession & session,
      H323Connection * connection
    );

    /**Create the service control session object.
     */
    virtual H323ServiceControlSession * CreateServiceControlSession(
      const H225_ServiceControlDescriptor & contents
    );
  //@}

  /**@name Additional call services */
  //@{
    /** Called when an endpoint receives a SETUP PDU with a
        conference goal of "callIndependentSupplementaryService"
      
        The default behaviour is to return false, which will close the connection
     */
    virtual PBoolean OnCallIndependentSupplementaryService(
      const H323SignalPDU & setupPDU
    );

    /** Called when an endpoint receives a SETUP PDU with a
        conference goal of "capability_negotiation"
      
        The default behaviour is to return false, which will close the connection
     */
    virtual PBoolean OnNegotiateConferenceCapabilities(
      const H323SignalPDU & setupPDU
    );
  //@}

  /**@name Member variable access */
  //@{
    /**Set the default local party name for all connections on this endpoint.
      */
    virtual void SetDefaultLocalPartyName(
      const PString & name  /// Name for local party
    );

    /**Set alias names to be used for the local end of any connections. If
       an alias name already exists in the list then is is not added again.

       The list defaults to the value set in the SetDefaultLocalPartyName() function.
       Note that this will clear the alias list and the first entry will become
       the value returned by GetLocalUserName().
     */
    bool SetAliasNames(
      const PStringList & names  ///< New alias names to add
    );

    /**Add alias names to be used for the local end of any connections. If
       the alias name already exists in the list then is is not added again.

       The list defaults to the value set in the SetDefaultLocalPartyName() function.
       Note that calling SetDefaultLocalPartyName() will clear the alias list.
     */
    bool AddAliasNames(
      const PStringList & names,  ///< New alias names to add
      const PString & altGk = PString::Empty(), ///< Alternate gk for these aliases
      bool updateGk = true        ///< Indicate gatekeeper(s) to be updated
    );

    /** Remove an alias name.
        Note, you cannot remove the last alias name.
     */
    bool RemoveAliasNames(
      const PStringList & names,  ///< Alias names to remove
      bool updateGk = true        ///< Indicate gatekeeper(s) to be updated
    );

    /**Add an alias name to be used for the local end of any connections. If
       the alias name already exists in the list then is is not added again.

       The list defaults to the value set in the SetDefaultLocalPartyName() function.
       Note that calling SetDefaultLocalPartyName() will clear the alias list.
     */
    bool AddAliasName(
      const PString & name,  ///< New alias name to add
      const PString & altGk = PString::Empty(), ///< Alternate gk for this alias
      bool updateGk = true   ///< Indicate gatekeeper(s) to be updated
    );

    /** Remove an alias name.
        Note, you cannot remove the last alias name.
     */
    bool RemoveAliasName(
      const PString & name,  ///< Alias name to remove
      bool updateGk = true   ///< Indicate gatekeeper(s) to be updated
    );

    /**Get the user name to be used for the local end of any connections. This
       defaults to the logged in user as obtained from the
       PProcess::GetUserName() function.
     */
    PStringList GetAliasNames() const;

    /** Add alias patterns.
        If the pattern already exists in the list then is is not added again.
     */
    bool AddAliasNamePatterns(
      const PStringList & patterns, ///< Patterns to add
      const PString & altGk = PString::Empty(), ///< Alternate gk for these patterns
      bool updateGk = true          ///< Indicate gatekeeper(s) to be updated
    );

    /** Remove alias pattern.
    */
    bool RemoveAliasNamePatterns(
      const PStringList & patterns, ///< Patterns to remove
      bool updateGk = true          ///< Indicate gatekeeper(s) to be updated
    );

    /** Add an alias pattern.
        If the pattern already exists in the list then is is not added again.
     */
    bool AddAliasNamePattern(
      const PString & pattern,  ///< Pattern to add
      const PString & altGk = PString::Empty(), ///< Alternate gk for this pattern
      bool updateGk = true      ///< Indicate gatekeeper(s) to be updated
    );

    /** Remove an alias pattern.
    */
    bool RemoveAliasNamePattern(
      const PString & pattern,  ///< Pattern to remove
      bool updateGk = true      ///< Indicate gatekeeper(s) to be updated
    );

    /** Set all alias patterns.
        Overwrites all existing patterns.
      */
    bool SetAliasNamePatterns(
      const PStringList & patterns  ///< Patterns to set
    );

    /**Get the alias patterns.
    */
    PStringList GetAliasNamePatterns() const;

    static int ParseAliasPatternRange(const PString & pattern, PString & start, PString & end);

    /**Determine if enpoint has alias.
      */
    bool HasAlias(
      const PString & alias
    ) const;

    /**Get the default ILS server to use for user lookup.
      */
    const PString & GetDefaultILSServer() const { return m_manager.GetDefaultILSServer(); }

    /**Set the default ILS server to use for user lookup.
      */
    void SetDefaultILSServer(
      const PString & server
    ) { m_manager.SetDefaultILSServer(server); }

    /**Get the default fast start mode.
      */
    PBoolean IsFastStartDisabled() const
      { return disableFastStart; }

    /**Set the default fast start mode.
      */
    void DisableFastStart(
      PBoolean mode ///<  New default mode
    ) { disableFastStart = mode; } 

    /**Get the default H.245 tunneling mode.
      */
    PBoolean IsH245TunnelingDisabled() const
      { return disableH245Tunneling || GetProductInfo() == AvayaPhone(); }

    /**Set the default H.245 tunneling mode.
      */
    void DisableH245Tunneling(
      PBoolean mode ///<  New default mode
    ) { disableH245Tunneling = mode; } 

    /**Get the default H.245 tunneling mode.
      */
    PBoolean IsH245inSetupDisabled() const
      { return disableH245inSetup; }

    /**Set the default H.245 tunneling mode.
      */
    void DisableH245inSetup(
      PBoolean mode ///<  New default mode
    ) { disableH245inSetup = mode; } 

    /**Get the default H.245 tunneling mode.
      */
    bool IsForcedSymmetricTCS() const
      { return m_forceSymmetricTCS; }

    /**Set the default H.245 tunneling mode.
      */
    void ForceSymmetricTCS(
      bool mode ///<  New default mode
    ) { m_forceSymmetricTCS = mode; } 

    /** find out if h245 is disabled or enabled 
      * @return true if h245 is disabled 
      */
    PBoolean IsH245Disabled() const
    { return m_bH245Disabled || GetProductInfo() == AvayaPhone(); }

    /**Disable/Enable H.245, used at least for h450.7 calls
     * @param  bH245Disabled true if h245 has to be disabled 
     */
    void DisableH245(PBoolean bH245Disabled) { m_bH245Disabled = bH245Disabled; } 
    
    /**Get the flag indicating the endpoint can display an amount string.
      */
    PBoolean CanDisplayAmountString() const
      { return canDisplayAmountString; }

    /**Set the flag indicating the endpoint can display an amount string.
      */
    void SetCanDisplayAmountString(
      PBoolean mode ///<  New default mode
    ) { canDisplayAmountString = mode; } 

    /**Get the flag indicating the call will automatically clear after a time.
      */
    PBoolean CanEnforceDurationLimit() const
      { return canEnforceDurationLimit; }

    /**Set the flag indicating the call will automatically clear after a time.
      */
    void SetCanEnforceDurationLimit(
      PBoolean mode ///<  New default mode
    ) { canEnforceDurationLimit = mode; } 

#if OPAL_H450
    /**Get Call Intrusion Protection Level of the end point.
      */
    unsigned GetCallIntrusionProtectionLevel() const { return callIntrusionProtectionLevel; }

    /**Set Call Intrusion Protection Level of the end point.
      */
    void SetCallIntrusionProtectionLevel(
      unsigned level  ///<  New level from 0 to 3
    ) { PAssert(level<=3, PInvalidParameter); callIntrusionProtectionLevel = level; }
#endif

    /**Called from H.450 OnReceivedInitiateReturnError
      */
    virtual void OnReceivedInitiateReturnError();

    /**See if should automatically do call forward of connection.
     */
    PBoolean CanAutoCallForward() const { return autoCallForward; }

    /**Get the "template" capability table for this endpoint.
       This table contains all known capabilities from which specific
       capabilities for a H323Connection, local and remote, are derived.
     */
    const H323Capabilities & GetCapabilities() const { return m_capabilities; }

    /**Endpoint types.
     */
    enum TerminalTypes {
      e_SimpleEndpointType = 40,
      e_TerminalOnly = 50,
      e_TerminalAndMC = 70,
      e_GatewayOnly = 60,
      e_GatewayAndMC = 80,
      e_GatewayAndMCWithDataMP = 90,
      e_GatewayAndMCWithAudioMP = 100,
      e_GatewayAndMCWithAVMP = 110,
      e_GatekeeperOnly = 120,
      e_GatekeeperWithDataMP = 130,
      e_GatekeeperWithAudioMP = 140,
      e_GatekeeperWithAVMP = 150,
      e_MCUOnly = 160,
      e_MCUWithDataMP = 170,
      e_MCUWithAudioMP = 180,
      e_MCUWithAVMP = 190
    };

    /**Get the endpoint terminal type.
     */
    void SetTerminalType(TerminalTypes type) { terminalType = type; }

    /**Get the endpoint terminal type.
     */
    TerminalTypes GetTerminalType() const { return terminalType; }

    /**Determine if endpoint is terminal type.
     */
    PBoolean IsTerminal() const;

    /**Determine if endpoint is gateway type.
     */
    PBoolean IsGateway() const;

    /**Determine if endpoint is gatekeeper type.
     */
    PBoolean IsGatekeeper() const;

    /**Determine if endpoint is gatekeeper type.
     */
    PBoolean IsMCU() const;

    /**Get the default maximum audio jitter delay parameter.
       Defaults to 50ms
     */
    unsigned GetMinAudioJitterDelay() const { return m_manager.GetMinAudioJitterDelay(); }

    /**Get the default maximum audio delay jitter parameter.
       Defaults to 250ms.
     */
    unsigned GetMaxAudioJitterDelay() const { return m_manager.GetMaxAudioJitterDelay(); }

    /**Set the maximum audio delay jitter parameter.
     */
    void SetAudioJitterDelay(
      unsigned minDelay,   ///<  New minimum jitter buffer delay in milliseconds
      unsigned maxDelay    ///<  New maximum jitter buffer delay in milliseconds
    ) { m_manager.SetAudioJitterDelay(minDelay, maxDelay); }

#if OPAL_H239
    /**Get the default H.239 control capability.
     */
    bool GetDefaultH239Control() const { return m_defaultH239Control; }

    /**Set the default H.239 control capability.
     */
    void SetDefaultH239Control(
      bool on   ///< H.239 control capability is to be sent to remote
    ) { m_defaultH239Control = on; }
#endif

#if OPAL_H460
    /** Is the FeatureSet disabled
      */
    bool H460Disabled() const { return m_disableH460; }

    /** Disable all FeatureSets. Use this for pre H323v4 interoperability
      */
    void DisableH460(bool disable = true) { m_disableH460 = disable; }

    H460_FeatureSet * GetFeatures() const { return m_features; }

    /** Get the Endpoint FeatureSet
        This creates a new instance of the featureSet
      */
    virtual H460_FeatureSet * CreateFeatureSet(
      H323Connection * connection
    );
    virtual H460_FeatureSet * InternalCreateFeatureSet(
      H323Connection * connection
    );

    /**Called when an outgoing PDU requires a feature set
     */
    virtual PBoolean OnSendFeatureSet(H460_MessageType pduType, H225_FeatureSet &);

    /**Called when an incoming PDU contains a feature set
     */
    virtual void OnReceiveFeatureSet(H460_MessageType pduType, const H225_FeatureSet &);
    
    /**Callback when creating Feature Instance. This can be used to disable features on
       a case by case basis by returning FALSE
       Default returns TRUE
      */
    virtual bool OnLoadFeature(
      H460_Feature & feature
    );

#endif // OPAL_H460

#if OPAL_H460_NAT
    H46019Server * GetH46019Server() const { return m_H46019Server; }
#endif

    /**Determine if the address is "local", ie does not need STUN
     */
    virtual PBoolean IsLocalAddress(
      const PIPSocket::Address & remoteAddress
    ) const { return m_manager.IsLocalAddress(remoteAddress); }

    /**Provide TCP address translation hook
     */
    virtual void TranslateTCPAddress(
      PIPSocket::Address & localAddr,
      const PIPSocket::Address & remoteAddr
    );

    /**Get the default timeout for calling another endpoint.
     */
    const PTimeInterval & GetSignallingChannelCallTimeout() const { return signallingChannelCallTimeout; }

    /**Get the default timeout for first signalling PDU on a connection
     */
    const PTimeInterval & GetFirstSignalPduTimeout() const { return firstSignalPduTimeout; }

    /**Get the default timeout for waiting on an end session.
     */
    const PTimeInterval & GetEndSessionTimeout() const { return endSessionTimeout; }

    /**Get the default timeout for master slave negotiations.
     */
    const PTimeInterval & GetMasterSlaveDeterminationTimeout() const { return masterSlaveDeterminationTimeout; }

    /**Get the default retries for H245 master slave negotiations.
     */
    unsigned GetMasterSlaveDeterminationRetries() const { return masterSlaveDeterminationRetries; }

    /**Get the default timeout for H245 capability exchange negotiations.
     */
    const PTimeInterval & GetCapabilityExchangeTimeout() const { return capabilityExchangeTimeout; }

    /**Get the default timeout for H245 logical channel negotiations.
     */
    const PTimeInterval & GetLogicalChannelTimeout() const { return logicalChannelTimeout; }

    /**Get the default timeout for H245 request mode negotiations.
     */
    const PTimeInterval & GetRequestModeTimeout() const { return logicalChannelTimeout; }

    /**Get the default timeout for H245 round trip delay negotiations.
     */
    const PTimeInterval & GetRoundTripDelayTimeout() const { return roundTripDelayTimeout; }

    /**Get the default rate H245 round trip delay is calculated by connection.
     */
    const PTimeInterval & GetRoundTripDelayRate() const { return roundTripDelayRate; }

    /**Get the flag for clearing a call if the round trip delay calculation fails.
     */
    PBoolean ShouldClearCallOnRoundTripFail() const { return clearCallOnRoundTripFail; }

    /**Get the amount of time with no media that should cause call to clear
     */
    const PTimeInterval & GetNoMediaTimeout() const { return m_manager.GetNoMediaTimeout(); }

    /**Set the amount of time with no media that should cause call to clear
     */
    void SetNoMediaTimeout(
      const PTimeInterval & newInterval  ///<  New timeout for media
    ) { m_manager.SetNoMediaTimeout(newInterval); }

    /**Get the default timeout for GatekeeperRequest and Gatekeeper discovery.
     */
    const PTimeInterval & GetGatekeeperRequestTimeout() const { return gatekeeperRequestTimeout; }

    /**Get the default retries for GatekeeperRequest and Gatekeeper discovery.
     */
    unsigned GetGatekeeperRequestRetries() const { return gatekeeperRequestRetries; }

    /**Get the default timeout for RAS protocol transactions.
     */
    const PTimeInterval & GetRasRequestTimeout() const { return rasRequestTimeout; }

    /**Get the default retries for RAS protocol transations.
     */
    unsigned GetRasRequestRetries() const { return rasRequestRetries; }

    /**Get the default time for gatekeeper to reregister.
       A value of zero disables the keep alive facility.
     */
    const PTimeInterval & GetGatekeeperTimeToLive() const { return registrationTimeToLive; }

    /**Set the default time for gatekeeper to reregister.
       A value of zero disables the keep alive facility.
     */
    void SetGatekeeperTimeToLive(const PTimeInterval & ttl) { registrationTimeToLive = ttl; }

    /**Get the default time for gatekeeper registration delay.
       This is used to slow down registrations when many aliases are used, and the
       gatekeeper does not support multiple aliases in on GRQ/RRQ.
     */
    const PTimeInterval & GetGatekeeperStartDelay() const { return m_gatekeeperStartDelay; }

    /**Set the default time for gatekeeper registration delay.
       This is used to slow down registrations when many aliases are used, and the
       gatekeeper does not support multiple aliases in on GRQ/RRQ.
     */
    void SetGatekeeperStartDelay(const PTimeInterval & gatekeeperStartDelay) { m_gatekeeperStartDelay = gatekeeperStartDelay; }

    /**Get the iNow Gatekeeper Access Token OID.
     */
    const PString & GetGkAccessTokenOID() const { return gkAccessTokenOID; }

    /**Set the iNow Gatekeeper Access Token OID.
     */
    void SetGkAccessTokenOID(const PString & token) { gkAccessTokenOID = token; }

    /**Get flag to indicate whether to send GRQ on gatekeeper registration
     */
    bool GetSendGRQ() const { return m_sendGRQ; }

    /**Sent flag to indicate whether to send GRQ on gatekeeper registration
     */
    void SetSendGRQ(bool v) { m_sendGRQ = v; }

    /**Get flag to indicate whether to only use one signal address in gatekeeper registration
     */
    bool GetOneSignalAddressInRRQ() const { return m_oneSignalAddressInRRQ; }

    /**Sent flag to indicate whether to only use one signal address in gatekeeper registration
     */
    void SetOneSignalAddressInRRQ(bool v) { m_oneSignalAddressInRRQ = v; }

    /**Get the default timeout for Call Transfer Timer CT-T1.
     */
    const PTimeInterval & GetCallTransferT1() const { return callTransferT1; }

    /**Get the default timeout for Call Transfer Timer CT-T2.
     */
    const PTimeInterval & GetCallTransferT2() const { return callTransferT2; }

    /**Get the default timeout for Call Transfer Timer CT-T3.
     */
    const PTimeInterval & GetCallTransferT3() const { return callTransferT3; }

    /**Get the default timeout for Call Transfer Timer CT-T4.
     */
    const PTimeInterval & GetCallTransferT4() const { return callTransferT4; }

    /** Get Call Intrusion timers timeout */
    const PTimeInterval & GetCallIntrusionT1() const { return callIntrusionT1; }
    const PTimeInterval & GetCallIntrusionT2() const { return callIntrusionT2; }
    const PTimeInterval & GetCallIntrusionT3() const { return callIntrusionT3; }
    const PTimeInterval & GetCallIntrusionT4() const { return callIntrusionT4; }
    const PTimeInterval & GetCallIntrusionT5() const { return callIntrusionT5; }
    const PTimeInterval & GetCallIntrusionT6() const { return callIntrusionT6; }

#if OPAL_H450
    /**Get the dictionary of <callIdentities, connections>
     */
    H323CallIdentityDict& GetCallIdentityDictionary() { return m_secondaryConnectionsActive; }

    /**Get the next available invoke Id for H450 operations
      */
    unsigned GetNextH450CallIdentityValue() const { return ++m_nextH450CallIdentity; }
#endif //OPAL_H450

    /**Get the default transports for the endpoint type.
       Overrides the default behaviour to return udp and tcp.
      */
    virtual PString GetDefaultTransport() const;

    /**Get the default signal port for this endpoint.
     */
    virtual WORD GetDefaultSignalPort() const;

    /// Gets the current regular expression for the compatibility issue
    PString GetCompatibility(
      H323Connection::CompatibilityIssues issue    ///< Issue being worked around
    ) const;

    /** Indicate a compatibility issue with remote endpoint product type.
        Note the regular expression is always "extended".

        @returns true of regular expression is legal.
      */
    bool SetCompatibility(
      H323Connection::CompatibilityIssues issue,    ///< Issue being worked around
      const PString & regex         ///< Regular expression for endpoint matching, see OpalProductInfo::AsString()
    );

    /** Indicate a compatibility issue with remote endpoint product type.
        This will add to the regular expression for the issue a new value to
        try and match. It effectively adds "|" + regex to the existing regular
        expression.

        Note the regular expression is always "extended".
      */
    bool AddCompatibility(
      H323Connection::CompatibilityIssues issue,    ///< Issue being worked around
      const PString & regex         ///< Regular expression for endpoint matching, see OpalProductInfo::AsString()
    );

    /// Determine if we must compensate for remote endpoint.
    bool HasCompatibilityIssue(
      H323Connection::CompatibilityIssues issue,            ///< Issue being worked around
      const OpalProductInfo & productInfo   ///< Product into for check for issue
    ) const;
  //@}

    // For backward compatibility
    void SetLocalUserName(const PString & name) { return SetDefaultLocalPartyName(name); }
    const PString & GetLocalUserName() const { return GetDefaultLocalPartyName(); }

    PTimeInterval InternalGetGatekeeperStartDelay();

  protected:
    bool InternalStartGatekeeper(const H323TransportAddress & remoteAddress, const PString & localAddress);
    bool InternalRestartGatekeeper(bool adjustingRegistrations = true);
    bool InternalCreateGatekeeper(const H323TransportAddress & remoteAddress, const PStringList & aliases);
    void InternalSetGatekeeperPassword(H323Gatekeeper& gatekeeper, const OpalTransportAddress& gatekeeperAddress) const;

    H323Connection * InternalMakeCall(
      OpalCall & call,
      const PString & existingToken,           ///<  Existing connection to be transferred
      const PString & callIdentity,            ///<  Call identity of the secondary call (if it exists)
      unsigned capabilityLevel,                ///<  Intrusion capability level
      const PString & remoteParty,             ///<  Remote party to call
      void * userData,                         ///<  user data to pass to CreateConnection
      unsigned int options = 0,                ///<  options to pass to connection
      OpalConnection::StringOptions * stringOptions = NULL ///<  complex string options
    );

    // Configuration variables, commonly changed
    typedef map<PString, OpalTransportAddress> AliasToGkMap;
    AliasToGkMap    m_localAliasNames;
    AliasToGkMap    m_localAliasPatterns;
    PDECLARE_MUTEX( m_aliasMutex);
    PBoolean        autoCallForward;
    PBoolean        disableFastStart;
    PBoolean        disableH245Tunneling;
    PBoolean        disableH245inSetup;
    bool            m_forceSymmetricTCS;
    PBoolean        m_bH245Disabled; /* enabled or disabled h245 */
    PBoolean        canDisplayAmountString;
    PBoolean        canEnforceDurationLimit;
#if OPAL_H450
    unsigned        callIntrusionProtectionLevel;
#endif

    TerminalTypes   terminalType;

#if OPAL_H239
    bool            m_defaultH239Control;
#endif

    PBoolean        clearCallOnRoundTripFail;

    // Some more configuration variables, rarely changed.
    PTimeInterval signallingChannelCallTimeout;
    PTimeInterval firstSignalPduTimeout;
    PTimeInterval endSessionTimeout;
    PTimeInterval masterSlaveDeterminationTimeout;
    unsigned      masterSlaveDeterminationRetries;
    PTimeInterval capabilityExchangeTimeout;
    PTimeInterval logicalChannelTimeout;
    PTimeInterval requestModeTimeout;
    PTimeInterval roundTripDelayTimeout;
    PTimeInterval roundTripDelayRate;
    PTimeInterval gatekeeperRequestTimeout;
    unsigned      gatekeeperRequestRetries;
    PTimeInterval rasRequestTimeout;
    unsigned      rasRequestRetries;
    PTimeInterval registrationTimeToLive;
    PTimeInterval m_gatekeeperStartDelay;

    PString       gkAccessTokenOID;
    bool          m_sendGRQ;
    bool          m_oneSignalAddressInRRQ;
    PTime         m_nextGatekeeperDiscovery;
    PDECLARE_MUTEX(m_delayGatekeeperMutex);

    /* Protect against absence of a response to the ctIdentify reqest
       (Transferring Endpoint - Call Transfer with a secondary Call) */
    PTimeInterval callTransferT1;
    /* Protect against failure of completion of the call transfer operation
       involving a secondary Call (Transferred-to Endpoint) */
    PTimeInterval callTransferT2;
    /* Protect against failure of the Transferred Endpoint not responding
       within sufficient time to the ctInitiate APDU (Transferring Endpoint) */
    PTimeInterval callTransferT3;
    /* May optionally operate - protects against absence of a response to the
       ctSetup request (Transferred Endpoint) */
    PTimeInterval callTransferT4;

    /** Call Intrusion Timers */
    PTimeInterval callIntrusionT1;
    PTimeInterval callIntrusionT2;
    PTimeInterval callIntrusionT3;
    PTimeInterval callIntrusionT4;
    PTimeInterval callIntrusionT5;
    PTimeInterval callIntrusionT6;

    // Dynamic variables
    PSafeDictionary<PString, H323Connection> m_connectionsByCallId;
    std::set<OpalTransportPtr> m_reusableTransports;
    PDECLARE_MUTEX(            m_reusableTransportMutex);

    H323Capabilities m_capabilities;

    typedef PDictionary<PString, H323Gatekeeper> GatekeeperByAlias;

    GatekeeperList            m_gatekeepers;
    GatekeeperByAlias         m_gatekeeperByAlias;
    OpalTransportAddressArray m_gatekeeperInterfaces;
    PString                   m_gatekeeperUsername;
    PString                   m_gatekeeperPassword;
    PINDEX                    m_gatekeeperAliasLimit;
    bool                      m_gatekeeperSimulatePattern;
    bool                      m_gatekeeperRasRedirect;
    PDECLARE_MUTEX(           m_gatekeeperMutex);
    PStringToString           m_aliasPasswords;
    PString                   m_aliasPwdDefaultAddress;

#if OPAL_H450
    H323CallIdentityDict   m_secondaryConnectionsActive;
    mutable atomic<unsigned> m_nextH450CallIdentity;
            /// Next available callIdentity for H450 Transfer operations via consultation.
#endif

#if OPAL_H460
    bool              m_disableH460;
    H460_FeatureSet * m_features;
#endif
#if OPAL_H460_NAT
    H46019Server * m_H46019Server;
#endif

    typedef map<H323Connection::CompatibilityIssues, PRegularExpression> CompatibilityEndpoints;
    CompatibilityEndpoints m_compatibility;

  private:
    P_REMOVE_VIRTUAL_VOID(OnConnectionCleared(H323Connection &, const PString &));
    P_REMOVE_VIRTUAL_VOID(OnRTPStatistics(const H323Connection &, const OpalRTPSession &) const);
    P_REMOVE_VIRTUAL(PBoolean, OnConferenceInvite(const H323SignalPDU &), false);
    P_REMOVE_VIRTUAL_VOID(OnGatekeeperConfirm());
    P_REMOVE_VIRTUAL_VOID(OnGatekeeperReject());
    P_REMOVE_VIRTUAL_VOID(OnRegistrationConfirm());
    P_REMOVE_VIRTUAL_VOID(OnRegistrationReject());
    P_REMOVE_VIRTUAL_VOID(OnGatekeeperNATDetect(PIPSocket::Address, PString &, H323TransportAddress &));
};

#endif // OPAL_H323

#endif // OPAL_H323_H323EP_H


/////////////////////////////////////////////////////////////////////////////
