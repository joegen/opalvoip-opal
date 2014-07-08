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

#define RTP_VIDEO_RX_BUFFER_SIZE 0x100000 // 1Mb
#define RTP_AUDIO_RX_BUFFER_SIZE 0x4000   // 16kb
#define RTP_DATA_TX_BUFFER_SIZE  0x2000   // 8kb
#define RTP_CTRL_BUFFER_SIZE     0x1000   // 4kb


#define PTraceModule() "RTP"

enum { JitterRoundingGuardBits = 4 };

#if PTRACING
static ostream & operator<<(ostream & strm, const std::set<unsigned> & us)
{
  for (std::set<unsigned>::const_iterator it = us.begin(); it != us.end(); ++it) {
    if (it != us.begin())
      strm << ',';
    strm << *it;
  }
  return strm;
}
#endif


#define new PNEW

/////////////////////////////////////////////////////////////////////////////

const PCaselessString & OpalRTPSession::RTP_AVP () { static const PConstCaselessString s("RTP/AVP" ); return s; }
const PCaselessString & OpalRTPSession::RTP_AVPF() { static const PConstCaselessString s("RTP/AVPF"); return s; }

PFACTORY_CREATE(OpalMediaSessionFactory, OpalRTPSession, OpalRTPSession::RTP_AVP());
PFACTORY_SYNONYM(OpalMediaSessionFactory, OpalRTPSession, AVPF, OpalRTPSession::RTP_AVPF());


#define DEFAULT_OUT_OF_ORDER_WAIT_TIME 50

#if P_CONFIG_FILE
static PTimeInterval GetDefaultOutOfOrderWaitTime()
{
  static PTimeInterval ooowt(PConfig(PConfig::Environment).GetInteger("OPAL_RTP_OUT_OF_ORDER_TIME", DEFAULT_OUT_OF_ORDER_WAIT_TIME));
  return ooowt;
}
#else
#define GetDefaultOutOfOrderWaitTime() (DEFAULT_OUT_OF_ORDER_WAIT_TIME)
#endif


OpalRTPSession::OpalRTPSession(const Init & init)
  : OpalMediaSession(init)
  , m_endpoint(dynamic_cast<OpalRTPEndPoint &>(init.m_connection.GetEndPoint()))
  , m_singlePortRx(false)
  , m_singlePortTx(false)
  , m_isAudio(init.m_mediaType == OpalMediaType::Audio())
  , m_timeUnits(m_isAudio ? 8 : 90)
  , m_toolName(PProcess::Current().GetName())
  , m_allowAnySyncSource(true)
  , m_maxNoReceiveTime(init.m_connection.GetEndPoint().GetManager().GetNoMediaTimeout())
  , m_maxNoTransmitTime(0, 10)          // Sending data for 10 seconds, ICMP says still not there
  , m_maxOutOfOrderPackets(20)
  , m_waitOutOfOrderTime(GetDefaultOutOfOrderWaitTime())
  , m_txStatisticsInterval(100)
  , m_rxStatisticsInterval(100)
#if OPAL_RTP_FEC
  , m_redundencyPayloadType(RTP_DataFrame::IllegalPayloadType)
  , m_ulpFecPayloadType(RTP_DataFrame::IllegalPayloadType)
  , m_ulpFecSendLevel(2)
#endif
  , m_feedback(OpalMediaFormat::e_NoRTCPFb)
  , m_dummySyncSource(*this, 0, e_Receive, "-")
  , m_rtcpPacketsSent(0)
  , m_roundTripTime(0)
  , m_reportTimer(0, 12)  // Seconds
  , m_localAddress(PIPSocket::GetInvalidAddress())
  , m_remoteAddress(PIPSocket::GetInvalidAddress())
  , m_shutdownRead(false)
  , m_shutdownWrite(false)
  , m_remoteBehindNAT(init.m_remoteBehindNAT)
  , m_localHasRestrictedNAT(false)
  , m_noTransmitErrors(0)
#if OPAL_ICE
  , m_stunServer(NULL)
  , m_stunClient(NULL)
#endif
#if PTRACING
  , m_levelTxRR(3)
  , m_levelRxSR(3)
  , m_levelRxRR(3)
  , m_levelRxSDES(3)
  , m_levelTxRED(3)
  , m_levelRxRED(3)
  , m_levelRxUnknownFEC(3)
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
  , m_dummySyncSource(*this, 0, e_Receive, NULL)
{
}


OpalRTPSession::~OpalRTPSession()
{
  Close();

  for (SyncSourceMap::iterator it = m_SSRC.begin(); it != m_SSRC.end(); ++it)
    delete it->second;

#if OPAL_ICE
  delete m_stunServer;
  delete m_stunClient;
#endif
}


RTP_SyncSourceId OpalRTPSession::AddSyncSource(RTP_SyncSourceId id, Direction dir, const char * cname)
{
  PWaitAndSignal mutex(m_dataMutex);

  if (id == 0) {
    do {
      id = PRandom::Number();
    } while (m_SSRC.find(id) != m_SSRC.end());
  }

  SyncSource * ssrc = CreateSyncSource(id, dir, cname);
  if (m_SSRC.insert(SyncSourceMap::value_type(id, ssrc)).second)
    return id;

  PTRACE(2, "Session " << GetSessionID() << ", could not add SSRC "
         << RTP_TRACE_SRC(id) << ", probable clash with receiver.");
  delete ssrc;
  return 0;
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
      ssrcs.SetAt(ssrcs.GetSize(), it->first);
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
  if (ssrc != 0)
    it = m_SSRC.find(ssrc);
  else {
    for (it = m_SSRC.begin(); it != m_SSRC.end(); ++it) {
      if (it->second->m_direction == dir)
        break;
    }
  }

  if (it == m_SSRC.end()) {
    PTRACE(3, "Cannot find " << (dir == e_Receive ? "receive" : "transmit")
           << " SyncSource for SSRC=" << RTP_TRACE_SRC(ssrc));
    return false;
  }

  info = const_cast<SyncSource *>(it->second);
  return true;
}


OpalRTPSession::SyncSource::SyncSource(OpalRTPSession & session, RTP_SyncSourceId id, Direction dir, const char * cname)
  : m_session(session)
  , m_direction(dir)
  , m_sourceIdentifier(id)
  , m_canonicalName(cname)
  , m_lastSequenceNumber(0)
  , m_lastFIRSequenceNumber(0)
  , m_lastTSTOSequenceNumber(0)
  , m_consecutiveOutOfOrderPackets(0)
  , m_syncTimestamp(0)
  , m_syncRealTime(0)
  , m_firstPacketTime(0)
  , m_packets(0)
  , m_octets(0)
  , m_senderReports(0)
  , m_packetsLost(0)
  , m_packetsOutOfOrder(0)
  , m_averagePacketTime(0)
  , m_maximumPacketTime(0)
  , m_minimumPacketTime(0)
  , m_jitter(0)
  , m_maximumJitter(0)
  , m_markerCount(0)
  , m_lastTimestamp(0)
  , m_averageTimeAccum(0)
  , m_maximumTimeAccum(0)
  , m_minimumTimeAccum(0)
  , m_jitterAccum(0)
  , m_lastJitterTimestamp(0)
  , m_packetsLostSinceLastRR(0)
  , m_lastRRSequenceNumber(0)
  , m_lastReportTime(0)
  , m_statisticsCount(0)
#if OPAL_RTCP_XR
  , m_metrics(NULL)
