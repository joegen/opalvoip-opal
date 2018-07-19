/*
 * mediasession.cxx
 *
 * Media session abstraction
 *
 * Open Phone Abstraction Library (OPAL)
 *
 * Copyright (C) 2010 Vox Lucida Pty. Ltd.
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
 * The Original Code is Open Phone Abstraction Library.
 *
 * The Initial Developer of the Original Code is Vox Lucida Pty. Ltd.
 *
 * Contributor(s): ______________________________________.
 *
 */

#include <ptlib.h>

#ifdef P_USE_PRAGMA
#pragma implementation "mediasession.h"
#endif

#include <opal_config.h>

#include <opal/mediasession.h>
#include <opal/connection.h>
#include <opal/endpoint.h>
#include <opal/manager.h>
//#include <h323/h323caps.h>
#include <sdp/sdp.h>

#include <ptclib/random.h>
#include <ptclib/cypher.h>
#include <ptclib/pstunsrvr.h>


#define PTraceModule() "Media"
#define new PNEW


///////////////////////////////////////////////////////////////////////////////

#if OPAL_STATISTICS

OpalCodecStatistics::OpalCodecStatistics()
  : m_SSRC(0)
  , m_payloadType(-1)
  , m_threadIdentifier(PNullThreadIdentifier)
{
}


#if OPAL_ICE
OpalCandidateStatistics::OpalCandidateStatistics(const PNatCandidate & cand)
  : PNatCandidate(cand)
  , m_selected(false)
  , m_nominations(0)
  , m_lastNomination(0)
{
}

OpalCandidateStatistics::STUN::STUN()
  : m_first(0)
  , m_last(0)
  , m_count(0)
{
}

void OpalCandidateStatistics::STUN::Count()
{
  if (!m_first.IsValid())
    m_first.SetCurrentTime();
  m_last.SetCurrentTime();
  ++m_count;
}

static ostream & operator<<(ostream & strm, const OpalCandidateStatistics::STUN & stun)
{
  strm << " count=" << left << setw(5) << stun.m_count;

  if (stun.m_first.IsValid())
    strm << " first=" << stun.m_first.AsString(PTime::TodayFormat)
         << " last="  << stun.m_last .AsString(PTime::TodayFormat);

  return strm;
}

void OpalCandidateStatistics::PrintOn(ostream & strm) const
{
  strm.width(-1);
  PNatCandidate::PrintOn(strm);
  strm << "  tx-requests:" << m_txRequests << "  rx-requests:" << m_rxRequests;
  if (m_nominations > 0)
    strm << "  nominations: count=" << m_nominations << ' ' << m_lastNomination.AsString(PTime::TodayFormat);
  if (m_selected)
    strm << " SELECTED";
}
#endif // OPAL_ICE


OpalNetworkStatistics::OpalNetworkStatistics()
  : m_startTime(0)
  , m_totalBytes(0)
  , m_totalPackets(0)
  , m_controlPacketsIn(0)
  , m_controlPacketsOut(0)
  , m_NACKs(-1)
  , m_rtxSSRC(0)
  , m_rtxPackets(-1)
  , m_rtxDuplicates(-1)
  , m_FEC(-1)
  , m_unrecovered(-1)
  , m_packetsLost(-1)
  , m_maxConsecutiveLost(-1)
  , m_packetsOutOfOrder(-1)
  , m_lateOutOfOrder(-1)
  , m_packetsTooLate(-1)
  , m_packetOverruns(-1)
  , m_minimumPacketTime(-1)
  , m_averagePacketTime(-1)
  , m_maximumPacketTime(-1)
  , m_averageJitter(-1)
  , m_maximumJitter(-1)
  , m_jitterBufferDelay(-1)
  , m_roundTripTime(-1)
  , m_lastPacketAbsTime(0)
  , m_lastPacketNetTime(0)
  , m_lastReportTime(0)
  , m_targetBitRate(0)
  , m_targetFrameRate(0)
{
}


OpalMediaStatistics::UpdateInfo::UpdateInfo()
  : m_lastUpdateTime(0)
  , m_previousUpdateTime(0)
  , m_previousBytes(0)
  , m_previousPackets(0)
#if OPAL_VIDEO
  , m_previousFrames(0)
#endif
{
}


#if OPAL_VIDEO
OpalVideoStatistics::OpalVideoStatistics()
  : m_totalFrames(0)
  , m_keyFrames(0)
  , m_droppedFrames(0)
  , m_lastKeyFrameTime(0)
  , m_fullUpdateRequests(0)
  , m_pictureLossRequests(0)
  , m_lastUpdateRequestTime(0)
  , m_frameWidth(0)
  , m_frameHeight(0)
  , m_tsto(0)
  , m_videoQuality(-1)
{
}


void OpalVideoStatistics::IncrementFrames(bool key)
{
  ++m_totalFrames;
  if (key) {
    ++m_keyFrames;
    m_lastKeyFrameTime.SetCurrentTime();
    if (m_updateResponseTime == 0 && m_lastUpdateRequestTime.IsValid())
      m_updateResponseTime = m_lastKeyFrameTime - m_lastUpdateRequestTime;
  }
}


void OpalVideoStatistics::IncrementUpdateCount(bool full)
{
  if (full)
    ++m_fullUpdateRequests;
  else
    ++m_pictureLossRequests;
  m_lastUpdateRequestTime.SetCurrentTime();
  m_updateResponseTime = 0;
}

#endif // OPAL_VIDEO

#if OPAL_FAX
OpalFaxStatistics::OpalFaxStatistics()
  : m_result(FaxNotStarted)
  , m_phase(' ')
  , m_bitRate(9600)
  , m_compression(FaxCompressionUnknown)
  , m_errorCorrection(false)
  , m_txPages(-1)
  , m_rxPages(-1)
  , m_totalPages(0)
  , m_imageSize(0)
  , m_resolutionX(0)
  , m_resolutionY(0)
  , m_pageWidth(0)
  , m_pageHeight(0)
  , m_badRows(0)
  , m_mostBadRows(0)
  , m_errorCorrectionRetries(0)
{
}


ostream & operator<<(ostream & strm, OpalFaxStatistics::FaxCompression compression)
{
  static const char * const Names[] = { "N/A", "T.4 1d", "T.4 2d", "T.6" };
  if (compression >= 0 && (PINDEX)compression < PARRAYSIZE(Names))
    strm << Names[compression];
  else
    strm << (unsigned)compression;
  return strm;
}
#endif // OPAL_FAX


OpalMediaStatistics::OpalMediaStatistics()
#if OPAL_FAX
  : m_fax(*this) // Backward compatibility
#endif
{
}


OpalMediaStatistics::OpalMediaStatistics(const OpalMediaStatistics & other)
  : PObject(other)
  , OpalCodecStatistics(other)
  , OpalNetworkStatistics(other)
  , OpalVideoStatistics(other)
  , OpalFaxStatistics(other)
#if OPAL_FAX
  , m_fax(*this) // Backward compatibility
#endif
{
}


OpalMediaStatistics & OpalMediaStatistics::operator=(const OpalMediaStatistics & other)
{
  // Copy everything but m_updateInfo
  OpalNetworkStatistics::operator=(other);
  OpalVideoStatistics::operator=(other);
  OpalFaxStatistics::operator=(other);
  m_mediaType = other.m_mediaType;
  m_mediaFormat = other.m_mediaFormat;
  m_threadIdentifier = other.m_threadIdentifier;

  return *this;
}


void OpalMediaStatistics::PreUpdate()
{
  m_updateInfo.m_previousUpdateTime = m_updateInfo.m_lastUpdateTime;
  m_updateInfo.m_lastUpdateTime.SetCurrentTime();

  m_updateInfo.m_previousBytes = m_totalBytes;
  m_updateInfo.m_previousPackets = m_totalPackets;
  m_updateInfo.m_previousLost = m_packetsLost;
  if (m_maxConsecutiveLost > 0)
    m_maxConsecutiveLost = 0;
#if OPAL_VIDEO
  m_updateInfo.m_previousFrames = m_totalFrames;
#endif

  if (m_threadIdentifier != PNullThreadIdentifier) {
    PThread::Times times;
    PThread::GetTimes(m_threadIdentifier, times);
    if (times.m_real > 0) {
      m_updateInfo.m_previousCPU = m_updateInfo.m_usedCPU;
      m_updateInfo.m_usedCPU = times.m_kernel + times.m_user;
    }
  }
}


