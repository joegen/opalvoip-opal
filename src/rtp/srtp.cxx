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
 * $Log: srtp.cxx,v $
 * Revision 1.2.2.2  2006/09/12 07:06:58  csoutheren
 * More implementation of SRTP and general call security
 *
 * Revision 1.2.2.1  2006/09/08 06:23:31  csoutheren
 * Implement initial support for SRTP media encryption and H.235-SRTP support
 * This code currently inserts SRTP offers into outgoing H.323 OLC, but does not
 * yet populate capabilities or respond to negotiations. This code to follow
 *
 * Revision 1.2  2006/09/05 06:18:23  csoutheren
 * Start bringing in SRTP code for libSRTP
 *
 * Revision 1.1  2006/08/21 06:19:28  csoutheren
 * Added placeholders for SRTP implementation
 *
 */

#include <ptlib.h>
#include <opal/buildopts.h>

#ifdef __GNUC__
#pragma implementation "srtp.h"
#endif

#include <rtp/srtp.h>
#include <opal/connection.h>

////////////////////////////////////////////////////////////////////
//
//  this class implements SRTP over UDP
//

OpalSRTP_UDP::OpalSRTP_UDP(
     unsigned id,                   ///<  Session ID for RTP channel
      BOOL remoteIsNAT,             ///<  TRUE is remote is behind NAT
      OpalSecurityMode * _srtpParms ///<  Paramaters to use for SRTP
) : RTP_UDP(id, remoteIsNAT)
{
  srtpParms = dynamic_cast<OpalSRTPSecurityMode *>(_srtpParms);
}

OpalSRTP_UDP::~OpalSRTP_UDP()
{
  delete srtpParms;
}

/////////////////////////////////////////////////////////////////////////////////////
//
//  SRTP implementation using Cisco libSRTP
//  See http://srtp.sourceforge.net/srtp.html
//

////////////////////////////////////////////////////////////////////
//
//  implement SRTP via libSRTP
//

#if defined(HAS_LIBSRTP) 

namespace PWLibStupidLinkerHacks {
  int libSRTPLoader;
};

#pragma comment(lib, LIBSRTP_LIBRARY)

#include <srtp/include/srtp.h>

class LibSRTP_UDP : public OpalSRTP_UDP
{
  PCLASSINFO(LibSRTP_UDP, OpalSRTP_UDP);
  public:
    LibSRTP_UDP(
      unsigned int id,             ///<  Session ID for RTP channel
      BOOL remoteIsNAT,            ///<  TRUE is remote is behind NAT
      OpalSecurityMode * srtpParms ///<  Paramaters to use for SRTP
    );

    ~LibSRTP_UDP();

    virtual SendReceiveStatus OnSendData   (RTP_DataFrame & frame);
    virtual SendReceiveStatus OnReceiveData(RTP_DataFrame & frame);
};

///////////////////////////////////////////////////////

class StaticInitializer
{
  public:
    StaticInitializer()
    { srtp_init(); }
};

static StaticInitializer initializer;

///////////////////////////////////////////////////////

class LibSRTPSecurityMode_Base : public OpalSRTPSecurityMode
{
  PCLASSINFO(LibSRTPSecurityMode_Base, OpalSRTPSecurityMode);
  public:
    OpalSRTP_UDP * CreateRTPSession(
      unsigned id,          ///<  Session ID for RTP channel
      BOOL remoteIsNAT      ///<  TRUE is remote is behind NAT
    );

    BOOL SetKey(const PBYTEArray & key);
    BOOL SetKey(const PBYTEArray & key, const PBYTEArray & salt);

    BOOL GetKey(PBYTEArray & key)
    { 
      PBYTEArray salt; 
      if (GetKey(key, salt)) 
        return FALSE; 
      PINDEX len = key.GetSize();
      memcpy(key.GetPointer(len + salt.GetSize()), salt.GetPointer(), salt.GetSize());
      return TRUE;
    }
    BOOL GetKey(PBYTEArray & _key, PBYTEArray & _salt)
    { _key = masterKey; _salt = salt; return TRUE; }

    BOOL SetSSRC(DWORD ssrc);
    BOOL GetSSRC(DWORD & ssrc) const;

  protected:
    void Init();
    PBYTEArray masterKey;
    PBYTEArray salt;
    srtp_policy_t inboundPolicy;
    srtp_policy_t outboundPolicy;
};

