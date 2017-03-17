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

#include <opal_config.h>

#include <rtp/rtp_session.h>

#include <opal/endpoint.h>
#include <sdp/ice.h>
#include <rtp/rtpep.h>
#include <rtp/rtpconn.h>
#include <rtp/rtp_stream.h>
#include <rtp/metrics.h>
#include <codec/vidcodec.h>

#include <ptclib/random.h>
#include <ptclib/cypher.h>

#include <algorithm>


static const uint16_t SequenceReorderThreshold = (1<<16)-100;  // As per RFC3550 RTP_SEQ_MOD - MAX_MISORDER
static const uint16_t SequenceRestartThreshold = 3000;         // As per RFC3550 MAX_DROPOUT


enum { JitterRoundingGuardBits = 4 };

#if PTRACING
ostream & operator<<(ostream & strm, OpalRTPSession::Direction dir)
{
  return strm << (dir == OpalRTPSession::e_Receiver ? "receiver" : "sender");
}
#endif

#define PTraceModule() "RTP"

#define new PNEW

/////////////////////////////////////////////////////////////////////////////

const PCaselessString & OpalRTPSession::RTP_AVP () { static const PConstCaselessString s("RTP/AVP" ); return s; }
const PCaselessString & OpalRTPSession::RTP_AVPF() { static const PConstCaselessString s("RTP/AVPF"); return s; }

PFACTORY_CREATE(OpalMediaSessionFactory, OpalRTPSession, OpalRTPSession::RTP_AVP());
PFACTORY_SYNONYM(OpalMediaSessionFactory, OpalRTPSession, AVPF, OpalRTPSession::RTP_AVPF());


#define DEFAULT_OUT_OF_ORDER_WAIT_TIME_AUDIO 40
#define DEFAULT_OUT_OF_ORDER_WAIT_TIME_VIDEO 100

#if P_CONFIG_FILE
static PTimeInterval GetDefaultOutOfOrderWaitTime(bool audio)
{
  static PTimeInterval dooowta(PConfig(PConfig::Environment).GetInteger("OPAL_RTP_OUT_OF_ORDER_TIME_AUDIO", DEFAULT_OUT_OF_ORDER_WAIT_TIME_AUDIO));
  static PTimeInterval dooowtv(PConfig(PConfig::Environment).GetInteger("OPAL_RTP_OUT_OF_ORDER_TIME_VIDEO",
                               PConfig(PConfig::Environment).GetInteger("OPAL_RTP_OUT_OF_ORDER_TIME",       DEFAULT_OUT_OF_ORDER_WAIT_TIME_VIDEO)));
  return audio ? dooowta : dooowtv;
}
#else
#define GetDefaultOutOfOrderWaitTime(audio) ((audio) ? DEFAULT_OUT_OF_ORDER_WAIT_TIME_AUDIO : DEFAULT_OUT_OF_ORDER_WAIT_TIME_VIDEO)
#endif


OpalRTPSession::OpalRTPSession(const Init & init)
  : OpalMediaSession(init)
  , m_endpoint(dynamic_cast<OpalRTPEndPoint &>(m_connection.GetEndPoint()))
  , m_manager(m_endpoint.GetManager())
  , m_singlePortRx(false)
  , m_singlePortTx(false)
  , m_reducedSizeRTCP(false)
  , m_isAudio(init.m_mediaType == OpalMediaType::Audio())
  , m_timeUnits(m_isAudio ? 8 : 90)
  , m_toolName(PProcess::Current().GetName())
  , m_absSendTimeHdrExtId(UINT_MAX)
  , m_absSendTimeHighBits(0)
  , m_absSendTimeAllBits(0)
  , m_transportWideSeqNumHdrExtId(UINT_MAX)
  , m_allowAnySyncSource(true)
  , m_staleReceiverTimeout(m_manager.GetStaleReceiverTimeout())
  , m_maxOutOfOrderPackets(20)
  , m_waitOutOfOrderTime(GetDefaultOutOfOrderWaitTime(m_isAudio))
  , m_txStatisticsInterval(100)
  , m_rxStatisticsInterval(100)
  , m_feedback(OpalMediaFormat::e_NoRTCPFb)
  , m_jitterBuffer(NULL)
#if OPAL_RTP_FEC
  , m_redundencyPayloadType(RTP_DataFrame::IllegalPayloadType)
  , m_ulpFecPayloadType(RTP_DataFrame::IllegalPayloadType)
  , m_ulpFecSendLevel(2)
#endif
  , m_dummySyncSource(*this, 0, e_Receiver, "-")
  , m_rtcpPacketsSent(0)
  , m_rtcpPacketsReceived(0)
  , m_roundTripTime(-1)
  , m_reportTimer(0, 4)  // Seconds
  , m_qos(m_manager.GetMediaQoS(init.m_mediaType))
  , m_packetOverhead(0)
  , m_remoteControlPort(0)
  , m_sendEstablished(true)
  , m_dataNotifier(PCREATE_NOTIFIER(OnRxDataPacket))
  , m_controlNotifier(PCREATE_NOTIFIER(OnRxControlPacket))
{
  PTRACE_CONTEXT_ID_TO(m_reportTimer);
  m_reportTimer.SetNotifier(PCREATE_NOTIFIER(TimedSendReport), "RTP-Report");
  m_reportTimer.Stop();
}


OpalRTPSession::OpalRTPSession(const OpalRTPSession & other)
  : OpalMediaSession(Init(other.m_connection, 0, OpalMediaType(), false))
  , m_endpoint(other.m_endpoint)
  , m_manager(other.m_manager)
  , m_dummySyncSource(*this, 0, e_Receiver, NULL)
{
}


OpalRTPSession::~OpalRTPSession()
{
  Close();

  for (SyncSourceMap::iterator it = m_SSRC.begin(); it != m_SSRC.end(); ++it)
    delete it->second;
}


RTP_SyncSourceId OpalRTPSession::AddSyncSource(RTP_SyncSourceId id, Direction dir, const char * cname)
{
  PSafeLockReadWrite lock(*this);
  if (!lock.IsLocked())
    return 0;

  if (id == 0) {
    do {
      id = PRandom::Number();
    } while (id < 4 || m_SSRC.find(id) != m_SSRC.end());
  }
  else {
    SyncSourceMap::iterator it = m_SSRC.find(id);
    if (it != m_SSRC.end()) {
      if (cname == NULL || it->second->m_canonicalName == cname)
        return id;
      PTRACE(2, *this << "could not add SSRC=" << RTP_TRACE_SRC(id) << ","
                " probable clash with " << it->second->m_direction << ","
                " cname=" << it->second->m_canonicalName);
      return 0;
    }
  }

  m_SSRC[id] = CreateSyncSource(id, dir, cname);
  return id;
}


bool OpalRTPSession::RemoveSyncSource(RTP_SyncSourceId ssrc)
{
  PSafeLockReadWrite lock(*this);
  if (!lock.IsLocked())
    return false;

  SyncSourceMap::iterator it = m_SSRC.find(ssrc);
  if (it == m_SSRC.end())
    return false;

  if (it->second->m_direction == e_Sender)
    it->second->SendBYE();

  PTRACE(3, *this << "removed " << it->second->m_direction << " SSRC=" << RTP_TRACE_SRC(ssrc));
  delete it->second;
  m_SSRC.erase(it);
  return true;
}


OpalRTPSession::SyncSource * OpalRTPSession::UseSyncSource(RTP_SyncSourceId ssrc, Direction dir, bool force)
{
  SyncSourceMap::iterator it = m_SSRC.find(ssrc);
  if (it != m_SSRC.end())
    return it->second;

  if ((force || m_allowAnySyncSource) && AddSyncSource(ssrc, dir) == ssrc) {
    PTRACE(4, *this << "automatically added " << GetMediaType() << ' ' << dir << " SSRC=" << RTP_TRACE_SRC(ssrc));
    return m_SSRC.find(ssrc)->second;
  }

#if PTRACING
  const unsigned Level = IsGroupMember(GetBundleGroupId()) ? 6 : 2;
  if (PTrace::CanTrace(Level)) {
    ostream & trace = PTRACE_BEGIN(Level);
    trace << *this << "packet from SSRC=" << RTP_TRACE_SRC(ssrc) << " ignored";
    SyncSource * existing;
    if (GetSyncSource(0, e_Receiver, existing))
      trace << ", expecting SSRC=" << RTP_TRACE_SRC(existing->m_sourceIdentifier);
    trace << PTrace::End;
  }
#endif

  return NULL;
}


OpalRTPSession::SyncSource * OpalRTPSession::CreateSyncSource(RTP_SyncSourceId id, Direction dir, const char * cname)
{
  return new SyncSource(*this, id, dir, cname);
}


RTP_SyncSourceArray OpalRTPSession::GetSyncSources(Direction dir) const
{
  RTP_SyncSourceArray ssrcs;

  PSafeLockReadOnly lock(*this);
  if (lock.IsLocked()) {
    for (SyncSourceMap::const_iterator it = m_SSRC.begin(); it != m_SSRC.end(); ++it) {
      if (it->second->m_direction == dir)
        ssrcs.push_back(it->first);
    }
  }

  return ssrcs;
}


const OpalRTPSession::SyncSource & OpalRTPSession::GetSyncSource(RTP_SyncSourceId ssrc, Direction dir) const
{
  PSafeLockReadOnly lock(*this);
  SyncSource * info;
  return GetSyncSource(ssrc, dir, info) ? *info : m_dummySyncSource;
}


bool OpalRTPSession::GetSyncSource(RTP_SyncSourceId ssrc, Direction dir, SyncSource * & info) const
{
  SyncSourceMap::const_iterator it;
  if (ssrc != 0) {
    if ((it = m_SSRC.find(ssrc)) == m_SSRC.end()) {
      PTRACE(3, *this << "cannot find info for " << dir << " SSRC=" << RTP_TRACE_SRC(ssrc));
      return false;
    }
  }
  else {
    for (it = m_SSRC.begin(); it != m_SSRC.end(); ++it) {
      if (it->second->m_direction == dir && it->second->m_packets > 0)
        break;
    }
    if (it == m_SSRC.end()) {
      for (it = m_SSRC.begin(); it != m_SSRC.end(); ++it) {
        if (it->second->m_direction == dir)
          break;
      }
      if (it == m_SSRC.end()) {
        PTRACE(3, *this << "cannot find info for any " << dir);
        return false;
      }
    }
  }

  info = const_cast<SyncSource *>(it->second);
  return true;
}


OpalRTPSession::SyncSource::SyncSource(OpalRTPSession & session, RTP_SyncSourceId id, Direction dir, const char * cname)
  : m_session(session)
  , m_direction(dir)
  , m_sourceIdentifier(id)
  , m_loopbackIdentifier(0)
  , m_canonicalName(cname)
  , m_notifiers(m_session.m_notifiers)
  , m_lastSequenceNumber(0)
  , m_extendedSequenceNumber(0)
  , m_lastFIRSequenceNumber(0)
  , m_lastTSTOSequenceNumber(0)
  , m_consecutiveOutOfOrderPackets(0)
  , m_nextOutOfOrderPacket(0)
  , m_lateOutOfOrderAdaptCount(0)
  , m_lateOutOfOrderAdaptMax(2)
  , m_lateOutOfOrderAdaptBoost(10)
  , m_lateOutOfOrderAdaptPeriod(0, 1)
  , m_reportTimestamp(0)
  , m_reportAbsoluteTime(0)
  , m_synthesizeAbsTime(true)
  , m_firstPacketTime(0)
  , m_packets(0)
  , m_octets(0)
  , m_senderReports(0)
  , m_NACKs(0)
  , m_packetsLost(dir == e_Sender ? -1 : 0)
  , m_packetsOutOfOrder(0)
  , m_lateOutOfOrder(dir == e_Sender ? -1 : 0)
  , m_averagePacketTime(-1)
  , m_maximumPacketTime(-1)
  , m_minimumPacketTime(-1)
  , m_currentjitter(-1)
  , m_maximumJitter(-1)
  , m_markerCount(0)
  , m_lastPacketTimestamp(0)
  , m_lastPacketAbsTime(0)
  , m_averageTimeAccum(0)
  , m_maximumTimeAccum(0)
  , m_minimumTimeAccum(0)
  , m_jitterAccum(0)
  , m_lastJitterTimestamp(0)
  , m_packetsLostSinceLastRR(0)
  , m_lastRRSequenceNumber(0)
  , m_ntpPassThrough(0)
  , m_lastSenderReportTime(0)
  , m_referenceReportTime(0)
  , m_referenceReportNTP(0)
  , m_statisticsCount(0)
#if OPAL_RTCP_XR
  , m_metrics(NULL)
#endif
  , m_jitterBuffer(NULL)
{
  if (m_canonicalName.IsEmpty()) {
    /* CNAME is no longer just a username@host string, for security!
       But RFC 6222 hopelessly complicated, while not exactly the same, just
       using the base64 of a GUID is very similar. It will do. */
    m_canonicalName = PBase64::Encode(PGloballyUniqueID());
    m_canonicalName.Delete(m_canonicalName.GetLength()-2, 2); // Chop off == at end.
  }
}

OpalRTPSession::SyncSource::~SyncSource()
{
#if PTRACING
  unsigned Level = 3;
  if (m_packets > 0 && PTrace::CanTrace(3)) {
    PTime now;
    int duration = (now - m_firstPacketTime).GetSeconds();
    if (duration == 0)
      duration = 1;
    ostream & trace = PTRACE_BEGIN(Level, &m_session, PTraceModule());
    trace << m_session
          << m_direction << " statistics:\n"
               "    Sync Source ID       = " << RTP_TRACE_SRC(m_sourceIdentifier) << "\n"
               "    first packet         = " << m_firstPacketTime << "\n"
               "    last packet          = " << m_lastPacketAbsTime << "\n"
               "    total packets        = " << m_packets << "\n"
               "    total octets         = " << m_octets << "\n"
               "    bitRateSent          = " << (8 * m_octets / duration) << "\n"
               "    lostPackets          = " << m_packetsLost << '\n';
    if (m_direction == e_Receiver) {
      OpalJitterBuffer * jb = GetJitterBuffer();
      if (jb != NULL)
        trace << "    packets too late     = " << jb->GetPacketsTooLate() << '\n';
      trace << "    restored out of order= " << m_packetsOutOfOrder << "\n"
               "    late out of order    = " << m_lateOutOfOrder << '\n';
    }
    trace <<   "    average time         = " << m_averagePacketTime << "\n"
               "    maximum time         = " << m_maximumPacketTime << "\n"
               "    minimum time         = " << m_minimumPacketTime << "\n"
               "    last jitter          = " << m_currentjitter << "\n"
               "    max jitter           = " << m_maximumJitter
          << PTrace::End;
  }
#endif

#if OPAL_RTCP_XR
  delete m_metrics;
#endif
}


