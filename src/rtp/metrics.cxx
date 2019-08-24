/*
 * metrics.cxx
 *
 * E-Model implementation
 *
 * Open Phone Abstraction Library (OPAL)
 *
 * Copyright (c) 2010 Universidade Federal do Amazonas - BRAZIL
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
 */

#ifdef __GNUC__
#pragma implementation "metrics.h"
#endif

#include <ptlib.h>

#include <rtp/metrics.h>

#if OPAL_RTCP_XR

#include <rtp/rtp.h>
#include <rtp/jitter.h>

#include <math.h>


class RTP_DataFrame;

// RFC3611

RTCP_XR_Metrics::RTCP_XR_Metrics(float    Ie,
                                 float    Bpl,
                                 float    lookAheadTime,
                                 PINDEX   payloadSize,
                                 unsigned payloadBitrate
)
  : m_Ie(Ie)                     /* equipment impairment factor for the codec utilized */
  , m_Bpl(Bpl)                   /* robustness factor for the codec utilized */
  , m_lookAheadTime(lookAheadTime)   /* codec lookahead time */
  , m_payloadSize(payloadSize)
  , m_payloadBitrate(payloadBitrate)
  , m_gmin(16)                   /* gap threshold */
  , m_lostInBurst(1)             /* number of lost packets within the current burst */
  , m_packetsReceived(0)         /* packets received since the beggining of the reception */
  , m_packetsSinceLastLoss(0)    /* packets received since the last loss or discard event */
  , m_packetsLost(0)             /* packets lost since the beggining of the reception */
  , m_packetsDiscarded(0)        /* packets discarded since the beggining of the receptions */
  , m_srPacketsReceived(0)       /* count of SR packets received */
  , m_packetsReceivedInGap(0)    /* packets received within gap periods */
  , m_packetsLostInGap(0)        /* packets lost within gap periods */
  , m_packetsReceivedInBurst(0)  /* packets received within burst periods */
  , m_packetsLostInBurst(0)      /* packets lost within burst periods */
  , c5(0)
  , c11(0)
  , c13(0)
  , c14(0)
  , c22(0)
  , c23(0)
  , c31(0)
  , c32(0)
  , c33(0)
  , m_jitterDelay(0)  /* jitter buffer delay, in milliseconds */
  , m_lastId(0)       /* last Id calculated */
  , m_lastIe(0)       /* last Ie calculated */
  , m_currentPeriodType(GAP) /* indicates if we are within a gap or burst */
{		
  PTRACE(4, "VoIP Metrics\tRTCP_XR_Metrics created.");
}


RTCP_XR_Metrics::~RTCP_XR_Metrics()
{
  PTRACE_IF(3, m_packetsReceived != 0 || m_packetsLost != 0,
            "VoIP Metrics\tRTCP_XR_Metrics final statistics:\n"
            "   R Factor = "<< GetEndOfCallRFactor() << "\n"
            "   MOS = "<< GetEndOfCallMOS());
}


void RTCP_XR_Metrics::ResetCounters()
{
  c5 = 0;
  c11 = 0;
  c13 = 0;
  c14 = 0;
  c22 = 0;
  c23 = 0;
  c31 = 0;
  c32 = 0;
  c33 = 0;
}


RTCP_XR_Metrics * RTCP_XR_Metrics::Create(const RTP_DataFrame & frame)
{
  /* Get Ie and Bpl for the coded utilized, according to ITU-T G.113 */
  switch (frame.GetPayloadType()) {
    case RTP_DataFrame::PCMU:
    case RTP_DataFrame::PCMA:
      return new RTCP_XR_Metrics(0, 25.1f, 0, frame.GetPayloadSize(), 64000);

    case RTP_DataFrame::G7231:
      return new RTCP_XR_Metrics(15, 16.1f, 7.5f, 24, 6300);

    case RTP_DataFrame::G729:
      return new RTCP_XR_Metrics(11, 19, 5, 10, 8000);

    case RTP_DataFrame::GSM:
      return new RTCP_XR_Metrics(20, 10, 5, 33, 13000);

    default:
      PTRACE(3, "VoIP Metrics", "No Ie and Bpl data for payload type " << frame.GetPayloadType() <<
                ", unable to calculate R Factor and MOS score.");
      return NULL;
  }
}


