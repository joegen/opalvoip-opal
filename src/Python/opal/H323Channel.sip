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
#include <h323/channels.h>
%End

///////////////////////////////////////////////////////////////////////////////

/**Description of a Logical Channel Number.
   This is used as index into dictionary of logical channels.
 */
class H323ChannelNumber : PObject
{

  public:
    H323ChannelNumber();
    H323ChannelNumber(unsigned number, PBoolean fromRemote);

    virtual PObject * Clone() const;
    virtual PINDEX HashFunction() const;
    virtual Comparison Compare(const PObject & obj) const;

    //H323ChannelNumber & operator++(int);

    operator unsigned() const;
    PBoolean IsFromRemote() const;

};


/**This class describes a logical channel between the two endpoints. They may
   be created and deleted as required in the H245 protocol.

   An application may create a descendent off this class and override
   functions as required for operating the channel protocol.
 */
class H323Channel : PObject /Abstract/
{
  public:
  /**@name Construction */
  //@{
    /**Create a new channel.
     */
    H323Channel(
      H323Connection & connection,        ///<  Connection to endpoint for channel
      const H323Capability & capability   ///<  Capability channel is using
    );

    /**Destroy new channel.
       To avoid usage of deleted objects in background threads, this waits
       for the H323LogicalChannelThread to terminate before continuing.
     */
    ~H323Channel();
  //@}


  /**@name Operations */
  //@{
    enum Directions {
      IsBidirectional,
      IsTransmitter,
      IsReceiver,
      NumDirections
    };

    /**Indicate the direction of the channel.
       Return if the channel is bidirectional, or unidirectional, and which
       direction for the latter case.
     */
    virtual Directions GetDirection() const = 0;

    /**Indicate the session number of the channel.
       Return session for channel. This is primarily for use by RTP based
       channels, for channels for which the concept of a session is not
       meaningfull, the default simply returns 0.
     */
    virtual unsigned GetSessionID() const;

    /**Set the session number of the channel.
       During OLC negotations teh master may change the session number being
       used for the logical channel.

       Returns false if the sesion could not be renumbered.
      */
    virtual bool SetSessionID(
      unsigned sessionID   ///< New session ID
    );

    /**Get the media transport address for the connection.
       This is primarily used to determine if media bypass is possible for the
       call between two connections.

       The default behaviour returns PFalse.
     */
    virtual PBoolean GetMediaTransportAddress(
      OpalTransportAddress & data,        ///<  Data channel address
      OpalTransportAddress & control      ///<  Control channel address
    ) const;

    /**Set the initial bandwidth for the channel.
       This calculates the initial bandwidth required by the channel and
       returns PTrue if the connection can support this bandwidth.

       The default behaviour gets the bandwidth requirement from the codec
       object created by the channel.
     */
    virtual PBoolean SetInitialBandwidth() = 0;

    /**Open the channel.
       The default behaviour just calls connection.OnStartLogicalChannel() and
       if successful sets the opened member variable.
      */
    virtual PBoolean Open();

    /**This is called when the channel can start transferring data.
     */
    virtual PBoolean Start() = 0;

    /**This is called to clean up any threads on connection termination.
     */
    virtual void Close();

    /**Indicate if has been opened.
     */
    PBoolean IsOpen() const;

    /**Limit bit flow for the logical channel.
       The default behaviour passes this on to the codec if not NULL.
     */
    virtual void OnFlowControl(
      long bitRateRestriction   ///<  Bit rate limitation
    );

    /**Limit bit flow for the logical channel.
       The default behaviour does nothing.
     */
    virtual void OnJitterIndication(
      unsigned long jitter,           ///<  Estimated received jitter in microseconds
      int skippedFrameCount,  ///<  Frames skipped by decodec
      int additionalBuffer    ///<  Additional size of video decoder buffer
    );
  //@}

  /**@name Member variable access */
  //@{
    /**Get the number of the channel.
     */
    const H323ChannelNumber & GetNumber() const;

    /**Set the number of the channel.
     */
    void SetNumber(const H323ChannelNumber & num);

    /**Get the number of the reverse channel (if present).
     */
    const H323ChannelNumber & GetReverseChannel() const;

    /**Set the number of the reverse channel (if present).
     */
    void SetReverseChannel(const H323ChannelNumber & num);

    /**Get the bandwidth used by the channel in 100's of bits/sec.
     */
    unsigned GetBandwidthUsed() const;

    /**Get the bandwidth used by the channel in 100's of bits/sec.
     */
    PBoolean SetBandwidthUsed(
      unsigned bandwidth  ///<  New bandwidth
    );

    /**Get the capability that created this channel.
      */
    const H323Capability & GetCapability() const;

