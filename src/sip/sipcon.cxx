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
#include <h323/q931.h>

#if OPAL_HAS_H224
#include <h224/h224.h>
#endif

#if OPAL_HAS_IM
#include <im/msrp.h>
#endif

#define new PNEW

//
//  uncomment this to force pause ReINVITES to have c=0.0.0.0
//
//#define PAUSE_WITH_EMPTY_ADDRESS  1

typedef void (SIPConnection::* SIPMethodFunction)(SIP_PDU & pdu);

static const char HeaderPrefix[] = SIP_HEADER_PREFIX;
static const char TagParamName[] = ";tag=";
static const char ApplicationDTMFRelayKey[]       = "application/dtmf-relay";
static const char ApplicationDTMFKey[]            = "application/dtmf";

#if OPAL_VIDEO
static const char ApplicationMediaControlXMLKey[] = "application/media_control+xml";
#endif


static SIP_PDU::StatusCodes GetStatusCodeFromReason(OpalConnection::CallEndReason reason)
{
  static const struct {
    unsigned             q931Code;
    SIP_PDU::StatusCodes sipCode;
  }
  //
  // This table comes from RFC 3398 para 7.2.4.1
  //
  Q931ToSIPCode[] = {
    {   1, SIP_PDU::Failure_NotFound               }, // Unallocated number
    {   2, SIP_PDU::Failure_NotFound               }, // no route to network
    {   3, SIP_PDU::Failure_NotFound               }, // no route to destination
    {  17, SIP_PDU::Failure_BusyHere               }, // user busy                            
    {  18, SIP_PDU::Failure_RequestTimeout         }, // no user responding                   
    {  19, SIP_PDU::Failure_TemporarilyUnavailable }, // no answer from the user              
    {  20, SIP_PDU::Failure_TemporarilyUnavailable }, // subscriber absent                    
    {  21, SIP_PDU::Failure_Forbidden              }, // call rejected                        
    {  22, SIP_PDU::Failure_Gone                   }, // number changed (w/o diagnostic)      
    {  22, SIP_PDU::Redirection_MovedPermanently   }, // number changed (w/ diagnostic)       
    {  23, SIP_PDU::Failure_Gone                   }, // redirection to new destination       
    {  26, SIP_PDU::Failure_NotFound               }, // non-selected user clearing           
    {  27, SIP_PDU::Failure_BadGateway             }, // destination out of order             
    {  28, SIP_PDU::Failure_AddressIncomplete      }, // address incomplete                   
    {  29, SIP_PDU::Failure_NotImplemented         }, // facility rejected                    
    {  31, SIP_PDU::Failure_TemporarilyUnavailable }, // normal unspecified                   
    {  34, SIP_PDU::Failure_ServiceUnavailable     }, // no circuit available                 
    {  38, SIP_PDU::Failure_ServiceUnavailable     }, // network out of order                 
    {  41, SIP_PDU::Failure_ServiceUnavailable     }, // temporary failure                    
    {  42, SIP_PDU::Failure_ServiceUnavailable     }, // switching equipment congestion       
    {  47, SIP_PDU::Failure_ServiceUnavailable     }, // resource unavailable                 
    {  55, SIP_PDU::Failure_Forbidden              }, // incoming calls barred within CUG     
    {  57, SIP_PDU::Failure_Forbidden              }, // bearer capability not authorized     
    {  58, SIP_PDU::Failure_ServiceUnavailable     }, // bearer capability not presently available
    {  65, SIP_PDU::Failure_NotAcceptableHere      }, // bearer capability not implemented
    {  70, SIP_PDU::Failure_NotAcceptableHere      }, // only restricted digital avail    
    {  79, SIP_PDU::Failure_NotImplemented         }, // service or option not implemented
    {  87, SIP_PDU::Failure_Forbidden              }, // user not member of CUG           
    {  88, SIP_PDU::Failure_ServiceUnavailable     }, // incompatible destination         
    { 102, SIP_PDU::Failure_ServerTimeout          }, // recovery of timer expiry         
    { 111, SIP_PDU::Failure_InternalServerError    }, // protocol error                   
    { 127, SIP_PDU::Failure_InternalServerError    }, // interworking unspecified         
  };
  for (PINDEX i = 0; i < PARRAYSIZE(Q931ToSIPCode); i++) {
    if (Q931ToSIPCode[i].q931Code == reason.q931)
      return Q931ToSIPCode[i].sipCode;
  }

  static const struct {
    OpalConnection::CallEndReasonCodes reasonCode;
    SIP_PDU::StatusCodes               sipCode;
  }
  ReasonToSIPCode[] = {
    { OpalConnection::EndedByNoUser            , SIP_PDU::Failure_NotFound               }, // Unallocated number
    { OpalConnection::EndedByLocalBusy         , SIP_PDU::Failure_BusyHere               }, // user busy                            
    { OpalConnection::EndedByNoAnswer          , SIP_PDU::Failure_RequestTimeout         }, // no user responding                   
    { OpalConnection::EndedByNoUser            , SIP_PDU::Failure_TemporarilyUnavailable }, // subscriber absent                    
    { OpalConnection::EndedByCapabilityExchange, SIP_PDU::Failure_NotAcceptableHere      }, // bearer capability not implemented
    { OpalConnection::EndedByCallerAbort       , SIP_PDU::Failure_RequestTerminated      },
    { OpalConnection::EndedByCallForwarded     , SIP_PDU::Redirection_MovedTemporarily   },
    { OpalConnection::EndedByAnswerDenied      , SIP_PDU::GlobalFailure_Decline          },
    { OpalConnection::EndedByRefusal           , SIP_PDU::GlobalFailure_Decline          }, // TODO - SGW - add for call reject from H323 side.
    { OpalConnection::EndedByHostOffline       , SIP_PDU::Failure_NotFound               }, // TODO - SGW - add for no ip from H323 side.
    { OpalConnection::EndedByNoEndPoint        , SIP_PDU::Failure_NotFound               }, // TODO - SGW - add for endpoints not running on a ip from H323 side.
    { OpalConnection::EndedByUnreachable       , SIP_PDU::Failure_Forbidden              }, // TODO - SGW - add for avoid sip calls to SGW IP.
    { OpalConnection::EndedByNoBandwidth       , SIP_PDU::GlobalFailure_NotAcceptable    }, // TODO - SGW - added to reject call when no bandwidth 
  };

  for (PINDEX i = 0; i < PARRAYSIZE(ReasonToSIPCode); i++) {
    if (ReasonToSIPCode[i].reasonCode == reason.code)
      return ReasonToSIPCode[i].sipCode;
  }

  return SIP_PDU::Failure_BadGateway;
}

static OpalConnection::CallEndReason GetCallEndReasonFromResponse(SIP_PDU & response)
{
  //
  // This table comes from RFC 3398 para 8.2.6.1
  //
  static const struct {
    SIP_PDU::StatusCodes               sipCode;
    OpalConnection::CallEndReasonCodes reasonCode;
    unsigned                           q931Code;
  } SIPCodeToReason[] = {
    { SIP_PDU::Local_Timeout                      , OpalConnection::EndedByHostOffline       ,  27 }, // Destination out of order
    { SIP_PDU::Failure_RequestTerminated          , OpalConnection::EndedByNoAnswer          ,  19 }, // No answer
    { SIP_PDU::Failure_BadRequest                 , OpalConnection::EndedByQ931Cause         ,  41 }, // Temporary Failure
    { SIP_PDU::Failure_UnAuthorised               , OpalConnection::EndedBySecurityDenial    ,  21 }, // Call rejected (*)
    { SIP_PDU::Failure_PaymentRequired            , OpalConnection::EndedByRefusal           ,  21 }, // Call rejected
    { SIP_PDU::Failure_Forbidden                  , OpalConnection::EndedBySecurityDenial    ,  21 }, // Call rejected
    { SIP_PDU::Failure_NotFound                   , OpalConnection::EndedByNoUser            ,   1 }, // Unallocated number
    { SIP_PDU::Failure_MethodNotAllowed           , OpalConnection::EndedByQ931Cause         ,  63 }, // Service or option unavailable
    { SIP_PDU::Failure_NotAcceptable              , OpalConnection::EndedByQ931Cause         ,  79 }, // Service/option not implemented (+)
    { SIP_PDU::Failure_ProxyAuthenticationRequired, OpalConnection::EndedBySecurityDenial    ,  21 }, // Call rejected (*)
    { SIP_PDU::Failure_RequestTimeout             , OpalConnection::EndedByTemporaryFailure  , 102 }, // Recovery on timer expiry
    { SIP_PDU::Failure_Gone                       , OpalConnection::EndedByQ931Cause         ,  22 }, // Number changed (w/o diagnostic)
    { SIP_PDU::Failure_RequestEntityTooLarge      , OpalConnection::EndedByQ931Cause         , 127 }, // Interworking (+)
    { SIP_PDU::Failure_RequestURITooLong          , OpalConnection::EndedByQ931Cause         , 127 }, // Interworking (+)
    { SIP_PDU::Failure_UnsupportedMediaType       , OpalConnection::EndedByCapabilityExchange,  79 }, // Service/option not implemented (+)
    { SIP_PDU::Failure_NotAcceptableHere          , OpalConnection::EndedByCapabilityExchange,  79 }, // Service/option not implemented (+)
    { SIP_PDU::Failure_UnsupportedURIScheme       , OpalConnection::EndedByQ931Cause         , 127 }, // Interworking (+)
    { SIP_PDU::Failure_BadExtension               , OpalConnection::EndedByNoUser            , 127 }, // Interworking (+)
    { SIP_PDU::Failure_ExtensionRequired          , OpalConnection::EndedByNoUser            , 127 }, // Interworking (+)
    { SIP_PDU::Failure_IntervalTooBrief           , OpalConnection::EndedByQ931Cause         , 127 }, // Interworking (+)
    { SIP_PDU::Failure_TemporarilyUnavailable     , OpalConnection::EndedByTemporaryFailure  ,  18 }, // No user responding
    { SIP_PDU::Failure_TransactionDoesNotExist    , OpalConnection::EndedByQ931Cause         ,  41 }, // Temporary Failure
    { SIP_PDU::Failure_LoopDetected               , OpalConnection::EndedByQ931Cause         ,  25 }, // Exchange - routing error
    { SIP_PDU::Failure_TooManyHops                , OpalConnection::EndedByQ931Cause         ,  25 }, // Exchange - routing error
    { SIP_PDU::Failure_AddressIncomplete          , OpalConnection::EndedByQ931Cause         ,  28 }, // Invalid Number Format (+)
    { SIP_PDU::Failure_Ambiguous                  , OpalConnection::EndedByNoUser            ,   1 }, // Unallocated number
    { SIP_PDU::Failure_BusyHere                   , OpalConnection::EndedByRemoteBusy        ,  17 }, // User busy
    { SIP_PDU::Failure_InternalServerError        , OpalConnection::EndedByOutOfService      ,  41 }, // Temporary failure
    { SIP_PDU::Failure_NotImplemented             , OpalConnection::EndedByQ931Cause         ,  79 }, // Not implemented, unspecified
    { SIP_PDU::Failure_BadGateway                 , OpalConnection::EndedByOutOfService      ,  38 }, // Network out of order
    { SIP_PDU::Failure_ServiceUnavailable         , OpalConnection::EndedByOutOfService      ,  41 }, // Temporary failure
    { SIP_PDU::Failure_ServerTimeout              , OpalConnection::EndedByOutOfService      , 102 }, // Recovery on timer expiry
    { SIP_PDU::Failure_SIPVersionNotSupported     , OpalConnection::EndedByQ931Cause         , 127 }, // Interworking (+)
    { SIP_PDU::Failure_MessageTooLarge            , OpalConnection::EndedByQ931Cause         , 127 }, // Interworking (+)
    { SIP_PDU::GlobalFailure_BusyEverywhere       , OpalConnection::EndedByRemoteBusy        ,  17 }, // User busy
    { SIP_PDU::GlobalFailure_Decline              , OpalConnection::EndedByRefusal           ,  21 }, // Call rejected
    { SIP_PDU::GlobalFailure_DoesNotExistAnywhere , OpalConnection::EndedByNoUser            ,   1 }, // Unallocated number
  };

  for (PINDEX i = 0; i < PARRAYSIZE(SIPCodeToReason); i++) {
    if (response.GetStatusCode() == SIPCodeToReason[i].sipCode)
      return OpalConnection::CallEndReason(SIPCodeToReason[i].reasonCode, SIPCodeToReason[i].q931Code);
  }

  // default Q.931 code is 31 Normal, unspecified
  return OpalConnection::CallEndReason(OpalConnection::EndedByQ931Cause, Q931::NormalUnspecified);
}


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
  , deleteTransport(newTransport == NULL || !newTransport->IsReliable())
  , m_holdToRemote(eHoldOff)
  , m_holdFromRemote(false)
  , originalInvite(NULL)
  , originalInviteTime(0)
  , m_sdpSessionId(PTime().GetTimeInSeconds())
  , m_sdpVersion(0)
  , needReINVITE(false)
  , m_appearanceCode(ep.GetDefaultAppearanceCode())
  , authentication(NULL)
  , ackReceived(false)
  , releaseMethod(ReleaseWithNothing)
