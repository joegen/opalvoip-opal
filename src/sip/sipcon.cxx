/*
 * sipcon.cxx
 *
 * Session Initiation Protocol connection.
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
 * The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * Contributor(s): ______________________________________.
 *
 * $Log: sipcon.cxx,v $
 * Revision 1.2013  2002/04/09 01:19:41  robertj
 * Fixed typo on changing "callAnswered" to better description of "originating".
 *
 * Revision 2.11  2002/04/09 01:02:14  robertj
 * Fixed problems with restarting INVITE on  authentication required response.
 *
 * Revision 2.10  2002/04/08 02:40:13  robertj
 * Fixed issues with using double originate call, eg from simple app command line.
 *
 * Revision 2.9  2002/04/05 10:42:04  robertj
 * Major changes to support transactions (UDP timeouts and retries).
 *
 * Revision 2.8  2002/03/18 08:13:42  robertj
 * Added more logging.
 * Removed proxyusername/proxypassword parameters from URL passed on.
 * Fixed GNU warning.
 *
 * Revision 2.7  2002/03/15 10:55:28  robertj
 * Added ability to specify proxy username/password in URL.
 *
 * Revision 2.6  2002/03/08 06:28:03  craigs
 * Changed to allow Authorisation to be included in other PDUs
 *
 * Revision 2.5  2002/02/19 07:53:17  robertj
 * Restructured media bypass functions to fix problems with RFC2833.
 *
 * Revision 2.4  2002/02/13 04:55:44  craigs
 * Fixed problem with endless loop if proxy keeps failing authentication with 407
 *
 * Revision 2.3  2002/02/11 07:33:36  robertj
 * Changed SDP to use OpalTransport for hosts instead of IP addresses/ports
 * Added media bypass for streams between compatible protocols.
 *
 * Revision 2.2  2002/02/01 05:47:55  craigs
 * Removed :: from in front of isspace that confused gcc 2.96
 *
 * Revision 2.1  2002/02/01 04:53:01  robertj
 * Added (very primitive!) SIP support.
 *
 */

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "sipcon.h"
#endif

#include <sip/sipcon.h>

#include <sip/sipep.h>
#include <codec/rfc2833.h>
#include <opal/manager.h>
#include <opal/call.h>
#include <opal/patch.h>


typedef void (SIPConnection::* SIPMethodFunction)(SIP_PDU & pdu);


#define new PNEW


////////////////////////////////////////////////////////////////////////////

SIPConnection::SIPConnection(OpalCall & call,
                             SIPEndPoint & ep,
                             const PString & token,
                             OpalTransport * inviteTransport)
  : OpalConnection(call, ep, token),
    endpoint(ep),
    authentication(endpoint.GetRegistrationName(),
                   endpoint.GetRegistrationPassword()),
    pduSemaphore(0, P_MAX_INDEX)
{
  sendUserInputMode = SendUserInputAsInlineRFC2833;

  currentPhase = SetUpPhase;

  localPartyName = endpoint.GetRegistrationName();

  if (inviteTransport == NULL)
    transport = NULL;
  else {
    transport = inviteTransport->GetLocalAddress().CreateTransport(
                                         endpoint, OpalTransportAddress::HostOnly);
    transport->SetRemoteAddress(inviteTransport->GetRemoteAddress());
    transport->SetBufferSize(SIP_PDU::MaxSize); // Maximum possible PDU size
  }

  originalInvite = NULL;
  pduHandler = NULL;
  lastSentCSeq = 0;
  releaseMethod = ReleaseWithNothing;

  transactions.DisallowDeleteObjects();

  PTRACE(3, "SIP\tCreated connection.");
}


SIPConnection::~SIPConnection()
{
  delete originalInvite;
  delete transport;

  PTRACE(3, "SIP\tDeleted connection.");
}


OpalConnection::Phases SIPConnection::GetPhase() const
{
  return currentPhase;
}


