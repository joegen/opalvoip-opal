/*
 * gkclient.cxx
 *
 * Gatekeeper client protocol handler
 *
 * Open H323 Library
 *
 * Copyright (c) 1999-2000 Equivalence Pty. Ltd.
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
 * Portions of this code were written with the assisance of funding from
 * iFace In, http://www.iface.com
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#include <ptlib.h>

#include <opal_config.h>
#if OPAL_H323

#ifdef __GNUC__
#pragma implementation "gkclient.h"
#endif

#include <h323/gkclient.h>

#include <h323/h323ep.h>
#include <h323/h323con.h>
#include <h323/h323pdu.h>
#include <h323/h323rtp.h>
#include <h460/h460_std18.h>


#define new PNEW
#define PTraceModule() "GkClient"


static PTimeInterval AdjustTimeout(unsigned seconds)
{
  // Allow for an incredible amount of system/network latency
  static unsigned TimeoutDeadband = 5; // seconds

  return PTimeInterval(0, seconds > TimeoutDeadband
                              ? (seconds - TimeoutDeadband)
                              : TimeoutDeadband);
}


/////////////////////////////////////////////////////////////////////////////

H323Gatekeeper::H323Gatekeeper(H323EndPoint & ep, H323Transport * trans)
  : H225_RAS(ep, trans)
  , discoveryComplete(false)
  , m_aliases(ep.GetAliasNames())
  , m_registrationFailReason(UnregisteredLocally)
  , m_onHighPriorityInterfaceChange(PCREATE_InterfaceNotifier(OnHighPriorityInterfaceChange))
  , m_onLowPriorityInterfaceChange(PCREATE_InterfaceNotifier(OnLowPriorityInterfaceChange))
  , m_alternateTemporary(false)
  , m_authenticators(ep.CreateAuthenticators())
#if OPAL_H460
  , m_features(ep.InternalCreateFeatureSet(NULL))
#endif
  , pregrantMakeCall(RequireARQ)
  , pregrantAnswerCall(RequireARQ)
  , m_autoReregister(true)
  , m_forceRegister(false)
  , m_requiresDiscovery(false)
  , m_willRespondToIRR(false)
  , m_monitorThread(NULL)
  , m_monitorRunning(false)
{
  PTRACE_CONTEXT_ID_NEW();
  PInterfaceMonitor::GetInstance().AddNotifier(m_onHighPriorityInterfaceChange, 80);
  PInterfaceMonitor::GetInstance().AddNotifier(m_onLowPriorityInterfaceChange, 40);
}


H323Gatekeeper::~H323Gatekeeper()
{
  PInterfaceMonitor::GetInstance().RemoveNotifier(m_onHighPriorityInterfaceChange);
  PInterfaceMonitor::GetInstance().RemoveNotifier(m_onLowPriorityInterfaceChange);

  StopChannel();

#if OPAL_H460
  delete m_features;
#endif
}


void H323Gatekeeper::SetRegistrationFailReason(unsigned reason, unsigned commandMask)
{
  SetRegistrationFailReason((RegistrationFailReasons)(reason|commandMask));
}


void H323Gatekeeper::SetRegistrationFailReason(RegistrationFailReasons reason)
{
  if (m_registrationFailReason != reason) {
    m_registrationFailReason = reason;
    endpoint.OnGatekeeperStatus(reason);
  }
}


PString H323Gatekeeper::GetRegistrationFailReasonString(RegistrationFailReasons reason)
{
  static const char * const ReasonStrings[] = {
    "Successfull",
    "Unregistered locally",
    "Unregistered by gatekeeper",
    "Gatekeeper lost registration",
    "Invalid listener",
    "Duplicate alias",
    "Security denied",
    "Transport error",
    "Trying alternate gatekeeper"
  };

  if (reason < PARRAYSIZE(ReasonStrings))
    return ReasonStrings[reason];

  if ((reason&RegistrationRejectReasonMask) != 0) {
    PString tag = H225_RegistrationRejectReason(reason&(RegistrationRejectReasonMask-1)).GetTagName();
    if (!tag.IsEmpty())
      return tag;
  }

  return psprintf("<%04x>", reason);
}


PString H323Gatekeeper::GetName() const
{
  PStringStream s;
  s << *this;
  return s;
}


PString H323Gatekeeper::GetEndpointIdentifier() const
{
  for (PINDEX i = 0; i < m_endpointIdentifier.GetSize(); ++i) {
    if (m_endpointIdentifier[i] <= 0 || m_endpointIdentifier[i] > 255 || !isprint(m_endpointIdentifier[i]))
      return PSTRSTRM('[' << hex << setfill('0') << setw(4) << m_endpointIdentifier << ']');
  }

  return m_endpointIdentifier; // Convert to string
}


PBoolean H323Gatekeeper::DiscoverAny()
{
  gatekeeperIdentifier.MakeEmpty();
  return StartGatekeeper(H323TransportAddress());
}


PBoolean H323Gatekeeper::DiscoverByName(const PString & identifier)
{
  gatekeeperIdentifier = identifier;
  return StartGatekeeper(H323TransportAddress());
}


PBoolean H323Gatekeeper::DiscoverByAddress(const H323TransportAddress & address)
{
  gatekeeperIdentifier.MakeEmpty();
  return StartGatekeeper(address);
}


PBoolean H323Gatekeeper::DiscoverByNameAndAddress(const PString & identifier,
                                              const H323TransportAddress & address)
{
  gatekeeperIdentifier = identifier;
  return StartGatekeeper(address);
}


void H323RasPDU::WriteGRQ(H323Transport & transport, bool & succeeded)
{
  H323TransportAddress localAddress = transport.GetLocalAddress();

  // Check if interface is used by signalling channel listeners
  if (transport.GetEndPoint().GetInterfaceAddresses(&transport).IsEmpty()) {
    PTRACE(3, "Not sending GRQ on " << localAddress << " as no signalling channel is listening there.");
    return;
  }

  // Adjust out outgoing local address in PDU
  H225_GatekeeperRequest & grq = *this;
  localAddress.SetPDU(grq.m_rasAddress);
  succeeded = Write(transport);
}


bool H323Gatekeeper::StartGatekeeper(const H323TransportAddress & initialAddress)
{
  if (PAssertNULL(transport) == NULL)
    return false;

  PAssert(!transport->IsRunning(), "Cannot do initial discovery on running RAS channel");

  H323TransportAddress address = initialAddress;
  if (address.IsEmpty())
    address = "udp$*:1719";

  if (!transport->ConnectTo(address)) {
    SetRegistrationFailReason(TransportError);
    return false;
  }

  if (!StartChannel()) {
    SetRegistrationFailReason(TransportError);
    return false;
  }

  if (m_monitorThread == NULL) {
    m_monitorRunning = true;
    m_monitorThread = new PThreadObj<H323Gatekeeper>(*this, &H323Gatekeeper::Monitor, false, "GkMonitor");
    PTRACE_CONTEXT_ID_TO(m_monitorThread);
  }

  ReRegisterNow();
  return true;
}


bool H323Gatekeeper::DiscoverGatekeeper()
{
  discoveryComplete = false;

  for (;;) {
    H323RasPDU pdu;
    Request request(SetupGatekeeperRequest(pdu), pdu);
  
    H323TransportAddress address = transport->GetRemoteAddress();
    request.responseInfo = &address;
  
    requestsMutex.Wait();
    requests.SetAt(request.sequenceNumber, &request);
    requestsMutex.Signal();
  
    request.Poll(*this, endpoint.GetGatekeeperRequestRetries(), endpoint.GetGatekeeperRequestTimeout());
  
    requestsMutex.Wait();
    requests.SetAt(request.sequenceNumber, NULL);
    requestsMutex.Signal();

    if (request.responseResult != Request::TryAlternate)
      return discoveryComplete;

    for (AlternateList::iterator it = m_alternates.begin(); ; ++it) {
      if (it == m_alternates.end())
        return discoveryComplete;
      if (it->registrationState != AlternateInfo::RegistrationFailed) {
        transport->ConnectTo(it->rasAddress);
        break;
      }
    }
  }
}


void H323Gatekeeper::StopChannel()
{
  m_monitorRunning = false;
  m_monitorTickle.Signal();
  PThread::WaitAndDelete(m_monitorThread);

  H323Transactor::StopChannel();
}


PBoolean H323Gatekeeper::WriteTo(H323TransactionPDU & pdu, 
                                 const H323TransportAddressArray & addresses, 
                                 PBoolean callback)
{
  PSafeLockReadWrite mutex(*transport);

  if (discoveryComplete || pdu.GetPDU().GetTag() != H225_RasMessage::e_gatekeeperRequest)
    return H323Transactor::WriteTo(pdu, addresses, callback);

  PString oldInterface = transport->GetInterface();
  bool ok = transport->WriteConnect(PCREATE_NOTIFIER_EXT(dynamic_cast<H323RasPDU&>(pdu.GetPDU()),H323RasPDU,WriteGRQ));
  transport->SetInterface(oldInterface);

  PTRACE_IF(1, !ok, "Error writing discovery PDU: " << transport->GetErrorText());

  return ok;
}


unsigned H323Gatekeeper::SetupGatekeeperRequest(H323RasPDU & request)
{
  if (PAssertNULL(transport) == NULL)
    return 0;

  H225_GatekeeperRequest & grq = request.BuildGatekeeperRequest(GetNextSequenceNumber());

  H323TransportAddress rasAddress = transport->GetLocalAddress();
  rasAddress.SetPDU(grq.m_rasAddress);

  endpoint.SetEndpointTypeInfo(grq.m_endpointType);

  grq.IncludeOptionalField(H225_GatekeeperRequest::e_endpointAlias);
  m_aliasMutex.Wait();
  H323SetAliasAddresses(m_aliases, grq.m_endpointAlias);
  m_aliasMutex.Signal();

  if (!gatekeeperIdentifier) {
    grq.IncludeOptionalField(H225_GatekeeperRequest::e_gatekeeperIdentifier);
    grq.m_gatekeeperIdentifier = gatekeeperIdentifier;
  }

  grq.IncludeOptionalField(H225_GatekeeperRequest::e_supportsAltGK);

  H225_RAS::OnSendGatekeeperRequest(request, grq);

  discoveryComplete = false;

  return grq.m_requestSeqNum;
}


void H323Gatekeeper::OnSendGatekeeperRequest(H225_GatekeeperRequest & grq)
{
  H225_RAS::OnSendGatekeeperRequest(grq);

  for (H235Authenticators::iterator iterAuth = m_authenticators.begin(); iterAuth != m_authenticators.end(); ++iterAuth) {
    if (iterAuth->SetCapability(grq.m_authenticationCapability, grq.m_algorithmOIDs)) {
      grq.IncludeOptionalField(H225_GatekeeperRequest::e_authenticationCapability);
      grq.IncludeOptionalField(H225_GatekeeperRequest::e_algorithmOIDs);
    }
  }
}


PBoolean H323Gatekeeper::OnReceiveGatekeeperConfirm(const H225_GatekeeperConfirm & gcf)
{
  if (!H225_RAS::OnReceiveGatekeeperConfirm(gcf))
    return false;

  for (H235Authenticators::iterator iterAuth = m_authenticators.begin(); iterAuth != m_authenticators.end(); ++iterAuth) {
    if (iterAuth->UseGkAndEpIdentifiers())
      iterAuth->SetRemoteId(gatekeeperIdentifier);
  }

  if (gcf.HasOptionalField(H225_GatekeeperConfirm::e_authenticationMode) && gcf.HasOptionalField(H225_GatekeeperConfirm::e_algorithmOID)) {
    for (H235Authenticators::iterator iterAuth = m_authenticators.begin(); iterAuth != m_authenticators.end(); ++iterAuth) {
      if (!iterAuth->IsCapability(gcf.m_authenticationMode, gcf.m_algorithmOID))
        iterAuth->Enable(false);
      else {
        PBYTEArray rawPDU;
        if (lastRequest != NULL)
          rawPDU = lastRequest->requestPDU.GetRawPDU();
        H235Authenticator::ValidationResult result = iterAuth->ValidateTokens(gcf.m_tokens, gcf.m_cryptoTokens, rawPDU);
        iterAuth->Enable(result == H235Authenticator::e_OK || result == H235Authenticator::e_Absent);
      }
    }
  }

  if (gcf.HasOptionalField(H225_GatekeeperConfirm::e_alternateGatekeeper))
    SetAlternates(gcf.m_alternateGatekeeper, false);

  PSafeLockReadWrite mutex(*transport);

  OpalTransportAddress previousAddress(transport->GetRemoteAddress());
  H323TransportAddress locatedAddress(gcf.m_rasAddress, OpalTransportAddress::UdpPrefix());
  if (previousAddress == locatedAddress) {
    PTRACE(3, "Gatekeeper address verified: " << *transport);
    discoveryComplete = true;
    return true;
  }

  if (previousAddress.Find('*') != P_MAX_INDEX) {
    PTRACE(3, "Gatekeeper discovered at: " << *transport);
    discoveryComplete = true;
  }
  else if (endpoint.GetGatekeeperRasRedirect()) {
    PTRACE(3, "Remote gatekeeper redirected to " << locatedAddress << " from " << *transport);
    discoveryComplete = false; // Need to do GRQ again
    ReRegisterNow();
  }
  else {
    PTRACE(3, "Remote gatekeeper RAS address (" << locatedAddress << ") different: " << *transport);
    return true;
  }

  if (transport->SetRemoteAddress(locatedAddress))
    return true;

  PTRACE(2, "Invalid gatekeeper discovery address: \"" << locatedAddress << '"');
  return false;
}


PBoolean H323Gatekeeper::OnReceiveGatekeeperReject(const H225_GatekeeperReject & grj)
{
  if (!H225_RAS::OnReceiveGatekeeperReject(grj))
    return false;

  if (grj.HasOptionalField(H225_GatekeeperReject::e_altGKInfo))
    SetAlternates(grj.m_altGKInfo.m_alternateGatekeeper, grj.m_altGKInfo.m_altGKisPermanent);

  switch (lastRequest->rejectReason) {
    case H225_GatekeeperRejectReason::e_securityDenial :
    case H225_GatekeeperRejectReason::e_securityError :
      SetRegistrationFailReason(SecurityDenied);
      break;

    case H225_GatekeeperRejectReason::e_resourceUnavailable :
      lastRequest->responseResult = Request::TryAlternate;
      SetRegistrationFailReason(TryingAlternate);
      break;

    default :
      SetRegistrationFailReason(lastRequest->rejectReason, GatekeeperRejectReasonMask);
  }

  return true;
}


bool H323Gatekeeper::SetListenerAddresses(H225_ArrayOf_TransportAddress & pdu)
{
  H323TransportAddressArray listeners = endpoint.GetInterfaceAddresses(transport);
  if (listeners.IsEmpty())
    return false;

  for (PINDEX i = 0; i < listeners.GetSize(); i++) {
    if (listeners[i].GetProtoPrefix() != OpalTransportAddress::TcpPrefix())
      continue;

    H225_TransportAddress pduAddr;
    if (!listeners[i].SetPDU(pduAddr))
      continue;

    // Check for already have had that address.
    PINDEX pos, lastPos = pdu.GetSize();
    for (pos = 0; pos < lastPos; pos++) {
      if (pdu[pos] == pduAddr)
        break;
    }

    if (pos >= lastPos) {
      // Put new listener into array
      pdu.SetSize(lastPos+1);
      pdu[lastPos] = pduAddr;
      if (endpoint.GetOneSignalAddressInRRQ())
        return true;
    }
  }

  return pdu.GetSize() > 0;
}


static H225_AddressPattern & AddTerminalAliasPattern(H225_RegistrationRequest & rrq)
{
  rrq.IncludeOptionalField(H225_RegistrationRequest::e_terminalAliasPattern);
  PINDEX len = rrq.m_terminalAliasPattern.GetSize();
  rrq.m_terminalAliasPattern.SetSize(len + 1);
  return rrq.m_terminalAliasPattern[len];
}


bool H323Gatekeeper::RegistrationRequest(bool autoReg, bool didGkDiscovery, bool lightweight)
{
  if (PAssertNULL(transport) == NULL)
    return false;

  m_autoReregister = autoReg;

  H323RasPDU pdu;
  H225_RegistrationRequest & rrq = pdu.BuildRegistrationRequest(GetNextSequenceNumber());

  // the discoveryComplete flag of the RRQ is not to be confused with the
  // discoveryComplete member variable of this instance...
  rrq.m_discoveryComplete = didGkDiscovery;

  rrq.m_rasAddress.SetSize(1);
  H323TransportAddress rasAddress = transport->GetLocalAddress();
  
  // We do have to use the translated address if specified
  PIPSocket::Address localAddress, remoteAddress;
  WORD localPort;
  
  if (rasAddress.GetIpAndPort(localAddress, localPort) &&
      transport->GetRemoteAddress().GetIpAddress(remoteAddress) &&
      transport->GetEndPoint().GetManager().TranslateIPAddress(localAddress, remoteAddress))
    rasAddress = H323TransportAddress(localAddress, localPort);
  
  rasAddress.SetPDU(rrq.m_rasAddress[0]);

  if (!gatekeeperIdentifier) {
    rrq.IncludeOptionalField(H225_RegistrationRequest::e_gatekeeperIdentifier);
    rrq.m_gatekeeperIdentifier = gatekeeperIdentifier;
  }

  if (!m_endpointIdentifier.IsEmpty()) {
    rrq.IncludeOptionalField(H225_RegistrationRequest::e_endpointIdentifier);
    rrq.m_endpointIdentifier = m_endpointIdentifier;
  }

  PTimeInterval ttl = endpoint.GetGatekeeperTimeToLive();
  if (ttl > 0) {
    rrq.IncludeOptionalField(H225_RegistrationRequest::e_timeToLive);
    rrq.m_timeToLive = (int)ttl.GetSeconds();
  }

  if (!IsRegistered())
    lightweight = false;

  if (lightweight) { // send lightweight RRQ
    rrq.IncludeOptionalField(H225_RegistrationRequest::e_keepAlive);
    rrq.m_keepAlive = true;
  }
  else {
    if (!SetListenerAddresses(rrq.m_callSignalAddress)) {
      PTRACE(1, "Cannot register with Gatekeeper without a H323Listener!");
      return false;
    }

    endpoint.SetEndpointTypeInfo(rrq.m_terminalType);
    rrq.m_terminalType.RemoveOptionalField(H225_EndpointType::e_vendor); // Don't want it twice
    endpoint.SetVendorIdentifierInfo(rrq.m_endpointVendor);

    rrq.IncludeOptionalField(H225_RegistrationRequest::e_terminalAlias);
    m_aliasMutex.Wait();
    H323SetAliasAddresses(m_aliases, rrq.m_terminalAlias);
    m_aliasMutex.Signal();

    if (!endpoint.GetGatekeeperSimulatePattern()) {
      const PStringList patterns = endpoint.GetAliasNamePatterns();
      for (PStringList::const_iterator it = patterns.begin(); it != patterns.end(); ++it) {
        if (OpalIsE164(*it)) {
          H225_AddressPattern & addressPattern = AddTerminalAliasPattern(rrq);
          addressPattern.SetTag(H225_AddressPattern::e_wildcard);
          H323SetAliasAddress(*it, addressPattern, H225_AliasAddress::e_dialedDigits);
        }
        else {
          PString start, end;
          if (H323EndPoint::ParseAliasPatternRange(*it, start, end) > 0) {
            H225_AddressPattern & addressPattern = AddTerminalAliasPattern(rrq);
            addressPattern.SetTag(H225_AddressPattern::e_range);
            H225_AddressPattern_range &addressPattern_range = addressPattern;

            addressPattern_range.m_startOfRange.SetTag(H225_PartyNumber::e_e164Number);
            H225_PublicPartyNumber & startOfRange = addressPattern_range.m_startOfRange;
            startOfRange.m_publicNumberDigits = start;
            startOfRange.m_publicTypeOfNumber.SetTag(H225_PublicTypeOfNumber::e_unknown);

            addressPattern_range.m_endOfRange.SetTag(H225_PartyNumber::e_e164Number);
            H225_PublicPartyNumber & endOfRange = addressPattern_range.m_endOfRange;
            endOfRange.m_publicNumberDigits = end;
            endOfRange.m_publicTypeOfNumber.SetTag(H225_PublicTypeOfNumber::e_unknown);
          }
        }
      }
    }

    rrq.m_willSupplyUUIEs = true;
    rrq.IncludeOptionalField(H225_RegistrationRequest::e_usageReportingCapability);
    rrq.m_usageReportingCapability.IncludeOptionalField(H225_RasUsageInfoTypes::e_startTime);
    rrq.m_usageReportingCapability.IncludeOptionalField(H225_RasUsageInfoTypes::e_endTime);
    rrq.m_usageReportingCapability.IncludeOptionalField(H225_RasUsageInfoTypes::e_terminationCause);
    rrq.IncludeOptionalField(H225_RegistrationRequest::e_supportsAltGK);

    if (endpoint.CanDisplayAmountString()) {
      rrq.IncludeOptionalField(H225_RegistrationRequest::e_callCreditCapability);
      rrq.m_callCreditCapability.IncludeOptionalField(H225_CallCreditCapability::e_canDisplayAmountString);
      rrq.m_callCreditCapability.m_canDisplayAmountString = true;
    }

    if (endpoint.CanEnforceDurationLimit()) {
      rrq.IncludeOptionalField(H225_RegistrationRequest::e_callCreditCapability);
      rrq.m_callCreditCapability.IncludeOptionalField(H225_CallCreditCapability::e_canEnforceDurationLimit);
      rrq.m_callCreditCapability.m_canEnforceDurationLimit = true;
    }
  }

  Request request(rrq.m_requestSeqNum, pdu);
  if (MakeRequest(request))
    return true;

  PTRACE(3, "Failed registration of " << GetEndpointIdentifier() << " with " << *this);
  switch (request.responseResult) {
    case Request::RejectReceived :
      switch (request.rejectReason) {
        case H225_RegistrationRejectReason::e_discoveryRequired :
          // If have been told by GK that we need to discover it again, set flag
          // for next register done by timeToLive handler to do discovery
          m_requiresDiscovery = true;
          // Do next case

        case H225_RegistrationRejectReason::e_fullRegistrationRequired :
          SetRegistrationFailReason(GatekeeperLostRegistration);
          // Set timer to retry registration
          ReRegisterNow();
          break;

        // Onse below here are permananent errors, so don't try again
        case H225_RegistrationRejectReason::e_invalidCallSignalAddress :
          SetRegistrationFailReason(InvalidListener);
          break;

        case H225_RegistrationRejectReason::e_duplicateAlias :
          SetRegistrationFailReason(DuplicateAlias);
          break;

        case H225_RegistrationRejectReason::e_securityDenial :
          SetRegistrationFailReason(SecurityDenied);
          break;

        default :
          SetRegistrationFailReason(request.rejectReason, RegistrationRejectReasonMask);
          break;
      }
      break;

    case Request::BadCryptoTokens :
      SetRegistrationFailReason(SecurityDenied);
      break;

    default :
      SetRegistrationFailReason(TransportError);
      break;
  }

  return false;
}


PBoolean H323Gatekeeper::OnReceiveRegistrationConfirm(const H225_RegistrationConfirm & rcf)
{
  if (!H225_RAS::OnReceiveRegistrationConfirm(rcf))
    return false;

  m_forceRegister = false;

  m_endpointIdentifier = rcf.m_endpointIdentifier;
  PTRACE(3, "Registered " << GetEndpointIdentifier() << " with " << *this);

  if (rcf.HasOptionalField(H225_RegistrationConfirm::e_alternateGatekeeper))
    SetAlternates(rcf.m_alternateGatekeeper, false);

  // If gk does not include timetoLive then we assume it accepted what we offered
  if (rcf.HasOptionalField(H225_RegistrationConfirm::e_timeToLive))
    m_currentTimeToLive = AdjustTimeout(rcf.m_timeToLive);
  else
    m_currentTimeToLive = endpoint.GetGatekeeperTimeToLive();

#if OPAL_H460_NAT
  if (m_features != NULL && m_features->HasFeature(H460_FeatureStd18::ID()) && m_currentTimeToLive > endpoint.GetManager().GetNatKeepAliveTime()) {
    PTRACE(4, "H.440.18 detected, reducing TTL from " << m_currentTimeToLive << " to " << endpoint.GetManager().GetNatKeepAliveTime());
    m_currentTimeToLive = endpoint.GetManager().GetNatKeepAliveTime();
  }
#endif

  // At present only support first call signal address to GK
  if (rcf.m_callSignalAddress.GetSize() > 0)
    gkRouteAddress = rcf.m_callSignalAddress[0];

  m_willRespondToIRR = rcf.m_willRespondToIRR;

  pregrantMakeCall = pregrantAnswerCall = RequireARQ;
  if (rcf.HasOptionalField(H225_RegistrationConfirm::e_preGrantedARQ)) {
    if (rcf.m_preGrantedARQ.m_makeCall)
      pregrantMakeCall = rcf.m_preGrantedARQ.m_useGKCallSignalAddressToMakeCall
                                                      ? PreGkRoutedARQ : PregrantARQ;
    if (rcf.m_preGrantedARQ.m_answerCall)
      pregrantAnswerCall = rcf.m_preGrantedARQ.m_useGKCallSignalAddressToAnswer
                                                      ? PreGkRoutedARQ : PregrantARQ;
    if (rcf.m_preGrantedARQ.HasOptionalField(H225_RegistrationConfirm_preGrantedARQ::e_irrFrequencyInCall))
      SetInfoRequestRate(AdjustTimeout(rcf.m_preGrantedARQ.m_irrFrequencyInCall));
    else
      ClearInfoRequestRate();
  }
  else
    ClearInfoRequestRate();

  // Remove the endpoint aliases that the gatekeeper did not like and add the
  // ones that it really wants us to be.
  if (rcf.HasOptionalField(H225_RegistrationConfirm::e_terminalAlias)) {
    PStringSet aliasesFromGk;
    for (PINDEX i = 0; i < rcf.m_terminalAlias.GetSize(); i++) {
      PString alias = H323GetAliasAddressString(rcf.m_terminalAlias[i]);
      if (!alias)
        aliasesFromGk += alias;
    }

    PStringList aliasesToAdd, aliasesToRemove;

    m_aliasMutex.Wait();

    for (PStringSet::iterator alias = aliasesFromGk.begin(); alias != aliasesFromGk.end(); ++alias) {
      if (m_aliases.GetValuesIndex(*alias) == P_MAX_INDEX) {
        aliasesToAdd.AppendString(*alias);
        m_aliases.AppendString(*alias);
      }
    }

    for (PStringList::iterator alias = m_aliases.begin(); alias != m_aliases.end(); ++alias) {
      if (!aliasesFromGk.Contains(*alias))
        aliasesToRemove.AppendString(*alias);
    }

    m_aliasMutex.Signal();

    for (PStringList::iterator alias = aliasesToAdd.begin(); alias != aliasesToAdd.end(); ++alias) {
      PTRACE(3, "Gatekeeper add of alias \"" << *alias << '"');
      endpoint.AddAliasName(*alias, PString::Empty(), false);
    }
    for (PStringList::iterator alias = aliasesToRemove.begin(); alias != aliasesToRemove.end(); ++alias) {
      PTRACE(3, "Gatekeeper removal of alias \"" << *alias << '"');
      endpoint.RemoveAliasName(*alias, false);
    }
  }

  if (rcf.HasOptionalField(H225_RegistrationConfirm::e_serviceControl))
    OnServiceControlSessions(rcf.m_serviceControl, NULL);

  // NAT Detection with GNUGK
  if (rcf.HasOptionalField(H225_RegistrationConfirm::e_nonStandardData)) {
    PString NATaddr = rcf.m_nonStandardData.m_data.AsString();
    if (NATaddr.NumCompare("NAT=") == EqualTo) {
      PIPSocket::Address ip(NATaddr.Mid(4));
      if (ip.IsValid())
        endpoint.OnGatekeeperNATDetect(ip, gkRouteAddress);
    }
  }
  
  SetRegistrationFailReason(RegistrationSuccessful);

  return true;
}


PBoolean H323Gatekeeper::OnReceiveRegistrationReject(const H225_RegistrationReject & rrj)
{
  if (!H225_RAS::OnReceiveRegistrationReject(rrj))
    return false;

  if (rrj.HasOptionalField(H225_RegistrationReject::e_altGKInfo))
    SetAlternates(rrj.m_altGKInfo.m_alternateGatekeeper, rrj.m_altGKInfo.m_altGKisPermanent);

  // Update registration fail reason from last request fail reason
  switch(lastRequest->rejectReason) {
    case H225_RegistrationRejectReason::e_duplicateAlias :
      SetRegistrationFailReason(DuplicateAlias);
      break;

    case H225_RegistrationRejectReason::e_securityDenial :
      SetRegistrationFailReason(SecurityDenied);
      break;

    case H225_RegistrationRejectReason::e_resourceUnavailable :
      lastRequest->responseResult = Request::TryAlternate;
      SetRegistrationFailReason(TryingAlternate);
      break;

    default:
      SetRegistrationFailReason(lastRequest->rejectReason, RegistrationRejectReasonMask);
      break;
  }
  
  return true;
}


void H323Gatekeeper::Monitor()
{
  PTRACE(3, "Gatekeeper monitor thread started " << *this);
  PSimpleTimer ttlTimer, irrTimer;
  while (m_monitorRunning) {
    PTimeInterval waitTime = PMaxTimeInterval;
    if (m_currentTimeToLive > 0 && waitTime > ttlTimer.GetRemaining())
      waitTime = ttlTimer.GetRemaining();
    if (m_infoRequestTime > 0 && waitTime > irrTimer.GetRemaining())
      waitTime = irrTimer.GetRemaining();

    m_monitorTickle.Wait(waitTime);
    if (!m_monitorRunning)
      break;

    if (m_forceRegister || (m_currentTimeToLive > 0 && ttlTimer.HasExpired()))
      ttlTimer = InternalRegister();

    if (m_infoRequestTime > 0 && irrTimer.HasExpired()) {
      InfoRequestResponse();
      irrTimer = m_infoRequestTime;
    }
  }
  PTRACE(3, "Gatekeeper monitor thread ended " << *this);
}

PTimeInterval H323Gatekeeper::InternalRegister()
{
  static PTimeInterval const OffLineRetryTime(0, 0, 1);

  PTRACE(3, (m_forceRegister ? "Forced" : "Time To Live") << " registration of \"" << GetEndpointIdentifier() << "\" with " << *this);
  
  bool didGkDiscovery = discoveryComplete;

  if (!discoveryComplete) {
    if (endpoint.GetSendGRQ()) {
      if (!DiscoverGatekeeper()) {
        PTRACE_IF(2, !m_forceRegister, "Discovery failed, retrying in " << OffLineRetryTime);
        return OffLineRetryTime;
      }
      m_requiresDiscovery = false;
      didGkDiscovery = true;
    }
    else {
      PTRACE_IF(3, !m_requiresDiscovery, "Skipping gatekeeper discovery for " << transport->GetRemoteAddress());
      discoveryComplete = true;
    }
  }

  if (m_requiresDiscovery) {
    PTRACE(3, "Repeating discovery on gatekeepers request.");

    H323RasPDU pdu;
    Request request(SetupGatekeeperRequest(pdu), pdu);
    if (!MakeRequest(request) || !discoveryComplete) {
      PTRACE(2, "Rediscovery failed, retrying in " << OffLineRetryTime);
      return OffLineRetryTime;
    }

    m_requiresDiscovery = false;
    didGkDiscovery = true;
  }

  if (RegistrationRequest(m_autoReregister, didGkDiscovery, !m_forceRegister))
    return m_currentTimeToLive;

  PTRACE_IF(2, !m_forceRegister, "Time To Live reregistration failed, retrying in " << OffLineRetryTime);
  return OffLineRetryTime;
}


void H323Gatekeeper::ClearAllCalls()
{
  while (!m_activeConnections.empty()) {
    PSafePtr<H323Connection> connection = *m_activeConnections.begin();
    if (connection != NULL)
      connection->Release(OpalConnection::EndedByGatekeeper, true);
    else
      m_activeConnections.erase(*m_activeConnections.begin());
  }
}


PBoolean H323Gatekeeper::UnregistrationRequest(int reason)
{
  if (PAssertNULL(transport) == NULL)
    return false;

  ClearAllCalls();

  H323RasPDU pdu;
  H225_UnregistrationRequest & urq = pdu.BuildUnregistrationRequest(GetNextSequenceNumber());

  H323TransportAddress rasAddress = transport->GetLocalAddress();

  SetListenerAddresses(urq.m_callSignalAddress);

  urq.IncludeOptionalField(H225_UnregistrationRequest::e_endpointAlias);
  m_aliasMutex.Wait();
  H323SetAliasAddresses(m_aliases, urq.m_endpointAlias);
  m_aliasMutex.Signal();

  if (!gatekeeperIdentifier) {
    urq.IncludeOptionalField(H225_UnregistrationRequest::e_gatekeeperIdentifier);
    urq.m_gatekeeperIdentifier = gatekeeperIdentifier;
  }

  if (!m_endpointIdentifier.IsEmpty()) {
    urq.IncludeOptionalField(H225_UnregistrationRequest::e_endpointIdentifier);
    urq.m_endpointIdentifier = m_endpointIdentifier;
  }

  if (reason >= 0) {
    urq.IncludeOptionalField(H225_UnregistrationRequest::e_reason);
    urq.m_reason = reason;
  }

  Request request(urq.m_requestSeqNum, pdu);

  bool requestResult = MakeRequest(request);

  AlternateList alternates = m_alternates;
  m_alternates = AlternateList(); // Do not use RemoveAll(), this just detaches the reference
  for (AlternateList::iterator it = alternates.begin(); it != alternates.end(); ++it) {
    if (it->registrationState == AlternateInfo::IsRegistered && it->gatekeeperIdentifier != GetIdentifier()) {
      Connect(it->rasAddress, it->gatekeeperIdentifier);
      UnregistrationRequest(reason);
    }
  }

  if (requestResult)
    return true;

  switch (request.responseResult) {
    case Request::NoResponseReceived :
      SetRegistrationFailReason(TransportError);
      m_currentTimeToLive = 0; // disables lightweight RRQ
      break;

    case Request::BadCryptoTokens :
      SetRegistrationFailReason(SecurityDenied);
      m_currentTimeToLive = 0; // disables lightweight RRQ
      break;

    default :
      SetRegistrationFailReason(request.rejectReason, UnregistrationRejectReasonMask);
      break;
  }

  return !IsRegistered();
}


PBoolean H323Gatekeeper::OnReceiveUnregistrationConfirm(const H225_UnregistrationConfirm & ucf)
{
  if (!H225_RAS::OnReceiveUnregistrationConfirm(ucf))
    return false;

  SetRegistrationFailReason(UnregisteredLocally);
  m_currentTimeToLive = 0; // disables lightweight RRQ

  return true;
}


PBoolean H323Gatekeeper::OnReceiveUnregistrationRequest(const H225_UnregistrationRequest & urq)
{
  if (!H225_RAS::OnReceiveUnregistrationRequest(urq))
    return false;

  PTRACE(3, "Unregistration received");
  if (!urq.HasOptionalField(H225_UnregistrationRequest::e_gatekeeperIdentifier) ||
       urq.m_gatekeeperIdentifier.GetValue() != gatekeeperIdentifier) {
    PTRACE(2, "Inconsistent gatekeeperIdentifier!");
    return false;
  }

  if (!urq.HasOptionalField(H225_UnregistrationRequest::e_endpointIdentifier) ||
      m_endpointIdentifier != (PWCharArray)urq.m_endpointIdentifier)
  {
    PTRACE(2, "Inconsistent endpointIdentifier!");
    return false;
  }

  PTRACE(3, "Unregistered, calls cleared");
  SetRegistrationFailReason(UnregisteredByGatekeeper);
  m_currentTimeToLive = 0; // disables lightweight RRQ

  if (urq.HasOptionalField(H225_UnregistrationRequest::e_alternateGatekeeper))
    SetAlternates(urq.m_alternateGatekeeper, false);

  H323RasPDU response(m_authenticators);
  response.BuildUnregistrationConfirm(urq.m_requestSeqNum);
  PBoolean ok = WritePDU(response);

  ClearAllCalls();

  if (m_autoReregister) {
    PTRACE(4, "Reregistering by setting timeToLive");
    discoveryComplete = false;
    ReRegisterNow();
  }

  return ok;
}


PBoolean H323Gatekeeper::OnReceiveUnregistrationReject(const H225_UnregistrationReject & urj)
{
  if (!H225_RAS::OnReceiveUnregistrationReject(urj))
    return false;

  if (lastRequest->rejectReason != H225_UnregRejectReason::e_callInProgress) {
    SetRegistrationFailReason(UnregisteredLocally);
    m_currentTimeToLive = 0; // disables lightweight RRQ
  }

  return true;
}


PBoolean H323Gatekeeper::LocationRequest(const PString & alias,
                                     H323TransportAddress & address)
{
  PStringList aliases;
  aliases.AppendString(alias);
  return LocationRequest(aliases, address);
}


PBoolean H323Gatekeeper::LocationRequest(const PStringList & aliases,
                                     H323TransportAddress & address)
{
  if (PAssertNULL(transport) == NULL)
    return false;

  H323RasPDU pdu;
  H225_LocationRequest & lrq = pdu.BuildLocationRequest(GetNextSequenceNumber());

  H323SetAliasAddresses(aliases, lrq.m_destinationInfo);

  if (!m_endpointIdentifier.IsEmpty()) {
    lrq.IncludeOptionalField(H225_LocationRequest::e_endpointIdentifier);
    lrq.m_endpointIdentifier = m_endpointIdentifier;
  }

  H323TransportAddress replyAddress = transport->GetLocalAddress();
  replyAddress.SetPDU(lrq.m_replyAddress);

  lrq.IncludeOptionalField(H225_LocationRequest::e_sourceInfo);
  m_aliasMutex.Wait();
  H323SetAliasAddresses(m_aliases, lrq.m_sourceInfo);
  m_aliasMutex.Signal();

  if (!gatekeeperIdentifier) {
    lrq.IncludeOptionalField(H225_LocationRequest::e_gatekeeperIdentifier);
    lrq.m_gatekeeperIdentifier = gatekeeperIdentifier;
  }

  Request request(lrq.m_requestSeqNum, pdu);
  request.responseInfo = &address;
  if (!MakeRequest(request))
    return false;

  // sanity check the address - some Gks return address 0.0.0.0 and port 0
  PIPSocket::Address ipAddr;
  WORD port;
  return address.GetIpAndPort(ipAddr, port) && (port != 0);
}


H323Gatekeeper::AdmissionResponse::AdmissionResponse()
{
  rejectReason = UINT_MAX;

  gatekeeperRouted = false;
  endpointCount = 1;
  transportAddress = NULL;
  accessTokenData = NULL;

  aliasAddresses = NULL;
  destExtraCallInfo = NULL;
}


struct AdmissionRequestResponseInfo {
  AdmissionRequestResponseInfo(
    H323Gatekeeper::AdmissionResponse & r,
    H323Connection & c
  ) : param(r), connection(c) { }

  H323Gatekeeper::AdmissionResponse & param;
  H323Connection & connection;
  OpalBandwidth allocatedBandwidth;
  unsigned uuiesRequested;
  PString      accessTokenOID1;
  PString      accessTokenOID2;
};


PBoolean H323Gatekeeper::AdmissionRequest(H323Connection & connection,
                                      AdmissionResponse & response,
                                      PBoolean ignorePreGrantedARQ)
{
  bool answeringCall = connection.HadAnsweredCall();

  if (!ignorePreGrantedARQ) {
    switch (answeringCall ? pregrantAnswerCall : pregrantMakeCall) {
      case RequireARQ :
        break;
      case PregrantARQ :
        return true;
      case PreGkRoutedARQ :
        if (gkRouteAddress.IsEmpty()) {
          response.rejectReason = UINT_MAX;
          return false;
        }
        if (response.transportAddress != NULL)
          *response.transportAddress = gkRouteAddress;
        response.gatekeeperRouted = true;
        return true;
    }
  }

  PTRACE(3, "AdmissionRequest " << (answeringCall ? "answering" : "originating")
         << " call with local alias names: " << setfill(',') << connection.GetLocalAliasNames());

  H323RasPDU pdu;
  H225_AdmissionRequest & arq = pdu.BuildAdmissionRequest(GetNextSequenceNumber());

  arq.m_callType.SetTag(H225_CallType::e_pointToPoint);
  arq.m_endpointIdentifier = m_endpointIdentifier;
  arq.m_answerCall = answeringCall;
  arq.m_canMapAlias = true; // Stack supports receiving a different number in the ACF 
                            // to the one sent in the ARQ
  arq.m_willSupplyUUIEs = true;

  if (!gatekeeperIdentifier) {
    arq.IncludeOptionalField(H225_AdmissionRequest::e_gatekeeperIdentifier);
    arq.m_gatekeeperIdentifier = gatekeeperIdentifier;
  }

  PString destInfo = connection.GetRemotePartyName();
  bool e164 = OpalIsE164(destInfo);
  PIPAddressAndPort destAddr(destInfo, endpoint.GetDefaultSignalPort());

  arq.m_srcInfo.SetSize(1);
  if (answeringCall) {
    if (e164 || !destAddr.IsValid())
      H323SetAliasAddress(destInfo, arq.m_srcInfo[0]);

    if (!connection.GetLocalPartyName()) {
      arq.IncludeOptionalField(H225_AdmissionRequest::e_destinationInfo);
      H323SetAliasAddresses(connection.GetLocalAliasNames(), arq.m_destinationInfo);
    }
  }
  else {
    H323SetAliasAddresses(connection.GetLocalAliasNames(), arq.m_srcInfo);

    if (e164 || !destAddr.IsValid()) {
      arq.IncludeOptionalField(H225_AdmissionRequest::e_destinationInfo);
      arq.m_destinationInfo.SetSize(1);
      H323SetAliasAddress(destInfo, arq.m_destinationInfo[0]);
    }
  }

  const H323Transport * signallingChannel = connection.GetSignallingChannel();
  if (answeringCall) {
    arq.IncludeOptionalField(H225_AdmissionRequest::e_srcCallSignalAddress);
    H323TransportAddress signalAddress = signallingChannel->GetRemoteAddress();
    signalAddress.SetPDU(arq.m_srcCallSignalAddress);
    signalAddress = signallingChannel->GetLocalAddress();
    arq.IncludeOptionalField(H225_AdmissionRequest::e_destCallSignalAddress);
    signalAddress.SetPDU(arq.m_destCallSignalAddress);
  }
  else {
    if (signallingChannel != NULL && signallingChannel->IsOpen()) {
      arq.IncludeOptionalField(H225_AdmissionRequest::e_srcCallSignalAddress);
      H323TransportAddress signalAddress = signallingChannel->GetLocalAddress();
      signalAddress.SetPDU(arq.m_srcCallSignalAddress);
    }

    if (!e164 && destAddr.IsValid()) {
      arq.IncludeOptionalField(H225_AdmissionRequest::e_destCallSignalAddress);
      H323TransportAddress(destAddr).SetPDU(arq.m_destCallSignalAddress);
    }
  }

  connection.GetBandwidthAvailable(OpalBandwidth::RxTx).SetH225(arq.m_bandWidth);
  arq.m_callReferenceValue = connection.GetCallReference();
  arq.m_conferenceID = connection.GetConferenceIdentifier();
  arq.m_callIdentifier.m_guid = connection.GetCallIdentifier();

  AdmissionRequestResponseInfo info(response, connection);
  info.accessTokenOID1 = connection.GetGkAccessTokenOID();
  PINDEX comma = info.accessTokenOID1.Find(',');
  if (comma == P_MAX_INDEX)
    info.accessTokenOID2 = info.accessTokenOID1;
  else {
    info.accessTokenOID2 = info.accessTokenOID1.Mid(comma+1);
    info.accessTokenOID1.Delete(comma, P_MAX_INDEX);
  }

  connection.OnSendARQ(arq);

  Request request(arq.m_requestSeqNum, pdu);
  request.responseInfo = &info;

  if (!m_authenticators.IsEmpty()) {
    H235Authenticators adjustedAuthenticators;
    if (connection.GetAdmissionRequestAuthentication(arq, adjustedAuthenticators)) {
      PTRACE(3, "Authenticators credentials replaced with \""
             << setfill(',') << adjustedAuthenticators << setfill(' ') << "\" during ARQ");

      for (H235Authenticators::iterator iterAuth = adjustedAuthenticators.begin(); iterAuth != adjustedAuthenticators.end(); ++iterAuth) {
        if (iterAuth->UseGkAndEpIdentifiers())
          iterAuth->SetRemoteId(gatekeeperIdentifier);
      }

      pdu.SetAuthenticators(adjustedAuthenticators);
    }

    pdu.Prepare(arq);
  }

  if (!MakeRequest(request)) {
    response.rejectReason = request.rejectReason;

    // See if we are registered.
    if (request.responseResult == Request::RejectReceived &&
        response.rejectReason != H225_AdmissionRejectReason::e_callerNotRegistered &&
        response.rejectReason != H225_AdmissionRejectReason::e_invalidEndpointIdentifier)
      return false;

    PTRACE(2, "Endpoint has become unregistered during ARQ from gatekeeper " << gatekeeperIdentifier);

    // Have been told we are not registered (or gk offline)
    switch (request.responseResult) {
      case Request::NoResponseReceived :
        SetRegistrationFailReason(TransportError);
        response.rejectReason = UINT_MAX;
        break;

      case Request::BadCryptoTokens :
        SetRegistrationFailReason(SecurityDenied);
        response.rejectReason = H225_AdmissionRejectReason::e_securityDenial;
        break;

      default :
        SetRegistrationFailReason(GatekeeperLostRegistration);
    }

    // If we are not registered and auto register is set ...
    if (!m_autoReregister)
      return false;

    // Then immediately reregister.
    if (!RegistrationRequest(m_autoReregister))
      return false;

    // Reset the gk info in ARQ
    arq.m_endpointIdentifier = m_endpointIdentifier;
    if (!gatekeeperIdentifier) {
      arq.IncludeOptionalField(H225_AdmissionRequest::e_gatekeeperIdentifier);
      arq.m_gatekeeperIdentifier = gatekeeperIdentifier;
    }
    else
      arq.RemoveOptionalField(H225_AdmissionRequest::e_gatekeeperIdentifier);

    // Is new request so need new sequence number as well.
    arq.m_requestSeqNum = GetNextSequenceNumber();
    request.sequenceNumber = arq.m_requestSeqNum;

    if (!MakeRequest(request)) {
      response.rejectReason = request.responseResult == Request::RejectReceived
                                                ? request.rejectReason : UINT_MAX;
     
      return false;
    }
  }

  connection.SetBandwidthAllocated(OpalBandwidth::RxTx, info.allocatedBandwidth);
  connection.SetUUIEsRequested(info.uuiesRequested);
  m_activeConnections.insert(&connection);

  return true;
}


void H323Gatekeeper::OnSendAdmissionRequest(H225_AdmissionRequest & /*arq*/)
{
  // Override default function as it sets crypto tokens and this is really
  // done by the AdmissionRequest() function.
}


