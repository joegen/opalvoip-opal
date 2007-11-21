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
 * $Id$
 */

#ifdef __GNUC__
#pragma implementation "handlers.h"
#endif

#include <ptlib.h>

#include <ptclib/enum.h>

#if P_EXPAT
#include <ptclib/pxml.h>
#endif

#include <ptclib/pdns.h>

#include <sip/sipep.h>
#include <sip/handlers.h>

#define new PNEW


////////////////////////////////////////////////////////////////////////////

SIPHandler::SIPHandler(SIPEndPoint & ep,
                       const PString & to,
                       const PTimeInterval & retryMin,
                       const PTimeInterval & retryMax)
  : endpoint(ep), 
    expire(0),
    retryTimeoutMin(retryMin), 
    retryTimeoutMax(retryMax)
{
  targetAddress.Parse(to);
  remotePartyAddress = targetAddress.AsQuotedString();

  authenticationAttempts = 0;

  // Look for a "proxy" parameter to override default proxy
  const PStringToString& params = targetAddress.GetParamVars();
  SIPURL proxy;
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

  SetState (Unsubscribed);
}


SIPHandler::~SIPHandler() 
{
  if (transport) {
    transport->CloseWait();
    delete transport;
  }
}


const PString SIPHandler::GetRemotePartyAddress ()
{
  SIPURL cleanAddress = remotePartyAddress;
  cleanAddress.AdjustForRequestURI();

  return cleanAddress.AsString();
}


void SIPHandler::SetExpire (int e)
{
  expire = e;

  if (expire > 0) 
    expireTimer = PTimeInterval (0, (unsigned) (9*expire/10));
}


BOOL SIPHandler::WriteSIPHandler(OpalTransport & transport, void * param)
{
  if (param == NULL)
    return FALSE;

  SIPHandler * handler = (SIPHandler *)param;

  SIPTransaction * transaction = handler->CreateTransaction(transport);
  if (transaction != NULL) {
    handler->callID = transaction->GetMIME().GetCallID();
    if (transaction->Start())
      return TRUE;
  }

    PTRACE(2, "SIP\tDid not start transaction on " << transport);
    return FALSE;
}


BOOL SIPHandler::SendRequest(SIPHandler::State s)
{
  if (transport == NULL)
    return FALSE;

  SetState(expire != 0 ? s : Unsubscribing); 

  return transport->WriteConnect(WriteSIPHandler, this);
}


BOOL SIPHandler::OnReceivedNOTIFY(SIP_PDU & /*response*/)
{
  return FALSE;
}


void SIPHandler::OnReceivedOK(SIP_PDU & /*response*/)
{
  switch (GetState()) {
    case Unsubscribing :
      SetState(Unsubscribed);
      expire = -1;
      break;

    case Subscribing :
    case Refreshing :
      SetState(Subscribed);
      break;

    default :
      PTRACE(2, "SIP\tUnexpected OK in handler with state " << GetState ());
  }
}


void SIPHandler::OnTransactionTimeout(SIPTransaction & /*transaction*/)
{
}


void SIPHandler::OnFailed(SIP_PDU::StatusCodes r)
{
  switch (r) {
    case SIP_PDU::Failure_UnAuthorised :
    case SIP_PDU::Failure_ProxyAuthenticationRequired :
      return;
    case SIP_PDU::Failure_RequestTimeout :
      if (GetState() != Subscribed)
        break;
    default :
      expire = -1;
  }

  SetState(GetState() == Unsubscribing ? Subscribed : Unsubscribed);
}


BOOL SIPHandler::CanBeDeleted()
{
  if (GetState() == Unsubscribed && GetExpire() == -1)
    return TRUE;

  return FALSE;
}


SIPRegisterHandler::SIPRegisterHandler(SIPEndPoint & endpoint,
                                       const PString & to,   /* The to field  */
                                       const PString & authName, 
                                       const PString & pass, 
                                       const PString & realm,
                                       int exp,
                                       const PTimeInterval & minRetryTime, 
                                       const PTimeInterval & maxRetryTime)
  : SIPHandler(endpoint, to, minRetryTime, maxRetryTime)
{
  expire = exp;
  if (expire <= 0)
    expire = endpoint.GetRegistrarTimeToLive().GetSeconds();

  password = pass;
  authUser = authName;
  authRealm = realm;

  originalExpire = exp;

  expireTimer.SetNotifier(PCREATE_NOTIFIER(OnExpireTimeout));
}


