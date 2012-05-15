/*
 * rtp.h
 *
 * RTP protocol handler
 *
 * Open H323 Library
 *
 * Copyright (c) 1998-2001 Equivalence Pty. Ltd.
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
 * Portions of this code were written with the assisance of funding from
 * Vovida Networks, Inc. http://www.vovida.com.
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#ifndef OPAL_RTP_RTP_H
#define OPAL_RTP_RTP_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <opal/buildopts.h>

#include <opal/mediasession.h>
#include <ptlib/sockets.h>
#include <ptlib/safecoll.h>
#include <ptclib/pnat.h>
#include <rtp/metrics.h>
#include <ptclib/url.h>

#include <list>


class RTP_JitterBuffer;
class PNatMethod;
class OpalSecurityMode;

///////////////////////////////////////////////////////////////////////////////
// Real Time Protocol - IETF RFC1889 and RFC1890

/**An RTP data frame encapsulation.
  */
class RTP_DataFrame : public PBYTEArray
{
  PCLASSINFO(RTP_DataFrame, PBYTEArray);

  public:
    RTP_DataFrame(PINDEX payloadSize = 0, PINDEX bufferSize = 0);
    RTP_DataFrame(const BYTE * data, PINDEX len, bool dynamic = true);

    enum {
      ProtocolVersion = 2,
      MinHeaderSize = 12,
      // Max safe MTU size (576 bytes as per RFC879) minus IP, UDP an RTP headers
      MaxMtuPayloadSize = (576-20-16-12)
    };

    enum PayloadTypes {
      PCMU,         // G.711 u-Law
      FS1016,       // Federal Standard 1016 CELP
      G721,         // ADPCM - Subsumed by G.726
      G726 = G721,
      GSM,          // GSM 06.10
      G7231,        // G.723.1 at 6.3kbps or 5.3 kbps
      DVI4_8k,      // DVI4 at 8kHz sample rate
      DVI4_16k,     // DVI4 at 16kHz sample rate
      LPC,          // LPC-10 Linear Predictive CELP
      PCMA,         // G.711 A-Law
      G722,         // G.722
      L16_Stereo,   // 16 bit linear PCM
      L16_Mono,     // 16 bit linear PCM
      G723,         // G.723
      CN,           // Confort Noise
      MPA,          // MPEG1 or MPEG2 audio
      G728,         // G.728 16kbps CELP
      DVI4_11k,     // DVI4 at 11kHz sample rate
      DVI4_22k,     // DVI4 at 22kHz sample rate
      G729,         // G.729 8kbps
      Cisco_CN,     // Cisco systems comfort noise (unofficial)

      CelB = 25,    // Sun Systems Cell-B video
      JPEG,         // Motion JPEG
      H261 = 31,    // H.261
      MPV,          // MPEG1 or MPEG2 video
      MP2T,         // MPEG2 transport system
      H263,         // H.263

      T38 = 38,     // T.38 (internal)

      LastKnownPayloadType,

      DynamicBase = 96,
      MaxPayloadType = 127,
      IllegalPayloadType
    };

    unsigned GetVersion() const { return (theArray[0]>>6)&3; }

    bool GetExtension() const   { return (theArray[0]&0x10) != 0; }
    void SetExtension(bool ext);

    bool GetMarker() const { return (theArray[1]&0x80) != 0; }
    void SetMarker(bool m);

    bool GetPadding() const { return (theArray[0]&0x20) != 0; }
    void SetPadding(bool v)  { if (v) theArray[0] |= 0x20; else theArray[0] &= 0xdf; }
    BYTE * GetPaddingPtr() const { return (BYTE *)(theArray+m_headerSize+m_payloadSize); }

    PINDEX GetPaddingSize() const { return m_paddingSize; }
    bool   SetPaddingSize(PINDEX sz);

    PayloadTypes GetPayloadType() const { return (PayloadTypes)(theArray[1]&0x7f); }
    void         SetPayloadType(PayloadTypes t);

    WORD GetSequenceNumber() const { return *(PUInt16b *)&theArray[2]; }
    void SetSequenceNumber(WORD n) { *(PUInt16b *)&theArray[2] = n; }

    DWORD GetTimestamp() const  { return *(PUInt32b *)&theArray[4]; }
    void  SetTimestamp(DWORD t) { *(PUInt32b *)&theArray[4] = t; }

    DWORD GetSyncSource() const  { return *(PUInt32b *)&theArray[8]; }
    void  SetSyncSource(DWORD s) { *(PUInt32b *)&theArray[8] = s; }

    PINDEX GetContribSrcCount() const { return theArray[0]&0xf; }
    DWORD  GetContribSource(PINDEX idx) const;
    void   SetContribSource(PINDEX idx, DWORD src);

    PINDEX GetHeaderSize() const { return m_headerSize; }

    void CopyHeader(const RTP_DataFrame & other);

