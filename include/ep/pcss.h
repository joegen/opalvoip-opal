/*
 * pcss.h
 *
 * PC Sound System support.
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
 */

#ifndef OPAL_OPAL_PCSS_H
#define OPAL_OPAL_PCSS_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif


#include <opal_config.h>

#if OPAL_HAS_PCSS

#include <ptlib/sound.h>
#include <ep/localep.h>


class OpalPCSSConnection;

#define OPAL_PCSS_PREFIX "pc"


/** PC Sound System endpoint.
 */
class OpalPCSSEndPoint : public OpalLocalEndPoint
{
    PCLASSINFO(OpalPCSSEndPoint, OpalLocalEndPoint);
  public:
  /**@name Construction */
  //@{
    /**Create a new endpoint.
     */
    OpalPCSSEndPoint(
      OpalManager & manager,  ///<  Manager of all endpoints.
      const char * prefix = OPAL_PCSS_PREFIX ///<  Prefix for URL style address strings
    );

    /**Destroy endpoint.
     */
    ~OpalPCSSEndPoint();
  //@}

  /**@name Overrides from OpalEndPoint */
  //@{
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
      unsigned options = 0,                    ///< Option bit map to be passed to connection
      OpalConnection::StringOptions * stringOptions = NULL ///< Options to be passed to connection
    );
  //@}

  /**@name Overrides from OpalLocalEndPoint */
  //@{
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

    /**Call back to indicate that the remote user has indicated something.
       If false is returned the call is aborted.

       The default implementation does nothing.
      */
    virtual bool OnUserInput(
      const OpalLocalConnection & connection, ///<  Connection having event
      const PString & indication              ///<  Received user input indications
    );

#if OPAL_VIDEO
    /**Create an PVideoInputDevice for a source media stream.
      */
    virtual bool CreateVideoInputDevice(
      const OpalConnection & connection,    ///<  Connection needing created video device
      const OpalMediaFormat & mediaFormat,  ///<  Media format for stream
      PVideoInputDevice * & device,         ///<  Created device
      bool & autoDelete                     ///<  Flag for auto delete device
    );
