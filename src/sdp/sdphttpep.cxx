/*
 * sdphttpep.cxx
 *
 * Open Phone Abstraction Library (OPAL)
 *
 * Copyright (C) 2014 Vox Lucida Pty. Ltd.
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

#include <ptlib.h>

#ifdef P_USE_PRAGMA
#pragma implementation "sdphttpep.h"
#endif

#include <sdp/sdphttpep.h>


#define PTraceModule() "SDP-HTTP"


#if OPAL_SDP_HTTP

OpalSDPHTTPEndPoint::OpalSDPHTTPEndPoint(OpalManager & manager, const PCaselessString & prefix)
  : OpalSDPEndPoint(manager, prefix, IsNetworkEndPoint)
{
  manager.AttachEndPoint(this, "http");
#if OPAL_PTLIB_SSL
  manager.AttachEndPoint(this, "sdps");
  manager.AttachEndPoint(this, "https");
#endif
  m_httpSpace.AddResource(new OpalSDPHTTPResource(*this, "/"));

  // This is almost always for WebRTC, so turn on some advanced features by default.
  m_defaultStringOptions.SetBoolean(OPAL_OPT_AV_BUNDLE, true);
  m_defaultStringOptions.SetBoolean(OPAL_OPT_USE_MEDIA_STREAMS, true);
  m_defaultStringOptions.SetString(OPAL_OPT_CRYPTO_EXCHANGE, OPAL_OPT_CRYPTO_EXCHANGE_INBAND_KEY_EXCHANGE);
}


OpalSDPHTTPEndPoint::~OpalSDPHTTPEndPoint()
{
}


PString OpalSDPHTTPEndPoint::GetDefaultTransport() const 
{
#if OPAL_PTLIB_SSL
  return PSTRSTRM(OpalTransportAddress::TcpPrefix() << ',' << OpalTransportAddress::TlsPrefix() << ':' << DefaultSecurePort);
#else
  return OpalTransportAddress::TcpPrefix();
#endif
}


WORD OpalSDPHTTPEndPoint::GetDefaultSignalPort() const
{
  return DefaultPort;
}


void OpalSDPHTTPEndPoint::NewIncomingConnection(OpalListener &, const OpalTransportPtr & transport)
{
  if (transport == NULL)
    return;

  PHTTPServer ws(m_httpSpace);

  if (ws.Open(transport->GetChannel(), false)) {
    while (ws.ProcessCommand())
      ;
    ws.Detach();
  }
}


PSafePtr<OpalConnection> OpalSDPHTTPEndPoint::MakeConnection(OpalCall & call,
                                                             const PString & party,
                                                             void * userData,
                                                             unsigned int options,
                                                             OpalConnection::StringOptions * stringOptions)
{
  PString urlStr = StripPrefixName(party);
  PURL destURL;
  if (!urlStr.IsEmpty() && !destURL.Parse(urlStr, "http")) {
    PTRACE(2, "Cannot parse as URL destination string \"" << urlStr << '"');
    return NULL;
  }

  OpalSDPHTTPConnection * connection = CreateConnection(call, userData, options, stringOptions);
  if (AddConnection(connection) == NULL) {
    delete connection;
    return NULL;
  }

  connection->m_destination = destURL;
  return connection;
}


OpalSDPHTTPConnection * OpalSDPHTTPEndPoint::CreateConnection(OpalCall & call,
                                                                  void * ,
                                                              unsigned   options,
                                         OpalConnection::StringOptions * stringOptions)
{
  return new OpalSDPHTTPConnection(call, *this, options, stringOptions);
}


bool OpalSDPHTTPEndPoint::OnReceivedHTTP(PHTTPRequest & request)
{
  PSafePtr<OpalSDPHTTPConnection> connection;

  if (request.url.GetQueryVars().Get("operation", "start") *= "start") {
    OpalCall * call = m_manager.InternalCreateCall();
    if (call == NULL)
      return request.OnError("Could not create internal call");

    PTRACE_CONTEXT_ID_PUSH_THREAD(call);

    OpalSDPHTTPConnection * newConn = CreateConnection(*call, NULL, 0, NULL);
    if (AddConnection(newConn) == NULL) {
      m_manager.DestroyCall(call);
      return request.OnError("Could not create internal connection");
    }

    connection = newConn;
  }
  else {
    connection = GetConnectionWithLockAs<OpalSDPHTTPConnection>(request.url.GetQueryVars().Get("connection-id"));
    if (connection == NULL)
      return request.OnError(PHTTP::NotFound, "Could not find connection id");
  }

  return connection->OnReceivedHTTP(request);
}


///////////////////////////////////////////////////////////////////////////////

OpalSDPHTTPResource::OpalSDPHTTPResource(OpalSDPHTTPEndPoint & ep, const PURL & url)
  : PHTTPResource(url)
  , m_endpoint(ep)
{
}


OpalSDPHTTPResource::OpalSDPHTTPResource(OpalSDPHTTPEndPoint & ep, const PURL & url, const PHTTPAuthority & auth)
  : PHTTPResource(url, auth)
  , m_endpoint(ep)
{
}


bool OpalSDPHTTPResource::OnGETData(PHTTPRequest & request)
{
  return m_endpoint.OnReceivedHTTP(request);
}


bool OpalSDPHTTPResource::OnPOSTData(PHTTPRequest & request, const PStringToString &)
{
  return m_endpoint.OnReceivedHTTP(request);
}


///////////////////////////////////////////////////////////////////////////////

OpalSDPHTTPConnection::OpalSDPHTTPConnection(OpalCall & call,
                                             OpalSDPHTTPEndPoint & ep,
                                             unsigned options,
                                             OpalConnection::StringOptions * stringOptions)
  : OpalSDPConnection(call, ep, ep.GetManager().GetNextToken('W'), options, stringOptions)
  , m_offerSDP(NULL)
  , m_answerSDP(NULL)
{
}


OpalSDPHTTPConnection::~OpalSDPHTTPConnection()
{
  delete m_offerSDP;
  delete m_answerSDP;
}


PBoolean OpalSDPHTTPConnection::SetUpConnection()
{
  PTRACE(3, "Setting up SDP over HTTP connection to " << m_destination << " on " << *this);

  InternalSetAsOriginating();

  PHTTPClient http;
  if (!http.ConnectURL(m_destination)) {
    PTRACE(2, "Could not connect to " << m_destination << ": "
           << http.GetLastResponseCode() << " - " << http.GetLastResponseInfo());
    return false;
  }

  InternalSetMediaAddresses(http);

  PString offer = GetOfferSDP();
  if (offer.IsEmpty())
    return false;

  PMIMEInfo outMIME, replyMIME;
  outMIME.SetAt(PHTTP::ContentTypeTag(), OpalSDPEndPoint::ContentType());

  PString answer;
  if (!http.PostData(m_destination, outMIME, offer, replyMIME, answer)) {
    PTRACE(2, "Could not POST to " << m_destination << ": "
           << http.GetLastResponseCode() << " - " << http.GetLastResponseInfo());
    return false;
  }

  return HandleAnswerSDP(answer);
}


void OpalSDPHTTPConnection::OnReleased()
{
  m_connected.Signal(); // Break block if waiting

  if (IsOriginating() && !m_destination.IsEmpty()) {
    PURL stop = m_destination;
    stop.SetParamVar("operation", "stop");

    PHTTPClient http;
    PString reply;
    if (http.GetTextDocument(stop, reply))
      PTRACE(3, "Sent stop command to " << m_destination);
    else
      PTRACE(2, "Could not send stop command to " << m_destination << ": "
             << http.GetLastResponseCode() << ' ' << http.GetLastResponseInfo());
  }

  OpalSDPConnection::OnReleased();
}


bool OpalSDPHTTPConnection::OnReceivedHTTP(PHTTPRequest & request)
{
  const PStringToString & parameters = request.url.GetQueryVars();
  PCaselessString operation = parameters("operation");
  if (operation == "stop" || operation == "release" || operation == "disconnect") {
    Release();
    return request.SendResponse("Releasing connection " + GetIdentifier());
  }

  if (operation == "status") {
    PHTML html;
    html << PHTML::HTML()
         << PHTML::Title() << "Status for connection " << GetIdentifier() << PHTML::Title()
         << PHTML::Body()
         << PHTML::Heading(1) << "Status for connection " << GetIdentifier() << PHTML::Heading(1);
    if (IsEstablished())
      html << "Established";
    else if (IsReleased())
      html << "Ending";
    else
      html << "Starting";
    html << PHTML::Body()
         << PHTML::HTML();
    return request.SendResponse(html);
  }

  if (m_destination.Parse(parameters("destination"), "http")) {
    PTRACE(1, "HTTP URL does not have a destination query parameter");
    return request.OnError(PHTTP::NotFound, "Must have a destination query parameter");
  }

  if (OpalSDPEndPoint::ContentType() != request.inMIME.Get(PHTTP::ContentTypeTag())) {
    PTRACE(1, "HTTP does not have " << PHTTP::ContentTypeTag() << " of " << OpalSDPEndPoint::ContentType());
    return request.OnError(PHTTP::NoneAcceptable, "Must be " + OpalSDPEndPoint::ContentType());
  }

  SetPhase(SetUpPhase);
  OnApplyStringOptions();
  if (!OnIncomingConnection(0, NULL))
    return request.OnError(PHTTP::NotFound, "Invalid destination query parameter");

  InternalSetMediaAddresses(request.server);

  PTRACE(4, "Received SDP on " << *this << ":\n" << request.GetEntityBody());
  m_offerSDP = m_endpoint.CreateSDP(0, 0, OpalTransportAddress());
  if (!m_offerSDP->Decode(request.GetEntityBody(), GetLocalMediaFormats()) || m_offerSDP->GetMediaDescriptions().IsEmpty()) {
    PTRACE(1, "HTTP body does not have acceptable SDP");
    return request.OnError(PHTTP::BadRequest, "HTTP body does not have acceptable SDP");
  }

  if (!m_ownerCall.OnSetUp(*this))
    return request.OnError(PHTTP::BadGateway, "Could not set up secondary connection");

  PTRACE(3, "Awaiting SetConnected on " << *this);
  m_connected.Wait();

  delete m_offerSDP;
  m_offerSDP = NULL;

  if (m_answerSDP == NULL) {
    PTRACE(1, "SDP over HTTP call not answered on " << *this);
    return request.OnError(PHTTP::ServiceUnavailable, "No answer");
  }

  PString answer = m_answerSDP->Encode();
  delete m_answerSDP;
  m_answerSDP = NULL;

  PTRACE(4, "Sending SDP on " << *this << ":\n" << answer);

  request.outMIME.Set(PHTTP::ContentTypeTag(), OpalSDPEndPoint::ContentType());
  request.outMIME.Set("X-OPAL-Connection-Id", GetIdentifier());
  if (request.SendResponse(answer))
    return true;

  Release(EndedByTransportFail);
  return false;
}


PBoolean OpalSDPHTTPConnection::SetConnected()
{
  SDPSessionDescription * answerSDP = m_endpoint.CreateSDP(0, 0, OpalTransportAddress());

  bool ok = OnSendAnswerSDP(*m_offerSDP, *answerSDP);
  if (ok)
    m_answerSDP = answerSDP;
  else {
    delete answerSDP;
    Release(EndedByCapabilityExchange);
  }

  m_connected.Signal();
  return ok && OpalSDPConnection::SetConnected();
}


PString OpalSDPHTTPConnection::GetIdentifier() const
{
  return m_guid.AsString();
}


PString OpalSDPHTTPConnection::GetDestinationAddress()
{
  return m_destination.AsString();
}


PString OpalSDPHTTPConnection::GetMediaInterface()
{
  return m_mediaInterface;
}


OpalTransportAddress OpalSDPHTTPConnection::GetRemoteMediaAddress()
{
  return m_remoteAddress;
}


void OpalSDPHTTPConnection::InternalSetMediaAddresses(PIndirectChannel & channel)
{
  PIPSocket * socket = dynamic_cast<PIPSocket *>(channel.GetBaseReadChannel());
  if (socket == NULL) {
    PTRACE(3, "Could not get media addresses, not an IP socket");
    return;
  }

  PIPAddress ip;
  if (socket->GetLocalAddress(ip))
    m_mediaInterface = ip.AsString();
  else
    PTRACE(3, "Could not get local media address");

  PIPAddressAndPort ap;
  if (socket->GetPeerAddress(ap))
    m_remoteAddress = ap;
  else
    PTRACE(3, "Could not get remote media address");
}


#endif // OPAL_SDP_HTTP

/////////////////////////////////////////////////////////////////////////////
