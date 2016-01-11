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
 * $Revision$
 * $Author$
 * $Date$
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

OpalNetworkStatistics::OpalNetworkStatistics()
  : m_SSRC(0)
  , m_startTime(0)
  , m_totalBytes(0)
  , m_totalPackets(0)
  , m_controlPacketsIn(0)
  , m_controlPacketsOut(0)
  , m_NACKs(-1)
  , m_packetsLost(-1)
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
  , m_lastPacketTime(0)
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
  :  m_threadIdentifier(PNullThreadIdentifier)
#if OPAL_FAX
  , m_fax(*this) // Backward compatibility
#endif
{
}


OpalMediaStatistics::OpalMediaStatistics(const OpalMediaStatistics & other)
  : PObject(other)
  , OpalNetworkStatistics(other)
  , OpalVideoStatistics(other)
  , OpalFaxStatistics(other)
  , m_mediaType(other.m_mediaType)
  , m_mediaFormat(other.m_mediaFormat)
  , m_threadIdentifier(other.m_threadIdentifier)
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


static PString InternalTimeDiff(PTime lastUpdate, const PTime & previousUpdate)
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
       << setw(indent) <<       "Late out of order" << " = " << m_lateOutOfOrder << '\n';

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
         << setw(indent) <<      "Total key frames" << " = " << m_keyFrames << '\n';
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
  : m_name(name)
  , m_remoteBehindNAT(false)
  , m_remoteAddressSet(false)
  , m_packetSize(2048)
  , m_maxNoTransmitTime(0, 10)          // Sending data for 10 seconds, ICMP says still not there
  , m_started(false)
{
}


void OpalMediaTransport::PrintOn(ostream & strm) const
{
  strm << m_name << ", ";
}


bool OpalMediaTransport::IsOpen() const
{
  PSafeLockReadOnly lock(*this);
  if (!lock.IsLocked() || m_subchannels.empty())
    return false;

  for (vector<Transport>::const_iterator it = m_subchannels.begin(); it != m_subchannels.end(); ++it) {
    if (it->m_channel == NULL || !it->m_channel->IsOpen())
      return false;
  }
  return true;
}


bool OpalMediaTransport::IsEstablished() const
{
  return m_remoteAddressSet && IsOpen();
}


OpalTransportAddress OpalMediaTransport::GetLocalAddress(SubChannels) const
{
  return OpalTransportAddress();
}


