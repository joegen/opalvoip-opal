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
 *
 * $Log: h323ep.h,v $
 * Revision 1.2017  2002/09/10 07:42:40  robertj
 * Added function to get gatekeeper password.
 *
 * Revision 2.15  2002/09/06 02:39:27  robertj
 * Added function to set gatekeeper access token OID.
 *
 * Revision 2.14  2002/09/04 06:01:46  robertj
 * Updated to OpenH323 v1.9.6
 *
 * Revision 2.13  2002/07/04 07:41:46  robertj
 * Fixed memory/thread leak of transports.
 *
 * Revision 2.12  2002/07/01 04:56:29  robertj
 * Updated to OpenH323 v1.9.1
 *
 * Revision 2.11  2002/03/22 06:57:48  robertj
 * Updated to OpenH323 version 1.8.2
 *
 * Revision 2.10  2002/03/18 00:33:36  robertj
 * Removed duplicate initialBandwidth variable in H.323 class, moved to ancestor.
 *
 * Revision 2.9  2002/02/11 09:32:11  robertj
 * Updated to openH323 v1.8.0
 *
 * Revision 2.8  2002/01/22 04:59:04  robertj
 * Update from OpenH323, RFC2833 support
 *
 * Revision 2.7  2002/01/14 06:35:57  robertj
 * Updated to OpenH323 v1.7.9
 *
 * Revision 2.6  2001/11/13 06:25:56  robertj
 * Changed SetUpConnection() so returns BOOL as returning
 *   pointer to connection is not useful.
 *
 * Revision 2.5  2001/11/02 10:45:19  robertj
 * Updated to OpenH323 v1.7.3
 *
 * Revision 2.4  2001/10/05 00:22:13  robertj
 * Updated to PWLib 1.2.0 and OpenH323 1.7.0
 *
 * Revision 2.3  2001/08/17 08:21:15  robertj
 * Update from OpenH323
 * Moved call end reasons enum from OpalConnection to global.
 *
 * Revision 2.2  2001/08/01 05:10:40  robertj
 * Moved media formats list from endpoint to connection.
 *
 * Revision 2.1  2001/07/30 07:22:25  robertj
 * Abstracted listener management from H.323 to OpalEndPoint class.
 *
 * Revision 2.0  2001/07/27 15:48:24  robertj
 * Conversion of OpenH323 to Open Phone Abstraction Library (OPAL)
 *
 * Revision 1.34  2002/09/03 06:19:36  robertj
 * Normalised the multi-include header prevention ifdef/define symbol.
 *
 * Revision 1.33  2002/07/19 03:39:19  robertj
 * Bullet proofed setting of RTP IP port base, can't be zero!
 *
 * Revision 1.32  2002/07/18 01:50:10  robertj
 * Changed port secltion code to force apps to use function interface.
 *
 * Revision 1.31  2002/06/22 05:48:38  robertj
 * Added partial implementation for H.450.11 Call Intrusion
 *
 * Revision 1.30  2002/06/13 06:15:19  robertj
 * Allowed TransferCall() to be used on H323Connection as well as H323EndPoint.
 *
 * Revision 1.29  2002/06/12 03:55:21  robertj
 * Added function to add/remove multiple listeners in one go comparing against
 *   what is already running so does not interrupt unchanged listeners.
 *
 * Revision 1.28  2002/05/29 06:40:29  robertj
 * Changed sending of endSession/ReleaseComplete PDU's to occur immediately
 *   on call clearance and not wait for background thread to do it.
 * Stricter compliance by waiting for reply endSession before closing down.
 *
 * Revision 1.27  2002/05/28 06:15:09  robertj
 * Split UDP (for RAS) from RTP port bases.
 * Added current port variable so cycles around the port range specified which
 *   fixes some wierd problems on some platforms, thanks Federico Pinna
 *
 * Revision 1.26  2002/05/17 03:38:05  robertj
 * Fixed problems with H.235 authentication on RAS for server and client.
 *
 * Revision 1.25  2002/05/16 00:03:05  robertj
 * Added function to get the tokens for all active calls.
 * Improved documentation for use of T.38 and T.120 functions.
 *
 * Revision 1.24  2002/05/15 08:59:18  rogerh
 * Update comments
 *
 * Revision 1.23  2002/05/03 05:38:15  robertj
 * Added Q.931 Keypad IE mechanism for user indications (DTMF).
 *
 * Revision 1.22  2002/05/02 07:56:24  robertj
 * Added automatic clearing of call if no media (RTP data) is transferred in a
 *   configurable (default 5 minutes) amount of time.
 *
 * Revision 1.21  2002/04/18 01:41:07  robertj
 * Fixed bad variable name for disabling DTMF detection, very confusing.
 *
 * Revision 1.20  2002/04/17 00:49:56  robertj
 * Added ability to disable the in band DTMF detection.
 *
 * Revision 1.19  2002/04/10 06:48:47  robertj
 * Added functions to set port member variables.
 *
 * Revision 1.18  2002/03/14 03:49:38  dereks
 * Fix minor documentation error.
 *
 * Revision 1.17  2002/02/04 07:17:52  robertj
 * Added H.450.2 Consultation Transfer, thanks Norwood Systems.
 *
 * Revision 1.16  2002/01/24 06:29:02  robertj
 * Added option to disable H.245 negotiation in SETUP pdu, this required
 *   API change so have a bit mask instead of a series of booleans.
 *
 * Revision 1.15  2002/01/17 07:04:58  robertj
 * Added support for RFC2833 embedded DTMF in the RTP stream.
 *
 * Revision 1.14  2002/01/13 23:59:43  robertj
 * Added CallTransfer timeouts to endpoint, hanks Ben Madsen of Norwood Systems.
 *
 * Revision 1.13  2002/01/08 04:45:35  robertj
 * Added MakeCallLocked() so can start a call with the H323Connection instance
 *   initally locked so can do things to it before the call really starts.
 *
 * Revision 1.12  2001/12/22 03:20:44  robertj
 * Added create protocol function to H323Connection.
 *
 * Revision 1.11  2001/12/13 10:55:30  robertj
 * Added gatekeeper access token OID specification for auto inclusion of
 *   access tokens frm ACF to SETUP pdu.
 *
 * Revision 1.10  2001/11/09 05:39:54  craigs
 * Added initial T.38 support thanks to Adam Lazur
 *
 * Revision 1.9  2001/11/01 00:27:33  robertj
 * Added default Fast Start disabled and H.245 tunneling disable flags
 *   to the endpoint instance.
 *
 * Revision 1.8  2001/09/11 01:24:36  robertj
 * Added conditional compilation to remove video and/or audio codecs.
 *
 * Revision 1.7  2001/09/11 00:21:21  robertj
 * Fixed missing stack sizes in endpoint for cleaner thread and jitter thread.
 *
 * Revision 1.6  2001/08/24 14:03:26  rogerh
 * Fix some spelling mistakes
 *
 * Revision 1.5  2001/08/16 07:49:16  robertj
 * Changed the H.450 support to be more extensible. Protocol handlers
 *   are now in separate classes instead of all in H323Connection.
 *
 * Revision 1.4  2001/08/10 11:03:49  robertj
 * Major changes to H.235 support in RAS to support server.
 *
 * Revision 1.3  2001/08/08 23:54:11  robertj
 * Fixed problem with setting gk password before have a gk variable.
 *
 * Revision 1.2  2001/08/06 03:15:17  robertj
 * Improved access to H.235 secure RAS functionality.
 *
 * Revision 1.1  2001/08/06 03:08:11  robertj
 * Fission of h323.h to h323ep.h & h323con.h, h323.h now just includes files.
 */

