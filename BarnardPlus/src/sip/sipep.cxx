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
 * $Revision$
 * $Author$
 * $Date$
 */

#include <ptlib.h>

#include <opal/buildopts.h>
#ifdef OPAL_SIP

#ifdef __GNUC__
#pragma implementation "sipep.h"
#endif

#include <sip/sipep.h>

#include <ptclib/enum.h>
#include <sip/sippdu.h>
#include <sip/sipcon.h>
#include <opal/manager.h>
#include <opal/call.h>
#include <sip/handlers.h>


#define new PNEW


////////////////////////////////////////////////////////////////////////////

SIPEndPoint::SIPEndPoint(OpalManager & mgr)
: OpalEndPoint(mgr, "sip", CanTerminateCall),
    retryTimeoutMin(500),            // 0.5 seconds
    retryTimeoutMax(0, 4),           // 4 seconds
    nonInviteTimeout(0, 16),         // 16 seconds
    pduCleanUpTimeout(0, 5),         // 5 seconds
    inviteTimeout(0, 32),            // 32 seconds
    ackTimeout(0, 32),               // 32 seconds
    registrarTimeToLive(0, 0, 0, 1), // 1 hour
    notifierTimeToLive(0, 0, 0, 1),  // 1 hour
    natBindingTimeout(0, 0, 1)       // 1 minute
{
  defaultSignalPort = 5060;
  mimeForm = PFalse;
  maxRetries = 10;

  natBindingTimer.SetNotifier(PCREATE_NOTIFIER(NATBindingRefresh));
  natBindingTimer.RunContinuous(natBindingTimeout);

  natMethod = None;

  // Make sure these have been contructed now to avoid
  // payload type disambiguation problems.
  GetOpalRFC2833();
  GetOpalCiscoNSE();

#ifdef P_SSL
  manager.AttachEndPoint(this, "sips");
#endif

  PTRACE(4, "SIP\tCreated endpoint.");
}


SIPEndPoint::~SIPEndPoint()
{
}


void SIPEndPoint::ShutDown()
{
  while (activeSIPHandlers.GetSize() > 0) {
    PSafePtr<SIPHandler> handler = activeSIPHandlers;
    PString aor = handler->GetRemotePartyAddress();
    if (handler->GetMethod() != SIP_PDU::Method_REGISTER || handler->GetState() != SIPHandler::Subscribed)
      activeSIPHandlers.Remove(handler);
    else {
      Unregister(aor);
      PThread::Sleep(500);
    }
  }

  for (PSafePtr<SIPTransaction> transaction(transactions, PSafeReference); transaction != NULL; ++transaction)
    transaction->WaitForCompletion();

  // Clean up
  transactions.RemoveAll();

  // Stop timers before compiler destroys member objects
  natBindingTimer.Stop();

  // Now shut down listeners and aggregators
  OpalEndPoint::ShutDown();
}


PString SIPEndPoint::GetDefaultTransport() const 
{  
  return "udp$,tcp$"
#ifdef P_SSL
         ",tcps$:5061"
#endif
    ; 
}

PBoolean SIPEndPoint::NewIncomingConnection(OpalTransport * transport)
{
  PTRACE_IF(2, transport->IsReliable(), "SIP\tListening thread started.");

  transport->SetBufferSize(SIP_PDU::MaxSize);

  do {
    HandlePDU(*transport);
  } while (transport->IsOpen() && transport->IsReliable() && !transport->bad() && !transport->eof());

  PTRACE_IF(2, transport->IsReliable(), "SIP\tListening thread finished.");

  return PTrue;
}


void SIPEndPoint::TransportThreadMain(PThread &, INT param)
{
  PTRACE(4, "SIP\tRead thread started.");
  OpalTransport * transport = (OpalTransport *)param;

  do {
    HandlePDU(*transport);
  } while (transport->IsOpen() && !transport->bad() && !transport->eof());
  
  PTRACE(4, "SIP\tRead thread finished.");
}


void SIPEndPoint::NATBindingRefresh(PTimer &, INT)
{
  PTRACE(5, "SIP\tNAT Binding refresh started.");

  if (natMethod != None) {
    for (PSafePtr<SIPHandler> handler(activeSIPHandlers, PSafeReadOnly); handler != NULL; ++handler) {

      OpalTransport * transport = handler->GetTransport();
      if (transport == NULL || transport->IsReliable() || GetManager().GetSTUN(transport->GetRemoteAddress().GetHostName()) == NULL)
        continue;

      switch (natMethod) {

        case Options: 
          {
            PStringList emptyRouteSet;
            SIPOptions options(*this, *transport, SIPURL(transport->GetRemoteAddress()).GetHostName());
            options.Write(*transport, options.GetSendAddress(emptyRouteSet));
          }
          break;

        case EmptyRequest:
          transport->Write("\r\n", 2);
          break;

        default:
          break;
      }
    }
  }

  PTRACE(5, "SIP\tNAT Binding refresh finished.");
}


