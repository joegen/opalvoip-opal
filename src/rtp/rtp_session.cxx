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
#include <rtp/rtpep.h>
#include <rtp/rtpconn.h>
#include <rtp/rtp_stream.h>
#include <rtp/metrics.h>
#include <codec/vidcodec.h>

#include <ptclib/random.h>
#include <ptclib/pnat.h>
#include <ptclib/cypher.h>
#include <ptclib/pstunsrvr.h>

#include <algorithm>

#include <h323/h323con.h>

#define EXPERIMENTAL_ICE 0

#define RTP_VIDEO_RX_BUFFER_SIZE 0x100000 // 1Mb
#define RTP_AUDIO_RX_BUFFER_SIZE 0x4000   // 16kb
#define RTP_DATA_TX_BUFFER_SIZE  0x2000   // 8kb
#define RTP_CTRL_BUFFER_SIZE     0x1000   // 4kb

static const uint16_t SequenceReorderThreshold = (1<<16)-100;  // As per RFC3550 RTP_SEQ_MOD - MAX_MISORDER
static const uint16_t SequenceRestartThreshold = 3000;         // As per RFC3550 MAX_DROPOUT


#if OPAL_ICE

class OpalRTPSession::ICEServer : public PSTUNServer
{
  PCLASSINFO(ICEServer, PSTUNServer);
public:
  ICEServer(OpalRTPSession & session) : m_session(session) { }
  virtual void OnBindingResponse(const PSTUNMessage & request, PSTUNMessage & response);

protected:
  OpalRTPSession & m_session;
};

#endif


enum { JitterRoundingGuardBits = 4 };

#if PTRACING
ostream & operator<<(ostream & strm, OpalRTPSession::Direction dir)
{
  return strm << (dir == OpalRTPSession::e_Receiver ? "receiver" : "sender");
}

ostream & operator<<(ostream & strm, OpalRTPSession::Channel chan)
{
  return strm << (chan == OpalRTPSession::e_Data ? "Data" : "Control");
}
#endif

#define PTraceModule() "RTP"

#define new PNEW

/////////////////////////////////////////////////////////////////////////////

const PCaselessString & OpalRTPSession::RTP_AVP () { static const PConstCaselessString s("RTP/AVP" ); return s; }
const PCaselessString & OpalRTPSession::RTP_AVPF() { static const PConstCaselessString s("RTP/AVPF"); return s; }

#if OPAL_PERFORMANCE_CHECK_HACK
  unsigned OpalRTPSession::m_readPerformanceCheckHack = INT_MAX;
  PTRACE_THROTTLE_STATIC(s_readPerformanceCheckHackThrottle,3,1000);
  #define READ_PERFORMANCE_HACK(n,r) \
    if (m_readPerformanceCheckHack <= n) { \
      PTRACE(s_readPerformanceCheckHackThrottle, NULL, "PerfHack", "Read short circuit " << n); \
      return r; \
    }
  unsigned OpalRTPSession::m_writePerformanceCheckHack = INT_MAX;
  PTRACE_THROTTLE_STATIC(s_writePerformanceCheckHackThrottle,3,1000);
  #define WRITE_PERFORMANCE_HACK(n,r) \
    if (m_writePerformanceCheckHack <= n) { \
      PTRACE(s_writePerformanceCheckHackThrottle, NULL, "PerfHack", "Write short circuit " << n); \
      return r; \
    }
#else
  #define READ_PERFORMANCE_HACK(n,r)
  #define WRITE_PERFORMANCE_HACK(n,r)
#endif

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
  , m_isAudio(init.m_mediaType == OpalMediaType::Audio())
  , m_timeUnits(m_isAudio ? 8 : 90)
  , m_toolName(PProcess::Current().GetName())
  , m_allowAnySyncSource(true)
  , m_maxNoReceiveTime(m_manager.GetNoMediaTimeout())
  , m_maxNoTransmitTime(0, 10)          // Sending data for 10 seconds, ICMP says still not there
  , m_maxOutOfOrderPackets(20)
  , m_waitOutOfOrderTime(GetDefaultOutOfOrderWaitTime(m_isAudio))
  , m_txStatisticsInterval(100)
  , m_rxStatisticsInterval(100)
  , m_feedback(OpalMediaFormat::e_NoRTCPFb)
#if OPAL_RTP_FEC
  , m_redundencyPayloadType(RTP_DataFrame::IllegalPayloadType)
  , m_ulpFecPayloadType(RTP_DataFrame::IllegalPayloadType)
  , m_ulpFecSendLevel(2)
#endif
  , m_dummySyncSource(*this, 0, e_Receiver, "-")
  , m_rtcpPacketsSent(0)
  , m_rtcpPacketsReceived(0)
  , m_roundTripTime(0)
  , m_reportTimer(0, 12)  // Seconds
  , m_localAddress(PIPSocket::GetInvalidAddress())
  , m_remoteAddress(PIPSocket::GetInvalidAddress())
  , m_qos(m_manager.GetMediaQoS(init.m_mediaType))
  , m_thread(NULL)
  , m_localHasRestrictedNAT(false)
  , m_noTransmitErrors(0)
#if OPAL_ICE
  , m_candidateType(e_UnknownCandidates)
  , m_iceServer(NULL)
  , m_stunClient(NULL)
  , m_maxICESetUpTime(0, 5)
#endif

{
  m_localAddress = PIPSocket::GetInvalidAddress();
  m_localPort[e_Data] = m_localPort[e_Control] = 0;
  m_remoteAddress = PIPSocket::GetInvalidAddress();
  m_remotePort[e_Data] = m_remotePort[e_Control] = 0;
  m_socket[e_Data] = m_socket[e_Control] = NULL;

  PTRACE_CONTEXT_ID_TO(m_reportTimer);
  m_reportTimer.SetNotifier(PCREATE_NOTIFIER(TimedSendReport));
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

#if OPAL_ICE
  delete m_iceServer;
  delete m_stunClient;
#endif
}


RTP_SyncSourceId OpalRTPSession::AddSyncSource(RTP_SyncSourceId id, Direction dir, const char * cname)
{
  PSafeLockReadWrite lock(*this);
  if (!lock.IsLocked())
    return 0;

  if (id == 0) {
    do {
      id = PRandom::Number();
    } while (m_SSRC.find(id) != m_SSRC.end());
  }

  SyncSource * ssrc = CreateSyncSource(id, dir, cname);
  if (m_SSRC.insert(SyncSourceMap::value_type(id, ssrc)).second)
    return id;

  PTRACE(2, *this << "could not add SSRC "
         << RTP_TRACE_SRC(id) << ", probable clash with receiver.");
  delete ssrc;
  return 0;
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

  delete it->second;
  m_SSRC.erase(it);
  return true;
}


OpalRTPSession::SyncSource * OpalRTPSession::UseSyncSource(RTP_SyncSourceId ssrc, Direction PTRACE_PARAM(dir), bool force)
{
  SyncSourceMap::iterator it = m_SSRC.find(ssrc);
  if (it != m_SSRC.end())
    return it->second;

  if ((force || m_allowAnySyncSource) && AddSyncSource(ssrc, e_Receiver) == ssrc) {
    PTRACE(4, *this << "automatically added " << dir << " SSRC " << RTP_TRACE_SRC(ssrc));
    return m_SSRC.find(ssrc)->second;
  }

#if PTRACING
  static const unsigned Level = 2;
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

  for (SyncSourceMap::const_iterator it = m_SSRC.begin(); it != m_SSRC.end(); ++it) {
    if (it->second->m_direction == dir)
      ssrcs.push_back(it->first);
  }

  return ssrcs;
}


const OpalRTPSession::SyncSource & OpalRTPSession::GetSyncSource(RTP_SyncSourceId ssrc, Direction dir) const
{
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
  , m_reportTimestamp(0)
  , m_reportAbsoluteTime(0)
  , m_synthesizeAbsTime(true)
  , m_firstPacketTime(0)
  , m_packets(0)
  , m_octets(0)
  , m_senderReports(0)
  , m_NACKs(0)
  , m_packetsLost(0)
  , m_packetsOutOfOrder(0)
  , m_packetsTooLate(0)
  , m_averagePacketTime(0)
  , m_maximumPacketTime(0)
  , m_minimumPacketTime(0)
  , m_jitter(0)
  , m_maximumJitter(0)
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
  , m_lastReportTime(0)
  , m_canCalculateRTT(false)
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
               "    total packets        = " << m_packets << "\n"
               "    total octets         = " << m_octets << "\n"
               "    bitRateSent          = " << (8 * m_octets / duration) << "\n"
               "    lostPackets          = " << m_packetsLost << '\n';
    if (m_direction == e_Receiver)
      trace << "    packets too late     = " << (m_jitterBuffer != NULL ? m_jitterBuffer->GetPacketsTooLate() : m_packetsTooLate) << "\n"
               "    packets out of order = " << m_packetsOutOfOrder << '\n';
    trace <<   "    average time         = " << m_averagePacketTime << "\n"
               "    maximum time         = " << m_maximumPacketTime << "\n"
               "    minimum time         = " << m_minimumPacketTime << "\n"
               "    last jitter          = " << m_jitter << "\n"
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
  PTimeInterval lastPacketTick = m_lastPacketTick;

  PTimeInterval tick = PTimer::Tick();
  m_lastPacketTick = tick;
  m_lastPacketAbsTime = frame.GetAbsoluteTime();
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

  unsigned diff = (tick - lastPacketTick).GetInterval();

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
    m_jitter = (m_jitterAccum >> JitterRoundingGuardBits) / m_session.m_timeUnits;
    if (m_jitter > m_maximumJitter)
      m_maximumJitter = m_jitter;
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
      if (m_jitterBuffer != NULL)
        trace << " tooLate=" << m_jitterBuffer->GetPacketsTooLate();
    }
    trace <<     " avgTime=" << m_averagePacketTime <<
                 " maxTime=" << m_maximumPacketTime <<
                 " minTime=" << m_minimumPacketTime <<
                 " jitter=" << m_jitter <<
                 " maxJitter=" << m_maximumJitter
          << PTrace::End;
  }
