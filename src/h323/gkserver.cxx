/*
 * gkserver.cxx
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
 * This code was based on original code from OpenGate of Egoboo Ltd. thanks
 * to Ashley Unitt for his efforts.
 *
 * Contributor(s): ______________________________________.
 *
 * $Log: gkserver.cxx,v $
 * Revision 1.2005  2002/01/14 06:35:57  robertj
 * Updated to OpenH323 v1.7.9
 *
 * Revision 2.3  2001/11/13 04:29:47  robertj
 * Changed OpalTransportAddress CreateTransport and CreateListsner functions
 *   to have extra parameter to control local binding of sockets.
 *
 * Revision 2.2  2001/10/05 00:22:13  robertj
 * Updated to PWLib 1.2.0 and OpenH323 1.7.0
 *
 * Revision 2.1  2001/08/13 05:10:39  robertj
 * Updates from OpenH323 v1.6.0 release.
 *
 * Revision 2.0  2001/07/27 15:48:25  robertj
 * Conversion of OpenH323 to Open Phone Abstraction Library (OPAL)
 *
 * Revision 1.14  2001/12/15 10:10:48  robertj
 * GCC compatibility
 *
 * Revision 1.13  2001/12/15 08:09:08  robertj
 * Added alerting, connect and end of call times to be sent to RAS server.
 *
 * Revision 1.12  2001/12/14 06:41:17  robertj
 * Added call end reason codes in DisengageRequest for GK server use.
 *
 * Revision 1.11  2001/12/13 11:08:45  robertj
 * Significant changes to support slow request handling, automatically sending
 *   RIP and spawning thread to handle time consuming operation.
 *
 * Revision 1.10  2001/12/06 06:44:42  robertj
 * Removed "Win32 SSL xxx" build configurations in favour of system
 *   environment variables to select optional libraries.
 *
 * Revision 1.9  2001/11/21 13:35:23  craigs
 * Removed deadlock with empty registered endpoint list
 *
 * Revision 1.8  2001/11/19 06:56:44  robertj
 * Added prefix strings for gateways registered with this gk, thanks Mikael Stolt
 *
 * Revision 1.7  2001/09/18 10:06:54  robertj
 * Added trace for if RRJ on security because user has no password.
 *
 * Revision 1.6  2001/09/13 01:15:20  robertj
 * Added flag to H235Authenticator to determine if gkid and epid is to be
 *   automatically set as the crypto token remote id and local id.
 *
 * Revision 1.5  2001/08/10 11:03:52  robertj
 * Major changes to H.235 support in RAS to support server.
 *
 * Revision 1.4  2001/08/06 07:44:55  robertj
 * Fixed problems with building without SSL
 *
 * Revision 1.3  2001/08/06 03:18:38  robertj
 * Fission of h323.h to h323ep.h & h323con.h, h323.h now just includes files.
 * Improved access to H.235 secure RAS functionality.
 * Changes to H.323 secure RAS contexts to help use with gk server.
 *
 * Revision 1.2  2001/07/24 10:12:33  craigs
 * Fixed problem with admission requests not having src signalling addresses
 *
 * Revision 1.1  2001/07/24 02:31:07  robertj
 * Added gatekeeper RAS protocol server classes.
 *
 */

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "gkserver.h"
#endif

#include <h323/gkserver.h>

#include <h323/h323ep.h>
#include <h323/h323pdu.h>


#define new PNEW


/////////////////////////////////////////////////////////////////////////////

H323GatekeeperRequest::H323GatekeeperRequest(H323GatekeeperListener & ras, unsigned seqNum)
  : rasChannel(ras),
    confirm(ras),
    reject(ras)
{
  requestSequenceNumber = seqNum;
  thread = NULL;
}


BOOL H323GatekeeperRequest::HandlePDU()
{
  int response = OnHandlePDU();
  switch (response) {
    case Confirm :
      rasChannel.WritePDU(confirm);
      delete this;
      return FALSE;

    case Reject :
      rasChannel.WritePDU(reject);
      delete this;
      return FALSE;
  }

  H323RasPDU pdu(rasChannel);
  pdu.BuildRequestInProgress(requestSequenceNumber, response);
  if (!rasChannel.WritePDU(pdu)) {
    delete this;
    return FALSE;
  }

  if (thread != NULL)
    return TRUE;

  thread = PThread::Create(PCREATE_NOTIFIER(SlowHandler), 0,
                           PThread::AutoDeleteThread,
                           PThread::NormalPriority,
                           "GkSrvr:%x");
  return TRUE;
}


void H323GatekeeperRequest::SlowHandler(PThread &, INT)
{
  PTRACE(3, "RAS\tStarted slow PDU handler thread.");

  while (HandlePDU())
    ;

  PTRACE(3, "RAS\tEnded slow PDU handler thread.");
}


/////////////////////////////////////////////////////////////////////////////

H323GatekeeperGRQ::H323GatekeeperGRQ(H323GatekeeperListener & rasChannel,
                                     const H225_GatekeeperRequest & grq_)
  : H323GatekeeperRequest(rasChannel, grq_.m_requestSeqNum),
    grq(grq_),
    gcf(confirm.BuildGatekeeperConfirm(grq.m_requestSeqNum)),
    grj(reject.BuildGatekeeperReject(grq.m_requestSeqNum,
                                     H225_GatekeeperRejectReason::e_terminalExcluded))
{
}


H323GatekeeperRequest::Response H323GatekeeperGRQ::OnHandlePDU()
{
  return rasChannel.OnDiscovery(*this);
}


H323GatekeeperRRQ::H323GatekeeperRRQ(H323GatekeeperListener & rasChannel,
                                     const H225_RegistrationRequest & rrq_)
  : H323GatekeeperRequest(rasChannel, rrq_.m_requestSeqNum),
    rrq(rrq_),
    rcf(confirm.BuildRegistrationConfirm(rrq.m_requestSeqNum)),
    rrj(reject.BuildRegistrationReject(rrq.m_requestSeqNum))
{
}


H323GatekeeperRequest::Response H323GatekeeperRRQ::OnHandlePDU()
{
  return rasChannel.OnRegistration(*this);
}


H323GatekeeperURQ::H323GatekeeperURQ(H323GatekeeperListener & rasChannel,
                                     const H225_UnregistrationRequest & urq_)
  : H323GatekeeperRequest(rasChannel, urq_.m_requestSeqNum),
    urq(urq_),
    ucf(confirm.BuildUnregistrationConfirm(urq.m_requestSeqNum)),
    urj(reject.BuildUnregistrationReject(urq.m_requestSeqNum))
{
}


H323GatekeeperRequest::Response H323GatekeeperURQ::OnHandlePDU()
{
  return rasChannel.OnUnregistration(*this);
}


H323GatekeeperARQ::H323GatekeeperARQ(H323GatekeeperListener & rasChannel,
                                     const H225_AdmissionRequest & arq_)
  : H323GatekeeperRequest(rasChannel, arq_.m_requestSeqNum),
    arq(arq_),
    acf(confirm.BuildAdmissionConfirm(arq.m_requestSeqNum)),
    arj(reject.BuildAdmissionReject(arq.m_requestSeqNum))
{
}


H323GatekeeperRequest::Response H323GatekeeperARQ::OnHandlePDU()
{
  return rasChannel.OnAdmission(*this);
}


H323GatekeeperDRQ::H323GatekeeperDRQ(H323GatekeeperListener & rasChannel,
                                     const H225_DisengageRequest & drq_)
  : H323GatekeeperRequest(rasChannel, drq_.m_requestSeqNum),
    drq(drq_),
    dcf(confirm.BuildDisengageConfirm(drq.m_requestSeqNum)),
    drj(reject.BuildDisengageReject(drq.m_requestSeqNum))
{
}


H323GatekeeperRequest::Response H323GatekeeperDRQ::OnHandlePDU()
{
  return rasChannel.OnDisengage(*this);
}


