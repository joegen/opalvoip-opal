/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. 
 
 Author: Jonathan M. Henson
 Date: 07/08/2012
 Contact: jonathan.michael.henson@gmail.com
 */

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Runtime.InteropServices;

namespace OpalDotNET
{
  public class Opal_API
  {
    #region defines
   
    /// <summary>
    /// Current API version
    /// </summary> 
    public const int OPAL_C_API_VERSION = 27;

    /// <summary>
    /// H.323 Protocol supported string for OpalInitialise()
    /// </summary>
    public const string OPAL_PREFIX_H323 = "h323";

    /// <summary>
    /// SIP Protocol supported string for OpalInitialise()
    /// </summary>
    public const string OPAL_PREFIX_SIP = "sip";

    /// <summary>
    /// IAX2 Protocol supported string for OpalInitialise()
    /// </summary>
    public const string OPAL_PREFIX_IAX2 = "iax2";

    /// <summary>
    ///  PC sound system supported string for OpalInitialise()
    /// </summary>
    public const string OPAL_PREFIX_PCSS = "pc"; 

    /// <summary>
    /// Local endpoint supported string for OpalInitialise()
    /// </summary>
    public const string OPAL_PREFIX_LOCAL = "local";

    /// <summary>
    /// Plain Old Telephone System supported string for OpalInitialise()
    /// </summary>
    public const string OPAL_PREFIX_POTS = "pots";

    /// <summary>
    /// Public Switched Network supported string for OpalInitialise()
    /// </summary>
    public const string OPAL_PREFIX_PSTN = "pstn";

    /// <summary>
    /// Interactive Voice Response supported string for OpalInitialise()
    /// </summary>
    public const string OPAL_PREFIX_IVR = "ivr";

    public const string OPAL_PREFIX_ALL = OPAL_PREFIX_H323 + " " + OPAL_PREFIX_SIP + " " +
                        OPAL_PREFIX_IAX2 + " " + OPAL_PREFIX_PCSS + " " + OPAL_PREFIX_LOCAL + " " +
                        OPAL_PREFIX_POTS + " " + OPAL_PREFIX_PSTN + " " + OPAL_PREFIX_IVR;

    /// <summary>
    /// Name of SIP event package for Message Waiting events.
    /// </summary>
    public const string OPAL_MWI_EVENT_PACKAGE = "message-summary";
        
    /// <summary>
    /// Name of SIP even package fo rmonitoring call status
    /// </summary>
    public const string OPAL_LINE_APPEARANCE_EVENT_PACKAGE = "dialog;sla;ma";
    #endregion

    #region enumerations
    /// <summary>
    /// Type code for messages defined by OpalMessage.
    /// </summary>
    public enum OpalMessageType
    {
      /// <summary>
      /// An error occurred during a command. This is only returned by
      ///	OpalSendMessage(). The details of the error are shown in the
      /// OpalMessage::m_commandError field.
      /// </summary>
      OpalIndCommandError,

      /// <summary>
      /// Set general parameters command. This configures global settings in OPAL.
      /// See the OpalParamGeneral structure for more information.
      /// </summary>
      OpalCmdSetGeneralParameters,

      /// <summary>
      /// Set protocol parameters command. This configures settings in OPAL that
      /// may be different for each protocol, e.g. SIP & H.323. See the 
      ///	OpalParamProtocol structure for more information.
      /// </summary>
      OpalCmdSetProtocolParameters,

      /// <summary>
      /// Register/Unregister command. This initiates a registration or
      ///	unregistration operation with a protocol dependent server. Currently
      ///	only for H.323 and SIP. See the OpalParamRegistration structure for more
      ///	information.
      /// </summary>
      OpalCmdRegistration,

      /// <summary>
      /// Status of registration indication. After the OpalCmdRegistration has
      /// initiated a registration, this indication will be returned by the
      ///	OpalGetMessage() function when the status of the registration changes,
      ///	e.g. successful registration or communications failure etc. See the
      /// OpalStatusRegistration structure for more information.
      /// </summary>
      OpalIndRegistration,

      /// <summary>
      /// Set up a call command. This starts the outgoing call process. The
      ///	OpalIndAlerting, OpalIndEstablished and OpalIndCallCleared messages are
      ///	returned by OpalGetMessage() to indicate the call progress. See the 
      ///	OpalParamSetUpCall structure for more information.
      /// </summary>
      OpalCmdSetUpCall,

      /// <summary>
      /// Incoming call indication. This is returned by the OpalGetMessage() function
      ///	at any time after listeners are set up via the OpalCmdSetProtocolParameters
      ///	command. See the OpalStatusIncomingCall structure for more information.
      /// </summary>
      OpalIndIncomingCall,

      /// <summary>
      /// Answer call command. After a OpalIndIncomingCall is returned by the
      ///	OpalGetMessage() function, an application maye indicate that the call is
      ///	to be answered with this message. The OpalMessage m_callToken field is
      ///	set to the token returned in OpalIndIncomingCall.
      /// </summary>
      OpalCmdAnswerCall,

      /// <summary>
      /// Hang Up call command. After a OpalCmdSetUpCall command is executed or a
      ///	OpalIndIncomingCall indication is received then this may be used to
      /// "hang up" the call. The OpalIndCallCleared is subsequently returned in
      ///	the OpalGetMessage() when the call has completed its hang up operation.
      ///	See OpalParamCallCleared structure for more information.
      /// </summary>
      OpalCmdClearCall,

      /// <summary>
      /// Remote is alerting indication. This message is returned in the
      ///	OpalGetMessage() function when the underlying protocol states the remote
      ///	telephone is "ringing". See the OpalParamSetUpCall structure for more
      ///	information.
      /// </summary>
      OpalIndAlerting,

      /// <summary>
      /// Call is established indication. This message is returned in the
      ///	OpalGetMessage() function when the remote or local endpont has "answered"
      ///	the call and there is media flowing. See the  OpalParamSetUpCall
      /// structure for more information.
      /// </summary>
      OpalIndEstablished,

      /// <summary>
      /// User input indication. This message is returned in the OpalGetMessage()
      ///	function when, during a call, user indications (aka DTMF tones) are
      ///	received. See the OpalStatusUserInput structure for more information.
      /// </summary>
      OpalIndUserInput,

      /// <summary>
      /// Call is cleared indication. This message is returned in the
      ///	OpalGetMessage() function when the call has completed. The OpalMessage
      ///	m_callToken field indicates which call cleared.
      /// </summary>
      OpalIndCallCleared,

      /// <summary>
      /// Place call in a hold state. The OpalMessage m_callToken field is set to
      ///	the token returned in OpalIndIncomingCall.
      /// </summary>
      OpalCmdHoldCall,

      /// <summary>
      /// Retrieve call from hold state. The OpalMessage m_callToken field is set
      ///	to the token for the call.
      /// </summary>
      OpalCmdRetrieveCall,

      /// <summary>
      /// Transfer a call to another party. This starts the outgoing call process
      ///	for the other party. See the  OpalParamSetUpCall structure for more
      ///	information.
      /// </summary>
      OpalCmdTransferCall,

      /// <summary>
      /// User input command. This sends specified user input to the remote
      ///	connection. See the OpalStatusUserInput structure for more information.
      /// </summary>
      OpalCmdUserInput,

      /// <summary>
      /// Message Waiting indication. This message is returned in the
      /// OpalGetMessage() function when an MWI is received on any of the supported
      ///	protocols.
      /// </summary>
      OpalIndMessageWaiting,

      /// <summary>
      /// A media stream has started/stopped. This message is returned in the
      ///	OpalGetMessage() function when a media stream is started or stopped. See the
      ///	OpalStatusMediaStream structure for more information.
      /// </summary>
      OpalIndMediaStream,

      /// <summary>
      /// Execute control on a media stream. See the OpalStatusMediaStream structure
      ///	for more information.
      /// </summary>
      OpalCmdMediaStream,

      /// <summary>
      /// Set the user data field associated with a call
      /// </summary>
      OpalCmdSetUserData,

      /// <summary>
      /// Line Appearance indication. This message is returned in the
      ///	OpalGetMessage() function when any of the supported protocols indicate that
      /// the state of a "line" has changed, e.g. free, busy, on hold etc.
      /// </summary>
      OpalIndLineAppearance,

      /// <summary>
      /// Start recording an active call. See the OpalParamRecording structure
      ///	for more information.
      /// </summary>
      OpalCmdStartRecording,

      /// <summary>
      /// Stop recording an active call. Only the m_callToken field of the
      ///	OpalMessage union is used.
      /// </summary>
      OpalCmdStopRecording,

      /// <summary>
      /// Call has been accepted by remote. This message is returned in the
      ///	OpalGetMessage() function when the underlying protocol states the remote
      ///	endpoint acknowledged that it will route the call. This is distinct from
      /// OpalIndAlerting in that it is not known at this time if anything is
      /// ringing. This indication may be used to distinguish between "transport"
      ///	level error, in which case another host may be tried, and that the
      ///	responsibility for finalising the call has moved "upstream". See the
      ///	OpalParamSetUpCall structure for more information.
      /// </summary>
      OpalIndProceeding,