    /**Get header extension.
       If idx < 0 then gets RFC 3550 format extension type.
       If idx >= 0 then get RFC 5285 format extension type for the idx'th extension.

       @returns NULL if no extension
      */
    BYTE * GetHeaderExtension(
      unsigned & id,      ///< Identifier for extension
      PINDEX & length,    ///< Length of extension in bytes
      int idx = -1        ///< Index of extension
    ) const;

    enum HeaderExtensionType {
      RFC3550,
      RFC5285_OneByte,
      RFC5285_TwoByte
    };

    /**Set header extension.
       Note when RFC 5285 formats are used, the extension is appened to ones
       already present.

       @returns true if extension legal.
      */
    bool SetHeaderExtension(
      unsigned id,        ///< Identifier for extension
      PINDEX length,      ///< Length of extension in bytes
      const BYTE * data,  ///< Data to put into extension
      HeaderExtensionType type ///< RFC standard used
    );

    PINDEX GetExtensionSizeDWORDs() const;      // get the number of 32 bit words in the extension (excluding the header).
    bool   SetExtensionSizeDWORDs(PINDEX sz);   // set the number of 32 bit words in the extension (excluding the header)

    PINDEX GetPayloadSize() const { return m_payloadSize; }
    bool   SetPayloadSize(PINDEX sz);
    BYTE * GetPayloadPtr()     const { return (BYTE *)(theArray+m_headerSize); }

    virtual PObject * Clone() const { return new RTP_DataFrame(*this); }
    virtual void PrintOn(ostream & strm) const;

    // Note this sets the whole packet length, and calculates the various
    // sub-section sizes: header payload and padding.
    bool SetPacketSize(PINDEX sz);

    /**Get absolute (wall clock) time of packet, if known.
      */
    PTime GetAbsoluteTime() const { return m_absoluteTime; }

    /**Set absolute (wall clock) time of packet.
      */
    void SetAbsoluteTime() { m_absoluteTime.SetCurrentTime(); }
    void SetAbsoluteTime(const PTime & t) { m_absoluteTime = t; }

    /** Get sequence number discontinuity.
        If non-zero this indicates the number of packets detected as missing
        before this packet.
      */
    unsigned GetDiscontinuity() const { return m_discontinuity; }

    void SetDiscontinuity(unsigned lost) { m_discontinuity = lost; }

  protected:
    PINDEX   m_headerSize;
    PINDEX   m_payloadSize;
    PINDEX   m_paddingSize;
    PTime    m_absoluteTime;
    unsigned m_discontinuity;

#if PTRACING
    friend ostream & operator<<(ostream & o, PayloadTypes t);
#endif
};

PLIST(RTP_DataFrameList, RTP_DataFrame);


/**An RTP control frame encapsulation.
  */
class RTP_ControlFrame : public PBYTEArray
{
  PCLASSINFO(RTP_ControlFrame, PBYTEArray);

  public:
    RTP_ControlFrame(PINDEX compoundSize = 2048);

    unsigned GetVersion() const { return (BYTE)theArray[compoundOffset]>>6; }

    unsigned GetCount() const { return (BYTE)theArray[compoundOffset]&0x1f; }
    void     SetCount(unsigned count);

    enum PayloadTypes {
      e_IntraFrameRequest       = 192,
      e_SenderReport            = 200,
      e_ReceiverReport          = 201,
      e_SourceDescription       = 202,
      e_Goodbye                 = 203,
      e_ApplDefined             = 204,
      e_TransportLayerFeedBack  = 205, // RFC4585
      e_PayloadSpecificFeedBack = 206,
      e_ExtendedReport          = 207  // RFC3611
    };

    unsigned GetPayloadType() const { return (BYTE)theArray[compoundOffset+1]; }
    void     SetPayloadType(unsigned t);

    PINDEX GetPayloadSize() const { return 4*(*(PUInt16b *)&theArray[compoundOffset+2]); }
    void   SetPayloadSize(PINDEX sz);

    BYTE * GetPayloadPtr() const;

    bool ReadNextPacket();
    bool StartNewPacket();
    void EndPacket();

    PINDEX GetCompoundSize() const;

    void Reset(PINDEX size);

#pragma pack(1)
    struct ReceiverReport {
      PUInt32b ssrc;      /* data source being reported */
      BYTE fraction;      /* fraction lost since last SR/RR */
      BYTE lost[3];	  /* cumulative number of packets lost (signed!) */
      PUInt32b last_seq;  /* extended last sequence number received */
      PUInt32b jitter;    /* interarrival jitter */
      PUInt32b lsr;       /* last SR packet from this source */
      PUInt32b dlsr;      /* delay since last SR packet */

      unsigned GetLostPackets() const { return (lost[0]<<16U)+(lost[1]<<8U)+lost[2]; }
      void SetLostPackets(unsigned lost);
    };

    struct SenderReport {
      PUInt32b ntp_sec;   /* NTP timestamp */
      PUInt32b ntp_frac;
      PUInt32b rtp_ts;    /* RTP timestamp */
      PUInt32b psent;     /* packets sent */
      PUInt32b osent;     /* octets sent */ 
    };