#endif
  , m_jitterBuffer(NULL)
{
  if (m_canonicalName.IsEmpty()) {
    /* CNAME is no longer just a username@host string, for security!
       But RFC 6222 hopelessly complicated, while not exactly the same, just
       using the base64 of the right most 12 bytes of GUID is very similar.
       It will do. */
    PGloballyUniqueID guid;
    m_canonicalName = PBase64::Encode(&guid[PGloballyUniqueID::Size-12], 12);
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
    trace << "Session " << m_session.GetSessionID() << ", "
          << (m_direction == e_Receive ? "receive" : "transmit") << " statistics:\n"
               "    Sync Source ID       = " << RTP_TRACE_SRC(m_sourceIdentifier) << "\n"
               "    first packet         = " << m_firstPacketTime << "\n"
               "    total packets        = " << m_packets << "\n"
               "    total octets         = " << m_octets << "\n"
               "    bitRateSent          = " << (8 * m_octets / duration) << "\n"
               "    lostPackets          = " << m_packetsLost << '\n';
    if (m_direction == e_Receive) {
      if (m_jitterBuffer != NULL)
        trace << "    packets too late     = " << m_jitterBuffer->GetPacketsTooLate() << '\n';
      trace << "    packets out of order = " << m_packetsOutOfOrder << '\n';
    }
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

  /* For audio we do not do statistics on start of talk burst as that
      could be a substantial time and is not useful, so we only calculate
      when the marker bit os off.

      For video we measure jitter between whole video frames which is
      normally indicated by the marker bit being on, but this is unreliable,
      many endpoints not sending it correctly, so use a change in timestamp
      as most reliable method. */
  if (m_session.IsAudio() ? frame.GetMarker() : (m_lastTimestamp == frame.GetTimestamp()))
    return;

  PTimeInterval tick = PTimer::Tick();
  unsigned diff = (tick - m_lastPacketTick).GetInterval();
  m_lastPacketTick = tick;
  m_lastTimestamp = frame.GetTimestamp();

  m_averageTimeAccum += diff;
  if (diff > m_maximumTimeAccum)
    m_maximumTimeAccum = diff;
  if (diff < m_minimumTimeAccum)
    m_minimumTimeAccum = diff;
  m_statisticsCount++;

  if (m_direction == e_Receive) {
    // As per RFC3550 Appendix 8
    diff *= m_session.m_timeUnits; // Convert to timestamp units
    long variance = diff > m_lastJitterTimestamp ? (diff - m_lastJitterTimestamp) : (m_lastJitterTimestamp - diff);
    m_lastJitterTimestamp = diff;
    m_jitterAccum += variance - ((m_jitterAccum + (1 << (JitterRoundingGuardBits - 1))) >> JitterRoundingGuardBits);
    m_jitter = (m_jitterAccum >> JitterRoundingGuardBits) / m_session.m_timeUnits;
    if (m_jitter > m_maximumJitter)
      m_maximumJitter = m_jitter;
  }

  if (m_statisticsCount < (m_direction == e_Receive ? m_session.GetRxStatisticsInterval() : m_session.GetTxStatisticsInterval()))
    return;

  m_statisticsCount = 0;

  m_averagePacketTime = m_averageTimeAccum/100;
  m_maximumPacketTime = m_maximumTimeAccum;
  m_minimumPacketTime = m_minimumTimeAccum;

  m_averageTimeAccum = 0;
  m_maximumTimeAccum = 0;
  m_minimumTimeAccum = 0xffffffff;

#if PTRACING
  unsigned Level = 4;
  if (PTrace::CanTrace(Level)) {
    ostream & trace = PTRACE_BEGIN(Level, &m_session, PTraceModule());
    trace << "Session " << m_session.GetSessionID() << ", "
          << (m_direction == e_Receive ? "receive" : "transmit") << " statistics:"
                 " packets=" << m_packets <<
                 " octets=" << m_octets;
    if (m_direction == e_Receive) {
      trace <<   " lost=" << m_session.GetPacketsLost() <<
                 " order=" << m_session.GetPacketsOutOfOrder();
      if (m_jitterBuffer != NULL)
        trace << " tooLate=" << m_jitterBuffer->GetPacketsTooLate();
    }
    trace <<     " avgTime=" << m_averagePacketTime <<
                 " maxTime=" << m_maximumPacketTime <<
                 " minTime=" << m_minimumPacketTime <<
                 " jitter=" << m_jitter <<
                 " maxJitter=" << m_maximumJitter;
  }
#endif
}


OpalRTPSession::SendReceiveStatus OpalRTPSession::SyncSource::OnSendData(RTP_DataFrame & frame, bool rewriteHeader)
{
  if (rewriteHeader) {
    if (m_lastSequenceNumber == 0) {
      m_lastSequenceNumber = (uint16_t)PRandom::Number(1, 65535);
      PTRACE(3, &m_session, "RTP\tSession " << m_session.GetSessionID() << ", first sent data:"
                " ver=" << frame.GetVersion()
             << " pt=" << frame.GetPayloadType()
             << " psz=" << frame.GetPayloadSize()
             << " m=" << frame.GetMarker()
             << " x=" << frame.GetExtension()
             << " seq=" << m_lastSequenceNumber
             << " ts=" << frame.GetTimestamp()
             << " src=" << RTP_TRACE_SRC(m_sourceIdentifier)
             << " ccnt=" << frame.GetContribSrcCount()
             << " rem=" << m_session.GetRemoteAddress()
             << " local=" << m_session.GetLocalAddress());
    }
    else {
      m_lastSequenceNumber += (RTP_SequenceNumber)(frame.GetDiscontinuity() + 1);
      PTRACE_IF(6, frame.GetDiscontinuity() > 0, &m_session,
                "Have discontinuity: " << frame.GetDiscontinuity() << ", sn=" << m_lastSequenceNumber);
    }
    frame.SetSequenceNumber(m_lastSequenceNumber);
    frame.SetSyncSource(m_sourceIdentifier);
  }

  CalculateStatistics(frame);
  return e_ProcessPacket;
}


OpalRTPSession::SendReceiveStatus OpalRTPSession::SyncSource::OnReceiveData(RTP_DataFrame & frame, PINDEX pduSize)
{
  RTP_SequenceNumber sequenceNumber = frame.GetSequenceNumber();
  RTP_SequenceNumber expectedSequenceNumber = m_lastSequenceNumber + 1;

  if (pduSize != P_MAX_INDEX) {
    if (!frame.SetPacketSize(pduSize))
      return e_IgnorePacket; // Received PDU is not big enough, ignore

    if (!m_pendingPackets.empty() && sequenceNumber == expectedSequenceNumber) {
      PTRACE(5, &m_session, "Session " << m_session.GetSessionID() << ", SSRC=" << RTP_TRACE_SRC(m_sourceIdentifier)
             << ", received out of order packet " << sequenceNumber);
      ++m_packetsOutOfOrder; // it arrived after all!
    }
  }

  // Check packet sequence numbers
  if (m_packets == 0) {
    m_firstPacketTime.SetCurrentTime();
    m_lastPacketTick = PTimer::Tick();

    PTRACE(3, &m_session, "Session " << m_session.GetSessionID() << ", first receive data:"
              " ver=" << frame.GetVersion()
           << " pt=" << frame.GetPayloadType()
           << " psz=" << frame.GetPayloadSize()
           << " m=" << frame.GetMarker()
           << " x=" << frame.GetExtension()
           << " seq=" << sequenceNumber
           << " ts=" << frame.GetTimestamp()
           << " src=" << RTP_TRACE_SRC(frame.GetSyncSource())
           << " ccnt=" << frame.GetContribSrcCount());

#if OPAL_RTCP_XR
    delete m_metrics; // Should be NULL, but just in case ...
    m_metrics = RTCP_XR_Metrics::Create(frame);
    PTRACE_CONTEXT_ID_SET(m_metrics, m_session);
#endif

    m_lastSequenceNumber = sequenceNumber;
  }
  else if (sequenceNumber == expectedSequenceNumber) {
    ++m_lastSequenceNumber;
    m_consecutiveOutOfOrderPackets = 0;
  }
  else if (sequenceNumber < expectedSequenceNumber) {
#if OPAL_RTCP_XR
    if (m_metrics != NULL) m_metrics->OnPacketDiscarded();
#endif

    // Check for Cisco bug where sequence numbers suddenly start incrementing
    // from a different base.
    if (++m_consecutiveOutOfOrderPackets > 10) {
      PTRACE(2, &m_session, "Session " << m_session.GetSessionID() << ", SSRC=" << RTP_TRACE_SRC(m_sourceIdentifier)
              << ", abnormal change of sequence numbers, adjusting from " << m_lastSequenceNumber << " to " << sequenceNumber);
      m_lastSequenceNumber = sequenceNumber;
    }
    else {
      PTRACE(2, &m_session, "Session " << m_session.GetSessionID() << ", SSRC=" << RTP_TRACE_SRC(m_sourceIdentifier)
              << ", incorrect sequence, got " << sequenceNumber << " expected " << expectedSequenceNumber);

      if (m_jitterBuffer == NULL)
        return e_IgnorePacket; // Non fatal error, just ignore

      m_packetsOutOfOrder++;
    }
  }
  else {
    if (m_jitterBuffer == NULL) {
      SendReceiveStatus status = m_session.OnOutOfOrderPacket(frame);
      if (status != e_ProcessPacket)
        return status;
      sequenceNumber = frame.GetSequenceNumber();
    }

    unsigned dropped = sequenceNumber - expectedSequenceNumber;
    frame.SetDiscontinuity(dropped);
    m_packetsLost += dropped;
    m_packetsLostSinceLastRR += dropped;
    PTRACE(2, &m_session, "Session " << m_session.GetSessionID() << ", SSRC=" << RTP_TRACE_SRC(m_sourceIdentifier)
            << ", " << dropped << " packet(s) missing at " << expectedSequenceNumber);
    m_lastSequenceNumber = sequenceNumber;
    m_consecutiveOutOfOrderPackets = 0;
#if OPAL_RTCP_XR
    if (m_metrics != NULL) m_metrics->OnPacketLost(dropped);
#endif
  }

  if (m_syncRealTime.IsValid())
    frame.SetAbsoluteTime(m_syncRealTime + PTimeInterval((frame.GetTimestamp() - m_syncTimestamp)/m_session.m_timeUnits));

#if OPAL_RTCP_XR
  if (m_metrics != NULL) {
    m_metrics->OnPacketReceived();
    if (m_jitterBuffer != NULL)
      m_metrics->SetJitterDelay(m_jitterBuffer->GetCurrentJitterDelay() / m_session.m_timeUnits);
  }
#endif

  CalculateStatistics(frame);
  return e_ProcessPacket;
}


OpalRTPSession::SendReceiveStatus OpalRTPSession::SyncSource::OnOutOfOrderPacket(RTP_DataFrame & frame)
{
  RTP_SequenceNumber sequenceNumber = frame.GetSequenceNumber();
  RTP_SequenceNumber expectedSequenceNumber = m_lastSequenceNumber + 1;

  bool waiting = true;
  if (m_pendingPackets.empty())
    m_waitOutOfOrderTimer = m_session.GetOutOfOrderWaitTime();
  else if (m_pendingPackets.GetSize() > m_session.GetMaxOutOfOrderPackets() || m_waitOutOfOrderTimer.HasExpired())
    waiting = false;

  PTRACE(m_pendingPackets.empty() ? 2 : 5, &m_session,
         "Session " << m_session.GetSessionID() << ", SSRC=" << RTP_TRACE_SRC(m_sourceIdentifier) << ", " <<
         (m_pendingPackets.empty() ? "first" : (waiting ? "next" : "last")) <<
         " out of order packet, got " << sequenceNumber << " expected " << expectedSequenceNumber);

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

    PTRACE(2, &m_session, "Session " << m_session.GetSessionID() << ", SSRC=" << RTP_TRACE_SRC(m_sourceIdentifier)
            << ", incorrect out of order packet, got "
            << sequenceNumber << " expected " << expectedSequenceNumber);
  }

  return e_ProcessPacket;
}


