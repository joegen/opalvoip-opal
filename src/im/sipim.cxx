/*
 * sipim.cxx
 *
 * Support for SIP session mode IM
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

#include <ptlib.h>
#include <opal/buildopts.h>

#ifdef __GNUC__
#pragma implementation "sipim.h"
#endif

#include <ptclib/pxml.h>

#include <im/im.h>
#include <im/im_ep.h>
#include <im/sipim.h>
#include <im/t140.h>
#include <sip/sipep.h>
#include <sip/sipcon.h>
#include <sip/sdp.h>


#if OPAL_HAS_SIPIM

static PConstCaselessString const ComposingMimeType("application/im-iscomposing+xml");
static PConstCaselessString const DispositionMimeType("message/imdn+xml");


class OpalSIPIMMediaType : public OpalRTPAVPMediaType
{
  public:
    static const char * Name() { return OPAL_IM_MEDIA_TYPE_PREFIX"sip"; } // RFC 3428

    OpalSIPIMMediaType()
      : OpalRTPAVPMediaType(Name())
    {
    }

    static const PCaselessString & GetSDPMediaType();
    static const PCaselessString & GetSDPTransportType();

    virtual bool MatchesSDP(
      const PCaselessString & sdpMediaType,
      const PCaselessString & sdpTransport,
      const PStringArray & /*sdpLines*/,
      PINDEX /*index*/
    );

    SDPMediaDescription * CreateSDPMediaDescription(const OpalTransportAddress & localAddress) const;
};

OPAL_INSTANTIATE_MEDIATYPE(OpalSIPIMMediaType);


/////////////////////////////////////////////////////////
//
//  SDP media description for the SIPIM type
//
//  A new class is needed for "message" due to the following differences
//
//  - the SDP type is "message"
//  - the transport is "sip"
//  - the format list is a SIP URL
//

class SDPSIPIMMediaDescription : public SDPMediaDescription
{
  PCLASSINFO(SDPSIPIMMediaDescription, SDPMediaDescription);
  public:
    SDPSIPIMMediaDescription(const OpalTransportAddress & address);
    SDPSIPIMMediaDescription(
      const OpalTransportAddress & address,
      const OpalTransportAddress & transportAddr,
      const PString & fromURL
    );

    PCaselessString GetSDPTransportType() const
    {
      return OpalSIPIMMediaType::GetSDPTransportType();
    }

    virtual SDPMediaDescription * CreateEmpty() const
    {
      return new SDPSIPIMMediaDescription(OpalTransportAddress());
    }

    virtual PString GetSDPMediaType() const 
    {
      return OpalSIPIMMediaType::GetSDPMediaType();
    }

    virtual PString GetSDPPortList() const;

    virtual void CreateSDPMediaFormats(const PStringArray &);
    virtual void SetAttribute(const PString & attr, const PString & value);
    virtual void ProcessMediaOptions(SDPMediaFormat & sdpFormat, const OpalMediaFormat & mediaFormat);
    virtual void AddMediaFormat(const OpalMediaFormat & mediaFormat);

    virtual OpalMediaFormatList GetMediaFormats() const;

    // CreateSDPMediaFormat is used for processing format lists. MSRP always contains only "*"
    virtual SDPMediaFormat * CreateSDPMediaFormat(const PString & ) { return NULL; }

    // FindFormat is used only for rtpmap and fmtp, neither of which are used for MSRP
    virtual SDPMediaFormat * FindFormat(PString &) const { return NULL; }

  protected:
    OpalTransportAddress m_transportAddress;
    PString              m_fromURL;
};


////////////////////////////////////////////////////////////////////////////

const PCaselessString & OpalSIPIMMediaType::GetSDPMediaType()     { static PConstCaselessString const s("message"); return s; }
const PCaselessString & OpalSIPIMMediaType::GetSDPTransportType() { static PConstCaselessString const s("sip"); return s; }

