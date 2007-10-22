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
 * $Log: rtp.h,v $
 * Revision 2.44  2007/09/05 13:06:44  csoutheren
 * Applied 1720918 - Track marker packets sent/received
 * Thanks to Josh Mahonin
 *
 * Revision 2.43  2007/07/26 00:38:56  csoutheren
 * Make transmission of RFC2833 independent of the media stream
 *
 * Revision 2.42  2007/05/29 06:26:00  csoutheren
 * Add Reset function so we can reload used control frames
 *
 * Revision 2.41  2007/05/14 10:44:09  rjongbloed
 * Added PrintOn function to RTP_DataFrame
 *
 * Revision 2.40  2007/04/19 06:34:12  csoutheren
 * Applied 1703206 - OpalVideoFastUpdatePicture over SIP
 * Thanks to Josh Mahonin
 *
 * Revision 2.39  2007/04/05 02:41:05  rjongbloed
 * Added ability to have non-dynamic allocation of memory in RTP data frames.
 *
 * Revision 2.38  2007/03/28 05:25:56  csoutheren
 * Add virtual function to wait for incoming data
 * Swallow RTP packets that arrive on the socket before session starts
 *
 * Revision 2.37  2007/03/20 02:31:56  csoutheren
 * Don't send BYE twice or when channel is closed
 *
 * Revision 2.36  2007/03/12 23:33:18  csoutheren
 * Added virtual to functions
 *
 * Revision 2.35  2007/03/08 04:36:05  csoutheren
 * Add flag to close RTP session when RTCP BYE received
 *
 * Revision 2.34  2007/03/01 03:31:02  csoutheren
 * Remove spurious zero bytes from the end of RTCP SDES packets
 * Fix format of RTCP BYE packets
 *
 * Revision 2.33  2007/02/23 08:06:04  csoutheren
 * More implementation of ZRTP (not yet complete)
 *
 * Revision 2.32  2007/02/12 02:44:27  csoutheren
 * Start of support for ZRTP
 *
 * Revision 2.32  2007/02/10 07:08:41  craigs
 * Start of support for ZRTP
 *
 * Revision 2.31  2007/02/01 06:43:19  csoutheren
 * Added virtual to functions
 *
 * Revision 2.30  2006/12/08 04:12:12  csoutheren
 * Applied 1589274 - better rtp error handling of malformed rtcp packet
 * Thanks to frederich
 *
 * Revision 2.29  2006/11/29 06:31:59  csoutheren
 * Add support fort RTP BYE
 *
 * Revision 2.28  2006/11/20 03:37:12  csoutheren
 * Allow optional inclusion of RTP aggregation
 *
 * Revision 2.27  2006/10/28 16:40:28  dsandras
 * Fixed SIP reinvite without breaking H.323 calls.
 *
 * Revision 2.26  2006/10/24 04:18:28  csoutheren
 * Added support for encrypted RTCP
 *
 * Revision 2.25  2006/09/05 06:18:22  csoutheren
 * Start bringing in SRTP code for libSRTP
 *
 * Revision 2.24  2006/08/29 18:39:21  dsandras
 * Added function to better deal with reinvites.
 *
 * Revision 2.23  2006/08/03 04:57:12  csoutheren
 * Port additional NAT handling logic from OpenH323 and extend into OpalConnection class
 *
 * Revision 2.22  2006/02/02 07:02:57  csoutheren
 * Added RTP payload map to transcoders and connections to allow remote SIP endpoints
 * to change the payload type used for outgoing RTP.
 *
 * Revision 2.21  2005/12/30 14:29:15  dsandras
 * Removed the assumption that the jitter will contain a 8 kHz signal.
 *
 * Revision 2.20  2005/11/30 13:35:26  csoutheren
 * Changed tags for Doxygen
 *
 * Revision 2.19  2005/04/11 17:34:57  dsandras
 * Added support for dynamic sequence changes in case of Re-INVITE.
 *
 * Revision 2.18  2005/04/10 20:50:16  dsandras
 * Allow changes of remote transmit address and SyncSource in an established RTP connection.
 *
 * Revision 2.17  2004/10/02 11:50:54  rjongbloed
 * Fixed RTP media stream so assures RTP session is open before starting.
 *
 * Revision 2.16  2004/04/26 05:37:13  rjongbloed
 * Allowed for selectable auto deletion of RTP user data attached to an RTP session.
 *
 * Revision 2.15  2004/02/19 10:47:01  rjongbloed
 * Merged OpenH323 version 1.13.1 changes.
 *
 * Revision 2.14  2004/01/18 15:35:20  rjongbloed
 * More work on video support
 *
 * Revision 2.13  2003/01/07 04:39:53  robertj
 * Updated to OpenH323 v1.11.2
 *
 * Revision 2.12  2002/11/10 11:33:17  robertj
 * Updated to OpenH323 v1.10.3
 *
 * Revision 2.11  2002/09/16 02:52:35  robertj
 * Added #define so can select if #pragma interface/implementation is used on
 *   platform basis (eg MacOS) rather than compiler, thanks Robert Monaghan.
 *
 * Revision 2.10  2002/09/04 06:01:47  robertj
 * Updated to OpenH323 v1.9.6
 *
 * Revision 2.9  2002/07/01 04:56:31  robertj
 * Updated to OpenH323 v1.9.1
 *
 * Revision 2.8  2002/04/10 03:10:13  robertj
 * Added referential (container) copy functions to session manager class.
 *
 * Revision 2.7  2002/02/13 02:30:06  robertj
 * Added ability for media patch (and transcoders) to handle multiple RTP frames.
 *
 * Revision 2.6  2002/02/11 09:32:12  robertj
 * Updated to openH323 v1.8.0
 *
 * Revision 2.5  2002/01/22 05:58:55  robertj
 * Put MaxPayloadType back in for backward compatibility
 *
 * Revision 2.4  2002/01/22 05:03:06  robertj
 * Added enum for illegal payload type value.
 *
 * Revision 2.3  2001/11/14 06:20:40  robertj
 * Changed sending of control channel reports to be timer based.
 *
 * Revision 2.2  2001/10/05 00:22:13  robertj
 * Updated to PWLib 1.2.0 and OpenH323 1.7.0
 *
 * Revision 2.1  2001/08/01 05:08:43  robertj
 * Moved default session ID's to OpalMediaFormat class.
 *
 * Revision 2.0  2001/07/27 15:48:24  robertj
 * Conversion of OpenH323 to Open Phone Abstraction Library (OPAL)
 *
 * Revision 1.51  2003/10/27 06:03:39  csoutheren
 * Added support for QoS
 *   Thanks to Henry Harrison of AliceStreet
 *
 * Revision 1.50  2003/10/09 09:47:45  csoutheren
 * Fixed problem with re-opening RTP half-channels under unusual
 * circumstances. Thanks to Damien Sandras
 *
 * Revision 1.49  2003/05/14 13:46:39  rjongbloed
 * Removed hack of using special payload type for H.263 for a method which
 *   would be less prone to failure in the future.
 *
 * Revision 1.48  2003/05/02 04:57:43  robertj
 * Added header extension support to RTP data frame class.
 *
 * Revision 1.47  2003/05/02 04:21:53  craigs
 * Added hacked OpalH263 codec
 *
 * Revision 1.46  2003/02/07 00:30:41  robertj
 * Changes for bizarre usage of RTP code outside of scope of H.323 specs.
 *
 * Revision 1.45  2003/02/05 06:32:08  robertj
 * Fixed non-stun symmetric NAT support recently broken.
 *
 * Revision 1.44  2003/02/04 07:06:41  robertj
 * Added STUN support.
 *
 * Revision 1.43  2002/11/19 01:48:15  robertj
 * Allowed get/set of canonical anme and tool name.
 *
 * Revision 1.42  2002/11/12 22:10:48  robertj
 * Updated documentation.
 *
 * Revision 1.41  2002/10/31 00:33:29  robertj
 * Enhanced jitter buffer system so operates dynamically between minimum and
 *   maximum values. Altered API to assure app writers note the change!
 *
 * Revision 1.40  2002/09/26 04:01:58  robertj
 * Fixed calculation of fraction of packets lost in RR, thanks Awais Ali
 *
 * Revision 1.39  2002/09/16 01:14:15  robertj
 * Added #define so can select if #pragma interface/implementation is used on
 *   platform basis (eg MacOS) rather than compiler, thanks Robert Monaghan.
 *
 * Revision 1.38  2002/09/03 05:47:02  robertj
 * Normalised the multi-include header prevention ifdef/define symbol.
 * Added copy constructor/operator for session manager.
 *
 * Revision 1.37  2002/08/05 10:03:47  robertj
 * Cosmetic changes to normalise the usage of pragma interface/implementation.
 *
 * Revision 1.36  2002/05/28 02:37:37  robertj
 * Fixed reading data out of RTCP compound statements.
 *
 * Revision 1.35  2002/05/02 05:58:24  robertj
 * Changed the mechanism for sending RTCP reports so that they will continue
 *   to be sent regardless of if there is any actual data traffic.
 * Added support for compound RTCP statements for sender and receiver reports.
 *
 * Revision 1.34  2002/02/09 02:33:37  robertj
 * Improved payload type docuemntation and added Cisco CN.
 *
 * Revision 1.33  2002/01/22 07:08:26  robertj
 * Added IllegalPayloadType enum as need marker for none set
 *   and MaxPayloadType is a legal value.
 *
 * Revision 1.32  2001/11/09 05:39:54  craigs
 * Added initial T.38 support thanks to Adam Lazur
 *
 * Revision 1.31  2001/09/11 00:21:21  robertj
 * Fixed missing stack sizes in endpoint for cleaner thread and jitter thread.
 *
 * Revision 1.30  2001/07/06 06:32:22  robertj
 * Added flag and checks for RTP data having specific SSRC.
 * Changed transmitter IP address check so is based on first received
 *    PDU instead of expecting it to come from the host we are sending to.
 *
 * Revision 1.29  2001/06/04 11:37:48  robertj
 * Added thread safe enumeration functions of RTP sessions.
 * Added member access functions to UDP based RTP sessions.
 *
 * Revision 1.28  2001/04/24 06:15:50  robertj
 * Added work around for strange Cisco bug which suddenly starts sending
 *   RTP packets beginning at a difference sequence number base.
 *
 * Revision 1.27  2001/04/02 23:58:23  robertj
 * Added jitter calculation to RTP session.
 * Added trace of statistics.
 *
 * Revision 1.26  2001/02/09 05:16:24  robertj
 * Added #pragma interface for GNU C++.
 *
 * Revision 1.25  2000/12/18 08:58:30  craigs
 * Added ability set ports
 *
 * Revision 1.24  2000/09/25 01:44:31  robertj
 * Fixed possible race condition on shutdown of RTP session with jitter buffer.
 *
 * Revision 1.23  2000/09/21 02:06:06  craigs
 * Added handling for endpoints that return conformant, but useless, RTP address
 * and port numbers
 *
 * Revision 1.22  2000/05/23 12:57:28  robertj
 * Added ability to change IP Type Of Service code from applications.
 *
 * Revision 1.21  2000/05/18 11:53:34  robertj
 * Changes to support doc++ documentation generation.
 *
 * Revision 1.20  2000/05/04 11:49:21  robertj
 * Added Packets Too Late statistics, requiring major rearrangement of jitter buffer code.
 *
 * Revision 1.19  2000/05/02 04:32:25  robertj
 * Fixed copyright notice comment.
 *
 * Revision 1.18  2000/05/01 01:01:24  robertj
 * Added flag for what to do with out of orer packets (use if jitter, don't if not).
 *
 * Revision 1.17  2000/04/30 03:55:18  robertj
 * Improved the RTCP messages, epecially reports
 *
 * Revision 1.16  2000/04/10 17:39:21  robertj
 * Fixed debug output of RTP payload types to allow for unknown numbers.
 *
 * Revision 1.15  2000/04/05 03:17:31  robertj
 * Added more RTP statistics gathering and H.245 round trip delay calculation.
 *
 * Revision 1.14  2000/03/23 02:55:18  robertj
 * Added sending of SDES control packets.
 *
 * Revision 1.13  2000/03/20 20:54:04  robertj
 * Fixed problem with being able to reopen for reading an RTP_Session (Cisco compatibilty)
 *
 * Revision 1.12  2000/02/29 13:00:13  robertj
 * Added extra statistic display for RTP packets out of order.
 *
 * Revision 1.11  1999/12/30 09:14:31  robertj
 * Changed payload type functions to use enum.
 *
 * Revision 1.10  1999/12/23 23:02:35  robertj
 * File reorganision for separating RTP from H.323 and creation of LID for VPB support.
 *
 * Revision 1.9  1999/11/20 05:35:57  robertj
 * Fixed possibly I/O block in RTP read loops.
 *
 * Revision 1.8  1999/11/19 09:17:15  robertj
 * Fixed problems with aycnhronous shut down of logical channels.
 *
 * Revision 1.7  1999/11/14 11:41:18  robertj
 * Added access functions to RTP statistics.
 *
 * Revision 1.6  1999/09/08 04:05:48  robertj
 * Added support for video capabilities & codec, still needs the actual codec itself!
 *
 * Revision 1.5  1999/08/31 12:34:18  robertj
 * Added gatekeeper support.
 *
 * Revision 1.4  1999/07/13 09:53:24  robertj
 * Fixed some problems with jitter buffer and added more debugging.
 *
 * Revision 1.3  1999/07/09 06:09:49  robertj
 * Major implementation. An ENORMOUS amount of stuff added everywhere.
 *
 * Revision 1.2  1999/06/22 13:49:40  robertj
 * Added GSM support and further RTP protocol enhancements.
 *
 * Revision 1.1  1999/06/14 06:12:25  robertj
 * Changes for using RTP sessions correctly in H323 Logical Channel context
 *
 */

#ifndef __OPAL_RTP_H
#define __OPAL_RTP_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <opal/buildopts.h>
#include <ptlib/sockets.h>


class RTP_JitterBuffer;
class PSTUNClient;
class OpalSecurityMode;

#if OPAL_RTP_AGGREGATE
#include <ptclib/sockagg.h>
#else
typedef void * PHandleAggregator;
typedef void * RTP_AggregatedHandle;
#endif

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
// Real Time Protocol - IETF RFC1889 and RFC1890

/**An RTP data frame encapsulation.
  */
class RTP_DataFrame : public PBYTEArray
{
  PCLASSINFO(RTP_DataFrame, PBYTEArray);

  public:
    RTP_DataFrame(PINDEX payloadSize = 2048);
    RTP_DataFrame(const BYTE * data, PINDEX len, BOOL dynamic = TRUE);

    enum {
      ProtocolVersion = 2,
      MinHeaderSize = 12,
      // Max Ethernet packet (1518 bytes) minus 802.3/CRC, 802.3, IP, UDP an RTP headers
      MaxEthernetPayloadSize = (1518-14-4-8-20-16-12)
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

      LastKnownPayloadType,

      DynamicBase = 96,
      MaxPayloadType = 127,
      IllegalPayloadType
    };

    typedef std::map<PayloadTypes, PayloadTypes> PayloadMapType;

    unsigned GetVersion() const { return (theArray[0]>>6)&3; }

    BOOL GetExtension() const   { return (theArray[0]&0x10) != 0; }
    void SetExtension(BOOL ext);

    BOOL GetMarker() const { return (theArray[1]&0x80) != 0; }
    void SetMarker(BOOL m);

    BOOL GetPadding() const { return (theArray[0]&0x20) != 0; }
    void SetPadding(BOOL v) { if (v) theArray[0] |= 0x20; else theArray[0] &= 0xdf; }

    unsigned GetPaddingSize() const;
    
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

    PINDEX GetHeaderSize() const;

    int GetExtensionType() const; // -1 is no extension
    void   SetExtensionType(int type);
    PINDEX GetExtensionSize() const;
    BOOL   SetExtensionSize(PINDEX sz);
    BYTE * GetExtensionPtr() const;

    PINDEX GetPayloadSize() const { return payloadSize - GetPaddingSize(); }
    BOOL   SetPayloadSize(PINDEX sz);
    BYTE * GetPayloadPtr()     const { return (BYTE *)(theArray+GetHeaderSize()); }

    virtual void PrintOn(ostream & strm) const;

  protected:
    PINDEX payloadSize;

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
      e_IntraFrameRequest = 192,
      e_SenderReport = 200,
      e_ReceiverReport,
      e_SourceDescription,
      e_Goodbye,
      e_ApplDefined
    };

    unsigned GetPayloadType() const { return (BYTE)theArray[compoundOffset+1]; }
    void     SetPayloadType(unsigned t);

    PINDEX GetPayloadSize() const { return 4*(*(PUInt16b *)&theArray[compoundOffset+2]); }
    void   SetPayloadSize(PINDEX sz);

    BYTE * GetPayloadPtr() const;

    BOOL ReadNextPacket();
    BOOL StartNewPacket();
    void EndPacket();

    PINDEX GetCompoundSize() const;

    BOOL GetPadding() const { return theArray[compoundOffset & 0x20] != 0; }
    void SetPadding(BOOL v) { if (v) theArray[compoundOffset] |= 0x20; else theArray[compoundOffset] &= 0xdf; }

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
#pragma pack()

  protected:
    PINDEX compoundOffset;
    PINDEX payloadSize;
};