      /// <summary>
      /// Send an indication to the remote that we are "ringing". The OpalMessage
      /// m_callToken field indicates which call is alerting.
      /// </summary>
      OpalCmdAlerting,

      /// <summary>
      /// Indicate a call has been placed on hold by remote. This message is returned
      ///	in the OpalGetMessage() function.
      /// </summary>
      OpalIndOnHold,

      /// <summary>
      /// Indicate a call has been retrieved from hold by remote. This message is
      ///	returned in the OpalGetMessage() function.
      /// </summary>
      OpalIndOffHold,

      /// <summary>
      /// Status of transfer operation that is under way. This message is returned in
      ///	the OpalGetMessage() function. See the OpalStatusTransferCall structure for
      ///	more information.
      /// </summary>
      OpalIndTransferCall,

      /// <summary>
      /// Indicates completion of the IVR (VXML) script. This message is returned in
      ///	the OpalGetMessage() function. See the OpalStatusIVR structure for
      ///	more information.
      /// </summary>
      OpalIndCompletedIVR,

      /// <summary>
      /// Always add new messages to ethe end to maintain backward compatibility.
      /// </summary>
      OpalMessageTypeCount
    }

    /// <summary>
    /// Type code the silence detect algorithm modes.
    /// This is used by the OpalCmdSetGeneralParameters command in the OpalParamGeneral structure.
    /// </summary>
    public enum OpalSilenceDetectMode
    {
      /// <summary>
      /// No change to the silence detect mode.
      /// </summary>
      OpalSilenceDetectNoChange,

      /// <summary>
      /// Indicate silence detect is disabled
      /// </summary>
      OpalSilenceDetectDisabled,

      /// <summary>
      /// Indicate silence detect uses a fixed threshold
      /// </summary>
      OpalSilenceDetectFixed,

      /// <summary>
      /// Indicate silence detect uses an adaptive threashold
      /// </summary>
      OpalSilenceDetectAdaptive
    }

    /// <summary>
    /// Type code the echo cancellation algorithm modes.
    /// This is used by the OpalCmdSetGeneralParameters command in the OpalParamGeneral structure.
    /// </summary>
    public enum OpalEchoCancelMode
    {
      /// <summary>
      /// No change to the echo cancellation mode.
      /// </summary>
      OpalEchoCancelNoChange,

      /// <summary>
      /// Indicate the echo cancellation is disabled
      /// </summary>
      OpalEchoCancelDisabled,

      /// <summary>
      /// Indicate the echo cancellation is enabled
      /// </summary>
      OpalEchoCancelEnabled
    }

    /// <summary>
    /// Type code for media stream status/control.
    /// This is used by the OpalIndMediaStream indication and OpalCmdMediaStream command
    /// in the OpalStatusMediaStream structure.
    /// </summary>
    public enum OpalLineAppearanceStates
    {
      /// <summary>
      /// Line has ended a call.
      /// </summary>
      OpalLineTerminated,

      /// <summary>
      /// Line has been siezed.
      /// </summary>
      OpalLineTrying,

      /// <summary>
      /// Line is trying to make a call.
      /// </summary>
      OpalLineProceeding,

      /// <summary>
      /// Line is ringing.
      /// </summary>
      OpalLineRinging,

      /// <summary>
      /// Line is connected.
      /// </summary>
      OpalLineConnected,

      /// <summary>
      /// Line appearance subscription successful.
      /// </summary>
      OpalLineSubcribed,

      /// <summary>
      /// Line appearance unsubscription successful.
      /// </summary>
      OpalLineUnsubcribed,

      OpalLineIdle = OpalLineTerminated // Kept for backward compatibility
    }

    /// <summary>
    /// Timing mode for the media data call back functions data type.
    /// This is used by the OpalCmdSetGeneralParameters command in the
    /// OpalParamGeneral structure.
    ///
    /// This controls if the read/write function is in control of the real time
    /// aspects of the media flow. If synchronous then the read/write function
    /// is expected to handle the real time "pacing" of the read or written data.
    ///
    /// Note this is important both for reads and writes. For example in
    /// synchronous mode you cannot simply read from a file and send, or you will
    /// likely overrun the remotes buffers. Similarly for writing to a file, the
    /// correct operation of the OPAL jitter buffer is dependent on it not being
    /// drained too fast by the "write" function.
    ///
    /// If marked as asynchroous then the OPAL stack itself will take care of the
    /// timing and things like read/write to a disk file will work correctly.
    /// </summary>
    public enum OpalMediaTiming
    {
      /// <summary>
      /// No change to the media data type.
      /// </summary>
      OpalMediaTimingNoChange,

      /// <summary>
      /// Indicate the read/write function is going to handle
      /// all real time aspects of the media flow.
      /// </summary> 
      OpalMediaTimingSynchronous,

      /// <summary>
      /// Indicate the read/write function does not require
      /// real time aspects of the media flow.
      /// </summary>
      OpalMediaTimingAsynchronous,

      /// <summary>
      /// Indicate the read/write function does not handle
      /// the real time aspects of the media flow and they
      /// must be simulated by the OPAL library.
      /// </summary>
      OpalMediaTimingSimulated
    }

   /// <summary>
   /// Type code the media data call back functions data type.   
   ///This is used by the OpalCmdSetGeneralParameters command in the
   ///OpalParamGeneral structure.
   ///
   ///This controls if the whole RTP data frame or just the paylaod part
   ///is passed to the read/write function.
   /// </summary>
  public enum OpalMediaDataType
  {
    /// <summary>
    /// No change to the media data type.
    /// </summary>
    OpalMediaDataNoChange,

    /// <summary>
    /// Indicate only the RTP payload is passed to the
    /// read/write function
    /// </summary>
    OpalMediaDataPayloadOnly, 

    /// <summary>
    /// Indicate the whole RTP frame including header is   
    /// passed to the read/write function
    /// </summary>
    OpalMediaDataWithHeader
  }

    /// <summary>
    /// Type code for controlling the mode in which user input (DTMF) is sent.
    /// This is used by the OpalCmdSetProtocolParameters command in the OpalParamProtocol structure.
    /// </summary>       
    public enum OpalUserInputModes
    {
      /// <summary>
      /// Default mode for protocol
      /// </summary>
      OpalUserInputDefault,

      /// <summary>
      /// Use Q.931 Information Elements (H.323 only)
      /// </summary>
      OpalUserInputAsQ931,

      /// <summary>
      /// Use arbitrary strings (H.245 string, or INFO dtmf)
      /// </summary>
      OpalUserInputAsString,

      /// <summary>
      /// Use DTMF specific names (H.245 signal, or INFO dtmf-relay)
      /// </summary>
      OpalUserInputAsTone,

      /// <summary>
      /// Use RFC 2833 for DTMF only
      /// </summary>
      OpalUserInputAsRFC2833,

      /// <summary>
      /// Use in-band generated audio tones for DTMF
      /// </summary>
      OpalUserInputInBand,
    }

    /// <summary>
    /// Type code for media stream status/control.
    /// This is used by the OpalIndRegistration indication in the OpalStatusRegistration structure.
    /// </summary>
    public enum OpalRegistrationStates
    {
      /// <summary>
      /// Successfully registered.
      /// </summary>
      OpalRegisterSuccessful,

      /// <summary>
      /// Successfully unregistered. Note that the m_error field may be
      /// non-null if an error occurred during unregistration, however
      /// the unregistration will "complete" as far as the local endpoint
      /// is concerned and no more registration retries are made.
      /// </summary>
      OpalRegisterRemoved,

      /// <summary>
      /// Registration has failed. The m_error field of the
      /// OpalStatusRegistration structure will contain more details.
      /// </summary>
      OpalRegisterFailed,

      /// <summary>
      /// Registrar/Gatekeeper has gone offline and a failed retry
      /// has been executed.
      /// </summary>
      OpalRegisterRetrying,

      /// <summary>
      /// Registration has been restored after a succesfull retry.
      /// </summary>
      OpalRegisterRestored,
    }

    /// <summary>
    /// Type code for media stream status/control.
    /// This is used by the OpalIndMediaStream indication and OpalCmdMediaStream command
    /// in the OpalStatusMediaStream structure.
    /// </summary>
    public enum OpalMediaStates
    {
      /// <summary>
      /// No change to the media stream state.
      /// </summary>
      OpalMediaStateNoChange,

      /// <summary>
      /// Media stream has been opened when indication,
      /// or is to be opened when a command.
      /// </summary>
      OpalMediaStateOpen,

      /// <summary>
      /// Media stream has been closed when indication,
      /// or is to be closed when a command.
      /// </summary>
      OpalMediaStateClose,

      /// <summary>
      /// Media stream has been paused when indication,
      /// or is to be paused when a command.
      /// </summary>
      OpalMediaStatePause,

      /// <summary>
      /// Media stream has been paused when indication,
      /// or is to be paused when a command.
      /// </summary>
      OpalMediaStateResume
    }