void OpalRTPSession::SyncSource::CalculateStatistics(const RTP_DataFrame & frame)
{
  m_octets += frame.GetPayloadSize();
  m_packets++;

  if (frame.GetMarker())
    ++m_markerCount;

  RTP_Timestamp lastTimestamp = m_lastPacketTimestamp;

  PTime previousPacketNetTime = m_lastPacketNetTime;
  m_lastPacketNetTime.SetCurrentTime();

  m_lastPacketAbsTime = frame.GetAbsoluteTime();
  if (!m_lastPacketAbsTime.IsValid())
    m_lastPacketAbsTime.SetCurrentTime();

  m_lastPacketTimestamp = frame.GetTimestamp();

  /* For audio we do not do statistics on start of talk burst as that
      could be a substantial time and is not useful, so we only calculate
      when the marker bit os off.

      For video we measure jitter between whole video frames which is
      normally indicated by the marker bit being on, but this is unreliable,
      many endpoints not sending it correctly, so use a change in timestamp
      as most reliable method. */
  if (m_session.IsAudio() ? frame.GetMarker() : (lastTimestamp == frame.GetTimestamp()))
    return;

  unsigned diff = (m_lastPacketNetTime - previousPacketNetTime).GetInterval();

  m_averageTimeAccum += diff;
  if (diff > m_maximumTimeAccum)
    m_maximumTimeAccum = diff;
  if (diff < m_minimumTimeAccum)
    m_minimumTimeAccum = diff;
  m_statisticsCount++;

  if (m_direction == e_Receiver) {
    // As per RFC3550 Appendix 8
    diff *= m_session.m_timeUnits; // Convert to timestamp units
    long variance = diff > m_lastJitterTimestamp ? (diff - m_lastJitterTimestamp) : (m_lastJitterTimestamp - diff);
    m_lastJitterTimestamp = diff;
    m_jitterAccum += variance - ((m_jitterAccum + (1 << (JitterRoundingGuardBits - 1))) >> JitterRoundingGuardBits);
    m_currentjitter = (m_jitterAccum >> JitterRoundingGuardBits) / m_session.m_timeUnits;
    if (m_maximumJitter < m_currentjitter)
      m_maximumJitter = m_currentjitter;
  }

  if (m_statisticsCount < (m_direction == e_Receiver ? m_session.GetRxStatisticsInterval() : m_session.GetTxStatisticsInterval()))
    return;

  m_averagePacketTime = m_averageTimeAccum/m_statisticsCount;
  m_maximumPacketTime = m_maximumTimeAccum;
  m_minimumPacketTime = m_minimumTimeAccum;

  m_statisticsCount  = 0;
  m_averageTimeAccum = 0;
  m_maximumTimeAccum = 0;
  m_minimumTimeAccum = 0xffffffff;

#if PTRACING
  unsigned Level = 4;
  if (PTrace::CanTrace(Level)) {
    ostream & trace = PTRACE_BEGIN(Level, &m_session, PTraceModule());
    trace << *this
          << m_direction << " statistics:"
                 " packets=" << m_packets <<
                 " octets=" << m_octets;
    if (m_direction == e_Receiver) {
      trace <<   " lost=" << m_session.GetPacketsLost() <<
                 " order=" << m_session.GetPacketsOutOfOrder();
      OpalJitterBuffer * jb = GetJitterBuffer();
      if (jb != NULL)
        trace << " tooLate=" << jb->GetPacketsTooLate();
    }
    trace <<     " avgTime=" << m_averagePacketTime <<
                 " maxTime=" << m_maximumPacketTime <<
                 " minTime=" << m_minimumPacketTime <<
                 " jitter=" << m_currentjitter <<
                 " maxJitter=" << m_maximumJitter
          << PTrace::End;
  }
#endif
}


OpalRTPSession::SendReceiveStatus OpalRTPSession::SyncSource::OnSendData(RTP_DataFrame & frame, RewriteMode rewrite)
{
  if (rewrite != e_RewriteNothing)
    frame.SetSyncSource(m_sourceIdentifier);

  if (m_packets == 0) {
    m_firstPacketTime.SetCurrentTime();
    if (rewrite == e_RewriteHeader)
      frame.SetSequenceNumber(m_lastSequenceNumber = (RTP_SequenceNumber)PRandom::Number(1, 32768));
    PTRACE(3, &m_session, m_session << "first sent data: "
            << setw(1) << frame
            << " rem=" << m_session.GetRemoteAddress()
            << " local=" << m_session.GetLocalAddress());
  }
  else {
    PTRACE_IF(5, frame.GetDiscontinuity() > 0, &m_session,
              *this << "have discontinuity: " << frame.GetDiscontinuity() << ", sn=" << m_lastSequenceNumber);
    if (rewrite == e_RewriteHeader) {
      frame.SetSequenceNumber(m_lastSequenceNumber += (RTP_SequenceNumber)(frame.GetDiscontinuity() + 1));
      PTRACE_IF(4, m_lastSequenceNumber == 0, &m_session, m_session << "sequence number wraparound");
    }
  }

  if (rewrite == e_RewriteSSRC)
    m_lastSequenceNumber = frame.GetSequenceNumber();

#if OPAL_RTP_FEC
  if (rewrite != e_RewriteNothing && m_session.GetRedundencyPayloadType() != RTP_DataFrame::IllegalPayloadType) {
    SendReceiveStatus status = OnSendRedundantFrame(frame);
    if (status != e_ProcessPacket)
      return status;
  }
#endif

  // Update absolute time and RTP timestamp for next SR that goes out
  if (m_synthesizeAbsTime && !frame.GetAbsoluteTime().IsValid())
      frame.SetAbsoluteTime();

  PTRACE_IF(3, !m_reportAbsoluteTime.IsValid() && frame.GetAbsoluteTime().IsValid(), &m_session,
            m_session << "sent first RTP with absolute time: " <<
            frame.GetAbsoluteTime().AsString(PTime::TodayFormat, PTrace::GetTimeZone()));

  m_reportAbsoluteTime = frame.GetAbsoluteTime();
  m_reportTimestamp = frame.GetTimestamp();

  CalculateStatistics(frame);

  PTRACE(m_throttleSendData, &m_session, m_session << "sending packet " << setw(1) << frame << m_throttleSendData);
  return e_ProcessPacket;
}


OpalRTPSession::SendReceiveStatus OpalRTPSession::SyncSource::OnReceiveData(RTP_DataFrame & frame, bool newData)
{
  frame.SetLipSyncId(m_mediaStreamId);

  RTP_SequenceNumber sequenceNumber = frame.GetSequenceNumber();
  RTP_SequenceNumber expectedSequenceNumber = m_lastSequenceNumber + 1;
  RTP_SequenceNumber sequenceDelta = sequenceNumber - expectedSequenceNumber;

  if (newData && !m_pendingPackets.empty() && sequenceNumber == expectedSequenceNumber) {
    PTRACE(5, &m_session, *this << "received out of order packet " << sequenceNumber);
    ++m_packetsOutOfOrder; // it arrived after all!
  }

  // Check packet sequence numbers
  if (m_packets == 0) {
    m_firstPacketTime.SetCurrentTime();
    m_lastPacketNetTime.SetCurrentTime();

    PTRACE(3, &m_session, m_session << "first receive data:" << setw(1) << frame);

#if OPAL_RTCP_XR
    delete m_metrics; // Should be NULL, but just in case ...
    m_metrics = RTCP_XR_Metrics::Create(frame);
    PTRACE_CONTEXT_ID_SET(m_metrics, m_session);
#endif

    SetLastSequenceNumber(sequenceNumber);
  }
  else if (sequenceDelta == 0) {
    PTRACE(m_throttleReceiveData, &m_session, m_session << "received packet " << setw(1) << frame << m_throttleReceiveData);
    SetLastSequenceNumber(sequenceNumber);
    m_consecutiveOutOfOrderPackets = 0;
  }
  else if (sequenceDelta > SequenceReorderThreshold) {
    ++m_lateOutOfOrder;
    if (m_packetsLost > 0)
      --m_packetsLost; // Previously marked as lost

    // If get multiple late out of order packet inside a period of time
    bool running = m_lateOutOfOrderAdaptTimer.IsRunning();
    if (running && ++m_lateOutOfOrderAdaptCount >= m_lateOutOfOrderAdaptMax) {
      PTimeInterval timeout = m_session.GetOutOfOrderWaitTime() + m_lateOutOfOrderAdaptBoost;
      m_session.SetOutOfOrderWaitTime(timeout);
      PTRACE(2, &m_session, *this << "late out of order packet:"
                                     " got " << sequenceNumber << ", expected " << expectedSequenceNumber << ","
                                     " increased timeout to " << setprecision(2) << timeout);
      running = false;
    }
    else {
      PTRACE(3, &m_session, *this << "late out of order packet: got " << sequenceNumber << ", expected " << expectedSequenceNumber);
    }
    if (!running) {
      m_lateOutOfOrderAdaptTimer = m_lateOutOfOrderAdaptPeriod;
      m_lateOutOfOrderAdaptCount = 0;
    }
  }
  else if (sequenceDelta > SequenceRestartThreshold) {
    // Check for where sequence numbers suddenly start incrementing from a different base.

    if (m_consecutiveOutOfOrderPackets > 0 && (sequenceNumber != m_nextOutOfOrderPacket || m_consecutiveOutOfOrderTimer.HasExpired())) {
      m_consecutiveOutOfOrderPackets = 0;
      m_consecutiveOutOfOrderTimer = 1000;
    }
    m_nextOutOfOrderPacket = sequenceNumber+1;

    if (++m_consecutiveOutOfOrderPackets < 10) {
      PTRACE(m_consecutiveOutOfOrderPackets == 1 ? 3 : 4, &m_session,
             *this << "incorrect sequence, got " << sequenceNumber << " expected " << expectedSequenceNumber);
      m_packetsOutOfOrder++; // Allow next layer up to deal with out of order packet
    }
    else {
      PTRACE(2, &m_session, *this << "abnormal change of sequence numbers, adjusting from " << m_lastSequenceNumber << " to " << sequenceNumber);
      SetLastSequenceNumber(sequenceNumber);
    }

#if OPAL_RTCP_XR
    if (m_metrics != NULL) m_metrics->OnPacketDiscarded();
#endif
  }
  else {
    if (m_session.ResequenceOutOfOrderPackets(*this)) {
      SendReceiveStatus status = m_session.OnOutOfOrderPacket(frame);
      if (status != e_ProcessPacket)
        return status;
      sequenceNumber = frame.GetSequenceNumber();
      sequenceDelta = sequenceNumber - expectedSequenceNumber;
    }

    frame.SetDiscontinuity(sequenceDelta);
    m_packetsLost += sequenceDelta;
    m_packetsLostSinceLastRR += sequenceDelta;
    PTRACE(3, &m_session, *this << sequenceDelta << " packet(s) missing at " << expectedSequenceNumber << ", processing from " << sequenceNumber);
    SetLastSequenceNumber(sequenceNumber);
    m_consecutiveOutOfOrderPackets = 0;
#if OPAL_RTCP_XR
    if (m_metrics != NULL) m_metrics->OnPacketLost(sequenceDelta);
#endif
  }

  PTime absTime(0);
  if (m_reportAbsoluteTime.IsValid()) {
    int64_t deltaTS = (int64_t)frame.GetTimestamp() - (int64_t)m_reportTimestamp;
    if (deltaTS > -500*(int)m_session.m_timeUnits && deltaTS < (m_lastSenderReportTime.GetElapsed().GetMilliSeconds()+500)*m_session.m_timeUnits)
      absTime = m_reportAbsoluteTime + PTimeInterval(deltaTS/m_session.m_timeUnits);
    else {
      PTRACE(4,  &m_session, *this << "unexpected jump in RTP timestamp (" << frame.GetTimestamp() << ")"
                                      " from SenderReport (" << m_reportTimestamp << ") delta=" << deltaTS);
      m_reportAbsoluteTime = 0;
    }
  }
  frame.SetAbsoluteTime(absTime);

#if OPAL_RTCP_XR
  if (m_metrics != NULL) {
    m_metrics->OnPacketReceived();
    OpalJitterBuffer * jb = GetJitterBuffer();
    if (jb != NULL)
      m_metrics->SetJitterDelay(jb->GetCurrentJitterDelay() / m_session.m_timeUnits);
  }
#endif

  SendReceiveStatus status = m_session.OnReceiveData(frame);

#if OPAL_RTP_FEC
  if (status == e_ProcessPacket && frame.GetPayloadType() == m_session.m_redundencyPayloadType)
    status = OnReceiveRedundantFrame(frame);
#endif

  CalculateStatistics(frame);

  // Final user handling of the read frame
  if (status != e_ProcessPacket)
    return status;

  Data data(frame);
  for (NotifierMap::iterator it = m_notifiers.begin(); it != m_notifiers.end(); ++it) {
    it->second(m_session, data);
    if (data.m_status != e_ProcessPacket) {
      PTRACE(5, &m_session, "Data processing ended, notifier returned " << data.m_status);
      return data.m_status;
    }
  }

  return e_ProcessPacket;
}


void OpalRTPSession::SyncSource::SetLastSequenceNumber(RTP_SequenceNumber sequenceNumber)
{
  if (sequenceNumber < m_lastSequenceNumber)
    m_extendedSequenceNumber += 0x10000; // Next cycle
  m_extendedSequenceNumber = (m_extendedSequenceNumber & 0xffff0000) | sequenceNumber;
  m_lastSequenceNumber = sequenceNumber;
}


bool OpalRTPSession::ResequenceOutOfOrderPackets(SyncSource & receiver) const
{
  OpalJitterBuffer * jb = receiver.GetJitterBuffer();
  return jb == NULL || jb->GetCurrentJitterDelay() == 0;
}


