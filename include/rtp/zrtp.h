/*
 * zrtp.h
 *
 * ZRTP protocol handler
 *
 * OPAL Library
 *
 * Copyright (C) 2007 Post Increment
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

#ifndef __OPAL_ZRTP_H
#define __OPAL_ZRTP_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#ifndef _PTLIB_H
#include <ptlib.h>
#endif

#include <opal/buildopts.h>

#if defined(OPAL_ZRTP)

#include <rtp/rtp.h>
#include <opal/connection.h>

namespace PWLibStupidLinkerHacks {
  extern int libZRTPLoader;
};

extern "C" {
struct zrtp_global_ctx;
};

////////////////////////////////////////////////////////////////////
//
//  this class holds the parameters required for a ZRTP session
//

class OpalZRTPSecurityMode : public OpalSecurityMode
{
  PCLASSINFO(OpalZRTPSecurityMode, OpalSecurityMode);
};


////////////////////////////////////////////////////////////////////
//
//  this class implements ZRTP over UDP
//

class OpalZRTP_UDP : public SecureRTP_UDP
{
  PCLASSINFO(OpalZRTP_UDP, SecureRTP_UDP);
  public:
    OpalZRTP_UDP(
      PHandleAggregator * _aggregator,   ///< handle aggregator
      unsigned id,                       ///<  Session ID for RTP channel
      PBoolean remoteIsNAT                  ///<  PTrue is remote is behind NAT
    );

    ~OpalZRTP_UDP();

    SendReceiveStatus OnSendData   (RTP_DataFrame & frame);
    SendReceiveStatus OnReceiveData(RTP_DataFrame & frame);
    SendReceiveStatus OnSendControl(RTP_ControlFrame & frame, PINDEX & len);
    SendReceiveStatus OnReceiveControl(RTP_ControlFrame & frame);
};

#endif // OPAL_ZRTP

#endif // __OPAL_ZRTP_H