BOOL SIPConnection::OnReleased()
{
  PTRACE(3, "SIP\tOnReleased");

  switch (releaseMethod) {
    case ReleaseWithNothing :
      break;

    case ReleaseWithResponse :
      switch (callEndReason) {
        case EndedByAnswerDenied :
          SendResponseToINVITE(SIP_PDU::Failure_Decline);
          break;
        case EndedByLocalBusy :
          SendResponseToINVITE(SIP_PDU::Failure_BusyHere);
          break;
        case EndedByCallerAbort :
          SendResponseToINVITE(SIP_PDU::Failure_RequestTerminated);
          break;
        default :
          SendResponseToINVITE(SIP_PDU::Failure_BadGateway);
      }
      break;

    case ReleaseWithBYE :
      {
        SIPTransaction bye(*this, *transport, SIP_PDU::Method_BYE);
        bye.Wait();
      }
      break;

    case ReleaseWithCANCEL :
      for (PINDEX i = 0; i < invitations.GetSize(); i++) {
        if (invitations[i].IsInProgress()) {
          invitations[i].SendCANCEL();
          invitations[i].Wait();
        }
      }
  }

  LockOnRelease();

  // send the appropriate form 
  PTRACE(2, "SIP\tReceived OnReleased in phase " << currentPhase);

  invitations.RemoveAll();

  if (pduHandler != NULL) {
    pduSemaphore.Signal();
    pduHandler->WaitForTermination();
    delete pduHandler;
  }

  if (transport != NULL)
    transport->CloseWait();

  return OpalConnection::OnReleased();
}


BOOL SIPConnection::SetAlerting(const PString & /*calleeName*/, BOOL /*withMedia*/)
{
  if (IsOriginating()) {
    PTRACE(2, "SIP\tSetAlerting ignored on call we originated.");
    return TRUE;
  }

  PTRACE(2, "SIP\tSetAlerting");
  if (!Lock())
    return FALSE;

  if (currentPhase != SetUpPhase) 
    return FALSE;

  SendResponseToINVITE(SIP_PDU::Information_Ringing);
  currentPhase = AlertingPhase;

  Unlock();

  return TRUE;
}


BOOL SIPConnection::SetConnected()
{
  if (IsOriginating()) {
    PTRACE(2, "SIP\tSetConnected ignored on call we originated.");
    return TRUE;
  }

  PINDEX i;
  PTRACE(2, "SIP\tSetConnected");

  // create an RTP session number
  // for now, just hard code it
  unsigned rtpSessionId = OpalMediaFormat::DefaultAudioSessionID;

  SDPSessionDescription sdpOut(GetLocalAddress());

  // get the remote media formats
  SDPSessionDescription & sdpIn = originalInvite->GetSDP();


  // look for audio sessions only, at this stage
  SDPMediaDescription::MediaType rtpMediaType = SDPMediaDescription::Audio;


  // if no matching media type, return FALSE
  SDPMediaDescription * incomingMedia = sdpIn.GetMediaDescription(rtpMediaType);
  if (incomingMedia == NULL) {
    PTRACE(2, "SIP\tCould not find matching media type");
    return FALSE;
  }

  // create the list of Opal format names for the remote end
  remoteFormatList = incomingMedia->GetMediaFormats();

  // find the payload type used for telephone-event, if present
  const SDPMediaFormatList & sdpMediaList = incomingMedia->GetSDPMediaFormats();

  BOOL hasTelephoneEvent = FALSE;
  for (i = 0; !hasTelephoneEvent && (i < sdpMediaList.GetSize()); i++) {
    if (sdpMediaList[i].GetEncodingName() == "telephone-event") {
      rfc2833Handler->SetPayloadType(sdpMediaList[i].GetPayloadType());
      hasTelephoneEvent = TRUE;
      remoteFormatList += OpalRFC2833;
    }
  }

  // create an RTP session
  RTP_UDP * rtpSession = (RTP_UDP *)UseSession(rtpSessionId);

  // set the remote addresses
  PIPSocket::Address ip;
  WORD port;
  incomingMedia->GetTransportAddress().GetIpAndPort(ip, port);
  if (!rtpSession->SetRemoteSocketInfo(ip, port, TRUE)) {
    PTRACE(1, "SIP\tCannot set remote ports on RTP session");
    delete rtpSession;
    return FALSE;
  }


  // look for matching codec in peer connection
  if (!GetCall().OpenSourceMediaStreams(remoteFormatList, rtpSessionId)) {
    PTRACE(2, "SIP\tCould not find compatible codecs");
    return FALSE;
  }

  // find out what media stream was opened
  // and get the payload type that was selected
  if (mediaStreams.GetSize() < 1) {
    PTRACE(2, "SIP\tNo media streams open");
    return FALSE;
  }

  // construct a new media session list with the selected format
  SDPMediaDescription * localMedia = new SDPMediaDescription(
                            GetLocalAddress(rtpSession->GetLocalDataPort()), rtpMediaType);
  
  for (i = 0; i < mediaStreams.GetSize(); i++) {
    OpalMediaStream & mediaStream = mediaStreams[i];
    localMedia->AddMediaFormat(mediaStream.GetMediaFormat());
    if (hasTelephoneEvent) {
      OpalMediaPatch * patch = mediaStream.GetPatch();
      if (patch != NULL) {
        if (mediaStream.IsSource())
          patch->AddFilter(rfc2833Handler->GetReceiveHandler(), mediaStream.GetMediaFormat());
        else
          patch->AddFilter(rfc2833Handler->GetTransmitHandler());
      }
    }
  }

  if (hasTelephoneEvent)
    localMedia->AddSDPMediaFormat(new SDPMediaFormat("0-15", rfc2833Handler->GetPayloadType()));

  sdpOut.AddMediaDescription(localMedia);

  // send the response
  SIP_PDU response(*originalInvite, SIP_PDU::Successful_OK);
  response.SetSDP(sdpOut);
  response.Write(*transport);
  currentPhase = ConnectedPhase;
  
  return TRUE;
}