SIPRegisterHandler::~SIPRegisterHandler()
{
  PTRACE(4, "SIP\tDeleting SIPRegisterHandler " << GetRemotePartyAddress());
}


SIPTransaction * SIPRegisterHandler::CreateTransaction(OpalTransport &t)
{
  authentication.SetUsername(authUser);
  authentication.SetPassword(password);
  if (!authRealm.IsEmpty())
    authentication.SetAuthRealm(authRealm);   

  if (expire != 0)
    expire = originalExpire;

  return new SIPRegister(endpoint, 
                             t, 
                             GetRouteSet(),
                             targetAddress, 
                             callID, 
                             expire, 
                             retryTimeoutMin, 
                             retryTimeoutMax);
}


void SIPRegisterHandler::OnReceivedOK(SIP_PDU & response)
{
  PString contact = response.GetMIME().GetContact();

  int newExpiryTime = SIPURL(contact).GetParamVars()("expires").AsUnsigned();
  if (newExpiryTime == 0) {
    if (response.GetMIME().HasFieldParameter("expires", contact))
      newExpiryTime = response.GetMIME().GetFieldParameter("expires", contact).AsUnsigned();
    else
      newExpiryTime = response.GetMIME().GetExpires(endpoint.GetRegistrarTimeToLive().GetSeconds());
  }

  if (newExpiryTime > 0 && newExpiryTime < expire)
    expire = newExpiryTime;

  SetExpire(expire);

  if (authRealm.IsEmpty()) {
    SetAuthRealm(targetAddress.GetHostName());
    PTRACE(2, "SIP\tUpdated realm to " << authentication.GetAuthRealm());
  }

  SendStatus(SIP_PDU::Successful_OK);
  SIPHandler::OnReceivedOK(response);
}


void SIPRegisterHandler::OnTransactionTimeout(SIPTransaction & /*transaction*/)
{
  OnFailed(SIP_PDU::Failure_RequestTimeout);
  expireTimer = PTimeInterval (0, 30);
}


void SIPRegisterHandler::OnFailed (SIP_PDU::StatusCodes r)
{ 
  SendStatus(r);
  SIPHandler::OnFailed(r);
}