void RTCP_XR_Metrics::SetJitterDelay(unsigned delay)
{
  m_jitterDelay = delay;

  /* If the Id factor has changed, create a new Id period */
  if (fabs(GetIdFactor() - m_lastId) < 1e-14) {
    PTime now;
    CreateIdPeriod(m_lastJitterBufferChangeTimestamp, now);
    m_lastJitterBufferChangeTimestamp = now;
    m_lastId = GetIdFactor();
  }
}


unsigned RTCP_XR_Metrics::GetLossRate() const
{
  unsigned packets = m_packetsReceived + m_packetsLost + m_packetsDiscarded;
  return packets > 0 ? m_packetsLost*256/packets : 0;
}


unsigned RTCP_XR_Metrics::GetDiscardRate() const
{
  unsigned packets = m_packetsReceived + m_packetsLost + m_packetsDiscarded;
  return packets > 0 ? m_packetsDiscarded*256/packets : 0;
}


unsigned RTCP_XR_Metrics::GetBurstDensity() const
{
  unsigned packets = m_packetsReceivedInBurst + m_packetsLostInBurst;
  return packets > 0 ? (m_packetsLostInBurst*256+packets-1)/packets : 0;
}


unsigned RTCP_XR_Metrics::GetGapDensity() const
{
  unsigned packets = m_packetsReceivedInGap + m_packetsLostInGap + m_packetsSinceLastLoss;
  return packets > 0 ? (m_packetsLostInGap*256+packets-1)/packets : 0;
}


unsigned RTCP_XR_Metrics::GetBurstDuration() const
{
  unsigned averageDuration = 0;
  uint64_t totalDuration = 0;
  unsigned count = 0;

  /* If we are within a burst account for it */
  if (m_currentPeriodType == BURST) {
    totalDuration = totalDuration + (m_lastLossTimestamp - m_periodBeginTimestamp).GetMilliSeconds();
    count ++;
  }

  /* Iterate over the list of periods to calculate a mean duration */
  for (list<TimePeriod>::const_iterator period = m_timePeriods.begin(); period != m_timePeriods.end(); period++) {
    if ((*period).type == BURST) {
      totalDuration = totalDuration + (*period).duration.GetMilliSeconds();
      count++;
    }
  }

  return (unsigned)(count > 0 ? (totalDuration/count) : averageDuration);
}


unsigned RTCP_XR_Metrics::GetGapDuration() const
{
  unsigned averageDuration = 0;
  uint64_t totalDuration = 0;
  unsigned count = 0;

  /* If we are within a burst, the last received packets after the last loss are assumed to be in gap */
  PTime now;
  if (m_currentPeriodType == BURST) {
    totalDuration = totalDuration + (now - m_lastLossTimestamp).GetMilliSeconds();
    count ++;
  }
  else {
    /* If we are in a gap, just account for it */
    totalDuration = totalDuration + (now - m_periodBeginTimestamp).GetMilliSeconds();
    count++;
  }

  /* Iterate over the list of periods to calculate a mean duration */
  for (list<TimePeriod>::const_iterator period = m_timePeriods.begin(); period != m_timePeriods.end(); period++) {
    if ((*period).type == GAP) {
      totalDuration = totalDuration + (*period).duration.GetMilliSeconds();
      count++;
    }
  }

  return (unsigned)(count > 0 ? (totalDuration/count) : averageDuration);
}


