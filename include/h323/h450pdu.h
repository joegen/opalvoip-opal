/*
 * h450pdu.h
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
 * $Log: h450pdu.h,v $
 * Revision 1.2003  2002/01/14 06:35:57  robertj
 * Updated to OpenH323 v1.7.9
 *
 * Revision 2.1  2001/08/17 08:20:26  robertj
 * Update from OpenH323
 *
 * Revision 2.0  2001/07/27 15:48:24  robertj
 * Conversion of OpenH323 to Open Phone Abstraction Library (OPAL)
 *
 * Revision 1.3  2002/01/14 00:02:40  robertj
 * Added H.450.6
 * Added extra "failure mode" parts of H.250.2.
 * Various other bug fixes.
 *   Thanks Ben Madsen of Norwood Systems
 *
 * Revision 1.2  2001/08/16 07:49:16  robertj
 * Changed the H.450 support to be more extensible. Protocol handlers
 *   are now in separate classes instead of all in H323Connection.
 *
 * Revision 1.1  2001/04/11 03:01:27  robertj
 * Added H.450.2 (call transfer), thanks a LOT to Graeme Reid & Norwood Systems
 *
 */

#ifndef __H323_H450PDU_H
#define __H323_H450PDU_H

#ifdef __GNUC__
#pragma interface
#endif


#include <asn/x880.h>
#include <asn/h4501.h>
#include <asn/h4502.h>


class H323EndPoint;
class H323Connection;
class H323TransportAddress;
class H323SignalPDU;
class H4501_EndpointAddress;


///////////////////////////////////////////////////////////////////////////////

/**PDU definition for H.450 services.
  */
class H450ServiceAPDU : public X880_ROS
{
  public:
    X880_Invoke& BuildInvoke(int invokeId, int operation);
    X880_ReturnResult& BuildReturnResult(int invokeId);
    X880_ReturnError& BuildReturnError(int invokeId, int error);
    X880_Reject& BuildReject(int invokeId);

    void BuildCallTransferInitiate(int invokeId,
                                   const PString & callIdentity,
                                   const PString & alias,
                                   const H323TransportAddress & address);

    void BuildCallTransferIdentify(int invokeId);
    void BuildCallTransferAbandon(int invokeId);
    void BuildCallTransferSetup(int invokeId,
                                const PString & callIdentity);

    void BuildCallWaiting(int invokeId, int numCallsWaiting);

    void AttachSupplementaryServiceAPDU(H323SignalPDU & pdu);
    BOOL WriteFacilityPDU(
      H323Connection & connection
    );

    static void ParseEndpointAddress(H4501_EndpointAddress & address,
                                     PString & party);
};


class H450xDispatcher;

class H450xHandler : public PObject
{
    PCLASSINFO(H450xHandler, PObject);
  public:
    H450xHandler(
      H323Connection & connection,
      H450xDispatcher & dispatcher
    );

    virtual void AttachToSetup(
      H323SignalPDU & pdu
    );

    virtual void AttachToAlerting(
      H323SignalPDU & pdu
    );

    virtual void AttachToConnect(
      H323SignalPDU & pdu
    );

    virtual void AttachToReleaseComplete(
      H323SignalPDU & pdu
    );

    virtual BOOL OnReceivedInvoke(
      int opcode,
      int invokeId,                           /// InvokeId of operation (used in response)
      int linkedId,                           /// InvokeId of associated operation (if any)
      PASN_OctetString * argument             /// Parameters for the initiate operation
    ) = 0;

    virtual void OnReceivedReturnResult(
      X880_ReturnResult & returnResult
    );

    virtual void OnReceivedReturnError(
      int errorCode,
      X880_ReturnError & returnError
    );

    virtual void OnReceivedReject(
      int problemType,
      int problemNumber
    );

    /**Send a return error in response to an invoke operation.
     */
    void SendReturnError(int returnError);

    void SendGeneralReject(int problem);

    void SendInvokeReject(int problem);

    void SendReturnResultReject(int problem);

    void SendReturnErrorReject(int problem);

