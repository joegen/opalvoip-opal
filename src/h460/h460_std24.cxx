/*
 * h460_std24.cxx
 *
 * Copyright (c) 2009 ISVO (Asia) Pte Ltd. All Rights Reserved.
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Alternatively, the contents of this file may be used under the terms
 * of the General Public License (the  "GNU License"), in which case the
 * provisions of GNU License are applicable instead of those
 * above. If you wish to allow use of your version of this file only
 * under the terms of the GNU License and not to allow others to use
 * your version of this file under the MPL, indicate your decision by
 * deleting the provisions above and replace them with the notice and
 * other provisions required by the GNU License. If you do not delete
 * the provisions above, a recipient may use your version of this file
 * under either the MPL or the GNU License."
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
 * the License for the specific language governing rights and limitations
 * under the License.
 *
 * The Original Code is derived from and used in conjunction with the 
 * H323Plus Project (www.h323plus.org/)
 *
 * The Initial Developer of the Original Code is ISVO (Asia) Pte Ltd.
 *
 * Contributor(s): Many thanks to Simon Horne.
 *                 Robert Jongbloed (robertj@voxlucida.com.au).
 *
 */

#include <ptlib.h>

#define P_FORCE_STATIC_PLUGIN 1

#include <h460/h460_std24.h>

#if OPAL_H460_24

#include <h323/h323ep.h>
#include <h323/h323pdu.h>
#include <h460/h460_std19.h>
#include <h460/h460_std23.h>
#include <ptclib/random.h>
#include <ptclib/cypher.h>


#define PTraceModule() "H46024"


// H.460.24 P2Pnat Feature & parameters
const H460_FeatureID & H460_FeatureStd24::ID() { static const H460_FeatureID id(24); return id; }

static const H460_FeatureID RemoteProxy_ID     (1);  // bool if gatekeeper will proxy
static const H460_FeatureID RemoteNAT_ID       (2);  // bool if remote endpoint can assist local endpoint directly
static const H460_FeatureID MustProxyNAT_ID    (3);  // bool if gatekeeper must proxy to reach local endpoint
static const H460_FeatureID CalledIsNAT_ID     (4);  // bool if local endpoint is behind a NAT/FW
static const H460_FeatureID CalledNATType_ID   (5);  // integer 8 reported NAT type
static const H460_FeatureID ApparentSrcAddr_ID (6);  // H225_TransportAddress of apparent source address of endpoint
static const H460_FeatureID SameNATProbe_ID    (7);  // bool if local endpoint supports H.460.24 Annex A
static const H460_FeatureID MediaStrategy_ID   (8);  // integer 8 Instruction on how NAT is to be Traversed
static const H460_FeatureID ExternalNATProbe_ID(9);  // bool if endpoint supports H.460.24 Annex B


ostream & operator<<(ostream & strm, H460_FeatureStd24::MediaStrategyIndicator strategy)
{
  static const char * const Names[] = {
    "Unknown Strategy",
    "No Assistance",
    "Local Master",
    "Remote Master",
    "Local Proxy",
    "Remote Proxy",
    "Full Proxy",
    "AnnexA SameNAT",
    "AnnexB NAToffload",
    "Call Failure!"
  };

  if (strategy < PARRAYSIZE(Names))
    strm << Names[strategy];
  else if (strategy == H460_FeatureStd24::e_MediaFailure)
    strm << "Media Strategy Failure";
  else
    strm << '<' << (unsigned)strategy << '>';

  return strm;
}


///////////////////////////////////////////////////////////////////

H460_FEATURE(Std24, "H.460.24");

H460_FeatureStd24::H460_FeatureStd24()
  : H460_Feature(ID())
  , m_natMethod(NULL)
  , m_strategy(e_Unknown)
{
}


bool H460_FeatureStd24::Initialise(H323EndPoint & ep, H323Connection * con)
{
  if (!H460_Feature::Initialise(ep, con))
    return false;

  m_natMethod = dynamic_cast<PNatMethod_H46024 *>(GetNatMethod(PNatMethod_H46024::MethodName()));
  if (m_natMethod == NULL)
    return false;

  if (con == NULL)
    return true;

  if (!IsFeatureNegotiatedOnGk(H460_FeatureStd23::ID())) {
    PTRACE(2, "Disabled as H.460.23 not negotiated with gatekeeper");
    return false;
  }

  return true;
}


bool H460_FeatureStd24::OnSendAdmissionRequest(H460_FeatureDescriptor &)
{
  return true;
}


void H460_FeatureStd24::OnReceiveAdmissionConfirm(const H460_FeatureDescriptor & pdu)
{
  HandleStrategy(pdu);
}


void H460_FeatureStd24::OnReceiveAdmissionReject(const H460_FeatureDescriptor & pdu) 
{
  HandleStrategy(pdu);
}


