/*
 * h323trans.cxx
 *
 * H.323 Transactor handler
 *
 * Open H323 Library
 *
 * Copyright (c) 2003 Equivalence Pty. Ltd.
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
 * The Original Code is Open H323 Library.
 *
 * The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * Contributor(s): ______________________________________.
 *
 */

#include <ptlib.h>

#include <opal_config.h>
#if OPAL_H323

#ifdef __GNUC__
#pragma implementation "h323trans.h"
#endif

#include <h323/h323trans.h>

#include <h323/h323ep.h>
#include <h323/h323pdu.h>

#include <ptclib/random.h>


static PTimeInterval ResponseRetirementAge(0, 30); // Seconds


#define new PNEW


static struct AuthenticatorString {
  int code;
  const char * desc;
} authenticatorStrings[] = {
  { H235Authenticator::e_OK,          "Security parameters and Msg are ok, no security attacks" },
  { H235Authenticator::e_Absent,      "Security parameters are expected but absent" },
  { H235Authenticator::e_Error,       "Security parameters are present but incorrect" },
  { H235Authenticator::e_InvalidTime, "Security parameters indicate peer has bad real time clock" },
  { H235Authenticator::e_BadPassword, "Security parameters indicate bad password in token" },
  { H235Authenticator::e_ReplyAttack, "Security parameters indicate an attack was made" },
  { H235Authenticator::e_Disabled,    "Security is disabled by local system" },
  { -1, NULL }
};

/////////////////////////////////////////////////////////////////////////////////

H323TransactionPDU::H323TransactionPDU()
{
}


H323TransactionPDU::H323TransactionPDU(const H235Authenticators & auth)
  : authenticators(auth)
{
}


ostream & operator<<(ostream & strm, const H323TransactionPDU & pdu)
{
  pdu.GetPDU().PrintOn(strm);
  return strm;
}


PBoolean H323TransactionPDU::Read(H323Transport & transport)
{
  if (!transport.ReadPDU(rawPDU)) {
    PTRACE(1, GetProtocolName() << "\tRead error ("
           << transport.GetErrorNumber(PChannel::LastReadError)
           << "): " << transport.GetErrorText(PChannel::LastReadError));
    return false;
  }

  rawPDU.ResetDecoder();
  PBoolean ok = GetPDU().Decode(rawPDU);
  if (!ok) {
    PTRACE(1, GetProtocolName() << "\tRead error: PER decode failure:\n  "
           << setprecision(2) << rawPDU << "\n "  << setprecision(2) << *this);
    GetChoice().SetTag(UINT_MAX);
    return true;
  }

  H323TraceDumpPDU(GetProtocolName(), false, rawPDU, GetPDU(), GetChoice(), GetSequenceNumber());

  return true;
}


PBoolean H323TransactionPDU::Write(H323Transport & transport)
{
  PPER_Stream strm;
  GetPDU().Encode(strm);
  strm.CompleteEncoding();

  // Finalise the security if present
  for (H235Authenticators::iterator iterAuth = authenticators.begin(); iterAuth != authenticators.end(); ++iterAuth)
    iterAuth->Finalise(strm);

  H323TraceDumpPDU("Trans", true, strm, GetPDU(), GetChoice(), GetSequenceNumber());

  if (transport.WritePDU(strm))
    return true;

  PTRACE(1, GetProtocolName() << "\tWrite PDU failed ("
         << transport.GetErrorNumber(PChannel::LastWriteError)
         << "): " << transport.GetErrorText(PChannel::LastWriteError));
  return false;
}


/////////////////////////////////////////////////////////////////////////////////

H323Transactor::H323Transactor(H323EndPoint & ep,
                               H323Transport * trans,
                               WORD local_port,
                               WORD remote_port)
  : m_endpoint(ep),
    m_defaultLocalPort(local_port),
    m_defaultRemotePort(remote_port)
{
  if (trans != NULL)
    m_transport = trans;
  else
    m_transport = new H323TransportUDP(ep, PIPSocket::GetDefaultIpAny(), local_port);
  PTRACE_CONTEXT_ID_TO(m_transport);

  Construct();
}


