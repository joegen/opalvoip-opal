/*
 * metrics.h
 *
 * E-Model implementation
 *
 * Open Phone Abstraction Library (OPAL)
 *
 * Copyright (c) 2010 Universidade Federal do Amazonas
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
 * Contributor(s): ______________________________________.
 */

#ifndef OPAL_RTP_METRICS_H
#define OPAL_RTP_METRICS_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <opal_config.h>

#if OPAL_RTCP_XR

#include <rtp/rtp_session.h>

#include <list>

class RTP_Session;
class RTP_DataFrame;

///////////////////////////////////////////////////////////////////////////////
// RTCP-XR - VoIP Metrics Report Block

class RTP_MetricsReport : public PObject
{
    PCLASSINFO(RTP_MetricsReport, PObject);
  public:
    RTP_MetricsReport(const RTP_ControlFrame::MetricsReport & mr);
#if PTRACING
    void PrintOn(ostream &) const;
#endif

    RTP_SyncSourceId sourceIdentifier;
    float            lossRate;       /* percentage of RTP data packets lost */ 
    float            discardRate;    /* percentage of RTP data packets discarded */
    float            burstDensity;   /* percentage of RTP data packets within burst periods */
    float            gapDensity;     /* percentage of RTP data packets within inter-burst gaps */
    unsigned         roundTripDelay; /* the most recently calculated round trip time */    
    unsigned         RFactor;        /* voice quality metric of the call */
    unsigned         mosLQ;          /* MOS for listen quality */
    unsigned         mosCQ;          /* MOS for conversational quality */
    unsigned         jbNominal;      /* current nominal jitter buffer delay, in ms */ 
    unsigned         jbMaximum;      /* current maximum jitter buffer delay, in ms */
    unsigned         jbAbsolute;     /* current absolute maximum jitter buffer delay, in ms */
};


/**This is a class used to calculate voice quality metrics acording to the
   E-Model, proposed by ITU-T G.107 recomendation.	
  */
class RTCP_XR_Metrics : public PObject
{
    PCLASSINFO(RTCP_XR_Metrics, PObject);
  protected:
    RTCP_XR_Metrics(
      float    Ie,
      float    Bpl,
      float    lookAheadTime,
      PINDEX   payloadSize,
      unsigned payloadBitrate
    );

  public:
    static RTCP_XR_Metrics * Create(const RTP_DataFrame & frame);

    ~RTCP_XR_Metrics();

    enum PacketEvent {
      PACKET_RECEIVED,
      PACKET_DISCARDED,
      PACKET_LOST
    };

    enum PeriodType {
      GAP,     /* a period of low packet losses and/or discards */
      BURST,   /* a period of a high proportion of packet losses and/or discards */
    };

    /* A period of time, which can be a burst or a gap */
    typedef struct TimePeriod {
      PeriodType      type;
      PTimeInterval   duration;
    } TimePeriod;

    /**A period of time, used to calculate an average Id, needed to compute the R factor.
       Id factor represents the impairments caused by delay.
      */
    typedef struct IdPeriod {
      PTimeInterval duration;
      float         Id;
    } IdPeriod;

    /**A period of time, used to calculate an average Ie, needed to compute the R factor.
       Ie factor represents the impairments caused by low bit-rate codecs.
      */
    typedef struct IePeriod {
      PeriodType    type;
      PTimeInterval duration;
      float         Ieff;
    } IePeriod;

    enum QualityType {
      LQ,  /* Listening Quality, not include the effects of delay */
      CQ   /* Conversational Quality, include the effects of delay */
    };

    /**Set the jitter buffer delay.
       This must be called to obtain the Id parameter.
      */
    void SetJitterDelay(
      unsigned delay   ///<  The jitter buffer delay in milliseconds
    );

    /**Called when a packet is received.
      */
    void OnPacketReceived();

    /**Called when a packet is discarded.
      */
    void OnPacketDiscarded();

    /**Called when a packet is lost.
      */
    void OnPacketLost();

    /**Called when several packets are lost.
      */
    void OnPacketLost(
      unsigned dropped   ///< Number of lost packets.
    );

    /**Called when a Sender Report is receveid.
      */
    void OnRxSenderReport(
      const PTime & lastTimestamp,  ///< "Last SR" field of the received SR.
      const PTimeInterval & delay   ///< "Delay since the last SR" field of the received SR.
    );

    /**Get the fraction of RTP packets lost since the beginning of the reception.
      */
    unsigned GetLossRate() const;

    /**Get the fraction of RTP packets discarded since the beginning of the reception.
      */
    unsigned GetDiscardRate() const;

    /**Get the fraction of RTP packets lost/discarded within burst periods since
       the beginning of the reception.
      */
    unsigned GetBurstDensity() const;

    /**Get the fraction of RTP packets lost/discarded within gap periods since
       the beginning of the reception.
      */
    unsigned GetGapDensity() const;

    /**Get the mean duration, in milliseconds, of burst periods since the
       beginning of the reception.
     */
    unsigned GetBurstDuration() const;

    /**Get the mean duration, in milliseconds, of gap periods since the beginning
       of the reception.
      */
    unsigned GetGapDuration() const;

    /**Get the most recently calculated round trip time between RTP interfaces,
       expressed in millisecond.
      */
    unsigned GetRoundTripDelay() const;

    /**Get the most recently estimated end system delay, expressed in millisecond.
      */
    unsigned GetEndSystemDelay() const;

    /**Get the R factor for the current RTP session, expressed in the range of 0 to 100.
      */
    unsigned GetRFactor() const;

