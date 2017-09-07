/*
 * H225_RAS.cxx
 *
 * H.225 RAS protocol handler
 *
 * Open H323 Library
 *
 * Copyright (c) 2001 Equivalence Pty. Ltd.
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
 */

#include <ptlib.h>

#include <opal_config.h>
#if OPAL_H323

#ifdef __GNUC__
#pragma implementation "h225ras.h"
#endif

#include <h323/h225ras.h>

#include <h323/h323ep.h>
#include <h323/transaddr.h>
#include <h323/h323pdu.h>
#include <h323/h235auth.h>
#include <asn/h248.h>
#include <h460/h460.h>
#include <ptclib/random.h>


#define new PNEW


/////////////////////////////////////////////////////////////////////////////

H225_RAS::H225_RAS(H323EndPoint & ep, H323Transport * trans)
  : H323Transactor(ep, trans, DefaultRasUdpPort, DefaultRasUdpPort)
{
}


H225_RAS::~H225_RAS()
{
  StopChannel();
}


void H225_RAS::PrintOn(ostream & strm) const
{
  if (gatekeeperIdentifier.IsEmpty())
    strm << "H225-RAS@";
  else
    strm << gatekeeperIdentifier << '@';
  H323Transactor::PrintOn(strm);
}


H323TransactionPDU * H225_RAS::CreateTransactionPDU() const
{
  return new H323RasPDU;
}


PBoolean H225_RAS::HandleTransaction(const PASN_Object & rawPDU)
{
  const H323RasPDU & pdu = (const H323RasPDU &)rawPDU;

  switch (pdu.GetTag()) {
    case H225_RasMessage::e_gatekeeperRequest :
      if (SendCachedResponse(pdu))
        return false;
      OnReceiveGatekeeperRequest(pdu, pdu);
      break;

    case H225_RasMessage::e_gatekeeperConfirm :
      return OnReceiveGatekeeperConfirm(pdu, pdu);

    case H225_RasMessage::e_gatekeeperReject :
      return OnReceiveGatekeeperReject(pdu, pdu);

    case H225_RasMessage::e_registrationRequest :
      if (SendCachedResponse(pdu))
        return false;
      OnReceiveRegistrationRequest(pdu, pdu);
      break;

    case H225_RasMessage::e_registrationConfirm :
      return OnReceiveRegistrationConfirm(pdu, pdu);

    case H225_RasMessage::e_registrationReject :
      return OnReceiveRegistrationReject(pdu, pdu);

    case H225_RasMessage::e_unregistrationRequest :
      if (SendCachedResponse(pdu))
        return false;
      OnReceiveUnregistrationRequest(pdu, pdu);
      break;

    case H225_RasMessage::e_unregistrationConfirm :
      return OnReceiveUnregistrationConfirm(pdu, pdu);

    case H225_RasMessage::e_unregistrationReject :
      return OnReceiveUnregistrationReject(pdu, pdu);

    case H225_RasMessage::e_admissionRequest :
      if (SendCachedResponse(pdu))
        return false;
      OnReceiveAdmissionRequest(pdu, pdu);
      break;

    case H225_RasMessage::e_admissionConfirm :
      return OnReceiveAdmissionConfirm(pdu, pdu);

    case H225_RasMessage::e_admissionReject :
      return OnReceiveAdmissionReject(pdu, pdu);

    case H225_RasMessage::e_bandwidthRequest :
      if (SendCachedResponse(pdu))
        return false;
      OnReceiveBandwidthRequest(pdu, pdu);
      break;

    case H225_RasMessage::e_bandwidthConfirm :
      return OnReceiveBandwidthConfirm(pdu, pdu);

    case H225_RasMessage::e_bandwidthReject :
      return OnReceiveBandwidthReject(pdu, pdu);

    case H225_RasMessage::e_disengageRequest :
      if (SendCachedResponse(pdu))
        return false;
      OnReceiveDisengageRequest(pdu, pdu);
      break;

    case H225_RasMessage::e_disengageConfirm :
      return OnReceiveDisengageConfirm(pdu, pdu);

    case H225_RasMessage::e_disengageReject :
      return OnReceiveDisengageReject(pdu, pdu);

    case H225_RasMessage::e_locationRequest :
      if (SendCachedResponse(pdu))
        return false;
      OnReceiveLocationRequest(pdu, pdu);
      break;

    case H225_RasMessage::e_locationConfirm :
      return OnReceiveLocationConfirm(pdu, pdu);

    case H225_RasMessage::e_locationReject :
      return OnReceiveLocationReject(pdu, pdu);

    case H225_RasMessage::e_infoRequest :
      if (SendCachedResponse(pdu))
        return false;
      OnReceiveInfoRequest(pdu, pdu);
      break;

    case H225_RasMessage::e_infoRequestResponse :
      return OnReceiveInfoRequestResponse(pdu, pdu);

    case H225_RasMessage::e_nonStandardMessage :
      OnReceiveNonStandardMessage(pdu, pdu);
      break;

    case H225_RasMessage::e_unknownMessageResponse :
      OnReceiveUnknownMessageResponse(pdu, pdu);
      break;

    case H225_RasMessage::e_requestInProgress :
      return OnReceiveRequestInProgress(pdu, pdu);

    case H225_RasMessage::e_resourcesAvailableIndicate :
      if (SendCachedResponse(pdu))
        return false;
      OnReceiveResourcesAvailableIndicate(pdu, pdu);
      break;

    case H225_RasMessage::e_resourcesAvailableConfirm :
      return OnReceiveResourcesAvailableConfirm(pdu, pdu);

    case H225_RasMessage::e_infoRequestAck :
      return OnReceiveInfoRequestAck(pdu, pdu);

    case H225_RasMessage::e_infoRequestNak :
      return OnReceiveInfoRequestNak(pdu, pdu);

    case H225_RasMessage::e_serviceControlIndication :
      if (SendCachedResponse(pdu))
        return false;
      OnReceiveServiceControlIndication(pdu, pdu);
      break;

    case H225_RasMessage::e_serviceControlResponse :
      return OnReceiveServiceControlResponse(pdu, pdu);

    default :
      OnReceiveUnknown(pdu);
  }

  return false;
}


