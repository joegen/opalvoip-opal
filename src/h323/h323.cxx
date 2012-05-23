/*
 * h323.cxx
 *
 * H.323 protocol handler
 *
 * Open H323 Library
 *
 * Copyright (c) 1998-2000 Equivalence Pty. Ltd.
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
 * Contributor(s): ______________________________________.
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#include <ptlib.h>

#include <opal/buildopts.h>
#if OPAL_H323

#ifdef __GNUC__
#pragma implementation "h323con.h"
#endif

#include <opal/buildopts.h>

#include <h323/h323con.h>

#include <h323/h323ep.h>
#include <h323/h323neg.h>
#include <h323/h323rtp.h>
#include <h323/gkclient.h>

#if OPAL_H450
#include <h323/h450pdu.h>
#endif

#include <h323/transaddr.h>
#include <opal/call.h>
#include <opal/patch.h>
#include <codec/rfc2833.h>

#if OPAL_VIDEO
#include <codec/vidcodec.h>
#endif

#if OPAL_HAS_H224
#include <h224/h224.h>
#endif

#if OPAL_H460
#include <h460/h460.h>
#include <h460/h4601.h>
#include <ptclib/random.h>
#include "h460/h46018_h225.h"
#include "h460/h46019.h"
#endif


static const char h323_rtp_session_type[] = "rtp/h323";
static OpalMediaSessionFactory::Worker<H323RTPSession> h323_rtp_session(h323_rtp_session_type);

const PTimeInterval MonitorCallStatusTime(0, 10); // Seconds

#if OPAL_H239
static const PString & H239MessageOID = "0.0.8.239.2";
#endif

#define new PNEW


/////////////////////////////////////////////////////////////////////////////

#if PTRACING

const char * H323Connection::GetConnectionStatesName(ConnectionStates s)
{
  static const char * const names[NumConnectionStates] = {
    "NoConnectionActive",
    "AwaitingGatekeeperAdmission",
    "AwaitingTransportConnect",
    "AwaitingSignalConnect",
    "AwaitingLocalAnswer",
    "HasExecutedSignalConnect",
    "EstablishedConnection",
    "ShuttingDownConnection"
  };
  return s < PARRAYSIZE(names) ? names[s] : "<Unknown>";
}

const char * H323Connection::GetFastStartStateName(FastStartStates s)
{
  static const char * const names[NumFastStartStates] = {
    "FastStartDisabled",
    "FastStartInitiate",
    "FastStartResponse",
    "FastStartAcknowledged"
  };
  return s < PARRAYSIZE(names) ? names[s] : "<Unknown>";
}
#endif

#if OPAL_H460
static void ReceiveSetupFeatureSet(const H323Connection * connection, const H225_Setup_UUIE & pdu)
{
  H225_FeatureSet fs;
  PBoolean hasFeaturePDU = false;
  
  if(pdu.HasOptionalField(H225_Setup_UUIE::e_neededFeatures)) {
    fs.IncludeOptionalField(H225_FeatureSet::e_neededFeatures);
    H225_ArrayOf_FeatureDescriptor & fsn = fs.m_neededFeatures;
    fsn = pdu.m_neededFeatures;
    hasFeaturePDU = true;
  }
  
  if(pdu.HasOptionalField(H225_Setup_UUIE::e_desiredFeatures)) {
    fs.IncludeOptionalField(H225_FeatureSet::e_desiredFeatures);
    H225_ArrayOf_FeatureDescriptor & fsn = fs.m_desiredFeatures;
    fsn = pdu.m_desiredFeatures;
    hasFeaturePDU = true;
  }
  
  if(pdu.HasOptionalField(H225_Setup_UUIE::e_supportedFeatures)) {
    fs.IncludeOptionalField(H225_FeatureSet::e_supportedFeatures);
    H225_ArrayOf_FeatureDescriptor & fsn = fs.m_supportedFeatures;
    fsn = pdu.m_supportedFeatures;
    hasFeaturePDU = true;
  }
  
  if (hasFeaturePDU) {
      connection->OnReceiveFeatureSet(H460_MessageType::e_setup, fs);
  }
}


template <typename PDUType>
static void ReceiveFeatureData(const H323Connection * connection, unsigned code, const PDUType & pdu)
{
  if (pdu.m_h323_uu_pdu.HasOptionalField(H225_H323_UU_PDU::e_genericData)) {
    H225_FeatureSet fs;
    fs.IncludeOptionalField(H225_FeatureSet::e_supportedFeatures);
    H225_ArrayOf_FeatureDescriptor & fsn = fs.m_supportedFeatures;
    const H225_ArrayOf_GenericData & data = pdu.m_h323_uu_pdu.m_genericData;
    for (PINDEX i=0; i < data.GetSize(); i++) {
      PINDEX lastPos = fsn.GetSize();
      fsn.SetSize(lastPos+1);
      fsn[lastPos] = (H225_FeatureDescriptor &)data[i];
    }
    connection->OnReceiveFeatureSet(code, fs);
  }
}


template <typename PDUType>
static void ReceiveFeatureSet(const H323Connection * connection, unsigned code, const PDUType & pdu)
{
    if (pdu.HasOptionalField(PDUType::e_featureSet))
      connection->OnReceiveFeatureSet(code, pdu.m_featureSet);
}
#endif


#if OPAL_H460
const char * H46018OID = "0.0.8.460.18.0.1";
const char * H46019OID = "0.0.8.460.19.0.1";
const unsigned defH46019payload = 127;
const unsigned defH46019TTL = 20;

const char * H46024AOID = "0.0.8.460.24.1";
const char * H46024BOID = "0.0.8.460.24.2";
#endif


H323Connection::H323Connection(OpalCall & call,
                               H323EndPoint & ep,
                               const PString & token,
                               const PString & alias,
                               const H323TransportAddress & address,
                               unsigned options,
                               OpalConnection::StringOptions * stringOptions)
  : OpalRTPConnection(call, ep, token, options, stringOptions)
  , endpoint(ep)
  , m_remoteConnectAddress(address)
  , remoteCallWaiting(-1)
  , gatekeeperRouted(false)
  , distinctiveRing(0)
  , callReference(token.Mid(token.Find('/')+1).AsUnsigned())
  , m_progressIndicator(0)
  , remoteMaxAudioDelayJitter(0)
  , uuiesRequested(0) // Empty set
  , gkAccessTokenOID(ep.GetGkAccessTokenOID())
  , addAccessTokenToSetup(true) // Automatic inclusion of ACF access token in SETUP
  , signallingChannel(NULL)
  , controlChannel(NULL)
  , controlListener(NULL)
  , h245TunnelRxPDU(NULL)
  , h245TunnelTxPDU(NULL)
  , setupPDU(NULL)
  , alertingPDU(NULL)
  , connectPDU(NULL)
  , progressPDU(NULL)
  , connectionState(NoConnectionActive)
  , h225version(H225_PROTOCOL_VERSION)
  , h245version(H245_PROTOCOL_VERSION)
  , h245versionSet(false)
  , lastPDUWasH245inSETUP(false)
  , mustSendDRQ(false)
  , mediaWaitForConnect(false)
  , transmitterSidePaused(false)
  , remoteTransmitPaused(false)
  , earlyStart(false)
  , endSessionSent(false)
  , endSessionNeeded(false)
  , holdMediaChannel(NULL)
  , isConsultationTransfer(false)
#if OPAL_H450
  , isCallIntrusion(false)
  , callIntrusionProtectionLevel(endpoint.GetCallIntrusionProtectionLevel())
#endif
#if OPAL_H239
  , m_h239Control(ep.GetDefaultH239Control())
#endif
#if OPAL_H460
  , features(ep.GetFeatureSet())
#endif
{
  if (alias.IsEmpty())
    remotePartyName = remotePartyAddress = address;
  else {
    remotePartyName = alias;
    remotePartyAddress = alias + '@' + address;
  }

  /* Add the local alias name in the ARQ, TODO: overwrite alias name from partyB */
  localAliasNames = ep.GetAliasNames();
  
  switch (options&H245TunnelingOptionMask) {
    case H245TunnelingOptionDisable :
      h245Tunneling = false;
      break;

    case H245TunnelingOptionEnable :
      h245Tunneling = true;
      break;

    default :
      h245Tunneling = !ep.IsH245TunnelingDisabled();
      break;
  }

  switch (options&FastStartOptionMask) {
    case FastStartOptionDisable :
      fastStartState = FastStartDisabled;
      break;

    case FastStartOptionEnable :
      fastStartState = FastStartInitiate;
      break;

    default :
      fastStartState = ep.IsFastStartDisabled() ? FastStartDisabled : FastStartInitiate;
      break;
  }

  switch (options&H245inSetupOptionMask) {
    case H245inSetupOptionDisable :
      doH245inSETUP = false;
      break;

    case H245inSetupOptionEnable :
      doH245inSETUP = true;
      break;

    default :
      doH245inSETUP = !ep.IsH245inSetupDisabled();
      break;
  }

  masterSlaveDeterminationProcedure = new H245NegMasterSlaveDetermination(endpoint, *this);
  capabilityExchangeProcedure = new H245NegTerminalCapabilitySet(endpoint, *this);
  logicalChannels = new H245NegLogicalChannels(endpoint, *this);
  requestModeProcedure = new H245NegRequestMode(endpoint, *this);
  roundTripDelayProcedure = new H245NegRoundTripDelay(endpoint, *this);

#if OPAL_H450
  h450dispatcher = new H450xDispatcher(*this);
  h4502handler = new H4502Handler(*this, *h450dispatcher);
  h4504handler = new H4504Handler(*this, *h450dispatcher);
  h4506handler = new H4506Handler(*this, *h450dispatcher);
  h4507handler = new H4507Handler(*this, *h450dispatcher);
  h45011handler = new H45011Handler(*this, *h450dispatcher);
#endif


  NATsupport = true;

#if OPAL_H460
  disableH460 = ep.FeatureSetDisabled();
  features->LoadFeatureSet(H460_Feature::FeatureSignal, this);

  m_H46019CallReceiver = false;
  m_H46019enabled = false;
  m_h245Connect = false; 

  // NOTE additional 46024a stuff not yet copied in
#endif
}


H323Connection::~H323Connection()
{
  delete masterSlaveDeterminationProcedure;
  delete capabilityExchangeProcedure;
  delete logicalChannels;
  delete requestModeProcedure;
  delete roundTripDelayProcedure;
#if OPAL_H450
  delete h450dispatcher;
#endif
  delete signallingChannel;
  delete controlChannel;
  delete setupPDU;
  delete alertingPDU;
  delete connectPDU;
  delete progressPDU;
  delete holdMediaChannel;
#if OPAL_H460
  delete features;
#endif
  delete controlListener;

  PTRACE(4, "H323\tConnection " << callToken << " deleted.");
}


void H323Connection::OnApplyStringOptions()
{
  OpalConnection::OnApplyStringOptions();

  if (LockReadWrite()) {
    PString str = m_stringOptions(OPAL_OPT_CALL_IDENTIFIER);
    if (!str.IsEmpty())
      callIdentifier = PGloballyUniqueID(str);
    UnlockReadWrite();
  }
}


void H323Connection::OnReleased()
{
  PTRACE(4, "H323\tOnReleased: " << callToken << ", connectionState=" << connectionState);

  connectionState = ShuttingDownConnection;

  PTRACE(3, "H225\tSending release complete PDU: callRef=" << callReference);
  H323SignalPDU rcPDU;
  rcPDU.BuildReleaseComplete(*this);
#if OPAL_H450
  h450dispatcher->AttachToReleaseComplete(rcPDU);
#endif

  PBoolean sendingReleaseComplete = OnSendReleaseComplete(rcPDU);

  if (endSessionNeeded) {
    if (sendingReleaseComplete)
      h245TunnelTxPDU = &rcPDU; // Piggy back H245 on this reply

    // Send an H.245 end session to the remote endpoint.
    H323ControlPDU pdu;
    pdu.BuildEndSessionCommand(H245_EndSessionCommand::e_disconnect);
    WriteControlPDU(pdu);
  }

  if (sendingReleaseComplete) {
    h245TunnelTxPDU = NULL;
    WriteSignalPDU(rcPDU);
  }

  // Check for gatekeeper and do disengage if have one
  if (mustSendDRQ) {
    H323Gatekeeper * gatekeeper = endpoint.GetGatekeeper();
    if (gatekeeper != NULL)
      gatekeeper->DisengageRequest(*this, H225_DisengageReason::e_normalDrop);
  }

  // Unblock sync points
  digitsWaitFlag.Signal();

  if (LockReadWrite()) {
    // Clean up any fast start "pending" channels we may have running.
    for (H323LogicalChannelList::iterator channel = fastStartChannels.begin(); channel != fastStartChannels.end(); ++channel)
      channel->Close();
    fastStartChannels.RemoveAll();

    // Dispose of all the logical channels
    logicalChannels->RemoveAll();

    UnlockReadWrite();
  }

  if (endSessionNeeded) {
    // Calculate time since we sent the end session command so we do not actually
    // wait for returned endSession if it has already been that long
    PTimeInterval waitTime = endpoint.GetEndSessionTimeout();
    if (GetConnectionEndTime().IsValid()) {
      PTime now;
      if (now > GetConnectionEndTime()) { // Allow for backward motion in time (DST change)
        waitTime -= now - GetConnectionEndTime();
        if (waitTime < 0)
          waitTime = 0;
      }
    }

    // Wait a while for the remote to send an endSession
    PTRACE(4, "H323\tAwaiting end session from remote for " << waitTime << " seconds");
    if (!endSessionReceived.Wait(waitTime)) {
      PTRACE(2, "H323\tTimed out waiting for end session from remote.");
    }
  }

  // Wait for control channel to be cleaned up (thread ended).
  if (controlChannel != NULL)
    controlChannel->CloseWait();

  // Wait for signalling channel to be cleaned up (thread ended).
  if (signallingChannel != NULL)
    signallingChannel->CloseWait();

  OpalRTPConnection::OnReleased();
}


PString H323Connection::GetDestinationAddress()
{
  if (!localDestinationAddress)
    return localDestinationAddress;

  return OpalRTPConnection::GetDestinationAddress();
}


PString H323Connection::GetAlertingType() const
{
  return psprintf("%u", distinctiveRing);
}


bool H323Connection::SetAlertingType(const PString & info)
{
  if (!isdigit(info[0]))
    return false;

  unsigned value = info.AsUnsigned();
  if (value > 7)
    return false;

  distinctiveRing = value;
  return true;
}


void H323Connection::AttachSignalChannel(const PString & token,
                                         H323Transport * channel,
                                         PBoolean answeringCall)
{
  if (!answeringCall)
    InternalSetAsOriginating();

  if (signallingChannel != NULL && signallingChannel->IsOpen()) {
    PAssertAlways(PLogicError);
    return;
  }

  delete signallingChannel;
  signallingChannel = channel;

  // Set our call token for identification in endpoint dictionary
  callToken = token;
}


PBoolean H323Connection::WriteSignalPDU(H323SignalPDU & pdu)
{
  PAssert(signallingChannel != NULL, PLogicError);

  lastPDUWasH245inSETUP = false;

  if (signallingChannel != NULL && signallingChannel->IsOpen()) {
    pdu.m_h323_uu_pdu.m_h245Tunneling = h245Tunneling;

    H323Gatekeeper * gk = endpoint.GetGatekeeper();
    if (gk != NULL)
      gk->InfoRequestResponse(*this, pdu.m_h323_uu_pdu, true);

    pdu.SetQ931Fields(*this); // Make sure display name is the final value

    if (pdu.Write(*signallingChannel))
      return true;
  }

  Release(EndedByTransportFail);
  return false;
}


void H323Connection::HandleSignallingChannel()
{
  PAssert(signallingChannel != NULL, PLogicError);

  PTRACE(3, "H225\tReading PDUs: callRef=" << callReference);

  while (signallingChannel->IsOpen()) {
    H323SignalPDU pdu;
    if (pdu.Read(*signallingChannel)) {
      if (!HandleSignalPDU(pdu)) {
        Release(EndedByTransportFail);
        break;
      }
    }
    else if (signallingChannel->GetErrorCode() != PChannel::Timeout) {
      if (controlChannel == NULL || !controlChannel->IsOpen())
        Release(EndedByTransportFail);
      signallingChannel->Close();
      break;
    }
    else {
      switch (connectionState) {
        case AwaitingSignalConnect :
          // Had time out waiting for remote to send a CONNECT
          ClearCall(EndedByNoAnswer);
          break;
        case HasExecutedSignalConnect :
          // Have had minimum MonitorCallStatusTime delay since CONNECT but
          // still no media to move it to EstablishedConnection state. Must
          // thus not have any common codecs to use!
          ClearCall(EndedByCapabilityExchange);
          break;
        default :
          break;
      }
    }

    if (controlChannel == NULL)
      MonitorCallStatus();
  }

  // If we are the only link to the far end then indicate that we have
  // received endSession even if we hadn't, because we are now never going
  // to get one so there is no point in having CleanUpOnCallEnd wait.
  if (controlChannel == NULL)
    endSessionReceived.Signal();

  PTRACE(3, "H225\tSignal channel closed.");
}


PBoolean H323Connection::HandleSignalPDU(H323SignalPDU & pdu)
{
  // Process the PDU.
  const Q931 & q931 = pdu.GetQ931();

  PTRACE(3, "H225\tHandling PDU: " << q931.GetMessageTypeName()
                    << " callRef=" << q931.GetCallReference());

  PSafeLockReadWrite safeLock(*this);
  if (!safeLock.IsLocked())
    return false;

  if (IsReleased()) {
    // Continue to look for endSession/releaseComplete pdus
    if (pdu.m_h323_uu_pdu.m_h245Tunneling) {
      for (PINDEX i = 0; i < pdu.m_h323_uu_pdu.m_h245Control.GetSize(); i++) {
        PPER_Stream strm = pdu.m_h323_uu_pdu.m_h245Control[i].GetValue();
        if (!InternalEndSessionCheck(strm))
          break;
      }
    }
    if (q931.GetMessageType() == Q931::ReleaseCompleteMsg)
      endSessionReceived.Signal();
    return false;
  }

  // If remote does not do tunneling, so we don't either. Note that if it
  // gets turned off once, it stays off for good.
  if (h245Tunneling && !pdu.m_h323_uu_pdu.m_h245Tunneling && pdu.GetQ931().HasIE(Q931::UserUserIE)) {
    masterSlaveDeterminationProcedure->Stop();
    capabilityExchangeProcedure->Stop();
    h245Tunneling = false;
  }

  h245TunnelRxPDU = &pdu;

  // Check for presence of supplementary services
#if OPAL_H450
  if (pdu.m_h323_uu_pdu.HasOptionalField(H225_H323_UU_PDU::e_h4501SupplementaryService)) {
    if (!h450dispatcher->HandlePDU(pdu)) { // Process H4501SupplementaryService APDU
      return false;
    }
  }
#endif

#if OPAL_H460
   ReceiveFeatureData<H323SignalPDU>(this, q931.GetMessageType(), pdu);
#endif

  // Add special code to detect if call is from a Cisco and remoteApplication needs setting
  if (remoteProductInfo.name.IsEmpty() && pdu.m_h323_uu_pdu.HasOptionalField(H225_H323_UU_PDU::e_nonStandardControl)) {
    for (PINDEX i = 0; i < pdu.m_h323_uu_pdu.m_nonStandardControl.GetSize(); i++) {
      const H225_NonStandardIdentifier & id = pdu.m_h323_uu_pdu.m_nonStandardControl[i].m_nonStandardIdentifier;
      if (id.GetTag() == H225_NonStandardIdentifier::e_h221NonStandard) {
        const H225_H221NonStandard & h221 = id;
        if (h221.m_t35CountryCode == 181 && h221.m_t35Extension == 0 && h221.m_manufacturerCode == 18) {
          remoteProductInfo.name = "Cisco IOS";
          remoteProductInfo.version = "12.x";
          remoteProductInfo.t35CountryCode = 181;
          remoteProductInfo.manufacturerCode = 18;
          PTRACE(3, "H225\tSet remote application name: \"" << GetRemoteApplication() << '"');
          break;
        }
      }
    }
  }

  pdu.GetQ931().GetProgressIndicator(m_progressIndicator);

  PBoolean ok;
  switch (q931.GetMessageType()) {
    case Q931::SetupMsg :
      ok = OnReceivedSignalSetup(pdu);
      break;

    case Q931::CallProceedingMsg :
      ok = OnReceivedCallProceeding(pdu);
      break;

    case Q931::ProgressMsg :
      ok = OnReceivedProgress(pdu);
      break;

    case Q931::AlertingMsg :
      ok = OnReceivedAlerting(pdu);
      break;

    case Q931::ConnectMsg :
      ok = OnReceivedSignalConnect(pdu);
      break;

    case Q931::FacilityMsg :
      ok = OnReceivedFacility(pdu);
      break;

    case Q931::SetupAckMsg :
      ok = OnReceivedSignalSetupAck(pdu);
      break;

    case Q931::InformationMsg :
      ok = OnReceivedSignalInformation(pdu);
      break;

    case Q931::NotifyMsg :
      ok = OnReceivedSignalNotify(pdu);
      break;

    case Q931::StatusMsg :
      ok = OnReceivedSignalStatus(pdu);
      break;

    case Q931::StatusEnquiryMsg :
      ok = OnReceivedStatusEnquiry(pdu);
      break;

    case Q931::ReleaseCompleteMsg :
      OnReceivedReleaseComplete(pdu);
      ok = false;
      break;

    default :
      ok = OnUnknownSignalPDU(pdu);
  }

  if (ok) {
    // Process tunnelled H245 PDU, if present.
    HandleTunnelPDU(NULL);

    // Check for establishment criteria met
    InternalEstablishedConnectionCheck();
  }

  h245TunnelRxPDU = NULL;

  PString digits = pdu.GetQ931().GetKeypad();
  if (!digits)
    OnUserInputString(digits);

  H323Gatekeeper * gk = endpoint.GetGatekeeper();
  if (gk != NULL)
    gk->InfoRequestResponse(*this, pdu.m_h323_uu_pdu, false);

  return ok;
}


void H323Connection::HandleTunnelPDU(H323SignalPDU * txPDU)
{
  if (h245TunnelRxPDU == NULL || !h245TunnelRxPDU->m_h323_uu_pdu.m_h245Tunneling)
    return;

  if (!h245Tunneling && h245TunnelRxPDU->m_h323_uu_pdu.m_h323_message_body.GetTag() == H225_H323_UU_PDU_h323_message_body::e_setup)
    return;

  H323SignalPDU localTunnelPDU;
  if (txPDU != NULL)
    h245TunnelTxPDU = txPDU;
  else {
    /* Compensate for Cisco bug. IOS cannot seem to accept multiple tunnelled
       H.245 PDUs insode the same facility message */
    if (GetRemoteApplication().Find("Cisco IOS") == P_MAX_INDEX) {
      // Not Cisco, so OK to tunnel multiple PDUs
      localTunnelPDU.BuildFacility(*this, true);
      h245TunnelTxPDU = &localTunnelPDU;
    }
  }

  // if a response to a SETUP PDU containing TCS/MSD was ignored, then shutdown negotiations
  PINDEX i;
  if (lastPDUWasH245inSETUP && 
      (h245TunnelRxPDU->m_h323_uu_pdu.m_h245Control.GetSize() == 0) &&
      (h245TunnelRxPDU->GetQ931().GetMessageType() != Q931::CallProceedingMsg)) {
    PTRACE(4, "H225\tH.245 in SETUP ignored - resetting H.245 negotiations");
    masterSlaveDeterminationProcedure->Stop();
    lastPDUWasH245inSETUP = false;
    capabilityExchangeProcedure->Stop(true);
  } else {
    for (i = 0; i < h245TunnelRxPDU->m_h323_uu_pdu.m_h245Control.GetSize(); i++) {
      PPER_Stream strm = h245TunnelRxPDU->m_h323_uu_pdu.m_h245Control[i].GetValue();
      HandleControlData(strm);
    }
  }

  // Make sure does not get repeated, clear tunnelled H.245 PDU's
  h245TunnelRxPDU->m_h323_uu_pdu.m_h245Control.SetSize(0);

  if (h245TunnelRxPDU->m_h323_uu_pdu.m_h323_message_body.GetTag() == H225_H323_UU_PDU_h323_message_body::e_setup) {
    H225_Setup_UUIE & setup = h245TunnelRxPDU->m_h323_uu_pdu.m_h323_message_body;

    if (doH245inSETUP && setup.HasOptionalField(H225_Setup_UUIE::e_parallelH245Control)) {
      for (i = 0; i < setup.m_parallelH245Control.GetSize(); i++) {
        PPER_Stream strm = setup.m_parallelH245Control[i].GetValue();
        HandleControlData(strm);
      }

      // Make sure does not get repeated, clear tunnelled H.245 PDU's
      setup.m_parallelH245Control.SetSize(0);
    }
  }

  h245TunnelTxPDU = NULL;

  // If had replies, then send them off in their own packet
  if (txPDU == NULL && localTunnelPDU.m_h323_uu_pdu.m_h245Control.GetSize() > 0)
    WriteSignalPDU(localTunnelPDU);
}


