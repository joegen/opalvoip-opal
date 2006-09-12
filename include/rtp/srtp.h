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
 * Revision 1.2.2.2  2006/09/12 07:06:58  csoutheren
 * More implementation of SRTP and general call security
 *
 * Revision 1.2.2.1  2006/09/08 06:23:28  csoutheren
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

#ifndef __OPAL_SRTP_H
#define __OPAL_SRTP_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <ptlib.h>
#include <opal/buildopts.h>
#include <rtp/rtp.h>
#include <opal/connection.h>

#if OPAL_SRTP

namespace PWLibStupidLinkerHacks {
  extern int libSRTPLoader;
};

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

class OpalSRTP_UDP;

class OpalSRTPSecurityMode : public OpalSecurityMode
{
  PCLASSINFO(OpalSRTPSecurityMode, OpalSecurityMode);
  public:
    virtual BOOL SetKey(const PBYTEArray & key) = 0;
    virtual BOOL SetKey(const PBYTEArray & key, const PBYTEArray & salt) = 0;

    virtual BOOL GetKey(PBYTEArray & key) = 0;
    virtual BOOL GetKey(PBYTEArray & key, PBYTEArray & salt) = 0;

    virtual BOOL SetSSRC(DWORD ssrc) = 0;
    virtual BOOL GetSSRC(DWORD & ssrc) const = 0;

    virtual OpalSRTP_UDP * CreateRTPSession(
      unsigned id,          ///<  Session ID for RTP channel
      BOOL remoteIsNAT      ///<  TRUE is remote is behind NAT
    ) = 0;
};

////////////////////////////////////////////////////////////////////
//
//  this class implements SRTP over UDP
//

class OpalSRTP_UDP : public RTP_UDP
{
  PCLASSINFO(OpalSRTP_UDP, RTP_UDP);
  public:
    OpalSRTP_UDP(
     unsigned id,                  ///<  Session ID for RTP channel
      BOOL remoteIsNAT,            ///<  TRUE is remote is behind NAT
      OpalSecurityMode * srtpParms ///<  Paramaters to use for SRTP
    );

    ~OpalSRTP_UDP();

    virtual SendReceiveStatus OnSendData   (RTP_DataFrame & frame) = 0;
    virtual SendReceiveStatus OnReceiveData(RTP_DataFrame & frame) = 0;

    virtual OpalSRTPSecurityMode * GetSRTPParms() const
    { return srtpParms; }

  protected:
    OpalSRTPSecurityMode * srtpParms;
};

#endif // OPAL_SRTP

#endif // __OPAL_SRTP_H