void H225_RAS::OnSendingPDU(PASN_Object & rawPDU)
{
  H323RasPDU & pdu = (H323RasPDU &)rawPDU;

  switch (pdu.GetTag()) {
    case H225_RasMessage::e_gatekeeperRequest :
      OnSendGatekeeperRequest(pdu, pdu);
      break;

    case H225_RasMessage::e_gatekeeperConfirm :
      OnSendGatekeeperConfirm(pdu, pdu);
      break;

    case H225_RasMessage::e_gatekeeperReject :
      OnSendGatekeeperReject(pdu, pdu);
      break;

    case H225_RasMessage::e_registrationRequest :
      OnSendRegistrationRequest(pdu, pdu);
      break;

    case H225_RasMessage::e_registrationConfirm :
      OnSendRegistrationConfirm(pdu, pdu);
      break;

    case H225_RasMessage::e_registrationReject :
      OnSendRegistrationReject(pdu, pdu);
      break;

    case H225_RasMessage::e_unregistrationRequest :
      OnSendUnregistrationRequest(pdu, pdu);
      break;

    case H225_RasMessage::e_unregistrationConfirm :
      OnSendUnregistrationConfirm(pdu, pdu);
      break;

    case H225_RasMessage::e_unregistrationReject :
      OnSendUnregistrationReject(pdu, pdu);
      break;

    case H225_RasMessage::e_admissionRequest :
      OnSendAdmissionRequest(pdu, pdu);
      break;

    case H225_RasMessage::e_admissionConfirm :
      OnSendAdmissionConfirm(pdu, pdu);
      break;

    case H225_RasMessage::e_admissionReject :
      OnSendAdmissionReject(pdu, pdu);
      break;

    case H225_RasMessage::e_bandwidthRequest :
      OnSendBandwidthRequest(pdu, pdu);
      break;

    case H225_RasMessage::e_bandwidthConfirm :
      OnSendBandwidthConfirm(pdu, pdu);
      break;

    case H225_RasMessage::e_bandwidthReject :
      OnSendBandwidthReject(pdu, pdu);
      break;

    case H225_RasMessage::e_disengageRequest :
      OnSendDisengageRequest(pdu, pdu);
      break;

    case H225_RasMessage::e_disengageConfirm :
      OnSendDisengageConfirm(pdu, pdu);
      break;

    case H225_RasMessage::e_disengageReject :
      OnSendDisengageReject(pdu, pdu);
      break;

    case H225_RasMessage::e_locationRequest :
      OnSendLocationRequest(pdu, pdu);
      break;

    case H225_RasMessage::e_locationConfirm :
      OnSendLocationConfirm(pdu, pdu);
      break;

    case H225_RasMessage::e_locationReject :
      OnSendLocationReject(pdu, pdu);
      break;

    case H225_RasMessage::e_infoRequest :
      OnSendInfoRequest(pdu, pdu);
      break;

    case H225_RasMessage::e_infoRequestResponse :
      OnSendInfoRequestResponse(pdu, pdu);
      break;

    case H225_RasMessage::e_nonStandardMessage :
      OnSendNonStandardMessage(pdu, pdu);
      break;

    case H225_RasMessage::e_unknownMessageResponse :
      OnSendUnknownMessageResponse(pdu, pdu);
      break;

    case H225_RasMessage::e_requestInProgress :
      OnSendRequestInProgress(pdu, pdu);
      break;

    case H225_RasMessage::e_resourcesAvailableIndicate :
      OnSendResourcesAvailableIndicate(pdu, pdu);
      break;

    case H225_RasMessage::e_resourcesAvailableConfirm :
      OnSendResourcesAvailableConfirm(pdu, pdu);
      break;

    case H225_RasMessage::e_infoRequestAck :
      OnSendInfoRequestAck(pdu, pdu);
      break;

    case H225_RasMessage::e_infoRequestNak :
      OnSendInfoRequestNak(pdu, pdu);
      break;

    case H225_RasMessage::e_serviceControlIndication :
      OnSendServiceControlIndication(pdu, pdu);
      break;

    case H225_RasMessage::e_serviceControlResponse :
      OnSendServiceControlResponse(pdu, pdu);
      break;

    default :
      break;
  }
}


PBoolean H225_RAS::OnReceiveRequestInProgress(const H323RasPDU & pdu, const H225_RequestInProgress & rip)
{
  if (!HandleRequestInProgress(pdu, rip.m_delay))
    return false;

  return OnReceiveRequestInProgress(rip);
}


PBoolean H225_RAS::OnReceiveRequestInProgress(const H225_RequestInProgress & /*rip*/)
{
  return true;
}


#if OPAL_H460
/// For some completely silly reason the ITU decided to send/receive H460 Messages in two places,
/// for some messages it is included in the messsage body FeatureSet (to Advertise it) and others 
/// in the genericData field (as actual information). Even though a Feature is generic data. It is beyond
/// me to determine Why it is so. Anyway if a message is to be carried in the genericData field it is given
/// the default category of supported.

template <typename PDUType>
static void SendGenericData(const H225_RAS * ras, H460_MessageType pduType, PDUType & pdu)
{
  H225_FeatureSet fs;
  if (ras->OnSendFeatureSet(pduType, fs) && H460_FeatureSet::Copy(pdu.m_genericData, fs))
    pdu.IncludeOptionalField(PDUType::e_genericData);
}


