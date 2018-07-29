/*
 * srtp_session.cxx
 *
 * SRTP protocol session handler
 *
 * OPAL Library
 *
 * Copyright (C) 2012 Vox Lucida Pty. Ltd.
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
 * The Initial Developer of the Original Code is Vox Lucida
 *
 * Contributor(s): ______________________________________.
 *
 */

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "srtp_session.h"
#endif

#include <opal_config.h>

#if OPAL_SRTP

#include <opal.h>
#include <rtp/dtls_srtp_session.h>
#include <h323/h323caps.h>
#include <ptclib/cypher.h>
#include <ptclib/random.h>


#define PTraceModule() "SRTP"


static const unsigned MaxConsecutiveErrors = 100;

#if PTRACING
  #define OPAL_SRTP_TRACE(level, dir, subchan, ssrc, item, arg) PTRACE(GetThrottle(level, dir, subchan, ssrc, item), \
                *this << "SSRC=" << RTP_TRACE_SRC(ssrc) << ", " << arg << GetThrottle(level, dir, subchan, ssrc, item))

  PTrace::ThrottleBase & OpalSRTPSession::GetThrottle(unsigned level, Direction dir, SubChannels subchannel, RTP_SyncSourceId ssrc, int item)
  {
    uint64_t index = item|(dir<<3)|(subchannel<<5)|((uint64_t)ssrc<<8);
    map<uint64_t, PTrace::ThrottleBase>::iterator it = m_throttle.find(index);
    if (it == m_throttle.end())
      it = m_throttle.insert(make_pair(index, PTrace::ThrottleBase(level))).first;
    return it->second;
  }
#else
  #define OPAL_SRTP_TRACE(level, dir, subchan, ssrc, item, arg)
#endif

/////////////////////////////////////////////////////////////////////////////////////
//
//  SRTP implementation using Cisco libSRTP
//  See https://github.com/cisco/libsrtp
//
// Will use OS installed version for Linux, for DevStudio uses built in version
//

#if OPAL_SRTP==2
  #define uint32_t uint32_t
  #pragma warning(disable:4505)
  #include <srtp.h>
  #pragma comment(lib, "ws2_32.lib") // As libsrtp uses htonl etc
#elif HAS_SRTP_SRTP_H
  #include <srtp2/srtp.h>
#else
  #include <srtp.h>
#endif


///////////////////////////////////////////////////////

