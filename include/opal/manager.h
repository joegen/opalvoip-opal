/*
 * manager.h
 *
 * OPAL system manager.
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
 * $Log: manager.h,v $
 * Revision 1.2001  2001/07/27 15:48:24  robertj
 * Conversion of OpenH323 to Open Phone Abstraction Library (OPAL)
 *
 */

#ifndef __OPAL_MANAGER_H
#define __OPAL_MANAGER_H

#ifdef __GNUC__
#pragma interface
#endif


#include <opal/endpoint.h>
#include <opal/connection.h>
#include <opal/call.h>


class OpalCall;
class OpalMediaPatch;


/**This class is the central manager for OPAL.
   The OpalManager embodies the root of the tree of objects that constitute an
   OPAL system. It contains all of the endpoints that make up the system. Other
   entities such as media streams etc are in turn contained in these objects.
   It is expected that an application would only ever have one instance of
   this class, and also descend from it to override call back functions.

   The manager is the eventual destination for call back indications from
   various other objects. It is possible, for instance, to get an indication
   of a completed call by creating a descendant of the OpalCall and overriding
   the OnEstablished() virtual. However, this could quite unwieldy for all of
   the possible classes, so the default behaviour is to call the equivalent
   function on the OpalManager. This allows most applications to only have to
   create a descendant of the OpalManager and override virtual functions there
   to get all the indications it needs.
 */
class OpalManager : public PObject
{
    PCLASSINFO(OpalManager, PObject);
  public:
  /**@name Construction */
  //@{
    /**Create a new manager.
     */
    OpalManager();

    /**Destroy the manager.
       This will clear all calls, then deletes all endpoints still attached to
       the manager.
     */
    ~OpalManager();
  //@}

  /**@name Endpoint management */
  //@{
    /**Attach a new endpoint to the manager.
       This is an internal function called by the OpalEndPoint constructor.

       Note that any endpoint is automatically "owned" by the manager. They
       should not be deleted directly. The RemoveEndPoint() command should be
       used to do this.
      */
    void AttachEndPoint(
      OpalEndPoint * endpoint
    );

    /**Remove an endpoint from the manager.
       This will delete the endpoint object.
      */
    void RemoveEndPoint(
      OpalEndPoint * endpoint
    );
  //@}

  /**@name Call management */
  //@{
    /**Set up a call between two parties.
       This is used to initiate a call. Incoming calls are "answered" using a
       different mechanism.

       The A party and B party strings indicate the protocol and address of
       the party to call in the style of a URL. The A party is the initiator
       of the call and the B party is the remote system being called. See the
       SetUpConnection() function for more details on the format of these
       strings.

       The token returned is a unique identifier for the call that allows an
       application to gain access to the call at later time. This is necesary
       as the pointer being returned could become invalid (due to being
       deleted) at any time due to the multithreaded nature of the OPAL
       system. While this function does return a pointer it is only safe to
       use immediately after the function returns. At any other time the
       FindCallWithLock() function should be used to gain a locked pointer
       to the call.
     */
    virtual OpalCall * SetUpCall(
      const PString & partyA,       /// The A party of call
      const PString & partyB,       /// The B party of call
      PString & token               /// Token for connection
    );

    /**A call back function whenever a call is completed.
       In telephony terminology a completed call is one where there is an
       established link between two parties.

       This called from the OpalCall::OnEstablished() function.

       The default behaviour does nothing.
      */
    virtual void OnEstablished(
      OpalCall & call   /// Call that was completed
    );

    /**Determine if a call is active.
       Return TRUE if there is an active call with the specified token. Note
       that the call could clear any time (even milliseconds) after this
       function returns TRUE.
      */
    virtual BOOL HasCall(
      const PString & token  /// Token for identifying call
    );

