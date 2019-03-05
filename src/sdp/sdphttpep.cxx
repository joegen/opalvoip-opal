/*
 * sdpep.cxx
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
  m_defaultStringOptions.SetBoolean(OPAL_OPT_RTCP_MUX, true);
  m_defaultStringOptions.SetBoolean(OPAL_OPT_USE_MEDIA_STREAMS, true);
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
  OpalSDPHTTPConnection * connection = CreateConnection(call, userData, options, stringOptions);
  if (AddConnection(connection) == NULL) {
    delete connection;
    return NULL;
  }

  connection->m_destination = party;
  return connection;
}


OpalSDPHTTPConnection * OpalSDPHTTPEndPoint::CreateConnection(OpalCall & call,
                                                                  void * ,
                                                              unsigned   options,
                                         OpalConnection::StringOptions * stringOptions)
{
  return new OpalSDPHTTPConnection(call, *this, options, stringOptions);
}


bool OpalSDPHTTPEndPoint::OnReceivedHTTP(PHTTPServer & server, const PHTTPConnectionInfo & connectInfo)
{
  OpalCall * call = m_manager.InternalCreateCall();
  if (call == NULL)
    return server.OnError(PHTTP::InternalServerError, "Could not create call", connectInfo);

  PTRACE_CONTEXT_ID_PUSH_THREAD(call);

  OpalSDPHTTPConnection *connection = CreateConnection(*call, NULL, 0, NULL);
  if (AddConnection(connection) != NULL)
    return connection->OnReceivedHTTP(server, connectInfo);

  m_manager.DestroyCall(call);
  return server.OnError(PHTTP::InternalServerError, "Could not create connection", connectInfo);
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


bool OpalSDPHTTPResource::OnGET(PHTTPServer & server, const PHTTPConnectionInfo & conInfo)
{
  return m_endpoint.OnReceivedHTTP(server, conInfo);
}


bool OpalSDPHTTPResource::OnPOST(PHTTPServer & server, const PHTTPConnectionInfo & conInfo)
{
  return m_endpoint.OnReceivedHTTP(server, conInfo);
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
  PHTTPClient http;
  if (!http.ConnectURL(m_destination))
    return false;

  InternalSetMediaAddresses(http);

  PString offer = GetOfferSDP();
  if (offer.IsEmpty())
    return false;

  PMIMEInfo outMIME, replyMIME;
  outMIME.SetAt(PHTTP::ContentTypeTag(), OpalSDPEndPoint::ContentType());

  PString answer;
  if (!http.PostData(m_destination, outMIME, offer, replyMIME, answer))
    return false;

  return HandleAnswerSDP(answer);
}


void OpalSDPHTTPConnection::OnReleased()
{
  m_connected.Signal(); // Break block if waiting
  OpalSDPConnection::OnReleased();
}


bool OpalSDPHTTPConnection::OnReceivedHTTP(PHTTPServer & server, const PHTTPConnectionInfo & connectInfo)
{
  m_destination = connectInfo.GetURL().GetQueryVars()("destination");
  if (m_destination.IsEmpty()) {
    PTRACE(1, "HTTP URL does not have a destination query parameter");
    return server.OnError(PHTTP::NotFound, "Must have a destination query parameter", connectInfo);
  }

  if (OpalSDPEndPoint::ContentType() != connectInfo.GetMIME().Get(PHTTP::ContentTypeTag())) {
    PTRACE(1, "HTTP does not have " << PHTTP::ContentTypeTag() << " of " << OpalSDPEndPoint::ContentType());
    return server.OnError(PHTTP::NoneAcceptable, "Must be " + OpalSDPEndPoint::ContentType(), connectInfo);
  }

  SetPhase(SetUpPhase);
  OnApplyStringOptions();
  if (!OnIncomingConnection(0, NULL))
    return server.OnError(PHTTP::NotFound, "Invalid destination query parameter", connectInfo);

  InternalSetMediaAddresses(server);

  PTRACE(4, "Received SDP:\n" << connectInfo.GetEntityBody());
  m_offerSDP = m_endpoint.CreateSDP(0, 0, OpalTransportAddress());
  if (!m_offerSDP->Decode(connectInfo.GetEntityBody(), GetLocalMediaFormats()) || m_offerSDP->GetMediaDescriptions().IsEmpty()) {
    PTRACE(1, "HTTP body does not have acceptable SDP");
    return server.OnError(PHTTP::BadRequest, "HTTP body does not have acceptable SDP", connectInfo);
  }

  if (!m_ownerCall.OnSetUp(*this))
    return server.OnError(PHTTP::BadGateway, "Could not set up secondary connection", connectInfo);

  m_connected.Wait();

  delete m_offerSDP;
  m_offerSDP = NULL;

  if (m_answerSDP == NULL) {
    PTRACE(1, "SDP over HTTP call not answered");
    return server.OnError(PHTTP::ServiceUnavailable, "No answer", connectInfo);
  }

  PString answer = m_answerSDP->Encode();
  delete m_answerSDP;
  m_answerSDP = NULL;

  PTRACE(4, "Sending SDP:\n" << answer);

  PMIMEInfo headers;
  headers.Set(PHTTP::ContentTypeTag(), OpalSDPEndPoint::ContentType());
  server.StartResponse(PHTTP::RequestOK, headers, answer.GetLength());
  return server.WriteString(answer);
}


PBoolean OpalSDPHTTPConnection::SetConnected()
{
  SDPSessionDescription * answerSDP = m_endpoint.CreateSDP(0, 0, OpalTransportAddress());

  bool ok = OnSendAnswerSDP(*m_offerSDP, *answerSDP);
  if (ok)
    m_answerSDP = answerSDP;
  else
    delete answerSDP;

  m_connected.Signal();
  return ok;
}


PString OpalSDPHTTPConnection::GetDestinationAddress()
{
  return m_destination;
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
