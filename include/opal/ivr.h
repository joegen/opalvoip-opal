/*
 * ivr.h
 *
 * Interactive Voice Response support.
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
 * $Log: ivr.h,v $
 * Revision 1.2006  2004/04/18 13:35:27  rjongbloed
 * Fixed ability to make calls where both endpoints are specified a priori. In particular
 *   fixing the VXML support for an outgoing sip/h323 call.
 *
 * Revision 2.4  2003/03/19 02:30:45  robertj
 * Added removal of IVR stuff if EXPAT is not installed on system.
 *
 * Revision 2.3  2003/03/17 10:26:59  robertj
 * Added video support.
 *
 * Revision 2.2  2003/03/07 05:49:54  robertj
 * Use OpalVXMLSession so OnEndSession() to automatically closes connection.
 *
 * Revision 2.1  2003/03/06 03:57:47  robertj
 * IVR support (work in progress) requiring large changes everywhere.
 *
 */

#ifndef __OPAL_IVR_H
#define __OPAL_IVR_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif


#if P_EXPAT

#include <opal/opalvxml.h>
#include <opal/endpoint.h>


class OpalIVRConnection;


/**Interactive Voice Response endpoint.
 */
class OpalIVREndPoint : public OpalEndPoint
{
    PCLASSINFO(OpalIVREndPoint, OpalEndPoint);
  public:
  /**@name Construction */
  //@{
    /**Create a new endpoint.
     */
    OpalIVREndPoint(
      OpalManager & manager,  /// Manager of all endpoints.
      const char * prefix = "ivr" /// Prefix for URL style address strings
    );

    /**Destroy endpoint.
     */
    ~OpalIVREndPoint();
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
    virtual OpalIVRConnection * CreateConnection(
      OpalCall & call,        /// Owner of connection
      const PString & token,  /// Call token for new connection
      void * userData,        /// Arbitrary data to pass to connection
      const PString & vxml    /// vxml to execute
    );
  //@}

  /**@name Customisation call backs */
  //@{
    /**Create a unique token for a new conection.
      */
    virtual PString CreateConnectionToken();
  //@}

    const PString & GetDefaultVXML() const { return defaultVXML; }
    void SetDefaultVXML(
      const PString & vxml
    );

  protected:
    unsigned nextTokenNumber;
    PString  defaultVXML;
};


/**Interactive Voice Response connection.
 */
class OpalIVRConnection : public OpalConnection
{
    PCLASSINFO(OpalIVRConnection, OpalConnection);
  public:
  /**@name Construction */
  //@{
    /**Create a new endpoint.
     */
    OpalIVRConnection(
      OpalCall & call,            /// Owner calll for connection
      OpalIVREndPoint & endpoint, /// Owner endpoint for connection
      const PString & token,      /// Token for connection
      void * userData,            /// Arbitrary data to pass to connection
      const PString & vxml        /// vxml to execute
    );

    /**Destroy endpoint.
     */
    ~OpalIVRConnection();
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

    /**Call is initiated as the A-Party.
      */
    virtual void InitiateCall();

  protected:
    OpalIVREndPoint & endpoint;
    PString           vxmlToLoad;
    OpalVXMLSession   vxmlSession;
};


/**This class describes a media stream that transfers data to/from an IVR
   vxml session.
  */
class OpalIVRMediaStream : public OpalRawMediaStream
{
    PCLASSINFO(OpalIVRMediaStream, OpalRawMediaStream);
  public:
  /**@name Construction */
  //@{
    /**Construct a new media stream for IVR session.
      */
    OpalIVRMediaStream(
      const OpalMediaFormat & mediaFormat, /// Media format for stream
      unsigned sessionID,                  /// Session number for stream
      BOOL isSource,                       /// Is a source stream
      PVXMLSession & vxml                  /// vxml session to use
    );
  //@}

  /**@name Overrides of OpalMediaStream class */
  //@{
    /**Open the media stream using the media format.

       The default behaviour simply sets the member variable "mediaFormat"
       and "defaultDataSize".
      */
    virtual BOOL Open();

    /**Indicate if the media stream is synchronous.
       Returns FALSE for IVR streams.
      */
    virtual BOOL IsSynchronous() const;
  //@}

  protected:
    PVXMLSession & vxmlSession;
};


#endif // P_EXPAT

#endif // __OPAL_IVR_H


// End of File ///////////////////////////////////////////////////////////////
