/*
 * connection.h
 *
 * Telephony connection abstraction
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
 * Contributor(s): Post Increment
 *     Portions of this code were written with the assistance of funding from
 *     US Joint Forces Command Joint Concept Development & Experimentation (J9)
 *     http://www.jfcom.mil/about/abt_j9.htm
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#ifndef __OPAL_CONNECTION_H
#define __OPAL_CONNECTION_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <opal/buildopts.h>

#include <opal/mediafmt.h>
#include <opal/mediastrm.h>
#include <opal/guid.h>
#include <opal/transports.h>
#include <ptclib/dtmf.h>
#include <ptlib/safecoll.h>
#include <rtp/rtp.h>

class OpalEndPoint;
class OpalCall;
class OpalSilenceDetector;
class OpalEchoCanceler;
class OpalRFC2833Proto;
class OpalRFC2833Info;
class OpalT120Protocol;
class OpalT38Protocol;
class OpalH224Handler;
class OpalH281Handler;


/** Class for carying vendor/product information.
  */
class OpalProductInfo
{
  public:
    OpalProductInfo();

    static OpalProductInfo & Default();

    PCaselessString AsString() const;

    PString vendor;
    PString name;
    PString version;
    BYTE    t35CountryCode;
    BYTE    t35Extension;
    WORD    manufacturerCode;
};


/**This is the base class for connections to an endpoint.
   A particular protocol will have a descendant class from this to implement
   the specific semantics of that protocols connection.

   A connection is part of a call, and will be "owned" by an OpalCall object.
   It is also attached to the creator endpoint to do any protocol specific
   management of the connection. However the deletion of the connection is
   done by a special thread in the OpalManager class. A connnection should
   never be deleted directly.

   The connection is also in charge of creating media streams. It may do this
   in respose to an explicit call to OpenMediaStream or implicitly due to
   requests in the underlying protocol.

   When media streams are created they must make requests for bandwidth which
   is managed by the connection.
 */
class OpalConnection : public PSafeObject
{
    PCLASSINFO(OpalConnection, PSafeObject);
  public:
    /**Call/Connection ending reasons.
       NOTE: if anything is added to this, you also need to add the field to
       the tables in connection.cxx and h323pdu.cxx.
      */
    enum CallEndReason {
      EndedByLocalUser,         /// Local endpoint application cleared call
      EndedByNoAccept,          /// Local endpoint did not accept call OnIncomingCall()=PFalse
      EndedByAnswerDenied,      /// Local endpoint declined to answer call
      EndedByRemoteUser,        /// Remote endpoint application cleared call
      EndedByRefusal,           /// Remote endpoint refused call
      EndedByNoAnswer,          /// Remote endpoint did not answer in required time
      EndedByCallerAbort,       /// Remote endpoint stopped calling
      EndedByTransportFail,     /// Transport error cleared call
      EndedByConnectFail,       /// Transport connection failed to establish call
      EndedByGatekeeper,        /// Gatekeeper has cleared call
      EndedByNoUser,            /// Call failed as could not find user (in GK)
      EndedByNoBandwidth,       /// Call failed as could not get enough bandwidth
      EndedByCapabilityExchange,/// Could not find common capabilities
      EndedByCallForwarded,     /// Call was forwarded using FACILITY message
      EndedBySecurityDenial,    /// Call failed a security check and was ended
      EndedByLocalBusy,         /// Local endpoint busy
      EndedByLocalCongestion,   /// Local endpoint congested
      EndedByRemoteBusy,        /// Remote endpoint busy
      EndedByRemoteCongestion,  /// Remote endpoint congested
      EndedByUnreachable,       /// Could not reach the remote party
      EndedByNoEndPoint,        /// The remote party is not running an endpoint
      EndedByHostOffline,       /// The remote party host off line
      EndedByTemporaryFailure,  /// The remote failed temporarily app may retry
      EndedByQ931Cause,         /// The remote ended the call with unmapped Q.931 cause code
      EndedByDurationLimit,     /// Call cleared due to an enforced duration limit
      EndedByInvalidConferenceID, /// Call cleared due to invalid conference ID
      EndedByNoDialTone,        /// Call cleared due to missing dial tone
      EndedByNoRingBackTone,    /// Call cleared due to missing ringback tone
      EndedByOutOfService,      /// Call cleared because the line is out of service, 
      EndedByAcceptingCallWaiting, /// Call cleared because another call is answered
      NumCallEndReasons,

      EndedWithQ931Code = 0x100  /// Q931 code specified in MS byte
    };

#if PTRACING
    friend ostream & operator<<(ostream & o, CallEndReason reason);
#endif

    enum AnswerCallResponse {
      AnswerCallNow,               /// Answer the call continuing with the connection.
      AnswerCallDenied,            /// Refuse the call sending a release complete.
      AnswerCallPending,           /// Send an Alerting PDU and wait for AnsweringCall()
      AnswerCallDeferred,          /// As for AnswerCallPending but does not send Alerting PDU
      AnswerCallAlertWithMedia,    /// As for AnswerCallPending but starts media channels
      AnswerCallDeferredWithMedia, /// As for AnswerCallDeferred but starts media channels
      AnswerCallProgress,          /// Answer the call with a h323 progress, or sip 183 session in progress, or ... 
      AnswerCallNowAndReleaseCurrent, /// Answer the call and destroy the current call
      NumAnswerCallResponses
    };
#if PTRACING
    friend ostream & operator<<(ostream & o, AnswerCallResponse s);
#endif