class RTP_Session;

/**This class is the base for user data that may be attached to the RTP_session
   allowing callbacks for statistics and progress monitoring to be passed to an
   arbitrary object that an RTP consumer may require.
  */
class RTP_UserData : public PObject
{
  PCLASSINFO(RTP_UserData, PObject);

  public:
    /**Callback from the RTP session for transmit statistics monitoring.
       This is called every RTP_Session::txStatisticsInterval packets on the
       transmitter indicating that the statistics have been updated.

       The default behaviour does nothing.
      */
    virtual void OnTxStatistics(
      const RTP_Session & session   ///<  Session with statistics
    ) const;

    /**Callback from the RTP session for receive statistics monitoring.
       This is called every RTP_Session::receiverReportInterval packets on the
       receiver indicating that the statistics have been updated.

       The default behaviour does nothing.
      */
    virtual void OnRxStatistics(
      const RTP_Session & session   ///<  Session with statistics
    ) const;

#if OPAL_VIDEO
    /**Callback from the RTP session when an intra frame request control
       packet is sent.

       The default behaviour does nothing.
      */
    virtual void OnTxIntraFrameRequest(
      const RTP_Session & session   ///<  Session with statistics
    ) const;

    /**Callback from the RTP session when an intra frame request control
       packet is received.

       The default behaviour does nothing.
      */
    virtual void OnRxIntraFrameRequest(
      const RTP_Session & session   ///<  Session with statistics
    ) const;
#endif
};


