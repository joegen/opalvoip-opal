/*
 * sipep.cxx
 *
 * Session Initiation Protocol endpoint.
 *
 * Open Phone Abstraction Library (OPAL)
 *
 * Copyright (c) 2000 Equivalence Pty. Ltd.
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
 * The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * Contributor(s): ______________________________________.
 *
 * $Log: sipep.cxx,v $
 * Revision 1.2002  2002/02/01 04:53:01  robertj
 * Added (very primitive!) SIP support.
 *
 */

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "sipep.h"
#endif

#include <sip/sipep.h>

#include <sip/sippdu.h>
#include <sip/sipcon.h>

//#include <ptclib/cypher.h>
#include <opal/manager.h>
//#include <opal/call.h>
//#include <opal/patch.h>
//#include <codec/rfc2833.h>


#define	ALLOWED_METHODS	"INVITE"

#define new PNEW


////////////////////////////////////////////////////////////////////////////

SIPEndPoint::SIPEndPoint(OpalManager & mgr)
  : OpalEndPoint(mgr, "sip", CanTerminateCall),
    registrationID(OpalGloballyUniqueID().AsString()),
    registrationName(PProcess::Current().GetUserName())
{
  defaultSignalPort = 5060;
  mimeForm = FALSE;
  lastSentCSeq = 1;

  PTRACE(3, "SIP\tCreated endpoint.");
}


SIPEndPoint::~SIPEndPoint()
{
  // Shut down the listeners as soon as possible to avoid race conditions
  listeners.RemoveAll();

  PTRACE(3, "SIP\tDeleted endpoint.");
}


void SIPEndPoint::NewIncomingConnection(OpalTransport * transport)
{
  PTRACE_IF(2, transport->IsReliable(), "SIP\tListening thread started.");

  transport->SetBufferSize(MAX_SIP_UDP_PDU_SIZE);

  do {
    HandlePDU(*transport);
  } while (transport->IsOpen() && transport->IsReliable());

  PTRACE_IF(2, transport->IsReliable(), "SIP\tListening thread finished.");
}


void SIPEndPoint::HandlePDU(OpalTransport & transport)
{
  // create a SIP_PDU structure, then get it to read and process PDU
  SIP_PDU pdu(transport);

  PTRACE(3, "SIP\tWaiting for PDU on " << transport);
  if (pdu.Read()) {
    if (!OnReceivedPDU(pdu)) {
      SIP_PDU response(pdu, SIP_PDU::Failure_MethodNotAllowed);
      response.GetMIME().SetAt("Allow", ALLOWED_METHODS);
      response.Write();
    }
  }
  else if (transport.good()) {
    PTRACE(1, "SIP\tMalformed request received on " << transport);
    pdu.SendResponse(SIP_PDU::Failure_BadRequest);
  }
}


BOOL SIPEndPoint::SetUpConnection(OpalCall & call,
                                  const PString & remoteParty,
                                  void * userData)
{
  if (remoteParty.Find("sip:") != 0)
    return FALSE;

  PStringStream callID;
  OpalGloballyUniqueID id;
  callID << id << '@' << PIPSocket::GetHostName();
  SIPConnection * connection = CreateConnection(call, callID, NULL, userData);
  if (connection == NULL)
    return FALSE;

  connection->Lock();
  AddNewConnection(connection);
  connection->InitiateCall(remoteParty);
  connection->Unlock();

  return TRUE;
}


SIPConnection * SIPEndPoint::GetSIPConnectionWithLock(const SIPMIMEInfo & mime)
{
  return GetSIPConnectionWithLock(mime.GetCallID());
}


SIPConnection * SIPEndPoint::GetSIPConnectionWithLock(const PString & str)
{
  return (SIPConnection *)GetConnectionWithLock(str);
}


BOOL SIPEndPoint::IsAcceptedAddress(const SIPURL & /*toAddr*/)
{
  return TRUE;
}


SIPConnection * SIPEndPoint::CreateConnection(OpalCall & call,
                                              const PString & token,
                                              SIP_PDU * invite,
                                              void * /*userData*/)
{
  return new SIPConnection(call, *this, token, invite);
}