H323Transactor::H323Transactor(H323EndPoint & ep,
                               const H323TransportAddress & iface,
                               WORD local_port,
                               WORD remote_port)
  : m_endpoint(ep),
    m_defaultLocalPort(local_port),
    m_defaultRemotePort(remote_port)
{
  if (iface.IsEmpty())
    m_transport = NULL;
  else {
    PIPSocket::Address addr;
    PAssert(iface.GetIpAndPort(addr, local_port), "Cannot parse address");
    m_transport = new H323TransportUDP(ep, addr, local_port);
    PTRACE_CONTEXT_ID_TO(m_transport);
  }

  Construct();
}


void H323Transactor::Construct()
{
  m_nextSequenceNumber = PRandom::Number()%65536;
  m_checkResponseCryptoTokens = true;
  m_lastRequest = NULL;

  m_requests.DisallowDeleteObjects();
}


H323Transactor::~H323Transactor()
{
  StopChannel();
}


void H323Transactor::PrintOn(ostream & strm) const
{
  if (m_transport == NULL)
    strm << "<<no-transport>>";
  else
    strm << m_transport->GetRemoteAddress().GetHostName(true);
}


PBoolean H323Transactor::SetTransport(const H323TransportAddress & iface)
{
  PWaitAndSignal mutex(m_pduWriteMutex);

  if (m_transport != NULL && m_transport->GetLocalAddress().IsEquivalent(iface)) {
    PTRACE(2, "Trans\tAlready have listener for " << iface);
    return true;
  }

  PIPSocket::Address addr;
  WORD port = m_defaultLocalPort;
  if (!iface.GetIpAndPort(addr, port)) {
    PTRACE(1, "Trans\tCannot create listener for " << iface);
    return false;
  }

  if (m_transport != NULL) {
    m_transport->CleanUpOnTermination();
    delete m_transport;
  }

  m_transport = new H323TransportUDP(m_endpoint, addr, port);
  PTRACE_CONTEXT_ID_TO(m_transport);
  m_transport->SetPromiscuous(H323Transport::AcceptFromAny);
  return StartChannel();
}


PBoolean H323Transactor::StartChannel()
{
  if (m_transport == NULL)
    return false;

  m_transport->AttachThread(PThread::Create(PCREATE_NOTIFIER(HandleTransactions), "Transactor"));
  return true;
}


void H323Transactor::StopChannel()
{
  if (m_transport != NULL) {
    m_transport->CleanUpOnTermination();
    delete m_transport;
    m_transport = NULL;
  }
}


void H323Transactor::HandleTransactions(PThread &, P_INT_PTR)
{
  if (PAssertNULL(m_transport) == NULL)
    return;

  PTRACE(3, "Trans\tStarting listener thread on " << *m_transport);

  m_transport->SetReadTimeout(PMaxTimeInterval);

  PINDEX consecutiveErrors = 0;

  PBoolean ok = true;
  while (ok) {
    PTRACE(5, "Trans\tReading PDU");
    H323TransactionPDU * response = CreateTransactionPDU();
    if (response->Read(*m_transport)) {
      if (m_transport->GetInterface().IsEmpty())
        m_transport->SetInterface(m_transport->GetLastReceivedInterface());
      consecutiveErrors = 0;
      m_lastRequest = NULL;
      if (HandleTransaction(response->GetPDU()))
        m_lastRequest->m_responseHandled.Signal();
      if (m_lastRequest != NULL)
        m_lastRequest->m_responseMutex.Signal();
    }
    else {
      switch (m_transport->GetErrorCode(PChannel::LastReadError)) {
        case PChannel::Interrupted :
          if (m_transport->IsOpen())
            break;
          // Do NotOpen case

        case PChannel::NotOpen :
          ok = false;
          break;

        default :
          switch (m_transport->GetErrorCode(PChannel::LastReadError)) {
            case PChannel::Unavailable :
              PTRACE(2, "Trans\tCannot access remote " << m_transport->GetRemoteAddress());
              break;

            default:
              PTRACE(1, "Trans\tRead error: " << m_transport->GetErrorText(PChannel::LastReadError));
              if (++consecutiveErrors > 10)
                ok = false;
          }
      }
    }

    delete response;
    AgeResponses();
  }

  PTRACE(3, "Trans\tEnded listener thread on " << *m_transport);
}


