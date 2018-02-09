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
 */

#ifndef OPAL_RTP_RTP_H
#define OPAL_RTP_RTP_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <opal_config.h>

#include <ptclib/url.h>

#include <set>


typedef uint32_t RTP_Timestamp;
typedef uint16_t RTP_SequenceNumber;
typedef uint32_t RTP_SyncSourceId;
typedef std::vector<RTP_SyncSourceId> RTP_SyncSourceArray;
class RTP_ReceiverReport;
class RTP_SenderReport;


///////////////////////////////////////////////////////////////////////////////
// Real Time Protocol - IETF RFC1889 and RFC1890

class RTP_SourceDescription : public PObject
{
    PCLASSINFO(RTP_SourceDescription, PObject);
  public:
    RTP_SourceDescription(RTP_SyncSourceId src) { sourceIdentifier = src; }
#if PTRACING
    void PrintOn(ostream &) const;
#endif

    RTP_SyncSourceId sourceIdentifier;
    POrdinalToString items;
};

typedef PArray<RTP_SourceDescription> RTP_SourceDescriptionArray;


class RTP_TransportWideCongestionControl : public PObject
{
    PCLASSINFO(RTP_TransportWideCongestionControl, PObject);
  public:
    RTP_TransportWideCongestionControl();

    struct Info
    {
      Info(const PTimeInterval & ts = 0, unsigned id = 0, RTP_SyncSourceId ssrc = 0)
        : m_timestamp(ts)
        , m_sessionID(id)
        , m_SSRC(ssrc)
      { }

      PTimeInterval    m_timestamp;  ///< Time relative to an arbitrary moment in time
      unsigned         m_sessionID;  ///< Session ID we sent packet on, unused on rx RTCP
      RTP_SyncSourceId m_SSRC;       ///< SSRC we sent packet with, unused on rx RTCP
    };
    /* Note, map index is (effectively) a 17 bit transport wide sequence
       number, even though the over the wire value is 16 bit, as wraparound
       is compensated for to make sure the correct order in the map of the
       packets is maintained. */
    typedef std::map<unsigned, Info> PacketMap;
    PacketMap m_packets;            ///< Info of each packet that was sent
    unsigned  m_rtcpSequenceNumber; ///< RTCP sequence number, note, lower 8 bits only are significant
};


/**An RTP control frame encapsulation.
  */
class RTP_ControlFrame : public PBYTEArray
{
  PCLASSINFO(RTP_ControlFrame, PBYTEArray);

  public:
    RTP_ControlFrame(PINDEX compoundSize = 2048);
    RTP_ControlFrame(const BYTE * data, PINDEX size, bool dynamic = true);

#if PTRACING
    virtual void PrintOn(ostream &) const;
#endif

    bool IsValid() const;

    unsigned GetVersion() const { return (BYTE)theArray[m_compoundOffset]>>6; }

    unsigned GetCount() const { return (BYTE)theArray[m_compoundOffset]&0x1f; }
    void     SetCount(unsigned count);

    RTP_SyncSourceId GetSenderSyncSource() const { return *(PUInt32b *)(theArray + 4); } // Always first DWORD in the first payload

    enum PayloadTypes {
      e_FirstValidPayloadType   = 192, // RFC5761
      e_IntraFrameRequest       = 192,
      e_SenderReport            = 200,
      e_ReceiverReport          = 201,
      e_SourceDescription       = 202,
      e_Goodbye                 = 203,
      e_ApplDefined             = 204,
      e_TransportLayerFeedBack  = 205, // RFC4585
      e_PayloadSpecificFeedBack = 206,
      e_ExtendedReport          = 207, // RFC3611
      e_LastValidPayloadType    = 223  // RFC5761
    };

    PayloadTypes GetPayloadType() const { return (PayloadTypes)(BYTE)theArray[m_compoundOffset+1]; }
    void         SetPayloadType(PayloadTypes pt);

    PINDEX GetPayloadSize() const { return 4*(*(PUInt16b *)&theArray[m_compoundOffset+2]); }
    bool   SetPayloadSize(PINDEX sz);

    BYTE * GetPayloadPtr() const;

