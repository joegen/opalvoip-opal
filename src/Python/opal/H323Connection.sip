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
#include <h323/h323con.h>
%End

///////////////////////////////////////////////////////////////////////////////

/**This class represents a particular H323 connection between two endpoints.
   There are at least two threads in use, this one to look after the
   signalling channel, an another to look after the control channel. There
   would then be additional threads created for each data channel created by
   the control channel protocol thread.
 */
class H323Connection : OpalRTPConnection
{
  public:
  /**@name Construction */
  //@{
    /**Create a new connection.
     */
    H323Connection(
      OpalCall & call,                         ///<  Call object connection belongs to
      H323EndPoint & endpoint,                 ///<  H323 End Point object
      const PString & token,                   ///<  Token for new connection
      const PString & alias,                   ///<  Alias for outgoing call
      const H323TransportAddress & address,    ///<  Address for outgoing call
      unsigned options = 0,                    ///<  Connection option bits
      OpalConnection::StringOptions * stringOptions = NULL ///<  complex string options
    );

    /**Destroy the connection
     */
    ~H323Connection();
  //@}

  /**@name Overrides from OpalConnection */
  //@{
    /**Get indication of connection being to a "network".
       This indicates the if the connection may be regarded as a "network"
       connection. The distinction is about if there is a concept of a "remote"
       party being connected to and is best described by example: sip, h323,
       iax and pstn are all "network" connections as they connect to something
       "remote". While pc, pots and ivr are not as the entity being connected
       to is intrinsically local.
      */
    virtual bool IsNetworkConnection() const;

    /**Get this connections protocol prefix for URLs.
      */
    virtual PString GetPrefixName() const;

	/// Call back for connection to act on changed string options
	virtual void OnApplyStringOptions();

    /**Start an outgoing connection.
       This function will initiate the connection to the remote entity, for
       example in H.323 it sends a SETUP, in SIP it sends an INVITE etc.

       The default behaviour is to send SETUP packet.
      */
    virtual PBoolean SetUpConnection();

    /**Indicate to remote endpoint an alert is in progress.
       If this is an incoming connection and it is in the Alerting phase, then
       this function is used to indicate to that endpoint that an alert is in
       progress. This is usually due to another connection which is in the
       call (the B party) has received an OnAlerting() indicating that its
       remote endpoint is "ringing".

       The default behaviour sends an ALERTING pdu.
      */
    virtual PBoolean SetAlerting(
      const PString & calleeName,   ///<  Name of endpoint being alerted.
      PBoolean withMedia                ///<  Open media with alerting
    );

    /**Indicate to remote endpoint we are connected.

       The default behaviour sends a CONNECT pdu.
      */
    virtual PBoolean SetConnected();

    /**Indicate to remote endpoint we are sending a progress.

      The default behaviour sends a PROGRESS pdu.
     */
    virtual PBoolean SetProgressed();
    
    /** Called when a connection is established.
        This indicates that a connection to an endpoint was established. This
        usually occurs after OnConnected() and indicates that the connection
        is both connected and has media flowing.

        Default behaviour is to call H323EndPoint::OnConnectionEstablished
      */
    virtual void OnEstablished();

    /**Clean up the termination of the connection.
       This function can do any internal cleaning up and waiting on background
       threads that may be using the connection object.

       Note that there is not a one to one relationship with the
       OnEstablishedConnection() function. This function may be called without
       that function being called. For example if SetUpConnection() was used
       but the call never completed.

       Classes that override this function should make sure they call the
       ancestor version for correct operation.

       An application will not typically call this function as it is used by
       the OpalManager during a release of the connection.

       The default behaviour calls OnRelease() then calls the ancestor.
      */
    virtual void OnReleased();

    /**Get the destination address of an incoming connection.
       This will, for example, collect a phone number from a POTS line, or
       get the fields from the H.225 SETUP pdu in a H.323 connection.
      */
    virtual PString GetDestinationAddress();