bool OpalSIPIMMediaType::MatchesSDP(const PCaselessString & sdpMediaType,
                                    const PCaselessString & sdpTransport,
                                    const PStringArray & /*sdpLines*/,
                                    PINDEX /*index*/)
{
  return sdpMediaType == GetSDPMediaType() && sdpTransport == GetSDPTransportType();
}

SDPMediaDescription * OpalSIPIMMediaType::CreateSDPMediaDescription(const OpalTransportAddress & localAddress) const
{
  return new SDPSIPIMMediaDescription(localAddress);
}

////////////////////////////////////////////////////////////////////////////

const OpalMediaFormat & GetOpalSIPIM() 
{ 
  static class IMSIPMediaFormat : public OpalMediaFormat { 
    public: 
      IMSIPMediaFormat() 
        : OpalMediaFormat(OPAL_SIPIM, 
                          OpalSIPIMMediaType::Name(),
                          RTP_DataFrame::MaxPayloadType, 
                          "+", 
                          false,  
                          1440, 
                          512, 
                          0, 
                          1000)     // as defined in RFC 4103 - good as anything else
      { 
        OpalMediaOptionString * option = new OpalMediaOptionString("URL", false, "");
        option->SetMerge(OpalMediaOption::NoMerge);
        AddOption(option);
      } 
  } const f; 
  return f; 
} 


///////////////////////////////////////////////////////////////////////////////////////////

SDPSIPIMMediaDescription::SDPSIPIMMediaDescription(const OpalTransportAddress & address)
  : SDPMediaDescription(address, OpalSIPIMMediaType::Name())
{
  SetDirection(SDPMediaDescription::SendRecv);
}


SDPSIPIMMediaDescription::SDPSIPIMMediaDescription(const OpalTransportAddress & address,
                                                   const OpalTransportAddress & transportAddr,
                                                   const PString & fromURL)
  : SDPMediaDescription(address, OpalSIPIMMediaType::Name())
  , m_transportAddress(transportAddr)
  , m_fromURL(fromURL)
{
  SetDirection(SDPMediaDescription::SendRecv);
}


void SDPSIPIMMediaDescription::CreateSDPMediaFormats(const PStringArray &)
{
  formats.Append(new SDPMediaFormat(*this, OpalSIPIM));
}


PString SDPSIPIMMediaDescription::GetSDPPortList() const
{ 
  PString str = m_fromURL;
  if (str.Find('@') == P_MAX_INDEX)
    str += '@' + m_transportAddress.GetHostName(true);
  return str;
}


void SDPSIPIMMediaDescription::SetAttribute(const PString & /*attr*/, const PString & /*value*/)
{
}


void SDPSIPIMMediaDescription::ProcessMediaOptions(SDPMediaFormat & /*sdpFormat*/, const OpalMediaFormat & /*mediaFormat*/)
{
}


OpalMediaFormatList SDPSIPIMMediaDescription::GetMediaFormats() const
{
  OpalMediaFormat sipim(OpalSIPIM);
  sipim.SetOptionString("URL", m_fromURL);

  PTRACE(4, "SIPIM\tNew format is " << setw(-1) << sipim);

  OpalMediaFormatList fmts;
  fmts += sipim;
  return fmts;
}

void SDPSIPIMMediaDescription::AddMediaFormat(const OpalMediaFormat & mediaFormat)
{
  if (!mediaFormat.IsTransportable() || !mediaFormat.IsValidForProtocol("sip") || mediaFormat.GetMediaType() != OpalSIPIMMediaType::Name()) {
    PTRACE(4, "SIPIM\tSDP not including " << mediaFormat << " as it is not a valid SIPIM format");
    return;
  }

  SDPMediaFormat * sdpFormat = new SDPMediaFormat(*this, mediaFormat);
  ProcessMediaOptions(*sdpFormat, mediaFormat);
  AddSDPMediaFormat(sdpFormat);
}


