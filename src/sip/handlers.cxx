/*
 * handlers.cxx
 *
 * Session Initiation Protocol non-connection protocol handlers.
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
 * The Initial Developer of the Original Code is Damien Sandras. 
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision$
 * $Author$
 * $Date$
 *
 * This code implements all or part of the following RFCs
 *
 * RFC 4479 "A Data Model for Presence"
 * RFC 4480 "RPID: Rich Presence Extensions to the Presence Information Data Format (PIDF)"
 */

#include <ptlib.h>
#include <opal/buildopts.h>

#if OPAL_SIP

#ifdef __GNUC__
#pragma implementation "handlers.h"
#endif

#include <sip/handlers.h>

#include <ptclib/pdns.h>
#include <ptclib/enum.h>
#include <sip/sipep.h>
#include <ptclib/pxml.h>

#include <algorithm>


#define new PNEW


#if PTRACING
ostream & operator<<(ostream & strm, SIPHandler::State state)
{
  static const char * const StateNames[] = {
    "Subscribed", "Subscribing", "Unavailable", "Refreshing", "Restoring", "Unsubscribing", "Unsubscribed"
  };
  if (state < PARRAYSIZE(StateNames))
    strm << StateNames[state];
  else
    strm << (unsigned)state;
  return strm;
}
#endif


////////////////////////////////////////////////////////////////////////////

SIPHandler::SIPHandler(const PString & callID)
  : m_endpoint(NULL)
  , authentication(NULL)
  , m_transport(NULL)
  , m_method(SIP_PDU::NumMethods)
  , m_callID(callID)
{
}


SIPHandler::SIPHandler(SIP_PDU::Methods method,
                       SIPEndPoint & ep,
                       const SIPParameters & params,
                       const PString & callID)
  : m_endpoint(&ep)
  , authentication(NULL)
  , m_username(params.m_authID)
  , m_password(params.m_password)
  , m_realm(params.m_realm)
  , m_transport(NULL)
  , m_method(method)
  , m_addressOfRecord(params.m_addressOfRecord)
  , m_remoteAddress(params.m_remoteAddress)
  , m_callID(callID)
  , m_lastCseq(0)
  , m_currentExpireTime(params.m_expire)
  , m_originalExpireTime(params.m_expire)
  , m_offlineExpireTime(params.m_restoreTime)
  , m_state(Unavailable)
  , m_receivedResponse(false)
  , m_proxy(params.m_proxyAddress)
{
  PTRACE_CONTEXT_ID_NEW();

  m_transactions.DisallowDeleteObjects();
  m_expireTimer.SetNotifier(PCREATE_NOTIFIER(OnExpireTimeout));

  if (m_proxy.IsEmpty())
    m_proxy = ep.GetProxy();

  PTRACE(4, "SIP\tConstructed " << m_method << " handler for " << m_addressOfRecord);
}


SIPHandler::~SIPHandler() 
{
  m_expireTimer.Stop();

  if (m_transport) {
    m_transport->CloseWait();
    delete m_transport;
  }

  delete authentication;

  PTRACE_IF(4, !m_addressOfRecord.IsEmpty(),
            "SIP\tDestroyed " << m_method << " handler for " << m_addressOfRecord);
}


PObject::Comparison SIPHandler::Compare(const PObject & obj) const
{
  PAssert(PIsDescendant(&obj, SIPHandler), PInvalidCast);
  const SIPHandler * other = dynamic_cast<const SIPHandler *>(&obj);
  return other != NULL ? GetCallID().Compare(other->GetCallID()) : GreaterThan;
}


bool SIPHandler::ShutDown()
{
  PSafeList<SIPTransaction> transactions;

  {
    PSafeLockReadWrite mutex(*this);
    if (!mutex.IsLocked())
    return true;

  while (!m_stateQueue.empty())
    m_stateQueue.pop();

    switch (GetState()) {
      case Subscribed :
      case Unavailable :
        SendRequest(Unsubscribing);
      case Unsubscribing :
        return m_transactions.IsEmpty();

      default :
        break;
    }

    transactions = m_transactions;
  }

  for (PSafePtr<SIPTransaction> transaction(transactions, PSafeReference); transaction != NULL; ++transaction)
    transaction->Abort();

  return true;
}


void SIPHandler::SetState(SIPHandler::State newState) 
{
  if (m_state == newState)
    return;

  PTRACE(4, "SIP\tChanging " << GetMethod() << " handler from " << GetState() << " to " << newState
         << ", target=" << GetAddressOfRecord() << ", id=" << GetCallID());

  m_state = newState;

  switch (m_state) {
    case Subscribing :
    case Refreshing :
    case Restoring :
    case Unsubscribing :
      return;

    default :
      break;
  }

  if (m_stateQueue.empty())
    return;

  newState = m_stateQueue.front();
  m_stateQueue.pop();
  SendRequest(newState);
}


bool SIPHandler::ActivateState(SIPHandler::State newState)
{
  PTRACE_CONTEXT_ID_PUSH_THREAD(this);

  if (GetState() == Unsubscribed)
    return false;

  // If subscribing with zero expiry time, is same as unsubscribe
  if (newState == Subscribing && GetExpire() == 0)
    newState = Unsubscribing;

  // If unsubscribing and never got a response from server, rather than trying
  // to send more unsuccessful packets, abort transactions and mark Unsubscribed
  if (newState == Unsubscribing && !m_receivedResponse) {
    SetState(Unsubscribed); // Allow garbage collection thread to clean up
    return true;
  }

  static const enum {
    e_Invalid,
    e_NoChange,
    e_Execute,
    e_Queue
  } StateChangeActions[NumStates][NumStates] =
  {
    /* old-state
           V      new-state-> Subscribed  Subscribing Unavailable Refreshing  Restoring   Unsubscribing Unsubscribed */
    /* Subscribed        */ { e_NoChange, e_Execute,  e_Invalid,  e_Execute,  e_Execute,  e_Execute,    e_Invalid  },
    /* Subscribing       */ { e_Invalid,  e_Queue,    e_Invalid,  e_NoChange, e_NoChange, e_Queue,      e_Invalid  },
    /* Unavailable       */ { e_Invalid,  e_Execute,  e_NoChange, e_Execute,  e_Execute,  e_Execute,    e_Invalid  },
    /* Refreshing        */ { e_Invalid,  e_Queue,    e_Invalid,  e_NoChange, e_NoChange, e_Queue,      e_Invalid  },
    /* Restoring         */ { e_Invalid,  e_Queue,    e_Invalid,  e_NoChange, e_NoChange, e_Queue,      e_Invalid  },
    /* Unsubscribing     */ { e_Invalid,  e_Invalid,  e_Invalid,  e_Invalid,  e_NoChange, e_NoChange,   e_Invalid  },
    /* Unsubscribed      */ { e_Invalid,  e_Invalid,  e_Invalid,  e_Invalid,  e_Invalid,  e_NoChange,   e_NoChange }
  };

  PSafeLockReadWrite mutex(*this);
  if (!mutex.IsLocked())
    return true;

  switch (StateChangeActions[GetState()][newState]) {
    case e_Invalid :
      PTRACE(2, "SIP\tCannot change state to " << newState << " for " << GetMethod()
             << " handler while in " << GetState() << " state, target="
             << GetAddressOfRecord() << ", id=" << GetCallID());
      return false;

    case e_NoChange :
      PTRACE(4, "SIP\tAlready in state " << GetState() << " for " << GetMethod()
             << " handler, target=" << GetAddressOfRecord() << ", id=" << GetCallID());
      break;

    case e_Execute :
      PTRACE(4, "SIP\tExecuting state change to " << newState << " for " << GetMethod()
             << " handler, target=" << GetAddressOfRecord() << ", id=" << GetCallID());
      return SendRequest(newState);

    case e_Queue :
      PTRACE(3, "SIP\tQueueing state change to " << newState << " for " << GetMethod()
             << " handler while in " << GetState() << " state, target="
             << GetAddressOfRecord() << ", id=" << GetCallID());
      m_stateQueue.push(newState);
      break;
  }

  return true;
}


PBoolean SIPHandler::SendRequest(SIPHandler::State newState)
{
  m_expireTimer.Stop(false); // Stop automatic retry

  SetState(newState);

  SIP_PDU::StatusCodes reason = SIP_PDU::Local_BadTransportAddress;

  for (unsigned retry = 0; retry < 2; ++retry) {
    if (m_transport == NULL) {
      if (m_proxy.IsEmpty()) {
        // Look for a "proxy" parameter to override default proxy
        const PStringToString & params = m_remoteAddress.GetParamVars();
        if (params.Contains(OPAL_PROXY_PARAM)) {
          m_proxy.Parse(params(OPAL_PROXY_PARAM));
          m_remoteAddress.SetParamVar(OPAL_PROXY_PARAM, PString::Empty());
        }
      }

      SIPURL url;
      if (!m_proxy.IsEmpty())
        url = m_proxy;
      else {
        url = m_remoteAddress;
        url.AdjustToDNS();
      }

      // Must specify a network interface or get infinite recursion
      m_transport = GetEndPoint().CreateTransport(url, "*", &reason);
      if (m_transport == NULL)
        break;

      PTRACE_CONTEXT_ID_TO(m_transport);
    }

    if (m_transport->IsOpen()) {
      m_lastCseq = 0;

      // Restoring or first time, try every interface
      if (newState == Restoring || m_transport->GetInterface().IsEmpty()) {
        PWaitAndSignal mutex(m_transport->GetWriteMutex());
        if (m_transport->WriteConnect(WriteSIPHandler, this))
          return true;
      }
      else {
        // We contacted the server on an interface last time, assume it still works!
        if (WriteSIPHandler(*m_transport, false))
          return true;
      }

      reason = SIP_PDU::Local_TransportError;
      m_transport->CloseWait();
    }

    delete m_transport;
    m_transport = NULL;
  }

  OnFailed(reason);

  if (newState == Unsubscribing)
    SetState(Unsubscribed); // Transport level error, probably never going to get the unsubscribe through
  else
    RetryLater(m_offlineExpireTime);
  return true;
}


void SIPHandler::SetExpire(int e)
{
  m_currentExpireTime = e;
  PTRACE(3, "SIP\tExpiry time for " << GetMethod() << " set to " << e << " seconds.");

  // Only modify the m_originalExpireTime for future requests if IntervalTooBrief gives
  // a bigger expire time. expire itself will always reflect the proxy decision
  // (bigger or lower), but m_originalExpireTime determines what is used in future 
  // requests and is only modified if interval too brief
  if (m_originalExpireTime < e)
    m_originalExpireTime = e;

  // retry before the expire time.
  // if the expire time is more than 20 mins, retry 10mins before expiry
  // if the expire time is less than 20 mins, retry after half of the expiry time
  if (GetExpire() > 0 && GetState() < Unsubscribing)
    m_expireTimer.SetInterval(0, (unsigned)(GetExpire() < 20*60 ? GetExpire()/2 : GetExpire()-10*60));
}


PBoolean SIPHandler::WriteSIPHandler(OpalTransport & transport, void * param)
{
  return param != NULL && ((SIPHandler *)param)->WriteSIPHandler(transport, true);
}