    /// <summary>
    /// Type of mixing for video when recording.
    /// This is used by the OpalCmdStartRecording command in the OpalParamRecording structure.
    /// </summary>
    public enum OpalVideoRecordMixMode
    {
      /// <summary>
      /// Two images side by side with black bars top and bottom.
      /// It is expected that the input frames and output are all
      /// the same aspect ratio, e.g. 4:3. Works well if inputs
      /// are QCIF and output is CIF for example.
      /// </summary>
      OpalSideBySideLetterbox,

      /// <summary>
      /// Two images side by side, scaled to fit halves of output
      /// frame. It is expected that the output frame be double
      /// the width of the input data to maintain aspect ratio.
      /// e.g. for CIF inputs, output would be 704x288.
      /// </summary>
      OpalSideBySideScaled,

      /// <summary>
      /// Two images, one on top of the other with black bars down
      /// the sides. It is expected that the input frames and output
      /// are all the same aspect ratio, e.g. 4:3. Works well if
      /// inputs are QCIF and output is CIF for example.
      /// </summary>
      OpalStackedPillarbox,

      /// <summary>
      /// Two images, one on top of the other, scaled to fit halves
      /// of output frame. It is expected that the output frame be
      /// double the height of the input data to maintain aspect
      /// ratio. e.g. for CIF inputs, output would be 352x576.
      /// </summary>
      OpalStackedScaled,
    }

    /// <summary>
    /// Type code for media stream status/control.
    /// This is used by the OpalIndMediaStream indication and OpalCmdMediaStream command
    /// in the OpalStatusMediaStream structure.
    /// </summary>
    public enum OpalCallEndReason 
    {
      /// <summary>
      /// Local endpoint application cleared call
      /// </summary>
      OpalCallEndedByLocalUser,

      /// <summary>
      /// Local endpoint did not accept call OnIncomingCall()=PFalse
      /// </summary>
      OpalCallEndedByNoAccept,

      /// <summary>
      /// Local endpoint declined to answer call
      /// </summary>
      OpalCallEndedByAnswerDenied, 

      /// <summary>
      /// Remote endpoint application cleared call
      /// </summary>
      OpalCallEndedByRemoteUser, 

      /// <summary>
      /// Remote endpoint refused call
      /// </summary>
      OpalCallEndedByRefusal,

      /// <summary>
      /// Remote endpoint did not answer in required time
      /// </summary>
      OpalCallEndedByNoAnswer,

      /// <summary>
      /// Remote endpoint stopped calling
      /// </summary>
      OpalCallEndedByCallerAbort,

      /// <summary>
      /// Transport error cleared call
      /// </summary>
      OpalCallEndedByTransportFail,

      /// <summary>
      /// Transport connection failed to establish call
      /// </summary>
      OpalCallEndedByConnectFail,

      /// <summary>
      /// Gatekeeper has cleared call
      /// </summary>
      OpalCallEndedByGatekeeper,

      /// <summary>
      /// Call failed as could not find user (in GK)
      /// </summary>
      OpalCallEndedByNoUser,

      /// <summary>
      /// Call failed as could not get enough bandwidth
      /// </summary>
      OpalCallEndedByNoBandwidth,

      /// <summary>
      /// Could not find common capabilities
      /// </summary>
      OpalCallEndedByCapabilityExchange,

      /// <summary>
      /// Call was forwarded using FACILITY message
      /// </summary>
      OpalCallEndedByCallForwarded,

      /// <summary>
      /// Call failed a security check and was ended
      /// </summary>
      OpalCallEndedBySecurityDenial,

      /// <summary>
      /// Local endpoint busy
      /// </summary>
      OpalCallEndedByLocalBusy,

      /// <summary>
      /// Local endpoint congested
      /// </summary>
      OpalCallEndedByLocalCongestion,

      /// <summary>
      /// Remote endpoint busy
      /// </summary>
      OpalCallEndedByRemoteBusy,

      /// <summary>
      /// Remote endpoint congested
      /// </summary>
      OpalCallEndedByRemoteCongestion,

      /// <summary>
      /// Could not reach the remote party
      /// </summary>
      OpalCallEndedByUnreachable,

      /// <summary>
      ///  The remote party is not running an endpoint
      /// </summary>
      OpalCallEndedByNoEndPoint,

      /// <summary>
      /// The remote party host off line
      /// </summary>
      OpalCallEndedByHostOffline,

      /// <summary>
      /// The remote failed temporarily app may retry
      /// </summary>
      OpalCallEndedByTemporaryFailure,

      /// <summary>
      /// The remote ended the call with unmapped Q.931 cause code
      /// </summary>
      OpalCallEndedByQ931Cause,

      /// <summary>
      /// Call cleared due to an enforced duration limit
      /// </summary>
      OpalCallEndedByDurationLimit,

      /// <summary>
      /// Call cleared due to invalid conference ID 
      /// </summary>
      OpalCallEndedByInvalidConferenceID,

      /// <summary>
      /// Call cleared due to missing dial tone
      /// </summary>
      OpalCallEndedByNoDialTone,

      /// <summary>
      /// Call cleared due to missing ringback tone
      /// </summary>
      OpalCallEndedByNoRingBackTone,

      /// <summary>
      /// Call cleared because the line is out of service, 
      /// </summary>
      OpalCallEndedByOutOfService,

      /// <summary>
      /// Call cleared because another call is answered
      /// </summary>
      OpalCallEndedByAcceptingCallWaiting,

      /// <summary>
      /// Q931 code specified in MS byte
      /// </summary>
      OpalCallEndedWithQ931Code = 0x100
    }
    #endregion

    //please note that I used the IntPtr for ALL reference types. As far as I can tell, there is no way to port this code
    //without the union in OpalMessage. The Explicit layout option does not allow for the mixing of reference and value types. There are wrappers
    //for each of these structures in OpalContext.cs which handle marshalling to and from the IntPtr structure. Please use those wrappers
    //unless you know what you are doing. These structures are specifically for platform invoke.
    #region structures
    
    [StructLayout(LayoutKind.Explicit)]
    public struct OpalMessageUnion
    {
      //string
      [FieldOffset(0)]
      public IntPtr m_commandError;       ///< Used by OpalIndCommandError

      //string
      [FieldOffset(0)]
      public IntPtr m_callToken;          ///< Used by OpalCmdHoldcall/OpalCmdRetrieveCall/OpalCmdStopRecording
                                          ///
      [FieldOffset(0)]      
      public OpalParamGeneral m_general;            ///< Used by OpalCmdSetGeneralParameters

      [FieldOffset(0)]
      public OpalParamProtocol m_protocol;           ///< Used by OpalCmdSetProtocolParameters

      [FieldOffset(0)]
      public OpalParamRegistration m_registrationInfo;   ///< Used by OpalCmdRegistration

      [FieldOffset(0)]
      public OpalStatusRegistration m_registrationStatus; ///< Used by OpalIndRegistration

      [FieldOffset(0)]
      public OpalParamSetUpCall m_callSetUp;          ///< Used by OpalCmdSetUpCall/OpalIndProceeding/OpalIndAlerting/OpalIndEstablished     

      [FieldOffset(0)]
      public OpalStatusIncomingCall m_incomingCall;       ///< Used by OpalIndIncomingCall

      [FieldOffset(0)]
      public OpalParamAnswerCall m_answerCall;         ///< Used by OpalCmdAnswerCall/OpalCmdAlerting

      [FieldOffset(0)]
      public OpalStatusUserInput m_userInput;          ///< Used by OpalIndUserInput/OpalCmdUserInput

      [FieldOffset(0)]
      public OpalStatusMessageWaiting m_messageWaiting;     ///< Used by OpalIndMessageWaiting

      [FieldOffset(0)]
      public OpalStatusLineAppearance m_lineAppearance;     ///< Used by OpalIndLineAppearance

      [FieldOffset(0)]
      public OpalStatusCallCleared m_callCleared;        ///< Used by OpalIndCallCleared

      [FieldOffset(0)]
      public OpalParamCallCleared m_clearCall;          ///< Used by OpalCmdClearCall

      [FieldOffset(0)]
      public OpalStatusMediaStream m_mediaStream;        ///< Used by OpalIndMediaStream/OpalCmdMediaStream

      [FieldOffset(0)]
      public OpalParamSetUserData m_setUserData;        ///< Used by OpalCmdSetUserData

      [FieldOffset(0)]
      public OpalParamRecording m_recording;          ///< Used by OpalCmdStartRecording

      [FieldOffset(0)]
      public OpalStatusTransferCall m_transferStatus;     ///< Used by OpalIndTransferCall

      [FieldOffset(0)]
      public OpalStatusIVR m_ivrStatus;          ///< Used by OpalIndCompletedIVR

    }

    /// <summary>
    /// Message to/from OPAL system.
    /// This is passed via the OpalGetMessage() or OpalSendMessage() functions.
    /// </summary> 
    [StructLayout(LayoutKind.Explicit)]
    public struct OpalMessage
    {
      /// <summary>
      /// type of message
      /// </summary> 
      [FieldOffset(0)]
      public OpalMessageType m_type;