    /**Get alerting type information of an incoming call.
       The type of "distinctive ringing" for the call. The string is protocol
       dependent, so the caller would need to be aware of the type of call
       being made. Some protocols may ignore the field completely.

       For SIP this corresponds to the string contained in the "Alert-Info"
       header field of the INVITE. This is typically a URI for the ring file.

       For H.323 this must be a string representation of an integer from 0 to 7
       which will be contained in the Q.931 SIGNAL (0x34) Information Element.

       Default behaviour returns an empty string.
      */
    virtual PString GetAlertingType() const;

    /**Set alerting type information for outgoing call.
       The type of "distinctive ringing" for the call. The string is protocol
       dependent, so the caller would need to be aware of the type of call
       being made. Some protocols may ignore the field completely.

       For SIP this corresponds to the string contained in the "Alert-Info"
       header field of the INVITE. This is typically a URI for the ring file.

       For H.323 this must be a string representation of an integer from 0 to 7
       which will be contained in the Q.931 SIGNAL (0x34) Information Element.

       Default behaviour returns false.
      */
    virtual bool SetAlertingType(const PString & info);
	
    /** Get Q.931 Progress Indicator IE description from received PROGRESS or ALERTING pdus.
      */
    unsigned GetProgressIndicator() const;

    /** Set Q.931 Progress Indicator IE description to send in PROGRESS or ALERTING pdus.
      */
    void SetProgressIndicator(unsigned indicator);

    /**Get the data formats this connection is capable of operating.
       This provides a list of media data format names that an
       OpalMediaStream may be created in within this connection.

       The default behaviour returns media data format names contained in
       the remote capability table.
      */
    virtual OpalMediaFormatList GetMediaFormats() const;

    /**Get next available session ID for the media type.
      */
    virtual unsigned GetNextSessionID(
      const OpalMediaType & mediaType,   ///< Media type of stream being opened
      bool isSource                      ///< Stream is a source/sink
    );

    /**Switch to/from FAX mode.
      */
    virtual bool SwitchFaxMediaStreams(
      bool enableFax  ///< Enable FAX or return to audio mode
    );

    /**Request close of a specific media stream.
       Note that this is usually asymchronous, the OnClosedMediaStream() function is
       called when the stream is really closed.
      */
    virtual bool CloseMediaStream(
      OpalMediaStream & stream  ///< Stream to close
    );

    /**Get information on the media channel for the connection.
       The default behaviour returns PTrue and fills the info structure if
       there is a media channel active for the sessionID.
     */
    virtual PBoolean GetMediaInformation(
      unsigned sessionID,     ///<  Session ID for media channel
      MediaInformation & info ///<  Information on media channel
    ) const;
  //@}

  /**@name Signalling Channel */
  //@{
    /**Attach a transport to this connection as the signalling channel.
      */
    void AttachSignalChannel(
      const PString & token,    ///<  New token to use to identify connection
      H323Transport * channel,  ///<  Transport for the PDU's
      PBoolean answeringCall        ///<  Flag for if incoming/outgoing call.
    );

    /**Forward incoming call to specified address.
       This would typically be called from within the OnIncomingCall()
       function when an application wishes to redirct an unwanted incoming
       call.

       The return value is PTrue if the call is to be forwarded, PFalse
       otherwise. Note that if the call is forwarded the current connection is
       cleared with the ended call code of EndedByCallForwarded.
      */
    virtual PBoolean ForwardCall(
      const PString & forwardParty   ///<  Party to forward call to.
    );

    /**Initiate the transfer of an existing call (connection) to a new remote 
       party.

       If remoteParty is a valid call token, then the remote party is transferred
       to that party (consultation transfer) and both calls are cleared.
     */
    virtual bool TransferConnection(
      const PString & remoteParty   ///<  Remote party to transfer the existing call to
    );


    /**Initiate the transfer of an existing call (connection) to a new remote party
       using H.450.2.  This sends a Call Transfer Initiate Invoke message from the
       A-Party (transferring endpoint) to the B-Party (transferred endpoint).
     */
    bool TransferCall(
      const PString & remoteParty,   ///<  Remote party to transfer the existing call to
      const PString & callIdentity = PString::Empty()
                                    ///<  Call Identity of secondary call if present
    );

