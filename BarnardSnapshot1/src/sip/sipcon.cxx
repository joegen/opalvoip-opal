
/* sipcon.cxx
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

#include <opal/buildopts.h>
#if OPAL_SIP

#ifdef __GNUC__
#pragma implementation "sipcon.h"
#endif

#include <sip/sipcon.h>

#include <sip/sipep.h>
#include <codec/rfc2833.h>
#include <opal/manager.h>
#include <opal/call.h>
#include <opal/patch.h>
#include <opal/transcoders.h>
#include <ptclib/random.h>              // for local dialog tag
#include <ptclib/pdns.h>


#define new PNEW

//
//  uncomment this to force pause ReINVITES to have c=0.0.0.0
//
//#define PAUSE_WITH_EMPTY_ADDRESS  1

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
  { SIP_PDU::Failure_RequestTimeout             , OpalConnection::EndedByNoAnswer          ,  18 }, // no user responding                   
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
  { SIP_PDU::GlobalFailure_Decline              , OpalConnection::EndedByRefusal                 }, // TODO - SGW - add for call reject from H323 side.
  { SIP_PDU::Failure_NotFound                   , OpalConnection::EndedByHostOffline             }, // TODO - SGW - add for no ip from H323 side.
  { SIP_PDU::Failure_NotFound                   , OpalConnection::EndedByNoEndPoint              }, // TODO - SGW - add for endpoints not running on a ip from H323 side.
  { SIP_PDU::Failure_Forbidden                  , OpalConnection::EndedByUnreachable             }, // TODO - SGW - add for avoid sip calls to SGW IP.
  { SIP_PDU::GlobalFailure_NotAcceptable        , OpalConnection::EndedByNoBandwidth             }, // TODO - SGW - added to reject call when no bandwidth 
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
  : OpalRTPConnection(call, ep, token, options, stringOptions)
  , endpoint(ep)
  , transport(newTransport)
  , m_holdToRemote(eHoldOff)
  , m_holdFromRemote(false)
  , originalInvite(NULL)
  , needReINVITE(false)
  , m_requestURI(destination)
  , authentication(NULL)
  , ackReceived(false)
  , releaseMethod(ReleaseWithNothing)
{
  synchronousOnRelease = false;

  // Look for a "proxy" parameter to override default proxy
  PStringToString params = m_requestURI.GetParamVars();
  SIPURL proxy;
  if (params.Contains("proxy")) {
    proxy.Parse(params("proxy"));
    m_requestURI.SetParamVar("proxy", PString::Empty());
  }

  if (proxy.IsEmpty())
    proxy = endpoint.GetProxy();

  // Default routeSet if there is a proxy
  if (!proxy.IsEmpty()) 
    routeSet += "sip:" + proxy.GetHostName() + ':' + PString(proxy.GetPort()) + ";lr";
  
  // Update remote party parameters
  UpdateRemoteAddresses(m_requestURI.AsQuotedString());

  m_requestURI.AdjustForRequestURI(),

  forkedInvitations.DisallowDeleteObjects();

  ackTimer.SetNotifier(PCREATE_NOTIFIER(OnAckTimeout));
  ackRetry.SetNotifier(PCREATE_NOTIFIER(OnInviteResponseRetry));

  sessionTimer.SetNotifier(PCREATE_NOTIFIER(OnSessionTimeout));

  PTRACE(4, "SIP\tCreated connection.");
}


void SIPConnection::UpdateRemoteAddresses(const PString & addr)
{
  m_dialogTo = addr;

  SIPURL url(addr);

  remotePartyAddress = url.AsString();
  PINDEX pos;
  if ((pos = remotePartyAddress.Find(';')) != P_MAX_INDEX)
    remotePartyAddress = remotePartyAddress.Left(pos);

  remotePartyName   = url.GetDisplayName();
  remotePartyNumber = url.GetUserName();
}

SIPConnection::~SIPConnection()
{
  delete authentication;
  delete originalInvite;
  delete transport;

  PTRACE(4, "SIP\tDeleted connection.");
}


void SIPConnection::OnReleased()
{
  PTRACE(3, "SIP\tOnReleased: " << *this << ", phase = " << GetPhase());
  
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
  if (byeTransaction != NULL) {
    byeTransaction->WaitForCompletion();
    byeTransaction.SetNULL();
  }

  // Wait until all INVITEs have completed
  for (PSafePtr<SIPTransaction> invitation(forkedInvitations, PSafeReference); invitation != NULL; ++invitation)
    invitation->WaitForCompletion();
  forkedInvitations.RemoveAll();

  if (referTransaction != NULL) {
    referTransaction->WaitForCompletion();
    referTransaction.SetNULL();
  }

  SetPhase(ReleasedPhase);

  OpalRTPConnection::OnReleased();

  if (transport != NULL)
    transport->CloseWait();
}


bool SIPConnection::TransferConnection(const PString & remoteParty)
{
  // There is still an ongoing REFER transaction 
  if (referTransaction != NULL) 
    return false;

  PSafePtr<OpalCall> call = endpoint.GetManager().FindCallWithLock(remoteParty, PSafeReadOnly);
  if (call == NULL) {
    referTransaction = new SIPRefer(*this, *transport, remoteParty, GetLocalPartyURL());
    return referTransaction->Start();
  }

  for (PSafePtr<OpalConnection> connection = call->GetConnection(0); connection != NULL; ++connection) {
    PSafePtr<SIPConnection> sip = PSafePtrCast<OpalConnection, SIPConnection>(connection);
    if (sip != NULL) {
      PStringStream referTo;
      referTo << sip->GetRemotePartyURL()
              << "?Replaces=" << sip->GetToken()
              << "%3Bto-tag%3D"   << SIPMIMEInfo::ExtractFieldParameter(sip->GetDialogFrom(), "tag") // "to/from" is from the other sides perspective
              << "%3Bfrom-tag%3D" << SIPMIMEInfo::ExtractFieldParameter(sip->GetDialogTo(), "tag");
      referTransaction = new SIPRefer(*this, *transport, referTo, GetLocalPartyURL());
      referTransaction->GetMIME().SetAt("Refer-Sub", "false"); // Use RFC4488 to indicate we are NOT doing NOTIFYs
      return referTransaction->Start();
    }
  }

  PTRACE(2, "SIP\tConsultation transfer requires other party to be SIP.");
  return false;
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

  if (GetPhase() != SetUpPhase) 
    return PFalse;

  if (!withMedia) 
    SendInviteResponse(SIP_PDU::Information_Ringing);
  else {
    SDPSessionDescription sdpOut(GetLocalAddress());
    if (!OnSendSDP(true, m_rtpSessions, sdpOut)) {
      SendInviteResponse(SIP_PDU::Failure_NotAcceptableHere);
      releaseMethod = ReleaseWithNothing;
      Release(EndedByCapabilityExchange);
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
  if (!OnSendSDP(true, m_rtpSessions, sdpOut)) {
    SendInviteResponse(SIP_PDU::Failure_NotAcceptableHere);
    releaseMethod = ReleaseWithNothing;
    Release(EndedByCapabilityExchange);
    return PFalse;
  }
    
  // send the 200 OK response
  SendInviteOK(sdpOut);

  releaseMethod = ReleaseWithBYE;
  sessionTimer = 10000;

  // switch phase and if media was previously set up, then move to Established
  return OpalConnection::SetConnected();
}


RTP_UDP *SIPConnection::OnUseRTPSession(const unsigned rtpSessionId, const OpalMediaType & mediaType, const OpalTransportAddress & mediaAddress, OpalTransportAddress & localAddress)
{
  RTP_UDP *rtpSession = NULL;
  RTP_DataFrame::PayloadTypes ntePayloadCode = RTP_DataFrame::IllegalPayloadType;

  PSafeLockReadOnly m(ownerCall);

  PSafePtr<OpalConnection> otherParty = GetCall().GetOtherPartyConnection(*this);
  if (otherParty == NULL) {
    PTRACE(2, "H323\tCowardly refusing to create an RTP channel with only one connection");
    return NULL;
   }

  // if doing media bypass, we need to set the local address
  // otherwise create an RTP session
  if (ownerCall.IsMediaBypassPossible(*this, rtpSessionId)) {
    PSafePtr<OpalRTPConnection> otherParty = PSafePtrCast<OpalConnection, OpalRTPConnection>(GetCall().GetOtherPartyConnection(*this));
    if (otherParty != NULL) {
      MediaInformation info;
      if (otherParty->GetMediaInformation(rtpSessionId, info)) {
        localAddress = info.data;
        ntePayloadCode = info.rfc2833;
      }
    }
    mediaTransportAddresses.SetAt(rtpSessionId, new OpalTransportAddress(mediaAddress));
  }
  else {
    // create an RTP session
    rtpSession = (RTP_UDP *)UseSession(GetTransport(), rtpSessionId, mediaType);
    if (rtpSession == NULL) {
      return NULL;
    }
    
    // Set user data
    if (rtpSession->GetUserData() == NULL)
      rtpSession->SetUserData(new SIP_RTP_Session(*this));

    // Local Address of the session
    localAddress = GetLocalAddress(rtpSession->GetLocalDataPort());
  }
  
  return rtpSession;
}


PBoolean SIPConnection::OnSendSDP(bool isAnswerSDP, OpalRTPSessionManager & rtpSessions, SDPSessionDescription & sdpOut)
{
  bool sdpOK = false;

  if (isAnswerSDP)
    needReINVITE = false;

  // get the remote media formats, if any
  if (isAnswerSDP && originalInvite != NULL && originalInvite->HasSDP()) {
    const SDPMediaDescriptionArray & mediaDescriptions = originalInvite->GetSDP().GetMediaDescriptions();
    for (PINDEX i = 0; i < mediaDescriptions.GetSize(); ++i) 
      sdpOK |= AnswerSDPMediaDescription(originalInvite->GetSDP(), i+1, sdpOut);
     remoteFormatList += OpalT38;
  }
  else if (needReINVITE && !mediaStreams.IsEmpty()) {
    for (OpalMediaStreamPtr stream(mediaStreams, PSafeReference); stream != NULL; ++stream) {
      if (stream->IsSource())
        sdpOK |= OfferSDPMediaDescription(stream->GetMediaFormat().GetMediaType(), stream->GetSessionID(), rtpSessions, sdpOut);
    }
  }

  else {

    // construct offer as per RFC 3261, para 14.2
    // Use |= to avoid McCarthy boolean || from not calling video/fax
    sdpOK  |= OfferSDPMediaDescription(OpalMediaType::Audio(), 0, rtpSessions, sdpOut);
#if OPAL_VIDEO
    sdpOK |= OfferSDPMediaDescription(OpalMediaType::Video(), 0, rtpSessions, sdpOut);
#endif
  }

  needReINVITE = true;

  return sdpOK;
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


bool SIPConnection::OfferSDPMediaDescription(const OpalMediaType & mediaType,
                                             unsigned rtpSessionId,
                                             OpalRTPSessionManager & rtpSessions,
                                             SDPSessionDescription & sdp)
{
  OpalTransportAddress localAddress;
  RTP_DataFrame::PayloadTypes ntePayloadCode = RTP_DataFrame::IllegalPayloadType;
  RTP_DataFrame::PayloadTypes nsePayloadCode = RTP_DataFrame::IllegalPayloadType;

  OpalMediaFormatList formats = GetLocalMediaFormats();

  // See if any media formats of this session id, so don't create unused RTP session
  OpalMediaFormatList::iterator format;
  for (format = formats.begin(); format != formats.end(); ++format) {
    if (format->GetMediaType() == mediaType && format->IsTransportable())
      break;
  }
  if (format == formats.end()) {
    PTRACE(3, "SIP\tNo media formats of type " << mediaType << ", not adding SDP");
    return PFalse;
  }

  PTRACE(3, "SIP\tOffering media type " << mediaType << " in SDP with formats\n" << setfill(',') << formats << setfill(' '));

  if (rtpSessionId == 0)
    rtpSessionId = sdp.GetMediaDescriptions().GetSize()+1;

  if (ownerCall.IsMediaBypassPossible(*this, rtpSessionId)) {
    PSafePtr<OpalRTPConnection> otherParty = PSafePtrCast<OpalConnection, OpalRTPConnection>(GetCall().GetOtherPartyConnection(*this));
    if (otherParty != NULL) {
      MediaInformation info;
      if (otherParty->GetMediaInformation(rtpSessionId, info)) {
        localAddress = info.data;
        ntePayloadCode = info.rfc2833;
        nsePayloadCode = info.ciscoNSE;
      }
    }
  }

  if (localAddress.IsEmpty()) {

    /* We are not doing media bypass, so must have some media session
       Due to the possibility of several INVITEs going out, all with different
       transport requirements, we actually need to use an rtpSession dictionary
       for each INVITE and not the one for the connection. Once an INVITE is
       accepted the rtpSessions for that INVITE is put into the connection. */
    RTP_Session * rtpSession = rtpSessions.GetSession(rtpSessionId);
    if (rtpSession == NULL) {

      // Not already there, so create one
      rtpSession = CreateSession(GetTransport(), rtpSessionId, NULL);
      if (rtpSession == NULL) {
        PTRACE(1, "SIP\tCould not create session id " << rtpSessionId << ", released " << *this);
        Release(OpalConnection::EndedByTransportFail);
        return PFalse;
      }

      rtpSession->SetUserData(new SIP_RTP_Session(*this));

      // add the RTP session to the RTP session manager in INVITE
      rtpSessions.AddSession(rtpSession, mediaType);
    }

    if (rtpSession == NULL)
      return false;

    localAddress = GetLocalAddress(((RTP_UDP *)rtpSession)->GetLocalDataPort());
  }

  if (localAddress.IsEmpty()) {
    PTRACE(2, "SIP\tRefusing to add SDP media description for session id " << rtpSessionId << " with no transport address");
    return false;
  }

  if (sdp.GetDefaultConnectAddress().IsEmpty())
    sdp.SetDefaultConnectAddress(localAddress);

  OpalMediaTypeDefinition * def;
  if (mediaType.empty() || ((def = mediaType.GetDefinition()) == NULL)) {
    PTRACE(2, "SIP\tCan't create media type for unknown default session ID " << rtpSessionId);
    return false;
  }

  SDPMediaDescription * localMedia = def->CreateSDPMediaDescription(localAddress);
  if (localMedia == NULL) {
    PTRACE(2, "SIP\tCan't create SDP media description for media type " << mediaType);
    return false;
  }

  if (needReINVITE) {
    OpalMediaStreamPtr sendStream = GetMediaStream(rtpSessionId, false);
    bool sending = sendStream != NULL && sendStream->IsOpen();
    OpalMediaStreamPtr recvStream = GetMediaStream(rtpSessionId, true);
    bool recving = recvStream != NULL && recvStream->IsOpen();
    if (sending) {
      localMedia->AddMediaFormat(sendStream->GetMediaFormat(), rtpPayloadMap);
      localMedia->SetDirection(m_holdToRemote >= eHoldOn ? recving ? SDPMediaDescription::RecvOnly : SDPMediaDescription::Inactive : recving ? SDPMediaDescription::SendRecv : SDPMediaDescription::SendOnly);
    }
    else if (recving) {
      localMedia->AddMediaFormat(recvStream->GetMediaFormat(), rtpPayloadMap);
      localMedia->SetDirection(m_holdToRemote >= eHoldOn ? SDPMediaDescription::Inactive : SDPMediaDescription::RecvOnly);
    }
    else {
      localMedia->AddMediaFormats(formats, mediaType, rtpPayloadMap);
      localMedia->SetDirection(SDPMediaDescription::Inactive);
    }
#if PAUSE_WITH_EMPTY_ADDRESS
    if (m_holdToRemote >= eHoldOn) {
      PString addr = localMedia->GetTransportAddress();
      PString prot = addr.Left(addr.Find('$')+1);
      WORD port; { PIPSocket::Address dummy; localMedia->GetTransportAddress().GetIpAndPort(dummy, port); }
      OpalTransportAddress newAddr = prot + "0.0.0.0:" + PString(PString::Unsigned, port);
      localMedia->SetTransportAddress(newAddr);
      localMedia->SetDirection(SDPMediaDescription::Undefined);
    }
#endif
  }
  else {
    localMedia->AddMediaFormats(formats, mediaType, rtpPayloadMap);

#if OPAL_VIDEO
    if (rtpSessionId == OpalMediaFormat::DefaultVideoSessionID) {
      if (endpoint.GetManager().CanAutoStartTransmitVideo())
        localMedia->SetDirection(endpoint.GetManager().CanAutoStartReceiveVideo() ? SDPMediaDescription::SendRecv : SDPMediaDescription::SendOnly);
      else
        localMedia->SetDirection(endpoint.GetManager().CanAutoStartReceiveVideo() ? SDPMediaDescription::RecvOnly : SDPMediaDescription::Inactive);
    }
#endif
  }

  // Set format if we have an RTP payload type for RFC2833 and/or NSE
  // Must be after other codecs, as Mediatrix gateways barf if RFC2833 is first
  if (mediaType == OpalMediaType::Audio()) {
    SetNXEPayloadCode(localMedia, ntePayloadCode, rfc2833Handler,  OpalRFC2833, OpalDefaultNTEString, "NTE"); // RFC 2833
#if OPAL_T38_CAPABILITY
    SetNXEPayloadCode(localMedia, nsePayloadCode, ciscoNSEHandler, OpalCiscoNSE, OpalDefaultNSEString, "NSE"); // Cisco NSE
#endif
  }

  sdp.AddMediaDescription(localMedia);

  return true;
}