void LibSRTPSecurityMode_Base::Init()
{
  inboundPolicy.ssrc.type  = ssrc_any_inbound;
  inboundPolicy.next       = NULL;
  outboundPolicy.ssrc.type = ssrc_any_outbound;
  outboundPolicy.next      = NULL;

  crypto_get_random(masterKey.GetPointer(SRTP_MASTER_KEY_LEN), sizeof(SRTP_MASTER_KEY_LEN));
}


OpalSRTP_UDP * LibSRTPSecurityMode_Base::CreateRTPSession(unsigned id, BOOL remoteIsNAT)
{
  return new LibSRTP_UDP(id, remoteIsNAT, this);
}

BOOL LibSRTPSecurityMode_Base::SetSSRC(DWORD ssrc)
{
  inboundPolicy.ssrc.type  = ssrc_specific;
  inboundPolicy.ssrc.value = ssrc;

  outboundPolicy.ssrc.type = ssrc_specific;
  outboundPolicy.ssrc.value = ssrc;

  return TRUE;
}

BOOL LibSRTPSecurityMode_Base::GetSSRC(DWORD & ssrc) const
{
  ssrc = outboundPolicy.ssrc.value;
  return TRUE;
}

BOOL LibSRTPSecurityMode_Base::SetKey(const PBYTEArray & k, const PBYTEArray & s)
{
  PBYTEArray key(k);
  if (s.GetSize() > 0) {
    key.MakeUnique();
    PINDEX l = key.GetSize();
    memcpy(key.GetPointer(l + s.GetSize()) + l, (const BYTE *)s, s.GetSize());
  }
  return SetKey(key);
}

BOOL LibSRTPSecurityMode_Base::SetKey(const PBYTEArray & key)
{
  if (key.GetSize() != SRTP_MASTER_KEY_LEN)
    return FALSE;

  masterKey = key;
  masterKey.MakeUnique();

  return TRUE;
}


#define DECLARE_LIBSRTP_CRYPTO_ALG(name, policy_fn) \
class LibSRTPSecurityMode_##name : public LibSRTPSecurityMode_Base \
{ \
  public: \
  LibSRTPSecurityMode_##name() \
    { \
      policy_fn(&inboundPolicy.rtp); \
      policy_fn(&inboundPolicy.rtcp); \
      policy_fn(&outboundPolicy.rtp); \
      policy_fn(&outboundPolicy.rtcp); \
      Init(); \
    } \
}; \
static PFactory<OpalSecurityMode>::Worker<LibSRTPSecurityMode_##name> factoryLibSRTPSecurityMode_##name("SRTP|" #name); \

DECLARE_LIBSRTP_CRYPTO_ALG(AES_CM_128_HMAC_SHA1_80,  crypto_policy_set_aes_cm_128_hmac_sha1_80);
DECLARE_LIBSRTP_CRYPTO_ALG(AES_CM_128_HMAC_SHA1_32,  crypto_policy_set_aes_cm_128_hmac_sha1_32);
DECLARE_LIBSRTP_CRYPTO_ALG(AES_CM_128_NULL_AUTH,     crypto_policy_set_aes_cm_128_null_auth);
DECLARE_LIBSRTP_CRYPTO_ALG(NULL_CIPHER_HMAC_SHA1_80, crypto_policy_set_null_cipher_hmac_sha1_80);

DECLARE_LIBSRTP_CRYPTO_ALG(STRONGHOLD,               crypto_policy_set_aes_cm_128_hmac_sha1_80);

///////////////////////////////////////////////////////

LibSRTP_UDP::LibSRTP_UDP(unsigned _sessionId, BOOL _remoteIsNAT, OpalSecurityMode * _srtpParms)
  : OpalSRTP_UDP(_sessionId, _remoteIsNAT, _srtpParms)
{
}

LibSRTP_UDP::~LibSRTP_UDP()
{
}

RTP_UDP::SendReceiveStatus LibSRTP_UDP::OnSendData(RTP_DataFrame & frame)
{
  return RTP_UDP::OnSendData(frame);
}

RTP_UDP::SendReceiveStatus LibSRTP_UDP::OnReceiveData(RTP_DataFrame & frame)
{
  return RTP_UDP::OnReceiveData(frame);
}

#endif // OPAL_HAS_LIBSRTP

