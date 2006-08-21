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
 * Revision 1.1  2006/08/21 06:19:28  csoutheren
 * Added placeholders for SRTP implementation
 *
 */

#define OPAL_HAS_SRTP 1

#ifdef OPAL_HAS_SRTP

#include <srtp/include/srtp.h>

class SRTPSession : public PObject
{
  public:
    class Policy {
      public:
        Policy()
        { 
          crypto_policy_set_rtp_default(&policy.rtp); 
          crypto_policy_set_rtcp_default(&policy.rtcp); 
        }
      protected:
        srtp_policy_t policy;
    };

    SRTPSession(DWORD ssrc, PBYTEArray & key, Policy & policy)
    { }

  protected:
    srtp_t session;
};

#endif