static PBoolean BuildFastStartList(const H323Channel & channel,
                               H225_ArrayOf_PASN_OctetString & array,
                               H323Channel::Directions reverseDirection)
{
  H245_OpenLogicalChannel open;
  const H323Capability & capability = channel.GetCapability();

  if (channel.GetDirection() != reverseDirection) {
    if (!capability.OnSendingPDU(open.m_forwardLogicalChannelParameters.m_dataType))
      return false;
  }
  else {
    if (!capability.OnSendingPDU(open.m_reverseLogicalChannelParameters.m_dataType))
      return false;

    open.m_forwardLogicalChannelParameters.m_multiplexParameters.SetTag(
                H245_OpenLogicalChannel_forwardLogicalChannelParameters_multiplexParameters::e_none);
    open.m_forwardLogicalChannelParameters.m_dataType.SetTag(H245_DataType::e_nullData);
    open.IncludeOptionalField(H245_OpenLogicalChannel::e_reverseLogicalChannelParameters);
  }

  if (!channel.OnSendingPDU(open))
    return false;

  PTRACE(4, "H225\tBuild fastStart:\n  " << setprecision(2) << open);
  PINDEX last = array.GetSize();
  array.SetSize(last+1);
  array[last].EncodeSubType(open);

  PTRACE(3, "H225\tBuilt fastStart for " << capability);
  return true;
}

void H323Connection::OnEstablished()
{
  endpoint.OnConnectionEstablished(*this, callToken);
  OpalRTPConnection::OnEstablished();
}

void H323Connection::OnSendARQ(H225_AdmissionRequest & arq)
{
#if OPAL_H460
  H225_FeatureSet fs;
  if (OnSendFeatureSet(H460_MessageType::e_admissionRequest, fs)) {
    if (fs.HasOptionalField(H225_FeatureSet::e_supportedFeatures)) {
      arq.IncludeOptionalField(H225_AdmissionRequest::e_genericData);

      H225_ArrayOf_FeatureDescriptor & fsn = fs.m_supportedFeatures;
      H225_ArrayOf_GenericData & data = arq.m_genericData;

      for (PINDEX i=0; i < fsn.GetSize(); i++) {
        PINDEX lastPos = data.GetSize();
        data.SetSize(lastPos+1);
        data[lastPos] = fsn[i];
      }
    }
  }
#endif

  endpoint.OnSendARQ(*this, arq);
}


void H323Connection::OnReceivedACF(const H225_AdmissionConfirm & acf)
{
#if OPAL_H460
  if (acf.HasOptionalField(H225_AdmissionConfirm::e_genericData)) {
    const H225_ArrayOf_GenericData & data = acf.m_genericData;

    if (data.GetSize() > 0) {
      H225_FeatureSet fs;
      fs.IncludeOptionalField(H225_FeatureSet::e_supportedFeatures);
      H225_ArrayOf_FeatureDescriptor & fsn = fs.m_supportedFeatures;
      fsn.SetSize(data.GetSize());
      for (PINDEX i=0; i < data.GetSize(); i++) 
        fsn[i] = (H225_FeatureDescriptor &)data[i];

      OnReceiveFeatureSet(H460_MessageType::e_admissionConfirm, fs);
    }
  }
#endif
}


void H323Connection::OnReceivedARJ(const H225_AdmissionReject & arj)
{
#if OPAL_H460
  if (arj.HasOptionalField(H225_AdmissionReject::e_genericData)) {
    const H225_ArrayOf_GenericData & data = arj.m_genericData;

    if (data.GetSize() > 0) {
      H225_FeatureSet fs;
      fs.IncludeOptionalField(H225_FeatureSet::e_supportedFeatures);
      H225_ArrayOf_FeatureDescriptor & fsn = fs.m_supportedFeatures;
      fsn.SetSize(data.GetSize());
      for (PINDEX i=0; i < data.GetSize(); i++) 
        fsn[i] = (H225_FeatureDescriptor &)data[i];

      OnReceiveFeatureSet(H460_MessageType::e_admissionReject, fs);
    }
  }
#endif
}


void H323Connection::OnSendIRR(H225_InfoRequestResponse & irr) const
{
#if OPAL_H460
  H225_FeatureSet fs;
  if (OnSendFeatureSet(H460_MessageType::e_inforequestresponse, fs)) {
    if (fs.HasOptionalField(H225_FeatureSet::e_supportedFeatures)) {
      irr.IncludeOptionalField(H225_InfoRequestResponse::e_genericData);

      H225_ArrayOf_FeatureDescriptor & fsn = fs.m_supportedFeatures;
      H225_ArrayOf_GenericData & data = irr.m_genericData;

      for (PINDEX i=0; i < fsn.GetSize(); i++) {
        PINDEX lastPos = data.GetSize();
        data.SetSize(lastPos+1);
        data[lastPos] = fsn[i];
      }
    }
  }
#endif
}


void H323Connection::OnSendDRQ(H225_DisengageRequest & drq) const
{
#if OPAL_H460
  H225_FeatureSet fs;
  if (OnSendFeatureSet(H460_MessageType::e_disengagerequest, fs)) {
    if (fs.HasOptionalField(H225_FeatureSet::e_supportedFeatures)) {
      drq.IncludeOptionalField(H225_DisengageRequest::e_genericData);

      H225_ArrayOf_FeatureDescriptor & fsn = fs.m_supportedFeatures;
      H225_ArrayOf_GenericData & data = drq.m_genericData;

      for (PINDEX i=0; i < fsn.GetSize(); i++) {
        PINDEX lastPos = data.GetSize();
        data.SetSize(lastPos+1);
        data[lastPos] = fsn[i];
      }
    }
  }
#endif
}


void H323Connection::SetRemoteVersions(const H225_ProtocolIdentifier & protocolIdentifier)
{
  if (protocolIdentifier.GetSize() < 6)
    return;

  h225version = protocolIdentifier[5];

  if (h245versionSet) {
    PTRACE(3, "H225\tSet protocol version to " << h225version);
    return;
  }

  // If has not been told explicitly what the H.245 version use, make an
  // assumption based on the H.225 version
  switch (h225version) {
    case 1 :
      h245version = 2;  // H.323 version 1
      break;
    case 2 :
      h245version = 3;  // H.323 version 2
      break;
    case 3 :
      h245version = 5;  // H.323 version 3
      break;
    case 4 :
      h245version = 7;  // H.323 version 4
      break;
    case 5 :
      h245version = 9;  // H.323 version 5 
      break;
    default:
      h245version = 13; // H.323 version 6
      break;
  }
  PTRACE(3, "H225\tSet protocol version to " << h225version
         << " and implying H.245 version " << h245version);
}


PBoolean H323Connection::OnReceivedSignalSetup(const H323SignalPDU & originalSetupPDU)
{
  if (originalSetupPDU.m_h323_uu_pdu.m_h323_message_body.GetTag() != H225_H323_UU_PDU_h323_message_body::e_setup)
    return false;

  SetPhase(SetUpPhase);

  setupPDU = new H323SignalPDU(originalSetupPDU);

  H225_Setup_UUIE & setup = setupPDU->m_h323_uu_pdu.m_h323_message_body;

  switch (setup.m_conferenceGoal.GetTag()) {
    case H225_Setup_UUIE_conferenceGoal::e_create:
      m_conferenceGoal = e_Create;
      break;

    case H225_Setup_UUIE_conferenceGoal::e_join:
      m_conferenceGoal = e_Join;
      break;

    case H225_Setup_UUIE_conferenceGoal::e_invite:
      m_conferenceGoal= e_Invite;
      break;

    case H225_Setup_UUIE_conferenceGoal::e_callIndependentSupplementaryService:
      return endpoint.OnCallIndependentSupplementaryService(*setupPDU);

    case H225_Setup_UUIE_conferenceGoal::e_capability_negotiation:
      return endpoint.OnNegotiateConferenceCapabilities(*setupPDU);
  }

  SetRemoteVersions(setup.m_protocolIdentifier);

  // Get the ring pattern
  distinctiveRing = setupPDU->GetDistinctiveRing();

  // Save the identifiers sent by caller
  if (setup.HasOptionalField(H225_Setup_UUIE::e_callIdentifier))
    callIdentifier = setup.m_callIdentifier.m_guid;
  conferenceIdentifier = setup.m_conferenceID;
  SetRemoteApplication(setup.m_sourceInfo);

  // Determine the remote parties name/number/address as best we can
  if (!setupPDU->GetQ931().GetCallingPartyNumber(remotePartyNumber))
    remotePartyNumber = H323GetAliasAddressE164(setup.m_sourceAddress);
  remotePartyName = setupPDU->GetSourceAliases(signallingChannel);

  // get the destination number and name, just in case we are a gateway
  setupPDU->GetQ931().GetCalledPartyNumber(m_calledPartyNumber);
  if (m_calledPartyNumber.IsEmpty())
    m_calledPartyNumber = H323GetAliasAddressE164(setup.m_destinationAddress);

  for (PINDEX i = 0; i < setup.m_destinationAddress.GetSize(); ++i) {
    PString addr = H323GetAliasAddressString(setup.m_destinationAddress[i]);
    if (addr != m_calledPartyNumber) {
      m_calledPartyName = addr;
      break;
    }
  }

  setupPDU->GetQ931().GetRedirectingNumber(m_redirectingParty);

  // get the peer address
  remotePartyAddress = signallingChannel->GetRemoteAddress();
  if (setup.m_sourceAddress.GetSize() > 0)
    remotePartyAddress = H323GetAliasAddressString(setup.m_sourceAddress[0]) + '@' + signallingChannel->GetRemoteAddress();

  // compare the source call signalling address
  if (setup.HasOptionalField(H225_Setup_UUIE::e_sourceCallSignalAddress)) {

    // get the address that remote end *thinks* it is using from the sourceCallSignalAddress field
    PIPSocket::Address sigAddr;
    {
      H323TransportAddress sigAddress(setup.m_sourceCallSignalAddress);
      sigAddress.GetIpAddress(sigAddr);
    }

    // get the local and peer transport addresses
    PIPSocket::Address peerAddr, localAddr;
    signallingChannel->GetRemoteAddress().GetIpAddress(peerAddr);
    signallingChannel->GetLocalAddress().GetIpAddress(localAddr);

    // allow the application to determine if RTP NAT is enabled or not
    remoteIsNAT = IsRTPNATEnabled(localAddr, peerAddr, sigAddr, true);
  }

  // Anything else we need from setup PDU
  mediaWaitForConnect = setup.m_mediaWaitForConnect;
  if (!setupPDU->GetQ931().GetCalledPartyNumber(localDestinationAddress)) {
    localDestinationAddress = setupPDU->GetDestinationAlias(true);
    if (signallingChannel->GetLocalAddress().IsEquivalent(localDestinationAddress))
      localDestinationAddress = '*';
  }
  
#if OPAL_H460
  ReceiveSetupFeatureSet(this, setup);
#endif

  // Send back a H323 Call Proceeding PDU in case OnIncomingCall() takes a while
  PTRACE(3, "H225\tSending call proceeding PDU");
  H323SignalPDU callProceedingPDU;
  H225_CallProceeding_UUIE & callProceeding = callProceedingPDU.BuildCallProceeding(*this);

  if (!isConsultationTransfer) {
    if (OnSendCallProceeding(callProceedingPDU)) {
      if (fastStartState == FastStartDisabled)
        callProceeding.IncludeOptionalField(H225_CallProceeding_UUIE::e_fastConnectRefused);

      if (!WriteSignalPDU(callProceedingPDU))
        return false;

      if (GetPhase() < ProceedingPhase) {
        SetPhase(ProceedingPhase);
        OnProceeding();
      }
    }

    /** Here is a spot where we should wait in case of Call Intrusion
    for CIPL from other endpoints 
    if (isCallIntrusion) return true;
    */

    // if the application indicates not to contine, then send a Q931 Release Complete PDU
    alertingPDU = new H323SignalPDU;
    alertingPDU->BuildAlerting(*this);

    /** If we have a case of incoming call intrusion we should not Clear the Call*/
    if (!OnIncomingCall(*setupPDU, *alertingPDU)
#if OPAL_H450
        && !isCallIntrusion
#endif
        ) {
      Release(EndedByNoAccept);
      PTRACE(2, "H225\tApplication not accepting calls");
      return false;
    }
    if (IsReleased()) {
      PTRACE(1, "H225\tApplication called ClearCall during OnIncomingCall");
      return false;
    }

    // send Q931 Alerting PDU
    PTRACE(3, "H225\tIncoming call accepted");

    // Check for gatekeeper and do admission check if have one
    H323Gatekeeper * gatekeeper = endpoint.GetGatekeeper();
    if (gatekeeper != NULL) {
      H225_ArrayOf_AliasAddress destExtraCallInfoArray;
      H323Gatekeeper::AdmissionResponse response;
      response.destExtraCallInfo = &destExtraCallInfoArray;
      if (!gatekeeper->AdmissionRequest(*this, response)) {
        PTRACE(2, "H225\tGatekeeper refused admission: "
               << (response.rejectReason == UINT_MAX
                    ? PString("Transport error")
                    : H225_AdmissionRejectReason(response.rejectReason).GetTagName()));
        switch (response.rejectReason) {
          case H225_AdmissionRejectReason::e_calledPartyNotRegistered :
            Release(EndedByNoUser);
            break;
          case H225_AdmissionRejectReason::e_requestDenied :
            Release(EndedByNoBandwidth);
            break;
          case H225_AdmissionRejectReason::e_invalidPermission :
          case H225_AdmissionRejectReason::e_securityDenial :
            ClearCall(EndedBySecurityDenial);
            break;
          case H225_AdmissionRejectReason::e_resourceUnavailable :
            Release(EndedByRemoteBusy);
            break;
          default :
            Release(EndedByGkAdmissionFailed);
        }
        return false;
      }

      if (destExtraCallInfoArray.GetSize() > 0)
        destExtraCallInfo = H323GetAliasAddressString(destExtraCallInfoArray[0]);
      mustSendDRQ = true;
      gatekeeperRouted = response.gatekeeperRouted;
    }
  }

  OnApplyStringOptions();

  // Get the local capabilities before fast start or tunnelled TCS is handled
  OnSetLocalCapabilities();

  // DO this handle in case a TCS is in the SETUP packet
  HandleTunnelPDU(NULL);

  if (fastStartState != FastStartDisabled && setup.HasOptionalField(H225_Setup_UUIE::e_fastStart)) {
    PTRACE(3, "H225\tFast start detected");

    fastStartState = FastStartDisabled;

    // If we have not received caps from remote, we are going to build a
    // fake one from the fast connect data.
    if (!capabilityExchangeProcedure->HasReceivedCapabilities())
      remoteCapabilities.RemoveAll();

    // Extract capabilities from the fast start OpenLogicalChannel structures
    PINDEX i;
    for (i = 0; i < setup.m_fastStart.GetSize(); i++) {
      H245_OpenLogicalChannel open;
      if (setup.m_fastStart[i].DecodeSubType(open)) {
        PTRACE(4, "H225\tFast start open:\n  " << setprecision(2) << open);
        const H245_DataType * dataType = NULL;
        if (open.HasOptionalField(H245_OpenLogicalChannel::e_reverseLogicalChannelParameters)) {
          if (open.m_reverseLogicalChannelParameters.m_multiplexParameters.GetTag() ==
                H245_OpenLogicalChannel_reverseLogicalChannelParameters_multiplexParameters::e_h2250LogicalChannelParameters)
            dataType = &open.m_reverseLogicalChannelParameters.m_dataType;
        }
        else {
          if (open.m_forwardLogicalChannelParameters.m_multiplexParameters.GetTag() ==
              H245_OpenLogicalChannel_forwardLogicalChannelParameters_multiplexParameters::e_h2250LogicalChannelParameters)
            dataType = &open.m_forwardLogicalChannelParameters.m_dataType;
        }
        if (dataType != NULL) {
          H323Capability * capability = remoteCapabilities.FindCapability(*dataType);
          if (capability == NULL &&
                      !capabilityExchangeProcedure->HasReceivedCapabilities() &&
                      (capability = localCapabilities.FindCapability(*dataType)) != NULL) {
            // If we actually have the remote capabilities then the remote (very oddly)
            // had a fast connect entry it could not do. If we have not yet got a remote
            // cap table then build one using all possible caps.
            capability = remoteCapabilities.Copy(*capability);
            remoteCapabilities.SetCapability(0, capability->GetDefaultSessionID(), capability);
          }
          if (capability != NULL) {
            unsigned error;
            H323Channel * channel = CreateLogicalChannel(open, true, error);
            if (channel != NULL) {
              if (channel->GetDirection() == H323Channel::IsTransmitter)
                channel->SetNumber(logicalChannels->GetNextChannelNumber());
              fastStartChannels.Append(channel);
              fastStartState = FastStartResponse;
            }
          }
        }
      }
      else {
        PTRACE(1, "H225\tInvalid fast start PDU decode:\n  " << open);
      }
    }

    PTRACE(3, "H225\tFound " << fastStartChannels.GetSize() << " fast start channels");
    PTRACE_IF(4, !capabilityExchangeProcedure->HasReceivedCapabilities(),
              "H323\tPreliminary remote capabilities generated from fast start:\n" << remoteCapabilities);
  }

  // Check that it has the H.245 channel connection info
  if (setup.HasOptionalField(H225_Setup_UUIE::e_h245Address) && (!setupPDU->m_h323_uu_pdu.m_h245Tunneling || endpoint.IsH245TunnelingDisabled()))
    if (!CreateOutgoingControlChannel(setup.m_h245Address))
      return false;

  // Build the reply with the channels we are actually using
  connectPDU = new H323SignalPDU;
  connectPDU->BuildConnect(*this);
  
  progressPDU = new H323SignalPDU;
  progressPDU->BuildProgress(*this);

  connectionState = AwaitingLocalAnswer;

  // OK are now ready to send SETUP to remote protocol
  ownerCall.OnSetUp(*this);

  if (connectionState == ShuttingDownConnection)
    return false;

  if (connectionState != AwaitingLocalAnswer)
    return true;

#if OPAL_H450
  // If Call Intrusion is allowed we must answer the call
  if (IsCallIntrusion())
    AnsweringCall(AnswerCallDeferred);
  else if (isConsultationTransfer)
    AnsweringCall(AnswerCallNow);
  else
#endif
    // call the application callback to determine if to answer the call or not
    AnsweringCall(OnAnswerCall(remotePartyName, *setupPDU, *connectPDU, *progressPDU));

  return connectionState != ShuttingDownConnection;
}


PString H323Connection::GetIdentifier() const
{
  return callIdentifier.AsString();
}


void H323Connection::SetLocalPartyName(const PString & name)
{
  if (!name.IsEmpty()) {
    OpalRTPConnection::SetLocalPartyName(name);
    localAliasNames.RemoveAll();
    localAliasNames.AppendString(name);
  }
}


void H323Connection::SetRemotePartyInfo(const H323SignalPDU & pdu)
{
  PString newNumber;
  if (pdu.GetQ931().GetCalledPartyNumber(newNumber))
    remotePartyNumber = newNumber;

  PString remoteHostName = signallingChannel->GetRemoteAddress().GetHostName();

  PString newRemotePartyName = pdu.GetQ931().GetDisplayName();
  if (newRemotePartyName.IsEmpty() || newRemotePartyName == remoteHostName)
    remotePartyName = remoteHostName;
  else
    remotePartyName = newRemotePartyName + " [" + remoteHostName + ']';

  PTRACE(3, "H225\tSet remote party name: \"" << remotePartyName << '"');
}


void H323Connection::SetRemoteApplication(const H225_EndpointType & pdu)
{
  if (pdu.HasOptionalField(H225_EndpointType::e_vendor)) {
    H323GetApplicationInfo(remoteProductInfo, pdu.m_vendor);
    PTRACE(3, "H225\tSet remote application name: \"" << GetRemoteApplication() << '"');
  }
}


PString H323Connection::GetRemotePartyURL() const
{
  PString remote;
  PINDEX j;
  
  if (IsGatekeeperRouted()) {

    remote = GetRemotePartyNumber();
  }
  
  if (remote.IsEmpty() && signallingChannel) {
  
    remote = signallingChannel->GetRemoteAddress();

    /* The address is formated the H.323 way */
    j = remote.FindLast("$");
    if (j != P_MAX_INDEX)
      remote = remote.Mid (j+1);
    j = remote.FindLast(":");
    if (j != P_MAX_INDEX)
      remote = remote.Left (j);

    if (!GetRemotePartyNumber().IsEmpty())
      remote = GetRemotePartyNumber() + "@" + remote;
  }

  remote = GetPrefixName() + ":" + remote;

  return remote;
}


PBoolean H323Connection::OnReceivedSignalSetupAck(const H323SignalPDU & /*setupackPDU*/)
{
  OnInsufficientDigits();
  return true;
}


PBoolean H323Connection::OnReceivedSignalInformation(const H323SignalPDU & /*infoPDU*/)
{
  return true;
}


PBoolean H323Connection::OnReceivedCallProceeding(const H323SignalPDU & pdu)
{
  if (pdu.m_h323_uu_pdu.m_h323_message_body.GetTag() != H225_H323_UU_PDU_h323_message_body::e_callProceeding)
    return false;
  const H225_CallProceeding_UUIE & call = pdu.m_h323_uu_pdu.m_h323_message_body;

  SetRemoteVersions(call.m_protocolIdentifier);
  SetRemotePartyInfo(pdu);
  SetRemoteApplication(call.m_destinationInfo);
  
#if OPAL_H460
  ReceiveFeatureSet<H225_CallProceeding_UUIE>(this, H460_MessageType::e_callProceeding, call);
#endif

  // Check for fastStart data and start fast
  if (call.HasOptionalField(H225_CallProceeding_UUIE::e_fastStart))
    HandleFastStartAcknowledge(call.m_fastStart);

  // Check that it has the H.245 channel connection info
  if (call.HasOptionalField(H225_CallProceeding_UUIE::e_h245Address) && (!pdu.m_h323_uu_pdu.m_h245Tunneling || endpoint.IsH245TunnelingDisabled()))
    CreateOutgoingControlChannel(call.m_h245Address);

  if (GetPhase() < ProceedingPhase) {
    SetPhase(ProceedingPhase);
    OnProceeding();
  }

  return true;
}


PBoolean H323Connection::OnReceivedProgress(const H323SignalPDU & pdu)
{
  if (pdu.m_h323_uu_pdu.m_h323_message_body.GetTag() != H225_H323_UU_PDU_h323_message_body::e_progress)
    return false;
  const H225_Progress_UUIE & progress = pdu.m_h323_uu_pdu.m_h323_message_body;

  SetRemoteVersions(progress.m_protocolIdentifier);
  SetRemotePartyInfo(pdu);
  SetRemoteApplication(progress.m_destinationInfo);

  // Check for fastStart data and start fast
  if (progress.HasOptionalField(H225_Progress_UUIE::e_fastStart))
    HandleFastStartAcknowledge(progress.m_fastStart);

  // Check that it has the H.245 channel connection info
  if (progress.HasOptionalField(H225_Progress_UUIE::e_h245Address) && (!pdu.m_h323_uu_pdu.m_h245Tunneling || endpoint.IsH245TunnelingDisabled()))
    return CreateOutgoingControlChannel(progress.m_h245Address);

  return true;
}


PBoolean H323Connection::OnReceivedAlerting(const H323SignalPDU & pdu)
{
  if (pdu.m_h323_uu_pdu.m_h323_message_body.GetTag() != H225_H323_UU_PDU_h323_message_body::e_alerting)
    return false;

  if (GetPhase() >= AlertingPhase)
    return true;

  SetPhase(AlertingPhase);

  const H225_Alerting_UUIE & alert = pdu.m_h323_uu_pdu.m_h323_message_body;

  SetRemoteVersions(alert.m_protocolIdentifier);
  SetRemotePartyInfo(pdu);
  SetRemoteApplication(alert.m_destinationInfo);
  
#if OPAL_H460
  ReceiveFeatureSet<H225_Alerting_UUIE>(this, H460_MessageType::e_alerting, alert);
#endif

  // Check for fastStart data and start fast
  if (alert.HasOptionalField(H225_Alerting_UUIE::e_fastStart))
    HandleFastStartAcknowledge(alert.m_fastStart);

  // Check that it has the H.245 channel connection info
  if (alert.HasOptionalField(H225_Alerting_UUIE::e_h245Address) && (!pdu.m_h323_uu_pdu.m_h245Tunneling || endpoint.IsH245TunnelingDisabled()))
    if (!CreateOutgoingControlChannel(alert.m_h245Address))
      return false;

  return OnAlerting(pdu, remotePartyName);
}