OpalMediaStatistics & OpalMediaStatistics::Update(const OpalMediaStream & stream)
{
  PreUpdate();
  stream.GetStatistics(*this);
  return *this;
}


bool OpalMediaStatistics::IsValid() const
{
  return m_updateInfo.m_lastUpdateTime.IsValid() &&
         m_updateInfo.m_previousUpdateTime.IsValid() &&
         m_updateInfo.m_lastUpdateTime > m_updateInfo.m_previousUpdateTime;
}


static PString InternalGetRate(const PTime & lastUpdate,
                               const PTime & previousUpdate,
                               int64_t lastValue,
                               int64_t previousValue,
                               const char * units,
                               unsigned significantFigures)
{
  PString str = "N/A";

  if (lastValue >= 0 && previousValue >= 0 && lastUpdate.IsValid() && previousUpdate.IsValid()) {
    int64_t milliseconds = (lastUpdate - previousUpdate).GetMilliSeconds();
    if (milliseconds == 0)
      str = '0';
    else {
      double value = (lastValue - previousValue)*1000.0/milliseconds;
      if (value == 0)
        str = '0';
      else if (value <= 1.0)
        str = PSTRSTRM(fixed << setprecision(significantFigures) << value);
      else
        str = PString(PString::ScaleSI, value, significantFigures);
    }
    str += units;
  }

  return str;
}


PString OpalMediaStatistics::GetRateStr(int64_t total, const char * units, unsigned significantFigures) const
{
  return InternalGetRate(m_updateInfo.m_lastUpdateTime, m_startTime, total, 0, units, significantFigures);
}


PString OpalMediaStatistics::GetRateStr(int64_t current, int64_t previous, const char * units, unsigned significantFigures) const
{
  return InternalGetRate(m_updateInfo.m_lastUpdateTime,
                         m_updateInfo.m_previousUpdateTime.IsValid() ? m_updateInfo.m_previousUpdateTime : m_startTime,
                         current, previous, units, significantFigures);
}


unsigned OpalMediaStatistics::GetRateInt(int64_t current, int64_t previous) const
{
  if (IsValid()) {
    int64_t milliseconds = (m_updateInfo.m_lastUpdateTime - m_updateInfo.m_previousUpdateTime).GetMilliSeconds();
    if (milliseconds > 0)
      return (unsigned)((current - previous)*1000/milliseconds);
  }
  return 0;
}


#if OPAL_VIDEO
PString OpalMediaStatistics::GetAverageFrameRate(const char * units, unsigned significantFigures) const
{
  if (m_mediaType != OpalMediaType::Video())
    return "N/A";
  return GetRateStr(m_totalFrames, units, significantFigures);
}


PString OpalMediaStatistics::GetCurrentFrameRate(const char * units, unsigned significantFigures) const
{
  if (m_mediaType != OpalMediaType::Video())
    return "N/A";
  return GetRateStr(m_totalFrames, m_updateInfo.m_previousFrames, units, significantFigures);
}
#endif


PString OpalMediaStatistics::GetCPU() const
{
  int64_t milliseconds = 1;
  if (m_updateInfo.m_usedCPU <= 0 ||
      m_updateInfo.m_previousCPU <= 0 ||
     !m_updateInfo.m_lastUpdateTime.IsValid() ||
     !m_updateInfo.m_previousUpdateTime.IsValid() ||
      m_updateInfo.m_lastUpdateTime <= m_updateInfo.m_previousUpdateTime ||
      (milliseconds = (m_updateInfo.m_lastUpdateTime - m_updateInfo.m_previousUpdateTime).GetMilliSeconds()) == 0)
    return "N/A";
  
  unsigned percentBy10 = (unsigned)((m_updateInfo.m_usedCPU - m_updateInfo.m_previousCPU).GetMilliSeconds()*1000/milliseconds);
  return psprintf("%u.%u%%", percentBy10/10, percentBy10%10);
}


static PString InternalTimeDiff(const PTime & lastUpdate, const PTime & previousUpdate)
{
  if (previousUpdate.IsValid())
    return ((lastUpdate.IsValid() ? lastUpdate : PTime()) - previousUpdate).AsString();

  return "N/A";
}


void OpalMediaStatistics::PrintOn(ostream & strm) const
{
  std::streamsize indent = strm.precision()+20;

  strm << setw(indent) <<            "Media format" << " = " << m_mediaFormat << " (" << m_mediaType << ")\n"
       << setw(indent) <<      "Session start time" << " = " << m_startTime << '\n'
       << setw(indent) <<        "Session duration" << " = " << InternalTimeDiff(m_updateInfo.m_lastUpdateTime, m_startTime)
                                                             << " (" << InternalTimeDiff(m_updateInfo.m_lastUpdateTime, m_updateInfo.m_previousUpdateTime) << ")\n"
       << setw(indent) <<               "CPU usage" << " = " << GetCPU() << '\n';
  if (!m_localAddress.IsEmpty())
    strm << setw(indent) <<         "Local address" << " = " << m_localAddress << '\n';
  if (!m_remoteAddress.IsEmpty())
    strm << setw(indent) <<        "Remote address" << " = " << m_remoteAddress << '\n';
  strm << setw(indent) <<             "Total bytes" << " = " << PString(PString::ScaleSI, m_totalBytes) << "B\n"
       << setw(indent) <<        "Average bit rate" << " = " << GetAverageBitRate("bps", 2) << '\n'
       << setw(indent) <<        "Current bit rate" << " = " << GetCurrentBitRate("bps", 3) << '\n'
       << setw(indent) <<           "Total packets" << " = " << m_totalPackets << '\n'
       << setw(indent) <<     "Average packet rate" << " = " << GetAveragePacketRate("pkt/s") << '\n'
       << setw(indent) <<     "Current packet rate" << " = " << GetCurrentPacketRate("pkt/s") << '\n'
       << setw(indent) <<     "Minimum packet time" << " = " << m_minimumPacketTime << "ms\n"
       << setw(indent) <<     "Average packet time" << " = " << m_averagePacketTime << "ms\n"
       << setw(indent) <<     "Maximum packet time" << " = " << m_maximumPacketTime << "ms\n"
       << setw(indent) <<            "Packets lost" << " = " << m_packetsLost << '\n'
       << setw(indent) <<   "Restored out of order" << " = " << m_packetsOutOfOrder << '\n'
       << setw(indent) <<       "Late out of order" << " = " << m_lateOutOfOrder << '\n'
       << setw(indent) <<                    "NACK" << " = " << m_NACKs << '\n'
       << setw(indent) <<                     "FEC" << " = " << m_FEC << '\n';

  if (m_roundTripTime >= 0)
    strm << setw(indent) <<       "Round Trip Time" << " = " << m_roundTripTime << '\n';

  if (m_mediaType == OpalMediaType::Audio()) {
    strm << setw(indent) <<           "JB too late" << " = " << m_packetsTooLate << '\n'
         << setw(indent) <<           "JB overruns" << " = " << m_packetOverruns << '\n';
    if (m_averageJitter >= 0 || m_maximumJitter >= 0)
      strm << setw(indent) <<      "Average jitter" << " = " << m_averageJitter << "ms\n"
           << setw(indent) <<      "Maximum jitter" << " = " << m_maximumJitter << "ms\n";
    if (m_jitterBufferDelay >= 0)
      strm << setw(indent) << "Jitter buffer delay" << " = " << m_jitterBufferDelay << "ms\n";
  }
#if OPAL_VIDEO
  else if (m_mediaType == OpalMediaType::Video()) {
    strm << setw(indent) <<    "Total video frames" << " = " << m_totalFrames << '\n'
         << setw(indent) <<    "Average Frame rate" << " = " << GetAverageFrameRate("fps", 1) << '\n'
         << setw(indent) <<    "Current Frame rate" << " = " << GetCurrentFrameRate("fps", 1) << '\n'
         << setw(indent) <<      "Total key frames" << " = " << m_keyFrames << '\n'
         << setw(indent) <<        "Dropped frames" << " = " << m_droppedFrames << '\n';
    if (m_videoQuality >= 0)
      strm << setw(indent) <<  "Video quality (QP)" << " = " << m_videoQuality << '\n';
  }
#endif
#if OPAL_FAX
  else if (m_mediaType == OpalMediaType::Fax()) {
    strm << setw(indent) <<          "Fax result" << " = " << m_fax.m_result << '\n'
         << setw(indent) <<   "Selected Bit rate" << " = " << m_fax.m_bitRate << '\n'
         << setw(indent) <<         "Compression" << " = " << m_fax.m_compression << '\n'
         << setw(indent) <<   "Total image pages" << " = " << m_fax.m_totalPages << '\n'
         << setw(indent) <<   "Total image bytes" << " = " << m_fax.m_imageSize << '\n'
         << setw(indent) <<          "Resolution" << " = " << m_fax.m_resolutionX << 'x' << m_fax.m_resolutionY << '\n'
         << setw(indent) <<     "Page dimensions" << " = " << m_fax.m_pageWidth << 'x' << m_fax.m_pageHeight << '\n'
         << setw(indent) <<   "Bad rows received" << " = " << m_fax.m_badRows << " (longest run=" << m_fax.m_mostBadRows << ")\n"
         << setw(indent) <<    "Error correction" << " = " << m_fax.m_errorCorrection << '\n'
         << setw(indent) <<       "Error retries" << " = " << m_fax.m_errorCorrectionRetries << '\n';
  }
#endif
  strm << '\n';
}
#endif

