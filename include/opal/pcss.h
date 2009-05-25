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
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#ifndef OPAL_OPAL_PCSS_H
#define OPAL_OPAL_PCSS_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif


#include <opal/buildopts.h>

#ifndef OPAL_PTLIB_AUDIO
#ifdef _MSC_VER
#pragma message("PTLib soundcard support not available")
#else
#warning "PTLib soundcard support not available"
#endif
#else

#include <ptlib/sound.h>
#include <opal/endpoint.h>

class OpalPCSSConnection;


/** PC Sound System endpoint.
 */
class OpalPCSSEndPoint : public OpalEndPoint
{
    PCLASSINFO(OpalPCSSEndPoint, OpalEndPoint);
  public:
  /**@name Construction */
  //@{
    /**Create a new endpoint.
     */
    OpalPCSSEndPoint(
      OpalManager & manager,  ///<  Manager of all endpoints.
      const char * prefix = "pc" ///<  Prefix for URL style address strings
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
       PFalse is returned.

       This function usually returns almost immediately with the connection
       continuing to occur in a new background thread.

       If PFalse is returned then the connection could not be established. For
       example if a PSTN endpoint is used and the assiciated line is engaged
       then it may return immediately. Returning a non-NULL value does not
       mean that the connection will succeed, only that an attempt is being
       made.

       The default behaviour is pure.
     */
    virtual PBoolean MakeConnection(
      OpalCall & call,           ///<  Owner of connection
      const PString & party,     ///<  Remote party to call
      void * userData = NULL,    ///<  Arbitrary data to pass to connection
      unsigned int options = 0,  ///<  options to pass to conneciton
      OpalConnection::StringOptions * stringOptions  = NULL
    );
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
      unsigned options,
      OpalConnection::StringOptions * stringOptions
    );

    /**Create an PSoundChannel based media stream.
      */
    virtual PSoundChannel * CreateSoundChannel(
      const OpalPCSSConnection & connection, ///<  Connection needing created sound channel
      const OpalMediaFormat & mediaFormat,   ///<  Media format for the connection
      PBoolean isSource                          ///<  Direction for channel
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
      PSafetyMode mode = PSafeReadWrite
    ) { return GetConnectionWithLockAs<OpalPCSSConnection>(token, mode); }

    /**Call back to indicate that remote is ringing.
       If PFalse is returned the call is aborted.

       The default implementation is pure.
      */
    virtual PBoolean OnShowIncoming(
      const OpalPCSSConnection & connection ///<  Connection having event
    ) = 0;

    /**Accept the incoming connection.
       Returns PFalse if the connection token does not correspond to a valid
       connection.
      */
    virtual PBoolean AcceptIncomingConnection(
      const PString & connectionToken ///<  Token of connection to accept call
    );

    /**Reject the incoming connection.
       Returns PFalse if the connection token does not correspond to a valid
       connection.
      */
    virtual PBoolean RejectIncomingConnection(
      const PString & connectionToken ///<  Token of connection to accept call
    );

    /**Call back to indicate that remote is ringing.
       If PFalse is returned the call is aborted.

       The default implementation is pure.
      */
    virtual PBoolean OnShowOutgoing(
      const OpalPCSSConnection & connection ///<  Connection having event
    ) = 0;

    /**Call back to indicate that the remote user has indicated something.
       If PFalse is returned the call is aborted.

       The default implementation does nothing.
      */
    virtual PBoolean OnShowUserInput(
      const OpalPCSSConnection & connection, ///<  Connection having event
      const PString & indication
    );

    
    /**Call back when patching a media stream.
       This function is called when a connection has created a new media
       patch between two streams.
      */
    virtual void OnPatchMediaStream(
      const OpalPCSSConnection & connection, ///<  Connection having new patch
      PBoolean isSource,                         ///<  Source patch
      OpalMediaPatch & patch                 ///<  New patch
    );
  //@}

  /**@name Member variable access */
  //@{
    /**Set the name for the sound channel to be used for output.
       If the name is not suitable for use with the PSoundChannel class then
       the function will return PFalse and not change the device.

       This defaults to the value of the PSoundChannel::GetDefaultDevice()
       function.
     */
    virtual PBoolean SetSoundChannelPlayDevice(const PString & name);

    /**Get the name for the sound channel to be used for output.
       This defaults to the value of the PSoundChannel::GetDefaultDevice()
       function.
     */
    const PString & GetSoundChannelPlayDevice() const { return soundChannelPlayDevice; }

    /**Set the name for the sound channel to be used for input.
       If the name is not suitable for use with the PSoundChannel class then
       the function will return PFalse and not change the device.

       This defaults to the value of the PSoundChannel::GetDefaultDevice()
       function.
     */
    virtual PBoolean SetSoundChannelRecordDevice(const PString & name);