////////////////////////////////////////////////////////////////////////////////////////////

static PFactory<OpalIMContext>::Worker<OpalSIPIMContext> static_OpalSIPContext("sip");

OpalSIPIMContext::OpalSIPIMContext()
{
  m_conversationId += ';' + SIPURL::GenerateTag();
  m_attributes.Set("acceptable-content-types", "text/plain\ntext/html\napplication/im-iscomposing+xml");
  m_rxCompositionIdleTimeout.SetNotifier(PCREATE_NOTIFIER(OnRxCompositionIdleTimer));
  m_txCompositionIdleTimeout.SetNotifier(PCREATE_NOTIFIER(OnTxCompositionIdleTimer));
}


bool OpalSIPIMContext::Open(bool byRemote)
{
  if (byRemote)
    return true;

  if (m_call != NULL) {
    PSafePtr<SIPConnection> conn = m_call->GetConnectionAs<SIPConnection>();
    if (conn == NULL) {
      PTRACE(2, "SIPIM\tNo SIP connection");
      return false;
    }

    if (conn->DoesRemoteAllowMethod(SIP_PDU::Method_MESSAGE))
      return true;

    PTRACE(2, "SIPIM\tMESSAGE not allowed");
    return false;
  }

  OpalManager & manager = m_endpoint->GetManager();

  OpalMediaFormatList list = m_endpoint->GetMediaFormats();
  list.Remove(manager.GetMediaFormatMask());
  list.Reorder(manager.GetMediaFormatOrder());
  if (list.IsEmpty()) {
    PTRACE(2, "SIPIM\tNo media formats available");
    return false;
  }

  if (list.front() == OpalSIPIM) {
    PTRACE(3, "SIPIM\tDefault RFC 3428 pager mode for SIP");
    return true;
  }

  OpalConnection::StringOptions options;
  options.ExtractFromURL(m_remoteURL);
  options.Set(OPAL_OPT_CALLING_PARTY_URL, m_localURL);
  options.Set(OPAL_OPT_AUTO_START, list.front().GetMediaType() + ":exclusive");

  m_call = manager.SetUpCall(m_endpoint->GetPrefixName()+":*", m_remoteURL, this, 0, &options);
  if (m_call == NULL)
    return list.HasFormat(OpalSIPIM); // Fallback to MESSAGE commands

  m_weStartedCall = true;
  return true;
}


void OpalSIPIMContext::OnMESSAGECompleted(SIPEndPoint & endpoint,
                                          const SIPMessage::Params & params,
                                          SIP_PDU::StatusCodes reason)
{
  // RFC3428
  OpalIMEndPoint * imEP = endpoint.GetManager().FindEndPointAs<OpalIMEndPoint>(OpalIMEndPoint::Prefix());
  if (imEP == NULL) {
    PTRACE2(2, &endpoint, "SIPIM\tCannot find IM endpoint");
    return;
  }

  PSafePtr<OpalSIPIMContext> context = PSafePtrCast<OpalIMContext,OpalSIPIMContext>(
                                          imEP->FindContextByIdWithLock(params.m_id));
  if (context == NULL) {
    PTRACE2(2, &endpoint, "SIPIM\tCannot find IM context for \"" << params.m_id << '"');
    return;
  }

  OpalIMContext::DispositionInfo result;
  result.m_conversationId = context->GetID();
  result.m_messageId = params.m_messageId;

  switch (reason) {
    case SIP_PDU::Failure_RequestTimeout:
      result.m_disposition = TransmissionTimeout;
      break;
    case SIP_PDU::Failure_TemporarilyUnavailable:
      result.m_disposition = DestinationUnavailable;
      break;
    default:
      switch (reason/100) {
        case 2:
          result.m_disposition = DispositionAccepted;
          break;
        default:
          result.m_disposition = GenericError;
          break;
      }
  }

  context->InternalOnMessageSent(result);
}