OpalRTPSession::SendReceiveStatus OpalRTPSession::SyncSource::GetPendingFrame(RTP_DataFrame & frame)
{
  if (m_pendingPackets.empty())
    return e_IgnorePacket;

  unsigned sequenceNumber = m_pendingPackets.back().GetSequenceNumber();
  unsigned expectedSequenceNumber = m_lastSequenceNumber + 1;
  if (sequenceNumber != expectedSequenceNumber) {
    PTRACE(5, &m_session, "Session " << m_session.GetSessionID() << ", "
              "SSRC=" << RTP_TRACE_SRC(m_pendingPackets.back().GetSyncSource()) << ", "
              "still out of order packets, next " << sequenceNumber << " expected " << expectedSequenceNumber);
    return e_IgnorePacket;
  }

  frame = m_pendingPackets.back();
  m_pendingPackets.pop_back();

  SendReceiveStatus status = OnReceiveData(frame, P_MAX_INDEX);
#if OPAL_RTP_FEC
  if (status == e_ProcessPacket && frame.GetPayloadType() == m_session.m_redundencyPayloadType)
    status = m_session.OnReceiveRedundantFrame(frame);
#endif

  PTRACE_IF(m_pendingPackets.empty() ? 2 : 5, status == e_ProcessPacket, &m_session,
          "Session " << m_session.GetSessionID() << ", SSRC=" << RTP_TRACE_SRC(m_sourceIdentifier) << ", "
          "resequenced " << (m_pendingPackets.empty() ? "last" : "next") << " out of order packet " << sequenceNumber);
  return status;
}


void OpalRTPSession::AttachTransport(Transport & transport)
{
  transport.DisallowDeleteObjects();

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

  m_endpoint.RegisterLocalRTP(this, false);
  transport.AllowDeleteObjects();
}


OpalMediaSession::Transport OpalRTPSession::DetachTransport()
{
  Transport temp;

  m_endpoint.RegisterLocalRTP(this, true);

  Shutdown(true);
  m_readMutex.Wait();
  m_dataMutex.Wait();

  for (int i = 1; i >= 0; --i) {
    if (m_socket[i] != NULL) {
      temp.Append(m_socket[i]);
      m_socket[i] = NULL;
    }
  }

  m_dataMutex.Signal();
  m_readMutex.Signal();

  PTRACE_IF(2, temp.IsEmpty(), "Detaching transport from closed session.");
  return temp;
}


void OpalRTPSession::SendBYE(RTP_SyncSourceId ssrc)
{
  if (!IsOpen())
    return;

  PWaitAndSignal mutex(m_dataMutex);
  SyncSourceMap::iterator it = m_SSRC.find(ssrc);
  if (it == m_SSRC.end())
    return;

  RTP_ControlFrame report;
  InitialiseControlFrame(report, *it->second);

  PTRACE(3, "Session " << m_sessionId << ", Sending BYE, SSRC=" << RTP_TRACE_SRC(ssrc));

  static char const ReasonStr[] = "Session ended";
  static size_t ReasonLen = sizeof(ReasonStr);

  // insert BYE
  report.StartNewPacket(RTP_ControlFrame::e_Goodbye);
  report.SetPayloadSize(4+1+ReasonLen);  // length is SSRC + ReasonLen + reason

  BYTE * payload = report.GetPayloadPtr();

  // one SSRC
  report.SetCount(1);
  *(PUInt32b *)payload = ssrc;

  // insert reason
  payload[4] = (BYTE)ReasonLen;
  memcpy((char *)(payload+5), ReasonStr, ReasonLen);

  report.EndPacket();
  WriteControl(report);

  delete it->second;
  m_SSRC.erase(it);
}


PString OpalRTPSession::GetCanonicalName(RTP_SyncSourceId ssrc, Direction dir) const
{
  PWaitAndSignal mutex(m_reportMutex);

  SyncSource * info;
  if (!GetSyncSource(ssrc, dir, info))
    return PString::Empty();

  PString s = info->m_canonicalName;
  s.MakeUnique();
  return s;
}


void OpalRTPSession::SetCanonicalName(const PString & name, RTP_SyncSourceId ssrc, Direction dir)
{
  PWaitAndSignal mutex(m_reportMutex);

  SyncSource * info;
  if (GetSyncSource(ssrc, dir, info)) {
    info->m_canonicalName = name;
    info->m_canonicalName.MakeUnique();
  }
}


PString OpalRTPSession::GetGroupId() const
{
  PWaitAndSignal mutex(m_reportMutex);
  PString s = m_groupId;
  s.MakeUnique();
  return s;
}


void OpalRTPSession::SetGroupId(const PString & id)
{
  PWaitAndSignal mutex(m_reportMutex);
  m_groupId = id;
  m_groupId.MakeUnique();
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


bool OpalRTPSession::ReadData(RTP_DataFrame & frame)
{
  if (!IsOpen() || m_shutdownRead)
    return false;

  for (SyncSourceMap::iterator it = m_SSRC.begin(); it != m_SSRC.end(); ++it) {
    switch (it->second->GetPendingFrame(frame)) {
      case e_AbortTransport:
        return false;
      case e_IgnorePacket:
        break;
      case e_ProcessPacket:
        return true;
    }
  }

  PWaitAndSignal mutex(m_readMutex);

  SendReceiveStatus receiveStatus = e_IgnorePacket;
  while (receiveStatus == e_IgnorePacket) {
    if (m_shutdownRead || PAssertNULL(m_socket[e_Data]) == NULL)
      return false;

    if (m_socket[e_Control] == NULL)
      receiveStatus = InternalReadData(frame);
    else {
      int selectStatus = PSocket::Select(*m_socket[e_Data], *m_socket[e_Control], m_maxNoReceiveTime);

      if (m_shutdownRead)
        return false;

      if (selectStatus > 0) {
        PTRACE(1, "Session " << m_sessionId << ", Select error: "
                << PChannel::GetErrorText((PChannel::Errors)selectStatus));
        return false;
      }

      if (selectStatus == 0)
        receiveStatus = OnReadTimeout(frame);

      if ((-selectStatus & 2) != 0) {
        if (InternalReadControl() == e_AbortTransport)
          return false;
      }

      if ((-selectStatus & 1) != 0)
        receiveStatus = InternalReadData(frame);
    }
  }

  return receiveStatus == e_ProcessPacket;
}


void OpalRTPSession::SyncSource::OnSendReceiverReport(RTP_ControlFrame::ReceiverReport & report, SyncSource & sender)
{
  report.ssrc = m_sourceIdentifier;

  unsigned lost = m_session.GetPacketsLost();
  if (m_jitterBuffer != NULL)
    lost += m_jitterBuffer->GetPacketsTooLate();
  report.SetLostPackets(lost);

  if (m_lastSequenceNumber >= m_lastRRSequenceNumber)
    report.fraction = (BYTE)((m_packetsLostSinceLastRR<<8)/(m_lastSequenceNumber - m_lastRRSequenceNumber + 1));
  else
    report.fraction = 0;
  m_packetsLostSinceLastRR = 0;

  report.last_seq = m_lastRRSequenceNumber;
  m_lastRRSequenceNumber = m_lastSequenceNumber;

  report.jitter = m_jitterAccum >> JitterRoundingGuardBits; // Allow for rounding protection bits

  PTime now;
  sender.m_lastReportTime = now; // Remember when we last sent RR

  // Time remote sent us an SR
  if (m_lastReportTime.IsValid()) {
    report.lsr  = (uint32_t)(m_syncRealTime.GetNTP() >> 16);
    report.dlsr = (uint32_t)((now - m_lastReportTime).GetMilliSeconds()*65536/1000); // Delay since last received SR
  }
  else {
    report.lsr = 0;
    report.dlsr = 0;
  }

  PTRACE(4, &m_session, "Session " << m_session.GetSessionID() << ", Sending ReceiverReport:"
            " SSRC=" << RTP_TRACE_SRC(m_sourceIdentifier)
         << " fraction=" << (unsigned)report.fraction
         << " lost=" << report.GetLostPackets()
         << " last_seq=" << report.last_seq
         << " jitter=" << report.jitter
         << " lsr=" << report.lsr
         << " dlsr=" << report.dlsr);
} 


void OpalRTPSession::SyncSource::OnRxSenderReport(const RTP_SenderReport & report)
{
  m_lastReportTime.SetCurrentTime(); // Remember when we received the SR
  m_syncTimestamp = report.rtpTimestamp;
  m_syncRealTime = report.realTimestamp;
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

  if (m_lastReportTime.IsValid())
    m_session.m_roundTripTime = std::max(1U,(unsigned)((PTime() - m_lastReportTime) - report.delay).GetMilliSeconds());
}


OpalRTPSession::SendReceiveStatus OpalRTPSession::OnSendData(RTP_DataFrame & frame, bool rewriteHeader)
{
  SendReceiveStatus status;
#if OPAL_ICE
  if ((status = OnSendICE(false)) != e_ProcessPacket)
    return status;
#else
  if (m_remotePort[e_Data] == 0)
    return e_IgnorePacket;
#endif // OPAL_ICE

  RTP_SyncSourceId ssrc = frame.GetSyncSource();
  SyncSource * syncSource;
  if (!GetSyncSource(ssrc, e_Transmit, syncSource)) {
    if ((ssrc = AddSyncSource(ssrc, e_Transmit)) == 0)
      return e_AbortTransport;
    GetSyncSource(ssrc, e_Transmit, syncSource);
  }

  status = syncSource->OnSendData(frame, rewriteHeader);

#if OPAL_RTP_FEC
  if (m_redundencyPayloadType != RTP_DataFrame::IllegalPayloadType) {
    status = OnSendRedundantFrame(frame);
    if (status != e_ProcessPacket)
      return status;
  }
#endif

  return status;
}


OpalRTPSession::SendReceiveStatus OpalRTPSession::OnSendControl(RTP_ControlFrame &)
{
  ++m_rtcpPacketsSent;

#if OPAL_ICE
  return OnSendICE(false);
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
    PTRACE(4, "Session " << GetSessionID() << ", ignoring left over audio packet from switch to T.38");
    return e_IgnorePacket; // Non fatal error, just ignore
  }

  RTP_SyncSourceId ssrc = frame.GetSyncSource();
  SyncSourceMap::iterator it = m_SSRC.find(ssrc);
  if (it == m_SSRC.end()) {
    if (!m_allowAnySyncSource) {
      PTRACE(4, "Session " << GetSessionID() << ", ignoring SSRC " << RTP_TRACE_SRC(ssrc));
      return e_IgnorePacket;
    }
    if (AddSyncSource(ssrc, e_Receive) != ssrc)
      return e_AbortTransport;
    it = m_SSRC.find(ssrc);
  }

  SendReceiveStatus status = it->second->OnReceiveData(frame, pduSize);

  // Final user handling of the read frame
  for (list<FilterNotifier>::iterator filter = m_filters.begin(); filter != m_filters.end(); ++filter) 
    (*filter)(frame, status);

  return status;
}


