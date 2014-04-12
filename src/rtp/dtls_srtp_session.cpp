r/*
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


class OpalDTLSSRTPChannel : public PSSLChannelDTLS
{
  PCLASSINFO(OpalDTLSSRTPChannel, PSSLChannelDTLS);
public:
  OpalDTLSSRTPChannel(OpalDTLSSRTPSession & session, PUDPSocket * socket, bool isServer, bool isData)
    : m_session(session)
    , m_usedForData(isData)
  {
    if (isServer)
      Accept(socket);
    else
      Connect(socket);
  }

  OpalDTLSSRTPSession & m_session;
  bool                  m_usedForData;
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
      PSSLCertificate cert;
      if (!cert.CreateRoot(dn, pk))
      {
        PTRACE(1, "DTLSFactory\tCould not create certificate for DTLS.");
        return;
      }

      if (!UseCertificate(cert))
      {
        PTRACE(1, "DTLSFactory\tCould not use DTLS certificate.");
        return;
      }

      if (!UsePrivateKey(pk))
      {
        PTRACE(1, "DTLSFactory\tCould not use private key for DTLS.");
        return;
      }

      m_fingerprint = PSSLCertificateFingerprint(PSSLCertificateFingerprint::HashSha1, cert);

      PStringStream ext;
      for (PINDEX i = 0; i < PARRAYSIZE(ProfileNames); ++i) {
        if (!ext.IsEmpty())
          ext << ':';
        if (OpalMediaCryptoSuiteFactory::CreateInstance(ProfileNames[i].m_opalName) != NULL)
          ext << ProfileNames[i].m_dtlsName;
      }
      if (!SetExtension(ext))
      {
        PTRACE(1, "DTLSFactory\tCould not set TLS extension for SSL context.");
        return;
      }
    }

    const PSSLCertificateFingerprint & GetFingerprint() const
    {
      return m_fingerprint;
    }

  protected:
    PSSLCertificateFingerprint m_fingerprint;
};

typedef PSingleton<DTLSContext> DTLSContextSingleton;


///////////////////////////////////////////////////////////////////////////////

const PCaselessString & OpalDTLSSRTPSession::RTP_DTLS_SAVP () { static const PConstCaselessString s("UDP/TLS/RTP/SAVP" ); return s; }
const PCaselessString & OpalDTLSSRTPSession::RTP_DTLS_SAVPF() { static const PConstCaselessString s("UDP/TLS/RTP/SAVPF" ); return s; }

PFACTORY_CREATE(OpalMediaSessionFactory, OpalDTLSSRTPSession, OpalDTLSSRTPSession::RTP_DTLS_SAVP());
static bool RegisteredSAVPF = OpalMediaSessionFactory::RegisterAs(OpalDTLSSRTPSession::RTP_DTLS_SAVPF(), OpalDTLSSRTPSession::RTP_DTLS_SAVP());


OpalDTLSSRTPSession::OpalDTLSSRTPSession(const Init & init)
  : OpalSRTPSession(init)
  , m_connectionInitiator(false)
  , m_stunNegotiated(false)
  , m_dtlsReady(false)
{
  m_stopSend[0] = false;
  m_stopSend[1] = false;
}


OpalDTLSSRTPSession::~OpalDTLSSRTPSession()
{
  Close();
}


OpalRTPSession::SendReceiveStatus OpalDTLSSRTPSession::ReadRawPDU(BYTE * framePtr, PINDEX & frameSize, bool fromDataChannel)
{
  OpalRTPSession::SendReceiveStatus result = OpalSRTPSession::ReadRawPDU(framePtr, frameSize, fromDataChannel);
  if (result == e_ProcessPacket)
    result = ProcessPacket(framePtr, frameSize, fromDataChannel, true);
  return result;
}


void OpalDTLSSRTPSession::SetConnectionInitiator(bool connectionInitiator)
{
  m_connectionInitiator = connectionInitiator;
}


bool OpalDTLSSRTPSession::GetConnectionInitiator() const
{
  return m_connectionInitiator;
}


OpalRTPSession::SendReceiveStatus OpalDTLSSRTPSession::OnSendData(RTP_DataFrame& frame, bool rewriteHeader)
{
  OpalRTPSession::SendReceiveStatus result = ProcessPacket(frame, frame.GetPacketSize(), true, false);
  if (result == e_ProcessPacket)
    return OpalSRTPSession::OnSendData(frame, rewriteHeader);
  return result;
}


OpalRTPSession::SendReceiveStatus OpalDTLSSRTPSession::OnSendControl(RTP_ControlFrame& frame)
{
  OpalRTPSession::SendReceiveStatus result = ProcessPacket(frame, frame.GetSize(), false, false);
  if (result == e_ProcessPacket)
    return ProtectRTCP(frame) ? e_ProcessPacket : e_IgnorePacket;
  return result;
}


const PSSLCertificateFingerprint& OpalDTLSSRTPSession::GetLocalFingerprint() const
{
  return DTLSContextSingleton()->GetFingerprint();
}


void OpalDTLSSRTPSession::SetRemoteFingerprint(const PSSLCertificateFingerprint& fp)
{
  m_remoteFingerprint = fp;
}


const PSSLCertificateFingerprint& OpalDTLSSRTPSession::GetRemoteFingerprint() const
{
  return m_remoteFingerprint;
}


OpalDTLSSRTPSession::SendReceiveStatus OpalDTLSSRTPSession::ProcessPacket(const BYTE* framePtr, PINDEX frameSize, bool forDataChannel, bool isReceive)
{
  OpalRTPSession::SendReceiveStatus result = IceNegotiation(framePtr, frameSize, forDataChannel, isReceive);
  if (e_ProcessPacket == result || m_stunNegotiated)
    result = HandshakeIfNeeded(framePtr, frameSize, forDataChannel, isReceive);
  return result;
}


bool OpalDTLSSRTPSession::SendStunResponse(const PSTUNMessage& request, PUDPSocket& socket)
{
  PSTUNMessage response;
  if (m_remoteStun.m_userName.IsEmpty())
    m_remoteStun.SetCredentials(m_localUsername + ':' + m_remoteUsername, m_localPassword, PString::Empty());

  PSTUNStringAttribute* userAttr = request.FindAttributeAs<PSTUNStringAttribute>(PSTUNAttribute::USERNAME);
  if (!userAttr)
  {
    PTRACE(2, "DTLS-RTP\tNo USERNAME attribute in " << request << " on interface " << request.GetSourceAddressAndPort());
    response.SetErrorType(400, request.GetTransactionID());
  }
  else if (userAttr->GetString() != m_remoteStun.m_userName) {
    PTRACE(2, "DTLS-RTP\tIncorrect USERNAME attribute in " << request << " on interface " << request.GetSourceAddressAndPort());
    PTRACE(2, "DTLS-RTP\t current " << m_remoteStun.m_userName << ", received " << userAttr->GetString());
    response.SetErrorType(401, request.GetTransactionID());
  }
  else if (!request.CheckMessageIntegrity(m_remoteStun.m_password)) {
    PTRACE(2, "DTLS-RTP\tIntegrity check failed for " << request << " on interface " << request.GetSourceAddressAndPort());
    response.SetErrorType(401, request.GetTransactionID());
  }

  if (response.GetType() != PSTUNMessage::BindingError)
  {
    response.SetType(PSTUNMessage::BindingResponse, request.GetTransactionID());
    response.AddAttribute(PSTUNAddressAttribute(PSTUNAttribute::XOR_MAPPED_ADDRESS, request.GetSourceAddressAndPort()));
  }

  response.AddMessageIntegrity(m_remoteStun.m_password);
  response.AddFingerprint();
  response.Write(socket, request.GetSourceAddressAndPort());

  return (response.GetType() == PSTUNMessage::BindingResponse);
}


bool OpalDTLSSRTPSession::SendStunRequest(PUDPSocket& socket, const PIPSocketAddressAndPort& ap)
{
  PSTUNMessage request(PSTUNMessage::BindingRequest);

  if (m_localStun.m_userName.IsEmpty())
    m_localStun.SetCredentials(m_remoteUsername + ':' + m_localUsername, m_remotePassword, PString::Empty());

  request.AddAttribute(PSTUNStringAttribute(PSTUNAttribute::USERNAME, m_localStun.m_userName));
  request.AddAttribute(PSTUNStringAttribute(PSTUNAttribute::REALM,    m_localStun.m_realm));
  request.AddMessageIntegrity(m_localStun.m_password);

  return request.Write(socket, ap);
}


bool OpalDTLSSRTPSession::CheckStunResponse(const PSTUNMessage& response)
{
  // \todo Check transaction id.
  if (!response.CheckMessageIntegrity(m_localStun.m_password))
  {
    PTRACE(2, "DTLS-RTP\tIntegrity check failed for " << response << " from " << response.GetSourceAddressAndPort());
    return false;
  }
  return true;
}


OpalDTLSSRTPSession::SendReceiveStatus OpalDTLSSRTPSession::IceNegotiation(const BYTE* framePtr, PINDEX frameSize, bool forDataChannel, bool isReceive)
{
  bool useData = (forDataChannel || IsSinglePortTx());
  int index = (useData ? 0 : 1);
  PUDPSocket* socket = (useData ? m_dataSocket : m_controlSocket);

  if (m_candidates[index].size() == 0)
    m_candidates[index].push_back(Candidate(PIPSocketAddressAndPort(m_remoteAddress, (useData ? m_remoteDataPort : m_remoteControlPort))));

  if (isReceive)
  {
    PIPSocketAddressAndPort ap;
    socket->GetLastReceiveAddress(ap);

    PSTUNMessage message(framePtr, frameSize, ap);
    for (Candidates::iterator it = m_candidates[index].begin(); message.IsValid() && it != m_candidates[index].end(); ++it)
    {
      PIPSocketAddressAndPort msgAp = message.GetSourceAddressAndPort();
      // The message not for this candidate...
      if (msgAp != (*it).ap)
        continue;

      if (message.GetType() == PSTUNMessage::BindingRequest)
      {
        PTRACE(3, "DTLS-RTP\tReceive binding request from " << msgAp << ", index " << index);
        (*it).otherPartyRequests++;
        if (SendStunResponse(message, *socket))
          (*it).ourResponses++;

        break;
      }
      else if (message.GetType() == PSTUNMessage::BindingResponse)
      {
        PTRACE(3, "DTLS-RTP\tReceive binding response from " << msgAp << " index " << index);
        if (CheckStunResponse(message))
        {
          (*it).otherPartyResponses++;
          m_stopSend[index] = ((*it).otherPartyResponses > 1);
        }

        break;
      }
      else
      {
        PTRACE(3, "DTLS-RTP\tReceive STUN message " << message);
      }
    }
  }
  else 
  {
    if (!m_stopSend[index])
    {
      for (Candidates::const_iterator it = m_candidates[index].cbegin(); it != m_candidates[index].cend(); ++it)
      {
        PTRACE(4, "DTLS-RTP\tSend stun request to " << (*it).ap << " index " << index);
        if (!SendStunRequest(*socket, (*it).ap))
          PTRACE(2, "DTLS-RTP\tCan't send stun request.");
      }
    }
  }

  for (Candidates::const_iterator it = m_candidates[index].cbegin(); !m_stunNegotiated && it != m_candidates[index].cend(); ++it)
  {
    if ((*it).Ready())
    {
      PTRACE(4, "DTLS-RTP\tSession " << m_sessionId << " remote address set to " << (*it).ap);
      if (useData)
        m_remoteDataPort = (*it).ap.GetPort();
      else
        m_remoteControlPort = (*it).ap.GetPort();
      m_remoteAddress = (*it).ap.GetAddress();
      m_stunNegotiated = true;
      break;
    }
  }

  if (m_stunNegotiated)
    return e_ProcessPacket;

  return e_IgnorePacket;
}


void OpalDTLSSRTPSession::SetCandidates(const PNatCandidateList& candidates)
{
  for (PNatCandidateList::const_iterator aCandidate = candidates.begin(); aCandidate != candidates.end(); ++aCandidate) {
    if (aCandidate->m_localTransportAddress.GetPort() == m_remoteDataPort ||
        aCandidate->m_localTransportAddress.GetPort() == m_remoteControlPort)
    {
      PTRACE(3, "DTLS-RTP\tAppend candidate " << aCandidate->m_localTransportAddress
             << " for " << (aCandidate->m_component == PNatMethod::eComponent_RTP ? "RTP" : "RTCP"));
      m_candidates[aCandidate->m_component - 1].push_back(Candidate(aCandidate->m_localTransportAddress));
    }
  }
}


void OpalDTLSSRTPSession::OnHandshake(PSSLChannelDTLS & channel, INT failed)
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

  unsigned int masterKeyLength = srtp_profile_get_master_key_length(profile);
  unsigned int masterSaltLength = srtp_profile_get_master_salt_length(profile);

  const uint8_t *localKey;
  const uint8_t *localSalt;
  const uint8_t *remoteKey;
  const uint8_t *remoteSalt;
  if (channel.IsServer())
  {
    remoteKey = channel.GetKeyMaterial();
    localKey = remoteKey + masterKeyLength;
    remoteSalt = (localKey + masterKeyLength);
    localSalt = (remoteSalt + masterSaltLength);
  }
  else
  {
    localKey = channel.GetKeyMaterial();
    remoteKey = localKey + masterKeyLength;
    localSalt = (remoteKey + masterKeyLength); 
    remoteSalt = (localSalt + masterSaltLength);
  }

  std::unique_ptr<OpalSRTPKeyInfo> keyPtr;

  keyPtr.reset(dynamic_cast<OpalSRTPKeyInfo*>(cryptoSuite->CreateKeyInfo()));
  keyPtr->SetCipherKey(PBYTEArray(remoteKey, masterKeyLength));
  keyPtr->SetAuthSalt(PBYTEArray(remoteSalt, masterSaltLength));

  OpalDTLSSRTPChannel & srtpChannel = dynamic_cast<OpalDTLSSRTPChannel &>(channel);
  if (srtpChannel.m_usedForData) // Data socket context
    SetCryptoKey(keyPtr.get(), true);

  if (!srtpChannel.m_usedForData || IsSinglePortRx()) // Control socket context
    OpalLibSRTP::Open(GetSyncSourceIn(), keyPtr.get(), true);

  keyPtr.reset(dynamic_cast<OpalSRTPKeyInfo*>(cryptoSuite->CreateKeyInfo()));
  keyPtr->SetCipherKey(PBYTEArray(localKey, masterKeyLength));
  keyPtr->SetAuthSalt(PBYTEArray(localSalt, masterSaltLength));

  if (srtpChannel.m_usedForData) // Data socket context
    SetCryptoKey(keyPtr.get(), false);

  if (!srtpChannel.m_usedForData || IsSinglePortTx()) // Control socket context
    OpalLibSRTP::Open(GetSyncSourceOut(), keyPtr.get(), false);

  m_dtlsReady = true;
}


OpalDTLSSRTPSession::SendReceiveStatus OpalDTLSSRTPSession::OnReceiveControl(RTP_ControlFrame & frame)
{
  return (m_dtlsReady && UnprotectRTCP(frame)) ? OpalRTPSession::OnReceiveControl(frame) : e_IgnorePacket;
}


void OpalDTLSSRTPSession::OnVerify(PSSLChannel & channel, PSSLChannel::VerifyInfo & info)
{
  OpalDTLSSRTPChannel & srtpChannel = dynamic_cast<OpalDTLSSRTPChannel &>(channel);
  info.m_ok = srtpChannel.m_session.GetRemoteFingerprint().MatchForCertificate(info.m_peerCertificate);
  PTRACE_IF(2, !info.m_ok, "DTLSChannel\tInvalid remote certificate.");
}


OpalRTPSession::SendReceiveStatus OpalDTLSSRTPSession::HandshakeIfNeeded(const BYTE * framePtr, PINDEX frameSize, bool forDataChannel, bool isReceive)
{
  int ctxIndex = 1;
  bool useData = (forDataChannel || IsSinglePortTx());
  if (useData)
    ctxIndex = 0;

  PSSLChannelDTLS* channel = m_channels[ctxIndex].get();
  if (channel == NULL) {
    PUDPSocket * socket = useData ? m_dataSocket : m_controlSocket;
    socket->SetSendAddress(m_remoteAddress, (useData ? m_remoteDataPort : m_remoteControlPort));
    m_channels[ctxIndex].reset(channel = new OpalDTLSSRTPChannel(*this, socket, !GetConnectionInitiator(), useData));
    channel->SetVerifyMode(PSSLContext::VerifyPeerMandatory, PCREATE_SSLVerifyNotifier(OnVerify));
  }

  if (!m_dtlsReady && channel && !channel->HandshakeCompleted()) {
    if (channel->Handshake(framePtr, frameSize, isReceive))
      return e_IgnorePacket;

    Close();
    return e_AbortTransport;
  }

  return e_ProcessPacket;
}

#endif // OPAL_SRTP
