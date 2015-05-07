/*
 * Python bindings.
 *
 * Open Phone Abstraction Library (OPAL)
 *
 * Copyright (c) 2011 Demetrius Cassidy
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
 * The Original Code is Open Phone Abstraction Library (OPAL)
 *
 * The Initial Developer of the Original Code is Demetrius Cassidy
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision$
 * $Author$
 * $Date$
 */

%ModuleHeaderCode
#include <ptlib.h>
#include <opal/pcss.h>
%End


/** PC Sound System endpoint.
 */
class OpalPCSSEndPoint : OpalLocalEndPoint
{
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
    // virtual PSafePtr<OpalConnection> MakeConnection(
      // OpalCall & call,           ///<  Owner of connection
      // const PString & party,     ///<  Remote party to call
      // void * userData = NULL,    ///<  Arbitrary data to pass to connection
      // unsigned options = 0,                    ///< Option bit map to be passed to connection
      // OpalConnection::StringOptions * stringOptions = NULL ///< Options to be passed to connection
    // );
  //@}

  /**@name Overrides from OpalLocalEndPoint */
  //@{
    /**Call back to indicate that remote is ringing.
       If false is returned the call is aborted.

       The default implementation does nothing.
      */
    virtual bool OnOutgoingCall(
      const OpalLocalConnection & connection /NoCopy/ ///<  Connection having event
    );

    /**Call back to indicate that remote is ringing.
       If false is returned the call is aborted.

       The default implementation answers the call immediately.
      */
    virtual bool OnIncomingCall(
      OpalLocalConnection & connection ///<  Connection having event
    );

    /**Call back to indicate that the remote user has indicated something.
       If false is returned the call is aborted.

       The default implementation does nothing.
      */
    virtual bool OnUserInput(
      const OpalLocalConnection & connection /NoCopy/, ///<  Connection having event
      const PString & indication              ///<  Received user input indications
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
      unsigned options,                    ///< Option bit map to be passed to connection
      OpalConnection::StringOptions * stringOptions ///< Options to be passed to connection
    ) /Transfer/;

    /**Create an PSoundChannel based media stream.
      */
    virtual PSoundChannel * CreateSoundChannel(
      const OpalPCSSConnection & connection /NoCopy/, ///<  Connection needing created sound channel
      const OpalMediaFormat & mediaFormat /NoCopy/,   ///<  Media format for the connection
      PBoolean isSource                          ///<  Direction for channel
    ) /Transfer/;
  //@}


    /**Call back to indicate that remote is ringing.
       If false is returned the call is aborted.

       The default implementation is pure.
      */
    virtual PBoolean OnShowIncoming(
      const OpalPCSSConnection & connection /NoCopy/ ///<  Connection having event
    ) = 0;

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
      const OpalPCSSConnection & connection /NoCopy/ ///<  Connection having event
    ) = 0;

    /**Call back to indicate that the remote user has indicated something.
       If false is returned the call is aborted.

       The default implementation does nothing.
      */
    virtual PBoolean OnShowUserInput(
      const OpalPCSSConnection & connection /NoCopy/, ///<  Connection having event
      const PString & indication             ///<  Received user input indications
    );
  //@}

  /**@name Member variable access */
  //@{
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
    const PString & GetSoundChannelPlayDevice() const;

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
    const PString & GetSoundChannelRecordDevice() const;

    /**Get default the sound channel buffer depth.
       Note the largest of the depth in frames and the depth in milliseconds
       as returned by GetSoundBufferTime() is used.
      */
    unsigned GetSoundChannelBufferDepth() const;

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
    unsigned GetSoundChannelBufferTime() const;

    /**Set the default sound channel buffer time in milliseconds.
       Note the largest of the depth in frames and the depth in milliseconds
       as returned by GetSoundBufferTime() is used.
      */
    void SetSoundChannelBufferTime(
      unsigned depth    ///<  New depth in milliseconds
    );
  //@}

};


/** PC Sound System connection.
 */
class OpalPCSSConnection : OpalLocalConnection
{

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
    /**Initiate the transfer of an existing call (connection) to a new remote 
       party.

       If remoteParty is a valid call token, then the remote party is transferred
       to that party (consultation transfer) and both calls are cleared.
     */
    virtual bool TransferConnection(
      const PString & remoteParty   ///<  Remote party to transfer the existing call to
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
  //@}

  /**@name New operations */
  //@{
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
    const PString & GetSoundChannelPlayDevice() const;

    /**Get the name for the sound channel to be used for input.
       This defaults to the value of the PSoundChannel::GetDefaultDevice()
       function.
     */
    const PString & GetSoundChannelRecordDevice() const;

    /**Get default the sound channel buffer depth.
       Note the largest of the depth in frames and the depth in milliseconds
       as returned by GetSoundBufferTime() is used.
      */
    unsigned GetSoundChannelBufferDepth() const;

    /**Get default the sound channel buffer time in milliseconds.
       Note the largest of the depth in frames and the depth in milliseconds
       as returned by GetSoundBufferTime() is used.
      */
    unsigned GetSoundChannelBufferTime() const;
  //@}


};
