/*
 * h323con.h
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
 * $Log: h323con.h,v $
 * Revision 1.2004  2001/08/22 09:42:14  robertj
 * Removed duplicate member variables moved to ancestor.
 *
 * Revision 2.2  2001/08/17 08:21:15  robertj
 * Update from OpenH323
 * Moved call end reasons enum from OpalConnection to global.
 *
 * Revision 2.1  2001/08/01 05:11:04  robertj
 * Moved media formats list from endpoint to connection.
 *
 * Revision 2.0  2001/07/27 15:48:24  robertj
 * Conversion of OpenH323 to Open Phone Abstraction Library (OPAL)
 * Revision 1.2  2001/08/16 07:49:16  robertj
 * Changed the H.450 support to be more extensible. Protocol handlers
 *   are now in separate classes instead of all in H323Connection.
 *
 * Revision 1.1  2001/08/06 03:08:11  robertj
 * Fission of h323.h to h323ep.h & h323con.h, h323.h now just includes files.
 */

#ifndef __H323_H323CON_H
#define __H323_H323CON_H

#ifdef __GNUC__
#pragma interface
#endif


#include <opal/connection.h>
#include <opal/guid.h>
#include <h323/h323caps.h>


/* The following classes have forward references to avoid including the VERY
   large header files for H225 and H245. If an application requires access
   to the protocol classes they can include them, but for simple usage their
   inclusion can be avoided.
 */
class PPER_Stream;
class PASN_OctetString;

class H225_EndpointType;
class H225_TransportAddress;
class H225_ArrayOf_PASN_OctetString;

class H245_TerminalCapabilitySet;
class H245_TerminalCapabilitySetReject;
class H245_OpenLogicalChannel;
class H245_OpenLogicalChannelAck;
class H245_TransportAddress;
class H245_UserInputIndication;
class H245_RequestMode;
class H245_RequestModeAck;
class H245_RequestModeReject;
class H245_SendTerminalCapabilitySet;
class H245_MultiplexCapability;
class H245_FlowControlCommand;
class H245_MiscellaneousCommand;
class H245_MiscellaneousIndication;
class H245_JitterIndication;

class H323SignalPDU;
class H323ControlPDU;
class H323EndPoint;
class H323TransportAddress;

class H245NegMasterSlaveDetermination;
class H245NegTerminalCapabilitySet;
class H245NegLogicalChannels;
class H245NegRequestMode;
class H245NegRoundTripDelay;

class H450xDispatcher;
class H4502Handler;
class H4504Handler;

class OpalCall;


///////////////////////////////////////////////////////////////////////////////

/**This class represents a particular H323 connection between two endpoints.
   There are at least two threads in use, this one to look after the
   signalling channel, an another to look after the control channel. There
   would then be additional threads created for each data channel created by
   the control channel protocol thread.
 */
class H323Connection : public OpalConnection
{
  PCLASSINFO(H323Connection, OpalConnection);

  public:
  /**@name Construction */
  //@{
    /**Create a new connection.
     */
    H323Connection(
      OpalCall & call,                /// Call object connection belongs to
      H323EndPoint & endpoint,        /// H323 End Point object
      const PString & token,          /// Token for new connection
      BOOL disableFastStart = FALSE,  /// Flag to prevent fast start operation
      BOOL disableTunneling = FALSE   /// Flag to prevent H.245 tunneling
    );

    /**Destroy the connection
     */
    ~H323Connection();
  //@}

  /**@name Overrides from OpalConnection */
  //@{
    /**Get the phase of the connection.
       This indicates the current phase of the connection sequence. Whether
       all phases and the transitions between phases is protocol dependent.

       The default bahaviour is pure.
      */
    virtual Phases GetPhase() const;

    /**Indicate to remote endpoint an alert is in progress.
       If this is an incoming connection and the AnswerCallResponse is in a
       AnswerCallDeferred or AnswerCallPending state, then this function is
       used to indicate to that endpoint that an alert is in progress. This is
       usually due to another connection which is in the call (the B party)
       has received an OnAlerting() indicating that its remoteendpoint is
       "ringing".

       The default behaviour sends an ALERTING pdu.
      */
    virtual BOOL SetAlerting(
      const PString & calleeName    /// Name of endpoint being alerted.
    );

    /**Indicate to remote endpoint we are connected.

       The default behaviour sends a CONNECT pdu.
      */
    virtual BOOL SetConnected();

    /** Called when a connection is established.
        Defautl behaviour is to call H323Connection::OnConnectionEstablished
      */
    virtual void OnEstablished();

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

       The default behaviour calls CleanUpOnCallEnd() then calls the ancestor.
      */
    virtual BOOL OnReleased();

    /**Get the destination address of an incoming connection.
       This will, for example, collect a phone number from a POTS line, or
       get the fields from the H.225 SETUP pdu in a H.323 connection.
      */
    virtual PString GetDestinationAddress();

    /**Get the data formats this connection is capable of operating.
       This provides a list of media data format names that an
       OpalMediaStream may be created in within this connection.

       The default behaviour returns media data format names contained in
       the remote capability table.
      */
    virtual OpalMediaFormatList GetMediaFormats() const;

    /**Open source transmitter media stream for session.
      */
    virtual BOOL OpenSourceMediaStream(
      const OpalMediaFormatList & mediaFormats, /// Optional media format to open
      unsigned sessionID                   /// Session to start stream on
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
      BOOL isSource,      /// Is a source stream
      unsigned sessionID  /// Session number for stream
    );
  //@}

  /**@name Backward compatibility functions */
  //@{
    /** Called when a connection is cleared, just after CleanUpOnCallEnd()
        Default behaviour is to call H323Connection::OnConnectionCleared
      */
    virtual void OnCleared();

    /**Determine if the call has been established.
       This can be used in combination with the GetCallEndReason() function
       to determine the three main phases of a call, call setup, call
       established and call cleared.
      */
    BOOL IsEstablished() const { return connectionState == EstablishedConnection; }

