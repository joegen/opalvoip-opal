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
 * $Log: t38proto.h,v $
 * Revision 1.2014  2007/09/25 19:35:30  csoutheren
 * Fix compilation when using --disable-audio
 *
 * Revision 2.12  2007/07/25 01:14:52  csoutheren
 * Add support for receiving faxes into a TIFF file
 *
 * Revision 2.11  2007/05/10 05:34:01  csoutheren
 * Ensure fax transmission works with reasonable size audio blocks
 *
 * Revision 2.10  2007/03/29 08:31:25  csoutheren
 * Fix media formats for T.38 endpoint
 *
 * Revision 2.9  2007/03/29 05:19:54  csoutheren
 * Implement T.38 and fax
 *
 * Revision 2.8  2005/02/21 12:19:48  rjongbloed
 * Added new "options list" to the OpalMediaFormat class.
 *
 * Revision 2.7  2003/01/07 04:39:53  robertj
 * Updated to OpenH323 v1.11.2
 *
 * Revision 2.6  2002/11/10 11:33:17  robertj
 * Updated to OpenH323 v1.10.3
 *
 * Revision 2.5  2002/09/16 02:52:36  robertj
 * Added #define so can select if #pragma interface/implementation is used on
 *   platform basis (eg MacOS) rather than compiler, thanks Robert Monaghan.
 *
 * Revision 2.4  2002/09/04 06:01:47  robertj
 * Updated to OpenH323 v1.9.6
 *
 * Revision 2.3  2002/02/11 09:32:12  robertj
 * Updated to openH323 v1.8.0
 *
 * Revision 2.2  2002/01/14 06:35:57  robertj
 * Updated to OpenH323 v1.7.9
 *
 * Revision 2.1  2001/08/01 05:06:00  robertj
 * Major changes to H.323 capabilities, uses OpalMediaFormat for base name.
 *
 * Revision 2.0  2001/07/27 15:48:24  robertj
 * Conversion of OpenH323 to Open Phone Abstraction Library (OPAL)
 *
 * Revision 1.9  2002/12/02 04:07:58  robertj
 * Turned T.38 Originate inside out, so now has WriteXXX() functions that can
 *   be call ed in different thread contexts.
 *
 * Revision 1.8  2002/12/02 00:37:15  robertj
 * More implementation of T38 base library code, some taken from the t38modem
 *   application by Vyacheslav Frolov, eg redundent frames.
 *
 * Revision 1.7  2002/09/16 01:14:15  robertj
 * Added #define so can select if #pragma interface/implementation is used on
 *   platform basis (eg MacOS) rather than compiler, thanks Robert Monaghan.
 *
 * Revision 1.6  2002/09/03 06:19:37  robertj
 * Normalised the multi-include header prevention ifdef/define symbol.
 *
 * Revision 1.5  2002/02/09 04:39:01  robertj
 * Changes to allow T.38 logical channels to use single transport which is
 *   now owned by the OpalT38Protocol object instead of H323Channel.
 *
 * Revision 1.4  2002/01/01 23:27:50  craigs
 * Added CleanupOnTermination functions
 * Thanks to Vyacheslav Frolov
 *
 * Revision 1.3  2001/12/22 01:57:04  robertj
 * Cleaned up code and allowed for repeated sequence numbers.
 *
 * Revision 1.2  2001/11/09 05:39:54  craigs
 * Added initial T.38 support thanks to Adam Lazur
 *
 * Revision 1.1  2001/07/17 04:44:29  robertj
 * Partial implementation of T.120 and T.38 logical channels.
 *
 */

#ifndef __OPAL_T38PROTO_H
#define __OPAL_T38PROTO_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif


#include <ptlib/pipechan.h>

#include <opal/mediafmt.h>
#include <opal/mediastrm.h>
#include <opal/endpoint.h>

class OpalTransport;
class T38_IFPPacket;
class PASN_OctetString;

namespace PWLibStupidLinkerHacks {
  extern int t38Loader;
};

///////////////////////////////////////////////////////////////////////////////

/**This class handles the processing of the T.38 protocol.
  */
class OpalT38Protocol : public PObject
{
    PCLASSINFO(OpalT38Protocol, PObject);
  public:
  /**@name Construction */
  //@{
    /**Create a new protocol handler.
     */
    OpalT38Protocol();