OpalRTPSession::SendReceiveStatus OpalRTPSession::SyncSource::OnOutOfOrderPacket(RTP_DataFrame & frame)
{
  RTP_SequenceNumber sequenceNumber = frame.GetSequenceNumber();
  RTP_SequenceNumber expectedSequenceNumber = m_lastSequenceNumber + 1;

  bool waiting = true;
  if (m_pendingPackets.empty()) {
    m_waitOutOfOrderTimer = m_session.GetOutOfOrderWaitTime();
    PTRACE(3, &m_session, *this << "first out of order packet, got " << sequenceNumber
           << " expected " << expectedSequenceNumber << ", waiting " << m_session.GetOutOfOrderWaitTime() << 's');
  }
  else if (m_pendingPackets.GetSize() > m_session.GetMaxOutOfOrderPackets() || m_waitOutOfOrderTimer.HasExpired()) {
    waiting = false;
    PTRACE(4, &m_session, *this << "last out of order packet, got " << sequenceNumber
           << " expected " << expectedSequenceNumber << ", waited " << m_waitOutOfOrderTimer.GetElapsed() << 's');
  }
  else {
    PTRACE(5, &m_session, *this << "next out of order packet, got " << sequenceNumber
           << " expected " << expectedSequenceNumber);
  }

  RTP_DataFrameList::iterator it;
  for (it = m_pendingPackets.begin(); it != m_pendingPackets.end(); ++it) {
    if (sequenceNumber > it->GetSequenceNumber())
      break;
  }

  m_pendingPackets.insert(it, frame);
  frame.MakeUnique();

  if (waiting)
    return e_IgnorePacket;

  // Give up on the packet, probably never coming in. Save current and switch in
  // the lowest numbered packet.

  while (!m_pendingPackets.empty()) {
    frame = m_pendingPackets.back();
    m_pendingPackets.pop_back();

    sequenceNumber = frame.GetSequenceNumber();
    if (sequenceNumber >= expectedSequenceNumber)
      return e_ProcessPacket;

    PTRACE(2, &m_session, *this << "incorrect out of order packet, got " << sequenceNumber << " expected " << expectedSequenceNumber);
  }

  return e_ProcessPacket;
}


bool OpalRTPSession::SyncSource::HandlePendingFrames()
{
  while (!m_pendingPackets.empty()) {
    RTP_DataFrame resequencedPacket = m_pendingPackets.back();

    RTP_SequenceNumber sequenceNumber = resequencedPacket.GetSequenceNumber();
    RTP_SequenceNumber expectedSequenceNumber = m_lastSequenceNumber + 1;
    if (sequenceNumber != expectedSequenceNumber)
      return true;

    m_pendingPackets.pop_back();

#if PTRACING
    unsigned level = m_pendingPackets.empty() ? 3 : 5;
    if (PTrace::CanTrace(level)) {
      ostream & trace = PTRACE_BEGIN(level, &m_session);
      trace << *this << "resequenced out of order packet " << sequenceNumber;
      if (m_pendingPackets.empty())
        trace << ", completed. Time to resequence=" << m_waitOutOfOrderTimer.GetElapsed();
      else
        trace << ", " << m_pendingPackets.size() << " remaining.";
      trace << PTrace::End;
    }
#endif

    // Still more packets, reset timer to allow for later out-of-order packets
    if (!m_pendingPackets.empty())
      m_waitOutOfOrderTimer = m_session.GetOutOfOrderWaitTime();

    if (OnReceiveData(resequencedPacket, false) == e_AbortTransport)
      return false;
  }

  return true;
}


void OpalRTPSession::AttachTransport(const OpalMediaTransportPtr & newTransport)
{
  InternalAttachTransport(newTransport PTRACE_PARAM(, "attached"));
}


void OpalRTPSession::InternalAttachTransport(const OpalMediaTransportPtr & newTransport PTRACE_PARAM(, const char * from))
{
  OpalMediaSession::AttachTransport(newTransport);
  if (!IsOpen())
    return;

  newTransport->AddReadNotifier(m_dataNotifier, e_Data);
  if (!m_singlePortRx)
    newTransport->AddReadNotifier(m_controlNotifier, e_Control);

  m_rtcpPacketsReceived = 0;

  PIPAddress localAddress(0);
  GetDataSocket().GetLocalAddress(localAddress);
  m_packetOverhead = localAddress.GetVersion() == 4 ? (20 + 8 + 12) : (40 + 8 + 12);

  SetQoS(m_qos);

  if (!m_reportTimer.IsRunning())
    m_reportTimer.RunContinuous(m_reportTimer.GetResetTime());

  RTP_SyncSourceId ssrc = GetSyncSourceOut();
  if (ssrc == 0)
    ssrc = AddSyncSource(0, e_Sender); // Add default sender SSRC

  m_endpoint.RegisterLocalRTP(this, false);

  PTRACE(3, *this << from << ": "
            " local=" << GetLocalAddress() << '-' << GetLocalControlPort()
         << " remote=" << GetRemoteAddress()
         << " added default sender SSRC=" << RTP_TRACE_SRC(ssrc));
}


OpalMediaTransportPtr OpalRTPSession::DetachTransport()
{
  PTRACE(4, *this << "detaching transport " << m_transport);
  m_endpoint.RegisterLocalRTP(this, true);
  return OpalMediaSession::DetachTransport();
}


bool OpalRTPSession::AddGroup(const PString & groupId, const PString & mediaId, bool overwrite)
{
  if (!OpalMediaSession::AddGroup(groupId, mediaId, overwrite))
    return false;

  if (IsGroupMember(GetBundleGroupId())) {
    // When bundling we force rtcp-mux and only allow announced SSRC values
    m_singlePortRx = true;
    m_allowAnySyncSource = false;
  }

  return true;
}


OpalRTPSession::SendReceiveStatus OpalRTPSession::SendBYE(RTP_SyncSourceId ssrc)
{
  if (!IsOpen())
    return e_AbortTransport;

  PSafeLockReadOnly lock(*this);
  if (!lock.IsLocked())
    return e_AbortTransport;

  SyncSource * sender;
  if (!GetSyncSource(ssrc, e_Sender, sender))
    return e_ProcessPacket;

  SendReceiveStatus status = sender->SendBYE();
  if (status == e_ProcessPacket) {
    // Now remove the shut down SSRC
    LockReadWrite();

    SyncSourceMap::iterator it = m_SSRC.find(ssrc);
    if (it != m_SSRC.end()) {
      delete it->second;
      m_SSRC.erase(it);
    }

    UnlockReadWrite();
  }

  return status;
}


OpalRTPSession::SendReceiveStatus OpalRTPSession::SyncSource::SendBYE()
{
  PTRACE(3, &m_session, *this << "sending BYE");

  RTP_ControlFrame report;
  m_session.InitialiseControlFrame(report, *this);

  static char const ReasonStr[] = "Session ended";
  static size_t ReasonLen = sizeof(ReasonStr);

  // insert BYE
  report.StartNewPacket(RTP_ControlFrame::e_Goodbye);
  report.SetPayloadSize(4+1+ReasonLen);  // length is SSRC + ReasonLen + reason

  BYTE * payload = report.GetPayloadPtr();

  // one SSRC
  report.SetCount(1);
  *(PUInt32b *)payload = m_sourceIdentifier;

  // insert reason
  payload[4] = (BYTE)ReasonLen;
  memcpy((char *)(payload+5), ReasonStr, ReasonLen);

  report.EndPacket();
  return m_session.WriteControl(report);
}


OpalJitterBuffer * OpalRTPSession::SyncSource::GetJitterBuffer() const
{
    if (m_direction != e_Receiver)
        return NULL;

    return m_jitterBuffer != NULL ? m_jitterBuffer : m_session.m_jitterBuffer;
}


PString OpalRTPSession::GetCanonicalName(RTP_SyncSourceId ssrc, Direction dir) const
{
  PSafeLockReadOnly lock(*this);

  SyncSource * info;
  if (!GetSyncSource(ssrc, dir, info))
    return PString::Empty();

  PString s = info->m_canonicalName;
  s.MakeUnique();
  return s;
}


void OpalRTPSession::SetCanonicalName(const PString & name, RTP_SyncSourceId ssrc, Direction dir)
{
  PSafeLockReadWrite lock(*this);

  SyncSource * info;
  if (GetSyncSource(ssrc, dir, info)) {
    info->m_canonicalName = name;
    info->m_canonicalName.MakeUnique();
  }
}


PString OpalRTPSession::GetMediaStreamId(RTP_SyncSourceId ssrc, Direction dir) const
{
  PString s;
  PSafeLockReadOnly lock(*this);
  SyncSource * info;
  if (GetSyncSource(ssrc, dir, info)) {
    s = info->m_mediaStreamId;
    s.MakeUnique();
  }
  return s;
}


void OpalRTPSession::SetMediaStreamId(const PString & id, RTP_SyncSourceId ssrc, Direction dir)
{
  PSafeLockReadWrite lock(*this);
  SyncSource * info;
  if (GetSyncSource(ssrc, dir, info)) {
    if (info->m_mediaTrackId.IsEmpty() || info->m_mediaTrackId.NumCompare(info->m_mediaStreamId + '+') == EqualTo)
      info->m_mediaTrackId = PSTRSTRM(id << '+' << m_mediaType);
    info->m_mediaStreamId = id;
    info->m_mediaStreamId.MakeUnique();
    PTRACE(4, *this << "set MediaStream id for SSRC=" << RTP_TRACE_SRC(ssrc) << " to \"" << id << '"');
  }
}


PString OpalRTPSession::GetMediaTrackId(RTP_SyncSourceId ssrc, Direction dir) const
{
  PString s;
  PSafeLockReadOnly lock(*this);
  SyncSource * info;
  if (GetSyncSource(ssrc, dir, info)) {
    s = info->m_mediaTrackId;
    s.MakeUnique();
  }
  return s;
}


void OpalRTPSession::SetMediaTrackId(const PString & id, RTP_SyncSourceId ssrc, Direction dir)
{
  PSafeLockReadWrite lock(*this);
  SyncSource * info;
  if (GetSyncSource(ssrc, dir, info)) {
    info->m_mediaTrackId = id;
    info->m_mediaTrackId.MakeUnique();
    PTRACE(4, *this << "set MediaStreamTrack id for SSRC=" << RTP_TRACE_SRC(ssrc) << " to \"" << id << '"');
  }
}


PString OpalRTPSession::GetToolName() const
{
  PSafeLockReadOnly lock(*this);
  PString s = m_toolName;
  s.MakeUnique();
  return s;
}


void OpalRTPSession::SetToolName(const PString & name)
{
  PSafeLockReadWrite lock(*this);
  m_toolName = name;
  m_toolName.MakeUnique();
}


RTPHeaderExtensions OpalRTPSession::GetHeaderExtensions() const
{
  PSafeLockReadOnly lock(*this);
  return m_headerExtensions;
}


const PString & OpalRTPSession::GetAbsSendTimeHdrExtURI() { static const PConstString s("http://www.webrtc.org/experiments/rtp-hdrext/abs-send-time"); return s; }
const PString & OpalRTPSession::GetTransportWideSeqNumHdrExtURI() { static const PConstString s("http://www.ietf.org/id/draft-holmer-rmcat-transport-wide-cc-extensions-01"); return s; }

void OpalRTPSession::SetHeaderExtensions(const RTPHeaderExtensions & ext)
{
  PSafeLockReadWrite lock(*this);

  for (RTPHeaderExtensions::const_iterator it = ext.begin(); it != ext.end(); ++it) {
    PCaselessString uri = it->m_uri.AsString();
    if (uri == GetAbsSendTimeHdrExtURI())
      m_absSendTimeHdrExtId = it->m_id;
    else if (uri == GetTransportWideSeqNumHdrExtURI())
      m_transportWideSeqNumHdrExtId = it->m_id;
    else {
      PTRACE(3, "Unsupported header extension: id=" << it->m_id << ", uri=" << it->m_uri);
      continue;
    }

    m_headerExtensions.insert(*it);
  }
}


bool OpalRTPSession::SyncSource::OnSendReceiverReport(RTP_ControlFrame::ReceiverReport * report PTRACE_PARAM(, unsigned logLevel))
{
  if (m_direction != e_Receiver)
    return false;

  if (m_packets == 0 && !m_lastSenderReportTime.IsValid())
    return false;

  if (report == NULL)
    return true;

  report->ssrc = m_sourceIdentifier;

  unsigned lost = m_session.GetPacketsLost();
  OpalJitterBuffer * jb = GetJitterBuffer();
  if (jb != NULL)
    lost += jb->GetPacketsTooLate();
  report->SetLostPackets(lost);

  if (m_extendedSequenceNumber >= m_lastRRSequenceNumber)
    report->fraction = (BYTE)((m_packetsLostSinceLastRR<<8)/(m_extendedSequenceNumber - m_lastRRSequenceNumber + 1));
  else
    report->fraction = 0;
  m_packetsLostSinceLastRR = 0;

  report->last_seq = m_lastRRSequenceNumber;
  m_lastRRSequenceNumber = m_extendedSequenceNumber;

  report->jitter = m_jitterAccum >> JitterRoundingGuardBits; // Allow for rounding protection bits

  /* Time remote sent us in SR. Note this has to be IDENTICAL to what we
     received in SR as it is used as a de-facto sequence number for the
     SR that was sent. We match RR's to SR's this way. */
  report->lsr = (uint32_t)(m_ntpPassThrough >> 16);

  // Delay since last received SR
  report->dlsr = m_lastSenderReportTime.IsValid() ? (uint32_t)((PTime() - m_lastSenderReportTime).GetMilliSeconds()*65536/1000) : 0;

  PTRACE(logLevel, &m_session, *this << "sending ReceiverReport:"
            " fraction=" << (unsigned)report->fraction
         << " lost=" << report->GetLostPackets()
         << " last_seq=" << report->last_seq
         << " jitter=" << report->jitter
         << " lsr=" << report->lsr
         << " dlsr=" << report->dlsr);
  return true;
}


bool OpalRTPSession::SyncSource::OnSendDelayLastReceiverReport(RTP_ControlFrame::DelayLastReceiverReport::Receiver * report)
{
  if (m_direction != e_Receiver || !m_referenceReportNTP.IsValid() || !m_referenceReportTime.IsValid())
    return false;

  if (report != NULL)
    RTP_ControlFrame::AddDelayLastReceiverReport(*report, m_sourceIdentifier, m_referenceReportNTP, PTime() - m_referenceReportTime);

  return true;
}


#if PTRACING
__inline PTimeInterval abs(const PTimeInterval & i) { return i < 0 ? -i : i; }
#endif

void OpalRTPSession::SyncSource::OnRxSenderReport(const RTP_SenderReport & report)
{
  PAssert(m_direction == e_Receiver, PLogicError);
  PTRACE(m_throttleRxSR, &m_session, m_session << "OnRxSenderReport: " << report << m_throttleRxSR);

  PTime now;
  PTRACE_IF(2, m_reportAbsoluteTime.IsValid() && m_lastSenderReportTime.IsValid() && report.realTimestamp.IsValid() &&
               abs(report.realTimestamp - m_reportAbsoluteTime) > std::max(PTimeInterval(0,10),(now - m_lastSenderReportTime)*2),
            &m_session, m_session << "OnRxSenderReport: remote NTP time jumped by unexpectedly large amount,"
            " was " << m_reportAbsoluteTime.AsString(PTime::TodayFormat, PTrace::GetTimeZone()) << ","
            " now " << report.realTimestamp.AsString(PTime::TodayFormat, PTrace::GetTimeZone()) << ","
            " last report " << m_lastSenderReportTime.AsString(PTime::TodayFormat, PTrace::GetTimeZone()));
  m_ntpPassThrough = report.ntpPassThrough;
  m_reportAbsoluteTime =  report.realTimestamp;
  m_reportTimestamp = report.rtpTimestamp;
  m_lastSenderReportTime = now; // Remember when SR came in to calculate dlsr in RR when we send it
  m_senderReports++;
}