static unsigned GetUUIEsRequested(const H225_UUIEsRequested & pdu)
{
  unsigned uuiesRequested = 0;

  if ((PBoolean)pdu.m_setup)
    uuiesRequested |= (1<<H225_H323_UU_PDU_h323_message_body::e_setup);
  if ((PBoolean)pdu.m_callProceeding)
    uuiesRequested |= (1<<H225_H323_UU_PDU_h323_message_body::e_callProceeding);
  if ((PBoolean)pdu.m_connect)
    uuiesRequested |= (1<<H225_H323_UU_PDU_h323_message_body::e_connect);
  if ((PBoolean)pdu.m_alerting)
    uuiesRequested |= (1<<H225_H323_UU_PDU_h323_message_body::e_alerting);
  if ((PBoolean)pdu.m_information)
    uuiesRequested |= (1<<H225_H323_UU_PDU_h323_message_body::e_information);
  if ((PBoolean)pdu.m_releaseComplete)
    uuiesRequested |= (1<<H225_H323_UU_PDU_h323_message_body::e_releaseComplete);
  if ((PBoolean)pdu.m_facility)
    uuiesRequested |= (1<<H225_H323_UU_PDU_h323_message_body::e_facility);
  if ((PBoolean)pdu.m_progress)
    uuiesRequested |= (1<<H225_H323_UU_PDU_h323_message_body::e_progress);
  if ((PBoolean)pdu.m_empty)
    uuiesRequested |= (1<<H225_H323_UU_PDU_h323_message_body::e_empty);

  if (pdu.HasOptionalField(H225_UUIEsRequested::e_status) && (PBoolean)pdu.m_status)
    uuiesRequested |= (1<<H225_H323_UU_PDU_h323_message_body::e_status);
  if (pdu.HasOptionalField(H225_UUIEsRequested::e_statusInquiry) && (PBoolean)pdu.m_statusInquiry)
    uuiesRequested |= (1<<H225_H323_UU_PDU_h323_message_body::e_statusInquiry);
  if (pdu.HasOptionalField(H225_UUIEsRequested::e_setupAcknowledge) && (PBoolean)pdu.m_setupAcknowledge)
    uuiesRequested |= (1<<H225_H323_UU_PDU_h323_message_body::e_setupAcknowledge);
  if (pdu.HasOptionalField(H225_UUIEsRequested::e_notify) && (PBoolean)pdu.m_notify)
    uuiesRequested |= (1<<H225_H323_UU_PDU_h323_message_body::e_notify);

  return uuiesRequested;
}