H323GatekeeperBRQ::H323GatekeeperBRQ(H323GatekeeperListener & rasChannel,
                                     const H225_BandwidthRequest & brq_)
  : H323GatekeeperRequest(rasChannel, brq_.m_requestSeqNum),
    brq(brq_),
    bcf(confirm.BuildBandwidthConfirm(brq.m_requestSeqNum)),
    brj(reject.BuildBandwidthReject(brq.m_requestSeqNum))
{
}


H323GatekeeperRequest::Response H323GatekeeperBRQ::OnHandlePDU()
{
  return rasChannel.OnBandwidth(*this);
}


H323GatekeeperLRQ::H323GatekeeperLRQ(H323GatekeeperListener & rasChannel,
                                     const H225_LocationRequest & lrq_)
  : H323GatekeeperRequest(rasChannel, lrq_.m_requestSeqNum),
    lrq(lrq_),
    lcf(confirm.BuildLocationConfirm(lrq.m_requestSeqNum)),
    lrj(reject.BuildLocationReject(lrq.m_requestSeqNum))
{
}


H323GatekeeperRequest::Response H323GatekeeperLRQ::OnHandlePDU()
{
  return rasChannel.OnLocation(*this);
}


/////////////////////////////////////////////////////////////////////////////

H323GatekeeperCall::H323GatekeeperCall(const OpalGloballyUniqueID & id,
                                       H323RegisteredEndPoint * ep)
  : callIdentifier(id),
    conferenceIdentifier(NULL),
    alertingTime(0),
    connectedTime(0),
    callEndTime(0)

{
  endpoint = ep;
  if (endpoint != NULL)
    endpoint->AddCall(this);
  callEndReason = OpalNumCallEndReasons;
}


H323GatekeeperCall::~H323GatekeeperCall()
{
  if (endpoint != NULL)
    endpoint->RemoveCall(this);
}


PObject::Comparison H323GatekeeperCall::Compare(const PObject & obj) const
{
  PAssert(obj.IsDescendant(H323GatekeeperCall::Class()), PInvalidCast);
  return callIdentifier.Compare(((const H323GatekeeperCall &)obj).callIdentifier);
}


void H323GatekeeperCall::PrintOn(ostream & strm) const
{
  strm << "Call:" << callIdentifier;
}


H323GatekeeperRequest::Response
     H323GatekeeperCall::OnAdmission(const H323GatekeeperServer & /*server*/,
                                     H323GatekeeperARQ & info)
{
  callReference = info.arq.m_callReferenceValue;
  conferenceIdentifier = info.arq.m_conferenceID;

  if (info.arq.HasOptionalField(H225_AdmissionRequest::e_srcCallSignalAddress))
    srcAddress = info.arq.m_srcCallSignalAddress;

  if (info.arq.m_answerCall) {
    if (info.arq.HasOptionalField(H225_AdmissionRequest::e_destCallSignalAddress))
      dstAddress = info.arq.m_destCallSignalAddress;
  } else
    dstAddress = info.acf.m_destCallSignalAddress;

  bandwidthUsed = info.acf.m_bandWidth;

  return H323GatekeeperRequest::Confirm;
}


H323GatekeeperRequest::Response
     H323GatekeeperCall::OnDisengage(const H323GatekeeperServer & /*server*/,
                                     H323GatekeeperDRQ & info)
{
  if (info.drq.HasOptionalField(H225_DisengageRequest::e_usageInformation)) {
    if (info.drq.m_usageInformation.HasOptionalField(H225_RasUsageInformation::e_alertingTime))
      alertingTime = PTime((unsigned)info.drq.m_usageInformation.m_alertingTime);
    if (info.drq.m_usageInformation.HasOptionalField(H225_RasUsageInformation::e_connectTime))
      connectedTime = PTime((unsigned)info.drq.m_usageInformation.m_connectTime);
    if (info.drq.m_usageInformation.HasOptionalField(H225_RasUsageInformation::e_endTime))
      callEndTime = PTime((unsigned)info.drq.m_usageInformation.m_endTime);
  }

  if (info.drq.HasOptionalField(H225_DisengageRequest::e_terminationCause)) {
    if (info.drq.m_terminationCause.GetTag() == H225_CallTerminationCause::e_releaseCompleteReason) {
      H225_ReleaseCompleteReason & reason = info.drq.m_terminationCause;
      callEndReason = H323TranslateToCallEndReason(Q931::ErrorInCauseIE, reason);
    }
    else {
      PASN_OctetString & cause = info.drq.m_terminationCause;
      H225_ReleaseCompleteReason dummy;
      callEndReason = H323TranslateToCallEndReason((Q931::CauseValues)(cause[1]&0x7f), dummy);
    }
  }

  return H323GatekeeperRequest::Confirm;
}


/////////////////////////////////////////////////////////////////////////////

H323RegisteredEndPoint::H323RegisteredEndPoint(const PString & id)
  : identifier(id)
{
  PTRACE(4, "RAS\tCreated registered endpoint: " << id);

#if P_SSL
  authenticators.Append(new H235AuthProcedure1);
#endif
  authenticators.Append(new H235AuthSimpleMD5);

  activeCalls.DisallowDeleteObjects();
}


PObject::Comparison H323RegisteredEndPoint::Compare(const PObject & obj) const
{
  PAssert(obj.IsDescendant(H323RegisteredEndPoint::Class()), PInvalidCast);
  return identifier.Compare(((const H323RegisteredEndPoint &)obj).identifier);
}


void H323RegisteredEndPoint::PrintOn(ostream & strm) const
{
  strm << identifier;
}


