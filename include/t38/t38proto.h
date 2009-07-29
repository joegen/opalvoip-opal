/*
 * t38proto.h
 *
 * T.38 protocol handler
 *
 * Open Phone Abstraction Library
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
 * The Original Code is Open H323 Library.
 *
 * The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#ifndef OPAL_T38_T38PROTO_H
#define OPAL_T38_T38PROTO_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <opal/buildopts.h>


#if OPAL_FAX

#include <ptlib/pipechan.h>

#include <opal/mediafmt.h>
#include <opal/mediastrm.h>
#include <opal/endpoint.h>


class OpalTransport;
class T38_IFPPacket;
class PASN_OctetString;
class OpalFaxConnection;


namespace PWLibStupidLinkerHacks {
  extern int t38Loader;
};

///////////////////////////////////////////////////////////////////////////////

/**
  *  This format is identical to the OpalPCM16 except that it uses a different
  *  sessionID in order to be compatible with T.38
  */

class OpalFaxAudioFormat : public OpalMediaFormat
{
  friend class OpalPluginCodecManager;
    PCLASSINFO(OpalFaxAudioFormat, OpalMediaFormat);
  public:
    OpalFaxAudioFormat(
      const char * fullName,    ///<  Full name of media format
      RTP_DataFrame::PayloadTypes rtpPayloadType, ///<  RTP payload type code
      const char * encodingName,///<  RTP encoding name
      PINDEX   frameSize,       ///<  Size of frame in bytes (if applicable)
      unsigned frameTime,       ///<  Time for frame in RTP units (if applicable)
      unsigned rxFrames,        ///<  Maximum number of frames per packet we can receive
      unsigned txFrames,        ///<  Desired number of frames per packet we transmit
      unsigned maxFrames = 256, ///<  Maximum possible frames per packet
      unsigned clockRate = 8000, ///<  Clock Rate 
      time_t timeStamp = 0       ///<  timestamp (for versioning)
    );
};


///////////////////////////////////////////////////////////////////////////////

class OpalFaxCallInfo {
  public:
    OpalFaxCallInfo();
    PUDPSocket socket;
    PPipeChannel spanDSP;
    PThread * stdoutThread;
    unsigned refCount;
    PIPSocket::Address spanDSPAddr;
    WORD spanDSPPort;
};

///////////////////////////////////////////////////////////////////////////////

/**This class describes a media stream that transfers data to/from a fax session
  */
class OpalFaxMediaStream : public OpalMediaStream
{
  PCLASSINFO(OpalFaxMediaStream, OpalMediaStream);
  public:
  /**@name Construction */
  //@{
    /**Construct a new media stream for T.38 sessions.
      */
    OpalFaxMediaStream(
      OpalFaxConnection & conn,
      const OpalMediaFormat & mediaFormat, ///<  Media format for stream
      unsigned sessionID, 
      bool isSource,                       ///<  Is a source stream
      const PString & token,               ///<  token used to match incoming/outgoing streams
      const PString & filename,
      bool  receive,
      const PString & stationId
    );
  //@}

  /**@name Overrides of OpalMediaStream class */
  //@{
    /**Open the media stream using the media format.

       The default behaviour simply sets the isOpen variable to PTrue.
      */
    virtual PBoolean Open();

    /**Close the media stream.

       The default does nothing.
      */
    virtual PBoolean Close();

    /**Read an RTP frame of data from the source media stream.
       The new behaviour simply calls RTP_Session::ReadData().
      */
    virtual PBoolean ReadPacket(
      RTP_DataFrame & packet
    );

    /**Write an RTP frame of data to the sink media stream.
       The new behaviour simply calls RTP_Session::WriteData().
      */
    virtual PBoolean WritePacket(
      RTP_DataFrame & packet
    );

    /**Indicate if the media stream is synchronous.
       Returns PFalse for RTP streams.
      */
    virtual PBoolean IsSynchronous() const;

#if OPAL_STATISTICS
    virtual void GetStatistics(OpalMediaStatistics & statistics, bool fromPatch = false) const;
#endif
  //@}

    virtual PString GetSpanDSPCommandLine(OpalFaxCallInfo &);

  protected:
    PDECLARE_NOTIFIER(PThread, OpalFaxMediaStream, ReadStdOut);