void SIPEndPoint::AddNewConnection(SIPConnection * conn)
{
  PWaitAndSignal m(inUseFlag);
  connectionsActive.SetAt(conn->GetToken(), conn);
}


BOOL SIPEndPoint::OnReceivedPDU(SIP_PDU & pdu)
{
  SIPMIMEInfo & mime = pdu.GetMIME();

  SIPConnection * connection = GetSIPConnectionWithLock(mime);
  if (connection != NULL) {
    BOOL ok = connection->OnReceivedPDU(pdu);
    connection->Unlock();
     return ok;
  }

  PString method = pdu.GetMethod();

  // An INVITE is a special case
  if (method *= "INVITE") {
    OnReceivedINVITE(pdu);
    return TRUE;
  }

  // dispatch other commands not associated with a connection
  // ....

  return FALSE;
}


BOOL SIPEndPoint::OnReceivedINVITE(SIP_PDU & request)
{
  // send a response just in case this takes a while
  request.SendResponse(SIP_PDU::Information_Trying);

  SIPMIMEInfo & mime = request.GetMIME();
  PString callID     = mime.GetCallID();

  // parse the incoming To field, and check if we accept incoming calls for this address
  SIPURL toAddr(mime.GetTo());
  if (!IsAcceptedAddress(toAddr)) {
    PTRACE(1, "SIP\tIncoming INVITE from " << request.GetURI() << " for unknown address " << toAddr);
    request.SendResponse(SIP_PDU::Failure_NotFound);
    return FALSE;
  }

  // create outgoing To field
  PString outgoingTo = mime.GetTo() + ";tag=12345678";
  mime.SetTo(outgoingTo);

  // ask the endpoint for a connection
  SIPConnection * connection = CreateConnection(*GetManager().CreateCall(), callID, &request);

  // clean up the connection
  if (connection == NULL) {
    request.SendResponse(SIP_PDU::Failure_NotFound);
    PTRACE(2, "SIP\tFailed to create SIPConnection for INVITE from " << request.GetURI() << " for " << toAddr);
    return FALSE;
  }

  // add the connection to the endpoint list
  AddNewConnection(connection);

  // indicate the other is to start ringing
  if (!connection->OnIncomingConnection()) {
    PTRACE(2, "SIP\tOnIncomingConnection failed for INVITE from " << request.GetURI() << " for " << toAddr);
    connection->Release();
    return FALSE;
  }

  PTRACE(2, "SIP\tOnIncomingConnection succeeded for INVITE from " << request.GetURI() << " for " << toAddr);
  connection->OnReceivedINVITE(request);
  return TRUE;
}


static BOOL WriteREGISTER(OpalTransport & transport, PObject * param)
{
  SIPEndPoint & endpoint = *(SIPEndPoint *)param;

  SIP_PDU request(transport);
  SIPURL name(endpoint.GetRegistrationName(),
              transport.GetLocalAddress(),
              endpoint.GetDefaultSignalPort());
  SIPURL contact(name.GetUserName(),
                 transport.GetLocalAddress(),
                 endpoint.GetDefaultSignalPort());
  request.BuildREGISTER(endpoint, name, contact);

  return request.Write();
}


BOOL SIPEndPoint::Register(const PString & server)
{
  if (listeners.IsEmpty())
    return FALSE;

  OpalTransportAddress address(server, defaultSignalPort, OpalTransportAddress::UdpPrefix);

  OpalTransport * transport = address.CreateTransport(*this, OpalTransportAddress::NoBinding);
  transport->ConnectTo(address);

  transport->WriteConnect(WriteREGISTER, this);

  SIP_PDU response(*transport);
  transport->SetReadTimeout(15000);
  if (!response.Read()) {
    delete transport;
    return FALSE;
  }

  transport->SetReadTimeout(PMaxTimeInterval);
  transport->EndConnect(transport->GetLocalAddress());

  PTRACE(2, "SIP\tSelecting reply from " << *transport);

  while (response.GetStatusCode() < SIP_PDU::Successful_OK) {
    if (!response.Read())
      break;
  }

  delete transport;
  return response.GetStatusCode() == SIP_PDU::Successful_OK;
}


// End of file ////////////////////////////////////////////////////////////////