unsigned RTCP_XR_Metrics::GetRoundTripDelay() const
{
  if (m_srPacketsReceived == 0 || !m_lsrTime.IsValid() || m_dlsrTime == 0)
    return 0;

  /* calculate the round trip delay, according to RFC 3611 */
  PTimeInterval roundTripDelay;
  roundTripDelay = m_arrivalTime - m_lsrTime;
  roundTripDelay = roundTripDelay - m_dlsrTime;

  return (WORD)roundTripDelay.GetMilliSeconds();
}


unsigned RTCP_XR_Metrics::GetEndSystemDelay() const
{
  if (m_payloadBitrate == 0)
    return 0;

  /* Account 2 times for the codec time (sender and receiver sides) */
  return (WORD)(2*((m_payloadSize*8)/m_payloadBitrate)*1000 + m_lookAheadTime + m_jitterDelay);
}


unsigned RTCP_XR_Metrics::GetRFactor() const
{
  /* The reported R factor is for conversational listening quality */
  return GetRFactor(CQ);
}


unsigned RTCP_XR_Metrics::GetRFactor(QualityType qt) const
{
  if (m_payloadBitrate == 0)
    return 127;

  double R;

  /* Compute R factor, according to ITU-T G.107 */
  switch (qt) {
    case CQ:
      /* Include the delay effects */
      R = 93.4 - GetPonderateId() - GetPonderateIe();
      break;

    case LQ:
      R = 93.4 - GetPonderateIe();
      break;

    default:
      R = 127;
  }

  return (unsigned)ceil(R);
}


unsigned RTCP_XR_Metrics::GetEndOfCallRFactor() const
{
  if (m_payloadBitrate == 0)
    return 127;

  /* Compute end of call R factor, according to the extended E-Model */
  double R = 93.4 - GetPonderateId() - GetEndOfCallIe();
  return (unsigned)ceil(R);
}


unsigned RTCP_XR_Metrics::GetMOS_LQ() const
{
  if (m_payloadBitrate == 0)
    return 127;

  /* RTCP-XR requires MOS score in the range of 10 to 50 */
  return (unsigned)ceil(GetMOS(LQ)*10);
}


unsigned RTCP_XR_Metrics::GetMOS_CQ() const
{
  if (m_payloadBitrate == 0)
    return 127;

  /* RTCP-XR requires MOS score in the range of 10 to 50 */
  return (unsigned)ceil(GetMOS(CQ)*10);
}


float RTCP_XR_Metrics::GetMOS(QualityType qt) const
{
  double R = GetRFactor(qt);

  /* Compute MOS, according to ITU-T G.107 */
  if (R <= 6.5153)
    return 1;

  if ((6.5153 < R) && (R < 100.))
    return (float)(1.0 + (0.035 * R) + (R * (R - 60.0) * (100.0 - R) * 7.0 * pow(10.0, -6.0)));

  if (R >= 100)
    return 4.5;

  return 0;
}


float RTCP_XR_Metrics::GetEndOfCallMOS() const
{
  double R = GetEndOfCallRFactor();

  /* Compute MOS, according to ITU-T G.107 */
  if (R <= 6.5153)
    return 1;

  if ((6.5153 < R) && (R < 100.))
    return (float)(1.0 + (0.035 * R) + (R * (R - 60.0) * (100.0 - R) * 7.0 * pow(10.0, -6.0)));

  if (R >= 100.)
    return 4.5;

  return 0;
}


float RTCP_XR_Metrics::GetIdFactor() const
{
  float delay = (float) GetEndSystemDelay();

  /* Calculate Id with an aproximate equation, for computational sakeness */
  if (delay < 177.3)
    return 0.024f * delay;

  if (delay < 300)
    return 0.024f * delay + 0.11f * (delay - 177.3f);

  if (delay < 600)
    return -2.468f    * pow(10.0f, -14.0f) * pow(delay, 6.0f) +
           (5.062f    * pow(10.0f, -11.0f) * pow(delay, 5.0f)) -
           (3.903f    * pow(10.0f, -8.0f)  * pow(delay, 4.0f)) +
           (1.344f    * pow(10.0f, -5.0f)  * pow(delay, 3.0f)) -
           (0.001802f * delay              * delay           ) +
           (0.103f    * delay - 0.1698f);

  return 44;
}