template <typename PDUType>
static void SendFeatureSet(const H225_RAS * ras, H460_MessageType pduType, PDUType & pdu)
{
  if (ras->OnSendFeatureSet(pduType, pdu.m_featureSet))
    pdu.IncludeOptionalField(PDUType::e_featureSet);
}


template <typename PDUType>
static void ReceiveGenericData(const H225_RAS * ras, H460_MessageType pduType, const PDUType & pdu)
{
  if (!pdu.HasOptionalField(PDUType::e_genericData))
    return;

  H225_FeatureSet fs;
  if (H460_FeatureSet::Copy(fs, pdu.m_genericData))
    ras->OnReceiveFeatureSet(pduType, fs);
}


template <typename PDUType>
static void ReceiveFeatureSet(const H225_RAS * ras, H460_MessageType pduType, const PDUType & pdu)
{
  if (pdu.HasOptionalField(PDUType::e_featureSet))
    ras->OnReceiveFeatureSet(pduType, pdu.m_featureSet);
}
#endif // OPAL_H460


void H225_RAS::OnSendGatekeeperRequest(H323RasPDU &, H225_GatekeeperRequest & grq)
{
  // This function is never called during sending GRQ
  if (!gatekeeperIdentifier.IsEmpty()) {
    grq.IncludeOptionalField(H225_GatekeeperRequest::e_gatekeeperIdentifier);
    grq.m_gatekeeperIdentifier = gatekeeperIdentifier;
  }

  OnSendGatekeeperRequest(grq);
}


void H225_RAS::OnSendGatekeeperRequest(H225_GatekeeperRequest & grq)
{
#if OPAL_H460
  SendFeatureSet(this, H460_MessageType::e_gatekeeperRequest, grq);
#endif
}


void H225_RAS::OnSendGatekeeperConfirm(H323RasPDU &, H225_GatekeeperConfirm & gcf)
{
  if (!gatekeeperIdentifier.IsEmpty()) {
    gcf.IncludeOptionalField(H225_GatekeeperConfirm::e_gatekeeperIdentifier);
    gcf.m_gatekeeperIdentifier = gatekeeperIdentifier;
  }

#if OPAL_H460
  SendFeatureSet(this, H460_MessageType::e_gatekeeperConfirm, gcf);
#endif

  OnSendGatekeeperConfirm(gcf);
}


void H225_RAS::OnSendGatekeeperConfirm(H225_GatekeeperConfirm & /*gcf*/)
{
}


void H225_RAS::OnSendGatekeeperReject(H323RasPDU &, H225_GatekeeperReject & grj)
{
  if (!gatekeeperIdentifier.IsEmpty()) {
    grj.IncludeOptionalField(H225_GatekeeperReject::e_gatekeeperIdentifier);
    grj.m_gatekeeperIdentifier = gatekeeperIdentifier;
  }
    
#if OPAL_H460
  SendFeatureSet(this, H460_MessageType::e_gatekeeperReject, grj);
#endif

  OnSendGatekeeperReject(grj);
}


void H225_RAS::OnSendGatekeeperReject(H225_GatekeeperReject & /*grj*/)
{
}


PBoolean H225_RAS::OnReceiveGatekeeperRequest(const H323RasPDU &, const H225_GatekeeperRequest & grq)
{
#if OPAL_H460
  ReceiveFeatureSet(this, H460_MessageType::e_gatekeeperRequest, grq);
#endif

  return OnReceiveGatekeeperRequest(grq);
}


PBoolean H225_RAS::OnReceiveGatekeeperRequest(const H225_GatekeeperRequest &)
{
  return true;
}


PBoolean H225_RAS::OnReceiveGatekeeperConfirm(const H323RasPDU &, const H225_GatekeeperConfirm & gcf)
{
  if (!CheckForResponse(H225_RasMessage::e_gatekeeperRequest, gcf.m_requestSeqNum))
    return false;

  if (gatekeeperIdentifier.IsEmpty())
    gatekeeperIdentifier = gcf.m_gatekeeperIdentifier;
  else {
    PString gkid = gcf.m_gatekeeperIdentifier;
    if (gatekeeperIdentifier *= gkid)
      gatekeeperIdentifier = gkid;
    else {
      PTRACE(2, "RAS\tReceived a GCF from " << gkid
             << " but wanted it from " << gatekeeperIdentifier);
      return false;
    }
  }

#if OPAL_H460
  if (gcf.HasOptionalField(H225_GatekeeperConfirm::e_featureSet))
    ReceiveFeatureSet(this, H460_MessageType::e_gatekeeperConfirm, gcf);
  else
    DisableFeatureSet();
#endif

  return OnReceiveGatekeeperConfirm(gcf);
}


PBoolean H225_RAS::OnReceiveGatekeeperConfirm(const H225_GatekeeperConfirm & /*gcf*/)
{
  return true;
}


PBoolean H225_RAS::OnReceiveGatekeeperReject(const H323RasPDU &, const H225_GatekeeperReject & grj)
{
  if (!CheckForResponse(H225_RasMessage::e_gatekeeperRequest, grj.m_requestSeqNum, &grj.m_rejectReason))
    return false;
    
#if OPAL_H460
  ReceiveFeatureSet(this, H460_MessageType::e_gatekeeperReject, grj);
#endif

  return OnReceiveGatekeeperReject(grj);
}


PBoolean H225_RAS::OnReceiveGatekeeperReject(const H225_GatekeeperReject & /*grj*/)
{
  return true;
}


void H225_RAS::OnSendRegistrationRequest(H323RasPDU & pdu, H225_RegistrationRequest & rrq)
{
  OnSendRegistrationRequest(rrq);
    
#if OPAL_H460
  SendFeatureSet(this,
                 rrq.m_keepAlive.GetValue() ? H460_MessageType::e_lightweightRegistrationRequest
                                            : H460_MessageType::e_registrationRequest,
                 rrq);
#endif
    
  pdu.Prepare(rrq);
}


