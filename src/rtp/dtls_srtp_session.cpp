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

extern "C" {
#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>
}

#if OPAL_SRTP==2
#define uint32_t uint32_t
#pragma warning(disable:4505)
#include <srtp.h>
#elif HAS_SRTP_SRTP_H
#include <srtp/srtp.h>
#else
#include <srtp.h>
#endif


#if !defined(SRTP_MAX_KEY_LEN)
#	define cipher_key_length (128 >> 3) // rfc5764 4.1.2.  SRTP Protection Profiles
#	define cipher_salt_length (112 >> 3) // rfc5764 4.1.2.  SRTP Protection Profiles
#	define SRTP_MAX_KEY_LEN (cipher_key_length + cipher_salt_length)
#endif /* SRTP_MAX_KEY_LEN */

static const size_t KEYRING_MATERIAL_SIZE = (SRTP_MAX_KEY_LEN << 1);

#define EXTRACTOR_DTLS_SRTP_TEXT "EXTRACTOR-dtls_srtp"
#define EXTRACTOR_DTLS_SRTP_TEXT_LEN 19

const int HandshakeReadTimeout = 500; // ms

const char* DefaultSrtpProfile = "SRTP_AES128_CM_SHA1_80:SRTP_AES128_CM_SHA1_32";

class DTLSFactory
{
public:
  DTLSFactory()
    : m_ready(false)
    , m_context(PSSLContext::DTLSv1)
    , m_pk(1024)
  {
    PStringStream dn;
    dn << "/O=" << PProcess::Current().GetManufacturer()
      << "/CN=*";

    if (!m_cert.CreateRoot(dn, m_pk))
    {
      PTRACE(1, "DTLSFactory\tCould not create certificate for DTLS.");
      return;
    }

    if (!m_context.UseCertificate(m_cert))
    {
      PTRACE(1, "DTLSFactory\tCould not use DTLS certificate.");
      return;
    }

    if (!m_context.UsePrivateKey(m_pk))
    {
      PTRACE(1, "DTLSFactory\tCould not use private key for DTLS.");
      return;
    }

    if (SSL_CTX_set_tlsext_use_srtp(m_context, DefaultSrtpProfile) != 0)
    {
      PTRACE(1, "DTLSFactory\tCould not set TLS extension for SSL context.");
      return;
    }

    m_localFingerprint = PSSLCertificateFingerprint(PSSLCertificateFingerprint::HashSha256, m_cert);

    m_ready = true;
  }
  SSL_CTX* Context()
  {
    return m_context;
  }
  bool IsReady() const
  {
    return m_ready;
  }
  const PSSLCertificateFingerprint& GetFingerprint() const
  {
    return m_localFingerprint;
  }
private:
  bool m_ready;
  PSSLContext m_context;
  PSSLCertificate m_cert;
  PSSLPrivateKey m_pk;
  PSSLCertificateFingerprint m_localFingerprint;
};

class CSHelper
{
public:
  CSHelper(PMutex& cs)
    : m_cs(cs)
    , m_entered(m_cs.Try())
  {
  }
  ~CSHelper()
  {
    if (m_entered)
      m_cs.Leave();
  }
  bool Entered() const
  {
    return m_entered;
  }
private:
  PMutex& m_cs;
  bool m_entered;
};