OpalRTPSession::SendReceiveStatus OpalRTPSession::OnOutOfOrderPacket(RTP_DataFrame & frame)
{
  SyncSource * ssrc;
  if (GetSyncSource(frame.GetSyncSource(), e_Receive, ssrc))
    return ssrc->OnOutOfOrderPacket(frame);

  return e_ProcessPacket;
}


void OpalRTPSession::InitialiseControlFrame(RTP_ControlFrame & report, SyncSource & sender)
{
  unsigned receivers = 0;
  for (SyncSourceMap::iterator it = m_SSRC.begin(); it != m_SSRC.end(); ++it) {
    if (it->second->m_direction == e_Receive && it->second->m_packets > 0)
      ++receivers;
  }

  RTP_ControlFrame::ReceiverReport * rr = NULL;

  // No packets sent yet, so only set RR
  if (sender.m_packets == 0) {

    // Send RR as we are not transmitting
    report.StartNewPacket(RTP_ControlFrame::e_ReceiverReport);

    // if no packets received, put in an empty report
    if (receivers == 0) {
      report.SetPayloadSize(sizeof(PUInt32b));  // length is SSRC 
      report.SetCount(0);

      // add the SSRC to the start of the payload
      *(PUInt32b *)report.GetPayloadPtr() = sender.m_sourceIdentifier;
      PTRACE(m_levelTxRR, "Session " << m_sessionId << ", Sending empty ReceiverReport");
    }
    else {
      report.SetPayloadSize(sizeof(PUInt32b) + receivers*sizeof(RTP_ControlFrame::ReceiverReport));  // length is SSRC of packet sender plus RRs
      report.SetCount(receivers);
      BYTE * payload = report.GetPayloadPtr();

      // add the SSRC to the start of the payload
      *(PUInt32b *)payload = sender.m_sourceIdentifier;

      // add the RR's after the SSRC
      rr = (RTP_ControlFrame::ReceiverReport *)(payload + sizeof(PUInt32b));
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
    sr->ntp_ts = PTime().GetNTP();
    sr->rtp_ts = sender.m_lastTimestamp;
    sr->psent  = sender.m_packets;
    sr->osent  = (uint32_t)sender.m_octets;

    PTRACE(m_levelTxRR, "Session " << m_sessionId << ", Sending SenderReport:"
              " SSRC=" << RTP_TRACE_SRC(sender.m_sourceIdentifier)
           << " ntp=0x" << hex << sr->ntp_ts << dec
           << " rtp=" << sr->rtp_ts
           << " psent=" << sr->psent
           << " osent=" << sr->osent);

    // add the RR's after the SR
    rr = (RTP_ControlFrame::ReceiverReport *)(payload + sizeof(PUInt32b) + sizeof(RTP_ControlFrame::SenderReport));
  }

  if (rr != NULL) {
    for (SyncSourceMap::iterator it = m_SSRC.begin(); it != m_SSRC.end(); ++it) {
      if (it->second->m_direction == e_Receive && it->second->m_packets > 0)
        it->second->OnSendReceiverReport(*rr++, sender);
    }
  }

  report.EndPacket();

  // Add the SDES part to compound RTCP packet
  PTRACE(m_levelTxRR, "Session " << m_sessionId << ", Sending SDES: " << sender.m_canonicalName);
  report.StartNewPacket(RTP_ControlFrame::e_SourceDescription);

  report.SetCount(0); // will be incremented automatically
  report.StartSourceDescription(sender.m_sourceIdentifier);
  report.AddSourceDescriptionItem(RTP_ControlFrame::e_CNAME, sender.m_canonicalName);
  report.AddSourceDescriptionItem(RTP_ControlFrame::e_TOOL, m_toolName);
  report.EndPacket();
  
#if PTRACING
  m_levelTxRR = 4;
#endif
}


void OpalRTPSession::TimedSendReport(PTimer&, P_INT_PTR)
{
  SendReport(false);
}


void OpalRTPSession::SendReport(bool force)
{
  PWaitAndSignal mutex(m_reportMutex);

  // Have not got anything yet, do nothing
  for (SyncSourceMap::iterator it = m_SSRC.begin(); it != m_SSRC.end(); ++it) {
    if (!it->second->m_direction == e_Receive && (force || it->second->m_packets > 0)) {
      RTP_ControlFrame report;
      InitialiseControlFrame(report, *it->second);

#if OPAL_RTCP_XR
      //Generate and send RTCP-XR packet
      if (it->second->m_metrics != NULL)
        it->second->m_metrics->InsertExtendedReportPacket(m_sessionId, it->second->m_sourceIdentifier, it->second->m_jitterBuffer, report);
#endif

      WriteControl(report);
    }
  }
}


#if OPAL_STATISTICS
void OpalRTPSession::GetStatistics(OpalMediaStatistics & statistics, bool receiver, RTP_SyncSourceId ssrc) const
{
  SyncSource * info;
  if (GetSyncSource(ssrc, receiver ? e_Receive : e_Transmit, info))
    info->GetStatistics(statistics);
  else {
    statistics.m_totalBytes        = 0;
    statistics.m_totalPackets      = 0;
    statistics.m_packetsLost       = 0;
    statistics.m_packetsOutOfOrder = 0;
    statistics.m_minimumPacketTime = 0;
    statistics.m_averagePacketTime = 0;
    statistics.m_maximumPacketTime = 0;
    statistics.m_averageJitter     = 0;
    statistics.m_maximumJitter     = 0;
  }

  statistics.m_roundTripTime = m_roundTripTime;
}
#endif


void OpalRTPSession::SyncSource::GetStatistics(OpalMediaStatistics & statistics) const
{
  statistics.m_startTime         = m_firstPacketTime;
  statistics.m_totalBytes        = m_octets;
  statistics.m_totalPackets      = m_packets;
  statistics.m_packetsLost       = m_packetsLost;
  statistics.m_packetsOutOfOrder = m_packetsOutOfOrder;
  statistics.m_minimumPacketTime = m_minimumPacketTime;
  statistics.m_averagePacketTime = m_averagePacketTime;
  statistics.m_maximumPacketTime = m_maximumPacketTime;
  statistics.m_averageJitter     = m_jitter;
  statistics.m_maximumJitter     = m_maximumJitter;
}


bool OpalRTPSession::CheckControlSSRC(RTP_SyncSourceId senderSSRC, RTP_SyncSourceId targetSSRC, SyncSource * & info PTRACE_PARAM(, const char * pduName)) const
{
  PTRACE_IF(4, m_SSRC.find(senderSSRC) != m_SSRC.end(),
            "Session " << m_sessionId << ", " << pduName << " from incorrect SSRC " << RTP_TRACE_SRC(senderSSRC));

  if (GetSyncSource(targetSSRC, e_Receive, info))
    return true;

  PTRACE(2, "Session " << m_sessionId << ", " << pduName << " for incorrect SSRC: " << RTP_TRACE_SRC(targetSSRC));
  return false;
}


OpalRTPSession::SendReceiveStatus OpalRTPSession::OnReceiveControl(RTP_ControlFrame & frame)
{
  if (frame.GetPacketSize() == 0)
    return e_IgnorePacket;

  PTRACE(6, "Session " << m_sessionId << ", OnReceiveControl - "
         << frame.GetPacketSize() << " bytes:\n"
         << hex << setprecision(2) << setfill('0') << PBYTEArray(frame, frame.GetPacketSize(), false) << dec);

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
          PTRACE(2, "Session " << m_sessionId << ", SenderReport packet truncated");
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
          PTRACE(2, "Session " << m_sessionId << ", ReceiverReport packet truncated");
        }
        break;
      }

      case RTP_ControlFrame::e_SourceDescription:
      {
        RTP_SourceDescriptionArray descriptions;
        if (frame.ParseSourceDescriptions(descriptions))
          OnRxSourceDescription(descriptions);
        else {
          PTRACE(2, "Session " << m_sessionId << ", SourceDescription packet malformed");
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
          PTRACE(2, "Session " << m_sessionId << ", Goodbye packet truncated");
        }
        break;
      }

      case RTP_ControlFrame::e_ApplDefined:
      {
        RTP_ControlFrame::ApplDefinedInfo info;
        if (frame.ParseApplDefined(info))
          OnRxApplDefined(info);
        else {
          PTRACE(2, "Session " << m_sessionId << ", ApplDefined packet truncated");
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
          PTRACE(2, "Session " << m_sessionId << ", ReceiverReport packet truncated");
        }
        break;
      }