#if PTRACING
static bool CheckError(srtp_err_status_t err,
                       const char * fn,
                       const char * file,
                       int line,
                       const OpalMediaSession * session = NULL,
                       RTP_SyncSourceId ssrc = 0,
                       RTP_SequenceNumber sn = 0)
{
  if (err == srtp_err_status_ok)
    return true;

  static unsigned const Level = 2;
  if (!PTrace::CanTrace(Level))
    return false;

  ostream & trace = PTrace::Begin(Level, file, line, NULL, PTraceModule());
  if (session != NULL)
      trace << *session;
  trace << "Library error " << err << " from " << fn << "() - ";
  switch (err) {
    case srtp_err_status_fail :
      trace << "unspecified failure";
      break;
    case srtp_err_status_bad_param :
      trace << "unsupported parameter";
      break;
    case srtp_err_status_alloc_fail :
      trace << "couldn't allocate memory";
      break;
    case srtp_err_status_dealloc_fail :
      trace << "couldn't deallocate properly";
      break;
    case srtp_err_status_init_fail :
      trace << "couldn't initialize";
      break;
    case srtp_err_status_terminus :
      trace << "can't process as much data as requested";
      break;
    case srtp_err_status_auth_fail :
      trace << "authentication failure";
      break;
    case srtp_err_status_cipher_fail :
      trace << "cipher failure";
      break;
    case srtp_err_status_replay_fail :
      trace << "replay check failed (bad index)";
      break;
    case srtp_err_status_replay_old :
      trace << "replay check failed (index too old)";
      break;
    case srtp_err_status_algo_fail :
      trace << "algorithm failed test routine";
      break;
    case srtp_err_status_no_such_op :
      trace << "unsupported operation";
      break;
    case srtp_err_status_no_ctx :
      trace << "no appropriate context found";
      break;
    case srtp_err_status_cant_check :
      trace << "unable to perform desired validation";
      break;
    case srtp_err_status_key_expired :
      trace << "can't use key any more";
      break;
    case srtp_err_status_socket_err :
      trace << "error in use of socket";
      break;
    case srtp_err_status_signal_err :
      trace << "error in use POSIX signals";
      break;
    case srtp_err_status_nonce_bad :
      trace << "nonce check failed";
      break;
    case srtp_err_status_read_fail :
      trace << "couldn't read data";
      break;
    case srtp_err_status_write_fail :
      trace << "couldn't write data";
      break;
    case srtp_err_status_parse_err :
      trace << "error pasring data";
      break;
    case srtp_err_status_encode_err :
      trace << "error encoding data";
      break;
    case srtp_err_status_semaphore_err :
      trace << "error while using semaphores";
      break;
    case srtp_err_status_pfkey_err :
      trace << "error while using pfkey";
      break;
    case srtp_err_status_bad_mki :
      trace << "MKI present in packet is invalid";
      break;
    case srtp_err_status_pkt_idx_old :
      trace << "packet index is too old to consider";
      break;
    case srtp_err_status_pkt_idx_adv :
      trace << "packet index advanced, reset needed";
      break;
    default :
      trace << "unknown error (" << err << ')';
  }
  if (ssrc != 0)
    trace << " - SSRC=" << RTP_TRACE_SRC(ssrc);
  if (sn != 0)
    trace << " SN=" << sn;
  trace << PTrace::End;
  return false;
}

#define CHECK_ERROR(fn, param, ...) CheckError(fn param, #fn, __FILE__, __LINE__, ##__VA_ARGS__)
#else //PTRACING
#define CHECK_ERROR(fn, param, ...) ((fn param) == srtp_err_status_ok)
#endif //PTRACING

extern "C" {
  void srtp_log_handler(srtp_log_level_t PTRACE_PARAM(srtpLevel), const char * PTRACE_PARAM(msg), void *)
  {
#if PTRACING
    unsigned traceLevel;
    switch (srtpLevel) {
      case srtp_log_level_error:
         traceLevel = 1;
         break;
      case srtp_log_level_warning:
         traceLevel = 2;
         break;
      case srtp_log_level_info:
         traceLevel = 4;
         break;
      case srtp_log_level_debug:
      default:
         traceLevel = 5;
         break;
    }

    PTRACE(traceLevel, "libsrtp2: " << msg);
#endif //PTRACING
  }
};

///////////////////////////////////////////////////////

class PSRTPInitialiser : public PProcessStartup
{
  PCLASSINFO(PSRTPInitialiser, PProcessStartup)
  public:
    virtual void OnStartup()
    {
      PTRACE(2, "Initialising SRTP: " << srtp_get_version_string());
      CHECK_ERROR(srtp_install_log_handler,(srtp_log_handler, NULL));
      CHECK_ERROR(srtp_init,());
    }
};

PFACTORY_CREATE_SINGLETON(PProcessStartupFactory, PSRTPInitialiser);


///////////////////////////////////////////////////////

const PCaselessString & OpalSRTPSession::RTP_SAVP () { static const PConstCaselessString s("RTP/SAVP" ); return s; }
const PCaselessString & OpalSRTPSession::RTP_SAVPF() { static const PConstCaselessString s("RTP/SAVPF"); return s; }

PFACTORY_CREATE(OpalMediaSessionFactory, OpalSRTPSession, OpalSRTPSession::RTP_SAVP());
PFACTORY_SYNONYM(OpalMediaSessionFactory, OpalSRTPSession, SAVPF, OpalSRTPSession::RTP_SAVPF());