OpalTransport * SIPEndPoint::CreateTransport(const OpalTransportAddress & remoteAddress,
                                             const OpalTransportAddress & localAddress)
{
  OpalTransport * transport = NULL;
  
  for (OpalListenerList::iterator listener = listeners.begin(); listener != listeners.end(); ++listener) {
    if ((transport = listener->CreateTransport(localAddress, remoteAddress)) != NULL)
      break;
  }

  if (transport == NULL) {
    // No compatible listeners, can't use their binding
    transport = remoteAddress.CreateTransport(*this, OpalTransportAddress::NoBinding);
    if (transport == NULL) {
      PTRACE(1, "SIP\tCould not create transport for " << remoteAddress);
      return NULL;
    }
  }

  if (!transport->SetRemoteAddress(remoteAddress)) {
    PTRACE(1, "SIP\tCould not find " << remoteAddress);
    delete transport;
    return NULL;
  }

  PTRACE(4, "SIP\tCreated transport " << *transport);

  transport->SetBufferSize(SIP_PDU::MaxSize);
  if (!transport->Connect()) {
    PTRACE(1, "SIP\tCould not connect to " << remoteAddress << " - " << transport->GetErrorText());
    transport->CloseWait();
    delete transport;
    return NULL;
  }

  transport->SetPromiscuous(OpalTransport::AcceptFromAny);

  if (transport->IsReliable())
    transport->AttachThread(PThread::Create(PCREATE_NOTIFIER(TransportThreadMain),
                                            (INT)transport,
                                            PThread::NoAutoDeleteThread,
                                            PThread::NormalPriority,
                                            "SIP Transport"));
  return transport;
}


void SIPEndPoint::HandlePDU(OpalTransport & transport)
{
  // create a SIP_PDU structure, then get it to read and process PDU
  SIP_PDU * pdu = new SIP_PDU;

  PTRACE(4, "SIP\tWaiting for PDU on " << transport);
  if (pdu->Read(transport)) {
    if (OnReceivedPDU(transport, pdu)) 
      return;
  }
  else if (transport.good()) {
    PTRACE(2, "SIP\tMalformed request received on " << transport);
    SendResponse(SIP_PDU::Failure_BadRequest, transport, *pdu);
  }

  delete pdu;
}


static PString TranslateENUM(const PString & remoteParty)
{
#if P_DNS
  // if there is no '@', and then attempt to use ENUM
  if (remoteParty.Find('@') == P_MAX_INDEX) {

    // make sure the number has only digits
    PString e164 = remoteParty;
    PINDEX pos = e164.Find(':');
    if (pos != P_MAX_INDEX)
      e164.Delete(0, pos+1);
    pos = e164.FindSpan("0123456789*#", e164[0] != '+' ? 0 : 1);
    e164.Delete(pos, P_MAX_INDEX);

    if (!e164.IsEmpty()) {
      PString str;
      if (PDNS::ENUMLookup(e164, "E2U+SIP", str)) {
        PTRACE(4, "SIP\tENUM converted remote party " << remoteParty << " to " << str);
        return str;
      }
    }
  }
#endif

  return remoteParty;
}


PBoolean SIPEndPoint::MakeConnection(OpalCall & call,
                                 const PString & remoteParty,
                                 void * userData,
                                 unsigned int options,
                                 OpalConnection::StringOptions * stringOptions)
{
  if ((remoteParty.NumCompare("sip:") != EqualTo) && (remoteParty.NumCompare("sips:") != EqualTo))
    return false;

  if (listeners.IsEmpty())
    return false;

  PStringStream callID;
  OpalGloballyUniqueID id;
  callID << id << '@' << PIPSocket::GetHostName();
  return AddConnection(CreateConnection(call, callID, userData, TranslateENUM(remoteParty), NULL, NULL, options, stringOptions));
}


OpalMediaFormatList SIPEndPoint::GetMediaFormats() const
{
  return OpalMediaFormat::GetAllRegisteredMediaFormats();
}


PBoolean SIPEndPoint::GarbageCollection()
{
  PSafePtr<SIPTransaction> transaction(transactions, PSafeReadOnly);
  while (transaction != NULL) {
    if (transaction->IsTerminated()) {
      PString id = transaction->GetTransactionID();
      ++transaction;
      transactions.RemoveAt(id);
    }
    else
      ++transaction;
  }

  transactions.DeleteObjectsToBeRemoved();
  activeSIPHandlers.DeleteObjectsToBeRemoved();

  return OpalEndPoint::GarbageCollection();
}


PBoolean SIPEndPoint::IsAcceptedAddress(const SIPURL & /*toAddr*/)
{
  return PTrue;
}


SIPConnection * SIPEndPoint::CreateConnection(OpalCall & call,
                                              const PString & token,
                                              void * /*userData*/,
                                              const SIPURL & destination,
                                              OpalTransport * transport,
                                              SIP_PDU * /*invite*/,
                                              unsigned int options,
                                              OpalConnection::StringOptions * stringOptions)
{
  return new SIPConnection(call, *this, token, destination, transport, options, stringOptions);
}