class DTLSContext
{
public:
  DTLSContext(PUDPSocket* socket, const PIPSocketAddressAndPort& remoteAddresAndPort, DTLSFactory* factory, bool asServer)
    : m_handshakeFinished(false)
    , m_factory(factory)
    , m_asServer(asServer)
    , m_waitResponse(false)
    , m_inBio(0)
    , m_outBio(0)
    , m_callback(0)
    , m_useForData(false)
  {
    m_socketData.socket = socket;
    m_socketData.ap = remoteAddresAndPort;
    m_ssl = SSL_new(m_factory->Context());
    PAssert(m_ssl != 0, "SSL_new error");

    if (m_asServer)
    {
      SSL_set_accept_state(m_ssl);
      SSL_set_verify(m_ssl, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, DTLSContext::VerifyCertificateCallback);
    }
    else
    {
      SSL_set_connect_state(m_ssl);
    }

    m_inBio = BIO_new(BIO_s_mem());
    m_outBio = BIO_new(BIO_s_mem());

    BIO_set_mem_eof_return(m_inBio, -1);
    BIO_set_mem_eof_return(m_outBio, -1);

    SSL_set_bio(m_ssl, m_inBio, m_outBio);

    SSL_set_mode(m_ssl, SSL_MODE_AUTO_RETRY);
    SSL_set_read_ahead(m_ssl, 1);

    SSL_set_app_data(m_ssl, this);

    m_readTimer.SetNotifier(PCREATE_NOTIFIER(OnReadTimeout));
  }

  bool IsServer() const
  {
    return m_asServer;
  }
  void SetUsedForData(bool isData)
  {
    m_useForData = isData;
  }
  bool GetUsedForData() const
  {
    return m_useForData;
  }
  void SetCallback(DTLSHandshakeListener* callBack)
  {
    m_callback = callBack;
  }

  // Returns false if handshake phase failed. 
  bool Handshake(const BYTE * framePtr, PINDEX frameSize, bool isReceive)
  {
    if (!m_factory->IsReady())
    {
      PTRACE(2, "DTLSContext\tDTLS factory not ready.");
      return false;
    }

    if (m_handshakeFinished)
      return true;
    
    if (isReceive && frameSize > 0 && !isDTLSPacket(framePtr, frameSize))
    {
      PTRACE(4, "DTLSContext\tSkip non DTLS packet...");
      return true;
    }

    CSHelper csHelper(m_mutex);
    if (!csHelper.Entered())
      return true;

    // Skip if we wait for data...
    if (m_waitResponse && !isReceive)
      return true;

    char errbuf[1024];

    if (!m_inBio || !m_outBio)
      return false;

    int ret;

    if (isReceive && frameSize > 0)
    {
      m_waitResponse = false;
      m_readTimer.Stop();
      ret = BIO_write(m_inBio, framePtr, frameSize);
      return true;
    }

    ret = SSL_do_handshake(m_ssl);

    errbuf[0] = 0;
    ERR_error_string_n(ERR_peek_error(), errbuf, sizeof(errbuf));

    // See what was written
    int outBioLen;
    unsigned char *outBioData;  
    outBioLen = BIO_get_mem_data(m_outBio, &outBioData);

    ret = SSL_get_error(m_ssl, ret);
    switch (ret)
    {
    case SSL_ERROR_NONE:
      m_readTimer.Stop();
      PTRACE(4, "DTLSContext\tHandshake step done..");
      m_handshakeFinished = true;
      m_waitResponse = false;
      break;
    case SSL_ERROR_WANT_READ:
      m_waitResponse = true;
      // There are two cases here:
      // (1) We didn't get enough data. In this case we leave the
      //     timers alone and wait for more packets.
      // (2) We did get a full flight and then handled it, but then
      //     wrote some more message and now we need to flush them
      //     to the network and now reset the timers
      //
      // If data was written then this means we got a complete
      // something or a retransmit so we need to reset the timer
      if (outBioLen)
        m_readTimer.SetInterval(HandshakeReadTimeout);
      break;
    default:
      PTRACE(2, "DTLSContext\t Handshake failed [" << errbuf << "]");
      return false;
    }

    if (outBioLen)
    {
      PTRACE(4, "DTLSContext\tWrite " << outBioLen << " bytes to " << m_socketData.ap.GetAddress().AsString() << ":" << m_socketData.ap.GetPort());
      if (!m_socketData.socket->WriteTo(outBioData, outBioLen, m_socketData.ap))
      {
        PTRACE(2, "DTLSContext\tCan't write to socket... " << outBioLen);
        return false;
      }
    }

    BIO_reset(m_outBio);
    BIO_reset(m_inBio);

    if (m_handshakeFinished)
    {
      if (CompleteHandshake())
      {
        if (m_callback)
          m_callback->OnDTLSReady(this);
      }
      else
        return false;
    }
    return true;
  }
  ~DTLSContext()
  {
    m_readTimer.Stop();
    if (m_ssl)
    {
      SSL_shutdown(m_ssl);
      SSL_free(m_ssl);
    }
  }
  static bool isDTLSPacket(const BYTE * framePtr, PINDEX frameSize)
  {
    return (frameSize > 0 && framePtr[0] >= 20 && framePtr[0] <= 64);
  }
  bool NeedHandshake() const
  {
    return !m_handshakeFinished;
  }
  void SetRemoteFingerprint(const PSSLCertificateFingerprint &fp)
  {
    m_remoteFingerprint = fp;
  }
  const PSSLCertificateFingerprint& GetRemoteFingerprint() const
  {
    return m_remoteFingerprint;
  }
  const OpalMediaCryptoSuite* GetCryptoSelectedSuit() const
  {
    if (GetSelectedProfile() == srtp_profile_aes128_cm_sha1_32)
      return OpalMediaCryptoSuiteFactory::CreateInstance("AES_CM_128_HMAC_SHA1_32"); 
    else if (GetSelectedProfile() == srtp_profile_aes128_cm_sha1_80)
      return OpalMediaCryptoSuiteFactory::CreateInstance("AES_CM_128_HMAC_SHA1_80"); 
    return NULL; 
  }
  srtp_profile_t GetSelectedProfile() const
  {
    return m_profile;
  }

