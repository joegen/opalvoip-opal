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
 * Revision 1.2016  2004/03/13 06:51:32  rjongbloed
 * Alllowed for empty "username" in registration
 *
 * Revision 2.14  2004/03/13 06:32:18  rjongbloed
 * Fixes for removal of SIP and H.323 subsystems.
 * More registration work.
 *
 * Revision 2.13  2004/03/09 12:09:56  rjongbloed
 * More work on SIP register.
 *
 * Revision 2.12  2004/02/17 12:41:54  csoutheren
 * Use correct Contact field when behind NAT
 *
 * Revision 2.11  2003/12/20 12:21:18  rjongbloed
 * Applied more enhancements, thank you very much Ted Szoczei
 *
 * Revision 2.10  2003/03/24 04:32:58  robertj
 * SIP register must have a server address
 *
 * Revision 2.9  2003/03/06 03:57:47  robertj
 * IVR support (work in progress) requiring large changes everywhere.
 *
 * Revision 2.8  2002/10/09 04:27:44  robertj
 * Fixed memory leak on error reading PDU, thanks Ted Szoczei
 *
 * Revision 2.7  2002/09/12 06:58:34  robertj
 * Removed protocol prefix strings as static members as has problems with
 *   use in DLL environment.
 *
 * Revision 2.6  2002/07/04 07:41:47  robertj
 * Fixed memory/thread leak of transports.
 *
 * Revision 2.5  2002/04/10 03:15:17  robertj
 * Fixed using incorrect field as default reply address on receiving a respons PDU.
 *
 * Revision 2.4  2002/04/09 08:04:01  robertj
 * Fixed typo in previous patch!
 *
 * Revision 2.3  2002/04/09 07:18:33  robertj
 * Fixed the default return address for PDU requests not attached to connection.
 *
 * Revision 2.2  2002/04/05 10:42:04  robertj
 * Major changes to support transactions (UDP timeouts and retries).
 *
 * Revision 2.1  2002/02/01 04:53:01  robertj
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

#include <opal/manager.h>


#define new PNEW


////////////////////////////////////////////////////////////////////////////

SIPEndPoint::SIPEndPoint(OpalManager & mgr)
  : OpalEndPoint(mgr, "sip", CanTerminateCall),
    registrationID(OpalGloballyUniqueID().AsString()),
    retryTimeoutMin(500),     // 0.5 seconds
    retryTimeoutMax(0, 4),    // 4 seconds
    nonInviteTimeout(0, 16),  // 16 seconds
    pduCleanUpTimeout(0, 5),  // 5 seconds
    inviteTimeout(0, 32),     // 32 seconds
    ackTimeout(0, 32)         // 32 seconds
{
  defaultSignalPort = 5060;
  mimeForm = FALSE;
  maxRetries = 10;
  lastSentCSeq = 0;

  PTRACE(3, "SIP\tCreated endpoint.");
}


SIPEndPoint::~SIPEndPoint()
{
  // Shut down the listeners as soon as possible to avoid race conditions
  listeners.RemoveAll();

  PTRACE(3, "SIP\tDeleted endpoint.");
}


BOOL SIPEndPoint::NewIncomingConnection(OpalTransport * transport)
{
  PTRACE_IF(2, transport->IsReliable(), "SIP\tListening thread started.");

  transport->SetBufferSize(SIP_PDU::MaxSize);

  do {
    HandlePDU(*transport);
  } while (transport->IsOpen() && transport->IsReliable());

  PTRACE_IF(2, transport->IsReliable(), "SIP\tListening thread finished.");

  return TRUE;
}


void SIPEndPoint::HandlePDU(OpalTransport & transport)
{
  // create a SIP_PDU structure, then get it to read and process PDU
  SIP_PDU * pdu = new SIP_PDU;

  PTRACE(4, "SIP\tWaiting for PDU on " << transport);
  if (pdu->Read(transport)) {
    if (!transport.IsReliable()) {
      // Calculate default return address
      if (pdu->GetMethod() != SIP_PDU::NumMethods) {
        PString via = pdu->GetMIME().GetVia();
        transport.SetRemoteAddress(via.Mid(via.FindLast(' ')));
      }
    }
    if (OnReceivedPDU(transport, pdu))
      return;
  }
  else if (transport.good()) {
    PTRACE(1, "SIP\tMalformed request received on " << transport);
    SIP_PDU response(*pdu, SIP_PDU::Failure_BadRequest);
    response.Write(transport);
  }

  delete pdu;
}


BOOL SIPEndPoint::MakeConnection(OpalCall & call,
                                 const PString & remoteParty,
                                 void * userData)
{
  if (remoteParty.Find("sip:") != 0)
    return FALSE;

  PStringStream callID;
  OpalGloballyUniqueID id;
  callID << id << '@' << PIPSocket::GetHostName();
  SIPConnection * connection = CreateConnection(call, callID, userData, remoteParty, NULL, NULL);
  if (connection == NULL)
    return FALSE;

  AddNewConnection(connection);

  // If we are the A-party then need to initiate a call now in this thread. If
  // we are the B-Party then SetUpConnection() gets called in the context of
  // the A-party thread.
  if (&call.GetConnection(0) == connection)
    connection->SetUpConnection();

  return TRUE;
}


BOOL SIPEndPoint::IsAcceptedAddress(const SIPURL & /*toAddr*/)
{
  return TRUE;
}


