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
 * Revision 1.2072  2005/06/04 12:44:36  dsandras
 * Applied patch from Ted Szoczei to fix leaks and problems on cancelling a call and to improve the Allow PDU field handling.
 *
 * Revision 2.70  2005/05/25 18:34:25  dsandras
 * Added missing AdjustMediaFormats so that only enabled common codecs are used on outgoing calls.
 *
 * Revision 2.69  2005/05/23 20:55:15  dsandras
 * Use STUN on incoming calls if required so that all the PDU fields are setup appropriately.
 *
 * Revision 2.68  2005/05/18 17:26:39  dsandras
 * Added back proxy support for INVITE.
 *
 * Revision 2.67  2005/05/16 14:40:32  dsandras
 * Make the connection fail when there is no authentication information present
 * and authentication is required.
 *
 * Revision 2.66  2005/05/12 19:47:29  dsandras
 * Fixed indentation.
 *
 * Revision 2.65  2005/05/06 07:37:06  csoutheren
 * Various changed while working with SIP carrier
 *   - remove assumption that authentication realm is a domain name.
 *   - stopped rewrite of "To" field when proxy being used
 *   - fix Contact field in REGISTER to match actual port used when Symmetric NATin use
 *   - lots of formatting changes and cleanups
 *
 * Revision 2.64  2005/05/04 17:09:40  dsandras
 * Re-Invite happens during the "EstablishedPhase". Ignore duplicate INVITEs
 * due to retransmission.
 *
 * Revision 2.63  2005/05/02 21:31:54  dsandras
 * Reinvite only if connectedPhase.
 *
 * Revision 2.62  2005/05/02 21:23:22  dsandras
 * Set default contact port to the first listener port.
 *
 * Revision 2.61  2005/05/02 20:43:03  dsandras
 * Remove the via parameters when updating to the via address in the transport.
 *
 * Revision 2.60  2005/04/28 20:22:54  dsandras
 * Applied big sanity patch for SIP thanks to Ted Szoczei <tszoczei@microtronix.ca>.
 * Thanks a lot!
 *
 * Revision 2.59  2005/04/28 07:59:37  dsandras
 * Applied patch from Ted Szoczei to fix problem when answering to PDUs containing
 * multiple Via fields in the message header. Thanks!
 *
 * Revision 2.58  2005/04/18 17:07:17  dsandras
 * Fixed cut and paste error in last commit thanks to Ted Szoczei.
 *
 * Revision 2.57  2005/04/16 18:57:36  dsandras
 * Use a TO header without tag when restarting an INVITE.
 *
 * Revision 2.56  2005/04/11 11:12:38  dsandras
 * Added Method_MESSAGE support for future use.
 *
 * Revision 2.55  2005/04/11 10:23:58  dsandras
 * 1) Added support for SIP ReINVITE without codec changes.
 * 2) Added support for GetRemotePartyCallbackURL.
 * 3) Added support for call hold (local and remote).
 * 4) Fixed missing tag problem when sending BYE requests.
 * 5) Added support for Blind Transfer (without support for being transfered).
 *
 * Revision 2.54  2005/03/11 18:12:09  dsandras
 * Added support to specify the realm when registering. That way softphones already know what authentication information to use when required. The realm/domain can also be used in the From field.
 *
 * Revision 2.53  2005/02/19 22:48:48  dsandras
 * Added the possibility to register to several registrars and be able to do authenticated calls to each of them. Added SUBSCRIBE/NOTIFY support for Message Waiting Indications.
 *
 * Revision 2.52  2005/02/19 22:26:09  dsandras
 * Ignore TO tag added by OPAL. Reported by Nick Noath.
 *
 * Revision 2.51  2005/01/16 11:28:05  csoutheren
 * Added GetIdentifier virtual function to OpalConnection, and changed H323
 * and SIP descendants to use this function. This allows an application to
 * obtain a GUID for any connection regardless of the protocol used
 *
 * Revision 2.50  2005/01/15 19:42:33  csoutheren
 * Added temporary workaround for deadlock that occurs with incoming calls
 *
 * Revision 2.49  2005/01/09 03:42:46  rjongbloed
 * Fixed warning about unused parameter
 *
 * Revision 2.48  2004/12/25 20:43:42  dsandras
 * Attach the RFC2833 handlers when we are in connected state to ensure
 * OpalMediaPatch exist. Fixes problem for DTMF sending.
 *
 * Revision 2.47  2004/12/23 20:43:27  dsandras
 * Only start the media streams if we are in the phase connectedPhase.
 *
 * Revision 2.46  2004/12/22 18:55:08  dsandras
 * Added support for Call Forwarding the "302 Moved Temporarily" SIP response.
 *
 * Revision 2.45  2004/12/18 03:54:43  rjongbloed
 * Added missing call of callback virtual for SIP 100 Trying response
 *
 * Revision 2.44  2004/12/12 13:40:45  dsandras
 * - Correctly update the remote party name, address and applications at various strategic places.
 * - If no outbound proxy authentication is provided, then use the registrar authentication parameters when proxy authentication is required.
 * - Fixed use of connectedTime.
 * - Correctly set the displayName in the FROM field for outgoing connections.
 * - Added 2 "EndedBy" cases when a connection ends.
 *
 * Revision 2.43  2004/11/29 08:18:32  csoutheren
 * Added support for setting the SIP authentication domain/realm as needed for many service
 *  providers
 *
 * Revision 2.42  2004/08/20 12:13:32  rjongbloed
 * Added correct handling of SIP 180 response
 *
 * Revision 2.41  2004/08/14 07:56:43  rjongbloed
 * Major revision to utilise the PSafeCollection classes for the connections and calls.
 *
 * Revision 2.40  2004/04/26 05:40:39  rjongbloed
 * Added RTP statistics callback to SIP
 *
 * Revision 2.39  2004/04/25 08:34:08  rjongbloed
 * Fixed various GCC 3.4 warnings
 *
 * Revision 2.38  2004/03/16 12:03:33  rjongbloed
 * Fixed poropagating port of proxy to connection target address.
 *
 * Revision 2.37  2004/03/15 12:33:48  rjongbloed
 * Fixed proxy override in URL
 *
 * Revision 2.36  2004/03/14 11:32:20  rjongbloed
 * Changes to better support SIP proxies.
 *
 * Revision 2.35  2004/03/14 10:09:54  rjongbloed
 * Moved transport on SIP top be constructed by endpoint as any transport created on
 *   an endpoint can receive data for any connection.
 *
 * Revision 2.34  2004/03/13 06:29:27  rjongbloed
 * Slight rearrangement of local party name and alias list to beter match common
 *   behaviour in ancestor.
 * Abstracted local party name for endpoint into ancestor from H.,323.
 * Changed parameter in UDP write function to void * from PObject *.
 *
 * Revision 2.33  2004/02/24 11:33:47  rjongbloed
 * Normalised RTP session management across protocols
 * Added support for NAT (via STUN)
 *
 * Revision 2.32  2004/02/15 03:12:10  rjongbloed
 * More patches from Ted Szoczei, fixed shutting down of RTP session if INVITE fails. Added correct return if no meda formats can be matched.
 *
 * Revision 2.31  2004/02/14 22:52:51  csoutheren
 * Re-ordered initialisations to remove warning on Linux
 *
 * Revision 2.30  2004/02/07 02:23:21  rjongbloed
 * Changed to allow opening of more than just audio streams.
 * Assured that symmetric codecs are used as required by "basic" SIP.
 * Made media format list subject to adjust \\ment by remoe/order lists.
 *
 * Revision 2.29  2004/01/08 22:23:23  csoutheren
 * Fixed problem with not using session ID when constructing SDP lists
 *
 * Revision 2.28  2003/12/20 12:21:18  rjongbloed
 * Applied more enhancements, thank you very much Ted Szoczei
 *
 * Revision 2.27  2003/12/15 11:56:17  rjongbloed
 * Applied numerous bug fixes, thank you very much Ted Szoczei
 *
 * Revision 2.26  2003/03/17 10:27:00  robertj
 * Added video support.
 *
 * Revision 2.25  2003/03/07 05:52:35  robertj
 * Made sure connection is locked with all function calls that are across
 *   the "call" object.
 *
 * Revision 2.24  2003/03/06 03:57:47  robertj
 * IVR support (work in progress) requiring large changes everywhere.
 *
 * Revision 2.23  2002/11/10 11:33:20  robertj
 * Updated to OpenH323 v1.10.3
 *
 * Revision 2.22  2002/09/12 06:58:34  robertj
 * Removed protocol prefix strings as static members as has problems with
 *   use in DLL environment.
 *
 * Revision 2.21  2002/07/01 04:56:33  robertj
 * Updated to OpenH323 v1.9.1
 *
 * Revision 2.20  2002/06/16 02:26:37  robertj
 * Fixed creation of RTP session for incoming calls, thanks Ted Szoczei
 *
 * Revision 2.19  2002/04/17 07:22:54  robertj
 * Added identifier for conenction in OnReleased trace message.
 *
 * Revision 2.18  2002/04/16 09:04:18  robertj
 * Fixed setting of target URI from Contact regardless of route set
 *
 * Revision 2.17  2002/04/16 07:53:15  robertj
 * Changes to support calls through proxies.
 *
 * Revision 2.16  2002/04/15 08:53:58  robertj
 * Fixed correct setting of jitter buffer size in RTP media stream.
 * Fixed starting incoming (not outgoing!) media on call progress.
 *
 * Revision 2.15  2002/04/10 06:58:31  robertj
 * Fixed incorrect return value when starting INVITE transactions.
 *
 * Revision 2.14  2002/04/10 03:14:35  robertj
 * Moved code for handling media bypass address resolution into ancestor as
 *   now done ths same way in both SIP and H.323.
 * Major changes to RTP session management when initiating an INVITE.
 * Improvements in error handling and transaction cancelling.
 *
 * Revision 2.13  2002/04/09 07:26:20  robertj
 * Added correct error return if get transport failure to remote endpoint.
 *
 * Revision 2.12  2002/04/09 01:19:41  robertj
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
#include <ptclib/random.h>              // for local dialog tag