static void ExtractToken(const AdmissionRequestResponseInfo & info,
                         const H225_ArrayOf_ClearToken & tokens,
                         PBYTEArray & accessTokenData)
{
  if (!info.accessTokenOID1 && tokens.GetSize() > 0) {
    PTRACE(4, "Looking for OID " << info.accessTokenOID1 << " in ACF to copy.");
    for (PINDEX i = 0; i < tokens.GetSize(); i++) {
      if (tokens[i].m_tokenOID == info.accessTokenOID1) {
        PTRACE(4, "Looking for OID " << info.accessTokenOID2 << " in token to copy.");
        if (tokens[i].HasOptionalField(H235_ClearToken::e_nonStandard) &&
            tokens[i].m_nonStandard.m_nonStandardIdentifier == info.accessTokenOID2) {
          PTRACE(4, "Copying ACF nonStandard OctetString.");
          accessTokenData = tokens[i].m_nonStandard.m_data;
          break;
        }
      }
    }
  }
}


PBoolean H323Gatekeeper::OnReceiveAdmissionConfirm(const H225_AdmissionConfirm & acf)
{
  if (!H225_RAS::OnReceiveAdmissionConfirm(acf))
    return false;

  AdmissionRequestResponseInfo & info = *(AdmissionRequestResponseInfo *)lastRequest->responseInfo;
  info.allocatedBandwidth = acf.m_bandWidth;
  if (info.param.transportAddress != NULL)
    *info.param.transportAddress = acf.m_destCallSignalAddress;

  info.param.gatekeeperRouted = acf.m_callModel.GetTag() == H225_CallModel::e_gatekeeperRouted;

  // Remove the endpoint aliases that the gatekeeper did not like and add the
  // ones that it really wants us to be.
  if (info.param.aliasAddresses != NULL &&
      acf.HasOptionalField(H225_AdmissionConfirm::e_destinationInfo)) {
    PTRACE(3, "Gatekeeper specified " << acf.m_destinationInfo.GetSize() << " aliases in ACF");
    *info.param.aliasAddresses = acf.m_destinationInfo;
  }

  if (acf.HasOptionalField(H225_AdmissionConfirm::e_uuiesRequested))
    info.uuiesRequested = GetUUIEsRequested(acf.m_uuiesRequested);

  if (info.param.destExtraCallInfo != NULL &&
      acf.HasOptionalField(H225_AdmissionConfirm::e_destExtraCallInfo))
    *info.param.destExtraCallInfo = acf.m_destExtraCallInfo;

  if (info.param.accessTokenData != NULL && acf.HasOptionalField(H225_AdmissionConfirm::e_tokens))
    ExtractToken(info, acf.m_tokens, *info.param.accessTokenData);

  if (info.param.transportAddress != NULL) {
    PINDEX count = 1;
    for (PINDEX i = 0; i < acf.m_alternateEndpoints.GetSize() && count < info.param.endpointCount; i++) {
      if (acf.m_alternateEndpoints[i].HasOptionalField(H225_Endpoint::e_callSignalAddress) &&
          acf.m_alternateEndpoints[i].m_callSignalAddress.GetSize() > 0) {
        info.param.transportAddress[count] = acf.m_alternateEndpoints[i].m_callSignalAddress[0];
        if (info.param.accessTokenData != NULL)
          ExtractToken(info, acf.m_alternateEndpoints[i].m_tokens, info.param.accessTokenData[count]);
        count++;
      }
    }
    info.param.endpointCount = count;
  }

  if (acf.HasOptionalField(H225_AdmissionConfirm::e_irrFrequency))
    SetInfoRequestRate(AdjustTimeout(acf.m_irrFrequency));
  m_willRespondToIRR = acf.m_willRespondToIRR;

  if (acf.HasOptionalField(H225_AdmissionConfirm::e_serviceControl))
    OnServiceControlSessions(acf.m_serviceControl, &info.connection);

  return true;
}