#ifndef __OPAL_H323EP_H
#define __OPAL_H323EP_H

#ifdef __GNUC__
#pragma interface
#endif


#include <opal/endpoint.h>
#include <opal/manager.h>
#include <opal/transports.h>
#include <h323/h323con.h>
#include <h323/h323caps.h>
#include <h323/h235auth.h>


class H225_EndpointType;
class H225_VendorIdentifier;
class H225_H221NonStandard;

class H235SecurityInfo;

class H323Gatekeeper;
class H323SignalPDU;


///////////////////////////////////////////////////////////////////////////////

/**This class manages the H323 endpoint.
   An endpoint may have zero or more listeners to create incoming connections
   or zero or more outgoing conenctions initiated via the MakeCall() function.
   Once a conection exists it is managed by this class instance.

   The main thing this class embodies is the capabilities of the application,
   that is the codecs and protocols it is capable of.

   An application may create a descendent off this class and overide the
   CreateConnection() function, if they require a descendent of H323Connection
   to be created. This would be quite likely in most applications.
 */
class H323EndPoint : public OpalEndPoint
{
  PCLASSINFO(H323EndPoint, OpalEndPoint);

  public:
    enum {
      DefaultSignalTcpPort = 1720
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
    virtual BOOL SetUpConnection(
      OpalCall & call,        /// Owner of connection
      const PString & party,  /// Remote party to call
      void * userData = NULL  /// Arbitrary data to pass to connection
    );
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
      H323Capability * capability   /// New codec specification
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
      PINDEX descriptorNum, /// The member of the capabilityDescriptor to add
      PINDEX simultaneous,  /// The member of the SimultaneousCapabilitySet to add
      H323Capability * cap  /// New capability specification
    );

