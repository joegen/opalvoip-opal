/*
 * lyncep.h
 *
 * Header file for Lync (UCMA) interface
 *
 * Copyright (C) 2016 Vox Lucida Pty. Ltd.
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
 * The Original Code is Portable Windows Library.
 *
 * The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * Contributor(s): ______________________________________.
 */

#ifndef OPAL_EP_LYNCEP_H
#define OPAL_EP_LYNCEP_H

#include <opal_config.h>

#if OPAL_LYNC

#include <string>

/* Class to hide Lync UCMA Managed code
   This odd arrangement is so when compiling the shim and we include this header
   file, we do not get all the PTLib stuff which interferes with the managed
   code interfacing.
 */
class OpalLyncShim
{
  public:
    OpalLyncShim();
    ~OpalLyncShim();

    struct Platform;
    Platform * CreatePlatform(const char * userAgent);
    bool DestroyPlatform(Platform * platform);

    struct AppEndpoint;
    AppEndpoint * CreateAppEndpoint(Platform & platform);
    void DestroyAppEndpoint(AppEndpoint * app);

    struct UserEndpoint;
    UserEndpoint * CreateUserEndpoint(Platform & platform,
                                      const char * uri,
                                      const char * password,
                                      const char * authID,
                                      const char * domain);
    void DestroyUserEndpoint(UserEndpoint * user);

    struct Conversation;
    Conversation * CreateConversation(UserEndpoint & uep);
    void DestroyConversation(Conversation * conv);

    struct AudioVideoCall;
    AudioVideoCall * CreateAudioVideoCall(Conversation & conv, const char * uri, bool answering);
    void DestroyAudioVideoCall(AudioVideoCall * call);

    struct AudioVideoFlow;
    AudioVideoFlow * CreateAudioVideoFlow(AudioVideoCall & call);
    void DestroyAudioVideoFlow(AudioVideoFlow * flow);

    struct SpeechRecognitionConnector;
    SpeechRecognitionConnector * CreateSpeechRecognitionConnector(AudioVideoFlow & flow);
    void DestroySpeechRecognitionConnector(SpeechRecognitionConnector * connector);

    struct SpeechSynthesisConnector;
    SpeechSynthesisConnector * CreateSpeechSynthesisConnector(AudioVideoFlow & flow);
    void DestroySpeechSynthesisConnector(SpeechSynthesisConnector * connector);

    struct AudioVideoStream;
    AudioVideoStream * CreateAudioVideoStream(SpeechRecognitionConnector & connector);
    AudioVideoStream * CreateAudioVideoStream(SpeechSynthesisConnector & connector);
    int ReadAudioVideoStream(AudioVideoStream & stream, unsigned char * data, int size);
    int WriteAudioVideoStream(AudioVideoStream & stream, const unsigned char * data, int length);
    void DestroyAudioVideoStream(AudioVideoStream * stream);

    static int const CallStateEstablishing;
    static int const CallStateEstablished;
    static int const CallStateTerminating;
    static int const MediaFlowActive;

    virtual void OnIncomingLyncCall(AudioVideoCall * /*call*/) { }
    virtual void OnLyncCallStateChanged(int /*previousState*/, int /*newState*/) { }
    virtual void OnLyncCallFailed(const std::string & /*error*/) { }
    virtual void OnMediaFlowStateChanged(int /*previousState*/, int /*newState*/) { }

    const std::string & GetLastError() const { return m_lastError; }

#if PTRACING
    virtual void OnTraceOutput(unsigned level, const char * file, unsigned line, const std::string & out) = 0;
#endif

  private:
    struct Callbacks;
    Callbacks * m_callbacks;
    std::string m_lastError;
};


#ifndef OPAL_LYNC_SHIM

#include <opal/endpoint.h>
#include <opal/connection.h>


//////////////////////////////////////////////////////////////////

class OpalLyncConnection;

#if PTRACING
  class OpalLyncShimBase : public OpalLyncShim
  {
    virtual void OnTraceOutput(unsigned level, const char * file, unsigned line, const std::string & out) override;
  };
#else
  typedef OpalLyncShim OpalLyncShimBase;
#endif


/**Endpoint for interfacing Microsoft Lync via UCMA.
 */
class OpalLyncEndPoint : public OpalEndPoint, public OpalLyncShimBase
{
    PCLASSINFO(OpalLyncEndPoint, OpalEndPoint);
  public:
    /**@name Construction */
    //@{
    /**Create a new endpoint.
     */
    OpalLyncEndPoint(
      OpalManager & manager,
      const char *prefix = "lync"
    );

    /**Destroy endpoint.
     */
    virtual ~OpalLyncEndPoint();
    //@}

    /**@name Overrides from OpalEndPoint */
    //@{
    /** Shut down the endpoint, this is called by the OpalManager just before
        destroying the object and can be handy to make sure some things are
        stopped before the vtable gets clobbered.
        */
    virtual void ShutDown();

    /**Get the data formats this endpoint is capable of operating.
       This provides a list of media data format names that may be used by an
       OpalMediaStream may be created by a connection from this endpoint.

       Note that a specific connection may not actually support all of the
       media formats returned here, but should return no more.
       */
    virtual OpalMediaFormatList GetMediaFormats() const;

    /** Set up a connection to a remote party.
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
        */
    virtual PSafePtr<OpalConnection> MakeConnection(
      OpalCall & call,                         ///<  Owner of connection
      const PString & party,                   ///<  Remote party to call
      void * userData,                         ///<  Arbitrary data to pass to connection
      unsigned int options,                    ///<  options to pass to conneciton
      OpalConnection::StringOptions * stringOptions  ///<  complex string options
    );