void SIPRegisterHandler::OnExpireTimeout(PTimer &, INT)
{
  PTRACE(2, "SIP\tStarting REGISTER for binding refresh");

  if (!SendRequest(Refreshing))
    SetState(Unsubscribed);
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


/////////////////////////////////////////////////////////////////////////

SIPSubscribeHandler::SIPSubscribeHandler (SIPEndPoint & endpoint, 
                                          SIPSubscribe::SubscribeType t,
                                          const PString & to,
                                          int exp)
  : SIPHandler(endpoint, to)
{
  lastSentCSeq = 0;
  lastReceivedCSeq = 0;

  expire = exp;
  originalExpire = exp;

  if (expire == 0)
    expire = endpoint.GetNotifierTimeToLive().GetSeconds();
  type = t;
  dialogCreated = FALSE;

  expireTimer.SetNotifier(PCREATE_NOTIFIER(OnExpireTimeout));
}


SIPSubscribeHandler::~SIPSubscribeHandler()
{
  PTRACE(4, "SIP\tDeleting SIPSubscribeHandler " << GetRemotePartyAddress());
}


SIPTransaction * SIPSubscribeHandler::CreateTransaction(OpalTransport &trans)
{ 
  PString partyName;

  if (expire != 0)
    expire = originalExpire;

  if (localPartyAddress.IsEmpty()) {

    if (type == SIPSubscribe::Presence)
      localPartyAddress = endpoint.GetRegisteredPartyName(targetAddress).AsQuotedString();

    else
      localPartyAddress = targetAddress.AsQuotedString();

    localPartyAddress += ";tag=" + OpalGloballyUniqueID().AsString();
  }

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


void SIPSubscribeHandler::OnReceivedOK(SIP_PDU & response)
{
  /* An "expire" parameter in the Contact header has no semantics
   * for SUBSCRIBE. RFC3265, 3.1.1.
   * An answer can only shorten the expires time.
   */
  int responseExpires = response.GetMIME().GetExpires(3600);
  if (responseExpires < expire && responseExpires > 0)
    expire = responseExpires;

  SetExpire(expire);

  /* Update the routeSet according 12.1.2. */
  if (!dialogCreated) {
    PStringList recordRoute = response.GetMIME().GetRecordRoute();
    routeSet.RemoveAll();
    for (int i = recordRoute.GetSize() - 1 ; i >= 0 ; i--)
      routeSet += recordRoute [i];
    if (!response.GetMIME().GetContact().IsEmpty()) 
      targetAddress = response.GetMIME().GetContact();
    dialogCreated = TRUE;
  }

  /* Update the To */
  remotePartyAddress = response.GetMIME().GetTo();

  SIPHandler::OnReceivedOK(response);
}


void SIPSubscribeHandler::OnTransactionTimeout(SIPTransaction & /*transaction*/)
{
  OnFailed(SIP_PDU::Failure_RequestTimeout);
}


BOOL SIPSubscribeHandler::OnReceivedNOTIFY(SIP_PDU & request)
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
    return FALSE;

  else if (requestCSeq < lastReceivedCSeq) {

    endpoint.SendResponse(SIP_PDU::Failure_InternalServerError, *transport, request);
    return FALSE;
  }
  lastReceivedCSeq = requestCSeq;

  PTRACE(3, "SIP\tFound a SUBSCRIBE corresponding to the NOTIFY");
  // We received a NOTIFY corresponding to an active SUBSCRIBE
  // for which we have just unSUBSCRIBEd. That is the final NOTIFY.
  // We can remove the SUBSCRIBE from the list.
  if (GetState() != SIPHandler::Subscribed && GetExpire () == 0) {

    PTRACE(3, "SIP\tFinal NOTIFY received");
    expire = -1;
  }

  PString state = request.GetMIME().GetSubscriptionState();

  // Check the susbscription state
  if (state.Find("terminated") != P_MAX_INDEX) {

    PTRACE(3, "SIP\tSubscription is terminated");
    expire = -1;
  }
  else if (state.Find("active") != P_MAX_INDEX
           || state.Find("pending") != P_MAX_INDEX) {

    PTRACE(3, "SIP\tSubscription is " << state);
    if (request.GetMIME().HasFieldParameter("expire", state)) {

      unsigned sec = 3600;
      sec = request.GetMIME().GetFieldParameter("expire", state).AsUnsigned();
      if (sec < (unsigned) expire)
        SetExpire (sec);
    }
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


BOOL SIPSubscribeHandler::OnReceivedMWINOTIFY(SIP_PDU & request)
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
          return TRUE;
        }
      }
    }

    // Received MWI, unknown messages number
    endpoint.OnMWIReceived (GetRemotePartyAddress(),
                      (SIPSubscribe::MWIType) 0, 
                      "1/0");
  } 

  return TRUE;
}


#if P_EXPAT
BOOL SIPSubscribeHandler::OnReceivedPresenceNOTIFY(SIP_PDU & request)
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
    return FALSE;

  rootElement = xmlPresence.GetRootElement();
  if (rootElement == NULL)
    return FALSE;

  if (rootElement->GetName() != "presence")
    return FALSE;

  tupleElement = rootElement->GetElement("tuple");
  if (tupleElement == NULL)
    return FALSE;

  statusElement = tupleElement->GetElement("status");
  if (statusElement == NULL)
    return FALSE;

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
  return TRUE;
}
#else
BOOL SIPSubscribeHandler::OnReceivedPresenceNOTIFY(SIP_PDU &)
{
  return TRUE;
}
#endif


void SIPSubscribeHandler::OnFailed(SIP_PDU::StatusCodes reason)
{ 
  SIPHandler::OnFailed(reason);
}


void SIPSubscribeHandler::OnExpireTimeout(PTimer &, INT)
{
  PTRACE(2, "SIP\tStarting SUBSCRIBE for binding refresh");

  if (!SendRequest(Refreshing))
    SetState(Unsubscribed);
}



/////////////////////////////////////////////////////////////////////////

SIPPublishHandler::SIPPublishHandler(SIPEndPoint & endpoint,
                                     const PString & to,   /* The to field  */
                                     const PString & b,
                                     int exp)
  : SIPHandler(endpoint, to)
{
  expire = exp;
  originalExpire = exp;
  if (expire == 0)
    expire = endpoint.GetNotifierTimeToLive().GetSeconds();

  expireTimer.SetNotifier(PCREATE_NOTIFIER(OnExpireTimeout));

  publishTimer.SetNotifier(PCREATE_NOTIFIER(OnPublishTimeout));
  publishTimer.RunContinuous (PTimeInterval (0, 5));

  stateChanged = FALSE;

  body = b;
}


