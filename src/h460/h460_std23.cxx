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
 */

#include <ptlib.h>

#define P_FORCE_STATIC_PLUGIN 1

#include <h460/h460_std23.h>

#if OPAL_H460_NAT

#include <h323/h323ep.h>
#include <h460/h460_std19.h>
#include <h460/h460_std24.h>


// Note WE want upnp too?
#ifdef H323_UPnP
#include "h460/upnpcp.h"
#endif

#define PTraceModule() "H46023"


// H.460.23 NAT Detection Feature & parameters
const H460_FeatureID & H460_FeatureStd23::ID() { static const H460_FeatureID id(23); return id; }

static const H460_FeatureID RemoteNAT_ID       (1); // bool if endpoint has remote NAT support
static const H460_FeatureID SameNATProbe_ID    (2); // bool if endpoint supports H.460.24 Annex A
static const H460_FeatureID NATPresent_ID      (3); // bool if endpoint is NATed
static const H460_FeatureID RASPresentation_ID (4); // Detected RAS H225_TransportAddress 
static const H460_FeatureID NATTest_ID         (5); // H225_TransportAddress of STUN Server 
static const H460_FeatureID NATType_ID         (6); // integer 8 Endpoint NAT Type
static const H460_FeatureID ExternalNATProbe_ID(7); // bool if endpoint supports H.460.24 Annex B


H460_FEATURE(Std23, "H.460.23");


H460_FeatureStd23::H460_FeatureStd23()
  : H460_Feature(ID())
#if OPAL_H460_24
  , m_natMethod(NULL)
#endif
  , m_applicationLevelGateway(false)
{
}


bool H460_FeatureStd23::Initialise(H323EndPoint & ep, H323Connection * con)
{
  if (!H460_Feature::Initialise(ep, con))
    return false;

#if OPAL_H460_24
  if (!GetNatMethod(PNatMethod_H46024::MethodName(), m_natMethod)) {
    PTRACE(4, "Could not find NAT method " << PNatMethod_H46024::MethodName());
    return false;
  }
#endif

  AddParameter(RemoteNAT_ID, ep.GetH46019Server() != NULL);
#if OPAL_H460_24A
  AddParameter(SameNATProbe_ID, true);
#endif

  return true;
}


bool H460_FeatureStd23::OnSendGatekeeperRequest(H460_FeatureDescriptor & pdu) 
{
  // Don't show the extra parameters on GRQ, just the id to see if supported at all
  pdu.RemoveOptionalField(H225_FeatureDescriptor::e_parameters);
  return true;
}


bool H460_FeatureStd23::OnSendRegistrationRequest(H460_FeatureDescriptor & pdu, bool lightweight)
{
  if (lightweight) {
#if OPAL_H460_24
    PNatMethod::NatTypes natType = m_natMethod->GetNatType();
    if (natType == PNatMethod::UnknownNat)
      return false;

    pdu.AddParameter(NATType_ID, H460_FeatureContent(natType, sizeof(BYTE)));
#endif
    return true;
  }

  if (m_applicationLevelGateway) {
    pdu.AddParameter(RemoteNAT_ID, false);
  }
#ifdef H323_UPnP
  else if (m_natNotify || UsePnP()) {
    feat.Add(NATTypeOID, H460_FeatureContent(natType, sizeof(BYTE))); 
  }
#endif

  return true; 
}


void H460_FeatureStd23::OnReceiveRegistrationConfirm(const H460_FeatureDescriptor & pdu)
{
  H323Gatekeeper * gk = m_endpoint->GetGatekeeper();
  if (PAssertNULL(gk) == NULL)
    return;

  PIPSocket::Address gkIface(gk->GetTransport().GetInterface());

  if (pdu.HasParameter(RASPresentation_ID) && !pdu.GetBooleanParameter(NATPresent_ID)) {
    H323TransportAddress addr = pdu.GetParameter(RASPresentation_ID);
    PIPSocket::Address ip;
    addr.GetIpAddress(ip);
    if (ip != gkIface) {
      m_applicationLevelGateway = true;
      gk->RegistrationRequest(true, false, false); // Force full registration again
    }
  }

#if OPAL_H460_24
  if (!m_applicationLevelGateway && pdu.HasParameter(NATTest_ID)) {
    H323TransportAddress addr = pdu.GetParameter(NATTest_ID);
    m_natMethod->SetPortRanges(m_endpoint->GetManager().GetUDPPortBase(), m_endpoint->GetManager().GetUDPPortMax());
    if (m_natMethod->SetServer(addr.GetHostName(true)) && m_natMethod->Open(gkIface)) {
      PTRACE(3, "Setting STUN server to " << addr);
    }
    else {
      PTRACE(2, "Could not set STUN server to " << addr);
    }
  }
#endif
}


#ifdef H323_UPnP
void H460_FeatureStd23::InitialiseUPnP()
{
  PTRACE(4,"Starting Alternate UPnP");

  PNatMethod_UPnP * natMethod = (PNatMethod_UPnP *)EP->GetNatMethods().LoadNatMethod("UPnP");
  if (natMethod) {
    natMethod->AttachEndPoint(EP);
    EP->GetNatMethods().AddMethod(natMethod);
  }
}


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
          PTRACE(4,"UPnP Change NAT from " << natType << " to " << PSTUNClient::ConeNat);
          natType = PSTUNClient::ConeNat;
          useAlternate = 1;
          EP->NATMethodCallBack(natlist[i].GetName(),1,"Available");
          return true;
        } else {
          PTRACE(4,"UPnP Unavailable subNAT STUN: " << externalIP << " UPnP " << extIP);
          useAlternate = 2;
        }
    }
  }
  return false;
}
#endif


#endif // OPAL_H460_NAT
