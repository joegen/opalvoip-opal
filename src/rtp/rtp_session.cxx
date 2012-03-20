/*
 * rtp_session.cxx
 *
 * RTP protocol session
 *
 * Copyright (c) 2012 Equivalence Pty. Ltd.
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

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "rtp_session.h"
#endif

#include <opal/buildopts.h>

#include <rtp/rtp_session.h>

#include <opal/endpoint.h>
#include <opal/rtpep.h>
#include <opal/rtpconn.h>
#include <rtp/jitter.h>
#include <rtp/metrics.h>

#include <ptclib/random.h>
#include <ptclib/pnat.h>

#include <algorithm>

#include <h323/h323con.h>

#define BAD_TRANSMIT_PKT_MAX 5      // Number of consecutive bad tx packet in below number of seconds
#define BAD_TRANSMIT_TIME_MAX 10    //  maximum of seconds of transmit fails before session is killed

const unsigned SecondsFrom1900to1970 = (70*365+17)*24*60*60U;

#define RTP_VIDEO_RX_BUFFER_SIZE 0x100000 // 1Mb
#define RTP_AUDIO_RX_BUFFER_SIZE 0x4000   // 16kb
#define RTP_DATA_TX_BUFFER_SIZE  0x2000   // 8kb
#define RTP_CTRL_BUFFER_SIZE     0x1000   // 4kb


/////////////////////////////////////////////////////////////////////////////
/**A descendant of the OpalJitterBuffer that reads RTP_DataFrame instances
   from the OpalRTPSession
  */
class RTP_JitterBuffer : public OpalJitterBufferThread
{
   PCLASSINFO(RTP_JitterBuffer, OpalJitterBufferThread);
public:
   RTP_JitterBuffer(
     OpalRTPSession & session, ///<  Associated RTP session for data to be read from
     unsigned minJitterDelay,  ///<  Minimum delay in RTP timestamp units
     unsigned maxJitterDelay,  ///<  Maximum delay in RTP timestamp units
     unsigned timeUnits = 8,   ///<  Time units, usually 8 or 16
     PINDEX packetSize = 2048  ///<  Max RTP packet size
   ) : OpalJitterBufferThread(minJitterDelay, maxJitterDelay, timeUnits, packetSize)
     , m_session(session)
   {
      PTRACE_CONTEXT_ID_FROM(session);
      StartThread();
   }

   ~RTP_JitterBuffer()
   {
     PTRACE(4, "Jitter\tDestroying jitter buffer " << *this);

     m_running = false;
     bool reopen = m_session.Shutdown(true);

     WaitForThreadTermination();

     if (reopen)
       m_session.Restart(true);
   }

    virtual PBoolean OnReadPacket(RTP_DataFrame & frame)
    {
      bool success = m_session.InternalReadData(frame);
      PTRACE_IF(6, success, "Jitter\tOnReadPacket: Frame from network, timestamp " << frame.GetTimestamp());
      return success;
   }

 protected:
   /**This class extracts data from the outside world by reading from this session variable */
   OpalRTPSession & m_session;
};


#define new PNEW

/////////////////////////////////////////////////////////////////////////////

const PCaselessString & OpalRTPSession::RTP_AVP() { static const PConstCaselessString s("RTP/AVP"); return s; }

PFACTORY_CREATE(OpalMediaSessionFactory, OpalRTPSession, OpalRTPSession::RTP_AVP());

#if P_CONFIG_FILE
static PTimeInterval GetDefaultOutOfOrderWaitTime()
{
  static PTimeInterval ooowt(PConfig(PConfig::Environment).GetInteger("OPAL_RTP_OUT_OF_ORDER_TIME", 100));
  return ooowt;
}
#else
#define GetDefaultOutOfOrderWaitTime() (100)
#endif

OpalRTPSession::OpalRTPSession(const Init & init)
  : OpalMediaSession(init)
  , isAudio(init.m_mediaType == OpalMediaType::Audio())
  , m_timeUnits(isAudio ? 8 : 90)
  , canonicalName(PProcess::Current().GetUserName())
  , toolName(PProcess::Current().GetName())
  , reportTimeInterval(0, 12)  // Seconds
  , lastSRTimestamp(0)
  , lastSRReceiveTime(0)
  , outOfOrderWaitTime(GetDefaultOutOfOrderWaitTime())
  , firstPacketSent(0)
  , firstPacketReceived(0)
  , remoteAddress(0)
  , remoteTransmitAddress(0)
  , remoteIsNAT(false)
{
  ignorePayloadTypeChanges = true;
  syncSourceOut = PRandom::Number();

  timeStampOffs = 0;
  oobTimeStampBaseEstablished = false;
  lastSentPacketTime = PTimer::Tick();

  syncSourceIn = 0;
  allowAnySyncSource = true;
  allowOneSyncSourceChange = false;
  allowRemoteTransmitAddressChange = false;
  allowSequenceChange = false;
  txStatisticsInterval = 100;  // Number of data packets between tx reports
  rxStatisticsInterval = 100;  // Number of data packets between rx reports
  lastSentSequenceNumber = (WORD)PRandom::Number();
  expectedSequenceNumber = 0;
  lastRRSequenceNumber = 0;
  resequenceOutOfOrderPackets = true;
  senderReportsReceived = 0;
  consecutiveOutOfOrderPackets = 0;

  ClearStatistics();

  lastReceivedPayloadType = RTP_DataFrame::IllegalPayloadType;

  closeOnBye = false;
  byeSent    = false;

  lastSentTimestamp = 0;  // should be calculated, but we'll settle for initialising it

  remoteDataPort    = 0;
  remoteControlPort = 0;
  shutdownRead      = false;
  shutdownWrite     = false;
  dataSocket        = NULL;
  controlSocket     = NULL;
  appliedQOS        = false;
  localHasNAT       = false;
  badTransmitCounter = 0;
}


OpalRTPSession::OpalRTPSession(const OpalRTPSession & other)
  : OpalMediaSession(Init(other.m_connection, other.m_sessionId, other.m_mediaType))
{
}


OpalRTPSession::~OpalRTPSession()
{
  Close();

#if PTRACING
  PTime now;
  int sentDuration = (now-firstPacketSent).GetSeconds();
  if (sentDuration == 0)
    sentDuration = 1;
  int receiveDuration = (now-firstPacketReceived).GetSeconds();
  if (receiveDuration == 0)
    receiveDuration = 1;
 #endif
  PTRACE_IF(3, packetsSent != 0 || packetsReceived != 0,
      "RTP\tSession " << m_sessionId << ", final statistics:\n"
      "    firstPacketSent    = " << firstPacketSent << "\n"
      "    packetsSent        = " << packetsSent << "\n"
      "    octetsSent         = " << octetsSent << "\n"
      "    bitRateSent        = " << (8*octetsSent/sentDuration) << "\n"
      "    averageSendTime    = " << averageSendTime << "\n"
      "    maximumSendTime    = " << maximumSendTime << "\n"
      "    minimumSendTime    = " << minimumSendTime << "\n"
      "    packetsLostByRemote= " << packetsLostByRemote << "\n"
      "    jitterLevelOnRemote= " << jitterLevelOnRemote << "\n"
      "    firstPacketReceived= " << firstPacketReceived << "\n"
      "    packetsReceived    = " << packetsReceived << "\n"
      "    octetsReceived     = " << octetsReceived << "\n"
      "    bitRateReceived    = " << (8*octetsReceived/receiveDuration) << "\n"
      "    packetsLost        = " << packetsLost << "\n"
      "    packetsTooLate     = " << GetPacketsTooLate() << "\n"
      "    packetOverruns     = " << GetPacketOverruns() << "\n"
      "    packetsOutOfOrder  = " << packetsOutOfOrder << "\n"
      "    averageReceiveTime = " << averageReceiveTime << "\n"
      "    maximumReceiveTime = " << maximumReceiveTime << "\n"
      "    minimumReceiveTime = " << minimumReceiveTime << "\n"
      "    averageJitter      = " << GetAvgJitterTime() << "\n"
      "    maximumJitter      = " << GetMaxJitterTime()
   );
}


void OpalRTPSession::ClearStatistics()
{
  firstPacketSent.SetTimestamp(0);
  packetsSent = 0;
  rtcpPacketsSent = 0;
  octetsSent = 0;
  firstPacketReceived.SetTimestamp(0);
  packetsReceived = 0;
  octetsReceived = 0;
  packetsLost = 0;
  packetsLostByRemote = 0;
  packetsOutOfOrder = 0;
  averageSendTime = 0;
  maximumSendTime = 0;
  minimumSendTime = 0;
  averageReceiveTime = 0;
  maximumReceiveTime = 0;
  minimumReceiveTime = 0;
  jitterLevel = 0;
  maximumJitterLevel = 0;
  jitterLevelOnRemote = 0;
  markerRecvCount = 0;
  markerSendCount = 0;

  txStatisticsCount = 0;
  rxStatisticsCount = 0;
  averageSendTimeAccum = 0;
  maximumSendTimeAccum = 0;
  minimumSendTimeAccum = 0xffffffff;
  averageReceiveTimeAccum = 0;
  maximumReceiveTimeAccum = 0;
  minimumReceiveTimeAccum = 0xffffffff;
  packetsLostSinceLastRR = 0;
  lastTransitTime = 0;
}


void OpalRTPSession::AttachTransport(Transport & transport)
{
  transport.DisallowDeleteObjects();

  PObject * channel = transport.RemoveHead();
  dataSocket = dynamic_cast<PUDPSocket *>(channel);
  if (dataSocket == NULL)
    delete channel;

  channel = transport.RemoveHead();
  controlSocket = dynamic_cast<PUDPSocket *>(channel);
  if (controlSocket == NULL)
    delete channel;

  transport.AllowDeleteObjects();
}


OpalMediaSession::Transport OpalRTPSession::DetachTransport()
{
  Transport temp;

  if (dataSocket != NULL) {
    temp.Append(dataSocket);
    dataSocket = NULL;
  }

  if (controlSocket != NULL) {
    temp.Append(controlSocket);
    controlSocket = NULL;
  }

  return temp;
}