PBoolean H323Transactor::SetUpCallSignalAddresses(H225_ArrayOf_TransportAddress & addresses)
{
  if (PAssertNULL(m_transport) == NULL)
    return false;

  H323SetTransportAddresses(*m_transport, m_endpoint.GetInterfaceAddresses(m_transport), addresses);
  return addresses.GetSize() > 0;
}


unsigned H323Transactor::GetNextSequenceNumber()
{
  unsigned sn;
  do {
    sn = ++m_nextSequenceNumber;
  }  while (sn == 0);
  return sn;
}


void H323Transactor::AgeResponses()
{
  PTime now;

  PWaitAndSignal mutex(m_pduWriteMutex);

  for (PINDEX i = 0; i < m_responses.GetSize(); i++) {
    const Response & response = m_responses[i];
    if ((now - response.m_lastUsedTime) > response.m_retirementAge) {
      PTRACE(4, "Trans\tRemoving cached response: " << response);
      m_responses.RemoveAt(i--);
    }
  }
}


PBoolean H323Transactor::SendCachedResponse(const H323TransactionPDU & pdu)
{
  if (PAssertNULL(m_transport) == NULL)
    return false;

  Response key(m_transport->GetLastReceivedAddress(), pdu.GetSequenceNumber());

  PWaitAndSignal mutex(m_pduWriteMutex);

  PINDEX idx = m_responses.GetValuesIndex(key);
  if (idx != P_MAX_INDEX)
    return m_responses[idx].SendCachedResponse(*m_transport);

  m_responses.Append(new Response(key));
  return false;
}


PBoolean H323Transactor::WritePDU(H323TransactionPDU & pdu)
{
  if (PAssertNULL(m_transport) == NULL)
    return false;

  OnSendingPDU(pdu.GetPDU());

  PWaitAndSignal mutex(m_pduWriteMutex);

  Response key(m_transport->GetLastReceivedAddress(), pdu.GetSequenceNumber());
  PINDEX idx = m_responses.GetValuesIndex(key);
  if (idx != P_MAX_INDEX)
    m_responses[idx].SetPDU(pdu);

  return pdu.Write(*m_transport);
}


PBoolean H323Transactor::WriteTo(H323TransactionPDU & pdu,
                             const H323TransportAddressArray & addresses,
                             PBoolean callback)
{
  if (PAssertNULL(m_transport) == NULL)
    return false;

  if (addresses.IsEmpty()) {
    if (callback)
      return WritePDU(pdu);

    return pdu.Write(*m_transport);
  }

  m_pduWriteMutex.Wait();

  H323TransportAddress oldAddress = m_transport->GetRemoteAddress();

  PBoolean ok = false;
  for (PINDEX i = 0; i < addresses.GetSize(); i++) {
    if (m_transport->SetRemoteAddress(addresses[i])) {
      PTRACE(3, "Trans\tWrite address set to " << addresses[i]);
      if (callback)
        ok = WritePDU(pdu);
      else
        ok = pdu.Write(*m_transport);
    }
  }

  m_transport->SetRemoteAddress(oldAddress);

  m_pduWriteMutex.Signal();

  return ok;
}


