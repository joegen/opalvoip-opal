/*
 * srtp.h
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

#ifndef OPAL_RTP_SRTP_H
#define OPAL_RTP_SRTP_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#ifndef _PTLIB_H
#include <ptlib.h>
#endif

#include <opal/buildopts.h>

#include <rtp/rtp.h>
#include <opal/rtpconn.h>

#if OPAL_SRTP

class OpalSRTPCryptoSuite;


////////////////////////////////////////////////////////////////////
//
//  this class holds the parameters required for an SRTP session
//
//  Crypto modes are identified by key strings that are contained in PFactory<OpalSRTPParms>
//  The following strings should be implemented:
//
//     AES_CM_128_HMAC_SHA1_80,
//     AES_CM_128_HMAC_SHA1_32,
//     AES_CM_128_NULL_AUTH,   
//     NULL_CIPHER_HMAC_SHA1_80
//     STRONGHOLD
//

struct OpalSRTPKeyInfo : public OpalMediaCryptoKeyInfo {
  public:
    OpalSRTPKeyInfo(const OpalSRTPCryptoSuite & cryptoSuite);

    PObject * Clone() const;

    virtual bool IsValid() const;
    virtual void Randomise();
    virtual bool FromString(const PString & str);
    virtual PString ToString() const;

    bool SetCipherKey(const PBYTEArray & key);
    bool SetAuthSalt(const PBYTEArray & key);

    PBYTEArray GetCipherKey() const { return m_key; }
    PBYTEArray GetAuthSalt() const { return m_salt; }

    const OpalSRTPCryptoSuite & GetCryptoSuite() const { return m_cryptoSuite; }

  protected:
    const OpalSRTPCryptoSuite & m_cryptoSuite;
    PBYTEArray m_key;
    PBYTEArray m_salt;
};


class OpalSRTPCryptoSuite : public OpalMediaCryptoSuite
{
    PCLASSINFO(OpalSRTPCryptoSuite, OpalMediaCryptoSuite);
  protected:
    OpalSRTPCryptoSuite() { }

  public:
    virtual bool Supports(const PCaselessString & proto) const;
    virtual bool ChangeSessionType(PCaselessString & mediaSession) const;

    virtual OpalMediaCryptoKeyInfo * CreateKeyInfo() const;

    virtual PINDEX GetCipherKeyBits() const = 0;
    virtual PINDEX GetAuthSaltBits() const = 0;

    virtual void SetCryptoPolicy(struct crypto_policy_t & policy) const = 0;
};

class OpalLibSRTP
{
  protected:
    OpalLibSRTP();
    ~OpalLibSRTP();

    struct Context {
      Context();
      ~Context() { Close(); }
      void Close();

      struct srtp_ctx_t * m_ctx;
      BYTE m_key_salt[32]; // libsrtp vague on size, looking at source code, says 32 bytes
#if PTRACING
      bool m_firstRTP;
      bool m_firstRTCP;
#endif
    };

    bool OpenCrypto(Context & ctx, DWORD ssrc, OpalMediaCryptoKeyList & keys);
    void CloseCrypto();

    bool ProtectRTP(RTP_DataFrame & frame);
    bool ProtectRTCP(RTP_ControlFrame & frame);
    bool UnprotectRTP(RTP_DataFrame & frame);
    bool UnprotectRTCP(RTP_ControlFrame & frame);

    Context m_rx, m_tx;
};


/** This class implements SRTP using libSRTP
  */
class OpalSRTPSession : public OpalRTPSession, OpalLibSRTP
{
  PCLASSINFO(OpalSRTPSession, OpalRTPSession);
  public:
    static const PCaselessString & RTP_SAVP();

    OpalSRTPSession(const Init & init);
    ~OpalSRTPSession();

    virtual const PCaselessString & GetSessionType() const { return RTP_SAVP(); }
    virtual bool Close();
    virtual bool ApplyCryptoKey(OpalMediaCryptoKeyList & keys, bool rx);

    virtual SendReceiveStatus OnSendData(RTP_DataFrame & frame);
    virtual SendReceiveStatus OnSendControl(RTP_ControlFrame & frame);
    virtual SendReceiveStatus OnReceiveData(RTP_DataFrame & frame);
    virtual SendReceiveStatus OnReceiveControl(RTP_ControlFrame & frame);
};


#endif // OPAL_SRTP

#endif // OPAL_RTP_SRTP_H