void OpalRTPSession::SendBYE()
{
  {
    PWaitAndSignal mutex(dataMutex);
    if (byeSent)
      return;

    byeSent = true;
  }

  RTP_ControlFrame report;
  InsertReportPacket(report);

  static char const ReasonStr[] = "Session ended";
  static size_t ReasonLen = sizeof(ReasonStr);

  // insert BYE
  report.StartNewPacket();
  report.SetPayloadType(RTP_ControlFrame::e_Goodbye);
  report.SetPayloadSize(4+1+ReasonLen);  // length is SSRC + ReasonLen + reason

  BYTE * payload = report.GetPayloadPtr();

  // one SSRC
  report.SetCount(1);
  *(PUInt32b *)payload = syncSourceOut;

  // insert reason
  payload[4] = (BYTE)ReasonLen;
  memcpy((char *)(payload+5), ReasonStr, ReasonLen);

  report.EndPacket();
  WriteControl(report);
}

PString OpalRTPSession::GetCanonicalName() const
{
  PWaitAndSignal mutex(m_reportMutex);
  PString s = canonicalName;
  s.MakeUnique();
  return s;
}


void OpalRTPSession::SetCanonicalName(const PString & name)
{
  PWaitAndSignal mutex(m_reportMutex);
  canonicalName = name;
}


PString OpalRTPSession::GetToolName() const
{
  PWaitAndSignal mutex(m_reportMutex);
  PString s = toolName;
  s.MakeUnique();
  return s;
}


void OpalRTPSession::SetToolName(const PString & name)
{
  PWaitAndSignal mutex(m_reportMutex);
  toolName = name;
}


RTPExtensionHeaders OpalRTPSession::GetExtensionHeaders() const
{
  PWaitAndSignal mutex(m_reportMutex);
  return m_extensionHeaders;
}


void OpalRTPSession::SetExtensionHeader(const RTPExtensionHeaders & ext)
{
  PWaitAndSignal mutex(m_reportMutex);
  m_extensionHeaders = ext;
}


void OpalRTPSession::SetJitterBufferSize(unsigned minJitterDelay,
                                      unsigned maxJitterDelay,
                                      unsigned timeUnits,
                                        PINDEX packetSize)
{
  if (timeUnits > 0)
    m_timeUnits = timeUnits;

  if (minJitterDelay == 0 && maxJitterDelay == 0) {
    PTRACE_IF(4, m_jitterBuffer != NULL, "RTP\tSwitching off jitter buffer " << *m_jitterBuffer);
    m_jitterBuffer.SetNULL();
  }
  else {
    resequenceOutOfOrderPackets = false;
    FlushData();
    if (m_jitterBuffer != NULL) {
      PTRACE(4, "RTP\tSetting jitter buffer time from " << minJitterDelay << " to " << maxJitterDelay);
      m_jitterBuffer->SetDelay(minJitterDelay, maxJitterDelay, packetSize);
    }
    else {
      m_jitterBuffer = new RTP_JitterBuffer(*this, minJitterDelay, maxJitterDelay, m_timeUnits, packetSize);
      PTRACE(4, "RTP\tCreated RTP jitter buffer " << *m_jitterBuffer);
    }
  }
}


unsigned OpalRTPSession::GetJitterBufferSize() const
{
  JitterBufferPtr jitter = m_jitterBuffer; // Increase reference count
  return jitter != NULL ? jitter->GetCurrentJitterDelay() : 0;
}


bool OpalRTPSession::ReadData(RTP_DataFrame & frame)
{
  JitterBufferPtr jitter = m_jitterBuffer; // Increase reference count
  if (jitter != NULL)
    return jitter->ReadData(frame);

  if (m_outOfOrderPackets.empty())
    return InternalReadData(frame);

  unsigned sequenceNumber = m_outOfOrderPackets.back().GetSequenceNumber();
  if (sequenceNumber != expectedSequenceNumber) {
    PTRACE(5, "RTP\tSession " << m_sessionId << ", ssrc=" << syncSourceIn
           << ", still out of order packets, next "
           << sequenceNumber << " expected " << expectedSequenceNumber);
    return InternalReadData(frame);
  }

  frame = m_outOfOrderPackets.back();
  m_outOfOrderPackets.pop_back();
  expectedSequenceNumber = (WORD)(sequenceNumber + 1);

  PTRACE(5, "RTP\tSession " << m_sessionId << ", ssrc=" << syncSourceIn << ", resequenced "
         << (m_outOfOrderPackets.empty() ? "last" : "next") << " out of order packet " << sequenceNumber);
  return true;
}


void OpalRTPSession::FlushData()
{
  if (dataSocket == NULL)
    return;

  PTimeInterval oldTimeout = dataSocket->GetReadTimeout();  
  dataSocket->SetReadTimeout(0);

  PINDEX count = 0;
  BYTE buffer[2000];
  while (dataSocket->Read(buffer, sizeof(buffer)))
    ++count;

  dataSocket->SetReadTimeout(oldTimeout);

  PTRACE_IF(3, count > 0, "RTP\tSession " << m_sessionId << ", flushed "
            << count << " RTP data packets before activating jitter buffer");
}


void OpalRTPSession::SetTxStatisticsInterval(unsigned packets)
{
  txStatisticsInterval = PMAX(packets, 2);
  txStatisticsCount = 0;
  averageSendTimeAccum = 0;
  maximumSendTimeAccum = 0;
  minimumSendTimeAccum = 0xffffffff;
}


void OpalRTPSession::SetRxStatisticsInterval(unsigned packets)
{
  rxStatisticsInterval = PMAX(packets, 2);
  rxStatisticsCount = 0;
  averageReceiveTimeAccum = 0;
  maximumReceiveTimeAccum = 0;
  minimumReceiveTimeAccum = 0xffffffff;
}


void OpalRTPSession::AddReceiverReport(RTP_ControlFrame::ReceiverReport & receiver)
{
  receiver.ssrc = syncSourceIn;
  receiver.SetLostPackets(GetPacketsLost()+GetPacketsTooLate());

  if (expectedSequenceNumber > lastRRSequenceNumber)
    receiver.fraction = (BYTE)((packetsLostSinceLastRR<<8)/(expectedSequenceNumber - lastRRSequenceNumber));
  else
    receiver.fraction = 0;
  packetsLostSinceLastRR = 0;

  receiver.last_seq = lastRRSequenceNumber;
  lastRRSequenceNumber = expectedSequenceNumber;

  receiver.jitter = jitterLevel >> JitterRoundingGuardBits; // Allow for rounding protection bits
  
  if (senderReportsReceived > 0) {
    // Calculate the last SR timestamp
    PUInt32b lsr_ntp_sec  = (DWORD)(lastSRTimestamp.GetTimeInSeconds()+SecondsFrom1900to1970); // Convert from 1970 to 1900
    PUInt32b lsr_ntp_frac = lastSRTimestamp.GetMicrosecond()*4294; // Scale microseconds to "fraction" from 0 to 2^32
    receiver.lsr = (((lsr_ntp_sec << 16) & 0xFFFF0000) | ((lsr_ntp_frac >> 16) & 0x0000FFFF));
  
    // Calculate the delay since last SR
    PTime now;
    delaySinceLastSR = now - lastSRReceiveTime;
    receiver.dlsr = (DWORD)(delaySinceLastSR.GetMilliSeconds()*65536/1000);
  }
  else {
    receiver.lsr = 0;
    receiver.dlsr = 0;
  }
  
  PTRACE(3, "RTP\tSession " << m_sessionId << ", SentReceiverReport:"
            " ssrc=" << receiver.ssrc
         << " fraction=" << (unsigned)receiver.fraction
         << " lost=" << receiver.GetLostPackets()
         << " last_seq=" << receiver.last_seq
         << " jitter=" << receiver.jitter
         << " lsr=" << receiver.lsr
         << " dlsr=" << receiver.dlsr);
} 


#if OPAL_RTCP_XR