void OpalSIPIMContext::OnReceivedMESSAGE(SIPEndPoint & endpoint,
                                         SIPConnection * connection,
                                         SIP_PDU & request)
{
  // RFC3428
  OpalIMEndPoint * imEP = endpoint.GetManager().FindEndPointAs<OpalIMEndPoint>(OpalIMEndPoint::Prefix());
  if (imEP == NULL) {
    PTRACE2(2, &endpoint, "SIPIM\tCannot find IM endpoint");
    return;
  }

  const SIPMIMEInfo & mime  = request.GetMIME();

  OpalIMContext::MessageDisposition status;
  PString errorInfo;
  {
    SIPURL to(mime.GetTo());
    SIPURL from(mime.GetFrom());

    OpalIM message;
    message.m_conversationId = mime.GetCallID() + ';' + to.GetTag();
    message.m_to             = to;
    message.m_from           = from;
    message.m_toAddr         = request.GetTransport()->GetLastReceivedAddress();
    message.m_fromAddr       = request.GetTransport()->GetRemoteAddress();
    message.m_fromName       = from.GetDisplayName();
    message.m_bodies.SetAt(mime.GetContentType(), request.GetEntityBody());
    status = imEP->OnRawMessageReceived(message, connection, errorInfo);
  }

  SIPResponse * response = new SIPResponse(endpoint, request, SIP_PDU::Failure_BadRequest);

  switch (status ) {
    case OpalIMContext::DispositionAccepted:
    case OpalIMContext::DispositionPending:
      response->SetStatusCode(SIP_PDU::Successful_Accepted);
      break;

    case OpalIMContext::UnacceptableContent:
      response->SetStatusCode(SIP_PDU::Failure_UnsupportedMediaType);
      response->GetMIME().SetAccept(errorInfo);
      break;

    default:
      if (status < DispositionErrors)
        response->SetStatusCode(SIP_PDU::Successful_OK);
      break;
  }

  // After this, response is owned by transaction layer and will be deleted there
  response->Send();
}


void OpalSIPIMContext::PopulateParams(SIPMessage::Params & params, const OpalIM & message)
{
  SIPURL from(message.m_from);
  from.SetDisplayName(m_localName);
  from.SetTag(message.m_conversationId.Mid(message.m_conversationId.Find(';')+1));

  params.m_localAddress    = from.AsQuotedString();
  params.m_addressOfRecord = params.m_localAddress;
  params.m_remoteAddress   = message.m_to.AsString();
  params.m_id              = message.m_conversationId;
  params.m_messageId       = message.m_messageId;

  if (message.m_bodies.Contains(PMIMEInfo::TextPlain())) {
    params.m_contentType = PMIMEInfo::TextPlain();
    params.m_body        = message.m_bodies[PMIMEInfo::TextPlain()];
  }
  else {
    params.m_contentType = message.m_bodies.GetKeyAt(0);
    params.m_body        = message.m_bodies.GetDataAt(0);
  }

  if (params.m_contentType != ComposingMimeType) {
      m_txCompositionIdleTimeout.Stop(true);
      m_txCompositionState = CompositionIndicationIdle();
  }
}


OpalIMContext::MessageDisposition OpalSIPIMContext::InternalSendOutsideCall(OpalIM & message)
{
  SIPEndPoint * ep = m_endpoint->GetManager().FindEndPointAs<SIPEndPoint>("sip");
  if (ep == NULL) {
    PTRACE(2, "OpalSIPIMContext\tAttempt to send SIP IM without SIP endpoint");
    return ConversationClosed;
  }

  SIPMessage::Params params;
  PopulateParams(params, message);

  return ep->SendMESSAGE(params) ? DispositionPending : TransportFailure;
}


