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

#if OPAL_SIP

  #ifdef _MSC_VER
    #pragma message("SIP support enabled")
  #endif

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

SIPEndPoint::SIPEndPoint(OpalManager & mgr,
                         unsigned maxConnectionThreads,
                         unsigned maxHandlerThreads)
  : OpalRTPEndPoint(mgr, "sip", CanTerminateCall|SupportsE164)
  , m_defaultPrackMode(SIPConnection::e_prackSupported)
  , retryTimeoutMin(500)             // 0.5 seconds
  , retryTimeoutMax(0, 4)            // 4 seconds
  , nonInviteTimeout(0, 16)          // 16 seconds
  , pduCleanUpTimeout(0, 5)          // 5 seconds
  , inviteTimeout(0, 32)             // 32 seconds
  , m_progressTimeout(0, 0, 3)       // 3 minutes
  , ackTimeout(0, 32)                // 32 seconds
  , registrarTimeToLive(0, 0, 0, 1)  // 1 hour
  , notifierTimeToLive(0, 0, 0, 1)   // 1 hour
  , natBindingTimeout(0, 0, 1)       // 1 minute
  , m_registeredUserMode(false)
  , m_shuttingDown(false)
  , m_defaultAppearanceCode(-1)
  , m_connectionThreadPool(maxConnectionThreads, "SIPCon Pool")
  , m_handlerThreadPool(maxHandlerThreads, "SIPHandler Pool")

#ifdef _MSC_VER
#pragma warning(disable:4355)
#endif

  , m_highPriorityMonitor(*this, HighPriority)
  , m_lowPriorityMonitor(*this, LowPriority)
  , m_disableTrying(true)

#ifdef _MSC_VER
#pragma warning(default:4355)
#endif
{
  defaultSignalPort = SIPURL::DefaultPort;
  mimeForm = PFalse;
  maxRetries = 10;

  m_allowedEvents += SIPEventPackage(SIPSubscribe::Dialog);
  m_allowedEvents += SIPEventPackage(SIPSubscribe::Conference);

  natBindingTimer.SetNotifier(PCREATE_NOTIFIER(NATBindingRefresh));
  natBindingTimer.RunContinuous(natBindingTimeout);

  natMethod = None;

  // Make sure these have been contructed now to avoid
  // payload type disambiguation problems.
  GetOpalRFC2833();

#if OPAL_T38_CAPABILITY
  GetOpalCiscoNSE();
#endif

#if OPAL_PTLIB_SSL
  manager.AttachEndPoint(this, "sips");
#endif

  PTRACE(4, "SIP\tCreated endpoint.");
}


SIPEndPoint::~SIPEndPoint()
{
}


void SIPEndPoint::ShutDown()
{
  PTRACE(4, "SIP\tShutting down.");
  m_shuttingDown = true;

  // Stop timers before compiler destroys member objects, must wait ...
  natBindingTimer.Stop(true);

  // Clean up the handlers, wait for them to finish before destruction.
  bool shuttingDown = true;
  while (shuttingDown) {
    shuttingDown = false;
    PSafePtr<SIPHandler> handler = activeSIPHandlers.GetFirstHandler();
    while (handler != NULL) {
      if (handler->ShutDown())
        activeSIPHandlers.Remove(handler++);
      else {
        shuttingDown = true;
        ++handler;
      }
    }
    PThread::Sleep(100);
  }

  // Clean up transactions still in progress, waiting for them to terminate.
  PSafePtr<SIPTransactionBase> transaction;
  while ((transaction = m_transactions.GetAt(0, PSafeReference)) != NULL) {
    if (transaction->IsTerminated())
      m_transactions.Remove(transaction);
    else
      PThread::Sleep(100);
  }

  // Now shut down listeners and aggregators
  OpalEndPoint::ShutDown();
}


PString SIPEndPoint::GetDefaultTransport() const 
{
  return "udp$,tcp$"
#if OPAL_PTLIB_SSL
         ",tls$:5061"
#endif
    ; 
}


PStringList SIPEndPoint::GetNetworkURIs(const PString & name) const
{
  PStringList list = OpalRTPEndPoint::GetNetworkURIs(name);

  for (PSafePtr<SIPHandler> handler = activeSIPHandlers.GetFirstHandler(PSafeReadOnly); handler != NULL; ++handler) {
    if (handler->GetMethod() == SIP_PDU::Method_REGISTER && handler->GetAddressOfRecord().GetUserName() == name)
      list += handler->GetAddressOfRecord();
  }

  return list;
}


PBoolean SIPEndPoint::NewIncomingConnection(OpalTransport * transport)
{
  if (transport->IsReliable()) {
    PTRACE(2, "SIP\tListening thread started.");

    do {
      HandlePDU(*transport);
    } while (transport->IsOpen() && !transport->bad() && !transport->eof());

    // Need to make sure all connections are not referencing this transport
    // before we return and it gets deleted.
    for (PSafePtr<OpalConnection> connection(connectionsActive, PSafeReference); connection != NULL; ++connection) {
      PSafePtr<SIPConnection> sipConnection = PSafePtrCast<OpalConnection, SIPConnection>(connection);
      if (&sipConnection->GetTransport() == transport && sipConnection->LockReadWrite()) {
        sipConnection->SetTransport(SIPURL());
        sipConnection->UnlockReadWrite();
      }
    }

    PTRACE(2, "SIP\tListening thread finished.");
  }
  else {
    transport->SetBufferSize(m_maxSizeUDP);
    HandlePDU(*transport); // Always just one PDU
  }

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
  if (m_shuttingDown)
    return;

  if (natMethod != None) {
    PTRACE(5, "SIP\tNAT Binding refresh started.");
    for (PSafePtr<SIPHandler> handler = activeSIPHandlers.GetFirstHandler(PSafeReadOnly); handler != NULL; ++handler) {

      OpalTransport * transport = NULL;
      if (handler->GetState () != SIPHandler::Subscribed ||
           (transport = handler->GetTransport()) == NULL ||
           transport->IsReliable()
#if P_NAT
           || GetManager().GetNatMethod(transport->GetRemoteAddress().GetHostName()) == NULL
#endif
           )
        continue;

      switch (natMethod) {

        case Options:
          {
            SIPOptions::Params params;
            params.m_remoteAddress = transport->GetRemoteAddress().GetHostName();
            SendOPTIONS(params);
          }
          break;

        case EmptyRequest:
          transport->Write("\r\n", 2);
          break;

        default:
          break;
      }
    }

    PTRACE(5, "SIP\tNAT Binding refresh finished.");
  }
}


OpalTransport * SIPEndPoint::CreateTransport(const SIPURL & remoteURL, const PString & localInterface)
{
  OpalTransportAddress remoteAddress = remoteURL.GetHostAddress();
  if (remoteAddress.IsEmpty()) {
    if (GetRegistrationsCount() == 0) {
      PTRACE(1, "SIP\tCannot use tel URI with phone-context or existing registration.");
      return false;
    }
    remoteAddress = SIPURL(GetRegistrations()[0]).GetHostAddress();
  }

  OpalTransportAddress localAddress;
  if (!localInterface.IsEmpty()) {
    if (localInterface != "*") // Nasty kludge to get around infinite recursion in REGISTER
      localAddress = OpalTransportAddress(localInterface, 0, remoteAddress.GetProtoPrefix());
  }
  else {
    PSafePtr<SIPHandler> handler = activeSIPHandlers.FindSIPHandlerByDomain(remoteURL.GetHostName(), SIP_PDU::Method_REGISTER, PSafeReadOnly);
    if (handler != NULL) {
      OpalTransport * transport = handler->GetTransport();
      if (transport != NULL) {
        localAddress = transport->GetInterface();
        PTRACE(4, "SIP\tFound registrar on domain " << remoteURL.GetHostName() << ", using interface " << transport->GetInterface());
      }
    }
  }

  OpalTransport * transport = NULL;

  for (OpalListenerList::iterator listener = listeners.begin(); listener != listeners.end(); ++listener) {
    if ((transport = listener->CreateTransport(localAddress, remoteAddress)) != NULL)
      break;
  }

  if (transport == NULL) {
    // No compatible listeners, can't create a transport to send if we cannot hear the responses!
    PTRACE(2, "SIP\tNo compatible listener to create transport for " << remoteAddress);
    return NULL;
  }

  if (!transport->SetRemoteAddress(remoteAddress)) {
    PTRACE(1, "SIP\tCould not find " << remoteAddress);
    delete transport;
    return NULL;
  }

  PTRACE(4, "SIP\tCreated transport " << *transport);

  transport->SetBufferSize(m_maxSizeUDP);
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
                                            PThread::HighestPriority,
                                            "SIP Transport"));
  return transport;
}