static PConstCaselessString AES_CM_128_HMAC_SHA1_80("AES_CM_128_HMAC_SHA1_80");

class OpalSRTPCryptoSuite_AES_CM_128_HMAC_SHA1_80 : public OpalSRTPCryptoSuite
{
    PCLASSINFO(OpalSRTPCryptoSuite_AES_CM_128_HMAC_SHA1_80, OpalSRTPCryptoSuite);
  public:
    virtual const PCaselessString & GetFactoryName() const { return AES_CM_128_HMAC_SHA1_80; }
    virtual const char * GetDescription() const { return "SRTP: AES-128 & SHA1-80"; }
#if OPAL_H235_6 || OPAL_H235_8
    virtual const char * GetOID() const { return "0.0.8.235.0.4.91"; }
#endif

    virtual void SetCryptoPolicy(struct srtp_crypto_policy_t & policy) const { srtp_crypto_policy_set_aes_cm_128_hmac_sha1_80(&policy); }
};

PFACTORY_CREATE(OpalMediaCryptoSuiteFactory, OpalSRTPCryptoSuite_AES_CM_128_HMAC_SHA1_80, AES_CM_128_HMAC_SHA1_80, true);


static PConstCaselessString AES_CM_128_HMAC_SHA1_32("AES_CM_128_HMAC_SHA1_32");

class OpalSRTPCryptoSuite_AES_CM_128_HMAC_SHA1_32 : public OpalSRTPCryptoSuite
{
    PCLASSINFO(OpalSRTPCryptoSuite_AES_CM_128_HMAC_SHA1_32, OpalSRTPCryptoSuite);
  public:
    virtual const PCaselessString & GetFactoryName() const { return AES_CM_128_HMAC_SHA1_32; }
    virtual const char * GetDescription() const { return "SRTP: AES-128 & SHA1-32"; }
#if OPAL_H235_6 || OPAL_H235_8
    virtual const char * GetOID() const { return "0.0.8.235.0.4.92"; }
#endif

    virtual void SetCryptoPolicy(struct srtp_crypto_policy_t & policy) const { srtp_crypto_policy_set_aes_cm_128_hmac_sha1_32(&policy); }
};

PFACTORY_CREATE(OpalMediaCryptoSuiteFactory, OpalSRTPCryptoSuite_AES_CM_128_HMAC_SHA1_32, AES_CM_128_HMAC_SHA1_32, true);



///////////////////////////////////////////////////////

#if OPAL_H235_8
H235SecurityCapability * OpalSRTPCryptoSuite::CreateCapability(const H323Capability & mediaCapability) const
{
  return new H235SecurityGenericCapability(mediaCapability);
}
#endif

bool OpalSRTPCryptoSuite::Supports(const PCaselessString & proto) const
{
  return proto == OPAL_PREFIX_SIP || proto == OPAL_PREFIX_H323 || proto == OPAL_PREFIX_SDP;
}


bool OpalSRTPCryptoSuite::ChangeSessionType(PCaselessString & mediaSession, KeyExchangeModes modes) const
{
  if (modes&e_InBandKeyEchange) {
    if (mediaSession == OpalRTPSession::RTP_AVP()) {
      mediaSession = OpalDTLSSRTPSession::RTP_DTLS_SAVP();
      return true;
    }

    if (mediaSession == OpalRTPSession::RTP_AVPF()) {
      mediaSession = OpalDTLSSRTPSession::RTP_DTLS_SAVPF();
      return true;
    }
  }

  if (modes&e_SecureSignalling) {
    if (mediaSession == OpalRTPSession::RTP_AVP()) {
      mediaSession = OpalSRTPSession::RTP_SAVP();
      return true;
    }

    if (mediaSession == OpalRTPSession::RTP_AVPF()) {
      mediaSession = OpalSRTPSession::RTP_SAVPF();
      return true;
    }
  }

  return false;
}


PINDEX OpalSRTPCryptoSuite::GetCipherKeyBits() const
{
  return 128;
}


