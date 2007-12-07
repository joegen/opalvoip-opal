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
 * $Revision$
 * $Author$
 * $Date$
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
#include <ptclib/pdns.h>

#if OPAL_T38FAX
#include <t38/t38proto.h>
#endif

#define new PNEW

typedef void (SIPConnection::* SIPMethodFunction)(SIP_PDU & pdu);

static const char ApplicationDTMFRelayKey[]       = "application/dtmf-relay";
static const char ApplicationDTMFKey[]            = "application/dtmf";

#if OPAL_VIDEO
static const char ApplicationMediaControlXMLKey[] = "application/media_control+xml";
#endif


static const struct {
  SIP_PDU::StatusCodes          code;
  OpalConnection::CallEndReason reason;
  unsigned                      q931Cause;
}
//
// This table comes from RFC 3398 para 7.2.4.1
//
ReasonToSIPCode[] = {
  { SIP_PDU::Failure_NotFound                   , OpalConnection::EndedByNoUser            ,   1 }, // Unallocated number
  { SIP_PDU::Failure_NotFound                   , OpalConnection::EndedByNoUser            ,   2 }, // no route to network
  { SIP_PDU::Failure_NotFound                   , OpalConnection::EndedByNoUser            ,   3 }, // no route to destination
  { SIP_PDU::Failure_BusyHere                   , OpalConnection::EndedByLocalBusy         ,  17 }, // user busy                            
  { SIP_PDU::Failure_RequestTimeout             , OpalConnection::EndedByNoUser            ,  18 }, // no user responding                   
  { SIP_PDU::Failure_TemporarilyUnavailable     , OpalConnection::EndedByNoAnswer          ,  19 }, // no answer from the user              
  { SIP_PDU::Failure_TemporarilyUnavailable     , OpalConnection::EndedByNoUser            ,  20 }, // subscriber absent                    
  { SIP_PDU::Failure_Forbidden                  , OpalConnection::EndedByNoUser            ,  21 }, // call rejected                        
  { SIP_PDU::Failure_Gone                       , OpalConnection::EndedByNoUser            ,  22 }, // number changed (w/o diagnostic)      
  { SIP_PDU::Redirection_MovedPermanently       , OpalConnection::EndedByNoUser            ,  22 }, // number changed (w/ diagnostic)       
  { SIP_PDU::Failure_Gone                       , OpalConnection::EndedByNoUser            ,  23 }, // redirection to new destination       
  { SIP_PDU::Failure_NotFound                   , OpalConnection::EndedByNoUser            ,  26 }, // non-selected user clearing           
  { SIP_PDU::Failure_BadGateway                 , OpalConnection::EndedByNoUser            ,  27 }, // destination out of order             
  { SIP_PDU::Failure_AddressIncomplete          , OpalConnection::EndedByNoUser            ,  28 }, // address incomplete                   
  { SIP_PDU::Failure_NotImplemented             , OpalConnection::EndedByNoUser            ,  29 }, // facility rejected                    
  { SIP_PDU::Failure_TemporarilyUnavailable     , OpalConnection::EndedByNoUser            ,  31 }, // normal unspecified                   
  { SIP_PDU::Failure_ServiceUnavailable         , OpalConnection::EndedByNoUser            ,  34 }, // no circuit available                 
  { SIP_PDU::Failure_ServiceUnavailable         , OpalConnection::EndedByNoUser            ,  38 }, // network out of order                 
  { SIP_PDU::Failure_ServiceUnavailable         , OpalConnection::EndedByNoUser            ,  41 }, // temporary failure                    
  { SIP_PDU::Failure_ServiceUnavailable         , OpalConnection::EndedByNoUser            ,  42 }, // switching equipment congestion       
  { SIP_PDU::Failure_ServiceUnavailable         , OpalConnection::EndedByNoUser            ,  47 }, // resource unavailable                 
  { SIP_PDU::Failure_Forbidden                  , OpalConnection::EndedByNoUser            ,  55 }, // incoming calls barred within CUG     
  { SIP_PDU::Failure_Forbidden                  , OpalConnection::EndedByNoUser            ,  57 }, // bearer capability not authorized     
  { SIP_PDU::Failure_ServiceUnavailable         , OpalConnection::EndedByNoUser            ,  58 }, // bearer capability not presently available
  { SIP_PDU::Failure_NotAcceptableHere          , OpalConnection::EndedByNoUser            ,  65 }, // bearer capability not implemented
  { SIP_PDU::Failure_NotAcceptableHere          , OpalConnection::EndedByNoUser            ,  70 }, // only restricted digital avail    
  { SIP_PDU::Failure_NotImplemented             , OpalConnection::EndedByNoUser            ,  79 }, // service or option not implemented
  { SIP_PDU::Failure_Forbidden                  , OpalConnection::EndedByNoUser            ,  87 }, // user not member of CUG           
  { SIP_PDU::Failure_ServiceUnavailable         , OpalConnection::EndedByNoUser            ,  88 }, // incompatible destination         
  { SIP_PDU::Failure_ServerTimeout              , OpalConnection::EndedByNoUser            , 102 }, // recovery of timer expiry         
  { SIP_PDU::Failure_InternalServerError        , OpalConnection::EndedByNoUser            , 111 }, // protocol error                   
  { SIP_PDU::Failure_InternalServerError        , OpalConnection::EndedByNoUser            , 127 }, // interworking unspecified         
  { SIP_PDU::Failure_RequestTerminated          , OpalConnection::EndedByCallerAbort             },
  { SIP_PDU::Failure_UnsupportedMediaType       , OpalConnection::EndedByCapabilityExchange      },
  { SIP_PDU::Redirection_MovedTemporarily       , OpalConnection::EndedByCallForwarded           },
  { SIP_PDU::GlobalFailure_Decline              , OpalConnection::EndedByAnswerDenied            },
},

//
// This table comes from RFC 3398 para 8.2.6.1
//
SIPCodeToReason[] = {
  { SIP_PDU::Failure_BadRequest                 , OpalConnection::EndedByQ931Cause         ,  41 }, // Temporary Failure
  { SIP_PDU::Failure_UnAuthorised               , OpalConnection::EndedBySecurityDenial    ,  21 }, // Call rejected (*)
  { SIP_PDU::Failure_PaymentRequired            , OpalConnection::EndedByQ931Cause         ,  21 }, // Call rejected
  { SIP_PDU::Failure_Forbidden                  , OpalConnection::EndedBySecurityDenial    ,  21 }, // Call rejected
  { SIP_PDU::Failure_NotFound                   , OpalConnection::EndedByNoUser            ,   1 }, // Unallocated number
  { SIP_PDU::Failure_MethodNotAllowed           , OpalConnection::EndedByQ931Cause         ,  63 }, // Service or option unavailable
  { SIP_PDU::Failure_NotAcceptable              , OpalConnection::EndedByQ931Cause         ,  79 }, // Service/option not implemented (+)
  { SIP_PDU::Failure_ProxyAuthenticationRequired, OpalConnection::EndedByQ931Cause         ,  21 }, // Call rejected (*)
  { SIP_PDU::Failure_RequestTimeout             , OpalConnection::EndedByTemporaryFailure  , 102 }, // Recovery on timer expiry
  { SIP_PDU::Failure_Gone                       , OpalConnection::EndedByQ931Cause         ,  22 }, // Number changed (w/o diagnostic)
  { SIP_PDU::Failure_RequestEntityTooLarge      , OpalConnection::EndedByQ931Cause         , 127 }, // Interworking (+)
  { SIP_PDU::Failure_RequestURITooLong          , OpalConnection::EndedByQ931Cause         , 127 }, // Interworking (+)
  { SIP_PDU::Failure_UnsupportedMediaType       , OpalConnection::EndedByCapabilityExchange,  79 }, // Service/option not implemented (+)
  { SIP_PDU::Failure_UnsupportedURIScheme       , OpalConnection::EndedByQ931Cause         , 127 }, // Interworking (+)
  { SIP_PDU::Failure_BadExtension               , OpalConnection::EndedByQ931Cause         , 127 }, // Interworking (+)
  { SIP_PDU::Failure_ExtensionRequired          , OpalConnection::EndedByQ931Cause         , 127 }, // Interworking (+)
  { SIP_PDU::Failure_IntervalTooBrief           , OpalConnection::EndedByQ931Cause         , 127 }, // Interworking (+)
  { SIP_PDU::Failure_TemporarilyUnavailable     , OpalConnection::EndedByTemporaryFailure  ,  18 }, // No user responding
  { SIP_PDU::Failure_TransactionDoesNotExist    , OpalConnection::EndedByQ931Cause         ,  41 }, // Temporary Failure
  { SIP_PDU::Failure_LoopDetected               , OpalConnection::EndedByQ931Cause         ,  25 }, // Exchange - routing error
  { SIP_PDU::Failure_TooManyHops                , OpalConnection::EndedByQ931Cause         ,  25 }, // Exchange - routing error
  { SIP_PDU::Failure_AddressIncomplete          , OpalConnection::EndedByQ931Cause         ,  28 }, // Invalid Number Format (+)
  { SIP_PDU::Failure_Ambiguous                  , OpalConnection::EndedByNoUser            ,   1 }, // Unallocated number
  { SIP_PDU::Failure_BusyHere                   , OpalConnection::EndedByRemoteBusy        ,  17 }, // User busy
  { SIP_PDU::Failure_InternalServerError        , OpalConnection::EndedByQ931Cause         ,  41 }, // Temporary failure
  { SIP_PDU::Failure_NotImplemented             , OpalConnection::EndedByQ931Cause         ,  79 }, // Not implemented, unspecified
  { SIP_PDU::Failure_BadGateway                 , OpalConnection::EndedByQ931Cause         ,  38 }, // Network out of order
  { SIP_PDU::Failure_ServiceUnavailable         , OpalConnection::EndedByQ931Cause         ,  41 }, // Temporary failure
  { SIP_PDU::Failure_ServerTimeout              , OpalConnection::EndedByQ931Cause         , 102 }, // Recovery on timer expiry
  { SIP_PDU::Failure_SIPVersionNotSupported     , OpalConnection::EndedByQ931Cause         , 127 }, // Interworking (+)
  { SIP_PDU::Failure_MessageTooLarge            , OpalConnection::EndedByQ931Cause         , 127 }, // Interworking (+)
  { SIP_PDU::GlobalFailure_BusyEverywhere       , OpalConnection::EndedByQ931Cause         ,  17 }, // User busy
  { SIP_PDU::GlobalFailure_Decline              , OpalConnection::EndedByRefusal           ,  21 }, // Call rejected
  { SIP_PDU::GlobalFailure_DoesNotExistAnywhere , OpalConnection::EndedByNoUser            ,   1 }, // Unallocated number
};