float RTCP_XR_Metrics::GetPonderateId() const
{
  float   ponderateId = 0;
  float   sumId = 0;
  PUInt64 sumDuration = 0;
  DWORD   count = 0;

  /* Account for the current time and Id value */
  PTime now;
  sumId = GetIdFactor() * (now - m_lastJitterBufferChangeTimestamp).GetMilliSeconds();
  sumDuration = sumDuration + (now - m_lastJitterBufferChangeTimestamp).GetMilliSeconds();
  count++;

  /* Iterate over the list of periods to calculate an average Id */
  for (list<IdPeriod>::const_iterator period = m_idPeriods.begin(); period != m_idPeriods.end(); period++) {
    sumId = sumId + (*period).Id * (*period).duration.GetMilliSeconds();
    sumDuration = sumDuration + (*period).duration.GetMilliSeconds();
    count++;
  }

  if (count != 0 && sumDuration != 0) {
    ponderateId = sumId/sumDuration;
  }
  return ponderateId;
}


float RTCP_XR_Metrics::GetIeff(PeriodType type) const
{
  float Ppl = 0;

  /* Compute Ieff factor, according to ITU-T G.107 */
  if (type == GAP )
  {
    if (c11 + c14 == 0) {
      Ppl = 0;
    }
    else {
      Ppl = c14 * 100.0f / (c11 + c14);
    }
  }
  else if (type == BURST)
  {
    if (c13 + c23 + c33 + c22 == 0) {
      Ppl = 0;
    }
    else {
      Ppl = (c13 + c23 + c33) * 100.0f / (c13 + c23 + c33 + c22);
    }
  }
  return (m_Ie + (95 - m_Ie) * Ppl/(Ppl + m_Bpl));
}


float RTCP_XR_Metrics::GetEndOfCallIe() const
{
  /* Calculate the time since the last burst period */
  PTime now;
  PInt64 y = (now - m_lastLossInBurstTimestamp).GetMilliSeconds();

  /* Compute end of call Ie factor, according to the extended E-Model */
  return GetPonderateIe() + 0.7f * (m_lastIe - GetPonderateIe()) * exp((float)(-y/30000.0));
}


float RTCP_XR_Metrics::GetPonderateIe() const
{
  float   ponderateIe = 0;
  float   sumIe = 0;
  PUInt64 sumDuration = 0;
  DWORD   count = 0;

  /* Account for the current time and Ie value */
  PTime now;
  ponderateIe = GetIeff(m_currentPeriodType) * (now - m_periodBeginTimestamp).GetMilliSeconds();
  sumDuration = sumDuration + (now - m_periodBeginTimestamp).GetMilliSeconds();
  count++;

  /* Iterate over the list of periods to calculate an average Ie */
  for (list<IePeriod>::const_iterator period = m_iePeriods.begin(); period != m_iePeriods.end(); period++) {
    sumIe = (*period).Ieff * (*period).duration.GetMilliSeconds();
    sumDuration = sumDuration + (*period).duration.GetMilliSeconds();
    count++;
  }

  if (count != 0 && sumDuration != 0) {
    ponderateIe = sumIe/sumDuration;
  }
  return ponderateIe;
}


RTCP_XR_Metrics::TimePeriod RTCP_XR_Metrics::CreateTimePeriod(PeriodType type, const PTime & beginTimestamp, const PTime & endTimestamp)
{
  TimePeriod newPeriod;

  newPeriod.type = type;
  newPeriod.duration = endTimestamp - beginTimestamp;

  m_timePeriods.push_back(newPeriod);

  return newPeriod;
}