#endif
  //@}

  /**@name Customisation call backs */
  //@{
    /**Create a connection for the PCSS endpoint.
       The default implementation is to create a OpalPCSSConnection.
      */
    virtual OpalPCSSConnection * CreateConnection(
      OpalCall & call,    ///<  Owner of connection
      const PString & playDevice, ///<  Sound channel play device
      const PString & recordDevice, ///<  Sound channel record device
      void * userData,    ///<  Arbitrary data to pass to connection
      unsigned options,                    ///< Option bit map to be passed to connection
      OpalConnection::StringOptions * stringOptions ///< Options to be passed to connection
    );

    /**Create an PSoundChannel based media stream.
      */
    virtual PSoundChannel * CreateSoundChannel(
      const OpalPCSSConnection & connection, ///<  Connection needing created sound channel
      const OpalMediaFormat & mediaFormat,   ///<  Media format for the connection
      unsigned sessionID,                    ///<  Session number for stream
      bool isSource                          ///<  Direction for channel
    );

    /**Create an PSoundChannel based media stream.
      */
    virtual PSoundChannel * CreateSoundChannel(
      const OpalPCSSConnection & connection, ///<  Connection needing created sound channel
      const OpalMediaFormat & mediaFormat,   ///<  Media format for the connection
      unsigned sessionID,                    ///<  Session number for stream
      bool isSource,                         ///<  Direction for channel
      const PString & device                 ///<  Name of audio device to create
    );
  //@}

  /**@name User Interface operations */
  //@{
    /**Find a connection that uses the specified token.
       This searches the endpoint for the connection that contains the token
       as provided by functions such as MakeConnection(). If not then it
       attempts to use the token as a OpalCall token and find a connection
       of the same class.
      */
    PSafePtr<OpalPCSSConnection> GetPCSSConnectionWithLock(
      const PString & token,     ///<  Token to identify connection
      PSafetyMode mode = PSafeReadWrite ///< Lock mode
    ) { return GetConnectionWithLockAs<OpalPCSSConnection>(token, mode); }

    /**Call back to indicate that there is an incoming call.
       Note this function should not block or it will impede the operation of
       the stack.

       The default implementation calls OpalPCSSEndPoint::OnIncomingCall().

       @return false is returned the call is aborted with status of EndedByLocalBusy.
      */
    virtual PBoolean OnShowIncoming(
      const OpalPCSSConnection & connection ///<  Connection having event
    );

    /**Accept the incoming connection.
       Returns false if the connection token does not correspond to a valid
       connection.
      */
    virtual PBoolean AcceptIncomingConnection(
      const PString & connectionToken ///<  Token of connection to accept call
    );

    /**Reject the incoming connection.
       Returns false if the connection token does not correspond to a valid
       connection.
      */
    virtual PBoolean RejectIncomingConnection(
      const PString & connectionToken,       ///<  Token of connection to accept call
      const OpalConnection::CallEndReason & reason = OpalConnection::EndedByAnswerDenied ///<  Reason for rejecting the call
    );

    /**Call back to indicate that remote is ringing.
       If false is returned the call is aborted.

       The default implementation is pure.
      */
    virtual PBoolean OnShowOutgoing(
      const OpalPCSSConnection & connection ///<  Connection having event
    );

    /**Call back to indicate that the remote user has indicated something.
       If false is returned the call is aborted.

       The default implementation does nothing.
      */
    virtual PBoolean OnShowUserInput(
      const OpalPCSSConnection & connection, ///<  Connection having event
      const PString & indication             ///<  Received user input indications
    );
  //@}

  /**@name Member variable access */
  //@{
    /**Set the local ringback tone to be played when remote indicates it is
       ringing.

       This string may be one of several forms:
          A filename for an existing WAV file.
          A two letter country code, or the full country name.
          A tone specification as per PTones class.
      */
    bool SetLocalRingbackTone(
      const PString & tone    ///< Tone specification string
    );

    /**Get the local ringback tone to be played when remote indicates it is
       ringing.
      */
    const PString & GetLocalRingbackTone() const { return m_localRingbackTone; }

    /**Set the name for the sound channel to be used for output.
       If the name is not suitable for use with the PSoundChannel class then
       the function will return false and not change the device.

       This defaults to the value of the PSoundChannel::GetDefaultDevice()
       function.
     */
    virtual PBoolean SetSoundChannelPlayDevice(const PString & name);

    /**Get the name for the sound channel to be used for output.
       This defaults to the value of the PSoundChannel::GetDefaultDevice()
       function.
     */
    const PString & GetSoundChannelPlayDevice() const { return m_soundChannelPlayDevice; }

    /**Set the name for the sound channel to be used for input.
       If the name is not suitable for use with the PSoundChannel class then
       the function will return false and not change the device.

       This defaults to the value of the PSoundChannel::GetDefaultDevice()
       function.
     */
    virtual PBoolean SetSoundChannelRecordDevice(const PString & name);

    /**Get the name for the sound channel to be used for input.
       This defaults to the value of the PSoundChannel::GetDefaultDevice()
       function.
     */
    const PString & GetSoundChannelRecordDevice() const { return m_soundChannelRecordDevice; }

    /**Set the name for the sound channel to be used for input when on hold.
       If the name is not suitable for use with the PSoundChannel class then
       the function will return false and not change the device.

       This defaults to the "Null Audio".
     */
    virtual bool SetSoundChannelOnHoldDevice(const PString & name);

    /**Get the name for the sound channel to be used for input when on hold.
       This defaults to the "Null Audio".
     */
    const PString & GetSoundChannelOnHoldDevice() const { return m_soundChannelOnHoldDevice; }

    /**Set the name for the sound channel to be used for input when ringing.

       This defaults to an empty string which does not send video on ringing.
     */
    virtual bool SetSoundChannelOnRingDevice(const PString & name);

    /**Get the name for the sound channel to be used for input when on hold.
       This defaults to an empty string which does not send video on ringing.
     */
    const PString & GetSoundChannelOnRingDevice() const { return m_soundChannelOnRingDevice; }