OpalTransportAddress SIPConnection::GetLocalAddress(WORD port) const
{
  PIPSocket::Address localIP;
  if (!transport->GetLocalAddress().GetIpAddress(localIP)) {
    PTRACE(1, "SIP\tNot using IP transport");
    return OpalTransportAddress();
  }

  PIPSocket::Address remoteIP;
  if (!transport->GetRemoteAddress().GetIpAddress(remoteIP)) {
    PTRACE(1, "SIP\tNot using IP transport");
    return OpalTransportAddress();
  }

  endpoint.GetManager().TranslateIPAddress(localIP, remoteIP);
  return OpalTransportAddress(localIP, port, OpalTransportAddress::UdpPrefix);
}


RTP_Session * SIPConnection::UseSession(unsigned rtpSessionId)
{
  // make sure this RTP session does not already exist
  RTP_Session * oldSession = rtpSessions.UseSession(rtpSessionId);
  if (oldSession != NULL)
    return oldSession;

  // create an RTP session
  RTP_UDP * rtpSession = new RTP_UDP(rtpSessionId);

  // add the RTP session to the RTP session manager
  rtpSessions.AddSession(rtpSession);

  PIPSocket::Address ip;
  transport->GetLocalAddress().GetIpAddress(ip);

  // open an RTP session
  OpalManager & mgr = endpoint.GetManager();
  if (!rtpSession->Open(ip,
		        mgr.GetUDPPortBase(),
                        mgr.GetUDPPortMax(),
		        mgr.GetRtpIpTypeofService())) {
    PTRACE(1, "SIP\tCould not open RTP session");
    delete rtpSession;
    return NULL;
  }
 
  return rtpSession;
}


OpalMediaFormatList SIPConnection::GetMediaFormats() const
{
  return remoteFormatList;
}


OpalMediaStream * SIPConnection::CreateMediaStream(BOOL isSource, unsigned sessionID)
{
  if (ownerCall.IsMediaBypassPossible(*this, sessionID))
    return new OpalNullMediaStream(isSource, sessionID);

  if (rtpSessions.GetSession(sessionID) == NULL)
    return NULL;

  return new OpalRTPMediaStream(isSource, *rtpSessions.GetSession(sessionID));
}


BOOL SIPConnection::IsMediaBypassPossible(unsigned sessionID) const
{
  PTRACE(3, "SIP\tIsMediaBypassPossible: session " << sessionID);

  return sessionID == OpalMediaFormat::DefaultAudioSessionID ||
         sessionID == OpalMediaFormat::DefaultVideoSessionID;
}


