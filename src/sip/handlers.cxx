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

#ifdef OPAL_SIP

#ifdef __GNUC__
#pragma implementation "handlers.h"
#endif

#include <sip/handlers.h>

#include <ptclib/pdns.h>
#include <ptclib/enum.h>
#include <sip/sipep.h>

#ifdef OPAL_PTLIB_EXPAT
#include <ptclib/pxml.h>
#endif


#define new PNEW


#if PTRACING
ostream & operator<<(ostream & strm, SIPHandler::State state)
{
  static const char * const StateNames[] = {
    "Subscribed", "Subscribing", "Refreshing", "Restoring", "Unsubscribing", "Unsubscribed"
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
                       const PString & to,
                       int expireTime,
                       int offlineExpireTime,
                       const PTimeInterval & retryMin,
                       const PTimeInterval & retryMax)
  : endpoint(ep)
  , transport(NULL)
  , expire(expireTime)
  , originalExpire(expire)
  , offlineExpire(offlineExpireTime)
  , state(Unsubscribed)
  , retryTimeoutMin(retryMin)
  , retryTimeoutMax(retryMax)
{
  transactions.DisallowDeleteObjects();

  targetAddress.Parse(to);
  remotePartyAddress = targetAddress.AsQuotedString();

  authenticationAttempts = 0;
  authentication = NULL;

  callID = OpalGloballyUniqueID().AsString() + "@" + PIPSocket::GetHostName();

  expireTimer.SetNotifier(PCREATE_NOTIFIER(OnExpireTimeout));
}


SIPHandler::~SIPHandler() 
{
  if (transport) {
    transport->CloseWait();
    delete transport;
  }
  if (authentication != NULL)
    delete authentication;
}


void SIPHandler::SetState(SIPHandler::State newState) 
{
  PTRACE(4, "SIP\tChanging " << GetMethod() << " handler from " << state << " to " << newState
         << ", target=" << GetTargetAddress() << ", id=" << callID);
  state = newState;
}


OpalTransport * SIPHandler::GetTransport()
{
  if (transport != NULL) {
    if (transport->IsOpen())
      return transport;

    transport->CloseWait();
    delete transport;
    transport = NULL;
  }

  if (proxy.IsEmpty()) {
    // Look for a "proxy" parameter to override default proxy
    const PStringToString& params = targetAddress.GetParamVars();
    if (params.Contains("proxy")) {
      proxy.Parse(params("proxy"));
      targetAddress.SetParamVar("proxy", PString::Empty());
    }
  }

  if (proxy.IsEmpty())
    proxy = endpoint.GetProxy();

  if (proxy.IsEmpty())
    return (transport = endpoint.CreateTransport(targetAddress.GetHostAddress()));

  // Default routeSet if there is a proxy
  if (routeSet.IsEmpty()) 
    routeSet += "sip:" + proxy.GetHostName() + ':' + PString(proxy.GetPort()) + ";lr";

  transport = endpoint.CreateTransport(proxy.GetHostAddress());
  return transport;
}


const PString SIPHandler::GetRemotePartyAddress ()
{
  SIPURL cleanAddress = remotePartyAddress;
  cleanAddress.Sanitise(SIPURL::ExternalURI);

  return cleanAddress.AsString();
}


void SIPHandler::SetExpire(int e)
{
  expire = e > 0 ? e : originalExpire;
  PTRACE(3, "SIP\tExpiry time for " << GetMethod() << " set to " << expire << " seconds.");
  // Only modify the originalExpire for future requests if IntervalTooBrief gives
  // a bigger expire time. expire itself will always reflect the proxy decision
  // (bigger or lower), but originalExpire determines what is used in future 
  // requests and is only modified if interval too brief
  if (originalExpire < e)
    originalExpire = e;

  if (expire > 0 && state < Unsubscribing)
    expireTimer.SetInterval(0, (unsigned) (9*expire/10));
}


PBoolean SIPHandler::WriteSIPHandler(OpalTransport & transport, void * param)
{
  return param != NULL && ((SIPHandler *)param)->WriteSIPHandler(transport);
}


bool SIPHandler::WriteSIPHandler(OpalTransport & transport)
{
  SIPTransaction * transaction = CreateTransaction(transport);

  if (transaction != NULL) {
    if (state == Unsubscribing)
      transaction->GetMIME().SetExpires(0);
    callID = transaction->GetMIME().GetCallID();
    if (authentication != NULL)
      authentication->Authorise(*transaction); // If already have info from last time, use it!
    if (transaction->Start()) {
      transactions.Append(transaction);
      return true;
    }
  }

  PTRACE(2, "SIP\tDid not start transaction on " << transport);
  OnFailed(SIP_PDU::Local_TransportError);
  return false;
}


PBoolean SIPHandler::SendRequest(SIPHandler::State s)
{
  expireTimer.Stop(); // Stop automatic retry

  switch (s) {
    case Unsubscribing:
      if (state != Subscribed)
        return false;
      SetState(s);
      break;

    case Subscribing :
    case Refreshing :
    case Restoring :
      SetState(s);
      if (GetTransport() == NULL) {
        PTRACE(4, "SIP\tRetrying " << GetMethod() << " in " << offlineExpire << " seconds.");
        OnFailed(SIP_PDU::Local_BadTransportAddress);
        expireTimer.SetInterval(0, offlineExpire); // Keep trying to get it back
        SetState(Unsubscribed);
        return true;
      }
      break;

    default :
      PAssertAlways(PInvalidParameter);
      return false;
  }

  // Restoring or first time, try every interface
  if (s == Restoring || transport->GetInterface().IsEmpty())
    return transport->WriteConnect(WriteSIPHandler, this);

  // We contacted the server on an interface last time, assume it still works!
  return WriteSIPHandler(*transport);
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
    OnFailed(SIP_PDU::Failure_UnAuthorised);
    return;
  }

  // check to see if this is a follow-on from the last authentication scheme used
  if (authentication != NULL && 
      (authentication->GetClass() == newAuth->GetClass()) && 
      newAuth->EquivalentTo(*authentication) &&
      GetState() == SIPHandler::Subscribing) {
    delete newAuth;
    PTRACE(1, "SIP\tAuthentication already performed for " << proxyTrace << "Authentication Required");
    OnFailed(SIP_PDU::Failure_UnAuthorised);
    return;
  }

  // Try to find authentication parameters for the given realm,
  // if not, use the proxy authentication parameters (if any)
  if (authenticationUsername.IsEmpty () && authenticationPassword.IsEmpty ()) {
    if (endpoint.GetAuthentication(newAuth->GetAuthRealm(), authenticationAuthRealm, authenticationUsername, authenticationPassword)) {
      PTRACE (3, "SIP\tFound auth info for realm " << newAuth->GetAuthRealm());
    }
    else {
      SIPURL proxy = endpoint.GetProxy();
      if (proxy.IsEmpty()) {
        PTRACE (3, "SIP\tNo auth info for realm " << newAuth->GetAuthRealm());
        delete newAuth;
        OnFailed(SIP_PDU::Failure_UnAuthorised);
        return;
      }

      PTRACE (3, "SIP\tNo auth info for realm " << newAuth->GetAuthRealm() << ", using proxy auth");
      authenticationUsername = proxy.GetUserName();
      authenticationPassword = proxy.GetPassword();
    }
  }
  authenticationAuthRealm = newAuth->GetAuthRealm();
  newAuth->SetUsername(authenticationUsername);
  newAuth->SetPassword(authenticationPassword);

  // switch authentication schemes
  delete authentication;
  authentication = newAuth;

  // And end connect mode on the transport
  CollapseFork(transaction);

  // Restart the transaction with new authentication handler
  SIPTransaction * newTransaction = CreateTransaction(*transport);
  if (!authentication->Authorise(*newTransaction)) {
    // don't send again if no authentication handler available
    OnFailed(SIP_PDU::Failure_UnAuthorised);
    delete newTransaction; // Not started yet, we need to delete
    return;
  }

  // Section 8.1.3.5 of RFC3261 tells that the authenticated
  // request SHOULD have the same value of the Call-ID, To and From.
  newTransaction->GetMIME().SetFrom(transaction.GetMIME().GetFrom());
  newTransaction->GetMIME().SetCallID(GetCallID());
  if (!newTransaction->Start()) {
    PTRACE(1, "SIP\tCould not restart REGISTER/SUBSCRIBE for Authentication Required");
  }
}


