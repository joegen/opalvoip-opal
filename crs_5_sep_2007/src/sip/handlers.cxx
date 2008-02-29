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
 * $Log: handlers.cxx,v $
 * Revision 1.9  2007/07/22 13:02:13  rjongbloed
 * Cleaned up selection of registered name usage of URL versus host name.
 *
 * Revision 1.8  2007/07/22 05:07:40  rjongbloed
 * Fixed incorrect disable of time to live timer when get unauthorised error.
 *
 * Revision 1.7  2007/07/03 16:27:14  dsandras
 * Fixed bug reported by Anand R Setlur : crash when registering to
 * non existant domain (ie no transport).
 *
 * Revision 1.6  2007/07/01 12:15:45  dsandras
 * Fixed cut/paste error. Thanks Robert for noticing this!
 *
 * Revision 1.5  2007/06/30 16:43:18  dsandras
 * Make sure transactions are completed before allowing destruction using
 * WaitForTransactionCompletion. If we have a timeout while unsusbscribing,
 * then allow deleting the handler.
 *
 * Revision 1.4  2007/06/10 08:55:12  rjongbloed
 * Major rework of how SIP utilises sockets, using new "socket bundling" subsystem.
 *
 * Revision 1.3  2007/05/22 18:19:52  dsandras
 * Added conditional check for P_EXPAT.
 *
 * Revision 1.2  2007/05/16 01:17:07  csoutheren
 * Added new files to Windows build
 * Removed compiler warnings on Windows
 * Added backwards compatible SIP Register function
 *
 * Revision 1.1  2007/05/15 20:45:09  dsandras
 * Added various handlers to manage subscriptions for presence, message
 * waiting indications, registrations, state publishing,
 * message conversations, ...
 * Adds/fixes support for RFC3856, RFC3903, RFC3863, RFC3265, ...
 * Many improvements over the original SIPInfo code.
 * Code contributed by NOVACOM (http://www.novacom.be) thanks to
 * EuroWeb (http://www.euroweb.hu).
 *
 *
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
  request = NULL;

  targetAddress.Parse(to);
  remotePartyAddress = targetAddress.AsQuotedString();

  transport = endpoint.CreateTransport(targetAddress.GetHostAddress());

  authenticationAttempts = 0;

  const SIPURL & proxy = endpoint.GetProxy();

  // Default routeSet if there is a proxy
  if (!proxy.IsEmpty() && routeSet.GetSize() == 0) 
    routeSet += "sip:" + proxy.GetHostName() + ':' + PString(proxy.GetPort()) + ";lr";

  callID = OpalGloballyUniqueID().AsString() + "@" + PIPSocket::GetHostName();

  SetState (Unsubscribed);
}


SIPHandler::~SIPHandler() 
{
  if (request)
    endpoint.WaitForTransactionCompletion(request);

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
  SIPTransaction *request = NULL;

  if (param == NULL)
    return FALSE;

  SIPHandler * handler = (SIPHandler *)param;

  request = handler->CreateTransaction(transport);
  if (!request) 
    return FALSE;

  if (!request->Start()) {
    PTRACE(2, "SIP\tDid not start transaction on " << transport);
    return FALSE;
  }

  return TRUE;
}


BOOL SIPHandler::SendRequest()
{
  if (transport == NULL)
    return FALSE;

  if (expire == 0)
    SetState(Unsubscribing); 
  else
    SetState(Subscribing); 

  return transport->WriteConnect(WriteSIPHandler, this);
}


BOOL SIPHandler::OnReceivedNOTIFY(SIP_PDU & /*response*/)
{
  return FALSE;
}


void SIPHandler::OnReceivedOK(SIP_PDU & /*response*/)
{
  if (GetState() == Unsubscribing) {
    SetState(Unsubscribed);
    expire = -1;
  }
  else if (GetState() == Subscribing)
    SetState(Subscribed);
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

  if (GetState() == Unsubscribing)
    SetState(Subscribed);
  else if (GetState() == Subscribing)
    SetState(Unsubscribed);
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

  if (request)
    endpoint.WaitForTransactionCompletion(request);

  request = new SIPRegister (endpoint, 
                             t, 
                             GetRouteSet(),
                             targetAddress, 
                             callID, 
                             expire, 
                             retryTimeoutMin, 
                             retryTimeoutMax);

  if (request)
    endpoint.AddTransaction(request);

  return request;
}


void SIPRegisterHandler::OnReceivedOK(SIP_PDU & response)
{
  PString aor;
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

  aor = targetAddress.AsString();
  aor = aor.Mid(4);
  endpoint.OnRegistered(aor, (GetState() != Unsubscribing)); 

  SIPHandler::OnReceivedOK(response);
}


void SIPRegisterHandler::OnTransactionTimeout(SIPTransaction & /*transaction*/)
{
  OnFailed(SIP_PDU::Failure_RequestTimeout);
  expireTimer = PTimeInterval (0, 30);
}


void SIPRegisterHandler::OnFailed (SIP_PDU::StatusCodes r)
{ 
  PString aor;

  aor = targetAddress.AsString();
  aor = aor.Mid(4);
  endpoint.OnRegistrationFailed (aor, r, (GetState() != Unsubscribing));

  SIPHandler::OnFailed(r);
}


void SIPRegisterHandler::OnExpireTimeout(PTimer &, INT)
{
  PTRACE(2, "SIP\tStarting REGISTER for binding refresh");

  SetState(Refreshing);
  if (!SendRequest())
    SetState(Unsubscribed);
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

  if (request)
    endpoint.WaitForTransactionCompletion(request);

  request = new SIPSubscribe (endpoint,
                              trans, 
                              type,
                              GetRouteSet(),
                              targetAddress, 
                              remotePartyAddress,
                              localPartyAddress,
                              callID, 
                              GetNextCSeq(),
                              expire); 

  if (request)
    endpoint.AddTransaction(request);

  return request;
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


BOOL SIPSubscribeHandler::OnReceivedPresenceNOTIFY(SIP_PDU & request)
{
#if P_EXPAT
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
#endif

  return TRUE;
}


void SIPSubscribeHandler::OnFailed(SIP_PDU::StatusCodes reason)
{ 
  SIPHandler::OnFailed(reason);
}


void SIPSubscribeHandler::OnExpireTimeout(PTimer &, INT)
{
  PTRACE(2, "SIP\tStarting SUBSCRIBE for binding refresh");

  SetState(Refreshing);
  if (!SendRequest())
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

  if (request)
    endpoint.WaitForTransactionCompletion(request);

  request = new SIPPublish(endpoint,
                           t, 
                           GetRouteSet(), 
                           targetAddress, 
                           sipETag, 
                           (GetState() == Refreshing)?PString::Empty():body, 
                           expire);
  callID = request->GetMIME().GetCallID();

  if (request)
    endpoint.AddTransaction(request);

  return request;
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

  SetState(Refreshing); 
  if (!SendRequest())
    SetState(Unsubscribed);
}


void SIPPublishHandler::OnPublishTimeout(PTimer &, INT)
{
  if (GetState() == Subscribed) {
    if (stateChanged) {
      SetState(Subscribing); 
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
  if (request)
    endpoint.WaitForTransactionCompletion(request);

  SetExpire(expire);
  request = new SIPMessage(endpoint, t, targetAddress, routeSet, body);
  callID = request->GetMIME().GetCallID();

  if (request)
    endpoint.AddTransaction(request);

  return request;
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
  if (request)
    endpoint.WaitForTransactionCompletion(request);

  request = new SIPPing(endpoint, t, targetAddress, body);
  callID = request->GetMIME().GetCallID();

  if (request)
    endpoint.AddTransaction(request);

  return request;
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