    struct ExtendedReport {
      /* VoIP Metrics Report Block */
      BYTE bt;                     /* block type */
      BYTE type_specific;          /* determined by the block definition */
      PUInt16b length;             /* length of the report block */
      PUInt32b ssrc;               /* data source being reported */
      BYTE loss_rate;              /* fraction of RTP data packets lost */ 
      BYTE discard_rate;           /* fraction of RTP data packets discarded */
      BYTE burst_density;          /* fraction of RTP data packets within burst periods */
      BYTE gap_density;            /* fraction of RTP data packets within inter-burst gaps */
      PUInt16b burst_duration;     /* the mean duration, in ms, of burst periods */
      PUInt16b gap_duration;       /* the mean duration, in ms, of gap periods */
      PUInt16b round_trip_delay;   /* the most recently calculated round trip time */    
      PUInt16b end_system_delay;   /* the most recently estimates end system delay */
      BYTE signal_level;           /* voice signal level related to 0 dBm */
      BYTE noise_level;            /* ratio of the silent background level to 0 dBm */
      BYTE rerl;                   /* residual echo return loss */
      BYTE gmin;                   /* gap threshold */
      BYTE r_factor;               /* voice quality metric of the call */
      BYTE ext_r_factor;           /* external R factor */
      BYTE mos_lq;                 /* MOS for listen quality */
      BYTE mos_cq;                 /* MOS for conversational quality */
      BYTE rx_config;              /* receiver configuration byte */
      BYTE reserved;               /* reserved for future definition */
      PUInt16b jb_nominal;         /* current nominal jitter buffer delay, in ms */ 
      PUInt16b jb_maximum;         /* current maximum jitter buffer delay, in ms */
      PUInt16b jb_absolute;        /* current absolute maximum jitter buffer delay, in ms */
    };

    enum DescriptionTypes {
      e_END,
      e_CNAME,
      e_NAME,
      e_EMAIL,
      e_PHONE,
      e_LOC,
      e_TOOL,
      e_NOTE,
      e_PRIV,
      NumDescriptionTypes
    };

    struct SourceDescription {
      PUInt32b src;       /* first SSRC/CSRC */
      struct Item {
        BYTE type;        /* type of SDES item (enum DescriptionTypes) */
        BYTE length;      /* length of SDES item (in octets) */
        char data[1];     /* text, not zero-terminated */

        /* WARNING, SourceDescription may not be big enough to contain length and data, for 
           instance, when type == RTP_ControlFrame::e_END.
           Be careful whan calling the following function of it may read to over to 
           memory allocated*/
        unsigned int GetLengthTotal() const {return (unsigned int)(length + 2);} 
        const Item * GetNextItem() const { return (const Item *)((char *)this + length + 2); }
        Item * GetNextItem() { return (Item *)((char *)this + length + 2); }
      } item[1];          /* list of SDES items */
    };

    void StartSourceDescription(
      DWORD src   ///<  SSRC/CSRC identifier
    );

    void AddSourceDescriptionItem(
      unsigned type,            ///<  Description type
      const PString & data      ///<  Data for description
    );

    // RFC4585 Feedback Message Type (FMT)
    unsigned GetFbType() const { return (BYTE)theArray[compoundOffset]&0x1f; }
    void     SetFbType(unsigned type, PINDEX fciSize);

    enum TransportLayerFbTypes {
      e_TransportNACK = 1,
      e_TMMBR = 3,
      e_TMMBN
    };

    enum PayloadSpecificFbTypes {
      e_PictureLossIndication = 1,
      e_SliceLostIndication,
      e_ReferencePictureSelectionIndication,
      e_FullIntraRequest,                     //RFC5104
      e_TemporalSpatialTradeOffRequest,
      e_TemporalSpatialTradeOffNotification,
      e_VideoBackChannelMessage,
      e_ApplicationLayerFbMessage = 15
    };

    struct FbFCI {
      PUInt32b senderSSRC;  /* data source of sender of message */
      PUInt32b mediaSSRC;   /* data source of media */
    };

    struct FbFIR {
      FbFCI    fci;
      PUInt32b requestSSRC;
      BYTE     sequenceNUmber;
    };

    struct FbTSTO {
      FbFCI    fci;
      PUInt32b requestSSRC;
      BYTE     sequenceNUmber;
      BYTE     reserver[2];
      BYTE     tradeOff;
    };

    // Same for request (e_TMMBR) and notification (e_TMMBN)
    struct FbTMMB {
      FbFCI    fci;
      PUInt32b requestSSRC;
      PUInt32b bitRateAndOverhead; // Various bit fields

      unsigned GetBitRate() const;
      unsigned GetOverhead() const { return bitRateAndOverhead & 0x1ff; }
    };

#pragma pack()

  protected:
    PINDEX compoundOffset;
    PINDEX payloadSize;
};


///////////////////////////////////////////////////////////////////////////////

/** Information for RFC 5285 header extensions.
  */