    /**Transfer the call through consultation so the remote party in the primary call is connected to
       the called party in the second call using H.450.2.  This sends a Call Transfer Identify Invoke 
       message from the A-Party (transferring endpoint) to the C-Party (transferred-to endpoint).
     */
    void ConsultationTransfer(
      const PString & primaryCallToken  ///<  Primary call
    );

    /**Handle the reception of a callTransferSetupInvoke APDU whilst a secondary call exists.  This 
       method checks whether the secondary call is still waiting for a callTransferSetupInvoke APDU and 
       proceeds to clear the call if the call identies match.
       This is an internal function and it is not expected the user will call
       it directly.
     */
    virtual void HandleConsultationTransfer(
      const PString & callIdentity, /**Call Identity of secondary call 
                                       received in SETUP Message. */
      H323Connection & incoming     ///<  Connection upon which SETUP PDU was received.
    );

    /**Determine whether this connection is being transferred.
     */
    PBoolean IsTransferringCall() const;

    /**Determine whether this connection is the result of a transferred call.
     */
    PBoolean IsTransferredCall() const;

    /**Handle the transfer of an existing connection to a new remote.
       This is an internal function and it is not expected the user will call
       it directly.
     */
    virtual void HandleTransferCall(
      const PString & token,
      const PString & identity
    );

    /**Get transfer invoke ID dureing trasfer.
       This is an internal function and it is not expected the user will call
       it directly.
      */
    int GetCallTransferInvokeId();

    /**Handle the failure of a call transfer operation at the Transferred Endpoint.  This method is
       used to handle the following transfer failure cases that can occur at the Transferred Endpoint. 
       The cases are:
       Reception of an Admission Reject
       Reception of a callTransferSetup return error APDU.
       Expiry of Call Transfer timer CT-T4.
     */
    virtual void HandleCallTransferFailure(
      const int returnError    ///<  Failure reason code
    );

    /**Store the passed token on the current connection's H4502Handler.
       This is an internal function and it is not expected the user will call
       it directly.
     */
    void SetAssociatedCallToken(
      const PString & token  ///<  Associated token
    );

    /**Callback to indicate a successful transfer through consultation.  The paramter passed is a
       reference to the existing connection between the Transferring endpoint and Transferred-to 
       endpoint.
     */
    virtual void OnConsultationTransferSuccess(
      H323Connection & secondaryCall  ///<  Secondary call for consultation
    );

    /**Place the call on hold, suspending all media channels (H.450.4).  Note it is the responsibility 
       of the application layer to delete the MOH Channel if music on hold is provided to the remote
       endpoint.  So far only Local Hold has been implemented. 
     */
    bool HoldCall(
      PBoolean localHold   ///<  true for Local Hold, false for Remote Hold
    );

    /**Retrieve the call from hold, activating all media channels (H.450.4).
       This method examines the call hold state and performs the necessary
       actions required to retrieve a Near-end or Remote-end call on hold.
       NOTE: Only Local Hold is implemented so far. 
    */
    bool RetrieveCall();

    /**Set the alternative media channel.  This channel can be used to provide
       Media On Hold (MOH) for a near end call hold operation or to provide
       Recorded Voice Anouncements (RVAs).  If this method is not called before
       a call hold operation is attempted, no media on hold will be provided
       for the held endpoint.
      */
    void SetHoldMedia(
      PChannel * audioChannel
    );

    /**Determine if Meadia On Hold is enabled.
      */
    PBoolean IsMediaOnHold() const;

    /**Determine if held.
      */
    PBoolean IsLocalHold() const;

    /**Determine if held.
      */
    PBoolean IsRemoteHold() const;

    /**Determine if the current call is held or in the process of being held.
      */
    PBoolean IsCallOnHold() const;

