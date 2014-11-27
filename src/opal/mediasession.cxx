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
#include <h323/h323caps.h>
#include <sdp/sdp.h>

#include <ptclib/random.h>
#include <ptclib/cypher.h>


#define PTraceModule() "Media"
#define new PNEW


///////////////////////////////////////////////////////////////////////////////

#if OPAL_STATISTICS

OpalNetworkStatistics::OpalNetworkStatistics()
  : m_startTime(0)
  , m_totalBytes(0)
  , m_totalPackets(0)
  , m_NACKs(0)
  , m_packetsLost(0)
  , m_packetsOutOfOrder(0)
  , m_packetsTooLate(0)
  , m_packetOverruns(0)
  , m_minimumPacketTime(0)
  , m_averagePacketTime(0)
  , m_maximumPacketTime(0)
  , m_averageJitter(0)
  , m_maximumJitter(0)
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
  , m_targetBitRate(0)
  , m_targetFrameRate(0)
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


void OpalVideoStatistics::Merge(const OpalVideoStatistics & other)
{
  if (m_totalFrames == 0 && other.m_totalFrames > 0)
    m_totalFrames = other.m_totalFrames;

  if (m_keyFrames == 0 && other.m_keyFrames > 0) {
    m_keyFrames = other.m_keyFrames;
    m_lastKeyFrameTime = other.m_lastKeyFrameTime;
  }

  if (m_fullUpdateRequests == 0 && other.m_fullUpdateRequests > 0) {
    m_fullUpdateRequests = other.m_fullUpdateRequests;
    m_lastUpdateRequestTime = other.m_lastUpdateRequestTime;
    m_updateResponseTime = other.m_updateResponseTime;
  }

  if (m_pictureLossRequests == 0 && other.m_pictureLossRequests > 0) {
    m_pictureLossRequests = other.m_pictureLossRequests;
    m_lastUpdateRequestTime = other.m_lastUpdateRequestTime;
    m_updateResponseTime = other.m_updateResponseTime;
  }
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
  : m_threadIdentifier(PNullThreadIdentifier)
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


static PString InternalGetRate(const PTime & lastUpdate,
                               const PTime & previousUpdate,
                               int64_t lastValue,
                               int64_t previousValue,
                               const char * units,
                               unsigned decimals)
{
  PString str = "N/A";

  if (lastUpdate.IsValid() && previousUpdate.IsValid()) {
    PTimeInterval interval = lastUpdate - previousUpdate;
    if (interval == 0)
      str = '0';
    else {
      double val = (lastValue - previousValue)*1000.0 / interval.GetMilliSeconds();
      if (val < 0.1)
        str = '0';
      else
        str = PString(PString::ScaleSI, val, decimals);
    }
    str += units;
  }

  return str;
}


PString OpalMediaStatistics::GetRate(int64_t total, const char * units, unsigned decimals) const
{
  return InternalGetRate(m_updateInfo.m_lastUpdateTime, m_startTime, total, 0, units, decimals);
}


PString OpalMediaStatistics::GetRate(int64_t current, int64_t previous, const char * units, unsigned decimals) const
{
  return InternalGetRate(m_updateInfo.m_lastUpdateTime,
                         m_updateInfo.m_previousUpdateTime.IsValid() ? m_updateInfo.m_previousUpdateTime : m_startTime,
                         current, previous, units, decimals);
}


#if OPAL_VIDEO
PString OpalMediaStatistics::GetAverageFrameRate(const char * units, unsigned decimals) const
{
  if (m_mediaType != OpalMediaType::Video())
    return "N/A";
  return GetRate(m_totalFrames, units, decimals);
}


PString OpalMediaStatistics::GetCurrentFrameRate(const char * units, unsigned decimals) const
{
  if (m_mediaType != OpalMediaType::Video())
    return "N/A";
  return GetRate(m_totalFrames, m_updateInfo.m_previousFrames, units, decimals);
}
#endif


PString OpalMediaStatistics::GetCPU() const
{
  if (m_updateInfo.m_usedCPU > 0 &&
      m_updateInfo.m_previousCPU > 0 &&
      m_updateInfo.m_lastUpdateTime.IsValid() &&
      m_updateInfo.m_previousUpdateTime.IsValid() &&
      m_updateInfo.m_lastUpdateTime > m_updateInfo.m_previousUpdateTime)
    return psprintf("%u%%", (unsigned)((m_updateInfo.m_usedCPU - m_updateInfo.m_previousCPU).GetMilliSeconds()*100 /
                                       (m_updateInfo.m_lastUpdateTime - m_updateInfo.m_previousUpdateTime).GetMilliSeconds()));

  return "N/A";
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
       << setw(indent) <<               "CPU usage" << " = " << GetCPU() << '\n'
       << setw(indent) <<             "Total bytes" << " = " << PString(PString::ScaleSI, m_totalBytes) << "B\n"
       << setw(indent) <<        "Average bit rate" << " = " << GetAverageBitRate("bps", 2) << '\n'
       << setw(indent) <<        "Current bit rate" << " = " << GetCurrentBitRate("bps", 3) << '\n'
       << setw(indent) <<           "Total packets" << " = " << m_totalPackets << '\n'
       << setw(indent) <<     "Average packet rate" << " = " << GetAveragePacketRate("pkt/s") << '\n'
       << setw(indent) <<     "Current packet rate" << " = " << GetCurrentPacketRate("pkt/s") << '\n'
       << setw(indent) <<     "Minimum packet time" << " = " << m_minimumPacketTime << "ms\n"
       << setw(indent) <<     "Average packet time" << " = " << m_averagePacketTime << "ms\n"
       << setw(indent) <<     "Maximum packet time" << " = " << m_maximumPacketTime << "ms\n"
       << setw(indent) <<            "Packets lost" << " = " << m_packetsLost << '\n'
       << setw(indent) <<    "Packets out of order" << " = " << m_packetsOutOfOrder << '\n'
       << setw(indent) <<        "Packets too late" << " = " << m_packetsTooLate << '\n';

  if (m_mediaType == OpalMediaType::Audio()) {
    strm << setw(indent) <<       "Packet overruns" << " = " << m_packetOverruns << '\n';
    if (m_averageJitter > 0 || m_maximumJitter > 0)
      strm << setw(indent) <<      "Average jitter" << " = " << m_averageJitter << "ms\n"
           << setw(indent) <<      "Maximum jitter" << " = " << m_maximumJitter << "ms\n";
    if (m_jitterBufferDelay > 0)
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


/////////////////////////////////////////////////////////////////////////////

OpalMediaSession::OpalMediaSession(const Init & init)
  : m_connection(init.m_connection)
  , m_sessionId(init.m_sessionId)
  , m_mediaType(init.m_mediaType)
{
  PTRACE_CONTEXT_ID_FROM(init.m_connection);
  PTRACE(5, *this << "created for " << m_mediaType);
}


void OpalMediaSession::PrintOn(ostream & strm) const
{
  strm << "Session " << m_sessionId << ", ";
}


bool OpalMediaSession::Open(const PString & PTRACE_PARAM(localInterface),
                            const OpalTransportAddress & remoteAddress,
                            bool isMediaAddress)
{
  PTRACE(5, *this << "opening on interface \"" << localInterface << "\" to "
         << (isMediaAddress ? "media" : "control") << " address " << remoteAddress);
  return SetRemoteAddress(remoteAddress, isMediaAddress);
}


bool OpalMediaSession::IsOpen() const
{
  return true;
}


bool OpalMediaSession::Close()
{
  return true;
}


OpalTransportAddress OpalMediaSession::GetLocalAddress(bool) const
{
  return OpalTransportAddress();
}


OpalTransportAddress OpalMediaSession::GetRemoteAddress(bool) const
{
  return OpalTransportAddress();
}


bool OpalMediaSession::SetRemoteAddress(const OpalTransportAddress &, bool)
{
  return true;
}


void OpalMediaSession::AttachTransport(Transport &)
{
}


OpalMediaSession::Transport OpalMediaSession::DetachTransport()
{
  return Transport();
}


bool OpalMediaSession::UpdateMediaFormat(const OpalMediaFormat &)
{
  return true;
}


#if OPAL_SDP
SDPMediaDescription * OpalMediaSession::CreateSDPMediaDescription()
{
  return m_mediaType->CreateSDPMediaDescription(GetLocalAddress());
}
#endif // OPAL_SDP

#if OPAL_STATISTICS
void OpalMediaSession::GetStatistics(OpalMediaStatistics &, bool) const
{
}
#endif


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


#if OPAL_ICE
static void InternalSetLocalUserPass(PString & user, PString & pass)
{
  if (user.IsEmpty())
    user = PBase64::Encode(PRandom::Octets(12));

  if (pass.IsEmpty())
    pass = PBase64::Encode(PRandom::Octets(18));
}

void OpalMediaSession::SetICE(const PString & user, const PString & pass, const PNatCandidateList &)
{
  InternalSetLocalUserPass(m_localUsername, m_localPassword);
  m_remoteUsername = user;
  m_remotePassword = pass;
}


bool OpalMediaSession::GetICE(PString & user, PString & pass, PNatCandidateList &)
{
  InternalSetLocalUserPass(m_localUsername, m_localPassword);
  user = m_localUsername;
  pass = m_localPassword;
  return true;
}
#endif


//////////////////////////////////////////////////////////////////////////////

PFACTORY_CREATE(OpalMediaSessionFactory, OpalDummySession, OpalDummySession::SessionType());

OpalDummySession::OpalDummySession(const Init & init, const OpalTransportAddressArray & transports)
  : OpalMediaSession(init)
{
  switch (transports.GetSize()) {
    case 2 :
      m_localTransportAddress[false] = transports[1]; // Control
    case 1 :
      m_localTransportAddress[true] = transports[0]; // Media
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


bool OpalDummySession::Open(const PString &, const OpalTransportAddress & remoteAddress, bool isMediaAddress)
{
  PSafePtr<OpalConnection> otherParty = m_connection.GetOtherPartyConnection();
  if (otherParty != NULL) {
    OpalTransportAddressArray transports;
    if (otherParty->GetMediaTransportAddresses(m_connection, m_sessionId, m_mediaType, transports)) {
      switch (transports.GetSize()) {
        case 2:
          m_localTransportAddress[false] = transports[1]; // Control
        case 1:
          m_localTransportAddress[true] = transports[0]; // Media
      }
    }
  }

  PTRACE(5, *this << "dummy opened at local media address " << GetLocalAddress());

  return SetRemoteAddress(remoteAddress, isMediaAddress);
}


bool OpalDummySession::IsOpen() const
{
  return !m_localTransportAddress[true].IsEmpty() && !m_remoteTransportAddress[true].IsEmpty();
}


OpalTransportAddress OpalDummySession::GetLocalAddress(bool isMediaAddress) const
{
  return m_localTransportAddress[isMediaAddress];
}


OpalTransportAddress OpalDummySession::GetRemoteAddress(bool isMediaAddress) const
{
  return m_remoteTransportAddress[isMediaAddress];
}


bool OpalDummySession::SetRemoteAddress(const OpalTransportAddress & remoteAddress, bool isMediaAddress)
{
  // Some code to keep the port if new one does not have it but old did.
  PIPSocket::Address ip;
  WORD port = 0;
  if (m_remoteTransportAddress[isMediaAddress].GetIpAndPort(ip, port) && remoteAddress.GetIpAndPort(ip, port))
    m_remoteTransportAddress[isMediaAddress] = OpalTransportAddress(ip, port, remoteAddress.GetProto());
  else
    m_remoteTransportAddress[isMediaAddress] = remoteAddress;

  PTRACE(5, *this << "dummy remote "
         << (isMediaAddress ? "media" : "control") << " address set to "
         << m_remoteTransportAddress[isMediaAddress]);

  return true;
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

