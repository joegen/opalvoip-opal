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
 * $Log: pcss.h,v $
 * Revision 1.2012  2004/05/17 13:24:18  rjongbloed
 * Added silence suppression.
 *
 * Revision 2.10  2004/03/11 06:54:27  csoutheren
 * Added ability to disable SIP or H.323 stacks
 *
 * Revision 2.9  2003/03/17 10:11:05  robertj
 * Added call back functions for creating sound channel.
 * Added video support.
 *
 * Revision 2.8  2003/03/06 03:57:47  robertj
 * IVR support (work in progress) requiring large changes everywhere.
 *
 * Revision 2.7  2002/09/16 02:52:35  robertj
 * Added #define so can select if #pragma interface/implementation is used on
 *   platform basis (eg MacOS) rather than compiler, thanks Robert Monaghan.
 *
 * Revision 2.6  2002/06/16 02:19:31  robertj
 * Fixed and clarified function for initiating call, thanks Ted Szoczei
 *
 * Revision 2.5  2002/01/22 05:05:16  robertj
 * Revamp of user input API triggered by RFC2833 support
 *
 * Revision 2.4  2001/11/13 06:25:56  robertj
 * Changed SetUpConnection() so returns BOOL as returning
 *   pointer to connection is not useful.
 *
 * Revision 2.3  2001/10/15 04:29:26  robertj
 * Removed answerCall signal and replaced with state based functions.
 *
 * Revision 2.2  2001/08/17 08:33:38  robertj
 * More implementation.
 *
 * Revision 2.1  2001/08/01 05:52:24  robertj
 * Moved media formats list from endpoint to connection.
 *
 * Revision 2.0  2001/07/27 15:48:24  robertj
 * Conversion of OpenH323 to Open Phone Abstraction Library (OPAL)
 *
 */

#ifndef __OPAL_PCSS_H
#define __OPAL_PCSS_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <opal/buildopts.h>

#include <opal/endpoint.h>


class OpalPCSSConnection;


/**PC Sound System endpoint.
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
      OpalManager & manager,  /// Manager of all endpoints.
      const char * prefix = "pc" /// Prefix for URL style address strings
    );

    /**Destroy endpoint.
     */
    ~OpalPCSSEndPoint();
  //@}

  /**@name Overrides from OpalManager */
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
       FALSE is returned.

       This function usually returns almost immediately with the connection
       continuing to occur in a new background thread.

       If FALSE is returned then the connection could not be established. For
       example if a PSTN endpoint is used and the assiciated line is engaged
       then it may return immediately. Returning a non-NULL value does not
       mean that the connection will succeed, only that an attempt is being
       made.

       The default behaviour is pure.
     */
    virtual BOOL MakeConnection(
      OpalCall & call,        /// Owner of connection
      const PString & party,  /// Remote party to call
      void * userData = NULL  /// Arbitrary data to pass to connection
    );
  //@}

  /**@name Customisation call backs */
  //@{
    /**Create a connection for the PCSS endpoint.
       The default implementation is to create a OpalPCSSConnection.
      */
    virtual OpalPCSSConnection * CreateConnection(
      OpalCall & call,    /// Owner of connection
      const PString & playDevice, /// Sound channel play device
      const PString & recordDevice, /// Sound channel record device
      void * userData     /// Arbitrary data to pass to connection
    );

    /**Create an PSoundChannel based media stream.
      */
    virtual PSoundChannel * CreateSoundChannel(
      const OpalPCSSConnection & connection, /// Connection needing created sound channel
      BOOL isSource                          // Direction for channel
    );
  //@}

  /**@name User Interface operations */
  //@{
    /**Call back to get the destination for outgoing call.
       If FALSE is returned the call is aborted.

       The default implementation is pure.
      */
    virtual PString OnGetDestination(
      const OpalPCSSConnection & connection /// Connection having event
    ) = 0;

    /**Call back to indicate that remote is ringing.
       If FALSE is returned the call is aborted.

       The default implementation is pure.
      */
    virtual void OnShowIncoming(
      const OpalPCSSConnection & connection /// Connection having event
    ) = 0;

    /**Accept the incoming connection.
      */
    virtual void AcceptIncomingConnection(
      const PString & connectionToken /// Token of connection to accept call
    );

    /**Call back to indicate that remote is ringing.
       If FALSE is returned the call is aborted.

       The default implementation is pure.
      */
    virtual BOOL OnShowOutgoing(
      const OpalPCSSConnection & connection /// Connection having event
    ) = 0;

    /**Call back to indicate that the remote user has indicated something.
       If FALSE is returned the call is aborted.

       The default implementation does nothing.
      */
    virtual BOOL OnShowUserInput(
      const OpalPCSSConnection & connection, /// Connection having event
      const PString & indication
    );
  //@}

  /**@name Member variable access */
  //@{
    /**Set the name for the sound channel to be used for output.
       If the name is not suitable for use with the PSoundChannel class then
       the function will return FALSE and not change the device.

       This defaults to the value of the PSoundChannel::GetDefaultDevice()
       function.
     */
    virtual BOOL SetSoundChannelPlayDevice(const PString & name);

    /**Get the name for the sound channel to be used for output.
       This defaults to the value of the PSoundChannel::GetDefaultDevice()
       function.
     */
    const PString & GetSoundChannelPlayDevice() const { return soundChannelPlayDevice; }

    /**Set the name for the sound channel to be used for input.
       If the name is not suitable for use with the PSoundChannel class then
       the function will return FALSE and not change the device.

       This defaults to the value of the PSoundChannel::GetDefaultDevice()
       function.
     */
    virtual BOOL SetSoundChannelRecordDevice(const PString & name);

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
      unsigned depth    // New depth
    );
  //@}

  protected:
    PString  soundChannelPlayDevice;
    PString  soundChannelRecordDevice;
    unsigned soundChannelBuffers;
};