////////////////////////////////////////////////////////////////////////////

SIPConnection::SIPConnection(OpalCall & call,
                             SIPEndPoint & ep,
                             const PString & token,
                             const SIPURL & destination,
                             OpalTransport * newTransport,
                             unsigned int options,
                             OpalConnection::StringOptions * stringOptions)
  : OpalConnection(call, ep, token, options, stringOptions),
    endpoint(ep),
    transport(newTransport),
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

  if (proxy.IsEmpty())
    proxy = endpoint.GetProxy();

  // Default routeSet if there is a proxy
  if (!proxy.IsEmpty()) 
    routeSet += "sip:" + proxy.GetHostName() + ':' + PString(proxy.GetPort()) + ";lr";
  
  // Update remote party parameters
  remotePartyAddress = targetAddress.AsQuotedString();
  UpdateRemotePartyNameAndNumber();
  
  originalInvite = NULL;
  pduHandler = NULL;
  lastSentCSeq.SetValue(0);
  releaseMethod = ReleaseWithNothing;

  forkedInvitations.DisallowDeleteObjects();

  referTransaction = (SIPTransaction *)NULL;
  local_hold = PFalse;
  remote_hold = PFalse;

  ackTimer.SetNotifier(PCREATE_NOTIFIER(OnAckTimeout));

  PTRACE(4, "SIP\tCreated connection.");
}


void SIPConnection::UpdateRemotePartyNameAndNumber()
{
  SIPURL url(remotePartyAddress);
  remotePartyName   = url.GetDisplayName ();
  remotePartyNumber = url.GetUserName();
}

SIPConnection::~SIPConnection()
{
  delete originalInvite;
  delete transport;

  if (pduHandler) 
    delete pduHandler;

  PTRACE(4, "SIP\tDeleted connection.");
}

void SIPConnection::OnReleased()
{
  PTRACE(3, "SIP\tOnReleased: " << *this << ", phase = " << phase);
  
  // OpalConnection::Release sets the phase to Releasing in the SIP Handler 
  // thread
  if (GetPhase() >= ReleasedPhase){
    PTRACE(2, "SIP\tOnReleased: already released");
    return;
  };

  SetPhase(ReleasingPhase);

  PSafePtr<SIPTransaction> byeTransaction;

  switch (releaseMethod) {
    case ReleaseWithNothing :
      break;

    case ReleaseWithResponse :
      {
        SIP_PDU::StatusCodes sipCode = SIP_PDU::Failure_BadGateway;

        // Try find best match for return code
        for (PINDEX i = 0; i < PARRAYSIZE(ReasonToSIPCode); i++) {
          if (ReasonToSIPCode[i].q931Cause == GetQ931Cause()) {
            sipCode = ReasonToSIPCode[i].code;
            break;
          }
          if (ReasonToSIPCode[i].reason == callEndReason) {
            sipCode = ReasonToSIPCode[i].code;
            break;
          }
        }

        // EndedByCallForwarded is a special case because it needs extra paramater
        SendInviteResponse(sipCode, NULL, callEndReason == EndedByCallForwarded ? (const char *)forwardParty : NULL);
      }
      break;

    case ReleaseWithBYE :
      // create BYE now & delete it later to prevent memory access errors
      byeTransaction = new SIPTransaction(*this, *transport, SIP_PDU::Method_BYE);
      break;

    case ReleaseWithCANCEL :
      {
        PTRACE(3, "SIP\tCancelling " << forkedInvitations.GetSize() << " transactions.");
        for (PSafePtr<SIPTransaction> invitation(forkedInvitations, PSafeReference); invitation != NULL; ++invitation)
          invitation->Cancel();
      }
  }

  // Close media
  CloseMediaStreams();

  // Sent a BYE, wait for it to complete
  if (byeTransaction != NULL)
    byeTransaction->WaitForCompletion();

  // Wait until all INVITEs have completed
  for (PSafePtr<SIPTransaction> invitation(forkedInvitations, PSafeReference); invitation != NULL; ++invitation)
    invitation->WaitForCompletion();
  forkedInvitations.RemoveAll();

  SetPhase(ReleasedPhase);

  if (pduHandler != NULL) {
    pduSemaphore.Signal();
    pduHandler->WaitForTermination();
  }
  
  OpalConnection::OnReleased();

  if (transport != NULL)
    transport->CloseWait();
}


void SIPConnection::TransferConnection(const PString & remoteParty, const PString & callIdentity)
{
  // There is still an ongoing REFER transaction 
  if (referTransaction != NULL) 
    return;
 
  referTransaction = new SIPRefer(*this, *transport, remoteParty, callIdentity);
  referTransaction->Start ();
}

PBoolean SIPConnection::ConstructSDP(SDPSessionDescription & sdpOut)
{
  PBoolean sdpOK;

  // get the remote media formats, if any
  if (originalInvite->HasSDP()) {
    remoteSDP = originalInvite->GetSDP();

    // Use |= to avoid McCarthy boolean || from not calling video/fax
    sdpOK  = OnSendSDPMediaDescription(remoteSDP, OpalMediaSessionId("audio"), sdpOut);
#if OPAL_VIDEO
    sdpOK |= OnSendSDPMediaDescription(remoteSDP, OpalMediaSessionId("video"), sdpOut);
#endif
#if OPAL_T38FAX
    sdpOK |= OnSendSDPMediaDescription(remoteSDP, OpalMediaSessionId("image"), sdpOut);
#endif
  }
  
  else {

    // construct offer as per RFC 3261, para 14.2
    // Use |= to avoid McCarthy boolean || from not calling video/fax
    SDPSessionDescription *sdp = (SDPSessionDescription *) &sdpOut;
    sdpOK  = BuildSDP(sdp, rtpSessions, OpalMediaSessionId("audio"));
#if OPAL_VIDEO
    sdpOK |= BuildSDP(sdp, rtpSessions, OpalMediaSessionId("video"));
#endif
#if OPAL_T38FAX
    sdpOK |= BuildSDP(sdp, rtpSessions, OpalMediaSessionId("image"));
#endif
  }

  if (sdpOK)
    return phase < ReleasingPhase; // abort if already in releasing phase

  Release(EndedByCapabilityExchange);
  return PFalse;
}

PBoolean SIPConnection::SetAlerting(const PString & /*calleeName*/, PBoolean withMedia)
{
  if (IsOriginating()) {
    PTRACE(2, "SIP\tSetAlerting ignored on call we originated.");
    return PTrue;
  }

  PSafeLockReadWrite safeLock(*this);
  if (!safeLock.IsLocked())
    return PFalse;
  
  PTRACE(3, "SIP\tSetAlerting");

  if (phase != SetUpPhase) 
    return PFalse;

  if (!withMedia) 
    SendInviteResponse(SIP_PDU::Information_Ringing);
  else {
    SDPSessionDescription sdpOut(GetLocalAddress());
    if (!ConstructSDP(sdpOut)) {
      SendInviteResponse(SIP_PDU::Failure_NotAcceptableHere);
      return PFalse;
    }
    if (!SendInviteResponse(SIP_PDU::Information_Session_Progress, NULL, NULL, &sdpOut))
      return PFalse;
  }

  SetPhase(AlertingPhase);

  return PTrue;
}


PBoolean SIPConnection::SetConnected()
{
  if (transport == NULL) {
    Release(EndedByTransportFail);
    return PFalse;
  }

  if (IsOriginating()) {
    PTRACE(2, "SIP\tSetConnected ignored on call we originated.");
    return PTrue;
  }

  PSafeLockReadWrite safeLock(*this);
  if (!safeLock.IsLocked())
    return PFalse;
  
  if (GetPhase() >= ConnectedPhase) {
    PTRACE(2, "SIP\tSetConnected ignored on already connected call.");
    return PFalse;
  }
  
  PTRACE(3, "SIP\tSetConnected");

  SDPSessionDescription sdpOut(GetLocalAddress());

  if (!ConstructSDP(sdpOut)) {
    SendInviteResponse(SIP_PDU::Failure_NotAcceptableHere);
    return PFalse;
  }
    
  // update the route set and the target address according to 12.1.1
  // requests in a dialog do not modify the route set according to 12.2
  if (phase < ConnectedPhase) {
    routeSet.RemoveAll();
    routeSet = originalInvite->GetMIME().GetRecordRoute();
    PString originalContact = originalInvite->GetMIME().GetContact();
    if (!originalContact.IsEmpty()) {
      targetAddress = originalContact;
    }
  }

  // send the 200 OK response
  SendInviteOK(sdpOut);

  // switch phase 
  SetPhase(ConnectedPhase);
  connectedTime = PTime ();
  
  return PTrue;
}


RTP_UDP *SIPConnection::OnUseRTPSession(const OpalMediaSessionId & sessionId, const OpalTransportAddress & mediaAddress, OpalTransportAddress & localAddress)
{
  RTP_UDP *rtpSession = NULL;
  RTP_DataFrame::PayloadTypes ntePayloadCode = RTP_DataFrame::IllegalPayloadType;

  {
    PSafeLockReadOnly m(ownerCall);
    PSafePtr<OpalConnection> otherParty = GetCall().GetOtherPartyConnection(*this);
    if (otherParty == NULL) {
      PTRACE(2, "H323\tCowardly refusing to create an RTP channel with only one connection");
      return NULL;
     }

    // if doing media bypass, we need to set the local address
    // otherwise create an RTP session
    if (ownerCall.IsMediaBypassPossible(*this, sessionId)) {
      OpalConnection * otherParty = GetCall().GetOtherPartyConnection(*this);
      if (otherParty != NULL) {
        MediaInformation info;
        if (otherParty->GetMediaInformation(sessionId, info)) {
          localAddress = info.data;
          ntePayloadCode = info.rfc2833;
        }
      }
      mediaTransportAddresses.SetAt(sessionId.sessionId, new OpalTransportAddress(mediaAddress));
    }
    else {
      // create an RTP session
      rtpSession = (RTP_UDP *)UseSession(GetTransport(), sessionId, NULL);
      if (rtpSession == NULL) {
        return NULL;
      }
      
      // Set user data
      if (rtpSession->GetUserData() == NULL)
        rtpSession->SetUserData(new SIP_RTP_Session(*this));
  
      // Local Address of the session
      localAddress = GetLocalAddress(rtpSession->GetLocalDataPort());
    }
  }
  
  return rtpSession;
}


