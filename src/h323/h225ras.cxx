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
 * $Log: h225ras.cxx,v $
 * Revision 1.2002  2001/08/13 05:10:39  robertj
 * Updates from OpenH323 v1.6.0 release.
 *
 * Revision 2.0  2001/07/27 15:48:25  robertj
 * Conversion of OpenH323 to Open Phone Abstraction Library (OPAL)
 *
 * Revision 1.8  2001/08/10 11:03:52  robertj
 * Major changes to H.235 support in RAS to support server.
 *
 * Revision 1.7  2001/08/06 07:44:55  robertj
 * Fixed problems with building without SSL
 *
 * Revision 1.6  2001/08/06 03:18:38  robertj
 * Fission of h323.h to h323ep.h & h323con.h, h323.h now just includes files.
 * Improved access to H.235 secure RAS functionality.
 * Changes to H.323 secure RAS contexts to help use with gk server.
 *
 * Revision 1.5  2001/08/03 05:56:04  robertj
 * Fixed RAS read of UDP when get ICMP error for host unreachabe.
 *
 * Revision 1.4  2001/06/25 01:06:40  robertj
 * Fixed resolution of RAS timeout so not rounded down to second.
 *
 * Revision 1.3  2001/06/22 00:21:10  robertj
 * Fixed bug in H.225 RAS protocol with 16 versus 32 bit sequence numbers.
 *
 * Revision 1.2  2001/06/18 07:44:21  craigs
 * Made to compile with h225ras.cxx under Linux
 *
 * Revision 1.1  2001/06/18 06:23:50  robertj
 * Split raw H.225 RAS protocol out of gatekeeper client class.
 *
 */

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "h225ras.h"
#endif

#include <h323/h225ras.h>

#include <h323/h323ep.h>
#include <h323/transaddr.h>
#include <h323/h323pdu.h>
#include <h323/h235auth.h>


#define new PNEW


/////////////////////////////////////////////////////////////////////////////

H225_RAS::H225_RAS(H323EndPoint & ep, OpalTransport * trans)
  : endpoint(ep)
{
  if (trans != NULL)
    transport = trans;
  else
    transport = new OpalTransportUDP(ep, INADDR_ANY, DefaultRasUdpPort);

  lastSequenceNumber = 0;
}


H225_RAS::~H225_RAS()
{
  if (transport != NULL) {
    transport->CloseWait();
    delete transport;
  }
}


BOOL H225_RAS::StartRasChannel()
{
  transport->AttachThread(PThread::Create(PCREATE_NOTIFIER(HandleRasChannel), 0,
                                          PThread::NoAutoDeleteThread,
                                          PThread::NormalPriority,
                                          "H225 RAS:%x"));
  return TRUE;
}


void H225_RAS::HandleRasChannel(PThread &, INT)
{
  transport->SetReadTimeout(PMaxTimeInterval);

  H323RasPDU response(*this);
  lastReceivedPDU = &response;

  for (;;) {
    if (response.Read(*transport)) {
      if (HandleRasPDU(response))
        responseHandled.Signal();
    }
    else {
      switch (transport->GetErrorNumber()) {
        case ECONNRESET :
        case ECONNREFUSED :
          PTRACE(2, "RAS\tCannot access remote " << transport->GetRemoteAddress());
          responseResult = RejectReceived;
          responseHandled.Signal();
          break;

        default:
          PTRACE(1, "RAS\tRead error: " << transport->GetErrorText());
          return;
      }
    }
  }
}