    /**Add all matching capabilities in list.
       All capabilities that match the specified name are added. See the
       capabilities code for details on the matching algorithm.
      */
    PINDEX AddAllCapabilities(
      PINDEX descriptorNum, /// The member of the capabilityDescriptor to add
      PINDEX simultaneous,  /// The member of the SimultaneousCapabilitySet to add
      const PString & name  /// New capabilities name, if using "known" one.
    );

    /**Add all user input capabilities to this endpoints capability table.
      */
    void AddAllUserInputCapabilities(
      PINDEX descriptorNum, /// The member of the capabilityDescriptor to add
      PINDEX simultaneous   /// The member of the SimultaneousCapabilitySet to add
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
      const H245_Capability & cap  /// H245 capability table entry
    ) const;

    /**Find a capability that has been registered.
     */
    H323Capability * FindCapability(
      const H245_DataType & dataType  /// H245 data type of codec
    ) const;

    /**Find a capability that has been registered.
     */
    H323Capability * FindCapability(
      H323Capability::MainTypes mainType,   /// Main type of codec
      unsigned subType                      /// Subtype of codec
    ) const;
  //@}

  /**@name Gatekeeper management */
  //@{
    /**Select and register with an explicit gatekeeper.
       This will use the specified transport and a string giving a transport
       dependent address to locate a specific gatekeeper. The endpoint will
       register with that gatekeeper and, if successful, set it as the current
       gatekeeper used by this endpoint.

       Note the transport being passed in will be deleted by this function or
       the H323Gatekeeper object it becomes associated with. Also if transport
       is NULL then a OpalTransportUDP is created.
     */
    BOOL SetGatekeeper(
      const PString & address,          /// Address of gatekeeper to use.
      OpalTransport * transport = NULL  /// Transport over which to talk to gatekeeper.
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
       is NULL then a OpalTransportUDP is created.
     */
    BOOL SetGatekeeperZone(
      const PString & address,          /// Address of gatekeeper to use.
      const PString & identifier,       /// Identifier of gatekeeper to use.
      OpalTransport * transport = NULL  /// Transport over which to talk to gatekeeper.
    );

    /**Locate and select gatekeeper.
       This function will use the automatic gatekeeper discovery methods to
       locate the gatekeeper on the particular transport that has the specified
       gatekeeper identifier name. This is often the "Zone" for the gatekeeper.

       Note the transport being passed in will be deleted becomes owned by the
       H323Gatekeeper created by this function and will be deleted by it. Also
       if transport is NULL then a OpalTransportUDP is created.
     */
    BOOL LocateGatekeeper(
      const PString & identifier,       /// Identifier of gatekeeper to locate.
      OpalTransport * transport = NULL  /// Transport over which to talk to gatekeeper.
    );

    /**Discover and select gatekeeper.
       This function will use the automatic gatekeeper discovery methods to
       locate the first gatekeeper on a particular transport.

       Note the transport being passed in will be deleted becomes owned by the
       H323Gatekeeper created by this function and will be deleted by it. Also
       if transport is NULL then a OpalTransportUDP is created.
     */
    BOOL DiscoverGatekeeper(
      OpalTransport * transport = NULL  /// Transport over which to talk to gatekeeper.
    );

    /**Create a gatekeeper.
       This allows the application writer to have the gatekeeper as a
       descendent of the H323Gatekeeper in order to add functionality to the
       base capabilities in the library.

       The default creates an instance of the H323Gatekeeper class.
     */
    virtual H323Gatekeeper * CreateGatekeeper(
      OpalTransport * transport  /// Transport over which gatekeepers communicates.
    );

    /**Get the gatekeeper we are registered with.
     */
    H323Gatekeeper * GetGatekeeper() const { return gatekeeper; }

    /**Unregister and delete the gatekeeper we are registered with.
       The return value indicates FALSE if there was an error during the
       unregistration. However the gatekeeper is still removed and its
       instance deleted regardless of this error.
     */
    BOOL RemoveGatekeeper(
      int reason = -1    /// Reason for gatekeeper removal
    );

    /**Set the H.235 password for the gatekeeper.
      */
    void SetGatekeeperPassword(
      const PString & password
    );

    /**Get the H.235 password for the gatekeeper.
      */
    const PString & GetGatekeeperPassword() const { return gatekeeperPassword; }

    /**Create a list of authenticators for gatekeeper.
      */
    virtual H235Authenticators CreateAuthenticators();
  //@}

  /**@name Connection management */
  //@{
    /**Handle new incoming connetion from listener.
      */
    virtual BOOL NewIncomingConnection(
      OpalTransport * transport  /// Transport connection came in on
    );

    /**Create a connection that uses the specified call.
      */
    virtual H323Connection * CreateConnection(
      OpalCall & call,           /// Call object to attach the connection to
      const PString & token,     /// Call token for new connection
      void * userData,           /// Arbitrary user data from SetUpConnection
      OpalTransport * transport, /// Transport connection came in on
      H323SignalPDU * setupPDU   /// Setup PDU for incoming call
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
    virtual BOOL SetupTransfer(
      const PString & token,        /// Existing connection to be transferred
      const PString & callIdentity, /// Call identity of the secondary call (if it exists)
      const PString & remoteParty,  /// Remote party to transfer the existing call to
      void * userData = NULL        /// user data to pass to CreateConnection
    );

    /**Initiate the transfer of an existing call (connection) to a new remote
       party using H.450.2.  This sends a Call Transfer Initiate Invoke
       message from the A-Party (transferring endpoint) to the B-Party
       (transferred endpoint).
     */
    void TransferCall(
      const PString & token,        /// Existing connection to be transferred
      const PString & remoteParty,  /// Remote party to transfer the existing call to
      const PString & callIdentity = PString::Empty()
                                    /// Call Identity of secondary call if present
    );

    /**Transfer the call through consultation so the remote party in the
       primary call is connected to the called party in the second call
       using H.450.2.  This sends a Call Transfer Identify Invoke message
       from the A-Party (transferring endpoint) to the C-Party
       (transferred-to endpoint).
     */
    void ConsultationTransfer(
      const PString & primaryCallToken,   /// Token of primary call
      const PString & secondaryCallToken  /// Token of secondary call
    );

    /**Place the call on hold, suspending all media channels (H.450.4)
    * NOTE: Only Local Hold is implemented so far. 
    */
    void HoldCall(
      const PString & token,        /// Existing connection to be transferred
      BOOL localHold   /// true for Local Hold, false for Remote Hold
    );

    /** Initiate Call intrusion
        Designed similar to MakeCall function
      */
    BOOL IntrudeCall(
      const PString & remoteParty,  /// Remote party to intrude call
      unsigned capabilityLevel,     /// Capability level
      void * userData = NULL        /// user data to pass to CreateConnection
    );

    /**Parse a party address into alias and transport components.
       An appropriate transport is determined from the remoteParty parameter.
       The general form for this parameter is [alias@][transport$]host[:port]
       where the default alias is the same as the host, the default transport
       is "ip" and the default port is 1720.
      */
    void ParsePartyName(
      const PString & party,          /// Party name string.
      PString & alias,                /// Parsed alias name
      OpalTransportAddress & address  /// Parsed transport address
    ) const;

    /**Find a connection that uses the specified token.
       This searches the endpoint for the connection that contains the token
       as provided by functions such as MakeCall(). if not found it will then search for
       the string representation of the CallIdentifier for the connection, and
       finally try for the string representation of the ConferenceIdentifier.

       Note the caller of this function MUSt call the H323Connection::Unlock()
       function if this function returns a non-NULL pointer. If it does not
       then a deadlock can occur.
      */
    H323Connection * FindConnectionWithLock(
      const PString & token     /// Token to identify connection
    );

    /**Call back for incoming call.
       This function is called from the OnReceivedSignalSetup() function
       before it sends the Alerting PDU. It gives an opportunity for an
       application to alter the reply before transmission to the other
       endpoint.

       If FALSE is returned the connection is aborted and a Release Complete
       PDU is sent.

       The default behaviour simply returns TRUE.
     */
    virtual BOOL OnIncomingCall(
      H323Connection & connection,    /// Connection that was established
      const H323SignalPDU & setupPDU,   /// Received setup PDU
      H323SignalPDU & alertingPDU       /// Alerting PDU to send
    );

    /**Handle a connection transfer.
       This gives the application an opportunity to abort the transfer.
       The default behaviour just returns TRUE.
      */
    BOOL OnCallTransferInitiate(
      H323Connection & connection,    /// Connection to transfer
      const PString & remoteParty     /// Party transferring to.
    );

    /**Handle a transfer via consultation.
       This gives the transferred-to user an opportunity to abort the transfer.
       The default behaviour just returns TRUE.
      */
    virtual BOOL OnCallTransferIdentify(
      H323Connection & connection    /// Connection to transfer
    );

    /**Call back for answering an incoming call.
       This function is called from the OnReceivedSignalSetup() function
       before it sends the Connect PDU. It gives an opportunity for an
       application to alter the reply before transmission to the other
       endpoint.

       It also gives an application time to wait for some event before
       signalling to the endpoint that the connection is to proceed. For
       example the user pressing an "Answer call" button.

       If AnswerCallDenied is returned the connection is aborted and a Release
       Complete PDU is sent. If AnswerCallNow is returned then the H.323
       protocol proceeds. Finally if AnswerCallPending is returned then the
       protocol negotiations are paused until the AnsweringCall() function is
       called.

       The default behaviour simply returns AnswerNow.
     */
    virtual H323Connection::AnswerCallResponse OnAnswerCall(
      H323Connection & connection,    /// Connection that was established
      const PString & callerName,       /// Name of caller
      const H323SignalPDU & setupPDU,   /// Received setup PDU
      H323SignalPDU & connectPDU        /// Connect PDU to send. 
    );

    /**Call back for remote party being alerted.
       This function is called from the SendSignalSetup() function after it
       receives the optional Alerting PDU from the remote endpoint. That is
       when the remote "phone" is "ringing".

       If FALSE is returned the connection is aborted and a Release Complete
       PDU is sent.

       The default behaviour simply returns TRUE.
     */
    virtual BOOL OnAlerting(
      H323Connection & connection,    /// Connection that was established
      const H323SignalPDU & alertingPDU,  /// Received Alerting PDU
      const PString & user                /// Username of remote endpoint
    );

    /**A call back function whenever connection indicates it should be
       forwarded.
       Return TRUE if the call is being redirected.

       The default behaviour simply returns FALSE and does not forward the
       call. The H323 application needs to handle this call back so it can
       call MakeCall() and know the token of the new H323 connection.
      */
    virtual BOOL OnConnectionForwarded(
      H323Connection & connection,    /// Connection to be forwarded
      const PString & forwardParty,   /// Remote party to forward to
      const H323SignalPDU & pdu       /// Full PDU initiating forwarding
    );

    /**A call back function whenever a connection is established.
       This indicates that a connection to a remote endpoint was established
       with a control channel and zero or more logical channels.

       The default behaviour does nothing.
      */
    virtual void OnConnectionEstablished(
      H323Connection & connection,    /// Connection that was established
      const PString & token           /// Token for identifying connection
    );

    /**Determine if a connection is established.
      */
    virtual BOOL IsConnectionEstablished(
      const PString & token   /// Token for identifying connection
    );

    /**A call back function whenever a connection is broken.
       This indicates that a connection to a remote endpoint is no longer
       available.

       The default behaviour does nothing.
      */
    virtual void OnConnectionCleared(
      H323Connection & connection,    /// Connection that was established
      const PString & token           /// Token for identifying connection
    );
  //@}


  /**@name Logical Channels management */
  //@{
    /**Call back for opening a logical channel.

       The default behaviour simply returns TRUE.
      */
    virtual BOOL OnStartLogicalChannel(
      H323Connection & connection,    /// Connection for the channel
      H323Channel & channel           /// Channel being started
    );

    /**Call back for closed a logical channel.

       The default behaviour does nothing.
      */
    virtual void OnClosedLogicalChannel(
      H323Connection & connection,    /// Connection for the channel
      const H323Channel & channel     /// Channel being started
    );

    /**Callback from the RTP session for statistics monitoring.
       This is called every so many packets on the transmitter and receiver
       threads of the RTP session indicating that the statistics have been
       updated.

       The default behaviour does nothing.
      */
    virtual void OnRTPStatistics(
      const H323Connection & connection,  /// Connection for the channel
      const RTP_Session & session         /// Session with statistics
    ) const;
  //@}

  /**@name Member variable access */
  //@{
    /**Set the user name to be used for the local end of any connections. This
       defaults to the logged in user as obtained from the
       PProcess::GetUserName() function.

       Note that this name is technically the first alias for the endpoint.
       Additional aliases may be added by the use of the AddAliasName()
       function, however that list will be cleared when this function is used.
     */
    void SetLocalUserName(
      const PString & name  /// Local name of endpoint (prime alias)
    );

    /**Get the user name to be used for the local end of any connections. This
       defaults to the logged in user as obtained from the
       PProcess::GetUserName() function.
     */
    const PString & GetLocalUserName() const { return localAliasNames[0]; }

    /**Add an alias name to be used for the local end of any connections. If
       the alias name already exists in the list then is is not added again.

       The list defaults to the value set in the SetLocalUserName() function.
       Note that calling SetLocalUserName() will clear the alias list.
     */
    BOOL AddAliasName(
      const PString & name  /// New alias name to add
    );

    /**Remove an alias name used for the local end of any connections. 
       defaults to an empty list.
     */
    BOOL RemoveAliasName(
      const PString & name  /// New alias namer to add
    );

    /**Get the user name to be used for the local end of any connections. This
       defaults to the logged in user as obtained from the
       PProcess::GetUserName() function.
     */
    const PStringList & GetAliasNames() const { return localAliasNames; }

    /**Get the default fast start mode.
      */
    BOOL IsFastStartDisabled() const
      { return disableFastStart; }

    /**Set the default fast start mode.
      */
    void DisableFastStart(
      BOOL mode /// New default mode
    ) { disableFastStart = mode; } 

    /**Get the default H.245 tunneling mode.
      */
    BOOL IsH245TunnelingDisabled() const
      { return disableH245Tunneling; }

    /**Set the default H.245 tunneling mode.
      */
    void DisableH245Tunneling(
      BOOL mode /// New default mode
    ) { disableH245Tunneling = mode; } 

    /**Get the default H.245 tunneling mode.
      */
    BOOL IsH245inSetupDisabled() const
      { return disableH245inSetup; }

    /**Set the default H.245 tunneling mode.
      */
    void DisableH245inSetup(
      BOOL mode /// New default mode
    ) { disableH245inSetup = mode; } 

    /**Get Call Intrusion Protection Level of the end point.
      */
    unsigned GetCallIntrusionProtectionLevel() const { return callIntrusionProtectionLevel; }

    /**Set Call Intrusion Protection Level of the end point.
      */
    void SetCallIntrusionProtectionLevel(
      unsigned level  // New level from 0 to 3
    ) { PAssert(level<=3, PInvalidParameter); callIntrusionProtectionLevel = level; }

    /**Get the default mode for sending User Input Indications.
      */
    H323Connection::SendUserInputModes GetSendUserInputMode() const { return defaultSendUserInputMode; }

    /**Set the default mode for sending User Input Indications.
      */
    void SetSendUserInputMode(H323Connection::SendUserInputModes mode) { defaultSendUserInputMode = mode; }

    /**See if should auto-start receive video channels on connection.
     */
    BOOL CanAutoStartReceiveVideo() const { return manager.CanAutoStartReceiveVideo(); }

    /**See if should auto-start transmit video channels on connection.
     */
    BOOL CanAutoStartTransmitVideo() const { return manager.CanAutoStartTransmitVideo(); }

    /**See if should auto-start receive fax channels on connection.
     */
    BOOL CanAutoStartReceiveFax() const { return autoStartReceiveFax; }

    /**See if should auto-start transmit fax channels on connection.
     */
    BOOL CanAutoStartTransmitFax() const { return autoStartTransmitFax; }

    /**Get the current capability table for this endpoint.
     */
    const H323Capabilities & GetCapabilities() const { return capabilities; }

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
    TerminalTypes GetTerminalType() const { return terminalType; }

    /**Determine if endpoint is terminal type.
     */
    BOOL IsTerminal() const;

    /**Determine if endpoint is gateway type.
     */
    BOOL IsGateway() const;

    /**Determine if endpoint is gatekeeper type.
     */
    BOOL IsGatekeeper() const;

    /**Determine if endpoint is gatekeeper type.
     */
    BOOL IsMCU() const;

    /**Get the default timeout for calling another endpoint.
     */
    const PTimeInterval & GetSignallingChannelCallTimeout() const { return signallingChannelCallTimeout; }

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
    BOOL ShouldClearCallOnRoundTripFail() const { return clearCallOnRoundTripFail; }

    /**Get the amount of time with no media that should cause call to clear
     */
    const PTimeInterval & GetNoMediaTimeout() const { return noMediaTimeout; }

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

    /**Get the iNow Gatekeeper Access Token OID.
     */
    const PString & GetGkAccessTokenOID() const { return gkAccessTokenOID; }

    /**Set the iNow Gatekeeper Access Token OID.
     */
    void SetGkAccessTokenOID(const PString & token) { gkAccessTokenOID = token; }

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

    /**Get the dictionary of <callIdentities, connections>
     */
    H323CallIdentityDict& GetCallIdentityDictionary() { return secondaryConenctionsActive; }
  //@}


  protected:
    H323Gatekeeper * InternalCreateGatekeeper(OpalTransport * transport);
    BOOL InternalRegisterGatekeeper(H323Gatekeeper * gk, BOOL discovered);
    H323Connection * FindConnectionWithoutLocks(const PString & token);
    BOOL InternalMakeCall(
      OpalCall & call,
      const PString & existingToken,/// Existing connection to be transferred
      const PString & callIdentity, /// Call identity of the secondary call (if it exists)
      unsigned capabilityLevel,     /// Intrusion capability level
      const PString & remoteParty,  /// Remote party to call
      void * userData               /// user data to pass to CreateConnection
    );

    // Configuration variables, commonly changed
    PStringList localAliasNames;
    BOOL        autoStartReceiveFax;
    BOOL        autoStartTransmitFax;
    BOOL        disableFastStart;
    BOOL        disableH245Tunneling;
    BOOL        disableH245inSetup;
    unsigned    callIntrusionProtectionLevel;
    H323Connection::SendUserInputModes defaultSendUserInputMode;

    BYTE          t35CountryCode;
    BYTE          t35Extension;
    WORD          manufacturerCode;
    TerminalTypes terminalType;

    BOOL     clearCallOnRoundTripFail;

    // Some more configuration variables, rarely changed.
    PTimeInterval signallingChannelCallTimeout;
    PTimeInterval endSessionTimeout;
    PTimeInterval masterSlaveDeterminationTimeout;
    unsigned      masterSlaveDeterminationRetries;
    PTimeInterval capabilityExchangeTimeout;
    PTimeInterval logicalChannelTimeout;
    PTimeInterval requestModeTimeout;
    PTimeInterval roundTripDelayTimeout;
    PTimeInterval roundTripDelayRate;
    PTimeInterval noMediaTimeout;
    PTimeInterval gatekeeperRequestTimeout;
    unsigned      gatekeeperRequestRetries;
    PTimeInterval rasRequestTimeout;
    unsigned      rasRequestRetries;
    PTimeInterval registrationTimeToLive;

    PString       gkAccessTokenOID;

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
    H323Capabilities     capabilities;
    H323Gatekeeper *     gatekeeper;
    PString              gatekeeperPassword;
    H323CallIdentityDict secondaryConenctionsActive;
};


#endif // __OPAL_H323EP_H


/////////////////////////////////////////////////////////////////////////////