bool SIPHandler::WriteSIPHandler(OpalTransport & transport, bool forked)
{
  SIPTransaction * transaction = CreateTransaction(transport);
  if (transaction == NULL) {
    PTRACE(2, "SIP\tCould not create transaction on " << transport);
    return false;
  }

  PTRACE_CONTEXT_ID_TO(transaction);

  SIPMIMEInfo & mime = transaction->GetMIME();

  if (forked) {
    if (m_lastCseq == 0)
      m_lastCseq = mime.GetCSeqIndex();
    else
      transaction->SetCSeq(m_lastCseq);
  }

  for (PStringToString::iterator it = m_mime.begin(); it != m_mime.end(); ++it)
    mime.SetAt(it->first, it->second);

  if (GetState() == Unsubscribing)
    mime.SetExpires(0);

  if (authentication != NULL) {
    SIPAuthenticator auth(*transaction);
    authentication->Authorise(auth); // If already have info from last time, use it!
  }

  if (transaction->Start()) {
    m_transactions.Append(transaction);
    return true;
  }

  PTRACE(2, "SIP\tDid not start transaction on " << transport);
  return false;
}


PBoolean SIPHandler::OnReceivedNOTIFY(SIP_PDU & /*response*/)
{
  return PFalse;
}


void SIPHandler::OnReceivedResponse(SIPTransaction & transaction, SIP_PDU & response)
{
  unsigned responseClass = response.GetStatusCode()/100;
  if (responseClass < 2)
    return; // Don't do anything with pending responses.

  // Received a response, so indicate that and collapse the forking on multiple interfaces.
  m_receivedResponse = true;

  m_transactions.Remove(&transaction); // Take this transaction out of list

  // And kill all the rest
  PSafePtr<SIPTransaction> transToGo;
  while ((transToGo = m_transactions.GetAt(0)) != NULL) {
    m_transactions.Remove(transToGo);
    transToGo->Abort();
  }

  // Finally end connect mode on the transport
  m_transport->SetInterface(transaction.GetInterface());

  switch (response.GetStatusCode()) {
    case SIP_PDU::Failure_UnAuthorised :
    case SIP_PDU::Failure_ProxyAuthenticationRequired :
      OnReceivedAuthenticationRequired(transaction, response);
      break;

    case SIP_PDU::Failure_IntervalTooBrief :
      OnReceivedIntervalTooBrief(transaction, response);
      break;

    case SIP_PDU::Failure_TemporarilyUnavailable:
      OnReceivedTemporarilyUnavailable(transaction, response);
      break;

    default :
      if (responseClass == 2)
        OnReceivedOK(transaction, response);
      else
        OnFailed(response);
  }
}


void SIPHandler::OnReceivedIntervalTooBrief(SIPTransaction & /*transaction*/, SIP_PDU & response)
{
  SetExpire(response.GetMIME().GetMinExpires());

  // Restart the transaction with new authentication handler
  SendRequest(GetState());
}


void SIPHandler::OnReceivedTemporarilyUnavailable(SIPTransaction & /*transaction*/, SIP_PDU & response)
{
  OnFailed(SIP_PDU::Failure_TemporarilyUnavailable);
  RetryLater(response.GetMIME().GetInteger("Retry-After", m_offlineExpireTime));
}


void SIPHandler::OnReceivedAuthenticationRequired(SIPTransaction & transaction, SIP_PDU & response)
{
  bool isProxy = response.GetStatusCode() == SIP_PDU::Failure_ProxyAuthenticationRequired;

#if PTRACING
  const char * proxyTrace = isProxy ? "Proxy " : "";
#endif
  PTRACE(3, "SIP\tReceived " << proxyTrace << "Authentication Required response");
  
  // authenticate 
  PString errorMsg;
  SIPAuthentication * newAuth = PHTTPClientAuthentication::ParseAuthenticationRequired(isProxy, response.GetMIME(), errorMsg);
  if (newAuth == NULL) {
    PTRACE(2, "SIP\t" << errorMsg);
    OnFailed(SIP_PDU::Failure_Forbidden);
    return;
  }

  // If either username or password blank, try and fine values from other
  // handlers which might be logged into the realm, or the proxy, if one.
  PString username = m_username;
  PString password = m_password;
  if (username.IsEmpty() || password.IsEmpty()) {
    // Try to find authentication parameters for the given realm,
    // if not, use the proxy authentication parameters (if any)
    if (GetEndPoint().GetAuthentication(newAuth->GetAuthRealm(), username, password)) {
      PTRACE (3, "SIP\tFound auth info for realm " << newAuth->GetAuthRealm());
    }
    else if (username.IsEmpty()) {
      if (m_proxy.IsEmpty()) {
        delete newAuth;
        PTRACE(2, "SIP\tAuthentication not possible yet, no credentials available.");
        OnFailed(SIP_PDU::Failure_TemporarilyUnavailable);
        if (!transaction.IsCanceled())
          RetryLater(m_offlineExpireTime);
        return;
      }

      PTRACE (3, "SIP\tNo auth info for realm " << newAuth->GetAuthRealm() << ", using proxy auth");
      username = m_proxy.GetUserName();
      password = m_proxy.GetPassword();
    }
  }

  newAuth->SetUsername(username);
  newAuth->SetPassword(password);

  // check to see if this is a follow-on from the last authentication scheme used
  if (GetState() == Subscribing && authentication != NULL && *newAuth == *authentication) {
    delete newAuth;
    PTRACE(1, "SIP\tAuthentication already performed using current credentials, not trying again.");
    OnFailed(SIP_PDU::Failure_UnAuthorised);
    return;
  }

  PTRACE(4, "SIP\tUpdating authentication credentials.");

  // switch authentication schemes
  delete authentication;
  authentication = newAuth;
  m_username = username;
  m_password = password;

  // If we changed realm (or hadn't got one yet) update the handler database
  if (m_realm != newAuth->GetAuthRealm()) {
    m_realm = newAuth->GetAuthRealm();
    PTRACE(3, "SIP\tAuth realm set to " << m_realm);
    GetEndPoint().UpdateHandlerIndexes(this);
  }

  // Restart the transaction with new authentication handler
  SendRequest(GetState());
}


void SIPHandler::OnReceivedOK(SIPTransaction & /*transaction*/, SIP_PDU & response)
{
  response.GetMIME().GetProductInfo(m_productInfo);

  switch (GetState()) {
    case Unsubscribing :
      SetState(Unsubscribed);
      break;

    case Subscribing :
    case Refreshing :
    case Restoring :
      if (GetExpire() == 0)
        SetState(Unsubscribed);
      else
        SetState(Subscribed);
      break;

    default :
      PTRACE(2, "SIP\tUnexpected 200 OK in handler with state " << GetState());
  }
}


void SIPHandler::OnTransactionFailed(SIPTransaction & transaction)
{
  if (m_transactions.Remove(&transaction)) {
    OnFailed(transaction.GetStatusCode());
    if (!transaction.IsCanceled())
      RetryLater(m_offlineExpireTime);
  }
}


void SIPHandler::RetryLater(unsigned after)
{
  if (after == 0 || GetExpire() == 0)
    return;

  PTRACE(3, "SIP\tRetrying " << GetMethod() << " after " << after << " seconds.");
  m_expireTimer.SetInterval(0, after); // Keep trying to get it back
}


void SIPHandler::OnFailed(const SIP_PDU & response)
{
  OnFailed(response.GetStatusCode());
}


void SIPHandler::OnFailed(SIP_PDU::StatusCodes code)
{
  switch (code) {
    case SIP_PDU::Local_TransportError :
    case SIP_PDU::Local_Timeout :
    case SIP_PDU::Failure_RequestTimeout :
    case SIP_PDU::Local_BadTransportAddress :
    case SIP_PDU::Failure_TemporarilyUnavailable:
      if (GetState() != Unsubscribing) {
        SetState(Unavailable);
        break;
      }
      // Do next case to finalise Unsubscribe even though there was an error

    default :
      PTRACE(4, "SIP\tNot retrying " << GetMethod() << " due to error response " << code);
      m_currentExpireTime = 0; // OK, stop trying
      m_expireTimer.Stop(false);
      SetState(Unsubscribed); // Allow garbage collection thread to clean up
  }
}


void SIPHandler::OnExpireTimeout(PTimer &, INT)
{
  PSafeLockReadWrite lock(*this);
  if (!lock.IsLocked())
    return;

  switch (GetState()) {
    case Subscribed :
      PTRACE(2, "SIP\tStarting " << GetMethod() << " for binding refresh");
      if (SendRequest(Refreshing))
        return;
      break;

    case Unavailable :
      PTRACE(2, "SIP\tStarting " << GetMethod() << " for offline retry");
      if (SendRequest(Restoring))
        return;
      break;

    default :
      return;
  }

  SetState(Unavailable);
}


///////////////////////////////////////////////////////////////////////////////

SIPRegisterHandler::SIPRegisterHandler(SIPEndPoint & endpoint, const SIPRegister::Params & params)
  : SIPHandler(SIP_PDU::Method_REGISTER, endpoint, params)
  , m_parameters(params)
  , m_sequenceNumber(0)
{
  // Foer some bizarre reason, even though REGISTER does not create a dialog,
  // some registrars insist that you have a from tag ...
  SIPURL local = params.m_localAddress.IsEmpty() ? params.m_addressOfRecord : params.m_localAddress;
  local.SetTag();
  m_parameters.m_localAddress = local.AsQuotedString();
  m_parameters.m_proxyAddress = m_proxy.AsString();
}


SIPTransaction * SIPRegisterHandler::CreateTransaction(OpalTransport & trans)
{
  SIPRegister::Params params = m_parameters;

  if (GetState() == Unsubscribing) {
    params.m_expire = 0;

    if (params.m_contactAddress.IsEmpty()) {
      if (m_contactAddresses.empty())
        params.m_contactAddress = "*";
      else {
        for (SIPURLList::iterator contact = m_contactAddresses.begin(); contact != m_contactAddresses.end(); ++contact)
          contact->GetFieldParameters().Remove("expires");
        params.m_contactAddress = m_contactAddresses.ToString();
      }
    }
  }
  else {
    params.m_expire = GetExpire();

    if (params.m_contactAddress.IsEmpty()) {
      if (GetState() != Refreshing || m_contactAddresses.empty()) {
        PString userName = SIPURL(params.m_addressOfRecord).GetUserName();
        OpalTransportAddressArray interfaces = GetEndPoint().GetInterfaceAddresses(true, &trans);
        if (params.m_compatibility == SIPRegister::e_CannotRegisterMultipleContacts) {
          // If translated by STUN then only the external address of the interface is used.
          SIPURL contact(userName, interfaces[0]);
          contact.Sanitise(SIPURL::RegContactURI);
          params.m_contactAddress += contact.AsQuotedString();
        }
        else {
          OpalTransportAddress localAddress = trans.GetLocalAddress(params.m_compatibility != SIPRegister::e_HasApplicationLayerGateway);
          unsigned qvalue = 1000;
          for (PINDEX i = 0; i < interfaces.GetSize(); ++i) {
            /* If fully compliant, put into the contact field all the bound
               interfaces. If special then we only put into the contact
               listeners that are on the same interface. If translated by STUN
               then only the external address of the interface is used. */
            if (params.m_compatibility == SIPRegister::e_FullyCompliant || localAddress.IsEquivalent(interfaces[i], true)) {
              SIPURL contact(userName, interfaces[i]);
              contact.Sanitise(SIPURL::RegContactURI);
              contact.GetFieldParameters().Set("q", qvalue < 1000 ? psprintf("0.%03u", qvalue) : "1");

              if (!params.m_contactAddress.IsEmpty())
                params.m_contactAddress += ", ";
              params.m_contactAddress += contact.AsQuotedString();

              qvalue -= 1000/interfaces.GetSize();
            }
          }
        }
      }
      else
        params.m_contactAddress = m_contactAddresses.ToString();
    }
  }

  return new SIPRegister(GetEndPoint(), trans, GetCallID(), m_sequenceNumber, params);
}


