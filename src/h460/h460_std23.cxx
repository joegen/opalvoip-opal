/*
 * h460_std23.cxx
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
 * $Revision$
 * $Author$
 * $Date$
 */

#include <ptlib.h>

#define P_FORCE_STATIC_PLUGIN 1

#include <h460/h460_std23.h>

#if OPAL_H460_NAT

#include <h323/h323ep.h>
#include <h460/h460_std19.h>
#include <ptclib/random.h>
#include <ptclib/pdns.h>

// Note WE want upnp too?
#ifdef H323_UPnP
#include "h460/upnpcp.h"
#endif

#pragma message("H.460.23/.24 Enabled. Contact consulting@h323plus.org for licensing terms.")

#ifdef _MSC_VER
#pragma warning(disable : 4239)
#endif


// H.460.23 NAT Detection Feature
static const H460_FeatureID RemoteNAT_ID(1);  // bool if endpoint has remote NAT support
static const H460_FeatureID AnnexA_ID   (2);  // bool if endpoint supports H.460.24 Annex A
static const H460_FeatureID LocalNAT_ID (3);  // bool if endpoint is NATed
static const H460_FeatureID NATDetRAS_ID(4);  // Detected RAS H225_TransportAddress 
static const H460_FeatureID STUNServ_ID (5);  // H225_TransportAddress of STUN Server 
static const H460_FeatureID NATType_ID  (6);  // integer 8 Endpoint NAT Type
static const H460_FeatureID AnnexB_ID   (7);  // bool if endpoint supports H.460.24 Annex B


// H.460.24 P2Pnat Feature
static const H460_FeatureID NATProxy_ID      (1);  // bool if gatekeeper will proxy
static const H460_FeatureID RemoteMast_ID    (2);  // bool if remote endpoint can assist local endpoint directly
static const H460_FeatureID MustProxy_ID     (3);  // bool if gatekeeper must proxy to reach local endpoint
static const H460_FeatureID CalledIsNat_ID   (4);  // bool if local endpoint is behind a NAT/FW
static const H460_FeatureID NatRemoteType_ID (5);  // integer 8 reported NAT type
static const H460_FeatureID ApparentSource_ID(6);  // H225_TransportAddress of apparent source address of endpoint
static const H460_FeatureID SupAnnexA_ID     (7);  // bool if local endpoint supports H.460.24 Annex A
static const H460_FeatureID NATInst_ID       (8);  // integer 8 Instruction on how NAT is to be Traversed
static const H460_FeatureID SupAnnexB_ID     (9);  // bool if endpoint supports H.460.24 Annex B


//////////////////////////////////////////////////////////////////////

PCREATE_NAT_PLUGIN(H46024);

PNatMethod_H46024::PNatMethod_H46024(unsigned priority)
  : PNatMethod(priority)
  , m_endpoint(NULL)
{
  // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  // Default to inactive until we get it all working
  m_active = false; 
}


const char * PNatMethod_H46024::MethodName()
{
  return PPlugin_PNatMethod_H46024::ServiceName();
}


PCaselessString PNatMethod_H46024::GetMethodName() const
{
  return MethodName();
}


PString PNatMethod_H46024::GetServer() const
{
  if (m_endpoint != NULL) {
    H323Gatekeeper * gk = m_endpoint->GetGatekeeper();
    if (gk != NULL)
      return gk->GetTransport().GetRemoteAddress().GetHostName();
  }

  return PString::Empty();
}


bool PNatMethod_H46024::IsAvailable(const PIPSocket::Address & binding, PObject * context)
{
  return PNatMethod::IsAvailable(binding) && dynamic_cast<OpalRTPSession *>(context) != NULL;
}


PNATUDPSocket * PNatMethod_H46024::InternalCreateSocket(Component component, PObject *)
{
  return new PNATUDPSocket(component);
}


void PNatMethod_H46024::InternalUpdate()
{
}


//////////////////////////////////////////////////////////////////////

H460_FEATURE(Std23, "H.460.23");

H460_FeatureStd23::H460_FeatureStd23()
  : H460_Feature(23)
  , m_natMethod(NULL)
  , m_alg(false)
{
  PTRACE(6,"Std23\tInstance Created");

  AddParameter(RemoteNAT_ID, true);
  AddParameter(AnnexA_ID, true);
  AddParameter(AnnexB_ID, true);
}