void OpalRTPSession::InsertExtendedReportPacket(RTP_ControlFrame & report)
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
  
  xr.loss_rate = m_metrics.GetLossRate();
  xr.discard_rate = m_metrics.GetDiscardRate();
  xr.burst_density = m_metrics.GetBurstDensity();
  xr.gap_density = m_metrics.GetGapDensity();
  xr.burst_duration = m_metrics.GetBurstDuration();
  xr.gap_duration = m_metrics.GetGapDuration();
  xr.round_trip_delay = m_metrics.GetRoundTripDelay();
  xr.end_system_delay = m_metrics.GetEndSystemDelay();
  xr.signal_level = 0x7F;
  xr.noise_level = 0x7F;
  xr.rerl = 0x7F;    
  xr.gmin = 16;
  xr.r_factor = m_metrics.RFactor();
  xr.ext_r_factor = 0x7F;
  xr.mos_lq = m_metrics.MOS_LQ();
  xr.mos_cq = m_metrics.MOS_CQ();
  xr.rx_config = 0x00;
  xr.reserved = 0x00;

  JitterBufferPtr jitter = m_jitterBuffer; // Increase reference count in case gets deleted out from under us
  if (jitter != NULL) {
    xr.jb_nominal = (WORD)(jitter->GetMinJitterDelay()/jitter->GetTimeUnits());
    xr.jb_maximum = (WORD)(jitter->GetCurrentJitterDelay()/jitter->GetTimeUnits());
    xr.jb_absolute = (WORD)(jitter->GetMaxJitterDelay()/jitter->GetTimeUnits());
  }
  
  report.EndPacket();
  
  PTRACE(3, "RTP\tSession " << m_sessionId << ", SentExtendedReport:"
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

static OpalRTPSession::ExtendedReportArray
BuildExtendedReportArray(const RTP_ControlFrame & frame, PINDEX offset)
{
  OpalRTPSession::ExtendedReportArray reports(frame.GetCount());

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


void OpalRTPSession::OnRxSenderReportToMetrics(const RTP_ControlFrame & frame, PINDEX offset)
{
  const RTP_ControlFrame::ReceiverReport * rr = (const RTP_ControlFrame::ReceiverReport *)(frame.GetPayloadPtr()+offset);
  for (unsigned repIdx = 0; repIdx < frame.GetCount(); repIdx++, rr++)
    m_metrics.OnRxSenderReport(rr->lsr, rr->dlsr);
}


void OpalRTPSession::OnRxExtendedReport(DWORD PTRACE_PARAM(src), const ExtendedReportArray & PTRACE_PARAM(reports))
{
#if PTRACING
  if (PTrace::CanTrace(3)) {
    ostream & strm = PTrace::Begin(2, __FILE__, __LINE__, this);
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

#endif


OpalRTPSession::SendReceiveStatus OpalRTPSession::OnSendData(RTP_DataFrame & frame)
{
  PWaitAndSignal mutex(dataMutex);

  PTimeInterval tick = PTimer::Tick();  // Timestamp set now

  frame.SetSequenceNumber(++lastSentSequenceNumber);
  frame.SetSyncSource(syncSourceOut);

  // special handling for first packet
  if (packetsSent == 0) {

    // establish timestamp offset
    if (oobTimeStampBaseEstablished)  {
      timeStampOffs = oobTimeStampOutBase - frame.GetTimestamp() + ((PTimer::Tick() - oobTimeStampBase).GetInterval() * 8);
      frame.SetTimestamp(frame.GetTimestamp() + timeStampOffs);
    }
    else {
      oobTimeStampBaseEstablished = true;
      timeStampOffs               = 0;
      oobTimeStampOutBase         = frame.GetTimestamp();
      oobTimeStampBase            = PTimer::Tick();
    }

    firstPacketSent.SetCurrentTime();

    // display stuff
    PTRACE(3, "RTP\tSession " << m_sessionId << ", first sent data:"
              " ver=" << frame.GetVersion()
           << " pt=" << frame.GetPayloadType()
           << " psz=" << frame.GetPayloadSize()
           << " m=" << frame.GetMarker()
           << " x=" << frame.GetExtension()
           << " seq=" << frame.GetSequenceNumber()
           << " ts=" << frame.GetTimestamp()
           << " src=" << frame.GetSyncSource()
           << " ccnt=" << frame.GetContribSrcCount()
           << ' ' << GetRemoteMediaAddress());
  }

  else {
    // set timestamp
    DWORD ts = frame.GetTimestamp() + timeStampOffs;
    frame.SetTimestamp(ts);

    // reset OOB timestamp every marker bit
    if (frame.GetMarker()) {
      oobTimeStampOutBase = ts;
      oobTimeStampBase    = PTimer::Tick();
    }

    // Only do statistics on subsequent packets
    if ( ! (isAudio && frame.GetMarker()) ) {
      DWORD diff = (tick - lastSentPacketTime).GetInterval();

      averageSendTimeAccum += diff;
      if (diff > maximumSendTimeAccum)
        maximumSendTimeAccum = diff;
      if (diff < minimumSendTimeAccum)
        minimumSendTimeAccum = diff;
      txStatisticsCount++;
    }
  }

  lastSentPacketTime = tick;

  octetsSent += frame.GetPayloadSize();
  packetsSent++;

  if (frame.GetMarker())
    markerSendCount++;

  if (!SendReport())
    return e_AbortTransport;

  if (txStatisticsCount < txStatisticsInterval)
    return e_ProcessPacket;

  txStatisticsCount = 0;

  averageSendTime = averageSendTimeAccum/txStatisticsInterval;
  maximumSendTime = maximumSendTimeAccum;
  minimumSendTime = minimumSendTimeAccum;

  averageSendTimeAccum = 0;
  maximumSendTimeAccum = 0;
  minimumSendTimeAccum = 0xffffffff;

  PTRACE(3, "RTP\tSession " << m_sessionId << ", transmit statistics: "
   " packets=" << packetsSent <<
   " octets=" << octetsSent <<
   " avgTime=" << averageSendTime <<
   " maxTime=" << maximumSendTime <<
   " minTime=" << minimumSendTime
  );

  return e_ProcessPacket;
}

OpalRTPSession::SendReceiveStatus OpalRTPSession::OnSendControl(RTP_ControlFrame & frame)
{
  frame.SetSize(frame.GetCompoundSize());
  return e_ProcessPacket;
}


OpalRTPSession::SendReceiveStatus OpalRTPSession::OnReceiveData(RTP_DataFrame & frame)
{
  // Check that the PDU is the right version
  if (frame.GetVersion() != RTP_DataFrame::ProtocolVersion)
    return e_IgnorePacket; // Non fatal error, just ignore

  // Check if expected payload type
  if (lastReceivedPayloadType == RTP_DataFrame::IllegalPayloadType)
    lastReceivedPayloadType = frame.GetPayloadType();

  if (lastReceivedPayloadType != frame.GetPayloadType() && !ignorePayloadTypeChanges) {

    PTRACE(4, "RTP\tSession " << m_sessionId << ", got payload type "
           << frame.GetPayloadType() << ", but was expecting " << lastReceivedPayloadType);
    return e_IgnorePacket;
  }

  // Check for if a control packet rather than data packet.
  if (frame.GetPayloadType() > RTP_DataFrame::MaxPayloadType)
    return e_IgnorePacket; // Non fatal error, just ignore

  PTimeInterval tick = PTimer::Tick();  // Get timestamp now

  // Have not got SSRC yet, so grab it now
  if (syncSourceIn == 0)
    syncSourceIn = frame.GetSyncSource();

  // Check packet sequence numbers
  if (packetsReceived == 0) {
    firstPacketReceived.SetCurrentTime();
    PTRACE(3, "RTP\tSession " << m_sessionId << ", first receive data:"
              " ver=" << frame.GetVersion()
           << " pt=" << frame.GetPayloadType()
           << " psz=" << frame.GetPayloadSize()
           << " m=" << frame.GetMarker()
           << " x=" << frame.GetExtension()
           << " seq=" << frame.GetSequenceNumber()
           << " ts=" << frame.GetTimestamp()
           << " src=" << hex << frame.GetSyncSource()
           << " ccnt=" << frame.GetContribSrcCount() << dec);

#if OPAL_RTCP_XR
    m_metrics.SetPayloadInfo(frame);
#endif

    if ((frame.GetPayloadType() == RTP_DataFrame::T38) &&
        (frame.GetSequenceNumber() >= 0x8000) &&
         (frame.GetPayloadSize() == 0)) {
      PTRACE(4, "RTP\tSession " << m_sessionId << ", ignoring left over audio packet from switch to T.38");
      return e_IgnorePacket; // Non fatal error, just ignore
    }

    expectedSequenceNumber = (WORD)(frame.GetSequenceNumber() + 1);
  }
  else {
    if (frame.GetSyncSource() != syncSourceIn) {
      if (allowAnySyncSource) {
        PTRACE(2, "RTP\tSession " << m_sessionId << ", SSRC changed from " << hex << frame.GetSyncSource() << " to " << syncSourceIn << dec);
        syncSourceIn = frame.GetSyncSource();
        allowSequenceChange = true;
      } 
      else if (allowOneSyncSourceChange) {
        PTRACE(2, "RTP\tSession " << m_sessionId << ", allowed one SSRC change from SSRC=" << hex << syncSourceIn << " to =" << dec << frame.GetSyncSource() << dec);
        syncSourceIn = frame.GetSyncSource();
        allowSequenceChange = true;
        allowOneSyncSourceChange = false;
      }
      else {
        PTRACE(2, "RTP\tSession " << m_sessionId << ", packet from SSRC=" << hex << frame.GetSyncSource() << " ignored, expecting SSRC=" << syncSourceIn << dec);
        return e_IgnorePacket; // Non fatal error, just ignore
      }

      JitterBufferPtr jitter = m_jitterBuffer; // Increase reference count
      if (jitter != NULL) {
        jitter->Reset();
        PTRACE(4, "RTP\tSession " << m_sessionId << ", jitter buffer reset due to SSRC change.");
      }
    }

    WORD sequenceNumber = frame.GetSequenceNumber();
    if (sequenceNumber == expectedSequenceNumber) {
      expectedSequenceNumber++;
      consecutiveOutOfOrderPackets = 0;

      if (!m_outOfOrderPackets.empty()) {
        PTRACE(5, "RTP\tSession " << m_sessionId << ", ssrc=" << syncSourceIn
               << ", received out of order packet " << sequenceNumber);
        outOfOrderPacketTime = tick;
        packetsOutOfOrder++;
      }

      // Only do statistics on packets after first received in talk burst
      if ( ! (isAudio && frame.GetMarker()) ) {
        DWORD diff = (tick - lastReceivedPacketTime).GetInterval();

        averageReceiveTimeAccum += diff;
        if (diff > maximumReceiveTimeAccum)
          maximumReceiveTimeAccum = diff;
        if (diff < minimumReceiveTimeAccum)
          minimumReceiveTimeAccum = diff;
        rxStatisticsCount++;

        // As per RFC3550 Appendix 8
        diff *= GetJitterTimeUnits(); // Convert to timestamp units
        long variance = diff - lastTransitTime;
        lastTransitTime = diff;
        if (variance < 0)
          variance = -variance;
        jitterLevel += variance - ((jitterLevel+(1<<(JitterRoundingGuardBits-1))) >> JitterRoundingGuardBits);
        if (jitterLevel > maximumJitterLevel)
          maximumJitterLevel = jitterLevel;
#if OPAL_RTCP_XR
        JitterBufferPtr jitter = m_jitterBuffer; // Increase reference count in case gets deleted out from under us
        if (jitter != NULL)
          m_metrics.SetJitterDelay(jitter->GetCurrentJitterDelay()/jitter->GetTimeUnits());
#endif
      }

      if (frame.GetMarker())
        markerRecvCount++;
    }
    else if (allowSequenceChange) {
      expectedSequenceNumber = (WORD) (sequenceNumber + 1);
      allowSequenceChange = false;
      m_outOfOrderPackets.clear();
      PTRACE(2, "RTP\tSession " << m_sessionId << ", ssrc=" << syncSourceIn
             << ", adjusting sequence numbers to expect " << expectedSequenceNumber);
    }
    else if (sequenceNumber < expectedSequenceNumber) {
#if OPAL_RTCP_XR
      m_metrics.OnPacketDiscarded();
#endif

      // Check for Cisco bug where sequence numbers suddenly start incrementing
      // from a different base.
      if (++consecutiveOutOfOrderPackets > 10) {
        expectedSequenceNumber = (WORD)(sequenceNumber + 1);
        PTRACE(2, "RTP\tSession " << m_sessionId << ", ssrc=" << syncSourceIn
               << ", abnormal change of sequence numbers, adjusting to expect " << expectedSequenceNumber);
      }
      else {
        PTRACE(2, "RTP\tSession " << m_sessionId << ", ssrc=" << syncSourceIn
               << ", incorrect sequence, got " << sequenceNumber << " expected " << expectedSequenceNumber);

        if (resequenceOutOfOrderPackets)
          return e_IgnorePacket; // Non fatal error, just ignore

        packetsOutOfOrder++;
      }
    }
    else if (resequenceOutOfOrderPackets &&
                (m_outOfOrderPackets.empty() || (tick - outOfOrderPacketTime) < outOfOrderWaitTime)) {
      if (m_outOfOrderPackets.empty())
        outOfOrderPacketTime = tick;
      // Maybe packet lost, maybe out of order, save for now
      SaveOutOfOrderPacket(frame);
      return e_IgnorePacket;
    }
    else {
      if (!m_outOfOrderPackets.empty()) {
        // Give up on the packet, probably never coming in. Save current and switch in
        // the lowest numbered packet.
        SaveOutOfOrderPacket(frame);

        frame = m_outOfOrderPackets.back();
        m_outOfOrderPackets.pop_back();
        sequenceNumber = frame.GetSequenceNumber();
        outOfOrderPacketTime = tick;
      }

      unsigned dropped = sequenceNumber - expectedSequenceNumber;
      frame.SetDiscontinuity(dropped);
      packetsLost += dropped;
      packetsLostSinceLastRR += dropped;
      PTRACE(2, "RTP\tSession " << m_sessionId << ", ssrc=" << syncSourceIn
             << ", " << dropped << " packet(s) missing at " << expectedSequenceNumber);
      expectedSequenceNumber = (WORD)(sequenceNumber + 1);
      consecutiveOutOfOrderPackets = 0;
#if OPAL_RTCP_XR
      m_metrics.OnPacketLost(dropped);
#endif
    }
  }

  lastReceivedPacketTime = tick;

  octetsReceived += frame.GetPayloadSize();
  packetsReceived++;

  if (m_syncRealTime.IsValid())
    frame.SetAbsoluteTime(m_syncRealTime + PTimeInterval((frame.GetTimestamp() - m_syncTimestamp)/m_timeUnits));

#if OPAL_RTCP_XR
  m_metrics.OnPacketReceived();
#endif

  if (!SendReport())
    return e_AbortTransport;

  if (rxStatisticsCount >= rxStatisticsInterval) {

    rxStatisticsCount = 0;

    averageReceiveTime = averageReceiveTimeAccum/rxStatisticsInterval;
    maximumReceiveTime = maximumReceiveTimeAccum;
    minimumReceiveTime = minimumReceiveTimeAccum;

    averageReceiveTimeAccum = 0;
    maximumReceiveTimeAccum = 0;
    minimumReceiveTimeAccum = 0xffffffff;

    PTRACE(4, "RTP\tSession " << m_sessionId << ", receive statistics:"
              " packets=" << packetsReceived <<
              " octets=" << octetsReceived <<
              " lost=" << packetsLost <<
              " tooLate=" << GetPacketsTooLate() <<
              " order=" << packetsOutOfOrder <<
              " avgTime=" << averageReceiveTime <<
              " maxTime=" << maximumReceiveTime <<
              " minTime=" << minimumReceiveTime <<
              " jitter=" << GetAvgJitterTime() <<
              " maxJitter=" << GetMaxJitterTime());
  }

  SendReceiveStatus status = e_ProcessPacket;
  for (list<FilterNotifier>::iterator filter = m_filters.begin(); filter != m_filters.end(); ++filter) 
    (*filter)(frame, status);

  return status;
}


void OpalRTPSession::SaveOutOfOrderPacket(RTP_DataFrame & frame)
{
  WORD sequenceNumber = frame.GetSequenceNumber();

  PTRACE(m_outOfOrderPackets.empty() ? 2 : 5,
         "RTP\tSession " << m_sessionId << ", ssrc=" << syncSourceIn << ", "
         << (m_outOfOrderPackets.empty() ? "first" : "next") << " out of order packet, got "
         << sequenceNumber << " expected " << expectedSequenceNumber);

  std::list<RTP_DataFrame>::iterator it;
  for (it  = m_outOfOrderPackets.begin(); it != m_outOfOrderPackets.end(); ++it) {
    if (sequenceNumber > it->GetSequenceNumber())
      break;
  }

  m_outOfOrderPackets.insert(it, frame);
  frame.MakeUnique();
}


bool OpalRTPSession::InsertReportPacket(RTP_ControlFrame & report)
{
  report.StartNewPacket();

  // No packets sent yet, so only set RR
  if (packetsSent == 0) {

    // Send RR as we are not transmitting
    report.SetPayloadType(RTP_ControlFrame::e_ReceiverReport);

    // if no packets received, put in an empty report
    if (packetsReceived == 0) {
      report.SetPayloadSize(sizeof(PUInt32b));  // length is SSRC 
      report.SetCount(0);

      // add the SSRC to the start of the payload
      *(PUInt32b *)report.GetPayloadPtr() = syncSourceOut;
    }
    else {
      report.SetPayloadSize(sizeof(PUInt32b) + sizeof(RTP_ControlFrame::ReceiverReport));  // length is SSRC of packet sender plus RR
      report.SetCount(1);
      BYTE * payload = report.GetPayloadPtr();

      // add the SSRC to the start of the payload
      *(PUInt32b *)payload = syncSourceOut;

      // add the RR after the SSRC
      AddReceiverReport(*(RTP_ControlFrame::ReceiverReport *)(payload+sizeof(PUInt32b)));
    }
  }
  else {
    // send SR and RR
    report.SetPayloadType(RTP_ControlFrame::e_SenderReport);
    report.SetPayloadSize(sizeof(PUInt32b) + sizeof(RTP_ControlFrame::SenderReport));  // length is SSRC of packet sender plus SR
    report.SetCount(0);
    BYTE * payload = report.GetPayloadPtr();

    // add the SSRC to the start of the payload
    *(PUInt32b *)payload = syncSourceOut;

    // add the SR after the SSRC
    RTP_ControlFrame::SenderReport * sender = (RTP_ControlFrame::SenderReport *)(payload+sizeof(PUInt32b));
    PTime now;
    sender->ntp_sec  = (DWORD)(now.GetTimeInSeconds()+SecondsFrom1900to1970); // Convert from 1970 to 1900
    sender->ntp_frac = now.GetMicrosecond()*4294; // Scale microseconds to "fraction" from 0 to 2^32
    sender->rtp_ts   = lastSentTimestamp;
    sender->psent    = packetsSent;
    sender->osent    = octetsSent;

    PTRACE(3, "RTP\tSession " << m_sessionId << ", SentSenderReport:"
              " ssrc=" << syncSourceOut
           << " ntp=" << sender->ntp_sec << '.' << sender->ntp_frac
           << " rtp=" << sender->rtp_ts
           << " psent=" << sender->psent
           << " osent=" << sender->osent);

    if (syncSourceIn != 0) {
      report.SetPayloadSize(sizeof(PUInt32b) + sizeof(RTP_ControlFrame::SenderReport) + sizeof(RTP_ControlFrame::ReceiverReport));
      report.SetCount(1);
      AddReceiverReport(*(RTP_ControlFrame::ReceiverReport *)(payload+sizeof(PUInt32b)+sizeof(RTP_ControlFrame::SenderReport)));
    }
  }

  report.EndPacket();

  // Wait a fuzzy amount of time so things don't get into lock step
  int interval = (int)reportTimeInterval.GetMilliSeconds();
  int third = interval/3;
  interval += PRandom::Number()%(2*third);
  interval -= third;
  m_reportTimer = interval;

  return true;
}


bool OpalRTPSession::SendReport()
{
  PWaitAndSignal mutex(m_reportMutex);

  if (m_reportTimer.IsRunning())
    return true;

  // Have not got anything yet, do nothing
  if (packetsSent == 0 && packetsReceived == 0) {
    m_reportTimer = reportTimeInterval;
    return true;
  }

  RTP_ControlFrame report;
  InsertReportPacket(report);

  // Add the SDES part to compound RTCP packet
  PTRACE(3, "RTP\tSession " << m_sessionId << ", sending SDES: " << canonicalName);
  report.StartNewPacket();

  report.SetCount(0); // will be incremented automatically
  report.StartSourceDescription  (syncSourceOut);
  report.AddSourceDescriptionItem(RTP_ControlFrame::e_CNAME, canonicalName);
  report.AddSourceDescriptionItem(RTP_ControlFrame::e_TOOL, toolName);
  report.EndPacket();
  
#if OPAL_RTCP_XR
  //Generate and send RTCP-XR packet
  InsertExtendedReportPacket(report);
#endif

  return WriteControl(report);
}


#if OPAL_STATISTICS
void OpalRTPSession::GetStatistics(OpalMediaStatistics & statistics, bool receiver) const
{
  statistics.m_totalBytes        = receiver ? GetOctetsReceived()     : GetOctetsSent();
  statistics.m_totalPackets      = receiver ? GetPacketsReceived()    : GetPacketsSent();
  statistics.m_packetsLost       = receiver ? GetPacketsLost()        : GetPacketsLostByRemote();
  statistics.m_packetsOutOfOrder = receiver ? GetPacketsOutOfOrder()  : 0;
  statistics.m_packetsTooLate    = receiver ? GetPacketsTooLate()     : 0;
  statistics.m_packetOverruns    = receiver ? GetPacketOverruns()     : 0;
  statistics.m_minimumPacketTime = receiver ? GetMinimumReceiveTime() : GetMinimumSendTime();
  statistics.m_averagePacketTime = receiver ? GetAverageReceiveTime() : GetAverageSendTime();
  statistics.m_maximumPacketTime = receiver ? GetMaximumReceiveTime() : GetMaximumSendTime();
  statistics.m_averageJitter     = receiver ? GetAvgJitterTime()      : GetJitterTimeOnRemote();
  statistics.m_maximumJitter     = receiver ? GetMaxJitterTime()      : 0;
  statistics.m_jitterBufferDelay = receiver ? GetJitterBufferDelay()  : 0;
}
#endif


static OpalRTPSession::ReceiverReportArray
BuildReceiverReportArray(const RTP_ControlFrame & frame, PINDEX offset)
{
  OpalRTPSession::ReceiverReportArray reports;

  const RTP_ControlFrame::ReceiverReport * rr = (const RTP_ControlFrame::ReceiverReport *)(frame.GetPayloadPtr()+offset);
  for (PINDEX repIdx = 0; repIdx < (PINDEX)frame.GetCount(); repIdx++) {
    OpalRTPSession::ReceiverReport * report = new OpalRTPSession::ReceiverReport;
    report->sourceIdentifier = rr->ssrc;
    report->fractionLost = rr->fraction;
    report->totalLost = rr->GetLostPackets();
    report->lastSequenceNumber = rr->last_seq;
    report->jitter = rr->jitter;
    report->lastTimestamp = (PInt64)(DWORD)rr->lsr;
    report->delay = ((PInt64)rr->dlsr << 16)/1000;
    reports.SetAt(repIdx, report);
    rr++;
  }

  return reports;
}


OpalRTPSession::SendReceiveStatus OpalRTPSession::OnReceiveControl(RTP_ControlFrame & frame)
{
  PTRACE(6, "RTP\tSession " << m_sessionId << ", OnReceiveControl - "
         << frame.GetSize() << " bytes:\n" << hex << setprecision(2) << setfill('0') << frame << dec);
  do {
    BYTE * payload = frame.GetPayloadPtr();
    PINDEX size = frame.GetPayloadSize(); 
    if ((payload == NULL) || (size == 0) || ((payload + size) > (frame.GetPointer() + frame.GetSize()))){
      /* TODO: 1.shall we test for a maximum size ? Indeed but what's the value ? *
               2. what's the correct exit status ? */
      PTRACE(2, "RTP\tSession " << m_sessionId << ", OnReceiveControl invalid frame");
      break;
    }

    switch (frame.GetPayloadType()) {
      case RTP_ControlFrame::e_SenderReport :
        if (size >= (PINDEX)(sizeof(PUInt32b)+sizeof(RTP_ControlFrame::SenderReport)+frame.GetCount()*sizeof(RTP_ControlFrame::ReceiverReport))) {
          SenderReport sender;
          sender.sourceIdentifier = *(const PUInt32b *)payload;
          const RTP_ControlFrame::SenderReport & sr = *(const RTP_ControlFrame::SenderReport *)(payload+sizeof(PUInt32b));
          sender.realTimestamp = PTime(sr.ntp_sec-SecondsFrom1900to1970, sr.ntp_frac/4294);

          // Save the receive time
          lastSRTimestamp = sender.realTimestamp;
          lastSRReceiveTime.SetCurrentTime();
          senderReportsReceived++;

          sender.rtpTimestamp = sr.rtp_ts;
          sender.packetsSent = sr.psent;
          sender.octetsSent = sr.osent;
#if OPAL_RTCP_XR
          OnRxSenderReportToMetrics(frame, sizeof(PUInt32b)+sizeof(RTP_ControlFrame::SenderReport));
#endif
          OnRxSenderReport(sender, BuildReceiverReportArray(frame, sizeof(PUInt32b)+sizeof(RTP_ControlFrame::SenderReport)));
        }
        else {
          PTRACE(2, "RTP\tSession " << m_sessionId << ", SenderReport packet truncated");
        }
        break;

      case RTP_ControlFrame::e_ReceiverReport :
        if (size >= (PINDEX)(sizeof(PUInt32b)+frame.GetCount()*sizeof(RTP_ControlFrame::ReceiverReport)))
          OnRxReceiverReport(*(const PUInt32b *)payload, BuildReceiverReportArray(frame, sizeof(PUInt32b)));
        else {
          PTRACE(2, "RTP\tSession " << m_sessionId << ", ReceiverReport packet truncated");
        }
        break;

      case RTP_ControlFrame::e_SourceDescription :
        if (size >= (PINDEX)(frame.GetCount()*sizeof(RTP_ControlFrame::SourceDescription))) {
          SourceDescriptionArray descriptions;
          const RTP_ControlFrame::SourceDescription * sdes = (const RTP_ControlFrame::SourceDescription *)payload;
          PINDEX srcIdx;
          for (srcIdx = 0; srcIdx < (PINDEX)frame.GetCount(); srcIdx++) {
            descriptions.SetAt(srcIdx, new SourceDescription(sdes->src));
            const RTP_ControlFrame::SourceDescription::Item * item = sdes->item;
            PINDEX uiSizeCurrent = 0;   /* current size of the items already parsed */
            while ((item != NULL) && (item->type != RTP_ControlFrame::e_END)) {
              descriptions[srcIdx].items.SetAt(item->type, PString(item->data, item->length));
              uiSizeCurrent += item->GetLengthTotal();
              PTRACE(4,"RTP\tSession " << m_sessionId << ", SourceDescription item " << item << ", current size = " << uiSizeCurrent);
              
              /* avoid reading where GetNextItem() shall not */
              if (uiSizeCurrent >= size){
                PTRACE(4,"RTP\tSession " << m_sessionId << ", SourceDescription end of items");
                item = NULL;
                break;
              }

              item = item->GetNextItem();
            }

            /* RTP_ControlFrame::e_END doesn't have a length field, so do NOT call item->GetNextItem()
               otherwise it reads over the buffer */
            if (item == NULL || 
                item->type == RTP_ControlFrame::e_END || 
                (sdes = (const RTP_ControlFrame::SourceDescription *)item->GetNextItem()) == NULL)
              break;
          }
          OnRxSourceDescription(descriptions);
        }
        else {
          PTRACE(2, "RTP\tSession " << m_sessionId << ", SourceDescription packet truncated");
        }
        break;

      case RTP_ControlFrame::e_Goodbye :
        if (size >= 4) {
          PString str;
          PINDEX count = frame.GetCount()*4;
    
          if (size > count) {
            if (size >= (PINDEX)(payload[count] + sizeof(DWORD) /*SSRC*/ + sizeof(unsigned char) /* length */))
              str = PString((const char *)(payload+count+1), payload[count]);
            else {
              PTRACE(2, "RTP\tSession " << m_sessionId << ", Goodbye packet invalid");
            }
          }

          PDWORDArray sources(count);
          for (PINDEX i = 0; i < count; i++)
            sources[i] = ((const PUInt32b *)payload)[i];
          OnRxGoodbye(sources, str);
        }
        else {
          PTRACE(2, "RTP\tSession " << m_sessionId << ", Goodbye packet truncated");
        }

        if (closeOnBye) {
          PTRACE(3, "RTP\tSession " << m_sessionId << ", Goodbye packet closing transport");
          return e_AbortTransport;
        }
        break;

      case RTP_ControlFrame::e_ApplDefined :
        if (size >= 4) {
          PString str((const char *)(payload+4), 4);
          OnRxApplDefined(str, frame.GetCount(), *(const PUInt32b *)payload,
          payload+8, frame.GetPayloadSize()-8);
        }
        else {
          PTRACE(2, "RTP\tSession " << m_sessionId << ", ApplDefined packet truncated");
        }
        break;

#if OPAL_RTCP_XR
      case RTP_ControlFrame::e_ExtendedReport :
        if (size >= (PINDEX)(sizeof(PUInt32b)+frame.GetCount()*sizeof(RTP_ControlFrame::ExtendedReport)))
          OnRxExtendedReport(*(const PUInt32b *)payload, BuildExtendedReportArray(frame, sizeof(PUInt32b)));
        else {
          PTRACE(2, "RTP\tSession " << m_sessionId << ", ReceiverReport packet truncated");
        }
        break;
#endif

  #if OPAL_VIDEO
    case RTP_ControlFrame::e_IntraFrameRequest :
      PTRACE(4, "RTP\tSession " << m_sessionId << ", received RFC2032 FIR");
      m_connection.OnRxIntraFrameRequest(*this, true);
      break;

      case RTP_ControlFrame::e_PayloadSpecificFeedBack :
        switch (frame.GetFbType()) {
          case RTP_ControlFrame::e_PictureLossIndication :
            PTRACE(4, "RTP\tSession " << m_sessionId << ", received RFC5104 PLI");
            m_connection.OnRxIntraFrameRequest(*this, false);
            break;

          case RTP_ControlFrame::e_FullIntraRequest :
            PTRACE(4, "RTP\tSession " << m_sessionId << ", received RFC5104 FIR");
            m_connection.OnRxIntraFrameRequest(*this, true);
            break;

          default :
            PTRACE(2, "RTP\tSession " << m_sessionId << ", Unknown Payload Specific feedback type: " << frame.GetFbType());
        }
        break;
  #endif

      default :
        PTRACE(2, "RTP\tSession " << m_sessionId << ", Unknown control payload type: " << frame.GetPayloadType());
    }
  } while (frame.ReadNextPacket());

  return e_ProcessPacket;
}


void OpalRTPSession::OnRxSenderReport(const SenderReport & sender, const ReceiverReportArray & reports)
{
  m_syncTimestamp = sender.rtpTimestamp;
  m_syncRealTime = sender.realTimestamp;

#if PTRACING
  if (PTrace::CanTrace(3)) {
    ostream & strm = PTrace::Begin(3, __FILE__, __LINE__, this);
    strm << "RTP\tSession " << m_sessionId << ", OnRxSenderReport: " << sender << '\n';
    for (PINDEX i = 0; i < reports.GetSize(); i++)
      strm << "  RR: " << reports[i] << '\n';
    strm << PTrace::End;
  }
#endif

  OnReceiverReports(reports);
}


void OpalRTPSession::OnRxReceiverReport(DWORD PTRACE_PARAM(src), const ReceiverReportArray & reports)
{
#if PTRACING
  if (PTrace::CanTrace(3)) {
    ostream & strm = PTrace::Begin(2, __FILE__, __LINE__, this);
    strm << "RTP\tSession " << m_sessionId << ", OnReceiverReport: ssrc=" << src << '\n';
    for (PINDEX i = 0; i < reports.GetSize(); i++)
      strm << "  RR: " << reports[i] << '\n';
    strm << PTrace::End;
  }
#endif
  OnReceiverReports(reports);
}


void OpalRTPSession::OnReceiverReports(const ReceiverReportArray & reports)
{
  for (PINDEX i = 0; i < reports.GetSize(); i++) {
    if (reports[i].sourceIdentifier == syncSourceOut) {
      packetsLostByRemote = reports[i].totalLost;
      jitterLevelOnRemote = reports[i].jitter;
      break;
    }
  }
}


void OpalRTPSession::OnRxSourceDescription(const SourceDescriptionArray & PTRACE_PARAM(description))
{
#if PTRACING
  if (PTrace::CanTrace(3)) {
    ostream & strm = PTrace::Begin(3, __FILE__, __LINE__, this);
    strm << "RTP\tSession " << m_sessionId << ", OnSourceDescription: " << description.GetSize() << " entries";
    for (PINDEX i = 0; i < description.GetSize(); i++)
      strm << "\n  " << description[i];
    strm << PTrace::End;
  }
#endif
}


void OpalRTPSession::OnRxGoodbye(const PDWORDArray & PTRACE_PARAM(src), const PString & PTRACE_PARAM(reason))
{
  PTRACE(3, "RTP\tSession " << m_sessionId << ", OnGoodbye: \"" << reason << "\" srcs=" << src);
}


void OpalRTPSession::OnRxApplDefined(const PString & PTRACE_PARAM(type),
          unsigned PTRACE_PARAM(subtype), DWORD PTRACE_PARAM(src),
          const BYTE * /*data*/, PINDEX PTRACE_PARAM(size))
{
  PTRACE(3, "RTP\tSession " << m_sessionId << ", OnApplDefined: \""
         << type << "\"-" << subtype << " " << src << " [" << size << ']');
}


void OpalRTPSession::ReceiverReport::PrintOn(ostream & strm) const
{
  strm << "ssrc=" << sourceIdentifier
       << " fraction=" << fractionLost
       << " lost=" << totalLost
       << " last_seq=" << lastSequenceNumber
       << " jitter=" << jitter
       << " lsr=" << lastTimestamp
       << " dlsr=" << delay;
}


void OpalRTPSession::SenderReport::PrintOn(ostream & strm) const
{
  strm << "ssrc=" << sourceIdentifier
       << " ntp=" << realTimestamp.AsString("yyyy/M/d-h:m:s.uuuu")
       << " rtp=" << rtpTimestamp
       << " psent=" << packetsSent
       << " osent=" << octetsSent;
}


void OpalRTPSession::SourceDescription::PrintOn(ostream & strm) const
{
  static const char * const DescriptionNames[RTP_ControlFrame::NumDescriptionTypes] = {
    "END", "CNAME", "NAME", "EMAIL", "PHONE", "LOC", "TOOL", "NOTE", "PRIV"
  };

  strm << "ssrc=" << sourceIdentifier;
  for (POrdinalToString::const_iterator it = items.begin(); it != items.end(); ++it) {
    unsigned typeNum = it->first;
    strm << "\n  item[" << typeNum << "]: type=";
    if (typeNum < PARRAYSIZE(DescriptionNames))
      strm << DescriptionNames[typeNum];
    else
      strm << typeNum;
    strm << " data=\"" << it->second << '"';
  }
}


DWORD OpalRTPSession::GetPacketsTooLate() const
{
  JitterBufferPtr jitter = m_jitterBuffer; // Increase reference count
  return jitter != NULL ? jitter->GetPacketsTooLate() : 0;
}


DWORD OpalRTPSession::GetPacketOverruns() const
{
  JitterBufferPtr jitter = m_jitterBuffer; // Increase reference count
  return jitter != NULL ? jitter->GetBufferOverruns() : 0;
}


void OpalRTPSession::SendIntraFrameRequest(bool rfc2032, bool pictureLoss)
{
  PTRACE(3, "RTP\tSession " << m_sessionId << ", SendIntraFrameRequest using "
         << (rfc2032 ? "RFC2032" : (pictureLoss ? "RFC4585 PLI" : "RFC5104 FIR")));

  // Create packet
  RTP_ControlFrame request;
  InsertReportPacket(request);

  request.StartNewPacket();

  if (rfc2032) {
    // Create packet
    request.SetPayloadType(RTP_ControlFrame::e_IntraFrameRequest);
    request.SetPayloadSize(4);
    // Insert SSRC
    request.SetCount(1);
    BYTE * payload = request.GetPayloadPtr();
    *(PUInt32b *)payload = syncSourceOut;
  }
  else {
    request.SetPayloadType(RTP_ControlFrame::e_PayloadSpecificFeedBack);
    if (pictureLoss)
      request.SetFbType(RTP_ControlFrame::e_PictureLossIndication, 0);
    else {
      request.SetFbType(RTP_ControlFrame::e_FullIntraRequest, sizeof(RTP_ControlFrame::FbFIR));
      RTP_ControlFrame::FbFIR * fir = (RTP_ControlFrame::FbFIR *)request.GetPayloadPtr();
      fir->requestSSRC = syncSourceIn;
    }
    RTP_ControlFrame::FbFCI * fci = (RTP_ControlFrame::FbFCI *)request.GetPayloadPtr();
    fci->senderSSRC = syncSourceOut;
  }

  // Send it
  request.EndPacket();
  WriteControl(request);
}


void OpalRTPSession::SendTemporalSpatialTradeOff(unsigned tradeOff)
{
  PTRACE(3, "RTP\tSession " << m_sessionId << ", SendTemporalSpatialTradeOff " << tradeOff);

  RTP_ControlFrame request;
  InsertReportPacket(request);

  request.StartNewPacket();

  request.SetPayloadType(RTP_ControlFrame::e_PayloadSpecificFeedBack);
  request.SetFbType(RTP_ControlFrame::e_TemporalSpatialTradeOffRequest, sizeof(RTP_ControlFrame::FbTSTO));
  RTP_ControlFrame::FbTSTO * tsto = (RTP_ControlFrame::FbTSTO *)request.GetPayloadPtr();
  tsto->requestSSRC = syncSourceIn;
  tsto->tradeOff = (BYTE)tradeOff;

  // Send it
  request.EndPacket();
  WriteControl(request);
}


void OpalRTPSession::AddFilter(const FilterNotifier & filter)
{
  // ensures that a filter is added only once
  if (std::find(m_filters.begin(), m_filters.end(), filter) == m_filters.end())
    m_filters.push_back(filter);
}


/////////////////////////////////////////////////////////////////////////////

static void SetMinBufferSize(PUDPSocket & sock, int buftype, int bufsz)
{
  int sz = 0;
  if (!sock.GetOption(buftype, sz)) {
    PTRACE(1, "RTP_UDP\tGetOption(" << sock.GetHandle() << ',' << buftype << ") failed: " << sock.GetErrorText());
    return;
  }

  // Already big enough
  if (sz >= bufsz)
    return;

  for (; bufsz >= 1024; bufsz /= 2) {
    // Set to new size
    if (!sock.SetOption(buftype, bufsz)) {
      PTRACE(1, "RTP_UDP\tSetOption(" << sock.GetHandle() << ',' << buftype << ',' << bufsz << ") failed: " << sock.GetErrorText());
      continue;
    }

    // As some stacks lie about setting the buffer size, we double check.
    if (!sock.GetOption(buftype, sz)) {
      PTRACE(1, "RTP_UDP\tGetOption(" << sock.GetHandle() << ',' << buftype << ") failed: " << sock.GetErrorText());
      return;
    }

    if (sz >= bufsz) {
      PTRACE(4, "RTP_UDP\tSetOption(" << sock.GetHandle() << ',' << buftype << ',' << bufsz << ") succeeded.");
      return;
    }

    PTRACE(1, "RTP_UDP\tSetOption(" << sock.GetHandle() << ',' << buftype << ',' << bufsz << ") failed, even though it said it succeeded!");
  }
}


OpalTransportAddress OpalRTPSession::GetLocalMediaAddress() const
{
  if (localAddress.IsValid() && localDataPort != 0)
    return OpalTransportAddress(localAddress, localDataPort, OpalTransportAddress::UdpPrefix());
  else
    return OpalTransportAddress();
}


OpalTransportAddress OpalRTPSession::GetRemoteMediaAddress() const
{
  if (remoteAddress.IsValid() && remoteDataPort != 0)
    return OpalTransportAddress(remoteAddress, remoteDataPort, OpalTransportAddress::UdpPrefix());
  else
    return OpalTransportAddress();
}


bool OpalRTPSession::SetRemoteMediaAddress(const OpalTransportAddress & address)
{
  PIPSocket::Address ip;
  WORD port = remoteDataPort;
  return address.GetIpAndPort(ip, port) && InternalSetRemoteAddress(ip, port, true);
}


OpalTransportAddress OpalRTPSession::GetRemoteControlAddress() const
{
  if (remoteAddress.IsValid() && remoteControlPort != 0)
    return OpalTransportAddress(remoteAddress, remoteControlPort, OpalTransportAddress::UdpPrefix());
  else
    return OpalTransportAddress();
}


bool OpalRTPSession::SetRemoteControlAddress(const OpalTransportAddress & address)
{
  PIPSocket::Address ip;
  WORD port = remoteControlPort;
  return address.GetIpAndPort(ip, port) && InternalSetRemoteAddress(ip, port, false);
}


OpalMediaStream * OpalRTPSession::CreateMediaStream(const OpalMediaFormat & mediaFormat, 
                                                    unsigned sessionId, 
                                                    bool isSource)
{
  if (PAssert(m_sessionId == sessionId && m_mediaType == mediaFormat.GetMediaType(), PLogicError))
    return new OpalRTPMediaStream(dynamic_cast<OpalRTPConnection &>(m_connection),
                                  mediaFormat, isSource, *this,
                                  m_connection.GetMinAudioJitterDelay(),
                                  m_connection.GetMaxAudioJitterDelay());

  return NULL;
}


void OpalRTPSession::ApplyQOS(const PIPSocket::Address & addr)
{
  if (controlSocket != NULL)
    controlSocket->SetSendAddress(addr,GetRemoteControlPort());
  if (dataSocket != NULL)
    dataSocket->SetSendAddress(addr,GetRemoteDataPort());
  appliedQOS = true;
}

#if P_QOS

bool OpalRTPSession::ModifyQOS(RTP_QOS * rtpqos)
{
  bool retval = false;

  if (rtpqos == NULL)
    return retval;

  if (controlSocket != NULL)
    retval = controlSocket->ModifyQoSSpec(&(rtpqos->ctrlQoS));
    
  if (dataSocket != NULL)
    retval &= dataSocket->ModifyQoSSpec(&(rtpqos->dataQoS));

  appliedQOS = false;
  return retval;
}

#endif


void OpalRTPSession::SetExternalTransport(const OpalTransportAddressArray & transports)
{
  if (transports.IsEmpty())
    return;

  PWaitAndSignal mutex(dataMutex);

  delete dataSocket;
  delete controlSocket;
  dataSocket = NULL;
  controlSocket = NULL;

  transports[0].GetIpAndPort(localAddress, localDataPort);
  if (transports.GetSize() > 1)
    transports[1].GetIpAndPort(localAddress, localControlPort);
  else
    localControlPort = (WORD)(localDataPort+1);

  OpalMediaSession::SetExternalTransport(transports);
}


bool OpalRTPSession::Open(const PString & localInterface)
{
  PTRACE(5, "RTP\tSession " << m_sessionId << ", opening.");

  PWaitAndSignal mutex(dataMutex);

  if (IsExternalTransport())
    return true;

  if (dataSocket != NULL && controlSocket != NULL)
    return true;

  m_firstControl = true;
  byeSent = false;
  shutdownRead = false;
  shutdownWrite = false;

  OpalManager & manager = m_connection.GetEndPoint().GetManager();

#if P_NAT
  PNatMethod * natMethod = m_connection.GetNatMethod(remoteAddress);
  if (natMethod != NULL) {
    PTRACE(4, "RTP\tNAT Method " << natMethod->GetName() << " selected for call.");
    natMethod->CreateSocketPairAsync(m_connection.GetToken());
  }
#endif

  WORD firstPort = manager.GetRtpIpPortPair();

  delete dataSocket;
  delete controlSocket;
  dataSocket = NULL;
  controlSocket = NULL;

  localDataPort    = firstPort;
  localControlPort = (WORD)(firstPort + 1);

  PIPSocket::Address bindingAddress = localInterface;

#if P_NAT
  if (natMethod != NULL && natMethod->IsAvailable(bindingAddress)) {
    switch (natMethod->GetRTPSupport()) {
      case PNatMethod::RTPIfSendMedia :
        /* This NAT variant will work if we send something out through the
            NAT port to "open" it so packets can then flow inward. We set
            this flag to make that happen as soon as we get the remotes IP
            address and port to send to.
          */
        localHasNAT = true;
        // Then do case for full cone support and create STUN sockets

      case PNatMethod::RTPSupported :
        {
        PTRACE ( 4, "RTP\tAttempting natMethod: " << natMethod->GetName() );            
        void * natInfo = NULL;
#if OPAL_H460_NAT
        H323Connection* pCon = dynamic_cast<H323Connection*>(&m_connection);
        if (pCon != NULL)
          natInfo = pCon->BuildSessionInformation(GetSessionID());
#endif
        if (natMethod->GetSocketPairAsync(m_connection.GetToken(), dataSocket, controlSocket, bindingAddress, natInfo)) {
          PTRACE(4, "RTP\tSession " << m_sessionId << ", " << natMethod->GetName() << " created STUN RTP/RTCP socket pair.");
          dataSocket->GetLocalAddress(bindingAddress, localDataPort);
          controlSocket->GetLocalAddress(bindingAddress, localControlPort);
        }
        else {
          PTRACE(2, "RTP\tSession " << m_sessionId << ", " << natMethod->GetName()
                  << " could not create STUN RTP/RTCP socket pair; trying to create individual sockets.");
          if (natMethod->CreateSocket(dataSocket, bindingAddress) && natMethod->CreateSocket(controlSocket, bindingAddress)) {
            dataSocket->GetLocalAddress(bindingAddress, localDataPort);
            controlSocket->GetLocalAddress(bindingAddress, localControlPort);
          }
          else {
            delete dataSocket;
            delete controlSocket;
            dataSocket = NULL;
            controlSocket = NULL;
            PTRACE(2, "RTP\tSession " << m_sessionId << ", " << natMethod->GetName()
                    << " could not create STUN RTP/RTCP sockets individually either, using normal sockets.");
          }
        }
        }
        break;

      default :
        /* We canot use NAT traversal method (e.g. STUN) to create sockets
            in the remaining modes as the NAT router will then not let us
            talk to the real RTP destination. All we can so is bind to the
            local interface the NAT is on and hope the NAT router is doing
            something sneaky like symmetric port forwarding. */
        natMethod->GetInterfaceAddress(bindingAddress);
        break;
    }
  }
#endif

  if (dataSocket == NULL || controlSocket == NULL) {
    dataSocket = new PUDPSocket();
    controlSocket = new PUDPSocket();
    while (!   dataSocket->Listen(bindingAddress, 1, localDataPort) ||
           !controlSocket->Listen(bindingAddress, 1, localControlPort)) {
      dataSocket->Close();
      controlSocket->Close();

      localDataPort = manager.GetRtpIpPortPair();
      if (localDataPort == firstPort) {
        PTRACE(1, "RTPCon\tNo ports available for RTP session " << m_sessionId << ","
                  " base=" << manager.GetRtpIpPortBase() << ","
                  " max=" << manager.GetRtpIpPortMax() << ","
                  " bind=" << bindingAddress << ","
                  " for " << m_connection);
        return false; // Used up all the available ports!
      }
      localControlPort = (WORD)(localDataPort + 1);
    }
  }

#ifndef __BEOS__
  // Set the IP Type Of Service field for prioritisation of media UDP packets
  // through some Cisco routers and Linux boxes
  if (!dataSocket->SetOption(IP_TOS, manager.GetMediaTypeOfService(m_mediaType), IPPROTO_IP)) {
    PTRACE(1, "RTP_UDP\tSession " << m_sessionId << ", could not set TOS field in IP header: " << dataSocket->GetErrorText());
  }

  // Increase internal buffer size on media UDP sockets
  SetMinBufferSize(*dataSocket,    SO_RCVBUF, isAudio ? RTP_AUDIO_RX_BUFFER_SIZE : RTP_VIDEO_RX_BUFFER_SIZE);
  SetMinBufferSize(*dataSocket,    SO_SNDBUF, RTP_DATA_TX_BUFFER_SIZE);
  SetMinBufferSize(*controlSocket, SO_RCVBUF, RTP_CTRL_BUFFER_SIZE);
  SetMinBufferSize(*controlSocket, SO_SNDBUF, RTP_CTRL_BUFFER_SIZE);
#endif

  if (canonicalName.Find('@') == P_MAX_INDEX)
    canonicalName += '@' + GetLocalHostName();

  dataSocket->GetLocalAddress(localAddress);
  manager.TranslateIPAddress(localAddress, remoteAddress);

  PTRACE(3, "RTP_UDP\tSession " << m_sessionId << " opened: "
         << localAddress << ':' << localDataPort << '-' << localControlPort
         << " ssrc=" << syncSourceOut);

  OpalRTPEndPoint * ep = dynamic_cast<OpalRTPEndPoint *>(&m_connection.GetEndPoint());
  if (PAssert(ep != NULL, "RTP createed by non OpalRTPEndPoint derived class"))
    ep->SetConnectionByRtpLocalPort(this, &m_connection);

  return true;
}


bool OpalRTPSession::Close()
{
  bool ok = Shutdown(true) | Shutdown(false);

  OpalRTPEndPoint * ep = dynamic_cast<OpalRTPEndPoint *>(&m_connection.GetEndPoint());
  if (ep != NULL)
    ep->SetConnectionByRtpLocalPort(this, NULL);

  // We need to do this to make sure that the sockets are not
  // deleted before select decides there is no more data coming
  // over them and exits the reading thread.
  SetJitterBufferSize(0, 0);

  delete dataSocket;
  dataSocket = NULL;

  delete controlSocket;
  controlSocket = NULL;

  return ok;
}


bool OpalRTPSession::Shutdown(bool reading)
{
  if (reading) {
    {
      PWaitAndSignal mutex(dataMutex);

      if (shutdownRead) {
        PTRACE(4, "RTP_UDP\tSession " << m_sessionId << ", read already shut down .");
        return false;
      }

      PTRACE(3, "RTP_UDP\tSession " << m_sessionId << ", Shutting down read.");

      syncSourceIn = 0;
      shutdownRead = true;

      if (dataSocket != NULL && controlSocket != NULL) {
        PIPSocket::Address addr;
        controlSocket->GetLocalAddress(addr);
        if (addr.IsAny())
          PIPSocket::GetHostAddress(addr);
        dataSocket->WriteTo("", 1, addr, controlSocket->GetPort());
      }
    }

    SetJitterBufferSize(0, 0); // Kill jitter buffer too, but outside mutex
  }
  else {
    if (shutdownWrite) {
      PTRACE(4, "RTP_UDP\tSession " << m_sessionId << ", write already shut down .");
      return false;
    }

    PTRACE(3, "RTP_UDP\tSession " << m_sessionId << ", shutting down write.");
    shutdownWrite = true;
  }

  return true;
}


void OpalRTPSession::Restart(bool reading)
{
  PWaitAndSignal mutex(dataMutex);

  if (reading)
    shutdownRead = false;
  else
    shutdownWrite = false;

  badTransmitCounter = 0;

  PTRACE(3, "RTP_UDP\tSession " << m_sessionId << " reopened for " << (reading ? "reading" : "writing"));
}


PString OpalRTPSession::GetLocalHostName()
{
  return PIPSocket::GetHostName();
}


bool OpalRTPSession::InternalSetRemoteAddress(PIPSocket::Address address, WORD port, bool isDataPort)
{
  if (remoteIsNAT) {
    PTRACE(2, "RTP_UDP\tSession " << m_sessionId << ", ignoring remote socket info as remote is behind NAT");
    return true;
  }

  if (!PAssert(address.IsValid() && port != 0,PInvalidParameter))
    return false;

  PTRACE(3, "RTP_UDP\tSession " << m_sessionId << ", SetRemoteSocketInfo: "
         << (isDataPort ? "data" : "control") << " channel, "
            "new=" << address << ':' << port << ", "
            "local=" << localAddress << ':' << localDataPort << '-' << localControlPort << ", "
            "remote=" << remoteAddress << ':' << remoteDataPort << '-' << remoteControlPort);

  if (localAddress == address && remoteAddress == address && (isDataPort ? localDataPort : localControlPort) == port)
    return true;
  
  remoteAddress = address;
  
  allowOneSyncSourceChange = true;
  allowRemoteTransmitAddressChange = true;
  allowSequenceChange = packetsReceived != 0;

  if (isDataPort) {
    remoteDataPort = port;
    if (remoteControlPort == 0 || allowRemoteTransmitAddressChange)
      remoteControlPort = (WORD)(port + 1);
  }
  else {
    remoteControlPort = port;
    if (remoteDataPort == 0 || allowRemoteTransmitAddressChange)
      remoteDataPort = (WORD)(port - 1);
  }

  if (!appliedQOS)
      ApplyQOS(remoteAddress);

  if (localHasNAT) {
    // If have Port Restricted NAT on local host then send a datagram
    // to remote to open up the port in the firewall for return data.
    static const BYTE dummy[1] = { 0 };
    WriteDataOrControlPDU(dummy, sizeof(dummy), true);
    WriteDataOrControlPDU(dummy, sizeof(dummy), false);
    PTRACE(2, "RTP_UDP\tSession " << m_sessionId << ", sending empty datagrams to open local Port Restricted NAT");
  }

  return true;
}


bool OpalRTPSession::InternalReadData(RTP_DataFrame & frame)
{
  SendReceiveStatus status;
  while ((status = InternalReadData2(frame)) == e_IgnorePacket)
    ;
  return status == e_ProcessPacket;
}


OpalRTPSession::SendReceiveStatus OpalRTPSession::InternalReadData2(RTP_DataFrame & frame)
{
  int selectStatus = WaitForPDU(*dataSocket, *controlSocket, m_reportTimer.GetRemaining());

  {
    PWaitAndSignal mutex(dataMutex);
    if (shutdownRead) {
      PTRACE(3, "RTP_UDP\tSession " << m_sessionId << ", Read shutdown.");
      return e_AbortTransport;
    }
  }

  switch (selectStatus) {
    case -2 :
      switch (ReadControlPDU()) {
        case e_ProcessPacket :
          return shutdownRead ? e_AbortTransport : e_IgnorePacket;
        case e_AbortTransport :
          return e_AbortTransport;
        case e_IgnorePacket :
          return e_IgnorePacket;
      }
      break;

    case -3 :
      if (ReadControlPDU() == e_AbortTransport)
        return e_AbortTransport;
      // Then do -1 case

    case -1 :
      switch (ReadDataPDU(frame)) {
        case e_ProcessPacket :
          return shutdownRead ? e_AbortTransport : OnReceiveData(frame);
        case e_IgnorePacket :
          return e_IgnorePacket ;
        case e_AbortTransport :
          return e_AbortTransport;
      }
      break;

    case 0 :
      return OnReadTimeout(frame);

    case PSocket::Interrupted:
      PTRACE(2, "RTP_UDP\tSession " << m_sessionId << ", Interrupted.");
      return e_AbortTransport;
  }

  PTRACE(1, "RTP_UDP\tSession " << m_sessionId << ", Select error: "
          << PChannel::GetErrorText((PChannel::Errors)selectStatus));
  return e_AbortTransport;
}

int OpalRTPSession::WaitForPDU(PUDPSocket & dataSocket, PUDPSocket & controlSocket, const PTimeInterval & timeout)
{
  return PSocket::Select(dataSocket, controlSocket, timeout);
}

OpalRTPSession::SendReceiveStatus OpalRTPSession::ReadDataOrControlPDU(BYTE * framePtr,
                                                             PINDEX frameSize,
                                                             bool fromDataChannel)
{
#if PTRACING
  const char * channelName = fromDataChannel ? "Data" : "Control";
#endif
  PUDPSocket & socket = *(fromDataChannel ? dataSocket : controlSocket);
  PIPSocket::Address addr;
  WORD port;

  if (socket.ReadFrom(framePtr, frameSize, addr, port)) {
    // If remote address never set from higher levels, then try and figure
    // it out from the first packet received.
    if (!remoteAddress.IsValid()) {
      remoteAddress = addr;
      PTRACE(4, "RTP\tSession " << m_sessionId << ", set remote address from first "
             << channelName << " PDU from " << addr << ':' << port);
    }
    if (fromDataChannel) {
      if (remoteDataPort == 0)
        remoteDataPort = port;
    }
    else {
      if (remoteControlPort == 0)
        remoteControlPort = port;
    }

    if (!remoteTransmitAddress.IsValid())
      remoteTransmitAddress = addr;
    else if (allowRemoteTransmitAddressChange && remoteAddress == addr) {
      remoteTransmitAddress = addr;
      allowRemoteTransmitAddressChange = false;
    }
    else if (remoteTransmitAddress != addr && !allowRemoteTransmitAddressChange) {
      PTRACE(2, "RTP_UDP\tSession " << m_sessionId << ", "
             << channelName << " PDU from incorrect host, "
                " is " << addr << " should be " << remoteTransmitAddress);
      return OpalRTPSession::e_IgnorePacket;
    }

    if (remoteAddress.IsValid() && !appliedQOS) 
      ApplyQOS(remoteAddress);

    badTransmitCounter = 0;

    return OpalRTPSession::e_ProcessPacket;
  }

  switch (socket.GetErrorNumber(PChannel::LastReadError)) {
    case ECONNRESET :
    case ECONNREFUSED :
      PTRACE(2, "RTP_UDP\tSession " << m_sessionId << ", " << channelName << " port on remote not ready.");
      if (++badTransmitCounter == 1) 
        badTransmitStart = PTime();
      else {
        if (badTransmitCounter <= BAD_TRANSMIT_PKT_MAX || (PTime()- badTransmitStart).GetSeconds() < BAD_TRANSMIT_TIME_MAX)
          return OpalRTPSession::e_IgnorePacket;
        PTRACE(2, "RTP_UDP\tSession " << m_sessionId << ", " << channelName << " " << BAD_TRANSMIT_TIME_MAX << " seconds of transmit fails - informing connection");
      }
      return OpalRTPSession::e_IgnorePacket;

    case EMSGSIZE :
      PTRACE(2, "RTP_UDP\tSession " << m_sessionId << ", " << channelName
             << " read packet too large for buffer of " << frameSize << " bytes.");
      return OpalRTPSession::e_IgnorePacket;

    case EAGAIN :
      PTRACE(4, "RTP_UDP\tSession " << m_sessionId << ", " << channelName
             << " read packet interrupted.");
      // Shouldn't happen, but it does.
      return OpalRTPSession::e_IgnorePacket;

    default:
      PTRACE(1, "RTP_UDP\tSession " << m_sessionId << ", " << channelName
             << " read error (" << socket.GetErrorNumber(PChannel::LastReadError) << "): "
             << socket.GetErrorText(PChannel::LastReadError));
      return OpalRTPSession::e_AbortTransport;
  }
}


OpalRTPSession::SendReceiveStatus OpalRTPSession::ReadDataPDU(RTP_DataFrame & frame)
{
  SendReceiveStatus status = ReadDataOrControlPDU(frame.GetPointer(), frame.GetSize(), true);
  if (status != e_ProcessPacket)
    return status;

  // Check received PDU is big enough
  return frame.SetPacketSize(dataSocket->GetLastReadCount()) ? e_ProcessPacket : e_IgnorePacket;
}


OpalRTPSession::SendReceiveStatus OpalRTPSession::OnReadTimeout(RTP_DataFrame & /*frame*/)
{
  return SendReport() ? e_IgnorePacket : e_AbortTransport;
}


OpalRTPSession::SendReceiveStatus OpalRTPSession::ReadControlPDU()
{
  RTP_ControlFrame frame(2048);

  SendReceiveStatus status = ReadDataOrControlPDU(frame.GetPointer(), frame.GetSize(), false);
  if (status != e_ProcessPacket)
    return status;

  PINDEX pduSize = controlSocket->GetLastReadCount();
  if (pduSize < 4 || pduSize < 4+frame.GetPayloadSize()) {
    PTRACE_IF(2, pduSize != 1 || !m_firstControl, "RTP_UDP\tSession " << m_sessionId
              << ", Received control packet too small: " << pduSize << " bytes");
    return e_IgnorePacket;
  }

  m_firstControl = false;
  frame.SetSize(pduSize);
  return OnReceiveControl(frame);
}


bool OpalRTPSession::WriteOOBData(RTP_DataFrame & frame, bool rewriteTimeStamp)
{
  PWaitAndSignal m(dataMutex);

  // set timestamp offset if not already set
  // otherwise offset timestamp
  if (!oobTimeStampBaseEstablished) {
    oobTimeStampBaseEstablished = true;
    oobTimeStampBase            = PTimer::Tick();
    if (rewriteTimeStamp)
      oobTimeStampOutBase = PRandom::Number();
    else
      oobTimeStampOutBase = frame.GetTimestamp();
  }

  // set new timestamp
  if (rewriteTimeStamp) 
    frame.SetTimestamp(oobTimeStampOutBase + ((PTimer::Tick() - oobTimeStampBase).GetInterval() * 8));

  // write the data
  return WriteData(frame);
}


bool OpalRTPSession::WriteData(RTP_DataFrame & frame)
{
  {
    PWaitAndSignal mutex(dataMutex);
    if (shutdownWrite) {
      PTRACE(3, "RTP_UDP\tSession " << m_sessionId << ", write shutdown.");
      return false;
    }
  }

  // Trying to send a PDU before we are set up!
  if (!remoteAddress.IsValid() || remoteDataPort == 0)
    return true;

  switch (OnSendData(frame)) {
    case e_ProcessPacket :
      break;
    case e_IgnorePacket :
      return true;
    case e_AbortTransport :
      return false;
  }

  return WriteDataOrControlPDU(frame.GetPointer(), frame.GetHeaderSize()+frame.GetPayloadSize(), true);
}


bool OpalRTPSession::WriteControl(RTP_ControlFrame & frame)
{
  // Trying to send a PDU before we are set up!
  if (!remoteAddress.IsValid() || remoteControlPort == 0 || controlSocket == NULL)
    return true;

  switch (OnSendControl(frame)) {
    case e_ProcessPacket :
      rtcpPacketsSent++;
      break;
    case e_IgnorePacket :
      return true;
    case e_AbortTransport :
      return false;
  }

  return WriteDataOrControlPDU(frame.GetPointer(), frame.GetSize(), false);
}


bool OpalRTPSession::WriteDataOrControlPDU(const BYTE * framePtr, PINDEX frameSize, bool toDataChannel)
{
  PUDPSocket & socket = *(toDataChannel ? dataSocket : controlSocket);
  WORD port = toDataChannel ? remoteDataPort : remoteControlPort;
  int retry = 0;

  while (!socket.WriteTo(framePtr, frameSize, remoteAddress, port)) {
    switch (socket.GetErrorNumber()) {
      case ECONNRESET :
      case ECONNREFUSED :
        break;

      default:
        PTRACE(1, "RTP_UDP\tSession " << m_sessionId
               << ", write (" << frameSize << " bytes) error on "
               << (toDataChannel ? "data" : "control") << " port ("
               << socket.GetErrorNumber(PChannel::LastWriteError) << "): "
               << socket.GetErrorText(PChannel::LastWriteError));
        return false;
    }

    if (++retry >= 10)
      break;
  }

  PTRACE_IF(2, retry > 0, "RTP_UDP\tSession " << m_sessionId << ", " << (toDataChannel ? "data" : "control")
            << " port on remote not ready " << retry << " time" << (retry > 1 ? "s" : "")
            << (retry < 10 ? "" : ", data never sent"));
  return true;
}


/////////////////////////////////////////////////////////////////////////////

#if 0
SecureRTP_UDP::SecureRTP_UDP(OpalConnection & conn, unsigned sessionId, const OpalMediaType & mediaType)
  : OpalRTPSession(conn, sessionId, mediaType)
{
  securityParms = NULL;
}

SecureRTP_UDP::~SecureRTP_UDP()
{
  delete securityParms;
}

void SecureRTP_UDP::SetSecurityMode(OpalSecurityMode * newParms)
{ 
  if (securityParms != NULL)
    delete securityParms;
  securityParms = newParms; 
}
  
OpalSecurityMode * SecureRTP_UDP::GetSecurityParms() const
{ 
  return securityParms; 
}
#endif


/////////////////////////////////////////////////////////////////////////////
