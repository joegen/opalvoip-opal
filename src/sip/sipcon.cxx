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
 * Revision 1.2009  2002/03/18 08:13:42  robertj
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
#include <ptclib/cypher.h>

#ifdef __GNUC__
#pragma implementation "sipcon.h"
#endif

#include <sip/sipcon.h>

#include <sip/sipep.h>
#include <codec/rfc2833.h>
#include <opal/manager.h>
#include <opal/call.h>
#include <opal/patch.h>


static struct {
  const char        * method;
  SIPMethodFunction function;
  BOOL              cseqSameAsInvite;
} sipMethods[] = {
  { "INVITE",     &SIPConnection::OnReceivedINVITE,   TRUE  },
  { "ACK",        &SIPConnection::OnReceivedACK,      TRUE  },
  { "CANCEL",     &SIPConnection::OnReceivedCANCEL,   TRUE  },
  { "BYE",        &SIPConnection::OnReceivedBYE,      FALSE },
  { "OPTIONS",    &SIPConnection::OnReceivedOPTIONS,  FALSE },
  { "REGISTER",   &SIPConnection::OnReceivedREGISTER, FALSE }
};

static struct {
  SIP_PDU::StatusCodes code;
  SIPMethodFunction    function;
} sipResponses[] = {
  { SIP_PDU::Information_Trying,                  &SIPConnection::OnReceivedTrying },
  { SIP_PDU::Information_Session_Progress,        &SIPConnection::OnReceivedSessionProgress },
  { SIP_PDU::Failure_UnAuthorised,                &SIPConnection::OnReceivedAuthenticationRequired },
  { SIP_PDU::Failure_ProxyAuthenticationRequired, &SIPConnection::OnReceivedAuthenticationRequired },
  { SIP_PDU::Redirection_MovedTemporarily,        &SIPConnection::OnReceivedRedirection },
  { SIP_PDU::Successful_OK,                       &SIPConnection::OnReceivedOK }
};


#define new PNEW


////////////////////////////////////////////////////////////////////////////

SIPConnection::SIPConnection(OpalCall & call,
                             SIPEndPoint & ep,
                             const PString & token,
                             SIP_PDU * invite)
  : OpalConnection(call, ep, token),
    endpoint(ep),
    password(endpoint.GetRegistrationPassword())
{
  sendUserInputMode = SendUserInputAsInlineRFC2833;

  currentPhase = SetUpPhase;

  localPartyName = endpoint.GetRegistrationName();

  if (invite != NULL) {
    transport = invite->GetTransport().GetLocalAddress().CreateTransport(
                                  endpoint, OpalTransportAddress::HostOnly);
    PString contact = invite->GetMIME().GetContact();
    if (contact.IsEmpty())
      transport->SetRemoteAddress(invite->GetTransport().GetRemoteAddress());
    else {
      SIPURL remote = contact;
      transport->SetRemoteAddress(remote.GetHostAddress());
    }
    transport->SetBufferSize(MAX_SIP_UDP_PDU_SIZE);

    originalInvite = new SIP_PDU(*transport, this);
    originalInvite->GetMIME() = invite->GetMIME();
    if (invite->HasSDP())
      originalInvite->SetSDP(invite->GetSDP());

    inviteCSeq         = invite->GetMIME().GetCSeqIndex();
    remotePartyAddress = invite->GetMIME().GetFrom();
    localPartyAddress = invite->GetMIME().GetTo();
  }
  else {
    originalInvite = NULL;
    transport = NULL;
    //expectedCseq = 0;
  }

  lastSentCseq = 1;               // initial CSeq for outgoing PDUs
  haveLastReceivedCseq = FALSE;   // not yet received CSeq for incoming PDUs

  sendBYE = TRUE;

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

  LockOnRelease();

  // send the appropriate form 
  PTRACE(2, "SIP\tReceived OnReleased in phase " << currentPhase);
  switch (currentPhase) {
    case SetUpPhase:
      SendResponse(SIP_PDU::Failure_NotFound);
      break;

    case AlertingPhase:
      SendResponse(SIP_PDU::Failure_BusyHere);
      break;

    default:
      if (sendBYE) {
        SIP_PDU bye(*transport, this);
        bye.BuildBYE();
        AddAuthorisation(bye);
        bye.Write();
      }
      break;
  }

  if (transport != NULL)
    transport->CloseWait();

  return OpalConnection::OnReleased();
}