typedef void (SIPConnection::* SIPMethodFunction)(SIP_PDU & pdu);


#define new PNEW


////////////////////////////////////////////////////////////////////////////

SIPConnection::SIPConnection(OpalCall & call,
                             SIPEndPoint & ep,
                             const PString & token,
                             const SIPURL & destination,
                             OpalTransport * inviteTransport)
  : OpalConnection(call, ep, token),
    endpoint(ep),
    pduSemaphore(0, P_MAX_INDEX)
{
  targetAddress = destination;

  // Look for a "proxy" parameter to override default proxy
  PStringToString params = targetAddress.GetParamVars();
  SIPURL proxy;
  if (params.Contains("proxy")) {
    proxy.Parse(params("proxy"));
    targetAddress.SetParamVar("proxy", PString::Empty());
  }

  remotePartyAddress = targetAddress.AsQuotedString();
  SIPURL url(remotePartyAddress);
  remotePartyName = url.GetDisplayName ();

  if (proxy.IsEmpty())
    proxy = endpoint.GetProxy();

  // Proxy parameters. 
  if (!proxy.IsEmpty()) {
    targetAddress = proxy.GetScheme() + ':' + proxy.GetHostName() + ':' + PString(proxy.GetPort());
  }
  
  if (inviteTransport == NULL)
    transport = NULL;
  else {
    OpalManager & manager = endpoint.GetManager();
    PSTUNClient * stun = manager.GetSTUN(targetAddress.GetHostAddress());
    if (stun != NULL) 
      transport = endpoint.CreateTransport(targetAddress.GetHostAddress());
    else
      transport = inviteTransport->GetLocalAddress().CreateTransport(endpoint, OpalTransportAddress::HostOnly);
    transport->SetBufferSize(SIP_PDU::MaxSize); // Maximum possible PDU size
  }

  originalInvite = NULL;
  pduHandler = NULL;
  lastSentCSeq = 0;
  releaseMethod = ReleaseWithNothing;

  transactions.DisallowDeleteObjects();

  referTransaction = NULL;
  local_hold = FALSE;
  remote_hold = FALSE;

  PTRACE(3, "SIP\tCreated connection.");
}


SIPConnection::~SIPConnection()
{
  delete originalInvite;
  delete transport;

  PTRACE(3, "SIP\tDeleted connection.");
}