    bool ReadNextPacket();
    bool StartNewPacket(PayloadTypes pt);
    void EndPacket();

    PINDEX GetPacketSize() const { return m_packetSize; }
    bool SetPacketSize(PINDEX size);

    bool ParseGoodbye(RTP_SyncSourceId & ssrc, RTP_SyncSourceArray & csrc, PString & msg);

#pragma pack(1)
    struct ReceiverReport {
      PUInt32b ssrc;      /* data source being reported */
      BYTE     fraction;  /* fraction lost since last SR/RR */
      BYTE     lost[3];	  /* cumulative number of packets lost (signed!) */
      PUInt32b last_seq;  /* extended last sequence number received */
      PUInt32b jitter;    /* interarrival jitter */
      PUInt32b lsr;       /* last SR packet from this source */
      PUInt32b dlsr;      /* delay since last SR packet */

      int GetLostPackets() const;
      void SetLostPackets(int packets);
    };

    bool ParseReceiverReport(
      RTP_SyncSourceId & ssrc,
      const ReceiverReport * & rr,
      unsigned & count
    );
    ReceiverReport * AddReceiverReport(
      RTP_SyncSourceId ssrc,
      unsigned receivers
    );

    struct SenderReport {
      PUInt32b ssrc;
      PUInt64b ntp_ts;    /* NTP timestamp */
      PUInt32b rtp_ts;    /* RTP timestamp */
      PUInt32b psent;     /* packets sent */
      PUInt32b osent;     /* octets sent */ 
    };

    bool ParseSenderReport(
      RTP_SenderReport & txReport,
      const ReceiverReport * & rr,
      unsigned & count
    );
    ReceiverReport * AddSenderReport(
      RTP_SyncSourceId ssrc,
      const PTime & ntp,
      RTP_Timestamp ts,
      unsigned packets,
      uint64_t octets,
      unsigned receivers
    );

    struct ExtendedReport {
      BYTE     bt;                 /* block type */
      BYTE     type_specific;      /* determined by the block definition */
      PUInt16b length;             /* length of the report block */
    };

    struct ReceiverReferenceTimeReport : ExtendedReport {
      PUInt64b ntp;
    };
    void AddReceiverReferenceTimeReport(
      RTP_SyncSourceId ssrc,
      const PTime & ntp
    );