PBoolean SIPConnection::OnSendSDPMediaDescription(const SDPSessionDescription & sdpIn,
                                                     const OpalMediaSessionId & sessionId,
                                                        SDPSessionDescription & sdpOut)
{
  // make sure the media type is known
  OpalMediaTypeDefinition * mediaDefn = OpalMediaTypeFactory::CreateInstance(sessionId.mediaType);
  if (mediaDefn == NULL) {
    PTRACE(2, "SIP\tUnknown media type " << sessionId.mediaType);
    return PFalse;
  }

  RTP_UDP * rtpSession = NULL;

  // if no matching media type, return PFalse
  SDPMediaDescription * incomingMedia = sdpIn.GetMediaDescription(sessionId.mediaType);
  if (incomingMedia == NULL) {
    PTRACE(2, "SIP\tCould not find matching media type for session " << sessionId.mediaType);
    return PFalse;
  }

  if (incomingMedia->GetMediaFormats(sessionId.mediaType).GetSize() == 0) {
    PTRACE(1, "SIP\tCould not find media formats in SDP media description for session " << sessionId.mediaType);
    return PFalse;
  }
  
  // Create the list of Opal format names for the remote end.
  // We will answer with the media format that will be opened.
  // When sending an answer SDP, remove media formats that we do not support.
  remoteFormatList += incomingMedia->GetMediaFormats(sessionId.mediaType);
  remoteFormatList.Remove(endpoint.GetManager().GetMediaFormatMask());
  if (remoteFormatList.GetSize() == 0) {
    Release(EndedByCapabilityExchange);
    return PFalse;
  }

  SDPMediaDescription * localMedia = NULL;
  OpalTransportAddress mediaAddress = incomingMedia->GetTransportAddress();

  // find the payload type used for telephone-event, if present
  const SDPMediaFormatList & sdpMediaList = incomingMedia->GetSDPMediaFormats();
  PBoolean hasTelephoneEvent = PFalse;
#if OPAL_T38FAX
  PBoolean hasNSE = PFalse;
#endif
  PINDEX i;
  for (i = 0; !hasTelephoneEvent && (i < sdpMediaList.GetSize()); i++) {
    if (sdpMediaList[i].GetEncodingName() == "telephone-event") {
      rfc2833Handler->SetPayloadType(sdpMediaList[i].GetPayloadType());
      hasTelephoneEvent = PTrue;
      remoteFormatList += OpalRFC2833;
    }
#if OPAL_T38FAX
    if (sdpMediaList[i].GetEncodingName() == "nse") {
      ciscoNSEHandler->SetPayloadType(sdpMediaList[i].GetPayloadType());
      hasNSE = PTrue;
      remoteFormatList += OpalCiscoNSE;
    }
#endif
  }

  // Create or re-use the RTP session
  OpalTransportAddress localAddress;
  rtpSession = OnUseRTPSession(sessionId, mediaAddress, localAddress);
  if (rtpSession == NULL && !ownerCall.IsMediaBypassPossible(*this, sessionId)) {
    if (sessionId.mediaType == "audio") 
      Release(EndedByTransportFail);
    return PFalse;
  }

  // set the remote address
  PIPSocket::Address ip;
  WORD port;
  if (!mediaAddress.GetIpAndPort(ip, port) || (rtpSession && !rtpSession->SetRemoteSocketInfo(ip, port, PTrue))) {
    PTRACE(1, "SIP\tCannot set remote ports on RTP session");
    if (sessionId.mediaType == "audio") 
      Release(EndedByTransportFail);
    return PFalse;
  }

  // construct a new media session list 
  localMedia = mediaDefn->CreateSDPMediaDescription(sessionId.mediaType, localAddress);

  // create map for RTP payloads
  incomingMedia->CreateRTPMap(sessionId, rtpPayloadMap);

  // Open the streams and the reverse media streams
  PBoolean reverseStreamsFailed = PTrue;
  reverseStreamsFailed = OnOpenSourceMediaStreams(remoteFormatList, sessionId, localMedia);

  // Add in the RFC2833 handler, if used
  if (hasTelephoneEvent) {
    localMedia->AddSDPMediaFormat(new SDPMediaFormat(OpalRFC2833, rfc2833Handler->GetPayloadType(), OpalDefaultNTEString));
  }
#if OPAL_T38FAX
  if (hasNSE) {
    localMedia->AddSDPMediaFormat(new SDPMediaFormat(OpalCiscoNSE, ciscoNSEHandler->GetPayloadType(), OpalDefaultNSEString));
  }
#endif

  // No stream opened for this session, use the default SDP
  if (reverseStreamsFailed) {
    SDPSessionDescription *sdp = (SDPSessionDescription *) &sdpOut;
    if (!BuildSDP(sdp, rtpSessions, sessionId)) {
      delete localMedia;
      return PFalse;
    }
    return PTrue;
  }

  localMedia->SetDirection(GetDirection(sessionId));
  sdpOut.AddMediaDescription(localMedia);

  return PTrue;
}


PBoolean SIPConnection::OnOpenSourceMediaStreams(const OpalMediaFormatList & remoteFormatList, const OpalMediaSessionId & sessionId, SDPMediaDescription *localMedia)
{
  PBoolean reverseStreamsFailed = PTrue;

  ownerCall.OpenSourceMediaStreams(*this, remoteFormatList, sessionId);

  OpalMediaFormatList otherList;
  {
    PSafePtr<OpalConnection> otherParty = GetCall().GetOtherPartyConnection(*this);
    if (otherParty == NULL) {
      PTRACE(1, "SIP\tCannot get other connection");
      return PFalse;
    }
    otherList = otherParty->GetMediaFormats();
  }

  for (PINDEX i = 0; i < mediaStreams.GetSize(); i++) {
    OpalMediaStream & mediaStream = mediaStreams[i];
    if (mediaStream.GetSessionID().sessionId == sessionId.sessionId) {
      if (OpenSourceMediaStream(otherList, sessionId) && localMedia) {
        localMedia->AddMediaFormat(mediaStream.GetMediaFormat(), rtpPayloadMap);
        reverseStreamsFailed = PFalse;
      }
    }
  }

  return reverseStreamsFailed;
}


