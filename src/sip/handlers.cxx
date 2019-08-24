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
 *
 * This code implements all or part of the following RFCs
 *
 * RFC 4479 "A Data Model for Presence"
 * RFC 4480 "RPID: Rich Presence Extensions to the Presence Information Data Format (PIDF)"
 */

#include <ptlib.h>
#include <opal_config.h>

#if OPAL_SIP

#ifdef __GNUC__
#pragma implementation "handlers.h"
#endif

#include <sip/handlers.h>

#include <ptclib/pdns.h>
#include <ptclib/enum.h>
#include <sip/sipep.h>
#include <ptclib/pxml.h>
#include <ptclib/random.h>

#include <algorithm>


#define new PNEW
#define PTraceModule() "SIP"


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

SIPHandler::SIPHandler(SIP_PDU::Methods method,
                       SIPEndPoint & ep,
                       const SIPParameters & params,
                       const PString & callID)
  : P_DISABLE_MSVC_WARNINGS(4355, SIPTransactionOwner(*this, ep))
  , m_callID(callID)
  , m_username(params.m_authID)
  , m_password(params.m_password)
  , m_realm(params.m_realm)
  , m_method(method)
  , m_addressOfRecord(params.m_addressOfRecord)
  , m_lastCseq(0)
  , m_lastResponseStatus(SIP_PDU::Information_Trying)
  , m_currentExpireTime(params.m_expire)
  , m_originalExpireTime(params.m_expire)
  , m_offlineExpireTime(params.m_restoreTime)
  , m_state(Unavailable)
  , m_receivedResponse(false)
  , m_expireTimer(ep.GetThreadPool(), ep, m_callID, &SIPHandler::OnExpireTimeout)
  , m_retryForbidden(params.m_retryForbidden)
{
  PTRACE_CONTEXT_ID_NEW();

  m_dialog.SetRequestURI(m_addressOfRecord);
  m_dialog.SetCallID(m_callID);

  SIPURL remoteAddress(params.m_remoteAddress);

  // Look for a "proxy" parameter to override default proxy
  SIPURL proxy = params.m_proxyAddress;
  if (remoteAddress.GetParamVars().Contains(OPAL_PROXY_PARAM)) {
    if (proxy.IsEmpty())
      proxy.Parse(remoteAddress.GetParamVars().Get(OPAL_PROXY_PARAM));
    remoteAddress.SetParamVar(OPAL_PROXY_PARAM, PString::Empty());
  }

  if (proxy.IsEmpty())
    proxy = ep.GetProxy();

  m_dialog.SetProxy(proxy, true);
  m_dialog.SetRemoteURI(remoteAddress);

  PTRACE(4, "Constructed " << m_method << " handler for " << m_addressOfRecord);
}


SIPHandler::~SIPHandler() 
{
  m_expireTimer.Stop();

  PTRACE_IF(4, !m_addressOfRecord.IsEmpty(),
            "Destroyed " << m_method << " handler for " << m_addressOfRecord);
}


bool SIPHandler::ShutDown()
{
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
  }

  AbortPendingTransactions();
  return m_transactions.IsEmpty();
}


void SIPHandler::SetState(SIPHandler::State newState) 
{
  if (m_state == newState)
    return;

  PTRACE(4, "Changing " << GetMethod() << " handler from " << GetState() << " to " << newState
         << ", target=" << GetAddressOfRecord() << ", id=" << GetCallID());

  m_state = newState;

  switch (m_state) {
    case Subscribing :
    case Refreshing :
    case Restoring :
    case Unsubscribing :
      return;

    case Unavailable :
      RetryLater(m_offlineExpireTime);
      break;

    default :
      break;
  }

  if (m_stateQueue.empty())
    return;

  newState = m_stateQueue.front();
  m_stateQueue.pop();
  SendRequest(newState);
}


bool SIPHandler::ActivateState(SIPHandler::State newState, bool resetInterface)
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
    e_Change,
    e_Execute,
    e_Queue
  } StateChangeActions[NumStates][NumStates] =
  {
    /* old-state
           V      new-state-> Subscribed  Subscribing Unavailable Refreshing  Restoring   Unsubscribing Unsubscribed */
    /* Subscribed        */ { e_NoChange, e_Execute,  e_Change,   e_Execute,  e_Execute,  e_Execute,    e_Invalid  },
    /* Subscribing       */ { e_Invalid,  e_Queue,    e_Change,   e_NoChange, e_NoChange, e_Queue,      e_Invalid  },
    /* Unavailable       */ { e_Invalid,  e_Execute,  e_NoChange, e_Execute,  e_Execute,  e_Execute,    e_Invalid  },
    /* Refreshing        */ { e_Invalid,  e_Queue,    e_Change,   e_NoChange, e_NoChange, e_Queue,      e_Invalid  },
    /* Restoring         */ { e_Invalid,  e_Queue,    e_Change,   e_NoChange, e_NoChange, e_Queue,      e_Invalid  },
    /* Unsubscribing     */ { e_Invalid,  e_Invalid,  e_Invalid,  e_Invalid,  e_NoChange, e_NoChange,   e_Invalid  },
    /* Unsubscribed      */ { e_Invalid,  e_Invalid,  e_Invalid,  e_Invalid,  e_Invalid,  e_NoChange,   e_NoChange }
  };

  PSafeLockReadWrite mutex(*this);
  if (!mutex.IsLocked())
    return true;

  if (resetInterface)
    m_dialog.SetInterface(PString::Empty());

  switch (StateChangeActions[GetState()][newState]) {
    case e_Invalid :
      PTRACE(2, "Cannot change state to " << newState << " for " << GetMethod()
             << " handler while in " << GetState() << " state, target="
             << GetAddressOfRecord() << ", id=" << GetCallID());
      return false;

    case e_NoChange :
      PTRACE(4, "Already in state " << GetState() << " for " << GetMethod()
             << " handler, target=" << GetAddressOfRecord() << ", id=" << GetCallID());
      break;

    case e_Change :
      SetState(newState);
      break;

    case e_Execute :
      PTRACE(4, "Executing state change to " << newState << " for " << GetMethod()
             << " handler, target=" << GetAddressOfRecord() << ", id=" << GetCallID());
      return SendRequest(newState);

    case e_Queue :
      PTRACE(3, "Queueing state change to " << newState << " for " << GetMethod()
             << " handler while in " << GetState() << " state, target="
             << GetAddressOfRecord() << ", id=" << GetCallID());
      m_stateQueue.push(newState);
      break;
  }

  return true;
}


PBoolean SIPHandler::SendRequest(SIPHandler::State newState)
{
  SendStatus(SIP_PDU::Information_Trying, newState);

  m_expireTimer.Stop(false); // Stop automatic retry

  SetState(newState);

  SIP_PDU::StatusCodes reason = StartTransaction(PCREATE_NOTIFIER(WriteTransaction));
  if (reason != SIP_PDU::Successful_OK)
    OnFailed(reason);
  return true;
}


