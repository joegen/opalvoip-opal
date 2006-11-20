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
 * Revision 1.5  2006/11/20 03:37:12  csoutheren
 * Allow optional inclusion of RTP aggregation
 *
 * Revision 1.4  2006/10/24 04:18:28  csoutheren
 * Added support for encrypted RTCP
 *
 * Revision 1.3  2006/09/28 07:42:17  csoutheren
 * Merge of useful SRTP implementation
 *
 * Revision 1.2.2.3  2006/09/12 07:47:15  csoutheren
 * Changed to use seperate incoming and outgoing keys
 *
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

class OpalSRTPSecurityMode : public OpalSecurityMode
{
  PCLASSINFO(OpalSRTPSecurityMode, OpalSecurityMode);
  public:
    struct KeySalt {
      KeySalt()                                                       { }
      KeySalt(const BYTE * data, PINDEX dataLen) : key(data, dataLen) { }
      PBYTEArray key;
      PBYTEArray salt;
    };
    virtual BOOL SetOutgoingKey(const KeySalt & key) = 0;
    virtual BOOL GetOutgoingKey(KeySalt & key) const = 0;
    virtual BOOL SetOutgoingSSRC(DWORD ssrc) = 0;
    virtual BOOL GetOutgoingSSRC(DWORD & ssrc) const = 0;

    virtual BOOL SetIncomingKey(const KeySalt & key) = 0;
    virtual BOOL GetIncomingKey(KeySalt & key) const = 0;
    virtual BOOL SetIncomingSSRC(DWORD ssrc) = 0;
    virtual BOOL GetIncomingSSRC(DWORD & ssrc) const = 0;

    virtual RTP_UDP * CreateRTPSession(
      PHandleAggregator * _aggregator,   ///< handle aggregator
      unsigned id,                       ///<  Session ID for RTP channel
      BOOL remoteIsNAT                   ///<  TRUE is remote is behind NAT
    ) = 0;

    virtual BOOL Open() = 0;

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
      PHandleAggregator * _aggregator,   ///< handle aggregator
      unsigned id,                       ///<  Session ID for RTP channel
      BOOL remoteIsNAT                  ///<  TRUE is remote is behind NAT
    );

    virtual void SetSecurityMode(OpalSecurityMode * srtpParms);

    ~OpalSRTP_UDP();

    virtual SendReceiveStatus OnSendData   (RTP_DataFrame & frame) = 0;
    virtual SendReceiveStatus OnReceiveData(RTP_DataFrame & frame) = 0;
    virtual SendReceiveStatus OnSendControl(RTP_ControlFrame & frame, PINDEX & len) = 0;
    virtual SendReceiveStatus OnReceiveControl(RTP_ControlFrame & frame) = 0;

    virtual OpalSRTPSecurityMode * GetSRTPParms() const
    { return srtpParms; }

  protected:
    OpalSRTPSecurityMode * srtpParms;
};


////////////////////////////////////////////////////////////////////
//
//  this class implements SRTP using libSRTP
//

#if HAS_LIBSRTP

class LibSRTP_UDP : public OpalSRTP_UDP
{
  PCLASSINFO(LibSRTP_UDP, OpalSRTP_UDP);
  public:
    LibSRTP_UDP(PHandleAggregator * _aggregator,   ///< handle aggregator
                  unsigned int id,                 ///<  Session ID for RTP channel
                  BOOL remoteIsNAT                 ///<  TRUE is remote is behind NAT
    );

    ~LibSRTP_UDP();

    void SetSecurityMode(OpalSecurityMode * _srtpParms);

    virtual SendReceiveStatus OnSendData   (RTP_DataFrame & frame);
    virtual SendReceiveStatus OnReceiveData(RTP_DataFrame & frame);
    virtual SendReceiveStatus OnSendControl(RTP_ControlFrame & frame, PINDEX & len);
    virtual SendReceiveStatus OnReceiveControl(RTP_ControlFrame & frame);
};

#endif // HAS_LIBSRTP


#endif // OPAL_SRTP

#endif // __OPAL_SRTP_H