    /**Begin a call intrusion request.
       Calls h45011handler->IntrudeCall where SS pdu is added to Call Setup
       message.
      */
    virtual void IntrudeCall(
      unsigned capabilityLevel
    );

    /**Handle an incoming call instrusion request.
       Calls h45011handler->AwaitSetupResponse where we set Handler state to
       CI-Wait-Ack
      */
    virtual void HandleIntrudeCall(
      const PString & token,
      const PString & identity
    );

    /**Set flag indicating call intrusion.
       Used to set a flag when intrusion occurs and to determine if
       connection is created for Call Intrusion. This flag is used when we
       should decide whether to Answer the call or to Close it.
      */
    void SetCallIntrusion();

    PBoolean IsCallIntrusion();

    /**Get Call Intrusion Protection Level of the local endpoint.
      */
    unsigned GetLocalCallIntrusionProtectionLevel();

    /**Get Call Intrusion Protection Level of other endpoints that we are in
       connection with.
      */
    virtual PBoolean GetRemoteCallIntrusionProtectionLevel(
      const PString & callToken,
      unsigned callIntrusionProtectionLevel
    );

    virtual void SetIntrusionImpending();

    virtual void SetForcedReleaseAccepted();

    virtual void SetIntrusionNotAuthorized();

    /**Send a Call Waiting indication message to the remote endpoint using
       H.450.6.  The second paramter is used to indicate to the calling user
       how many additional users are "camped on" the called user. A value of
       zero indicates to the calling user that he/she is the only user
       attempting to reach the busy called user.
     */
    void SendCallWaitingIndication(
      const unsigned nbOfAddWaitingCalls = 0   ///<  number of additional waiting calls at the served user
    );

    /**This function is called when insufficient digits have been entered.
       This supports overlapped dialling so that a call can begin when it is
       not known how many more digits are to be entered in a phone number.

       It is expected that the application will override this function. It
       should be noted that the application should not block in the function
       but only indicate to whatever other thread is gathering digits that
       more are required and that thread should call SendMoreDigits().

       If PFalse is returned the connection is aborted and a Release Complete
       PDU is sent.

       The default behaviour simply returns PFalse.
     */
     virtual PBoolean OnInsufficientDigits();

    /**This function is called when sufficient digits have been entered.
       This supports overlapped dialling so that a call can begin when it is
       not known how many more digits are to be entered in a phone number.

       The digits parameter is appended to the existing remoteNumber member
       variable and the call is retried.

       If PFalse is returned the connection is aborted and a Release Complete
       PDU is sent.

       The default behaviour simply returns PTrue.
     */
    virtual void SendMoreDigits(
      const PString & digits    ///<  Extra digits
    );

    /**Start control channel negotiations.
      */
    virtual PBoolean StartControlNegotiations();

    /**Handle reading data on the control channel.
     */
    virtual void HandleControlChannel();

 
    /**Handle a H.239 flow control request.
       Default behaviour simply sends an acknowedge response.
      */
    virtual bool OnH239FlowControlRequest(
      unsigned logicalChannel,
      unsigned bitRate
    );

    /**Handle a H.239 flow control ack/reject response.
       Default behaviour does nothing
      */
    virtual bool OnH239FlowControlResponse(
      unsigned logicalChannel,
      bool rejected
    );

    /**Handle a H.239 presentation token request.
       Default behaviour simply sends an acknowedge response.
      */
    virtual bool OnH239PresentationRequest(
      unsigned logicalChannel,
      unsigned symmetryBreaking,
      unsigned terminalLabel
    );
	
	/**Send a H.239 presentation token request.
       Asks the remote for the "token" allow local system to own the
       presentation video channel. A OnH239PresentationResponse() callback
       should occur when the remote responds to the request.
      */
    virtual bool SendH239PresentationRequest(
      unsigned logicalChannel,
      unsigned symmetryBreaking,
      unsigned terminalLabel
    );