    /**Get the name for the sound channel to be used for input.
       This defaults to the value of the PSoundChannel::GetDefaultDevice()
       function.
     */
    const PString & GetSoundChannelRecordDevice() const { return soundChannelRecordDevice; }

    /**Get default the sound channel buffer depth.
      */
    unsigned GetSoundChannelBufferDepth() const { return soundChannelBuffers; }

    /**Set the default sound channel buffer depth.
      */
    void SetSoundChannelBufferDepth(
      unsigned depth    ///<  New depth
    );
  //@}

    OpalMediaFormatList GetMediaFormats() const;

  protected:
    PString  soundChannelPlayDevice;
    PString  soundChannelRecordDevice;
    unsigned soundChannelBuffers;

  private:
    P_REMOVE_VIRTUAL(OpalPCSSConnection *, CreateConnection(OpalCall &, const PString &, const PString &, void *), 0)
};


/** PC Sound System connection.
 */
class OpalPCSSConnection : public OpalConnection
{
    PCLASSINFO(OpalPCSSConnection, OpalConnection);
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
      unsigned options = 0,
      OpalConnection::StringOptions * stringOptions = NULL
    );

    /**Destroy endpoint.
     */
    ~OpalPCSSConnection();
  //@}

  /**@name Overrides from OpalConnection */
  //@{
    /**Get indication of connection being to a "network".
       This indicates the if the connection may be regarded as a "network"
       connection. The distinction is about if there is a concept of a "remote"
       party being connected to and is best described by example: sip, h323,
       iax and pstn are all "network" connections as they connect to something
       "remote". While pc, pots and ivr are not as the entity being connected
       to is intrinsically local.
      */
    virtual bool IsNetworkConnection() const { return false; }

    /**Start an outgoing connection.
       This function will initiate the connection to the remote entity, for
       example in H.323 it sends a SETUP, in SIP it sends an INVITE etc.

       The default behaviour does.
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
      PBoolean withMedia                ///<  Open media with alerting
    );

    /**Initiate the transfer of an existing call (connection) to a new remote 
       party.

       If remoteParty is a valid call token, then the remote party is transferred
       to that party (consultation transfer) and both calls are cleared.
     */
    virtual bool TransferConnection(
      const PString & remoteParty   ///<  Remote party to transfer the existing call to
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

    /**Call back when patching a media stream.
       This function is called when a connection has created a new media
       patch between two streams.
       Add the echo canceler patch and call the endpoint function of
       the same name.
       Add a PCM silence detector filter.
      */
    virtual void OnPatchMediaStream(
      PBoolean isSource,
      OpalMediaPatch & patch    ///<  New patch
    );

    /**Open source or sink media stream for session.
      */
    virtual OpalMediaStreamPtr OpenMediaStream(
      const OpalMediaFormat & mediaFormat, ///<  Media format to open
      unsigned sessionID,                  ///<  Session to start stream on
      bool isSource                        ///< Stream is a source/sink
    );

    /**Set  the volume (gain) for the audio media channel to the specified percentage.
      */
    virtual PBoolean SetAudioVolume(
      PBoolean source,                  ///< true for source (microphone), false for sink (speaker)
      unsigned percentage           ///< Gain, 0=silent, 100=maximun
    );

    /**Get the average signal level (0..32767) for the audio media channel.
       A return value of UINT_MAX indicates no valid signal, eg no audio channel opened.
      */
    virtual unsigned GetAudioSignalLevel(
      PBoolean source                   ///< true for source (microphone), false for sink (speaker)
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
    /**Accept the incoming connection.
      */
    virtual void AcceptIncoming();

    /**Create an PSoundChannel based media stream.
      */
    virtual PSoundChannel * CreateSoundChannel(
      const OpalMediaFormat & mediaFormat, ///<  Media format for the connection
      PBoolean isSource                        ///<  Direction for channel
    );
  //@}

  /**@name Member variable access */
  //@{
    /**Get the name for the sound channel to be used for output.
       This defaults to the value of the PSoundChannel::GetDefaultDevice()
       function.
     */
    const PString & GetSoundChannelPlayDevice() const { return soundChannelPlayDevice; }

    /**Get the name for the sound channel to be used for input.
       This defaults to the value of the PSoundChannel::GetDefaultDevice()
       function.
     */
    const PString & GetSoundChannelRecordDevice() const { return soundChannelRecordDevice; }

    /**Get default the sound channel buffer depth.
      */
    unsigned GetSoundChannelBufferDepth() const { return soundChannelBuffers; }
  //@}


  protected:
    OpalPCSSEndPoint & endpoint;
    PString            soundChannelPlayDevice;
    PString            soundChannelRecordDevice;
    unsigned           soundChannelBuffers;
};

#endif // OPAL_PTLIB_AUDIO

#endif // OPAL_OPAL_PCSS_H


// End of File ///////////////////////////////////////////////////////////////