void SIPConnection::OnReleased()
{
  PTRACE(3, "SIP\tOnReleased: " << *this);

  SIPTransaction * byeTransaction = NULL;

  switch (releaseMethod) {
    case ReleaseWithNothing :
      break;

    case ReleaseWithResponse :
      switch (callEndReason) {
        case EndedByAnswerDenied :
          SendResponseToINVITE(SIP_PDU::GlobalFailure_Decline);
          break;

        case EndedByLocalBusy :
          SendResponseToINVITE(SIP_PDU::Failure_BusyHere);
          break;

        case EndedByCallerAbort :
          SendResponseToINVITE(SIP_PDU::Failure_RequestTerminated);
          break;

        // call ended by no codec match or stream open failure
        case EndedByCapabilityExchange :
          SendResponseToINVITE(SIP_PDU::Failure_UnsupportedMediaType);
          break;

        case EndedByCallForwarded :
          SendResponseToINVITE(SIP_PDU::Redirection_MovedTemporarily, forwardParty);
          break;

        default :
          SendResponseToINVITE(SIP_PDU::Failure_BadGateway);
      }
      break;

    case ReleaseWithBYE :
      // create BYE now & delete it later to prevent memory access errors
      byeTransaction = new SIPTransaction(*this, *transport, SIP_PDU::Method_BYE);
      break;

    case ReleaseWithCANCEL :
      for (PINDEX i = 0; i < invitations.GetSize(); i++) {
        if (invitations[i].SendCANCEL())
          invitations[i].Wait();
      }
  }

  // close media
  for (PINDEX i = 0; i < mediaStreams.GetSize(); i++)
    mediaStreams[i].Close();

  invitations.RemoveAll();

  // Sent a BYE, wait for it to complete
  if (byeTransaction != NULL) {
    byeTransaction->Wait();
    delete byeTransaction;
  }

  phase = ReleasedPhase;

  if (pduHandler != NULL) {
    pduSemaphore.Signal();
    pduHandler->WaitForTermination();
    delete pduHandler;
    pduHandler = NULL;  // clear pointer to deleted object
  }

  if (transport != NULL)
    transport->CloseWait();

  OpalConnection::OnReleased();
}

void SIPConnection::TransferConnection(const PString & remoteParty, const PString & /*callIdentity*/)
{
  if (referTransaction != NULL) 
    /* There is still an ongoing REFER transaction */
    return;
 
  referTransaction = 
    new SIPRefer(*this, *transport, remoteParty);
  referTransaction->Start ();
}

BOOL SIPConnection::SetAlerting(const PString & /*calleeName*/, BOOL /*withMedia*/)
{
  if (IsOriginating()) {
    PTRACE(2, "SIP\tSetAlerting ignored on call we originated.");
    return TRUE;
  }

  PTRACE(2, "SIP\tSetAlerting");

  if (phase != SetUpPhase) 
    return FALSE;

  SendResponseToINVITE(SIP_PDU::Information_Ringing);
  phase = AlertingPhase;

  return TRUE;
}


BOOL SIPConnection::SetConnected()
{
  if (IsOriginating()) {
    PTRACE(2, "SIP\tSetConnected ignored on call we originated.");
    return TRUE;
  }

  PTRACE(2, "SIP\tSetConnected");

  SDPSessionDescription sdpOut(GetLocalAddress());

  // get the remote media formats
  SDPSessionDescription & sdpIn = originalInvite->GetSDP();

  if (!OnSendSDPMediaDescription(sdpIn, SDPMediaDescription::Audio, OpalMediaFormat::DefaultAudioSessionID, sdpOut)) {
    Release(EndedByCapabilityExchange);
    return FALSE;
  }

  if (endpoint.GetManager().CanAutoStartTransmitVideo())
    OnSendSDPMediaDescription(sdpIn, SDPMediaDescription::Video, OpalMediaFormat::DefaultVideoSessionID, sdpOut);

  // translate contact address
  OpalTransportAddress contactAddress = transport->GetLocalAddress();
  WORD contactPort = endpoint.GetDefaultSignalPort();
  PIPSocket::Address localIP;
  if (!endpoint.GetListeners().IsEmpty())
    endpoint.GetListeners()[0].GetLocalAddress().GetIpAndPort(localIP, contactPort);
  if (transport->GetLocalAddress().GetIpAddress(localIP)) {
    PIPSocket::Address remoteIP;
    if (transport->GetRemoteAddress().GetIpAddress(remoteIP)) {
      endpoint.GetManager().TranslateIPAddress(localIP, remoteIP);
      contactAddress = OpalTransportAddress(localIP, contactPort, "udp");
    }
  }
    
  // send the 200 OK response
  SIPURL contact(endpoint.GetRegisteredPartyName(SIPURL(remotePartyAddress).GetHostName()).GetUserName(), contactAddress, contactPort);
  SIP_PDU response(*originalInvite, SIP_PDU::Successful_OK, (const char *) contact.AsQuotedString());
  response.SetSDP(sdpOut);
  response.Write(*transport);
  phase = ConnectedPhase;

  connectedTime = PTime ();
  
  return TRUE;
}