      [FieldOffset(4)]
      public OpalMessageUnion m_param;
    }

    /// <summary>
    /// General parameters for the OpalCmdSetGeneralParameters command.
    ///   This is only passed to and returned from the OpalSendMessage() function.
    ///
    ///   Example:
    ///      <code>
    ///     OpalMessage   command;
    ///      OpalMessage * response;
    ///
    ///      memset(&command, 0, sizeof(command));
    ///      command.m_type = OpalCmdSetGeneralParameters;
    ///      command.m_param.m_general.m_stunServer = "stun.voxgratia.org";
    ///      command.m_param.m_general.m_mediaMask = "RFC4175*";
    ///      response = OpalSendMessage(hOPAL, &command);
    ///      </code>
    ///
    ///   For m_mediaOrder and m_mediaMask, each '\n' seperated sub-string in the
    ///   array is checked using a simple wildcard matching algorithm.
    ///
    ///   The '*' character indicates substrings, for example: "G.711*" would remove
    ///   "G.711-uLaw-64k" and "G.711-ALaw-64k".
    ///
    ///   The '@' character indicates a type of media format, so say "\@video" would
    ///   remove all video codecs.
    ///
    ///   The '!' character indicates a negative test. That is the entres that do NOT
    ///   match the string are removed. The string after the '!' may contain '*' and
    ///   '@' characters.
    ///
    ///   It should be noted that when the ! operator is used, they are combined
    ///   differently to the usual application of each entry in turn. Thus, the string
    ///   "!A\n!B" will result in keeping <i>both</i> A and B formats.
    /// </summary>
    public struct OpalParamGeneral
    {
      /// <summary>
      /// Audio recording device name
      /// </summary>      
      public IntPtr m_audioRecordDevice;

      /// <summary>
      /// Audio playback device name
      /// </summary>      
      public IntPtr m_audioPlayerDevice;

      /// <summary>
      /// Video input (e.g. camera) device name
      /// </summary>      
      public IntPtr m_videoInputDevice;

      /// <summary>
      /// Video output (e.g. window) device name
      /// </summary>      
      public IntPtr m_videoOutputDevice;

      /// <summary>
      /// Video preview (e.g. window) device name
      /// </summary>      
      public IntPtr m_videoPreviewDevice;

      /// <summary>
      /// List of media format names to set the preference order for media.
      /// This list of names (e.g. "G.723.1") is separated by the '\n'/
      /// * character.
      /// </summary>      
      public IntPtr m_mediaOrder;

      /// <summary>
      /// List of media format names to set media to be excluded.
      /// This list of names (e.g. "G.723.1") is separated by the '\n'
      /// character.
      /// </summary>      
      public IntPtr m_mediaMask;

      /// <summary>
      ///  List of media types (e.g. audio, video) separated by spaces
      ///  which may automatically be received automatically. If NULL
      ///  no change is made, but if "" then all media is prevented from
      ///  auto-starting.
      /// </summary>      
      public IntPtr m_autoRxMedia;

      /// <summary>
      /// List of media types (e.g. audio, video) separated by spaces
      /// which may automatically be transmitted automatically. If NULL
      /// no change is made, but if "" then all media is prevented from
      /// auto-starting.
      /// </summary>      
      public IntPtr m_autoTxMedia;

      /// <summary>
      /// The host name or IP address of the Network Address Translation
      /// router which may be between the endpoint and the Internet.
      /// </summary>      
      public IntPtr m_natRouter;

      /// <summary>
      /// The host name or IP address of the STUN server which may be
      /// used to determine the NAT router characteristics automatically.
      /// </summary>     
      public IntPtr m_stunServer;

      /// <summary>
      /// Base of range of ports to use for TCP communications. This may
      /// be required by some firewalls.
      /// </summary>      
      public uint m_tcpPortBase;

      /// <summary>
      /// Max of range of ports to use for TCP communications. This may
      /// be required by some firewalls.
      /// </summary>      
      public uint m_tcpPortMax;

      /// <summary>
      /// Base of range of ports to use for UDP communications. This may
      /// be required by some firewalls.
      /// </summary>      
      public uint m_udpPortBase;

      /// <summary>
      ///  Max of range of ports to use for UDP communications. This may
      ///  be required by some firewalls.
      /// </summary>      
      public uint m_udpPortMax;

      /// <summary>
      /// Base of range of ports to use for RTP/UDP communications. This may
      /// be required by some firewalls.
      /// </summary>      
      public uint m_rtpPortBase;

      /// <summary>
      /// Max of range of ports to use for RTP/UDP communications. This may
      /// be required by some firewalls.
      /// </summary>      
      public uint m_rtpPortMax;

      /// <summary>
      /// Value for the Type Of Service byte with UDP/IP packets which may
      /// be used by some routers for simple Quality of Service control.
      /// </summary>      
      public uint m_rtpTypeOfService;

      /// <summary>
      /// Maximum payload size for RTP packets. This may sometimes need to
      /// be set according to the MTU or the underlying network. 
      /// </summary>      
      public uint m_rtpMaxPayloadSize;

      /// <summary>
      /// Minimum jitter time in milliseconds. For audio RTP data being
      /// received this sets the minimum time of the adaptive jitter buffer
      /// which smooths out irregularities in the transmission of audio
      /// data over the Internet.
      /// </summary>      
      public uint m_minAudioJitter;

      /// <summary>
      /// Maximum jitter time in milliseconds. For audio RTP data being
      /// received this sets the maximum time of the adaptive jitter buffer
      /// which smooths out irregularities in the transmission of audio
      /// data over the Internet.
      /// </summary>     
      public uint m_maxAudioJitter;

      /// <summary>
      /// Silence detection mode. This controls the silence
      /// detection algorithm for audio transmission: 0=no change,
      /// 1=disabled, 2=fixed, 3=adaptive.
      /// </summary>
      public OpalSilenceDetectMode m_silenceDetectMode;

      /// <summary>
      /// Silence detection threshold value. This applies if
      /// m_silenceDetectMode is fixed (2) and is a PCM-16 value.
      /// </summary>
      public uint m_silenceThreshold;

      /// <summary>
      /// Time signal is required before audio is transmitted. This is
      /// is RTP timestamp units (8000Hz).
      /// </summary>
      public uint m_signalDeadband;

      /// <summary>
      /// Time silence is required before audio is transmission is stopped.
      /// This is is RTP timestamp units (8000Hz).
      /// </summary>
      public uint m_silenceDeadband;

      /// <summary>
      /// Window for adapting the silence threashold. This applies if
      /// m_silenceDetectMode is adaptive (3). This is is RTP timestamp
      /// units (8000Hz).
      /// </summary>
      public uint m_silenceAdaptPeriod;

      /// <summary>
      /// Accoustic Echo Cancellation control. 0=no change, 1=disabled,
      /// 2=enabled.
      /// </summary>
      public OpalEchoCancelMode m_echoCancellation;

      /// <summary>
      /// Set the number of hardware sound buffers to use.
      /// Note the largest of m_audioBuffers and m_audioBufferTime/frametime
      /// will be used.
      /// </summary>
      public uint m_audioBuffers;

      /// <summary>
      /// Callback function for reading raw media data. See
      /// OpalMediaDataFunction for more information.
      /// </summary>
      public IntPtr m_mediaReadData;

      /// <summary>
      /// Callback function for writing raw media data. See
      /// OpalMediaDataFunction for more information.
      /// </summary>
      public IntPtr m_mediaWriteData;

      /// <summary>
      /// Indicate that the media read/write callback function
      /// is passed the full RTP header or just the payload.
      /// 0=no change, 1=payload only, 2=with RTP header.
      /// </summary>
      public uint m_mediaDataHeader;

      /// <summary>
      /// If non-null then this function is called before
      /// the message is queued for return in the
      /// GetMessage(). See the
      /// OpalMessageAvailableFunction for more details.
      /// </summary>
      public IntPtr m_messageAvailable;

      /// <summary>
      /// List of media format options to be set. This is a '\n' separated
      /// list of entries of the form "codec:option=value". Codec is either
      /// a media type (e.g. "Audio" or "Video") or a specific media format,
      /// for example:
      /// <code>
      /// "G.723.1:Tx Frames Per Packet=2\nH.263:Annex T=0\n"
      /// "Video:Max Rx Frame Width=176\nVideo:Max Rx Frame Height=144"
      /// </code>
      /// </summary>      
      public IntPtr m_mediaOptions;

      /// <summary>
      /// Set the hardware sound buffers to use in milliseconds.
      /// Note the largest of m_audioBuffers and m_audioBufferTime/frametime
      /// will be used.
      /// </summary>
      public uint m_audioBufferTime;

      /// <summary>
      /// Indicate that an "alerting" message is automatically (value=1)
      /// or manually (value=2) sent to the remote on receipt of an
      /// OpalIndIncomingCall message. If set to manual then it is up
      /// to the application to send a OpalCmdAlerting message to
      /// indicate to the remote system that we are "ringing".
      /// If zero then no change is made.
      /// </summary>
      public uint m_manualAlerting;