void SIPHandler::SetExpire(int e)
{
  m_currentExpireTime = e;
  PTRACE(3, "Expiry time for " << GetMethod() << " set to " << e << " seconds.");

  // Only modify the m_originalExpireTime for future requests if IntervalTooBrief gives
  // a bigger expire time. expire itself will always reflect the proxy decision
  // (bigger or lower), but m_originalExpireTime determines what is used in future 
  // requests and is only modified if interval too brief
  if (m_originalExpireTime < e)
    m_originalExpireTime = e;

  // retry before the expire time.
  if (GetExpire() > 0 && GetState() < Unsubscribing) {
    // Allow for max number of retries at max timeout, plus 10 seconds for good measure
    // But don't make it any shorter than half the expiry time.
    int retryTimeout = GetEndPoint().GetRetryTimeoutMax().GetSeconds();
    int deadband = retryTimeout * GetEndPoint().GetMaxRetries() + 10;
    int timeout = GetExpire();
    if (timeout > deadband*2)
      timeout -= deadband;
    else {
      // Short timeout, try for allowing just one retry, plus a couple of seconds
      deadband = retryTimeout + 2;
      if (timeout > deadband*2)
        timeout -= deadband;
      else
        timeout /= 2; // Really short, just use half the expire time
    }
    m_expireTimer.SetInterval(0, timeout);
    PTRACE(4, "Expiry timer running: " << m_expireTimer);
  }
}


void SIPHandler::WriteTransaction(OpalTransport & transport, bool & succeeded)
{
  PSafePtr<SIPTransaction> transaction = CreateTransaction(transport);
  if (transaction == NULL) {
    PTRACE(2, "Could not create transaction on " << transport);
    return;
  }

  PTRACE_CONTEXT_ID_TO(transaction);

  SIPMIMEInfo & mime = transaction->GetMIME();

  if (GetInterface().IsEmpty()) {
    if (m_lastCseq == 0)
      m_lastCseq = mime.GetCSeqIndex();
    else
      transaction->SetCSeq(m_lastCseq);
  }

  for (PStringToString::iterator it = m_mime.begin(); it != m_mime.end(); ++it)
    mime.SetAt(it->first, it->second);

  if (GetState() == Unsubscribing)
    mime.SetExpires(0);

  succeeded = transaction->Start();
}


PBoolean SIPHandler::OnReceivedNOTIFY(SIP_PDU & /*response*/)
{
  return false;
}


void SIPHandler::OnReceivedResponse(SIPTransaction & transaction, SIP_PDU & response)
{
  SIPTransactionOwner::OnReceivedResponse(transaction, response);

  m_lastResponseStatus = response.GetStatusCode();
  unsigned responseClass = m_lastResponseStatus/100;
  if (responseClass < 2)
    return; // Don't do anything with pending responses.

  // Received a response, so indicate that and collapse the forking on multiple interfaces.
  m_receivedResponse = true;

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

    case SIP_PDU::Failure_Forbidden:
      OnReceivedForbidden(transaction, response);
      break;

    default :
      if (responseClass == 2)
        OnReceivedOK(transaction, response);
      else
        OnFailed(response.GetStatusCode());
  }
}

void SIPHandler::OnReceivedForbidden(SIPTransaction & PTRACE_PARAM(transaction), SIP_PDU & response)
{
  OnFailed(SIP_PDU::Failure_Forbidden);
  if (m_retryForbidden) {
    PTRACE(2, "SIP\tRetrying on 403 Forbidden (in violation of RFC 3261) on " << transaction);
    RetryLater(response.GetMIME().GetInteger("Retry-After", m_offlineExpireTime));
  }
}

void SIPHandler::OnReceivedIntervalTooBrief(SIPTransaction & /*transaction*/, SIP_PDU & response)
{
  SetExpire(response.GetMIME().GetMinExpires());

  // Restart the transaction with new time to live
  SendRequest(GetState());
}


void SIPHandler::OnReceivedTemporarilyUnavailable(SIPTransaction & /*transaction*/, SIP_PDU & response)
{
  OnFailed(SIP_PDU::Failure_TemporarilyUnavailable);
  SetState(Unavailable);
  RetryLater(response.GetMIME().GetInteger("Retry-After"));
}


void SIPHandler::OnReceivedAuthenticationRequired(SIPTransaction &, SIP_PDU & response)
{
  // If either username or password blank, try and fine values from other
  // handlers which might be logged into the realm, or the proxy, if one.
  SIP_PDU::StatusCodes status = HandleAuthentication(response);
  if (status != SIP_PDU::Successful_OK) {
    OnFailed(status);
    return;
  }

  // If we changed realm (or hadn't got one yet) update the handler database
  if (m_realm != m_authentication->GetAuthRealm()) {
    m_realm = m_authentication->GetAuthRealm();
    PTRACE(3, "Auth realm set to " << m_realm);
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
      PTRACE(2, "Unexpected 200 OK in handler with state " << GetState());
  }
}


void SIPHandler::OnTransactionFailed(SIPTransaction & transaction)
{
  SIPTransactionOwner::OnTransactionFailed(transaction);

  if (transaction.IsCanceled()) {
    PTRACE(4, "Not reporting canceled forked.");
    return;
  }

  if (!m_transactions.IsEmpty()) {
    PTRACE(4, "Not reporting failed forked transaction.");
    return;
  }

  OnFailed(transaction.GetStatusCode());
}


void SIPHandler::RetryLater(unsigned after)
{
  if (after == 0 || GetExpire() == 0)
    return;

  PTRACE(3, "Retrying " << GetMethod() << ' ' << GetAddressOfRecord() << " after " << after << " seconds.");
  m_expireTimer.SetInterval(0, after); // Keep trying to get it back
}


void SIPHandler::OnFailed(SIP_PDU::StatusCodes code)
{
  m_lastResponseStatus = code;

  SendStatus(code, GetState());

  switch (code) {
    case SIP_PDU::Failure_TemporarilyUnavailable:
      return;

    case SIP_PDU::Local_TransportError :
    case SIP_PDU::Local_Timeout :
    case SIP_PDU::Failure_RequestTimeout :
    case SIP_PDU::Failure_ServiceUnavailable:
    case SIP_PDU::Failure_ServerTimeout:
      if (GetState() != Unsubscribing) {
        SetState(Unavailable);
        return;
      }
      break;

    case SIP_PDU::Failure_Forbidden:
      if (m_retryForbidden && GetState() != Unsubscribing) {
        SetState(Unavailable);
        return;
      }
      break;

    default :
      break;
  }

  PTRACE(4, "Not retrying " << GetMethod() << " due to error response " << code);
  m_currentExpireTime = 0; // OK, stop trying
  m_expireTimer.Stop(false);
  if (GetState() != Unsubscribing)
    SendStatus(SIP_PDU::Successful_OK, Unsubscribing);
  SetState(Unsubscribed); // Allow garbage collection thread to clean up
}


void SIPHandler::SendStatus(SIP_PDU::StatusCodes, State)
{
}


void SIPHandler::OnExpireTimeout()
{
  switch (GetState()) {
    case Subscribed :
      PTRACE(2, "Starting " << GetMethod() << " for binding refresh");
      if (SendRequest(Refreshing))
        return;
      break;

    case Unavailable :
      PTRACE(2, "Starting " << GetMethod() << " for offline retry");
      if (SendRequest(Restoring))
        return;
      break;

    default :
      return;
  }

  SetState(Unavailable);
}


///////////////////////////////////////////////////////////////////////////////

static atomic<uint32_t> LastRegId;

SIPRegisterHandler::SIPRegisterHandler(SIPEndPoint & endpoint, const SIPRegister::Params & params)
  : SIPHandler(SIP_PDU::Method_REGISTER, endpoint, params)
  , m_parameters(params)
  , m_sequenceNumber(0)
  , m_rfc5626_reg_id(++LastRegId)
{
  // Foer some bizarre reason, even though REGISTER does not create a dialog,
  // some registrars insist that you have a from tag ...
  SIPURL local = params.m_localAddress.IsEmpty() ? params.m_addressOfRecord : params.m_localAddress;
  local.SetTag();
  m_parameters.m_localAddress = local.AsQuotedString();
  m_parameters.m_proxyAddress = GetProxy().AsString();
}