void SIPHandler::OnReceivedOK(SIPTransaction & transaction, SIP_PDU & /*response*/)
{
  switch (GetState()) {
    case Unsubscribing :
      SetState(Unsubscribed);
      break;

    case Subscribing :
    case Refreshing :
    case Restoring :
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
  transport->SetInterface(transaction.GetInterface());
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


void SIPHandler::OnFailed(SIP_PDU::StatusCodes code)
{
  switch (code) {
    case SIP_PDU::Failure_UnAuthorised :
    case SIP_PDU::Failure_ProxyAuthenticationRequired :
      return;

    case SIP_PDU::Local_TransportError :
    case SIP_PDU::Failure_RequestTimeout :
    case SIP_PDU::Local_BadTransportAddress :
      break;

    default :
      PTRACE(4, "SIP\tNot retrying " << GetMethod() << " due to error response " << code);
      expire = 0; // OK, stop trying
      expireTimer.Stop();
  }

  SetState(GetState() == Unsubscribing ? Subscribed : Unsubscribed);
}


void SIPHandler::OnExpireTimeout(PTimer &, INT)
{
  PTRACE(2, "SIP\tStarting " << GetMethod() << " for binding refresh");

  if (!SendRequest(GetState() == Subscribed ? Refreshing : Restoring))
    SetState(Unsubscribed);
}


PBoolean SIPHandler::CanBeDeleted()
{
  return GetState() == Unsubscribed;
}


///////////////////////////////////////////////////////////////////////////////

SIPRegisterHandler::SIPRegisterHandler(SIPEndPoint & endpoint, const SIPRegister::Params & params)
  : SIPHandler(endpoint,
               params.m_addressOfRecord,
               params.m_expire > 0 ? params.m_expire : endpoint.GetRegistrarTimeToLive().GetSeconds(),
               params.m_restoreTime, params.m_minRetryTime, params.m_maxRetryTime)
  , m_parameters(params)
{
  m_parameters.m_expire = expire; // Put possibly adjusted value back

  SIPURL registrar = params.m_registrarAddress;
  if (!registrar.IsEmpty() && !registrar.GetHostAddress().IsEquivalent(targetAddress.GetHostAddress()))
    proxy = registrar;

  authenticationUsername  = params.m_authID;
  authenticationPassword  = params.m_password;
  authenticationAuthRealm = params.m_realm;
}


SIPRegisterHandler::~SIPRegisterHandler()
{
  PTRACE(4, "SIP\tDeleting SIPRegisterHandler " << GetRemotePartyAddress());
}


SIPTransaction * SIPRegisterHandler::CreateTransaction(OpalTransport & trans)
{
  m_parameters.m_expire = expire; 

  return new SIPRegister(endpoint, trans, GetRouteSet(), callID, m_parameters);
}


void SIPRegisterHandler::OnReceivedOK(SIPTransaction & transaction, SIP_PDU & response)
{
  std::vector<SIPURL> requestContacts, replyContacts;
  transaction.GetMIME().GetContacts(requestContacts);
  response.GetMIME().GetContacts(replyContacts);

  for (size_t request = 0; request < requestContacts.size(); request++) {
    for (size_t reply = 0; reply < replyContacts.size(); reply++) {
      if (requestContacts[request] == replyContacts[reply]) {
        PString expires = SIPMIMEInfo::ExtractFieldParameter(replyContacts[reply].GetFieldParameters(), "expires");
        if (expires.IsEmpty())
          SetExpire(response.GetMIME().GetExpires(endpoint.GetRegistrarTimeToLive().GetSeconds()));
        else
          SetExpire(expires.AsUnsigned());
      }
    }
  }

  SendStatus(SIP_PDU::Successful_OK);
  SIPHandler::OnReceivedOK(transaction, response);
}


void SIPRegisterHandler::OnFailed(SIP_PDU::StatusCodes r)
{
  SendStatus(r);
  SIPHandler::OnFailed(r);
}


PBoolean SIPRegisterHandler::SendRequest(SIPHandler::State s)
{
  SendStatus(SIP_PDU::Information_Trying);
  return SIPHandler::SendRequest(s);
}


void SIPRegisterHandler::SendStatus(SIP_PDU::StatusCodes code)
{
  PString aor = targetAddress.AsString().Mid(4);
  switch (GetState()) {
    case Subscribing :
      endpoint.OnRegistrationStatus(aor, true, false, code);
      break;

    case Subscribed :
    case Refreshing :
      endpoint.OnRegistrationStatus(aor, true, true, code);
      break;

    case Unsubscribed :
    case Restoring :
      endpoint.OnRegistrationStatus(aor, true, code/100 != 2, code);
      break;

    case Unsubscribing :
      endpoint.OnRegistrationStatus(aor, false, false, code);
      break;
  }
}


void SIPRegisterHandler::UpdateParameters(const SIPRegister::Params & params)
{
  if (!params.m_authID.IsEmpty())
    authenticationUsername = params.m_authID;   // Adjust the authUser if required 
  if (!params.m_realm.IsEmpty())
    authenticationAuthRealm = params.m_realm;   // Adjust the realm if required 
  if (!params.m_password.IsEmpty())
    authenticationPassword = params.m_password; // Adjust the password if required 

  SetExpire(params.m_expire);
}


/////////////////////////////////////////////////////////////////////////

SIPSubscribeHandler::SIPSubscribeHandler (SIPEndPoint & endpoint, 
                                          const PString & eventPackage,
                                          const PString & to,
                                          int exp)
  : SIPHandler(endpoint, to, exp > 0 ? exp : endpoint.GetNotifierTimeToLive().GetSeconds())
  , m_eventPackage(eventPackage)
{
  lastSentCSeq = 0;
  lastReceivedCSeq = 0;
  dialogCreated = PFalse;
}


SIPSubscribeHandler::~SIPSubscribeHandler()
{
  PTRACE(4, "SIP\tDeleting SIPSubscribeHandler " << GetRemotePartyAddress());
}


SIPTransaction * SIPSubscribeHandler::CreateTransaction(OpalTransport &trans)
{ 
  PString partyName;

  if (localPartyAddress.IsEmpty()) {

    if (m_eventPackage == SIPSubscribe::Presence)
      localPartyAddress = endpoint.GetRegisteredPartyName(targetAddress).AsQuotedString();
    else
      localPartyAddress = targetAddress.AsQuotedString();

    localPartyAddress += ";tag=" + OpalGloballyUniqueID().AsString();
  }

  SetExpire(originalExpire);
  return new SIPSubscribe(endpoint,
                          trans, 
                          m_eventPackage,
                          GetRouteSet(),
                          targetAddress, 
                          remotePartyAddress,
                          localPartyAddress,
                          callID, 
                          GetNextCSeq(),
                          expire); 
}


void SIPSubscribeHandler::OnReceivedOK(SIPTransaction & transaction, SIP_PDU & response)
{
  /* An "expire" parameter in the Contact header has no semantics
   * for SUBSCRIBE. RFC3265, 3.1.1.
   * An answer can only shorten the expires time.
   */
  SetExpire(response.GetMIME().GetExpires(3600));

  /* Update the routeSet according 12.1.2. */
  if (!dialogCreated) {
    PStringList recordRoute = response.GetMIME().GetRecordRoute();
    routeSet.RemoveAll();
    for (PStringList::iterator route = recordRoute.rbegin(); route != recordRoute.rend(); --route)
      routeSet += *route;
    if (!response.GetMIME().GetContact().IsEmpty()) 
      targetAddress = response.GetMIME().GetContact();
    dialogCreated = PTrue;
  }

  /* Update the To */
  remotePartyAddress = response.GetMIME().GetTo();

  SIPHandler::OnReceivedOK(transaction, response);
}


PBoolean SIPSubscribeHandler::OnReceivedNOTIFY(SIP_PDU & request)
{
  if (transport == NULL)
    return PFalse;

  unsigned requestCSeq = request.GetMIME().GetCSeq().AsUnsigned();

  if (lastReceivedCSeq == 0)
  lastReceivedCSeq = requestCSeq;
  else if (requestCSeq > lastReceivedCSeq)
    lastReceivedCSeq = requestCSeq;
  else if (requestCSeq == lastReceivedCSeq)
    return endpoint.SendResponse(SIP_PDU::Successful_OK, *transport, request);
  else
    return endpoint.SendResponse(SIP_PDU::Failure_InternalServerError, *transport, request);

  PTRACE(3, "SIP\tFound a SUBSCRIBE corresponding to the NOTIFY");

  // We received a NOTIFY corresponding to an active SUBSCRIBE
  // for which we have just unSUBSCRIBEd. That is the final NOTIFY.
  // We can remove the SUBSCRIBE from the list.
  if (GetState() != SIPHandler::Subscribed && expire == 0) {
    PTRACE(3, "SIP\tFinal NOTIFY received");
  }

  PString state = request.GetMIME().GetSubscriptionState();

  // Check the susbscription state
  if (state.Find("terminated") != P_MAX_INDEX) {
    PTRACE(3, "SIP\tSubscription is terminated");
    SetState(Unsubscribed);
  }
  else if (state.Find("active") != P_MAX_INDEX || state.Find("pending") != P_MAX_INDEX) {

    PTRACE(3, "SIP\tSubscription is " << state);
    PString expire = SIPMIMEInfo::ExtractFieldParameter(state, "expire");
    if (!expire.IsEmpty())
      SetExpire(expire.AsUnsigned());
  }

  PCaselessString eventPackage = request.GetMIME().GetEvent();
  if (eventPackage == SIPSubscribe::MessageSummary)
      OnReceivedMWINOTIFY(request);
  else if (eventPackage == SIPSubscribe::Presence)
      OnReceivedPresenceNOTIFY(request);

  return endpoint.SendResponse(SIP_PDU::Successful_OK, *transport, request);
}


PBoolean SIPSubscribeHandler::OnReceivedMWINOTIFY(SIP_PDU & request)
{
  PString body = request.GetEntityBody();
  PString msgs;

  // Extract the string describing the number of new messages
  if (!body.IsEmpty ()) {

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
    PStringArray bodylines = body.Lines ();
    for (PINDEX z = 0 ; z < PARRAYSIZE(validMessageClasses); z++) {

      for (int i = 0 ; i < bodylines.GetSize () ; i++) {

        PCaselessString line (bodylines [i]);
        PINDEX j = line.FindLast(validMessageClasses[z].name);
        if (j != P_MAX_INDEX) {
          line.Replace(validMessageClasses[z].name, "");
          line.Replace (":", "");
          msgs = line.Trim ();
          endpoint.OnMWIReceived(GetRemotePartyAddress(), validMessageClasses[z].type, msgs);
          return PTrue;
        }
      }
    }

    // Received MWI, unknown messages number
    endpoint.OnMWIReceived(GetRemotePartyAddress(), OpalManager::NumMessageWaitingTypes, "1/0");
  } 

  return PTrue;
}


#ifdef OPAL_PTLIB_EXPAT
PBoolean SIPSubscribeHandler::OnReceivedPresenceNOTIFY(SIP_PDU & request)
{
  PString body = request.GetEntityBody();
  SIPURL from = request.GetMIME().GetFrom();
  PString basic;
  PString note;

  PXML xmlPresence;
  PXMLElement *rootElement = NULL;
  PXMLElement *tupleElement = NULL;
  PXMLElement *statusElement = NULL;
  PXMLElement *basicElement = NULL;
  PXMLElement *noteElement = NULL;

  if (!xmlPresence.Load(body))
    return PFalse;

  rootElement = xmlPresence.GetRootElement();
  if (rootElement == NULL)
    return PFalse;

  if (rootElement->GetName() != "presence")
    return PFalse;

  tupleElement = rootElement->GetElement("tuple");
  if (tupleElement == NULL)
    return PFalse;

  statusElement = tupleElement->GetElement("status");
  if (statusElement == NULL)
    return PFalse;

  basicElement = statusElement->GetElement("basic");
  if (basicElement)
    basic = basicElement->GetData();

  noteElement = statusElement->GetElement("note");
  if (!noteElement)
    noteElement = rootElement->GetElement("note");
  if (!noteElement)
    noteElement = tupleElement->GetElement("note");
  if (noteElement)
    note = noteElement->GetData();

  from.Sanitise(SIPURL::ExternalURI);
  endpoint.OnPresenceInfoReceived (from.AsQuotedString(), basic, note);
  return PTrue;
}
#else
PBoolean SIPSubscribeHandler::OnReceivedPresenceNOTIFY(SIP_PDU &)
{
  return TRUE;
}
#endif


/////////////////////////////////////////////////////////////////////////

SIPPublishHandler::SIPPublishHandler(SIPEndPoint & endpoint,
                                     const PString & to,   /* The to field  */
                                     const PString & b,
                                     int exp)
  : SIPHandler(endpoint, to, exp > 0 ? exp : endpoint.GetNotifierTimeToLive().GetSeconds())
{
  publishTimer.SetNotifier(PCREATE_NOTIFIER(OnPublishTimeout));
  publishTimer.RunContinuous (PTimeInterval (0, 5));

  stateChanged = PFalse;

  body = b;
}


SIPPublishHandler::~SIPPublishHandler()
{
  PTRACE(4, "SIP\tDeleting SIPPublishHandler " << GetRemotePartyAddress());
}


SIPTransaction * SIPPublishHandler::CreateTransaction(OpalTransport & t)
{
  SetExpire(originalExpire);
  callID = OpalGloballyUniqueID().AsString() + "@" + PIPSocket::GetHostName();
  return new SIPPublish(endpoint,
                        t, 
                        GetRouteSet(), 
                        targetAddress, 
                        callID,
                        sipETag, 
                        (GetState() == Refreshing)?PString::Empty():body, 
                        expire);
}


void SIPPublishHandler::OnReceivedOK(SIPTransaction & transaction, SIP_PDU & response)
{
  if (!response.GetMIME().GetSIPETag().IsEmpty())
    sipETag = response.GetMIME().GetSIPETag();

  SetExpire(response.GetMIME().GetExpires(3600));

  SIPHandler::OnReceivedOK(transaction, response);
}


void SIPPublishHandler::OnPublishTimeout(PTimer &, INT)
{
  if (GetState() == Subscribed) {
    if (stateChanged) {
      if (!SendRequest(Subscribing))
        SetState(Unsubscribed);
      stateChanged = PFalse;
    }
  }
}


void SIPPublishHandler::SetBody(const PString & b)
{
  stateChanged = PTrue;

  SIPHandler::SetBody (b);
}


PString SIPPublishHandler::BuildBody(const PString & to,
                                     const PString & basic,
                                     const PString & note)
{
  PString data;

  if (to.IsEmpty())
    return data;

  data += "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n";

  data += "<presence xmlns=\"urn:ietf:params:xml:ns:pidf\" entity=\"pres:";
  data += to;
  data += "\">\r\n";

  data += "<tuple id=\"";
  data += OpalGloballyUniqueID().AsString();
  data += "\">\r\n";

  data += "<note>";
  data += note;
  data += "</note>\r\n";

  data += "<status>\r\n";
  data += "<basic>";
  data += basic;
  data += "</basic>\r\n";
  data += "</status>\r\n";

  data += "<contact priority=\"1\">";
  data += to;
  data += "</contact>\r\n";

  data += "</tuple>\r\n";

  data += "</presence>\r\n";


  return data;
}


/////////////////////////////////////////////////////////////////////////

SIPMessageHandler::SIPMessageHandler (SIPEndPoint & endpoint, 
                                      const PString & to,
                                      const PString & b)
  : SIPHandler(endpoint, to, 300)
{
  body = b;

  SetState(Subscribed);
}


SIPMessageHandler::~SIPMessageHandler ()
{
  PTRACE(4, "SIP\tDeleting SIPMessageHandler " << GetRemotePartyAddress());
}


SIPTransaction * SIPMessageHandler::CreateTransaction(OpalTransport &t)
{ 
  SetExpire(originalExpire);
  return new SIPMessage(endpoint, t, targetAddress, routeSet, callID, body);
}


void SIPMessageHandler::OnFailed(SIP_PDU::StatusCodes reason)
{ 
  endpoint.OnMessageFailed(targetAddress, reason);
  SIPHandler::OnFailed(reason);
}


void SIPMessageHandler::OnExpireTimeout(PTimer &, INT)
{
  SetState(Unsubscribed);
}


void SIPMessageHandler::SetBody(const PString & b)
{
  callID = OpalGloballyUniqueID().AsString() + "@" + PIPSocket::GetHostName();
  
  SIPHandler::SetBody (b);
}

/////////////////////////////////////////////////////////////////////////

SIPPingHandler::SIPPingHandler (SIPEndPoint & endpoint, 
                                const PString & to)
  : SIPHandler(endpoint, to, 500)
{
}


SIPTransaction * SIPPingHandler::CreateTransaction(OpalTransport &t)
{
  return new SIPPing(endpoint, t, targetAddress, body);
}


//////////////////////////////////////////////////////////////////

unsigned SIPHandlersList::GetRegistrationsCount()
{
  unsigned count = 0;
  for (PSafePtr<SIPHandler> handler(*this, PSafeReference); handler != NULL; ++handler)
    if (handler->GetState () == SIPHandler::Subscribed && handler->GetMethod() == SIP_PDU::Method_REGISTER) 
      count++;
  return count;
}


/**
 * Find the SIPHandler object with the specified callID
 */
PSafePtr<SIPHandler> SIPHandlersList::FindSIPHandlerByCallID(const PString & callID, PSafetyMode m)
{
  for (PSafePtr<SIPHandler> handler(*this, m); handler != NULL; ++handler)
    if (callID == handler->GetCallID())
      return handler;
  return NULL;
}


/**
 * Find the SIPHandler object with the specified authRealm
 */
PSafePtr<SIPHandler> SIPHandlersList::FindSIPHandlerByAuthRealm (const PString & authRealm, const PString & userName, PSafetyMode m)
{
  PIPSocket::Address realmAddress;

  for (PSafePtr<SIPHandler> handler(*this, m); handler != NULL; ++handler) {
    if (authRealm == handler->authenticationAuthRealm && (userName.IsEmpty() || userName == handler->authenticationUsername))
      return handler;
  }
  for (PSafePtr<SIPHandler> handler(*this, m); handler != NULL; ++handler) {
    if (PIPSocket::GetHostAddress(handler->authenticationAuthRealm, realmAddress))
      if (realmAddress == PIPSocket::Address(authRealm) && (userName.IsEmpty() || userName == handler->authenticationUsername))
        return handler;
  }
  return NULL;
}


/**
 * Find the SIPHandler object with the specified URL. The url is
 * the registration address, for example, 6001@sip.seconix.com
 * when registering 6001 to sip.seconix.com with realm seconix.com
 * or 6001@seconix.com when registering 6001@seconix.com to
 * sip.seconix.com
 */
PSafePtr<SIPHandler> SIPHandlersList::FindSIPHandlerByUrl(const PString & url, SIP_PDU::Methods meth, PSafetyMode m)
{
  for (PSafePtr<SIPHandler> handler(*this, m); handler != NULL; ++handler) {
    if (SIPURL(url) == SIPURL(handler->GetRemotePartyAddress()) && meth == handler->GetMethod())
      return handler;
  }
  return NULL;
}


PSafePtr<SIPHandler> SIPHandlersList::FindSIPHandlerByUrl(const PString & url, SIP_PDU::Methods meth, const PString & eventPackage, PSafetyMode m)
{
  for (PSafePtr<SIPHandler> handler(*this, m); handler != NULL; ++handler) {
    if (SIPURL(url) == handler->GetTargetAddress() && meth == handler->GetMethod() && handler->GetEventPackage() == eventPackage)
      return handler;
  }
  return NULL;
}


/**
 * Find the SIPHandler object with the specified registration host.
 * For example, in the above case, the name parameter
 * could be "sip.seconix.com" or "seconix.com".
 */
PSafePtr<SIPHandler> SIPHandlersList::FindSIPHandlerByDomain(const PString & name, SIP_PDU::Methods /*meth*/, PSafetyMode m)
{
  for (PSafePtr<SIPHandler> handler(*this, m); handler != NULL; ++handler) {

    if (name *= handler->GetTargetAddress().GetHostName())
      return handler;

    OpalTransportAddress addr;
    PIPSocket::Address infoIP;
    PIPSocket::Address nameIP;
    WORD port = 5060;
    addr = name;

    if (addr.GetIpAndPort (nameIP, port)) {
      addr = handler->GetTargetAddress().GetHostName();
      if (addr.GetIpAndPort (infoIP, port)) {
        if (infoIP == nameIP) {
          return handler;
        }
      }
    }
  }
  return NULL;
}


#endif // OPAL_SIP