PBoolean SIPConnection::AnswerSDPMediaDescription(const SDPSessionDescription & sdpIn,
                                                                       unsigned rtpSessionId,
                                                        SDPSessionDescription & sdpOut)
{
  SDPMediaDescription * incomingMedia = sdpIn.GetMediaDescriptionByIndex(rtpSessionId);
  if (incomingMedia == NULL) {
    PTRACE(2, "SIP\tCould not find matching media type for session " << rtpSessionId);
    return PFalse;
  }

  OpalMediaType mediaType = incomingMedia->GetMediaType();
  OpalMediaTypeDefinition * defn = mediaType.GetDefinition();
  if (defn == NULL) {
    PTRACE(1, "SIP\tUnknown media type " << mediaType << " in session " << rtpSessionId);
    return false;
  }

  SDPMediaDescription * localMedia = NULL;

  OpalMediaFormatList sdpFormats = incomingMedia->GetMediaFormats(mediaType);
  sdpFormats.Remove(endpoint.GetManager().GetMediaFormatMask());
  if (sdpFormats.GetSize() == 0) {
    PTRACE(1, "SIP\tCould not find media formats in SDP media description for session " << rtpSessionId);
    // Send back a m= line with port value zero and the first entry of the offer payload types as per RFC3264
    if ((localMedia = defn->CreateSDPMediaDescription(OpalTransportAddress())) == NULL) {
      PTRACE(1, "SIP\tCould not create SDP media description for media type " << mediaType);
      return false;
    }
    if (!incomingMedia->GetSDPMediaFormats().IsEmpty())
      localMedia->AddSDPMediaFormat(new SDPMediaFormat(incomingMedia->GetSDPMediaFormats().front()));
    sdpOut.AddMediaDescription(localMedia);
    return PFalse;
  }
  
  // Create the list of Opal format names for the remote end.
  // We will answer with the media format that will be opened.
  // When sending an answer SDP, remove media formats that we do not support.
  remoteFormatList += sdpFormats;

  // find the payload type used for telephone-event, if present
  const SDPMediaFormatList & sdpMediaList = incomingMedia->GetSDPMediaFormats();
  PBoolean hasTelephoneEvent = PFalse;
  PBoolean hasNSE = PFalse;
  for (SDPMediaFormatList::const_iterator format = sdpMediaList.begin(); format != sdpMediaList.end(); ++format) {
    if (format->GetEncodingName() == "telephone-event") {
      rfc2833Handler->SetPayloadType(format->GetPayloadType());
      remoteFormatList += OpalRFC2833;
      hasTelephoneEvent = PTrue;
    }
    if (format->GetEncodingName() == "nse") {
      ciscoNSEHandler->SetPayloadType(format->GetPayloadType());
      remoteFormatList += OpalCiscoNSE;
      hasNSE = PTrue;
    }
  }

  OpalTransportAddress localAddress;
  OpalTransportAddress mediaAddress = incomingMedia->GetTransportAddress();
  bool remoteChanged = false;

  if (mediaAddress.IsEmpty()) {
    // Hold aka pause media, should only occur on a re-INVITE.
    RTP_UDP * rtpSession = dynamic_cast<RTP_UDP *>(GetSession(rtpSessionId));
    if (rtpSession == NULL)
      return false;
    localAddress = OpalTransportAddress(rtpSession->GetLocalAddress(), rtpSession->GetLocalDataPort());
  }
  else {
    PIPSocket::Address ip;
    WORD port = 0;
    if (!mediaAddress.GetIpAndPort(ip, port)) {
      PTRACE(1, "SIP\tCannot get remote ports for RTP session " << rtpSessionId);
      return PFalse;
    }

    // Create the RTPSession if required
    RTP_UDP * rtpSession = OnUseRTPSession(rtpSessionId, mediaType, mediaAddress, localAddress);
    if (rtpSession == NULL) {
      if (!ownerCall.IsMediaBypassPossible(*this, rtpSessionId)) {
        PTRACE(1, "SIP\tCannot create RTP session on non-bypassed connection");
        return false;
      }
    }
    else {
      // see if remote socket information has changed
      remoteChanged = (rtpSession->GetRemoteAddress() != ip) || (rtpSession->GetRemoteDataPort() != port);
      if (remoteChanged) {
        if (!rtpSession->SetRemoteSocketInfo(ip, port, PTrue)) {
          PTRACE(1, "SIP\tCannot set remote ports on RTP session");
          return false;
        }
        PTRACE(4, "SIP\tRemote changed IP address");
      }
    }
  }

  // construct a new media session list 
  if ((localMedia = defn->CreateSDPMediaDescription(localAddress)) == NULL) {
    PTRACE(1, "SIP\tCould not create SDP media description for media type " << mediaType);
    return false;
  }

  // create map for RTP payloads
  incomingMedia->CreateRTPMap(rtpSessionId, rtpPayloadMap);

  SDPMediaDescription::Direction otherSidesDir = sdpIn.GetDirection(rtpSessionId);
#if OPAL_VIDEO
  if (rtpSessionId == OpalMediaFormat::DefaultVideoSessionID && GetPhase() < EstablishedPhase) {
    // If processing initial INVITE and video, obey the auto-start flags
    if (!endpoint.GetManager().CanAutoStartTransmitVideo())
      otherSidesDir = (otherSidesDir&SDPMediaDescription::SendOnly) != 0 ? SDPMediaDescription::SendOnly : SDPMediaDescription::Inactive;
    if (!endpoint.GetManager().CanAutoStartReceiveVideo())
      otherSidesDir = (otherSidesDir&SDPMediaDescription::RecvOnly) != 0 ? SDPMediaDescription::RecvOnly : SDPMediaDescription::Inactive;
  }
#endif

  SDPMediaDescription::Direction newDirection = SDPMediaDescription::Inactive;

  // Check if we had a stream and the remote has either changed the codec or
  // changed the direction of the stream
  OpalMediaStreamPtr sendStream = GetMediaStream(rtpSessionId, false);
  if (sendStream != NULL && sendStream->IsOpen()) {
    if (!remoteChanged && sdpFormats.HasFormat(sendStream->GetMediaFormat())) {
      bool paused = (otherSidesDir&SDPMediaDescription::RecvOnly) == 0;
      sendStream->SetPaused(paused);
      if (!paused)
        newDirection = SDPMediaDescription::SendOnly;
    }
    else {
      sendStream->GetPatch()->GetSource().Close();
      sendStream.SetNULL();
    }
  }

  OpalMediaStreamPtr recvStream = GetMediaStream(rtpSessionId, true);
  if (recvStream != NULL && recvStream->IsOpen()) {
    if (!remoteChanged && sdpFormats.HasFormat(recvStream->GetMediaFormat())) {
      bool paused = (otherSidesDir&SDPMediaDescription::SendOnly) == 0;
      recvStream->SetPaused(paused);
      if (!paused)
        newDirection = newDirection == SDPMediaDescription::SendOnly ? SDPMediaDescription::SendRecv : SDPMediaDescription::RecvOnly;
    }
    else {
      recvStream->Close();
      recvStream.SetNULL();
    }
  }

  // After (possibly) closing streams, we now open them again if necessary
  // OpenSourceMediaStreams will just return true if they are already open
  if ((otherSidesDir&SDPMediaDescription::SendOnly) != 0 &&
      recvStream == NULL &&
      ownerCall.OpenSourceMediaStreams(*this, mediaType, rtpSessionId) &&
      (recvStream = GetMediaStream(rtpSessionId, true)) != NULL)
    newDirection = SDPMediaDescription::RecvOnly;

  PSafePtr<OpalConnection> otherParty = GetCall().GetOtherPartyConnection(*this);
  if ((otherSidesDir&SDPMediaDescription::RecvOnly) != 0 &&
       otherParty != NULL &&
       sendStream == NULL &&
       ownerCall.OpenSourceMediaStreams(*otherParty, mediaType, rtpSessionId) &&
       (sendStream = GetMediaStream(rtpSessionId, false)) != NULL)
    newDirection = newDirection == SDPMediaDescription::RecvOnly ? SDPMediaDescription::SendRecv : SDPMediaDescription::SendOnly;

  // Now we build the reply, setting "direction" as appropriate for what we opened.
  localMedia->SetDirection(newDirection);
  if (sendStream != NULL)
    localMedia->AddMediaFormat(sendStream->GetMediaFormat(), rtpPayloadMap);
  else if (recvStream != NULL)
    localMedia->AddMediaFormat(recvStream->GetMediaFormat(), rtpPayloadMap);
  else {
    // Add all possible formats
    bool empty = true;
    OpalMediaFormatList localFormatList = GetLocalMediaFormats();
    for (OpalMediaFormatList::iterator remoteFormat = remoteFormatList.begin(); remoteFormat != remoteFormatList.end(); ++remoteFormat) {
      for (OpalMediaFormatList::iterator localFormat = localFormatList.begin(); localFormat != localFormatList.end(); ++localFormat) {
        OpalMediaFormat intermediateFormat;
        if (remoteFormat->GetDefaultSessionID() == rtpSessionId &&
            OpalTranscoder::FindIntermediateFormat(*localFormat, *remoteFormat, intermediateFormat)) {
          localMedia->AddMediaFormat(*remoteFormat, rtpPayloadMap);
          empty = false;
          break;
        }
      }
    }

    // RFC3264 says we MUST have an entry, but it should have port zero
    if (empty) {
      localMedia->AddMediaFormat(sdpFormats.front(), rtpPayloadMap);
      localMedia->SetTransportAddress(OpalTransportAddress());
    }
    else {
      // We can do the media type but choose not to at this time
      localMedia->SetDirection(SDPMediaDescription::Inactive);
    }
  }

  // Add in the RFC2833 handler, if used
  if (hasTelephoneEvent)
    localMedia->AddSDPMediaFormat(new SDPMediaFormat(OpalRFC2833, rfc2833Handler->GetPayloadType(), OpalDefaultNTEString));

#if OPAL_T38_CAPABILITY
  if (hasNSE)
    localMedia->AddSDPMediaFormat(new SDPMediaFormat(OpalCiscoNSE, ciscoNSEHandler->GetPayloadType(), OpalDefaultNSEString));
#endif

  sdpOut.AddMediaDescription(localMedia);

  return true;
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


OpalMediaStreamPtr SIPConnection::OpenMediaStream(const OpalMediaFormat & mediaFormat, unsigned sessionID, bool isSource)
{
  // Disable send of re-INVITE if the ancestor OpenMediaStream has to close
  // the media stream, so don't get two re-INVITEs in quick succession.
  bool oldReINVITE = needReINVITE;
  needReINVITE = false;

  OpalMediaStreamPtr newStream = OpalRTPConnection::OpenMediaStream(mediaFormat, sessionID, isSource);
  if (newStream == NULL) {
    needReINVITE = oldReINVITE;
    return newStream;
  }

  // Make sure stream is symmetrical, if codec changed, close and re-open it
  OpalMediaStreamPtr otherStream = GetMediaStream(sessionID, !isSource);
  if (otherStream != NULL && otherStream->IsOpen() && otherStream->GetMediaFormat() != mediaFormat) {
    if (isSource) {
      OpalMediaPatch * patch = otherStream->GetPatch();
      if (patch != NULL)
        patch->GetSource().Close();
      GetCall().OpenSourceMediaStreams(*GetCall().GetOtherPartyConnection(*this),
                                       mediaFormat.GetMediaType(), sessionID, mediaFormat);
    }
    else {
      otherStream->Close();
      GetCall().OpenSourceMediaStreams(*this, mediaFormat.GetMediaType(), sessionID, mediaFormat);
    }
  }

  needReINVITE = oldReINVITE;

  if (GetPhase() == EstablishedPhase && needReINVITE) {
    PTRACE(3, "SIP\tStarting re-INVITE to open channel.");
    SIPTransaction * invite = new SIPInvite(*this, *transport, m_rtpSessions);
    invite->Start();
  }

  return newStream;
}


bool SIPConnection::CloseMediaStream(OpalMediaStream & stream)
{
  bool ok = OpalConnection::CloseMediaStream(stream);

  if (GetPhase() == EstablishedPhase && needReINVITE) {
    PTRACE(3, "SIP\tStarting re-INVITE to close channel.");
    SIPTransaction * invite = new SIPInvite(*this, *transport, m_rtpSessions);
    invite->Start();
  }

  return ok;
}


PBoolean SIPConnection::WriteINVITE(OpalTransport & transport, void * param)
{
  SIPConnection & connection = *(SIPConnection *)param;

  connection.AdjustOutgoingINVITE();
  connection.needReINVITE = false;
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
  PTRACE(3, "SIP\tSetUpConnection: " << m_requestURI);

  SetPhase(SetUpPhase);

  ApplyStringOptions();

  SIPURL transportAddress;

  if (!routeSet.IsEmpty()) 
    transportAddress = routeSet.front();
  else {
    transportAddress = m_requestURI;
    transportAddress.AdjustToDNS(); // Do a DNS SRV lookup
    PTRACE(4, "SIP\tConnecting to " << m_requestURI << " via " << transportAddress);
  }

  originating = PTrue;

  delete transport;
  transport = endpoint.CreateTransport(transportAddress.GetHostAddress());
  if (transport == NULL) {
    Release(EndedByUnreachable);
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


PString SIPConnection::GetDestinationAddress()
{
  return calledDestinationURL;
}


bool SIPConnection::HoldConnection()
{
  if (transport == NULL)
    return false;

  if (m_holdToRemote != eHoldOff)
    return true;

  PTRACE(3, "SIP\tStarting re-INVITE to put connection on hold");

  m_holdToRemote = eHoldInProgress;

  SIPTransaction * invite = new SIPInvite(*this, *transport, m_rtpSessions);
  if (invite->Start())
    return true;

  m_holdToRemote = eHoldOff;
  return false;
}


bool SIPConnection::RetrieveConnection()
{
  if (transport == NULL)
    return false;

  switch (m_holdToRemote) {
    case eHoldOff :
      return true;

    case eHoldOn :
      break;

    default :
      return false;
  }

  m_holdToRemote = eRetrieveInProgress;

  PTRACE(3, "SIP\tStarting re-INVITE to retrieve connection from hold");

  SIPTransaction * invite = new SIPInvite(*this, *transport, m_rtpSessions);
  if (invite->Start())
    return true;

  m_holdToRemote = eHoldOn;
  return false;
}


PBoolean SIPConnection::IsConnectionOnHold()
{
  return m_holdToRemote != eHoldOff;
}


void SIPConnection::AdjustOutgoingINVITE()
{
  SIPURL myAddress = endpoint.GetRegisteredPartyName(m_requestURI);

  PString transport = m_requestURI.GetParamVars()("transport");
  if (!transport.IsEmpty())
    myAddress.SetParamVar("transport", transport);

  // allow callers to override the From field
  if (stringOptions != NULL) {
    // only allow override of calling party number if the local party
    // name hasn't been first specified by a register handler. i.e a
    // register handler's target number is always used
    PString number((*stringOptions)("Calling-Party-Number"));
    if (!number.IsEmpty() && myAddress.GetUserName() == endpoint.GetDefaultLocalPartyName())
      myAddress.SetUserName(number);

    PString name((*stringOptions)("Calling-Party-Name"));
    if (!name.IsEmpty())
      myAddress.SetDisplayName(name);
  }

  m_dialogFrom = myAddress.AsQuotedString() + ";tag=" + OpalGloballyUniqueID().AsString();
}


PString SIPConnection::GetRemotePartyURL() const
{
  SIPURL url = GetRemotePartyAddress();
  url.AdjustForRequestURI();
  return url.AsString();
}


void SIPConnection::OnTransactionFailed(SIPTransaction & transaction)
{
  switch (transaction.GetMethod()) {
    case SIP_PDU::Method_INVITE :
      break;

    case SIP_PDU::Method_REFER :
      referTransaction.SetNULL();
      // Do next case

    default :
      return;
  }

  // If we are releasing then I can safely ignore failed
  // transactions - otherwise I'll deadlock.
  if (GetPhase() >= ReleasingPhase)
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
  SIP_PDU::Methods method = pdu.GetMethod();

  if (method == SIP_PDU::NumMethods) {
    // Handle a response to one of our transactions
    PSafePtr<SIPTransaction> transaction = endpoint.GetTransaction(pdu.GetTransactionID(), PSafeReference);
    if (transaction != NULL)
      transaction->OnReceivedResponse(pdu);
    else {
      PTRACE(3, "SIP\tCannot find transaction for response");
    }
    return;
  }

  PSafeLockReadWrite lock(*this);
  if (!lock.IsLocked())
    return;

  // Prevent retries from getting through to processing
  unsigned sequenceNumber = pdu.GetMIME().GetCSeqIndex();
  if (m_lastRxCSeq.find(method) != m_lastRxCSeq.end() && sequenceNumber <= m_lastRxCSeq[method]) {
    PTRACE(3, "SIP\tIgnoring duplicate PDU " << pdu);
    return;
  }
  m_lastRxCSeq[method] = sequenceNumber;

  PTRACE(4, "SIP\tHandling PDU " << pdu);

  switch (method) {
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
    default :
      // Shouldn't have got this!
      PTRACE(2, "SIP\tUnhandled PDU " << pdu);
      break;
  }
}


void SIPConnection::OnReceivedResponseToINVITE(SIPTransaction & transaction, SIP_PDU & response)
{
  unsigned statusClass = response.GetStatusCode()/100;
  if (statusClass > 2)
    return;

  PSafeLockReadWrite lock(*this);
  if (!lock.IsLocked())
    return;

  // See if this is an initial INVITE or a re-INVITE
  bool reInvite = true;
  for (PSafePtr<SIPTransaction> invitation(forkedInvitations, PSafeReference); invitation != NULL; ++invitation) {
    if (invitation == &transaction) {
      reInvite = false;
      break;
    }
  }

  // If we are in a dialog, then m_requestURI needs to be set
  // to the contact field in the 2xx/1xx response for a target refresh 
  // request
  PString contact = response.GetMIME().GetContact();
  if (statusClass == 2 && !contact.IsEmpty()) {
    m_requestURI = contact;
    m_requestURI.AdjustForRequestURI();
    PTRACE(4, "SIP\tSet Request URI to " << m_requestURI);
  }

  if (reInvite) {
    (((SIPInvite &)transaction).GetSessionManager()).SetCleanup(false);
    return;
  }

  if (statusClass == 2) {
    // Have a final response to the INVITE, so cancel all the other invitations sent.
    for (PSafePtr<SIPTransaction> invitation(forkedInvitations, PSafeReference); invitation != NULL; ++invitation) {
      if (invitation != &transaction)
        invitation->Cancel();
    }

    // And end connect mode on the transport
    transport->SetInterface(transaction.GetInterface());
  }

  // get the route set from the Record-Route response field (in reverse order)
  // according to 12.1.2
  // requests in a dialog do not modify the initial route set fo according 
  // to 12.2
  routeSet.RemoveAll();
  PStringList recordRoute = response.GetMIME().GetRecordRoute();
  for (PStringList::iterator route = recordRoute.rbegin(); route != recordRoute.rend(); --route)
    routeSet.AppendString(*route);

  // Save the sessions etc we are actually using of all the forked INVITES sent
  if (response.HasSDP())
    m_rtpSessions.CopyToMaster(((SIPInvite &)transaction).GetSessionManager());

  m_dialogFrom = transaction.GetMIME().GetFrom();
  UpdateRemoteAddresses(response.GetMIME().GetTo());

  response.GetMIME().GetProductInfo(remoteProductInfo);
}


void SIPConnection::OnReceivedResponse(SIPTransaction & transaction, SIP_PDU & response)
{
  PSafeLockReadWrite lock(*this);
  if (!lock.IsLocked())
    return;

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

    case 2 : // Successful response - there really is only 200 OK
      OnReceivedOK(transaction, response);
      return;

    case 3 : // Redirection response
      return;
  }

  // We don't always release the connection, eg not till all forked invites have completed
  // This INVITE is from a different "dialog", any errors do not cause a release
  if (transaction.GetMethod() != SIP_PDU::Method_INVITE)
    return;

  // If we are doing a local hold, and it failed, we do not release the connection
  switch (m_holdToRemote) {
    case eHoldInProgress :
      m_holdToRemote = eHoldOff;  // Did not go into hold
      OnHold(false, false);   // Signal the manager that there is no more hold
      return;

    case eRetrieveInProgress :
      m_holdToRemote = eHoldOn;  // Did not go out of hold
      OnHold(false, true);   // Signal the manager that hold is still active
      return;

    default :
      break;
  }

  if (GetPhase() < ConnectedPhase) {
    // Final check to see if we have forked INVITEs still running, don't
    // release connection until all of them have failed.
    for (PSafePtr<SIPTransaction> invitation(forkedInvitations, PSafeReference); invitation != NULL; ++invitation) {
      if (invitation->IsInProgress())
    return;
    }
  }

  // All other responses are errors, see if they should cause a Release()
  for (PINDEX i = 0; i < PARRAYSIZE(SIPCodeToReason); i++) {
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
  PString requestFromTag = requestMIME.GetFieldParameter("From", "tag");
  PString requestToTag   = requestMIME.GetFieldParameter("To",   "tag");

  if (IsOriginating()) {
    // Check for in the same dialog
    if (SIPMIMEInfo::ExtractFieldParameter(m_dialogTo,   "tag") != requestFromTag ||
        SIPMIMEInfo::ExtractFieldParameter(m_dialogFrom, "tag") != requestToTag) {
      PTRACE(2, "SIP\tIgnoring INVITE from " << request.GetURI() << " when originated call.");
      SIP_PDU response(request, SIP_PDU::Failure_LoopDetected);
      SendPDU(response, request.GetViaAddress(endpoint));
      return;
    }

    // We originated the call, so any INVITE must be a re-INVITE, 
    isReinvite = PTrue;
  }
  else {
    if (originalInvite == NULL) {
      isReinvite = PFalse; // First time incoming call
      PTRACE(4, "SIP\tInitial INVITE from " << request.GetURI());
    }
    else {
      // Have received multiple INVITEs, three possibilities
      const SIPMIMEInfo & originalMIME = originalInvite->GetMIME();

      // #1 - Same sequence number means it is a retransmission
      if (originalMIME.GetCSeq() == requestMIME.GetCSeq()) {
        PTimeInterval timeSinceInvite = PTime() - originalInviteTime;
        PTRACE(3, "SIP\tIgnoring duplicate INVITE from " << request.GetURI() << " after " << timeSinceInvite);
        return;
      }

      // #2 - Different "dialog" determined by the tags in the to and from fields indicate forking
      if (requestMIME.GetFieldParameter("From", "tag") != requestFromTag ||
          requestMIME.GetFieldParameter("To",   "tag") != requestToTag) {
        PTRACE(3, "SIP\tIgnoring forked INVITE from " << request.GetURI());
        SIP_PDU response(request, SIP_PDU::Failure_LoopDetected);
        response.GetMIME().SetProductInfo(endpoint.GetUserAgent(), GetProductInfo());
        SendPDU(response, request.GetViaAddress(endpoint));
        return;
      }

      // #3 - A new INVITE in the same "dialog" but different cseq must be a re-INVITE
      isReinvite = PTrue;
    }
  }

  // originalInvite should contain the first received INVITE for
  // this connection
  delete originalInvite;
  originalInvite     = new SIP_PDU(request);
  originalInviteTime = PTime();

  SIPMIMEInfo & mime = originalInvite->GetMIME();

  // update the target address
  PString contact = mime.GetContact();
  if (!contact.IsEmpty()) {
    m_requestURI = contact;
    m_requestURI.AdjustForRequestURI();
    PTRACE(4, "SIP\tSet Request URI to " << m_requestURI);
  }

  // We received a Re-INVITE for a current connection
  if (isReinvite) { 
    OnReceivedReINVITE(request);
    return;
  }

  // Fill in all the various connection info, not our to/from is their from/to
  UpdateRemoteAddresses(mime.GetFrom());
  mime.GetProductInfo(remoteProductInfo);
  m_dialogFrom = mime.GetTo() + ";tag=" + OpalGloballyUniqueID().AsString(); // put a real random 
  mime.SetTo(m_dialogFrom);

  // update the route set and the target address according to 12.1.1
  // requests in a dialog do not modify the route set according to 12.2
  routeSet = mime.GetRecordRoute();

  // get the called destination
  calledDestinationName   = originalInvite->GetURI().GetDisplayName(PFalse);   
  calledDestinationNumber = originalInvite->GetURI().GetUserName();
  calledDestinationURL    = originalInvite->GetURI().AsString();

  // get the address that remote end *thinks* it is using from the Contact field
  PIPSocket::Address sigAddr;
  PIPSocket::GetHostAddress(m_requestURI.GetHostName(), sigAddr);  

  // get the local and peer transport addresses
  PIPSocket::Address peerAddr, localAddr;
  transport->GetRemoteAddress().GetIpAddress(peerAddr);
  transport->GetLocalAddress().GetIpAddress(localAddr);

  // allow the application to determine if RTP NAT is enabled or not
  remoteIsNAT = IsRTPNATEnabled(localAddr, peerAddr, sigAddr, PTrue);

  releaseMethod = ReleaseWithResponse;

  PTRACE(3, "SIP\tOnIncomingConnection succeeded for INVITE from " << request.GetURI() << " for " << *this);
  SetPhase(SetUpPhase);

  // Replaces header has already been validated in SIPEndPoint::OnReceivedINVITE
  PString replaces = mime("Replaces");
  PSafePtr<SIPConnection> replacedConnection = endpoint.GetSIPConnectionWithLock(replaces.Left(replaces.Find(';')).Trim(), PSafeReadOnly);
  if (replacedConnection != NULL) {
    PTRACE(3, "SIP\tConnection " << *replacedConnection << " replaced by " << *this);

    replacedConnection->Release(OpalConnection::EndedByCallForwarded);
    replacedConnection->CloseMediaStreams();

    if (!OnOpenIncomingMediaChannels()) {
      PTRACE(1, "SIP\tOnOpenIncomingMediaChannels failed for INVITE from " << request.GetURI() << " for " << *this);
      Release();
    }

    SetConnected();
    return;
  }

  // indicate the other is to start ringing (but look out for clear calls)
  if (!OnIncomingConnection(0, NULL)) {
    PTRACE(1, "SIP\tOnIncomingConnection failed for INVITE from " << request.GetURI() << " for " << *this);
    Release();
    return;
  }

  if (!OnOpenIncomingMediaChannels()) {
    PTRACE(1, "SIP\tOnOpenIncomingMediaChannels failed for INVITE from " << request.GetURI() << " for " << *this);
    Release();
    return;
  }

  if (!ownerCall.OnSetUp(*this)) {
    PTRACE(1, "SIP\tOnSetUp failed for INVITE from " << request.GetURI() << " for " << *this);
    Release();
    return;
  }

  AnsweringCall(OnAnswerCall(remotePartyAddress));
}


void SIPConnection::OnReceivedReINVITE(SIP_PDU & request)
{
  if (GetPhase() != EstablishedPhase) {
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
    if (sdpIn.IsHold()) {
      PTRACE(3, "SIP\tRemote hold detected");
      m_holdFromRemote = true;
      OnHold(true, true);
    }
    else {
      // If we receive a consecutive reinvite without the SendOnly
      // parameter, then we are not on hold anymore
      if (m_holdFromRemote) {
        PTRACE(3, "SIP\tRemote retrieve from hold detected");
        m_holdFromRemote = false;
        OnHold(true, false);
      }
    }
  }
  else {
    if (m_holdFromRemote) {
      PTRACE(3, "SIP\tRemote retrieve from hold without SDP detected");
      m_holdFromRemote = false;
      OnHold(true, false);
    }
  }
  
  // send the 200 OK response
  if (OnSendSDP(true, m_rtpSessions, sdpOut))
    SendInviteOK(sdpOut);
  else
    SendInviteResponse(SIP_PDU::Failure_NotAcceptableHere);
}


PBoolean SIPConnection::OnOpenIncomingMediaChannels()
{
  ApplyStringOptions();

  // in some circumstances, the peer OpalConnection needs to see the newly arrived media formats
  // before it knows what what formats can support. 
  if (originalInvite != NULL && originalInvite->HasSDP()) {

    OpalMediaFormatList previewFormats;

    SDPSessionDescription & sdp = originalInvite->GetSDP();
    static struct PreviewType {
      const char * mediaType;
      unsigned sessionID;
    } previewTypes[] = 
    { 
      { "audio", OpalMediaFormat::DefaultAudioSessionID },
#if OPAL_VIDEO
      { "video", OpalMediaFormat::DefaultVideoSessionID },
#endif
#if OPAL_T38_CAPABILITY
      { "image", OpalMediaFormat::DefaultDataSessionID },
#endif
    };
    PINDEX i;
    for (i = 0; i < (PINDEX) (sizeof(previewTypes)/sizeof(previewTypes[0])); ++i) {
      SDPMediaDescription * mediaDescription = sdp.GetMediaDescriptionByType(previewTypes[i].mediaType);
      if (mediaDescription != NULL) {
        previewFormats += mediaDescription->GetMediaFormats(previewTypes[i].mediaType);
        OpalTransportAddress mediaAddress = mediaDescription->GetTransportAddress();
        mediaTransportAddresses.SetAt(previewTypes[i].sessionID, new OpalTransportAddress(mediaAddress));
      }
    }

    if (previewFormats.GetSize() != 0) 
      ownerCall.GetOtherPartyConnection(*this)->PreviewPeerMediaFormats(previewFormats);
  }

  return PTrue;
}


void SIPConnection::AnsweringCall(AnswerCallResponse response)
{
  switch (GetPhase()) {
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
  PString origFromTag = originalInvite->GetMIME().GetFieldParameter("From", "tag");
  PString origToTag   = originalInvite->GetMIME().GetFieldParameter("To",   "tag");
  PString fromTag     = response.GetMIME().GetFieldParameter("From", "tag");
  PString toTag       = response.GetMIME().GetFieldParameter("To",   "tag");
  if (fromTag != origFromTag || (!toTag.IsEmpty() && (toTag != origToTag))) {
    PTRACE(3, "SIP\tACK received for forked INVITE from " << response.GetURI());
    return;
  }

  PTRACE(3, "SIP\tACK received: " << GetPhase());

  ackReceived = true;
  ackTimer.Stop(false); // Asynchornous stop to avoid deadlock
  ackRetry.Stop(false);
  
  OnReceivedSDP(response);

  // If we receive an ACK in established phase, it is a re-INVITE
  if (GetPhase() == EstablishedPhase) {
    StartMediaStreams();
    return;
  }
  
  if (GetPhase() != ConnectedPhase)  
    return;
  
  SetPhase(EstablishedPhase);
  OnEstablished();
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
    referTransaction.SetNULL();

    // Release the connection
    if (GetPhase() < ReleasingPhase) {
      releaseMethod = ReleaseWithBYE;
      Release(OpalConnection::EndedByCallForwarded);
    }
  }

  // The REFER is not over yet, ignore the state of the REFER for now
}


void SIPConnection::OnReceivedREFER(SIP_PDU & request)
{
  PString referTo = request.GetMIME().GetReferTo();
  if (referTo.IsEmpty()) {
    SIP_PDU response(request, SIP_PDU::Failure_BadRequest, NULL, "Missing refer-to header");
    SendPDU(response, request.GetViaAddress(endpoint));
    return;
  }    

  SIPURL to = referTo;
  PString replaces = PURL::UntranslateString(to.GetQueryVars()("Replaces"), PURL::QueryTranslation);
  to.SetQuery(PString::Empty());

  if (!endpoint.SetupTransfer(GetToken(), replaces, to.AsString(), NULL)) {
    SIP_PDU response(request, SIP_PDU::Failure_TransactionDoesNotExist);
    SendPDU(response, request.GetViaAddress(endpoint));
    return;
  }    

  SIP_PDU response(request, SIP_PDU::Successful_Accepted);

  if (request.GetMIME()("Refer-Sub") *= "false") {
    // Use RFC4488 to indicate we are NOT doing NOTIFYs
    response.GetMIME().SetAt("Refer-Sub", "false");
  }
  else {
    // Send a NOTIFY for done, even though strictly speaking we aren't yet
    SIPReferNotify * notify = new SIPReferNotify(*this, *transport, SIP_PDU::Successful_Accepted);
    notify->Start();
  }

  SendPDU(response, request.GetViaAddress(endpoint));
}


void SIPConnection::OnReceivedBYE(SIP_PDU & request)
{
  PTRACE(3, "SIP\tBYE received for call " << request.GetMIME().GetCallID());
  SIP_PDU response(request, SIP_PDU::Successful_OK);
  SendPDU(response, request.GetViaAddress(endpoint));
  
  if (GetPhase() >= ReleasingPhase) {
    PTRACE(2, "SIP\tAlready released " << *this);
    return;
  }
  releaseMethod = ReleaseWithNothing;
  
  UpdateRemoteAddresses(request.GetMIME().GetFrom());
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

  if (GetPhase() < AlertingPhase) {
    SetPhase(AlertingPhase);
    OnAlerting();
  }
}


void SIPConnection::OnReceivedSessionProgress(SIP_PDU & response)
{
  PTRACE(3, "SIP\tReceived Session Progress response");

  OnReceivedSDP(response);

  if (GetPhase() < AlertingPhase) {
    SetPhase(AlertingPhase);
    OnAlerting();
  }

  PTRACE(4, "SIP\tStarting receive media to annunciate remote progress tones");
  StartMediaStreams();
}


void SIPConnection::OnReceivedRedirection(SIP_PDU & response)
{
  m_requestURI = response.GetMIME().GetContact();
  UpdateRemoteAddresses(m_requestURI.AsQuotedString());
  endpoint.ForwardConnection (*this, remotePartyAddress);
}


PBoolean SIPConnection::OnReceivedAuthenticationRequired(SIPTransaction & transaction, SIP_PDU & response)
{
  PBoolean isProxy = response.GetStatusCode() == SIP_PDU::Failure_ProxyAuthenticationRequired;

#if PTRACING
  const char * proxyTrace = isProxy ? "Proxy " : "";
#endif
  
  if (transaction.GetMethod() != SIP_PDU::Method_INVITE) {
    PTRACE(1, "SIP\tCannot do " << proxyTrace << "Authentication Required for non INVITE");
    return PFalse;
  }

  PTRACE(3, "SIP\tReceived " << proxyTrace << "Authentication Required response");

  // determine the authentication type
  PString errorMsg;
  SIPAuthentication * newAuth = SIPAuthentication::ParseAuthenticationRequired(isProxy, 
                                                                               response.GetMIME()(isProxy ? "Proxy-Authenticate" : "WWW-Authenticate"),
                                                                               errorMsg);
  if (newAuth == NULL) {
    PTRACE(1, "SIP\t" << errorMsg);
    return false;
  }

  SIPURL proxy = endpoint.GetProxy();

   // Try to find authentication parameters for the given realm,
  // if not, use the proxy authentication parameters (if any)
  PString realm, user, password;
  if (endpoint.GetAuthentication(newAuth->GetAuthRealm(), realm, user, password)) {
    PTRACE (3, "SIP\tFound auth info for realm " << newAuth->GetAuthRealm());
    newAuth->SetUsername(user);
    newAuth->SetPassword(password);
  }
  else if (!proxy.IsEmpty()) {
    PTRACE (3, "SIP\tNo auth info for realm " << newAuth->GetAuthRealm() << ", using proxy auth");
    newAuth->SetUsername(proxy.GetUserName());
    newAuth->SetPassword(proxy.GetPassword());
  } 
  else {
    PTRACE (3, "SIP\tNo auth info for realm " << newAuth->GetAuthRealm());
    delete newAuth;
    return PFalse;
  }

  // Restart the transaction with new authentication info
  delete authentication;
  authentication = newAuth;

  needReINVITE = false; // Is not actually a re-INVITE though it looks a little bit like one.
  transport->SetInterface(transaction.GetInterface());
  OpalRTPSessionManager & origRtpSessions = ((SIPInvite &)transaction).GetSessionManager();
  (((SIPInvite &)transaction).GetSessionManager()).SetCleanup(false);
  SIPTransaction * invite = new SIPInvite(*this, *transport, origRtpSessions);

  // Section 8.1.3.5 of RFC3261 tells that the authenticated
  // request SHOULD have the same value of the Call-ID, To and From.
  // For Asterisk this is not merely SHOULD, but SHALL ....
  invite->GetMIME().SetFrom(transaction.GetMIME().GetFrom());

  if (!invite->Start()) {
    PTRACE(2, "SIP\tCould not restart INVITE for " << proxyTrace << "Authentication Required");
    return PFalse;
  }

  forkedInvitations.Append(invite);
  return PTrue;
}


void SIPConnection::OnReceivedOK(SIPTransaction & transaction, SIP_PDU & response)
{
  PTRACE(3, "SIP\tHandling " << response.GetStatusCode() << " response for " << transaction.GetMethod());

  switch (transaction.GetMethod()) {
    case SIP_PDU::Method_INVITE :
      break;

    case SIP_PDU::Method_REFER :
      if (response.GetMIME()("Refer-Sub") == "false")
        referTransaction.SetNULL(); // Used RFC4488 to indicate we are NOT doing NOTIFYs
      // Do next case

    default :
      return;
  }

  PTRACE(3, "SIP\tReceived INVITE OK response");
  releaseMethod = ReleaseWithBYE;
  sessionTimer = 10000;

  OnReceivedSDP(response);

  switch (m_holdToRemote) {
    case eHoldInProgress :
      m_holdToRemote = eHoldOn;
      OnHold(false, true);   // Signal the manager that they are on hold
      break;

    case eRetrieveInProgress :
      m_holdToRemote = eHoldOff;
      OnHold(false, false);   // Signal the manager that there is no more hold
      break;

    default :
      break;
  }

  OnConnectedInternal();
}


void SIPConnection::OnReceivedSDP(SIP_PDU & pdu)
{
  if (!pdu.HasSDP())
    return;

  needReINVITE = false;

  SDPSessionDescription & sdp = pdu.GetSDP();

  bool ok = false;
  for (PINDEX i = 0; i < sdp.GetMediaDescriptions().GetSize(); ++i) 
    ok |= OnReceivedSDPMediaDescription(sdp, i+1);

  remoteFormatList += OpalRFC2833;

  needReINVITE = true;

  if (GetPhase() == EstablishedPhase) // re-INVITE
    StartMediaStreams();
  else if (!ok)
    Release(EndedByCapabilityExchange);
}


bool SIPConnection::OnReceivedSDPMediaDescription(SDPSessionDescription & sdp, unsigned rtpSessionId)
{
  RTP_UDP *rtpSession = NULL;
  SDPMediaDescription * mediaDescription = sdp.GetMediaDescriptionByIndex(rtpSessionId);
  PAssert(mediaDescription != NULL, "media description list changed");

  OpalMediaType mediaType = mediaDescription->GetMediaType();
  
  if (mediaDescription->GetPort() == 0) {
    PTRACE(2, "SIP\tDisabled/missing SDP media description for " << mediaType);

    OpalMediaStreamPtr stream = GetMediaStream(rtpSessionId, false);
    if (stream != NULL)
      stream->Close();

    stream = GetMediaStream(rtpSessionId, true);
    if (stream != NULL)
      stream->Close();

    return false;
  }

  // see if the remote supports this media
  OpalMediaFormatList mediaFormatList = mediaDescription->GetMediaFormats(mediaType);
  if (mediaFormatList.GetSize() == 0) {
    PTRACE(1, "SIP\tCould not find media formats in SDP media description for session " << rtpSessionId);
    return false;
  }

  // When receiving an answer SDP, keep the remote SDP media formats order
  // but remove the media formats we do not support.
  remoteFormatList += mediaFormatList;
  remoteFormatList.Remove(endpoint.GetManager().GetMediaFormatMask());

  // create map for RTP payloads
  mediaDescription->CreateRTPMap(rtpSessionId, rtpPayloadMap);

  // create the RTPSession
  OpalTransportAddress localAddress;
  OpalTransportAddress address = mediaDescription->GetTransportAddress();
  if (!address.IsEmpty()) {
    rtpSession = OnUseRTPSession(rtpSessionId, mediaType, address, localAddress);
    if (rtpSession == NULL && !ownerCall.IsMediaBypassPossible(*this, rtpSessionId))
      return false;

    // set the remote address 
    PIPSocket::Address ip;
    WORD port = 0;
    if (!address.GetIpAndPort(ip, port) || port == 0 || (rtpSession && !rtpSession->SetRemoteSocketInfo(ip, port, true))) {
      PTRACE(1, "SIP\tCannot set remote ports on RTP session");
      return false;
    }
  }

  SDPMediaDescription::Direction otherSidesDir = sdp.GetDirection(rtpSessionId);

  // Check if we had a stream and the remote has either changed the codec or
  // changed the direction of the stream
  OpalMediaStreamPtr sendStream = GetMediaStream(rtpSessionId, false);
  if (sendStream != NULL && sendStream->IsOpen()) {
    if (mediaFormatList.HasFormat(sendStream->GetMediaFormat()))
      sendStream->SetPaused((otherSidesDir&SDPMediaDescription::RecvOnly) == 0);
    else {
      sendStream->GetPatch()->GetSource().Close(); // Was removed from list so close channel
      sendStream.SetNULL();
    }
  }

  OpalMediaStreamPtr recvStream = GetMediaStream(rtpSessionId, true);
  if (recvStream != NULL && recvStream->IsOpen()) {
    if (mediaFormatList.HasFormat(recvStream->GetMediaFormat()))
      recvStream->SetPaused((otherSidesDir&SDPMediaDescription::SendOnly) == 0);
    else {
      recvStream->Close(); // Was removed from list so close channel
      recvStream.SetNULL();
    }
  }

  // Then open the streams if the direction allows
  if ((otherSidesDir&SDPMediaDescription::SendOnly) != 0)
    ownerCall.OpenSourceMediaStreams(*this, mediaType, rtpSessionId);

  PSafePtr<OpalConnection> otherParty = GetCall().GetOtherPartyConnection(*this);
  if ((otherSidesDir&SDPMediaDescription::RecvOnly) != 0 && otherParty != NULL)
    ownerCall.OpenSourceMediaStreams(*otherParty, mediaType, rtpSessionId);

  return true;
}


void SIPConnection::OnCreatingINVITE(SIP_PDU & request)
{
  PTRACE(3, "SIP\tCreating INVITE request");

  if (stringOptions != NULL) {
    PString replaces((*stringOptions)("Replaces"));
    if (!replaces.IsEmpty()) {
      SIPMIMEInfo & mime = request.GetMIME();
      mime.SetAt("Require", "replaces");
      mime.SetAt("Replaces", replaces);
    }
  }
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
  SIPURL localPartyURL(m_dialogFrom);
  PString userName = endpoint.GetRegisteredPartyName(localPartyURL).GetUserName();
  SIPURL contact = endpoint.GetContactURL(*transport, userName, localPartyURL.GetHostName());

  return SendInviteResponse(SIP_PDU::Successful_OK, (const char *) contact.AsQuotedString(), NULL, &sdp);
}


PBoolean SIPConnection::SendInviteResponse(SIP_PDU::StatusCodes code, const char * contact, const char * extra, const SDPSessionDescription * sdp)
{
  OpalTransportAddress viaAddress;
  {
    if (originalInvite == NULL)
      return true;
    viaAddress = originalInvite->GetViaAddress(endpoint);
  }

  SIP_PDU response(*originalInvite, code, contact, extra);
  if (NULL != sdp)
    response.SetSDP(*sdp);
  response.GetMIME().SetProductInfo(endpoint.GetUserAgent(), GetProductInfo());

  if (response.GetStatusCode() >= 200) {
    ackPacket = response;
    ackRetry = endpoint.GetRetryTimeoutMin();
    ackTimer = endpoint.GetAckTimeout();
    ackReceived = false;
  }

  return SendPDU(response, viaAddress); 
}


void SIPConnection::OnInviteResponseRetry(PTimer &, INT)
{
  PSafeLockReadWrite safeLock(*this);
  if (safeLock.IsLocked() && !ackReceived) {
    PTRACE(3, "SIP\tACK not received yet, retry sending response.");
    if (originalInvite != NULL) {
      OpalTransportAddress viaAddress = originalInvite->GetViaAddress(endpoint);
      SendPDU(ackPacket, viaAddress); 
    }
  }
}


void SIPConnection::OnAckTimeout(PTimer &, INT)
{
  PSafeLockReadWrite safeLock(*this);
  if (safeLock.IsLocked() && !ackReceived) {
    PTRACE(1, "SIP\tFailed to receive ACK!");
    ackRetry.Stop();
    releaseMethod = ReleaseWithBYE;
    Release(EndedByTemporaryFailure);
  }
}


PBoolean SIPConnection::SendPDU(SIP_PDU & pdu, const OpalTransportAddress & address)
{
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
          tone = val[0];   // DTMF relay does not use RFC2833 encoding
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
          str << "Signal= " << tone << "\r\n" << "Duration= " << duration << "\r\n";  // spaces are important. Who can guess why?
        }
        else {
          mimeInfo.SetContentType(ApplicationDTMFKey);
          str << tone;
        }
        infoTransaction->GetEntityBody() = str;

        // cannot wait for completion as this keeps the SIPConnection locks, thus preventing the response from being processed
        //infoTransaction->WaitForCompletion();
        //return !infoTransaction->IsFailed();
        return infoTransaction->Start();
      }

    // anything else - send as RFC 2833
    case SendUserInputAsProtocolDefault:
    default:
      break;
  }

  return OpalRTPConnection::SendUserInputTone(tone, duration);
}