PBoolean H323Connection::OnReceivedSignalConnect(const H323SignalPDU & pdu)
{
  if (GetPhase() < AlertingPhase) {
    // Never actually got the ALERTING, fake it as some things get upset
    // if the sequence is not strictly followed.
    SetPhase(AlertingPhase);
    if (!OnAlerting(pdu, remotePartyName))
      return false;
  }

  if (connectionState == ShuttingDownConnection)
    return false;
  connectionState = HasExecutedSignalConnect;

  if (pdu.m_h323_uu_pdu.m_h323_message_body.GetTag() != H225_H323_UU_PDU_h323_message_body::e_connect)
    return false;
  const H225_Connect_UUIE & connect = pdu.m_h323_uu_pdu.m_h323_message_body;

  SetRemoteVersions(connect.m_protocolIdentifier);
  SetRemotePartyInfo(pdu);
  SetRemoteApplication(connect.m_destinationInfo);
  
#if OPAL_H460
  ReceiveFeatureSet<H225_Connect_UUIE>(this, H460_MessageType::e_connect, connect);
#endif

  if (!OnOutgoingCall(pdu)) {
    Release(EndedByNoAccept);
    return false;
  }

#if OPAL_H450
  // Are we involved in a transfer with a non H.450.2 compatible transferred-to endpoint?
  if (h4502handler->GetState() == H4502Handler::e_ctAwaitSetupResponse &&
      h4502handler->IsctTimerRunning())
  {
    PTRACE(4, "H4502\tRemote Endpoint does not support H.450.2.");
    h4502handler->OnReceivedSetupReturnResult();
  }
#endif

  // have answer, so set timeout to interval for monitoring calls health
  signallingChannel->SetReadTimeout(MonitorCallStatusTime);

  // NOTE h323plus checks for faststart here
  // and bails if already started.

  // Check for fastStart data and start fast
  if (connect.HasOptionalField(H225_Connect_UUIE::e_fastStart))
    HandleFastStartAcknowledge(connect.m_fastStart);

  // Check that it has the H.245 channel connection info
  if (connect.HasOptionalField(H225_Connect_UUIE::e_h245Address) && (!pdu.m_h323_uu_pdu.m_h245Tunneling || endpoint.IsH245TunnelingDisabled())) {
    if (!endpoint.IsH245Disabled() && (!CreateOutgoingControlChannel(connect.m_h245Address))) {
      if (fastStartState != FastStartAcknowledged)
        return false;
    }
  }

  // If didn't get fast start channels accepted by remote then clear our
  // proposed channels
  if (fastStartState != FastStartAcknowledged) {
    fastStartState = FastStartDisabled;
    fastStartChannels.RemoveAll();
  }
  else {
    // We have fast start, can connect immediately.
    OnConnectedInternal();
 
    // Otherwise make sure fast started channels are open
    for (H323LogicalChannelList::iterator channel = fastStartChannels.begin(); channel != fastStartChannels.end(); ++channel)
      channel->Open();
  }

  /* do not start h245 negotiation if it is disabled */
  if (endpoint.IsH245Disabled()){
    PTRACE(3, "H245\tOnReceivedSignalConnect: h245 is disabled, do not start negotiation");
    return true;
  }  
  // If we have a H.245 channel available, bring it up. We either have media
  // and this is just so user indications work, or we don't have media and
  // desperately need it!
  if (h245Tunneling)
    return StartControlNegotiations();
  
  // Already started
  if (controlChannel != NULL)
    return true;

  // We have no tunnelling and not separate channel, but we really want one
  // so we will start one using a facility message
  PTRACE(3, "H225\tNo H245 address provided by remote, starting control channel");

  H323SignalPDU want245PDU;
  H225_Facility_UUIE * fac = want245PDU.BuildFacility(*this, false);
  fac->m_reason.SetTag(H225_FacilityReason::e_startH245);
  fac->IncludeOptionalField(H225_Facility_UUIE::e_h245Address);

  if (!CreateIncomingControlChannel(fac->m_h245Address))
    return false;

  return WriteSignalPDU(want245PDU);
}


PBoolean H323Connection::OnReceivedFacility(const H323SignalPDU & pdu)
{
  if (pdu.m_h323_uu_pdu.m_h323_message_body.GetTag() == H225_H323_UU_PDU_h323_message_body::e_empty)
    return true;

  if (pdu.m_h323_uu_pdu.m_h323_message_body.GetTag() != H225_H323_UU_PDU_h323_message_body::e_facility)
    return false;
  const H225_Facility_UUIE & fac = pdu.m_h323_uu_pdu.m_h323_message_body;
  
#if OPAL_H460
  // Do not process H.245 Control PDU's
  if (!pdu.m_h323_uu_pdu.HasOptionalField(H225_H323_UU_PDU::e_h245Control))
    ReceiveFeatureSet<H225_Facility_UUIE>(this, H460_MessageType::e_facility, fac);
#endif

  SetRemoteVersions(fac.m_protocolIdentifier);

  // Check for fastStart data and start fast
  if (fac.HasOptionalField(H225_Facility_UUIE::e_fastStart))
    HandleFastStartAcknowledge(fac.m_fastStart);

  // Check that it has the H.245 channel connection info
  if (fac.HasOptionalField(H225_Facility_UUIE::e_h245Address) && (!pdu.m_h323_uu_pdu.m_h245Tunneling || endpoint.IsH245TunnelingDisabled())) {
    if (controlChannel != NULL) {
      // Fix race condition where both side want to open H.245 channel. we have
      // channel bit it is not open (ie we are listening) and the remote has
      // sent us an address to connect to. To resolve we compare the addresses.

      H323TransportAddress h323Address = controlChannel->GetLocalAddress();
      H225_TransportAddress myAddress;
      h323Address.SetPDU(myAddress);
      PPER_Stream myBuffer;
      myAddress.Encode(myBuffer);

      PPER_Stream otherBuffer;
      fac.m_h245Address.Encode(otherBuffer);

      if (myBuffer < otherBuffer) {
        PTRACE(2, "H225\tSimultaneous start of H.245 channel, connecting to remote.");
        controlChannel->CloseWait();
        delete controlChannel;
        controlChannel = NULL;
      }
      else {
        PTRACE(2, "H225\tSimultaneous start of H.245 channel, using local listener.");
      }
    }

    return CreateOutgoingControlChannel(fac.m_h245Address);
  }

  if (fac.m_reason.GetTag() != H225_FacilityReason::e_callForwarded && 
      fac.m_reason.GetTag() != H225_FacilityReason::e_routeCallToGatekeeper)
    return true;

  PURL addrURL = GetRemotePartyURL();
  if (fac.HasOptionalField(H225_Facility_UUIE::e_alternativeAliasAddress) &&
      fac.m_alternativeAliasAddress.GetSize() > 0)
    addrURL.SetUserName(H323GetAliasAddressString(fac.m_alternativeAliasAddress[0]));

  if (fac.HasOptionalField(H225_Facility_UUIE::e_alternativeAddress)) {
    // Handle routeCallToGatekeeper and send correct destCallSignalAddress 
    // in the H.225 setup message
    if (fac.m_reason.GetTag() == H225_FacilityReason::e_routeCallToGatekeeper)
      addrURL.SetUserName(addrURL.GetUserName()+'@'+addrURL.GetHostName());

    // Set the new host/port in URL from alternative address
    H323TransportAddress alternative(fac.m_alternativeAddress);
    if (!alternative.IsEmpty()) {
      PIPSocket::Address ip;
      WORD port = endpoint.GetDefaultSignalPort();
      if (!alternative.GetIpAndPort(ip, port))
        addrURL.SetHostName(alternative.Mid(alternative.Find('$')+1));
      else {
        addrURL.SetHostName(ip.AsString(true));
        addrURL.SetPort(port);
      }
    }
  }

  PString address = addrURL.AsString();

  if (endpoint.OnConnectionForwarded(*this, address, pdu)) {
    Release(EndedByCallForwarded);
    return false;
  }

  if (!endpoint.OnForwarded(*this, address)) {
    Release(EndedByCallForwarded);
    return false;
  }

  if (!endpoint.CanAutoCallForward())
    return true;

  // If forward is successful, then return false to terminate THIS call.
  return !endpoint.ForwardConnection(*this, address, pdu);
}


PBoolean H323Connection::OnReceivedSignalNotify(const H323SignalPDU & pdu)
{
  if (pdu.m_h323_uu_pdu.m_h323_message_body.GetTag() == H225_H323_UU_PDU_h323_message_body::e_notify) {
    const H225_Notify_UUIE & notify = pdu.m_h323_uu_pdu.m_h323_message_body;
    SetRemoteVersions(notify.m_protocolIdentifier);
  }
  return true;
}


PBoolean H323Connection::OnReceivedSignalStatus(const H323SignalPDU & pdu)
{
  if (pdu.m_h323_uu_pdu.m_h323_message_body.GetTag() == H225_H323_UU_PDU_h323_message_body::e_status) {
    const H225_Status_UUIE & status = pdu.m_h323_uu_pdu.m_h323_message_body;
    SetRemoteVersions(status.m_protocolIdentifier);
  }
  return true;
}


PBoolean H323Connection::OnReceivedStatusEnquiry(const H323SignalPDU & pdu)
{
  if (pdu.m_h323_uu_pdu.m_h323_message_body.GetTag() == H225_H323_UU_PDU_h323_message_body::e_statusInquiry) {
    const H225_StatusInquiry_UUIE & status = pdu.m_h323_uu_pdu.m_h323_message_body;
    SetRemoteVersions(status.m_protocolIdentifier);
  }

  H323SignalPDU reply;
  reply.BuildStatus(*this);
  return reply.Write(*signallingChannel);
}


void H323Connection::OnReceivedReleaseComplete(const H323SignalPDU & pdu)
{
  endSessionReceived.Signal();

  CallEndReason reason(EndedByRefusal, pdu.GetQ931().GetCause());
  
  const H225_ReleaseComplete_UUIE & rc = pdu.m_h323_uu_pdu.m_h323_message_body;

  switch (connectionState) {
    case EstablishedConnection :
      reason.code = EndedByRemoteUser;
      break;

    case AwaitingLocalAnswer :
      reason.code = EndedByCallerAbort;
      break;

    default :
      if (callEndReason == EndedByRefusal)
        callEndReason = NumCallEndReasons;
      
      // Are we involved in a transfer with a non H.450.2 compatible transferred-to endpoint?
#if OPAL_H450
      if (h4502handler->GetState() == H4502Handler::e_ctAwaitSetupResponse &&
          h4502handler->IsctTimerRunning())
      {
        PTRACE(4, "H4502\tThe Remote Endpoint has rejected our transfer request and does not support H.450.2.");
        h4502handler->OnReceivedSetupReturnError(H4501_GeneralErrorList::e_notAvailable);
      }
#endif
          
#if OPAL_H460
      ReceiveFeatureSet<H225_ReleaseComplete_UUIE>(this, H460_MessageType::e_releaseComplete, rc);
#endif

      if (pdu.m_h323_uu_pdu.m_h323_message_body.GetTag() == H225_H323_UU_PDU_h323_message_body::e_releaseComplete) {
        SetRemoteVersions(rc.m_protocolIdentifier);
        reason = H323TranslateToCallEndReason(pdu.GetQ931().GetCause(), rc.m_reason.GetTag());
      }
  }

  Release(reason);
}


PBoolean H323Connection::OnIncomingCall(const H323SignalPDU & setupPDU,
                                    H323SignalPDU & alertingPDU)
{
  return endpoint.OnIncomingCall(*this, setupPDU, alertingPDU);
}


PBoolean H323Connection::ForwardCall(const PString & forwardParty)
{
  if (forwardParty.IsEmpty())
    return false;

  PString alias;
  H323TransportAddress address;
  endpoint.ParsePartyName(forwardParty, alias, address);

  H323SignalPDU redirectPDU;
  H225_Facility_UUIE * fac = redirectPDU.BuildFacility(*this, false);

  fac->m_reason.SetTag(H225_FacilityReason::e_callForwarded);

  if (!address) {
    fac->IncludeOptionalField(H225_Facility_UUIE::e_alternativeAddress);
    address.SetPDU(fac->m_alternativeAddress, endpoint.GetDefaultSignalPort());
  }

  if (!alias) {
    fac->IncludeOptionalField(H225_Facility_UUIE::e_alternativeAliasAddress);
    fac->m_alternativeAliasAddress.SetSize(1);
    H323SetAliasAddress(alias, fac->m_alternativeAliasAddress[0]);
  }

  if (WriteSignalPDU(redirectPDU))
    Release(EndedByCallForwarded);

  return true;
}


H323Connection::AnswerCallResponse
     H323Connection::OnAnswerCall(const PString & caller,
                                  const H323SignalPDU & setupPDU,
                                  H323SignalPDU & connectPDU,
                                  H323SignalPDU & progressPDU)
{
  PTRACE(3, "H323CON\tOnAnswerCall " << *this << ", caller = " << caller);
  return endpoint.OnAnswerCall(*this, caller, setupPDU, connectPDU, progressPDU);
}

H323Connection::AnswerCallResponse
     H323Connection::OnAnswerCall(const PString & caller)
{
  return OpalRTPConnection::OnAnswerCall(caller);
}

void H323Connection::AnsweringCall(AnswerCallResponse response)
{
  PTRACE(3, "H323\tAnswering call: " << response);

  PSafeLockReadWrite safeLock(*this);
  if (!safeLock.IsLocked() || IsReleased())
    return;

  if (response == AnswerCallProgress) {
    H323SignalPDU want245PDU;
    want245PDU.BuildProgress(*this);
    WriteSignalPDU(want245PDU);
  }

  OpalConnection::AnsweringCall(response);
}


PString H323Connection::GetPrefixName() const
{
  return OpalConnection::GetPrefixName();
}


PBoolean H323Connection::SetUpConnection()
{
  InternalSetAsOriginating();

  OnApplyStringOptions();

  signallingChannel->AttachThread(PThread::Create(PCREATE_NOTIFIER(StartOutgoing), "H225 Caller"));
  return true;
}


void H323Connection::StartOutgoing(PThread &, INT)
{
  PTRACE(3, "H225\tStarted call thread");

  if (!SafeReference())
    return;

  PString alias;
  if (remotePartyName != m_remoteConnectAddress)
    alias = remotePartyName;

  CallEndReason reason = SendSignalSetup(alias, m_remoteConnectAddress);

  // Check if had an error, clear call if so
  if (reason != NumCallEndReasons)
    Release(reason);
  else
    HandleSignallingChannel();

  SafeDereference();
}


OpalConnection::CallEndReason H323Connection::SendSignalSetup(const PString & alias,
                                                              const H323TransportAddress & address)
{
  PSafeLockReadWrite safeLock(*this);
  if (!safeLock.IsLocked())
    return EndedByCallerAbort;

  // Start the call, first state is asking gatekeeper
  connectionState = AwaitingGatekeeperAdmission;

  // See if we are "proxied" that is the destCallSignalAddress is different
  // from the transport connection address
  H323TransportAddress destCallSignalAddress = address;
  PINDEX atInAlias = alias.Find('@');
  if (atInAlias != P_MAX_INDEX)
    destCallSignalAddress = H323TransportAddress(alias.Mid(atInAlias+1), endpoint.GetDefaultSignalPort());

  // Initial value for alias in SETUP, could be overridden by gatekeeper
  H225_ArrayOf_AliasAddress newAliasAddresses;
  if (!alias.IsEmpty() && atInAlias > 0) {
    newAliasAddresses.SetSize(1);
    H323SetAliasAddress(alias.Left(atInAlias), newAliasAddresses[0]);
  }

  // Start building the setup PDU to get various ID's
  H323SignalPDU setupPDU;
  H225_Setup_UUIE & setup = setupPDU.BuildSetup(*this, destCallSignalAddress);

#if OPAL_H450
  h450dispatcher->AttachToSetup(setupPDU);
#endif

  // Save the identifiers generated by BuildSetup
  callReference = setupPDU.GetQ931().GetCallReference();
  conferenceIdentifier = setup.m_conferenceID;
  setupPDU.GetQ931().GetCalledPartyNumber(remotePartyNumber);

  H323TransportAddress gatekeeperRoute = address;

  // Check for gatekeeper and do admission check if have one
  H323Gatekeeper * gatekeeper = endpoint.GetGatekeeper();
  if (gatekeeper != NULL) {
    H323Gatekeeper::AdmissionResponse response;
    response.transportAddress = &gatekeeperRoute;
    response.aliasAddresses = &newAliasAddresses;
    if (!gkAccessTokenOID)
      response.accessTokenData = &gkAccessTokenData;
    for (;;) {
      safeLock.Unlock();
      PBoolean ok = gatekeeper->AdmissionRequest(*this, response, alias.IsEmpty());
      if (!safeLock.Lock())
        return EndedByCallerAbort;
      if (ok)
        break;
      PTRACE(2, "H225\tGatekeeper refused admission: "
             << (response.rejectReason == UINT_MAX
                  ? PString("Transport error")
                  : H225_AdmissionRejectReason(response.rejectReason).GetTagName()));
#if OPAL_H450
      h4502handler->onReceivedAdmissionReject(H4501_GeneralErrorList::e_notAvailable);
#endif

      switch (response.rejectReason) {
        case H225_AdmissionRejectReason::e_calledPartyNotRegistered :
          return EndedByNoUser;
        case H225_AdmissionRejectReason::e_requestDenied :
          return EndedByNoBandwidth;
        case H225_AdmissionRejectReason::e_invalidPermission :
        case H225_AdmissionRejectReason::e_securityDenial :
          return EndedBySecurityDenial;
        case H225_AdmissionRejectReason::e_resourceUnavailable :
          return EndedByRemoteBusy;
        case H225_AdmissionRejectReason::e_incompleteAddress :
          if (OnInsufficientDigits())
            break;
          // Then default case
        default :
          return EndedByGatekeeper;
      }

      PString lastRemotePartyName = remotePartyName;
      while (lastRemotePartyName == remotePartyName) {
        UnlockReadWrite(); // Release the mutex as can deadlock trying to clear call during connect.
        digitsWaitFlag.Wait();
        if (!LockReadWrite()) // Lock while checking for shutting down.
          return EndedByCallerAbort;
        if (IsReleased())
          return EndedByCallerAbort;
      }
    }
    mustSendDRQ = true;
    if (response.gatekeeperRouted) {
      setup.IncludeOptionalField(H225_Setup_UUIE::e_endpointIdentifier);
      setup.m_endpointIdentifier = gatekeeper->GetEndpointIdentifier();
      gatekeeperRouted = true;
    }
  }

  // Update the field e_destinationAddress in the SETUP PDU to reflect the new 
  // alias received in the ACF (m_destinationInfo).
  if (newAliasAddresses.GetSize() > 0) {
    setup.IncludeOptionalField(H225_Setup_UUIE::e_destinationAddress);
    setup.m_destinationAddress = newAliasAddresses;

    // Update the Q.931 Information Element (if is an E.164 address)
    PString e164 = H323GetAliasAddressE164(newAliasAddresses);
    if (!e164)
      remotePartyNumber = e164;
  }

  if (addAccessTokenToSetup && !gkAccessTokenOID && !gkAccessTokenData.IsEmpty()) {
    PString oid1, oid2;
    if (!gkAccessTokenOID.Split(',', oid1, oid2))
      oid1 = oid2 = gkAccessTokenOID;
    setup.IncludeOptionalField(H225_Setup_UUIE::e_tokens);
    PINDEX last = setup.m_tokens.GetSize();
    setup.m_tokens.SetSize(last+1);
    setup.m_tokens[last].m_tokenOID = oid1;
    setup.m_tokens[last].IncludeOptionalField(H235_ClearToken::e_nonStandard);
    setup.m_tokens[last].m_nonStandard.m_nonStandardIdentifier = oid2;
    setup.m_tokens[last].m_nonStandard.m_data = gkAccessTokenData;
  }

  if (!signallingChannel->SetRemoteAddress(gatekeeperRoute)) {
    PTRACE(1, "H225\tInvalid "
           << (gatekeeperRoute != address ? "gatekeeper" : "user")
           << " supplied address: \"" << gatekeeperRoute << '"');
    connectionState = AwaitingTransportConnect;
    return EndedByConnectFail;
  }

#if OPAL_H460
  setupPDU.InsertH460Setup(*this, setup);
#endif

  // Do the transport connect
  connectionState = AwaitingTransportConnect;

  // Release the mutex as can deadlock trying to clear call during connect.
  safeLock.Unlock();

  PBoolean connectFailed = !signallingChannel->Connect();

    // Lock while checking for shutting down.
  if (!safeLock.Lock())
    return EndedByCallerAbort;

  if (IsReleased())
    return EndedByCallerAbort;

  // See if transport connect failed, abort if so.
  if (connectFailed) {
    connectionState = NoConnectionActive;
    SetPhase(UninitialisedPhase);
    switch (signallingChannel->GetErrorNumber()) {
      case ENETUNREACH :
        return EndedByUnreachable;
      case ECONNREFUSED :
        return EndedByNoEndPoint;
      case ETIMEDOUT :
        return EndedByHostOffline;
    }
    return EndedByConnectFail;
  }

  PTRACE(3, "H225\tSending Setup PDU");
  connectionState = AwaitingSignalConnect;

  // Put in all the signalling addresses for link
  H323TransportAddress transportAddress = signallingChannel->GetLocalAddress();
  setup.IncludeOptionalField(H225_Setup_UUIE::e_sourceCallSignalAddress);
  transportAddress.SetPDU(setup.m_sourceCallSignalAddress);
  if (!setup.HasOptionalField(H225_Setup_UUIE::e_destCallSignalAddress)) {
    transportAddress = signallingChannel->GetRemoteAddress();
    setup.IncludeOptionalField(H225_Setup_UUIE::e_destCallSignalAddress);
    transportAddress.SetPDU(setup.m_destCallSignalAddress);
  }

  OnApplyStringOptions();

  // Get the local capabilities before fast start is handled
  OnSetLocalCapabilities();

  // Ask the application what channels to open
  PTRACE(3, "H225\tCheck for Fast start by local endpoint");
  fastStartChannels.RemoveAll();
  OnSelectLogicalChannels();

  // If application called OpenLogicalChannel, put in the fastStart field
  if (!fastStartChannels.IsEmpty()) {
    PTRACE(3, "H225\tFast start begun by local endpoint");
    for (H323LogicalChannelList::iterator channel = fastStartChannels.begin(); channel != fastStartChannels.end(); ++channel)
      BuildFastStartList(*channel, setup.m_fastStart, H323Channel::IsReceiver);
    if (setup.m_fastStart.GetSize() > 0)
      setup.IncludeOptionalField(H225_Setup_UUIE::e_fastStart);
  }

  // Search the capability set and see if we have video capability
  const char * bearerCaps = "Speech,1"; // TransferSpeech, 1 x 64k bearer
  for (PINDEX i = 0; i < localCapabilities.GetSize(); i++) {
    if (!PIsDescendant(&localCapabilities[i], H323AudioCapability) &&
        !PIsDescendant(&localCapabilities[i], H323_UserInputCapability)) {
      bearerCaps = "Digital,6"; // Unrestricted digital, 384 kbps (6 bearers)
      break;
    }
  }
  setupPDU.GetQ931().SetBearerCapabilities(m_stringOptions(OPAL_OPT_Q931_BEARER_CAPS, bearerCaps));

  if (!OnSendSignalSetup(setupPDU))
    return EndedByNoAccept;

  // Do this again (was done when PDU was constructed) in case
  // OnSendSignalSetup() changed something.
  setupPDU.SetQ931Fields(*this, true);
  setupPDU.GetQ931().GetCalledPartyNumber(remotePartyNumber);

  fastStartState = FastStartDisabled;
  PBoolean set_lastPDUWasH245inSETUP = false;

  if (h245Tunneling && doH245inSETUP && !endpoint.IsH245Disabled()) {
    h245TunnelTxPDU = &setupPDU;

    // Try and start the master/slave and capability exchange through the tunnel
    // Note: this used to be disallowed but is now allowed as of H323v4
    PBoolean ok = StartControlNegotiations();

    h245TunnelTxPDU = NULL;

    if (!ok)
      return EndedByTransportFail;

    if (doH245inSETUP && (setup.m_fastStart.GetSize() > 0)) {

      // Now if fast start as well need to put this in setup specific field
      // and not the generic H.245 tunneling field
      setup.IncludeOptionalField(H225_Setup_UUIE::e_parallelH245Control);
      setup.m_parallelH245Control = setupPDU.m_h323_uu_pdu.m_h245Control;
      setupPDU.m_h323_uu_pdu.RemoveOptionalField(H225_H323_UU_PDU::e_h245Control);
      set_lastPDUWasH245inSETUP = true;
    }
  }

  // Send the initial PDU
  if (!WriteSignalPDU(setupPDU))
    return EndedByTransportFail;

  SetPhase(SetUpPhase);

  // WriteSignalPDU always resets lastPDUWasH245inSETUP.
  // So set it here if required
  if (set_lastPDUWasH245inSETUP)
    lastPDUWasH245inSETUP = true;

  // Set timeout for remote party to answer the call
  signallingChannel->SetReadTimeout(endpoint.GetSignallingChannelCallTimeout());

  connectionState = AwaitingSignalConnect;

  return NumCallEndReasons;
}