BOOL SIPConnection::SetAlerting(const PString & /*calleeName*/, BOOL /*withMedia*/)
{
  PTRACE(2, "SIP\tSetAlerting");
  if (!Lock())
    return FALSE;

  if (currentPhase != SetUpPhase) 
    return FALSE;

  SendResponse(SIP_PDU::Information_Ringing);
  currentPhase = AlertingPhase;

  Unlock();

  return TRUE;
}


BOOL SIPConnection::SetConnected()
{
  PINDEX i;
  PTRACE(2, "SIP\tSetConnected");

  // create an RTP session number
  // for now, just hard code it
  unsigned rtpSessionId = OpalMediaFormat::DefaultAudioSessionID;

  // look for audio sessions only, at this stage
  SDPMediaDescription::MediaType rtpMediaType = SDPMediaDescription::Audio;

  // get the remote media formats
  SDPSessionDescription & sdpIn = originalInvite->GetSDP();

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

  SDPSessionDescription sdpOut(GetLocalAddress());

  // construct a new media session list with the selected format
  SDPMediaDescription * localMedia =
            new SDPMediaDescription(sdpOut.GetDefaultConnectAddress(), rtpMediaType);
  
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
  response.Write();
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


BOOL SIPConnection::SendConnectionAlert(const PString & calleeName)
{
  currentPhase = AlertingPhase;
  remotePartyName = calleeName;
  return TRUE;
}


void SIPConnection::InitiateCall(const SIPURL & destination)
{
  PTRACE(2, "SIP\tInitiating connection to " << destination);

  originalDestination = destination;

  PStringToString params = originalDestination.GetParamVars();
  if (params.Contains("proxyusername") && params.Contains("proxypassword")) {
    localPartyName = params["proxyusername"];
    originalDestination.SetParamVar("proxyusername", PString::Empty());
    password = params["proxypassword"];
    originalDestination.SetParamVar("proxypassword", PString::Empty());
    PTRACE(3, "SIP\tExtracted proxy authentication user=\"" << localPartyName << '"');
  }

  remotePartyAddress = '<' + originalDestination.AsString() + '>';

  OpalTransportAddress address = originalDestination.GetHostAddress();
  transport = address.CreateTransport(endpoint, OpalTransportAddress::NoBinding);
  if (transport == NULL)
    return;

  transport->SetBufferSize(MAX_SIP_UDP_PDU_SIZE);
  transport->ConnectTo(address);

  transport->AttachThread(PThread::Create(PCREATE_NOTIFIER(InitiateCallThreadMain), 0,
                                          PThread::NoAutoDeleteThread,
                                          PThread::NormalPriority,
                                          "SIP Call:%x"));
}


static BOOL WriteINVITE(OpalTransport & transport, PObject * param)
{
  SIPConnection & connection = *(SIPConnection *)param;

  connection.SetLocalPartyAddress();

  SIP_PDU request(transport, &connection);
  request.BuildINVITE();

  return request.Write();
}


void SIPConnection::InitiateCallThreadMain(PThread &, INT)
{
  PTRACE(3, "SIP\tStarted Initiate Call thread.");

  transport->WriteConnect(WriteINVITE, this);

  SIP_PDU response(*transport);
  transport->SetReadTimeout(15000);
  if (response.Read()) {
    transport->SetReadTimeout(PMaxTimeInterval);
    transport->EndConnect(transport->GetLocalAddress());
  
    PTRACE(2, "SIP\tSelecting reply from " << *transport);
    SetLocalPartyAddress();

    endpoint.OnReceivedPDU(response);

    while (transport->IsOpen())
      endpoint.HandlePDU(*transport);
  }
  else
    Release(EndedByConnectFail);

  PTRACE(3, "SIP\tStarted Initiate Call thread.");
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


void SIPConnection::HandlePDUsThreadMain(PThread &, INT)
{
  PTRACE(2, "SIP\tAnswer thread started.");

  while (transport->IsOpen())
    endpoint.HandlePDU(*transport);

  PTRACE(2, "SIP\tAnswer thread finished.");
}


BOOL SIPConnection::OnReceivedPDU(SIP_PDU & pdu)
{
  PINDEX i;
  SIPMIMEInfo & mime = pdu.GetMIME();
  PString callId     = mime.GetCallID();

  PString method = pdu.GetMethod();

  // if no method, must be response
  if (method.IsEmpty()) {
    PTRACE(4, "SIP\tChecking request CSeq " << mime.GetCSeqIndex() << " against expected CSeq " << lastSentCseq);
    if (mime.GetCSeqIndex() != (PINDEX)lastSentCseq)
      PTRACE(3, "SIP\tIgnoring response " << pdu.GetStatusCode() << " for callId " << mime.GetCallID() << " with CSeq " << mime.GetCSeq());
    else {
      remotePartyAddress = pdu.GetMIME().GetTo();
      for (i = 0; i < PARRAYSIZE(sipResponses); i++) {
        if (pdu.GetStatusCode() == sipResponses[i].code) {
          (this->*sipResponses[i].function)(pdu);
          return TRUE;
        }
      }
      OnReceivedResponse(pdu);
    }
    return TRUE;
  }

  // ignore duplicate INVITES
  if (method *= "INVITE") {
    PTRACE(4, "SIP\tIgnoring duplicate INVITE");
    return TRUE;
  }

  // process other commands
  for (i = 0; i < PARRAYSIZE(sipMethods); i++) {
    if (method *= sipMethods[i].method) {

      PINDEX expectedCseq;
      if (sipMethods[i].cseqSameAsInvite) 
        expectedCseq = inviteCSeq;
      else if (haveLastReceivedCseq)
        expectedCseq = lastReceivedCseq + 1;
      else {
        expectedCseq = lastReceivedCseq = mime.GetCSeqIndex();
        haveLastReceivedCseq = TRUE;
      }

      // check the CSeq
      if (mime.GetCSeqIndex() != expectedCseq)
        PTRACE(3, "SIP\tIgnoring " << method << " for callId " << mime.GetCallID() << " with unexpected CSeq " << mime.GetCSeq());
      else {
        PTRACE(3, "SIP\tAccepting " << method << " for callId " << mime.GetCallID() << " with CSeq " << expectedCseq);

        // process the command
        PTRACE(3, "SIP\tHandling " << method << " for callId " << mime.GetCallID());
        pdu.SetConnection(this);
        (this->*sipMethods[i].function)(pdu);
        lastReceivedCseq++;
      }
      return TRUE;
    }
  }

  return FALSE;
}


void SIPConnection::OnReceivedINVITE(SIP_PDU & request)
{
  if (transport->IsRunning()) {
    PTRACE(2, "SIP\tIgnoring duplicate INVITE from " << request.GetURI() << " for " << request.GetMIME().GetTo());
    return;
  }

  transport->AttachThread(PThread::Create(PCREATE_NOTIFIER(HandlePDUsThreadMain), 0,
                                          PThread::NoAutoDeleteThread,
                                          PThread::NormalPriority,
                                          "SIP Answer:%x"));
}


void SIPConnection::OnReceivedACK(SIP_PDU & /*request*/)
{
  PTRACE(2, "SIP\tACK received");

  // start all of the media threads for the connection
  StartMediaStreams();
  currentPhase = EstablishedPhase;
}


void SIPConnection::OnReceivedOPTIONS(SIP_PDU & /*request*/)
{
  PTRACE(1, "SIP\tOPTIONS not yet supported");
}


void SIPConnection::OnReceivedBYE(SIP_PDU & request)
{
  PTRACE(2, "SIP\tBYE received for call " << request.GetMIME().GetCallID());
  request.SendResponse(SIP_PDU::Successful_OK);
  Release(EndedByRemoteUser);
  sendBYE = FALSE;
}


void SIPConnection::OnReceivedCANCEL(SIP_PDU & request)
{
  PTRACE(2, "SIP\tCancel received for call " << request.GetMIME().GetCallID());

  request.SendResponse(SIP_PDU::Successful_OK);
  Release(EndedByRemoteUser);
  sendBYE = FALSE;
}


void SIPConnection::OnReceivedREGISTER(SIP_PDU & /*request*/)
{
  PTRACE(1, "SIP\tREGISTER not yet supported");
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


static PString GetAuthParam(const PString & auth, const char * name)
{
  PString value;

  PINDEX pos = auth.Find(name);
  if (pos != P_MAX_INDEX)  {
    pos += strlen(name);
    while (isspace(auth[pos]))
      pos++;
    if (auth[pos] == '=') {
      pos++;
      while (isspace(auth[pos]))
        pos++;
      if (auth[pos] == '"') {
        pos++;
        value = auth(pos, auth.Find('"', pos)-1);
      }
      else {
        PINDEX base = pos;
        while (auth[pos] != '\0' && !isspace(auth[pos]))
          pos++;
        value = auth(base, pos-1);
      }
    }
  }

  return value;
}

void SIPConnection::OnReceivedRedirection(SIP_PDU & response)
{
  // HACK: write an ack with the old CSeq
  {
    SIP_PDU ack(*transport, this);
    ack.BuildACK();
    ack.Write();
  }

  // extract the contact field, and set that as the new Contact address
  PString newContact = response.GetMIME()("Contact");
  PTRACE(2, "SIP\tCall redirected to " << newContact);
  SIPURL newURL(newContact);
  transport->SetRemoteAddress(newURL.GetHostAddress());

  // set the remote party address
  remotePartyAddress = newContact;

  // send a new INVITE
  SIP_PDU request(*transport, this);
  request.BuildINVITE();
  request.Write();
}


void SIPConnection::OnReceivedAuthenticationRequired(SIP_PDU & response)
{
  // HACK: write an ack with the old CSeq
  {
    SIP_PDU ack(*transport, this);
    ack.BuildACK();
    ack.Write();
  }

  lastSentCseq++;

  isProxyAuthenticate = response.GetStatusCode() == SIP_PDU::Failure_ProxyAuthenticationRequired;

  PTRACE(2, "SIP\tReceived " << (isProxyAuthenticate ? "Proxy " : "") << "Authentication Required response");

  PCaselessString auth;
  if (isProxyAuthenticate)
    auth = response.GetMIME()("Proxy-Authenticate");
  else
    auth = response.GetMIME()("WWW-Authenticate");

  if (!realm.IsEmpty()) {
    PTRACE(1, "SIP\tINVITE authentication failed");
    Release(EndedBySecurityDenial);
    return;
  }

  if (auth.Find("digest") != 0) {
    PTRACE(1, "SIP\tUnknown proxy authentication scheme");
    Release(EndedBySecurityDenial);
    return;
  }

  PCaselessString algorithm = GetAuthParam(auth, "algorithm");
  if (!algorithm && algorithm != "md5") {
    PTRACE(1, "SIP\tUnknown proxy authentication algorithm");
    Release(EndedBySecurityDenial);
    return;
  }

  realm = GetAuthParam(auth, "realm");
  if (realm.IsEmpty()) {
    PTRACE(1, "SIP\tNo realm in proxy authentication");
    Release(EndedBySecurityDenial);
    return;
  }

  nonce = GetAuthParam(auth, "nonce");
  if (nonce.IsEmpty()) {
    PTRACE(1, "SIP\tNo nonce in proxy authentication");
    Release(EndedBySecurityDenial);
    return;
  }

  // make sure the To field is the same as the original
  remotePartyAddress = '<' + originalDestination.AsString() + '>';

  // build the PDU and add authorisation
  SIP_PDU request(*transport, this);

  request.BuildINVITE();
  AddAuthorisation(request);
  request.Write();
}

static PString AsHex(PMessageDigest5::Code & digest)
{
  PStringStream out;
  out << hex << setfill('0');
  for (PINDEX i = 0; i < 16; i++)
    out << setw(2) << (unsigned)((BYTE *)&digest)[i];
  return out;
}

void SIPConnection::AddAuthorisation(SIP_PDU & pdu)
{ 
  if (realm.IsEmpty()) {
    PTRACE(1, "SIP\tNo authentication information available");
    return;
  }

  PTRACE(1, "SIP\tAdding authentication information");

  SIPMIMEInfo & mime = pdu.GetMIME();
  PString method   = pdu.GetMethod();
  PString username = GetLocalPartyName();
  SIPURL uri       = pdu.GetURI();

  PMessageDigest5 digestor;
  PMessageDigest5::Code a1, a2, response;

  PStringStream uriText; uriText << uri;
  PINDEX pos = uriText.Find(";");
  if (pos != P_MAX_INDEX)
    uriText = uriText.Left(pos);

  digestor.Start();
  digestor.Process(username);
  digestor.Process(":");
  digestor.Process(realm);
  digestor.Process(":");
  digestor.Process(password);
  digestor.Complete(a1);

  digestor.Start();
  digestor.Process(method);
  digestor.Process(":");
  digestor.Process(uriText);
  digestor.Complete(a2);

  digestor.Start();
  digestor.Process(AsHex(a1));
  digestor.Process(":");
  digestor.Process(nonce);
  digestor.Process(":");
  digestor.Process(AsHex(a2));
  digestor.Complete(response);

  PStringStream auth;
  auth << "Digest "
          "username=\"" << username << "\", "
          "realm=\"" << realm << "\", "
          "nonce=\"" << nonce << "\", "
          "uri=\"" << uriText << "\", "
          "response=\"" << AsHex(response) << "\", "
          "algorithm=md5";

  if (isProxyAuthenticate)
    mime.SetAt("Proxy-Authorization", auth);

  else
    mime.SetAt("Authorization", auth);
}


void SIPConnection::OnReceivedOK(SIP_PDU & response)
{
  if (currentPhase == EstablishedPhase) {
    PTRACE(2, "SIP\tIgnoring OK response");
    return;
  }

  PTRACE(2, "SIP\tReceived OK response");

  OnReceivedSDP(response);

  SIP_PDU ack(*transport, this);
  ack.BuildACK();
  ack.Write();
  lastSentCseq++;

  OnConnected();

  currentPhase = EstablishedPhase;
  StartMediaStreams();
}


void SIPConnection::OnReceivedResponse(SIP_PDU & response)
{
  OnReceivedSDP(response);

  if (response.GetStatusCode()/100 < 3)
    return;

  switch (response.GetStatusCode()) {
    case SIP_PDU::Failure_BusyHere :
      Release(EndedByRemoteBusy);
      break;

    default :
      Release(EndedByRefusal);
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


void SIPConnection::SendResponse(SIP_PDU::StatusCodes code, const char * extra)
{
  if (originalInvite != NULL)
    originalInvite->SendResponse(code, extra);
}


// End of file ////////////////////////////////////////////////////////////////