static PString CreateCiscoDeviceName(const PString & pattern, const PString & name)
{
  if (name.GetLength() > pattern.GetLength())
    return name(name.GetLength() - pattern.GetLength(), name.GetLength());

  if (name.GetLength() < pattern.GetLength())
    return pattern(0, pattern.GetLength() - name.GetLength() - 1) + name;

  return name;
}


PString SIPRegisterHandler::CreateRegisterContact(const OpalTransportAddress & address, int q)
{
  SIPURL contact(m_addressOfRecord.GetUserName(), address, 0, m_addressOfRecord.GetScheme());
  contact.Sanitise(SIPURL::RegContactURI);

  if (m_parameters.m_compatibility == SIPRegister::e_RFC5626 || m_parameters.m_compatibility == SIPRegister::e_Cisco) {
    if (m_parameters.m_instanceId.IsNULL())
      m_parameters.m_instanceId = PGloballyUniqueID();

    PStringOptions & params = contact.GetFieldParameters();
    params.Set("+sip.instance", "<urn:uuid:" + m_parameters.m_instanceId.AsString() + '>');
    params.SetInteger("reg-id", m_rfc5626_reg_id);
    if (m_parameters.m_compatibility == SIPRegister::e_Cisco) {
      params.Set("+u.sip!devicename.ccm.features.cisco.com", "\"" + CreateCiscoDeviceName(m_parameters.m_ciscoDevicePattern, contact.GetUserName()) + "\"");
      params.Set("+u.sip!model.ccm.cisco.com", "\"" + m_parameters.m_ciscoDeviceType + "\"");
    }
  }

  if (q >= 0)
    contact.GetFieldParameters().Set("q", q < 1000 ? psprintf("0.%03u", q) : "1");

  return contact.AsQuotedString();
}


SIPTransaction * SIPRegisterHandler::CreateTransaction(OpalTransport & transport)
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
      if (GetState() == Refreshing && !m_contactAddresses.empty()) {
        params.m_contactAddress = m_contactAddresses.ToString();
        if (params.m_compatibility == SIPRegister::e_Cisco)
          params.m_body.RemoveAll();
      }
      else {
        bool singleContact;
        PString localAddress;
        switch (params.m_compatibility) {
          default :
          case SIPRegister::e_FullyCompliant :
            singleContact = false;
            localAddress = PIPSocket::Address::GetAny().AsString();
            break;

          case SIPRegister::e_CannotRegisterMultipleContacts :
            singleContact = true;
            localAddress = transport.GetLocalAddress().GetHostName();
            break;

          case SIPRegister::e_CannotRegisterPrivateContacts :
            singleContact = false;
            localAddress = transport.GetLocalAddress().GetHostName();
            break;

          case SIPRegister::e_HasApplicationLayerGateway :
            singleContact = false;
            localAddress = transport.GetInterface();
            break;

          case SIPRegister::e_RFC5626 :
            singleContact = true;
            localAddress = transport.GetInterface();
            break;

          case SIPRegister::e_Cisco :
            singleContact = true;
            localAddress = transport.GetInterface();
            params.m_body.RemoveAll();
            params.m_body.AddPart("<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
                                  "<x-cisco-remotecc-request>"
                                  "<bulkregisterreq>"
                                  "<contact all=\"true\">"
                                  "<register></register>"
                                  "</contact>"
                                  "</bulkregisterreq>"
                                  "</x-cisco-remotecc-request>",
                                  "application/x-cisco-remotecc-request+xml",
                                  "session;handling=optional");
            params.m_body.AddPart("<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
                                  "<x-cisco-remotecc-request>"
                                  " <optionsind>"
                                  "  <combine max=\"6\">"
                                  "   <remotecc>"
                                  "    <status></status>"
                                  "   </remotecc>"
                                  "   <service-control></service-control>"
                                  "  </combine>"
                                  "  <dialog usage=\"hook status\">"
                                  "   <unot></unot>"
                                  "   <sub></sub>"
                                  "  </dialog>"
                                  "  <dialog usage=\"shared line\">"
                                  "   <unot></unot>"
                                  "   <sub></sub>"
                                  "  </dialog>"
                                  "  <presence usage=\"blf speed dial\">"
                                  "   <unot></unot>"
                                  "   <sub></sub>"
                                  "  </presence>"
                                  "  <joinreq></joinreq>"
                                  "  <cfwdall-anyline>No</cfwdall-anyline>"
                                  "  <coaching></coaching>"
                                  "  <oosalarm></oosalarm>"
                                  " </optionsind>"
                                  "</x-cisco-remotecc-request>",
                                  "application/x-cisco-remotecc-request+xml",
                                  "session;handling=optional");

            params.m_mime.AddSupported("replaces");
            params.m_mime.AddSupported("join");
            params.m_mime.AddSupported("sdp-anat");
            params.m_mime.AddSupported("norefersub");
            params.m_mime.AddSupported("extended-refer");
            params.m_mime.AddSupported("X-cisco-callinfo");
            params.m_mime.AddSupported("X-cisco-serviceuri");
            params.m_mime.AddSupported("X-cisco-escapecodes");
            params.m_mime.AddSupported("X-cisco-service-control");
            params.m_mime.AddSupported("X-cisco-srtp-fallback");
            params.m_mime.AddSupported("X-cisco-monrec");
            params.m_mime.AddSupported("X-cisco-config");
            params.m_mime.AddSupported("X-cisco-sis-5.1.0");
            params.m_mime.AddSupported("X-cisco-xsi-8.5.1");
          
            break;
        }

        // Want interface and protocol, wildcard the port
        OpalTransportAddress localInterface(localAddress, 65535, singleContact ? transport.GetProtoPrefix() : OpalTransportAddress::IpPrefix());

        unsigned qvalue = 1000;
        OpalTransportAddressArray listenerInterfaces = GetEndPoint().GetInterfaceAddresses(&transport);
        for (PINDEX i = 0; i < listenerInterfaces.GetSize(); ++i) {
          OpalTransportAddress listenerInterface = listenerInterfaces[i];

          /* If fully compliant, put into the contact field all the bound
              interfaces. If special then we only put into the contact
              listeners that are on the same interface. If translated by STUN
              then only the external address of the interface is used. */
          if (localInterface.IsEquivalent(listenerInterface, true)) {
            if (singleContact) {
              params.m_contactAddress += CreateRegisterContact(listenerInterface, -1);
              break;
            }

            if (!params.m_contactAddress.IsEmpty())
              params.m_contactAddress += ", ";
            params.m_contactAddress += CreateRegisterContact(listenerInterface, qvalue);

            qvalue -= 1000/listenerInterfaces.GetSize();
          }
        }
        if (params.m_contactAddress.IsEmpty()) {
          if (listenerInterfaces.IsEmpty())
            params.m_contactAddress = CreateRegisterContact(localAddress, -1);
          else
            params.m_contactAddress = CreateRegisterContact(listenerInterfaces[0], -1);
        }
      }
    }
  }

  return new SIPRegister(*this, transport, GetCallID(), m_sequenceNumber, params);
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
      PTRACE(2, "Unexpected 200 OK in handler with state " << previousState);
      return;
  }

  SIPMIMEInfo & mime = response.GetMIME();

  mime.GetProductInfo(m_productInfo);

  m_serviceRoute.FromString(mime("Service-Route"));

  unsigned systemDefaultExpires = GetEndPoint().GetRegistrarTimeToLive().GetSeconds();
  SIPURLList requestedContacts;
  transaction.GetMIME().GetContacts(requestedContacts, systemDefaultExpires);

  SIPURLList replyContacts;
  mime.GetContacts(replyContacts, systemDefaultExpires);

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
    PStringOptions & params = reply->GetFieldParameters();
    // More Cisco idiocy
    for (PStringOptions::iterator it = params.begin(); it != params.end(); ++it) {
      if (it->first.Find("cisco.com") != P_MAX_INDEX)
        it->second = it->second.ToLiteral();
    }

    if (reply->GetTransportAddress() == externalAddress) {
      externalAddress.MakeEmpty(); // Clear this so no further action taken
      m_externalAddress.MakeEmpty();
      ++reply;
    }
    else if (std::find(requestedContacts.begin(), requestedContacts.end(), *reply) != requestedContacts.end())
      ++reply;
    else
      replyContacts.erase(reply++);
  }

  if (replyContacts.empty() && GetExpire() != 0) {
    // Huh? Nothing we tried to register, registered! How is that possible?
    PTRACE(2, "REGISTER returned no Contact addresses we requested, not really registered.");
    SendRequest(Unsubscribing);
    SendStatus(SIP_PDU::GlobalFailure_NotAcceptable, previousState);
    return;
  }

  if (m_parameters.m_compatibility == SIPRegister::e_RFC5626) {
    bool notRFC5626 = true;
    for (SIPURLList::iterator contact = replyContacts.begin(); contact != replyContacts.end(); ++contact) {
      if (!contact->GetFieldParameters().GetString("+sip.instance").IsEmpty()) {
        notRFC5626 = false;
        break;
      }
    }
    if (notRFC5626) {
      PTRACE(3, "Registrar has not indicated support for RFC5626");
      m_parameters.m_compatibility = SIPRegister::e_FullyCompliant;
    }
  }

  // If this is the final (possibly one and only) REGISTER, process it
  if (m_externalAddress == externalAddress) {
    int minExpiry = INT_MAX;
    for (SIPURLList::iterator contact = replyContacts.begin(); contact != replyContacts.end(); ++contact) {
      long expires = contact->GetFieldParameters().GetInteger("expires");
      if (expires > 0 && minExpiry > expires)
        minExpiry = expires;
    }
    if (minExpiry != INT_MAX) {
      SetExpire(minExpiry);
      m_contactAddresses = replyContacts;
      SetState(Subscribed);
      SendStatus(SIP_PDU::Successful_OK, previousState);
      return;
    }

    PTRACE(4, "No Contact addresses we requested have non-zero expiry.");
    SetExpire(0);
  }

  if (GetExpire() == 0) {
    // If we had discovered we are behind NAT and had unregistered, re-REGISTER with new addresses
    PTRACE(2, "Re-registering NAT address change (" << m_contactAddresses << ") to " << externalAddress);
    for (SIPURLList::iterator contact = m_contactAddresses.begin(); contact != m_contactAddresses.end(); ++contact)
      contact->SetHostAddress(externalAddress);
    m_contactAddresses.unique();
    SetExpire(m_originalExpireTime);
  }
  else {
    /* If we got here then we have done a successful register, but registrar indicated
       that we are behind firewall. Unregister what we just registered */
    for (SIPURLList::iterator contact = replyContacts.begin(); contact != replyContacts.end(); ++contact)
      contact->GetFieldParameters().Remove("expires");
    PTRACE(2, "Remote indicated change of REGISTER Contact address(s) (" << replyContacts
           << ") required due to NAT address " << externalAddress << ", previous=" << m_externalAddress);
    m_contactAddresses = replyContacts;
    SetExpire(0);
  }

  // Remember (possibly new) NAT address
  m_externalAddress = externalAddress;

  SendRequest(Refreshing);
  SendStatus(SIP_PDU::Information_Trying, previousState);
}