    /** Connection options
    */
    enum Options {
      FastStartOptionDisable       = 0x0001,   // H.323 specific
      FastStartOptionEnable        = 0x0002,
      FastStartOptionMask          = 0x0003,

      H245TunnelingOptionDisable   = 0x0004,   // H.323 specific
      H245TunnelingOptionEnable    = 0x0008,
      H245TunnelingOptionMask      = 0x000c,

      H245inSetupOptionDisable     = 0x0010,   // H.323 specific
      H245inSetupOptionEnable      = 0x0020,
      H245inSetupOptionMask        = 0x0030,

      DetectInBandDTMFOptionDisable = 0x0040,  // SIP and H.323
      DetectInBandDTMFOptionEnable  = 0x0080,
      DetectInBandDTMFOptionMask    = 0x00c0,

      RTPAggregationDisable        = 0x0100,   // SIP and H.323
      RTPAggregationEnable         = 0x0200,
      RTPAggregationMask           = 0x0300,

      SendDTMFAsDefault            = 0x0000,   // SIP and H.323
      SendDTMFAsString             = 0x0400,
      SendDTMFAsTone               = 0x0800,
      SendDTMFAsRFC2833            = 0x0c00,
      SendDTMFMask                 = 0x0c00
    };

    class StringOptions : public PStringToString 
    {
    };

  /**@name Construction */
  //@{
    /**Create a new connection.
     */
    OpalConnection(
      OpalCall & call,                         ///<  Owner calll for connection
      OpalEndPoint & endpoint,                 ///<  Owner endpoint for connection
      const PString & token,                   ///<  Token to identify the connection
      unsigned options = 0,                    ///<  Connection options
      OpalConnection::StringOptions * stringOptions = NULL     ///< more complex options
    );  

    /**Destroy connection.
     */
    ~OpalConnection();
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

  /**@name Basic operations */
  //@{
    enum Phases {
      UninitialisedPhase,
      SetUpPhase,
      AlertingPhase,
      ConnectedPhase,
      EstablishedPhase,
      ReleasingPhase, // Note these must be the last two phases.
      ReleasedPhase,
      NumPhases
    };

    /**Get the phase of the connection.
       This indicates the current phase of the connection sequence. Whether
       all phases and the transitions between phases is protocol dependent.
      */
    inline Phases GetPhase() const { return phase; }

    /**Get the call clearand reason for this connection shutting down.
       Note that this function is only generally useful in the
       H323EndPoint::OnConnectionCleared() function. This is due to the
       connection not being cleared before that, and the object not even
       exiting after that.

       If the call is still active then this will return NumCallEndReasons.
      */
    CallEndReason GetCallEndReason() const { return callEndReason; }

    /**Set the call clearance reason.
       An application should have no cause to use this function. It is present
       for the H323EndPoint::ClearCall() function to set the clearance reason.
      */
    virtual void SetCallEndReason(
      CallEndReason reason        ///<  Reason for clearance of connection.
    );

    /**Clear a current call.
       This hangs up the current call. This will release all connections
       currently in the call by calling the OpalCall::Clear() function.

       Note that this function will return quickly as the release and
       disposal of the connections is done by another thread.
      */
    void ClearCall(
      CallEndReason reason = EndedByLocalUser ///<  Reason for call clearing
    );

    /**Clear a current connection, synchronously
      */
    virtual void ClearCallSynchronous(
      PSyncPoint * sync,
      CallEndReason reason = EndedByLocalUser  ///<  Reason for call clearing
    );

    /**Get the Q.931 cause code (Q.850) that terminated this call.
       See Q931::CauseValues for common values.
     */
    unsigned GetQ931Cause() const { return q931Cause; }

    /**Set the outgoing Q.931 cause code (Q.850) that is sent for this call
       See Q931::CauseValues for common values.
     */
    void SetQ931Cause(unsigned v) { q931Cause = v; }

    /**Initiate the transfer of an existing call (connection) to a new remote 
       party.
     */
    virtual void TransferConnection(
      const PString & remoteParty,   ///<  Remote party to transfer the existing call to
      const PString & callIdentity = PString::Empty()
                                    ///<  Call Identity of secondary call if present
    );
    
    /**Put the current connection on hold, suspending all media streams.
     */
    virtual void HoldConnection();

    /**Retrieve the current connection from hold, activating all media 
     * streams.
     */
    virtual void RetrieveConnection();

    /**Return PTrue if the current connection is on hold.
     */
    virtual PBoolean IsConnectionOnHold();
  //@}

  /**@name Call progress functions */
  //@{
    /**Call back for an incoming call.
       This function is used for an application to control the answering of
       incoming calls.

       If PTrue is returned then the connection continues. If PFalse then the
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

       Note that the most explicit version of this override is made pure, so as to force 
       descendant classes to implement it. This will only affect code that implements new
       descendants of OpalConnection - code that uses existing descendants will be unaffected
     */
    virtual PBoolean OnIncomingConnection(unsigned int options, OpalConnection::StringOptions * stringOptions);
    virtual PBoolean OnIncomingConnection(unsigned int options);
    virtual PBoolean OnIncomingConnection();

    /**Start an outgoing connection.
       This function will initiate the connection to the remote entity, for
       example in H.323 it sends a SETUP, in SIP it sends an INVITE etc.

       The default behaviour is pure.
      */
    virtual PBoolean SetUpConnection() = 0;