class RTPExtensionHeaderInfo : public PObject
{
    PCLASSINFO(RTPExtensionHeaderInfo, PObject);
  public:
    unsigned m_id;

    enum Direction {
      Undefined = -1,
      Inactive,
      RecvOnly,
      SendOnly,
      SendRecv
    } m_direction;

    PURL m_uri;

    PString m_attributes;

    RTPExtensionHeaderInfo();
    virtual Comparison Compare(const PObject & other) const;

#if OPAL_SIP
    bool ParseSDP(const PString & param);
    void OutputSDP(ostream & strm) const;
#endif
};

typedef std::set<RTPExtensionHeaderInfo> RTPExtensionHeaders;


///////////////////////////////////////////////////////////////////////////////
// 
// class to hold the QoS definitions for an RTP channel

class RTP_QOS : public PObject
{
  PCLASSINFO(RTP_QOS,PObject);
  public:
    PQoS dataQoS;
    PQoS ctrlQoS;
};


///////////////////////////////////////////////////////////////////////////////

/**This class is for encpsulating the IETF Real Time Protocol interface.
 */
class OpalRTPSession : public OpalMediaSession
{
  PCLASSINFO(OpalRTPSession, OpalMediaSession);

  public:
  /**@name Construction */
  //@{
    /**Create a new RTP session.
     */
    OpalRTPSession(
      OpalConnection & connection,    ///< Connection that owns the sesion
      unsigned sessionId,             ///< Unique (in connection) session ID for session
      const OpalMediaType & mediaType ///< Media type for session
    );

    /**Delete a session.
       This deletes the userData field if autoDeleteUserData is true.
     */
    ~OpalRTPSession();
  //@}

  /**@name Overrides from class OpalMediaSession */
  //@{
    virtual bool Open(const PString & localInterface);
    virtual bool Close();
    virtual bool Shutdown(bool reading);
    virtual OpalTransportAddress GetLocalMediaAddress() const;
    virtual OpalTransportAddress GetRemoteMediaAddress() const;
    virtual bool SetRemoteMediaAddress(const OpalTransportAddress & address);
    virtual OpalTransportAddress GetRemoteControlAddress() const;
    virtual bool SetRemoteControlAddress(const OpalTransportAddress & address);

    virtual void AttachTransport(Transport & transport);
    virtual Transport DetachTransport();

    virtual void SetExternalTransport(const OpalTransportAddressArray & transports);

    virtual OpalMediaStream * CreateMediaStream(
      const OpalMediaFormat & mediaFormat, 
      unsigned sessionID, 
      bool isSource
    );
  //@}

  /**@name Operations */
  //@{
    /**Sets the size of the jitter buffer to be used by this RTP session.
       A session defaults to not having any jitter buffer enabled for reading
       and the ReadBufferedData() function simply calls ReadData().

       If either jitter delay parameter is zero, it destroys the jitter buffer
       attached to this RTP session.
      */
    void SetJitterBufferSize(
      unsigned minJitterDelay, ///<  Minimum jitter buffer delay in RTP timestamp units
      unsigned maxJitterDelay, ///<  Maximum jitter buffer delay in RTP timestamp units
      unsigned timeUnits = 0,  ///<  Time Units, zero uses default
      PINDEX packetSize = 2048 ///<  Receive RTP packet size
    );

    /**Get current size of the jitter buffer.
       This returns the currently used jitter buffer delay in RTP timestamp
       units. It will be some value between the minimum and maximum set in
       the SetJitterBufferSize() function.
      */
    unsigned GetJitterBufferSize() const;
    unsigned GetJitterBufferDelay() const { return GetJitterBufferSize()/GetJitterTimeUnits(); }
    
    /**Get current time units of the jitter buffer.
     */
    unsigned GetJitterTimeUnits() const { return m_timeUnits; }

#if P_QOS
    /**Modifies the QOS specifications for this RTP session*/
    virtual bool ModifyQOS(RTP_QOS *);
#endif

    /**Read a data frame from the RTP channel.
       This function will conditionally read data from the jitter buffer or
       directly if there is no jitter buffer enabled.
      */
    virtual bool ReadData(
      RTP_DataFrame & frame   ///<  Frame read from the RTP session
    );

    /**Flush incoming data.
      */
    virtual void FlushData();

    /**Write a data frame from the RTP channel.
      */
    virtual bool WriteData(
      RTP_DataFrame & frame   ///<  Frame to write to the RTP session
    );

    /** Write data frame to the RTP channel outside the normal stream of media
      * Used for RFC2833 packets
      */
    virtual bool WriteOOBData(
      RTP_DataFrame & frame,
      bool rewriteTimeStamp = true
    );

    /**Write a control frame from the RTP channel.
      */
    virtual bool WriteControl(
      RTP_ControlFrame & frame    ///<  Frame to write to the RTP session
    );

   /**Restarts an existing session in the given direction.
      */
    virtual void Restart(
      bool isReading
    );

    /**Get the local host name as used in SDES packes.
      */
    virtual PString GetLocalHostName();

#if OPAL_STATISTICS
    virtual void GetStatistics(OpalMediaStatistics & statistics, bool receiver) const;
#endif
  //@}