#if OPAL_HAS_IM
  , rfc4103Context(OpalT140)
#endif

{
  synchronousOnRelease = false;

  SIPURL adjustedDestination = destination;
  m_connStringOptions.ExtractFromURL(adjustedDestination);

  // Look for a "proxy" parameter to override default proxy
  PStringToString params = adjustedDestination.GetParamVars();
  SIPURL proxy;
  if (params.Contains("proxy")) {
    proxy.Parse(params["proxy"]);
    adjustedDestination.SetParamVar("proxy", PString::Empty());
  }

  if (params.Contains("x-line-id")) {
    m_appearanceCode = params["x-line-id"].AsUnsigned();
    adjustedDestination.SetParamVar("x-line-id", PString::Empty());
  }

  if (params.Contains("appearance")) {
    m_appearanceCode = params["appearance"].AsUnsigned();
    adjustedDestination.SetParamVar("appearance", PString::Empty());
  }

  const PStringToString & query = adjustedDestination.GetQueryVars();
  for (PINDEX i = 0; i < query.GetSize(); ++i)
    m_connStringOptions.SetAt(HeaderPrefix+query.GetKeyAt(i),
                          PURL::UntranslateString(query.GetDataAt(i), PURL::QueryTranslation));
  adjustedDestination.SetQuery(PString::Empty());

  m_dialog.SetRequestURI(adjustedDestination);
  m_dialog.SetRemoteURI(adjustedDestination);

  // Update remote party parameters
  UpdateRemoteAddresses();

  if (proxy.IsEmpty())
    proxy = endpoint.GetProxy();

  m_dialog.UpdateRouteSet(proxy); // Default routeSet if there is a proxy
  
  forkedInvitations.DisallowDeleteObjects();

  ackTimer.SetNotifier(PCREATE_NOTIFIER(OnAckTimeout));
  ackRetry.SetNotifier(PCREATE_NOTIFIER(OnInviteResponseRetry));

  sessionTimer.SetNotifier(PCREATE_NOTIFIER(OnSessionTimeout));

#if OPAL_T38_CAPABILITY
  /* We default to having T.38 included as most UAs do not actually
     tell you that they support it or not. For the re-INVITE mechanism
     to work correctly, the rest ofthe system ahs to assume that the
     UA is capable of it, even it it isn't. */
  remoteFormatList += OpalT38;
#endif

  PTRACE(4, "SIP\tCreated connection.");
}


SIPConnection::~SIPConnection()
{
  delete authentication;
  delete originalInvite;

  if (transport != NULL)
    transport->CloseWait();

  if (deleteTransport)
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
  SIPDialogNotification::Events notifyDialogEvent = SIPDialogNotification::NoEvent;
  SIP_PDU::StatusCodes sipCode = SIP_PDU::IllegalStatusCode;

  switch (releaseMethod) {
    case ReleaseWithNothing :
      for (PSafePtr<SIPTransaction> invitation(forkedInvitations, PSafeReference); invitation != NULL; ++invitation) {
        /* If we never even received a "100 Trying" from a remote, then just abort
           the transaction, do not wait, it is probably on an interface that the
           remote is not physically on. */
        if (!invitation->IsCompleted())
          invitation->Abort();
        notifyDialogEvent = SIPDialogNotification::Timeout;
      }
      break;

    case ReleaseWithResponse :
      // Try find best match for return code
      sipCode = GetStatusCodeFromReason(callEndReason);

      // EndedByCallForwarded is a special case because it needs extra paramater
      SendInviteResponse(sipCode, NULL, callEndReason == EndedByCallForwarded ? (const char *)forwardParty : NULL);

      // Wait for ACK from remote before destroying object
      while (!ackReceived)
        PThread::Sleep(100);

      notifyDialogEvent = SIPDialogNotification::Rejected;
      break;

    case ReleaseWithBYE :
      // create BYE now & delete it later to prevent memory access errors
      byeTransaction = new SIPTransaction(*this, *transport, SIP_PDU::Method_BYE);
      for (PSafePtr<SIPTransaction> invitation(forkedInvitations, PSafeReference); invitation != NULL; ++invitation) {
        /* If we never even received a "100 Trying" from a remote, then just abort
           the transaction, do not wait, it is probably on an interface that the
           remote is not physically on. */
        if (!invitation->IsCompleted())
          invitation->Abort();
      }
      break;

    case ReleaseWithCANCEL :
      PTRACE(3, "SIP\tCancelling " << forkedInvitations.GetSize() << " transactions.");
      for (PSafePtr<SIPTransaction> invitation(forkedInvitations, PSafeReference); invitation != NULL; ++invitation) {
        /* If we never even received a "100 Trying" from a remote, then just abort
           the transaction, do not wait, it is probably on an interface that the
           remote is not physically on, otherwise we have to CANCEL and wait. */
        if (invitation->IsTrying())
          invitation->Abort();
        else
          invitation->Cancel();
      }
      notifyDialogEvent = SIPDialogNotification::Cancelled;
  }

  // No termination event set yet, get it from the call end reason
  if (notifyDialogEvent == SIPDialogNotification::NoEvent) {
    switch (GetCallEndReason()) {
      case EndedByRemoteUser :
        notifyDialogEvent = SIPDialogNotification::RemoteBye;
        break;

      case OpalConnection::EndedByCallForwarded :
        notifyDialogEvent = SIPDialogNotification::Replaced;
        break;

      default :
        notifyDialogEvent = SIPDialogNotification::LocalBye;
    }
  }

  NotifyDialogState(SIPDialogNotification::Terminated, notifyDialogEvent, sipCode);

  // Close media
  CloseMediaStreams();

  /* Note we wait for various transactions to complete as the transport they
     rely on may be owned by the connection, and would be deleted once we exit
     from OnRelease() causing a crash in the transaction processing. */

  // Sent a BYE, wait for it to complete
  if (byeTransaction != NULL) {
    PTRACE(4, "SIP\tAwaiting BYE transaction completion, id=" << byeTransaction->GetTransactionID());
    byeTransaction->WaitForCompletion();
    byeTransaction.SetNULL();
  }

  // Wait until all INVITEs have completed
  for (PSafePtr<SIPTransaction> invitation(forkedInvitations, PSafeReference); invitation != NULL; ++invitation) {
    PTRACE(4, "SIP\tAwaiting forked INVITE transaction completion, id=" << invitation->GetTransactionID());
    invitation->WaitForCompletion();
  }
  forkedInvitations.RemoveAll();

  // Sent a REFER, wait for it to complete
  if (referTransaction != NULL) {
    PTRACE(4, "SIP\tAwaiting REFER transaction completion, id=" << referTransaction->GetTransactionID());
    referTransaction->WaitForCompletion();
    referTransaction.SetNULL();
  }

  SetPhase(ReleasedPhase);

  OpalRTPConnection::OnReleased();
}


