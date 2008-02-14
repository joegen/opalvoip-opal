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
 * $Revision$
 * $Author$
 * $Date$
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
      OpalManager & manager   ///<  Manager for the opal system
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
      ostream & strm    ///<  Stream to output text representation
    ) const;
  //@}

  /**@name Basic operations */
  //@{
    /**Indicate tha all connections in call are connected and media is going.
      */
    PBoolean IsEstablished() const { return isEstablished; }

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
      OpalConnection::CallEndReason reason   ///<  Reason for clearance of connection.
    );

    /**Clear call.
       This releases all connections currently attached to the call. Note that
       this function will return quickly as the release and disposal of the
       connections is done by another thread.

       The sync parameter is a PSyncPoint that will be signalled during the
       destructor for the OpalCall. Note only one thread may do this at a time.
     */
    void Clear(
      OpalConnection::CallEndReason reason = OpalConnection::EndedByLocalUser, ///<  Reason for call clearing
      PSyncPoint * sync = NULL                                                 ///<  Sync point to signal on call destruction
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
    virtual PBoolean OnSetUp(
      OpalConnection & connection   ///<  Connection that indicates it is alerting
    );

    /**Call back for alerting.

       The default behaviour is to call SetAlerting() on all the other
       connections in the call.
      */
    virtual PBoolean OnAlerting(
      OpalConnection & connection   ///<  Connection that indicates it is alerting
    );

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

       The default behaviour returns AnswerCallPending.
     */
    virtual OpalConnection::AnswerCallResponse OnAnswerCall(
      OpalConnection & connection,
      const PString & caller
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
    virtual PBoolean OnConnected(
      OpalConnection & connection   ///<  Connection that indicates it is alerting
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
    virtual PBoolean OnEstablished(
      OpalConnection & connection   ///<  Connection that indicates it is alerting
    );

    /**A call back function whenever a connection is released.

       The default behaviour releases the remaining connection if there is
       only one left.
      */
    virtual void OnReleased(
      OpalConnection & connection   ///<  Connection that was established
    );

    /**Get the other party's connection object.
       This will return the other party in the call. It will return NULL if
       there is no other party yet, or there are more than two parties in the
       call. Usefull during certain stages during initial call set up.
      */
    PSafePtr<OpalConnection> GetOtherPartyConnection(
      const OpalConnection & connection  ///<  Source requesting formats
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
    virtual OpalMediaFormatList GetMediaFormats(
      const OpalConnection & connection,  ///<  Connection requesting formats
      PBoolean includeSpecifiedConnection     ///<  Include parameters media
    );

    /**Open source media streams for the specified connection.
       A source media stream is opened for the connection, if successful then
       sink media streams are created for every other connection in the call.
       If at least one sink is created then an OpalMediaPatch is created to
       transfer data from the source to the sinks.
      */
    virtual bool OpenSourceMediaStreams(
      OpalConnection & connection,              ///<  Connection requesting open
      unsigned sessionID                        ///<  Session to start streams on
    );

    /**Select media format pair from the source/destination list.

       Default behavour calls OpalTranscoder::SelectFormats().
      */
    virtual bool SelectMediaFormats(
      unsigned sessionID,                     ///<  Session for selection
      const OpalMediaFormatList & srcFormats, ///<  Names of possible source formats
      const OpalMediaFormatList & dstFormats, ///<  Names of possible destination formats
      const OpalMediaFormatList & allFormats, ///<  Master list of formats for merging options
      OpalMediaFormat & srcFormat,            ///<  Selected source format to be used
      OpalMediaFormat & dstFormat             ///<  Selected destination format to be used
    ) const;

    /**Callback from the RTP session for statistics monitoring.
       This is called every so many packets on the transmitter and receiver
       threads of the RTP session indicating that the statistics have been
       updated.

       The default behaviour does nothing.
      */
    virtual void OnRTPStatistics(
      const OpalConnection & connection,  ///<  Connection for the channel
      const RTP_Session & session         ///<  Session with statistics
    );

    /**Close the media streams on the connections.
     */
    virtual void CloseMediaStreams();
    
    /**See if the media can bypass the local host.
     */
    virtual PBoolean IsMediaBypassPossible(
      const OpalConnection & connection,  ///<  Source connection
      unsigned sessionID                  ///<  Session ID for media channel
    ) const;
  //@}

  /**@name User indications */
  //@{
    /**Call back for remote endpoint has sent user input as a string.

       The default behaviour call OpalConnection::SetUserInput() which
       saves the value so the GetUserInput() function can return it.
      */
    virtual void OnUserInputString(
      OpalConnection & connection,  ///<  Connection input has come from
      const PString & value         ///<  String value of indication
    );

    /**Call back for remote enpoint has sent user input as tones.
       If duration is zero then this indicates the beginning of the tone. If
       duration is non-zero then it indicates the end of the tone output.

       The default behaviour calls connection.OnUserInputString(tone) if there
       are no other connections in the call, otherwise it calls
       SendUserInputTone() for each of the other connections in the call.
      */
    virtual void OnUserInputTone(
      OpalConnection & connection,  ///<  Connection input has come from
      char tone,                    ///<  Tone received
      int duration                  ///<  Duration of tone in milliseconds
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

    virtual PBoolean StartRecording(const PFilePath & fn);
    virtual void StopRecording();
    void OnStopRecordAudio(const PString & callToken);

  protected:
    OpalManager & manager;

    PString myToken;

    PString partyA;
    PString partyB;
    PTime   startTime;
    PBoolean    isEstablished;
    PBoolean    isClearing;

    OpalConnection::CallEndReason callEndReason;

    PSafeList<OpalConnection> connectionsActive;

    PSyncPoint * endCallSyncPoint;

    
  //use to add the connection to the call's connection list
  friend OpalConnection::OpalConnection(OpalCall &, OpalEndPoint &, const PString &, unsigned int, OpalConnection::StringOptions *);
  //use to remove the connection from the call's connection list
  friend OpalConnection::~OpalConnection();
};


#endif // __OPAL_CALL_H


// End of File ///////////////////////////////////////////////////////////////
