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

#if OPAL_SRTP==2
#define uint32_t uint32_t
#pragma warning(disable:4505)
#include <srtp.h>
#elif HAS_SRTP_SRTP_H
#include <srtp/srtp.h>
#else
#include <srtp.h>
#endif


#define PTraceModule() "DTLS"


// from srtp_profile_t
static struct ProfileInfo
{
  srtp_profile_t m_profile;
  const char *   m_dtlsName;
  const char *   m_opalName;
} const ProfileNames[] = {
  { srtp_profile_aes128_cm_sha1_80, "SRTP_AES128_CM_SHA1_80", "AES_CM_128_HMAC_SHA1_80" },
  { srtp_profile_aes128_cm_sha1_32, "SRTP_AES128_CM_SHA1_32", "AES_CM_128_HMAC_SHA1_32" },
  { srtp_profile_aes256_cm_sha1_80, "SRTP_AES256_CM_SHA1_80", "AES_CM_256_HMAC_SHA1_80" },
  { srtp_profile_aes256_cm_sha1_32, "SRTP_AES256_CM_SHA1_32", "AES_CM_256_HMAC_SHA1_32" },
};



class DTLSContext : public PSSLContext
{
    PCLASSINFO(DTLSContext, PSSLContext);
  public:
    DTLSContext()
      : PSSLContext(PSSLContext::DTLSv1)
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

    PSSLCertificateFingerprint GetFingerprint(PSSLCertificateFingerprint::HashType aType) const
    {
      return PSSLCertificateFingerprint(aType, m_cert);
    }

  protected:
    PSSLCertificate m_cert;
};

typedef PSingleton<DTLSContext, PAtomicInteger> DTLSContextSingleton;


class OpalDTLSSRTPSession::SSLChannel : public PSSLChannelDTLS
{
  PCLASSINFO(SSLChannel, PSSLChannelDTLS);
public:
  SSLChannel(OpalDTLSSRTPSession & session, PUDPSocket * socket, bool isServer)
    : PSSLChannelDTLS(*DTLSContextSingleton())
    , m_session(session)
  {
    SetVerifyMode(PSSLContext::VerifyPeerMandatory, PCREATE_NOTIFIER2_EXT(session, OpalDTLSSRTPSession, OnVerify, PSSLChannel::VerifyInfo &));

    Open(socket, false);
    SetReadTimeout(2000);

    if (isServer)
      Accept();
    else
      Connect();
  }

  virtual int BioWrite(const char * buf, int len)
  {
    PUDPSocket * udp = dynamic_cast<PUDPSocket *>(GetBaseReadChannel());
    if (udp == NULL)
      return PSSLChannel::BioWrite(buf, len);

    PIPSocketAddressAndPort rx;
    udp->GetLastReceiveAddress(rx);
    if (!rx.IsValid())
      return PSSLChannel::BioWrite(buf, len);

    // Make sure we send replies to the address that the packet came in on
    PIPSocketAddressAndPort old;
    udp->GetSendAddress(old);
    udp->SetSendAddress(rx);

    int ret = PSSLChannel::BioWrite(buf, len);

    udp->SetSendAddress(old);

    return ret;
  }

  OpalDTLSSRTPSession & m_session;
};


///////////////////////////////////////////////////////////////////////////////

const PCaselessString & OpalDTLSSRTPSession::RTP_DTLS_SAVP () { static const PConstCaselessString s("UDP/TLS/RTP/SAVP" ); return s; }
const PCaselessString & OpalDTLSSRTPSession::RTP_DTLS_SAVPF() { static const PConstCaselessString s("UDP/TLS/RTP/SAVPF" ); return s; }

PFACTORY_CREATE(OpalMediaSessionFactory, OpalDTLSSRTPSession, OpalDTLSSRTPSession::RTP_DTLS_SAVP());
static bool RegisteredSAVPF = OpalMediaSessionFactory::RegisterAs(OpalDTLSSRTPSession::RTP_DTLS_SAVPF(), OpalDTLSSRTPSession::RTP_DTLS_SAVP());


OpalDTLSSRTPSession::OpalDTLSSRTPSession(const Init & init)
  : OpalSRTPSession(init)
  , m_passiveMode(false)
{
  m_sslChannel[e_Control] = m_sslChannel[e_Data] = NULL;
}


OpalDTLSSRTPSession::~OpalDTLSSRTPSession()
{
  Close();
}


bool OpalDTLSSRTPSession::Open(const PString & localInterface, const OpalTransportAddress & remoteAddress, bool mediaAddress)
{
  PSafeLockReadWrite lock(*this);
  if (!lock.IsLocked())
    return false;

  if (IsOpen())
    return true;

  if (!OpalSRTPSession::Open(localInterface, remoteAddress, mediaAddress))
    return false;

  for (int i = 0; i < 2; ++i) {
    if (m_socket[i] != NULL)
      m_sslChannel[i] = new SSLChannel(*this, m_socket[i], m_passiveMode);
  }

  return true;
}


