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

#include <opal/buildopts.h>
#ifdef OPAL_SIP

#ifdef __GNUC__
#pragma implementation "sipcon.h"
#endif

#ifdef HAS_LIBZRTP
#include <zrtp/opalzrtp.h>
#include <rtp/zrtpudp.h>
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
  : OpalConnection(call, ep, token, options, stringOptions)
  , endpoint(ep)
  , transport(newTransport)
  , local_hold(false)
  , remote_hold(false)
  , originalInvite(NULL)
  , needReINVITE(false)
  , targetAddress(destination)
  , ackReceived(false)
  , releaseMethod(ReleaseWithNothing)
{
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

  forkedInvitations.DisallowDeleteObjects();

  ackTimer.SetNotifier(PCREATE_NOTIFIER(OnAckTimeout));
  ackRetry.SetNotifier(PCREATE_NOTIFIER(OnInviteResponseRetry));

#ifdef HAS_LIBZRTP
  zrtpSession = new(zrtp_conn_ctx_t);
  ::zrtp_init_session_ctx(zrtpSession, OpalZrtp::GetZrtpContext(), OpalZrtp::GetZID());
  zrtpSession->ctx_usr_data = this;
#endif

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
#ifdef HAS_LIBZRTP
  if (zrtpSession){
    ::zrtp_done_session_ctx(zrtpSession);
    delete (zrtpSession);
  }
#endif

  delete originalInvite;
  originalInvite = NULL;

  delete transport;

  PTRACE(4, "SIP\tDeleted connection.");
}

#ifdef HAS_LIBZRTP
RTP_Session * SIPConnection::CreateSession(const OpalTransport & transport,
                                           unsigned sessionID,
                                           RTP_QOS * rtpqos)
{
  // We only support RTP over UDP at this point in time ...
  if (!transport.IsCompatibleTransport("ip$127.0.0.1")) 
    return NULL;

  // We support video, audio and T38 over IP
  if (sessionID != OpalMediaFormat::DefaultAudioSessionID && 
      sessionID != OpalMediaFormat::DefaultVideoSessionID
#if OPAL_T38FAX
      && sessionID != OpalMediaFormat::DefaultDataSessionID
#endif
      )
    {
      return NULL;
  }

  PIPSocket::Address localAddress;
 
  transport.GetLocalAddress().GetIpAddress(localAddress);

  OpalManager & manager = GetEndPoint().GetManager();

  PIPSocket::Address remoteAddress;
  transport.GetRemoteAddress().GetIpAddress(remoteAddress);
  PSTUNClient * stun = manager.GetSTUN(remoteAddress);

  // create an (S)RTP session or T38 pseudo-session as appropriate
  RTP_UDP * rtpSession = NULL;

#if OPAL_T38FAX
  if (sessionID == OpalMediaFormat::DefaultDataSessionID) {
    rtpSession = new T38PseudoRTP(
#if OPAL_RTP_AGGREGATE
                                        NULL,
#endif
                                        sessionID, remoteIsNAT);
  }
  else
#endif

///////////
//  securityMode = "ZRTP|DEFAULT";
//////////

  if (!securityMode.IsEmpty()) {
          printf("call Factory for security mode\n");
    OpalSecurityMode * parms = PFactory<OpalSecurityMode>::CreateInstance(securityMode);
    if (parms == NULL) {
      printf("can't create security mode ZRTP\n");
      PTRACE(1, "OpalCon\tSecurity mode " << securityMode << " unknown");
      return NULL;
    }
    
    printf("create ZRTP security mode\n");
    
    rtpSession = parms->CreateRTPSession(
#if OPAL_RTP_AGGREGATE
                  useRTPAggregation ? endpoint.GetRTPAggregator() : NULL, 
#endif
                  sessionID, remoteIsNAT, *this);

    if (rtpSession == NULL) {
      PTRACE(1, "OpalCon\tCannot create RTP session for security mode " << securityMode);
      delete parms;
      return NULL;
    }

    //check is security mode ZRTP
    if (0 == securityMode.Find("ZRTP")) {
      OpalZrtp_UDP* castedSession = (OpalZrtp_UDP*) rtpSession;
      int ssrc = castedSession->GetOutgoingSSRC();

      // attach new zrtp stream to zrtp connection and start it
      OpalSecurityMode *security_mode = castedSession->GetSecurityParms();
      if (NULL != security_mode){
        castedSession->zrtpStream = ::zrtp_attach_stream(zrtpSession, 
                                                         ((LibZrtpSecurityMode_Base*)security_mode)->GetZrtpProfile(), 
                                                         ssrc);
      } else {
        castedSession->zrtpStream = ::zrtp_attach_stream(zrtpSession, NULL, ssrc);
      }

      printf("attaching stream to sessionID %d\n", sessionID);
      if (castedSession->zrtpStream) {
        castedSession->zrtpStream->stream_usr_data = rtpSession;
        zrtp_start_stream(castedSession->zrtpStream);
      }
    }
  } else {
    rtpSession = new RTP_UDP(
#if OPAL_RTP_AGGREGATE
                             useRTPAggregation ? endpoint.GetRTPAggregator() : NULL, 
#endif
                             sessionID, remoteIsNAT);
  }

  WORD firstPort = manager.GetRtpIpPortPair();
  WORD nextPort = firstPort;
  while (!rtpSession->Open(localAddress, nextPort, nextPort, manager.GetRtpIpTypeofService(), stun, rtpqos)) {
    nextPort = manager.GetRtpIpPortPair();
    if (nextPort == firstPort) {
      PTRACE(1, "OpalCon\tNo ports available for RTP session " << sessionID << " for " << *this);
      delete rtpSession;
      return NULL;
    }
  }

  localAddress = rtpSession->GetLocalAddress();
  if (manager.TranslateIPAddress(localAddress, remoteAddress)){
    rtpSession->SetLocalAddress(localAddress);
  }
  
  return rtpSession;
}