PBoolean H323Connection::OnSendSignalSetup(H323SignalPDU & pdu)
{
  return endpoint.OnSendSignalSetup(*this, pdu);
}


PBoolean H323Connection::OnSendCallProceeding(H323SignalPDU & callProceedingPDU)
{
  return endpoint.OnSendCallProceeding(*this, callProceedingPDU);
}


void H323Connection::NatDetection(const PIPSocket::Address & srcAddress, const PIPSocket::Address & sigAddress)
{
  // if the peer address is a public address, but the advertised source address is a private address
  // then there is a good chance the remote endpoint is behind a NAT but does not know it.
  // in this case, we active the NAT mode and wait for incoming RTP to provide the media address before 
  // sending anything to the remote endpoint
  if ((!sigAddress.IsRFC1918() && srcAddress.IsRFC1918()) ||    // Internet Address
      ((sigAddress.IsRFC1918() && srcAddress.IsRFC1918()) && (sigAddress != srcAddress)))  // LAN on another LAN
  {
    PTRACE(3, "H225\tSource signal address " << srcAddress << " and TCP peer address " << sigAddress << " indicate remote endpoint is behind NAT");
    if (OnNatDetected())
      remoteIsNAT = true;
  }
}


PBoolean H323Connection::OnNatDetected()
{
#if OPAL_H460
  if (m_H46019enabled) 
    return false;
#endif
  return true;
}


PBoolean H323Connection::OnSendReleaseComplete(H323SignalPDU & /*releaseCompletePDU*/)
{
  return true;
}


PBoolean H323Connection::OnAlerting(const H323SignalPDU & alertingPDU,
                                const PString & username)
{
  return endpoint.OnAlerting(*this, alertingPDU, username);
}


PBoolean H323Connection::SetAlerting(const PString & calleeName, PBoolean withMedia)
{
  PSafeLockReadWrite safeLock(*this);
  if (!safeLock.IsLocked())
    return false;

  PTRACE(3, "H323\tSetAlerting " << *this);
  if (alertingPDU == NULL)
    return false;

  if (withMedia && !mediaWaitForConnect) {
    H225_Alerting_UUIE & alerting = alertingPDU->m_h323_uu_pdu.m_h323_message_body;
    if (SendFastStartAcknowledge(alerting.m_fastStart))
      alerting.IncludeOptionalField(H225_Alerting_UUIE::e_fastStart);
    else {
      // See if aborted call
      if (connectionState == ShuttingDownConnection)
        return false;

      // Do early H.245 start
      if (!endpoint.IsH245Disabled()) {
        earlyStart = true;
        if (h245Tunneling) {
          if (!StartControlNegotiations())
            return false;
        } else if (controlChannel == NULL) {
          if (!CreateIncomingControlChannel(alerting.m_h245Address))
            return false;
          alerting.IncludeOptionalField(H225_Alerting_UUIE::e_h245Address);
        }
      }
    }
  }

  HandleTunnelPDU(alertingPDU);

#if OPAL_H450
  h450dispatcher->AttachToAlerting(*alertingPDU);
#endif

  if (!endpoint.OnSendAlerting(*this, *alertingPDU, calleeName, withMedia)){
    /* let the application to avoid sending the alerting, mainly for testing other endpoints*/
    PTRACE(3, "H323CON\tSetAlerting Alerting not sent");
    return true;
  }
  
  // send Q931 Alerting PDU
  PTRACE(3, "H323CON\tSetAlerting sending Alerting PDU");
  
  PBoolean bOk = WriteSignalPDU(*alertingPDU);
  if (!endpoint.OnSentAlerting(*this)){
    /* let the application to know that the alerting has been sent */
    /* do nothing for now, at least check for the return value */
  }

  InternalEstablishedConnectionCheck();
  return bOk;
}


PBoolean H323Connection::SetConnected()
{
  PSafeLockReadWrite safeLock(*this);
  if (!safeLock.IsLocked() || GetPhase() >= ConnectedPhase)
    return false;

  mediaWaitForConnect = false;

  PTRACE(3, "H323CON\tSetConnected " << *this);
  if (connectPDU == NULL){
    PTRACE(1, "H323CON\tSetConnected connectPDU is null" << *this);
    return false;
  }  

  if (!endpoint.OnSendConnect(*this, *connectPDU)){
    /* let the application to avoid sending the connect, mainly for testing other endpoints*/
    PTRACE(2, "H323CON\tSetConnected connect not sent");
    return true;
  }  
  // Assure capabilities are set to other connections media list (if not already)
  OnSetLocalCapabilities();

  H225_Connect_UUIE & connect = connectPDU->m_h323_uu_pdu.m_h323_message_body;

  // Now ask the application to select which channels to start
  if (SendFastStartAcknowledge(connect.m_fastStart))
    connect.IncludeOptionalField(H225_Connect_UUIE::e_fastStart);

  // See if aborted call
  if (connectionState == ShuttingDownConnection)
    return false;

  // Set flag that we are up to CONNECT stage
  connectionState = HasExecutedSignalConnect;
  SetPhase(ConnectedPhase);

#if OPAL_H450
  h450dispatcher->AttachToConnect(*connectPDU);
#endif

  if (!endpoint.IsH245Disabled()){
    if (h245Tunneling) {
      HandleTunnelPDU(connectPDU);
  
      // If no channels selected (or never provided) do traditional H245 start
      if (fastStartState == FastStartDisabled) {
        h245TunnelTxPDU = connectPDU; // Piggy back H245 on this reply
        PBoolean ok = StartControlNegotiations();
        h245TunnelTxPDU = NULL;
        if (!ok)
          return false;
      }
    }
    else if (!controlChannel) { // Start separate H.245 channel if not tunneling.
      if (!CreateIncomingControlChannel(connect.m_h245Address))
        return false;
      connect.IncludeOptionalField(H225_Connect_UUIE::e_h245Address);
    }
  }

  if (!WriteSignalPDU(*connectPDU)) // Send H323 Connect PDU
    return false;

  delete connectPDU;
  connectPDU = NULL;
  delete alertingPDU;
  alertingPDU = NULL;

  InternalEstablishedConnectionCheck();
  return true;
}

PBoolean H323Connection::SetProgressed()
{
  PSafeLockReadWrite safeLock(*this);
  if (!safeLock.IsLocked())
    return false;

  mediaWaitForConnect = false;

  PTRACE(3, "H323\tSetProgressed " << *this);
  if (progressPDU == NULL){
    PTRACE(1, "H323\tSetProgressed progressPDU is null" << *this);
    return false;
  }  

  // Assure capabilities are set to other connections media list (if not already)
  OnSetLocalCapabilities();

  H225_Progress_UUIE & progress = progressPDU->m_h323_uu_pdu.m_h323_message_body;

  // Now ask the application to select which channels to start
  if (SendFastStartAcknowledge(progress.m_fastStart))
    progress.IncludeOptionalField(H225_Connect_UUIE::e_fastStart);

  // See if aborted call
  if (connectionState == ShuttingDownConnection)
    return false;

  //h450dispatcher->AttachToProgress(*progress);
  if (!endpoint.IsH245Disabled()){
    if (h245Tunneling) {
      HandleTunnelPDU(progressPDU);
  
      // If no channels selected (or never provided) do traditional H245 start
      if (fastStartState == FastStartDisabled) {
        h245TunnelTxPDU = progressPDU; // Piggy back H245 on this reply
        PBoolean ok = StartControlNegotiations();
        h245TunnelTxPDU = NULL;
        if (!ok)
          return false;
      }
    }
    else if (!controlChannel) { // Start separate H.245 channel if not tunneling.
      if (!CreateIncomingControlChannel(progress.m_h245Address))
        return false;
      progress.IncludeOptionalField(H225_Connect_UUIE::e_h245Address);
    }
  }
  
  if (!WriteSignalPDU(*progressPDU)) // Send H323 Progress PDU
    return false;

  InternalEstablishedConnectionCheck();
  return true;
}


PBoolean H323Connection::OnInsufficientDigits()
{
  return false;
}


void H323Connection::SendMoreDigits(const PString & digits)
{
  remotePartyNumber += digits;
  remotePartyName = remotePartyNumber;
  if (connectionState == AwaitingGatekeeperAdmission)
    digitsWaitFlag.Signal();
  else {
    H323SignalPDU infoPDU;
    infoPDU.BuildInformation(*this);
    infoPDU.GetQ931().SetCalledPartyNumber(digits);
    if (!WriteSignalPDU(infoPDU))
      Release(EndedByTransportFail);
  }
}


PBoolean H323Connection::OnOutgoingCall(const H323SignalPDU & connectPDU)
{
  return endpoint.OnOutgoingCall(*this, connectPDU);
}


PBoolean H323Connection::SendFastStartAcknowledge(H225_ArrayOf_PASN_OctetString & fastStartReply)
{
  // See if we have already added the fast start OLC's
  if (fastStartReply.GetSize() > 0)
    return true;

  if (fastStartState == FastStartDisabled)
    return false;

  if (fastStartState == FastStartAcknowledged)
    return true;

  if (fastStartChannels.IsEmpty()) {
    // If we are incapable of ANY of the fast start channels, don't do fast start
    fastStartState = FastStartDisabled;
    return false;
  }

  // See if we need to select our fast start channels
  if (fastStartState == FastStartResponse)
    OnSelectLogicalChannels();

  // Remove any channels that were not started by OnSelectLogicalChannels(),
  // those that were started are put into the logical channel dictionary
  for (H323LogicalChannelList::iterator channel = fastStartChannels.begin(); channel != fastStartChannels.end(); ) {
    if (channel->IsOpen())
      ++channel;
    else
      fastStartChannels.erase(channel++); // Do ++ in both legs so iterator works with erase
  }

  // None left, so didn't open any channels fast
  if (fastStartChannels.IsEmpty()) {
    fastStartState = FastStartDisabled;
    return false;
  }

  // The channels we just transferred to the logical channels dictionary
  // should not be deleted via this structure now.
  fastStartChannels.DisallowDeleteObjects();

  PTRACE(3, "H225\tAccepting fastStart for " << fastStartChannels.GetSize() << " channels");

  for (H323LogicalChannelList::iterator channel = fastStartChannels.begin(); channel != fastStartChannels.end(); ++channel)
    BuildFastStartList(*channel, fastStartReply, H323Channel::IsTransmitter);

  // Have moved open channels to logicalChannels structure, remove all others.
  fastStartChannels.RemoveAll();

  // Set flag so internal establishment check does not require H.245
  fastStartState = FastStartAcknowledged;

  return true;
}


PBoolean H323Connection::HandleFastStartAcknowledge(const H225_ArrayOf_PASN_OctetString & array)
{
  if (fastStartChannels.IsEmpty()) {
    PTRACE(2, "H225\tFast start response with no channels to open");
    return false;
  }

  PTRACE(3, "H225\tFast start accepted by remote endpoint");

  PINDEX i;

  H323LogicalChannelList replyFastStartChannels;

  // Go through provided list of structures, if can decode it and match it up
  // with a channel we requested AND it has all the information needed in the
  // m_multiplexParameters, then we can start the channel.
  for (i = 0; i < array.GetSize(); i++) {
    H245_OpenLogicalChannel open;
    if (array[i].DecodeSubType(open)) {
      PTRACE(4, "H225\tFast start open:\n  " << setprecision(2) << open);
      PBoolean reverse = open.HasOptionalField(H245_OpenLogicalChannel::e_reverseLogicalChannelParameters);
      const H245_DataType & dataType = reverse ? open.m_reverseLogicalChannelParameters.m_dataType
                                               : open.m_forwardLogicalChannelParameters.m_dataType;
      H323Capability * replyCapability = localCapabilities.FindCapability(dataType);
      if (replyCapability != NULL) {
        for (H323LogicalChannelList::iterator channel = fastStartChannels.begin(); channel != fastStartChannels.end(); ++channel) {
          H323Channel & channelToStart = *channel;
          H323Channel::Directions dir = channelToStart.GetDirection();
          if ((dir == H323Channel::IsReceiver) == reverse &&
               channelToStart.GetCapability() == *replyCapability) {
            unsigned error = 1000;
            if (channelToStart.OnReceivedPDU(open, error)) {
              H323Capability * channelCapability;
              if (dir == H323Channel::IsReceiver)
                channelCapability = replyCapability;
              else {
                // For transmitter, need to fake a capability into the remote table
                channelCapability = remoteCapabilities.FindCapability(channelToStart.GetCapability());
                if (channelCapability == NULL) {
                  channelCapability = remoteCapabilities.Copy(channelToStart.GetCapability());
                  remoteCapabilities.SetCapability(0, channelCapability->GetDefaultSessionID()-1, channelCapability);
                }
              }
              // Must use the actual capability instance from the
              // localCapability or remoteCapability structures.
              if (OnCreateLogicalChannel(*channelCapability, dir, error)) {
                if (channelToStart.SetInitialBandwidth()) {
                  replyFastStartChannels.Append(&*channel);
                  fastStartChannels.DisallowDeleteObjects();
                  fastStartChannels.erase(channel);
                  fastStartChannels.AllowDeleteObjects();
                  break;
                }
                else
                  PTRACE(2, "H225\tFast start channel open fail: insufficent bandwidth");
              }
              else
                PTRACE(2, "H225\tFast start channel open error: " << error);
            }
            else
              PTRACE(2, "H225\tFast start capability error: " << error);
          }
        }
      }
    }
    else {
      PTRACE(1, "H225\tInvalid fast start PDU decode:\n  " << setprecision(2) << open);
    }
  }

  // The channels we just transferred to the logical channels dictionary
  // should not be deleted via this structure now.
  fastStartChannels = replyFastStartChannels;
  fastStartChannels.DisallowDeleteObjects();

  PTRACE(3, "H225\tFast starting " << fastStartChannels.GetSize() << " channels");
  if (fastStartChannels.IsEmpty())
    return false;

  fastStartState = FastStartAcknowledged;

  return true;
}


PBoolean H323Connection::OnUnknownSignalPDU(const H323SignalPDU & PTRACE_PARAM(pdu))
{
  PTRACE(2, "H225\tUnknown signalling PDU: " << pdu);
  return true;
}


PBoolean H323Connection::CreateOutgoingControlChannel(const H225_TransportAddress & h245Address)
{
  PTRACE(3, "H225\tCreateOutgoingControlChannel h245Address = " << h245Address);
  if (endpoint.IsH245Disabled()){
    PTRACE(2, "H225\tCreateOutgoingControlChannel h245 is disabled, do nothing");
    /* return true to act as if it was succeded*/
    return true;
  }
  // Already have the H245 channel up.
  if (controlChannel != NULL)
    return true;

  // Check that it is an IP address, all we support at the moment
  controlChannel = signallingChannel->GetLocalAddress().CreateTransport(
                                  endpoint, OpalTransportAddress::HostOnly);
  if (controlChannel == NULL) {
    PTRACE(1, "H225\tConnect of H245 failed: Unsupported transport");
    return false;
  }

  if (!controlChannel->SetRemoteAddress(H323TransportAddress(h245Address))) {
    PTRACE(1, "H225\tCould not extract H245 address");
    delete controlChannel;
    controlChannel = NULL;
    return false;
  }

  if (!controlChannel->Connect()) {
    PTRACE(1, "H225\tConnect of H245 failed: " << controlChannel->GetErrorText());
    delete controlChannel;
    controlChannel = NULL;
    return false;
  }

  controlChannel->AttachThread(PThread::Create(PCREATE_NOTIFIER(NewOutgoingControlChannel), "H.245 Handler"));
  return true;
}


void H323Connection::NewOutgoingControlChannel(PThread &, INT)
{
  if (PAssertNULL(controlChannel) == NULL)
    return;

  if (!SafeReference())
    return;

  HandleControlChannel();
  SafeDereference();
}


PBoolean H323Connection::CreateIncomingControlChannel(H225_TransportAddress & h245Address)
{
  PAssert(controlChannel == NULL, PLogicError);

  if (endpoint.IsH245Disabled()){
    PTRACE(2, "H225\tCreateIncomingControlChannel: do not create channel because h245 is disabled");
    return false;
  }
  
  if (controlListener == NULL) {
    controlListener = signallingChannel->GetLocalAddress().CreateListener(endpoint, OpalTransportAddress::HostOnly);
    if (controlListener == NULL)
      return false;

    if (!controlListener->Open(PCREATE_NOTIFIER(NewIncomingControlChannel), OpalListener::HandOffThreadMode)) {
      delete controlListener;
      controlListener = NULL;
      return false;
    }
  }

  H323TransportAddress listeningAddress = controlListener->GetLocalAddress(signallingChannel->GetRemoteAddress());

  // assign address into the PDU
  return listeningAddress.SetPDU(h245Address);
}


void H323Connection::NewIncomingControlChannel(PThread & listener, INT param)
{
  ((OpalListener&)listener).Close();

  if (param == 0) {
    // If H.245 channel failed to connect and have no media (no fast start)
    // then clear the call as it is useless.
    if (mediaStreams.IsEmpty())
      Release(EndedByTransportFail);
    return;
  }

  if (!SafeReference())
    return;

  controlChannel = (H323Transport *)param;
  HandleControlChannel();
  SafeDereference();
}


PBoolean H323Connection::WriteControlPDU(const H323ControlPDU & pdu)
{
  PPER_Stream strm;
  pdu.Encode(strm);
  strm.CompleteEncoding();

  H323TraceDumpPDU("H245", true, strm, pdu, pdu, 0);

  if (!h245Tunneling) {
    if (controlChannel == NULL) {
      PTRACE(1, "H245\tWrite PDU fail: no control channel.");
      return false;
    }

    if (controlChannel->IsOpen() && controlChannel->WritePDU(strm))
      return true;

    PTRACE(1, "H245\tWrite PDU fail: " << controlChannel->GetErrorText(PChannel::LastWriteError));
    return false;
  }

  // If have a pending signalling PDU, use it rather than separate write
  H323SignalPDU localTunnelPDU;
  H323SignalPDU * tunnelPDU;
  if (h245TunnelTxPDU != NULL)
    tunnelPDU = h245TunnelTxPDU;
  else {
    localTunnelPDU.BuildFacility(*this, true);
    tunnelPDU = &localTunnelPDU;
  }

  tunnelPDU->m_h323_uu_pdu.IncludeOptionalField(H225_H323_UU_PDU::e_h245Control);
  PINDEX last = tunnelPDU->m_h323_uu_pdu.m_h245Control.GetSize();
  tunnelPDU->m_h323_uu_pdu.m_h245Control.SetSize(last+1);
  tunnelPDU->m_h323_uu_pdu.m_h245Control[last] = strm;

  if (h245TunnelTxPDU != NULL)
    return true;

  return WriteSignalPDU(localTunnelPDU);
}


PBoolean H323Connection::StartControlNegotiations()
{
  PTRACE(3, "H245\tStarted control channel");

 
  if (endpoint.IsH245Disabled()){
    PTRACE(2, "H245\tStartControlNegotiations h245 is disabled, do not start negotiation");
    return false;
  }
  // Get the local capabilities before fast start is handled
  OnSetLocalCapabilities();

  // Begin the capability exchange procedure
  if (!capabilityExchangeProcedure->Start(false)) {
    PTRACE(1, "H245\tStart of Capability Exchange failed");
    return false;
  }

  // Begin the Master/Slave determination procedure
  if (!masterSlaveDeterminationProcedure->Start(false)) {
    PTRACE(1, "H245\tStart of Master/Slave determination failed");
    return false;
  }

  endSessionNeeded = true;
  return true;
}

PBoolean H323Connection::OnStartHandleControlChannel()
{
  PSafeLockReadWrite mutex(*this);

  PTRACE(2, "H46018\tStarted control channel");

#ifdef OPAL_H460
  if (m_H46019enabled) {
    // according to H.460.18 cl.11 we have to send a generic Indication on the opening of a
    // H.245 control channel. Details are specified in H.460.18 cl.16
    // This must be the first PDU otherwise gatekeeper/proxy will close the channel.
    
    if (endpoint.H46018IsEnabled() && !m_h245Connect) {
      H323ControlPDU pdu;
      H245_GenericMessage & cap = pdu.Build(H245_IndicationMessage::e_genericIndication);
      
      H245_CapabilityIdentifier & id = cap.m_messageIdentifier;
      id.SetTag(H245_CapabilityIdentifier::e_standard);
      PASN_ObjectId & gid = id;
      gid.SetValue(H46018OID);
      
      cap.IncludeOptionalField(H245_GenericMessage::e_subMessageIdentifier);
      PASN_Integer & sub = cap.m_subMessageIdentifier;
      sub = 1;
      
      cap.IncludeOptionalField(H245_GenericMessage::e_messageContent);
      H245_ArrayOf_GenericParameter & msg = cap.m_messageContent;
      
      // callIdentifer
      H245_GenericParameter call;
      H245_ParameterIdentifier & idx = call.m_parameterIdentifier;
      idx.SetTag(H245_ParameterIdentifier::e_standard);
      PASN_Integer & m = idx;
      m =1;
      H245_ParameterValue & conx = call.m_parameterValue;
      conx.SetTag(H245_ParameterValue::e_octetString);
      PASN_OctetString & raw = conx;
      raw.SetValue(callIdentifier);
      msg.SetSize(1);
      msg[0] = call;
      
      // Is receiver
      if (m_H46019CallReceiver) {
        H245_GenericParameter answer;
        H245_ParameterIdentifier & an = answer.m_parameterIdentifier;
        an.SetTag(H245_ParameterIdentifier::e_standard);
        PASN_Integer & n = an;
        n = 2;
        H245_ParameterValue & aw = answer.m_parameterValue;
        aw.SetTag(H245_ParameterValue::e_logical);
        msg.SetSize(2);
        msg[1] = answer;
      }
      
      PTRACE(4,"H46018\tSending H.245 Control PDU " << pdu);
      if (!WriteControlPDU(pdu))
        return false;
      
      m_h245Connect = true;
    }    
  }  
#endif

  return StartHandleControlChannel();
}


PBoolean H323Connection::HandleReceivedControlPDU(PBoolean readStatus, PPER_Stream & strm)
{
  PBoolean ok = FALSE;

  if (readStatus) {
    // Lock while checking for shutting down.
    if (LockReadWrite()) {
      // Process the received PDU
      PTRACE(4, "H245\tReceived TPKT: " << strm);
      ok = HandleControlData(strm);
      UnlockReadWrite(); // Unlock connection
    }
    else
      ok = InternalEndSessionCheck(strm);
  }
  else if (controlChannel->GetErrorCode() == PChannel::Timeout) {
    ok = TRUE;
  }
  else {
      PTRACE(1, "H245\tRead error: " << controlChannel->GetErrorText(PChannel::LastReadError)
          << " endSessionSent=" << endSessionSent);
    // If the connection is already shutting down then don't overwrite the
    // call end reason.  This could happen if the remote end point misbehaves
    // and simply closes the H.245 TCP connection rather than sending an
    // endSession.
    if(endSessionSent == FALSE)
      ClearCall(EndedByTransportFail);
    else
      PTRACE(1, "H245\tendSession already sent assuming H245 connection closed by remote side");
    ok = FALSE;
  }

  return ok;
}


PBoolean H323Connection::StartHandleControlChannel()
{

  // Start the TCS and MSD operations on new H.245 channel.
  if (!StartControlNegotiations())
    return FALSE;

  // Disable the signalling channels timeout for monitoring call status and
  // start up one in this thread instead. Then the Q.931 channel can be closed
  // without affecting the call.
  signallingChannel->SetReadTimeout(PMaxTimeInterval);
  controlChannel->SetReadTimeout(MonitorCallStatusTime);

  return TRUE;
}


