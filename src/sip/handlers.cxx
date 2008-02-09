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

#if P_EXPAT
#include <ptclib/pxml.h>
#endif


#define new PNEW


#if PTRACING
ostream & operator<<(ostream & strm, SIPHandler::State state)
{
  static const char * const StateNames[] = {
    "Subscribed", "Subscribing", "Refreshing", "Unsubscribing", "Unsubscribed"
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
                       const PTimeInterval & retryMin,
                       const PTimeInterval & retryMax)
  : endpoint(ep)
  , expire(expireTime)
  , originalExpire(expire)
  , state(Unsubscribed)
  , retryTimeoutMin(retryMin)
  , retryTimeoutMax(retryMax)
{
  transactions.DisallowDeleteObjects();

  targetAddress.Parse(to);
  remotePartyAddress = targetAddress.AsQuotedString();

  authenticationAttempts = 0;

  // Look for a "proxy" parameter to override default proxy
  const PStringToString& params = targetAddress.GetParamVars();
  if (params.Contains("proxy")) {
    proxy.Parse(params("proxy"));
    targetAddress.SetParamVar("proxy", PString::Empty());
  }

  if (proxy.IsEmpty())
    proxy = endpoint.GetProxy();

  if (!proxy.IsEmpty())
    transport = endpoint.CreateTransport(proxy.GetHostAddress());
  else
    transport = endpoint.CreateTransport(targetAddress.GetHostAddress());

  // Default routeSet if there is a proxy
  if (!proxy.IsEmpty() && routeSet.GetSize() == 0) 
    routeSet += "sip:" + proxy.GetHostName() + ':' + PString(proxy.GetPort()) + ";lr";

  callID = OpalGloballyUniqueID().AsString() + "@" + PIPSocket::GetHostName();

  expireTimer.SetNotifier(PCREATE_NOTIFIER(OnExpireTimeout));
}


SIPHandler::~SIPHandler() 
{
  if (transport) {
    transport->CloseWait();
    delete transport;
  }
}


void SIPHandler::SetState(SIPHandler::State newState) 
{
  PTRACE(4, "SIP\tChanging " << GetMethod() << " handler from " << state << " to " << newState
         << ", target=" << GetTargetAddress() << ", id=" << callID);
  state = newState;
}


const PString SIPHandler::GetRemotePartyAddress ()
{
  SIPURL cleanAddress = remotePartyAddress;
  cleanAddress.AdjustForRequestURI();

  return cleanAddress.AsString();
}


void SIPHandler::SetExpire(int e)
{
  expire = e > 0 && e < originalExpire ? e : originalExpire;
  PTRACE(3, "SIP\tExpiry time for " << GetMethod() << " set to " << expire << " seconds.");

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
    authentication.Authorise(*transaction); // If already have info from last time, use it!
    if (transaction->Start()) {
      transactions.Append(transaction);
      return true;
    }
  }

  PTRACE(2, "SIP\tDid not start transaction on " << transport);
  return false;
}