PBoolean H323Gatekeeper::OnReceiveAdmissionReject(const H225_AdmissionReject & arj)
{
  if (!H225_RAS::OnReceiveAdmissionReject(arj))
    return false;

  if (arj.HasOptionalField(H225_AdmissionConfirm::e_serviceControl))
    OnServiceControlSessions(arj.m_serviceControl,
              &((AdmissionRequestResponseInfo *)lastRequest->responseInfo)->connection);

  if (lastRequest->rejectReason == H225_AdmissionRejectReason::e_callerNotRegistered)
    lastRequest->responseResult = Request::TryAlternate;

  return true;
}


static void SetRasUsageInformation(const H323Connection & connection, 
                                   H225_RasUsageInformation & usage)
{
  time_t time = connection.GetAlertingTime().GetTimeInSeconds();
  if (time != 0) {
    usage.IncludeOptionalField(H225_RasUsageInformation::e_alertingTime);
    usage.m_alertingTime = (unsigned)time;
  }

  time = connection.GetConnectionStartTime().GetTimeInSeconds();
  if (time != 0) {
    usage.IncludeOptionalField(H225_RasUsageInformation::e_connectTime);
    usage.m_connectTime = (unsigned)time;
  }

  time = connection.GetConnectionEndTime().GetTimeInSeconds();
  if (time != 0) {
    usage.IncludeOptionalField(H225_RasUsageInformation::e_endTime);
    usage.m_endTime = (unsigned)time;
  }
}