    /**Inernal function to check if call established.
       This checks all the criteria for establishing a call an initiating the
       starting of media channels, if they have not already been started vi
       the fast start algorithm.
    */
    virtual void InternalEstablishedConnectionCheck();

    /**Clean up the call clearance of the connection.
       This function will do any internal cleaning up and waiting on background
       threads that may be using the connection object. After this returns it
       is then safe to delete the object.

       An application will not typically use this function as it is used by the
       H323EndPoint during a clear call.
      */
    virtual void CleanUpOnCallEnd();
  //@}


  /**@name Signalling Channel */
  //@{
    /**Attach a transport to this connection as the signalling channel.
      */
    void AttachSignalChannel(
      OpalTransport * channel,  /// Transport for the PDU's
      BOOL answeringCall        /// Flag for if incoming/outgoing call.
    );

    /**Write a PDU to the signalling channel.
      */
    BOOL WriteSignalPDU(
      H323SignalPDU & pdu       /// PDU to write.
    );

    /* Handle reading PDU's from the signalling channel.
     */
    void HandleSignallingChannel();

    /* Handle PDU from the signalling channel.
     */
    virtual BOOL HandleSignalPDU(
      H323SignalPDU & pdu       /// PDU to handle.
    );

    /* Handle Control PDU tunnelled in the signalling channel.
     */
    virtual void HandleTunnelPDU(
      const H323SignalPDU & pdu,    /// PDU to handle.
      H323SignalPDU * reply         /// Optional PDU to piggy back replies
    );

    /**Handle an incoming Q931 setup PDU.
       The default behaviour is to do the handshaking operation calling a few
       virtuals at certain moments in the sequence.

       If FALSE is returned the connection is aborted and a Release Complete
       PDU is sent.
     */
    virtual BOOL OnReceivedSignalSetup(
      const H323SignalPDU & pdu   /// Received setup PDU
    );

    /**Handle an incoming Q931 call proceeding PDU.
       If FALSE is returned the connection is aborted and a Release Complete
       PDU is sent.

       The default behaviour checks for hH245Address field and if present
       starts the separate H245 channel, if successful or not present it
       returns TRUE.
     */
    virtual BOOL OnReceivedCallProceeding(
      const H323SignalPDU & pdu   /// Received call proceeding PDU
    );

    /**Handle an incoming Q931 progress PDU.
       If FALSE is returned the connection is aborted and a Release Complete
       PDU is sent.

       The default behaviour checks for hH245Address field and if present
       starts the separate H245 channel, if successful or not present it
       returns TRUE.
     */
    virtual BOOL OnReceivedProgress(
      const H323SignalPDU & pdu   /// Received call proceeding PDU
    );

    /**Handle an incoming Q931 alerting PDU.
       If FALSE is returned the connection is aborted and a Release Complete
       PDU is sent.

       The default behaviour obtains the display name and calls OnAlerting().
     */
    virtual BOOL OnReceivedAlerting(
      const H323SignalPDU & pdu   /// Received connect PDU
    );

    /**Handle an incoming Q931 connect PDU.
       If FALSE is returned the connection is aborted and a Release Complete
       PDU is sent.

       The default behaviour checks for hH245Address field and if present
       starts the separate H245 channel, if successful it returns TRUE.
       If not present and there is no H245Tunneling then it returns FALSE.
     */
    virtual BOOL OnReceivedSignalConnect(
      const H323SignalPDU & pdu   /// Received connect PDU
    );

    /**Handle an incoming Q931 facility PDU.
       If FALSE is returned the connection is aborted and a Release Complete
       PDU is sent.

       The default behaviour checks for hH245Address field and if present
       starts the separate H245 channel, if successful or not present it
       returns TRUE.
     */
    virtual BOOL OnReceivedFacility(
      const H323SignalPDU & pdu   /// Received connect PDU
    );

    /**Handle an incoming Q931 Status Enquiry PDU.
       If FALSE is returned the connection is aborted and a Release Complete
       PDU is sent.

       The default behaviour sends a Q931 Status PDU back.
     */
    virtual BOOL OnReceivedStatusEnquiry(
      const H323SignalPDU & pdu   /// Received connect PDU
    );

    /**Handle an incoming Q931 Release Complete PDU.
       The default behaviour calls Clear() using reason code based on the
       Release Complete Cause field and the current connection state.
     */
    virtual void OnReceivedReleaseComplete(
      const H323SignalPDU & pdu   /// Received connect PDU
    );

    /**This function is called from the HandleSignallingChannel() function
       for unhandled PDU types.

       If FALSE is returned the connection is aborted and a Release Complete
       PDU is sent. The default behaviour returns TRUE.
     */
    virtual BOOL OnUnknownSignalPDU(
      const H323SignalPDU & pdu  /// Received PDU
    );

    /**Call back for incoming call.
       This function is called from the OnReceivedSignalSetup() function
       before it sends the Alerting PDU. It gives an opportunity for an
       application to alter the reply before transmission to the other
       endpoint.

       If FALSE is returned the connection is aborted and a Release Complete
       PDU is sent.

       The default behaviour calls the endpoint function of the same name.
     */
    virtual BOOL OnIncomingCall(
      const H323SignalPDU & setupPDU,   /// Received setup PDU
      H323SignalPDU & alertingPDU       /// Alerting PDU to send
    );

    /**Forward incoming call to specified address.
       This would typically be called from within the OnIncomingCall()
       function when an application wishes to redirct an unwanted incoming
       call.

       The return value is TRUE if the call is to be forwarded, FALSE
       otherwise. Note that if the call is forwarded the current connection is
       cleared with teh ended call code of EndedByCallForwarded.
      */
    virtual BOOL ForwardCall(
      const PString & forwardParty   /// Party to forward call to.
    );