OpalTransportAddress OpalMediaTransport::GetRemoteAddress(SubChannels) const
{
  return OpalTransportAddress();
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


bool OpalMediaTransport::Write(const void * data, PINDEX length, SubChannels subchannel, const PIPSocketAddressAndPort *)
{
  PSafeLockReadOnly lock(*this);
  if (!lock.IsLocked())
    return false;

  PChannel * channel;
  if (subchannel >= (PINDEX)m_subchannels.size() || (channel = m_subchannels[subchannel].m_channel) == NULL) {
    PTRACE(4, *this << "write to closed/unopened subchannel " << subchannel);
    return false;
  }

  if (channel->Write(data, length))
    return true;

  if (channel->GetErrorCode(PChannel::LastWriteError) == PChannel::Unavailable && m_subchannels[subchannel].HandleUnavailableError())
    return true;

  PTRACE(1, *this << "write (" << length << " bytes) error"
            " on " << subchannel << " subchannel"
            " (" << channel->GetErrorNumber(PChannel::LastWriteError) << "):"
            " " << channel->GetErrorText(PChannel::LastWriteError));
  return false;
}


#if OPAL_SRTP
bool OpalMediaTransport::GetKeyInfo(OpalMediaCryptoKeyInfo * [2])
{
  return false;
}
#endif


void OpalMediaTransport::AddReadNotifier(const ReadNotifier & notifier, SubChannels subchannel)
{
  if (LockReadWrite()) {
    if ((size_t)subchannel < m_subchannels.size())
      m_subchannels[subchannel].m_notifiers.Add(notifier);
    UnlockReadWrite();
  }
}


void OpalMediaTransport::RemoveReadNotifier(const ReadNotifier & notifier, SubChannels subchannel)
{
  if (LockReadWrite()) {
    if ((size_t)subchannel < m_subchannels.size())
      m_subchannels[subchannel].m_notifiers.Remove(notifier);
    UnlockReadWrite();
  }
}


void OpalMediaTransport::RemoveReadNotifier(PObject * target, SubChannels subchannel)
{
  if (LockReadWrite()) {
    if (subchannel != e_AllSubChannels) {
      if ((size_t)subchannel < m_subchannels.size())
        m_subchannels[subchannel].m_notifiers.RemoveTarget(target);
    }
    else {
      for (vector<Transport>::iterator it = m_subchannels.begin(); it != m_subchannels.end(); ++it)
        it->m_notifiers.RemoveTarget(target);
    }
    UnlockReadWrite();
  }
}


void OpalMediaTransport::SetRemoteBehindNAT()
{
  m_remoteBehindNAT = true;
}

OpalMediaTransport::Transport::Transport(OpalMediaTransport * owner, SubChannels subchannel, PChannel * chan)
  : m_owner(owner)
  , m_subchannel(subchannel)
  , m_channel(chan)
  , m_thread(NULL)
  , m_consecutiveUnavailableErrors(0)
{
}


void OpalMediaTransport::Transport::ThreadMain()
{
  PTRACE(4, m_owner, *m_owner << m_subchannel << " media transport read thread starting");

  m_owner->InternalOnStart(m_subchannel);

  while (m_channel->IsOpen()) {
    PBYTEArray data(m_owner->m_packetSize);

    PTRACE(m_throttleReadPacket, m_owner, *m_owner << m_subchannel <<
           " read packet: sz=" << data.GetSize() << " timeout=" << m_channel->GetReadTimeout());

    if (m_channel->Read(data.GetPointer(), data.GetSize())) {
      data.SetSize(m_channel->GetLastReadCount());
      m_owner->InternalRxData(m_subchannel, data);
    }
    else {
      switch (m_channel->GetErrorCode(PChannel::LastReadError)) {
        case PChannel::Unavailable:
          HandleUnavailableError();
          break;

        case PChannel::BufferTooSmall:
          PTRACE(2, m_owner, *m_owner << m_subchannel << " read packet too large for buffer of " << data.GetSize() << " bytes.");
          break;

        case PChannel::Interrupted:
          PTRACE(4, m_owner, *m_owner << m_subchannel << " read packet interrupted.");
          // Shouldn't happen, but it does.
          break;

        case PChannel::NoError:
          PTRACE(3, m_owner, *m_owner << m_subchannel << " received UDP packet with no payload.");
          break;

        case PChannel::Timeout:
          PTRACE(1, m_owner, *m_owner << m_subchannel << " timed out (" << m_channel->GetReadTimeout() << "s)");
          Close();
          break;

        default:
          PTRACE(1, m_owner, *m_owner << m_subchannel
                 << " read error (" << m_channel->GetErrorNumber(PChannel::LastReadError) << "): "
                 << m_channel->GetErrorText(PChannel::LastReadError));
          Close();
          break;
      }
    }
  }

  m_owner->InternalRxData(m_subchannel, PBYTEArray());
  PTRACE(4, m_owner, *m_owner << m_subchannel << " media transport read thread ended");
}


bool OpalMediaTransport::Transport::HandleUnavailableError()
{
  if (m_timeForUnavailableErrors.HasExpired() && m_consecutiveUnavailableErrors < 10)
    m_consecutiveUnavailableErrors = 0;

  if (++m_consecutiveUnavailableErrors == 1) {
    PTRACE(2, m_owner, *m_owner << m_subchannel << " port on remote not ready: " << m_owner->GetRemoteAddress(m_subchannel));
    m_timeForUnavailableErrors = m_channel->GetReadTimeout();
    return true;
  }

  if (m_timeForUnavailableErrors.IsRunning() || m_consecutiveUnavailableErrors < 10)
    return true;

  PTRACE(2, m_owner, *m_owner << m_subchannel << ' ' << m_owner->m_maxNoTransmitTime
         << " seconds of transmit fails to " << m_owner->GetRemoteAddress(m_subchannel));
  Close();
  return false;
}


void OpalMediaTransport::Transport::Close()
{
  if (m_channel == NULL)
    return;

  PChannel * base = m_channel->GetBaseReadChannel();
  if (base == NULL)
    return;

  base->Close();
  m_owner->InternalRxData(m_subchannel, PBYTEArray());
}


void OpalMediaTransport::InternalOnStart(SubChannels)
{
}


void OpalMediaTransport::InternalRxData(SubChannels subchannel, const PBYTEArray & data)
{
  if (!LockReadOnly())
    return;

  Transport::NotifierList notifiers = m_subchannels[subchannel].m_notifiers;
  UnlockReadOnly();

  notifiers(*this, data);
}


void OpalMediaTransport::Start()
{
  PSafeLockReadWrite lock(*this);
  if (!lock.IsLocked())
    return;

  if (m_started)
    return;
  m_started = true;

  PTRACE(4, *this << "starting read theads, " << m_subchannels.size() << " sub-channels");
  for (size_t subchannel = 0; subchannel < m_subchannels.size(); ++subchannel) {
    if (m_subchannels[subchannel].m_channel != NULL && m_subchannels[subchannel].m_thread == NULL) {
      PStringStream threadName;
      threadName << m_name;
      if (m_subchannels.size() > 1)
        threadName << '-' << (SubChannels)subchannel;
      threadName.Replace(" Session ", "-");
      threadName.Replace(" bundle", "-B");
      m_subchannels[subchannel].m_thread = new PThreadObj<OpalMediaTransport::Transport>
              (m_subchannels[subchannel], &OpalMediaTransport::Transport::ThreadMain, false, threadName, PThread::HighPriority);
      PTRACE_CONTEXT_ID_TO(m_subchannels[subchannel].m_thread);
    }
  }
}


void OpalMediaTransport::InternalStop()
{
  PTRACE(4, *this << "stopping");
  LockReadOnly();
  for (vector<Transport>::iterator it = m_subchannels.begin(); it != m_subchannels.end(); ++it)
    it->Close();
  UnlockReadOnly();

  for (vector<Transport>::iterator it = m_subchannels.begin(); it != m_subchannels.end(); ++it) {
    if (it->m_thread != NULL) {
      PAssert(it->m_thread->WaitForTermination(2000), "RTP thread failed to terminate");
      delete it->m_thread;
    }
  }

  LockReadWrite();
  for (vector<Transport>::iterator it = m_subchannels.begin(); it != m_subchannels.end(); ++it)
    delete it->m_channel;
  m_subchannels.clear();
  UnlockReadWrite();

  PTRACE(4, *this << "stopped");
}


//////////////////////////////////////////////////////////////////////////////

OpalTCPMediaTransport::OpalTCPMediaTransport(const PString & name)
  : OpalMediaTransport(name)
{
  m_subchannels.resize(1);
  m_subchannels.front().m_channel = new PTCPSocket;
}


bool OpalTCPMediaTransport::Open(OpalMediaSession &, PINDEX, const PString & localInterface, const OpalTransportAddress &)
{
  return dynamic_cast<PTCPSocket &>(*m_subchannels[0].m_channel).Listen(PIPAddress(localInterface));
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

  m_remoteAddressSet = true;
  return true;
}


//////////////////////////////////////////////////////////////////////////////

OpalUDPMediaTransport::OpalUDPMediaTransport(const PString & name)
  : OpalMediaTransport(name)
  , m_localHasRestrictedNAT(false)
{
}


OpalTransportAddress OpalUDPMediaTransport::GetLocalAddress(SubChannels subchannel) const
{
  PSafeLockReadOnly lock(*this);
  if (lock.IsLocked()) {
    PUDPSocket * socket = GetSubChannelAsSocket(subchannel);
    if (socket != NULL) {
      PIPSocketAddressAndPort ap;
      if (socket->GetLocalAddress(ap) && ap.IsValid())
        return OpalTransportAddress(ap, OpalTransportAddress::UdpPrefix());
    }
  }
  return OpalTransportAddress();
}


OpalTransportAddress OpalUDPMediaTransport::GetRemoteAddress(SubChannels subchannel) const
{
  PSafeLockReadOnly lock(*this);
  if (lock.IsLocked()) {
    PUDPSocket * socket = GetSubChannelAsSocket(subchannel);
    if (socket != NULL) {
      PIPSocketAddressAndPort ap;
      socket->GetSendAddress(ap);
      if (ap.IsValid())
        return OpalTransportAddress(ap, OpalTransportAddress::UdpPrefix());
    }
  }
  return OpalTransportAddress();
}


bool OpalUDPMediaTransport::SetRemoteAddress(const OpalTransportAddress & remoteAddress, SubChannels subchannel)
{
  // This bool is safe, and lock is in InternalSetRemoteAddress
  if (m_remoteBehindNAT) {
    PTRACE(3, *this << "ignoring remote address as is behind NAT");
    return true;
  }

  PIPAddressAndPort ap;
  if (!remoteAddress.GetIpAndPort(ap))
    return false;

  return InternalSetRemoteAddress(ap, subchannel, false PTRACE_PARAM(, "signalling"));
}


void OpalUDPMediaTransport::InternalRxData(SubChannels subchannel, const PBYTEArray & data)
{
  if (m_remoteBehindNAT) {
    // If remote address never set from higher levels, then try and figure
    // it out from the first packet received.
    PIPAddressAndPort ap;
    GetSubChannelAsSocket(subchannel)->GetLastReceiveAddress(ap);
    InternalSetRemoteAddress(ap, subchannel, true PTRACE_PARAM(, "first PDU"));
  }

  OpalMediaTransport::InternalRxData(subchannel, data);
}


bool OpalUDPMediaTransport::InternalSetRemoteAddress(const PIPSocket::AddressAndPort & newAP,
                                                     SubChannels subchannel,
                                                     bool dontOverride
                                                     PTRACE_PARAM(, const char * source))
{
  PSafeLockReadWrite lock(*this);
  if (!lock.IsLocked())
    return false;

  PUDPSocket * socket = GetSubChannelAsSocket(subchannel);
  if (socket == NULL)
    return false;

  PIPSocketAddressAndPort oldAP;
  socket->GetLocalAddress(oldAP);
  if (newAP == oldAP) {
    PTRACE(2, *this << source << " cannot set remote address/port to same as local: " << oldAP);
    return false;
  }

  socket->GetSendAddress(oldAP);
  if (newAP == oldAP)
    return true;

  if (dontOverride && oldAP.IsValid()) {
    PTRACE(3, *this << source << " cannot set remote address/port to " << newAP << ", already set to " << oldAP);
    return false;
  }

  socket->SetSendAddress(newAP);
  m_remoteAddressSet = true;

  if (m_localHasRestrictedNAT) {
    // If have Port Restricted NAT on local host then send a datagram
    // to remote to open up the port in the firewall for return data.
    static const BYTE dummy[1] = { 0 };
    socket->Write(dummy, sizeof(dummy));
  }

#if PTRACING
  static const int Level = 3;
  if (PTrace::CanTrace(Level)) {
    ostream & trace = PTRACE_BEGIN(Level);
    trace << *this << source << " set remote "
          << subchannel << " address to " << newAP;
    for (size_t sub = 0; sub < m_subchannels.size(); ++sub)
      trace << ", " << (SubChannels)sub << " rem=" << GetRemoteAddress((SubChannels)sub)
                         << " local=" << GetLocalAddress((SubChannels)sub);
    if (m_localHasRestrictedNAT)
      trace << ", restricted NAT";
    trace << PTrace::End;
  }
#endif

  return true;
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

  OpalManager & manager = session.GetConnection().GetEndPoint().GetManager();

  m_packetSize = manager.GetMaxRtpPacketSize();
  if (session.IsRemoteBehindNAT())
    SetRemoteBehindNAT();
  m_mediaTimeout = session.GetStringOptions().GetVar(OPAL_OPT_MEDIA_RX_TIMEOUT, manager.GetNoMediaTimeout());
  m_maxNoTransmitTime = session.GetStringOptions().GetVar(OPAL_OPT_MEDIA_TX_TIMEOUT, manager.GetTxMediaTimeout());

  PIPAddress bindingIP(localInterface);
  PIPAddress remoteIP;
  remoteAddress.GetIpAddress(remoteIP);

  PTRACE(4, session << "opening: interface=\"" << localInterface << "\" local=" << bindingIP << " remote=" << remoteIP);

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
              m_subchannels.push_back(Transport(this, e_Data, dataSocket));
              m_subchannels.push_back(Transport(this, e_Control, controlSocket));
              PTRACE(4, session << natMethod->GetMethodName() << " created NAT RTP/RTCP socket pair.");
              break;
            }

            PTRACE(2, session << natMethod->GetMethodName()
                    << " could not create NAT RTP/RTCP socket pair; trying to create individual sockets.");
          }

          for (PINDEX i = 0; i < subchannelCount; ++i) {
            PUDPSocket * socket = NULL;
            if (natMethod->CreateSocket(socket, bindingIP))
              m_subchannels.push_back(Transport(this, (SubChannels)i, socket));
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
      m_subchannels.push_back(Transport(this, (SubChannels)m_subchannels.size(), sockets[i]));
  }

  for (size_t subchannel = 0; subchannel < m_subchannels.size(); ++subchannel) {
    PUDPSocket & socket = *GetSubChannelAsSocket((SubChannels)subchannel);
    PTRACE_CONTEXT_ID_TO(socket);
    socket.SetReadTimeout(m_mediaTimeout);

    // Increase internal buffer size on media UDP sockets
    SetMinBufferSize(socket, SO_RCVBUF, session.GetMediaType() == OpalMediaType::Audio() ? 0x4000 : 0x100000);
    SetMinBufferSize(socket, SO_SNDBUF, 0x2000);
  }

  return true;
}