RTCP_XR_Metrics::IdPeriod RTCP_XR_Metrics::CreateIdPeriod(const PTime & beginTimestamp, const PTime & endTimestamp)
{
  IdPeriod newPeriod;

  /* Get the current Id value */
  newPeriod.Id = GetIdFactor();
  newPeriod.duration = endTimestamp - beginTimestamp;

  m_idPeriods.push_back(newPeriod);

  return newPeriod;
}


RTCP_XR_Metrics::IePeriod RTCP_XR_Metrics::CreateIePeriod(const RTCP_XR_Metrics::TimePeriod & timePeriod)
{
  /* Calculate a perceptual Ie average value, according to the extended E-Model, presented by Alan Clark */
  float Ieg = 0;
  float Ieb = 0;
  float I1 = 0;
  float I2 = 0;
  float averageIe = 0;
  PInt64 g = 0;
  PInt64 b = 0;

  IePeriod newPeriod;

  newPeriod.type = timePeriod.type;
  newPeriod.duration = timePeriod.duration;
  newPeriod.Ieff = GetIeff(newPeriod.type);

  if (newPeriod.type == BURST)
  {
    if (!m_iePeriods.empty())
    {
      /* Get the last period in the list */
      list<IePeriod>::reverse_iterator period = m_iePeriods.rbegin();
      IePeriod & lastPeriod = (*period);

      /* If the last period was a gap, calculate an perceptual average Ie value */
      if (lastPeriod.type == GAP)
      {
        float T1 = 5000.0;
        float T2 = 15000.0;

        Ieg = lastPeriod.Ieff;
        I1 = m_lastIe;
        g = lastPeriod.duration.GetMilliSeconds();
        I2 = Ieg + (I1 - Ieg) * exp(-g/T2);

        Ieb = newPeriod.Ieff;
        b = newPeriod.duration.GetMilliSeconds();
        I1 = Ieb - (Ieb - I2) * exp(-b/T1);

        m_lastIe = I1;

        averageIe = (b * Ieb + g * Ieg - T1 * (Ieb - I2) * (1 - exp(-b/T1)) + T2 * (I1 - Ieg) * (1 -  exp(-g/T2))) / (b + g);

        lastPeriod.Ieff = averageIe;
        newPeriod.Ieff = averageIe;
      }
    }
  }
  /* Insert the new period on the list */
  m_iePeriods.push_back(newPeriod);

  return newPeriod;
}


void RTCP_XR_Metrics::OnPacketReceived()
{
  m_packetsReceived++;
  m_packetsSinceLastLoss++;
  Markov(PACKET_RECEIVED);
}


void RTCP_XR_Metrics::OnPacketDiscarded()
{
  m_packetsDiscarded++;
  Markov(PACKET_DISCARDED);
}


void RTCP_XR_Metrics::OnPacketLost()
{
  m_packetsLost++;
  Markov(PACKET_LOST);
}


void RTCP_XR_Metrics::OnPacketLost(unsigned dropped)
{
  for (DWORD i = 0; i < dropped; i++)
    OnPacketLost();
}


void RTCP_XR_Metrics::OnRxSenderReport(const PTime & lastTimestamp, const PTimeInterval & delay)
{
  m_arrivalTime.SetCurrentTime();

  /* Get the timestamp of the last SR */
  if (lastTimestamp.IsValid())
    m_lsrTime = lastTimestamp;

  /* Get the delay since last SR */
  if (delay != 0)
    m_dlsrTime = delay;

  m_srPacketsReceived++;
}