SDPMediaDescription::Direction SIPConnection::GetDirection(
#if OPAL_VIDEO
  const OpalMediaSessionId & sessionId
#else
  const OpalMediaSessionId & 
#endif
  )
{
  if (remote_hold)
    return SDPMediaDescription::RecvOnly;

  if (local_hold)
    return SDPMediaDescription::SendOnly;

#if OPAL_VIDEO
  if (sessionId.mediaType == "video") {
    if (endpoint.GetManager().CanAutoStartTransmitVideo() && !endpoint.GetManager().CanAutoStartReceiveVideo())
      return SDPMediaDescription::SendOnly;
    if (!endpoint.GetManager().CanAutoStartTransmitVideo() && endpoint.GetManager().CanAutoStartReceiveVideo())
      return SDPMediaDescription::RecvOnly;
  }
#endif
  
  return SDPMediaDescription::Undefined;
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


PBoolean SIPConnection::OpenSourceMediaStream(const OpalMediaFormatList & mediaFormats, const OpalMediaSessionId & sessionID)
{
  // The remote user is in recvonly mode or in inactive mode for that session
  switch (remoteSDP.GetDirection(sessionID)) {
    case SDPMediaDescription::Inactive :
    case SDPMediaDescription::RecvOnly :
      return PFalse;

    default :
      return OpalConnection::OpenSourceMediaStream(mediaFormats, sessionID);
  }
}


OpalMediaStream * SIPConnection::OpenSinkMediaStream(OpalMediaStream & source)
{
  // The remote user is in sendonly mode or in inactive mode for that session
  switch (remoteSDP.GetDirection(source.GetSessionID())) {
    case SDPMediaDescription::Inactive :
    case SDPMediaDescription::SendOnly :
      return NULL;

    default :
      return OpalConnection::OpenSinkMediaStream(source);
  }
}

void SIPConnection::OnPatchMediaStream(PBoolean isSource, OpalMediaPatch & patch)
{
  OpalConnection::OnPatchMediaStream(isSource, patch);
  if (patch.GetSource().GetSessionID().mediaType == "audio") {
    AttachRFC2833HandlerToPatch(isSource, patch);
  }
}


void SIPConnection::OnConnected ()
{
  OpalConnection::OnConnected ();
}


PBoolean SIPConnection::WriteINVITE(OpalTransport & transport, void * param)
{
  SIPConnection & connection = *(SIPConnection *)param;

  connection.SetLocalPartyAddress();

  SIPTransaction * invite = new SIPInvite(connection, transport);
  
  // It may happen that constructing the INVITE causes the connection
  // to be released (e.g. there are no UDP ports available for the RTP sessions)
  // Since the connection is released immediately, a INVITE must not be
  // sent out.
  if (connection.GetPhase() >= OpalConnection::ReleasingPhase) {
    PTRACE(2, "SIP\tAborting INVITE transaction since connection is in releasing phase");
    delete invite; // Before Start() is called we are responsible for deletion
    return PFalse;
  }
  
  if (invite->Start()) {
    connection.forkedInvitations.Append(invite);
    return PTrue;
  }

  PTRACE(2, "SIP\tDid not start INVITE transaction on " << transport);
  return PFalse;
}


PBoolean SIPConnection::SetUpConnection()
{
  PTRACE(3, "SIP\tSetUpConnection: " << remotePartyAddress);

  ApplyStringOptions();

  SIPURL transportAddress;

  PStringList routeSet = GetRouteSet();
  if (!routeSet.IsEmpty()) 
    transportAddress = routeSet[0];
  else {
    transportAddress = targetAddress;
    transportAddress.AdjustToDNS(); // Do a DNS SRV lookup
    PTRACE(4, "SIP\tConnecting to " << targetAddress << " via " << transportAddress);
  }

  originating = PTrue;

  delete transport;
  transport = endpoint.CreateTransport(transportAddress.GetHostAddress());
  if (transport == NULL) {
    Release(EndedByTransportFail);
    return PFalse;
  }

  if (!transport->WriteConnect(WriteINVITE, this)) {
    PTRACE(1, "SIP\tCould not write to " << transportAddress << " - " << transport->GetErrorText());
    Release(EndedByTransportFail);
    return PFalse;
  }

  releaseMethod = ReleaseWithCANCEL;
  return PTrue;
}


void SIPConnection::HoldConnection()
{
  if (local_hold || transport == NULL)
    return;

  PTRACE(3, "SIP\tWill put connection on hold");

  local_hold = PTrue;

  SIPTransaction * invite = new SIPInvite(*this, *transport, rtpSessions);
  if (invite->Start()) {
    // Pause the media streams
    PauseMediaStreams(PTrue);
    
    // Signal the manager that there is a hold
    endpoint.OnHold(*this);
  }
  else
    local_hold = PFalse;
}


void SIPConnection::RetrieveConnection()
{
  if (!local_hold)
    return;

  local_hold = PFalse;

  if (transport == NULL)
    return;

  PTRACE(3, "SIP\tWill retrieve connection from hold");

  SIPTransaction * invite = new SIPInvite(*this, *transport, rtpSessions);
  if (invite->Start()) {
    // Un-Pause the media streams
    PauseMediaStreams(PFalse);

    // Signal the manager that there is a hold
    endpoint.OnHold(*this);
  }
}


PBoolean SIPConnection::IsConnectionOnHold()
{
  return (local_hold || remote_hold);
}

static void SetNXEPayloadCode(SDPMediaDescription * localMedia, 
                      RTP_DataFrame::PayloadTypes & nxePayloadCode, 
                              OpalRFC2833Proto    * handler,
                            const OpalMediaFormat &  mediaFormat,
                                       const char * defaultNXEString, 
                                       const char * PTRACE_PARAM(label))
{
  if (nxePayloadCode != RTP_DataFrame::IllegalPayloadType) {
    PTRACE(3, "SIP\tUsing bypass RTP payload " << nxePayloadCode << " for " << label);
    localMedia->AddSDPMediaFormat(new SDPMediaFormat(mediaFormat, nxePayloadCode, defaultNXEString));
  }
  else {
    nxePayloadCode = handler->GetPayloadType();
    if (nxePayloadCode == RTP_DataFrame::IllegalPayloadType) {
      nxePayloadCode = mediaFormat.GetPayloadType();
    }
    if (nxePayloadCode != RTP_DataFrame::IllegalPayloadType) {
      PTRACE(3, "SIP\tUsing RTP payload " << nxePayloadCode << " for " << label);

      // create and add the NXE media format
      localMedia->AddSDPMediaFormat(new SDPMediaFormat(mediaFormat, nxePayloadCode, defaultNXEString));
    }
    else {
      PTRACE(2, "SIP\tCould not allocate dynamic RTP payload for " << label);
    }
  }

  handler->SetPayloadType(nxePayloadCode);
}

PBoolean SIPConnection::BuildSDP(SDPSessionDescription * & sdp, 
                                      RTP_SessionManager & rtpSessions,
                                const OpalMediaSessionId & sessionID)
{
  OpalTransportAddress localAddress;
  RTP_DataFrame::PayloadTypes ntePayloadCode = RTP_DataFrame::IllegalPayloadType;
#if OPAL_T38FAX
  RTP_DataFrame::PayloadTypes nsePayloadCode = RTP_DataFrame::IllegalPayloadType;
#endif

#if OPAL_VIDEO
  if (sessionID.mediaType == "video" &&
           !endpoint.GetManager().CanAutoStartReceiveVideo() &&
           !endpoint.GetManager().CanAutoStartTransmitVideo())
    return PFalse;
#endif

  OpalMediaFormatList formats = GetLocalMediaFormats();

  // See if any media formats of this session id, so don't create unused RTP session
  PINDEX i;
  for (i = 0; i < formats.GetSize(); i++) {
    OpalMediaFormat & fmt = formats[i];
    if (fmt.GetMediaType() == sessionID.mediaType &&
            (sessionID.mediaType == "image" || fmt.IsTransportable()))
      break;
  }
  if (i >= formats.GetSize()) {
    PTRACE(3, "SIP\tNo media formats for session type " << sessionID.mediaType << ", not adding SDP");
    return PFalse;
  }

  if (ownerCall.IsMediaBypassPossible(*this, sessionID)) {
    PSafePtr<OpalConnection> otherParty = GetCall().GetOtherPartyConnection(*this);
    if (otherParty != NULL) {
      MediaInformation info;
      if (otherParty->GetMediaInformation(sessionID, info)) {
        localAddress = info.data;
        ntePayloadCode = info.rfc2833;
#if OPAL_T38FAX
        nsePayloadCode = info.ciscoNSE;
#endif
      }
    }
  }

  if (localAddress.IsEmpty()) {

    /* We are not doing media bypass, so must have some media session
       Due to the possibility of several INVITEs going out, all with different
       transport requirements, we actually need to use an rtpSession dictionary
       for each INVITE and not the one for the connection. Once an INVITE is
       accepted the rtpSessions for that INVITE is put into the connection. */
    RTP_Session * rtpSession = rtpSessions.UseSession(sessionID);
    if (rtpSession == NULL) {

      // Not already there, so create one
      rtpSession = CreateSession(GetTransport(), sessionID, NULL);
      if (rtpSession == NULL) {
        PTRACE(1, "SIP\tCould not create session id " << sessionID.sessionId << ", released " << *this);
        Release(OpalConnection::EndedByTransportFail);
        return PFalse;
      }

      rtpSession->SetUserData(new SIP_RTP_Session(*this));

      // add the RTP session to the RTP session manager in INVITE
      rtpSessions.AddSession(rtpSession);
    }

    if (rtpSession != NULL)
      localAddress = GetLocalAddress(((RTP_UDP *)rtpSession)->GetLocalDataPort());
  } 

  if (sdp == NULL)
    sdp = new SDPSessionDescription(localAddress);

  if (localAddress.IsEmpty()) {
    PTRACE(2, "SIP\tRefusing to add SDP media description for session id " << sessionID.sessionId << " with no transport address");
  }
  else {

    // look up the media type
    OpalMediaTypeDefinition * opalMediaTypeDefn = OpalMediaTypeFactory::CreateInstance(sessionID.mediaType);
    SDPMediaDescription * localMedia = opalMediaTypeDefn->CreateSDPMediaDescription(sessionID.mediaType, localAddress);
    if (localMedia == NULL) {
      PTRACE(2, "SIP\tUnable to find SDP media description for unknown media type " << sessionID.mediaType);
    }
    else {
      // add media formats first, as Mediatrix gateways barf if RFC2833 is first
      localMedia->AddMediaFormats(formats, sessionID, rtpPayloadMap);

      // Set format if we have an RTP payload type for RFC2833 and/or NSE
      if (sessionID.mediaType == "audio") {

        // RFC 2833
        SetNXEPayloadCode(localMedia, ntePayloadCode, rfc2833Handler,  OpalRFC2833, OpalDefaultNTEString, "NTE");

#if OPAL_T38FAX
        // Cisco NSE
        SetNXEPayloadCode(localMedia, nsePayloadCode, ciscoNSEHandler, OpalCiscoNSE, OpalDefaultNSEString, "NSE");
#endif
      }
  
      localMedia->SetDirection(GetDirection(sessionID));
      sdp->AddMediaDescription(localMedia);
    }
  }

  return PTrue;
}


void SIPConnection::SetLocalPartyAddress()
{
  // preserve tag to be re-added to explicitFrom below, if there are
  // stringOptions
  PString tag = ";tag=" + OpalGloballyUniqueID().AsString();
  SIPURL registeredPartyName = endpoint.GetRegisteredPartyName(remotePartyAddress);
  localPartyAddress = registeredPartyName.AsQuotedString() + tag; 

  // allow callers to override the From field
  if (stringOptions != NULL) {
    SIPURL newFrom(GetLocalPartyAddress());

    // only allow override of calling party number if the local party
    // name hasn't been first specified by a register handler. i.e a
    // register handler's target number is always used

    // $$$ perhaps a register handler could have a configurable option
    // to control this behavior
    PString number((*stringOptions)("Calling-Party-Number"));
    if (!number.IsEmpty() && newFrom.GetUserName() == endpoint.GetDefaultLocalPartyName())
      newFrom.SetUserName(number);

    PString name((*stringOptions)("Calling-Party-Name"));
    if (!name.IsEmpty())
      newFrom.SetDisplayName(name);

    explicitFrom = newFrom.AsQuotedString() + tag;

    PTRACE(1, "SIP\tChanging From from " << GetLocalPartyAddress()
           << " to " << explicitFrom << " using " << name << " and " << number);
  }
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

  // If we are releasing then I can safely ignore failed
  // transactions - otherwise I'll deadlock.
  if (phase >= ReleasingPhase)
    return;

  {
    // The connection stays alive unless all INVITEs have failed
    for (PSafePtr<SIPTransaction> invitation(forkedInvitations, PSafeReference); invitation != NULL; ++invitation) {
      if (!invitation->IsFailed())
        return;
    }
  }

  // All invitations failed, die now
  releaseMethod = ReleaseWithNothing;
  Release(EndedByConnectFail);
}


void SIPConnection::OnReceivedPDU(SIP_PDU & pdu)
{
  PTRACE(4, "SIP\tHandling PDU " << pdu);

  if (!LockReadWrite())
    return;

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
    case SIP_PDU::Method_INFO :
      OnReceivedINFO(pdu);
      break;
    case SIP_PDU::Method_PING :
      OnReceivedPING(pdu);
      break;
    case SIP_PDU::Method_MESSAGE :
    case SIP_PDU::Method_SUBSCRIBE :
    case SIP_PDU::Method_REGISTER :
    case SIP_PDU::Method_PUBLISH :
      // Shouldn't have got this!
      break;
    case SIP_PDU::NumMethods :  // if no method, must be response
      {
        UnlockReadWrite(); // Need to avoid deadlocks with transaction mutex
        PSafePtr<SIPTransaction> transaction = endpoint.GetTransaction(pdu.GetTransactionID(), PSafeReference);
        if (transaction != NULL)
          transaction->OnReceivedResponse(pdu);
      }
      return;
  }

  UnlockReadWrite();
}