PBoolean SIPEndPoint::SetupTransfer(const PString & token,
                                    const PString & callId,
                                    const PString & remoteParty,
                                    void * userData)
{
  // Make a new connection
  PSafePtr<OpalConnection> otherConnection = GetConnectionWithLock(token, PSafeReference);
  if (otherConnection == NULL)
    return false;

  OpalCall & call = otherConnection->GetCall();

  PTRACE(3, "SIP\tTransferring " << *otherConnection << " to " << remoteParty << " in call " << call);

  OpalConnection::StringOptions options;
  if (!callId.IsEmpty())
    options.SetAt("Replaces", callId);

  PStringStream callID;
  OpalGloballyUniqueID id;
  callID << id << '@' << PIPSocket::GetHostName();
  SIPConnection * connection = CreateConnection(call, callID, userData, TranslateENUM(remoteParty), NULL, NULL, 0, &options);
  if (!AddConnection(connection))
    return false;

  otherConnection->Release(OpalConnection::EndedByCallForwarded);
  otherConnection->CloseMediaStreams();

  return connection->SetUpConnection();
}


PBoolean SIPEndPoint::ForwardConnection(SIPConnection & connection,  
            const PString & forwardParty)
{
  OpalCall & call = connection.GetCall();
  
  PStringStream callID;
  OpalGloballyUniqueID id;
  callID << id << '@' << PIPSocket::GetHostName();

  SIPConnection * conn = CreateConnection(call, callID, NULL, forwardParty, NULL, NULL);
  if (!AddConnection(conn))
    return PFalse;

  call.OnReleased(connection);
  
  conn->SetUpConnection();
  connection.Release(OpalConnection::EndedByCallForwarded);

  return PTrue;
}

PBoolean SIPEndPoint::OnReceivedPDU(OpalTransport & transport, SIP_PDU * pdu)
{
  if (PAssertNULL(pdu) == NULL)
    return PFalse;

  // Adjust the Via list 
  if (pdu->GetMethod() != SIP_PDU::NumMethods)
    pdu->AdjustVia(transport);

  // pass message off to thread pool
  PString callID = pdu->GetMIME().GetCallID();
  if (HasConnection(callID)) {
    SIP_PDU_Work * work = new SIP_PDU_Work;
    work->callID    = callID;
    work->ep        = this;
    work->pdu       = pdu;
    threadPool.AddWork(work);
    return true;
  }

  return OnReceivedConnectionlessPDU(transport, pdu);
}


bool SIPEndPoint::OnReceivedConnectionlessPDU(OpalTransport & transport, SIP_PDU * pdu)
{
  switch (pdu->GetMethod()) {
    case SIP_PDU::NumMethods :
      {
        PSafePtr<SIPTransaction> transaction = GetTransaction(pdu->GetTransactionID(), PSafeReference);
        if (transaction != NULL)
          transaction->OnReceivedResponse(*pdu);
      }
      break;

    case SIP_PDU::Method_INVITE :
      return OnReceivedINVITE(transport, pdu);

    case SIP_PDU::Method_REGISTER :
      return OnReceivedREGISTER(transport, *pdu);

    case SIP_PDU::Method_SUBSCRIBE :
      return OnReceivedSUBSCRIBE(transport, *pdu);

    case SIP_PDU::Method_NOTIFY :
       return OnReceivedNOTIFY(transport, *pdu);

    case SIP_PDU::Method_MESSAGE :
      OnReceivedMESSAGE(transport, *pdu);
      SendResponse(SIP_PDU::Successful_OK, transport, *pdu);
      break;
   
    case SIP_PDU::Method_OPTIONS :
      SendResponse(SIP_PDU::Successful_OK, transport, *pdu);
      break;

    case SIP_PDU::Method_ACK :
      // If we receive an ACK outside of the context of a connection, ignore it.
      break;

    default :
      SendResponse(SIP_PDU::Failure_TransactionDoesNotExist, transport, *pdu);
  }

  return false;
}

PBoolean SIPEndPoint::OnReceivedREGISTER(OpalTransport & transport, SIP_PDU & pdu)
{
  SendResponse(SIP_PDU::Failure_MethodNotAllowed, transport, pdu);
  return PFalse;
}

PBoolean SIPEndPoint::OnReceivedSUBSCRIBE(OpalTransport & transport, SIP_PDU & pdu)
{
  SendResponse(SIP_PDU::Failure_MethodNotAllowed, transport, pdu);
  return PFalse;
}

