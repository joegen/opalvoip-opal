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
 * Contributor(s): ______________________________________.
 *
 * $Log: connection.h,v $
 * Revision 1.2006  2001/10/03 05:56:15  robertj
 * Changes abndwidth management API.
 *
 * Revision 2.4  2001/08/22 10:20:09  robertj
 * Changed connection locking to use double mutex to guarantee that
 *   no threads can ever deadlock or access deleted connection.
 *
 * Revision 2.3  2001/08/17 08:22:23  robertj
 * Moved call end reasons enum from OpalConnection to global.
 *
 * Revision 2.2  2001/08/13 05:10:39  robertj
 * Updates from OpenH323 v1.6.0 release.
 *
 * Revision 2.1  2001/08/01 05:26:35  robertj
 * Moved media formats list from endpoint to connection.
 *
 * Revision 2.0  2001/07/27 15:48:24  robertj
 * Conversion of OpenH323 to Open Phone Abstraction Library (OPAL)
 *
 */

#ifndef __OPAL_CONNECTION_H
#define __OPAL_CONNECTION_H

#ifdef __GNUC__
#pragma interface
#endif


#include <opal/mediafmt.h>
#include <opal/mediastrm.h>


class OpalEndPoint;
class OpalCall;


/**Call/Connection ending reasons.
   NOTE: if anything is added to this, you also need to add the field to
   the tables in call.cxx and h323pdu.cxx.
  */
enum OpalCallEndReason {
  EndedByLocalUser,         /// Local endpoint application cleared call
  EndedByNoAccept,          /// Local endpoint did not accept call OnIncomingCall()=FALSE
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
  OpalNumCallEndReasons
};

#if PTRACING
ostream & operator<<(ostream & o, OpalCallEndReason reason);
#endif


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
class OpalConnection : public PObject
{
    PCLASSINFO(OpalConnection, PObject);
  public:
  /**@name Construction */
  //@{
    /**Create a new connection.
     */
    OpalConnection(
      OpalCall & call,          /// Owner calll for connection
      OpalEndPoint & endpoint,  /// Owner endpoint for connection
      const PString & token     /// Token to identify the connection
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
      ostream & strm    /// Stream to output text representation
    ) const;
  //@}

  /**@name Basic operations */
  //@{
    /**Lock connection.
       When the OpalManager::FindCallWithLock() function (or other Find
       functions that are protocol specific) is used to gain access to a
       call or connection object, this is called to prevent it from being
       closed and deleted by the background threads.
     */
    virtual BOOL Lock();

    /**Unlock connection.
     */
    void Unlock();

    enum Phases {
      SetUpPhase,
      AlertingPhase,
      ConnectedPhase,
      EstablishedPhase,
      ReleasedPhase,
      NumPhases
    };

    /**Get the phase of the connection.
       This indicates the current phase of the connection sequence. Whether
       all phases and the transitions between phases is protocol dependent.

       The default bahaviour is pure.
      */
    virtual Phases GetPhase() const = 0;

    /**Get the call clearand reason for this connection shutting down.
       Note that this function is only generally useful in the
       H323EndPoint::OnConnectionCleared() function. This is due to the
       connection not being cleared before that, and the object not even
       exiting after that.

       If the call is still active then this will return NumOpalCallEndReasons.
      */
    OpalCallEndReason GetCallEndReason() const { return callEndReason; }

    /**Set the call clearance reason.
       An application should have no cause to use this function. It is present
       for the H323EndPoint::ClearCall() function to set the clearance reason.
      */
    void SetCallEndReason(
      OpalCallEndReason reason   /// Reason for clearance of connection.
    );

    /**Clear a current call.
       This hangs up the current call. This will release all connections
       currently in the call by calling the OpalCall::Clear() function.

       Note that this function will return quickly as the release and
       disposal of the connections is done by another thread.
      */
    void ClearCall(
      OpalCallEndReason reason = EndedByLocalUser /// Reason for call clearing
    );

    /**Clear a current connection, synchronously
      */
    virtual void ClearCallSynchronous(
      PSyncPoint * sync,
      OpalCallEndReason reason = EndedByLocalUser  /// Reason for call clearing
    );
  //@}

