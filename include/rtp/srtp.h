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
 * $Log: srtp.h,v $
 * Revision 1.2  2006/09/05 06:18:23  csoutheren
 * Start bringing in SRTP code for libSRTP
 *
 * Revision 1.1  2006/08/21 06:19:28  csoutheren
 * Added placeholders for SRTP implementation
 *
 */

#ifndef __OPAL_SRTP_H
#define __OPAL_SRTP_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <ptlib.h>
#include <rtp/rtp.h>

////////////////////////////////////////////////////////////////////
//
//  this class holds the parameters required for an SRTP session
//
//  Crypto functons are identified by key strings that may be elements
//  PFactory<PCypher>. The following crypto functions will usually be implemented
//        NULL, AES_128_ICM, AES_128_CBC
//  The special string STRONGHOLD maps to the strongest cypher available
//
//  Hash functons are identified by keys that may be elements
//  PFactory<PMessageDigest>. The following hash functions will usually be implemented
//        NULL, SHA1
//  The special string STRONGHOLD maps to the strongest hash available
//

class SRTPParms : public PObject
{
  PCLASSINFO(SRTPParms, PObject);
  public:
    SRTPParms();

    virtual BOOL SetSSRC(DWORD ssrc) = 0;
    virtual BOOL GetSSRC(DWORD & ssrc) const = 0;

    virtual BOOL SetCrypto(
          const PBYTEArray & key, 
          const PBYTEArray & salt
    )
    {
      return SetCrypto(key, salt, "STRONGHOLD", "STRONGHOLD");
    }
    virtual BOOL SetCrypto(
          const PBYTEArray & key, 
          const PBYTEArray & salt,
          const PString & cryptoAlg, 
          const PString & hashAlg 
    ) = 0;

    virtual PStringList GetAvailableCrypto() const = 0;
    virtual PStringList GetAvailableHash() const = 0;
};

////////////////////////////////////////////////////////////////////
//
//  implement SRTP via libSRTP
//

#if defined(HAS_LIBSRTP) 

#define NO_64BIT_MATH
#include <srtp/include/srtp.h>

class LibSRTPParms : public SRTPParms
{
  PCLASSINFO(LibSRTPParms, SRTPParms);
  public:
    LibSRTPParms();

    BOOL SetSSRC(DWORD ssrc);
    BOOL GetSSRC(DWORD & ssrc) const;

    BOOL SetCrypto(
          const PBYTEArray & key, 
          const PBYTEArray & salt,
          const PString & cryptoAlg, 
          const PString & hashAlg 
    );

    PStringList GetAvailableCrypto() const;
    PStringList GetAvailableHash() const;

  protected:
    BOOL ssrcSet;
    BOOL masterKeySet;
    PBYTEArray masterKey;
    srtp_policy_t inboundPolicy;
    srtp_policy_t outboundPolicy;
};

#endif // defined(HAS_LIBSRTP)

////////////////////////////////////////////////////////////////////
//
//  this class implements SRTP over UDP
//

class SRTP_UDP : public RTP_UDP
{
  PCLASSINFO(SRTP_UDP, RTP_UDP);
  public:
    SRTP_UDP(
     unsigned id,        ///<  Session ID for RTP channel
      BOOL remoteIsNAT   ///<  TRUE is remote is behind NAT
    );

    ~SRTP_UDP();

    virtual SendReceiveStatus OnSendData   (RTP_DataFrame & frame);
    virtual SendReceiveStatus OnReceiveData(RTP_DataFrame & frame);
};

#endif // __OPAL_SRTP_H