SIPPublishHandler::~SIPPublishHandler()
{
  PTRACE(4, "SIP\tDeleting SIPPublishHandler " << GetRemotePartyAddress());
}


SIPTransaction * SIPPublishHandler::CreateTransaction(OpalTransport & t)
{
  if (expire != 0)
    expire = originalExpire;

  return new SIPPublish(endpoint,
                           t, 
                           GetRouteSet(), 
                           targetAddress, 
                           sipETag, 
                           (GetState() == Refreshing)?PString::Empty():body, 
                           expire);
}


void SIPPublishHandler::OnReceivedOK(SIP_PDU & response)
{
  int sec = 3600;
  sec = response.GetMIME().GetExpires(3600);

  if (!response.GetMIME().GetSIPETag().IsEmpty())
    sipETag = response.GetMIME().GetSIPETag();

  if (sec < expire && sec > 0)
    expire = sec;

  SetExpire(expire);

  SIPHandler::OnReceivedOK(response);
}


void SIPPublishHandler::OnTransactionTimeout(SIPTransaction & /*transaction*/)
{
  OnFailed(SIP_PDU::Failure_RequestTimeout);
  expireTimer = PTimeInterval (0, 30);
}


void SIPPublishHandler::OnFailed (SIP_PDU::StatusCodes r)
{ 
  SIPHandler::OnFailed(r);
}


void SIPPublishHandler::OnExpireTimeout(PTimer &, INT)
{
  PTRACE(2, "SIP\tStarting PUBLISH for binding refresh");

  if (!SendRequest(Refreshing))
    SetState(Unsubscribed);
}


void SIPPublishHandler::OnPublishTimeout(PTimer &, INT)
{
  if (GetState() == Subscribed) {
    if (stateChanged) {
      if (!SendRequest())
        SetState(Unsubscribed);
      stateChanged = FALSE;
    }
  }
}


void SIPPublishHandler::SetBody(const PString & b)
{
  stateChanged = TRUE;
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
  : SIPHandler(endpoint, to)
{
  originalExpire = expire;
  body = b;

  SetState(Subscribed);

  expireTimer.SetNotifier(PCREATE_NOTIFIER(OnExpireTimeout));
  SetExpire(300);

  timeoutRetry = 0;
}


SIPMessageHandler::~SIPMessageHandler ()
{
  PTRACE(4, "SIP\tDeleting SIPMessageHandler " << GetRemotePartyAddress());
}


SIPTransaction * SIPMessageHandler::CreateTransaction(OpalTransport &t)
{ 
  SetExpire(expire);
  return new SIPMessage(endpoint, t, targetAddress, routeSet, body);
}


void SIPMessageHandler::OnReceivedOK(SIP_PDU & /*response*/)
{
}


void SIPMessageHandler::OnTransactionTimeout(SIPTransaction & /*transaction*/)
{
  if (timeoutRetry > 2)
    OnFailed(SIP_PDU::Failure_RequestTimeout);
  else {
    SendRequest();
    timeoutRetry++;
  }
}


void SIPMessageHandler::OnFailed(SIP_PDU::StatusCodes reason)
{ 
  endpoint.OnMessageFailed(targetAddress, reason);
}


void SIPMessageHandler::OnExpireTimeout(PTimer &, INT)
{
  SetState(Unsubscribed);
  expire = -1;
}


/////////////////////////////////////////////////////////////////////////

SIPPingHandler::SIPPingHandler (SIPEndPoint & endpoint, 
                                const PString & to)
  : SIPHandler(endpoint, to)
{
  expire = 500; 
  originalExpire = expire;

  expireTimer.SetNotifier(PCREATE_NOTIFIER(OnExpireTimeout));
}


SIPTransaction * SIPPingHandler::CreateTransaction(OpalTransport &t)
{
  return new SIPPing(endpoint, t, targetAddress, body);
}


void SIPPingHandler::OnReceivedOK(SIP_PDU & /*response*/)
{
}


void SIPPingHandler::OnTransactionTimeout(SIPTransaction & /*transaction*/)
{
}


void SIPPingHandler::OnFailed(SIP_PDU::StatusCodes /*reason*/)
{
}


void SIPPingHandler::OnExpireTimeout(PTimer &, INT)
{
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

