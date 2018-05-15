/*
 * h235_session.h
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
 */

#ifndef OPAL_RTP_H235_SESSION_H
#define OPAL_RTP_H235_SESSION_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#ifndef _PTLIB_H
#include <ptlib.h>
#endif

#include <opal_config.h>

#if OPAL_H235_6

#include <rtp/rtp.h>
#include <rtp/rtpconn.h>
#include <ptclib/pssl.h>


class H2356_CryptoSuite;


#define H235_6_CODED_TO_CORRECT_SPECIFICATION 0


////////////////////////////////////////////////////////////////////
//
//  this class holds the parameters required for an AES session
//
//  Crypto modes are identified by key strings that are contained in PFactory<OpalSRTPParms>
//  The following strings should be implemented:
//
//     AES_128, AES_192, AES_256
//

struct H2356_KeyInfo : public OpalMediaCryptoKeyInfo {
  public:
    H2356_KeyInfo(const H2356_CryptoSuite & cryptoSuite);

    virtual PObject * Clone() const;
    virtual Comparison Compare(const PObject & other) const;

    virtual bool IsValid() const;
    virtual void Randomise();
    virtual bool FromString(const PString & str);
    virtual PString ToString() const;
    virtual bool SetCipherKey(const PBYTEArray & key);
    virtual bool SetAuthSalt(const PBYTEArray & key);
    virtual PBYTEArray GetCipherKey() const;
    virtual PBYTEArray GetAuthSalt() const;
    virtual PINDEX GetAuthSaltBits() const { return 0; }

    const H2356_CryptoSuite & GetCryptoSuite() const { return m_cryptoSuite; }

  protected:
    const H2356_CryptoSuite & m_cryptoSuite;
    PBYTEArray m_key;
};


class H2356_CryptoSuite : public OpalMediaCryptoSuite
{
    PCLASSINFO(H2356_CryptoSuite, OpalMediaCryptoSuite);
  protected:
    H2356_CryptoSuite() { }

  public:
    virtual H235SecurityCapability * CreateCapability(const H323Capability & mediaCapability) const;
    virtual bool Supports(const PCaselessString & proto) const;
    virtual bool ChangeSessionType(PCaselessString & mediaSession, KeyExchangeModes modes) const;

    virtual OpalMediaCryptoKeyInfo * CreateKeyInfo() const;

    virtual PINDEX GetCipherKeyBits() const = 0;
    virtual PINDEX GetAuthSaltBits() const { return 0; }
};

/** This class implements AES using OpenSSL
  */
class H2356_Session : public OpalRTPSession
{
  PCLASSINFO(H2356_Session, OpalRTPSession);
  public:
    static const PCaselessString & SessionType();

    H2356_Session(const Init & init);
    ~H2356_Session();

    virtual const PCaselessString & GetSessionType() const;
    virtual bool Close();
    virtual OpalMediaCryptoKeyList & GetOfferedCryptoKeys();
    virtual bool ApplyCryptoKey(OpalMediaCryptoKeyList & keys, bool rx);
    virtual OpalMediaCryptoKeyInfo * IsCryptoSecured(bool rx) const;

    virtual SendReceiveStatus OnSendData(RewriteMode & rewrite, RTP_DataFrame & frame, const PTime & now);
#if !H235_6_CODED_TO_CORRECT_SPECIFICATION
    virtual SendReceiveStatus OnPreReceiveData(RTP_DataFrame & frame, const PTime & now);
#endif
    virtual SendReceiveStatus OnReceiveData(RTP_DataFrame & frame, ReceiveType rxType, const PTime & now);

  protected:
    struct Context {
      Context(bool encrypt) : m_keyInfo(NULL), m_cipher(encrypt) { }
      ~Context() { delete m_keyInfo; }

      bool Open(H2356_KeyInfo & info);
      bool PreProcess(RTP_DataFrame & frame);
      bool Encrypt(RTP_DataFrame & frame);
      bool Decrypt(RTP_DataFrame & frame);

      H2356_KeyInfo   * m_keyInfo;
      PSSLCipherContext m_cipher;
      RTP_DataFrame     m_buffer;
      PBYTEArray        m_iv;
    } m_rx, m_tx;
};


#endif // OPAL_H235_6

#endif // OPAL_RTP_H235_SESSION_H