    BOOL DecodeArguments(
      PASN_OctetString * argString,
      PASN_Object & argObject,
      int absentErrorCode
    );

    int GetInvokeId() const { return currentInvokeId; }


  protected:
    H323EndPoint   & endpoint;
    H323Connection & connection;
    H450xDispatcher & dispatcher;
    int              currentInvokeId;
};

PLIST(H450xHandlerList, H450xHandler);
PDICTIONARY(H450xHandlerDict, POrdinalKey, H450xHandler);


class H450xDispatcher : public PObject
{
    PCLASSINFO(H450xDispatcher, PObject);
  public:
    H450xDispatcher(
      H323Connection & connection
    );

    /**Add a handler for the op code.
      */
    void AddOpCode(
      unsigned opcode,
      H450xHandler * handler
    );

    virtual void AttachToSetup(
      H323SignalPDU & pdu
    );

    virtual void AttachToAlerting(
      H323SignalPDU & pdu
    );

    virtual void AttachToConnect(
      H323SignalPDU & pdu
    );

    virtual void AttachToReleaseComplete(
      H323SignalPDU & pdu
    );

    /** Handle the H.450.x Supplementary Service PDU if present in the H225_H323_UU_PDU
     */
    virtual void HandlePDU(
      const H323SignalPDU & pdu
    );

    /**Handle an incoming X880 Invoke PDU.
       The default behaviour is to attempt to decode the invoke operation
       and call the corresponding OnReceived<Operation> method on the EndPoint.
     */
    virtual void OnReceivedInvoke(X880_Invoke& invoke);

    /**Handle an incoming X880 Return Result PDU.
       The default behaviour is to attempt to match the return result
       to a previous invoke operation and call the corresponding
       OnReceived<Operation>Success method on the EndPoint.
     */
    virtual void OnReceivedReturnResult(X880_ReturnResult& returnResult);

    /**Handle an incoming X880 Return Error PDU.
       The default behaviour is to attempt to match the return error
       to a previous invoke operation and call the corresponding
       OnReceived<Operation>Error method on the EndPoint.
     */
    virtual void OnReceivedReturnError(X880_ReturnError& returnError);

    /**Handle an incoming X880 Reject PDU.
       The default behaviour is to attempt to match the reject
       to a previous invoke, return result or return error operation
       and call OnReceived<Operation>Reject method on the EndPoint.
     */
    virtual void OnReceivedReject(X880_Reject& reject);

    /**Send a return error in response to an invoke operation.
     */
    void SendReturnError(int invokeId, int returnError);

    void SendGeneralReject(int invokeId, int problem);

    void SendInvokeReject(int invokeId, int problem);

    void SendReturnResultReject(int invokeId, int problem);

    void SendReturnErrorReject(int invokeId, int problem);

    /**Get the next available invoke Id for H450 operations
     */
    int GetNextInvokeId() const { return nextInvokeId++; }

  protected:
    H323Connection & connection;
    H450xHandlerList  handlers;
    H450xHandlerDict  opcodeHandler;
    mutable int      nextInvokeId;             // Next available invoke ID for H450 operations
};


class H4502Handler : public H450xHandler
{
    PCLASSINFO(H4502Handler, H450xHandler);
  public:
    H4502Handler(
      H323Connection & connection,
      H450xDispatcher & dispatcher
    );

    virtual void AttachToSetup(
      H323SignalPDU & pdu
    );

    virtual void AttachToAlerting(
      H323SignalPDU & pdu
    );

    virtual void AttachToConnect(
      H323SignalPDU & pdu
    );

    virtual void AttachToReleaseComplete(
      H323SignalPDU & pdu
    );

    virtual BOOL OnReceivedInvoke(
      int opcode,
      int invokeId,                           /// InvokeId of operation (used in response)
      int linkedId,                           /// InvokeId of associated operation (if any)
      PASN_OctetString * argument             /// Parameters for the initiate operation
    );

    /**Handle an incoming Call Transfer Identify operation.
     */
    virtual void OnReceivedCallTransferIdentify(
      int linkedId                            /// InvokeId of associated operation (if any)
    );

