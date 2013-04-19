/*
 * localep.h
 *
 * Local EndPoint/Connection.
 *
 * Open Phone Abstraction Library (OPAL)
 * Formally known as the Open H323 project.
 *
 * Copyright (c) 2008 Vox Lucida Pty. Ltd.
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

#ifndef OPAL_OPAL_LOCALEP_H
#define OPAL_OPAL_LOCALEP_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <opal_config.h>

#include <opal/endpoint.h>

class OpalLocalConnection;


/** Local EndPoint.
    This class represents an endpoint on the local machine that can receive
    media via virtual functions.
 */
class OpalLocalEndPoint : public OpalEndPoint
{
    PCLASSINFO(OpalLocalEndPoint, OpalEndPoint);
  public:
  /**@name Construction */
  //@{
    /**Create a new endpoint.
     */
    OpalLocalEndPoint(
      OpalManager & manager,        ///<  Manager of all endpoints.
      const char * prefix = "local" ///<  Prefix for URL style address strings
    );

    /**Destroy endpoint.
     */
    ~OpalLocalEndPoint();
  //@}

  /**@name Overrides from OpalEndPoint */
  //@{
    /**Get the data formats this endpoint is capable of operating.
       This provides a list of media data format names that may be used by an
       OpalMediaStream may be created by a connection from this endpoint.

       Note that a specific connection may not actually support all of the
       media formats returned here, but should return no more.

       The default behaviour returns the most basic media formats, PCM audio
       and YUV420P video.
      */
    virtual OpalMediaFormatList GetMediaFormats() const;

    /**Set up a connection to a remote party.
       This is called from the OpalManager::MakeConnection() function once
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
       false is returned.

       This function usually returns almost immediately with the connection
       continuing to occur in a new background thread.

       If false is returned then the connection could not be established. For
       example if a PSTN endpoint is used and the assiciated line is engaged
       then it may return immediately. Returning a non-NULL value does not
       mean that the connection will succeed, only that an attempt is being
       made.

       The default behaviour is pure.
     */
    virtual PSafePtr<OpalConnection> MakeConnection(
      OpalCall & call,           ///<  Owner of connection
      const PString & party,     ///<  Remote party to call
      void * userData = NULL,    ///<  Arbitrary data to pass to connection
      unsigned int options = 0,  ///<  options to pass to conneciton
      OpalConnection::StringOptions * stringOptions  = NULL ///< Options to pass to connection
    );
  //@}

  /**@name Customisation call backs */
  //@{
    /**Find a connection that uses the specified token.
       This searches the endpoint for the connection that contains the token
       as provided by functions such as MakeConnection(). If not then it
       attempts to use the token as a OpalCall token and find a connection
       of the same class.
      */
    PSafePtr<OpalLocalConnection> GetLocalConnectionWithLock(
      const PString & token,     ///<  Token to identify connection
      PSafetyMode mode = PSafeReadWrite   ///< Lock mode
    ) { return GetConnectionWithLockAs<OpalLocalConnection>(token, mode); }

    /**Create a connection for the PCSS endpoint.
       The default implementation is to create a OpalLocalConnection.
      */
    virtual OpalLocalConnection * CreateConnection(
      OpalCall & call,    ///<  Owner of connection
      void * userData,    ///<  Arbitrary data to pass to connection
      unsigned options,   ///< Option bit mask to pass to connection
      OpalConnection::StringOptions * stringOptions ///< Options to pass to connection
    );

    /**Call back just before remote is contacted.
       If false is returned the call is aborted with EndedByNoAccept.

       The default implementation returns true.
      */
    virtual bool OnOutgoingSetUp(
      const OpalLocalConnection & connection ///<  Connection having event
    );

    /**Call back to indicate that remote is ringing.
       If false is returned the call is aborted.

       The default implementation does nothing.
      */
    virtual bool OnOutgoingCall(
      const OpalLocalConnection & connection ///<  Connection having event
    );