H323GatekeeperRequest::Response
     H323RegisteredEndPoint::OnRegistration(const H323GatekeeperServer & server,
                                            H323GatekeeperRRQ & info)
{
  PINDEX i;

  if (info.rrq.m_rasAddress.GetSize() > 0) {
    rasAddresses.SetSize(info.rrq.m_rasAddress.GetSize());
    for (i = 0; i < info.rrq.m_rasAddress.GetSize(); i++) {
      H323TransportAddress address(info.rrq.m_rasAddress[i]);
      if (!address)
        rasAddresses.SetAt(i, new OpalTransportAddress(address));
    }
  }

  if (rasAddresses.IsEmpty()) {
    info.rrj.m_rejectReason.SetTag(H225_RegistrationRejectReason::e_invalidRASAddress);
    return H323GatekeeperRequest::Reject;
  }

  if (info.rrq.m_callSignalAddress.GetSize() > 0) {
    signalAddresses.SetSize(info.rrq.m_callSignalAddress.GetSize());
    for (i = 0; i < info.rrq.m_callSignalAddress.GetSize(); i++)
      signalAddresses.SetAt(i, new H323TransportAddress(info.rrq.m_callSignalAddress[i]));
  }

  if (signalAddresses.IsEmpty()) {
    info.rrj.m_rejectReason.SetTag(H225_RegistrationRejectReason::e_invalidCallSignalAddress);
    return H323GatekeeperRequest::Reject;
  }

  if (info.rrq.HasOptionalField(H225_RegistrationRequest::e_terminalAlias)) {
    aliases.SetSize(info.rrq.m_terminalAlias.GetSize());
    for (i = 0; i < info.rrq.m_terminalAlias.GetSize(); i++) {
      PString alias = H323GetAliasAddressString(info.rrq.m_terminalAlias[i]);
      aliases[i] = alias;

      PString password;
      if (!server.GetUsersPassword(alias, password)) {
        PTRACE(2, "RAS\tUser \"" << alias << "\" has no password.");
        info.rrj.m_rejectReason.SetTag(H225_RegistrationRejectReason::e_securityDenial);
        return H323GatekeeperRequest::Reject;
      }
      if (!password) {
        for (PINDEX j = 0; j < authenticators.GetSize(); j++) {
          H235Authenticator & authenticator = authenticators[j];
          if (authenticator.UseGkAndEpIdentifiers())
            authenticator.SetRemoteId(identifier);
          authenticator.SetPassword(password);
          if (!info.rrq.m_keepAlive)
            authenticator.Enable();
        }
      }
    }
  }

  const H225_EndpointType & terminalType = info.rrq.m_terminalType;
  if (terminalType.HasOptionalField(H225_EndpointType::e_gateway) &&
      terminalType.m_gateway.HasOptionalField(H225_GatewayInfo::e_protocol)) {
    const H225_ArrayOf_SupportedProtocols & protocols = terminalType.m_gateway.m_protocol;
    for (i = 0; i < protocols.GetSize(); i++) {

      // Only voice prefixes are supported
      if (protocols[i].GetTag() == H225_SupportedProtocols::e_voice) {
	H225_VoiceCaps & voiceCaps = protocols[i];
	if (voiceCaps.HasOptionalField(H225_VoiceCaps::e_supportedPrefixes)) {
	  H225_ArrayOf_SupportedPrefix & prefixes = voiceCaps.m_supportedPrefixes;
	  voicePrefixes.SetSize(prefixes.GetSize());
	  for (PINDEX j = 0; j < prefixes.GetSize(); j++) {
	    PString prefix = H323GetAliasAddressString(prefixes[j].m_prefix);
	    voicePrefixes[j] = prefix;
	  }
	}
	break;  // If voice protocol is found, don't look any further
      }
    }
  }

  timeToLive = server.GetTimeToLive();
  if (info.rrq.HasOptionalField(H225_RegistrationRequest::e_timeToLive) &&
      timeToLive > info.rrq.m_timeToLive)
    timeToLive = info.rrq.m_timeToLive;

  lastRegistration = PTime();

  // Build the RCF reply

  info.rcf.m_endpointIdentifier = identifier;

  if (aliases.GetSize() > 0) {
    info.rcf.m_terminalAlias.SetSize(aliases.GetSize());
    for (i = 0; i < aliases.GetSize(); i++)
      H323SetAliasAddress(aliases[i], info.rcf.m_terminalAlias[i]);
  }

  if (timeToLive > 0) {
    info.rcf.IncludeOptionalField(H225_RegistrationRequest::e_timeToLive);
    info.rcf.m_timeToLive = timeToLive;
  }

  return H323GatekeeperRequest::Confirm;
}


BOOL H323RegisteredEndPoint::HasExceededTimeToLive() const
{
  if (timeToLive == 0)
    return FALSE;

  PTime now;
  PTimeInterval delta = now - lastRegistration;
  return (unsigned)delta.GetSeconds() > timeToLive;
}


/////////////////////////////////////////////////////////////////////////////

H323GatekeeperListener::H323GatekeeperListener(H323EndPoint & ep,
                                           H323GatekeeperServer & gk,
                                           const PString & id,
                                           OpalTransport * trans)
  : H225_RAS(ep, trans),
    gatekeeper(gk)
{
  gatekeeperIdentifier = id;
  currentEP = NULL;

  transport->SetPromiscuous(TRUE);

  PTRACE(2, "H323gk\tGatekeeper server created.");
}


H323GatekeeperListener::~H323GatekeeperListener()
{
  PTRACE(2, "H323gk\tGatekeeper server destroyed.");
}


#if PTRACING
#define PDU_name PDU_name_variable
#else
#define PDU_name
#endif

BOOL H323GatekeeperListener::CheckGatekeeperIdentifier(unsigned optionalField,
                                                      const PASN_Sequence & pdu,
                                  const H225_GatekeeperIdentifier & identifier)
{
  if (!pdu.HasOptionalField(optionalField))
    return TRUE;

  PString gkid = identifier.GetValue();

  if (gatekeeperIdentifier == gkid)
    return TRUE;

  PTRACE(2, "RAS\t" << lastReceivedPDU->GetTagName()
         << " rejected, has different identifier, got \"" << gkid << '"');
  return FALSE;
}


BOOL H323GatekeeperListener::GetRegisteredEndPoint(unsigned rejectTag,
                                                   unsigned securityRejectTag,
                                                   PASN_Choice & reject,
                                                   unsigned securityField,
                                                   const PASN_Sequence & pdu,
                                                   const H225_ArrayOf_CryptoH323Token & cryptoTokens,
                                                   const H225_EndpointIdentifier & identifier)
{
  currentEP = gatekeeper.FindEndPointByIdentifier(identifier);
  if (currentEP == NULL) {
    reject.SetTag(rejectTag);
    PTRACE(2, "RAS\t" << lastReceivedPDU->GetTagName()
           << " rejected, \"" << identifier.GetValue() << "\" not registered");
    return FALSE;
  }

  transport->ConnectTo(currentEP->GetRASAddress(0));

  if (!CheckCryptoTokens(cryptoTokens, pdu, securityField)) {
    reject.SetTag(securityRejectTag);
    PTRACE(2, "RAS\t" << lastReceivedPDU->GetTagName()
           << " rejected, security tokens for \"" << identifier.GetValue() << "\" invalid.");
    return FALSE;
  }

  return TRUE;
}


BOOL H323GatekeeperListener::HandleRasPDU(const H323RasPDU & response)
{
  BOOL status = H225_RAS::HandleRasPDU(response);
  currentEP = NULL;
  return status;
}


H323GatekeeperRequest::Response
     H323GatekeeperListener::OnDiscovery(H323GatekeeperGRQ & info)
{
  if (info.grq.m_protocolIdentifier.GetSize() != 6 || info.grq.m_protocolIdentifier[5] < 2) {
    info.grj.m_rejectReason.SetTag(H225_GatekeeperRejectReason::e_invalidRevision);
    PTRACE(2, "RAS\tGRQ rejected, version 1 not supported");
    return H323GatekeeperRequest::Reject;
  }

  if (!CheckGatekeeperIdentifier(H225_GatekeeperRequest::e_gatekeeperIdentifier,
                                 info.grq, info.grq.m_gatekeeperIdentifier))
    return H323GatekeeperRequest::Reject;

  H323TransportAddress address = transport->GetLocalAddress();
  PTRACE(3, "RAS\tGRQ accepted on " << address);
  address.SetPDU(info.gcf.m_rasAddress);
  return H323GatekeeperRequest::Confirm;
}


BOOL H323GatekeeperListener::OnReceiveGatekeeperRequest(const H225_GatekeeperRequest & grq)
{
  return (new H323GatekeeperGRQ(*this, grq))->HandlePDU();
}


