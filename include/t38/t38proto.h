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

#include <opal/mediafmt.h>
#include <opal/mediastrm.h>
#include <opal/mediasession.h>
#include <ep/localep.h>


class OpalTransport;
class OpalFaxConnection;

#if OPAL_PTLIB_ASN
class T38_IFPPacket;
class PASN_OctetString;
#endif

#define OPAL_OPT_STATION_ID  "Station-Id"      ///< String option for fax station ID string
#define OPAL_OPT_HEADER_INFO "Header-Info"     ///< String option for transmitted fax page header
#define OPAL_NO_G111_FAX     "No-G711-Fax"     ///< Suppress G.711 fall back
#define OPAL_SWITCH_ON_CED   "Switch-On-CED"   ///< Try switch to T.38 on receipt of CED tone
#define OPAL_T38_SWITCH_TIME "T38-Switch-Time" ///< Seconds for fail safe switch to T.38 mode

#define OPAL_FAX_TIFF_FILE "TIFF-File"


///////////////////////////////////////////////////////////////////////////////

class OpalFaxConnection;

/** Fax Endpoint.
    This class represents connection that can take a standard group 3 fax
    TIFF file and produce either T.38 packets or actual tones represented
    by a stream of PCM. For T.38 it is expected the second connection in the
    call supports T.38 e.g. SIP or H.323. If PCM is being used then the second
    connection may be anything that supports PCM, such as SIP or H.323 using
    G.711 codec or OpalLineEndpoint which could the send the TIFF file to a
    physical fax machine.

    Relies on the presence of the spandsp plug in to do the hard work.
 */
class OpalFaxEndPoint : public OpalLocalEndPoint
{
  PCLASSINFO(OpalFaxEndPoint, OpalLocalEndPoint);
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

  /**@name Overrides from OpalEndPoint */
  //@{
    virtual PSafePtr<OpalConnection> MakeConnection(
      OpalCall & call,          ///<  Owner of connection
      const PString & party,    ///<  Remote party to call
      void * userData = NULL,          ///<  Arbitrary data to pass to connection
      unsigned int options = 0,     ///<  options to pass to conneciton
      OpalConnection::StringOptions * stringOptions = NULL  ///< Options to pass to connection
    );

    /**Get the data formats this endpoint is capable of operating.
       This provides a list of media data format names that may be used by an
       OpalMediaStream may be created by a connection from this endpoint.

       Note that a specific connection may not actually support all of the
       media formats returned here, but should return no more.
      */
    virtual OpalMediaFormatList GetMediaFormats() const;
  //@}

  /**@name Fax specific operations */
  //@{
    /**Determine if the fax plug in is available, that is fax system can be used.
      */
    virtual bool IsAvailable() const;

    /**Create a connection for the fax endpoint.
      */
    virtual OpalFaxConnection * CreateConnection(
      OpalCall & call,          ///< Owner of connection
      void * userData,          ///<  Arbitrary data to pass to connection
      OpalConnection::StringOptions * stringOptions, ///< Options to pass to connection
      const PString & filename, ///< filename to send/receive
      bool receiving,           ///< Flag for receiving/sending fax
      bool disableT38           ///< Flag to disable use of T.38
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
    PDirectory m_defaultDirectory;
};


///////////////////////////////////////////////////////////////////////////////

/** Fax Connection.
    There are six modes of operation:
        Mode            receiving     disableT38    filename
        TIFF -> T.38      false         false       "something.tif"
        T.38 -> TIFF      true          false       "something.tif"
        TIFF -> G.711     false         true        "something.tif"
        G.711 ->TIFF      true          true        "something.tif"
        T.38  -> G.711    false       don't care    PString::Empty()
        G.711 -> T.38     true        don't care    PString::Empty()

    If T.38 is involved then there is generally two stages to the setup, as
    indicated by the m_switchedToT38 flag. When false then we are in audio
    mode looking for CNG/CED tones. When true, then we are switching, or have
    switched, to T.38 operation. If the switch fails, then the m_disableT38
    is set and we proceed in fall back mode.
 */
class OpalFaxConnection : public OpalLocalConnection
{
  PCLASSINFO(OpalFaxConnection, OpalLocalConnection);
  public:
  /**@name Construction */
  //@{
    /**Create a new endpoint.
     */
    OpalFaxConnection(
      OpalCall & call,                 ///< Owner calll for connection
      OpalFaxEndPoint & endpoint,      ///< Owner endpoint for connection
      const PString & filename,        ///< TIFF file name to send/receive
      bool receiving,                  ///< True if receiving a fax
      bool disableT38,                 ///< True if want to force G.711
      OpalConnection::StringOptions * stringOptions = NULL  ///< Options to pass to connection
    );

    /**Destroy endpoint.
     */
    ~OpalFaxConnection();
  //@}

  /**@name Overrides from OpalLocalConnection */
  //@{
    virtual PString GetPrefixName() const;