PINDEX OpalSRTPCryptoSuite::GetAuthSaltBits() const
{
  return 112;
}


OpalMediaCryptoKeyInfo * OpalSRTPCryptoSuite::CreateKeyInfo() const
{
  return new OpalSRTPKeyInfo(*this);
}


///////////////////////////////////////////////////////////////////////

OpalSRTPKeyInfo::OpalSRTPKeyInfo(const OpalSRTPCryptoSuite & cryptoSuite)
  : OpalMediaCryptoKeyInfo(cryptoSuite)
  , m_cryptoSuite(cryptoSuite)
{
}


PObject * OpalSRTPKeyInfo::Clone() const
{
  OpalSRTPKeyInfo * clone = new OpalSRTPKeyInfo(*this);
  clone->m_key.MakeUnique();
  clone->m_salt.MakeUnique();
  return clone;
}


PObject::Comparison OpalSRTPKeyInfo::Compare(const PObject & obj) const
{
  const OpalSRTPKeyInfo& other = dynamic_cast<const OpalSRTPKeyInfo&>(obj);
  Comparison c = m_key.Compare(other.m_key);
  if (c == EqualTo)
    c = m_salt.Compare(other.m_salt);
  return c;
}


bool OpalSRTPKeyInfo::IsValid() const
{
  return m_key.GetSize() == m_cryptoSuite.GetCipherKeyBytes() &&
         m_salt.GetSize() == m_cryptoSuite.GetAuthSaltBytes();
}


bool OpalSRTPKeyInfo::FromString(const PString & str)
{
  PBYTEArray key_salt;
  if (!PBase64::Decode(str, key_salt)) {
    PTRACE2(2, &m_cryptoSuite, "Illegal key/salt \"" << str << '"');
    return false;
  }

  PINDEX keyBytes = m_cryptoSuite.GetCipherKeyBytes();
  PINDEX saltBytes = m_cryptoSuite.GetAuthSaltBytes();

  if (key_salt.GetSize() < keyBytes+saltBytes) {
    PTRACE2(2, &m_cryptoSuite, "Incorrect combined key/salt size"
            " (" << key_salt.GetSize() << " bytes) for " << m_cryptoSuite);
    return false;
  }

  SetCipherKey(PBYTEArray(key_salt, keyBytes));
  SetAuthSalt(PBYTEArray(key_salt+keyBytes, key_salt.GetSize()-keyBytes));

  return true;
}


PString OpalSRTPKeyInfo::ToString() const
{
  PBYTEArray key_salt(m_key.GetSize()+m_salt.GetSize());
  memcpy(key_salt.GetPointer(), m_key, m_key.GetSize());
  memcpy(key_salt.GetPointer()+m_key.GetSize(), m_salt, m_salt.GetSize());
  return PBase64::Encode(key_salt, "");
}


void OpalSRTPKeyInfo::Randomise()
{
  m_key = PRandom::Octets(m_cryptoSuite.GetCipherKeyBytes());
  m_salt = PRandom::Octets(m_cryptoSuite.GetAuthSaltBytes());
}


bool OpalSRTPKeyInfo::SetCipherKey(const PBYTEArray & key)
{
  if (key.GetSize() < m_cryptoSuite.GetCipherKeyBytes()) {
    PTRACE2(2, &m_cryptoSuite, "Incorrect key size (" << key.GetSize() << " bytes) for " << m_cryptoSuite);
    return false;
  }

  m_key = key;
  return true;
}


bool OpalSRTPKeyInfo::SetAuthSalt(const PBYTEArray & salt)
{
  if (salt.GetSize() < m_cryptoSuite.GetAuthSaltBytes()) {
    PTRACE2(2, &m_cryptoSuite, "Incorrect salt size (" << salt.GetSize() << " bytes) for " << m_cryptoSuite);
    return false;
  }

  m_salt = salt;
  return true;
}


PBYTEArray OpalSRTPKeyInfo::GetCipherKey() const
{
  return m_key;
}


