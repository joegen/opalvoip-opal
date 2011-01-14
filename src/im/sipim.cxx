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

const char SIP_IM[] = "sip-im";

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
    { return "sip"; }

    virtual PString GetSDPMediaType() const 
    { return "message"; }

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

SDPMediaDescription * OpalSIPIMMediaType::CreateSDPMediaDescription(const OpalTransportAddress & localAddress)
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
  str << " " << fromURL << "@" << addr << ":" << port;

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

OpalSIPIMMediaSession::OpalSIPIMMediaSession(OpalConnection & _conn, unsigned _sessionId)
: OpalMediaSession(_conn, SIP_IM, _sessionId)
{
  transportAddress = connection.GetTransport().GetLocalAddress();
  localURL         = connection.GetLocalPartyURL();
  remoteURL        = connection.GetRemotePartyURL();
  callId           = connection.GetToken();
}

OpalSIPIMMediaSession::OpalSIPIMMediaSession(const OpalSIPIMMediaSession & obj)
  : OpalMediaSession(obj)
{
  transportAddress = obj.transportAddress;
  localURL         = obj.localURL;        
  remoteURL        = obj.remoteURL;       
  callId           = obj.callId;          
}

OpalTransportAddress OpalSIPIMMediaSession::GetLocalMediaAddress() const
{
  return transportAddress;
}


SDPMediaDescription * OpalSIPIMMediaSession::CreateSDPMediaDescription(const OpalTransportAddress & sdpContactAddress)
{
  return new SDPSIPIMMediaDescription(sdpContactAddress, transportAddress, localURL);
}


OpalMediaStream * OpalSIPIMMediaSession::CreateMediaStream(const OpalMediaFormat & mediaFormat, 
                                                                         unsigned sessionID, 
                                                                         PBoolean isSource)
{
  PTRACE(2, "SIPIM\tCreated " << (isSource ? "source" : "sink") << " media stream in " << (connection.IsOriginating() ? "originator" : "receiver") << " with local " << localURL << " and remote " << remoteURL);
  return new OpalIMMediaStream(connection, mediaFormat, sessionID, isSource);
}


void OpalSIPIMMediaSession::SetRemoteMediaAddress(const OpalTransportAddress &, const OpalMediaFormatList &)
{
}


////////////////////////////////////////////////////////////////////////////////////////////

static PFactory<OpalIMContext>::Worker<OpalSIPIMContext> static_OpalSIPContext("sip");

OpalSIPIMContext::OpalSIPIMContext()
{
  m_attributes.Set("rx-composition-indication-state",   "idle");
  m_attributes.Set("tx-composition-indication-state",   "idle");
  m_attributes.Set("acceptable-content-types",          "text/plain\ntext/html\napplication/im-iscomposing+xml");
  m_rxCompositionTimeout.SetNotifier(PCREATE_NOTIFIER(OnRxCompositionTimerExpire));
  m_txCompositionTimeout.SetNotifier(PCREATE_NOTIFIER(OnTxCompositionTimerExpire));
  m_txIdleTimeout.SetNotifier(PCREATE_NOTIFIER(OnTxIdleTimerExpire));
}

void OpalSIPIMContext::PopulateParams(SIPMessage::Params & params, OpalIM & message)
{
  params.m_localAddress    = message.m_from.AsString();
  params.m_addressOfRecord = params.m_localAddress;
  params.m_remoteAddress   = message.m_to.AsString();
  params.m_id              = message.m_conversationId;
  params.m_messageId       = message.m_messageId;

  switch (message.m_type) {
    case OpalIM::CompositionIndication_Idle:    // RFC 3994
    case OpalIM::CompositionIndication_Active:  // RFC 3994
      {
        bool toActive = message.m_type == OpalIM::CompositionIndication_Active;
        params.m_contentType = "application/im-iscomposing+xml";
        params.m_body = "<?xml version='1.0' encoding='UTF-8'?>\n"
                        "<isComposing xmlns='urn:ietf:params:xml:ns:im-iscomposing'\n"
                        "  xmlns:xsi='http://www.w3.org/2001/XMLSchema-instance'>\n";

        params.m_body += PString("    <state>") + (toActive ? "active" : "idle") + "</state>\n";

        // DO NOT ENABLE THIS BECAUSE XLite barfs on it
        //params.m_body += PString("    <lastactive>") + PTime().AsString(PTime::RFC3339) + "</lastactive>\n";

        params.m_body +=         "    <refresh>60</refresh>\n"
                         "</isComposing>";
      }
      break;

    //case OpalIM::Disposition:  // RFC 5438
    //  break;

    case OpalIM::Text:
    default:
      params.m_contentType = message.m_mimeType;
      params.m_body        = message.m_body;
      break;
  }
}


OpalIMContext::SentStatus OpalSIPIMContext::InternalSendOutsideCall(OpalIM * message)
{
  ResetTimers(*message);

  SIPEndPoint * ep = dynamic_cast<SIPEndPoint *>(m_manager->FindEndPoint("sip"));
  if (ep == NULL) {
    PTRACE(2, "OpalSIPIMContext\tAttempt to send SIP IM without SIP endpoint");
    return SentNoTransport;
  }

  SIPMessage::Params params;
  PopulateParams(params, *message);

  return ep->SendMESSAGE(params);
}


