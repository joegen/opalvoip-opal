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
 * Revision 1.2003  2001/10/05 00:22:13  robertj
 * Updated to PWLib 1.2.0 and OpenH323 1.7.0
 *
 * Revision 2.1  2001/08/13 05:10:39  robertj
 * Updates from OpenH323 v1.6.0 release.
 *
 * Revision 2.0  2001/07/27 15:48:25  robertj
 * Conversion of OpenH323 to Open Phone Abstraction Library (OPAL)
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

H323GatekeeperCall::H323GatekeeperCall(const OpalGloballyUniqueID & id,
                                       H323RegisteredEndPoint * ep)
  : callIdentifier(id),
    conferenceIdentifier(NULL)
{
  endpoint = ep;
  if (endpoint != NULL)
    endpoint->AddCall(this);
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


BOOL H323GatekeeperCall::OnAdmission(const H323GatekeeperServer & /*server*/,
                                     const H225_AdmissionRequest & arq,
                                     H225_AdmissionConfirm & acf,
                                     H225_AdmissionReject & /*arj*/)
{
  callReference = arq.m_callReferenceValue;
  conferenceIdentifier = arq.m_conferenceID;
  srcAddress = arq.m_srcCallSignalAddress;
  if (arq.m_answerCall)
    dstAddress = arq.m_destCallSignalAddress;
  else
    dstAddress = acf.m_destCallSignalAddress;
  bandwidthUsed = acf.m_bandWidth;
  return TRUE;
}


BOOL H323GatekeeperCall::OnDisengage(const H323GatekeeperServer & /*server*/,
                                     const H225_DisengageRequest & /*drq*/,
                                     H225_DisengageConfirm & /*dcf*/,
                                     H225_DisengageReject & /*drj*/)
{
  return TRUE;
}


/////////////////////////////////////////////////////////////////////////////

H323RegisteredEndPoint::H323RegisteredEndPoint(const PString & id)
  : identifier(id)
{
  PTRACE(4, "RAS\tCreated registered endpoint: " << id);

#ifdef P_SSL
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


BOOL H323RegisteredEndPoint::OnRegistration(const H323GatekeeperServer & server,
                                            const H225_RegistrationRequest & rrq,
                                            H225_RegistrationConfirm & rcf,
                                            H225_RegistrationReject & rrj)
{
  PINDEX i;

  if (rrq.m_rasAddress.GetSize() > 0) {
    rasAddresses.SetSize(rrq.m_rasAddress.GetSize());
    for (i = 0; i < rrq.m_rasAddress.GetSize(); i++) {
      H323TransportAddress address(rrq.m_rasAddress[i]);
      if (!address)
        rasAddresses.SetAt(i, new OpalTransportAddress(address));
    }
  }

  if (rasAddresses.IsEmpty()) {
    rrj.m_rejectReason.SetTag(H225_RegistrationRejectReason::e_invalidRASAddress);
    return FALSE;
  }

  if (rrq.m_callSignalAddress.GetSize() > 0) {
    signalAddresses.SetSize(rrq.m_callSignalAddress.GetSize());
    for (i = 0; i < rrq.m_callSignalAddress.GetSize(); i++)
      signalAddresses.SetAt(i, new H323TransportAddress(rrq.m_callSignalAddress[i]));
  }

  if (signalAddresses.IsEmpty()) {
    rrj.m_rejectReason.SetTag(H225_RegistrationRejectReason::e_invalidCallSignalAddress);
    return FALSE;
  }

  if (rrq.HasOptionalField(H225_RegistrationRequest::e_terminalAlias)) {
    aliases.SetSize(rrq.m_terminalAlias.GetSize());
    for (i = 0; i < rrq.m_terminalAlias.GetSize(); i++) {
      PString alias = H323GetAliasAddressString(rrq.m_terminalAlias[i]);
      aliases[i] = alias;

      PString password;
      if (!server.GetUsersPassword(alias, password)) {
        PTRACE(2, "RAS\tUser \"" << alias << "\" has no password.");
        rrj.m_rejectReason.SetTag(H225_RegistrationRejectReason::e_securityDenial);
        return FALSE;
      }
      if (!password) {
        for (PINDEX j = 0; j < authenticators.GetSize(); j++) {
          H235Authenticator & authenticator = authenticators[j];
          if (authenticator.UseGkAndEpIdentifiers())
            authenticator.SetRemoteId(identifier);
          authenticator.SetPassword(password);
          if (!rrq.m_keepAlive)
            authenticator.Enable();
        }
      }
    }
  }

  timeToLive = server.GetTimeToLive();
  if (rrq.HasOptionalField(H225_RegistrationRequest::e_timeToLive) && timeToLive > rrq.m_timeToLive)
    timeToLive = rrq.m_timeToLive;

  lastRegistration = PTime();

  // Build the RCF reply

  rcf.m_endpointIdentifier = identifier;

  if (aliases.GetSize() > 0) {
    rcf.m_terminalAlias.SetSize(aliases.GetSize());
    for (i = 0; i < aliases.GetSize(); i++)
      H323SetAliasAddress(aliases[i], rcf.m_terminalAlias[i]);
  }

  if (timeToLive > 0) {
    rcf.IncludeOptionalField(H225_RegistrationRequest::e_timeToLive);
    rcf.m_timeToLive = timeToLive;
  }

  return TRUE;
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


BOOL H323GatekeeperListener::OnDiscovery(const H225_GatekeeperRequest & grq,
                                         H225_GatekeeperConfirm & gcf,
                                         H225_GatekeeperReject & grj)
{
  if (grq.m_protocolIdentifier.GetSize() != 6 || grq.m_protocolIdentifier[5] < 2) {
    grj.m_rejectReason.SetTag(H225_GatekeeperRejectReason::e_invalidRevision);
    PTRACE(2, "RAS\tGRQ rejected, version 1 not supported");
    return FALSE;
  }

  if (!CheckGatekeeperIdentifier(H225_GatekeeperRequest::e_gatekeeperIdentifier,
                                 grq, grq.m_gatekeeperIdentifier))
    return FALSE;

  H323TransportAddress address = transport->GetLocalAddress();
  PTRACE(3, "RAS\tGRQ accepted on " << address);
  address.SetPDU(gcf.m_rasAddress);
  return TRUE;
}


BOOL H323GatekeeperListener::OnReceiveGatekeeperRequest(const H225_GatekeeperRequest & request)
{
  if (!transport->ConnectTo(H323TransportAddress(request.m_rasAddress))) {
    PTRACE(2, "RAS\tGRQ does not have valid RAS address! Ignoring PDU.");
    return FALSE;
  }

  H323RasPDU confirm(*this);
  H323RasPDU reject(*this);
  if (OnDiscovery(request,
                  confirm.BuildGatekeeperConfirm(request.m_requestSeqNum),
                  reject.BuildGatekeeperReject(request.m_requestSeqNum,
                                               H225_GatekeeperRejectReason::e_terminalExcluded)))
    return WritePDU(confirm);
  else
    return WritePDU(reject);
}


BOOL H323GatekeeperListener::OnRegistration(const H225_RegistrationRequest & rrq,
                                            H225_RegistrationConfirm & rcf,
                                            H225_RegistrationReject & rrj)
{
  if (rrq.m_keepAlive && rrq.HasOptionalField(H225_RegistrationRequest::e_endpointIdentifier))
    currentEP = gatekeeper.FindEndPointByIdentifier(rrq.m_endpointIdentifier);

  PINDEX i;
  OpalTransportAddress oldRemoteAddress = transport->GetRemoteAddress();
  for (i = 0; i < rrq.m_rasAddress.GetSize(); i++) {
    if (transport->ConnectTo(H323TransportAddress(rrq.m_rasAddress[i])))
      break;
  }

  if (i >= rrq.m_rasAddress.GetSize() &&
     (currentEP == NULL || !transport->ConnectTo(currentEP->GetRASAddress(0)))) {
    transport->ConnectTo(oldRemoteAddress);
    rrj.m_rejectReason.SetTag(H225_RegistrationRejectReason::e_invalidRASAddress);
    PTRACE(2, "RAS\tRRQ rejected, does not have valid RAS address!");
    return FALSE;
  }

  if (rrq.m_protocolIdentifier.GetSize() != 6 || rrq.m_protocolIdentifier[5] < 2) {
    rrj.m_rejectReason.SetTag(H225_RegistrationRejectReason::e_invalidRevision);
    PTRACE(2, "RAS\tRRQ rejected, version 1 not supported");
    return FALSE;
  }

  if (!CheckGatekeeperIdentifier(H225_RegistrationRequest::e_gatekeeperIdentifier,
                                 rrq, rrq.m_gatekeeperIdentifier))
    return FALSE;

  if (!gatekeeper.OnRegistration(currentEP, rrq, rcf, rrj)) {
    PTRACE(2, "RAS\tRRQ rejected: " << rrj.m_rejectReason.GetTagName());
    return FALSE;
  }

  if (!CheckCryptoTokens(rrq.m_cryptoTokens, rrq, H225_RegistrationRequest::e_cryptoTokens)) {
    rrj.m_rejectReason.SetTag(H225_RegistrationRejectReason::e_securityDenial);
    PTRACE(2, "RAS\tRRQ rejected, security tokens for \"" << rcf.m_endpointIdentifier.GetValue() << "\" invalid.");
    return FALSE;
  }

  PTRACE(2, "RAS\tRRQ accepted: \"" << rcf.m_endpointIdentifier.GetValue() << '"');

  SetUpCallSignalAddresses(rcf.m_callSignalAddress);

  return TRUE;
}


BOOL H323GatekeeperListener::OnReceiveRegistrationRequest(const H225_RegistrationRequest & request)
{
  H323RasPDU confirm(*this);
  H323RasPDU reject(*this);
  if (OnRegistration(request,
                     confirm.BuildRegistrationConfirm(request.m_requestSeqNum),
                     reject.BuildRegistrationReject(request.m_requestSeqNum)))
    return WritePDU(confirm);
  else
    return WritePDU(reject);
}


BOOL H323GatekeeperListener::OnUnregistration(const H225_UnregistrationRequest & urq,
                                              H225_UnregistrationConfirm & ucf,
                                              H225_UnregistrationReject & urj)
{
  if (urq.HasOptionalField(H225_RegistrationRequest::e_endpointIdentifier))
    currentEP = gatekeeper.FindEndPointByIdentifier(urq.m_endpointIdentifier);
  else
    currentEP = gatekeeper.FindEndPointBySignalAddresses(urq.m_callSignalAddress);

  if (currentEP == NULL) {
    urj.m_rejectReason.SetTag(H225_UnregRejectReason::e_notCurrentlyRegistered);
    PTRACE(2, "RAS\tURQ rejected, not registered");
    return FALSE;
  }

  transport->ConnectTo(currentEP->GetRASAddress(0));

  return gatekeeper.OnUnregistration(currentEP, urq, ucf, urj);
}


BOOL H323GatekeeperListener::OnReceiveUnregistrationRequest(const H225_UnregistrationRequest & request)
{
  BOOL ok;
  H323RasPDU confirm(*this);
  H323RasPDU reject(*this);
  if (OnUnregistration(request,
                       confirm.BuildUnregistrationConfirm(request.m_requestSeqNum),
                       reject.BuildUnregistrationReject(request.m_requestSeqNum)))
    ok = WritePDU(confirm);
  else
    ok = WritePDU(reject);

  if (currentEP != NULL && gatekeeper.FindEndPointByIdentifier(currentEP->GetIdentifier()) == NULL)
    delete currentEP;

  return ok;
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


BOOL H323GatekeeperListener::OnAdmission(const H225_AdmissionRequest & arq,
                                         H225_AdmissionConfirm & acf,
                                         H225_AdmissionReject & arj)
{
  if (!GetRegisteredEndPoint(H225_AdmissionRejectReason::e_invalidEndpointIdentifier,
                             H225_AdmissionRejectReason::e_securityDenial,
                             arj.m_rejectReason, H225_AdmissionRequest::e_cryptoTokens,
                             arq, arq.m_cryptoTokens, arq.m_endpointIdentifier))
    return FALSE;

  if (!CheckGatekeeperIdentifier(H225_AdmissionRequest::e_gatekeeperIdentifier,
                                 arq, arq.m_gatekeeperIdentifier))
    return FALSE;

  if (!gatekeeper.OnAdmission(*currentEP, arq, acf, arj))
    return FALSE;

  if (acf.m_callModel.GetTag() == H225_CallModel::e_gatekeeperRouted) {
    H225_ArrayOf_TransportAddress addresses;
    if (SetUpCallSignalAddresses(addresses))
      acf.m_destCallSignalAddress = addresses[0];
  }

  return TRUE;
}


BOOL H323GatekeeperListener::OnReceiveAdmissionRequest(const H225_AdmissionRequest & request)
{
  H323RasPDU confirm(*this);
  H323RasPDU reject(*this);
  if (OnAdmission(request,
                  confirm.BuildAdmissionConfirm(request.m_requestSeqNum),
                  reject.BuildAdmissionReject(request.m_requestSeqNum)))
    return WritePDU(confirm);
  else
    return WritePDU(reject);
}


BOOL H323GatekeeperListener::OnDisengage(const H225_DisengageRequest & drq,
                                         H225_DisengageConfirm & dcf,
                                         H225_DisengageReject & drj)
{
  if (!GetRegisteredEndPoint(H225_DisengageRejectReason::e_notRegistered,
                             H225_DisengageRejectReason::e_securityDenial,
                             drj.m_rejectReason, H225_DisengageRequest::e_cryptoTokens,
                             drq, drq.m_cryptoTokens, drq.m_endpointIdentifier))
    return FALSE;

  if (!CheckGatekeeperIdentifier(H225_DisengageRequest::e_gatekeeperIdentifier,
                                 drq, drq.m_gatekeeperIdentifier))
    return FALSE;

  return gatekeeper.OnDisengage(*currentEP, drq, dcf, drj);
}


BOOL H323GatekeeperListener::OnReceiveDisengageRequest(const H225_DisengageRequest & request)
{
  H323RasPDU confirm(*this);
  H323RasPDU reject(*this);
  if (OnDisengage(request,
                  confirm.BuildDisengageConfirm(request.m_requestSeqNum),
                  reject.BuildDisengageReject(request.m_requestSeqNum)))
    return WritePDU(confirm);
  else
    return WritePDU(reject);
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


BOOL H323GatekeeperListener::OnBandwidth(const H225_BandwidthRequest & brq,
                                         H225_BandwidthConfirm & bcf,
                                         H225_BandwidthReject & brj)
{
  if (!GetRegisteredEndPoint(H225_BandRejectReason::e_notBound,
                             H225_BandRejectReason::e_securityDenial,
                             brj.m_rejectReason, H225_BandwidthRequest::e_cryptoTokens,
                             brq, brq.m_cryptoTokens, brq.m_endpointIdentifier))
    return FALSE;

  if (!CheckGatekeeperIdentifier(H225_BandwidthRequest::e_gatekeeperIdentifier,
                                 brq, brq.m_gatekeeperIdentifier))
    return FALSE;

  return gatekeeper.OnBandwidth(*currentEP, brq, bcf, brj);
}


BOOL H323GatekeeperListener::OnReceiveBandwidthRequest(const H225_BandwidthRequest & request)
{
  H323RasPDU confirm(*this);
  H323RasPDU reject(*this);
  if (OnBandwidth(request,
                  confirm.BuildBandwidthConfirm(request.m_requestSeqNum),
                  reject.BuildBandwidthReject(request.m_requestSeqNum)))
    return WritePDU(confirm);
  else
    return WritePDU(reject);
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


BOOL H323GatekeeperListener::OnLocation(const H225_LocationRequest & lrq,
                                        H225_LocationConfirm & lcf,
                                        H225_LocationReject & lrj)
{
  if (lrq.HasOptionalField(H225_LocationRequest::e_endpointIdentifier)) {
    if (!GetRegisteredEndPoint(H225_LocationRejectReason::e_notRegistered,
                               H225_LocationRejectReason::e_securityDenial,
                               lrj.m_rejectReason, H225_LocationRequest::e_cryptoTokens,
                               lrq, lrq.m_cryptoTokens, lrq.m_endpointIdentifier))
      return FALSE;
  }
  else if (!transport->ConnectTo(H323TransportAddress(lrq.m_replyAddress))) {
    lrj.m_rejectReason.SetTag(H225_LocationRejectReason::e_undefinedReason);
    PTRACE(2, "RAS\tLRQ does not have valid RAS reply address.");
    return FALSE;
  }

  if (!CheckGatekeeperIdentifier(H225_LocationRequest::e_gatekeeperIdentifier,
                                 lrq, lrq.m_gatekeeperIdentifier))
    return FALSE;

  return gatekeeper.OnLocation(lrq, lcf, lrj);
}


BOOL H323GatekeeperListener::OnReceiveLocationRequest(const H225_LocationRequest & request)
{
  H323RasPDU confirm(*this);
  H323RasPDU reject(*this);
  if (OnLocation(request,
                 confirm.BuildLocationConfirm(request.m_requestSeqNum),
                 reject.BuildLocationReject(request.m_requestSeqNum)))
    return WritePDU(confirm);
  else
    return WritePDU(reject);
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

  enumerationIndex = P_MAX_INDEX;
}


BOOL H323GatekeeperServer::AddListener(const OpalTransportAddress & interfaceName)
{
  PIPSocket::Address addr;
  WORD port = H225_RAS::DefaultRasUdpPort;
  if (!interfaceName.GetIpAndPort(addr, port))
    return AddListener(interfaceName.CreateTransport(endpoint));

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


BOOL H323GatekeeperServer::OnRegistration(H323RegisteredEndPoint * & ep,
                                          const H225_RegistrationRequest & rrq,
                                          H225_RegistrationConfirm & rcf,
                                          H225_RegistrationReject & rrj)
{
  PINDEX i;

  BOOL isNew = FALSE;

  if (rrq.m_keepAlive) {
    if (ep == NULL) {
      rrj.m_rejectReason.SetTag(H225_RegistrationRejectReason::e_fullRegistrationRequired);
      PTRACE(2, "RAS\tRRQ keep alive rejected, not registered");
      return FALSE;
    }
  }
  else {
    for (i = 0; i < rrq.m_callSignalAddress.GetSize(); i++) {
      H323RegisteredEndPoint * ep2 = FindEndPointBySignalAddress(rrq.m_callSignalAddress[i]);
      if (ep2 != NULL && ep2 != ep) {
        if (overwriteOnSameSignalAddress) {
          PTRACE(2, "RAS\tOverwriting existing endpoint " << *ep2);
          RemoveEndPoint(ep2);
        }
        else {
          rrj.m_rejectReason.SetTag(H225_RegistrationRejectReason::e_invalidCallSignalAddress);
          PTRACE(2, "RAS\tRRQ rejected, duplicate callSignalAddress");
          return FALSE;
        }
      }
    }

    if (rrq.HasOptionalField(H225_RegistrationRequest::e_terminalAlias)) {
      for (i = 0; i < rrq.m_terminalAlias.GetSize(); i++) {
        H323RegisteredEndPoint * ep2 = FindEndPointByAliasAddress(rrq.m_terminalAlias[i]);
        if (ep2 != NULL && ep2 != ep) {
          rrj.m_rejectReason.SetTag(H225_RegistrationRejectReason::e_duplicateAlias);
          ((H225_AliasAddress&)rrj.m_rejectReason) = rrq.m_terminalAlias[i];
          PTRACE(2, "RAS\tRRQ rejected, duplicate alias");
          return FALSE;
        }
      }
    }

    if (ep == NULL) {
      ep = CreateRegisteredEndPoint(rrq);
      if (ep == NULL) {
        rrj.m_rejectReason.SetTag(H225_RegistrationRejectReason::e_undefinedReason);
        PTRACE(1, "RAS\tRRQ rejected, CreateRegisteredEndPoint() returned NULL");
        return FALSE;
      }

      isNew = TRUE;
    }
  }

  rcf.m_preGrantedARQ.m_answerCall = answerCallPreGrantedARQ;
  rcf.m_preGrantedARQ.m_useGKCallSignalAddressToAnswer = answerCallPreGrantedARQ && isGatekeeperRouted;
  rcf.m_preGrantedARQ.m_makeCall = makeCallPreGrantedARQ;
  rcf.m_preGrantedARQ.m_useGKCallSignalAddressToMakeCall = makeCallPreGrantedARQ && isGatekeeperRouted;

  BOOL accepted = ep->OnRegistration(*this, rrq, rcf, rrj);

  if (isNew) {
    if (accepted)
      AddEndPoint(ep);
    else {
      delete ep;
      ep = NULL;
    }
  }

  return accepted;
}


BOOL H323GatekeeperServer::OnUnregistration(H323RegisteredEndPoint * ep,
                                            const H225_UnregistrationRequest & urq,
                                            H225_UnregistrationConfirm & /*ucf*/,
                                            H225_UnregistrationReject & urj)
{
  PINDEX i;

  if (urq.HasOptionalField(H225_UnregistrationRequest::e_endpointAlias)) {
    // See if all aliases to be removed are on the same endpoint
    for (i = 0; i < urq.m_endpointAlias.GetSize(); i++) {
      if (FindEndPointByAliasAddress(urq.m_endpointAlias[i]) != ep) {
        urj.m_rejectReason.SetTag(H225_UnregRejectReason::e_permissionDenied);
        PTRACE(2, "RAS\tURQ rejected, alias " << urq.m_endpointAlias[i]
               << " not owned by registration");
        return FALSE;
      }
    }
    // Remove all the aliases specified in PDU, if no aliases left, then
    // RemoveAlias() will remove the endpoint
    for (i = 0; i < urq.m_endpointAlias.GetSize(); i++)
      RemoveAlias(H323GetAliasAddressString(urq.m_endpointAlias[i]), FALSE);
  }
  else
    RemoveEndPoint(ep, FALSE);

  return TRUE;
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

  mutex.Signal();
}


void H323GatekeeperServer::RemoveEndPoint(H323RegisteredEndPoint * ep, BOOL autoDelete, BOOL lock)
{
  PTRACE(3, "RAS\tRemoving registered endpoint: " << *ep);

  PINDEX i;

  if (lock)
    mutex.Wait();

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
  return byAlias.GetAt(alias);
}


BOOL H323GatekeeperServer::OnAdmission(H323RegisteredEndPoint & ep,
                                       const H225_AdmissionRequest & arq,
                                       H225_AdmissionConfirm & acf,
                                       H225_AdmissionReject & arj)
{
  PINDEX i;

  unsigned bandwidthAllocated = AllocateBandwidth(arq.m_bandWidth);
  if (bandwidthAllocated == 0) {
    arj.m_rejectReason.SetTag(H225_AdmissionRejectReason::e_requestDenied);
    PTRACE(2, "RAS\tARQ rejected, not enough bandwidth");
    return FALSE;
  }
  acf.m_bandWidth = bandwidthAllocated;


  if (arq.m_answerCall) {
    // Incoming call

    BOOL denied = TRUE;
    for (i = 0; i < arq.m_srcInfo.GetSize(); i++) {
      if (CheckAliasAddressPolicy(ep, arq, arq.m_srcInfo[i])) {
        denied = FALSE;
        break;
      }
    }

    if (arq.HasOptionalField(H225_AdmissionRequest::e_srcCallSignalAddress)) {
      if (CheckSignalAddressPolicy(ep, arq, H323TransportAddress(arq.m_srcCallSignalAddress)))
        denied = FALSE;
    }

    if (denied) {
      arj.m_rejectReason.SetTag(H225_AdmissionRejectReason::e_securityDenial);
      PTRACE(2, "RAS\tARQ rejected, not allowed to answer call");
      return FALSE;
    }

    acf.m_destCallSignalAddress = arq.m_destCallSignalAddress;
  }
  else {
    // Outgoing call.

    H323TransportAddress destAddress;
    if (arq.HasOptionalField(H225_AdmissionRequest::e_destCallSignalAddress))
      destAddress = arq.m_destCallSignalAddress;

    H323RegisteredEndPoint * destEP = NULL;

    if (arq.HasOptionalField(H225_AdmissionRequest::e_destinationInfo)) {

      // Have provided an alias, see if we can route it.
      BOOL denied = TRUE;
      BOOL noAddress = TRUE;
      for (i = 0; i < arq.m_destinationInfo.GetSize(); i++) {
        if (CheckAliasAddressPolicy(ep, arq, arq.m_destinationInfo[i])) {
          denied = FALSE;
          if (TranslateAliasAddressToSignalAddress(arq.m_destinationInfo[i], destAddress)) {
            destEP = FindEndPointByAliasAddress(arq.m_destinationInfo[i]);
            noAddress = FALSE;
            break;
          }
        }
      }

      if (denied) {
        arj.m_rejectReason.SetTag(H225_AdmissionRejectReason::e_securityDenial);
        PTRACE(2, "RAS\tARQ rejected, not allowed to make call");
        return FALSE;
      }

      if (noAddress) {
        arj.m_rejectReason.SetTag(H225_AdmissionRejectReason::e_calledPartyNotRegistered);
        PTRACE(2, "RAS\tARQ rejected, destination alias not registered");
        return FALSE;
      }
    }

    // Has provided an alias and signal address, see if agree
    if (destEP != NULL &&
        arq.HasOptionalField(H225_AdmissionRequest::e_destCallSignalAddress) &&
        FindEndPointBySignalAddress(arq.m_destCallSignalAddress) != destEP) {
      arj.m_rejectReason.SetTag(H225_AdmissionRejectReason::e_aliasesInconsistent);
      PTRACE(2, "RAS\tARQ rejected, destination address not for specified alias");
      return FALSE;
    }

    // HAve no address from anywhere
    if (destAddress.IsEmpty()) {
      arj.m_rejectReason.SetTag(H225_AdmissionRejectReason::e_incompleteAddress);
      PTRACE(2, "RAS\tARQ rejected, must have destination address or alias");
      return FALSE;
    }

    if (!CheckSignalAddressPolicy(ep, arq, destAddress)) {
      arj.m_rejectReason.SetTag(H225_AdmissionRejectReason::e_securityDenial);
      PTRACE(2, "RAS\tARQ rejected, not allowed to make call");
      return FALSE;
    }

    destAddress.SetPDU(acf.m_destCallSignalAddress);

    if (isGatekeeperRouted)
      acf.m_callModel.SetTag(H225_CallModel::e_gatekeeperRouted);
  }

  H323GatekeeperCall * call = CreateCall(ep, arq.m_callIdentifier.m_guid);
  if (!call->OnAdmission(*this, arq, acf, arj)) {
    delete call;
    return FALSE;
  }

  AddCall(call);

  return TRUE;
}


BOOL H323GatekeeperServer::OnDisengage(H323RegisteredEndPoint & /*ep*/,
                                       const H225_DisengageRequest & drq,
                                       H225_DisengageConfirm & dcf,
                                       H225_DisengageReject & drj)
{
  H323GatekeeperCall * call = FindCall(drq.m_callIdentifier.m_guid);
  if (call == NULL) {
    drj.m_rejectReason.SetTag(H225_DisengageRejectReason::e_requestToDropOther);
    PTRACE(2, "RAS\tDRQ rejected, no call with ID");
    return FALSE;
  }

  if (!call->OnDisengage(*this, drq, dcf, drj))
    return FALSE;

  RemoveCall(call);
  return TRUE;
}


BOOL H323GatekeeperServer::OnBandwidth(H323RegisteredEndPoint & ep,
                                       const H225_BandwidthRequest & brq,
                                       H225_BandwidthConfirm & bcf,
                                       H225_BandwidthReject & brj)
{
  H323GatekeeperCall * call = FindCall(brq.m_callIdentifier.m_guid);
  if (call == NULL) {
    brj.m_rejectReason.SetTag(H225_BandRejectReason::e_invalidConferenceID);
    PTRACE(2, "RAS\tBRQ rejected, no call with ID");
    return FALSE;
  }

  if (&call->GetEndPoint() != &ep) {
    brj.m_rejectReason.SetTag(H225_BandRejectReason::e_invalidPermission);
    PTRACE(2, "RAS\tBRQ rejected, call is not owned by endpoint");
    return FALSE;
  }

  unsigned newBandwidth = AllocateBandwidth(brq.m_bandWidth, call->GetBandwidthUsed());
  if (newBandwidth == 0) {
    brj.m_rejectReason.SetTag(H225_BandRejectReason::e_insufficientResources);
    PTRACE(2, "RAS\tBRQ rejected, no bandwidth");
    return FALSE;
  }

  bcf.m_bandWidth = newBandwidth;
  return TRUE;
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


BOOL H323GatekeeperServer::OnLocation(const H225_LocationRequest & lrq,
                                      H225_LocationConfirm & lcf,
                                      H225_LocationReject & lrj)
{
  PINDEX i;
  for (i = 0; i < lrq.m_destinationInfo.GetSize(); i++) {
    H323TransportAddress address;
    if (TranslateAliasAddressToSignalAddress(lrq.m_destinationInfo[i], address)) {
      address.SetPDU(lcf.m_callSignalAddress);
      return TRUE;
    }
  }

  lrj.m_rejectReason.SetTag(H225_LocationRejectReason::e_requestDenied);
  PTRACE(2, "RAS\tLRQ rejected, not found");
  return FALSE;
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
