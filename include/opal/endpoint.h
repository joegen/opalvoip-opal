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
 *
 * $Log: endpoint.h,v $
 * Revision 1.2001  2001/07/27 15:48:24  robertj
 * Conversion of OpenH323 to Open Phone Abstraction Library (OPAL)
 *
 */

#ifndef __OPAL_ENDPOINT_H
#define __OPAL_ENDPOINT_H

#ifdef __GNUC__
#pragma interface
#endif


#include <opal/connection.h>
#include <opal/mediafmt.h>


class OpalManager;
class OpalCall;
class OpalMediaStream;
class OpalTransport;


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
    enum Attributes {
      CanTerminateCall = 1,
      HasLineInterface = 2
    };

  /**@name Construction */
  //@{
    /**Create a new endpoint.
     */
    OpalEndPoint(
      OpalManager & manager,          /// Manager of all endpoints.
      const PCaselessString & prefix, /// Prefix for URL style address strings
      unsigned attributes             /// Bit mask of attributes endpoint has
    );

    /**Destroy the endpoint.
     */
    ~OpalEndPoint();
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

  /**@name Connection management */
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
       NULL is returned.

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

       The default behaviour is pure.
     */
    virtual OpalConnection * SetUpConnection(
      OpalCall & call,        /// Owner of connection
      const PString & party,  /// Remote party to call
      void * userData         /// Arbitrary data to pass to connection
    ) = 0;

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

       The default behaviour calls the OpalManager function of the same name.
     */
    virtual BOOL OnIncomingConnection(
      OpalConnection & connection   /// Connection that is calling
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

       The default behaviour calls the OpalManager function of the same name.
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

    /**A call back function whenever a connection is established.
       This indicates that a connection to an endpoint was established. That
       is the endpoint received acknowledgement via whatever protocol it uses
       that the connection may now start media streams.

       In the context of H.323 this means that the signalling and control
       channels are open and the TerminalCapabilitySet and MasterSlave
       negotiations are complete.

       The default behaviour does nothing.
      */
    virtual void OnEstablished(
      OpalConnection & connection   /// Connection that was established
    );

    /**A call back function whenever a connection is broken.
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

       The default behaviour removes the connection from the internal database
       and calls the OpalManager function of the same name.
      */
    virtual BOOL OnReleased(
      OpalConnection & connection   /// Connection that was established
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

    /**Clear a current connection.
       This hangs up the connection to a remote endpoint. Note that these functions
       are synchronous
      */
    virtual BOOL ClearCallSynchronous(
      const PString & token,    /// Token for identifying connection
      OpalConnection::CallEndReason reason =
                  OpalConnection::EndedByLocalUser, /// Reason for call clearing
      PSyncPoint * sync = NULL  /// Sync point to wait on.
    );

    /**Clear all current connections.
       This hangs up all the connections to remote endpoints. The wait
       parameter is used to wait for all the calls to be cleared and their
       memory usage cleaned up before returning. This is typically used in
       the destructor for your descendant of H323EndPoint.
      */
    virtual void ClearAllCalls(
      OpalConnection::CallEndReason reason =
                  OpalConnection::EndedByLocalUser, /// Reason for call clearing
      BOOL wait = TRUE   /// Flag for wait for calls to e cleared.
    );

    /**Find a connection that uses the specified token.
       This searches the endpoint for the connection that contains the token
       as provided by functions such as SetUpConnection().

       Note the caller of this function MUSt call the OpalConnection::Unlock()
       function if this function returns a non-NULL pointer. If it does not
       then a deadlock can occur.
      */
    virtual OpalConnection * GetConnectionWithLock(
      const PString & token     /// Token to identify connection
    );

    /**Determine if a connection is active.
      */
    virtual BOOL HasConnection(
      const PString & token   /// Token for identifying connection
    );
  //@}

  /**@name Media Streams management */
  //@{
    /**Get the data formats this endpoint is capable of operating in.
       This provides a list of media data format names that an
       OpalMediaStream may be created in within connections created by this
       endpoint.

       The default behaviour is pure.
      */
    virtual OpalMediaFormat::List GetMediaFormats() const = 0;

    /**Call back when opening a media stream.
       This function is called when a connection has created a new media
       stream according to the logic of its underlying protocol.

       The usual requirement is that media streams are created on all other
       connections participating in the call and all of the media streams are
       attached to an instance of an OpalMediaPatch object that will read from
       one of the media streams passing data to the other media streams.

       The default behaviour calls the OpalManager function of the same name.
      */
    virtual BOOL OnOpenMediaStream(
      OpalConnection & connection,  /// Connection that owns the media stream
      OpalMediaStream & stream      /// New media stream being opened
    );

    /**Call back for closed a media stream.

       The default behaviour calls the OpalManager function of the same name.
      */
    virtual void OnClosedMediaStream(
      const OpalMediaStream & stream     /// Media stream being closed
    );
  //@}

  /**@name User indications */
  //@{
    /**Call back for remote enpoint has sent user input as a string.

       The default behaviour calls the OpalManager function of the same name.
      */
    virtual void OnUserIndicationString(
      OpalConnection & connection,  /// Connection input has come from
      const PString & value   /// String value of indication
    );

    /**Call back for remote enpoint has sent user input.

       The default behaviour calls the OpalManager function of the same name.
      */
    virtual void OnUserIndicationTone(
      OpalConnection & connection,  /// Connection input has come from
      char tone,                    /// Tone received
      int duration                  /// Duration of tone
    );
  //@}

  /**@name Member variable access */
  //@{
    /**Get the manager for this endpoint.
     */
    OpalManager & GetManager() const { return manager; }

    /**Get the protocol prefix name for the endpoint.
      */
    const PString & GetPrefixName() const { return prefixName; }

    /**Get an indication of if this endpoint has particular option.
      */
    BOOL HasAttribute(Attributes opt) const { return (attributeBits&opt) != 0; }

    /**Get the initial bandwidth parameter.
     */
    WORD GetDefaultSignalPort() const { return defaultSignalPort; }

    /**Get the initial bandwidth parameter.
     */
    unsigned GetInitialBandwidth() const { return initialBandwidth; }

    /**Get the initial bandwidth parameter.
     */
    void SetInitialBandwidth(unsigned bandwidth) { initialBandwidth = bandwidth; }
  //@}


  protected:
    OpalManager   & manager;
    PCaselessString prefixName;
    unsigned        attributeBits;
    WORD            defaultSignalPort;

    unsigned initialBandwidth;  // in 100s of bits/sev

    OpalConnectionDict connectionsActive;
    PSyncPoint         allConnectionsCleared;

    PMutex inUseFlag;
};


PLIST(OpalEndPointList, OpalEndPoint);


#endif // __OPAL_ENDPOINT_H


// End of File ///////////////////////////////////////////////////////////////