void SIPConnection::ReleaseSession(unsigned sessionID, PBoolean clearAll/* = PFalse */)
{
  //check is security mode ZRTP
  if (0 == securityMode.Find("ZRTP")) {
    RTP_Session *session = GetSession(sessionID);
    if (NULL != session){
      OpalZrtp_UDP *zsession = (OpalZrtp_UDP*)session;
      if (NULL != zsession->zrtpStream){
        ::zrtp_stop_stream(zsession->zrtpStream);
        zsession->zrtpStream = NULL;
      }
    }
  }

  printf("release session %i\n", sessionID);

  OpalConnection::ReleaseSession(sessionID, clearAll);
}
#endif //HAS_LIBZRTP


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
    if (!OnSendSDP(true, rtpSessions, sdpOut)) {
      SendInviteResponse(SIP_PDU::Failure_NotAcceptableHere);
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
  if (!OnSendSDP(true, rtpSessions, sdpOut)) {
    SendInviteResponse(SIP_PDU::Failure_NotAcceptableHere);
    Release(EndedByCapabilityExchange);
    return PFalse;
  }
    
  // update the route set and the target address according to 12.1.1
  // requests in a dialog do not modify the route set according to 12.2
  if (phase < ConnectedPhase) {
    routeSet.RemoveAll();

    if (originalInvite != NULL) {
      routeSet = originalInvite->GetMIME().GetRecordRoute();
      PString originalContact = originalInvite->GetMIME().GetContact();
      if (!originalContact.IsEmpty()) 
        targetAddress = originalContact;
    }
  }

  // send the 200 OK response
  SendInviteOK(sdpOut);

  releaseMethod = ReleaseWithBYE;

  // switch phase 
  SetPhase(ConnectedPhase);
  connectedTime = PTime ();

  // if media was previously set up, then move to Established
  if (!mediaStreams.IsEmpty()) {
    SetPhase(EstablishedPhase);
    OnEstablished();
  }
  
  return PTrue;
}


RTP_UDP *SIPConnection::OnUseRTPSession(const unsigned rtpSessionId, const OpalTransportAddress & mediaAddress, OpalTransportAddress & localAddress)
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
    OpalConnection * otherParty = GetCall().GetOtherPartyConnection(*this);
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
    rtpSession = (RTP_UDP *)UseSession(GetTransport(), rtpSessionId, NULL);
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