  /**@name Call back functions */
  //@{
    enum SendReceiveStatus {
      e_ProcessPacket,
      e_IgnorePacket,
      e_AbortTransport
    };
    virtual SendReceiveStatus OnSendData(RTP_DataFrame & frame);
    virtual SendReceiveStatus OnSendControl(RTP_ControlFrame & frame, PINDEX & len);
    virtual SendReceiveStatus OnReceiveData(RTP_DataFrame & frame);
    virtual SendReceiveStatus OnReceiveControl(RTP_ControlFrame & frame);

    class ReceiverReport : public PObject  {
        PCLASSINFO(ReceiverReport, PObject);
      public:
        void PrintOn(ostream &) const;

        DWORD sourceIdentifier;
        DWORD fractionLost;         /* fraction lost since last SR/RR */
        DWORD totalLost;	    /* cumulative number of packets lost (signed!) */
        DWORD lastSequenceNumber;   /* extended last sequence number received */
        DWORD jitter;               /* interarrival jitter */
        PTimeInterval lastTimestamp;/* last SR packet from this source */
        PTimeInterval delay;        /* delay since last SR packet */
    };
    PARRAY(ReceiverReportArray, ReceiverReport);

    class SenderReport : public PObject  {
        PCLASSINFO(SenderReport, PObject);
      public:
        void PrintOn(ostream &) const;

        DWORD sourceIdentifier;
        PTime realTimestamp;
        DWORD rtpTimestamp;
        DWORD packetsSent;
        DWORD octetsSent;
    };

    virtual void OnRxSenderReport(const SenderReport & sender,
                                  const ReceiverReportArray & reports);
    virtual void OnRxReceiverReport(DWORD src,
                                    const ReceiverReportArray & reports);
    virtual void OnReceiverReports(const ReceiverReportArray & reports);

    class SourceDescription : public PObject  {
        PCLASSINFO(SourceDescription, PObject);
      public:
        SourceDescription(DWORD src) { sourceIdentifier = src; }
        void PrintOn(ostream &) const;

        DWORD            sourceIdentifier;
        POrdinalToString items;
    };
    PARRAY(SourceDescriptionArray, SourceDescription);
    virtual void OnRxSourceDescription(const SourceDescriptionArray & descriptions);

    virtual void OnRxGoodbye(const PDWORDArray & sources,
                             const PString & reason);

    virtual void OnRxApplDefined(const PString & type, unsigned subtype, DWORD src,
                                 const BYTE * data, PINDEX size);

#if OPAL_RTCP_XR
    class ExtendedReport : public PObject  {
        PCLASSINFO(ExtendedReport, PObject);
      public:
        void PrintOn(ostream &) const;

        DWORD sourceIdentifier;
        DWORD lossRate;            /* fraction of RTP data packets lost */ 
        DWORD discardRate;         /* fraction of RTP data packets discarded */
        DWORD burstDensity;        /* fraction of RTP data packets within burst periods */
        DWORD gapDensity;          /* fraction of RTP data packets within inter-burst gaps */
        DWORD roundTripDelay;  /* the most recently calculated round trip time */    
        DWORD RFactor;            /* voice quality metric of the call */
        DWORD mosLQ;               /* MOS for listen quality */
        DWORD mosCQ;               /* MOS for conversational quality */
        DWORD jbNominal;      /* current nominal jitter buffer delay, in ms */ 
        DWORD jbMaximum;      /* current maximum jitter buffer delay, in ms */
        DWORD jbAbsolute;     /* current absolute maximum jitter buffer delay, in ms */
    };
    PARRAY(ExtendedReportArray, ExtendedReport);

    virtual void OnRxExtendedReport(DWORD src,
                                    const ExtendedReportArray & reports);                             
#endif
  //@}

  /**@name Member variable access */
  //@{
    /**Get the ID for the RTP session.
      */
    unsigned GetSessionID() const { return sessionID; }

    /**Set the ID for the RTP session.
      */
    void SetSessionID(unsigned id) { sessionID = id; }

    /**Get flag for is audio RTP.
      */
    bool IsAudio() const { return isAudio; }

    /**Set flag for RTP session is audio.
     */
    void SetAudio(
      bool aud    /// New audio indication flag
    ) { isAudio = aud; }

    /**Get the canonical name for the RTP session.
      */
    PString GetCanonicalName() const;

    /**Set the canonical name for the RTP session.
      */
    void SetCanonicalName(const PString & name);

    /**Get the tool name for the RTP session.
      */
    PString GetToolName() const;

    /**Set the tool name for the RTP session.
      */
    void SetToolName(const PString & name);

    /**Get the RTP header extension codes for the session.
      */
    RTPExtensionHeaders GetExtensionHeaders() const;

    /**Set the RTP header extension codes for the session.
      */
    void SetExtensionHeader(const RTPExtensionHeaders & ext);

    /**Get the source output identifier.
      */
    DWORD GetSyncSourceOut() const { return syncSourceOut; }

