/*
 * capi_ep.h
 *
 * ISDN via CAPI EndPoint
 *
 * Open Phone Abstraction Library
 *
 * Copyright (c) 2010 Vox Lucida Pty. Ltd.
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
 * The Original Code is Open H323 Library.
 *
 * The Initial Developer of the Original Code is Vox Lucida Pty. Ltd.
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#ifndef OPAL_LIDS_CAPI_EP_H
#define OPAL_LIDS_CAPI_EP_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <opal/buildopts.h>

#if OPAL_CAPI

#include <opal/endpoint.h>


class OpalCapiFunctions;
class OpalCapiConnection;
struct OpalCapiMessage;


/**This class describes and endpoint that handles ISDN via CAPI.
 */
class OpalCapiEndPoint : public OpalEndPoint
{
  PCLASSINFO(OpalCapiEndPoint, OpalEndPoint);

  public:
  /**@name Construction */
  //@{
    /**Create a new endpoint.
     */
    OpalCapiEndPoint(
      OpalManager & manager   ///<  Manager of all endpoints.
    );

    /// Make sure thread has stopped before exiting.
    ~OpalCapiEndPoint();
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
      OpalCall & call,          ///< Owner of connection
      const PString & party,    ///< Remote party to call
      void * userData = NULL,   ///< Arbitrary data to pass to connection
      unsigned int options = 0,  ///<  options to pass to conneciton
      OpalConnection::StringOptions * stringOptions  = NULL ///< Options to pass to connection
    );

    /**Get the data formats this endpoint is capable of operating.
       This provides a list of media data format names that may be used by an
       OpalMediaStream may be created by a connection from this endpoint.

       Note that a specific connection may not actually support all of the
       media formats returned here, but should return no more.

       The default behaviour is pure.
      */
    virtual OpalMediaFormatList GetMediaFormats() const;
  //@}

  /**@name Connection management */
  //@{
    virtual OpalCapiConnection * CreateConnection(
      OpalCall & call,        ///<  Call that owns the connection
      void * userData,        ///<  Arbitrary user data from MakeConnection
      unsigned int options,     ///<  Options bit mask to pass to conneciton
      OpalConnection::StringOptions * stringOptions, ///< Options to pass to connection
      unsigned controller,
      unsigned bearer
    );
  //@}

  /**@name Actions */
  //@{
    /** Open controllers and start accepting incoming calls.
      */
    unsigned OpenControllers();
  //@}

  /**@name Member variable access */
  //@{
  //@}

  protected:
    bool GetFreeLine(unsigned & controller, unsigned & bearer);
    PDECLARE_NOTIFIER(PThread, OpalCapiEndPoint, ProcessMessages);
    virtual void ProcessMessage(const OpalCapiMessage & message);
    void ProcessConnectInd(const OpalCapiMessage & message);
    virtual bool PutMessage(OpalCapiMessage & message);

    OpalCapiFunctions * m_capi;
    PThread           * m_thread;
    unsigned            m_applicationId;
    PSyncPoint          m_listenCompleted;

    struct Controller {
      Controller() : m_active(false) { }

      bool         m_active;
      vector<bool> m_bearerInUse;
    };
    typedef std::vector<Controller> ControllerVector;
    ControllerVector m_controllers;
    PMutex           m_controllerMutex;

    struct IdToConnMap : public std::map<DWORD, PSafePtr<OpalCapiConnection> >
    {
      bool Forward(const OpalCapiMessage & message, DWORD id);
    };

    IdToConnMap m_cbciToConnection;
    IdToConnMap m_plciToConnection;
    IdToConnMap m_ncciToConnection;

  friend class OpalCapiConnection;
};


/**This class describes the ISDN via CAPI connection.
 */
class OpalCapiConnection : public OpalConnection
{
  PCLASSINFO(OpalCapiConnection, OpalConnection);

