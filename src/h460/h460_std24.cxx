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
 * $Revision$
 * $Author$
 * $Date$
 */

#include <ptlib.h>

#define P_FORCE_STATIC_PLUGIN 1

#include <h460/h460_std24.h>

#if OPAL_H460_NAT

#include <h323/h323ep.h>
#include <h460/h460_std19.h>
#include <h460/h460_std23.h>

#pragma message("H.460.23/.24 Enabled. Contact consulting@h323plus.org for licensing terms.")

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
    H460_FeatureStd24 * feat24 = GetFeatureOnGkAs<H460_FeatureStd24>(ID());
    if (feat24 == NULL) {
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
    PTRACE(4, "Disabled via H.460.24 strategy.");
    return false;
  }

  return PSTUNClient::IsAvailable(binding, context);
}


#endif // OPAL_H460_NAT
