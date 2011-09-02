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
 * $Revision$
 * $Author$
 * $Date$
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

#if OPAL_SIP
#include <sip/sdp.h>
#endif


const char T140MediaType[] = OPAL_IM_MEDIA_TYPE_PREFIX"t140";


#if OPAL_SIP

/////////////////////////////////////////////////////////
//
//  SDP media description for text media
//

class OpalT140MediaDescription : public SDPRTPAVPMediaDescription
{
  PCLASSINFO(OpalT140MediaDescription, SDPRTPAVPMediaDescription);
  public:
    OpalT140MediaDescription(const OpalTransportAddress & address)
      : SDPRTPAVPMediaDescription(address, T140MediaType)
    {
    }

    virtual PString GetSDPMediaType() const
    {
      return "text";
    }
};

#endif


/////////////////////////////////////////////////////////////////////////////

class OpalT140MediaType : public OpalRTPAVPMediaType 
{
  public:
    OpalT140MediaType()
      : OpalRTPAVPMediaType(T140MediaType, "text|RTP/AVP")
    {
    }
  
#if OPAL_SIP
    SDPMediaDescription * CreateSDPMediaDescription(const OpalTransportAddress & localAddress, OpalMediaSession *) const
    {
      return new OpalT140MediaDescription(localAddress);
    }
#endif
};

OPAL_INSTANTIATE_MEDIATYPE2(im_t140, T140MediaType, OpalT140MediaType);


/////////////////////////////////////////////////////////////////////////////

const OpalMediaFormat & GetOpalT140() 
{ 
  static class T140MediaFormat : public OpalMediaFormat { 
    public: 
      T140MediaFormat() 
        : OpalMediaFormat(OPAL_T140, 
                          T140MediaType, 
                          RTP_DataFrame::DynamicBase, 
                          "t140", 
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


/////////////////////////////////////////////////////////////////////////////

OpalT140RTPFrame::OpalT140RTPFrame(const RTP_DataFrame & frame)
  : RTP_DataFrame(frame)
{
}


OpalT140RTPFrame::OpalT140RTPFrame(const BYTE * data, PINDEX len, bool dynamic)
  : RTP_DataFrame(data, len, dynamic)
{
}


OpalT140RTPFrame::OpalT140RTPFrame()
{
  SetExtension(true);
  SetExtensionSizeDWORDs(0);
  SetPayloadSize(0);
}


OpalT140RTPFrame::OpalT140RTPFrame(const PString & contentType)
{
  SetExtension(true);
  SetExtensionSizeDWORDs(0);
  SetPayloadSize(0);
  SetContentType(contentType);
}


OpalT140RTPFrame::OpalT140RTPFrame(const PString & contentType, const T140String & content)
{
  SetExtension(true);
  SetExtensionSizeDWORDs(0);
  SetPayloadSize(0);
  SetContentType(contentType);
  SetContent(content);
}


void OpalT140RTPFrame::SetContentType(const PString & contentType)
{
  PINDEX newExtensionBytes  = contentType.GetLength();
  PINDEX newExtensionDWORDs = (newExtensionBytes + 3) / 4;
  PINDEX oldPayloadSize = GetPayloadSize();

  // adding an extension adds 4 bytes to the header,
  //  plus the number of 32 bit words needed to hold the extension
  if (!GetExtension()) {
    SetPayloadSize(4 + newExtensionDWORDs + oldPayloadSize);
    if (oldPayloadSize > 0)
      memcpy(GetPayloadPtr() + newExtensionBytes + 4, GetPayloadPtr(), oldPayloadSize);
  }

  // if content type has not changed, nothing to do
  else if (GetContentType() == contentType) 
    return;

  // otherwise copy the new extension in
  else {
    PINDEX oldExtensionDWORDs = GetExtensionSizeDWORDs();
    if (oldPayloadSize != 0) {
      if (newExtensionDWORDs <= oldExtensionDWORDs) {
        memcpy(GetExtensionPtr() + newExtensionBytes, GetPayloadPtr(), oldPayloadSize);
      }
      else {
        SetPayloadSize((newExtensionDWORDs - oldExtensionDWORDs)*4 + oldPayloadSize);
        memcpy(GetExtensionPtr() + newExtensionDWORDs*4, GetPayloadPtr(), oldPayloadSize);
      }
    }
  }
  
  // reset lengths
  SetExtensionSizeDWORDs(newExtensionDWORDs);
  memcpy(GetExtensionPtr(), (const char *)contentType, newExtensionBytes);
  SetPayloadSize(oldPayloadSize);
  if (newExtensionDWORDs*4 > newExtensionBytes)
    memset(GetExtensionPtr() + newExtensionBytes, 0, newExtensionDWORDs*4 - newExtensionBytes);
}


PString OpalT140RTPFrame::GetContentType() const
{
  if (!GetExtension() || (GetExtensionSizeDWORDs() == 0))
    return PString::Empty();

  const char * p = (const char *)GetExtensionPtr();
  return PString(p, strlen(p));
}


void OpalT140RTPFrame::SetContent(const T140String & text)
{
  SetPayloadSize(text.GetSize());
  memcpy(GetPayloadPtr(), (const BYTE *)text, text.GetSize());
}


bool OpalT140RTPFrame::GetContent(T140String & text) const
{
  if (GetPayloadSize() == 0) 
    return false;

  text = T140String((const BYTE *)GetPayloadPtr(), GetPayloadSize());
  return true;
}


bool OpalT140RTPFrame::GetContent(PString & str) const
{
  if (GetPayloadSize() == 0) 
    return false;

  T140String text((const BYTE *)GetPayloadPtr(), GetPayloadSize());
  return text.AsString(str);
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
  OpalIMEndPoint * imEP = connection.GetEndPoint().GetManager().FindEndPointAs<OpalIMEndPoint>(OpalIMEndPoint::Prefix());
  if (imEP == NULL) {
    PTRACE(2, "OpalIM\tCannot find IM endpoint");
    return false;
  }

  OpalT140RTPFrame imFrame(frame);
  PString str;
  if (imFrame.GetContent(str)) {
    OpalIM message;
    message.m_from = connection.GetRemotePartyCallbackURL();
    message.m_to = connection.GetLocalPartyURL();
    message.m_bodies[imFrame.GetContentType()] = str;

    PString error;
    imEP->OnRawMessageReceived(message, &connection, error);
  }

  return true;
}


#endif // OPAL_HAS_RFC4103