    /**Destroy the protocol handler.
     */
    ~OpalT38Protocol();
  //@}

  /**@name Operations */
  //@{
    /**This is called to clean up any threads on connection termination.
     */
    virtual void Close();

    /**Handle the origination of a T.38 connection.
       An application would normally override this. The default just sends
       "heartbeat" T.30 no signal indicator.
      */
    virtual BOOL Originate();

    /**Write packet to the T.38 connection.
      */
    virtual BOOL WritePacket(
      const T38_IFPPacket & pdu
    );

    /**Write T.30 indicator packet to the T.38 connection.
      */
    virtual BOOL WriteIndicator(
      unsigned indicator
    );

    /**Write data packet to the T.38 connection.
      */
    virtual BOOL WriteMultipleData(
      unsigned mode,
      PINDEX count,
      unsigned * type,
      const PBYTEArray * data
    );

    /**Write data packet to the T.38 connection.
      */
    virtual BOOL WriteData(
      unsigned mode,
      unsigned type,
      const PBYTEArray & data
    );

    /**Handle the origination of a T.38 connection.
      */
    virtual BOOL Answer();

    /**Handle incoming T.38 packet.

       If returns FALSE, then the reading loop should be terminated.
      */
    virtual BOOL HandlePacket(
      const T38_IFPPacket & pdu
    );

    /**Handle lost T.38 packets.

       If returns FALSE, then the reading loop should be terminated.
      */
    virtual BOOL HandlePacketLost(
      unsigned nLost
    );

    /**Handle incoming T.38 indicator packet.
       If returns FALSE, then the reading loop should be terminated.
      */
    virtual BOOL OnIndicator(
      unsigned indicator
    );

    /**Handle incoming T.38 CNG indicator.
       If returns FALSE, then the reading loop should be terminated.
      */
    virtual BOOL OnCNG();

    /**Handle incoming T.38 CED indicator.
       If returns FALSE, then the reading loop should be terminated.
      */
    virtual BOOL OnCED();

    /**Handle incoming T.38 V.21 preamble indicator.
       If returns FALSE, then the reading loop should be terminated.
      */
    virtual BOOL OnPreamble();

    /**Handle incoming T.38 data mode training indicator.
       If returns FALSE, then the reading loop should be terminated.
      */
    virtual BOOL OnTraining(
      unsigned indicator
    );

    /**Handle incoming T.38 data packet.

       If returns FALSE, then the reading loop should be terminated.
      */
    virtual BOOL OnData(
      unsigned mode,
      unsigned type,
      const PBYTEArray & data
    );
  //@}

    OpalTransport * GetTransport() const { return transport; }
    void SetTransport(
      OpalTransport * transport,
      BOOL autoDelete = TRUE
    );

  protected:
    BOOL HandleRawIFP(
      const PASN_OctetString & pdu
    );

    OpalTransport * transport;
    BOOL            autoDeleteTransport;

    BOOL     corrigendumASN;
    unsigned indicatorRedundancy;
    unsigned lowSpeedRedundancy;
    unsigned highSpeedRedundancy;

    int               lastSentSequenceNumber;
    PList<PBYTEArray> redundantIFPs;
};


///////////////////////////////////////////////////////////////////////////////

#if OPAL_AUDIO

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

#endif

///////////////////////////////////////////////////////////////////////////////

/**
  *  This defines a pseudo RTP session used for T.38 channels
  */
/**This class is for the IETF Real Time Protocol interface on UDP/IP.
 */
class T38PseudoRTP : public RTP_UDP
{
  PCLASSINFO(T38PseudoRTP, RTP_UDP);

  public:
  /**@name Construction */
  //@{
    /**Create a new RTP channel.
     */
    T38PseudoRTP(
      PHandleAggregator * aggregator, ///< RTP aggregator
      unsigned id,                    ///<  Session ID for RTP channel
      BOOL remoteIsNAT                ///<  TRUE is remote is behind NAT
    );

    /// Destroy the RTP
    ~T38PseudoRTP();

    BOOL ReadData(RTP_DataFrame & frame, BOOL loop);
    BOOL WriteData(RTP_DataFrame & frame);
    RTP_Session::SendReceiveStatus OnSendData(RTP_DataFrame & frame);
    RTP_Session::SendReceiveStatus OnSendControl(RTP_ControlFrame & /*frame*/, PINDEX & /*len*/);