/**PC Sound System connection.
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
      OpalCall & call,              /// Owner calll for connection
      OpalPCSSEndPoint & endpoint,  /// Owner endpoint for connection
      const PString & playDevice,   /// Sound channel play device
      const PString & recordDevice  /// Sound channel record device
    );

    /**Destroy endpoint.
     */
    ~OpalPCSSConnection();
  //@}

  /**@name Overrides from OpalConnection */
  //@{
    /**Start an outgoing connection.
       This function will initiate the connection to the remote entity, for
       example in H.323 it sends a SETUP, in SIP it sends an INVITE etc.

       The default behaviour does.
      */
    virtual BOOL SetUpConnection();

    /**Indicate to remote endpoint an alert is in progress.
       If this is an incoming connection and the AnswerCallResponse is in a
       AnswerCallDeferred or AnswerCallPending state, then this function is
       used to indicate to that endpoint that an alert is in progress. This is
       usually due to another connection which is in the call (the B party)
       has received an OnAlerting() indicating that its remote endpoint is
       "ringing".

       The default behaviour does nothing.
      */
    virtual BOOL SetAlerting(
      const PString & calleeName,   /// Name of endpoint being alerted.
      BOOL withMedia                /// Open media with alerting
    );

    /**Indicate to remote endpoint we are connected.

       The default behaviour does nothing.
      */
    virtual BOOL SetConnected();

    /**Get the destination address of an incoming connection.
       The default behaviour collects a DTMF number terminated with a '#' or
       if no digits were entered for a time (default 3 seconds). If no digits
       are entered within a longer time time (default 30 seconds), then an
       empty string is returned.
      */
    virtual PString GetDestinationAddress();

    /**Get the data formats this connection is capable of operating.
       This provides a list of media data format names that an
       OpalMediaStream may be created in within this connection.

       The default behaviour returns the formats the PSoundChannel can do,
       typically only PCM-16.
      */
    virtual OpalMediaFormatList GetMediaFormats() const;

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
      const OpalMediaFormat & mediaFormat, /// Media format for stream
      unsigned sessionID,                  /// Session number for stream
      BOOL isSource                        /// Is a source stream
    );

    /**Call back when opening a media stream.
       This function is called when a connection has created a new media
       stream according to the logic of its underlying protocol.

       The usual requirement is that media streams are created on all other
       connections participating in the call and all of the media streams are
       attached to an instance of an OpalMediaPatch object that will read from
       one of the media streams passing data to the other media streams.

       The default behaviour calls the ancestor and adds a PCM silence
       detector filter.
      */
    virtual BOOL OnOpenMediaStream(
      OpalMediaStream & stream    /// New media stream being opened
    );

    /**Send a user input indication to the remote endpoint.
       This sends an arbitrary string as a user indication. If DTMF tones in
       particular are required to be sent then the SendIndicationTone()
       function should be used.

       The default behaviour plays the DTMF tones on the line.
      */
    virtual BOOL SendUserInputString(
      const PString & value                   /// String value of indication
    );
  //@}

  /**@name New operations */
  //@{
    /**Call is initiated as the A-Party.
      */
    virtual void InitiateCall();

    /**Accept the incoming connection.
      */
    virtual void AcceptIncoming();

    /**Create an PSoundChannel based media stream.
      */
    virtual PSoundChannel * CreateSoundChannel(
      BOOL isSource // Direction for channel
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


#endif // __OPAL_PCSS_H


// End of File ///////////////////////////////////////////////////////////////