    /**Initiate the transfer of an existing call (connection) to a new remote party
       using H.450.2.  This sends a Call Transfer Initiate Invoke message from the
       A-Party (transferring endpoint) to the B-Party (transferred endpoint).
     */
    void TransferCall(
      const PString & remoteParty   /// Remote party to transfer the existing call to
    );

    /**Determine whether this connection is being transferred.
     */
    BOOL IsTransferringCall() const;

    /**Determine whether this connection is the result of a transferred call.
     */
    BOOL IsTransferredCall() const;

    /**Handle the transfer of an existing connection to a new remote.
       This is an internal function and it is not expected the user will call
       it directly.
     */
    void HandleTransferCall(
      const PString & token,
      const PString & identity
    );

    /**Get transfer invoke ID dureing trasfer.
       This is an internal function and it is not expected the user will call
       it directly.
      */
    int GetCallTransferInvokeId();

    /**Place the call on hold, suspending all media channels (H.450.4)
    * NOTE: Only Local Hold is implemented so far. 
    */
    void HoldCall(
      BOOL localHold   /// true for Local Hold, false for Remote Hold
    );

    /**Retrieve the call from hold, activating all media channels (H.450.4)
    * NOTE: Only Local Hold is implemented so far. 
    */
    void RetrieveCall(
      bool localHold						  /// true for Local Hold, false for Remote Hold
    );

    /**Determine if held.
      */
    BOOL IsLocalHold() const;

    /**Determine if held.
      */
    BOOL IsRemoteHold() const;


    /**Call back for answering an incoming call.
       This function is used for an application to control the answering of
       incoming calls. It is usually used to indicate the immediate action to
       be taken in answering the call.

       It is called from the OnReceivedSignalSetup() function before it sends
       the Alerting or Connect PDUs. It also gives an opportunity for an
       application to alter the Connect PDU reply before transmission to the
       remote endpoint.

       If AnswerCallNow is returned then the H.323 protocol proceeds with the
       connection. If AnswerCallDenied is returned the connection is aborted
       and a Release Complete PDU is sent. If AnswerCallPending is returned
       then the Alerting PDU is sent and the protocol negotiations are paused
       until the AnsweringCall() function is called. Finally, if
       AnswerCallDeferred is returned then no Alerting PDU is sent, but the
       system still waits as in the AnswerCallPending response.

       Note this function should not block for any length of time. If the
       decision to answer the call may take some time eg waiting for a user to
       pick up the phone, then AnswerCallPending or AnswerCallDeferred should
       be returned.

       The default behaviour calls the endpoint function of the same name
       which in turn will return AnswerCallNow.
     */
    virtual AnswerCallResponse OnAnswerCall(
      const PString & callerName,       /// Name of caller
      const H323SignalPDU & setupPDU,   /// Received setup PDU
      H323SignalPDU & connectPDU        /// Connect PDU to send. 
    );

    /**Indicate the result of answering an incoming call.
       This should only be called if the OnAnswerCall() callback function has
       returned a AnswerCallPending or AnswerCallDeferred response.

       Note sending further AnswerCallPending responses via this function will
       have the result of an Alerting PDU being sent to the remote endpoint.
       In this way multiple Alerting PDUs may be sent.

       Sending a AnswerCallDeferred response would have no effect.
      */
    void AnsweringCall(
      AnswerCallResponse response /// Answer response to incoming call
    );

    /**Send first PDU in signalling channel.
       This function does the signalling handshaking for establishing a
       connection to a remote endpoint. The transport (TCP/IP) for the
       signalling channel is assumed to be already created. This function
       will then do the SetRemoteAddress() and Connect() calls o establish
       the transport.

       Returns the error code for the call failure reason or NumCallEndReasons
       if the call was successful to that point in the protocol.
     */
    virtual OpalCallEndReason SendSignalSetup(
      const PString & alias,                /// Name of remote party
      const H323TransportAddress & address  /// Address of destination
    );

    /**Adjust setup PDU being sent on initialisation of signal channel.
       This function is called from the SendSignalSetup() function before it
       sends the Setup PDU. It gives an opportunity for an application to
       alter the request before transmission to the other endpoint.

       The default behaviour simply returns TRUE. Note that this is usually
       overridden by the transport dependent descendent class, eg the
       H323ConnectionTCP descendent fills in the destCallSignalAddress field
       with the TCP/IP data. Therefore if you override this in your
       application make sure you call the ancestor function.
     */
    virtual BOOL OnSendSignalSetup(
      H323SignalPDU & setupPDU   /// Setup PDU to send
    );

    /**Adjust call proceeding PDU being sent. This function is called from
       the OnReceivedSignalSetup() function before it sends the Call
       Proceeding PDU. It gives an opportunity for an application to alter
       the request before transmission to the other endpoint. If this function
       returns FALSE then the Call Proceeding PDU is not sent at all.

       The default behaviour simply returns TRUE.
     */
    virtual BOOL OnSendCallProceeding(
      H323SignalPDU & callProceedingPDU   /// Call Proceeding PDU to send
    );

    /**Call back for remote party being alerted.
       This function is called from the SendSignalSetup() function after it
       receives the optional Alerting PDU from the remote endpoint. That is
       when the remote "phone" is "ringing".

       If FALSE is returned the connection is aborted and a Release Complete
       PDU is sent.

       The default behaviour calls the endpoint function of the same name.
     */
    virtual BOOL OnAlerting(
      const H323SignalPDU & alertingPDU,  /// Received Alerting PDU
      const PString & user                /// Username of remote endpoint
    );

    /**This function is called from the SendSignalSetup() function after it
       receives the Connect PDU from the remote endpoint, but before it
       attempts to open the control channel.

       If FALSE is returned the connection is aborted and a Release Complete
       PDU is sent.

       The default behaviour simply returns TRUE.
     */
    virtual BOOL OnOutgoingCall(
      const H323SignalPDU & connectPDU   /// Received Connect PDU
    );