PBoolean H323Gatekeeper::DisengageRequest(H323Connection & connection, unsigned reason)
{
  H323RasPDU pdu;
  H225_DisengageRequest & drq = pdu.BuildDisengageRequest(GetNextSequenceNumber());

  drq.m_endpointIdentifier = m_endpointIdentifier;
  drq.m_conferenceID = connection.GetConferenceIdentifier();
  drq.m_callReferenceValue = connection.GetCallReference();
  drq.m_callIdentifier.m_guid = connection.GetCallIdentifier();
  drq.m_disengageReason.SetTag(reason);
  drq.m_answeredCall = connection.HadAnsweredCall();

  drq.IncludeOptionalField(H225_DisengageRequest::e_usageInformation);
  SetRasUsageInformation(connection, drq.m_usageInformation);

  drq.IncludeOptionalField(H225_DisengageRequest::e_terminationCause);
  drq.m_terminationCause.SetTag(H225_CallTerminationCause::e_releaseCompleteReason);
  Q931::CauseValues cause = H323TranslateFromCallEndReason(connection.GetCallEndReason(), drq.m_terminationCause);
  if (cause != Q931::ErrorInCauseIE) {
    drq.m_terminationCause.SetTag(H225_CallTerminationCause::e_releaseCompleteCauseIE);
    PASN_OctetString & rcReason = drq.m_terminationCause;
    rcReason.SetSize(2);
    rcReason[0] = 0x80;
    rcReason[1] = (BYTE)(0x80|cause);
  }

  if (!gatekeeperIdentifier) {
    drq.IncludeOptionalField(H225_DisengageRequest::e_gatekeeperIdentifier);
    drq.m_gatekeeperIdentifier = gatekeeperIdentifier;
  }

  Request request(drq.m_requestSeqNum, pdu);
  bool ok = MakeRequestWithReregister(request, H225_DisengageRejectReason::e_notRegistered);

  m_activeConnections.erase(&connection);

  return ok;
}


