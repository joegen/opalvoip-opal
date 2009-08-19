/*
 * handlers.cxx
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
 * The Initial Developer of the Original Code is Damien Sandras. 
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

#ifdef __GNUC__
#pragma implementation "handlers.h"
#endif

#include <sip/handlers.h>

#include <ptclib/pdns.h>
#include <ptclib/enum.h>
#include <sip/sipep.h>

#if OPAL_PTLIB_EXPAT
#include <ptclib/pxml.h>
#endif


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

SIPHandler::SIPHandler(SIPEndPoint & ep,
                       const PString & target,
                       const PString & remote,
                       int expireTime,
                       int offlineExpireTime,
                       const PTimeInterval & retryMin,
                       const PTimeInterval & retryMax)
  : endpoint(ep)
  , m_transport(NULL)
  , callID(SIPTransaction::GenerateCallID())
  , expire(expireTime > 0 ? expireTime : endpoint.GetNotifierTimeToLive().GetSeconds())
  , originalExpire(expire)
  , offlineExpire(offlineExpireTime)
  , state(Unavailable)
  , retryTimeoutMin(retryMin)
  , retryTimeoutMax(retryMax)
{
  transactions.DisallowDeleteObjects();

  /* Try to be intelligent about what we got in the two fields target/remote,
     we have several scenarios depending on which is a partial or full URL.
   */

  if (target.IsEmpty()) {
    if (remote.IsEmpty())
      m_addressOfRecord = m_remoteAddress = ep.GetDefaultLocalPartyName() + '@' + PIPSocket::GetHostName();
    else if (remote.Find('@') == P_MAX_INDEX)
      m_addressOfRecord = m_remoteAddress = ep.GetDefaultLocalPartyName() + '@' + remote;
    else
      m_addressOfRecord = m_remoteAddress = remote;
  }
  else if (target.Find('@') == P_MAX_INDEX) {
    if (remote.IsEmpty())
      m_addressOfRecord = m_remoteAddress = ep.GetDefaultLocalPartyName() + '@' + target;
    else if (remote.Find('@') == P_MAX_INDEX)
      m_addressOfRecord = m_remoteAddress = target + '@' + remote;
    else {
      m_remoteAddress = remote;
      m_addressOfRecord = target + '@' + m_remoteAddress.GetHostName();
    }
  }
  else {
    m_addressOfRecord = target;
    if (remote.IsEmpty())
      m_remoteAddress = m_addressOfRecord;
    else if (remote.Find('@') != P_MAX_INDEX)
      m_remoteAddress = remote; // For third party registrations
    else {
      SIPURL remoteURL = remote;
      if (m_addressOfRecord.GetHostAddress().IsEquivalent(remoteURL.GetHostAddress()))
        m_remoteAddress = m_addressOfRecord;
      else {
        /* Note this sets the proxy field because the user has givena full AOR
           with a domain for "target" and then specified a specific host name
           which as far as we are concered is the host to talk to. Setting the
           proxy will prevent SRV lookups or other things that might stop uis
           from going to that very specific host.
         */
        m_remoteAddress = m_proxy = remoteURL;
        m_remoteAddress.SetUserName(m_addressOfRecord.GetUserName());
      }
    }
  }

  authenticationAttempts = 0;
  authentication = NULL;

  expireTimer.SetNotifier(PCREATE_NOTIFIER(OnExpireTimeout));
}


SIPHandler::~SIPHandler() 
{
  expireTimer.Stop();

  if (m_transport) {
    m_transport->CloseWait();
    delete m_transport;
  }

  delete authentication;

  PTRACE(4, "SIP\tDeleted handler.");
}


bool SIPHandler::ShutDown()
{
  PSafeLockReadWrite mutex(*this);
  if (!mutex.IsLocked())
    return true;

  switch (state) {
    case Subscribed :
      SendRequest(Unsubscribing);
    case Unsubscribing :
      return transactions.IsEmpty();

    default :
      break;
  }

  for (PSafePtr<SIPTransaction> transaction(transactions, PSafeReference); transaction != NULL; ++transaction)
    transaction->Abort();

  return true;
}


void SIPHandler::SetState(SIPHandler::State newState) 
{
  PTRACE(4, "SIP\tChanging " << GetMethod() << " handler from " << state << " to " << newState
         << ", target=" << GetAddressOfRecord() << ", id=" << GetCallID());
  state = newState;
}


OpalTransport * SIPHandler::GetTransport()
{
  if (m_transport != NULL) {
    if (m_transport->IsOpen())
      return m_transport;

    m_transport->CloseWait();
    delete m_transport;
    m_transport = NULL;
  }

  if (m_proxy.IsEmpty()) {
    // Look for a "proxy" parameter to override default proxy
    const PStringToString & params = m_addressOfRecord.GetParamVars();
    if (params.Contains("proxy")) {
      m_proxy.Parse(params("proxy"));
      m_addressOfRecord.SetParamVar("proxy", PString::Empty());
    }
  }

  if (m_proxy.IsEmpty())
    m_proxy = endpoint.GetProxy();

  // Must specify a network interface or get infinite recursion
  m_transport = endpoint.CreateTransport(m_proxy.IsEmpty() ? GetAddressOfRecord() : m_proxy, "*");
  return m_transport;
}


void SIPHandler::SetExpire(int e)
{
  expire = e;
  PTRACE(3, "SIP\tExpiry time for " << GetMethod() << " set to " << expire << " seconds.");

  // Only modify the originalExpire for future requests if IntervalTooBrief gives
  // a bigger expire time. expire itself will always reflect the proxy decision
  // (bigger or lower), but originalExpire determines what is used in future 
  // requests and is only modified if interval too brief
  if (originalExpire < e)
    originalExpire = e;

  // retry before the expire time.
  // if the expire time is more than 20 mins, retry 10mins before expiry
  // if the expire time is less than 20 mins, retry after half of the expiry time
  if (expire > 0 && state < Unsubscribing)
    expireTimer.SetInterval(0, (unsigned)(expire < 20*60 ? expire/2 : expire-10*60));
}


PBoolean SIPHandler::WriteSIPHandler(OpalTransport & transport, void * param)
{
  return param != NULL && ((SIPHandler *)param)->WriteSIPHandler(transport);
}


