/*
 * h450pdu.cxx
 *
 * H.450 Helper functions
 *
 * Open H323 Library
 *
 * Copyright (c) 2001 Norwood Systems Pty. Ltd.
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
 * $Log: h450pdu.cxx,v $
 * Revision 1.2001  2001/07/27 15:48:25  robertj
 * Conversion of OpenH323 to Open Phone Abstraction Library (OPAL)
 *
 * Revision 1.5  2001/06/14 06:25:16  robertj
 * Added further H.225 PDU build functions.
 * Moved some functionality from connection to PDU class.
 *
 * Revision 1.4  2001/06/05 03:14:41  robertj
 * Upgraded H.225 ASN to v4 and H.245 ASN to v7.
 *
 * Revision 1.3  2001/05/09 04:59:04  robertj
 * Bug fixes in H.450.2, thanks Klein Stefan.
 *
 * Revision 1.2  2001/04/20 02:16:53  robertj
 * Removed GNU C++ warnings.
 *
 * Revision 1.1  2001/04/11 03:01:29  robertj
 * Added H.450.2 (call transfer), thanks a LOT to Graeme Reid & Norwood Systems
 *
 */

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "h450pdu.h"
#endif

#include <h323/h450pdu.h>

#include <h323/transaddr.h>
#include <h323/h323pdu.h>


///////////////////////////////////////////////////////////////////////////////

X880_Invoke& H450ServiceAPDU::BuildInvoke(int invokeId, int operation)
{
  SetTag(X880_ROS::e_invoke);
  X880_Invoke& invoke = (X880_Invoke&) *this;

  invoke.m_invokeId = invokeId;

  invoke.m_opcode.SetTag(X880_Code::e_local);
  PASN_Integer& opcode = (PASN_Integer&) invoke.m_opcode;
  opcode.SetValue(operation);

  return invoke;
}


X880_ReturnResult& H450ServiceAPDU::BuildReturnResult(int invokeId)
{
  SetTag(X880_ROS::e_returnResult);
  X880_ReturnResult& returnResult = (X880_ReturnResult&) *this;

  returnResult.m_invokeId = invokeId;

  return returnResult;
}


X880_ReturnError& H450ServiceAPDU::BuildReturnError(int invokeId, int error)
{
  SetTag(X880_ROS::e_returnError);
  X880_ReturnError& returnError = (X880_ReturnError&) *this;

  returnError.m_invokeId = invokeId;

  returnError.m_errorCode.SetTag(X880_Code::e_local);
  PASN_Integer& errorCode = (PASN_Integer&) returnError.m_errorCode;
  errorCode.SetValue(error);

  return returnError;
}


X880_Reject& H450ServiceAPDU::BuildReject(int invokeId)
{
  SetTag(X880_ROS::e_reject);
  X880_Reject& reject = (X880_Reject&) *this;

  reject.m_invokeId = invokeId;

  return reject;
}


void H450ServiceAPDU::BuildCallTransferInitiate(int invokeId,
                                                const PString & callIdentity,
                                                const PString & alias,
                                                const H323TransportAddress & address)
{
  X880_Invoke& invoke = BuildInvoke(invokeId, H4502_CallTransferOperation::e_callTransferInitiate);

  H4502_CTInitiateArg argument;

  argument.m_callIdentity = callIdentity;

  H4501_ArrayOf_AliasAddress& aliasAddress = argument.m_reroutingNumber.m_destinationAddress;

  if (alias.IsEmpty()) {
    aliasAddress.SetSize(1);
  }
  else {
    aliasAddress.SetSize(2);
    aliasAddress[1].SetTag(H225_AliasAddress::e_dialedDigits);
    H323SetAliasAddress(alias, aliasAddress[1]);
  }

  aliasAddress[0].SetTag(H225_AliasAddress::e_transportID);
  H225_TransportAddress& cPartyTransport = (H225_TransportAddress&) aliasAddress[0];
  address.SetPDU(cPartyTransport);

  invoke.IncludeOptionalField(X880_Invoke::e_argument);
  invoke.m_argument.EncodeSubType(argument);
}


void H450ServiceAPDU::BuildCallTransferSetup(int invokeId,
                                             const PString & callIdentity)
{
  X880_Invoke& invoke = BuildInvoke(invokeId, H4502_CallTransferOperation::e_callTransferSetup);

  H4502_CTSetupArg argument;

  argument.m_callIdentity = callIdentity;

  invoke.IncludeOptionalField(X880_Invoke::e_argument);
  invoke.m_argument.EncodeSubType(argument);
}


void H450ServiceAPDU::ParseEndpointAddress(H4501_EndpointAddress& endpointAddress,
                                           PString& remoteParty)
{
  H4501_ArrayOf_AliasAddress& destinationAddress = endpointAddress.m_destinationAddress;

  PString alias;
  H323TransportAddress transportAddress;

  for (PINDEX i = 0; i < destinationAddress.GetSize(); i++) {
    H225_AliasAddress& aliasAddress = destinationAddress[i];

    if (aliasAddress.GetTag() == H225_AliasAddress::e_transportID)
      transportAddress = (H225_TransportAddress &)aliasAddress;
    else
      alias = ::H323GetAliasAddressString(aliasAddress);
  }

  if (alias.IsEmpty()) {
    remoteParty = transportAddress;
  }
  else {
    remoteParty = alias + '@' + transportAddress;
  }
}


/////////////////////////////////////////////////////////////////////////////