void SIPEndPoint::OnReceivedResponse(SIPTransaction & transaction, SIP_PDU & response)
{
  PSafePtr<SIPHandler> handler = NULL;
 
  if (transaction.GetMethod() == SIP_PDU::Method_REGISTER
      || transaction.GetMethod() == SIP_PDU::Method_SUBSCRIBE
      || transaction.GetMethod() == SIP_PDU::Method_PUBLISH
      || transaction.GetMethod() == SIP_PDU::Method_MESSAGE) {

    PString callID = transaction.GetMIME().GetCallID ();

    // Have a response to the REGISTER/SUBSCRIBE/MESSAGE, 
    handler = activeSIPHandlers.FindSIPHandlerByCallID (callID, PSafeReadOnly);
    if (handler == NULL) 
      return;
  }

  switch (response.GetStatusCode()) {
    case SIP_PDU::Failure_IntervalTooBrief :
      OnReceivedIntervalTooBrief(transaction, response);
      break;

    case SIP_PDU::Failure_UnAuthorised :
    case SIP_PDU::Failure_ProxyAuthenticationRequired :
      OnReceivedAuthenticationRequired(transaction, response);
      break;

    case SIP_PDU::Failure_RequestTimeout :
      if (handler != NULL)
        handler->OnTransactionFailed(transaction);
      break;

    default :
      switch (response.GetStatusCode()/100) {
        case 1 :
          // Do nothing on 1xx
          break;

        case 2 :
          OnReceivedOK(transaction, response);
          break;

        default :
          // Failure for a SUBSCRIBE/REGISTER/PUBLISH/MESSAGE 
    if (handler != NULL)
      handler->OnFailed(response.GetStatusCode());
          break;
      }
  }
}


PBoolean SIPEndPoint::OnReceivedINVITE(OpalTransport & transport, SIP_PDU * request)
{
  // send provisional response here because creating the connection can take a long time
  // on some systems
  SendResponse(SIP_PDU::Information_Trying, transport, *request);

  SIPMIMEInfo & mime = request->GetMIME();

  // parse the incoming To field, and check if we accept incoming calls for this address
  SIPURL toAddr(mime.GetTo());
  if (!IsAcceptedAddress(toAddr)) {
    PTRACE(2, "SIP\tIncoming INVITE from " << request->GetURI() << " for unknown address " << toAddr);
    SendResponse(SIP_PDU::Failure_NotFound, transport, *request);
    return PFalse;
  }

  // See if we are replacing an existing call.
  OpalCall * call = NULL;
  if (mime.Contains("Replaces")) {
    PString replaces = mime("Replaces");

    PString to;
    static const char toTag[] = ";to-tag=";
    PINDEX pos = replaces.Find(toTag);
    if (pos != P_MAX_INDEX) {
      pos += sizeof(toTag)-1;
      to = replaces(pos, replaces.Find(';', pos)-1).Trim();
    }

    PString from;
    static const char fromTag[] = ";from-tag=";
    pos = replaces.Find(fromTag);
    if (pos != P_MAX_INDEX) {
      pos += sizeof(fromTag)-1;
      from = replaces(pos, replaces.Find(';', pos)-1).Trim();
    }

    PString callid = replaces.Left(replaces.Find(';')).Trim();
    if (callid.IsEmpty() || to.IsEmpty() || from.IsEmpty()) {
      PTRACE(2, "SIP\tBad Replaces header in INVITE from " << request->GetURI());
      SendResponse(SIP_PDU::Failure_BadRequest, transport, *request);
      return false;
    }

    PSafePtr<SIPConnection> replacedConnection = GetSIPConnectionWithLock(callid, PSafeReadOnly);
    if (replacedConnection == NULL ||
        replacedConnection->GetDialogTo().Find(to) == P_MAX_INDEX ||
        replacedConnection->GetDialogFrom().Find(from) == P_MAX_INDEX) {
      PTRACE(2, "SIP\tUnknown " << (replacedConnection != NULL ? "to/from" : "call-id")
             << " in Replaces header of INVITE from " << request->GetURI());
      SendResponse(SIP_PDU::Failure_TransactionDoesNotExist, transport, *request);
      return false;
    }

    // Use the existing call instance when replacing the SIP side of it.
    call = &replacedConnection->GetCall();
    PTRACE(3, "SIP\tIncoming INVITE replaces connection " << *replacedConnection);
  }

  // create and check transport
  OpalTransport * newTransport = CreateTransport(transport.GetRemoteAddress(), transport.GetInterface());
  if (newTransport == NULL) {
    PTRACE(1, "SIP\tFailed to create transport for SIPConnection for INVITE from " << request->GetURI() << " for " << toAddr);
    SendResponse(SIP_PDU::Failure_NotFound, transport, *request);
    return PFalse;
  }

  if (call == NULL) {
    // Get new instance of a call, abort if none created
    call = manager.InternalCreateCall();
    if (call == NULL) {
      SendResponse(SIP_PDU::Failure_TemporarilyUnavailable, transport, *request);
      return false;
    }
  }

  // ask the endpoint for a connection
  SIPConnection *connection = CreateConnection(*call,
                                               mime.GetCallID(),
                                               NULL,
                                               request->GetURI(),
                                               newTransport,
                                               request);
  if (!AddConnection(connection)) {
    PTRACE(1, "SIP\tFailed to create SIPConnection for INVITE from " << request->GetURI() << " for " << toAddr);
    SendResponse(SIP_PDU::Failure_NotFound, transport, *request);
    return PFalse;
  }

  // Get the connection to handle the rest of the INVITE in the thread pool
  SIP_PDU_Work * work = new SIP_PDU_Work;
  work->ep        = this;
  work->pdu       = request;
  work->callID    = mime.GetCallID();
  threadPool.AddWork(work);

  return PTrue;
}