void SIPRegisterHandler::OnReceivedOK(SIPTransaction & transaction, SIP_PDU & response)
{
  State previousState = GetState();
  switch (previousState) {
    case Unsubscribing :
      SetState(Unsubscribed);
      SendStatus(SIP_PDU::Successful_OK, Unsubscribing);
      return;

    case Subscribing :
    case Refreshing :
    case Restoring :
      break;

    default :
      PTRACE(2, "SIP\tUnexpected 200 OK in handler with state " << previousState);
      return;
  }

  SIPMIMEInfo & mime = response.GetMIME();

  mime.GetProductInfo(m_productInfo);

  m_serviceRoute.FromString(mime("Service-Route"));

  SIPURLList requestedContacts;
  transaction.GetMIME().GetContacts(requestedContacts);

  SIPURLList replyContacts;
  mime.GetContacts(replyContacts);

  /* See if we are behind NAT and the Via header rport was present. This is
     a STUN free mechanism for working behind firewalls. Also, some servers
     will refuse to work unless the Contact header agrees whith where the
     packet is physically coming from. */
  OpalTransportAddress externalAddress;
  if (m_parameters.m_compatibility != SIPRegister::e_HasApplicationLayerGateway)
    externalAddress = mime.GetViaReceivedAddress();

  /* RFC3261 does say the registrar must send back ALL registrations, however
     we only deal with replies from the registrar that we actually requested.

     Except! We look for special condition where registrar (e.g. OpenSIPS)
     tries to be smart and adjusts the Contact field to be the external
     address as per the Via header rport. IMHO this is a bug as it does not follow
     what is implied by RFC3261/10.2.4 and 10.3 step 7. It should include the
     original as well as the extra Contact. But it doesn't, so we have to deal.
     */
  for (SIPURLList::iterator reply = replyContacts.begin(); reply != replyContacts.end(); ) {
    if (reply->GetHostAddress() == externalAddress) {
      externalAddress.MakeEmpty(); // Clear this so no further action taken
      ++reply;
    }
    else if (std::find(requestedContacts.begin(), requestedContacts.end(), *reply) != requestedContacts.end())
      ++reply;
    else
      replyContacts.erase(reply++);
  }

  if (replyContacts.empty() && GetExpire() != 0) {
    // Huh? Nothing we tried to register, registered! How is that possible?
    PTRACE(2, "SIP\tREGISTER returned no Contact addresses we requested, not really registered.");
    SendRequest(Unsubscribing);
    SendStatus(SIP_PDU::GlobalFailure_NotAcceptable, previousState);
    return;
  }

  // If this is the final (possibly one and only) REGISTER, process it
  if (m_externalAddress == externalAddress) {
    int minExpiry = INT_MAX;
    for (SIPURLList::iterator contact = replyContacts.begin(); contact != replyContacts.end(); ++contact) {
      long expires = contact->GetFieldParameters().GetInteger("expires",
                          mime.GetExpires(GetEndPoint().GetRegistrarTimeToLive().GetSeconds()));
      if (expires > 0 && minExpiry > expires)
        minExpiry = expires;
    }
    SetExpire(minExpiry);

    m_contactAddresses = replyContacts;
    SetState(Subscribed);
    SendStatus(SIP_PDU::Successful_OK, previousState);
    return;
  }

  // Remember (possibly new) NAT address
  m_externalAddress == externalAddress;

  if (GetExpire() == 0) {
    // If we had discovered we are behind NAT and had unregistered, re-REGISTER with new addresses
    PTRACE(2, "SIP\tRe-registering with NAT address " << externalAddress);
    for (SIPURLList::iterator contact = m_contactAddresses.begin(); contact != m_contactAddresses.end(); ++contact)
      contact->SetHostAddress(externalAddress);
    SetExpire(m_originalExpireTime);
  }
  else {
    /* If we got here then we have done a successful register, but registrar indicated
       that we are behind firewall. Unregister what we just registered */
    PTRACE(2, "SIP\tRemote indicated change of REGISTER Contact header required due to NAT");
    for (SIPURLList::iterator contact = replyContacts.begin(); contact != replyContacts.end(); ++contact)
      contact->GetFieldParameters().Remove("expires");
    m_contactAddresses = replyContacts;
    SetExpire(0);
  }

  SendRequest(previousState);
  SendStatus(SIP_PDU::Information_Trying, previousState);
}


void SIPRegisterHandler::OnFailed(SIP_PDU::StatusCodes r)
{
  SendStatus(r, GetState());
  SIPHandler::OnFailed(r);
}


PBoolean SIPRegisterHandler::SendRequest(SIPHandler::State s)
{
  SendStatus(SIP_PDU::Information_Trying, s);
  m_sequenceNumber = GetEndPoint().GetNextCSeq();
  return SIPHandler::SendRequest(s);
}


void SIPRegisterHandler::SendStatus(SIP_PDU::StatusCodes code, State state)
{
  SIPEndPoint::RegistrationStatus status;
  status.m_handler = this;
  status.m_addressofRecord = GetAddressOfRecord().AsString();
  status.m_productInfo = m_productInfo;
  status.m_reason = code;
  status.m_userData = m_parameters.m_userData;

  switch (state) {
    case Subscribing :
      status.m_wasRegistering = true;
      status.m_reRegistering = false;
      break;

    case Subscribed :
    case Refreshing :
      status.m_wasRegistering = true;
      status.m_reRegistering = true;
      break;

    case Unsubscribed :
    case Unavailable :
    case Restoring :
      status.m_wasRegistering = true;
      status.m_reRegistering = code/100 != 2;
      break;

    case Unsubscribing :
      status.m_wasRegistering = false;
      status.m_reRegistering = false;
      break;

    default :
      PAssertAlways(PInvalidParameter);
  }

  GetEndPoint().OnRegistrationStatus(status);
}


void SIPRegisterHandler::UpdateParameters(const SIPRegister::Params & params)
{
  if (!params.m_authID.IsEmpty())
    m_username = m_parameters.m_authID = params.m_authID;   // Adjust the authUser if required 
  if (!params.m_realm.IsEmpty())
    m_realm = m_parameters.m_realm = params.m_realm;   // Adjust the realm if required 
  if (!params.m_password.IsEmpty())
    m_password = m_parameters.m_password = params.m_password; // Adjust the password if required 

  if (params.m_expire > 0)
    SetExpire(m_parameters.m_expire = params.m_expire);

  m_parameters.m_contactAddress = params.m_contactAddress;
  m_contactAddresses.clear();
}


/////////////////////////////////////////////////////////////////////////

SIPSubscribeHandler::SIPSubscribeHandler(SIPEndPoint & endpoint, const SIPSubscribe::Params & params)
  : SIPHandler(SIP_PDU::Method_SUBSCRIBE, endpoint, params)
  , m_parameters(params)
  , m_unconfirmed(true)
  , m_packageHandler(SIPEventPackageFactory::CreateInstance(params.m_eventPackage))
  , m_previousResponse(NULL)
{
  PTRACE_CONTEXT_ID_TO(m_packageHandler);

  m_dialog.SetCallID(GetCallID());

  m_parameters.m_proxyAddress = m_proxy.AsString();

  if (m_parameters.m_contentType.IsEmpty() && (m_packageHandler != NULL))
    m_parameters.m_contentType = m_packageHandler->GetContentType();
}


SIPSubscribeHandler::~SIPSubscribeHandler()
{
  delete m_packageHandler;
  delete m_previousResponse;
}


SIPTransaction * SIPSubscribeHandler::CreateTransaction(OpalTransport & trans)
{ 
  // Do all the following here as must be after we have called GetTransport()
  // which sets various fields correctly for transmission
  if (!m_dialog.IsEstablished()) {
    m_dialog.SetRequestURI(GetAddressOfRecord());
    if (m_parameters.m_eventPackage.IsWatcher())
      m_parameters.m_localAddress = GetAddressOfRecord().AsString();

    m_dialog.SetRemoteURI(m_parameters.m_addressOfRecord);

    if (m_parameters.m_localAddress.IsEmpty())
      m_dialog.SetLocalURI(GetEndPoint().GetRegisteredPartyName(m_parameters.m_addressOfRecord, *m_transport));
    else
      m_dialog.SetLocalURI(m_parameters.m_localAddress);

    m_dialog.SetProxy(m_proxy, true);
  }

  m_parameters.m_expire = GetState() != Unsubscribing ? GetExpire() : 0;
  return new SIPSubscribe(GetEndPoint(), trans, m_dialog, m_parameters);
}


void SIPSubscribeHandler::OnFailed(const SIP_PDU & response)
{
  SIP_PDU::StatusCodes responseCode = response.GetStatusCode();

  SendStatus(responseCode, GetState());

  if (GetState() != Unsubscribing && responseCode == SIP_PDU::Failure_TransactionDoesNotExist) {
    // Resubscribe as previous subscription totally lost, but dialog processing
    // may have altered the target so restore the original target address
    m_parameters.m_addressOfRecord = GetAddressOfRecord().AsString();
    PString dummy;
    GetEndPoint().Subscribe(m_parameters, dummy);
  }

  SIPHandler::OnFailed(responseCode);
}


PBoolean SIPSubscribeHandler::SendRequest(SIPHandler::State s)
{
  SendStatus(SIP_PDU::Information_Trying, s);
  return SIPHandler::SendRequest(s);
}


bool SIPSubscribeHandler::WriteSIPHandler(OpalTransport & transport, bool forked)
{
  m_dialog.SetForking(forked);
  bool ok = SIPHandler::WriteSIPHandler(transport, false);
  m_dialog.SetForking(false);
  return ok;
}


void SIPSubscribeHandler::SendStatus(SIP_PDU::StatusCodes code, State state)
{
  SIPEndPoint::SubscriptionStatus status;
  status.m_handler = this;
  status.m_addressofRecord = GetAddressOfRecord().AsString();
  status.m_productInfo = m_productInfo;
  status.m_reason = code;
  status.m_userData = m_parameters.m_userData;

  switch (state) {
    case Subscribing :
      status.m_wasSubscribing = true;
      status.m_reSubscribing = false;
      break;

    case Subscribed :
      if (m_unconfirmed) {
        status.m_wasSubscribing = true;
        status.m_reSubscribing = false;
        GetEndPoint().OnSubscriptionStatus(status);
      }
      // Do next state

    case Refreshing :
      status.m_wasSubscribing = true;
      status.m_reSubscribing = true;
      break;

    case Unsubscribed :
    case Unavailable :
    case Restoring :
      status.m_wasSubscribing = true;
      status.m_reSubscribing = code/100 != 2;
      break;

    case Unsubscribing :
      status.m_wasSubscribing = false;
      status.m_reSubscribing = false;
      break;

    default :
      PAssertAlways(PInvalidParameter);
  }

  if (!m_parameters.m_onSubcribeStatus.IsNULL()) 
    m_parameters.m_onSubcribeStatus(*this, status);

  GetEndPoint().OnSubscriptionStatus(status);
}