    /**Callback for outgoing connection, it is invoked after SetUpConnection
       This function allows the application to set up some parameters or to log some messages
     */
    virtual PBoolean OnSetUpConnection();

    
    /**Call back for remote party being alerted.
       This function is called after the connection is informed that the
       remote endpoint is "ringing". Generally some time after the
       SetUpConnection() function was called, this is function is called.

       If PFalse is returned the connection is aborted.

       If an application overrides this function, it should generally call the
       ancestor version for correct operation. An application would typically
       only intercept this function if it wishes to do some form of logging.
       For this you can obtain the name of the caller by using the function
       OpalConnection::GetRemotePartyName().

       The default behaviour calls the OpalEndPoint function of the same name.
     */
    virtual void OnAlerting();

    /**Indicate to remote endpoint an alert is in progress.
       If this is an incoming connection and the AnswerCallResponse is in a
       AnswerCallDeferred or AnswerCallPending state, then this function is
       used to indicate to that endpoint that an alert is in progress. This is
       usually due to another connection which is in the call (the B party)
       has received an OnAlerting() indicating that its remoteendpoint is
       "ringing".

       The default behaviour is pure.
      */
    virtual PBoolean SetAlerting(
      const PString & calleeName,   ///<  Name of endpoint being alerted.
      PBoolean withMedia                ///<  Open media with alerting
    ) = 0;

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

       The default behaviour calls the endpoint function of the same name.
     */
    virtual AnswerCallResponse OnAnswerCall(
      const PString & callerName        ///<  Name of caller
    );

    /**Indicate the result of answering an incoming call.
       This should only be called if the OnAnswerCall() callback function has
       returned a AnswerCallPending or AnswerCallDeferred response.

       Note sending further AnswerCallPending responses via this function will
       have the result of notification PDUs being sent to the remote endpoint (if possible).
       In this way multiple notification PDUs may be sent.

       Sending a AnswerCallDeferred response would have no effect.
      */
    virtual void AnsweringCall(
      AnswerCallResponse response ///<  Answer response to incoming call
    );

    /**A call back function whenever a connection is "connected".
       This indicates that a connection to an endpoint was connected. That
       is the endpoint received acknowledgement via whatever protocol it uses
       that the connection may now start media streams.

       In the context of H.323 this means that the CONNECT pdu has been
       received.

       The default behaviour calls the OpalEndPoint function of the same name.
      */
    virtual void OnConnected();

    /**Indicate to remote endpoint we are connected.

       The default behaviour is pure.
      */
    virtual PBoolean SetConnected() = 0;

    /**A call back function whenever a connection is established.
       This indicates that a connection to an endpoint was established. This
       differs from OnConnected() in that the media streams are started.

       In the context of H.323 this means that the signalling and control
       channels are open and the TerminalCapabilitySet and MasterSlave
       negotiations are complete.

       The default behaviour calls the OpalEndPoint function of the same name.
      */
    virtual void OnEstablished();

    /**Release the current connection.
       This removes the connection from the current call. The call may
       continue if there are other connections still active on it. If this was
       the last connection for the call then the call is disposed of as well.

       Note that this function will return quickly as the release and
       disposal of the connections is done by another thread.
      */
    virtual void Release(
      CallEndReason reason = EndedByLocalUser ///<  Reason for call release
    );

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

       The default behaviour calls the OpalEndPoint function of the same name.
      */
    virtual void OnReleased();
  //@}

  /**@name Additional signalling functions */
  //@{
    /**Get the destination address of an incoming connection.
       This will, for example, collect a phone number from a POTS line, or
       get the fields from the H.225 SETUP pdu in a H.323 connection.

       The default behaviour returns "*", which by convention means any
       address the endpoint/connection can get to.
      */
    virtual PString GetDestinationAddress();

    /**Forward incoming call to specified address.
       This would typically be called from within the OnIncomingCall()
       function when an application wishes to redirct an unwanted incoming
       call.

       The return value is PTrue if the call is to be forwarded, PFalse
       otherwise. Note that if the call is forwarded the current connection is
       cleared with teh ended call code of EndedByCallForwarded.
      */
    virtual PBoolean ForwardCall(
      const PString & forwardParty   ///<  Party to forward call to.
    );
  //@}

  /**@name Media Stream Management */
  //@{
    /**Get the data formats this connection is capable of operating.
       This provides a list of media data format names that a
       OpalMediaStream may be created in within this connection.

       The default behaviour is pure.
      */
    virtual OpalMediaFormatList GetMediaFormats() const = 0;

    /**Get the list of data formats used for making calls
       
       The default behaviour is to call call.GetMediaFormats();
      */
    virtual OpalMediaFormatList GetLocalMediaFormats();

    /**Adjust media formats available on a connection.
       This is called by a connection after it has called
       OpalCall::GetMediaFormats() to get all media formats that it can use so
       that an application may remove or reorder the media formats before they
       are used to open media streams.

       The default behaviour calls the OpalEndPoint function of the same name.
      */
    virtual void AdjustMediaFormats(
      OpalMediaFormatList & mediaFormats  ///<  Media formats to use
    ) const;
    
    /**Open source or sink media stream for session.
      */
    virtual OpalMediaStreamPtr OpenMediaStream(
      const OpalMediaFormat & mediaFormat, ///<  Media format to open
      unsigned sessionID,                  ///<  Session to start stream on
      bool isSource                        ///< Stream is a source/sink
    );

    /**Request close of a media stream by session.
       Note that this is usually asymchronous, the OnClosedMediaStream() function is
       called when the stream is really closed.
      */
    virtual bool CloseMediaStream(
      unsigned sessionId,  ///<  Session ID to search for.
      bool source          ///<  Indicates the direction of stream.
    );

