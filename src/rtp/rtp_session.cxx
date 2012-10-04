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
      if (!m_session.InternalReadData(frame))
        return false;

#if OPAL_RTCP_XR
      RTCP_XR_Metrics * metrics = m_session.GetExtendedMetrics();
      if (metrics != NULL)
        metrics->SetJitterDelay(GetCurrentJitterDelay()/GetTimeUnits());
#endif

      PTRACE(6, "Jitter\tOnReadPacket: Frame from network, timestamp " << frame.GetTimestamp());
      return true;
   }

 protected:
   /**This class extracts data from the outside world by reading from this session variable */
   OpalRTPSession & m_session;
};


#define new PNEW

/////////////////////////////////////////////////////////////////////////////

const PCaselessString & OpalRTPSession::RTP_AVP () { static const PConstCaselessString s("RTP/AVP" ); return s; }
const PCaselessString & OpalRTPSession::RTP_AVPF() { static const PConstCaselessString s("RTP/AVPF"); return s; }

PFACTORY_CREATE(OpalMediaSessionFactory, OpalRTPSession, OpalRTPSession::RTP_AVP());
static bool RegisteredAVPF = OpalMediaSessionFactory::RegisterAs(OpalRTPSession::RTP_AVPF(), OpalRTPSession::RTP_AVP());

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
  , m_isAudio(init.m_mediaType == OpalMediaType::Audio())
  , m_timeUnits(m_isAudio ? 8 : 90)
  , m_canonicalName(PProcess::Current().GetUserName())
  , m_toolName(PProcess::Current().GetName())
  , m_maxNoReceiveTime(init.m_connection.GetEndPoint().GetManager().GetNoMediaTimeout())
  , m_maxNoTransmitTime(0, 10)          // Sending data for 10 seconds, ICMP says still not there
  , syncSourceOut(PRandom::Number())
  , syncSourceIn(0)
  , lastSentTimestamp(0)  // should be calculated, but we'll settle for initialising it)
  , allowAnySyncSource(true)
  , allowOneSyncSourceChange(false)
  , allowRemoteTransmitAddressChange(false)
  , allowSequenceChange(false)
  , txStatisticsInterval(100)
  , rxStatisticsInterval(100)
  , lastSentSequenceNumber((WORD)PRandom::Number())
  , expectedSequenceNumber(0)
  , lastSentPacketTime(PTimer::Tick())
  , lastSRTimestamp(0)
  , lastSRReceiveTime(0)
  , lastRRSequenceNumber(0)
  , resequenceOutOfOrderPackets(true)
  , consecutiveOutOfOrderPackets(0)
  , outOfOrderWaitTime(GetDefaultOutOfOrderWaitTime())
  , timeStampOffs(0)
  , oobTimeStampBaseEstablished(false)
  , oobTimeStampOutBase(0)
  , firstPacketSent(0)
  , firstPacketReceived(0)
  , senderReportsReceived(0)
#if OPAL_RTCP_XR
  , m_metrics(NULL)
#endif
  , lastReceivedPayloadType(RTP_DataFrame::IllegalPayloadType)
  , ignorePayloadTypeChanges(true)
  , m_reportTimer(0, 12)  // Seconds
  , m_closeOnBye(false)
  , m_byeSent(false)
  , m_localAddress(0)
  , m_localDataPort(0)
  , m_localControlPort(0)
  , m_remoteAddress(0)
  , m_remoteDataPort(0)
  , m_remoteControlPort(0)
  , m_remoteTransmitAddress(0)
  , m_dataSocket(NULL)
  , m_controlSocket(NULL)
  , m_shutdownRead(false)
  , m_shutdownWrite(false)
  , m_appliedQOS(false)
  , m_remoteBehindNAT(init.m_remoteBehindNAT)
  , m_localHasRestrictedNAT(false)
  , m_noTransmitErrors(0)
{
  ClearStatistics();

  PTRACE_CONTEXT_ID_TO(m_reportTimer);
  m_reportTimer.SetNotifier(PCREATE_NOTIFIER(SendReport));
}


OpalRTPSession::OpalRTPSession(const OpalRTPSession & other)
  : OpalMediaSession(Init(other.m_connection, 0, OpalMediaType(), false))
{
}


OpalRTPSession::~OpalRTPSession()
{
  Close();

#if OPAL_RTCP_XR
  delete m_metrics;
#endif

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
  m_lastReceivedStatisticTimestamp = 0;
}


void OpalRTPSession::AttachTransport(Transport & transport)
{
  transport.DisallowDeleteObjects();

  PObject * channel = transport.RemoveHead();
  m_dataSocket = dynamic_cast<PUDPSocket *>(channel);
  if (m_dataSocket == NULL)
    delete channel;
  else {
    PTRACE_CONTEXT_ID_TO(m_dataSocket);
    m_dataSocket->GetLocalAddress(m_localAddress, m_localDataPort);
  }

  channel = transport.RemoveHead();
  m_controlSocket = dynamic_cast<PUDPSocket *>(channel);
  if (m_controlSocket == NULL)
    delete channel;
  else {
    PTRACE_CONTEXT_ID_TO(m_controlSocket);
    m_controlSocket->GetLocalAddress(m_localAddress, m_localControlPort);
  }

  transport.AllowDeleteObjects();
}