#endif

      case RTP_ControlFrame::e_TransportLayerFeedBack :
        switch (frame.GetFbType()) {
          case RTP_ControlFrame::e_TransportNACK:
          {
            RTP_SyncSourceId senderSSRC, targetSSRC;
            std::set<unsigned> lostPackets;
            if (frame.ParseNACK(senderSSRC, targetSSRC, lostPackets)) {
              SyncSource * ssrc;
              if (CheckControlSSRC(senderSSRC, targetSSRC, ssrc PTRACE_PARAM(,"NACK")))
                OnRxNACK(targetSSRC, lostPackets);
            }
            else {
              PTRACE(2, "Session " << m_sessionId << ", NACK packet truncated");
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
                PTRACE(4, "Session " << m_sessionId << ", received TMMBR: rate=" << maxBitRate << ", SSRC=" << RTP_TRACE_SRC(targetSSRC));
                m_connection.ExecuteMediaCommand(OpalMediaFlowControl(maxBitRate, m_mediaType, m_sessionId, targetSSRC), true);
              }
            }
            else {
              PTRACE(2, "Session " << m_sessionId << ", TMMB" << (frame.GetFbType() == RTP_ControlFrame::e_TMMBR ? 'R' : 'N') << " packet truncated");
            }
            break;
          }
        }
        break;

#if OPAL_VIDEO
      case RTP_ControlFrame::e_IntraFrameRequest :
        PTRACE(4, "Session " << m_sessionId << ", received RFC2032 FIR");
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
                PTRACE(4, "Session " << m_sessionId << ", received RFC4585 PLI, SSRC=" << RTP_TRACE_SRC(targetSSRC));
                m_connection.ExecuteMediaCommand(OpalVideoPictureLoss(0, 0, m_sessionId, targetSSRC), true);
              }
            }
            else {
              PTRACE(2, "Session " << m_sessionId << ", PLI packet truncated");
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
                PTRACE(4, "Session " << m_sessionId << ", received RFC5104 FIR:"
                          " sn=" << sequenceNumber << ", last-sn=" << ssrc->m_lastFIRSequenceNumber
                       << ", SSRC=" << RTP_TRACE_SRC(targetSSRC));
                if (ssrc->m_lastFIRSequenceNumber != sequenceNumber) {
                  ssrc->m_lastFIRSequenceNumber = sequenceNumber;
                  m_connection.ExecuteMediaCommand(OpalVideoUpdatePicture(m_sessionId, targetSSRC), true);
                }
              }
            }
            else {
              PTRACE(2, "Session " << m_sessionId << ", FIR packet truncated");
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
                PTRACE(4, "Session " << m_sessionId << ", received TSTOR: " << ", tradeOff=" << tradeOff
                       << ", sn=" << sequenceNumber << ", last-sn=" << ssrc->m_lastTSTOSequenceNumber
                       << ", SSRC=" << RTP_TRACE_SRC(targetSSRC));
                if (ssrc->m_lastTSTOSequenceNumber != sequenceNumber) {
                  ssrc->m_lastTSTOSequenceNumber = sequenceNumber;
                  m_connection.ExecuteMediaCommand(OpalTemporalSpatialTradeOff(tradeOff, m_sessionId, targetSSRC), true);
                }
              }
            }
            else {
              PTRACE(2, "Session " << m_sessionId << ", TSTO packet truncated");
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
                PTRACE(4, "Session " << m_sessionId << ", received REMB: maxBitRate=" << maxBitRate
                       << ", SSRC=" << RTP_TRACE_SRC(targetSSRC));
                m_connection.ExecuteMediaCommand(OpalMediaFlowControl(maxBitRate, m_mediaType, m_sessionId, targetSSRC), true);
              }
              break;
            }
          }

          default :
            PTRACE(2, "Session " << m_sessionId << ", Unknown Payload Specific feedback type: " << frame.GetFbType());
        }
        break;
  
      default :
        PTRACE(2, "Session " << m_sessionId << ", Unknown control payload type: " << frame.GetPayloadType());
    }
  } while (frame.ReadNextPacket());

  return e_ProcessPacket;
}


void OpalRTPSession::OnRxSenderReport(const SenderReport & senderReport, const ReceiverReportArray & receiveReports)
{
#if PTRACING
  if (PTrace::CanTrace(m_levelRxSR)) {
    ostream & strm = PTrace::Begin(m_levelRxSR, __FILE__, __LINE__, this);
    strm << "Session " << m_sessionId << ", OnRxSenderReport: " << senderReport << '\n';
    for (PINDEX i = 0; i < receiveReports.GetSize(); i++)
      strm << "  RR: " << receiveReports[i] << '\n';
    strm << PTrace::End;
    m_levelRxSR = 4;
  }
#endif

  // This is report for their treanmitter, our receiver
  SyncSource * receiver;
  if (GetSyncSource(senderReport.sourceIdentifier, e_Receive, receiver))
    receiver->OnRxSenderReport(senderReport);

  OnRxReceiverReports(receiveReports);
}


void OpalRTPSession::OnRxReceiverReport(RTP_SyncSourceId PTRACE_PARAM(src), const ReceiverReportArray & reports)
{
#if PTRACING
  if (PTrace::CanTrace(m_levelRxRR)) {
    ostream & strm = PTrace::Begin(m_levelRxRR, __FILE__, __LINE__, this);
    strm << "Session " << m_sessionId << ", OnReceiverReport: SSRC=" << RTP_TRACE_SRC(src) << '\n';
    for (PINDEX i = 0; i < reports.GetSize(); i++)
      strm << "  RR: " << reports[i] << '\n';
    strm << PTrace::End;
    m_levelRxRR = 4;
  }
#endif
  OnRxReceiverReports(reports);
}


void OpalRTPSession::OnRxReceiverReports(const ReceiverReportArray & reports)
{
  for (PINDEX i = 0; i < reports.GetSize(); i++) {
    // This is report for their receiver, our transmitter
    SyncSource * sender;
    if (GetSyncSource(reports[i].sourceIdentifier, e_Transmit, sender))
      sender->OnRxReceiverReport(reports[i]);
  }
}


void OpalRTPSession::OnRxSourceDescription(const SourceDescriptionArray & PTRACE_PARAM(description))
{
#if PTRACING
  if (PTrace::CanTrace(m_levelRxSDES)) {
    ostream & strm = PTrace::Begin(m_levelRxSDES, __FILE__, __LINE__, this);
    strm << "Session " << m_sessionId << ", OnSourceDescription: " << description.GetSize() << " entries";
    for (PINDEX i = 0; i < description.GetSize(); i++)
      strm << "\n  " << description[i];
    strm << PTrace::End;
    m_levelRxSDES = 4;
  }
#endif
}


void OpalRTPSession::OnRxGoodbye(const RTP_SyncSourceArray & PTRACE_PARAM(src), const PString & PTRACE_PARAM(reason))
{
#if PTRACING
  if (PTrace::CanTrace(3)) {
    ostream & strm = PTrace::Begin(3, __FILE__, __LINE__, this);
    strm << "Session " << m_sessionId << ", OnGoodbye: " << reason << "\" SSRC=";
    for (PINDEX i = 0; i < src.GetSize(); i++)
      strm << RTP_TRACE_SRC(src[i]) << ' ';
    strm << PTrace::End;
  }
#endif
}


void OpalRTPSession::OnRxNACK(RTP_SyncSourceId PTRACE_PARAM(ssrc), const std::set<unsigned> PTRACE_PARAM(lostPackets))
{
  PTRACE(3, "Session " << m_sessionId << ", OnRxNACK: SSRC=" << RTP_TRACE_SRC(ssrc) << ", sn=" << lostPackets);
}


void OpalRTPSession::OnRxApplDefined(const RTP_ControlFrame::ApplDefinedInfo & info)
{
  PTRACE(3, "Session " << m_sessionId << ", OnApplDefined: \""
         << info.m_type << "\"-" << info.m_subType << " " << info.m_SSRC << " [" << info.m_data.GetSize() << ']');
  m_applDefinedNotifiers(*this, info);
}


bool OpalRTPSession::SendNACK(const std::set<unsigned> & lostPackets, RTP_SyncSourceId syncSourceIn)
{
  if (!(m_feedback&OpalMediaFormat::e_NACK)) {
    PTRACE(3, "Remote not capable of NACK");
    return false;
  }

  SyncSource * sender;
  if (!GetSyncSource(0, e_Transmit, sender))
    return false;

  SyncSource * receiver;
  if (!GetSyncSource(syncSourceIn, e_Receive, receiver))
    return false;

  RTP_ControlFrame request;
  InitialiseControlFrame(request, *sender);

  PTRACE(3, "Session " << m_sessionId << ", Sending NACK, "
            "SSRC=" << RTP_TRACE_SRC(receiver->m_sourceIdentifier) << ", "
            "lost=" << lostPackets);

  request.AddNACK(sender->m_sourceIdentifier, receiver->m_sourceIdentifier, lostPackets);

  // Send it
  request.EndPacket();
  return WriteControl(request);
}