void SIPEndPoint::OnReceivedIntervalTooBrief(SIPTransaction & transaction, SIP_PDU & response)
{
  const SIPMIMEInfo & responseMIME = response.GetMIME();
  PSafePtr<SIPHandler> handler = activeSIPHandlers.FindSIPHandlerByCallID(responseMIME.GetCallID(), PSafeReadOnly);
  if (handler == NULL)
    return;

  SIPTransaction *newTransaction = handler->CreateTransaction(transaction.GetTransport());
  if (newTransaction) {
    handler->SetExpire(responseMIME.GetMinExpires());
    newTransaction->GetMIME().SetExpires(responseMIME.GetMinExpires());
    newTransaction->GetMIME().SetCallID(handler->GetCallID());
    if (newTransaction->Start())
      return;
  }

  PTRACE(1, "SIP\t Could not restart REGISTER after IntervalTooBrief error!");
  handler->OnFailed(SIP_PDU::Failure_IntervalTooBrief);
  return;
}

void SIPEndPoint::OnReceivedAuthenticationRequired(SIPTransaction & transaction, SIP_PDU & response)
{
  // Try to find authentication information for the given call ID
  PSafePtr<SIPHandler> handler = activeSIPHandlers.FindSIPHandlerByCallID(response.GetMIME().GetCallID(), PSafeReadOnly);
  if (handler == NULL)
    return;
  
  handler->OnReceivedAuthenticationRequired(transaction, response);
}


void SIPEndPoint::OnReceivedOK(SIPTransaction & transaction, SIP_PDU & response)
{
  PSafePtr<SIPHandler> handler = activeSIPHandlers.FindSIPHandlerByCallID(response.GetMIME().GetCallID(), PSafeReadOnly);
  if (handler == NULL) 
    return;
  
  handler->OnReceivedOK(transaction, response);
}
    

void SIPEndPoint::OnTransactionFailed(SIPTransaction & transaction)
{
  PSafePtr<SIPHandler> handler = activeSIPHandlers.FindSIPHandlerByCallID(transaction.GetMIME().GetCallID(), PSafeReadOnly);
  if (handler == NULL) 
    return;
  
  handler->OnTransactionFailed(transaction);
}


PBoolean SIPEndPoint::OnReceivedNOTIFY (OpalTransport & transport, SIP_PDU & pdu)
{
  PCaselessString state;
  SIPSubscribe::SubscribeType event;
  
  PTRACE(3, "SIP\tReceived NOTIFY");
  
  if (pdu.GetMIME().GetEvent().Find("message-summary") != P_MAX_INDEX)
    event = SIPSubscribe::MessageSummary;
  else if (pdu.GetMIME().GetEvent().Find("presence") != P_MAX_INDEX)
    event = SIPSubscribe::Presence;
  else {

    // Only support MWI and presence Subscribe for now, 
    // other events are unrequested
    SendResponse(SIP_PDU::Failure_BadEvent, transport, pdu);

    return PFalse;
  }

    
  // A NOTIFY will have the same CallID than the SUBSCRIBE request
  // it corresponds to
  PSafePtr<SIPHandler> handler = activeSIPHandlers.FindSIPHandlerByCallID(pdu.GetMIME().GetCallID(), PSafeReadOnly);
  if (handler == NULL) {

    if (event == SIPSubscribe::Presence) {
      PTRACE(3, "SIP\tCould not find a SUBSCRIBE corresponding to the NOTIFY");
      SendResponse(SIP_PDU::Failure_TransactionDoesNotExist, transport, pdu);
      return PFalse;
    }
    if (event == SIPSubscribe::MessageSummary) {
      PTRACE(3, "SIP\tCould not find a SUBSCRIBE corresponding to the NOTIFY : Work around Asterisk bug");
      SIPURL url_from (pdu.GetMIME().GetFrom());
      SIPURL url_to (pdu.GetMIME().GetTo());
      PString to = url_to.GetUserName() + "@" + url_from.GetHostName();
      handler = activeSIPHandlers.FindSIPHandlerByUrl(to, SIP_PDU::Method_SUBSCRIBE, event, PSafeReadOnly);
      if (handler == NULL) {
        SendResponse(SIP_PDU::Failure_TransactionDoesNotExist, transport, pdu);
        return PFalse;
      }
    }
  }
  if (!handler->OnReceivedNOTIFY(pdu))
    return PFalse;

  return PTrue;
}