/////////////////////////////////////////////////////////////////////////////

const PCaselessString & OpalMediaCryptoSuite::ClearText() { static const PConstCaselessString s("Clear"); return s; }

void OpalMediaCryptoSuite::PrintOn(ostream & strm) const
{
    strm << GetDescription();
}


#if OPAL_H235_6 || OPAL_H235_8
H235SecurityCapability * OpalMediaCryptoSuite::CreateCapability(const H323Capability &) const
{
  return NULL;
}


OpalMediaCryptoSuite * OpalMediaCryptoSuite::FindByOID(const PString & oid)
{
  OpalMediaCryptoSuiteFactory::KeyList_T all = OpalMediaCryptoSuiteFactory::GetKeyList();
  for  (OpalMediaCryptoSuiteFactory::KeyList_T::iterator it = all.begin(); it != all.end(); ++it) {
    OpalMediaCryptoSuite * cryptoSuite = OpalMediaCryptoSuiteFactory::CreateInstance(*it);
    if (oid == cryptoSuite->GetOID())
      return cryptoSuite;
  }

  return NULL;
}
#endif // OPAL_H235_6 || OPAL_H235_8


void OpalMediaCryptoKeyList::Select(iterator & it)
{
  OpalMediaCryptoKeyInfo * keyInfo = &*it;
  DisallowDeleteObjects();
  erase(it);
  AllowDeleteObjects();
  RemoveAll();
  Append(keyInfo);
}


OpalMediaCryptoSuite::List OpalMediaCryptoSuite::FindAll(const PStringArray & cryptoSuiteNames, const char * prefix)
{
  List list;

  for (PINDEX i = 0; i < cryptoSuiteNames.GetSize(); ++i) {
    OpalMediaCryptoSuite * cryptoSuite = OpalMediaCryptoSuiteFactory::CreateInstance(cryptoSuiteNames[i]);
    if (cryptoSuite != NULL && (prefix == NULL || strncmp(cryptoSuite->GetDescription(), prefix, strlen(prefix)) == 0))
      list.Append(cryptoSuite);
  }

  return list;
}


class OpalMediaClearText : public OpalMediaCryptoSuite
{
  virtual const PCaselessString & GetFactoryName() const { return ClearText(); }
  virtual bool Supports(const PCaselessString &) const { return true; }
  virtual bool ChangeSessionType(PCaselessString & /*mediaSession*/, KeyExchangeModes modes) const { return modes&e_AllowClear; }
  virtual const char * GetDescription() const { return OpalMediaCryptoSuite::ClearText(); }
#if OPAL_H235_6 || OPAL_H235_8
  virtual const char * GetOID() const { return "0.0.8.235.0.3.26"; }
#endif
  virtual PINDEX GetCipherKeyBits() const { return 0; }
  virtual PINDEX GetAuthSaltBits() const { return 0; }

  struct KeyInfo : public OpalMediaCryptoKeyInfo
  {
    KeyInfo(const OpalMediaCryptoSuite & cryptoSuite) : OpalMediaCryptoKeyInfo(cryptoSuite) { }
    PObject * Clone() const { return new KeyInfo(m_cryptoSuite); }
    virtual bool IsValid() const { return true; }
    virtual void Randomise() { }
    virtual bool FromString(const PString &) { return true; }
    virtual PString ToString() const { return PString::Empty(); }
    virtual PBYTEArray GetCipherKey() const { return PBYTEArray(); }
    virtual PBYTEArray GetAuthSalt() const { return PBYTEArray(); }
    virtual bool SetCipherKey(const PBYTEArray &) { return true; }
    virtual bool SetAuthSalt(const PBYTEArray &) { return true; }
  };

  virtual OpalMediaCryptoKeyInfo * CreateKeyInfo() const { return new KeyInfo(*this); }
};

PFACTORY_CREATE(OpalMediaCryptoSuiteFactory, OpalMediaClearText, OpalMediaCryptoSuite::ClearText(), true);


//////////////////////////////////////////////////////////////////////////////

#if PTRACING
ostream & operator<<(ostream & strm, OpalMediaTransportChannelTypes::SubChannels subchannel)
{
  switch (subchannel) {
    case OpalMediaTransportChannelTypes::e_Media :
      return strm << "media";
    case OpalMediaTransportChannelTypes::e_Control :
      return strm << "control";
    default :
      return strm << "chan<" << subchannel << '>';
  }
}
#endif // PTRACING


OpalMediaTransport::OpalMediaTransport(const PString & name)
  : PSafeObject(m_instrumentedMutex)
  , m_name(name)
  , m_remoteBehindNAT(false)
  , m_packetSize(2048)
  , m_mtuDiscoverMode(-1)
  , m_mediaTimeout(0, 0, 5)       // Nothing received for 5 minutes
  , m_maxNoTransmitTime(0, 10)    // Sending data for 10 seconds, ICMP says still not there
  , m_opened(false)
  , m_established(false)
  , m_started(false)
  , m_congestionControl(NULL)
{
  m_ccTimer.SetNotifier(PCREATE_NOTIFIER(ProcessCongestionControl), "RTP-CC");
}


OpalMediaTransport::~OpalMediaTransport()
{
  m_ccTimer.Stop();
  delete m_congestionControl.exchange(NULL);

  InternalStop();
}


OpalMediaTransport::CongestionControl * OpalMediaTransport::SetCongestionControl(CongestionControl * cc)
{
  CongestionControl * previous = NULL;
  if (m_congestionControl.compare_exchange_strong(previous, cc)) {
    if (cc != NULL && !m_ccTimer.IsRunning())
      m_ccTimer.RunContinuous(cc->GetProcessInterval());
    return cc;
  }

  delete cc;
  return previous;
}


void OpalMediaTransport::ProcessCongestionControl(PTimer&, P_INT_PTR)
{
  PTRACE_CONTEXT_ID_PUSH_THREAD(*this);
  CongestionControl * cc = GetCongestionControl();
  if (cc != NULL && !cc->ProcessReceivedPackets())
    m_ccTimer.Stop();
}


void OpalMediaTransport::PrintOn(ostream & strm) const
{
  strm << m_name << ", ";
}