BOOL SIPConnection::GetMediaInformation(unsigned sessionID,
                                        MediaInformation & info) const
{
  if (!mediaTransports.Contains(sessionID)) {
    PTRACE(3, "SIP\tGetMediaInformation for session " << sessionID << " - no channel.");
    return FALSE;
  }

  info.data = mediaTransports[sessionID];
  PIPSocket::Address ip;
  WORD port;
  if (info.data.GetIpAndPort(ip, port))
    info.control = OpalTransportAddress(ip, (WORD)(port+1));

  info.rfc2833 = rfc2833Handler->GetPayloadType();
  PTRACE(3, "SIP\tGetMediaInformation for session " << sessionID
         << " data=" << info.data << " rfc2833=" << info.rfc2833);
  return TRUE;
}


BOOL SIPConnection::WriteINVITE(OpalTransport & transport, PObject * param)
{
  SIPConnection & connection = *(SIPConnection *)param;

  connection.SetLocalPartyAddress();

  SIPTransaction * invite = new SIPTransaction(connection, transport, SIP_PDU::Method_INVITE);
  connection.invitations.Append(invite);

  return invite->Start();
}


void SIPConnection::InitiateCall(const SIPURL & destination)
{
  PTRACE(2, "SIP\tInitiating connection to " << destination);

  originalDestination = destination;
  originating = TRUE;

  PStringToString params = originalDestination.GetParamVars();
  if (params.Contains("proxyusername") && params.Contains("proxypassword")) {
    localPartyName = params["proxyusername"];
    authentication.SetUsername(localPartyName);
    originalDestination.SetParamVar("proxyusername", PString::Empty());
    authentication.SetPassword(params["proxypassword"]);
    originalDestination.SetParamVar("proxypassword", PString::Empty());
    PTRACE(3, "SIP\tExtracted proxy authentication user=\"" << localPartyName << '"');
  }

  remotePartyAddress = '<' + originalDestination.AsString() + '>';

  OpalTransportAddress address = originalDestination.GetHostAddress();

  delete transport;
  transport = address.CreateTransport(endpoint, OpalTransportAddress::NoBinding);
  if (transport == NULL)
    return;

  transport->SetBufferSize(SIP_PDU::MaxSize);
  transport->ConnectTo(address);

  transport->WriteConnect(WriteINVITE, this);

  releaseMethod = ReleaseWithCANCEL;
}


SDPSessionDescription * SIPConnection::BuildSDP()
{
  OpalTransportAddress localAddress;
  RTP_DataFrame::PayloadTypes ntePayloadCode = RTP_DataFrame::IllegalPayloadType;

  if (ownerCall.IsMediaBypassPossible(*this, OpalMediaFormat::DefaultAudioSessionID)) {
    OpalConnection * otherParty = GetCall().GetOtherPartyConnection(*this);
    if (otherParty != NULL) {
      MediaInformation info;
      if (otherParty->GetMediaInformation(OpalMediaFormat::DefaultAudioSessionID, info)) {
        localAddress = info.data;
        ntePayloadCode = info.rfc2833;
      }
    }
  }

  if (localAddress.IsEmpty()) {
    // create the SDP definition for the audio channel
    RTP_UDP * audioRTPSession = (RTP_UDP*)UseSession(OpalMediaFormat::DefaultAudioSessionID);
    localAddress = GetLocalAddress(audioRTPSession->GetLocalDataPort());
  }

  SDPSessionDescription * sdp = new SDPSessionDescription(localAddress);

  SDPMediaDescription * localMedia = new SDPMediaDescription(localAddress, SDPMediaDescription::Audio);

  // Set format if we have an RTP payload type for RFC2833
  if (ntePayloadCode != RTP_DataFrame::IllegalPayloadType) {
    PTRACE(3, "SIP\tUsing bypass RTP payload " << ntePayloadCode << " for NTE");
    localMedia->AddSDPMediaFormat(new SDPMediaFormat("0-15", ntePayloadCode));
  }
  else {
    ntePayloadCode = rfc2833Handler->GetPayloadType();
    if (ntePayloadCode == RTP_DataFrame::IllegalPayloadType) {
      ntePayloadCode = OpalRFC2833.GetPayloadType();
    }

    if (ntePayloadCode != RTP_DataFrame::IllegalPayloadType) {
      PTRACE(3, "SIP\tUsing RTP payload " << ntePayloadCode << " for NTE");

      // create and add the NTE media format
      localMedia->AddSDPMediaFormat(new SDPMediaFormat("0-15", ntePayloadCode));
    }
    else {
      PTRACE(2, "SIP\tCould not allocate dynamic RTP payload for NTE");
    }
  }

  rfc2833Handler->SetPayloadType(ntePayloadCode);


  // add the audio formats
  OpalMediaFormatList formats = ownerCall.GetMediaFormats(*this);
  localMedia->AddMediaFormats(formats);

  // add in SDP records
  sdp->AddMediaDescription(localMedia);

  return sdp;
}