    /**Send an the acknowldege of a fast start.
       This function is called when the fast start channels provided to this
       connection by the original SETUP PDU have been selected and opened and
       need to be sent back to the remote endpoint.

       If FALSE is returned then no fast start has been acknowledged, possibly
       due to no common codec in fast start request.

       The default behaviour uses OnSelectLogicalChannels() to find a pair of
       channels and adds then to the provided PDU.
     */
    virtual BOOL SendFastStartAcknowledge(
      H225_ArrayOf_PASN_OctetString & array   /// Array of H245_OpenLogicalChannel
    );

    /**Handle the acknowldege of a fast start.
       This function is called from one of a number of functions after it
       receives a PDU from the remote endpoint that has a fastStart field. It
       is in response to a request for a fast strart from the local endpoint.

       If FALSE is returned the connection is aborted and a Release Complete
       PDU is sent.

       The default behaviour parses the provided array and starts the channels
       acknowledged in it.
     */
    virtual BOOL HandleFastStartAcknowledge(
      const H225_ArrayOf_PASN_OctetString & array   /// Array of H245_OpenLogicalChannel
    );
  //@}

  /**@name Control Channel */
  //@{
    /**Start a separate H245 channel.
       This function is called from one of a number of functions after it
       receives a PDU from the remote endpoint that has a h245Address field.

       If FALSE is returned the connection is aborted and a Release Complete
       PDU is sent.

       The default behaviour checks to see if it is a known transport and
       creates a corresponding OpalTransport decendent for the control
       channel.
     */
    virtual BOOL CreateOutgoingControlChannel(
      const H225_TransportAddress & h245Address   /// H245 address
    );

    /**Start a separate control channel.
       This function is called from one of a number of functions when it needs
       to listen for an incoming H.245 connection.


       If FALSE is returned the connection is aborted and a Release Complete
       PDU is sent.

       The default behaviour checks to see if it is a known transport and
       creates a corresponding OpalTransport decendent for the control
       channel.
      */
    virtual BOOL CreateIncomingControlChannel(
      H225_TransportAddress & h245Address  /// PDU transport address to set
    );

    PDECLARE_NOTIFIER(PThread, H323Connection, NewIncomingControlChannel);

    /**Write a PDU to the control channel.
       If there is no control channel open then this will tunnel the PDU
       into the signalling channel.
      */
    BOOL WriteControlPDU(
      const H323ControlPDU & pdu
    );

    /**Start control channel negotiations.
      */
    BOOL StartControlNegotiations();

    /**Handle reading data on the control channel.
     */
    virtual void HandleControlChannel();

    /**Handle incoming data on the control channel.
       This decodes the data stream into a PDU and calls HandleControlPDU().

       If FALSE is returned the connection is aborted. The default behaviour
       returns TRUE.
     */
    virtual BOOL HandleControlData(
      PPER_Stream & strm
    );

    /**Handle incoming PDU's on the control channel. Dispatches them to the
       various virtuals off this class.

       If FALSE is returned the connection is aborted. The default behaviour
       returns TRUE.
     */
    virtual BOOL HandleControlPDU(
      const H323ControlPDU & pdu
    );

    /**This function is called from the HandleControlPDU() function
       for unhandled PDU types.

       If FALSE is returned the connection is aborted and a Release Complete
       PDU is sent. The default behaviour returns TRUE.

       The default behaviour send a FunctioNotUnderstood indication back to
       the sender, and returns TRUE to continue operation.
     */
    virtual BOOL OnUnknownControlPDU(
      const H323ControlPDU & pdu  /// Received PDU
    );

    /**Handle incoming request PDU's on the control channel.
       Dispatches them to the various virtuals off this class.
     */
    virtual BOOL OnH245Request(
      const H323ControlPDU & pdu  /// Received PDU
    );

    /**Handle incoming response PDU's on the control channel.
       Dispatches them to the various virtuals off this class.
     */
    virtual BOOL OnH245Response(
      const H323ControlPDU & pdu  /// Received PDU
    );

    /**Handle incoming command PDU's on the control channel.
       Dispatches them to the various virtuals off this class.
     */
    virtual BOOL OnH245Command(
      const H323ControlPDU & pdu  /// Received PDU
    );

    /**Handle incoming indication PDU's on the control channel.
       Dispatches them to the various virtuals off this class.
     */
    virtual BOOL OnH245Indication(
      const H323ControlPDU & pdu  /// Received PDU
    );

    /**Handle H245 command to send terminal capability set.
     */
    virtual BOOL OnH245_SendTerminalCapabilitySet(
      const H245_SendTerminalCapabilitySet & pdu  /// Received PDU
    );

    /**Handle H245 command to control flow control.
       This function calls OnLogicalChannelFlowControl() with the channel and
       bit rate restriction.
     */
    virtual BOOL OnH245_FlowControlCommand(
      const H245_FlowControlCommand & pdu  /// Received PDU
    );

    /**Handle H245 miscellaneous command.
       This function passes the miscellaneous command on to the channel
       defined by the pdu.
     */
    virtual BOOL OnH245_MiscellaneousCommand(
      const H245_MiscellaneousCommand & pdu  /// Received PDU
    );

    /**Handle H245 miscellaneous indication.
       This function passes the miscellaneous indication on to the channel
       defined by the pdu.
     */
    virtual BOOL OnH245_MiscellaneousIndication(
      const H245_MiscellaneousIndication & pdu  /// Received PDU
    );

    /**Handle H245 indication of received jitter.
       This function calls OnLogicalChannelJitter() with the channel and
       estimated jitter.
     */
    virtual BOOL OnH245_JitterIndication(
      const H245_JitterIndication & pdu  /// Received PDU
    );

    /**Error discriminator for the OnControlProtocolError() function.
      */
    enum ControlProtocolErrors {
      e_MasterSlaveDetermination,
      e_CapabilityExchange,
      e_LogicalChannel,
      e_ModeRequest,
      e_RoundTripDelay
    };