bool OpalRTPSession::SendFlowControl(unsigned maxBitRate, unsigned overhead, bool notify, RTP_SyncSourceId syncSourceIn)
{
  if (!(m_feedback&(OpalMediaFormat::e_TMMBR|OpalMediaFormat::e_REMB))) {
    PTRACE(3, "Remote not capable of flow control (TMMBR or REMB)");
    return false;
  }

  SyncSource * sender;
  if (!GetSyncSource(0, e_Transmit, sender))
    return false;

  SyncSource * receiver;
  if (!GetSyncSource(syncSourceIn, e_Receive, receiver))
    return false;

  // Create packet
  RTP_ControlFrame request;
  InitialiseControlFrame(request, *sender);

  if (m_feedback&OpalMediaFormat::e_TMMBR) {
    if (overhead == 0)
      overhead = m_localAddress.GetVersion() == 4 ? (20 + 8 + 12) : (40 + 8 + 12);

    PTRACE(3, "Session " << m_sessionId << ", Sending TMMBR (flow control) "
           "rate=" << maxBitRate << ", overhead=" << overhead << ", "
           "SSRC=" << RTP_TRACE_SRC(receiver->m_sourceIdentifier));

    request.AddTMMB(sender->m_sourceIdentifier, receiver->m_sourceIdentifier, maxBitRate, overhead, notify);
  }
  else {
    PTRACE(3, "Session " << m_sessionId << ", Sending REMB (flow control) "
           "rate=" << maxBitRate << ", SSRC=" << RTP_TRACE_SRC(receiver->m_sourceIdentifier));

    request.AddREMB(sender->m_sourceIdentifier, receiver->m_sourceIdentifier, maxBitRate);
  }

  // Send it
  request.EndPacket();
  return WriteControl(request);
}


#if OPAL_VIDEO

bool OpalRTPSession::SendIntraFrameRequest(unsigned options, RTP_SyncSourceId syncSourceIn)
{
  SyncSource * sender;
  if (!GetSyncSource(0, e_Transmit, sender))
    return false;

  SyncSource * receiver;
  if (!GetSyncSource(syncSourceIn, e_Receive, receiver))
    return false;

  // Create packet
  RTP_ControlFrame request;
  InitialiseControlFrame(request, *sender);

  bool has_AVPF_PLI = (m_feedback & OpalMediaFormat::e_PLI) || (options & OPAL_OPT_VIDUP_METHOD_PLI);
  bool has_AVPF_FIR = (m_feedback & OpalMediaFormat::e_FIR) || (options & OPAL_OPT_VIDUP_METHOD_FIR);

  if ((has_AVPF_PLI && !has_AVPF_FIR) || (has_AVPF_PLI && (options & OPAL_OPT_VIDUP_METHOD_PREFER_PLI))) {
    PTRACE(3, "Session " << m_sessionId << ", Sending RFC4585 PLI"
           << ((options & OPAL_OPT_VIDUP_METHOD_PLI) ? " (forced)" : "")
           << ", SSRC=" << RTP_TRACE_SRC(receiver->m_sourceIdentifier));
    request.AddPLI(sender->m_sourceIdentifier, receiver->m_sourceIdentifier);
  }
  else if (has_AVPF_FIR) {
    PTRACE(3, "Session " << m_sessionId << ", Sending RFC5104 FIR"
           << ((options & OPAL_OPT_VIDUP_METHOD_FIR) ? " (forced)" : "")
           << ", SSRC=" << RTP_TRACE_SRC(receiver->m_sourceIdentifier));
    request.AddFIR(sender->m_sourceIdentifier, receiver->m_sourceIdentifier, receiver->m_lastFIRSequenceNumber++);
  }
  else {
    PTRACE(3, "Session " << m_sessionId << ", Sending RFC2032, SSRC=" << RTP_TRACE_SRC(syncSourceIn));
    request.AddIFR(receiver->m_sourceIdentifier);
  }

  // Send it
  request.EndPacket();
  return WriteControl(request);
}


bool OpalRTPSession::SendTemporalSpatialTradeOff(unsigned tradeOff, RTP_SyncSourceId syncSourceIn)
{
  if (!(m_feedback&OpalMediaFormat::e_TSTR)) {
    PTRACE(3, "Remote not capable of Temporal/Spatial Tradeoff (TSTR)");
    return false;
  }

  SyncSource * sender;
  if (!GetSyncSource(0, e_Transmit, sender))
    return false;

  SyncSource * receiver;
  if (!GetSyncSource(syncSourceIn, e_Receive, receiver))
    return false;

  RTP_ControlFrame request;
  InitialiseControlFrame(request, *sender);

  PTRACE(3, "Session " << m_sessionId << ", Sending TSTO (temporal spatial trade off) "
            "value=" << tradeOff << ", SSRC=" << RTP_TRACE_SRC(receiver->m_sourceIdentifier));

  request.AddTSTO(sender->m_sourceIdentifier, receiver->m_sourceIdentifier, tradeOff, sender->m_lastTSTOSequenceNumber++);

  // Send it
  request.EndPacket();
  return WriteControl(request);
}

#endif // OPAL_VIDEO


void OpalRTPSession::AddFilter(const FilterNotifier & filter)
{
  // ensures that a filter is added only once
  if (std::find(m_filters.begin(), m_filters.end(), filter) == m_filters.end())
    m_filters.push_back(filter);
}