PString OpalMediaTransport::GetType()
{
  PString type;

  P_INSTRUMENTED_LOCK_READ_ONLY();
  if (lock.IsLocked() && !m_subchannels.empty()) {
    PChannel * channel = m_subchannels.front().m_channel;
    if (channel != NULL) {
      channel = channel->GetBaseReadChannel();
      if (channel != NULL) {
        type = channel->GetName();
        type.Delete(type.Find(':'), P_MAX_INDEX);
      }
    }
  }

  return type;
}


bool OpalMediaTransport::IsOpen() const
{
  return m_opened;
}


bool OpalMediaTransport::IsEstablished() const
{
  return m_established;
}


OpalTransportAddress OpalMediaTransport::GetLocalAddress(SubChannels subchannel) const
{
  OpalTransportAddress addr;

  P_INSTRUMENTED_LOCK_READ_ONLY();
  if (lock.IsLocked() && (size_t)subchannel < m_subchannels.size()) {
    addr = m_subchannels[subchannel].m_localAddress;
    addr.MakeUnique();
  }

  return addr;
}


OpalTransportAddress OpalMediaTransport::GetRemoteAddress(SubChannels subchannel) const
{
  OpalTransportAddress addr;

  P_INSTRUMENTED_LOCK_READ_ONLY();
  if (lock.IsLocked() && (size_t)subchannel < m_subchannels.size()) {
    addr = m_subchannels[subchannel].m_remoteAddress;
    addr.MakeUnique();
  }

  return addr;
}


bool OpalMediaTransport::SetRemoteAddress(const OpalTransportAddress &, SubChannels)
{
  return true;
}


void OpalMediaTransport::SetCandidates(const PString &, const PString &, const PNatCandidateList &)
{
}


bool OpalMediaTransport::GetCandidates(PString &, PString &, PNatCandidateList &, bool)
{
  return true;
}


#if OPAL_SRTP
bool OpalMediaTransport::GetKeyInfo(OpalMediaCryptoKeyInfo * [2])
{
  return false;
}
#endif


void OpalMediaTransport::AddReadNotifier(const ReadNotifier & notifier, SubChannels subchannel)
{
  P_INSTRUMENTED_LOCK_READ_WRITE(return);

  if ((size_t)subchannel < m_subchannels.size())
    m_subchannels[subchannel].m_notifiers.Add(notifier);
}


void OpalMediaTransport::RemoveReadNotifier(const ReadNotifier & notifier, SubChannels subchannel)
{
  P_INSTRUMENTED_LOCK_READ_WRITE(return);

  if ((size_t)subchannel < m_subchannels.size())
    m_subchannels[subchannel].m_notifiers.Remove(notifier);
}


void OpalMediaTransport::RemoveReadNotifier(PObject * target, SubChannels subchannel)
{
  P_INSTRUMENTED_LOCK_READ_WRITE(return);

  if (subchannel != e_AllSubChannels) {
    if ((size_t)subchannel < m_subchannels.size())
      m_subchannels[subchannel].m_notifiers.RemoveTarget(target);
  }
  else {
    for (vector<ChannelInfo>::iterator it = m_subchannels.begin(); it != m_subchannels.end(); ++it)
      it->m_notifiers.RemoveTarget(target);
  }
}


PChannel * OpalMediaTransport::GetChannel(SubChannels subchannel) const
{
  return (size_t)subchannel < m_subchannels.size() ? m_subchannels[subchannel].m_channel : NULL;
}


void OpalMediaTransport::SetMediaTimeout(const PTimeInterval & t)
{
  m_mediaTimeout = t;
  m_mediaTimer = t;
}


void OpalMediaTransport::SetRemoteBehindNAT()
{
  m_remoteBehindNAT = true;
}


#if OPAL_STATISTICS
void OpalMediaTransport::GetStatistics(OpalMediaStatistics & statistics) const
{
  statistics.m_transportName = m_name;
  statistics.m_localAddress  = GetLocalAddress(e_Media);
  statistics.m_remoteAddress = GetRemoteAddress(e_Media);
}
#endif


OpalMediaTransport::ChannelInfo::ChannelInfo(OpalMediaTransport & owner, SubChannels subchannel, PChannel * chan)
  : m_owner(owner)
  , m_subchannel(subchannel)
  , m_channel(chan)
  , m_thread(NULL)
  , m_consecutiveUnavailableErrors(0)
  , m_remoteAddressSource(e_RemoteAddressUnknown)
  PTRACE_PARAM(, m_logFirstRead(true))
{
}


void OpalMediaTransport::ChannelInfo::ThreadMain()
{
  PTRACE_CONTEXT_ID_PUSH_THREAD(m_owner);
  PTRACE(4, &m_owner, m_owner << m_subchannel << " media transport read thread starting");

  while (m_channel->IsOpen()) {
    PBYTEArray data(m_owner.m_packetSize);

    PTRACE(m_throttleReadPacket, &m_owner, m_owner << m_subchannel << " reading packet:"
           " sz=" << data.GetSize() << ","
           " timeout=" << m_channel->GetReadTimeout() << ","
           " if=" << m_localAddress);

    if (m_channel->Read(data.GetPointer(), data.GetSize())) {
      data.SetSize(m_channel->GetLastReadCount());
      PTRACE_IF(4, m_logFirstRead, &m_owner, m_owner << m_subchannel << " first receive data: sz=" << data.GetSize());
      PTRACE_PARAM(m_logFirstRead = false);
      m_owner.InternalRxData(m_subchannel, data);
    }
    else {
      P_INSTRUMENTED_LOCK_READ_ONLY2(lock, m_owner);
      if (!lock.IsLocked())
        break;

      switch (m_channel->GetErrorCode(PChannel::LastReadError)) {
        case PChannel::BufferTooSmall:
          PTRACE(2, &m_owner, m_owner << m_subchannel << " read packet too large for buffer of " << data.GetSize() << " bytes.");
          break;

        case PChannel::Interrupted:
          PTRACE(4, &m_owner, m_owner << m_subchannel << " read packet interrupted.");
          // Shouldn't happen, but it does.
          break;

        case PChannel::NoError:
          PTRACE(3, &m_owner, m_owner << m_subchannel << " received UDP packet with no payload.");
          break;

        case PChannel::Unavailable:
          if (m_owner.m_mediaTimer.IsRunning()) {
            HandleUnavailableError();
            break;
          }
          // Do timeout case

        case PChannel::Timeout:
          if (m_owner.m_mediaTimer.IsRunning())
            PTRACE(2, &m_owner, m_owner << m_subchannel << " timed out (" << m_channel->GetReadTimeout() << "s), other subchannels running");
          else {
            PTRACE(1, &m_owner, m_owner << m_subchannel << " timed out (" << m_owner.m_mediaTimeout << "s), closing");
            m_owner.InternalClose();
          }
          break;

        default:
          PTRACE(1, &m_owner, m_owner << m_subchannel
                 << " read error (" << m_channel->GetErrorNumber(PChannel::LastReadError) << "): "
                 << m_channel->GetErrorText(PChannel::LastReadError));
          m_owner.InternalClose();
          break;
      }
    }
  }

  // Send and empty packet to consumer to indicate transport has closed.
  if (m_owner.LockReadOnly(P_DEBUG_LOCATION)) {
    ChannelInfo::NotifierList notifiers = m_notifiers;
    m_owner.UnlockReadOnly(P_DEBUG_LOCATION);
    notifiers(m_owner, PBYTEArray());
  }

  PTRACE(4, &m_owner, m_owner << m_subchannel << " media transport read thread ended");
}


