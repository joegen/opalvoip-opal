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
 * $Revision$
 * $Author$
 * $Date$
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


const unsigned SecondsFrom1900to1970 = (70*365+17)*24*60*60U;

class RTP_DataFrame;

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
            "   R Factor = "<< EndOfCallRFactor() << "\n"
            "   MOS = "<< EndOfCallMOS());
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
      PTRACE(3, NULL, "VoIP Metrics", "No Ie and Bpl data for payload type " << frame.GetPayloadType() <<
                ", unable to calculate R Factor and MOS score.");
      return NULL;
  }
}


void RTCP_XR_Metrics::SetJitterDelay(DWORD delay)
{
  m_jitterDelay = delay;

  /* If the Id factor has changed, create a new Id period */
  if (fabs(IdFactor() - m_lastId) < 1e-14) {
    PTime now;
    createIdPeriod(m_lastJitterBufferChangeTimestamp, now);
    m_lastJitterBufferChangeTimestamp = now;
    m_lastId = IdFactor();
  }
}


BYTE RTCP_XR_Metrics::GetLossRate()
{
  unsigned packets = m_packetsReceived + m_packetsLost + m_packetsDiscarded;
  if (packets == 0)
    return 0;

  return (BYTE)(256 * (float)m_packetsLost/packets);
}


BYTE RTCP_XR_Metrics::GetDiscardRate()
{
  unsigned packets = m_packetsReceived + m_packetsLost + m_packetsDiscarded;
  if (packets == 0)
    return 0;

  return (BYTE)(256 * (float)m_packetsDiscarded/packets);
}


BYTE RTCP_XR_Metrics::GetBurstDensity()
{
  unsigned packets = m_packetsReceivedInBurst + m_packetsLostInBurst;
  if (packets == 0)
    return 0;

  return (BYTE)(ceil(256 * ((float)m_packetsLostInBurst)/packets));
}


BYTE RTCP_XR_Metrics::GetGapDensity()
{
  unsigned packets = m_packetsReceivedInGap + m_packetsLostInGap + m_packetsSinceLastLoss;
  if (packets == 0) {
    return 0;
  }
  return (BYTE)(ceil(256 * ((float)m_packetsLostInGap)/packets));
}


PUInt16b RTCP_XR_Metrics::GetBurstDuration()
{
  unsigned averageDuration = 0;
  PUInt64  totalDuration = 0;
  DWORD    count = 0;

  /* If we are within a burst account for it */
  if (m_currentPeriodType == BURST) {
    totalDuration = totalDuration + (m_lastLossTimestamp - m_periodBeginTimestamp).GetMilliSeconds();
    count ++;
  }

  /* Iterate over the list of periods to calculate a mean duration */
  for (list<TimePeriod>::iterator period = m_timePeriods.begin(); period != m_timePeriods.end(); period++) {
    if ((*period).type == BURST) {
      totalDuration = totalDuration + (*period).duration.GetMilliSeconds();
      count++;
    }
  }

  return (WORD)(count > 0 ? (totalDuration/count) : averageDuration);
}


PUInt16b RTCP_XR_Metrics::GetGapDuration()
{
  unsigned averageDuration = 0;
  PUInt64  totalDuration = 0;
  DWORD    count = 0;

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
  for (list<TimePeriod>::iterator period = m_timePeriods.begin(); period != m_timePeriods.end(); period++) {
    if ((*period).type == GAP) {
      totalDuration = totalDuration + (*period).duration.GetMilliSeconds();
      count++;
    }
  }

  return (WORD)(count > 0 ? (totalDuration/count) : averageDuration);
}


PUInt16b RTCP_XR_Metrics::GetRoundTripDelay()
{
  if (m_srPacketsReceived == 0 || !m_lsrTime.IsValid() || m_dlsrTime == 0)
    return 0;

  /* calculate the round trip delay, according to RFC 3611 */
  PTimeInterval roundTripDelay;
  roundTripDelay = m_arrivalTime - m_lsrTime;
  roundTripDelay = roundTripDelay - m_dlsrTime;

  return (WORD)roundTripDelay.GetMilliSeconds();
}


PUInt16b RTCP_XR_Metrics::GetEndSystemDelay()
{
  if (m_payloadBitrate == 0)
    return 0;

  /* Account 2 times for the codec time (sender and receiver sides) */
  return (WORD)(2*((m_payloadSize*8)/m_payloadBitrate)*1000 + m_lookAheadTime + m_jitterDelay);
}