void SIPEndPoint::OnReceivedMESSAGE(OpalTransport & /*transport*/, 
            SIP_PDU & pdu)
{
  PString from = pdu.GetMIME().GetFrom();
  PINDEX j = from.Find (';');
  if (j != P_MAX_INDEX)
    from = from.Left(j); // Remove all parameters
  j = from.Find ('<');
  if (j != P_MAX_INDEX && from.Find ('>') == P_MAX_INDEX)
    from += '>';

  OnMessageReceived(from, pdu.GetEntityBody());
}


void SIPEndPoint::OnRegistrationStatus(const PString & aor,
                                       PBoolean wasRegistering,
                                       PBoolean /*reRegistering*/,
                                       SIP_PDU::StatusCodes reason)
{
  if (reason == SIP_PDU::Successful_OK)
    OnRegistered(aor, wasRegistering);
  else
    OnRegistrationFailed(aor, reason, wasRegistering);
}


void SIPEndPoint::OnRegistrationFailed(const PString & /*aor*/, 
               SIP_PDU::StatusCodes /*reason*/, 
               PBoolean /*wasRegistering*/)
{
}
    

void SIPEndPoint::OnRegistered(const PString & /*aor*/, 
             PBoolean /*wasRegistering*/)
{
}


PBoolean SIPEndPoint::IsRegistered(const PString & url) 
{
  PSafePtr<SIPHandler> handler = activeSIPHandlers.FindSIPHandlerByUrl(url, SIP_PDU::Method_REGISTER, PSafeReadOnly);
  if (handler == NULL)
    return PFalse;
  
  return (handler->GetState() == SIPHandler::Subscribed);
}


PBoolean SIPEndPoint::IsSubscribed(SIPSubscribe::SubscribeType type,
                               const PString & to) 
{
  PSafePtr<SIPHandler> handler = activeSIPHandlers.FindSIPHandlerByUrl(to, SIP_PDU::Method_SUBSCRIBE, type, PSafeReadOnly);
  if (handler == NULL)
    return PFalse;

  return (handler->GetState() == SIPHandler::Subscribed);
}


bool SIPEndPoint::Register(const PString & host,
                           const PString & user,
                           const PString & authName,
                           const PString & password,
                           const PString & realm,
                           unsigned expire,
                           const PTimeInterval & minRetryTime,
                           const PTimeInterval & maxRetryTime)
{
  SIPRegister::Params params;

  if (user.Find('@') == P_MAX_INDEX)
    params.m_addressOfRecord = user + '@' + host;
  else {
    params.m_addressOfRecord = user;
    if (!host.IsEmpty())
      params.m_addressOfRecord += ";proxy=" + host;
  }

  params.m_authID = authName;
  params.m_password = password;
  params.m_realm = realm;
  params.m_expire = expire;
  params.m_minRetryTime = minRetryTime;
  params.m_maxRetryTime = maxRetryTime;

  return Register(params);
}


bool SIPEndPoint::Register(const SIPRegister::Params & params)
{
  PSafePtr<SIPRegisterHandler> handler = PSafePtrCast<SIPHandler, SIPRegisterHandler>(
          activeSIPHandlers.FindSIPHandlerByUrl(params.m_addressOfRecord, SIP_PDU::Method_REGISTER, PSafeReadOnly));

  // If there is already a request with this URL and method, 
  // then update it with the new information
  if (handler != NULL)
    handler->UpdateParameters(params);
  else {
    // Otherwise create a new request with this method type
    handler = CreateRegisterHandler(params);
    activeSIPHandlers.Append(handler);
  }

  if (handler->SendRequest(SIPHandler::Subscribing))
    return true;

  activeSIPHandlers.Remove(handler);
  return false;
}


SIPRegisterHandler * SIPEndPoint::CreateRegisterHandler(const SIPRegister::Params & params)
{
  return new SIPRegisterHandler(*this, params);
}


PBoolean SIPEndPoint::Unregister(const PString & addressOfRecord)
{
  PSafePtr<SIPHandler> handler = activeSIPHandlers.FindSIPHandlerByUrl(addressOfRecord, SIP_PDU::Method_REGISTER, PSafeReadOnly);
  if (handler == NULL) {
    PTRACE(1, "SIP\tCould not find active REGISTER for " << addressOfRecord);
    return false;
  }

  return handler->SendRequest(SIPHandler::Unsubscribing);
}


bool SIPEndPoint::UnregisterAll()
{
  bool ok = true;

  for (PSafePtr<SIPHandler> handler(activeSIPHandlers, PSafeReadOnly); handler != NULL; ++handler) {
    if (handler->GetMethod() == SIP_PDU::Method_REGISTER) {
      if (!handler->SendRequest(SIPHandler::Unsubscribing))
        ok = false;
    }
  }

  return ok;
}


