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

#include <ptlib/socket.h>
#include <ptclib/random.h>
#include <ptclib/pxml.h>

#include <opal/transports.h>
#include <opal/mediatype.h>
#include <opal/mediafmt.h>
#include <opal/endpoint.h>

#include <im/im.h>
#include <im/sipim.h>
#include <sip/sipep.h>
#include <sip/sipcon.h>

#if OPAL_HAS_SIPIM

const char SIP_IM[] = "sip-im"; // RFC 3428

static PConstCaselessString const ComposingMimeType("application/im-iscomposing+xml");
static PConstCaselessString const DispositionMimeType("message/imdn+xml");


////////////////////////////////////////////////////////////////////////////

OpalSIPIMMediaType::OpalSIPIMMediaType()
  : OpalIMMediaType(SIP_IM, "message|sip")
{
}


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
    SDPSIPIMMediaDescription(const OpalTransportAddress & address, const OpalTransportAddress & _transportAddr, const PString & fromURL);

    PCaselessString GetSDPTransportType() const
    {
      return "sip";
    }

    virtual SDPMediaDescription * CreateEmpty() const
    {
      return new SDPSIPIMMediaDescription(OpalTransportAddress());
    }

    virtual PString GetSDPMediaType() const 
    {
      return "message";
    }

    virtual PString GetSDPPortList() const;

    virtual void CreateSDPMediaFormats(const PStringArray &);
    virtual bool PrintOn(ostream & str, const PString & connectString) const;
    virtual void SetAttribute(const PString & attr, const PString & value);
    virtual void ProcessMediaOptions(SDPMediaFormat & sdpFormat, const OpalMediaFormat & mediaFormat);
    virtual void AddMediaFormat(const OpalMediaFormat & mediaFormat);

    virtual OpalMediaFormatList GetMediaFormats() const;

    // CreateSDPMediaFormat is used for processing format lists. MSRP always contains only "*"
    virtual SDPMediaFormat * CreateSDPMediaFormat(const PString & ) { return NULL; }

    // FindFormat is used only for rtpmap and fmtp, neither of which are used for MSRP
    virtual SDPMediaFormat * FindFormat(PString &) const { return NULL; }

  protected:
    OpalTransportAddress transportAddress;
    PString fromURL;
};

////////////////////////////////////////////////////////////////////////////////////////////

SDPMediaDescription * OpalSIPIMMediaType::CreateSDPMediaDescription(const OpalTransportAddress & localAddress, OpalMediaSession *) const
{
  return new SDPSIPIMMediaDescription(localAddress);
}

///////////////////////////////////////////////////////////////////////////////////////////

SDPSIPIMMediaDescription::SDPSIPIMMediaDescription(const OpalTransportAddress & address)
  : SDPMediaDescription(address, SIP_IM)
{
  SetDirection(SDPMediaDescription::SendRecv);
}

SDPSIPIMMediaDescription::SDPSIPIMMediaDescription(const OpalTransportAddress & address, const OpalTransportAddress & _transportAddr, const PString & _fromURL)
  : SDPMediaDescription(address, SIP_IM)
  , transportAddress(_transportAddr)
  , fromURL(_fromURL)
{
  SetDirection(SDPMediaDescription::SendRecv);
}

void SDPSIPIMMediaDescription::CreateSDPMediaFormats(const PStringArray &)
{
  formats.Append(new SDPMediaFormat(*this, OpalSIPIM));
}


bool SDPSIPIMMediaDescription::PrintOn(ostream & str, const PString & /*connectString*/) const
{
  // call ancestor
  if (!SDPMediaDescription::PrintOn(str, ""))
    return false;

  return true;
}

PString SDPSIPIMMediaDescription::GetSDPPortList() const
{ 
  PIPSocket::Address addr; WORD port;
  transportAddress.GetIpAndPort(addr, port);

  PStringStream str;
  str << ' ' << fromURL << '@' << addr << ':' << port;

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
  sipim.SetOptionString("URL", fromURL);

  PTRACE(4, "SIPIM\tNew format is " << setw(-1) << sipim);

  OpalMediaFormatList fmts;
  fmts += sipim;
  return fmts;
}

void SDPSIPIMMediaDescription::AddMediaFormat(const OpalMediaFormat & mediaFormat)
{
  if (!mediaFormat.IsTransportable() || !mediaFormat.IsValidForProtocol("sip") || mediaFormat.GetMediaType() != SIP_IM) {
    PTRACE(4, "SIPIM\tSDP not including " << mediaFormat << " as it is not a valid SIPIM format");
    return;
  }

  SDPMediaFormat * sdpFormat = new SDPMediaFormat(*this, mediaFormat);
  ProcessMediaOptions(*sdpFormat, mediaFormat);
  AddSDPMediaFormat(sdpFormat);
}


////////////////////////////////////////////////////////////////////////////////////////////