bool H460_FeatureStd23::Initialise(H323EndPoint & ep, H323Connection * con)
{
  if (!H460_Feature::Initialise(ep, con))
    return false;

  m_natMethod = dynamic_cast<PNatMethod_H46024 *>(GetNatMethod(PNatMethod_H46024::MethodName()));
  if (m_natMethod == NULL)
    return false;

  m_natMethod->m_endpoint = &ep;
  PTRACE(4, "Std23\tEnabled H.460.23");
  return true;
}


bool H460_FeatureStd23::OnSendGatekeeperRequest(H460_FeatureDescriptor & pdu) 
{
  // Don't show the extra parameters on GRQ, just the id to see if supported at all
  pdu.RemoveOptionalField(H225_FeatureDescriptor::e_parameters);
  return true;
}


bool H460_FeatureStd23::OnSendRegistrationRequest(H460_FeatureDescriptor & pdu) 
{ 
  // Build Message
  H323Gatekeeper * gk = m_endpoint->GetGatekeeper();
  if (gk != NULL && gk->IsRegistered() && m_alg) {
    /* We should be disabling H.460.23/.24 support but we will disable
       H.460.18/.19 instead :) and say we have no NAT.. */
    pdu.AddParameter(NATType_ID, H460_FeatureContent(1,8)); 
    pdu.AddParameter(RemoteNAT_ID, H460_FeatureContent(false)); 
    m_alg = false;
  }
#ifdef H323_UPnP
  else if (m_natNotify || UsePnP()) {
    feat.Add(NATTypeOID, H460_FeatureContent(natType, 8)); 
    natNotify = false;
  }
#endif

  return true; 
}


void H460_FeatureStd23::OnReceiveRegistrationConfirm(const H460_FeatureDescriptor & pdu)
{
  if (pdu.HasParameter(LocalNAT_ID) && (bool)pdu.GetParameter(LocalNAT_ID))
    m_natMethod->m_natType = PNatMethod::PortRestrictedNat;

  if (pdu.HasParameter(NATDetRAS_ID)) {
    H323TransportAddress addr = pdu.GetParameter(NATDetRAS_ID);
    PIPSocket::Address ip;
    addr.GetIpAddress(ip);
    if (DetectALG(ip)) {
      // if we have an ALG then to be on the safe side
      // disable .23/.24 so that the ALG has more chance
      // of behaving properly...
      m_alg = true;
      new PThreadObj<H460_FeatureStd23>(*this, &H460_FeatureStd23::DelayedReRegistration, true, "H.460.23");
    }
  }

  if (pdu.HasParameter(STUNServ_ID) && m_endpoint->GetManager().GetNATServer(PSTUNClient::MethodName()).IsEmpty()) {
    H323TransportAddress addr = pdu.GetParameter(STUNServ_ID);
    PTRACE(3, "Std23\tSetting STUN server to " << addr);
    m_endpoint->GetManager().SetNATServer(PSTUNClient::MethodName(), addr.GetHostName(true));
  }
}


#ifdef H323_UPnP
void H460_FeatureStd23::InitialiseUPnP()
{
  PTRACE(4,"Std23\tStarting Alternate UPnP");

  PNatMethod_UPnP * natMethod = (PNatMethod_UPnP *)EP->GetNatMethods().LoadNatMethod("UPnP");
  if (natMethod) {
    natMethod->AttachEndPoint(EP);
    EP->GetNatMethods().AddMethod(natMethod);
  }
}
#endif


#if crap
void H460_FeatureStd23::OnNATTypeDetection(PSTUNClient::NatTypes type, const PIPSocket::Address & ExtIP)
{
  if (natType == type)
    return;

  externalIP = ExtIP;

  if (natType == PSTUNClient::UnknownNat) {
    PTRACE(4,"Std23\tSTUN Test Result: " << type << " forcing reregistration.");
#ifdef H323_UPnP
    if (type > PSTUNClient::ConeNat)
      InitialiseUPnP();
#endif
    natType = type;  // first time detection
  } else {
    PTRACE(2,"Std23\tBAD NAT Detected: Was " << natType << " Now " << type << " Disabling H.460.23/.24");
    natType = PSTUNClient::UnknownNat;  // Leopard changed it spots (disable H.460.23/.24)
  }

  natNotify = true;
  //m_endpoint->ForceGatekeeperReRegistration();
}
#endif