void SIPConnection::SetLocalPartyAddress()
{
  SIPURL myAddress(GetLocalPartyName(), transport->GetLocalAddress(), endpoint.GetDefaultSignalPort());
  SetLocalPartyAddress(myAddress.AsString());
}


void SIPConnection::OnTransactionFailed(SIPTransaction & transaction)
{
  if (transaction.GetMethod() != SIP_PDU::Method_INVITE)
    return;

  for (PINDEX i = 0; i < invitations.GetSize(); i++) {
    if (!invitations[i].IsFailed())
      return;
  }

  // All invitations failed, die now
  Release(EndedByConnectFail);
}


void SIPConnection::OnReceivedPDU(SIP_PDU & pdu)
{
  SIPTransaction * transaction = transactions.GetAt(pdu.GetTransactionID());
  PTRACE(4, "SIP\tHandling PDU " << pdu << " (" <<
            (transaction != NULL ? "with" : "no") << " transaction)");

  switch (pdu.GetMethod()) {
    case SIP_PDU::Method_INVITE :
      OnReceivedINVITE(pdu);
      break;
    case SIP_PDU::Method_ACK :
      OnReceivedACK(pdu);
      break;
    case SIP_PDU::Method_CANCEL :
      OnReceivedCANCEL(pdu);
      break;
    case SIP_PDU::Method_BYE :
      OnReceivedBYE(pdu);
      break;
    case SIP_PDU::Method_OPTIONS :
      OnReceivedOPTIONS(pdu);
      break;
    case SIP_PDU::Method_REGISTER :
      // Shouldn't have got this!
      break;
    case SIP_PDU::NumMethods :  // if no method, must be response
      if (transaction != NULL)
        transaction->OnReceivedResponse(pdu);
      break;
  }
}


void SIPConnection::OnReceivedINVITE(SIP_PDU & request)
{
  // ignore duplicate INVITES
  if (originalInvite != NULL) {
    PTRACE(2, "SIP\tIgnoring duplicate INVITE from " << request.GetURI() << " for " << *this);
    return;
  }

  originalInvite = new SIP_PDU(request);
  releaseMethod = ReleaseWithResponse;

  SIPMIMEInfo & mime = originalInvite->GetMIME();
  remotePartyAddress = mime.GetFrom();
  localPartyAddress  = mime.GetTo() + ";tag=XXXXX";
  mime.SetTo(localPartyAddress);

  // indicate the other is to start ringing
  if (!OnIncomingConnection()) {
    PTRACE(2, "SIP\tOnIncomingConnection failed for INVITE from " << request.GetURI() << " for " << *this);
    Release();
    return;
  }

  PTRACE(2, "SIP\tOnIncomingConnection succeeded for INVITE from " << request.GetURI() << " for " << *this);
}


void SIPConnection::OnReceivedACK(SIP_PDU & /*request*/)
{
  PTRACE(2, "SIP\tACK received");

  // start all of the media threads for the connection
  StartMediaStreams();
  currentPhase = EstablishedPhase;
  releaseMethod = ReleaseWithBYE;
}


void SIPConnection::OnReceivedOPTIONS(SIP_PDU & /*request*/)
{
  PTRACE(1, "SIP\tOPTIONS not yet supported");
}


void SIPConnection::OnReceivedBYE(SIP_PDU & request)
{
  PTRACE(2, "SIP\tBYE received for call " << request.GetMIME().GetCallID());
  SIP_PDU response(request, SIP_PDU::Successful_OK);
  response.Write(*transport);
  releaseMethod = ReleaseWithNothing;
  Release(EndedByRemoteUser);
}