void H460_FeatureStd24::HandleStrategy(const H460_FeatureDescriptor & pdu)
{
  if (m_natMethod == NULL)
    return;

  m_strategy = e_MediaFailure;
  if (pdu.HasParameter(MediaStrategy_ID)) {
    m_strategy = (MediaStrategyIndicator)(unsigned)pdu.GetParameter(MediaStrategy_ID);
    PTRACE(4, "Strategy selected: " << m_strategy);
  }
  else {
    PTRACE(4, "No strategy present, DISABLED.");
  }
}


bool H460_FeatureStd24::OnSendSetup_UUIE(H460_FeatureDescriptor & pdu)
{
  {
    H460_FeatureStd24 * feat24;
    if (!GetFeatureOnGk(feat24)) {
      PTRACE(2, "Disabled as not negotiated with gatekeeper");
      return false;
    }
    m_strategy = feat24->GetStrategy();
  }

  MediaStrategyIndicator remoteStrategy;
  switch (m_strategy) {
    case H460_FeatureStd24::e_Unknown:
      return false;

    case H460_FeatureStd24::e_LocalMediaMaster:
      remoteStrategy = H460_FeatureStd24::e_RemoteMediaMaster;
      break;

    case H460_FeatureStd24::e_RemoteMediaMaster:
      remoteStrategy = H460_FeatureStd24::e_LocalMediaMaster;
      break;

    case H460_FeatureStd24::e_LocalProxy:
      remoteStrategy = H460_FeatureStd24::e_RemoteProxy;
      break;
    case H460_FeatureStd24::e_RemoteProxy:
      remoteStrategy = H460_FeatureStd24::e_LocalProxy;
      break;

    default:
      remoteStrategy = m_strategy;
  }

  pdu.AddParameter(MediaStrategy_ID, H460_FeatureContent(remoteStrategy, sizeof(BYTE)));

  return true; 
}


void H460_FeatureStd24::OnReceiveSetup_UUIE(const H460_FeatureDescriptor & pdu)
{
  HandleStrategy(pdu);
}


bool H460_FeatureStd24::IsDisabledH46019() const
{
  switch (m_strategy) {
    case H460_FeatureStd24::e_NoAssist :
    case H460_FeatureStd24::e_LocalMediaMaster :
    case H460_FeatureStd24::e_RemoteMediaMaster :
      return true;

    default:
      return false;
  }
}


//////////////////////////////////////////////////////////////////////

PCREATE_NAT_PLUGIN(H46024, "H.460.24");

PNatMethod_H46024::PNatMethod_H46024(unsigned priority)
  : PSTUNClient(priority)
{
}


const char * PNatMethod_H46024::MethodName()
{
  return PPlugin_PNatMethod_H46024::ServiceName();
}


PCaselessString PNatMethod_H46024::GetMethodName() const
{
  return MethodName();
}


bool PNatMethod_H46024::IsAvailable(const PIPSocket::Address & binding, PObject * context)
{
  H460_FeatureStd24 * feature;
  if (!H460_Feature::FromContext(context, feature))
    return false;

  if (feature->GetStrategy() != H460_FeatureStd24::e_LocalMediaMaster) {
    PTRACE(4, "Disabled via H.460.24 strategy " << feature->GetStrategy());
    return false;
  }

  return PSTUNClient::IsAvailable(binding, context);
}


///////////////////////////////////////////////////////////////////

#if OPAL_H460_24A

H460_FEATURE(Std24AnnexA, "H.460.24 Annex A");

const H460_FeatureID & H460_FeatureStd24AnnexA::ID()
{
  static const H460_FeatureID id(PASN_ObjectId("0.0.8.460.24.1")); return id;
}

H460_FeatureStd24AnnexA::H460_FeatureStd24AnnexA()
  : H460_Feature(ID())
  , m_session(NULL)
  , m_probeInfo("24.1")
  , m_probeNotifier(PCREATE_NOTIFIER(ProbeResponse))
{
  m_probeTimer.SetNotifier(PCREATE_NOTIFIER(ProbeTimeout), "H460Probe");
}


H460_FeatureStd24AnnexA::~H460_FeatureStd24AnnexA()
{
  m_probeTimer.Stop();

  if (m_session != NULL)
    m_session->RemoveApplDefinedNotifier(m_probeNotifier);
}


bool H460_FeatureStd24AnnexA::OnSendingOLCGenericInformation(unsigned sessionID, H245_ArrayOf_GenericParameter & param, bool isAck)
{
  PWaitAndSignal mutex(m_mutex);

  if (!InitProbe(sessionID))
    return false;

  H225_TransportAddress mediaChannel;
  if (!H323TransportAddress(m_session->GetLocalAddress(true)).SetPDU(mediaChannel))
    return false;

  H225_TransportAddress controlChannel;
  if (!H323TransportAddress(m_session->GetLocalAddress(false)).SetPDU(controlChannel))
    return false;

  H323AddGenericParameterObject(param, 0, PASN_IA5String(m_localCUI));
  H323AddGenericParameterObject(param, 1, mediaChannel);
  H323AddGenericParameterObject(param, 2, controlChannel);

  if (isAck)
    StartProbe();

  return true;
}