PBoolean H323Gatekeeper::OnReceiveDisengageReject(const H323RasPDU &pdu, const H225_DisengageReject & drj)
{
  if (!H225_RAS::OnReceiveDisengageReject(pdu, drj))
    return false;

  if (lastRequest->rejectReason == H225_DisengageRejectReason::e_notRegistered)
    lastRequest->responseResult = Request::TryAlternate;

  return true;
}


PBoolean H323Gatekeeper::OnReceiveDisengageRequest(const H225_DisengageRequest & drq)
{
  if (!H225_RAS::OnReceiveDisengageRequest(drq))
    return false;

  OpalGloballyUniqueID id = NULL;
  if (drq.HasOptionalField(H225_DisengageRequest::e_callIdentifier))
    id = drq.m_callIdentifier.m_guid;
  if (id == NULL)
    id = drq.m_conferenceID;

  H323RasPDU response(m_authenticators);
  PSafePtr<H323Connection> connection = endpoint.FindConnectionWithLock(id.AsString());
  if (connection == NULL)
    response.BuildDisengageReject(drq.m_requestSeqNum,
                                  H225_DisengageRejectReason::e_requestToDropOther);
  else {
    H225_DisengageConfirm & dcf = response.BuildDisengageConfirm(drq.m_requestSeqNum);

    dcf.IncludeOptionalField(H225_DisengageConfirm::e_usageInformation);
    SetRasUsageInformation(*connection, dcf.m_usageInformation);

    connection->Release(H323Connection::EndedByGatekeeper);
  }

  if (drq.HasOptionalField(H225_DisengageRequest::e_serviceControl))
    OnServiceControlSessions(drq.m_serviceControl, connection);

  return WritePDU(response);
}


PBoolean H323Gatekeeper::BandwidthRequest(H323Connection & connection, OpalBandwidth requestedBandwidth)
{
  H323RasPDU pdu;
  H225_BandwidthRequest & brq = pdu.BuildBandwidthRequest(GetNextSequenceNumber());

  brq.m_endpointIdentifier = m_endpointIdentifier;
  brq.m_conferenceID = connection.GetConferenceIdentifier();
  brq.m_callReferenceValue = connection.GetCallReference();
  brq.m_callIdentifier.m_guid = connection.GetCallIdentifier();
  requestedBandwidth.SetH225(brq.m_bandWidth);
  brq.IncludeOptionalField(H225_BandwidthRequest::e_usageInformation);
  SetRasUsageInformation(connection, brq.m_usageInformation);

  Request request(brq.m_requestSeqNum, pdu);
  
  OpalBandwidth allocatedBandwidth;
  request.responseInfo = &allocatedBandwidth;

  if (!MakeRequestWithReregister(request, H225_BandRejectReason::e_notBound))
    return false;

  connection.SetBandwidthAllocated(OpalBandwidth::RxTx, allocatedBandwidth);
  return true;
}


PBoolean H323Gatekeeper::OnReceiveBandwidthConfirm(const H225_BandwidthConfirm & bcf)
{
  if (!H225_RAS::OnReceiveBandwidthConfirm(bcf))
    return false;

  if (lastRequest->responseInfo != NULL)
    *(OpalBandwidth *)lastRequest->responseInfo = bcf.m_bandWidth;

  return true;
}


PBoolean H323Gatekeeper::OnReceiveBandwidthRequest(const H225_BandwidthRequest & brq)
{
  if (!H225_RAS::OnReceiveBandwidthRequest(brq))
    return false;

  OpalGloballyUniqueID id = brq.m_callIdentifier.m_guid;
  PSafePtr<H323Connection> connection = endpoint.FindConnectionWithLock(id.AsString());

  H323RasPDU response(m_authenticators);
  if (connection == NULL)
    response.BuildBandwidthReject(brq.m_requestSeqNum, H225_BandRejectReason::e_invalidConferenceID);
  else if (connection->SetBandwidthAllocated(OpalBandwidth::RxTx, OpalBandwidth(brq.m_bandWidth)))
    response.BuildBandwidthConfirm(brq.m_requestSeqNum, brq.m_bandWidth);
  else
    response.BuildBandwidthReject(brq.m_requestSeqNum, H225_BandRejectReason::e_invalidPermission);

  return WritePDU(response);
}


void H323Gatekeeper::SetInfoRequestRate(const PTimeInterval & rate)
{
  m_infoRequestTime = rate;
}


void H323Gatekeeper::ClearInfoRequestRate()
{
  // Only reset rate to zero (disabled) if no calls present
  if (endpoint.GetConnectionCount())
    m_infoRequestTime = 0;
}


H225_InfoRequestResponse & H323Gatekeeper::BuildInfoRequestResponse(H323RasPDU & response,
                                                                    unsigned seqNum)
{
  H225_InfoRequestResponse & irr = response.BuildInfoRequestResponse(seqNum);

  endpoint.SetEndpointTypeInfo(irr.m_endpointType);
  irr.m_endpointIdentifier = m_endpointIdentifier;

  H323TransportAddress address = transport->GetLocalAddress();
  
  // We do have to use the translated address if specified
  PIPSocket::Address localAddress, remoteAddress;
  WORD localPort;
  
  if (address.GetIpAndPort(localAddress, localPort) &&
      transport->GetRemoteAddress().GetIpAddress(remoteAddress)) {

    OpalManager & manager = transport->GetEndPoint().GetManager();
    if (manager.TranslateIPAddress(localAddress, remoteAddress))
      address = H323TransportAddress(localAddress, localPort);
  }
  address.SetPDU(irr.m_rasAddress);

  SetListenerAddresses(irr.m_callSignalAddress);

  irr.IncludeOptionalField(H225_InfoRequestResponse::e_endpointAlias);
  m_aliasMutex.Wait();
  H323SetAliasAddresses(m_aliases, irr.m_endpointAlias);
  m_aliasMutex.Signal();

  return irr;
}