PBoolean SIPConnection::OnSendSDP(bool isAnswerSDP, RTP_SessionManager & rtpSessions, SDPSessionDescription & sdpOut)
{
  bool sdpOK;

  if (isAnswerSDP)
    needReINVITE = false;

  // get the remote media formats, if any
  if (isAnswerSDP && originalInvite != NULL && originalInvite->HasSDP()) {
    // Use |= to avoid McCarthy boolean || from not calling video/fax
    sdpOK  = AnswerSDPMediaDescription(originalInvite->GetSDP(), SDPMediaDescription::Audio, OpalMediaFormat::DefaultAudioSessionID, sdpOut);
#if OPAL_VIDEO
    sdpOK |= AnswerSDPMediaDescription(originalInvite->GetSDP(), SDPMediaDescription::Video, OpalMediaFormat::DefaultVideoSessionID, sdpOut);
#endif
#if OPAL_T38FAX
    sdpOK |= AnswerSDPMediaDescription(originalInvite->GetSDP(), SDPMediaDescription::Image, OpalMediaFormat::DefaultDataSessionID,  sdpOut);
#endif
  }
  
  else {

    // construct offer as per RFC 3261, para 14.2
    // Use |= to avoid McCarthy boolean || from not calling video/fax
    sdpOK  = OfferSDPMediaDescription(OpalMediaFormat::DefaultAudioSessionID, rtpSessions, sdpOut);
#if OPAL_VIDEO
    sdpOK |= OfferSDPMediaDescription(OpalMediaFormat::DefaultVideoSessionID, rtpSessions, sdpOut);
#endif
#if OPAL_T38FAX
    sdpOK |= OfferSDPMediaDescription(OpalMediaFormat::DefaultDataSessionID, rtpSessions, sdpOut);
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


bool SIPConnection::OfferSDPMediaDescription(unsigned rtpSessionId,
                                             RTP_SessionManager & rtpSessions,
                                             SDPSessionDescription & sdp)
{
  OpalTransportAddress localAddress;
  RTP_DataFrame::PayloadTypes ntePayloadCode = RTP_DataFrame::IllegalPayloadType;
#if OPAL_T38FAX
  RTP_DataFrame::PayloadTypes nsePayloadCode = RTP_DataFrame::IllegalPayloadType;
#endif

  OpalMediaFormatList formats = GetLocalMediaFormats();

  // See if any media formats of this session id, so don't create unused RTP session
  OpalMediaFormatList::iterator format;
  for (format = formats.begin(); format != formats.end(); ++format) {
    if (format->GetDefaultSessionID() == rtpSessionId &&
            (rtpSessionId == OpalMediaFormat::DefaultDataSessionID || format->IsTransportable()))
      break;
  }
  if (format == formats.end()) {
    PTRACE(3, "SIP\tNo media formats for session id " << rtpSessionId << ", not adding SDP");
    return PFalse;
  }

  if (ownerCall.IsMediaBypassPossible(*this, rtpSessionId)) {
    PSafePtr<OpalConnection> otherParty = GetCall().GetOtherPartyConnection(*this);
    if (otherParty != NULL) {
      MediaInformation info;
      if (otherParty->GetMediaInformation(rtpSessionId, info)) {
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
    RTP_Session * rtpSession = rtpSessions.UseSession(rtpSessionId);
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
      rtpSessions.AddSession(rtpSession);
    }

    if (rtpSession == NULL)
      return false;

    localAddress = GetLocalAddress(((RTP_UDP *)rtpSession)->GetLocalDataPort());
  }

  if (sdp.GetDefaultConnectAddress().IsEmpty())
    sdp.SetDefaultConnectAddress(localAddress);

  SDPMediaDescription * localMedia;
  switch (rtpSessionId) {
    case OpalMediaFormat::DefaultAudioSessionID:
      localMedia = new SDPMediaDescription(localAddress, SDPMediaDescription::Audio);
      break;

#if OPAL_VIDEO
    case OpalMediaFormat::DefaultVideoSessionID:
      localMedia = new SDPMediaDescription(localAddress, SDPMediaDescription::Video);
      break;
#endif

#if OPAL_T38FAX
    case OpalMediaFormat::DefaultDataSessionID:
#endif
      if (localAddress.IsEmpty()) {
        PTRACE(2, "SIP\tRefusing to add SDP media description for session id " << rtpSessionId << " with no transport address");
        return false;
      }
      localMedia = new SDPMediaDescription(localAddress, SDPMediaDescription::Image);
      break;

    default:
      return false;
  }

  if (needReINVITE) {
    PSafePtr<OpalMediaStream> sendStream = GetMediaStream(rtpSessionId, false);
    bool sending = sendStream != NULL && sendStream->IsOpen();
    PSafePtr<OpalMediaStream> recvStream = GetMediaStream(rtpSessionId, true);
    bool recving = recvStream != NULL && recvStream->IsOpen();
    if (sending) {
      localMedia->AddMediaFormat(sendStream->GetMediaFormat(), rtpPayloadMap);
      localMedia->SetDirection(recving ? SDPMediaDescription::SendRecv : SDPMediaDescription::SendOnly);
    }
    else if (recving) {
      localMedia->AddMediaFormat(recvStream->GetMediaFormat(), rtpPayloadMap);
      localMedia->SetDirection(SDPMediaDescription::RecvOnly);
    }
    else {
      localMedia->AddMediaFormats(formats, rtpSessionId, rtpPayloadMap);
      localMedia->SetDirection(SDPMediaDescription::Inactive);
    }
  }
  else {
    localMedia->AddMediaFormats(formats, rtpSessionId, rtpPayloadMap);

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
  if (rtpSessionId == OpalMediaFormat::DefaultAudioSessionID) {
    SetNXEPayloadCode(localMedia, ntePayloadCode, rfc2833Handler,  OpalRFC2833, OpalDefaultNTEString, "NTE"); // RFC 2833
#if OPAL_T38FAX
    SetNXEPayloadCode(localMedia, nsePayloadCode, ciscoNSEHandler, OpalCiscoNSE, OpalDefaultNSEString, "NSE"); // Cisco NSE
#endif
  }

  sdp.AddMediaDescription(localMedia);

  return true;
}


PBoolean SIPConnection::AnswerSDPMediaDescription(const SDPSessionDescription & sdpIn,
                                              SDPMediaDescription::MediaType rtpMediaType,
                                              unsigned rtpSessionId,
                                              SDPSessionDescription & sdpOut)
{
  RTP_UDP * rtpSession = NULL;
  SDPMediaDescription * localMedia = NULL;

  // if no matching media type, return PFalse
  SDPMediaDescription * incomingMedia = sdpIn.GetMediaDescription(rtpMediaType);
  if (incomingMedia == NULL) {
    PTRACE(2, "SIP\tCould not find matching media type for session " << rtpSessionId);
    return PFalse;
  }

  OpalMediaFormatList sdpFormats = incomingMedia->GetMediaFormats(rtpSessionId);
  sdpFormats.Remove(endpoint.GetManager().GetMediaFormatMask());
  if (sdpFormats.GetSize() == 0) {
    PTRACE(1, "SIP\tCould not find media formats in SDP media description for session " << rtpSessionId);
    // Send back a m= line with port value zero and the first entry of the offer payload types as per RFC3264
    localMedia = new SDPMediaDescription(OpalTransportAddress(), rtpMediaType);
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
#if OPAL_T38FAX
  PBoolean hasNSE = PFalse;
#endif
  for (SDPMediaFormatList::const_iterator format = sdpMediaList.begin(); format != sdpMediaList.end(); ++format) {
    if (format->GetEncodingName() == "telephone-event") {
      rfc2833Handler->SetPayloadType(format->GetPayloadType());
      remoteFormatList += OpalRFC2833;
      hasTelephoneEvent = PTrue;
    }
#if OPAL_T38FAX
    if (format->GetEncodingName() == "nse") {
      ciscoNSEHandler->SetPayloadType(format->GetPayloadType());
      remoteFormatList += OpalCiscoNSE;
      hasNSE = PTrue;
    }
#endif
  }

  OpalTransportAddress localAddress;
  OpalTransportAddress mediaAddress = incomingMedia->GetTransportAddress();
  if (!mediaAddress.IsEmpty()) {

    PIPSocket::Address ip;
    WORD port = 0;
    if (!mediaAddress.GetIpAndPort(ip, port)) {
      PTRACE(1, "SIP\tCannot get remote ports for RTP session " << rtpSessionId);
      if (rtpSessionId == OpalMediaFormat::DefaultAudioSessionID) 
        Release(EndedByTransportFail);
      return PFalse;
    }

    // Create the RTPSession if required
    rtpSession = OnUseRTPSession(rtpSessionId, mediaAddress, localAddress);
    if (rtpSession == NULL && !ownerCall.IsMediaBypassPossible(*this, rtpSessionId)) {
      if (rtpSessionId == OpalMediaFormat::DefaultAudioSessionID) 
        Release(EndedByTransportFail);
      return PFalse;
    }

    // set the remote address, but note that endpoints send IP address 
    // of 0.0.0.0 when pausing media
    if (ip != 0) {
      if ((rtpSession != NULL)&& !rtpSession->SetRemoteSocketInfo(ip, port, PTrue)) {
        PTRACE(1, "SIP\tCannot set remote ports on RTP session");
        if (rtpSessionId == OpalMediaFormat::DefaultAudioSessionID) 
          Release(EndedByTransportFail);
        return PFalse;
      }
    }
  }

  // construct a new media session list 
  localMedia = new SDPMediaDescription(localAddress, rtpMediaType);

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

  // Check if we had a stream and the remote has either changed the codec or
  // changed the direction of the stream
  PSafePtr<OpalMediaStream> sendStream = GetMediaStream(rtpSessionId, false);
  bool sending = sendStream != NULL && sendStream->IsOpen();
  if (sending && ((otherSidesDir&SDPMediaDescription::RecvOnly) == 0 || !sdpFormats.HasFormat(sendStream->GetMediaFormat()))) {
    sendStream->Close();
    sending = false;
  }

  PSafePtr<OpalMediaStream> recvStream = GetMediaStream(rtpSessionId, true);
  bool recving = recvStream != NULL && recvStream->IsOpen();
  if (recving && ((otherSidesDir&SDPMediaDescription::SendOnly) == 0 || !sdpFormats.HasFormat(recvStream->GetMediaFormat()))) {
    recvStream->Close();
    recving = false;
  }

  // After (possibly) closing streams, we now open them again if necessary
  // OpenSourceMediaStreams will just return true if they are already open
  if ((otherSidesDir&SDPMediaDescription::SendOnly) != 0)
    recving = ownerCall.OpenSourceMediaStreams(*this, rtpSessionId);

  PSafePtr<OpalConnection> otherParty = GetCall().GetOtherPartyConnection(*this);
  if ((otherSidesDir&SDPMediaDescription::RecvOnly) != 0 && otherParty != NULL)
    sending = ownerCall.OpenSourceMediaStreams(*otherParty, rtpSessionId);

  // Now we build the reply, setting "direction" as appropriate for what we opened.
  if (sending) {
    if (sendStream == NULL)
      sendStream = GetMediaStream(rtpSessionId, false);
    localMedia->AddMediaFormat(sendStream->GetMediaFormat(), rtpPayloadMap);
    localMedia->SetDirection(recving ? SDPMediaDescription::SendRecv : SDPMediaDescription::SendOnly);
  }
  else if (recving) {
    if (recvStream == NULL)
      recvStream = GetMediaStream(rtpSessionId, true);
    localMedia->AddMediaFormat(recvStream->GetMediaFormat(), rtpPayloadMap);
    localMedia->SetDirection(SDPMediaDescription::RecvOnly);
  }
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
#if OPAL_T38FAX
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
  OpalMediaStreamPtr stream = OpalConnection::OpenMediaStream(mediaFormat, sessionID, isSource);

  if (stream != NULL && needReINVITE) {
    SIPTransaction * invite = new SIPInvite(*this, *transport, rtpSessions);
    invite->Start();
  }

  return stream;
}


bool SIPConnection::CloseMediaStream(OpalMediaStream & stream)
{
  bool ok = OpalConnection::CloseMediaStream(stream);

  if (GetPhase() == EstablishedPhase && needReINVITE) {
    SIPTransaction * invite = new SIPInvite(*this, *transport, rtpSessions);
    invite->Start();
  }

  return ok;
}


OpalMediaStream * SIPConnection::CreateMediaStream(const OpalMediaFormat & mediaFormat,
                                                   unsigned sessionID,
                                                   PBoolean isSource)
{
  // Use a NULL stream if media is bypassing us, 
  if (ownerCall.IsMediaBypassPossible(*this, sessionID)) {
    PTRACE(3, "SIP\tBypassing media for session " << sessionID);
    return new OpalNullMediaStream(*this, mediaFormat, sessionID, isSource);
  }

  // if no RTP sessions matching this session ID, then nothing to do
  if (rtpSessions.GetSession(sessionID) == NULL)
    return NULL;

  return new OpalRTPMediaStream(*this, mediaFormat, isSource, *rtpSessions.GetSession(sessionID),
                                GetMinAudioJitterDelay(),
                                GetMaxAudioJitterDelay());
}

void SIPConnection::OnPatchMediaStream(PBoolean isSource, OpalMediaPatch & patch)
{
  OpalConnection::OnPatchMediaStream(isSource, patch);
  if(patch.GetSource().GetSessionID() == OpalMediaFormat::DefaultAudioSessionID) {
    AttachRFC2833HandlerToPatch(isSource, patch);
  }
}


PBoolean SIPConnection::IsMediaBypassPossible(unsigned sessionID) const
{
  return sessionID == OpalMediaFormat::DefaultAudioSessionID ||
         sessionID == OpalMediaFormat::DefaultVideoSessionID;
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
    transportAddress = routeSet.front();
  else {
    transportAddress = targetAddress;
    transportAddress.AdjustToDNS(); // Do a DNS SRV lookup
    PTRACE(4, "SIP\tConnecting to " << targetAddress << " via " << transportAddress);
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
        transport->SetInterface(transaction.GetInterface());
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
        for (PStringList::iterator route = recordRoute.rbegin(); route != recordRoute.rend(); --route)
          routeSet.AppendString(*route);
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
      PString origToTag   = originalInvite->GetMIME().GetFieldParameter("tag", originalMIME.GetTo());
      PString origFromTag = originalInvite->GetMIME().GetFieldParameter("tag", originalMIME.GetFrom());
      PString fromTag = request.GetMIME().GetFieldParameter("tag", requestFrom);
      PString toTag   = request.GetMIME().GetFieldParameter("tag", requestTo);
      if (fromTag != origFromTag || toTag != origToTag) {
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
    if ((sdpIn.GetDirection(OpalMediaFormat::DefaultAudioSessionID)&SDPMediaDescription::RecvOnly) == 0 &&
        (sdpIn.GetDirection(OpalMediaFormat::DefaultVideoSessionID)&SDPMediaDescription::RecvOnly) == 0) {

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
  
  // send the 200 OK response
  if (OnSendSDP(true, rtpSessions, sdpOut))
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
      SDPMediaDescription::MediaType mediaType;
      unsigned sessionID;
    } previewTypes[] = 
    { 
      { SDPMediaDescription::Audio, OpalMediaFormat::DefaultAudioSessionID },
#if OPAL_VIDEO
      { SDPMediaDescription::Video, OpalMediaFormat::DefaultVideoSessionID },
#endif
#if OPAL_T38FAX
      { SDPMediaDescription::Image, OpalMediaFormat::DefaultDataSessionID },
#endif
    };
    PINDEX i;
    for (i = 0; i < (PINDEX) (sizeof(previewTypes)/sizeof(previewTypes[0])); ++i) {
      SDPMediaDescription * mediaDescription = sdp.GetMediaDescription(previewTypes[i].mediaType);
      if (mediaDescription != NULL) {
        previewFormats += mediaDescription->GetMediaFormats(previewTypes[i].sessionID);
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
  PString origFromTag = originalInvite->GetMIME().GetFieldParameter("tag", originalInvite->GetMIME().GetFrom());
  PString origToTag   = originalInvite->GetMIME().GetFieldParameter("tag", originalInvite->GetMIME().GetTo());
  PString fromTag     = response.GetMIME().GetFieldParameter("tag", response.GetMIME().GetFrom());
  PString toTag       = response.GetMIME().GetFieldParameter("tag", response.GetMIME().GetTo());
  if (fromTag != origFromTag || (!toTag.IsEmpty() && (toTag != origToTag))) {
    PTRACE(3, "SIP\tACK received for forked INVITE from " << response.GetURI());
    return;
  }

  PTRACE(3, "SIP\tACK received: " << phase);

  ackReceived = true;
  ackTimer.Stop(false); // Asynchornous stop to avoid deadlock
  ackRetry.Stop(false);
  
  OnReceivedSDP(response);

  // If we receive an ACK in established phase, it is a re-INVITE
  if (phase == EstablishedPhase) {
    StartMediaStreams();
    return;
  }
  
  if (phase != ConnectedPhase)  
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
  remotePartyAddress.Delete(remotePartyAddress.Find (';'), P_MAX_INDEX);
  
  if (proxy.IsEmpty())
    proxy = endpoint.GetProxy();

  // Default routeSet if there is a proxy
  if (!proxy.IsEmpty() && routeSet.GetSize() == 0) 
    routeSet += "sip:" + proxy.GetHostName() + ':' + PString(proxy.GetPort()) + ";lr";

  needReINVITE = false; // Is not actually a re-INVITE though it looks a little bit like one.
  RTP_SessionManager & origRtpSessions = ((SIPInvite &)transaction).GetSessionManager();
  SIPTransaction * invite = new SIPInvite(*this, *transport, origRtpSessions);
  transport->SetInterface(transaction.GetInterface());
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

  // Is an OK to our re-INVITE
  if (phase == EstablishedPhase)
    return;

  SetPhase(EstablishedPhase);

  connectedTime = PTime();
  OnConnected();

  OnEstablished();
}


void SIPConnection::OnReceivedSDP(SIP_PDU & pdu)
{
  if (!pdu.HasSDP())
    return;

  needReINVITE = false;

  bool ok = OnReceivedSDPMediaDescription(pdu.GetSDP(), SDPMediaDescription::Audio, OpalMediaFormat::DefaultAudioSessionID);

  remoteFormatList += OpalRFC2833;

#if OPAL_VIDEO
  ok |= OnReceivedSDPMediaDescription(pdu.GetSDP(), SDPMediaDescription::Video, OpalMediaFormat::DefaultVideoSessionID);
#endif

#if OPAL_T38FAX
  ok |= OnReceivedSDPMediaDescription(pdu.GetSDP(), SDPMediaDescription::Image, OpalMediaFormat::DefaultDataSessionID);
#endif

  needReINVITE = true;

  if (GetPhase() == EstablishedPhase) // re-INVITE
    StartMediaStreams();
  else if (!ok)
    Release(EndedByCapabilityExchange);
}


bool SIPConnection::OnReceivedSDPMediaDescription(SDPSessionDescription & sdp,
                                                  SDPMediaDescription::MediaType mediaType,
                                                  unsigned rtpSessionId)
{
  RTP_UDP *rtpSession = NULL;
  SDPMediaDescription * mediaDescription = sdp.GetMediaDescription(mediaType);
  
  if (mediaDescription == NULL) {
    PTRACE(mediaType <= SDPMediaDescription::Video ? 2 : 3, "SIP\tCould not find SDP media description for " << mediaType);
    return false;
  }

  // see if the remote supports this media
  OpalMediaFormatList mediaFormatList = mediaDescription->GetMediaFormats(rtpSessionId);
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
    rtpSession = OnUseRTPSession(rtpSessionId, address, localAddress);
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
  if (sendStream != NULL && sendStream->IsOpen() &&
        ((otherSidesDir&SDPMediaDescription::RecvOnly) == 0 || !mediaFormatList.HasFormat(sendStream->GetMediaFormat())))
    sendStream->Close();

  OpalMediaStreamPtr recvStream = GetMediaStream(rtpSessionId, true);
  if (recvStream != NULL && recvStream->IsOpen() &&
        ((otherSidesDir&SDPMediaDescription::SendOnly) == 0 || !mediaFormatList.HasFormat(recvStream->GetMediaFormat())))
    recvStream->Close();

  // Then open the streams if the direction allows
  if ((otherSidesDir&SDPMediaDescription::SendOnly) != 0)
    ownerCall.OpenSourceMediaStreams(*this, rtpSessionId);

  PSafePtr<OpalConnection> otherParty = GetCall().GetOtherPartyConnection(*this);
  if ((otherSidesDir&SDPMediaDescription::RecvOnly) != 0 && otherParty != NULL)
    ownerCall.OpenSourceMediaStreams(*otherParty, rtpSessionId);

  return true;
}


void SIPConnection::OnCreatingINVITE(SIP_PDU & /*request*/)
{
  PTRACE(3, "SIP\tCreating INVITE request");
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

  if (response.GetStatusCode()/100 != 1) {
    if (response.GetStatusCode() > 299) {
      PTRACE(3, "SIP\tACK sent for error");
    }
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
          tone = OpalRFC2833Proto::RFC2833ToASCII(val.AsUnsigned());
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

    OpalMediaStreamPtr encodingStream = GetMediaStream(OpalMediaFormat::DefaultVideoSessionID, PTrue);

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


#endif // OPAL_SIP

// End of file ////////////////////////////////////////////////////////////////