    /**Request close of a specific media stream.
       Note that this is usually asymchronous, the OnClosedMediaStream() function is
       called when the stream is really closed.
      */
    virtual bool CloseMediaStream(
      OpalMediaStream & stream  ///< Stream to close
    );

    /**Remove the specified media stream from the list of streams for this channel.
       This will automatically delete the stream if the stream was found in the
       stream list.

      Returns true if the media stream was removed the list and deleted, else
      returns false if the media stream was unchanged
      */
    bool RemoveMediaStream(
      OpalMediaStream & strm     // media stream to remove
    );

    /**Start all media streams for connection.
      */
    virtual void StartMediaStreams();
    
    /**Request close all media streams on connection.
      */
    virtual void CloseMediaStreams();
    
    /**Pause media streams for connection.
      */
    virtual void PauseMediaStreams(PBoolean paused);

    /**Create a new media stream.
       This will create a media stream of an appropriate subclass as required
       by the underlying connection protocol. For instance H.323 would create
       an OpalRTPStream.

       The sessionID parameter may not be needed by a particular media stream
       and may be ignored. In the case of an OpalRTPStream it us used.

       Note that media streams may be created internally to the underlying
       protocol. This function is not the only way a stream can come into
       existance.
     */
    virtual OpalMediaStream * CreateMediaStream(
      const OpalMediaFormat & mediaFormat, ///<  Media format for stream
      unsigned sessionID,                  ///<  Session number for stream
      PBoolean isSource                        ///<  Is a source stream
    );

    /**Get a media stream.
       Locates a stream given a RTP session ID. Each session would usually
       have two media streams associated with it, so the source flag
       may be used to distinguish which channel to return.
      */
    OpalMediaStreamPtr GetMediaStream(
      unsigned sessionId,  ///<  Session ID to search for.
      bool source          ///<  Indicates the direction of stream.
    ) const;

    /**Call back when opening a media stream.
       This function is called when a connection has created a new media
       stream according to the logic of its underlying protocol.

       The usual requirement is that media streams are created on all other
       connections participating in the call and all of the media streams are
       attached to an instance of an OpalMediaPatch object that will read from
       one of the media streams passing data to the other media streams.

       The default behaviour calls the OpalEndPoint function of the same name.
      */
    virtual PBoolean OnOpenMediaStream(
      OpalMediaStream & stream    ///<  New media stream being opened
    );

    /**Call back for closed a media stream.

       The default behaviour calls the OpalEndPoint function of the same name.
      */
    virtual void OnClosedMediaStream(
      const OpalMediaStream & stream     ///<  Media stream being closed
    );
    
    /**Call back when patching a media stream.
       This function is called when a connection has created a new media
       patch between two streams.
      */
    virtual void OnPatchMediaStream(
      PBoolean isSource,
      OpalMediaPatch & patch    ///<  New patch
    );
	
    /**Attaches the RFC 2833 handler to the media patch
       This method may be called from subclasses, e.g. within
       OnPatchMediaStream()
      */
    virtual void AttachRFC2833HandlerToPatch(PBoolean isSource, OpalMediaPatch & patch);

    /**See if the media can bypass the local host.

       The default behaviour returns PFalse indicating that media bypass is not
       possible.
     */
    virtual PBoolean IsMediaBypassPossible(
      unsigned sessionID                  ///<  Session ID for media channel
    ) const;

    /**Meda information structure for GetMediaInformation() function.
      */
    struct MediaInformation {
      MediaInformation() { 
        rfc2833  = RTP_DataFrame::IllegalPayloadType; 
#if OPAL_T38FAX
        ciscoNSE = RTP_DataFrame::IllegalPayloadType; 
#endif
      }

      OpalTransportAddress data;           ///<  Data channel address
      OpalTransportAddress control;        ///<  Control channel address
      RTP_DataFrame::PayloadTypes rfc2833; ///<  Payload type for RFC2833
#if OPAL_T38FAX
      RTP_DataFrame::PayloadTypes ciscoNSE; ///<  Payload type for RFC2833
#endif
    };

    /**Get information on the media channel for the connection.
       The default behaviour checked the mediaTransportAddresses dictionary
       for the session ID and returns information based on that. It also uses
       the rfc2833Handler variable for that part of the info.

       It is up to the descendant class to assure that the mediaTransportAddresses
       dictionary is set correctly before OnIncomingCall() is executed.
     */
    virtual PBoolean GetMediaInformation(
      unsigned sessionID,     ///<  Session ID for media channel
      MediaInformation & info ///<  Information on media channel
    ) const;

#if OPAL_VIDEO

    /**Add video media formats available on a connection.

       The default behaviour calls the OpalEndPoint function of the same name.
      */
    virtual void AddVideoMediaFormats(
      OpalMediaFormatList & mediaFormats  ///<  Media formats to use
    ) const;

    /**Create an PVideoInputDevice for a source media stream.
      */
    virtual PBoolean CreateVideoInputDevice(
      const OpalMediaFormat & mediaFormat,  ///<  Media format for stream
      PVideoInputDevice * & device,         ///<  Created device
      PBoolean & autoDelete                     ///<  Flag for auto delete device
    );

    /**Create an PVideoOutputDevice for a sink media stream or the preview
       display for a source media stream.
      */
    virtual PBoolean CreateVideoOutputDevice(
      const OpalMediaFormat & mediaFormat,  ///<  Media format for stream
      PBoolean preview,                         ///<  Flag indicating is a preview output
      PVideoOutputDevice * & device,        ///<  Created device
      PBoolean & autoDelete                     ///<  Flag for auto delete device
    );
#endif 

