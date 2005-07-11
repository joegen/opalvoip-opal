/*
 * call.h
 *
 * Telephone call management
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
 * $Log: call.h,v $
 * Revision 1.2021  2005/07/11 01:52:23  csoutheren
 * Extended AnsweringCall to work for SIP as well as H.323
 * Fixed problems with external RTP connection in H.323
 * Added call to OnClosedMediaStream
 *
 * Revision 2.19  2005/05/11 04:25:09  dereksmithies
 * Add description of the OpalConnection class instances managed by an OpalCall structure.
 *
 * Revision 2.18  2004/08/14 07:56:29  rjongbloed
 * Major revision to utilise the PSafeCollection classes for the connections and calls.
 *
 * Revision 2.17  2004/07/14 13:26:14  rjongbloed
 * Fixed issues with the propagation of the "established" phase of a call. Now
 *   calling an OnEstablished() chain like OnAlerting() and OnConnected() to
 *   finally arrive at OnEstablishedCall() on OpalManager
 *
 * Revision 2.16  2004/05/01 10:00:50  rjongbloed
 * Fixed ClearCallSynchronous so now is actually signalled when call is destroyed.
 *
 * Revision 2.15  2004/04/18 07:09:12  rjongbloed
 * Added a couple more API functions to bring OPAL into line with similar functions in OpenH323.
 *
 * Revision 2.14  2004/03/11 06:54:27  csoutheren
 * Added ability to disable SIP or H.323 stacks
 *
 * Revision 2.13  2004/02/07 00:35:46  rjongbloed
 * Fixed calls GetMediaFormats so no DOES return intersection of all connections formats.
 * Tidied some API elements to make usage more explicit.
 *
 * Revision 2.12  2003/06/02 03:13:28  rjongbloed
 * Made changes so that media stream in opposite direction to the one already
 *   opened will use same media format for preference. That is try and use
 *   symmetric codecs if possible.
 *
 * Revision 2.11  2003/03/06 03:57:46  robertj
 * IVR support (work in progress) requiring large changes everywhere.
 *
 * Revision 2.10  2002/11/10 11:33:17  robertj
 * Updated to OpenH323 v1.10.3
 *
 * Revision 2.9  2002/09/16 02:52:35  robertj
 * Added #define so can select if #pragma interface/implementation is used on
 *   platform basis (eg MacOS) rather than compiler, thanks Robert Monaghan.
 *
 * Revision 2.8  2002/04/08 02:40:13  robertj
 * Fixed issues with using double originate call, eg from simple app command line.
 *
 * Revision 2.7  2002/02/19 07:42:25  robertj
 * Restructured media bypass functions to fix problems with RFC2833.
 *
 * Revision 2.6  2002/02/11 07:38:01  robertj
 * Added media bypass for streams between compatible protocols.
 *
 * Revision 2.5  2002/01/22 05:03:47  robertj
 * Revamp of user input API triggered by RFC2833 support
 *
 * Revision 2.4  2001/11/15 07:02:12  robertj
 * Changed OpalCall::OpenSourceMediaStreams so the connection to not open
 *   a media stream on is optional.
 *
 * Revision 2.3  2001/08/22 10:20:09  robertj
 * Changed connection locking to use double mutex to guarantee that
 *   no threads can ever deadlock or access deleted connection.
 *
 * Revision 2.2  2001/08/17 08:24:33  robertj
 * Added call end reason for whole call, not just connection.
 *
 * Revision 2.1  2001/08/01 05:28:59  robertj
 * Added function to get all media formats possible in a call.
 *
 * Revision 2.0  2001/07/27 15:48:24  robertj
 * Conversion of OpenH323 to Open Phone Abstraction Library (OPAL)
 *
 */

#ifndef __OPAL_CALL_H
#define __OPAL_CALL_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <opal/buildopts.h>

#include <opal/connection.h>
#include <opal/guid.h>

#include <ptlib/safecoll.h>


class OpalManager;


/**This class manages a call.
   A call consists of one or more OpalConnection instances. While these
   connections may be created elsewhere this class is responsible for their
   disposal.

   An OpalCall could manage (for example) a H323Connection and
   PCSSConnection instance, which allows the user to use opal in a
   H.323 application. Alternatively, if OpalCall manages a
   H323Connection and a SIPConnection instance, the call is being
   gatewayed from one protocol to another.

   In a conference situation, one OpalCall would manage lots of
   H323Connection/SIPConnection classes.
 */