PBoolean H323Gatekeeper::SendUnsolicitedIRR(H225_InfoRequestResponse & irr,
                                        H323RasPDU & response)
{
  irr.m_unsolicited = true;

  if (m_willRespondToIRR) {
    PTRACE(4, "Sending unsolicited IRR and awaiting acknowledgement");
    Request request(irr.m_requestSeqNum, response);
    return MakeRequest(request);
  }

  PTRACE(4, "Sending unsolicited IRR and without acknowledgement");
  response.SetAuthenticators(m_authenticators);
  return WritePDU(response);
}


static void AddInfoRequestResponseCall(H225_InfoRequestResponse & irr,
                                       const H323Connection & connection)
{
  if (connection.IsReleased() || connection.GetPhase() == OpalConnection::UninitialisedPhase)
    return;

  irr.IncludeOptionalField(H225_InfoRequestResponse::e_perCallInfo);

  PINDEX sz = irr.m_perCallInfo.GetSize();
  if (!irr.m_perCallInfo.SetSize(sz+1))
    return;

  H225_InfoRequestResponse_perCallInfo_subtype & info = irr.m_perCallInfo[sz];

  info.m_callReferenceValue = connection.GetCallReference();
  info.m_callIdentifier.m_guid = connection.GetCallIdentifier();
  info.m_conferenceID = connection.GetConferenceIdentifier();
  info.IncludeOptionalField(H225_InfoRequestResponse_perCallInfo_subtype::e_originator);
  info.m_originator = !connection.HadAnsweredCall();

  H323_RTPChannel * channel = dynamic_cast<H323_RTPChannel *>(connection.FindChannel(H323Capability::DefaultAudioSessionID, false));
  if (channel == NULL)
    channel = dynamic_cast<H323_RTPChannel *>(connection.FindChannel(H323Capability::DefaultAudioSessionID, true));
  if (channel != NULL) {
    info.IncludeOptionalField(H225_InfoRequestResponse_perCallInfo_subtype::e_audio);
    info.m_audio.SetSize(1);
    channel->OnSendRasInfo(info.m_audio[0]);
  }

  channel = dynamic_cast<H323_RTPChannel *>(connection.FindChannel(H323Capability::DefaultVideoSessionID, false));
  if (channel == NULL)
    channel = dynamic_cast<H323_RTPChannel *>(connection.FindChannel(H323Capability::DefaultVideoSessionID, true));
  if (channel != NULL) {
    info.IncludeOptionalField(H225_InfoRequestResponse_perCallInfo_subtype::e_video);
    info.m_video.SetSize(1);
    channel->OnSendRasInfo(info.m_video[0]);
  }

  H323TransportAddress address = connection.GetControlChannel().GetLocalAddress();
  address.SetPDU(info.m_h245.m_sendAddress);
  address = connection.GetControlChannel().GetRemoteAddress();
  address.SetPDU(info.m_h245.m_recvAddress);

  info.m_callType.SetTag(H225_CallType::e_pointToPoint);
  connection.GetBandwidthUsed(OpalBandwidth::RxTx).SetH225(info.m_bandWidth);
  info.m_callModel.SetTag(connection.IsGatekeeperRouted() ? H225_CallModel::e_gatekeeperRouted
                                                          : H225_CallModel::e_direct);

  info.IncludeOptionalField(H225_InfoRequestResponse_perCallInfo_subtype::e_usageInformation);
  SetRasUsageInformation(connection, info.m_usageInformation);
}


static PBoolean AddAllInfoRequestResponseCall(H225_InfoRequestResponse & irr,
                                          H323EndPoint & endpoint,
                                          const PStringList & tokens)
{
  PBoolean addedOne = false;

  for (PStringList::const_iterator token = tokens.begin(); token != tokens.end(); ++token) {
    PSafePtr<H323Connection> connection = endpoint.FindConnectionWithLock(*token);
    if (connection != NULL) {
      AddInfoRequestResponseCall(irr, *connection);
      addedOne = true;
    }
  }

  return addedOne;
}


void H323Gatekeeper::InfoRequestResponse()
{
  PStringList tokens = endpoint.GetAllConnections();
  if (tokens.IsEmpty())
    return;

  H323RasPDU response;
  H225_InfoRequestResponse & irr = BuildInfoRequestResponse(response, GetNextSequenceNumber());

  if (AddAllInfoRequestResponseCall(irr, endpoint, tokens))
    SendUnsolicitedIRR(irr, response);
}


void H323Gatekeeper::InfoRequestResponse(const H323Connection & connection)
{
  H323RasPDU response;
  H225_InfoRequestResponse & irr = BuildInfoRequestResponse(response, GetNextSequenceNumber());

  AddInfoRequestResponseCall(irr, connection);

  SendUnsolicitedIRR(irr, response);
}


void H323Gatekeeper::InfoRequestResponse(const H323Connection & connection,
                                         const H225_H323_UU_PDU & pdu,
                                         PBoolean sent)
{
  // Are unknown Q.931 PDU
  if (pdu.m_h323_message_body.GetTag() > H225_H323_UU_PDU_h323_message_body::e_notify)
    return;

  // Check mask of things to report on
  if ((connection.GetUUIEsRequested() & (1<<pdu.m_h323_message_body.GetTag())) == 0)
    return;

  PTRACE(3, "Sending unsolicited IRR for requested UUIE");

  // Report the PDU
  H323RasPDU response;
  H225_InfoRequestResponse & irr = BuildInfoRequestResponse(response, GetNextSequenceNumber());

  AddInfoRequestResponseCall(irr, connection);

  irr.m_perCallInfo[0].IncludeOptionalField(H225_InfoRequestResponse_perCallInfo_subtype::e_pdu);
  irr.m_perCallInfo[0].m_pdu.SetSize(1);
  irr.m_perCallInfo[0].m_pdu[0].m_sent = sent;
  irr.m_perCallInfo[0].m_pdu[0].m_h323pdu = pdu;

  SendUnsolicitedIRR(irr, response);
}


PBoolean H323Gatekeeper::OnReceiveInfoRequest(const H225_InfoRequest & irq)
{
  if (!H225_RAS::OnReceiveInfoRequest(irq))
    return false;

  H323RasPDU response(m_authenticators);
  H225_InfoRequestResponse & irr = BuildInfoRequestResponse(response, irq.m_requestSeqNum);

  if (irq.m_callReferenceValue == 0) {
    if (!AddAllInfoRequestResponseCall(irr, endpoint, endpoint.GetAllConnections())) {
      irr.IncludeOptionalField(H225_InfoRequestResponse::e_irrStatus);
      irr.m_irrStatus.SetTag(H225_InfoRequestResponseStatus::e_invalidCall);
    }
  }
  else {
    OpalGloballyUniqueID id = irq.m_callIdentifier.m_guid;
    PSafePtr<H323Connection> connection = endpoint.FindConnectionWithLock(id.AsString());
    if (connection == NULL) {
      irr.IncludeOptionalField(H225_InfoRequestResponse::e_irrStatus);
      irr.m_irrStatus.SetTag(H225_InfoRequestResponseStatus::e_invalidCall);
    }
    else {
      if (irq.HasOptionalField(H225_InfoRequest::e_uuiesRequested))
        connection->SetUUIEsRequested(::GetUUIEsRequested(irq.m_uuiesRequested));

      AddInfoRequestResponseCall(irr, *connection);
    }
  }

  if (!irq.HasOptionalField(H225_InfoRequest::e_replyAddress))
    return WritePDU(response);

  H323TransportAddress replyAddress = irq.m_replyAddress;
  if (replyAddress.IsEmpty())
    return false;

  H323TransportAddress oldAddress = transport->GetRemoteAddress();
  if (oldAddress.IsEquivalent(replyAddress))
    return WritePDU(response);

  if (!transport->LockReadWrite())
    return false;

  bool ok = transport->ConnectTo(replyAddress) && WritePDU(response);
  transport->ConnectTo(oldAddress);

  transport->UnlockReadWrite();

  return ok;
}


PBoolean H323Gatekeeper::OnReceiveInfoRequestResponse(const H225_InfoRequestResponse & irr)
{
  if (!H225_RAS::OnReceiveInfoRequestResponse(irr))
    return false;

  if (lastRequest->rejectReason == H225_InfoRequestNakReason::e_notRegistered)
    lastRequest->responseResult = Request::TryAlternate;

  return true;
}


PBoolean H323Gatekeeper::OnReceiveServiceControlIndication(const H225_ServiceControlIndication & sci)
{
  if (!H225_RAS::OnReceiveServiceControlIndication(sci))
    return false;

  H323Connection * connection = NULL;

  if (sci.HasOptionalField(H225_ServiceControlIndication::e_callSpecific)) {
    OpalGloballyUniqueID id = sci.m_callSpecific.m_callIdentifier.m_guid;
    if (id.IsNULL())
      id = sci.m_callSpecific.m_conferenceID;
    connection = endpoint.FindConnectionWithLock(id.AsString());
  }

  OnServiceControlSessions(sci.m_serviceControl, connection);

  H323RasPDU response(m_authenticators);
  response.BuildServiceControlResponse(sci.m_requestSeqNum);
  return WritePDU(response);
}


void H323Gatekeeper::OnServiceControlSessions(const H225_ArrayOf_ServiceControlSession & serviceControl,
                                              H323Connection * connection)
{
  for (PINDEX i = 0; i < serviceControl.GetSize(); i++) {
    H225_ServiceControlSession & pdu = serviceControl[i];

    H323ServiceControlSession * session = NULL;
    unsigned sessionId = pdu.m_sessionId;

    if (serviceControlSessions.Contains(sessionId)) {
      session = &serviceControlSessions[sessionId];
      if (pdu.HasOptionalField(H225_ServiceControlSession::e_contents)) {
        if (!session->OnReceivedPDU(pdu.m_contents)) {
          PTRACE(2, "Service control for session has changed!");
          session = NULL;
        }
      }
    }

    if (session == NULL && pdu.HasOptionalField(H225_ServiceControlSession::e_contents)) {
      session = endpoint.CreateServiceControlSession(pdu.m_contents);
      serviceControlSessions.SetAt(sessionId, session);
    }

    if (session != NULL)
      endpoint.OnServiceControlSession(sessionId, pdu.m_reason.GetTag(), *session, connection);
  }
}


void H323Gatekeeper::OnTerminalAliasChanged()
{
  // Do a non-lightweight RRQ. Treat the GK as unregistered and immediately send a RRQ
  SetRegistrationFailReason(UnregisteredLocally);
  ReRegisterNow();
}


