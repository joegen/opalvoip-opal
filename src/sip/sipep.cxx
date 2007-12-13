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
#include <ptclib/enum.h>

#ifdef __GNUC__
#pragma implementation "sipep.h"
#endif


#include <sip/sipep.h>

#include <sip/sippdu.h>
#include <sip/sipcon.h>

#include <opal/manager.h>
#include <opal/call.h>

#include <sip/handlers.h>


#define new PNEW

 
////////////////////////////////////////////////////////////////////////////

SIPEndPoint::SIPEndPoint(OpalManager & mgr)
  : OpalEndPoint(mgr, "sip", CanTerminateCall),
    retryTimeoutMin(500),     // 0.5 seconds
    retryTimeoutMax(0, 4),    // 4 seconds
    nonInviteTimeout(0, 16),  // 16 seconds
    pduCleanUpTimeout(0, 5),  // 5 seconds
    inviteTimeout(0, 32),     // 32 seconds
    ackTimeout(0, 32),        // 32 seconds
    registrarTimeToLive(0, 0, 0, 1), // 1 hour
    notifierTimeToLive(0, 0, 0, 1), // 1 hour
    natBindingTimeout(0, 0, 1) // 1 minute
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

  PTRACE(4, "SIP\tCreated endpoint.");
}


SIPEndPoint::~SIPEndPoint()
{
  while (activeSIPHandlers.GetSize() > 0) {
    PSafePtr<SIPHandler> handler = activeSIPHandlers;
    PString aor = handler->GetRemotePartyAddress();
    if (handler->GetMethod() == SIP_PDU::Method_REGISTER && handler->GetState()  == SIPHandler::Subscribed) {
        Unregister(aor);
      PThread::Sleep(500);
      }
    else
      activeSIPHandlers.Remove(handler);
    }

  for (PSafePtr<SIPTransaction> transaction(transactions, PSafeReference); transaction != NULL; ++transaction)
    transaction->WaitForCompletion();

  // Clean up
  transactions.RemoveAll();
  listeners.RemoveAll();

  // Stop timers before compiler destroys member objects
  natBindingTimer.Stop();
  
  PTRACE(4, "SIP\tDeleted endpoint.");
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

  if (natMethod == None)
    return;

  for (PSafePtr<SIPHandler> handler(activeSIPHandlers, PSafeReadOnly); handler != NULL; ++handler) {

    OpalTransport & transport = handler->GetTransport();
    PBoolean stunTransport = PFalse;

    if (!transport)
      return;

    stunTransport = (!transport.IsReliable() && GetManager().GetSTUN(transport.GetRemoteAddress().GetHostName()));

    if (!stunTransport)
      return;

    switch (natMethod) {

      case Options: 
        {
          PStringList emptyRouteSet;
          SIPOptions options(*this, transport, SIPURL(transport.GetRemoteAddress()).GetHostName());
          options.Write(transport, options.GetSendAddress(emptyRouteSet));
        }
        break;
      case EmptyRequest:
        {
          transport.Write("\r\n", 2);
        }
        break;
      default:
        break;
    }
  }

  PTRACE(5, "SIP\tNAT Binding refresh finished.");
}


OpalTransport * SIPEndPoint::CreateTransport(const OpalTransportAddress & remoteAddress,
                                             const OpalTransportAddress & localAddress)
{
  OpalTransport * transport = NULL;
	
  for (PINDEX i = 0; i < listeners.GetSize(); i++) {
    if ((transport = listeners[i].CreateTransport(localAddress, remoteAddress)) != NULL)
      break;
  }

  if (transport == NULL) {
    // No compatible listeners, can't use their binding
    transport = remoteAddress.CreateTransport(*this, OpalTransportAddress::NoBinding);
    if(transport == NULL) {
      PTRACE(1, "SIP\tCould not create transport for " << remoteAddress);
      return NULL;
    }
  }

  transport->SetRemoteAddress(remoteAddress);
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
                                            "SIP Transport:%x"));
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