H323GatekeeperRequest::Response
     H323GatekeeperListener::OnRegistration(H323GatekeeperRRQ & info)
{
  if (info.rrq.m_keepAlive && info.rrq.HasOptionalField(H225_RegistrationRequest::e_endpointIdentifier))
    currentEP = gatekeeper.FindEndPointByIdentifier(info.rrq.m_endpointIdentifier);

  PINDEX i;
  OpalTransportAddress oldRemoteAddress = transport->GetRemoteAddress();
  for (i = 0; i < info.rrq.m_rasAddress.GetSize(); i++) {
    if (transport->ConnectTo(H323TransportAddress(info.rrq.m_rasAddress[i])))
      break;
  }

  if (i >= info.rrq.m_rasAddress.GetSize() &&
     (currentEP == NULL || !transport->ConnectTo(currentEP->GetRASAddress(0)))) {
    transport->ConnectTo(oldRemoteAddress);
    info.rrj.m_rejectReason.SetTag(H225_RegistrationRejectReason::e_invalidRASAddress);
    PTRACE(2, "RAS\tRRQ rejected, does not have valid RAS address!");
    return H323GatekeeperRequest::Reject;
  }

  if (info.rrq.m_protocolIdentifier.GetSize() != 6 || info.rrq.m_protocolIdentifier[5] < 2) {
    info.rrj.m_rejectReason.SetTag(H225_RegistrationRejectReason::e_invalidRevision);
    PTRACE(2, "RAS\tRRQ rejected, version 1 not supported");
    return H323GatekeeperRequest::Reject;
  }

  if (!CheckGatekeeperIdentifier(H225_RegistrationRequest::e_gatekeeperIdentifier,
                                 info.rrq, info.rrq.m_gatekeeperIdentifier))
    return H323GatekeeperRequest::Reject;

  H323GatekeeperRequest::Response response = gatekeeper.OnRegistration(currentEP, info);
  switch (response) {
    case H323GatekeeperRequest::Reject :
      PTRACE(2, "RAS\tRRQ rejected: " << info.rrj.m_rejectReason.GetTagName());
      break;

    case H323GatekeeperRequest::Confirm :
      if (!CheckCryptoTokens(info.rrq.m_cryptoTokens, info.rrq, H225_RegistrationRequest::e_cryptoTokens)) {
        info.rrj.m_rejectReason.SetTag(H225_RegistrationRejectReason::e_securityDenial);
        PTRACE(2, "RAS\tRRQ rejected, security tokens for \"" << info.rcf.m_endpointIdentifier.GetValue() << "\" invalid.");
        return H323GatekeeperRequest::Reject;
      }

      PTRACE(2, "RAS\tRRQ accepted: \"" << info.rcf.m_endpointIdentifier.GetValue() << '"');

      SetUpCallSignalAddresses(info.rcf.m_callSignalAddress);
  }

  return response;
}


BOOL H323GatekeeperListener::OnReceiveRegistrationRequest(const H225_RegistrationRequest & rrq)
{
  return (new H323GatekeeperRRQ(*this, rrq))->HandlePDU();
}


H323GatekeeperRequest::Response
     H323GatekeeperListener::OnUnregistration(H323GatekeeperURQ & info)
{
  if (info.urq.HasOptionalField(H225_RegistrationRequest::e_endpointIdentifier))
    currentEP = gatekeeper.FindEndPointByIdentifier(info.urq.m_endpointIdentifier);
  else
    currentEP = gatekeeper.FindEndPointBySignalAddresses(info.urq.m_callSignalAddress);

  if (currentEP == NULL) {
    info.urj.m_rejectReason.SetTag(H225_UnregRejectReason::e_notCurrentlyRegistered);
    PTRACE(2, "RAS\tURQ rejected, not registered");
    return H323GatekeeperRequest::Reject;
  }

  transport->ConnectTo(currentEP->GetRASAddress(0));

  return gatekeeper.OnUnregistration(currentEP, info);
}


BOOL H323GatekeeperListener::OnReceiveUnregistrationRequest(const H225_UnregistrationRequest & urq)
{
  H323GatekeeperURQ * info = new H323GatekeeperURQ(*this, urq);
  if (info->HandlePDU())
    return TRUE;

  if (currentEP != NULL && gatekeeper.FindEndPointByIdentifier(currentEP->GetIdentifier()) == NULL)
    delete currentEP;

  return FALSE;
}


BOOL H323GatekeeperListener::OnReceiveUnregistrationConfirm(const H225_UnregistrationConfirm & pdu)
{
  if (!H225_RAS::OnReceiveUnregistrationConfirm(pdu))
    return FALSE;

  return TRUE;
}


BOOL H323GatekeeperListener::OnReceiveUnregistrationReject(const H225_UnregistrationReject & pdu)
{
  if (!H225_RAS::OnReceiveUnregistrationReject(pdu))
    return FALSE;

  return TRUE;
}


H323GatekeeperRequest::Response
     H323GatekeeperListener::OnAdmission(H323GatekeeperARQ & info)
{
  if (!GetRegisteredEndPoint(H225_AdmissionRejectReason::e_invalidEndpointIdentifier,
                             H225_AdmissionRejectReason::e_securityDenial,
                             info.arj.m_rejectReason, H225_AdmissionRequest::e_cryptoTokens,
                             info.arq, info.arq.m_cryptoTokens, info.arq.m_endpointIdentifier))
    return H323GatekeeperRequest::Reject;

  if (!CheckGatekeeperIdentifier(H225_AdmissionRequest::e_gatekeeperIdentifier,
                                 info.arq, info.arq.m_gatekeeperIdentifier))
    return H323GatekeeperRequest::Reject;

  H323GatekeeperRequest::Response response = gatekeeper.OnAdmission(*currentEP, info);
  if (response != H323GatekeeperRequest::Confirm)
    return response;

  if (info.acf.m_callModel.GetTag() == H225_CallModel::e_gatekeeperRouted) {
    H225_ArrayOf_TransportAddress addresses;
    if (SetUpCallSignalAddresses(addresses))
      info.acf.m_destCallSignalAddress = addresses[0];
  }

  return H323GatekeeperRequest::Confirm;
}


BOOL H323GatekeeperListener::OnReceiveAdmissionRequest(const H225_AdmissionRequest & arq)
{
  return (new H323GatekeeperARQ(*this, arq))->HandlePDU();
}


H323GatekeeperRequest::Response
     H323GatekeeperListener::OnDisengage(H323GatekeeperDRQ & info)
{
  if (!GetRegisteredEndPoint(H225_DisengageRejectReason::e_notRegistered,
                             H225_DisengageRejectReason::e_securityDenial,
                             info.drj.m_rejectReason, H225_DisengageRequest::e_cryptoTokens,
                             info.drq, info.drq.m_cryptoTokens, info.drq.m_endpointIdentifier))
    return H323GatekeeperRequest::Reject;

  if (!CheckGatekeeperIdentifier(H225_DisengageRequest::e_gatekeeperIdentifier,
                                 info.drq, info.drq.m_gatekeeperIdentifier))
    return H323GatekeeperRequest::Reject;

  return gatekeeper.OnDisengage(*currentEP, info);
}


BOOL H323GatekeeperListener::OnReceiveDisengageRequest(const H225_DisengageRequest & drq)
{
  return (new H323GatekeeperDRQ(*this, drq))->HandlePDU();
}


BOOL H323GatekeeperListener::OnReceiveDisengageConfirm(const H225_DisengageConfirm & pdu)
{
  if (!H225_RAS::OnReceiveDisengageConfirm(pdu))
    return FALSE;

  return TRUE;
}


BOOL H323GatekeeperListener::OnReceiveDisengageReject(const H225_DisengageReject & pdu)
{
  if (!H225_RAS::OnReceiveDisengageReject(pdu))
    return FALSE;

  return TRUE;
}


H323GatekeeperRequest::Response
     H323GatekeeperListener::OnBandwidth(H323GatekeeperBRQ & info)
{
  if (!GetRegisteredEndPoint(H225_BandRejectReason::e_notBound,
                             H225_BandRejectReason::e_securityDenial,
                             info.brj.m_rejectReason, H225_BandwidthRequest::e_cryptoTokens,
                             info.brq, info.brq.m_cryptoTokens, info.brq.m_endpointIdentifier))
    return H323GatekeeperRequest::Reject;

  if (!CheckGatekeeperIdentifier(H225_BandwidthRequest::e_gatekeeperIdentifier,
                                 info.brq, info.brq.m_gatekeeperIdentifier))
    return H323GatekeeperRequest::Reject;

  return gatekeeper.OnBandwidth(*currentEP, info);
}


BOOL H323GatekeeperListener::OnReceiveBandwidthRequest(const H225_BandwidthRequest & brq)
{
  return (new H323GatekeeperBRQ(*this, brq))->HandlePDU();
}


BOOL H323GatekeeperListener::OnReceiveBandwidthConfirm(const H225_BandwidthConfirm & pdu)
{
  if (!H225_RAS::OnReceiveBandwidthConfirm(pdu))
    return FALSE;

  return TRUE;
}


