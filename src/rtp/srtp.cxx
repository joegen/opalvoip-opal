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

#include "rtp/srtp.h"

/////////////////////////////////////////////////////////////////////////////////////
//
//  SRTP implementation using Cisco libSRTP
//  See http://srtp.sourceforge.net/srtp.html
//

#if defined(HAS_LIBSRTP)

#pragma comment(lib, LIBSRTP_LIBRARY)


class StaticInitializer
{
  public:
    StaticInitializer()
    { srtp_init(); }
};

static StaticInitializer initializer;

///////////////////////////////////////////////////////

PStringList LibSRTPParms::GetAvailableCrypto() const
{
  return PStringList();
}

PStringList LibSRTPParms::GetAvailableHash() const
{
  return PStringList();
}

LibSRTPParms::LibSRTPParms()
{
  masterKeySet = ssrcSet = FALSE;

  crypto_policy_set_rtp_default(&inboundPolicy.rtp);
  crypto_policy_set_rtcp_default(&inboundPolicy.rtcp);
  inboundPolicy.ssrc.type  = ssrc_any_inbound;
  inboundPolicy.next       = NULL;

  crypto_policy_set_rtp_default(&outboundPolicy.rtp);
  crypto_policy_set_rtcp_default(&outboundPolicy.rtcp);
  outboundPolicy.ssrc.type = ssrc_any_outbound;
  outboundPolicy.next       = NULL;
}

BOOL LibSRTPParms::SetSSRC(DWORD ssrc)
{
  ssrcSet = TRUE;
  inboundPolicy.ssrc.type  = ssrc_specific;
  inboundPolicy.ssrc.value = ssrc;

  outboundPolicy.ssrc.type = ssrc_specific;
  outboundPolicy.ssrc.value = ssrc;

  return TRUE;
}

BOOL LibSRTPParms::GetSSRC(DWORD & ssrc) const
{
  if (!ssrcSet)
    return FALSE;

  ssrc = outboundPolicy.ssrc.value;

  return TRUE;
}

struct AlgMapEntry {
  const char * alg;
  int mode;
};

static AlgMapEntry CryptoAlgMap[] = {
  { "NULL",        NULL_CIPHER },
  { "AES_128_ICM", AES_128_ICM },
  { "SEAL",        SEAL },
  { "AES_128_CBC", AES_128_CBC },
  { "STRONGHOLD",  STRONGHOLD_CIPHER },
  { NULL }
};

static AlgMapEntry HashAlgMap[] = {
  { "NULL",              NULL_AUTH },
  { "UST_TMMHv2",        UST_TMMHv2 },
  { "UST_AES_128_XMAC",  UST_AES_128_XMAC },
  { "SHA1",              HMAC_SHA1 },
  { "STRONGHOLD",        STRONGHOLD_AUTH },
  { NULL }
};

static int MapAlgToMode(AlgMapEntry * entry, const PString & alg)
{
  while (entry->alg != NULL) {
    if (alg *= entry->alg)
      return entry->mode;
    ++entry;
  }
  return -1;
}

BOOL LibSRTPParms::SetCrypto(
          const PBYTEArray & key, 
          const PBYTEArray & /*salt*/,
          const PString & cryptoAlg, 
          const PString & hashAlg 
      )
{
  if (key.GetSize() != SRTP_MASTER_KEY_LEN)
    return FALSE;

  masterKey = key;
  masterKey.MakeUnique();

  inboundPolicy.key = (uint8_t *)masterKey.GetPointer();
  inboundPolicy.key = outboundPolicy.key;

  int cryptoMode;
  if (cryptoAlg.IsEmpty())
    cryptoMode = STRONGHOLD_CIPHER;
  else
    cryptoMode = MapAlgToMode(CryptoAlgMap, cryptoAlg);
  if (cryptoMode < 0)
    return FALSE;

  inboundPolicy.rtp.cipher_type       = 
    inboundPolicy.rtcp.cipher_type    = 
    outboundPolicy.rtp.cipher_type    = 
      outboundPolicy.rtcp.cipher_type = cryptoMode;

  int hashMode;
  if (hashAlg.IsEmpty())
    hashMode = STRONGHOLD_CIPHER;
  else
    hashMode = MapAlgToMode(HashAlgMap, hashAlg);
  if (hashMode < 0)
    return FALSE;

  inboundPolicy.rtp.auth_type       =
      inboundPolicy.rtcp.auth_type  =
      outboundPolicy.rtp.auth_type  = 
      outboundPolicy.rtcp.auth_type = hashMode;

  return TRUE;
}

///////////////////////////////////////////////////////

SRTP_UDP::SRTP_UDP(unsigned sessionId, BOOL remoteIsNAT)
  : RTP_UDP(sessionId, remoteIsNAT)
{
}

SRTP_UDP::~SRTP_UDP()
{
}

RTP_UDP::SendReceiveStatus SRTP_UDP::OnSendData(RTP_DataFrame & frame)
{
  return RTP_UDP::OnSendData(frame);
}

RTP_UDP::SendReceiveStatus SRTP_UDP::OnReceiveData(RTP_DataFrame & frame)
{
  return RTP_UDP::OnReceiveData(frame);
}

#endif // OPAL_HAS_LIBSRTP

