// H4601.cxx: implementation of the H460 class.
/*
 * h4601.cxx
 *
 * H.460 Implementation for the h323plus Library.
 *
 * Copyright (c) 2004 ISVO (Asia) Pte Ltd. All Rights Reserved.
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
 * OpenH323 Project (www.openh323.org/)
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

#ifdef __GNUC__
#pragma implementation "h4601.h"
#endif

#include <opal_config.h>

#include "h460/h460.h"

#if OPAL_H460

#include "h460/h4601.h"

#include <ptlib/pluginmgr.h>
#include <h323/h323ep.h>


#if PTRACING
ostream & operator<<(ostream & strm, H460_MessageType pduType)
{
  switch (pduType.m_code) {
  case H460_MessageType::e_gatekeeperRequest :        return strm << "GRQ";
  case H460_MessageType::e_gatekeeperConfirm :        return strm << "GCF";
  case H460_MessageType::e_gatekeeperReject :         return strm << "GRJ";
  case H460_MessageType::e_registrationRequest :      return strm << "RRQ";
  case H460_MessageType::e_registrationConfirm :      return strm << "RCF";
  case H460_MessageType::e_registrationReject :       return strm << "RRJ";
  case H460_MessageType::e_admissionRequest :         return strm << "ARQ";
  case H460_MessageType::e_admissionConfirm :         return strm << "ACF";
  case H460_MessageType::e_admissionReject :          return strm << "ARJ";
  case H460_MessageType::e_locationRequest :          return strm << "LRQ";
  case H460_MessageType::e_locationConfirm :          return strm << "LCF";
  case H460_MessageType::e_locationReject :           return strm << "LRJ";
  case H460_MessageType::e_nonStandardMessage :       return strm << "NonStd";
  case H460_MessageType::e_serviceControlIndication : return strm << "SCI";
  case H460_MessageType::e_serviceControlResponse :   return strm << "SCR";
  case H460_MessageType::e_unregistrationRequest:     return strm << "URQ";
  case H460_MessageType::e_inforequest:               return strm << "IRQ";
  case H460_MessageType::e_inforequestresponse:       return strm << "IRR";
  case H460_MessageType::e_disengagerequest:          return strm << "DRQ";
  case H460_MessageType::e_disengageconfirm:          return strm << "DCF";
  case H460_MessageType::e_setup :                    return strm << "SETUP";
  case H460_MessageType::e_alerting :                 return strm << "ALERTING";
  case H460_MessageType::e_callProceeding :           return strm << "CALL-PROCEEDING";
  case H460_MessageType::e_connect :                  return strm << "CONNECT";
  case H460_MessageType::e_facility :                 return strm << "FACILITY";
  case H460_MessageType::e_releaseComplete :          return strm << "RELEASE-COMPLETE";
  default:                                            return strm << '<' << (unsigned)pduType.m_code << '>';
  }
}
#endif


///////////////////////////////////////////////////////////////////////////////

H460_FeatureID::H460_FeatureID()
{
}


H460_FeatureID::H460_FeatureID(unsigned ID)
{ 
  SetTag(H225_GenericIdentifier::e_standard); 
  PASN_Integer & val = *this;
  val.SetValue(ID);
}


H460_FeatureID::H460_FeatureID(const PASN_ObjectId & id)
{
  SetTag(H225_GenericIdentifier::e_oid);
  PASN_ObjectId & val = *this;
  val = id;
}


H460_FeatureID::H460_FeatureID(const PString & id)
{
  SetTag(H225_GenericIdentifier::e_nonStandard); 
  H225_GloballyUniqueID & val = *this;
  val.SetValue(id);
}


H460_FeatureID::H460_FeatureID(const H225_GenericIdentifier & id)
  : H225_GenericIdentifier(id)
{
}


PString H460_FeatureID::GetOID() const
{
  switch (GetTag()) {
    case H225_GenericIdentifier::e_standard :
      return psprintf("0.0.8.460.%u.0.1", ((const PASN_Integer &)*this).GetValue());
    case H225_GenericIdentifier::e_oid :
      return ((const PASN_ObjectId &)*this).AsString();
    default :
      return PString::Empty();
  }
}


#if PTRACING
static PString TraceFeatureID(const H460_FeatureID & id)
{
  switch (id.GetTag()) {
    case H225_GenericIdentifier::e_standard :
      return "Std " + PString(((const PASN_Integer &)id).GetValue());

    case H225_GenericIdentifier::e_oid :
      return "OID " + ((const PASN_ObjectId &)id).AsString();

    case H225_GenericIdentifier::e_nonStandard :
      PGloballyUniqueID guid((const H225_GloballyUniqueID &)id);
      return "NonStd " + guid.AsString();
  }

  return "unknown";
}
#endif


//////////////////////////////////////////////////////////////////////

H460_FeatureContent::H460_FeatureContent()
{
}


H460_FeatureContent::H460_FeatureContent(const PASN_OctetString & param) 
{
  SetTag(H225_Content::e_raw); 
  PASN_OctetString & val = *this;
  val.SetValue(param);
}


H460_FeatureContent::H460_FeatureContent(const PString & param) 
{
  SetTag(H225_Content::e_text); 
  PASN_IA5String & val = *this;
  val = param;
}


H460_FeatureContent::H460_FeatureContent(const PASN_BMPString & param) 
{
  SetTag(H225_Content::e_unicode); 
  PASN_BMPString & val = *this;
  val.SetValue(param);
}


H460_FeatureContent::H460_FeatureContent(bool param) 
{
  SetTag(H225_Content::e_bool); 
  PASN_Boolean & val = *this;
  val.SetValue(param);
}


H460_FeatureContent::H460_FeatureContent(unsigned param, unsigned len) 
{
  if (len == 8) {
    SetTag(H225_Content::e_number8);
    PASN_Integer & val = *this;
    val.SetConstraints(PASN_Object::FixedConstraint, 0, 255);
    val.SetValue(param);
  }
  else if (len == 16) {
    SetTag(H225_Content::e_number16);
    PASN_Integer & val = *this;
    val.SetConstraints(PASN_Object::FixedConstraint, 0, 65535);
    val.SetValue(param);
  }
  else if (len == 32) {
    SetTag(H225_Content::e_number32);
    PASN_Integer & val = *this;
    val.SetConstraints(PASN_Object::FixedConstraint, 0, 4294967295U);
    val.SetValue(param);
  }
  else {
    SetTag(H225_Content::e_number8);
    PASN_Integer & val = *this;
    val.SetConstraints(PASN_Object::FixedConstraint, 0, 255);
    val.SetValue(param);
  }
}


H460_FeatureContent::H460_FeatureContent(const H460_FeatureID & id) 
{
  SetTag(H225_Content::e_id); 
  H225_GenericIdentifier & val = *this;
  val = id;
}


H460_FeatureContent::H460_FeatureContent(const H225_AliasAddress & add) 
{
  SetTag(H225_Content::e_alias); 
  H225_AliasAddress & val = *this;
  val = add;
}


H460_FeatureContent::H460_FeatureContent(const PURL & add)
{

  H225_AliasAddress alias;
  alias.SetTag(H225_AliasAddress::e_url_ID);
  PASN_IA5String & url = alias;
  url = add.AsString();

  SetTag(H225_Content::e_alias); 
  H225_AliasAddress & val = *this;
  val = alias;
}


H460_FeatureContent::H460_FeatureContent(const H323TransportAddress & add) 
{
  SetTag(H225_Content::e_transport); 
  H225_TransportAddress & val = *this;
  add.SetPDU(val);
}


H460_FeatureContent::H460_FeatureContent(const OpalGloballyUniqueID & guid)
{
  SetTag(H225_Content::e_id); 
  H225_GenericIdentifier & val = *this;
  val.SetTag(H225_GenericIdentifier::e_nonStandard);
  H225_GloballyUniqueID & id = val;
  id.SetValue(guid.AsString());
}


H460_FeatureContent::H460_FeatureContent(const H225_ArrayOf_EnumeratedParameter & table) 
{
  SetTag(H225_Content::e_compound);
  H225_ArrayOf_EnumeratedParameter & val = *this;
  val = table;
}


H460_FeatureContent::H460_FeatureContent(const H460_Feature & nested) 
{
  SetTag(H225_Content::e_nested); 
  H225_ArrayOf_GenericData & val = *this;
  val.SetSize(1);
  val[0] = nested.GetDescriptor();
}


/////////////////////////////////////////////////////////////////////

H460_FeatureParameter::H460_FeatureParameter()
{
}


H460_FeatureParameter::H460_FeatureParameter(const H460_FeatureID & id)
{
  m_id = id;
}


H460_FeatureParameter::H460_FeatureParameter(const H460_FeatureID & id, const H460_FeatureContent & content)
{
  m_id = id;
  SetContent(content);
}


H460_FeatureParameter::H460_FeatureParameter(const H225_EnumeratedParameter & param)
  : H225_EnumeratedParameter(param)
{
}


H460_FeatureParameter & H460_FeatureParameter::operator=(const PASN_OctetString & value) 
{
  SetContent(*value);
  return *this; 
}


H460_FeatureParameter & H460_FeatureParameter::operator=(const PString & value)
{
  // Check if url;
  PURL url;
  if (url.Parse(value)) {  // Parameter is an Http Address
    SetContent(H460_FeatureContent(url));
    return *this;
  }

  if (value.Find(':') != P_MAX_INDEX) {  
    PStringArray cmds = value.Tokenise(":", false);	
    if (cmds.GetSize() == 2) {	// Parameter is an Address
      SetContent(H323TransportAddress(cmds[0], (WORD)cmds[1].AsUnsigned()));
      return *this;
    }
  }

  SetContent(value);
  return *this; 
}


H460_FeatureParameter & H460_FeatureParameter::operator=( const PASN_BMPString & value) 
{
  SetContent(value);
  return *this; 
}


H460_FeatureParameter & H460_FeatureParameter::operator=(bool value ) 
{
  SetContent(value);
  return *this;
}


H460_FeatureParameter & H460_FeatureParameter::operator=(unsigned value) 
{
  if (value == 0)
    SetContent(H460_FeatureContent(value,32));
  else if (value < 16)
    SetContent(H460_FeatureContent(value,8));
  else if (value < 256)
    SetContent(H460_FeatureContent(value,16));
  else
    SetContent(H460_FeatureContent(value,32));
  return *this;
}


H460_FeatureParameter & H460_FeatureParameter::operator=(const H460_FeatureID & value) 
{
  SetContent(value);
  return *this; 
}


H460_FeatureParameter & H460_FeatureParameter::operator=(const H225_AliasAddress & value) 
{
  SetContent(value);
  return *this; 
}


H460_FeatureParameter & H460_FeatureParameter::operator=(const H323TransportAddress & value)
{
  SetContent(value);
  return *this; 
}


H460_FeatureParameter & H460_FeatureParameter::operator=(const H225_ArrayOf_EnumeratedParameter & value) 
{
  SetContent(value);
  return *this; 
}


H460_FeatureParameter::operator const PASN_OctetString &()  const
{ 
  return (const PASN_OctetString &)m_content;
}


H460_FeatureParameter::operator PString() const
{ 
  return (const PASN_IA5String &)m_content;
}


H460_FeatureParameter::operator const PASN_BMPString &() const
{ 
  return (const PASN_BMPString &)m_content;
}


H460_FeatureParameter::operator bool() const
{ 
  return (const PASN_Boolean &)m_content;
}


H460_FeatureParameter::operator unsigned() const
{ 
  return (const PASN_Integer &)m_content;
}


H460_FeatureParameter::operator const H460_FeatureID &() const
{ 
  const H225_GenericIdentifier & content = m_content;
  return reinterpret_cast<const H460_FeatureID &>(content);
}


H460_FeatureParameter::operator const H225_AliasAddress &() const
{ 
  return (const H225_AliasAddress &)m_content;
}


H460_FeatureParameter::operator H323TransportAddress() const
{
  return (const H225_TransportAddress &)m_content;
}


H460_FeatureParameter::operator const H225_ArrayOf_EnumeratedParameter &() const
{ 	     
  return (const H225_ArrayOf_EnumeratedParameter &)m_content;
}


H460_FeatureParameter::operator PURL() const
{ 	     
  const H225_AliasAddress & content = m_content;
  if (content.GetTag() == H225_AliasAddress::e_url_ID)
    return PURL((PASN_IA5String &)content);

  return PURL(); 
}


H460_FeatureParameter::operator OpalGloballyUniqueID() const
{
  const H225_GenericIdentifier & content = m_content;
  if (content.GetTag() == H225_GenericIdentifier::e_nonStandard) {
    const H225_GloballyUniqueID & id = content;
    return OpalGloballyUniqueID((PASN_OctetString &)id);
  }

  return OpalGloballyUniqueID();
}


void H460_FeatureParameter::SetContent(const H460_FeatureContent & con)
{
  IncludeOptionalField(e_content);
  m_content = con;
}


/////////////////////////////////////////////////////////////////////

H460_FeatureDescriptor::H460_FeatureDescriptor()
{
}


H460_FeatureDescriptor::H460_FeatureDescriptor(const H460_FeatureID & id)
{
  m_id = id;
}


H460_FeatureDescriptor::H460_FeatureDescriptor(const H225_FeatureDescriptor & descriptor)
  : H225_FeatureDescriptor(descriptor)
{
}


H460_FeatureParameter & H460_FeatureDescriptor::AddParameter(const H460_FeatureID & id)
{
  return AddParameter(new H460_FeatureParameter(id));
}


H460_FeatureParameter & H460_FeatureDescriptor::AddParameter(const H460_FeatureID & id, const H460_FeatureContent & content)
{
  return AddParameter(new H460_FeatureParameter(id, content));
}


H460_FeatureParameter & H460_FeatureDescriptor::AddParameter(H460_FeatureParameter * param)
{
  // Ths odd code is because m_parameter hsa a lower constraint so always has at least one entry
  if (HasOptionalField(e_parameters)) {
    m_parameters.Append(param);
    return *param;
  }

  IncludeOptionalField(e_parameters);
  m_parameters[0] = *param;
  delete param;
  return GetParameterAt(0);
}


H460_FeatureParameter & H460_FeatureDescriptor::GetParameter(const H460_FeatureID & id) const
{
  PINDEX index = GetParameterIndex(id);
  if (index != P_MAX_INDEX)
    return GetParameterAt(index);

  static H460_FeatureParameter empty;
  return empty;
}


PINDEX H460_FeatureDescriptor::GetParameterIndex(const H460_FeatureID & id) const
{
  for (PINDEX i = 0; i < m_parameters.GetSize(); i++) {
    if (id == m_parameters[i].m_id)
      return i;
  }

  return P_MAX_INDEX;
}


bool H460_FeatureDescriptor::HasParameter(const H460_FeatureID & id) const
{
  return GetParameterIndex(id) != P_MAX_INDEX;
}


void H460_FeatureDescriptor::RemoveParameterAt(PINDEX index)
{
  m_parameters.RemoveAt(index);
  if (m_parameters.GetSize() <= 1)
    RemoveOptionalField(e_parameters);
}


void H460_FeatureDescriptor::RemoveParameter(const H460_FeatureID & id)
{
  RemoveParameterAt(GetParameterIndex(id));
}


void H460_FeatureDescriptor::ReplaceParameter(const H460_FeatureID & id, const H460_FeatureContent & content)
{
  PTRACE(6, "H460\tReplace ID: " << id  << " content " << content);

  PINDEX index = GetParameterIndex(id); 
  if (index != P_MAX_INDEX)
    GetParameterAt(index).SetContent(content);
}


bool H460_FeatureDescriptor::IsParameterIsUnique(const H460_FeatureID & id) const
{
  for (PINDEX i = 0; i < GetSize(); i++) {
    if (GetParameter(i).GetID() == id)
      return false;
  }

  return true;
}


/////////////////////////////////////////////////////////////////////

H460_Feature::H460_Feature(const H460_FeatureID & id)
  : m_category(Supported)
  , m_endpoint(NULL)
  , m_connection(NULL)
  , m_descriptor(id)
  , m_supportedByRemote(false)
{
}


bool H460_Feature::Initialise(H323EndPoint & ep, H323Connection * con)
{
  m_endpoint = &ep;
  m_connection = con;
  return true;
}


bool H460_Feature::OnSendPDU(H460_MessageType pduType, H460_FeatureDescriptor & pdu)
{
  PTRACE(6,"H460\tEncoding " << pduType << " PDU for " << TraceFeatureID(GetID()));
  pdu = m_descriptor;

  switch (pduType) {
  case H460_MessageType::e_gatekeeperRequest:
    return OnSendGatekeeperRequest(pdu);
  case H460_MessageType::e_gatekeeperConfirm:
    return OnSendGatekeeperConfirm(pdu);
  case H460_MessageType::e_gatekeeperReject:
    return OnSendGatekeeperReject(pdu);
  case H460_MessageType::e_registrationRequest:
    return OnSendRegistrationRequest(pdu);
  case H460_MessageType::e_registrationConfirm:
    return OnSendRegistrationConfirm(pdu);
  case H460_MessageType::e_registrationReject:
    return OnSendRegistrationReject(pdu);
  case H460_MessageType::e_admissionRequest:
    return OnSendAdmissionRequest(pdu);
  case H460_MessageType::e_admissionConfirm:
    return OnSendAdmissionConfirm(pdu);
  case H460_MessageType::e_admissionReject:
    return OnSendAdmissionReject(pdu);
  case H460_MessageType::e_locationRequest:
    return OnSendLocationRequest(pdu);
  case H460_MessageType::e_locationConfirm:
    return OnSendLocationConfirm(pdu);
  case H460_MessageType::e_locationReject:
    return OnSendLocationReject(pdu);
  case H460_MessageType::e_nonStandardMessage:
    return OnSendNonStandardMessage(pdu);
  case H460_MessageType::e_serviceControlIndication:
    return OnSendServiceControlIndication(pdu);
  case H460_MessageType::e_serviceControlResponse:
    return OnSendServiceControlResponse(pdu);
  case H460_MessageType::e_unregistrationRequest:
    return OnSendUnregistrationRequest(pdu);
  case H460_MessageType::e_inforequest:
    return OnSendInfoRequestMessage(pdu);
  case H460_MessageType::e_inforequestresponse:
    return OnSendInfoRequestResponseMessage(pdu);
  case H460_MessageType::e_disengagerequest:
    return OnSendDisengagementRequestMessage(pdu);
  case H460_MessageType::e_disengageconfirm:
    return OnSendDisengagementConfirmMessage(pdu);
  case H460_MessageType::e_setup:
    return OnSendSetup_UUIE(pdu);
  case H460_MessageType::e_alerting:
    return OnSendAlerting_UUIE(pdu);
  case H460_MessageType::e_callProceeding:
    return OnSendCallProceeding_UUIE(pdu);
  case H460_MessageType::e_connect:
    return OnSendCallConnect_UUIE(pdu);
  case H460_MessageType::e_facility:
    return OnSendFacility_UUIE(pdu);
  case H460_MessageType::e_releaseComplete:
    return OnSendReleaseComplete_UUIE(pdu);
  default:
    return OnSendUnAllocatedPDU(pdu);
  }
}


void H460_Feature::OnReceivePDU(H460_MessageType pduType, const H460_FeatureDescriptor & pdu)
{
  PTRACE(6,"H460\tDecoding " << pduType << " PDU for " << TraceFeatureID(GetID()));
  m_supportedByRemote = true;

  switch (pduType) {
  case H460_MessageType::e_gatekeeperRequest:
    OnReceiveGatekeeperRequest(pdu);
    break;
  case H460_MessageType::e_gatekeeperConfirm:
    OnReceiveGatekeeperConfirm(pdu);
    break;
  case H460_MessageType::e_gatekeeperReject:
    OnReceiveGatekeeperReject(pdu);
    break;
  case H460_MessageType::e_registrationRequest:
    OnReceiveRegistrationRequest(pdu);
    break;
  case H460_MessageType::e_registrationConfirm:
    OnReceiveRegistrationConfirm(pdu);
    break;
  case H460_MessageType::e_registrationReject:
    OnReceiveRegistrationReject(pdu);
    break;
  case H460_MessageType::e_admissionRequest:
    OnReceiveAdmissionRequest(pdu);
    break;
  case H460_MessageType::e_admissionConfirm:
    OnReceiveAdmissionConfirm(pdu);
    break;
  case H460_MessageType::e_admissionReject:
    OnReceiveAdmissionReject(pdu);
    break;
  case H460_MessageType::e_locationRequest:
    OnReceiveLocationRequest(pdu);
    break;
  case H460_MessageType::e_locationConfirm:
    OnReceiveLocationConfirm(pdu);
    break;
  case H460_MessageType::e_locationReject:
    OnReceiveLocationReject(pdu);
    break;
  case H460_MessageType::e_nonStandardMessage:
    OnReceiveNonStandardMessage(pdu);
    break;
  case H460_MessageType::e_serviceControlIndication:
    OnReceiveServiceControlIndication(pdu);
    break;
  case H460_MessageType::e_serviceControlResponse:
    OnReceiveServiceControlResponse(pdu);
    break;
  case H460_MessageType::e_unregistrationRequest:
    OnReceiveUnregistrationRequest(pdu);
    break;
  case H460_MessageType::e_inforequest:
    OnReceiveInfoRequestMessage(pdu);
    break;
  case H460_MessageType::e_inforequestresponse:
    OnReceiveInfoRequestResponseMessage(pdu);
    break;
  case H460_MessageType::e_disengagerequest:
    OnReceiveDisengagementRequestMessage(pdu);
    break;
  case H460_MessageType::e_disengageconfirm:
    OnReceiveDisengagementConfirmMessage(pdu);
    break;
  case H460_MessageType::e_setup:
    OnReceiveSetup_UUIE(pdu);
    break;
  case H460_MessageType::e_alerting:
    OnReceiveAlerting_UUIE(pdu);
    break;
  case H460_MessageType::e_callProceeding:
    OnReceiveCallProceeding_UUIE(pdu);
    break;
  case H460_MessageType::e_connect:
    OnReceiveCallConnect_UUIE(pdu);
    break;
  case H460_MessageType::e_facility:
    OnReceiveFacility_UUIE(pdu);
    break;
  case H460_MessageType::e_releaseComplete :
    OnReceiveReleaseComplete_UUIE(pdu);
    break;
  default:
    OnReceivedUnAllocatedPDU(pdu);
  }
}


/////////////////////////////////////////////////////////////////////

PStringList H460_Feature::GetFeatureNames(PPluginManager * pluginMgr)
{
  return PPluginManager::GetPluginsProviding(pluginMgr, PPlugin_H460_Feature::ServiceType(), false);
}


H460_Feature * H460_Feature::CreateFeature(const PString & featurename, H460_Feature::Purpose purpose, PPluginManager * pluginMgr)
{
  return PPluginManager::CreatePluginAs<H460_Feature>(pluginMgr, featurename, PPlugin_H460_Feature::ServiceType(), purpose.AsBits());
}


/////////////////////////////////////////////////////////////////////

H460_FeatureSet::H460_FeatureSet(H323EndPoint & ep, H460_Feature::Purpose purpose)
  : m_endpoint(ep)
  , m_purpose(purpose)
{
}


H460_FeatureSet::~H460_FeatureSet()
{
  for (iterator it = begin(); it != end(); ++it)
    delete it->second;
}


void H460_FeatureSet::LoadFeatureSet(H323Connection * con)
{
  H460_FeatureSet * baseSet = m_endpoint.GetFeatures();

  PStringList features = H460_Feature::GetFeatureNames();
  for (PINDEX i = 0; i < features.GetSize(); i++) {
    if (!m_endpoint.OnLoadFeature(m_purpose, features[i])) {
      PTRACE(4,"H460\tFeature " << features[i] << " disabled due to policy.");
      continue;
    }

    H460_Feature * feature = NULL;
    if (baseSet != NULL && (feature = baseSet->GetFeature(features[i])) != NULL && (feature->GetPurpose() & m_purpose))
      feature = feature->CloneAs<H460_Feature>();
    else if ((feature = H460_Feature::CreateFeature(features[i], m_purpose)) == NULL) {
      PTRACE(4,"H460\tFeature " << features[i] << " disabled due to purpose.");
      continue;
    }

    if (feature->Initialise(m_endpoint, con))
      AddFeature(feature);
    else
      delete feature;
  }
}


bool H460_FeatureSet::LoadFeature(const PString & featid)
{
  return AddFeature(H460_Feature::CreateFeature(featid, m_purpose));
}


bool H460_FeatureSet::AddFeature(H460_Feature * feat)
{
  if (feat == NULL)
    return false;

  PTRACE(4, "H460\tLoaded feature " << TraceFeatureID(feat->GetID()));
  if (insert(value_type(feat->GetID(), feat)).second)
    return true;

  delete feat;
  return false;
}


void H460_FeatureSet::RemoveFeature(H460_FeatureID id)
{
  PTRACE(4, "H460\tRemoved " << TraceFeatureID(id));
  erase(id);
}


void H460_FeatureSet::OnReceivePDU(H460_MessageType pduType, const H225_FeatureSet & pdu)
{
  PTRACE(5, "H460\tOnReceivePDU FeatureSet " << pduType << " PDU");

  if (pdu.HasOptionalField(H225_FeatureSet::e_neededFeatures))
    OnReceivePDU(pduType, pdu.m_neededFeatures);

  if (pdu.HasOptionalField(H225_FeatureSet::e_desiredFeatures))
    OnReceivePDU(pduType, pdu.m_desiredFeatures);

  if (pdu.HasOptionalField(H225_FeatureSet::e_supportedFeatures))
    OnReceivePDU(pduType, pdu.m_supportedFeatures);
}


void H460_FeatureSet::OnReceivePDU(H460_MessageType pduType, const H225_ArrayOf_FeatureDescriptor & descriptors)
{
  for (PINDEX i = 0; i < descriptors.GetSize(); i++) {
    const H460_FeatureDescriptor & descriptor = reinterpret_cast<const H460_FeatureDescriptor &>(descriptors[i]);
    iterator feature = find(descriptor.GetID());
    if (feature != end()) {
      feature->second->SetCategory(H460_Feature::Needed);
      feature->second->OnReceivePDU(pduType, descriptor);
    }
  }
}


bool H460_FeatureSet::OnSendPDU(H460_MessageType pduType, H225_FeatureSet & pdu)
{
  PTRACE(6,"H460\tCreate FeatureSet " << pduType << " PDU");

  bool builtPDU = false;

  for (iterator it = begin(); it != end(); ++it) {    // Iterate thro the features
    H460_FeatureDescriptor descriptor;
    if (it->second->OnSendPDU(pduType, descriptor) && descriptor.GetDataLength() > 0) {
      // Add it to the correct feature list
      H225_ArrayOf_FeatureDescriptor * afd = NULL;
      switch (it->second->GetCategory()) {
        case H460_Feature::Needed:
          pdu.IncludeOptionalField(H225_FeatureSet::e_neededFeatures);
          afd = &pdu.m_neededFeatures;
          break;

        case H460_Feature::Desired:
          pdu.IncludeOptionalField(H225_FeatureSet::e_desiredFeatures);
          afd = &pdu.m_desiredFeatures;
          break;

        case H460_Feature::Supported:
          pdu.IncludeOptionalField(H225_FeatureSet::e_supportedFeatures);
          afd = &pdu.m_supportedFeatures; 
          break;

        default :
          break;
      }

      if (afd != NULL) {
        PTRACE(5, "H460\tLoading Feature " << TraceFeatureID(it->first) << " as " 
               << it->second->GetCategory() << " feature to " << pduType << " PDU\n" << descriptor);
        PINDEX lastPos = afd->GetSize();
        afd->SetSize(lastPos+1);
        (*afd)[lastPos] = descriptor;
        builtPDU = true;
      }
    }
  }

  return builtPDU;
}


bool H460_FeatureSet::HasFeature(const H460_FeatureID & id)
{
  return find(id) != end();
}


H460_Feature * H460_FeatureSet::GetFeature(const H460_FeatureID & id)
{
  iterator it = find(id);
  return it != end() ? it->second : NULL;
}


#endif // OPAL_H460