    /**Handle a H.239 presentation token ack/reject response.
       Default behaviour simply sends a release command.
      */
    virtual bool OnH239PresentationResponse(
      unsigned logicalChannel,
      unsigned terminalLabel,
      bool rejected
    );
	
	/**Send a H.239 presentation token release.
       Tell the remote we no longer require the "token" allowing the local
       system to own the presentation video channel.
      */
    virtual bool SendH239PresentationRelease(
      unsigned logicalChannel,
      unsigned terminalLabel
    );

    /**Handle a H.239 presentation token release command.
       Default behaviour does nothing.
      */
    virtual bool OnH239PresentationRelease(
      unsigned logicalChannel,
      unsigned terminalLabel
    );

    /**Handle a H.239 presentation token owner indication.
       Default behaviour does nothing.
      */
    virtual bool OnH239PresentationIndication(
      unsigned logicalChannel,
      unsigned terminalLabel
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

       If PFalse is returned the connection is aborted. The default behaviour
       returns PTrue.
     */
    virtual PBoolean OnControlProtocolError(
      ControlProtocolErrors errorSource,  ///<  Source of the proptoerror
      const void * errorData = NULL       ///<  Data associated with error
    );

    /**Send a new capability set.
      */
    virtual bool SendCapabilitySet(
      PBoolean empty  ///<  Send an empty set.
    );

    /**check if TCS procedure in progress states.
      */
    virtual bool IsSendingCapabilitySet();

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
    PBoolean IsH245Master() const;

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
      const OpalMediaType & mediaType,  ///<  media type of channel
      unsigned sessionID                ///<  Session ID to find default logical channel.
    );

    /**Select default logical channel for fast start.
       Internal function, not for normal use.
      */
    virtual void SelectFastStartChannels(
      unsigned sessionID,   ///<  Session ID to find default logical channel.
      PBoolean transmitter,     ///<  Whether to open transmitters
      PBoolean receiver         ///<  Whether to open receivers
    );

  /**@name Bandwidth Management */
  //@{
    /**Set the available bandwidth in 100's of bits/sec.
       Note if the force parameter is PTrue this function will close down
       active logical channels to meet the new bandwidth requirement.
      */
    virtual PBoolean SetBandwidthAvailable(
      unsigned newBandwidth,    ///<  New bandwidth limit
      PBoolean force = PFalse        ///<  Force bandwidth limit
    );

    /**Get the bandwidth currently used.
       This totals the open channels and returns the total bandwidth used in
       100's of bits/sec
      */
    virtual unsigned GetBandwidthUsed() const;
  //@}

  /**@name Indications */
  //@{
    /**Get the real user input indication transmission mode.
       This will return the user input mode that will actually be used for
       transmissions. It will be the value of GetSendUserInputMode() provided
       the remote endpoint is capable of that mode.
      */
    virtual SendUserInputModes GetRealSendUserInputMode() const;

