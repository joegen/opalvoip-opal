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
#include <sip/sippres.h>
#include <im/sipim.h>


class SIP_PDU_Work : public SIPWorkItem
{
  public:
    SIP_PDU_Work(SIPEndPoint & ep, const PString & token, SIP_PDU * pdu);
    virtual ~SIP_PDU_Work();

    virtual void Work();

    SIP_PDU * m_pdu;
};

#define new PNEW


static BYTE DefaultKeepAliveData[] = { '\r', '\n', '\r', '\n' };


////////////////////////////////////////////////////////////////////////////

SIPEndPoint::SIPEndPoint(OpalManager & mgr, unsigned maxThreads)
  : OpalRTPEndPoint(mgr, "sip", CanTerminateCall|SupportsE164)
  , m_defaultPrackMode(SIPConnection::e_prackSupported)
  , m_maxPacketSizeUDP(1300)         // As per RFC 3261 section 18.1.1
  , maxRetries(10)
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
  , m_keepAliveTimeout(0, 0, 1)      // 1 minute
  , m_keepAliveType(NoKeepAlive)
  , m_registeredUserMode(false)
  , m_shuttingDown(false)
  , m_defaultAppearanceCode(-1)
  , m_threadPool(maxThreads, "SIP Pool")
  , m_disableTrying(true)
{
  defaultSignalPort = SIPURL::DefaultPort;

  m_allowedEvents += SIPEventPackage(SIPSubscribe::Dialog);
  m_allowedEvents += SIPEventPackage(SIPSubscribe::Conference);

  // Make sure these have been contructed now to avoid
  // payload type disambiguation problems.
  GetOpalRFC2833();

#if OPAL_T38_CAPABILITY
  GetOpalCiscoNSE();
#endif

#if OPAL_PTLIB_SSL
  manager.AttachEndPoint(this, "sips");
#endif

  PInterfaceMonitor::GetInstance().AddNotifier(PCREATE_InterfaceNotifier(OnHighPriorityInterfaceChange), 80);
  PInterfaceMonitor::GetInstance().AddNotifier(PCREATE_InterfaceNotifier(OnLowPriorityInterfaceChange), 30);

  PTRACE(4, "SIP\tCreated endpoint.");
}


SIPEndPoint::~SIPEndPoint()
{
  PInterfaceMonitor::GetInstance().RemoveNotifier(PCREATE_InterfaceNotifier(OnHighPriorityInterfaceChange));
  PInterfaceMonitor::GetInstance().RemoveNotifier(PCREATE_InterfaceNotifier(OnLowPriorityInterfaceChange));
}


