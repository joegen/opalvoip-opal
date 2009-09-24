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
 * $Revision$
 * $Author$
 * $Date$
 */

#include <ptlib.h>
#include <opal/buildopts.h>

#if OPAL_HAS_IM

#include <opal/mediafmt.h>
#include <opal/connection.h>
#include <opal/patch.h>
#include <im/im.h>
#include <im/msrp.h>
#include <im/sipim.h>
#include <im/rfc4103.h>
#include <rtp/rtp.h>

#define new PNEW

/////////////////////////////////////////////////////////////////////////////

#if OPAL_HAS_MSRP

OPAL_INSTANTIATE_MEDIATYPE(msrp, OpalMSRPMediaType);

/////////////////////////////////////////////////////////////////////////////


#define DECLARE_MSRP_ENCODING(title, encoding) \
class IM##title##OpalMSRPEncoding : public OpalMSRPEncoding \
{ \
}; \
static PFactory<OpalMSRPEncoding>::Worker<IM##title##OpalMSRPEncoding> worker_##IM##title##OpalMSRPEncoding(encoding, true); \

/////////////////////////////////////////////////////////////////////////////

DECLARE_MSRP_ENCODING(Text, "text/plain");
DECLARE_MSRP_ENCODING(CPIM, "message/cpim");
DECLARE_MSRP_ENCODING(HTML, "message/html");

const OpalMediaFormat & GetOpalMSRP() 
{ 
  static class IMMSRPMediaFormat : public OpalMediaFormat { 
    public: 
      IMMSRPMediaFormat() 
        : OpalMediaFormat(OPAL_MSRP, 
                          "msrp", 
                          RTP_DataFrame::MaxPayloadType, 
                          "+", 
                          false,  
                          1440, 
                          512, 
                          0, 
                          1000)            // as defined in RFC 4103 - good as anything else   
      { 
        PFactory<OpalMSRPEncoding>::KeyList_T types = PFactory<OpalMSRPEncoding>::GetKeyList();
        PFactory<OpalMSRPEncoding>::KeyList_T::iterator r;

        PString acceptTypes;
        for (r = types.begin(); r != types.end(); ++r) {
          if (!acceptTypes.IsEmpty())
            acceptTypes += " ";
          acceptTypes += *r;
        }
        
        OpalMediaOption * option = new OpalMediaOptionString("Accept Types", false, acceptTypes);
        option->SetMerge(OpalMediaOption::NoMerge);
        AddOption(option);

        option = new OpalMediaOptionString("Path", false, "");
        option->SetMerge(OpalMediaOption::NoMerge);
        AddOption(option);
      } 
  } const f; 
  return f; 
} 

#endif // OPAL_HAS_MSRP


//////////////////////////////////////////////////////////////////////////////////////////

#if OPAL_HAS_SIPIM

OPAL_INSTANTIATE_MEDIATYPE2(sipim, "sip-im", OpalSIPIMMediaType);

const OpalMediaFormat & GetOpalSIPIM() 
{ 
  static class IMSIPMediaFormat : public OpalMediaFormat { 
    public: 
      IMSIPMediaFormat() 
        : OpalMediaFormat(OPAL_SIPIM, 
                          "sip-im", 
                          RTP_DataFrame::MaxPayloadType, 
                          "+", 
                          false,  
                          1440, 
                          512, 
                          0, 
                          1000)     // as defined in RFC 4103 - good as anything else
      { 
        OpalMediaOption * option = new OpalMediaOptionString("URL", false, "");
        option->SetMerge(OpalMediaOption::NoMerge);
        AddOption(option);
      } 
  } const f; 
  return f; 
} 

#endif // OPAL_HAS_SIPIM

//////////////////////////////////////////////////////////////////////////////////////////

const OpalMediaFormat & GetOpalT140() 
{ 
  static class T140MediaFormat : public OpalMediaFormat { 
    public: 
      T140MediaFormat() 
        : OpalMediaFormat(OPAL_T140, 
                          "t140", 
                          RTP_DataFrame::DynamicBase, 
                          "text", 
                          false,  
                          1440, 
                          512, 
                          0, 
                          1000)    // as defined in RFC 4103
      { 
      } 
  } const f; 
  return f; 
} 

OPAL_INSTANTIATE_MEDIATYPE(t140, OpalT140MediaType);

//////////////////////////////////////////////////////////////////////////////////////////

OpalIMMediaStream::OpalIMMediaStream(
      OpalConnection & conn,
      const OpalMediaFormat & mediaFormat, ///<  Media format for stream
      unsigned sessionID,                  ///<  Session number for stream
      bool isSource                        ///<  Is a source stream
    )
  : OpalMediaStream(conn, mediaFormat, sessionID, isSource)
  , rfc4103(mediaFormat)
{

}

bool OpalIMMediaStream::PushIM(const T140String & text)
{
  RTP_DataFrameList frames = rfc4103.ConvertToFrames(text);
  for (PINDEX i = 0; i < frames.GetSize(); ++i)
    if (!PushIM(frames[i]))
      return false;
  return true;
}


bool OpalIMMediaStream::PushIM(RTP_DataFrame & frame)
{
  return GetPatch()->PushFrame(frame);
}


#endif // OPAL_HAS_IM