    OpalFaxConnection & m_connection;
    PMutex              infoMutex;
    PString             sessionToken;
    OpalFaxCallInfo   * m_faxCallInfo;
    PFilePath           m_filename;
    bool                m_receive;
    BYTE                writeBuffer[320];
    PINDEX              writeBufferLen;
    PString             m_stationId;

#if OPAL_STATISTICS
    OpalMediaStatistics::Fax m_statistics;
#else
    int m_result;
#endif
};

///////////////////////////////////////////////////////////////////////////////

/**This class describes a media stream that transfers data to/from a T.38 session
  */
class OpalT38MediaStream : public OpalFaxMediaStream
{
  PCLASSINFO(OpalT38MediaStream, OpalFaxMediaStream);
  public:
    OpalT38MediaStream(
      OpalFaxConnection & conn,
      const OpalMediaFormat & mediaFormat, ///<  Media format for stream
      unsigned sessionID, 
      bool isSource,                       ///<  Is a source stream
      const PString & token,               ///<  token used to match incoming/outgoing streams
      const PString & filename,            ///<  filename
      bool receive,
      const PString & stationId
    );

    PString GetSpanDSPCommandLine(OpalFaxCallInfo &);

    virtual PBoolean ReadPacket(RTP_DataFrame & packet);
    virtual PBoolean WritePacket(RTP_DataFrame & packet);

    RTP_DataFrameList queuedFrames;
};

///////////////////////////////////////////////////////////////////////////////

class OpalFaxConnection;

/** Fax Endpoint.
    This class represents connection that can take a standard group 3 fax
    TIFF file and produce tones represented by a stream of PCM. This may be
    directed to another endpoint such as OpalLineEndpoint which can send
    the TIFF file to a physical fax machine.

    Relies on the presence of spandsp to do the hard work.
 */
class OpalFaxEndPoint : public OpalEndPoint
{
  PCLASSINFO(OpalFaxEndPoint, OpalEndPoint);
  public:
  /**@name Construction */
  //@{
    /**Create a new endpoint.
     */
    OpalFaxEndPoint(
      OpalManager & manager,        ///<  Manager of all endpoints.
      const char * g711Prefix = "fax", ///<  Prefix for URL style address strings
      const char * t38Prefix = "t38"  ///<  Prefix for URL style address strings
    );

    /**Destroy endpoint.
     */
    ~OpalFaxEndPoint();
  //@}

    virtual PBoolean MakeConnection(
      OpalCall & call,          ///<  Owner of connection
      const PString & party,    ///<  Remote party to call
      void * userData = NULL,          ///<  Arbitrary data to pass to connection
      unsigned int options = 0,     ///<  options to pass to conneciton
      OpalConnection::StringOptions * stringOptions = NULL
    );

    /**Create a connection for the fax endpoint.
      */
    virtual OpalFaxConnection * CreateConnection(
      OpalCall & call,          ///< Owner of connection
      void * userData,
      OpalConnection::StringOptions * stringOptions,
      const PString & filename, ///< filename to send/receive
      bool receive,
      bool t38
    );

    /**Get the data formats this endpoint is capable of operating.
       This provides a list of media data format names that may be used by an
       OpalMediaStream may be created by a connection from this endpoint.

       Note that a specific connection may not actually support all of the
       media formats returned here, but should return no more.

       The default behaviour is pure.
      */
    virtual OpalMediaFormatList GetMediaFormats() const;

  /**@name User Interface operations */
    /**Accept the incoming connection.
      */
    virtual void AcceptIncomingConnection(
      const PString & connectionToken ///<  Token of connection to accept call
    );

    /**Fax transmission/receipt completed.
       Default behaviour releases the connection.
      */
    virtual void OnFaxCompleted(
      OpalFaxConnection & connection, ///< Connection that completed.
      bool failed   ///< Fax ended with failure
    );
  //@}

  /**@name Member variable access */
    /**Get the location of the spandsp executable.
      */
    const PFilePath & GetSpanDSP() const { return m_spanDSP; }

    /**Set the location of the spandsp executable.
      */
    void SetSpanDSP(
      const PString & path    ///< New path for SpanDSP executable
    ) { m_spanDSP = path; }

    /**Get the default directory for received faxes.
      */
    const PString & GetDefaultDirectory() const { return m_defaultDirectory; }

    /**Set the default directory for received faxes.
      */
    void SetDefaultDirectory(
      const PString & dir    /// New directory for fax reception
    ) { m_defaultDirectory = dir; }

    const PString & GetT38Prefix() const { return m_t38Prefix; }
  //@}