    /**Set the volume (gain) for the audio media channel to the specified percentage.
      */
    virtual PBoolean SetAudioVolume(
      PBoolean source,                  ///< true for source (microphone), false for sink (speaker)
      unsigned percentage           ///< Gain, 0=silent, 100=maximun
    );

    /**Get the average signal level (0..32767) for the audio media channel.
       A return value of UINT_MAX indicates no valid signal, eg no audio channel opened.
      */
    virtual unsigned GetAudioSignalLevel(
      PBoolean source                   ///< true for source (microphone), false for sink (speaker)
    );
  //@}

  /**@name RTP Session Management */
  //@{
    /**Get an RTP session for the specified ID.
       If there is no session of the specified ID, NULL is returned.
      */
    virtual RTP_Session * GetSession(
      unsigned sessionID    ///<  RTP session number
    ) const;

    /**Use an RTP session for the specified ID.
       This will find a session of the specified ID and increment its
       reference count. Multiple OpalRTPStreams use this to indicate their
       usage of the RTP session.

       If this function is used, then the ReleaseSession() function MUST be
       called or the session is never deleted for the lifetime of the Opal
       connection.

       If there is no session of the specified ID one is created.

       The type of RTP session that is created will be compatible with the
       transport. At this time only IP (RTp over UDP) is supported.
      */
    virtual RTP_Session * UseSession(
      unsigned sessionID
    );
    virtual RTP_Session * UseSession(
      const OpalTransport & transport,  ///<  Transport of signalling
      unsigned sessionID,               ///<  RTP session number
      RTP_QOS * rtpqos = NULL           ///<  Quiality of Service information
    );

    /**Release the session.
       If the session ID is not being used any more any clients via the
       UseSession() function, then the session is deleted.
     */
    virtual void ReleaseSession(
      unsigned sessionID,    ///<  RTP session number
      PBoolean clearAll = PFalse  ///<  Clear all sessions
    );

    /**Create and open a new RTP session.
       The type of RTP session that is created will be compatible with the
       transport. At this time only IP (RTp over UDP) is supported.
      */
    virtual RTP_Session * CreateSession(
      const OpalTransport & transport,
      unsigned sessionID,
      RTP_QOS * rtpqos
    );

    /**Get the maximum RTP payload size. This function allows a user to
       override the value returned on a connection by connection basis, for
       example knowing the connection is on a LAN with ethernet MTU the
       payload size could be increased.

       Defaults to the value returned by the OpalManager function of the same
       name.
      */
    virtual PINDEX GetMaxRtpPayloadSize() const;
  //@}

  /**@name Bandwidth Management */
  //@{
    /**Get the available bandwidth in 100's of bits/sec.
      */
    unsigned GetBandwidthAvailable() const { return bandwidthAvailable; }

    /**Set the available bandwidth in 100's of bits/sec.
       Note if the force parameter is PTrue this function will close down
       active media streams to meet the new bandwidth requirement.
      */
    virtual PBoolean SetBandwidthAvailable(
      unsigned newBandwidth,    ///<  New bandwidth limit
      PBoolean force = PFalse        ///<  Force bandwidth limit
    );

    /**Get the bandwidth currently used.
       This totals the bandwidth used by open streams and returns the total
       bandwidth used in 100's of bits/sec
      */
    virtual unsigned GetBandwidthUsed() const;

    /**Set the used bandwidth in 100's of bits/sec.
       This is an internal function used by the OpalMediaStream bandwidth
       management code.

       If there is insufficient bandwidth available, PFalse is returned. If
       sufficient bandwidth is available, then PTrue is returned and the amount
       of available bandwidth is reduced by the specified amount.
      */
    virtual PBoolean SetBandwidthUsed(
      unsigned releasedBandwidth,   ///<  Bandwidth to release
      unsigned requiredBandwidth    ///<  Bandwidth required
    );
  //@}

  /**@name User input */
  //@{
    enum SendUserInputModes {
      SendUserInputAsQ931,
      SendUserInputAsString,
      SendUserInputAsTone,
      SendUserInputAsInlineRFC2833,
      SendUserInputAsSeparateRFC2833,  // Not implemented
      SendUserInputAsProtocolDefault,
      NumSendUserInputModes
    };
#if PTRACING
    friend ostream & operator<<(ostream & o, SendUserInputModes m);
#endif

    /**Set the user input indication transmission mode.
      */
    virtual void SetSendUserInputMode(SendUserInputModes mode);

    /**Get the user input indication transmission mode.
      */
    virtual SendUserInputModes GetSendUserInputMode() const { return sendUserInputMode; }

    /**Get the real user input indication transmission mode.
       This will return the user input mode that will actually be used for
       transmissions. It will be the value of GetSendUserInputMode() provided
       the remote endpoint is capable of that mode.
      */
    virtual SendUserInputModes GetRealSendUserInputMode() const { return GetSendUserInputMode(); }

    /**Send a user input indication to the remote endpoint.
       This is for sending arbitrary strings as user indications.

       The default behaviour is to call SendUserInputTone() for each character
       in the string.
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

       The default behaviour sends the tone using RFC2833.
      */
    virtual PBoolean SendUserInputTone(
      char tone,        ///<  DTMF tone code
      unsigned duration = 0  ///<  Duration of tone in milliseconds
    );