PBoolean SIPEndPoint::MakeConnection(OpalCall & call,
                                 const PString & _remoteParty,
                                 void * userData,
                                 unsigned int options,
                                 OpalConnection::StringOptions * stringOptions)
{
  PString remoteParty;
  
  if (_remoteParty.Find("sip:") != 0)
    return PFalse;
  
  ParsePartyName(_remoteParty, remoteParty);

  PStringStream callID;
  OpalGloballyUniqueID id;
  callID << id << '@' << PIPSocket::GetHostName();
  SIPConnection * connection = CreateConnection(call, callID, userData, remoteParty, NULL, NULL, options, stringOptions);
  if (!AddConnection(connection))
    return PFalse;

  // If we are the A-party then need to initiate a call now in this thread. If
  // we are the B-Party then SetUpConnection() gets called in the context of
  // the A-party thread.
  if (call.GetConnection(0) == (OpalConnection*)connection)
    connection->SetUpConnection();

  return PTrue;
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
				const PString & /*callIdentity*/, 
				const PString & remoteParty,  
				void * userData)
{
  // Make a new connection
  PSafePtr<OpalConnection> otherConnection = GetConnectionWithLock(token, PSafeReference);
  if (otherConnection == NULL)
    return false;
  
  OpalCall & call = otherConnection->GetCall();
  
  call.CloseMediaStreams();
  
  PString parsedParty;
  ParsePartyName(remoteParty, parsedParty);

  PStringStream callID;
  OpalGloballyUniqueID id;
  callID << id << '@' << PIPSocket::GetHostName();
  SIPConnection * connection = CreateConnection(call, callID, userData, parsedParty, NULL, NULL);
  if (!AddConnection(connection))
    return false;

  call.OnReleased(*otherConnection);
  
  connection->SetUpConnection();
  otherConnection->Release(OpalConnection::EndedByCallForwarded);

  return true;
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

  // Find a corresponding connection
  PSafePtr<SIPConnection> connection = GetSIPConnectionWithLock(pdu->GetMIME().GetCallID(), PSafeReadWrite);
  if (connection != NULL) {
    connection->QueuePDU(pdu);
    return PTrue;
  }
  
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

  return PFalse;
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
      if (handler != NULL) {
        handler->OnTransactionTimeout(transaction);
      }
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
	  if (handler != NULL) {
	    // Failure for a SUBSCRIBE/REGISTER/PUBLISH/MESSAGE 
	    handler->OnFailed (response.GetStatusCode());
	  }
      }
  }
}


PBoolean SIPEndPoint::OnReceivedINVITE(OpalTransport & transport, SIP_PDU * request)
{
  SIPMIMEInfo & mime = request->GetMIME();

  // parse the incoming To field, and check if we accept incoming calls for this address
  SIPURL toAddr(mime.GetTo());
  if (!IsAcceptedAddress(toAddr)) {
    PTRACE(2, "SIP\tIncoming INVITE from " << request->GetURI() << " for unknown address " << toAddr);
    SendResponse(SIP_PDU::Failure_NotFound, transport, *request);
    return PFalse;
  }
  
  // send provisional response here because creating the connection can take a long time
  // on some systems
  SendResponse(SIP_PDU::Information_Trying, transport, *request);

  // ask the endpoint for a connection
  SIPConnection *connection = CreateConnection(*manager.CreateCall(NULL),
                                               mime.GetCallID(),
                                               NULL,
                                               request->GetURI(),
                                               CreateTransport(transport.GetRemoteAddress(), transport.GetInterface()),
                                               request);
  if (!AddConnection(connection)) {
    PTRACE(1, "SIP\tFailed to create SIPConnection for INVITE from " << request->GetURI() << " for " << toAddr);
    SendResponse(SIP_PDU::Failure_NotFound, transport, *request);
    return PFalse;
  }

  // Get the connection to handle the rest of the INVITE
  connection->QueuePDU(request);
  return PTrue;
}

void SIPEndPoint::OnReceivedIntervalTooBrief(SIPTransaction & transaction, SIP_PDU & response)
{
  SIPTransaction *newTransaction = NULL;
  const SIPMIMEInfo & responseMIME = response.GetMIME();
  PSafePtr<SIPHandler> callid_handler = activeSIPHandlers.FindSIPHandlerByCallID(response.GetMIME().GetCallID(), PSafeReadOnly);

  if (!callid_handler)
    return;

  newTransaction = callid_handler->CreateTransaction(transaction.GetTransport());
  if (newTransaction) {
    callid_handler->SetExpire(responseMIME.GetMinExpires());
    newTransaction->GetMIME().SetExpires(responseMIME.GetMinExpires());
    newTransaction->GetMIME().SetCallID(callid_handler->GetCallID());
    if (newTransaction->Start())
      return;
  }
  PTRACE(1, "SIP\t Could not restart REGISTER after IntervalTooBrief error!");
  callid_handler->OnFailed(SIP_PDU::Failure_IntervalTooBrief);
  return;
}