    /** Execute garbage collection for endpoint.
        Returns true if all garbage has been collected.
        */
    virtual PBoolean GarbageCollection();
    //@}

    /**@name User registrations */
    //@{
    struct RegistrationInfo
    {
      PString m_uri;
      PString m_authID;
      PString m_password;
      PString m_domain;
    };

    /// Register URI as a local user with Lync server
    bool Register(
      const RegistrationInfo & info
    );

    /// Unregister URI as a local user with Lync server
    bool Unregister(
      const PString & uri
    );

    /// Get registered URI with Lync server
    UserEndpoint * GetRegistration(
      const PString & uri
    );
    //@}

    /**@name Customisation call backs */
    //@{
    /** Create a connection for the skinny endpoint.
      */
    virtual OpalLyncConnection * CreateConnection(
      OpalCall & call,     ///< Owner of connection
      AudioVideoCall * avCall,
      const PString & uri, ///< Number to dial out, empty if incoming
      void * userData,     ///< Arbitrary data to pass to connection
      unsigned int options,
      OpalConnection::StringOptions * stringOptions
    );
    //@}

  protected:
    virtual void OnIncomingLyncCall(AudioVideoCall * call) override;

    Platform * m_platform;

    typedef std::map<PString, UserEndpoint *> RegistrationMap;
    RegistrationMap m_registrations;
    PMutex          m_registrationMutex;
};


/**Connection for interfacing Microsoft Lync via UCMA.
  */
class OpalLyncConnection : public OpalConnection, public OpalLyncShimBase
{
    PCLASSINFO(OpalLyncConnection, OpalConnection);
  public:
  /**@name Construction */
  //@{
    /**Create a new connection.
     */
    OpalLyncConnection(
      OpalCall & call,
      OpalLyncEndPoint & ep,
      AudioVideoCall * avCall,
      const PString & uri,
      void * userData,
      unsigned options,
      OpalConnection::StringOptions * stringOptions
    );
  //@}

  /**@name Overrides from OpalConnection */
  //@{
    /** Get indication of connection being to a "network".
        This indicates the if the connection may be regarded as a "network"
        connection. The distinction is about if there is a concept of a "remote"
        party being connected to and is best described by example: sip, h323,
        iax and pstn are all "network" connections as they connect to something
        "remote". While pc, pots and ivr are not as the entity being connected
        to is intrinsically local.
    */
    virtual bool IsNetworkConnection() const { return true; }

    /** Start an outgoing connection.
        This function will initiate the connection to the remote entity, for
        example in H.323 it sends a SETUP, in SIP it sends an INVITE etc.
    */
    virtual PBoolean SetUpConnection();

    /** Clean up the termination of the connection.
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
      */
    virtual void OnReleased();

    /**Get the data formats this connection is capable of operating.
       This provides a list of media data format names that a
       OpalMediaStream may be created in within this connection.
      */
    virtual OpalMediaFormatList GetMediaFormats() const;

    /**Indicate to remote endpoint an alert is in progress.
       If this is an incoming connection and the AnswerCallResponse is in a
       AnswerCallDeferred or AnswerCallPending state, then this function is
       used to indicate to that endpoint that an alert is in progress. This is
       usually due to another connection which is in the call (the B party)
       has received an OnAlerting() indicating that its remoteendpoint is
       "ringing".
      */
    virtual PBoolean SetAlerting(
      const PString & calleeName,   ///<  Name of endpoint being alerted.
      PBoolean withMedia                ///<  Open media with alerting
    );

    /**Indicate to remote endpoint we are connected.
      */
    virtual PBoolean SetConnected();

    /** Get the remote transport address
      */
    virtual OpalTransportAddress GetRemoteAddress() const;

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
  //@}

    AudioVideoFlow * GetAudioVideoFlow() const { return m_flow; }

  protected:
    OpalLyncEndPoint & m_endpoint;

    Conversation   * m_conversation;
    AudioVideoCall * m_audioVideoCall;
    AudioVideoFlow * m_flow;

    virtual void OnLyncCallStateChanged(int previousState, int newState) override;
    virtual void OnLyncCallFailed(const std::string & error) override;
    virtual void OnMediaFlowStateChanged(int previousState, int newState) override;
};


/**This class describes a media stream that transfers data to/from Microsoft Lync via UCMA.
  */
class OpalLyncMediaStream : public OpalMediaStream, public OpalLyncShimBase
{
    PCLASSINFO(OpalLyncMediaStream, OpalMediaStream);
  public:
  /**@name Construction */
  //@{
    /**Construct a new media stream for Line Interface Devices.
      */
    OpalLyncMediaStream(
      OpalLyncConnection & conn,           ///< Owner connection
      const OpalMediaFormat & mediaFormat, ///<  Media format for stream
      unsigned sessionID,                  ///<  Session number for stream
      bool isSource                        ///<  Is a source stream
    );
  //@}

    ~OpalLyncMediaStream();


  /**@name Overrides of OpalMediaStream class */
  //@{
    /**Open the media stream.

       The default behaviour sets the OpalLineInterfaceDevice format and
       calls Resume() on the associated OpalMediaPatch thread.
      */
    virtual PBoolean Open();

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
    virtual void InternalClose();

    OpalLyncConnection         & m_connection;
    SpeechRecognitionConnector * m_inputConnector;
    SpeechSynthesisConnector   * m_outputConnector;
    AudioVideoStream           * m_avStream;
};


#endif // OPAL_LYNC_INTERNAL

#endif // OPAL_LYNC

#endif // OPAL_EP_LYNCEP_H


// End of File ///////////////////////////////////////////////////////////////