      /// <summary>
      /// Indicate how the media read/write callback function
      /// handles the real time aspects of the media flow.
      /// 0=no change, 1=synchronous, 2=asynchronous,
      /// 3=simulate synchronous.
      /// </summary>
      public OpalMediaTiming m_mediaTiming;

      /// <summary>
      /// Indicate that the video read callback function
      /// handles the real time aspects of the media flow.
      /// This can override the m_mediaTiming.
      /// </summary>
      public OpalMediaTiming m_videoSourceTiming;
    }

    /// <summary>
    /// Protocol parameters for the OpalCmdSetProtocolParameters command.
    ///   This is only passed to and returned from the OpalSendMessage() function.
    ///
    ///   Example:
    ///      <code>
    ///      OpalMessage   command;
    ///      OpalMessage * response;
    ///
    ///      memset(&command, 0, sizeof(command));
    ///      command.m_type = OpalCmdSetProtocolParameters;
    ///      command.m_param.m_protocol.m_userName = "robertj";
    ///      command.m_param.m_protocol.m_displayName = "Robert Jongbloed";
    ///      command.m_param.m_protocol.m_interfaceAddresses = "*";
    ///      response = OpalSendMessage(hOPAL, &command);
    ///      </code>
    /// </summary>
    public struct OpalParamProtocol
    {

      /// <summary>
      /// Protocol prefix for parameters, e.g. "h323" or "sip". If this is
      /// NULL or empty string, then the parameters are set for all protocols
      /// where they maybe set.
      /// </summary>      
      public IntPtr m_prefix;

      /// <summary>
      /// User name to identify the endpoint. This is usually the protocol
      /// specific name and may interact with the OpalCmdRegistration
      /// command. e.g. "robertj" or 61295552148
      /// </summary>      
      public IntPtr m_userName;

      /// <summary>
      /// Display name to be used. This is the human readable form of the
      /// users name, e.g. "Robert Jongbloed".
      /// </summary>      
      public IntPtr m_displayName;

      /// <summary>
      /// Product description data
      /// </summary>
      public OpalProductDescription m_product;

      /// <summary>
      /// A list of interfaces to start listening for incoming calls.
      /// This list is separated by the '\n' character. If NULL no
      /// listeners are started or stopped. If and empty string ("")
      /// then all listeners are stopped. If a "*" then listeners
      /// are started for all interfaces in the system.
      /// 
      ///  If the prefix is "ivr", then this is the default VXML script
      ///  or URL to execute on incoming calls.
      /// </summary>      
      public IntPtr m_interfaceAddresses;

      /// <summary>
      /// The mode for user input transmission. Note this only applies if an
      /// explicit protocol is indicated in m_prefix. See OpalUserInputModes
      /// for more information.
      /// </summary>
      public OpalUserInputModes m_userInputMode;

      /// <summary>
      /// Default options for new calls using the specified protocol. This
      /// string is of the form key=value\nkey=value
      /// </summary>      
      public IntPtr m_defaultOptions;
    }

    /// <summary>
    /// Registration parameters for the OpalCmdRegistration command.
    ///   This is only passed to and returned from the OpalSendMessage() function.
    ///
    ///   Example:
    ///      <code>
    ///      OpalMessage   command;
    ///      OpalMessage * response;
    ///
    ///      // H.323 register with gatekeeper
    ///      memset(&command, 0, sizeof(command));
    ///      command.m_type = OpalCmdRegistration;
    ///      command.m_param.m_registrationInfo.m_protocol = "h323";
    ///      command.m_param.m_registrationInfo.m_identifier = "31415";
    ///      command.m_param.m_registrationInfo.m_hostName = gk.voxgratia.org;
    ///      command.m_param.m_registrationInfo.m_password = "secret";
    ///      command.m_param.m_registrationInfo.m_timeToLive = 300;
    ///      response = OpalSendMessage(hOPAL, &command);
    ///      if (response != NULL && response->m_type == OpalCmdRegistration)
    ///        m_AddressOfRecord = response->m_param.m_registrationInfo.m_identifier
    ///
    ///      // SIP register with regstrar/proxy
    ///      memset(&command, 0, sizeof(command));
    ///      command.m_type = OpalCmdRegistration;
    ///      command.m_param.m_registrationInfo.m_protocol = "sip";
    ///      command.m_param.m_registrationInfo.m_identifier = "rjongbloed@ekiga.net";
    ///      command.m_param.m_registrationInfo.m_password = "secret";
    ///      command.m_param.m_registrationInfo.m_timeToLive = 300;
    ///      response = OpalSendMessage(hOPAL, &command);
    ///      if (response != NULL && response->m_type == OpalCmdRegistration)
    ///        m_AddressOfRecord = response->m_param.m_registrationInfo.m_identifier
    ///
    ///      // unREGISTER
    ///      memset(&command, 0, sizeof(command));
    ///      command.m_type = OpalCmdRegistration;
    ///      command.m_param.m_registrationInfo.m_protocol = "sip";
    ///      command.m_param.m_registrationInfo.m_identifier = m_AddressOfRecord;
    ///      command.m_param.m_registrationInfo.m_timeToLive = 0;
    ///      response = OpalSendMessage(hOPAL, &command);
    ///
    ///      // Set event package so do SUBSCRIBE
    ///      memset(&command, 0, sizeof(command));
    ///      command.m_type = OpalCmdRegistration;
    ///      command.m_param.m_registrationInfo.m_protocol = "sip";
    ///      command.m_param.m_registrationInfo.m_identifier = "2012@pbx.local";
    ///      command.m_param.m_registrationInfo.m_hostName = "sa@pbx.local";
    ///      command.m_param.m_registrationInfo.m_eventPackage = OPAL_LINE_APPEARANCE_EVENT_PACKAGE;
    ///      command.m_param.m_registrationInfo.m_timeToLive = 300;
    ///      response = OpalSendMessage(hOPAL, &command);
    ///      if (response != NULL && response->m_type == OpalCmdRegistration)
    ///        m_AddressOfRecord = response->m_param.m_registrationInfo.m_identifier
    ///
    ///      // unSUBSCRIBE
    ///      memset(&command, 0, sizeof(command));
    ///      command.m_type = OpalCmdRegistration;
    ///      command.m_param.m_registrationInfo.m_protocol = "sip";
    ///      command.m_param.m_registrationInfo.m_identifier = m_AddressOfRecord;
    ///      command.m_param.m_registrationInfo.m_eventPackage = OPAL_LINE_APPEARANCE_EVENT_PACKAGE;
    ///      command.m_param.m_registrationInfo.m_timeToLive = 0;
    ///      response = OpalSendMessage(hOPAL, &command);
    ///      </code>
    /// </summary>
    public struct OpalParamRegistration
    {
      /// <summary>
      /// Protocol prefix for registration. Currently must be "h323" or
      /// "sip", cannot be NULL.
      /// </summary>      
      public IntPtr m_protocol;

      /// <summary>
      /// Identifier for name to be registered at server. If NULL
      /// or empty then the value provided in the OpalParamProtocol::m_userName
      /// field of the OpalCmdSetProtocolParameters command is used. Note
      /// that for SIP the default value will have "@" and the
      /// OpalParamRegistration::m_hostName field apepnded to it to create
      /// and Address-Of_Record.
      /// </summary>     
      public IntPtr m_identifier;

      /// <summary>
      ///  Host or domain name for server. For SIP this cannot be NULL.
      ///  For H.323 a NULL value indicates that a broadcast discovery is
      ///  be performed. If, for SIP, this contains an "@" and a user part
      ///  then a "third party" registration is performed.
      /// </summary>      
      public IntPtr m_hostName;

      /// <summary>
      ///  User name for authentication. 
      /// </summary>      
      public IntPtr m_authUserName;

      /// <summary>
      /// Password for authentication with server.
      /// </summary>      
      public IntPtr m_password;

      /// <summary>
      /// Identification of the administrative entity. For H.323 this will
      /// by the gatekeeper identifier. For SIP this is the authentication
      /// realm.
      /// </summary>      
      public IntPtr m_adminEntity;

      /// <summary>
      /// Time in seconds between registration updates. If this is zero then
      /// the identifier is unregistered from the server.
      /// </summary>
      public uint m_timeToLive;

      /// <summary>
      /// Time in seconds between attempts to restore a registration after
      /// registrar/gatekeeper has gone offline. If zero then a default
      /// value is used. 
      /// </summary>
      public uint m_restoreTime;

      /// <summary>
      /// If non-NULL then this indicates that a subscription is made
      /// rather than a registration. The string represents the particular
      /// event package being subscribed too.
      /// A value of OPAL_MWI_EVENT_PACKAGE will cause an
      /// OpalIndMessageWaiting to be sent.
      /// A value of OPAL_LINE_APPEARANCE_EVENT_PACKAGE will cause the
      /// OpalIndLineAppearance to be sent.
      /// Other values are currently not supported.
      /// </summary>      
      public IntPtr m_eventPackage;
    }