    RTP_Session::SendReceiveStatus ReadDataPDU(RTP_DataFrame & frame);
    RTP_Session::SendReceiveStatus OnReceiveData(RTP_DataFrame & frame);

    BOOL SetRemoteSocketInfo(PIPSocket::Address address, WORD port, BOOL isDataPort);

  protected:
    int WaitForPDU(PUDPSocket & dataSocket, PUDPSocket & controlSocket, const PTimeInterval & timeout);
    BOOL OnTimeout(RTP_DataFrame & frame);
    BOOL corrigendumASN;
    int consecutiveBadPackets;

    PBYTEArray lastIFP;

#if 0
    PList<PBYTEArray> redundantIFPs;
#endif

  //@}
};

///////////////////////////////////////////////////////////////////////////////

class OpalFaxCallInfo {
  public:
    OpalFaxCallInfo();
    PUDPSocket socket;
    PPipeChannel spanDSP;
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
      OpalConnection & conn,
      const OpalMediaFormat & mediaFormat, ///<  Media format for stream
      unsigned sessionID, 
      BOOL isSource ,                      ///<  Is a source stream
      const PString & token,               ///<  token used to match incoming/outgoing streams
      const PString & filename,
      BOOL receive
    );
  //@}

  /**@name Overrides of OpalMediaStream class */
  //@{
    /**Open the media stream using the media format.

       The default behaviour simply sets the isOpen variable to TRUE.
      */
    virtual BOOL Open();

    /**Close the media stream.

       The default does nothing.
      */
    virtual BOOL Close();

    /**Start the media stream.

       The default behaviour calls Resume() on the associated OpalMediaPatch
       thread if it was suspended.
      */
    virtual BOOL Start();

    /**Read an RTP frame of data from the source media stream.
       The new behaviour simply calls RTP_Session::ReadData().
      */
    virtual BOOL ReadPacket(
      RTP_DataFrame & packet
    );

    /**Write an RTP frame of data to the sink media stream.
       The new behaviour simply calls RTP_Session::WriteData().
      */
    virtual BOOL WritePacket(
      RTP_DataFrame & packet
    );

    /**Indicate if the media stream is synchronous.
       Returns FALSE for RTP streams.
      */
    virtual BOOL IsSynchronous() const;

    virtual PString GetSpanDSPCommandLine(OpalFaxCallInfo &);

  //@}

  protected:
    PMutex infoMutex;
    PString sessionToken;
    OpalFaxCallInfo * faxCallInfo;
    PFilePath filename;
    BOOL receive;
    BYTE writeBuffer[320];
    PINDEX writeBufferLen;
};

///////////////////////////////////////////////////////////////////////////////

/**This class describes a media stream that transfers data to/from a T.38 session
  */
class OpalT38MediaStream : public OpalFaxMediaStream
{
  PCLASSINFO(OpalT38MediaStream, OpalFaxMediaStream);
  public:
    OpalT38MediaStream(
      OpalConnection & conn,
      const OpalMediaFormat & mediaFormat, ///<  Media format for stream
      unsigned sessionID, 
      BOOL isSource ,                      ///<  Is a source stream
      const PString & token,               ///<  token used to match incoming/outgoing streams
      const PString & filename,            ///<  filename
      BOOL receive
    );

    PString GetSpanDSPCommandLine(OpalFaxCallInfo &);

    BOOL ReadPacket(RTP_DataFrame & packet);
    BOOL WritePacket(RTP_DataFrame & packet);
};

///////////////////////////////////////////////////////////////////////////////

class OpalFaxConnection;