void SIPConnection::OnMediaCommand(OpalMediaCommand & command, INT extra)
{
#if OPAL_VIDEO
  if (PIsDescendant(&command, OpalVideoUpdatePicture)) {
    PTRACE(3, "SIP\tSending PictureFastUpdate");
    PSafePtr<SIPTransaction> infoTransaction = new SIPTransaction(*this, *transport, SIP_PDU::Method_INFO);
    SIPMIMEInfo & mimeInfo = infoTransaction->GetMIME();
    mimeInfo.SetContentType(ApplicationMediaControlXMLKey);
    PStringStream str;
    infoTransaction->GetEntityBody() = 
                  "<?xml version=\"1.0\" encoding=\"utf-8\" ?>"
                  "<media_control>"
                   "<vc_primitive>"
                    "<to_encoder>"
                     "<picture_fast_update>"
                     "</picture_fast_update>"
                    "</to_encoder>"
                   "</vc_primitive>"
                  "</media_control>"
                ;
    // cannot wait for completion as this keeps the SIPConnection locks, thus preventing the response from being processed
    //infoTransaction->WaitForCompletion();
    //if (infoTransaction->IsFailed()) { }
    infoTransaction->Start();
#ifdef OPAL_STATISTICS
    m_VideoUpdateRequestsSent++;
#endif
  }
  else
#endif
    OpalRTPConnection::OnMediaCommand(command, extra);
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
        //cout << state << "  " << str << endl;
        unsigned i;
        for (i = 0; i < numStates; ++i) {
          //cout << "comparing '" << str << "' to '" << states[i].str << "'" << endl;
          if ((state == states[i].currState) && (str.compare(0, strlen(states[i].str), states[i].str) == 0)) {
            state = states[i].newState;
            break;
          }
        }
        if (i == numStates) {
          //cout << "unknown string " << str << " in state " << state << endl;
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

    int state;

  protected:
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
        { 0, "?xml",                 1 },
        { 1, "media_control",        2 },
        { 2, "vc_primitive",         3 },
        { 3, "to_encoder",           4 },
        { 4, "picture_fast_update",  5 },
        { 5, "/picture_fast_update", 6 },
        { 6, "/to_encoder",          7 },
        { 7, "/vc_primitive",        8 },
        { 8, "/media_control",       255 },
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
  if (!vfu.Parse(pdu.GetEntityBody()) || !vfu.vfu) {
    PTRACE(3, "SIP\tUnable to parse received PictureFastUpdate");
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
  else {
    PTRACE(3, "SIP\tPictureFastUpdate received");
    if (LockReadWrite()) {
      OpalMediaStreamPtr encodingStream = GetMediaStream(OpalMediaFormat::DefaultVideoSessionID, PTrue);
      if (encodingStream == NULL){
        PTRACE(3, "SIP\tNo video stream to update");
      } else {
        OpalVideoUpdatePicture updatePictureCommand;
        encodingStream->ExecuteCommand(updatePictureCommand);
        PTRACE(3, "SIP\tI-frame requested in video stream");
      }
      UnlockReadWrite();
    }

    SIP_PDU response(pdu, SIP_PDU::Successful_OK);
    SendPDU(response, pdu.GetViaAddress(endpoint));
  }

  return PTrue;
}
#endif


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

#ifdef OPAL_VIDEO
void SIP_RTP_Session::OnRxIntraFrameRequest(const RTP_Session & session) const
{
  // We got an intra frame request control packet, alert the encoder.
  // We're going to grab the call, find the other connection, then grab the
  // encoding stream
  PSafePtr<OpalConnection> otherConnection = connection.GetCall().GetOtherPartyConnection(connection);
  if (otherConnection == NULL)
    return; // No other connection.  Bail.

  // Found the encoding stream, send an OpalVideoFastUpdatePicture
  OpalMediaStreamPtr encodingStream = otherConnection->GetMediaStream(session.GetSessionID(), PTrue);
  if (encodingStream) {
    OpalVideoUpdatePicture updatePictureCommand;
    encodingStream->ExecuteCommand(updatePictureCommand);
  }
}

void SIP_RTP_Session::OnTxIntraFrameRequest(const RTP_Session & /*session*/) const
{
}

#endif // OPAL_VIDEO

void SIPConnection::OnSessionTimeout(PTimer &, INT)
{
  //SIPTransaction * invite = new SIPInvite(*this, *transport, rtpSessions);  
  //invite->Start();  
  //sessionTimer = 10000;
}

#endif // OPAL_SIP

// End of file ////////////////////////////////////////////////////////////////