bool SIPConnection::TransferConnection(const PString & remoteParty)
{
  // There is still an ongoing REFER transaction 
  if (referTransaction != NULL) {
    PTRACE(2, "SIP\tTransfer already in progress for " << *this);
    return false;
  }

  PTRACE(3, "SIP\tTransferring " << *this << " to " << remoteParty);

  SIPURL localPartyURL = endpoint.GetRegisteredPartyName(remoteParty, *transport);
  localPartyURL.Sanitise(SIPURL::RequestURI);

  PSafePtr<OpalCall> call = endpoint.GetManager().FindCallWithLock(remoteParty, PSafeReadOnly);
  if (call == NULL) {
    referTransaction = new SIPRefer(*this, *transport, remoteParty, localPartyURL);
    return referTransaction->Start();
  }

  for (PSafePtr<OpalConnection> connection = call->GetConnection(0); connection != NULL; ++connection) {
    PSafePtr<SIPConnection> sip = PSafePtrCast<OpalConnection, SIPConnection>(connection);
    if (sip != NULL) {
      PStringStream referTo;
      referTo << sip->GetRemotePartyURL()
              << "?Replaces="     << PURL::TranslateString(sip->GetDialog().GetCallID(),    PURL::QueryTranslation)
              << "%3Bto-tag%3D"   << PURL::TranslateString(sip->GetDialog().GetLocalTag(),  PURL::QueryTranslation) // "to/from" is from the other sides perspective
              << "%3Bfrom-tag%3D" << PURL::TranslateString(sip->GetDialog().GetRemoteTag(), PURL::QueryTranslation);
      referTransaction = new SIPRefer(*this, *transport, referTo, endpoint.GetLocalURL(*transport, GetLocalPartyName()));
      referTransaction->GetMIME().SetAt("Refer-Sub", "false"); // Use RFC4488 to indicate we are NOT doing NOTIFYs
      referTransaction->GetMIME().SetAt("Supported", "replaces");
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

  if (GetPhase() >= AlertingPhase) 
    return PFalse;

  if (!withMedia) 
    SendInviteResponse(SIP_PDU::Information_Ringing);
  else {
    SDPSessionDescription sdpOut(m_sdpSessionId, ++m_sdpVersion, GetDefaultSDPConnectAddress());
    if (!OnSendSDP(true, m_rtpSessions, sdpOut)) {
      Release(EndedByCapabilityExchange);
      return PFalse;
    }
    if (!SendInviteResponse(SIP_PDU::Information_Session_Progress, NULL, NULL, &sdpOut))
      return PFalse;
  }

  SetPhase(AlertingPhase);
  NotifyDialogState(SIPDialogNotification::Early);

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

  SDPSessionDescription sdpOut(m_sdpSessionId, ++m_sdpVersion, GetDefaultSDPConnectAddress());
  if (!OnSendSDP(true, m_rtpSessions, sdpOut)) {
    Release(EndedByCapabilityExchange);
    return PFalse;
  }
    
  // send the 200 OK response
  SendInviteOK(sdpOut);

  releaseMethod = ReleaseWithBYE;
  sessionTimer = 10000;

  NotifyDialogState(SIPDialogNotification::Confirmed);

  // switch phase and if media was previously set up, then move to Established
  return OpalConnection::SetConnected();
}


RTP_UDP *SIPConnection::OnUseRTPSession(const unsigned rtpSessionId,
                                        const OpalMediaType & mediaType,
                                        const OpalTransportAddress & mediaAddress,
                                        OpalTransportAddress & localAddress)
{
  RTP_UDP *rtpSession = NULL;
  RTP_DataFrame::PayloadTypes ntePayloadCode = RTP_DataFrame::IllegalPayloadType;

  PSafeLockReadOnly m(ownerCall);

  PSafePtr<OpalConnection> otherParty = GetOtherPartyConnection();
  if (otherParty == NULL) {
    PTRACE(2, "SIP\tCowardly refusing to create an RTP channel with only one connection");
    return NULL;
  }

  // if doing media bypass, we need to set the local address
  // otherwise create an RTP session
  if (ownerCall.IsMediaBypassPossible(*this, rtpSessionId)) {
    PSafePtr<OpalRTPConnection> otherParty = GetOtherPartyConnectionAs<OpalRTPConnection>();
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
    localAddress = GetDefaultSDPConnectAddress(rtpSession->GetLocalDataPort());
  }
  
  return rtpSession;
}


PBoolean SIPConnection::OnSendSDP(bool isAnswerSDP, OpalRTPSessionManager & rtpSessions, SDPSessionDescription & sdpOut)
{
  bool sdpOK = false;

  if (isAnswerSDP)
    needReINVITE = false;

  // get the remote media formats, if any
  SDPSessionDescription * sdp;
  if (isAnswerSDP && originalInvite != NULL && (sdp = originalInvite->GetSDP()) != NULL) {
    const SDPMediaDescriptionArray & mediaDescriptions = sdp->GetMediaDescriptions();
    for (PINDEX i = 0; i < mediaDescriptions.GetSize(); ++i) 
      sdpOK |= AnswerSDPMediaDescription(*sdp, i+1, sdpOut);
  }
  else if (needReINVITE && !mediaStreams.IsEmpty()) {
    std::vector<bool> sessions;
    for (OpalMediaStreamPtr stream(mediaStreams, PSafeReference); stream != NULL; ++stream) {
      std::vector<bool>::size_type session = stream->GetSessionID();
      sessions.resize(std::max(sessions.size(),session+1));
      if (!sessions[session]) {
        sessions[session] = true;
        sdpOK |= OfferSDPMediaDescription(stream->GetMediaFormat().GetMediaType(), session, rtpSessions, sdpOut);
      }
    }
  }

  else {

    // construct offer as per RFC 3261, para 14.2
    // Use |= to avoid McCarthy boolean || from not calling video/fax

    // always offer audio first
    sdpOK  |= OfferSDPMediaDescription(OpalMediaType::Audio(), 0, rtpSessions, sdpOut);

#if OPAL_VIDEO
    // always offer video second (if enabled)
    sdpOK |= OfferSDPMediaDescription(OpalMediaType::Video(), 0, rtpSessions, sdpOut);
#endif

    // offer other formats
    OpalMediaTypeFactory::KeyList_T mediaTypes = OpalMediaType::GetList();
    for (OpalMediaTypeFactory::KeyList_T::iterator r = mediaTypes.begin(); r != mediaTypes.end(); ++r) {
      OpalMediaType mediaType = *r;
      if (mediaType != OpalMediaType::Video() && mediaType != OpalMediaType::Audio())
        sdpOK |= OfferSDPMediaDescription(mediaType, 0, rtpSessions, sdpOut);
    }
  }

  if (isAnswerSDP)
    needReINVITE = true;

  return sdpOK;
}


static void SetNxECapabilities(SDPMediaDescription * localMedia,
                             const OpalMediaFormat & mediaFormat, 
                            const OpalRFC2833Proto * handler,
                       RTP_DataFrame::PayloadTypes   nxePayloadCode = RTP_DataFrame::IllegalPayloadType)
{
  if (mediaFormat.IsEmpty())
    return;

  if (nxePayloadCode != RTP_DataFrame::IllegalPayloadType) {
    PTRACE(3, "SIP\tUsing bypass RTP payload " << nxePayloadCode << " for " << mediaFormat);
  }
  else if ((nxePayloadCode = handler->GetPayloadType()) != RTP_DataFrame::IllegalPayloadType) {
    PTRACE(3, "SIP\tUsing handler RTP payload " << nxePayloadCode << " for " << mediaFormat);
  }
  else if ((nxePayloadCode = mediaFormat.GetPayloadType()) != RTP_DataFrame::IllegalPayloadType) {
    PTRACE(3, "SIP\tUsing default RTP payload " << nxePayloadCode << " for " << mediaFormat);
  }
  else {
    PTRACE(2, "SIP\tCould not allocate dynamic RTP payload for " << mediaFormat);
    return;
  }

  OpalMediaFormat adjustedFormat = mediaFormat;
  adjustedFormat.SetPayloadType(nxePayloadCode);
  adjustedFormat.SetOptionString("FMTP", handler->GetRxCapability());
  localMedia->AddSDPMediaFormat(new SDPMediaFormat(*localMedia, adjustedFormat));
}


static OpalMediaFormat GetNxECapabilities(OpalRFC2833Proto * handler,
                                 const SDPMediaDescription * incomingMedia,
                                     const OpalMediaFormat & mediaFormat)
{
  // Find the payload type and capabilities used for telephone-event, if present
  const SDPMediaFormatList & sdpMediaList = incomingMedia->GetSDPMediaFormats();
  for (SDPMediaFormatList::const_iterator format = sdpMediaList.begin(); format != sdpMediaList.end(); ++format) {
    if (format->GetEncodingName() == mediaFormat.GetEncodingName()) {
      handler->SetPayloadType(format->GetPayloadType());
      handler->SetTxCapability(format->GetFMTP());
      return format->GetMediaFormat();
    }
  }

  return OpalMediaFormat();
}


bool SIPConnection::OfferSDPMediaDescription(const OpalMediaType & mediaType,
                                             unsigned rtpSessionId,
                                             OpalRTPSessionManager & rtpSessions,
                                             SDPSessionDescription & sdp)
{
  OpalMediaType::AutoStartMode autoStart = GetAutoStart(mediaType);
  if (rtpSessionId == 0 && autoStart == OpalMediaType::DontOffer)
    return false;

  OpalMediaFormatList formats = GetLocalMediaFormats();

  // See if any media formats of this session id, so don't create unused RTP session
  if (!formats.HasType(mediaType)) {
    PTRACE(3, "SIP\tNo media formats of type " << mediaType << ", not adding SDP");
    return PFalse;
  }

  PTRACE(3, "SIP\tOffering media type " << mediaType << " in SDP with formats\n" << setfill(',') << formats << setfill(' '));

  if (rtpSessionId == 0)
    rtpSessionId = sdp.GetMediaDescriptions().GetSize()+1;

  OpalTransportAddress localAddress;
  RTP_DataFrame::PayloadTypes ntePayloadCode = RTP_DataFrame::IllegalPayloadType;
  RTP_DataFrame::PayloadTypes nsePayloadCode = RTP_DataFrame::IllegalPayloadType;

  if (ownerCall.IsMediaBypassPossible(*this, rtpSessionId)) {
    PSafePtr<OpalRTPConnection> otherParty = GetOtherPartyConnectionAs<OpalRTPConnection>();
    if (otherParty != NULL) {
      MediaInformation info;
      if (otherParty->GetMediaInformation(rtpSessionId, info)) {
        localAddress = info.data;
        ntePayloadCode = info.rfc2833;
        nsePayloadCode = info.ciscoNSE;
      }
    }
  }

  OpalMediaSession * mediaSession = rtpSessions.GetMediaSession(rtpSessionId);

  OpalTransportAddress sdpContactAddress;

  if (!localAddress.IsEmpty())
    sdpContactAddress = localAddress;
  else {

    /* We are not doing media bypass, so must have some media session
       Due to the possibility of several INVITEs going out, all with different
       transport requirements, we actually need to use an rtpSession dictionary
       for each INVITE and not the one for the connection. Once an INVITE is
       accepted the rtpSessions for that INVITE is put into the connection. */
    // need different handling for RTP and non-RTP sessions
    if (!mediaType.GetDefinition()->UsesRTP()) {
      if (mediaSession == NULL) {
        mediaSession = mediaType.GetDefinition()->CreateMediaSession(*this, rtpSessionId);
        if (mediaSession != NULL)
          rtpSessions.AddMediaSession(mediaSession, mediaType);
      }
      if (mediaSession != NULL)
        sdpContactAddress = mediaSession->GetLocalMediaAddress();
    }

    else {
      RTP_Session * rtpSession = rtpSessions.GetSession(rtpSessionId);
      if (rtpSession == NULL) {

        // Not already there, so create one
        rtpSession = CreateSession(GetTransport(), rtpSessionId, mediaType, NULL);
        if (rtpSession == NULL) {
          PTRACE(1, "SIP\tCould not create RTP session " << rtpSessionId << " for media type " << mediaType << ", released " << *this);
          Release(OpalConnection::EndedByTransportFail);
          return PFalse;
        }

        rtpSession->SetUserData(new SIP_RTP_Session(*this));

        // add the RTP session to the RTP session manager in INVITE
        rtpSessions.AddSession(rtpSession, mediaType);

        mediaSession = rtpSessions.GetMediaSession(rtpSessionId);
        PAssert(mediaSession != NULL, "cannot retrieve newly added RTP session");
      }
      sdpContactAddress = GetDefaultSDPConnectAddress(((RTP_UDP *)rtpSession)->GetLocalDataPort());
    }
  }

  if (sdpContactAddress.IsEmpty()) {
    PTRACE(2, "SIP\tRefusing to add SDP media description for session id " << rtpSessionId << " with no transport address");
    return false;
  }

  if (mediaSession == NULL) {
    PTRACE(1, "SIP\tCould not create media session " << rtpSessionId << " for media type " << mediaType << ", released " << *this);
    Release(OpalConnection::EndedByTransportFail);
    return PFalse;
  }

  SDPMediaDescription * localMedia = mediaSession->CreateSDPMediaDescription(sdpContactAddress);
  if (localMedia == NULL) {
    PTRACE(2, "SIP\tCan't create SDP media description for media type " << mediaType);
    return false;
  }

  if (sdp.GetDefaultConnectAddress().IsEmpty())
    sdp.SetDefaultConnectAddress(sdpContactAddress);

  if (needReINVITE) {
    OpalMediaStreamPtr sendStream = GetMediaStream(rtpSessionId, false);
    bool sending = sendStream != NULL && sendStream->IsOpen();
    OpalMediaStreamPtr recvStream = GetMediaStream(rtpSessionId, true);
    bool recving = recvStream != NULL && recvStream->IsOpen();
    if (sending) {
      localMedia->AddMediaFormat(sendStream->GetMediaFormat());
      localMedia->SetDirection(m_holdToRemote >= eHoldOn ? SDPMediaDescription::SendOnly : (recving ? SDPMediaDescription::SendRecv : SDPMediaDescription::SendOnly));
    }
    else if (recving) {
      localMedia->AddMediaFormat(recvStream->GetMediaFormat());
      localMedia->SetDirection(m_holdToRemote >= eHoldOn ? SDPMediaDescription::Inactive : SDPMediaDescription::RecvOnly);
    }
    else {
      localMedia->AddMediaFormats(formats, mediaType);
      localMedia->SetDirection(SDPMediaDescription::Inactive);
    }
#if PAUSE_WITH_EMPTY_ADDRESS
    if (m_holdToRemote >= eHoldOn) {
      PString addr = localMedia->GetTransportAddress();
      PCaselessString proto = addr.GetProto();
      WORD port; { PIPSocket::Address dummy; localMedia->GetTransportAddress().GetIpAndPort(dummy, port); }
      OpalTransportAddress newAddr = proto + "$0.0.0.0:" + PString(PString::Unsigned, port);
      localMedia->SetTransportAddress(newAddr);
      localMedia->SetDirection(SDPMediaDescription::Undefined);
    }
#endif
  }
  else {
    localMedia->AddMediaFormats(formats, mediaType);
    localMedia->SetDirection((SDPMediaDescription::Direction)autoStart);
  }

  // Set format if we have an RTP payload type for RFC2833 and/or NSE
  // Must be after other codecs, as Mediatrix gateways barf if RFC2833 is first
  if (mediaType == OpalMediaType::Audio()) {
    SetNxECapabilities(localMedia, OpalRFC2833, rfc2833Handler,  ntePayloadCode);
#if OPAL_T38_CAPABILITY
    SetNxECapabilities(localMedia, OpalCiscoNSE, ciscoNSEHandler, nsePayloadCode);
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
  OpalMediaTypeDefinition * mediaDefinition = mediaType.GetDefinition();
  if (mediaDefinition == NULL) {
    PTRACE(1, "SIP\tUnknown media type " << mediaType << " in session " << rtpSessionId);
    return false;
  }

  // Find the payload type and capabilities used for telephone-event, if present
  OpalMediaFormat nteFormat = GetNxECapabilities(rfc2833Handler, incomingMedia, OpalRFC2833);
  remoteFormatList += nteFormat;
#if OPAL_T38_CAPABILITY
  OpalMediaFormat nseFormat = GetNxECapabilities(ciscoNSEHandler, incomingMedia, OpalCiscoNSE);
  remoteFormatList += nseFormat;
#endif

  OpalTransportAddress localAddress;
  OpalTransportAddress mediaAddress = incomingMedia->GetTransportAddress();
  bool remoteChanged = false;

  OpalMediaSession * mediaSession = NULL;

  if (mediaAddress.IsEmpty()) {
    // Hold aka pause media, should only occur on a re-INVITE.
    RTP_UDP * rtpSession = dynamic_cast<RTP_UDP *>(GetSession(rtpSessionId));
    if (rtpSession == NULL)
      return false;
    localAddress = OpalTransportAddress(rtpSession->GetLocalAddress(), rtpSession->GetLocalDataPort());
    mediaSession = m_rtpSessions.GetMediaSession(rtpSessionId);
    remoteChanged = true;
  }
  else {
    PIPSocket::Address ip;
    WORD port = 0;
    if (!mediaAddress.GetIpAndPort(ip, port)) {
      PTRACE(1, "SIP\tCannot get remote ports for RTP session " << rtpSessionId);
      return PFalse;
    }

    if (mediaDefinition->UsesRTP()) {
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
        bool remoteSet = rtpSession->GetRemoteDataPort() != 0 && rtpSession->GetRemoteAddress().IsValid();
        if (remoteSet)
          remoteChanged = (rtpSession->GetRemoteAddress() != ip) || (rtpSession->GetRemoteDataPort() != port);
        if (remoteChanged || !remoteSet) {
          PTRACE_IF(3, remoteChanged, "SIP\tRemote changed IP address: "
                    << rtpSession->GetRemoteAddress() << "!=" << ip
                    << " || " << rtpSession->GetRemoteDataPort() << "!=" << port);
          if (!rtpSession->SetRemoteSocketInfo(ip, port, PTrue)) {
            PTRACE(1, "SIP\tCannot set remote ports on RTP session");
            return false;
          }
        }
      }
      mediaSession = m_rtpSessions.GetMediaSession(rtpSessionId);
    }
    else {
      mediaSession = mediaDefinition->CreateMediaSession(*this, rtpSessionId);
      if (mediaSession == NULL) {
        PTRACE(1, "SIP\tCannot create session for " << mediaType);
        return false;
      }
      m_rtpSessions.AddMediaSession(mediaSession, mediaType);
      localAddress = mediaSession->GetLocalMediaAddress();
    }
  }

  if (!PAssertNULL(mediaSession))
    return false;

  // For fax we have to translate the media type
  mediaSession->mediaType = mediaType;

  SDPMediaDescription * localMedia = NULL;

  OpalMediaFormatList sdpFormats = incomingMedia->GetMediaFormats();
  sdpFormats.Remove(endpoint.GetManager().GetMediaFormatMask());
  if (sdpFormats.GetSize() == 0) {
    PTRACE(1, "SIP\tCould not find media formats in SDP media description for session " << rtpSessionId);
    // Send back a m= line with port value zero and the first entry of the offer payload types as per RFC3264
    localMedia = mediaSession->CreateSDPMediaDescription(OpalTransportAddress());
    if (localMedia  == NULL) {
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

  // construct a new media session list 
  if ((localMedia = mediaSession->CreateSDPMediaDescription(localAddress)) == NULL) {
    PTRACE(1, "SIP\tCould not create SDP media description for media type " << mediaType);
    return false;
  }

  SDPMediaDescription::Direction otherSidesDir = sdpIn.GetDirection(rtpSessionId);
  if (GetPhase() < EstablishedPhase) {
    // If processing initial INVITE and video, obey the auto-start flags
    OpalMediaType::AutoStartMode autoStart = GetAutoStart(mediaType);
    if ((autoStart&OpalMediaType::Transmit) == 0)
      otherSidesDir = (otherSidesDir&SDPMediaDescription::SendOnly) != 0 ? SDPMediaDescription::SendOnly : SDPMediaDescription::Inactive;
    if ((autoStart&OpalMediaType::Receive) == 0)
      otherSidesDir = (otherSidesDir&SDPMediaDescription::RecvOnly) != 0 ? SDPMediaDescription::RecvOnly : SDPMediaDescription::Inactive;
  }

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
        newDirection = newDirection != SDPMediaDescription::Inactive ? SDPMediaDescription::SendRecv : SDPMediaDescription::RecvOnly;
    }
    else {
      recvStream->Close();
      recvStream.SetNULL();
    }
  }

  /* After (possibly) closing streams, we now open them again if necessary,
     OpenSourceMediaStreams will just return true if they are already open.
     We open tx (other party source) side first so we follow the remote
     endpoints preferences. */
  PSafePtr<OpalConnection> otherParty = GetOtherPartyConnection();
  if ((otherSidesDir&SDPMediaDescription::RecvOnly) != 0 &&
       otherParty != NULL &&
       sendStream == NULL &&
       ownerCall.OpenSourceMediaStreams(*otherParty, mediaType, rtpSessionId) &&
       (sendStream = GetMediaStream(rtpSessionId, false)) != NULL)
    newDirection = newDirection != SDPMediaDescription::Inactive ? SDPMediaDescription::SendRecv : SDPMediaDescription::SendOnly;

  if ((otherSidesDir&SDPMediaDescription::SendOnly) != 0 &&
      recvStream == NULL &&
      ownerCall.OpenSourceMediaStreams(*this, mediaType, rtpSessionId) &&
      (recvStream = GetMediaStream(rtpSessionId, true)) != NULL)
    newDirection = newDirection != SDPMediaDescription::Inactive ? SDPMediaDescription::SendRecv : SDPMediaDescription::RecvOnly;

  if (newDirection == SDPMediaDescription::SendRecv) {
    // If we are sendrecv we will receive the same payload type as we transmit.
    OpalMediaFormat adjustedMediaFormat = recvStream->GetMediaFormat();
    adjustedMediaFormat.SetPayloadType(sendStream->GetMediaFormat().GetPayloadType());
    recvStream->UpdateMediaFormat(adjustedMediaFormat);
  }

  // Now we build the reply, setting "direction" as appropriate for what we opened.
  localMedia->SetDirection(newDirection);
  if (sendStream != NULL)
    localMedia->AddMediaFormat(sendStream->GetMediaFormat());
  else if (recvStream != NULL)
    localMedia->AddMediaFormat(recvStream->GetMediaFormat());
  else {
    // Add all possible formats
    bool empty = true;
    OpalMediaFormatList localFormatList = GetLocalMediaFormats();
    for (OpalMediaFormatList::iterator remoteFormat = remoteFormatList.begin(); remoteFormat != remoteFormatList.end(); ++remoteFormat) {
      if (remoteFormat->GetMediaType() == mediaType) {
        for (OpalMediaFormatList::iterator localFormat = localFormatList.begin(); localFormat != localFormatList.end(); ++localFormat) {
          if (localFormat->GetMediaType() == mediaType) {
            OpalMediaFormat intermediateFormat;
            if (OpalTranscoder::FindIntermediateFormat(*localFormat, *remoteFormat, intermediateFormat)) {
              localMedia->AddMediaFormat(*remoteFormat);
              empty = false;
              break;
            }
          }
        }
      }
    }

    // RFC3264 says we MUST have an entry, but it should have port zero
    if (empty) {
      localMedia->AddMediaFormat(sdpFormats.front());
      localMedia->SetTransportAddress(OpalTransportAddress());
    }
    else {
      // We can do the media type but choose not to at this time
      localMedia->SetDirection(SDPMediaDescription::Inactive);
    }
  }

  // Set format if we have an RTP payload type for RFC2833 and/or NSE
  SetNxECapabilities(localMedia, nteFormat, rfc2833Handler);
#if OPAL_T38_CAPABILITY
  SetNxECapabilities(localMedia, nseFormat, ciscoNSEHandler);
#endif

  sdpOut.AddMediaDescription(localMedia);

  return true;
}


OpalTransportAddress SIPConnection::GetDefaultSDPConnectAddress(WORD port) const
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
  return OpalTransportAddress(localIP, port, transport->GetProtoPrefix());
}


OpalMediaFormatList SIPConnection::GetMediaFormats() const
{
  SDPSessionDescription * sdp;
  if (remoteFormatList.IsEmpty() && originalInvite != NULL && (sdp = originalInvite->GetSDP()) != NULL) {
    SIPConnection * writableThis = const_cast<SIPConnection *>(this);
    // Extract the media from SDP into our remote capabilities list
    for (PINDEX sessionID = 1; sessionID <= sdp->GetMediaDescriptions().GetSize(); ++sessionID) {
      SDPMediaDescription * mediaDescription = sdp->GetMediaDescriptionByIndex(sessionID);
      writableThis->remoteFormatList += mediaDescription->GetMediaFormats();
      OpalTransportAddress dummy;
      writableThis->OnUseRTPSession(sessionID, mediaDescription->GetMediaType(), mediaDescription->GetTransportAddress(), dummy);
    }
    writableThis->remoteFormatList.Remove(endpoint.GetManager().GetMediaFormatMask());
  }

  return remoteFormatList;
}


OpalMediaStreamPtr SIPConnection::OpenMediaStream(const OpalMediaFormat & mediaFormat, unsigned sessionID, bool isSource)
{
  // Disable send of re-INVITE if the ancestor OpenMediaStream has to close
  // the media stream, so don't get two re-INVITEs in quick succession.
  bool oldReINVITE = needReINVITE;
  needReINVITE = false;

  // Make sure stream is symmetrical, if codec changed, close and re-open it
  OpalMediaStreamPtr otherStream = GetMediaStream(sessionID, !isSource);
  bool makesymmetrical = otherStream != NULL &&
                          otherStream->IsOpen() &&
                          otherStream->GetMediaFormat() != mediaFormat;
  if (makesymmetrical) {
    // We must make sure reverse stream is closed before opening the
    // new forward one or can really confuse the RTP stack, especially
    // if switching to udptl in fax mode
    if (isSource) {
      OpalMediaPatch * patch = otherStream->GetPatch();
      if (patch != NULL)
        patch->GetSource().Close();
    }
    else
      otherStream->Close();
  }

  OpalMediaStreamPtr oldStream = GetMediaStream(sessionID, isSource);

  // Open forward side
  OpalMediaStreamPtr newStream = OpalRTPConnection::OpenMediaStream(mediaFormat, sessionID, isSource);
  if (newStream == NULL) {
    needReINVITE = oldReINVITE;
    return newStream;
  }

  // Open other direction, if needed (must be after above open)
  if (makesymmetrical &&
      !GetCall().OpenSourceMediaStreams(*(isSource ? GetCall().GetOtherPartyConnection(*this) : this),
                                                  mediaFormat.GetMediaType(), sessionID, mediaFormat)) {
    newStream->Close();
    needReINVITE = oldReINVITE;
    return NULL;
  }

  needReINVITE = oldReINVITE;

  if (needReINVITE && GetPhase() == EstablishedPhase &&
        (newStream != oldStream || GetMediaStream(sessionID, !isSource) != otherStream)) {
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
  return ((SIPConnection *)param)->WriteINVITE(transport);
}


bool SIPConnection::WriteINVITE(OpalTransport & transport)
{
  const SIPURL & requestURI = m_dialog.GetRequestURI();
  SIPURL myAddress = m_connStringOptions(OPAL_OPT_CALLING_PARTY_URL);
  if (myAddress.IsEmpty())
    myAddress = endpoint.GetRegisteredPartyName(requestURI, transport);

  PString transportProtocol = requestURI.GetParamVars()("transport");
  if (!transportProtocol.IsEmpty())
    myAddress.SetParamVar("transport", transportProtocol);

  // only allow override of calling party number if the local party
  // name hasn't been first specified by a register handler. i.e a
  // register handler's target number is always used
  PString number(m_connStringOptions(OPAL_OPT_CALLING_PARTY_NUMBER));
  if (!number.IsEmpty())
    myAddress.SetUserName(number);

  PString name(m_connStringOptions(OPAL_OPT_CALLING_PARTY_NAME));
  if (!name.IsEmpty())
    myAddress.SetDisplayName(name);

  PString domain(m_connStringOptions(OPAL_OPT_CALLING_PARTY_DOMAIN));
  if (!domain.IsEmpty())
    myAddress.SetHostName(domain);

  if (myAddress.GetDisplayName(false).IsEmpty())
    myAddress.SetDisplayName(GetDisplayName());

  myAddress.SetTag(GetToken());
  m_dialog.SetLocalURI(myAddress);

  NotifyDialogState(SIPDialogNotification::Trying);

  needReINVITE = false;
  SIPTransaction * invite = new SIPInvite(*this, transport, OpalRTPSessionManager(*this));

  SIPURL contact = invite->GetMIME().GetContact();
  if (!number.IsEmpty())
    contact.SetDisplayName(name);
  if (!domain.IsEmpty())
    contact.SetHostName(domain);
  invite->GetMIME().SetContact(contact);

  invite->GetMIME().SetAlertInfo(m_alertInfo, m_appearanceCode);

  // It may happen that constructing the INVITE causes the connection
  // to be released (e.g. there are no UDP ports available for the RTP sessions)
  // Since the connection is released immediately, a INVITE must not be
  // sent out.
  if (GetPhase() >= OpalConnection::ReleasingPhase) {
    PTRACE(2, "SIP\tAborting INVITE transaction since connection is in releasing phase");
    delete invite; // Before Start() is called we are responsible for deletion
    return PFalse;
  }

  if (invite->Start()) {
    forkedInvitations.Append(invite);
    return PTrue;
  }

  PTRACE(2, "SIP\tDid not start INVITE transaction on " << transport);
  return PFalse;
}


PBoolean SIPConnection::SetUpConnection()
{
  PTRACE(3, "SIP\tSetUpConnection: " << m_dialog.GetRequestURI());

  SetPhase(SetUpPhase);

  OnApplyStringOptions();

  SIPURL transportAddress;

  if (!m_dialog.GetRouteSet().IsEmpty()) 
    transportAddress = m_dialog.GetRouteSet().front();
  else {
    transportAddress = m_dialog.GetRequestURI();
    transportAddress.AdjustToDNS(); // Do a DNS SRV lookup
    PTRACE(4, "SIP\tConnecting to " << m_dialog.GetRequestURI() << " via " << transportAddress);
  }

  originating = PTrue;

  if (deleteTransport)
    delete transport;
  transport = endpoint.CreateTransport(transportAddress, m_connStringOptions(OPAL_OPT_INTERFACE));
  if (transport == NULL) {
    Release(EndedByUnreachable);
    return PFalse;
  }

  ++m_sdpVersion;

  bool ok;
  if (!transport->GetInterface().IsEmpty())
    ok = WriteINVITE(*transport);
  else {
    PWaitAndSignal mutex(transport->GetWriteMutex());
    ok = transport->WriteConnect(WriteINVITE, this);
  }

  if (ok) {
    releaseMethod = ReleaseWithCANCEL;
    return true;
  }

  PTRACE(1, "SIP\tCould not write to " << transportAddress << " - " << transport->GetErrorText());
  Release(EndedByTransportFail);
  return false;
}


PString SIPConnection::GetDestinationAddress()
{
  return originalInvite != NULL ? originalInvite->GetURI().AsString() : OpalConnection::GetDestinationAddress();
}


PString SIPConnection::GetCalledPartyURL()
{
  if (originalInvite != NULL)
    return originalInvite->GetURI().AsString();

  SIPURL calledParty = m_dialog.GetRemoteURI();
  calledParty.Sanitise(SIPURL::ToURI);
  return calledParty.AsString();
}


PString SIPConnection::GetAlertingType() const
{
  return m_alertInfo;
}


bool SIPConnection::SetAlertingType(const PString & info)
{
  m_alertInfo = info;
  return true;
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


PString SIPConnection::GetPrefixName() const
{
  return m_dialog.GetRequestURI().GetScheme();
}


PString SIPConnection::GetIdentifier() const
{
  return m_dialog.GetCallID();
}


PString SIPConnection::GetRemotePartyURL() const
{
  SIPURL url = m_dialog.GetRemoteURI();
  url.Sanitise(SIPURL::ExternalURI);
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

  bool allFailed = true;
  {
    // The connection stays alive unless all INVITEs have failed
    PSafePtr<SIPTransaction> invitation(forkedInvitations, PSafeReference);
    while (invitation != NULL) {
      if (invitation == &transaction)
        forkedInvitations.Remove(invitation++);
      else {
        if (!invitation->IsFailed())
          allFailed = false;
        ++invitation;
      }
    }
  }

  // All invitations failed, die now, with correct code
  if (allFailed && GetPhase() < ConnectedPhase)
    Release(GetCallEndReasonFromResponse(transaction));
}


void SIPConnection::OnReceivedPDU(SIP_PDU & pdu)
{
  SIP_PDU::Methods method = pdu.GetMethod();

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
    case SIP_PDU::Method_MESSAGE :
      OnReceivedMESSAGE(pdu);
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

  // If we are in a dialog, then m_dialog needs to be updated in the 2xx/1xx
  // response for a target refresh request
  m_dialog.Update(response);
  UpdateRemoteAddresses();

  if (reInvite)
    return;

  if (statusClass == 2) {
    // Have a final response to the INVITE, so cancel all the other invitations sent.
    for (PSafePtr<SIPTransaction> invitation(forkedInvitations, PSafeReference); invitation != NULL; ++invitation) {
      if (invitation != &transaction)
        invitation->Cancel();
    }

    // And end connect mode on the transport
    transport->SetInterface(transaction.GetInterface());
  }

  // Save the sessions etc we are actually using of all the forked INVITES sent
  if (response.GetSDP() != NULL)
    m_rtpSessions = ((SIPInvite &)transaction).GetSessionManager();

  response.GetMIME().GetProductInfo(remoteProductInfo);
}


void SIPConnection::UpdateRemoteAddresses()
{
  SIPURL url = m_dialog.GetRemoteURI();
  url.Sanitise(SIPURL::ExternalURI);

  remotePartyAddress = url.GetHostAddress();

  remotePartyNumber = url.GetUserName();
  if (remotePartyNumber.FindSpan("0123456789*#") != P_MAX_INDEX)
    remotePartyNumber.MakeEmpty();

  remotePartyName = url.GetDisplayName();
  if (remotePartyName.IsEmpty())
    remotePartyName = remotePartyNumber.IsEmpty() ? url.GetUserName() : url.AsString();
}


void SIPConnection::NotifyDialogState(SIPDialogNotification::States state, SIPDialogNotification::Events eventType, unsigned eventCode)
{
  SIPURL url = m_dialog.GetLocalURI();
  url.Sanitise(SIPURL::ExternalURI);

  SIPDialogNotification info(url.AsString());

  info.m_dialogId = m_dialogNotifyId.AsString();
  info.m_callId = m_dialog.GetCallID();

  info.m_local.m_URI = url.AsString();
  info.m_local.m_dialogTag  = m_dialog.GetLocalTag();
  info.m_local.m_identity = url.AsString();
  info.m_local.m_display = url.GetDisplayName();
  info.m_local.m_appearance = m_appearanceCode;

  url = m_dialog.GetRemoteURI();
  url.Sanitise(SIPURL::ExternalURI);

  info.m_remote.m_URI = m_dialog.GetRequestURI().AsString();
  info.m_remote.m_dialogTag = m_dialog.GetRemoteTag();
  info.m_remote.m_identity = url.AsString();
  info.m_remote.m_display = url.GetDisplayName();

  if (!info.m_remote.m_dialogTag.IsEmpty() && state == SIPDialogNotification::Proceeding)
    state = SIPDialogNotification::Early;

  info.m_initiator = IsOriginating();
  info.m_state = state;
  info.m_eventType = eventType;
  info.m_eventCode = eventCode;

  if (GetPhase() == EstablishedPhase)
    info.m_local.m_rendering = info.m_remote.m_rendering = SIPDialogNotification::NotRenderingMedia;

  for (OpalMediaStreamPtr mediaStream(mediaStreams, PSafeReference); mediaStream != NULL; ++mediaStream) {
    if (mediaStream->IsSource())
      info.m_remote.m_rendering = SIPDialogNotification::RenderingMedia;
    else
      info.m_local.m_rendering = SIPDialogNotification::RenderingMedia;
  }

  endpoint.SendNotifyDialogInfo(info);
}


void SIPConnection::OnReceivedResponse(SIPTransaction & transaction, SIP_PDU & response)
{
  // Break out to virtual functions for some special cases.
  switch (response.GetStatusCode()) {
    case SIP_PDU::Information_Ringing :
      if (transaction.GetMethod() == SIP_PDU::Method_INVITE)
        OnReceivedRinging(response);
      return;

    case SIP_PDU::Information_Session_Progress :
      if (transaction.GetMethod() == SIP_PDU::Method_INVITE)
        OnReceivedSessionProgress(response);
      return;

    case SIP_PDU::Failure_UnAuthorised :
    case SIP_PDU::Failure_ProxyAuthenticationRequired :
      if (OnReceivedAuthenticationRequired(transaction, response))
        return;
      break;

    default :
      switch (response.GetStatusCode()/100) {
        case 1 : // Treat all other provisional responses like a Trying.
          OnReceivedTrying(transaction, response);
          return;

        case 2 : // Successful response - there really is only 200 OK
          OnReceivedOK(transaction, response);
          return;

        case 3 : // Redirection response
          if (transaction.GetMethod() == SIP_PDU::Method_INVITE)
            OnReceivedRedirection(response);
          return;
      }
  }

  // We don't always release the connection, eg not till all forked invites have completed
  // This INVITE is from a different "dialog", any errors do not cause a release
  if (transaction.GetMethod() != SIP_PDU::Method_INVITE)
    return;

  PSafeLockReadWrite lock(*this);
  if (!lock.IsLocked())
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
      if (invitation->IsProceeding())
        return;
      // If we have not even got a 1xx from the remote for this forked INVITE,
      // don't keep waiting, cancel it and take the error we got
      if (invitation->IsTrying())
        invitation->Cancel();
    }
  }

  // All other responses are errors, set Q931 code if available
  releaseMethod = ReleaseWithNothing;
  Release(GetCallEndReasonFromResponse(response));
}


void SIPConnection::OnReceivedINVITE(SIP_PDU & request)
{
  PBoolean isReinvite;

  const SIPMIMEInfo & requestMIME = request.GetMIME();
  PString requestFromTag = requestMIME.GetFieldParameter("From", "tag");
  PString requestToTag   = requestMIME.GetFieldParameter("To",   "tag");

  if (IsOriginating()) {
    // Check for in the same dialog
    if (m_dialog.GetRemoteTag() != requestFromTag || m_dialog.GetLocalTag() != requestToTag) {
      /* Weird, got incoming INVITE for a call we originated, however it is not in
         the same dialog. Send back a refusal as this just isn't handled. */
      PTRACE(2, "SIP\tIgnoring INVITE from " << request.GetURI() << " when originated call.");
      request.SendResponse(*transport, SIP_PDU::Failure_LoopDetected);
      return;
    }

    // We originated the call, same dialog, so any INVITE must be a re-INVITE, 
    isReinvite = PTrue;
  }
  else {
    if (originalInvite == NULL) {
      isReinvite = PFalse; // First time incoming call
      PTRACE(4, "SIP\tInitial INVITE from " << request.GetURI());
    }
    else {
      /* If we have same transaction ID, it means it is a retransmission
         of the original INVITE, probably should re-transmit last sent response
         but we just ignore it. Still should work. */
      if (originalInvite->GetTransactionID() == request.GetTransactionID()) {
        PTimeInterval timeSinceInvite = PTime() - originalInviteTime;
        PTRACE(3, "SIP\tIgnoring duplicate INVITE from " << request.GetURI() << " after " << timeSinceInvite);
        return;
      }

      /* Same transaction but different "dialog" determined by the tags in the to
         and from fields probably indicate forking request or request arriving via
         multiple IP paths. This will also include the RFC3261/8.2.2.2 case relating
         to merged requests. */
      if (m_dialog.GetRemoteTag() != requestFromTag || m_dialog.GetLocalTag() != requestToTag) {
        PTRACE(3, "SIP\tIgnoring forked INVITE from " << request.GetURI());
        SIP_PDU response(request, SIP_PDU::Failure_LoopDetected);
        response.GetMIME().SetProductInfo(endpoint.GetUserAgent(), GetProductInfo());
        request.SendResponse(*transport, response);
        return;
      }

      // Different transaction but same dialog, must be a re-INVITE
      isReinvite = PTrue;
    }
  }

  // originalInvite should contain the first received INVITE for
  // this connection
  delete originalInvite;
  originalInvite     = new SIP_PDU(request);
  originalInviteTime = PTime();

  SIPMIMEInfo & mime = originalInvite->GetMIME();

  // update the dialog context
  m_dialog.SetLocalTag(GetToken());
  m_dialog.Update(request);
  UpdateRemoteAddresses();

  // We received a Re-INVITE for a current connection
  if (isReinvite) { 
    OnReceivedReINVITE(request);
    return;
  }

  NotifyDialogState(SIPDialogNotification::Trying);
  mime.GetAlertInfo(m_alertInfo, m_appearanceCode);

  // Fill in all the various connection info, note our to/from is their from/to
  mime.GetProductInfo(remoteProductInfo);

  mime.SetTo(m_dialog.GetLocalURI().AsQuotedString());

  // get the called destination number and name
  m_calledPartyName = request.GetURI().GetUserName();
  if (!m_calledPartyName.IsEmpty() && m_calledPartyName.FindSpan("0123456789*#") == P_MAX_INDEX) {
    m_calledPartyNumber = m_calledPartyName;
    m_calledPartyName = request.GetURI().GetDisplayName(false);
  }

  // get the address that remote end *thinks* it is using from the Contact field
  PIPSocket::Address sigAddr;
  PIPSocket::GetHostAddress(m_dialog.GetRequestURI().GetHostName(), sigAddr);  

  // get the local and peer transport addresses
  PIPSocket::Address peerAddr, localAddr;
  transport->GetRemoteAddress().GetIpAddress(peerAddr);
  transport->GetLocalAddress().GetIpAddress(localAddr);

  // allow the application to determine if RTP NAT is enabled or not
  remoteIsNAT = IsRTPNATEnabled(localAddr, peerAddr, sigAddr, PTrue);

  releaseMethod = ReleaseWithResponse;

  SetPhase(SetUpPhase);

  // Replaces header has already been validated in SIPEndPoint::OnReceivedINVITE
  PSafePtr<SIPConnection> replacedConnection = endpoint.GetSIPConnectionWithLock(mime("Replaces"), PSafeReadOnly);
  if (replacedConnection != NULL) {
    PTRACE(3, "SIP\tConnection " << *replacedConnection << " replaced by " << *this);

    // Do OnRelease for other connection synchronously or there is
    // confusion with media streams still open
    replacedConnection->synchronousOnRelease = true;
    replacedConnection->Release(OpalConnection::EndedByCallForwarded);

    // in some circumstances, the peer OpalConnection needs to see the newly arrived media formats
    // before it knows what what formats can support. 
    GetOtherPartyConnection()->PreviewPeerMediaFormats(GetMediaFormats());

    SetConnected();
    return;
  }

  // indicate the other is to start ringing (but look out for clear calls)
  if (!OnIncomingConnection(0, NULL)) {
    PTRACE(1, "SIP\tOnIncomingConnection failed for INVITE from " << request.GetURI() << " for " << *this);
    Release();
    return;
  }

  PTRACE(3, "SIP\tOnIncomingConnection succeeded for INVITE from " << request.GetURI() << " for " << *this);

  // in some circumstances, the peer OpalConnection needs to see the newly arrived media formats
  // before it knows what what formats can support. 
  GetOtherPartyConnection()->PreviewPeerMediaFormats(GetMediaFormats());

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
    request.SendResponse(*transport, SIP_PDU::Failure_NotAcceptableHere);
    return;
  }

  PTRACE(3, "SIP\tReceived re-INVITE from " << request.GetURI() << " for " << *this);

  remoteFormatList.RemoveAll();
  SDPSessionDescription sdpOut(m_sdpSessionId, ++m_sdpVersion, GetDefaultSDPConnectAddress());

  // get the remote media formats, if any
  SDPSessionDescription * sdpIn = originalInvite->GetSDP();
  if (sdpIn != NULL) {
    // The Re-INVITE can be sent to change the RTP Session parameters,
    // the current codecs, or to put the call on hold
    if (sdpIn->IsHold()) {
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


void SIPConnection::OnReceivedNOTIFY(SIP_PDU & request)
{
  const SIPMIMEInfo & mime = request.GetMIME();

  SIPSubscribe::EventPackage package = mime.GetEvent();
  if (package != "refer") {
    PTRACE(2, "SIP\tNOTIFY in a connection only supported for REFER requests");
    request.SendResponse(*transport, SIP_PDU::Failure_BadEvent);
    return;
  }

  if (referTransaction == NULL) {
    PTRACE(2, "SIP\tNOTIFY for REFER we never sent.");
    request.SendResponse(*transport, SIP_PDU::Failure_TransactionDoesNotExist);
    return;
  }

  if (mime.GetContentType() != "message/sipfrag") {
    PTRACE(2, "SIP\tNOTIFY for REFER has incorrect Content-Type");
    request.SendResponse(*transport, SIP_PDU::Failure_BadRequest);
    return;
  }

  PCaselessString body = request.GetEntityBody();
  unsigned code = body.Mid(body.Find(' ')).AsUnsigned();
  if (body.NumCompare("SIP/") != EqualTo || code < 100) {
    PTRACE(2, "SIP\tNOTIFY for REFER has incorrect body");
    request.SendResponse(*transport, SIP_PDU::Failure_BadRequest);
    return;
  }

  request.SendResponse(*transport, SIP_PDU::Successful_OK);

  if (mime.GetSubscriptionState().Find("terminated") == P_MAX_INDEX)
    return; // The REFER is not over yet, ignore

  referTransaction->WaitForCompletion();
  referTransaction.SetNULL();

  // The REFER is over, see if successful
  if (code >= 300) {
    PTRACE(2, "SIP\tNOTIFY indicated REFER did not proceed, taking call back");
    return;
  }

  // Release the connection
  if (GetPhase() < ReleasingPhase) {
    releaseMethod = ReleaseWithBYE;
    Release(OpalConnection::EndedByCallForwarded);
  }
}


void SIPConnection::OnReceivedREFER(SIP_PDU & request)
{
  PString referTo = request.GetMIME().GetReferTo();
  if (referTo.IsEmpty()) {
    if (!request.SendResponse(*transport, SIP_PDU::Failure_BadRequest, NULL, "Missing refer-to header")) {
      PTRACE(1, "SIP\tCould not send response to REFER 1");
    }
    return;
  }    

  SIPURL to = referTo;
  PString replaces = PURL::UntranslateString(to.GetQueryVars()("Replaces"), PURL::QueryTranslation);
  to.SetQuery(PString::Empty());

  SIP_PDU response(request, SIP_PDU::Successful_Accepted);

  // Comply to RFC4488
  if (request.GetMIME()("Refer-Sub") *= "false") 
    response.GetMIME().SetAt("Refer-Sub", "false");

  // send response before attempting the transfer
  if (!request.SendResponse(*transport, response)) {
    PTRACE(1, "SIP\tCould not send response to REFER 3");
    return;
  }

  if (!endpoint.SetupTransfer(GetToken(), replaces, to.AsString(), NULL)) {
    // send NOTIFY if transfer failed, but only if allowed by RFC4488
    if (!(request.GetMIME()("Refer-Sub") *= "false")) {
      SIPReferNotify * notify = new SIPReferNotify(*this, *transport, SIP_PDU::GlobalFailure_Decline);
      notify->Start();
    }
    return;
  }    

  // send NOTIFY if transfer succeeded, but only if allowed by RFC4488
  // Strictly speaking, we shouldn't send this until the the 200 OK, but.....
  if (!(request.GetMIME()("Refer-Sub") *= "false")) {
    SIPReferNotify * notify = new SIPReferNotify(*this, *transport, SIP_PDU::Successful_OK);
    notify->Start();
  }
}


void SIPConnection::OnReceivedBYE(SIP_PDU & request)
{
  PTRACE(3, "SIP\tBYE received for call " << request.GetMIME().GetCallID());
  request.SendResponse(*transport, SIP_PDU::Successful_OK);
  
  if (GetPhase() >= ReleasingPhase) {
    PTRACE(2, "SIP\tAlready released " << *this);
    return;
  }
  releaseMethod = ReleaseWithNothing;

  m_dialog.Update(request);
  UpdateRemoteAddresses();
  request.GetMIME().GetProductInfo(remoteProductInfo);

  Release(EndedByRemoteUser);
}


void SIPConnection::OnReceivedCANCEL(SIP_PDU & request)
{
  // Currently only handle CANCEL requests for the original INVITE that
  // created this connection, all else ignored

  if (originalInvite == NULL || originalInvite->GetTransactionID() != request.GetTransactionID()) {
    PTRACE(2, "SIP\tUnattached " << request << " received for " << *this);
    request.SendResponse(*transport, SIP_PDU::Failure_TransactionDoesNotExist);
    return;
  }

  PTRACE(3, "SIP\tCancel received for " << *this);

  SIP_PDU response(request, SIP_PDU::Successful_OK);
  response.GetMIME().SetTo(m_dialog.GetLocalURI().AsQuotedString());
  request.SendResponse(*transport, response);
  
  if (!IsOriginating())
    Release(EndedByCallerAbort);
}


void SIPConnection::OnReceivedTrying(SIPTransaction & transaction, SIP_PDU & /*response*/)
{
  if (transaction.GetMethod() != SIP_PDU::Method_INVITE)
    return;

  PSafeLockReadWrite lock(*this);
  if (!lock.IsLocked())
    return;

  PTRACE(3, "SIP\tReceived Trying response");
  NotifyDialogState(SIPDialogNotification::Proceeding);

  if (GetPhase() < ProceedingPhase) {
    SetPhase(ProceedingPhase);
    OnProceeding();
  }
}


void SIPConnection::OnStartTransaction(SIPTransaction & transaction)
{
  endpoint.OnStartTransaction(*this, transaction);
}


void SIPConnection::OnReceivedRinging(SIP_PDU & response)
{
  PSafeLockReadWrite lock(*this);
  if (!lock.IsLocked())
    return;

  PTRACE(3, "SIP\tReceived Ringing response");

  OnReceivedSDP(response);

  response.GetMIME().GetAlertInfo(m_alertInfo, m_appearanceCode);

  if (GetPhase() < AlertingPhase) {
    SetPhase(AlertingPhase);
    OnAlerting();
    NotifyDialogState(SIPDialogNotification::Early);
  }

  PTRACE_IF(4, response.GetSDP() != NULL, "SIP\tStarting receive media to annunciate remote alerting tone");
  StartMediaStreams();
}


void SIPConnection::OnReceivedSessionProgress(SIP_PDU & response)
{
  PSafeLockReadWrite lock(*this);
  if (!lock.IsLocked())
    return;

  PTRACE(3, "SIP\tReceived Session Progress response");

  OnReceivedSDP(response);

  if (GetPhase() < AlertingPhase) {
    SetPhase(AlertingPhase);
    OnAlerting();
    NotifyDialogState(SIPDialogNotification::Early);
  }

  PTRACE(4, "SIP\tStarting receive media to annunciate remote progress tones");
  StartMediaStreams();
}


void SIPConnection::OnReceivedRedirection(SIP_PDU & response)
{
  PSafeLockReadWrite lock(*this);
  if (!lock.IsLocked())
    return;

  PTRACE(4, "SIP\tReceived redirect");
  SIPURL whereTo = response.GetMIME().GetContact();
  endpoint.ForwardConnection(*this, whereTo.AsString());
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

  PSafeLockReadWrite lock(*this);
  if (!lock.IsLocked())
    return false;

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

   // Try to find authentication parameters for the given realm,
  // if not, use the proxy authentication parameters (if any)
  PString realm, password;
  PString username = m_dialog.GetLocalURI().GetUserName();
  if (endpoint.GetAuthentication(newAuth->GetAuthRealm(), realm, username, password)) {
    PTRACE (3, "SIP\tFound auth info for realm " << newAuth->GetAuthRealm());
  }
  else {
    SIPURL proxy = endpoint.GetProxy();
    if (proxy.IsEmpty()) {
      PTRACE (3, "SIP\tNo auth info for realm " << newAuth->GetAuthRealm());
      delete newAuth;
      return false;
    }

    PTRACE (3, "SIP\tNo auth info for realm " << newAuth->GetAuthRealm() << ", using proxy auth");
    username = proxy.GetUserName();
    password = proxy.GetPassword();
  } 

  newAuth->SetUsername(username);
  newAuth->SetPassword(password);

  // check to see if this is a follow-on from the last authentication scheme used
  if (authentication != NULL && *newAuth == *authentication) {
    PTRACE(1, "SIP\tAuthentication already performed using current credentials, not trying again.");
    delete newAuth;
    return false;
  }

  // Restart the transaction with new authentication info
  delete authentication;
  authentication = newAuth;

  transport->SetInterface(transaction.GetInterface());
  SIPTransaction * invite = new SIPInvite(*this, *transport, ((SIPInvite &)transaction).GetSessionManager());

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

  PSafeLockReadWrite lock(*this);
  if (!lock.IsLocked())
    return;

  PTRACE(3, "SIP\tHandling " << response.GetStatusCode() << " response for " << transaction.GetMethod());

  // see if the contact address provided in the response changes the transport type
  OpalTransportAddress newContactAddress = SIPURL(response.GetMIME().GetContact()).GetHostAddress();
  if (!newContactAddress.IsCompatible(transport->GetLocalAddress())) {
    PTRACE(2, "SIP\tINVITE response changed transport for call");

    OpalTransport * newTransport = endpoint.CreateTransport(newContactAddress);
    if (newTransport != NULL) {
      if (deleteTransport)
        delete transport;
      transport = newTransport;
    }
  }

  PTRACE(3, "SIP\tReceived INVITE OK response");
  releaseMethod = ReleaseWithBYE;
  sessionTimer = 10000;

  NotifyDialogState(SIPDialogNotification::Confirmed);

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


void SIPConnection::OnReceivedSDP(SIP_PDU & request)
{
  SDPSessionDescription * sdp = request.GetSDP();
  if (sdp == NULL)
    return;

  needReINVITE = false;

  bool ok = false;
  for (PINDEX i = 0; i < sdp->GetMediaDescriptions().GetSize(); ++i) 
    ok |= OnReceivedSDPMediaDescription(*sdp, i+1);

  remoteFormatList += OpalRFC2833;

  needReINVITE = true;

  if (GetPhase() == EstablishedPhase) // re-INVITE
    StartMediaStreams();
  else if (!ok)
    Release(EndedByCapabilityExchange);
}


bool SIPConnection::OnReceivedSDPMediaDescription(SDPSessionDescription & sdp, unsigned rtpSessionId)
{
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
  PTRACE(4, "SIP\tProcessing received SDP media description for " << mediaType);

  /* Get the media the remote has answered to our offer. Remove the media
     formats we do not support, in case the remote is insane and replied
     with something we did not actually offer. */
  OpalMediaFormatList mediaFormatList = mediaDescription->GetMediaFormats();
  mediaFormatList.Remove(endpoint.GetManager().GetMediaFormatMask());
  if (mediaFormatList.GetSize() == 0) {
    PTRACE(2, "SIP\tCould not find supported media formats in SDP media description for session " << rtpSessionId);
    return false;
  }

  // When receiving an answer SDP, keep the remote SDP media formats order
  remoteFormatList += mediaFormatList;

  // Find the payload type and capabilities used for telephone-event, if present
  remoteFormatList += GetNxECapabilities(rfc2833Handler, mediaDescription, OpalRFC2833);
#if OPAL_T38_CAPABILITY
  remoteFormatList += GetNxECapabilities(ciscoNSEHandler, mediaDescription, OpalCiscoNSE);
#endif

  // create the RTPSession
  bool remoteChanged = false;
  OpalTransportAddress localAddress;
  OpalTransportAddress address = mediaDescription->GetTransportAddress();
  if (!address.IsEmpty()) {
    if (!mediaType.GetDefinition()->UsesRTP()) {
      OpalMediaSession * mediaSession = GetMediaSession(rtpSessionId);
      if (mediaSession == NULL) {
        PTRACE(1, "SIP\tCould not find media session " << rtpSessionId);
        return false;
      }

      // set the remote address 
      mediaSession->SetRemoteMediaAddress(address, mediaDescription->GetMediaFormats());

    } else {
      RTP_UDP *rtpSession = OnUseRTPSession(rtpSessionId, mediaType, address, localAddress);
      if (rtpSession == NULL && !ownerCall.IsMediaBypassPossible(*this, rtpSessionId))
        return false;

      // set the remote address 
      PIPSocket::Address ip;
      WORD port = 0;
      if (!address.GetIpAndPort(ip, port) || port == 0) {
        PTRACE(1, "SIP\tCannot get remote ip/ports for RTP session");
        return false;
      }

      // see if remote socket information has changed
      remoteChanged = (rtpSession->GetRemoteAddress() != ip) || (rtpSession->GetRemoteDataPort() != port);
      if (remoteChanged && !rtpSession->SetRemoteSocketInfo(ip, port, true)) {
        PTRACE(1, "SIP\tCannot set remote ports on RTP session");
        return false;
      }
      PTRACE(4, "SIP\tRemote changed IP address");
    }
  }

  SDPMediaDescription::Direction otherSidesDir = sdp.GetDirection(rtpSessionId);

  // Check if we had a stream and the remote has either changed the codec or
  // changed the direction of the stream
  OpalMediaStreamPtr sendStream = GetMediaStream(rtpSessionId, false);
  if (sendStream != NULL && sendStream->IsOpen()) {
    if (!remoteChanged && mediaFormatList.HasFormat(sendStream->GetMediaFormat()))
      sendStream->SetPaused((otherSidesDir&SDPMediaDescription::RecvOnly) == 0);
    else {
      sendStream->GetPatch()->GetSource().Close(); // Was removed from list so close channel
      sendStream.SetNULL();
    }
  }

  OpalMediaStreamPtr recvStream = GetMediaStream(rtpSessionId, true);
  if (recvStream != NULL && recvStream->IsOpen()) {
    if (!remoteChanged && mediaFormatList.HasFormat(recvStream->GetMediaFormat()))
      recvStream->SetPaused((otherSidesDir&SDPMediaDescription::SendOnly) == 0);
    else {
      recvStream->Close(); // Was removed from list so close channel
      recvStream.SetNULL();
    }
  }

  // Then open the streams if the direction allows
  if (recvStream == NULL && (otherSidesDir&SDPMediaDescription::SendOnly) != 0)
    ownerCall.OpenSourceMediaStreams(*this, mediaType, rtpSessionId);

  if (sendStream == NULL && (otherSidesDir&SDPMediaDescription::RecvOnly) != 0) {
    PSafePtr<OpalConnection> otherParty = GetOtherPartyConnection();
    if (otherParty != NULL)
      ownerCall.OpenSourceMediaStreams(*otherParty, mediaType, rtpSessionId);
  }

  PTRACE_IF(3, otherSidesDir == SDPMediaDescription::Inactive, "SIP\tNo streams opened as " << mediaType << " inactive");
  return true;
}


void SIPConnection::OnCreatingINVITE(SIPInvite & request)
{
  PTRACE(3, "SIP\tCreating INVITE request");

  SIPMIMEInfo & mime = request.GetMIME();
  for (PINDEX i = 0; i < m_connStringOptions.GetSize(); ++i) {
    PCaselessString key = m_connStringOptions.GetKeyAt(i);
    if (key.NumCompare(HeaderPrefix) == EqualTo) {
      PString data = m_connStringOptions.GetDataAt(i);
      if (!data.IsEmpty()) {
        mime.SetAt(key.Mid(sizeof(HeaderPrefix)-1), m_connStringOptions.GetDataAt(i));
        if (key == SIP_HEADER_REPLACES)
          mime.SetRequire("replaces", false);
      }
    }
  }

  if (needReINVITE)
    ++m_sdpVersion;

  SDPSessionDescription * sdp = new SDPSessionDescription(m_sdpSessionId, m_sdpVersion, OpalTransportAddress());
  if (OnSendSDP(false, request.GetSessionManager(), *sdp) && !sdp->GetMediaDescriptions().IsEmpty())
    request.SetSDP(sdp);
  else {
    delete sdp;
    Release(EndedByCapabilityExchange);
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
  // this can be used to prompoe any incoming calls to TCP. Not quite there yet, but it *almost* works
  SIPURL contact;
  bool promoteToTCP = false;    // disable code for now
  if (!promoteToTCP || (transport != NULL && (PString(transport->GetProtoPrefix()) == "$tcp"))) 
    contact = endpoint.GetContactURL(*transport, m_dialog.GetLocalURI());
  else {
    // see if endpoint contains a TCP listener we can use
    OpalTransportAddress newAddr;
    if (!endpoint.FindListenerForProtocol("tcp", newAddr))
      return false;
    contact = SIPURL("", newAddr, 0);
  }

  return SendInviteResponse(SIP_PDU::Successful_OK, (const char *) contact.AsQuotedString(), NULL, &sdp);
}


PBoolean SIPConnection::SendInviteResponse(SIP_PDU::StatusCodes code, const char * contact, const char * extra, const SDPSessionDescription * sdp)
{
  if (originalInvite == NULL)
    return true;

  SIP_PDU response(*originalInvite, code, contact, extra, sdp);
  response.GetMIME().SetProductInfo(endpoint.GetUserAgent(), GetProductInfo());
  response.SetAllow(endpoint.GetAllowedMethods());

  if (sdp != NULL)
    response.GetSDP()->SetSessionName(response.GetMIME().GetUserAgent());

  if (response.GetStatusCode() == SIP_PDU::Information_Ringing)
    response.GetMIME().SetAlertInfo(m_alertInfo, m_appearanceCode);

  if (response.GetStatusCode() >= 200) {
    ackPacket = response;
    ackRetry = endpoint.GetRetryTimeoutMin();
    ackTimer = endpoint.GetAckTimeout();
    ackReceived = false;
  }

  return originalInvite->SendResponse(*transport, response); 
}


void SIPConnection::OnInviteResponseRetry(PTimer &, INT)
{
  PSafeLockReadWrite safeLock(*this);
  if (safeLock.IsLocked() && !ackReceived && originalInvite != NULL) {
    PTRACE(3, "SIP\tACK not received yet, retry sending response.");
    originalInvite->SendResponse(*transport, ackPacket); // Not really a resonse but teh function will work
  }
}


void SIPConnection::OnAckTimeout(PTimer &, INT)
{
  PSafeLockReadWrite safeLock(*this);
  if (safeLock.IsLocked() && !ackReceived) {
    PTRACE(1, "SIP\tFailed to receive ACK!");
    ackRetry.Stop();
    ackReceived = true;
    if (GetPhase() < ReleasingPhase) {
      releaseMethod = ReleaseWithBYE;
      Release(EndedByTemporaryFailure);
    }
  }
}


void SIPConnection::OnRTPStatistics(const RTP_Session & session) const
{
  endpoint.OnRTPStatistics(*this, session);
}


void SIPConnection::OnReceivedINFO(SIP_PDU & request)
{
  SIP_PDU::StatusCodes status = SIP_PDU::Failure_UnsupportedMediaType;
  SIPMIMEInfo & mimeInfo = request.GetMIME();
  PCaselessString contentType = mimeInfo.GetContentType();

  if (contentType == ApplicationDTMFRelayKey) {
    PStringArray lines = request.GetEntityBody().Lines();
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

  else if (contentType == ApplicationDTMFKey) {
    OnUserInputString(request.GetEntityBody().Trim());
    status = SIP_PDU::Successful_OK;
  }

#if OPAL_VIDEO
  else if (contentType == ApplicationMediaControlXMLKey) {
    if (OnMediaControlXML(request))
      return;
    status = SIP_PDU::Failure_UnsupportedMediaType;
  }
#endif

  else 
    status = SIP_PDU::Failure_UnsupportedMediaType;

  request.SendResponse(*transport, status);
}


void SIPConnection::OnReceivedPING(SIP_PDU & request)
{
  PTRACE(3, "SIP\tReceived PING");
  request.SendResponse(*transport, SIP_PDU::Successful_OK);
}


void SIPConnection::OnReceivedMESSAGE(SIP_PDU & pdu)
{
  PTRACE(3, "SIP\tReceived MESSAGE");

  RTP_DataFrameList frames = rfc4103Context.ConvertToFrames(pdu.GetEntityBody());

  for (PINDEX i = 0; i < frames.GetSize(); ++i)
    OnReceiveExternalIM(OpalT140, frames[i]);

  pdu.SendResponse(*transport, SIP_PDU::Successful_OK);
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
  if (m_holdFromRemote || m_holdToRemote >= eHoldOn)
    return false;

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

#if OPAL_HAS_IM

bool SIPConnection::TransmitExternalIM(const OpalMediaFormat & /*format*/, RTP_DataFrame & body)
{
  PTRACE(3, "SIP\tSending MESSAGE within call");
  PSafePtr<SIPTransaction> infoTransaction = new SIPTransaction(*this, *transport, SIP_PDU::Method_MESSAGE);
  SIPMIMEInfo & mimeInfo = infoTransaction->GetMIME();
  mimeInfo.SetContentType("text/plain");
  infoTransaction->GetEntityBody() = PString((const char *)(const BYTE *)body.GetPayloadPtr(), body.GetPayloadSize());

  // cannot wait for completion as this keeps the SIPConnection locked, thus preventing the response from being processed
  //infoTransaction->WaitForCompletion();
  //if (infoTransaction->IsFailed()) { }
  infoTransaction->Start();
  return true;
}

#endif

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
    // cannot wait for completion as this keeps the SIPConnection locked, thus preventing the response from being processed
    //infoTransaction->WaitForCompletion();
    //if (infoTransaction->IsFailed()) { }
    infoTransaction->Start();
#if OPAL_STATISTICS
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

PBoolean SIPConnection::OnMediaControlXML(SIP_PDU & request)
{
  VFUXML vfu;
  if (!vfu.Parse(request.GetEntityBody()) || !vfu.vfu) {
    PTRACE(3, "SIP\tUnable to parse received PictureFastUpdate");
    SIP_PDU response(request, SIP_PDU::Failure_Undecipherable);
    response.GetEntityBody() = 
      "<?xml version=\"1.0\" encoding=\"utf-8\" ?>\n"
      "<media_control>\n"
      "  <general_error>\n"
      "  Unable to parse XML request\n"
      "   </general_error>\n"
      "</media_control>\n";
    request.SendResponse(*transport, response);
  }
  else {
    PTRACE(3, "SIP\tPictureFastUpdate received");
    if (LockReadWrite()) {
      OpalMediaStreamPtr encodingStream = GetMediaStream(OpalMediaType::Video(), false);
      if (encodingStream == NULL){
        PTRACE(3, "SIP\tNo video stream to update");
      } else {
        OpalVideoUpdatePicture updatePictureCommand;
        encodingStream->ExecuteCommand(updatePictureCommand);
        PTRACE(3, "SIP\tI-frame requested in video stream");
      }
      UnlockReadWrite();
    }

    request.SendResponse(*transport, SIP_PDU::Successful_OK);
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

#if OPAL_VIDEO
void SIP_RTP_Session::OnRxIntraFrameRequest(const RTP_Session & session) const
{
  // We got an intra frame request control packet, alert the encoder.
  // We're going to grab the call, find the other connection, then grab the
  // encoding stream
  PSafePtr<OpalConnection> otherConnection = connection.GetOtherPartyConnection();
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

void SIP_RTP_Session::SessionFailing(RTP_Session & session)
{
  ((SIPConnection &)connection).SessionFailing(session);
}

void SIPConnection::OnSessionTimeout(PTimer &, INT)
{
  //SIPTransaction * invite = new SIPInvite(*this, *transport, rtpSessions);  
  //invite->Start();  
  //sessionTimer = 10000;
}

PString SIPConnection::GetLocalPartyURL() const
{
  SIPURL url = m_dialog.GetLocalURI();
  url.Sanitise(SIPURL::ExternalURI);
  return url.AsString();
}


#endif // OPAL_SIP


// End of file ////////////////////////////////////////////////////////////////
