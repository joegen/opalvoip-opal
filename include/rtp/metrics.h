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
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#ifndef OPAL_RTP_METRICS_H
#define OPAL_RTP_METRICS_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <rtp/rtp.h>

#if OPAL_RTCP_XR

#include <list>

class RTP_Session;
class RTP_DataFrame;

///////////////////////////////////////////////////////////////////////////////
// RTCP-XR - VoIP Metrics Report Block

/**This is a class used to calculate voice quality metrics acording to the 
   E-Model, proposed by ITU-T G.107 recomendation.	
  */
class RTCP_XR_Metrics
{  
  public:
     RTCP_XR_Metrics();
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
       
    std::list<TimePeriod> timePeriods;
    std::list<IePeriod> iePeriods;
    std::list<IdPeriod> idPeriods;
    
    /**Set the payload information.
       This must be called to obtain bitrate and E-Model parameters.
      */
    void SetPayloadInfo(
      RTP_DataFrame frame   ///<  Frame from which we get the payload info
    );
    
    /**Set the jitter buffer delay.
       This must be called to obtain the Id parameter.
      */
    void SetJitterDelay(
      DWORD delay   ///<  The jitter buffer delay in milliseconds
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
      DWORD dropped   ///< Number of lost packets.
    );
    
    /**Called when a Sender Report is receveid.
      */
    void OnRxSenderReport(
      PUInt32b lsr,   ///< "Last SR" field of the received SR.
      PUInt32b dlsr   ///< "Delay since the last SR" field of the received SR.
    );
    
    /**Get the fraction of RTP packets lost since the beginning of the reception.
      */
    BYTE GetLossRate();
    
    /**Get the fraction of RTP packets discarded since the beginning of the reception.
      */
    BYTE GetDiscardRate();
    
    /**Get the fraction of RTP packets lost/discarded within burst periods since
       the beginning of the reception.
      */
    BYTE GetBurstDensity();
    
    /**Get the fraction of RTP packets lost/discarded within gap periods since
       the beginning of the reception.
      */
    BYTE GetGapDensity();
    
    /**Get the mean duration, in milliseconds, of burst periods since the
       beginning of the reception.
     */
    PUInt16b GetBurstDuration();
    
    /**Get the mean duration, in milliseconds, of gap periods since the beginning
       of the reception.
      */
    PUInt16b GetGapDuration();
    
    /**Get the most recently calculated round trip time between RTP interfaces,
       expressed in millisecond.
      */
    PUInt16b GetRoundTripDelay ();
    
    /**Get the most recently estimated end system delay, expressed in millisecond.
      */
    PUInt16b GetEndSystemDelay();
    
    /**Get the R factor for the current RTP session, expressed in the range of 0 to 100.
      */
    BYTE RFactor();
    
    /**Get the estimated MOS score for listening quality of the current RTP session,
       expressed in the range of 10 to 50.
       This metric not includes the effects of delay.
      */
    BYTE MOS_LQ();
    
    /**Get the estimated MOS score for conversational quality of the current RTP session,
       expressed in the range of 10 to 50.
       This metric includes the effects of delay.
      */
    BYTE MOS_CQ();
    
  private:
    /**Given the following definition of states: 
       state 1 = received a packet during a gap;
       state 2 = received a packet during a burst;
       state 3 = lost a packet during a burst;
       state 4 = lost a isolated packet during a gap.
      
       This function models the transitions of a Markov chain with the states above.
      */
    void markov(
      PacketEvent event   ///< Event triggered
    );
    
    /**Reset the counters used for the Markov chain
      */
    void ResetCounters();
    
    /**Get the R factor for the current RTP session, expressed in the range of 0 to 100.
      */
    BYTE RFactor(
      QualityType qt   ///< Listening or Conversational quality
    );
    
    /**Get the R factor at the end of the RTP session, expressed in the range of 0 to 100.
      */
    BYTE EndOfCallRFactor();
    
    /**Get the estimated MOS score for the current RTP session,
       expressed in the range of 1 to 5.
      */
    float MOS(
      QualityType qt   ///< Listening or Conversational quality
    );
    