  public:
  /**@name Construction */
  //@{
    /**Create a new connection.
     */
    OpalCapiConnection(
      OpalCall & call,              ///<  Owner calll for connection
      OpalCapiEndPoint & endpoint,  ///<  Endpoint for LID connection
      unsigned int options,         ///<  Options bit mask to pass to conneciton
      OpalConnection::StringOptions * stringOptions, ///< Options to pass to connection
      unsigned controller,
      unsigned bearer
    );
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
    virtual bool IsNetworkConnection() const;

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
       has received an OnAlerting() indicating that its remoteendpoint is
       "ringing".

       The default behaviour starts the ring back tone.
      */
    virtual PBoolean SetAlerting(
      const PString & calleeName,   ///<  Name of endpoint being alerted.
      PBoolean withMedia                ///<  Open media with alerting
    );

    /**Indicate to remote endpoint we are connected.

       The default behaviour stops the ring back tone.
      */
    virtual PBoolean SetConnected();

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

       The default behaviour calls starts playing the busy tone and calls the
       ancestor function.
      */
    virtual void OnReleased();

    /**Get the destination address of an incoming connection.
       The default behaviour collects a DTMF number terminated with a '#' or
       if no digits were entered for a time (default 3 seconds). If no digits
       are entered within a longer time time (default 30 seconds), then an
       empty string is returned.
      */
    virtual PString GetDestinationAddress();

    /**Get the data formats this connection is capable of operating.
       This provides a list of media data format names that a
       OpalMediaStream may be created in within this connection.

       The default behaviour returns the capabilities of the LID line.
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
      const OpalMediaFormat & mediaFormat, ///<  Media format for stream
      unsigned sessionID,                  ///<  Session number for stream
      PBoolean isSource                        ///<  Is a source stream
    );

    /**Send a user input indication to the remote endpoint.
       This sends DTMF emulation user input. If something other than the
       standard tones need be sent use the SendUserInputString() function.

       The default behaviour plays the DTMF tone on the line.
      */
    virtual PBoolean SendUserInputTone(
      char tone,    ///<  DTMF tone code
      int duration  ///<  Duration of tone in milliseconds
    );
  //@}

  /**@name Member variable access */
  //@{
  //@}

  protected:
    virtual void ProcessMessage(const OpalCapiMessage & message);
    virtual bool PutMessage(OpalCapiMessage & message);

    OpalCapiEndPoint & m_endpoint;
    unsigned           m_controller; // 1..127
    unsigned           m_bearer;
    DWORD              m_PLCI;
    DWORD              m_NCCI;

    PSyncPoint m_disconnected;

  friend class OpalCapiEndPoint;
  friend struct OpalCapiEndPoint::IdToConnMap;
  friend class OpalCapiMediaStream;
};


/**This class describes a media stream that transfers data to/from ISDN
   via CAPI.
  */
class OpalCapiMediaStream : public OpalMediaStream
{
    PCLASSINFO(OpalCapiMediaStream, OpalMediaStream);
  public:
  /**@name Construction */
  //@{
    /**Construct a new media stream for ISDN via CAPI.
      */
    OpalCapiMediaStream(
      OpalCapiConnection & conn,            ///< Owner connection
      const OpalMediaFormat & mediaFormat,  ///<  Media format for stream
      unsigned sessionID,                   ///<  Session number for stream
      PBoolean isSource                     ///<  Is a source stream
    );
  //@}


  /**@name Overrides of OpalMediaStream class */
  //@{
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

  /**@name Member variable access */
  //@{
  //@}

  protected:
    virtual void InternalClose();

    OpalCapiConnection & m_connection;
    PQueueChannel        m_queue;
    PSyncPoint           m_written;
    PAdaptiveDelay       m_delay;

  friend class OpalCapiConnection;
};


#endif // OPAL_CAPI

#endif // OPAL_LIDS_CAPI_EP_H


// End of File ///////////////////////////////////////////////////////////////
