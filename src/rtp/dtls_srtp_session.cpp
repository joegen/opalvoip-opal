/*
 * dtls_srtp_session.cxx
 *
 * SRTP protocol session handler with DTLS key exchange
 *
 * OPAL Library
 *
 * Copyright (C) 2014 Vox Lucida Pty. Ltd.
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
 * The Original Code is OPAL Library.
 *
 * The Initial Developer of the Original Code is Sysolyatin Pavel
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "dtls_srtp_session.h"
#endif

#include <opal_config.h>

#if OPAL_SRTP
#include <rtp/dtls_srtp_session.h>
#ifdef _MSC_VER
#pragma comment(lib, P_SSL_LIB1)
#pragma comment(lib, P_SSL_LIB2)
#endif

#include <opal/manager.h>
#include <opal/endpoint.h>
#include <opal/connection.h>


#define PTraceModule() "DTLS"


// from srtp_profile_t
static struct ProfileInfo
{
  const char *   m_dtlsName;
  const char *   m_opalName;
} const ProfileNames[] = {
  { "SRTP_AES128_CM_SHA1_80", "AES_CM_128_HMAC_SHA1_80" },
  { "SRTP_AES128_CM_SHA1_32", "AES_CM_128_HMAC_SHA1_32" },
  { "SRTP_AES256_CM_SHA1_80", "AES_CM_256_HMAC_SHA1_80" },
  { "SRTP_AES256_CM_SHA1_32", "AES_CM_256_HMAC_SHA1_32" },
};



class OpalDTLSContext : public PSSLContext
{
    PCLASSINFO(OpalDTLSContext, PSSLContext);
  public:
    OpalDTLSContext()
      : PSSLContext(PSSLContext::DTLSv1_2_v1_0)
    {
      PStringStream dn;
      dn << "/O=" << PProcess::Current().GetManufacturer()
        << "/CN=*";

      PSSLPrivateKey pk(1024);
      if (!m_cert.CreateRoot(dn, pk))
      {
        PTRACE(1, "Could not create certificate for DTLS.");
        return;
      }

      if (!UseCertificate(m_cert))
      {
        PTRACE(1, "Could not use DTLS certificate.");
        return;
      }

      if (!UsePrivateKey(pk))
      {
        PTRACE(1, "Could not use private key for DTLS.");
        return;
      }

      PStringStream ext;
      for (PINDEX i = 0; i < PARRAYSIZE(ProfileNames); ++i) {
        const OpalMediaCryptoSuite* cryptoSuite = OpalMediaCryptoSuiteFactory::CreateInstance(ProfileNames[i].m_opalName);
        if (cryptoSuite)
        {
          if (!ext.IsEmpty())
            ext << ':';
          ext << ProfileNames[i].m_dtlsName;
        }
      }
      if (!SetExtension(ext))
      {
        PTRACE(1, "Could not set TLS extension for SSL context.");
        return;
      }
    }

    PSSLCertificateFingerprint GetFingerprint(PSSLCertificateFingerprint::HashType hashType) const
    {
      return PSSLCertificateFingerprint(hashType, m_cert);
    }

  protected:
    PSSLCertificate m_cert;
};

typedef PSingleton<OpalDTLSContext, atomic<uint32_t> > DTLSContextSingleton;


OpalDTLSMediaTransport::DTLSChannel::DTLSChannel()
  : PSSLChannelDTLS(*DTLSContextSingleton())
{
}


#if PTRACING
int OpalDTLSMediaTransport::DTLSChannel::BioRead(char * buf, int len)
{
  int result;
  if ((result = PSSLChannelDTLS::BioRead(buf, len)) > 0) {
    PTRACE(5, "Read " << result << " bytes from "
            << dynamic_cast<PUDPSocket *>(GetBaseReadChannel())->GetLastReceiveAddress());
    return result;
  }

  PTRACE(2, "Read error: " << GetErrorText(PChannel::LastReadError));
  return result;
}


int OpalDTLSMediaTransport::DTLSChannel::BioWrite(const char * buf, int len)
{
  int result = PSSLChannelDTLS::BioWrite(buf, len);
  if (result > 0)
    PTRACE(5, "Written " << result << " bytes to "
            << dynamic_cast<PUDPSocket *>(GetBaseReadChannel())->GetSendAddress());
  else
    PTRACE(2, "Write error: " << GetErrorText(PChannel::LastWriteError));
  return result;
}
#endif // PTRACING


OpalDTLSMediaTransport::OpalDTLSMediaTransport(const PString & name, bool passiveMode, const PSSLCertificateFingerprint& fp)
  : OpalDTLSMediaTransportParent(name)
  , m_passiveMode(passiveMode)
  , m_handshakeTimeout(0, 2)
  , m_MTU(1400)
  , m_remoteFingerprint(fp)
{
}


bool OpalDTLSMediaTransport::Open(OpalMediaSession & session,
                                  PINDEX count,
                                  const PString & localInterface,
                                  const OpalTransportAddress & remoteAddress)
{
  m_handshakeTimeout = dynamic_cast<OpalDTLSSRTPSession &>(session).GetHandshakeTimeout();
  m_MTU = session.GetConnection().GetMaxRtpPayloadSize();
  return OpalDTLSMediaTransportParent::Open(session, count, localInterface, remoteAddress);
}


bool OpalDTLSMediaTransport::IsEstablished() const
{
  for (PINDEX i = 0; i < 2; ++i) {
    if (m_keyInfo[i].get() == NULL)
      return false;
  }
  return OpalICEMediaTransport::IsEstablished();
}


bool OpalDTLSMediaTransport::GetKeyInfo(OpalMediaCryptoKeyInfo * keyInfo[2])
{
  for (PINDEX i = 0; i < 2; ++i) {
    if ((keyInfo[i] = m_keyInfo[i].get()) == NULL)
      return false;
  }
  return true;
}


OpalDTLSMediaTransport::DTLSChannel * OpalDTLSMediaTransport::CreateDTLSChannel()
{
  return new DTLSChannel();
}


void OpalDTLSMediaTransport::InternalOnStart(SubChannels subchannel)
{
  OpalDTLSMediaTransportParent::InternalOnStart(subchannel);

  PChannel * baseChannel = GetChannel(subchannel);
  if (baseChannel == NULL)
    return;

  std::auto_ptr<DTLSChannel> sslChannel(CreateDTLSChannel());
  if (sslChannel.get() == NULL) {
    PTRACE(2, *this << "could not create DTLS channel");
    baseChannel->Close();
    return;
  }

  sslChannel->SetMTU(m_MTU);
  sslChannel->SetVerifyMode(PSSLContext::VerifyPeerMandatory, PCREATE_NOTIFIER2_EXT(*this, OpalDTLSMediaTransport, OnVerify, PSSLChannel::VerifyInfo &));

  if (!sslChannel->Open(baseChannel, false)) {
    PTRACE(2, *this << "could not open DTLS channel");
    return;
  }

  if (!(m_passiveMode ? sslChannel->Accept() : sslChannel->Connect())) {
    PTRACE(2, *this << "could not " << (m_passiveMode ? "accept" : "connect") << " DTLS channel");
    return;
  }

  PTimeInterval oldTimeout = sslChannel->GetReadTimeout();
  sslChannel->SetReadTimeout(m_handshakeTimeout);

  if (!sslChannel->ExecuteHandshake()) {
    PTRACE(2, *this << "error in DTLS handshake.");
    return;
  }

  PCaselessString profileName = sslChannel->GetSelectedProfile();
  const OpalMediaCryptoSuite* cryptoSuite = NULL;
  for (PINDEX i = 0; i < PARRAYSIZE(ProfileNames); ++i) {
    if (profileName == ProfileNames[i].m_dtlsName) {
      cryptoSuite = OpalMediaCryptoSuiteFactory::CreateInstance(ProfileNames[i].m_opalName);
      break;
    }
  }

  if (cryptoSuite == NULL) {
    PTRACE(2, *this << "error in SRTP profile (" << profileName << ") after DTLS handshake.");
    return;
  }

  PINDEX keyLength = cryptoSuite->GetCipherKeyBytes();
  PINDEX saltLength = std::max(cryptoSuite->GetAuthSaltBytes(), (PINDEX)14); // Weird, but 32 bit salt still needs 14 bytes,

  PBYTEArray keyMaterial = sslChannel->GetKeyMaterial((saltLength + keyLength)*2, "EXTRACTOR-dtls_srtp");
  if (keyMaterial.IsEmpty()) {
    PTRACE(2, *this << "no SRTP keys after DTLS handshake.");
    return;
  }

  OpalSRTPKeyInfo * keyInfo = dynamic_cast<OpalSRTPKeyInfo*>(cryptoSuite->CreateKeyInfo());
  PAssertNULL(keyInfo);

  keyInfo->SetCipherKey(PBYTEArray(keyMaterial, keyLength));
  keyInfo->SetAuthSalt(PBYTEArray(keyMaterial + keyLength*2, saltLength));
  m_keyInfo[sslChannel->IsServer() ? OpalRTPSession::e_Receiver : OpalRTPSession::e_Sender].reset(keyInfo);

  keyInfo = dynamic_cast<OpalSRTPKeyInfo*>(cryptoSuite->CreateKeyInfo());
  keyInfo->SetCipherKey(PBYTEArray(keyMaterial + keyLength, keyLength));
  keyInfo->SetAuthSalt(PBYTEArray(keyMaterial + keyLength*2 + saltLength, saltLength));
  m_keyInfo[sslChannel->IsServer() ? OpalRTPSession::e_Sender : OpalRTPSession::e_Receiver].reset(keyInfo);

  sslChannel->Detach()->SetReadTimeout(oldTimeout);
  PTRACE(3, *this << "completed DTLS handshake.");
}


void OpalDTLSMediaTransport::OnVerify(PSSLChannel &, PSSLChannel::VerifyInfo & info)
{
  info.m_ok = m_remoteFingerprint.MatchForCertificate(info.m_peerCertificate);
  PTRACE_IF(2, !info.m_ok, "Invalid remote certificate.");
}


///////////////////////////////////////////////////////////////////////////////

const PCaselessString & OpalDTLSSRTPSession::RTP_DTLS_SAVP () { static const PConstCaselessString s("UDP/TLS/RTP/SAVP" ); return s; }
const PCaselessString & OpalDTLSSRTPSession::RTP_DTLS_SAVPF() { static const PConstCaselessString s("UDP/TLS/RTP/SAVPF" ); return s; }

PFACTORY_CREATE(OpalMediaSessionFactory, OpalDTLSSRTPSession, OpalDTLSSRTPSession::RTP_DTLS_SAVP());
bool OpalRegisteredSAVPF = OpalMediaSessionFactory::RegisterAs(OpalDTLSSRTPSession::RTP_DTLS_SAVPF(), OpalDTLSSRTPSession::RTP_DTLS_SAVP());


OpalDTLSSRTPSession::OpalDTLSSRTPSession(const Init & init)
  : OpalSRTPSession(init)
  , m_passiveMode(false)
  , m_handshakeTimeout(0, 2)
{
}


OpalDTLSSRTPSession::~OpalDTLSSRTPSession()
{
  Close();
}


void OpalDTLSSRTPSession::SetPassiveMode(bool passive)
{
  PTRACE(4, *this << "set DTLS to " << (passive ? "passive" : " active") << " mode");
  m_passiveMode = passive;

  OpalMediaTransportPtr transport = m_transport;
  if (transport == NULL)
    return;

  OpalDTLSMediaTransport * dtls = dynamic_cast<OpalDTLSMediaTransport *>(&*transport);
  if (dtls != NULL)
    dtls->SetPassiveMode(passive);
}


const PSSLCertificateFingerprint & OpalDTLSSRTPSession::GetLocalFingerprint(PSSLCertificateFingerprint::HashType preferredHashType) const
{
  if (!m_localFingerprint.IsValid())
    const_cast<OpalDTLSSRTPSession*>(this)->m_localFingerprint = DTLSContextSingleton()->GetFingerprint(preferredHashType);
  return m_localFingerprint;
}


void OpalDTLSSRTPSession::SetRemoteFingerprint(const PSSLCertificateFingerprint& fp)
{
  m_remoteFingerprint = fp;
}


OpalMediaTransport * OpalDTLSSRTPSession::CreateMediaTransport(const PString & name)
{
  return new OpalDTLSMediaTransport(name, m_passiveMode, m_remoteFingerprint);
}


#endif // OPAL_SRTP