    /**Call back to indicate that there is an incoming call.
       Note this function should not block or it will impede the operation of
       the stack.

       The default implementation returns true;

       @return false is returned the call is aborted with status of EndedByLocalBusy.
      */
    virtual bool OnIncomingCall(
      OpalLocalConnection & connection ///<  Connection having event
    );

    /**Indicate alerting for the incoming connection.
       Returns false if the connection token does not correspond to a valid
       connection.
      */
    virtual bool AlertingIncomingCall(
      const PString & token, ///<  Token of connection to indicate alerting
      OpalConnection::StringOptions * options = NULL  ///< Optional string optiosn to apply
    );

    /**Accept the incoming connection.
       Returns false if the connection token does not correspond to a valid
       connection.
      */
    virtual bool AcceptIncomingCall(
      const PString & token, ///<  Token of connection to accept call
      OpalConnection::StringOptions * options = NULL  ///< Optional string optiosn to apply
    );

    /**Reject the incoming connection.
       Returns false if the connection token does not correspond to a valid
       connection.
      */
    virtual bool RejectIncomingCall(
      const PString & token,                 ///<  Token of connection to accept call
      const OpalConnection::CallEndReason & reason = OpalConnection::EndedByAnswerDenied ///<  Reason for rejecting the call
    );

    /**Call back to indicate that the remote user has indicated something.
       If false is returned the call is aborted.

       The default implementation does nothing.
      */
    virtual bool OnUserInput(
      const OpalLocalConnection & connection, ///<  Connection having event
      const PString & indication    ///< User input received
    );

    /**Call back to get media data for transmission.
       If false is returned then OnReadMediaData() is called.

       Care with the handling of real time is rqeuired, see GetSynchronicity
       for more details.

       The default implementation returns false.
      */
    virtual bool OnReadMediaFrame(
      const OpalLocalConnection & connection, ///<  Connection for media
      const OpalMediaStream & mediaStream,    ///<  Media stream data is required for
      RTP_DataFrame & frame                   ///<  RTP frame for data
    );

    /**Call back to handle received media data.
       If false is returned then OnWriteMediaData() is called.

       Care with the handling of real time is rqeuired, see GetSynchronicity
       for more details.

       The default implementation returns false.
      */
    virtual bool OnWriteMediaFrame(
      const OpalLocalConnection & connection, ///<  Connection for media
      const OpalMediaStream & mediaStream,    ///<  Media stream data is required for
      RTP_DataFrame & frame                   ///<  RTP frame for data
    );

    /**Call back to get media data for transmission.
       If false is returned the media stream will be closed.

       Care with the handling of real time is rqeuired, see GetSynchronicity
       for more details.

       The default implementation returns false.
      */
    virtual bool OnReadMediaData(
      const OpalLocalConnection & connection, ///<  Connection for media
      const OpalMediaStream & mediaStream,    ///<  Media stream data is required for
      void * data,                            ///<  Data to send
      PINDEX size,                            ///<  Maximum size of data buffer
      PINDEX & length                         ///<  Number of bytes placed in buffer
    );

    /**Call back to handle received media data.
       If false is returned the media stream will be closed.

       Note: For audio media, if \p data is NULL then that indicates there is
       no incoming audio available from the jitter buffer. The application
       should output silence for a time. The \p written value should still
       contain the bytes of silence emitted, even though it ewill be larger
       that \p length.
       
       Also, it is expected that this function be real time. That is if 320
       bytes of PCM-16 are written, this function should take 20ms to execute.
       If not then the jitter buffer will not operate correctly and audio will
       not be of high quality.

       Care with the handling of real time is rqeuired, see GetSynchronicity
       for more details.

       The default implementation returns false.
      */
    virtual bool OnWriteMediaData(
      const OpalLocalConnection & connection, ///<  Connection for media
      const OpalMediaStream & mediaStream,    ///<  Media stream data is required for
      const void * data,                      ///<  Data received
      PINDEX length,                          ///<  Amount of data available to write
      PINDEX & written                        ///<  Amount of data written
    );