class OpalCall : public PSafeObject
{
    PCLASSINFO(OpalCall, PSafeObject);
  public:
  /**@name Construction */
  //@{
    /**Create a new call.
     */
    OpalCall(
      OpalManager & manager   /// Manager for the opal system
    );

    /**Destroy call.
     */
    ~OpalCall();
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
    /**Indicate tha all connections in call are connected and media is going.
      */
    BOOL IsEstablished() const { return isEstablished; }

    /**Call back to indicate that the call has been established.
       At this point in time there are no connections left in the call.

       The default behaviour is to call OpalManager::OnEstablishedCall().
      */
    virtual void OnEstablishedCall();

    /**Get the call clearand reason for this connection shutting down.
       Note that this function is only generally useful in the
       H323EndPoint::OnConnectionCleared() function. This is due to the
       connection not being cleared before that, and the object not even
       exiting after that.

       If the call is still active then this will return OpalConnection::NumCallEndReasons.
      */
    OpalConnection::CallEndReason GetCallEndReason() const { return callEndReason; }

    /**Set the call clearance reason.
       An application should have no cause to use this function. It is present
       for the H323EndPoint::ClearCall() function to set the clearance reason.
      */
    void SetCallEndReason(
      OpalConnection::CallEndReason reason   /// Reason for clearance of connection.
    );

    /**Clear call.
       This releases all connections currently attached to the call. Note that
       this function will return quickly as the release and disposal of the
       connections is done by another thread.

       The sync parameter is a PSyncPoint that will be signalled during the
       destructor for the OpalCall. Note only one thread may do this at a time.
     */
    void Clear(
      OpalConnection::CallEndReason reason = OpalConnection::EndedByLocalUser, /// Reason for call clearing
      PSyncPoint * sync = NULL                                                 /// Sync point to signal on call destruction
    );

    /**Call back to indicate that the call has been cleared.
       At this point in time there are no connections left in the call.

       The default behaviour is to call OpalManager::OnClearedCall().
      */
    virtual void OnCleared();
  //@}

  /**@name Connection management */
  //@{
    /**Call back for SetUp conenction.

       The default behaviour is to call SetUpConnection() on all the other
       connections in the call.
      */
    virtual BOOL OnSetUp(
      OpalConnection & connection   /// Connection that indicates it is alerting
    );

    /**Call back for alerting.

       The default behaviour is to call SetAlerting() on all the other
       connections in the call.
      */
    virtual BOOL OnAlerting(
      OpalConnection & connection   /// Connection that indicates it is alerting
    );

    virtual OpalConnection::AnswerCallResponse
       OnAnswerCall(OpalConnection & connection,
                     const PString & caller
    );

    virtual void AnsweringCall(
      OpalConnection::AnswerCallResponse response /// Answer response to incoming call
    );

    /**A call back function whenever a connection is "connected".
       This indicates that a connection to an endpoint was connected. That
       is the endpoint received acknowledgement via whatever protocol it uses
       that the connection may now start media streams.

       In the context of H.323 this means that the CONNECT pdu has been
       received.

       The default behaviour is to call SetConnected() on all other
       connections in the call.
      */
    virtual BOOL OnConnected(
      OpalConnection & connection   /// Connection that indicates it is alerting
    );

    /**A call back function whenever a connection is "established".
       This indicates that a connection to an endpoint was connected and that
       media streams are opened. 

       In the context of H.323 this means that the CONNECT pdu has been
       received and either fast start was in operation or the subsequent Open
       Logical Channels have occurred. For SIP it indicates the INVITE/OK/ACK
       sequence is complete.

       The default behaviour is to check that all connections in call are
       established and if so, marks the call as established and calls
       OnEstablishedCall().
      */
    virtual BOOL OnEstablished(
      OpalConnection & connection   /// Connection that indicates it is alerting
    );

    /**A call back function whenever a connection is released.

       The default behaviour releases the remaining connection if there is
       only one left.
      */
    virtual void OnReleased(
      OpalConnection & connection   /// Connection that was established
    );

    /**Get the other party's connection object.
       This will return the other party in the call. It will return NULL if
       there is no other party yet, or there are more than two parties in the
       call. Usefull during certain stages during initial call set up.
      */
    PSafePtr<OpalConnection> GetOtherPartyConnection(
      const OpalConnection & connection  /// Source requesting formats
    ) const;