  /**@name Call progress functions */
  //@{
    /**Call back for an incoming call.
       This function is used for an application to control the answering of
       incoming calls.

       If TRUE is returned then the connection continues. If FALSE then the
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
    virtual BOOL OnIncomingConnection();

    enum AnswerCallResponse {
      AnswerCallNow,      /// Answer the call continuing with the connection.
      AnswerCallDenied,   /// Refuse the call sending a release complete.
      AnswerCallAlert,  /// Send an Alerting PDU and wait for AnsweringCall()
      AnswerCallPending = AnswerCallAlert,
      AnswerCallDeferred, /// As for AnswerCallPending but does not send Alerting PDU
      AnswerCallAlertWithMedia, /// As for AnswerCallPending but starts media channels
      AnswerCallDeferredWithMedia, /// As for AnswerCallPending but starts media channels
      NumAnswerCallResponses
    };

    /**Indicate the result of answering an incoming call.
       This should only be called if the OnIncomingConnection() callback
       function has returned TRUE. It is an anychronous indication for
       other threads that are waiting for a response to continue.

       Note sending further AnswerCallPending responses via this function will
       have the result of an alerting message being sent to the remote
       endpoint. In this way multiple alerting mesages may be sent.

       Sending a AnswerCallDeferred response would have no effect.
      */
    virtual void SetAnswerResponse(
      AnswerCallResponse response /// Answer response to incoming call
    );

    /**Call back for remote party being alerted.
       This function is called after the connection is informed that the
       remote endpoint is "ringing". Generally some time after the
       SetUpConnection() function was called, this is function is called.

       If FALSE is returned the connection is aborted.

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
    virtual BOOL SetAlerting(
      const PString & calleeName    /// Name of endpoint being alerted.
    ) = 0;

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
    virtual BOOL SetConnected() = 0;

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
    void Release(
      OpalCallEndReason reason = EndedByLocalUser /// Reason for call release
    );

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

       The default behaviour calls the OpalEndPoint function of the same name.
      */
    virtual BOOL OnReleased();
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