PBoolean H323Transactor::MakeRequest(Request & request)
{
  PTRACE(3, "Trans\tMaking request: " << request.m_requestPDU.GetChoice().GetTagName());

  OnSendingPDU(request.m_requestPDU.GetPDU());

  m_requestsMutex.Wait();
  m_requests.SetAt(request.m_sequenceNumber, &request);
  m_requestsMutex.Signal();

  PBoolean ok = request.Poll(*this);

  m_requestsMutex.Wait();
  m_requests.SetAt(request.m_sequenceNumber, NULL);
  m_requestsMutex.Signal();

  return ok;
}


PBoolean H323Transactor::CheckForResponse(unsigned reqTag, unsigned seqNum, const PASN_Choice * reason)
{
  m_requestsMutex.Wait();
  m_lastRequest = m_requests.GetAt(seqNum);
  m_requestsMutex.Signal();

  if (m_lastRequest == NULL) {
    PTRACE_IF(2, reqTag != H225_RasMessage::e_nonStandardMessage, "Trans",
              "Timed out or received sequence number (" << seqNum << ") for PDU we never requested");
    return false;
  }

  m_lastRequest->m_responseMutex.Wait();
  m_lastRequest->CheckResponse(reqTag, reason);
  return true;
}


PBoolean H323Transactor::HandleRequestInProgress(const H323TransactionPDU & pdu,
                                             unsigned delay)
{
  unsigned seqNum = pdu.GetSequenceNumber();

  m_requestsMutex.Wait();
  m_lastRequest = m_requests.GetAt(seqNum);
  m_requestsMutex.Signal();

  if (m_lastRequest == NULL) {
    PTRACE(2, "Trans\tTimed out or received sequence number (" << seqNum << ") for PDU we never requested");
    return false;
  }

  m_lastRequest->m_responseMutex.Wait();

  PTRACE(3, "Trans\tReceived RIP on sequence number " << seqNum);
  m_lastRequest->OnReceiveRIP(delay);
  return true;
}


bool H323Transactor::CheckCryptoTokens1(const H323TransactionPDU & pdu)
{
  // If cypto token checking disabled, just return true.
  if (!GetCheckResponseCryptoTokens())
    return true;

  if (m_lastRequest != NULL && pdu.GetAuthenticators().IsEmpty()) {
    ((H323TransactionPDU &)pdu).SetAuthenticators(m_lastRequest->m_requestPDU.GetAuthenticators());
    PTRACE(4, "Trans\tUsing credentials from request: "
           << setfill(',') << pdu.GetAuthenticators() << setfill(' '));
  }

  // Need futher checking
  return false;
}


bool H323Transactor::CheckCryptoTokens2()
{
  if (m_lastRequest == NULL)
    return false; // or true?

  /* Note that a crypto tokens error is flagged to the requestor in the
     responseResult field but the other thread is NOT signalled. This is so
     it can wait for the full timeout for any other packets that might have
     the correct tokens, preventing a possible DOS attack.
   */
  m_lastRequest->m_responseResult = Request::BadCryptoTokens;
  m_lastRequest->m_responseHandled.Signal();
  m_lastRequest->m_responseMutex.Signal();
  m_lastRequest = NULL;
  return false;
}


/////////////////////////////////////////////////////////////////////////////

H323Transactor::Request::Request(unsigned seqNum,
                                 H323TransactionPDU & pdu,
                                 const H323TransportAddressArray & addresses)
  : m_sequenceNumber(seqNum)
  , m_requestPDU(pdu)
  , m_requestAddresses(addresses)
  , m_rejectReason(0)
  , m_responseInfo(NULL)
  , m_responseResult(NoResponseReceived)
{
}


