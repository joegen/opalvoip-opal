/*
 * zrtpudp.h
 *
 * ZRTP protocol handler
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
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#ifndef OPAL_RTP_ZRTPUDP_H
#define OPAL_RTP_ZRTPUDP_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <ptlib.h>

#include <opal/buildopts.h>

#include <opal/rtpconn.h>


struct zrtp_profile_t;
struct zrtp_conn_ctx_t;
struct zrtp_stream_ctx_t;


class OpalZrtp_UDP : public SecureRTP_UDP
{
    PCLASSINFO(OpalZrtp_UDP, SecureRTP_UDP);
  public:
    OpalZrtp_UDP(
      const PString & encoding,       ///<  identifies initial RTP encoding (RTP/AVP, UDPTL etc)
      bool audio,                     ///<  is audio RTP data
#if OPAL_RTP_AGGREGATE
      PHandleAggregator * aggregator, ///< RTP aggregator
#endif
      unsigned id,                    ///<  Session ID for RTP channel
      PBoolean remoteIsNAT            ///<  TRUE is remote is behind NAT
    );

    virtual ~OpalZrtp_UDP();

    virtual PBoolean WriteZrtpData(RTP_DataFrame & frame);

    virtual SendReceiveStatus OnSendData(RTP_DataFrame & frame);
    virtual SendReceiveStatus OnReceiveData(RTP_DataFrame & frame);
    virtual SendReceiveStatus OnSendControl(RTP_ControlFrame & frame, PINDEX & len);
    virtual SendReceiveStatus OnReceiveControl(RTP_ControlFrame & frame);
    virtual DWORD GetOutgoingSSRC();

  public:
    zrtp_stream_ctx_t *zrtpStream;
};


class OpalZrtpSecurityMode : public OpalSecurityMode
{
  PCLASSINFO(OpalZrtpSecurityMode, OpalSecurityMode);
};

class LibZrtpSecurityMode_Base : public OpalZrtpSecurityMode
{
    PCLASSINFO(LibZrtpSecurityMode_Base, OpalZrtpSecurityMode);
  public:
    LibZrtpSecurityMode_Base();
    ~LibZrtpSecurityMode_Base();

    RTP_UDP * CreateRTPSession(
      OpalRTPConnection & connection,     ///< Connection creating session (may be needed by secure connections)
      const RTP_Session::Params & options ///< Parameters to construct with session.
    );

    PBoolean Open();

    zrtp_profile_t *GetZrtpProfile();

    zrtp_conn_ctx_t	* zrtpSession;

  protected:
    // last element of each array mush be 0
    void Init(int *sas, int *pk, int *auth, int *cipher, int *hash);
    zrtp_profile_t *profile;
};


#endif // OPAL_RTP_ZRTPUDP_H