bool SIPHandler::WriteSIPHandler(OpalTransport & transport)
{
  SIPTransaction * transaction = CreateTransaction(transport);

  if (transaction != NULL) {
    for (PINDEX i = 0; i < m_mime.GetSize(); ++i) 
      transaction->GetMIME().SetAt(m_mime.GetKeyAt(i), PString(m_mime.GetDataAt(i)));
    if (state == Unsubscribing)
      transaction->GetMIME().SetExpires(0);
    if (authentication != NULL)
      authentication->Authorise(*transaction); // If already have info from last time, use it!
    if (transaction->Start()) {
      transactions.Append(transaction);
      return true;
    }
  }

  PTRACE(2, "SIP\tDid not start transaction on " << transport);
  return false;
}


bool SIPHandler::ActivateState(SIPHandler::State newState, unsigned msecs)
{
  PTimer timer(msecs);
  for (;;) {
    {
      PSafeLockReadWrite mutex(*this);
      if (!mutex.IsLocked())
        return false;

      if (SendRequest(newState))
        return true;
    }

    if (!timer.IsRunning())
      return false;

    PThread::Sleep(100);
  }
}


PBoolean SIPHandler::SendRequest(SIPHandler::State newState)
{
  expireTimer.Stop(false); // Stop automatic retry

  if (expire == 0)
    newState = Unsubscribing;

  switch (newState) {
    case Unsubscribing:
      switch (state) {
        case Subscribed :
        case Unavailable :
          break;  // Can try and do Unsubscribe

        case Subscribing :
        case Refreshing :
        case Restoring :
          PTRACE(2, "SIP\tCan't send " << newState << " request for " << GetMethod()
                 << " handler while in " << state << " state, target="
                 << GetAddressOfRecord() << ", id=" << GetCallID());
          return false; // Are in the process of doing something

        case Unsubscribed :
        case Unsubscribing:
          PTRACE(3, "SIP\tAlready doing " << state << " request for " << GetMethod()
                 << " handler, target=" << GetAddressOfRecord() << ", id=" << GetCallID());
          return true;  // Already done or doing it

        default :
          PAssertAlways(PInvalidParameter);
          return false;
      }
      break;

    case Subscribing :
    case Refreshing :
    case Restoring :
      switch (state) {
        case Subscribed :
        case Unavailable :
          break;  // Can do subscribe/refresh/restore

        case Refreshing :
        case Restoring :
          PTRACE(3, "SIP\tAlready doing " << state << " request for " << GetMethod()
                 << " handler, target=" << GetAddressOfRecord() << ", id=" << GetCallID());
          return true; // Already doing it

        case Subscribing :
        case Unsubscribing :
        case Unsubscribed :
          PTRACE(2, "SIP\tCan't send " << newState << " request for " << GetMethod()
                 << " handler while in " << state << " state, target="
                 << GetAddressOfRecord() << ", id=" << GetCallID());
          return false; // Can't restart as are on the way out

        default : // Are in the process of doing something
          PAssertAlways(PInvalidParameter);
          return false;
      }
      break;

    default :
      PAssertAlways(PInvalidParameter);
      return false;
  }

  SetState(newState);

  if (GetTransport() != NULL) {
    // Restoring or first time, try every interface
    if (newState == Restoring || m_transport->GetInterface().IsEmpty()) {
      PWaitAndSignal mutex(m_transport->GetWriteMutex());
      if (m_transport->WriteConnect(WriteSIPHandler, this))
        return true;
    }
    else {
      // We contacted the server on an interface last time, assume it still works!
      if (WriteSIPHandler(*m_transport))
        return true;
    }

    OnFailed(SIP_PDU::Local_TransportError);
  }

  if (newState == Unsubscribing) {
    // Transport level error, probably never going to get the unsubscribe through
    SetState(Unsubscribed);
    return true;
  }

  PTRACE(4, "SIP\tRetrying " << GetMethod() << " in " << offlineExpire << " seconds.");
  OnFailed(SIP_PDU::Local_BadTransportAddress);
  expireTimer.SetInterval(0, offlineExpire); // Keep trying to get it back
  SetState(Unavailable);
  return true;
}


PBoolean SIPHandler::OnReceivedNOTIFY(SIP_PDU & /*response*/)
{
  return PFalse;
}