void SIPEndPoint::ShutDown()
{
  PTRACE(4, "SIP\tShutting down.");
  m_shuttingDown = true;

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


void SIPEndPoint::NewIncomingConnection(OpalListener &, const OpalTransportPtr & transport)
{
  if (transport == NULL)
    return;

  if (!transport->IsReliable()) {
    HandlePDU(transport); // Always just one PDU
    return;
  }

  PTRACE(2, "SIP\tListening thread started.");

  AddTransport(transport);

  do {
    HandlePDU(transport);
  } while (transport->IsGood());

  PTRACE(2, "SIP\tListening thread finished.");
}


void SIPEndPoint::AddTransport(const OpalTransportPtr & transport)
{
  switch (m_keepAliveType) {
    case KeepAliveByCRLF :
      transport->SetKeepAlive(m_keepAliveTimeout, PBYTEArray(DefaultKeepAliveData, sizeof(DefaultKeepAliveData)));
      break;

    case KeepAliveByOPTION :
    {
      SIPURL addr(transport->GetRemoteAddress());
      SIP_PDU pdu(SIP_PDU::Method_OPTIONS, transport);
      pdu.InitialiseHeaders(addr, addr, addr, SIPTransaction::GenerateCallID(), 1);
      PString str;
      PINDEX len;
      pdu.Build(str, len);
      transport->SetKeepAlive(m_keepAliveTimeout, PBYTEArray((const BYTE *)str.GetPointer(), len));
      break;
    }

    default :
      break;
  }

  m_transportsTable.SetAt(transport->GetRemoteAddress(), transport);
  PTRACE(4, "SIP\tRemembering transport " << *transport);
}


void SIPEndPoint::TransportThreadMain(OpalTransportPtr transport)
{
  PTRACE(4, "SIP\tRead thread started.");

  do {
    HandlePDU(transport);
  } while (transport->IsGood());

  transport->Close();

  PTRACE(4, "SIP\tRead thread finished.");
}


OpalTransportPtr SIPEndPoint::GetTransport(const SIPTransactionOwner & transactor,
                                            SIP_PDU::StatusCodes & reason)
{
  OpalTransportAddress remoteAddress = transactor.GetRemoteTransportAddress();
  if (remoteAddress.IsEmpty()) {
    for (PSafePtr<SIPHandler> handler = activeSIPHandlers.GetFirstHandler(); ; ++handler) {
      if (handler == NULL) {
        reason = SIP_PDU::Local_CannotMapScheme;
        PTRACE(1, "SIP\tCannot use " << transactor.GetRequestURI().GetScheme() << " URI without phone-context or existing registration.");
        return NULL;
      }
      if (handler->GetMethod() == SIP_PDU::Method_REGISTER) {
        remoteAddress = handler->GetRemoteTransportAddress();
        break;
      }
    }
  }

  // See if already have a link to that remote
  OpalTransportPtr transport = m_transportsTable.FindWithLock(remoteAddress, PSafeReference);
  if (transport != NULL && transport->IsOpen()) {
    PTRACE(4, "SIP\tFound existing transport " << *transport);
    return transport;
  }

  if (transport == NULL) {
    // No link, so need to create one
    OpalTransportAddress localAddress;
    PString localInterface = transactor.GetInterface();
    if (localInterface.IsEmpty())
      localInterface = transactor.GetRemoteURI().GetParamVars()(OPAL_INTERFACE_PARAM);
    if (!localInterface.IsEmpty())
      localAddress = OpalTransportAddress(localInterface, 0, remoteAddress.GetProtoPrefix());
    else {
      PString domain = transactor.GetRequestURI().GetHostPort();
      PSafePtr<SIPHandler> handler = activeSIPHandlers.FindSIPHandlerByDomain(domain, SIP_PDU::Method_REGISTER, PSafeReadOnly);
      if (handler != NULL) {
        localAddress = handler->GetInterface();
        if (localAddress.IsEmpty()) {
          PTRACE(4, "SIP\tNo transport to registrar on domain " << domain);
        }
        else {
          PTRACE(4, "SIP\tFound registrar on domain " << domain << ", using interface " << handler->GetInterface());
        }
      }
      else {
        PTRACE(4, "SIP\tNo registrar on domain " << domain);
      }
    }

    for (OpalListenerList::iterator listener = listeners.begin(); listener != listeners.end(); ++listener) {
      if ((transport = listener->CreateTransport(localAddress, remoteAddress)) != NULL)
        break;
    }

    if (transport == NULL) {
      // No compatible listeners, can't create a transport to send if we cannot hear the responses!
      PTRACE(2, "SIP\tNo compatible listener to create transport for " << remoteAddress);
      reason = SIP_PDU::Local_NoCompatibleListener;
      return NULL;
    }

    if (!transport->SetRemoteAddress(remoteAddress)) {
      PTRACE(1, "SIP\tCould not find " << remoteAddress);
      return NULL;
    }

    transport->GetChannel()->SetBufferSize(m_maxSizeUDP);

    PTRACE(4, "SIP\tCreated transport " << *transport);
  }

  // Link just created or was closed/lost
  if (!transport->Connect()) {
    PTRACE(1, "SIP\tCould not connect to " << remoteAddress << " - " << transport->GetErrorText());
    switch (transport->GetErrorCode()) {
      case PChannel::Timeout :
        reason = SIP_PDU::Local_Timeout;
        break;
      case PChannel::AccessDenied :
        reason = SIP_PDU::Local_NotAuthenticated;
        break;
      default :
        reason = SIP_PDU::Local_TransportError;
    }
    transport->CloseWait();
    return NULL;
  }

  if (!transport->IsAuthenticated(transactor.GetRequestURI().GetHostName())) {
    PTRACE(1, "SIP\tCould not connect to " << remoteAddress << " - " << transport->GetErrorText());
    reason = SIP_PDU::Local_NotAuthenticated;
    transport->CloseWait();
    return NULL;
  }

  if (transport->IsReliable())
    transport->AttachThread(new PThreadObj1Arg<SIPEndPoint, OpalTransportPtr>
            (*this, transport, &SIPEndPoint::TransportThreadMain, false, "SIP Transport", PThread::HighestPriority));
  else
    transport->SetPromiscuous(OpalTransport::AcceptFromAny);

  AddTransport(transport);
  return transport;
}


void SIPEndPoint::HandlePDU(const OpalTransportPtr & transport)
{
  // create a SIP_PDU structure, then get it to read and process PDU
  SIP_PDU * pdu = new SIP_PDU(SIP_PDU::NumMethods, transport);

  PTRACE(4, "SIP\tWaiting for PDU on " << *transport);
  SIP_PDU::StatusCodes status = pdu->Read();
  switch (status/100) {
    case 0 :
      if (status == SIP_PDU::Local_KeepAlive)
        transport->Write("\r\n", 2); // Send PONG
      break;

    case 2 :
      if (OnReceivedPDU(pdu)) 
        return;
      break;

    default :
      const SIPMIMEInfo & mime = pdu->GetMIME();
      if (!mime.GetCSeq().IsEmpty() &&
          !mime.GetVia().IsEmpty() &&
          !mime.GetCallID().IsEmpty() &&
          !mime.GetFrom().IsEmpty() &&
          !mime.GetTo().IsEmpty())
        pdu->SendResponse(status);
  }

  delete pdu;
}


static PString TranslateENUM(const PString & remoteParty)
{
#if OPAL_PTLIB_DNS_RESOLVER
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
#endif // OPAL_PTLIB_DNS_RESOLVER

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

  SIPConnection::Init init(call);
  init.m_token = SIPURL::GenerateTag();
  init.m_userData = userData;
  init.m_address = TranslateENUM(remoteParty);
  init.m_options = options;
  init.m_stringOptions = stringOptions;
  return AddConnection(CreateConnection(init));
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

  {
    PSafePtr<SIPTransactionBase> transaction(m_transactions, PSafeReference);
    while (transaction != NULL) {
      if (transaction->IsTerminated())
        m_transactions.Remove(transaction++);
      else
        ++transaction;
    }
  }
  bool transactionsDone = m_transactions.DeleteObjectsToBeRemoved();

  {
    PSafePtr<SIPHandler> handler = activeSIPHandlers.GetFirstHandler();
    while (handler != NULL) {
      // If unsubscribed then we do the shut down to clean up the handler
      if (handler->GetState() == SIPHandler::Unsubscribed && handler->ShutDown())
        activeSIPHandlers.Remove(handler++);
      else
        ++handler;
    }
  }
  bool handlersDone = activeSIPHandlers.DeleteObjectsToBeRemoved();


  {
    PWaitAndSignal mutex(m_transportsTable.GetMutex());
    PArray<OpalTransportAddress> keys = m_transportsTable.GetKeys();
    for (PINDEX i = 0; i < keys.GetSize(); ++i) {
      const OpalTransportAddress & addr = keys[i];
      PSafePtr<OpalTransport> transport = m_transportsTable.FindWithLock(addr, PSafeReference);
      if (transport != NULL) {
        PWaitAndSignal mutex(m_transportsTable.GetMutex());
        if (transport->GetSafeReferenceCount() == 1) {
          PTRACE(3, "SIP\tRemoving transport to " << addr);
          m_transportsTable.RemoveAt(addr);
        }
      }
    }
  }
  bool transportsDone = m_transportsTable.DeleteObjectsToBeRemoved();

  if (!OpalEndPoint::GarbageCollection())
    return false;

  if (m_shuttingDown)
    return transactionsDone && handlersDone && transportsDone;

  return true;
}


PBoolean SIPEndPoint::IsAcceptedAddress(const SIPURL & /*toAddr*/)
{
  return true;
}



SIPConnection * SIPEndPoint::CreateConnection(const SIPConnection::Init & init)
{
  return new SIPConnection(*this, init);
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

  SIPConnection::Init init(call);
  init.m_token = SIPURL::GenerateTag();
  init.m_userData = userData;
  init.m_address = TranslateENUM(remoteParty);
  init.m_stringOptions = &options;
  SIPConnection * connection = CreateConnection(init);
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
  
  SIPConnection::Init init(call);
  init.m_token = SIPURL::GenerateTag();
  init.m_address = forwardParty;
  SIPConnection * conn = CreateConnection(init);
  if (!AddConnection(conn))
    return false;

  call.OnReleased(connection);
  
  conn->SetUpConnection();
  connection.Release(OpalConnection::EndedByCallForwarded);

  return true;
}


bool SIPEndPoint::ClearDialogContext(const PString & descriptor)
{
  SIPDialogContext context;
  return context.FromString(descriptor) && ClearDialogContext(context);
}


bool SIPEndPoint::ClearDialogContext(SIPDialogContext & context)
{
  if (!context.IsEstablished())
    return true; // Was not actually fully formed dialog, assume cleared

  /* This is an extra increment of the sequence number to allow for
     any PDU's in the dialog being sent between the last saved
     context. Highly unlikely this will ever be by a million ... */
  context.IncrementCSeq(1000000);

  PSafePtr<SIPTransaction> byeTransaction = new SIPBye(*this, context);
  byeTransaction->WaitForCompletion();
  return !byeTransaction->IsFailed();
}


bool SIPEndPoint::OnReceivedPDU(SIP_PDU * pdu)
{
  if (PAssertNULL(pdu) == NULL)
    return false;

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
        new SIP_PDU_Work(*this, token, pdu);
        return true;
      }
      break;

    case SIP_PDU::Method_INVITE :
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
                return false;

              case SIPConnection::IsReINVITE : // Pass on to worker thread if re-INVITE
                new SIP_PDU_Work(*this, token, pdu);
                return true;

              case SIPConnection::IsLoopedINVITE : // Send back error if looped INVITE
                SIP_PDU response(*pdu, SIP_PDU::Failure_LoopDetected);
                response.GetMIME().SetProductInfo(GetUserAgent(), connection->GetProductInfo());
                response.Send();
                return false;
            }
          }
        }

        pdu->SendResponse(SIP_PDU::Information_Trying);
        return OnReceivedConnectionlessPDU(pdu);
      }

      if (!hasToConnection) {
        // Has to tag but doesn't correspond to anything, odd.
        pdu->SendResponse(SIP_PDU::Failure_TransactionDoesNotExist);
        return false;
      }
      pdu->SendResponse(SIP_PDU::Information_Trying);
      break;

    case SIP_PDU::Method_ACK :
      break;

    case SIP_PDU::NumMethods :  // unknown method
      break;

    default :   // any known method other than INVITE, CANCEL and ACK
      if (!m_disableTrying)
        pdu->SendResponse(SIP_PDU::Information_Trying);
      break;
  }

  if (hasToConnection)
    token = toToken;
  else if (hasFromConnection)
    token = fromToken;
  else
    return OnReceivedConnectionlessPDU(pdu);

  new SIP_PDU_Work(*this, token, pdu);
  return true;
}