    /**This function is called from the HandleControlPDU() function or
       any of its sub-functions for protocol errors, eg unhandled PDU types.

       The errorData field may be a string or PDU or some other data depending
       on the value of the errorSource parameter. These are:
          e_UnhandledPDU                    &H323ControlPDU
          e_MasterSlaveDetermination        const char *

       If FALSE is returned the connection is aborted. The default behaviour
       returns TRUE.
     */
    virtual BOOL OnControlProtocolError(
      ControlProtocolErrors errorSource,  // Source of the proptoerror
      const void * errorData = NULL       // Data associated with error
    );

    /**This function is called from the HandleControlPDU() function when
       it is about to send the Capabilities Set to the remote endpoint. This
       gives the application an oppurtunity to alter the PDU to be sent.

       The default behaviour will make "adjustments" for compatibiliry with
       some broken remote endpoints.
     */
    virtual void OnSendCapabilitySet(
      H245_TerminalCapabilitySet & pdu  /// PDU to send
    );

    /**This function is called when the remote endpoint sends its capability
       set. This gives the application an opportunity to determine what codecs
       are available and if it supports any of the combinations of codecs.

       Note any codec types that are the remote system supports that are not in
       the codecs list member variable for the endpoint are ignored and not
       included in the remoteCodecs list.

       The default behaviour assigns the table and set to member variables and
       returns TRUE if the remoteCodecs list is not empty.
     */
    virtual BOOL OnReceivedCapabilitySet(
      const H323Capabilities & remoteCaps,      /// Capability combinations remote supports
      const H245_MultiplexCapability * muxCap,  /// Transport capability, if present
      H245_TerminalCapabilitySetReject & reject /// Rejection PDU (if return FALSE)
    );

    /**Send a new capability set.
      */
    virtual void SendCapabilitySet(
      BOOL empty  /// Send an empty set.
    );

    /**Call back to set the local capabilities.
       This is called just before the capabilties are required when a call
       is begun. It is called when a SETUP PDU is received or when one is
       about to be sent, so that the capabilities may be adjusted for correct
       fast start operation.

       The default behaviour adds all media formats.
      */
    virtual void OnSetLocalCapabilities();

    /**Return if this H245 connection is a master or slave
     */
    BOOL IsH245Master() const;

    /**Start the round trip delay calculation over the control channel.
     */
    void StartRoundTripDelay();

    /**Get the round trip delay over the control channel.
     */
    PTimeInterval GetRoundTripDelay() const;
  //@}

  /**@name Logical Channel Management */
  //@{
    /**Call back to select logical channels to start.

       This function must be defined by the descendent class. It is used
       to select the logical channels to be opened between the two endpoints.
       There are three ways in which this may be called: when a "fast start"
       has been initiated by the local endpoint (via SendSignalSetup()
       function), when a "fast start" has been requested from the remote
       endpoint (via the OnReceivedSignalSetup() function) or when the H245
       capability set (and master/slave) negotiations have completed (via the
       OnControlChannelOpen() function.

       The function would typically examine several member variable to decide
       which mode it is being called in and what to do. If fastStartState is
       FastStartDisabled then non-fast start semantics should be used. The
       H245 capabilities in the remoteCapabilities members should be
       examined, and appropriate transmit channels started using
       OpenLogicalChannel().

       If fastStartState is FastStartInitiate, then the local endpoint has
       initiated a call and is asking the application if fast start semantics
       are to be used. If so it is expected that the function call 
       OpenLogicalChannel() for all the channels that it wishes to be able to
       be use. A subset (possibly none!) of these would actually be started
       when the remote endpoint replies.

       If fastStartState is FastStartResponse, then this indicates the remote
       endpoint is attempting a fast start. The fastStartChannels member
       contains a list of possible channels from the remote that the local
       endpoint is to select which to accept. For each accepted channel it
       simply necessary to call the Start() function on that channel eg
       fastStartChannels[0].Start();

       The default behaviour selects the first codec of each session number
       that is available. This is according to the order of the capabilities
       in the remoteCapabilities, the local capability table or of the
       fastStartChannels list respectively for each of the above scenarios.
      */
    virtual void OnSelectLogicalChannels();

    /**Select default logical channel for normal start.
      */
    virtual void SelectDefaultLogicalChannel(
      unsigned sessionID    /// Session ID to find default logical channel.
    );

    /**Select default logical channel for fast start.
       Internal function, not for normal use.
      */
    virtual void SelectFastStartChannels(
      unsigned sessionID,   /// Session ID to find default logical channel.
      BOOL transmitter,     /// Whether to open transmitters
      BOOL receiver         /// Whether to open receivers
    );

    /**Start a logical channel for fast start.
       Internal function, not for normal use.
      */
    virtual void StartFastStartChannel(
      unsigned sessionID,               /// Session ID to find logical channel.
      H323Channel::Directions direction /// Direction of channel to start
    );

    /**Open a new logical channel.
       This function will open a channel between the endpoints for the
       specified capability.

       If this function is called while there is not yet a conenction
       established, eg from the OnFastStartLogicalChannels() function, then
       a "trial" receiver/transmitter channel is created. This channel is not
       started until the remote enpoint has confirmed that they are to start.
       Any channels not confirmed are deleted.

       If this function is called later in the call sequence, eg from
       OnSelectLogicalChannels(), then it may only establish a transmit
       channel, ie fromRemote must be FALSE.
      */
    virtual BOOL OpenLogicalChannel(
      const H323Capability & capability,  /// Capability to open channel with
      unsigned sessionID,                 /// Session for the channel
      H323Channel::Directions dir         /// Direction of channel
    );

    /**This function is called when the remote endpoint want's to open
       a new channel.

       If the return value is FALSE then the open is rejected using the
       errorCode as the cause, this would be a value from the enum
       H245_OpenLogicalChannelReject_cause::Choices.

       The default behaviour simply returns TRUE.
     */
    virtual BOOL OnOpenLogicalChannel(
      const H245_OpenLogicalChannel & openPDU,  /// Received PDU for the channel open
      H245_OpenLogicalChannelAck & ackPDU,      /// PDU to send for acknowledgement
      unsigned & errorCode                      /// Error to return if refused
    );