BOOL H225_RAS::HandleRasPDU(const H323RasPDU & response)
{
  switch (response.GetTag()) {
    case H225_RasMessage::e_gatekeeperRequest :
      OnReceiveGatekeeperRequest(response);
      break;

    case H225_RasMessage::e_gatekeeperConfirm :
      return OnReceiveGatekeeperConfirm(response);

    case H225_RasMessage::e_gatekeeperReject :
      return OnReceiveGatekeeperReject(response);

    case H225_RasMessage::e_registrationRequest :
      OnReceiveRegistrationRequest(response);
      break;

    case H225_RasMessage::e_registrationConfirm :
      return OnReceiveRegistrationConfirm(response);

    case H225_RasMessage::e_registrationReject :
      return OnReceiveRegistrationReject(response);

    case H225_RasMessage::e_unregistrationRequest :
      OnReceiveUnregistrationRequest(response);
      break;

    case H225_RasMessage::e_unregistrationConfirm :
      return OnReceiveUnregistrationConfirm(response);

    case H225_RasMessage::e_unregistrationReject :
      return OnReceiveUnregistrationReject(response);

    case H225_RasMessage::e_admissionRequest :
      OnReceiveAdmissionRequest(response);
      break;

    case H225_RasMessage::e_admissionConfirm :
      return OnReceiveAdmissionConfirm(response);

    case H225_RasMessage::e_admissionReject :
      return OnReceiveAdmissionReject(response);

    case H225_RasMessage::e_bandwidthRequest :
      OnReceiveBandwidthRequest(response);
      break;

    case H225_RasMessage::e_bandwidthConfirm :
      return OnReceiveBandwidthConfirm(response);

    case H225_RasMessage::e_bandwidthReject :
      return OnReceiveBandwidthReject(response);

    case H225_RasMessage::e_disengageRequest :
      OnReceiveDisengageRequest(response);
      break;

    case H225_RasMessage::e_disengageConfirm :
      return OnReceiveDisengageConfirm(response);

    case H225_RasMessage::e_disengageReject :
      return OnReceiveDisengageReject(response);

    case H225_RasMessage::e_locationRequest :
      OnReceiveLocationRequest(response);
      break;

    case H225_RasMessage::e_locationConfirm :
      return OnReceiveLocationConfirm(response);

    case H225_RasMessage::e_locationReject :
      return OnReceiveLocationReject(response);

    case H225_RasMessage::e_infoRequest :
      OnReceiveInfoRequest(response);
      break;

    case H225_RasMessage::e_infoRequestResponse :
      return OnReceiveInfoRequestResponse(response);

    case H225_RasMessage::e_nonStandardMessage :
      OnReceiveNonStandardMessage(response);
      break;

    case H225_RasMessage::e_unknownMessageResponse :
      OnReceiveUnknownMessageResponse(response);
      break;

    case H225_RasMessage::e_requestInProgress :
      return OnReceiveRequestInProgress(response);

    case H225_RasMessage::e_resourcesAvailableIndicate :
      OnReceiveResourcesAvailableIndicate(response);
      break;

    case H225_RasMessage::e_resourcesAvailableConfirm :
      return OnReceiveResourcesAvailableConfirm(response);

    case H225_RasMessage::e_infoRequestAck :
      return OnReceiveInfoRequestAck(response);

    case H225_RasMessage::e_infoRequestNak :
      return OnReceiveInfoRequestNak(response);

    default :
      OnReceiveUnkown(response);
  }

  return FALSE;
}