BYTE RTCP_XR_Metrics::RFactor()
{
  /* The reported R factor is for conversational listening quality */
  return RFactor(CQ);
}


BYTE RTCP_XR_Metrics::RFactor(QualityType qt)
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

  return (BYTE)ceil(R);
}


BYTE RTCP_XR_Metrics::EndOfCallRFactor()
{
  if (m_payloadBitrate == 0)
    return 127;

  /* Compute end of call R factor, according to the extended E-Model */
  double R = 93.4 - GetPonderateId() - GetEndOfCallIe();
  return (BYTE)ceil(R);
}


BYTE RTCP_XR_Metrics::MOS_LQ()
{
  if (m_payloadBitrate == 0)
    return 127;

  /* RTCP-XR requires MOS score in the range of 10 to 50 */
  return (BYTE)ceil(MOS(LQ)*10);
}


BYTE RTCP_XR_Metrics::MOS_CQ()
{
  if (m_payloadBitrate == 0)
    return 127;

  /* RTCP-XR requires MOS score in the range of 10 to 50 */
  return (BYTE)ceil(MOS(CQ)*10);
}


float RTCP_XR_Metrics::MOS(QualityType qt)
{
  float R = RFactor(qt);

  /* Compute MOS, according to ITU-T G.107 */
  if (R <= 6.5153)
    return 1;

  if ((6.5153 < R) && (R < 100.))
    return 1.0f + (0.035f * R) + (R * (R - 60.0f) * (100.0f - R) * 7.0f * pow(10.0f, -6.0f));

  if (R >= 100)
    return 4.5;

  return 0;
}


float RTCP_XR_Metrics::EndOfCallMOS()
{
  float R = EndOfCallRFactor();

  /* Compute MOS, according to ITU-T G.107 */
  if (R <= 6.5153)
    return 1;

  if ((6.5153 < R) && (R < 100.))
    return 1.0f + (0.035f * R) + (R * (R - 60.0f) * (100.0f - R) * 7.0f * pow(10.0f, -6.0f));

  if (R >= 100.)
    return 4.5;

  return 0;
}


float RTCP_XR_Metrics::IdFactor()
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


float RTCP_XR_Metrics::GetPonderateId()
{
  float   ponderateId = 0;
  float   sumId = 0;
  PUInt64 sumDuration = 0;
  DWORD   count = 0;

  /* Account for the current time and Id value */
  PTime now;
  sumId = IdFactor() * (now - m_lastJitterBufferChangeTimestamp).GetMilliSeconds();
  sumDuration = sumDuration + (now - m_lastJitterBufferChangeTimestamp).GetMilliSeconds();
  count++;

  /* Iterate over the list of periods to calculate an average Id */
  for (list<IdPeriod>::iterator period = m_idPeriods.begin(); period != m_idPeriods.end(); period++) {
    sumId = sumId + (*period).Id * (*period).duration.GetMilliSeconds();
    sumDuration = sumDuration + (*period).duration.GetMilliSeconds();
    count++;
  }

  if (count != 0 && sumDuration != 0) {
    ponderateId = sumId/sumDuration;
  }
  return ponderateId;
}


float RTCP_XR_Metrics::Ieff(PeriodType type)
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


float RTCP_XR_Metrics::GetEndOfCallIe()
{
  /* Calculate the time since the last burst period */
  PTime now;
  PInt64 y = (now - m_lastLossInBurstTimestamp).GetMilliSeconds();

  /* Compute end of call Ie factor, according to the extended E-Model */
  return GetPonderateIe() + 0.7f * (m_lastIe - GetPonderateIe()) * exp((float)(-y/30000.0));
}


float RTCP_XR_Metrics::GetPonderateIe()
{
  float   ponderateIe = 0;
  float   sumIe = 0;
  PUInt64 sumDuration = 0;
  DWORD   count = 0;

  /* Account for the current time and Ie value */
  PTime now;
  ponderateIe = Ieff(m_currentPeriodType) * (now - m_periodBeginTimestamp).GetMilliSeconds();
  sumDuration = sumDuration + (now - m_periodBeginTimestamp).GetMilliSeconds();
  count++;

  /* Iterate over the list of periods to calculate an average Ie */
  for (list<IePeriod>::iterator period = m_iePeriods.begin(); period != m_iePeriods.end(); period++) {
    sumIe = (*period).Ieff * (*period).duration.GetMilliSeconds();
    sumDuration = sumDuration + (*period).duration.GetMilliSeconds();
    count++;
  }

  if (count != 0 && sumDuration != 0) {
    ponderateIe = sumIe/sumDuration;
  }
  return ponderateIe;
}


