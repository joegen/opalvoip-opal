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

#include <rtp/dtls_srtp_session.h>
#include <h323/h323caps.h>
#include <ptclib/cypher.h>
#include <ptclib/random.h>


#define PTraceModule() "SRTP"


static const unsigned MaxConsecutiveErrors = 100;


/////////////////////////////////////////////////////////////////////////////////////
//
//  SRTP implementation using Cisco libSRTP
//  See http://srtp.sourceforge.net/srtp.html
//
// Will use OS installed version for Linux, for DevStudio uses built in version
//

#if OPAL_SRTP==2
  #define uint32_t uint32_t
  #pragma warning(disable:4505)
  #include <srtp.h>
  #pragma comment(lib, "ws2_32.lib") // As libsrtp uses htonl etc
#elif HAS_SRTP_SRTP_H
  #include <srtp/srtp.h>
#else
  #include <srtp.h>
#endif


///////////////////////////////////////////////////////

#if PTRACING
static bool CheckError(err_status_t err,
                       const char * fn,
                       const char * file,
                       int line,
                       const OpalMediaSession * session = NULL,
                       RTP_SyncSourceId ssrc = 0,
                       RTP_SequenceNumber sn = 0)
{
  if (err == err_status_ok)
    return true;

  ostream & trace = PTrace::Begin(2, file, line, NULL, PTraceModule());
  if (session != NULL)
      trace << *session;
  trace << "Library error " << err << " from " << fn << "() - ";
  switch (err) {
    case err_status_fail :
      trace << "unspecified failure";
      break;
    case err_status_bad_param :
      trace << "unsupported parameter";
      break;
    case err_status_alloc_fail :
      trace << "couldn't allocate memory";
      break;
    case err_status_dealloc_fail :
      trace << "couldn't deallocate properly";
      break;
    case err_status_init_fail :
      trace << "couldn't initialize";
      break;
    case err_status_terminus :
      trace << "can't process as much data as requested";
      break;
    case err_status_auth_fail :
      trace << "authentication failure";
      break;
    case err_status_cipher_fail :
      trace << "cipher failure";
      break;
    case err_status_replay_fail :
      trace << "replay check failed (bad index)";
      break;
    case err_status_replay_old :
      trace << "replay check failed (index too old)";
      break;
    case err_status_algo_fail :
      trace << "algorithm failed test routine";
      break;
    case err_status_no_such_op :
      trace << "unsupported operation";
      break;
    case err_status_no_ctx :
      trace << "no appropriate context found";
      break;
    case err_status_cant_check :
      trace << "unable to perform desired validation";
      break;
    case err_status_key_expired :
      trace << "can't use key any more";
      break;
    case err_status_socket_err :
      trace << "error in use of socket";
      break;
    case err_status_signal_err :
      trace << "error in use POSIX signals";
      break;
    case err_status_nonce_bad :
      trace << "nonce check failed";
      break;
    case err_status_read_fail :
      trace << "couldn't read data";
      break;
    case err_status_write_fail :
      trace << "couldn't write data";
      break;
    case err_status_parse_err :
      trace << "error pasring data";
      break;
    case err_status_encode_err :
      trace << "error encoding data";
      break;
    case err_status_semaphore_err :
      trace << "error while using semaphores";
      break;
    case err_status_pfkey_err :
      trace << "error while using pfkey";
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
#define CHECK_ERROR(fn, param, ...) ((fn param) == err_status_ok)
#endif //PTRACING

#ifndef OPAL_SRTP_ERROR_CONST
  #define OPAL_SRTP_ERROR_CONST
#endif

extern "C" {
  err_reporting_level_t err_level = err_level_none;

  err_status_t err_reporting_init(OPAL_SRTP_ERROR_CONST char *)
  {
    return err_status_ok;
  }

  void err_report(int PTRACE_PARAM(priority), OPAL_SRTP_ERROR_CONST char * PTRACE_PARAM(format), ...)
  {
#if PTRACING
    va_list args;
    va_start(args, format);

    unsigned level;
    switch (priority) {
      case err_level_emergency:
      case err_level_alert:
      case err_level_critical:
         level = 0;
         break;
      case err_level_error:
         level = 1;
         break;
      case err_level_warning:
         level = 2;
         break;
      case err_level_notice:
         level = 3;
         break;
      case err_level_info:
         level = 4;
         break;
      case err_level_debug:
      case err_level_none:
      default:
         level = 5;
         break;
    }

    PTRACE(level, "libsrtp: " << pvsprintf(format, args));
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

    virtual void SetCryptoPolicy(struct crypto_policy_t & policy) const { crypto_policy_set_aes_cm_128_hmac_sha1_80(&policy); }
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

    virtual void SetCryptoPolicy(struct crypto_policy_t & policy) const { crypto_policy_set_aes_cm_128_hmac_sha1_32(&policy); }
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
  return proto == "sip" || proto == "h323";
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
  for (int i = 0; i < 2; ++i) {
    for (int j = 0; j < 2; j++)
      m_consecutiveErrors[i][j] = 0;
  }

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

  m_SSRC.erase(m_SSRC.find(ssrc));
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


OpalRTPSession::SendReceiveStatus OpalSRTPSession::OnSendData(RTP_DataFrame & frame, RewriteMode rewrite)
{
  // Aleady locked on entry

  SendReceiveStatus status = OpalRTPSession::OnSendData(frame, rewrite);
  if (status != e_ProcessPacket)
    return status;

  if (rewrite == e_RewriteNothing)
    return e_ProcessPacket;

  if (!IsCryptoSecured(e_Sender)) {
    PTRACE(GetThrottle(e_Sender, e_Data, frame.GetSyncSource()),
           *this << "keys not set, cannot protect data"
           << GetThrottle(e_Sender, e_Data, frame.GetSyncSource()));
    return e_IgnorePacket;
  }

  int len = frame.GetPacketSize();

  frame.MakeUnique();
  frame.SetMinSize(len + SRTP_MAX_TRAILER_LEN);

  status = CheckConsecutiveErrors(
              CHECK_ERROR(
                  srtp_protect, (m_context, frame.GetPointer(), &len),
                  this, frame.GetSyncSource(), frame.GetSequenceNumber()
              ),
              e_Sender, e_Data);
  if (status != e_ProcessPacket)
    return status;

  PTRACE(GetThrottle(e_Sender, e_Data, frame.GetSyncSource()),
         *this << "protected RTP packet: " << frame.GetPacketSize()
         << "->" << len << " SSRC=" << frame.GetSyncSource()
         << GetThrottle(e_Sender, e_Data, frame.GetSyncSource()));

  frame.SetPayloadSize(len - frame.GetHeaderSize());

  return e_ProcessPacket;
}


OpalRTPSession::SendReceiveStatus OpalSRTPSession::OnSendControl(RTP_ControlFrame & frame)
{
  // Aleady locked on entry

  SendReceiveStatus status = OpalRTPSession::OnSendControl(frame);
  if (status != e_ProcessPacket)
    return status;

  if (!IsCryptoSecured(e_Sender)) {
    PTRACE(GetThrottle(e_Sender, e_Control, frame.GetSenderSyncSource()),
           *this << "keys not set, cannot protect control"
           << GetThrottle(e_Sender, e_Control, frame.GetSenderSyncSource()));
    return OpalRTPSession::e_IgnorePacket;
  }

  int len = frame.GetPacketSize();

  frame.MakeUnique();
  frame.SetMinSize(len + SRTP_MAX_TRAILER_LEN);

  status = CheckConsecutiveErrors(
              CHECK_ERROR(
                  srtp_protect_rtcp, (m_context, frame.GetPointer(), &len),
                  this, frame.GetSenderSyncSource()
              ),
              e_Sender, e_Control);
  if (status != e_ProcessPacket)
    return status;

  PTRACE(GetThrottle(e_Sender, e_Control, frame.GetSenderSyncSource()),
         *this << "protected RTCP packet: " << frame.GetPacketSize()
         << "->" << len << " SSRC=" << frame.GetSenderSyncSource()
         << GetThrottle(e_Sender, e_Control, frame.GetSenderSyncSource()));

  frame.SetPacketSize(len);

  return OpalRTPSession::e_ProcessPacket;
}


OpalRTPSession::SendReceiveStatus OpalSRTPSession::OnReceiveData(RTP_DataFrame & frame)
{
  // Aleady locked on entry

  if (!IsCryptoSecured(e_Receiver)) {
    PTRACE(GetThrottle(e_Receiver, e_Data, frame.GetSyncSource()),
           *this << "keys not set, cannot protect data"
           << GetThrottle(e_Receiver, e_Data, frame.GetSyncSource()));
    return OpalRTPSession::e_IgnorePacket;
  }

  /* Need to have a receiver SSRC (their sender) even if we have never been
      told about it, or we can't decrypt the RTCP packet. */
  RTP_SyncSourceId ssrc = frame.GetSyncSource();
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

  PTRACE(GetThrottle(e_Receiver, e_Data, ssrc),
         *this << "unprotected RTP packet: " << frame.GetPacketSize()
         << "->" << len << " SSRC=" << ssrc
         << GetThrottle(e_Receiver, e_Data, ssrc));

  frame.SetPayloadSize(len - frame.GetHeaderSize());

  return OpalRTPSession::OnReceiveData(frame);
}


OpalRTPSession::SendReceiveStatus OpalSRTPSession::OnReceiveControl(RTP_ControlFrame & encoded)
{
  /* Aleady locked on entry */

  if (!IsCryptoSecured(e_Receiver)) {
    PTRACE(GetThrottle(e_Receiver, e_Control, encoded.GetSenderSyncSource()),
           *this << "keys not set, cannot protect control: "
           << GetThrottle(e_Receiver, e_Control, encoded.GetSenderSyncSource()));
    return OpalRTPSession::e_IgnorePacket;
  }

  RTP_SyncSourceId ssrc = encoded.GetSenderSyncSource();
  bool force = false;

  /* Need to have a receiver SSRC (their sender) or we can't decrypt the RTCP
     packet. Generally, the SSRC info created on the fly, unless we are using
     later SDP and that is disabled. This case is handed within useSyncSource.
     However, for Chrome, we have a special cases of SSRC=1 for video and a
     random number for audio, neither of which they indicate in the SDP when
     in recvonly mode. So, we try and compensate. */
  if (m_anyRTCP_SSRC) {
#if OPAL_VIDEO
    if ((ssrc == 1) != (GetMediaType() == OpalMediaType::Video())) {
      PTRACE(4, *this << "not automatically adding SSRC=" << RTP_TRACE_SRC(ssrc) << " for " << GetMediaType());
      return e_IgnorePacket;
    }
#endif

    m_anyRTCP_SSRC = false;
    force = true;
  }

  if (UseSyncSource(ssrc, e_Receiver, force) == NULL)
    return e_IgnorePacket;

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


  PTRACE(GetThrottle(e_Receiver, e_Control, ssrc),
         *this << "unprotected RTCP packet: " << decoded.GetPacketSize()
         << "->" << len << " SSRC=" << ssrc
         << GetThrottle(e_Receiver, e_Control, ssrc));

  decoded.SetPacketSize(len);

  return OnReceiveDecodedControl(decoded);
}


OpalRTPSession::SendReceiveStatus OpalSRTPSession::OnReceiveDecodedControl(RTP_ControlFrame & frame)
{
  return OpalRTPSession::OnReceiveControl(frame);
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