PBYTEArray OpalSRTPKeyInfo::GetAuthSalt() const
{
  return m_salt;
}


///////////////////////////////////////////////////////////////////////////////

OpalSRTPSession::OpalSRTPSession(const Init & init)
  : OpalRTPSession(init)
  , m_anyRTCP_SSRC(false)
{
  CHECK_ERROR(srtp_create, (&m_context, NULL));

  for (int i = 0; i < 2; ++i) {
    m_keyInfo[i] = NULL;
    for (int j = 0; j < 2; j++)
      m_consecutiveErrors[i][j] = 0;
  }
}


OpalSRTPSession::~OpalSRTPSession()
{
  Close();

  for (int i = 0; i < 2; ++i)
    delete m_keyInfo[i];

  if (m_context != NULL)
    CHECK_ERROR(srtp_dealloc,(m_context));
}


bool OpalSRTPSession::ResequenceOutOfOrderPackets(SyncSource &) const
{
  return true;  // With SRTP, always resequence
}


OpalMediaCryptoKeyList & OpalSRTPSession::GetOfferedCryptoKeys()
{
  PSafeLockReadOnly lock(*this);

  if (m_offeredCryptokeys.IsEmpty() && m_keyInfo[e_Sender] != NULL)
    m_offeredCryptokeys.Append(m_keyInfo[e_Sender]->CloneAs<OpalMediaCryptoKeyInfo>());

  return OpalRTPSession::GetOfferedCryptoKeys();
}


bool OpalSRTPSession::ApplyCryptoKey(OpalMediaCryptoKeyList & keys, bool rx)
{
  PSafeLockReadOnly lock(*this);

  for (OpalMediaCryptoKeyList::iterator it = keys.begin(); it != keys.end(); ++it) {
    if (ApplyKeyToSRTP(*it, rx ? e_Receiver : e_Sender)) {
      keys.Select(it);
      return true;
    }
  }
  return false;
}


bool OpalSRTPSession::ApplyKeyToSRTP(const OpalMediaCryptoKeyInfo & keyInfo, Direction dir)
{
  // Aleady locked on entry

  const OpalSRTPKeyInfo * srtpKeyInfo = dynamic_cast<const OpalSRTPKeyInfo*>(&keyInfo);
  if (srtpKeyInfo == NULL) {
    PTRACE(2, *this << "unsuitable crypto suite " << keyInfo.GetCryptoSuite());
    return false;
  }

  BYTE tmp_key_salt[32];
  memset(tmp_key_salt, 0, sizeof(tmp_key_salt));
  memcpy(tmp_key_salt, keyInfo.GetCipherKey(), std::min((PINDEX)16, keyInfo.GetCipherKey().GetSize()));
  memcpy(&tmp_key_salt[16], keyInfo.GetAuthSalt(), std::min((PINDEX)14, keyInfo.GetAuthSalt().GetSize()));

  if (m_keyInfo[dir] != NULL) {
    if (memcmp(tmp_key_salt, m_keyInfo[dir]->m_key_salt, 32) == 0) {
      PTRACE(3, *this << "crypto key for " << dir << " already set.");
      return true;
    }
    PTRACE(3, *this << "changing crypto keys from \"" << m_keyInfo[dir]->GetCryptoSuite() << "\""
                       " to \"" << keyInfo.GetCryptoSuite() << "\" for " << dir);
    delete m_keyInfo[dir];

    for (SyncSourceMap::iterator it = m_SSRC.begin(); it != m_SSRC.end(); ++it) {
      if (it->second->m_direction == dir) {
        RTP_SyncSourceId ssrc = it->first;
        if (m_addedStream.erase(ssrc) > 0) {
          srtp_remove_stream(m_context, ssrc);
          PTRACE(4, *this << "removed " << dir << " SRTP stream for SSRC=" << RTP_TRACE_SRC(ssrc));
        }
      }
    }
  }
  else {
    PTRACE(3, *this << "setting crypto keys (" << keyInfo.GetCryptoSuite() << ") for " << dir);
  }

  m_keyInfo[dir] = new OpalSRTPKeyInfo(*srtpKeyInfo);
  memcpy(m_keyInfo[dir]->m_key_salt, tmp_key_salt, 32);

  for (SyncSourceMap::iterator it = m_SSRC.begin(); it != m_SSRC.end(); ++it) {
    if (it->second->m_direction == dir && !AddStreamToSRTP(it->first, dir))
      return false;
  }

  return true;
}