void H225_RAS::OnSendRegistrationRequest(H225_RegistrationRequest & /*rrq*/)
{
}


void H225_RAS::OnSendRegistrationConfirm(H323RasPDU & pdu, H225_RegistrationConfirm & rcf)
{
  if (!gatekeeperIdentifier.IsEmpty()) {
    rcf.IncludeOptionalField(H225_RegistrationConfirm::e_gatekeeperIdentifier);
    rcf.m_gatekeeperIdentifier = gatekeeperIdentifier;
  }

  OnSendRegistrationConfirm(rcf);

#if OPAL_H460
  SendFeatureSet(this, H460_MessageType::e_registrationConfirm, rcf);
#endif

  pdu.Prepare(rcf);
}


void H225_RAS::OnSendRegistrationConfirm(H225_RegistrationConfirm & /*rcf*/)
{
}


void H225_RAS::OnSendRegistrationReject(H323RasPDU & pdu, H225_RegistrationReject & rrj)
{
  if (!gatekeeperIdentifier.IsEmpty()) {
    rrj.IncludeOptionalField(H225_RegistrationReject::e_gatekeeperIdentifier);
    rrj.m_gatekeeperIdentifier = gatekeeperIdentifier;
  }

  OnSendRegistrationReject(rrj);
    
#if OPAL_H460
  SendFeatureSet(this, H460_MessageType::e_registrationReject, rrj);
#endif

  pdu.Prepare(rrj);
}


void H225_RAS::OnSendRegistrationReject(H225_RegistrationReject & /*rrj*/)
{
}


PBoolean H225_RAS::OnReceiveRegistrationRequest(const H323RasPDU &, const H225_RegistrationRequest & rrq)
{
#if OPAL_H460
  ReceiveFeatureSet(this, H460_MessageType::e_registrationRequest, rrq);
#endif
    
  return OnReceiveRegistrationRequest(rrq);
}


PBoolean H225_RAS::OnReceiveRegistrationRequest(const H225_RegistrationRequest &)
{
  return true;
}


PBoolean H225_RAS::OnReceiveRegistrationConfirm(const H323RasPDU & pdu, const H225_RegistrationConfirm & rcf)
{
  if (!CheckForResponse(H225_RasMessage::e_registrationRequest, rcf.m_requestSeqNum))
    return false;

  if (m_lastRequest != NULL) {
    PString endpointIdentifier = rcf.m_endpointIdentifier;
    H235Authenticators & authenticators = m_lastRequest->m_requestPDU.GetAuthenticators();
    for (H235Authenticators::iterator it = authenticators.begin(); it != authenticators.end(); ++it) {
      if (it->UseGkAndEpIdentifiers())
        it->SetLocalId(endpointIdentifier);
    }
  }

  if (!CheckCryptoTokens(pdu, rcf))
    return false;

#if OPAL_H460
  if (rcf.HasOptionalField(H225_RegistrationConfirm::e_featureSet))
    ReceiveFeatureSet(this, H460_MessageType::e_registrationConfirm, rcf);
  else
    DisableFeatureSet();
#endif

  if (rcf.HasOptionalField(H225_RegistrationConfirm::e_gatekeeperIdentifier))
    gatekeeperIdentifier = rcf.m_gatekeeperIdentifier;
  return OnReceiveRegistrationConfirm(rcf);
}


PBoolean H225_RAS::OnReceiveRegistrationConfirm(const H225_RegistrationConfirm & /*rcf*/)
{
  return true;
}


PBoolean H225_RAS::OnReceiveRegistrationReject(const H323RasPDU & pdu, const H225_RegistrationReject & rrj)
{
  if (!CheckForResponse(H225_RasMessage::e_registrationRequest, rrj.m_requestSeqNum, &rrj.m_rejectReason))
    return false;

  if (!CheckCryptoTokens(pdu, rrj))
    return false;
  
#if OPAL_H460
  ReceiveFeatureSet(this, H460_MessageType::e_registrationReject, rrj);
#endif

  return OnReceiveRegistrationReject(rrj);
}


PBoolean H225_RAS::OnReceiveRegistrationReject(const H225_RegistrationReject & /*rrj*/)
{
  return true;
}


void H225_RAS::OnSendUnregistrationRequest(H323RasPDU & pdu, H225_UnregistrationRequest & urq)
{
  OnSendUnregistrationRequest(urq);
  pdu.Prepare(urq);
}


void H225_RAS::OnSendUnregistrationRequest(H225_UnregistrationRequest & /*urq*/)
{
}


void H225_RAS::OnSendUnregistrationConfirm(H323RasPDU & pdu, H225_UnregistrationConfirm & ucf)
{
  OnSendUnregistrationConfirm(ucf);
  pdu.Prepare(ucf);
}


void H225_RAS::OnSendUnregistrationConfirm(H225_UnregistrationConfirm & /*ucf*/)
{
}


void H225_RAS::OnSendUnregistrationReject(H323RasPDU & pdu, H225_UnregistrationReject & urj)
{
  OnSendUnregistrationReject(urj);
  pdu.Prepare(urj);
}


void H225_RAS::OnSendUnregistrationReject(H225_UnregistrationReject & /*urj*/)
{
}


PBoolean H225_RAS::OnReceiveUnregistrationRequest(const H323RasPDU & pdu, const H225_UnregistrationRequest & urq)
{
  if (!CheckCryptoTokens(pdu, urq))
    return false;

#if OPAL_H460
  ReceiveGenericData(this, H460_MessageType::e_unregistrationRequest, urq);
#endif

  return OnReceiveUnregistrationRequest(urq);
}


PBoolean H225_RAS::OnReceiveUnregistrationRequest(const H225_UnregistrationRequest & /*urq*/)
{
  return true;
}