BOOL SIPConnection::OnSendSDPMediaDescription(const SDPSessionDescription & sdpIn,
                                              SDPMediaDescription::MediaType rtpMediaType,
                                              unsigned rtpSessionId,
                                              SDPSessionDescription & sdpOut)
{
  // if no matching media type, return FALSE
  SDPMediaDescription * incomingMedia = sdpIn.GetMediaDescription(rtpMediaType);
  if (incomingMedia == NULL) {
    PTRACE(2, "SIP\tCould not find matching media type");
    return FALSE;
  }

  // create the list of Opal format names for the remote end
  remoteFormatList = incomingMedia->GetMediaFormats();
  AdjustMediaFormats(remoteFormatList);

  // find the payload type used for telephone-event, if present
  const SDPMediaFormatList & sdpMediaList = incomingMedia->GetSDPMediaFormats();

  PINDEX i;
  BOOL hasTelephoneEvent = FALSE;
  for (i = 0; !hasTelephoneEvent && (i < sdpMediaList.GetSize()); i++) {
    if (sdpMediaList[i].GetEncodingName() == "telephone-event") {
      rfc2833Handler->SetPayloadType(sdpMediaList[i].GetPayloadType());
      hasTelephoneEvent = TRUE;
      remoteFormatList += OpalRFC2833;
    }
  }

  // create an RTP session
  RTP_UDP * rtpSession = (RTP_UDP *)UseSession(GetTransport(), rtpSessionId, NULL);
  if (rtpSession == NULL)
    return FALSE;

  // If it is not a hold, update the RTP Session info
  if (!IsConnectionOnHold()) {
    
    // Set user data
    if (rtpSession->GetUserData() == NULL)
      rtpSession->SetUserData(new SIP_RTP_Session(*this));
    
    // set the remote addresses
    PIPSocket::Address ip;
    WORD port;
    incomingMedia->GetTransportAddress().GetIpAndPort(ip, port);
    if (!rtpSession->SetRemoteSocketInfo(ip, port, TRUE)) {
      PTRACE(1, "SIP\tCannot set remote ports on RTP session");
      ReleaseSession(rtpSessionId);
      return FALSE;
    }
  }

  // look for matching codec in peer connection
  if (!ownerCall.OpenSourceMediaStreams(*this, remoteFormatList, rtpSessionId)) {
    PTRACE(2, "SIP\tCould not find compatible codecs");
    ReleaseSession(rtpSessionId);
    return FALSE;
  }

  // find out what media stream was opened
  // and get the payload type that was selected
  if (mediaStreams.IsEmpty()) {
    PTRACE(2, "SIP\tNo media streams opened");
    ReleaseSession(rtpSessionId);
    return FALSE;
  }

  // construct a new media session list with the selected format
  SDPMediaDescription * localMedia = new SDPMediaDescription(
                            GetLocalAddress(rtpSession->GetLocalDataPort()), rtpMediaType);

  // Locate the opened media stream, add it to the reply and open the reverse direction
  BOOL reverseStreamsFailed = TRUE;
  for (i = 0; i < mediaStreams.GetSize(); i++) {
    OpalMediaStream & mediaStream = mediaStreams[i];
    if (mediaStream.GetSessionID() == rtpSessionId) {
      OpalMediaFormat mediaFormat = mediaStream.GetMediaFormat();
      if (OpenSourceMediaStream(mediaFormat, rtpSessionId)) {
        localMedia->AddMediaFormat(mediaFormat);
        reverseStreamsFailed = FALSE;
      }
    }
  }

  if (reverseStreamsFailed) {
    PTRACE(2, "SIP\tNo reverse media streams opened");
    mediaStreams.RemoveAll();
    delete localMedia;
    ReleaseSession(rtpSessionId);
    return FALSE;
  }

  // Add in the RFC2833 handler, if used
  if (hasTelephoneEvent) {
    localMedia->AddSDPMediaFormat(new SDPMediaFormat("0-15", rfc2833Handler->GetPayloadType()));
  }
  
  sdpOut.AddMediaDescription(localMedia);
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
  return OpalTransportAddress(localIP, port, "udp");
}


OpalMediaFormatList SIPConnection::GetMediaFormats() const
{
  return remoteFormatList;
}


OpalMediaStream * SIPConnection::CreateMediaStream(const OpalMediaFormat & mediaFormat,
                                                   unsigned sessionID,
                                                   BOOL isSource)
{
  if (ownerCall.IsMediaBypassPossible(*this, sessionID))
    return new OpalNullMediaStream(mediaFormat, sessionID, isSource);

  if (rtpSessions.GetSession(sessionID) == NULL)
    return NULL;

  return new OpalRTPMediaStream(mediaFormat, isSource, *rtpSessions.GetSession(sessionID),
				endpoint.GetManager().GetMinAudioJitterDelay(),
				endpoint.GetManager().GetMaxAudioJitterDelay());
}


void SIPConnection::OnConnected ()
{
  if (rfc2833Handler != NULL) {

    for (int i = 0; i < mediaStreams.GetSize(); i++) {

      OpalMediaStream & mediaStream = mediaStreams[i];

      if (mediaStream.GetSessionID() == OpalMediaFormat::DefaultAudioSessionID) {
	OpalMediaPatch * patch = mediaStream.GetPatch();

	if (patch != NULL) {
	  if (mediaStream.IsSource()) {
	    patch->AddFilter(rfc2833Handler->GetReceiveHandler(), mediaStream.GetMediaFormat());
	  }
	  else {
	    patch->AddFilter(rfc2833Handler->GetTransmitHandler());
	  }
	}
      }
    }
  }

  OpalConnection::OnConnected ();
}
	

BOOL SIPConnection::IsMediaBypassPossible(unsigned sessionID) const
{
  PTRACE(3, "SIP\tIsMediaBypassPossible: session " << sessionID);

  return sessionID == OpalMediaFormat::DefaultAudioSessionID ||
         sessionID == OpalMediaFormat::DefaultVideoSessionID;
}


BOOL SIPConnection::WriteINVITE(OpalTransport & transport, void * param)
{
  SIPConnection & connection = *(SIPConnection *)param;

  connection.SetLocalPartyAddress();

  SIPTransaction * invite = new SIPInvite(connection, transport);
  if (invite->Start()) {
    connection.invitations.Append(invite);
    return TRUE;
  }

  PTRACE(2, "SIP\tDid not start INVITE transaction on " << transport);
  return FALSE;
}


BOOL SIPConnection::SetUpConnection()
{
  PTRACE(2, "SIP\tSetUpConnection: " << remotePartyAddress);

  originating = TRUE;

  delete transport;
  transport = endpoint.CreateTransport(targetAddress.GetHostAddress());
  if (transport == NULL) {
    Release(EndedByTransportFail);
    return FALSE;
  }

  if (!transport->WriteConnect(WriteINVITE, this)) {
    PTRACE(1, "SIP\tCould not write to " << targetAddress << " - " << transport->GetErrorText());
    Release(EndedByTransportFail);
    return FALSE;
  }

  releaseMethod = ReleaseWithCANCEL;
  return TRUE;
}


void SIPConnection::HoldConnection()
{
  if (local_hold)
    return;
  else
    local_hold = TRUE;

  if (transport == NULL)
    return;

  PTRACE(2, "SIP\tWill put connection on hold");

  SIPTransaction * invite = new SIPInvite(*this, *transport, rtpSessions);
  if (invite->Start()) {
    invitations.Append(invite);
    
    // Pause the media streams
    PauseMediaStreams(TRUE);
    
    // Signal the manager that there is a hold
    endpoint.OnHold(*this);
  }
}


void SIPConnection::RetrieveConnection()
{
  if (!local_hold)
    return;
  else
    local_hold = FALSE;

  if (transport == NULL)
    return;

  PTRACE(2, "SIP\tWill retrieve connection from hold");

  SIPTransaction * invite = new SIPInvite(*this, *transport, rtpSessions);
  if (invite->Start()) {
    invitations.Append(invite);
    
    // Un-Pause the media streams
    PauseMediaStreams(FALSE);

    // Signal the manager that there is a hold
    endpoint.OnHold(*this);
  }
}


BOOL SIPConnection::IsConnectionOnHold()
{
  return (local_hold || remote_hold);
}