void SIPConnection::OnReceivedResponse(SIPTransaction & transaction, SIP_PDU & response)
{
  PSafeLockReadWrite lock(*this);
  if (!lock.IsLocked())
    return;

  PINDEX i;
  PBoolean ignoreErrorResponse = PFalse;
  PBoolean reInvite = PFalse;

  if (transaction.GetMethod() == SIP_PDU::Method_INVITE) {
    // See if this is an initial INVITE or a re-INVITE
    reInvite = PTrue;
    for (PSafePtr<SIPTransaction> invitation(forkedInvitations, PSafeReference); invitation != NULL; ++invitation) {
      if (invitation == &transaction) {
        reInvite = PFalse;
        break;
      }
    }

    if (!reInvite && response.GetStatusCode()/100 <= 2) {
      if (response.GetStatusCode()/100 == 2) {
        // Have a final response to the INVITE, so cancel all the other invitations sent.
        for (PSafePtr<SIPTransaction> invitation(forkedInvitations, PSafeReference); invitation != NULL; ++invitation) {
          if (invitation != &transaction)
            invitation->Cancel();
        }

        // And end connect mode on the transport
        transport->EndConnect(transaction.GetTransport().GetInterface());
      }

      // Save the sessions etc we are actually using
      // If we are in the EstablishedPhase, then the 
      // sessions are kept identical because the response is the
      // response to a hold/retrieve
      rtpSessions = ((SIPInvite &)transaction).GetSessionManager();
      localPartyAddress = transaction.GetMIME().GetFrom();
      remotePartyAddress = response.GetMIME().GetTo();
      UpdateRemotePartyNameAndNumber();

      response.GetMIME().GetProductInfo(remoteProductInfo);

      // get the route set from the Record-Route response field (in reverse order)
      // according to 12.1.2
      // requests in a dialog do not modify the initial route set fo according 
      // to 12.2
      if (phase < ConnectedPhase) {
        PStringList recordRoute = response.GetMIME().GetRecordRoute();
        routeSet.RemoveAll();
        for (i = recordRoute.GetSize(); i > 0; i--)
          routeSet.AppendString(recordRoute[i-1]);
      }

      // If we are in a dialog or create one, then targetAddress needs to be set
      // to the contact field in the 2xx/1xx response for a target refresh 
      // request
      PString contact = response.GetMIME().GetContact();
      if (!contact.IsEmpty()) {
        targetAddress = contact;
        PTRACE(4, "SIP\tSet targetAddress to " << targetAddress);
      }
    }

    // Send the ack if not pending
    if (response.GetStatusCode()/100 != 1)
      SendACK(transaction, response);

    if (phase < ConnectedPhase) {
      // Final check to see if we have forked INVITEs still running, don't
      // release connection until all of them have failed.
      for (PSafePtr<SIPTransaction> invitation(forkedInvitations, PSafeReference); invitation != NULL; ++invitation) {
        if (invitation->IsInProgress()) {
          ignoreErrorResponse = PTrue;
          break;
        }
      }
    }
    else {
      // This INVITE is from a different "dialog", any errors do not cause a release
      ignoreErrorResponse = localPartyAddress != response.GetMIME().GetFrom() || remotePartyAddress != response.GetMIME().GetTo();
    }
  }

  // Break out to virtual functions for some special cases.
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
      if (OnReceivedAuthenticationRequired(transaction, response))
        return;
      break;

    case SIP_PDU::Redirection_MovedTemporarily :
      OnReceivedRedirection(response);
      break;

    default :
      break;
  }

  switch (response.GetStatusCode()/100) {
    case 1 : // Do nothing for Provisional responses
      return;

    case 2 : // Successful esponse - there really is only 200 OK
      OnReceivedOK(transaction, response);
      return;

    case 3 : // Redirection response
      return;
  }

  // If we are doing a local hold, and fail, we do not release the conneciton
  if (reInvite && local_hold) {
    local_hold = PFalse;       // It failed
    PauseMediaStreams(PFalse); // Un-Pause the media streams
    endpoint.OnHold(*this);   // Signal the manager that there is no more hold
    return;
  }

  // We don't always release the connection, eg not till all forked invites have completed
  if (ignoreErrorResponse)
    return;

  // All other responses are errors, see if they should cause a Release()
  for (i = 0; i < PARRAYSIZE(SIPCodeToReason); i++) {
    if (response.GetStatusCode() == SIPCodeToReason[i].code) {
      releaseMethod = ReleaseWithNothing;
      SetQ931Cause(SIPCodeToReason[i].q931Cause);
      Release(SIPCodeToReason[i].reason);
      break;
    }
  }
}


void SIPConnection::OnReceivedINVITE(SIP_PDU & request)
{
  PBoolean isReinvite;

  const SIPMIMEInfo & requestMIME = request.GetMIME();
  PString requestTo = requestMIME.GetTo();
  PString requestFrom = requestMIME.GetFrom();

  if (IsOriginating()) {
    if (remotePartyAddress != requestFrom || localPartyAddress != requestTo) {
      PTRACE(2, "SIP\tIgnoring INVITE from " << request.GetURI() << " when originated call.");
      SIP_PDU response(request, SIP_PDU::Failure_LoopDetected);
      SendPDU(response, request.GetViaAddress(endpoint));
      return;
    }

    // We originated the call, so any INVITE must be a re-INVITE, 
    isReinvite = PTrue;
  }
  else if (originalInvite == NULL) {
    isReinvite = PFalse; // First time incoming call
    PTRACE(4, "SIP\tInitial INVITE from " << request.GetURI());
  }
  else {
    // Have received multiple INVITEs, three possibilities
    const SIPMIMEInfo & originalMIME = originalInvite->GetMIME();

    if (originalMIME.GetCSeq() == requestMIME.GetCSeq()) {
      // Same sequence number means it is a retransmission
      PTRACE(3, "SIP\tIgnoring duplicate INVITE from " << request.GetURI());
      return;
    }

    // Different "dialog" determined by the tags in the to and from fields indicate forking
    PString fromTag = request.GetMIME().GetFieldParameter("tag", requestFrom);
    PString origFromTag = originalInvite->GetMIME().GetFieldParameter("tag", originalMIME.GetFrom());
    PString toTag = request.GetMIME().GetFieldParameter("tag", requestTo);
    PString origToTag = originalInvite->GetMIME().GetFieldParameter("tag", originalMIME.GetTo());
    if (fromTag != origFromTag || toTag != origToTag) {
      PTRACE(3, "SIP\tIgnoring forked INVITE from " << request.GetURI());
      SIP_PDU response(request, SIP_PDU::Failure_LoopDetected);
      response.GetMIME().SetProductInfo(endpoint.GetUserAgent(), GetProductInfo());
      SendPDU(response, request.GetViaAddress(endpoint));
      return;
    }

    // A new INVITE in the same "dialog" but different cseq must be a re-INVITE
    isReinvite = PTrue;
  }
   

  // originalInvite should contain the first received INVITE for
  // this connection
  delete originalInvite;
  originalInvite = new SIP_PDU(request);

  if (request.HasSDP())
    remoteSDP = request.GetSDP();

  // We received a Re-INVITE for a current connection
  if (isReinvite) { 
    OnReceivedReINVITE(request);
    return;
  }
  
  releaseMethod = ReleaseWithResponse;

  // Fill in all the various connection info
  SIPMIMEInfo & mime = originalInvite->GetMIME();
  remotePartyAddress = mime.GetFrom(); 
  UpdateRemotePartyNameAndNumber();
  mime.GetProductInfo(remoteProductInfo);
  localPartyAddress  = mime.GetTo() + ";tag=" + OpalGloballyUniqueID().AsString(); // put a real random 
  mime.SetTo(localPartyAddress);

  // get the called destination
  calledDestinationName   = originalInvite->GetURI().GetDisplayName(PFalse);   
  calledDestinationNumber = originalInvite->GetURI().GetUserName();
  calledDestinationURL    = originalInvite->GetURI().AsString();

  // update the target address
  PString contact = mime.GetContact();
  if (!contact.IsEmpty()) 
    targetAddress = contact;
  targetAddress.AdjustForRequestURI();
  PTRACE(4, "SIP\tSet targetAddress to " << targetAddress);

  // get the address that remote end *thinks* it is using from the Contact field
  PIPSocket::Address sigAddr;
  PIPSocket::GetHostAddress(targetAddress.GetHostName(), sigAddr);  

  // get the local and peer transport addresses
  PIPSocket::Address peerAddr, localAddr;
  transport->GetRemoteAddress().GetIpAddress(peerAddr);
  transport->GetLocalAddress().GetIpAddress(localAddr);

  // allow the application to determine if RTP NAT is enabled or not
  remoteIsNAT = IsRTPNATEnabled(localAddr, peerAddr, sigAddr, PTrue);

  // indicate the other is to start ringing (but look out for clear calls)
  if (!OnIncomingConnection(0, NULL)) {
    PTRACE(1, "SIP\tOnIncomingConnection failed for INVITE from " << request.GetURI() << " for " << *this);
    Release();
    return;
  }

  PTRACE(3, "SIP\tOnIncomingConnection succeeded for INVITE from " << request.GetURI() << " for " << *this);
  SetPhase(SetUpPhase);

  if (!OnOpenIncomingMediaChannels()) {
    PTRACE(1, "SIP\tOnOpenIncomingMediaChannels failed for INVITE from " << request.GetURI() << " for " << *this);
    Release();
    return;
  }
}