void H323Connection::EndHandleControlChannel()
{
  PSafeLockReadOnly mutex(*this);

  // If we are the only link to the far end or if we have already sent our
  // endSession command then indicate that we have received endSession even
  // if we hadn't, because we are now never going to get one so there is no
  // point in having CleanUpOnCallEnd wait.
  if (signallingChannel == NULL || endSessionSent == TRUE)
    endSessionReceived.Signal();
}


void H323Connection::HandleControlChannel()
{
  
  // If have started separate H.245 channel then don't tunnel any more
  h245Tunneling = FALSE;
  
  if (!OnStartHandleControlChannel())
    return;

  PBoolean ok = TRUE;
  while (ok) {
    MonitorCallStatus();
    PPER_Stream strm;
    PBoolean readStatus = controlChannel->ReadPDU(strm);
    ok = HandleReceivedControlPDU(readStatus, strm);
  }

  EndHandleControlChannel();

  PTRACE(2, "H245\tControl channel closed.");
}


PBoolean H323Connection::InternalEndSessionCheck(PPER_Stream & strm)
{
  H323ControlPDU pdu;

  if (!pdu.Decode(strm)) {
    PTRACE(1, "H245\tInvalid PDU decode:\n  " << setprecision(2) << pdu);
    return false;
  }

  PTRACE(3, "H245\tChecking for end session on PDU: " << pdu.GetTagName()
         << ' ' << ((PASN_Choice &)pdu.GetObject()).GetTagName());

  if (pdu.GetTag() != H245_MultimediaSystemControlMessage::e_command)
    return true;

  H245_CommandMessage & command = pdu;
  if (command.GetTag() == H245_CommandMessage::e_endSessionCommand)
    endSessionReceived.Signal();
  return false;
}


PBoolean H323Connection::HandleControlData(PPER_Stream & strm)
{
  while (!strm.IsAtEnd()) {
    H323ControlPDU pdu;
    if (!pdu.Decode(strm)) {
      PTRACE(1, "H245\tInvalid PDU decode!"
                "\nRaw PDU:\n" << hex << setfill('0')
                               << setprecision(2) << strm
                               << dec << setfill(' ') <<
                "\nPartial PDU:\n  " << setprecision(2) << pdu);
      return true;
    }

    H323TraceDumpPDU("H245", false, strm, pdu, pdu, 0);

    if (!HandleControlPDU(pdu))
      return false;

    InternalEstablishedConnectionCheck();

    strm.ByteAlign();
  }

  return true;
}


PBoolean H323Connection::HandleControlPDU(const H323ControlPDU & pdu)
{
  switch (pdu.GetTag()) {
    case H245_MultimediaSystemControlMessage::e_request :
      return OnH245Request(pdu);

    case H245_MultimediaSystemControlMessage::e_response :
      return OnH245Response(pdu);

    case H245_MultimediaSystemControlMessage::e_command :
      return OnH245Command(pdu);

    case H245_MultimediaSystemControlMessage::e_indication :
      return OnH245Indication(pdu);
  }

  return OnUnknownControlPDU(pdu);
}


PBoolean H323Connection::OnUnknownControlPDU(const H323ControlPDU & pdu)
{
  PTRACE(2, "H245\tUnknown Control PDU: " << pdu);

  H323ControlPDU reply;
  reply.BuildFunctionNotUnderstood(pdu);
  return WriteControlPDU(reply);
}


PBoolean H323Connection::OnH245Request(const H323ControlPDU & pdu)
{
  const H245_RequestMessage & request = pdu;

  switch (request.GetTag()) {
    case H245_RequestMessage::e_masterSlaveDetermination :
      return masterSlaveDeterminationProcedure->HandleIncoming(request);

    case H245_RequestMessage::e_terminalCapabilitySet :
    {
      const H245_TerminalCapabilitySet & tcs = request;
      if (tcs.m_protocolIdentifier.GetSize() >= 6) {
        h245version = tcs.m_protocolIdentifier[5];
        h245versionSet = true;
        PTRACE(3, "H245\tSet protocol version to " << h245version);
      }
      return capabilityExchangeProcedure->HandleIncoming(tcs);
    }

    case H245_RequestMessage::e_openLogicalChannel :
      return logicalChannels->HandleOpen(request);

    case H245_RequestMessage::e_closeLogicalChannel :
      return logicalChannels->HandleClose(request);

    case H245_RequestMessage::e_requestChannelClose :
      return logicalChannels->HandleRequestClose(request);

    case H245_RequestMessage::e_requestMode :
      return requestModeProcedure->HandleRequest(request);

    case H245_RequestMessage::e_roundTripDelayRequest :
      return roundTripDelayProcedure->HandleRequest(request);

#if OPAL_H239
    case H245_RequestMessage::e_genericRequest :
    {
      const H245_GenericMessage & gen = request;
      if (H323GetCapabilityIdentifier(gen.m_messageIdentifier) == H239MessageOID)
        return OnH239Message(gen.m_subMessageIdentifier, gen.m_messageContent);
    }
#endif
  }

  return OnUnknownControlPDU(pdu);
}


PBoolean H323Connection::OnH245Response(const H323ControlPDU & pdu)
{
  const H245_ResponseMessage & response = pdu;

  switch (response.GetTag()) {
    case H245_ResponseMessage::e_masterSlaveDeterminationAck :
      return masterSlaveDeterminationProcedure->HandleAck(response);

    case H245_ResponseMessage::e_masterSlaveDeterminationReject :
      return masterSlaveDeterminationProcedure->HandleReject(response);

    case H245_ResponseMessage::e_terminalCapabilitySetAck :
      return capabilityExchangeProcedure->HandleAck(response);

    case H245_ResponseMessage::e_terminalCapabilitySetReject :
      return capabilityExchangeProcedure->HandleReject(response);

    case H245_ResponseMessage::e_openLogicalChannelAck :
      return logicalChannels->HandleOpenAck(response);

    case H245_ResponseMessage::e_openLogicalChannelReject :
      return logicalChannels->HandleReject(response);

    case H245_ResponseMessage::e_closeLogicalChannelAck :
      return logicalChannels->HandleCloseAck(response);

    case H245_ResponseMessage::e_requestChannelCloseAck :
      return logicalChannels->HandleRequestCloseAck(response);

    case H245_ResponseMessage::e_requestChannelCloseReject :
      return logicalChannels->HandleRequestCloseReject(response);

    case H245_ResponseMessage::e_requestModeAck :
      return requestModeProcedure->HandleAck(response);

    case H245_ResponseMessage::e_requestModeReject :
      return requestModeProcedure->HandleReject(response);

    case H245_ResponseMessage::e_roundTripDelayResponse :
      return roundTripDelayProcedure->HandleResponse(response);

#if OPAL_H239
    case H245_ResponseMessage::e_genericResponse :
    {
      const H245_GenericMessage & gen = response;
      if (H323GetCapabilityIdentifier(gen.m_messageIdentifier) == H239MessageOID)
        return OnH239Message(gen.m_subMessageIdentifier, gen.m_messageContent);
    }
#endif
  }

  return OnUnknownControlPDU(pdu);
}


PBoolean H323Connection::OnH245Command(const H323ControlPDU & pdu)
{
  const H245_CommandMessage & command = pdu;

  switch (command.GetTag()) {
    case H245_CommandMessage::e_sendTerminalCapabilitySet :
      return OnH245_SendTerminalCapabilitySet(command);

    case H245_CommandMessage::e_flowControlCommand :
      return OnH245_FlowControlCommand(command);

    case H245_CommandMessage::e_miscellaneousCommand :
      return OnH245_MiscellaneousCommand(command);

    case H245_CommandMessage::e_endSessionCommand :
      endSessionNeeded = true;
      endSessionReceived.Signal();
      switch (connectionState) {
        case EstablishedConnection :
          Release(EndedByRemoteUser);
          break;
        case AwaitingLocalAnswer :
          Release(EndedByCallerAbort);
          break;
        default :
          Release(EndedByRefusal);
      }
      return false;

#if OPAL_H239
    case H245_CommandMessage::e_genericCommand :
    {
      const H245_GenericMessage & gen = command;
      if (H323GetCapabilityIdentifier(gen.m_messageIdentifier) == H239MessageOID)
        return OnH239Message(gen.m_subMessageIdentifier, gen.m_messageContent);
    }
#endif
  }

  return OnUnknownControlPDU(pdu);
}


PBoolean H323Connection::OnH245Indication(const H323ControlPDU & pdu)
{
  const H245_IndicationMessage & indication = pdu;

  switch (indication.GetTag()) {
    case H245_IndicationMessage::e_masterSlaveDeterminationRelease :
      return masterSlaveDeterminationProcedure->HandleRelease(indication);

    case H245_IndicationMessage::e_terminalCapabilitySetRelease :
      return capabilityExchangeProcedure->HandleRelease(indication);

    case H245_IndicationMessage::e_openLogicalChannelConfirm :
      return logicalChannels->HandleOpenConfirm(indication);

    case H245_IndicationMessage::e_requestChannelCloseRelease :
      return logicalChannels->HandleRequestCloseRelease(indication);

    case H245_IndicationMessage::e_requestModeRelease :
      return requestModeProcedure->HandleRelease(indication);

    case H245_IndicationMessage::e_miscellaneousIndication :
      return OnH245_MiscellaneousIndication(indication);

    case H245_IndicationMessage::e_jitterIndication :
      return OnH245_JitterIndication(indication);

    case H245_IndicationMessage::e_userInput :
      OnUserInputIndication(indication);
      break;

#if OPAL_H239
    case H245_IndicationMessage::e_genericIndication :
    {
      const H245_GenericMessage & gen = indication;
      if (H323GetCapabilityIdentifier(gen.m_messageIdentifier) == H239MessageOID)
        return OnH239Message(gen.m_subMessageIdentifier, gen.m_messageContent);
    }
#endif
  }

  return true; // Do NOT call OnUnknownControlPDU for indications
}


PBoolean H323Connection::OnH245_SendTerminalCapabilitySet(
                 const H245_SendTerminalCapabilitySet & pdu)
{
  if (pdu.GetTag() == H245_SendTerminalCapabilitySet::e_genericRequest)
    return capabilityExchangeProcedure->Start(true);

  PTRACE(2, "H245\tUnhandled SendTerminalCapabilitySet: " << pdu);
  return true;
}


PBoolean H323Connection::OnH245_FlowControlCommand(
                 const H245_FlowControlCommand & pdu)
{
  PTRACE(3, "H245\tFlowControlCommand: scope=" << pdu.m_scope.GetTagName());

  long restriction;
  if (pdu.m_restriction.GetTag() == H245_FlowControlCommand_restriction::e_maximumBitRate)
    restriction = (const PASN_Integer &)pdu.m_restriction;
  else
    restriction = -1; // H245_FlowControlCommand_restriction::e_noRestriction

  switch (pdu.m_scope.GetTag()) {
    case H245_FlowControlCommand_scope::e_wholeMultiplex :
      OnLogicalChannelFlowControl(NULL, restriction);
      break;

    case H245_FlowControlCommand_scope::e_logicalChannelNumber :
    {
      H323Channel * chan = logicalChannels->FindChannel((unsigned)(const H245_LogicalChannelNumber &)pdu.m_scope, false);
      if (chan != NULL)
        OnLogicalChannelFlowControl(chan, restriction);
    }
  }

  return true;
}


PBoolean H323Connection::OnH245_MiscellaneousCommand(
                 const H245_MiscellaneousCommand & pdu)
{
  H323Channel * chan = logicalChannels->FindChannel((unsigned)pdu.m_logicalChannelNumber, false);
  if (chan != NULL)
    chan->OnMiscellaneousCommand(pdu.m_type);
  else
    PTRACE(2, "H245\tMiscellaneousCommand: is ignored chan=" << pdu.m_logicalChannelNumber
           << ", type=" << pdu.m_type.GetTagName());

  return true;
}


PBoolean H323Connection::OnH245_MiscellaneousIndication(
                 const H245_MiscellaneousIndication & pdu)
{
  H323Channel * chan = logicalChannels->FindChannel((unsigned)pdu.m_logicalChannelNumber, true);
  if (chan != NULL)
    chan->OnMiscellaneousIndication(pdu.m_type);
  else
    PTRACE(2, "H245\tMiscellaneousIndication is ignored. chan=" << pdu.m_logicalChannelNumber
           << ", type=" << pdu.m_type.GetTagName());

  return true;
}


PBoolean H323Connection::OnH245_JitterIndication(
                 const H245_JitterIndication & pdu)
{
  PTRACE(3, "H245\tJitterIndication: scope=" << pdu.m_scope.GetTagName());

  static const DWORD mantissas[8] = { 0, 1, 10, 100, 1000, 10000, 100000, 1000000 };
  static const DWORD exponents[8] = { 10, 25, 50, 75 };
  DWORD jitter = mantissas[pdu.m_estimatedReceivedJitterMantissa]*
                 exponents[pdu.m_estimatedReceivedJitterExponent]/10;

  int skippedFrameCount = -1;
  if (pdu.HasOptionalField(H245_JitterIndication::e_skippedFrameCount))
    skippedFrameCount = pdu.m_skippedFrameCount;

  int additionalBuffer = -1;
  if (pdu.HasOptionalField(H245_JitterIndication::e_additionalDecoderBuffer))
    additionalBuffer = pdu.m_additionalDecoderBuffer;

  switch (pdu.m_scope.GetTag()) {
    case H245_JitterIndication_scope::e_wholeMultiplex :
      OnLogicalChannelJitter(NULL, jitter, skippedFrameCount, additionalBuffer);
      break;

    case H245_JitterIndication_scope::e_logicalChannelNumber :
    {
      H323Channel * chan = logicalChannels->FindChannel((unsigned)(const H245_LogicalChannelNumber &)pdu.m_scope, false);
      if (chan != NULL)
        OnLogicalChannelJitter(chan, jitter, skippedFrameCount, additionalBuffer);
    }
  }

  return true;
}


#if OPAL_H239
bool H323Connection::OnH239Message(unsigned subMessage, const H245_ArrayOf_GenericParameter & params)
{
  switch (subMessage) {
    case 1 : // flowControlReleaseRequest GenericRequest
      return OnH239FlowControlRequest(H323GetGenericParameterInteger(params, 42),
                                      H323GetGenericParameterInteger(params, 41));

    case 2 : // flowControlReleaseResponse GenericResponse
      return OnH239FlowControlResponse(H323GetGenericParameterInteger(params, 42),
                                       H323GetGenericParameterBoolean(params, 127));

    case 3 : // presentationTokenRequest GenericRequest
      return OnH239PresentationRequest(H323GetGenericParameterInteger(params, 42),
                                       H323GetGenericParameterInteger(params, 43),
                                       H323GetGenericParameterInteger(params, 44));

    case 4 : // presentationTokenResponse GenericResponse
      return OnH239PresentationResponse(H323GetGenericParameterInteger(params, 42),
                                        H323GetGenericParameterInteger(params, 44),
                                        H323GetGenericParameterBoolean(params, 127));

    case 5 : // presentationTokenRelease GenericCommand
      return OnH239PresentationRelease(H323GetGenericParameterInteger(params, 42),
                                       H323GetGenericParameterInteger(params, 44));

    case 6 : // presentationTokenIndicateOwner GenericIndication 
      return OnH239PresentationIndication(H323GetGenericParameterInteger(params, 42),
                                          H323GetGenericParameterInteger(params, 44));
  }
  return true;
}


bool H323Connection::OnH239FlowControlRequest(unsigned logicalChannel, unsigned bitRate)
{
  PTRACE(3, "H239\tOnH239FlowControlRequest(" << logicalChannel << ',' << bitRate << ") - sending acknowledge");

  H323ControlPDU pdu;
  H245_ArrayOf_GenericParameter & params = pdu.BuildGenericResponse(H239MessageOID, 2).m_messageContent;
  //Viji    08/05/2009 Fix the order of the generic parameters as per Table 11 of H.239 ITU spec
  H323AddGenericParameterBoolean(params, 126, true); // Acknowledge
  H323AddGenericParameterInteger(params, 42, logicalChannel, H245_ParameterValue::e_unsignedMin);
  return WriteControlPDU(pdu);
}


bool H323Connection::OnH239FlowControlResponse(unsigned PTRACE_PARAM(logicalChannel), bool PTRACE_PARAM(rejected))
{
  PTRACE(3, "H239\tOnH239FlowControlResponse(" << logicalChannel << ',' << rejected << ')');

  return true;
}


bool H323Connection::OnH239PresentationRequest(unsigned logicalChannel, unsigned symmetryBreaking, unsigned terminalLabel)
{
  PTRACE(3, "H239\tOnH239PresentationRequest(" << logicalChannel << ',' << symmetryBreaking << ',' << terminalLabel << ") - sending acknowledge");

  H323ControlPDU pdu;
  H245_ArrayOf_GenericParameter & params = pdu.BuildGenericResponse(H239MessageOID, 4).m_messageContent;
  //Viji    08/05/2009 Fix the order of the generic parameters as per 
  //Table 13/H.239  - presentationTokenResponse syntax in the H.239 ITU spec
  H323AddGenericParameterBoolean(params, 126, true); // Acknowledge
  H323AddGenericParameterInteger(params, 44, terminalLabel, H245_ParameterValue::e_unsignedMin);
  H323AddGenericParameterInteger(params, 42, logicalChannel, H245_ParameterValue::e_unsignedMin);

  return WriteControlPDU(pdu);
}


bool H323Connection::SendH239PresentationRequest(unsigned logicalChannel, unsigned symmetryBreaking, unsigned terminalLabel)
{
  if (!GetRemoteH239Control()) {
    PTRACE(2, "H239\tCannot send presentation token request, not completed TCS or remote not capable");
    return false;
  }

  PTRACE(3, "H239\tSendH239PresentationRequest(" << logicalChannel << ',' << symmetryBreaking << ',' << terminalLabel << ')');

  H323ControlPDU pdu;
  H245_ArrayOf_GenericParameter & params = pdu.BuildGenericResponse(H239MessageOID, 3).m_messageContent;
  // Note order is important (Table 12/H.239)
  H323AddGenericParameterInteger(params, 44, terminalLabel, H245_ParameterValue::e_unsignedMin);
  H323AddGenericParameterInteger(params, 42, logicalChannel, H245_ParameterValue::e_unsignedMin);
  H323AddGenericParameterInteger(params, 43, symmetryBreaking, H245_ParameterValue::e_unsignedMin);

  return WriteControlPDU(pdu);
}


bool H323Connection::OnH239PresentationResponse(unsigned logicalChannel, unsigned terminalLabel, bool rejected)
{
  PTRACE(3, "H239\tOnH239PresentationResponse(" << logicalChannel << ',' << terminalLabel << ',' << rejected << ')');

  if (rejected)
    return true;

  H323ControlPDU pdu;
  H245_ArrayOf_GenericParameter & params = pdu.BuildGenericCommand(H239MessageOID, 5).m_messageContent;
  //Viji    08/05/2009 Fix the order of the generic parameters as per Table 14 of H.239 ITU spec
  H323AddGenericParameterInteger(params, 44, terminalLabel, H245_ParameterValue::e_unsignedMin);
  H323AddGenericParameterInteger(params, 42, logicalChannel, H245_ParameterValue::e_unsignedMin);
  return WriteControlPDU(pdu);
}


bool H323Connection::OnH239PresentationRelease(unsigned PTRACE_PARAM(logicalChannel), unsigned PTRACE_PARAM(terminalLabel))
{
  PTRACE(3, "H239\tOnH239PresentationRelease(" << logicalChannel << ',' << terminalLabel << ')');
  return true;
}


bool H323Connection::SendH239PresentationRelease(unsigned logicalChannel, unsigned terminalLabel)
{
  if (!GetRemoteH239Control()) {
    PTRACE(2, "H239\tCannot send presentation token release, not completed TCS or remote not capable");
    return false;
  }

  PTRACE(3, "H239\tSendH239PresentationRelease(" << logicalChannel << ',' << terminalLabel << ')');

  H323ControlPDU pdu;
  H245_ArrayOf_GenericParameter & params = pdu.BuildGenericResponse(H239MessageOID, 5).m_messageContent;
  // Note order is important (Table 12/H.239)
  H323AddGenericParameterInteger(params, 44, terminalLabel, H245_ParameterValue::e_unsignedMin);
  H323AddGenericParameterInteger(params, 42, logicalChannel, H245_ParameterValue::e_unsignedMin);

  return WriteControlPDU(pdu);
}


bool H323Connection::OnH239PresentationIndication(unsigned PTRACE_PARAM(logicalChannel), unsigned PTRACE_PARAM(terminalLabel))
{
  PTRACE(3, "H239\tOnH239PresentationIndication(" << logicalChannel << ',' << terminalLabel << ')');
  return true;
}
#endif


H323Channel * H323Connection::GetLogicalChannel(unsigned number, PBoolean fromRemote) const
{
  PSafeLockReadWrite mutex(*this);
  return logicalChannels->FindChannel(number, fromRemote);
}


H323Channel * H323Connection::FindChannel(unsigned rtpSessionId, PBoolean fromRemote) const
{
  PSafeLockReadWrite mutex(*this);
  return logicalChannels->FindChannelBySession(rtpSessionId, fromRemote);
}


bool H323Connection::Hold(bool fromRemote, bool placeOnHold)
{
  if (fromRemote) {
#if OPAL_H450
    if (!HoldCall(false))
      return false;
#endif
    PTRACE(2, "H323\tCannot place/retrieve call from remote hold");
    return false;
  }

#if OPAL_H450
  if (!(placeOnHold ? HoldCall(true) : RetrieveCall()))
    return false;
#endif

  if (!SendCapabilitySet(placeOnHold))
    return false;

  // Signal the manager that there is a hold
  OnHold(false, placeOnHold);
  return true;
}


PBoolean H323Connection::IsOnHold(bool fromRemote) 
{
#if OPAL_H450
  // Yes this looks around the wrong way, it isn't!
  return fromRemote ? (transmitterSidePaused || IsLocalHold()) : (remoteTransmitPaused || IsRemoteHold());
#else
  return fromRemote ? transmitterSidePaused : remoteTransmitPaused;
#endif
}


bool H323Connection::TransferConnection(const PString & remoteParty)
{
  PTRACE(3, "H323\tTransferring " << *this << " to " << remoteParty);

  PSafePtr<OpalCall> call = endpoint.GetManager().FindCallWithLock(remoteParty, PSafeReadOnly);
  if (call == NULL) {
#if OPAL_H450
    return TransferCall(remoteParty);
#else
    return ForwardCall(remoteParty);
#endif
  }

#if OPAL_H450
  for (PSafePtr<OpalConnection> connection = call->GetConnection(0); connection != NULL; ++connection) {
    PSafePtr<H323Connection> h323 = PSafePtrCast<OpalConnection, H323Connection>(connection);
    if (h323 != NULL)
      return TransferCall(h323->GetRemotePartyURL(), h323->GetToken());
  }
#endif

  PTRACE(2, "H323\tConsultation transfer requires other party to be H.323.");
  return false;
}


#if OPAL_H450

bool H323Connection::TransferCall(const PString & remoteParty,
                                  const PString & callIdentity)
{
  // According to H.450.4, if prior to consultation the primary call has been put on hold, the 
  // transferring endpoint shall first retrieve the call before Call Transfer is invoked.
  if (!callIdentity.IsEmpty() && IsLocalHold())
    RetrieveCall();

  return h4502handler->TransferCall(remoteParty, callIdentity);
}


void H323Connection::ConsultationTransfer(const PString & primaryCallToken)
{
  h4502handler->ConsultationTransfer(primaryCallToken);
}


void H323Connection::HandleConsultationTransfer(const PString & callIdentity,
                                                H323Connection& incoming)
{
  h4502handler->HandleConsultationTransfer(callIdentity, incoming);
}

PBoolean H323Connection::IsTransferringCall() const
{
  switch (h4502handler->GetState()) {
    case H4502Handler::e_ctAwaitIdentifyResponse :
    case H4502Handler::e_ctAwaitInitiateResponse :
    case H4502Handler::e_ctAwaitSetupResponse :
      return true;

    default :
      return false;
  }
}


PBoolean H323Connection::IsTransferredCall() const
{
   return (h4502handler->GetInvokeId() != 0 &&
           h4502handler->GetState() == H4502Handler::e_ctIdle) ||
           h4502handler->isConsultationTransferSuccess();
}

void H323Connection::HandleTransferCall(const PString & token,
                                        const PString & identity)
{
  if (!token.IsEmpty() || !identity)
    h4502handler->AwaitSetupResponse(token, identity);
}


int H323Connection::GetCallTransferInvokeId()
{
  return h4502handler->GetInvokeId();
}


void H323Connection::HandleCallTransferFailure(const int returnError)
{
  h4502handler->HandleCallTransferFailure(returnError);
}


void H323Connection::SetAssociatedCallToken(const PString& token)
{
  h4502handler->SetAssociatedCallToken(token);
}