#if OPAL_VIDEO
    /**Set the name for the video device to be used for sent video.
     */
    virtual bool SetVideoGrabberDevice(const PVideoDevice::OpenArgs & args);

    /**Get the name for the video device to be used for sent video.
     */
    const PVideoDevice::OpenArgs & GetVideoGrabberDevice() const { return GetManager().GetVideoInputDevice(); }

    /**Set the name for the video device to be used for displaying sent video.
     */
    virtual bool SetVideoPreviewDevice(const PVideoDevice::OpenArgs & args);

    /**Get the name for the video device to be used for displaying sent video.
     */
    const PVideoDevice::OpenArgs & GetVideoPreviewDevice() const { return GetManager().GetVideoPreviewDevice(); }

    /**Set the name for the video device to be used for displaying received video.
     */
    virtual bool SetVideoDisplayDevice(const PVideoDevice::OpenArgs & args);

    /**Get the name for the video device to be used for displaying received video.
     */
    const PVideoDevice::OpenArgs & GetVideoDisplayDevice() const { return GetManager().GetVideoOutputDevice(); }

    /**Set the name for the video device to be used for input when on hold.
       This defaults to the "Null Video Out".
     */
    virtual bool SetVideoOnHoldDevice(const PVideoDevice::OpenArgs & args);

    /**Get the name for the video device to be used for input when on hold.
       This defaults to the "Null Video Out".
     */
    const PVideoDevice::OpenArgs & GetVideoOnHoldDevice() const { return m_videoOnHoldDevice; }

    /**Set the name for the video device to be used for input when ringing.
       This defaults to an empty string which does not send video on ringing.
     */
    virtual bool SetVideoOnRingDevice(const PVideoDevice::OpenArgs & args);

    /**Get the name for the video device to be used for input when ringing.
       This defaults to an empty string which does not send video on ringing.
     */
    const PVideoDevice::OpenArgs & GetVideoOnRingDevice() const { return m_videoOnRingDevice; }
#endif

    /**Get default the sound channel buffer depth.
       Note the largest of the depth in frames and the depth in milliseconds
       as returned by GetSoundBufferTime() is used.
      */
    unsigned GetSoundChannelBufferDepth() const { return m_soundChannelBuffers; }

    /**Set the default sound channel buffer depth.
       Note the largest of the depth in frames and the depth in milliseconds
       as returned by GetSoundBufferTime() is used.
      */
    void SetSoundChannelBufferDepth(
      unsigned depth    ///<  New depth
    );

    /**Get default the sound channel buffer time in milliseconds.
       Note the largest of the depth in frames and the depth in milliseconds
       as returned by GetSoundBufferTime() is used.
      */
    unsigned GetSoundChannelBufferTime() const { return m_soundChannelBufferTime; }

    /**Set the default sound channel buffer time in milliseconds.
       Note the largest of the depth in frames and the depth in milliseconds
       as returned by GetSoundBufferTime() is used.
      */
    void SetSoundChannelBufferTime(
      unsigned depth    ///<  New depth in milliseconds
    );
  //@}

  protected:
    PString  m_localRingbackTone;
    PString  m_soundChannelPlayDevice;
    PString  m_soundChannelRecordDevice;
    PString  m_soundChannelOnHoldDevice;
    PString  m_soundChannelOnRingDevice;
    unsigned m_soundChannelBuffers;
    unsigned m_soundChannelBufferTime;
#if OPAL_VIDEO
    PVideoDevice::OpenArgs m_videoOnHoldDevice;
    PVideoDevice::OpenArgs m_videoOnRingDevice;
#endif

  private:
    P_REMOVE_VIRTUAL(OpalPCSSConnection *, CreateConnection(OpalCall &, const PString &, const PString &, void *), 0)
    P_REMOVE_VIRTUAL(PSoundChannel *,CreateSoundChannel(const OpalPCSSConnection&,const OpalMediaFormat&,PBoolean),NULL);
    P_REMOVE_VIRTUAL(PSoundChannel *,CreateSoundChannel(const OpalPCSSConnection&,const OpalMediaFormat&,const PString&,bool),NULL);
};


