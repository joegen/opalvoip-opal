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
#include <im/im.h>
#include <rtp/rtp.h>

#define new PNEW

#if OPAL_IM_CAPABILITY

OPAL_INSTANTIATE_MEDIATYPE(im, OpalIMMediaType);

/////////////////////////////////////////////////////////////////////////////

OpalIMMediaType::OpalIMMediaType()
  : OpalMediaTypeDefinition("im", "message", 5, true)
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


#define DECLARE_MSRP_FORMAT(title, encoding) \
class IM##title##MSRPMediaType : public MSRPMediaType \
{ \
}; \
static PFactory<MSRPMediaType>::Worker<IM##title##MSRPMediaType> worker_##IM##title##MSRPMediaType(encoding, true); \

/////////////////////////////////////////////////////////////////////////////

DECLARE_MSRP_FORMAT(Text, "text/plain");
DECLARE_MSRP_FORMAT(CPIM, "message/cpim");
DECLARE_MSRP_FORMAT(HTML, "message/html");

const OpalMediaFormat & GetOpalIMMSRP() 
{ 
  static class IMMSRPMediaFormat : public OpalMediaFormat { 
    public: 
      IMMSRPMediaFormat() 
        : OpalMediaFormat("IM-MSRP", 
                          "im", 
                          RTP_DataFrame::MaxPayloadType, 
                          "+", 
                          false,  
                          1440, 
                          512, 
                          0, 
                          0) 
      { 
        PFactory<MSRPMediaType>::KeyList_T types = PFactory<MSRPMediaType>::GetKeyList();
        PFactory<MSRPMediaType>::KeyList_T::iterator r;

        PString acceptTypes;
        for (r = types.begin(); r != types.end(); ++r) {
          if (!acceptTypes.IsEmpty())
            acceptTypes += " ";
          acceptTypes += *r;
        }
        
        OpalMediaOption * acceptOption = new OpalMediaOptionString("Accept Types", false, acceptTypes);
        acceptOption->SetMerge(OpalMediaOption::NoMerge);
        AddOption(acceptOption);
      } 
  } const f; 
  return f; 
} 

//////////////////////////////////////////////////////////////////////////////////////////

OpalIMMediaStream::OpalIMMediaStream(
      OpalConnection & conn,
      const OpalMediaFormat & mediaFormat, ///<  Media format for stream
      unsigned sessionID,                  ///<  Session number for stream
      bool isSource                       ///<  Is a source stream
    )
  : OpalMediaStream(conn, mediaFormat, sessionID, isSource)
{
}

#endif // OPAL_IM_CAPABILITY