SIPConnection * SIPEndPoint::CreateConnection(OpalCall & call,
                                              const PString & token,
                                              void * /*userData*/,
                                              const SIPURL & destination,
                                              OpalTransport * transport,
                                              SIP_PDU * /*invite*/)
{
  return new SIPConnection(call, *this, token, destination, transport);
}


void SIPEndPoint::AddNewConnection(SIPConnection * conn)
{
  PWaitAndSignal m(inUseFlag);
  connectionsActive.SetAt(conn->GetToken(), conn);
}


BOOL SIPEndPoint::OnReceivedPDU(OpalTransport & transport, SIP_PDU * pdu)
{
  SIPConnection * connection = GetSIPConnectionWithLock(pdu->GetMIME().GetCallID());
  if (connection != NULL) {
    connection->QueuePDU(pdu);
    connection->Unlock();
    return TRUE;
  }

  // An INVITE is a special case
  switch (pdu->GetMethod()) {
    case SIP_PDU::Method_INVITE :
      return OnReceivedINVITE(transport, pdu);

    case SIP_PDU::Method_REGISTER :
      {
        SIP_PDU response(*pdu, SIP_PDU::Failure_MethodNotAllowed);
        response.GetMIME().SetAt("Allow", "INVITE");
        response.Write(transport);
      }
      break;

    default :
      {
        SIP_PDU response(*pdu, SIP_PDU::Failure_TransactionDoesNotExist);
        response.Write(transport);
      }
  }

  return FALSE;
}


BOOL SIPEndPoint::OnReceivedINVITE(OpalTransport & transport, SIP_PDU * request)
{
  SIPMIMEInfo & mime = request->GetMIME();

  // parse the incoming To field, and check if we accept incoming calls for this address
  SIPURL toAddr(mime.GetTo());
  if (!IsAcceptedAddress(toAddr)) {
    PTRACE(1, "SIP\tIncoming INVITE from " << request->GetURI() << " for unknown address " << toAddr);
    SIP_PDU response(*request, SIP_PDU::Failure_NotFound);
    response.Write(transport);
    return FALSE;
  }

  // ask the endpoint for a connection
  SIPConnection * connection = CreateConnection(*GetManager().CreateCall(), mime.GetCallID(),
                                                NULL, request->GetURI(), &transport, request);
  if (connection == NULL) {
    PTRACE(2, "SIP\tFailed to create SIPConnection for INVITE from " << request->GetURI() << " for " << toAddr);
    SIP_PDU response(*request, SIP_PDU::Failure_NotFound);
    response.Write(transport);
    return FALSE;
  }

  // add the connection to the endpoint list
  AddNewConnection(connection);

  // Get the connection to handle the rest of the INVITE
  connection->QueuePDU(request);
  return TRUE;
}


struct WriteRegisterParam
{
  SIPEndPoint  * endpoint;
  const SIPURL * address;
};

static BOOL WriteREGISTER(OpalTransport & transport, void * param)
{
  SIPEndPoint & endpoint = *((WriteRegisterParam *)param)->endpoint;
  const SIPURL & address = *((WriteRegisterParam *)param)->address;

  SIPTransaction request(endpoint, transport);

  // translate contact address
  OpalTransportAddress contactAddress = transport.GetLocalAddress();
  WORD contactPort = endpoint.GetDefaultSignalPort();

  PIPSocket::Address localIP;
  if (transport.GetLocalAddress().GetIpAddress(localIP)) {
    PIPSocket::Address remoteIP;
    if (transport.GetRemoteAddress().GetIpAddress(remoteIP)) {
      endpoint.GetManager().TranslateIPAddress(localIP, remoteIP);
      contactAddress = OpalTransportAddress(localIP, contactPort, "udp");
    }
  }

  SIPURL contact(address.GetUserName(), contactAddress, contactPort);
  request.BuildREGISTER(address, contact);

  return request.Start();
}


BOOL SIPEndPoint::Register(const PString & hostname,
                           const PString & username,
                           const PString & password)
{
  if (listeners.IsEmpty())
    return FALSE;

  PString adjustedUsername = username;
  if (adjustedUsername.IsEmpty())
    adjustedUsername = GetDefaultLocalPartyName();

  if (adjustedUsername.Find('@') == P_MAX_INDEX)
    adjustedUsername += '@' + hostname;

  SIPURL registrationAddress = adjustedUsername;

  OpalTransportAddress registrarAddress(hostname, defaultSignalPort, "udp");
  // Should do DNS SRV record loolup to get regisrar address

  OpalTransport * transport = registrarAddress.CreateTransport(*this, OpalTransportAddress::NoBinding);
  transport->ConnectTo(registrarAddress);

  WriteRegisterParam params;
  params.endpoint = this;
  params.address = &registrationAddress;
  transport->WriteConnect(WriteREGISTER, &params);

  SIP_PDU response;
  transport->SetReadTimeout(30000);
  if (!response.Read(*transport)) {
    delete transport;
    return FALSE;
  }

  transport->SetReadTimeout(PMaxTimeInterval);
  transport->EndConnect(transport->GetLocalAddress());

  PTRACE(2, "SIP\tSelecting reply from " << *transport);

  while (response.GetStatusCode() < SIP_PDU::Successful_OK) {
    if (!response.Read(*transport))
      break;
  }

  delete transport;
  return response.GetStatusCode() == SIP_PDU::Successful_OK;
}


// End of file ////////////////////////////////////////////////////////////////