    /**Indicate if will ignore all but first received SSRC value.
      */
    bool AllowAnySyncSource() const { return allowAnySyncSource; }

    /**Indicate if will ignore all but first received SSRC value.
      */
    void SetAnySyncSource(
      bool allow    ///<  Flag for allow any SSRC values
    ) { allowAnySyncSource = allow; }

    /**Indicate if will ignore rtp payload type changes in received packets.
     */
    void SetIgnorePayloadTypeChanges(
      bool ignore   ///<  Flag to ignore payload type changes
    ) { ignorePayloadTypeChanges = ignore; }

    /**Get the time interval for sending RTCP reports in the session.
      */
    const PTimeInterval & GetReportTimeInterval() { return m_reportTimer.GetResetTime(); }

    /**Set the time interval for sending RTCP reports in the session.
      */
    void SetReportTimeInterval(
      const PTimeInterval & interval ///<  New time interval for reports.
    )  { m_reportTimer.RunContinuous(interval); }

    /**Get the interval for transmitter statistics in the session.
      */
    unsigned GetTxStatisticsInterval() { return txStatisticsInterval; }

    /**Set the interval for transmitter statistics in the session.
      */
    void SetTxStatisticsInterval(
      unsigned packets   ///<  Number of packets between callbacks
    );

    /**Get the interval for receiver statistics in the session.
      */
    unsigned GetRxStatisticsInterval() { return rxStatisticsInterval; }

    /**Set the interval for receiver statistics in the session.
      */
    void SetRxStatisticsInterval(
      unsigned packets   ///<  Number of packets between callbacks
    );

    /**Clear statistics
      */
    void ClearStatistics();

    /**Get local address of session.
      */
    virtual PIPSocket::Address GetLocalAddress() const { return localAddress; }

    /**Get remote address of session.
      */
    PIPSocket::Address GetRemoteAddress() const { return remoteAddress; }

    /**Get local data port of session.
      */
    virtual WORD GetLocalDataPort() const { return localDataPort; }

    /**Get local control port of session.
      */
    virtual WORD GetLocalControlPort() const { return localControlPort; }

    /**Get remote data port of session.
      */
    virtual WORD GetRemoteDataPort() const { return remoteDataPort; }

    /**Get remote control port of session.
      */
    virtual WORD GetRemoteControlPort() const { return remoteControlPort; }

    /**Get data UDP socket of session.
      */
    virtual PUDPSocket & GetDataSocket() { return *dataSocket; }

    /**Get control UDP socket of session.
      */
    virtual PUDPSocket & GetControlSocket() { return *controlSocket; }

    /**Get total number of packets sent in session.
      */
    DWORD GetPacketsSent() const { return packetsSent; }

    /**Get total number of octets sent in session.
      */
    DWORD GetOctetsSent() const { return octetsSent; }

    /**Get total number of packets received in session.
      */
    DWORD GetPacketsReceived() const { return packetsReceived; }

    /**Get total number of octets received in session.
      */
    DWORD GetOctetsReceived() const { return octetsReceived; }

    /**Get total number received packets lost in session.
      */
    DWORD GetPacketsLost() const { return packetsLost; }

    /**Get total number transmitted packets lost by remote in session.
       Determined via RTCP.
      */
    DWORD GetPacketsLostByRemote() const { return packetsLostByRemote; }

    /**Get total number of packets received out of order in session.
      */
    DWORD GetPacketsOutOfOrder() const { return packetsOutOfOrder; }

    /**Get total number received packets too late to go into jitter buffer.
      */
    DWORD GetPacketsTooLate() const;

    /**Get total number received packets that could not fit into the jitter buffer.
      */
    DWORD GetPacketOverruns() const;

    /**Get average time between sent packets.
       This is averaged over the last txStatisticsInterval packets and is in
       milliseconds.
      */
    DWORD GetAverageSendTime() const { return averageSendTime; }

    /**Get the number of marker packets received this session.
       This can be used to find out the number of frames received in a video
       RTP stream.
      */
    DWORD GetMarkerRecvCount() const { return markerRecvCount; }

    /**Get the number of marker packets sent this session.
       This can be used to find out the number of frames sent in a video
       RTP stream.
      */
    DWORD GetMarkerSendCount() const { return markerSendCount; }

    /**Get maximum time between sent packets.
       This is over the last txStatisticsInterval packets and is in
       milliseconds.
      */
    DWORD GetMaximumSendTime() const { return maximumSendTime; }

    /**Get minimum time between sent packets.
       This is over the last txStatisticsInterval packets and is in
       milliseconds.
      */
    DWORD GetMinimumSendTime() const { return minimumSendTime; }

    /**Get average time between received packets.
       This is averaged over the last rxStatisticsInterval packets and is in
       milliseconds.
      */
    DWORD GetAverageReceiveTime() const { return averageReceiveTime; }

    /**Get maximum time between received packets.
       This is over the last rxStatisticsInterval packets and is in
       milliseconds.
      */
    DWORD GetMaximumReceiveTime() const { return maximumReceiveTime; }