BOOL H225_RAS::WritePDU(H323RasPDU & pdu)
{
  switch (pdu.GetTag()) {
    case H225_RasMessage::e_gatekeeperRequest :
      OnSendGatekeeperRequest(pdu);
      break;

    case H225_RasMessage::e_gatekeeperConfirm :
      OnSendGatekeeperConfirm(pdu);
      break;

    case H225_RasMessage::e_gatekeeperReject :
      OnSendGatekeeperReject(pdu);
      break;

    case H225_RasMessage::e_registrationRequest :
      OnSendRegistrationRequest(pdu);
      break;

    case H225_RasMessage::e_registrationConfirm :
      OnSendRegistrationConfirm(pdu);
      break;

    case H225_RasMessage::e_registrationReject :
      OnSendRegistrationReject(pdu);
      break;

    case H225_RasMessage::e_unregistrationRequest :
      OnSendUnregistrationRequest(pdu);
      break;

    case H225_RasMessage::e_unregistrationConfirm :
      OnSendUnregistrationConfirm(pdu);
      break;

    case H225_RasMessage::e_unregistrationReject :
      OnSendUnregistrationReject(pdu);
      break;

    case H225_RasMessage::e_admissionRequest :
      OnSendAdmissionRequest(pdu);
      break;

    case H225_RasMessage::e_admissionConfirm :
      OnSendAdmissionConfirm(pdu);
      break;

    case H225_RasMessage::e_admissionReject :
      OnSendAdmissionReject(pdu);
      break;

    case H225_RasMessage::e_bandwidthRequest :
      OnSendBandwidthRequest(pdu);
      break;

    case H225_RasMessage::e_bandwidthConfirm :
      OnSendBandwidthConfirm(pdu);
      break;

    case H225_RasMessage::e_bandwidthReject :
      OnSendBandwidthReject(pdu);
      break;

    case H225_RasMessage::e_disengageRequest :
      OnSendDisengageRequest(pdu);
      break;

    case H225_RasMessage::e_disengageConfirm :
      OnSendDisengageConfirm(pdu);
      break;

    case H225_RasMessage::e_disengageReject :
      OnSendDisengageReject(pdu);
      break;

    case H225_RasMessage::e_locationRequest :
      OnSendLocationRequest(pdu);
      break;

    case H225_RasMessage::e_locationConfirm :
      OnSendLocationConfirm(pdu);
      break;

    case H225_RasMessage::e_locationReject :
      OnSendLocationReject(pdu);
      break;

    case H225_RasMessage::e_infoRequest :
      OnSendInfoRequest(pdu);
      break;

    case H225_RasMessage::e_infoRequestResponse :
      OnSendInfoRequestResponse(pdu);
      break;

    case H225_RasMessage::e_nonStandardMessage :
      OnSendNonStandardMessage(pdu);
      break;

    case H225_RasMessage::e_unknownMessageResponse :
      OnSendUnknownMessageResponse(pdu);
      break;

    case H225_RasMessage::e_requestInProgress :
      OnSendRequestInProgress(pdu);
      break;

    case H225_RasMessage::e_resourcesAvailableIndicate :
      OnSendResourcesAvailableIndicate(pdu);
      break;

    case H225_RasMessage::e_resourcesAvailableConfirm :
      OnSendResourcesAvailableConfirm(pdu);
      break;

    case H225_RasMessage::e_infoRequestAck :
      OnSendInfoRequestAck(pdu);
      break;

    case H225_RasMessage::e_infoRequestNak :
      OnSendInfoRequestNak(pdu);
      break;

    default :
      break;
  }

  return pdu.Write(*transport);
}


BOOL H225_RAS::MakeRequest(H323RasPDU & request)
{
  PTRACE(3, "RAS\tMaking request: " << request.GetTagName());

  // Assume a confirm, reject callbacks will set to RejectReceived
  responseResult = AwaitingResponse;

  // Set request code for determining response
  requestTag = request.GetTag();

  for (unsigned retry = 1; retry <= endpoint.GetRasRequestRetries(); retry++) {
    if (!WritePDU(request))
      return FALSE;

    whenResponseExpected = PTimer::Tick() + endpoint.GetRasRequestTimeout();
    PTRACE(3, "RAS\tWaiting on response for "
           << setprecision(1) << endpoint.GetRasRequestTimeout() << " seconds");
    while (responseHandled.Wait(whenResponseExpected - PTimer::Tick())) {
      switch (responseResult) {
        case ConfirmReceived :
          return TRUE;

        case RejectReceived :
          return FALSE;

        case RequestInProgress :
          responseResult = ConfirmReceived;
          PTRACE(3, "RAS\tWaiting again on response for "
            << setprecision(1) << (whenResponseExpected - PTimer::Tick()) << " seconds");

        default :
          ; // Keep waiting
      }
    }

    PTRACE(1, "RAS\tTimeout on request, try #" << retry << " of " << endpoint.GetRasRequestRetries());
  }

  return FALSE;
}


BOOL H225_RAS::CheckForResponse(unsigned reqTag, unsigned seqNum)
{
  if (requestTag != reqTag)
    return FALSE;

  if (seqNum == lastSequenceNumber) {
    responseResult = ConfirmReceived;
    return TRUE;
  }

  PTRACE(3, "RAS\tReceived incorrect sequence number, want "
         << lastSequenceNumber << ", got " << seqNum);
  return FALSE;
}


BOOL H225_RAS::CheckForRejectResponse(unsigned reqTag,
                                            unsigned seqNum,
#if PTRACING
                                            const char * name,
#else
                                            const char *,
#endif
                                            const PASN_Choice & reason)
{
  if (!CheckForResponse(reqTag, seqNum))
    return FALSE;

  PTRACE(1, "RAS\t" << name << " rejected: " << reason.GetTagName());
  responseResult = RejectReceived;
  rejectReason = reason.GetTag();
  return TRUE;
}


