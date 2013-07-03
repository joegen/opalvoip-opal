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


#define new PNEW


///////////////////////////////////////////////////////////////////////////////

#if OPAL_STATISTICS

OpalMediaStatistics::OpalMediaStatistics()
  : m_lastUpdateTime(PTimer::Tick())
  , m_startTime(0)
  , m_totalBytes(0)
  , m_deltaBytes(0)
  , m_totalPackets(0)
  , m_deltaPackets(0)
  , m_packetsLost(0)
  , m_packetsOutOfOrder(0)
  , m_packetsTooLate(0)
  , m_packetOverruns(0)
  , m_minimumPacketTime(0)
  , m_averagePacketTime(0)
  , m_maximumPacketTime(0)

    // Audio
  , m_averageJitter(0)
  , m_maximumJitter(0)

#if OPAL_VIDEO
    // Video
  , m_totalFrames(0)
  , m_deltaFrames(0)
  , m_keyFrames(0)
  , m_quality(-1)
#endif
{
}

#if OPAL_FAX
OpalMediaStatistics::Fax::Fax()
  : m_result(OpalMediaStatistics::FaxNotStarted)
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

ostream & operator<<(ostream & strm, OpalMediaStatistics::FaxCompression compression)
{
  static const char * const Names[] = { "N/A", "T.4 1d", "T.4 2d", "T.6" };
  if (compression >= 0 && compression < PARRAYSIZE(Names))
    strm << Names[compression];
  else
    strm << (unsigned)compression;
  return strm;
}
#endif


OpalMediaStatistics & OpalMediaStatistics::Update(const OpalMediaStream & stream)
{
  PTimeInterval now = PTimer::Tick();
  m_updateInterval = now > m_lastUpdateTime ? now - m_lastUpdateTime : 0;
  m_lastUpdateTime = now;

  uint64_t previousBytes = m_totalBytes;
  unsigned previousPackets = m_totalPackets;
#if OPAL_VIDEO
  unsigned previousFrames = m_totalFrames;
#endif

  stream.GetStatistics(*this);

  m_deltaBytes = m_totalBytes > previousBytes ? m_totalBytes - previousBytes : 0;
  m_deltaPackets = m_totalPackets > previousPackets ? m_totalPackets - previousPackets : 0;
#if OPAL_VIDEO
  m_deltaFrames = m_totalFrames > previousFrames ? m_totalFrames - previousFrames : 0;
#endif

  return *this;
}


static ostream & AsRate(ostream & strm, int64_t things, const PTimeInterval & interval, unsigned decimals, const char * units)
{
  if (interval != 0)
    strm << PString(PString::ScaleSI, things*1000.0/interval.GetMilliSeconds(), decimals) << ' ' << units;
  else
    strm << "N/A";
  return strm;
}