PBoolean SIPHandler::SendRequest(SIPHandler::State s)
{
  if (transport == NULL)
    return PFalse;

  if (s == Unsubscribing && state != Subscribed)
    return false;

  SetState(s);

  if (!transport->IsOpen ()) {

    transport->CloseWait();
    delete transport;

    if (!proxy.IsEmpty())
      transport = endpoint.CreateTransport(proxy.GetHostAddress());
    else
      transport = endpoint.CreateTransport(targetAddress.GetHostAddress());
  }

  // First time, try every interface
  if (transport->GetInterface().IsEmpty())
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

  PString lastNonce;
  PString lastUsername;
  if (authentication.IsValid()) {
    lastUsername = authentication.GetUsername();
    lastNonce = authentication.GetNonce();
  }

  // Received authentication required response, parse it
  if (!authentication.Parse(response.GetMIME()(isProxy ? "Proxy-Authenticate" : "WWW-Authenticate"), isProxy)) {
    OnFailed(SIP_PDU::Failure_UnAuthorised);
    return;
  }

  // Already sent handler for that callID. Check if params are different
  // from the ones found for the given realm
  if (authentication.IsValid() &&
      authentication.GetUsername() == lastUsername &&
      authentication.GetNonce() == lastNonce &&
      GetState() == SIPHandler::Subscribing) {
    PTRACE(2, "SIP\tAlready done REGISTER/SUBSCRIBE for " << proxyTrace << "Authentication Required");
    OnFailed(SIP_PDU::Failure_UnAuthorised);
    return;
  }
  
  // Abort after some unsuccesful authentication attempts. This is required since
  // some implementations return "401 Unauthorized" with a different nonce at every
  // time.
  if(authenticationAttempts >= 10) {
    PTRACE(1, "SIP\tAborting after " << authenticationAttempts << " attempts to REGISTER/SUBSCRIBE");
    OnFailed(SIP_PDU::Failure_UnAuthorised);
    return;
  }

  ++authenticationAttempts;

  // And end connect mode on the transport
  CollapseFork(transaction);

  // Restart the transaction with new authentication handler
  SIPTransaction * newTransaction = CreateTransaction(GetTransport());
  if (!authentication.Authorise(*newTransaction)) {
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
  // Take this transaction out and kill all the rest
  transactions.Remove(&transaction);
  while (transactions.GetSize() > 0) {
    PSafePtr<SIPTransaction> transToGo = transactions.GetAt(0);
    transactions.Remove(transToGo);
    transToGo->Abort();
  }

  // And end connect mode on the transport
  transport->SetInterface(transaction.GetInterface());
}


void SIPHandler::OnTransactionFailed(SIPTransaction & transaction)
{
  if (transactions.Remove(&transaction)) {
    OnFailed(transaction.GetStatusCode());

    if (expire > 0 && !transaction.IsCanceled()) {
      PTRACE(4, "SIP\tRetrying " << GetMethod() << " in 30 seconds.");
      expireTimer.SetInterval(0, 30); // Keep trying to get it back
    }
  }
}


void SIPHandler::OnFailed(SIP_PDU::StatusCodes code)
{
  switch (code) {
    case SIP_PDU::Failure_UnAuthorised :
    case SIP_PDU::Failure_ProxyAuthenticationRequired :
      return;

    case SIP_PDU::Failure_RequestTimeout :
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

  if (!SendRequest(Refreshing))
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
               params.m_minRetryTime, params.m_maxRetryTime)
  , m_parameters(params)
{
  m_parameters.m_expire = expire; // Put possibly adjusted value back

  authentication.SetUsername(params.m_authID);
  authentication.SetPassword(params.m_password);
  authentication.SetAuthRealm(params.m_realm);
}


SIPRegisterHandler::~SIPRegisterHandler()
{
  PTRACE(4, "SIP\tDeleting SIPRegisterHandler " << GetRemotePartyAddress());
}


SIPTransaction * SIPRegisterHandler::CreateTransaction(OpalTransport & trans)
{
  return new SIPRegister(endpoint, trans, GetRouteSet(), callID, m_parameters);
}


void SIPRegisterHandler::OnReceivedOK(SIPTransaction & transaction, SIP_PDU & response)
{
  PString contact = response.GetMIME().GetContact();

  int newExpiryTime = SIPURL(contact).GetParamVars()("expires").AsUnsigned();
  if (newExpiryTime == 0) {
    if (response.GetMIME().HasFieldParameter("expires", contact))
      newExpiryTime = response.GetMIME().GetFieldParameter("expires", contact).AsUnsigned();
    else
      newExpiryTime = response.GetMIME().GetExpires(endpoint.GetRegistrarTimeToLive().GetSeconds());
  }

  SetExpire(newExpiryTime);

  SendStatus(SIP_PDU::Successful_OK);
  SIPHandler::OnReceivedOK(transaction, response);
}


void SIPRegisterHandler::OnFailed (SIP_PDU::StatusCodes r)
{ 
  SendStatus(r);
  SIPHandler::OnFailed(r);
}


void SIPRegisterHandler::SendStatus(SIP_PDU::StatusCodes code)
{
  PString aor = targetAddress.AsString().Mid(4);
  switch (GetState()) {
    case Subscribing :
      endpoint.OnRegistrationStatus(aor, true, false, code);
      break;

    case Refreshing :
      endpoint.OnRegistrationStatus(aor, true, true, code);
      break;

    case Unsubscribing :
      endpoint.OnRegistrationStatus(aor, false, false, code);
      break;

    default :
      break;
  }
}


void SIPRegisterHandler::UpdateParameters(const SIPRegister::Params & params)
{
  if (!params.m_authID.IsEmpty())
    authentication.SetUsername(params.m_authID); // Adjust the authUser if required 
  if (!params.m_realm.IsEmpty())
    authentication.SetAuthRealm(params.m_realm);   // Adjust the realm if required 
  if (!params.m_password.IsEmpty())
    authentication.SetPassword(params.m_password); // Adjust the password if required 
  SetExpire(params.m_expire);
}


/////////////////////////////////////////////////////////////////////////

SIPSubscribeHandler::SIPSubscribeHandler (SIPEndPoint & endpoint, 
                                          SIPSubscribe::SubscribeType t,
                                          const PString & to,
                                          int exp)
  : SIPHandler(endpoint, to, exp > 0 ? exp : endpoint.GetNotifierTimeToLive().GetSeconds())
{
  lastSentCSeq = 0;
  lastReceivedCSeq = 0;
  type = t;
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

    if (type == SIPSubscribe::Presence)
      localPartyAddress = endpoint.GetRegisteredPartyName(targetAddress).AsQuotedString();
    else
      localPartyAddress = targetAddress.AsQuotedString();

    localPartyAddress += ";tag=" + OpalGloballyUniqueID().AsString();
  }

  SetExpire(originalExpire);
  return new SIPSubscribe(endpoint,
                          trans, 
                          type,
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
  unsigned requestCSeq = request.GetMIME().GetCSeq().AsUnsigned();
  SIPSubscribe::SubscribeType event = SIPSubscribe::MessageSummary;

  if (request.GetMIME().GetEvent().Find("message-summary") != P_MAX_INDEX)
    event = SIPSubscribe::MessageSummary;
  else if (request.GetMIME().GetEvent().Find("presence") != P_MAX_INDEX)
    event = SIPSubscribe::Presence;

  if (lastReceivedCSeq == 0)
    lastReceivedCSeq = requestCSeq;

  if (transport == NULL)
    return PFalse;

  else if (requestCSeq < lastReceivedCSeq) {

    endpoint.SendResponse(SIP_PDU::Failure_InternalServerError, *transport, request);
    return PFalse;
  }
  lastReceivedCSeq = requestCSeq;

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
    if (request.GetMIME().HasFieldParameter("expire", state))
      SetExpire(request.GetMIME().GetFieldParameter("expire", state).AsUnsigned());
  }

  switch (event) 
    {
    case SIPSubscribe::MessageSummary:
      OnReceivedMWINOTIFY(request);
      break;
    case SIPSubscribe::Presence:
      OnReceivedPresenceNOTIFY(request);
      break;
    default:
      break;
    }

  return endpoint.SendResponse(SIP_PDU::Successful_OK, *transport, request);
}


PBoolean SIPSubscribeHandler::OnReceivedMWINOTIFY(SIP_PDU & request)
{
  PString body = request.GetEntityBody();
  PString msgs;

  // Extract the string describing the number of new messages
  if (!body.IsEmpty ()) {

    const char * validMessageClasses [] = 
      {
        "voice-message", 
        "fax-message", 
        "pager-message", 
        "multimedia-message", 
        "text-message", 
        "none", 
        NULL
      };
    PStringArray bodylines = body.Lines ();
    for (int z = 0 ; validMessageClasses [z] != NULL ; z++) {

      for (int i = 0 ; i < bodylines.GetSize () ; i++) {

        PCaselessString line (bodylines [i]);
        PINDEX j = line.FindLast(validMessageClasses [z]);
        if (j != P_MAX_INDEX) {
          line.Replace (validMessageClasses[z], "");
          line.Replace (":", "");
          msgs = line.Trim ();
          endpoint.OnMWIReceived (GetRemotePartyAddress(),
                            (SIPSubscribe::MWIType) z, 
                            msgs);
          return PTrue;
        }
      }
    }

    // Received MWI, unknown messages number
    endpoint.OnMWIReceived (GetRemotePartyAddress(),
                      (SIPSubscribe::MWIType) 0, 
                      "1/0");
  } 

  return PTrue;
}


#if P_EXPAT
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

  from.AdjustForRequestURI();
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
  return new SIPPublish(endpoint,
                        t, 
                        GetRouteSet(), 
                        targetAddress, 
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
  body = b;
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
  return new SIPMessage(endpoint, t, targetAddress, routeSet, body);
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

  for (PSafePtr<SIPHandler> handler(*this, m); handler != NULL; ++handler)
    if (authRealm == handler->GetAuthentication().GetAuthRealm() && (userName.IsEmpty() || userName == handler->GetAuthentication().GetUsername()))
      return handler;
  for (PSafePtr<SIPHandler> handler(*this, m); handler != NULL; ++handler) {
    if (PIPSocket::GetHostAddress(handler->GetAuthentication().GetAuthRealm(), realmAddress))
      if (realmAddress == PIPSocket::Address(authRealm) && (userName.IsEmpty() || userName == handler->GetAuthentication().GetUsername()))
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


PSafePtr<SIPHandler> SIPHandlersList::FindSIPHandlerByUrl(const PString & url, SIP_PDU::Methods meth, SIPSubscribe::SubscribeType type, PSafetyMode m)
{
  for (PSafePtr<SIPHandler> handler(*this, m); handler != NULL; ++handler) {
    if (SIPURL(url) == handler->GetTargetAddress() && meth == handler->GetMethod() && handler->GetSubscribeType() == type)
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