bool H460_FeatureStd23::DetectALG(const PIPSocket::Address & detectAddress)
{
  PIPSocket::InterfaceTable if_table;
  if (!PIPSocket::GetInterfaceTable(if_table)) {
    PTRACE(1, "Std23\tERROR: Can't get interface table");
    return false;
  }

  for (PINDEX i=0; i< if_table.GetSize(); i++) {
    if (detectAddress == if_table[i].GetAddress()) {
      PTRACE(4, "Std23\tNo Intermediary device detected between EP and GK");
      return false;
    }
  }
  PTRACE(4, "Std23\tWARNING: Intermediary device detected!");
  return true;
}


void H460_FeatureStd23::DelayedReRegistration()
{
}


#ifdef H323_UPnP
bool H460_FeatureStd23::UsePnP()
{
  if (natType <= PSTUNClient::ConeNat || useAlternate > 0)
    return false;

  PNatList & natlist = EP->GetNatMethods().GetNATList();

  for (PINDEX i=0; i< natlist.GetSize(); i++) {
    if (natlist[i].GetName() == "UPnP" && 
      natlist[i].GetRTPSupport() == PSTUNClient::RTPSupported) {
        PIPSocket::Address extIP;
        natlist[i].GetExternalAddress(extIP);
        if (!extIP.IsValid() || extIP == externalIP) {
          PTRACE(4,"H46023\tUPnP Change NAT from " << natType << " to " << PSTUNClient::ConeNat);
          natType = PSTUNClient::ConeNat;
          useAlternate = 1;
          EP->NATMethodCallBack(natlist[i].GetName(),1,"Available");
          return true;
        } else {
          PTRACE(4,"H46023\tUPnP Unavailable subNAT STUN: " << externalIP << " UPnP " << extIP);
          useAlternate = 2;
        }
    }
  }
  return false;
}
#endif


///////////////////////////////////////////////////////////////////

H460_FEATURE(Std24, "H.460.24");

H460_FeatureStd24::H460_FeatureStd24()
  : H460_Feature(24)
{
  PTRACE(6,"Std24\tInstance Created");

  natconfig = H460_FeatureStd24::e_unknown;
  useAlternate = false;
}


bool H460_FeatureStd24::IsNegotiated() const
{
  return IsFeatureNegotiatedOnGk(23); // Must have 23 before 24 can be used
}


bool H460_FeatureStd24::OnSendAdmissionRequest(H460_FeatureDescriptor & pdu) 
{ 
  // Ignore if already not enabled or manually using STUN
  if (!IsNegotiated()) 
    return false;

  PWaitAndSignal m(h460mute);

  // Build Message
  PTRACE(6,"Std24\tSending ARQ ");

  if (natconfig != e_unknown)
    pdu.AddParameter(NATInst_ID, H460_FeatureContent((unsigned)natconfig, 8));

  return true;  
}


void H460_FeatureStd24::OnReceiveAdmissionConfirm(const H460_FeatureDescriptor & pdu)
{
  if (pdu.HasParameter(NATInst_ID)) {
    PTRACE(6,"Std24\tReading ACF");
    unsigned NATinst = pdu.GetParameter(NATInst_ID);
    natconfig = (NatInstruct)NATinst;
    HandleNATInstruction(natconfig);
  }
}


void H460_FeatureStd24::OnReceiveAdmissionReject(const H460_FeatureDescriptor &) 
{
  PTRACE(6,"Std24\tARJ Received");
  HandleNATInstruction(H460_FeatureStd24::e_natFailure);
}


bool H460_FeatureStd24::OnSendSetup_UUIE(H460_FeatureDescriptor & pdu)
{ 
  // Ignore if already not enabled or manually using STUN
  if (!IsNegotiated()) 
    return false;

  PTRACE(6,"Std24\tSend Setup");
  if (natconfig == H460_FeatureStd24::e_unknown)
    return false;

  int remoteconfig;
  switch (natconfig) {
  case H460_FeatureStd24::e_noassist:
    remoteconfig = H460_FeatureStd24::e_noassist;
    break;
  case H460_FeatureStd24::e_localMaster:
    remoteconfig = H460_FeatureStd24::e_remoteMaster;
    break;
  case H460_FeatureStd24::e_remoteMaster:
    remoteconfig = H460_FeatureStd24::e_localMaster;
    break;
  case H460_FeatureStd24::e_localProxy:
    remoteconfig = H460_FeatureStd24::e_remoteProxy;
    break;
  case H460_FeatureStd24::e_remoteProxy:
    remoteconfig = H460_FeatureStd24::e_localProxy;
    break;
  default:
    remoteconfig = natconfig;
  }

  pdu.AddParameter(NATInst_ID, H460_FeatureContent(remoteconfig,8));

  return true; 
}