bool OpalDTLSSRTPSession::Close()
{
  PTRACE(4, "Closing DTLS.");

  bool ok = OpalSRTPSession::Close();

  PSafeLockReadWrite lock(*this);

  for (int i = 0; i < 2; ++i) {
    delete m_sslChannel[i];
    m_sslChannel[i] = NULL;
  }

  return ok;
}


void OpalDTLSSRTPSession::ThreadMain()
{
  if (ExecuteHandshake(e_Data) && ExecuteHandshake(e_Control))
    OpalSRTPSession::ThreadMain();
}


OpalRTPSession::SendReceiveStatus OpalDTLSSRTPSession::OnSendData(RTP_DataFrame & frame, RewriteMode rewrite)
{
  // Already locked on entry
  return m_sslChannel[e_Data] != NULL ? e_IgnorePacket : OpalSRTPSession::OnSendData(frame, rewrite);
}


OpalRTPSession::SendReceiveStatus OpalDTLSSRTPSession::OnSendControl(RTP_ControlFrame & frame)
{
  // Already locked on entry
  return m_sslChannel[IsSinglePortRx() || IsSinglePortTx() ? e_Data : e_Control] != NULL ? e_IgnorePacket : OpalSRTPSession::OnSendControl(frame);
}


PSSLCertificateFingerprint OpalDTLSSRTPSession::GetLocalFingerprint() const
{
  return DTLSContextSingleton()->GetFingerprint(m_remoteFingerprint.IsValid() ? m_remoteFingerprint.GetHash() : PSSLCertificateFingerprint::HashSha1);
}


void OpalDTLSSRTPSession::SetRemoteFingerprint(const PSSLCertificateFingerprint& fp)
{
  m_remoteFingerprint = fp;
}


const PSSLCertificateFingerprint& OpalDTLSSRTPSession::GetRemoteFingerprint() const
{
  return m_remoteFingerprint;
}


bool OpalDTLSSRTPSession::ExecuteHandshake(Channel channel)
{
  if (m_sslChannel[channel] == NULL)
    return IsOpen();

  while (!m_remoteAddress.IsValid() || m_remotePort[channel] == 0) {
    if (!InternalRead())
      return false;
  }

  if (!m_sslChannel[channel]->ExecuteHandshake()) {
    PTRACE(2, *this << "error in DTLS handshake.");
    return false;
  }

  PSafeLockReadWrite lock(*this);
  if (!lock.IsLocked())
    return false;

  const OpalMediaCryptoSuite* cryptoSuite = NULL;
  srtp_profile_t profile = srtp_profile_reserved;
  for (PINDEX i = 0; i < PARRAYSIZE(ProfileNames); ++i) {
    if (m_sslChannel[channel]->GetSelectedProfile() == ProfileNames[i].m_dtlsName) {
      profile = ProfileNames[i].m_profile;
      cryptoSuite = OpalMediaCryptoSuiteFactory::CreateInstance(ProfileNames[i].m_opalName);
      break;
    }
  }

  if (profile == srtp_profile_reserved || cryptoSuite == NULL) {
    PTRACE(2, *this << "error in SRTP profile after DTLS handshake.");
    return false;
  }

  PINDEX masterKeyLength = srtp_profile_get_master_key_length(profile);
  PINDEX masterSaltLength = srtp_profile_get_master_salt_length(profile);

  PBYTEArray keyMaterial = m_sslChannel[channel]->GetKeyMaterial(((masterSaltLength + masterKeyLength) << 1), "EXTRACTOR-dtls_srtp");
  if (keyMaterial.IsEmpty()) {
    PTRACE(2, *this << "no SRTP keys after DTLS handshake.");
    return false;
  }

  OpalSRTPKeyInfo * keyInfo = dynamic_cast<OpalSRTPKeyInfo*>(cryptoSuite->CreateKeyInfo());
  PAssertNULL(keyInfo);

  keyInfo->SetCipherKey(PBYTEArray(keyMaterial, masterKeyLength));
  keyInfo->SetAuthSalt(PBYTEArray(keyMaterial + masterKeyLength*2, masterSaltLength));
  ApplyKeyToSRTP(*keyInfo, m_sslChannel[channel]->IsServer() ? e_Receiver : e_Sender);

  keyInfo->SetCipherKey(PBYTEArray(keyMaterial + masterKeyLength, masterKeyLength));
  keyInfo->SetAuthSalt(PBYTEArray(keyMaterial + masterKeyLength*2 + masterSaltLength, masterSaltLength));
  ApplyKeyToSRTP(*keyInfo, m_sslChannel[channel]->IsServer() ? e_Sender : e_Receiver);

  delete keyInfo;

  m_sslChannel[channel]->Detach();
  delete m_sslChannel[channel];
  m_sslChannel[channel] = NULL;
  PTRACE(2, *this << "completed DTLS handshake.");
  return true;
}


void OpalDTLSSRTPSession::OnVerify(PSSLChannel & channel, PSSLChannel::VerifyInfo & info)
{
  info.m_ok = dynamic_cast<SSLChannel &>(channel).m_session.GetRemoteFingerprint().MatchForCertificate(info.m_peerCertificate);
  PTRACE_IF(2, !info.m_ok, "Invalid remote certificate.");
}


#endif // OPAL_SRTP