PBoolean H225_RAS::OnReceiveUnregistrationConfirm(const H323RasPDU & pdu, const H225_UnregistrationConfirm & ucf)
{
  if (!CheckForResponse(H225_RasMessage::e_unregistrationRequest, ucf.m_requestSeqNum))
    return false;

  if (!CheckCryptoTokens(pdu, ucf))
    return false;

  return OnReceiveUnregistrationConfirm(ucf);
}


PBoolean H225_RAS::OnReceiveUnregistrationConfirm(const H225_UnregistrationConfirm & /*ucf*/)
{
  return true;
}


PBoolean H225_RAS::OnReceiveUnregistrationReject(const H323RasPDU & pdu, const H225_UnregistrationReject & urj)
{
  if (!CheckForResponse(H225_RasMessage::e_unregistrationRequest, urj.m_requestSeqNum, &urj.m_rejectReason))
    return false;

  if (!CheckCryptoTokens(pdu, urj))
    return false;

  return OnReceiveUnregistrationReject(urj);
}


PBoolean H225_RAS::OnReceiveUnregistrationReject(const H225_UnregistrationReject & /*urj*/)
{
  return true;
}


void H225_RAS::OnSendAdmissionRequest(H323RasPDU & pdu, H225_AdmissionRequest & arq)
{
  OnSendAdmissionRequest(arq);
    
#if OPAL_H460
  SendGenericData(this, H460_MessageType::e_admissionRequest, arq);
#endif
    
  pdu.Prepare(arq);
}


void H225_RAS::OnSendAdmissionRequest(H225_AdmissionRequest & /*arq*/)
{
}


void H225_RAS::OnSendAdmissionConfirm(H323RasPDU & pdu, H225_AdmissionConfirm & acf)
{
  OnSendAdmissionConfirm(acf);
    
#if OPAL_H460
  SendGenericData(this, H460_MessageType::e_admissionConfirm, acf);
#endif
    
  pdu.Prepare(acf);
}


void H225_RAS::OnSendAdmissionConfirm(H225_AdmissionConfirm & /*acf*/)
{
}


void H225_RAS::OnSendAdmissionReject(H323RasPDU & pdu, H225_AdmissionReject & arj)
{
  OnSendAdmissionReject(arj);
    
#if OPAL_H460
    SendGenericData(this, H460_MessageType::e_admissionReject, arj);
#endif
    
  pdu.Prepare(arj);
}


void H225_RAS::OnSendAdmissionReject(H225_AdmissionReject & /*arj*/)
{
}


PBoolean H225_RAS::OnReceiveAdmissionRequest(const H323RasPDU & pdu, const H225_AdmissionRequest & arq)
{
  if (!CheckCryptoTokens(pdu, arq))
    return false;
    
#if OPAL_H460
  ReceiveGenericData(this, H460_MessageType::e_admissionRequest, arq);
#endif

  return OnReceiveAdmissionRequest(arq);
}


PBoolean H225_RAS::OnReceiveAdmissionRequest(const H225_AdmissionRequest & /*arq*/)
{
  return true;
}


PBoolean H225_RAS::OnReceiveAdmissionConfirm(const H323RasPDU & pdu, const H225_AdmissionConfirm & acf)
{
  if (!CheckForResponse(H225_RasMessage::e_admissionRequest, acf.m_requestSeqNum))
    return false;

  if (!CheckCryptoTokens(pdu, acf))
    return false;
  
#if OPAL_H460
  ReceiveGenericData(this, H460_MessageType::e_admissionConfirm, acf);
#endif

  return OnReceiveAdmissionConfirm(acf);
}


PBoolean H225_RAS::OnReceiveAdmissionConfirm(const H225_AdmissionConfirm & /*acf*/)
{
  return true;
}


PBoolean H225_RAS::OnReceiveAdmissionReject(const H323RasPDU & pdu, const H225_AdmissionReject & arj)
{
  if (!CheckForResponse(H225_RasMessage::e_admissionRequest, arj.m_requestSeqNum, &arj.m_rejectReason))
    return false;

  if (!CheckCryptoTokens(pdu, arj))
    return false;
  
#if OPAL_H460
  ReceiveGenericData(this, H460_MessageType::e_admissionReject, arj);
#endif

  return OnReceiveAdmissionReject(arj);
}


PBoolean H225_RAS::OnReceiveAdmissionReject(const H225_AdmissionReject & /*arj*/)
{
  return true;
}


void H225_RAS::OnSendBandwidthRequest(H323RasPDU & pdu, H225_BandwidthRequest & brq)
{
  OnSendBandwidthRequest(brq);
  pdu.Prepare(brq);
}


void H225_RAS::OnSendBandwidthRequest(H225_BandwidthRequest & /*brq*/)
{
}


PBoolean H225_RAS::OnReceiveBandwidthRequest(const H323RasPDU & pdu, const H225_BandwidthRequest & brq)
{
  if (!CheckCryptoTokens(pdu, brq))
    return false;

  return OnReceiveBandwidthRequest(brq);
}


PBoolean H225_RAS::OnReceiveBandwidthRequest(const H225_BandwidthRequest & /*brq*/)
{
  return true;
}


void H225_RAS::OnSendBandwidthConfirm(H323RasPDU & pdu, H225_BandwidthConfirm & bcf)
{
  OnSendBandwidthConfirm(bcf);
  pdu.Prepare(bcf);
}


void H225_RAS::OnSendBandwidthConfirm(H225_BandwidthConfirm & /*bcf*/)
{
}


PBoolean H225_RAS::OnReceiveBandwidthConfirm(const H323RasPDU & pdu, const H225_BandwidthConfirm & bcf)
{
  if (!CheckForResponse(H225_RasMessage::e_bandwidthRequest, bcf.m_requestSeqNum))
    return false;

  if (!CheckCryptoTokens(pdu, bcf))
    return false;

  return OnReceiveBandwidthConfirm(bcf);
}


PBoolean H225_RAS::OnReceiveBandwidthConfirm(const H225_BandwidthConfirm & /*bcf*/)
{
  return true;
}