void OpalRTPSession::SyncSource::OnRxReceiverReport(const RTP_ReceiverReport & report)
{
  PTRACE(m_throttleRxRR, &m_session, m_session << "OnReceiverReport: " << report << m_throttleRxRR);

  m_packetsLost = report.totalLost;
  PTRACE_IF(m_throttleInvalidLost, (unsigned)m_packetsLost > m_packets, &m_session,
            m_session << "remote indicated packet loss (" << m_packetsLost << ")"
            " larger than number of packets we sent (" << m_packets << ')' << m_throttleInvalidLost);
  m_currentjitter = (report.jitter + m_session.m_timeUnits -1)/m_session.m_timeUnits;
  if (m_maximumJitter < m_currentjitter)
    m_maximumJitter = m_currentjitter;

#if OPAL_RTCP_XR
  if (m_metrics != NULL)
    m_metrics->OnRxSenderReport(report.lastTimestamp, report.delay);
#endif

  CalculateRTT(report.lastTimestamp, report.delay);
}

void OpalRTPSession::SyncSource::CalculateRTT(const PTime & reportTime, const PTimeInterval & reportDelay)
{
  if (!reportTime.IsValid()) {
    PTRACE(4, &m_session, *this << "not calculating round trip time, NTP in RR does not match SR.");
    return;
  }

  PTimeInterval myDelay = PTime() - reportTime;
  if (m_session.m_roundTripTime > 0 && myDelay <= reportDelay)
    PTRACE(4, &m_session, *this << "not calculating round trip time, RR arrived too soon after SR.");
  else if (myDelay <= reportDelay) {
    m_session.m_roundTripTime = 1;
    PTRACE(4, &m_session, *this << "very small round trip time, using 1ms");
  }
  else if (myDelay > 2000) {
    PTRACE(4, &m_session, *this << "very large round trip time, ignoring");
  }
  else {
    m_session.m_roundTripTime = (myDelay - reportDelay).GetInterval();
    PTRACE(4, &m_session, *this << "determined round trip time: " << m_session.m_roundTripTime << "ms");
  }
}


void OpalRTPSession::SyncSource::OnRxDelayLastReceiverReport(const RTP_DelayLastReceiverReport & dlrr)
{
  PTRACE(4, &m_session, m_session << "OnRxDelayLastReceiverReport: ssrc=" << RTP_TRACE_SRC(m_sourceIdentifier));
  CalculateRTT(dlrr.m_lastTimestamp, dlrr.m_delay);
}


OpalRTPSession::SendReceiveStatus OpalRTPSession::OnSendData(RTP_DataFrame & frame, RewriteMode rewrite)
{
  RTP_SyncSourceId ssrc = frame.GetSyncSource();
  SyncSource * syncSource;
  if (GetSyncSource(ssrc, e_Sender, syncSource)) {
    if (syncSource->m_direction == e_Receiver) {
      // Got a loopback
      if (syncSource->m_loopbackIdentifier != 0)
        ssrc = syncSource->m_loopbackIdentifier;
      else {
        // Look for an unused one
        SyncSourceMap::iterator it;
        for (it = m_SSRC.begin(); it != m_SSRC.end(); ++it) {
          if (it->second->m_direction == e_Sender && it->second->m_loopbackIdentifier == 0) {
            it->second->m_loopbackIdentifier = ssrc;
            ssrc = it->second->m_sourceIdentifier;
            PTRACE(4, *this << "using loopback SSRC=" << RTP_TRACE_SRC(ssrc)
                   << " for receiver SSRC=" << RTP_TRACE_SRC(syncSource->m_sourceIdentifier));
            break;
          }
        }

        if (it == m_SSRC.end()) {
          if ((ssrc = AddSyncSource(ssrc, e_Sender)) == 0)
            return e_AbortTransport;

          PTRACE(4, *this << "added loopback SSRC=" << RTP_TRACE_SRC(ssrc)
                 << " for receiver SSRC=" << RTP_TRACE_SRC(syncSource->m_sourceIdentifier));
        }

        syncSource->m_loopbackIdentifier = ssrc;
      }
      if (!GetSyncSource(ssrc, e_Sender, syncSource))
        return e_AbortTransport;
    }
  }
  else {
    if ((ssrc = AddSyncSource(ssrc, e_Sender)) == 0)
      return e_AbortTransport;
    PTRACE(4, *this << "added sender SSRC=" << RTP_TRACE_SRC(ssrc));
    GetSyncSource(ssrc, e_Sender, syncSource);
  }

  SendReceiveStatus status = syncSource->OnSendData(frame, rewrite);
  if (status != e_ProcessPacket)
    return status;

  frame.SetTransmitTime();

  if (rewrite != e_RewriteNothing) {
    if (m_absSendTimeHdrExtId <= RTP_DataFrame::MaxHeaderExtensionIdOneByte) {
      unsigned ntp = (frame.GetMetaData().m_transmitTime.GetNTP() >> 14) & 0x00ffffff;
      BYTE data[3] = { (BYTE)(ntp >> 16), (BYTE)(ntp >> 8), (BYTE)ntp };
      frame.SetHeaderExtension(m_absSendTimeHdrExtId, sizeof(data), data, RTP_DataFrame::RFC5285_OneByte);
    }

    OpalMediaTransport::CongestionControl * cc = GetCongestionControl();
    if (cc != NULL) {
      PUInt16b sn((uint16_t)cc->HandleTransmitPacket(m_sessionId, frame.GetSyncSource()));
      frame.SetHeaderExtension(m_transportWideSeqNumHdrExtId, 2, (const BYTE *)&sn, RTP_DataFrame::RFC5285_OneByte);
    }
  }

  return status;
}


OpalRTPSession::SendReceiveStatus OpalRTPSession::OnSendControl(RTP_ControlFrame &)
{
  ++m_rtcpPacketsSent;
  return e_ProcessPacket;
}


OpalRTPSession::SendReceiveStatus OpalRTPSession::OnPreReceiveData(RTP_DataFrame & frame)
{
  for (SyncSourceMap::iterator it = m_SSRC.begin(); it != m_SSRC.end(); ++it) {
    if (!it->second->HandlePendingFrames())
      return e_AbortTransport;
  }

  // Check that the PDU is the right version
  if (frame.GetVersion() != RTP_DataFrame::ProtocolVersion)
    return e_IgnorePacket; // Non fatal error, just ignore

  RTP_DataFrame::PayloadTypes pt = frame.GetPayloadType();

  // Check for if a control packet rather than data packet.
  if (pt > RTP_DataFrame::MaxPayloadType)
    return e_IgnorePacket; // Non fatal error, just ignore

  if (pt == RTP_DataFrame::T38 && frame[3] >= 0x80 && frame.GetPayloadSize() == 0) {
    PTRACE(4, *this << "ignoring left over audio packet from switch to T.38");
    return e_IgnorePacket; // Non fatal error, just ignore
  }

  SyncSource * receiver = UseSyncSource(frame.GetSyncSource(), e_Receiver, false);
  if (receiver == NULL)
      return e_IgnorePacket;

  return receiver->OnReceiveData(frame, true);
}


OpalRTPSession::SendReceiveStatus OpalRTPSession::OnReceiveData(RTP_DataFrame & frame)
{
  BYTE * exthdr;
  PINDEX hdrlen;
  if ((exthdr = frame.GetHeaderExtension(RTP_DataFrame::RFC5285_OneByte, m_absSendTimeHdrExtId, hdrlen)) != NULL) {
    if (m_absSendTimeHighBits == 0) {
      PTime highBits = frame.GetAbsoluteTime();
      if (!highBits.IsValid())
        highBits.SetCurrentTime();
      m_absSendTimeHighBits = highBits.GetNTP() & (-1LL << 38);
    }

    int64_t     ntp  = *exthdr++;
    ntp <<= 8;  ntp |= *exthdr++;
    ntp <<= 8;  ntp |= *exthdr++;
    ntp <<= 14; ntp |= m_absSendTimeHighBits;

    if (ntp < m_absSendTimeAllBits) {
      m_absSendTimeHighBits += 1LL << 38;
      ntp = (ntp & ~(-1LL << 38)) | m_absSendTimeHighBits;
    }

    m_absSendTimeAllBits = ntp;
    frame.SetTransmitTimeNTP(ntp);
    PTRACE(6, "Set transmit time on RTP:"
              " sn=" << frame.GetSequenceNumber() << ","
              " time=" << frame.GetMetaData().m_transmitTime.AsString(PTime::TodayFormat));
  }

  if ((exthdr = frame.GetHeaderExtension(RTP_DataFrame::RFC5285_OneByte, m_transportWideSeqNumHdrExtId, hdrlen)) != NULL) {
    uint16_t sn = *(PUInt16b *)exthdr;
    OpalMediaTransport::CongestionControl * cc = GetCongestionControl();
    PTRACE(6, *this << "Received TWCC sequence number: len=" << hdrlen << " sn=" << sn << " cc=" << cc);
    if (cc != NULL)
      cc->HandleReceivePacket(sn, frame.GetMetaData().m_receivedTime);
  }

  return e_ProcessPacket;
}


// Support for http://www.ietf.org/id/draft-holmer-rmcat-transport-wide-cc-extensions
class RTP_TransportWideCongestionControlHandler : public OpalMediaTransport::CongestionControl
{
protected:
  OpalRTPSession & m_session;

  // For transmit
  atomic<uint16_t> m_transportWideSequenceNumber;
  RTP_TransportWideCongestionControl::PacketMap m_sentPackets;

  // For receive
  struct Info
  {
    unsigned m_transportWideSequenceNumber;
    PTime    m_receivedTime;

    Info(unsigned sn, const PTime & received)
      : m_transportWideSequenceNumber(sn)
      , m_receivedTime(received)
    { }
  };
  std::queue<Info> m_queue;
  PTime m_packetBaseTime;
  unsigned m_rtcpSequenceNumber;

public:
  RTP_TransportWideCongestionControlHandler(OpalRTPSession & session)
    : m_session(session)
    , m_packetBaseTime(0)
    , m_rtcpSequenceNumber(0)
  {
  }

  virtual unsigned HandleTransmitPacket(unsigned sessionID, uint32_t ssrc)
  {
    unsigned sn = ++m_transportWideSequenceNumber;
    m_sentPackets.insert(make_pair(sn, RTP_TransportWideCongestionControl::Info(0, sessionID, ssrc)));
    return sn;
  }

  virtual void HandleReceivePacket(unsigned sn, const PTime & received)
  {
    m_queue.push(Info(sn, received));
  }

  virtual PTimeInterval GetProcessInterval() const
  {
    static PTimeInterval const interval(200); // Needs to be frequent, but not ridiculously so.
    return interval;
  }

  virtual bool ProcessReceivedPackets()
  {
    if (m_queue.empty())
      return true;

    /* These are used to detect when SN wraps around, and make sequence number
       larger than 16 bit, so we can maintain the correct order in the
       RTP_TransportWideCongestionControl::PacketTimeMap */
    bool wrapped = false;
    unsigned lastSN = 0;

    RTP_TransportWideCongestionControl twcc;
    twcc.m_rtcpSequenceNumber = ++m_rtcpSequenceNumber;
    do {
      const Info & info = m_queue.front();

      unsigned sn = info.m_transportWideSequenceNumber;

      // detect when we go from top 32768 to bottom 32768
      if (!wrapped && (lastSN & 0x8000) != 0 && (sn & 0x8000) == 0)
        wrapped = true;
      lastSN = sn;

      /* Only add in the 17th bit if on bottom 32768, so out of order packets
         don't end up in wrong half. */
      if (wrapped && (sn & 0x8000) == 0)
        sn |= 0x10000;

      if (!m_packetBaseTime.IsValid())
        m_packetBaseTime = info.m_receivedTime;
      twcc.m_packets.insert(make_pair(sn, info.m_receivedTime - m_packetBaseTime));

      m_queue.pop();
    } while (!m_queue.empty());

    return m_session.SendTWCC(twcc) != OpalRTPSession::e_AbortTransport;
  }

  virtual void ProcessTWCC(RTP_TransportWideCongestionControl & twcc)
  {
    for (RTP_TransportWideCongestionControl::PacketMap::iterator pkt = twcc.m_packets.begin(); pkt != twcc.m_packets.end(); ++pkt) {
      RTP_TransportWideCongestionControl::PacketMap::iterator sent = m_sentPackets.find(pkt->first);
      if (sent != m_sentPackets.end()) {
        pkt->second.m_sessionID = sent->second.m_sessionID;
        pkt->second.m_SSRC = sent->second.m_SSRC;
        m_sentPackets.erase(sent);
      }
    }
  }
};



OpalMediaTransport::CongestionControl * OpalRTPSession::GetCongestionControl()
{
  // Yes, this is all thread safe
  if (m_transportWideSeqNumHdrExtId > RTP_DataFrame::MaxHeaderExtensionIdOneByte)
    return NULL;

  OpalMediaTransportPtr transport = m_transport;
  if (transport == NULL)
    return NULL;

  OpalMediaTransport::CongestionControl * cc = transport->GetCongestionControl();
  if (cc != NULL)
    return cc;

  PTRACE(3, *this << "setting TWCC handler");
  return transport->SetCongestionControl(new RTP_TransportWideCongestionControlHandler(*this));
}


OpalRTPSession::SendReceiveStatus OpalRTPSession::OnOutOfOrderPacket(RTP_DataFrame & frame)
{
  SyncSource * ssrc;
  if (GetSyncSource(frame.GetSyncSource(), e_Receiver, ssrc))
    return ssrc->OnOutOfOrderPacket(frame);

  return e_ProcessPacket;
}


void OpalRTPSession::InitialiseControlFrame(RTP_ControlFrame & frame, SyncSource & sender)
{
  if (m_reducedSizeRTCP)
    return;

  frame.AddReceiverReport(sender.m_sourceIdentifier, 0);
  frame.EndPacket();
}