/**This class is for encpsulating the IETF Real Time Protocol interface.
 */
class RTP_Session : public PObject
{
  PCLASSINFO(RTP_Session, PObject);

  public:
  /**@name Construction */
  //@{
    /**Create a new RTP session.
     */
    RTP_Session(
      PHandleAggregator * aggregator, ///<  RTP aggregator
      unsigned id,                    ///<  Session ID for RTP channel
      RTP_UserData * userData = NULL, ///<  Optional data for session.
      BOOL autoDeleteUserData = TRUE  ///<  Delete optional data with session.
    );

    /**Delete a session.
       This deletes the userData field if autoDeleteUserData is TRUE.
     */
    ~RTP_Session();
  //@}

  /**@name Operations */
  //@{
    /**Sets the size of the jitter buffer to be used by this RTP session.
       A session default to not having any jitter buffer enabled for reading
       and the ReadBufferedData() function simply calls ReadData(). Once a
       jitter buffer has been created it cannot be removed, though its size
       may be adjusted.
       
       If the jitterDelay paramter is zero, it destroys the jitter buffer
       attached to this RTP session.
      */
    void SetJitterBufferSize(
      unsigned minJitterDelay, ///<  Minimum jitter buffer delay in RTP timestamp units
      unsigned maxJitterDelay, ///<  Maximum jitter buffer delay in RTP timestamp units
      unsigned timeUnits = 8,  ///<  Time Units
      PINDEX stackSize = 30000 ///<  Stack size for jitter thread
    );

