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
#include <rtp/rtp.h>
#include <rtp/metrics.h>

#if OPAL_RTCP_XR

#include <math.h>


const unsigned SecondsFrom1900to1970 = (70*365+17)*24*60*60U;

class RTP_DataFrame;

RTCP_XR_Metrics::RTCP_XR_Metrics()
{		
  PTRACE(4, "RTP\tRTCP_XR_Metrics created.");
  
  gmin = 16;
  lost = 1;
  packetsReceived = 0;
  packetsSinceLastLoss = 0;
  packetsLost = 0;
  packetsDiscarded = 0;
  
  c5 = 0;
  c11 = 0;
  c13 = 0;
  c14 = 0;
  c22 = 0;
  c23 = 0;
  c31 = 0;
  c32 = 0;
  c33 = 0;
  
  Ie = 0;
  Bpl = 0;
  payloadBitrate = 0;
  payloadSize = 0;
  lookAheadTime = 0;
  
  
  isPayloadTypeOK = true;
  
  jitterDelay = 0;
  lastId = 0;
  lastIe = 0;
  
  PTime lsrTime(0);
  PTimeInterval dlsrTime(0);
  
  packetsReceivedInGap = 0;
  packetsLostInGap = 0;
   
  packetsReceivedInBurst = 0;
  packetsLostInBurst = 0;
}


