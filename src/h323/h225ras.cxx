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
 * Revision 1.2001  2001/07/27 15:48:25  robertj
 * Conversion of OpenH323 to Open Phone Abstraction Library (OPAL)
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
#include <h323/h235ras.h>
#include <h323/transaddr.h>


#define new PNEW


/////////////////////////////////////////////////////////////////////////////

H225_RAS::H225_RAS(H323EndPoint & ep, OpalTransport * trans)
  : endpoint(ep)
{
  if (trans != NULL)
    transport = trans;
  else
    transport = new OpalTransportUDP(ep, INADDR_ANY, DefaultRasUdpPort);

  pregrantMakeCall = pregrantAnswerCall = RequireARQ;

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

  H323_DECLARE_RAS_PDU(response, endpoint);
  while (response.Read(*transport)) {
    if (HandleRasPDU(response))
      responseHandled.Signal();
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


void H225_RAS::OnReceiveGatekeeperRequest(const H225_GatekeeperRequest &)
{
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


void H225_RAS::OnSendRegistrationRequest(H225_RegistrationRequest &)
{
}


void H225_RAS::OnSendRegistrationConfirm(H225_RegistrationConfirm & rcf)
{
  if (!gatekeeperIdentifier) {
    rcf.IncludeOptionalField(H225_RegistrationConfirm::e_gatekeeperIdentifier);
    rcf.m_gatekeeperIdentifier = gatekeeperIdentifier;
  }
}


void H225_RAS::OnSendRegistrationReject(H225_RegistrationReject & rrj)
{
  if (!gatekeeperIdentifier) {
    rrj.IncludeOptionalField(H225_RegistrationReject::e_gatekeeperIdentifier);
    rrj.m_gatekeeperIdentifier = gatekeeperIdentifier;
  }
}


void H225_RAS::OnReceiveRegistrationRequest(const H225_RegistrationRequest &)
{
}


BOOL H225_RAS::OnReceiveRegistrationConfirm(const H225_RegistrationConfirm & rcf)
{
  return CheckForResponse(H225_RasMessage::e_registrationRequest, rcf.m_requestSeqNum);
}


BOOL H225_RAS::OnReceiveRegistrationReject(const H225_RegistrationReject & pdu)
{
  return CheckForRejectResponse(H225_RasMessage::e_registrationRequest,
                                pdu.m_requestSeqNum,
                                "Registration",
                                pdu.m_rejectReason);
}


void H225_RAS::OnSendUnregistrationRequest(H225_UnregistrationRequest &)
{
}


void H225_RAS::OnSendUnregistrationConfirm(H225_UnregistrationConfirm &)
{
}


void H225_RAS::OnSendUnregistrationReject(H225_UnregistrationReject &)
{
}


void H225_RAS::OnReceiveUnregistrationRequest(const H225_UnregistrationRequest &)
{
}


BOOL H225_RAS::OnReceiveUnregistrationConfirm(const H225_UnregistrationConfirm & ucf)
{
  return CheckForResponse(H225_RasMessage::e_unregistrationRequest, ucf.m_requestSeqNum);
}


BOOL H225_RAS::OnReceiveUnregistrationReject(const H225_UnregistrationReject & pdu)
{
  return CheckForRejectResponse(H225_RasMessage::e_unregistrationRequest,
                                pdu.m_requestSeqNum,
                                "Unregistration",
                                pdu.m_rejectReason);
}


void H225_RAS::OnSendAdmissionRequest(H225_AdmissionRequest &)
{
}


void H225_RAS::OnSendAdmissionConfirm(H225_AdmissionConfirm &)
{
}


void H225_RAS::OnSendAdmissionReject(H225_AdmissionReject &)
{
}


void H225_RAS::OnReceiveAdmissionRequest(const H225_AdmissionRequest &)
{
}


BOOL H225_RAS::OnReceiveAdmissionConfirm(const H225_AdmissionConfirm & acf)
{
  return CheckForResponse(H225_RasMessage::e_admissionRequest, acf.m_requestSeqNum);
}


BOOL H225_RAS::OnReceiveAdmissionReject(const H225_AdmissionReject & pdu)
{
  return CheckForRejectResponse(H225_RasMessage::e_admissionRequest,
                                pdu.m_requestSeqNum,
                                "Admission",
                                pdu.m_rejectReason);
}


void H225_RAS::OnSendBandwidthRequest(H225_BandwidthRequest &)
{
}


void H225_RAS::OnReceiveBandwidthRequest(const H225_BandwidthRequest &)
{
}


void H225_RAS::OnSendBandwidthConfirm(H225_BandwidthConfirm &)
{
}


BOOL H225_RAS::OnReceiveBandwidthConfirm(const H225_BandwidthConfirm & bcf)
{
  return CheckForResponse(H225_RasMessage::e_bandwidthRequest, bcf.m_requestSeqNum);
}


void H225_RAS::OnSendBandwidthReject(H225_BandwidthReject &)
{
}


BOOL H225_RAS::OnReceiveBandwidthReject(const H225_BandwidthReject & pdu)
{
  return CheckForRejectResponse(H225_RasMessage::e_bandwidthRequest,
                                pdu.m_requestSeqNum,
                                "Bandwidth",
                                pdu.m_rejectReason);
}


void H225_RAS::OnSendDisengageRequest(H225_DisengageRequest &)
{
}


void H225_RAS::OnReceiveDisengageRequest(const H225_DisengageRequest &)
{
}


void H225_RAS::OnSendDisengageConfirm(H225_DisengageConfirm &)
{
}


BOOL H225_RAS::OnReceiveDisengageConfirm(const H225_DisengageConfirm & dcf)
{
  return CheckForResponse(H225_RasMessage::e_disengageRequest, dcf.m_requestSeqNum);
}


void H225_RAS::OnSendDisengageReject(H225_DisengageReject &)
{
}


BOOL H225_RAS::OnReceiveDisengageReject(const H225_DisengageReject & pdu)
{
  return CheckForRejectResponse(H225_RasMessage::e_disengageRequest,
                                pdu.m_requestSeqNum,
                                "Disengage",
                                pdu.m_rejectReason);
}


void H225_RAS::OnSendLocationRequest(H225_LocationRequest &)
{
}


void H225_RAS::OnReceiveLocationRequest(const H225_LocationRequest &)
{
}


void H225_RAS::OnSendLocationConfirm(H225_LocationConfirm &)
{
}


BOOL H225_RAS::OnReceiveLocationConfirm(const H225_LocationConfirm & lcf)
{
  if (!CheckForResponse(H225_RasMessage::e_locationRequest, lcf.m_requestSeqNum))
    return FALSE;

  locatedAddress = lcf.m_callSignalAddress;

  return TRUE;
}


void H225_RAS::OnSendLocationReject(H225_LocationReject &)
{
}


BOOL H225_RAS::OnReceiveLocationReject(const H225_LocationReject & pdu)
{
  return CheckForRejectResponse(H225_RasMessage::e_locationRequest,
                                pdu.m_requestSeqNum,
                                "Location",
                                pdu.m_rejectReason);
}


void H225_RAS::OnSendInfoRequest(H225_InfoRequest &)
{
}


void H225_RAS::OnReceiveInfoRequest(const H225_InfoRequest &)
{
}


void H225_RAS::OnSendInfoRequestResponse(H225_InfoRequestResponse &)
{
}


BOOL H225_RAS::OnReceiveInfoRequestResponse(const H225_InfoRequestResponse & pdu)
{
  return CheckForResponse(H225_RasMessage::e_infoRequest, pdu.m_requestSeqNum);
}


void H225_RAS::OnSendNonStandardMessage(H225_NonStandardMessage &)
{
}


void H225_RAS::OnReceiveNonStandardMessage(const H225_NonStandardMessage &)
{
}


void H225_RAS::OnSendUnknownMessageResponse(H225_UnknownMessageResponse &)
{
}


void H225_RAS::OnReceiveUnknownMessageResponse(const H225_UnknownMessageResponse &)
{
}


void H225_RAS::OnSendRequestInProgress(H225_RequestInProgress &)
{
}


void H225_RAS::OnSendResourcesAvailableIndicate(H225_ResourcesAvailableIndicate &)
{
}


void H225_RAS::OnReceiveResourcesAvailableIndicate(const H225_ResourcesAvailableIndicate &)
{
}


void H225_RAS::OnSendResourcesAvailableConfirm(H225_ResourcesAvailableConfirm &)
{
}


BOOL H225_RAS::OnReceiveResourcesAvailableConfirm(const H225_ResourcesAvailableConfirm & pdu)
{
  return CheckForResponse(H225_RasMessage::e_resourcesAvailableIndicate, pdu.m_requestSeqNum);
}


void H225_RAS::OnSendInfoRequestAck(H225_InfoRequestAck &)
{
}


BOOL H225_RAS::OnReceiveInfoRequestAck(const H225_InfoRequestAck & pdu)
{
  return CheckForResponse(H225_RasMessage::e_infoRequest, pdu.m_requestSeqNum);
}


void H225_RAS::OnSendInfoRequestNak(H225_InfoRequestNak &)
{
}


BOOL H225_RAS::OnReceiveInfoRequestNak(const H225_InfoRequestNak & pdu)
{
  return CheckForResponse(H225_RasMessage::e_infoRequest, pdu.m_requestSeqNum);
}


void H225_RAS::OnReceiveUnkown(const H323RasPDU &)
{
  H323_DECLARE_RAS_PDU(response, endpoint);
  response.BuildUnknownMessageResponse(0);
  response.Write(*transport);
}


unsigned H225_RAS::GetNextSequenceNumber()
{
  lastSequenceNumber++;
  if (lastSequenceNumber >= 65536)
    lastSequenceNumber = 1;
  return lastSequenceNumber;
}


/////////////////////////////////////////////////////////////////////////////
