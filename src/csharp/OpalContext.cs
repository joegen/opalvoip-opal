/* ----------------------------------------------------------------------------
 * This file was automatically generated by SWIG (http://www.swig.org).
 * Version 2.0.9
 *
 * Do not make changes to this file unless you know what you are doing--modify
 * the SWIG interface file instead.
 * ----------------------------------------------------------------------------- */


using System;
using System.Runtime.InteropServices;

public class OpalContext : IDisposable {
  private HandleRef swigCPtr;
  protected bool swigCMemOwn;

  internal OpalContext(IntPtr cPtr, bool cMemoryOwn) {
    swigCMemOwn = cMemoryOwn;
    swigCPtr = new HandleRef(this, cPtr);
  }

  internal static HandleRef getCPtr(OpalContext obj) {
    return (obj == null) ? new HandleRef(null, IntPtr.Zero) : obj.swigCPtr;
  }

  ~OpalContext() {
    Dispose();
  }

  public virtual void Dispose() {
    lock(this) {
      if (swigCPtr.Handle != IntPtr.Zero) {
        if (swigCMemOwn) {
          swigCMemOwn = false;
          OPALPINVOKE.delete_OpalContext(swigCPtr);
        }
        swigCPtr = new HandleRef(null, IntPtr.Zero);
      }
      GC.SuppressFinalize(this);
    }
  }

  public OpalContext() : this(OPALPINVOKE.new_OpalContext(), true) {
  }

  public uint Initialise(string options, uint version) {
    uint ret = OPALPINVOKE.OpalContext_Initialise__SWIG_0(swigCPtr, options, version);
    return ret;
  }

  public uint Initialise(string options) {
    uint ret = OPALPINVOKE.OpalContext_Initialise__SWIG_1(swigCPtr, options);
    return ret;
  }

  public bool IsInitialised() {
    bool ret = OPALPINVOKE.OpalContext_IsInitialised(swigCPtr);
    return ret;
  }

  public void ShutDown() {
    OPALPINVOKE.OpalContext_ShutDown(swigCPtr);
  }

  public bool GetMessage(OpalMessagePtr message, uint timeout) {
    bool ret = OPALPINVOKE.OpalContext_GetMessage__SWIG_0(swigCPtr, OpalMessagePtr.getCPtr(message), timeout);
    if (OPALPINVOKE.SWIGPendingException.Pending) throw OPALPINVOKE.SWIGPendingException.Retrieve();
    return ret;
  }

  public bool GetMessage(OpalMessagePtr message) {
    bool ret = OPALPINVOKE.OpalContext_GetMessage__SWIG_1(swigCPtr, OpalMessagePtr.getCPtr(message));
    if (OPALPINVOKE.SWIGPendingException.Pending) throw OPALPINVOKE.SWIGPendingException.Retrieve();
    return ret;
  }

  public bool SendMessage(OpalMessagePtr message) {
    bool ret = OPALPINVOKE.OpalContext_SendMessage__SWIG_0(swigCPtr, OpalMessagePtr.getCPtr(message));
    if (OPALPINVOKE.SWIGPendingException.Pending) throw OPALPINVOKE.SWIGPendingException.Retrieve();
    return ret;
  }

  public bool SendMessage(OpalMessagePtr message, OpalMessagePtr response) {
    bool ret = OPALPINVOKE.OpalContext_SendMessage__SWIG_1(swigCPtr, OpalMessagePtr.getCPtr(message), OpalMessagePtr.getCPtr(response));
    if (OPALPINVOKE.SWIGPendingException.Pending) throw OPALPINVOKE.SWIGPendingException.Retrieve();
    return ret;
  }

  public bool SetUpCall(OpalMessagePtr response, string partyB, string partyA, string alertingType) {
    bool ret = OPALPINVOKE.OpalContext_SetUpCall__SWIG_0(swigCPtr, OpalMessagePtr.getCPtr(response), partyB, partyA, alertingType);
    if (OPALPINVOKE.SWIGPendingException.Pending) throw OPALPINVOKE.SWIGPendingException.Retrieve();
    return ret;
  }

  public bool SetUpCall(OpalMessagePtr response, string partyB, string partyA) {
    bool ret = OPALPINVOKE.OpalContext_SetUpCall__SWIG_1(swigCPtr, OpalMessagePtr.getCPtr(response), partyB, partyA);
    if (OPALPINVOKE.SWIGPendingException.Pending) throw OPALPINVOKE.SWIGPendingException.Retrieve();
    return ret;
  }

  public bool SetUpCall(OpalMessagePtr response, string partyB) {
    bool ret = OPALPINVOKE.OpalContext_SetUpCall__SWIG_2(swigCPtr, OpalMessagePtr.getCPtr(response), partyB);
    if (OPALPINVOKE.SWIGPendingException.Pending) throw OPALPINVOKE.SWIGPendingException.Retrieve();
    return ret;
  }

  public bool AnswerCall(string callToken) {
    bool ret = OPALPINVOKE.OpalContext_AnswerCall(swigCPtr, callToken);
    return ret;
  }

  public bool ClearCall(string callToken, OpalCallEndReason reason) {
    bool ret = OPALPINVOKE.OpalContext_ClearCall__SWIG_0(swigCPtr, callToken, (int)reason);
    return ret;
  }

  public bool ClearCall(string callToken) {
    bool ret = OPALPINVOKE.OpalContext_ClearCall__SWIG_1(swigCPtr, callToken);
    return ret;
  }

  public bool SendUserInput(string callToken, string userInput, uint duration) {
    bool ret = OPALPINVOKE.OpalContext_SendUserInput__SWIG_0(swigCPtr, callToken, userInput, duration);
    return ret;
  }

  public bool SendUserInput(string callToken, string userInput) {
    bool ret = OPALPINVOKE.OpalContext_SendUserInput__SWIG_1(swigCPtr, callToken, userInput);
    return ret;
  }

}