/** PC Sound System connection.
 */
class OpalPCSSConnection : public OpalLocalConnection
{
    PCLASSINFO(OpalPCSSConnection, OpalLocalConnection);
  public:
  /**@name Construction */
  //@{
    /**Create a new endpoint.
     */
    OpalPCSSConnection(
      OpalCall & call,              ///<  Owner calll for connection
      OpalPCSSEndPoint & endpoint,  ///<  Owner endpoint for connection
      const PString & playDevice,   ///<  Sound channel play device
      const PString & recordDevice,  ///<  Sound channel record device
      unsigned options = 0,                    ///< Option bit map to be passed to connection
      OpalConnection::StringOptions * stringOptions = NULL ///< Options to be passed to connection
    );

    /**Destroy endpoint.
     */
    ~OpalPCSSConnection();
  //@}

  /**@name Overrides from OpalConnection */
  //@{
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

    /// Call back for connection to act on changed string options
    virtual void OnApplyStringOptions();

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

    /**Initiate the transfer of an existing call (connection) to a new remote 
       party.

       If remoteParty is a valid call token, then the remote party is transferred
       to that party (consultation transfer) and both calls are cleared.
     */
    virtual bool TransferConnection(
      const PString & remoteParty   ///<  Remote party to transfer the existing call to
    );

    /**Call back indicating result of last hold/retrieve operation.
       This also indicates if the local connection has been put on hold by the
       remote connection.
     */
    virtual void OnHold(
      bool fromRemote,               ///<  Indicates remote has held local connection
      bool onHold                    ///<  Indicates have just been held/retrieved.
    );

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
      PBoolean isSource                        ///<  Is a source stream
    );

    /**Call back when media stream patch thread starts.

       Default behaviour calls OpalManager function of same name.
      */
    virtual void OnStartMediaPatch(
      OpalMediaPatch & patch    ///< Patch being started
    );

    /**Set  the volume (gain) for the audio media channel.
       The volume range is 0 == muted, 100 == LOUDEST.
      */
    virtual PBoolean SetAudioVolume(
      PBoolean source,        ///< true for source (microphone), false for sink (speaker)
      unsigned percentage     ///< Gain, 0=silent, 100=maximun
    );

    /**Get  the volume (gain) for the audio media channel.
       The volume range is 0 == muted, 100 == LOUDEST.
      */
    virtual PBoolean GetAudioVolume(
      PBoolean source,        ///< true for source (microphone), false for sink (speaker)
      unsigned & percentage   ///< Gain, 0=silent, 100=maximun
    );

    /**Set the mute state for the audio media channel.
      */
    virtual bool SetAudioMute(
      bool source,        ///< true for source (microphone), false for sink (speaker)
      bool mute           ///< Flag for muted audio
    );

    /**Get the mute state for the audio media channel.
      */
    virtual bool GetAudioMute(
      bool source,        ///< true for source (microphone), false for sink (speaker)
      bool & mute         ///< Flag for muted audio
    );

    /**Get the signal level in dBov (-127 to 0) for the audio media channel.
       A return value of INT_MAX indicates no valid signal, eg no audio channel opened.
      */
    virtual int GetAudioLevelDB(
      bool source   ///< true for source (microphone), false for sink (speaker)
    );

    /**Indicate alerting for the incoming connection.
       If GetSoundChannelOnRingDevice() is non-empty, then the \p withMedia
       is overridden to true.
      */
    virtual void AlertingIncoming(
      bool withMedia = false  ///< Indicate media should be started
    );

    /**Accept the incoming connection.
      */
    virtual void AcceptIncoming();
  //@}

  /**@name New operations */
  //@{
    /**Create an PSoundChannel based media stream.
      */
    virtual PSoundChannel * CreateSoundChannel(
      const OpalMediaFormat & mediaFormat, ///<  Media format for the connection
      unsigned sessionID,                  ///<  Session number for stream
      bool isSource                        ///<  Direction for channel
    );

    /**Change a PVideoInputDevice for a source media stream.
      */
    virtual bool ChangeSoundChannel(
      const PString & device,   ///< Device to change to
      bool isSource,            ///< Change source (recorder) or sink (player)
      unsigned sessionID = 0    ///< Session for media stream, 0 indicates all audio streams
    );
    virtual bool ChangeSoundChannel(
      const PString & device,       ///< Device to change to
      OpalAudioMediaStream * stream ///< Audio stream to change
    );

    /**Read characters from the channel and produce User Input Indications.
       Note that this will start a background thread to read the characters, so
       the lifetime of the connection.
      */
    bool StartReadUserInput(
      PChannel * channel,
      bool autoDelete = true
    );

    /**Stop the background thread reading user input.
       See StartReadUserInput().
      */
    void StopReadUserInput();
  //@}

  /**@name Member variable access */
  //@{
    /**Get the local ringback tone to be played when remote indicates it is
       ringing.
      */
    const PString & GetLocalRingbackTone() const { return m_localRingbackTone; }

    /**Get the name for the sound channel to be used for output.
       This defaults to the value of the PSoundChannel::GetDefaultDevice()
       function.
     */
    const PString & GetSoundChannelPlayDevice() const { return m_soundChannelPlayDevice; }

    /**Get the name for the sound channel to be used for input.
       This defaults to the value of the PSoundChannel::GetDefaultDevice()
       function.
     */
    const PString & GetSoundChannelRecordDevice() const { return m_soundChannelRecordDevice; }

    /**Get the name for the sound channel to be used for input when on hold.
       This defaults to the "Null Audio".
     */
    const PString & GetSoundChannelOnHoldDevice() const { return m_soundChannelOnHoldDevice; }

    /**Get the name for the sound channel to be used for input when on hold.
       This defaults to an empty string which does not send video on ringing.
     */
    const PString & GetSoundChannelOnRingDevice() const { return m_soundChannelOnRingDevice; }