BOOL H323GatekeeperListener::OnReceiveBandwidthReject(const H225_BandwidthReject & pdu)
{
  if (!H225_RAS::OnReceiveBandwidthReject(pdu))
    return FALSE;

  return TRUE;
}


H323GatekeeperRequest::Response
     H323GatekeeperListener::OnLocation(H323GatekeeperLRQ & info)
{
  if (info.lrq.HasOptionalField(H225_LocationRequest::e_endpointIdentifier)) {
    if (!GetRegisteredEndPoint(H225_LocationRejectReason::e_notRegistered,
                               H225_LocationRejectReason::e_securityDenial,
                               info.lrj.m_rejectReason, H225_LocationRequest::e_cryptoTokens,
                               info.lrq, info.lrq.m_cryptoTokens, info.lrq.m_endpointIdentifier))
      return H323GatekeeperRequest::Reject;
  }
  else if (!transport->ConnectTo(H323TransportAddress(info.lrq.m_replyAddress))) {
    info.lrj.m_rejectReason.SetTag(H225_LocationRejectReason::e_undefinedReason);
    PTRACE(2, "RAS\tLRQ does not have valid RAS reply address.");
    return H323GatekeeperRequest::Reject;
  }

  if (!CheckGatekeeperIdentifier(H225_LocationRequest::e_gatekeeperIdentifier,
                                 info.lrq, info.lrq.m_gatekeeperIdentifier))
    return H323GatekeeperRequest::Reject;

  return gatekeeper.OnLocation(info);
}


BOOL H323GatekeeperListener::OnReceiveLocationRequest(const H225_LocationRequest & lrq)
{
  return (new H323GatekeeperLRQ(*this, lrq))->HandlePDU();
}


BOOL H323GatekeeperListener::OnReceiveInfoRequestResponse(const H225_InfoRequestResponse & pdu)
{
  if (!H225_RAS::OnReceiveInfoRequestResponse(pdu))
    return FALSE;

  return TRUE;
}


BOOL H323GatekeeperListener::OnReceiveResourcesAvailableConfirm(const H225_ResourcesAvailableConfirm & pdu)
{
  if (!H225_RAS::OnReceiveResourcesAvailableConfirm(pdu))
    return FALSE;

  return TRUE;
}


BOOL H323GatekeeperListener::OnReceiveInfoRequestAck(const H225_InfoRequestAck & pdu)
{
  if (!H225_RAS::OnReceiveInfoRequestAck(pdu))
    return FALSE;

  return TRUE;
}


BOOL H323GatekeeperListener::OnReceiveInfoRequestNak(const H225_InfoRequestNak & pdu)
{
  if (!H225_RAS::OnReceiveInfoRequestNak(pdu))
    return FALSE;

  return TRUE;
}


H235Authenticators H323GatekeeperListener::GetAuthenticators() const
{
  H235Authenticators empty;
  if (currentEP == NULL)
    return empty;

  H235Authenticators authenticators = currentEP->GetAuthenticators();
  if (authenticators.IsEmpty())
    return empty;

  for (PINDEX i = 0; i < authenticators.GetSize(); i++) {
    H235Authenticator & authenticator = authenticators[i];
    if (authenticator.UseGkAndEpIdentifiers())
      authenticator.SetLocalId(gatekeeperIdentifier);
  }

  return authenticators;
}


/////////////////////////////////////////////////////////////////////////////

H323GatekeeperServer::H323GatekeeperServer(H323EndPoint & ep)
  : endpoint(ep)
{
  availableBandwidth = UINT_MAX;  // Unlimited total bandwidth
  defaultBandwidth = 2560;        // Enough for bidirectional G.711 and 64k H.261
  maximumBandwidth = 200000;      // 10baseX LAN bandwidth
  defaultTimeToLive = 3600;       // One hour, zero disables
  overwriteOnSameSignalAddress = TRUE;
  canOnlyCallRegisteredEP = FALSE;
  canOnlyAnswerRegisteredEP = FALSE;
  answerCallPreGrantedARQ = FALSE;
  makeCallPreGrantedARQ = FALSE;
  isGatekeeperRouted = FALSE;
  aliasCanBeHostName = TRUE;
  usingH235 = FALSE;

  identifierBase = time(NULL);
  nextIdentifier = 1;

  byIdentifier.DisallowDeleteObjects();
  byAddress.DisallowDeleteObjects();
  byAlias.DisallowDeleteObjects();
  byVoicePrefix.DisallowDeleteObjects();

  enumerationIndex = P_MAX_INDEX;
}


BOOL H323GatekeeperServer::AddListener(const OpalTransportAddress & interfaceName)
{
  PIPSocket::Address addr;
  WORD port = H225_RAS::DefaultRasUdpPort;
  if (!interfaceName.GetIpAndPort(addr, port))
    return AddListener(interfaceName.CreateTransport(endpoint, OpalTransportAddress::FullTSAP));

  if (addr != INADDR_ANY)
    return AddListener(new OpalTransportUDP(endpoint, addr, port));

  PIPSocket::InterfaceTable interfaces;
  if (!PIPSocket::GetInterfaceTable(interfaces)) {
    PTRACE(1, "RAS\tNo interfaces on system!");
    if (!PIPSocket::GetHostAddress(addr))
      return FALSE;
    return AddListener(new OpalTransportUDP(endpoint, addr, port));
  }

  PTRACE(4, "RAS\tAdding interfaces:\n" << setfill('\n') << interfaces << setfill(' '));

  BOOL atLeastOne = FALSE;

  PINDEX i;
  for (i = 0; i < interfaces.GetSize(); i++) {
    addr = interfaces[i].GetAddress();
    if (addr != 0 && addr != PIPSocket::Address()) {
      if (AddListener(new OpalTransportUDP(endpoint, addr, port)))
        atLeastOne = TRUE;
    }
  }

  return atLeastOne;
}


BOOL H323GatekeeperServer::AddListener(OpalTransport * transport)
{
  if (transport == NULL)
    return FALSE;

  if (!transport->IsOpen()) {
    delete transport;
    return FALSE;
  }

  return AddListener(CreateListener(transport));
}


BOOL H323GatekeeperServer::AddListener(H323GatekeeperListener * listener)
{
  if (listener == NULL)
    return FALSE;

  PWaitAndSignal wait(mutex);

  listeners.Append(listener);

  listener->StartRasChannel();

  return TRUE;
}


H323GatekeeperListener * H323GatekeeperServer::CreateListener(OpalTransport * transport)
{
  return new H323GatekeeperListener(endpoint, *this, gatekeeperIdentifier, transport);
}


BOOL H323GatekeeperServer::RemoveListener(H323GatekeeperListener * listener)
{
  mutex.Wait();
  BOOL ok = listeners.Remove(listener);
  mutex.Signal();
  return ok;
}