void H323Connection::OnConsultationTransferSuccess(H323Connection& /*secondaryCall*/)
{
   h4502handler->SetConsultationTransferSuccess();
}


bool H323Connection::HoldCall(PBoolean localHold)
{
  if (!h4504handler->HoldCall(localHold))
    return false;

  holdMediaChannel = SwapHoldMediaChannels(holdMediaChannel);
  return true;
}


bool H323Connection::RetrieveCall()
{
  if (IsRemoteHold()) {
    PTRACE(4, "H4504\tRemote-end Call Hold not implemented.");
    return false;
  }

  // Is the current call on hold?
  if (!IsLocalHold())
    return true;

  if (!h4504handler->RetrieveCall())
    return false;

  holdMediaChannel = SwapHoldMediaChannels(holdMediaChannel);
  
  // Signal the manager that there is a retrieve 
  endpoint.OnHold(*this, false, false);
  return true;
}


void H323Connection::SetHoldMedia(PChannel * audioChannel)
{
  holdMediaChannel = PAssertNULL(audioChannel);
}


PBoolean H323Connection::IsMediaOnHold() const
{
  return holdMediaChannel != NULL;
}


PChannel * H323Connection::SwapHoldMediaChannels(PChannel * newChannel)
{
  PSafeLockReadWrite mutex(*this);
  if (IsMediaOnHold()) {
    if (PAssertNULL(newChannel) == NULL)
      return NULL;
  }

  PChannel * existingTransmitChannel = NULL;

  for (H245LogicalChannelDict::iterator it  = logicalChannels->GetChannels().begin();
                                        it != logicalChannels->GetChannels().end(); ++it) {
    H323Channel* channel = it->second.GetChannel();
    if (!channel)
      return NULL;

    unsigned int session_id = channel->GetSessionID();
    if (session_id == H323Capability::DefaultAudioSessionID || session_id == H323Capability::DefaultVideoSessionID) {
      const H323ChannelNumber & channelNumber = channel->GetNumber();

      if (!channelNumber.IsFromRemote()) { // Transmit channel
        OpalMediaStreamPtr stream = GetMediaStream(session_id, false);
        if (IsMediaOnHold()) {
//          H323Codec & codec = *channel->GetCodec();
//          existingTransmitChannel = codec.GetRawDataChannel();
      //FIXME
        }
        else {
          // Enable/mute the transmit channel depending on whether the remote end is held
          stream->SetPaused(IsLocalHold());
        }
      }
      else {
        OpalMediaStreamPtr stream = GetMediaStream(session_id, true);
        // Enable/mute the receive channel depending on whether the remote endis held
        stream->SetPaused(IsLocalHold());
      }
    }
  }

  return existingTransmitChannel;
}


PBoolean H323Connection::IsLocalHold() const
{
  return h4504handler->GetState() == H4504Handler::e_ch_NE_Held;
}


PBoolean H323Connection::IsRemoteHold() const
{
  return h4504handler->GetState() == H4504Handler::e_ch_RE_Held;
}


PBoolean H323Connection::IsCallOnHold() const
{
  return h4504handler->GetState() != H4504Handler::e_ch_Idle;
}


void H323Connection::IntrudeCall(unsigned capabilityLevel)
{
  h45011handler->IntrudeCall(capabilityLevel);
}


void H323Connection::HandleIntrudeCall(const PString & token,
                                       const PString & identity)
{
  if (!token.IsEmpty() || !identity)
    h45011handler->AwaitSetupResponse(token, identity);
}


PBoolean H323Connection::GetRemoteCallIntrusionProtectionLevel(const PString & intrusionCallToken,
                                                           unsigned intrusionCICL)
{
  return h45011handler->GetRemoteCallIntrusionProtectionLevel(intrusionCallToken, intrusionCICL);
}


void H323Connection::SetIntrusionImpending()
{
  h45011handler->SetIntrusionImpending();
}


void H323Connection::SetForcedReleaseAccepted()
{
  h45011handler->SetForcedReleaseAccepted();
}


void H323Connection::SetIntrusionNotAuthorized()
{
  h45011handler->SetIntrusionNotAuthorized();
}


void H323Connection::SendCallWaitingIndication(const unsigned nbOfAddWaitingCalls)
{
  h4506handler->AttachToAlerting(*alertingPDU, nbOfAddWaitingCalls);
}


#endif

PBoolean H323Connection::OnControlProtocolError(ControlProtocolErrors /*errorSource*/,
                                            const void * /*errorData*/)
{
  return true;
}

void H323Connection::OnSendCapabilitySet(H245_TerminalCapabilitySet & /*pdu*/)
{
}


PBoolean H323Connection::OnReceivedCapabilitySet(const H323Capabilities & remoteCaps,
                                             const H245_MultiplexCapability * muxCap,
                                             H245_TerminalCapabilitySetReject & /*rejectPDU*/)
{
  if (muxCap != NULL) {
    if (muxCap->GetTag() != H245_MultiplexCapability::e_h2250Capability) {
      PTRACE(1, "H323\tCapabilitySet contains unsupported multiplex.");
      return false;
    }

    const H245_H2250Capability & h225_0 = *muxCap;
    remoteMaxAudioDelayJitter = h225_0.m_maximumAudioDelayJitter;
  }

  if (remoteCaps.GetSize() == 0) {
    PTRACE(3, "H323\tReceived empty CapabilitySet, shutting down transmitters.");
    // Received empty TCS, so close all transmit channels
    for (H245LogicalChannelDict::iterator it  = logicalChannels->GetChannels().begin();
                                          it != logicalChannels->GetChannels().end(); ++it) {
      H245NegLogicalChannel & negChannel = it->second;
      H323Channel * channel = negChannel.GetChannel();
      if (channel != NULL && !channel->GetNumber().IsFromRemote())
        negChannel.Close();
    }
    transmitterSidePaused = true;
  }
  else {
    /* Received non-empty TCS, if was in paused state or this is the first TCS
       received so we should kill the fake table created by fast start kill
       the remote capabilities table so Merge() becomes a simple assignment */
    if (transmitterSidePaused || !capabilityExchangeProcedure->HasReceivedCapabilities())
      remoteCapabilities.RemoveAll();

    PINDEX previousCaps = remoteCapabilities.GetSize();

    if (!remoteCapabilities.Merge(remoteCaps))
      return false;

    if (transmitterSidePaused) {
      PTRACE(3, "H323\tReceived CapabilitySet while paused, re-starting transmitters.");
      transmitterSidePaused = false;
      connectionState = HasExecutedSignalConnect;
      capabilityExchangeProcedure->Start(true);
      masterSlaveDeterminationProcedure->Start(false);
    }
    else if (connectionState > HasExecutedSignalConnect && previousCaps > 0 && remoteCapabilities.GetSize() > previousCaps) {
      PTRACE(3, "H323\tReceived CapabilitySet with more media types.");
      OnSelectLogicalChannels();
    }
    else {
      if (localCapabilities.GetSize() > 0)
        capabilityExchangeProcedure->Start(false);
    }

    // Adjust the RF2388 transitter to remotes capabilities.
    H323Capability * capability = remoteCapabilities.FindCapability(H323_UserInputCapability::GetSubTypeName(H323_UserInputCapability::SignalToneRFC2833));
    rfc2833Handler->SetTxMediaFormat(capability != NULL ? capability->GetMediaFormat() : OpalMediaFormat());
  }

  return true;
}

bool H323Connection::SendCapabilitySet(PBoolean empty)
{
  PSafeLockReadWrite mutex(*this);
  if (!capabilityExchangeProcedure->Start(true, empty))
    return false;

  remoteTransmitPaused = empty;
  return true;
}


bool H323Connection::IsSendingCapabilitySet()
{
  PSafeLockReadOnly mutex(*this);
  return capabilityExchangeProcedure->IsSendingCapabilities();
}


void H323Connection::OnSetLocalCapabilities()
{
  if (capabilityExchangeProcedure->HasSentCapabilities())
    return;

  // create the list of media formats supported locally
  OpalMediaFormatList formats = GetLocalMediaFormats();
  if (formats.IsEmpty()) {
    PTRACE(2, "H323\tSetLocalCapabilities - no existing formats in call");
    return;
  }

#if OPAL_H239
  H323H239ControlCapability * h329Control = NULL;
  if (m_h239Control) {
    h329Control = new H323H239ControlCapability();
    formats += h329Control->GetMediaFormat();
  }
#endif

  // Remove those things not in the other parties media format list
  for (PINDEX c = 0; c < localCapabilities.GetSize(); c++) {
    H323Capability & capability = localCapabilities[c];
    OpalMediaFormat format = capability.GetMediaFormat();
    if (format.GetMediaType() == OpalMediaType::UserInput() || !formats.HasFormat(format)) {
      localCapabilities.Remove(&capability);
      c--;
    }
  }

  H323Capability::CapabilityDirection symmetric = H323Capability::e_Receive;
  PSafePtr<OpalConnection> otherConnection = GetOtherPartyConnection();
  if (otherConnection != NULL &&otherConnection->RequireSymmetricMediaStreams())
    symmetric = H323Capability::e_ReceiveAndTransmit;

  // Add those things that are in the other parties media format list
  static const OpalMediaType mediaList[] = {
    OpalMediaType::Audio(),
    OpalMediaType::Fax(),
    OpalMediaType::Video()
#if OPAL_HAS_H224
    , OpalH224MediaType::MediaType()
#endif
  };

  PINDEX simultaneous = P_MAX_INDEX;

  for (PINDEX m = 0; m < PARRAYSIZE(mediaList); m++) {
    if (m > 1)                    // First two (Audio/Fax) are in same simultaneous set
      simultaneous = P_MAX_INDEX; // After that each is in it's own simultaneous set in TCS

    for (OpalMediaFormatList::iterator format = formats.begin(); format != formats.end(); ++format) {
      if (format->GetMediaType() == mediaList[m] && format->IsTransportable())
        simultaneous = localCapabilities.AddMediaFormat(0, simultaneous, *format, symmetric);
    }
  }

#if OPAL_H239
  simultaneous = P_MAX_INDEX;
  for (OpalMediaFormatList::iterator format = formats.begin(); format != formats.end(); ++format) {
    if (localCapabilities.FindCapability(format->GetName()) != NULL &&
        format->GetOptionInteger(OpalVideoFormat::ContentRoleMaskOption()) != 0) {
      H323H239VideoCapability * newCap = new H323H239VideoCapability(*format);
      if (localCapabilities.FindCapability(*newCap) == NULL)
        simultaneous = localCapabilities.SetCapability(0, simultaneous, newCap);
      else
        delete newCap;
    }
  }

  if (h329Control != NULL) {
    if (localCapabilities.FindCapability(*h329Control) == NULL)
      localCapabilities.SetCapability(0, P_MAX_INDEX, h329Control);
    else
      delete h329Control;
  }
#endif

  OpalMediaFormatList::const_iterator rfc2833 = formats.FindFormat(OpalRFC2833);
  H323_UserInputCapability::AddAllCapabilities(localCapabilities, 0, P_MAX_INDEX, rfc2833 != formats.end());

  // Special test for the RFC2833 capability to get the correct dynamic payload type
  H323Capability * capability = localCapabilities.FindCapability(OpalRFC2833);
  if (capability != NULL && rfc2833 != formats.end())
    capability->SetPayloadType(rfc2833->GetPayloadType());

  // Adjust the RF2388 transitter to local capabilities.
  rfc2833Handler->SetRxMediaFormat(capability != NULL ? capability->GetMediaFormat() : OpalMediaFormat());

  PTRACE(3, "H323\tSetLocalCapabilities:\n" << setprecision(2) << localCapabilities);
}


#if OPAL_H239
bool H323Connection::GetRemoteH239Control() const
{
  return remoteCapabilities.FindCapability(H323H239ControlCapability()) != NULL;
}


OpalMediaFormatList H323Connection::GetRemoteH239Formats() const
{
  OpalMediaFormatList formats;

  for (PINDEX i = 0; i < remoteCapabilities.GetSize(); ++i) {
    const H323Capability & capability = remoteCapabilities[i];
    if (capability.GetMainType() == H323Capability::e_Video &&
        capability.GetSubType() == H245_VideoCapability::e_extendedVideoCapability)
      formats += capability.GetMediaFormat();
  }

  return formats;
}
#endif


PBoolean H323Connection::IsH245Master() const
{
  return masterSlaveDeterminationProcedure->IsMaster();
}


void H323Connection::StartRoundTripDelay()
{
  if (LockReadWrite()) {
    if (!IsReleased() &&
        masterSlaveDeterminationProcedure->IsDetermined() &&
        capabilityExchangeProcedure->HasSentCapabilities()) {
      if (roundTripDelayProcedure->IsRemoteOffline()) {
        PTRACE(1, "H245\tRemote failed to respond to PDU.");
        if (endpoint.ShouldClearCallOnRoundTripFail())
          Release(EndedByTransportFail);
      }
      else
        roundTripDelayProcedure->StartRequest();
    }
    UnlockReadWrite();
  }
}


PTimeInterval H323Connection::GetRoundTripDelay() const
{
  return roundTripDelayProcedure->GetRoundTripDelay();
}


void H323Connection::InternalEstablishedConnectionCheck()
{
  bool h245_available = masterSlaveDeterminationProcedure->IsDetermined() &&
                        capabilityExchangeProcedure->HasSentCapabilities() &&
                        capabilityExchangeProcedure->HasReceivedCapabilities();

  PTRACE(3, "H323\tInternalEstablishedConnectionCheck: "
            "connectionState=" << connectionState << " "
            "fastStartState=" << fastStartState << " "
            "H.245 is " << (h245_available ? "ready" : "unavailable"));

  if (h245_available)
    endSessionNeeded = true;

  // Check for if all the 245 conditions are met so can start up logical
  // channels and complete the connection establishment.
  if (fastStartState != FastStartAcknowledged) {
    if (!h245_available)
      return;

    // If we are early starting, start channels as soon as possible instead of
    // waiting for connect PDU
    if (earlyStart && IsH245Master() && FindChannel(H323Capability::DefaultAudioSessionID, false) == NULL)
      OnSelectLogicalChannels();
  }

#if 0
  if (h245_available && startT120) {
    if (remoteCapabilities.FindCapability("T.120") != NULL) {
      H323Capability * capability = localCapabilities.FindCapability("T.120");
      if (capability != NULL)
        OpenLogicalChannel(*capability, 3, H323Channel::IsBidirectional);
    }
    startT120 = false;
  }
#endif
  
  // Check if we have just been connected, or have come out of a transmitter side
  // paused, and have not already got an audio transmitter running via fast connect
  if (connectionState == HasExecutedSignalConnect && FindChannel(H323Capability::DefaultAudioSessionID, false) == NULL)
    OnSelectLogicalChannels(); // Start some media

  switch (GetPhase()) {
    case SetUpPhase :
    case ProceedingPhase :
    case AlertingPhase :
      if ( connectionState == HasExecutedSignalConnect &&
           FindChannel(H323Capability::DefaultAudioSessionID, true) != NULL &&
          (FindChannel(H323Capability::DefaultVideoSessionID, true) != NULL ||
              localCapabilities.FindCapability(H323Capability::e_Video) == NULL ||
              remoteCapabilities.FindCapability(H323Capability::e_Video) == NULL))
        OnConnectedInternal();
      break;

    case ConnectedPhase :
      SetPhase(EstablishedPhase);
      OnEstablished();
      // Set established in next case

    case EstablishedPhase :
      connectionState = EstablishedConnection; // Keep in sync
      break;

    default :
      break;
  }
}


OpalMediaFormatList H323Connection::GetMediaFormats() const
{
  OpalMediaFormatList list;

  list += remoteCapabilities.GetMediaFormats();

#if OPAL_H239
  OpalMediaFormatList h239 = GetRemoteH239Formats();
  for (OpalMediaFormatList::iterator format = h239.begin(); format != h239.end(); ++format)
    list += *format;
#endif

  AdjustMediaFormats(false, NULL, list);

  if (  IsH245Master() &&
        (
          (localCapabilities.GetSize() > 0 &&
           localCapabilities[0].GetCapabilityDirection() == H323Capability::e_ReceiveAndTransmit)
          ||
          (remoteCapabilities.GetSize() > 0 &&
           remoteCapabilities[0].GetCapabilityDirection() == H323Capability::e_ReceiveAndTransmit)
        )) {
    /* If symmetry is required and we are the master we re-order their formats
       to OUR order. This avoids a masterSlaveConflict (assuming other end is
       working correctly) and some unecessaary open logical channel round
       trips.
       
       Techniically, we should be a bit more sophisticated in determining
       symmtery requirement, but 99.9% of the time of the first, if the entry
       is symmetric, they all are. */
    PStringArray order;
    OpalMediaFormatList local = localCapabilities.GetMediaFormats();
    for (OpalMediaFormatList::iterator it = local.begin(); it != local.end(); ++it)
      order.AppendString(it->GetName());
    list.Reorder(order);
    PTRACE(2, "H323\tRe-ordered media formats due to symmetry rules on " << *this);
  }

  return list;
}


unsigned H323Connection::GetNextSessionID(const OpalMediaType & mediaType, bool isSource)
{
  if (GetMediaStream(mediaType, isSource) == NULL) {
    OpalMediaStreamPtr mediaStream = GetMediaStream(mediaType, !isSource);
    return mediaStream != NULL ? mediaStream->GetSessionID() : mediaType->GetDefaultSessionId();
  }

  unsigned sessionID = 1000000;
  if (IsH245Master()) {
    sessionID = 4;
    while (OpalMediaType::GetDefinition(sessionID) == NULL)
      ++sessionID;
  }

  while (GetMediaStream(sessionID, true) != NULL || GetMediaStream(sessionID, false) != NULL)
    ++sessionID;

  return sessionID;
}


#if OPAL_FAX
bool H323Connection::SwitchFaxMediaStreams(bool enableFax)
{
  if (m_faxMediaStreamsSwitchState != e_NotSwitchingFaxMediaStreams) {
    PTRACE(2, "H323\tNested call to SwitchFaxMediaStreams on " << *this);
    return false;
  }

  if (enableFax && remoteCapabilities.FindCapability(OpalT38) == NULL) {
    PTRACE(3, "H323\tRemote does not have T.38 capabilities on " << *this);
    return false;
  }

  if (enableFax ? (GetMediaStream(H323Capability::DefaultDataSessionID, true) != NULL)
                : (GetMediaStream(H323Capability::DefaultAudioSessionID, true) != NULL)) {
    PTRACE(3, "H323\tAlready switched media streams to " << (enableFax ? "fax" : "audio") << " on " << *this);
    return false;
  }

  PTRACE(3, "H323\tSwitchFaxMediaStreams to " << (enableFax ? "fax" : "audio") << " on " << *this);
  if (!RequestModeChangeT38(enableFax ? OpalT38 : OpalG711uLaw))
    return false;

  m_faxMediaStreamsSwitchState = enableFax ? e_SwitchingToFaxMediaStreams : e_SwitchingFromFaxMediaStreams;
  return true;
}
#endif


OpalMediaStreamPtr H323Connection::OpenMediaStream(const OpalMediaFormat & mediaFormat, unsigned sessionID, bool isSource)
{
  // See if already opened
  OpalMediaStreamPtr stream = GetMediaStream(sessionID, isSource);
  if (stream != NULL && stream->IsOpen()) {
    if (stream->GetMediaFormat() == mediaFormat) {
      PTRACE(3, "H323\tOpenMediaStream (already opened) for session " << sessionID << " on " << *this);
      return stream;
    }

    if (isSource) {
      stream = CreateMediaStream(mediaFormat, sessionID, isSource);
      if (stream == NULL) {
        PTRACE(1, "H323\tCreateMediaStream returned NULL for session " << sessionID << " on " << *this);
        return NULL;
      }
      mediaStreams.Append(stream);

      // Channel from other side, do RequestModeChange
      RequestModeChange(mediaFormat);
      return stream;
    }

    // Changing the media format, needs to close and re-open the stream
    stream->Close();
    stream.SetNULL();
  }

  if ( isSource &&
      !ownerCall.IsEstablished() &&
      (GetAutoStart(mediaFormat.GetMediaType())&OpalMediaType::Receive) == 0) {
    PTRACE(3, "H323\tOpenMediaStream auto start disabled, refusing " << mediaFormat.GetMediaType() << " open");
    return NULL;
  }

  for (H323LogicalChannelList::iterator iterChan = fastStartChannels.begin();
                                        iterChan != fastStartChannels.end(); ++iterChan) {
    if (iterChan->GetDirection() == (isSource ? H323Channel::IsReceiver : H323Channel::IsTransmitter) &&
        iterChan->GetCapability().GetMediaFormat() == mediaFormat) {
      PTRACE(4, "H323\tOpenMediaStream fast opened for session " << sessionID);
      stream = CreateMediaStream(mediaFormat, sessionID, isSource);
      iterChan->SetMediaStream(stream);
      logicalChannels->Add(*iterChan);
      break;
    }
  }

  if (stream == NULL) {
    H323Channel * channel = FindChannel(sessionID, isSource);
    if (channel == NULL) {
      // Logical channel not open, if receiver that is an error
      if (isSource) {
        PTRACE(2, "H323\tOpenMediaStream canot have logical channel for session " << sessionID);
        return NULL;
      }

      if (!masterSlaveDeterminationProcedure->IsDetermined() ||
          !capabilityExchangeProcedure->HasSentCapabilities() ||
          !capabilityExchangeProcedure->HasReceivedCapabilities()) {
        PTRACE(2, "H323\tOpenMediaStream cannot (H.245 unavailable) open logical channel for " << mediaFormat);
        return NULL;
      }

      // If transmitter, send OpenLogicalChannel using the capability associated with the media format
      H323Capability * capability = remoteCapabilities.FindCapability(mediaFormat.GetName());
      if (capability == NULL) {
        PTRACE(2, "H323\tOpenMediaStream could not find capability for " << mediaFormat);
        return NULL;
      }

      capability->UpdateMediaFormat(mediaFormat);

      if (!OpenLogicalChannel(*capability, sessionID, H323Channel::IsTransmitter)) {
        PTRACE(2, "H323\tOpenMediaStream could not open logical channel for " << mediaFormat);
        return NULL;
      }
      channel = FindChannel(sessionID, isSource);
      if (PAssertNULL(channel) == NULL)
        return NULL;
    }

    stream = channel->GetMediaStream();
    if (stream == NULL) {
      PTRACE(2, "H323\tCould not open stream for logical channel " << channel->GetNumber());
      channel->Close();
      return NULL;
    }

    PTRACE(3, "H323\tOpenMediaStream using channel " << channel->GetNumber() << " for session " << sessionID);
  }

  if (stream->Open()) {
    OpalMediaFormat adjustedMediaFormat = mediaFormat;
    adjustedMediaFormat.SetPayloadType(stream->GetMediaFormat().GetPayloadType());
    stream->UpdateMediaFormat(adjustedMediaFormat);

    if (OnOpenMediaStream(*stream)) {
      mediaStreams.Append(stream);
      return stream;
    }

    PTRACE(2, "H323\tOnOpenMediaStream failed for " << mediaFormat << ", session " << sessionID);
  }
  else {
    PTRACE(2, "H323\tMedia stream open failed for " << mediaFormat << ", session " << sessionID);
  }

  stream->Close();

  return NULL;
}


bool H323Connection::CloseMediaStream(OpalMediaStream & stream)
{
  PSafeLockReadWrite mutex(*this);

  // For a channel, this gets called twice. The first time we send CLC to remote
  // The second time is after CLC Ack or a timeout occurs, then we call the ancestor
  // function to clean up the media stream.
  if (!IsReleased()) {
    for (H245LogicalChannelDict::iterator it  = logicalChannels->GetChannels().begin();
                                          it != logicalChannels->GetChannels().end(); ++it) {
      H323Channel * channel = it->second.GetChannel();
      if (channel != NULL && channel->GetMediaStream() == &stream) {
        const H323ChannelNumber & number = channel->GetNumber();
        if (!logicalChannels->Close(number, number.IsFromRemote()))
          return false;
      }
    }
  }
  return OpalRTPConnection::CloseMediaStream(stream);
}