bool OpalMediaTransport::ChannelInfo::HandleUnavailableError()
{
  P_INSTRUMENTED_LOCK_READ_WRITE2(lock, m_owner);
  if (!lock.IsLocked())
    return false;

  if (m_timeForUnavailableErrors.HasExpired() && m_consecutiveUnavailableErrors < 10)
    m_consecutiveUnavailableErrors = 0;

  if (++m_consecutiveUnavailableErrors == 1) {
    PTRACE(2, &m_owner, m_owner << m_subchannel << " port on remote not ready: " << m_owner.GetRemoteAddress(m_subchannel));
    m_timeForUnavailableErrors = m_owner.m_maxNoTransmitTime;
    return true;
  }

  if (m_timeForUnavailableErrors.IsRunning() || m_consecutiveUnavailableErrors < 10)
    return true;

  PTRACE(2, &m_owner, m_owner << m_subchannel << ' ' << m_owner.m_maxNoTransmitTime
         << " seconds of transmit fails to " << m_owner.GetRemoteAddress(m_subchannel));
  m_owner.InternalClose();
  return false;
}


void OpalMediaTransport::InternalClose()
{
  P_INSTRUMENTED_LOCK_READ_ONLY(return);

  m_opened = m_established = false;

  for (vector<ChannelInfo>::iterator it = m_subchannels.begin(); it != m_subchannels.end(); ++it) {
    if (it->m_channel != NULL) {
      PChannel * base = it->m_channel->GetBaseReadChannel();
      if (base != NULL) {
        PTRACE(4, *this << it->m_subchannel << " closing.");
        base->Close();
      }
      else {   
        PTRACE(3, *this << it->m_subchannel << " already closed.");
      }
    }
    else {
      PTRACE(3, *this << it->m_subchannel << " not created.");
    }
  }
}


void OpalMediaTransport::InternalRxData(SubChannels subchannel, const PBYTEArray & data)
{
  // An empty packet indicates transport was closed, so don't send it here.
  if (data.IsEmpty())
    return;

  if (!LockReadOnly(P_DEBUG_LOCATION))
    return;

  ChannelInfo::NotifierList notifiers = m_subchannels[subchannel].m_notifiers;
  UnlockReadOnly(P_DEBUG_LOCATION);

  notifiers(*this, data);

  m_mediaTimer = m_mediaTimeout;
}


void OpalMediaTransport::Start()
{
  if (m_started.exchange(true))
    return;

  P_INSTRUMENTED_LOCK_READ_WRITE();
  if (!lock.IsLocked())
    return;

  PTRACE(4, *this << "starting read theads, " << m_subchannels.size() << " sub-channels");
  for (ChannelArray::iterator it = m_subchannels.begin(); it != m_subchannels.end(); ++it) {
    if (it->m_channel != NULL && it->m_thread == NULL) {
      PStringStream threadName;
      threadName << m_name;
      if (m_subchannels.size() > 1)
        threadName << '-' << it->m_subchannel;
      threadName.Replace(" Session ", "-");
      threadName.Replace(" bundle", "-B");
      it->m_thread = new PThreadObj<ChannelInfo>(*it, &ChannelInfo::ThreadMain, false, threadName, PThread::HighPriority);
    }
  }
}


void OpalMediaTransport::InternalStop()
{
  if (m_subchannels.empty())
    return;

  PTRACE(4, *this << "stopping " << m_subchannels.size() << " subchannels.");
  InternalClose();

  for (vector<ChannelInfo>::iterator it = m_subchannels.begin(); it != m_subchannels.end(); ++it)
    PThread::WaitAndDelete(it->m_thread);

  P_INSTRUMENTED_LOCK_READ_WRITE();

  for (vector<ChannelInfo>::iterator it = m_subchannels.begin(); it != m_subchannels.end(); ++it)
    delete it->m_channel;
  m_subchannels.clear();

  PTRACE(4, *this << "stopped");
}


void OpalMediaTransport::AddChannel(PChannel * channel)
{
  SubChannels subchannel = (SubChannels)m_subchannels.size();

  PTRACE_CONTEXT_ID_TO(channel);
  channel = AddWrapperChannels(subchannel, channel);

  /* Make socket timeout slightly longer (200ms) than media timeout to avoid
      a race condition with m_mediaTimer expiring. */
  channel->SetReadTimeout(m_mediaTimeout+200);

  m_subchannels.push_back(ChannelInfo(*this, subchannel, channel));
  PTRACE(5, "Added " << subchannel << " channel " << channel->GetName());
}


PChannel * OpalMediaTransport::AddWrapperChannels(SubChannels /*subchannel*/, PChannel * channel)
{
  return channel;
}


//////////////////////////////////////////////////////////////////////////////

OpalTCPMediaTransport::OpalTCPMediaTransport(const PString & name)
  : OpalMediaTransport(name)
{
  AddChannel(new PTCPSocket);
}


bool OpalTCPMediaTransport::Open(OpalMediaSession &, PINDEX, const PString & localInterface, const OpalTransportAddress &)
{
  return m_opened = dynamic_cast<PTCPSocket &>(*m_subchannels[0].m_channel).Listen(PIPAddress(localInterface));
}


bool OpalTCPMediaTransport::SetRemoteAddress(const OpalTransportAddress & remoteAddress, PINDEX)
{
  PIPSocketAddressAndPort ap;
  if (!remoteAddress.GetIpAndPort(ap) || !ap.IsValid() || ap.GetAddress().IsAny())
    return false;

  PTCPSocket & socket = dynamic_cast<PTCPSocket &>(*m_subchannels[0].m_channel);
  socket.SetPort(ap.GetPort());
  if (!socket.Connect(ap.GetAddress()))
    return false;

  m_subchannels[0].m_remoteAddressSource = e_RemoteAddressFromSignalling;
  m_established = true;
  return true;
}


bool OpalTCPMediaTransport::Write(const void * data, PINDEX length, SubChannels subchannel, const PIPSocketAddressAndPort *, int *)
{
  P_INSTRUMENTED_LOCK_READ_ONLY(return false);

  PChannel * channel;
  if ((size_t)subchannel >= m_subchannels.size() || (channel = m_subchannels[subchannel].m_channel) == NULL) {
    PTRACE(4, *this << "write to closed/unopened subchannel " << subchannel);
    return false;
  }

  if (channel->Write(data, length))
    return true;

  PTRACE(1, *this << "write (" << length << " bytes) error"
            " on " << subchannel << " subchannel"
            " (" << channel->GetErrorNumber(PChannel::LastWriteError) << "):"
            " " << channel->GetErrorText(PChannel::LastWriteError));
  return false;
}


//////////////////////////////////////////////////////////////////////////////

OpalUDPMediaTransport::OpalUDPMediaTransport(const PString & name)
  : OpalMediaTransport(name)
  , m_localHasRestrictedNAT(false)
{
}


bool OpalUDPMediaTransport::SetRemoteAddress(const OpalTransportAddress & remoteAddress, SubChannels subchannel)
{
  PIPAddressAndPort ap;
  if (!remoteAddress.GetIpAndPort(ap))
    return false;

  return InternalSetRemoteAddress(ap, subchannel, e_RemoteAddressFromSignalling);
}


void OpalUDPMediaTransport::InternalRxData(SubChannels subchannel, const PBYTEArray & data)
{
  if (m_remoteBehindNAT) {
    // If remote address never set from higher levels, then try and figure
    // it out from the first packet received.
    PIPAddressAndPort ap;
    GetSubChannelAsSocket(subchannel)->GetLastReceiveAddress(ap);
    InternalSetRemoteAddress(ap, subchannel, e_RemoteAddressFromFirstPacket);
    if (subchannel == e_Control) {
      ap.SetPort(ap.GetPort() - 1);
      InternalSetRemoteAddress(ap, e_Data, e_RemoteAddressFromProvisionalPair);
    }
    else if (subchannel == e_Data && m_subchannels.size() > e_Control) {
      ap.SetPort(ap.GetPort() + 1);
      InternalSetRemoteAddress(ap, e_Control, e_RemoteAddressFromProvisionalPair);
    }
  }

  OpalMediaTransport::InternalRxData(subchannel, data);
}