    /**Get minimum time between received packets.
       This is over the last rxStatisticsInterval packets and is in
       milliseconds.
      */
    DWORD GetMinimumReceiveTime() const { return minimumReceiveTime; }

    enum { JitterRoundingGuardBits = 4 };
    /**Get averaged jitter time for received packets.
       This is the calculated statistical variance of the interarrival
       time of received packets in milliseconds.
      */
    DWORD GetAvgJitterTime() const { return (jitterLevel>>JitterRoundingGuardBits)/GetJitterTimeUnits(); }

    /**Get averaged jitter time for received packets.
       This is the maximum value of jitterLevel for the session.
      */
    DWORD GetMaxJitterTime() const { return (maximumJitterLevel>>JitterRoundingGuardBits)/GetJitterTimeUnits(); }

    /**Get jitter time for received packets on remote.
       This is the calculated statistical variance of the interarrival
       time of received packets in milliseconds.
      */
    DWORD GetJitterTimeOnRemote() const { return jitterLevelOnRemote/GetJitterTimeUnits(); }
  //@}

    virtual void SetCloseOnBYE(bool v)  { closeOnBye = v; }

    /**Send flow control (Temporary Maximum Media Stream Bit Rate) Request/Notification.
      */
    virtual void SendFlowControl(
      unsigned maxBitRate,    ///< New temporary maximum bit rate
      unsigned overhead = 0,  ///< Protocol overhead, defaults to IP/UDP/RTP header size
      bool notify = false     ///< Send request/notification
    );

#if OPAL_VIDEO
    /** Tell the rtp session to send out an intra frame request control packet.
        This is called when the media stream receives an OpalVideoUpdatePicture
        media command.
      */
    virtual void SendIntraFrameRequest(bool rfc2032, bool pictureLoss);

    /** Tell the rtp session to send out an temporal spatial trade off request
        control packet. This is called when the media stream receives an
        OpalTemporalSpatialTradeOff media command.
      */
    virtual void SendTemporalSpatialTradeOff(unsigned tradeOff);
#endif

    void SetNextSentSequenceNumber(WORD num) { lastSentSequenceNumber = (WORD)(num-1); }

    DWORD GetSyncSourceIn() const { return syncSourceIn; }

    typedef PNotifierTemplate<SendReceiveStatus &> FilterNotifier;
    #define PDECLARE_RTPFilterNotifier(cls, fn) PDECLARE_NOTIFIER2(RTP_DataFrame, cls, fn, OpalRTPSession::SendReceiveStatus &)
    #define PCREATE_RTPFilterNotifier(fn) PCREATE_NOTIFIER2(fn, OpalRTPSession::SendReceiveStatus &)

    void AddFilter(const FilterNotifier & filter);

#if OPAL_RTCP_XR
    const RTCP_XR_Metrics & GetMetrics() const { return m_metrics; }
          RTCP_XR_Metrics & GetMetrics()       { return m_metrics; }
#endif

    virtual void SendBYE();

  protected:
    void AddReceiverReport(RTP_ControlFrame::ReceiverReport & receiver);
    bool InsertReportPacket(RTP_ControlFrame & report);
    virtual int WaitForPDU(PUDPSocket & dataSocket, PUDPSocket & controlSocket, const PTimeInterval & timer);
    virtual SendReceiveStatus ReadDataPDU(RTP_DataFrame & frame);
    virtual SendReceiveStatus OnReadTimeout(RTP_DataFrame & frame);
    
#if OPAL_RTCP_XR
    void InsertExtendedReportPacket(RTP_ControlFrame & report);
    void OnRxSenderReportToMetrics(const RTP_ControlFrame & frame, PINDEX offset);    
#endif

    bool InternalSetRemoteAddress(PIPSocket::Address address, WORD port, bool isDataPort);
    virtual void ApplyQOS(const PIPSocket::Address & addr);
    virtual bool InternalReadData(RTP_DataFrame & frame);
    virtual SendReceiveStatus InternalReadData2(RTP_DataFrame & frame);
    virtual SendReceiveStatus ReadControlPDU();
    virtual SendReceiveStatus ReadDataOrControlPDU(
      BYTE * framePtr,
      PINDEX frameSize,
      bool fromDataChannel
    );

    virtual bool WriteDataOrControlPDU(
      const BYTE * framePtr,
      PINDEX frameSize,
      bool toDataChannel
    );


    unsigned            sessionID;
    bool                isAudio;
    unsigned            m_timeUnits;
    PString             canonicalName;
    PString             toolName;
    RTPExtensionHeaders m_extensionHeaders;

    typedef PSafePtr<RTP_JitterBuffer, PSafePtrMultiThreaded> JitterBufferPtr;
    JitterBufferPtr m_jitterBuffer;