void H460_FeatureStd24::OnReceiveSetup_UUIE(const H460_FeatureDescriptor & pdu)
{
  PWaitAndSignal m(h460mute);

  if (pdu.HasParameter(NATInst_ID)) {
    PTRACE(6,"Std24\tReceive Setup");

    unsigned inst = pdu.GetParameter(NATInst_ID); 
    natconfig = (NatInstruct)inst;
    HandleNATInstruction(natconfig);
  }
}


void H460_FeatureStd24::HandleNATInstruction(NatInstruct _config)
{

  PTRACE(4,"Std24\tNAT Instruction Received: " << _config);
  switch (_config) {
  case H460_FeatureStd24::e_localMaster:
    PTRACE(4,"Std24\tLocal NAT Support: H.460.24 ENABLED");
    SetNATMethods(e_enable);
    break;

  case H460_FeatureStd24::e_remoteMaster:
    PTRACE(4,"Std24\tRemote NAT Support: ALL NAT DISABLED");
    SetNATMethods(e_disable);
    break;

  case H460_FeatureStd24::e_remoteProxy:
    PTRACE(4,"Std24\tRemote Proxy Support: H.460.24 DISABLED");
    SetNATMethods(e_default);
    break;

  case H460_FeatureStd24::e_localProxy:
    PTRACE(4,"Std24\tCall Local Proxy: H.460.24 DISABLED");
    SetNATMethods(e_default);
    break;

#if 0
  case H460_FeatureStd24::e_natAnnexA:
    PTRACE(4,"Std24\tSame NAT: H.460.24 AnnexA ENABLED");
    m_connection->H46024AEnabled();
    SetNATMethods(e_AnnexA);
    break;

  case H460_FeatureStd24::e_natAnnexB:
    PTRACE(4,"Std24\tSame NAT: H.460.24 AnnexA ENABLED");
    m_connection->H46024BEnabled();
    SetNATMethods(e_AnnexB);
    break;

  case H460_FeatureStd24::e_natFailure:
    PTRACE(4,"Std24\tCall Failure Detected");
    m_connection->FeatureCallBack(GetPluginName(),1,"Call Failure");
    break;
  case H460_FeatureStd24::e_noassist:
    PTRACE(4,"Std24\tNo Assist: H.460.24 DISABLED.");
    m_connection->DisableNATSupport();
    SetNATMethods(e_default);
    break;
#endif
  default:
    break;
  }
}


void H460_FeatureStd24::SetNATMethods(H46024NAT state)
{
  H460_FeatureStd19 * feat19 = dynamic_cast<H460_FeatureStd19 *>(m_connection->GetFeatureSet()->GetFeature(19));
  if (state != H460_FeatureStd24::e_default &&
      state != H460_FeatureStd24::e_AnnexA &&
      state != H460_FeatureStd24::e_AnnexB) {
    feat19->DisableByH46024();
    PTRACE(4, "Std24\tH.460.19 disabled by H.460.24 option.");
  }

  PNatMethod * natMethod = m_endpoint->GetManager().GetNatMethods().GetMethodByName("H46024");
  if (natMethod != NULL) {
    switch (state) {
      case H460_FeatureStd24::e_AnnexA:   // To do Annex A Implementation.
      case H460_FeatureStd24::e_AnnexB:   // To do Annex B Implementation.
      case H460_FeatureStd24::e_default:
        natMethod->Activate(true);
        break;

      case H460_FeatureStd24::e_enable:
        natMethod->Activate(!useAlternate);
        break;

      case H460_FeatureStd24::e_disable:
        natMethod->Activate(false);
        break;
      default:
        break;
    }
  }
}


PString H460_FeatureStd24::GetNATStrategyString(NatInstruct method)
{
  static const char * const Names[10] = {
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

  if (method < 10)
    return Names[method];

  return psprintf("<NAT Strategy %u>", method);
}


#endif // OPAL_H460_NAT