    /**Indicate the synchronous mode for I/O.
       This indicates that the OnReadMediaXXX and OnWriteMediaXXX functions
       will execute blocking to the correct real time synchronisation.
       If GetSynchronicity() returns e_Synchronous, then, for example, when
       OnWriteMediaData() is sent 320 bytes of PCM data, it will block for
       20 milliseconds.

       If GetSynchronicity() returns e_SimulateSyncronous, then the system will try and
       simulate the correct timing using the operating system sleep function.
       This is not desirable as this function is notoriously inaccurate, and
       OPAL does it's best to compensate, but very often there is no other
       choice.

       Note, it is important for the correct oeprating of the jitter buffer
       that one of the above two modes is used for audio.

       If GetSynchronicity() returns e_Asynchronous, then the system will indicate
       that blocking is not required in any way. For example, when playing video,
       this is done as fast as data comes in from the network and there is no
       real time pacing required.
      */
    enum Synchronicity {
      e_Synchronous,        ///< Functions will block for correct real time
      e_Asynchronous,       ///< Functions will not block, and do not require any real time handling.
      e_SimulateSyncronous  ///< Functions wlll not block, but do require real time handling.
    };

    /**Indicate the I/O synchronous mode.
       See Synchronicity for more details.

       Default behaviour returns m_defaultAudioSynchronicity (initially
       e_Synchronous) when is audio source or sink,
       m_defaultVideoSourceSynchronicity (initially e_Synchronous) when a
       video source, e_Asynchronous in all other cases.
      */
    virtual Synchronicity GetSynchronicity(
      const OpalMediaFormat & mediaFormat,  ///< Media format for stream being opened.
      bool isSource                         ///< Stream is a a source
    ) const;

    /**Get default synchronous mode for audio sources and sinks.
      */
    Synchronicity GetDefaultAudioSynchronicity() const { return m_defaultAudioSynchronicity; }

    /**Set default synchronous mode for audio sources and sinks.
      */
    void SetDefaultAudioSynchronicity(Synchronicity sync) { m_defaultAudioSynchronicity = sync; }

    /**Get default synchronous mode for video sources.
      */
    Synchronicity GetDefaultVideoSourceSynchronicity() const { return m_defaultVideoSourceSynchronicity; }

    /**Set default synchronous mode for video sources.
      */
    void SetDefaultVideoSourceSynchronicity(Synchronicity sync) { m_defaultVideoSourceSynchronicity = sync; }

    /**Indicate OnAlerting() is be deferred or immediate.
      */
    bool IsDeferredAlerting() const { return m_deferredAlerting; }

    /**Indicate OnAlerting() is be deferred or immediate.
      */
    void SetDeferredAlerting(bool defer) { m_deferredAlerting = defer; }

    /**Indicate AcceptIncomingCall() execution is be deferred or immediate on OnIncomingCall().
      */
    bool IsDeferredAnswer() const { return m_deferredAnswer; }

    /**Indicate AcceptIncomingCall() execution is be deferred or immediate on OnIncomingCall().
      */
    void SetDeferredAnswer(bool defer) { m_deferredAnswer = defer; }
  //@}

  protected:
    bool m_deferredAlerting;
    bool m_deferredAnswer;

    Synchronicity m_defaultAudioSynchronicity;
    Synchronicity m_defaultVideoSourceSynchronicity;

  private:
    P_REMOVE_VIRTUAL(OpalLocalConnection *, CreateConnection(OpalCall &, void *), 0);
    P_REMOVE_VIRTUAL(bool, IsSynchronous() const, false);
};


/** Local connection.
    This class represents a connection on the local machine that can receive
    media via virtual functions.
 */
