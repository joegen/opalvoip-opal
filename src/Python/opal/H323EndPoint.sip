/*
 * Python bindings.
 *
 * Open Phone Abstraction Library (OPAL)
 *
 * Copyright (c) 2011 Demetrius Cassidy
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
 * The Original Code is Open Phone Abstraction Library (OPAL)
 *
 * The Initial Developer of the Original Code is Demetrius Cassidy
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision$
 * $Author$
 * $Date$
 */

%ModuleHeaderCode
#include <ptlib.h>
#include <h323/h323ep.h>
%End


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
class H323EndPoint : OpalRTPEndPoint
{
  public:
    enum {
      DefaultTcpSignalPort = 1720
    };

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
       PFalse is returned.

       This function usually returns almost immediately with the connection
       continuing to occur in a new background thread.

       If PFalse is returned then the connection could not be established. For
       example if a PSTN endpoint is used and the assiciated line is engaged
       then it may return immediately. Returning a non-NULL value does not
       mean that the connection will succeed, only that an attempt is being
       made.

       The default behaviour is pure.
     */
  /*  virtual PSafePtr<OpalConnection> MakeConnection(
      OpalCall & call,                  ///<  Owner of connection
      const PString & party,            ///<  Remote party to call
      void * userData  = NULL,          ///<  Arbitrary data to pass to connection
      unsigned int options = NULL,      ///<  options to pass to conneciton
      OpalConnection::StringOptions * stringOptions = NULL
    );*/
  //@}

 
    /**Set the gateway prefixes 
       Override this to set the acceptable prefixes to the gatekeeper
      */
    virtual bool OnSetGatewayPrefixes(
      PStringList & prefixes
    ) const;
  //@}


  /**@name Capabilities */
  //@{
    /**Add a codec to the capabilities table. This will assure that the
       assignedCapabilityNumber field in the codec is unique for all codecs
       installed on this endpoint.

       If the specific instnace of the capability is already in the table, it
       is not added again. Ther can be multiple instances of the same
       capability class however.
     */
    void AddCapability(
      H323Capability * capability   ///<  New codec specification
    );

    /**Set the capability descriptor lists. This is three tier set of
       codecs. The top most level is a list of particular capabilities. Each
       of these consists of a list of alternatives that can operate
       simultaneously. The lowest level is a list of codecs that cannot
       operate together. See H323 section 6.2.8.1 and H245 section 7.2 for
       details.

       If descriptorNum is P_MAX_INDEX, the the next available index in the
       array of descriptors is used. Similarly if simultaneous is P_MAX_INDEX
       the the next available SimultaneousCapabilitySet is used. The return
       value is the index used for the new entry. Note if both are P_MAX_INDEX
       then the return value is the descriptor index as the simultaneous index
       must be zero.

       Note that the capability specified here is automatically added to the
       capability table using the AddCapability() function. A specific
       instance of a capability is only ever added once, so multiple
       SetCapability() calls with the same H323Capability pointer will only
       add that capability once.
     */
    PINDEX SetCapability(
      PINDEX descriptorNum, ///<  The member of the capabilityDescriptor to add
      PINDEX simultaneous,  ///<  The member of the SimultaneousCapabilitySet to add
      H323Capability * cap  ///<  New capability specification
    );

    /**Add all matching capabilities in list.
       All capabilities that match the specified name are added. See the
       capabilities code for details on the matching algorithm.
      */
    PINDEX AddAllCapabilities(
      PINDEX descriptorNum, ///<  The member of the capabilityDescriptor to add
      PINDEX simultaneous,  ///<  The member of the SimultaneousCapabilitySet to add
      const PString & name  ///<  New capabilities name, if using "known" one.
    );

    /**Add all user input capabilities to this endpoints capability table.
      */
    void AddAllUserInputCapabilities(
      PINDEX descriptorNum, ///<  The member of the capabilityDescriptor to add
      PINDEX simultaneous   ///<  The member of the SimultaneousCapabilitySet to add
    );

    /**Remove capabilites in table.
      */
    void RemoveCapabilities(
      const PStringArray & codecNames
    );

    /**Reorder capabilites in table.
      */
    void ReorderCapabilities(
      const PStringArray & preferenceOrder
    );

