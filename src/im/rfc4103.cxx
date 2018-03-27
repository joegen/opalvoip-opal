/*
 * rfc4103.cxx
 *
 * Implementation of RFC 4103 RTP Payload for Text Conversation
 *
 * Open Phone Abstraction Library (OPAL)
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
 * The Original Code is Open Phone Abstraction Library.
 *
 * The Initial Developer of the Original Code is Post Increment
 *
 * Contributor(s): ______________________________________.
 *
 */

#ifdef __GNUC__
#pragma implementation "rfc4103.h"
#endif

#include <ptlib.h>

#include <im/rfc4103.h>

#if OPAL_HAS_RFC4103

#include <opal/connection.h>
#include <im/im_ep.h>
#include <im/t140.h>
#include <sdp/sdp.h>


class OpalT140MediaDefinition : public OpalRTPAVPMediaDefinition
{
  public:
    static const char * Name() { return OPAL_IM_MEDIA_TYPE_PREFIX"t140"; }

    OpalT140MediaDefinition()
      : OpalRTPAVPMediaDefinition(Name())
    {
    }

#if OPAL_SDP
    virtual PString GetSDPMediaType() const { static PConstCaselessString const s("text"); return s; }
    virtual SDPMediaDescription * CreateSDPMediaDescription(const OpalTransportAddress & localAddress) const
    {
      return new SDPRTPAVPMediaDescription(localAddress, GetMediaType());
    }
#endif // OPAL_SDP
};

OPAL_MEDIATYPE(OpalT140Media);


/////////////////////////////////////////////////////////////////////////////

const OpalMediaFormat & GetOpalT140() 
{ 
  class OpalT140MediaFormatInternal : public OpalMediaFormatInternal { 
    public: 
      OpalT140MediaFormatInternal()
        : OpalMediaFormatInternal(OPAL_T140,
                                  OpalT140MediaType(),
                                  RTP_DataFrame::DynamicBase,
                                  "t140",
                                  false,
                                  1440,
                                  512,
                                  0,
                                  1000)    // as defined in RFC 4103
      { 
        SetOptionString(OpalMediaFormat::DescriptionOption(), "ITU-T T.140 (RFC 4103) Instant Messaging");
      } 
  };
  static OpalMediaFormatStatic<OpalMediaFormat> T140(new OpalT140MediaFormatInternal);
  return T140; 
} 


//////////////////////////////////////////////////////////////////////////////////////////

OpalT140MediaStream::OpalT140MediaStream(OpalConnection & conn,
                              const OpalMediaFormat & mediaFormat,
                                           unsigned   sessionID,
                                               bool   isSource)
  : OpalMediaStream(conn, mediaFormat, sessionID, isSource)
{
}


bool OpalT140MediaStream::ReadPacket(RTP_DataFrame & /*packet*/)
{
  PAssertAlways("Cannot ReadData from OpalT140MediaStream");
  return false;
}


bool OpalT140MediaStream::WritePacket(RTP_DataFrame & frame)
{
  OpalIMEndPoint * imEP = m_connection.GetEndPoint().GetManager().FindEndPointAs<OpalIMEndPoint>(OpalIMEndPoint::Prefix());
  if (imEP == NULL) {
    PTRACE(2, "OpalIM\tCannot find IM endpoint");
    return false;
  }

  OpalT140RTPFrame imFrame(frame);
  PString str;
  if (imFrame.GetContent(str)) {
    OpalIM message;
    message.m_from = m_connection.GetRemotePartyURL();
    message.m_to = m_connection.GetLocalPartyURL();
    message.m_bodies[imFrame.GetContentType()] = str;

    PString error;
    imEP->OnRawMessageReceived(message, &m_connection, error);
  }

  return true;
}


#endif // OPAL_HAS_RFC4103