  const uint8_t* GetKeyringMaterial() const
  {
    return m_keyringMaterial;
  }
private:
  PDECLARE_NOTIFIER(PTimer, DTLSContext, OnReadTimeout);
  struct SocketContext
  {
    PUDPSocket* socket;
    PIPSocketAddressAndPort ap;
  };

  static int VerifyCertificateCallback(int preverify_ok, X509_STORE_CTX *ctx)
  {
    SSL* sslContext = reinterpret_cast<SSL*>(X509_STORE_CTX_get_app_data(ctx));
    DTLSContext* dtlsContext = reinterpret_cast<DTLSContext*>(SSL_get_app_data(sslContext));
    if(!sslContext || !dtlsContext)
      return 0;

    if (!dtlsContext->GetRemoteFingerprint().MatchForCertificate(PSSLCertificate(ctx->cert, true)))
    {
      PTRACE(2, "DTLSContext\tInvalid remote certificate.");
      return 0;
    }

    return 1;
  }
  bool CompleteHandshake()
  {
    SRTP_PROTECTION_PROFILE *p = SSL_get_selected_srtp_profile(m_ssl);
    if (!p)
    {
      PTRACE(2, "DTLSContext\tSSL_get_selected_srtp_profile returned NULL: " << ERR_error_string(ERR_get_error(), NULL));
      return false;
    }

    m_profile = srtp_profile_aes128_cm_sha1_80;
    if (PCaselessString(p->name) == "AES_CM_128_HMAC_SHA1_32" )
      m_profile = srtp_profile_aes128_cm_sha1_32;

    if (!GetCryptoSelectedSuit())
    {
      PTRACE(2, "DTLSContext\tCryptosuite not found for selected profile: " << p->name);
      return false;
    }

    unsigned int masterSaltLength = srtp_profile_get_master_key_length(m_profile);
    unsigned int masterKeyLength = srtp_profile_get_master_salt_length(m_profile);

    if (((masterSaltLength + masterKeyLength) << 1) > KEYRING_MATERIAL_SIZE)
    {
      PTRACE(2, "DTLSContext\tInvalid key size for this profile:  " << KEYRING_MATERIAL_SIZE);
      return false;
    }

    memset(m_keyringMaterial, 0, KEYRING_MATERIAL_SIZE);

    int result = SSL_export_keying_material(m_ssl, m_keyringMaterial,
      KEYRING_MATERIAL_SIZE, EXTRACTOR_DTLS_SRTP_TEXT, EXTRACTOR_DTLS_SRTP_TEXT_LEN, NULL, 0, 0);
    
    if (result != 1)
    {
      PTRACE(2, "DTLSContext\tSSL_export_keying_material failed: " << ERR_error_string(ERR_get_error(), NULL));
      return false;
    }

    return true;
  }
private:
  bool m_handshakeFinished;
  DTLSFactory* m_factory;
  SSL* m_ssl;
  bool m_asServer;
  SocketContext m_socketData;
  PTimer m_readTimer;
  bool m_waitResponse;
  PMutex m_mutex;
  BIO* m_outBio;
  BIO* m_inBio;
  DTLSHandshakeListener* m_callback;
  bool m_useForData;
  PSSLCertificateFingerprint m_remoteFingerprint;
  OpalMediaCryptoSuite* m_cryptoSuit;
  srtp_profile_t m_profile;
  uint8_t m_keyringMaterial[KEYRING_MATERIAL_SIZE];
};