void H225_RAS::OnSendBandwidthReject(H323RasPDU & pdu, H225_BandwidthReject & brj)
{
  OnSendBandwidthReject(brj);
  pdu.Prepare(brj);
}


void H225_RAS::OnSendBandwidthReject(H225_BandwidthReject & /*brj*/)
{
}


PBoolean H225_RAS::OnReceiveBandwidthReject(const H323RasPDU & pdu, const H225_BandwidthReject & brj)
{
  if (!CheckForResponse(H225_RasMessage::e_bandwidthRequest, brj.m_requestSeqNum, &brj.m_rejectReason))
    return false;

  if (!CheckCryptoTokens(pdu, brj))
    return false;

  return OnReceiveBandwidthReject(brj);
}


PBoolean H225_RAS::OnReceiveBandwidthReject(const H225_BandwidthReject & /*brj*/)
{
  return true;
}


void H225_RAS::OnSendDisengageRequest(H323RasPDU & pdu, H225_DisengageRequest & drq)
{
  OnSendDisengageRequest(drq);
  pdu.Prepare(drq);

#if OPAL_H460
  SendGenericData(this, H460_MessageType::e_disengagerequest, drq);
#endif
}


void H225_RAS::OnSendDisengageRequest(H225_DisengageRequest & /*drq*/)
{
}


PBoolean H225_RAS::OnReceiveDisengageRequest(const H323RasPDU & pdu, const H225_DisengageRequest & drq)
{
  if (!CheckCryptoTokens(pdu, drq))
    return false;

#if OPAL_H460
  ReceiveGenericData(this, H460_MessageType::e_disengagerequest, drq);
#endif

  return OnReceiveDisengageRequest(drq);
}


PBoolean H225_RAS::OnReceiveDisengageRequest(const H225_DisengageRequest & /*drq*/)
{
  return true;
}


void H225_RAS::OnSendDisengageConfirm(H323RasPDU & pdu, H225_DisengageConfirm & dcf)
{
  OnSendDisengageConfirm(dcf);
  pdu.Prepare(dcf);

#if OPAL_H460
  SendGenericData(this, H460_MessageType::e_disengageconfirm, dcf);
#endif
}


void H225_RAS::OnSendDisengageConfirm(H225_DisengageConfirm & /*dcf*/)
{
}


PBoolean H225_RAS::OnReceiveDisengageConfirm(const H323RasPDU & pdu, const H225_DisengageConfirm & dcf)
{
  if (!CheckForResponse(H225_RasMessage::e_disengageRequest, dcf.m_requestSeqNum))
    return false;

  if (!CheckCryptoTokens(pdu, dcf))
    return false;

#if OPAL_H460
  ReceiveGenericData(this, H460_MessageType::e_disengageconfirm, dcf);
#endif

  return OnReceiveDisengageConfirm(dcf);
}


PBoolean H225_RAS::OnReceiveDisengageConfirm(const H225_DisengageConfirm & /*dcf*/)
{
  return true;
}


void H225_RAS::OnSendDisengageReject(H323RasPDU & pdu, H225_DisengageReject & drj)
{
  OnSendDisengageReject(drj);
  pdu.Prepare(drj);
}


void H225_RAS::OnSendDisengageReject(H225_DisengageReject & /*drj*/)
{
}


PBoolean H225_RAS::OnReceiveDisengageReject(const H323RasPDU & pdu, const H225_DisengageReject & drj)
{
  if (!CheckForResponse(H225_RasMessage::e_disengageRequest, drj.m_requestSeqNum, &drj.m_rejectReason))
    return false;

  if (!CheckCryptoTokens(pdu, drj))
    return false;

  return OnReceiveDisengageReject(drj);
}


PBoolean H225_RAS::OnReceiveDisengageReject(const H225_DisengageReject & /*drj*/)
{
  return true;
}


void H225_RAS::OnSendLocationRequest(H323RasPDU & pdu, H225_LocationRequest & lrq)
{
  OnSendLocationRequest(lrq);
    
#if OPAL_H460
  SendGenericData(this, H460_MessageType::e_locationRequest, lrq);
#endif
    
  pdu.Prepare(lrq);
}


void H225_RAS::OnSendLocationRequest(H225_LocationRequest & /*lrq*/)
{
}


PBoolean H225_RAS::OnReceiveLocationRequest(const H323RasPDU & pdu, const H225_LocationRequest & lrq)
{
  if (!CheckCryptoTokens(pdu, lrq))
    return false;
    
#if OPAL_H460
  ReceiveGenericData(this, H460_MessageType::e_locationRequest, lrq);
#endif

  return OnReceiveLocationRequest(lrq);
}


PBoolean H225_RAS::OnReceiveLocationRequest(const H225_LocationRequest & /*lrq*/)
{
  return true;
}


void H225_RAS::OnSendLocationConfirm(H323RasPDU & pdu, H225_LocationConfirm & lcf)
{
  OnSendLocationConfirm(lcf);
    
#if OPAL_H460
  SendGenericData(this, H460_MessageType::e_locationConfirm, lcf);
#endif
    
  pdu.Prepare(lcf);
}


void H225_RAS::OnSendLocationConfirm(H225_LocationConfirm & /*lcf*/)
{
}


PBoolean H225_RAS::OnReceiveLocationConfirm(const H323RasPDU &, const H225_LocationConfirm & lcf)
{
  if (!CheckForResponse(H225_RasMessage::e_locationRequest, lcf.m_requestSeqNum))
    return false;

  if (m_lastRequest->m_responseInfo != NULL) {
    H323TransportAddress & locatedAddress = dynamic_cast<H323TransportAddress &>(*m_lastRequest->m_responseInfo);
    locatedAddress = lcf.m_callSignalAddress;
  }
  
#if OPAL_H460
  ReceiveGenericData(this, H460_MessageType::e_locationConfirm, lcf);
#endif

  return OnReceiveLocationConfirm(lcf);
}