    /// <summary>
    /// Registration status for the OpalIndRegistration indication.
    ///   This is only returned from the OpalGetMessage() function.
    /// </summary>
    public struct OpalStatusRegistration
    {
      /// <summary>
      /// Protocol prefix for registration. Currently must be "h323" or
      /// "sip", is never NULL.
      /// </summary>      
      public IntPtr m_protocol;

      /// <summary>
      /// Name of the registration server. The exact format is protocol
      /// specific but generally contains the host or domain name, e.g.
      /// "GkId@gatekeeper.voxgratia.org" or "sip.voxgratia.org".
      /// </summary>      
      public IntPtr m_serverName;

      /// <summary>
      /// Error message for registration. If any error in the initial
      /// registration or any subsequent registration update occurs, then
      /// this contains a string indicating the type of error. If no
      /// error occured then this will be NULL.
      /// </summary>      
      public IntPtr m_error;

      /// <summary>
      /// Status of registration, see enum for details. 
      /// </summary>
      public OpalRegistrationStates m_status;

      /// <summary>
      /// Product description data 
      /// </summary>
      public OpalProductDescription m_product;
    }

    /// <summary>
    /// Set up call parameters for several command and indication messages.
    ///
    ///   When establishing a new call via the OpalCmdSetUpCall command, the m_partyA and
    ///   m_partyB fields indicate the parties to connect.
    ///
    ///   For OpalCmdTransferCall, m_partyA indicates the connection to be transferred and
    ///   m_partyB is the party to be transferred to. If the call transfer is successful then
    ///   a OpalIndCallCleared message will be received clearing the local call.
    ///
    ///   For OpalIndAlerting and OpalIndEstablished indications the three fields are set
    ///   to the data for the call in progress.
    ///
    ///   Example:
    ///   <code>
    ///      OpalMessage   command;
    ///      OpalMessage * response;
    ///
    ///      memset(&command, 0, sizeof(command));
    ///      command.m_type = OpalCmdSetUpCall;
    ///      command.m_param.m_callSetUp.m_partyB = "h323:10.0.1.11";
    ///      response = OpalSendMessage(hOPAL, &command);
    ///
    ///      memset(&command, 0, sizeof(command));
    ///      command.m_type = OpalCmdSetUpCall;
    ///      command.m_param.m_callSetUp.m_partyA = "pots:LINE1";
    ///      command.m_param.m_callSetUp.m_partyB = "sip:10.0.1.11";
    ///      response = OpalSendMessage(hOPAL, &command);
    ///      callToken = strdup(response->m_param.m_callSetUp.m_callToken);
    ///
    ///      memset(&command, 0, sizeof(command));
    ///      command.m_type = OpalCmdTransferCall;
    ///      command.m_param.m_callSetUp.m_callToken = callToken;
    ///      command.m_param.m_callSetUp.m_partyB = "sip:10.0.1.12";
    ///      response = OpalSendMessage(hOPAL, &command);
    ///      </code>
    /// </summary>  
    public struct OpalParamSetUpCall
    {
      /// <summary>
      /// A-Party for call.
      /// For OpalCmdSetUpCall, this indicates what subsystem will
      /// be starting the call, e.g. "pots:Handset One". If NULL
      /// or empty string then "pc:*" is used indication that the
      /// standard PC sound system ans screen is to be used.
      ///
      /// For OpalCmdTransferCall this indicates the party to be
      /// transferred, e.g. "sip:fred@nurk.com". If NULL then
      /// it is assumed that the party to be transferred is of the
      /// same "protocol" as the m_partyB field, e.g. "pc" or "sip".
      /// For OpalIndAlerting and OpalIndEstablished this indicates
      /// the A-party of the call in progress.
      /// </summary>      
      public IntPtr m_partyA;

      /// <summary>
      /// B-Party for call. This is typically a remote host URL
      /// address with protocol, e.g. "h323:simple.com" or
      /// "sip:fred@nurk.com".
      /// This must be provided in the OpalCmdSetUpCall and
      /// OpalCmdTransferCall commands, and is set by the system
      /// in the OpalIndAlerting and OpalIndEstablished indications.
      /// If used in the OpalCmdTransferCall command, this may be
      /// a valid call token for another call on hold. The remote
      /// is transferred to the call on hold and both calls are
      /// then cleared.
      /// </summary>      
      public IntPtr m_partyB;

      /// <summary>
      /// Value of call token for new call. The user would pass NULL
      /// for this string in OpalCmdSetUpCall, a new value is
      /// returned by the OpalSendMessage() function. The user would
      /// provide the call token for the call being transferred when
      /// OpalCmdTransferCall is being called.
      /// </summary>      
      public IntPtr m_callToken;

      /// <summary>
      /// The type of "distinctive ringing" for the call. The string
      /// is protocol dependent, so the caller would need to be aware
      /// of the type of call being made. Some protocols may ignore
      /// the field completely.
      ///
      /// For SIP this corresponds to the string contained in the
      /// "Alert-Info" header field of the INVITE. This is typically
      /// a URI for the ring file.
      /// 
      /// For H.323 this must be a string representation of an
      /// integer from 0 to 7 which will be contained in the
      /// Q.931 SIGNAL (0x34) Information Element.
      ///
      /// This is only used in OpalCmdSetUpCall to set the string to
      /// be sent to the remote to change the type of ring the remote
      /// may emit.
      ///
      /// For other indications this field is NULL.
      /// </summary>     
      public IntPtr m_alertingType;

      /// <summary>
      ///  ID assigned by the underlying protocol for the call. 
      ///  Only available in version 18 and above
      /// </summary>     
      public IntPtr m_protocolCallId;

      /// <summary>
      /// Overrides for the default parameters for the protocol.
      /// For example, m_userName and m_displayName can be
      /// changed on a call by call basis.
      /// </summary>
      public OpalParamProtocol m_overrides;
    }

    /// <summary>
    /// Incoming call information for the OpalIndIncomingCall indication.
    ///   This is only returned from the OpalGetMessage() function.
    /// </summary>
    public struct OpalStatusIncomingCall
    {
      /// <summary>
      /// Call token for new call.
      /// </summary>      
      public IntPtr m_callToken;

      /// <summary>
      /// URL of local interface. e.g. "sip:me@here.com"
      /// </summary>      
      public IntPtr m_localAddress;

      /// <summary>
      /// URL of calling party. e.g. "sip:them@there.com"
      /// </summary>      
      public IntPtr m_remoteAddress;

      /// <summary>
      /// This is the E.164 number of the caller, if available.
      /// </summary>      
      public IntPtr m_remotePartyNumber;

      /// <summary>
      ///  Display name calling party. e.g. "Fred Nurk"
      /// </summary>      
      public IntPtr m_remoteDisplayName;

      /// <summary>
      /// URL of called party the remote is trying to contact.
      /// </summary>      
      public IntPtr m_calledAddress;

      /// <summary>
      /// This is the E.164 number of the called party, if available.
      /// </summary>      
      public IntPtr m_calledPartyNumber;

      /// <summary>
      /// Product description data
      /// </summary>
      public OpalProductDescription m_product;

      /// <summary>
      /// The type of "distinctive ringing" for the call. The string
      /// is protocol dependent, so the caller would need to be aware
      /// of the type of call being made. Some protocols may ignore
      /// the field completely.
      ///
      /// For SIP this corresponds to the string contained in the
      /// "Alert-Info" header field of the INVITE. This is typically
      /// a URI for the ring file.
      ///
      /// For H.323 this must be a string representation of an
      /// integer from 0 to 7 which will be contained in the
      /// Q.931 SIGNAL (0x34) Information Element.
      /// </summary>      
      public IntPtr m_alertingType;

      /// <summary>
      /// ID assigned by the underlying protocol for the call. 
      /// Only available in version 18 and above
      /// </summary>      
      public IntPtr m_protocolCallId;

      /// <summary>
      /// This is the full address of the party doing transfer, if available.
      /// </summary>      
      public IntPtr m_referredByAddress;

      /// <summary>
      /// This is the E.164 number of the party doing transfer, if available.
      /// </summary>      
      public IntPtr m_redirectingNumber;
    }

    /// <summary>
    /// Incoming call response parameters for OpalCmdAlerting and OpalCmdAnswerCall messages.
    ///
    ///   When a new call is detected via the OpalIndIncomingCall indication, the application
    ///   should respond with OpalCmdClearCall, which does not use this structure, or
    ///   OpalCmdAnswerCall, which does. An optional OpalCmdAlerting may also be sent
    ///   which also uses this structure to allow for the override of default call parameters
    ///   such as suer name or display name on a call by call basis.
    /// </summary>  
    public struct OpalParamAnswerCall
    {
      /// <summary>
      /// Call token for call to be answered.
      /// </summary>     
      public IntPtr m_callToken;