H323GatekeeperRequest::Response
     H323GatekeeperServer::OnRegistration(H323RegisteredEndPoint * & ep,
                                          H323GatekeeperRRQ & info)
{
  PINDEX i;

  BOOL isNew = FALSE;

  if (info.rrq.m_keepAlive) {
    if (ep == NULL) {
      info.rrj.m_rejectReason.SetTag(H225_RegistrationRejectReason::e_fullRegistrationRequired);
      PTRACE(2, "RAS\tRRQ keep alive rejected, not registered");
      return H323GatekeeperRequest::Reject;
    }
  }
  else {
    for (i = 0; i < info.rrq.m_callSignalAddress.GetSize(); i++) {
      H323RegisteredEndPoint * ep2 = FindEndPointBySignalAddress(info.rrq.m_callSignalAddress[i]);
      if (ep2 != NULL && ep2 != ep) {
        if (overwriteOnSameSignalAddress) {
          PTRACE(2, "RAS\tOverwriting existing endpoint " << *ep2);
          RemoveEndPoint(ep2);
        }
        else {
          info.rrj.m_rejectReason.SetTag(H225_RegistrationRejectReason::e_invalidCallSignalAddress);
          PTRACE(2, "RAS\tRRQ rejected, duplicate callSignalAddress");
          return H323GatekeeperRequest::Reject;
        }
      }
    }

    if (info.rrq.HasOptionalField(H225_RegistrationRequest::e_terminalAlias)) {
      for (i = 0; i < info.rrq.m_terminalAlias.GetSize(); i++) {
        H323RegisteredEndPoint * ep2 = FindEndPointByAliasAddress(info.rrq.m_terminalAlias[i]);
        if (ep2 != NULL && ep2 != ep) {
          info.rrj.m_rejectReason.SetTag(H225_RegistrationRejectReason::e_duplicateAlias);
          ((H225_AliasAddress&)info.rrj.m_rejectReason) = info.rrq.m_terminalAlias[i];
          PTRACE(2, "RAS\tRRQ rejected, duplicate alias");
          return H323GatekeeperRequest::Reject;
        }
      }
    }

    // Check if the endpoint is trying to register a prefix that can be resolved to another endpoint
    const H225_EndpointType & terminalType = info.rrq.m_terminalType;
    if (terminalType.HasOptionalField(H225_EndpointType::e_gateway) &&
	terminalType.m_gateway.HasOptionalField(H225_GatewayInfo::e_protocol)) {
      const H225_ArrayOf_SupportedProtocols & protocols = terminalType.m_gateway.m_protocol;
      for (i = 0; i < protocols.GetSize(); i++) {

	// Only voice prefixes are supported
	if (protocols[i].GetTag() == H225_SupportedProtocols::e_voice) {
	  H225_VoiceCaps & voiceCaps = protocols[i];
	  if (voiceCaps.HasOptionalField(H225_VoiceCaps::e_supportedPrefixes)) {
	    H225_ArrayOf_SupportedPrefix & prefixes = voiceCaps.m_supportedPrefixes;
	    for (PINDEX j = 0; j < prefixes.GetSize(); j++) {

	      // Reject if the prefix be matched to a registered alias or prefix
	      H323RegisteredEndPoint * ep2 = FindEndPointByAliasAddress(prefixes[j].m_prefix);
	      if (ep2 != NULL && ep2 != ep) {
		info.rrj.m_rejectReason.SetTag(H225_RegistrationRejectReason::e_duplicateAlias);
		((H225_AliasAddress&)info.rrj.m_rejectReason) = prefixes[j].m_prefix;
		PTRACE(2, "RAS\tRRQ rejected, duplicate prefix");
		return H323GatekeeperRequest::Reject;
	      }
	    }
	  }
	  break;  // If voice protocol is found, don't look any further
	}
      }
    }
 
    if (ep == NULL) {
      ep = CreateRegisteredEndPoint(info.rrq);
      if (ep == NULL) {
        info.rrj.m_rejectReason.SetTag(H225_RegistrationRejectReason::e_undefinedReason);
        PTRACE(1, "RAS\tRRQ rejected, CreateRegisteredEndPoint() returned NULL");
        return H323GatekeeperRequest::Reject;
      }

      isNew = TRUE;
    }
  }

  info.rcf.m_preGrantedARQ.m_answerCall = answerCallPreGrantedARQ;
  info.rcf.m_preGrantedARQ.m_useGKCallSignalAddressToAnswer = answerCallPreGrantedARQ && isGatekeeperRouted;
  info.rcf.m_preGrantedARQ.m_makeCall = makeCallPreGrantedARQ;
  info.rcf.m_preGrantedARQ.m_useGKCallSignalAddressToMakeCall = makeCallPreGrantedARQ && isGatekeeperRouted;

  H323GatekeeperRequest::Response response = ep->OnRegistration(*this, info);

  if (isNew) {
    if (response == H323GatekeeperRequest::Confirm)
      AddEndPoint(ep);
    else {
      delete ep;
      ep = NULL;
    }
  }

  return response;
}


H323GatekeeperRequest::Response
     H323GatekeeperServer::OnUnregistration(H323RegisteredEndPoint * ep,
                                            H323GatekeeperURQ & info)
{
  PINDEX i;

  if (info.urq.HasOptionalField(H225_UnregistrationRequest::e_endpointAlias)) {
    // See if all aliases to be removed are on the same endpoint
    for (i = 0; i < info.urq.m_endpointAlias.GetSize(); i++) {
      if (FindEndPointByAliasAddress(info.urq.m_endpointAlias[i]) != ep) {
        info.urj.m_rejectReason.SetTag(H225_UnregRejectReason::e_permissionDenied);
        PTRACE(2, "RAS\tURQ rejected, alias " << info.urq.m_endpointAlias[i]
               << " not owned by registration");
        return H323GatekeeperRequest::Reject;
      }
    }
    // Remove all the aliases specified in PDU, if no aliases left, then
    // RemoveAlias() will remove the endpoint
    for (i = 0; i < info.urq.m_endpointAlias.GetSize(); i++)
      RemoveAlias(H323GetAliasAddressString(info.urq.m_endpointAlias[i]), FALSE);
  }
  else
    RemoveEndPoint(ep, FALSE);

  return H323GatekeeperRequest::Confirm;
}


void H323GatekeeperServer::AddEndPoint(H323RegisteredEndPoint * ep)
{
  PTRACE(3, "RAS\tAdding registered endpoint: " << *ep);

  PINDEX i;

  mutex.Wait();

  byIdentifier.Append(ep);

  for (i = 0; i < ep->GetSignalAddressCount(); i++)
    byAddress.SetAt(ep->GetSignalAddress(i), ep);

  for (i = 0; i < ep->GetAliasCount(); i++)
    byAlias.SetAt(ep->GetAlias(i), ep);

  for (i = 0; i < ep->GetPrefixCount(); i++)
    byVoicePrefix.SetAt(ep->GetPrefix(i), ep);

  mutex.Signal();
}


void H323GatekeeperServer::RemoveEndPoint(H323RegisteredEndPoint * ep, BOOL autoDelete, BOOL lock)
{
  PTRACE(3, "RAS\tRemoving registered endpoint: " << *ep);

  PINDEX i;

  if (lock)
    mutex.Wait();

  for (i = 0; i < ep->GetPrefixCount(); i++)
    byVoicePrefix.SetAt(ep->GetPrefix(i), NULL);

  for (i = 0; i < ep->GetAliasCount(); i++)
    byAlias.SetAt(ep->GetAlias(i), NULL);

  for (i = 0; i < ep->GetSignalAddressCount(); i++)
    byAddress.SetAt(ep->GetSignalAddress(i), NULL);

  byIdentifier.Remove(ep);    // ep is deleted by this

  if (lock)
    mutex.Signal();

  if (autoDelete)
    delete ep;
}


void H323GatekeeperServer::RemoveAlias(const PString & alias, BOOL autoDelete)
{
  PTRACE(3, "RAS\tRemoving registered endpoint alias: " << alias);

  mutex.Wait();

  H323RegisteredEndPoint * ep = byAlias.GetAt(alias);
  if (ep != NULL) {
    byAlias.SetAt(alias, NULL);
    ep->RemoveAlias(alias);
    if (ep->GetAliasCount() == 0)
      RemoveEndPoint(ep, autoDelete, FALSE);
  }

  mutex.Signal();
}


H323RegisteredEndPoint * H323GatekeeperServer::CreateRegisteredEndPoint(
                                           const H225_RegistrationRequest & /*rrq*/)
{
  return new H323RegisteredEndPoint(CreateEndPointIdentifier());
}


PString H323GatekeeperServer::CreateEndPointIdentifier()
{
  PWaitAndSignal wait(mutex);
  return psprintf("%x:%u", identifierBase, nextIdentifier++);
}