void SIPConnection::OnReceivedReINVITE(SIP_PDU & request)
{
  if (phase != EstablishedPhase) {
    PTRACE(2, "SIP\tRe-INVITE from " << request.GetURI() << " received before initial INVITE completed on " << *this);
    SIP_PDU response(request, SIP_PDU::Failure_NotAcceptableHere);
    SendPDU(response, request.GetViaAddress(endpoint));
    return;
  }

  PTRACE(3, "SIP\tReceived re-INVITE from " << request.GetURI() << " for " << *this);

  // always send Trying for Re-INVITE
  SendInviteResponse(SIP_PDU::Information_Trying);

  remoteFormatList.RemoveAll();
  SDPSessionDescription sdpOut(GetLocalAddress());

  // get the remote media formats, if any
  if (originalInvite->HasSDP()) {

    SDPSessionDescription & sdpIn = originalInvite->GetSDP();
    // The Re-INVITE can be sent to change the RTP Session parameters,
    // the current codecs, or to put the call on hold
    if (sdpIn.GetDirection(OpalMediaSessionId("audio", 1)) == SDPMediaDescription::SendOnly &&
        sdpIn.GetDirection(OpalMediaSessionId("video", 2)) == SDPMediaDescription::SendOnly) {

      PTRACE(3, "SIP\tRemote hold detected");
      remote_hold = PTrue;
      PauseMediaStreams(PTrue);
      endpoint.OnHold(*this);
    }
    else {

      // If we receive a consecutive reinvite without the SendOnly
      // parameter, then we are not on hold anymore
      if (remote_hold) {
        PTRACE(3, "SIP\tRemote retrieve from hold detected");
        remote_hold = PFalse;
        PauseMediaStreams(PFalse);
        endpoint.OnHold(*this);
      }
    }
  }
  else {
    if (remote_hold) {
      PTRACE(3, "SIP\tRemote retrieve from hold without SDP detected");
      remote_hold = PFalse;
      PauseMediaStreams(PFalse);
      endpoint.OnHold(*this);
    }
  }
  
  // If it is a RE-INVITE that doesn't correspond to a HOLD, then
  // Close all media streams, they will be reopened.
  if (!IsConnectionOnHold()) {
    RemoveMediaStreams();
    GetCall().RemoveMediaStreams();
    ReleaseSession(OpalMediaSessionId("audio", 1), PTrue);
#if OPAL_VIDEO
    ReleaseSession(OpalMediaSessionId("video", 2), PTrue);
#endif
#if OPAL_T38FAX
    ReleaseSession(OpalMediaSessionId("image", 3), PTrue);
#endif
  }

  PBoolean sdpOK;

  if (originalInvite->HasSDP()) {

    // Try to send SDP media description for audio and video
    // Use |= to avoid McCarthy boolean || from not calling video/fax
    SDPSessionDescription & sdpIn = originalInvite->GetSDP();
    sdpOK  = OnSendSDPMediaDescription(sdpIn, OpalMediaSessionId("audio"), sdpOut);
#if OPAL_VIDEO
    sdpOK |= OnSendSDPMediaDescription(sdpIn, OpalMediaSessionId("video"), sdpOut);
#endif
#if OPAL_T38FAX
    sdpOK |= OnSendSDPMediaDescription(sdpIn, OpalMediaSessionId("image"),  sdpOut);
#endif
  }

  else {

    // construct offer as per RFC 3261, para 14.2
    // Use |= to avoid McCarthy boolean || from not calling video/fax
    SDPSessionDescription *sdp = &sdpOut;
    sdpOK  = BuildSDP(sdp, rtpSessions, OpalMediaSessionId("audio"));
#if OPAL_VIDEO
    sdpOK |= BuildSDP(sdp, rtpSessions, OpalMediaSessionId("video"));
#endif
#if OPAL_T38FAX
    sdpOK |= BuildSDP(sdp, rtpSessions, OpalMediaSessionId("image"));
#endif
  }

  // send the 200 OK response
  if (sdpOK) 
    SendInviteOK(sdpOut);
  else
    SendInviteResponse(SIP_PDU::Failure_NotAcceptableHere);
}


PBoolean SIPConnection::OnOpenIncomingMediaChannels()
{
  ApplyStringOptions();

  // in some circumstances, the peer OpalConnection needs to see the newly arrived media formats
  // before it knows what what formats can support. 
  if (originalInvite != NULL) {

    OpalMediaFormatList previewFormats;

    SDPSessionDescription & sdp = originalInvite->GetSDP();
    static struct PreviewType {
      const char * mediaType;
      unsigned sessionID;
    } previewTypes[] = 
    { 
      { "audio", 1 },
#if OPAL_VIDEO
      { "video", 2 },
#endif
#if OPAL_T38FAX
      { "image", 3 },
#endif
    };
    PINDEX i;
    for (i = 0; i < (PINDEX) (sizeof(previewTypes)/sizeof(previewTypes[0])); ++i) {
      SDPMediaDescription * mediaDescription = sdp.GetMediaDescription(previewTypes[i].mediaType);
      if (mediaDescription != NULL) {
        previewFormats += mediaDescription->GetMediaFormats(previewTypes[i].mediaType);
        OpalTransportAddress mediaAddress = mediaDescription->GetTransportAddress();
        mediaTransportAddresses.SetAt(previewTypes[i].sessionID, new OpalTransportAddress(mediaAddress));
      }
    }

    if (previewFormats.GetSize() != 0) 
      ownerCall.GetOtherPartyConnection(*this)->PreviewPeerMediaFormats(previewFormats);
  }

  ownerCall.OnSetUp(*this);

  AnsweringCall(OnAnswerCall(remotePartyAddress));
  return PTrue;
}


void SIPConnection::AnsweringCall(AnswerCallResponse response)
{
  switch (phase) {
    case SetUpPhase:
    case AlertingPhase:
      switch (response) {
        case AnswerCallNow:
        case AnswerCallProgress:
          SetConnected();
          break;

        case AnswerCallDenied:
          PTRACE(2, "SIP\tApplication has declined to answer incoming call");
          Release(EndedByAnswerDenied);
          break;

        case AnswerCallPending:
          SetAlerting(GetLocalPartyName(), PFalse);
          break;

        case AnswerCallAlertWithMedia:
          SetAlerting(GetLocalPartyName(), PTrue);
          break;

        default:
          break;
      }
      break;

    // 
    // cannot answer call in any of the following phases
    //
    case ConnectedPhase:
    case UninitialisedPhase:
    case EstablishedPhase:
    case ReleasingPhase:
    case ReleasedPhase:
    default:
      break;
  }
}


void SIPConnection::OnReceivedACK(SIP_PDU & response)
{
  if (originalInvite == NULL) {
    PTRACE(2, "SIP\tACK from " << response.GetURI() << " received before INVITE!");
    return;
  }

  // Forked request
  PString fromTag = response.GetMIME().GetFieldParameter("tag", response.GetMIME().GetFrom());
  PString origFromTag = originalInvite->GetMIME().GetFieldParameter("tag", originalInvite->GetMIME().GetFrom());
  PString toTag = response.GetMIME().GetFieldParameter("tag", response.GetMIME().GetTo());
  PString origToTag = originalInvite->GetMIME().GetFieldParameter("tag", originalInvite->GetMIME().GetTo());
  if (fromTag != origFromTag || toTag != origToTag) {
    PTRACE(3, "SIP\tACK received for forked INVITE from " << response.GetURI());
    return;
  }

  PTRACE(3, "SIP\tACK received: " << phase);

  ackTimer.Stop();
  
  OnReceivedSDP(response);

  // If we receive an ACK in established phase, perhaps it
  // is a re-INVITE
  if (phase == EstablishedPhase && !IsConnectionOnHold()) {
    OpalConnection::OnConnected ();
    StartMediaStreams();
  }
  
  releaseMethod = ReleaseWithBYE;
  if (phase != ConnectedPhase)  
    return;
  
  SetPhase(EstablishedPhase);
  OnEstablished();

  // HACK HACK HACK: this is a work-around for a deadlock that can occur
  // during incoming calls. What is needed is a proper resequencing that
  // avoids this problem
  // start all of the media threads for the connection
  StartMediaStreams();
}


void SIPConnection::OnReceivedOPTIONS(SIP_PDU & /*request*/)
{
  PTRACE(2, "SIP\tOPTIONS not yet supported");
}


void SIPConnection::OnReceivedNOTIFY(SIP_PDU & pdu)
{
  PCaselessString event, state;
  
  if (referTransaction == NULL){
    PTRACE(2, "SIP\tNOTIFY in a connection only supported for REFER requests");
    return;
  }
  
  event = pdu.GetMIME().GetEvent();
  
  // We could also compare the To and From tags
  if (pdu.GetMIME().GetCallID() != referTransaction->GetMIME().GetCallID()
      || event.Find("refer") == P_MAX_INDEX) {

    SIP_PDU response(pdu, SIP_PDU::Failure_BadEvent);
    SendPDU(response, pdu.GetViaAddress(endpoint));
    return;
  }

  state = pdu.GetMIME().GetSubscriptionState();
  
  SIP_PDU response(pdu, SIP_PDU::Successful_OK);
  SendPDU(response, pdu.GetViaAddress(endpoint));
  
  // The REFER is over
  if (state.Find("terminated") != P_MAX_INDEX) {
    referTransaction->WaitForCompletion();
    referTransaction = (SIPTransaction *)NULL;

    // Release the connection
    if (phase < ReleasingPhase) 
    {
      releaseMethod = ReleaseWithBYE;
      Release(OpalConnection::EndedByCallForwarded);
    }
  }

  // The REFER is not over yet, ignore the state of the REFER for now
}


void SIPConnection::OnReceivedREFER(SIP_PDU & pdu)
{
  SIPTransaction *notifyTransaction = NULL;
  PString referto = pdu.GetMIME().GetReferTo();
  
  if (referto.IsEmpty()) {
    SIP_PDU response(pdu, SIP_PDU::Failure_BadEvent);
    SendPDU(response, pdu.GetViaAddress(endpoint));
    return;
  }    

  SIP_PDU response(pdu, SIP_PDU::Successful_Accepted);
  SendPDU(response, pdu.GetViaAddress(endpoint));
  releaseMethod = ReleaseWithNothing;

  endpoint.SetupTransfer(GetToken(),  
                         PString (), 
                         referto,  
                         NULL);
  
  // Send a Final NOTIFY,
  notifyTransaction = new SIPReferNotify(*this, *transport, SIP_PDU::Successful_Accepted);
  notifyTransaction->WaitForCompletion();
}


void SIPConnection::OnReceivedBYE(SIP_PDU & request)
{
  PTRACE(3, "SIP\tBYE received for call " << request.GetMIME().GetCallID());
  SIP_PDU response(request, SIP_PDU::Successful_OK);
  SendPDU(response, request.GetViaAddress(endpoint));
  
  if (phase >= ReleasingPhase) {
    PTRACE(2, "SIP\tAlready released " << *this);
    return;
  }
  releaseMethod = ReleaseWithNothing;
  
  remotePartyAddress = request.GetMIME().GetFrom();
  UpdateRemotePartyNameAndNumber();
  response.GetMIME().GetProductInfo(remoteProductInfo);

  Release(EndedByRemoteUser);
}


void SIPConnection::OnReceivedCANCEL(SIP_PDU & request)
{
  PString origTo;

  // Currently only handle CANCEL requests for the original INVITE that
  // created this connection, all else ignored
  // Ignore the tag added by OPAL
  if (originalInvite != NULL) {
    origTo = originalInvite->GetMIME().GetTo();
    origTo.Delete(origTo.Find(";tag="), P_MAX_INDEX);
  }
  if (originalInvite == NULL || 
      request.GetMIME().GetTo() != origTo || 
      request.GetMIME().GetFrom() != originalInvite->GetMIME().GetFrom() || 
      request.GetMIME().GetCSeqIndex() != originalInvite->GetMIME().GetCSeqIndex()) {
    PTRACE(2, "SIP\tUnattached " << request << " received for " << *this);
    SIP_PDU response(request, SIP_PDU::Failure_TransactionDoesNotExist);
    SendPDU(response, request.GetViaAddress(endpoint));
    return;
  }

  PTRACE(3, "SIP\tCancel received for " << *this);

  SIP_PDU response(request, SIP_PDU::Successful_OK);
  SendPDU(response, request.GetViaAddress(endpoint));
  
  if (!IsOriginating())
    Release(EndedByCallerAbort);
}