SDPSessionDescription * SIPConnection::BuildSDP(RTP_SessionManager & rtpSessions,
                                                unsigned rtpSessionId)
{
  OpalTransportAddress localAddress;
  RTP_DataFrame::PayloadTypes ntePayloadCode = RTP_DataFrame::IllegalPayloadType;

  if (ownerCall.IsMediaBypassPossible(*this, rtpSessionId)) {
    OpalConnection * otherParty = GetCall().GetOtherPartyConnection(*this);
    if (otherParty != NULL) {
      MediaInformation info;
      if (otherParty->GetMediaInformation(rtpSessionId, info)) {
        localAddress = info.data;
        ntePayloadCode = info.rfc2833;
      }
    }
  }

  if (localAddress.IsEmpty()) {
    /* We are not doing media bypass, so must have an RTP session.
       Due to the possibility of several INVITEs going out, all with different
       transport requirements, we actually need to use an rtpSession dictionary
       for each INVITE and not the one for the connection. Once an INVITE is
       accepted the rtpSessions for that INVITE is put into the connection. */
    RTP_Session * rtpSession = rtpSessions.UseSession(rtpSessionId);
    if (rtpSession == NULL) {

      // Not already there, so create one
      rtpSession = CreateSession(GetTransport(), rtpSessionId, NULL);
      if (rtpSession == NULL)
        return NULL;

      rtpSession->SetUserData(new SIP_RTP_Session(*this));

      // add the RTP session to the RTP session manager in INVITE
      rtpSessions.AddSession(rtpSession);
    }

    localAddress = GetLocalAddress(((RTP_UDP *)rtpSession)->GetLocalDataPort());
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
  OpalMediaFormatList formats = ownerCall.GetMediaFormats(*this, FALSE);
  localMedia->AddMediaFormats(formats, rtpSessionId);

  // add in SDP records
  sdp->AddMediaDescription(localMedia);

  // add sendonly
  if (local_hold)
    sdp->SetDirection (SDPSessionDescription::SendOnly);

  return sdp;
}


void SIPConnection::SetLocalPartyAddress()
{
  OpalTransportAddress taddr = transport->GetLocalAddress();
  PIPSocket::Address addr; 
  WORD port;
  taddr.GetIpAndPort(addr, port);
  PString displayName = endpoint.GetDefaultDisplayName();
  PString localName = endpoint.GetRegisteredPartyName(SIPURL(remotePartyAddress).GetHostName()).GetUserName(); 
  PString domain = SIPURL(remotePartyAddress).GetHostName(); // //endpoint.GetRegisteredPartyName(SIPURL(remotePartyAddress).GetHostName()).GetHostName();

  // If no domain, use the local domain as default
  if (domain.IsEmpty()) {

    domain = addr.AsString();
    if (port != endpoint.GetDefaultSignalPort())
      domain += psprintf(":%d", port);
  }

  if (!localName.IsEmpty())
    SetLocalPartyName (localName);

  SIPURL myAddress("\"" + displayName + "\" <" + GetLocalPartyName() + "@" + domain + ">"); 

  // add displayname, <> and tag
  SetLocalPartyAddress(myAddress.AsQuotedString() + ";tag=" + GetTag());
}


const PString SIPConnection::GetRemotePartyCallbackURL() const
{
  SIPURL url = GetRemotePartyAddress();
  url.AdjustForRequestURI();
  return url.AsString();
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
    case SIP_PDU::Method_NOTIFY :
      OnReceivedNOTIFY(pdu);
      break;
    case SIP_PDU::Method_REFER :
      OnReceivedREFER(pdu);
      break;
    case SIP_PDU::Method_MESSAGE :
    case SIP_PDU::Method_SUBSCRIBE :
    case SIP_PDU::Method_REGISTER :
      // Shouldn't have got this!
      break;
    case SIP_PDU::NumMethods :  // if no method, must be response
      if (transaction != NULL)
        transaction->OnReceivedResponse(pdu);
      break;
  }
}


void SIPConnection::OnReceivedResponse(SIPTransaction & transaction, SIP_PDU & response)
{
  PINDEX i;

  if (transaction.GetMethod() == SIP_PDU::Method_INVITE) {
    // Have a response to the INVITE, so CANCEL all the other invitations sent.
    for (i = 0; i < invitations.GetSize(); i++) {
      if (&invitations[i] != &transaction)
        invitations[i].SendCANCEL();
    }

    // Save the sessions etc we are actually using
    // If we are in the EstablishedPhase, then the 
    // sessions are kept identical because the response is the
    // response to a hold/retrieve
    if (phase != EstablishedPhase)
      rtpSessions = ((SIPInvite &)transaction).GetSessionManager();
    localPartyAddress = transaction.GetMIME().GetFrom();
    remotePartyAddress = response.GetMIME().GetTo();
    SIPURL url(remotePartyAddress);
    remotePartyName = url.GetDisplayName ();
    remoteApplication = response.GetMIME().GetUserAgent ();
    remoteApplication.Replace ('/', '\t'); 

    // Get the route set from the Record-Route response field (in reverse order)
    PStringList recordRoute = response.GetMIME().GetRecordRoute();
    routeSet.RemoveAll();
    for (i = recordRoute.GetSize(); i > 0; i--)
      routeSet.AppendString(recordRoute[i-1]);
  }

  // If current SIP dialog does not have a route set (via Record-Route) and 
  // this response had a contact field then start using that address for all
  // future communication with remote SIP endpoint
  PString contact = response.GetMIME().GetContact();
  if (!contact)
    targetAddress = contact;

  switch (response.GetStatusCode()) {
    case SIP_PDU::Information_Trying :
      OnReceivedTrying(response);
      break;

    case SIP_PDU::Information_Ringing :
      OnReceivedRinging(response);
      break;

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

    case SIP_PDU::Failure_NotFound :
      Release(EndedByNoUser);
      break;
      
    case SIP_PDU::Failure_Forbidden :
      Release(EndedBySecurityDenial);
      break;

    case SIP_PDU::Failure_BusyHere :
      Release(EndedByRemoteBusy);
      break;

    case SIP_PDU::Failure_UnsupportedMediaType :
      Release(EndedByCapabilityExchange);
      break;

    default :
      switch (response.GetStatusCode()/100) {
        case 1 :
          // old: Do nothing on 1xx
          PTRACE(2, "SIP\tReceived unknown informational response " << response.GetStatusCode());
          break;
        case 2 :
          OnReceivedOK(transaction, response);
          break;
        default :
          // All other final responses cause a call end, if it is not a
          // local hold.
          if (!local_hold)
            Release(EndedByRefusal);
          else {
            local_hold = FALSE; // It failed
            // Un-Pause the media streams
            PauseMediaStreams(FALSE);
            // Signal the manager that there is no more hold
            endpoint.OnHold(*this);
          }
      }
  }
}