void SIPHandler::OnReceivedAuthenticationRequired(SIPTransaction & transaction, SIP_PDU & response)
{
  bool isProxy = response.GetStatusCode() == SIP_PDU::Failure_ProxyAuthenticationRequired;

#if PTRACING
  const char * proxyTrace = isProxy ? "Proxy " : "";
#endif
  PTRACE(3, "SIP\tReceived " << proxyTrace << "Authentication Required response");
  
  // Abort after some unsuccesful authentication attempts. This is required since
  // some implementations return "401 Unauthorized" with a different nonce at every
  // time.
  if (authenticationAttempts >= 10) {
    PTRACE(1, "SIP\tAborting after " << authenticationAttempts << " attempts to REGISTER/SUBSCRIBE");
    OnFailed(SIP_PDU::Failure_UnAuthorised);
    return;
  }
  ++authenticationAttempts;

  // authenticate 
  PString errorMsg;
  SIPAuthentication * newAuth = SIPAuthentication::ParseAuthenticationRequired(isProxy, 
                                                                               response.GetMIME()(isProxy ? "Proxy-Authenticate" : "WWW-Authenticate"),
                                                                               errorMsg);
  if (newAuth == NULL) {
    PTRACE(2, "SIP\t" << errorMsg);
    OnFailed(SIP_PDU::Failure_Forbidden);
    return;
  }

  // Try to find authentication parameters for the given realm,
  // if not, use the proxy authentication parameters (if any)
  PString authRealm = m_realm;
  PString username  = m_username;
  PString password  = m_password;
  if (endpoint.GetAuthentication(newAuth->GetAuthRealm(), authRealm, username, password)) {
    PTRACE (3, "SIP\tFound auth info for realm " << newAuth->GetAuthRealm());
  }
  else if (username.IsEmpty()) {
    const SIPURL & proxy = endpoint.GetProxy();
    if (!proxy.IsEmpty()) {
      PTRACE (3, "SIP\tNo auth info for realm " << newAuth->GetAuthRealm() << ", using proxy auth");
      username = proxy.GetUserName();
      password = proxy.GetPassword();
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

  // switch authentication schemes
  delete authentication;
  authentication = newAuth;
  m_realm    = newAuth->GetAuthRealm();
  m_username = username;
  m_password = password;

  // And end connect mode on the transport
  CollapseFork(transaction);

  // Restart the transaction with new authentication handler
  State oldState = state;
  state = Unavailable;
  SendRequest(oldState);
}


void SIPHandler::OnReceivedOK(SIPTransaction & transaction, SIP_PDU & response)
{
  response.GetMIME().GetProductInfo(m_productInfo);

  switch (GetState()) {
    case Unsubscribing :
      SetState(Unsubscribed);
      break;

    case Subscribing :
    case Refreshing :
    case Restoring :
      if (expire == 0)
        SetState(Unsubscribed);
      else
        SetState(Subscribed);
      break;

    default :
      PTRACE(2, "SIP\tUnexpected 200 OK in handler with state " << state);
  }

  // reset the number of unsuccesful authentication attempts
  authenticationAttempts = 0;

  CollapseFork(transaction);
}


void SIPHandler::CollapseFork(SIPTransaction & transaction)
{
  // Take this transaction out
  transactions.Remove(&transaction);

  // And kill all the rest
  PSafePtr<SIPTransaction> transToGo;
  while ((transToGo = transactions.GetAt(0)) != NULL) {
    transactions.Remove(transToGo);
    transToGo->Abort();
  }

  // Finally end connect mode on the transport
  m_transport->SetInterface(transaction.GetInterface());
}


void SIPHandler::OnTransactionFailed(SIPTransaction & transaction)
{
  if (transactions.Remove(&transaction)) {
    OnFailed(transaction.GetStatusCode());

    if (expire > 0 && !transaction.IsCanceled()) {
      PTRACE(4, "SIP\tRetrying " << GetMethod() << " in " << offlineExpire << " seconds.");
      expireTimer.SetInterval(0, offlineExpire); // Keep trying to get it back
    }
  }
}


void SIPHandler::OnFailed(const SIP_PDU & response)
{
  OnFailed(response.GetStatusCode());
}


void SIPHandler::OnFailed(SIP_PDU::StatusCodes code)
{
  switch (code) {
    case SIP_PDU::Failure_UnAuthorised :
    case SIP_PDU::Failure_ProxyAuthenticationRequired :
      break;

    case SIP_PDU::Local_TransportError :
    case SIP_PDU::Failure_RequestTimeout :
    case SIP_PDU::Local_BadTransportAddress :
      SetState(Unavailable);
      break;

    default :
      PTRACE(4, "SIP\tNot retrying " << GetMethod() << " due to error response " << code);
      expire = 0; // OK, stop trying
      expireTimer.Stop(false);
      SetState(Unsubscribed);
      ShutDown();
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
  : SIPHandler(endpoint,
               params.m_addressOfRecord,
               params.m_registrarAddress,
               params.m_expire,
               params.m_restoreTime, params.m_minRetryTime, params.m_maxRetryTime)
  , m_parameters(params)
  , m_sequenceNumber(0)
{
  // Put adjusted values back
  SIPURL aor = GetAddressOfRecord();
  aor.SetTag();
  m_parameters.m_addressOfRecord = aor.AsQuotedString();
  m_parameters.m_registrarAddress = m_remoteAddress.AsQuotedString();
  m_parameters.m_expire = expire;

  m_username = params.m_authID;
  m_password = params.m_password;
  m_realm    = params.m_realm;

  if (m_username.IsEmpty())
    m_username = m_addressOfRecord.GetUserName();
}


SIPRegisterHandler::~SIPRegisterHandler()
{
  PTRACE(4, "SIP\tDeleting SIPRegisterHandler " << GetAddressOfRecord());
}


SIPTransaction * SIPRegisterHandler::CreateTransaction(OpalTransport & trans)
{
  SIPRegister::Params params = m_parameters;

  if (expire == 0 || GetState() == Unsubscribing) {
    params.m_expire = 0;
    params.m_contactAddress = "*";
  }
  else {
    params.m_expire = expire;

    if (params.m_contactAddress.IsEmpty() || (params.m_contactAddress *= "%LIMITED")) {
      /* If nothing explicit, put into the contact field all the bound
         interfaces. If special contact name then we only put into the contact
         listeners that are on the same interface. If translated by STUN then
         only the external address of the interface is used. */
      OpalTransportAddress localAddress;
      if (!params.m_contactAddress.IsEmpty()) {
        localAddress = trans.GetLocalAddress();
        params.m_contactAddress.MakeEmpty();
      }

      unsigned qvalue = 1000;
      PString userName = SIPURL(params.m_addressOfRecord).GetUserName();
      OpalTransportAddressArray interfaces = endpoint.GetInterfaceAddresses(true, &trans);
      for (PINDEX i = 0; i < interfaces.GetSize(); ++i) {
        if (localAddress.IsEmpty() || localAddress.IsEquivalent(interfaces[i], true)) {
          if (!params.m_contactAddress.IsEmpty())
            params.m_contactAddress += ", ";
          SIPURL contact(userName, interfaces[i]);
          contact.Sanitise(SIPURL::ContactURI);
          params.m_contactAddress += contact.AsQuotedString();
          params.m_contactAddress.sprintf(qvalue < 1000 ? ";q=0.%03u" : ";q=1", qvalue);
          qvalue -= 1000/interfaces.GetSize();
        }
      }
    }
    else {
      // Sanitise the contact address URI provided
      SIPURL contact(params.m_contactAddress);
      contact.Sanitise(SIPURL::ContactURI);
      params.m_contactAddress = contact.AsQuotedString();
    }
  }

  return new SIPRegister(endpoint, trans, m_proxy, GetCallID(), m_sequenceNumber, params);
}


void SIPRegisterHandler::OnReceivedOK(SIPTransaction & transaction, SIP_PDU & response)
{
  State oldState = GetState();

  SIPHandler::OnReceivedOK(transaction, response);

  std::list<SIPURL> requestContacts, replyContacts;
  transaction.GetMIME().GetContacts(requestContacts);
  response.GetMIME().GetContacts(replyContacts);

  for (std::list<SIPURL>::iterator request = requestContacts.begin(); request != requestContacts.end(); ++request) {
    for (std::list<SIPURL>::iterator reply = replyContacts.begin(); reply != replyContacts.end(); ++reply) {
      if (*request == *reply) {
        PString expires = SIPMIMEInfo::ExtractFieldParameter(reply->GetFieldParameters(), "expires");
        if (expires.IsEmpty())
          SetExpire(response.GetMIME().GetExpires(endpoint.GetRegistrarTimeToLive().GetSeconds()));
        else
          SetExpire(expires.AsUnsigned());
      }
    }
  }

  response.GetMIME().GetProductInfo(m_productInfo);

  SendStatus(SIP_PDU::Successful_OK, oldState);
}


void SIPRegisterHandler::OnFailed(SIP_PDU::StatusCodes r)
{
  SendStatus(r, GetState());
  SIPHandler::OnFailed(r);
}


PBoolean SIPRegisterHandler::SendRequest(SIPHandler::State s)
{
  SendStatus(SIP_PDU::Information_Trying, GetState());
  m_sequenceNumber = endpoint.GetNextCSeq();
  return SIPHandler::SendRequest(s);
}


void SIPRegisterHandler::SendStatus(SIP_PDU::StatusCodes code, State state)
{
  SIPEndPoint::RegistrationStatus status;
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
  }

  endpoint.OnRegistrationStatus(status);
}


void SIPRegisterHandler::UpdateParameters(const SIPRegister::Params & params)
{
  if (!params.m_authID.IsEmpty())
    m_username = params.m_authID;   // Adjust the authUser if required 
  if (!params.m_realm.IsEmpty())
    m_realm = params.m_realm;   // Adjust the realm if required 
  if (!params.m_password.IsEmpty())
    m_password = params.m_password; // Adjust the password if required 

  if (params.m_expire > 0)
    SetExpire(params.m_expire);
}


/////////////////////////////////////////////////////////////////////////

SIPSubscribeHandler::SIPSubscribeHandler(SIPEndPoint & endpoint, const SIPSubscribe::Params & params)
  : SIPHandler(endpoint,
               params.m_addressOfRecord,
               params.m_agentAddress,
               params.m_expire,
               params.m_restoreTime, params.m_minRetryTime, params.m_maxRetryTime)
  , m_parameters(params)
  , m_unconfirmed(true)
  , m_packageHandler(SIPEventPackageFactory::CreateInstance(params.m_eventPackage))
{
  // Put possibly adjusted value back
  m_parameters.m_addressOfRecord = GetAddressOfRecord().AsString();
  m_parameters.m_expire = expire;

  m_dialog.SetRemoteURI(m_addressOfRecord);
  if (!params.m_eventPackage.IsWatcher()) {
    m_dialog.SetRequestURI(m_remoteAddress);
  }
  else {
    m_dialog.SetRequestURI(m_addressOfRecord);
    m_parameters.m_from = m_addressOfRecord.AsString();
  }

  callID = m_dialog.GetCallID();

  m_username = params.m_authID;
  m_password = params.m_password;
  m_realm    = params.m_realm;
}


SIPSubscribeHandler::~SIPSubscribeHandler()
{
  PTRACE(4, "SIP\tDeleting SIPSubscribeHandler " << GetAddressOfRecord());
  delete m_packageHandler;
}


SIPTransaction * SIPSubscribeHandler::CreateTransaction(OpalTransport &trans)
{ 
  // Default routeSet if there is a proxy
  m_dialog.UpdateRouteSet(m_proxy);

  if (!m_dialog.IsEstablished()) {
    if (!m_parameters.m_from.IsEmpty())
      m_dialog.SetLocalURI(m_parameters.m_from);
    else
      m_dialog.SetLocalURI(endpoint.GetRegisteredPartyName(GetAddressOfRecord(), *m_transport));
  }

  m_parameters.m_expire = state != Unsubscribing ? expire : 0;
  return new SIPSubscribe(endpoint, trans, m_dialog, m_parameters);
}


void SIPSubscribeHandler::OnFailed(const SIP_PDU & response)
{
  SIP_PDU::StatusCodes r = response.GetStatusCode();

  SendStatus(r, GetState());

  int newExpires = 0;
  PString dummy;
  
  switch (r) {
    case SIP_PDU::Failure_TransactionDoesNotExist:
      // Resubscribe as previous subscription totally lost, but dialog processing
      // may have altered the target so restore the original target address
      m_parameters.m_addressOfRecord = GetAddressOfRecord().AsString();
      endpoint.Subscribe(m_parameters, dummy);
      break;

    case SIP_PDU::Failure_IntervalTooBrief:
      // Resubscribe with altered expiry
      newExpires = response.GetMIME().GetExpires();
      if (newExpires > 0) {
        m_parameters.m_expire = newExpires;
        PString dummy;
        endpoint.Subscribe(m_parameters, dummy);
      }
      break;

    default:
      // fall through
      SIPHandler::OnFailed(r);
  }
}


PBoolean SIPSubscribeHandler::SendRequest(SIPHandler::State s)
{
  SendStatus(SIP_PDU::Information_Trying, GetState());
  return SIPHandler::SendRequest(s);
}


void SIPSubscribeHandler::SendStatus(SIP_PDU::StatusCodes code, State state)
{
  switch (state) {
    case Subscribing :
      endpoint.OnSubscriptionStatus(*this, GetAddressOfRecord(), true, false, code);
      break;

    case Subscribed :
      if (m_unconfirmed)
        endpoint.OnSubscriptionStatus(*this, GetAddressOfRecord(), true, false, code);
      // Do next state

    case Refreshing :
      endpoint.OnSubscriptionStatus(*this, GetAddressOfRecord(), true, true, code);
      break;

    case Unsubscribed :
    case Unavailable :
    case Restoring :
      endpoint.OnSubscriptionStatus(*this, GetAddressOfRecord(), true, code/100 != 2, code);
      break;

    case Unsubscribing :
      endpoint.OnSubscriptionStatus(*this, GetAddressOfRecord(), false, false, code);
      break;
  }
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

  if (params.m_expire > 0)
    SetExpire(params.m_expire);
}


void SIPSubscribeHandler::OnReceivedOK(SIPTransaction & transaction, SIP_PDU & response)
{
  /* An "expire" parameter in the Contact header has no semantics
   * for SUBSCRIBE. RFC3265, 3.1.1.
   * An answer can only shorten the expires time.
   */
  SetExpire(response.GetMIME().GetExpires(originalExpire));

  SIPHandler::OnReceivedOK(transaction, response);

  m_dialog.Update(response);

  response.GetMIME().GetProductInfo(m_productInfo);

  if (GetState() == Unsubscribed)
    SendStatus(SIP_PDU::Successful_OK, Unsubscribing);
}


PBoolean SIPSubscribeHandler::OnReceivedNOTIFY(SIP_PDU & request)
{
  if (PAssertNULL(m_transport) == NULL)
    return false;

  if (m_unconfirmed) {
    SendStatus(SIP_PDU::Successful_OK, GetState());
    m_unconfirmed = false;
  }

  // If we received a NOTIFY before
  if (m_dialog.IsDuplicateCSeq(request.GetMIME().GetCSeqIndex())) {
    PTRACE(3, "SIP\tReceived duplicate NOTIFY");
    return request.SendResponse(*m_transport, SIP_PDU::Successful_OK, &endpoint);
  }

  // We received a NOTIFY corresponding to an active SUBSCRIBE
  // for which we have just unSUBSCRIBEd. That is the final NOTIFY.
  // We can remove the SUBSCRIBE from the list.
  PTRACE_IF(3, GetState() != SIPHandler::Subscribed && expire == 0, "SIP\tFinal NOTIFY received");

  PString state = request.GetMIME().GetSubscriptionState();

  // Check the susbscription state
  if (state.Find("terminated") != P_MAX_INDEX) {
    PTRACE(3, "SIP\tSubscription is terminated");
    ShutDown();
  }
  else if (state.Find("active") != P_MAX_INDEX || state.Find("pending") != P_MAX_INDEX) {

    PTRACE(3, "SIP\tSubscription is " << state);
    PString expire = SIPMIMEInfo::ExtractFieldParameter(state, "expire");
    if (!expire.IsEmpty())
      SetExpire(expire.AsUnsigned());
  }

  if (m_packageHandler == NULL)
    request.SendResponse(*m_transport, SIP_PDU::Failure_BadEvent, &endpoint);
  else if (m_packageHandler->OnReceivedNOTIFY(*this, request))
    request.SendResponse(*m_transport, SIP_PDU::Successful_OK, &endpoint);
  else
    request.SendResponse(*m_transport, SIP_PDU::Failure_BadRequest, &endpoint);
  return true;
}


class SIPMwiEventPackageHandler : public SIPEventPackageHandler
{
  virtual PCaselessString GetContentType() const
  {
    return "application/simple-message-summary";
  }

  virtual bool OnReceivedNOTIFY(SIPHandler & handler, SIP_PDU & request)
  {
    PString body = request.GetEntityBody();
    if (body.IsEmpty ())
      return true;

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
    PString msgs;
    PStringArray bodylines = body.Lines ();
    for (PINDEX z = 0 ; z < PARRAYSIZE(validMessageClasses); z++) {

      for (int i = 0 ; i < bodylines.GetSize () ; i++) {

        PCaselessString line (bodylines [i]);
        PINDEX j = line.FindLast(validMessageClasses[z].name);
        if (j != P_MAX_INDEX) {
          line.Replace(validMessageClasses[z].name, "");
          line.Replace (":", "");
          msgs = line.Trim ();
          handler.GetEndPoint().OnMWIReceived(handler.GetAddressOfRecord().AsString(), validMessageClasses[z].type, msgs);
          return true;
        }
      }
    }

    // Received MWI, unknown messages number
    handler.GetEndPoint().OnMWIReceived(handler.GetAddressOfRecord().AsString(), OpalManager::NumMessageWaitingTypes, "1/0");

    return true;
  }
};

static SIPEventPackageFactory::Worker<SIPMwiEventPackageHandler> mwiEventPackageHandler(SIPSubscribe::MessageSummary);

#if OPAL_PTLIB_EXPAT

class SIPPresenceEventPackageHandler : public SIPEventPackageHandler
{
  virtual PCaselessString GetContentType() const
  {
    return "application/pidf+xml";
  }

  virtual bool OnReceivedNOTIFY(SIPHandler & handler, SIP_PDU & request)
  {
    SIPURL from = request.GetMIME().GetFrom();
    from.Sanitise(SIPURL::ExternalURI);

    SIPURL to = request.GetMIME().GetTo();
    to.Sanitise(SIPURL::ExternalURI);

    SIPPresenceInfo info;
    info.m_address = from.AsQuotedString();
    info.m_to      = to.AsQuotedString();

    // Check for empty body, if so then is OK, just a ping ...
    if (request.GetEntityBody().IsEmpty()) {
      info.m_basic = SIPPresenceInfo::Unchanged;
      handler.GetEndPoint().OnPresenceInfoReceived(info);
      return true;
    }

    PXML xml;
    if (!xml.Load(request.GetEntityBody()))
      return false;

    PXMLElement * rootElement = xml.GetRootElement();
    if (rootElement == NULL || rootElement->GetName() != "presence")
      return false;

    {
      PXMLElement * tupleElement = rootElement->GetElement("tuple");
      if (tupleElement == NULL)
        return false;

      {
        PXMLElement * statusElement = tupleElement->GetElement("status");
        if (statusElement == NULL)
          return false;
        {
          PXMLElement * basicElement = statusElement->GetElement("basic");
          if (basicElement != NULL) {
            PCaselessString value = basicElement->GetData();
            if (value == "open")
              info.m_basic = SIPPresenceInfo::Open;
            else if (value == "closed")
              info.m_basic = SIPPresenceInfo::Closed;
          }
        }
        {
          PXMLElement * noteElement = statusElement->GetElement("note");
          if (!noteElement)
            noteElement = rootElement->GetElement("note");
          if (!noteElement)
            noteElement = tupleElement->GetElement("note");
          if (noteElement)
            info.m_note = noteElement->GetData();
        }
      }
      {
        PXMLElement * contactElement = tupleElement->GetElement("contact");
        if (contactElement != NULL)
          info.m_contact = contactElement->GetData();
      }
    }
    {
      info.m_activity = SIPPresenceInfo::UnknownExtended;
      PXMLElement * personElement = rootElement->GetElement("dm:person");
      if (personElement != NULL) {
        PXMLElement * activitiesElement = personElement->GetElement("rpid:activities");
        if (activitiesElement != NULL) {
          PINDEX index = 0;
          PXMLObject * activity = activitiesElement->GetElement(index);
          while (activity != NULL) {
            if (activity->IsElement()) {
              PXMLElement * activityElement = (PXMLElement *)activity;
              PString state = activityElement->GetName();
              if (state.Left(5) *= "rpid:") 
                state = state.Mid(5);

              if (state *= "working")
                info.m_activity = SIPPresenceInfo::Working;
              else if (state *= "on-the-phone")
                info.m_activity = SIPPresenceInfo::OnThePhone;
              else if (state *= "busy")
                info.m_activity = SIPPresenceInfo::Busy;
              else if (state *= "away")
                info.m_activity = SIPPresenceInfo::Away;
              else if (state *= "vacation")
                info.m_activity = SIPPresenceInfo::Vacation;
              else if (state *= "holiday")
                info.m_activity = SIPPresenceInfo::Holiday;

              info.m_activities.AppendString(state);
            }
            activity = activitiesElement->GetElement(++index);
          }
        }
      }
    }

    handler.GetEndPoint().OnPresenceInfoReceived(info);
    return true;
  }
};

static SIPEventPackageFactory::Worker<SIPPresenceEventPackageHandler> presenceEventPackageHandler(SIPSubscribe::Presence);


class SIPPresenceURL : public PURLLegacyScheme
{
  public:
    SIPPresenceURL()
      : PURLLegacyScheme("pres", true, false, true, false, false, false, false, false, false, false, 5060)
    {
    }
};

static PFactory<PURLScheme>::Worker<SIPPresenceURL> presenceURL("pres", true);

///////////////////////////////////////////////////////////////////////////////

class SIPPresenceWatcherEventPackageHandler : public SIPEventPackageHandler
{
  virtual PCaselessString GetContentType() const
  {
    return "application/watcherinfo+xml";
  }

  virtual bool OnReceivedNOTIFY(SIPHandler & handler, SIP_PDU & request)
  {
#if 0
    SIPURL from = request.GetMIME().GetFrom();
    from.Sanitise(SIPURL::ExternalURI);

    SIPURL to = request.GetMIME().GetTo();
    to.Sanitise(SIPURL::ExternalURI);

    SIPPresenceInfo info;
    info.m_address = from.AsQuotedString();
    info.m_to      = to.AsQuotedString();

    // Check for empty body, if so then is OK, just a ping ...
    if (request.GetEntityBody().IsEmpty()) {
      info.m_basic = SIPPresenceInfo::Unchanged;
      handler.GetEndPoint().OnPresenceInfoReceived(info);
      return true;
    }

    PXML xml;
    if (!xml.Load(request.GetEntityBody()))
      return false;

    PXMLElement * rootElement = xml.GetRootElement();
    if (rootElement == NULL || rootElement->GetName() != "presence")
      return false;

    {
      PXMLElement * tupleElement = rootElement->GetElement("tuple");
      if (tupleElement == NULL)
        return false;

      {
        PXMLElement * statusElement = tupleElement->GetElement("status");
        if (statusElement == NULL)
          return false;
        {
          PXMLElement * basicElement = statusElement->GetElement("basic");
          if (basicElement != NULL) {
            PCaselessString value = basicElement->GetData();
            if (value == "open")
              info.m_basic = SIPPresenceInfo::Open;
            else if (value == "closed")
              info.m_basic = SIPPresenceInfo::Closed;
          }
        }
        {
          PXMLElement * noteElement = statusElement->GetElement("note");
          if (!noteElement)
            noteElement = rootElement->GetElement("note");
          if (!noteElement)
            noteElement = tupleElement->GetElement("note");
          if (noteElement)
            info.m_note = noteElement->GetData();
        }
      }
      {
        PXMLElement * contactElement = tupleElement->GetElement("contact");
        if (contactElement != NULL)
          info.m_contact = contactElement->GetData();
      }
    }
    {
      info.m_activity = SIPPresenceInfo::UnknownExtended;
      PXMLElement * personElement = rootElement->GetElement("dm:person");
      if (personElement != NULL) {
        PXMLElement * activitiesElement = personElement->GetElement("rpid:activities");
        if (activitiesElement != NULL) {
          PINDEX index = 0;
          PXMLObject * activity = activitiesElement->GetElement(index);
          while (activity != NULL) {
            if (activity->IsElement()) {
              PXMLElement * activityElement = (PXMLElement *)activity;
              PString state = activityElement->GetName();
              if (state.Left(5) *= "rpid:") 
                state = state.Mid(5);

              if (state *= "working")
                info.m_activity = SIPPresenceInfo::Working;
              else if (state *= "on-the-phone")
                info.m_activity = SIPPresenceInfo::OnThePhone;
              else if (state *= "busy")
                info.m_activity = SIPPresenceInfo::Busy;
              else if (state *= "away")
                info.m_activity = SIPPresenceInfo::Away;
              else if (state *= "vacation")
                info.m_activity = SIPPresenceInfo::Vacation;
              else if (state *= "holiday")
                info.m_activity = SIPPresenceInfo::Holiday;

              info.m_activities.AppendString(state);
            }
            activity = activitiesElement->GetElement(++index);
          }
        }
      }
    }

    handler.GetEndPoint().OnPresenceInfoReceived(info);
#endif
    return true;
  }
};

static SIPEventPackageFactory::Worker<SIPPresenceWatcherEventPackageHandler> presenceWatcherEventPackageHandler(SIPSubscribe::Presence | SIPSubscribe::Watcher);

///////////////////////////////////////////////////////////////////////////////


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

  virtual bool OnReceivedNOTIFY(SIPHandler & handler, SIP_PDU & request)
  {
    // Check for empty body, if so then is OK, just a ping ...
    if (request.GetEntityBody().IsEmpty())
      return true;

    PXML xml;
    if (!xml.Load(request.GetEntityBody()))
      return false;

    PXMLElement * rootElement = xml.GetRootElement();
    if (rootElement == NULL || rootElement->GetName() != "dialog-info")
      return false;

    SIPDialogNotification info(rootElement->GetAttribute("entity"));
    if (info.m_entity.IsEmpty())
      return false;

    PINDEX index = 0;
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
      handler.GetEndPoint().OnDialogInfoReceived(info);
      index++;
    }

    if (index == 0)
      handler.GetEndPoint().OnDialogInfoReceived(info);
    return true;
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

#endif // P_EXPAT


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

/////////////////////////////////////////////////////////////////////////

SIPNotifyHandler::SIPNotifyHandler(SIPEndPoint & endpoint,
                                   const PString & targetAddress,
                                   const SIPEventPackage & eventPackage,
                                   const SIPDialogContext & dialog)
  : SIPHandler(endpoint, targetAddress, dialog.GetRemoteURI().AsString())
  , m_eventPackage(eventPackage)
  , m_dialog(dialog)
  , m_reason(Deactivated)
  , m_packageHandler(SIPEventPackageFactory::CreateInstance(eventPackage))
{
  callID = m_dialog.GetCallID();
}


SIPNotifyHandler::~SIPNotifyHandler()
{
  delete m_packageHandler;
}


SIPTransaction * SIPNotifyHandler::CreateTransaction(OpalTransport & trans)
{
  PString state;
  if (expire > 0 && GetState() != Unsubscribing)
    state.sprintf("active;expires=%u", expire);
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

  return new SIPNotify(endpoint, trans, m_dialog, m_eventPackage, state, body);
}


PBoolean SIPNotifyHandler::SendRequest(SIPHandler::State state)
{
  // If times out, i.e. Refreshing, then this is actually a time out unsubscribe.
  if (state == Refreshing)
    m_reason = Timeout;

  return SIPHandler::SendRequest(state == Refreshing ? Unsubscribing : state);
}


bool SIPNotifyHandler::SendNotify(const PObject * body)
{
  if (!LockReadWrite())
    return false;

  if (m_packageHandler != NULL)
    SetBody(m_packageHandler->OnSendNOTIFY(*this, body));
  else if (body == NULL)
    SetBody(PString::Empty());
  else {
    PStringStream str;
    str << *body;
    SetBody(str);
  }

  UnlockReadWrite();

  return ActivateState(Subscribing, endpoint.GetNonInviteTimeout().GetInterval());
}


/////////////////////////////////////////////////////////////////////////

SIPPublishHandler::SIPPublishHandler(SIPEndPoint & endpoint,
                                     const SIPSubscribe::Params & params,
                                     const PString & b)
  : SIPHandler(endpoint,
               params.m_addressOfRecord,
               params.m_agentAddress,
               params.m_expire,
               params.m_restoreTime, params.m_minRetryTime, params.m_maxRetryTime)
  , m_parameters(params)
  , m_stateChanged(false)
{
  // Put possibly adjusted value back
  m_parameters.m_addressOfRecord = GetAddressOfRecord().AsString();
  m_parameters.m_expire = expire;

  m_username = params.m_authID;
  m_password = params.m_password;
  m_realm    = params.m_realm;

  body = b;
}


SIPPublishHandler::~SIPPublishHandler()
{
  PTRACE(4, "SIP\tDeleting SIPPublishHandler " << GetAddressOfRecord());
}


SIPTransaction * SIPPublishHandler::CreateTransaction(OpalTransport & t)
{
  SetExpire(originalExpire);
  return new SIPPublish(endpoint,
                        t, 
                        GetCallID(),
                        m_sipETag,
                        m_parameters,
                        (GetState() == Refreshing) ? PString::Empty() : body);
}


void SIPPublishHandler::OnReceivedOK(SIPTransaction & transaction, SIP_PDU & response)
{
  if (!response.GetMIME().GetSIPETag().IsEmpty())
    m_sipETag = response.GetMIME().GetSIPETag();

  SetExpire(response.GetMIME().GetExpires(originalExpire));

  SIPHandler::OnReceivedOK(transaction, response);
}


void SIPPublishHandler::SetBody(const PString & b)
{
  m_stateChanged = true;
  SIPHandler::SetBody(b);
}


void SIPPresenceInfo::PrintOn(ostream & strm) const
{
  if (m_address.IsEmpty())
    return;

  if (m_activities.GetSize() > 0)
    strm << setfill(',') << m_activities << setfill(' ');
  else {
    switch (m_basic) {
      case SIPPresenceInfo::Open :
        if (m_note.IsEmpty())
          strm << "Open";
        else
          strm << m_note;
        break;

      case SIPPresenceInfo::Closed :
        strm << "Closed";
        break;

      case SIPPresenceInfo::Unknown :
      default:
        strm << "Unknown";
    }
  }
}


PString SIPPresenceInfo::AsXML() const
{
  if (m_address.IsEmpty())
    return PString::Empty();

  PStringStream xml;

  xml << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n"
    "<impp:presence xmlns:impp=\"urn:ietf:params:xml:ns:pidf\" entity=\"";

  PCaselessString entity = m_entity;
  if (entity.IsEmpty()) 
    entity = m_address;

#if 1
  if (entity.NumCompare("sip:") == PObject::EqualTo)
    entity.Delete(0, 4);
  if (entity.NumCompare("pres:") != PObject::EqualTo)
    entity = "pres:" + entity;
#else
  if (entity.NumCompare("sip:") != PObject::EqualTo)
    entity = "sip:" + entity;
#endif

  xml << entity;

  xml << "\">\r\n"
    "  <impp:tuple id=\"id_" << OpalGloballyUniqueID() << "\">\r\n";

  xml << "    <impp:status>\r\n";
  switch (m_basic) {
    case Open :
      xml << "      <impp:basic>open</impp:basic>\r\n";
      break;

    case Closed :
    default:
      xml << "      <impp:basic>closed</impp:basic>\r\n";
      break;
  }
  xml << "    </impp:status>\r\n"

  //if (!m_note.IsEmpty())
  //  xml << "    <impp:note xml:lang=\"en\">" << m_note << "</impp:note>\r\n";

  //       "    <contact priority=\"1\">" << (m_contact.IsEmpty() ? m_address : m_contact) << "</contact>\r\n"
         "  </impp:tuple>\r\n"
         "</impp:presence>\r\n";

  return xml;
}


/////////////////////////////////////////////////////////////////////////

SIPMessageHandler::SIPMessageHandler (SIPEndPoint & endpoint, const PString & to, const PString & b, const PString & c, const PString & id)
  : SIPHandler(endpoint, to, c)
{
  body   = b;
  callID = id;
  SetState(Subscribed);
}


SIPMessageHandler::~SIPMessageHandler ()
{
  PTRACE(4, "SIP\tDeleting SIPMessageHandler " << GetAddressOfRecord());
}


SIPTransaction * SIPMessageHandler::CreateTransaction(OpalTransport & transport)
{ 
  SetExpire(originalExpire);
  return new SIPMessage(endpoint, transport, m_proxy, GetAddressOfRecord(), GetCallID(), body, m_localAddress);
}


void SIPMessageHandler::OnFailed(SIP_PDU::StatusCodes reason)
{ 
  endpoint.OnMessageFailed(GetAddressOfRecord(), reason);
  SIPHandler::OnFailed(reason);
}


void SIPMessageHandler::OnExpireTimeout(PTimer &, INT)
{
  SetState(Unavailable);
}


void SIPMessageHandler::SetBody(const PString & b)
{
  SIPHandler::SetBody (b);
}

/////////////////////////////////////////////////////////////////////////

SIPPingHandler::SIPPingHandler (SIPEndPoint & endpoint, 
                                const PString & to)
  : SIPHandler(endpoint, to, "")
{
}


SIPTransaction * SIPPingHandler::CreateTransaction(OpalTransport &t)
{
  return new SIPPing(endpoint, t, GetAddressOfRecord(), body);
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


/**
  * called when a handler is added
  */

void SIPHandlersList::Append(SIPHandler * obj)
{
  if (obj == NULL)
    return;

  PWaitAndSignal m(m_extraMutex);

  PSafePtr<SIPHandler> handler = m_handlersList.Append(obj, PSafeReference);

  // add entry to call to handler map
  m_handlersByCallId.insert(StringToHandlerMap::value_type(handler->GetCallID(), handler));

  // add entry to username/realm map
  if (!handler->GetUsername().IsEmpty()) {
    PString key(handler->GetUsername() + '\n' + handler->GetRealm());
    handler->m_userNameAndRealmKey = (const char *)key;
    m_handlersByUserNameAndRealm.insert(StringToHandlerMap::value_type(handler->m_userNameAndRealmKey, handler));
  }

  // add entry to url map
  PString url(handler->GetAddressOfRecord().AsString());
  PString key;
  key.sprintf("%i\n%s", handler->GetMethod(), (const char *)url);
  handler->m_urlKey = (const char *)key;
  m_handlersByUrl.insert(StringToHandlerMap::value_type(handler->m_urlKey, handler));

  // add entry to url and package map
  PString pkg(handler->GetEventPackage());
  key.sprintf("%i\n%s\n%s", handler->GetMethod(), (const char *)url, (const char *)pkg);
  handler->m_urlAndPackageKey = (const char *)key;
  m_handlersByUrlAndPackage.insert(StringToHandlerMap::value_type(handler->m_urlAndPackageKey, handler));
}


/**
  * Called when a handler is removed
  */
void SIPHandlersList::Remove(SIPHandler * handler)
{
  if (handler == NULL)
    return;

  PWaitAndSignal m(m_extraMutex);

  m_handlersByCallId.erase(handler->GetCallID());
  m_handlersByUserNameAndRealm.erase(handler->m_userNameAndRealmKey);

  m_handlersList.Remove(handler);
}


/**
 * Find the SIPHandler object with the specified callID
 */
PSafePtr<SIPHandler> SIPHandlersList::FindSIPHandlerByCallID(const PString & callID, PSafetyMode mode)
{
  PSafePtr<SIPHandler> ptr;
  {
    PWaitAndSignal m(m_extraMutex);

    StringToHandlerMap::iterator r = m_handlersByCallId.find(std::string((const char *)callID));
    if (r == m_handlersByCallId.end())
      return NULL;

    ptr = r->second;
  }

  return ptr.SetSafetyMode(mode) ? ptr : NULL;
}


/**
 * Find the SIPHandler object with the specified authRealm
 */
PSafePtr<SIPHandler> SIPHandlersList::FindSIPHandlerByAuthRealm (const PString & authRealm, const PString & userName, PSafetyMode mode)
{
  m_extraMutex.Wait();

  // if username is specified, look for exact matches
  if (!userName.IsEmpty()) {

    // look for a match to exact user name and realm
    PString key(userName + '\n' + authRealm);
    StringToHandlerMap::iterator r = m_handlersByUserNameAndRealm.find((const char *)key);
    if (r != m_handlersByUserNameAndRealm.end()) {
      PSafePtr<SIPHandler> handler = r->second;
      m_extraMutex.Signal();
      handler.SetSafetyMode(mode);
      return handler;
    }

    // look for a match to exact user name and empty realm
    key = PString(userName + '\n');
    r = m_handlersByUserNameAndRealm.find((const char *)key);
    if (r != m_handlersByUserNameAndRealm.end()) {
      PSafePtr<SIPHandler> handler = r->second;
      m_extraMutex.Signal();
      handler.SetSafetyMode(mode);
      return handler;
    }

#if 0
    // look for a match to exact username and realm as hostname
    PIPSocket::Address addr;
    if (PIPSocket::GetHostAddress(authRealm, addr)) {
      key = PString(userName + '\n' + addr.AsString());
      r = m_handlersByUserNameAndRealmAddress.find(std::string(key));
      if (r != m_handlersByUserNameAndRealmAddress.end()) {
        PSafePtr<SIPHandler> handler = r->second;
        m_extraMutex.Signal();
        handler.SetSafetyMode(mode);
        return handler;
      }
    }
#endif
  }

  m_extraMutex.Signal();

  // look for a match to exact realm
  for (PSafePtr<SIPHandler> handler(m_handlersList, PSafeReference); handler != NULL; ++handler) {
    if (handler->GetRealm() == authRealm && handler.SetSafetyMode(mode))
      return handler;
  }

#if 0
  // look for a match to exact realm as hostname
  for (PSafePtr<SIPHandler> handler(m_handlersList, PSafeReference); handler != NULL; ++handler) {
    PIPSocket::Address addr;
    if (PIPSocket::GetHostAddress(handler->GetRealm(), addr) &&
        handlerRealmAddress == authRealmAddress &&
        handler.SetSafetyMode(mode))
      return handler;
  }
#endif

  return NULL;
}


/**
 * Find the SIPHandler object with the specified URL. The url is
 * the registration address, for example, 6001@sip.seconix.com
 * when registering 6001 to sip.seconix.com with realm seconix.com
 * or 6001@seconix.com when registering 6001@seconix.com to
 * sip.seconix.com
 */
PSafePtr<SIPHandler> SIPHandlersList::FindSIPHandlerByUrl(const PString & aor, SIP_PDU::Methods method, PSafetyMode mode)
{
  PStringStream key;
  key << method << '\n' << aor;

  PSafePtr<SIPHandler> ptr;
  {
    PWaitAndSignal m(m_extraMutex);

    StringToHandlerMap::iterator r = m_handlersByUrl.find(std::string((const char *)key));
    if (r == m_handlersByUrl.end())
      return NULL;

    ptr = r->second;
  }

  return ptr.SetSafetyMode(mode) ? ptr : NULL;
}


PSafePtr<SIPHandler> SIPHandlersList::FindSIPHandlerByUrl(const PString & aor, SIP_PDU::Methods method, const PString & eventPackage, PSafetyMode mode)
{
  PStringStream key;
  key << method << '\n' << aor << '\n' << eventPackage;

  PSafePtr<SIPHandler> ptr;
  {
    PWaitAndSignal m(m_extraMutex);

    StringToHandlerMap::iterator r = m_handlersByUrlAndPackage.find(std::string((const char *)key));
    if (r == m_handlersByUrlAndPackage.end())
      return NULL;

    ptr = r->second;
  }

  return ptr.SetSafetyMode(mode) ? ptr : NULL;
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