bool OpalUDPMediaTransport::Write(const void * data, PINDEX length, SubChannels subchannel, const PIPSocketAddressAndPort * dest)
{
  PSafeLockReadOnly lock(*this);
  if (!lock.IsLocked())
    return false;

  PUDPSocket * socket;
  if (dest == NULL || (socket = GetSubChannelAsSocket(subchannel)) == NULL)
    return OpalMediaTransport::Write(data, length, subchannel, dest);

  bool writeSuccess = socket->WriteTo(data, length, *dest);
  // This insanity prevents a totally unbelievable CPU issue when under heavy load.
  if (writeSuccess)
    return true;

  PTRACE(1, *this << "write to " << *dest << " (" << length << " bytes) error"
            " on " << subchannel << " subchannel to " << *dest <<
            " (" << socket->GetErrorNumber(PChannel::LastWriteError) << "):"
            " " << socket->GetErrorText(PChannel::LastWriteError));
  return false;
}


PUDPSocket * OpalUDPMediaTransport::GetSubChannelAsSocket(SubChannels subchannel) const
{
  PChannel * chanPtr = GetChannel(subchannel);
  if (chanPtr == NULL)
    return NULL;
  return dynamic_cast<PUDPSocket *>(chanPtr->GetBaseReadChannel());
}


