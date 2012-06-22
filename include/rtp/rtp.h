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

#include <ptclib/url.h>

#include <set>


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


#endif // OPAL_RTP_RTP_H

/////////////////////////////////////////////////////////////////////////////