/** Fax Endpoint.
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
      OpalManager & manager,      ///<  Manager of all endpoints.
      const char * prefix = "fax" ///<  Prefix for URL style address strings
    );

    /**Destroy endpoint.
     */
    ~OpalFaxEndPoint();
  //@}

    virtual BOOL MakeConnection(
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
      const PString & filename, ///< filename to send/receive
      BOOL receive,
      void * userData = NULL,   ///< Arbitrary data to pass to connection
      OpalConnection::StringOptions * stringOptions = NULL
    );

    /**Get the data formats this endpoint is capable of operating.
       This provides a list of media data format names that may be used by an
       OpalMediaStream may be created by a connection from this endpoint.

       Note that a specific connection may not actually support all of the
       media formats returned here, but should return no more.

       The default behaviour is pure.
      */
    virtual OpalMediaFormatList GetMediaFormats() const;

    virtual PString MakeToken();

  /**@name User Interface operations */
    /**Accept the incoming connection.
      */
    virtual void AcceptIncomingConnection(
      const PString & connectionToken ///<  Token of connection to accept call
    );

    /**Call back when patching a media stream.
       This function is called when a connection has created a new media
       patch between two streams.
      */
    virtual void OnPatchMediaStream(
      const OpalFaxConnection & connection, ///<  Connection having new patch
      BOOL isSource,                         ///<  Source patch
      OpalMediaPatch & patch                 ///<  New patch
    );
  //@}
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
      OpalCall & call,                 ///<   Owner calll for connection
      OpalFaxEndPoint & endpoint,      ///<   Owner endpoint for connection
      const PString & filename,        ///<   filename to send/receive
      BOOL receive,                    ///<   TRUE if receiving a fax
      const PString & _token,           ///<  token for connection
      OpalConnection::StringOptions * stringOptions = NULL
    );

    /**Destroy endpoint.
     */
    ~OpalFaxConnection();
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
      const PString & calleeName,   ///<  Name of endpoint being alerted.
      BOOL withMedia                ///<  Open media with alerting
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

    OpalMediaStream * CreateMediaStream(const OpalMediaFormat & mediaFormat, unsigned sessionID, BOOL isSource);

    /**Call back when patching a media stream.
       This function is called when a connection has created a new media
       patch between two streams.
       Add the echo canceler patch and call the endpoint function of
       the same name.
       Add a PCM silence detector filter.
      */
    virtual void OnPatchMediaStream(
      BOOL isSource,
      OpalMediaPatch & patch    ///<  New patch
    );

    /**Open source transmitter media stream for session.
      */
    virtual BOOL OpenSourceMediaStream(
      const OpalMediaFormatList & mediaFormats, ///<  Optional media format to open
      unsigned sessionID                   ///<  Session to start stream on
    );

    /**Open source transmitter media stream for session.
      */
    virtual OpalMediaStream * OpenSinkMediaStream(
      OpalMediaStream & source    ///<  Source media sink format to open to
    );

  /**@name New operations */
  //@{
    /**Accept the incoming connection.
      */
    virtual void AcceptIncoming();

  //@}

    void AdjustMediaFormats(OpalMediaFormatList & mediaFormats) const;

  protected:
    OpalFaxEndPoint & endpoint;
    PString filename;
    BOOL receive;
    BOOL forceFaxAudio;
};

/////////////////////////////////////////////////////////////////////////////////

class OpalT38Connection;

/** T.38 Endpoint
 */
class OpalT38EndPoint : public OpalFaxEndPoint
{
  PCLASSINFO(OpalT38EndPoint, OpalFaxEndPoint);
  public:
  /**@name Construction */
  //@{
    /**Create a new endpoint.
     */
    OpalT38EndPoint(
      OpalManager & manager,      ///<  Manager of all endpoints.
      const char * prefix = "t38" ///<  Prefix for URL style address strings
    );
    OpalMediaFormatList GetMediaFormats() const;
    PString MakeToken();
    virtual OpalFaxConnection * CreateConnection(OpalCall & call, const PString & filename, BOOL receive, void * /*userData*/, OpalConnection::StringOptions * stringOptions);
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
      OpalT38EndPoint & endpoint,      ///<  Owner endpoint for connection
      const PString & filename,        ///<  filename to send/receive
      BOOL receive,
      const PString & _token,           ///<  token for connection
      OpalConnection::StringOptions * stringOptions = NULL
    );
    void AdjustMediaFormats(OpalMediaFormatList & mediaFormats) const;
    OpalMediaStream * CreateMediaStream(const OpalMediaFormat & mediaFormat, unsigned sessionID, BOOL isSource);
    OpalMediaFormatList GetMediaFormats() const;
};

///////////////////////////////////////////////////////////////////////////////

#define OPAL_T38            "T.38"
#define OPAL_PCM16_FAX      "PCM-16-Fax"

extern const OpalFaxAudioFormat & GetOpalPCM16Fax();
extern const OpalMediaFormat    & GetOpalT38();

#define OpalPCM16Fax          GetOpalPCM16Fax()
#define OpalT38               GetOpalT38()


#endif // __OPAL_T38PROTO_H