BOOL H225_RAS::SetUpCallSignalAddresses(H225_ArrayOf_TransportAddress & addresses)
{
  H323TransportAddress address = transport->GetRemoteAddress();
  H225_TransportAddress rasAddress;
  address.SetPDU(rasAddress);

  const OpalListenerList & listeners = endpoint.GetListeners();
  for (PINDEX i = 0; i < listeners.GetSize(); i++) {
    address = listeners[i].GetLocalAddress();
    address.SetPDU(addresses, rasAddress);
  }

  return addresses.GetSize() > 0;
}


BOOL H225_RAS::OnReceiveRequestInProgress(const H225_RequestInProgress & rip)
{
  if (!CheckForResponse(requestTag, rip.m_requestSeqNum))
    return FALSE;

  responseResult = RequestInProgress;
  whenResponseExpected = PTimer::Tick() + PTimeInterval(rip.m_delay);

  return TRUE;
}


void H225_RAS::OnSendGatekeeperRequest(H225_GatekeeperRequest & grq)
{
  if (!gatekeeperIdentifier) {
    grq.IncludeOptionalField(H225_GatekeeperRequest::e_gatekeeperIdentifier);
    grq.m_gatekeeperIdentifier = gatekeeperIdentifier;
  }

  H235Authenticators authenticators = GetAuthenticators();
  for (PINDEX i = 0; i < authenticators.GetSize(); i++) {
    if (authenticators[i].SetCapability(grq.m_authenticationCapability, grq.m_algorithmOIDs)) {
      grq.IncludeOptionalField(H225_GatekeeperRequest::e_authenticationCapability);
      grq.IncludeOptionalField(H225_GatekeeperRequest::e_algorithmOIDs);
    }
  }
}


void H225_RAS::OnSendGatekeeperConfirm(H225_GatekeeperConfirm & gcf)
{
  if (!gatekeeperIdentifier) {
    gcf.IncludeOptionalField(H225_GatekeeperConfirm::e_gatekeeperIdentifier);
    gcf.m_gatekeeperIdentifier = gatekeeperIdentifier;
  }
}


void H225_RAS::OnSendGatekeeperReject(H225_GatekeeperReject & grj)
{
  if (!gatekeeperIdentifier) {
    grj.IncludeOptionalField(H225_GatekeeperReject::e_gatekeeperIdentifier);
    grj.m_gatekeeperIdentifier = gatekeeperIdentifier;
  }
}


BOOL H225_RAS::OnReceiveGatekeeperRequest(const H225_GatekeeperRequest &)
{
  return TRUE;
}


BOOL H225_RAS::OnReceiveGatekeeperConfirm(const H225_GatekeeperConfirm & gcf)
{
  return CheckForResponse(H225_RasMessage::e_gatekeeperRequest, gcf.m_requestSeqNum);
}


BOOL H225_RAS::OnReceiveGatekeeperReject(const H225_GatekeeperReject & grj)
{
  if (!CheckForResponse(H225_RasMessage::e_gatekeeperReject, grj.m_requestSeqNum))
    return FALSE;

  PTRACE(1, "RAS\tGatekeeper discovery rejected: " << grj.m_rejectReason.GetTagName());
  rejectReason = grj.m_rejectReason.GetTag();
  return FALSE;
}


void H225_RAS::OnSendRegistrationRequest(H225_RegistrationRequest & rrq)
{
  SetCryptoTokens(rrq.m_cryptoTokens, rrq, H225_RegistrationRequest::e_cryptoTokens);
}


void H225_RAS::OnSendRegistrationConfirm(H225_RegistrationConfirm & rcf)
{
  if (!gatekeeperIdentifier) {
    rcf.IncludeOptionalField(H225_RegistrationConfirm::e_gatekeeperIdentifier);
    rcf.m_gatekeeperIdentifier = gatekeeperIdentifier;
  }

  SetCryptoTokens(rcf.m_cryptoTokens, rcf, H225_RegistrationConfirm::e_cryptoTokens);
}