PBoolean H225_RAS::OnReceiveLocationConfirm(const H225_LocationConfirm & /*lcf*/)
{
  return true;
}


void H225_RAS::OnSendLocationReject(H323RasPDU & pdu, H225_LocationReject & lrj)
{
  OnSendLocationReject(lrj);
    
#if OPAL_H460
  SendGenericData(this, H460_MessageType::e_locationReject, lrj);
#endif
    
  pdu.Prepare(lrj);
}


void H225_RAS::OnSendLocationReject(H225_LocationReject & /*lrj*/)
{
}


PBoolean H225_RAS::OnReceiveLocationReject(const H323RasPDU & pdu, const H225_LocationReject & lrj)
{
  if (!CheckForResponse(H225_RasMessage::e_locationRequest, lrj.m_requestSeqNum, &lrj.m_rejectReason))
    return false;

  if (!CheckCryptoTokens(pdu, lrj))
    return false;
  
#if OPAL_H460
  ReceiveGenericData(this, H460_MessageType::e_locationReject, lrj);
#endif

  return OnReceiveLocationReject(lrj);
}


PBoolean H225_RAS::OnReceiveLocationReject(const H225_LocationReject & /*lrj*/)
{
  return true;
}


void H225_RAS::OnSendInfoRequest(H323RasPDU & pdu, H225_InfoRequest & irq)
{
  OnSendInfoRequest(irq);
  pdu.Prepare(irq);

#if OPAL_H460
  SendGenericData(this, H460_MessageType::e_inforequest, irq);
#endif
}


void H225_RAS::OnSendInfoRequest(H225_InfoRequest & /*irq*/)
{
}


PBoolean H225_RAS::OnReceiveInfoRequest(const H323RasPDU & pdu, const H225_InfoRequest & irq)
{
  if (!CheckCryptoTokens(pdu, irq))
    return false;

#if OPAL_H460
  ReceiveGenericData(this, H460_MessageType::e_inforequest, irq);
#endif

  return OnReceiveInfoRequest(irq);
}


PBoolean H225_RAS::OnReceiveInfoRequest(const H225_InfoRequest & /*irq*/)
{
  return true;
}


void H225_RAS::OnSendInfoRequestResponse(H323RasPDU & pdu, H225_InfoRequestResponse & irr)
{
  OnSendInfoRequestResponse(irr);
  pdu.Prepare(irr);

#if OPAL_H460
  SendGenericData(this, H460_MessageType::e_inforequestresponse, irr);
#endif
}


void H225_RAS::OnSendInfoRequestResponse(H225_InfoRequestResponse & /*irr*/)
{
}


PBoolean H225_RAS::OnReceiveInfoRequestResponse(const H323RasPDU & pdu, const H225_InfoRequestResponse & irr)
{
  if (!CheckForResponse(H225_RasMessage::e_infoRequest, irr.m_requestSeqNum))
    return false;

  if (!CheckCryptoTokens(pdu, irr))
    return false;

#if OPAL_H460
  ReceiveGenericData(this, H460_MessageType::e_inforequestresponse, irr);
#endif

  return OnReceiveInfoRequestResponse(irr);
}


PBoolean H225_RAS::OnReceiveInfoRequestResponse(const H225_InfoRequestResponse & /*irr*/)
{
  return true;
}


void H225_RAS::OnSendNonStandardMessage(H323RasPDU & pdu, H225_NonStandardMessage & nsm)
{
  OnSendNonStandardMessage(nsm);
    
#if OPAL_H460
  SendGenericData(this, H460_MessageType::e_nonStandardMessage, nsm);
#endif
    
  pdu.Prepare(nsm);
}


void H225_RAS::OnSendNonStandardMessage(H225_NonStandardMessage & /*nsm*/)
{
}


PBoolean H225_RAS::OnReceiveNonStandardMessage(const H323RasPDU & pdu, const H225_NonStandardMessage & nsm)
{
  CheckForResponse(H225_RasMessage::e_nonStandardMessage, nsm.m_requestSeqNum);

  if (!CheckCryptoTokens(pdu, nsm))
    return false;

#if OPAL_H460
  ReceiveGenericData(this, H460_MessageType::e_nonStandardMessage, nsm);
#endif

  return OnReceiveNonStandardMessage(nsm);
}


PBoolean H225_RAS::OnReceiveNonStandardMessage(const H225_NonStandardMessage & /*nsm*/)
{
  return true;
}


void H225_RAS::OnSendUnknownMessageResponse(H323RasPDU & pdu, H225_UnknownMessageResponse & umr)
{
  OnSendUnknownMessageResponse(umr);
  pdu.Prepare(umr);
}


void H225_RAS::OnSendUnknownMessageResponse(H225_UnknownMessageResponse & /*umr*/)
{
}


PBoolean H225_RAS::OnReceiveUnknownMessageResponse(const H323RasPDU & pdu, const H225_UnknownMessageResponse & umr)
{
  if (!CheckCryptoTokens(pdu, umr))
    return false;

  return OnReceiveUnknownMessageResponse(umr);
}


PBoolean H225_RAS::OnReceiveUnknownMessageResponse(const H225_UnknownMessageResponse & /*umr*/)
{
  return true;
}


void H225_RAS::OnSendRequestInProgress(H323RasPDU & pdu, H225_RequestInProgress & rip)
{
  OnSendRequestInProgress(rip);
  pdu.Prepare(rip);
}


void H225_RAS::OnSendRequestInProgress(H225_RequestInProgress & /*rip*/)
{
}


void H225_RAS::OnSendResourcesAvailableIndicate(H323RasPDU & pdu, H225_ResourcesAvailableIndicate & rai)
{
  OnSendResourcesAvailableIndicate(rai);
  pdu.Prepare(rai);
}