void SIPSubscribeHandler::UpdateParameters(const SIPSubscribe::Params & params)
{
  if (!params.m_authID.IsEmpty())
    m_username = params.m_authID;   // Adjust the authUser if required 
  if (!params.m_realm.IsEmpty())
    m_realm = params.m_realm;   // Adjust the realm if required 
  if (!params.m_password.IsEmpty())
    m_password = params.m_password; // Adjust the password if required 

  m_parameters.m_contactAddress = params.m_contactAddress;
  m_parameters.m_onSubcribeStatus = params.m_onSubcribeStatus;
  m_parameters.m_onNotify = params.m_onNotify;

  if (params.m_expire > 0)
    SetExpire(params.m_expire);
}


void SIPSubscribeHandler::OnReceivedOK(SIPTransaction & transaction, SIP_PDU & response)
{
  /* An "expire" parameter in the Contact header has no semantics
   * for SUBSCRIBE. RFC3265, 3.1.1.
   * An answer can only shorten the expires time.
   */
  SetExpire(response.GetMIME().GetExpires(m_originalExpireTime));
  m_dialog.Update(*m_transport, response);

  if (GetState() != Unsubscribing)
    SIPHandler::OnReceivedOK(transaction, response);
}


PBoolean SIPSubscribeHandler::OnReceivedNOTIFY(SIP_PDU & request)
{
  if (PAssertNULL(m_transport) == NULL)
    return false;

  if (m_unconfirmed) {
    SendStatus(SIP_PDU::Successful_OK, GetState());
    m_unconfirmed = false;
  }

  SIPMIMEInfo & requestMIME = request.GetMIME();

  requestMIME.GetProductInfo(m_productInfo);

  // If we received this NOTIFY before, send the previous response
  if (m_dialog.IsDuplicateCSeq(requestMIME.GetCSeqIndex())) {

    // duplicate CSEQ but no previous response? That's unpossible!
    if (m_previousResponse == NULL)
      return request.SendResponse(*m_transport, SIP_PDU::Failure_InternalServerError, m_endpoint);

    /* Make sure our repeat response has the CSeq/Via etc of this request, due
       to retries at various protocol layers we can have old and even older
       NOTIFY packets still coming in requiring matching responses. */
    m_previousResponse->InitialiseHeaders(request);
    return request.SendResponse(*m_transport, *m_previousResponse, m_endpoint);
  }

  // remove last response
  delete m_previousResponse;
  m_previousResponse = new SIP_PDU(request, SIP_PDU::Failure_BadRequest);
  PTRACE_CONTEXT_ID_TO(m_previousResponse);

  PStringToString subscriptionStateInfo;
  PCaselessString subscriptionState = requestMIME.GetSubscriptionState(subscriptionStateInfo);
  if (subscriptionState.IsEmpty()) {
    PTRACE(2, "SIP\tNOTIFY received without Subscription-State");
    m_previousResponse->SetInfo("No Subscription-State field");
    return request.SendResponse(*m_transport, *m_previousResponse, m_endpoint);
  }

  // Check the susbscription state
  if (subscriptionState == "terminated") {
    PTRACE(3, "SIP\tSubscription is terminated, state=" << GetState());
    m_previousResponse->SetStatusCode(SIP_PDU::Successful_OK);
    request.SendResponse(*m_transport, *m_previousResponse, m_endpoint);

    switch (GetState()) {
      case Unsubscribed :
        break;

      case Subscribed :
      case Unavailable :
        SendRequest(Unsubscribing);
        break;

      default :
        SetState(Unsubscribed); // Allow garbage collection thread to clean up
        SendStatus(SIP_PDU::Successful_OK, Unsubscribing);
        break;
    }

    return true;
  }

  PString requestEvent = requestMIME.GetEvent();
  if (m_parameters.m_eventPackage != requestEvent) {
    PTRACE(2, "SIP\tNOTIFY received for incorrect event \"" << requestEvent << "\", requires \"" << m_parameters.m_eventPackage << '"');
    m_previousResponse->SetStatusCode(SIP_PDU::Failure_BadEvent);
    m_previousResponse->GetMIME().SetAt("Allow-Events", m_parameters.m_eventPackage);
    return request.SendResponse(*m_transport, *m_previousResponse, m_endpoint);
  }

  // Check any Requires field
  PStringSet require = requestMIME.GetRequire();
  if (m_parameters.m_eventList)
    require -= "eventlist";
  if (!require.IsEmpty()) {
    PTRACE(2, "SIPPres\tNOTIFY contains unsupported Require field \"" << setfill(',') << require << '"');
    m_previousResponse->SetStatusCode(SIP_PDU::Failure_BadExtension);
    m_previousResponse->GetMIME().SetUnsupported(require);
    m_previousResponse->SetInfo("Unsupported Require");
    return request.SendResponse(*m_transport, *m_previousResponse, m_endpoint);
  }

  // check the ContentType
  if (!m_parameters.m_contentType.IsEmpty()) {
    PCaselessString requestContentType = requestMIME.GetContentType();
    if (
        (m_parameters.m_contentType.Find(requestContentType) == P_MAX_INDEX) && 
        !(m_parameters.m_eventList && (requestContentType == "multipart/related")) &&
        !((m_packageHandler != NULL) && m_packageHandler->ValidateContentType(requestContentType, requestMIME))
        ) {
      PTRACE(2, "SIPPres\tNOTIFY contains unsupported Content-Type \""
             << requestContentType << "\", expecting \"" << m_parameters.m_contentType << "\" or multipart/related or other validated type");
      m_previousResponse->SetStatusCode(SIP_PDU::Failure_UnsupportedMediaType);
      m_previousResponse->GetMIME().SetAt("Accept", m_parameters.m_contentType);
      m_previousResponse->SetInfo("Unsupported Content-Type");
      return request.SendResponse(*m_transport, *m_previousResponse, m_endpoint);
    }
  }

  // Check if we know how to deal with this event
  if (m_packageHandler == NULL && m_parameters.m_onNotify.IsNULL()) {
    PTRACE(2, "SIP\tNo handler for NOTIFY received for event \"" << requestEvent << '"');
    m_previousResponse->SetStatusCode(SIP_PDU::Failure_InternalServerError);
    return request.SendResponse(*m_transport, *m_previousResponse, m_endpoint);
  }

  // Adjust timeouts if state is a go
  if (subscriptionState == "active" || subscriptionState == "pending") {
    PTRACE(3, "SIP\tSubscription is " << GetState());
    PString expire = SIPMIMEInfo::ExtractFieldParameter(GetState(), "expire");
    if (!expire.IsEmpty())
      SetExpire(expire.AsUnsigned());
  }

  bool sendResponse = true;

  // Check for empty body, if so then is OK, just a ping ...
  if (request.GetEntityBody().IsEmpty())
    m_previousResponse->SetStatusCode(SIP_PDU::Successful_OK);
  else {
    PMultiPartList parts;
    if (!m_parameters.m_eventList || !requestMIME.DecodeMultiPartList(parts, request.GetEntityBody()))
      sendResponse = DispatchNOTIFY(request);
    else {
      // If GetMultiParts() returns true there as at least one part and that
      // part must be the meta list, guranteed by DecodeMultiPartList()
      PMultiPartList::iterator iter = parts.begin();

      // First part is always Meta Information
      if (iter->m_mime.GetString(PMIMEInfo::ContentTypeTag) != "application/rlmi+xml") {
        PTRACE(2, "SIP\tNOTIFY received without RLMI as first multipart body");
        m_previousResponse->SetInfo("No Resource List Meta-Information");
        return request.SendResponse(*m_transport, *m_previousResponse, m_endpoint);
      }

#if P_EXPAT
      PXML xml;
      if (!xml.Load(iter->m_textBody)) {
        PTRACE(2, "SIP\tNOTIFY received with illegal RLMI\n"
                  "Line " << xml.GetErrorLine() << ", Column " << xml.GetErrorColumn() << ": " << xml.GetErrorString());
        m_previousResponse->SetInfo("Bad Resource List Meta-Information");
        return request.SendResponse(*m_transport, *m_previousResponse, m_endpoint);
      }

      if (parts.GetSize() == 1)
        m_previousResponse->SetStatusCode(SIP_PDU::Successful_OK);
      else {
        while (++iter != parts.end()) {
          SIP_PDU pdu(request.GetMethod());
          SIPMIMEInfo & pduMIME = pdu.GetMIME();

          pduMIME.AddMIME(iter->m_mime);
          pdu.SetEntityBody(iter->m_textBody);

          PStringToString cid;
          if (iter->m_mime.GetComplex(PMIMEInfo::ContentIdTag, cid)) {
            PINDEX index = 0;
            PXMLElement * resource;
            while ((resource = xml.GetElement("resource", index++)) != NULL) {
              SIPURL uri = resource->GetAttribute("uri");
              if (!uri.IsEmpty()) {
                PXMLElement * instance = resource->GetElement("instance");
                if (instance != NULL && instance->GetAttribute("cid") == cid[PString::Empty()]) {
                  pduMIME.SetSubscriptionState(instance->GetAttribute("state"));
                  PXMLElement * name = resource->GetElement("name");
                  if (name != NULL)
                    uri.SetDisplayName(name->GetData());
                  pduMIME.SetFrom(uri.AsQuotedString());
                  pduMIME.SetTo(uri.AsQuotedString());
                  break;
                }
              }
            }
          }

          if (DispatchNOTIFY(pdu))
            sendResponse = false;
        }
      }
#else
      for ( ; iter != parts.end(); ++iter) {
        SIP_PDU pdu(request.GetMethod());
        pdu.GetMIME().AddMIME(iter->m_mime);
        pdu.SetEntityBody(iter->m_textBody);

        if (DispatchNOTIFY(pdu))
          sendResponse = false;
      }
#endif
    }
  }

  if (sendResponse)
    request.SendResponse(*m_transport, *m_previousResponse, m_endpoint);
  return true;
}


bool SIPSubscribeHandler::DispatchNOTIFY(SIP_PDU & request)
{
  SIPSubscribe::NotifyCallbackInfo notifyInfo(*this, GetEndPoint(), *m_transport, request, *m_previousResponse);

  if (!m_parameters.m_onNotify.IsNULL()) {
    PTRACE(4, "SIP\tCalling NOTIFY callback for AOR \"" << m_addressOfRecord << "\"");
    m_parameters.m_onNotify(*this, notifyInfo);
    return notifyInfo.m_sendResponse;
  }

  if (m_packageHandler != NULL) {
    PTRACE(4, "SIP\tCalling package NOTIFY handler for AOR \"" << m_addressOfRecord << "\"");
    m_packageHandler->OnReceivedNOTIFY(notifyInfo);
    return notifyInfo.m_sendResponse;
  }

  PTRACE(2, "SIP\tNo NOTIFY handler for AOR \"" << m_addressOfRecord << "\"");
  m_previousResponse->SetStatusCode(SIP_PDU::Failure_BadEvent);
  return true;
}


///////////////////////////////////////

bool SIPEventPackageHandler::ValidateContentType(const PString & type, const SIPMIMEInfo & mime) 
{ 
  return type.IsEmpty() && (mime.GetContentLength() == 0);
}