bool OpalRTPSession::InternalSendReport(RTP_ControlFrame & report, SyncSource & sender, bool includeReceivers, bool forced)
{
  if (sender.m_direction != e_Sender || (!forced && sender.m_packets == 0))
    return false;

#if PTRACING
  unsigned logLevel = 3U;
  const char * forcedStr = "(periodic) ";
  if (forced) {
      m_throttleTxReport.CanTrace(); // Sneakiness to make sure throttle works
      logLevel = m_throttleTxReport;
      forcedStr = "(forced) ";
  }
#endif

  unsigned receivers = 0;
  if (includeReceivers) {
    for (SyncSourceMap::iterator it = m_SSRC.begin(); it != m_SSRC.end(); ++it) {
      if (it->second->OnSendReceiverReport(NULL PTRACE_PARAM(, logLevel)))
        ++receivers;
    }
  }

  RTP_ControlFrame::ReceiverReport * rr = NULL;
  if (sender.m_packets == 0) {
    rr = report.AddReceiverReport(sender.m_sourceIdentifier, receivers);

    PTRACE(logLevel, sender << "sending " << forcedStr
           << (receivers == 0 ? "empty " : "") << "ReceiverReport" << m_throttleTxReport);
  }
  else {
    rr = report.AddSenderReport(sender.m_sourceIdentifier,
                                sender.m_reportAbsoluteTime,
                                sender.m_reportTimestamp,
                                sender.m_packets,
                                sender.m_octets,
                                receivers);

    sender.m_ntpPassThrough = sender.m_reportAbsoluteTime.GetNTP();
    sender.m_lastSenderReportTime.SetCurrentTime();

    PTRACE(logLevel, sender << "sending " << forcedStr << "SenderReport:"
              " ntp=" << sender.m_reportAbsoluteTime.AsString(PTime::TodayFormat, PTrace::GetTimeZone())
           << " 0x" << hex << sender.m_ntpPassThrough << dec
           << " rtp=" << sender.m_reportTimestamp
           << " psent=" << sender.m_packets
           << " osent=" << sender.m_octets
           << m_throttleTxReport);
  }

  if (rr != NULL) {
    for (SyncSourceMap::iterator it = m_SSRC.begin(); it != m_SSRC.end(); ++it) {
      if (it->second->OnSendReceiverReport(rr PTRACE_PARAM(, logLevel)))
        ++rr;
    }
  }

  report.EndPacket();

#if OPAL_RTCP_XR
  if (includeReceivers) {
    for (SyncSourceMap::iterator it = m_SSRC.begin(); it != m_SSRC.end(); ++it) {
      //Generate and send RTCP-XR packet
      if (it->second->m_direction == e_Receiver && it->second->m_metrics != NULL)
        it->second->m_metrics->InsertMetricsReport(report, *this, it->second->m_sourceIdentifier, it->second->GetJitterBuffer());
    }
  }
#endif

  // Add the SDES part to compound RTCP packet
  PTRACE(logLevel, sender << "sending SDES cname=\"" << sender.m_canonicalName << '"');
  report.AddSourceDescription(sender.m_sourceIdentifier, sender.m_canonicalName, m_toolName);

  // Count receivers that have had a RRTR
  receivers = 0;
  for (SyncSourceMap::iterator it = m_SSRC.begin(); it != m_SSRC.end(); ++it) {
    if (it->second->OnSendDelayLastReceiverReport(NULL))
      ++receivers;
  }

  if (receivers != 0) {
    RTP_ControlFrame::DelayLastReceiverReport::Receiver * dlrr = report.AddDelayLastReceiverReport(sender.m_sourceIdentifier, receivers);

    for (SyncSourceMap::iterator it = m_SSRC.begin(); it != m_SSRC.end(); ++it, ++dlrr) {
      if (it->second->OnSendDelayLastReceiverReport(dlrr)) {
        PTRACE(logLevel, *it->second << "sending DLRR");
      }
    }
    report.EndPacket();
  }

  return report.IsValid();
}


bool OpalRTPSession::SyncSource::IsStaleReceiver() const
{
  if (m_direction != e_Receiver)
    return false;

  // Had n SR sent to us, so still active
  if (m_lastSenderReportTime.IsValid() && m_lastSenderReportTime.GetElapsed() < m_session.m_staleReceiverTimeout)
      return false;

  // Not started yet, no RR should be sent so safe to leave for now
  if (m_packets == 0)
    return false;

  // Are still getting packets
  if (m_lastPacketNetTime.GetElapsed() < m_session.m_staleReceiverTimeout)
    return false;

  PTRACE(3, &m_session, *this << "removing stale receiver");
  return true;
}


void OpalRTPSession::TimedSendReport(PTimer&, P_INT_PTR)
{
  PTRACE_CONTEXT_ID_PUSH_THREAD(*this);
  PTRACE(5, *this << "sending periodic report");
  SendReport(0, false);
}


OpalRTPSession::SendReceiveStatus OpalRTPSession::SendReport(RTP_SyncSourceId ssrc, bool force)
{
  std::list<RTP_ControlFrame> frames;

  if (!LockReadOnly())
    return e_AbortTransport;

  // Clean out old stale SSRC's
  for (SyncSourceMap::iterator it = m_SSRC.begin(); it != m_SSRC.end();) {
    if (it->second->IsStaleReceiver())
      m_SSRC.erase(it++);
    else
      ++it;
  }

  if (ssrc != 0) {
    SyncSource * sender;
    if (GetSyncSource(ssrc, e_Sender, sender)) {
      RTP_ControlFrame frame;
      if (InternalSendReport(frame, *sender, true, force))
        frames.push_back(frame);
    }
  }
  else {
    bool includeReceivers = true;
    for (SyncSourceMap::iterator it = m_SSRC.begin(); it != m_SSRC.end(); ++it) {
      RTP_ControlFrame frame;
      if (InternalSendReport(frame, *it->second, includeReceivers, force)) {
        frames.push_back(frame);
        includeReceivers = false;
      }
    }
    if (includeReceivers) {
      RTP_ControlFrame frame;
      SyncSource * sender = NULL;
      if (!GetSyncSource(0, e_Sender, sender))
        GetSyncSource(AddSyncSource(0, e_Sender), e_Sender, sender); // Must always have one sender
      if (sender != NULL && InternalSendReport(frame, *sender, true, true))
        frames.push_back(frame);
    }

    if (force && !frames.empty() && !m_reportTimer.IsRunning())
      m_reportTimer.RunContinuous(m_reportTimer.GetResetTime());
  }

  UnlockReadOnly();

  // Actual transmission has to be outside mutex
  SendReceiveStatus status = e_ProcessPacket;
  for (std::list<RTP_ControlFrame>::iterator it = frames.begin(); it != frames.end(); ++it) {
    status = WriteControl(*it);
    if (status != e_ProcessPacket)
      break;
  }

  PTRACE(4, *this << "sent " << frames.size() << ' ' << (force ? "forced" : "periodic") << " reports: status=" << status);
  return status;
}


#if OPAL_STATISTICS
static void AddSpecial(int & left, int right)
{
  if (left < 0)
    left = right;
  else
    left += right;
}

void OpalRTPSession::GetStatistics(OpalMediaStatistics & statistics, Direction dir) const
{
  PSafeLockReadOnly lock(*this);
  if (!lock.IsLocked())
    return;

  OpalMediaSession::GetStatistics(statistics, dir);

  statistics.m_totalBytes        = 0;
  statistics.m_totalPackets      = 0;
  statistics.m_controlPacketsIn  = m_rtcpPacketsReceived;
  statistics.m_controlPacketsOut = m_rtcpPacketsSent;
  statistics.m_NACKs             = -1;
  statistics.m_packetsLost       = -1;
  statistics.m_packetsOutOfOrder = -1;
  statistics.m_lateOutOfOrder    = -1;
  statistics.m_packetsTooLate    = -1;
  statistics.m_packetOverruns    = -1;
  statistics.m_minimumPacketTime = -1;
  statistics.m_averagePacketTime = -1;
  statistics.m_maximumPacketTime = -1;
  statistics.m_averageJitter     = -1;
  statistics.m_maximumJitter     = -1;
  statistics.m_jitterBufferDelay = -1;
  statistics.m_roundTripTime     = m_roundTripTime;
  statistics.m_lastPacketRTP     = -1;
  statistics.m_lastPacketAbsTime = 0;
  statistics.m_lastPacketNetTime = 0;
  statistics.m_lastReportTime    = 0;

  if (statistics.m_SSRC != 0) {
    SyncSource * info;
    if (GetSyncSource(statistics.m_SSRC, dir, info))
      info->GetStatistics(statistics);
    return;
  }

  unsigned pktTimeSum = 0;
  unsigned pktTimeCount = 0;
  unsigned jitterSum = 0;
  unsigned jitterCount = 0;
  for (SyncSourceMap::const_iterator it = m_SSRC.begin(); it != m_SSRC.end(); ++it) {
    if (it->second->m_direction == dir) {
      OpalMediaStatistics ssrcStats;
      it->second->GetStatistics(ssrcStats);
      if (ssrcStats.m_totalPackets > 0) {
        statistics.m_totalBytes   += ssrcStats.m_totalBytes;
        statistics.m_totalPackets += ssrcStats.m_totalPackets;

        AddSpecial(statistics.m_NACKs, ssrcStats.m_NACKs);
        AddSpecial(statistics.m_packetsLost, ssrcStats.m_packetsLost);
        AddSpecial(statistics.m_packetsOutOfOrder, ssrcStats.m_packetsOutOfOrder);
        AddSpecial(statistics.m_lateOutOfOrder, ssrcStats.m_lateOutOfOrder);
        AddSpecial(statistics.m_packetsTooLate, ssrcStats.m_packetsTooLate);
        AddSpecial(statistics.m_packetOverruns, ssrcStats.m_packetOverruns);
        AddSpecial(statistics.m_minimumPacketTime, ssrcStats.m_minimumPacketTime);
        AddSpecial(statistics.m_maximumPacketTime, ssrcStats.m_maximumPacketTime);

        if (ssrcStats.m_averagePacketTime >= 0) {
          pktTimeSum += ssrcStats.m_averagePacketTime;
          ++pktTimeCount;
        }

        if (ssrcStats.m_averageJitter >= 0) {
          jitterSum += ssrcStats.m_averageJitter;
          ++jitterCount;
        }

        if (statistics.m_maximumJitter < ssrcStats.m_maximumJitter)
          statistics.m_maximumJitter = ssrcStats.m_maximumJitter;

        if (statistics.m_jitterBufferDelay < ssrcStats.m_jitterBufferDelay)
          statistics.m_jitterBufferDelay = ssrcStats.m_jitterBufferDelay;

        if (!statistics.m_startTime.IsValid() || statistics.m_startTime > ssrcStats.m_startTime)
          statistics.m_startTime = ssrcStats.m_startTime;

        if (statistics.m_lastPacketRTP < ssrcStats.m_lastPacketRTP)
          statistics.m_lastPacketRTP = ssrcStats.m_lastPacketRTP;

        if (statistics.m_lastPacketAbsTime < ssrcStats.m_lastPacketAbsTime)
          statistics.m_lastPacketAbsTime = ssrcStats.m_lastPacketAbsTime;

        if (statistics.m_lastPacketNetTime < ssrcStats.m_lastPacketNetTime)
          statistics.m_lastPacketNetTime = ssrcStats.m_lastPacketNetTime;

        if (statistics.m_lastReportTime < ssrcStats.m_lastReportTime)
          statistics.m_lastReportTime = ssrcStats.m_lastReportTime;
      }
    }
  }
  if (pktTimeCount > 0)
    statistics.m_averagePacketTime = pktTimeSum/pktTimeCount;
  if (jitterCount > 0)
    statistics.m_averageJitter = jitterSum/jitterCount;
}
#endif


#if OPAL_STATISTICS
void OpalRTPSession::SyncSource::GetStatistics(OpalMediaStatistics & statistics) const
{
  statistics.m_startTime         = m_firstPacketTime;
  statistics.m_totalBytes        = m_octets;
  statistics.m_totalPackets      = m_packets;
  if (m_session.m_feedback&OpalMediaFormat::e_NACK)
    statistics.m_NACKs           = m_NACKs;
  statistics.m_packetsLost       = m_packetsLost;
  statistics.m_minimumPacketTime = m_minimumPacketTime;
  statistics.m_averagePacketTime = m_averagePacketTime;
  statistics.m_maximumPacketTime = m_maximumPacketTime;
  statistics.m_averageJitter     = m_currentjitter;
  statistics.m_maximumJitter     = m_maximumJitter;
  if (m_packets > 0)
    statistics.m_lastPacketRTP   = m_lastPacketTimestamp;
  statistics.m_lastPacketAbsTime = m_lastPacketAbsTime;
  statistics.m_lastPacketNetTime = m_lastPacketNetTime;
  statistics.m_lastReportTime    = m_lastSenderReportTime;

  if (m_direction == e_Receiver) {
    statistics.m_packetsOutOfOrder = m_packetsOutOfOrder;
    statistics.m_lateOutOfOrder = m_lateOutOfOrder;

    OpalJitterBuffer * jb = GetJitterBuffer();
    if (jb != NULL && jb->GetCurrentJitterDelay() > 0) {
      statistics.m_packetsTooLate = jb->GetPacketsTooLate();
      statistics.m_packetOverruns = jb->GetBufferOverruns();
      statistics.m_jitterBufferDelay = jb->GetCurrentJitterDelay() / jb->GetTimeUnits();
    }
  }
}
#endif // OPAL_STATISTICS


bool OpalRTPSession::CheckControlSSRC(RTP_SyncSourceId PTRACE_PARAM(senderSSRC),
                                      RTP_SyncSourceId targetSSRC,
                                      SyncSource * & info
                                      PTRACE_PARAM(, const char * pduName)) const
{
#if PTRACING
  unsigned level = IsGroupMember(GetBundleGroupId()) ? 6 : 2;
  PTRACE_IF(level, m_SSRC.find(senderSSRC) == m_SSRC.end(), *this << pduName << " from incorrect SSRC=" << RTP_TRACE_SRC(senderSSRC));
  if (m_SSRC.find(targetSSRC) == m_SSRC.end()) {
    PTRACE(level, *this << pduName << " for incorrect SSRC=" << RTP_TRACE_SRC(targetSSRC));
    return false;
  }
#endif

  return GetSyncSource(targetSSRC, e_Sender, info);
}