    /**Get current size of the jitter buffer.
       This returns the currently used jitter buffer delay in RTP timestamp
       units. It will be some value between the minimum and maximum set in
       the SetJitterBufferSize() function.
      */
    unsigned GetJitterBufferSize() const;
    
    /**Get current time units of the jitter buffer.
     */
    unsigned GetJitterTimeUnits() const;

    /**Modifies the QOS specifications for this RTP session*/
    virtual BOOL ModifyQOS(RTP_QOS * )
    { return FALSE; }

    /**Read a data frame from the RTP channel.
       This function will conditionally read data from the jitter buffer or
       directly if there is no jitter buffer enabled. An application should
       generally use this in preference to directly calling ReadData().
      */
    virtual BOOL ReadBufferedData(
      DWORD timestamp,        ///<  Timestamp to read from buffer.
      RTP_DataFrame & frame   ///<  Frame read from the RTP session
    );

    /**Read a data frame from the RTP channel.
       Any control frames received are dispatched to callbacks and are not
       returned by this function. It will block until a data frame is
       available or an error occurs.
      */
    virtual BOOL ReadData(
      RTP_DataFrame & frame,  ///<  Frame read from the RTP session
      BOOL loop               ///<  If TRUE, loop as long as data is available, if FALSE, only process once
    ) = 0;