    /**Handle an incoming Call Transfer Abandon operation.
     */
    virtual void OnReceivedCallTransferAbandon(
      int linkedId                            /// InvokeId of associated operation (if any)
    );

    /**Handle an incoming Call Transfer Initiate operation.
     */
    virtual void OnReceivedCallTransferInitiate(
      int linkedId,                           /// InvokeId of associated operation (if any)
      PASN_OctetString * argument             /// Parameters for the initiate operation
    );

    /**Handle an incoming Call Transfer Setup operation.
     */
    virtual void OnReceivedCallTransferSetup(
      int linkedId,                           /// InvokeId of associated operation (if any)
      PASN_OctetString * argument             /// Parameters for the setup operation
    );

    /**Handle an incoming Call Transfer Update operation.
     */
    virtual void OnReceivedCallTransferUpdate(
      int linkedId,                           /// InvokeId of associated operation (if any)
      PASN_OctetString * argument             /// Parameters for the update operation
    );

    /**Handle an incoming Subaddress Transfer operation.
     */
    virtual void OnReceivedSubaddressTransfer(
      int linkedId,                           /// InvokeId of associated operation (if any)
      PASN_OctetString * argument             /// Parameters for the subaddress transfer operation
    );

    /**Handle an incoming Call Transfer Complete operation.
     */
    virtual void OnReceivedCallTransferComplete(
      int linkedId,                           /// InvokeId of associated operation (if any)
      PASN_OctetString * argument             /// Parameters for the complete operation
    );

    /**Handle an incoming Call Transfer Active operation.
     */
    virtual void OnReceivedCallTransferActive(
      int linkedId,                           /// InvokeId of associated operation (if any)
      PASN_OctetString * argument             /// Parameters for the active operation
    );

    virtual void OnReceivedReturnResult(
      X880_ReturnResult & returnResult
    );

    virtual void OnReceivedReturnError(
      int errorCode,
      X880_ReturnError & returnError
    );

    /**Initiate the transfer of an existing call (connection) to a new remote party
       using H.450.2.  This sends a Call Transfer Initiate Invoke message from the
       A-Party (transferring endpoint) to the B-Party (transferred endpoint).
     */
    void TransferCall(
      const PString & remoteParty   /// Remote party to transfer the existing call to
    );

    void AwaitSetupResponse(
      const PString & token,
      const PString & identity
    );

    /**Sub-state for call transfer.
      */
    enum State {
      e_ctIdle,
      e_ctAwaitIdentifyResponse,
      e_ctAwaitInitiateResponse,
      e_ctAwaitSetupResponse,
      e_ctAwaitSetup,
      e_ctAwaitConnect
    };

    /**Get the current call transfer state.
     */
    State GetState() const { return ctState; }

    /**Handle the reception of an Admission Reject during a pending call
       transfer operation at the transferred endpoint. If the call transfer
       state of the current connection is e_ctAwaitSetupResponse, the stack
       attempts to find the existing connection between the transferred and
       transferring endpoints and inform this connection that a
       callTransferInitiateReturnError PDU needs to be sent.  No action is
       taken if the current connection is not in call transfer state
       e_ctAwaitSetupResponse.
     */
    virtual void onReceivedAdmissionReject(const int returnError);

    /**Handle the failure of a call transfer operation.
      */
    void HandleCallTransferFailure(const int returnError    // failure reason
    );

    /** Start the Call Transfer Timer using the specified time interval.
     */
    void StartctTimer(const PTimeInterval value) { ctTimer = value; }

    /** Stop the Call Transfer Timer
     */
    void StopctTimer();

    /**Callback mechanism for Call Transfer Timers CT-T1, CT-T2, CT-T3 & CT-T4
     */
    PDECLARE_NOTIFIER(PTimer, H4502Handler, OnCallTransferTimeOut);

    /**Get the connection assoicated with this H4502Handler.
     */
    const H323Connection& getAssociatedConnection() const { return connection; }