OpalRTPSession::SendReceiveStatus OpalRTPSession::OnReceiveControl(RTP_ControlFrame & frame)
{
  if (frame.GetPacketSize() == 0)
    return e_IgnorePacket;

  PTRACE(6, *this << "OnReceiveControl - " << frame);

  ++m_rtcpPacketsReceived;

  do {
    switch (frame.GetPayloadType()) {
      case RTP_ControlFrame::e_SenderReport:
      {
        RTP_SenderReport txReport;
        const RTP_ControlFrame::ReceiverReport * rr;
        unsigned count;
        if (frame.ParseSenderReport(txReport, rr, count)) {
          OnRxSenderReport(txReport);
          OnRxReceiverReports(txReport.sourceIdentifier, rr, count);
        }
        else {
          PTRACE(2, *this << "SenderReport packet truncated - " << frame);
        }
        break;
      }

      case RTP_ControlFrame::e_ReceiverReport:
      {
        RTP_SyncSourceId ssrc;
        const RTP_ControlFrame::ReceiverReport * rr;
        unsigned count;
        if (frame.ParseReceiverReport(ssrc, rr, count)) {
          if (count != 0) {
            OnRxReceiverReports(ssrc, rr, count);
          }
          else {
            PTRACE(m_throttleRxEmptyRR, *this << "received empty ReceiverReport: sender SSRC=" << RTP_TRACE_SRC(ssrc));
          }
        }
        else {
          PTRACE(2, *this << "ReceiverReport packet truncated - " << frame);
        }
        break;
      }

      case RTP_ControlFrame::e_SourceDescription:
      {
        RTP_SourceDescriptionArray descriptions;
        if (frame.ParseSourceDescriptions(descriptions))
          OnRxSourceDescription(descriptions);
        else {
          PTRACE(2, *this << "SourceDescription packet malformed - " << frame);
        }
        break;
      }

      case RTP_ControlFrame::e_Goodbye:
      {
        RTP_SyncSourceId rxSSRC;
        RTP_SyncSourceArray csrc;
        PString msg;
        if (frame.ParseGoodbye(rxSSRC, csrc, msg))
          OnRxGoodbye(csrc, msg);
        else {
          PTRACE(2, *this << "Goodbye packet truncated - " << frame);
        }
        break;
      }

      case RTP_ControlFrame::e_ApplDefined:
      {
        RTP_ControlFrame::ApplDefinedInfo info;
        if (frame.ParseApplDefined(info))
          OnRxApplDefined(info);
        else {
          PTRACE(2, *this << "ApplDefined packet truncated - " << frame);
        }
        break;
      }

      case RTP_ControlFrame::e_ExtendedReport:
        if (!OnReceiveExtendedReports(frame)) {
          PTRACE(2, *this << "ExtendedReport packet truncated - " << frame);
        }
        break;

      case RTP_ControlFrame::e_TransportLayerFeedBack :
        switch (frame.GetFbType()) {
          case RTP_ControlFrame::e_TransportNACK:
          {
            RTP_SyncSourceId senderSSRC, targetSSRC;
            RTP_ControlFrame::LostPacketMask lostPackets;
            if (frame.ParseNACK(senderSSRC, targetSSRC, lostPackets)) {
              SyncSource * ssrc;
              if (CheckControlSSRC(senderSSRC, targetSSRC, ssrc PTRACE_PARAM(, "NACK"))) {
                ++ssrc->m_NACKs;
                OnRxNACK(targetSSRC, lostPackets);
              }
            }
            else {
              PTRACE(2, *this << "NACK packet truncated - " << frame);
            }
            break;
          }

          case RTP_ControlFrame::e_TMMBR :
          {
            RTP_SyncSourceId senderSSRC, targetSSRC;
            unsigned maxBitRate;
            unsigned overhead;
            if (frame.ParseTMMB(senderSSRC, targetSSRC, maxBitRate, overhead)) {
              SyncSource * ssrc;
              if (CheckControlSSRC(senderSSRC, targetSSRC, ssrc PTRACE_PARAM(,"TMMBR"))) {
                PTRACE(4, *this << "received TMMBR:"
                       " rate=" << maxBitRate << ","
                       " receiver SSRC=" << RTP_TRACE_SRC(targetSSRC) << ","
                       " sender SSRC=" << RTP_TRACE_SRC(senderSSRC));
                m_connection.ExecuteMediaCommand(OpalMediaFlowControl(maxBitRate, m_mediaType, m_sessionId, targetSSRC), true);
              }
            }
            else {
              PTRACE(2, *this << "TMMB" << (frame.GetFbType() == RTP_ControlFrame::e_TMMBR ? 'R' : 'N') << " packet truncated - " << frame);
            }
            break;
          }

          case RTP_ControlFrame::e_TWCC:
          {
            RTP_TransportWideCongestionControl twcc;
            RTP_SyncSourceId senderSSRC;
            if (frame.ParseTWCC(senderSSRC, twcc)) {
              OpalMediaTransport::CongestionControl * cc = GetCongestionControl();
              if (cc != NULL)
                cc->ProcessTWCC(twcc);
              OnRxTWCC(twcc);
            }
            else
              PTRACE(2, *this << "PWCC packet truncated - " << frame);
          }
        }
        break;

#if OPAL_VIDEO
      case RTP_ControlFrame::e_IntraFrameRequest :
        PTRACE(4, *this << "received RFC2032 FIR");
        m_connection.ExecuteMediaCommand(OpalVideoUpdatePicture(m_sessionId, GetSyncSourceOut()), true);
        break;
#endif

      case RTP_ControlFrame::e_PayloadSpecificFeedBack :
        switch (frame.GetFbType()) {
#if OPAL_VIDEO
          case RTP_ControlFrame::e_PictureLossIndication:
          {
            RTP_SyncSourceId senderSSRC, targetSSRC;
            if (frame.ParsePLI(senderSSRC, targetSSRC)) {
              SyncSource * ssrc;
              if (CheckControlSSRC(senderSSRC, targetSSRC, ssrc PTRACE_PARAM(,"PLI"))) {
                PTRACE(4, *this << "received RFC4585 PLI:"
                       " receiver SSRC=" << RTP_TRACE_SRC(targetSSRC) << ","
                       " sender SSRC=" << RTP_TRACE_SRC(senderSSRC));
                m_connection.ExecuteMediaCommand(OpalVideoPictureLoss(0, 0, m_sessionId, targetSSRC), true);
              }
            }
            else {
              PTRACE(2, *this << "PLI packet truncated - " << frame);
            }
            break;
          }

          case RTP_ControlFrame::e_FullIntraRequest:
          {
            RTP_SyncSourceId senderSSRC, targetSSRC;
            unsigned sequenceNumber;
            if (frame.ParseFIR(senderSSRC, targetSSRC, sequenceNumber)) {
              SyncSource * ssrc;
              if (CheckControlSSRC(senderSSRC, targetSSRC, ssrc PTRACE_PARAM(,"FIR"))) {
                PTRACE(4, *this << "received RFC5104 FIR:"
                       " sn=" << sequenceNumber << ","
                       " last-sn=" << ssrc->m_lastFIRSequenceNumber << ","
                       " receiver SSRC=" << RTP_TRACE_SRC(targetSSRC) << ","
                       " sender SSRC=" << RTP_TRACE_SRC(senderSSRC));
                if (ssrc->m_lastFIRSequenceNumber != sequenceNumber) {
                  ssrc->m_lastFIRSequenceNumber = sequenceNumber;
                  m_connection.ExecuteMediaCommand(OpalVideoUpdatePicture(m_sessionId, targetSSRC), true);
                }
              }
            }
            else {
              PTRACE(2, *this << "FIR packet truncated - " << frame);
            }
            break;
          }

          case RTP_ControlFrame::e_TemporalSpatialTradeOffRequest:
          {
            RTP_SyncSourceId senderSSRC, targetSSRC;
            unsigned tradeOff, sequenceNumber;
            if (frame.ParseTSTO(senderSSRC, targetSSRC, tradeOff, sequenceNumber)) {
              SyncSource * ssrc;
              if (CheckControlSSRC(senderSSRC, targetSSRC, ssrc PTRACE_PARAM(,"TSTOR"))) {
                PTRACE(4, *this << "received TSTOR:"
                       " tradeOff=" << tradeOff << ","
                       " sn=" << sequenceNumber << ","
                       " last-sn=" << ssrc->m_lastTSTOSequenceNumber << ","
                       " receiver SSRC=" << RTP_TRACE_SRC(targetSSRC) << ","
                       " sender SSRC=" << RTP_TRACE_SRC(senderSSRC));
                if (ssrc->m_lastTSTOSequenceNumber != sequenceNumber) {
                  ssrc->m_lastTSTOSequenceNumber = sequenceNumber;
                  m_connection.ExecuteMediaCommand(OpalTemporalSpatialTradeOff(tradeOff, m_sessionId, targetSSRC), true);
                }
              }
            }
            else {
              PTRACE(2, *this << "TSTO packet truncated - " << frame);
            }
            break;
          }
#endif

          case RTP_ControlFrame::e_ApplicationLayerFbMessage:
          {
            RTP_SyncSourceId senderSSRC;
            RTP_SyncSourceArray targetSSRCs;
            unsigned maxBitRate;
            if (frame.ParseREMB(senderSSRC, targetSSRCs, maxBitRate)) {
              SyncSource * ssrc;
              if (CheckControlSSRC(senderSSRC, targetSSRCs[0], ssrc PTRACE_PARAM(,"REMB"))) {
                PTRACE(4, *this << "received REMB:"
                       " maxBitRate=" << maxBitRate << ","
                       " first receiver SSRC=" << RTP_TRACE_SRC(targetSSRCs[0]) << ","
                       " sender SSRC=" << RTP_TRACE_SRC(senderSSRC));
                m_connection.ExecuteMediaCommand(OpalMediaFlowControl(maxBitRate, m_mediaType, m_sessionId, targetSSRCs), true);
              }
              break;
            }
          }

          default :
            PTRACE(2, *this << "unknown Payload Specific feedback type: " << frame.GetFbType());
        }
        break;
  
      default :
        PTRACE(2, *this << "unknown control payload type: " << frame.GetPayloadType());
    }
  } while (frame.ReadNextPacket());

  return e_ProcessPacket;
}


bool OpalRTPSession::OnReceiveExtendedReports(const RTP_ControlFrame & frame)
{
  size_t size = frame.GetPayloadSize();
  if (size < sizeof(PUInt32b))
    return false;

  const BYTE * payload = frame.GetPayloadPtr();
  RTP_SyncSourceId ssrc = *(const PUInt32b *)payload;
  payload += sizeof(PUInt32b);
  size -= sizeof(PUInt32b);

  while (size >= sizeof(RTP_ControlFrame::ExtendedReport)) {
    const RTP_ControlFrame::ExtendedReport & xr = *(const RTP_ControlFrame::ExtendedReport *)payload;
    size_t blockSize = xr.length*4 + sizeof(RTP_ControlFrame::ExtendedReport);
    if (size < blockSize)
      return false;

    switch (xr.bt) {
      case 4 :
        if (blockSize < sizeof(RTP_ControlFrame::ReceiverReferenceTimeReport))
          return false;
        else {
          PTime ntp(0);
          ntp.SetNTP(((const RTP_ControlFrame::ReceiverReferenceTimeReport *)payload)->ntp);
          OnRxReceiverReferenceTimeReport(ssrc, ntp);
        }
        break;

      case 5 :
        if (blockSize < sizeof(RTP_ControlFrame::DelayLastReceiverReport))
          return false;
        else {
          PINDEX count = (blockSize - sizeof(RTP_ControlFrame::ExtendedReport)) / sizeof(RTP_ControlFrame::DelayLastReceiverReport::Receiver);
          if (blockSize != count*sizeof(RTP_ControlFrame::DelayLastReceiverReport::Receiver)+sizeof(RTP_ControlFrame::ExtendedReport))
            return false;

          RTP_ControlFrame::DelayLastReceiverReport * dlrr = (RTP_ControlFrame::DelayLastReceiverReport *)payload;
          for (PINDEX i = 0; i < count; ++i)
            OnRxDelayLastReceiverReport(dlrr->m_receiver[i]);
        }
        break;

#if OPAL_RTCP_XR
      case 7:
        if (blockSize < sizeof(RTP_ControlFrame::MetricsReport))
          return false;

        OnRxMetricsReport(ssrc, *(const RTP_ControlFrame::MetricsReport *)payload);
        break;
#endif

      default :
        PTRACE(4, *this << "unknown extended report: code=" << (unsigned)xr.bt << " length=" << blockSize);
    }

    payload += blockSize;
    size -= blockSize;
  }

  return size == 0;
}


void OpalRTPSession::OnRxReceiverReferenceTimeReport(RTP_SyncSourceId ssrc, const PTime & ntp)
{
  SyncSource * receiver = UseSyncSource(ssrc, e_Receiver, true);
  if (receiver != NULL) {
    receiver->m_referenceReportNTP = ntp;
    receiver->m_referenceReportTime.SetCurrentTime();
    PTRACE(4, *this << "OnRxReceiverReferenceTimeReport: ssrc=" << RTP_TRACE_SRC(ssrc) << " ntp=" << ntp.AsString("hh:m:ss.uuu"));
  }
}


void OpalRTPSession::OnRxDelayLastReceiverReport(const RTP_DelayLastReceiverReport & dlrr)
{
  SyncSource * receiver;
  if (GetSyncSource(dlrr.m_ssrc, e_Receiver, receiver))
    receiver->OnRxDelayLastReceiverReport(dlrr);
  else {
    PTRACE(4, *this << "OnRxDelayLastReceiverReport: unknown ssrc=" << RTP_TRACE_SRC(dlrr.m_ssrc));
  }
}


void OpalRTPSession::OnRxSenderReport(const RTP_SenderReport & senderReport)
{
  // This is report for their sender, our receiver, don't use GetSyncSource() due to log level noise
  SyncSourceMap::const_iterator it = m_SSRC.find(senderReport.sourceIdentifier);
  if (it != m_SSRC.end())
    it->second->OnRxSenderReport(senderReport);
  else {
    PTRACE(IsGroupMember(GetBundleGroupId()) ? 6 : 2,
           *this << "OnRxSenderReport: unknown SSRC=" << RTP_TRACE_SRC(senderReport.sourceIdentifier));
  }
}


void OpalRTPSession::OnRxReceiverReports(RTP_SyncSourceId ssrc, const RTP_ControlFrame::ReceiverReport * rr, unsigned count)
{
  std::vector<RTP_ReceiverReport> reports;
  for (unsigned i = 0; i < count; ++i) {
    OnRxReceiverReport(ssrc, rr[i]);

    SyncSource * sender = NULL;
    if (CheckControlSSRC(ssrc, rr[i].ssrc, sender PTRACE_PARAM(, "RR"))) {
      RTP_ReceiverReport report(rr[i], sender->m_ntpPassThrough);
      sender->OnRxReceiverReport(report);
      reports.push_back(report);
    }
  }
  OnRxReceiverReports(ssrc, reports);
}