class OpalLocalConnection : public OpalConnection
{
    PCLASSINFO(OpalLocalConnection, OpalConnection);
  public:
  /**@name Construction */
  //@{
    /**Create a new connection.
     */
    OpalLocalConnection(
      OpalCall & call,              ///<  Owner call for connection
      OpalLocalEndPoint & endpoint, ///<  Owner endpoint for connection
      void * userData,              ///<  Arbitrary data to pass to connection
      unsigned options,             ///< Option bit mask to pass to connection
      OpalConnection::StringOptions * stringOptions, ///< Options to pass to connection
      char tokenPrefix = 'L'        ///< Prefix for token generation
    );

    /**Destroy connection.
     */
    ~OpalLocalConnection();
  //@}

  /**@name Overrides from OpalConnection */
  //@{
    /**Call back for an incoming call.
       This function is used for an application to control the answering of
       incoming calls.

       If true is returned then the connection continues. If false then the
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

       The default behaviour calls the base class then OnOutgoingSetUp().

       Note that the most explicit version of this override is made pure, so as to force 
       descendant classes to implement it. This will only affect code that implements new
       descendants of OpalConnection - code that uses existing descendants will be unaffected
     */
    virtual PBoolean OnIncomingConnection(unsigned int options, OpalConnection::StringOptions * stringOptions);

    /**Get indication of connection being to a "network".
       This indicates the if the connection may be regarded as a "network"
       connection. The distinction is about if there is a concept of a "remote"
       party being connected to and is best described by example: sip, h323,
       iax and pstn are all "network" connections as they connect to something
       "remote". While pc, pots and ivr are not as the entity being connected
       to is intrinsically local.
      */
    virtual PBoolean IsNetworkConnection() const { return false; }

    /// Call back for connection to act on changed string options
    virtual void OnApplyStringOptions();

    /**Start an outgoing connection.
       This function will initiate the connection to the remote entity, for
       example in H.323 it sends a SETUP, in SIP it sends an INVITE etc.

       The default behaviour determines if this si incoming or outgoing call
       by checking if we are party A or B, then does approriate setting up
       of the conenction, including calling OnOutgoing() or OnIncoming() as
       appropriate.
      */
    virtual PBoolean SetUpConnection();

    /**Indicate to remote endpoint an alert is in progress.
       If this is an incoming connection and the AnswerCallResponse is in a
       AnswerCallDeferred or AnswerCallPending state, then this function is
       used to indicate to that endpoint that an alert is in progress. This is
       usually due to another connection which is in the call (the B party)
       has received an OnAlerting() indicating that its remote endpoint is
       "ringing".

       The default behaviour does nothing.
      */
    virtual PBoolean SetAlerting(
      const PString & calleeName,   ///<  Name of endpoint being alerted.
      PBoolean withMedia            ///<  Open media with alerting
    );

    /**Indicate to remote endpoint we are connected.

       The default behaviour sets the phase to ConnectedPhase, sets the
       connection start time and then checks if there is any media channels
       opened and if so, moves on to the established phase, calling
       OnEstablished().

       In other words, this method is used to handle incoming calls,
       and is an indication that we have accepted the incoming call.
      */
    virtual PBoolean SetConnected();

    /**Open a new media stream.
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
      const OpalMediaFormat & mediaFormat, ///<  Media format for stream
      unsigned sessionID,                  ///<  Session number for stream
      PBoolean isSource                    ///<  Is a source stream
    );

    /**Open source or sink media stream for session.
      */
    virtual OpalMediaStreamPtr OpenMediaStream(
      const OpalMediaFormat & mediaFormat, ///<  Media format to open
      unsigned sessionID,                  ///<  Session to start stream on
      bool isSource                        ///< Stream is a source/sink
    );

    /**Send a user input indication to the remote endpoint.
       This sends an arbitrary string as a user indication. If DTMF tones in
       particular are required to be sent then the SendIndicationTone()
       function should be used.

       The default behaviour plays the DTMF tones on the line.
      */
    virtual PBoolean SendUserInputString(
      const PString & value                   ///<  String value of indication
    );
  //@}

