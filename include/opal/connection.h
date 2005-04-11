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
 * Revision 1.2038  2005/04/11 11:12:00  dsandras
 * Fixed previous commit.
 *
 * Revision 2.36  2005/04/11 10:42:35  dsandras
 * Fixed previous commit.
 *
 * Revision 2.35  2005/04/10 20:43:39  dsandras
 * Added support for function allowing to put the OpalMediaStreams on pause.
 *
 * Revision 2.34  2005/04/10 20:42:33  dsandras
 * Added support for a function that returns the "best guess" callback URL.
 *
 * Revision 2.33  2005/04/10 20:41:29  dsandras
 * Added support for call hold.
 *
 * Revision 2.32  2005/04/10 20:40:20  dsandras
 * Added support for Blind Transfert.
 *
 * Revision 2.31  2005/01/16 11:28:05  csoutheren
 * Added GetIdentifier virtual function to OpalConnection, and changed H323
 * and SIP descendants to use this function. This allows an application to
 * obtain a GUID for any connection regardless of the protocol used
 *
 * Revision 2.30  2004/12/12 12:29:02  dsandras
 * Moved GetRemoteApplication () to OpalConnection so that it is usable for all types of connection.
 *
 * Revision 2.29  2004/08/14 07:56:29  rjongbloed
 * Major revision to utilise the PSafeCollection classes for the connections and calls.
 *
 * Revision 2.28  2004/05/17 13:24:18  rjongbloed
 * Added silence suppression.
 *
 * Revision 2.27  2004/05/01 10:00:51  rjongbloed
 * Fixed ClearCallSynchronous so now is actually signalled when call is destroyed.
 *
 * Revision 2.26  2004/04/26 04:33:05  rjongbloed
 * Move various call progress times from H.323 specific to general conenction.
 *
 * Revision 2.25  2004/04/18 13:31:28  rjongbloed
 * Added new end call value from OpenH323.
 *
 * Revision 2.24  2004/03/13 06:25:50  rjongbloed
 * Slight rearrangement of local party name and alias list to beter match common
 *   behaviour in ancestor.
 * Abstracted local party name for endpoint into ancestor from H.,323.
 *
 * Revision 2.23  2004/03/11 06:54:27  csoutheren
 * Added ability to disable SIP or H.323 stacks
 *
 * Revision 2.22  2004/02/24 11:28:45  rjongbloed
 * Normalised RTP session management across protocols
 *
 * Revision 2.21  2003/03/17 10:26:59  robertj
 * Added video support.
 *
 * Revision 2.20  2003/03/06 03:57:47  robertj
 * IVR support (work in progress) requiring large changes everywhere.
 *
 * Revision 2.19  2003/01/07 04:39:53  robertj
 * Updated to OpenH323 v1.11.2
 *
 * Revision 2.18  2002/11/10 11:33:17  robertj
 * Updated to OpenH323 v1.10.3
 *
 * Revision 2.17  2002/09/16 02:52:35  robertj
 * Added #define so can select if #pragma interface/implementation is used on
 *   platform basis (eg MacOS) rather than compiler, thanks Robert Monaghan.
 *
 * Revision 2.16  2002/09/12 06:54:06  robertj
 * Added missing virtual to Release() function so can be overridden.
 *
 * Revision 2.15  2002/07/01 04:56:30  robertj
 * Updated to OpenH323 v1.9.1
 *
 * Revision 2.14  2002/04/10 03:08:42  robertj
 * Moved code for handling media bypass address resolution into ancestor as
 *   now done ths same way in both SIP and H.323.
 *
 * Revision 2.13  2002/04/09 00:16:46  robertj
 * Changed "callAnswered" to better description of "originating".
 *
 * Revision 2.12  2002/02/19 07:42:07  robertj
 * Added OpalRFC2833 as a OpalMediaFormat variable.
 * Restructured media bypass functions to fix problems with RFC2833.
 *
 * Revision 2.11  2002/02/11 09:32:12  robertj
 * Updated to openH323 v1.8.0
 *
 * Revision 2.10  2002/02/11 07:38:35  robertj
 * Added media bypass for streams between compatible protocols.
 *
 * Revision 2.9  2002/01/22 05:04:21  robertj
 * Revamp of user input API triggered by RFC2833 support
 *
 * Revision 2.8  2001/11/14 01:31:55  robertj
 * Corrected placement of adjusting media format list.
 *
 * Revision 2.7  2001/11/02 10:45:19  robertj
 * Updated to OpenH323 v1.7.3
 *
 * Revision 2.6  2001/10/15 04:29:14  robertj
 * Added delayed start of media patch threads.
 * Removed answerCall signal and replaced with state based functions.
 *
 * Revision 2.5  2001/10/03 05:56:15  robertj
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


class OpalEndPoint;
class OpalCall;
class OpalSilenceDetector;
class OpalRFC2833Proto;
class OpalRFC2833Info;
class OpalT120Protocol;
class OpalT38Protocol;


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
       the tables in call.cxx and h323pdu.cxx.
      */
    enum CallEndReason {
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
      EndedByTemporaryFailure,  /// The remote failed temporarily app may retry
      EndedByQ931Cause,         /// The remote ended the call with unmapped Q.931 cause code
      EndedByDurationLimit,     /// Call cleared due to an enforced duration limit
      EndedByInvalidConferenceID, /// Call cleared due to invalid conference ID
      NumCallEndReasons
    };