void H225_RAS::OnSendRegistrationReject(H225_RegistrationReject & rrj)
{
  if (!gatekeeperIdentifier) {
    rrj.IncludeOptionalField(H225_RegistrationReject::e_gatekeeperIdentifier);
    rrj.m_gatekeeperIdentifier = gatekeeperIdentifier;
  }

  SetCryptoTokens(rrj.m_cryptoTokens, rrj, H225_RegistrationReject::e_cryptoTokens);
}


BOOL H225_RAS::OnReceiveRegistrationRequest(const H225_RegistrationRequest &)
{
  return TRUE;
}


BOOL H225_RAS::OnReceiveRegistrationConfirm(const H225_RegistrationConfirm & rcf)
{
  return CheckForResponse(H225_RasMessage::e_registrationRequest, rcf.m_requestSeqNum) &&
         CheckCryptoTokens(rcf.m_cryptoTokens, rcf, H225_RegistrationConfirm::e_cryptoTokens);
}


BOOL H225_RAS::OnReceiveRegistrationReject(const H225_RegistrationReject & rrj)
{
  return CheckForRejectResponse(H225_RasMessage::e_registrationRequest,
                                rrj.m_requestSeqNum,
                                "Registration",
                                rrj.m_rejectReason) &&
         CheckCryptoTokens(rrj.m_cryptoTokens, rrj, H225_RegistrationReject::e_cryptoTokens);
}


void H225_RAS::OnSendUnregistrationRequest(H225_UnregistrationRequest & urq)
{
  SetCryptoTokens(urq.m_cryptoTokens, urq, H225_UnregistrationRequest::e_cryptoTokens);
}


void H225_RAS::OnSendUnregistrationConfirm(H225_UnregistrationConfirm & ucf)
{
  SetCryptoTokens(ucf.m_cryptoTokens, ucf, H225_UnregistrationConfirm::e_cryptoTokens);
}


void H225_RAS::OnSendUnregistrationReject(H225_UnregistrationReject & urj)
{
  SetCryptoTokens(urj.m_cryptoTokens, urj, H225_UnregistrationReject::e_cryptoTokens);
}


BOOL H225_RAS::OnReceiveUnregistrationRequest(const H225_UnregistrationRequest & urq)
{
  return CheckCryptoTokens(urq.m_cryptoTokens, urq, H225_UnregistrationRequest::e_cryptoTokens);
}


BOOL H225_RAS::OnReceiveUnregistrationConfirm(const H225_UnregistrationConfirm & ucf)
{
  return CheckForResponse(H225_RasMessage::e_unregistrationRequest, ucf.m_requestSeqNum) &&
         CheckCryptoTokens(ucf.m_cryptoTokens, ucf, H225_UnregistrationConfirm::e_cryptoTokens);
}


BOOL H225_RAS::OnReceiveUnregistrationReject(const H225_UnregistrationReject & urj)
{
  return CheckForRejectResponse(H225_RasMessage::e_unregistrationRequest,
                                urj.m_requestSeqNum,
                                "Unregistration",
                                urj.m_rejectReason) &&
         CheckCryptoTokens(urj.m_cryptoTokens, urj, H225_UnregistrationReject::e_cryptoTokens);
}


void H225_RAS::OnSendAdmissionRequest(H225_AdmissionRequest & arq)
{
  SetCryptoTokens(arq.m_cryptoTokens, arq, H225_AdmissionRequest::e_cryptoTokens);
}


void H225_RAS::OnSendAdmissionConfirm(H225_AdmissionConfirm & acf)
{
  SetCryptoTokens(acf.m_cryptoTokens, acf, H225_AdmissionConfirm::e_cryptoTokens);
}


void H225_RAS::OnSendAdmissionReject(H225_AdmissionReject & arj)
{
  SetCryptoTokens(arj.m_cryptoTokens, arj, H225_AdmissionReject::e_cryptoTokens);
}


BOOL H225_RAS::OnReceiveAdmissionRequest(const H225_AdmissionRequest & arq)
{
  return CheckCryptoTokens(arq.m_cryptoTokens, arq, H225_AdmissionRequest::e_cryptoTokens);
}