H323RegisteredEndPoint * H323GatekeeperServer::FirstEndPoint()
{
  mutex.Wait();

  if (byIdentifier.GetSize() == 0) {
    AbortEnumeration();
    return NULL;
  }

  enumerationIndex = 0;
  return &byIdentifier[0];
}


H323RegisteredEndPoint * H323GatekeeperServer::NextEndPoint()
{
  if (enumerationIndex == P_MAX_INDEX)
    return NULL;

  enumerationIndex++;

  if (enumerationIndex < byIdentifier.GetSize())
    return &byIdentifier[enumerationIndex];

  AbortEnumeration();
  return NULL;
}


void H323GatekeeperServer::AbortEnumeration()
{
  enumerationIndex = P_MAX_INDEX;
  mutex.Signal();
}


void H323GatekeeperServer::AgeEndPoints()
{
  PTRACE(2, "RAS\tAging registered endpoints");

  PWaitAndSignal wait(mutex);

  for (PINDEX i = 0; i < byIdentifier.GetSize(); i++) {
    if (byIdentifier[i].HasExceededTimeToLive())
      RemoveEndPoint(&byIdentifier[i--], TRUE, FALSE);
  }
}


H323RegisteredEndPoint * H323GatekeeperServer::FindEndPointByIdentifier(const PString & identifier)
{
  if (identifier.IsEmpty())
    return NULL;

  PWaitAndSignal wait(mutex);

  PINDEX idx = byIdentifier.GetValuesIndex(H323RegisteredEndPoint(identifier));
  if (idx == P_MAX_INDEX)
    return NULL;

  return &byIdentifier[idx];
}


H323RegisteredEndPoint * H323GatekeeperServer::FindEndPointBySignalAddresses(
                               const H225_ArrayOf_TransportAddress & addresses)
{
  PWaitAndSignal wait(mutex);

  for (PINDEX i = 0; i < addresses.GetSize(); i++) {
    H323RegisteredEndPoint * ep = byAddress.GetAt(H323TransportAddress(addresses[i]));
    if (ep != NULL)
      return ep;
  }

  return NULL;
}


H323RegisteredEndPoint * H323GatekeeperServer::FindEndPointBySignalAddress(
                                          const H323TransportAddress & address)
{
  PWaitAndSignal wait(mutex);
  return byAddress.GetAt(address);
}


H323RegisteredEndPoint * H323GatekeeperServer::FindEndPointByAliasAddress(
                                               const H225_AliasAddress & alias)
{
  return FindEndPointByAliasString(H323GetAliasAddressString(alias));
}


H323RegisteredEndPoint * H323GatekeeperServer::FindEndPointByAliasString(
                                                         const PString & alias)
{
  PWaitAndSignal wait(mutex);
  H323RegisteredEndPoint * result = byAlias.GetAt(alias);
  
  // If alias is not registered, check if there is a prefix that matches any substring of the alias.
  // ( Would it be more efficient to check every registered prefix instead ? )
  if (result == NULL) {
    PINDEX i = alias.GetLength();
    while (result == NULL && i > 0)
      result = byVoicePrefix.GetAt(alias.Left(i--));
  }

  return result;
}


H323GatekeeperRequest::Response
     H323GatekeeperServer::OnAdmission(H323RegisteredEndPoint & ep,
                                       H323GatekeeperARQ & info)
{
  PINDEX i;

  unsigned bandwidthAllocated = AllocateBandwidth(info.arq.m_bandWidth);
  if (bandwidthAllocated == 0) {
    info.arj.m_rejectReason.SetTag(H225_AdmissionRejectReason::e_requestDenied);
    PTRACE(2, "RAS\tARQ rejected, not enough bandwidth");
    return H323GatekeeperRequest::Reject;
  }
  info.acf.m_bandWidth = bandwidthAllocated;


  if (info.arq.m_answerCall) {
    // Incoming call

    BOOL denied = TRUE;
    for (i = 0; i < info.arq.m_srcInfo.GetSize(); i++) {
      if (CheckAliasAddressPolicy(ep, info.arq, info.arq.m_srcInfo[i])) {
        denied = FALSE;
        break;
      }
    }

    if (info.arq.HasOptionalField(H225_AdmissionRequest::e_srcCallSignalAddress)) {
      if (CheckSignalAddressPolicy(ep, info.arq, H323TransportAddress(info.arq.m_srcCallSignalAddress)))
        denied = FALSE;
    }

    if (denied) {
      info.arj.m_rejectReason.SetTag(H225_AdmissionRejectReason::e_securityDenial);
      PTRACE(2, "RAS\tARQ rejected, not allowed to answer call");
      return H323GatekeeperRequest::Reject;
    }

    info.acf.m_destCallSignalAddress = info.arq.m_destCallSignalAddress;
  }
  else {
    // Outgoing call.

    H323TransportAddress destAddress;
    if (info.arq.HasOptionalField(H225_AdmissionRequest::e_destCallSignalAddress))
      destAddress = info.arq.m_destCallSignalAddress;

    H323RegisteredEndPoint * destEP = NULL;

    if (info.arq.HasOptionalField(H225_AdmissionRequest::e_destinationInfo)) {

      // Have provided an alias, see if we can route it.
      BOOL denied = TRUE;
      BOOL noAddress = TRUE;
      for (i = 0; i < info.arq.m_destinationInfo.GetSize(); i++) {
        if (CheckAliasAddressPolicy(ep, info.arq, info.arq.m_destinationInfo[i])) {
          denied = FALSE;
          if (TranslateAliasAddressToSignalAddress(info.arq.m_destinationInfo[i], destAddress)) {
            destEP = FindEndPointByAliasAddress(info.arq.m_destinationInfo[i]);
            noAddress = FALSE;
            break;
          }
        }
      }

      if (denied) {
        info.arj.m_rejectReason.SetTag(H225_AdmissionRejectReason::e_securityDenial);
        PTRACE(2, "RAS\tARQ rejected, not allowed to make call");
        return H323GatekeeperRequest::Reject;
      }

      if (noAddress) {
        info.arj.m_rejectReason.SetTag(H225_AdmissionRejectReason::e_calledPartyNotRegistered);
        PTRACE(2, "RAS\tARQ rejected, destination alias not registered");
        return H323GatekeeperRequest::Reject;
      }
    }

    // Has provided an alias and signal address, see if agree
    if (destEP != NULL &&
        info.arq.HasOptionalField(H225_AdmissionRequest::e_destCallSignalAddress) &&
        FindEndPointBySignalAddress(info.arq.m_destCallSignalAddress) != destEP) {
      info.arj.m_rejectReason.SetTag(H225_AdmissionRejectReason::e_aliasesInconsistent);
      PTRACE(2, "RAS\tARQ rejected, destination address not for specified alias");
      return H323GatekeeperRequest::Reject;
    }

    // HAve no address from anywhere
    if (destAddress.IsEmpty()) {
      info.arj.m_rejectReason.SetTag(H225_AdmissionRejectReason::e_incompleteAddress);
      PTRACE(2, "RAS\tARQ rejected, must have destination address or alias");
      return H323GatekeeperRequest::Reject;
    }

    if (!CheckSignalAddressPolicy(ep, info.arq, destAddress)) {
      info.arj.m_rejectReason.SetTag(H225_AdmissionRejectReason::e_securityDenial);
      PTRACE(2, "RAS\tARQ rejected, not allowed to make call");
      return H323GatekeeperRequest::Reject;
    }

    destAddress.SetPDU(info.acf.m_destCallSignalAddress);

    if (isGatekeeperRouted)
      info.acf.m_callModel.SetTag(H225_CallModel::e_gatekeeperRouted);
  }

  H323GatekeeperCall * call = CreateCall(ep, info.arq.m_callIdentifier.m_guid);
  H323GatekeeperRequest::Response response = call->OnAdmission(*this, info);
  if (response == H323GatekeeperRequest::Confirm)
    AddCall(call);
  else
    delete call;

  return response;
}