    /**Get the transferringCallToken member
     */
    const PString& getTransferringCallToken() const { return transferringCallToken; }

  protected:
    PString transferringCallToken;    // Stores the call token for the transferring connection (if there is one)
    PString transferringCallIdentity; // Stores the call identity for the transferring call (if there is one)
    State   ctState;                  // Call Transfer state of the conneciton
    BOOL    ctResponseSent;           // Has a callTransferSetupReturnResult been sent?
    PTimer  ctTimer;                  // Call Transfer Timer - Handles all four timers CT-T1,
};


class H4504Handler : public H450xHandler
{
    PCLASSINFO(H4504Handler, H450xHandler);
  public:
    H4504Handler(
      H323Connection & connection,
      H450xDispatcher & dispatcher
    );

    virtual BOOL OnReceivedInvoke(
      int opcode,
      int invokeId,                           /// InvokeId of operation (used in response)
      int linkedId,                           /// InvokeId of associated operation (if any)
      PASN_OctetString * argument             /// Parameters for the initiate operation
    );

    /**Handle an incoming Near-End Call Hold operation
    */
    virtual void OnReceivedLocalCallHold(
      int linkedId                            /// InvokeId of associated operation (if any)
    );

    /**Handle an incoming Near-End Call Retrieve operation
    */
    virtual void OnReceivedLocalCallRetrieve(
      int linkedId                            /// InvokeId of associated operation (if any)
    );

    /**Handle an incoming Remote Call Hold operation
    * TBD: Remote hold operations not yet implemented -- dcassel 4/01
    */
    virtual void OnReceivedRemoteCallHold(
      int linkedId                            /// InvokeId of associated operation (if any)
    );

    /**Handle an incoming Remote Call Retrieve operation
    * TBD: Remote hold operations not yet implemented -- dcassel 4/01
    */
    virtual void OnReceivedRemoteCallRetrieve(
      int linkedId                            /// InvokeId of associated operation (if any)
    );

    /**Place the call on hold, suspending all media channels (H.450.4)
    * NOTE: Only Local Hold is implemented so far. 
    */
    void HoldCall(
      BOOL localHold   /// true for Local Hold, false for Remote Hold
    );

    /**Retrieve the call from hold, activating all media channels (H.450.4)
    * NOTE: Only Local Hold is implemented so far. 
    */
    void RetrieveCall();

    /**Sub-state for call hold.
      */
    enum State {
      e_ch_Idle,
      e_ch_NE_Held,
      e_ch_RE_Requested,
      e_ch_RE_Held,
      e_ch_RE_Retrieve_Req
    };

    State GetState() const { return holdState; }


  protected:
    State holdState;  // Call Hold state of this connection
};


class H4506Handler : public H450xHandler
{
    PCLASSINFO(H4506Handler, H450xHandler);
  public:
    H4506Handler(
      H323Connection & connection,
      H450xDispatcher & dispatcher
    );

    virtual BOOL OnReceivedInvoke(
      int opcode,
      int invokeId,                           /// InvokeId of operation (used in response)
      int linkedId,                           /// InvokeId of associated operation (if any)
      PASN_OctetString * argument             /// Parameters for the initiate operation
    );

    /**Handle an incoming Call Waiting Indication PDU
    */
    virtual void OnReceivedCallWaitingIndication(
      int linkedId,
      PASN_OctetString *argument
    );

    /**Attach a call waiting APDU to the passed in Alerting PDU.  The second paramter is used to
       indicate to the calling user how many additional users are "camped on" the called user.  A
       value of zero indicates to the calling user that he/she is the only user attempting to reach
       the busy called user.
    */
    virtual void AttachToAlerting(
      H323SignalPDU & pdu,
      unsigned numberOfCallsWaiting = 0
    );

    /**Sub-state for call waiting.
      */
    enum State {
      e_cw_Idle,
      e_cw_Invoked
    };

    State GetState() const { return cwState; }


  protected:
    State cwState;  // Call Waiting state of this connection
};


#endif // __H323_H450PDU_H


/////////////////////////////////////////////////////////////////////////////