bool H323Connection::OnMediaCommand(OpalMediaStream & stream, const OpalMediaCommand & command)
{
  H323Channel * channel = FindChannel(stream.GetSessionID(), true);
  if (channel == NULL) {
    PTRACE(4, "H.323\tOnMediaCommand, no channel found for session " << stream.GetSessionID());
  }
  else {
    const OpalMediaFlowControl * flow = dynamic_cast<const OpalMediaFlowControl *>(&command);
    if (flow != NULL) {
      H323ControlPDU pdu;
      pdu.BuildFlowControlCommand(channel->GetNumber(), flow->GetMaxBitRate()/100);
      WriteControlPDU(pdu);
      return true;
    }

#if OPAL_VIDEO
    if (PIsDescendant(&command, OpalVideoUpdatePicture)) {
      if (m_h245FastUpdatePictureTimer.IsRunning()) {
        PTRACE(4, "H.323\tRecent H.245 VideoFastUpdatePicture was sent, not sending another");
        return true;
      }

      H323ControlPDU pdu;
      pdu.BuildMiscellaneousCommand(channel->GetNumber(), H245_MiscellaneousCommand_type::e_videoFastUpdatePicture);
      WriteControlPDU(pdu);

#if OPAL_STATISTICS
      m_VideoUpdateRequestsSent++;
#endif

      return true;
    }
#endif // OPAL_VIDEO
  }

  return OpalRTPConnection::OnMediaCommand(stream, command);
}


bool H323Connection::GetMediaTransportAddresses(const OpalMediaType & mediaType,
                                          OpalTransportAddressArray & transports) const
{
  // Can only do media bypass if we have fast connect
  for (H323LogicalChannelList::const_iterator channel = fastStartChannels.begin(); channel != fastStartChannels.end(); ++channel) {
    if (channel->GetCapability().GetMediaFormat().GetMediaType() == mediaType) {
      OpalTransportAddress media, control;
      if (channel->GetMediaTransportAddress(media, control)) {
        transports.AppendAddress(media);
        transports.AppendAddress(control);
        PTRACE(3, "H323\tGetMediaTransport for " << mediaType << " found "
                  "media=" << media << ", control=" << control);
        return true;
      }
    }
  }

  return OpalRTPConnection::GetMediaTransportAddresses(mediaType, transports);
}


void H323Connection::OpenFastStartChannel(unsigned sessionID, H323Channel::Directions direction)
{
  for (H323LogicalChannelList::iterator channel = fastStartChannels.begin(); channel != fastStartChannels.end(); ++channel) {
    if (channel->GetSessionID() == sessionID && channel->GetDirection() == direction) {
      unsigned error;
      if (OnCreateLogicalChannel(channel->GetCapability(), direction, error)) {
        PTRACE(3, "H225\tOpening fast start channel for " << channel->GetCapability());
        if (channel->Open())
          break;
      }
    }
  }
}


void H323Connection::OnSelectLogicalChannels()
{
  PTRACE(3, "H245\tDefault OnSelectLogicalChannels, " << fastStartState);

#if OPAL_VIDEO
  OpalMediaType::AutoStartMode autoStartVideo = GetAutoStart(OpalMediaType::Video());
#endif
#if OPAL_T38_CAPABILITY
  OpalMediaType::AutoStartMode autoStartFax = GetAutoStart(OpalMediaType::Fax());
#endif
#if OPAL_HAS_H224
  OpalMediaType::AutoStartMode autoStartH224 = GetAutoStart(GetOpalH224_H323AnnexQ().GetMediaType());
#endif

  // Select the first codec that uses the "standard" audio session.
  switch (fastStartState) {
    default : //FastStartDisabled :
      SelectDefaultLogicalChannel(OpalMediaType::Audio(), H323Capability::DefaultAudioSessionID);
#if OPAL_VIDEO
      if ((autoStartVideo&OpalMediaType::Transmit) != 0)
        SelectDefaultLogicalChannel(OpalMediaType::Video(), H323Capability::DefaultVideoSessionID);
      else {
        PTRACE(4, "H245\tOnSelectLogicalChannels, video not auto-started");
      }
#endif
#if OPAL_T38_CAPABILITY
      if ((autoStartFax&OpalMediaType::Transmit) != 0)
        SelectDefaultLogicalChannel(OpalMediaType::Fax(), H323Capability::DefaultDataSessionID);
      else {
        PTRACE(4, "H245\tOnSelectLogicalChannels, fax not auto-started");
      }
#endif
#if OPAL_HAS_H224
      if ((autoStartH224&OpalMediaType::Transmit) != 0)
        SelectDefaultLogicalChannel(OpalH224MediaType::MediaType(), H323Capability::DefaultH224SessionID);
      else {
        PTRACE(4, "H245\tOnSelectLogicalChannels, H.224 camera control not auto-started");
      }
#endif
      break;

    case FastStartInitiate :
      SelectFastStartChannels(H323Capability::DefaultAudioSessionID, true, true);
#if OPAL_VIDEO
      if (autoStartVideo != OpalMediaType::DontOffer)
        SelectFastStartChannels(H323Capability::DefaultVideoSessionID,
                                (autoStartVideo&OpalMediaType::Transmit) != 0,
                                (autoStartVideo&OpalMediaType::Receive) != 0);
#endif
#if OPAL_T38_CAPABILITY
      if (autoStartFax != OpalMediaType::DontOffer)
        SelectFastStartChannels(H323Capability::DefaultDataSessionID,
                                (autoStartFax&OpalMediaType::Transmit) != 0,
                                (autoStartFax&OpalMediaType::Receive) != 0);
#endif
#if OPAL_HAS_H224
      if (autoStartH224 != OpalMediaType::DontOffer)
        SelectFastStartChannels(H323Capability::DefaultH224SessionID,
                                (autoStartH224&OpalMediaType::Transmit) != 0,
                                (autoStartH224&OpalMediaType::Receive) != 0);
#endif
      break;

    case FastStartResponse :
      OpenFastStartChannel(H323Capability::DefaultAudioSessionID, H323Channel::IsTransmitter);
      OpenFastStartChannel(H323Capability::DefaultAudioSessionID, H323Channel::IsReceiver);
#if OPAL_VIDEO
      if ((autoStartVideo&OpalMediaType::Transmit) != 0)
        OpenFastStartChannel(H323Capability::DefaultVideoSessionID, H323Channel::IsTransmitter);
      if ((autoStartVideo&OpalMediaType::Receive) != 0)
        OpenFastStartChannel(H323Capability::DefaultVideoSessionID, H323Channel::IsReceiver);
#endif
#if OPAL_T38_CAPABILITY
      if ((autoStartFax&OpalMediaType::Transmit) != 0)
        OpenFastStartChannel(H323Capability::DefaultDataSessionID, H323Channel::IsTransmitter);
      if ((autoStartFax&OpalMediaType::Receive) != 0)
        OpenFastStartChannel(H323Capability::DefaultDataSessionID, H323Channel::IsReceiver);
#endif
#if OPAL_HAS_H224
      if ((autoStartH224&OpalMediaType::Transmit) != 0)
        OpenFastStartChannel(H323Capability::DefaultH224SessionID, H323Channel::IsTransmitter);
      if ((autoStartH224&OpalMediaType::Receive) != 0)
        OpenFastStartChannel(H323Capability::DefaultH224SessionID, H323Channel::IsReceiver);
#endif
      break;
  }
}


void H323Connection::SelectDefaultLogicalChannel(const OpalMediaType & mediaType, unsigned sessionID)
{
  if (FindChannel(sessionID, false))
    return;

  PSafePtr<OpalConnection> otherConnection = GetOtherPartyConnection();
  if (otherConnection == NULL) {
    PTRACE(2, "H323\tSelectLogicalChannel(" << sessionID << ") cannot start channel without second connection in call.");
    return;
  }

  if (!ownerCall.OpenSourceMediaStreams(*otherConnection, mediaType, sessionID)) {
    PTRACE(2, "H323\tSelectLogicalChannel(" << sessionID << ") could not start media stream.");
  }
}


void H323Connection::SelectFastStartChannels(unsigned sessionID,
                                             PBoolean transmitter,
                                             PBoolean receiver)
{
  // Select all of the fast start channels to offer to the remote when initiating a call.
  for (PINDEX i = 0; i < localCapabilities.GetSize(); i++) {
    H323Capability & capability = localCapabilities[i];
    if (capability.GetDefaultSessionID() == sessionID) {
      if (receiver) {
        if (!OpenLogicalChannel(capability, sessionID, H323Channel::IsReceiver)) {
          PTRACE(2, "H323\tOnSelectLogicalChannels, OpenLogicalChannel rx failed: " << capability);
        }
      }
      if (transmitter) {
        if (!OpenLogicalChannel(capability, sessionID, H323Channel::IsTransmitter)) {
          PTRACE(2, "H323\tOnSelectLogicalChannels, OpenLogicalChannel tx failed: " << capability);
        }
      }
    }
  }
}

void H323Connection::SendFlowControlCommand(unsigned channelNumber, unsigned newBitRate)
{
  H323ControlPDU pdu;
  pdu.BuildFlowControlCommand(channelNumber,newBitRate);
  WriteControlPDU(pdu);
}


PBoolean H323Connection::OpenLogicalChannel(const H323Capability & capability,
                                        unsigned sessionID,
                                        H323Channel::Directions dir)
{
  PSafeLockReadWrite mutex(*this);

  switch (fastStartState) {
    default : // FastStartDisabled
      if (dir == H323Channel::IsReceiver)
        return false;

      // Traditional H245 handshake
      return logicalChannels->Open(capability, sessionID);

    case FastStartResponse :
      // Do not use OpenLogicalChannel for starting these.
      return false;

    case FastStartInitiate :
      break;
  }

  /*If starting a receiver channel and are initiating the fast start call,
    indicated by the remoteCapabilities being empty, we do a "trial"
    listen on the channel. That is, for example, the UDP sockets are created
    to receive data in the RTP session, but no thread is started to read the
    packets and pass them to the codec. This is because at this point in time,
    we do not know which of the codecs is to be used, and more than one thread
    cannot read from the RTP ports at the same time.
  */
  H323Channel * channel = capability.CreateChannel(*this, dir, sessionID, NULL);
  if (channel == NULL)
    return false;

  if (dir != H323Channel::IsReceiver)
    channel->SetNumber(logicalChannels->GetNextChannelNumber());

  fastStartChannels.Append(channel);
  return true;
}


PBoolean H323Connection::OnOpenLogicalChannel(const H245_OpenLogicalChannel & openPDU,
                                          H245_OpenLogicalChannelAck & ackPDU,
                                          unsigned & errorCode,
                                          H323Channel & channel)
{
  unsigned sessionID = channel.GetSessionID();

  PTRACE(4,"H323\tOnOpenLogicalChannel: sessionId=" << sessionID);

  // If get a OLC via H.245 stop trying to do fast start
  fastStartState = FastStartDisabled;
  if (!fastStartChannels.IsEmpty()) {
    fastStartChannels.RemoveAll();
#if OPAL_H460_NAT
    m_NATSockets.clear();
#endif // OPAL_H460_NAT
    PTRACE(3, "H245\tReceived early start OLC, aborting fast start");
  }

#if OPAL_H460
  if (openPDU.HasOptionalField(H245_OpenLogicalChannel::e_genericInformation)) {
    OnReceiveOLCGenericInformation(sessionID,openPDU.m_genericInformation);

    if (OnSendingOLCGenericInformation(sessionID,ackPDU.m_genericInformation,true))
      ackPDU.IncludeOptionalField(H245_OpenLogicalChannelAck::e_genericInformation);
  }
#endif

  // Detect symmetry issues
  H323Capability * capability = remoteCapabilities.FindCapability(channel.GetCapability());
  if (capability == NULL || capability->GetCapabilityDirection() != H323Capability::e_ReceiveAndTransmit) {
    capability = localCapabilities.FindCapability(channel.GetCapability());
    if (capability == NULL || capability->GetCapabilityDirection() != H323Capability::e_ReceiveAndTransmit)
      return true; // No symmetry requested
  }

  // Yep are symmetrical, see if opening something different
  H323Channel * otherChannel = FindChannel(sessionID, false);
  if (otherChannel == NULL)
    return true; // No other channel so symmtery yet to raise it's ugly head

  if (channel.GetCapability() == otherChannel->GetCapability())
    return true; // Is symmetric, all OK!

  /* The correct protocol thing to do is reject the channel if we are the
     master. However, NetMeeting will not then re-open a channel, so we act
     like we are a slave and close our end instead. */
  if (IsH245Master() && GetRemoteApplication().Find("NetMeeting") == P_MAX_INDEX) {
    errorCode = H245_OpenLogicalChannelReject_cause::e_masterSlaveConflict;
    return false;
  }

  OnConflictingLogicalChannel(channel);
  return true;
}


PBoolean H323Connection::OnReceiveOLCGenericInformation(unsigned sessionID,
                          const H245_ArrayOf_GenericInformation & alternate) const
{
  PBoolean success = false;

#if OPAL_H460_NAT
  PTRACE(4,"Handling Generic OLC Session " << sessionID );
  for (PINDEX i = 0; i < alternate.GetSize(); i++) {
    const H245_GenericInformation & info = alternate[i];
    const H245_CapabilityIdentifier & id = info.m_messageIdentifier;
    if (id.GetTag() != H245_CapabilityIdentifier::e_standard)
      break;

    const PASN_ObjectId & oid = id;
    const H245_ArrayOf_GenericParameter & msg = info.m_messageContent;
    if (m_H46019enabled && (oid.AsString() == H46019OID)) {
      H245_GenericParameter & val = msg[0];
      if (val.m_parameterValue.GetTag() != H245_ParameterValue::e_octetString)
        break;

      PASN_OctetString & raw = val.m_parameterValue;
      PPER_Stream pdu(raw);
      H46019_TraversalParameters params;
      if (!params.Decode(pdu)) {
        PTRACE(2,"H46019\tError decoding Traversal Parameters!");
        break;
      }

      PTRACE(4,"H46019\tTraversal Parameters:\n" << params);

      H323TransportAddress RTPaddress;
      H323TransportAddress RTCPaddress;
      if (params.HasOptionalField(H46019_TraversalParameters::e_keepAliveChannel)) {
        H245_TransportAddress & k = params.m_keepAliveChannel;
        RTPaddress = H323TransportAddress(k);
        PIPSocket::Address add; WORD port;
        RTPaddress.GetIpAndPort(add,port);
        RTCPaddress = H323TransportAddress(add,port+1);  // Compute the RTCP Address
      }

      unsigned payload = 0;
      if (params.HasOptionalField(H46019_TraversalParameters::e_keepAlivePayloadType)) {
        PASN_Integer & p = params.m_keepAlivePayloadType;
        payload = p;
      }

      unsigned ttl = 0;
      if (params.HasOptionalField(H46019_TraversalParameters::e_keepAliveInterval)) {
        H225_TimeToLive & a = params.m_keepAliveInterval;
        ttl = a;
      }

      unsigned muxId = 0;
      if (params.HasOptionalField(H46019_TraversalParameters::e_multiplexID)) {
        muxId = params.m_multiplexID;
      }

      std::map<unsigned,NAT_Sockets>::const_iterator sockets_iter = m_NATSockets.find(sessionID);
      if (sockets_iter != m_NATSockets.end()) {
        NAT_Sockets sockets = sockets_iter->second;
        //PTRACE ( 5, "H460\tFound Sockets in m_NATSockets: " << (int)sockets.rtp << " " << (int)sockets.rtcp);
        ((H46019UDPSocket *)sockets.rtp)->Activate(RTPaddress,payload,ttl, muxId);
        ((H46019UDPSocket *)sockets.rtcp)->Activate(RTCPaddress,payload,ttl, muxId);
      }

      success = true;
    }
  }
#endif // OPAL_H460_NAT

  return success;
}


PBoolean H323Connection::OnSendingOLCGenericInformation(const unsigned & sessionID,
                              H245_ArrayOf_GenericInformation & generic, PBoolean isAck) const
{
  PTRACE(4,"Set Generic " << (isAck ? "OLCack" : "OLC") << " Session " << sessionID );
#if OPAL_H460_NAT
  if (m_H46019enabled) {
    unsigned payload=0; unsigned ttl=0;
    std::map<unsigned,NAT_Sockets>::const_iterator sockets_iter = m_NATSockets.find(sessionID);
    if (sockets_iter != m_NATSockets.end()) {
      NAT_Sockets sockets = sockets_iter->second;
      //PTRACE ( 5, "H460\tFound Sockets in m_NATSockets: " << (int)sockets.rtp << " " << (int)sockets.rtcp);
      H46019UDPSocket * rtp = ((H46019UDPSocket *)sockets.rtp);
      H46019UDPSocket * rtcp = ((H46019UDPSocket *)sockets.rtcp);
      if (rtp->GetPingPayload() == 0)
        rtp->SetPingPayLoad(defH46019payload);
      payload = rtp->GetPingPayload();

      if (rtp->GetTTL() == 0)
        rtp->SetTTL(ttl);
      ttl = rtp->GetTTL();

      if (isAck) {
        rtp->Activate();  // Start the RTP Channel if not already started
        rtcp->Activate();  // Start the RTCP Channel if not already started
      }

    } else {
      PTRACE(4,"H46019\tERROR NAT Socket not found for " << sessionID << " ABORTING!" );
      return false;
    }

    H245_GenericInformation info;
    H245_CapabilityIdentifier & id = info.m_messageIdentifier;
    id.SetTag(H245_CapabilityIdentifier::e_standard);
    PASN_ObjectId & oid = id;
    oid.SetValue(H46019OID);

    bool h46019msg = false;
    H46019_TraversalParameters params;
    if (/*!isAck ||*/ payload > 0) {
      params.IncludeOptionalField(H46019_TraversalParameters::e_keepAlivePayloadType);
      PASN_Integer & p = params.m_keepAlivePayloadType;
      p = payload;
      h46019msg = true;
    }
    if (/*isAck &&*/ ttl > 0) {
      params.IncludeOptionalField(H46019_TraversalParameters::e_keepAliveInterval);
      H225_TimeToLive & a = params.m_keepAliveInterval;
      a = ttl;
      h46019msg = true;
    }

    if (h46019msg) {
      PTRACE(5,"H46019\tTraversal Parameters:\n" << params);
      info.IncludeOptionalField(H245_GenericMessage::e_messageContent);
      H245_ArrayOf_GenericParameter & msg = info.m_messageContent;
      H245_GenericParameter genericParameter;
      H245_ParameterIdentifier & idm = genericParameter.m_parameterIdentifier;
      idm.SetTag(H245_ParameterIdentifier::e_standard);
      PASN_Integer & idx = idm;
      idx = 1;
      genericParameter.m_parameterValue.SetTag(H245_ParameterValue::e_octetString);
      H245_ParameterValue & octetValue = genericParameter.m_parameterValue;
      PASN_OctetString & raw = octetValue;
      raw.EncodeSubType(params);
      msg.SetSize(1);
      msg[0] = genericParameter;
    }
    PINDEX sz = generic.GetSize();
    generic.SetSize(sz+1);
    generic[sz] = info;

    if (generic.GetSize() > 0)
      return true;
  }
#endif // OPAL_H460_NAT

  return false;
}



PBoolean H323Connection::OnConflictingLogicalChannel(H323Channel & conflictingChannel)
{
  unsigned session = conflictingChannel.GetSessionID();
  PTRACE(2, "H323\tLogical channel " << conflictingChannel
         << " conflict on session " << session
         << ", we are " << (IsH245Master() ? "master" : " slave")
         << ", codec: " << conflictingChannel.GetCapability());

  /* Matrix of conflicts:
       Local EP is master and conflicting channel from remote (OLC)
          Reject remote transmitter (function is not called)
       Local EP is master and conflicting channel to remote (OLCAck)
          Should not happen (function is not called)
       Local EP is slave and conflicting channel from remote (OLC)
          Close sessions reverse channel from remote
          Start new reverse channel using codec in conflicting channel
          Accept the OLC for masters transmitter
       Local EP is slave and conflicting channel to remote (OLCRej)
          Start transmitter channel using codec in sessions reverse channel

      Upshot is this is only called if a slave and require a restart of
      some channel. Possibly closing channels as master has precedence.
   */

  PBoolean fromRemote = conflictingChannel.GetNumber().IsFromRemote();
  H323Channel * channel = FindChannel(session, !fromRemote);
  if (channel == NULL) {
    PTRACE(1, "H323\tCould not resolve conflict, no reverse channel.");
    return false;
  }

  if (!fromRemote) {
    // close the source media stream so it will be re-established
    OpalMediaStreamPtr stream = conflictingChannel.GetMediaStream();
    if (stream != NULL) {
      OpalMediaPatch * patch = stream->GetPatch();
      if (patch != NULL) 
        patch->GetSource().Close();
    }

    conflictingChannel.Close();
    H323Capability * capability = remoteCapabilities.FindCapability(channel->GetCapability());
    if (capability == NULL) {
      PTRACE(1, "H323\tCould not resolve conflict, capability not available on remote.");
      return false;
    }
    OpenLogicalChannel(*capability, session, H323Channel::IsTransmitter);
    return true;
  }

  // Get the conflisting channel number to close
  H323ChannelNumber number = channel->GetNumber();

  // Close the conflicting channel that got in before our transmitter
  channel->Close();
  CloseLogicalChannelNumber(number);

  // Must be slave and conflict from something we are sending, so try starting a
  // new channel using the master endpoints transmitter codec.
  logicalChannels->Open(conflictingChannel.GetCapability(), session, number);
  return true;
}


H323Channel * H323Connection::CreateLogicalChannel(const H245_OpenLogicalChannel & open,
                                                   PBoolean startingFast,
                                                   unsigned & errorCode)
{
  const H245_H2250LogicalChannelParameters * param;
  const H245_DataType * dataType;
  H323Channel::Directions direction;
  H323Capability * capability;

  if (startingFast && open.HasOptionalField(H245_OpenLogicalChannel::e_reverseLogicalChannelParameters)) {
    if (open.m_reverseLogicalChannelParameters.m_multiplexParameters.GetTag() !=
              H245_OpenLogicalChannel_reverseLogicalChannelParameters_multiplexParameters
                                                      ::e_h2250LogicalChannelParameters) {
      errorCode = H245_OpenLogicalChannelReject_cause::e_unsuitableReverseParameters;
      PTRACE(1, "H323\tCreateLogicalChannel - reverse channel, H225.0 only supported");
      return NULL;
    }

    PTRACE(3, "H323\tCreateLogicalChannel - reverse channel");
    dataType = &open.m_reverseLogicalChannelParameters.m_dataType;
    param = &(const H245_H2250LogicalChannelParameters &)
                      open.m_reverseLogicalChannelParameters.m_multiplexParameters;
    direction = H323Channel::IsTransmitter;

    // Must have been put in earlier
    capability = remoteCapabilities.FindCapability(*dataType);
  }
  else {
    if (open.m_forwardLogicalChannelParameters.m_multiplexParameters.GetTag() !=
              H245_OpenLogicalChannel_forwardLogicalChannelParameters_multiplexParameters
                                                      ::e_h2250LogicalChannelParameters) {
      PTRACE(1, "H323\tCreateLogicalChannel - forward channel, H225.0 only supported");
      errorCode = H245_OpenLogicalChannelReject_cause::e_unspecified;
      return NULL;
    }

    PTRACE(3, "H323\tCreateLogicalChannel - forward channel");
    dataType = &open.m_forwardLogicalChannelParameters.m_dataType;
    param = &(const H245_H2250LogicalChannelParameters &)
                      open.m_forwardLogicalChannelParameters.m_multiplexParameters;
    direction = H323Channel::IsReceiver;

    PString mediaPacketization;
    if (param->HasOptionalField(H245_H2250LogicalChannelParameters::e_mediaPacketization) &&
        param->m_mediaPacketization.GetTag() == H245_H2250LogicalChannelParameters_mediaPacketization::e_rtpPayloadType)
      mediaPacketization = H323GetRTPPacketization(param->m_mediaPacketization);

    // See if datatype is supported
    capability = localCapabilities.FindCapability(*dataType, mediaPacketization);
  }

  if (capability == NULL) {
    errorCode = H245_OpenLogicalChannelReject_cause::e_unknownDataType;
    PTRACE(1, "H323\tCreateLogicalChannel - unknown data type");
    return NULL; // If codec not supported, return error
  }

  if (!capability->OnReceivedPDU(*dataType, direction == H323Channel::IsReceiver)) {
    errorCode = H245_OpenLogicalChannelReject_cause::e_dataTypeNotSupported;
    PTRACE(1, "H323\tCreateLogicalChannel - data type not supported");
    return NULL; // If codec not supported, return error
  }

  if (!OnCreateLogicalChannel(*capability, direction, errorCode))
    return NULL; // If codec combination not supported, return error

  H323Channel * channel = capability->CreateChannel(*this, direction, param->m_sessionID, param);
  if (channel == NULL) {
    errorCode = H245_OpenLogicalChannelReject_cause::e_dataTypeNotAvailable;
    PTRACE(1, "H323\tCreateLogicalChannel - data type not available");
    return NULL;
  }

  if (!channel->SetInitialBandwidth())
    errorCode = H245_OpenLogicalChannelReject_cause::e_insufficientBandwidth;
  else if (channel->OnReceivedPDU(open, errorCode))
    return channel;

  PTRACE(1, "H323\tOnReceivedPDU gave error " << errorCode);
  delete channel;
  return NULL;
}


