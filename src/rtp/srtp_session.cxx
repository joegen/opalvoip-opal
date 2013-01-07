/*
 * srtp.cxx
 *
 * SRTP protocol handler
 *
 * OPAL Library
 *
 * Copyright (C) 2006 Post Increment
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
 * The Initial Developer of the Original Code is Post Increment
 *     Portions of this code were written with the assistance of funding from
 *     US Joint Forces Command Joint Concept Development & Experimentation (J9)
 *     http://www.jfcom.mil/about/abt_j9.htm
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "srtp_session.h"
#endif

#include <opal/buildopts.h>

#if OPAL_SRTP

#include <rtp/srtp_session.h>
#include <ptclib/cypher.h>


class PNatMethod;


// default key = 2687012454


/////////////////////////////////////////////////////////////////////////////////////
//
//  SRTP implementation using Cisco libSRTP
//  See http://srtp.sourceforge.net/srtp.html
//
// Will use OS installed version for Linux, for DevStudio uses built in version
//

#ifdef _MSC_VER
#define uint32_t uint32_t
#pragma warning(disable:4505)
#include <srtp.h>
#pragma comment(lib, "ws2_32.lib") // As libsrtp uses htonl etc
#else
#include <srtp/srtp.h>
#endif


struct OpalLibSRTP::Context
{
  Context();
  ~Context();

  bool Open(DWORD ssrc, OpalMediaCryptoKeyList & keys);
  bool Change(DWORD from_ssrc, DWORD to_ssrc);
  void Close();
  bool ProtectRTP(RTP_DataFrame & frame);
  bool ProtectRTCP(RTP_ControlFrame & frame);
  bool UnprotectRTP(RTP_DataFrame & frame);
  bool UnprotectRTCP(RTP_ControlFrame & frame);

  OpalSRTPKeyInfo * m_keyInfo;
  struct srtp_ctx_t * m_ctx;
  BYTE m_key_salt[32]; // libsrtp vague on size, looking at source code, says 32 bytes
#if PTRACING
  bool m_firstRTP;
  bool m_firstRTCP;
#endif
  std::map<DWORD, srtp_policy_t> m_policies;
};

///////////////////////////////////////////////////////

#if PTRACING
static bool CheckError(err_status_t err, const char * fn, const char * file, int line)
{
  if (err == err_status_ok)
    return true;

  ostream & trace = PTrace::Begin(2, file, line);
  trace << "SRTP\tLibrary error " << err << " from " << fn << "() - ";
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
  trace << PTrace::End;
  return false;
}

#define CHECK_ERROR(fn, param) CheckError(fn param, #fn, __FILE__, __LINE__)
#else //PTRACING
#define CHECK_ERROR(fn, param) ((fn param) == err_status_ok)
#endif //PTRACING

extern "C" {
  err_reporting_level_t err_level = err_level_none;

  err_status_t err_reporting_init(char *)
  {
    return err_status_ok;
  }

  void err_report(int PTRACE_PARAM(priority), char * PTRACE_PARAM(format), ...)
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

    PTRACE(level, "SRTP\t" << pvsprintf(format, args));
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
static bool RegisteredSAVPF = OpalMediaSessionFactory::RegisterAs(OpalSRTPSession::RTP_SAVPF(), OpalSRTPSession::RTP_SAVP());

static PConstCaselessString AES_CM_128_HMAC_SHA1_80("AES_CM_128_HMAC_SHA1_80");

class OpalSRTPCryptoSuite_AES_CM_128_HMAC_SHA1_80 : public OpalSRTPCryptoSuite
{
    PCLASSINFO(OpalSRTPCryptoSuite, OpalMediaCryptoSuite);
  public:
    virtual const PCaselessString & GetFactoryName() const { return AES_CM_128_HMAC_SHA1_80; }
    virtual const char * GetDescription() const { return "SRTP: AES-128 & SHA1-80"; }
    virtual PINDEX GetCipherKeyBits() const { return 128; }
    virtual PINDEX GetAuthSaltBits() const { return 112; }

    virtual void SetCryptoPolicy(struct crypto_policy_t & policy) const { crypto_policy_set_aes_cm_128_hmac_sha1_80(&policy); }
};

PFACTORY_CREATE(OpalMediaCryptoSuiteFactory, OpalSRTPCryptoSuite_AES_CM_128_HMAC_SHA1_80, AES_CM_128_HMAC_SHA1_80, true);


static PConstCaselessString AES_CM_128_HMAC_SHA1_32("AES_CM_128_HMAC_SHA1_32");

class OpalSRTPCryptoSuite_AES_CM_128_HMAC_SHA1_32 : public OpalSRTPCryptoSuite
{
    PCLASSINFO(OpalSRTPCryptoSuite, OpalMediaCryptoSuite);
  public:
    virtual const PCaselessString & GetFactoryName() const { return AES_CM_128_HMAC_SHA1_32; }
    virtual const char * GetDescription() const { return "SRTP: AES-128 & SHA1-32"; }
    virtual PINDEX GetCipherKeyBits() const { return 128; }
    virtual PINDEX GetAuthSaltBits() const { return 32; }

    virtual void SetCryptoPolicy(struct crypto_policy_t & policy) const { crypto_policy_set_aes_cm_128_hmac_sha1_32(&policy); }
};

PFACTORY_CREATE(OpalMediaCryptoSuiteFactory, OpalSRTPCryptoSuite_AES_CM_128_HMAC_SHA1_32, AES_CM_128_HMAC_SHA1_32, true);



///////////////////////////////////////////////////////

bool OpalSRTPCryptoSuite::Supports(const PCaselessString & proto) const
{
  return proto *= "sip"; 
}


bool OpalSRTPCryptoSuite::ChangeSessionType(PCaselessString & mediaSession) const
{
  if (mediaSession == OpalRTPSession::RTP_AVP()) {
    mediaSession = OpalSRTPSession::RTP_SAVP();
    return true;
  }

  if (mediaSession == OpalRTPSession::RTP_AVPF()) {
    mediaSession = OpalSRTPSession::RTP_SAVPF();
    return true;
  }

  return false;
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


bool OpalSRTPKeyInfo::IsValid() const
{
  return m_key.GetSize() == (m_cryptoSuite.GetCipherKeyBits()+7)/8 &&
         m_salt.GetSize() == (m_cryptoSuite.GetAuthSaltBits()+7)/8;
}


bool OpalSRTPKeyInfo::FromString(const PString & str)
{
  PBYTEArray key_salt;
  if (!PBase64::Decode(str, key_salt)) {
    PTRACE2(2, &m_cryptoSuite, "SDP\tIllegal key/salt \"" << str << '"');
    return false;
  }

  PINDEX keyBytes = (m_cryptoSuite.GetCipherKeyBits()+7)/8;
  PINDEX saltBytes = (m_cryptoSuite.GetAuthSaltBits()+7)/8;

  if (key_salt.GetSize() < keyBytes+saltBytes) {
    PTRACE2(2, &m_cryptoSuite, "Crypto\tIncorrect combined key/salt size (" << key_salt.GetSize()
            << " bytes) for " << m_cryptoSuite.GetDescription());
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
  m_key.SetSize((m_cryptoSuite.GetCipherKeyBits()+7)/8);
  rand_source_get_octet_string(m_key.GetPointer(), m_key.GetSize());

  m_salt.SetSize((m_cryptoSuite.GetAuthSaltBits()+7)/8);
  rand_source_get_octet_string(m_salt.GetPointer(), m_salt.GetSize());
}


bool OpalSRTPKeyInfo::SetCipherKey(const PBYTEArray & key)
{
  if (key.GetSize() < (m_cryptoSuite.GetCipherKeyBits()+7)/8) {
    PTRACE2(2, &m_cryptoSuite, "Crypto\tIncorrect key size (" << key.GetSize() << " bytes) for " << m_cryptoSuite.GetDescription());
    return false;
  }

  m_key = key;
  return true;
}


bool OpalSRTPKeyInfo::SetAuthSalt(const PBYTEArray & salt)
{
  if (salt.GetSize() < (m_cryptoSuite.GetAuthSaltBits()+7)/8) {
    PTRACE2(2, &m_cryptoSuite, "Crypto\tIncorrect salt size (" << salt.GetSize() << " bytes) for " << m_cryptoSuite.GetDescription());
    return false;
  }

  m_salt = salt;
  return true;
}



///////////////////////////////////////////////////////

OpalLibSRTP::OpalLibSRTP()
  : m_rx(new Context)
  , m_tx(new Context)
{
}


OpalLibSRTP::~OpalLibSRTP()
{
  delete m_rx;
  delete m_tx;
}


bool OpalLibSRTP::Context::Open(DWORD ssrc, OpalMediaCryptoKeyList & keys)
{
  for (OpalMediaCryptoKeyList::iterator it = keys.begin(); it != keys.end(); ++it) {
    OpalSRTPKeyInfo * keyInfo = dynamic_cast<OpalSRTPKeyInfo *>(&*it);
    if (keyInfo == NULL) {
      PTRACE(2, "SRTP\tUnsuitable crypto suite " << it->GetCryptoSuite().GetDescription());
      continue;
    }

    BYTE tmp_key_salt[32];
    // This is all a bit vague in docs for libSRTP. Had to look into source to figure it out.
    memcpy(tmp_key_salt, keyInfo->GetCipherKey(), std::min((PINDEX)16, keyInfo->GetCipherKey().GetSize()));
    memcpy(&tmp_key_salt[16], keyInfo->GetAuthSalt(), std::min((PINDEX)14, keyInfo->GetAuthSalt().GetSize()));

    if (memcmp(tmp_key_salt, m_key_salt, 32) == 0){
      if (m_policies[ssrc].ssrc.type != ssrc_undefined){
        PTRACE(2, "SRTP\tPolicy for ssrc " << ssrc << " already in this context");
        return true;
      }
    }
    else {
      PTRACE(3, "SRTP\tDifferent keys in this context.");
      CHECK_ERROR(srtp_dealloc,(m_ctx));
      CHECK_ERROR(srtp_create,(&m_ctx, NULL));
    }

    srtp_policy_t policy;
    memset(&policy, 0, sizeof(policy));

    policy.ssrc.value = ssrc;
    policy.ssrc.type = ssrc != 0 ? ssrc_specific : ssrc_any_inbound;

    const OpalSRTPCryptoSuite & cryptoSuite = keyInfo->GetCryptoSuite();
    cryptoSuite.SetCryptoPolicy(policy.rtp);
    cryptoSuite.SetCryptoPolicy(policy.rtcp);

    memcpy(m_key_salt, tmp_key_salt, 32);
    policy.key = m_key_salt;

    if (CHECK_ERROR(srtp_add_stream,(m_ctx, &policy))) {
      m_policies[ssrc] = policy;
      keys.DisallowDeleteObjects();
      keys.erase(it);
      keys.AllowDeleteObjects();
      keys.RemoveAll();
      keys.Append(keyInfo);
      m_keyInfo = new OpalSRTPKeyInfo(*keyInfo);
      return true;
    }

  }

  return false;
}


bool OpalLibSRTP::Context::Change(DWORD from_ssrc, DWORD to_ssrc)
{
  srtp_policy_t policy = m_policies[from_ssrc];
  if (policy.ssrc.type == ssrc_specific) {
    policy.ssrc.value = to_ssrc;
    if( !CHECK_ERROR(srtp_add_stream, (m_ctx, &policy)) ||
        !CHECK_ERROR(srtp_remove_stream, (m_ctx, from_ssrc)))
    {
      PTRACE(2, "SRTP\tUnable to change ssrc from " << from_ssrc << " to " << to_ssrc);
    }
  }

  return true;
}


OpalLibSRTP::Context::Context()
  : m_keyInfo(NULL)
#if PTRACING
  , m_firstRTP(true)
  , m_firstRTCP(true)
#endif
{
  CHECK_ERROR(srtp_create,(&m_ctx, NULL));
  memset(m_key_salt, 0, sizeof(m_key_salt));
}


OpalLibSRTP::Context::~Context()
{
  Close();
  delete m_keyInfo;
}


void OpalLibSRTP::Context::Close()
{
  if (m_ctx != NULL) {
    CHECK_ERROR(srtp_dealloc,(m_ctx));
    m_ctx = NULL;
  }
}


bool OpalLibSRTP::ProtectRTP(RTP_DataFrame & frame)
{
  return m_tx->ProtectRTP(frame);
}


bool OpalLibSRTP::Context::ProtectRTP(RTP_DataFrame & frame)
{
  int len = frame.GetHeaderSize() + frame.GetPayloadSize();
  frame.SetMinSize(len + SRTP_MAX_TRAILER_LEN);
  if (!CHECK_ERROR(srtp_protect,(m_ctx, frame.GetPointer(), &len)))
    return false;

#if PTRACING
  if (m_firstRTP) {
    PTRACE(3, "SRTP\tProtected first RTP packet.");
    m_firstRTP = false;
  }
#endif

  frame.SetPayloadSize(len - frame.GetHeaderSize());
  return true;
}


bool OpalLibSRTP::ProtectRTCP(RTP_ControlFrame & frame)
{
  return m_tx->ProtectRTCP(frame);
}


bool OpalLibSRTP::Context::ProtectRTCP(RTP_ControlFrame & frame)
{
  int len = frame.GetCompoundSize();
  frame.SetMinSize(len + SRTP_MAX_TRAILER_LEN);

  if (!CHECK_ERROR(srtp_protect_rtcp,(m_ctx, frame.GetPointer(), &len)))
    return false;

#if PTRACING
  if (m_firstRTCP) {
    PTRACE(3, "SRTP\tProtected first RTCP packet.");
    m_firstRTCP = false;
  }
#endif

  frame.SetSize(len);
  return true;
}


bool OpalLibSRTP::UnprotectRTP(RTP_DataFrame & frame)
{
  return m_rx->UnprotectRTP(frame);
}


bool OpalLibSRTP::Context::UnprotectRTP(RTP_DataFrame & frame)
{
  int len = frame.GetHeaderSize() + frame.GetPayloadSize();
  if (!CHECK_ERROR(srtp_unprotect,(m_ctx, frame.GetPointer(), &len)))
    return false;

#if PTRACING
  if (m_firstRTP) {
    PTRACE(3, "SRTP\tUnprotected first RTP packet.");
    m_firstRTP = false;
  }
#endif

  frame.SetPayloadSize(len - frame.GetHeaderSize());
  return true;
}


bool OpalLibSRTP::UnprotectRTCP(RTP_ControlFrame & frame)
{
  return m_rx->UnprotectRTCP(frame);
}


bool OpalLibSRTP::Context::UnprotectRTCP(RTP_ControlFrame & frame)
{
  int len = frame.GetSize();
  if (!CHECK_ERROR(srtp_unprotect_rtcp,(m_ctx, frame.GetPointer(), &len)))
    return false;

#if PTRACING
  if (m_firstRTCP) {
    PTRACE(3, "SRTP\tUnprotected first RTCP packet.");
    m_firstRTCP = false;
  }
#endif

  frame.SetSize(len);
  return true;
}


///////////////////////////////////////////////////////

OpalSRTPSession::OpalSRTPSession(const Init & init)
  : OpalRTPSession(init)
{
}


OpalSRTPSession::~OpalSRTPSession()
{
  Close();
}


bool OpalSRTPSession::Close()
{
  m_rx->Close();
  m_tx->Close();
  return OpalRTPSession::Close();
}


OpalMediaCryptoKeyList & OpalSRTPSession::GetOfferedCryptoKeys()
{
  if (m_offeredCryptokeys.IsEmpty() && m_tx->m_keyInfo != NULL)
    m_offeredCryptokeys.Append(new OpalSRTPKeyInfo(*m_tx->m_keyInfo));

  return OpalRTPSession::GetOfferedCryptoKeys();
}


bool OpalSRTPSession::ApplyCryptoKey(OpalMediaCryptoKeyList & keys, bool rx)
{
  return rx ? m_rx->Open(GetSyncSourceIn(), keys)
            : m_tx->Open(GetSyncSourceOut(), keys);
}


OpalRTPSession::SendReceiveStatus OpalSRTPSession::OnSendData(RTP_DataFrame & frame)
{
  SendReceiveStatus status = OpalRTPSession::OnSendData(frame);
  if (status != e_ProcessPacket)
    return status;

  return ProtectRTP(frame) ? e_ProcessPacket : e_IgnorePacket;
}


OpalRTPSession::SendReceiveStatus OpalSRTPSession::OnSendControl(RTP_ControlFrame & frame)
{
  // Do not call OpalRTPSession::OnSendControl() as that makes frame too small
  return ProtectRTCP(frame) ? e_ProcessPacket : e_IgnorePacket;
}


OpalRTPSession::SendReceiveStatus OpalSRTPSession::OnReceiveData(RTP_DataFrame & frame)
{
  if (GetSyncSourceIn() != frame.GetSyncSource())
    m_rx->Change(GetSyncSourceIn(), frame.GetSyncSource());
  return UnprotectRTP(frame) ? OpalRTPSession::OnReceiveData(frame) : e_IgnorePacket;
}


OpalRTPSession::SendReceiveStatus OpalSRTPSession::OnReceiveControl(RTP_ControlFrame & frame)
{
  return UnprotectRTCP(frame) ? OpalRTPSession::OnReceiveControl(frame) : e_IgnorePacket;
}


#endif // OPAL_SRTP