    /**Find a call with the specified token.
       This searches the manager database for the call that contains the token
       as provided by functions such as SetUpCall().

       Note the caller of this function MUST call the OpalCall::Unlock()
       function if this function returns a non-NULL pointer. If it does not
       then a deadlock can occur.
      */
    OpalCall * FindCallWithLock(
      const PString & token  /// Token to identify connection
    );

    /**Clear a call.
       This finds the call by using the token then calls the OpalCall::Clear()
       function on it. All connections are released, and the conenctions and
       call disposed of. Note that this function returns quickly and the
       disposal happens at some later time by a background thread. This it is
       safe to call this function from anywhere.
      */
    virtual BOOL ClearCall(
      const PString & token,    /// Token for identifying connection
      OpalConnection::CallEndReason reason =
                  OpalConnection::EndedByLocalUser, /// Reason for call clearing
      PSyncPoint * sync = NULL  /// Sync point to wait on.
    );

    /**Clear all current calls.
       This effectively executes the OpalCall::Clear() on every call that the
       manager has active.
      */
    virtual void ClearAllCalls(
      OpalConnection::CallEndReason reason =
                  OpalConnection::EndedByLocalUser, /// Reason for call clearing
      BOOL wait = TRUE   /// Flag for wait for calls to e cleared.
    );

    /**A call back function whenever a call is cleared.
       A call is cleared whenever there is no longer any connections attached
       to it. This function is called just before the call is deleted.
       However, it may be sued to display information on the call after
       completion, eg the call parties and duration.

       The default behaviour does nothing.
      */
    virtual void OnClearedCall(
      OpalCall & call   /// Connection that was established
    );

    /**Create a call object.
       This function allows an application to have the system create
       desccendants of the OpalCall class instead of instances of that class
       directly. The application can thus override call backs or add extra
       information that it wishes to maintain on a call by call basis.

       The default behavious returns an instance of OpalCall.
      */
    virtual OpalCall * CreateCall();

    /**Destroy a call object.
       This gets called from background thread that garbage collects all calls
       and connections. If an application has object lifetime issues with the
       threading, it can override this function and take responsibility for
       deleting the object at some later time.

       The default behaviour simply calls "delete call".
      */
    virtual void DestroyCall(
      OpalCall * call
    );

    /**Get next uique token ID for calls.
       This is an internal function called by the OpalCall constructor.
      */
    PString GetNextCallToken();

    /**Attach a new call to the manager.
       This is an internal function called by the OpalCall constructor.

       Note that any call is automatically "owned" by the manager. They
       should not be deleted directly. The ClearCall() command should be
       used to do this.
      */
    void AttachCall(
      OpalCall * call
    );

    /**Clean up calls and connections.
       This is an internal function called from a background thread and
       checks for closed calls and connections to clean up.

       This would not normally be called by an application.
      */
    void GarbageCollection();

    /**Signal background thread to do garbage collection.
       This is an internal function used by the OpalCall to indicate that a
       connection has been released and it should call the GarbageCollection()
       function at its earliest convenience.
      */
    void SignalGarbageCollector();
  //@}

  /**@name Connection management */
  //@{
    /**Set up a connection to a remote party.
       An appropriate protocol (endpoint) is determined from the party
       parameter. That endpoint is then called to create a connection and that
       connection attached to the call provided.
       
       If the endpoint is already occupied in a call then the endpoints list
       is further searched for additional endpoints that support the protocol.
       For example multiple pstn endpoints may be present for multiple LID's.
       
       The general form for this party parameter is:

            [proto:][alias@][transport$]address[:port]

       where the various fields will have meanings specific to the endpoint
       type. For example, with H.323 it could be "h323:Fred@site.com" which
       indicates a user Fred at gatekeeper size.com. Whereas for the PSTN
       endpoint it could be "pstn:5551234" which is to call 5551234 on the
       first available PSTN line.

       The default for the proto is the name of the protocol for the first
       endpoint attached to the manager. Other fields default to values on an
       endpoint basis.

       This function returns almost immediately with the connection continuing
       to occur in a new background thread.
       
       If NULL is returned then the connection could not be established. For
       example if a PSTN endpoint is used and the assiciated line is engaged
       then it may return immediately. Returning a non-NULL value does not
       mean that the connection will succeed, only that an attempt is being
       made.

       Note, the returned pointer to the connection is not locked and may be
       deleted at any time. This is extremely unlikely immediately after the
       function is called, but you should not keep this pointer beyond that
       brief time. The the FindCallWithLock() function for future references
       to the connection object.
     */
    virtual OpalConnection * SetUpConnection(
      OpalCall & call,        /// Owner of connection
      const PString & party   /// Party to call
    );