void OpalRTPSession::SetJitterBuffer(OpalJitterBuffer * jitterBuffer, RTP_SyncSourceId ssrc)
{
  SyncSource * receiver;
  if (GetSyncSource(ssrc, e_Receive, receiver)) {
    receiver->m_jitterBuffer = jitterBuffer;
#if PTRACING
    static unsigned const Level = 4;
    if (PTrace::CanTrace(Level)) {
      ostream & trace = PTRACE_BEGIN(Level);
      if (jitterBuffer != NULL)
        trace << "Attached jitter buffer " << *jitterBuffer << " to";
      else
        trace << "Detached jitter buffer from";
      trace << "SSRC=" << RTP_TRACE_SRC(receiver->m_sourceIdentifier) << PTrace::End;
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
  if (m_localAddress.IsValid() && m_localPort[isMediaAddress] != 0)
    return OpalTransportAddress(m_localAddress, m_localPort[isMediaAddress], OpalTransportAddress::UdpPrefix());
  else
    return OpalTransportAddress();
}


OpalTransportAddress OpalRTPSession::GetRemoteAddress(bool isMediaAddress) const
{
  if (m_remoteAddress.IsValid() && m_remotePort[isMediaAddress] != 0)
    return OpalTransportAddress(m_remoteAddress, m_remotePort[isMediaAddress], OpalTransportAddress::UdpPrefix());
  else
    return OpalTransportAddress();
}


bool OpalRTPSession::UpdateMediaFormat(const OpalMediaFormat & mediaFormat)
{
  if (!OpalMediaSession::UpdateMediaFormat(mediaFormat))
    return false;

  m_feedback = mediaFormat.GetOptionEnum(OpalMediaFormat::RTCPFeedbackOption(), OpalMediaFormat::e_NoRTCPFb);
  return true;
}


OpalMediaStream * OpalRTPSession::CreateMediaStream(const OpalMediaFormat & mediaFormat, 
                                                    unsigned sessionId, 
                                                    bool isSource)
{
  if (m_socket[e_Data] != NULL) {
    PIPSocket::QoS qos = m_connection.GetEndPoint().GetManager().GetMediaQoS(m_mediaType);
    qos.m_remote.SetAddress(m_remoteAddress, m_remotePort[e_Data]);

    unsigned maxBitRate = mediaFormat.GetMaxBandwidth();
    if (maxBitRate != 0) {
      unsigned overheadBytes = m_localAddress.GetVersion() == 4 ? (20+8+12) : (40+8+12);
      unsigned overheadBits = overheadBytes*8;

      unsigned frameSize = mediaFormat.GetFrameSize();
      if (frameSize == 0)
        frameSize = m_connection.GetEndPoint().GetManager().GetMaxRtpPayloadSize();

      unsigned packetSize = frameSize*mediaFormat.GetOptionInteger(OpalAudioFormat::RxFramesPerPacketOption(), 1);

      qos.m_receive.m_maxPacketSize = packetSize + overheadBytes;
      packetSize *= 8;
      qos.m_receive.m_maxBandwidth = maxBitRate + (maxBitRate+packetSize-1)/packetSize * overheadBits;

      maxBitRate = mediaFormat.GetOptionInteger(OpalMediaFormat::TargetBitRateOption(), maxBitRate);
      packetSize = frameSize*mediaFormat.GetOptionInteger(OpalAudioFormat::TxFramesPerPacketOption(), 1);

      qos.m_transmit.m_maxPacketSize = packetSize + overheadBytes;
      packetSize *= 8;
      qos.m_transmit.m_maxBandwidth = maxBitRate + (maxBitRate+packetSize-1)/packetSize * overheadBits;
    }

    // Audio has tighter constraints to video
    if (m_isAudio) {
      qos.m_transmit.m_maxLatency = qos.m_receive.m_maxLatency = 250000; // 250ms
      qos.m_transmit.m_maxJitter = qos.m_receive.m_maxJitter = 100000; // 100ms
    }
    else {
      qos.m_transmit.m_maxLatency = qos.m_receive.m_maxLatency = 750000; // 750ms
      qos.m_transmit.m_maxJitter = qos.m_receive.m_maxJitter = 250000; // 250ms
    }
    m_socket[e_Data]->SetQoS(qos);
  }

  if (PAssert(m_sessionId == sessionId && m_mediaType == mediaFormat.GetMediaType(), PLogicError))
    return new OpalRTPMediaStream(dynamic_cast<OpalRTPConnection &>(m_connection), mediaFormat, isSource, *this);

  return NULL;
}


bool OpalRTPSession::Open(const PString & localInterface, const OpalTransportAddress & remoteAddress, bool mediaAddress)
{
  if (IsOpen())
    return true;

  PWaitAndSignal mutex1(m_readMutex);
  PWaitAndSignal mutex2(m_dataMutex);

  if (!OpalMediaSession::Open(localInterface, remoteAddress, mediaAddress))
    return false;

  m_firstControl = true;
  m_shutdownRead = false;
  m_shutdownWrite = false;

  for (int i = 0; i < 2; ++i) {
    delete m_socket[i];
    m_socket[i] = NULL;
  }

  PIPSocket::Address bindingAddress(localInterface);

  OpalManager & manager = m_connection.GetEndPoint().GetManager();

#if OPAL_PTLIB_NAT
  if (!manager.IsLocalAddress(m_remoteAddress)) {
    PNatMethod * natMethod = manager.GetNatMethods().GetMethod(bindingAddress, this);
    if (natMethod != NULL) {
      PTRACE(4, "NAT Method " << natMethod->GetMethodName() << " selected for call.");

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
          PTRACE(4, "Attempting natMethod: " << natMethod->GetMethodName());
          if (m_singlePortRx) {
            if (!natMethod->CreateSocket(m_socket[e_Data], bindingAddress)) {
              delete m_socket[e_Data];
              m_socket[e_Data] = NULL;
              PTRACE(2, "Session " << m_sessionId << ", " << natMethod->GetMethodName()
                     << " could not create NAT RTP socket, using normal sockets.");
            }
          }
          else if (natMethod->CreateSocketPair(m_socket[e_Data], m_socket[e_Control], bindingAddress, this)) {
            PTRACE(4, "Session " << m_sessionId << ", " << natMethod->GetMethodName() << " created NAT RTP/RTCP socket pair.");
          }
          else {
            PTRACE(2, "Session " << m_sessionId << ", " << natMethod->GetMethodName()
                   << " could not create NAT RTP/RTCP socket pair; trying to create individual sockets.");
            if (!natMethod->CreateSocket(m_socket[e_Data], bindingAddress, 0, this) ||
                !natMethod->CreateSocket(m_socket[e_Control], bindingAddress, 0, this)) {
              for (int i = 0; i < 2; ++i) {
                delete m_socket[i];
                m_socket[i] = NULL;
              }
              PTRACE(2, "Session " << m_sessionId << ", " << natMethod->GetMethodName()
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
      ok = manager.GetRtpIpPortRange().Listen(*m_socket[e_Data], bindingAddress);
    else {
      m_socket[e_Control] = new PUDPSocket();

      // Don't use for loop, they are in opposite order!
      PIPSocket * sockets[2];
      sockets[0] = m_socket[e_Data];
      sockets[1] = m_socket[e_Control];
      ok = manager.GetRtpIpPortRange().Listen(sockets, 2, bindingAddress);
    }

    if (!ok) {
      PTRACE(1, "RTPCon\tNo ports available for RTP session " << m_sessionId << ","
                " base=" << manager.GetRtpIpPortBase() << ","
                " max=" << manager.GetRtpIpPortMax() << ","
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

  manager.TranslateIPAddress(m_localAddress, m_remoteAddress);

  m_reportTimer.RunContinuous(m_reportTimer.GetResetTime());

  AddSyncSource(0, e_Transmit); // Add default transmit SSRC

  PTRACE(3, "Session " << m_sessionId << " opened: "
            " local=" << m_localAddress << ':' << m_localPort[e_Data] << '-' << m_localPort[e_Control]
         << " remote=" << m_remoteAddress);

  return true;
}


bool OpalRTPSession::IsOpen() const
{
  return m_socket[e_Data] != NULL && m_socket[e_Data]->IsOpen() &&
        (m_socket[e_Control] == NULL || m_socket[e_Control]->IsOpen());
}


bool OpalRTPSession::Close()
{
  bool ok = Shutdown(true) | Shutdown(false);

  m_reportTimer.Stop(true);

  m_endpoint.RegisterLocalRTP(this, true);

  m_readMutex.Wait();
  m_dataMutex.Wait();

  for (int i = 0; i < 2; ++i) {
    delete m_socket[i];
    m_socket[i] = NULL;
  }

  m_localAddress = PIPSocket::GetInvalidAddress();
  m_localPort[e_Data] = m_localPort[e_Control] = 0;
  m_remoteAddress = PIPSocket::GetInvalidAddress();
  m_remotePort[e_Data] = m_remotePort[e_Control] = 0;

  m_dataMutex.Signal();
  m_readMutex.Signal();

  return ok;
}


bool OpalRTPSession::Shutdown(bool reading)
{
  if (reading) {
    {
      PWaitAndSignal mutex(m_dataMutex);

      if (m_shutdownRead) {
        PTRACE(4, "Session " << m_sessionId << ", read already shut down .");
        return false;
      }

      PTRACE(3, "Session " << m_sessionId << ", shutting down read.");

      m_shutdownRead = true;

      if (m_socket[e_Data] != NULL) {
        PIPSocketAddressAndPort addrAndPort;
        m_socket[e_Data]->PUDPSocket::InternalGetLocalAddress(addrAndPort);
        if (!addrAndPort.IsValid())
          addrAndPort.Parse(PIPSocket::GetHostName());
        BYTE dummy = 0;
        PUDPSocket::Slice slice(&dummy, 1);
        if (!m_socket[e_Data]->PUDPSocket::InternalWriteTo(&slice, 1, addrAndPort)) {
          PTRACE(1, "Session " << m_sessionId << ", could not write to unblock read socket: "
                 << m_socket[e_Data]->GetErrorText(PChannel::LastReadError));
          m_socket[e_Data]->Close();
        }
      }
    }
  }
  else {
    if (m_shutdownWrite) {
      PTRACE(4, "Session " << m_sessionId << ", write already shut down .");
      return false;
    }

    PTRACE(3, "Session " << m_sessionId << ", shutting down write.");
    m_shutdownWrite = true;
  }

  // If shutting down write, no reporting any more
  if (m_shutdownWrite)
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
    m_noTransmitErrors = 0;
    m_reportTimer.RunContinuous(m_reportTimer.GetResetTime());
  }

  PTRACE(3, "Session " << m_sessionId << " reopened for " << (reading ? "reading" : "writing"));
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
  PWaitAndSignal m(m_dataMutex);

  if (m_remoteBehindNAT) {
    PTRACE(2, "Session " << m_sessionId << ", ignoring remote address as is behind NAT");
    return true;
  }

  PIPAddressAndPort ap;
  if (!remoteAddress.GetIpAndPort(ap))
    return false;

  return InternalSetRemoteAddress(ap, isMediaAddress PTRACE_PARAM(, "signalling"));
}


bool OpalRTPSession::InternalSetRemoteAddress(const PIPSocket::AddressAndPort & ap, bool isMediaAddress PTRACE_PARAM(, const char * source))
{
  WORD port = ap.GetPort();

  if (m_localAddress == ap.GetAddress() && m_remoteAddress == ap.GetAddress() && m_localPort[isMediaAddress] == port)
    return true;

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
    trace << "Session " << m_sessionId << ", " << source << " set remote "
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

  return true;
}


#if OPAL_ICE
void OpalRTPSession::SetICE(const PString & user, const PString & pass, const PNatCandidateList & candidates)
{
  delete m_stunServer;
  m_stunServer = NULL;

  delete m_stunClient;
  m_stunClient = NULL;

  OpalMediaSession::SetICE(user, pass, candidates);
  if (user.IsEmpty() || pass.IsEmpty())
    return;

  for (PNatCandidateList::const_iterator it = candidates.begin(); it != candidates.end(); ++it) {
    if (it->m_protocol == "udp" &&
        it->m_component >= PNatMethod::eComponent_RTP &&
        it->m_component <= PNatMethod::eComponent_RTCP &&
        it->m_localTransportAddress.GetPort() == m_remotePort[it->m_component-1])
      m_candidates[it->m_component-1].push_back(it->m_localTransportAddress);
  }

  m_stunServer = new PSTUNServer;
  m_stunServer->Open(m_socket[e_Data], m_socket[e_Control]);
  m_stunServer->SetCredentials(m_localUsername + ':' + m_remoteUsername, m_localPassword, PString::Empty());

  m_stunClient = new PSTUNClient;
  m_stunClient->SetCredentials(m_remoteUsername + ':' + m_localUsername, m_remoteUsername, PString::Empty());

  m_remoteBehindNAT = true;

  PTRACE(4, "Session " << m_sessionId << ", configured for ICE with candidates: "
            "data=" << m_candidates[e_Data].size() << ", " "control=" << m_candidates[e_Control].size());
}


OpalRTPSession::SendReceiveStatus OpalRTPSession::OnReceiveICE(bool fromDataChannel,
                                                               const BYTE * framePtr,
                                                               PINDEX frameSize,
                                                               const PIPSocket::AddressAndPort & ap)
{
  if (m_stunServer == NULL)
    return e_ProcessPacket;

  PSTUNMessage message(framePtr, frameSize, ap);
  if (!message.IsValid())
    return e_ProcessPacket;

  if (message.IsRequest()) {
    if (!m_stunServer->OnReceiveMessage(message, PSTUNServer::SocketInfo(m_socket[fromDataChannel])))
      return e_IgnorePacket;

    if (!m_candidates[fromDataChannel].empty() && message.FindAttribute(PSTUNAttribute::USE_CANDIDATE) == NULL)
      return e_IgnorePacket;
  }
  else {
    if (!m_stunClient->ValidateMessageIntegrity(message))
      return e_IgnorePacket;

    for (CandidateStates::const_iterator it = m_candidates[fromDataChannel].begin(); ; ++it) {
      if (it == m_candidates[fromDataChannel].end())
        return e_IgnorePacket;
      if (it->m_remoteAP == ap)
        break;
    }
  }

  if (!m_remoteAddress.IsValid())
    InternalSetRemoteAddress(ap, fromDataChannel PTRACE_PARAM(, "ICE"));
  m_candidates[fromDataChannel].clear();
  return e_IgnorePacket;
}


OpalRTPSession::SendReceiveStatus OpalRTPSession::OnSendICE(bool toDataChannel)
{
  if (m_candidates[toDataChannel].empty()) {
    if (m_remotePort[toDataChannel] != 0)
      return e_ProcessPacket;

    PTRACE(4, "Session " << m_sessionId << ", waiting for " << (toDataChannel ? "data" : "control") << " remote address.");

    while (m_remotePort[toDataChannel] == 0) {
      if (m_shutdownWrite)
        return e_AbortTransport;
      PThread::Sleep(10);
    }
  }

  for (CandidateStates::iterator it = m_candidates[toDataChannel].begin(); it != m_candidates[toDataChannel].end(); ++it) {
    PSTUNMessage request(PSTUNMessage::BindingRequest);
    m_stunClient->AppendMessageIntegrity(request);
    if (!request.Write(*m_socket[toDataChannel], it->m_remoteAP))
      return e_AbortTransport;
  }

  return e_IgnorePacket;
}


#endif // OPAL_ICE


OpalRTPSession::SendReceiveStatus OpalRTPSession::ReadRawPDU(BYTE * framePtr,
                                                             PINDEX & frameSize,
                                                             bool fromDataChannel)
{
#if PTRACING
  const char * channelName = fromDataChannel ? "Data" : "Control";
#endif
  PUDPSocket & socket = *m_socket[fromDataChannel];
  PIPSocket::AddressAndPort ap;

  if (socket.ReadFrom(framePtr, frameSize, ap)) {
    frameSize = socket.GetLastReadCount();

    // Ignore one byte packet from ourself, likely from the I/O block breaker in OpalRTPSession::Shutdown()
    if (frameSize == 1) {
      PIPSocketAddressAndPort localAP;
      socket.PUDPSocket::InternalGetLocalAddress(localAP);
      if (ap == localAP) {
        PTRACE(5, "Session " << m_sessionId << ", " << channelName << " I/O block breaker ignored.");
        return e_IgnorePacket;
      }
    }

#if OPAL_ICE
    SendReceiveStatus status = OnReceiveICE(fromDataChannel, framePtr, frameSize, ap);
    if (status != e_ProcessPacket)
      return status;
#endif // OPAL_ICE

    // If remote address never set from higher levels, then try and figure
    // it out from the first packet received.
    if (m_remotePort[fromDataChannel] == 0)
      InternalSetRemoteAddress(ap, fromDataChannel PTRACE_PARAM(, "first PDU"));

    m_noTransmitErrors = 0;

    return e_ProcessPacket;
  }

  switch (socket.GetErrorCode(PChannel::LastReadError)) {
    case PChannel::Unavailable :
      if (!HandleUnreachable(PTRACE_PARAM(channelName)))
        Shutdown(false); // Terminate transmission
      return e_IgnorePacket;

    case PChannel::BufferTooSmall :
      PTRACE(2, "Session " << m_sessionId << ", " << channelName
             << " read packet too large for buffer of " << frameSize << " bytes.");
      return e_IgnorePacket;

    case PChannel::Interrupted :
      PTRACE(4, "Session " << m_sessionId << ", " << channelName
             << " read packet interrupted.");
      // Shouldn't happen, but it does.
      return e_IgnorePacket;

    case PChannel::NoError :
      PTRACE(3, "Session " << m_sessionId << ", " << channelName
             << " received UDP packet with no payload.");
      return e_IgnorePacket;

    default:
      PTRACE(1, "Session " << m_sessionId << ", " << channelName
             << " read error (" << socket.GetErrorNumber(PChannel::LastReadError) << "): "
             << socket.GetErrorText(PChannel::LastReadError));
      m_connection.OnMediaFailed(m_sessionId, true);
      return e_AbortTransport;
  }
}


bool OpalRTPSession::HandleUnreachable(PTRACE_PARAM(const char * channelName))
{
  if (++m_noTransmitErrors == 1) {
    PTRACE(2, "Session " << m_sessionId << ", " << channelName << " port on remote not ready.");
    m_noTransmitTimer = m_maxNoTransmitTime;
    return true;
  }

  if (m_noTransmitErrors < 10 || m_noTransmitTimer.IsRunning())
    return true;

  PTRACE(2, "Session " << m_sessionId << ", " << channelName << ' '
         << m_maxNoTransmitTime << " seconds of transmit fails - informing connection");
  if (m_connection.OnMediaFailed(m_sessionId, false))
    return false;

  m_noTransmitErrors = 0;
  return true;
}


OpalRTPSession::SendReceiveStatus OpalRTPSession::InternalReadData(RTP_DataFrame & frame)
{
  if (!frame.SetMinSize(m_connection.GetEndPoint().GetManager().GetMaxRtpPacketSize()))
    return e_AbortTransport;

  PINDEX pduSize = frame.GetSize();
  SendReceiveStatus status = ReadRawPDU(frame.GetPointer(), pduSize, true);
  if (status != e_ProcessPacket)
    return status;

  // Check for single port operation, incoming RTCP on RTP
  RTP_ControlFrame control(frame, pduSize, false);
  unsigned type = control.GetPayloadType();
  if (type >= RTP_ControlFrame::e_FirstValidPayloadType && type <= RTP_ControlFrame::e_LastValidPayloadType) {
    status = OnReceiveControl(control);
    if (status == e_ProcessPacket)
      status = e_IgnorePacket;
  }
  else {
    status = OnReceiveData(frame, pduSize);
#if OPAL_RTP_FEC
    if (status == e_ProcessPacket && frame.GetPayloadType() == m_redundencyPayloadType)
      status = OnReceiveRedundantFrame(frame);
#endif
  }

  return status;
}


OpalRTPSession::SendReceiveStatus OpalRTPSession::OnReadTimeout(RTP_DataFrame & /*frame*/)
{
  if (m_connection.OnMediaFailed(m_sessionId, true))
    return e_AbortTransport;

  return e_IgnorePacket;
}


OpalRTPSession::SendReceiveStatus OpalRTPSession::InternalReadControl()
{
  PINDEX pduSize = 2048;
  RTP_ControlFrame frame(pduSize);
  SendReceiveStatus status = ReadRawPDU(frame.GetPointer(), pduSize, m_socket[e_Control] == NULL);
  if (status != e_ProcessPacket)
    return status;

  if (frame.SetPacketSize(pduSize))
    return OnReceiveControl(frame);

  PTRACE_IF(2, pduSize != 1 || !m_firstControl, "Session " << m_sessionId
            << ", Received control packet too small: " << pduSize << " bytes");
  return e_IgnorePacket;
}


bool OpalRTPSession::WriteData(RTP_DataFrame & frame,
                               const PIPSocketAddressAndPort * remote,
                               bool rewriteHeader)
{
  PWaitAndSignal m(m_dataMutex);

  while (IsOpen() && !m_shutdownWrite) {
    switch (OnSendData(frame, rewriteHeader)) {
      case e_ProcessPacket :
        return WriteRawPDU(frame, frame.GetPacketSize(), true, remote);

      case e_AbortTransport :
        return false;

      case e_IgnorePacket :
        m_dataMutex.Signal();
        PTRACE(5, "Session " << m_sessionId << ", data packet write delayed.");
        PThread::Sleep(20);
        m_dataMutex.Wait();
    }
  }

  PTRACE(3, "Session " << m_sessionId << ", data packet write shutdown.");
  return false;
}


bool OpalRTPSession::WriteControl(RTP_ControlFrame & frame, const PIPSocketAddressAndPort * remote)
{
  PWaitAndSignal m(m_dataMutex);

  while (IsOpen() && !m_shutdownWrite) {
    switch (OnSendControl(frame)) {
      case e_ProcessPacket :
        return WriteRawPDU(frame.GetPointer(), frame.GetPacketSize(), m_socket[e_Control] == NULL, remote);

      case e_AbortTransport :
        return false;

      case e_IgnorePacket :
        m_dataMutex.Signal();
        PTRACE(5, "Session " << m_sessionId << ", control packet write delayed.");
        PThread::Sleep(20);
        m_dataMutex.Wait();
    }
  }

  PTRACE(3, "Session " << m_sessionId << ", control packet write shutdown.");
  return false;
}


bool OpalRTPSession::WriteRawPDU(const BYTE * framePtr, PINDEX frameSize, bool toDataChannel, const PIPSocketAddressAndPort * remote)
{
  PIPSocketAddressAndPort remoteAddressAndPort;
  if (remote == NULL) {
    remoteAddressAndPort.SetAddress(m_remoteAddress, m_remotePort[toDataChannel]);

    // Trying to send a PDU before we are set up!
    if (!remoteAddressAndPort.IsValid())
      return true;

    remote = &remoteAddressAndPort;
  }

  PUDPSocket & socket = *m_socket[toDataChannel];
  do {
    if (socket.WriteTo(framePtr, frameSize, *remote))
      return true;
  } while (socket.GetErrorCode(PChannel::LastWriteError) == PChannel::Unavailable &&
           HandleUnreachable(PTRACE_PARAM(toDataChannel ? "Data" : "Control")));

  PTRACE(1, "Session " << m_sessionId
          << ", write (" << frameSize << " bytes) error on "
          << (toDataChannel ? "data" : "control") << " port ("
          << socket.GetErrorNumber(PChannel::LastWriteError) << "): "
          << socket.GetErrorText(PChannel::LastWriteError));
  m_connection.OnMediaFailed(m_sessionId, false);
  return false;
}


/////////////////////////////////////////////////////////////////////////////