OpalMediaCryptoKeyInfo * OpalSRTPSession::IsCryptoSecured(bool rx) const
{
  return m_keyInfo[rx ? e_Receiver : e_Sender];
}


bool OpalSRTPSession::Open(const PString & localInterface, const OpalTransportAddress & remoteAddress)
{
  m_anyRTCP_SSRC = GetSyncSourceIn() == 0 && m_stringOptions.GetBoolean(OPAL_OPT_SRTP_RTCP_ANY_SSRC, m_anyRTCP_SSRC);

  return OpalRTPSession::Open(localInterface, remoteAddress);
}


RTP_SyncSourceId OpalSRTPSession::AddSyncSource(RTP_SyncSourceId ssrc, Direction dir, const char * cname)
{
  PSafeLockReadWrite lock(*this);
  if (!lock.IsLocked())
    return 0;

  if ((ssrc = OpalRTPSession::AddSyncSource(ssrc, dir, cname)) == 0)
    return 0;

  if (m_keyInfo[dir] == NULL) {
    PTRACE(3, *this << "could not add SRTP stream for SSRC=" << RTP_TRACE_SRC(ssrc) << ", no " << dir << " key (yet)");
    return ssrc;
  }

  if (AddStreamToSRTP(ssrc, dir))
    return ssrc;

  RemoveSyncSource(ssrc PTRACE_PARAM(, "Could not add SRTP stream"));
  return 0;
}


bool OpalSRTPSession::AddStreamToSRTP(RTP_SyncSourceId ssrc, Direction dir)
{
  // Aleady locked on entry

  if (m_addedStream.find(ssrc) != m_addedStream.end()) {
    PTRACE(4, *this << "already have " << dir << " SRTP stream for SSRC=" << RTP_TRACE_SRC(ssrc));
    return true;
  }

  // Get policy, create blank one if needed
  srtp_policy_t policy;
  memset(&policy, 0, sizeof(policy));

  policy.ssrc.type = ssrc_specific;
  policy.ssrc.value = ssrc;

  const OpalSRTPCryptoSuite & cryptoSuite = m_keyInfo[dir]->GetCryptoSuite();
  cryptoSuite.SetCryptoPolicy(policy.rtp);
  cryptoSuite.SetCryptoPolicy(policy.rtcp);

  policy.key = m_keyInfo[dir]->m_key_salt;

  if (!CHECK_ERROR(srtp_add_stream, (m_context, &policy), this, ssrc))
    return false;

  PTRACE(4, *this << "added " << dir << " SRTP stream for SSRC=" << RTP_TRACE_SRC(ssrc));
  m_addedStream.insert(ssrc);
  return true;
}


bool OpalSRTPSession::ApplyKeysToSRTP(OpalMediaTransport & transport)
{
  if (IsCryptoSecured(e_Sender) && IsCryptoSecured(e_Receiver))
    return true;

  OpalMediaCryptoKeyInfo * keyInfo[2];
  if (!transport.GetKeyInfo(keyInfo))
    return false;

  for (PINDEX i = 0; i < 2; ++i) {
    if (!ApplyKeyToSRTP(*keyInfo[i], (Direction)i))
      return false;
  }
  return true;
}


void OpalSRTPSession::OnRxDataPacket(OpalMediaTransport & transport, PBYTEArray data)
{
  ApplyKeysToSRTP(transport);
  OpalRTPSession::OnRxDataPacket(transport, data);
}