    /**Call back for answering an incoming call.
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

       The default behaviour call OnRouteConnection to determine a B party for
       the connection.

       If the call associated with the incoming call already had two parties
       and this connection is a third party for a conference call then
       AnswerCallNow is returned as a B party is not required.
     */
    virtual BOOL OnIncomingConnection(
      OpalConnection & connection   /// Connection that is calling
    );

    /**Route a connection to another connection from an endpoint.

       The default behaviour gets the destination address from the connection
       itself.
      */
    virtual PString OnRouteConnection(
      OpalConnection & connection  /// Connection being routed
    );

    /**Call back for remote party being alerted on outgoing call.
       This function is called after the connection is informed that the
       remote endpoint is "ringing". Generally some time after the
       SetUpConnection() function was called, this is function is called.

       If FALSE is returned the connection is aborted.

       If an application overrides this function, it should generally call the
       ancestor version for correct operation. An application would typically
       only intercept this function if it wishes to do some form of logging.
       For this you can obtain the name of the caller by using the function
       OpalConnection::GetRemotePartyName().

       The default behaviour calls the OpalConnection::SendConnectionAlert()
       function on the all the other connections contained in the OpalCall
       this connection is associated with. As a rule that means the A party of
       the call.
     */
    virtual void OnAlerting(
      OpalConnection & connection   /// Connection that was established
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
      OpalConnection & connection   /// Connection that was established
    );

    /**A call back function whenever a connection is released.
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

       The default behaviour indicates to the call that the conection has been
       released so it can release the last remaining connection and then
       returns TRUE.
      */
    virtual BOOL OnReleased(
      OpalConnection & connection   /// Connection that was established
    );
  //@}

  /**@name Media Streams management */
  //@{
    /**Call back when opening a media stream.
       This function is called when a connection has created a new media
       stream according to the logic of its underlying protocol.

       The usual requirement is that media streams are created on all other
       connections participating in the call and all of the media streams are
       attached to an instance of an OpalMediaPatch object that will read from
       one of the media streams passing data to the other media streams.

       The default behaviour achieves the above using the FindMatchingCodecs()
       to determine what (if any) software codecs are required, the
       OpalConnection::CreateMediaStream() function to open streams and the
       CreateMediaPatch() function to create a patch for all of the streams
       and codecs just produced.
      */
    virtual BOOL OnOpenMediaStream(
      OpalConnection & connection,  /// Connection that owns the media stream
      OpalMediaStream & stream    /// New media stream being opened
    );

    /**Call back for closed a media stream.

       The default behaviour does nothing.
      */
    virtual void OnClosedMediaStream(
      const OpalMediaStream & stream     /// Stream being closed
    );

    /**Create a OpalMediaPatch instance.
       This function allows an application to have the system create descendant
       class versions of the OpalMediPatch class. The application could use
       this to modify the default behaviour of a patch.

       The default behaviour returns an instance of OpalMediaPatch.
      */
    virtual OpalMediaPatch * CreateMediaPatch(
      OpalMediaStream & source         /// Source media stream
    );

    /**Destroy a OpalMediaPatch instance.

       The default behaviour simply calls delete patch.
      */
    virtual void DestroyMediaPatch(
      OpalMediaPatch * patch
    );