OpalIMContext::MessageDisposition OpalSIPIMContext::InternalSendInsideCall(OpalIM & message)
{
  if (!m_call->IsEstablished())
    return DispositionPending;

  PSafePtr<SIPConnection> conn = m_call->GetConnectionAs<SIPConnection>();
  if (conn == NULL) {
    PTRACE(2, "OpalSIPIMContext\tAttempt to send SIP IM on non-SIP connection");
    return ConversationClosed;
  }

#if OPAL_HAS_RFC4103
  OpalMediaStreamPtr stream = conn->GetMediaStream(OpalT140.GetMediaType(), false);
  if (stream != NULL) {
    for (PStringToString::iterator it = message.m_bodies.begin(); it != message.m_bodies.end(); ++it) {
      PTRACE(5, "OpalSIPIMContext\tSending T.140 packet");
      OpalT140RTPFrame packet(it->first, T140String(it->second));
      if (!stream->PushPacket(packet))
        return TransportFailure;
    }
    return DispositionPending;
  }
#endif

  SIPMessage::Params params;
  PopulateParams(params, message);

  PSafePtr<SIPTransaction> transaction = new SIPMessage(*conn, params);
  return transaction->Start() ? DispositionPending : TransportFailure;
}


#if P_EXPAT

OpalIMContext::MessageDisposition OpalSIPIMContext::InternalOnCompositionIndication(const OpalIM & message)
{
  //RFC3994
  static PXML::ValidationInfo const CompositionIndicationValidation[] = {
    { PXML::SetDefaultNamespace,        "urn:ietf:params:xml:ns:im-iscomposing" },
    { PXML::ElementName,                "isComposing", },

    { PXML::OptionalElement,                   "state"       },
    { PXML::OptionalElement,                   "lastactive"  },
    { PXML::OptionalElement,                   "contenttype" },
    { PXML::OptionalElementWithBodyMatchingEx, "refresh",    { "[0-9]*" } },

    { PXML::EndOfValidationList }
  };

  PXML xml;
  PString error;
  if (!xml.LoadAndValidate(message.m_bodies[ComposingMimeType], CompositionIndicationValidation, error, PXML::WithNS)) {
    PTRACE(2, "OpalSIPIMContext\tXML error: " << error);
    return InvalidContent;
  }

  PCaselessString newState;

  PXMLElement * element = xml.GetRootElement()->GetElement("state");
  if (element != NULL)
    newState = element->GetData().Trim();

  if (newState == m_rxCompositionState) {
    PTRACE(3, "OpalSIPIMContext\tComposition indication refreshed for \"" << newState << '"');
    return DeliveryOK;
  }

  m_rxCompositionState = newState;

  if (newState == CompositionIndicationIdle())
    m_rxCompositionIdleTimeout.Stop(true);
  else {
    int timeout;
    if ((element = xml.GetRootElement()->GetElement("refresh")) == NULL)
      timeout = 15;
    else {
      timeout = element->GetData().AsInteger();
      if (timeout < 15)
        timeout = 15;
    }

    m_rxCompositionIdleTimeout.SetInterval(0, timeout);
  }

  OnCompositionIndication(CompositionInfo(GetID(), newState));

  return DeliveryOK;
}


OpalIMContext::MessageDisposition OpalSIPIMContext::InternalOnDisposition(const OpalIM & message)
{
  //RFC5438
  static PXML::ValidationInfo const DispositionValidation[] = {
    { PXML::SetDefaultNamespace,        "urn:ietf:params:xml:ns:imdn" },
    { PXML::ElementName,                "imdn", },

    { PXML::RequiredElement,            "message-id" },
    { PXML::RequiredElement,            "datetime"   },
    { PXML::OptionalElement,            "recipient-uri" },
    { PXML::OptionalElement,            "original-recipient-uri" },
    { PXML::OptionalElement,            "subject" },

    { PXML::EndOfValidationList }
  };

  PXML xml;
  PString error;
  if (!xml.LoadAndValidate(message.m_bodies[DispositionMimeType], DispositionValidation, error, PXML::WithNS)) {
    PTRACE(2, "OpalSIPIMContext\tXML error: " << error);
    return InvalidContent;
  }

  PXMLElement * rootElement = xml.GetRootElement();

  DispositionInfo info;

  PXMLElement * element = rootElement->GetElement("message-id");
  if (element != NULL)
    info.m_messageId = element->GetData().AsUnsigned();

  element = rootElement->GetElement("deliveryNotification");
  if (element != NULL)
    info.m_disposition = element->GetElement("delivered") != NULL ? DeliveryOK : DeliveryFailed;

  // Still largely incomplete!!

  OnMessageDisposition(info);
  return DeliveryOK;
}

