/*
 * im_mf.cxx
 *
 * Instant Messaging Media Format descriptions
 *
 * Open Phone Abstraction Library
 *
 * Copyright (c) 2008 Post Increment
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
 * The Original Code is Open Phone Abstraction Library
 *
 * The Initial Developer of the Original Code is Post Increment
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision: 21406 $
 * $Author: csoutheren $
 * $Date: 2008-10-22 22:59:46 +1100 (Wed, 22 Oct 2008) $
 */

#include <ptlib.h>
#include <opal/buildopts.h>

#include <opal/mediafmt.h>
#include <opal/connection.h>
#include <im/msrp.h>
#include <rtp/rtp.h>

#define new PNEW

#if OPAL_IM_CAPABILITY

OPAL_INSTANTIATE_MEDIATYPE(im, OpalIMMediaType);

/////////////////////////////////////////////////////////////////////////////

OpalIMMediaType::OpalIMMediaType()
  : OpalMediaTypeDefinition("im", "message", 5)
{
}

PString OpalIMMediaType::GetRTPEncoding() const
{
  return "";
}

RTP_UDP * OpalIMMediaType::CreateRTPSession(OpalRTPConnection & , unsigned /*sessionID*/, bool /*remoteIsNAT*/)
{
  return NULL;
}

bool OpalIMMediaType::UsesRTP() const
{ 
  return false; 
}

OpalMediaSession * OpalIMMediaType::CreateMediaSession(OpalConnection & conn, unsigned sessionID) const
{
  // as this is called in the constructor of an OpalConnection descendant, 
  // we can't use a virtual function on OpalConnection

#if OPAL_IM_CAPABILITY
  if (conn.GetPrefixName() *= "sip")
    return new OpalMSRPMediaSession(conn, sessionID);
#endif

  return NULL;
}

/////////////////////////////////////////////////////////////////////////////

#define DECLARE_IM_FORMAT(title, name, encoding) \
const OpalMediaFormat & GetOpalIM##title() \
{ \
  static class IM##title##MediaFormat : public OpalMediaFormat { \
    public: \
      IM##title##MediaFormat() \
        : OpalMediaFormat(name, \
                          "im", \
                          RTP_DataFrame::MaxPayloadType, \
                          encoding, \
                          false,  \
                          1440, \
                          512, \
                          0, \
                          0) \
      { } \
  } const f; \
  return f; \
} 

DECLARE_IM_FORMAT(Text, "IM-Text", "+text/plain");
DECLARE_IM_FORMAT(CPIM, "IM-CPIM", "+message/cpim");
DECLARE_IM_FORMAT(HTML, "IM-HTML", "+message/html");

#endif // OPAL_IM_CAPABILITY