PBoolean SIPRegisterHandler::SendRequest(SIPHandler::State s)
{
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

  m_parameters.m_compatibility = params.m_compatibility;
  m_parameters.m_contactAddress = params.m_contactAddress;
  m_contactAddresses.clear();

  PTRACE(4, "REGISTER parameters updated.");
}


/////////////////////////////////////////////////////////////////////////

SIPSubscribeHandler::SIPSubscribeHandler(SIPEndPoint & endpoint, const SIPSubscribe::Params & params)
  : SIPHandler(SIP_PDU::Method_SUBSCRIBE, endpoint, params)
  , m_parameters(params)
  , m_unconfirmed(true)
  , m_packageHandler(SIPEventPackageFactory::CreateInstance(params.m_eventPackage))
{
  PTRACE_CONTEXT_ID_TO(m_packageHandler);

  if (m_parameters.m_contentType.IsEmpty() && (m_packageHandler != NULL))
    m_parameters.m_contentType = m_packageHandler->GetContentType();
}


SIPSubscribeHandler::~SIPSubscribeHandler()
{
  delete m_packageHandler;
}


SIPTransaction * SIPSubscribeHandler::CreateTransaction(OpalTransport & transport)
{ 
  // Do all the following here as must be after we have called GetTransport()
  // which sets various fields correctly for transmission
  if (!m_dialog.IsEstablished()) {
    if (m_parameters.m_eventPackage.IsWatcher())
      m_parameters.m_localAddress = GetAddressOfRecord().AsString();

    m_dialog.SetRemoteURI(m_parameters.m_addressOfRecord);
    m_dialog.SetLocalURI(m_parameters.m_localAddress.IsEmpty() ? m_parameters.m_addressOfRecord : m_parameters.m_localAddress);
  }

  m_parameters.m_expire = GetState() != Unsubscribing ? GetExpire() : 0;
  return new SIPSubscribe(*this, transport, m_dialog, m_parameters);
}


void SIPSubscribeHandler::OnFailed(SIP_PDU::StatusCodes responseCode)
{
  if (responseCode != SIP_PDU::Failure_TransactionDoesNotExist || GetState() == Unsubscribing) {
    SIPHandler::OnFailed(responseCode);
    return;
  }

  // Resubscribe as previous subscription totally lost, reset the dialog and
  // hopefully that Call-ID not changing is not a problem.
  m_parameters.m_addressOfRecord = GetAddressOfRecord().AsString();
  m_dialog.SetLocalTag(PString::Empty());     // If this is empty
  m_dialog.SetLocalURI(GetAddressOfRecord()); // This will regenerate tag
  m_dialog.SetRemoteTag(PString::Empty());
  SendRequest(GetState());
}


void SIPSubscribeHandler::WriteTransaction(OpalTransport & transport, bool & succeeded)
{
  m_dialog.SetForking(GetInterface().IsEmpty());
  SIPHandler::WriteTransaction(transport, succeeded);
  m_dialog.SetForking(false);
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

    case Unavailable :
    case Restoring :
      status.m_wasSubscribing = true;
      status.m_reSubscribing = code/100 != 2;
      break;

    case Unsubscribing :
    case Unsubscribed :
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
  m_dialog.Update(response);

  if (GetState() != Unsubscribing)
    SIPHandler::OnReceivedOK(transaction, response);
}