void SIPConnection::OnReceivedINVITE(SIP_PDU & request)
{
  BOOL isReinvite = FALSE;
  
  // Is Re-INVITE?
  if (phase == EstablishedPhase 
      && ((!IsOriginating() && originalInvite != NULL)
	  || (IsOriginating()))) {
    PTRACE(2, "SIP\tReceived re-INVITE from " << request.GetURI() << " for " << *this);
    isReinvite = TRUE;
  }

  if (originalInvite && !isReinvite) {
    PTRACE(2, "SIP\tIgnoring duplicate INVITE from " << request.GetURI());
    return;
  }

  if (originalInvite)
    delete originalInvite;

  originalInvite = new SIP_PDU(request);
  if (!isReinvite)
    releaseMethod = ReleaseWithResponse;

  SIPMIMEInfo & mime = originalInvite->GetMIME();
  remotePartyAddress = mime.GetFrom(); 
  SIPURL url(remotePartyAddress);
  remotePartyName = url.GetDisplayName ();
  remoteApplication = mime.GetUserAgent ();
  remoteApplication.Replace ('/', '\t'); 
  localPartyAddress  = mime.GetTo() + ";tag=" + GetTag(); // put a real random 
  mime.SetTo(localPartyAddress);

  PStringList viaList = request.GetMIME().GetViaList();
  PString via = viaList[0];
  PINDEX j = 0;
  if ((j = via.FindLast (' ')) != P_MAX_INDEX)
    via = via.Mid(j+1);
  if ((j = via.Find (';')) != P_MAX_INDEX)
    via = via.Left(j);
  OpalTransportAddress viaAddress(via, endpoint.GetDefaultSignalPort(), "udp$");
  transport->SetRemoteAddress(viaAddress);
  PTRACE(4, "SIP\tOnReceivedINVITE Changed remote address of transport " << *transport);

  targetAddress = mime.GetFrom();
  targetAddress.AdjustForRequestURI();

  // We received a Re-INVITE for a current connection
  if (isReinvite) {

    SDPSessionDescription sdpOut(GetLocalAddress());

    // get the remote media formats
    SDPSessionDescription & sdpIn = originalInvite->GetSDP();

    // The Re-INVITE can be sent to change the RTP Session parameters,
    // the current codecs, or to put the call on hold
    if (sdpIn.GetDirection() == SDPSessionDescription::SendOnly) {

      // Remotely put on hold
      remote_hold = TRUE;
      
      // Pause the media streams or not
      PauseMediaStreams(TRUE);
      
      // Signal the manager that there is a hold
      endpoint.OnHold(*this);
    }
    else {
      
      // If we receive a consecutive reinvite without the SendOnly
      // parameter, then we are not on hold anymore
      if (remote_hold) {
	
        // No hold
        remote_hold = FALSE;
	
        // Pause the media streams or not
        PauseMediaStreams(FALSE);
	
        // Signal the manager that there is a hold
        endpoint.OnHold(*this);
      }
    }

    if (!OnSendSDPMediaDescription(sdpIn, SDPMediaDescription::Audio, OpalMediaFormat::DefaultAudioSessionID, sdpOut)) {
      Release(EndedByCapabilityExchange);
      return;
    }

    if (endpoint.GetManager().CanAutoStartTransmitVideo())
      OnSendSDPMediaDescription(sdpIn, SDPMediaDescription::Video, OpalMediaFormat::DefaultVideoSessionID, sdpOut);

    // translate contact address to put it in the 200 OK response
    OpalTransportAddress contactAddress = transport->GetLocalAddress();
    WORD contactPort = endpoint.GetDefaultSignalPort();
    PIPSocket::Address localIP;
    WORD localPort;
    if (transport->GetLocalAddress().GetIpAndPort(localIP, localPort)) {
      PIPSocket::Address remoteIP;
      if (transport->GetRemoteAddress().GetIpAddress(remoteIP)) {
        PIPSocket::Address _localIP(localIP);
        endpoint.GetManager().TranslateIPAddress(localIP, remoteIP);
        if (localIP != _localIP)
          contactPort = localPort;
        contactAddress = OpalTransportAddress(localIP, contactPort, "udp");
      }
    }
    
    // send the 200 OK response
    SIPURL contact(endpoint.GetRegisteredPartyName(SIPURL(remotePartyAddress).GetHostName()).GetUserName(), contactAddress, contactPort);
    SIP_PDU response(*originalInvite, SIP_PDU::Successful_OK, (const char *) contact.AsQuotedString ());
    response.SetSDP(sdpOut);
    if (remote_hold)
      sdpOut.SetDirection(SDPSessionDescription::RecvOnly);

    response.Write(*transport);    
    return;
  }
  
  // send trying with To: tag
  SendResponseToINVITE(SIP_PDU::Information_Trying);

  // indicate the other is to start ringing
  if (!OnIncomingConnection()) {
    PTRACE(2, "SIP\tOnIncomingConnection failed for INVITE from " << request.GetURI() << " for " << *this);
    Release();
    return;
  }

  PTRACE(2, "SIP\tOnIncomingConnection succeeded for INVITE from " << request.GetURI() << " for " << *this);
  phase = SetUpPhase;
  ownerCall.OnSetUp(*this);
}


void SIPConnection::OnReceivedACK(SIP_PDU & /*response*/)
{
  PTRACE(2, "SIP\tACK received: " << phase);

  // start all of the media threads for the connection
  if (phase != ConnectedPhase)
    return;
  
  releaseMethod = ReleaseWithBYE;
  phase = EstablishedPhase;
  OnEstablished();

  // HACK HACK HACK: this is a work-around for a deadlock that can occur
  // during incoming calls. What is needed is a proper resequencing that
  // avoids this problem
  StartMediaStreams();
}


void SIPConnection::OnReceivedOPTIONS(SIP_PDU & /*request*/)
{
  PTRACE(1, "SIP\tOPTIONS not yet supported");
}