bool SIPEndPoint::OnReceivedConnectionlessPDU(SIP_PDU * pdu)
{
  if (pdu->GetMethod() == SIP_PDU::NumMethods || pdu->GetMethod() == SIP_PDU::Method_CANCEL) {
    PString id = pdu->GetTransactionID();
    PSafePtr<SIPTransaction> transaction = GetTransaction(id, PSafeReadOnly);
    if (transaction != NULL) {
      SIPConnection * connection = transaction->GetConnection();
      new SIP_PDU_Work(*this, connection != NULL ? connection->GetToken() : id, pdu);
      return true;
    }

    PTRACE(2, "SIP\tReceived response for unmatched transaction, id=" << id);
    if (pdu->GetMethod() == SIP_PDU::Method_CANCEL)
      pdu->SendResponse(SIP_PDU::Failure_TransactionDoesNotExist);
    return false;
  }

  // Prevent any new INVITE/SUBSCRIBE etc etc while we are on the way out.
  if (m_shuttingDown) {
    pdu->SendResponse(SIP_PDU::Failure_ServiceUnavailable);
    return false;
  }

  // Check if we have already received this command (have a transaction)
  {
    PString id = pdu->GetTransactionID();
    PSafePtr<SIPResponse> transaction = PSafePtrCast<SIPTransaction, SIPResponse>(GetTransaction(id, PSafeReadOnly));
    if (transaction != NULL) {
      PTRACE(4, "SIP\tRetransmitting previous response for transaction id=" << id);
      transaction->Resend(*pdu);
      return false;
    }
  }

  switch (pdu->GetMethod()) {
    case SIP_PDU::Method_INVITE :
      return OnReceivedINVITE(pdu);

    case SIP_PDU::Method_REGISTER :
      if (OnReceivedREGISTER(*pdu))
        return false;
      break;

    case SIP_PDU::Method_SUBSCRIBE :
      if (OnReceivedSUBSCRIBE(*pdu, NULL))
        return false;
      break;

    case SIP_PDU::Method_NOTIFY :
       if (OnReceivedNOTIFY(*pdu))
         return false;
       break;

    case SIP_PDU::Method_MESSAGE :
      if (OnReceivedMESSAGE(*pdu))
        return false;
      break;
   
    case SIP_PDU::Method_OPTIONS :
      if (OnReceivedOPTIONS(*pdu))
        return false;
      break;

    case SIP_PDU::Method_BYE :
      // If we receive a BYE outside of the context of a connection, tell them.
      pdu->SendResponse(SIP_PDU::Failure_TransactionDoesNotExist);
      return false;

      // If we receive an ACK outside of the context of a connection, ignore it.
    case SIP_PDU::Method_ACK :
      return false;

    default :
      break;
  }

  SIP_PDU response(*pdu, SIP_PDU::Failure_MethodNotAllowed);
  response.SetAllow(GetAllowedMethods()); // Required by spec
  response.Send();
  return false;
}