#endif
}


OpalRTPSession::SendReceiveStatus OpalRTPSession::SyncSource::OnSendData(RTP_DataFrame & frame, RewriteMode rewrite)
{
  PPROFILE_FUNCTION();

  if (rewrite != e_RewriteNothing)
    frame.SetSyncSource(m_sourceIdentifier);

  if (m_packets == 0) {
    if (rewrite == e_RewriteHeader)
      frame.SetSequenceNumber(m_lastSequenceNumber = (RTP_SequenceNumber)PRandom::Number(1, 65535));
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

  WRITE_PERFORMANCE_HACK(4,(++m_packets,e_IgnorePacket))

#if OPAL_RTP_FEC
  if (rewrite != e_RewriteNothing && m_session.GetRedundencyPayloadType() != RTP_DataFrame::IllegalPayloadType) {
    SendReceiveStatus status = OnSendRedundantFrame(frame);
    if (status != e_ProcessPacket)
      return status;
  }
#endif

  WRITE_PERFORMANCE_HACK(5,(++m_packets,e_IgnorePacket))

  // Update absolute time and RTP timestamp for next SR that goes out
  if (m_synthesizeAbsTime && !frame.GetAbsoluteTime().IsValid())
      frame.SetAbsoluteTime(PTime());

  PTRACE_IF(3, !m_reportAbsoluteTime.IsValid() && frame.GetAbsoluteTime().IsValid(), &m_session,
            m_session << "sent first RTP with absolute time: " << frame.GetAbsoluteTime().AsString("hh:mm:dd.uuu"));

  m_reportAbsoluteTime = frame.GetAbsoluteTime();
  m_reportTimestamp = frame.GetTimestamp();

  CalculateStatistics(frame);

  WRITE_PERFORMANCE_HACK(6,e_IgnorePacket)

  PTRACE(m_throttleSendData, &m_session, m_session << "sending packet " << setw(1) << frame << m_throttleSendData);
  return e_ProcessPacket;
}


OpalRTPSession::SendReceiveStatus OpalRTPSession::SyncSource::OnReceiveData(RTP_DataFrame & frame, bool newData)
{
  PPROFILE_FUNCTION();

  frame.SetBundleId(m_mediaStreamId);

  RTP_SequenceNumber sequenceNumber = frame.GetSequenceNumber();
  RTP_SequenceNumber expectedSequenceNumber = m_lastSequenceNumber + 1;
  RTP_SequenceNumber sequenceDelta = sequenceNumber - expectedSequenceNumber;

  if (newData && !m_pendingPackets.empty() && sequenceNumber == expectedSequenceNumber) {
    PTRACE(5, &m_session, *this << ", received out of order packet " << sequenceNumber);
    ++m_packetsOutOfOrder; // it arrived after all!
  }

  // Check packet sequence numbers
  if (m_packets == 0) {
    m_firstPacketTime.SetCurrentTime();
    m_lastPacketTick = PTimer::Tick();

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
    PTRACE(3, &m_session, *this << "late out of order packet, got " << sequenceNumber << " expected " << expectedSequenceNumber);
    ++m_packetsTooLate;
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
    PTRACE(2, &m_session, *this << sequenceDelta << " packet(s) missing at " << expectedSequenceNumber << ", processing from " << sequenceNumber);
    SetLastSequenceNumber(sequenceNumber);
    m_consecutiveOutOfOrderPackets = 0;
#if OPAL_RTCP_XR
    if (m_metrics != NULL) m_metrics->OnPacketLost(sequenceDelta);
#endif
  }

  READ_PERFORMANCE_HACK(6,e_ProcessPacket)

  PTime absTime(0);
  if (m_reportAbsoluteTime.IsValid())
    absTime = m_reportAbsoluteTime + PTimeInterval(((int64_t)frame.GetTimestamp() - (int64_t)m_reportTimestamp) / m_session.m_timeUnits);
  frame.SetAbsoluteTime(absTime);

#if OPAL_RTCP_XR
  if (m_metrics != NULL) {
    m_metrics->OnPacketReceived();
    if (m_jitterBuffer != NULL)
      m_metrics->SetJitterDelay(m_jitterBuffer->GetCurrentJitterDelay() / m_session.m_timeUnits);
  }
#endif

  SendReceiveStatus status = m_session.OnReceiveData(frame);

  READ_PERFORMANCE_HACK(7,status)

#if OPAL_RTP_FEC
  if (status == e_ProcessPacket && frame.GetPayloadType() == m_session.m_redundencyPayloadType)
    status = OnReceiveRedundantFrame(frame);
#endif

  READ_PERFORMANCE_HACK(8,status)

  CalculateStatistics(frame);

  // Final user handling of the read frame
  if (status != e_ProcessPacket)
    return status;

  READ_PERFORMANCE_HACK(9,status);

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


bool OpalRTPSession::ResequenceOutOfOrderPackets(SyncSource & ssrc) const
{
  return ssrc.m_jitterBuffer == NULL || ssrc.m_jitterBuffer->GetCurrentJitterDelay() == 0;
}


OpalRTPSession::SendReceiveStatus OpalRTPSession::SyncSource::OnOutOfOrderPacket(RTP_DataFrame & frame)
{
  PPROFILE_FUNCTION();

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
  PPROFILE_FUNCTION();

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

    if (OnReceiveData(resequencedPacket, false) == e_AbortTransport)
      return false;
  }

  return true;
}


void OpalRTPSession::AttachTransport(Transport & transport)
{
  Close();

  transport.DisallowDeleteObjects();

  if (LockReadWrite()) {
    for (int i = 1; i >= 0; --i) {
      PObject * channel = transport.RemoveHead();
      m_socket[i] = dynamic_cast<PUDPSocket *>(channel);
      if (m_socket[i] == NULL)
        delete channel;
      else {
        PTRACE_CONTEXT_ID_TO(m_socket[i]);
        m_socket[i]->GetLocalAddress(m_localAddress, m_localPort[i]);
      }
    }

    UnlockReadWrite();
  }

  m_endpoint.RegisterLocalRTP(this, false);
  transport.AllowDeleteObjects();
}


OpalMediaSession::Transport OpalRTPSession::DetachTransport()
{
  Transport temp;

  m_endpoint.RegisterLocalRTP(this, true);

  InternalStopRead();

  if (LockReadWrite()) {
    for (int i = 1; i >= 0; --i) {
      if (m_socket[i] != NULL) {
        temp.Append(m_socket[i]);
        m_socket[i] = NULL;
      }
    }

    UnlockReadWrite();
  }

  PTRACE_IF(2, temp.IsEmpty(), *this << "detaching transport from closed session.");
  return temp;
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


PString OpalRTPSession::GetGroupId() const
{
  PSafeLockReadOnly lock(*this);
  PString s = m_groupId;
  s.MakeUnique();
  return s;
}


void OpalRTPSession::SetGroupId(const PString & id)
{
  PSafeLockReadWrite lock(*this);
  m_groupId = id;
  m_groupId.MakeUnique();
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
    info->m_mediaStreamId = id;
    info->m_mediaStreamId.MakeUnique();
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


RTPExtensionHeaders OpalRTPSession::GetExtensionHeaders() const
{
  PSafeLockReadOnly lock(*this);
  return m_extensionHeaders;
}


void OpalRTPSession::SetExtensionHeader(const RTPExtensionHeaders & ext)
{
  PSafeLockReadWrite lock(*this);
  m_extensionHeaders = ext;
}


void OpalRTPSession::SyncSource::OnSendReceiverReport(RTP_ControlFrame::ReceiverReport & report, ReportForce force)
{
  if (force != e_Forced && m_packets == 0)
    return;

  report.ssrc = m_sourceIdentifier;

  unsigned lost = m_session.GetPacketsLost();
  if (m_jitterBuffer != NULL)
    lost += m_jitterBuffer->GetPacketsTooLate();
  report.SetLostPackets(lost);

  if (m_extendedSequenceNumber >= m_lastRRSequenceNumber)
    report.fraction = (BYTE)((m_packetsLostSinceLastRR<<8)/(m_extendedSequenceNumber - m_lastRRSequenceNumber + 1));
  else
    report.fraction = 0;
  m_packetsLostSinceLastRR = 0;

  report.last_seq = m_lastRRSequenceNumber;
  m_lastRRSequenceNumber = m_extendedSequenceNumber;

  report.jitter = m_jitterAccum >> JitterRoundingGuardBits; // Allow for rounding protection bits

  // Time remote sent us an SR
  if (m_lastReportTime.IsValid()) {
    report.lsr  = (uint32_t)(m_reportAbsoluteTime.GetNTP() >> 16);
    report.dlsr = (uint32_t)((PTime() - m_lastReportTime).GetMilliSeconds()*65536/1000); // Delay since last received SR
  }
  else {
    report.lsr = 0;
    report.dlsr = 0;
  }

  PTRACE((unsigned)m_session.m_throttleTxReport, &m_session, *this << "sending ReceiverReport:"
            " fraction=" << (unsigned)report.fraction
         << " lost=" << report.GetLostPackets()
         << " last_seq=" << report.last_seq
         << " jitter=" << report.jitter
         << " lsr=" << report.lsr
         << " dlsr=" << report.dlsr);
} 


#if PTRACING
__inline PTimeInterval abs(const PTimeInterval & i) { return i < 0 ? -i : i; }
#endif

void OpalRTPSession::SyncSource::OnRxSenderReport(const RTP_SenderReport & report)
{
  PTime now;
  PTRACE_IF(2, m_reportAbsoluteTime.IsValid() && m_lastReportTime.IsValid() &&
               abs(report.realTimestamp - m_reportAbsoluteTime) > std::max(PTimeInterval(0,10),(now - m_lastReportTime)*2),
            &m_session, m_session << "OnRxSenderReport: remote NTP time jumped by unexpectedly large amount,"
            " was " << m_reportAbsoluteTime.AsString(PTime::LoggingFormat) << ","
            " now " << report.realTimestamp.AsString(PTime::LoggingFormat) << ","
                        " last report " << m_lastReportTime.AsString(PTime::LoggingFormat));
  m_reportAbsoluteTime = report.realTimestamp;
  m_reportTimestamp = report.rtpTimestamp;
  m_lastReportTime = now; // Remember when we received the SR
  m_senderReports++;
}


void OpalRTPSession::SyncSource::OnRxReceiverReport(const ReceiverReport & report)
{
  m_packetsLost = report.totalLost;
  m_jitter = report.jitter/m_session.m_timeUnits;

#if OPAL_RTCP_XR
  if (m_metrics != NULL)
    m_metrics->OnRxSenderReport(report.lastTimestamp, report.delay);
#endif

  if (m_canCalculateRTT) {
    PTimeInterval myDelay = PTime() - m_lastReportTime;
    if (m_session.m_roundTripTime > 0 && myDelay <= report.delay)
      PTRACE(4, &m_session, *this << "not calculating round trip time, RR arrived too soon after SR.");
    else if (myDelay <= report.delay) {
      m_session.m_roundTripTime = 1;
      PTRACE(4, &m_session, *this << "very small round trip time, using 1ms");
    }
    else if (myDelay > 1000) {
      PTRACE(4, &m_session, *this << "very large round trip time, ignoring");
    }
    else {
      m_session.m_roundTripTime = (myDelay - report.delay).GetInterval();
      PTRACE(4, &m_session, *this << "determined round trip time: " << m_session.m_roundTripTime << "ms");
    }
    m_canCalculateRTT = false;
  }
}


OpalRTPSession::SendReceiveStatus OpalRTPSession::OnSendData(RTP_DataFrame & frame, RewriteMode rewrite)
{
  PPROFILE_FUNCTION();

  SendReceiveStatus status;
#if OPAL_ICE
  if ((status = OnSendICE(e_Data)) != e_ProcessPacket)
    return status;
#else
  if (m_remotePort[e_Data] == 0)
    return e_IgnorePacket;
#endif // OPAL_ICE

  WRITE_PERFORMANCE_HACK(2,e_IgnorePacket)

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
            PTRACE(4, *this << "using loopback SSRC " << RTP_TRACE_SRC(ssrc)
                   << " for receiver SSRC " << RTP_TRACE_SRC(syncSource->m_sourceIdentifier));
            break;
          }
        }

        if (it == m_SSRC.end()) {
          if ((ssrc = AddSyncSource(ssrc, e_Sender)) == 0)
            return e_AbortTransport;

          PTRACE(4, *this << "added loopback SSRC " << RTP_TRACE_SRC(ssrc)
                 << " for receiver SSRC " << RTP_TRACE_SRC(syncSource->m_sourceIdentifier));
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
    PTRACE(4, *this << "added sender SSRC " << RTP_TRACE_SRC(ssrc));
    GetSyncSource(ssrc, e_Sender, syncSource);
  }

  WRITE_PERFORMANCE_HACK(3,e_IgnorePacket)

  return syncSource->OnSendData(frame, rewrite);
}


OpalRTPSession::SendReceiveStatus OpalRTPSession::OnSendControl(RTP_ControlFrame &)
{
  ++m_rtcpPacketsSent;

#if OPAL_ICE
  return OnSendICE(e_Control);
#else
  return m_remotePort[e_Control] == 0 ? e_IgnorePacket : e_ProcessPacket;
#endif // OPAL_ICE
}


OpalRTPSession::SendReceiveStatus OpalRTPSession::OnReceiveData(RTP_DataFrame & frame, PINDEX pduSize)
{
  if (pduSize < RTP_DataFrame::MinHeaderSize)
    return e_IgnorePacket; // Non fatal error, just ignore

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

  if (!frame.SetPacketSize(pduSize))
    return e_IgnorePacket; // Non fatal error, just ignore

  READ_PERFORMANCE_HACK(4,e_IgnorePacket)

  SyncSource * receiver = UseSyncSource(frame.GetSyncSource(), e_Receiver, false);
  if (receiver == NULL)
      return e_IgnorePacket;

  frame.SetBundleId(m_groupId);

  READ_PERFORMANCE_HACK(5,e_IgnorePacket)

  return receiver->OnReceiveData(frame, true);
}


OpalRTPSession::SendReceiveStatus OpalRTPSession::OnReceiveData(RTP_DataFrame &)
{
  return e_ProcessPacket;
}


OpalRTPSession::SendReceiveStatus OpalRTPSession::OnOutOfOrderPacket(RTP_DataFrame & frame)
{
  SyncSource * ssrc;
  if (GetSyncSource(frame.GetSyncSource(), e_Receiver, ssrc))
    return ssrc->OnOutOfOrderPacket(frame);

  return e_ProcessPacket;
}


void OpalRTPSession::InitialiseControlFrame(RTP_ControlFrame & report, SyncSource & sender, ReportForce force)
{
  if (force == e_Periodic && sender.m_packets == 0)
    return;

#if PTRACING
  m_throttleTxReport.CanTrace(); // Sneakiness to make sure throttle works
  unsigned logLevel = force > 0 ? 3 : m_throttleTxReport;
#endif

  unsigned receivers = 0;
  for (SyncSourceMap::iterator it = m_SSRC.begin(); it != m_SSRC.end(); ++it) {
    if (it->second->m_direction == e_Receiver && it->second->m_packets > 0)
      ++receivers;
  }

  RTP_ControlFrame::ReceiverReport * rr = NULL;

  // No valid packets sent yet, so only send RR
  if (!sender.m_reportAbsoluteTime.IsValid()) {

    // Send RR as we are not sending data yet
    report.StartNewPacket(RTP_ControlFrame::e_ReceiverReport);

    // if no packets received, put in an empty report
    if (receivers == 0) {
      report.SetPayloadSize(sizeof(PUInt32b));  // length is SSRC 
      report.SetCount(0);

      // add the SSRC to the start of the payload
      *(PUInt32b *)report.GetPayloadPtr() = sender.m_sourceIdentifier;
      PTRACE(logLevel, *this << "sending empty ReceiverReport,"
             " SenderReport SSRC=" << RTP_TRACE_SRC(sender.m_sourceIdentifier) << m_throttleTxReport);
    }
    else {
      report.SetPayloadSize(sizeof(PUInt32b) + receivers*sizeof(RTP_ControlFrame::ReceiverReport));  // length is SSRC of packet sender plus RRs
      report.SetCount(receivers);
      BYTE * payload = report.GetPayloadPtr();

      // add the SSRC to the start of the payload
      *(PUInt32b *)payload = sender.m_sourceIdentifier;

      // add the RR's after the SSRC
      rr = (RTP_ControlFrame::ReceiverReport *)(payload + sizeof(PUInt32b));
      PTRACE(logLevel, *this << "not sending SenderReport for SSRC="
             << RTP_TRACE_SRC(sender.m_sourceIdentifier) << m_throttleTxReport);
    }
  }
  else {
    // send SR and RR
    report.StartNewPacket(RTP_ControlFrame::e_SenderReport);
    report.SetPayloadSize(sizeof(PUInt32b) + sizeof(RTP_ControlFrame::SenderReport) + receivers*sizeof(RTP_ControlFrame::ReceiverReport));  // length is SSRC of packet sender plus SR + RRs
    report.SetCount(receivers);
    BYTE * payload = report.GetPayloadPtr();

    // add the SSRC to the start of the payload
    *(PUInt32b *)payload = sender.m_sourceIdentifier;

    // add the SR after the SSRC
    RTP_ControlFrame::SenderReport * sr = (RTP_ControlFrame::SenderReport *)(payload+sizeof(PUInt32b));
    sr->ntp_ts = sender.m_reportAbsoluteTime.GetNTP();
    sr->rtp_ts = sender.m_reportTimestamp;
    sr->psent  = sender.m_packets;
    sr->osent  = (uint32_t)sender.m_octets;

    PTRACE(logLevel, *this << "sending SenderReport:"
              " SSRC=" << RTP_TRACE_SRC(sender.m_sourceIdentifier)
           << " ntp=0x" << hex << sr->ntp_ts << dec
           << sender.m_reportAbsoluteTime.AsString("(hh:mm:ss.uuu)")
           << " rtp=" << sr->rtp_ts
           << " psent=" << sr->psent
           << " osent=" << sr->osent
           << m_throttleTxReport);

    sender.m_lastReportTime.SetCurrentTime(); // Remember when we last sent SR
    sender.m_canCalculateRTT = true;

    // add the RR's after the SR
    rr = (RTP_ControlFrame::ReceiverReport *)(payload + sizeof(PUInt32b) + sizeof(RTP_ControlFrame::SenderReport));
  }

  if (rr != NULL) {
    for (SyncSourceMap::iterator it = m_SSRC.begin(); it != m_SSRC.end(); ++it) {
      if (it->second->m_direction == e_Receiver)
        it->second->OnSendReceiverReport(*rr++, force);
    }
  }

  report.EndPacket();

  // Add the SDES part to compound RTCP packet
  PTRACE(logLevel, *this << "sending SDES for SSRC="
         << RTP_TRACE_SRC(sender.m_sourceIdentifier) << ": \"" << sender.m_canonicalName << '"');
  report.StartNewPacket(RTP_ControlFrame::e_SourceDescription);

  report.SetCount(0); // will be incremented automatically
  report.StartSourceDescription(sender.m_sourceIdentifier);
  report.AddSourceDescriptionItem(RTP_ControlFrame::e_CNAME, sender.m_canonicalName);
  report.AddSourceDescriptionItem(RTP_ControlFrame::e_TOOL, m_toolName);
  report.EndPacket();
}


void OpalRTPSession::TimedSendReport(PTimer&, P_INT_PTR)
{
  SendReport(0, false);
}


OpalRTPSession::SendReceiveStatus OpalRTPSession::SendReport(RTP_SyncSourceId ssrc, bool force)
{
  if (!LockReadOnly())
    return e_AbortTransport;

  RTP_ControlFrame packet;

  if (ssrc != 0) {
    SyncSource * sender;
    if (GetSyncSource(ssrc, e_Sender, sender))
      InitialiseControlFrame(packet, *sender, force ? e_Forced : e_Periodic);
  }
  else {
    // Have not got anything yet, do nothing
    for (SyncSourceMap::iterator it = m_SSRC.begin(); it != m_SSRC.end(); ++it) {
      if (it->second->m_direction == e_Sender) {
        InitialiseControlFrame(packet, *it->second, force ? e_Forced : e_Periodic);

#if OPAL_RTCP_XR
        //Generate and send RTCP-XR packet
        if (it->second->m_metrics != NULL)
          it->second->m_metrics->InsertExtendedReportPacket(m_sessionId, it->second->m_sourceIdentifier, it->second->m_jitterBuffer, packet);
#endif
      }
    }
  }

  UnlockReadOnly();

  SendReceiveStatus status = e_IgnorePacket;
  if (packet.IsValid())
      status = WriteControl(packet);

  PTRACE(4, *this << "sending " << (force ? "forced" : "periodic")
         << " report for SSRC=" << RTP_TRACE_SRC(ssrc)
         << " valid=" << boolalpha << packet.IsValid()
         << " status=" << status);
  return status;
}


#if OPAL_STATISTICS
void OpalRTPSession::GetStatistics(OpalMediaStatistics & statistics, Direction dir) const
{
  statistics.m_mediaType         = m_mediaType;
  statistics.m_totalBytes        = 0;
  statistics.m_totalPackets      = 0;
  statistics.m_controlPackets    = m_rtcpPacketsReceived;
  statistics.m_NACKs             = 0;
  statistics.m_packetsLost       = 0;
  statistics.m_packetsOutOfOrder = 0;
  statistics.m_packetsTooLate    = 0;
  statistics.m_packetOverruns    = 0;
  statistics.m_minimumPacketTime = 0;
  statistics.m_averagePacketTime = 0;
  statistics.m_maximumPacketTime = 0;
  statistics.m_averageJitter     = 0;
  statistics.m_maximumJitter     = 0;
  statistics.m_roundTripTime     = m_roundTripTime;
  statistics.m_lastPacketTime    = 0;
  statistics.m_lastReportTime    = 0;

  if (statistics.m_SSRC != 0) {
    SyncSource * info;
    if (GetSyncSource(statistics.m_SSRC, dir, info))
      info->GetStatistics(statistics);
    return;
  }

  unsigned count = 0;
  for (SyncSourceMap::const_iterator it = m_SSRC.begin(); it != m_SSRC.end(); ++it) {
    if (it->second->m_direction == dir) {
      OpalMediaStatistics ssrcStats;
      it->second->GetStatistics(ssrcStats);
      if (ssrcStats.m_totalPackets > 0) {
        statistics.m_totalBytes        += ssrcStats.m_totalBytes;
        statistics.m_totalPackets      += ssrcStats.m_totalPackets;
        statistics.m_NACKs             += ssrcStats.m_NACKs;
        statistics.m_packetsLost       += ssrcStats.m_packetsLost;
        statistics.m_packetsOutOfOrder += ssrcStats.m_packetsOutOfOrder;
        statistics.m_packetsTooLate    += ssrcStats.m_packetsTooLate;
        statistics.m_packetOverruns    += ssrcStats.m_packetOverruns;
        statistics.m_minimumPacketTime += ssrcStats.m_minimumPacketTime;
        statistics.m_averagePacketTime += ssrcStats.m_averagePacketTime;
        statistics.m_maximumPacketTime += ssrcStats.m_maximumPacketTime;
        statistics.m_averageJitter     += ssrcStats.m_averageJitter;
        if (statistics.m_lastPacketTime < ssrcStats.m_lastPacketTime)
          statistics.m_lastPacketTime = ssrcStats.m_lastPacketTime;
        if (statistics.m_lastReportTime < ssrcStats.m_lastReportTime)
          statistics.m_lastReportTime = ssrcStats.m_lastReportTime;
        ++count;
      }
    }
  }
  if (count > 1) {
    statistics.m_averagePacketTime /= count;
    statistics.m_averageJitter /= count;
  }
}
#endif


#if OPAL_STATISTICS
void OpalRTPSession::SyncSource::GetStatistics(OpalMediaStatistics & statistics) const
{
  statistics.m_startTime         = m_firstPacketTime;
  statistics.m_totalBytes        = m_octets;
  statistics.m_totalPackets      = m_packets;
  statistics.m_NACKs             = m_NACKs;
  statistics.m_packetsLost       = m_packetsLost;
  statistics.m_packetsOutOfOrder = m_packetsOutOfOrder;
  statistics.m_minimumPacketTime = m_minimumPacketTime;
  statistics.m_averagePacketTime = m_averagePacketTime;
  statistics.m_maximumPacketTime = m_maximumPacketTime;
  statistics.m_averageJitter     = m_jitter;
  statistics.m_maximumJitter     = m_maximumJitter;
  statistics.m_lastPacketTime    = m_lastPacketAbsTime;
  statistics.m_lastReportTime    = m_lastReportTime;

  if (m_jitterBuffer != NULL) {
    statistics.m_packetsTooLate    = m_jitterBuffer->GetPacketsTooLate();
    statistics.m_packetOverruns    = m_jitterBuffer->GetBufferOverruns();
    statistics.m_jitterBufferDelay = m_jitterBuffer->GetCurrentJitterDelay()/m_jitterBuffer->GetTimeUnits();
  }
  else {
    statistics.m_packetsTooLate    = m_packetsTooLate;
    statistics.m_packetOverruns    = 0;
    statistics.m_jitterBufferDelay = 0;
  }
}
#endif // OPAL_STATISTICS


bool OpalRTPSession::CheckControlSSRC(RTP_SyncSourceId PTRACE_PARAM(senderSSRC),
                                      RTP_SyncSourceId targetSSRC,
                                      SyncSource * & info
                                      PTRACE_PARAM(, const char * pduName)) const
{
  PTRACE_IF(4, m_SSRC.find(senderSSRC) == m_SSRC.end(), *this << pduName << " from incorrect SSRC " << RTP_TRACE_SRC(senderSSRC));

  if (GetSyncSource(targetSSRC, e_Sender, info))
    return true;

  PTRACE(2, *this << pduName << " for incorrect SSRC: " << RTP_TRACE_SRC(targetSSRC));
  return false;
}


OpalRTPSession::SendReceiveStatus OpalRTPSession::OnReceiveControl(RTP_ControlFrame & frame)
{
  if (frame.GetPacketSize() == 0)
    return e_IgnorePacket;

  PPROFILE_FUNCTION();

  PTRACE(6, *this << "OnReceiveControl - " << frame);

  ++m_rtcpPacketsReceived;

  m_firstControl = false;

  do {
    switch (frame.GetPayloadType()) {
      case RTP_ControlFrame::e_SenderReport:
      {
        RTP_SenderReport txReport;
        RTP_ReceiverReportArray rxReports;
        if (frame.ParseSenderReport(txReport, rxReports))
          OnRxSenderReport(txReport, rxReports);
        else {
          PTRACE(2, *this << "SenderReport packet truncated - " << frame);
        }
        break;
      }

      case RTP_ControlFrame::e_ReceiverReport:
      {
        RTP_SyncSourceId ssrc;
        RTP_ReceiverReportArray reports;
        if (frame.ParseReceiverReport(ssrc, reports))
          OnRxReceiverReport(ssrc, reports);
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

#if OPAL_RTCP_XR
      case RTP_ControlFrame::e_ExtendedReport:
      {
        RTP_SyncSourceId rxSSRC;
        RTP_ExtendedReportArray reports;
        if (RTCP_XR_Metrics::ParseExtendedReportArray(frame, rxSSRC, reports))
          OnRxExtendedReport(rxSSRC, reports);
        else {
          PTRACE(2, *this << "ReceiverReport packet truncated - " << frame);
        }
        break;
      }
#endif

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
                PTRACE(4, *this << "received TMMBR: rate=" << maxBitRate << ", SSRC=" << RTP_TRACE_SRC(targetSSRC));
                m_connection.ExecuteMediaCommand(OpalMediaFlowControl(maxBitRate, m_mediaType, m_sessionId, targetSSRC), true);
              }
            }
            else {
              PTRACE(2, *this << "TMMB" << (frame.GetFbType() == RTP_ControlFrame::e_TMMBR ? 'R' : 'N') << " packet truncated - " << frame);
            }
            break;
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
                PTRACE(4, *this << "received RFC4585 PLI, SSRC=" << RTP_TRACE_SRC(targetSSRC));
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
                          " sn=" << sequenceNumber << ", last-sn=" << ssrc->m_lastFIRSequenceNumber
                       << ", SSRC=" << RTP_TRACE_SRC(targetSSRC));
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
                PTRACE(4, *this << "received TSTOR: " << ", tradeOff=" << tradeOff
                       << ", sn=" << sequenceNumber << ", last-sn=" << ssrc->m_lastTSTOSequenceNumber
                       << ", SSRC=" << RTP_TRACE_SRC(targetSSRC));
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
            RTP_SyncSourceId senderSSRC, targetSSRC;
            unsigned maxBitRate;
            if (frame.ParseREMB(senderSSRC, targetSSRC, maxBitRate)) {
              SyncSource * ssrc;
              if (CheckControlSSRC(senderSSRC, targetSSRC, ssrc PTRACE_PARAM(,"REMB"))) {
                PTRACE(4, *this << "received REMB: maxBitRate=" << maxBitRate
                       << ", SSRC=" << RTP_TRACE_SRC(targetSSRC));
                m_connection.ExecuteMediaCommand(OpalMediaFlowControl(maxBitRate, m_mediaType, m_sessionId, targetSSRC), true);
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


void OpalRTPSession::OnRxSenderReport(const SenderReport & senderReport, const ReceiverReportArray & receiveReports)
{
#if PTRACING
  if (PTrace::CanTrace(m_throttleRxSR)) {
    ostream & strm = PTRACE_BEGIN(m_throttleRxSR);
    strm << *this << "OnRxSenderReport: " << senderReport << m_throttleRxSR << '\n';
    for (PINDEX i = 0; i < receiveReports.GetSize(); i++)
      strm << "  RR: " << receiveReports[i] << '\n';
    strm << PTrace::End;
  }
#endif

  // This is report for their sender, our receiver
  SyncSource * receiver;
  if (GetSyncSource(senderReport.sourceIdentifier, e_Receiver, receiver))
    receiver->OnRxSenderReport(senderReport);

  OnRxReceiverReports(receiveReports);
}


void OpalRTPSession::OnRxReceiverReport(RTP_SyncSourceId PTRACE_PARAM(src), const ReceiverReportArray & reports)
{
#if PTRACING
  if (PTrace::CanTrace(m_throttleRxRR)) {
    ostream & strm = PTRACE_BEGIN(m_throttleRxRR);
    strm << *this << "OnReceiverReport: SSRC=" << RTP_TRACE_SRC(src) << ", count=" << reports.GetSize() << m_throttleRxRR;
    for (PINDEX i = 0; i < reports.GetSize(); i++)
      strm << "\n  RR: " << reports[i];
    strm << PTrace::End;
  }
#endif
  OnRxReceiverReports(reports);
}


void OpalRTPSession::OnRxReceiverReports(const ReceiverReportArray & reports)
{
  for (PINDEX i = 0; i < reports.GetSize(); i++) {
    // This is report for their receiver, our sender
    SyncSource * sender;
    if (GetSyncSource(reports[i].sourceIdentifier, e_Sender, sender))
      sender->OnRxReceiverReport(reports[i]);
  }
}


void OpalRTPSession::OnRxSourceDescription(const SourceDescriptionArray & PTRACE_PARAM(description))
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
    request.StartNewPacket(RTP_ControlFrame::e_ReceiverReport);
    request.SetPayloadSize(sizeof(PUInt32b));  // length is SSRC
    request.SetCount(0);
    *(PUInt32b *)request.GetPayloadPtr() = sender->m_sourceIdentifier; // add the SSRC to the start of the payload

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
    request.StartNewPacket(RTP_ControlFrame::e_ReceiverReport);
    request.SetPayloadSize(sizeof(PUInt32b));  // length is SSRC
    request.SetCount(0);
    *(PUInt32b *)request.GetPayloadPtr() = sender->m_sourceIdentifier; // add the SSRC to the start of the payload

    if (m_feedback&OpalMediaFormat::e_TMMBR) {
      if (overhead == 0)
        overhead = m_localAddress.GetVersion() == 4 ? (20 + 8 + 12) : (40 + 8 + 12);

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
    request.StartNewPacket(RTP_ControlFrame::e_ReceiverReport);
    request.SetPayloadSize(sizeof(PUInt32b));  // length is SSRC
    request.SetCount(0);
    *(PUInt32b *)request.GetPayloadPtr() = sender->m_sourceIdentifier; // add the SSRC to the start of the payload

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
    request.StartNewPacket(RTP_ControlFrame::e_ReceiverReport);
    request.SetPayloadSize(sizeof(PUInt32b));  // length is SSRC
    request.SetCount(0);
    *(PUInt32b *)request.GetPayloadPtr() = sender->m_sourceIdentifier; // add the SSRC to the start of the payload

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

  SyncSource * receiver;
  if (GetSyncSource(ssrc, e_Receiver, receiver)) {
    receiver->m_jitterBuffer = jitterBuffer;
#if PTRACING
    static unsigned const Level = 4;
    if (PTrace::CanTrace(Level)) {
      ostream & trace = PTRACE_BEGIN(Level);
      trace << *this;
      if (jitterBuffer != NULL)
        trace << "attached jitter buffer " << *jitterBuffer << " to";
      else
        trace << "detached jitter buffer from";
      trace << " SSRC=" << RTP_TRACE_SRC(receiver->m_sourceIdentifier)
            << PTrace::End;
    }
#endif
  }
}


/////////////////////////////////////////////////////////////////////////////

#if PTRACING
#define SetMinBufferSize(sock, bufType, newSize) SetMinBufferSizeFn(sock, bufType, newSize, #bufType)
static void SetMinBufferSizeFn(PUDPSocket & sock, int bufType, int newSize, const char * bufTypeName)
#else
static void SetMinBufferSize(PUDPSocket & sock, int bufType, int newSize)
#endif
{
  int originalSize = 0;
  if (!sock.GetOption(bufType, originalSize)) {
    PTRACE(1, "GetOption(" << sock.GetHandle() << ',' << bufTypeName << ")"
              " failed: " << sock.GetErrorText());
    return;
  }

  // Already big enough
  if (originalSize >= newSize) {
    PTRACE(4, "SetOption(" << sock.GetHandle() << ',' << bufTypeName << ',' << newSize << ")"
              " unecessary, already " << originalSize);
    return;
  }

  for (; newSize >= 1024; newSize -= newSize/10) {
    // Set to new size
    if (!sock.SetOption(bufType, newSize)) {
      PTRACE(1, "SetOption(" << sock.GetHandle() << ',' << bufTypeName << ',' << newSize << ")"
                " failed: " << sock.GetErrorText());
      continue;
    }

    // As some stacks lie about setting the buffer size, we double check.
    int adjustedSize;
    if (!sock.GetOption(bufType, adjustedSize)) {
      PTRACE(1, "GetOption(" << sock.GetHandle() << ',' << bufTypeName << ")"
                " failed: " << sock.GetErrorText());
      return;
    }

    if (adjustedSize >= newSize) {
      PTRACE(4, "SetOption(" << sock.GetHandle() << ',' << bufTypeName << ',' << newSize << ")"
                " succeeded, actually " << adjustedSize);
      return;
    }

    if (adjustedSize > originalSize) {
      PTRACE(4, "SetOption(" << sock.GetHandle() << ',' << bufTypeName << ',' << newSize << ")"
                " clamped to maximum " << adjustedSize);
      return;
    }

    PTRACE(2, "SetOption(" << sock.GetHandle() << ',' << bufTypeName << ',' << newSize << ")"
              " failed, even though it said it succeeded!");
  }
}


OpalTransportAddress OpalRTPSession::GetLocalAddress(bool isMediaAddress) const
{
  PSafeLockReadOnly lock(*this);

  if (m_localAddress.IsValid() && m_localPort[isMediaAddress] != 0)
    return OpalTransportAddress(m_localAddress, m_localPort[isMediaAddress], OpalTransportAddress::UdpPrefix());
  else
    return OpalTransportAddress();
}


OpalTransportAddress OpalRTPSession::GetRemoteAddress(bool isMediaAddress) const
{
  PSafeLockReadOnly lock(*this);

  if (m_remoteAddress.IsValid() && m_remotePort[isMediaAddress] != 0)
    return OpalTransportAddress(m_remoteAddress, m_remotePort[isMediaAddress], OpalTransportAddress::UdpPrefix());
  else
    return OpalTransportAddress();
}


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
    unsigned overheadBytes = m_localAddress.GetVersion() == 4 ? (20+8+12) : (40+8+12);
    unsigned overheadBits = overheadBytes*8;

    unsigned frameSize = mediaFormat.GetFrameSize();
    if (frameSize == 0)
      frameSize = m_manager.GetMaxRtpPayloadSize();

    unsigned packetSize = frameSize*mediaFormat.GetOptionInteger(OpalAudioFormat::RxFramesPerPacketOption(), 1);

    m_qos.m_receive.m_maxPacketSize = packetSize + overheadBytes;
    packetSize *= 8;
    m_qos.m_receive.m_maxBandwidth = maxBitRate + (maxBitRate+packetSize-1)/packetSize * overheadBits;

    maxBitRate = mediaFormat.GetOptionInteger(OpalMediaFormat::TargetBitRateOption(), maxBitRate);
    packetSize = frameSize*mediaFormat.GetOptionInteger(OpalAudioFormat::TxFramesPerPacketOption(), 1);

    m_qos.m_transmit.m_maxPacketSize = packetSize + overheadBytes;
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


bool OpalRTPSession::Open(const PString & localInterface, const OpalTransportAddress & remoteAddress, bool mediaAddress)
{
  if (IsOpen())
    return true;

  PSafeLockReadWrite lock(*this);
  if (!lock.IsLocked())
    return false;

  if (!OpalMediaSession::Open(localInterface, remoteAddress, mediaAddress))
    return false;

  PIPSocket::Address bindingAddress(localInterface);

  PTRACE(4, *this << "opening: local=" << bindingAddress << " remote=" << m_remoteAddress);

  m_firstControl = true;

  for (int i = 0; i < 2; ++i) {
    delete m_socket[i];
    m_socket[i] = NULL;
  }

#if OPAL_PTLIB_NAT
  if (!m_manager.IsLocalAddress(m_remoteAddress)) {
    PNatMethod * natMethod = m_manager.GetNatMethods().GetMethod(bindingAddress, this);
    if (natMethod != NULL) {
      PTRACE(4, *this << "NAT Method " << natMethod->GetMethodName() << " selected for call.");

      switch (natMethod->GetRTPSupport()) {
        case PNatMethod::RTPIfSendMedia :
          /* This NAT variant will work if we send something out through the
              NAT port to "open" it so packets can then flow inward. We set
              this flag to make that happen as soon as we get the remotes IP
              address and port to send to.
              */
          m_localHasRestrictedNAT = true;
          // Then do case for full cone support and create NAT sockets

        case PNatMethod::RTPSupported :
          PTRACE(4, *this << "attempting natMethod: " << natMethod->GetMethodName());
          if (m_singlePortRx) {
            if (!natMethod->CreateSocket(m_socket[e_Data], bindingAddress)) {
              delete m_socket[e_Data];
              m_socket[e_Data] = NULL;
              PTRACE(2, *this << natMethod->GetMethodName()
                     << " could not create NAT RTP socket, using normal sockets.");
            }
          }
          else if (natMethod->CreateSocketPair(m_socket[e_Data], m_socket[e_Control], bindingAddress, this)) {
            PTRACE(4, *this << natMethod->GetMethodName() << " created NAT RTP/RTCP socket pair.");
          }
          else {
            PTRACE(2, *this << natMethod->GetMethodName()
                   << " could not create NAT RTP/RTCP socket pair; trying to create individual sockets.");
            if (!natMethod->CreateSocket(m_socket[e_Data], bindingAddress, 0, this) ||
                !natMethod->CreateSocket(m_socket[e_Control], bindingAddress, 0, this)) {
              for (int i = 0; i < 2; ++i) {
                delete m_socket[i];
                m_socket[i] = NULL;
              }
              PTRACE(2, *this << natMethod->GetMethodName()
                     << " could not create NAT RTP/RTCP sockets individually either, using normal sockets.");
            }
          }
          break;

        default :
          /* We canot use NAT traversal method (e.g. STUN) to create sockets
              in the remaining modes as the NAT router will then not let us
              talk to the real RTP destination. All we can so is bind to the
              local interface the NAT is on and hope the NAT router is doing
              something sneaky like symmetric port forwarding. */
          natMethod->GetInterfaceAddress(m_localAddress);
          break;
      }
    }
  }
#endif // OPAL_PTLIB_NAT

  if (m_socket[e_Data] == NULL) {
    bool ok;

    m_socket[e_Data] = new PUDPSocket();
    if (m_singlePortRx)
      ok = m_manager.GetRtpIpPortRange().Listen(*m_socket[e_Data], bindingAddress);
    else {
      m_socket[e_Control] = new PUDPSocket();

      // Don't use for loop, they are in opposite order!
      PIPSocket * sockets[2];
      sockets[0] = m_socket[e_Data];
      sockets[1] = m_socket[e_Control];
      ok = m_manager.GetRtpIpPortRange().Listen(sockets, 2, bindingAddress);
    }

    if (!ok) {
      PTRACE(1, *this << "no ports available for RTP session " << m_sessionId << ","
                " base=" << m_manager.GetRtpIpPortBase() << ","
                " max=" << m_manager.GetRtpIpPortMax() << ","
                " bind=" << bindingAddress << ","
                " for " << m_connection);
      return false; // Used up all the available ports!
    }
  }

  for (int i = 0; i < 2; ++i) {
    if (m_socket[i] != NULL) {
      PUDPSocket & socket = *m_socket[i];
      PTRACE_CONTEXT_ID_TO(socket);
      socket.GetLocalAddress(m_localAddress, m_localPort[i]);
      socket.SetReadTimeout(m_maxNoReceiveTime);
      // Increase internal buffer size on media UDP sockets
      SetMinBufferSize(socket, SO_RCVBUF, i == e_Control ? RTP_CTRL_BUFFER_SIZE : (m_isAudio ? RTP_AUDIO_RX_BUFFER_SIZE : RTP_VIDEO_RX_BUFFER_SIZE));
      SetMinBufferSize(socket, SO_SNDBUF, i == e_Control ? RTP_CTRL_BUFFER_SIZE : RTP_DATA_TX_BUFFER_SIZE);
    }
  }

  if (m_socket[e_Control] == NULL)
    m_localPort[e_Control] = m_localPort[e_Data];

  SetQoS(m_qos);

  m_manager.TranslateIPAddress(m_localAddress, m_remoteAddress);

  m_reportTimer.RunContinuous(m_reportTimer.GetResetTime());
  m_thread = new PThreadObj<OpalRTPSession>(*this, &OpalRTPSession::ThreadMain,
                                            false, PSTRSTRM("RTP-"<<m_sessionId), PThread::HighPriority);
  PTRACE_CONTEXT_ID_TO(m_thread);

  RTP_SyncSourceId ssrc = GetSyncSourceOut();
  if (ssrc == 0)
    ssrc = AddSyncSource(0, e_Sender); // Add default sender SSRC

  PTRACE(3, *this << "opened: "
            " local=" << m_localAddress << ':' << m_localPort[e_Data] << '-' << m_localPort[e_Control]
         << " remote=" << m_remoteAddress
         << " added default sender SSRC=" << RTP_TRACE_SRC(ssrc));

  return true;
}


bool OpalRTPSession::SetQoS(const PIPSocket::QoS & qos)
{
  PSafeLockReadOnly lock(*this);

  if (!lock.IsLocked() || m_socket[e_Data] == NULL || !m_remoteAddress.IsValid() || m_remotePort[e_Data] == 0)
    return false;

  m_qos = qos;
  m_qos.m_remote.SetAddress(m_remoteAddress, m_remotePort[e_Data]);
  return m_socket[e_Data]->SetQoS(m_qos);
}


bool OpalRTPSession::IsOpen() const
{
  PSafeLockReadOnly lock(*this);

  return lock.IsLocked() &&
         m_thread != NULL &&
        !m_thread->IsTerminated() &&
         m_socket[e_Data   ] != NULL && m_socket[e_Data   ]->IsOpen() &&
        (m_socket[e_Control] == NULL || m_socket[e_Control]->IsOpen());
}


bool OpalRTPSession::IsEstablished() const
{
#if OPAL_ICE
  if (m_iceServer == NULL)
    return m_candidates[e_Data].empty();

  return m_remoteAddress.IsValid() && m_remotePort[e_Data] != 0;
#else
  return true;
#endif
}


void OpalRTPSession::InternalStopRead()
{
  PThread * thread;
  {
    PSafeLockReadWrite lock(*this);
    if (m_thread == NULL)
      return;
    thread = m_thread;
    m_thread = NULL;
  }

  m_reportTimer.Stop(true);

  m_endpoint.RegisterLocalRTP(this, true);

  for (int i = 0; i < 2; ++i) {
    if (m_socket[i] != NULL)
      m_socket[i]->Close();
  }

  PAssert(thread->WaitForTermination(2000), "RTP thread failed to terminate");
  delete thread;
}


bool OpalRTPSession::Close()
{
  if (!IsOpen())
    return false;

  PTRACE(4, *this << "closing RTP.");

  if (LockReadOnly()) {
    for (SyncSourceMap::iterator it = m_SSRC.begin(); it != m_SSRC.end(); ++it) {
      if (it->second->m_direction == e_Sender && it->second->m_packets > 0)
        it->second->SendBYE();
    }
    UnlockReadOnly();
  }

  InternalStopRead();

  PSafeLockReadWrite lock(*this);
  InternalClose();
  return true;
}


void OpalRTPSession::InternalClose()
{
  for (int i = 0; i < 2; ++i) {
    delete m_socket[i];
    m_socket[i] = NULL;
  }

  m_localAddress = PIPSocket::GetInvalidAddress();
  m_localPort[e_Data] = m_localPort[e_Control] = 0;
  m_remoteAddress = PIPSocket::GetInvalidAddress();
  m_remotePort[e_Data] = m_remotePort[e_Control] = 0;
}


PString OpalRTPSession::GetLocalHostName()
{
  return PIPSocket::GetHostName();
}


void OpalRTPSession::SetSinglePortTx(bool v)
{
  m_singlePortTx = v;

  if (m_remotePort[e_Data] != 0)
    m_remotePort[e_Control] = m_remotePort[e_Data];
  else if (m_remotePort[e_Control] != 0)
    m_remotePort[e_Data] = m_remotePort[e_Control];
}


bool OpalRTPSession::SetRemoteAddress(const OpalTransportAddress & remoteAddress, bool isMediaAddress)
{
  PSafeLockReadWrite lock(*this);
  if (!lock.IsLocked())
    return false;

  if (m_remoteBehindNAT) {
    PTRACE(3, *this << "ignoring remote address as is behind NAT");
    return true;
  }

  PIPAddressAndPort ap;
  if (!remoteAddress.GetIpAndPort(ap))
    return false;

  return InternalSetRemoteAddress(ap, isMediaAddress, false PTRACE_PARAM(, "signalling"));
}


bool OpalRTPSession::InternalSetRemoteAddress(const PIPSocket::AddressAndPort & ap, bool isMediaAddress, bool dontOverride PTRACE_PARAM(, const char * source))
{
  WORD port = ap.GetPort();

  if (m_remoteAddress == ap.GetAddress() && m_remotePort[isMediaAddress] == port)
    return true;

  if (m_localAddress == ap.GetAddress() && m_localPort[isMediaAddress] == port) {
    PTRACE(2, *this << "cannot set remote address/port to same as local: " << ap);
    return false;
  }

  if (dontOverride && m_remoteAddress.IsValid() && m_remotePort[isMediaAddress] != 0) {
    PTRACE(2, *this << "cannot set remote address/port to " << ap
                    << ", already set to " << m_remoteAddress << ':' << m_remotePort[isMediaAddress]);
    return false;
  }

  m_remoteAddress = ap.GetAddress();
  
  if (port != 0) {
    if (m_singlePortTx)
      m_remotePort[e_Control] = m_remotePort[e_Data] = port;
    else if (isMediaAddress) {
      m_remotePort[e_Data] = port;
      if (m_remotePort[e_Control] == 0)
        m_remotePort[e_Control] = (WORD)(port | 1);
    }
    else {
      m_remotePort[e_Control] = port;
      if (m_remotePort[e_Data] == 0)
        m_remotePort[e_Data] = (WORD)(port & 0xfffe);
    }
  }

#if PTRACING
  static const int Level = 3;
  if (PTrace::CanTrace(Level)) {
    ostream & trace = PTRACE_BEGIN(Level);
    trace << *this << source << " set remote "
          << (isMediaAddress ? "data" : "control") << " address to " << ap << ", ";

    if (IsSinglePortTx())
      trace << "single port mode";
    else {
      if (m_remotePort[e_Data] != (m_remotePort[e_Control] & 0xfffe))
        trace << "disjoint ";
      trace << (isMediaAddress ? "control" : "data") << " port " << m_remotePort[!isMediaAddress];
    }

    trace << ", local=";
    if (m_localAddress.IsValid()) {
      trace << m_localAddress << ':' << m_localPort[e_Data];
      if (!IsSinglePortRx())
        trace << '-' << m_localPort[e_Control];
      if (m_localHasRestrictedNAT)
        trace << ", restricted NAT";
    }
    else
      trace << "not open";

    trace << PTrace::End;
  }
#endif

  for (int i = 0; i < 2; ++i) {
    if (m_socket[i] != NULL) {
      m_socket[i]->SetSendAddress(m_remoteAddress, m_remotePort[i]);

      if (m_localHasRestrictedNAT) {
        // If have Port Restricted NAT on local host then send a datagram
        // to remote to open up the port in the firewall for return data.
        static const BYTE dummy[e_Data] = { 0 };
        m_socket[i]->Write(dummy, sizeof(dummy));
      }
    }
  }

  m_manager.QueueDecoupledEvent(new PSafeWorkNoArg<OpalConnection, bool>(&m_connection, &OpalConnection::InternalOnEstablished));

  return true;
}


#if OPAL_ICE

void OpalRTPSession::SetICE(const PString & user, const PString & pass, const PNatCandidateList & candidates)
{
  PSafeLockReadWrite lock(*this);
  if (!lock.IsLocked())
    return;

  if (user == m_remoteUsername && pass == m_remotePassword) {
    PTRACE(3, *this << "ICE username/password not changed");
    return;
  }

  delete m_iceServer;
  m_iceServer = NULL;

  delete m_stunClient;
  m_stunClient = NULL;

  OpalMediaSession::SetICE(user, pass, candidates);
  if (user.IsEmpty() || pass.IsEmpty()) {
    PTRACE(3, *this << "ICE disabled");
    return;
  }

  if (m_candidateType == e_UnknownCandidates)
    m_candidateType = e_RemoteCandidates;

  if (m_candidateType == e_RemoteCandidates) {
    for (int channel = 0; channel < 2; ++channel)
      m_candidates[channel].clear();

    for (PNatCandidateList::const_iterator it = candidates.begin(); it != candidates.end(); ++it) {
      PTRACE(4, "Checking candidate: " << *it);
      if (it->m_protocol == "udp") {
        switch (it->m_component) {
          case PNatMethod::eComponent_RTP:
            m_candidates[e_Data].push_back(*it);
            break;
          case PNatMethod::eComponent_RTCP:
            m_candidates[e_Control].push_back(*it);
            break;
          default :
            break;
        }
      }
    }
  }

  m_iceServer = new ICEServer(*this);
  PTRACE_CONTEXT_ID_TO(m_iceServer);
  m_iceServer->Open(m_socket[e_Data], m_socket[e_Control]);
  m_iceServer->SetCredentials(m_localUsername + ':' + m_remoteUsername, m_localPassword, PString::Empty());

  m_stunClient = new PSTUNClient;
  PTRACE_CONTEXT_ID_TO(m_stunClient);
  m_stunClient->SetCredentials(m_remoteUsername + ':' + m_localUsername, m_remotePassword, PString::Empty());

  m_remoteAddress = PIPSocket::GetInvalidAddress();
  m_remoteBehindNAT = true;

  PTRACE(3, *this << "configured by remote for ICE with "
         << (m_candidateType == e_RemoteCandidates ? "remote" : "local") << " candidates: "
            "data=" << m_candidates[e_Data].size() << ", " "control=" << m_candidates[e_Control].size());
}


bool OpalRTPSession::GetICE(PString & user, PString & pass, PNatCandidateList & candidates)
{
  PSafeLockReadWrite lock(*this);
  if (!lock.IsLocked())
    return false;

  if (!OpalMediaSession::GetICE(user, pass, candidates))
    return false;

  if (!m_localAddress.IsValid())
    return false;

  if (m_candidateType == e_UnknownCandidates)
    m_candidateType = e_LocalCandidates;

  for (int channel = e_Data; channel >= e_Control; --channel) {
    if (m_candidateType == e_LocalCandidates)
      m_candidates[channel].clear();

    if (m_localPort[channel] != 0 && m_socket[channel] != NULL) {
      // Only do ICE-Lite right now so just offer "host" type using local address.
      static const PNatMethod::Component ComponentId[2] = { PNatMethod::eComponent_RTCP, PNatMethod::eComponent_RTP };
      PNatCandidate candidate(PNatCandidate::HostType, ComponentId[channel], "xyzzy");
      candidate.m_baseTransportAddress.SetAddress(m_localAddress, m_localPort[channel]);
      candidate.m_priority = (126 << 24) | (256 - candidate.m_component);

      if (m_localAddress.GetVersion() != 6)
        candidate.m_priority |= 0xffff00;
      else {
        /* Incomplete need to get precedence from following table, for now use 50
              Prefix        Precedence Label
              ::1/128               50     0
              ::/0                  40     1
              2002::/16             30     2
              ::/96                 20     3
              ::ffff:0:0/96         10     4
        */
        candidate.m_priority |= 50 << 8;
      }

      candidates.Append(new PNatCandidate(candidate));

      if (m_candidateType == e_LocalCandidates)
        m_candidates[channel].push_back(candidate);
    }
  }

  m_remoteBehindNAT = true;

  PTRACE(3, *this << "configured locally for ICE with "
         << (m_candidateType == e_RemoteCandidates ? "remote" : "local") << " candidates: "
            "data=" << m_candidates[e_Data].size() << ", " "control=" << m_candidates[e_Control].size());

  return !candidates.empty();
}


OpalRTPSession::SendReceiveStatus OpalRTPSession::OnReceiveICE(Channel channel,
                                                               const BYTE * framePtr,
                                                               PINDEX frameSize,
                                                               const PIPSocket::AddressAndPort & ap)
{
  if (m_iceServer == NULL)
    return m_candidates[channel].empty() ? e_ProcessPacket : e_IgnorePacket;

  PPROFILE_FUNCTION();

  PSTUNMessage message(framePtr, frameSize, ap);
  if (!message.IsValid())
    return m_candidates[channel].empty() ? e_ProcessPacket : e_IgnorePacket;

  if (message.IsRequest()) {
    if (!m_iceServer->OnReceiveMessage(message, PSTUNServer::SocketInfo(m_socket[channel])))
      return e_IgnorePacket;

    if (message.FindAttribute(PSTUNAttribute::USE_CANDIDATE) == NULL) {
      PTRACE(4, *this << "awaiting USE-CANDIDATE");
      return e_IgnorePacket;
    }
    PTRACE(m_throttleUseCandidate, *this << "USE-CANDIDATE found" << m_throttleUseCandidate);
  }
  else {
    if (!m_stunClient->ValidateMessageIntegrity(message))
      return e_IgnorePacket;

    if (m_candidateType != e_LocalCandidates) {
      PTRACE(3, *this << "Unexpected STUN response received.");
      return e_IgnorePacket;
    }

    PTRACE(4, *this << "trying to match STUN answer to candidates");
    for (CandidateStateList::const_iterator it = m_candidates[channel].begin(); ; ++it) {
      if (it == m_candidates[channel].end())
        return e_IgnorePacket;
      if (it->m_baseTransportAddress == ap)
        break;
    }
  }

  InternalSetRemoteAddress(ap, channel, true PTRACE_PARAM(, "ICE"));

  m_candidates[channel].clear();
  return e_IgnorePacket;
}


void OpalRTPSession::ICEServer::OnBindingResponse(const PSTUNMessage &, PSTUNMessage & response)
{
  response.AddAttribute(PSTUNAttribute::USE_CANDIDATE);
}


OpalRTPSession::SendReceiveStatus OpalRTPSession::OnSendICE(Channel channel)
{
#if EXPERIMENTAL_ICE
  if (m_candidateType == e_LocalCandidates && m_socket[channel] != NULL) {
    for (CandidateStateList::iterator it = m_candidates[channel].begin(); it != m_candidates[channel].end(); ++it) {
      if (it->m_baseTransportAddress.IsValid()) {
        PTRACE(4, *this << "sending BINDING-REQUEST to " << *it);
        PSTUNMessage request(PSTUNMessage::BindingRequest);
        request.AddAttribute(PSTUNAttribute::ICE_CONTROLLED); // We are ICE-lite and always controlled
        m_stunClient->AppendMessageIntegrity(request);
        if (!request.Write(*m_socket[channel], it->m_baseTransportAddress))
          return e_AbortTransport;
      }
    }
  }
#endif
  return m_remotePort[channel] != 0 ? e_ProcessPacket : e_IgnorePacket;
}


#endif // OPAL_ICE


OpalRTPSession::SendReceiveStatus OpalRTPSession::ReadRawPDU(BYTE * framePtr, PINDEX & frameSize, Channel channel)
{
  PPROFILE_FUNCTION();

  if (PAssertNULL(m_socket[channel]) == NULL)
    return e_AbortTransport;

  PUDPSocket & socket = *m_socket[channel];
  PIPSocket::AddressAndPort ap;

  PTRACE(m_throttleReadPacket, *this << "Read " << channel << " packet: sz=" << frameSize << " timeout=" << socket.GetReadTimeout());
  if (socket.ReadFrom(framePtr, frameSize, ap)) {
    frameSize = socket.GetLastReadCount();

    READ_PERFORMANCE_HACK(0,e_IgnorePacket)

    // Ignore one byte packet from ourself, likely from the I/O block breaker in OpalRTPSession::Shutdown()
    if (frameSize == 1) {
      PIPSocketAddressAndPort localAP;
      socket.PUDPSocket::InternalGetLocalAddress(localAP);
      if (ap == localAP) {
        PTRACE(5, *this << channel << " I/O block breaker ignored.");
        return e_IgnorePacket;
      }
    }

  PSafeLockReadWrite lock(*this);
  if (!lock.IsLocked())
    return e_AbortTransport;

    READ_PERFORMANCE_HACK(1,e_IgnorePacket)

#if OPAL_ICE
    SendReceiveStatus status = OnReceiveICE(channel, framePtr, frameSize, ap);
    if (status != e_ProcessPacket)
      return status;
#endif // OPAL_ICE

    // If remote address never set from higher levels, then try and figure
    // it out from the first packet received.
    if (m_remotePort[channel] == 0)
      InternalSetRemoteAddress(ap, channel, true PTRACE_PARAM(, "first PDU"));

    m_noTransmitErrors = 0;

    return e_ProcessPacket;
  }

  switch (socket.GetErrorCode(PChannel::LastReadError)) {
    case PChannel::Unavailable :
      return HandleUnreachable(PTRACE_PARAM(channel)) ? e_IgnorePacket : e_AbortTransport;

    case PChannel::BufferTooSmall :
      PTRACE(2, *this << channel << " read packet too large for buffer of " << frameSize << " bytes.");
      return e_IgnorePacket;

    case PChannel::Interrupted :
      PTRACE(4, *this << channel << " read packet interrupted.");
      // Shouldn't happen, but it does.
      return e_IgnorePacket;

    case PChannel::NoError :
      PTRACE(3, *this << channel << " received UDP packet with no payload.");
      return e_IgnorePacket;

    case PChannel::Timeout :
      PTRACE(1, *this << "Timeout (" << socket.GetReadTimeout() << "s) receiving " << channel << " packets");
      return channel == e_Data ? e_AbortTransport : e_IgnorePacket;

    default:
      PTRACE(1, *this << channel
             << " read error (" << socket.GetErrorNumber(PChannel::LastReadError) << "): "
             << socket.GetErrorText(PChannel::LastReadError));
      return e_AbortTransport;
  }
}


bool OpalRTPSession::HandleUnreachable(PTRACE_PARAM(Channel channel))
{
  if (++m_noTransmitErrors == 1) {
    PTRACE(2, *this << channel << " port on remote not ready.");
    m_noTransmitTimer = m_maxNoTransmitTime;
    return true;
  }

  if (m_noTransmitErrors < 10 || m_noTransmitTimer.IsRunning())
    return true;

  PTRACE(2, *this << channel << ' ' << m_maxNoTransmitTime << " seconds of transmit fails");
  if (m_connection.OnMediaFailed(m_sessionId, false))
    return false;

  m_noTransmitErrors = 0;
  return true;
}


void OpalRTPSession::ThreadMain()
{
  PTRACE(3, *this << "thread started");

  while (InternalRead())
    ;

  PTRACE(3, *this << "thread ended");
}


bool OpalRTPSession::InternalRead()
{
  PPROFILE_FUNCTION();

  /* Note this should only be called from within the RTP read thread.
     The locking strategy depends on it. */

  if (!IsOpen())
    return false;

  PTimeInterval readTimeout = m_maxNoReceiveTime;
#if OPAL_ICE
  if (m_iceServer == NULL ? !m_candidates[e_Data].empty() : (!m_remoteAddress.IsValid() || m_remotePort[e_Data] == 0))
    readTimeout = m_maxICESetUpTime;
#endif

  if (m_socket[e_Control] == NULL) {
    m_socket[e_Data]->SetReadTimeout(readTimeout);
    return InternalReadData();
  }

  int selectStatus = PSocket::Select(*m_socket[e_Data], *m_socket[e_Control], readTimeout);
  switch (selectStatus) {
    case -3 :
      return InternalReadData() && InternalReadControl();

    case -2 :
      return InternalReadControl();

    case -1 :
      return InternalReadData();

    case 0 :
      PTRACE(2, *this << "timeout on received RTP and RTCP sockets.");
      return !(m_connection.OnMediaFailed(m_sessionId, true) && m_connection.OnMediaFailed(m_sessionId, false));

    default :
      PTRACE(2, *this << "select error: " << PChannel::GetErrorText((PChannel::Errors)selectStatus));
      return false;
  }
}


bool OpalRTPSession::InternalReadData()
{
  PPROFILE_FUNCTION();

  /* Note this should only be called from within the RTP read thread.
     The locking strategy depends on it. */

  PINDEX pduSize = m_manager.GetMaxRtpPacketSize();
  RTP_DataFrame frame((PINDEX)0, pduSize);
  switch (ReadRawPDU(frame.GetPointer(), pduSize, e_Data)) {
    case e_AbortTransport :
      if (m_connection.OnMediaFailed(m_sessionId, true) &&
          m_socket[e_Control] == NULL &&
          m_connection.OnMediaFailed(m_sessionId, false)) {
        PTRACE(4, *this << "aborting single port read");
        return false;
      }
      // Do next case

    case e_IgnorePacket :
      return true;

    case e_ProcessPacket :
      break;
  }

  PSafeLockReadWrite lock(*this);
  if (!lock.IsLocked())
    return false;

  READ_PERFORMANCE_HACK(2,true)

  // Check for single port operation, incoming RTCP on RTP
  RTP_ControlFrame control(frame, pduSize, false);
  unsigned type = control.GetPayloadType();
  if ((type >= RTP_ControlFrame::e_FirstValidPayloadType && type <= RTP_ControlFrame::e_LastValidPayloadType
                            ? OnReceiveControl(control) : OnReceiveData(frame, pduSize)) == e_AbortTransport)
    return false;

  READ_PERFORMANCE_HACK(3,true)

  for (SyncSourceMap::iterator it = m_SSRC.begin(); it != m_SSRC.end(); ++it) {
    if (!it->second->HandlePendingFrames())
      return false;
  }

  return true;
}


bool OpalRTPSession::InternalReadControl()
{
  /* Note this should only be called from within the RTP read thread.
     The locking strategy depends on it. */

  PINDEX pduSize = 2048;
  RTP_ControlFrame frame(pduSize);
  switch (ReadRawPDU(frame.GetPointer(), pduSize, e_Control)) {
    case e_AbortTransport:
      m_connection.OnMediaFailed(m_sessionId, false);
      return false;

    case e_IgnorePacket:
      return true;

    case e_ProcessPacket:
      break;
  }

  if (frame.SetPacketSize(pduSize)) {
    PSafeLockReadWrite lock(*this);
    return lock.IsLocked() && OnReceiveControl(frame) != e_AbortTransport;
  }

  PTRACE_IF(2, pduSize != 1 || !m_firstControl, *this << "received control packet too small: " << pduSize << " bytes");
  return true;
}


OpalRTPSession::SendReceiveStatus OpalRTPSession::WriteData(RTP_DataFrame & frame, RewriteMode rewrite, const PIPSocketAddressAndPort * remote)
{
  PPROFILE_FUNCTION();

  WRITE_PERFORMANCE_HACK(0,e_ProcessPacket)

  PSafeLockReadWrite lock(*this);
  if (!lock.IsLocked() || !IsOpen())
    return e_AbortTransport;

  WRITE_PERFORMANCE_HACK(1,e_ProcessPacket)

  SendReceiveStatus status = OnSendData(frame, rewrite);

  WRITE_PERFORMANCE_HACK(7,e_ProcessPacket)

  if (status != e_ProcessPacket)
      return status;

  return WriteRawPDU(frame.GetPointer(), frame.GetPacketSize(), e_Data, remote);
}


OpalRTPSession::SendReceiveStatus OpalRTPSession::WriteControl(RTP_ControlFrame & frame, const PIPSocketAddressAndPort * remote)
{
  PSafeLockReadWrite lock(*this);
  if (!lock.IsLocked() || !IsOpen())
    return e_AbortTransport;

  SendReceiveStatus status = OnSendControl(frame);
  if (status == e_ProcessPacket)
    status = WriteRawPDU(frame.GetPointer(), frame.GetPacketSize(), e_Control, remote);
  return status;
}


OpalRTPSession::SendReceiveStatus OpalRTPSession::WriteRawPDU(const BYTE * framePtr, PINDEX frameSize, Channel channel, const PIPSocketAddressAndPort * remote)
{
  PPROFILE_FUNCTION();

  PIPSocketAddressAndPort remoteAddressAndPort;
  if (remote == NULL) {
    remoteAddressAndPort.SetAddress(m_remoteAddress, m_remotePort[channel]);

    // Trying to send a PDU before we are set up!
    if (!remoteAddressAndPort.IsValid())
      return e_IgnorePacket;

    remote = &remoteAddressAndPort;
  }

  PUDPSocket & socket = *(m_socket[channel] != NULL ? m_socket[channel] : m_socket[e_Data]);
  if (socket.WriteTo(framePtr, frameSize, *remote))
    return e_ProcessPacket;

  if (socket.GetErrorCode(PChannel::LastWriteError) == PChannel::Unavailable)
    return e_IgnorePacket;

  if (HandleUnreachable(PTRACE_PARAM(channel)))
    return e_IgnorePacket;

  PTRACE(1, *this << "write (" << frameSize << " bytes) error on "
         << channel << " port (" << socket.GetErrorNumber(PChannel::LastWriteError) << "): "
         << socket.GetErrorText(PChannel::LastWriteError));
  return e_AbortTransport;
}


/////////////////////////////////////////////////////////////////////////////