void H225_RAS::OnSendResourcesAvailableIndicate(H225_ResourcesAvailableIndicate & /*rai*/)
{
}


PBoolean H225_RAS::OnReceiveResourcesAvailableIndicate(const H323RasPDU & pdu, const H225_ResourcesAvailableIndicate & rai)
{
  if (!CheckCryptoTokens(pdu, rai))
    return false;

  return OnReceiveResourcesAvailableIndicate(rai);
}


PBoolean H225_RAS::OnReceiveResourcesAvailableIndicate(const H225_ResourcesAvailableIndicate & /*rai*/)
{
  return true;
}


void H225_RAS::OnSendResourcesAvailableConfirm(H323RasPDU & pdu, H225_ResourcesAvailableConfirm & rac)
{
  OnSendResourcesAvailableConfirm(rac);
  pdu.Prepare(rac);
}


void H225_RAS::OnSendResourcesAvailableConfirm(H225_ResourcesAvailableConfirm & /*rac*/)
{
}


PBoolean H225_RAS::OnReceiveResourcesAvailableConfirm(const H323RasPDU & pdu, const H225_ResourcesAvailableConfirm & rac)
{
  if (!CheckForResponse(H225_RasMessage::e_resourcesAvailableIndicate, rac.m_requestSeqNum))
    return false;

  if (!CheckCryptoTokens(pdu, rac))
    return false;

  return OnReceiveResourcesAvailableConfirm(rac);
}


PBoolean H225_RAS::OnReceiveResourcesAvailableConfirm(const H225_ResourcesAvailableConfirm & /*rac*/)
{
  return true;
}


void H225_RAS::OnSendServiceControlIndication(H323RasPDU & pdu, H225_ServiceControlIndication & sci)
{
  OnSendServiceControlIndication(sci);
    
#if OPAL_H460
  SendGenericData(this, H460_MessageType::e_serviceControlIndication, sci);
#endif
    
  pdu.Prepare(sci);
}


void H225_RAS::OnSendServiceControlIndication(H225_ServiceControlIndication & /*sci*/)
{
}


PBoolean H225_RAS::OnReceiveServiceControlIndication(const H323RasPDU & pdu, const H225_ServiceControlIndication & sci)
{
  if (!CheckCryptoTokens(pdu, sci))
    return false;
    
#if OPAL_H460
  ReceiveGenericData(this, H460_MessageType::e_serviceControlIndication, sci);
#endif

  return OnReceiveServiceControlIndication(sci);
}


PBoolean H225_RAS::OnReceiveServiceControlIndication(const H225_ServiceControlIndication & /*sci*/)
{
  return true;
}


void H225_RAS::OnSendServiceControlResponse(H323RasPDU & pdu, H225_ServiceControlResponse & scr)
{
  OnSendServiceControlResponse(scr);
    
#if OPAL_H460
  SendGenericData(this, H460_MessageType::e_serviceControlResponse, scr);
#endif
    
  pdu.Prepare(scr);
}


void H225_RAS::OnSendServiceControlResponse(H225_ServiceControlResponse & /*scr*/)
{
}


PBoolean H225_RAS::OnReceiveServiceControlResponse(const H323RasPDU & pdu, const H225_ServiceControlResponse & scr)
{
  if (!CheckForResponse(H225_RasMessage::e_serviceControlIndication, scr.m_requestSeqNum))
    return false;

  if (!CheckCryptoTokens(pdu, scr))
    return false;
  
#if OPAL_H460
  ReceiveGenericData(this, H460_MessageType::e_serviceControlResponse, scr);
#endif

  return OnReceiveServiceControlResponse(scr);
}


PBoolean H225_RAS::OnReceiveServiceControlResponse(const H225_ServiceControlResponse & /*scr*/)
{
  return true;
}


void H225_RAS::OnSendInfoRequestAck(H323RasPDU & pdu, H225_InfoRequestAck & iack)
{
  OnSendInfoRequestAck(iack);
  pdu.Prepare(iack);
}


void H225_RAS::OnSendInfoRequestAck(H225_InfoRequestAck & /*iack*/)
{
}


PBoolean H225_RAS::OnReceiveInfoRequestAck(const H323RasPDU & pdu, const H225_InfoRequestAck & iack)
{
  if (!CheckForResponse(H225_RasMessage::e_infoRequestResponse, iack.m_requestSeqNum))
    return false;

  if (!CheckCryptoTokens(pdu, iack))
    return false;

  return OnReceiveInfoRequestAck(iack);
}


PBoolean H225_RAS::OnReceiveInfoRequestAck(const H225_InfoRequestAck & /*iack*/)
{
  return true;
}


void H225_RAS::OnSendInfoRequestNak(H323RasPDU & pdu, H225_InfoRequestNak & inak)
{
  OnSendInfoRequestNak(inak);
  pdu.Prepare(inak);
}


void H225_RAS::OnSendInfoRequestNak(H225_InfoRequestNak & /*inak*/)
{
}


PBoolean H225_RAS::OnReceiveInfoRequestNak(const H323RasPDU & pdu, const H225_InfoRequestNak & inak)
{
  if (!CheckForResponse(H225_RasMessage::e_infoRequestResponse, inak.m_requestSeqNum, &inak.m_nakReason))
    return false;

  if (!CheckCryptoTokens(pdu, inak))
    return false;

  return OnReceiveInfoRequestNak(inak);
}


PBoolean H225_RAS::OnReceiveInfoRequestNak(const H225_InfoRequestNak & /*inak*/)
{
  return true;
}


PBoolean H225_RAS::OnReceiveUnknown(const H323RasPDU &)
{
  H323RasPDU response;
  response.BuildUnknownMessageResponse(0);
  return response.Write(*m_transport);
}


#endif // OPAL_H323

/////////////////////////////////////////////////////////////////////////////