void RTCP_XR_Metrics::Markov(RTCP_XR_Metrics::PacketEvent event)
{
  if (m_packetsReceived == 0) {
    m_periodBeginTimestamp.SetCurrentTime();
    m_currentPeriodType = GAP;
  }

  switch (event) {
    case PACKET_LOST :
    case PACKET_DISCARDED :
      c5 += m_packetsSinceLastLoss;

      if (m_packetsSinceLastLoss >= m_gmin || m_packetsReceived <= m_gmin) {

        if (m_currentPeriodType == BURST) {
          /* The burst period ended with the last packet loss */
          CreateIePeriod(CreateTimePeriod(m_currentPeriodType, m_periodBeginTimestamp, m_lastLossTimestamp));
          /* mark as a gap */
          m_currentPeriodType = GAP;
          m_periodBeginTimestamp = m_lastLossTimestamp;
          ResetCounters();
        }

        if (m_lostInBurst == 1) {
          c14++;
          m_packetsLostInGap++;
        }
        else {	
          c13++;
          m_packetsLostInBurst++;
        }
        m_lostInBurst = 1;
        c11 += m_packetsSinceLastLoss;
        m_packetsReceivedInGap += m_packetsSinceLastLoss;
      }
      break;

    case PACKET_RECEIVED :
      if (m_currentPeriodType == GAP) {
        /* The burst period ended with the packet received before the last loss */
        CreateIePeriod(CreateTimePeriod(m_currentPeriodType, m_periodBeginTimestamp, m_lastLossTimestamp));
         /* mark as a burst */
        m_currentPeriodType = BURST;
        m_periodBeginTimestamp = m_lastLossTimestamp;
        ResetCounters();
      }

      m_lastLossInBurstTimestamp.SetCurrentTime();
      ++m_lostInBurst;

      if (m_lostInBurst > 8) {
        c5 = 0;
      }
      if (m_packetsSinceLastLoss == 0) {
        c33++;
      }
      else {
        c23++;
        c22 += (m_packetsSinceLastLoss - 1);
        m_packetsReceivedInBurst += m_packetsSinceLastLoss;
      }
      m_packetsLostInBurst++;

      m_packetsSinceLastLoss = 0;
      m_lastLossTimestamp.SetCurrentTime();
  }

  /* calculate additional transaction counts */
  c31 = c13;
  c32 = c23;
}


void RTCP_XR_Metrics::InsertMetricsReport(RTP_ControlFrame & report,
                                          const OpalRTPSession & PTRACE_PARAM(session),
                                          RTP_SyncSourceId syncSourceOut,
                                          OpalJitterBuffer * jitter)
{
  report.StartNewPacket(RTP_ControlFrame::e_ExtendedReport);
  report.SetPayloadSize(sizeof(PUInt32b) + sizeof(RTP_ControlFrame::MetricsReport));  // length is SSRC of packet sender plus MR
  report.SetCount(1);
  BYTE * payload = report.GetPayloadPtr();

  // add the SSRC to the start of the payload
  *(PUInt32b *)payload = syncSourceOut;
  
  RTP_ControlFrame::MetricsReport & xr = *(RTP_ControlFrame::MetricsReport *)(payload+4);
  
  xr.bt = 0x07; // Code for Metrics Report
  xr.type_specific = 0x00;
  xr.length = (sizeof(RTP_ControlFrame::MetricsReport)-sizeof(RTP_ControlFrame::ExtendedReport)+3)/4;
  xr.ssrc = syncSourceOut;
  
  xr.loss_rate = (uint8_t)GetLossRate();
  xr.discard_rate = (uint8_t)GetDiscardRate();
  xr.burst_density = (uint8_t)GetBurstDensity();
  xr.gap_density = (uint8_t)GetGapDensity();
  xr.burst_duration = (uint16_t)GetBurstDuration();
  xr.gap_duration = (uint16_t)GetGapDuration();
  xr.round_trip_delay = (uint16_t)GetRoundTripDelay();
  xr.end_system_delay = (uint16_t)GetEndSystemDelay();
  xr.signal_level = 0x7F;
  xr.noise_level = 0x7F;
  xr.rerl = 0x7F;	
  xr.gmin = 16;
  xr.r_factor = (uint8_t)GetRFactor();
  xr.ext_r_factor = 0x7F;
  xr.mos_lq = (uint8_t)GetMOS_LQ();
  xr.mos_cq = (uint8_t)GetMOS_CQ();
  xr.rx_config = 0x00;
  xr.reserved = 0x00;

  if (jitter != NULL) {
    xr.jb_nominal = (uint16_t)(jitter->GetMinJitterDelay()/jitter->GetTimeUnits());
    xr.jb_maximum = (uint16_t)(jitter->GetCurrentJitterDelay()/jitter->GetTimeUnits());
    xr.jb_absolute = (uint16_t)(jitter->GetMaxJitterDelay()/jitter->GetTimeUnits());
  }
  
  report.EndPacket();
  
  PTRACE(3, "RTP", session << "SentExtendedReport:"
            " ssrc=" << RTP_TRACE_SRC(xr.ssrc)
         << " loss_rate=" << (unsigned)xr.loss_rate
         << " discard_rate=" << (unsigned)xr.discard_rate
         << " burst_density=" << (unsigned)xr.burst_density
         << " gap_density=" << (unsigned)xr.gap_density
         << " burst_duration=" << xr.burst_duration
         << " gap_duration=" << xr.gap_duration
         << " round_trip_delay="<< xr.round_trip_delay
         << " end_system_delay="<< xr.end_system_delay
         << " gmin="<< (unsigned)xr.gmin
         << " r_factor="<< (unsigned)xr.r_factor
         << " mos_lq="<< (unsigned)xr.mos_lq
         << " mos_cq="<< (unsigned)xr.mos_cq
         << " jb_nominal_delay="<< xr.jb_nominal
         << " jb_maximum_delay="<< xr.jb_maximum
         << " jb_absolute_delay="<< xr.jb_absolute);

}