void OpalSRTPSession::OnRxControlPacket(OpalMediaTransport & transport, PBYTEArray data)
{
  ApplyKeysToSRTP(transport);
  OpalRTPSession::OnRxControlPacket(transport, data);
}


OpalRTPSession::SendReceiveStatus OpalSRTPSession::OnSendData(RewriteMode & rewrite, RTP_DataFrame & frame, const PTime & now)
{
  // Aleady locked on entry

  SendReceiveStatus status = OpalRTPSession::OnSendData(rewrite, frame, now);
  if (status != e_ProcessPacket)
    return status;

  if (rewrite == e_RewriteNothing)
    return e_ProcessPacket;

  PTRACE_PARAM(RTP_SyncSourceId ssrc = frame.GetSyncSource());
  if (!IsCryptoSecured(e_Sender)) {
    OPAL_SRTP_TRACE(2, e_Sender, e_Data, ssrc, 1, "keys not set, cannot protect data");
    return e_IgnorePacket;
  }

  int len = frame.GetPacketSize();

  frame.MakeUnique();
  frame.SetMinSize(len + SRTP_MAX_TRAILER_LEN);

  status = CheckConsecutiveErrors(
              CHECK_ERROR(
                  srtp_protect, (m_context, frame.GetPointer(), &len),
                  this, ssrc, frame.GetSequenceNumber()
              ),
              e_Sender, e_Data);
  if (status != e_ProcessPacket)
    return status;

  OPAL_SRTP_TRACE(3, e_Sender, e_Data, ssrc, 2, "protected RTP packet: " << frame.GetPacketSize() << "->" << len);

  frame.SetPayloadSize(len - frame.GetHeaderSize());

  return e_ProcessPacket;
}


OpalRTPSession::SendReceiveStatus OpalSRTPSession::OnSendControl(RTP_ControlFrame & frame, const PTime & now)
{
  // Aleady locked on entry

  SendReceiveStatus status = OpalRTPSession::OnSendControl(frame, now);
  if (status != e_ProcessPacket)
    return status;

  PTRACE_PARAM(RTP_SyncSourceId ssrc = frame.GetSenderSyncSource());
  if (!IsCryptoSecured(e_Sender)) {
    OPAL_SRTP_TRACE(2, e_Sender, e_Control, ssrc, 1, "keys not set, cannot protect control");
    return e_IgnorePacket;
  }

  int len = frame.GetPacketSize();

  frame.MakeUnique();
  frame.SetMinSize(len + SRTP_MAX_TRAILER_LEN);

  status = CheckConsecutiveErrors(
              CHECK_ERROR(
                  srtp_protect_rtcp, (m_context, frame.GetPointer(), &len),
                  this, ssrc
              ),
              e_Sender, e_Control);
  if (status != e_ProcessPacket)
    return status;

  OPAL_SRTP_TRACE(3, e_Sender, e_Control, ssrc, 2, "protected RTCP packet: " << frame.GetPacketSize() << "->" << len);

  frame.SetPacketSize(len);

  return OpalRTPSession::e_ProcessPacket;
}


OpalRTPSession::SendReceiveStatus OpalSRTPSession::OnReceiveData(RTP_DataFrame & frame, ReceiveType rxType, const PTime & now)
{
  // Aleady locked on entry

  if (rxType == e_RxFromRTX)
    return OpalRTPSession::OnReceiveData(frame, rxType, now);

  RTP_SyncSourceId ssrc = frame.GetSyncSource();
  if (!IsCryptoSecured(e_Receiver)) {
    OPAL_SRTP_TRACE(2, e_Receiver, e_Data, ssrc, 1, "keys not set, cannot protect data");
    return e_IgnorePacket;
  }

  /* Need to have a receiver SSRC (their sender) or we can't decrypt the RTCP
     packet. Applies only if remote did not tell us about SSRC's via signalling
     (e.g. SDP) and we just acceting anything, if it did tell us valid SSRC's
     the this fails and we don't use the RTP data as it may be some spoofing. */
  if (UseSyncSource(ssrc, e_Receiver, false) == NULL)
    return e_IgnorePacket;

  int len = frame.GetPacketSize();

  frame.MakeUnique();

  SendReceiveStatus status = CheckConsecutiveErrors(
                                CHECK_ERROR(
                                    srtp_unprotect, (m_context, frame.GetPointer(), &len),
                                    this, ssrc, frame.GetSequenceNumber()
                                ),
                                e_Receiver, e_Data);
  if (status != e_ProcessPacket)
    return status;

  OPAL_SRTP_TRACE(3, e_Receiver, e_Data, ssrc, 2, "unprotected RTP packet: " << frame.GetPacketSize() << "->" << len);

  frame.SetPayloadSize(len - frame.GetHeaderSize());

  return OpalRTPSession::OnReceiveData(frame, rxType, now);
}