void SIPConnection::OnReceivedNOTIFY(SIP_PDU & pdu)
{
  PCaselessString event, state;
  
  if (referTransaction == NULL){
    PTRACE(1, "SIP\tNOTIFY in a connection only supported for REFER requests");
    return;
  }
  
  event = pdu.GetMIME().GetEvent();
  
  // We could also compare the To and From tags
  if (pdu.GetMIME().GetCallID() != referTransaction->GetMIME().GetCallID()
      || event.Find("refer") == P_MAX_INDEX) {

    SIP_PDU response(pdu, SIP_PDU::Failure_BadEvent);
    response.Write(*transport);
    return;
  }

  state = pdu.GetMIME().GetSubscriptionState();
  // The REFER is over
  if (state.Find("terminated") != P_MAX_INDEX) {
    referTransaction->Wait();
    delete referTransaction;
    referTransaction = NULL;
  }

  // The REFER is not over yet, ignore the state of the REFER for now
  SIP_PDU response(pdu, SIP_PDU::Successful_OK);
  response.Write(*transport);
}


void SIPConnection::OnReceivedREFER(SIP_PDU & pdu)
{
//  SIPTransaction *notifyTransaction = NULL;
  PString referto = pdu.GetMIME().GetReferTo();
  
  if (referto.IsEmpty()) {
    SIP_PDU response(pdu, SIP_PDU::Failure_BadEvent);
    response.Write(*transport);
    return;
  }    

  // Reject the Refer
  SIP_PDU response(pdu, SIP_PDU::GlobalFailure_Decline);
  response.Write(*transport);
  
//  endpoint.SetupTransfer(GetToken(),  
//			 PString (), 
//			 referto,  
//			 NULL);
  
  // Send a Final NOTIFY,
//  notifyTransaction = 
//    new SIPReferNotify(*this, *transport, SIP_PDU::Successful_Accepted);
}


void SIPConnection::OnReceivedBYE(SIP_PDU & request)
{
  PTRACE(2, "SIP\tBYE received for call " << request.GetMIME().GetCallID());
  SIP_PDU response(request, SIP_PDU::Successful_OK);
  response.Write(*transport);
  releaseMethod = ReleaseWithNothing;
  
  remotePartyAddress = request.GetMIME().GetFrom();
  SIPURL url(remotePartyAddress);
  remotePartyName = url.GetDisplayName ();
  remoteApplication = request.GetMIME ().GetUserAgent ();
  remoteApplication.Replace ('/', '\t'); 

  Release(EndedByRemoteUser);
}