OpalIMContext::SentStatus OpalSIPIMContext::InternalSendInsideCall(OpalIM * message)
{
  ResetTimers(*message);

  PSafePtr<SIPConnection> conn = PSafePtrCast<OpalConnection, SIPConnection>(m_connection);
  if (conn == NULL) {
    PTRACE(2, "OpalSIPIMContext\tAttempt to send SIP IM on non-SIP connection");
    return SentFailedGeneric;
  }

  SIPMessage::Params params;
  PopulateParams(params, *message);

  PSafePtr<SIPTransaction> transaction = new SIPMessage(*conn, params);
  return transaction->Start() ? SentOK : SentFailedGeneric;  
}


void OpalSIPIMContext::ResetTimers(OpalIM & message)
{
  if (message.m_type == OpalIM::Text) {
    m_txCompositionTimeout.Stop(true);
    m_txIdleTimeout.Stop(true);
    m_attributes.Set("tx-composition-indication-state", "idle");
  }
}

static PXML::ValidationInfo const CompositionIndicationValidation[] = {
  { PXML::SetDefaultNamespace,        "urn:ietf:params:xml:ns:im-iscomposing" },
  { PXML::ElementName,                "isComposing", },

  { PXML::OptionalElement,                   "state"       },
  { PXML::OptionalElement,                   "lastactive"  },
  { PXML::OptionalElement,                   "contenttype" },
  { PXML::OptionalElementWithBodyMatchingEx, "refresh",    { "[0-9]*" } },

  { PXML::EndOfValidationList }
};



OpalIMContext::SentStatus OpalSIPIMContext::OnIncomingIM(OpalIM & message)
{
  if (message.m_mimeType == "application/im-iscomposing+xml") {
    PXML xml;
    PString error;
    if (!xml.LoadAndValidate(message.m_body, CompositionIndicationValidation, error, PXML::WithNS)) {
      PTRACE(2, "OpalSIPIMContext\tXML error: " << error);
      return SentInvalidContent;
    }
    PString state = "idle";
    int timeout = 15;

    PXMLElement * element = xml.GetRootElement()->GetElement("state");
    if ((element != NULL) && (element->GetData().Trim() == "active"))
      state = "active";

    element = xml.GetRootElement()->GetElement("refresh");
    if (element != NULL)
      timeout = element->GetData().Trim().AsInteger();

    if (state == m_attributes.Get("rx-composition-indication-state")) {
      PTRACE(2, "OpalSIPIMContext\tcomposition indication refreshed");
      return SentOK;
    }
    m_attributes.Set("rx-composition-indication-state", state);

    if (state == "active")
      m_rxCompositionTimeout = timeout * 1000;
    else
      m_rxCompositionTimeout.Stop(true);

    OnCompositionIndicationChanged(state);

    return SentOK;
  }

  // receipt of text always indicated idle
  m_rxCompositionTimeout.Stop(true);
  OnCompositionIndicationTimeout();

  // forward the text
  return OpalIMContext::OnIncomingIM(message);
}

void OpalSIPIMContext::OnRxCompositionTimerExpire(PTimer &, INT)
{
  m_manager->GetIMManager().OnCompositionIndicationTimeout(GetID());
}

void OpalSIPIMContext::OnCompositionIndicationTimeout()
{
  if (m_attributes.Get("rx-composition-indication-state") != "idle") {
    m_attributes.Set("rx-composition-indication-state", "idle");
    OnCompositionIndicationChanged("idle");
  }
}


/*
OpalIMContext::SentStatus OpalSIPIMContext::SendText(bool active)
{
  m_txCompositionTimeout.Stop(true);
  m_txIdleTimeout.Stop(true);
  m_attributes.Set("tx-composition-indication-state", "idle");
}
*/

OpalIMContext::SentStatus OpalSIPIMContext::SendCompositionIndication(bool active)
{
  bool currentState = !(m_attributes.Get("tx-composition-indication-state", "idle") == "idle");

  if (active == currentState)
    return OpalIMContext::SentOK;

  if (active) {
    m_attributes.Set("tx-composition-indication-state", "active");
    m_txCompositionTimeout = 1000 * 60;
    m_txIdleTimeout = 1000 * 15;

  }
  else {
    m_txCompositionTimeout.Stop(true);
    m_txIdleTimeout.Stop(true);
  }

  return OpalIMContext::SendCompositionIndication(active);
}


void OpalSIPIMContext::OnTxCompositionTimerExpire(PTimer &, INT)
{
  OpalIMContext::SendCompositionIndication(true);
  m_txCompositionTimeout.SetMilliSeconds(1000 * 60);
}

void OpalSIPIMContext::OnTxIdleTimerExpire(PTimer &, INT)
{
  m_txCompositionTimeout.Stop(true);
  OpalIMContext::SendCompositionIndication(false);
}

////////////////////////////////////////////////////////////////////////////////////////////

#endif // OPAL_HAS_SIPIM