void SIPConnection::OnReceivedCANCEL(SIP_PDU & request)
{
  // Currently only handle CANCEL requests for the original INVITE that
  // created this connection, all else ignored
  if (originalInvite == NULL ||
      request.GetMIME().GetTo() != originalInvite->GetMIME().GetTo() ||
      request.GetMIME().GetFrom() != originalInvite->GetMIME().GetFrom() ||
      request.GetMIME().GetCSeqIndex() != originalInvite->GetMIME().GetCSeqIndex()) {
    PTRACE(1, "SIP\tUnattached " << request << " received for " << *this);
    SIP_PDU response(request, SIP_PDU::Failure_TransactionDoesNotExist);
    response.Write(*transport);
    return;
  }

  PTRACE(2, "SIP\tCancel received for " << *this);

  SIP_PDU response(request, SIP_PDU::Successful_OK);
  response.Write(*transport);
  Release(EndedByCallerAbort);
}


void SIPConnection::OnReceivedTrying(SIP_PDU & /*response*/)
{
  PTRACE(2, "SIP\tReceived Trying response");
}


void SIPConnection::OnReceivedSessionProgress(SIP_PDU & response)
{
  PTRACE(2, "SIP\tReceived Session Progress response");

  OnReceivedSDP(response);

  OnAlerting();
  currentPhase = AlertingPhase;

  for (PINDEX i = 0; i < mediaStreams.GetSize(); i++) {
    if (mediaStreams[i].IsSink())
      mediaStreams[i].Start();
  }
}


void SIPConnection::OnReceivedRedirection(SIP_PDU & response)
{
  // extract the contact field, and set that as the new Contact address
  PString newContact = response.GetMIME()("Contact");
  PTRACE(2, "SIP\tCall redirected to " << newContact);
  SIPURL newURL(newContact);
  transport->SetRemoteAddress(newURL.GetHostAddress());

  // set the remote party address
  remotePartyAddress = newContact;

  // send a new INVITE
  SIPTransaction request(*this, *transport, SIP_PDU::Method_INVITE);
  request.Start();
}


void SIPConnection::OnReceivedAuthenticationRequired(SIPTransaction & transaction,
                                                     SIP_PDU & response)
{
  BOOL isProxy = response.GetStatusCode() == SIP_PDU::Failure_ProxyAuthenticationRequired;

  if (transaction.GetMethod() != SIP_PDU::Method_INVITE) {
    PTRACE(1, "SIP\tCannot do " << (isProxy ? "Proxy " : "") << "Authentication Required for non INVITE");
    return;
  }

  PTRACE(2, "SIP\tReceived " << (isProxy ? "Proxy " : "") << "Authentication Required response");

  if (!authentication.Parse(response.GetMIME()(isProxy ? "Proxy-Authenticate"
                                                       : "WWW-Authenticate"),
                                               isProxy)) {
    Release(EndedBySecurityDenial);
    return;
  }

  // make sure the To field is the same as the original
  remotePartyAddress = originalDestination.AsString();

  // Restart the transaction with new authentication info
  SIPTransaction * invite = new SIPTransaction(*this, *transport, SIP_PDU::Method_INVITE);
  if (invite->Start())
    invitations.Append(invite);
  else {
    delete invite;
    PTRACE(1, "SIP\tCould not restart INVITE for " << (isProxy ? "Proxy " : "") << "Authentication Required");
  }
}


void SIPConnection::OnReceivedOK(SIPTransaction & transaction, SIP_PDU & response)
{
  PINDEX index = invitations.GetObjectsIndex(&transaction);
  if (index  == P_MAX_INDEX) {
    PTRACE(3, "SIP\tIgnoring OK response");
    return;
  }

  if (currentPhase == EstablishedPhase) {
    PTRACE(2, "SIP\tIgnoring OK response");
    return;
  }

  PTRACE(2, "SIP\tReceived OK response");

  OnReceivedSDP(response);

  OnConnected();

  currentPhase = EstablishedPhase;
  StartMediaStreams();
  releaseMethod = ReleaseWithBYE;
}