    DWORD         syncSourceOut;
    DWORD         syncSourceIn;
    DWORD         lastSentTimestamp;
    bool          allowAnySyncSource;
    bool          allowOneSyncSourceChange;
    bool          allowRemoteTransmitAddressChange;
    bool          allowSequenceChange;
    unsigned      txStatisticsInterval;
    unsigned      rxStatisticsInterval;
    WORD          lastSentSequenceNumber;
    WORD          expectedSequenceNumber;
    PTimeInterval lastSentPacketTime;
    PTimeInterval lastReceivedPacketTime;
    PTime         lastSRTimestamp;
    PTime         lastSRReceiveTime;
    PTimeInterval delaySinceLastSR;
    WORD          lastRRSequenceNumber;
    bool          resequenceOutOfOrderPackets;
    unsigned      consecutiveOutOfOrderPackets;
    PTimeInterval outOfOrderWaitTime;
    PTimeInterval outOfOrderPacketTime;

    std::list<RTP_DataFrame> m_outOfOrderPackets;
    void SaveOutOfOrderPacket(RTP_DataFrame & frame);

    PMutex        dataMutex;
    DWORD         timeStampOffs;               // offset between incoming media timestamp and timeStampOut
    bool          oobTimeStampBaseEstablished; // true if timeStampOffs has been established by media
    DWORD         oobTimeStampOutBase;         // base timestamp value for oob data
    PTimeInterval oobTimeStampBase;            // base time for oob timestamp

    // Statistics
    PTime firstPacketSent;
    DWORD packetsSent;
    DWORD rtcpPacketsSent;
    DWORD octetsSent;
    PTime firstPacketReceived;
    DWORD packetsReceived;
    DWORD senderReportsReceived;
    DWORD octetsReceived;
    DWORD packetsLost;
    DWORD packetsLostByRemote;
    DWORD packetsOutOfOrder;
    DWORD averageSendTime;
    DWORD maximumSendTime;
    DWORD minimumSendTime;
    DWORD averageReceiveTime;
    DWORD maximumReceiveTime;
    DWORD minimumReceiveTime;
    DWORD jitterLevel;
    DWORD jitterLevelOnRemote;
    DWORD maximumJitterLevel;

    DWORD m_syncTimestamp;
    PTime m_syncRealTime;

    DWORD markerSendCount;
    DWORD markerRecvCount;

    unsigned txStatisticsCount;
    unsigned rxStatisticsCount;
    
#if OPAL_RTCP_XR
    // Calculate the VoIP Metrics for RTCP-XR
    RTCP_XR_Metrics m_metrics;
#endif

    DWORD    averageSendTimeAccum;
    DWORD    maximumSendTimeAccum;
    DWORD    minimumSendTimeAccum;
    DWORD    averageReceiveTimeAccum;
    DWORD    maximumReceiveTimeAccum;
    DWORD    minimumReceiveTimeAccum;
    DWORD    packetsLostSinceLastRR;
    DWORD    lastTransitTime;
    
    RTP_DataFrame::PayloadTypes lastReceivedPayloadType;
    bool ignorePayloadTypeChanges;

    PMutex m_reportMutex;
    PTimer m_reportTimer;
    PDECLARE_NOTIFIER(PTimer, OpalRTPSession, SendReport);

    bool closeOnBye;
    bool byeSent;

    list<FilterNotifier> m_filters;

    PIPSocket::Address localAddress;
    WORD               localDataPort;
    WORD               localControlPort;

    PIPSocket::Address remoteAddress;
    WORD               remoteDataPort;
    WORD               remoteControlPort;

    PIPSocket::Address remoteTransmitAddress;

    PUDPSocket * dataSocket;
    PUDPSocket * controlSocket;

    bool shutdownRead;
    bool shutdownWrite;
    bool appliedQOS;
    bool remoteIsNAT;
    bool localHasNAT;
    bool m_firstControl;
    int  badTransmitCounter;
    PTime badTransmitStart;

  private:
    OpalRTPSession(const OpalRTPSession & other);
    void operator=(const OpalRTPSession &) { }

  friend class RTP_JitterBuffer;
};


//typedef OpalRTPSession RTP_Session; // Backward compatibility
//typedef OpalRTPSession RTP_UDP; // Backward compatibility


/////////////////////////////////////////////////////////////////////////////

#if 0
class SecureRTP_UDP : public OpalRTPSession
{
  PCLASSINFO(SecureRTP_UDP, OpalRTPSession);

  public:
  /**@name Construction */
  //@{
    /**Create a new RTP channel.
     */
    SecureRTP_UDP(
      OpalConnection & connection,    ///< Connection that owns the sesion
      unsigned sessionId,             ///< Unique (in connection) session ID for session
      const OpalMediaType & mediaType ///< Media type for session
    );

    /// Destroy the RTP
    ~SecureRTP_UDP();

    virtual void SetSecurityMode(OpalSecurityMode * srtpParms);  
    virtual OpalSecurityMode * GetSecurityParms() const;

  protected:
    OpalSecurityMode * securityParms;
};
#endif

#endif // OPAL_RTP_RTP_H

/////////////////////////////////////////////////////////////////////////////