#if OPAL_VIDEO
    /**Get the name for the video device to be used for input when on hold.
       This defaults to the "Null Video Out".
     */
    const PVideoDevice::OpenArgs & GetVideoOnHoldDevice() const { return m_videoOnHoldDevice; }

    /**Get the name for the video device to be used for input when ringing.
       This defaults to an empty string which does not send video on ringing.
     */
    const PVideoDevice::OpenArgs & GetVideoOnRingDevice() const { return m_videoOnRingDevice; }
#endif

    /**Get default the sound channel buffer depth.
       Note the largest of the depth in frames and the depth in milliseconds
       as returned by GetSoundBufferTime() is used.
      */
    unsigned GetSoundChannelBufferDepth() const { return m_soundChannelBuffers; }

    /**Get default the sound channel buffer time in milliseconds.
       Note the largest of the depth in frames and the depth in milliseconds
       as returned by GetSoundBufferTime() is used.
      */
    unsigned GetSoundChannelBufferTime() const { return m_soundChannelBufferTime; }
  //@}


  protected:
    OpalPCSSEndPoint & m_endpoint;
    PString            m_localRingbackTone;
    PString            m_soundChannelPlayDevice;
    PString            m_soundChannelRecordDevice;
    PString            m_soundChannelOnHoldDevice;
    PString            m_soundChannelOnRingDevice;
    unsigned           m_soundChannelBuffers;
    unsigned           m_soundChannelBufferTime;
#if OPAL_VIDEO
    PVideoDevice::OpenArgs m_videoOnHoldDevice;
    PVideoDevice::OpenArgs m_videoOnRingDevice;
#endif

    PThread  * m_ringbackThread;
    PSyncPoint m_ringbackStop;
    void RingbackMain();

    PThread  * m_userInputThread;
    PChannel * m_userInputChannel;
    bool       m_userInputAutoDelete;
    void UserInputMain();

    P_REMOVE_VIRTUAL(PSoundChannel *,CreateSoundChannel(const OpalMediaFormat&,PBoolean),NULL);
};

#else

#define OPAL_PCSS_PREFIX

#endif // OPAL_HAS_PCSS

#endif // OPAL_OPAL_PCSS_H