bool SIPEventPackageHandler::ValidateNotificationSequence(SIPSubscribeHandler & handler, unsigned newSequenceNumber, bool fullUpdate)
{
  if (m_expectedSequenceNumber != UINT_MAX && newSequenceNumber < m_expectedSequenceNumber) {
    PTRACE(3, "SIP\tReceived repeated NOTIFY of " << handler.GetAddressOfRecord() << ", already processed.");
    return false;
  }

  // if this is a full list of watcher info, we can empty out our pending lists
  if (fullUpdate) {
    PTRACE(3, "SIP\tReceived full update NOTIFY of " << handler.GetAddressOfRecord());
    m_expectedSequenceNumber = newSequenceNumber+1;
    return true;
  }

  if (m_expectedSequenceNumber != UINT_MAX) {
    PTRACE(2, "SIPPres\tReceived partial update NOTIFY of " << handler.GetAddressOfRecord() << ", but still awaiting full update.");
    return false;
  }

  if (newSequenceNumber == m_expectedSequenceNumber) {
    PTRACE(3, "SIP\tReceived partial update NOTIFY of " << handler.GetAddressOfRecord());
    ++m_expectedSequenceNumber;
    return true;
  }

  PTRACE(2, "SIP\tReceived out of sequence partial update, resubscribing " << handler.GetAddressOfRecord());
  m_expectedSequenceNumber = UINT_MAX;
  handler.ActivateState(SIPHandler::Refreshing);
  return false;
}


///////////////////////////////////////

class SIPMwiEventPackageHandler : public SIPEventPackageHandler
{
  virtual PCaselessString GetContentType() const
  {
    return "application/simple-message-summary";
  }

  virtual void OnReceivedNOTIFY(SIPSubscribe::NotifyCallbackInfo & notifyInfo)
  {
    notifyInfo.SendResponse(); // Send OK even of we don;t understand the below

    // Extract the string describing the number of new messages
    static struct {
      const char * name;
      OpalManager::MessageWaitingType type;
    } const validMessageClasses[] = {
      { "voice-message",      OpalManager::VoiceMessageWaiting      },
      { "fax-message",        OpalManager::FaxMessageWaiting        },
      { "pager-message",      OpalManager::PagerMessageWaiting      },
      { "multimedia-message", OpalManager::MultimediaMessageWaiting },
      { "text-message",       OpalManager::TextMessageWaiting       },
      { "none",               OpalManager::NoMessageWaiting         }
    };

    PMIMEInfo info(notifyInfo.m_request.GetEntityBody());

    PString account = info.Get("Message-Account", notifyInfo.m_handler.GetAddressOfRecord().AsString());

    bool nothingSent = true;
    for (PINDEX z = 0 ; z < PARRAYSIZE(validMessageClasses); z++) {
      if (info.Contains(validMessageClasses[z].name)) {
        notifyInfo.m_endpoint.OnMWIReceived(account, validMessageClasses[z].type, info[validMessageClasses[z].name]);
        nothingSent = false;
      }
    }

    // Received MWI, but not counts, must be old form. Also make sure we send a
    // lower case yes/no to call back function, regardless of what comes down the wire.
    if (nothingSent)
      notifyInfo.m_endpoint.OnMWIReceived(account, OpalManager::NumMessageWaitingTypes,
                                 (info.Get("Message-Waiting") *= "yes") ? "yes" : "no");
  }
};

static SIPEventPackageFactory::Worker<SIPMwiEventPackageHandler> mwiEventPackageHandler(SIPSubscribe::MessageSummary);


///////////////////////////////////////////////////////////////////////////////

// This package is on for backward compatibility, presence should now use the
// the OpalPresence classes to manage SIP presence.
class SIPPresenceEventPackageHandler : public SIPEventPackageHandler
{
  virtual PCaselessString GetContentType() const
  {
    return "application/pidf+xml";
  }

  virtual void OnReceivedNOTIFY(SIPSubscribe::NotifyCallbackInfo & notifyInfo)
  {
    PTRACE(4, "SIP\tProcessing presence NOTIFY using old API");

    list<SIPPresenceInfo> infoList;
    if (!SIPPresenceInfo::ParseNotify(notifyInfo, infoList))
      return;

    notifyInfo.SendResponse();

    // support old API 
    SIPURL from = notifyInfo.m_request.GetMIME().GetFrom();
    from.Sanitise(SIPURL::ExternalURI);

    SIPURL to = notifyInfo.m_request.GetMIME().GetTo();
    to.Sanitise(SIPURL::ExternalURI);

    for (list<SIPPresenceInfo>::iterator it = infoList.begin(); it != infoList.end(); ++it) {
      it->m_entity = from; // Really should not do this, but put in for backward compatibility
      it->m_target = to;
      notifyInfo.m_endpoint.OnPresenceInfoReceived(*it);
    }
  }
};

static SIPEventPackageFactory::Worker<SIPPresenceEventPackageHandler> presenceEventPackageHandler(SIPSubscribe::Presence);


///////////////////////////////////////////////////////////////////////////////

#if P_EXPAT

static void ParseParticipant(PXMLElement * participantElement, SIPDialogNotification::Participant & participant)
{
  if (participantElement == NULL)
    return;

  PXMLElement * identityElement = participantElement->GetElement("identity");
  if (identityElement != NULL) {
    participant.m_identity = identityElement->GetData();
    participant.m_display = identityElement->GetAttribute("display");
  }

  PXMLElement * targetElement = participantElement->GetElement("target");
  if (targetElement == NULL)
    return;

  participant.m_URI = targetElement->GetAttribute("uri");

  PXMLElement * paramElement;
  PINDEX i = 0;
  while ((paramElement = targetElement->GetElement("param", i++)) != NULL) {
    PCaselessString name = paramElement->GetAttribute("pname");
    PCaselessString value = paramElement->GetAttribute("pvalue");
    if (name == "appearance" || // draft-anil-sipping-bla-04 version
        name == "x-line-id")    // draft-anil-sipping-bla-03 version
      participant.m_appearance = value.AsUnsigned();
    else if (name == "sip.byeless" || name == "+sip.byeless")
      participant.m_byeless = value == "true";
    else if (name == "sip.rendering" || name == "+sip.rendering") {
      if (value == "yes")
        participant.m_rendering = SIPDialogNotification::RenderingMedia;
      else if (value == "no")
        participant.m_rendering = SIPDialogNotification::NotRenderingMedia;
      else
        participant.m_rendering = SIPDialogNotification::RenderingUnknown;
    }
  }
}

#endif // P_EXPAT


class SIPDialogEventPackageHandler : public SIPEventPackageHandler
{
public:
  SIPDialogEventPackageHandler()
    : m_dialogNotifyVersion(1)
  {
  }

  virtual PCaselessString GetContentType() const
  {
    return "application/dialog-info+xml";
  }

  virtual void OnReceivedNOTIFY(SIPSubscribe::NotifyCallbackInfo & notifyInfo)
  {
    PINDEX index = 0;

#if P_EXPAT
    static PXML::ValidationInfo const DialogInfoValidation[] = {
      { PXML::SetDefaultNamespace,        "urn:ietf:params:xml:ns:dialog-info" },
      { PXML::ElementName,                "dialog-info", },
      { PXML::RequiredNonEmptyAttribute,  "entity"},
      { PXML::RequiredNonEmptyAttribute,  "version"},
      { PXML::RequiredAttributeWithValue, "state",   { "full\npartial" } },

      { PXML::EndOfValidationList }
    };

    PXML xml;
    if (!notifyInfo.LoadAndValidate(xml, DialogInfoValidation))
      return;

    // Enough is OK to send OK now
    notifyInfo.SendResponse();

    PXMLElement * rootElement = xml.GetRootElement();
    if (!ValidateNotificationSequence(notifyInfo.m_handler,
                                      rootElement->GetAttribute("version").AsUnsigned(),
                                      rootElement->GetAttribute("state") *= "full"))
      return;

    SIPDialogNotification info(rootElement->GetAttribute("entity"));

    PXMLElement * dialogElement;
    while ((dialogElement = rootElement->GetElement("dialog", index)) != NULL) {
      info.m_callId = dialogElement->GetAttribute("call-id");
      info.m_local.m_dialogTag = dialogElement->GetAttribute("local-tag");
      info.m_remote.m_dialogTag = dialogElement->GetAttribute("remote-tag");

      PXMLElement * stateElement = dialogElement->GetElement("state");
      if (stateElement == NULL)
        info.m_state = SIPDialogNotification::Terminated;
      else {
        PCaselessString str = stateElement->GetData();
        for (info.m_state = SIPDialogNotification::LastState; info.m_state > SIPDialogNotification::FirstState; --info.m_state) {
          if (str == info.GetStateName())
            break;
        }

        str = stateElement->GetAttribute("event");
        for (info.m_eventType = SIPDialogNotification::LastEvent; info.m_eventType >= SIPDialogNotification::FirstEvent; --info.m_eventType) {
          if (str == info.GetEventName())
            break;
        }

        info.m_eventCode = stateElement->GetAttribute("code").AsUnsigned();
      }

      ParseParticipant(dialogElement->GetElement("local"), info.m_local);
      ParseParticipant(dialogElement->GetElement("remote"), info.m_remote);
      notifyInfo.m_endpoint.OnDialogInfoReceived(info);
      index++;
    }
#else
    SIPDialogNotification info(notifyInfo.m_request.GetMIME().GetFrom());
    notifyInfo.SendResponse();
#endif // P_EXPAT

    if (index == 0)
      notifyInfo.m_endpoint.OnDialogInfoReceived(info);
  }

  virtual PString OnSendNOTIFY(SIPHandler & handler, const PObject * data)
  {
    PStringStream body;
    body << "<?xml version=\"1.0\"?>\r\n"
            "<dialog-info xmlns=\"urn:ietf:params:xml:ns:dialog-info\" version=\""
         << m_dialogNotifyVersion++ << "\" state=\"partial\" entity=\""
         << handler.GetAddressOfRecord() << "\">\r\n";

    std::map<PString, SIPDialogNotification>::iterator iter;

    const SIPDialogNotification * info = dynamic_cast<const SIPDialogNotification *>(data);
    if (info != NULL) {
      if (info->m_state != SIPDialogNotification::Terminated)
        m_activeDialogs[info->m_callId] = *info;
      else {
        iter = m_activeDialogs.find(info->m_callId);
        if (iter != m_activeDialogs.end())
          m_activeDialogs.erase(iter);

        body << *info;
      }
    }

    for (iter = m_activeDialogs.begin(); iter != m_activeDialogs.end(); ++iter)
      body << iter->second;

    body << "</dialog-info>\r\n";
    return body;
  }

  unsigned m_dialogNotifyVersion;
  std::map<PString, SIPDialogNotification> m_activeDialogs;
};

static SIPEventPackageFactory::Worker<SIPDialogEventPackageHandler> dialogEventPackageHandler(SIPSubscribe::Dialog);


///////////////////////////////////////////////////////////////////////////////

class SIPRegEventPackageHandler : public SIPEventPackageHandler
{
  virtual PCaselessString GetContentType() const
  {
    return "application/reginfo+xml";
  }