PBoolean SIPSubscribeHandler::OnReceivedNOTIFY(SIP_PDU & request)
{
  if (m_unconfirmed) {
    SendStatus(SIP_PDU::Successful_OK, GetState());
    m_unconfirmed = false;
  }

  SIPMIMEInfo & requestMIME = request.GetMIME();

  requestMIME.GetProductInfo(m_productInfo);

  // Remember response for retries by using transaction, note: deleted by SIPEndPoint
  SIPResponse * response = new SIPResponse(m_sipEndpoint, request, SIP_PDU::Failure_BadRequest);
  PTRACE_CONTEXT_ID_TO(response);

  PStringToString subscriptionStateInfo;
  PCaselessString subscriptionState = requestMIME.GetSubscriptionState(subscriptionStateInfo);
  if (subscriptionState.IsEmpty()) {
    PTRACE(2, "NOTIFY received without Subscription-State, assuming 'active'");
    subscriptionState = "active";
  }

  // Check the susbscription state
  if (subscriptionState == "terminated") {
    PTRACE(3, "Subscription is terminated, state=" << GetState());
    response->SetStatusCode(SIP_PDU::Successful_OK);
    response->Send();

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
    PTRACE(2, "NOTIFY received for incorrect event \"" << requestEvent << "\", requires \"" << m_parameters.m_eventPackage << '"');
    response->SetStatusCode(SIP_PDU::Failure_BadEvent);
    response->GetMIME().SetAt("Allow-Events", m_parameters.m_eventPackage);
    return response->Send();
  }

  // Check any Requires field
  PStringSet require = requestMIME.GetRequire();
  if (m_parameters.m_eventList)
    require -= "eventlist";
  if (!require.IsEmpty()) {
    PTRACE(2, "NOTIFY contains unsupported Require field \"" << setfill(',') << require << '"');
    response->SetStatusCode(SIP_PDU::Failure_BadExtension);
    response->GetMIME().SetUnsupported(require);
    response->SetInfo("Unsupported Require");
    return response->Send();
  }

  // Check if we know how to deal with this event
  if (m_packageHandler == NULL && m_parameters.m_onNotify.IsNULL()) {
    PTRACE(2, "No handler for NOTIFY received for event \"" << requestEvent << '"');
    response->SetStatusCode(SIP_PDU::Failure_InternalServerError);
    return response->Send();
  }

  // Adjust timeouts if state is a go
  if (subscriptionState == "active" || subscriptionState == "pending") {
    PTRACE(3, "Subscription is " << GetState() << " for " << GetAddressOfRecord() << ' ' << GetEventPackage());
    PString expire = SIPMIMEInfo::ExtractFieldParameter(GetState(), "expire");
    if (!expire.IsEmpty())
      SetExpire(expire.AsUnsigned());
  }

  // Check for empty body, if so then is OK, just a ping ...
  if (request.GetEntityBody().IsEmpty()) {
    response->SetStatusCode(SIP_PDU::Successful_OK);
    return response->Send();
  }

  // check the ContentType
  if (!m_parameters.m_contentType.IsEmpty()) {
    PCaselessString requestContentType = requestMIME.GetContentType();
    if (
        (m_parameters.m_contentType.Find(requestContentType) == P_MAX_INDEX) && 
        !(m_parameters.m_eventList && (requestContentType == "multipart/related")) &&
        !((m_packageHandler != NULL) && m_packageHandler->ValidateContentType(requestContentType, requestMIME))
        ) {
      PTRACE(2, "NOTIFY contains unsupported Content-Type \""
              << requestContentType << "\", expecting \"" << m_parameters.m_contentType << "\" or multipart/related or other validated type");
      response->SetStatusCode(SIP_PDU::Failure_UnsupportedMediaType);
      response->GetMIME().SetAt("Accept", m_parameters.m_contentType);
      response->SetInfo("Unsupported Content-Type");
      return response->Send();
    }
  }

  PMultiPartList parts;
  if (!m_parameters.m_eventList || !requestMIME.DecodeMultiPartList(parts, request.GetEntityBody())) {
    if (DispatchNOTIFY(request, *response))
      return response->Send();
    return true;
  }

  // If GetMultiParts() returns true there as at least one part and that
  // part must be the meta list, guranteed by DecodeMultiPartList()
  PMultiPartList::iterator iter = parts.begin();

  // First part is always Meta Information
  if (iter->m_mime.GetString(PMIMEInfo::ContentTypeTag) != "application/rlmi+xml") {
    PTRACE(2, "NOTIFY received without RLMI as first multipart body");
    response->SetInfo("No Resource List Meta-Information");
    return response->Send();
  }

#if OPAL_PTLIB_EXPAT
  PXML xml;
  if (!xml.Load(iter->m_textBody)) {
    PTRACE(2, "NOTIFY received with illegal RLMI\n"
              "Line " << xml.GetErrorLine() << ", Column " << xml.GetErrorColumn() << ": " << xml.GetErrorString());
    response->SetInfo("Bad Resource List Meta-Information");
    return response->Send();
  }

  if (parts.GetSize() == 1) {
    response->SetStatusCode(SIP_PDU::Successful_OK);
    return response->Send();
  }

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
            pduMIME.SetFrom(uri);
            pduMIME.SetTo(uri);
            break;
          }
        }
      }
    }

    if (!DispatchNOTIFY(pdu, *response))
      return true;
  }
#else
  for ( ; iter != parts.end(); ++iter) {
    SIP_PDU pdu(request.GetMethod());
    pdu.GetMIME().AddMIME(iter->m_mime);
    pdu.SetEntityBody(iter->m_textBody);

    if (!DispatchNOTIFY(pdu, *response))
      return true;
  }
#endif

  return response->Send();
}


bool SIPSubscribeHandler::DispatchNOTIFY(SIP_PDU & request, SIP_PDU & response)
{
  SIPSubscribe::NotifyCallbackInfo notifyInfo(*this, GetEndPoint(), request, response);

  if (!m_parameters.m_onNotify.IsNULL()) {
    PTRACE(4, "Calling NOTIFY callback for " << GetEventPackage() << " of AOR \"" << GetAddressOfRecord() << "\"");
    m_parameters.m_onNotify(*this, notifyInfo);
    return notifyInfo.m_sendResponse;
  }

  if (m_packageHandler != NULL) {
    PTRACE(4, "Calling package NOTIFY handler for " << GetEventPackage() << " of AOR \"" << GetAddressOfRecord() << "\"");
    m_packageHandler->OnReceivedNOTIFY(notifyInfo);
    return notifyInfo.m_sendResponse;
  }

  PTRACE(2, "No NOTIFY handler for " << GetEventPackage() << " of AOR \"" << GetAddressOfRecord() << "\"");
  response.SetStatusCode(SIP_PDU::Failure_BadEvent);
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
    PTRACE(3, "Received repeated NOTIFY of " << handler.GetAddressOfRecord() << ", already processed.");
    return false;
  }

  // if this is a full list of watcher info, we can empty out our pending lists
  if (fullUpdate) {
    PTRACE(3, "Received full update NOTIFY of " << handler.GetAddressOfRecord());
    m_expectedSequenceNumber = newSequenceNumber+1;
    return true;
  }

  if (m_expectedSequenceNumber != UINT_MAX) {
    PTRACE(2, "SIPPres\tReceived partial update NOTIFY of " << handler.GetAddressOfRecord() << ", but still awaiting full update.");
    return false;
  }

  if (newSequenceNumber == m_expectedSequenceNumber) {
    PTRACE(3, "Received partial update NOTIFY of " << handler.GetAddressOfRecord());
    ++m_expectedSequenceNumber;
    return true;
  }

  PTRACE(2, "Received out of sequence partial update, resubscribing " << handler.GetAddressOfRecord());
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

    const SIPURL & aor = notifyInfo.m_handler.GetAddressOfRecord();
    PString account = info.Get("Message-Account");
    SIPURL accountURI(account);
    if (account.IsEmpty() || aor.GetUserName() == account ||
            (accountURI.GetUserName() == "asterisk" && accountURI.GetHostName() == aor.GetHostPort()))
      account = aor.AsString();

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
                                 (info.Get("Messages-Waiting") *= "yes") ? "yes" : "no");
  }
};

