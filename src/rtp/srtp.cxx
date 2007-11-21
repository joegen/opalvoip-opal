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
#pragma implementation "srtp.h"
#endif

#include <opal/buildopts.h>

#if defined(OPAL_SRTP)

#include <rtp/srtp.h>
#include <opal/connection.h>
#include <h323/h323caps.h>
#include <h323/h235auth.h>

// default key = 2687012454


////////////////////////////////////////////////////////////////////
//
//  this class implements SRTP over UDP
//

OpalSRTP_UDP::OpalSRTP_UDP(PHandleAggregator * _aggregator,   ///<  RTP aggregator
                                      unsigned id,            ///<  Session ID for RTP channel
                                          BOOL remoteIsNAT)   ///<  TRUE is remote is behind NAT
  : SecureRTP_UDP(_aggregator, id, remoteIsNAT)
{
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

#if HAS_LIBSRTP || HAS_LIBZRTP

namespace PWLibStupidLinkerHacks {
  int libSRTPLoader;
};

#if HAS_LIBSRTP && _WIN32
#pragma comment(lib, LIBSRTP_LIBRARY)
#endif

#ifdef _WIN32
#pragma warning(disable:4244)
#pragma warning(disable:4505)
#endif

extern "C" {
#include "srtp/srtp.h"
};

///////////////////////////////////////////////////////

class LibSRTPSecurityMode_Base : public OpalSRTPSecurityMode
{
  PCLASSINFO(LibSRTPSecurityMode_Base, OpalSRTPSecurityMode);
  public:
    RTP_UDP * CreateRTPSession(PHandleAggregator * _aggregator,   ///< handle aggregator
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

  private:
    static BOOL inited;
    static PMutex initMutex;
};

BOOL LibSRTPSecurityMode_Base::inited = FALSE;
PMutex LibSRTPSecurityMode_Base::initMutex;

void LibSRTPSecurityMode_Base::Init()
{
  {
    PWaitAndSignal m(initMutex);
    if (!inited) {
      srtp_init();
      inited = TRUE;
    }
  }
  inboundPolicy.ssrc.type  = ssrc_any_inbound;
  inboundPolicy.next       = NULL;
  outboundPolicy.ssrc.type = ssrc_any_outbound;
  outboundPolicy.next      = NULL;

  crypto_get_random(outgoingKey.key.GetPointer(SRTP_MASTER_KEY_LEN), SRTP_MASTER_KEY_LEN);
}


RTP_UDP * LibSRTPSecurityMode_Base::CreateRTPSession(
                              PHandleAggregator * _aggregator,   ///< handle aggregator
                                         unsigned id, 
                                             BOOL remoteIsNAT)
{
  LibSRTP_UDP * session = new LibSRTP_UDP(_aggregator, id, remoteIsNAT);
  session->SetSecurityMode(this);
  return session;
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
  if (outboundPolicy.ssrc.type != ssrc_specific)
    return FALSE;
  ssrc = outboundPolicy.ssrc.value;
  return TRUE;
}

BOOL LibSRTPSecurityMode_Base::GetIncomingSSRC(DWORD & ssrc) const
{
  if (inboundPolicy.ssrc.type != ssrc_specific)
    return FALSE;

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

LibSRTP_UDP::LibSRTP_UDP(PHandleAggregator * _aggregator,   ///< handle aggregator
                                    unsigned _sessionId, 
                                        BOOL _remoteIsNAT)
  : OpalSRTP_UDP(_aggregator, _sessionId, _remoteIsNAT)
{
}

LibSRTP_UDP::~LibSRTP_UDP()
{
}

BOOL LibSRTP_UDP::Open(
      PIPSocket::Address localAddress,  ///<  Local interface to bind to
      WORD portBase,                    ///<  Base of ports to search
      WORD portMax,                     ///<  end of ports to search (inclusive)
      BYTE ipTypeOfService,             ///<  Type of Service byte
      PSTUNClient * stun,               ///<  STUN server to use createing sockets (or NULL if no STUN)
      RTP_QOS * rtpqos                  ///<  QOS spec (or NULL if no QoS)
)
{
  LibSRTPSecurityMode_Base * srtp = (LibSRTPSecurityMode_Base *)securityParms;
  if (srtp == NULL)
    return FALSE;

  // get the inbound and outbound SSRC from the SRTP parms and into the RTP session
  srtp->GetOutgoingSSRC(syncSourceOut);
  srtp->GetIncomingSSRC(syncSourceIn);

  return OpalSRTP_UDP::Open(localAddress, portBase, portMax, ipTypeOfService, stun, rtpqos);
}


RTP_UDP::SendReceiveStatus LibSRTP_UDP::OnSendData(RTP_DataFrame & frame)
{
  SendReceiveStatus stat = RTP_UDP::OnSendData(frame);
  if (stat != e_ProcessPacket)
    return stat;

  LibSRTPSecurityMode_Base * srtp = (LibSRTPSecurityMode_Base *)securityParms;

  int len = frame.GetHeaderSize() + frame.GetPayloadSize();
  frame.SetPayloadSize(len + SRTP_MAX_TRAILER_LEN);
  err_status_t err = ::srtp_protect(srtp->outboundSession, frame.GetPointer(), &len);
  if (err != err_status_ok)
    return RTP_Session::e_IgnorePacket;
  frame.SetPayloadSize(len - frame.GetHeaderSize());
  return e_ProcessPacket;
}

RTP_UDP::SendReceiveStatus LibSRTP_UDP::OnReceiveData(RTP_DataFrame & frame)
{
  LibSRTPSecurityMode_Base * srtp = (LibSRTPSecurityMode_Base *)securityParms;

  int len = frame.GetHeaderSize() + frame.GetPayloadSize();
  err_status_t err = ::srtp_unprotect(srtp->inboundSession, frame.GetPointer(), &len);
  if (err != err_status_ok)
    return RTP_Session::e_IgnorePacket;
  frame.SetPayloadSize(len - frame.GetHeaderSize());

  return RTP_UDP::OnReceiveData(frame);
}

RTP_UDP::SendReceiveStatus LibSRTP_UDP::OnSendControl(RTP_ControlFrame & frame, PINDEX & transmittedLen)
{
  SendReceiveStatus stat = RTP_UDP::OnSendControl(frame, transmittedLen);
  if (stat != e_ProcessPacket)
    return stat;

  frame.SetMinSize(transmittedLen + SRTP_MAX_TRAILER_LEN);
  int len = transmittedLen;

  LibSRTPSecurityMode_Base * srtp = (LibSRTPSecurityMode_Base *)securityParms;

  err_status_t err = ::srtp_protect_rtcp(srtp->outboundSession, frame.GetPointer(), &len);
  if (err != err_status_ok)
    return RTP_Session::e_IgnorePacket;
  transmittedLen = len;

  return e_ProcessPacket;
}

RTP_UDP::SendReceiveStatus LibSRTP_UDP::OnReceiveControl(RTP_ControlFrame & frame)
{
  LibSRTPSecurityMode_Base * srtp = (LibSRTPSecurityMode_Base *)securityParms;

  int len = frame.GetSize();
  err_status_t err = ::srtp_unprotect_rtcp(srtp->inboundSession, frame.GetPointer(), &len);
  if (err != err_status_ok)
    return RTP_Session::e_IgnorePacket;
  frame.SetSize(len);

  return RTP_UDP::OnReceiveControl(frame);
}

///////////////////////////////////////////////////////

#endif // OPAL_HAS_LIBSRTP

#endif // OPAL_SRTP