void SIPEndPoint::HandlePDU(OpalTransport & transport)
{
  // create a SIP_PDU structure, then get it to read and process PDU
  SIP_PDU * pdu = new SIP_PDU;

  PTRACE(4, "SIP\tWaiting for PDU on " << transport);
  SIP_PDU::StatusCodes status = pdu->Read(transport);
  if (status == SIP_PDU::Successful_OK) {
    if (OnReceivedPDU(transport, pdu)) 
      return;
  }
  else {
    const SIPMIMEInfo & mime = pdu->GetMIME();
    if (!mime.GetCSeq().IsEmpty() &&
        !mime.GetVia().IsEmpty() &&
        !mime.GetCallID().IsEmpty() &&
        !mime.GetFrom().IsEmpty() &&
        !mime.GetTo().IsEmpty())
      pdu->SendResponse(transport, status, this);
  }

  delete pdu;
}


static PString TranslateENUM(const PString & remoteParty)
{
#if OPAL_PTLIB_DNS
  // if there is no '@', and then attempt to use ENUM
  if (remoteParty.Find('@') == P_MAX_INDEX) {

    // make sure the number has only digits
    PINDEX pos = remoteParty.Find(':');
    PString e164 = pos != P_MAX_INDEX ? remoteParty.Mid(pos+1) : remoteParty;
    if (OpalIsE164(e164)) {
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


PSafePtr<OpalConnection> SIPEndPoint::MakeConnection(OpalCall & call,
                                                const PString & remoteParty,
                                                         void * userData,
                                                   unsigned int options,
                                OpalConnection::StringOptions * stringOptions)
{
  if (listeners.IsEmpty())
    return NULL;

  return AddConnection(CreateConnection(call, SIPURL::GenerateTag(), userData, TranslateENUM(remoteParty), NULL, NULL, options, stringOptions));
}


void SIPEndPoint::OnReleased(OpalConnection & connection)
{
  m_receivedConnectionMutex.Wait();
  m_receivedConnectionTokens.RemoveAt(connection.GetIdentifier());
  m_receivedConnectionMutex.Signal();
  OpalEndPoint::OnReleased(connection);
}


void SIPEndPoint::OnConferenceStatusChanged(OpalEndPoint & endpoint, const PString & uri, OpalConferenceState::ChangeType change)
{
  OpalConferenceStates states;
  if (!endpoint.GetConferenceStates(states, uri) || states.empty()) {
    PTRACE(2, "SIP\tUnexpectedly unable to get conference state for " << uri);
    return;
  }

  const OpalConferenceState & state = states.front();
  PTRACE(4, "SIP\tConference state for " << state.m_internalURI << " has " << change);

  ConferenceMap::iterator it = m_conferenceAOR.find(uri);
  if (it != m_conferenceAOR.end())
    Notify(it->second, SIPEventPackage(SIPSubscribe::Conference), state);

  for (OpalConferenceState::URIs::const_iterator it = state.m_accessURI.begin(); it != state.m_accessURI.begin(); ++it) {
    PTRACE(4, "SIP\tConference access URI: \"" << it->m_uri << '"');

    PURL aor = it->m_uri;
    if (aor.GetScheme().NumCompare("sip") != EqualTo)
      continue;

    switch (change) {
      case OpalConferenceState::Destroyed :
        Unregister(it->m_uri);
        break;

      case OpalConferenceState::Created :
        if (activeSIPHandlers.FindSIPHandlerByDomain(aor.GetHostName(), SIP_PDU::Method_REGISTER, PSafeReference) == NULL) {
          PTRACE(4, "SIP\tConference domain " << aor.GetHostName() << " unregistered, not registering name " << aor.GetUserName());
        }
        else {
          SIPRegister::Params params;
          params.m_addressOfRecord = it->m_uri;
          PString dummy;
          Register(params, dummy);
        }
        break;

      default :
        break;
    }
  }
}


PBoolean SIPEndPoint::GarbageCollection()
{
  PTRACE(6, "SIP\tGarbage collection: transactions=" << m_transactions.GetSize() << ", connections=" << connectionsActive.GetSize());

  PSafePtr<SIPTransactionBase> transaction(m_transactions, PSafeReference);
  while (transaction != NULL) {
    if (transaction->IsTerminated())
      m_transactions.Remove(transaction++);
    else
      ++transaction;
  }

  bool transactionsDone = m_transactions.DeleteObjectsToBeRemoved();


  PSafePtr<SIPHandler> handler = activeSIPHandlers.GetFirstHandler();
  while (handler != NULL) {
    // If unsubscribed then we do the shut down to clean up the handler
    if (handler->GetState() == SIPHandler::Unsubscribed && handler->ShutDown())
      activeSIPHandlers.Remove(handler++);
    else
      ++handler;
  }

  bool handlersDone = activeSIPHandlers.DeleteObjectsToBeRemoved();


  if (!OpalEndPoint::GarbageCollection())
    return false;
  
  if (m_shuttingDown)
    return transactionsDone && handlersDone;

  return true;
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
    options.SetAt(SIP_HEADER_REPLACES, callId);
  options.SetAt(SIP_HEADER_REFERRED_BY, otherConnection->GetRedirectingParty());
  options.SetAt(OPAL_OPT_CALLING_PARTY_URL, otherConnection->GetLocalPartyURL());

  SIPConnection * connection = CreateConnection(call, SIPURL::GenerateTag(), userData, TranslateENUM(remoteParty), NULL, NULL, 0, &options);
  if (!AddConnection(connection))
    return false;

  if (remoteParty.Find(";OPAL-"OPAL_SIP_REFERRED_CONNECTION) == P_MAX_INDEX)
    otherConnection->Release(OpalConnection::EndedByCallForwarded);
  else
    otherConnection->SetPhase(OpalConnection::ForwardingPhase);
  otherConnection->CloseMediaStreams();

  return connection->SetUpConnection();
}


PBoolean SIPEndPoint::ForwardConnection(SIPConnection & connection, const PString & forwardParty)
{
  OpalCall & call = connection.GetCall();
  
  SIPConnection * conn = CreateConnection(call, SIPURL::GenerateTag(), NULL, forwardParty, NULL, NULL);
  if (!AddConnection(conn))
    return PFalse;

  call.OnReleased(connection);
  
  conn->SetUpConnection();
  connection.Release(OpalConnection::EndedByCallForwarded);

  return PTrue;
}


bool SIPEndPoint::ClearDialogContext(const PString & descriptor)
{
  SIPDialogContext context;
  return context.FromString(descriptor) && ClearDialogContext(context);
}


bool SIPEndPoint::ClearDialogContext(SIPDialogContext & context)
{
  if (!context.IsEstablished())
    return false;

  /* This is an extra increment of the sequence number to allow for
     any PDU's in the dialog being sent between the last saved
     context. Highly unlikely this will ever by a million ... */
  context.IncrementCSeq(1000000);

  std::auto_ptr<OpalTransport> transport(CreateTransport(context.GetRemoteURI(), context.GetLocalURI().GetHostName()));
  if (transport.get() == NULL)
    return true; // Can't create transport, so remote host uncontactable, assume dialog cleared.

  PSafePtr<SIPTransaction> byeTransaction = new SIPBye(*this, *transport, context);
  byeTransaction->WaitForCompletion();
  return !byeTransaction->IsFailed();
}


PBoolean SIPEndPoint::OnReceivedPDU(OpalTransport & transport, SIP_PDU * pdu)
{
  if (PAssertNULL(pdu) == NULL)
    return PFalse;

  const SIPMIMEInfo & mime = pdu->GetMIME();

  /* Get tokens to determine the connection to operate on, not as easy as it
     sounds due to allowing for talking to ones self, always thought madness
     generally lies that way ... */

  PString fromToken = mime.GetFieldParameter("from", "tag");
  PString toToken = mime.GetFieldParameter("to", "tag");
  bool hasFromConnection = HasConnection(fromToken);
  bool hasToConnection = HasConnection(toToken);

  PString token;

  switch (pdu->GetMethod()) {
    case SIP_PDU::Method_CANCEL :
      m_receivedConnectionMutex.Wait();
      token = m_receivedConnectionTokens(mime.GetCallID());
      m_receivedConnectionMutex.Signal();
      if (!token.IsEmpty()) {
        m_connectionThreadPool.AddWork(new SIP_Work(*this, pdu, token), token);
        return true;
      }
      break;

    case SIP_PDU::Method_INVITE :
      pdu->AdjustVia(transport);   // // Adjust the Via list
      if (toToken.IsEmpty()) {
        PWaitAndSignal mutex(m_receivedConnectionMutex);

        token = m_receivedConnectionTokens(mime.GetCallID());
        if (!token.IsEmpty()) {
          PSafePtr<SIPConnection> connection = GetSIPConnectionWithLock(token, PSafeReference);
          if (connection != NULL) {
            PTRACE_CONTEXT_ID_PUSH_THREAD(*connection);
            switch (connection->CheckINVITE(*pdu)) {
              case SIPConnection::IsNewINVITE : // Process new INVITE
                break;

              case SIPConnection::IsDuplicateINVITE : // Completely ignore duplicate INVITE
                return true;

              case SIPConnection::IsReINVITE : // Pass on to worker thread if re-INVITE
                m_connectionThreadPool.AddWork(new SIP_Work(*this, pdu, token), token);
                return true;

              case SIPConnection::IsLoopedINVITE : // Send back error if looped INVITE
                SIP_PDU response(*pdu, SIP_PDU::Failure_LoopDetected);
                response.GetMIME().SetProductInfo(GetUserAgent(), connection->GetProductInfo());
                pdu->SendResponse(transport, response);
                return true;
            }
          }
        }

        pdu->SendResponse(transport, SIP_PDU::Information_Trying, this);
        return OnReceivedConnectionlessPDU(transport, pdu);
      }

      if (!hasToConnection) {
        // Has to tag but doesn't correspond to anything, odd.
        pdu->SendResponse(transport, SIP_PDU::Failure_TransactionDoesNotExist);
        return false;
      }
      pdu->SendResponse(transport, SIP_PDU::Information_Trying, this);
      break;

    case SIP_PDU::Method_ACK :
      pdu->AdjustVia(transport);   // // Adjust the Via list
      break;

    case SIP_PDU::NumMethods :  // unknown method
      break;

    default :   // any known method other than INVITE, CANCEL and ACK
      if (!m_disableTrying)
        pdu->SendResponse(transport, SIP_PDU::Information_Trying, this);
      break;
  }

  if (hasToConnection)
    token = toToken;
  else if (hasFromConnection)
    token = fromToken;
  else
    return OnReceivedConnectionlessPDU(transport, pdu);

  m_connectionThreadPool.AddWork(new SIP_Work(*this, pdu, token), token);
  return true;
}


bool SIPEndPoint::OnReceivedConnectionlessPDU(OpalTransport & transport, SIP_PDU * pdu)
{
  if (pdu->GetMethod() == SIP_PDU::NumMethods || pdu->GetMethod() == SIP_PDU::Method_CANCEL) {
    PString id = pdu->GetTransactionID();
    if (GetTransaction(id, PSafeReference) != NULL) {
      m_handlerThreadPool.AddWork(new SIP_Work(*this, pdu, id), id);
      return true;
    }

    PTRACE(2, "SIP\tReceived response for unmatched transaction, id=" << id);
    return false;
  }

  // Prevent any new INVITE/SUBSCRIBE etc etc while we are on the way out.
  if (m_shuttingDown) {
    pdu->SendResponse(transport, SIP_PDU::Failure_ServiceUnavailable);
    return false;
  }

  // Check if we have already received this command (have a transaction)
  {
    PString id = pdu->GetTransactionID();
    PSafePtr<SIPResponse> transaction = PSafePtrCast<SIPTransaction, SIPResponse>(GetTransaction(id, PSafeReadOnly));
    if (transaction != NULL) {
      PTRACE(4, "SIP\tRetransmitting previous response for transaction id=" << id);
      transaction->Send(transport, *pdu);
      return false;
    }
  }

  switch (pdu->GetMethod()) {
    case SIP_PDU::Method_INVITE :
      pdu->AdjustVia(transport);   // // Adjust the Via list
      return OnReceivedINVITE(transport, pdu);

    case SIP_PDU::Method_REGISTER :
      pdu->AdjustVia(transport);   // // Adjust the Via list
      if (OnReceivedREGISTER(transport, *pdu))
        return false;
      break;

    case SIP_PDU::Method_SUBSCRIBE :
      pdu->AdjustVia(transport);   // // Adjust the Via list
      if (OnReceivedSUBSCRIBE(transport, *pdu, NULL))
        return false;
      break;

    case SIP_PDU::Method_NOTIFY :
      pdu->AdjustVia(transport);   // // Adjust the Via list
       if (OnReceivedNOTIFY(transport, *pdu))
         return false;
       break;

    case SIP_PDU::Method_MESSAGE :
      pdu->AdjustVia(transport);   // // Adjust the Via list
      if (OnReceivedMESSAGE(transport, *pdu))
        return false;
      break;
   
    case SIP_PDU::Method_OPTIONS :
      pdu->AdjustVia(transport);   // // Adjust the Via list
      if (OnReceivedOPTIONS(transport, *pdu))
        return false;
      break;

    case SIP_PDU::Method_BYE :
      // If we receive a BYE outside of the context of a connection, tell them.
      pdu->SendResponse(transport, SIP_PDU::Failure_TransactionDoesNotExist, this);
      return false;

      // If we receive an ACK outside of the context of a connection, ignore it.
    case SIP_PDU::Method_ACK :
      return false;

    default :
      break;
  }

  SIP_PDU response(*pdu, SIP_PDU::Failure_MethodNotAllowed);
  response.SetAllow(GetAllowedMethods()); // Required by spec
  pdu->SendResponse(transport, response, this);
  return false;
}

PBoolean SIPEndPoint::OnReceivedREGISTER(OpalTransport & /*transport*/, SIP_PDU & /*pdu*/)
{
  return false;
}


PBoolean SIPEndPoint::OnReceivedSUBSCRIBE(OpalTransport & transport, SIP_PDU & pdu, SIPDialogContext * dialog)
{
  SIPMIMEInfo & mime = pdu.GetMIME();

  SIPSubscribe::EventPackage eventPackage(mime.GetEvent());

  CanNotifyResult canNotify = CanNotifyImmediate;

  // See if already subscribed. Now this is not perfect as we only check the call-id and strictly
  // speaking we should check the from-tag and to-tags as well due to it being a dialog.
  PSafePtr<SIPHandler> handler = FindHandlerByPDU(pdu, PSafeReadWrite);
  if (handler == NULL) {
    SIPDialogContext newDialog(mime);
    if (dialog == NULL)
      dialog = &newDialog;

    if ((canNotify = CanNotify(eventPackage, dialog->GetLocalURI())) == CannotNotify) {
      SIP_PDU response(pdu, SIP_PDU::Failure_BadEvent);
      response.GetMIME().SetAllowEvents(m_allowedEvents); // Required by spec
      pdu.SendResponse(transport, response, this);
      return true;
    }

    handler = new SIPNotifyHandler(*this, eventPackage, *dialog);
    handler.SetSafetyMode(PSafeReadWrite);
    activeSIPHandlers.Append(handler);

    handler->GetTransport()->SetInterface(transport.GetInterface());

    mime.SetTo(dialog->GetLocalURI().AsQuotedString());
  }

  // Update expiry time
  unsigned expires = mime.GetExpires();

  SIP_PDU response(pdu, canNotify == CanNotifyImmediate ? SIP_PDU::Successful_OK : SIP_PDU::Successful_Accepted);
  response.GetMIME().SetEvent(eventPackage); // Required by spec
  response.GetMIME().SetExpires(expires);    // Required by spec
  pdu.SendResponse(transport, response, this);

  if (handler->IsDuplicateCSeq(mime.GetCSeqIndex()))
    return true;

  if (expires == 0) {
    handler->ActivateState(SIPHandler::Unsubscribing);
    return true;
  }

  handler->SetExpire(expires);

  if (canNotify == CanNotifyImmediate)
    handler->SendNotify(NULL); // Send initial NOTIFY as per spec 3.1.6.2/RFC3265

  return true;
}


void SIPEndPoint::OnReceivedResponse(SIPTransaction & transaction, SIP_PDU & response)
{
  PSafePtr<SIPHandler> handler = FindHandlerByPDU(response, PSafeReadWrite);
  if (handler != NULL)
    handler->OnReceivedResponse(transaction, response);
  else {
    PTRACE(2, "SIP\tResponse for " << transaction << " received,"
              " but unknown handler, ID: " << transaction.GetMIME().GetCallID());
  }
}


PSafePtr<SIPConnection> SIPEndPoint::GetSIPConnectionWithLock(const PString & token,
                                                              PSafetyMode mode,
                                                              SIP_PDU::StatusCodes * errorCode)
{
  PSafePtr<SIPConnection> connection = PSafePtrCast<OpalConnection, SIPConnection>(GetConnectionWithLock(token, mode));
  if (connection != NULL)
    return connection;

  PString to;
  static const char toTag[] = ";to-tag=";
  PINDEX pos = token.Find(toTag);
  if (pos != P_MAX_INDEX) {
    pos += sizeof(toTag)-1;
    to = token(pos, token.Find(';', pos)-1).Trim();
  }

  PString from;
  static const char fromTag[] = ";from-tag=";
  pos = token.Find(fromTag);
  if (pos != P_MAX_INDEX) {
    pos += sizeof(fromTag)-1;
    from = token(pos, token.Find(';', pos)-1).Trim();
  }

  PString callid = token.Left(token.Find(';')).Trim();
  if (callid.IsEmpty() || to.IsEmpty() || from.IsEmpty()) {
    if (errorCode != NULL)
      *errorCode = SIP_PDU::Failure_BadRequest;
    return NULL;
  }

  connection = PSafePtrCast<OpalConnection, SIPConnection>(connectionsActive.GetAt(0, PSafeReference));
  while (connection != NULL) {
    const SIPDialogContext & context = connection->GetDialog();
    if (context.GetCallID() == callid) {
      if (context.GetLocalTag() == to && context.GetRemoteTag() == from) {
        if (connection.SetSafetyMode(mode))
          return connection;
        break;
      }

      PTRACE(4, "SIP\tReplaces header matches callid, but not to/from tags: "
                "to=" << context.GetLocalTag() << ", from=" << context.GetRemoteTag());
    }

    ++connection;
  }

  if (errorCode != NULL)
    *errorCode = SIP_PDU::Failure_TransactionDoesNotExist;
  return NULL;
}


PBoolean SIPEndPoint::OnReceivedINVITE(OpalTransport & transport, SIP_PDU * request)
{
  SIPMIMEInfo & mime = request->GetMIME();

  // parse the incoming To field, and check if we accept incoming calls for this address
  SIPURL toAddr(mime.GetTo());
  if (!IsAcceptedAddress(toAddr)) {
    PTRACE(2, "SIP\tIncoming INVITE for " << request->GetURI() << " for unacceptable address " << toAddr);
    request->SendResponse(transport, SIP_PDU::Failure_NotFound, this);
    return false;
  }

  if (!request->GetEntityBody().IsEmpty() &&
         (!mime.GetContentEncoding().IsEmpty() ||
           mime.GetContentType() != "application/sdp")) {
    // Do not currently support anything other than SDP, in particular multipart stuff.
    PTRACE(2, "SIP\tIncoming INVITE for " << request->GetURI() << " does not contain SDP");
    SIP_PDU response(*request, SIP_PDU::Failure_UnsupportedMediaType);
    response.GetMIME().SetAccept("application/sdp");
    response.GetMIME().SetAcceptEncoding("identity");
    response.SetAllow(GetAllowedMethods());
    request->SendResponse(transport, response, this);
    return false;
  }

  // See if we are replacing an existing call.
  OpalCall * call = NULL;
  if (mime.Contains("Replaces")) {
    SIP_PDU::StatusCodes errorCode;
    PSafePtr<SIPConnection> replacedConnection = GetSIPConnectionWithLock(mime("Replaces"), PSafeReference, &errorCode);
    if (replacedConnection == NULL) {
      PTRACE_IF(2, errorCode==SIP_PDU::Failure_BadRequest,
                "SIP\tBad Replaces header in INVITE for " << request->GetURI());
      PTRACE_IF(2, errorCode==SIP_PDU::Failure_TransactionDoesNotExist,
                "SIP\tNo connection matching dialog info in Replaces header of INVITE from " << request->GetURI());
      request->SendResponse(transport, errorCode, this);
      return false;
    }

    // Use the existing call instance when replacing the SIP side of it.
    call = &replacedConnection->GetCall();
    PTRACE(3, "SIP\tIncoming INVITE replaces connection " << *replacedConnection);
  }

  // create and check transport
  OpalTransport * newTransport;
  if (transport.IsReliable())
    newTransport = &transport;
  else {
    newTransport = CreateTransport(SIPURL(PString::Empty(), transport.GetRemoteAddress(), 0), transport.GetInterface());
    if (newTransport == NULL) {
      PTRACE(1, "SIP\tFailed to create transport for SIPConnection for INVITE for " << request->GetURI() << " to " << toAddr);
      request->SendResponse(transport, SIP_PDU::Failure_NotFound, this);
      return PFalse;
    }
  }

  if (call == NULL) {
    // Get new instance of a call, abort if none created
    call = manager.InternalCreateCall();
    if (call == NULL) {
      request->SendResponse(transport, SIP_PDU::Failure_TemporarilyUnavailable, this);
      return false;
    }
  }

  PTRACE_CONTEXT_ID_PUSH_THREAD(call);

  // ask the endpoint for a connection
  SIPConnection *connection = CreateConnection(*call,
                                               SIPURL::GenerateTag(),
                                               NULL,
                                               SIPURL(),
                                               newTransport,
                                               request);
  if (!AddConnection(connection)) {
    PTRACE(1, "SIP\tFailed to create SIPConnection for INVITE for " << request->GetURI() << " to " << toAddr);
    request->SendResponse(transport, SIP_PDU::Failure_NotFound, this);
    return PFalse;
  }

  // m_receivedConnectionMutex already set
  PString token = connection->GetToken();
  m_receivedConnectionTokens.SetAt(mime.GetCallID(), token);

  // Get the connection to handle the rest of the INVITE in the thread pool
  m_connectionThreadPool.AddWork(new SIP_Work(*this, request, token), token);

  return PTrue;
}


void SIPEndPoint::OnTransactionFailed(SIPTransaction & transaction)
{
  PSafePtr<SIPHandler> handler = FindHandlerByPDU(transaction, PSafeReadWrite);
  if (handler != NULL) 
    handler->OnTransactionFailed(transaction);
  else {
    PTRACE(2, "SIP\tTransaction " << transaction << " failed,"
              " unknown handler, ID: " << transaction.GetMIME().GetCallID());
  }
}


PBoolean SIPEndPoint::OnReceivedNOTIFY(OpalTransport & transport, SIP_PDU & pdu)
{
  SIPEventPackage eventPackage(pdu.GetMIME().GetEvent());

  PTRACE(3, "SIP\tReceived NOTIFY " << eventPackage);
  
  // A NOTIFY will have the same CallID than the SUBSCRIBE request it corresponds to
  // Technically should check for whole dialog, but call-id will do.
  PSafePtr<SIPHandler> handler = FindHandlerByPDU(pdu, PSafeReadWrite);

  if (handler == NULL && eventPackage == SIPSubscribe::MessageSummary) {
    PTRACE(4, "SIP\tWork around Asterisk bug in message-summary event package.");
    SIPURL url_from (pdu.GetMIME().GetFrom());
    SIPURL url_to (pdu.GetMIME().GetTo());
    PString to = url_to.GetUserName() + "@" + url_from.GetHostName();
    handler = activeSIPHandlers.FindSIPHandlerByUrl(to, SIP_PDU::Method_SUBSCRIBE, eventPackage, PSafeReadWrite);
  }

  if (handler == NULL) {
    PTRACE(3, "SIP\tCould not find a SUBSCRIBE corresponding to the NOTIFY " << eventPackage);
    pdu.SendResponse(transport, SIP_PDU::Failure_TransactionDoesNotExist, this);
    return true;
  }

  PTRACE(3, "SIP\tFound a SUBSCRIBE corresponding to the NOTIFY " << eventPackage);
  return handler->OnReceivedNOTIFY(pdu);
}


bool SIPEndPoint::OnReceivedMESSAGE(OpalTransport & transport, SIP_PDU & pdu)
{
  // handle a MESSAGE received outside the context of a call
  PTRACE(3, "SIP\tReceived MESSAGE outside the context of a call");

  // if there is a callback, assume that the application knows what it is doing
  if (!m_onConnectionlessMessage.IsNULL()) {
    ConnectionlessMessageInfo info(transport, pdu);
    m_onConnectionlessMessage(*this, info);
    switch (info.m_status) {
      case ConnectionlessMessageInfo::MethodNotAllowed :
        return false;

      case ConnectionlessMessageInfo::SendOK :
        pdu.SendResponse(transport, SIP_PDU::Successful_OK, this);
        // Do next case

      case ConnectionlessMessageInfo::ResponseSent :
        return true;

      default :
        break;
    }
  }

#if OPAL_HAS_IM
  OpalSIPIMContext::OnReceivedMESSAGE(*this, NULL, transport, pdu);
#else
  pdu.SendResponse(transport, SIP_PDU::Failure_BadRequest, this);
#endif
  return true;
}


bool SIPEndPoint::OnReceivedOPTIONS(OpalTransport & transport, SIP_PDU & pdu)
{
  SIPResponse * response = new SIPResponse(*this, SIP_PDU::Successful_OK);
  response->Send(transport, pdu);
  return true;
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
  params.m_addressOfRecord = user;
  params.m_registrarAddress = host;
  params.m_authID = authName;
  params.m_password = password;
  params.m_realm = realm;
  params.m_expire = expire;
  params.m_minRetryTime = minRetryTime;
  params.m_maxRetryTime = maxRetryTime;

  PString dummy;
  return Register(params, dummy);
}


bool SIPEndPoint::Register(const SIPRegister::Params & newParams, PString & aor, bool asynchronous)
{
  SIP_PDU::StatusCodes reason;
  return Register(newParams, aor, asynchronous ? NULL : &reason);
}


bool SIPEndPoint::Register(const SIPRegister::Params & newParams, PString & aor, SIP_PDU::StatusCodes * reason)
{
  PTRACE(4, "SIP\tStart REGISTER\n" << newParams);

  SIPRegister::Params params(newParams);
  params.Normalise(GetDefaultLocalPartyName(), GetRegistrarTimeToLive());
  PSafePtr<SIPHandler> handler = activeSIPHandlers.FindSIPHandlerByUrl(params.m_addressOfRecord, SIP_PDU::Method_REGISTER, PSafeReadWrite);

  // If there is already a request with this URL and method, 
  // then update it with the new information
  if (handler != NULL) {
    PSafePtrCast<SIPHandler, SIPRegisterHandler>(handler)->UpdateParameters(params);
  }
  else {
    // Otherwise create a new request with this method type
    handler = CreateRegisterHandler(params);
    activeSIPHandlers.Append(handler);
  }

  aor = handler->GetAddressOfRecord().AsString();

  if (!handler->ActivateState(SIPHandler::Subscribing))
    return false;

  if (reason == NULL)
    return true;

  m_registrationComplete[aor].m_sync.Wait();
  *reason = m_registrationComplete[aor].m_reason;
  m_registrationComplete.erase(aor);
  return handler->GetState() == SIPHandler::Subscribed;
}


SIPRegisterHandler * SIPEndPoint::CreateRegisterHandler(const SIPRegister::Params & params)
{
  return new SIPRegisterHandler(*this, params);
}


PBoolean SIPEndPoint::IsRegistered(const PString & token, bool includeOffline) 
{
  PSafePtr<SIPHandler> handler = activeSIPHandlers.FindSIPHandlerByCallID(token, PSafeReference);
  if (handler == NULL)
    handler = activeSIPHandlers.FindSIPHandlerByUrl(token, SIP_PDU::Method_REGISTER, PSafeReference);

  if (handler != NULL)
    return includeOffline ? (handler->GetState() != SIPHandler::Unsubscribed)
                          : (handler->GetState() == SIPHandler::Subscribed);

  PTRACE(1, "SIP\tCould not find active REGISTER for " << token);
  return false;
}


PBoolean SIPEndPoint::Unregister(const PString & token)
{
  PSafePtr<SIPHandler> handler = activeSIPHandlers.FindSIPHandlerByCallID(token, PSafeReference);
  if (handler == NULL)
    handler = activeSIPHandlers.FindSIPHandlerByUrl(token, SIP_PDU::Method_REGISTER, PSafeReference);

  if (handler != NULL)
    return handler->ActivateState(SIPHandler::Unsubscribing);

  PTRACE(1, "SIP\tCould not find active REGISTER for " << token);
  return false;
}


bool SIPEndPoint::UnregisterAll()
{
  bool atLeastOne = false;

  for (PSafePtr<SIPHandler> handler = activeSIPHandlers.GetFirstHandler(); handler != NULL; ++handler) {
    if (handler->GetMethod() == SIP_PDU::Method_REGISTER &&
        handler->ActivateState(SIPHandler::Unsubscribing))
      atLeastOne = true;
  }

  return atLeastOne;
}


void SIPEndPoint::OnRegistrationStatus(const RegistrationStatus & status)
{
  OnRegistrationStatus(status.m_addressofRecord, status.m_wasRegistering, status.m_reRegistering, status.m_reason);

  if (!status.m_wasRegistering ||
       status.m_reRegistering ||
       status.m_reason == SIP_PDU::Information_Trying)
    return;

  std::map<PString, RegistrationCompletion>::iterator it = m_registrationComplete.find(status.m_addressofRecord);
  if (it != m_registrationComplete.end()) {
    it->second.m_reason = status.m_reason;
    it->second.m_sync.Signal();
  }
}


void SIPEndPoint::OnRegistrationStatus(const PString & aor,
                                       PBoolean wasRegistering,
                                       PBoolean /*reRegistering*/,
                                       SIP_PDU::StatusCodes reason)
{
  if (reason == SIP_PDU::Information_Trying)
    return;

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


bool SIPEndPoint::Subscribe(SIPSubscribe::PredefinedPackages eventPackage, unsigned expire, const PString & to)
{
  SIPSubscribe::Params params(eventPackage);
  params.m_addressOfRecord = to;
  params.m_expire = expire;

  PString dummy;
  return Subscribe(params, dummy);
}


bool SIPEndPoint::Subscribe(const SIPSubscribe::Params & newParams, PString & token, bool tokenIsAOR)
{
  PTRACE(4, "SIP\tStart SUBSCRIBE\n" << newParams);

  SIPSubscribe::Params params(newParams);
  params.Normalise(GetDefaultLocalPartyName(), GetNotifierTimeToLive());
  PSafePtr<SIPHandler> handler = activeSIPHandlers.FindSIPHandlerByUrl(params.m_addressOfRecord, SIP_PDU::Method_SUBSCRIBE, params.m_eventPackage, PSafeReadWrite);

  // If there is already a request with this URL and method, 
  // then update it with the new information
  if (handler != NULL)
    PSafePtrCast<SIPHandler, SIPSubscribeHandler>(handler)->UpdateParameters(params);
  else {
    // Otherwise create a new request with this method type
    handler = new SIPSubscribeHandler(*this, params);
    activeSIPHandlers.Append(handler);
  }

  token = tokenIsAOR ? handler->GetAddressOfRecord().AsString() : handler->GetCallID();

  return handler->ActivateState(SIPHandler::Subscribing);
}


bool SIPEndPoint::IsSubscribed(const PString & token, bool includeOffline) 
{
  PSafePtr<SIPHandler> handler = activeSIPHandlers.FindSIPHandlerByCallID(token, PSafeReadOnly);
  if (handler == NULL)
    return false;

  return includeOffline ? (handler->GetState() != SIPHandler::Unsubscribed)
                        : (handler->GetState() == SIPHandler::Subscribed);
}


bool SIPEndPoint::IsSubscribed(const PString & eventPackage, const PString & token, bool includeOffline) 
{
  PSafePtr<SIPHandler> handler = activeSIPHandlers.FindSIPHandlerByCallID(token, PSafeReference);
  if (handler == NULL)
    handler = activeSIPHandlers.FindSIPHandlerByUrl(token, SIP_PDU::Method_SUBSCRIBE, eventPackage, PSafeReference);
  else {
    if (handler->GetEventPackage() != eventPackage)
      handler.SetNULL();
  }

  return handler != NULL &&
         (includeOffline ? (handler->GetState() != SIPHandler::Unsubscribed)
                         : (handler->GetState() == SIPHandler::Subscribed));
}


bool SIPEndPoint::Unsubscribe(const PString & token, bool invalidateNotifiers)
{
  return Unsubscribe(SIPEventPackage(), token, invalidateNotifiers);
}


bool SIPEndPoint::Unsubscribe(SIPSubscribe::PredefinedPackages eventPackage,
                              const PString & token,
                              bool invalidateNotifiers)
{
  return Unsubscribe(SIPEventPackage(eventPackage), token, invalidateNotifiers);
}


bool SIPEndPoint::Unsubscribe(const PString & eventPackage, const PString & token, bool invalidateNotifiers)
{
  PSafePtr<SIPHandler> handler = activeSIPHandlers.FindSIPHandlerByCallID(token, PSafeReference);
  if (handler == NULL)
    handler = activeSIPHandlers.FindSIPHandlerByUrl(token, SIP_PDU::Method_SUBSCRIBE, eventPackage, PSafeReference);
  else {
    if (!eventPackage.IsEmpty() && handler->GetEventPackage() != eventPackage)
      handler.SetNULL();
  }

  if (handler == NULL) {
    PTRACE(1, "SIP\tCould not find active SUBSCRIBE of " << eventPackage << " package to " << token);
    return false;
  }

  if (SIPEventPackage(SIPSubscribe::Conference) == eventPackage) {
    ConferenceMap::iterator it = m_conferenceAOR.begin();
    while (it != m_conferenceAOR.end()) {
      if (it->second != handler->GetAddressOfRecord())
        ++it;
      else
        m_conferenceAOR.erase(it++);
    }
  }

  if (invalidateNotifiers) {
    PSafePtr<SIPSubscribeHandler> subscription = PSafePtrCast<SIPHandler, SIPSubscribeHandler>(handler);
    if (subscription != NULL) {
      SIPSubscribe::Params params(subscription->GetParams());
      params.m_onNotify = NULL;
      params.m_onSubcribeStatus = NULL;
      subscription->UpdateParameters(params);
    }
  }

  return handler->ActivateState(SIPHandler::Unsubscribing);
}


bool SIPEndPoint::UnsubcribeAll(SIPSubscribe::PredefinedPackages eventPackage)
{
  return UnsubcribeAll(SIPEventPackage(eventPackage));
}


bool SIPEndPoint::UnsubcribeAll(const PString & eventPackage)
{
  bool atLeastOne = false;

  for (PSafePtr<SIPHandler> handler = activeSIPHandlers.GetFirstHandler(); handler != NULL; ++handler) {
    if (handler->GetMethod() == SIP_PDU::Method_SUBSCRIBE &&
        handler->GetEventPackage() == eventPackage &&
        handler->ActivateState(SIPHandler::Unsubscribing))
      atLeastOne = true;
  }

  return atLeastOne;
}


void SIPEndPoint::OnSubscriptionStatus(const SubscriptionStatus & status)
{
  // backwards compatiblity
  OnSubscriptionStatus(*status.m_handler,
                        status.m_addressofRecord,
                        status.m_wasSubscribing,
                        status.m_reSubscribing,
                        status.m_reason);
}


void SIPEndPoint::OnSubscriptionStatus(const PString & /*eventPackage*/,
                                       const SIPURL & /*aor*/,
                                       bool /*wasSubscribing*/,
                                       bool /*reSubscribing*/,
                                       SIP_PDU::StatusCodes /*reason*/)
{
}


void SIPEndPoint::OnSubscriptionStatus(SIPSubscribeHandler & handler,
                                       const SIPURL & aor,
                                       bool wasSubscribing,
                                       bool reSubscribing,
                                       SIP_PDU::StatusCodes reason)
{
  // backwards compatiblity
  OnSubscriptionStatus(handler.GetParams().m_eventPackage, 
                       aor, 
                       wasSubscribing, 
                       reSubscribing, 
                       reason);
}


bool SIPEndPoint::CanNotify(const PString & eventPackage)
{
  if (m_allowedEvents.Contains(eventPackage))
    return true;

  PTRACE(3, "SIP\tCannot notify event \"" << eventPackage << "\" not one of "
         << setfill(',') << m_allowedEvents << setfill(' '));
  return false;
}


SIPEndPoint::CanNotifyResult SIPEndPoint::CanNotify(const PString & eventPackage, const SIPURL & aor)
{
  if (SIPEventPackage(SIPSubscribe::Conference) != eventPackage)
    return CanNotify(eventPackage) ? CanNotifyImmediate : CannotNotify;

  OpalConferenceStates states;
  if (!manager.GetConferenceStates(states, aor.GetUserName()) || states.empty()) {
    PTRACE(3, "SIP\tCannot notify event \"" << eventPackage << "\" event, no conferences enabled.");
    return CannotNotify;
  }

  PString uri = states.front().m_internalURI;
  ConferenceMap::iterator it = m_conferenceAOR.find(uri);
  while (it != m_conferenceAOR.end() && it->first == uri) {
    if (it->second == aor)
      return CanNotifyImmediate;
  }

  m_conferenceAOR.insert(ConferenceMap::value_type(uri, aor));
  return CanNotifyImmediate;
}


bool SIPEndPoint::Notify(const SIPURL & aor, const PString & eventPackage, const PObject & body)
{
  bool atLeastOne = false;

  for (PSafePtr<SIPHandler> handler = activeSIPHandlers.GetFirstHandler(); handler != NULL; ++handler) {
    if (handler->GetMethod() == SIP_PDU::Method_NOTIFY &&
        handler->GetAddressOfRecord() == aor &&
        handler->GetEventPackage() == eventPackage &&
        handler->SendNotify(&body))
      atLeastOne = true;
  }

  return atLeastOne;
}


bool SIPEndPoint::SendMESSAGE(SIPMessage::Params & params)
{
  if (params.m_remoteAddress.IsEmpty()) {
    PTRACE(2, "SIP\tCannot send MESSAGE to no-one.");
    return false;
  }

  // don't send empty MESSAGE because some clients barf (cough...X-Lite...cough)
  if (params.m_body.IsEmpty()) {
    PTRACE(2, "SIP\tCannot send empty MESSAGE.");
    return false;
  }

  /* if conversation ID has been set, assume the handler with the matching
     call ID is what was used last time. If no conversation ID has been set,
     see if the destination AOR exists and use that handler (and it's
     call ID). Else create a new conversation. */
  PSafePtr<SIPHandler> handler;
  if (params.m_id.IsEmpty())
    handler = activeSIPHandlers.FindSIPHandlerByUrl(params.m_remoteAddress, SIP_PDU::Method_MESSAGE, PSafeReference);
  else
    handler = activeSIPHandlers.FindSIPHandlerByCallID(params.m_id, PSafeReference);

  // create or update the handler if required
  if (handler == NULL) {
    handler = new SIPMessageHandler(*this, params);
    activeSIPHandlers.Append(handler);
  }
  else
    PSafePtrCast<SIPHandler, SIPMessageHandler>(handler)->UpdateParameters(params);

  params.m_id = handler->GetCallID();

  return handler->ActivateState(SIPHandler::Subscribing);
}


#if OPAL_HAS_IM
void SIPEndPoint::OnMESSAGECompleted(const SIPMessage::Params & params, SIP_PDU::StatusCodes reason)
{
    OpalSIPIMContext::OnMESSAGECompleted(*this, params, reason);
}
#else
void SIPEndPoint::OnMESSAGECompleted(const SIPMessage::Params &, SIP_PDU::StatusCodes)
{
}
#endif


bool SIPEndPoint::SendOPTIONS(const SIPOptions::Params & newParams)
{
  SIPOptions::Params params(newParams);
  params.Normalise(GetDefaultLocalPartyName(), GetNotifierTimeToLive());

  PSafePtr<SIPHandler> handler = new SIPOptionsHandler(*this, params);
  activeSIPHandlers.Append(handler);
  return handler->ActivateState(SIPHandler::Subscribing);
}


void SIPEndPoint::OnOptionsCompleted(const SIPOptions::Params & PTRACE_PARAM(params),
                                                const SIP_PDU & PTRACE_PARAM(response))
{
  PTRACE(3, "SIP\tCompleted OPTIONS command to " << params.m_remoteAddress << ", status=" << response.GetStatusCode());
}


PBoolean SIPEndPoint::Ping(const PURL & to)
{
  PSafePtr<SIPHandler> handler = activeSIPHandlers.FindSIPHandlerByUrl(to, SIP_PDU::Method_PING, PSafeReference);
  if (handler == NULL) {
    handler = new SIPPingHandler(*this, to);
    activeSIPHandlers.Append(handler);
  }

  return handler->ActivateState(SIPHandler::Subscribing);
}


bool SIPEndPoint::Publish(const SIPSubscribe::Params & newParams, const PString & body, PString & aor)
{
  PTRACE(4, "SIP\tStart PUBLISH\n" << newParams);

  SIPSubscribe::Params params(newParams);
  params.Normalise(GetDefaultLocalPartyName(), GetNotifierTimeToLive());
  PSafePtr<SIPHandler> handler = activeSIPHandlers.FindSIPHandlerByUrl(params.m_addressOfRecord, SIP_PDU::Method_PUBLISH, params.m_eventPackage, PSafeReadWrite);
  if (handler != NULL)
    handler->SetBody(body);
  else {
    handler = new SIPPublishHandler(*this, params, body);
    activeSIPHandlers.Append(handler);
  }

  aor = handler->GetAddressOfRecord().AsString();

  return handler->ActivateState(params.m_expire != 0 ? SIPHandler::Subscribing : SIPHandler::Unsubscribing);
}


bool SIPEndPoint::Publish(const PString & to, const PString & body, unsigned expire)
{
  SIPSubscribe::Params params(SIPSubscribe::Presence);
  params.m_addressOfRecord = to;
  params.m_expire = expire;

  PString aor;
  return Publish(params, body, aor);
}


bool SIPEndPoint::PublishPresence(const SIPPresenceInfo & info, unsigned expire)
{
  SIPSubscribe::Params params(SIPSubscribe::Presence);
  params.m_addressOfRecord = info.m_contact.IsEmpty() ? info.m_entity.AsString() : info.m_contact;
  params.m_expire          = expire;
  params.m_agentAddress    = info.m_presenceAgent;
  params.m_contentType     = "application/pidf+xml";

  PString aor;
  return Publish(params, expire == 0 ? PString::Empty() : info.AsXML(), aor);
}


void SIPEndPoint::OnPresenceInfoReceived(const SIPPresenceInfo & info)
{
  PTRACE(4, "SIP\tReceived presence for entity '" << info.m_entity << "' using old API");

  // For backward compatibility
  switch (info.m_state) {
    case OpalPresenceInfo::Available :
      OnPresenceInfoReceived(info.m_entity, "open", info.m_note);
      break;
    case OpalPresenceInfo::NoPresence :
      OnPresenceInfoReceived(info.m_entity, "closed", info.m_note);
      break;
    default :
      OnPresenceInfoReceived(info.m_entity, PString::Empty(), info.m_note);
  }
}


void SIPEndPoint::OnPresenceInfoReceived(const PString & /*entity*/,
                                         const PString & /*basic*/,
                                         const PString & /*note*/)
{
}


void SIPEndPoint::OnDialogInfoReceived(const SIPDialogNotification & PTRACE_PARAM(info))
{
  PTRACE(3, "SIP\tReceived dialog info for \"" << info.m_entity << "\" id=\"" << info.m_callId << '"');
}


void SIPEndPoint::SendNotifyDialogInfo(const SIPDialogNotification & info)
{
  Notify(info.m_entity, SIPEventPackage(SIPSubscribe::Dialog), info);
}


void SIPEndPoint::OnRegInfoReceived(const SIPRegNotification & PTRACE_PARAM(info))
{
  PTRACE(3, "SIP\tReceived registration info for \"" << info.m_aor << "\" state=" << info.GetStateName());
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
  PTRACE_IF(3, !proxy.IsEmpty(), "SIP\tOutbound proxy for endpoint set to " << proxy);
}


PString SIPEndPoint::GetUserAgent() const 
{
  return userAgentString;
}

void SIPEndPoint::OnStartTransaction(SIPConnection & /*conn*/, SIPTransaction & /*transaction*/)
{
}

unsigned SIPEndPoint::GetAllowedMethods() const
{
  return (1<<SIP_PDU::Method_INVITE   )|
         (1<<SIP_PDU::Method_ACK      )|
         (1<<SIP_PDU::Method_CANCEL   )|
         (1<<SIP_PDU::Method_BYE      )|
         (1<<SIP_PDU::Method_OPTIONS  )|
         (1<<SIP_PDU::Method_NOTIFY   )|
         (1<<SIP_PDU::Method_REFER    )|
         (1<<SIP_PDU::Method_MESSAGE  )|
         (1<<SIP_PDU::Method_INFO     )|
         (1<<SIP_PDU::Method_PING     )|
         (1<<SIP_PDU::Method_PRACK    )|
         (1<<SIP_PDU::Method_SUBSCRIBE);
}


bool SIPEndPoint::GetAuthentication(const PString & realm, PString & user, PString & password) 
{
  PSafePtr<SIPHandler> handler = activeSIPHandlers.FindSIPHandlerByAuthRealm(realm, user, PSafeReadOnly);
  if (handler == NULL) {
    if (m_registeredUserMode)
      return false;

    handler = activeSIPHandlers.FindSIPHandlerByAuthRealm(realm, PSafeReadOnly);
    if (handler == NULL)
      return false;
  }

  if (handler->GetPassword().IsEmpty())
    return false;

  // really just after password, but username MAY change too.
  user     = handler->GetUsername();
  password = handler->GetPassword();

  return true;
}


SIPURL SIPEndPoint::GetRegisteredProxy(const SIPURL & url)
{
  PSafePtr<SIPHandler> handler;

  if (url.GetScheme() != "tel") {
    // Look up by the full URL first in case of multiple registrations to the same domain.
    if ((handler = activeSIPHandlers.FindSIPHandlerByUrl(url.AsString(), SIP_PDU::Method_REGISTER, PSafeReadOnly)) == NULL) {
      if (m_registeredUserMode)
        return SIPURL();

      // Precise AOR not found, locate the name used for the domain.
      if ((handler = activeSIPHandlers.FindSIPHandlerByDomain(url.GetHostName(), SIP_PDU::Method_REGISTER, PSafeReadOnly)) == NULL)
        return SIPURL();
    }

    return handler->GetProxy();
  }

  // Can't do tel URI if in this mode
  if (m_registeredUserMode)
    return SIPURL();

  PString domain = url.GetHostName();
  if (domain.IsEmpty() || OpalIsE164(domain)) {
    // No context, just get first registration
    handler = activeSIPHandlers.GetFirstHandler();
    while (handler != NULL && handler->GetMethod() != SIP_PDU::Method_REGISTER)
      ++handler;
  }
  else
    handler = activeSIPHandlers.FindSIPHandlerByDomain(domain, SIP_PDU::Method_REGISTER, PSafeReadOnly);

  if (handler == NULL)
    return SIPURL();

  SIPURL proxy = handler->GetProxy();
  if (proxy.IsEmpty())
    return handler->GetAddressOfRecord();
  return proxy;
}


SIPURL SIPEndPoint::GetRegisteredPartyName(const SIPURL & url, const OpalTransport & transport)
{
  // Look up by the full URL first in case of multiple registrations to the same domain.
  PSafePtr<SIPHandler> handler = activeSIPHandlers.FindSIPHandlerByUrl(url, SIP_PDU::Method_REGISTER, PSafeReadOnly);
  if (handler == NULL) {
    if (m_registeredUserMode)
      return GetDefaultRegisteredPartyName(transport);

    // Precise AOR not found, locate the name used for the domain.
    handler = activeSIPHandlers.FindSIPHandlerByDomain(url.GetHostName(), SIP_PDU::Method_REGISTER, PSafeReadOnly);
    if (handler == NULL) 
      return GetDefaultRegisteredPartyName(transport);
  }

  PTRACE(4, "SIP\tGetting local URI from registeration: " << handler->GetAddressOfRecord());
  return handler->GetAddressOfRecord();
}


SIPURL SIPEndPoint::GetDefaultRegisteredPartyName(const OpalTransport & transport)
{
  PIPSocket::Address myAddress(0);
  WORD myPort = GetDefaultSignalPort();
  OpalTransportAddressArray interfaces = GetInterfaceAddresses();

  // find interface that matches transport address
  PIPSocket::Address transportAddress;
  WORD transportPort;
  if (transport.GetLocalAddress().GetIpAndPort(transportAddress, transportPort)) {
    for (PINDEX i = 0; i < interfaces.GetSize(); ++i) {
      PIPSocket::Address interfaceAddress;
      WORD interfacePort;
      if (interfaces[i].GetIpAndPort(interfaceAddress, interfacePort) && 
          interfaceAddress == transportAddress &&
          interfacePort == transportPort) {
        myAddress = interfaceAddress;
        myPort    = interfacePort;
        break;
      }
    }
  }

  if (!myAddress.IsValid() && !interfaces.IsEmpty())
    interfaces[0].GetIpAndPort(myAddress, myPort);

  if (!myAddress.IsValid())
    PIPSocket::GetHostAddress(myAddress);

  if (transport.GetRemoteAddress().GetIpAddress(transportAddress))
    GetManager().TranslateIPAddress(myAddress, transportAddress);

  OpalTransportAddress addr(myAddress, myPort, transport.GetLocalAddress().GetProtoPrefix());
  PString defPartyName(GetDefaultLocalPartyName());
  SIPURL rpn;
  PINDEX pos;
  if ((pos = defPartyName.Find('@')) == P_MAX_INDEX) 
    rpn = SIPURL(defPartyName, addr, myPort);
  else {
    rpn = SIPURL(defPartyName.Left(pos), addr, myPort);   // set transport from address
    rpn.SetHostName(defPartyName.Mid(pos+1));
  }

  rpn.SetDisplayName(GetDefaultDisplayName());
  PTRACE(4, "SIP\tGenerated default local URI: " << rpn);
  return rpn;
}


void SIPEndPoint::AdjustToRegistration(SIP_PDU & pdu,
                                       const OpalTransport & transport,
                                       const SIPConnection * connection)
{
  if (!PAssert(transport.IsOpen(), PLogicError))
    return;

  // When we start adding things like P-Asserted-Identity, this function will change
  // Right now it just does the Contact field

  SIPMIMEInfo & mime = pdu.GetMIME();

  SIPURL from(mime.GetFrom());
  SIPURL to(mime.GetTo());

  PString user, domain;
  if (pdu.GetMethod() == SIP_PDU::NumMethods) {
    user   = to.GetUserName();
    domain = from.GetHostName();
    if (connection != NULL && to.GetDisplayName() != connection->GetDisplayName()) {
      to.SetDisplayName(connection->GetDisplayName());
      mime.SetTo(to.AsQuotedString());
    }
  }
  else {
    user   = from.GetUserName();
    domain = to.GetHostName();
  }

  const SIPRegisterHandler * registrar = NULL;
  PSafePtr<SIPHandler> handler = activeSIPHandlers.FindSIPHandlerByUrl("sip:"+user+'@'+domain, SIP_PDU::Method_REGISTER, PSafeReadOnly);
  // If precise AOR not found, locate the name used for the domain.
  if (handler == NULL && !m_registeredUserMode)
    handler = activeSIPHandlers.FindSIPHandlerByDomain(domain, SIP_PDU::Method_REGISTER, PSafeReadOnly);
  if (handler != NULL) {
    registrar = dynamic_cast<const SIPRegisterHandler *>(&*handler);
    PAssertNULL(registrar);
  }

  if (!mime.Has("Contact") && pdu.GetStatusCode() != SIP_PDU::Information_Trying) {
    SIPURL contact;
    if (registrar != NULL) {
      PIPSocket::Address ip;
      bool transportLocal = transport.GetRemoteAddress().GetIpAddress(ip) && manager.IsLocalAddress(ip);

      const SIPURLList & contacts = registrar->GetContacts();
      for (SIPURLList::const_iterator it = contacts.begin(); it != contacts.end(); ++it) {
        OpalTransportAddress contactAddress = it->GetHostAddress();
        if (transport.GetLocalAddress().IsCompatible(contactAddress) &&
            contactAddress.GetIpAddress(ip) &&
            manager.IsLocalAddress(ip) == transportLocal) {
          contact = *it;
          break;
        }
      }
    }

    if (contact.IsEmpty())
      contact = SIPURL(user, transport.GetLocalAddress());

    if (connection != NULL) {
      PSafePtr<OpalConnection> other = connection->GetOtherPartyConnection();
      if (other != NULL && other->GetConferenceState(NULL))
        contact.GetFieldParameters().Set("isfocus", "");

      contact.SetDisplayName(connection->GetDisplayName());
    }

    contact.Sanitise(SIPURL::ContactURI);
    mime.SetContact(contact.AsQuotedString());
  }

  if (!mime.Has("Route") && registrar != NULL) {
    if (!pdu.SetRoute(registrar->GetServiceRoute()))
      pdu.SetRoute(registrar->GetProxy());
  }
}


PSafePtr<SIPHandler> SIPEndPoint::FindHandlerByPDU(const SIP_PDU & pdu, PSafetyMode mode)
{
  const SIPMIMEInfo & mime = pdu.GetMIME();

  PSafePtr<SIPHandler> handler;

  PString id = mime.GetCallID();
  if ((handler = activeSIPHandlers.FindSIPHandlerByCallID(id, mode)) != NULL)
    return handler;

  PString tag = mime.GetTo().GetTag();
  if ((handler = activeSIPHandlers.FindSIPHandlerByCallID(tag, mode)) != NULL)
    return handler;

  return activeSIPHandlers.FindSIPHandlerByCallID(id+';'+tag, mode);
}


///////////////////////////////////////////////////////////////////////////////////////////////////

SIPEndPoint::SIP_Work::SIP_Work(SIPEndPoint & ep, SIP_PDU * pdu, const PString & token)
  : m_endpoint(ep)
  , m_pdu(pdu)
  , m_token(token)
{
  PTRACE(4, "SIP\tQueueing PDU \"" << *m_pdu << "\", transaction="
         << m_pdu->GetTransactionID() << ", token=" << m_token);
}


SIPEndPoint::SIP_Work::~SIP_Work()
{
  delete m_pdu;
}


void SIPEndPoint::SIP_Work::Work()
{
  if (PAssertNULL(m_pdu) == NULL)
    return;

  if (m_pdu->GetMethod() == SIP_PDU::NumMethods) {
    PString transactionID = m_pdu->GetTransactionID();
    PSafePtr<SIPTransaction> transaction = m_endpoint.GetTransaction(transactionID, PSafeReference);
    if (transaction != NULL) {
      PTRACE_CONTEXT_ID_PUSH_THREAD(*transaction);
      PTRACE(3, "SIP\tHandling PDU \"" << *m_pdu << "\" for transaction=" << transactionID);
      transaction->OnReceivedResponse(*m_pdu);
      PTRACE(4, "SIP\tHandled PDU \"" << *m_pdu << '"');
    }
    else {
      PTRACE(2, "SIP\tCannot find transaction " << transactionID << " for response PDU \"" << *m_pdu << '"');
    }
  }

  else if (PAssert(!m_token.IsEmpty(), PInvalidParameter)) {
    PSafePtr<SIPConnection> connection = m_endpoint.GetSIPConnectionWithLock(m_token, PSafeReference);
    if (connection != NULL) {
      PTRACE_CONTEXT_ID_PUSH_THREAD(*connection);
      PTRACE(3, "SIP\tHandling PDU \"" << *m_pdu << "\" for token=" << m_token);
      connection->OnReceivedPDU(*m_pdu);
      PTRACE(4, "SIP\tHandled PDU \"" << *m_pdu << '"');
    }
    else {
      PTRACE(2, "SIP\tCannot find connection for PDU \"" << *m_pdu << "\" using token=" << m_token);
    }
  }
}


///////////////////////////////////////////////////////////////////////////////////////////////

SIPEndPoint::InterfaceMonitor::InterfaceMonitor(SIPEndPoint & ep, PINDEX priority)
  : PInterfaceMonitorClient(priority) 
  , m_endpoint(ep)
{
}

        
void SIPEndPoint::InterfaceMonitor::OnAddInterface(const PIPSocket::InterfaceEntry &)
{
  if (priority == SIPEndPoint::LowPriority) {
    for (PSafePtr<SIPHandler> handler = m_endpoint.activeSIPHandlers.GetFirstHandler(); handler != NULL; ++handler) {
      if (handler->GetState() == SIPHandler::Unavailable)
        handler->ActivateState(SIPHandler::Restoring);
    }
  } else {
    // special case if interface filtering is used: A new interface may 'hide' the old interface.
    // If this is the case, remove the transport interface. 
    //
    // There is a race condition: If the transport interface binding is cleared AFTER
    // PMonitoredSockets::ReadFromSocket() is interrupted and starts listening again,
    // the transport will still listen on the old interface only. Therefore, clear the
    // socket binding BEFORE the monitored sockets update their interfaces.
    if (PInterfaceMonitor::GetInstance().HasInterfaceFilter()) {
      for (PSafePtr<SIPHandler> handler = m_endpoint.activeSIPHandlers.GetFirstHandler(PSafeReadOnly); handler != NULL; ++handler) {
        OpalTransport *transport = handler->GetTransport();
        if (transport != NULL) {
          PString iface = transport->GetInterface();
          if (iface.IsEmpty()) // not connected
            continue;
          
          PIPSocket::Address addr;
          if (!transport->GetRemoteAddress().GetIpAddress(addr))
            continue;
          
          PStringArray ifaces = GetInterfaces(PFalse, addr);
          
          if (ifaces.GetStringsIndex(iface) == P_MAX_INDEX) { // original interface no longer available
            transport->SetInterface(PString::Empty());
            handler->SetState(SIPHandler::Unavailable);
          }
        }
      }
    }
  }
}


void SIPEndPoint::InterfaceMonitor::OnRemoveInterface(const PIPSocket::InterfaceEntry & entry)
{
  if (priority == SIPEndPoint::LowPriority) {
    for (PSafePtr<SIPHandler> handler = m_endpoint.activeSIPHandlers.GetFirstHandler(PSafeReadOnly); handler != NULL; ++handler) {
      if (handler->GetState() == SIPHandler::Subscribed &&
          handler->GetTransport() != NULL &&
          handler->GetTransport()->GetInterface().Find(entry.GetName()) != P_MAX_INDEX) {
        handler->GetTransport()->SetInterface(PString::Empty());
        handler->ActivateState(SIPHandler::Refreshing);
      }
    }
  }
}


#else

  #ifdef _MSC_VER
    #pragma message("SIP support DISABLED")
  #endif

#endif // OPAL_SIP

// End of file ////////////////////////////////////////////////////////////////