#if PTRACING
    friend ostream & operator<<(ostream & o, CallEndReason reason);
#endif


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
    Phases GetPhase() const { return phase; }

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
      CallEndReason reason        /// Reason for clearance of connection.
    );

    /**Clear a current call.
       This hangs up the current call. This will release all connections
       currently in the call by calling the OpalCall::Clear() function.

       Note that this function will return quickly as the release and
       disposal of the connections is done by another thread.
      */
    void ClearCall(
      CallEndReason reason = EndedByLocalUser /// Reason for call clearing
    );

    /**Clear a current connection, synchronously
      */
    virtual void ClearCallSynchronous(
      PSyncPoint * sync,
      CallEndReason reason = EndedByLocalUser  /// Reason for call clearing
    );

    /**Initiate the transfer of an existing call (connection) to a new remote 
       party.
     */
    virtual void TransferConnection(
      const PString & remoteParty,   /// Remote party to transfer the existing call to
      const PString & callIdentity = PString::Empty()
                                    /// Call Identity of secondary call if present
    );
    
    /**Put the current connection on hold, suspending all media streams.
     */
    virtual void HoldConnection();

    /**Retrieve the current connection from hold, activating all media 
     * streams.
     */
    virtual void RetrieveConnection();

    /**Return TRUE if the current connection is on hold.
     */
    virtual BOOL IsConnectionOnHold();
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

    /**Start an outgoing connection.
       This function will initiate the connection to the remote entity, for
       example in H.323 it sends a SETUP, in SIP it sends an INVITE etc.

       The default behaviour is pure.
      */
    virtual BOOL SetUpConnection() = 0;

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
      const PString & calleeName,   /// Name of endpoint being alerted.
      BOOL withMedia                /// Open media with alerting
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
    virtual void Release(
      CallEndReason reason = EndedByLocalUser /// Reason for call release
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

    /**Adjust media formats available on a connection.
       This is called by a connection after it has called
       OpalCall::GetMediaFormats() to get all media formats that it can use so
       that an application may remove or reorder the media formats before they
       are used to open media streams.

       The default behaviour calls the OpalEndPoint function of the same name.
      */
    virtual void AdjustMediaFormats(
      OpalMediaFormatList & mediaFormats  /// Media formats to use
    ) const;

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

    /**Start media streams for session.
      */
    virtual void StartMediaStreams();
    
    /**Pause media streams for session.
      */
    virtual void PauseMediaStreams(BOOL paused);

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
      const OpalMediaFormat & mediaFormat, /// Media format for stream
      unsigned sessionID,                  /// Session number for stream
      BOOL isSource                        /// Is a source stream
    );

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

    /**See if the media can bypass the local host.

       The default behaviour returns FALSE indicating that media bypass is not
       possible.
     */
    virtual BOOL IsMediaBypassPossible(
      unsigned sessionID                  /// Session ID for media channel
    ) const;

    /**Meda information structure for GetMediaInformation() function.
      */
    struct MediaInformation {
      MediaInformation() { rfc2833 = RTP_DataFrame::IllegalPayloadType; }

      OpalTransportAddress data;           /// Data channel address
      OpalTransportAddress control;        /// Control channel address
      RTP_DataFrame::PayloadTypes rfc2833; /// Payload type for RFC2833
    };

    /**Get information on the media channel for the connection.
       The default behaviour checked the mediaTransportAddresses dictionary
       for the session ID and returns information based on that. It also uses
       the rfc2833Handler variable for that part of the info.

       It is up to the descendant class to assure that the mediaTransportAddresses
       dictionary is set correctly before OnIncomingCall() is executed.
     */
    virtual BOOL GetMediaInformation(
      unsigned sessionID,     /// Session ID for media channel
      MediaInformation & info /// Information on media channel
    ) const;

    /**Add video media formats available on a connection.

       The default behaviour calls the OpalEndPoint function of the same name.
      */
    virtual void AddVideoMediaFormats(
      OpalMediaFormatList & mediaFormats  /// Media formats to use
    ) const;

    /**Create an PVideoInputDevice for a source media stream.
      */
    virtual PVideoInputDevice * CreateVideoInputDevice();

    /**Create an PVideoOutputDevice for a sink media stream.
      */
    virtual PVideoOutputDevice * CreateVideoOutputDevice();
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

       If there is no session of the specified ID one is created.

       The type of RTP session that is created will be compatible with the
       transport. At this time only IP (RTp over UDP) is supported.
      */
    virtual RTP_Session * UseSession(
      const OpalTransport & transport,  /// Transport of signalling
      unsigned sessionID,               /// RTP session number
      RTP_QOS * rtpqos = NULL           /// Quiality of Service information
    );

    /**Release the session.
       If the session ID is not being used any more any clients via the
       UseSession() function, then the session is deleted.
     */
    virtual void ReleaseSession(
      unsigned sessionID    /// RTP session number
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

  /**@name User input */
  //@{
    /**Send a user input indication to the remote endpoint.
       This is for sending arbitrary strings as user indications.

       The default behaviour is to call SendUserInputTone() for each character
       in the string.
      */
    virtual BOOL SendUserInputString(
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

       The default behaviour sends the tone using RFC2833.
      */
    virtual BOOL SendUserInputTone(
      char tone,        /// DTMF tone code
      unsigned duration = 0  /// Duration of tone in milliseconds
    );

    /**Call back for remote enpoint has sent user input as a string.
       This will be called irrespective of the source (H.245 string, H.245
       signal or RFC2833).

       The default behaviour calls the endpoint function of the same name.
      */
    virtual void OnUserInputString(
      const PString & value   /// String value of indication
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
      unsigned duration = 500  /// Duration of tone in milliseconds
    ) { SendUserInputTone('!', duration); }

    /**Get a user input indication string, waiting until one arrives.
      */
    virtual PString GetUserInput(
      unsigned timeout = 30   /// Timeout in seconds on input
    );

    /**Set a user indication string.
       This allows the GetUserInput() function to unblock and return this
       string.
      */
    virtual void SetUserInput(
      const PString & input     /// Input string
    );

    /**Read a sequence of user indications with timeouts.
      */
    virtual PString ReadUserInput(
      const char * terminators = "#\r\n", /// Characters that can terminte input
      unsigned lastDigitTimeout = 4,      /// Timeout on last digit in string
      unsigned firstDigitTimeout = 30     /// Timeout on receiving any digits
    );

    /**Play a prompt to the connection before rading user indication string.

       For example the LID connection would play a dial tone.

       The default behaviour does nothing.
      */
    virtual BOOL PromptUserInput(
      BOOL play   /// Flag to start or stop playing the prompt
    );
  //@}

  /**@name Other services */
  //@{
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
    BOOL IsOriginating() const { return originating; }

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

    /**Get the local name/alias.
      */
    const PString & GetLocalPartyName() const { return localPartyName; }

    /**Set the local name/alias.
      */
    virtual void SetLocalPartyName(const PString & name);

    /**Get the local display name.
      */
    const PString & GetDisplayName() const { return displayName; }

    /**Set the local display name.
      */
    void SetDisplayName(const PString & name) { displayName = name; }

    /**Get the caller name/alias.
      */
    const PString & GetRemotePartyName() const { return remotePartyName; }

    /**Get the remote application.
      */
    const PString & GetRemoteApplication() const { return remoteApplication; }
    
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
      unsigned minDelay,   // New minimum jitter buffer delay in milliseconds
      unsigned maxDelay    // New maximum jitter buffer delay in milliseconds
    );

    /**Get the silence detector activate on connection.
     */
    OpalSilenceDetector * GetSilenceDetector() const { return silenceDetector; }

    /**Get the protocol-specific unique identifier for this connection.
     */
    virtual const OpalGloballyUniqueID & GetIdentifier() const
    { return callIdentifier; }
  //@}

  protected:
    PDECLARE_NOTIFIER(OpalRFC2833Info, OpalConnection, OnUserInputInlineRFC2833);
    PDECLARE_NOTIFIER(RTP_DataFrame, OpalConnection, OnUserInputInBandDTMF);
    PDECLARE_NOTIFIER(PThread, OpalConnection, OnReleaseThreadMain);

  // Member variables
    OpalCall             & ownerCall;
    OpalEndPoint         & endpoint;

    Phases               phase;
    PString              callToken;
    OpalGloballyUniqueID callIdentifier;
    BOOL                 originating;
    PTime                setupTime;
    PTime                alertingTime;
    PTime                connectedTime;
    PTime                callEndTime;
    PString              localPartyName;
    PString              displayName;
    PString              remotePartyName;
    PString              remoteApplication;
    PString              remotePartyNumber;
    PString              remotePartyAddress;
    CallEndReason        callEndReason;

    PString               userInputString;
    PMutex                userInputMutex;
    PSyncPoint            userInputAvailable;
    BOOL                  detectInBandDTMF;
    OpalSilenceDetector * silenceDetector;
    OpalRFC2833Proto    * rfc2833Handler;
    OpalT120Protocol    * t120handler;
    OpalT38Protocol     * t38handler;


    PDICTIONARY(MediaAddressesDict, POrdinalKey, OpalTransportAddress);
    MediaAddressesDict  mediaTransportAddresses;
    OpalMediaStreamList mediaStreams;
    RTP_SessionManager  rtpSessions;
    unsigned            minAudioJitterDelay;
    unsigned            maxAudioJitterDelay;
    unsigned            bandwidthAvailable;

    // The In-Band DTMF detector. This is used inside an audio filter which is
    // added to the audio channel.
    PDTMFDecoder        dtmfDecoder;

#if PTRACING
    friend ostream & operator<<(ostream & o, Phases p);
#endif
};


#endif // __OPAL_CONNECTION_H


// End of File ///////////////////////////////////////////////////////////////
