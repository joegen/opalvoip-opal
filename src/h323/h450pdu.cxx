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
 * Revision 1.2003  2001/08/17 08:29:27  robertj
 * Update from OpenH323
 *
 * Revision 2.1  2001/08/13 05:10:40  robertj
 * Updates from OpenH323 v1.6.0 release.
 *
 * Revision 2.0  2001/07/27 15:48:25  robertj
 * Conversion of OpenH323 to Open Phone Abstraction Library (OPAL)
 *
 * Revision 1.7  2001/08/16 07:49:19  robertj
 * Changed the H.450 support to be more extensible. Protocol handlers
 *   are now in separate classes instead of all in H323Connection.
 *
 * Revision 1.6  2001/08/06 03:08:57  robertj
 * Fission of h323.h to h323ep.h & h323con.h, h323.h now just includes files.
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
#include <h323/h323ep.h>
#include <h323/h323con.h>
#include <asn/h4502.h>
#include <asn/h4504.h>


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


void H450ServiceAPDU::AttachSupplementaryServiceAPDU(H323SignalPDU & pdu)
{
  H4501_SupplementaryService supplementaryService;

  // Create an H.450.1 supplementary service object
  // and store the H450ServiceAPDU in the ROS array.
  supplementaryService.m_serviceApdu.SetTag(H4501_ServiceApdus::e_rosApdus);
  H4501_ArrayOf_ROS & operations = (H4501_ArrayOf_ROS &)supplementaryService.m_serviceApdu;
  operations.SetSize(1);
  operations[0] = *this;

  // Add the H.450 PDU to the H.323 User-to-User PDU as an OCTET STRING
  pdu.m_h323_uu_pdu.IncludeOptionalField(H225_H323_UU_PDU::e_h4501SupplementaryService);
  pdu.m_h323_uu_pdu.m_h4501SupplementaryService.SetSize(1);
  pdu.m_h323_uu_pdu.m_h4501SupplementaryService[0].EncodeSubType(supplementaryService);
}