void OpalRTPSession::OnRxReceiverReports(RTP_SyncSourceId ssrc, const std::vector<RTP_ReceiverReport> & reports)
{
  for (unsigned i = 0; i < reports.size(); ++i) {
    OnRxReceiverReport(ssrc, reports[i]);        
  }
}


void OpalRTPSession::OnRxReceiverReport(RTP_SyncSourceId, const RTP_ControlFrame::ReceiverReport &)
{
}


void OpalRTPSession::OnRxReceiverReport(RTP_SyncSourceId, const RTP_ReceiverReport & report)
{
  m_connection.ExecuteMediaCommand(OpalMediaPacketLoss(report.fractionLost * 100 / 255, m_mediaType, m_sessionId, report.sourceIdentifier), true);
}


void OpalRTPSession::OnRxSourceDescription(const RTP_SourceDescriptionArray & PTRACE_PARAM(description))
{
  PTRACE(m_throttleRxSDES, *this << "OnSourceDescription: " << description.GetSize() << " entries" << description);
}


void OpalRTPSession::OnRxGoodbye(const RTP_SyncSourceArray & PTRACE_PARAM(src), const PString & PTRACE_PARAM(reason))
{
#if PTRACING
  if (PTrace::CanTrace(3)) {
    ostream & strm = PTRACE_BEGIN(3);
    strm << *this << "OnGoodbye: " << reason << "\" SSRC=";
    for (size_t i = 0; i < src.size(); i++)
      strm << RTP_TRACE_SRC(src[i]) << ' ';
    strm << PTrace::End;
  }
#endif
}


void OpalRTPSession::OnRxNACK(RTP_SyncSourceId PTRACE_PARAM(ssrc), const RTP_ControlFrame::LostPacketMask PTRACE_PARAM(lostPackets))
{
  PTRACE(3, *this << "OnRxNACK: SSRC=" << RTP_TRACE_SRC(ssrc) << ", sn=" << lostPackets);
}


void OpalRTPSession::OnRxTWCC(const RTP_TransportWideCongestionControl & PTRACE_PARAM(twcc))
{
  PTRACE(5, *this << "OnRxTWCC:"
                     " rtcp-sn=" << twcc.m_rtcpSequenceNumber <<
                     " init-sn=" << twcc.m_packets.begin()->first <<
                     " end-sn=" << twcc.m_packets.rbegin()->first <<
                     " sz=" << twcc.m_packets.size());
}


void OpalRTPSession::OnRxApplDefined(const RTP_ControlFrame::ApplDefinedInfo & info)
{
  PTRACE(3, *this << "OnApplDefined: \""
         << info.m_type << "\"-" << info.m_subType << " " << info.m_SSRC << " [" << info.m_data.GetSize() << ']');
  m_applDefinedNotifiers(*this, info);
}


OpalRTPSession::SendReceiveStatus OpalRTPSession::SendNACK(const RTP_ControlFrame::LostPacketMask & lostPackets, RTP_SyncSourceId syncSourceIn)
{
  if (lostPackets.empty()) {
    PTRACE(5, *this << "no packet loss indicated, not sending NACK");
    return e_IgnorePacket;
  }

  RTP_ControlFrame request;

  {
    PSafeLockReadOnly lock(*this);
    if (!lock.IsLocked())
      return e_AbortTransport;

    if (!(m_feedback&OpalMediaFormat::e_NACK)) {
      PTRACE(3, *this << "remote not capable of NACK");
      return e_IgnorePacket;
    }

    SyncSource * sender;
    if (!GetSyncSource(0, e_Sender, sender))
      return e_ProcessPacket;

    SyncSource * receiver;
    if (!GetSyncSource(syncSourceIn, e_Receiver, receiver))
      return e_IgnorePacket;

    // Packet always starts with SR or RR, use empty RR as place holder
    InitialiseControlFrame(request, *sender);

    PTRACE(4, *this << "sending NACK, "
              "SSRC=" << RTP_TRACE_SRC(receiver->m_sourceIdentifier) << ", "
              "lost=" << lostPackets);

    request.AddNACK(sender->m_sourceIdentifier, receiver->m_sourceIdentifier, lostPackets);
    ++receiver->m_NACKs;
  }

  // Send it
  request.EndPacket();
  return WriteControl(request);
}


OpalRTPSession::SendReceiveStatus OpalRTPSession::SendTWCC(const RTP_TransportWideCongestionControl & twcc)
{
  if (twcc.m_packets.empty())
    return e_IgnorePacket;

  RTP_ControlFrame request;

  {
    PSafeLockReadOnly lock(*this);
    if (!lock.IsLocked())
      return e_AbortTransport;

    if (m_transportWideSeqNumHdrExtId > RTP_DataFrame::MaxHeaderExtensionIdOneByte) {
      PTRACE(3, *this << "remote not capable of TWCC");
      return e_IgnorePacket;
    }

    SyncSource * sender;
    if (!GetSyncSource(0, e_Sender, sender))
      return e_ProcessPacket;

    // Packet always starts with SR or RR, use empty RR as place holder
    InitialiseControlFrame(request, *sender);

    PTRACE(5, *this << "sending TWCC RTCP command:"
                       " ssrc=" << RTP_TRACE_SRC(sender->m_sourceIdentifier) <<
                       " rtcp-sn=" << twcc.m_rtcpSequenceNumber <<
                       " init-sn=" << twcc.m_packets.begin()->first <<
                       " end-sn=" << twcc.m_packets.rbegin()->first <<
                       " sz=" << twcc.m_packets.size());
    request.AddTWCC(sender->m_sourceIdentifier, twcc);
  }

  // Send it
  request.EndPacket();
  return WriteControl(request);
}


OpalRTPSession::SendReceiveStatus OpalRTPSession::SendFlowControl(unsigned maxBitRate, unsigned overhead, bool notify, RTP_SyncSourceId syncSourceIn)
{
  RTP_ControlFrame request;

  {
    PSafeLockReadOnly lock(*this);
    if (!lock.IsLocked())
      return e_AbortTransport;

    if (!(m_feedback&(OpalMediaFormat::e_TMMBR | OpalMediaFormat::e_REMB))) {
      PTRACE(3, *this << "remote not capable of flow control (TMMBR or REMB)");
      return e_ProcessPacket;
    }

    SyncSource * sender;
    if (!GetSyncSource(0, e_Sender, sender))
      return e_ProcessPacket;

    SyncSource * receiver;
    if (!GetSyncSource(syncSourceIn, e_Receiver, receiver))
      return e_ProcessPacket;

    // Packet always starts with SR or RR, use empty RR as place holder
    InitialiseControlFrame(request, *sender);

    if (m_feedback&OpalMediaFormat::e_TMMBR) {
      if (overhead == 0)
        overhead = m_packetOverhead;

      PTRACE(3, *this << "sending TMMBR (flow control) "
             "rate=" << maxBitRate << ", overhead=" << overhead << ", "
             "SSRC=" << RTP_TRACE_SRC(receiver->m_sourceIdentifier));

      request.AddTMMB(sender->m_sourceIdentifier, receiver->m_sourceIdentifier, maxBitRate, overhead, notify);
    }
    else {
      PTRACE(3, *this << "sending REMB (flow control) "
             "rate=" << maxBitRate << ", SSRC=" << RTP_TRACE_SRC(receiver->m_sourceIdentifier));

      request.AddREMB(sender->m_sourceIdentifier, receiver->m_sourceIdentifier, maxBitRate);
    }
  }

  // Send it
  request.EndPacket();
  return WriteControl(request);
}


#if OPAL_VIDEO

OpalRTPSession::SendReceiveStatus OpalRTPSession::SendIntraFrameRequest(unsigned options, RTP_SyncSourceId syncSourceIn)
{
  RTP_ControlFrame request;

  {
    PSafeLockReadOnly lock(*this);
    if (!lock.IsLocked())
      return e_AbortTransport;

    SyncSource * sender;
    if (!GetSyncSource(0, e_Sender, sender))
      return e_ProcessPacket;

    SyncSource * receiver;
    if (!GetSyncSource(syncSourceIn, e_Receiver, receiver))
      return e_ProcessPacket;

    // Packet always starts with SR or RR, use empty RR as place holder
    InitialiseControlFrame(request, *sender);

    bool has_AVPF_PLI = (m_feedback & OpalMediaFormat::e_PLI) || (options & OPAL_OPT_VIDUP_METHOD_PLI);
    bool has_AVPF_FIR = (m_feedback & OpalMediaFormat::e_FIR) || (options & OPAL_OPT_VIDUP_METHOD_FIR);

    if ((has_AVPF_PLI && !has_AVPF_FIR) || (has_AVPF_PLI && (options & OPAL_OPT_VIDUP_METHOD_PREFER_PLI))) {
      PTRACE(3, *this << "sending RFC4585 PLI"
             << ((options & OPAL_OPT_VIDUP_METHOD_PLI) ? " (forced)" : "")
             << ", SSRC=" << RTP_TRACE_SRC(receiver->m_sourceIdentifier));
      request.AddPLI(sender->m_sourceIdentifier, receiver->m_sourceIdentifier);
    }
    else if (has_AVPF_FIR) {
      PTRACE(3, *this << "sending RFC5104 FIR"
             << ((options & OPAL_OPT_VIDUP_METHOD_FIR) ? " (forced)" : "")
             << ", SSRC=" << RTP_TRACE_SRC(receiver->m_sourceIdentifier));
      request.AddFIR(sender->m_sourceIdentifier, receiver->m_sourceIdentifier, receiver->m_lastFIRSequenceNumber++);
    }
    else {
      PTRACE(3, *this << "sending RFC2032, SSRC=" << RTP_TRACE_SRC(syncSourceIn));
      request.AddIFR(receiver->m_sourceIdentifier);
    }
  }

  // Send it
  request.EndPacket();
  return WriteControl(request);
}


OpalRTPSession::SendReceiveStatus OpalRTPSession::SendTemporalSpatialTradeOff(unsigned tradeOff, RTP_SyncSourceId syncSourceIn)
{
  RTP_ControlFrame request;

  {
    PSafeLockReadOnly lock(*this);
    if (!lock.IsLocked())
      return e_AbortTransport;

    if (!(m_feedback&OpalMediaFormat::e_TSTR)) {
      PTRACE(3, *this << "remote not capable of Temporal/Spatial Tradeoff (TSTR)");
      return e_ProcessPacket;
    }

    SyncSource * sender;
    if (!GetSyncSource(0, e_Sender, sender))
      return e_ProcessPacket;

    SyncSource * receiver;
    if (!GetSyncSource(syncSourceIn, e_Receiver, receiver))
      return e_ProcessPacket;

    // Packet always starts with SR or RR, use empty RR as place holder
    InitialiseControlFrame(request, *sender);

    PTRACE(3, *this << "sending TSTO (temporal spatial trade off) "
           "value=" << tradeOff << ", SSRC=" << RTP_TRACE_SRC(receiver->m_sourceIdentifier));

    request.AddTSTO(sender->m_sourceIdentifier, receiver->m_sourceIdentifier, tradeOff, sender->m_lastTSTOSequenceNumber++);
  }

  // Send it
  request.EndPacket();
  return WriteControl(request);
}

#endif // OPAL_VIDEO


void OpalRTPSession::AddDataNotifier(unsigned priority, const DataNotifier & notifier, RTP_SyncSourceId ssrc)
{
  PSafeLockReadWrite lock(*this);
  if (!lock.IsLocked())
    return;

  if (ssrc != 0) {
    SyncSource * receiver;
    if (GetSyncSource(ssrc, e_Receiver, receiver))
      receiver->m_notifiers.Add(priority, notifier);
  }
  else {
    for (SyncSourceMap::iterator it = m_SSRC.begin(); it != m_SSRC.end(); ++it)
      it->second->m_notifiers.Add(priority, notifier);
    m_notifiers.Add(priority, notifier);
  }
}


void OpalRTPSession::RemoveDataNotifier(const DataNotifier & notifier, RTP_SyncSourceId ssrc)
{
  PSafeLockReadWrite lock(*this);
  if (!lock.IsLocked())
    return;

  if (ssrc != 0) {
    SyncSource * receiver;
    if (GetSyncSource(ssrc, e_Receiver, receiver))
      receiver->m_notifiers.Remove(notifier);
  }
  else {
    for (SyncSourceMap::iterator it = m_SSRC.begin(); it != m_SSRC.end(); ++it)
      it->second->m_notifiers.Remove(notifier);
    m_notifiers.Remove(notifier);
  }
}


void OpalRTPSession::NotifierMap::Add(unsigned priority, const DataNotifier & notifier)
{
  Remove(notifier);
  insert(make_pair(priority, notifier));
}


void OpalRTPSession::NotifierMap::Remove(const DataNotifier & notifier)
{
  NotifierMap::iterator it = begin();
  while (it != end()) {
    if (it->second != notifier)
      ++it;
    else
      erase(it++);
  }
}


void OpalRTPSession::SetJitterBuffer(OpalJitterBuffer * jitterBuffer, RTP_SyncSourceId ssrc)
{
  PSafeLockReadWrite lock(*this);
  if (!lock.IsLocked())
    return;

  if (ssrc == 0)
    m_jitterBuffer = jitterBuffer;
  else {
    SyncSource * receiver = NULL;
    if (!GetSyncSource(ssrc, e_Receiver, receiver)) {
      PTRACE(2, "Could not change jitter buffer on SSRC=" << RTP_TRACE_SRC(ssrc));
      return;
    }
    receiver->m_jitterBuffer = jitterBuffer;
  }

#if PTRACING
  static unsigned const Level = 3;
  if (PTrace::CanTrace(Level)) {
    ostream & trace = PTRACE_BEGIN(Level);
    trace << *this;
    if (jitterBuffer != NULL)
      trace << "attached jitter buffer " << *jitterBuffer << " to";
    else
      trace << "detached jitter buffer from";
    if (ssrc != 0)
      trace << " SSRC=" << RTP_TRACE_SRC(ssrc);
    else
      trace << " all receivers";
    trace << PTrace::End;
  }
#endif
}


/////////////////////////////////////////////////////////////////////////////