void DTLSContext::OnReadTimeout(PTimer&, int)
{
  PTRACE(2, "DTLSContext\tHandshake retransmit...");
  if (!Handshake(NULL, 0, true))
  {
    if (m_callback)
      m_callback->OnDTLSFailed();
  }
}

const PCaselessString & OpalDTLSSRTPSession::RTP_DTLS_SAVP()
{
  static const PConstCaselessString s("UDP/TLS/RTP/SAVP" );
  return s;
}

const PCaselessString & OpalDTLSSRTPSession::RTP_DTLS_SAVPF()
{
  static const PConstCaselessString s("UDP/TLS/RTP/SAVPF" );
  return s;
}

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

DTLSContext* OpalDTLSSRTPSession::GetDTLSContext(bool isData)
{
  int ctxIndex = 1;
  bool useData = (isData || m_remoteDataPort == m_remoteControlPort);
  if (useData)
    ctxIndex = 0;

  DTLSContext* result = 0;
  if (!m_contexts[ctxIndex])
  {
    PUDPSocket* socket = (useData ? m_dataSocket : m_controlSocket);
    PIPSocketAddressAndPort ap(m_remoteAddress, (useData ? m_remoteDataPort : m_remoteControlPort));
    result = new DTLSContext(socket, ap, DtlsFactory(), !GetConnectionInitiator());
    result->SetUsedForData(useData);
    result->SetCallback(this);
    result->SetRemoteFingerprint(GetRemoteFingerprint());
    m_contexts[ctxIndex].reset(result);
  }
  else
    result = m_contexts[ctxIndex].get();
  return result;
}

OpalRTPSession::SendReceiveStatus OpalDTLSSRTPSession::HandshakeIfNeeded(const BYTE * framePtr, PINDEX frameSize, bool forDataChannel, bool isReceive)
{
  DTLSContext* ctx = GetDTLSContext(forDataChannel);
  if (!m_dtlsReady && ctx && ctx->NeedHandshake())
  {
    if (ctx->Handshake(framePtr, frameSize, isReceive))
      return e_IgnorePacket;
    else 
      OnDTLSFailed();
    return e_AbortTransport;
  }
  return e_ProcessPacket;
}

void OpalDTLSSRTPSession::SetConnectionInitiator(bool connectionInitiator)
{
  m_connectionInitiator = connectionInitiator;
}

bool OpalDTLSSRTPSession::GetConnectionInitiator() const
{
  return m_connectionInitiator;
}

DTLSFactory* OpalDTLSSRTPSession::DtlsFactory()
{
  static DTLSFactory s_factory;
  return &s_factory;
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
    return m_srtcp.ProtectRTCP(frame) ? e_ProcessPacket : e_IgnorePacket;
  return result;
}