    /**Get the estimated MOS score at the end of the RTP session,
       expressed in the range of 1 to 5.
      */
    float EndOfCallMOS();
    
    /**Get instantaneous Id factor.
      */
    float IdFactor();
    
    /**Get a ponderated value of Id factor.
      */
    float GetPonderateId();
    
    /**Get instantaneous Ieff factor.
      */
    float Ieff(
      PeriodType type   ///< The type of period (burst or gap).
    );
    
    /**Get a value of Ie factor at the end of the RTP session.
      */
    float GetEndOfCallIe();
    
    /**Get a ponderated value of Ie factor.
      */
    float GetPonderateIe();
    
    /**Create a a burst or gap period.
      */
    TimePeriod createTimePeriod(
      PeriodType type,        ///< The type of period (burst or gap).
      PTime beginTimestamp,   ///< Beginning of period timestamp.
      PTime endTimestamp      ///< End of period timestamp.
    );
    
    /**Create a period of time with an Id value associated.
      */
    IdPeriod createIdPeriod(
      PTime beginTimestamp,   ///< Beginning of period timestamp.
      PTime endTimestamp      ///< End of period timestamp.
    );
    
    /**Create a period of time with an Ie value associated.
      */
    IePeriod createIePeriod(
      TimePeriod timePeriod   ///< Period of time to calculate the Ie.
    );
    
    DWORD gmin;                    /* gap threshold */
    DWORD lost;                    /* number of lost packets within the current burst */
    DWORD packetsReceived;         /* packets received since the beggining of the reception */
    DWORD packetsSinceLastLoss;    /* packets received since the last loss or discard event */
    DWORD packetsLost;             /* packets lost since the beggining of the reception */
    DWORD packetsDiscarded;        /* packets discarded since the beggining of the receptions */
    DWORD srPacketsReceived;       /* count of SR packets received */
    
    DWORD packetsReceivedInGap;    /* packets received within gap periods */
    DWORD packetsLostInGap;        /* packets lost within gap periods */
        
    DWORD packetsReceivedInBurst;  /* packets received within burst periods */
    DWORD packetsLostInBurst;      /* packets lost within burst periods */
     
    /**Given the following definition of states: 
       state 1 = received a packet during a gap;
       state 2 = received a packet during a burst;
       state 3 = lost a packet during a burst;
       state 4 = lost a isolated packet during a gap.
       
       Variables below correspond to state transition counts.
       To reset these counters, call the ResetCounters() function.
      */
    DWORD c5;
    DWORD c11;
    DWORD c13;
    DWORD c14;
    DWORD c22;
    DWORD c23;
    DWORD c31;
    DWORD c32;
    DWORD c33;
    
    /* variables to calculate round trip delay */
    PTime         lsrTime;
    PTimeInterval dlsrTime;
    PTime         arrivalTime;
    
    /* data associated with the payload */
    float    Ie;              /* equipment impairment factor for the codec utilized */ 
    float    Bpl;             /* robustness factor for the codec utilized */
    unsigned payloadBitrate;
    PINDEX   payloadSize;
    float    lookAheadTime;   /* codec lookahead time */
    
    bool isPayloadTypeOK;
    
    DWORD jitterDelay;  /* jitter buffer delay, in milliseconds */
    float lastId;       /* last Id calculated */
    float lastIe;       /* last Ie calculated */
    
    PeriodType currentPeriodType;           /* indicates if we are within a gap or burst */
    PTime periodBeginTimestamp;             /* timestamp of the beginning of the gap or burst period */
    PTime lastLossTimestamp;                /* timestamp of the last loss */
    PTime lastLossInBurstTimestamp;         /* timestamp of the last loss within a burst period */
    PTime lastJitterBufferChangeTimestamp;  /* timestamp of the last change in jitter buffer size */
    
};


#endif // OPAL_RTCP_XR

#endif // OPAL_METRICS_H


/////////////////////////////////////////////////////////////////////////////