  virtual void OnReceivedNOTIFY(SIPSubscribe::NotifyCallbackInfo & notifyInfo)
  {
#if P_EXPAT
    static PXML::ValidationInfo const RegInfoValidation[] = {
      { PXML::SetDefaultNamespace,        "urn:ietf:params:xml:ns:reginfo" },
      { PXML::ElementName,                "reginfo", },
      { PXML::RequiredNonEmptyAttribute,  "version"},
      { PXML::RequiredAttributeWithValue, "state",   { "full\npartial" } },

      { PXML::EndOfValidationList }
    };

    PXML xml;
    if (!notifyInfo.LoadAndValidate(xml, RegInfoValidation))
      return;

    // Enough is OK to send OK now
    notifyInfo.SendResponse();

    PXMLElement * rootElement = xml.GetRootElement();
    if (!ValidateNotificationSequence(notifyInfo.m_handler,
                                      rootElement->GetAttribute("version").AsUnsigned(),
                                      rootElement->GetAttribute("state") *= "full"))
      return;

    PINDEX regIndex = 0;
    PXMLElement * regElement;
    while ((regElement = rootElement->GetElement("registration", regIndex++)) != NULL) {
      SIPRegNotification info(regElement->GetAttribute("aor"),
                              SIPRegNotification::GetStateFromName(regElement->GetAttribute("state")));
      PINDEX contactIndex = 0;
      PXMLElement * contactElement;
      while ((contactElement = regElement->GetElement("contact", contactIndex++)) != NULL) {
        PXMLElement * element = contactElement->GetElement("uri");
        if (element == NULL)
          continue;

        SIPURL contact(element->GetData());

        PStringOptions & fields = contact.GetFieldParameters();
        fields.Set("expires", contactElement->GetAttribute("duration-registered"));
        fields.Set("q", contactElement->GetAttribute("q"));
        fields.Set("state", contactElement->GetAttribute("state"));
        fields.Set("event", contactElement->GetAttribute("event"));

        if ((element = contactElement->GetElement("display-name")) != NULL)
          contact.SetDisplayName(element->GetData());

        info.m_contacts.push_back(contact);
      }
      notifyInfo.m_endpoint.OnRegInfoReceived(info);
    }
#else
    notifyInfo.SendResponse();

    SIPRegNotification info(notifyInfo.m_request.GetMIME().GetFrom());
    notifyInfo.m_endpoint.OnRegInfoReceived(info);
#endif // P_EXPAT
  }
};

static SIPEventPackageFactory::Worker<SIPRegEventPackageHandler> regEventPackageHandler(SIPSubscribe::Reg);


///////////////////////////////////////////////////////////////////////////////

SIPDialogNotification::SIPDialogNotification(const PString & entity)
  : m_entity(entity)
  , m_initiator(false)
  , m_state(Terminated)
  , m_eventType(NoEvent)
  , m_eventCode(0)
{
}


PString SIPDialogNotification::GetStateName(States state)
{
  static const char * const Names[] = {
    "terminated",
    "trying",
    "proceeding",
    "early",
    "confirmed"
  };
  if (state < PARRAYSIZE(Names) && Names[state] != NULL)
    return Names[state];

  return psprintf("<%u>", state);
}


PString SIPDialogNotification::GetEventName(Events state)
{
  static const char * const Names[] = {
    "cancelled",
    "rejected",
    "replaced",
    "local-bye",
    "remote-bye",
    "error",
    "timeout"
  };
  if (state < PARRAYSIZE(Names) && Names[state] != NULL)
    return Names[state];

  return psprintf("<%u>", state);
}


static void OutputParticipant(ostream & strm, const char * name, const SIPDialogNotification::Participant & participant)
{
  if (participant.m_URI.IsEmpty())
    return;

  strm << "    <" << name << ">\r\n";

  if (!participant.m_identity.IsEmpty()) {
    strm << "      <identity";
    if (!participant.m_display.IsEmpty())
      strm << " display=\"" << participant.m_display << '"';
    strm << '>' << participant.m_identity << "</identity>\r\n";
  }

  strm << "      <target uri=\"" << participant.m_URI << "\">\r\n";

  if (participant.m_appearance >= 0)
    strm << "        <param pname=\"appearance\" pval=\"" << participant.m_appearance << "\"/>\r\n"
            "        <param pname=\"x-line-id\" pval=\"" << participant.m_appearance << "\"/>\r\n";

  if (participant.m_byeless)
    strm << "        <param pname=\"sip.byeless\" pval=\"true\"/>\r\n";

  if (participant.m_rendering >= 0)
    strm << "        <param pname=\"sip.rendering\" pval=\"" << (participant.m_rendering > 0 ? "yes" : "no") << "\"/>\r\n";

  strm << "      </target>\r\n"
       << "    </" << name << ">\r\n";
}


void SIPDialogNotification::PrintOn(ostream & strm) const
{
  if (m_dialogId.IsEmpty())
    return;

  // Start dialog XML tag
  strm << "  <dialog id=\"" << m_dialogId << '"';
  if (!m_callId)
    strm << " call-id=\"" << m_callId << '"';
  if (!m_local.m_dialogTag)
    strm << " local-tag=\"" << m_local.m_dialogTag << '"';
  if (!m_remote.m_dialogTag)
    strm << " remote-tag=\"" << m_remote.m_dialogTag << '"';
  strm << " direction=\"" << (m_initiator ? "initiator" : "receiver") << "\">\r\n";

  // State XML tag & value
  strm << "    <state";
  if (m_eventType > SIPDialogNotification::NoEvent) {
    strm << " event=\"" << GetEventName() << '"';
    if (m_eventCode > 0)
      strm << " code=\"" << m_eventCode << '"';
  }
  strm << '>' << GetStateName() << "</state>\r\n";

  // Participant XML tags (local/remopte)
  OutputParticipant(strm, "local", m_local);
  OutputParticipant(strm, "remote", m_remote);

  // Close out dialog tag
  strm << "  </dialog>\r\n";
}


///////////////////////////////////////////////////////////////////////////////

SIPRegNotification::SIPRegNotification(const SIPURL & aor, States state)
  : m_aor(aor)
  , m_state(state)
{
}


PString SIPRegNotification::GetStateName(States state)
{
  static const char * const Names[] = {
    "init",
    "active",
    "terminated"
  };
  if (state < PARRAYSIZE(Names) && Names[state] != NULL)
    return Names[state];

  return psprintf("<%u>", state);
}


SIPRegNotification::States SIPRegNotification::GetStateFromName(const PCaselessString & str)
{
  States state = Initial;
  while (state < NumStates && str != GetStateName(state))
   ++state;

  return state;
}


void SIPRegNotification::PrintOn(ostream & strm) const
{
  if (m_aor.IsEmpty())
    return;

  strm << "  <reginfo xmlns=\"urn:ietf:params:xml:ns:reginfo\" version=\"0\" state=\"full\">\r\n"
       << "    <registration aor=\"" << m_aor << "\" state=\"" << GetStateName() << "\">\r\n";

  for (std::list<SIPURL>::const_iterator it = m_contacts.begin(); it != m_contacts.end(); ++it) {
    const PStringOptions & fields = it->GetFieldParameters();
    strm << "      <contact state=\""               << fields.Get("state") << "\" "
                           "event=\""               << fields.Get("event") << "\" "
                           "duration-registered=\"" << fields.Get("expires") << "\" "
                           "q=\""                   << fields.Get("q") << "\">\r\n"
            "        <uri>" << *it << "</uri>\r\n";
    if (!it->GetDisplayName().IsEmpty())
      strm << "        <display-name>" << it->GetDisplayName() << "</display-name>\r\n";
    strm << "      </contact>\r\n";
  }
  // Close out dialog tag
  strm << "    </registration>\r\n"
          "  </reginfo>\r\n";
}


/////////////////////////////////////////////////////////////////////////

SIPNotifyHandler::SIPNotifyHandler(SIPEndPoint & endpoint,
                                   const SIPEventPackage & eventPackage,
                                   const SIPDialogContext & dialog)
  : SIPHandler(SIP_PDU::Method_NOTIFY,
               endpoint,
               SIPParameters(dialog.GetLocalURI().AsString(), dialog.GetRemoteURI().AsString()),
               dialog.GetCallID())
  , m_eventPackage(eventPackage)
  , m_dialog(dialog)
  , m_reason(Deactivated)
  , m_packageHandler(SIPEventPackageFactory::CreateInstance(eventPackage))
{
  PTRACE_CONTEXT_ID_TO(m_packageHandler);
}


SIPNotifyHandler::~SIPNotifyHandler()
{
  delete m_packageHandler;
}


SIPTransaction * SIPNotifyHandler::CreateTransaction(OpalTransport & trans)
{
  PString state;
  PString body;

  if (GetExpire() > 0 && GetState() != Unsubscribing) {
    state.sprintf("active;expires=%u", GetExpire());
    body = m_body;
  }
  else {
    state = "terminated;reason=";
    static const char * const ReasonNames[] = {
      "deactivated",
      "probation",
      "rejected",
      "timeout",
      "giveup",
      "noresource"
    };
    state += ReasonNames[m_reason];
  }

  return new SIPNotify(GetEndPoint(), trans, m_dialog, m_eventPackage, state, body);
}


PBoolean SIPNotifyHandler::SendRequest(SIPHandler::State state)
{
  // If times out, i.e. Refreshing, then this is actually a time out unsubscribe.
  if (state == Refreshing)
    m_reason = Timeout;

  return SIPHandler::SendRequest(state == Refreshing ? Unsubscribing : state);
}


bool SIPNotifyHandler::WriteSIPHandler(OpalTransport & transport, bool forked)
{
  m_dialog.SetForking(forked);
  bool ok = SIPHandler::WriteSIPHandler(transport, false);
  m_dialog.SetForking(false);
  return ok;
}


bool SIPNotifyHandler::SendNotify(const PObject * body)
{
  if (!LockReadWrite())
    return false;

  if (m_packageHandler != NULL)
    m_body = m_packageHandler->OnSendNOTIFY(*this, body);
  else if (body == NULL)
    m_body.MakeEmpty();
  else {
    PStringStream str;
    str << *body;
    m_body = str;
  }

  UnlockReadWrite();

  return ActivateState(Subscribing);
}


/////////////////////////////////////////////////////////////////////////

SIPPublishHandler::SIPPublishHandler(SIPEndPoint & endpoint,
                                     const SIPSubscribe::Params & params,
                                     const PString & body)
  : SIPHandler(SIP_PDU::Method_PUBLISH, endpoint, params)
  , m_parameters(params)
  , m_body(body)
{
  m_parameters.m_proxyAddress = m_proxy.AsString();
}


SIPTransaction * SIPPublishHandler::CreateTransaction(OpalTransport & transport)
{
  if (GetState() == Unsubscribing)
    return NULL;

  m_parameters.m_expire = m_originalExpireTime;
  return new SIPPublish(GetEndPoint(),
                        transport,
                        GetCallID(),
                        m_sipETag,
                        m_parameters,
                        (GetState() == Refreshing) ? PString::Empty() : m_body);
}


void SIPPublishHandler::OnReceivedOK(SIPTransaction & transaction, SIP_PDU & response)
{
  PString newETag = response.GetMIME().GetSIPETag();

  if (!newETag.IsEmpty())
    m_sipETag = newETag;

  SetExpire(response.GetMIME().GetExpires(m_originalExpireTime));

  SIPHandler::OnReceivedOK(transaction, response);
}


///////////////////////////////////////////////////////////////////////