    /**Write a data frame from the RTP channel.
      */
    virtual BOOL WriteData(
      RTP_DataFrame & frame   ///<  Frame to write to the RTP session
    ) = 0;

    /** Write data frame to the RTP channel outside the normal stream of media
      * Used for RFC2833 packets
      */
    virtual BOOL WriteOOBData(
      RTP_DataFrame & frame
    );

    /**Write a control frame from the RTP channel.
      */
    virtual BOOL WriteControl(
      RTP_ControlFrame & frame    ///<  Frame to write to the RTP session
    ) = 0;

    /**Write the RTCP reports.
      */
    virtual BOOL SendReport();

    /**Close down the RTP session.
      */
    virtual void Close(
      BOOL reading    ///<  Closing the read side of the session
    ) = 0;

   /**Reopens an existing session in the given direction.
      */
    virtual void Reopen(
      BOOL isReading
    ) = 0;

    /**Get the local host name as used in SDES packes.
      */
    virtual PString GetLocalHostName() = 0;
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
  //@}

  /**@name Member variable access */
  //@{
    /**Get the ID for the RTP session.
      */
    unsigned GetSessionID() const { return sessionID; }

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

    /**Get the user data for the session.
      */
    RTP_UserData * GetUserData() const { return userData; }

    /**Set the user data for the session.
      */
    void SetUserData(
      RTP_UserData * data,            ///<  New user data to be used
      BOOL autoDeleteUserData = TRUE  ///<  Delete optional data with session.
    );

    /**Get the source output identifier.
      */
    DWORD GetSyncSourceOut() const { return syncSourceOut; }