PBoolean H323Transactor::Request::Poll(H323Transactor & rasChannel, unsigned numRetries, const PTimeInterval & p_timeout)
{
  H323EndPoint & endpoint = rasChannel.GetEndPoint();

  m_responseResult = AwaitingResponse;
  
  if (numRetries == 0)
    numRetries = endpoint.GetRasRequestRetries();
 
  PTimeInterval timeout = p_timeout > 0 ? p_timeout : endpoint.GetRasRequestTimeout();

  for (unsigned retry = 1; retry <= numRetries; retry++) {
    // To avoid race condition with RIP must set timeout before sending the packet
    m_whenResponseExpected = PTimer::Tick() + timeout;

    if (!rasChannel.WriteTo(m_requestPDU, m_requestAddresses, false))
      break;

    PTRACE(3, "Trans\tWaiting on response to seqnum=" << m_requestPDU.GetSequenceNumber()
           << " for " << setprecision(1) << timeout << " seconds");

    do {
      // Wait for a response
      m_responseHandled.Wait(m_whenResponseExpected - PTimer::Tick());

      PWaitAndSignal mutex(m_responseMutex); // Wait till lastRequest goes out of scope

      switch (m_responseResult) {
        case AwaitingResponse :  // Was a timeout
          m_responseResult = NoResponseReceived;
          break;

        case ConfirmReceived :
          return true;

        case BadCryptoTokens :
          PTRACE(1, "Trans\tResponse to seqnum=" << m_requestPDU.GetSequenceNumber()
                 << " had invalid crypto tokens.");
          return false;

        case RequestInProgress :
          m_responseResult = AwaitingResponse; // Keep waiting
          break;

        default :
          return false;
      }

      PTRACE_IF(3, m_responseResult == AwaitingResponse,
                "Trans\tWaiting again on response to seqnum=" << m_requestPDU.GetSequenceNumber() <<
                " for " << setprecision(1) << (m_whenResponseExpected - PTimer::Tick()) << " seconds");
    } while (m_responseResult == AwaitingResponse);

    PTRACE(1, "Trans\tTimeout on request seqnum=" << m_requestPDU.GetSequenceNumber()
           << ", try #" << retry << " of " << numRetries);
  }

  return false;
}


void H323Transactor::Request::CheckResponse(unsigned reqTag, const PASN_Choice * reason)
{
  if (m_requestPDU.GetChoice().GetTag() != reqTag) {
    PTRACE(2, "Trans\tReceived reply for incorrect PDU tag.");
    m_responseResult = RejectReceived;
    m_rejectReason = UINT_MAX;
    return;
  }

  if (reason == NULL) {
    m_responseResult = ConfirmReceived;
    return;
  }

  PTRACE(2, "Trans\t" << m_requestPDU.GetChoice().GetTagName()
         << " rejected: " << reason->GetTagName());
  m_responseResult = RejectReceived;
  m_rejectReason = reason->GetTag();
}


void H323Transactor::Request::OnReceiveRIP(unsigned milliseconds)
{
  m_responseResult = RequestInProgress;
  m_whenResponseExpected = PTimer::Tick() + PTimeInterval(milliseconds);
}


/////////////////////////////////////////////////////////////////////////////

H323Transactor::Response::Response(const H323TransportAddress & addr, unsigned seqNum)
  : PString(addr),
    m_retirementAge(ResponseRetirementAge)
{
  sprintf("#%u", seqNum);
  m_replyPDU = NULL;
}


H323Transactor::Response::~Response()
{
  if (m_replyPDU != NULL)
    m_replyPDU->DeletePDU();
}


void H323Transactor::Response::SetPDU(const H323TransactionPDU & pdu)
{
  PTRACE(4, "Trans\tAdding cached response: " << *this);

  if (m_replyPDU != NULL)
    m_replyPDU->DeletePDU();
  m_replyPDU = pdu.ClonePDU();
  m_lastUsedTime = PTime();

  unsigned delay = pdu.GetRequestInProgressDelay();
  if (delay > 0)
    m_retirementAge = ResponseRetirementAge + delay;
}


