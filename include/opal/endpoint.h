/*
 * endpoint.h
 *
 * Telephony endpoint abstraction
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

#ifndef OPAL_OPAL_ENDPOINT_H
#define OPAL_OPAL_ENDPOINT_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <opal_config.h>

#include <opal/manager.h>
#include <opal/mediafmt.h>
#include <opal/transports.h>


class OpalCall;
class OpalMediaStream;


/**This class describes an endpoint base class.
   Each protocol (or psuedo-protocol) would create a descendant off this
   class to manage its particular subsystem. Typically this would involve
   listening for incoming connections and being able to set up outgoing
   connections. Depending on exact semantics it may need to spawn many threads
   to achieve this.

   An endpoint will also have a default set of media data formats that it can
   support. Connections created by it would initially have the same set, but
   according to the semantics of the underlying protocol may end up using a
   different set.

   Various call backs are provided for points in the connection management. As
   a rule these are passed straight on to the OpalManager for processing. An
   application may create descendants off this class' subclasses, eg
   H323EndPoint, to modify or monitor its behaviour but it does not have to do
   so as all basic operations are passed to the OpalManager so only that class
   need be subclassed.
 */
class OpalEndPoint : public PObject
{
    PCLASSINFO(OpalEndPoint, PObject);
  public:
    P_DECLARE_BITWISE_ENUM(Attributes, 2, (
      NoAttributes,
      IsNetworkEndPoint,
      SupportsE164
    ));

  /**@name Construction */
  //@{
    /**Create a new endpoint.
     */
    OpalEndPoint(
      OpalManager & manager,          ///<  Manager of all endpoints.
      const PCaselessString & prefix, ///<  Prefix for URL style address strings
      Attributes attributes           ///<  Bit mask of attributes endpoint has
    );

    /**Destroy the endpoint.
     */
    ~OpalEndPoint();

    /**Shut down the endpoint, this is called by the OpalManager just before
       destroying the object and can be handy to make sure some things are
       stopped before the vtable gets clobbered.
      */
    virtual void ShutDown();
  //@}

  /**@name Overrides from PObject */
  //@{
    /**Standard stream print function.
       The PObject class has a << operator defined that calls this function
       polymorphically.
      */
    void PrintOn(
      ostream & strm    ///<  Stream to output text representation
    ) const;
  //@}

  /**@name Listeners management */
  //@{
    /**Start a set of listeners on the endoint.
       This allows for the automatic creating of incoming call connections. 
       If the list is empty then GetDefaultListeners() is used.

       The \p add parameter indicatest that existing listeners are left running
       and new ones are added. If false then listeners that are not in the
       \p interfaces list are stopped. Listeners not already running are
       started and ones that are already running are untouched.

       Note: while the \p interfaces parameter is a string array, each element
       of the array should be compatible with OpalTransportAddress.

       See OpalTransportAddress for more details on the syntax of an interface
       definition.
      */
    bool StartListeners(
      const PStringArray & interfaces, ///<  Addresses of interfaces to listen on.
      bool add = true                  ///<  Add listeners to existing list
    );

    /**Add a listener to the endoint.
       This allows for the automatic creating of incoming call connections. /
       If the address is empty then the first entry of GetDefaultListeners() is used.

       See OpalTransportAddress for more details on the syntax of an interface
       definition.
      */
    virtual bool StartListener(
      const PString & address ///<  Address of interface to listen on.
    );

    /**Add a listener to the endoint.
       This allows for the automatic creating of incoming call connections. An
       application should use OnConnectionEstablished() to monitor when calls
       have arrived and been successfully negotiated.
      */
    virtual bool StartListener(
      OpalListener * listener ///<  Transport dependent listener.
    );

    /**Get the default listeners for the endpoint type.
       Default behaviour uses GetDefaultTransport() to produce a list of
       listener addresses based on IPv4 and IPv6 versions of INADDR_ANY.
      */
    virtual PStringArray GetDefaultListeners() const;

    /**Get comma separated list of transport protocols to create if
       no explicit listeners started.
       Transport protocols are as per OpalTransportAddress, e.g. "udp$". It
       may also have a ":port" after it if that transport type does not use
       the value from GetDefaultSignalPort().
      */
    virtual PString GetDefaultTransport() const;