void SIPConnection::OnReceivedCANCEL(SIP_PDU & request)
{
  PString origTo;
  PString origFrom;
  
  // Currently only handle CANCEL requests for the original INVITE that
  // created this connection, all else ignored
  // Ignore the tag added by OPAL
  if (originalInvite != NULL) {

    origTo = originalInvite->GetMIME().GetTo();
    origFrom = originalInvite->GetMIME().GetFrom();
    origTo.Replace (";tag=" + GetTag (), "");
    origFrom.Replace (";tag=" + GetTag (), "");
  }
  if (originalInvite == NULL ||
      request.GetMIME().GetTo() != origTo ||
      request.GetMIME().GetFrom() != origFrom ||
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


void SIPConnection::OnReceivedRinging(SIP_PDU & /*response*/)
{
  PTRACE(2, "SIP\tReceived Ringing response");

  if (phase < AlertingPhase)
  {
    phase = AlertingPhase;
    OnAlerting();
  }
}


void SIPConnection::OnReceivedSessionProgress(SIP_PDU & response)
{
  PTRACE(2, "SIP\tReceived Session Progress response");

  OnReceivedSDP(response);

  if (phase < AlertingPhase)
  {
    phase = AlertingPhase;
    OnAlerting();
  }

  PTRACE(3, "SIP\tStarting receive media to annunciate remote progress tones");
  for (PINDEX i = 0; i < mediaStreams.GetSize(); i++) {
    if (mediaStreams[i].IsSource())
      mediaStreams[i].Start();
  }
}


void SIPConnection::OnReceivedRedirection(SIP_PDU & /*response*/)
{
  // send a new INVITE
  remotePartyAddress = targetAddress.AsQuotedString(); // start with a new To tag
  SIPTransaction * invite = new SIPInvite(*this, *transport);
  if (invite->Start())
    invitations.Append(invite);
  else {
    delete invite;
    PTRACE(1, "SIP\tCould not restart INVITE for Redirection");
  }
}


void SIPConnection::OnReceivedAuthenticationRequired(SIPTransaction & transaction,
                                                     SIP_PDU & response)
{
  BOOL isProxy = response.GetStatusCode() == SIP_PDU::Failure_ProxyAuthenticationRequired;
  SIPAuthentication auth;
  PString lastRealm;
  PString lastUsername;
  PString lastNonce;

#if PTRACING
  const char * proxyTrace = isProxy ? "Proxy " : "";
#endif
  
  if (transaction.GetMethod() != SIP_PDU::Method_INVITE) {
    PTRACE(1, "SIP\tCannot do " << proxyTrace << "Authentication Required for non INVITE");
    return;
  }

  PTRACE(2, "SIP\tReceived " << proxyTrace << "Authentication Required response");

  // Received authentication required response, try to find authentication
  // for the given realm if no proxy
  if (!auth.Parse(response.GetMIME()(isProxy 
				     ? "Proxy-Authenticate"
				     : "WWW-Authenticate"),
		  isProxy)) {
    Release(EndedBySecurityDenial);
    return;
  }

  // Try to find authentication parameters for the given realm,
  // if not, use the proxy authentication parameters (if any)
  if (!endpoint.GetAuthentication(auth.GetAuthRealm(), authentication)) {
    PTRACE (3, "SIP\tCouldn't find authentication information for realm " << auth.GetAuthRealm() << ", will use SIP Outbound Proxy authentication settings, if any");
    if (!endpoint.GetProxy().IsEmpty()) {
      authentication.SetUsername(endpoint.GetProxy().GetUserName());
      authentication.SetPassword(endpoint.GetProxy().GetPassword());
    }
    else {
      Release(EndedBySecurityDenial);
      return;
    }
  }
  
  // Save the username, realm and nonce
  if (authentication.IsValid()) {
    lastRealm = authentication.GetAuthRealm();
    lastUsername = authentication.GetUsername();
    lastNonce = authentication.GetNonce();
  }

  if (!authentication.Parse(response.GetMIME()(isProxy 
					       ? "Proxy-Authenticate"
					       : "WWW-Authenticate"),
			    isProxy)) {
    Release(EndedBySecurityDenial);
    return;
  }
  
  if (authentication.IsValid()
      && lastRealm    == authentication.GetAuthRealm ()
      && lastUsername == authentication.GetUsername ()
      && lastNonce    == authentication.GetNonce ()) {

    PTRACE(1, "SIP\tAlready done INVITE for " << proxyTrace << "Authentication Required");
    Release(EndedBySecurityDenial);
    return;
  }

  // Restart the transaction with new authentication info
  remotePartyAddress = targetAddress.AsQuotedString(); // start with a new To tag

  SIPTransaction * invite = new SIPInvite(*this, *transport);
  if (invite->Start())
    invitations.Append(invite);
  else {
    delete invite;
    PTRACE(1, "SIP\tCould not restart INVITE for " << proxyTrace << "Authentication Required");
  }
}


void SIPConnection::OnReceivedOK(SIPTransaction & transaction, SIP_PDU & response)
{
  if (transaction.GetMethod() != SIP_PDU::Method_INVITE) {
    PTRACE(3, "SIP\tReceived OK response for non INVITE");
    return;
  }

  PTRACE(2, "SIP\tReceived INVITE OK response");

  OnReceivedSDP(response);

  if (phase == EstablishedPhase)
    return;
  
  connectedTime = PTime ();
  OnConnected();                        // start media streams

  releaseMethod = ReleaseWithBYE;
  phase = EstablishedPhase;
  OnEstablished();
}


void SIPConnection::OnReceivedSDP(SIP_PDU & pdu)
{
  if (!pdu.HasSDP())
    return;

  SDPSessionDescription & sdp = pdu.GetSDP();
  if (!OnReceivedSDPMediaDescription(sdp, SDPMediaDescription::Audio, OpalMediaFormat::DefaultAudioSessionID))
    return;

  remoteFormatList += OpalRFC2833;

  if (endpoint.GetManager().CanAutoStartTransmitVideo())
    OnReceivedSDPMediaDescription(sdp, SDPMediaDescription::Video, OpalMediaFormat::DefaultVideoSessionID);
}


BOOL SIPConnection::OnReceivedSDPMediaDescription(SDPSessionDescription & sdp,
                                                  SDPMediaDescription::MediaType mediaType,
                                                  unsigned rtpSessionId)
{
  SDPMediaDescription * mediaDescription = sdp.GetMediaDescription(mediaType);
  if (mediaDescription == NULL) {
    PTRACE(1, "SIP\tCould not find SDP media description for " << mediaType);
    return FALSE;
  }

  OpalTransportAddress address = mediaDescription->GetTransportAddress();
  if (ownerCall.IsMediaBypassPossible(*this, rtpSessionId))
    mediaTransportAddresses.SetAt(rtpSessionId, new OpalTransportAddress(address));
  else {

    PIPSocket::Address ip;
    WORD port;
    address.GetIpAndPort(ip, port);

    RTP_UDP * rtpSession = (RTP_UDP *)UseSession(GetTransport(), rtpSessionId, NULL);
    if (rtpSession == NULL) {
      PTRACE(1, "SIP\tSession in response that we never offered!");
      return FALSE;
    }

    rtpSession->SetUserData(new SIP_RTP_Session(*this));

    if (!rtpSession->SetRemoteSocketInfo(ip, port, TRUE)) {
      PTRACE(1, "SIP\tCould not set RTP remote socket");
      return FALSE;
    }
  }

  remoteFormatList = mediaDescription->GetMediaFormats();
  AdjustMediaFormats(remoteFormatList);

  if (!ownerCall.OpenSourceMediaStreams(*this, remoteFormatList, rtpSessionId)) {
    PTRACE(2, "SIP\tCould not open media streams for audio");
    return FALSE;
  }

  // OPen the reverse streams
  for (PINDEX i = 0; i < mediaStreams.GetSize(); i++) {
    OpalMediaStream & mediaStream = mediaStreams[i];
    if (mediaStream.GetSessionID() == rtpSessionId)
      OpenSourceMediaStream(mediaStream.GetMediaFormat(), rtpSessionId);
  }

  return TRUE;
}


void SIPConnection::QueuePDU(SIP_PDU * pdu)
{
  if (PAssertNULL(pdu) == NULL)
    return;

  if (phase >= ReleasingPhase && pduHandler == NULL) {
    // don't create another handler thread while releasing!
    PTRACE(4, "SIP\tIgnoring PDU: " << *pdu);
    delete pdu;
  }
  else {
    PTRACE(4, "SIP\tQueueing PDU: " << *pdu);
    pduQueue.Enqueue(pdu);
    pduSemaphore.Signal();

    if (pduHandler == NULL) {
      SafeReference();
      pduHandler = PThread::Create(PCREATE_NOTIFIER(HandlePDUsThreadMain), 0,
                                   PThread::NoAutoDeleteThread,
                                   PThread::NormalPriority,
                                   "SIP Handler:%x");
    }
  }
}


void SIPConnection::HandlePDUsThreadMain(PThread &, INT)
{
  PTRACE(2, "SIP\tPDU handler thread started.");

  while (phase != ReleasedPhase) {
    PTRACE(4, "SIP\tAwaiting next PDU.");
    pduSemaphore.Wait();

    if (!LockReadOnly())
      break;

    SIP_PDU * pdu = pduQueue.Dequeue();
    if (pdu != NULL) {
      OnReceivedPDU(*pdu);
      delete pdu;
    }

    UnlockReadOnly();
  }

  SafeDereference();

  PTRACE(2, "SIP\tPDU handler thread finished.");
}


BOOL SIPConnection::ForwardCall (const PString & fwdParty)
{
  if (fwdParty.IsEmpty ())
    return FALSE;
  
  forwardParty = fwdParty;
  PTRACE(2, "SIP\tIncoming SIP connection will be forwarded to " << forwardParty);
  Release(EndedByCallForwarded);

  return TRUE;
}


void SIPConnection::SendResponseToINVITE(SIP_PDU::StatusCodes code, const char * extra)
{
  if (originalInvite != NULL) {
        
    SIP_PDU response(*originalInvite, code, NULL, extra);
    response.Write(*transport);
  }
}


void SIPConnection::OnRTPStatistics(const RTP_Session & session) const
{
  endpoint.OnRTPStatistics(*this, session);
}


/////////////////////////////////////////////////////////////////////////////

SIP_RTP_Session::SIP_RTP_Session(const SIPConnection & conn)
  : connection(conn)
{
}


void SIP_RTP_Session::OnTxStatistics(const RTP_Session & session) const
{
  connection.OnRTPStatistics(session);
}


void SIP_RTP_Session::OnRxStatistics(const RTP_Session & session) const
{
  connection.OnRTPStatistics(session);
}


// End of file ////////////////////////////////////////////////////////////////