      /// <summary>
      /// Overrides for the default parameters for the protocol.
      /// For example, m_userName and m_displayName can be
      /// changed on a call by call basis.
      /// </summary>                         
      public OpalParamProtocol m_overrides;
    }

    /// <summary>
    /// User input information for the OpalIndUserInput/OpalCmdUserInput indication.
    ///
    /// This is may be returned from the OpalGetMessage() function or
    /// provided to the OpalSendMessage() function.
    /// </summary>
    public struct OpalStatusUserInput
    {
      /// <summary>
      /// Call token for the call the User Input was received on.
      /// </summary>      
      public IntPtr m_callToken;

      /// <summary>
      /// User input string, e.g. "#".
      /// </summary>      
      public IntPtr m_userInput;

      /// <summary>
      /// Duration in milliseconds for tone. For DTMF style user input
      /// the time the tone was detected may be placed in this field.
      /// Generally zero is passed which means the m_userInput is a
      /// single "string" input. If non-zero then m_userInput must
      /// be a single character.
      /// </summary>
      public uint m_duration;
    }

    /// <summary>
    /// Message Waiting information for the OpalIndMessageWaiting indication.
    ///   This is only returned from the OpalGetMessage() function.
    /// </summary>
    public struct OpalStatusMessageWaiting
    {
      /// <summary>
      /// Party for which the MWI is directed
      /// </summary>      
      public IntPtr m_party;

      /// <summary>
      /// Type for MWI, "Voice", "Fax", "Pager", "Multimedia", "Text", "None"
      /// </summary>      
      public IntPtr m_type;

      /// <summary>
      /// Extra information for the MWI, e.g. "SUBSCRIBED",
      /// "UNSUBSCRIBED", "2/8 (0/2)
      /// </summary>      
      public IntPtr m_extraInfo;
    }

    /// <summary>
    /// Line Appearance information for the OpalIndLineAppearance indication.
    /// This is only returned from the OpalGetMessage() function.
    /// </summary>
    public struct OpalStatusLineAppearance
    {
      /// <summary>
      /// URI for the line whose state is changing
      /// </summary>      
      public IntPtr m_line;

      /// <summary>
      /// State the line has just moved to.
      /// </summary>
      public OpalLineAppearanceStates m_state;

      /// <summary>
      /// Appearance code, this is an arbitrary integer
      /// and is defined by the remote servers. If negative
      /// then it is undefined.
      /// </summary>
      public int m_appearance;

      /// <summary>
      /// If line is "in use" then this gives information
      /// that identifies the call. Note that this will
      /// include the from/to "tags" that can identify
      /// the dialog for REFER/Replace.
      /// </summary>      
      public IntPtr m_callId;

      /// <summary>
      /// A-Party for call.
      /// </summary>      
      public IntPtr m_partyA;

      /// <summary>
      ///  B-Party for call.
      /// </summary>      
      public IntPtr m_partyB;
    }

    /// <summary>
    /// Call clearance information for the OpalIndCallCleared indication.
    ///   This is only returned from the OpalGetMessage() function.
    /// </summary>
    public struct OpalStatusCallCleared
    {
      /// <summary>
      /// Call token for call being cleared.
      /// </summary>      
      public IntPtr m_callToken;

      /// <summary>
      /// String representing the reason for the call
      /// completing. This string begins with a numeric
      /// code corresponding to values in the
      /// OpalCallEndReason enum, followed by a colon and
      /// an English description.
      /// </summary>      
      public IntPtr m_reason;
    }

    /// <summary>
    /// Call clearance information for the OpalCmdClearCall command.
    /// </summary>
    public struct OpalParamCallCleared
    {
      /// <summary>
      /// Call token for call being cleared.
      /// </summary>      
      public IntPtr m_callToken;

      /// <summary>
      /// Code for the call termination to be provided to the
      /// remote system.
      /// </summary>                            
      public OpalCallEndReason m_reason;
    }

    /// <summary>
    /// Media stream information for the OpalIndMediaStream indication and
    ///   OpalCmdMediaStream command.
    ///
    ///   This is may be returned from the OpalGetMessage() function or
    ///   provided to the OpalSendMessage() function.
    /// </summary>
    public struct OpalStatusMediaStream
    {
      /// <summary>
      ///  Call token for the call the media stream is.
      /// </summary>     
      public IntPtr m_callToken;

      /// <summary>
      /// Unique identifier for the media stream. For OpalCmdMediaStream
      /// this may be left empty and the first stream of the type
      /// indicated by m_mediaType is used.
      /// </summary>      
      public IntPtr m_identifier;

      /// <summary>
      /// Media type and direction for the stream. This is a keyword such
      /// as "audio" or "video" indicating the type of the stream, a space,
      /// then either "in" or "out" indicating the direction. For
      /// OpalCmdMediaStream this may be left empty if m_identifier is
      /// used.
      /// </summary>      
      public IntPtr m_type;

      /// <summary>
      /// Media format for the stream. For OpalIndMediaStream this
      /// shows the format being used. For OpalCmdMediaStream this
      /// is the format to use. In the latter case, if empty or
      /// NULL, then a default is used. 
      /// </summary>      
      public IntPtr m_format;

      /// <summary>
      /// For OpalIndMediaStream this indicates the status of the stream.
      /// For OpalCmdMediaStream this indicates the state to move to, see
      /// OpalMediaStates for more information.
      /// </summary>
      public OpalMediaStates m_state;

      /// <summary>
      /// Set the volume for the media stream as a percentage. Note this
      /// is dependent on the stream type and may be ignored. Also, a
      /// percentage of zero does not indicate muting, it indicates no
      /// change in volume. Use -1, to mute.
      /// </summary>
      public int m_volume;
    }

    /// <summary>
    /// Assign a user data field to a call
    /// </summary>
    public struct OpalParamSetUserData
    {
      /// <summary>
      /// Call token for the call the media stream is.
      /// </summary>      
      public IntPtr m_callToken;

      /// <summary>
      /// user data value to associate with this call
      /// </summary>
      public IntPtr m_userData;
    }

    /// <summary>
    /// Call recording information for the OpalCmdStartRecording command.
    /// </summary>
    public struct OpalParamRecording
    {
      /// <summary>
      /// Call token for call being recorded.
      /// </summary>      
      public IntPtr m_callToken;

      /// <summary>
      /// File to record into. If NULL then a test is done
      /// for if recording is currently active.
      /// </summary>     
      public IntPtr m_file;

      /// <summary>
      /// Number of channels in WAV file, 1 for mono (default) or 2 for
      /// stereo where incoming & outgoing audio are in individual
      /// channels.
      /// </summary>
      public uint m_channels;

      /// <summary>
      /// Audio recording format. This is generally an OpalMediaFormat
      /// name which will be used in the recording file. The exact
      /// values possible is dependent on many factors including the
      /// specific file type and what codecs are loaded as plug ins.
      /// </summary>     
      public IntPtr m_audioFormat;

      /// <summary>
      /// Video recording format. This is generally an OpalMediaFormat
      /// name which will be used in the recording file. The exact
      /// values possible is dependent on many factors including the
      /// specific file type and what codecs are loaded as plug ins.
      /// </summary>      
      public IntPtr m_videoFormat;

      /// <summary>
      /// Width of image for recording video.
      /// </summary>
      public uint m_videoWidth;

      /// <summary>
      /// Height of image for recording video.
      /// </summary>
      public uint m_videoHeight;

      /// <summary>
      /// Frame rate for recording video.
      /// </summary>
      public uint m_videoRate;

      /// <summary>
      /// How the two images are saved in video recording.
      /// </summary>
      public OpalVideoRecordMixMode m_videoMixing;
    }

    /// <summary>
    /// Call transfer information for the OpalIndTransferCall indication.
    /// This is only returned from the OpalGetMessage() function.
    /// </summary>
    public struct OpalStatusTransferCall
    {
      /// <summary>
      /// Call token for call being transferred.
      /// </summary>      
      public IntPtr m_callToken;

      /// <summary>
      /// ID assigned by the underlying protocol for the call. 
      /// Only available in version 18 and above
      /// </summary>      
      public IntPtr m_protocolCallId;

      /// <summary>
      /// Result of transfer operation. This is one of:
      /// "progress"  transfer of this call is still in progress.
      /// "success"   transfer of this call completed, call will
      ///             be cleared.
      /// "failed"    transfer initiated by this call did not
      ///             complete, call remains active.
      /// "started"   remote system has asked local connection
      ///             to transfer to another target.
      /// "completed" local connection has completed the transfer
      ///             to other target.
      /// "forwarded" remote has forwarded call local system has
      ///             initiated to another address.
      /// "incoming"  this call is the target of an incoming
      ///             transfer, e.g. party C in a consultation
      ///             transfer scenario.
      /// </summary>      
      public IntPtr m_result;

      /// <summary>
      /// Protocol dependent information in the form:
      /// key=value\n
      /// key=value\n
      /// etc
      /// </summary>      
      public IntPtr m_info;
    }