void SIPEndPoint::OnReceivedAuthenticationRequired(SIPTransaction & transaction, SIP_PDU & response)
{
  PBoolean isProxy = response.GetStatusCode() == SIP_PDU::Failure_ProxyAuthenticationRequired;
  
#if PTRACING
  const char * proxyTrace = isProxy ? "Proxy " : "";
#endif
  PTRACE(3, "SIP\tReceived " << proxyTrace << "Authentication Required response");
  
  // Only support REGISTER and SUBSCRIBE for now
  if (transaction.GetMethod() != SIP_PDU::Method_REGISTER
      && transaction.GetMethod() != SIP_PDU::Method_SUBSCRIBE
      && transaction.GetMethod() != SIP_PDU::Method_MESSAGE) {
    PTRACE(1, "SIP\tCannot do " << proxyTrace << "Authentication Required for non REGISTER, SUBSCRIBE or MESSAGE");
    return;
  }
  
  // Try to find authentication information for the given call ID
  PSafePtr<SIPHandler> callid_handler = activeSIPHandlers.FindSIPHandlerByCallID(response.GetMIME().GetCallID(), PSafeReadOnly);
  if (!callid_handler)
    return;
  
  // Received authentication required response, try to find authentication
  // for the given realm
  SIPAuthentication auth;
  if (!auth.Parse(response.GetMIME()(isProxy 
				     ? "Proxy-Authenticate"
				     : "WWW-Authenticate"),
		  isProxy)) {
    callid_handler->OnFailed(SIP_PDU::Failure_UnAuthorised);
    return;
  }

  // Try to find authentication information for the requested realm
  // That realm is specified when registering
  PSafePtr<SIPHandler> realm_handler = activeSIPHandlers.FindSIPHandlerByAuthRealm(auth.GetAuthRealm(), auth.GetUsername().IsEmpty()?SIPURL(response.GetMIME().GetFrom()).GetUserName():auth.GetUsername(), PSafeReadOnly);

  // No authentication information found for the realm, 
  // use what we have for the given CallID
  // and update the realm
  if (realm_handler == NULL) {
    realm_handler = callid_handler;
    if (!auth.GetAuthRealm().IsEmpty())
      realm_handler->SetAuthRealm(auth.GetAuthRealm());
    PTRACE(3, "SIP\tUpdated realm to " << auth.GetAuthRealm());
  }
  
  // No realm handler, weird
  if (realm_handler == NULL) {
    PTRACE(1, "SIP\tNo Authentication handler found for that realm, authentication impossible");
    return;
  }
  
  PString lastNonce;
  PString lastUsername;
  if (realm_handler->GetAuthentication().IsValid()) {
    lastUsername = realm_handler->GetAuthentication().GetUsername();
    lastNonce = realm_handler->GetAuthentication().GetNonce();
  }

  if (!realm_handler->GetAuthentication().Parse(response.GetMIME()(isProxy ? "Proxy-Authenticate"
								: "WWW-Authenticate"),
					     isProxy)) {
    callid_handler->OnFailed(SIP_PDU::Failure_UnAuthorised);
    return;
  }
  
  // Already sent handler for that callID. Check if params are different
  // from the ones found for the given realm
  if (callid_handler->GetAuthentication().IsValid() &&
      callid_handler->GetAuthentication().GetUsername() == lastUsername &&
      callid_handler->GetAuthentication().GetNonce() == lastNonce &&
      callid_handler->GetState() == SIPHandler::Subscribing) {
    PTRACE(2, "SIP\tAlready done REGISTER/SUBSCRIBE for " << proxyTrace << "Authentication Required");
    callid_handler->OnFailed(SIP_PDU::Failure_UnAuthorised);
    return;
  }
  
  // Abort after some unsuccesful authentication attempts. This is required since
  // some implementations return "401 Unauthorized" with a different nonce at every
  // time.
  unsigned authenticationAttempts = callid_handler->GetAuthenticationAttempts();
  if(authenticationAttempts >= 10) {
    PTRACE(1, "SIP\tAborting after " << authenticationAttempts << " attempts to REGISTER/SUBSCRIBE");
    callid_handler->OnFailed(SIP_PDU::Failure_UnAuthorised);
    return;
  }

  callid_handler->SetAuthenticationAttempts(++authenticationAttempts);

  // Restart the transaction with new authentication handler
  SIPTransaction * newTransaction = callid_handler->CreateTransaction(transaction.GetTransport());
  if (!realm_handler->GetAuthentication().Authorise(*newTransaction)) {
    // don't send again if no authentication handler available
    callid_handler->OnFailed(SIP_PDU::Failure_UnAuthorised);
    delete newTransaction; // Not started yet, we need to delete
    return;
  }

  // Section 8.1.3.5 of RFC3261 tells that the authenticated
  // request SHOULD have the same value of the Call-ID, To and From.
  newTransaction->GetMIME().SetFrom(transaction.GetMIME().GetFrom());
  newTransaction->GetMIME().SetCallID(callid_handler->GetCallID());
  if (!newTransaction->Start()) {
    PTRACE(1, "SIP\tCould not restart REGISTER/SUBSCRIBE for Authentication Required");
  }
}


