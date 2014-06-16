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

typedef PSingleton<DTLSContext> DTLSContextSingleton;


class OpalDTLSSRTPSession::SSLChannel : public PSSLChannelDTLS
{
  PCLASSINFO(SSLChannel, PSSLChannelDTLS);
public:
  SSLChannel(OpalDTLSSRTPSession & session, PUDPSocket * socket, PQueueChannel & queue, bool isServer)
    : PSSLChannelDTLS(*DTLSContextSingleton())
    , m_session(session)
  {
    SetVerifyMode(PSSLContext::VerifyPeerMandatory, PCREATE_NOTIFIER2_EXT(session, OpalDTLSSRTPSession, OnVerify, PSSLChannel::VerifyInfo &));

    queue.Open(2000);
    Open(&queue, socket, false, false);
    SetReadTimeout(2000);

    if (isServer)
      Accept();
    else
      Connect();
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
  if (!OpalSRTPSession::Open(localInterface, remoteAddress, mediaAddress))
    return false;

  for (int i = 0; i < 2; ++i)
    m_sslChannel[i] = new SSLChannel(*this, m_socket[i], m_queueChannel[i], m_passiveMode);

  return true;
}


bool OpalDTLSSRTPSession::Close()
{
  for (int i = 0; i < 2; ++i) {
    m_queueChannel[i].Close();
    delete m_sslChannel[i];
    m_sslChannel[i] = NULL;
  }

  return OpalSRTPSession::Close();
}


OpalRTPSession::SendReceiveStatus OpalDTLSSRTPSession::ReadRawPDU(BYTE * framePtr, PINDEX & frameSize, bool fromDataChannel)
{
  OpalRTPSession::SendReceiveStatus status = OpalSRTPSession::ReadRawPDU(framePtr, frameSize, fromDataChannel);
  if (status != e_ProcessPacket || !m_queueChannel[fromDataChannel].IsOpen())
    return status;

  if (frameSize == 0 || framePtr[0] < 20 || framePtr[0] > 64)
    return e_IgnorePacket; // Not a DTLS packet

  PTRACE(5, "Read DTLS packet: " << frameSize << " bytes, queueing to SSL.");
  return m_queueChannel[fromDataChannel].Write(framePtr, frameSize) ? e_IgnorePacket : e_AbortTransport;
}


OpalRTPSession::SendReceiveStatus OpalDTLSSRTPSession::OnSendData(RTP_DataFrame & frame, bool rewriteHeader)
{
  return ExecuteHandshake(true) ? OpalSRTPSession::OnSendData(frame, rewriteHeader) : e_AbortTransport;
}


OpalRTPSession::SendReceiveStatus OpalDTLSSRTPSession::OnSendControl(RTP_ControlFrame & frame)
{
  return ExecuteHandshake(false) ? OpalSRTPSession::OnSendControl(frame) : e_AbortTransport;
}


const PSSLCertificateFingerprint& OpalDTLSSRTPSession::GetLocalFingerprint() const
{
  if (!m_localFingerprint.IsValid()) {
    PSSLCertificateFingerprint::HashType type = PSSLCertificateFingerprint::HashSha1;
    if (GetRemoteFingerprint().IsValid())
      type = GetRemoteFingerprint().GetHash();
    const_cast<OpalDTLSSRTPSession*>(this)->m_localFingerprint = DTLSContextSingleton()->GetFingerprint(type);
  }
  return m_localFingerprint;
}


void OpalDTLSSRTPSession::SetRemoteFingerprint(const PSSLCertificateFingerprint& fp)
{
  m_remoteFingerprint = fp;
}


const PSSLCertificateFingerprint& OpalDTLSSRTPSession::GetRemoteFingerprint() const
{
  return m_remoteFingerprint;
}


bool OpalDTLSSRTPSession::ExecuteHandshake(bool dataChannel)
{
  {
    PWaitAndSignal mutex(m_dataMutex);

    if (m_remotePort[dataChannel] == 0)
      return true; // Too early

    if (m_sslChannel[dataChannel] == NULL)
      return true; // Too late (done)
  }

  SSLChannel & sslChannel = *m_sslChannel[dataChannel];
  if (!sslChannel.ExecuteHandshake())
    return false;

  const OpalMediaCryptoSuite* cryptoSuite = NULL;
  srtp_profile_t profile = srtp_profile_reserved;
  for (PINDEX i = 0; i < PARRAYSIZE(ProfileNames); ++i) {
    if (sslChannel.GetSelectedProfile() == ProfileNames[i].m_dtlsName) {
      profile = ProfileNames[i].m_profile;
      cryptoSuite = OpalMediaCryptoSuiteFactory::CreateInstance(ProfileNames[i].m_opalName);
      break;
    }
  }

  if (profile == srtp_profile_reserved || cryptoSuite == NULL)
    return false;

  PINDEX masterKeyLength = srtp_profile_get_master_key_length(profile);
  PINDEX masterSaltLength = srtp_profile_get_master_salt_length(profile);

  PBYTEArray keyMaterial = sslChannel.GetKeyMaterial(((masterSaltLength + masterKeyLength) << 1), "EXTRACTOR-dtls_srtp");
  if (keyMaterial.IsEmpty())
    return false;

  const uint8_t *localKey;
  const uint8_t *localSalt;
  const uint8_t *remoteKey;
  const uint8_t *remoteSalt;
  if (sslChannel.IsServer()) {
    remoteKey = keyMaterial;
    localKey = remoteKey + masterKeyLength;
    remoteSalt = (localKey + masterKeyLength);
    localSalt = (remoteSalt + masterSaltLength);
  }
  else {
    localKey = keyMaterial;
    remoteKey = localKey + masterKeyLength;
    localSalt = (remoteKey + masterKeyLength); 
    remoteSalt = (localSalt + masterSaltLength);
  }

  std::auto_ptr<OpalSRTPKeyInfo> keyPtr;

  keyPtr.reset(dynamic_cast<OpalSRTPKeyInfo*>(cryptoSuite->CreateKeyInfo()));
  keyPtr->SetCipherKey(PBYTEArray(remoteKey, masterKeyLength));
  keyPtr->SetAuthSalt(PBYTEArray(remoteSalt, masterSaltLength));

  if (dataChannel) // Data socket context
    SetCryptoKey(keyPtr.get(), true);

  keyPtr.reset(dynamic_cast<OpalSRTPKeyInfo*>(cryptoSuite->CreateKeyInfo()));
  keyPtr->SetCipherKey(PBYTEArray(localKey, masterKeyLength));
  keyPtr->SetAuthSalt(PBYTEArray(localSalt, masterSaltLength));

  if (dataChannel) // Data socket context
    SetCryptoKey(keyPtr.get(), false);

  sslChannel.Detach(PChannel::ShutdownWrite); // Do not close the socket!
  delete m_sslChannel[dataChannel];
  m_sslChannel[dataChannel] = NULL;
  return true;
}


void OpalDTLSSRTPSession::OnVerify(PSSLChannel & channel, PSSLChannel::VerifyInfo & info)
{
  info.m_ok = dynamic_cast<SSLChannel &>(channel).m_session.GetRemoteFingerprint().MatchForCertificate(info.m_peerCertificate);
  PTRACE_IF(2, !info.m_ok, "Invalid remote certificate.");
}


#endif // OPAL_SRTP