    /**Get the specified active connection in call.
      */
    PSafePtr<OpalConnection> GetConnection(
      PINDEX idx,
      PSafetyMode mode = PSafeReference
    ) { return connectionsActive.GetAt(idx, mode); }
  //@}

  /**@name Media management */
  //@{
    /**Get the media formats of the connections in call.
       This returns the intersection of all the media formats that all
       connections in the call, optionally excepting the one provided as a
       parameter, are capable of.

       This will also add to the list all media formats for which there are
       transcoders registered.
      */
    OpalMediaFormatList GetMediaFormats(
      const OpalConnection & connection,  /// Connection requesting formats
      BOOL includeSpecifiedConnection     /// Include parameters media
    );

    /**Open transmitter media streams for each connection.
       All connections in the call except the one specified are requested to
       open a source media stream. If successful, then PatchMediaStreams() is
       called which in turns starts the sink media stream on the connection.
      */
    virtual BOOL OpenSourceMediaStreams(
      const OpalConnection & connection,        /// Connection requesting open
      const OpalMediaFormatList & mediaFormats, /// Optional media format to open
      unsigned sessionID                        /// Session to start streams on
    );

    /**Connect up the media streams on the connections.
       This creates a media patch and a sink media stream on the specified
       connection for the specified source media stream. The created media
       patch is a thread, but it is not started immediately.
     */
    virtual BOOL PatchMediaStreams(
      const OpalConnection & connection, /// Source connection
      OpalMediaStream & source           /// Source media stream to patch
    );

    /**See if the media can bypass the local host.
     */
    virtual BOOL IsMediaBypassPossible(
      const OpalConnection & connection,  /// Source connection
      unsigned sessionID                  /// Session ID for media channel
    ) const;
  //@}

  /**@name User indications */
  //@{
    /**Call back for remote endpoint has sent user input as a string.

       The default behaviour call OpalConnection::SetUserInput() which
       saves the value so the GetUserInput() function can return it.
      */
    virtual void OnUserInputString(
      OpalConnection & connection,  /// Connection input has come from
      const PString & value         /// String value of indication
    );

    /**Call back for remote enpoint has sent user input as tones.
       If duration is zero then this indicates the beginning of the tone. If
       duration is non-zero then it indicates the end of the tone output.

       The default behaviour calls connection.OnUserInputString(tone) if there
       are no other connections in the call, otherwise it calls
       SendUserInputTone() for each of the other connections in the call.
      */
    virtual void OnUserInputTone(
      OpalConnection & connection,  /// Connection input has come from
      char tone,                    /// Tone received
      int duration                  /// Duration of tone in milliseconds
    );
  //@}

  /**@name Member variable access */
  //@{
    /**Get the manager for this endpoint.
     */
    OpalManager & GetManager() const { return manager; }

    /**Get the internal identifier token for this connection.
     */
    const PString & GetToken() const { return myToken; }

    /**Get the A party for the call.
       Note this will be available even after the A party connection has been
       released from the call.
     */
    const PString & GetPartyA() const { return partyA; }

    /**Get the B party for the call.
       Note this will be available even after the B party connection has been
       released from the call. Also this will only be the first B party if the
       object represents a conference call with more that 2 parties.
     */
    const PString & GetPartyB() const { return partyB; }

    /**Set the B party for a call.
       This is used when we wish to make two outgoing calls and bridge them.
       When the OnConnected() call back occurs for the first outgoing call
       (the A-Party) then this variable ised to make teh second outgoing call.
      */
    void SetPartyB(
      const PString & b
    ) { partyB = b; }

    /**Get the time the call started.
     */
    const PTime & GetStartTime() const { return startTime; }
  //@}


  protected:
    OpalManager & manager;

    PString myToken;

    PString partyA;
    PString partyB;
    PTime   startTime;
    BOOL    isEstablished;

    OpalConnection::CallEndReason callEndReason;

    PSafeList<OpalConnection> connectionsActive;

    PSyncPoint * endCallSyncPoint;

  friend OpalConnection::OpalConnection(OpalCall & call, OpalEndPoint & ep, const PString & token);
};


#endif // __OPAL_CALL_H


// End of File ///////////////////////////////////////////////////////////////