    /**Increment reference count for RTP session.
      */
    void IncrementReference() { referenceCount++; }

    /**Decrement reference count for RTP session.
      */
    BOOL DecrementReference() { return --referenceCount == 0; }

    /**Indicate if will ignore all but first received SSRC value.
      */
    BOOL WillIgnoreOtherSources() const { return ignoreOtherSources; }

    /**Indicate if will ignore all but first received SSRC value.
      */
    void SetIgnoreOtherSources(
      BOOL ignore   ///<  Flag for ignore other SSRC values
    ) { ignoreOtherSources = ignore; }

    /**Indicate if will ignore out of order packets.
      */
    BOOL WillIgnoreOutOfOrderPackets() const { return ignoreOutOfOrderPackets; }

    /**Indicate if will ignore out of order packets.
      */
    void SetIgnoreOutOfOrderPackets(
      BOOL ignore   ///<  Flag for ignore out of order packets
    ) { ignoreOutOfOrderPackets = ignore; }

    /**Indicate if will ignore rtp payload type changes in received packets.
     */
    void SetIgnorePayloadTypeChanges(
      BOOL ignore   ///<  Flag to ignore payload type changes
    ) { ignorePayloadTypeChanges = ignore; }

    /**Get the time interval for sending RTCP reports in the session.
      */
    const PTimeInterval & GetReportTimeInterval() { return reportTimeInterval; }

    /**Set the time interval for sending RTCP reports in the session.
      */
    void SetReportTimeInterval(
      const PTimeInterval & interval ///<  New time interval for reports.
    )  { reportTimeInterval = interval; }

    /**Get the current report timer
     */
    PTimeInterval GetReportTimer()
    { return reportTimer; }

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

    /**Get total number of packets received out of order in session.
      */
    DWORD GetPacketsOutOfOrder() const { return packetsOutOfOrder; }

    /**Get total number received packets too late to go into jitter buffer.
      */
    DWORD GetPacketsTooLate() const;

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

    /**Get averaged jitter time for received packets.
       This is the calculated statistical variance of the interarrival
       time of received packets in milliseconds.
      */
    DWORD GetAvgJitterTime() const { return jitterLevel>>7; }

    /**Get averaged jitter time for received packets.
       This is the maximum value of jitterLevel for the session.
      */
    DWORD GetMaxJitterTime() const { return maximumJitterLevel>>7; }
  //@}

  /**@name Functions added to support RTP aggregation */
  //@{
    virtual int GetDataSocketHandle() const
    { return -1; }
    virtual int GetControlSocketHandle() const
    { return -1; }
  //@}

    virtual void SendBYE();
    virtual void SetCloseOnBYE(BOOL v)  { closeOnBye = v; }
#if OPAL_VIDEO
    /** Tell the rtp session to send out an intra frame request control packet.
        This is called when the media stream receives an OpalVideoUpdatePicture
        media command.
      */
    virtual void SendIntraFrameRequest();
#endif    
  protected:
    void AddReceiverReport(RTP_ControlFrame::ReceiverReport & receiver);
    BOOL InsertReportPacket(RTP_ControlFrame & report);

    unsigned           sessionID;
    PString            canonicalName;
    PString            toolName;
    unsigned           referenceCount;
    RTP_UserData     * userData;
    BOOL               autoDeleteUserData;
    RTP_JitterBuffer * jitter;

    BOOL          ignoreOtherSources;
    BOOL          ignoreOutOfOrderPackets;
    DWORD         syncSourceOut;
    DWORD         syncSourceIn;
    DWORD         lastSentTimestamp;
    BOOL	        allowSyncSourceInChange;
    BOOL	        allowRemoteTransmitAddressChange;
    BOOL	        allowSequenceChange;
    PTimeInterval reportTimeInterval;
    unsigned      txStatisticsInterval;
    unsigned      rxStatisticsInterval;
    WORD          lastSentSequenceNumber;
    WORD          expectedSequenceNumber;
    PTimeInterval lastSentPacketTime;
    PTimeInterval lastReceivedPacketTime;
    WORD          lastRRSequenceNumber;
    PINDEX        consecutiveOutOfOrderPackets;

    PMutex        sendDataMutex;
    DWORD         timeStampOut;               // current timestamp for this session
    DWORD         timeStampOffs;              // offset between incoming media timestamp and timeStampOut
    BOOL          timeStampOffsetEstablished; // TRUE if timeStampOffs has been established by media
    BOOL          timeStampIsPremedia;        // TRUE if timeStampOutTick has been initialised