RTCP_XR_Metrics::~RTCP_XR_Metrics()
{
  PTRACE_IF(3, packetsReceived != 0 || packetsLost != 0,
            "RTP\tRTCP_XR_Metrics final statistics:\n"
            "   R Factor = "<< (PUInt32b) EndOfCallRFactor() << "\n"
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


void RTCP_XR_Metrics::SetPayloadInfo(RTP_DataFrame frame)
{
  /* Get Ie and Bpl for the coded utilized, according to ITU-T G.113 */
  switch (frame.GetPayloadType()) {
    case RTP_DataFrame::PCMU:
    case RTP_DataFrame::PCMA:
      Ie = 0;
      Bpl = 25.1f;
      payloadBitrate = 64000;
    break;
    
    case RTP_DataFrame::G7231:
      Ie = 15;
      Bpl = 16.1f;
      payloadBitrate = 6300;
      lookAheadTime = 7.5f;
    break;
      
    case RTP_DataFrame::G729:
      Ie = 11;
      Bpl = 19;
      payloadBitrate = 8000;
      lookAheadTime = 5;
    break;
      
    case RTP_DataFrame::GSM:
      Ie = 20;
      Bpl = 10;
      payloadBitrate = 13000;
    break;
    
    default:
      PTRACE(2, "RTP\tVoIP Metrics, No Ie and Bpl data for payload type. "
                "Unable to calculate R Factor and MOS score.");
      isPayloadTypeOK = false;          
  }  
  payloadSize = frame.GetPayloadSize();
}


void RTCP_XR_Metrics::SetJitterDelay (DWORD delay)
{
  jitterDelay = delay;
  
  /* If the Id factor has changed, create a new Id period */
  if (abs(IdFactor() - lastId) < 1e-14) {
    PTime now;
    createIdPeriod(lastJitterBufferChangeTimestamp, now);
    lastJitterBufferChangeTimestamp = now;
    lastId = IdFactor();
  }
}


BYTE RTCP_XR_Metrics::GetLossRate()
{
  if ((packetsReceived + packetsLost + packetsDiscarded) == 0) {
    return 0;
  }
  return (BYTE)(256 * (float)packetsLost/(packetsReceived + packetsLost + packetsDiscarded));
}


BYTE RTCP_XR_Metrics::GetDiscardRate()
{
  if ((packetsReceived + packetsLost + packetsDiscarded) == 0) {
    return 0;
  }
  return (BYTE)(256 * (float)packetsDiscarded/(packetsReceived + packetsLost + packetsDiscarded));
}


BYTE RTCP_XR_Metrics::GetBurstDensity()
{
  if ((packetsReceivedInBurst + packetsLostInBurst) == 0) {
    return 0;
  }
  return (BYTE)(ceil(256 * ((float)packetsLostInBurst)/(packetsReceivedInBurst + packetsLostInBurst)));
}


BYTE RTCP_XR_Metrics::GetGapDensity()
{  
  if ((packetsReceivedInGap + packetsLostInGap) == 0) {
    return 0;
  }
  return (BYTE)(ceil(256 * ((float)packetsLostInGap)/(packetsReceivedInGap + packetsLostInGap + packetsSinceLastLoss)));
}


PUInt16b RTCP_XR_Metrics::GetBurstDuration()
{
  unsigned averageDuration = 0;
  PUInt64  totalDuration = 0;
  DWORD    count = 0;
  
  /* If we are within a burst account for it */
  if (currentPeriodType == BURST) {
    totalDuration = totalDuration + (lastLossTimestamp - periodBeginTimestamp).GetMilliSeconds();
    count ++;  
  }
  
  /* Iterate over the list of periods to calculate a mean duration */
  for (list<TimePeriod>::iterator period = timePeriods.begin(); period != timePeriods.end(); period++) {
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
  if (currentPeriodType == BURST) {
    PTime now;
    totalDuration = totalDuration + (now - lastLossTimestamp).GetMilliSeconds();
    count ++;  
  }
  else {
    /* If we are in a gap, just account for it */
    PTime now;
    totalDuration = totalDuration + (now - periodBeginTimestamp).GetMilliSeconds();
    count++;
  }
  
  /* Iterate over the list of periods to calculate a mean duration */
  for (list<TimePeriod>::iterator period = timePeriods.begin(); period != timePeriods.end(); period++) {
    if ((*period).type == GAP) {
      totalDuration = totalDuration + (*period).duration.GetMilliSeconds();
      count++;
    }      
  }
  
  return (WORD)(count > 0 ? (totalDuration/count) : averageDuration); 
}


PUInt16b RTCP_XR_Metrics::GetRoundTripDelay()
{
  if (srPacketsReceived == 0 || lsrTime.GetTimestamp() == 0 || dlsrTime.GetMilliSeconds() == 0)
    return 0;

  /* calculate the round trip delay, according to RFC 3611 */
  PTimeInterval roundTripDelay;
  roundTripDelay = arrivalTime - lsrTime;
  roundTripDelay = roundTripDelay - dlsrTime;
  
  return (WORD)roundTripDelay.GetMilliSeconds();
}


PUInt16b RTCP_XR_Metrics::GetEndSystemDelay()
{
  if (!isPayloadTypeOK || payloadBitrate == 0) {
    return 0;
  }
  /* Account 2 times for the codec time (sender and receiver sides) */
  return (WORD)(2*((payloadSize*8)/payloadBitrate)*1000 + lookAheadTime + jitterDelay);
}


BYTE RTCP_XR_Metrics::RFactor()
{
  /* The reported R factor is for conversational listening quality */
  return RFactor(CQ);
}


BYTE RTCP_XR_Metrics::RFactor(QualityType qt)
{
  if (!isPayloadTypeOK)
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
  if (!isPayloadTypeOK)
    return 127;

  /* Compute end of call R factor, according to the extended E-Model */
  double R = 93.4 - GetPonderateId() - GetEndOfCallIe();   
  return (BYTE)ceil(R);
}


BYTE RTCP_XR_Metrics::MOS_LQ()
{
  if (!isPayloadTypeOK)
    return 127;

  /* RTCP-XR requires MOS score in the range of 10 to 50 */
  return (BYTE)ceil(MOS(LQ)*10);
}


BYTE RTCP_XR_Metrics::MOS_CQ()
{
  if (!isPayloadTypeOK)
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
  sumId = IdFactor() * (now - lastJitterBufferChangeTimestamp).GetMilliSeconds();
  sumDuration = sumDuration + (now - lastJitterBufferChangeTimestamp).GetMilliSeconds();
  count++;
  
  /* Iterate over the list of periods to calculate an average Id */
  for (list<IdPeriod>::iterator period = idPeriods.begin(); period != idPeriods.end(); period++) {
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
  return (Ie + (95 - Ie) * Ppl/(Ppl + Bpl));
}


float RTCP_XR_Metrics::GetEndOfCallIe()
{
  /* Calculate the time since the last burst period */
  PTime now;
  PInt64 y = (now - lastLossInBurstTimestamp).GetMilliSeconds();
  
  /* Compute end of call Ie factor, according to the extended E-Model */
  return GetPonderateIe() + 0.7f * (lastIe - GetPonderateIe()) * exp((float)(-y/30000.0));
}


float RTCP_XR_Metrics::GetPonderateIe()
{
  float   ponderateIe = 0;
  float   sumIe = 0;
  PUInt64 sumDuration = 0;
  DWORD   count = 0;
  
  /* Account for the current time and Ie value */
  PTime now;
  ponderateIe = Ieff(currentPeriodType) * (now - periodBeginTimestamp).GetMilliSeconds();
  sumDuration = sumDuration + (now - periodBeginTimestamp).GetMilliSeconds();
  count++;
  
  /* Iterate over the list of periods to calculate an average Ie */
  for (list<IePeriod>::iterator period = iePeriods.begin(); period != iePeriods.end(); period++) {
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
        
  timePeriods.push_back(newPeriod);
  
  return newPeriod;
}


RTCP_XR_Metrics::IdPeriod RTCP_XR_Metrics::createIdPeriod(PTime beginTimestamp, PTime endTimestamp)
{
  IdPeriod newPeriod;
  
  /* Get the current Id value */  
  newPeriod.Id = IdFactor();
  newPeriod.duration = endTimestamp - beginTimestamp;
        
  idPeriods.push_back(newPeriod);
  
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
    if (!iePeriods.empty())
    {
      /* Get the last period in the list */
      list<IePeriod>::reverse_iterator period = iePeriods.rbegin();
      IePeriod & lastPeriod = (*period);
      
      /* If the last period was a gap, calculate an perceptual average Ie value */
      if (lastPeriod.type == GAP)
      {
        float T1 = 5000.0;
        float T2 = 15000.0;
      
        Ieg = lastPeriod.Ieff;
        I1 = lastIe;
        g = lastPeriod.duration.GetMilliSeconds();
        I2 = Ieg + (I1 - Ieg) * exp(-g/T2);
      
        Ieb = newPeriod.Ieff;
        b = newPeriod.duration.GetMilliSeconds();
        I1 = Ieb - (Ieb - I2) * exp(-b/T1);
      
        lastIe = I1;
      
        averageIe = (b * Ieb + g * Ieg - T1 * (Ieb - I2) * (1 - exp(-b/T1)) + T2 * (I1 - Ieg) * (1 -  exp(-g/T2))) / (b + g);
      
        lastPeriod.Ieff = averageIe;
        newPeriod.Ieff = averageIe;
      }
    }
  }
  /* Insert the new period on the list */
  iePeriods.push_back(newPeriod);
  
  return newPeriod;
}


void RTCP_XR_Metrics::OnPacketReceived()
{
  packetsReceived++;
  packetsSinceLastLoss++;
  markov(PACKET_RECEIVED);
}


void RTCP_XR_Metrics::OnPacketDiscarded()
{
  packetsDiscarded++;
  markov(PACKET_DISCARDED);
}


void RTCP_XR_Metrics::OnPacketLost()
{
  packetsLost++;
  markov(PACKET_LOST);
}


void RTCP_XR_Metrics::OnPacketLost(DWORD dropped)
{
  for (int i = 0; i < (signed int) dropped; i++) {
    markov(PACKET_LOST);
  }
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
  
  arrivalTime = PTime(arrival_ntp_sec - SecondsFrom1900to1970, arrival_ntp_frac/4294);
  
  if (lsr != 0) {
    /* Get the timestamp of the last SR */
    PUInt32b lsr_ntp_sec = ((lsr >> 16) & 0x0000FFFF);
    PUInt32b lsr_ntp_frac = ((lsr << 16) & 0xFFFF0000);
    lsrTime = PTime(lsr_ntp_sec - SecondsFrom1900to1970, lsr_ntp_frac/4294);
  }
  
  if (dlsr != 0) {
    /* Get the delay since last SR */
    dlsrTime.SetInterval (dlsr*1000/65536);
  }
  
  srPacketsReceived++; 
}


void RTCP_XR_Metrics::markov(RTCP_XR_Metrics::PacketEvent event)
{
  if (packetsReceived == 0) {
    PTime periodBeginTimestamp;
    currentPeriodType = GAP;
  }
  
  if (event == PACKET_LOST || event == PACKET_DISCARDED) { 
    
    c5 += packetsSinceLastLoss;
    
    if (packetsSinceLastLoss >= gmin || packetsReceived <= gmin) {
      
      if (currentPeriodType == BURST) {
        /* The burst period ended with the last packet loss */ 
        createIePeriod(createTimePeriod(currentPeriodType, periodBeginTimestamp, lastLossTimestamp));
        /* mark as a gap */
        currentPeriodType = GAP;
        periodBeginTimestamp = lastLossTimestamp;
        ResetCounters();
      }
      
      if (lost == 1) {
        c14++;
        packetsLostInGap++;
      }
      else {	
        c13++;
        packetsLostInBurst++;          
      } 
      lost = 1;
      c11 += packetsSinceLastLoss;
      packetsReceivedInGap += packetsSinceLastLoss;
    }
    else if (event == PACKET_RECEIVED) {
      if (currentPeriodType == GAP) {
        /* The burst period ended with the packet received before the last loss */
        createIePeriod(createTimePeriod(currentPeriodType, periodBeginTimestamp, lastLossTimestamp));     
         /* mark as a burst */
        currentPeriodType = BURST;
        periodBeginTimestamp = lastLossTimestamp;
        ResetCounters();
      }
      
      PTime now;
      lastLossInBurstTimestamp = now;  
      lost ++;
      
      if (lost > 8) {
        c5 = 0;
      }
      if (packetsSinceLastLoss == 0) {
        c33++;        
      }
      else {
        c23++;
        c22 += (packetsSinceLastLoss - 1);
        packetsReceivedInBurst += packetsSinceLastLoss;        
      }
      packetsLostInBurst++;
    }
    else {
      PTRACE(2, "RTP\tVoIP Metrics, Unknown packet event: " << event);
    }   
    packetsSinceLastLoss = 0;
    PTime now;
    lastLossTimestamp = now;
  }
  
  /* calculate additional transaction counts */
  c31 = c13;
  c32 = c23;
}


#endif // OPAL_RTCP_XR
