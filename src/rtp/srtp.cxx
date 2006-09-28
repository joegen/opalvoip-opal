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
 * Revision 1.3  2006/09/28 07:42:18  csoutheren
 * Merge of useful SRTP implementation
 *
 * Revision 1.2.2.3  2006/09/12 07:47:15  csoutheren
 * Changed to use seperate incoming and outgoing keys
 *
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
#include <h323/h323caps.h>
#include <h323/h235auth.h>

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
    RTP_UDP * CreateRTPSession(
      unsigned id,          ///<  Session ID for RTP channel
      BOOL remoteIsNAT      ///<  TRUE is remote is behind NAT
    );

    BOOL SetOutgoingKey(const KeySalt & key)  { outgoingKey = key; return TRUE; }
    BOOL GetOutgoingKey(KeySalt & key) const  { key = outgoingKey; return TRUE; }
    BOOL SetOutgoingSSRC(DWORD ssrc);
    BOOL GetOutgoingSSRC(DWORD & ssrc) const;

    BOOL SetIncomingKey(const KeySalt & key)  { incomingKey = key; return TRUE; }
    BOOL GetIncomingKey(KeySalt & key) const  { key = incomingKey; return TRUE; } ;
    BOOL SetIncomingSSRC(DWORD ssrc);
    BOOL GetIncomingSSRC(DWORD & ssrc) const;

    BOOL Open();

    srtp_t inboundSession;
    srtp_t outboundSession;

  protected:
    void Init();
    KeySalt incomingKey;
    KeySalt outgoingKey;
    srtp_policy_t inboundPolicy;
    srtp_policy_t outboundPolicy;
};

void LibSRTPSecurityMode_Base::Init()
{
  inboundPolicy.ssrc.type  = ssrc_any_inbound;
  inboundPolicy.next       = NULL;
  outboundPolicy.ssrc.type = ssrc_any_outbound;
  outboundPolicy.next      = NULL;

  crypto_get_random(outgoingKey.key.GetPointer(SRTP_MASTER_KEY_LEN), SRTP_MASTER_KEY_LEN);
}


RTP_UDP * LibSRTPSecurityMode_Base::CreateRTPSession(unsigned id, BOOL remoteIsNAT)
{
  return new LibSRTP_UDP(id, remoteIsNAT, this);
}

BOOL LibSRTPSecurityMode_Base::SetIncomingSSRC(DWORD ssrc)
{
  inboundPolicy.ssrc.type  = ssrc_specific;
  inboundPolicy.ssrc.value = ssrc;
  return TRUE;
}

BOOL LibSRTPSecurityMode_Base::SetOutgoingSSRC(DWORD ssrc)
{
  outboundPolicy.ssrc.type = ssrc_specific;
  outboundPolicy.ssrc.value = ssrc;

  return TRUE;
}

BOOL LibSRTPSecurityMode_Base::GetOutgoingSSRC(DWORD & ssrc) const
{
  ssrc = outboundPolicy.ssrc.value;
  return TRUE;
}

BOOL LibSRTPSecurityMode_Base::GetIncomingSSRC(DWORD & ssrc) const
{
  ssrc = inboundPolicy.ssrc.value;
  return TRUE;
}

BOOL LibSRTPSecurityMode_Base::Open()
{
  outboundPolicy.key = outgoingKey.key.GetPointer();
  err_status_t err = srtp_create(&outboundSession, &outboundPolicy);
  if (err != ::err_status_ok)
    return FALSE;

  inboundPolicy.key = incomingKey.key.GetPointer();
  err = srtp_create(&inboundSession, &inboundPolicy);
  if (err != ::err_status_ok)
    return FALSE;

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
  LibSRTPSecurityMode_Base * srtp = (LibSRTPSecurityMode_Base *)srtpParms;

  int len = frame.GetHeaderSize() + frame.GetPayloadSize();
  err_status_t err = ::srtp_protect(srtp->outboundSession, frame.GetPointer(), &len);
  if (err != err_status_ok)
    return RTP_Session::e_IgnorePacket;
  frame.SetPayloadSize(len - frame.GetHeaderSize());

  return RTP_UDP::OnSendData(frame);
}

RTP_UDP::SendReceiveStatus LibSRTP_UDP::OnReceiveData(RTP_DataFrame & frame)
{
  LibSRTPSecurityMode_Base * srtp = (LibSRTPSecurityMode_Base *)srtpParms;

  int len = frame.GetHeaderSize() + frame.GetPayloadSize();
  err_status_t err = ::srtp_unprotect(srtp->inboundSession, frame.GetPointer(), &len);
  if (err != err_status_ok)
    return RTP_Session::e_IgnorePacket;
  frame.SetPayloadSize(len - frame.GetHeaderSize());

  return RTP_UDP::OnReceiveData(frame);
}

///////////////////////////////////////////////////////

#endif // OPAL_HAS_LIBSRTP