    struct DelayLastReceiverReport : ExtendedReport {
      struct Receiver {
        PUInt32b ssrc;
        PUInt32b lrr;
        PUInt32b dlrr;
      } m_receiver[1];
    };
    DelayLastReceiverReport::Receiver * AddDelayLastReceiverReport(
      RTP_SyncSourceId ssrc,
      unsigned receivers
    );
    static void AddDelayLastReceiverReport(
      DelayLastReceiverReport::Receiver & dlrr,
      RTP_SyncSourceId ssrc,
      const PTime & ntp,
      const PTimeInterval & delay
    );


#if OPAL_RTCP_XR
    struct MetricsReport : ExtendedReport {
      /* VoIP Metrics Report Block */
      PUInt32b ssrc;               /* data source being reported */
      BYTE     loss_rate;          /* fraction of RTP data packets lost */ 
      BYTE     discard_rate;       /* fraction of RTP data packets discarded */
      BYTE     burst_density;      /* fraction of RTP data packets within burst periods */
      BYTE     gap_density;        /* fraction of RTP data packets within inter-burst gaps */
      PUInt16b burst_duration;     /* the mean duration, in ms, of burst periods */
      PUInt16b gap_duration;       /* the mean duration, in ms, of gap periods */
      PUInt16b round_trip_delay;   /* the most recently calculated round trip time */    
      PUInt16b end_system_delay;   /* the most recently estimates end system delay */
      BYTE     signal_level;       /* voice signal level related to 0 dBm */
      BYTE     noise_level;        /* ratio of the silent background level to 0 dBm */
      BYTE     rerl;               /* residual echo return loss */
      BYTE     gmin;               /* gap threshold */
      BYTE     r_factor;           /* voice quality metric of the call */
      BYTE     ext_r_factor;       /* external R factor */
      BYTE     mos_lq;             /* MOS for listen quality */
      BYTE     mos_cq;             /* MOS for conversational quality */
      BYTE     rx_config;          /* receiver configuration byte */
      BYTE     reserved;           /* reserved for future definition */
      PUInt16b jb_nominal;         /* current nominal jitter buffer delay, in ms */ 
      PUInt16b jb_maximum;         /* current maximum jitter buffer delay, in ms */
      PUInt16b jb_absolute;        /* current absolute maximum jitter buffer delay, in ms */
    };
#endif


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
      RTP_SyncSourceId src   ///<  SSRC/CSRC identifier
    );

    void AddSourceDescriptionItem(
      unsigned type,            ///<  Description type
      const PString & data      ///<  Data for description
    );

    bool ParseSourceDescriptions(
      RTP_SourceDescriptionArray & descriptions
    );
    void AddSourceDescription(
      RTP_SyncSourceId ssrc,
      const PString & cname,
      const PString & toolName,
      bool endPacket = true
    );

    // Add RFC2032 Intra Frame Request
    void AddIFR(
      RTP_SyncSourceId syncSourceIn
    );


    // RFC4585 Feedback Message Type (FMT)
    struct FbHeader {
      PUInt32b senderSSRC;  /* data source of sender of message */
      PUInt32b mediaSSRC;   /* data source of media */
    };

    unsigned GetFbType() const { return (BYTE)theArray[m_compoundOffset]&0x1f; }

    FbHeader * AddFeedback(PayloadTypes pt, unsigned type, PINDEX fciSize);
    template <typename FB> void AddFeedback(PayloadTypes pt, unsigned type, FB * & data) { data = (FB *)AddFeedback(pt, type, sizeof(FB)); }

    // RFC4585 transport layer
    enum TransportLayerFbTypes {
      e_TransportNACK = 1,
      e_TMMBR = 3,
      e_TMMBN,
      e_TWCC = 15
    };

    struct FbNACK : FbHeader {
      struct Field
      {
        PUInt16b packetID;
        PUInt16b bitmask;
      } fld[1];
    };
    struct LostPacketMask : std::set<RTP_SequenceNumber>
    {
      friend ostream & operator<<(ostream & strm, const LostPacketMask & mask);
    };
    void AddNACK(
      RTP_SyncSourceId syncSourceOut,
      RTP_SyncSourceId syncSourceIn,
      const LostPacketMask & lostPackets
    );
    bool ParseNACK(
      RTP_SyncSourceId & senderSSRC,
      RTP_SyncSourceId & targetSSRC,
      LostPacketMask & lostPackets
    );

    // Same for request (e_TMMBR) and notification (e_TMMBN)
    struct FbTMMB : FbHeader {
      PUInt32b requestSSRC;
      PUInt32b bitRateAndOverhead; // Various bit fields
    };
    void AddTMMB(
      RTP_SyncSourceId syncSourceOut,
      RTP_SyncSourceId syncSourceIn,
      unsigned maxBitRate,
      unsigned overhead,
      bool notify
    );
    bool ParseTMMB(
      RTP_SyncSourceId & senderSSRC,
      RTP_SyncSourceId & targetSSRC,
      unsigned & maxBitRate,
      unsigned & overhead
    );

    struct FbTWCC : FbHeader {
      PUInt16b baseSN;
      PUInt16b statusCount;
      uint8_t  referenceTime[3];
      uint8_t  rtcpSN;
    };
    void AddTWCC(
      RTP_SyncSourceId syncSourceOut,
      const RTP_TransportWideCongestionControl & info
    );
    bool ParseTWCC(
      RTP_SyncSourceId & senderSSRC,
      RTP_TransportWideCongestionControl & info
    );

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

    void AddPLI(
      RTP_SyncSourceId syncSourceOut,
      RTP_SyncSourceId syncSourceIn
    );
    bool ParsePLI(
      RTP_SyncSourceId & senderSSRC,
      RTP_SyncSourceId & targetSSRC
    );

    struct FbFIR : FbHeader {
      PUInt32b requestSSRC;
      BYTE     sequenceNumber;
    };
    void AddFIR(
      RTP_SyncSourceId syncSourceOut,
      RTP_SyncSourceId syncSourceIn,
      unsigned sequenceNumber
    );
    bool ParseFIR(
      RTP_SyncSourceId & senderSSRC,
      RTP_SyncSourceId & targetSSRC,
      unsigned & sequenceNumber
    );

    struct FbTSTO : FbHeader {
      PUInt32b requestSSRC;
      BYTE     sequenceNumber;
      BYTE     reserver[2];
      BYTE     tradeOff;
    };
    void AddTSTO(
      RTP_SyncSourceId syncSourceOut,
      RTP_SyncSourceId syncSourceIn,
      unsigned tradeOff,
      unsigned sequenceNumber
    );
    bool ParseTSTO(
      RTP_SyncSourceId & senderSSRC,
      RTP_SyncSourceId & targetSSRC,
      unsigned & tradeOff,
      unsigned & sequenceNumber
    );

    struct FbREMB : FbHeader {
      char     id[4]; // 'R' 'E' 'M' 'B'
      BYTE     numSSRC;
      BYTE     bitRate[3];
      PUInt32b feedbackSSRC[1];
    };
    void AddREMB(
      RTP_SyncSourceId syncSourceOut,
      RTP_SyncSourceId syncSourceIn,
      unsigned maxBitRate
    );
    bool ParseREMB(
      RTP_SyncSourceId & senderSSRC,
      RTP_SyncSourceArray & targetSSRCs,
      unsigned & maxBitRate
    );