bool SIPEndPoint::OnReceivedREGISTER(SIP_PDU & /*request*/)
{
  return false;
}


bool SIPEndPoint::OnReceivedSUBSCRIBE(SIP_PDU & request, SIPDialogContext * dialog)
{
  SIPMIMEInfo & mime = request.GetMIME();

  SIPSubscribe::EventPackage eventPackage(mime.GetEvent());

  CanNotifyResult canNotify = CanNotifyImmediate;

  // See if already subscribed. Now this is not perfect as we only check the call-id and strictly
  // speaking we should check the from-tag and to-tags as well due to it being a dialog.
  PSafePtr<SIPHandler> handler = FindHandlerByPDU(request, PSafeReadWrite);
  if (handler == NULL) {
    SIPDialogContext newDialog(mime);
    if (dialog == NULL)
      dialog = &newDialog;

    if ((canNotify = CanNotify(eventPackage, dialog->GetLocalURI())) == CannotNotify) {
      SIPResponse * response = new SIPResponse(*this, request, SIP_PDU::Failure_BadEvent);
      response->GetMIME().SetAllowEvents(m_allowedEvents); // Required by spec
      response->Send();
      return true;
    }

    handler = new SIPNotifyHandler(*this, eventPackage, *dialog);
    handler.SetSafetyMode(PSafeReadWrite);
    activeSIPHandlers.Append(handler);

    mime.SetTo(dialog->GetLocalURI());
  }

  // Update expiry time
  unsigned expires = mime.GetExpires();

  SIPResponse * response = new SIPResponse(*this, request, SIP_PDU::Successful_Accepted);
  response->GetMIME().SetEvent(eventPackage); // Required by spec
  response->GetMIME().SetExpires(expires);    // Required by spec
  response->Send();

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


void SIPEndPoint::OnReceivedResponse(SIPTransaction &, SIP_PDU &)
{
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


bool SIPEndPoint::OnReceivedINVITE(SIP_PDU * request)
{
  SIPMIMEInfo & mime = request->GetMIME();

  // parse the incoming To field, and check if we accept incoming calls for this address
  SIPURL toAddr(mime.GetTo());
  if (!IsAcceptedAddress(toAddr)) {
    PTRACE(2, "SIP\tIncoming INVITE for " << request->GetURI() << " for unacceptable address " << toAddr);
    request->SendResponse(SIP_PDU::Failure_NotFound);
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
    response.Send();
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
      request->SendResponse(errorCode);
      return false;
    }

    // Use the existing call instance when replacing the SIP side of it.
    call = &replacedConnection->GetCall();
    PTRACE(3, "SIP\tIncoming INVITE replaces connection " << *replacedConnection);
  }

  if (call == NULL) {
    // Get new instance of a call, abort if none created
    call = manager.InternalCreateCall();
    if (call == NULL) {
      request->SendResponse(SIP_PDU::Failure_TemporarilyUnavailable);
      return false;
    }
  }

  PTRACE_CONTEXT_ID_PUSH_THREAD(call);

  // ask the endpoint for a connection
  SIPConnection::Init init(*call);
  init.m_token = SIPURL::GenerateTag();
  init.m_invite = request;
  SIPConnection *connection = CreateConnection(init);
  if (!AddConnection(connection)) {
    PTRACE(1, "SIP\tFailed to create SIPConnection for INVITE for " << request->GetURI() << " to " << toAddr);
    request->SendResponse(SIP_PDU::Failure_NotFound);
    return false;
  }

  // m_receivedConnectionMutex already set
  PString token = connection->GetToken();
  m_receivedConnectionTokens.SetAt(mime.GetCallID(), token);

  // Get the connection to handle the rest of the INVITE in the thread pool
  new SIP_PDU_Work(*this, token, request);

  return true;
}


void SIPEndPoint::OnTransactionFailed(SIPTransaction &)
{
}


bool SIPEndPoint::OnReceivedNOTIFY(SIP_PDU & request)
{
  const SIPMIMEInfo & mime = request.GetMIME();
  SIPEventPackage eventPackage(mime.GetEvent());

  PTRACE(3, "SIP\tReceived NOTIFY " << eventPackage);
  
  // A NOTIFY will have the same CallID than the SUBSCRIBE request it corresponds to
  // Technically should check for whole dialog, but call-id will do.
  PSafePtr<SIPHandler> handler = FindHandlerByPDU(request, PSafeReadWrite);

  if (handler == NULL && eventPackage == SIPSubscribe::MessageSummary) {
    PTRACE(4, "SIP\tWork around Asterisk bug in message-summary event package.");
    SIPURL to(mime.GetTo().GetUserName() + "@" + mime.GetFrom().GetHostName());
    handler = activeSIPHandlers.FindSIPHandlerByUrl(to, SIP_PDU::Method_SUBSCRIBE, eventPackage, PSafeReadWrite);
  }

  if (handler == NULL) {
    PTRACE(3, "SIP\tCould not find a SUBSCRIBE corresponding to the NOTIFY " << eventPackage);
    SIPResponse * response = new SIPResponse(*this, request, SIP_PDU::Failure_TransactionDoesNotExist);
    response->Send();
    return true;
  }

  PTRACE(3, "SIP\tFound a SUBSCRIBE corresponding to the NOTIFY " << eventPackage);
  return handler->OnReceivedNOTIFY(request);
}


bool SIPEndPoint::OnReceivedMESSAGE(SIP_PDU & request)
{
  // handle a MESSAGE received outside the context of a call
  PTRACE(3, "SIP\tReceived MESSAGE outside the context of a call");

  // if there is a callback, assume that the application knows what it is doing
  if (!m_onConnectionlessMessage.IsNULL()) {
    ConnectionlessMessageInfo info(request);
    m_onConnectionlessMessage(*this, info);
    switch (info.m_status) {
      case ConnectionlessMessageInfo::MethodNotAllowed :
        return false;

      case ConnectionlessMessageInfo::SendOK :
        {
          SIPResponse * response = new SIPResponse(*this, request, SIP_PDU::Successful_OK);
          response->Send();
        }
        // Do next case

      case ConnectionlessMessageInfo::ResponseSent :
        return true;

      default :
        break;
    }
  }

#if OPAL_HAS_SIPIM
  OpalSIPIMContext::OnReceivedMESSAGE(*this, NULL, request);
#else
  request.SendResponse(SIP_PDU::Failure_BadRequest);
#endif
  return true;
}


bool SIPEndPoint::OnReceivedOPTIONS(SIP_PDU & request)
{
  SIPResponse * response = new SIPResponse(*this, request, SIP_PDU::Successful_OK);
  response->Send();
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
  PTRACE(5, "SIP\tNormalised REGISTER\n" << params);
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


bool SIPEndPoint::GetRegistrationStatus(const PString & token, RegistrationStatus & status)
{
  PSafePtr<SIPHandler> handler = activeSIPHandlers.FindSIPHandlerByCallID(token, PSafeReference);
  if (handler == NULL)
    handler = activeSIPHandlers.FindSIPHandlerByUrl(token, SIP_PDU::Method_REGISTER, PSafeReference);

  if (handler == NULL) {
    PTRACE(1, "SIP\tCould not find active REGISTER for " << token);
    return false;
  }

  status.m_handler = dynamic_cast<SIPRegisterHandler *>(&*handler);
  status.m_addressofRecord = handler->GetAddressOfRecord();
  status.m_wasRegistering = handler->GetState() != SIPHandler::Unsubscribing;
  status.m_reRegistering = handler->GetState() == SIPHandler::Subscribed;
  status.m_reason = handler->GetLastResponseStatus();
  status.m_productInfo = handler->GetProductInfo();
  status.m_userData = NULL;
  return true;
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
  PTRACE(5, "SIP\tNormalised SUBSCRIBE\n" << params);
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
  PSafePtr<SIPSubscribeHandler> handler = PSafePtrCast<SIPHandler, SIPSubscribeHandler>(
                                                activeSIPHandlers.FindSIPHandlerByCallID(token, PSafeReference));
  if (handler == NULL)
    handler = PSafePtrCast<SIPHandler, SIPSubscribeHandler>(
          activeSIPHandlers.FindSIPHandlerByUrl(token, SIP_PDU::Method_SUBSCRIBE, eventPackage, PSafeReference));
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
    SIPSubscribe::Params params(handler->GetParams());
    params.m_onNotify = NULL;
    params.m_onSubcribeStatus = NULL;
    handler->UpdateParameters(params);
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


bool SIPEndPoint::GetSubscriptionStatus(const PString & token, const PString & eventPackage, SubscriptionStatus & status)
{
  PSafePtr<SIPSubscribeHandler> handler = PSafePtrCast<SIPHandler, SIPSubscribeHandler>(
                                                activeSIPHandlers.FindSIPHandlerByCallID(token, PSafeReference));
  if (handler == NULL)
    handler = PSafePtrCast<SIPHandler, SIPSubscribeHandler>(
          activeSIPHandlers.FindSIPHandlerByUrl(token, SIP_PDU::Method_SUBSCRIBE, eventPackage, PSafeReference));
  else {
    if (!eventPackage.IsEmpty() && handler->GetEventPackage() != eventPackage)
      handler.SetNULL();
  }

  if (handler == NULL) {
    PTRACE(1, "SIP\tCould not find active SUBSCRIBE of " << eventPackage << " package to " << token);
    return false;
  }

  status.m_handler = handler;
  status.m_addressofRecord = handler->GetAddressOfRecord();
  status.m_wasSubscribing = handler->GetState() != SIPHandler::Unsubscribing;
  status.m_reSubscribing = handler->GetState() == SIPHandler::Subscribed;
  status.m_reason = handler->GetLastResponseStatus();
  status.m_productInfo = handler->GetProductInfo();
  status.m_userData = NULL;
  return true;
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

  PTRACE(3, "SIP\tCannot notify event \"" << eventPackage << "\" not one of ["
         << setfill(',') << m_allowedEvents << setfill(' ') << ']');
  return false;
}


SIPEndPoint::CanNotifyResult SIPEndPoint::CanNotify(const PString & eventPackage, const SIPURL & aor)
{
  if (SIPEventPackage(SIPSubscribe::Conference) == eventPackage) {
    OpalConferenceStates states;
    if (manager.GetConferenceStates(states, aor.GetUserName()) || states.empty()) {
      PString uri = states.front().m_internalURI;
      ConferenceMap::iterator it = m_conferenceAOR.find(uri);
      while (it != m_conferenceAOR.end() && it->first == uri) {
        if (it->second == aor)
          return CanNotifyImmediate;
      }

      m_conferenceAOR.insert(ConferenceMap::value_type(uri, aor));
      return CanNotifyImmediate;
    }

    PTRACE(3, "SIP\tCannot notify \"" << eventPackage << "\" event, no conferences for " << aor);
    return CannotNotify;
  }

#if P_EXPAT
  if (SIPEventPackage(SIPSubscribe::Presence) == eventPackage) {
    PSafePtr<OpalPresentity> presentity = manager.GetPresentity(aor);
    if (presentity != NULL && presentity->GetAttributes().GetEnum(
                  SIP_Presentity::SubProtocolKey, SIP_Presentity::e_WithAgent) == SIP_Presentity::e_PeerToPeer)
      return CanNotifyImmediate;

    PTRACE(3, "SIP\tCannot notify \"" << eventPackage << "\" event, no presentity " << aor);
    return CannotNotify;
  }
#endif // P_EXPAT

  return CanNotify(eventPackage) ? CanNotifyImmediate : CannotNotify;
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


#if OPAL_HAS_SIPIM
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
  PTRACE(5, "SIP\tNormalised OPTIONS\n" << params);
  new SIPOptions(*this, params);
  return true;
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
  PTRACE(5, "SIP\tNormalised PUBLISH\n" << params);
  PSafePtr<SIPHandler> handler = activeSIPHandlers.FindSIPHandlerByUrl(params.m_addressOfRecord, SIP_PDU::Method_PUBLISH, params.m_eventPackage, PSafeReadWrite);
  if (handler != NULL)
    handler->SetBody(params.m_expire != 0 ? body : PString::Empty());
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
  m_proxy = str;
}


void SIPEndPoint::SetProxy(const SIPURL & url) 
{ 
  m_proxy = url; 
  PTRACE_IF(3, !url.IsEmpty(), "SIP\tOutbound proxy for endpoint set to " << url);
}


PString SIPEndPoint::GetUserAgent() const 
{
  return m_userAgentString;
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


bool SIPEndPoint::GetAuthentication(const PString & realm, PString & authId, PString & password)
{
  // Try to find authentication parameters for the given realm
  PSafePtr<SIPHandler> handler = activeSIPHandlers.FindSIPHandlerByAuthRealm(realm, authId, PSafeReadOnly);
  if (handler == NULL) {
    if (m_registeredUserMode)
      return false;

    if ((handler = activeSIPHandlers.FindSIPHandlerByAuthRealm(realm, PSafeReadOnly)) == NULL)
      return false;
  }

  // really just after password, but username MAY change too.
  authId = handler->GetAuthID();
  password = handler->GetPassword();
  PTRACE (3, "SIP\tUsing auth info for realm \"" << realm << '"');
  return true;
}


SIPURL SIPEndPoint::GetDefaultLocalURL(const OpalTransport & transport)
{
  OpalTransportAddress localAddress;

  OpalTransportAddressArray interfaces = GetInterfaceAddresses(true, &transport);
  if (!interfaces.IsEmpty())
    localAddress = interfaces[0];
  else {
    PIPSocket::Address myAddress = PIPSocket::GetInvalidAddress();
    PIPSocket::GetHostAddress(myAddress);
    PIPSocket::Address transportAddress;
    if (transport.GetRemoteAddress().GetIpAddress(transportAddress))
      GetManager().TranslateIPAddress(myAddress, transportAddress);
    localAddress = OpalTransportAddress(myAddress, GetDefaultSignalPort(), transport.GetProtoPrefix());
  }

  SIPURL localURL;

  PString defPartyName(GetDefaultLocalPartyName());
  PString user, host;
  if (!defPartyName.Split('@', user, host)) 
    localURL = SIPURL(defPartyName, localAddress);
  else {
    localURL = SIPURL(user, localAddress);   // set transport from address
    localURL.SetHostName(host);
  }

  localURL.SetDisplayName(GetDefaultDisplayName());
  PTRACE(4, "SIP\tGenerated default local URI: " << localURL);
  return localURL;
}


void SIPEndPoint::AdjustToRegistration(SIP_PDU & pdu, const SIPConnection * connection, const OpalTransport * transport)
{
  bool isMethod;
  switch (pdu.GetMethod()) {
    case SIP_PDU::Method_REGISTER :
      return;

    case SIP_PDU::NumMethods :
      isMethod = false;
      break;

    default :
      isMethod = true;
  }

  SIPMIMEInfo & mime = pdu.GetMIME();

  SIPURL from(mime.GetFrom());
  SIPURL to(mime.GetTo());

  PString user, domain;
  if (isMethod) {
    user   = from.GetUserName();
    domain = to.GetHostName();
  }
  else {
    user   = to.GetUserName();
    domain = from.GetHostName();
    if (connection != NULL && to.GetDisplayName() != connection->GetDisplayName()) {
      to.SetDisplayName(connection->GetDisplayName());
      to.Sanitise(SIPURL::ToURI);
      mime.SetTo(to);
    }
  }

  const SIPRegisterHandler * registrar = NULL;
  PSafePtr<SIPHandler> handler;

  if (to.GetScheme() != "tel") {
    handler = activeSIPHandlers.FindSIPHandlerByUrl("sip:"+user+'@'+domain, SIP_PDU::Method_REGISTER, PSafeReadOnly);
    PTRACE_IF(4, handler != NULL, "SIP\tFound registrar on aor sip:" << user << '@' << domain);
  }
  else  if (domain.IsEmpty() || OpalIsE164(domain)) {
    // No context, just get first registration
    handler = activeSIPHandlers.GetFirstHandler();
    while (handler != NULL && handler->GetMethod() != SIP_PDU::Method_REGISTER)
      ++handler;
  }

  // If precise AOR not found, locate the name used for the domain.
  if (handler == NULL && !m_registeredUserMode) {
    handler = activeSIPHandlers.FindSIPHandlerByDomain(domain, SIP_PDU::Method_REGISTER, PSafeReadOnly);
    PTRACE_IF(4, handler != NULL, "SIP\tFound registrar on domain " << domain);
  }
  if (handler != NULL) {
    registrar = dynamic_cast<const SIPRegisterHandler *>(&*handler);
    PAssertNULL(registrar);
  }
  else {
    PTRACE(4, "SIP\tNo registrar for aor sip:" << user << '@' << domain);
  }

  if (!mime.Has("Contact") && pdu.GetStatusCode() != SIP_PDU::Information_Trying) {
    OpalTransportAddress localAddress;
    if (transport == NULL)
      transport = pdu.GetTransport();
    if (transport != NULL)
      localAddress = transport->GetLocalAddress();

    SIPURL contact;
    if (registrar != NULL) {
      const SIPURLList & contacts = registrar->GetContacts();
      PTRACE(5, "SIP\tChecking " << localAddress << " against " << contacts.ToString());

      SIPURLList::const_iterator it;
      for (it = contacts.begin(); it != contacts.end(); ++it) {
        if (localAddress == it->GetTransportAddress())
          break;
      }

      if (it == contacts.end()) {
        for (it = contacts.begin(); it != contacts.end(); ++it) {
          if (localAddress.IsCompatible(it->GetTransportAddress()))
            break;
        }
      }

      if (it != contacts.end()) {
        contact = *it;
        PTRACE(4, "SIP\tAdjusted Contact to " << contact << " from registration " << registrar->GetAddressOfRecord());
      }
    }

    if (contact.IsEmpty()) {
      contact = SIPURL(user, localAddress);
      PTRACE(4, "SIP\tUsing transport local address (" << localAddress << ") for Contact");
    }

    if (connection != NULL) {
      PSafePtr<OpalConnection> other = connection->GetOtherPartyConnection();
      if (other != NULL && other->GetConferenceState(NULL))
        contact.GetFieldParameters().Set("isfocus", "");

      contact.SetDisplayName(connection->GetDisplayName());
    }

    contact.Sanitise(SIPURL::ContactURI);
    mime.SetContact(contact.AsQuotedString());
  }

  if (isMethod && registrar != NULL) {
    if (!mime.Has("Route")) {
      if (!pdu.SetRoute(registrar->GetServiceRoute()))
        pdu.SetRoute(registrar->GetProxy());
    }

    // For many registrars From address must be address-of-record, but don;t touch if dialog already done
    if (connection != NULL && !connection->GetDialog().IsEstablished()) {
      PStringToString fieldParams = from.GetFieldParameters();
      from = registrar->GetAddressOfRecord();
      from.GetFieldParameters() = fieldParams;
      from.Sanitise(SIPURL::FromURI);
      mime.SetFrom(from);
    }
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

SIP_PDU_Work::SIP_PDU_Work(SIPEndPoint & ep, const PString & token, SIP_PDU * pdu)
  : SIPWorkItem(ep, token)
  , m_pdu(pdu)
{
  PTRACE(4, "SIP\tQueueing PDU \"" << *m_pdu << "\", transaction="
         << m_pdu->GetTransactionID() << ", token=" << m_token);
  ep.GetThreadPool().AddWork(this);
}


SIP_PDU_Work::~SIP_PDU_Work()
{
  delete m_pdu;
}


void SIP_PDU_Work::Work()
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
  else {
    PSafePtr<SIPConnection> connection = m_endpoint.GetSIPConnectionWithLock(m_token, PSafeReadWrite);
    PTRACE_CONTEXT_ID_PUSH_THREAD(connection);
    if (connection != NULL) {
      PTRACE(3, "SIP\tHandling PDU \"" << *m_pdu << "\" for token=" << m_token);
      connection->OnReceivedPDU(*m_pdu);
      PTRACE(4, "SIP\tHandled PDU \"" << *m_pdu << '"');
    }
  }
}


///////////////////////////////////////////////////////////////////////////////////////////////

void SIPEndPoint::OnHighPriorityInterfaceChange(PInterfaceMonitor &, PInterfaceMonitor::InterfaceChange entry)
{
  if (entry.m_added) {
    // special case if interface filtering is used: A new interface may 'hide' the old interface.
    // If this is the case, remove the transport interface. 
    //
    // There is a race condition: If the transport interface binding is cleared AFTER
    // PMonitoredSockets::ReadFromSocket() is interrupted and starts listening again,
    // the transport will still listen on the old interface only. Therefore, clear the
    // socket binding BEFORE the monitored sockets update their interfaces.
    if (PInterfaceMonitor::GetInstance().HasInterfaceFilter()) {
      for (PSafePtr<SIPHandler> handler = activeSIPHandlers.GetFirstHandler(PSafeReadOnly); handler != NULL; ++handler) {
        if (handler->GetInterface() == entry.GetName()) {
          handler->ResetInterface();
          handler->SetState(SIPHandler::Unavailable);
        }
      }
    }
  }
}


void SIPEndPoint::OnLowPriorityInterfaceChange(PInterfaceMonitor &, PInterfaceMonitor::InterfaceChange entry)
{
  for (PSafePtr<SIPHandler> handler = activeSIPHandlers.GetFirstHandler(); handler != NULL; ++handler) {
    if (entry.m_added) {
      if (handler->GetState() == SIPHandler::Unavailable)
        handler->ActivateState(SIPHandler::Restoring);
    }
    else {
      if (handler->GetInterface() == entry.GetName()) {
        handler->ResetInterface();
        if (handler->GetState() == SIPHandler::Subscribed)
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