RTCP_XR_Metrics::TimePeriod RTCP_XR_Metrics::createTimePeriod(PeriodType type, PTime beginTimestamp, PTime endTimestamp)
{
  TimePeriod newPeriod;

  newPeriod.type = type;
  newPeriod.duration = endTimestamp - beginTimestamp;

  m_timePeriods.push_back(newPeriod);

  return newPeriod;
}


RTCP_XR_Metrics::IdPeriod RTCP_XR_Metrics::createIdPeriod(PTime beginTimestamp, PTime endTimestamp)
{
  IdPeriod newPeriod;

  /* Get the current Id value */
  newPeriod.Id = IdFactor();
  newPeriod.duration = endTimestamp - beginTimestamp;

  m_idPeriods.push_back(newPeriod);

  return newPeriod;
}


RTCP_XR_Metrics::IePeriod RTCP_XR_Metrics::createIePeriod(RTCP_XR_Metrics::TimePeriod timePeriod)
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
  newPeriod.Ieff = Ieff(newPeriod.type);

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
  markov(PACKET_RECEIVED);
}


void RTCP_XR_Metrics::OnPacketDiscarded()
{
  m_packetsDiscarded++;
  markov(PACKET_DISCARDED);
}


void RTCP_XR_Metrics::OnPacketLost()
{
  m_packetsLost++;
  markov(PACKET_LOST);
}


void RTCP_XR_Metrics::OnPacketLost(DWORD dropped)
{
  for (DWORD i = 0; i < dropped; i++)
    OnPacketLost();
}


void RTCP_XR_Metrics::OnRxSenderReport(PUInt32b lsr, PUInt32b dlsr)
{
  PTime now;

  /* Convert the arrival time to NTP timestamp */
  PUInt32b arrival_ntp_sec  = (DWORD)(now.GetTimeInSeconds()+SecondsFrom1900to1970); // Convert from 1970 to 1900
  PUInt32b arrival_ntp_frac = now.GetMicrosecond()*4294; // Scale microseconds to "fraction" from 0 to 2^32

  /* Take the middle 32 bits of NTP timestamp and break in seconds and microseconds */
  arrival_ntp_sec = ((arrival_ntp_sec) & 0x0000FFFF);
  arrival_ntp_frac = ((arrival_ntp_frac) & 0xFFFF0000);

  m_arrivalTime = PTime(arrival_ntp_sec - SecondsFrom1900to1970, arrival_ntp_frac/4294);

  if (lsr != 0) {
    /* Get the timestamp of the last SR */
    PUInt32b lsr_ntp_sec = ((lsr >> 16) & 0x0000FFFF);
    PUInt32b lsr_ntp_frac = ((lsr << 16) & 0xFFFF0000);
    m_lsrTime = PTime(lsr_ntp_sec - SecondsFrom1900to1970, lsr_ntp_frac/4294);
  }

  if (dlsr != 0) {
    /* Get the delay since last SR */
    m_dlsrTime.SetInterval (dlsr*1000/65536);
  }

  m_srPacketsReceived++;
}


void RTCP_XR_Metrics::markov(RTCP_XR_Metrics::PacketEvent event)
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
          createIePeriod(createTimePeriod(m_currentPeriodType, m_periodBeginTimestamp, m_lastLossTimestamp));
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
        createIePeriod(createTimePeriod(m_currentPeriodType, m_periodBeginTimestamp, m_lastLossTimestamp));
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


