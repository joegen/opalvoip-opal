/*
 * h460_std18.cxx
 *
 * Copyright (c) 2008 ISVO (Asia) Pte. Ltd.
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
 *
 * The Initial Developer of the Original Code is ISVO (Asia) Pte. Ltd.
 *
 * Portions of this code were written with the assisance of funding from
 * triple-IT. http://www.triple-it.nl.
 *
 * Contributor(s): Many thanks to Simon Horne.
 *                 Robert Jongbloed (robertj@voxlucida.com.au).
 *
09:15:04 +0100 (Tue, 12 Aug 2014) $
 */

#include <ptlib.h>

#define P_FORCE_STATIC_PLUGIN 1

#include <h460/h460_std18.h>

#if OPAL_H460_NAT

#include <h323/h323ep.h>
#include <h323/h323pdu.h>
#include <h460/h46018.h>


const H460_FeatureID & H460_FeatureStd18::ID() { static const H460_FeatureID id(18); return id; }

static const H460_FeatureID IncomingCallIndication_ID(1);


///////////////////////////////////////////////////////
// H.460.18
//
// Must Declare for Factory Loader.
H460_FEATURE(Std18, "H.460.18");

H460_FeatureStd18::H460_FeatureStd18()
: H460_Feature(ID())
{
  PTRACE(6,"Std18\tInstance Created");
}


bool H460_FeatureStd18::Initialise(H323EndPoint & ep, H323Connection * con)
{
  if (!H460_Feature::Initialise(ep, con))
    return false;

  if (con == NULL)
    return true; // If gatekeeper, we allow that always

  // We only enable on connection IF the gatekeeper supports H.460.18
  return IsFeatureNegotiatedOnGk(ID());
}


bool H460_FeatureStd18::IsNegotiated() const
{
  return m_connection != NULL ? IsFeatureNegotiatedOnGk(ID()) : H460_Feature::IsNegotiated();
}


void H460_FeatureStd18::OnReceiveServiceControlIndication(const H460_FeatureDescriptor & pdu) 
{
  if (!pdu.HasParameter(1)) {
    PTRACE(4,"Std18\tERROR: Received SCI without Call Indication!");
    return;
  }

  const PASN_OctetString & raw = pdu.GetParameter(IncomingCallIndication_ID);

  H46018_IncomingCallIndication callinfo;
  if (!raw.DecodeSubType(callinfo)) {
    PTRACE(2,"H46018\tUnable to decode incoming call Indication."); 
    return;
  }

  PTRACE(4, "H46018\tSCI: Processing Incoming call request\n  " << callinfo);

  OpalGloballyUniqueID callId(callinfo.m_callID.m_guid);
  if (callId.IsEmpty()) {
    PTRACE(3, "H46018\tTCP Connect Abort: No Call identifier");
    return;
  }

  // Fix for Tandberg boxes that send duplicate SCI messages.
  if (callId == m_lastCallIdentifer) {
    PTRACE(2,"H46018\tDuplicate Call Identifer " << m_lastCallIdentifer << " Ignoring request!"); 
    return;
  }

  m_lastCallIdentifer = callId;

  // We throw the socket creation onto another thread as with UMTS networks it may take several 
  // seconds to actually create the connection and we don't want to wait before signalling back
  // to the gatekeeper. This also speeds up connection time which is also nice :) - SH
  new PThreadObj2Arg<H460_FeatureStd18, H323TransportAddress, OpalGloballyUniqueID>(*this,
            callinfo.m_callSignallingAddress, callId, &H460_FeatureStd18::ConnectThreadMain, true, "H.460.18");
}


bool H460_FeatureStd18::OnStartControlChannel()
{
  if (!IsNegotiated())
    return false;

  // according to H.460.18 cl.11 we have to send a generic Indication on the opening of a
  // H.245 control channel. Details are specified in H.460.18 cl.16
  // This must be the first PDU otherwise gatekeeper/proxy will close the channel.

  H323ControlPDU pdu;
  H245_GenericMessage & cap = pdu.Build(H245_IndicationMessage::e_genericIndication);

  H245_CapabilityIdentifier & id = cap.m_messageIdentifier;
  id.SetTag(H245_CapabilityIdentifier::e_standard);
  PASN_ObjectId & gid = id;
  gid.SetValue("0.0.8.460.18.0.1");

  cap.IncludeOptionalField(H245_GenericMessage::e_subMessageIdentifier);
  ((PASN_Integer &)cap.m_subMessageIdentifier).SetValue(1);

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
  raw.SetValue(m_connection->GetCallIdentifier());
  msg.SetSize(1);
  msg[0] = call;

  // Is receiver
  if (!m_connection->IsOriginating()) {
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
  return m_connection->WriteControlPDU(pdu);
}


void H460_FeatureStd18::ConnectThreadMain(H323TransportAddress address, OpalGloballyUniqueID callId)
{
  PTRACE(3, "H46018\tConnecting " << callId << " to server at " << address);

  H323Gatekeeper * gk = m_endpoint->GetGatekeeper();
  if (!PAssert(gk != NULL, PLogicError))
    return;

  OpalTransportAddress localAddress(gk->GetTransport().GetInterface(), 0, address.GetProtoPrefix());
  OpalTransportPtr transport(localAddress.CreateTransport(*m_endpoint, OpalTransportAddress::Streamed));
  if (transport == NULL) {
    PTRACE(3, "H46018\tFailed to create TCP connection on " << localAddress);
    return;
  }

  if (!transport->ConnectTo(address)) {
    PTRACE(3, "H46018\tFailed to TCP Connect to " << address);
    return;
  }

  H323SignalPDU pdu;
  pdu.BuildFacility(callId);
  pdu.m_h323_uu_pdu.m_h245Tunneling = !m_endpoint->IsH245TunnelingDisabled();
  if (!pdu.Write(*transport)) {
    PTRACE(3, "H46018\tError Writing PDU.");
    return;
  }

  // Thread now owned by OpalTransport
  PThread * thread = PThread::Current();
  transport->AttachThread(thread);
  thread->SetNoAutoDelete();

  PTRACE(4, "H46018\tEstablished call for " << callId << ", awaiting SETUP.");
  m_endpoint->InternalNewIncomingConnection(transport);
}


#endif // OPAL_H460_NAT