    /**Call back for remote enpoint has sent user input as a string.
       This will be called irrespective of the source (H.245 string, H.245
       signal or RFC2833).

       The default behaviour calls the endpoint function of the same name.
      */
    virtual void OnUserInputString(
      const PString & value   ///<  String value of indication
    );

    /**Call back for remote enpoint has sent user input.
       If duration is zero then this indicates the beginning of the tone. If
       duration is non-zero then it indicates the end of the tone output.

       The default behaviour calls the OpalEndPoint function of the same name.
      */
    virtual void OnUserInputTone(
      char tone,
      unsigned duration
    );

    /**Send a user input indication to the remote endpoint.
       This sends a Hook Flash emulation user input.
      */
    void SendUserInputHookFlash(
      unsigned duration = 500  ///<  Duration of tone in milliseconds
    ) { SendUserInputTone('!', duration); }

    /**Get a user input indication string, waiting until one arrives.
      */
    virtual PString GetUserInput(
      unsigned timeout = 30   ///<  Timeout in seconds on input
    );

    /**Set a user indication string.
       This allows the GetUserInput() function to unblock and return this
       string.
      */
    virtual void SetUserInput(
      const PString & input     ///<  Input string
    );

    /**Read a sequence of user indications with timeouts.
      */
    virtual PString ReadUserInput(
      const char * terminators = "#\r\n", ///<  Characters that can terminte input
      unsigned lastDigitTimeout = 4,      ///<  Timeout on last digit in string
      unsigned firstDigitTimeout = 30     ///<  Timeout on receiving any digits
    );

    /**Play a prompt to the connection before rading user indication string.

       For example the LID connection would play a dial tone.

       The default behaviour does nothing.
      */
    virtual PBoolean PromptUserInput(
      PBoolean play   ///<  Flag to start or stop playing the prompt
    );
  //@}

  /**@name Other services */
  //@{
#if OPAL_T120DATA
    /**Create an instance of the T.120 protocol handler.
       This is called when the OpenLogicalChannel subsystem requires that
       a T.120 channel be established.

       Note that if the application overrides this and returns a pointer to a
       heap variable (using new) then it is the responsibility of the creator
       to subsequently delete the object. The user of this function (the 
       H323_T120Channel class) will not do so.

       The default behavour returns H323Endpoint::CreateT120ProtocolHandler()
       while keeping track of that variable for autmatic deletion.
      */
    virtual OpalT120Protocol * CreateT120ProtocolHandler();
#endif

#if OPAL_T38FAX
    /**Create an instance of the T.38 protocol handler.
       This is called when the OpenLogicalChannel subsystem requires that
       a T.38 fax channel be established.

       Note that if the application overrides this and returns a pointer to a
       heap variable (using new) then it is the responsibility of the creator
       to subsequently delete the object. The user of this function (the 
       H323_T38Channel class) will not do so.

       The default behavour returns H323Endpoint::CreateT38ProtocolHandler()
       while keeping track of that variable for autmatic deletion.
      */
    virtual OpalT38Protocol * CreateT38ProtocolHandler();
#endif

#if OPAL_H224
	
	/** Create an instance of the H.224 protocol handler.
	    This is called when the subsystem requires that a H.224 channel be established.
		
	    Note that if the application overrides this it should return a pointer
	    to a heap variable (using new) as it will be automatically deleted when
	    the OpalConnection is deleted.
	
	    The default behaviour calls the OpalEndpoint function of the same name if
        there is not already a H.224 handler associated with this connection. If there
        is already such a H.224 handler associated, this instance is returned instead.
	  */
	virtual OpalH224Handler *CreateH224ProtocolHandler(unsigned sessionID);
	
	/** Create an instance of the H.281 protocol handler.
		This is called when the subsystem requires that a H.224 channel be established.
		
		Note that if the application overrides this it should return a pointer
		to a heap variable (using new) as it will be automatically deleted when
		the associated H.224 handler is deleted.
		
		The default behaviour calls the OpalEndpoint function of the same name.
	*/
	virtual OpalH281Handler *CreateH281ProtocolHandler(OpalH224Handler & h224Handler);
	
    /** Returns the H.224 handler associated with this connection or NULL if no
		handler was created
	  */
	OpalH224Handler * GetH224Handler() const { return  h224Handler; }
#endif

    /** Execute garbage collection for endpoint.
        Returns PTrue if all garbage has been collected.
        Default behaviour deletes the objects in the connectionsActive list.
      */
    virtual bool GarbageCollection();
  //@}

  /**@name Member variable access */
  //@{
    /**Get the owner endpoint for this connection.
     */
    OpalEndPoint & GetEndPoint() const { return endpoint; }
    
    /**Get the owner call for this connection.
     */
    OpalCall & GetCall() const { return ownerCall; }

    /**Get the token for this connection.
     */
    const PString & GetToken() const { return callToken; }

    /**Get the call direction for this connection.
     */
    PBoolean IsOriginating() const { return originating; }

    /**Get the time at which the connection was begun
      */
    PTime GetSetupUpTime() const { return setupTime; }

    /**Get the time at which the ALERTING was received
      */
    PTime GetAlertingTime() const { return alertingTime; }

    /**Get the time at which the connection was established
      */
    PTime GetConnectionStartTime() const { return connectedTime; }

    /**Get the time at which the connection was cleared
      */
    PTime GetConnectionEndTime() const { return callEndTime; }

    /**Get the product info for all endpoints.
      */
    const OpalProductInfo & GetProductInfo() const { return productInfo; }