static PAtomicInteger DefaultTupleIdentifier(PRandom::Number());

SIPPresenceInfo::SIPPresenceInfo(State state)
  : OpalPresenceInfo(state)
  , m_tupleId(PString::Printf, "T%08X", ++DefaultTupleIdentifier)
{
}


void SIPPresenceInfo::PrintOn(ostream & strm) const
{
  if (m_entity.IsEmpty())
    return;

  if (m_activities.GetSize() > 0)
    strm << setfill(',') << m_activities << setfill(' ');
  else {
    switch (m_state) {
      case Unchanged :
        strm << "Unchanged";
        break;

      case NoPresence :
        strm << "Closed";
        break;

      default:
        if (m_note.IsEmpty())
          strm << "Open";
        else
          strm << m_note;
    }
  }
}

// defined in RFC 4480
static const char * const ExtendedSIPActivities[] = {
      "appointment",
      "away",
      "breakfast",
      "busy",
      "dinner",
      "holiday",
      "in-transit",
      "looking-for-work",
      "lunch",
      "meal",
      "meeting",
      "on-the-phone",
      "other",
      "performance",
      "permanent-absence",
      "playing",
      "presentation",
      "shopping",
      "sleeping",
      "spectator",
      "steering",
      "travel",
      "tv",
      "vacation",
      "working",
      "worship"
};

bool SIPPresenceInfo::AsSIPActivityString(State state, PString & str)
{
  if ((state >= Appointment) && (state <= Worship)) {
    str = PString(ExtendedSIPActivities[state - Appointment]);
    return true;
  }

  return false;
}

bool SIPPresenceInfo::AsSIPActivityString(PString & str) const
{
  return AsSIPActivityString(m_state, str);
}

OpalPresenceInfo::State SIPPresenceInfo::FromSIPActivityString(const PString & str)
{
  for (size_t i = 0; i < sizeof(ExtendedSIPActivities)/sizeof(ExtendedSIPActivities[0]);++i) {
    if (str == ExtendedSIPActivities[i])
      return (State)(Appointment + i);
  }

  return NoPresence;
}

PString SIPPresenceInfo::AsXML() const
{
  if (m_entity.IsEmpty() || m_tupleId.IsEmpty()) {
    PTRACE(1, "SIP\tCannot encode Presence XML as no address or no id.");
    return PString::Empty();
  }

  PStringStream xml;

  xml << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n"
         "<presence xmlns=\"urn:ietf:params:xml:ns:pidf\" "
                  " xmlns:dm=\"urn:ietf:params:xml:ns:pidf:data-model\""
                  " xmlns:rpid=\"urn:ietf:params:xml:ns:pidf:rpid\""
                  " entity=\"" << m_entity << "\">\r\n"
         "  <tuple id=\"" << m_tupleId << "\">\r\n"
         "    <status>\r\n";
  if (m_state != Unchanged)
    xml << "      <basic>" << (m_state != NoPresence ? "open" : "closed") << "</basic>\r\n";
  xml << "    </status>\r\n"
         "    <contact priority=\"1\">";
  if (m_contact.IsEmpty())
    xml << m_entity;
  else
    xml << m_contact;
  xml << "</contact>\r\n";

  if (!m_note.IsEmpty()) {
    //xml << "    <note xml:lang=\"en\">" << PXML::EscapeSpecialChars(m_note) << "</note>\r\n";
    xml << "    <note>" << PXML::EscapeSpecialChars(m_note) << "</note>\r\n";
  }

  xml << "    <timestamp>" << PTime().AsString(PTime::RFC3339) << "</timestamp>\r\n"
         "  </tuple>\r\n";
  if (!m_personId.IsEmpty() && (((m_state >= Appointment) && (m_state <= Worship)) || (m_activities.GetSize() > 0))) {
    xml << "  <dm:person id=\"p" << m_personId << "\">\r\n"
           "    <rpid:activities>\r\n";
    bool doneState = false;
    for (PINDEX i = 0; i < m_activities.GetSize(); ++i) {
      State s = FromString(m_activities[i]);
      if (s >= Appointment) {
        if (s == m_state)
          doneState = true;
        xml << "      <rpid:" << ExtendedSIPActivities[s - Appointment] <<"/>\r\n";
      }
    }
    if (!doneState)
      xml << "      <rpid:" << ExtendedSIPActivities[m_state - Appointment] <<"/>\r\n";

    xml << "    </rpid:activities>\r\n"
           "  </dm:person>\r\n";
  }

  xml << "</presence>\r\n";

  return xml;
}


#if P_EXPAT
static void SetNoteFromElement(PXMLElement * element, PString & note)
{
  PXMLElement * noteElement = element->GetElement("note");
  if (noteElement != NULL)
    note = noteElement->GetData();
}

bool SIPPresenceInfo::ParseNotify(SIPSubscribe::NotifyCallbackInfo & notifyInfo,
                                  list<SIPPresenceInfo> & infoList)
{
  infoList.clear();

  static PXML::ValidationInfo const StatusValidation[] = {
    { PXML::RequiredElement,            "basic" },
    { PXML::EndOfValidationList }
  };

  static PXML::ValidationInfo const TupleValidation[] = {
    { PXML::RequiredNonEmptyAttribute,  "id" },
    { PXML::Subtree,                    "status", { StatusValidation }, 1 },
    { PXML::EndOfValidationList }
  };

  static PXML::ValidationInfo const ActivitiesValidation[] = {
    { PXML::OptionalElement,            "rpid:busy" },
    { PXML::EndOfValidationList }
  };

  static PXML::ValidationInfo const PersonValidation[] = {
    { PXML::Subtree,                    "rpid:activities", { ActivitiesValidation }, 0, 1 },
    { PXML::EndOfValidationList }
  };

  static PXML::ValidationInfo const PresenceValidation[] = {
    { PXML::SetDefaultNamespace,        "urn:ietf:params:xml:ns:pidf" },
    { PXML::SetNamespace,               "dm",   { "urn:ietf:params:xml:ns:pidf:data-model" } },
    { PXML::SetNamespace,               "rpid", { "urn:ietf:params:xml:ns:pidf:rpid" } },
    { PXML::ElementName,                "presence" },
    { PXML::RequiredNonEmptyAttribute,  "entity" },
    { PXML::Subtree,                    "tuple",  { TupleValidation }, 0 },
    { PXML::Subtree,                    "dm:person", { PersonValidation }, 0, 1 },
    { PXML::EndOfValidationList }
  };

  PXML xml;
  if (!notifyInfo.LoadAndValidate(xml, PresenceValidation))
    return false;

  // Common info to all tuples
  PURL entity;
  PXMLElement * rootElement = xml.GetRootElement();
  if (notifyInfo.m_handler.GetProductInfo().name.Find("Asterisk") != P_MAX_INDEX) {
    entity = notifyInfo.m_response.GetMIME().GetFrom();
    PTRACE2(3, NULL, "SIP\tCompensating for " << notifyInfo.m_handler.GetProductInfo().name
           << ", replacing entity with " << entity.AsString());
  }
  else if (!entity.Parse(rootElement->GetAttribute("entity"), "pres")) {
    notifyInfo.m_response.SetEntityBody("Invalid/unsupported entity");
    PTRACE2(1, NULL, "SIPPres\tInvalid/unsupported entity \"" << rootElement->GetAttribute("entity") << '"');
    return false;
  }

  SIPPresenceInfo info;
  info.m_tupleId.MakeEmpty();

  PTime defaultTimestamp; // Start with "now"

  for (PINDEX idx = 0; idx < rootElement->GetSize(); ++idx) {
    PXMLElement * element = dynamic_cast<PXMLElement *>(rootElement->GetElement(idx));
    if (element == NULL)
      continue;

    if (element->GetName() == "urn:ietf:params:xml:ns:pidf|tuple") {
      PXMLElement * tupleElement = element;

      if (!info.m_tupleId.IsEmpty()) {
        infoList.push_back(info);
        defaultTimestamp = info.m_when - PTimeInterval(0, 1); // One second older
        info = SIPPresenceInfo();
      }

      info.m_entity = entity;
      info.m_tupleId = tupleElement->GetAttribute("id");

      SetNoteFromElement(rootElement, info.m_note);
      SetNoteFromElement(tupleElement, info.m_note);

      if ((element = tupleElement->GetElement("status")) != NULL) {
        SetNoteFromElement(element, info.m_note);
        if ((element = element->GetElement("basic")) != NULL) {
          PCaselessString value = element->GetData();
          if (value == "open")
            info.m_state = Available;
          else if (value == "closed")
            info.m_state = NoPresence;
        }
      }

      if ((element = tupleElement->GetElement("contact")) != NULL)
        info.m_contact = element->GetData();

      if ((element = tupleElement->GetElement("timestamp")) == NULL || !info.m_when.Parse(element->GetData()))
        info.m_when = defaultTimestamp;
    }
    else if (element->GetName() == "urn:ietf:params:xml:ns:pidf:data-model|person") {
      static PConstCaselessString rpid("urn:ietf:params:xml:ns:pidf:rpid|");
      PXMLElement * activities = element->GetElement(rpid + "activities");
      if (activities == NULL)
        continue;

      for (PINDEX i = 0; i < activities->GetSize(); ++i) {
        PXMLElement * activity = dynamic_cast<PXMLElement *>(activities->GetElement(i));
        if (activity == NULL)
          continue;

        PCaselessString name(activity->GetName());
        if (name.NumCompare(rpid) != PObject::EqualTo)
          continue;

        name.Delete(0, rpid.GetLength());
        info.m_activities.AppendString(name);

        State newState = SIPPresenceInfo::FromSIPActivityString(name);
        if (newState != SIPPresenceInfo::NoPresence && info.m_state == Available)
          info.m_state = newState;
      }
    }
  }

  if (!info.m_tupleId.IsEmpty())
    infoList.push_back(info);

  infoList.sort();

  return true;
}
#else // P_EXPAT
bool SIPPresenceInfo::ParseNotify(SIPSubscribe::NotifyCallbackInfo &,
                                  list<SIPPresenceInfo> &)
{
  return false;
}
#endif // P_EXPAT

/////////////////////////////////////////////////////////////////////////

SIPMessageHandler::SIPMessageHandler(SIPEndPoint & endpoint, const SIPMessage::Params & params)
  : SIPHandler(SIP_PDU::Method_MESSAGE, endpoint, params, params.m_id)
  , m_parameters(params)
  , m_messageSent(false)
{
  m_parameters.m_proxyAddress = m_proxy.AsString();
  m_parameters.m_id = GetCallID();

  m_offlineExpireTime = 0; // No retries for offline, just give up

  SetState(Subscribed);
}


SIPTransaction * SIPMessageHandler::CreateTransaction(OpalTransport & transport)
{
  if (GetState() == Unsubscribing)
    return NULL;

  // If message ID is zero, then it was sent once, don't do it again.
  if (m_messageSent) {
    PTRACE(4, "SIP\tMessage was already sent, not sending again.");
    return NULL;
  }

  SetExpire(m_originalExpireTime);

  SIPMessage * msg = new SIPMessage(GetEndPoint(), transport, m_parameters);
  m_parameters.m_localAddress = msg->GetLocalAddress().AsString();
  return msg;
}


