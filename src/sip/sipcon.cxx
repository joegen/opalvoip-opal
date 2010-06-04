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
    { OpalConnection::EndedByInvalidConferenceID,SIP_PDU::Failure_TransactionDoesNotExist}
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
  , m_needReINVITE(false)
  , m_handlingINVITE(false)
  , m_symmetricOpenStream(false)
  , m_appearanceCode(ep.GetDefaultAppearanceCode())
  , m_authentication(NULL)
  , m_authenticatedCseq(0)
  , ackReceived(false)
  , m_referInProgress(false)
#if OPAL_FAX
  , m_switchedToFaxMode(false)
#endif
  , releaseMethod(ReleaseWithNothing)
{
  synchronousOnRelease = false;

  SIPURL adjustedDestination = destination;

  // Look for a "proxy" parameter to override default proxy
  PStringToString params = adjustedDestination.GetParamVars();
  SIPURL proxy;
  if (params.Contains(OPAL_PROXY_PARAM)) {
    proxy.Parse(params[OPAL_PROXY_PARAM]);
    adjustedDestination.SetParamVar(OPAL_PROXY_PARAM, PString::Empty());
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
    m_connStringOptions.SetAt(HeaderPrefix+query.GetKeyAt(i), query.GetDataAt(i));
  adjustedDestination.SetQuery(PString::Empty());

  m_connStringOptions.ExtractFromURL(adjustedDestination);

  m_dialog.SetRequestURI(adjustedDestination);
  m_dialog.SetRemoteURI(adjustedDestination);

  // Update remote party parameters
  UpdateRemoteAddresses();

  if (proxy.IsEmpty())
    proxy = endpoint.GetProxy();

  if (proxy.IsEmpty())
    proxy = endpoint.GetRegisteredProxy(adjustedDestination);

  m_dialog.SetProxy(proxy, false); // No default routeSet if there is a proxy for INVITE

  forkedInvitations.DisallowDeleteObjects();
  pendingInvitations.DisallowDeleteObjects();
  m_pendingTransactions.DisallowDeleteObjects();

  ackTimer.SetNotifier(PCREATE_NOTIFIER(OnAckTimeout));
  ackRetry.SetNotifier(PCREATE_NOTIFIER(OnInviteResponseRetry));

  sessionTimer.SetNotifier(PCREATE_NOTIFIER(OnSessionTimeout));

  PTRACE(4, "SIP\tCreated connection.");
}


SIPConnection::~SIPConnection()
{
  delete m_authentication;
  delete originalInvite;

  PTRACE(4, "SIP\tDeleted connection.");
}


bool SIPConnection::SetTransport(const SIPURL & destination)
{
  if (deleteTransport && transport != NULL) {
    transport->CloseWait();
    delete transport;
  }

  if (destination.IsEmpty())
    transport = NULL;
  else
    transport = endpoint.CreateTransport(destination, m_connStringOptions(OPAL_OPT_INTERFACE));
  deleteTransport = true;

  if (transport != NULL)
    return true;

  if (GetPhase() < ReleasingPhase)
    Release(EndedByUnreachable);

  return false;
}


void SIPConnection::OnReleased()
{
  PTRACE(3, "SIP\tOnReleased: " << *this << ", phase = " << GetPhase());
  
  // OpalConnection::Release sets the phase to Releasing in the SIP Handler 
  // thread
  if (GetPhase() >= ReleasedPhase) {
    PTRACE(2, "SIP\tOnReleased: already released");
    return;
  };

  SetPhase(ReleasingPhase);

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

      /* Wait for ACK from remote before destroying object. Note that we either
         get the ACK, or OnAckTimeout() fires and sets this flag anyway. We are
         outside of a connection mutex lock at this point, so no deadlock
         should occur. Really should be a PSyncPoint. */
      while (!ackReceived)
        PThread::Sleep(100);

      notifyDialogEvent = SIPDialogNotification::Rejected;
      break;

    case ReleaseWithBYE :
      // create BYE now & delete it later to prevent memory access errors
      (new SIPBye(*this))->Start();
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

  // Abort the queued up re-INVITEs we never got a chance to send.
  for (PSafePtr<SIPTransaction> invitation(pendingInvitations, PSafeReference); invitation != NULL; ++invitation)
    invitation->Abort();

  /* Note we wait for various transactions to complete as the transport they
     rely on may be owned by the connection, and would be deleted once we exit
     from OnRelease() causing a crash in the transaction processing. */
  PSafePtr<SIPTransaction> transaction;
  while ((transaction = m_pendingTransactions.GetAt(0, PSafeReference)) != NULL) {
    PTRACE(4, "SIP\tAwaiting transaction completion, id=" << transaction->GetTransactionID());
    transaction->WaitForTermination();
    m_pendingTransactions.Remove(transaction);
  }

  // Remove all the references to the transactions so garbage can be collected
  pendingInvitations.RemoveAll();
  forkedInvitations.RemoveAll();

  SetPhase(ReleasedPhase);

  OpalRTPConnection::OnReleased();

  SetTransport(PString::Empty());
}


bool SIPConnection::TransferConnection(const PString & remoteParty)
{
  // There is still an ongoing REFER transaction 
  if (m_referInProgress) {
    PTRACE(2, "SIP\tTransfer already in progress for " << *this);
    return false;
  }

  PTRACE(3, "SIP\tTransferring " << *this << " to " << remoteParty);

  PURL url(remoteParty, "sip");
  StringOptions extra;
  extra.ExtractFromURL(url);

  // Tell the REFER processing UA if it should suppress NOTIFYs about the REFER processing.
  // If we want to get NOTIFYs we have to clear the old connection on the progress message
  // where the connection is transfered. See OnTransferNotify().
  const char * referSub = extra.GetBoolean(OPAL_OPT_REFER_SUB,
            m_connStringOptions.GetBoolean(OPAL_OPT_REFER_SUB, true)) ? "true" : "false";

  PSafePtr<OpalCall> call = endpoint.GetManager().FindCallWithLock(url.GetHostName(), PSafeReadOnly);
  if (call == NULL) {
    SIPRefer * referTransaction = new SIPRefer(*this, remoteParty, m_dialog.GetLocalURI());
    referTransaction->GetMIME().SetAt("Refer-Sub", referSub); // Use RFC4488 to indicate we are doing NOTIFYs or not
    m_referInProgress = referTransaction->Start();
    return m_referInProgress;
  }

  if (call == &ownerCall) {
    PTRACE(2, "SIP\tCannot transfer connection to itself: " << *this);
    return false;
  }

  for (PSafePtr<OpalConnection> connection = call->GetConnection(0); connection != NULL; ++connection) {
    PSafePtr<SIPConnection> sip = PSafePtrCast<OpalConnection, SIPConnection>(connection);
    if (sip != NULL) {
      PStringStream referTo;
      referTo << sip->GetRemotePartyURL()
              << "?Replaces="     << PURL::TranslateString(sip->GetDialog().GetCallID(),    PURL::QueryTranslation)
              << "%3Bto-tag%3D"   << PURL::TranslateString(sip->GetDialog().GetRemoteTag(), PURL::QueryTranslation)
              << "%3Bfrom-tag%3D" << PURL::TranslateString(sip->GetDialog().GetLocalTag(),  PURL::QueryTranslation);
      SIPRefer * referTransaction = new SIPRefer(*this, referTo, m_dialog.GetLocalURI());
      referTransaction->GetMIME().SetAt("Refer-Sub", referSub); // Use RFC4488 to indicate we doing NOTIFYs or NOT
      referTransaction->GetMIME().SetAt("Supported", "replaces");
      m_referInProgress = referTransaction->Start();
      return m_referInProgress;
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
    if (!OnSendAnswerSDP(m_rtpSessions, sdpOut, false)) {
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
  if (!OnSendAnswerSDP(m_rtpSessions, sdpOut, false)) {
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


OpalMediaSession * SIPConnection::SetUpMediaSession(const unsigned rtpSessionId,
                                                    const OpalMediaType & mediaType,
                                                    SDPMediaDescription * mediaDescription,
                                                    OpalTransportAddress & localAddress,
                                                    bool & remoteChanged)
{
  if (mediaDescription->GetPort() == 0) {
    PTRACE(2, "SIP\tReceived disabled/missing media description for " << mediaType);
    return false;
  }

  OpalTransportAddress remoteMediaAddress = mediaDescription->GetTransportAddress();
  if (remoteMediaAddress.IsEmpty()) {
    PTRACE(2, "SIP\tReceived media description with no address for " << mediaType);
    return false;
  }

  if (ownerCall.IsMediaBypassPossible(*this, rtpSessionId)) {
    PSafePtr<OpalRTPConnection> otherParty = GetOtherPartyConnectionAs<OpalRTPConnection>();
    if (otherParty != NULL) {
      MediaInformation gatewayInfo;
      if (otherParty->GetMediaInformation(rtpSessionId, gatewayInfo)) {
        PTRACE(1, "SIP\tMedia bypass unimplemented for media type " << mediaType << " in session " << rtpSessionId);
        return NULL;
      }
    }
    else {
      PTRACE(2, "SIP\tCowardly refusing to media bypass with only one RTP connection");
    }
  }

  OpalMediaTypeDefinition * mediaDefinition = mediaType.GetDefinition();
  if (mediaDefinition == NULL) {
    PTRACE(1, "SIP\tUnknown media type " << mediaType << " in session " << rtpSessionId);
    return NULL;
  }

  if (!mediaDefinition->UsesRTP()) {
    OpalMediaSession * mediaSession = GetMediaSession(rtpSessionId);
    if (mediaSession == NULL) {
      mediaSession = mediaDefinition->CreateMediaSession(*this, rtpSessionId);
      if (mediaSession == NULL) {
        PTRACE(1, "SIP\tMedia definition cannot create session for " << mediaType);
        return NULL;
      }
      m_rtpSessions.AddMediaSession(mediaSession, mediaType);
    }

    mediaSession->SetRemoteMediaAddress(remoteMediaAddress, mediaDescription->GetMediaFormats());
    localAddress = mediaSession->GetLocalMediaAddress();
    return mediaSession;
  }

  // Create the RTPSession if required
  RTP_UDP *rtpSession = dynamic_cast<RTP_UDP *>(UseSession(GetTransport(), rtpSessionId, mediaType));
  if (rtpSession == NULL) {
    PTRACE(1, "SIP\tCannot create RTP session on non-bypassed connection");
    return NULL;
  }

  // Set user data
  rtpSession->SetUserData(new SIP_RTP_Session(*this));

  // Local Address of the session
  localAddress = GetDefaultSDPConnectAddress(rtpSession->GetLocalDataPort());

  if (!remoteMediaAddress.IsEmpty()) {
    PIPSocket::Address ip;
    WORD port = 0;
    if (!remoteMediaAddress.GetIpAndPort(ip, port)) {
      PTRACE(1, "SIP\tCannot get remote address/port for RTP session " << rtpSessionId);
      return NULL;
    }

    // see if remote socket information has changed
    bool remoteSet = rtpSession->GetRemoteDataPort() != 0 && rtpSession->GetRemoteAddress().IsValid();
    if (remoteSet)
      remoteChanged = (rtpSession->GetRemoteAddress() != ip) || (rtpSession->GetRemoteDataPort() != port);
    if (remoteChanged || !remoteSet) {
      PTRACE_IF(3, remoteChanged, "SIP\tRemote changed IP address: "
                << rtpSession->GetRemoteAddress() << "!=" << ip
                << " || " << rtpSession->GetRemoteDataPort() << "!=" << port);
      if (!rtpSession->SetRemoteSocketInfo(ip, port, true)) {
        PTRACE(1, "SIP\tCannot set remote ports on RTP session");
        return NULL;
      }
    }
  }

  return m_rtpSessions.GetMediaSession(rtpSessionId);
}


static void SetNxECapabilities(OpalRFC2833Proto * handler,
                      const OpalMediaFormatList & localMediaFormats,
                      const OpalMediaFormatList & remoteMediaFormats,
                          const OpalMediaFormat & baseMediaFormat,
                            SDPMediaDescription * localMedia = NULL,
                    RTP_DataFrame::PayloadTypes   nxePayloadCode = RTP_DataFrame::IllegalPayloadType)
{
  OpalMediaFormatList::const_iterator fmt = localMediaFormats.FindFormat(baseMediaFormat);
  OpalMediaFormatList::const_iterator remFmt = remoteMediaFormats.FindFormat(baseMediaFormat);

  if (fmt == localMediaFormats.end() || remFmt == remoteMediaFormats.end()) {
    // Not in our local list, disable transmitter
    handler->SetTxCapability("-", false);
    return;
  }

  handler->SetTxCapability(remFmt->GetOptionString("FMTP", "0-15"), true);

  if (nxePayloadCode != RTP_DataFrame::IllegalPayloadType) {
    PTRACE(3, "SIP\tUsing bypass RTP payload " << nxePayloadCode << " for " << *fmt);
  }
  else if ((nxePayloadCode = fmt->GetPayloadType()) != RTP_DataFrame::IllegalPayloadType) {
    PTRACE(3, "SIP\tUsing default RTP payload " << nxePayloadCode << " for " << *fmt);

  }
  else if ((nxePayloadCode = handler->GetPayloadType()) != RTP_DataFrame::IllegalPayloadType) {
    PTRACE(3, "SIP\tUsing handler RTP payload " << nxePayloadCode << " for " << *fmt);
  }
  else {
    PTRACE(2, "SIP\tCould not allocate dynamic RTP payload for " << *fmt);
    return;
  }

  handler->SetPayloadType(nxePayloadCode);

  if (localMedia != NULL) {
    OpalMediaFormat adjustedFormat = *fmt;
    adjustedFormat.SetPayloadType(nxePayloadCode);
    localMedia->AddSDPMediaFormat(new SDPMediaFormat(*localMedia, adjustedFormat));
  }
}


PBoolean SIPConnection::OnSendOfferSDP(OpalRTPSessionManager & rtpSessions, SDPSessionDescription & sdpOut)
{
  bool sdpOK = false;

  if (m_needReINVITE && !mediaStreams.IsEmpty()) {
    std::vector<bool> sessions;
    for (OpalMediaStreamPtr stream(mediaStreams, PSafeReference); stream != NULL; ++stream) {
      std::vector<bool>::size_type session = stream->GetSessionID();
      sessions.resize(std::max(sessions.size(),session+1));
      if (!sessions[session]) {
        sessions[session] = true;
        if (OnSendOfferSDPSession(stream->GetMediaFormat().GetMediaType(), session, rtpSessions, sdpOut))
          sdpOK = true;
      }
    }
  }
  else {
    // always offer audio first
    sdpOK = OnSendOfferSDPSession(OpalMediaType::Audio(), 0, rtpSessions, sdpOut);

#if OPAL_VIDEO
    // always offer video second (if enabled)
    if (OnSendOfferSDPSession(OpalMediaType::Video(), 0, rtpSessions, sdpOut))
      sdpOK = true;
#endif

    // offer other formats
    OpalMediaTypeFactory::KeyList_T mediaTypes = OpalMediaType::GetList();
    for (OpalMediaTypeFactory::KeyList_T::iterator iter = mediaTypes.begin(); iter != mediaTypes.end(); ++iter) {
      OpalMediaType mediaType = *iter;
      if (mediaType != OpalMediaType::Video() && mediaType != OpalMediaType::Audio()) {
        if (OnSendOfferSDPSession(mediaType, 0, rtpSessions, sdpOut))
          sdpOK = true;
      }
    }
  }

  return sdpOK && !sdpOut.GetMediaDescriptions().IsEmpty();
}


bool SIPConnection::OnSendOfferSDPSession(const OpalMediaType & mediaType,
                                                     unsigned   rtpSessionId,
                                        OpalRTPSessionManager & rtpSessions,
                                        SDPSessionDescription & sdp)
{
  OpalMediaType::AutoStartMode autoStart = GetAutoStart(mediaType);
  if (rtpSessionId == 0 && autoStart == OpalMediaType::DontOffer)
    return false;

  OpalMediaFormatList localFormatList = GetLocalMediaFormats();

  // See if any media formats of this session id, so don't create unused RTP session
  if (!localFormatList.HasType(mediaType)) {
    PTRACE(3, "SIP\tNo media formats of type " << mediaType << ", not adding SDP");
    return PFalse;
  }

  PTRACE(3, "SIP\tOffering media type " << mediaType << " in SDP with formats\n" << setfill(',') << localFormatList << setfill(' '));

  if (rtpSessionId == 0)
    rtpSessionId = sdp.GetMediaDescriptions().GetSize()+1;

  MediaInformation gatewayInfo;
  if (ownerCall.IsMediaBypassPossible(*this, rtpSessionId)) {
    PSafePtr<OpalRTPConnection> otherParty = GetOtherPartyConnectionAs<OpalRTPConnection>();
    if (otherParty != NULL)
      otherParty->GetMediaInformation(rtpSessionId, gatewayInfo);
  }

  OpalMediaSession * mediaSession = rtpSessions.GetMediaSession(rtpSessionId);

  OpalTransportAddress sdpContactAddress;

  if (!gatewayInfo.data.IsEmpty())
    sdpContactAddress = gatewayInfo.data;
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

  if (m_needReINVITE) {
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
      localMedia->AddMediaFormats(localFormatList, mediaType);
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
    localMedia->AddMediaFormats(localFormatList, mediaType);
    localMedia->SetDirection((SDPMediaDescription::Direction)autoStart);
  }

  if (mediaType == OpalMediaType::Audio()) {
    SDPAudioMediaDescription * audioMedia = dynamic_cast<SDPAudioMediaDescription *>(localMedia);
    if (audioMedia != NULL)
      audioMedia->SetOfferPTime(m_connStringOptions.GetBoolean(OPAL_OPT_OFFER_SDP_PTIME));

    // Set format if we have an RTP payload type for RFC2833 and/or NSE
    // Must be after other codecs, as Mediatrix gateways barf if RFC2833 is first
    SetNxECapabilities(rfc2833Handler, localFormatList, m_remoteFormatList, OpalRFC2833, localMedia, gatewayInfo.rfc2833);
#if OPAL_T38_CAPABILITY
    SetNxECapabilities(ciscoNSEHandler, localFormatList, m_remoteFormatList, OpalCiscoNSE, localMedia, gatewayInfo.ciscoNSE);
#endif
  }

  sdp.AddMediaDescription(localMedia);

  return true;
}


static bool PauseOrCloseMediaStream(OpalMediaStreamPtr & stream,
                                    const OpalMediaFormatList & sdpFormats,
                                    bool remoteChanged,
                                    bool paused)
{
  if (stream == NULL)
    return false;

  if (!stream->IsOpen())
    return false;

  if (!remoteChanged) {
    OpalMediaFormatList::const_iterator fmt = sdpFormats.FindFormat(stream->GetMediaFormat());
    if (fmt != sdpFormats.end() && stream->UpdateMediaFormat(*fmt)) {
      PTRACE(4, "SIP\tINVITE change needs to " << (paused ? "pause" : "resume") << " stream " << *stream);
      stream->SetPaused(paused);
      return !paused;
    }
  }

  PTRACE(4, "SIP\tINVITE change needs to close stream " << *stream);
  stream->GetPatch()->GetSource().Close();
  stream.SetNULL();
  return false;
}


bool SIPConnection::OnSendAnswerSDP(OpalRTPSessionManager & rtpSessions, SDPSessionDescription & sdpOut, bool reInvite)
{
  if (!PAssert(originalInvite != NULL, PLogicError))
    return false;

  SDPSessionDescription * sdp = originalInvite->GetSDP();

  /* If we had SDP but no media could not be decoded from it, then we should return
     Not Acceptable Here error and not do an offer. Only offer if there was no body
     at all or there was a valid SDP with no m lines. */
  if (sdp == NULL && !originalInvite->GetEntityBody().IsEmpty())
    return false;

  if (sdp == NULL || sdp->GetMediaDescriptions().IsEmpty()) {
    if (m_holdFromRemote) {
      PTRACE(3, "SIP\tRemote retrieve from hold without SDP detected");
      m_holdFromRemote = false;
      OnHold(true, false);
    }

    // They did not offer anything, so it behooves us to do so: RFC 3261, para 14.2
    PTRACE(3, "SIP\tRemote did not offer media, so we will.");
    return OnSendOfferSDP(rtpSessions, sdpOut);
  }

  bool sdpOK = false;

  // get the remote media formats, if any
  unsigned sessionCount = sdp->GetMediaDescriptions().GetSize();
  m_answerFormatList = sdp->GetMediaFormats();
  m_answerFormatList.Remove(endpoint.GetManager().GetMediaFormatMask());
  if (m_answerFormatList.IsEmpty()) {
    PTRACE(3, "SIP\tAll media formats offered by remote have been removed.");
    return false;
  }

  for (unsigned session = 1; session <= sessionCount; ++session) {
    if (OnSendAnswerSDPSession(*sdp, session, sdpOut))
      sdpOK = true;
    else if (!reInvite) {
      OpalMediaStreamPtr stream;
      if ((stream = GetMediaStream(session, false)) != NULL)
        stream->Close();
      if ((stream = GetMediaStream(session, true)) != NULL)
        stream->Close();

      SDPMediaDescription * incomingMedia = sdp->GetMediaDescriptionByIndex(session);
      if (PAssert(incomingMedia != NULL, "SDP Media description list changed")) {
        SDPMediaDescription * outgoingMedia = incomingMedia->CreateEmpty();
        if (PAssert(outgoingMedia != NULL, "SDP Media description clone failed")) {
          if (!incomingMedia->GetSDPMediaFormats().IsEmpty())
            outgoingMedia->AddSDPMediaFormat(new SDPMediaFormat(incomingMedia->GetSDPMediaFormats().front()));
          sdpOut.AddMediaDescription(outgoingMedia);
        }
      }
    }
  }

  /* Shut down any media that is in a session not mentioned in a re-INVITE.
     While the SIP/SDP specification says this shouldn't happen, it does
     anyway so we need to deal. */
  for (OpalMediaStreamPtr stream(mediaStreams, PSafeReference); stream != NULL; ++stream) {
    if (stream->GetSessionID() > sessionCount)
      stream->Close();
  }

  if (!sdpOK)
    return false;

  // The Re-INVITE can be sent to change the RTP Session parameters,
  // the current codecs, or to put the call on hold
  if (sdp->IsHold()) {
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

  return true;
}


bool SIPConnection::OnSendAnswerSDPSession(const SDPSessionDescription & sdpIn,
                                                              unsigned   rtpSessionId,
                                                 SDPSessionDescription & sdpOut)
{
  SDPMediaDescription * incomingMedia = sdpIn.GetMediaDescriptionByIndex(rtpSessionId);
  if (!PAssert(incomingMedia != NULL, "SDP Media description list changed"))
    return false;

  OpalMediaType mediaType = incomingMedia->GetMediaType();

  OpalMediaFormatList localFormatList = GetLocalMediaFormats();
  // See if any media formats of this session id, so don't create unused RTP session
  if (!localFormatList.HasType(mediaType)) {
    PTRACE(3, "SIP\tNo media formats of type " << mediaType << ", not adding SDP");
    return false;
  }

  OpalTransportAddress localAddress;
  bool remoteChanged = false;
  OpalMediaSession * mediaSession = SetUpMediaSession(rtpSessionId, mediaType, incomingMedia, localAddress, remoteChanged);
  if (mediaSession == NULL)
    return false;

  // For fax we have to translate the media type
  mediaSession->mediaType = mediaType;

  SDPMediaDescription * localMedia = NULL;

  OpalMediaFormatList sdpFormats = incomingMedia->GetMediaFormats();
  sdpFormats.Remove(endpoint.GetManager().GetMediaFormatMask());
  while (!sdpFormats.IsEmpty() && localFormatList.FindFormat(sdpFormats.front()) == localFormatList.end())
    sdpFormats.RemoveAt(0);

  if (sdpFormats.IsEmpty()) {
    PTRACE(1, "SIP\tNo available media formats in SDP media description for session " << rtpSessionId);
    // Send back a m= line with port value zero and the first entry of the offer payload types as per RFC3264
    localMedia = mediaSession->CreateSDPMediaDescription(OpalTransportAddress());
    if (localMedia  == NULL) {
      PTRACE(1, "SIP\tCould not create SDP media description for media type " << mediaType);
      return false;
    }
    if (!incomingMedia->GetSDPMediaFormats().IsEmpty())
      localMedia->AddSDPMediaFormat(new SDPMediaFormat(incomingMedia->GetSDPMediaFormats().front()));
    sdpOut.AddMediaDescription(localMedia);
    return false;
  }
  
  // construct a new media session list 
  if ((localMedia = mediaSession->CreateSDPMediaDescription(localAddress)) == NULL) {
    PTRACE(1, "SIP\tCould not create SDP media description for media type " << mediaType);
    return false;
  }

  PTRACE(4, "SIP\tAnswering offer for media type " << mediaType);

  SDPMediaDescription::Direction otherSidesDir = sdpIn.GetDirection(rtpSessionId);
  if (GetPhase() < ConnectedPhase) {
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
  if (PauseOrCloseMediaStream(sendStream, sdpFormats, remoteChanged, (otherSidesDir&SDPMediaDescription::RecvOnly) == 0))
    newDirection = SDPMediaDescription::SendOnly;

  OpalMediaStreamPtr recvStream = GetMediaStream(rtpSessionId, true);
  if (PauseOrCloseMediaStream(recvStream, sdpFormats, remoteChanged, (otherSidesDir&SDPMediaDescription::SendOnly) == 0))
    newDirection = newDirection != SDPMediaDescription::Inactive ? SDPMediaDescription::SendRecv : SDPMediaDescription::RecvOnly;

  /* After (possibly) closing streams, we now open them again if necessary,
     OpenSourceMediaStreams will just return true if they are already open.
     We open tx (other party source) side first so we follow the remote
     endpoints preferences. */
  PSafePtr<OpalConnection> otherParty = GetOtherPartyConnection();
  if (otherParty != NULL && sendStream == NULL) {
    PTRACE(5, "SIP\tOpening tx " << mediaType << " stream from SDP");
    if (ownerCall.OpenSourceMediaStreams(*otherParty, mediaType, rtpSessionId)) {
      sendStream = GetMediaStream(rtpSessionId, false);

      if (sendStream != NULL && (otherSidesDir&SDPMediaDescription::RecvOnly) != 0)
        newDirection = newDirection != SDPMediaDescription::Inactive ? SDPMediaDescription::SendRecv
                                                                     : SDPMediaDescription::SendOnly;
    }
  }

  if (sendStream != NULL) {
    sendStream->UpdateMediaFormat(*sdpFormats.FindFormat(sendStream->GetMediaFormat()));
    sendStream->SetPaused((otherSidesDir&SDPMediaDescription::RecvOnly) == 0);
  }

  if (recvStream == NULL) {
    PTRACE(5, "SIP\tOpening rx " << mediaType << " stream from SDP");
    if (ownerCall.OpenSourceMediaStreams(*this, mediaType, rtpSessionId)) {
      recvStream = GetMediaStream(rtpSessionId, true);
      if (recvStream != NULL && (otherSidesDir&SDPMediaDescription::SendOnly) != 0)
        newDirection = newDirection != SDPMediaDescription::Inactive ? SDPMediaDescription::SendRecv
                                                                     : SDPMediaDescription::RecvOnly;
    }
  }

  if (recvStream != NULL) {
    OpalMediaFormat adjustedMediaFormat = *sdpFormats.FindFormat(recvStream->GetMediaFormat());

    // If we are sendrecv we will receive the same payload type as we transmit.
    if (newDirection == SDPMediaDescription::SendRecv)
      adjustedMediaFormat.SetPayloadType(sendStream->GetMediaFormat().GetPayloadType());

    recvStream->UpdateMediaFormat(adjustedMediaFormat);
    recvStream->SetPaused((otherSidesDir&SDPMediaDescription::SendOnly) == 0);
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
    for (OpalMediaFormatList::iterator remoteFormat = m_remoteFormatList.begin(); remoteFormat != m_remoteFormatList.end(); ++remoteFormat) {
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

  if (mediaType == OpalMediaType::Audio()) {
    // Set format if we have an RTP payload type for RFC2833 and/or NSE
    SetNxECapabilities(rfc2833Handler, localFormatList, sdpFormats, OpalRFC2833, localMedia);
#if OPAL_T38_CAPABILITY
    SetNxECapabilities(ciscoNSEHandler, localFormatList, sdpFormats, OpalCiscoNSE, localMedia);
#endif
  }

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
  // Need to limit the media formats to what the other side provided in a re-INVITE
  if (m_answerFormatList.IsEmpty()) {
    PTRACE(4, "SIP\tUsing remote media format list");
    return m_remoteFormatList;
  }
  else {
    PTRACE(4, "SIP\tUsing oferred media format list");
    return m_answerFormatList;
  }
}


void SIPConnection::SetRemoteMediaFormats(SDPSessionDescription * sdp)
{
  /* As SIP does not really do capability exchange, if we don't have an initial
     INVITE from the remote (indicated by sdp == NULL) then all we can do is
     assume the the remote can at do what we can do. We could assume it does
     everything we know about, but there is no point in assuming it can do any
     more than we can, really.
     */
  m_remoteFormatList = sdp != NULL ? sdp->GetMediaFormats() : GetLocalMediaFormats();

  m_remoteFormatList.Remove(endpoint.GetManager().GetMediaFormatMask());

  PTRACE(4, "SIP\tRemote media formats set to " << setfill(',') << m_remoteFormatList << setfill(' '));
}


OpalMediaStreamPtr SIPConnection::OpenMediaStream(const OpalMediaFormat & mediaFormat, unsigned sessionID, bool isSource)
{
  if (m_holdFromRemote && !isSource) {
    PTRACE(3, "SIP\tCannot start media stream as are currently in HOLD by remote.");
    return false;
  }

  // Make sure stream is symmetrical, if codec changed, close and re-open it
  OpalMediaStreamPtr otherStream = GetMediaStream(sessionID, !isSource);
  bool makesymmetrical = !m_symmetricOpenStream &&
                          otherStream != NULL &&
                          otherStream->IsOpen() &&
                          otherStream->GetMediaFormat() != mediaFormat;
  if (makesymmetrical) {
    m_symmetricOpenStream = true;
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
    m_symmetricOpenStream = false;
  }

  OpalMediaStreamPtr oldStream = GetMediaStream(sessionID, isSource);

  // Open forward side
  OpalMediaStreamPtr newStream = OpalRTPConnection::OpenMediaStream(mediaFormat, sessionID, isSource);
  if (newStream == NULL)
    return newStream;

  // Open other direction, if needed (must be after above open)
  if (makesymmetrical) {
    m_symmetricOpenStream = true;
    bool ok = GetCall().OpenSourceMediaStreams(*(isSource ? GetCall().GetOtherPartyConnection(*this) : this),
                                                          mediaFormat.GetMediaType(), sessionID, mediaFormat);
    m_symmetricOpenStream = false;
    if (!ok) {
      newStream->Close();
      return NULL;
    }
  }

  if (!m_symmetricOpenStream && !m_handlingINVITE && (newStream != oldStream || GetMediaStream(sessionID, !isSource) != otherStream))
    SendReINVITE(PTRACE_PARAM("open channel"));

  return newStream;
}


bool SIPConnection::CloseMediaStream(OpalMediaStream & stream)
{
  bool closed = OpalConnection::CloseMediaStream(stream);

  if (!m_symmetricOpenStream && !m_handlingINVITE)
    closed = SendReINVITE(PTRACE_PARAM("close channel")) && closed;

  return closed;
}


void SIPConnection::OnPauseMediaStream(OpalMediaStream & strm, bool paused)
{
  if (!m_symmetricOpenStream && !m_handlingINVITE)
    SendReINVITE(PTRACE_PARAM(paused ? "pausing channel" : "resuming channel"));
  OpalConnection::OnPauseMediaStream(strm, paused);
}


bool SIPConnection::SendReINVITE(PTRACE_PARAM(const char * msg))
{
  if (GetPhase() != EstablishedPhase)
    return false;

  bool startImmediate = !m_handlingINVITE && pendingInvitations.IsEmpty();

  PTRACE(3, "SIP\t" << (startImmediate ? "Start" : "Queue") << "ing re-INVITE to " << msg);

  m_needReINVITE = true;

  SIPTransaction * invite = new SIPInvite(*this, m_rtpSessions);

  // To avoid overlapping INVITE transactions, we place the new transaction
  // in a queue, if queue is empty we can start immediately, otherwise
  // it waits till we get a response.
  if (startImmediate) {
    if (!invite->Start())
      return false;
    m_handlingINVITE = true;
  }

  pendingInvitations.Append(invite);
  return true;
}


void SIPConnection::StartPendingReINVITE()
{
  while (!pendingInvitations.IsEmpty()) {
    PSafePtr<SIPTransaction> reInvite = pendingInvitations.GetAt(0, PSafeReadWrite);
    if (reInvite->IsInProgress())
      break;

    if (!reInvite->IsCompleted()) {
      if (reInvite->Start()) {
        m_handlingINVITE = true;
        break;
      }
    }

    pendingInvitations.RemoveAt(0);
  }
}


PBoolean SIPConnection::WriteINVITE(OpalTransport &, void * param)
{
  return ((SIPConnection *)param)->WriteINVITE();
}


bool SIPConnection::WriteINVITE()
{
  const SIPURL & requestURI = m_dialog.GetRequestURI();
  SIPURL myAddress = m_connStringOptions(OPAL_OPT_CALLING_PARTY_URL);
  if (myAddress.IsEmpty())
    myAddress = endpoint.GetRegisteredPartyName(requestURI, *transport);

  PString transportProtocol = requestURI.GetParamVars()("transport");
  if (!transportProtocol.IsEmpty())
    myAddress.SetParamVar("transport", transportProtocol);

  // only allow override of calling party number if the local party
  // name hasn't been first specified by a register handler. i.e a
  // register handler's target number is always used
  PString number(m_connStringOptions(OPAL_OPT_CALLING_PARTY_NUMBER, m_connStringOptions(OPAL_OPT_CALLING_PARTY_NAME)));
  if (!number.IsEmpty())
    myAddress.SetUserName(number);

  PString name(m_connStringOptions(OPAL_OPT_CALLING_DISPLAY_NAME));
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

  m_needReINVITE = false;
  SIPTransaction * invite = new SIPInvite(*this, OpalRTPSessionManager(*this));

  SIPURL contact = invite->GetMIME().GetContact();
  if (!number.IsEmpty())
    contact.SetUserName(number);
  if (!name.IsEmpty())
    contact.SetDisplayName(name);
  if (!domain.IsEmpty())
    contact.SetHostName(domain);
  invite->GetMIME().SetContact(contact);

  SIPURL redir(m_connStringOptions(OPAL_OPT_REDIRECTING_PARTY, m_redirectingParty));
  if (!redir.IsEmpty())
    invite->GetMIME().SetReferredBy(redir.AsQuotedString());

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

  if (!SetTransport(transportAddress))
    return false;

  ++m_sdpVersion;

  SetRemoteMediaFormats(NULL);

  bool ok;
  if (!transport->GetInterface().IsEmpty())
    ok = WriteINVITE();
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


PString SIPConnection::GetCallInfo() const
{
  return originalInvite != NULL ? originalInvite->GetMIME().GetCallInfo() : PString::Empty();
}


bool SIPConnection::Hold(bool fromRemote, bool placeOnHold)
{
  if (transport == NULL)
    return false;

#if PTRACING
  const char * holdStr = placeOnHold ? "on" : "off";
#endif

  if (fromRemote) {
    if (m_holdFromRemote == placeOnHold) {
      PTRACE(4, "SIP\tHold " << holdStr << " request ignored as already set on " << *this);
      return true;
    }
    m_holdFromRemote = placeOnHold;
    if (SendReINVITE(PTRACE_PARAM(placeOnHold ? "break remote hold" : "request remote hold")))
      return true;
    m_holdFromRemote = !placeOnHold;
    return false;
  }

  switch (m_holdToRemote) {
    case eHoldOff :
      if (!placeOnHold) {
        PTRACE(4, "SIP\tHold off request ignored as not in hold on " << *this);
        return true;
      }
      break;

    case eHoldOn :
      if (placeOnHold) {
        PTRACE(4, "SIP\tHold on request ignored as already in hold on " << *this);
        return true;
      }
      break;

    default :
      PTRACE(4, "SIP\tHold " << holdStr << " request ignored as in progress on " << *this);
      return false;
  }


  HoldState origState = m_holdToRemote;
  m_holdToRemote = placeOnHold ? eHoldInProgress : eRetrieveInProgress;

  if (SendReINVITE(PTRACE_PARAM(placeOnHold ? "put connection on hold" : "retrieve connection from hold")))
    return true;

  m_holdToRemote = origState;
  return false;
}


PBoolean SIPConnection::IsOnHold(bool fromRemote)
{
  return fromRemote ? m_holdFromRemote : (m_holdToRemote >= eHoldOn);
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
  std::map<std::string, SIP_PDU *>::iterator it = m_responses.find(transaction.GetTransactionID());
  if (it != m_responses.end()) {
    it->second->SetStatusCode(transaction.GetStatusCode());
    m_responses.erase(it);
  }

  switch (transaction.GetMethod()) {
    case SIP_PDU::Method_INVITE :
      break;

    case SIP_PDU::Method_REFER :
      m_referInProgress = false;
      // Do next case

    default :
      return;
  }

  m_handlingINVITE = false;

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

  PString user = url.GetUserName();
  if (OpalIsE164(user))
    remotePartyNumber = user;

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
  if (transaction.GetMethod() != SIP_PDU::Method_INVITE) {
    switch (response.GetStatusCode()) {
      case SIP_PDU::Failure_UnAuthorised :
      case SIP_PDU::Failure_ProxyAuthenticationRequired :
        if (OnReceivedAuthenticationRequired(transaction, response))
          return;

      default :
        switch (response.GetStatusCode()/100) {
          case 1 : // Treat all other provisional responses like a Trying.
            OnReceivedTrying(transaction, response);
            return;

          case 2 : // Successful response - there really is only 200 OK
            OnReceivedOK(transaction, response);
            break;
        }
    }

    std::map<std::string, SIP_PDU *>::iterator it = m_responses.find(transaction.GetTransactionID());
    if (it != m_responses.end()) {
      *it->second = response;
      m_responses.erase(it);
    }

    return;
  }

  PSafeLockReadWrite lock(*this);
  if (!lock.IsLocked())
    return;

  bool handled = false;

  // Break out to virtual functions for some special cases.
  switch (response.GetStatusCode()) {
    case SIP_PDU::Information_Ringing :
      OnReceivedRinging(response);
      return;

    case SIP_PDU::Information_Session_Progress :
      OnReceivedSessionProgress(response);
      return;

    case SIP_PDU::Failure_UnAuthorised :
    case SIP_PDU::Failure_ProxyAuthenticationRequired :
      if (OnReceivedAuthenticationRequired(transaction, response))
        handled = true;
      break;

    case SIP_PDU::Failure_MessageTooLarge :
    {
      SIPURL newTransportAddress(transport->GetRemoteAddress());
      newTransportAddress.SetParamVar("transport", "tcp");
      if (!SetTransport(newTransportAddress))
        break;

      SIPInvite * newInvite = new SIPInvite(*this, ((SIPInvite &)transaction).GetSessionManager());
      if (!newInvite->Start()) {
        PTRACE(2, "SIP\tCould not restart INVITE for switch to TCP");
        break;
      }

      forkedInvitations.Append(newInvite);
      return;
    }

    case SIP_PDU::Failure_RequestPending :
      SendReINVITE(PTRACE_PARAM("retry after getting 491 Request Pending"));
      return;

    default :
      switch (response.GetStatusCode()/100) {
        case 1 : // Treat all other provisional responses like a Trying.
          OnReceivedTrying(transaction, response);
          return;

        case 2 : // Successful response - there really is only 200 OK
          OnReceivedOK(transaction, response);
          handled = true;
          break;

        case 3 : // Redirection response
          OnReceivedRedirection(response);
          handled = true;
          break;
      }
  }

  m_handlingINVITE = false;

  // To avoid overlapping INVITE transactions, wait till here before
  // starting the next one.
  if (response.GetStatusCode()/100 != 1) {
    pendingInvitations.Remove(&transaction);
    StartPendingReINVITE();
  }

  if (handled)
    return;

  // If we are doing a local hold, and it failed, we do not release the connection
  switch (m_holdToRemote) {
    case eHoldInProgress :
      PTRACE(4, "SIP\tHold request failed on " << *this);
      m_holdToRemote = eHoldOff;  // Did not go into hold
      OnHold(false, false);   // Signal the manager that there is no more hold
      break;

    case eRetrieveInProgress :
      PTRACE(4, "SIP\tRetrieve request failed on " << *this);
      m_holdToRemote = eHoldOn;  // Did not go out of hold
      OnHold(false, true);   // Signal the manager that hold is still active
      break;

    default :
      break;
  }

  if (GetPhase() == EstablishedPhase) {
    // Is a re-INVITE if in here, so don't kill the call becuase it failed.
#if OPAL_FAX
    SDPSessionDescription * sdp = transaction.GetSDP();
    bool switchingToFax = sdp != NULL && sdp->GetMediaDescriptionByType(OpalMediaType::Fax()) != NULL;
    if (m_switchedToFaxMode != switchingToFax)
      OnSwitchedFaxMediaStreams(m_switchedToFaxMode);
#endif
    return;
  }

  // We don't always release the connection, eg not till all forked invites have completed
  // This INVITE is from a different "dialog", any errors do not cause a release

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


SIPConnection::TypeOfINVITE SIPConnection::CheckINVITE(const SIP_PDU & request) const
{
  const SIPMIMEInfo & requestMIME = request.GetMIME();
  PString requestFromTag = requestMIME.GetFieldParameter("From", "tag");
  PString requestToTag   = requestMIME.GetFieldParameter("To",   "tag");

  // Criteria for our existing dialog.
  if (!requestToTag.IsEmpty() &&
       m_dialog.GetCallID() == requestMIME.GetCallID() &&
       m_dialog.GetRemoteTag() == requestFromTag &&
       m_dialog.GetLocalTag() == requestToTag)
    return IsReINVITE;

  if (IsOriginating()) {
    /* Weird, got incoming INVITE for a call we originated, however it is not in
       the same dialog. Send back a refusal as this just isn't handled. */
    PTRACE(2, "SIP\tIgnoring INVITE from " << request.GetURI() << " when originated call.");
    return IsLoopedINVITE;
  }

  // No original INVITE, so must be first time
  if (originalInvite == NULL)
    return IsNewINVITE;

  /* If we have same transaction ID, it means it is a retransmission
     of the original INVITE, probably should re-transmit last sent response
     but we just ignore it. Still should work. */
  if (originalInvite->GetTransactionID() == request.GetTransactionID()) {
    PTimeInterval timeSinceInvite = PTime() - originalInviteTime;
    PTRACE(3, "SIP\tIgnoring duplicate INVITE from " << request.GetURI() << " after " << timeSinceInvite);
    return IsDuplicateINVITE;
  }

  // Check if is RFC3261/8.2.2.2 case relating to merged requests.
  if (requestToTag.IsEmpty() && requestMIME.GetCSeqIndex() > originalInvite->GetMIME().GetCSeqIndex())
    return IsNewINVITE; // No it isn't

  /* This is either a merged request or a brand new "dialog" to the same call.
     Probably indicates forking request or request arriving via multiple IP
     paths. In both cases we refuse the INVITE. The first because we don't
     support multiple dialogs on one call, the second because the RFC says
     we should! */
  PTRACE(3, "SIP\tIgnoring forked INVITE from " << request.GetURI());
  return IsLoopedINVITE;
}


void SIPConnection::OnReceivedINVITE(SIP_PDU & request)
{
  bool isReinvite = IsOriginating() || originalInvite != NULL;
  PTRACE_IF(4, !isReinvite, "SIP\tInitial INVITE from " << request.GetURI());

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

  m_redirectingParty = mime.GetReferredBy();

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
  m_handlingINVITE = true;

  SetPhase(SetUpPhase);

  SetRemoteMediaFormats(originalInvite->GetSDP());

  // See if we have a replaces header, if not is normal call
  PString replaces = mime("Replaces");
  if (replaces.IsEmpty()) {
    // indicate the other is to start ringing (but look out for clear calls)
    if (!OnIncomingConnection(0, NULL)) {
      PTRACE(1, "SIP\tOnIncomingConnection failed for INVITE from " << request.GetURI() << " for " << *this);
      Release();
      return;
    }

    PTRACE(3, "SIP\tOnIncomingConnection succeeded for INVITE from " << request.GetURI() << " for " << *this);

    OnApplyStringOptions();

    if (ownerCall.OnSetUp(*this)) {
      AnsweringCall(OnAnswerCall(GetRemotePartyURL()));
      return;
    }

    PTRACE(1, "SIP\tOnSetUp failed for INVITE from " << request.GetURI() << " for " << *this);
    Release();
    return;
  }

  // Replaces header string has already been validated in SIPEndPoint::OnReceivedINVITE
  PSafePtr<SIPConnection> replacedConnection = endpoint.GetSIPConnectionWithLock(replaces, PSafeReadOnly);
  if (replacedConnection == NULL) {
    /* Allow for a race condition where between when SIPEndPoint::OnReceivedINVITE()
       is executed and here, the call to be replaced was released. */
    Release(EndedByInvalidConferenceID);
    return;
  }

  if (replacedConnection->GetPhase() < ConnectedPhase) {
    if (!replacedConnection->IsOriginating()) {
      PTRACE(3, "SIP\tEarly connection " << *replacedConnection << " cannot be replaced by " << *this);
      Release(EndedByInvalidConferenceID);
      return;
    }
  }
  else {
    if (replaces.Find(";early-only") != P_MAX_INDEX) {
      PTRACE(3, "SIP\tReplaces has early-only on early connection " << *this);
      Release(EndedByLocalBusy);
      return;
    }
  }

  PTRACE(3, "SIP\tEstablished connection " << *replacedConnection << " replaced by " << *this);

  // Do OnRelease for other connection synchronously or there is
  // confusion with media streams still open
  replacedConnection->synchronousOnRelease = true;
  replacedConnection->Release(OpalConnection::EndedByCallForwarded);

  // Check if we are the target of an attended transfer, indicated by a referred-by header
  PString referredBy = mime.GetReferredBy();
  if (!referredBy.IsEmpty()) {
    /* Indicate to application we are party C in a consultation transfer.
       The calls are A->B (first call), B->C (consultation call) => A->C
       (final call after the transfer) */
    PStringToString info;
    PCaselessString state = mime.GetSubscriptionState(info);
    info.SetAt("party", "C");
    info.SetAt("Referred-By", referredBy);
    OnTransferNotify(info);
  }

  /* According to RFC 3891 we now send a 200 OK in both the earl and confirmed
     dialog cases. OnReleased() is responsible for if the replaced connection is
     sent a BYE or a CANCEL. */
  SetConnected();
}


void SIPConnection::OnReceivedReINVITE(SIP_PDU & request)
{
  if (m_handlingINVITE || GetPhase() < ConnectedPhase) {
    PTRACE(2, "SIP\tRe-INVITE from " << request.GetURI() << " received while INVITE in progress on " << *this);
    request.SendResponse(*transport, SIP_PDU::Failure_RequestPending);
    return;
  }

  PTRACE(3, "SIP\tReceived re-INVITE from " << request.GetURI() << " for " << *this);

  m_handlingINVITE = true;

  SDPSessionDescription sdpOut(m_sdpSessionId, ++m_sdpVersion, GetDefaultSDPConnectAddress());

  // send the 200 OK response
  if (OnSendAnswerSDP(m_rtpSessions, sdpOut, true))
    SendInviteOK(sdpOut);
  else
    SendInviteResponse(SIP_PDU::Failure_NotAcceptableHere);

  m_answerFormatList.RemoveAll();
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
  ackTimer.Stop(false); // Asynchronous stop to avoid deadlock
  ackRetry.Stop(false);

  OnReceivedAnswerSDP(response);

  m_handlingINVITE = false;

  switch (GetPhase()) {
    case ConnectedPhase :
      SetPhase(EstablishedPhase);
      OnEstablished();
      break;

    case EstablishedPhase :
      // If we receive an ACK in established phase, it is a re-INVITE
      StartMediaStreams();
      break;

    default :
      break;
  }

  StartPendingReINVITE();
}


void SIPConnection::OnReceivedOPTIONS(SIP_PDU & request)
{
  if (request.GetMIME().GetAccept().Find("application/sdp") == P_MAX_INDEX)
    request.SendResponse(*transport, SIP_PDU::Failure_UnsupportedMediaType);
  else {
    SDPSessionDescription sdp(m_sdpSessionId, m_sdpVersion, transport->GetLocalAddress());
    SIP_PDU response(request, SIP_PDU::Successful_OK);
    response.SetAllow(endpoint.GetAllowedMethods());
    response.SetEntityBody(sdp.Encode());
    request.SendResponse(*transport, response, &endpoint);
  }
}


void SIPConnection::OnAllowedEventNotify(const PString & /* eventStr */)
{
}


void SIPConnection::OnReceivedNOTIFY(SIP_PDU & request)
{
  /* Transfering a Call
     We have sent a REFER to the UA in this connection.
     Now we get Progress Indication of that REFER Request by NOTIFY Requests.
     Handle the response coded of "message/sipfrag" as follows:

     Use code >= 180  &&  code < 300 to Release this dialog.
     Use Subscription State = terminated to Release this dialog.
  */

  const SIPMIMEInfo & mime = request.GetMIME();

  SIPSubscribe::EventPackage package(mime.GetEvent());
  if (m_allowedEvents.GetStringsIndex(package) != P_MAX_INDEX) {
    PTRACE(2, "SIP\tReceived Notify for allowed event " << package);
    request.SendResponse(*transport, SIP_PDU::Successful_OK);
    OnAllowedEventNotify(package);
    return;
  }

  // Do not include the id parameter in this comparison, may need to
  // do it later if we ever support multiple simultaneous REFERs
  if (package.Find("refer") == P_MAX_INDEX) {
    PTRACE(2, "SIP\tNOTIFY in a connection only supported for REFER requests");
    request.SendResponse(*transport, SIP_PDU::Failure_BadEvent);
    return;
  }

  if (!m_referInProgress) {
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

  PStringToString info;
  PCaselessString state = mime.GetSubscriptionState(info);
  info.SetAt("party", "B"); // We are B party in consultation transfer
  info.SetAt("state", state);
  info.SetAt("code", psprintf("%u", code));
  info.SetAt("result", state != "terminated" || code < 200
               ? "progress" : (code < 300 ? "success" : "failed"));

  if (OnTransferNotify(info))
    return;

  m_referInProgress = false;

  // Release the connection
  if (GetPhase() >= ReleasingPhase)
    return;

  releaseMethod = ReleaseWithBYE;
  Release(OpalConnection::EndedByCallForwarded);
}


void SIPConnection::OnReceivedREFER(SIP_PDU & request)
{
  const SIPMIMEInfo & requestMIME = request.GetMIME();

  PString referTo = requestMIME.GetReferTo();
  if (referTo.IsEmpty()) {
    request.SendResponse(*transport, SIP_PDU::Failure_BadRequest, NULL, "Missing refer-to header");
    return;
  }    

  SIP_PDU response(request, SIP_PDU::Successful_Accepted);

  // Comply to RFC4488
  bool referSub = true;
  if (requestMIME.Contains("Refer-Sub")) {
    referSub = !(requestMIME["Refer-Sub"] *= "false");
    response.GetMIME().SetAt("Refer-Sub", referSub ? "true" : "false");
  }

  // send response before attempting the transfer
  if (!request.SendResponse(*transport, response))
    return;

  SIPURL to = referTo;
  PString replaces = to.GetQueryVars()("Replaces");
  to.SetQuery(PString::Empty());

  bool ok = endpoint.SetupTransfer(GetToken(), replaces, to.AsString(), NULL);

  // send NOTIFY if transfer failed, but only if allowed by RFC4488
  if (referSub) {
    SIPReferNotify * notify = new SIPReferNotify(*this, ok ? SIP_PDU::Successful_OK : SIP_PDU::GlobalFailure_Decline);
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
  PTRACE(3, "SIP\tReceived Ringing response");

  OnReceivedAnswerSDP(response);

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
  PTRACE(3, "SIP\tReceived Session Progress response");

  OnReceivedAnswerSDP(response);

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
  SIPURL whereTo = response.GetMIME().GetContact();
  PTRACE(3, "SIP\tReceived redirect to " << whereTo);
  endpoint.ForwardConnection(*this, whereTo.AsString());
}


PBoolean SIPConnection::OnReceivedAuthenticationRequired(SIPTransaction & transaction, SIP_PDU & response)
{
  PBoolean isProxy = response.GetStatusCode() == SIP_PDU::Failure_ProxyAuthenticationRequired;

#if PTRACING
  const char * proxyTrace = isProxy ? "Proxy " : "";
#endif

  PTRACE(3, "SIP\tReceived " << proxyTrace << "Authentication Required response for " << transaction);

  // determine the authentication type
  PString errorMsg;
  SIPAuthentication * newAuth = PHTTPClientAuthentication::ParseAuthenticationRequired(isProxy, response.GetMIME(), errorMsg);
  if (newAuth == NULL) {
    PTRACE(1, "SIP\t" << errorMsg);
    return false;
  }

  // Try to find authentication parameters for the given realm,
  // if not, use the proxy authentication parameters (if any)
  PString username = m_dialog.GetLocalURI().GetUserName();
  PString password;
  if (endpoint.GetAuthentication(newAuth->GetAuthRealm(), username, password)) {
    PTRACE (3, "SIP\tFound auth info for realm \"" << newAuth->GetAuthRealm() << "\", user \"" << username << '"');
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
  unsigned cseq = transaction.GetMIME().GetCSeqIndex();
  if (m_authenticatedCseq != cseq && m_authentication != NULL && *newAuth == *m_authentication) {
    PTRACE(1, "SIP\tAuthentication already performed using current credentials, not trying again.");
    delete newAuth;
    return false;
  }

  // Restart the transaction with new authentication info
  delete m_authentication;
  m_authentication = newAuth;
  m_authenticatedCseq = cseq;

  // Make sure we increment sequence number as the call inside SIPInvite ctor
  // will not do so due to prevention to increment on "interface forked" INVITEs
  m_dialog.GetNextCSeq();

  transport->SetInterface(transaction.GetInterface());

  SIPTransaction * newTransaction = transaction.CreateDuplicate();
  if (newTransaction == NULL) {
    PTRACE(1, "SIP\tCannot do " << proxyTrace << "Authentication Required for " << transaction);
    return false;
  }

  if (!newTransaction->Start()) {
    PTRACE(2, "SIP\tCould not restart " << transaction << " for " << proxyTrace << "Authentication Required");
    return false;
  }

  if (transaction.GetMethod() == SIP_PDU::Method_INVITE)
    forkedInvitations.Append(newTransaction);
  else {
    std::map<std::string, SIP_PDU *>::iterator it = m_responses.find(transaction.GetTransactionID());
    if (it != m_responses.end()) {
      m_responses[newTransaction->GetTransactionID()] = it->second;
      m_responses.erase(it);
    }
  }

  return true;
}


void SIPConnection::OnReceivedOK(SIPTransaction & transaction, SIP_PDU & response)
{
  switch (transaction.GetMethod()) {
    case SIP_PDU::Method_INVITE :
      break;

    case SIP_PDU::Method_REFER :
      if (response.GetMIME()("Refer-Sub") == "false") {
        // Used RFC4488 to indicate we are NOT doing NOTIFYs, release now
        PTRACE(3, "SIP\tBlind transfer accepted, without NOTIFY so ending local call.");
        Release(OpalConnection::EndedByCallForwarded);
      }
      // Do next case

    default :
      return;
  }

  PTRACE(3, "SIP\tHandling " << response.GetStatusCode() << " response for " << transaction.GetMethod());

  /* See if the contact address provided in the response changes the transport
     type. Do this only if no Record-Route header is set. Otherwise we will
     continue to send SIP Messages to the proxy. */
  if (response.GetMIME().GetRecordRoute(false).IsEmpty()) {  
    OpalTransportAddress newContactAddress = SIPURL(response.GetMIME().GetContact()).GetHostAddress();
    if (!newContactAddress.IsCompatible(transport->GetLocalAddress())) {
      PTRACE(2, "SIP\tINVITE response changed transport for call");
      if (!SetTransport(newContactAddress))
        return;
    }
  }

  PTRACE(3, "SIP\tReceived INVITE OK response");
  releaseMethod = ReleaseWithBYE;
  sessionTimer = 10000;

  NotifyDialogState(SIPDialogNotification::Confirmed);

  OnReceivedAnswerSDP(response);

#if OPAL_FAX
  SDPSessionDescription * sdp = transaction.GetSDP();
  bool switchingToFax = sdp != NULL && sdp->GetMediaDescriptionByType(OpalMediaType::Fax()) != NULL;

  sdp = response.GetSDP();
  bool switchedToFax = sdp != NULL && sdp->GetMediaDescriptionByType(OpalMediaType::Fax()) != NULL;

  // Attempted to change fax state, but the remote rudely ignored it!
  if (switchingToFax != switchedToFax)
    OnSwitchedFaxMediaStreams(m_switchedToFaxMode); // iIndicate no change of existing state
  else {
    // We asked for fax/audio, we got fax/audio
    if (m_switchedToFaxMode != switchedToFax) {
      // And wasn't a repeat ...
      m_switchedToFaxMode = switchedToFax;
      OnSwitchedFaxMediaStreams(m_switchedToFaxMode);
    }
  }
#endif

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


void SIPConnection::OnReceivedAnswerSDP(SIP_PDU & response)
{
  SDPSessionDescription * sdp = response.GetSDP();
  if (sdp == NULL)
    return;

  m_answerFormatList = sdp->GetMediaFormats();
  m_answerFormatList.Remove(endpoint.GetManager().GetMediaFormatMask());

  m_holdFromRemote = sdp->IsHold();

  unsigned sessionCount = sdp->GetMediaDescriptions().GetSize();

  bool ok = false;
  for (unsigned session = 1; session <= sessionCount; ++session) {
    if (OnReceivedAnswerSDPSession(*sdp, session))
      ok = true;
    else {
      OpalMediaStreamPtr stream;
      if ((stream = GetMediaStream(session, false)) != NULL)
        stream->Close();
      if ((stream = GetMediaStream(session, true)) != NULL)
        stream->Close();
    }
  }

  m_answerFormatList.RemoveAll();

  /* Shut down any media that is in a session not mentioned in a re-INVITE.
     While the SIP/SDP specification says this shouldn't happen, it does
     anyway so we need to deal. */
  for (OpalMediaStreamPtr stream(mediaStreams, PSafeReference); stream != NULL; ++stream) {
    if (stream->GetSessionID() > sessionCount)
      stream->Close();
  }

  if (GetPhase() == EstablishedPhase)
    StartMediaStreams(); // re-INVITE
  else {
    if (!ok)
      Release(EndedByCapabilityExchange);
  }
}


bool SIPConnection::OnReceivedAnswerSDPSession(SDPSessionDescription & sdp, unsigned rtpSessionId)
{
  SDPMediaDescription * mediaDescription = sdp.GetMediaDescriptionByIndex(rtpSessionId);
  if (!PAssert(mediaDescription != NULL, "SDP Media description list changed"))
    return false;

  OpalMediaType mediaType = mediaDescription->GetMediaType();
  
  PTRACE(4, "SIP\tProcessing received SDP media description for " << mediaType);

  /* Get the media the remote has answered to our offer. Remove the media
     formats we do not support, in case the remote is insane and replied
     with something we did not actually offer. */
  if (!m_answerFormatList.HasType(mediaType)) {
    PTRACE(2, "SIP\tCould not find supported media formats in SDP media description for session " << rtpSessionId);
    return false;
  }

  // Set up the media session, e.g. RTP
  bool remoteChanged = false;
  OpalTransportAddress localAddress;
  if (SetUpMediaSession(rtpSessionId, mediaType, mediaDescription, localAddress, remoteChanged) == NULL)
    return false;

  SDPMediaDescription::Direction otherSidesDir = sdp.GetDirection(rtpSessionId);

  // Check if we had a stream and the remote has either changed the codec or
  // changed the direction of the stream
  OpalMediaStreamPtr sendStream = GetMediaStream(rtpSessionId, false);
  PauseOrCloseMediaStream(sendStream, m_answerFormatList, remoteChanged, (otherSidesDir&SDPMediaDescription::RecvOnly) == 0);

  OpalMediaStreamPtr recvStream = GetMediaStream(rtpSessionId, true);
  PauseOrCloseMediaStream(recvStream, m_answerFormatList, remoteChanged, (otherSidesDir&SDPMediaDescription::SendOnly) == 0);

  // Then open the streams if the direction allows and if needed
  // If already open then update to new parameters/payload type

  if (recvStream == NULL && (otherSidesDir&SDPMediaDescription::SendOnly) != 0)
    ownerCall.OpenSourceMediaStreams(*this, mediaType, rtpSessionId);

  if (sendStream == NULL && (otherSidesDir&SDPMediaDescription::RecvOnly) != 0) {
    PSafePtr<OpalConnection> otherParty = GetOtherPartyConnection();
    if (otherParty != NULL)
      ownerCall.OpenSourceMediaStreams(*otherParty, mediaType, rtpSessionId);
  }

  if (mediaType == OpalMediaType::Audio()) {
    OpalMediaFormatList localMediaFormats = GetLocalMediaFormats();
    SetNxECapabilities(rfc2833Handler, localMediaFormats, m_answerFormatList, OpalRFC2833);
#if OPAL_T38_CAPABILITY
    SetNxECapabilities(ciscoNSEHandler, localMediaFormats, m_answerFormatList, OpalCiscoNSE);
#endif
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

  if (m_needReINVITE)
    ++m_sdpVersion;

  if (IsPresentationBlocked()) {
    // Should do more as per RFC3323, but this is all for now
    SIPURL from = mime.GetFrom();
    if (!from.GetDisplayName(false).IsEmpty())
      from.SetDisplayName("Anonymous");
    mime.SetFrom(from.AsQuotedString());
  }

  if (m_connStringOptions.GetBoolean(OPAL_OPT_INITIAL_OFFER, true)) {
    SDPSessionDescription * sdp = new SDPSessionDescription(m_sdpSessionId, m_sdpVersion, OpalTransportAddress());
    if (OnSendOfferSDP(request.GetSessionManager(), *sdp))
      request.SetSDP(sdp);
    else {
      delete sdp;
      Release(EndedByCapabilityExchange);
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

  if (response.GetStatusCode() == SIP_PDU::Information_Ringing) {
    if (m_allowedEvents.GetSize() > 0) {
      PStringStream strm; strm << setfill(',') << m_allowedEvents;
      response.GetMIME().SetAllowEvents(strm);
    }
    response.GetMIME().SetAlertInfo(m_alertInfo, m_appearanceCode);
  }

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
    m_handlingINVITE = false;
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

  if (contentType.NumCompare(ApplicationDTMFRelayKey) == EqualTo) {
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
      OnUserInputTone(tone, duration == 0 ? 100 : duration);
    status = SIP_PDU::Successful_OK;
  }

  else if (contentType.NumCompare(ApplicationDTMFKey) == EqualTo) {
    PString tones = request.GetEntityBody().Trim();
    if (tones.GetLength() == 1)
      OnUserInputTone(tones[0], 100);
    else
      OnUserInputString(tones);
    status = SIP_PDU::Successful_OK;
  }

#if OPAL_VIDEO
  else if (contentType.NumCompare(ApplicationMediaControlXMLKey) == EqualTo) {
    if (OnMediaControlXML(request))
      return;
    status = SIP_PDU::Failure_UnsupportedMediaType;
  }
#endif

  else 
    status = SIP_PDU::Failure_UnsupportedMediaType;

  request.SendResponse(*transport, status);

  if (status == SIP_PDU::Successful_OK) {
    // Have INFO user input, disable the in-band tone detcetor to avoid double detection
    m_detectInBandDTMF = false;

    OpalMediaStreamPtr stream = GetMediaStream(OpalMediaType::Audio(), true);
    if (stream != NULL && stream->RemoveFilter(m_dtmfDetectNotifier, OPAL_PCM16)) {
      PTRACE(4, "OpalCon\tRemoved detect DTMF filter on connection " << *this);
    }
  }
}


void SIPConnection::OnReceivedPING(SIP_PDU & request)
{
  PTRACE(3, "SIP\tReceived PING");
  request.SendResponse(*transport, SIP_PDU::Successful_OK);
}


void SIPConnection::OnReceivedMESSAGE(SIP_PDU & pdu)
{
  PTRACE(3, "SIP\tReceived MESSAGE in the context of a call");

  PString contentType = pdu.GetMIME().GetContentType();
  if (contentType.IsEmpty())
    contentType = "text/plain";
#if OPAL_HAS_IM
  RTP_DataFrameList frames = m_rfc4103Context[0].ConvertToFrames(contentType, pdu.GetEntityBody());

  for (PINDEX i = 0; i < frames.GetSize(); ++i)
    OnReceiveExternalIM(OpalT140, (RTP_IMFrame &)frames[i]);
#endif
  pdu.SendResponse(*transport, SIP_PDU::Successful_OK);
}


OpalConnection::SendUserInputModes SIPConnection::GetRealSendUserInputMode() const
{
  switch (sendUserInputMode) {
    case SendUserInputAsTone :
      return SendUserInputAsTone;

    case SendUserInputAsProtocolDefault :
    case SendUserInputAsRFC2833 :
      if (m_remoteFormatList.HasFormat(OpalRFC2833))
        return SendUserInputAsRFC2833;
      // Drop into INFO string mode

    default :
      // Eveything else is INFO string mode, lowest common denominator
      return SendUserInputAsString;
  }
}


PBoolean SIPConnection::SendUserInputTone(char tone, unsigned duration)
{
  if (m_holdFromRemote || m_holdToRemote >= eHoldOn)
    return false;

  SendUserInputModes mode = GetRealSendUserInputMode();

  PTRACE(3, "SIP\tSendUserInputTone('" << tone << "', " << duration << "), using mode " << mode);

  if (mode == SendUserInputAsRFC2833)
    return OpalRTPConnection::SendUserInputTone(tone, duration);

  SIPInfo::Params params;
  if (mode == SendUserInputAsTone) {
    params.m_contentType = ApplicationDTMFRelayKey;
    PStringStream strm;
    strm << "Signal= " << tone << "\r\n" << "Duration= " << duration << "\r\n";  // spaces are important. Who can guess why?
    params.m_body = strm;
  }
  else {
    params.m_contentType = ApplicationDTMFKey;
    params.m_body = tone;
  }

  if (SendINFO(params))
    return true;

  PTRACE(2, "SIP\tCould not send tone '" << tone << "' via INFO.");
  return OpalConnection::SendUserInputTone(tone, duration);
}


bool SIPConnection::SendOPTIONS(const SIPOptions::Params & params, SIP_PDU * reply)
{
  PSafePtr<SIPTransaction> transaction = new SIPOptions(*this, params);
  if (reply == NULL)
    return transaction->Start();

  m_responses[transaction->GetTransactionID()] = reply;
  transaction->WaitForTermination();
  return !transaction->IsFailed();
}


bool SIPConnection::SendINFO(const SIPInfo::Params & params, SIP_PDU * reply)
{
  PSafePtr<SIPTransaction> transaction = new SIPInfo(*this, params);
  if (reply == NULL)
    return transaction->Start();

  m_responses[transaction->GetTransactionID()] = reply;
  transaction->WaitForTermination();
  return !transaction->IsFailed();
}


#if OPAL_HAS_IM

bool SIPConnection::TransmitExternalIM(const OpalMediaFormat & /*format*/, RTP_IMFrame & body)
{
#if OPAL_HAS_MSRP
  // if the call contains an MSRP connection, then use that
  for (OpalMediaStreamPtr mediaStream(mediaStreams, PSafeReference); mediaStream != NULL; ++mediaStream) {
    if (mediaStream->IsSink() && (mediaStream->GetMediaFormat() == OpalMSRP)) {
      PTRACE(3, "SIP\tSending MSRP packet within call");
      mediaStream.SetSafetyMode(PSafeReadWrite);
      int written;
      return mediaStream->WriteData(body.GetPayloadPtr(), body.GetPayloadSize(), written);
    }
  }
#endif

  PTRACE(3, "SIP\tSending MESSAGE within call");

  // else send as MESSAGE
  SIPMessage::Params params;
  params.m_body = body.AsString();

  PSafePtr<SIPTransaction> transaction = new SIPMessage(*this, params);
  return transaction->Start();
}

#endif

void SIPConnection::OnMediaCommand(OpalMediaCommand & command, INT extra)
{
#if OPAL_VIDEO
  if (PIsDescendant(&command, OpalVideoUpdatePicture)) {
    PTRACE(3, "SIP\tSending PictureFastUpdate");
    SIPInfo::Params params(ApplicationMediaControlXMLKey,
                           "<?xml version=\"1.0\" encoding=\"utf-8\" ?>"
                           "<media_control>"
                            "<vc_primitive>"
                             "<to_encoder>"
                              "<picture_fast_update>"
                              "</picture_fast_update>"
                             "</to_encoder>"
                            "</vc_primitive>"
                           "</media_control>");
    SendINFO(params);
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
        unsigned i;
        for (i = 0; i < numStates; ++i) {
          if ((state == states[i].currState) && (str.compare(0, strlen(states[i].str), states[i].str) == 0)) {
            state = states[i].newState;
            break;
          }
        }
        if (i == numStates) {
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
        { 4, "picture_fast_update/", 6 },
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
      if (state == 6)
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
    response.SetEntityBody(
      "<?xml version=\"1.0\" encoding=\"utf-8\" ?>\n"
      "<media_control>\n"
      "  <general_error>\n"
      "  Unable to parse XML request\n"
      "   </general_error>\n"
      "</media_control>\n");
    request.SendResponse(*transport, response);
  }
  else {
    SendVideoUpdatePicture();
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
  connection.SendVideoUpdatePicture(session.GetSessionID());
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