#if PTRACING
ostream & operator<<(ostream & strm, OpalUDPMediaTransport::RemoteAddressSources source)
{
  switch (source) {
    case OpalUDPMediaTransport::e_RemoteAddressFromSignalling :
      return strm << "signalling";
    case OpalUDPMediaTransport::e_RemoteAddressFromFirstPacket :
      return strm << "first packet";
    case OpalUDPMediaTransport::e_RemoteAddressFromProvisionalPair :
      return strm << "first paired packet";
    case OpalUDPMediaTransport::e_RemoteAddressFromICE :
      return strm << "ICE";
    default :
      return strm;
  }
}
#endif

bool OpalUDPMediaTransport::InternalSetRemoteAddress(const PIPSocket::AddressAndPort & newAP,
                                                     SubChannels subchannel,
                                                     RemoteAddressSources source)
{
  if (!newAP.IsValid())
    return false;

  P_INSTRUMENTED_LOCK_READ_WRITE();
  if (!lock.IsLocked())
    return false;

  PUDPSocket * socket = GetSubChannelAsSocket(subchannel);
  if (socket == NULL)
    return false;

  PIPSocketAddressAndPort oldAP;
  if (socket->GetLocalAddress(oldAP) && newAP == oldAP) {
    PTRACE(2, *this << source << " cannot set remote address/port to same as local: " << oldAP);
    return false;
  }

  socket->GetSendAddress(oldAP);
  if (newAP == oldAP)
    return true;

  ChannelInfo & info = m_subchannels[subchannel];

  switch (info.m_remoteAddressSource) {
    case e_RemoteAddressFromFirstPacket :
      if (source == e_RemoteAddressFromProvisionalPair) {
        PTRACE(3, *this << source << " cannot set remote address/port to " << newAP << ", already set to " << oldAP);
        return false;
      }
      break;

    case e_RemoteAddressFromProvisionalPair :
      if (source != e_RemoteAddressFromFirstPacket) {
        PTRACE(3, *this << source << " is awaiting remote address/port, ignoring " << newAP);
        return false;
      }
      break;

    default :
      if (m_remoteBehindNAT && source == e_RemoteAddressFromSignalling) {
        PTRACE(3, *this << "ignoring signalled remote address, as is behind NAT, or ICE in operation.");
        return true;
      }
      break;
  }

  info.m_remoteAddressSource = source;
  info.m_remoteAddress = OpalTransportAddress(newAP, OpalTransportAddress::UdpPrefix());
  info.m_consecutiveUnavailableErrors = 0; // Prevent errors from previous address.
  if (socket->SetSendAddress(newAP, m_mtuDiscoverMode))
    PTRACE_IF(3, m_mtuDiscoverMode >= 0, *this << source << " enabling MTU discovery mode " << m_mtuDiscoverMode);
  else
    PTRACE(2, *this << source << " cannot enable MTU discovery: " << socket->GetErrorText());

  if (m_localHasRestrictedNAT) {
    // If have Port Restricted NAT on local host then send a datagram
    // to remote to open up the port in the firewall for return data.
    if (!InternalOpenPinHole(*socket))
      return false;
  }

  if (subchannel == e_Data)
    m_established = true;

#if PTRACING
  static const int Level = 3;
  if (PTrace::CanTrace(Level)) {
    ostream & trace = PTRACE_BEGIN(Level);
    trace << *this << source << " set remote "
          << subchannel << " address to " << newAP;
    for (ChannelArray::iterator it = m_subchannels.begin(); it != m_subchannels.end(); ++it)
      trace << ", " << it->m_subchannel
            << " rem=" << GetRemoteAddress(it->m_subchannel)
            << " local=" << GetLocalAddress(it->m_subchannel);
    if (m_localHasRestrictedNAT)
      trace << ", restricted NAT";
    trace << PTrace::End;
  }
#endif

  return true;
}


bool OpalUDPMediaTransport::InternalOpenPinHole(PUDPSocket & socket)
{
    static const BYTE dummy[1] = { 0 };
    return socket.Write(dummy, sizeof(dummy));
}


#if PTRACING
#define SetMinBufferSize(sock, bufType, newSize) SetMinBufferSizeFn(sock, bufType, newSize, #bufType)
static void SetMinBufferSizeFn(PUDPSocket & sock, int bufType, int newSize, const char * bufTypeName)
#else
static void SetMinBufferSize(PUDPSocket & sock, int bufType, int newSize)
#endif
{
  int originalSize = 0;
  if (!sock.GetOption(bufType, originalSize)) {
    PTRACE(1, &sock, "GetOption(" << sock.GetHandle() << ',' << bufTypeName << ")"
              " failed: " << sock.GetErrorText());
    return;
  }

  // Already big enough
  if (originalSize >= newSize) {
    PTRACE(4, &sock, "SetOption(" << sock.GetHandle() << ',' << bufTypeName << ',' << newSize << ")"
              " unecessary, already " << originalSize);
    return;
  }

  for (; newSize >= 1024; newSize -= newSize/10) {
    // Set to new size
    if (!sock.SetOption(bufType, newSize)) {
      PTRACE(1, &sock, "SetOption(" << sock.GetHandle() << ',' << bufTypeName << ',' << newSize << ")"
                " failed: " << sock.GetErrorText());
      continue;
    }

    // As some stacks lie about setting the buffer size, we double check.
    int adjustedSize;
    if (!sock.GetOption(bufType, adjustedSize)) {
      PTRACE(1, &sock, "GetOption(" << sock.GetHandle() << ',' << bufTypeName << ")"
                " failed: " << sock.GetErrorText());
      return;
    }

    if (adjustedSize >= newSize) {
      PTRACE(4, &sock, "SetOption(" << sock.GetHandle() << ',' << bufTypeName << ',' << newSize << ")"
                " succeeded, actually " << adjustedSize);
      return;
    }

    if (adjustedSize > originalSize) {
      PTRACE(4, &sock, "SetOption(" << sock.GetHandle() << ',' << bufTypeName << ',' << newSize << ")"
                " clamped to maximum " << adjustedSize);
      return;
    }

    PTRACE(2, &sock, "SetOption(" << sock.GetHandle() << ',' << bufTypeName << ',' << newSize << ")"
              " failed, even though it said it succeeded!");
  }
}


bool OpalUDPMediaTransport::Open(OpalMediaSession & session,
                                 PINDEX subchannelCount,
                                 const PString & localInterface,
                                 const OpalTransportAddress & remoteAddress)
{
  PTRACE_CONTEXT_ID_FROM(session);

  if (!PAssert(subchannelCount > 0, PInvalidParameter))
    return false;

  OpalManager & manager = session.GetConnection().GetEndPoint().GetManager();

  m_packetSize = manager.GetMaxRtpPacketSize();
  if (session.IsRemoteBehindNAT())
    SetRemoteBehindNAT();
  m_mediaTimeout = session.GetStringOptions().GetVar(OPAL_OPT_MEDIA_RX_TIMEOUT, manager.GetNoMediaTimeout());
  m_maxNoTransmitTime = session.GetStringOptions().GetVar(OPAL_OPT_MEDIA_TX_TIMEOUT, manager.GetTxMediaTimeout());

  PIPAddress bindingIP(localInterface);
  if (!bindingIP.IsValid()) {
    PTRACE(2, session << "open failed, illegal local interface \"" << localInterface << '"');
    return false;
  }

  PIPAddress remoteIP;
  remoteAddress.GetIpAddress(remoteIP);

  PTRACE(4, session << "opening " << subchannelCount << " subchannel(s):"
            " interface=\"" << localInterface << "\" local=" << bindingIP << " remote=" << remoteIP);

#if OPAL_PTLIB_NAT
  if (!manager.IsLocalAddress(remoteIP)) {
    PNatMethod * natMethod = manager.GetNatMethods().GetMethod(bindingIP, this);
    if (natMethod != NULL) {
      PTRACE(4, session << "NAT Method " << natMethod->GetMethodName() << " selected for call.");

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
          PTRACE(4, session << "attempting natMethod: " << natMethod->GetMethodName());
          if (subchannelCount == 2) {
            PUDPSocket * dataSocket = NULL;
            PUDPSocket * controlSocket = NULL;
            if (natMethod->CreateSocketPair(dataSocket, controlSocket, bindingIP, &session)) {
              AddChannel(dataSocket);
              AddChannel(controlSocket);
              PTRACE(4, session << natMethod->GetMethodName() << " created NAT RTP/RTCP socket pair.");
              break;
            }

            PTRACE(2, session << natMethod->GetMethodName()
                    << " could not create NAT RTP/RTCP socket pair; trying to create individual sockets.");
          }

          for (PINDEX i = 0; i < subchannelCount; ++i) {
            PUDPSocket * socket = NULL;
            if (natMethod->CreateSocket(socket, bindingIP))
              AddChannel(socket);
            else {
              delete socket;
              PTRACE(2, session << natMethod->GetMethodName()
                      << " could not create NAT RTP socket, using normal sockets.");
            }
          }
          break;

        default :
          /* We cannot use NAT traversal method (e.g. STUN) to create sockets
             in the remaining modes as the NAT router will then not let us
             talk to the real RTP destination. All we can so is bind to the
             local interface the NAT is on and hope the NAT router is doing
             something sneaky like symmetric port forwarding. */
          break;
      }
    }
  }