    /**Call back for a media patch thread starting.
       This function is called within the context of the thread associated
       with the media patch. It may be used to do any last checks on if the
       patch should proceed.

       The default behaviour simply returns TRUE.
      */
    virtual BOOL OnStartMediaPatch(
      const OpalMediaPatch & patch     /// Media patch being started
    );
  //@}

  /**@name User indications */
  //@{
    /**Call back for remote endpoint has sent user input as a string.

       The default behaviour call OpalConnection::SetUserIndication() which
       saves the value so the GetUserIndication() function can return it.
      */
    virtual void OnUserIndicationString(
      OpalConnection & connection,  /// Connection input has come from
      const PString & value         /// String value of indication
    );

    /**Call back for remote enpoint has sent user input as tones.

       The default behaviour creates a string from the tone character and
       calls the OnUserIndicationString() function.
      */
    virtual void OnUserIndicationTone(
      OpalConnection & connection,  /// Connection input has come from
      char tone,                    /// Tone received
      int duration                  /// Duration of tone
    );
  //@}

  /**@name Member variable access */
  //@{
    /**See if should auto-start receive video channels on connection.
     */
    BOOL CanAutoStartReceiveVideo() const { return autoStartReceiveVideo; }

    /**See if should auto-start transmit video channels on connection.
     */
    BOOL CanAutoStartTransmitVideo() const { return autoStartTransmitVideo; }

    /**Get the TCP port number base for signalling channels
     */
    WORD GetTCPPortBase() const { return tcpPortBase; }

    /**Get the TCP port number base for signalling channels.
     */
    WORD GetTCPPortMax() const { return tcpPortMax; }

    /**Provide TCP address translation hook
     */
    virtual void TranslateTCPAddress(PIPSocket::Address & /*localAddr*/, const PIPSocket::Address & /*remoteAddr */) { }

    /**Get the UDP port number base for media (eg RTP) channels.
     */
    WORD GetRtpIpPortBase() const { return rtpIpPortBase; }

    /**Get the max UDP port number for media (eg RTP) channels.
     */
    WORD GetRtpIpPortMax() const { return rtpIpPortMax; }

    /**Get the IP Type Of Service byte for media (eg RTP) channels.
     */
    BYTE GetRtpIpTypeofService() const { return rtpIpTypeofService; }

    /**Set the IP Type Of Service byte for media (eg RTP) channels.
     */
    void SetRtpIpTypeofService(unsigned tos) { rtpIpTypeofService = (BYTE)tos; }

    /**Get the default maximum audio delay jitter parameter.
     */
    WORD GetMaxAudioDelayJitter() const { return maxAudioDelayJitter; }

    /**Set the default maximum audio delay jitter parameter.
     */
    void SetMaxAudioDelayJitter(unsigned jitter) { maxAudioDelayJitter = (WORD)jitter; }
  //@}


  protected:
    // Configuration variables
    BOOL     autoStartReceiveVideo;
    BOOL     autoStartTransmitVideo;
    WORD     rtpIpPortBase, rtpIpPortMax;
    WORD     tcpPortBase, tcpPortMax;
    BYTE     rtpIpTypeofService;
    WORD     maxAudioDelayJitter;

    // Dynamic variables
    PMutex inUseFlag;

    OpalEndPointList endpoints;

    unsigned     lastCallTokenID;
    OpalCallDict callsActive;
    PMutex       callsMutex;
    PSyncPoint   allCallsCleared;
    PThread    * garbageCollector;
    PSyncPoint   garbageCollectFlag;
    BOOL         collectingGarbage;
    PDECLARE_NOTIFIER(PThread, OpalManager, GarbageMain);


  private:
    OpalCall * FindCallWithoutLocks(
      const PString & token     /// Token to identify connection
    );
};


#endif // __OPAL_MANAGER_H


// End of File ///////////////////////////////////////////////////////////////