BOOL H225_RAS::OnReceiveAdmissionConfirm(const H225_AdmissionConfirm & acf)
{
  return CheckForResponse(H225_RasMessage::e_admissionRequest, acf.m_requestSeqNum) &&
         CheckCryptoTokens(acf.m_cryptoTokens, acf, H225_AdmissionConfirm::e_cryptoTokens);
}


BOOL H225_RAS::OnReceiveAdmissionReject(const H225_AdmissionReject & arj)
{
  return CheckForRejectResponse(H225_RasMessage::e_admissionRequest,
                                arj.m_requestSeqNum,
                                "Admission",
                                arj.m_rejectReason) &&
         CheckCryptoTokens(arj.m_cryptoTokens, arj, H225_AdmissionReject::e_cryptoTokens);
}


void H225_RAS::OnSendBandwidthRequest(H225_BandwidthRequest & brq)
{
  SetCryptoTokens(brq.m_cryptoTokens, brq, H225_BandwidthRequest::e_cryptoTokens);
}


BOOL H225_RAS::OnReceiveBandwidthRequest(const H225_BandwidthRequest & brq)
{
  return CheckCryptoTokens(brq.m_cryptoTokens, brq, H225_BandwidthRequest::e_cryptoTokens);
}


void H225_RAS::OnSendBandwidthConfirm(H225_BandwidthConfirm & bcf)
{
  SetCryptoTokens(bcf.m_cryptoTokens, bcf, H225_BandwidthConfirm::e_cryptoTokens);
}


BOOL H225_RAS::OnReceiveBandwidthConfirm(const H225_BandwidthConfirm & bcf)
{
  return CheckForResponse(H225_RasMessage::e_bandwidthRequest, bcf.m_requestSeqNum) &&
         CheckCryptoTokens(bcf.m_cryptoTokens, bcf, H225_BandwidthConfirm::e_cryptoTokens);
}


void H225_RAS::OnSendBandwidthReject(H225_BandwidthReject & brj)
{
  SetCryptoTokens(brj.m_cryptoTokens, brj, H225_BandwidthReject::e_cryptoTokens);
}


BOOL H225_RAS::OnReceiveBandwidthReject(const H225_BandwidthReject & brj)
{
  return CheckForRejectResponse(H225_RasMessage::e_bandwidthRequest,
                                brj.m_requestSeqNum,
                                "Bandwidth",
                                brj.m_rejectReason) &&
         CheckCryptoTokens(brj.m_cryptoTokens, brj, H225_BandwidthReject::e_cryptoTokens);
}


void H225_RAS::OnSendDisengageRequest(H225_DisengageRequest & drq)
{
  SetCryptoTokens(drq.m_cryptoTokens, drq, H225_DisengageRequest::e_cryptoTokens);
}


BOOL H225_RAS::OnReceiveDisengageRequest(const H225_DisengageRequest & drq)
{
  return CheckCryptoTokens(drq.m_cryptoTokens, drq, H225_DisengageRequest::e_cryptoTokens);
}


void H225_RAS::OnSendDisengageConfirm(H225_DisengageConfirm & dcf)
{
  SetCryptoTokens(dcf.m_cryptoTokens, dcf, H225_DisengageConfirm::e_cryptoTokens);
}


BOOL H225_RAS::OnReceiveDisengageConfirm(const H225_DisengageConfirm & dcf)
{
  return CheckForResponse(H225_RasMessage::e_disengageRequest, dcf.m_requestSeqNum) &&
         CheckCryptoTokens(dcf.m_cryptoTokens, dcf, H225_DisengageConfirm::e_cryptoTokens);
}


void H225_RAS::OnSendDisengageReject(H225_DisengageReject & drj)
{
  SetCryptoTokens(drj.m_cryptoTokens, drj, H225_DisengageReject::e_cryptoTokens);
}


BOOL H225_RAS::OnReceiveDisengageReject(const H225_DisengageReject & drj)
{
  return CheckForRejectResponse(H225_RasMessage::e_disengageRequest,
                                drj.m_requestSeqNum,
                                "Disengage",
                                drj.m_rejectReason) &&
         CheckCryptoTokens(drj.m_cryptoTokens, drj, H225_DisengageReject::e_cryptoTokens);
}