    /**Send a user input indication to the remote endpoint.
       This is for sending arbitrary strings as user indications.

       The user indication is sent according to the sendUserInputMode member
       variable. If SendUserInputAsString then this uses an H.245 "string"
       UserInputIndication pdu sending the entire string in one go. If
       SendUserInputAsTone then a separate H.245 "signal" UserInputIndication
       pdu is sent for each character. If SendUserInputAsInlineRFC2833 then
       the indication is inserted into the outgoing audio stream as an RFC2833
       RTP data pdu.

       SendUserInputAsSeparateRFC2833 is not yet supported.
      */
    virtual PBoolean SendUserInputString(
      const PString & value                   ///<  String value of indication
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

       The user indication is sent according to the sendUserInputMode member
       variable. If SendUserInputAsString then this uses an H.245 "string"
       UserInputIndication pdu sending the entire string in one go. If
       SendUserInputAsTone then a separate H.245 "signal" UserInputIndication
       pdu is sent for each character. If SendUserInputAsInlineRFC2833 then
       the indication is inserted into the outgoing audio stream as an RFC2833
       RTP data pdu.

       SendUserInputAsSeparateRFC2833 is not yet supported.
      */
    virtual PBoolean SendUserInputTone(
      char tone,             ///<  DTMF tone code
      unsigned duration = 0  ///<  Duration of tone in milliseconds
    );

    /**Send a user input indication to the remote endpoint.
       This is for sending arbitrary strings as user indications.

       This always uses a Q.931 Keypad Information Element in a Information
       pdu sending the entire string in one go.
      */
    virtual PBoolean SendUserInputIndicationQ931(
      const PString & value                   ///<  String value of indication
    );

    /**Send a user input indication to the remote endpoint.
       This is for sending arbitrary strings as user indications.

       This always uses an H.245 "string" UserInputIndication pdu sending the
       entire string in one go.
      */
    virtual PBoolean SendUserInputIndicationString(
      const PString & value                   ///<  String value of indication
    );

    /**Send a user input indication to the remote endpoint.
       This sends DTMF emulation user input.This uses an H.245 "signal"
       UserInputIndication pdu.
      */
    virtual PBoolean SendUserInputIndicationTone(
      char tone,                   ///<  DTMF tone code
      unsigned duration = 0,       ///<  Duration of tone in milliseconds
      unsigned logicalChannel = 0, ///<  Logical channel number for RTP sync.
      unsigned rtpTimestamp = 0    ///<  RTP timestamp in logical channel sync.
    );


    /**Callback from the RTP session for statistics monitoring.
       This is called every so many packets on the transmitter and receiver
       threads of the RTP session indicating that the statistics have been
       updated.

       The default behaviour calls H323EndPoint::OnRTPStatistics().
      */
    virtual void OnRTPStatistics(
      const RTP_Session & session   ///<  Session with statistics
    ) const;

    /**Get the names of the codecs in use for the RTP session.
       If there is no session of the specified ID, an empty string is returned.
      */
    virtual PString GetSessionCodecNames(
      unsigned sessionID
    ) const;

  //@}

  /**@name Request Mode Changes */
  //@{
    /**Make a request to mode change to remote.
       This asks the remote system to stop it transmitters and start sending
       one of the combinations specifed.

       The modes are separated in the string by \n characters, and all of the
       channels (capabilities) are strings separated by \t characters. Thus a
       very simple mode change would be "T.38" which requests that the remote
       start sending T.38 data and nothing else. A more complicated example
       would be "G.723\tH.261\nG.729\tH.261\nG.728" which indicates that the
       remote should either start sending G.723 and H.261, G.729 and H.261 or
       just G.728 on its own.

       Returns PFalse if a mode change is currently in progress, only one mode
       change may be done at a time.
      */
    virtual PBoolean RequestModeChange(
      const PString & newModes  ///<  New modes to select
    );

  /**@name Other services */
  //@{
    /**Request a mode change to T.38 data.
       Note this function is strictly H.323 and does operate correctly in the
       OPAL media stream model. It is maintained for backward compatibility
       with older applications only.
      */
    virtual PBoolean RequestModeChangeT38(
      const char * capabilityNames = "T.38\nT38FaxUDP"
    );

  /**@name Member variable access */
  //@{
    /**Get the owner endpoint for this connection.
     */
    H323EndPoint & GetEndPoint() const;

    /**Get the call direction for this connection.
     */
    PBoolean HadAnsweredCall() const;

    /**Determined if connection is gatekeeper routed.
     */
    PBoolean IsGatekeeperRouted() const;

    /**Get the distinctive ring code for incoming call.
       This returns an integer from 0 to 7 that may indicate to an application
       that different ring cadences are to be used.
      */
    unsigned GetDistinctiveRing() const;

    /**Set the distinctive ring code for outgoing call.
       This sets the integer from 0 to 7 that will be used in the outgoing
       Setup PDU. Note this must be called either immediately after
       construction or during the OnSendSignalSetup() callback function so the
       member variable is set befor ethe PDU is sent.
      */
    void SetDistinctiveRing(unsigned pattern);

    /**Get the internal OpenH323 call token for this connection.
       Deprecated, only used for backward compatibility.
     */
    const PString & GetCallToken() const;

    /**Get the call reference for this connection.
     */
    unsigned GetCallReference() const;

     /**Get the protocol-specific unique identifier for this connection.
     */
    virtual PString GetIdentifier() const;

    /**Get the conference identifier for this connection.
     */
    const PGloballyUniqueID & GetConferenceIdentifier() const;

    /**Set the local name/alias from information in the PDU.
      */
    void SetLocalPartyName(const PString & name);

    /**Get the list of all alias names this connection is using.
      */
    const PStringList & GetLocalAliasNames() const;
    
    /**Get the remote party address.
       This will return the "best guess" at an address to use in a
       to call the user again later.
      */
    PString GetRemotePartyURL() const;
    
    ///**Get the remotes capability table for this connection.
    // */
    const H323Capabilities & GetLocalCapabilities() const;

    ///**Get the remotes capability table for this connection.
    // */
    const H323Capabilities & GetRemoteCapabilities() const;

    /**Get the maximum audio jitter delay.
     */
    unsigned GetRemoteMaxAudioDelayJitter() const;

    /**Get the signalling channel being used.
      */
    const H323Transport * GetSignallingChannel() const;

    /**Get the signalling channel protocol version number.
      */
    unsigned GetSignallingVersion() const;

    /**Get the control channel being used (may return signalling channel).
      */
    const H323Transport & GetControlChannel() const;

    /**Get the control channel being used (may return signalling channel).
      */
    OpalTransport & GetTransport() const;

    /**Get the control channel protocol version number.
      */
    unsigned GetControlVersion() const;

    /**Get the UUIE PDU monitor bit mask.
     */
    unsigned GetUUIEsRequested() const;

    /**Set the UUIE PDU monitor bit mask.
     */
    void SetUUIEsRequested(unsigned mask);

    /**Get the iNow Gatekeeper Access Token OID.
     */
    const PString GetGkAccessTokenOID() const;

    /**Set the iNow Gatekeeper Access Token OID.
     */
    void SetGkAccessTokenOID(const PString & oid);

    /**Get the iNow Gatekeeper Access Token data.
     */
    const PBYTEArray & GetGkAccessTokenData() const;

    /**Set the Destionation Extra Call Info memeber.
     */
    void SetDestExtraCallInfo(
      const PString & info
    );

    /** Set the remote call waiting flag
     */
    void SetRemotCallWaiting(const unsigned value);

    /**How many caller's are waiting on the remote endpoint?
      -1 - None
       0 - Just this connection
       n - n plus this connection
     */
    int GetRemoteCallWaiting() const;

    /**Set the enforced duration limit for the call.
       This starts a timer that will automatically shut down the call when it
       expires.
      */
    void SetEnforcedDurationLimit(
      unsigned seconds  ///<  max duration of call in seconds
    );
  //@}
    
    /**Get the local H.239 control capability.
     */
    bool GetLocalH239Control() const;

    /**Set the local H.239 control capability.
     */
    void SetLocalH239Control(
      bool on   ///< H.239 control capability is to be sent to remote
    );

    /**Get the remote H.239 control capability.
     */
    bool GetRemoteH239Control() const;

    /**Get the remote H.239 options.
     */
    OpalMediaFormatList GetRemoteH239Formats() const;


    virtual void OnMediaCommand(OpalMediaCommand & note, int extra);
    
  protected:
      enum ConnectionStates {
      NoConnectionActive,
      AwaitingGatekeeperAdmission,
      AwaitingTransportConnect,
      AwaitingSignalConnect,
      AwaitingLocalAnswer,
      HasExecutedSignalConnect,
      EstablishedConnection,
      ShuttingDownConnection,
      NumConnectionStates
    };

    enum FastStartStates {
      FastStartDisabled,
      FastStartInitiate,
      FastStartResponse,
      FastStartAcknowledged,
      NumFastStartStates
    };

};

/////////////////////////////////////////////////////////////////////////////
