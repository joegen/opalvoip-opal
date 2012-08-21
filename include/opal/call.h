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

#ifndef OPAL_OPAL_CALL_H
#define OPAL_OPAL_CALL_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <opal/buildopts.h>

#include <opal/connection.h>
#include <opal/recording.h>
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
    PBoolean IsEstablished() const { return m_isEstablished; }

    /**Call back to indicate that the call has been established.
       At this point in time every connection in the call is in the
       "Established" state. This is a better function than using
       OnEstablished() on a particular connection as it assures that
       bi-direction media is flowing.

       The default behaviour is to call OpalManager::OnEstablishedCall().
      */
    virtual void OnEstablishedCall();

    /**Get the call clearand reason for this connection shutting down.
       Note that this function is only generally useful in the
       OpalEndPoint::OnClearedCall() function. This is due to the
       connection not being cleared before that, and the object not even
       existing after that.

       If the call is still active then this will return OpalConnection::NumCallEndReasons.
      */
    OpalConnection::CallEndReason GetCallEndReason() const { return callEndReason; }

    /**Get the reason for this connection shutting down as text.
      */
    PString GetCallEndReasonText() const { return OpalConnection::GetCallEndReasonText(callEndReason); }

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
    /**Call back for a new connection has been constructed.
       This is called after CreateConnection has returned a new connection.
       It allows an application to make any custom adjustments to the
       connection before it begins to process the protocol. behind it.
      */
    virtual void OnNewConnection(
      OpalConnection & connection   ///< New connection just created
    );

    /**Call back for SetUp conenction.

       The default behaviour is to call SetUpConnection() on all the other
       connections in the call.
      */
    virtual PBoolean OnSetUp(
      OpalConnection & connection   ///<  Connection that indicates it is alerting
    );

    /**Call back for remote party is now responsible for completing the call.
       This function is called when the remote system has been contacted and it
       has accepted responsibility for completing, or failing, the call. This
       is distinct from OnAlerting() in that it is not known at this time if
       anything is ringing. This indication may be used to distinguish between
       "transport" level error, in which case another host may be tried, and
       that finalising the call has moved "upstream" and the local system has
       no more to do but await a result.

       If an application overrides this function, it should generally call the
       ancestor version for correct operation.

       The default behaviour does nothing.
     */
    virtual void OnProceeding(
      OpalConnection & connection   ///<  Connection that is proceeeding
    );

    /**Call back for remote party being alerted.
       This function is called after the connection is informed that the
       remote endpoint is "ringing". Generally some time after the
       MakeConnection() function was called, this is function is called.

       If false is returned the connection is aborted.

       If an application overrides this function, it should generally call the
       ancestor version for correct operation. An application would typically
       only intercept this function if it wishes to do some form of logging.
       For this you can obtain the name of the caller by using the function
       OpalConnection::GetRemotePartyName().

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
       This indicates that a connection to an endpoint was established. This
       usually occurs after OnConnected() and indicates that the connection
       is both connected and has media flowing.

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

    /**A call back function whenever a connection is "held" or "retrieved".
       This indicates that a connection of a call was held, or
       retrieved, either locally or by the remote endpoint.

       The default behaviour does nothing.
      */
    virtual void OnHold(
      OpalConnection & connection,   ///<  Connection that was held/retrieved
      bool fromRemote,               ///<  Indicates remote has held local connection
      bool onHold                    ///<  Indicates have just been held/retrieved.
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

    /**Find a connection of the specified class.
       This searches the call for the Nth connection of the specified class.
      */
    template <class ConnClass>
    PSafePtr<ConnClass> GetConnectionAs(
      PINDEX count = 0,
      PSafetyMode mode = PSafeReadWrite
    )
    {
      PSafePtr<ConnClass> connection;
      for (PSafePtr<OpalConnection> iterConn(connectionsActive, PSafeReference); iterConn != NULL; ++iterConn) {
        if ((connection = PSafePtrCast<OpalConnection, ConnClass>(iterConn)) != NULL && count-- == 0) {
          if (!connection.SetSafetyMode(mode))
            connection.SetNULL();
          break;
        }
      }
      return connection;
    }

    /**Put call on hold.
       This function places the remote user of the network connection in the
       call on hold.
      */
    bool Hold();

    /**Retrieve call from hold.
       This function retrieves the remote user of the network connection in
       the call from hold.
      */
    bool Retrieve();

    /**Indicate if call is in hold.
       This function determines if the remote user of the network connection
       in the call is currently on hold.
      */
    bool IsOnHold() const;

    /**Transfer connection.
       There are several scenarios for this function.

       If 'connection' is not NULL and the protocol type for 'address' is
       the same, or the protocol type is "*", or the address is a valid and
       current call token, then this simply calls TransferConnection() on the
       'connection' variable.
       
       e.g. connection="sip:fred@bloggs.com" and address="*:1234", then
       connection->TransferConnection("sip:1234") is called, which sends a
       REFER command to the remote. The connection and call are subsequently
       released in this case.

       Another example, if connection="pc:Speakers" and address="pc:Headset",
       then the sound device in the 'connection' instance is changed and the
       call and media is continued unchanged. The connection and call are
       <b>not</b> released in this case.

       If there is a protocol change, e.g. "pc:" to "t38:", then 'connection'
       is completely released, all media streams to the second conection in
       the call (if any) are severed and a new connection established and new
       media streams started.

       If 'connection' is NULL, it will choose the first connection in the call
       of the same protocol type. For example, in the previous example above
       where address="pc:Headset", and the call is from "pc:Speakers" to
       "sip:fred@bloggs.com", the function would operate the same even if
       'connection' is NULL.

       If there are no connections of the same protocol type, then nothing is
       done and false is returned.
      */
    bool Transfer(
      const PString & address,           ///< New address to transfer to
      OpalConnection * connection = NULL ///< Connection to transfer
    );
  //@}

  /**@name Media management */
  //@{
    /**Get the media formats of the connections in call.
       This returns the intersection of all the media formats available in
       connections in the call, not including the parameter. It is in
       esence the "local" media formats available to the connection.

       This will also add to the list all media formats for which there are
       transcoders registered.
      */
    virtual OpalMediaFormatList GetMediaFormats(
      const OpalConnection & connection  ///<  Connection requesting formats
    );

    /**Adjust media formats available on a connection.
       This is called by a connection after it has called
       OpalCall::GetMediaFormats() to get all media formats that it can use so
       that an application may remove or reorder the media formats before they
       are used to open media streams.

       The default behaviour calls the OpalManager function of the same name.
      */
    virtual void AdjustMediaFormats(
      bool local,                         ///<  Media formats a local ones to be presented to remote
      const OpalConnection & connection,  ///<  Connection that is about to use formats
      OpalMediaFormatList & mediaFormats  ///<  Media formats to use
    ) const;

    /**Open source media streams for the specified connection.
       A source media stream is opened for the connection, if successful then
       sink media streams are created for every other connection in the call.
       If at least one sink is created then an OpalMediaPatch is created to
       transfer data from the source to the sinks.

       If session ID is zero a new session is created. If session ID is non
       zero then that existing session is replaced.
      */
    virtual bool OpenSourceMediaStreams(
      OpalConnection & connection,              ///<  Connection requesting open
      const OpalMediaType & mediaType,          ///<  Media type of channel to open
      unsigned sessionID = 0,                   ///<  Session to start streams on
      const OpalMediaFormat & preselectedFormat = OpalMediaFormat()  ///< Format for source stream to choose from
#if OPAL_VIDEO
      , OpalVideoFormat::ContentRole contentRole = OpalVideoFormat::eNoRole ///< Content role for video
#endif
    );

    /**Select media format pair from the source/destination list.

       Default behavour calls OpalTranscoder::SelectFormats().
      */
    virtual bool SelectMediaFormats(
      const OpalMediaType & mediaType,        ///< Media type for selection.
      const OpalMediaFormatList & srcFormats, ///<  Names of possible source formats
      const OpalMediaFormatList & dstFormats, ///<  Names of possible destination formats
      const OpalMediaFormatList & allFormats, ///<  Master list of formats for merging options
      OpalMediaFormat & srcFormat,            ///<  Selected source format to be used
      OpalMediaFormat & dstFormat             ///<  Selected destination format to be used
    ) const;

    /**Start the media streams on the connections.
     */
    virtual void StartMediaStreams();

    /**Close the media streams on the connections.
     */
    virtual void CloseMediaStreams();
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

    /**Get the A party URI for the call.
       Note this will be available even after the A party connection has been
       released from the call.
     */
    const PString & GetPartyA() const { return m_partyA; }

    /**Get the B party URI for the call.
       Note this will be available even after the B party connection has been
       released from the call. Also this will only be the first B party if the
       object represents a conference call with more that 2 parties.
     */
    const PString & GetPartyB() const { return m_partyB; }

    /**Set the B party for a call.
       This is used when we wish to make two outgoing calls and bridge them.
       When the OnConnected() call back occurs for the first outgoing call
       (the A-Party) then this variable ised to make teh second outgoing call.
      */
    void SetPartyB(
      const PString & b
    ) { m_partyB = b; }

    /**Get the A party display name for the call.
       Note this will be available even after the A party connection has been
       released from the call.
     */
    const PString & GetNameA() const { return m_nameA; }

    /**Get the B party display name for the call.
       Note this will be available even after the B party connection has been
       released from the call. Also this will only be the first B party if the
       object represents a conference call with more that 2 parties.
     */
    const PString & GetNameB() const { return m_nameB; }

    /**Get indication that A-Party is the network.
       This will indicate if the call is "incoming" or "outgoing" by looking at
       the type of the A-party connection.
      */
    bool IsNetworkOriginated() const { return m_networkOriginated; }

    /**Get remote/network party URI.
      */
    const PString & GetRemoteParty() const { return m_networkOriginated ? m_partyA : m_partyB; }

    /**Get local party URI.
      */
    const PString & GetLocalParty() const { return m_networkOriginated ? m_partyB : m_partyA; }

    /**Get remote/network party display name.
      */
    const PString & GetRemoteName() const { return m_networkOriginated ? m_nameA : m_nameB; }

    /**Get local party display name.
      */
    const PString & GetLocalName() const { return m_networkOriginated ? m_nameB : m_nameA; }

    /**Get the time the call started.
     */
    const PTime & GetStartTime() const { return m_startTime; }

    /**Get the time the call started.
     */
    const PTime & GetEstablishedTime() const { return m_establishedTime; }
  //@}

#if OPAL_HAS_MIXER
    /**Start recording a call.
       Current version saves to a WAV file. It may either mix the receive and
       transmit audio stream to a single mono file, or the streams are placed
       into the left and right channels of a stereo WAV file.
      */
    bool StartRecording(
      const PFilePath & filename, ///< File into which to record
      const OpalRecordManager::Options & options = false ///< Record mixing options
    );

    /**Indicate if recording is currently active on call.
      */
    bool IsRecording() const;

    /** Stop a recording.
        Returns true if the call does exists, an active call is not indicated.
      */
    void StopRecording();

    /** Call back on recording started.
      */
    virtual bool OnStartRecording(
      const PString & streamId,       ///< Unique ID for stream within call
      const OpalMediaFormat & format  ///< Media format for stream
    );

    /** Call back on recording stopped.
      */
    virtual void OnStopRecording(
      const PString & streamId       ///< Unique ID for stream within call
    );

    /** Call back for having a frame of audio to record.
      */
    virtual void OnRecordAudio(
      const PString & streamId,       ///< Unique ID for stream within call
      const RTP_DataFrame & frame     ///< Media data
    );

#if OPAL_VIDEO
    /** Call back for having a frame of video to record.
      */
    virtual void OnRecordVideo(
      const PString & streamId,       ///< Unique ID for stream within call
      const RTP_DataFrame & frame     ///< Media data
    );
#endif
#endif // OPAL_HAS_MIXER

    void InternalOnClear();

    void SetPartyNames();

  protected:
    bool EnumerateConnections(
      PSafePtr<OpalConnection> & connection,
      PSafetyMode mode,
      const OpalConnection * skipConnection = NULL
    ) const;

    OpalManager & manager;

    PString myToken;

    PString m_partyA;
    PString m_partyB;
    PString m_nameA;
    PString m_nameB;
    bool    m_networkOriginated;
    PTime   m_startTime;
    PTime   m_establishedTime;
    bool    m_isEstablished;
    bool    m_isClearing;

    OpalConnection::CallEndReason callEndReason;
    std::list<PSyncPoint *> m_endCallSyncPoint;

    PSafeList<OpalConnection> connectionsActive;

#if OPAL_HAS_MIXER
    OpalRecordManager * m_recordManager;
#endif

#if OPAL_SCRIPT
    PDECLARE_ScriptFunctionNotifier(OpalCall, ScriptClear);
#endif

  //use to add the connection to the call's connection list
  friend OpalConnection::OpalConnection(OpalCall &, OpalEndPoint &, const PString &, unsigned int, OpalConnection::StringOptions *);
  //use to remove the connection from the call's connection list
  friend OpalConnection::~OpalConnection();

  P_REMOVE_VIRTUAL_VOID(OnRTPStatistics(const OpalConnection &, const OpalRTPSession &));
};


#endif // OPAL_OPAL_CALL_H


// End of File ///////////////////////////////////////////////////////////////
