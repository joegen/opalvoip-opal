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
        PTRACE(1, "DTLSFactory\tCould not create certificate for DTLS.");
        return;
      }

      if (!UseCertificate(m_cert))
      {
        PTRACE(1, "DTLSFactory\tCould not use DTLS certificate.");
        return;
      }

      if (!UsePrivateKey(pk))
      {
        PTRACE(1, "DTLSFactory\tCould not use private key for DTLS.");
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
        PTRACE(1, "DTLSFactory\tCould not set TLS extension for SSL context.");
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


class OpalDTLSSRTPChannel : public PSSLChannelDTLS
{
  PCLASSINFO(OpalDTLSSRTPChannel, PSSLChannelDTLS);
public:
  OpalDTLSSRTPChannel(OpalDTLSSRTPSession & session, PUDPSocket * socket, bool isServer, bool isData)
    : PSSLChannelDTLS(*(DTLSContextSingleton()))
    , m_session(session)
    , m_usedForData(isData)
  {
    if (isServer)
      Accept(socket, false);
    else
      Connect(socket, false);
  }

  OpalDTLSSRTPSession & m_session;
  bool                  m_usedForData;
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
  m_dtlsReady[e_Data] = false;
  m_dtlsReady[e_Control] = false;
}


OpalDTLSSRTPSession::~OpalDTLSSRTPSession()
{
  Close();
}


OpalRTPSession::SendReceiveStatus OpalDTLSSRTPSession::ReadRawPDU(BYTE * framePtr, PINDEX & frameSize, bool fromDataChannel)
{
  OpalRTPSession::SendReceiveStatus result = OpalSRTPSession::ReadRawPDU(framePtr, frameSize, fromDataChannel);
  if (result == e_ProcessPacket)
    result = HandshakeIfNeeded(framePtr, frameSize, fromDataChannel, true);
  return result;
}


OpalRTPSession::SendReceiveStatus OpalDTLSSRTPSession::OnSendData(RTP_DataFrame& frame, bool rewriteHeader)
{
  OpalRTPSession::SendReceiveStatus result = HandshakeIfNeeded(frame, frame.GetPacketSize(), true, false);
  return result != e_ProcessPacket ? result : OpalSRTPSession::OnSendData(frame, rewriteHeader);
}


OpalRTPSession::SendReceiveStatus OpalDTLSSRTPSession::OnSendControl(RTP_ControlFrame& frame)
{
  OpalRTPSession::SendReceiveStatus result = HandshakeIfNeeded(frame, frame.GetSize(), false, false);
  return result != e_ProcessPacket ? result : OpalSRTPSession::OnSendControl(frame);
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


void OpalDTLSSRTPSession::OnHandshake(PSSLChannelDTLS & channel, P_INT_PTR failed)
{
  if (failed) {
    Close();
    return;
  }

  const OpalMediaCryptoSuite* cryptoSuite = NULL;
  srtp_profile_t profile = srtp_profile_reserved;
  for (PINDEX i = 0; i < PARRAYSIZE(ProfileNames); ++i) {
    if (channel.GetSelectedProfile() == ProfileNames[i].m_dtlsName) {
      profile = ProfileNames[i].m_profile;
      cryptoSuite = OpalMediaCryptoSuiteFactory::CreateInstance(ProfileNames[i].m_opalName);
      break;
    }
  }
  if (profile == srtp_profile_reserved || cryptoSuite == NULL) {
    Close();
    return;
  }

  PINDEX masterKeyLength = srtp_profile_get_master_key_length(profile);
  PINDEX masterSaltLength = srtp_profile_get_master_salt_length(profile);

  PBYTEArray keyMaterial;
  if (!channel.GetKeyMaterial(((masterSaltLength + masterKeyLength) << 1), &keyMaterial))
  {
    Close();
    return;
  }

  const uint8_t *localKey;
  const uint8_t *localSalt;
  const uint8_t *remoteKey;
  const uint8_t *remoteSalt;
  if (channel.IsServer())
  {
    remoteKey = keyMaterial;
    localKey = remoteKey + masterKeyLength;
    remoteSalt = (localKey + masterKeyLength);
    localSalt = (remoteSalt + masterSaltLength);
  }
  else
  {
    localKey = keyMaterial;
    remoteKey = localKey + masterKeyLength;
    localSalt = (remoteKey + masterKeyLength); 
    remoteSalt = (localSalt + masterSaltLength);
  }

  std::auto_ptr<OpalSRTPKeyInfo> keyPtr;

  keyPtr.reset(dynamic_cast<OpalSRTPKeyInfo*>(cryptoSuite->CreateKeyInfo()));
  keyPtr->SetCipherKey(PBYTEArray(remoteKey, masterKeyLength));
  keyPtr->SetAuthSalt(PBYTEArray(remoteSalt, masterSaltLength));

  OpalDTLSSRTPChannel & srtpChannel = dynamic_cast<OpalDTLSSRTPChannel &>(channel);
  if (srtpChannel.m_usedForData) // Data socket context
    SetCryptoKey(keyPtr.get(), true);

  // If we use different ports for data and control, we need different SRTP contexts for each channel.
  //if (!srtpChannel.m_usedForData || IsSinglePortRx()) // Control socket context
  //  OpalLibSRTP::Open(GetSyncSourceIn(), keyPtr.get(), true);

  keyPtr.reset(dynamic_cast<OpalSRTPKeyInfo*>(cryptoSuite->CreateKeyInfo()));
  keyPtr->SetCipherKey(PBYTEArray(localKey, masterKeyLength));
  keyPtr->SetAuthSalt(PBYTEArray(localSalt, masterSaltLength));

  if (srtpChannel.m_usedForData) // Data socket context
    SetCryptoKey(keyPtr.get(), false);

  //if (!srtpChannel.m_usedForData || IsSinglePortTx()) // Control socket context
  //  OpalLibSRTP::Open(GetSyncSourceOut(), keyPtr.get(), false);

  m_dtlsReady[srtpChannel.m_usedForData || IsSinglePortTx()] = true;
}


void OpalDTLSSRTPSession::OnVerify(PSSLChannel & channel, PSSLChannel::VerifyInfo & info)
{
  OpalDTLSSRTPChannel & srtpChannel = dynamic_cast<OpalDTLSSRTPChannel &>(channel);
  info.m_ok = srtpChannel.m_session.GetRemoteFingerprint().MatchForCertificate(info.m_peerCertificate);
  PTRACE_IF(2, !info.m_ok, "DTLSChannel\tInvalid remote certificate.");
}


OpalRTPSession::SendReceiveStatus OpalDTLSSRTPSession::HandshakeIfNeeded(const BYTE * framePtr, PINDEX frameSize, bool dataChannel, bool isReceive)
{
  if (m_dtlsReady[dataChannel])
    return e_ProcessPacket;

  if (IsSinglePortTx())
    dataChannel = true;

  PSSLChannelDTLS* channel = m_sslChannel[dataChannel].get();
  if (channel == NULL) {
    m_socket[dataChannel]->SetSendAddress(m_remoteAddress, m_remotePort[dataChannel]);
    channel = new OpalDTLSSRTPChannel(*this, m_socket[dataChannel], m_passiveMode, dataChannel);
    channel->SetVerifyMode(PSSLContext::VerifyPeerMandatory, PCREATE_SSLVerifyNotifier(OnVerify));
    channel->SetHandshakeNotifier(OnHandshake_PNotifier::Create(this));
    m_sslChannel[dataChannel].reset(channel);
  }

  if (channel->Handshake(framePtr, frameSize, isReceive))
    return e_IgnorePacket;

  Close();
  return e_AbortTransport;
}


bool OpalDTLSSRTPSession::Close()
{
  for (int i = 0; i < 2; ++i) {
    PSSLChannelDTLS* channel = m_sslChannel[i].get();
    if (channel)
      channel->Close();
  }
  return OpalSRTPSession::Close();
}


#endif // OPAL_SRTP