      /**Find a capability that has been registered.
     */
    H323Capability * FindCapability(
      H323Capability::MainTypes mainType,   ///<  Main type of codec
      unsigned subType                      ///<  Subtype of codec
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
    PBoolean UseGatekeeper(
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
    PBoolean SetGatekeeper(
      const PString & address,          ///<  Address of gatekeeper to use.
      H323Transport * transport = NULL  ///<  Transport over which to talk to gatekeeper.
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
    PBoolean SetGatekeeperZone(
      const PString & address,          ///<  Address of gatekeeper to use.
      const PString & identifier,       ///<  Identifier of gatekeeper to use.
      H323Transport * transport = NULL  ///<  Transport over which to talk to gatekeeper.
    );

    /**Locate and select gatekeeper.
       This function will use the automatic gatekeeper discovery methods to
       locate the gatekeeper on the particular transport that has the specified
       gatekeeper identifier name. This is often the "Zone" for the gatekeeper.

       Note the transport being passed in will be deleted becomes owned by the
       H323Gatekeeper created by this function and will be deleted by it. Also
       if transport is NULL then a H323TransportUDP is created.
     */
    PBoolean LocateGatekeeper(
      const PString & identifier,       ///<  Identifier of gatekeeper to locate.
      H323Transport * transport = NULL  ///<  Transport over which to talk to gatekeeper.
    );

    /**Discover and select gatekeeper.
       This function will use the automatic gatekeeper discovery methods to
       locate the first gatekeeper on a particular transport.

       Note the transport being passed in will be deleted becomes owned by the
       H323Gatekeeper created by this function and will be deleted by it. Also
       if transport is NULL then a H323TransportUDP is created.
     */
    PBoolean DiscoverGatekeeper(
      H323Transport * transport = NULL  ///<  Transport over which to talk to gatekeeper.
    );

    /**Create a gatekeeper.
       This allows the application writer to have the gatekeeper as a
       descendent of the H323Gatekeeper in order to add functionality to the
       base capabilities in the library.

       The default creates an instance of the H323Gatekeeper class.
     */
    virtual H323Gatekeeper * CreateGatekeeper(
      H323Transport * transport  ///<  Transport over which gatekeepers communicates.
    ) /Factory/;

    /**Get the gatekeeper we are registered with.
     */
    H323Gatekeeper * GetGatekeeper() const;

    /**Return if endpoint is registered with gatekeeper.
      */
    PBoolean IsRegisteredWithGatekeeper() const;

    /**Unregister and delete the gatekeeper we are registered with.
       The return value indicates PFalse if there was an error during the
       unregistration. However the gatekeeper is still removed and its
       instance deleted regardless of this error.
     */
    PBoolean RemoveGatekeeper(
      int reason = -1    ///<  Reason for gatekeeper removal
    );

    /**Set the H.235 password for the gatekeeper.
      */
    virtual void SetGatekeeperPassword(
      const PString & password,
      const PString & username = PString::Empty()
    );

    /**Get the H.235 username for the gatekeeper.
      */
    virtual const PString & GetGatekeeperUsername() const;

    /**Get the H.235 password for the gatekeeper.
      */
    virtual const PString & GetGatekeeperPassword() const;

    /**Create a list of authenticators for gatekeeper.
      */
    /*virtual H235Authenticators CreateAuthenticators();*/

    /**Called when the gatekeeper sends a GatekeeperConfirm
      */
    virtual void  OnGatekeeperConfirm();

    /**Called when the gatekeeper sends a GatekeeperReject
      */
    virtual void  OnGatekeeperReject();

    /**Called when the gatekeeper sends a RegistrationConfirm
      */
    virtual void OnRegistrationConfirm();

    /**Called when the gatekeeper sends a RegistrationReject
      */
    virtual void  OnRegistrationReject();
  //@}

  /**@name Connection management */
  //@{
    /**Handle new incoming connetion from listener.
      */
    virtual PBoolean NewIncomingConnection(
      OpalTransport * transport  ///<  Transport connection came in on
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

    /**Place the call on hold, suspending all media channels (H.450.4)
    * NOTE: Only Local Hold is implemented so far. 
    */
    void HoldCall(
      const PString & token,        ///<  Existing connection to be transferred
      PBoolean localHold   ///<  true for Local Hold, false for Remote Hold
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

    ///**Find a connection that uses the specified token.
    //   This searches the endpoint for the connection that contains the token
    //   as provided by functions such as MakeCall(). if not found it will then search for
    //   the string representation of the CallIdentifier for the connection, and
    //   finally try for the string representation of the ConferenceIdentifier.

    //   Note the caller of this function MUSt call the H323Connection::Unlock()
    //   function if this function returns a non-NULL pointer. If it does not
    //   then a deadlock can occur.
    //  */
    //PSafePtr<H323Connection> FindConnectionWithLock(
    //  const PString & token,     ///<  Token to identify connection
    //  PSafetyMode mode = PSafeReadWrite
    //);

    /**Handle a connection transfer.
       This gives the application an opportunity to abort the transfer.
       The default behaviour just returns PTrue.
      */
    virtual PBoolean OnCallTransferInitiate(
      H323Connection & connection,    ///<  Connection to transfer
      const PString & remoteParty     ///<  Party transferring to.
    );

    /**Handle a transfer via consultation.
       This gives the transferred-to user an opportunity to abort the transfer.
       The default behaviour just returns PTrue.
      */
    virtual PBoolean OnCallTransferIdentify(
      H323Connection & connection    ///<  Connection to transfer
    );

    virtual OpalConnection::AnswerCallResponse OnAnswerCall(
       OpalConnection & connection,
       const PString & caller
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


    /**Call back from GK admission confirm to notify the 
     * Endpoint it is behind a NAT (GNUGK Gatekeeper).
     * The default does nothing. 
     * Override this to notify the user they are behind a NAT.
     */
    virtual void OnGatekeeperNATDetect(
      PIPSocket::Address publicAddr,         ///> Public address as returned by the Gatekeeper
      PString & gkIdentifier,                ///> Identifier at the gatekeeper
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

       The canDisplayAmountString member variable must also be set to PTrue
       for this to operate.

       The default behaviour does nothing.
      */
    virtual void OnCallCreditServiceControl(
      const PString & amount,  ///<  UTF-8 string for amount, including currency.
      PBoolean mode          ///<  Flag indicating that calls will debit the account.
    );

  /**@name Member variable access */
  //@{
    /**Set the default local party name for all connections on this endpoint.
      */
    virtual void SetDefaultLocalPartyName(
      const PString & name  /// Name for local party
    );

    /**Set the user name to be used for the local end of any connections. This
       defaults to the logged in user as obtained from the
       PProcess::GetUserName() function.

       Note that this name is technically the first alias for the endpoint.
       Additional aliases may be added by the use of the AddAliasName()
       function, however that list will be cleared when this function is used.
     */
    virtual void SetLocalUserName(
      const PString & name  ///<  Local name of endpoint (prime alias)
    );

    /**Get the user name to be used for the local end of any connections. This
       defaults to the logged in user as obtained from the
       PProcess::GetUserName() function.
     */
    virtual const PString & GetLocalUserName() const;

    /**Add an alias name to be used for the local end of any connections. If
       the alias name already exists in the list then is is not added again.

       The list defaults to the value set in the SetLocalUserName() function.
       Note that calling SetLocalUserName() will clear the alias list.
     */
    PBoolean AddAliasName(
      const PString & name  ///<  New alias name to add
    );

    /**Remove an alias name used for the local end of any connections. 
       defaults to an empty list.
     */
    PBoolean RemoveAliasName(
      const PString & name  ///<  New alias namer to add
    );

    /**Get the user name to be used for the local end of any connections. This
       defaults to the logged in user as obtained from the
       PProcess::GetUserName() function.
     */
    const PStringList & GetAliasNames() const;

    /**Get the alias patterns, might be used in terminalAliasPattern.
     */
    const PStringList & GetAliasNamePatterns() const;

    /**Add an alias name pattern to localAliasPatterns. If
       the pattern already exists in the list then is is not added again.
     */
    PBoolean AddAliasNamePattern(
      const PString & pattern  
    );

    /**Get the default ILS server to use for user lookup.
      */
    const PString & GetDefaultILSServer() const;

    /**Set the default ILS server to use for user lookup.
      */
    void SetDefaultILSServer(
      const PString & server
    );

    /**Get the default fast start mode.
      */
    PBoolean IsFastStartDisabled() const;

    /**Set the default fast start mode.
      */
    void DisableFastStart(
      PBoolean mode ///<  New default mode
    ); 

    /**Get the default H.245 tunneling mode.
      */
    PBoolean IsH245TunnelingDisabled() const;

    /**Set the default H.245 tunneling mode.
      */
    void DisableH245Tunneling(
      PBoolean mode ///<  New default mode
    ); 

    /**Get the default H.245 tunneling mode.
      */
    PBoolean IsH245inSetupDisabled() const;

    /**Set the default H.245 tunneling mode.
      */
    void DisableH245inSetup(
      PBoolean mode ///<  New default mode
    ); 

    /** find out if h245 is disabled or enabled 
      * @return PTrue if h245 is disabled 
      */
    PBoolean IsH245Disabled() const;

    /**Disable/Enable H.245, used at least for h450.7 calls
     * @param  bH245Disabled PTrue if h245 has to be disabled 
     */
    void DisableH245(PBoolean bH245Disabled); 
    
    /**Get the flag indicating the endpoint can display an amount string.
      */
    PBoolean CanDisplayAmountString() const;

    /**Set the flag indicating the endpoint can display an amount string.
      */
    void SetCanDisplayAmountString(
      PBoolean mode ///<  New default mode
    ); 

    /**Get the flag indicating the call will automatically clear after a time.
      */
    PBoolean CanEnforceDurationLimit() const;

    /**Set the flag indicating the call will automatically clear after a time.
      */
    void SetCanEnforceDurationLimit(
      PBoolean mode ///<  New default mode
    ); 

    /**Get Call Intrusion Protection Level of the end point.
      */
    unsigned GetCallIntrusionProtectionLevel() const;

    /**Set Call Intrusion Protection Level of the end point.
      */
    void SetCallIntrusionProtectionLevel(
      unsigned level  ///<  New level from 0 to 3
    );

    /**Called from H.450 OnReceivedInitiateReturnError
      */
    virtual void OnReceivedInitiateReturnError();

    /**See if should automatically do call forward of connection.
     */
    PBoolean CanAutoCallForward() const;

    /**Get the current capability table for this endpoint.
     */
    /*const H323Capabilities & GetCapabilities() const;*/

    /**Endpoint types.
     */
    enum TerminalTypes {
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
	void SetTerminalType(TerminalTypes type);

    /**Get the endpoint terminal type.
     */
    TerminalTypes GetTerminalType() const;

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
    unsigned GetMinAudioJitterDelay() const;

    /**Get the default maximum audio delay jitter parameter.
       Defaults to 250ms.
     */
    unsigned GetMaxAudioJitterDelay() const;

    /**Set the maximum audio delay jitter parameter.
     */
    void SetAudioJitterDelay(
      unsigned minDelay,   ///<  New minimum jitter buffer delay in milliseconds
      unsigned maxDelay    ///<  New maximum jitter buffer delay in milliseconds
    );

    /**Get the initial bandwidth parameter.
     */
    unsigned GetInitialBandwidth() const;

    /**Get the initial bandwidth parameter.
     */
    void SetInitialBandwidth(unsigned bandwidth);

    /**Get the default H.239 control capability.
     */
    bool GetDefaultH239Control() const;

    /**Set the default H.239 control capability.
     */
    void SetDefaultH239Control(
      bool on   ///< H.239 control capability is to be sent to remote
    );
     
    /**Determine if the address is "local", ie does not need STUN
     */
    virtual PBoolean IsLocalAddress(
      const PIPSocket::Address & remoteAddress
    ) const;

    /**Provide TCP address translation hook
     */
    virtual void TranslateTCPAddress(
      PIPSocket::Address & localAddr,
      const PIPSocket::Address & remoteAddr
    );

    /**Get the TCP port number base for H.245 channels
     */
    WORD GetTCPPortBase() const;

    /**Get the TCP port number base for H.245 channels.
     */
    WORD GetTCPPortMax() const;

    /**Set the TCP port number base and max for H.245 channels.
     */
    void SetTCPPorts(unsigned tcpBase, unsigned tcpMax);

    /**Get the next TCP port number for H.245 channels
     */
    WORD GetNextTCPPort();

    /**Get the UDP port number base for RAS channels
     */
    WORD GetUDPPortBase() const;

    /**Get the UDP port number base for RAS channels.
     */
    WORD GetUDPPortMax() const;

    /**Set the TCP port number base and max for H.245 channels.
     */
    void SetUDPPorts(unsigned udpBase, unsigned udpMax);

    /**Get the next UDP port number for RAS channels
     */
    WORD GetNextUDPPort();

    /**Get the UDP port number base for RTP channels.
     */
    WORD GetRtpIpPortBase() const;

    /**Get the max UDP port number for RTP channels.
     */
    WORD GetRtpIpPortMax() const;

    /**Set the UDP port number base and max for RTP channels.
     */
    void SetRtpIpPorts(unsigned udpBase, unsigned udpMax);

    /**Get the UDP port number pair for RTP channels.
     */
    WORD GetRtpIpPortPair();

    /**Get the default timeout for calling another endpoint.
     */
    const PTimeInterval & GetSignallingChannelCallTimeout() const;

    /**Get the default timeout for incoming H.245 connection.
     */
    const PTimeInterval & GetControlChannelStartTimeout() const;

    /**Get the default timeout for waiting on an end session.
     */
    const PTimeInterval & GetEndSessionTimeout() const;

    /**Get the default timeout for master slave negotiations.
     */
    const PTimeInterval & GetMasterSlaveDeterminationTimeout() const;

    /**Get the default retries for H245 master slave negotiations.
     */
    unsigned GetMasterSlaveDeterminationRetries() const;

    /**Get the default timeout for H245 capability exchange negotiations.
     */
    const PTimeInterval & GetCapabilityExchangeTimeout() const;

    /**Get the default timeout for H245 logical channel negotiations.
     */
    const PTimeInterval & GetLogicalChannelTimeout() const;

    /**Get the default timeout for H245 request mode negotiations.
     */
    const PTimeInterval & GetRequestModeTimeout() const;

    /**Get the default timeout for H245 round trip delay negotiations.
     */
    const PTimeInterval & GetRoundTripDelayTimeout() const;

    /**Get the default rate H245 round trip delay is calculated by connection.
     */
    const PTimeInterval & GetRoundTripDelayRate() const;

    /**Get the flag for clearing a call if the round trip delay calculation fails.
     */
    PBoolean ShouldClearCallOnRoundTripFail() const;

    /**Get the amount of time with no media that should cause call to clear
     */
    const PTimeInterval & GetNoMediaTimeout() const;

    /**Set the amount of time with no media that should cause call to clear
     */
    PBoolean SetNoMediaTimeout(
      const PTimeInterval & newInterval  ///<  New timeout for media
    );

    /**Get the default timeout for GatekeeperRequest and Gatekeeper discovery.
     */
    const PTimeInterval & GetGatekeeperRequestTimeout() const;

    /**Get the default retries for GatekeeperRequest and Gatekeeper discovery.
     */
    unsigned GetGatekeeperRequestRetries() const;

    /**Get the default timeout for RAS protocol transactions.
     */
    const PTimeInterval & GetRasRequestTimeout() const;

    /**Get the default retries for RAS protocol transations.
     */
    unsigned GetRasRequestRetries() const;

    /**Get the default time for gatekeeper to reregister.
       A value of zero disables the keep alive facility.
     */
    const PTimeInterval & GetGatekeeperTimeToLive() const;

    /**Set the default time for gatekeeper to reregister.
       A value of zero disables the keep alive facility.
     */
    void SetGatekeeperTimeToLive(const PTimeInterval & ttl);

    /**Get the iNow Gatekeeper Access Token OID.
     */
    const PString & GetGkAccessTokenOID() const;

    /**Set the iNow Gatekeeper Access Token OID.
     */
    void SetGkAccessTokenOID(const PString & token);

    /**Get flag to indicate whether to send GRQ on gatekeeper registration
     */
    PBoolean GetSendGRQ() const;

    /**Sent flag to indicate whether to send GRQ on gatekeeper registration
     */
    void SetSendGRQ(PBoolean v);

    /**Get the default timeout for Call Transfer Timer CT-T1.
     */
    const PTimeInterval & GetCallTransferT1() const;

    /**Get the default timeout for Call Transfer Timer CT-T2.
     */
    const PTimeInterval & GetCallTransferT2() const;

    /**Get the default timeout for Call Transfer Timer CT-T3.
     */
    const PTimeInterval & GetCallTransferT3() const;

    /**Get the default timeout for Call Transfer Timer CT-T4.
     */
    const PTimeInterval & GetCallTransferT4() const;

    /** Get Call Intrusion timers timeout */
    const PTimeInterval & GetCallIntrusionT1() const;
    const PTimeInterval & GetCallIntrusionT2() const;
    const PTimeInterval & GetCallIntrusionT3() const;
    const PTimeInterval & GetCallIntrusionT4() const;
    const PTimeInterval & GetCallIntrusionT5() const;
    const PTimeInterval & GetCallIntrusionT6() const;

    /**Get the next available invoke Id for H450 operations
      */
    unsigned GetNextH450CallIdentityValue() const;

    PString GetDefaultTransport() const;
  //@}

};