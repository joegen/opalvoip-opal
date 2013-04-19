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
  : m_startTime(0)
  , m_totalBytes(0)
  , m_totalPackets(0)
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
  , m_referenceCount(1)
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