void H460_FeatureStd24AnnexA::OnReceiveOLCGenericInformation(unsigned sessionID, const H245_ArrayOf_GenericParameter & param, bool isAck)
{
  PWaitAndSignal mutex(m_mutex);

  if (!InitProbe(sessionID))
    return;

  PASN_IA5String cui;
  H225_TransportAddress mediaChannel, controlChannel;
  if (!H323GetGenericParameterObject(param, 0, cui)) {
    PTRACE(2, "No CUI in generic parameters");
    return;
  }

  if (!H323GetGenericParameterObject(param, 1, mediaChannel)) {
    PTRACE(2, "No media channel address in generic parameters");
    return;
  }

  if (!H323GetGenericParameterObject(param, 2, controlChannel)) {
    PTRACE(2, "No control channel address in generic parameters");
    return;
  }

  H323TransportAddress(mediaChannel).GetIpAndPort(m_directMediaAddress);
  H323TransportAddress(mediaChannel).GetIpAndPort(m_directControlAddress);

  PMessageDigestSHA1 sha1;
  PMessageDigestSHA1::Result digest;
  sha1.Encode(m_session->GetConnection().GetIdentifier() + cui.GetValue(), digest);
  m_remoteSHA1 = digest;

  m_probeInfo.m_data = m_remoteSHA1;

  if (isAck)
    StartProbe();
}


bool H460_FeatureStd24AnnexA::InitProbe(unsigned sessionID)
{
  H460_FeatureStd24 * feat24;
  if (GetFeatureOnGk(feat24) && feat24->GetStrategy() != H460_FeatureStd24::e_ProbeSameNAT) {
    PTRACE(4, "Disabled Annex A via H.460.24 strategy " << feat24->GetStrategy());
    return false;
  }

  m_session = dynamic_cast<OpalRTPSession *>(m_connection->GetMediaSession(sessionID));
  if (m_session == NULL) {
    PTRACE(2, "Session " << sessionID << " not found or not RTP.");
    return false;
  }

  m_session->AddApplDefinedNotifier(m_probeNotifier);
  m_probeInfo.m_SSRC = m_session->GetSyncSourceOut();

  if (m_localCUI.IsEmpty()) {
    m_localCUI = PRandom::Number(sessionID * 100, sessionID * 100 + 99);
    PTRACE(4, "Annex A local CUI \"" << m_localCUI << '"');

    PMessageDigestSHA1 sha1;
    PMessageDigestSHA1::Result digest;
    sha1.Encode(m_session->GetConnection().GetIdentifier() + m_localCUI, digest);
    m_localSHA1 = digest;
  }

  return true;
}


void H460_FeatureStd24AnnexA::StartProbe()
{
  m_probeTimeout = 5000;
  m_probeTimer.RunContinuous(200);
  SendProbe();
}


void H460_FeatureStd24AnnexA::SendProbe()
{
  RTP_ControlFrame frame;
  frame.AddApplDefined(m_probeInfo);
  m_session->WriteControl(frame, &m_directControlAddress);
}


void H460_FeatureStd24AnnexA::ProbeTimeout(PTimer &, P_INT_PTR)
{
  PWaitAndSignal mutex(m_mutex);

  if (m_probeTimeout.IsRunning())
    SendProbe();
  else {
    m_probeTimer.Stop();  // No response. Give up.
    PTRACE(3, "Timed out waiting for probe response on session " << m_session->GetSessionID());
  }
}


void H460_FeatureStd24AnnexA::ProbeResponse(OpalRTPSession &, const RTP_ControlFrame::ApplDefinedInfo & info)
{
  if (strcmp(info.m_type, "24.1") != 0) {
    PTRACE(4, "Received probe of unknown type \"" << info.m_type << '"');
    return;
  }

  PWaitAndSignal mutex(m_mutex);

  if (m_localSHA1 != info.m_data) {
    PTRACE(4, "Received probe from some other connection/channel:\nGot " << info.m_data << "\nExpected: " << m_localSHA1);
    return;
  }

  PTRACE_IF(4, m_probeInfo.m_subType == 0, "Received probe, sending our response");
  m_probeInfo.m_subType = 1;

  SendProbe();

  if (info.m_subType != 0) {
    PTRACE(3, "Received probe response, adjusting session " << m_session->GetSessionID() << " transmit addresses to direct.");
    m_session->SetRemoteAddress(m_directMediaAddress, true);
    m_session->SetRemoteAddress(m_directControlAddress, false);
    m_probeTimer.Stop();
    SendProbe(); // Send a second one in case first one is lost
  }
}

#endif // OPAL_H460_24A

#endif // OPAL_H460_24