OpalMediaSession * OpalSIPIMMediaType::CreateMediaSession(OpalConnection & conn, unsigned sessionID) const
{
  // as this is called in the constructor of an OpalConnection descendant, 
  // we can't use a virtual function on OpalConnection

  if (conn.GetPrefixName() *= "sip")
    return new OpalSIPIMMediaSession(conn, sessionID);

  return NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////

OpalSIPIMMediaSession::OpalSIPIMMediaSession(OpalConnection & conn, unsigned sessionId)
  : OpalMediaSession(conn, sessionId, SIP_IM)
  , m_transportAddress(m_connection.GetTransport().GetLocalAddress())
  , m_localURL(m_connection.GetLocalPartyURL())
  , m_remoteURL(m_connection.GetRemotePartyURL())
  , m_callId(m_connection.GetToken())
{
}


OpalTransportAddress OpalSIPIMMediaSession::GetLocalMediaAddress() const
{
  return m_transportAddress;
}


OpalTransportAddress OpalSIPIMMediaSession::GetRemoteMediaAddress() const
{
  return m_remoteURL.GetHostAddress();
}


OpalMediaStream * OpalSIPIMMediaSession::CreateMediaStream(const OpalMediaFormat & mediaFormat, 
                                                            unsigned sessionID, 
                                                            PBoolean isSource)
{
  PTRACE(2, "SIPIM\tCreated " << (isSource ? "source" : "sink") << " media stream in "
         << (m_connection.IsOriginating() ? "originator" : "receiver")
         << " with local " << m_localURL << " and remote " << m_remoteURL);
  return new OpalIMMediaStream(m_connection, mediaFormat, sessionID, isSource);
}


////////////////////////////////////////////////////////////////////////////////////////////

static PFactory<OpalIMContext>::Worker<OpalSIPIMContext> static_OpalSIPContext("sip");

OpalSIPIMContext::OpalSIPIMContext()
{
  m_attributes.Set("acceptable-content-types", "text/plain\ntext/html\napplication/im-iscomposing+xml");
  m_rxCompositionIdleTimeout.SetNotifier(PCREATE_NOTIFIER(OnRxCompositionIdleTimer));
  m_txCompositionIdleTimeout.SetNotifier(PCREATE_NOTIFIER(OnTxCompositionIdleTimer));
}


void OpalSIPIMContext::OnMESSAGECompleted(SIPEndPoint & endpoint,
                                          const SIPMessage::Params & params,
                                          SIP_PDU::StatusCodes reason)
{
  // RFC3428
  PSafePtr<OpalSIPIMContext> context = PSafePtrCast<OpalIMContext,OpalSIPIMContext>(
                  endpoint.GetManager().GetIMManager().FindContextByIdWithLock(params.m_id));
  if (context == NULL) {
    PTRACE2(2, &endpoint, "OpalIM\tCannot find IM context for \"" << params.m_id << '"');
    return;
  }

  OpalIMContext::DispositionInfo result;
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
                                         OpalTransport & transport,
                                         SIP_PDU & pdu)
{
  // RFC3428
  const SIPMIMEInfo & mime  = pdu.GetMIME();

  OpalIMContext::MessageDisposition status;
  PString errorInfo;
  {
    SIPURL to(mime.GetTo());
    SIPURL from(mime.GetFrom());

    OpalIM message;
    message.m_conversationId = mime.GetCallID();
    message.m_to             = to;
    message.m_from           = from;
    message.m_toAddr         = transport.GetLastReceivedAddress();
    message.m_fromAddr       = transport.GetRemoteAddress();
    message.m_fromName       = from.GetDisplayName();
    message.m_bodies.SetAt(mime.GetContentType(), pdu.GetEntityBody());
    status = endpoint.GetManager().GetIMManager().OnMessageReceived(message, connection, errorInfo);
  }

  SIPResponse * response = new SIPResponse(endpoint, SIP_PDU::Failure_BadRequest);

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
  response->Send(transport, pdu);
}


void OpalSIPIMContext::PopulateParams(SIPMessage::Params & params, const OpalIM & message)
{
  SIPURL from = message.m_from;
  from.SetDisplayName(m_localName);

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
  SIPEndPoint * ep = dynamic_cast<SIPEndPoint *>(m_manager->FindEndPoint("sip"));
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
  PSafePtr<SIPConnection> conn = PSafePtrCast<OpalConnection, SIPConnection>(m_connection);
  if (conn == NULL) {
    PTRACE(2, "OpalSIPIMContext\tAttempt to send SIP IM on non-SIP connection");
    return ConversationClosed;
  }

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

  OnCompositionIndication(newState);

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

OpalIMContext::MessageDisposition OpalSIPIMContext::InternalOnCompositionIndication(const OpalIM & message)
{
  return UnsupportedFeature;
}

OpalIMContext::MessageDisposition OpalSIPIMContext::InternalOnDisposition(const OpalIM & message)
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
    OnCompositionIndication(m_rxCompositionState = CompositionIndicationIdle());

  // forward the text
  return OpalIMContext::OnMessageReceived(message);
}


void OpalSIPIMContext::OnRxCompositionIdleTimer(PTimer &, INT)
{
  OnCompositionIndication(m_rxCompositionState = CompositionIndicationIdle());
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
  SendCompositionIndication(CompositionIndicationIdle());
}


////////////////////////////////////////////////////////////////////////////////////////////

#endif // OPAL_HAS_SIPIM