    /**Get the "pause" flag.
       A paused channel is one that prevents the annunciation of the channels
       data. For example for audio this would mute the data, for video it would
       still frame.

       Note that channel is not stopped, and may continue to actually receive
       data, it is just that nothing is done with it.
      */
    PBoolean IsPaused() const;

    /**Set the "pause" flag.
       A paused channel is one that prevents the annunciation of the channels
       data. For example for audio this would mute the data, for video it would
       still frame.

       Note that channel is not stopped, and may continue to actually receive
       data, it is just that nothing is done with it.
      */
    void SetPause(
      PBoolean pause   ///<  New pause flag
    );
  //@}

    virtual void OnMediaCommand(OpalMediaCommand &);

};


/**This class describes a unidirectional logical channel between the two
   endpoints. They may be created and deleted as required in the H245 protocol.

   An application may create a descendent off this class and override
   functions as required for operating the channel protocol.
 */
class H323UnidirectionalChannel : H323Channel /Abstract/
{

  public:
  /**@name Construction */
  //@{
    /**Create a new channel.
     */
    H323UnidirectionalChannel(
      H323Connection & connection,        ///<  Connection to endpoint for channel
      const H323Capability & capability,  ///<  Capability channel is using
      Directions direction                ///<  Direction of channel
    );

    /**Destroy the channel, deleting the associated media stream.
      */
    ~H323UnidirectionalChannel();
  //@}

  /**@name Overrides from class H323Channel */
  //@{
    /**Indicate the direction of the channel.
       Return if the channel is bidirectional, or unidirectional, and which
       direction for th latter case.
     */
    virtual Directions GetDirection() const;

    /**Set the initial bandwidth for the channel.
       This calculates the initial bandwidth required by the channel and
       returns PTrue if the connection can support this bandwidth.

       The default behaviour gets the bandwidth requirement from the codec
       object created by the channel.
     */
    virtual PBoolean SetInitialBandwidth();

    /**Open the channel.
      */
    virtual PBoolean Open();

    /**This is called when the channel can start transferring data.
       The default action is to start one threads, with it either calling
       Receive() or Transmit() depending on the receiver member variable.
     */
    virtual PBoolean Start();


    void OnMediaCommand(OpalMediaCommand & command);

};


/**This class describes a bidirectional logical channel between the two
   endpoints. They may be created and deleted as required in the H245 protocol.

   An application may create a descendent off this class and override
   functions as required for operating the channel protocol.
 */
class H323BidirectionalChannel : H323Channel /Abstract/
{
  public:
  /**@name Construction */
  //@{
    /**Create a new channel.
     */
    H323BidirectionalChannel(
      H323Connection & connection,        ///<  Connection to endpoint for channel
      const H323Capability & capability   ///<  Capability channel is using
    );
  //@}

  /**@name Overrides from class H323Channel */
  //@{
    /**Indicate the direction of the channel.
       Return if the channel is bidirectional, or unidirectional, and which
       direction for th latter case.
     */
    virtual Directions GetDirection() const;

    /**This is called when the channel can start transferring data.
       The default action is to start two threads, one calls Receive() and the
       other calls Transmit().
     */
    virtual PBoolean Start();
  //@}
};


///////////////////////////////////////////////////////////////////////////////

/**This class is for encpsulating the IETF Real Time Protocol interface.
 */
class H323_RealTimeChannel : H323UnidirectionalChannel
{

  public:
  /**@name Construction */
  //@{
    /**Create a new channel.
     */
    H323_RealTimeChannel(
      H323Connection & connection,        ///<  Connection to endpoint for channel
      const H323Capability & capability,  ///<  Capability channel is using
      Directions direction                ///<  Direction of channel
    );
  //@}

    /**Set the dynamic payload type used by this channel.
      */
    virtual PBoolean SetDynamicRTPPayloadType(
      int newType  ///<  New RTP payload type number
    );

    RTP_DataFrame::PayloadTypes GetDynamicRTPPayloadType() const;
  //@}

};


///////////////////////////////////////////////////////////////////////////////

/**This class is for encpsulating the IETF Real Time Protocol interface.
 */
class H323_RTPChannel : H323_RealTimeChannel
{

  public:
  /**@name Construction */
  //@{
    /**Create a new channel.
     */
    H323_RTPChannel(
      H323Connection & connection,        ///<  Connection to endpoint for channel
      const H323Capability & capability,  ///<  Capability channel is using
      Directions direction,               /// Direction of channel
      RTP_Session & rtp                   /// RTP session for channel
    );

    /// Destroy the channel
    ~H323_RTPChannel();
  //@}