    /**Get the default signal port for this endpoint.
     */
    virtual WORD GetDefaultSignalPort() const;

#if OPAL_PTLIB_SSL
    /** Apply the SSL certificates/key for SSL based calls, e.g. sips or h323s
      */
    virtual bool ApplySSLCredentials(
      PSSLContext & context,    ///< Context to which t set certificates
      bool create               ///< Create self signed cert/key if required
    ) const;
#endif

    /**Find a listener given the transport address.
      */
    OpalListener * FindListener(
        const OpalTransportAddress & iface ///<  Address of interface we may be listening on.
    );

    /** Find a listener that is compatible with the specified protocol
     */
    bool FindListenerForProtocol(
      const char * proto,         ///< Protocol to findlistener, e.g "tcp" or "udp"
      OpalTransportAddress & addr ///< Address of listner interface
    );

    /**Stop a listener given the transport address.
       Returns true if a listener was on that interface, and was stopped.
      */
    PBoolean StopListener(
        const OpalTransportAddress & iface ///<  Address of interface we may be listening on.
    );

    /**Remove a listener from the endoint.
       If the listener parameter is NULL then all listeners are removed.
      */
    PBoolean RemoveListener(
      OpalListener * listener ///<  Transport dependent listener.
    );

    /**Return the transport addresses for all listeners on this endpoint.
       If \p associatedTransport is provided then only listeners that are
       using the same interface as that trasnprt are returned.
      */
    OpalTransportAddressArray GetInterfaceAddresses(
      const OpalTransport * associatedTransport = NULL
                          ///<  Associated transport for precedence and translation
    ) const;

    /**Handle new incoming connection.
       This will either create a new connection object or utilise a previously
       created connection on the same transport address and reference number.
      */
#if DOXYGEN
    virtual void NewIncomingConnection(
      OpalListener & listener,            ///<  Listner that created transport
      const OpalTransportPtr & transport  ///<  Transport connection came in on
    );
#endif
    PDECLARE_AcceptHandlerNotifier(OpalEndPoint, NewIncomingConnection);

    /**Call back for a new connection has been constructed.
       This is called after CreateConnection has returned a new connection.
       It allows an application to make any custom adjustments to the
       connection before it begins to process the protocol. behind it.
      */
    virtual void OnNewConnection(
      OpalCall & call,              ///< Call that owns the newly created connection.
      OpalConnection & connection   ///< New connection just created
    );
  //@}