PBoolean SIPEndPoint::Subscribe(SIPSubscribe::SubscribeType & type,
                            unsigned expire,
                            const PString & to)
{
  // Create the SIPHandler structure
  PSafePtr<SIPHandler> handler = activeSIPHandlers.FindSIPHandlerByUrl(to, SIP_PDU::Method_SUBSCRIBE, type, PSafeReadOnly);
  
  // If there is already a request with this URL and method, 
  // then update it with the new information
  if (handler != NULL) {
    handler->SetExpire(expire);      // Adjust the expire field
  } 
  // Otherwise create a new request with this method type
  else {
    handler = new SIPSubscribeHandler(*this, 
                                      type,
                                      to,
                                      expire);
    activeSIPHandlers.Append(handler);
  }

  return handler->SendRequest(SIPHandler::Subscribing);
}


PBoolean SIPEndPoint::Unsubscribe(SIPSubscribe::SubscribeType & type,
                              const PString & to)
{
  // Create the SIPHandler structure
  PSafePtr<SIPHandler> handler = activeSIPHandlers.FindSIPHandlerByUrl(to, SIP_PDU::Method_SUBSCRIBE, type, PSafeReadOnly);
  if (handler == NULL) {
    PTRACE(1, "SIP\tCould not find active SUBSCRIBE for " << to);
    return PFalse;
  }

  return handler->SendRequest(SIPHandler::Unsubscribing);
}



PBoolean SIPEndPoint::Message (const PString & to, 
                           const PString & body)
{
  // Create the SIPHandler structure
  PSafePtr<SIPHandler> handler = activeSIPHandlers.FindSIPHandlerByUrl(to, SIP_PDU::Method_MESSAGE, PSafeReadOnly);
  
  // Otherwise create a new request with this method type
  if (handler != NULL)
    handler->SetBody(body);
  else {
    handler = new SIPMessageHandler(*this, 
                                    to,
                                    body);
    activeSIPHandlers.Append(handler);
  }

  return handler->SendRequest(SIPHandler::Subscribing);
}


PBoolean SIPEndPoint::Publish(const PString & to,
                          const PString & body,
                          unsigned expire)
{
  // Create the SIPHandler structure
  PSafePtr<SIPHandler> handler = activeSIPHandlers.FindSIPHandlerByUrl(to, SIP_PDU::Method_PUBLISH, PSafeReadOnly);

  // Otherwise create a new request with this method type
  if (handler != NULL) {
    handler->SetBody(body);
    return true; // Let background timer send message
  }

  handler = new SIPPublishHandler(*this,
                                  to,
                                  body,
                                  expire);
  activeSIPHandlers.Append(handler);

  return handler->SendRequest(SIPHandler::Subscribing);
}


PBoolean SIPEndPoint::Ping(const PString & to)
{
  // Create the SIPHandler structure
  PSafePtr<SIPHandler> handler = activeSIPHandlers.FindSIPHandlerByUrl(to, SIP_PDU::Method_PING, PSafeReadOnly);

  // Otherwise create a new request with this method type
  if (handler == NULL) {
    handler = new SIPPingHandler(*this,
                                 to);
  }

  return handler->SendRequest(SIPHandler::Subscribing);
}


void SIPEndPoint::OnMessageReceived (const SIPURL & /*from*/,
             const PString & /*body*/)
{
}


void SIPEndPoint::OnPresenceInfoReceived (const PString & /*user*/,
                                          const PString & /*basic*/,
                                          const PString & /*note*/)
{
}


void SIPEndPoint::OnMessageFailed(const SIPURL & /* messageUrl */,
          SIP_PDU::StatusCodes /* reason */)
{
}


void SIPEndPoint::SetProxy(const PString & hostname,
                           const PString & username,
                           const PString & password)
{
  PStringStream str;
  if (!hostname) {
    str << "sip:";
    if (!username) {
      str << username;
      if (!password)
        str << ':' << password;
      str << '@';
    }
    str << hostname;
  }
  proxy = str;
}


void SIPEndPoint::SetProxy(const SIPURL & url) 
{ 
  proxy = url; 
}


PString SIPEndPoint::GetUserAgent() const 
{
  return userAgentString;
}


PBoolean SIPEndPoint::GetAuthentication(const PString & realm, SIPAuthentication &auth) 
{
  PSafePtr<SIPHandler> handler = activeSIPHandlers.FindSIPHandlerByAuthRealm(realm, PString::Empty(), PSafeReadOnly);
  if (handler == NULL)
    return PFalse;

  auth = handler->GetAuthentication();

  return PTrue;
}


SIPURL SIPEndPoint::GetRegisteredPartyName(const SIPURL & url)
{
  PSafePtr<SIPHandler> handler = activeSIPHandlers.FindSIPHandlerByDomain(url.GetHostName(), SIP_PDU::Method_REGISTER, PSafeReadOnly);
  if (handler == NULL) 
    return GetDefaultRegisteredPartyName();

  return handler->GetTargetAddress();
}


