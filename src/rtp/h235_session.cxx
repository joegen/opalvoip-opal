/*
 * h235_session.cxx
 *
 * H.235 encrypted RTP protocol session handler
 *
 * OPAL Library
 *
 * Copyright (C) 2013 Vox Lucida Pty. Ltd.
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
#pragma implementation "h235_session.h"
#endif

#include <opal_config.h>

#if OPAL_H235_6

#include <rtp/h235_session.h>
#include <h323/h323caps.h>
#include <ptclib/cypher.h>
#include <ptclib/random.h>


#define PTraceModule() "AES-Session"


///////////////////////////////////////////////////////

const PCaselessString & H2356_Session::SessionType() { static const PConstCaselessString s("RTP/H.235.6"); return s; }

PFACTORY_CREATE(OpalMediaSessionFactory, H2356_Session, H2356_Session::SessionType());

static PConstCaselessString AES_128("AES_128");

class H2356_CryptoSuite_128 : public H2356_CryptoSuite
{
    PCLASSINFO(H2356_CryptoSuite_128, H2356_CryptoSuite);
  public:
    virtual const PCaselessString & GetFactoryName() const { return AES_128; }
    virtual const char * GetDescription() const { return "H.235.6 AES-128"; }
    virtual const char * GetOID() const { return "2.16.840.1.101.3.4.1.2"; }
    virtual PINDEX GetCipherKeyBits() const { return 128; }
};

PFACTORY_CREATE(OpalMediaCryptoSuiteFactory, H2356_CryptoSuite_128, AES_128, true);


static PConstCaselessString AES_192("AES_192");

class H2356_CryptoSuite_192 : public H2356_CryptoSuite
{
    PCLASSINFO(H2356_CryptoSuite_192, H2356_CryptoSuite);
  public:
    virtual const PCaselessString & GetFactoryName() const { return AES_192; }
    virtual const char * GetDescription() const { return "H.235.6 AES-192"; }
    virtual const char * GetOID() const { return "2.16.840.1.101.3.4.1.22"; }
    virtual PINDEX GetCipherKeyBits() const { return 192; }
};

PFACTORY_CREATE(OpalMediaCryptoSuiteFactory, H2356_CryptoSuite_192, AES_192, true);


static PConstCaselessString AES_256("AES_256");

class H2356_CryptoSuite_256 : public H2356_CryptoSuite
{
    PCLASSINFO(H2356_CryptoSuite_256, H2356_CryptoSuite);
  public:
    virtual const PCaselessString & GetFactoryName() const { return AES_256; }
    virtual const char * GetDescription() const { return "H.235.6 AES-256"; }
    virtual const char * GetOID() const { return "2.16.840.1.101.3.4.1.42"; }
    virtual PINDEX GetCipherKeyBits() const { return 256; }
};

PFACTORY_CREATE(OpalMediaCryptoSuiteFactory, H2356_CryptoSuite_256, AES_256, true);



///////////////////////////////////////////////////////

H235SecurityCapability * H2356_CryptoSuite::CreateCapability(const H323Capability & mediaCapability) const
{
  return new H235SecurityAlgorithmCapability(mediaCapability);
}


bool H2356_CryptoSuite::Supports(const PCaselessString & proto) const
{
  return proto == "h323";
}


bool H2356_CryptoSuite::ChangeSessionType(PCaselessString & mediaSession, KeyExchangeModes modes) const
{
  if (mediaSession != OpalRTPSession::RTP_AVP())
    return false;

  if (!(modes&(e_SecureSignalling | e_InBandKeyEchange)))
    return false;

  mediaSession = H2356_Session::SessionType();
  return true;
}


OpalMediaCryptoKeyInfo * H2356_CryptoSuite::CreateKeyInfo() const
{
  return new H2356_KeyInfo(*this);
}


///////////////////////////////////////////////////////////////////////

H2356_KeyInfo::H2356_KeyInfo(const H2356_CryptoSuite & cryptoSuite)
  : OpalMediaCryptoKeyInfo(cryptoSuite)
  , m_cryptoSuite(cryptoSuite)
{
}


PObject * H2356_KeyInfo::Clone() const
{
  H2356_KeyInfo * clone = new H2356_KeyInfo(*this);
  clone->m_key.MakeUnique();
  return clone;
}


PObject::Comparison H2356_KeyInfo::Compare(const PObject & other) const
{
  return m_key.Compare(dynamic_cast<const H2356_KeyInfo&>(other).m_key);
}


bool H2356_KeyInfo::IsValid() const
{
  return m_key.GetSize() == m_cryptoSuite.GetCipherKeyBytes();
}


bool H2356_KeyInfo::FromString(const PString & str)
{
  PBYTEArray key;
  if (!PBase64::Decode(str, key)) {
    PTRACE2(2, &m_cryptoSuite, "SDP\tIllegal key \"" << str << '"');
    return false;
  }

  if (key.GetSize() < m_cryptoSuite.GetCipherKeyBytes()) {
    PTRACE2(2, &m_cryptoSuite, "Crypto\tIncorrect combined key/salt size"
            " (" << key.GetSize() << " bytes) for " << m_cryptoSuite);
    return false;
  }

  SetCipherKey(key);
  return true;
}


PString H2356_KeyInfo::ToString() const
{
  return PBase64::Encode(m_key, "");
}


void H2356_KeyInfo::Randomise()
{
  PRandom::Octets(m_key, m_cryptoSuite.GetCipherKeyBytes());
}


bool H2356_KeyInfo::SetCipherKey(const PBYTEArray & key)
{
  if (key.GetSize() < m_cryptoSuite.GetCipherKeyBytes()) {
    PTRACE2(2, &m_cryptoSuite, "Crypto\tIncorrect key size (" << key.GetSize() << " bytes) for " << m_cryptoSuite);
    return false;
  }

  m_key = key;
  return true;
}


bool H2356_KeyInfo::SetAuthSalt(const PBYTEArray &)
{
  return false;
}


PBYTEArray H2356_KeyInfo::GetCipherKey() const
{
  return m_key;
}


PBYTEArray H2356_KeyInfo::GetAuthSalt() const
{
  return PBYTEArray();
}


///////////////////////////////////////////////////////

H2356_Session::H2356_Session(const Init & init)
  : OpalRTPSession(init)
  , m_rx(false)
  , m_tx(true)
{
}


H2356_Session::~H2356_Session()
{
  Close();
}


const PCaselessString & H2356_Session::GetSessionType() const
{
  return SessionType();
}


bool H2356_Session::Close()
{
  return OpalRTPSession::Close();
}


OpalMediaCryptoKeyList & H2356_Session::GetOfferedCryptoKeys()
{
  if (m_offeredCryptokeys.IsEmpty() && m_tx.m_keyInfo != NULL)
    m_offeredCryptokeys.Append(new H2356_KeyInfo(*m_tx.m_keyInfo));

  return OpalRTPSession::GetOfferedCryptoKeys();
}


bool H2356_Session::ApplyCryptoKey(OpalMediaCryptoKeyList & keys, bool rx)
{
  for (OpalMediaCryptoKeyList::iterator it = keys.begin(); it != keys.end(); ++it) {
    H2356_KeyInfo * keyInfo = dynamic_cast<H2356_KeyInfo *>(&*it);
    if (keyInfo == NULL) {
      PTRACE(2, "Unsuitable crypto suite " << *it);
      continue;
    }

    if ((rx ? m_rx : m_tx).Open(*keyInfo)) {
      keys.Select(it);
      return true;
    }
  }

  return false;
}


OpalMediaCryptoKeyInfo * H2356_Session::IsCryptoSecured(bool rx) const
{
  return (rx ? m_rx : m_tx).m_keyInfo;
}


OpalRTPSession::SendReceiveStatus H2356_Session::OnSendData(RewriteMode & rewrite, RTP_DataFrame & frame, const PTime & now)
{
  SendReceiveStatus status = OpalRTPSession::OnSendData(rewrite, frame, now);
  if (status == e_ProcessPacket && rewrite != e_RewriteNothing) {
    if (!m_tx.Encrypt(frame))
      return e_IgnorePacket;
  }
  return status;
}


#if !H235_6_CODED_TO_CORRECT_SPECIFICATION
OpalRTPSession::SendReceiveStatus H2356_Session::OnPreReceiveData(RTP_DataFrame & frame, const PTime & now)
{
  // Allow for broken implementations that set padding bit but do not set the padding length!
  bool padding = frame.GetPadding();
  frame.SetPadding(false);

  SendReceiveStatus status = OpalRTPSession::OnPreReceiveData(frame, now);

  frame.SetPadding(padding);

  return status;
}
#endif


OpalRTPSession::SendReceiveStatus H2356_Session::OnReceiveData(RTP_DataFrame & frame, ReceiveType rxType, const PTime & now)
{
  return (rxType == e_RxFromRTX || m_rx.Decrypt(frame)) ? OpalRTPSession::OnReceiveData(frame, rxType, now) : e_IgnorePacket;
}


bool H2356_Session::Context::Open(H2356_KeyInfo & info)
{
  if (!m_cipher.SetAlgorithm(info.GetCryptoSuite().GetOID()))
    return false;

  if (!m_cipher.SetKey(info.GetCipherKey()))
    return false;

  m_iv.SetSize(m_cipher.GetKeyLength());
  m_keyInfo = new H2356_KeyInfo(info);
  return true;
}


bool H2356_Session::Context::PreProcess(RTP_DataFrame & frame)
{
  if (m_keyInfo == NULL)
    return false;

  // Make a copy of the frame
  m_buffer.Copy(frame);

  // Calculate the initial vector as per H.235.6/9.3.1.1 - CBC mode only for now
  PINDEX len = m_iv.GetSize();
  BYTE * ptr = m_iv.GetPointer();
  while (len > 0) {
    PINDEX copy = std::min(len, (PINDEX)6);
    memcpy(ptr, &frame[2], copy); // Copy the sequence number and time stamp bytes
    len -= copy;
    ptr += copy;
  }

  m_cipher.SetIV(m_iv);

  return true;
}


bool H2356_Session::Context::Encrypt(RTP_DataFrame & frame)
{
  if (!PreProcess(frame))
    return true;

  PINDEX len = m_cipher.GetBlockedDataSize(frame.GetPayloadSize());

#if H235_6_CODED_TO_CORRECT_SPECIFICATION
  frame.SetPaddingSize(len - frame.GetPayloadSize());
#else
  // Allow for broken implementations that set padding bit but not set the padding length!
  // We have to emulate their broken-ness in what we send too, that is set the bit bit do
  // not set the size bytes. Give me a BREAK ...
  frame.SetPadding(len != frame.GetPayloadSize());
#endif

  // Can't do cipher stealing .. yet ...  maybe never
  m_cipher.SetPadding(frame.GetPadding() ? PSSLCipherContext::PadPKCS : PSSLCipherContext::NoPadding);

  // Make sure we have enough memory space for an extra data block in case implementation is naughty
  frame.SetMinSize(len + m_cipher.GetBlockSize());

  // Do it encryption
  bool ok = m_cipher.Process(m_buffer.GetPayloadPtr(), m_buffer.GetPayloadSize(), frame.GetPayloadPtr(), len);
  frame.SetPayloadSize(len);

  PTRACE(5, NULL, PTraceModule(),
         "Encrypt: in=" << m_buffer.GetPayloadSize()
             << " pkt=" << frame.GetPacketSize()
             << " pad=" << frame.GetPadding()
             << " psz=" << frame.GetPaddingSize()
             << " out=" << len
             <<  " ok=" << ok);

  return ok;
}


bool H2356_Session::Context::Decrypt(RTP_DataFrame & frame)
{
  if (!PreProcess(frame))
    return true;

  // Get length of whole payload including the padding
  PINDEX len = frame.GetPacketSize() - frame.GetHeaderSize();

  if (frame.GetPadding())
    m_cipher.SetPadding(PSSLCipherContext::PadLoosePKCS);
  else if (frame.GetPayloadSize()%m_cipher.GetBlockSize() == 0)
    m_cipher.SetPadding(PSSLCipherContext::NoPadding);
  else
    m_cipher.SetPadding(PSSLCipherContext::PadCipherStealing);

  bool ok = m_cipher.Process(m_buffer.GetPayloadPtr(), len, frame.GetPayloadPtr(), len);

  PTRACE(5, NULL, PTraceModule(),
         "Decrypt: in=" << m_buffer.GetPayloadSize()
             << " pkt=" << m_buffer.GetPacketSize()
             << " pad=" << m_buffer.GetPadding()
             << " psz=" << m_buffer.GetPaddingSize()
             << " out=" << len
             <<  " ok=" << ok);

  frame.SetPaddingSize(0);
  frame.SetPayloadSize(len);
  return ok;
}
#endif // OPAL_H235_6