PBoolean H323Transactor::Response::SendCachedResponse(H323Transport & transport)
{
  PTRACE(3, "Trans\tSending cached response: " << *this);

  if (m_replyPDU != NULL) {
    H323TransportAddress oldAddress = transport.GetRemoteAddress();
    transport.ConnectTo(Left(FindLast('#')));
    m_replyPDU->Write(transport);
    transport.ConnectTo(oldAddress);
  }
  else {
    PTRACE(2, "Trans\tRetry made by remote before sending response: " << *this);
  }

  m_lastUsedTime = PTime();
  return true;
}


/////////////////////////////////////////////////////////////////////////////////

H323Transaction::H323Transaction(H323Transactor & trans,
                                 const H323TransactionPDU & requestToCopy,
                                 H323TransactionPDU * conf,
                                 H323TransactionPDU * rej)
  : m_transactor(trans),
    m_replyAddresses(trans.GetTransport().GetLastReceivedAddress()),
    m_request(requestToCopy.ClonePDU())
{
  m_confirm = conf;
  m_reject = rej;
  m_authenticatorResult = H235Authenticator::e_Disabled;
  m_fastResponseRequired = true;
  m_isBehindNAT = false;
  m_canSendRIP  = false;
}


H323Transaction::~H323Transaction()
{
  delete m_request;
  delete m_confirm;
  delete m_reject;
}


PBoolean H323Transaction::HandlePDU()
{
  int response = OnHandlePDU();
  switch (response) {
    case Ignore :
      return false;

    case Confirm :
      if (m_confirm != NULL) {
        PrepareConfirm();
        WritePDU(*m_confirm);
      }
      return false;

    case Reject :
      if (m_reject != NULL)
        WritePDU(*m_reject);
      return false;
  }

  H323TransactionPDU * rip = CreateRIP(m_request->GetSequenceNumber(), response);
  PBoolean ok = WritePDU(*rip);
  delete rip;

  if (!ok)
    return false;

  if (m_fastResponseRequired) {
    m_fastResponseRequired = false;
    PThread::Create(PCREATE_NOTIFIER(SlowHandler), 0,
                                     PThread::AutoDeleteThread,
                                     PThread::NormalPriority,
                                     "Transaction");
  }

  return true;
}


void H323Transaction::SlowHandler(PThread &, P_INT_PTR)
{
  PTRACE(4, "Trans\tStarted slow PDU handler thread.");

  while (HandlePDU())
    ;

  delete this;

  PTRACE(4, "Trans\tEnded slow PDU handler thread.");
}


PBoolean H323Transaction::WritePDU(H323TransactionPDU & pdu)
{
  pdu.SetAuthenticators(m_authenticators);
  return m_transactor.WriteTo(pdu, m_replyAddresses, true);
}


bool H323Transaction::CheckCryptoTokens()
{
  m_request->SetAuthenticators(m_authenticators);

  m_authenticatorResult = ValidatePDU();
  if (m_authenticatorResult == H235Authenticator::e_OK)
    return true;

  SetRejectReason(GetSecurityRejectTag());

  PINDEX i;
  for (i = 0; (authenticatorStrings[i].code >= 0) && (authenticatorStrings[i].code != m_authenticatorResult); ++i) 
    ;

  const char * desc = "Unknown error";
  if (authenticatorStrings[i].code >= 0)
    desc = authenticatorStrings[i].desc;

  PTRACE(2, "Trans\t" << GetName() << " rejected - " << desc);
  return false;
}


/////////////////////////////////////////////////////////////////////////////////

H323TransactionServer::H323TransactionServer(H323EndPoint & ep)
  : m_ownerEndPoint(ep)
{
}


H323TransactionServer::~H323TransactionServer()
{
}