void SIPConnection::OnReceivedTrying(SIP_PDU & /*response*/)
{
  PTRACE(3, "SIP\tReceived Trying response");
}


void SIPConnection::OnReceivedRinging(SIP_PDU & /*response*/)
{
  PTRACE(3, "SIP\tReceived Ringing response");

  if (phase < AlertingPhase)
  {
    SetPhase(AlertingPhase);
    OnAlerting();
  }
}


void SIPConnection::OnReceivedSessionProgress(SIP_PDU & response)
{
  PTRACE(3, "SIP\tReceived Session Progress response");

  OnReceivedSDP(response);

  if (phase < AlertingPhase)
  {
    SetPhase(AlertingPhase);
    OnAlerting();
  }

  PTRACE(4, "SIP\tStarting receive media to annunciate remote progress tones");
  StartMediaStreams();
}


void SIPConnection::OnReceivedRedirection(SIP_PDU & response)
{
  targetAddress = response.GetMIME().GetContact();
  remotePartyAddress = targetAddress.AsQuotedString();
  PINDEX j;
  if ((j = remotePartyAddress.Find (';')) != P_MAX_INDEX)
    remotePartyAddress = remotePartyAddress.Left(j);

  endpoint.ForwardConnection (*this, remotePartyAddress);
}


PBoolean SIPConnection::OnReceivedAuthenticationRequired(SIPTransaction & transaction,
                                                     SIP_PDU & response)
{
  PBoolean isProxy = response.GetStatusCode() == SIP_PDU::Failure_ProxyAuthenticationRequired;
  SIPURL proxy;
  SIPAuthentication auth;
  PString lastUsername;
  PString lastNonce;

#if PTRACING
  const char * proxyTrace = isProxy ? "Proxy " : "";
#endif
  
  if (transaction.GetMethod() != SIP_PDU::Method_INVITE) {
    PTRACE(1, "SIP\tCannot do " << proxyTrace << "Authentication Required for non INVITE");
    return PFalse;
  }

  PTRACE(3, "SIP\tReceived " << proxyTrace << "Authentication Required response");

  PCaselessString authenticateTag = isProxy ? "Proxy-Authenticate" : "WWW-Authenticate";

  // Received authentication required response, try to find authentication
  // for the given realm if no proxy
  if (!auth.Parse(response.GetMIME()(authenticateTag), isProxy)) {
    return PFalse;
  }

  // Save the username, realm and nonce
  lastUsername = auth.GetUsername();
  lastNonce = auth.GetNonce();

  // Try to find authentication parameters for the given realm,
  // if not, use the proxy authentication parameters (if any)
  if (!endpoint.GetAuthentication(auth.GetAuthRealm(), authentication)) {
    PTRACE (3, "SIP\tCouldn't find authentication information for realm " << auth.GetAuthRealm()
            << ", will use SIP Outbound Proxy authentication settings, if any");
    if (endpoint.GetProxy().IsEmpty())
      return PFalse;

    authentication.SetUsername(endpoint.GetProxy().GetUserName());
    authentication.SetPassword(endpoint.GetProxy().GetPassword());
  }

  if (!authentication.Parse(response.GetMIME()(authenticateTag), isProxy))
    return PFalse;
  
  if (!authentication.IsValid() || (lastUsername == authentication.GetUsername() && lastNonce == authentication.GetNonce())) {
    PTRACE(2, "SIP\tAlready done INVITE for " << proxyTrace << "Authentication Required");
    return PFalse;
  }

  // Restart the transaction with new authentication info
  // and start with a fresh To tag
  // Section 8.1.3.5 of RFC3261 tells that the authenticated
  // request SHOULD have the same value of the Call-ID, To and From.
  PINDEX j;
  if ((j = remotePartyAddress.Find (';')) != P_MAX_INDEX)
    remotePartyAddress = remotePartyAddress.Left(j);
  
  if (proxy.IsEmpty())
    proxy = endpoint.GetProxy();

  // Default routeSet if there is a proxy
  if (!proxy.IsEmpty() && routeSet.GetSize() == 0) 
    routeSet += "sip:" + proxy.GetHostName() + ':' + PString(proxy.GetPort()) + ";lr";

  RTP_SessionManager & origRtpSessions = ((SIPInvite &)transaction).GetSessionManager();
  SIPTransaction * invite = new SIPInvite(*this, *transport, origRtpSessions);
  if (invite->Start())
  {
    forkedInvitations.Append(invite);
    return PTrue;
  }

  PTRACE(2, "SIP\tCould not restart INVITE for " << proxyTrace << "Authentication Required");
  return PFalse;
}


void SIPConnection::OnReceivedOK(SIPTransaction & transaction, SIP_PDU & response)
{
  if (transaction.GetMethod() != SIP_PDU::Method_INVITE) {
    PTRACE(2, "SIP\tReceived OK response for non INVITE");
    return;
  }

  PTRACE(3, "SIP\tReceived INVITE OK response");

  OnReceivedSDP(response);

  releaseMethod = ReleaseWithBYE;

  connectedTime = PTime ();
  OnConnected();                        // start media streams

  if (phase == EstablishedPhase)
    return;

  SetPhase(EstablishedPhase);
  OnEstablished();
}


void SIPConnection::OnReceivedSDP(SIP_PDU & pdu)
{
  if (!pdu.HasSDP())
    return;

  remoteSDP = pdu.GetSDP();

  const SDPMediaDescriptionList & mediaDescs = remoteSDP.GetMediaDescriptions();
  PINDEX i;
  for (i = 0; i < mediaDescs.GetSize(); ++i) {
    SDPMediaDescription & mediaDescription = *(SDPMediaDescription *)mediaDescs.GetAt(i);
    OnReceivedSDPMediaDescription(remoteSDP, OpalMediaSessionId(mediaDescription.GetMediaType(), mediaDescription.GetPort()));
  }

  remoteFormatList += OpalRFC2833;
}


PBoolean SIPConnection::OnReceivedSDPMediaDescription(SDPSessionDescription & sdp, const OpalMediaSessionId & sessionId)
{
  RTP_UDP *rtpSession = NULL;
  SDPMediaDescription * mediaDescription = sdp.GetMediaDescription(sessionId.sessionId);
  
  if (mediaDescription == NULL) {
    PTRACE(sessionId.mediaType == "audio" ? 2 : 3, "SIP\tCould not find SDP media description for session " << sessionId.mediaType);
    return PFalse;
  }

  {
    // see if the remote supports this media
    OpalMediaFormatList mediaFormatList = mediaDescription->GetMediaFormats(sessionId.mediaType);
    if (mediaFormatList.GetSize() == 0) {
      PTRACE(1, "SIP\tCould not find media formats in SDP media description for session " << sessionId.mediaType);
      return PFalse;
    }

    // create the RTPSession
    OpalTransportAddress localAddress;
    OpalTransportAddress address = mediaDescription->GetTransportAddress();
    rtpSession = OnUseRTPSession(sessionId, address, localAddress);
    if (rtpSession == NULL && !ownerCall.IsMediaBypassPossible(*this, sessionId)) {
      if (sessionId.mediaType == "audio") 
        Release(EndedByTransportFail);
      return PFalse;
    }

    // set the remote address 
    PIPSocket::Address ip;
    WORD port;
    if (!address.GetIpAndPort(ip, port) || (rtpSession && !rtpSession->SetRemoteSocketInfo(ip, port, PTrue))) {
      PTRACE(1, "SIP\tCannot set remote ports on RTP session");
      if (sessionId.mediaType == "audio") 
        Release(EndedByTransportFail);
      return PFalse;
    }

    // When receiving an answer SDP, keep the remote SDP media formats order
    // but remove the media formats we do not support.
    remoteFormatList += mediaFormatList;
    remoteFormatList.Remove(endpoint.GetManager().GetMediaFormatMask());

    // create map for RTP payloads
    mediaDescription->CreateRTPMap(sessionId, rtpPayloadMap);
    
    // Open the streams and the reverse streams
    if (!OnOpenSourceMediaStreams(remoteFormatList, sessionId, NULL))
      return PFalse;
  }

  return PTrue;
}


void SIPConnection::OnCreatingINVITE(SIP_PDU & /*request*/)
{
  PTRACE(3, "SIP\tCreating INVITE request");
}