       The return value is TRUE if the call is to be forwarded, FALSE
       otherwise. Note that if the call is forwarded the current connection is
       cleared with teh ended call code of EndedByCallForwarded.
      */
    virtual BOOL ForwardCall(
      const PString & forwardParty   /// Party to forward call to.
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

    /**Open source transmitter media stream for session.
      */
    virtual BOOL OpenSourceMediaStream(
      const OpalMediaFormatList & mediaFormats, /// Optional media format to open
      unsigned sessionID                   /// Session to start stream on
    );

    /**Open source transmitter media stream for session.
      */
    virtual OpalMediaStream * OpenSinkMediaStream(
      OpalMediaStream & source    /// Source media sink format to open to
    );

    /**Create a new media stream.
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
    ) = 0;

    /**Call back when opening a media stream.
       This function is called when a connection has created a new media
       stream according to the logic of its underlying protocol.

       The usual requirement is that media streams are created on all other
       connections participating in the call and all of the media streams are
       attached to an instance of an OpalMediaPatch object that will read from
       one of the media streams passing data to the other media streams.

       The default behaviour calls the OpalEndPoint function of the same name.
      */
    virtual BOOL OnOpenMediaStream(
      OpalMediaStream & stream    /// New media stream being opened
    );

    /**Call back for closed a media stream.

       The default behaviour calls the OpalEndPoint function of the same name.
      */
    virtual void OnClosedMediaStream(
      const OpalMediaStream & stream     /// Media stream being closed
    );

    /**Get a media stream.
       Locates a stream given a RTP session ID. Each session would usually
       have two media streams associated with it, so the source flag
       may be used to distinguish which channel to return.
      */
    OpalMediaStream * GetMediaStream(
      unsigned sessionId,  /// Session ID to search for.
      BOOL source          /// Indicates the direction of stream.
    ) const;
  //@}

  /**@name RTP Session Management */
  //@{
    /**Get an RTP session for the specified ID.
       If there is no session of the specified ID, NULL is returned.
      */
    virtual RTP_Session * GetSession(
      unsigned sessionID    /// RTP session number
    ) const;

    /**Use an RTP session for the specified ID.
       This will find a session of the specified ID and increment its
       reference count. Multiple OpalRTPStreams use this to indicate their
       usage of the RTP session.

       If this function is used, then the ReleaseSession() function MUST be
       called or the session is never deleted for the lifetime of the Opal
       connection.

       If there is no session of the specified ID, NULL is returned.
      */
    virtual RTP_Session * UseSession(
      unsigned sessionID    /// RTP session number
    );

    /**Release the session.
       If the session ID is not being used any more any clients via the
       UseSession() function, then the session is deleted.
     */
    virtual void ReleaseSession(
      unsigned sessionID    /// RTP session number
    );
  //@}

  /**@name Bandwidth Management */
  //@{
    /**Get the available bandwidth in 100's of bits/sec.
      */
    unsigned GetBandwidthAvailable() const { return bandwidthAvailable; }

    /**Set the available bandwidth in 100's of bits/sec.
       Note if the force parameter is TRUE this function will close down
       active media streams to meet the new bandwidth requirement.
      */
    virtual BOOL SetBandwidthAvailable(
      unsigned newBandwidth,    /// New bandwidth limit
      BOOL force = FALSE        /// Force bandwidth limit
    );

    /**Get the bandwidth currently used.
       This totals the bandwidth used by open streams and returns the total
       bandwidth used in 100's of bits/sec
      */
    virtual unsigned GetBandwidthUsed() const;

    /**Set the used bandwidth in 100's of bits/sec.
       This is an internal function used by the OpalMediaStream bandwidth
       management code.

       If there is insufficient bandwidth available, FALSE is returned. If
       sufficient bandwidth is available, then TRUE is returned and the amount
       of available bandwidth is reduced by the specified amount.
      */
    virtual BOOL SetBandwidthUsed(
      unsigned releasedBandwidth,   /// Bandwidth to release
      unsigned requiredBandwidth    /// Bandwidth required
    );
  //@}

  /**@name User indications */
  //@{
    /**Send a user input indication to the remote endpoint.
       This sends an arbitrary string as a user indication. If DTMF tones in
       particular are required to be sent then the SendIndicationTone()
       function should be used.

       The default behaviour calls SendUserIndicationTone() for each character
       in the string.
      */
    virtual BOOL SendUserIndicationString(
      const PString & value                   /// String value of indication
    );

    /**Send a user input indication to the remote endpoint.
       This sends DTMF emulation user input. If something other than the
       standard tones need be sent use the SendUserIndicationString() function.

       The default behaviour does nothing.
      */
    virtual BOOL SendUserIndicationTone(
      char tone,    /// DTMF tone code
      int duration  /// Duration of tone in milliseconds
    );

    /**Call back for remote enpoint has sent user input as a string.

       The default behaviour calls the OpalEndPoint function of the same name.
      */
    virtual void OnUserIndicationString(
      const PString & value   /// String value of indication
    );

    /**Call back for remote enpoint has sent user input.

       The default behaviour calls the OpalEndPoint function of the same name.
      */
    virtual void OnUserIndicationTone(
      char tone,
      int duration
    );

    /**Get a user indication string, waiting until one arrives.
      */
    virtual PString GetUserIndication(
      unsigned timeout = 30   /// Timeout in seconds on input
    );

    /**Set a user indication string.
       This allows the GetUserIndication() function to unblock and return this
       string.
      */
    virtual void SetUserIndication(
      const PString & input     /// Input string
    );

    /**Read a sequence of user indications with timeouts.
      */
    virtual PString ReadUserIndication(
      const char * terminators = "#\r\n", /// Characters that can terminte input
      unsigned lastDigitTimeout = 4,      /// Timeout on last digit in string
      unsigned firstDigitTimeout = 30     /// Timeout on receiving any digits
    );

    /**Play a prompt to the connection before rading user indication string.

       For example the LID connection would play a dial tone.

       The default behaviour does nothing.
      */
    virtual BOOL PromptUserIndication(
      BOOL play   /// Flag to start or stop playing the prompt
    );
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
    BOOL HadAnsweredCall() const { return callAnswered; }

    /**Get the time at which the connection was established
      */
    PTime GetConnectionStartTime() const { return connectionStartTime; }

    /**Get the local name/alias.
      */
    const PString & GetLocalPartyName() const { return localPartyName; }

    /**Set the local name/alias from information in the PDU.
      */
    void SetLocalPartyName(const PString & name) { localPartyName = name; }

    /**Get the caller name/alias.
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
  //@}

  protected:
    /**Lock the connection at start of OnReleased() function.
      */
    void LockOnRelease();

  // Member variables
    OpalCall          & ownerCall;
    OpalEndPoint      & endpoint;

    PString             callToken;
    BOOL                callAnswered;
    PTime               connectionStartTime;
    PString             localPartyName;
    PString             remotePartyName;
    PString             remotePartyNumber;
    PString             remotePartyAddress;
    OpalCallEndReason   callEndReason;

    AnswerCallResponse  answerResponse;
    PSyncPoint          answerWaitFlag;

    PString             userIndicationString;
    PMutex              userIndicationMutex;
    PSyncPoint          userIndicationAvailable;

    OpalMediaStreamList mediaStreams;
    RTP_SessionManager  rtpSessions;
    unsigned            bandwidthAvailable;

  private:
    PMutex innerMutex;
    PMutex outerMutex;
    BOOL   isBeingReleased;


#if PTRACING
    friend ostream & operator<<(ostream & o, Phases p);
    friend ostream & operator<<(ostream & o, AnswerCallResponse s);
#endif
};


PLIST(OpalConnectionList, OpalConnection);
PDICTIONARY(OpalConnectionDict, PString, OpalConnection);


#endif // __OPAL_CONNECTION_H


// End of File ///////////////////////////////////////////////////////////////
