/*
 * t38proto.cxx
 *
 * T.38 protocol handler for SIP
 *
 * Open Phone Abstraction Library
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
 * The Original Code is OPAL
 *
 * The Initial Developer of the Original Code is Post Increment
 *
 * Contributor(s): 
 *
 * $Revision: 19081 $
 * $Author: rjongbloed $
 * $Date: 2007-12-13 15:21:19 +1100 (Thu, 13 Dec 2007) $
 */

#include <ptlib.h>

#include <opal/mediatype.h>
#include <t38/sipt38.h>
#include <t38/t38proto.h>

#define  SDP_MEDIA_TRANSPORT_UDPTL "udptl"

class OpalT38SDPMediaDescription : public SDPMediaDescription
{
  public:
    OpalT38SDPMediaDescription(const OpalTransportAddress & address);
    void SetAttribute(const PString & attr, const PString & value);
    bool PrintFormat(ostream & str) const;
    void AddSDPMediaFormat(const OpalMediaFormat & mediaFormat, RTP_DataFrame::PayloadTypes pt, const char * nteString);
    SDPMediaFormat * CreateMediaFormatByName(const OpalMediaFormat & mediaFormat, const RTP_DataFrame::PayloadTypes & payloadType);

  protected:      
    void CreateSDPMediaFormats(const PStringArray & tokens, PINDEX start);
    void PrintFormats(ostream & strm, WORD port) const;

    PStringToString t38Attributes;
};

SDPMediaDescription * OpalFaxMediaType::CreateSDPMediaDescription(OpalTransportAddress & localAddress)
{
  if (!localAddress.IsEmpty()) 
    return new OpalT38SDPMediaDescription(localAddress);

  PTRACE(2, "SIP\tRefusing to add fax SDP media description with no transport address");
  return NULL;
}

PCaselessString OpalFaxMediaType::GetTransport() const
{
  return SDP_MEDIA_TRANSPORT_UDPTL;
}

////////////////////////////////////////////////////////////////////////////////////

OpalT38SDPMediaDescription::OpalT38SDPMediaDescription(const OpalTransportAddress & address)
  : SDPMediaDescription(address, OpalMediaType::Fax())
{
}

void OpalT38SDPMediaDescription::CreateSDPMediaFormats(const PStringArray & tokens, PINDEX start)
{
  PINDEX i;
  for (i = start; i < tokens.GetSize(); i++) 
    formats.Append(new SDPMediaFormat(RTP_DataFrame::DynamicBase, tokens[i]));
}

void OpalT38SDPMediaDescription::SetAttribute(const PString & attr, const PString & value)
{
  if (attr.Left(3) *= "t38") {
    t38Attributes.SetAt(attr, value);
    return;
  }

  SDPMediaDescription::SetAttribute(attr, value);
}

void OpalT38SDPMediaDescription::PrintFormats(ostream & strm, WORD port) const
{
  PINDEX i;
  for (i = 0; i < formats.GetSize(); i++)
    strm << ' ' << formats[i].GetEncodingName();
  strm << "\r\n";

  // If we have a port of zero, then shutting down SDP stream. No need for anything more
  if (port == 0)
    return;

  // output options
  for (i = 0; i < t38Attributes.GetSize(); i++) 
    strm << "a=" << t38Attributes.GetKeyAt(i) << ":" << t38Attributes.GetDataAt(i) << "\r\n";
}

SDPMediaFormat * OpalT38SDPMediaDescription::CreateMediaFormatByName(const OpalMediaFormat & mediaFormat, const RTP_DataFrame::PayloadTypes & payloadType)
{
  SDPMediaFormat * sdpFormat = CreateMediaFormatByName(mediaFormat, payloadType);

  for (PINDEX i = 0; i < mediaFormat.GetOptionCount(); ++i) {
    const OpalMediaOption & option = mediaFormat.GetOption(i);
    if (option.GetName().Left(3) *= "t38") 
      t38Attributes.SetAt(option.GetName(), option.AsString());
  }

  return sdpFormat;
}