void SIPMessageHandler::OnFailed(SIP_PDU::StatusCodes reason)
{
  SIPHandler::OnFailed(reason);

  if (m_messageSent)
    return;

  GetEndPoint().OnMESSAGECompleted(m_parameters, reason);
  m_messageSent = true;
}


void SIPMessageHandler::OnReceivedOK(SIPTransaction & transaction, SIP_PDU & response)
{
  SIPHandler::OnReceivedOK(transaction, response);
  GetEndPoint().OnMESSAGECompleted(m_parameters, SIP_PDU::Successful_OK);
  m_messageSent = true;
}


void SIPMessageHandler::UpdateParameters(const SIPMessage::Params & params)
{
  m_parameters.m_remoteAddress = params.m_remoteAddress;
  m_parameters.m_localAddress = params.m_localAddress;
  m_parameters.m_messageId = params.m_messageId;

  if (!params.m_body.IsEmpty()) {
    m_parameters.m_body = params.m_body;
    m_parameters.m_contentType = params.m_contentType;
  }

  m_messageSent = false;
}


/////////////////////////////////////////////////////////////////////////

SIPOptionsHandler::SIPOptionsHandler(SIPEndPoint & endpoint, const SIPOptions::Params & params)
  : SIPHandler(SIP_PDU::Method_OPTIONS, endpoint, params)
  , m_parameters(params)
{
  m_parameters.m_proxyAddress = m_proxy.AsString();

  m_offlineExpireTime = 0; // No retries for offline, just give up

  SetState(Subscribed);
}


SIPTransaction * SIPOptionsHandler::CreateTransaction(OpalTransport & transport)
{ 
  if (GetState() == Unsubscribing)
    return NULL;

  return new SIPOptions(GetEndPoint(), transport, GetCallID(), m_parameters);
}


void SIPOptionsHandler::OnFailed(SIP_PDU::StatusCodes reason)
{
  SIP_PDU dummy;
  dummy.SetStatusCode(reason);
  GetEndPoint().OnOptionsCompleted(m_parameters, dummy);
  SIPHandler::OnFailed(reason);
}


void SIPOptionsHandler::OnFailed(const SIP_PDU & response)
{
  GetEndPoint().OnOptionsCompleted(m_parameters, response);
  SIPHandler::OnFailed(response.GetStatusCode());
}


void SIPOptionsHandler::OnReceivedOK(SIPTransaction & transaction, SIP_PDU & response)
{
  GetEndPoint().OnOptionsCompleted(m_parameters, response);
  SIPHandler::OnReceivedOK(transaction, response);
}


/////////////////////////////////////////////////////////////////////////

SIPPingHandler::SIPPingHandler(SIPEndPoint & endpoint, const PURL & to)
  : SIPHandler(SIP_PDU::Method_MESSAGE, endpoint, SIPParameters(to.AsString()))
{
}


SIPTransaction * SIPPingHandler::CreateTransaction(OpalTransport & transport)
{
  if (GetState() == Unsubscribing)
    return NULL;

  return new SIPPing(GetEndPoint(), transport, GetAddressOfRecord());
}


//////////////////////////////////////////////////////////////////

/* All of the bwlow search loops run through the list with only
   PSafeReference rather than PSafeReadOnly, even though they are
   reading fields from the handler instances. We can get away with
   this becuase the information being tested, e.g. AOR, is constant
   for the life of the handler instance, once constructed.

   We need to use PSafeReference as there are some cases where
   deadlocks can occur when locked handlers look for information
   from other handlers.
 */
unsigned SIPHandlersList::GetCount(SIP_PDU::Methods meth, const PString & eventPackage) const
{
  unsigned count = 0;
  for (PSafePtr<SIPHandler> handler(m_handlersList, PSafeReference); handler != NULL; ++handler)
    if (handler->GetState () == SIPHandler::Subscribed &&
        handler->GetMethod() == meth &&
        (eventPackage.IsEmpty() || handler->GetEventPackage() == eventPackage))
      count++;
  return count;
}


PStringList SIPHandlersList::GetAddresses(bool includeOffline, SIP_PDU::Methods meth, const PString & eventPackage) const
{
  PStringList addresses;
  for (PSafePtr<SIPHandler> handler(m_handlersList, PSafeReference); handler != NULL; ++handler)
    if ((includeOffline ? handler->GetState () != SIPHandler::Unsubscribed
                        : handler->GetState () == SIPHandler::Subscribed) &&
        handler->GetMethod() == meth &&
        (eventPackage.IsEmpty() || handler->GetEventPackage() == eventPackage))
      addresses.AppendString(handler->GetAddressOfRecord().AsString());
  return addresses;
}


static PString MakeUrlKey(const PString & aor, SIP_PDU::Methods method, const PString & eventPackage = PString::Empty())
{
  return PString((unsigned)method) + aor + eventPackage;
}


/**
  * called when a handler is added
  */

void SIPHandlersList::Append(SIPHandler * newHandler)
{
  if (newHandler == NULL)
    return;

  PWaitAndSignal mutex(m_handlersList.GetMutex());

  PSafePtr<SIPHandler> handler = m_handlersList.FindWithLock(*newHandler, PSafeReference);
  if (handler == NULL)
    handler = m_handlersList.Append(newHandler);

  // add entry to url and package map
  PString key = MakeUrlKey(handler->GetAddressOfRecord(), handler->GetMethod(), handler->GetEventPackage());
  handler->m_byAorAndPackage = m_byAorAndPackage.insert(IndexMap::value_type(key, handler));
  PTRACE_IF(1, !handler->m_byAorAndPackage.second, "Duplicate handler for Method/AOR/Package=\"" << key << '"');

  // add entry to username/realm map
  PString realm = handler->GetRealm();
  if (realm.IsEmpty())
    return;

  PString username = handler->GetUsername();
  if (!username.IsEmpty()) {
    handler->m_byAuthIdAndRealm = m_byAuthIdAndRealm.insert(IndexMap::value_type(username + '\n' + realm, handler));
    PTRACE_IF(4, !handler->m_byAuthIdAndRealm.second, "Duplicate handler for authId=\"" << username << "\", realm=\"" << realm << '"');
  }

  username = handler->GetAddressOfRecord().GetUserName();
  if (!username.IsEmpty()) {
    handler->m_byAorUserAndRealm = m_byAorUserAndRealm.insert(IndexMap::value_type(username + '\n' + realm, handler));
    PTRACE_IF(4, !handler->m_byAuthIdAndRealm.second, "Duplicate handler for AOR user=\"" << username << "\", realm=\"" << realm << '"');
  }
}


/**
  * Called when a handler is removed
  */
void SIPHandlersList::Remove(SIPHandler * handler)
{
  if (handler == NULL)
    return;

  PWaitAndSignal mutex(m_handlersList.GetMutex());

  if (m_handlersList.Remove(handler))
    RemoveIndexes(handler);
}


void SIPHandlersList::Update(SIPHandler * handler)
{
  if (handler == NULL)
    return;

  PWaitAndSignal mutex(m_handlersList.GetMutex());

  RemoveIndexes(handler);
  Append(handler);
}


void SIPHandlersList::RemoveIndexes(SIPHandler * handler)
{
  if (handler->m_byAorUserAndRealm.second)
    m_byAorUserAndRealm.erase(handler->m_byAorUserAndRealm.first);

  if (handler->m_byAuthIdAndRealm.second)
    m_byAuthIdAndRealm.erase(handler->m_byAuthIdAndRealm.first);

  if (handler->m_byAorAndPackage.second)
    m_byAorAndPackage.erase(handler->m_byAorAndPackage.first);
}


PSafePtr<SIPHandler> SIPHandlersList::FindBy(IndexMap & by, const PString & key, PSafetyMode mode)
{
  PSafePtr<SIPHandler> ptr;
  {
    PWaitAndSignal mutex(m_handlersList.GetMutex());

    IndexMap::iterator r = by.find(key);
    if (r == by.end())
      return NULL;

    ptr = r->second;
    // If above assignement ends up NULL, then entry in IndexMap was deleted
    if (ptr == NULL)
      return NULL;
  }

  if (ptr && ptr->GetState() != SIPHandler::Unsubscribed)
    return ptr.SetSafetyMode(mode) ? ptr : NULL;

  PTRACE(3, "SIP\tHandler " << *ptr << " unsubscribed, awaiting shutdown.");
  while (!ptr->ShutDown())
    PThread::Sleep(100);

  Remove(ptr);
  return NULL;
}


PSafePtr<SIPHandler> SIPHandlersList::FindSIPHandlerByCallID(const PString & callID, PSafetyMode mode)
{
  return m_handlersList.FindWithLock(callID, mode);
}


PSafePtr<SIPHandler> SIPHandlersList::FindSIPHandlerByUrl(const PString & aor, SIP_PDU::Methods method, PSafetyMode mode)
{
  return FindBy(m_byAorAndPackage, MakeUrlKey(aor, method), mode);
}


PSafePtr<SIPHandler> SIPHandlersList::FindSIPHandlerByUrl(const PString & aor, SIP_PDU::Methods method, const PString & eventPackage, PSafetyMode mode)
{
  return FindBy(m_byAorAndPackage, MakeUrlKey(aor, method, eventPackage), mode);
}


PSafePtr<SIPHandler> SIPHandlersList::FindSIPHandlerByAuthRealm(const PString & authRealm, PSafetyMode mode)
{
  // look for a match to realm without users
  for (PSafePtr<SIPHandler> handler(m_handlersList, PSafeReference); handler != NULL; ++handler) {
    if (handler->GetRealm() == authRealm && handler.SetSafetyMode(mode)) {
      PTRACE(4, "SIP\tLocated existing credentials for realm \"" << authRealm << '"');
      return handler;
    }
  }

  return NULL;
}


PSafePtr<SIPHandler> SIPHandlersList::FindSIPHandlerByAuthRealm(const PString & authRealm, const PString & userName, PSafetyMode mode)
{
  PSafePtr<SIPHandler> ptr;

  // look for a match to exact user name and realm
  if ((ptr = FindBy(m_byAuthIdAndRealm, userName + '\n' + authRealm, mode)) != NULL) {
    PTRACE(4, "SIP\tLocated existing credentials for ID \"" << userName << "\" at realm \"" << authRealm << '"');
    return ptr;
  }

  // look for a match to exact user name and realm
  if ((ptr = FindBy(m_byAorUserAndRealm, userName + '\n' + authRealm, mode)) != NULL) {
    PTRACE(4, "SIP\tLocated existing credentials for ID \"" << userName << "\" at realm \"" << authRealm << '"');
    return ptr;
  }

  return NULL;
}


/**
 * Find the SIPHandler object with the specified registration host.
 * For example, in the above case, the name parameter
 * could be "sip.seconix.com" or "seconix.com".
 */
PSafePtr<SIPHandler> SIPHandlersList::FindSIPHandlerByDomain(const PString & name, SIP_PDU::Methods meth, PSafetyMode mode)
{
  for (PSafePtr<SIPHandler> handler(m_handlersList, PSafeReference); handler != NULL; ++handler) {
    if ( handler->GetMethod() == meth &&
         handler->GetState() != SIPHandler::Unsubscribed &&
        (handler->GetAddressOfRecord().GetHostName() == name ||
         handler->GetAddressOfRecord().GetHostAddress().IsEquivalent(name)) &&
         handler.SetSafetyMode(mode))
      return handler;
  }
  return NULL;
}


#endif // OPAL_SIP