#endif // OPAL_PTLIB_NAT

  if (m_subchannels.size() < (size_t)subchannelCount) {
    PINDEX extraSockets = subchannelCount - m_subchannels.size();
    vector<PIPSocket*> sockets(extraSockets);
    for (PINDEX i = 0; i < extraSockets; ++i)
      sockets[i] = new PUDPSocket();
    if (!manager.GetRtpIpPortRange().Listen(sockets.data(), extraSockets, bindingIP)) {
      PTRACE(1, session << "no ports available,"
                " base=" << manager.GetRtpIpPortBase() << ","
                " max=" << manager.GetRtpIpPortMax() << ","
                " bind=" << bindingIP);
      for (PINDEX i = 0; i < extraSockets; ++i)
        delete sockets[i];
      return false; // Used up all the available ports!
    }
    for (PINDEX i = 0; i < extraSockets; ++i)
      AddChannel(sockets[i]);
  }

  for (ChannelArray::iterator it = m_subchannels.begin(); it != m_subchannels.end(); ++it) {
    PUDPSocket & socket = *dynamic_cast<PUDPSocket *>(it->m_channel->GetBaseReadChannel());
    m_socketCache.push_back(&socket);

    PIPSocketAddressAndPort ap;
    if (socket.GetLocalAddress(ap) && ap.IsValid())
      it->m_localAddress = OpalTransportAddress(ap, OpalTransportAddress::UdpPrefix());

    // Increase internal buffer size on media UDP sockets
    SetMinBufferSize(socket, SO_RCVBUF, session.GetMediaType() == OpalMediaType::Audio() ? 0x4000 : 0x100000);
    SetMinBufferSize(socket, SO_SNDBUF, 0x2000);
  }
  m_mediaTimer = m_mediaTimeout;

  m_opened = true;
  return true;
}


bool OpalUDPMediaTransport::Write(const void * data, PINDEX length, SubChannels subchannel, const PIPSocketAddressAndPort * dest, int * mtu)
{
  P_INSTRUMENTED_LOCK_READ_ONLY(return false);

  PUDPSocket * socket = GetSubChannelAsSocket(subchannel);
  if (socket == NULL) {
    PTRACE(4, *this << "write to closed/unopened subchannel " << subchannel);
    return false;
  }

  PIPSocketAddressAndPort sendAddr;
  if (dest != NULL)
    sendAddr = *dest;
  else
    socket->GetSendAddress(sendAddr);

  if (!sendAddr.IsValid()) {
    PTRACE(4, "UDP write has no destination address on subchannel " << subchannel);
    /* The following makes not having destination address yet be processed the
       same as if the remote is not yet listening on the port (ICMP errors) so it
       will keep trying for a while, then give up and close the media transport. */
    socket->SetErrorValues(PChannel::Unavailable, EINVAL, PChannel::LastWriteError);
    return false;
  }

  if (socket->WriteTo(data, length, sendAddr))
    return true;

  switch (socket->GetErrorCode(PChannel::LastWriteError)) {
    case PChannel::Unavailable:
      if (m_subchannels[subchannel].HandleUnavailableError())
        return true;
      break;

    case PChannel::BufferTooSmall:
      if (m_mtuDiscoverMode >= 0 && mtu != NULL) {
        *mtu = socket->GetCurrentMTU();
        return false;
      }
      break;

    default:
      break;
  }

  PTRACE(1, *this << "error writing to " << sendAddr
                  << " (" << length << " bytes)"
                     " on " << subchannel << " subchannel"
                     " (" << socket->GetErrorNumber(PChannel::LastWriteError) << "):"
                     " " << socket->GetErrorText(PChannel::LastWriteError));
  return false;
}


PUDPSocket * OpalUDPMediaTransport::GetSubChannelAsSocket(SubChannels subchannel) const
{
  return (size_t)subchannel < m_socketCache.size() ? m_socketCache[subchannel] : NULL;
}


/////////////////////////////////////////////////////////////////////////////

OpalMediaSession::OpalMediaSession(const Init & init)
  : PSafeObject(m_instrumentedMutex)
  , m_connection(init.m_connection)
  , m_sessionId(init.m_sessionId)
  , m_mediaType(init.m_mediaType)
  , m_remoteBehindNAT(init.m_remoteBehindNAT)
{
  PTRACE_CONTEXT_ID_FROM(init.m_connection);
  PTRACE(5, *this << "created " << this << " for " << m_mediaType);
}


OpalMediaSession::~OpalMediaSession()
{
  PTRACE(5, *this << "destroyed " << this);
}


void OpalMediaSession::PrintOn(ostream & strm) const
{
  strm << "Session " << m_sessionId << ", ";
}


bool OpalMediaSession::IsOpen() const
{
  OpalMediaTransportPtr transport = m_transport; // This way avoids races
  return transport != NULL && transport->IsOpen();
}


void OpalMediaSession::Start()
{
  OpalMediaTransportPtr transport = m_transport; // This way avoids races
  if (transport != NULL)
    transport->Start();
}


bool OpalMediaSession::IsEstablished() const
{
  OpalMediaTransportPtr transport = m_transport; // This way avoids races
  return transport != NULL && transport->IsEstablished();
}


bool OpalMediaSession::Close()
{
  // A close here is detach but don't do anything with detached transport
  return OpalMediaSession::DetachTransport() != NULL;
}


OpalTransportAddress OpalMediaSession::GetLocalAddress(bool isMediaAddress) const
{
  OpalMediaTransportPtr transport = m_transport; // This way avoids races
  return transport != NULL ? transport->GetLocalAddress(isMediaAddress ? e_Media : e_Control) : OpalTransportAddress();
}


OpalTransportAddress OpalMediaSession::GetRemoteAddress(bool isMediaAddress) const
{
  OpalMediaTransportPtr transport = m_transport; // This way avoids races
  return transport != NULL ? transport->GetRemoteAddress(isMediaAddress ? e_Media : e_Control) : OpalTransportAddress();
}


bool OpalMediaSession::SetRemoteAddress(const OpalTransportAddress & remoteAddress, bool isMediaAddress)
{
  OpalMediaTransportPtr transport = m_transport; // This way avoids races
  return transport == NULL || transport->SetRemoteAddress(remoteAddress, isMediaAddress ? e_Media : e_Control);
}


void OpalMediaSession::AttachTransport(const OpalMediaTransportPtr & transport)
{
  m_transport = transport;
}


OpalMediaTransportPtr OpalMediaSession::DetachTransport()
{
  OpalMediaTransportPtr transport = m_transport;
  m_transport.SetNULL();

  if (transport != NULL) {
    PTRACE(3, *transport << "detaching from session " << GetSessionID());
    transport->RemoveReadNotifier(this, e_AllSubChannels);
  }

  return transport;
}


bool OpalMediaSession::UpdateMediaFormat(const OpalMediaFormat &)
{
  return true;
}


const PString & OpalMediaSession::GetBundleGroupId() { static PConstString const s("BUNDLE"); return s;  }