void H225_RAS::OnSendLocationRequest(H225_LocationRequest & lrq)
{
  SetCryptoTokens(lrq.m_cryptoTokens, lrq, H225_LocationRequest::e_cryptoTokens);
}


BOOL H225_RAS::OnReceiveLocationRequest(const H225_LocationRequest & lrq)
{
  return CheckCryptoTokens(lrq.m_cryptoTokens, lrq, H225_LocationRequest::e_cryptoTokens);
}


void H225_RAS::OnSendLocationConfirm(H225_LocationConfirm & lcf)
{
  SetCryptoTokens(lcf.m_cryptoTokens, lcf, H225_LocationConfirm::e_cryptoTokens);
}


BOOL H225_RAS::OnReceiveLocationConfirm(const H225_LocationConfirm & lcf)
{
  if (!CheckForResponse(H225_RasMessage::e_locationRequest, lcf.m_requestSeqNum))
    return FALSE;

  locatedAddress = lcf.m_callSignalAddress;

  return TRUE;
}


void H225_RAS::OnSendLocationReject(H225_LocationReject & lrj)
{
  SetCryptoTokens(lrj.m_cryptoTokens, lrj, H225_LocationReject::e_cryptoTokens);
}


BOOL H225_RAS::OnReceiveLocationReject(const H225_LocationReject & lrj)
{
  return CheckForRejectResponse(H225_RasMessage::e_locationRequest,
                                lrj.m_requestSeqNum,
                                "Location",
                                lrj.m_rejectReason) &&
         CheckCryptoTokens(lrj.m_cryptoTokens, lrj, H225_LocationReject::e_cryptoTokens);
}


void H225_RAS::OnSendInfoRequest(H225_InfoRequest & irq)
{
  SetCryptoTokens(irq.m_cryptoTokens, irq, H225_InfoRequest::e_cryptoTokens);
}


BOOL H225_RAS::OnReceiveInfoRequest(const H225_InfoRequest & irq)
{
  return CheckCryptoTokens(irq.m_cryptoTokens, irq, H225_InfoRequest::e_cryptoTokens);
}


void H225_RAS::OnSendInfoRequestResponse(H225_InfoRequestResponse & irr)
{
  SetCryptoTokens(irr.m_cryptoTokens, irr, H225_InfoRequestResponse::e_cryptoTokens);
}


BOOL H225_RAS::OnReceiveInfoRequestResponse(const H225_InfoRequestResponse & irr)
{
  return CheckForResponse(H225_RasMessage::e_infoRequest, irr.m_requestSeqNum) &&
         CheckCryptoTokens(irr.m_cryptoTokens, irr, H225_InfoRequestResponse::e_cryptoTokens);
}


void H225_RAS::OnSendNonStandardMessage(H225_NonStandardMessage & nsm)
{
  SetCryptoTokens(nsm.m_cryptoTokens, nsm, H225_NonStandardMessage::e_cryptoTokens);
}


BOOL H225_RAS::OnReceiveNonStandardMessage(const H225_NonStandardMessage & nsm)
{
  return CheckCryptoTokens(nsm.m_cryptoTokens, nsm, H225_NonStandardMessage::e_cryptoTokens);
}


void H225_RAS::OnSendUnknownMessageResponse(H225_UnknownMessageResponse & umr)
{
  SetCryptoTokens(umr.m_cryptoTokens, umr, H225_UnknownMessageResponse::e_cryptoTokens);
}


BOOL H225_RAS::OnReceiveUnknownMessageResponse(const H225_UnknownMessageResponse & umr)
{
  return CheckCryptoTokens(umr.m_cryptoTokens, umr, H225_UnknownMessageResponse::e_cryptoTokens);
}


void H225_RAS::OnSendRequestInProgress(H225_RequestInProgress & rip)
{
  SetCryptoTokens(rip.m_cryptoTokens, rip, H225_RequestInProgress::e_cryptoTokens);
}


void H225_RAS::OnSendResourcesAvailableIndicate(H225_ResourcesAvailableIndicate & rai)
{
  SetCryptoTokens(rai.m_cryptoTokens, rai, H225_ResourcesAvailableIndicate::e_cryptoTokens);
}