OpalRTPSession::SendReceiveStatus OpalSRTPSession::OnReceiveControl(RTP_ControlFrame & encoded, const PTime & now)
{
  /* Aleady locked on entry */

  RTP_SyncSourceId ssrc = encoded.GetSenderSyncSource();
  if (!IsCryptoSecured(e_Receiver)) {
    OPAL_SRTP_TRACE(2, e_Receiver, e_Control, ssrc, 1, "keys not set, cannot protect control");
    return e_IgnorePacket;
  }

  /* Need to have a receiver SSRC (their sender) or we can't decrypt the RTCP
     packet. Generally, the SSRC info is created on the fly (m_anyRTCP_SSRC==true),
     unless we are using later SDP where that is disabled (m_anyRTCP_SSRC==false).
     However, for Chrome, we have a special cases of SSRC=1 for video and 0xfa17fa17
     for audio, neither of which they indicate in the SDP when in recvonly mode. So,
     we force creation of those specifically. */
  if (UseSyncSource(ssrc, e_Receiver, m_anyRTCP_SSRC || ssrc == 1 || ssrc == 0xfa17fa17) == NULL) {
    OPAL_SRTP_TRACE(2, e_Receiver, e_Control, ssrc, 3, "not automatically added");
    return e_IgnorePacket;
  }

  m_anyRTCP_SSRC = false;

  RTP_ControlFrame decoded(encoded);
  decoded.MakeUnique();

  int len = decoded.GetSize();

  SendReceiveStatus status = CheckConsecutiveErrors(
                                CHECK_ERROR(
                                    srtp_unprotect_rtcp, (m_context, decoded.GetPointer(), &len),
                                    this, ssrc
                                ),
                                e_Receiver, e_Control);
  if (status != e_ProcessPacket)
    return status;


  OPAL_SRTP_TRACE(3, e_Receiver, e_Control, ssrc, 2, "unprotected RTCP packet: " << decoded.GetPacketSize() << "->" << len);

  decoded.SetPacketSize(len);

  return OnReceiveDecodedControl(decoded, now);
}


OpalRTPSession::SendReceiveStatus OpalSRTPSession::OnReceiveDecodedControl(RTP_ControlFrame & frame, const PTime & now)
{
  return OpalRTPSession::OnReceiveControl(frame, now);
}


OpalRTPSession::SendReceiveStatus OpalSRTPSession::CheckConsecutiveErrors(bool ok, Direction dir, SubChannels subchannel)
{
    if (ok) {
      PTRACE_IF(3, m_consecutiveErrors[dir][subchannel] > 0, *this << "reset consecutive errors on " << dir << ' ' << subchannel);
      m_consecutiveErrors[dir][subchannel] = 0;
      return e_ProcessPacket;
    }

    if (++m_consecutiveErrors[dir][subchannel] < MaxConsecutiveErrors)
      return e_IgnorePacket;

    PTRACE(2, *this << "exceeded maximum consecutive errors, aborting " << dir << ' ' << subchannel);
    return e_AbortTransport;
}


#endif // OPAL_SRTP