  protected:
    PString    m_t38Prefix;
    PFilePath  m_spanDSP;
    PDirectory m_defaultDirectory;
};


///////////////////////////////////////////////////////////////////////////////

/** Fax Connection
 */
class OpalFaxConnection : public OpalConnection
{
  PCLASSINFO(OpalFaxConnection, OpalConnection);
  public:
  /**@name Construction */
  //@{
    /**Create a new endpoint.
     */
    OpalFaxConnection(
      OpalCall & call,                 ///< Owner calll for connection
      OpalFaxEndPoint & endpoint,      ///< Owner endpoint for connection
      const PString & filename,        ///< filename to send/receive
      bool receive,                    ///< true if receiving a fax
      const PString & _token,          ///< token for connection
      OpalConnection::StringOptions * stringOptions = NULL
    );

    /**Destroy endpoint.
     */
    ~OpalFaxConnection();
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

    /**Get the data formats this connection is capable of operating.
       This provides a list of media data format names that an
       OpalMediaStream may be created in within this connection.

       The default behaviour returns the formats the PSoundChannel can do,
       typically only PCM-16.
      */
    virtual OpalMediaFormatList GetMediaFormats() const;

    virtual void ApplyStringOptions(OpalConnection::StringOptions & stringOptions);
    virtual OpalMediaStream * CreateMediaStream(const OpalMediaFormat & mediaFormat, unsigned sessionID, PBoolean isSource);
    virtual void AdjustMediaFormats(OpalMediaFormatList & mediaFormats) const;

  /**@name New operations */
  //@{
    /**Accept the incoming connection.
      */
    virtual void AcceptIncoming();

    /**Fax transmission/receipt completed.
       Default behaviour calls equivalent function on OpalFaxEndPoint.
      */
    virtual void OnFaxCompleted(
      bool failed   ///< Fax ended with failure
    );

    /**Get receive fax flag.
      */
    bool IsReceive() const { return m_receive; }
  //@}

  protected:
    OpalFaxEndPoint & m_endpoint;
    PString           m_filename;
    bool              m_receive;
    PString           m_stationId;
};

///////////////////////////////////////////////////////////////////////////////

/** T.38 Connection
 */
class OpalT38Connection : public OpalFaxConnection
{
  PCLASSINFO(OpalT38Connection, OpalFaxConnection);
  public:
  /**@name Construction */
  //@{
    /**Create a new endpoint.
     */
    OpalT38Connection(
      OpalCall & call,                 ///<  Owner calll for connection
      OpalFaxEndPoint & endpoint,      ///<  Owner endpoint for connection
      const PString & filename,        ///<  filename to send/receive
      bool receive,                    ///<  true if receiving fax
      const PString & token,           ///<  token for connection
      OpalConnection::StringOptions * stringOptions = NULL
    );

    virtual PString GetPrefixName() const { return m_endpoint.GetT38Prefix(); }
    virtual void ApplyStringOptions(OpalConnection::StringOptions & stringOptions);
    virtual void OnEstablished();
    virtual OpalMediaStream * CreateMediaStream(const OpalMediaFormat & mediaFormat, unsigned sessionID, PBoolean isSource);
    virtual void OnMediaPatchStop(unsigned sessionId, bool isSource);
    virtual OpalMediaFormatList GetMediaFormats() const;

    // triggers into fax mode
    enum Mode {
      Mode_Wait,      // Do nothing and wait for the other end to send to switch to T.38 mode
      Mode_Timeout,   // Switch to T.38 mode a short time after call is established
      Mode_UserInput, // Send CNG/CED as UserInput (e.g. RFC2833).
      Mode_InBand     // Send CNG/CED tone in-band (not yet implemented!)
    };

    virtual PBoolean SendUserInputTone(
      char tone,
      unsigned duration
    );

  protected:
    PDECLARE_NOTIFIER(PTimer,  OpalT38Connection, OnSendCNGCED);
    PDECLARE_NOTIFIER(PTimer,  OpalT38Connection, OnFaxChangeTimeout);
    PDECLARE_NOTIFIER(PThread, OpalT38Connection, OpenFaxStreams);
    void RequestFaxMode(bool fax);

    Mode m_syncMode;

    bool   m_faxMode;  // false if audio, true if fax
    PTimer m_faxTimer;
};


#endif // OPAL_FAX

#endif // OPAL_T38_T38PROTO_H