#pragma pack()

    struct ApplDefinedInfo {
      char             m_type[5];
      unsigned         m_subType;
      RTP_SyncSourceId m_SSRC;
      PBYTEArray       m_data;

      ApplDefinedInfo(const char * type = NULL, unsigned subType = 0, RTP_SyncSourceId ssrc = 0, const BYTE * data = NULL, PINDEX size = 0);
    };
    void AddApplDefined(const ApplDefinedInfo & info);
    bool ParseApplDefined(ApplDefinedInfo & info);

  protected:
    PINDEX m_packetSize;
    PINDEX m_compoundOffset;
    PINDEX m_payloadSize;

  private:
    virtual PBoolean SetSize(PINDEX sz) { return PBYTEArray::SetSize(sz); }
};


class RTP_SenderReport : public PObject
{
    PCLASSINFO(RTP_SenderReport, PObject);
  public:
    RTP_SenderReport();
    RTP_SenderReport(const RTP_ControlFrame::SenderReport & report);
#if PTRACING
    void PrintOn(ostream &) const;
#endif

    RTP_SyncSourceId sourceIdentifier;
    uint64_t         ntpPassThrough;
    PTime            realTimestamp;
    RTP_Timestamp    rtpTimestamp;
    unsigned         packetsSent;
    unsigned         octetsSent;
};


class RTP_ReceiverReport : public PObject
{
    PCLASSINFO(RTP_ReceiverReport, PObject);
  public:
    RTP_ReceiverReport(const RTP_ControlFrame::ReceiverReport & report, uint64_t ntpPassThru);
#if PTRACING
    void PrintOn(ostream &) const;
#endif

    RTP_SyncSourceId sourceIdentifier;     /* data source being reported */
    unsigned         fractionLost;         /* fraction lost since last SR/RR */
    unsigned         totalLost;	          /* cumulative number of packets lost (signed!) */
    unsigned         lastSequenceNumber;   /* extended last sequence number received */
    unsigned         jitter;               /* interarrival jitter */
    PTime            lastTimestamp;        /* last SR time from this source */
    PTimeInterval    delay;                /* delay since last SR packet */
};


class RTP_DelayLastReceiverReport : public PObject
{
    PCLASSINFO(RTP_DelayLastReceiverReport, PObject);
  public:
    RTP_DelayLastReceiverReport(const RTP_ControlFrame::DelayLastReceiverReport::Receiver & dlrr);
#if PTRACING
    void PrintOn(ostream &) const;
#endif

    RTP_SyncSourceId m_ssrc;
    PTime            m_lastTimestamp;        /* last RR packet from this source */
    PTimeInterval    m_delay;                /* delay since last RR packet */
};