#else // P_EXPAT

OpalIMContext::MessageDisposition OpalSIPIMContext::InternalOnCompositionIndication(const OpalIM &)
{
  return UnsupportedFeature;
}

OpalIMContext::MessageDisposition OpalSIPIMContext::InternalOnDisposition(const OpalIM &)
{
  return UnsupportedFeature;
}

#endif // P_EXPAT


OpalIMContext::MessageDisposition OpalSIPIMContext::OnMessageReceived(const OpalIM & message)
{
  if (message.m_bodies.Contains(ComposingMimeType))
    return InternalOnCompositionIndication(message);

  if (message.m_bodies.Contains(DispositionMimeType))
    return InternalOnDisposition(message);

  // receipt of text always indicated idle
  m_rxCompositionIdleTimeout.Stop(true);
  if (m_rxCompositionState != CompositionIndicationIdle())
    OnCompositionIndication(CompositionInfo(GetID(), m_rxCompositionState = CompositionIndicationIdle()));

  // forward the text
  return OpalIMContext::OnMessageReceived(message);
}


void OpalSIPIMContext::OnRxCompositionIdleTimer(PTimer &, INT)
{
  OnCompositionIndication(CompositionInfo(GetID(), m_rxCompositionState = CompositionIndicationIdle()));
}


bool OpalSIPIMContext::SendCompositionIndication(const CompositionInfo & info)
{
  int refreshTime = 0;
  if (info.m_state == CompositionIndicationIdle()) {
    if (m_txCompositionState == CompositionIndicationIdle())
      return true;
    m_txCompositionIdleTimeout.Stop(true);
  }
  else {
    m_lastActive.SetCurrentTime();

    if (info.m_state == m_txCompositionState && m_txCompositionRefreshTimeout.IsRunning())
      return true;

    refreshTime = m_attributes.GetInteger("refresh-timeout", 60);
    m_txCompositionRefreshTimeout.SetInterval(0, refreshTime);
    m_txCompositionIdleTimeout.SetInterval(0, m_attributes.GetInteger("idle-timeout", 15));
  }

  m_txCompositionState = info.m_state;

  OpalIM * message = new OpalIM;

  PStringStream body;
  body << "<?xml version='1.0' encoding='UTF-8'?>\n"
          "<isComposing xmlns='urn:ietf:params:xml:ns:im-iscomposing'\n"
          "             xmlns:xsi='http://www.w3.org/2001/XMLSchema-instance'>\n"
          "  <state>" << info.m_state << "</state>\n"
          "  <lastactive>" << m_lastActive.AsString(PTime::RFC3339) << "</lastactive>\n";
  if (!info.m_contentType.IsEmpty())
    body << "  <contenttype>" << info.m_contentType << "</contenttype>\n";
  if (refreshTime > 0)
    body << "  <refresh>" << refreshTime << "</refresh>\n";
  body << "</isComposing>";

  message->m_bodies.SetAt(ComposingMimeType, body);

  return Send(message) < DispositionErrors;
}


void OpalSIPIMContext::OnTxCompositionIdleTimer(PTimer &, INT)
{
  SendCompositionIndication(CompositionInfo(GetID(), CompositionIndicationIdle()));
}


////////////////////////////////////////////////////////////////////////////////////////////

#endif // OPAL_HAS_SIPIM