void RTCP_XR_Metrics::InsertExtendedReportPacket(unsigned PTRACE_PARAM(sessionID),
                                                 DWORD syncSourceOut,
                                                 OpalRTPSession::JitterBufferPtr jitter,
                                                 RTP_ControlFrame & report)
{
  report.StartNewPacket();
  report.SetPayloadType(RTP_ControlFrame::e_ExtendedReport);
  report.SetPayloadSize(sizeof(PUInt32b) + sizeof(RTP_ControlFrame::ExtendedReport));  // length is SSRC of packet sender plus XR
  report.SetCount(1);
  BYTE * payload = report.GetPayloadPtr();

  // add the SSRC to the start of the payload
  *(PUInt32b *)payload = syncSourceOut;
  
  RTP_ControlFrame::ExtendedReport & xr = *(RTP_ControlFrame::ExtendedReport *)(payload+4);
  
  xr.bt = 0x07;
  xr.type_specific = 0x00;
  xr.length = 0x08;
  xr.ssrc = syncSourceOut;
  
  xr.loss_rate = GetLossRate();
  xr.discard_rate = GetDiscardRate();
  xr.burst_density = GetBurstDensity();
  xr.gap_density = GetGapDensity();
  xr.burst_duration = GetBurstDuration();
  xr.gap_duration = GetGapDuration();
  xr.round_trip_delay = GetRoundTripDelay();
  xr.end_system_delay = GetEndSystemDelay();
  xr.signal_level = 0x7F;
  xr.noise_level = 0x7F;
  xr.rerl = 0x7F;	
  xr.gmin = 16;
  xr.r_factor = RFactor();
  xr.ext_r_factor = 0x7F;
  xr.mos_lq = MOS_LQ();
  xr.mos_cq = MOS_CQ();
  xr.rx_config = 0x00;
  xr.reserved = 0x00;

  if (jitter != NULL) {
    xr.jb_nominal = (WORD)(jitter->GetMinJitterDelay()/jitter->GetTimeUnits());
    xr.jb_maximum = (WORD)(jitter->GetCurrentJitterDelay()/jitter->GetTimeUnits());
    xr.jb_absolute = (WORD)(jitter->GetMaxJitterDelay()/jitter->GetTimeUnits());
  }
  
  report.EndPacket();
  
  PTRACE(3, "RTP\tSession " << sessionID << ", SentExtendedReport:"
            " ssrc=" << xr.ssrc
         << " loss_rate=" << (PUInt32b) xr.loss_rate
         << " discard_rate=" << (PUInt32b) xr.discard_rate
         << " burst_density=" << (PUInt32b) xr.burst_density
         << " gap_density=" << (PUInt32b) xr.gap_density
         << " burst_duration=" << xr.burst_duration
         << " gap_duration=" << xr.gap_duration
         << " round_trip_delay="<< xr.round_trip_delay
         << " end_system_delay="<< xr.end_system_delay
         << " gmin="<< (PUInt32b) xr.gmin
         << " r_factor="<< (PUInt32b) xr.r_factor
         << " mos_lq="<< (PUInt32b) xr.mos_lq
         << " mos_cq="<< (PUInt32b) xr.mos_cq
         << " jb_nominal_delay="<< xr.jb_nominal
         << " jb_maximum_delay="<< xr.jb_maximum
         << " jb_absolute_delay="<< xr.jb_absolute);

}


OpalRTPSession::ExtendedReportArray
RTCP_XR_Metrics::BuildExtendedReportArray(const RTP_ControlFrame & frame, PINDEX offset)
{
  OpalRTPSession::ExtendedReportArray reports;

  const RTP_ControlFrame::ExtendedReport * rr = (const RTP_ControlFrame::ExtendedReport *)(frame.GetPayloadPtr()+offset);
  for (PINDEX repIdx = 0; repIdx < (PINDEX)frame.GetCount(); repIdx++) {
    OpalRTPSession::ExtendedReport * report = new OpalRTPSession::ExtendedReport;
    report->sourceIdentifier = rr->ssrc;
    report->lossRate = rr->loss_rate;
    report->discardRate = rr->discard_rate;
    report->burstDensity = rr->burst_density;
    report->gapDensity = rr->gap_density;
    report->roundTripDelay = rr->round_trip_delay;
    report->RFactor = rr->r_factor;
    report->mosLQ = rr->mos_lq;
    report->mosCQ = rr->mos_cq;
    report->jbNominal = rr->jb_nominal;
    report->jbMaximum = rr->jb_maximum;
    report->jbAbsolute = rr->jb_absolute;
    reports.SetAt(repIdx, report);
    rr++;
  }
  return reports;
}


void OpalRTPSession::OnRxExtendedReport(DWORD PTRACE_PARAM(src), const ExtendedReportArray & PTRACE_PARAM(reports))
{
#if PTRACING
  if (PTrace::CanTrace(3)) {
    ostream & strm = PTrace::Begin(3, __FILE__, __LINE__, this);
    strm << "RTP\tSession " << m_sessionId << ", OnExtendedReport: ssrc=" << src << '\n';
    for (PINDEX i = 0; i < reports.GetSize(); i++)
      strm << "  XR: " << reports[i] << '\n';
    strm << PTrace::End;
  }
#endif
}


void OpalRTPSession::ExtendedReport::PrintOn(ostream & strm) const
{
  strm << "ssrc=" << sourceIdentifier
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

#endif // OPAL_RTCP_XR