/**An RTP data frame encapsulation.
  */
class RTP_DataFrame : public PBYTEArray
{
  PCLASSINFO(RTP_DataFrame, PBYTEArray);

  public:
    RTP_DataFrame(PINDEX payloadSize = 0, PINDEX bufferSize = 0);
    RTP_DataFrame(const BYTE * data, PINDEX len, bool dynamic = true);
    RTP_DataFrame(const PBYTEArray data);

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

      StartConflictRTCP = RTP_ControlFrame::e_FirstValidPayloadType&0x7f,
      EndConflictRTCP = RTP_ControlFrame::e_LastValidPayloadType&0x7f,

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

    PINDEX GetPaddingSize() const { return m_paddingSize > 0 ? m_paddingSize-1 : 0; }
    bool   SetPaddingSize(PINDEX sz);

    PayloadTypes GetPayloadType() const { return (PayloadTypes)(theArray[1]&0x7f); }
    void         SetPayloadType(PayloadTypes t);

    RTP_SequenceNumber GetSequenceNumber() const { return *(PUInt16b *)&theArray[2]; }
    void SetSequenceNumber(RTP_SequenceNumber n) { *(PUInt16b *)&theArray[2] = n; }

    RTP_Timestamp GetTimestamp() const  { return *(PUInt32b *)&theArray[4]; }
    void  SetTimestamp(RTP_Timestamp t) { *(PUInt32b *)&theArray[4] = t; }

    RTP_SyncSourceId GetSyncSource() const  { return *(PUInt32b *)&theArray[8]; }
    void  SetSyncSource(RTP_SyncSourceId s) { *(PUInt32b *)&theArray[8] = s; }

    PINDEX GetContribSrcCount() const { return theArray[0]&0xf; }
    RTP_SyncSourceId  GetContribSource(PINDEX idx) const;
    void   SetContribSource(PINDEX idx, RTP_SyncSourceId src);

    PINDEX GetHeaderSize() const { return m_headerSize; }

    void CopyHeader(const RTP_DataFrame & other);
    void Copy(const RTP_DataFrame & other);

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

    /// Extension header types
    enum HeaderExtensionType {
      RFC3550,
      RFC5285_OneByte,
      RFC5285_TwoByte
    };

    static const unsigned MaxHeaderExtensionId = 65535;
    static const unsigned MaxHeaderExtensionIdOneByte = 14;
    static const unsigned MaxHeaderExtensionIdTwoByte = 255;

    /**Get header extension by specified id.
       @returns NULL if no extension of that id and type is present.
      */
    BYTE * GetHeaderExtension(
      HeaderExtensionType type, ///< Type of extension to get
      unsigned id,              ///< Identifier for extension
      PINDEX & length           ///< Length of extension in bytes
    ) const;

    /**Set header extension.
       Note when RFC 5285 formats are used, the extension is appended to ones
       already present. An existing extension of the same ID will be overwritten,
       if it is the same size. If not the same size, an assert is thrown.

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
    bool   SetPayload(const BYTE * data, PINDEX sz);
    BYTE * GetPayloadPtr()     const { return (BYTE *)(theArray+m_headerSize); }

    virtual PObject * Clone() const { return new RTP_DataFrame(*this); }
#if PTRACING
    virtual void PrintOn(ostream & strm) const;
#endif

    // Note this sets the whole packet length, and calculates the various
    // sub-section sizes: header payload and padding.
    bool SetPacketSize(PINDEX sz);

    // Get the packet size including headers, padding etc.
    PINDEX GetPacketSize() const;

    struct MetaData
    {
        MetaData();

        // Note the first two times here are not synchronised with the local CPU clock, but the remote systems.
        PTime    m_absoluteTime; // Wall clock time media was sampled, as calculated via RTCP and timestamp
        PTime    m_transmitTime; // Wall clock time packet was transmitted (calculated from header extensions)
        PTime    m_receivedTime; // Wall clock time packet physically read from socket
        unsigned m_discontinuity;
        PString  m_lipSyncId;
    };

    /**Get meta data for RTP packet.
      */
    const MetaData & GetMetaData() const { return m_metaData; }