  /**@name Overrides from class H323Channel */
  //@{
    /**Indicate the session number of the channel.
       Return session for channel. This returns the session ID of the
       RTP_Session member variable.
     */
    virtual unsigned GetSessionID() const;

    /**Set the session number of the channel.
       During OLC negotations teh master may change the session number being
       used for the logical channel.

       Returns false if the sesion could not be renumbered.
      */
    virtual bool SetSessionID(
      unsigned sessionID   ///< New session ID
    );
  //@}

};


///////////////////////////////////////////////////////////////////////////////

/**This class is for encpsulating the IETF Real Time Protocol interface as used
by a remote host.
 */
class H323_ExternalRTPChannel : H323_RealTimeChannel
{

  public:
  /**@name Construction */
  //@{
    /**Create a new channel.
     */
    H323_ExternalRTPChannel(
      H323Connection & connection,        ///<  Connection to endpoint for channel
      const H323Capability & capability,  ///<  Capability channel is using
      Directions direction,               ///<  Direction of channel
      unsigned sessionID                  ///<  Session ID for channel
    );
    /**Create a new channel.
     */
    H323_ExternalRTPChannel(
      H323Connection & connection,        ///<  Connection to endpoint for channel
      const H323Capability & capability,  ///<  Capability channel is using
      Directions direction,               ///<  Direction of channel
      unsigned sessionID,                 ///<  Session ID for channel
      const H323TransportAddress & data,  ///<  Data address
      const H323TransportAddress & control///<  Control address
    );
    /**Create a new channel.
     */
    H323_ExternalRTPChannel(
      H323Connection & connection,        ///<  Connection to endpoint for channel
      const H323Capability & capability,  ///<  Capability channel is using
      Directions direction,               ///<  Direction of channel
      unsigned sessionID,                 ///<  Session ID for channel
      const PIPSocket::Address & ip,      ///<  IP address of media server
      WORD dataPort                       ///<  Data port (control is dataPort+1)
    );
  //@}

  /**@name Overrides from class H323Channel */
  //@{
    /**Indicate the session number of the channel.
       Return session for channel. This returns the session ID of the
       RTP_Session member variable.
     */
    virtual unsigned GetSessionID() const;

    /**Get the media transport address for the connection.
       This is primarily used to determine if media bypass is possible for the
       call between two connections.

       The default behaviour returns PFalse.
     */
    virtual PBoolean GetMediaTransportAddress(
      OpalTransportAddress & data,        ///<  Data channel address
      OpalTransportAddress & control      ///<  Control channel address
    ) const;

    /**Start the channel.
      */
    virtual PBoolean Start();

    /**Handle channel data reception.

       This is called by the thread started by the Start() function and is
       typically a loop writing to the codec and reading from the transport
       (eg RTP_session).
      */
    virtual void Receive();

    /**Handle channel data transmission.

       This is called by the thread started by the Start() function and is
       typically a loop reading from the codec and writing to the transport
       (eg an RTP_session).
      */
    virtual void Transmit();
  //@}

    void SetExternalAddress(
      const H323TransportAddress & data,  ///<  Data address
      const H323TransportAddress & control///<  Control address
    );

    const H323TransportAddress & GetRemoteMediaAddress()        const;
    const H323TransportAddress & GetRemoteMediaControlAddress() const;

    PBoolean GetRemoteAddress(
      PIPSocket::Address & ip,
      WORD & dataPort
    ) const;

};


///////////////////////////////////////////////////////////////////////////////

/**This class describes a data logical channel between the two endpoints.
   They may be created and deleted as required in the H245 protocol.

   An application may create a descendent off this class and override
   functions as required for operating the channel protocol.
 */
class H323DataChannel : H323UnidirectionalChannel
{
  public:
  /**@name Construction */
  //@{
    /**Create a new channel.
     */
    H323DataChannel(
      H323Connection & connection,        ///<  Connection to endpoint for channel
      const H323Capability & capability,  ///<  Capability channel is using
      Directions direction,               ///<  Direction of channel
      unsigned sessionID                  ///<  Session ID for channel
    );

    /**Destroy the channel.
      */
    ~H323DataChannel();
  //@}


  /**@name Operations */
  //@{
    /**Create the H323Listener class to be used.
       This is called on receipt of an OpenLogicalChannel request.

       The default behaviour creates a compatible listener using the
       connections control channel as a basis and returns PTrue if successful.
      */
    virtual PBoolean CreateListener();

    /**Create the H323Transport class to be used.
       This is called on receipt of an OpenLogicalChannelAck response. It
       should not return PTrue unless the transport member variable is set.

       The default behaviour uses the connection signalling channel to create
       the transport and returns PTrue if successful.
      */
    virtual PBoolean CreateTransport();
  //@}


};


/////////////////////////////////////////////////////////////////////////////