void SIPEndPoint::OnReceivedOK(SIPTransaction & /*transaction*/, SIP_PDU & response)
{
  PSafePtr<SIPHandler> handler = activeSIPHandlers.FindSIPHandlerByCallID(response.GetMIME().GetCallID(), PSafeReadOnly);
  if (handler == NULL) 
    return;
  
  // reset the number of unsuccesful authentication attempts
  handler->SetAuthenticationAttempts(0);
  handler->OnReceivedOK(response);
}
    

void SIPEndPoint::OnTransactionTimeout(SIPTransaction & transaction)
{
  PSafePtr<SIPHandler> handler = activeSIPHandlers.FindSIPHandlerByCallID(transaction.GetMIME().GetCallID(), PSafeReadOnly);
  if (handler == NULL) 
    return;
  
  handler->OnTransactionTimeout(transaction);
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

PBoolean SIPEndPoint::Register(
      const PString & host,
      const PString & user,
      const PString & authName,
      const PString & password,
      const PString & authRealm,
      unsigned expire,
      const PTimeInterval & minRetryTime, 
      const PTimeInterval & maxRetryTime
)
{
  PString aor;
  if (user.Find('@') != P_MAX_INDEX)
    aor = user;
  else
    aor = user + '@' + host;

  if (expire == 0)
    expire = GetRegistrarTimeToLive().GetSeconds();
  return Register(expire, aor, authName, password, authRealm, minRetryTime, maxRetryTime);
}

PBoolean SIPEndPoint::Register(unsigned expire,
                           const PString & aor,
                           const PString & authName,
                           const PString & password,
                           const PString & realm,
                           const PTimeInterval & minRetryTime, 
                           const PTimeInterval & maxRetryTime)
{
  PSafePtr<SIPHandler> handler = activeSIPHandlers.FindSIPHandlerByUrl(aor, SIP_PDU::Method_REGISTER, PSafeReadOnly);
  
  // If there is already a request with this URL and method, 
  // then update it with the new information
  if (handler != NULL) {
    if (!password.IsEmpty())
      handler->SetPassword(password); // Adjust the password if required 
    if (!realm.IsEmpty())
      handler->SetAuthRealm(realm);   // Adjust the realm if required 
    if (!authName.IsEmpty())
      handler->SetAuthUser(authName); // Adjust the authUser if required 
    handler->SetExpire(expire);      // Adjust the expire field
  } 
  // Otherwise create a new request with this method type
  else {
    handler = CreateRegisterHandler(aor, authName, password, realm, expire, minRetryTime, maxRetryTime);
    activeSIPHandlers.Append(handler);
  }

  if (!handler->SendRequest()) {
    activeSIPHandlers.Remove(handler);
    return PFalse;
  }

  return PTrue;
}

SIPRegisterHandler * SIPEndPoint::CreateRegisterHandler(const PString & aor,
                                                        const PString & authName, 
                                                        const PString & password, 
                                                        const PString & realm,
                                                        int expire,
                                                        const PTimeInterval & minRetryTime, 
                                                        const PTimeInterval & maxRetryTime)
{
  return new SIPRegisterHandler(*this, aor, authName, password, realm, expire, minRetryTime, maxRetryTime);
}

PBoolean SIPEndPoint::Unregister(const PString & aor)
{
  PSafePtr<SIPHandler> handler = activeSIPHandlers.FindSIPHandlerByUrl (aor, SIP_PDU::Method_REGISTER, PSafeReadOnly);
  if (handler == NULL) {
    PTRACE(1, "SIP\tCould not find active REGISTER for " << aor);
    return PFalse;
  }

  if (!handler->GetState() == SIPHandler::Subscribed) {
    handler->SetExpire(-1);
    return PFalse;
  }
      
  return Register(0, aor);
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

  if (!handler->SendRequest()) {
    handler->SetExpire(-1);
    return PFalse;
  }

  return PTrue;
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

  if (!handler->GetState() == SIPHandler::Subscribed) {
    handler->SetExpire(-1);
    return PFalse;
  }
      
  return Subscribe(type, 0, to);
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

  if (!handler->SendRequest()) {
    handler->SetExpire(-1);
    return PFalse;
  }

  return PTrue;
}


PBoolean SIPEndPoint::Publish(const PString & to,
                          const PString & body,
                          unsigned expire)
{
  // Create the SIPHandler structure
  PSafePtr<SIPHandler> handler = activeSIPHandlers.FindSIPHandlerByUrl(to, SIP_PDU::Method_PUBLISH, PSafeReadOnly);

  // Otherwise create a new request with this method type
  if (handler != NULL)
    handler->SetBody(body);
  else {
    handler = new SIPPublishHandler(*this,
                                    to,
                                    body,
                                    expire);
    activeSIPHandlers.Append(handler);

    if (!handler->SendRequest()) {
      handler->SetExpire(-1);
      return PFalse;
    }
  }

  return PTrue;
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

  if (!handler->SendRequest()) {
    handler->SetExpire(-1);
    return PFalse;
  }

  return PTrue;
}


void SIPEndPoint::OnMessageReceived (const SIPURL & /*from*/,
				     const PString & /*body*/)
{
}


void SIPEndPoint::OnMWIReceived (const PString & /*to*/,
				 SIPSubscribe::MWIType /*type*/,
				 const PString & /*msgs*/)
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


void SIPEndPoint::ParsePartyName(const PString & remoteParty, PString & party)
{
  party = remoteParty;
  
#if P_DNS
  // if there is no '@', and then attempt to use ENUM
  if (remoteParty.Find('@') == P_MAX_INDEX) {

    // make sure the number has only digits
    PString e164 = remoteParty;
    if (e164.Left(4) *= "sip:")
      e164 = e164.Mid(4);
    PINDEX i;
    for (i = 0; i < e164.GetLength(); ++i)
      if (!isdigit(e164[i]) && (i != 0 || e164[0] != '+'))
	      break;
    if (i >= e164.GetLength()) {
      PString str;
      if (PDNS::ENUMLookup(e164, "E2U+SIP", str)) {
	      PTRACE(4, "SIP\tENUM converted remote party " << remoteParty << " to " << str);
	      party = str;
      }
    }
  }
#endif
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
  OpalTransportAddress addr(PIPSocket::GetHostName(), GetDefaultSignalPort(), "udp");
  SIPURL rpn(GetDefaultLocalPartyName(), addr, GetDefaultSignalPort());
  rpn.SetDisplayName(GetDefaultDisplayName());
  return rpn;
}


SIPURL SIPEndPoint::GetContactURL(const OpalTransport &transport, const PString & userName, const PString & host)
{
  PSafePtr<SIPHandler> handler = activeSIPHandlers.FindSIPHandlerByDomain(host, SIP_PDU::Method_REGISTER, PSafeReadOnly);
  if (handler == NULL)
    return GetLocalURL(transport, userName);
  return GetLocalURL(handler->GetTransport(), handler->GetTargetAddress().GetUserName());
}


SIPURL SIPEndPoint::GetLocalURL(const OpalTransport &transport, const PString & userName)
{
  PIPSocket::Address ip(PIPSocket::GetDefaultIpAny());
  OpalTransportAddress contactAddress = transport.GetLocalAddress();
  WORD contactPort = GetDefaultSignalPort();
  if (transport.IsOpen())
    transport.GetLocalAddress().GetIpAndPort(ip, contactPort);
  else {
    for (PINDEX i = 0; i < listeners.GetSize(); i++) {
      OpalTransportAddress binding = listeners[i].GetLocalAddress();
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
      contactAddress = OpalTransportAddress(localIP, contactPort, "udp");
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


// End of file ////////////////////////////////////////////////////////////////