void H323Gatekeeper::SetPassword(const PString & password, 
                                 const PString & username)
{
  PString localId = username;
  if (localId.IsEmpty())
    localId = endpoint.GetGatekeeperUsername();

  for (H235Authenticators::iterator iterAuth = m_authenticators.begin(); iterAuth != m_authenticators.end(); ++iterAuth) {
    iterAuth->SetLocalId(localId);
    iterAuth->SetPassword(password);
  }

  ReRegisterNow();
}


PStringList H323Gatekeeper::GetAliases() const
{
  PStringList aliases;
  {
    PWaitAndSignal mutex(m_aliasMutex);
    for (PStringList::const_iterator alias = m_aliases.begin(); alias != m_aliases.end(); ++alias)
      aliases += alias->GetPointer(); // Do not use reference
  }
  return aliases;
}

void H323Gatekeeper::ReRegisterNow()
{
  if (m_monitorRunning) {
    PTRACE(4, "Triggering re-register for " << *this);
    m_forceRegister = true;
    m_monitorTickle.Signal();
  }
}


void H323Gatekeeper::SetAlternates(const H225_ArrayOf_AlternateGK & alts, bool permanent)
{
  if (m_alternateTemporary)  {
    // don't want to replace alternates gatekeepers if this is an alternate and it's not permanent
    for (AlternateList::iterator it = m_alternates.begin(); it != m_alternates.end(); ++it) {
      if (transport->GetRemoteAddress().IsEquivalent(it->rasAddress) && gatekeeperIdentifier == it->gatekeeperIdentifier)
        return;
    }
  }

  AlternateList alternates;
  for (PINDEX i = 0; i < alts.GetSize(); i++)
    alternates.Append(new AlternateInfo(alts[i]));

  AlternateList::iterator it = m_alternates.begin();
  while (it != m_alternates.end()) {
    AlternateList::iterator that = alternates.find(*it);
    if (that == alternates.end()) {
      PTRACE(3, "Removing alternate gatekeeper: " << *it);
      m_alternates.erase(it++);
    }
    else {
      alternates.erase(that);
      ++it;
    }
  }

  alternates.DisallowDeleteObjects();
  for (AlternateList::iterator it = alternates.begin(); it != alternates.end(); ++it) {
    PTRACE(3, "Adding alternate gatekeeper: " << *it);
    m_alternates.Append(&*it);
  }

  m_alternateTemporary = !permanent;
}


PBoolean H323Gatekeeper::MakeRequestWithReregister(Request & request, unsigned unregisteredTag)
{
  if (MakeRequest(request))
    return true;

  if (request.responseResult == Request::RejectReceived &&
      request.rejectReason != unregisteredTag)
    return false;

  PTRACE(2, "Endpoint has become unregistered from gatekeeper " << gatekeeperIdentifier);

  // Have been told we are not registered (or gk offline)
  switch (request.responseResult) {
    case Request::NoResponseReceived :
      SetRegistrationFailReason(TransportError);
      break;

    case Request::BadCryptoTokens :
      SetRegistrationFailReason(SecurityDenied);
      break;

    default :
      SetRegistrationFailReason(GatekeeperLostRegistration);
  }

  // If we are not registered and auto register is set ...
  if (!m_autoReregister)
    return false;

  ReRegisterNow();
  return false;
}


void H323Gatekeeper::Connect(const H323TransportAddress & address,
                             const PString & gkid)
{
  if (transport == NULL) {
    transport = CreateTransport(PIPSocket::GetDefaultIpAny());
    PTRACE_CONTEXT_ID_TO(transport);
  }

  transport->SetRemoteAddress(address);
  transport->Connect();

  gatekeeperIdentifier = gkid;
}


PBoolean H323Gatekeeper::MakeRequest(Request & request)
{
  if (PAssertNULL(transport) == NULL)
    return false;

  // Set authenticators if not already set by caller
  m_requestMutex.Wait();

  if (request.requestPDU.GetAuthenticators().IsEmpty())
    request.requestPDU.SetAuthenticators(m_authenticators);

  /* To be sure that the H323 Cleaner, H225 Caller or Monitor don't set the
     transport address of the alternate while the other is in timeout. We
     have to block the function */

  H323TransportAddress tempAddr = transport->GetRemoteAddress();
  PString tempIdentifier = gatekeeperIdentifier;
  
  AlternateList::iterator alt = m_alternates.begin();
  for (;;) {
    if (H225_RAS::MakeRequest(request)) {
      if (m_alternateTemporary && (transport->GetRemoteAddress() != tempAddr || gatekeeperIdentifier != tempIdentifier))
        Connect(tempAddr, tempIdentifier);
      m_requestMutex.Signal();
      return true;
    }
    
    if (request.responseResult != Request::NoResponseReceived &&
        request.responseResult != Request::TryAlternate) {
      // try alternate in those cases and see if it's successful
      m_requestMutex.Signal();
      return false;
    }
    
    AlternateInfo * altInfo;
    do {
      if (alt == m_alternates.end()) {
        if (m_alternateTemporary) 
          Connect(tempAddr, tempIdentifier);
        m_requestMutex.Signal();
        return false;
      }
      
      altInfo = &*alt++;
      transport->ConnectTo(altInfo->rasAddress);
      gatekeeperIdentifier = altInfo->gatekeeperIdentifier;
    } while (altInfo->registrationState == AlternateInfo::RegistrationFailed);
    
    if (altInfo->registrationState == AlternateInfo::NeedToRegister) {
      altInfo->registrationState = AlternateInfo::RegistrationFailed;
      SetRegistrationFailReason(TransportError);
      discoveryComplete = false;
      H323RasPDU pdu;
      Request req(SetupGatekeeperRequest(pdu), pdu);
      
      if (H225_RAS::MakeRequest(req)) {
        m_requestMutex.Signal(); // avoid deadlock...
        if (RegistrationRequest(m_autoReregister)) {
          altInfo->registrationState = AlternateInfo::IsRegistered;
          // The wanted registration is done, we can return
          if (request.requestPDU.GetChoice().GetTag() == H225_RasMessage::e_registrationRequest) {
            if (m_alternateTemporary)
              Connect(tempAddr,tempIdentifier);
            return true;
          }
        }
        m_requestMutex.Wait();
      }
    }
  }
}


H323Transport * H323Gatekeeper::CreateTransport(PIPSocket::Address binding, WORD port, PBoolean reuseAddr)
{
  return new H323TransportUDP(endpoint, binding, port, reuseAddr);
}


void H323Gatekeeper::OnHighPriorityInterfaceChange(PInterfaceMonitor & monitor, PInterfaceMonitor::InterfaceChange entry)
{
  if (entry.m_added) {
    // special case if interface filtering is used: A new interface may 'hide' the old interface.
    // If this is the case, remove the transport interface and do a full rediscovery. 
    //
    // There is a race condition: If the transport interface binding is cleared AFTER
    // PMonitoredSockets::ReadFromSocket() is interrupted and starts listening again,
    // the transport will still listen on the old interface only. Therefore, clear the
    // socket binding BEFORE the monitored sockets update their interfaces.
    if (PInterfaceMonitor::GetInstance().HasInterfaceFilter()) {
      PString iface = transport->GetInterface();
      if (iface.IsEmpty()) // not connected.
        return;
      
      PIPSocket::Address addr;
      if (!transport->GetRemoteAddress().GetIpAddress(addr))
        return;
      
      PStringArray ifaces = monitor.GetInterfaces(false, addr);
      
      if (ifaces.GetStringsIndex(iface) == P_MAX_INDEX) { // original interface no longer available
        transport->SetInterface(PString::Empty());
      }
    }
  }
  else {
    if (transport == NULL)
      return;
  
    PString iface = transport->GetInterface();
    if (iface.IsEmpty()) // not connected.
      return;
  
    if (PInterfaceMonitor::IsMatchingInterface(iface, entry)) {
      // currently used interface went down. make transport listen
      // on all available interfaces.
      transport->SetInterface(PString::Empty());
      PTRACE(3, "Interface gatekeeper is bound to has gone down, restarting discovery");
    }
  }
}


void H323Gatekeeper::OnLowPriorityInterfaceChange(PInterfaceMonitor & monitor, PInterfaceMonitor::InterfaceChange)
{
  // sanity check
  if (transport == NULL)
    return;
  
  PString iface = transport->GetInterface();
  if (!iface.IsEmpty()) // still connected
    return;
  
  // not connected anymore. so see if there is an interface available
  // for connecting to the remote address.
  PIPSocket::Address addr;
  if (!transport->GetRemoteAddress().GetIpAddress(addr))
    return;
  
  if (monitor.GetInterfaces(false, addr).GetSize() > 0) {
    // at least one interface available, locate gatekeper
    discoveryComplete = false;
    ReRegisterNow();
  }
}


#if OPAL_H460
PBoolean H323Gatekeeper::OnSendFeatureSet(H460_MessageType pduType, H225_FeatureSet & message) const
{
  return m_features != NULL && m_features->OnSendPDU(pduType, message);
}


void H323Gatekeeper::OnReceiveFeatureSet(H460_MessageType pduType, const H225_FeatureSet & message) const
{
  if (m_features != NULL)
    m_features->OnReceivePDU(pduType, message);
}
#endif


/////////////////////////////////////////////////////////////////////////////

H323Gatekeeper::AlternateInfo::AlternateInfo(H225_AlternateGK & alt)
  : rasAddress(alt.m_rasAddress),
    gatekeeperIdentifier(alt.m_gatekeeperIdentifier.GetValue()),
    priority(alt.m_priority)
{
  registrationState = alt.m_needToRegister ? NeedToRegister : NoRegistrationNeeded;
}


H323Gatekeeper::AlternateInfo::~AlternateInfo ()
{

}


PObject::Comparison H323Gatekeeper::AlternateInfo::Compare(const PObject & obj) const
{
  const AlternateInfo * other = dynamic_cast<const AlternateInfo *>(&obj);
  if (!PAssert(other != NULL, PInvalidCast))
    return LessThan;

  if (priority < other->priority)
    return LessThan;
  if (priority > other->priority)
    return GreaterThan;

  return rasAddress.Compare(other->rasAddress);
}


void H323Gatekeeper::AlternateInfo::PrintOn(ostream & strm) const
{
  if (!gatekeeperIdentifier)
    strm << gatekeeperIdentifier << '@';

  strm << rasAddress;

  if (priority > 0)
    strm << ";priority=" << priority;
}


#endif // OPAL_H323

/////////////////////////////////////////////////////////////////////////////