RTP_MetricsReport::RTP_MetricsReport(const RTP_ControlFrame::MetricsReport & mr)
  : sourceIdentifier(mr.ssrc)
  , lossRate(mr.loss_rate*100.0f/256.0f)
  , discardRate(mr.discard_rate*100.0f/256.0f)
  , burstDensity(mr.burst_density*100.0f/256.0f)
  , gapDensity(mr.gap_density*100.0f/256.0f)
  , roundTripDelay(mr.round_trip_delay)
  , RFactor(mr.r_factor)
  , mosLQ(mr.mos_lq)
  , mosCQ(mr.mos_cq)
  , jbNominal(mr.jb_nominal)
  , jbMaximum(mr.jb_maximum)
  , jbAbsolute(mr.jb_absolute)
{
}


void OpalRTPSession::OnRxMetricsReport(RTP_SyncSourceId PTRACE_PARAM(ssrc), const RTP_MetricsReport & report)
{
  PTRACE(4, "RTP", *this << "OnMetricsReport: ssrc=" << RTP_TRACE_SRC(ssrc) << ' ' << report);
  SyncSource * info;
  if (GetSyncSource(report.sourceIdentifier, e_Sender, info)) {
    info->m_rtcpDiscardRate = report.discardRate;
    info->m_rtcpJitterBufferDelay = report.jbNominal;
  }
}


#if PTRACING
void RTP_MetricsReport::PrintOn(ostream & strm) const
{
  strm << "ssrc=" << RTP_TRACE_SRC(sourceIdentifier)
       << " loss_rate=" << lossRate
       << " discard_rate=" << discardRate
       << " burst_density=" << burstDensity
       << " gap_density=" << gapDensity
       << " round_trip_delay=" << roundTripDelay
       << " r_factor=" << RFactor
       << " mos_lq=" << mosLQ
       << " mos_cq=" << mosCQ
       << " jb_nominal=" << jbNominal
       << " jb_maximum=" << jbMaximum
       << " jb_absolute=" << jbAbsolute;
}
#endif // PTRACING


#endif // OPAL_RTCP_XR