    /**Callback for when a logical channel conflict has occurred.
       This is called when the remote endpoint, which is a master, rejects
       our transmitter channel due to a resource conflict. Typically an
       inability to do asymmetric codecs. The local (slave) endpoint must then
       try and open a new transmitter channel using the same codec as the
       receiver that is being opened.
      */
    virtual BOOL OnConflictingLogicalChannel(
      H323Channel & channel    /// Channel that conflicted
    );

    /**Create a new logical channel object.
       This is in response to a request from the remote endpoint to open a
       logical channel.
      */
    virtual H323Channel * CreateLogicalChannel(
      const H245_OpenLogicalChannel & open, /// Parameters for opening channel
      BOOL startingFast,                    /// Flag for fast/slow starting.
      unsigned & errorCode                  /// Reason for create failure
    );

    /**This function is called when the remote endpoint want's to create
       a new channel.

       If the return value is FALSE then the open is rejected using the
       errorCode as the cause, this would be a value from the enum
       H245_OpenLogicalChannelReject_cause::Choices.

       The default behaviour checks the capability set for if this capability
       is allowed to be opened with other channels that may already be open.
     */
    virtual BOOL OnCreateLogicalChannel(
      const H323Capability & capability,  /// Capability for the channel open
      H323Channel::Directions dir,        /// Direction of channel
      unsigned & errorCode                /// Error to return if refused
    );

    /**Call back function when a logical channel thread begins.

       The default behaviour does nothing and returns TRUE.
      */
    virtual BOOL OnStartLogicalChannel(
      H323Channel & channel    /// Channel that has been started.
    );

    /**Close a logical channel.
      */
    virtual void CloseLogicalChannel(
      unsigned number,    /// Channel number to close.
      BOOL fromRemote     /// Indicates close request of remote channel
    );

    /**Close a logical channel by number.
      */
    virtual void CloseLogicalChannelNumber(
      const H323ChannelNumber & number    /// Channel number to close.
    );

    /**This function is called when the remote endpoint has closed down
       a logical channel.

       The default behaviour does nothing.
     */
    virtual void OnClosedLogicalChannel(
      const H323Channel & channel   /// Channel that was closed
    );

    /**This function is called when the remote endpoint request the close of
       a logical channel.

       The application may get an opportunity to refuse to close the channel by
       returning FALSE from this function.

       The default behaviour returns TRUE.
     */
    virtual BOOL OnClosingLogicalChannel(
      H323Channel & channel   /// Channel that is to be closed
    );

    /**This function is called when the remote endpoint wishes to limit the
       bit rate being sent on a channel.

       If channel is NULL, then the bit rate limit applies to all channels.

       The default behaviour does nothing if channel is NULL, otherwise calls
       H323Channel::OnFlowControl() on the specific channel.
     */
    virtual void OnLogicalChannelFlowControl(
      H323Channel * channel,   /// Channel that is to be limited
      long bitRateRestriction  /// Limit for channel
    );

    /**This function is called when the remote endpoint indicates the level
       of jitter estimated by the receiver.

       If channel is NULL, then the jitter applies to all channels.

       The default behaviour does nothing if channel is NULL, otherwise calls
       H323Channel::OnJitter() on the specific channel.
     */
    virtual void OnLogicalChannelJitter(
      H323Channel * channel,   /// Channel that is to be limited
      DWORD jitter,            /// Estimated received jitter in microseconds
      int skippedFrameCount,   /// Frames skipped by decodec
      int additionalBuffer     /// Additional size of video decoder buffer
    );

    /**Get a logical channel.
       Locates the specified channel number and returns a pointer to it.
      */
    H323Channel * GetLogicalChannel(
      unsigned number,    /// Channel number to get.
      BOOL fromRemote     /// Indicates get a remote channel
    ) const;

    /**Find a logical channel.
       Locates a channel give a RTP session ID. Each session would usually
       have two logical channels associated with it, so the fromRemote flag
       bay be used to distinguish which channel to return.
      */
    H323Channel * FindChannel(
      unsigned sessionId,   /// Session ID to search for.
      BOOL fromRemote       /// Indicates the direction of RTP data.
    ) const;
  //@}

  /**@name Bandwidth Management */
  //@{
    /**Get the bandwidth currently used.
       This totals the open channels and returns the total bandwidth used in
       100's of bits/sec
      */
    unsigned GetBandwidthUsed() const;

    /**Request use the available bandwidth in 100's of bits/sec.
       If there is insufficient bandwidth available, FALSE is returned. If
       sufficient bandwidth is available, then TRUE is returned and the amount
       of available bandwidth is reduced by the specified amount.
      */
    BOOL UseBandwidth(
      unsigned bandwidth,     /// Bandwidth required
      BOOL removing           /// Flag for adding/removing bandwidth usage
    );

    /**Get the available bandwidth in 100's of bits/sec.
      */
    unsigned GetBandwidthAvailable() const { return bandwidthAvailable; }

    /**Set the available bandwidth in 100's of bits/sec.
       Note if the force parameter is TRUE this function will close down
       active logical channels to meet the new bandwidth requirement.
      */
    BOOL SetBandwidthAvailable(
      unsigned newBandwidth,    /// New bandwidth limit
      BOOL force = FALSE        /// Force bandwidth limit
    );
  //@}

  /**@name Indications */
  //@{
    /**Send a user input indication to the remote endpoint.
       The two forms are for basic user input of a simple string using the
       SendUserInput() function or a full DTMF emulation user input using the
       SendUserInputTone() function.

       An application could do more sophisticated usage by filling in the 
       H245_UserInputIndication structure directly ans using this function.
      */
    virtual void SendUserInputIndication(
      const H245_UserInputIndication & pdu    /// Full user indication PDU
    );