PFACTORY_CREATE(SIPEventPackageFactory, SIPMwiEventPackageHandler, SIPSubscribe::MessageSummary);


///////////////////////////////////////////////////////////////////////////////

#if OPAL_SIP_PRESENCE

static PConstCaselessString const SIPPresenceEventPackageContentType("application/pidf+xml");

// This package is on for backward compatibility, presence should now use the
// the OpalPresence classes to manage SIP presence.
class SIPPresenceEventPackageHandler : public SIPEventPackageHandler
{
  virtual PCaselessString GetContentType() const
  {
    return SIPPresenceEventPackageContentType;
  }

  virtual void OnReceivedNOTIFY(SIPSubscribe::NotifyCallbackInfo & notifyInfo)
  {
    PTRACE(4, "Processing presence NOTIFY using old API");

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

PFACTORY_CREATE(SIPEventPackageFactory, SIPPresenceEventPackageHandler, SIPSubscribe::Presence);

#endif // OPAL_SIP_PRESENCE

///////////////////////////////////////////////////////////////////////////////

#if OPAL_PTLIB_EXPAT

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

#endif // OPAL_PTLIB_EXPAT


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

#if OPAL_PTLIB_EXPAT
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
        for (info.m_state = SIPDialogNotification::BeginStates; info.m_state < SIPDialogNotification::EndStates; ++info.m_state) {
          if (str == info.GetStateName())
            break;
        }

        str = stateElement->GetAttribute("event");
        for (info.m_eventType = SIPDialogNotification::BeginEvents; info.m_eventType < SIPDialogNotification::EndEvents; ++info.m_eventType) {
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
#endif // OPAL_PTLIB_EXPAT

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

PFACTORY_CREATE(SIPEventPackageFactory, SIPDialogEventPackageHandler, SIPSubscribe::Dialog);


///////////////////////////////////////////////////////////////////////////////

class SIPRegEventPackageHandler : public SIPEventPackageHandler
{
  virtual PCaselessString GetContentType() const
  {
    return "application/reginfo+xml";
  }

  virtual void OnReceivedNOTIFY(SIPSubscribe::NotifyCallbackInfo & notifyInfo)
  {
#if OPAL_PTLIB_EXPAT
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
#endif // OPAL_PTLIB_EXPAT
  }
};

PFACTORY_CREATE(SIPEventPackageFactory, SIPRegEventPackageHandler, SIPSubscribe::Reg);


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
  if ((PINDEX)state < PARRAYSIZE(Names) && Names[state] != NULL)
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
  if (!m_callId.IsEmpty())
    strm << " call-id=\"" << m_callId << '"';
  if (!m_local.m_dialogTag.IsEmpty())
    strm << " local-tag=\"" << m_local.m_dialogTag << '"';
  if (!m_remote.m_dialogTag.IsEmpty())
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
  while (state < EndStates && str != GetStateName(state))
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
  , m_reason(Deactivated)
  , m_packageHandler(SIPEventPackageFactory::CreateInstance(eventPackage))
{
  PTRACE_CONTEXT_ID_TO(m_packageHandler);

  m_dialog = dialog;
}


SIPNotifyHandler::~SIPNotifyHandler()
{
  delete m_packageHandler;
}


SIPTransaction * SIPNotifyHandler::CreateTransaction(OpalTransport & transport)
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

  return new SIPNotify(*this, transport, m_dialog, m_eventPackage, state, body);
}


PBoolean SIPNotifyHandler::SendRequest(SIPHandler::State state)
{
  // If times out, i.e. Refreshing, then this is actually a time out unsubscribe.
  if (state == Refreshing)
    m_reason = Timeout;

  return SIPHandler::SendRequest(state == Refreshing ? Unsubscribing : state);
}


void SIPNotifyHandler::WriteTransaction(OpalTransport & transport, bool & succeeded)
{
  m_dialog.SetForking(GetInterface().IsEmpty());
  SIPHandler::WriteTransaction(transport, succeeded);
  m_dialog.SetForking(false);
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
}


SIPTransaction * SIPPublishHandler::CreateTransaction(OpalTransport & transport)
{
  if (m_body.IsEmpty())
    SetState(Unsubscribing);

  m_parameters.m_expire = m_originalExpireTime;
  return new SIPPublish(*this,
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

#if OPAL_SIP_PRESENCE

// defined in RFC 4480
static const char * const RFC4480ActivitiesInit[] = {
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

static PStringSet const RFC4480Activities(PARRAYSIZE(RFC4480ActivitiesInit), RFC4480ActivitiesInit, true);


// Cisco specific extensions
static const char * const CiscoActivitiesInit[] = {
  "available",
  "away",
  "dnd"
};

static PStringSet const CiscoActivities(PARRAYSIZE(CiscoActivitiesInit), CiscoActivitiesInit, true);


// RFC 5196 extensions
static const char * const RFC5196CapabilitiesInit[] = {
  "audio",
  "application",
  "data",
  "control",
  "video",
  "text",
  "message",
  "automata",
  "type"
};

static PStringSet const RFC5196Capabilities(PARRAYSIZE(RFC5196CapabilitiesInit), RFC5196CapabilitiesInit, true);



SIPPresenceInfo::SIPPresenceInfo(SIP_PDU::StatusCodes status, bool subscribing)
{
  if (status/100 == 2)
    return;

  m_note = SIP_PDU::GetStatusCodeDescription(status);

  if (!subscribing) {
    m_state = NoPresence;
    return;
  }

  switch (status) {
    case SIP_PDU::Failure_NotFound :
      m_state = UnknownUser;
      break;

    case SIP_PDU::Failure_Forbidden :
    case SIP_PDU::Failure_UnAuthorised :
      m_state = Forbidden;
      break;

    default :
      m_state = InternalError;
  }
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


PString SIPPresenceInfo::AsXML() const
{
  if (m_entity.IsEmpty() || m_service.IsEmpty()) {
    PTRACE(1, "Cannot encode Presence XML as no address or no id.");
    return PString::Empty();
  }

  PStringStream xml;

  xml << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n"
         "<presence xmlns=\"urn:ietf:params:xml:ns:pidf\" "
                  " xmlns:dm=\"urn:ietf:params:xml:ns:pidf:data-model\""
                  " xmlns:rpid=\"urn:ietf:params:xml:ns:pidf:rpid\""
                  " xmlns:sc=\"urn:ietf:params:xml:ns:pidf:caps\""
                  " xmlns:ce=\"urn:cisco:params:xml:ns:pidf:rpid\""
                  " entity=\"" << m_entity << "\">\r\n";
  if (!m_personId.IsEmpty()) {
    xml << "  <dm:person id=\"p" << m_personId << "\">\r\n";
    if (!m_activities.IsEmpty()) {
      xml << "    <rpid:activities>\r\n";
      for (PStringSet::const_iterator it = m_activities.begin(); it != m_activities.end(); ++it) {
        if (RFC4480Activities.Contains(*it))
          xml << "      <rpid:" << it->ToLower() << "/>\r\n";
        else if (CiscoActivities.Contains(*it))
          xml << "      <ce:" << it->ToLower() << " />\r\n";
        else
          xml << "      <rpid:other>" << *it << "</rpid:other>\r\n";
      }
      xml << "    </rpid:activities>\r\n";
    }
    xml << "  </dm:person>\r\n";
  }

  xml << "  <tuple id=\"" << m_service << "\">\r\n"
         "    <status>\r\n";
  if (m_state != Unchanged)
    xml << "      <basic>" << (m_state != NoPresence ? "open" : "closed") << "</basic>\r\n";
  xml << "    </status>\r\n";
  if (m_state == Available) {
    xml << "    <contact priority=\"1\">";
    if (m_contact.IsEmpty())
      xml << m_entity;
    else
      xml << m_contact;
    xml << "</contact>\r\n";
  }

  if (!m_capabilities.IsEmpty()) {
    xml << "    <sc:servcaps>\r\n";
    for (PStringSet::const_iterator it = m_capabilities.begin(); it != m_capabilities.end(); ++it) {
      PString name, value;
      if (!it->Split('=', name, value)) {
        name = *it;
        value = "true";
      }
      if (RFC5196Capabilities.Contains(name))
        xml << "      <sc:" << name.ToLower() << '>' << value << "</sc:" << name << ">\r\n";
    }
    xml << "    </sc:servcaps>\r\n";
  }

  if (!m_note.IsEmpty()) {
    //xml << "    <note xml:lang=\"en\">" << PXML::EscapeSpecialChars(m_note) << "</note>\r\n";
    xml << "    <note>" << PXML::EscapeSpecialChars(m_note) << "</note>\r\n";
  }

  xml << "    <timestamp>" << PTime().AsString(PTime::RFC3339) << "</timestamp>\r\n"
         "  </tuple>\r\n";

  xml << "</presence>\r\n";

  return xml;
}


static void SetNoteFromElement(PXMLElement * element, PString & notes)
{
  PXMLElement * noteElement = element->GetElement("note");
  if (noteElement == NULL)
    return;

  PString note = noteElement->GetData();
  if (notes.Find(note) == P_MAX_INDEX) {
    if (!notes.IsEmpty())
      notes += '\n';
    notes += note;
  }
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

  if (notifyInfo.m_handler.GetProductInfo().name.Find("Asterisk") != P_MAX_INDEX) {
    SIPURL from(notifyInfo.m_response.GetMIME().GetFrom());
    xml.GetRootElement()->SetAttribute("entity", from.AsString());
    PTRACE2(3, NULL, "Compensating for " << notifyInfo.m_handler.GetProductInfo().name
            << ", replacing entity with " << from);
  }

  if (ParseXML(xml, infoList))
    return true;

  notifyInfo.m_response.SetEntityBody("Invalid/unsupported entity");
  return false;
}


bool SIPPresenceInfo::ParseXML(const PXML & xml, list<SIPPresenceInfo> & infoList)
{
  infoList.clear();

  // Common info to all tuples
  PURL entity;
  PXMLElement * rootElement = xml.GetRootElement();
  if (!entity.Parse(rootElement->GetAttribute("entity"), "pres")) {
    PTRACE2(1, NULL, "SIPPres\tInvalid/unsupported entity \"" << rootElement->GetAttribute("entity") << '"');
    return false;
  }

  PTime defaultTimestamp; // Start with "now"
  PStringSet personActivities;
  PString    personNote;
  PXMLElement * element;

  // RFC4479/3.2 states there can be one and only one <person> component.
  if ((element = rootElement->GetElement("urn:ietf:params:xml:ns:pidf:data-model|person")) != NULL ||
      (element = rootElement->GetElement("urn:cisco:params:xml:ns:pidf:rpid|person")) != NULL) {
    SetNoteFromElement(element, personNote);
    PXMLElement * activities;
    if ((activities = element->GetElement("activities")) != NULL ||
        (activities = element->GetElement("urn:ietf:params:xml:ns:pidf:rpid|activities")) != NULL ||
        (activities = element->GetElement("urn:ietf:params:xml:ns:pidf:status:rpid|activities")) != NULL ||
        (activities = element->GetElement("urn:cisco:params:xml:ns:pidf:rpid|activities")) != NULL) {
      for (PINDEX i = 0; i < activities->GetSize(); ++i) {
        PXMLElement * activity = dynamic_cast<PXMLElement *>(activities->GetElement(i));
        if (activity == NULL)
          continue;

        PCaselessString name(activity->GetName());
        PINDEX pos = name.Find('|');
        if (pos != P_MAX_INDEX)
          name.Delete(0, pos+1);

        PCaselessString data = activity->GetData().Trim();
        if (data.IsEmpty())
          personActivities += name;
        else
          personActivities += name + '=' + data;

        SetNoteFromElement(activity, personNote);
      }
    }
  }

  // Now process all the <tuple> components.
  PXMLElement * tupleElement;
  for (PINDEX tupleIdx = 0; (tupleElement = rootElement->GetElement("urn:ietf:params:xml:ns:pidf|tuple", tupleIdx)) != NULL; ++tupleIdx) {
    SIPPresenceInfo info(Unavailable);
    info.m_infoType = SIPPresenceEventPackageContentType;
    info.m_infoData = xml.AsString();
    info.m_activities = personActivities;
    info.m_activities.MakeUnique();
    info.m_note = personNote;
    info.m_entity = entity;
    info.m_service = tupleElement->GetAttribute("id");

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

    if ((element = tupleElement->GetElement("timestamp")) != NULL && !info.m_when.Parse(element->GetData()))
      info.m_when = defaultTimestamp;

    if ((element = tupleElement->GetElement("urn:ietf:params:xml:ns:pidf:caps|servcaps")) != NULL ||
        (element = tupleElement->GetElement("urn:ietf:params:xml:ns:pidf:servcaps|servcaps")) != NULL) {
      for (PINDEX idx = 0; idx < element->GetSize(); ++idx) {
        PXMLElement * cap = dynamic_cast<PXMLElement *>(element->GetElement(idx));
        if (cap != NULL) {
          PCaselessString name = cap->GetName();
          name.Delete(0, name.Find('|')+1);
          PCaselessString data = cap->GetData().Trim();
          if (data == "true")
            info.m_capabilities += name;
          else if (!data.IsEmpty() && data != "false")
            info.m_capabilities += name + '=' + data;
        }
      }
    }

    infoList.push_back(info);
    defaultTimestamp = info.m_when - PTimeInterval(0, 1); // One second older
  }

  infoList.sort();

  return true;
}
#endif // OPAL_SIP_PRESENCE


/////////////////////////////////////////////////////////////////////////

SIPMessageHandler::SIPMessageHandler(SIPEndPoint & endpoint, const SIPMessage::Params & params)
  : SIPHandler(SIP_PDU::Method_MESSAGE, endpoint, params, params.m_id)
  , m_parameters(params)
  , m_messageSent(false)
{
  m_offlineExpireTime = 0; // No retries for offline, just give up

  SetState(Subscribed);
}


SIPTransaction * SIPMessageHandler::CreateTransaction(OpalTransport & transport)
{
  if (GetState() == Unsubscribing)
    return NULL;

  // If message ID is zero, then it was sent once, don't do it again.
  if (m_messageSent) {
    PTRACE(4, "Message was already sent, not sending again.");
    return NULL;
  }

  SetExpire(m_originalExpireTime);

  SIPMessage * msg = new SIPMessage(*this, transport, m_parameters);
  m_parameters.m_localAddress = msg->GetLocalAddress().AsQuotedString();
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
#if OPAL_HAS_IM
  m_parameters.m_messageId = params.m_messageId;
#endif

  if (!params.m_body.IsEmpty()) {
    m_parameters.m_body = params.m_body;
    m_parameters.m_contentType = params.m_contentType;
  }

  m_messageSent = false;
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

  return new SIPPing(*this, transport, GetAddressOfRecord());
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
unsigned SIPHandlers::GetCount(SIP_PDU::Methods meth, const PString & eventPackage) const
{
  unsigned count = 0;
  for (const_iterator it = begin(); it != end(); ++it)
    if (it->second->GetState () == SIPHandler::Subscribed &&
        it->second->GetMethod() == meth &&
        (eventPackage.IsEmpty() || it->second->GetEventPackage() == eventPackage))
      count++;
  return count;
}


PStringList SIPHandlers::GetAddresses(bool includeOffline, SIP_PDU::Methods meth, const PString & eventPackage) const
{
  PStringList addresses;
  for (const_iterator it = begin(); it != end(); ++it)
    if ((includeOffline ? it->second->GetState() != SIPHandler::Unsubscribed
                        : it->second->GetState() == SIPHandler::Subscribed) &&
        it->second->GetMethod() == meth &&
        (eventPackage.IsEmpty() || it->second->GetEventPackage() == eventPackage))
      addresses.AppendString(it->second->GetAddressOfRecord().AsString());
  return addresses;
}


static PString MakeUrlKey(const PString & aor, SIP_PDU::Methods method, const PString & eventPackage = PString::Empty())
{
  return PString((unsigned)method) + aor + eventPackage;
}


/**
  * called when a handler is added
  */

void SIPHandlers::Append(SIPHandler * newHandler)
{
  if (newHandler == NULL)
    return;

  PWaitAndSignal mutex(GetMutex());

  PSafePtr<SIPHandler> handler = Find(newHandler->GetCallID(), PSafeReference);
  if (handler == NULL) {
    SetAt(newHandler->GetCallID(), newHandler);
    handler = Find(newHandler->GetCallID(), PSafeReference);
  }

  if (handler != newHandler)
    delete newHandler;

  // add entry to url and package map
  PString key = MakeUrlKey(handler->GetAddressOfRecord(), handler->GetMethod(), handler->GetEventPackage());
  handler->m_byAorAndPackage = m_byAorAndPackage.insert(IndexMap::value_type(key, handler));
  PTRACE_IF(1, !handler->m_byAorAndPackage.second, "Duplicate handler for Method/AOR/Package=\"" << key << '"');

  // add entry to username/realm map
  PString realm = handler->GetRealm();
  if (realm.IsEmpty())
    return;

  PString username = handler->GetAuthID();
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
void SIPHandlers::Remove(SIPHandler * handler)
{
  if (handler == NULL)
    return;

  PWaitAndSignal mutex(GetMutex());

  if (RemoveAt(handler->GetCallID()))
    RemoveIndexes(handler);
}


void SIPHandlers::Update(SIPHandler * handler)
{
  if (handler == NULL)
    return;

  PWaitAndSignal mutex(GetMutex());

  RemoveIndexes(handler);
  Append(handler);
}


void SIPHandlers::RemoveIndexes(SIPHandler * handler)
{
  if (handler->m_byAorUserAndRealm.second)
    m_byAorUserAndRealm.erase(handler->m_byAorUserAndRealm.first);

  if (handler->m_byAuthIdAndRealm.second)
    m_byAuthIdAndRealm.erase(handler->m_byAuthIdAndRealm.first);

  if (handler->m_byAorAndPackage.second)
    m_byAorAndPackage.erase(handler->m_byAorAndPackage.first);
}


PSafePtr<SIPHandler> SIPHandlers::FindBy(IndexMap & by, const PString & key, PSafetyMode mode)
{
  PSafePtr<SIPHandler> ptr;
  {
    PWaitAndSignal mutex(GetMutex());

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

  PTRACE(3, "Handler " << *ptr << " unsubscribed, awaiting shutdown.");
  while (!ptr->ShutDown())
    PThread::Sleep(100);

  Remove(ptr);
  return NULL;
}


PSafePtr<SIPHandler> SIPHandlers::FindSIPHandlerByCallID(const PString & callID, PSafetyMode mode)
{
  return Find(callID, mode);
}


PSafePtr<SIPHandler> SIPHandlers::FindSIPHandlerByUrl(const PString & aor, SIP_PDU::Methods method, PSafetyMode mode)
{
  return FindBy(m_byAorAndPackage, MakeUrlKey(aor, method), mode);
}


PSafePtr<SIPHandler> SIPHandlers::FindSIPHandlerByUrl(const PString & aor, SIP_PDU::Methods method, const PString & eventPackage, PSafetyMode mode)
{
  return FindBy(m_byAorAndPackage, MakeUrlKey(aor, method, eventPackage), mode);
}


PSafePtr<SIPHandler> SIPHandlers::FindSIPHandlerByAuthRealm(const PString & authRealm, PSafetyMode mode)
{
  // look for a match to realm without users
  for (iterator it = begin(); it != end(); ++it) {
    PSafePtr<SIPHandler> handler = it->second;
    if (  handler->GetRealm() == authRealm &&
          handler.SetSafetyMode(PSafeReadOnly) &&
         !handler->GetAuthID().IsEmpty() &&
         !handler->GetPassword().IsEmpty() &&
          handler.SetSafetyMode(mode)) {
      PTRACE(3, "Located existing credentials for realm \"" << authRealm << "\" "
             << handler->GetMethod() << " of aor=" << handler->GetAddressOfRecord() << ", id=" << handler->GetCallID());
      return handler;
    }
  }

  PTRACE(4, "No existing credentials for realm \"" << authRealm << '"');
  return NULL;
}


PSafePtr<SIPHandler> SIPHandlers::FindSIPHandlerByAuthRealm(const PString & authRealm, const PString & userName, PSafetyMode mode)
{
  PSafePtr<SIPHandler> handler;

  // look for a match to exact user name and realm
  if ((handler = FindBy(m_byAuthIdAndRealm, userName + '\n' + authRealm, mode)) == NULL &&
      (handler = FindBy(m_byAorUserAndRealm, userName + '\n' + authRealm, mode)) == NULL) {
    PTRACE(5, "No existing credentials for ID \"" << userName << "\" at realm \"" << authRealm << '"');
    return NULL;
  }

  PTRACE(4, "Located existing credentials for ID \"" << userName << "\" at realm \"" << authRealm << "\" "
         << handler->GetMethod() << " of aor=" << handler->GetAddressOfRecord() << ", id=" << handler->GetCallID());
  return handler;
}


/**
 * Find the SIPHandler object with the specified registration host.
 * For example, in the above case, the name parameter
 * could be "sip.seconix.com" or "seconix.com".
 */
PSafePtr<SIPHandler> SIPHandlers::FindSIPHandlerByDomain(const PString & name, SIP_PDU::Methods meth, PSafetyMode mode)
{
  for (iterator it = begin(); it != end(); ++it) {
    PSafePtr<SIPHandler> handler = it->second;
    if ( handler->GetMethod() == meth &&
         handler->GetState() != SIPHandler::Unsubscribed &&
        (handler->GetAddressOfRecord().GetHostName() == name ||
         handler->GetAddressOfRecord().GetTransportAddress().IsEquivalent(name)) &&
         handler.SetSafetyMode(mode))
      return handler;
  }
  return NULL;
}


#endif // OPAL_SIP