SIPURL SIPEndPoint::GetDefaultRegisteredPartyName()
{
  SIPURL rpn;

  {
    PString hostName = PIPSocket::GetHostName();
    PIPSocket::Address dummy;
    if (!PIPSocket::GetHostAddress(hostName, dummy)) 
      rpn = SIPURL(PString("sip:") + GetDefaultLocalPartyName() + "@" + hostName + PString(PString::Unsigned, GetDefaultSignalPort()) + ";transport=udp");
    else {
      OpalTransportAddress addr(hostName, GetDefaultSignalPort(), "udp");
      rpn = SIPURL(GetDefaultLocalPartyName(), addr, GetDefaultSignalPort());
    }
  }

  rpn.SetDisplayName(GetDefaultDisplayName());
  return rpn;
}

SIPURL SIPEndPoint::GetContactURL(const OpalTransport &transport, const PString & userName, const PString & host)
{
  PSafePtr<SIPHandler> handler = activeSIPHandlers.FindSIPHandlerByDomain(host, SIP_PDU::Method_REGISTER, PSafeReadOnly);
  if (handler != NULL) {
    OpalTransport * handlerTransport = handler->GetTransport();
    if (handlerTransport != NULL)
      return GetLocalURL(*handlerTransport, handler->GetTargetAddress().GetUserName());
  }

    return GetLocalURL(transport, userName);
}


SIPURL SIPEndPoint::GetLocalURL(const OpalTransport &transport, const PString & userName)
{
  PIPSocket::Address ip(PIPSocket::GetDefaultIpAny());
  OpalTransportAddress contactAddress = transport.GetLocalAddress();
  WORD contactPort = GetDefaultSignalPort();
  if (transport.IsOpen())
    transport.GetLocalAddress().GetIpAndPort(ip, contactPort);
  else {
    for (OpalListenerList::iterator listener = listeners.begin(); listener != listeners.end(); ++listener) {
      OpalTransportAddress binding = listener->GetLocalAddress();
      if (transport.IsCompatibleTransport(binding)) {
        binding.GetIpAndPort(ip, contactPort);
        break;
      }
    }
  }

  PIPSocket::Address localIP;
  WORD localPort;
  
  if (contactAddress.GetIpAndPort(localIP, localPort)) {
    PIPSocket::Address remoteIP;
    if (transport.GetRemoteAddress().GetIpAddress(remoteIP)) {
      GetManager().TranslateIPAddress(localIP, remoteIP);    
      contactPort = localPort;
      PString proto = transport.GetProtoPrefix();
      contactAddress = OpalTransportAddress(localIP, contactPort, proto.Left(proto.GetLength()-1)); //xxxxxx
    }
  }

  SIPURL contact(userName, contactAddress, contactPort);

  return contact;
}


PBoolean SIPEndPoint::SendResponse(SIP_PDU::StatusCodes code, OpalTransport & transport, SIP_PDU & pdu)
{
  SIP_PDU response(pdu, code);
  PString username = SIPURL(response.GetMIME().GetTo()).GetUserName();
  response.GetMIME().SetContact(GetLocalURL(transport, username));
  return response.Write(transport, pdu.GetViaAddress(*this));
}


void SIPEndPoint::OnRTPStatistics(const SIPConnection & connection,
                                  const RTP_Session & session) const
{
  manager.OnRTPStatistics(connection, session);
}

SIPEndPoint::SIP_PDU_Thread::SIP_PDU_Thread(PThreadPoolBase & _pool)
  : PThreadPoolWorkerBase(_pool) { }

unsigned SIPEndPoint::SIP_PDU_Thread::GetWorkSize() const { return pduQueue.size(); }

void SIPEndPoint::SIP_PDU_Thread::OnAddWork(SIP_PDU_Work * work)
{
  PWaitAndSignal m(mutex);
  pduQueue.push(work);
  if (pduQueue.size() == 1)
    sync.Signal();
}

void SIPEndPoint::SIP_PDU_Thread::OnRemoveWork(SIP_PDU_Work *) 
{ }

void SIPEndPoint::SIP_PDU_Thread::Shutdown()
{
  shutdown = true;
  sync.Signal();
}


void SIPEndPoint::SIP_PDU_Thread::Main()
{
  while (!shutdown) {
    mutex.Wait();
    if (pduQueue.size() == 0) {
      mutex.Signal();
      sync.Wait();
      continue;
    }

    SIP_PDU_Work * work = pduQueue.front();
    pduQueue.pop();
    mutex.Signal();

    if (!work->callID.IsEmpty()) {
      PSafePtr<SIPConnection> connection = work->ep->GetSIPConnectionWithLock(work->callID, PSafeReference);
      if (connection != NULL)
        connection->OnReceivedPDU(*work->pdu);
    }

    delete work->pdu;
    delete work;
  }
}

#endif // OPAL_SIP

// End of file ////////////////////////////////////////////////////////////////