BOOL H225_RAS::OnReceiveResourcesAvailableIndicate(const H225_ResourcesAvailableIndicate & rai)
{
  return CheckCryptoTokens(rai.m_cryptoTokens, rai, H225_ResourcesAvailableIndicate::e_cryptoTokens);
}


void H225_RAS::OnSendResourcesAvailableConfirm(H225_ResourcesAvailableConfirm & rac)
{
  SetCryptoTokens(rac.m_cryptoTokens, rac, H225_ResourcesAvailableConfirm::e_cryptoTokens);
}


BOOL H225_RAS::OnReceiveResourcesAvailableConfirm(const H225_ResourcesAvailableConfirm & rac)
{
  return CheckForResponse(H225_RasMessage::e_resourcesAvailableIndicate, rac.m_requestSeqNum) &&
         CheckCryptoTokens(rac.m_cryptoTokens, rac, H225_ResourcesAvailableConfirm::e_cryptoTokens);
}


void H225_RAS::OnSendInfoRequestAck(H225_InfoRequestAck & ira)
{
  SetCryptoTokens(ira.m_cryptoTokens, ira, H225_InfoRequestAck::e_cryptoTokens);
}


BOOL H225_RAS::OnReceiveInfoRequestAck(const H225_InfoRequestAck & ira)
{
  return CheckForResponse(H225_RasMessage::e_infoRequest, ira.m_requestSeqNum) &&
         CheckCryptoTokens(ira.m_cryptoTokens, ira, H225_InfoRequestAck::e_cryptoTokens);
}


void H225_RAS::OnSendInfoRequestNak(H225_InfoRequestNak & irn)
{
  SetCryptoTokens(irn.m_cryptoTokens, irn, H225_InfoRequestNak::e_cryptoTokens);
}


BOOL H225_RAS::OnReceiveInfoRequestNak(const H225_InfoRequestNak & irn)
{
  return CheckForResponse(H225_RasMessage::e_infoRequest, irn.m_requestSeqNum) &&
         CheckCryptoTokens(irn.m_cryptoTokens, irn, H225_InfoRequestNak::e_cryptoTokens);
}


BOOL H225_RAS::OnReceiveUnkown(const H323RasPDU &)
{
  H323RasPDU response(*this);
  response.BuildUnknownMessageResponse(0);
  return response.Write(*transport);
}


unsigned H225_RAS::GetNextSequenceNumber()
{
  lastSequenceNumber++;
  if (lastSequenceNumber >= 65536)
    lastSequenceNumber = 1;
  return lastSequenceNumber;
}


void H225_RAS::SetCryptoTokens(H225_ArrayOf_CryptoH323Token & cryptoTokens,
                               PASN_Sequence & pdu,
                               unsigned optionalField)
{
  H235Authenticators authenticators = GetAuthenticators();
  for (PINDEX i = 0; i < authenticators.GetSize(); i++) {
    if (authenticators[i].Prepare(cryptoTokens))
      pdu.IncludeOptionalField(optionalField);
  }
}


BOOL H225_RAS::CheckCryptoTokens(const H225_ArrayOf_CryptoH323Token & cryptoTokens,
                                 const PASN_Sequence & pdu,
                                 unsigned optionalField)
{
  // Check for no security
  H235Authenticators authenticators = GetAuthenticators();
  BOOL noneActive = TRUE;
  PINDEX i;
  for (i = 0; i < authenticators.GetSize(); i++) {
    if (authenticators[i].IsActive()) {
      noneActive = FALSE;
      break;
    }
  }

  if (noneActive)
    return TRUE;

  //do not accept non secure RAS Messages
  if (!pdu.HasOptionalField(optionalField)) {
    PTRACE(1, "Received unsecured RAS Message in H323RasH235PDU");
    return FALSE;
  }

  BOOL ok = FALSE;
  for (i = 0; i < authenticators.GetSize(); i++) {
    switch (authenticators[i].Verify(cryptoTokens, lastReceivedPDU->GetLastReceivedRawPDU())) {
      case H235Authenticator::e_OK :
        ok = TRUE;
        break;

      case H235Authenticator::e_Absent :
        authenticators[i].Disable();
        break;

      case H235Authenticator::e_Disabled :
        break;

      default :
        return FALSE;
    }
  }

  return ok;
}


/////////////////////////////////////////////////////////////////////////////