H323GatekeeperRequest::Response
     H323GatekeeperServer::OnDisengage(H323RegisteredEndPoint & /*ep*/,
                                       H323GatekeeperDRQ & info)
{
  H323GatekeeperCall * call = FindCall(info.drq.m_callIdentifier.m_guid);
  if (call == NULL) {
    info.drj.m_rejectReason.SetTag(H225_DisengageRejectReason::e_requestToDropOther);
    PTRACE(2, "RAS\tDRQ rejected, no call with ID");
    return H323GatekeeperRequest::Reject;
  }

  H323GatekeeperRequest::Response response = call->OnDisengage(*this, info);
  if (response != H323GatekeeperRequest::Confirm)
    return response;

  RemoveCall(call);
  return H323GatekeeperRequest::Confirm;
}


H323GatekeeperRequest::Response
     H323GatekeeperServer::OnBandwidth(H323RegisteredEndPoint & ep,
                                       H323GatekeeperBRQ & info)
{
  H323GatekeeperCall * call = FindCall(info.brq.m_callIdentifier.m_guid);
  if (call == NULL) {
    info.brj.m_rejectReason.SetTag(H225_BandRejectReason::e_invalidConferenceID);
    PTRACE(2, "RAS\tBRQ rejected, no call with ID");
    return H323GatekeeperRequest::Reject;
  }

  if (&call->GetEndPoint() != &ep) {
    info.brj.m_rejectReason.SetTag(H225_BandRejectReason::e_invalidPermission);
    PTRACE(2, "RAS\tBRQ rejected, call is not owned by endpoint");
    return H323GatekeeperRequest::Reject;
  }

  unsigned newBandwidth = AllocateBandwidth(info.brq.m_bandWidth, call->GetBandwidthUsed());
  if (newBandwidth == 0) {
    info.brj.m_rejectReason.SetTag(H225_BandRejectReason::e_insufficientResources);
    PTRACE(2, "RAS\tBRQ rejected, no bandwidth");
    return H323GatekeeperRequest::Reject;
  }

  info.bcf.m_bandWidth = newBandwidth;
  return H323GatekeeperRequest::Confirm;
}


BOOL H323GatekeeperServer::GetUsersPassword(const PString & alias,
                                            PString & password) const
{
  if (usingH235) {
    if (passwords.Contains(alias)) {
      password = passwords[alias];
      return TRUE;
    }
    return FALSE;
  }

  password = PString();
  return TRUE;
}


unsigned H323GatekeeperServer::AllocateBandwidth(unsigned newBandwidth,
                                                 unsigned oldBandwidth)
{
  PWaitAndSignal wait(mutex);

  // If first request for bandwidth, then only give them a maximum of the
  // configured default bandwidth
  if (oldBandwidth == 0 && newBandwidth > defaultBandwidth)
    newBandwidth = defaultBandwidth;

  // If then are asking for more than we have in total, drop it down to whatevers left
  if (newBandwidth > oldBandwidth && (newBandwidth - oldBandwidth) > availableBandwidth)
    newBandwidth = availableBandwidth  - oldBandwidth;

  // If greater than the absolute maximum configured for any endpoint, clamp it
  if (newBandwidth > maximumBandwidth)
    newBandwidth = maximumBandwidth;

  // Finally have adjusted new bandwidth, allocate it!
  availableBandwidth -= (newBandwidth - oldBandwidth);
  return newBandwidth;
}


void H323GatekeeperServer::AddCall(H323GatekeeperCall * call)
{
  activeCalls.Append(call);
}


void H323GatekeeperServer::RemoveCall(H323GatekeeperCall * call)
{
  AllocateBandwidth(0, call->GetBandwidthUsed());
  activeCalls.Remove(call);
}


H323GatekeeperCall * H323GatekeeperServer::CreateCall(H323RegisteredEndPoint & endpoint,
                                            const OpalGloballyUniqueID & callIdentifier)
{
  return new H323GatekeeperCall(callIdentifier, &endpoint);
}


H323GatekeeperCall * H323GatekeeperServer::FindCall(const OpalGloballyUniqueID & callIdentifier)
{
  PINDEX idx = activeCalls.GetValuesIndex(H323GatekeeperCall(callIdentifier));
  if (idx == P_MAX_INDEX)
    return NULL;

  return &activeCalls[idx];
}


H323GatekeeperRequest::Response
     H323GatekeeperServer::OnLocation(H323GatekeeperLRQ & info)
{
  PINDEX i;
  for (i = 0; i < info.lrq.m_destinationInfo.GetSize(); i++) {
    H323TransportAddress address;
    if (TranslateAliasAddressToSignalAddress(info.lrq.m_destinationInfo[i], address)) {
      address.SetPDU(info.lcf.m_callSignalAddress);
      return H323GatekeeperRequest::Confirm;
    }
  }

  info.lrj.m_rejectReason.SetTag(H225_LocationRejectReason::e_requestDenied);
  PTRACE(2, "RAS\tLRQ rejected, not found");
  return H323GatekeeperRequest::Reject;
}


BOOL H323GatekeeperServer::TranslateAliasAddressToSignalAddress(const H225_AliasAddress & alias,
                                                                H323TransportAddress & address)
{
  PString aliasString = H323GetAliasAddressString(alias);

  if (isGatekeeperRouted) {
    const OpalListenerList & listeners = endpoint.GetListeners();
    address = listeners[0].GetLocalAddress();
    PTRACE(2, "RAS\tTranslating alias " << aliasString << " to " << address << ", gatekeeper routed");
    return TRUE;
  }

  H323RegisteredEndPoint * ep = FindEndPointByAliasAddress(alias);
  if (ep != NULL) {
    address = ep->GetSignalAddress(0);
    PTRACE(2, "RAS\tTranslating alias " << aliasString << " to " << address << ", registered endpoint");
    return TRUE;
  }

  if (aliasCanBeHostName) {
    PIPSocket::Address ip;
    if (PIPSocket::GetHostAddress(aliasString, ip)) {
      address = H323TransportAddress(ip, endpoint.GetDefaultSignalPort());
      PTRACE(2, "RAS\tTranslating alias " << aliasString << " to " << address << ", DNS host name");
    }
  }

  return !address;
}


BOOL H323GatekeeperServer::CheckSignalAddressPolicy(const H323RegisteredEndPoint &,
                                                    const H225_AdmissionRequest &,
                                                    const OpalTransportAddress &)
{
  return TRUE;
}


BOOL H323GatekeeperServer::CheckAliasAddressPolicy(const H323RegisteredEndPoint &,
                                                  const H225_AdmissionRequest & arq,
                                                  const H225_AliasAddress & alias)
{
  if (arq.m_answerCall ? canOnlyAnswerRegisteredEP : canOnlyCallRegisteredEP) {
    H323RegisteredEndPoint * ep = FindEndPointByAliasAddress(alias);
    if (ep == NULL)
      return FALSE;
  }

  return TRUE;
}


BOOL H323GatekeeperServer::CheckAliasStringPolicy(const H323RegisteredEndPoint &,
                                                  const H225_AdmissionRequest & arq,
                                                  const PString & alias)
{
  if (arq.m_answerCall ? canOnlyAnswerRegisteredEP : canOnlyCallRegisteredEP) {
    H323RegisteredEndPoint * ep = FindEndPointByAliasString(alias);
    if (ep == NULL)
      return FALSE;
  }

  return TRUE;
}


void H323GatekeeperServer::SetGatekeeperIdentifier(const PString & id,
                                                   BOOL adjustListeners)
{
  mutex.Wait();

  gatekeeperIdentifier = id;

  if (adjustListeners) {
    for (PINDEX i = 0; i < listeners.GetSize(); i++)
      listeners[i].SetIdentifier(id);
  }

  mutex.Signal();
}


/////////////////////////////////////////////////////////////////////////////