    /**Send a user input indication to the remote endpoint.
       This is for sending arbitrary strings as user indications. Simple DTMF
       tones can be sent this way with the tone value being a single
       character string.
      */
    virtual void SendUserInput(
      const PString & value                   /// String value of indication
    );

    /**Send a user input indication to the remote endpoint.
       This sends DTMF emulation user input. If something more sophisticated
       than the simple tones that can be sent using the SendUserInput()
       function.

       A duration of zero indicates that no duration is to be indicated.
       A non-zero logical channel indicates that the tone is to be syncronised
       with the logical channel at the rtpTimestamp value specified.

       The tone parameter must be one of "0123456789#*ABCD!" where '!'
       indicates a hook flash. If tone is a ' ' character then a
       signalUpdate PDU is sent that updates the last tone indication
       sent. See the H.245 specifcation for more details on this.
      */
    virtual void SendUserInputTone(
      char tone,                   /// DTMF tone code
      unsigned duration = 0,       /// Duration of tone in milliseconds
      unsigned logicalChannel = 0, /// Logical channel number for RTP sync.
      unsigned rtpTimestamp = 0    /// RTP timestamp in logical channel sync.
    );

    /**Send a user input indication to the remote endpoint.
       This sends a Hook Flash emulation user input.
      */
    void SendUserInputHookFlash(
      int duration = 500  /// Duration of tone in milliseconds
    ) { SendUserInputTone('!', duration); }

    /**Call back for remote enpoint has sent user input.
       The default behaviour calls OnUserInputString() if the PDU is of the
       alphanumeric type, or OnUserInputTone() if of a tone type.
      */
    virtual void OnUserInputIndication(
      const H245_UserInputIndication & pdu  /// Full user indication PDU
    );

    /**Call back for remote enpoint has sent user input as a string.

       The default behaviour calls the endpoint function of the same name.
      */
    virtual void OnUserInputString(
      const PString & value   /// String value of indication
    );

    /**Call back for remote enpoint has sent user input.

       The default behaviour calls the endpoint function of the same name.
      */
    virtual void OnUserInputTone(
      char tone,               /// DTMF tone code
      unsigned duration,       /// Duration of tone in milliseconds
      unsigned logicalChannel, /// Logical channel number for RTP sync.
      unsigned rtpTimestamp    /// RTP timestamp in logical channel sync.
    );
  //@}

  /**@name RTP Session Management */
  //@{
    /**Get an RTP session for the specified ID.
       If there is no session of the specified ID, NULL is returned.
      */
    RTP_Session * GetSession(
      unsigned sessionID
    ) const;

    /**Get an H323 RTP session for the specified ID.
       If there is no session of the specified ID, NULL is returned.
      */
    H323_RTP_Session * GetSessionCallbacks(
      unsigned sessionID
    ) const;

    /**Use an RTP session for the specified ID.
       If there is no session of the specified ID, a new one is created using
       the information prvided in the H245_TransportAddress PDU. If the system
       does not support the specified transport, NULL is returned.

       If this function is used, then the ReleaseSession() function MUST be
       called or the session is never deleted for the lifetime of the H323
       connection.
      */
    RTP_Session * UseSession(
      unsigned sessionID,
      const H245_TransportAddress & pdu
    );

    /**Release the session. If the session ID is not being used any more any
       clients via the UseSession() function, then the session is deleted.
     */
    void ReleaseSession(
      unsigned sessionID
    );

    /**Callback from the RTP session for statistics monitoring.
       This is called every so many packets on the transmitter and receiver
       threads of the RTP session indicating that the statistics have been
       updated.

       The default behaviour calls H323EndPoint::OnRTPStatistics().
      */
    virtual void OnRTPStatistics(
      const RTP_Session & session   /// Session with statistics
    ) const;

    /**Get the names of the codecs in use for the RTP session.
       If there is no session of the specified ID, an empty string is returned.
      */
    PString GetSessionCodecNames(
      unsigned sessionID
    ) const;
  //@}

  /**@name Request Mode Changes */
  //@{
    /**Make a request to mode change to remote.
      */
    void RequestModeChange();

    /**Received request for mode change from remote.
      */
    BOOL OnRequestModeChange(
      const H245_RequestMode & pdu,     /// Received PDU
      H245_RequestModeAck & ack,        /// Ack PDU to send
      H245_RequestModeReject & reject   /// Reject PDU to send
    );

    /**Received acceptance of last mode change request from remote.
      */
    void OnAcceptModeChange(
      const H245_RequestModeAck & pdu  /// Received PDU
    );

    /**Received reject of last mode change request from remote.
      */
    void OnRefusedModeChange(
      const H245_RequestModeReject * pdu  /// Received PDU, if NULL is a timeout
    );
  //@}

  /**@name Member variable access */
  //@{
    /**Get the owner endpoint for this connection.
     */
    H323EndPoint & GetEndPoint() const { return endpoint; }

    /**Get the call direction for this connection.
     */
    BOOL HadAnsweredCall() const { return callAnswered; }

    /**Get the distinctive ring code for incoming call.
       This returns an integer from 0 to 7 that may indicate to an application
       that different ring cadences are to be used.
      */
    unsigned GetDistinctiveRing() const { return distinctiveRing; }

    /**Set the distinctive ring code for outgoing call.
       This sets the integer from 0 to 7 that will be used in the outgoing
       Setup PDU. Note this must be called either immediately after
       construction or during the OnSendSignalSetup() callback function so the
       member variable is set befor ethe PDU is sent.
      */
    void SetDistinctiveRing(unsigned pattern) { distinctiveRing = pattern&7; }

    /**Get the call reference for this connection.
     */
    unsigned GetCallReference() const { return callReference; }

    /**Get the call identifier for this connection.
     */
    const OpalGloballyUniqueID & GetCallIdentifier() const { return callIdentifier; }