void SIPConnection::QueuePDU(SIP_PDU * pdu)
{
  if (PAssertNULL(pdu) == NULL)
    return;

  if (phase >= ReleasedPhase) {
    if(pdu->GetMethod() != SIP_PDU::NumMethods)
    {
      PTRACE(4, "SIP\tIgnoring PDU: " << *pdu);
    }
    else
    {
      PTRACE(4, "SIP\tProcessing PDU: " << *pdu);
      OnReceivedPDU(*pdu);
    }
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
  PTRACE(4, "SIP\tPDU handler thread started.");

  while (phase != ReleasedPhase) {
    PTRACE(4, "SIP\tAwaiting next PDU.");
    pduSemaphore.Wait();

    if (!LockReadWrite())
      break;

    SIP_PDU * pdu = pduQueue.Dequeue();

    UnlockReadWrite();

    if (pdu != NULL) {
      OnReceivedPDU(*pdu);
      delete pdu;
    }
  }

  SafeDereference();

  PTRACE(4, "SIP\tPDU handler thread finished.");
}


void SIPConnection::OnAckTimeout(PThread &, INT)
{
  releaseMethod = ReleaseWithBYE;
  Release(EndedByTemporaryFailure);
}


PBoolean SIPConnection::ForwardCall (const PString & fwdParty)
{
  if (fwdParty.IsEmpty ())
    return PFalse;
  
  forwardParty = fwdParty;
  PTRACE(2, "SIP\tIncoming SIP connection will be forwarded to " << forwardParty);
  Release(EndedByCallForwarded);

  return PTrue;
}


PBoolean SIPConnection::SendInviteOK(const SDPSessionDescription & sdp)
{
  SIPURL localPartyURL(GetLocalPartyAddress());
  PString userName = endpoint.GetRegisteredPartyName(localPartyURL).GetUserName();
  SIPURL contact = endpoint.GetContactURL(*transport, userName, localPartyURL.GetHostName());

  return SendInviteResponse(SIP_PDU::Successful_OK, (const char *) contact.AsQuotedString(), NULL, &sdp);
}

PBoolean SIPConnection::SendACK(SIPTransaction & invite, SIP_PDU & response)
{
  if (invite.GetMethod() != SIP_PDU::Method_INVITE) { // Sanity check
    return PFalse;
  }

  SIP_PDU ack;
  // ACK Constructed following 17.1.1.3
  if (response.GetStatusCode()/100 != 2) {
    ack = SIPAck(endpoint, invite, response); 
  } else { 
    ack = SIPAck(invite);
  }

  // Send the PDU using the connection transport
  if (!SendPDU(ack, ack.GetSendAddress(ack.GetMIME().GetRoute()))) {
    Release(EndedByTransportFail);
    return PFalse;
  }

  return PTrue;
}

PBoolean SIPConnection::SendInviteResponse(SIP_PDU::StatusCodes code, const char * contact, const char * extra, const SDPSessionDescription * sdp)
{
  if (NULL == originalInvite)
    return PFalse;

  SIP_PDU response(*originalInvite, code, contact, extra);
  if (NULL != sdp)
    response.SetSDP(*sdp);
  response.GetMIME().SetProductInfo(endpoint.GetUserAgent(), GetProductInfo());

  if (response.GetStatusCode()/100 != 1)
    ackTimer = endpoint.GetAckTimeout();

  return SendPDU(response, originalInvite->GetViaAddress(endpoint)); 
}


PBoolean SIPConnection::SendPDU(SIP_PDU & pdu, const OpalTransportAddress & address)
{
  PWaitAndSignal m(transportMutex); 
  return transport != NULL && pdu.Write(*transport, address);
}

void SIPConnection::OnRTPStatistics(const RTP_Session & session) const
{
  endpoint.OnRTPStatistics(*this, session);
}

void SIPConnection::OnReceivedINFO(SIP_PDU & pdu)
{
  SIP_PDU::StatusCodes status = SIP_PDU::Failure_UnsupportedMediaType;
  SIPMIMEInfo & mimeInfo = pdu.GetMIME();
  PString contentType = mimeInfo.GetContentType();

  if (contentType *= ApplicationDTMFRelayKey) {
    PStringArray lines = pdu.GetEntityBody().Lines();
    PINDEX i;
    char tone = -1;
    int duration = -1;
    for (i = 0; i < lines.GetSize(); ++i) {
      PStringArray tokens = lines[i].Tokenise('=', PFalse);
      PString val;
      if (tokens.GetSize() > 1)
        val = tokens[1].Trim();
      if (tokens.GetSize() > 0) {
        if (tokens[0] *= "signal")
          tone = val[0];
        else if (tokens[0] *= "duration")
          duration = val.AsInteger();
      }
    }
    if (tone != -1)
      OnUserInputTone(tone, duration == 0 ? 100 : tone);
    status = SIP_PDU::Successful_OK;
  }

  else if (contentType *= ApplicationDTMFKey) {
    OnUserInputString(pdu.GetEntityBody().Trim());
    status = SIP_PDU::Successful_OK;
  }

#if OPAL_VIDEO
  else if (contentType *= ApplicationMediaControlXMLKey) {
    if (OnMediaControlXML(pdu))
      return;
    status = SIP_PDU::Failure_UnsupportedMediaType;
  }
#endif

  else 
    status = SIP_PDU::Failure_UnsupportedMediaType;

  SIP_PDU response(pdu, status);
  SendPDU(response, pdu.GetViaAddress(endpoint));
}


void SIPConnection::OnReceivedPING(SIP_PDU & pdu)
{
  PTRACE(3, "SIP\tReceived PING");
  SIP_PDU response(pdu, SIP_PDU::Successful_OK);
  SendPDU(response, pdu.GetViaAddress(endpoint));
}


OpalConnection::SendUserInputModes SIPConnection::GetRealSendUserInputMode() const
{
  switch (sendUserInputMode) {
    case SendUserInputAsString:
    case SendUserInputAsTone:
      return sendUserInputMode;
    default:
      break;
  }

  return SendUserInputAsInlineRFC2833;
}

PBoolean SIPConnection::SendUserInputTone(char tone, unsigned duration)
{
  SendUserInputModes mode = GetRealSendUserInputMode();

  PTRACE(3, "SIP\tSendUserInputTone('" << tone << "', " << duration << "), using mode " << mode);

  switch (mode) {
    case SendUserInputAsTone:
    case SendUserInputAsString:
      {
        PSafePtr<SIPTransaction> infoTransaction = new SIPTransaction(*this, *transport, SIP_PDU::Method_INFO);
        SIPMIMEInfo & mimeInfo = infoTransaction->GetMIME();
        PStringStream str;
        if (mode == SendUserInputAsTone) {
          mimeInfo.SetContentType(ApplicationDTMFRelayKey);
          str << "Signal=" << tone << "\r\n" << "Duration=" << duration << "\r\n";
        }
        else {
          mimeInfo.SetContentType(ApplicationDTMFKey);
          str << tone;
        }
        infoTransaction->GetEntityBody() = str;
        infoTransaction->WaitForCompletion();
        return !infoTransaction->IsFailed();
      }

    // anything else - send as RFC 2833
    case SendUserInputAsProtocolDefault:
    default:
      break;
  }

  return OpalConnection::SendUserInputTone(tone, duration);
}

#if OPAL_VIDEO
class QDXML 
{
  public:
    struct statedef {
      int currState;
      const char * str;
      int newState;
    };

    virtual ~QDXML() {}

    bool ExtractNextElement(std::string & str)
    {
      while (isspace(*ptr))
        ++ptr;
      if (*ptr != '<')
        return false;
      ++ptr;
      if (*ptr == '\0')
        return false;
      const char * start = ptr;
      while (*ptr != '>') {
        if (*ptr == '\0')
          return false;
        ++ptr;
      }
      ++ptr;
      str = std::string(start, ptr-start-1);
      return true;
    }

    int Parse(const std::string & xml, const statedef * states, unsigned numStates)
    {
      ptr = xml.c_str(); 
      state = 0;
      std::string str;
      while ((state >= 0) && ExtractNextElement(str)) {
        cout << state << "  " << str << endl;
        unsigned i;
        for (i = 0; i < numStates; ++i) {
          cout << "comparing '" << str << "' to '" << states[i].str << "'" << endl;
          if ((state == states[i].currState) && (str.compare(0, strlen(states[i].str), states[i].str) == 0)) {
            state = states[i].newState;
            break;
          }
        }
        if (i == numStates) {
          cout << "unknown string " << str << " in state " << state << endl;
          state = -1;
          break;
        }
        if (!OnMatch(str)) {
          state = -1;
          break; 
        }
      }
      return state;
    }

    virtual bool OnMatch(const std::string & )
    { return true; }

  protected:
    int state;
    const char * ptr;
};

class VFUXML : public QDXML
{
  public:
    bool vfu;

    VFUXML()
    { vfu = false; }

    PBoolean Parse(const std::string & xml)
    {
      static const struct statedef states[] = {
        { 0, "?xml",                1 },
        { 1, "media_control",       2 },
        { 2, "vc_primitive",        3 },
        { 3, "to_encoder",          4 },
        { 4, "picture_fast_update", 5 },
        { 5, "/to_encoder",         6 },
        { 6, "/vc_primitive",       7 },
        { 7, "/media_control",      255 },
      };
      const int numStates = sizeof(states)/sizeof(states[0]);
      return QDXML::Parse(xml, states, numStates) == 255;
    }

    bool OnMatch(const std::string &)
    {
      if (state == 5)
        vfu = true;
      return true;
    }
};

PBoolean SIPConnection::OnMediaControlXML(SIP_PDU & pdu)
{
  VFUXML vfu;
  if (!vfu.Parse(pdu.GetEntityBody())) {
    SIP_PDU response(pdu, SIP_PDU::Failure_Undecipherable);
    response.GetEntityBody() = 
      "<?xml version=\"1.0\" encoding=\"utf-8\" ?>\n"
      "<media_control>\n"
      "  <general_error>\n"
      "  Unable to parse XML request\n"
      "   </general_error>\n"
      "</media_control>\n";
    SendPDU(response, pdu.GetViaAddress(endpoint));
  }
  else if (vfu.vfu) {
    if (!LockReadWrite())
      return PFalse;

    OpalMediaStream * encodingStream = GetMediaStream(OpalMediaSessionId("video", 2), PTrue);

    if (!encodingStream){
      OpalVideoUpdatePicture updatePictureCommand;
      encodingStream->ExecuteCommand(updatePictureCommand);
    }

    SIP_PDU response(pdu, SIP_PDU::Successful_OK);
    SendPDU(response, pdu.GetViaAddress(endpoint));
    
    UnlockReadWrite();
  }

  return PTrue;
}
#endif


PString SIPConnection::GetExplicitFrom() const
{
  if (!explicitFrom.IsEmpty())
    return explicitFrom;
  return GetLocalPartyAddress();
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

#if OPAL_VIDEO
void SIP_RTP_Session::OnRxIntraFrameRequest(const RTP_Session & session) const
{
  // We got an intra frame request control packet, alert the encoder.
  // We're going to grab the call, find the other connection, then grab the
  // encoding stream
  PSafePtr<OpalConnection> otherConnection = connection.GetCall().GetOtherPartyConnection(connection);
  if (otherConnection == NULL)
    return; // No other connection.  Bail.

  // Found the encoding stream, send an OpalVideoFastUpdatePicture
  OpalMediaStream * encodingStream = otherConnection->GetMediaStream(session.GetSessionID(), PTrue);
  if (encodingStream) {
    OpalVideoUpdatePicture updatePictureCommand;
    encodingStream->ExecuteCommand(updatePictureCommand);
  }
}

void SIP_RTP_Session::OnTxIntraFrameRequest(const RTP_Session & /*session*/) const
{
}
#endif

/////////////////////////////////////////////////////////////////////////////

SDPMediaDescription * OpalCommonMediaType::CreateSDPMediaDescription(const OpalMediaType & mediaType, OpalTransportAddress & localAddress)
{
  if (!localAddress.IsEmpty()) 
    return new OpalRTPAVPSDPMediaDescription(mediaType, localAddress);

  PTRACE(2, "SIP\tRefusing to add image SDP media description with no transport address");
  return NULL;
}

#if OPAL_AUDIO
SDPMediaDescription * OpalAudioMediaType::CreateSDPMediaDescription(OpalTransportAddress & localAddress)
{
  return OpalCommonMediaType::CreateSDPMediaDescription("audio", localAddress);
}
#endif

#if OPAL_VIDEO
SDPMediaDescription * OpalVideoMediaType::CreateSDPMediaDescription(OpalTransportAddress & localAddress)
{
  return OpalCommonMediaType::CreateSDPMediaDescription("video", localAddress);
}
#endif



// End of file ////////////////////////////////////////////////////////////////