/////////////////////////////////////////////////////////////////////////////

OpalMediaSession::OpalMediaSession(const Init & init)
  : m_connection(init.m_connection)
  , m_sessionId(init.m_sessionId)
  , m_mediaType(init.m_mediaType)
  , m_remoteBehindNAT(init.m_remoteBehindNAT)
{
  PTRACE_CONTEXT_ID_FROM(init.m_connection);
  PTRACE(5, *this << "created for " << m_mediaType);
}


OpalMediaSession::~OpalMediaSession()
{
  PTRACE(5, *this << "destroyed.");
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
  PSafeLockReadWrite lock(*this);

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
  PSafeLockReadOnly lock(*this);
  return m_groups.Contains(groupId);
}


PStringArray OpalMediaSession::GetGroups() const
{
  PSafeLockReadOnly lock(*this);
  return m_groups.GetKeys();
}


PString OpalMediaSession::GetGroupMediaId(const PString & groupId) const
{
  PSafeLockReadOnly lock(*this);
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
  statistics.m_mediaType     = GetMediaType();
  statistics.m_localAddress  = GetLocalAddress();
  statistics.m_remoteAddress = GetRemoteAddress();
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


bool OpalMediaSession::IsCryptoSecured(bool) const
{
  return false;
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
  if (!LockReadWrite())
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

  UnlockReadWrite();

  return true;
}


bool OpalDummySession::IsOpen() const
{
  PSafeLockReadOnly lock(*this);
  return lock.IsLocked() && !m_localTransportAddress[e_Media].IsEmpty() && !m_remoteTransportAddress[e_Media].IsEmpty();
}


OpalTransportAddress OpalDummySession::GetLocalAddress(bool isMediaAddress) const
{
  PSafeLockReadOnly lock(*this);
  return lock.IsLocked() ? m_localTransportAddress[isMediaAddress ? e_Media : e_Control] : OpalTransportAddress();
}


OpalTransportAddress OpalDummySession::GetRemoteAddress(bool isMediaAddress) const
{
  PSafeLockReadOnly lock(*this);
  return lock.IsLocked() ? m_remoteTransportAddress[isMediaAddress ? e_Media : e_Control] : OpalTransportAddress();
}


bool OpalDummySession::SetRemoteAddress(const OpalTransportAddress & remoteAddress, bool isMediaAddress)
{
  if (!LockReadWrite())
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

  UnlockReadWrite();

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
SDPMediaDescription * OpalDummySession::CreateSDPMediaDescription()
{
  if (m_mediaType.empty())
    return new SDPDummyMediaDescription(GetLocalAddress(), PStringArray());

  return OpalMediaSession::CreateSDPMediaDescription();
}
#endif // OPAL_SDP


OpalMediaStream * OpalDummySession::CreateMediaStream(const OpalMediaFormat & mediaFormat, unsigned sessionID, bool isSource)
{
  return new OpalNullMediaStream(m_connection, mediaFormat, sessionID, isSource, false, false);
}


/////////////////////////////////////////////////////////////////////////////