    /**Get the conference identifier for this connection.
     */
    const OpalGloballyUniqueID & GetConferenceIdentifier() const { return conferenceIdentifier; }

    /**Get the local name/alias.
      */
    const PString & GetLocalPartyName() const { return localPartyName; }

    /**Set the local name/alias from information in the PDU.
      */
    void SetLocalPartyName(const PString & name) { localPartyName = name; }

    /**Get the remote party name.
       This returns a string indicating the remote parties names and aliases.
       This can be a complicated string containing all the aliases and the
       remote host name. For example:
              "Fred Nurk (fred, 5551234) [fred.nurk.com]"
      */
    const PString & GetRemotePartyName() const { return remotePartyName; }

    /**Get the remote party number, if there was one one.
       If the remote party has indicated an e164 number as one of its aliases
       or as a field in the Q.931 PDU, then this function will return it.
      */
    const PString & GetRemotePartyNumber() const { return remotePartyNumber; }

    /**Get the remote party address.
       This will return the "best guess" at an address to use in a
       H323EndPoint::MakeCall() function to call the remote party back again.
       Note that due to the presence of gatekeepers/proxies etc this may not
       always be accurate.
      */
    const PString & GetRemotePartyAddress() const { return remotePartyAddress; }

    /**Set the name/alias of remote end from information in the PDU.
      */
    void SetRemotePartyInfo(
      const H323SignalPDU & pdu /// PDU from which to extract party info.
    );

    /**Get the remote application name and version.
       This information is obtained from the sourceInfo field of the H.225
       Setup PDU or the destinationInfo of the call proceeding or alerting
       PDU's. The general format of the string will be information extracted
       from the VendorIdentifier field of the EndpointType. In partcular:

          productId\tversionId\tt35CountryCode:t35Extension:manufacturerCode

      */
    const PString & GetRemoteApplication() const { return remoteApplication; }

    /**Set the name/alias of remote end from information in the PDU.
      */
    void SetRemoteApplication(
      const H225_EndpointType & pdu /// PDU from which to extract application info.
    );
    
    /**Get the remotes capability table for this connection.
     */
    const H323Capabilities & GetLocalCapabilities() const { return localCapabilities; }

    /**Get the remotes capability table for this connection.
     */
    const H323Capabilities & GetRemoteCapabilities() const { return remoteCapabilities; }

    /**Get the maximum audio jitter delay.
     */
    unsigned GetRemoteMaxAudioDelayJitter() const { return remoteMaxAudioDelayJitter; }

    /**Get the signalling channel being used.
      */
    const OpalTransport * GetSignallingChannel() const { return signallingChannel; }

    /**Get the control channel being used (may return signalling channel).
      */
    const OpalTransport & GetControlChannel() const;

    /**Get the time at which the connection was established
      */
    PTime GetConnectionStartTime() const { return connectionStartTime; }

    /**Get the default maximum audio delay jitter parameter.
     */
    WORD GetMaxAudioDelayJitter() const { return maxAudioDelayJitter; }

    /**Set the default maximum audio delay jitter parameter.
     */
    void SetMaxAudioDelayJitter(unsigned jitter) { maxAudioDelayJitter = (WORD)jitter; }
  //@}


  protected:
    H323EndPoint & endpoint;

    unsigned             distinctiveRing;
    unsigned             callReference;
    OpalGloballyUniqueID callIdentifier;
    OpalGloballyUniqueID conferenceIdentifier;

    PString            localPartyName;
    PString            localDestinationAddress;
    H323Capabilities   localCapabilities; // Capabilities local system supports
    PString            remotePartyNumber;
    PString            remotePartyAddress;
    PString            remoteApplication;
    H323Capabilities   remoteCapabilities; // Capabilities remote system supports
    unsigned           remoteMaxAudioDelayJitter;
    PTimer             roundTripDelayTimer;
    WORD               maxAudioDelayJitter;

    unsigned bandwidthAvailable;

    OpalTransport * signallingChannel;
    OpalTransport * controlChannel;
    OpalListener  * controlListener;
    BOOL            h245Tunneling;
    H323SignalPDU * h245TunnelPDU;

    enum ConnectionStates {
      NoConnectionActive,
      AwaitingGatekeeperAdmission,
      AwaitingTransportConnect,
      AwaitingSignalConnect,
      HasExecutedSignalConnect,
      EstablishedConnection,
      ShuttingDownConnection,
      NumConnectionStates
    } connectionState;

    BOOL transmitterSidePaused;

    RTP_SessionManager rtpSessions;

    enum FastStartStates {
      FastStartDisabled,
      FastStartInitiate,
      FastStartResponse,
      FastStartAcknowledged,
      NumFastStartStates
    };
    FastStartStates        fastStartState;
    H323LogicalChannelList fastStartChannels;
    OpalMediaStream      * fastStartedTransmitMediaStream;

    BOOL earlyStart;
    BOOL startT120;

#if PTRACING
    static const char * const ConnectionStatesNames[NumConnectionStates];
    friend ostream & operator<<(ostream & o, ConnectionStates s) { return o << ConnectionStatesNames[s]; }
    static const char * const FastStartStateNames[NumFastStartStates];
    friend ostream & operator<<(ostream & o, FastStartStates s) { return o << FastStartStateNames[s]; }
#endif


    // The following pointers are to protocol procedures, they are pointers to
    // hide their complexity from the H323Connection classes users.
    H245NegMasterSlaveDetermination  * masterSlaveDeterminationProcedure;
    H245NegTerminalCapabilitySet     * capabilityExchangeProcedure;
    H245NegLogicalChannels           * logicalChannels;
    H245NegRequestMode               * requestModeProcedure;
    H245NegRoundTripDelay            * roundTripDelayProcedure;

    H450xDispatcher                  * h450dispatcher;
    H4502Handler                     * h4502handler;
    H4504Handler                     * h4504handler;
};


#endif // __H323_H323CON_H


/////////////////////////////////////////////////////////////////////////////