void SIPConnection::OnReceivedResponse(SIPTransaction & transaction, SIP_PDU & response)
{
  if (transaction.GetMethod() == SIP_PDU::Method_INVITE) {
    // Stop all the other invitations that went out
    for (PINDEX i = 0; i < invitations.GetSize(); i++) {
      if (&transaction != &invitations[i])
        invitations.RemoveAt(i--);
    }
    transaction.GetTransport().EndConnect(transaction.GetLocalAddress());
  }

  switch (response.GetStatusCode()) {
    case SIP_PDU::Information_Session_Progress :
      OnReceivedSessionProgress(response);
      break;

    case SIP_PDU::Failure_UnAuthorised :
    case SIP_PDU::Failure_ProxyAuthenticationRequired :
      OnReceivedAuthenticationRequired(transaction, response);
      break;

    case SIP_PDU::Redirection_MovedTemporarily :
      OnReceivedRedirection(response);
      break;

    case SIP_PDU::Failure_BusyHere :
      Release(EndedByRemoteBusy);
      break;

    default :
      switch (response.GetStatusCode()/100) {
        case 1 :
          // Do nothing on 1xx
          break;
        case 2 :
          OnReceivedOK(transaction, response);
          break;
        default :
          // All other final responses cause a call end.
          Release(EndedByRefusal);
      }
  }
}


void SIPConnection::OnReceivedSDP(SIP_PDU & pdu)
{
  if (!pdu.HasSDP())
    return;

  SDPSessionDescription & sdp = pdu.GetSDP();

  SDPMediaDescription * mediaDescription = sdp.GetMediaDescription(SDPMediaDescription::Audio);
  if (mediaDescription == NULL) {
    PTRACE(1, "SIP\tCould not find SDP media description for audio");
    return;
  }

  OpalTransportAddress address = mediaDescription->GetTransportAddress();
  if (ownerCall.IsMediaBypassPossible(*this, OpalMediaFormat::DefaultAudioSessionID))
    mediaTransports.SetAt(OpalMediaFormat::DefaultAudioSessionID, new OpalTransportAddress(address));
  else {
    PIPSocket::Address ip;
    WORD port;
    address.GetIpAndPort(ip, port);

    RTP_UDP * rtpSession = (RTP_UDP *)UseSession(OpalMediaFormat::DefaultAudioSessionID);
    if (!rtpSession->SetRemoteSocketInfo(ip, port, TRUE)) {
      PTRACE(1, "SIP\tCould not set RTP remote socket");
      return;
    }
  }

  remoteFormatList = mediaDescription->GetMediaFormats();
  remoteFormatList += OpalRFC2833;

  if (!ownerCall.OpenSourceMediaStreams(remoteFormatList, OpalMediaFormat::DefaultAudioSessionID)) {
    PTRACE(2, "SIP\tCould not open media streams for audio");
    return;
  }
}


void SIPConnection::QueuePDU(SIP_PDU * pdu)
{
  PAssertNULL(pdu);

  PTRACE(4, "SIP\tQueueing PDU: " << *pdu);
  pduQueue.Enqueue(pdu);
  pduSemaphore.Signal();

  if (pduHandler == NULL)
    pduHandler = PThread::Create(PCREATE_NOTIFIER(HandlePDUsThreadMain), 0,
                                 PThread::NoAutoDeleteThread,
                                 PThread::NormalPriority,
                                 "SIP Handler:%x");
}


void SIPConnection::HandlePDUsThreadMain(PThread &, INT)
{
  PTRACE(2, "SIP\tPDU handler thread started.");

  for(;;) {
    PTRACE(4, "SIP\tAwaiting next PDU.");
    pduSemaphore.Wait();

    if (!Lock())
      break;

    SIP_PDU * pdu = pduQueue.Dequeue();
    if (pdu != NULL) {
      OnReceivedPDU(*pdu);
      delete pdu;
    }

    Unlock();
  }

  PTRACE(2, "SIP\tPDU handler thread finished.");
}


void SIPConnection::SendResponseToINVITE(SIP_PDU::StatusCodes code, const char * extra)
{
  if (originalInvite != NULL) {
    SIP_PDU response(*originalInvite, code, extra);
    response.Write(*transport);
  }
}


// End of file ////////////////////////////////////////////////////////////////