    /**Set the product info for all endpoints.
      */
    void SetProductInfo(
      const OpalProductInfo & info
    ) { productInfo = info; }

    /**Get the local name/alias.
      */
    const PString & GetLocalPartyName() const { return localPartyName; }

    /**Set the local name/alias.
      */
    virtual void SetLocalPartyName(const PString & name);

    /**Get the local name/alias.
      */
    virtual PString GetLocalPartyURL() const;

    /**Get the local display name.
      */
    const PString & GetDisplayName() const { return displayName; }

    /**Set the local display name.
      */
    void SetDisplayName(const PString & name) { displayName = name; }

    /**Get the caller name/alias.
      */
    const PString & GetRemotePartyName() const { return remotePartyName; }

    /**Get the remote application description. This is for backward
       compatibility and has been supercedded by GeREmoteProductInfo();
      */
    PCaselessString GetRemoteApplication() const { return remoteProductInfo.AsString(); }

    /** Get the remote product info.
      */
    const OpalProductInfo & GetRemoteProductInfo() const { return remoteProductInfo; }
    
    /**Get the remote party number, if there was one one.
       If the remote party has indicated an e164 number as one of its aliases
       or as a field in the Q.931 PDU, then this function will return it.
      */
    const PString & GetRemotePartyNumber() const { return remotePartyNumber; }

    /**Get the remote party address.
      */
    const PString & GetRemotePartyAddress() const { return remotePartyAddress; }

    /**Get the remote party address.
       This will return the "best guess" at an address to use in a
       to call the user again later.
      */
    virtual const PString GetRemotePartyCallbackURL() const { return remotePartyAddress; }


    /**Get the called number (for incoming calls). This is useful for gateway
       applications where the destination number may not be the same as the local number
      */
    virtual const PString & GetCalledDestinationNumber() const { return calledDestinationNumber; }

    /**Get the called name (for incoming calls). This is useful for gateway
       applications where the destination name may not be the same as the local username
      */
    virtual const PString & GetCalledDestinationName() const { return calledDestinationName; }

    /**Get the called URL (for incoming calls). This is useful for gateway
       applications where the destination number may not be the same as the local number
      */
    virtual const PString & GetCalledDestinationURL() const { return calledDestinationURL; }

    /**Get the default maximum audio jitter delay parameter.
       Defaults to 50ms
     */
    unsigned GetMinAudioJitterDelay() const { return minAudioJitterDelay; }

    /**Get the default maximum audio delay jitter parameter.
       Defaults to 250ms.
     */
    unsigned GetMaxAudioJitterDelay() const { return maxAudioJitterDelay; }

    /**Set the maximum audio delay jitter parameter.
     */
    void SetAudioJitterDelay(
      unsigned minDelay,   ///<  New minimum jitter buffer delay in milliseconds
      unsigned maxDelay    ///<  New maximum jitter buffer delay in milliseconds
    );

    /**Get the silence detector active on connection.
     */
    OpalSilenceDetector * GetSilenceDetector() const { return silenceDetector; }
    
    /**Get the echo canceler active on connection.
    */
    OpalEchoCanceler * GetEchoCanceler() const { return echoCanceler; }

    /**Get the protocol-specific unique identifier for this connection.
     */
    virtual const OpalGloballyUniqueID & GetIdentifier() const
    { return callIdentifier; }

    virtual OpalTransport & GetTransport() const
    { return *(OpalTransport *)NULL; }

    PDICTIONARY(MediaAddressesDict, POrdinalKey, OpalTransportAddress);
    MediaAddressesDict & GetMediaTransportAddresses()
    { return mediaTransportAddresses; }

  //@}

    const RTP_DataFrame::PayloadMapType & GetRTPPayloadMap() const
    { return rtpPayloadMap; }

    /** Return PTrue if the remote appears to be behind a NAT firewall
    */
    PBoolean RemoteIsNAT() const
    { return remoteIsNAT; }

    /**Determine if the RTP session needs to accommodate a NAT router.
       For endpoints that do not use STUN or something similar to set up all the
       correct protocol embeddded addresses correctly when a NAT router is between
       the endpoints, it is possible to still accommodate the call, with some
       restrictions. This function determines if the RTP can proceed with special
       NAT allowances.

       The special allowance is that the RTP code will ignore whatever the remote
       indicates in the protocol for the address to send RTP data and wait for
       the first packet to arrive from the remote and will then proceed to send
       all RTP data back to that address AND port.

       The default behaviour checks the values of the physical link
       (localAddr/peerAddr) against the signaling address the remote indicated in
       the protocol, eg H.323 SETUP sourceCallSignalAddress or SIP "To" or
       "Contact" fields, and makes a guess that the remote is behind a NAT router.
     */
    virtual PBoolean IsRTPNATEnabled(
      const PIPSocket::Address & localAddr,   ///< Local physical address of connection
      const PIPSocket::Address & peerAddr,    ///< Remote physical address of connection
      const PIPSocket::Address & signalAddr,  ///< Remotes signaling address as indicated by protocol of connection
      PBoolean incoming                       ///< Incoming/outgoing connection
    );

    virtual void SetSecurityMode(const PString & v);

    virtual PString GetSecurityMode() const;

    virtual OpalSecurityMode * GetSecurityData();         

    StringOptions * GetStringOptions() const;

    void SetStringOptions(StringOptions * options);