PBoolean H323TransactionServer::AddListeners(const H323TransportAddressArray & ifaces)
{
  if (ifaces.IsEmpty())
    return AddListener("udp$*");

  PINDEX i;

  m_mutex.Wait();
  ListenerList::iterator iterListener = m_listeners.begin();
  while (iterListener != m_listeners.end()) {
    PBoolean remove = true;
    for (PINDEX j = 0; j < ifaces.GetSize(); j++) {
      if (iterListener->GetTransport().GetLocalAddress().IsEquivalent(ifaces[j], true)) {
        remove = false;
       break;
      }
    }
    if (remove) {
      PTRACE(3, "Trans\tRemoving existing listener " << *iterListener);
      m_listeners.erase(iterListener++);
    }
    else
      ++iterListener;
  }
  m_mutex.Signal();

  for (i = 0; i < ifaces.GetSize(); i++) {
    if (!ifaces[i].IsEmpty())
      AddListener(ifaces[i]);
  }

  return m_listeners.GetSize() > 0;
}


PBoolean H323TransactionServer::AddListener(const H323TransportAddress & interfaceName)
{
  PWaitAndSignal wait(m_mutex);

  PINDEX i;
  for (ListenerList::iterator iterListener = m_listeners.begin(); iterListener != m_listeners.end(); ++iterListener) {
    if (iterListener->GetTransport().GetLocalAddress().IsEquivalent(interfaceName, true)) {
      PTRACE(2, "H323\tAlready have listener for " << interfaceName);
      return true;
    }
  }

  PIPSocket::Address addr;
  WORD port = GetDefaultUdpPort();
  if (!interfaceName.GetIpAndPort(addr, port))
    return AddListener(interfaceName.CreateTransport(m_ownerEndPoint));

  if (!addr.IsAny())
    return AddListener(new H323TransportUDP(m_ownerEndPoint, addr, port, false, true));

  PIPSocket::InterfaceTable interfaces;
  if (!PIPSocket::GetInterfaceTable(interfaces)) {
    PTRACE(1, "Trans\tNo interfaces on system!");
    if (!PIPSocket::GetHostAddress(addr))
      return false;
    return AddListener(new H323TransportUDP(m_ownerEndPoint, addr, port, false, true));
  }

  PTRACE(4, "Trans\tAdding interfaces:\n" << setfill('\n') << interfaces << setfill(' '));

  PBoolean atLeastOne = false;

  for (i = 0; i < interfaces.GetSize(); i++) {
    addr = interfaces[i].GetAddress();
    if (addr.IsValid()) {
      if (AddListener(new H323TransportUDP(m_ownerEndPoint, addr, port, false, true)))
        atLeastOne = true;
    }
  }

  return atLeastOne;
}


PBoolean H323TransactionServer::AddListener(H323Transport * transport)
{
  if (transport == NULL) {
    PTRACE(2, "Trans\tTried to listen on NULL transport");
    return false;
  }

  PTRACE_CONTEXT_ID_TO(transport);

  if (!transport->IsOpen()) {
    PTRACE(3, "Trans\tTransport is not open");
    delete transport;
    return false;
  }

  return AddListener(CreateListener(transport));
}


PBoolean H323TransactionServer::AddListener(H323Transactor * listener)
{
  if (listener == NULL)
    return false;

  PTRACE(3, "Trans\tStarted listener \"" << *listener << '"');

  m_mutex.Wait();
  m_listeners.Append(listener);
  m_mutex.Signal();

  listener->StartChannel();

  return true;
}


PBoolean H323TransactionServer::RemoveListener(H323Transactor * listener)
{
  PBoolean ok = true;

  m_mutex.Wait();
  if (listener != NULL) {
    PTRACE(3, "Trans\tRemoving listener " << *listener);
    ok = m_listeners.Remove(listener);
  }
  else {
    PTRACE(3, "Trans\tRemoving all listeners");
    m_listeners.RemoveAll();
  }
  m_mutex.Signal();

  return ok;
}


#endif // OPAL_H323

/////////////////////////////////////////////////////////////////////////////////