    /**Set meta data for RTP packet.
      */
    void SetMetaData(const MetaData & metaData) { m_metaData = metaData; }

    /**Get absolute (wall clock) time of packet, as calculated via RTCP and timestamp, if available.
      */
    PTime GetAbsoluteTime() const { return m_metaData.m_absoluteTime; }

    /**Set absolute (wall clock) time of media, as calculated via RTCP and timestamp.
      */
    void SetAbsoluteTime() { m_metaData.m_absoluteTime.SetCurrentTime(); }
    void SetAbsoluteTime(const PTime & t) { m_metaData.m_absoluteTime = t; }

    /**Set transmit (wall clock) time of packet transmission.
      */
    void SetTransmitTime() { m_metaData.m_transmitTime.SetCurrentTime(); }
    void SetTransmitTimeNTP(uint64_t ntp) { m_metaData.m_transmitTime.SetNTP(ntp); }

    /** Get sequence number discontinuity.
        If non-zero this indicates the number of packets detected as missing
        before this packet.
      */
    unsigned GetDiscontinuity() const { return m_metaData.m_discontinuity; }

    /** Set sequence number discontinuity.
      */
    void SetDiscontinuity(unsigned lost) { m_metaData.m_discontinuity = lost; }

    /** Get the identifier that links audio and video streams for
        "lip synch" purposes.
    */
    const PString & GetLipSyncId() const { return m_metaData.m_lipSyncId; }

    /** Set the identifier that links audio and video streams for
        "lip synch" purposes.
    */
    void SetLipSyncId(const PString & id) { m_metaData.m_lipSyncId = id; }

    // backward compatibility
    P_DEPRECATED const PString & GetBundleId() const { return m_metaData.m_lipSyncId; }
    P_DEPRECATED void SetBundleId(const PString & id) { m_metaData.m_lipSyncId = id; }

  protected:
    bool AdjustHeaderSize(PINDEX newSize);

    PINDEX   m_headerSize;
    PINDEX   m_payloadSize;
    PINDEX   m_paddingSize;
    MetaData m_metaData;

#if PTRACING
    friend ostream & operator<<(ostream & o, PayloadTypes t);
#endif

  private:
    virtual PBoolean SetSize(PINDEX sz) { return PBYTEArray::SetSize(sz); }
};

PLIST(RTP_DataFrameList, RTP_DataFrame);


///////////////////////////////////////////////////////////////////////////////

/** Information for RFC 5285 header extensions.
  */
class RTPHeaderExtensionInfo : public PObject
{
    PCLASSINFO(RTPHeaderExtensionInfo, PObject);
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

    RTPHeaderExtensionInfo();
    explicit RTPHeaderExtensionInfo(
      const PURL & uri,
      const PString & attributes = PString::Empty()
    );
    RTPHeaderExtensionInfo(
      unsigned id,
      const PURL & uri,
      const PString & attributes = PString::Empty()
    );

#if PTRACING
    virtual void PrintOn(ostream & strm) const;
#endif
    virtual Comparison Compare(const PObject & other) const;

#if OPAL_SDP
    bool ParseSDP(const PString & param);
    void OutputSDP(ostream & strm) const;
#endif
};

class RTPHeaderExtensions : public std::set<RTPHeaderExtensionInfo>
{
public:
  bool AddUniqueID(RTPHeaderExtensionInfo & info);
  bool Contains(const RTPHeaderExtensionInfo & info) const;
};


#if PTRACING
class RTP_TRACE_SRC
{
  public:
    RTP_TRACE_SRC(RTP_SyncSourceId src)
      : m_src(src)
    {
    }

    friend std::ostream & operator<<(std::ostream & strm, const RTP_TRACE_SRC & src)
    {
      return strm << src.m_src << " (0x" << std::hex << src.m_src << std::dec << ')';
    }

  protected:
    RTP_SyncSourceId m_src;
};
#endif // PTRACING

#endif // OPAL_RTP_RTP_H

/////////////////////////////////////////////////////////////////////////////