    /**Open the media channels associated with an incoming call
 
       This function is provided to allow an OpalConnection descendant to delay the opening of the
       media channels associated with an incoming call. By default, this function is called as soon
       as possible after an incoming connection request has been received (i.e. an INVITE or SETUP) and
       and do all of the work associated with handling SDP or fastStart eyc

       By overriding this function, the media channel open is deferred, which gives the other connections
       in the call a chance to do something before the media channels are started. This allows for features
       beyond those provided by the OnAnswer interface
      */
    virtual PBoolean OnOpenIncomingMediaChannels();

    virtual void ApplyStringOptions();

    virtual void PreviewPeerMediaFormats(const OpalMediaFormatList & fmts);

    virtual void EnableRecording();
    virtual void DisableRecording();

    virtual void OnMediaPatchStart(
      unsigned sessionId, 
          bool isSource
    );
    virtual void OnMediaPatchStop(
      unsigned sessionId, 
          bool isSource
    );

  protected:
    PDECLARE_NOTIFIER(OpalRFC2833Info, OpalConnection, OnUserInputInlineRFC2833);
    PDECLARE_NOTIFIER(OpalRFC2833Info, OpalConnection, OnUserInputInlineCiscoNSE);
#if P_DTMF
    PDECLARE_NOTIFIER(RTP_DataFrame, OpalConnection, OnUserInputInBandDTMF);
#endif
    PDECLARE_NOTIFIER(PThread, OpalConnection, OnReleaseThreadMain);
    PDECLARE_NOTIFIER(RTP_DataFrame, OpalConnection, OnRecordAudio);

  // Member variables
    OpalCall             & ownerCall;
    OpalEndPoint         & endpoint;

    PMutex               phaseMutex;
    Phases               phase;

    PString              callToken;
    OpalGloballyUniqueID callIdentifier;
    PBoolean             originating;
    PTime                setupTime;
    PTime                alertingTime;
    PTime                connectedTime;
    PTime                callEndTime;
    OpalProductInfo      productInfo;
    PString              localPartyName;
    PString              displayName;
    PString              remotePartyName;
    OpalProductInfo      remoteProductInfo;
    PString              remotePartyNumber;
    PString              remotePartyAddress;
    CallEndReason        callEndReason;
    PString              calledDestinationNumber;
    PString              calledDestinationName;
    PString              calledDestinationURL;
    PBoolean             remoteIsNAT;

    SendUserInputModes    sendUserInputMode;
    PString               userInputString;
    PSyncPoint            userInputAvailable;
    PBoolean              detectInBandDTMF;
    unsigned              q931Cause;

    OpalSilenceDetector * silenceDetector;
    OpalEchoCanceler    * echoCanceler;
    OpalRFC2833Proto    * rfc2833Handler;
#if OPAL_T120DATA
    OpalT120Protocol    * t120handler;
#endif
#if OPAL_T38FAX
    OpalT38Protocol     * t38handler;
    OpalRFC2833Proto    * ciscoNSEHandler;
#endif
#if OPAL_H224
    OpalH224Handler		  * h224Handler;
#endif

    OpalMediaFormatList        localMediaFormats;
    MediaAddressesDict         mediaTransportAddresses;
    PSafeList<OpalMediaStream> mediaStreams;
    RTP_SessionManager         rtpSessions;

    unsigned            minAudioJitterDelay;
    unsigned            maxAudioJitterDelay;
    unsigned            bandwidthAvailable;

    RTP_DataFrame::PayloadMapType rtpPayloadMap;

    // The In-Band DTMF detector. This is used inside an audio filter which is
    // added to the audio channel.
#if P_DTMF
    PDTMFDecoder        dtmfDecoder;
#endif

    PString securityModeName;
    OpalSecurityMode * securityModeData;

    /**Set the phase of the connection.
       @param phaseToSet the phase to set
      */
    void SetPhase(Phases phaseToSet);

#if PTRACING
    friend ostream & operator<<(ostream & o, Phases p);
#endif

    PBoolean useRTPAggregation;

    StringOptions * stringOptions;
    PString recordAudioFilename;
};

class RTP_UDP;

class OpalSecurityMode : public PObject
{
  PCLASSINFO(OpalSecurityMode, PObject);
  public:
    virtual ~OpalSecurityMode();

    virtual RTP_UDP * CreateRTPSession(
#if OPAL_RTP_AGGREGATE
      PHandleAggregator * _aggregator,   ///< handle aggregator
#endif
      unsigned id,                       ///< Session ID for RTP channel
      PBoolean remoteIsNAT,              ///< PTrue is remote is behind NAT
      OpalConnection & connection	 ///< Connection creating session (may be needed by secure connections)
    ) = 0;
    virtual PBoolean Open() = 0;
};

/*
 * this class exists simply to test the security mode functions
 */
class OpalTestSecurityMode : public OpalSecurityMode
{
  PCLASSINFO(OpalTestSecurityMode, OpalSecurityMode);
  public:
    OpalTestSecurityMode();
    ~OpalTestSecurityMode();

    virtual RTP_UDP * CreateRTPSession(
#if OPAL_RTP_AGGREGATE
      PHandleAggregator * _aggregator,   ///< handle aggregator
#endif
      unsigned id,                       ///< Session ID for RTP channel
      PBoolean remoteIsNAT,              ///< PTrue is remote is behind NAT
      OpalConnection & connection	 ///< Connection creating session (may be needed by secure connections)
    );


    virtual PBoolean Open();

  protected:
    void * opaqueSecurityData;
};


#endif // __OPAL_CONNECTION_H


// End of File ///////////////////////////////////////////////////////////////