  /**@name Connection management */
  //@{
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
      OpalCall & call,          ///<  Owner of connection
      const PString & party,    ///<  Remote party to call
      void * userData = NULL,          ///<  Arbitrary data to pass to connection
      unsigned int options = 0,     ///<  Options bit mask to pass to conneciton
      OpalConnection::StringOptions * stringOptions = NULL ///< Options to pass to connection
    ) = 0;

    /**Callback for outgoing connection, it is invoked after OpalLineConnection::SetUpConnection
       This function allows the application to set up some parameters or to log some messages
       */
    virtual PBoolean OnSetUpConnection(OpalConnection &connection);
    
    /**Call back for answering an incoming call.
       This function is used for an application to control the answering of
       incoming calls.

       If true is returned then the connection continues. If false then the
       connection is aborted.

       Note this function should not block for any length of time. If the
       decision to answer the call may take some time eg waiting for a user to
       pick up the phone, then AnswerCallPending or AnswerCallDeferred should
       be returned.

       If an application overrides this function, it should generally call the
       ancestor version to complete calls. Unless the application completely
       takes over that responsibility. Generally, an application would only
       intercept this function if it wishes to do some form of logging. For
       this you can obtain the name of the caller by using the function
       OpalConnection::GetRemotePartyName().

       The default behaviour calls the OpalManager function of the same name.
     */
    virtual PBoolean OnIncomingConnection(
      OpalConnection & connection,  ///<  Connection that is calling
      unsigned options,             ///<  options for new connection (can't use default value as overrides will fail)
      OpalConnection::StringOptions * stringOptions  ///< Options to pass to connection
    );

    /**Call back for remote party is now responsible for completing the call.
       This function is called when the remote system has been contacted and it
       has accepted responsibility for completing, or failing, the call. This
       is distinct from OnAlerting() in that it is not known at this time if
       anything is ringing. This indication may be used to distinguish between
       "transport" level error, in which case another host may be tried, and
       that finalising the call has moved "upstream" and the local system has
       no more to do but await a result.

       If an application overrides this function, it should generally call the
       ancestor version for correct operation.

       The default behaviour calls the OpalManager function of the same name.
     */
    virtual void OnProceeding(
      OpalConnection & connection   ///<  Connection that is proceeeding
    );

    /**Call back for remote party being alerted.
       This function is called after the connection is informed that the
       remote endpoint is "ringing". Generally some time after the
       MakeConnection() function was called, this is function is called.

       If false is returned the connection is aborted.

       If an application overrides this function, it should generally call the
       ancestor version for correct operation. An application would typically
       only intercept this function if it wishes to do some form of logging.
       For this you can obtain the name of the caller by using the function
       OpalConnection::GetRemotePartyName().

       The default behaviour calls the OpalManager function of the same name.
     */
    virtual void OnAlerting(
      OpalConnection & connection,   ///<  Connection that indicates it is alerting
      bool withMedia                 ///<  Indicated media should be started, if possible
    );
    virtual void OnAlerting(OpalConnection & connection); // For backward compatibility

    /**Call back for answering an incoming call.
       This function is called after the connection has been acknowledged
       but before the connection is established

       This gives the application time to wait for some event before
       signalling to the endpoint that the connection is to proceed. For
       example the user pressing an "Answer call" button.

       If AnswerCallDenied is returned the connection is aborted and the
       connetion specific end call PDU is sent. If AnswerCallNow is returned 
       then the connection proceeding, Finally if AnswerCallPending is returned then the
       protocol negotiations are paused until the AnsweringCall() function is
       called.

       The default behaviour simply returns AnswerNow.
     */
    virtual OpalConnection::AnswerCallResponse OnAnswerCall(
      OpalConnection & connection,    ///<  connection that is being answered
       const PString & caller         ///<  caller
    );

    /**A call back function whenever a connection is "connected".
       This indicates that a connection to an endpoint was connected. That
       is the endpoint received acknowledgement via whatever protocol it uses
       that the connection may now start media streams.

       In the context of H.323 this means that the CONNECT pdu has been
       received.

       The default behaviour calls the OpalManager function of the same name.
      */
    virtual void OnConnected(
      OpalConnection & connection   ///<  Connection that was established
    );

    /**A call back function whenever a connection is established.
       This indicates that a connection to an endpoint was established. This
       usually occurs after OnConnected() and indicates that the connection
       is both connected and media can flow.

       In the context of H.323 this means that the signalling and control
       channels are open and the TerminalCapabilitySet and MasterSlave
       negotiations are complete.

       The default behaviour does nothing.
      */
    virtual void OnEstablished(
      OpalConnection & connection   ///<  Connection that was established
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

    /**A call back function whenever a connection is "held" or "retrieved".
       This indicates that a connection to an endpoint was held, or
       retrieved, either locally or by the remote endpoint.

       The default behaviour calls the OpalManager function of the same name.
      */
    virtual void OnHold(
      OpalConnection & connection,   ///<  Connection that was held/retrieved
      bool fromRemote,               ///<  Indicates remote has held local connection
      bool onHold                    ///<  Indicates have just been held/retrieved.
    );
    virtual void OnHold(OpalConnection & connection); // For backward compatibility

    /**A call back function whenever a connection is forwarded.

       The default behaviour does nothing.
      */
    virtual PBoolean OnForwarded(
      OpalConnection & connection,  ///<  Connection that was held
      const PString & remoteParty         ///<  The new remote party
    );

    /**A call back function to monitor the progress of a transfer.
       When a transfer operation is initiated, the Transfer() function will
       generally return immediately and the transfer may take some time. This
       call back can give an indication to the application of the progress of
       the transfer.
       the transfer.

       For example in SIP, the OpalCall::Transfer() function will have sent a
       REFER request to the remote party. The remote party sends us NOTIFY
       requests about the progress of the REFER request.

       An application can now make a decision during the transfer operation
       to short circuit the sequence, or let it continue. It can also
       determine if the transfer did not go through, and it should "take back"
       the call. Note no action is required to "take back" the call other than
       indicate to the user that they are back on.

       A return value of false will immediately disconnect the current call.

       The exact format of the \p info parameter is dependent on the protocol
       being used. As a minimum, it will always have a values info["result"]
       and info["party"].

       The info["party"] indicates the part the \p connection is playing in
       the transfer. This will be:
          "A"   party being transferred
          "B"   party initiating the transfer of "A"
          "C"   party "A" is being transferred to

       The info["result"] will be at least one of the following:
          "success"     Transfer completed successfully (party A or B)
          "incoming"    New call was from a transfer (party C)
          "started"     Transfer operation has started (party A)
          "progress"    Transfer is in progress (party B)
          "blind"       Transfer is blind, no further notification (party B)
          "error"       Transfer could not begin (party B)
          "failed"      Transfer started but did not complete (party A or B)

       For SIP, there may be an additional info["state"] containing the NOTIFY
       subscription state, an info["code"] entry containing the 3 digit
       code returned in the NOTIFY body and info["Referred-By"] indicating the
       URI of party B. Other fields may also be present.

       The default behaviour calls the OpalManager function of the same name.
       The default action of that function is to return false, thereby
       releasing the connection if the info["result"] == "success".
      */
    virtual bool OnTransferNotify(
      OpalConnection & connection,  ///< Connection being transferred.
      const PStringToString & info, ///< Information on the transfer
      const OpalConnection * transferringConnection
                                    ///< Connection performing transfer, maybe "connection"
    );

    /**Clear a call.
       This finds the call by using the token then calls the OpalCall::Clear()
       function on it. All connections are released, and the conenctions and
       call disposed of. Note that this function returns quickly and the
       disposal happens at some later time by a background thread. This it is
       safe to call this function from anywhere.

       If \p sync is not NULL then it is signalled when the calls are cleared.
      */
    virtual PBoolean ClearCall(
      const PString & token,    ///<  Token for identifying connection
      OpalConnection::CallEndReason reason = OpalConnection::EndedByLocalUser, ///<  Reason for call clearing
      PSyncPoint * sync = NULL  ///<  Sync point to wait on.
    );

    /**Clear a current connection.
       This hangs up the connection to a remote endpoint. Note that this function
       is always synchronous. If \p sync is NULL then a local PSyncPoint is used.
      */
    virtual PBoolean ClearCallSynchronous(
      const PString & token,    ///<  Token for identifying connection
      OpalConnection::CallEndReason reason = OpalConnection::EndedByLocalUser, ///<  Reason for call clearing
      PSyncPoint * sync = NULL  ///<  Sync point to wait on.
    );

    /**Clear all current connections.
       This hangs up all the connections to remote endpoints. The wait
       parameter is used to wait for all the calls to be cleared and their
       memory usage cleaned up before returning. This is typically used in
       the destructor for your descendant of H323EndPoint.
      */
    virtual void ClearAllCalls(
      OpalConnection::CallEndReason reason = OpalConnection::EndedByLocalUser, ///<  Reason for call clearing
      PBoolean wait = true   ///<  Flag for wait for calls to e cleared.
    );

    /**Find a connection that uses the specified token.
       This searches the endpoint for the connection that is identified by the
       connection token as provided by functions such as MakeConnection().

       The \p token string may also be the call token that identifies the
       OpalCall instance. The first connection in that call that has this
       endpoint as it's endpoint is returned.

       The \p string may also be the protocol specific identifier used, as
       returned by OpalConnection::GetIdentifier().

       Finally, the \p token string may also be of the form prefix:name,
       e.g. ivr:fred, and the GetLocalPartyName() is used to locate the
       connection.
      */
    PSafePtr<OpalConnection> GetConnectionWithLock(
      const PString & token,     ///<  Token to identify connection
      PSafetyMode mode = PSafeReadWrite ///< Locking mode
    ) const;

    /**Find a connection that uses the specified token.
       This searches the endpoint for the connection that contains the token
       as provided by functions such as MakeConnection(). If not then it
       attempts to use the token as a OpalCall token and find a connection
       of the same class.
      */
    template <class ConnClass>
    PSafePtr<ConnClass> GetConnectionWithLockAs(
      const PString & token,     ///<  Token to identify connection
      PSafetyMode mode = PSafeReadWrite ///< Locking mode
    ) const
    {
      PSafePtr<ConnClass> connection = PSafePtrCast<OpalConnection, ConnClass>(GetConnectionWithLock(token, mode));
      if (connection == NULL) {
        PSafePtr<OpalCall> call = m_manager.FindCallWithLock(token, PSafeReadOnly);
        if (call != NULL) {
          connection = PSafePtrCast<OpalConnection, ConnClass>(call->GetConnection(0, mode));
          if (connection == NULL)
            connection = PSafePtrCast<OpalConnection, ConnClass>(call->GetConnection(1, mode));
        }
      }
      return connection;
    }

    /**Get all calls current on the endpoint.
      */
    PStringList GetAllConnections();
    
    /** Get calls count on the endpoint
      */
    PINDEX GetConnectionCount() const { return m_connectionsActive.GetSize(); }

    /**Determine if a connection is active.
      */
    virtual PBoolean HasConnection(
      const PString & token   ///<  Token for identifying connection
    );

    /**Destroy the connection.
      */
    virtual void DestroyConnection(
      OpalConnection * connection  ///<  Connection to destroy
    );
  //@}

  /**@name Media Streams management */
  //@{
    /**Get the data formats this endpoint is capable of operating.
       This provides a list of media data format names that may be used by an
       OpalMediaStream may be created by a connection from this endpoint.

       Note that a specific connection may not actually support all of the
       media formats returned here, but should return no more.

       The default behaviour is pure.
      */
    virtual OpalMediaFormatList GetMediaFormats() const = 0;

    /**Adjust media formats available on a connection.
       This is called by a connection after it has called
       OpalCall::GetMediaFormats() to get all media formats that it can use so
       that an application may remove or reorder the media formats before they
       are used to open media streams.

       The default behaviour calls the OpalManager function of the same name.
      */
    virtual void AdjustMediaFormats(
      bool local,                         ///<  Media formats a local ones to be presented to remote
      const OpalConnection & connection,  ///<  Connection that is about to use formats
      OpalMediaFormatList & mediaFormats  ///<  Media formats to use
    ) const;

    /**Get transports for the media session on the connection.
       This is primarily used by the media bypass feature controlled by the
       OpalManager::GetMediaTransferMode() function. It allows one side of the
       call to get the transport address of the media on the other side, so it
       can pass it on, bypassing the local host.

       @return true if a transport address is available and may be used to pass
               on to a remote system for direct access.
     */
    virtual bool GetMediaTransportAddresses(
      const OpalConnection & provider,       ///< Half of call providing media transport addresses
      const OpalConnection & consumer,       ///< Other half of call needing media transport addresses
      unsigned sessionId,                    ///< Session identifier
      const OpalMediaType & mediaType,       ///< Media type for session to return information
      OpalTransportAddressArray & transports ///<  Information on media session
    ) const;

    /**Call back when opening a media stream.
       This function is called when a connection has created a new media
       stream according to the logic of its underlying protocol.

       The usual requirement is that media streams are created on all other
       connections participating in the call and all of the media streams are
       attached to an instance of an OpalMediaPatch object that will read from
       one of the media streams passing data to the other media streams.

       The default behaviour calls the OpalManager function of the same name.
      */
    virtual PBoolean OnOpenMediaStream(
      OpalConnection & connection,  ///<  Connection that owns the media stream
      OpalMediaStream & stream      ///<  New media stream being opened
    );

    /**Call back for closed a media stream.

       The default behaviour calls the OpalManager function of the same name.
      */
    virtual void OnClosedMediaStream(
      const OpalMediaStream & stream     ///<  Media stream being closed
    );

    /**Call back for a media stream that failed to open.

       The default behaviour calls the OpalManager function of the same name.
      */
    virtual void OnFailedMediaStream(
      OpalConnection & connection,  ///<  Connection that attempted to open media stream
      bool fromRemote,              ///< Flag indicating the attempt to open was from remote
      const PString & reason        ///< Reason for the open fail
    );

    /**Set media security methods in priority order.
       The \p security parameter is an array of names for security methods,
       e.g. { "Clear", "AES_CM_128_HMAC_SHA1_80", "AES_CM_128_HMAC_SHA1_32" }.
       Note "Clear" is not compulsory and if absent will mean that a secure
       media is required or the call will not proceed. The special value of
       "!Clear" may also be used indicating all available crypto suites are
       offered but there must be encryption.

       An empty list is assumed to be "Clear".
      */
    void SetMediaCryptoSuites(
      const PStringArray & security   ///< New security methods to use
    );

    /**Get media security methods in priority order.
       Returns an array of names for security methods,
       e.g. { "Clear", "AES_CM_128_HMAC_SHA1_80", "AES_CM_128_HMAC_SHA1_32" }.
      */
    PStringArray GetMediaCryptoSuites() const
      { return m_mediaCryptoSuites.IsEmpty() ? GetAllMediaCryptoSuites() : m_mediaCryptoSuites; }

    /**Get all possible media security methods for this endpoint type.
       Note this will always return "Clear" as the first entry.
      */
    virtual PStringArray GetAllMediaCryptoSuites() const;
  //@}

  /**@name User indications */
  //@{
    /**Call back for remote enpoint has sent user input as a string.

       The default behaviour calls the OpalManager function of the same name.
      */
    virtual void OnUserInputString(
      OpalConnection & connection,  ///<  Connection input has come from
      const PString & value   ///<  String value of indication
    );

    /**Call back for remote enpoint has sent user input.
       If \p duration is zero then this indicates the beginning of the tone.
       If \p duration is greater than zero then it indicates the end of the
       tone output and how long the tone had run.

       Note, there is no guarantee a zero value (start tone) will occur. There
       is also no guarantee this function is called at all, given how the
       remote may send user indications. For simple, "event" based, user
       indications the OnUserInputString() should be used. THis function is
       only for when a more precise representation of the tone, and it's
       duration, is required.

       The default behaviour calls the OpalManager function of the same name.
      */
    virtual void OnUserInputTone(
      OpalConnection & connection,  ///<  Connection input has come from
      char tone,                    ///<  Tone received
      int duration                  ///<  Duration of tone
    );

    /**Read a sequence of user indications from connection with timeouts.
      */
    virtual PString ReadUserInput(
      OpalConnection & connection,        ///<  Connection to read input from
      const char * terminators = "#\r\n", ///<  Characters that can terminte input
      unsigned lastDigitTimeout = 4,      ///<  Timeout on last digit in string
      unsigned firstDigitTimeout = 30     ///<  Timeout on receiving any digits
    );
  //@}


#if OPAL_HAS_IM
  /**@name Instant Messaging */
  //@{
    /**Send text message
     */
    virtual PBoolean Message(
      const PString & to, 
      const PString & body
    );
    virtual PBoolean Message(
      const PURL & to, 
      const PString & type,
      const PString & body,
      PURL & from, 
      PString & conversationId
    );
    virtual PBoolean Message(
      OpalIM & Message
    );

    /**Called when text message received
     */
    virtual void OnMessageReceived(
      const OpalIM & message
    );
  //@}
#endif // OPAL_HAS_IM


  /**@name Other services */
  //@{
    /**Callback called when Message Waiting Indication (MWI) is received.
       Multiple callbacks may occur with each MessageWaitingType. A \p type
       of NumMessageWaitingTypes indicates the server is unable to distinguish
       the message type.

       The \p extraInfo parameter is generally of the form "a/b" where a and b
       unsigned integers representing new and old message count. However, it
       may be a simple "yes" or "no" if the remote cannot provide a message
       count.
     */
    virtual void OnMWIReceived (
      const PString & party,                ///< Name of party MWI is for
      OpalManager::MessageWaitingType type, ///< Type of message that is waiting
      const PString & extraInfo             ///< Addition information on the MWI
    );

    /**Get conference state information for all nodes.
       This obtains the state of one or more conferences managed by this
       endpoint. If this endpoint does not do conferencing, then false is
       returned.

       The \p name parameter may be one of the aliases for the conference, or
       the internal URI for the conference. An empty string indicates all 
       active conferences are to be returned.

       Note that if the \p name does not match an active conference, true is
       still returned, but the states list will be empty.

       The default behaviour returns false indicating this is not a
       conferencing endpoint.
      */
    virtual bool GetConferenceStates(
      OpalConferenceStates & states,           ///< List of conference states
      const PString & name = PString::Empty() ///< Name for specific node, empty string is all
    ) const;

    /**Call back when conferencing state information changes.
       If a conferencing endpoint type detects a change in a conference nodes
       state, as would be returned by GetConferenceStatus() then this function
       will be called on all endpoints in the OpalManager.

       The \p uri parameter is as is the internal URI for the conference.

       Default behaviour does nothing.
      */
    virtual void OnConferenceStatusChanged(
      OpalEndPoint & endpoint,  /// < Endpoint sending state change
      const PString & uri,      ///< Internal URI of conference node that changed
      OpalConferenceState::ChangeType change ///< Change that occurred
    );

    /**Build a list of network accessible URIs given a user name.
       This typically gets URI's like sip:user@interface, h323:user@interface
       etc, for each listener of each endpoint.
      */
    virtual PStringList GetNetworkURIs(
      const PString & name
    ) const;

    /// Strip the prefix off the party name.
    PString StripPrefixName(const PString & partyName) const;

    /** Execute garbage collection for endpoint.
        Returns true if all garbage has been collected.
        Default behaviour deletes the objects in the connectionsActive list.
      */
    virtual PBoolean GarbageCollection();
  //@}

  /**@name Member variable access */
  //@{
    /**Get the manager for this endpoint.
     */
    OpalManager & GetManager() const { return m_manager; }

    /**Get the protocol prefix name for the endpoint.
      */
    const PString & GetPrefixName() const { return m_prefixName; }

    /**Get an indication of if this endpoint has particular option.
      */
    bool HasAttribute(Attributes opt) const { return m_attributes & opt; }

    /**Get the product info for all endpoints.
      */
    const OpalProductInfo & GetProductInfo() const { return m_productInfo; }

    /**Set the product info for all endpoints.
      */
    void SetProductInfo(
      const OpalProductInfo & info
    ) { m_productInfo = info; }

    /**Get the default local party name for all connections on this endpoint.
      */
    const PString & GetDefaultLocalPartyName() const { return m_defaultLocalPartyName; }

    /**Set the default local party name for all connections on this endpoint.
      */
    virtual void SetDefaultLocalPartyName(
      const PString & name  /// Name for local party
    ) { m_defaultLocalPartyName = name; }

    /**Get the default local display name for all connections on this endpoint.
      */
    const PString & GetDefaultDisplayName() const { return m_defaultDisplayName; }

    /**Set the default local display name for all connections on this endpoint.
      */
    void SetDefaultDisplayName(const PString & name) { m_defaultDisplayName = name; }

    /**Get the initial bandwidth parameter for a connection.
     */
    OpalBandwidth GetInitialBandwidth(
      OpalBandwidth::Direction dir   ///< Bandwidth direction
    ) const;

    /**Set the initial bandwidth parameter for a connection.
     */
    bool SetInitialBandwidth(
      OpalBandwidth::Direction dir,   ///< Bandwidth direction
      OpalBandwidth bandwidth         ///< New bandwidth
    );

    /**Get the set of listeners (incoming call transports) for this endpoint.
     */
    const OpalListenerList & GetListeners() const { return m_listeners; }

    /**Get the default options for created connections.
      */
    const OpalConnection::StringOptions & GetDefaultStringOptions() const { return m_defaultStringOptions; }

    /**Set the default options for created connections.
      */
    void SetDefaultStringOptions(
      const OpalConnection::StringOptions & opts, ///< Options to set
      bool overwrite = false
    );

    /**Set the default option for created connections.
      */
    void SetDefaultStringOption(const PCaselessString & key, const PString & data) { m_defaultStringOptions.SetAt(key, data); }

    /**Removet the default option for created connections.
      */
    void RemoveDefaultStringOption(const PCaselessString & key) { m_defaultStringOptions.RemoveAt(key); }

    /** Get available string option names.
      */
    virtual PStringList GetAvailableStringOptions() const;

    /**Get the default mode for sending User Input Indications.
      */
    OpalConnection::SendUserInputModes GetSendUserInputMode() const { return m_defaultSendUserInputMode; }

    /**Set the default mode for sending User Input Indications.
      */
    void SetSendUserInputMode(OpalConnection::SendUserInputModes mode) { m_defaultSendUserInputMode = mode; }

    /**Get the IP Quality of Service info for signaling channels.
     */
    const PIPSocket::QoS & GetSignalQoS() const { return m_signalQoS; }

    /**Set the IP Quality of Service info for signaling channels.
     */
    void SetSignalQoS(const PIPSocket::QoS & qos) { m_signalQoS = qos; }

    /**Get the IP Quality of Service info for media (eg RTP) channels.
       If OpalEndPoint::SetMediaQoS() was not called for this media type, then
       the OpalManager::GetMediaQoS() is returned.
     */
    const PIPSocket::QoS & GetMediaQoS(const OpalMediaType & type) const;

    /**Set the IP Quality of Service info for media (eg RTP) channels.
     */
    void SetMediaQoS(const OpalMediaType & type, const PIPSocket::QoS & qos);

    /**Get the maximum size of a UDP packet that can be received.
      */
    PINDEX GetMaxSizeUDP() const { return m_maxSizeUDP; }

    /**Set the maximum size of a UDP packet that can be received.
      */
    void SetMaxSizeUDP(PINDEX size) { m_maxSizeUDP = size; }
  //@}

  protected:
    OpalManager   & m_manager;
    PCaselessString m_prefixName;
    Attributes      m_attributes;
    PINDEX          m_maxSizeUDP;
    OpalProductInfo m_productInfo;
    PString         m_defaultLocalPartyName;
    PString         m_defaultDisplayName;
    PStringArray    m_mediaCryptoSuites;
    PIPSocket::QoS  m_signalQoS;
    OpalManager::MediaQoSMap m_mediaQoS;

    OpalBandwidth m_initialRxBandwidth;
    OpalBandwidth m_initialTxBandwidth;
    OpalConnection::StringOptions      m_defaultStringOptions;
    OpalConnection::SendUserInputModes m_defaultSendUserInputMode;

    OpalListenerList   m_listeners;

    class ConnectionDict : public PSafeDictionary<PString, OpalConnection>
    {
        virtual void DeleteObject(PObject * object) const;
    } m_connectionsActive;
    OpalConnection * AddConnection(OpalConnection * connection);

    friend void OpalManager::GarbageCollection();
    friend void OpalConnection::SetToken(const PString & newToken);

  private:
    P_REMOVE_VIRTUAL(PBoolean, OnIncomingConnection(OpalConnection &, unsigned), false);
    P_REMOVE_VIRTUAL(PBoolean, OnIncomingConnection(OpalConnection &), false);
    P_REMOVE_VIRTUAL_VOID(AdjustMediaFormats(const OpalConnection &, OpalMediaFormatList &) const);
    P_REMOVE_VIRTUAL_VOID(OnMessageReceived(const PURL&,const PString&,const PURL&,const PString&,const PString&,const PString&));
    P_REMOVE_VIRTUAL(OpalMediaSession *, CreateMediaSession(OpalConnection &, unsigned, const OpalMediaType &), NULL);
    P_REMOVE_VIRTUAL(PBoolean, NewIncomingConnection(OpalTransport *), false);
    P_REMOVE_VIRTUAL(bool, OnTransferNotify(OpalConnection &, const PStringToString &), false);
#if OPAL_PTLIB_NAT
    P_REMOVE_VIRTUAL(PNatMethod *, GetNatMethod(const PIPSocket::Address &) const, NULL);
#endif
};


/// Test for if string is a valid E.164 number
bool OpalIsE164(
  const PString & number,   ///< Number to inspect
  bool strict = false     ///< Strict interpretation, or allow leading '+'
);


#endif // OPAL_OPAL_ENDPOINT_H


// End of File ///////////////////////////////////////////////////////////////