  /**@name New operations */
  //@{
    /**Call back just before remote is contacted.

       The default implementation call OpalLocalEndPoint::OnOutgoingSetUp().

       @return false if the call is to be aborted with EndedByNoAccept.
      */
    virtual bool OnOutgoingSetUp();

    /**Call back to indicate that remote is ringing.

       The default implementation call OpalLocalEndPoint::OnOutgoingCall().

       @return false if the call is to be aborted with EndedByNoAccept.
      */
    virtual bool OnOutgoing();

    /**Call back to indicate that there is an incoming call.
       Note this function should not block or it will impede the operation of
       the stack.

       The default implementation call OpalLocalEndPoint::OnIncomingCall().

       @return false if the call is to be aborted with status of EndedByLocalBusy.
      */
    virtual bool OnIncoming();

    /**Indicate alerting for the incoming connection.
      */
    virtual void AlertingIncoming();

    /**Accept the incoming connection.
      */
    virtual void AcceptIncoming();
  //@}

  /**@name Member variable access */
  //@{
    /// Get user data pointer.
    void * GetUserData() const  { return m_userData; }

    /// Set user data pointer.
    void SetUserData(void * v)  { m_userData = v; }
  //@}

  protected:
    friend class PSafeWorkNoArg<OpalLocalConnection>;
    void InternalAcceptIncoming();

    OpalLocalEndPoint & endpoint;
    void * m_userData;
};


/**Local media stream.
    This class represents a media stream on the local machine that can receive
    media via virtual functions.
  */
class OpalLocalMediaStream : public OpalMediaStream, public OpalMediaStreamPacing
{
    PCLASSINFO(OpalLocalMediaStream, OpalMediaStream);
  public:
  /**@name Construction */
  //@{
    /**Construct a new media stream for local system.
      */
    OpalLocalMediaStream(
      OpalLocalConnection & conn,          ///<  Connection for media stream
      const OpalMediaFormat & mediaFormat, ///<  Media format for stream
      unsigned sessionID,                  ///<  Session number for stream
      bool isSource,                       ///<  Is a source stream
      OpalLocalEndPoint::Synchronicity synchronicity ///< Synchronous mode
    );
  //@}

  /**@name Overrides of OpalMediaStream class */
  //@{
    /**Read an RTP frame of data from the source media stream.
       The default behaviour simply calls ReadData() on the data portion of the
       RTP_DataFrame and sets the frames timestamp and marker from the internal
       member variables of the media stream class.
      */
    virtual PBoolean ReadPacket(
      RTP_DataFrame & packet
    );

    /**Write an RTP frame of data to the sink media stream.
       The default behaviour simply calls WriteData() on the data portion of the
       RTP_DataFrame and and sets the internal timestamp and marker from the
       member variables of the media stream class.
      */
    virtual PBoolean WritePacket(
      RTP_DataFrame & packet
    );

    /**Read raw media data from the source media stream.
       The default behaviour reads from the OpalLine object.
      */
    virtual PBoolean ReadData(
      BYTE * data,      ///<  Data buffer to read to
      PINDEX size,      ///<  Size of buffer
      PINDEX & length   ///<  Length of data actually read
    );

    /**Write raw media data to the sink media stream.
       The default behaviour writes to the OpalLine object.
      */
    virtual PBoolean WriteData(
      const BYTE * data,   ///<  Data to write
      PINDEX length,       ///<  Length of data to read.
      PINDEX & written     ///<  Length of data actually written
    );

    /**Indicate if the media stream is synchronous.
       Returns true for LID streams.
      */
    virtual PBoolean IsSynchronous() const;
  //@}

  protected:
    virtual void InternalClose() { }

    OpalLocalEndPoint::Synchronicity m_synchronicity;
};


#endif // OPAL_OPAL_LOCALEP_H


// End of File ///////////////////////////////////////////////////////////////