void OpalMediaStatistics::PrintOn(ostream & strm) const
{
  std::streamsize indent = strm.precision()+20;

  PTimeInterval totalDuration = (PTime() - m_startTime);

  strm << setw(indent) <<            "Media format" << " = " << m_mediaFormat << " (" << m_mediaType << ")\n"
       << setw(indent) <<      "Session start time" << " = " << m_startTime << '\n'
       << setw(indent) <<        "Session duration" << " = " << totalDuration << " (" << m_updateInterval << ")\n"
       << setw(indent) <<             "Total bytes" << " = " << PString(PString::ScaleSI, m_totalBytes) << "B\n"
       << setw(indent) <<        "Average bit rate" << " = "; AsRate(strm, m_totalBytes*8, totalDuration, 2, "bps") << '\n'
       << setw(indent) <<        "Current bit rate" << " = "; AsRate(strm, m_deltaBytes*8, m_updateInterval, 2, "bps") << '\n'
       << setw(indent) <<           "Total packets" << " = " << m_totalPackets << '\n'
       << setw(indent) <<     "Average packet rate" << " = "; AsRate(strm, m_totalPackets, totalDuration, 0, "packets/second") << '\n'
       << setw(indent) <<     "Current packet rate" << " = "; AsRate(strm, m_deltaPackets, m_updateInterval, 0, "packets/second") << '\n'
       << setw(indent) <<     "Minimum packet time" << " = " << m_minimumPacketTime << "ms\n"
       << setw(indent) <<     "Average packet time" << " = " << m_averagePacketTime << "ms\n"
       << setw(indent) <<     "Maximum packet time" << " = " << m_maximumPacketTime << "ms\n"
       << setw(indent) <<            "Packets lost" << " = " << m_packetsLost << '\n'
       << setw(indent) <<    "Packets out of order" << " = " << m_packetsOutOfOrder << '\n'
       << setw(indent) <<        "Packets too late" << " = " << m_packetsTooLate << '\n';

  if (m_mediaType == OpalMediaType::Audio())
    strm << setw(indent) <<       "Packet overruns" << " = " << m_packetOverruns << '\n';
    if (m_averageJitter > 0 || m_maximumJitter > 0)
      strm << setw(indent) <<      "Average jitter" << " = " << m_averageJitter << "ms\n"
           << setw(indent) <<      "Maximum jitter" << " = " << m_maximumJitter << "ms\n";
    if (m_jitterBufferDelay > 0)
      strm << setw(indent) << "Jitter buffer delay" << " = " << m_jitterBufferDelay << "ms\n";
#if OPAL_VIDEO
  else if (m_mediaType == OpalMediaType::Video()) {
    strm << setw(indent) <<    "Total video frames" << " = " << m_totalFrames << '\n'
         << setw(indent) <<    "Average Frame rate" << " = "; AsRate(strm, m_totalFrames, totalDuration, 1, "fps") << '\n'
         << setw(indent) <<    "Current Frame rate" << " = "; AsRate(strm, m_deltaFrames, m_updateInterval, 1, "fps") << '\n'
         << setw(indent) <<      "Total key frames" << " = " << m_keyFrames << '\n';
    if (m_quality >= 0)
      strm << setw(indent) <<  "Video quality (QP)" << " = " << m_quality << '\n';
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

class OpalMediaClearText : public OpalMediaCryptoSuite
{
  virtual const PCaselessString & GetFactoryName() const { return ClearText(); }
  virtual bool Supports(const PCaselessString &) const { return true; }
  virtual bool ChangeSessionType(PCaselessString & /*mediaSession*/) const { return true; }
  virtual const char * GetDescription() const { return OpalMediaCryptoSuite::ClearText(); }

  struct KeyInfo : public OpalMediaCryptoKeyInfo
  {
    KeyInfo(const OpalMediaCryptoSuite & cryptoSuite) : OpalMediaCryptoKeyInfo(cryptoSuite) { }
    PObject * Clone() const { return new KeyInfo(m_cryptoSuite); }
    virtual bool IsValid() const { return true; }
    virtual void Randomise() { }
    virtual bool FromString(const PString &) { return true; }
    virtual PString ToString() const { return PString::Empty(); }
  };

  virtual OpalMediaCryptoKeyInfo * CreateKeyInfo() const { return new KeyInfo(*this); }
};

PFACTORY_CREATE(OpalMediaCryptoSuiteFactory, OpalMediaClearText, OpalMediaCryptoSuite::ClearText(), true);


/////////////////////////////////////////////////////////////////////////////

OpalMediaSession::OpalMediaSession(const Init & init)
  : m_connection(init.m_connection)
  , m_sessionId(init.m_sessionId)
  , m_mediaType(init.m_mediaType)
  , m_isExternalTransport(false)
{
  PTRACE_CONTEXT_ID_FROM(init.m_connection);
  PTRACE(5, "Media\tSession " << m_sessionId << " for " << m_mediaType << " created.");
}


bool OpalMediaSession::Open(const PString & PTRACE_PARAM(localInterface),
                            const OpalTransportAddress & remoteAddress,
                            bool isMediaAddress)
{
  PTRACE(5, "Media\tSession " << m_sessionId << ", opening on interface \"" << localInterface << "\" to "
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


void OpalMediaSession::SetExternalTransport(const OpalTransportAddressArray & PTRACE_PARAM(transports))
{
  PTRACE(3, "Media\tSetting external transport to " << setfill(',') << transports);
  m_isExternalTransport = true;
}


#if OPAL_SIP
SDPMediaDescription * OpalMediaSession::CreateSDPMediaDescription()
{
  return m_mediaType->CreateSDPMediaDescription(GetLocalAddress());
}
#endif

#if OPAL_STATISTICS
void OpalMediaSession::GetStatistics(OpalMediaStatistics &, bool) const
{
}
#endif


void OpalMediaSession::OfferCryptoSuite(const PString & cryptoSuiteName)
{
  OpalMediaCryptoSuite * cryptoSuite = OpalMediaCryptoSuiteFactory::CreateInstance(cryptoSuiteName);
  if (cryptoSuite == NULL) {
    PTRACE(1, "Media\tCannot create crypto suite for " << cryptoSuiteName);
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


/////////////////////////////////////////////////////////////////////////////