BOOL H450ServiceAPDU::WriteFacilityPDU(H323Connection & connection)
{
  H323SignalPDU facilityPDU;
  facilityPDU.BuildFacility(connection, TRUE);

  AttachSupplementaryServiceAPDU(facilityPDU);

  return connection.WriteSignalPDU(facilityPDU);
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

H450xDispatcher::H450xDispatcher(H323Connection & conn)
  : connection(conn)
{
  opcodeHandler.DisallowDeleteObjects();

  nextInvokeId = 0;
}


void H450xDispatcher::AddOpCode(unsigned opcode, H450xHandler * handler)
{
  PAssertNULL(handler);
  if (handlers.GetObjectsIndex(handler) == P_MAX_INDEX)
    handlers.Append(handler);

  opcodeHandler.SetAt(opcode, handler);
}


void H450xDispatcher::AttachToSetup(H323SignalPDU & pdu)
{
  for (PINDEX i = 0; i < handlers.GetSize(); i++)
    handlers[i].AttachToSetup(pdu);
}


void H450xDispatcher::AttachToAlerting(H323SignalPDU & pdu)
{
  for (PINDEX i = 0; i < handlers.GetSize(); i++)
    handlers[i].AttachToAlerting(pdu);
}


void H450xDispatcher::AttachToConnect(H323SignalPDU & pdu)
{
  for (PINDEX i = 0; i < handlers.GetSize(); i++)
    handlers[i].AttachToConnect(pdu);
}


void H450xDispatcher::AttachToReleaseComplete(H323SignalPDU & pdu)
{
  for (PINDEX i = 0; i < handlers.GetSize(); i++)
    handlers[i].AttachToReleaseComplete(pdu);
}


void H450xDispatcher::HandlePDU(const H323SignalPDU & pdu)
{
  for (PINDEX i = 0; i < pdu.m_h323_uu_pdu.m_h4501SupplementaryService.GetSize(); i++) {
    H4501_SupplementaryService supplementaryService;

    // Decode the supplementary service PDU from the PPER Stream
    if (pdu.m_h323_uu_pdu.m_h4501SupplementaryService[i].DecodeSubType(supplementaryService)) {
      PTRACE(4, "H4501\tSupplementary service PDU:\n  "
             << setprecision(2) << supplementaryService);
    }
    else {
      PTRACE(1, "H4501\tInvalid supplementary service PDU decode:\n  "
             << setprecision(2) << supplementaryService);
      continue;
    }

    if (supplementaryService.m_serviceApdu.GetTag() == H4501_ServiceApdus::e_rosApdus) {
      H4501_ArrayOf_ROS& operations = (H4501_ArrayOf_ROS&) supplementaryService.m_serviceApdu;

      for (PINDEX j = 0; j < operations.GetSize(); j ++) {
        X880_ROS& operation = operations[j];

        PTRACE(3, "H4501\tX880 ROS " << operation.GetTagName());

        switch (operation.GetTag()) {
          case X880_ROS::e_invoke:
            OnReceivedInvoke((X880_Invoke &)operation);
            break;

          case X880_ROS::e_returnResult:
            OnReceivedReturnResult((X880_ReturnResult &)operation);
            break;

          case X880_ROS::e_returnError:
            OnReceivedReturnError((X880_ReturnError &)operation);
            break;

          case X880_ROS::e_reject:
            OnReceivedReject((X880_Reject &)operation);
            break;

          default :
            break;
        }
      }
    }
  }
}


void H450xDispatcher::OnReceivedInvoke(X880_Invoke & invoke)
{
  // Get the invokeId
  int invokeId = invoke.m_invokeId.GetValue();

  // Get the linkedId if present
  int linkedId = -1;
  if (invoke.HasOptionalField(X880_Invoke::e_linkedId)) {
    linkedId = invoke.m_linkedId.GetValue();
  }

  // Get the argument if present
  PASN_OctetString * argument = NULL;
  if (invoke.HasOptionalField(X880_Invoke::e_argument)) {
    argument = &invoke.m_argument;
  }

  // Get the opcode
  if (invoke.m_opcode.GetTag() == X880_Code::e_local) {
    int opcode = ((PASN_Integer&) invoke.m_opcode).GetValue();
    if (!opcodeHandler.Contains(opcode) ||
        !opcodeHandler[opcode].OnReceivedInvoke(opcode, invokeId, linkedId, argument)) {
      PTRACE(2, "H4501\tInvoke of unsupported local opcode:\n  " << invoke);
      SendInvokeReject(invokeId, 1 /*X880_InvokeProblem::e_unrecognisedOperation*/);
    }
  }
  else {
    SendInvokeReject(invokeId, 1 /*X880_InvokeProblem::e_unrecognisedOperation*/);
    PTRACE(2, "H4501\tInvoke of unsupported global opcode:\n  " << invoke);
  }
}


void H450xDispatcher::OnReceivedReturnResult(X880_ReturnResult & returnResult)
{
  int invokeId = returnResult.m_invokeId.GetValue();

  for (PINDEX i = 0; i < handlers.GetSize(); i++) {
    if (handlers[i].GetInvokeId() == invokeId) {
      handlers[i].OnReceivedReturnResult(returnResult);
      break;
    }
  }
}


void H450xDispatcher::OnReceivedReturnError(X880_ReturnError & returnError)
{
  int invokeId = returnError.m_invokeId.GetValue();
  int errorCode = 0;

  if (returnError.m_errorCode.GetTag() == X880_Code::e_local)
    errorCode = ((PASN_Integer&) returnError.m_errorCode).GetValue();

  for (PINDEX i = 0; i < handlers.GetSize(); i++) {
    if (handlers[i].GetInvokeId() == invokeId) {
      handlers[i].OnReceivedReturnError(errorCode, returnError);
      break;
    }
  }
}


void H450xDispatcher::OnReceivedReject(X880_Reject & reject)
{
  int problem = 0;

  switch (reject.m_problem.GetTag()) {
    case X880_Reject_problem::e_general:
    {
      X880_GeneralProblem & generalProblem = reject.m_problem;
      problem = generalProblem.GetValue();
    }
    break;

    case X880_Reject_problem::e_invoke:
    {
      X880_InvokeProblem & invokeProblem = reject.m_problem;
      problem = invokeProblem.GetValue();
    }
    break;

    case X880_Reject_problem::e_returnResult:
    {
      X880_ReturnResultProblem & returnResultProblem = reject.m_problem;
      problem = returnResultProblem.GetValue();
    }
    break;

    case X880_Reject_problem::e_returnError:
    {
      X880_ReturnErrorProblem & returnErrorProblem = reject.m_problem;
      problem = returnErrorProblem.GetValue();
    }
    break;

    default:
      break;
  }


  int invokeId = reject.m_invokeId;
  for (PINDEX i = 0; i < handlers.GetSize(); i++) {
    if (handlers[i].GetInvokeId() == invokeId) {
      handlers[i].OnReceivedReject(reject.m_problem.GetTag(), problem);
      break;
    }
  }
}


void H450xDispatcher::SendReturnError(int invokeId, int returnError)
{
  H450ServiceAPDU serviceAPDU;

  serviceAPDU.BuildReturnError(invokeId, returnError);

  serviceAPDU.WriteFacilityPDU(connection);
}


void H450xDispatcher::SendGeneralReject(int invokeId, int problem)
{
  H450ServiceAPDU serviceAPDU;

  X880_Reject & reject = serviceAPDU.BuildReject(invokeId);
  reject.m_problem.SetTag(X880_Reject_problem::e_general);
  X880_GeneralProblem & generalProblem = (X880_GeneralProblem &) reject.m_problem;
  generalProblem = problem;

  serviceAPDU.WriteFacilityPDU(connection);
}


void H450xDispatcher::SendInvokeReject(int invokeId, int problem)
{
  H450ServiceAPDU serviceAPDU;

  X880_Reject & reject = serviceAPDU.BuildReject(invokeId);
  reject.m_problem.SetTag(X880_Reject_problem::e_invoke);
  X880_InvokeProblem & invokeProblem = (X880_InvokeProblem &) reject.m_problem;
  invokeProblem = problem;

  serviceAPDU.WriteFacilityPDU(connection);
}


void H450xDispatcher::SendReturnResultReject(int invokeId, int problem)
{
  H450ServiceAPDU serviceAPDU;

  X880_Reject & reject = serviceAPDU.BuildReject(invokeId);
  reject.m_problem.SetTag(X880_Reject_problem::e_returnResult);
  X880_ReturnResultProblem & returnResultProblem = reject.m_problem;
  returnResultProblem = problem;

  serviceAPDU.WriteFacilityPDU(connection);
}


void H450xDispatcher::SendReturnErrorReject(int invokeId, int problem)
{
  H450ServiceAPDU serviceAPDU;

  X880_Reject & reject = serviceAPDU.BuildReject(invokeId);
  reject.m_problem.SetTag(X880_Reject_problem::e_returnError);
  X880_ReturnErrorProblem & returnErrorProblem = reject.m_problem;
  returnErrorProblem = problem;

  serviceAPDU.WriteFacilityPDU(connection);
}


/////////////////////////////////////////////////////////////////////////////

H450xHandler::H450xHandler(H323Connection & conn, H450xDispatcher & disp)
  : endpoint(conn.GetEndPoint()),
    connection(conn),
    dispatcher(disp)
{
  currentInvokeId = -1;
}


void H450xHandler::AttachToSetup(H323SignalPDU &)
{
}


void H450xHandler::AttachToAlerting(H323SignalPDU &)
{
}


void H450xHandler::AttachToConnect(H323SignalPDU &)
{
}


void H450xHandler::AttachToReleaseComplete(H323SignalPDU &)
{
}


void H450xHandler::OnReceivedReturnResult(X880_ReturnResult & /*returnResult*/)
{
}


void H450xHandler::OnReceivedReturnError(int /*errorCode*/,
                                        X880_ReturnError & /*returnError*/)
{
}


void H450xHandler::OnReceivedReject(int /*problemType*/,
                                   int /*problemNumber*/)
{
}


void H450xHandler::SendReturnError(int returnError)
{
  dispatcher.SendReturnError(currentInvokeId, returnError);
  currentInvokeId = -1;
}


void H450xHandler::SendGeneralReject(int problem)
{
  dispatcher.SendGeneralReject(currentInvokeId, problem);
  currentInvokeId = -1;
}


void H450xHandler::SendInvokeReject(int problem)
{
  dispatcher.SendInvokeReject(currentInvokeId, problem);
  currentInvokeId = -1;
}


void H450xHandler::SendReturnResultReject(int problem)
{
  dispatcher.SendReturnResultReject(currentInvokeId, problem);
  currentInvokeId = -1;
}


void H450xHandler::SendReturnErrorReject(int problem)
{
  dispatcher.SendReturnErrorReject(currentInvokeId, problem);
  currentInvokeId = -1;
}


BOOL H450xHandler::DecodeArguments(PASN_OctetString * argString,
                                  PASN_Object & argObject,
                                  int absentErrorCode)
{
  if (argString == NULL) {
    if (absentErrorCode >= 0)
      SendReturnError(absentErrorCode);
    return FALSE;
  }

  PPER_Stream argStream(*argString);
  if (argObject.Decode(argStream)) {
    PTRACE(4, "H4501\tSupplementary service argument:\n  "
           << setprecision(2) << argObject);
    return TRUE;
  }

  PTRACE(1, "H4501\tInvalid supplementary service argument:\n  "
         << setprecision(2) << argObject);
  return FALSE;
}


/////////////////////////////////////////////////////////////////////////////

H4502Handler::H4502Handler(H323Connection & conn, H450xDispatcher & disp)
  : H450xHandler(conn, disp)
{
  dispatcher.AddOpCode(H4502_CallTransferOperation::e_callTransferIdentify, this);
  dispatcher.AddOpCode(H4502_CallTransferOperation::e_callTransferAbandon, this);
  dispatcher.AddOpCode(H4502_CallTransferOperation::e_callTransferInitiate, this);
  dispatcher.AddOpCode(H4502_CallTransferOperation::e_callTransferSetup, this);
  dispatcher.AddOpCode(H4502_CallTransferOperation::e_callTransferUpdate, this);
  dispatcher.AddOpCode(H4502_CallTransferOperation::e_subaddressTransfer, this);
  dispatcher.AddOpCode(H4502_CallTransferOperation::e_callTransferComplete, this);
  dispatcher.AddOpCode(H4502_CallTransferOperation::e_callTransferActive, this);

  transferringCallToken = "";
  ctState = e_ctIdle;
  ctResponseSent = FALSE;
}


void H4502Handler::AttachToSetup(H323SignalPDU & pdu)
{
  // Do we need to attach a call transfer setup invoke APDU?
  if (ctState != e_ctAwaitSetupResponse)
    return;

  H450ServiceAPDU serviceAPDU;

  // Store the outstanding invokeID associated with this connection
  currentInvokeId = dispatcher.GetNextInvokeId();

  // Use the call identity from the ctInitiateArg
  serviceAPDU.BuildCallTransferSetup(currentInvokeId, transferringCallIdentity);

  serviceAPDU.AttachSupplementaryServiceAPDU(pdu);
}


void H4502Handler::AttachToAlerting(H323SignalPDU & pdu)
{
  // Do we need to send a callTransferSetup return result APDU?
  if (currentInvokeId == -1 || ctResponseSent)
    return;

  H450ServiceAPDU serviceAPDU;
  serviceAPDU.BuildReturnResult(currentInvokeId);
  serviceAPDU.AttachSupplementaryServiceAPDU(pdu);
  ctResponseSent = TRUE;
}


void H4502Handler::AttachToConnect(H323SignalPDU & pdu)
{
  // Do we need to include a ctInitiateReturnResult APDU in our Release Complete Message?
  if (currentInvokeId == -1 || ctResponseSent)
    return;

  H450ServiceAPDU serviceAPDU;
  serviceAPDU.BuildReturnResult(currentInvokeId);
  serviceAPDU.AttachSupplementaryServiceAPDU(pdu);
  ctResponseSent = TRUE;
}


void H4502Handler::AttachToReleaseComplete(H323SignalPDU & pdu)
{
  // Do we need to include a ctInitiateReturnResult APDU in our Release Complete Message?
  if (currentInvokeId == -1)
    return;

  // If the SETUP message we received from the other end had a callTransferSetup APDU
  // in it, then we need to send back a RELEASE COMPLETE PDU with a callTransferSetup 
  // ReturnError.
  // Else normal call - clear it down
  H450ServiceAPDU serviceAPDU;

  if (ctResponseSent)
    serviceAPDU.BuildReturnResult(currentInvokeId);
  else {
    serviceAPDU.BuildReturnError(currentInvokeId, H4501_GeneralErrorList::e_notAvailable);
    ctResponseSent = TRUE;
  }

  serviceAPDU.AttachSupplementaryServiceAPDU(pdu);
}


BOOL H4502Handler::OnReceivedInvoke(int opcode,
                                    int invokeId,
                                    int linkedId,
                                    PASN_OctetString * argument)
{
  currentInvokeId = invokeId;

  switch (opcode) {
    case H4502_CallTransferOperation::e_callTransferIdentify:
      OnReceivedCallTransferIdentify(linkedId);
      break;

    case H4502_CallTransferOperation::e_callTransferAbandon:
      OnReceivedCallTransferAbandon(linkedId);
      break;

    case H4502_CallTransferOperation::e_callTransferInitiate:
      OnReceivedCallTransferInitiate(linkedId, argument);
      break;

    case H4502_CallTransferOperation::e_callTransferSetup:
      OnReceivedCallTransferSetup(linkedId, argument);
      break;

    case H4502_CallTransferOperation::e_callTransferUpdate:
      OnReceivedCallTransferUpdate(linkedId, argument);
      break;

    case H4502_CallTransferOperation::e_subaddressTransfer:
      OnReceivedSubaddressTransfer(linkedId, argument);
      break;

    case H4502_CallTransferOperation::e_callTransferComplete:
      OnReceivedCallTransferComplete(linkedId, argument);
      break;

    case H4502_CallTransferOperation::e_callTransferActive:
      OnReceivedCallTransferActive(linkedId, argument);
      break;

    default:
      currentInvokeId = -1;
      return FALSE;
  }

  return TRUE;
}


void H4502Handler::OnReceivedCallTransferIdentify(int /*linkedId*/)
{
}


void H4502Handler::OnReceivedCallTransferAbandon(int /*linkedId*/)
{
}


void H4502Handler::OnReceivedCallTransferInitiate(int /*linkedId*/,
                                                  PASN_OctetString * argument)
{
  // TBD: Check Call Hold status. If call is held, it must first be 
  // retrieved before being transferred. -- dcassel 4/01

  H4502_CTInitiateArg ctInitiateArg;
  if (!DecodeArguments(argument, ctInitiateArg,
                       H4502_CallTransferErrors::e_invalidReroutingNumber))
    return;

  PString remoteParty;
  H450ServiceAPDU::ParseEndpointAddress(ctInitiateArg.m_reroutingNumber, remoteParty);

  if (!endpoint.OnCallTransferInitiate(connection, remoteParty) ||
       endpoint.SetupTransfer(connection.GetToken(),
                              ctInitiateArg.m_callIdentity.GetValue(),
                              remoteParty) == NULL)
    SendReturnError(H4502_CallTransferErrors::e_establishmentFailure);
}


void H4502Handler::OnReceivedCallTransferSetup(int /*linkedId*/,
                                               PASN_OctetString * argument)
{
  H4502_CTSetupArg ctSetupArg;
  if (!DecodeArguments(argument, ctSetupArg,
                       H4502_CallTransferErrors::e_unrecognizedCallIdentity))
    return;

  PString callIdentity;
  switch (ctState) {
    case e_ctIdle:
      callIdentity = ctSetupArg.m_callIdentity;
      break;

    case e_ctAwaitSetup:
      // stop timer CT-T2
      // Need to check that the call identity and destination address match
      // those in the Identify message; for now we just check for empty.
      callIdentity = ctSetupArg.m_callIdentity;
      break;
  }

  if (callIdentity.IsEmpty())
    SendReturnError(H4502_CallTransferErrors::e_unrecognizedCallIdentity);
  else
    ctState = e_ctAwaitSetupResponse;
}


void H4502Handler::OnReceivedCallTransferUpdate(int /*linkedId*/,
                                                PASN_OctetString * argument)
{
  H4502_CTUpdateArg ctUpdateArg;
  if (!DecodeArguments(argument, ctUpdateArg, -1))
    return;

}


void H4502Handler::OnReceivedSubaddressTransfer(int /*linkedId*/,
                                                PASN_OctetString * argument)
{
  H4502_SubaddressTransferArg subaddressTransferArg;
  if (!DecodeArguments(argument, subaddressTransferArg, -1))
    return;

}


void H4502Handler::OnReceivedCallTransferComplete(int /*linkedId*/,
                                                  PASN_OctetString * argument)
{
  H4502_CTCompleteArg ctCompleteArg;
  if (!DecodeArguments(argument, ctCompleteArg, -1))
    return;

}


void H4502Handler::OnReceivedCallTransferActive(int /*linkedId*/,
                                                PASN_OctetString * argument)
{
  H4502_CTActiveArg ctActiveArg;
  if (!DecodeArguments(argument, ctActiveArg, -1))
    return;

}


void H4502Handler::OnReceivedReturnResult(X880_ReturnResult &)
{
  switch (ctState) {
    case e_ctAwaitInitiateResponse:
      // stop timer CT-T3
      // clear the primary call, if it exists
      ctState = e_ctIdle;
      break;

    case e_ctAwaitSetupResponse:
      // stop timer CT-T4

      // Clear the call
      ctState = e_ctIdle;
      endpoint.ClearCall(transferringCallToken, EndedByCallForwarded);
      break;
  }
}


void H4502Handler::OnReceivedReturnError(int errorCode, X880_ReturnError &)
{
  switch (ctState) {
    case e_ctAwaitInitiateResponse:
      // stop timer CT-T3
      // clear the primary call, if it exists
      ctState = e_ctIdle;
      break;

    case e_ctAwaitSetupResponse:
      // stop timer CT-T4

      // Send a facility to the transferring endpoint
      // containing a call transfer initiate return error
      H323Connection* existingConnection = endpoint.FindConnectionWithLock(transferringCallToken);

      if (existingConnection != NULL) {
        H450ServiceAPDU serviceAPDU;
        serviceAPDU.BuildReturnError(existingConnection->GetCallTransferInvokeId(), errorCode);
        serviceAPDU.WriteFacilityPDU(*existingConnection);
        existingConnection->Unlock();
      }

      ctState = e_ctIdle;
      break;
  }
}


void H4502Handler::TransferCall(const PString & remoteParty)
{
  currentInvokeId = dispatcher.GetNextInvokeId();

  // Send a FACILITY message with a callTransferInitiate Invoke
  // Supplementary Service PDU to the transferred endpoint.
  H450ServiceAPDU serviceAPDU;

  PString alias;
  H323TransportAddress address;
  endpoint.ParsePartyName(remoteParty, alias, address);

  PString callIdentity;
  serviceAPDU.BuildCallTransferInitiate(currentInvokeId, callIdentity, alias, address);
  serviceAPDU.WriteFacilityPDU(connection);

  // start timer CT-T3
  ctState = e_ctAwaitInitiateResponse;
}


void H4502Handler::AwaitSetupResponse(const PString & token,
                                      const PString & identity)
{
  transferringCallToken = token;
  transferringCallIdentity = identity;
  ctState = e_ctAwaitSetupResponse;
  // start timer CT-T4
}


/////////////////////////////////////////////////////////////////////////////

H4504Handler::H4504Handler(H323Connection & conn, H450xDispatcher & disp)
  : H450xHandler(conn, disp)
{
  dispatcher.AddOpCode(H4504_CallHoldOperation::e_holdNotific, this);
  dispatcher.AddOpCode(H4504_CallHoldOperation::e_retrieveNotific, this);
  dispatcher.AddOpCode(H4504_CallHoldOperation::e_remoteHold, this);
  dispatcher.AddOpCode(H4504_CallHoldOperation::e_remoteRetrieve, this);

  holdState = e_ch_Idle;
}


BOOL H4504Handler::OnReceivedInvoke(int opcode,
                                    int invokeId,
                                    int linkedId,
                                    PASN_OctetString *)
{
  currentInvokeId = invokeId;

  switch (opcode) {
    case H4504_CallHoldOperation::e_holdNotific:
      OnReceivedLocalCallHold(linkedId);
      break;

    case H4504_CallHoldOperation::e_retrieveNotific:
      OnReceivedLocalCallRetrieve(linkedId);
      break;

    case H4504_CallHoldOperation::e_remoteHold:
      OnReceivedRemoteCallHold(linkedId);
      break;

    case H4504_CallHoldOperation::e_remoteRetrieve:
      OnReceivedRemoteCallRetrieve(linkedId);
      break;

    default:
      currentInvokeId = -1;
      return FALSE;
  }

  return TRUE;
}


void H4504Handler::OnReceivedLocalCallHold(int /*linkedId*/)
{
  // Shut down media channels
  connection.SendCapabilitySet(TRUE);
}


void H4504Handler::OnReceivedLocalCallRetrieve(int /*linkedId*/)
{
  // Reactivate media channels
  connection.SendCapabilitySet(FALSE);
}


void H4504Handler::OnReceivedRemoteCallHold(int /*linkedId*/)
{
	// TBD
}


void H4504Handler::OnReceivedRemoteCallRetrieve(int /*linkedId*/)
{
	// TBD
}


void H4504Handler::HoldCall(BOOL localHold)
{
  // TBD: Implement Remote Hold. This implementation only does 
  // local hold. -- dcassel 4/01. 
  if (!localHold)
    return;
  
  // Send a FACILITY message with a callNotific Invoke
  // Supplementary Service PDU to the held endpoint.
  H450ServiceAPDU serviceAPDU;

  currentInvokeId = dispatcher.GetNextInvokeId();
  serviceAPDU.BuildInvoke(currentInvokeId, H4504_CallHoldOperation::e_holdNotific);
  serviceAPDU.WriteFacilityPDU(connection);
  
  // Shut down media channels
  connection.SendCapabilitySet(TRUE);
  
  // Update hold state
  holdState = e_ch_NE_Held;
}


void H4504Handler::RetrieveCall(bool localHold)
{
  // TBD: Implement Remote Hold. This implementation only does 
  // local hold. -- dcassel 4/01. 
  if (!localHold)
    return;
  
  // Send a FACILITY message with a retrieveNotific Invoke
  // Supplementary Service PDU to the held endpoint.
  currentInvokeId = dispatcher.GetNextInvokeId();
  H450ServiceAPDU serviceAPDU;
  
  serviceAPDU.BuildInvoke(currentInvokeId, H4504_CallHoldOperation::e_retrieveNotific);
  serviceAPDU.WriteFacilityPDU(connection);
  
  // Reactivate media channels
  connection.SendCapabilitySet(FALSE);
  
  // Update hold state
  holdState = e_ch_Idle;
}


/////////////////////////////////////////////////////////////////////////////