bool OpalMediaSession::AddGroup(const PString & groupId, const PString & mediaId, bool overwrite)
{
  P_INSTRUMENTED_LOCK_READ_WRITE();

  if (!overwrite && m_groups.Contains(groupId)) {
    if (m_groups[groupId] == mediaId)
      return true;
    PTRACE(3, *this << "could not set group \"" << groupId << "\" media id to"
           " \"" << mediaId << "\", already set to \"" << m_groups[groupId] << '"');
    return false;
  }

  m_groups.SetAt(groupId, mediaId);
  PTRACE(4, "Set group \"" << groupId << "\" to media id \"" << mediaId << '"');
  return true;
}


bool OpalMediaSession::IsGroupMember(const PString & groupId) const
{
  P_INSTRUMENTED_LOCK_READ_ONLY();
  return m_groups.Contains(groupId);
}


PStringArray OpalMediaSession::GetGroups() const
{
  P_INSTRUMENTED_LOCK_READ_ONLY();
  return m_groups.GetKeys();
}


PString OpalMediaSession::GetGroupMediaId(const PString & groupId) const
{
  P_INSTRUMENTED_LOCK_READ_ONLY();
  PString str = m_groups(groupId);
  str.MakeUnique();
  return str;
}


#if OPAL_SDP
SDPMediaDescription * OpalMediaSession::CreateSDPMediaDescription()
{
  return m_mediaType->CreateSDPMediaDescription(GetLocalAddress());
}
#endif // OPAL_SDP

#if OPAL_STATISTICS
void OpalMediaSession::GetStatistics(OpalMediaStatistics & statistics, bool) const
{
  statistics.m_mediaType = GetMediaType();

  OpalMediaTransportPtr transport = m_transport; // This way avoids races
  if (transport != NULL)
    transport->GetStatistics(statistics);
}
#endif


void OpalMediaSession::SetRemoteBehindNAT()
{
  m_remoteBehindNAT = true;
  OpalMediaTransportPtr transport = m_transport; // This way avoids races
  if (transport != NULL)
    transport->SetRemoteBehindNAT();
}

void OpalMediaSession::OfferCryptoSuite(const PString & cryptoSuiteName)
{
  OpalMediaCryptoSuite * cryptoSuite = OpalMediaCryptoSuiteFactory::CreateInstance(cryptoSuiteName);
  if (cryptoSuite == NULL) {
    PTRACE(1, *this << "cannot create crypto suite for " << cryptoSuiteName);
    return;
  }

  OpalMediaCryptoKeyInfo * keyInfo = cryptoSuite->CreateKeyInfo();
  keyInfo->Randomise();
  m_offeredCryptokeys.Append(keyInfo);
}


OpalMediaCryptoKeyList & OpalMediaSession::GetOfferedCryptoKeys()
{
  return m_offeredCryptokeys;
}


bool OpalMediaSession::ApplyCryptoKey(OpalMediaCryptoKeyList &, bool)
{
  return false;
}


OpalMediaCryptoKeyInfo * OpalMediaSession::IsCryptoSecured(bool) const
{
  return NULL;
}


//////////////////////////////////////////////////////////////////////////////

PFACTORY_CREATE(OpalMediaSessionFactory, OpalDummySession, OpalDummySession::SessionType());

OpalDummySession::OpalDummySession(const Init & init)
  : OpalMediaSession(init)
{
}


OpalDummySession::OpalDummySession(const Init & init, const OpalTransportAddressArray & transports)
  : OpalMediaSession(init)
{
  switch (transports.GetSize()) {
    case 2 :
      m_localTransportAddress[e_Control] = transports[e_Control];
    case 1 :
      m_localTransportAddress[e_Media] = transports[e_Media];
  }
}


const PCaselessString & OpalDummySession::SessionType()
{
  static PConstCaselessString const s("Dummy-Media-Session");
  return s;
}


const PCaselessString & OpalDummySession::GetSessionType() const
{
  return m_mediaType->GetMediaSessionType();
}


bool OpalDummySession::Open(const PString &, const OpalTransportAddress &)
{
  P_INSTRUMENTED_LOCK_READ_WRITE();
  if (!lock.IsLocked())
    return false;

  PSafePtr<OpalConnection> otherParty = m_connection.GetOtherPartyConnection();
  if (otherParty != NULL) {
    OpalTransportAddressArray transports;
    if (otherParty->GetMediaTransportAddresses(m_connection, m_sessionId, m_mediaType, transports)) {
      switch (transports.GetSize()) {
        default:
          m_localTransportAddress[e_Control] = transports[e_Control];
          PTRACE(4, *this << "dummy opened at local control address " << m_localTransportAddress[e_Control]);
        case 1:
          m_localTransportAddress[e_Media] = transports[e_Media];
          PTRACE(4, *this << "dummy opened at local media address " << m_localTransportAddress[e_Media]);
          break;
        case 0 :
          PTRACE(3, *this << "dummy could not be opened, no media transport.");
      }
    }
    else {
      PTRACE(3, *this << "dummy could not be opened, disabled media.");
    }
  }
  else {
    PTRACE(2, *this << "dummy could not be opened, no other connection.");
  }

  return true;
}


bool OpalDummySession::IsOpen() const
{
  P_INSTRUMENTED_LOCK_READ_ONLY();
  return lock.IsLocked() && !m_localTransportAddress[e_Media].IsEmpty() && !m_remoteTransportAddress[e_Media].IsEmpty();
}


OpalTransportAddress OpalDummySession::GetLocalAddress(bool isMediaAddress) const
{
  P_INSTRUMENTED_LOCK_READ_ONLY();
  return lock.IsLocked() ? m_localTransportAddress[isMediaAddress ? e_Media : e_Control] : OpalTransportAddress();
}


OpalTransportAddress OpalDummySession::GetRemoteAddress(bool isMediaAddress) const
{
  P_INSTRUMENTED_LOCK_READ_ONLY();
  return lock.IsLocked() ? m_remoteTransportAddress[isMediaAddress ? e_Media : e_Control] : OpalTransportAddress();
}


bool OpalDummySession::SetRemoteAddress(const OpalTransportAddress & remoteAddress, bool isMediaAddress)
{
  P_INSTRUMENTED_LOCK_READ_WRITE();
  if (!lock.IsLocked())
    return false;

  // Some code to keep the port if new one does not have it but old did.
  PIPSocket::Address ip;
  WORD port = 0;
  PINDEX subchannel = isMediaAddress ? e_Media : e_Control;
  if (m_remoteTransportAddress[subchannel].GetIpAndPort(ip, port) && remoteAddress.GetIpAndPort(ip, port))
    m_remoteTransportAddress[subchannel] = OpalTransportAddress(ip, port, remoteAddress.GetProto());
  else
    m_remoteTransportAddress[subchannel] = remoteAddress;

  PTRACE(4, *this << "dummy remote "
         << (isMediaAddress ? "media" : "control") << " address set to "
         << m_remoteTransportAddress[subchannel]);

  return true;
}


void OpalDummySession::AttachTransport(const OpalMediaTransportPtr &)
{

}


OpalMediaTransportPtr OpalDummySession::DetachTransport()
{
  return NULL;
}


#if OPAL_SDP
OpalDummySession::OpalDummySession(const Init & init, const PStringArray & sdpTokens)
  : OpalMediaSession(init)
  , m_sdpTokens(sdpTokens)
{
}


SDPMediaDescription * OpalDummySession::CreateSDPMediaDescription()
{
  if (m_sdpTokens.empty())
    return OpalMediaSession::CreateSDPMediaDescription();

  return new SDPDummyMediaDescription(GetLocalAddress(), m_sdpTokens);
}
#endif // OPAL_SDP


OpalMediaStream * OpalDummySession::CreateMediaStream(const OpalMediaFormat & mediaFormat, unsigned sessionID, bool isSource)
{
  return new OpalNullMediaStream(m_connection, mediaFormat, sessionID, isSource, false, false);
}


/////////////////////////////////////////////////////////////////////////////