    /**Get the estimated MOS score for listening quality of the current RTP session,
       expressed in the range of 10 to 50.
       This metric not includes the effects of delay.
      */
    unsigned GetMOS_LQ() const;

    /**Get the estimated MOS score for conversational quality of the current RTP session,
       expressed in the range of 10 to 50.
       This metric includes the effects of delay.
      */
    unsigned GetMOS_CQ() const;

    // Internal functions
    void InsertMetricsReport(
      RTP_ControlFrame & report,
      const OpalRTPSession & session,
      RTP_SyncSourceId syncSourceOut,
      OpalJitterBuffer * jitter
    );


  protected:
    /**Given the following definition of states:
       state 1 = received a packet during a gap;
       state 2 = received a packet during a burst;
       state 3 = lost a packet during a burst;
       state 4 = lost a isolated packet during a gap.

       This function models the transitions of a Markov chain with the states above.
      */
    void Markov(
      PacketEvent event   ///< Event triggered
    );

    /**Reset the counters used for the Markov chain
      */
    void ResetCounters();

    /**Get the R factor for the current RTP session, expressed in the range of 0 to 100.
      */
    unsigned GetRFactor(
      QualityType qt   ///< Listening or Conversational quality
    ) const;

    /**Get the R factor at the end of the RTP session, expressed in the range of 0 to 100.
      */
    unsigned GetEndOfCallRFactor() const;

    /**Get the estimated MOS score for the current RTP session,
       expressed in the range of 1 to 5.
      */
    float GetMOS(
      QualityType qt   ///< Listening or Conversational quality
    ) const;

    /**Get the estimated MOS score at the end of the RTP session,
       expressed in the range of 1 to 5.
      */
    float GetEndOfCallMOS() const;

    /**Get instantaneous Id factor.
      */
    float GetIdFactor() const;

    /**Get a ponderated value of Id factor.
      */
    float GetPonderateId() const;

    /**Get instantaneous Ieff factor.
      */
    float GetIeff(
      PeriodType type   ///< The type of period (burst or gap).
    ) const;

    /**Get a value of Ie factor at the end of the RTP session.
      */
    float GetEndOfCallIe() const;

    /**Get a ponderated value of Ie factor.
      */
    float GetPonderateIe() const;

    /**Create a a burst or gap period.
      */
    TimePeriod CreateTimePeriod(
      PeriodType type,        ///< The type of period (burst or gap).
      const PTime & beginTimestamp,   ///< Beginning of period timestamp.
      const PTime & endTimestamp      ///< End of period timestamp.
    );

    /**Create a period of time with an Id value associated.
      */
    IdPeriod CreateIdPeriod(
      const PTime & beginTimestamp,   ///< Beginning of period timestamp.
      const PTime & endTimestamp      ///< End of period timestamp.
    );

    /**Create a period of time with an Ie value associated.
      */
    IePeriod CreateIePeriod(
      const TimePeriod & timePeriod   ///< Period of time to calculate the Ie.
    );

    /* data associated with the payload */
    float    m_Ie;              /* equipment impairment factor for the codec utilized */
    float    m_Bpl;             /* robustness factor for the codec utilized */
    float    m_lookAheadTime;   /* codec lookahead time */
    PINDEX   m_payloadSize;
    unsigned m_payloadBitrate;

    unsigned m_gmin;                    /* gap threshold */
    unsigned m_lostInBurst;             /* number of lost packets within the current burst */
    unsigned m_packetsReceived;         /* packets received since the beggining of the reception */
    unsigned m_packetsSinceLastLoss;    /* packets received since the last loss or discard event */
    unsigned m_packetsLost;             /* packets lost since the beggining of the reception */
    unsigned m_packetsDiscarded;        /* packets discarded since the beggining of the receptions */
    unsigned m_srPacketsReceived;       /* count of SR packets received */

    unsigned m_packetsReceivedInGap;    /* packets received within gap periods */
    unsigned m_packetsLostInGap;        /* packets lost within gap periods */

    unsigned m_packetsReceivedInBurst;  /* packets received within burst periods */
    unsigned m_packetsLostInBurst;      /* packets lost within burst periods */

    /**Given the following definition of states:
       state 1 = received a packet during a gap;
       state 2 = received a packet during a burst;
       state 3 = lost a packet during a burst;
       state 4 = lost a isolated packet during a gap.

       Variables below correspond to state transition counts.
       To reset these counters, call the ResetCounters() function.
      */
    unsigned c5;
    unsigned c11;
    unsigned c13;
    unsigned c14;
    unsigned c22;
    unsigned c23;
    unsigned c31;
    unsigned c32;
    unsigned c33;

    /* variables to calculate round trip delay */
    PTime         m_lsrTime;
    PTimeInterval m_dlsrTime;
    PTime         m_arrivalTime;

    unsigned m_jitterDelay;  /* jitter buffer delay, in milliseconds */
    float    m_lastId;       /* last Id calculated */
    float    m_lastIe;       /* last Ie calculated */

    std::list<TimePeriod> m_timePeriods;
    std::list<IePeriod>   m_iePeriods;
    std::list<IdPeriod>   m_idPeriods;

    PeriodType m_currentPeriodType;           /* indicates if we are within a gap or burst */
    PTime m_periodBeginTimestamp;             /* timestamp of the beginning of the gap or burst period */
    PTime m_lastLossTimestamp;                /* timestamp of the last loss */
    PTime m_lastLossInBurstTimestamp;         /* timestamp of the last loss within a burst period */
    PTime m_lastJitterBufferChangeTimestamp;  /* timestamp of the last change in jitter buffer size */

};


#endif // OPAL_RTCP_XR

#endif // OPAL_METRICS_H


/////////////////////////////////////////////////////////////////////////////