const PSSLCertificateFingerprint& OpalDTLSSRTPSession::GetLocalFingerprint() const
{
  return DtlsFactory()->GetFingerprint();
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
  bool useData = (forDataChannel || m_remoteDataPort == m_remoteControlPort);
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

void OpalDTLSSRTPSession::AppendCandidate(const PNatCandidate& aCandidate)
{
  if (aCandidate.m_localTransportAddress.GetPort() == m_remoteDataPort
    || aCandidate.m_localTransportAddress.GetPort() == m_remoteControlPort)
  {
    PTRACE(3, "DTLS-RTP\tAppend candidate " << aCandidate.m_localTransportAddress
      << " for " << (aCandidate.m_component == PNatMethod::eComponent_RTP ? "RTP" : "RTCP"));
    m_candidates[static_cast<int>(aCandidate.m_component) - 1].push_back(Candidate(aCandidate.m_localTransportAddress));
  }
}

void OpalDTLSSRTPSession::SetRemoteUserPass(const PString & user, const PString & pass)
{
  OpalMediaSession::SetRemoteUserPass(user, pass);
}

void OpalDTLSSRTPSession::OnDTLSReady(DTLSContext* ctx)
{
  unsigned int masterKeyLength = srtp_profile_get_master_key_length(ctx->GetSelectedProfile());
  unsigned int masterSaltLength = srtp_profile_get_master_salt_length(ctx->GetSelectedProfile());

  const uint8_t *localKey;
  const uint8_t *localSalt;
  const uint8_t *remoteKey;
  const uint8_t *remoteSalt;
  if (ctx->IsServer())
  {
    remoteKey = ctx->GetKeyringMaterial();
    localKey = remoteKey + masterKeyLength;
    remoteSalt = (localKey + masterKeyLength);
    localSalt = (remoteSalt + masterSaltLength);
  }
  else
  {
    localKey = ctx->GetKeyringMaterial();
    remoteKey = localKey + masterKeyLength;
    localSalt = (remoteKey + masterKeyLength); 
    remoteSalt = (localSalt + masterSaltLength);
  }

  const OpalMediaCryptoSuite* cryptoSuite = ctx->GetCryptoSelectedSuit();

  std::unique_ptr<OpalSRTPKeyInfo> keyPtr;

  keyPtr.reset(dynamic_cast<OpalSRTPKeyInfo*>(cryptoSuite->CreateKeyInfo()));
  keyPtr->SetCipherKey(PBYTEArray(remoteKey, masterKeyLength));
  keyPtr->SetAuthSalt(PBYTEArray(remoteSalt, masterSaltLength));

  if (ctx->GetUsedForData()) // Data socket context
    SetCryptoKey(keyPtr.get(), true);

  if (!ctx->GetUsedForData() || m_singlePort) // Control socket context
    m_srtcp.Open(GetSyncSourceIn(), keyPtr.get(), true);

  keyPtr.reset(dynamic_cast<OpalSRTPKeyInfo*>(cryptoSuite->CreateKeyInfo()));
  keyPtr->SetCipherKey(PBYTEArray(localKey, masterKeyLength));
  keyPtr->SetAuthSalt(PBYTEArray(localSalt, masterSaltLength));

  if (ctx->GetUsedForData()) // Data socket context
    SetCryptoKey(keyPtr.get(), false);

  if (!ctx->GetUsedForData() || m_singlePort) // Control socket context
    m_srtcp.Open(GetSyncSourceOut(), keyPtr.get(), false);

  m_dtlsReady = true;
}

OpalDTLSSRTPSession::SendReceiveStatus OpalDTLSSRTPSession::OnReceiveControl(RTP_ControlFrame & frame)
{
  return (m_dtlsReady && m_srtcp.UnprotectRTCP(frame)) ? OpalRTPSession::OnReceiveControl(frame) : e_IgnorePacket;
}

bool OpalDTLSSRTPSession::Close()
{
  m_srtcp.Close();
  return OpalSRTPSession::Close();
}

void OpalDTLSSRTPSession::OnDTLSFailed()
{
  Close();
}

#else
  #pragma message("SRTP support DISABLED")
#endif // OPAL_SRTP