    /// <summary>
    /// IVR information for the OpalIndCompletedIVR indication.
    /// This is only returned from the OpalGetMessage() function.
    /// </summary>
    public struct OpalStatusIVR
    {
      /// <summary>
      /// Call token for call being cleared.
      /// </summary>      
      public IntPtr m_callToken;

      /// <summary>
      /// Final values for variables defined by the script.
      /// These will be in the form:
      /// varname=value\n
      /// varname=value\n
      /// etc
      /// </summary>      
      public IntPtr m_variables;
    }

    /// <summary>
    /// Product description variables.
    /// </summary>
    public struct OpalProductDescription
    {
      /// <summary>
      /// Name of the vendor or manufacturer of the application. This is
      /// used to identify the software which can be very useful for
      /// solving interoperability issues. e.g. "Vox Lucida".
      /// </summary>      
      public IntPtr m_vendor;

      /// <summary>
      /// Name of the product within the vendor name space. This is
      /// used to identify the software which can be very useful for
      /// solving interoperability issues. e.g. "OpenPhone".
      /// </summary>      
      public IntPtr m_name;

      /// <summary>
      /// Version of the product within the vendor/product name space. This
      /// is used to identify the software which can be very useful for
      /// solving interoperability issues. e.g. "2.1.4".
      /// </summary>      
      public IntPtr m_version;

      /// <summary>
      /// T.35 country code for the name space in which the vendor or
      /// manufacturer is identified. This is the part of the H.221
      /// equivalent of the m_vendor string above and  used to identify the
      /// software which can be very useful for solving interoperability
      /// issues. e.g. 9 is for Australia.
      /// </summary>
      public uint m_t35CountryCode;

      /// <summary>
      /// T.35 country extension code for the name space in which the vendor or
      /// manufacturer is identified. This is the part of the H.221
      /// equivalent of the m_vendor string above and  used to identify the
      /// software which can be very useful for solving interoperability
      /// issues. Very rarely used.
      /// </summary>
      public uint m_t35Extension;

      /// <summary>
      ///  Manuacturer code for the name space in which the vendor or
      ///  manufacturer is identified. This is the part of the H.221
      ///  equivalent of the m_vendor string above and  used to identify the
      ///  software which can be very useful for solving interoperability
      ///  issues. e.g. 61 is for Equivalance and was allocated by the
      ///  Australian Communications Authority, Oct 2000.
      /// </summary>
      public uint m_manufacturerCode;
    }
    #endregion
   
    /// <summary>
    /// Initialise the OPAL system, returning a "handle" to the system that must
    ///be used in other calls to OPAL.
    ///
    ///The version parameter indicates the version of the API being used by the
    ///caller. It should always be set to the constant OPAL_C_API_VERSION. On
    ///return the library will indicate the API version it supports, if it is
    ///lower than that provided by the application.
    ///
    ///The C string options are space separated tokens indicating various options
    ///to be enabled, for example the protocols to be available. NULL or an empty
    ///string will load all available protocols. The current protocol tokens are:
    ///    <code>
    ///    sip sips h323 h323s iax2 pc local pots pstn ivr
    ///    </code>
    ///The above protocols are in priority order, so if a protocol is not
    ///explicitly in the address, then the first one of the opposite "category"
    ///s used. There are two categories, network protocols (sip, h323, iax & pstn)
    ///and non-network protocols (pc, local, pots & ivr).
    ///
    ///Additional options are:
    ///    <table border=0>
    ///    <tr><td>TraceLevel=1     <td>Level for tracing.
    ///    <tr><td>TraceAppend      <td>Append to the trace file.
    ///    <tr><td>TraceFile="name" <td>Set the filename for trace output. Note quotes are
    ///                                 required if spaces are in filename.
    ///    </table>
    ///It should also be noted that there must not be spaces around the '=' sign
    ///in the above options.
    ///
    ///If NULL is returned then an initialisation error occurred. This can only
    ///really occur if the user specifies prefixes which are not supported by
    ///the library.

    ///Example:
    ///  <code>
    ///  OpalHandle hOPAL;
    ///  unsigned   version;
    ///
    ///  version = OPAL_C_API_VERSION;
    ///  if ((hOPAL = OpalInitialise(&version,
    ///                              OPAL_PREFIX_H323  " "
    ///                              OPAL_PREFIX_SIP   " "
    ///                              OPAL_PREFIX_IAX2  " "
    ///                              OPAL_PREFIX_PCSS
    ///                              " TraceLevel=4")) == NULL) {
    ///    fputs("Could not initialise OPAL\n", stderr);
    ///    return false;
    ///  }
    ///  </code>
    /// </summary>        
    [DllImport("OPAL.DLL", CallingConvention = CallingConvention.StdCall)]
    public static extern IntPtr OpalInitialise(ref uint version, [MarshalAs(UnmanagedType.LPStr)] string options);

    /// <summary>
    /// Shut down and clean up all resource used by the OPAL system. The parameter
    ///must be the handle returned by OpalInitialise().
    ///
    ///Example:
    ///<code>
    ///OpalShutDown(hOPAL);
    ///</code>
    /// </summary>
    /// <param name="opal"></param>
    [DllImport("OPAL.DLL", CallingConvention = CallingConvention.StdCall)]
    public static extern void OpalShutDown(IntPtr opal);

    /// <summary>
    /// Get a message from the OPAL system. The first parameter must be the handle
    /// returned by OpalInitialise(). The second parameter is a timeout in
    /// milliseconds. NULL is returned if a timeout occurs. A value of UINT_MAX
    /// will wait forever for a message.
    ///
    /// The returned message must be disposed of by a call to OpalFreeMessage().
    ///
    /// The OPAL system will serialise all messages returned from this function to
    /// avoid any multi-threading issues. If the application wishes to avoid even
    /// this small delay, there is a callback function that may be configured that
    /// is not thread safe but may be used to get the messages as soon as they are
    /// generated. See OpalCmdSetGeneralParameters.
    ///
    /// Note if OpalShutDown() is called from a different thread then this function
    /// will break from its block and return NULL.
    ///
    /// Example:
    /// <code>
    ///  OpalMessage * message;
    ///    
    ///  while ((message = OpalGetMessage(hOPAL, timeout)) != NULL) {
    ///    switch (message->m_type) {
    ///      case OpalIndRegistration :
    ///        HandleRegistration(message);
    ///        break;
    ///      case OpalIndIncomingCall :
    ///        Ring(message);
    ///        break;
    ///      case OpalIndCallCleared :
    ///        HandleHangUp(message);
    ///        break;
    ///    }
    ///    FreeMessageFunction(message);
    ///  }
    /// </code>
    /// </summary>
    [DllImport("OPAL.DLL", CallingConvention = CallingConvention.StdCall)]    
    public static extern IntPtr OpalGetMessage(IntPtr opal, uint timeout);

    /// <summary>
    /// Send a message to the OPAL system. The first parameter must be the handle
    /// returned by OpalInitialise(). The second parameter is a constructed message
    /// which is a command to the OPAL system.
    ///
    /// Within the command message, generally a NULL or empty string, or zero value
    /// for integral types, indicates the particular parameter is to be ignored.
    /// Documentation on individiual messages will indicate which are mandatory.
    ///
    /// The return value is another message which will have a type of
    /// OpalIndCommandError if an error occurs. The OpalMessage::m_commandError field
    /// will contain a string indicating the error that occurred.
    ///
    /// If successful, the the type of the message is the same as the command type.
    /// The message fields in the return will generally be set to the previous value
    /// for the field, where relevant. For example in the OpalCmdSetGeneralParameters
    /// command the OpalParamGeneral::m_stunServer would contain the STUN server name
    /// prior to the command.
    ///
    /// A NULL is only returned if the either OpalHandle or OpalMessage parameters is NULL.
    ///
    /// The returned message must be disposed of by a call to OpalFreeMessage().
    ///
    /// Example:
    ///  <code>
    ///  void SendCommand(OpalMessage * command)
    ///  {
    ///    OpalMessage * response;
    ///    if ((response = OpalSendMessage(hOPAL, command)) == NULL) {
    ///      puts("OPAL not initialised.");
    ///    else if (response->m_type != OpalIndCommandError)
    ///      HandleResponse(response);
    ///    else if (response->m_param.m_commandError == NULL || *response->m_param.m_commandError == '\\0')
    ///      puts("OPAL error.");
    ///    else
    ///      printf("OPAL error: %s\n", response->m_param.m_commandError);
    ///
    ///    FreeMessageFunction(response);
    ///  }
    ///  </code>
    /// </summary>    
    [DllImport("OPAL.DLL", CallingConvention = CallingConvention.StdCall)]    
    public static extern IntPtr OpalSendMessage(IntPtr opal, ref OpalMessage message);

    /// <summary>
    /// Free memeory in message the OPAL system has sent. The parameter must be
    /// the message returned by OpalGetMessage() or OpalSendMessage().
    /// </summary>  
    [DllImport("OPAL.DLL", CallingConvention = CallingConvention.StdCall)]
    public static extern void OpalFreeMessage(ref OpalMessage message);    
  }
}
