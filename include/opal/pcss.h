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
 * Revision 1.2001  2001/07/27 15:48:24  robertj
 * Conversion of OpenH323 to Open Phone Abstraction Library (OPAL)
 *
 */

#ifndef __OPAL_PCSS_H
#define __OPAL_PCSS_H

#ifdef __GNUC__
#pragma interface
#endif


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
      OpalManager & manager
    );

    /**Destroy endpoint.
     */
    ~OpalPCSSEndPoint();
  //@}

  /**@name Overrides from OpalManager */
  //@{
    /**Set up a connection to a remote party.
       This is called from the OpalManager::SetUpConnection() function once
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
       NULL is returned.

       This function returns almost immediately with the connection continuing
       to occur in a new background thread.

       If NULL is returned then the connection could not be established. For
       example if a PSTN endpoint is used and the assiciated line is engaged
       then it may return immediately. Returning a non-NULL value does not
       mean that the connection will succeed, only that an attempt is being
       made.

       Note, the returned pointer to the connection is not locked and may be
       deleted at any time. This is extremely unlikely immediately after the
       function is called, but you should not keep this pointer beyond that
       brief time. The the FindCallWithLock() function for future references
       to the connection object.

       The default behaviour is pure.
     */
    virtual OpalConnection * SetUpConnection(
      OpalCall & call,        /// Owner of connection
      const PString & party,  /// Remote party to call
      void * userData = NULL  /// Arbitrary data to pass to connection
    );

    /**Get the data formats this endpoint is capable of operating in.
       This provides a list of media data format names that an
       OpalMediaStream may be created in within connections created by this
       endpoint.
      */
    virtual OpalMediaFormat::List GetMediaFormats() const;
  //@}

  /**@name Customisation call backs */
  //@{
    /**Create a connection for the PCSS endpoint.
       The default implementation is to create a OpalPCSSConnection.
      */
    virtual OpalPCSSConnection * CreateConnection(
      OpalCall & call,    /// Owner of connection
      void * userData     /// Arbitrary data to pass to connection
    );
  //@}

  /**@name User Interface operations */
  //@{
    /**Call back to indicate that remote is ringing.
       If FALSE is returned the call is aborted.

       The default implementation is pure.
      */
    virtual void OnShowRinging(
      const PString & callerName    /// Name of endpoint that is calling.
    ) = 0;

    /**Call back to indicate that remote is ringing.
       If FALSE is returned the call is aborted.

       The default implementation is pure.
      */
    virtual BOOL OnShowAlerting(
      const PString & calleeName    /// Name of endpoint being alerted.
    ) = 0;
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
      OpalCall & call,            /// Owner calll for connection
      OpalPCSSEndPoint & endpoint /// Owner endpoint for connection
    );

    /**Destroy endpoint.
     */
    ~OpalPCSSConnection();
  //@}

  /**@name Overrides from OpalConnection */
  //@{
    /**Get the phase of the connection.
      */
    virtual Phases GetPhase() const;

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
      const PString & calleeName    /// Name of endpoint being alerted.
    );

    /**Indicate to remote endpoint we are connected.

       The default behaviour does nothing.
      */
    virtual BOOL SetConnected();

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
      BOOL isSource,      /// Is a source stream
      unsigned sessionID  /// Session number for stream
    );
  //@}

  /**@name Call progress functions */
  //@{
    /**Indicate the result of answering an incoming call.
       This should only be called if the OnAnswerCall() callback function has
       returned a AnswerCallPending or AnswerCallDeferred response.

       Note sending further AnswerCallPending responses via this function will
       have the result of an Alerting PDU being sent to the remote endpoint.
       In this way multiple Alerting PDUs may be sent.

       Sending a AnswerCallDeferred response would have no effect.
      */
    virtual void AnsweringCall(
      AnswerCallResponse response /// Answer response to incoming call
    );

    /**Indicate in some manner that a call is alerting.
       If FALSE is returned the call is aborted.

       The default implementation calls OpalPCSSEndPoint::OnShowAlerting().
      */
    virtual BOOL SendConnectionAlert(
      const PString & calleeName    /// Name of endpoint being alerted.
    );
  //@}

    void StartOutgoingCall();
    void StartIncomingCall(const PString & callerName);


  protected:
    OpalPCSSEndPoint & endpoint;
    Phases             currentPhase;
};


#endif // __OPAL_PCSS_H


// End of File ///////////////////////////////////////////////////////////////