H323Channel * H323Connection::CreateRealTimeLogicalChannel(const H323Capability & capability,
                                                          H323Channel::Directions dir,
                                                                         unsigned sessionID,
                                                     const H245_H2250LogicalChannelParameters * param,
                                                                        RTP_QOS * rtpqos)
{
  OpalMediaType mediaType = capability.GetMediaFormat().GetMediaType();

  if (sessionID == 0)
    sessionID = GetNextSessionID(mediaType, true);

  if (param != NULL && param->HasOptionalField(H245_H2250LogicalChannelParameters::e_mediaControlChannel)) {
    // We only support unicast IP at this time.
    if (param->m_mediaControlChannel.GetTag() != H245_TransportAddress::e_unicastAddress)
      return NULL;

    const H245_UnicastAddress & uaddr = param->m_mediaControlChannel;
    unsigned int tag = uaddr.GetTag();
    if ((tag != H245_UnicastAddress::e_iPAddress) && (tag != H245_UnicastAddress::e_iP6Address))
      return NULL;
  }

  const H323Transport & transport = GetControlChannel();

  // We only support RTP over UDP at this point in time ...
  if (!transport.IsCompatibleTransport("ip$127.0.0.1"))
    return NULL;

  H323RTPSession * session = dynamic_cast<H323RTPSession *>(UseMediaSession(sessionID, mediaType, h323_rtp_session_type));
  if (PAssertNULL(session) == NULL)
    return NULL;

  PIPSocket::Address localInterface;
  transport.GetLocalAddress(false).GetIpAddress(localInterface);
  if (!session->Open(localInterface.AsString())) {
    ReleaseMediaSession(sessionID);
    return NULL;
  }

#if P_QOS
  session->ModifyQOS(rtpqos);
#endif

  session->Restart(dir == H323Channel::IsReceiver);
  return CreateRTPChannel(capability, dir, *session);
}


H323_RTPChannel * H323Connection::CreateRTPChannel(const H323Capability & capability,
                                                   H323Channel::Directions direction,
                                                   H323RTPSession & rtp)
{
  return new H323_RTPChannel(*this, capability, direction, rtp);
}


PBoolean H323Connection::OnCreateLogicalChannel(const H323Capability & capability,
                                            H323Channel::Directions dir,
                                            unsigned & errorCode)
{
  if (connectionState == ShuttingDownConnection) {
    errorCode = H245_OpenLogicalChannelReject_cause::e_unspecified;
    return false;
  }

  // Default error if returns false
  errorCode = H245_OpenLogicalChannelReject_cause::e_unspecified;

  // Check if in set at all
  if (dir != H323Channel::IsReceiver) {
    H323Capability * remoteCapability = localCapabilities.FindCapability(capability);
    if (remoteCapability == NULL || !localCapabilities.IsAllowed(*remoteCapability)) {
      PTRACE(2, "H323\tOnCreateLogicalChannel - transmit capability " << capability << " not allowed.");
      return false;
    }
  }
  else {
    H323Capability * localCapability = localCapabilities.FindCapability(capability);
    if (localCapability == NULL || !localCapabilities.IsAllowed(*localCapability)) {
      PTRACE(2, "H323\tOnCreateLogicalChannel - receive capability " << capability << " not allowed.");
      return false;
    }
  }

  // Check all running channels, and if new one can't run with it return false
  for (H245LogicalChannelDict::iterator it  = logicalChannels->GetChannels().begin();
                                        it != logicalChannels->GetChannels().end(); ++it) {
    H323Channel * channel = it->second.GetChannel();
    if (channel != NULL && channel->GetDirection() == dir) {
      if (dir != H323Channel::IsReceiver) {
        if (!remoteCapabilities.IsAllowed(capability, channel->GetCapability())) {
          PTRACE(2, "H323\tOnCreateLogicalChannel - transmit capability " << capability
                 << " and " << channel->GetCapability() << " incompatible.");
          return false;
        }
      }
      else {
        if (!localCapabilities.IsAllowed(capability, channel->GetCapability())) {
          PTRACE(2, "H323\tOnCreateLogicalChannel - transmit capability " << capability
                 << " and " << channel->GetCapability() << " incompatible.");
          return false;
        }
      }
    }
  }

  return true;
}


PBoolean H323Connection::OnStartLogicalChannel(H323Channel & channel)
{
  return endpoint.OnStartLogicalChannel(*this, channel);
}


void H323Connection::CloseLogicalChannel(unsigned number, PBoolean fromRemote)
{
  PSafeLockReadWrite mutex(*this);
  if (connectionState != ShuttingDownConnection)
    logicalChannels->Close(number, fromRemote);
}


void H323Connection::CloseLogicalChannelNumber(const H323ChannelNumber & number)
{
  CloseLogicalChannel(number, number.IsFromRemote());
}


void H323Connection::CloseAllLogicalChannels(PBoolean fromRemote)
{
  PSafeLockReadWrite mutex(*this);
  for (H245LogicalChannelDict::iterator it  = logicalChannels->GetChannels().begin();
                                        it != logicalChannels->GetChannels().end(); ++it) {
    H245NegLogicalChannel & negChannel = it->second;
    H323Channel * channel = negChannel.GetChannel();
    if (channel != NULL && channel->GetNumber().IsFromRemote() == fromRemote)
      negChannel.Close();
  }
}


PBoolean H323Connection::OnClosingLogicalChannel(H323Channel & /*channel*/)
{
  return true;
}


void H323Connection::OnClosedLogicalChannel(const H323Channel & channel)
{
  endpoint.OnClosedLogicalChannel(*this, channel);
}


void H323Connection::OnLogicalChannelFlowControl(H323Channel * channel,
                                                 long bitRateRestriction)
{
  if (channel != NULL)
    channel->OnFlowControl(bitRateRestriction);
}


void H323Connection::OnLogicalChannelJitter(H323Channel * channel,
                                            DWORD jitter,
                                            int skippedFrameCount,
                                            int additionalBuffer)
{
  if (channel != NULL)
    channel->OnJitterIndication(jitter, skippedFrameCount, additionalBuffer);
}


unsigned H323Connection::GetBandwidthUsed() const
{
  PSafeLockReadOnly mutex(*this);
  unsigned used = 0;

  for (H245LogicalChannelDict::iterator it  = logicalChannels->GetChannels().begin();
                                        it != logicalChannels->GetChannels().end(); ++it) {
    H323Channel * channel = it->second.GetChannel();
    if (channel != NULL)
      used += channel->GetBandwidthUsed();
  }

  PTRACE(3, "H323\tBandwidth used: " << used);

  return used;
}


PBoolean H323Connection::SetBandwidthAvailable(unsigned newBandwidth, PBoolean force)
{
  PSafeLockReadWrite mutex(*this);

  unsigned used = GetBandwidthUsed();
  if (used > newBandwidth) {
    if (!force)
      return false;

    // Go through logical channels and close down some.
    for (H245LogicalChannelDict::iterator it  = logicalChannels->GetChannels().begin();
                                          it != logicalChannels->GetChannels().end() && used > newBandwidth ; ++it) {
      H323Channel * channel = it->second.GetChannel();
      if (channel != NULL) {
        used -= channel->GetBandwidthUsed();
        CloseLogicalChannelNumber(channel->GetNumber());
      }
    }
  }

  bandwidthAvailable = newBandwidth - used;
  return true;
}


static PBoolean CheckSendUserInputMode(const H323Capabilities & caps,
                                   OpalConnection::SendUserInputModes mode)
{
  // If have remote capabilities, then verify we can send selected mode,
  // otherwise just return and accept it for future validation
  static const H323_UserInputCapability::SubTypes types[H323Connection::NumSendUserInputModes] = {
    H323_UserInputCapability::NumSubTypes,
    H323_UserInputCapability::BasicString,
    H323_UserInputCapability::SignalToneH245,
    H323_UserInputCapability::SignalToneRFC2833,
    H323_UserInputCapability::NumSubTypes,
  };

  if (types[mode] == H323_UserInputCapability::NumSubTypes)
    return mode == H323Connection::SendUserInputAsQ931;

  return caps.FindCapability(H323_UserInputCapability::GetSubTypeName(types[mode])) != NULL;
}


OpalConnection::SendUserInputModes H323Connection::GetRealSendUserInputMode() const
{
  // If have not yet exchanged capabilities (ie not finished setting up the
  // H.245 channel) then the only thing we can do is Q.931
  if (!capabilityExchangeProcedure->HasReceivedCapabilities())
    return SendUserInputAsQ931;

  // First try recommended mode
  if (CheckSendUserInputMode(remoteCapabilities, sendUserInputMode))
    return sendUserInputMode;

  // Then try H.245 tones
  if (CheckSendUserInputMode(remoteCapabilities, SendUserInputAsTone))
    return SendUserInputAsTone;

  // Then try H.245 strings
  if (CheckSendUserInputMode(remoteCapabilities, SendUserInputAsString))
    return SendUserInputAsString;

  // Finally if is H.245 alphanumeric or does not indicate it could do other
  // modes we use H.245 alphanumeric as per spec.
  return SendUserInputAsString;
}


PBoolean H323Connection::SendUserInputString(const PString & value)
{
  SendUserInputModes mode = GetRealSendUserInputMode();

  PTRACE(3, "H323\tSendUserInput(\"" << value << "\"), using mode " << mode);

  if (mode == SendUserInputAsString || mode == SendUserInputAsProtocolDefault)
    return SendUserInputIndicationString(value);

  return OpalRTPConnection::SendUserInputString(value);
}


PBoolean H323Connection::SendUserInputTone(char tone, unsigned duration)
{
  SendUserInputModes mode = GetRealSendUserInputMode();

  PTRACE(3, "H323\tSendUserInputTime('" << tone << "', " << duration << "), using mode " << mode);

  switch (mode) {
    case SendUserInputAsQ931 :
      return SendUserInputIndicationQ931(PString(tone));

    case SendUserInputAsString :
    case SendUserInputAsProtocolDefault:
      return SendUserInputIndicationString(PString(tone));

    case SendUserInputAsTone :
      return SendUserInputIndicationTone(tone, duration);

    default :
      ;
  }

  return OpalRTPConnection::SendUserInputTone(tone, duration);
}


PBoolean H323Connection::SendUserInputIndicationQ931(const PString & value)
{
  PTRACE(3, "H323\tSendUserInputIndicationQ931(\"" << value << "\")");

  H323SignalPDU pdu;
  pdu.BuildInformation(*this);
  pdu.GetQ931().SetKeypad(value);
  if (WriteSignalPDU(pdu))
    return true;

  ClearCall(EndedByTransportFail);
  return false;
}


PBoolean H323Connection::SendUserInputIndicationString(const PString & value)
{
  PTRACE(3, "H323\tSendUserInputIndicationString(\"" << value << "\")");

  H323ControlPDU pdu;
  PASN_GeneralString & str = pdu.BuildUserInputIndication(value);
  if (!str.GetValue())
    return WriteControlPDU(pdu);

  PTRACE(1, "H323\tInvalid characters for UserInputIndication");
  return false;
}


PBoolean H323Connection::SendUserInputIndicationTone(char tone,
                                                 unsigned duration,
                                                 unsigned logicalChannel,
                                                 unsigned rtpTimestamp)
{
  PTRACE(3, "H323\tSendUserInputIndicationTone("
         << tone << ','
         << duration << ','
         << logicalChannel << ','
         << rtpTimestamp << ')');

  if (strchr("0123456789#*ABCD!", tone) == NULL)
    return false;

  H323ControlPDU pdu;
  pdu.BuildUserInputIndication(tone, duration, logicalChannel, rtpTimestamp);
  return WriteControlPDU(pdu);
}


PBoolean H323Connection::SendUserInputIndication(const H245_UserInputIndication & indication)
{
  H323ControlPDU pdu;
  H245_UserInputIndication & ind = pdu.Build(H245_IndicationMessage::e_userInput);
  ind = indication;
  return WriteControlPDU(pdu);
}


void H323Connection::OnUserInputIndication(const H245_UserInputIndication & ind)
{
  switch (ind.GetTag()) {
    case H245_UserInputIndication::e_alphanumeric :
      OnUserInputString((const PASN_GeneralString &)ind);
      break;

    case H245_UserInputIndication::e_signal :
    {
      const H245_UserInputIndication_signal & sig = ind;
      OnUserInputTone(sig.m_signalType[0],
                      sig.HasOptionalField(H245_UserInputIndication_signal::e_duration)
                                ? (unsigned)sig.m_duration : 0);
      break;
    }
    case H245_UserInputIndication::e_signalUpdate :
    {
      const H245_UserInputIndication_signalUpdate & sig = ind;
      OnUserInputTone(' ', sig.m_duration);
      break;
    }
  }
}


static void AddSessionCodecName(PStringStream & name, H323Channel * channel)
{
  if (channel == NULL)
    return;

  OpalMediaStreamPtr stream = channel->GetMediaStream();
  if (stream == NULL)
    return;

  OpalMediaFormat mediaFormat = stream->GetMediaFormat();
  if (!mediaFormat.IsValid())
    return;

  if (name.IsEmpty())
    name << mediaFormat;
  else if (name != mediaFormat)
    name << " / " << mediaFormat;
}


PString H323Connection::GetSessionCodecNames(unsigned sessionID) const
{
  PStringStream name;

  AddSessionCodecName(name, FindChannel(sessionID, false));
  AddSessionCodecName(name, FindChannel(sessionID, true));

  return name;
}


PBoolean H323Connection::RequestModeChange(const PString & newModes)
{
  PSafeLockReadWrite mutex(*this);
  return requestModeProcedure->StartRequest(newModes);
}


PBoolean H323Connection::RequestModeChange(const H245_ArrayOf_ModeDescription & newModes)
{
  PSafeLockReadWrite mutex(*this);
  return requestModeProcedure->StartRequest(newModes);
}


PBoolean H323Connection::OnRequestModeChange(const H245_RequestMode & pdu,
                                         H245_RequestModeAck & /*ack*/,
                                         H245_RequestModeReject & /*reject*/,
                                         PINDEX & selectedMode)
{
  for (selectedMode = 0; selectedMode < pdu.m_requestedModes.GetSize(); selectedMode++) {
    PBoolean ok = true;
    for (PINDEX i = 0; i < pdu.m_requestedModes[selectedMode].GetSize(); i++) {
      if (localCapabilities.FindCapability(pdu.m_requestedModes[selectedMode][i]) == NULL) {
        ok = false;
        break;
      }
    }
    if (ok)
      return true;
  }

  PTRACE(2, "H245\tMode change rejected as does not have capabilities");
  return false;
}


void H323Connection::OnModeChanged(const H245_ModeDescription & newMode)
{
  if (!t38ModeChangeCapabilities.IsEmpty()) {
    PTRACE(4, "H323\tOnModeChanged ignored as T.38 Mode Change in progress");
    return;
  }

  PSafePtr<OpalConnection> otherConnection = GetOtherPartyConnection();
  if (otherConnection == NULL)
    return;

  PTRACE(4, "H323\tOnModeChanged, closing channels");

  bool closedSomething = false;

  for (H245LogicalChannelDict::iterator it  = logicalChannels->GetChannels().begin();
                                        it != logicalChannels->GetChannels().end(); ++it) {
    H245NegLogicalChannel & negChannel = it->second;
    H323Channel * channel = negChannel.GetChannel();
    if (channel != NULL && !channel->GetNumber().IsFromRemote() &&
          (negChannel.IsAwaitingEstablishment() || negChannel.IsEstablished())) {
      bool closeOne = true;

      for (PINDEX m = 0; m < newMode.GetSize(); m++) {
        H323Capability * capability = localCapabilities.FindCapability(newMode[m]);
        if (PAssertNULL(capability) != NULL) { // Should not occur as OnRequestModeChange checks them
          OpalMediaStreamPtr mediaStream = channel->GetMediaStream();
          if (mediaStream != NULL && capability->GetMediaFormat() == mediaStream->GetMediaFormat()) {
            closeOne = false;
            break;
          }
        }
      }

      if (closeOne) {
        negChannel.Close();
        closedSomething = true;
      }
      else {
        PTRACE(4, "H323\tLeaving channel " << channel->GetNumber() << " open, as mode request has not changed it.");
      }
    }
  }

  if (closedSomething) {
    PTRACE(4, "H323\tOnModeChanged, opening channels");

    // Start up the new ones
    for (PINDEX i = 0; i < newMode.GetSize(); i++) {
      H323Capability * capability = localCapabilities.FindCapability(newMode[i]);
      if (PAssertNULL(capability) != NULL) { // Should not occur as OnRequestModeChange checks them
        OpalMediaFormat mediaFormat = capability->GetMediaFormat();
        if (!ownerCall.OpenSourceMediaStreams(*otherConnection, mediaFormat.GetMediaType(), 0, mediaFormat)) {
          PTRACE(2, "H245\tCould not open channel after mode change: " << *capability);
        }
      }
    }
  }
}


void H323Connection::OnAcceptModeChange(const H245_RequestModeAck & pdu)
{
  if (t38ModeChangeCapabilities.IsEmpty())
    return;

  PTRACE(3, "H323\tT.38 mode change accepted.");

  PSafePtr<OpalConnection> otherConnection = GetOtherPartyConnection();
  if (otherConnection == NULL)
    return;

  /* This is a special case for a T.38 switch over. Normally a H.245 Mode
     Request is completely independent of what WE are transmitting. But not
     in the case of T.38, we need to switch our side to the same mode as well.
     So, now we have convinced the other side to send us T.38 data we should
     do the T.38 switch locally, the RequestModeChangeT38() function provided
     a list of \n separated capability names to start, we use that to start
     our transmitters to the same formats. After we close the existing ones!
   */
  CloseAllLogicalChannels(false);

  PStringArray modes = t38ModeChangeCapabilities.Lines();
  t38ModeChangeCapabilities.MakeEmpty();

  PStringArray formats = modes[pdu.m_response.GetTag() != H245_RequestModeAck_response::e_willTransmitMostPreferredMode
                                     && modes.GetSize() > 1 ? 1 : 0].Tokenise('\t');

  bool switched = false;
  for (PINDEX i = 0; i < formats.GetSize(); i++) {
    H323Capability * capability = localCapabilities.FindCapability(formats[i]);
    if (PAssertNULL(capability) != NULL) { // Should not occur!
      OpalMediaFormat mediaFormat = capability->GetMediaFormat();
      if (ownerCall.OpenSourceMediaStreams(*otherConnection, mediaFormat.GetMediaType(), 0, mediaFormat))
        switched = true;
      else {
        PTRACE(2, "H245\tCould not open channel after T.38 mode change: " << *capability);
      }
    }
  }

#if OPAL_FAX
  OnSwitchedFaxMediaStreams(switched);
#endif
}


void H323Connection::OnRefusedModeChange(const H245_RequestModeReject * /*pdu*/)
{
#if OPAL_FAX
  if (!t38ModeChangeCapabilities.IsEmpty()) {
    t38ModeChangeCapabilities.MakeEmpty();
    OnSwitchedFaxMediaStreams(false);
  }
#endif
}


PBoolean H323Connection::RequestModeChangeT38(const char * capabilityNames)
{
  t38ModeChangeCapabilities = capabilityNames;
  if (RequestModeChange(t38ModeChangeCapabilities))
    return true;

  t38ModeChangeCapabilities = PString::Empty();
  return false;
}


PBoolean H323Connection::GetAdmissionRequestAuthentication(const H225_AdmissionRequest & /*arq*/,
                                                       H235Authenticators & /*authenticators*/)
{
  return false;
}


const H323Transport & H323Connection::GetControlChannel() const
{
  return *(controlChannel != NULL ? controlChannel : signallingChannel);
}

OpalTransport & H323Connection::GetTransport() const
{
  return *(controlChannel != NULL ? controlChannel : signallingChannel);
}

void H323Connection::SetEnforcedDurationLimit(unsigned seconds)
{
  enforcedDurationLimit.SetInterval(0, seconds);
}


void H323Connection::MonitorCallStatus()
{
  PSafeLockReadWrite safeLock(*this);
  if (!safeLock.IsLocked())
    return;

  if (IsReleased())
    return;

  if (endpoint.GetRoundTripDelayRate() > 0 && !roundTripDelayTimer.IsRunning()) {
    roundTripDelayTimer = endpoint.GetRoundTripDelayRate();
    StartRoundTripDelay();
  }

/*
  if (endpoint.GetNoMediaTimeout() > 0) {
    PBoolean oneRunning = false;
    PBoolean allSilent = true;
    for (PINDEX i = 0; i < logicalChannels->GetSize(); i++) {
      H323Channel * channel = logicalChannels->GetChannelAt(i);
      if (channel != NULL && channel->IsDescendant(H323_RTPChannel::Class())) {
        if (channel->IsRunning()) {
          oneRunning = true;
          if (((H323_RTPChannel *)channel)->GetSilenceDuration() < endpoint.GetNoMediaTimeout()) {
            allSilent = false;
            break;
          }
        }
      }
    }
    if (oneRunning && allSilent)
      ClearCall(EndedByTransportFail);
  }
*/

  if (enforcedDurationLimit.GetResetTime() > 0 && enforcedDurationLimit == 0)
    ClearCall(EndedByDurationLimit);
}


void H323Connection::H46019SetCallReceiver() 
{ 
    PTRACE(4,"H46019\tCall is receiver.");
    m_H46019CallReceiver = true; 
}


void H323Connection::H46019Enabled() 
{ 
    m_H46019enabled = true; 
}


#if OPAL_H460_NAT
PUDPSocket * H323Connection::GetNatSocket(unsigned session, PBoolean rtp) 
{
    std::map<unsigned,NAT_Sockets>::const_iterator sockets_iter = m_NATSockets.find(session);
    if (sockets_iter != m_NATSockets.end()) {
        NAT_Sockets sockets = sockets_iter->second;
        //PTRACE ( 5, "H323\tFound Sockets in m_NATSockets: " << (int)sockets.rtp << " " << (int)sockets.rtcp);
        if (rtp)
            return sockets.rtp;
        else
            return sockets.rtcp;
    }
    return NULL;
}


void H323Connection::SetRTPNAT(unsigned sessionid, PUDPSocket * _rtp, PUDPSocket * _rtcp)
{
    PWaitAndSignal m(NATSocketMutex);

    //PTRACE(4,"H323\tRTP NAT Connection Callback! Session: " << sessionid << " sockets: " << (int)_rtp << " " << (int)_rtcp);

    NAT_Sockets sockets;
     sockets.rtp = _rtp;
     sockets.rtcp = _rtcp;

    m_NATSockets.insert(pair<unsigned, NAT_Sockets>(sessionid, sockets));
}
#endif // OPAL_H460_NAT


PBoolean H323Connection::OnSendFeatureSet(unsigned code, H225_FeatureSet & featureSet) const
{
#if OPAL_H460
  return features->SendFeature(code, featureSet);
#else
  return endpoint.OnSendFeatureSet(code, featureSet);
#endif
}


void H323Connection::OnReceiveFeatureSet(unsigned code, const H225_FeatureSet & featureSet) const
{
#if OPAL_H460
  features->ReceiveFeature(code, featureSet);
#else
  endpoint.OnReceiveFeatureSet(code, featureSet);
#endif
}


#if OPAL_H460
H460_FeatureSet * H323Connection::GetFeatureSet()
{
  return features;
}
#endif


#if OPAL_H460_NAT
H323Connection::SessionInformation::SessionInformation(const OpalGloballyUniqueID & id, const PString & token, unsigned session)
  : m_callID(id), m_callToken(token), m_sessionID(session)
{
#if OPAL_H460
  // Some random number bases on the session id (for H.460.24A)
  int rand = PRandom::Number((session *100),((session+1)*100)-1);
  m_CUI = PString(rand); 
  PTRACE(4,"H46024A\tGenerated CUI s: " << session << " value: " << m_CUI);
#else
  m_CUI = PString();
#endif
}


const PString & H323Connection::SessionInformation::GetCallToken()
{
  return m_callToken;
}


unsigned H323Connection::SessionInformation::GetSessionID() const
{
  return m_sessionID;
}


H323Connection::SessionInformation * H323Connection::BuildSessionInformation(unsigned sessionID) const
{
  return new SessionInformation(GetCallIdentifier(),GetCallToken(),sessionID);
}


const OpalGloballyUniqueID & H323Connection::SessionInformation::GetCallIdentifer()
{
  return m_callID;
}


const PString & H323Connection::SessionInformation::GetCUI()
{
  return m_CUI;
}

#endif // OPAL_H460_NAT

#endif // OPAL_H323

/////////////////////////////////////////////////////////////////////////////