    // Statistics
    DWORD packetsSent;
    DWORD rtcpPacketsSent;
    DWORD octetsSent;
    DWORD packetsReceived;
    DWORD octetsReceived;
    DWORD packetsLost;
    DWORD packetsOutOfOrder;
    DWORD averageSendTime;
    DWORD maximumSendTime;
    DWORD minimumSendTime;
    DWORD averageReceiveTime;
    DWORD maximumReceiveTime;
    DWORD minimumReceiveTime;
    DWORD jitterLevel;
    DWORD maximumJitterLevel;

    DWORD markerSendCount;
    DWORD markerRecvCount;

    unsigned txStatisticsCount;
    unsigned rxStatisticsCount;

    DWORD    averageSendTimeAccum;
    DWORD    maximumSendTimeAccum;
    DWORD    minimumSendTimeAccum;
    DWORD    averageReceiveTimeAccum;
    DWORD    maximumReceiveTimeAccum;
    DWORD    minimumReceiveTimeAccum;
    DWORD    packetsLostSinceLastRR;
    DWORD    lastTransitTime;
    
    RTP_DataFrame::PayloadTypes lastReceivedPayloadType;
    BOOL ignorePayloadTypeChanges;

    PMutex reportMutex;
    PTimer reportTimer;

    PHandleAggregator * aggregator;

    BOOL closeOnBye;
    BOOL byeSent;
};


/**This class is for encpsulating the IETF Real Time Protocol interface.
 */
class RTP_SessionManager : public PObject
{
  PCLASSINFO(RTP_SessionManager, PObject);

  public:
  /**@name Construction */
  //@{
    /**Construct new session manager database.
      */
    RTP_SessionManager();
    RTP_SessionManager(const RTP_SessionManager & sm);
    RTP_SessionManager & operator=(const RTP_SessionManager & sm);
  //@}


  /**@name Operations */
  //@{
    /**Use an RTP session for the specified ID.

       If this function returns a non-null value, then the ReleaseSession()
       function MUST be called or the session is never deleted for the
       lifetime of the session manager.

       If there is no session of the specified ID, then you MUST call the
       AddSession() function with a new RTP_Session. The mutex flag is left
       locked in this case. The AddSession() expects the mutex to be locked
       and unlocks it automatically.
      */
    RTP_Session * UseSession(
      unsigned sessionID    ///<  Session ID to use.
    );

    /**Add an RTP session for the specified ID.

       This function MUST be called only after the UseSession() function has
       returned NULL. The mutex flag is left locked in that case. This
       function expects the mutex to be locked and unlocks it automatically.
      */
    void AddSession(
      RTP_Session * session    ///<  Session to add.
    );

    /**Release the session. If the session ID is not being used any more any
       clients via the UseSession() function, then the session is deleted.
     */
    void ReleaseSession(
      unsigned sessionID,    ///<  Session ID to release.
      BOOL clearAll = FALSE  ///<  Clear all sessions with that ID
    );

    /**Get a session for the specified ID.
       Unlike UseSession, this does not increment the usage count on the
       session so may be used to just gain a pointer to an RTP session.
     */
    RTP_Session * GetSession(
      unsigned sessionID    ///<  Session ID to get.
    ) const;

    /**Begin an enumeration of the RTP sessions.
       This function acquires the mutex for the sessions database and returns
       the first element in it.

         eg:
         RTP_Session * session;
         for (session = rtpSessions.First(); session != NULL; session = rtpSessions.Next()) {
           if (session->Something()) {
             rtpSessions.Exit();
             break;
           }
         }

       Note that the Exit() function must be called if the enumeration is
       stopped prior to Next() returning NULL.
      */
    RTP_Session * First();

    /**Get the next session in the enumeration.
       The session database remains locked until this function returns NULL.

       Note that the Exit() function must be called if the enumeration is
       stopped prior to Next() returning NULL.
      */
    RTP_Session * Next();

    /**Exit the enumeration of RTP sessions.
       If the enumeration is desired to be exited before Next() returns NULL
       this this must be called to unlock the session database.

       Note that you should NOT call Exit() if Next() HAS returned NULL, or
       race conditions can result.
      */
    void Exit();
  //@}


  protected:
    PDICTIONARY(SessionDict, POrdinalKey, RTP_Session);
    SessionDict sessions;
    PMutex      mutex;
    PINDEX      enumerationIndex;
};



/**This class is for the IETF Real Time Protocol interface on UDP/IP.
 */
class RTP_UDP : public RTP_Session
{
  PCLASSINFO(RTP_UDP, RTP_Session);

  public:
  /**@name Construction */
  //@{
    /**Create a new RTP channel.
     */
    RTP_UDP(
      PHandleAggregator * aggregator, ///< RTP aggregator
      unsigned id,                    ///<  Session ID for RTP channel
      BOOL remoteIsNAT                ///<  TRUE is remote is behind NAT
    );

    /// Destroy the RTP
    ~RTP_UDP();
  //@}