OpalMediaSession::Transport OpalRTPSession::DetachTransport()
{
  Transport temp;

  // Stop jitter buffer before detaching
  m_jitterBuffer.SetNULL();

  if (m_dataSocket != NULL) {
    temp.Append(m_dataSocket);
    m_dataSocket = NULL;
  }

  if (m_controlSocket != NULL) {
    temp.Append(m_controlSocket);
    m_controlSocket = NULL;
  }

  PTRACE_IF(2, temp.IsEmpty(), "RTP\tDetaching transport from closed session.");
  return temp;
}


void OpalRTPSession::SendBYE()
{
  {
    PWaitAndSignal mutex(m_dataMutex);
    if (m_byeSent)
      return;

    m_byeSent = true;
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
  PString s = m_canonicalName;
  s.MakeUnique();
  return s;
}


void OpalRTPSession::SetCanonicalName(const PString & name)
{
  PWaitAndSignal mutex(m_reportMutex);
  m_canonicalName = name;
  m_canonicalName.MakeUnique();
}


PString OpalRTPSession::GetToolName() const
{
  PWaitAndSignal mutex(m_reportMutex);
  PString s = m_toolName;
  s.MakeUnique();
  return s;
}


void OpalRTPSession::SetToolName(const PString & name)
{
  PWaitAndSignal mutex(m_reportMutex);
  m_toolName = name;
  m_toolName.MakeUnique();
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
  PWaitAndSignal mutex(m_dataMutex);

  if (!IsOpen())
    return;

  if (timeUnits > 0)
    m_timeUnits = timeUnits;

  if (minJitterDelay == 0 && maxJitterDelay == 0) {
    PTRACE_IF(4, m_jitterBuffer != NULL, "RTP\tSwitching off jitter buffer " << *m_jitterBuffer);
    // This can block waiting for JB thread to end, signal mutex to avoid deadlock
    m_dataMutex.Signal();
    m_jitterBuffer.SetNULL();
    m_dataMutex.Wait();
  }
  else {
    resequenceOutOfOrderPackets = false;
    if (m_jitterBuffer != NULL) {
      PTRACE(4, "RTP\tSetting jitter buffer time from " << minJitterDelay << " to " << maxJitterDelay);
      m_jitterBuffer->SetDelay(minJitterDelay, maxJitterDelay, packetSize);
    }
    else {
      FlushData();
      m_jitterBuffer = new RTP_JitterBuffer(*this, minJitterDelay, maxJitterDelay, m_timeUnits, packetSize);
      PTRACE(4, "RTP\tCreated RTP jitter buffer " << *m_jitterBuffer);
      m_jitterBuffer->Start();
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
  if (!IsOpen() || m_shutdownRead)
    return false;

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

  PTRACE(m_outOfOrderPackets.empty() ? 2 : 5,
         "RTP\tSession " << m_sessionId << ", ssrc=" << syncSourceIn << ", resequenced "
         << (m_outOfOrderPackets.empty() ? "last" : "next") << " out of order packet " << sequenceNumber);
  return true;
}


void OpalRTPSession::FlushData()
{
  if (!IsOpen())
    return;

  PTimeInterval oldTimeout = m_dataSocket->GetReadTimeout();  
  m_dataSocket->SetReadTimeout(0);

  PINDEX count = 0;
  BYTE buffer[2000];
  while (m_dataSocket->Read(buffer, sizeof(buffer)))
    ++count;

  m_dataSocket->SetReadTimeout(oldTimeout);

  PTRACE_IF(3, count > 0, "RTP\tSession " << m_sessionId << ", flushed "
            << count << " RTP data packets before activating jitter buffer");
}


void OpalRTPSession::SetTxStatisticsInterval(unsigned packets)
{
  txStatisticsInterval = std::max(packets, 2U);
  txStatisticsCount = 0;
  averageSendTimeAccum = 0;
  maximumSendTimeAccum = 0;
  minimumSendTimeAccum = 0xffffffff;
}


void OpalRTPSession::SetRxStatisticsInterval(unsigned packets)
{
  rxStatisticsInterval = std::max(packets, 2U);
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


OpalRTPSession::SendReceiveStatus OpalRTPSession::OnSendData(RTP_DataFrame & frame)
{
  PWaitAndSignal mutex(m_dataMutex);

  PTimeInterval tick = PTimer::Tick();  // Timestamp set now

  frame.SetSequenceNumber(++lastSentSequenceNumber);
  frame.SetSyncSource(syncSourceOut);

  DWORD frameTimestamp = frame.GetTimestamp();

  // special handling for first packet
  if (packetsSent == 0) {

    // establish timestamp offset
    if (oobTimeStampBaseEstablished)  {
      timeStampOffs = oobTimeStampOutBase - frameTimestamp + ((PTimer::Tick() - oobTimeStampBase).GetInterval() * m_timeUnits);
      frameTimestamp += timeStampOffs;
    }
    else {
      oobTimeStampBaseEstablished = true;
      timeStampOffs               = 0;
      oobTimeStampOutBase         = frameTimestamp;
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
           << " ts=" << frameTimestamp
           << " src=" << frame.GetSyncSource()
           << " ccnt=" << frame.GetContribSrcCount()
           << ' ' << GetRemoteAddress());
  }

  else {
    // set timestamp
    frameTimestamp += timeStampOffs;

    // reset OOB timestamp every marker bit
    if (frame.GetMarker()) {
      oobTimeStampOutBase = frameTimestamp;
      oobTimeStampBase    = PTimer::Tick();
    }

    /* For audio we do not do statistics on start of talk burst as that
       could be a substantial time and is not useful, so we only calculate
       when the marker bit os off.

       For video we measure jitter between whole video frames which is
       indicated by the marker bit being on.
    */
    if (m_isAudio  != frame.GetMarker()) {
      DWORD diff = (tick - lastSentPacketTime).GetInterval();

      averageSendTimeAccum += diff;
      if (diff > maximumSendTimeAccum)
        maximumSendTimeAccum = diff;
      if (diff < minimumSendTimeAccum)
        minimumSendTimeAccum = diff;
      txStatisticsCount++;
    }
  }

  frame.SetTimestamp(frameTimestamp);
  lastSentTimestamp = frameTimestamp;
  lastSentPacketTime = tick;

  octetsSent += frame.GetPayloadSize();
  packetsSent++;

  if (frame.GetMarker())
    markerSendCount++;

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
    delete m_metrics; // Should be NULL, but just in case ...
    m_metrics = RTCP_XR_Metrics::Create(frame);
    PTRACE_CONTEXT_ID_TO(m_metrics);
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

      /* For audio we do not do statistics on start of talk burst as that
         could be a substantial time and is not useful, so we only calculate
         when the marker bit os off.

         For video we measure jitter between whole video frames which is
         normally indicated by the marker bit being on, but this is unreliable,
         many endpoints not sending it correctly, so use a change in timestamp
         as most reliable method. */
      if (m_isAudio ? !frame.GetMarker() : (m_lastReceivedStatisticTimestamp != frame.GetTimestamp())) {
        m_lastReceivedStatisticTimestamp = frame.GetTimestamp();

        DWORD diff = (tick - lastReceivedPacketTime).GetInterval();

        averageReceiveTimeAccum += diff;
        if (diff > maximumReceiveTimeAccum)
          maximumReceiveTimeAccum = diff;
        if (diff < minimumReceiveTimeAccum)
          minimumReceiveTimeAccum = diff;
        rxStatisticsCount++;

        // As per RFC3550 Appendix 8
        diff *= GetJitterTimeUnits(); // Convert to timestamp units
        long variance = (long)diff - (long)lastTransitTime;
        lastTransitTime = diff;
        if (variance < 0)
          variance = -variance;
        jitterLevel += variance - ((jitterLevel+(1<<(JitterRoundingGuardBits-1))) >> JitterRoundingGuardBits);
        if (jitterLevel > maximumJitterLevel)
          maximumJitterLevel = jitterLevel;
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
      if (m_metrics != NULL) m_metrics->OnPacketDiscarded();
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

        for (;;) {
          if (m_outOfOrderPackets.empty())
            return e_IgnorePacket;

          frame = m_outOfOrderPackets.back();
          m_outOfOrderPackets.pop_back();

          sequenceNumber = frame.GetSequenceNumber();
          if (sequenceNumber >= expectedSequenceNumber)
            break;

          PTRACE(2, "RTP\tSession " << m_sessionId << ", ssrc=" << syncSourceIn
                 << ", incorrect sequence after re-ordering, got "
                 << sequenceNumber << " expected " << expectedSequenceNumber);
        }

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
      if (m_metrics != NULL) m_metrics->OnPacketLost(dropped);
#endif
    }
  }

  lastReceivedPacketTime = tick;

  octetsReceived += frame.GetPayloadSize();
  packetsReceived++;

  if (m_syncRealTime.IsValid())
    frame.SetAbsoluteTime(m_syncRealTime + PTimeInterval((frame.GetTimestamp() - m_syncTimestamp)/m_timeUnits));

#if OPAL_RTCP_XR
  if (m_metrics != NULL) m_metrics->OnPacketReceived();
#endif

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

  return true;
}


void OpalRTPSession::SendReport(PTimer&, INT)
{
  PWaitAndSignal mutex(m_reportMutex);

  // Have not got anything yet, do nothing
  if (packetsSent == 0 && packetsReceived == 0)
    return;

  RTP_ControlFrame report;
  InsertReportPacket(report);

  // Add the SDES part to compound RTCP packet
  PTRACE(3, "RTP\tSession " << m_sessionId << ", sending SDES: " << m_canonicalName);
  report.StartNewPacket();

  report.SetCount(0); // will be incremented automatically
  report.StartSourceDescription  (syncSourceOut);
  report.AddSourceDescriptionItem(RTP_ControlFrame::e_CNAME, m_canonicalName);
  report.AddSourceDescriptionItem(RTP_ControlFrame::e_TOOL, m_toolName);
  report.EndPacket();
  
#if OPAL_RTCP_XR
  //Generate and send RTCP-XR packet
  if (m_metrics != NULL) m_metrics->InsertExtendedReportPacket(m_sessionId, syncSourceOut, m_jitterBuffer, report);
#endif

  WriteControl(report);
}


#if OPAL_STATISTICS
void OpalRTPSession::GetStatistics(OpalMediaStatistics & statistics, bool receiver) const
{
  statistics.m_startTime         = receiver ? firstPacketReceived     : firstPacketSent;
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


OpalRTPSession::ReceiverReportArray
OpalRTPSession::BuildReceiverReportArray(const RTP_ControlFrame & frame, PINDEX offset)
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
#if OPAL_RTCP_XR
    if (m_metrics != NULL) m_metrics->OnRxSenderReport(rr->lsr, rr->dlsr);
#endif
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
    size_t size = frame.GetPayloadSize(); 
    if ((payload == NULL) || (size == 0) || ((payload + size) > (frame.GetPointer() + frame.GetSize()))){
      /* TODO: 1.shall we test for a maximum size ? Indeed but what's the value ? *
               2. what's the correct exit status ? */
      PTRACE(2, "RTP\tSession " << m_sessionId << ", OnReceiveControl invalid frame");
      break;
    }

    switch (frame.GetPayloadType()) {
      case RTP_ControlFrame::e_SenderReport :
        if (size >= sizeof(PUInt32b)+sizeof(RTP_ControlFrame::SenderReport)+frame.GetCount()*sizeof(RTP_ControlFrame::ReceiverReport)) {
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
          OnRxSenderReport(sender, BuildReceiverReportArray(frame, sizeof(PUInt32b)+sizeof(RTP_ControlFrame::SenderReport)));
        }
        else {
          PTRACE(2, "RTP\tSession " << m_sessionId << ", SenderReport packet truncated");
        }
        break;

      case RTP_ControlFrame::e_ReceiverReport :
        if (size >= sizeof(PUInt32b)+frame.GetCount()*sizeof(RTP_ControlFrame::ReceiverReport))
          OnRxReceiverReport(*(const PUInt32b *)payload, BuildReceiverReportArray(frame, sizeof(PUInt32b)));
        else {
          PTRACE(2, "RTP\tSession " << m_sessionId << ", ReceiverReport packet truncated");
        }
        break;

      case RTP_ControlFrame::e_SourceDescription :
        if (size >= frame.GetCount()*sizeof(RTP_ControlFrame::SourceDescription)) {
          SourceDescriptionArray descriptions;
          const RTP_ControlFrame::SourceDescription * sdes = (const RTP_ControlFrame::SourceDescription *)payload;
          PINDEX srcIdx;
          for (srcIdx = 0; srcIdx < (PINDEX)frame.GetCount(); srcIdx++) {
            descriptions.SetAt(srcIdx, new SourceDescription(sdes->src));
            const RTP_ControlFrame::SourceDescription::Item * item = sdes->item;
            size_t uiSizeCurrent = 0;   /* current size of the items already parsed */
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
          size_t count = frame.GetCount()*4;
    
          if (size > count) {
            if (size >= payload[count] + sizeof(DWORD) /*SSRC*/ + sizeof(unsigned char) /* length */)
              str = PString((const char *)(payload+count+1), payload[count]);
            else {
              PTRACE(2, "RTP\tSession " << m_sessionId << ", Goodbye packet invalid");
            }
          }

          PDWORDArray sources(count);
          for (size_t i = 0; i < count; i++)
            sources[i] = ((const PUInt32b *)payload)[i];
          OnRxGoodbye(sources, str);
        }
        else {
          PTRACE(2, "RTP\tSession " << m_sessionId << ", Goodbye packet truncated");
        }

        if (m_closeOnBye) {
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
        if (size >= sizeof(PUInt32b)+frame.GetCount()*sizeof(RTP_ControlFrame::ExtendedReport))
          OnRxExtendedReport(*(const PUInt32b *)payload, RTCP_XR_Metrics::BuildExtendedReportArray(frame, sizeof(PUInt32b)));
        else {
          PTRACE(2, "RTP\tSession " << m_sessionId << ", ReceiverReport packet truncated");
        }
        break;
#endif

      case RTP_ControlFrame::e_TransportLayerFeedBack :
        switch (frame.GetFbType()) {
          case RTP_ControlFrame::e_TMMBR :
            if (size >= sizeof(RTP_ControlFrame::FbTMMB)) {
              const RTP_ControlFrame::FbTMMB * tmmb = (const RTP_ControlFrame::FbTMMB *)payload;
              PTRACE(4, "RTP\tSession " << m_sessionId << ", received TMMBR " << tmmb->GetBitRate());
              m_connection.ExecuteMediaCommand(OpalMediaFlowControl(tmmb->GetBitRate()), m_sessionId);
            }
            else {
              PTRACE(2, "RTP\tSession " << m_sessionId << ", TMMBR packet truncated");
            }
            break;

          case RTP_ControlFrame::e_TMMBN :
            if (size >= sizeof(RTP_ControlFrame::FbTMMB)) {
            }
            else {
              PTRACE(2, "RTP\tSession " << m_sessionId << ", TMMBN packet truncated");
            }
            break;
        }
        break;

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


void OpalRTPSession::SendFlowControl(unsigned maxBitRate, unsigned overhead, bool notify)
{
  PTRACE(3, "RTP\tSession " << m_sessionId << ", SendFlowControl(" << maxBitRate << ") using TMMBR");
  // Create packet
  RTP_ControlFrame request;
  InsertReportPacket(request);

  request.StartNewPacket();

  request.SetPayloadType(RTP_ControlFrame::e_TransportLayerFeedBack);
  request.SetFbType(notify ? RTP_ControlFrame::e_TMMBN : RTP_ControlFrame::e_TMMBR, sizeof(RTP_ControlFrame::FbTMMB));

  RTP_ControlFrame::FbTMMB * tmmb = (RTP_ControlFrame::FbTMMB *)request.GetPayloadPtr();
  tmmb->requestSSRC = syncSourceIn;

  if (overhead == 0)
    overhead = m_localAddress.GetVersion() == 4 ? (20+8+12) : (40+8+12);

  unsigned exponent = 0;
  unsigned mantissa = maxBitRate;
  while (mantissa >= 0x20000) {
    mantissa >>= 1;
    ++exponent;
  }
  tmmb->bitRateAndOverhead = overhead | (mantissa << 9) | (exponent << 28);

  // Send it
  request.EndPacket();
  WriteControl(request);
}


#if OPAL_VIDEO

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
      request.SetFbType(RTP_ControlFrame::e_PictureLossIndication, sizeof(RTP_ControlFrame::FbFCI));
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

#endif // OPAL_VIDEO


void OpalRTPSession::AddFilter(const FilterNotifier & filter)
{
  // ensures that a filter is added only once
  if (std::find(m_filters.begin(), m_filters.end(), filter) == m_filters.end())
    m_filters.push_back(filter);
}


/////////////////////////////////////////////////////////////////////////////

static void SetMinBufferSize(PUDPSocket & sock, int bufType, int newSize)
{
  int originalSize = 0;
  if (!sock.GetOption(bufType, originalSize)) {
    PTRACE(1, "RTP_UDP\tGetOption(" << sock.GetHandle() << ',' << bufType << ") failed: " << sock.GetErrorText());
    return;
  }

  // Already big enough
  if (originalSize >= newSize)
    return;

  for (; newSize >= 1024; newSize -= newSize/10) {
    // Set to new size
    if (!sock.SetOption(bufType, newSize)) {
      PTRACE(1, "RTP_UDP\tSetOption(" << sock.GetHandle() << ',' << bufType << ',' << newSize << ") failed: " << sock.GetErrorText());
      continue;
    }

    // As some stacks lie about setting the buffer size, we double check.
    int adjustedSize;
    if (!sock.GetOption(bufType, adjustedSize)) {
      PTRACE(1, "RTP_UDP\tGetOption(" << sock.GetHandle() << ',' << bufType << ") failed: " << sock.GetErrorText());
      return;
    }

    if (adjustedSize >= newSize) {
      PTRACE(4, "RTP_UDP\tSetOption(" << sock.GetHandle() << ',' << bufType << ',' << newSize << ") succeeded.");
      return;
    }

    if (adjustedSize > originalSize) {
      PTRACE(4, "RTP_UDP\tSetOption(" << sock.GetHandle() << ',' << bufType << ',' << newSize << ") clamped to maximum " << adjustedSize);
      return;
    }

    PTRACE(2, "RTP_UDP\tSetOption(" << sock.GetHandle() << ',' << bufType << ',' << newSize << ") failed, even though it said it succeeded!");
  }
}


OpalTransportAddress OpalRTPSession::GetLocalAddress(bool isMediaAddress) const
{
  WORD port = isMediaAddress ? m_localDataPort : m_localControlPort;
  if (m_localAddress.IsValid() && port != 0)
    return OpalTransportAddress(m_localAddress, port, OpalTransportAddress::UdpPrefix());
  else
    return OpalTransportAddress();
}


OpalTransportAddress OpalRTPSession::GetRemoteAddress(bool isMediaAddress) const
{
  WORD port = isMediaAddress ? m_remoteDataPort : m_remoteControlPort;
  if (m_remoteAddress.IsValid() && port != 0)
    return OpalTransportAddress(m_remoteAddress, port, OpalTransportAddress::UdpPrefix());
  else
    return OpalTransportAddress();
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
  if (m_controlSocket != NULL)
    m_controlSocket->SetSendAddress(addr, m_remoteControlPort);
  if (m_dataSocket != NULL)
    m_dataSocket->SetSendAddress(addr, m_remoteDataPort);
  m_appliedQOS = true;
}

#if P_QOS

bool OpalRTPSession::ModifyQOS(RTP_QOS * rtpqos)
{
  bool retval = false;

  if (rtpqos == NULL)
    return retval;

  if (m_controlSocket != NULL)
    retval = m_controlSocket->ModifyQoSSpec(&(rtpqos->ctrlQoS));
    
  if (m_dataSocket != NULL)
    retval &= m_dataSocket->ModifyQoSSpec(&(rtpqos->dataQoS));

  m_appliedQOS = false;
  return retval;
}

#endif


void OpalRTPSession::SetExternalTransport(const OpalTransportAddressArray & transports)
{
  if (transports.IsEmpty())
    return;

  PWaitAndSignal mutex(m_dataMutex);

  delete m_dataSocket;
  delete m_controlSocket;
  m_dataSocket = NULL;
  m_controlSocket = NULL;

  transports[0].GetIpAndPort(m_localAddress, m_localDataPort);
  if (transports.GetSize() > 1)
    transports[1].GetIpAndPort(m_localAddress, m_localControlPort);
  else
    m_localControlPort = (WORD)(m_localDataPort+1);

  OpalMediaSession::SetExternalTransport(transports);
}


bool OpalRTPSession::Open(const PString & localInterface, const OpalTransportAddress & remoteAddress, bool mediaAddress)
{
  PWaitAndSignal mutex(m_dataMutex);

  if (IsExternalTransport())
    return true;

  if (m_dataSocket != NULL && m_controlSocket != NULL)
    return true;

  if (!OpalMediaSession::Open(localInterface, remoteAddress, mediaAddress))
    return false;

  m_firstControl = true;
  m_byeSent = false;
  m_shutdownRead = false;
  m_shutdownWrite = false;

  OpalManager & manager = m_connection.GetEndPoint().GetManager();

#if P_NAT
  PNatMethod * natMethod = m_connection.GetNatMethod(m_remoteAddress);
  if (natMethod != NULL) {
    PTRACE(4, "RTP\tNAT Method " << natMethod->GetName() << " selected for call.");
    natMethod->CreateSocketPairAsync(m_connection.GetToken());
  }
#endif

  WORD firstPort = manager.GetRtpIpPortPair();

  delete m_dataSocket;
  delete m_controlSocket;
  m_dataSocket = NULL;
  m_controlSocket = NULL;

  m_localDataPort    = firstPort;
  m_localControlPort = (WORD)(firstPort + 1);

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
        m_localHasRestrictedNAT = true;
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
        if (natMethod->GetSocketPairAsync(m_connection.GetToken(), m_dataSocket, m_controlSocket, bindingAddress, natInfo)) {
          PTRACE(4, "RTP\tSession " << m_sessionId << ", " << natMethod->GetName() << " created STUN RTP/RTCP socket pair.");
          m_dataSocket->GetLocalAddress(bindingAddress, m_localDataPort);
          m_controlSocket->GetLocalAddress(bindingAddress, m_localControlPort);
        }
        else {
          PTRACE(2, "RTP\tSession " << m_sessionId << ", " << natMethod->GetName()
                  << " could not create STUN RTP/RTCP socket pair; trying to create individual sockets.");
          if (natMethod->CreateSocket(m_dataSocket, bindingAddress) && natMethod->CreateSocket(m_controlSocket, bindingAddress)) {
            m_dataSocket->GetLocalAddress(bindingAddress, m_localDataPort);
            m_controlSocket->GetLocalAddress(bindingAddress, m_localControlPort);
          }
          else {
            delete m_dataSocket;
            delete m_controlSocket;
            m_dataSocket = NULL;
            m_controlSocket = NULL;
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

  if (m_dataSocket == NULL || m_controlSocket == NULL) {
    m_dataSocket = new PUDPSocket();
    m_controlSocket = new PUDPSocket();
    while (!   m_dataSocket->Listen(bindingAddress, 1, m_localDataPort) ||
           !m_controlSocket->Listen(bindingAddress, 1, m_localControlPort)) {
      m_dataSocket->Close();
      m_controlSocket->Close();

      m_localDataPort = manager.GetRtpIpPortPair();
      if (m_localDataPort == firstPort) {
        PTRACE(1, "RTPCon\tNo ports available for RTP session " << m_sessionId << ","
                  " base=" << manager.GetRtpIpPortBase() << ","
                  " max=" << manager.GetRtpIpPortMax() << ","
                  " bind=" << bindingAddress << ","
                  " for " << m_connection);
        return false; // Used up all the available ports!
      }
      m_localControlPort = (WORD)(m_localDataPort + 1);
    }
  }

  PTRACE_CONTEXT_ID_TO(m_dataSocket);
  PTRACE_CONTEXT_ID_TO(m_controlSocket);

#ifndef __BEOS__
  // Set the IP Type Of Service field for prioritisation of media UDP packets
  // through some Cisco routers and Linux boxes
  if (!m_dataSocket->SetOption(IP_TOS, manager.GetMediaTypeOfService(m_mediaType), IPPROTO_IP)) {
    PTRACE(1, "RTP_UDP\tSession " << m_sessionId << ", could not set TOS field in IP header: " << m_dataSocket->GetErrorText());
  }

  // Increase internal buffer size on media UDP sockets
  SetMinBufferSize(*m_dataSocket,    SO_RCVBUF, m_isAudio ? RTP_AUDIO_RX_BUFFER_SIZE : RTP_VIDEO_RX_BUFFER_SIZE);
  SetMinBufferSize(*m_dataSocket,    SO_SNDBUF, RTP_DATA_TX_BUFFER_SIZE);
  SetMinBufferSize(*m_controlSocket, SO_RCVBUF, RTP_CTRL_BUFFER_SIZE);
  SetMinBufferSize(*m_controlSocket, SO_SNDBUF, RTP_CTRL_BUFFER_SIZE);
#endif

  if (m_canonicalName.Find('@') == P_MAX_INDEX)
    m_canonicalName += '@' + GetLocalHostName();

  m_dataSocket->GetLocalAddress(m_localAddress);
  manager.TranslateIPAddress(m_localAddress, m_remoteAddress);

  PTRACE(3, "RTP_UDP\tSession " << m_sessionId << " opened: "
         << m_localAddress << ':' << m_localDataPort << '-' << m_localControlPort
         << " ssrc=" << syncSourceOut);

  OpalRTPEndPoint * ep = dynamic_cast<OpalRTPEndPoint *>(&m_connection.GetEndPoint());
  if (PAssert(ep != NULL, "RTP createed by non OpalRTPEndPoint derived class"))
    ep->SetConnectionByRtpLocalPort(this, &m_connection);

  return true;
}


bool OpalRTPSession::IsOpen() const
{
  if (m_isExternalTransport)
    return m_localAddress.IsValid() && m_localDataPort != 0 &&
           m_remoteAddress.IsValid() && m_remoteDataPort != 0;;

  return m_dataSocket != NULL && m_dataSocket->IsOpen() &&
         m_controlSocket != NULL && m_controlSocket->IsOpen();
}


bool OpalRTPSession::Close()
{
  bool ok = Shutdown(true) | Shutdown(false);

  m_reportTimer.Stop(true);

  OpalRTPEndPoint * ep = dynamic_cast<OpalRTPEndPoint *>(&m_connection.GetEndPoint());
  if (ep != NULL)
    ep->SetConnectionByRtpLocalPort(this, NULL);

  // We need to do this to make sure that the sockets are not
  // deleted before select decides there is no more data coming
  // over them and exits the reading thread.
  SetJitterBufferSize(0, 0);

  PWaitAndSignal mutex(m_dataMutex);

  delete m_dataSocket;
  m_dataSocket = NULL;

  delete m_controlSocket;
  m_controlSocket = NULL;

  m_localAddress = PIPSocket::GetInvalidAddress();
  m_localDataPort = m_localControlPort = 0;
  m_remoteAddress = PIPSocket::GetInvalidAddress();
  m_remoteDataPort = m_remoteControlPort = 0;

  return ok;
}


bool OpalRTPSession::Shutdown(bool reading)
{
  if (reading) {
    {
      PWaitAndSignal mutex(m_dataMutex);

      if (m_shutdownRead) {
        PTRACE(4, "RTP_UDP\tSession " << m_sessionId << ", read already shut down .");
        return false;
      }

      PTRACE(3, "RTP_UDP\tSession " << m_sessionId << ", shutting down read.");

      syncSourceIn = 0;
      m_shutdownRead = true;

      if (m_dataSocket != NULL && m_controlSocket != NULL) {
        PIPSocketAddressAndPort addrAndPort;
        m_controlSocket->PUDPSocket::InternalGetLocalAddress(addrAndPort);
        if (!addrAndPort.IsValid())
          addrAndPort.SetAddress(PIPSocket::GetHostName());
        if (!m_dataSocket->WriteTo("", 1, addrAndPort)) {
          PTRACE(1, "RTP_UDP\tSession " << m_sessionId << ", could not write to unblock read socket: "
                 << m_dataSocket->GetErrorText(PChannel::LastReadError));
        }
      }
    }

    SetJitterBufferSize(0, 0); // Kill jitter buffer too, but outside mutex
  }
  else {
    if (m_shutdownWrite) {
      PTRACE(4, "RTP_UDP\tSession " << m_sessionId << ", write already shut down .");
      return false;
    }

    PTRACE(3, "RTP_UDP\tSession " << m_sessionId << ", shutting down write.");
    m_shutdownWrite = true;
  }

  // If shutting down both, no reporting any more
  if (m_shutdownRead && m_shutdownWrite)
    m_reportTimer.Stop(false);

  return true;
}


void OpalRTPSession::Restart(bool reading)
{
  PWaitAndSignal mutex(m_dataMutex);

  if (reading) {
    if (!m_shutdownRead)
      return;
    m_shutdownRead = false;
  }
  else {
    if (!m_shutdownWrite)
      return;
    m_shutdownWrite = false;
  }

  m_noTransmitErrors = 0;
  m_reportTimer.RunContinuous(m_reportTimer.GetResetTime());

  PTRACE(3, "RTP_UDP\tSession " << m_sessionId << " reopened for " << (reading ? "reading" : "writing"));
}


PString OpalRTPSession::GetLocalHostName()
{
  return PIPSocket::GetHostName();
}


bool OpalRTPSession::SetRemoteAddress(const OpalTransportAddress & remoteAddress, bool isMediaAddress)
{
  if (m_remoteBehindNAT) {
    PTRACE(2, "RTP_UDP\tSession " << m_sessionId << ", ignoring remote socket info as remote is behind NAT");
    return true;
  }

  PIPSocket::Address address;
  WORD port = isMediaAddress ? m_remoteDataPort : m_remoteControlPort;
  if (!remoteAddress.GetIpAndPort(address, port))
    return false;

  PTRACE(3, "RTP_UDP\tSession " << m_sessionId << ", Set remote "
         << (isMediaAddress ? "data" : "control") << " address, "
            "new=" << address << ':' << port << ", "
            "old=" << m_remoteAddress << ':' << m_remoteDataPort << '-' << m_remoteControlPort << ", "
            "local=" << m_localAddress << ':' << m_localDataPort << '-' << m_localControlPort);

  if (m_localAddress == address && m_remoteAddress == address && (isMediaAddress ? m_localDataPort : m_localControlPort) == port)
    return true;
  
  m_remoteAddress = address;
  
  allowOneSyncSourceChange = true;
  allowRemoteTransmitAddressChange = true;
  allowSequenceChange = packetsReceived != 0;

  if (isMediaAddress) {
    m_remoteDataPort = port;
    if (m_remoteControlPort == 0 || allowRemoteTransmitAddressChange)
      m_remoteControlPort = (WORD)(port + 1);
  }
  else {
    m_remoteControlPort = port;
    if (m_remoteDataPort == 0 || allowRemoteTransmitAddressChange)
      m_remoteDataPort = (WORD)(port - 1);
  }

  if (!m_appliedQOS)
      ApplyQOS(m_remoteAddress);

  if (m_localHasRestrictedNAT) {
    // If have Port Restricted NAT on local host then send a datagram
    // to remote to open up the port in the firewall for return data.
    static const BYTE dummy[1] = { 0 };
    WriteDataOrControlPDU(dummy, sizeof(dummy), true);
    WriteDataOrControlPDU(dummy, sizeof(dummy), false);
    PTRACE(2, "RTP_UDP\tSession " << m_sessionId << ", sending empty datagrams to open local restricted NAT");
  }

  return true;
}


bool OpalRTPSession::InternalReadData(RTP_DataFrame & frame)
{
  SendReceiveStatus receiveStatus = e_IgnorePacket;
  while (receiveStatus == e_IgnorePacket) {
    if (m_shutdownRead || PAssertNULL(m_dataSocket) == NULL || PAssertNULL(m_controlSocket) == NULL)
      return false;

    int selectStatus = WaitForPDU(*m_dataSocket, *m_controlSocket, m_maxNoReceiveTime);
    if (m_shutdownRead)
      return false;

    if (selectStatus > 0) {
      PTRACE(1, "RTP_UDP\tSession " << m_sessionId << ", Select error: "
              << PChannel::GetErrorText((PChannel::Errors)selectStatus));
      return false;
    }

    if (selectStatus == 0)
      receiveStatus = OnReadTimeout(frame);

    if ((-selectStatus & 2) != 0) {
      if (ReadControlPDU() == e_AbortTransport)
        return false;
    }

    if ((-selectStatus & 1) != 0)
      receiveStatus = ReadDataPDU(frame);
  }

  return receiveStatus == e_ProcessPacket;
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
  PUDPSocket & socket = *(fromDataChannel ? m_dataSocket : m_controlSocket);
  PIPSocket::Address addr;
  WORD port;

  if (socket.ReadFrom(framePtr, frameSize, addr, port)) {
    // If remote address never set from higher levels, then try and figure
    // it out from the first packet received.
    if (!m_remoteAddress.IsValid()) {
      m_remoteAddress = addr;
      PTRACE(4, "RTP\tSession " << m_sessionId << ", set remote address from first "
             << channelName << " PDU from " << addr << ':' << port);
    }
    if (fromDataChannel) {
      if (m_remoteDataPort == 0)
        m_remoteDataPort = port;
    }
    else {
      if (m_remoteControlPort == 0)
        m_remoteControlPort = port;
    }

    if (!m_remoteTransmitAddress.IsValid())
      m_remoteTransmitAddress = addr;
    else if (allowRemoteTransmitAddressChange && m_remoteAddress == addr) {
      m_remoteTransmitAddress = addr;
      allowRemoteTransmitAddressChange = false;
    }
    else if (m_remoteTransmitAddress != addr && !allowRemoteTransmitAddressChange) {
      PTRACE(2, "RTP_UDP\tSession " << m_sessionId << ", "
             << channelName << " PDU from incorrect host, "
                " is " << addr << " should be " << m_remoteTransmitAddress);
      return e_IgnorePacket;
    }

    if (m_remoteAddress.IsValid() && !m_appliedQOS) 
      ApplyQOS(m_remoteAddress);

    m_noTransmitErrors = 0;

    return e_ProcessPacket;
  }

  switch (socket.GetErrorNumber(PChannel::LastReadError)) {
    case ECONNRESET :
    case ECONNREFUSED :
      PTRACE(2, "RTP_UDP\tSession " << m_sessionId << ", " << channelName << " port on remote not ready.");
      if (++m_noTransmitErrors == 1)
        m_noTransmitTimer = m_maxNoTransmitTime;
      else {
        if (m_noTransmitErrors < 10 || m_noTransmitTimer.IsRunning())
          return e_IgnorePacket;
        PTRACE(2, "RTP_UDP\tSession " << m_sessionId << ", " << channelName << ' '
               << m_maxNoTransmitTime << " seconds of transmit fails - informing connection");
        if (m_connection.OnMediaFailed(m_sessionId, false))
          return e_AbortTransport;
      }
      return e_IgnorePacket;

    case EMSGSIZE :
      PTRACE(2, "RTP_UDP\tSession " << m_sessionId << ", " << channelName
             << " read packet too large for buffer of " << frameSize << " bytes.");
      return e_IgnorePacket;

    case EAGAIN :
      PTRACE(4, "RTP_UDP\tSession " << m_sessionId << ", " << channelName
             << " read packet interrupted.");
      // Shouldn't happen, but it does.
      return e_IgnorePacket;

    default:
      PTRACE(1, "RTP_UDP\tSession " << m_sessionId << ", " << channelName
             << " read error (" << socket.GetErrorNumber(PChannel::LastReadError) << "): "
             << socket.GetErrorText(PChannel::LastReadError));
      return e_AbortTransport;
  }
}


OpalRTPSession::SendReceiveStatus OpalRTPSession::ReadDataPDU(RTP_DataFrame & frame)
{
  SendReceiveStatus status = ReadDataOrControlPDU(frame.GetPointer(), frame.GetSize(), true);
  if (status != e_ProcessPacket)
    return status;

  // Check received PDU is big enough
  if (frame.SetPacketSize(m_dataSocket->GetLastReadCount()))
    return OnReceiveData(frame);

  return e_IgnorePacket;
}


OpalRTPSession::SendReceiveStatus OpalRTPSession::OnReadTimeout(RTP_DataFrame & /*frame*/)
{
  if (m_connection.OnMediaFailed(m_sessionId, true))
    return e_AbortTransport;

  return e_IgnorePacket;
}


OpalRTPSession::SendReceiveStatus OpalRTPSession::ReadControlPDU()
{
  RTP_ControlFrame frame(2048);

  SendReceiveStatus status = ReadDataOrControlPDU(frame.GetPointer(), frame.GetSize(), false);
  if (status != e_ProcessPacket)
    return status;

  PINDEX pduSize = m_controlSocket->GetLastReadCount();
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
  PWaitAndSignal m(m_dataMutex);

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
  if (!IsOpen() || m_shutdownWrite) {
    PTRACE(3, "RTP_UDP\tSession " << m_sessionId << ", write shutdown.");
    return false;
  }

  // Trying to send a PDU before we are set up!
  if (!m_remoteAddress.IsValid() || m_remoteDataPort == 0)
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
  if (!m_remoteAddress.IsValid() || m_remoteControlPort == 0 || m_controlSocket == NULL)
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
  PUDPSocket & socket = *(toDataChannel ? m_dataSocket : m_controlSocket);
  WORD port = toDataChannel ? m_remoteDataPort : m_remoteControlPort;
  int retry = 0;

  while (!socket.WriteTo(framePtr, frameSize, m_remoteAddress, port)) {
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