bool OpalRTPSession::UpdateMediaFormat(const OpalMediaFormat & mediaFormat)
{
  PSafeLockReadWrite lock(*this);
  if (!lock.IsLocked())
    return false;

  if (!OpalMediaSession::UpdateMediaFormat(mediaFormat))
    return false;

  m_timeUnits = mediaFormat.GetTimeUnits();
  m_feedback = mediaFormat.GetOptionEnum(OpalMediaFormat::RTCPFeedbackOption(), OpalMediaFormat::e_NoRTCPFb);

  unsigned maxBitRate = mediaFormat.GetMaxBandwidth();
  if (maxBitRate != 0) {
    unsigned overheadBits = m_packetOverhead*8;

    unsigned frameSize = mediaFormat.GetFrameSize();
    if (frameSize == 0)
      frameSize = m_manager.GetMaxRtpPayloadSize();

    unsigned packetSize = frameSize*mediaFormat.GetOptionInteger(OpalAudioFormat::RxFramesPerPacketOption(), 1);

    m_qos.m_receive.m_maxPacketSize = packetSize + m_packetOverhead;
    packetSize *= 8;
    m_qos.m_receive.m_maxBandwidth = maxBitRate + (maxBitRate+packetSize-1)/packetSize * overheadBits;

    maxBitRate = mediaFormat.GetOptionInteger(OpalMediaFormat::TargetBitRateOption(), maxBitRate);
    packetSize = frameSize*mediaFormat.GetOptionInteger(OpalAudioFormat::TxFramesPerPacketOption(), 1);

    m_qos.m_transmit.m_maxPacketSize = packetSize + m_packetOverhead;
    packetSize *= 8;
    m_qos.m_transmit.m_maxBandwidth = maxBitRate + (maxBitRate+packetSize-1)/packetSize * overheadBits;
  }

  // Audio has tighter constraints to video
  if (m_isAudio) {
    m_qos.m_transmit.m_maxLatency = m_qos.m_receive.m_maxLatency = 250000; // 250ms
    m_qos.m_transmit.m_maxJitter = m_qos.m_receive.m_maxJitter = 100000; // 100ms
  }
  else {
    m_qos.m_transmit.m_maxLatency = m_qos.m_receive.m_maxLatency = 750000; // 750ms
    m_qos.m_transmit.m_maxJitter = m_qos.m_receive.m_maxJitter = 250000; // 250ms
  }

  SetQoS(m_qos);

  PTRACE(4, *this << "updated media format " << mediaFormat << ": timeUnits=" << m_timeUnits << " maxBitRate=" << maxBitRate << ", feedback=" << m_feedback);
  return true;
}


OpalMediaStream * OpalRTPSession::CreateMediaStream(const OpalMediaFormat & mediaFormat, 
                                                    unsigned sessionId, 
                                                    bool isSource)
{
  if (PAssert(m_sessionId == sessionId && m_mediaType == mediaFormat.GetMediaType(), PLogicError) && UpdateMediaFormat(mediaFormat))
    return new OpalRTPMediaStream(dynamic_cast<OpalRTPConnection &>(m_connection), mediaFormat, isSource, *this);

  return NULL;
}


OpalMediaTransport * OpalRTPSession::CreateMediaTransport(const PString & name)
{
#if OPAL_ICE
  return new OpalICEMediaTransport(name);
#else
  return new OpalUDPMediaTransport(name);
#endif
}


bool OpalRTPSession::Open(const PString & localInterface, const OpalTransportAddress & remoteAddress)
{
  if (IsOpen())
    return true;

  PSafeLockReadWrite lock(*this);
  if (!lock.IsLocked())
    return false;

  PString transportName("RTP ");
  if (IsGroupMember(GetBundleGroupId()))
    transportName += "bundle";
  else
    transportName.sprintf("Session %u", m_sessionId);
  OpalMediaTransportPtr transport = CreateMediaTransport(transportName);
  PTRACE_CONTEXT_ID_TO(*transport);

  if (!transport->Open(*this, m_singlePortRx ? 1 : 2, localInterface, remoteAddress))
    return false;

  InternalAttachTransport(transport PTRACE_PARAM(, "opened"));
  return true;
}


bool OpalRTPSession::SetQoS(const PIPSocket::QoS & qos)
{
  OpalMediaTransportPtr transport = m_transport; // This way avoids races
  if (transport == NULL || !transport->IsOpen())
    return false;

  PIPSocket * socket = dynamic_cast<PIPSocket *>(transport->GetChannel(e_Data));
  if (socket == NULL)
    return false;

  PSafeLockReadOnly lock(*this);
  if (!lock.IsLocked())
    return false;

  PIPAddress remoteAddress;
  WORD remotePort;
  transport->GetRemoteAddress().GetIpAndPort(remoteAddress, remotePort);

  m_qos = qos;
  m_qos.m_remote.SetAddress(remoteAddress, remotePort);
  return socket->SetQoS(m_qos);
}


bool OpalRTPSession::Close()
{
  PTRACE(3, *this << "closing RTP.");

  m_reportTimer.Stop(true);
  m_endpoint.RegisterLocalRTP(this, true);

  if (IsOpen() && LockReadOnly()) {
    for (SyncSourceMap::iterator it = m_SSRC.begin(); it != m_SSRC.end(); ++it) {
      if ( it->second->m_direction == e_Sender &&
           it->second->m_packets > 0 &&
           it->second->SendBYE() == e_AbortTransport)
        break;
    }
    UnlockReadOnly();
  }

  return OpalMediaSession::Close();
}


PString OpalRTPSession::GetLocalHostName()
{
  return PIPSocket::GetHostName();
}


void OpalRTPSession::SetSinglePortRx(bool singlePortRx)
{
  PTRACE_IF(3, m_singlePortRx != singlePortRx, *this << (singlePortRx ? "enable" : "disable") << " single port mode for receive");
  m_singlePortRx = singlePortRx;
}


void OpalRTPSession::SetSinglePortTx(bool singlePortTx)
{
  PTRACE_IF(3, m_singlePortTx != singlePortTx, *this << (singlePortTx ? "enable" : "disable") << " single port mode for transmit");

  m_singlePortTx = singlePortTx;

  OpalMediaTransportPtr transport = m_transport; // This way avoids races
  if (transport == NULL)
    return;

  OpalTransportAddress remoteDataAddress = transport->GetRemoteAddress(e_Data);
  if (singlePortTx)
    transport->SetRemoteAddress(remoteDataAddress, e_Control);
  else {
    PIPAddressAndPort ap;
    remoteDataAddress.GetIpAndPort(ap);
    ap.SetPort(ap.GetPort()+1);
    transport->SetRemoteAddress(ap, e_Control);
  }
}


WORD OpalRTPSession::GetLocalDataPort() const
{
  OpalMediaTransportPtr transport = m_transport; // This way avoids races
  OpalUDPMediaTransport * udp = transport != NULL ? dynamic_cast<OpalUDPMediaTransport *>(&*transport) : NULL;
  PUDPSocket * socket = udp != NULL ? udp->GetSubChannelAsSocket(e_Media) : NULL;
  return socket != NULL ? socket->GetPort() : 0;
}


WORD OpalRTPSession::GetLocalControlPort() const
{
  OpalMediaTransportPtr transport = m_transport; // This way avoids races
  OpalUDPMediaTransport * udp = transport != NULL ? dynamic_cast<OpalUDPMediaTransport *>(&*transport) : NULL;
  PUDPSocket * socket = udp != NULL ? udp->GetSubChannelAsSocket(e_Control) : NULL;
  return socket != NULL ? socket->GetPort() : 0;
}


PUDPSocket & OpalRTPSession::GetDataSocket()
{
  OpalMediaTransportPtr transport = m_transport; // This way avoids races
  OpalUDPMediaTransport * udp = transport != NULL ? dynamic_cast<OpalUDPMediaTransport *>(&*transport) : NULL;
  return *PAssertNULL(udp)->GetSubChannelAsSocket(e_Media);
}


PUDPSocket & OpalRTPSession::GetControlSocket()
{
  OpalMediaTransportPtr transport = m_transport; // This way avoids races
  OpalUDPMediaTransport * udp = transport != NULL ? dynamic_cast<OpalUDPMediaTransport *>(&*transport) : NULL;
  return *PAssertNULL(udp)->GetSubChannelAsSocket(e_Control);
}


OpalTransportAddress OpalRTPSession::GetLocalAddress(bool isMediaAddress) const
{
  return OpalMediaSession::GetLocalAddress(isMediaAddress || m_singlePortRx);
}


OpalTransportAddress OpalRTPSession::GetRemoteAddress(bool isMediaAddress) const
{
  if (!m_singlePortRx)
    return OpalMediaSession::GetRemoteAddress(isMediaAddress);

  if (m_singlePortTx || m_remoteControlPort == 0)
    return OpalMediaSession::GetRemoteAddress(true);

  PIPSocketAddressAndPort remote;
  OpalMediaSession::GetRemoteAddress().GetIpAndPort(remote);
  remote.SetPort(m_remoteControlPort);
  return OpalTransportAddress(remote, OpalTransportAddress::UdpPrefix());
}


bool OpalRTPSession::SetRemoteAddress(const OpalTransportAddress & remoteAddress, bool isMediaAddress)
{
  if (!OpalMediaSession::SetRemoteAddress(remoteAddress, isMediaAddress))
    return false;

  OpalMediaTransportPtr transport = m_transport; // This way avoids races
  if (transport == NULL)
    return false;

  SubChannels otherChannel = isMediaAddress ? e_Control : e_Data;
  if (transport->GetRemoteAddress(otherChannel).IsEmpty()) {
    PTRACE(3, *this << "set remote " << otherChannel << " port: "
           "singlePortTx=" << boolalpha << m_singlePortTx << " other=" << remoteAddress);
    if (m_singlePortTx)
      transport->SetRemoteAddress(remoteAddress, otherChannel);
    else {
      PIPAddressAndPort ap;
      remoteAddress.GetIpAndPort(ap);
      ap.SetPort(ap.GetPort() + (isMediaAddress ? 1 : -1));
      transport->SetRemoteAddress(ap, otherChannel);
    }
  }

  PIPAddress dummy(0);
  transport->GetRemoteAddress(e_Control).GetIpAndPort(dummy, m_remoteControlPort);
  return true;
}


void OpalRTPSession::OnRxDataPacket(OpalMediaTransport &, PBYTEArray data)
{
  PSafeLockReadWrite lock(*this);
  if (!lock.IsLocked())
    return;

  if (data.IsEmpty()) {
    CheckMediaFailed(e_Data);
    return;
  }

  if (m_sendEstablished && IsEstablished()) {
    m_sendEstablished = false;
    m_manager.QueueDecoupledEvent(new PSafeWorkNoArg<OpalConnection, bool>(&m_connection, &OpalConnection::InternalOnEstablished));
  }

  // Check for single port operation, incoming RTCP on RTP
  RTP_ControlFrame control(data, data.GetSize(), false);
  unsigned type = control.GetPayloadType();
  if (type >= RTP_ControlFrame::e_FirstValidPayloadType && type <= RTP_ControlFrame::e_LastValidPayloadType) {
    if (OnReceiveControl(control) == e_AbortTransport)
      CheckMediaFailed(e_Control);
  }
  else {
    RTP_DataFrame frame(data);
    if (OnPreReceiveData(frame) == e_AbortTransport)
      CheckMediaFailed(e_Data);
  }
}


void OpalRTPSession::OnRxControlPacket(OpalMediaTransport &, PBYTEArray data)
{
  PSafeLockReadWrite lock(*this);
  if (!lock.IsLocked())
    return;

  if (data.IsEmpty()) {
    CheckMediaFailed(e_Control);
    return;
  }

  RTP_ControlFrame control(data, data.GetSize(), false);
  if (control.IsValid()) {
    if (OnReceiveControl(control) == e_AbortTransport)
      CheckMediaFailed(e_Control);
  }
  else {
    PTRACE_IF(2, data.GetSize() > 1 || m_rtcpPacketsReceived > 0,
              *this << "received control packet invalid: " << data.GetSize() << " bytes");
  }
}


OpalRTPSession::SendReceiveStatus OpalRTPSession::WriteData(RTP_DataFrame & frame, RewriteMode rewrite, const PIPSocketAddressAndPort * remote)
{
  /* Note, copy to local safe pointer before the lock, so if is closed and
     destroyed via this pointer going out of scope, we avoid a deadlock as
     it is dstroyed after the lock is released. */
  OpalMediaTransportPtr transport = m_transport; // This way avoids races
  if (transport == NULL) {
    PTRACE(2, *this << "could not write data frame, no transport");
    return e_AbortTransport;
  }

  if (!transport->IsEstablished())
    return e_IgnorePacket;

  if (!LockReadWrite())
    return e_AbortTransport;

  SendReceiveStatus status = OnSendData(frame, rewrite);

  UnlockReadWrite();

  switch (status) {
    case e_IgnorePacket :
      return e_IgnorePacket;

    case e_ProcessPacket :
      if (transport->Write(frame.GetPointer(), frame.GetPacketSize(), e_Data, remote))
        return e_ProcessPacket;

      // Do abort case
    default :
      break;
  }

  CheckMediaFailed(e_Data);
  return e_AbortTransport;
}


OpalRTPSession::SendReceiveStatus OpalRTPSession::WriteControl(RTP_ControlFrame & frame, const PIPSocketAddressAndPort * remote)
{
  /* Note, copy to local safe pointer before the lock, so if is closed and
     destroyed via this pointer going out of scope, we avoid a deadlock as
     it is dstroyed after the lock is released. */
  OpalMediaTransportPtr transport = m_transport;
  if (transport == NULL) {
    PTRACE(2, *this << "could not write control frame, no transport");
    return e_AbortTransport;
  }

  if (!transport->IsEstablished())
    return e_IgnorePacket;

  PTRACE(6, *this << "Writing control packet:\n" << frame);

  if (!LockReadWrite())
    return e_AbortTransport;

  SendReceiveStatus status = OnSendControl(frame);

  UnlockReadWrite();

  switch (status) {
    case e_IgnorePacket :
      return e_IgnorePacket;

    case e_ProcessPacket:
      {
        PIPSocketAddressAndPort remoteRTCP;
        if (remote == NULL && m_singlePortRx && !m_singlePortTx) {
          GetRemoteAddress(false).GetIpAndPort(remoteRTCP);
          remote = &remoteRTCP;
        }

        if (transport->Write(frame.GetPointer(), frame.GetPacketSize(), m_singlePortRx ? e_Data : e_Control, remote))
          return e_ProcessPacket;
      }
      // Do abort case
    default :
      break;
  }

  CheckMediaFailed(e_Control);
  return e_AbortTransport;
}


void OpalRTPSession::CheckMediaFailed(SubChannels subchannel)
{
  PTRACE(4, *this << "media failed for " << subchannel);
  /* Really should test if both data and control fail, but as it is unlikely we would
     get one failed without the other, we don't bother. */
  if (subchannel == e_Data && m_connection.OnMediaFailed(m_sessionId)) {
    PTRACE(2, *this << "aborting transport, queuing close of media session.");
    m_manager.QueueDecoupledEvent(new PSafeWorkNoArg<OpalRTPSession, bool>(this, &OpalRTPSession::Close));
  }
}


/////////////////////////////////////////////////////////////////////////////