    virtual OpalMediaFormatList GetMediaFormats() const;
    virtual void AdjustMediaFormats(bool local, const OpalConnection * otherConnection, OpalMediaFormatList & mediaFormats) const;
    virtual void OnEstablished();
    virtual void OnReleased();
    virtual OpalMediaStream * CreateMediaStream(const OpalMediaFormat & mediaFormat, unsigned sessionID, PBoolean isSource);
    virtual void OnStartMediaPatch(OpalMediaPatch & patch);
    virtual void OnClosedMediaStream(const OpalMediaStream & stream);
    virtual PBoolean SendUserInputTone(char tone, unsigned duration);
    virtual void OnUserInputTone(char tone, unsigned duration);
    virtual bool SwitchFaxMediaStreams(bool toT38);
    virtual void OnSwitchedFaxMediaStreams(bool toT38, bool success);
    virtual bool OnSwitchingFaxMediaStreams(bool toT38);
    virtual void OnApplyStringOptions();
  //@}

  /**@name New operations */
  //@{
    /**Fax transmission/receipt completed.
       Default behaviour calls equivalent function on OpalFaxEndPoint.
      */
    virtual void OnFaxCompleted(
      bool failed   ///< Fax ended with failure
    );

    /**Get fax transmission/receipt statistics.
      */
    virtual void GetStatistics(
      OpalMediaStatistics & statistics  ///< Statistics for call
    ) const;

    /**Get the file to send/receive
      */
    const PString & GetFileName() const { return m_filename; }

    /**Get receive fax flag.
      */
    bool IsReceive() const { return m_receiving; }
  //@}

  protected:
    PDECLARE_NOTIFIER(PTimer,  OpalFaxConnection, OnSwitchTimeout);
    void InternalOpenFaxStreams();
    void InternalOnFaxCompleted();

    void SetFaxMediaFormatOptions(OpalMediaFormat & mediaFormat) const;

    OpalFaxEndPoint & m_endpoint;
    PString           m_filename;
    bool              m_receiving;
    bool              m_disableT38;
    unsigned          m_switchTime;
    OpalMediaFormat   m_tiffFileFormat;

    PTimer m_switchTimer;

    OpalMediaStatistics m_finalStatistics;
    bool                m_completed;
};


typedef OpalFaxConnection OpalT38Connection; // For backward compatibility

class T38_UDPTLPacket;

class OpalFaxSession : public OpalMediaSession
{
  public:
    static const PCaselessString & UDPTL();

    OpalFaxSession(const Init & init);
    ~OpalFaxSession();

    virtual const PCaselessString & GetSessionType() const { return UDPTL(); }
    virtual bool Open(const PString & localInterface, const OpalTransportAddress & remoteAddress, bool isMediaAddress);
    virtual bool IsOpen() const;
    virtual bool Close();
    virtual OpalTransportAddress GetLocalAddress(bool isMediaAddress = true) const;
    virtual OpalTransportAddress GetRemoteAddress(bool isMediaAddress = true) const;
    virtual bool SetRemoteAddress(const OpalTransportAddress & remoteAddress, bool isMediaAddress = true);

    virtual void AttachTransport(Transport & transport);
    virtual Transport DetachTransport();

    virtual OpalMediaStream * CreateMediaStream(
      const OpalMediaFormat & mediaFormat, 
      unsigned sessionID, 
      bool isSource
    );

    bool WriteData(RTP_DataFrame & frame);
    bool ReadData(RTP_DataFrame & frame);

    void ApplyMediaOptions(const OpalMediaFormat & mediaFormat);

  protected:
    void SetFrameFromIFP(RTP_DataFrame & frame, const PASN_OctetString & ifp, unsigned sequenceNumber);
    void DecrementSentPacketRedundancy(bool stripRedundancy);
    bool WriteUDPTL();

    Transport          m_savedTransport;
    PIPSocket        * m_dataSocket;
    bool               m_shuttingDown;

    bool               m_rawUDPTL; // Put UDPTL directly in RTP payload
    PINDEX             m_datagramSize;

    int                m_consecutiveBadPackets;
    bool               m_awaitingGoodPacket;
    T38_UDPTLPacket  * m_receivedPacket;
    unsigned           m_expectedSequenceNumber;
    int                m_secondaryPacket;

    std::map<int, int> m_redundancy;
    PTimeInterval      m_redundancyInterval;
    PTimeInterval      m_keepAliveInterval;
    bool               m_optimiseOnRetransmit;
    std::vector<int>   m_sentPacketRedundancy;
    T38_UDPTLPacket  * m_sentPacket;
    PMutex             m_writeMutex;
    PTimer             m_timerWriteDataIdle;
    PDECLARE_NOTIFIER(PTimer,  OpalFaxSession, OnWriteDataIdle);
};

class OpalFaxMediaStream : public OpalMediaStream
{
    PCLASSINFO(OpalFaxMediaStream, OpalMediaStream);

  public:
    OpalFaxMediaStream(OpalConnection & conn,
                       const OpalMediaFormat & mediaFormat,
                       unsigned sessionID,
                       bool isSource,
                       OpalFaxSession & session);

    virtual PBoolean Open();
    virtual PBoolean ReadPacket(RTP_DataFrame & packet);
    virtual PBoolean WritePacket(RTP_DataFrame & packet);
    virtual PBoolean IsSynchronous() const;
    virtual bool InternalUpdateMediaFormat(const OpalMediaFormat & mediaFormat);

  protected:
    virtual void InternalClose();

    OpalFaxSession & m_session;
};

#endif // OPAL_FAX

#endif // OPAL_T38_T38PROTO_H