  /**@name Overrides from class RTP_Session */
  //@{
    /**Read a data frame from the RTP channel.
       Any control frames received are dispatched to callbacks and are not
       returned by this function. It will block until a data frame is
       available or an error occurs.
      */
    virtual BOOL ReadData(RTP_DataFrame & frame, BOOL loop);

    /** Write a data frame to the RTP channel.
      */
    virtual BOOL WriteData(RTP_DataFrame & frame);

    /** Write data frame to the RTP channel outside the normal stream of media
      * Used for RFC2833 packets
      */
    virtual BOOL WriteOOBData(RTP_DataFrame & frame);

    /**Write a control frame from the RTP channel.
      */
    virtual BOOL WriteControl(RTP_ControlFrame & frame);

    /**Close down the RTP session.
      */
    virtual void Close(
      BOOL reading    ///<  Closing the read side of the session
    );

    /**Get the session description name.
      */
    virtual PString GetLocalHostName();
  //@}

    /**Change the QoS settings
      */
    virtual BOOL ModifyQOS(RTP_QOS * rtpqos);

  /**@name New functions for class */
  //@{
    /**Open the UDP ports for the RTP session.
      */
    virtual BOOL Open(
      PIPSocket::Address localAddress,  ///<  Local interface to bind to
      WORD portBase,                    ///<  Base of ports to search
      WORD portMax,                     ///<  end of ports to search (inclusive)
      BYTE ipTypeOfService,             ///<  Type of Service byte
      PSTUNClient * stun = NULL,        ///<  STUN server to use createing sockets (or NULL if no STUN)
      RTP_QOS * rtpqos = NULL           ///<  QOS spec (or NULL if no QoS)
    );
  //@}

   /**Reopens an existing session in the given direction.
      */
    virtual void Reopen(BOOL isReading);
  //@}

  /**@name Member variable access */
  //@{
    /**Get local address of session.
      */
    virtual PIPSocket::Address GetLocalAddress() const { return localAddress; }

    /**Set local address of session.
      */
    virtual void SetLocalAddress(
      const PIPSocket::Address & addr
    ) { localAddress = addr; }

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

    /**Set the remote address and port information for session.
      */
    virtual BOOL SetRemoteSocketInfo(
      PIPSocket::Address address,   ///<  Addre ss of remote
      WORD port,                    ///<  Port on remote
      BOOL isDataPort               ///<  Flag for data or control channel
    );

    /**Apply QOS - requires address to connect the socket on Windows platforms
     */
    virtual void ApplyQOS(
      const PIPSocket::Address & addr
    );
  //@}

    virtual int GetDataSocketHandle() const
    { return dataSocket != NULL ? dataSocket->GetHandle() : -1; }

    virtual int GetControlSocketHandle() const
    { return controlSocket != NULL ? controlSocket->GetHandle() : -1; }

  protected:
    virtual int WaitForPDU(PUDPSocket & dataSocket, PUDPSocket & controlSocket, const PTimeInterval & timer);
    virtual SendReceiveStatus ReadDataPDU(RTP_DataFrame & frame);
    virtual SendReceiveStatus ReadControlPDU();
    virtual SendReceiveStatus ReadDataOrControlPDU(
      PUDPSocket & socket,
      PBYTEArray & frame,
      BOOL fromDataChannel
    );

    PIPSocket::Address localAddress;
    WORD               localDataPort;
    WORD               localControlPort;

    PIPSocket::Address remoteAddress;
    WORD               remoteDataPort;
    WORD               remoteControlPort;

    PIPSocket::Address remoteTransmitAddress;

    BOOL shutdownRead;
    BOOL shutdownWrite;

    PUDPSocket * dataSocket;
    PUDPSocket * controlSocket;

    BOOL appliedQOS;
    BOOL remoteIsNAT;
    BOOL first;
};

/////////////////////////////////////////////////////////////////////////////

class SecureRTP_UDP : public RTP_UDP
{
  PCLASSINFO(SecureRTP_UDP, RTP_UDP);

  public:
  /**@name Construction */
  //@{
    /**Create a new RTP channel.
     */
    SecureRTP_UDP(
      PHandleAggregator * aggregator, ///< RTP aggregator
      unsigned id,                    ///<  Session ID for RTP channel
      BOOL remoteIsNAT                ///<  TRUE is remote is behind NAT
    );

    /// Destroy the RTP
    ~SecureRTP_UDP();

    virtual void SetSecurityMode(OpalSecurityMode * srtpParms);  
    virtual OpalSecurityMode * GetSecurityParms() const;

  protected:
    OpalSecurityMode * securityParms;
};

#endif // __OPAL_RTP_H

/////////////////////////////////////////////////////////////////////////////
