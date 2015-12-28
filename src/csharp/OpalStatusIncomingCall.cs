//------------------------------------------------------------------------------
// <auto-generated />
//
// This file was automatically generated by SWIG (http://www.swig.org).
// Version 3.0.7
//
// Do not make changes to this file unless you know what you are doing--modify
// the SWIG interface file instead.
//------------------------------------------------------------------------------


public class OpalStatusIncomingCall : global::System.IDisposable {
  private global::System.Runtime.InteropServices.HandleRef swigCPtr;
  protected bool swigCMemOwn;

  internal OpalStatusIncomingCall(global::System.IntPtr cPtr, bool cMemoryOwn) {
    swigCMemOwn = cMemoryOwn;
    swigCPtr = new global::System.Runtime.InteropServices.HandleRef(this, cPtr);
  }

  internal static global::System.Runtime.InteropServices.HandleRef getCPtr(OpalStatusIncomingCall obj) {
    return (obj == null) ? new global::System.Runtime.InteropServices.HandleRef(null, global::System.IntPtr.Zero) : obj.swigCPtr;
  }

  ~OpalStatusIncomingCall() {
    Dispose();
  }

  public virtual void Dispose() {
    lock(this) {
      if (swigCPtr.Handle != global::System.IntPtr.Zero) {
        if (swigCMemOwn) {
          swigCMemOwn = false;
          OPALPINVOKE.delete_OpalStatusIncomingCall(swigCPtr);
        }
        swigCPtr = new global::System.Runtime.InteropServices.HandleRef(null, global::System.IntPtr.Zero);
      }
      global::System.GC.SuppressFinalize(this);
    }
  }

  public string callToken {
    set {
      OPALPINVOKE.OpalStatusIncomingCall_callToken_set(swigCPtr, value);
    } 
    get {
      string ret = OPALPINVOKE.OpalStatusIncomingCall_callToken_get(swigCPtr);
      return ret;
    } 
  }

  public string localAddress {
    set {
      OPALPINVOKE.OpalStatusIncomingCall_localAddress_set(swigCPtr, value);
    } 
    get {
      string ret = OPALPINVOKE.OpalStatusIncomingCall_localAddress_get(swigCPtr);
      return ret;
    } 
  }

  public string remoteAddress {
    set {
      OPALPINVOKE.OpalStatusIncomingCall_remoteAddress_set(swigCPtr, value);
    } 
    get {
      string ret = OPALPINVOKE.OpalStatusIncomingCall_remoteAddress_get(swigCPtr);
      return ret;
    } 
  }

  public string remotePartyNumber {
    set {
      OPALPINVOKE.OpalStatusIncomingCall_remotePartyNumber_set(swigCPtr, value);
    } 
    get {
      string ret = OPALPINVOKE.OpalStatusIncomingCall_remotePartyNumber_get(swigCPtr);
      return ret;
    } 
  }

  public string remoteDisplayName {
    set {
      OPALPINVOKE.OpalStatusIncomingCall_remoteDisplayName_set(swigCPtr, value);
    } 
    get {
      string ret = OPALPINVOKE.OpalStatusIncomingCall_remoteDisplayName_get(swigCPtr);
      return ret;
    } 
  }

  public string calledAddress {
    set {
      OPALPINVOKE.OpalStatusIncomingCall_calledAddress_set(swigCPtr, value);
    } 
    get {
      string ret = OPALPINVOKE.OpalStatusIncomingCall_calledAddress_get(swigCPtr);
      return ret;
    } 
  }

  public string calledPartyNumber {
    set {
      OPALPINVOKE.OpalStatusIncomingCall_calledPartyNumber_set(swigCPtr, value);
    } 
    get {
      string ret = OPALPINVOKE.OpalStatusIncomingCall_calledPartyNumber_get(swigCPtr);
      return ret;
    } 
  }

  public OpalProductDescription product {
    set {
      OPALPINVOKE.OpalStatusIncomingCall_product_set(swigCPtr, OpalProductDescription.getCPtr(value));
    } 
    get {
      global::System.IntPtr cPtr = OPALPINVOKE.OpalStatusIncomingCall_product_get(swigCPtr);
      OpalProductDescription ret = (cPtr == global::System.IntPtr.Zero) ? null : new OpalProductDescription(cPtr, false);
      return ret;
    } 
  }

  public string alertingType {
    set {
      OPALPINVOKE.OpalStatusIncomingCall_alertingType_set(swigCPtr, value);
    } 
    get {
      string ret = OPALPINVOKE.OpalStatusIncomingCall_alertingType_get(swigCPtr);
      return ret;
    } 
  }

  public string protocolCallId {
    set {
      OPALPINVOKE.OpalStatusIncomingCall_protocolCallId_set(swigCPtr, value);
    } 
    get {
      string ret = OPALPINVOKE.OpalStatusIncomingCall_protocolCallId_get(swigCPtr);
      return ret;
    } 
  }

  public string referredByAddress {
    set {
      OPALPINVOKE.OpalStatusIncomingCall_referredByAddress_set(swigCPtr, value);
    } 
    get {
      string ret = OPALPINVOKE.OpalStatusIncomingCall_referredByAddress_get(swigCPtr);
      return ret;
    } 
  }

  public string redirectingNumber {
    set {
      OPALPINVOKE.OpalStatusIncomingCall_redirectingNumber_set(swigCPtr, value);
    } 
    get {
      string ret = OPALPINVOKE.OpalStatusIncomingCall_redirectingNumber_get(swigCPtr);
      return ret;
    } 
  }

  public uint extraCount {
    set {
      OPALPINVOKE.OpalStatusIncomingCall_extraCount_set(swigCPtr, value);
    } 
    get {
      uint ret = OPALPINVOKE.OpalStatusIncomingCall_extraCount_get(swigCPtr);
      return ret;
    } 
  }

  public OpalMIME extras {
    set {
      OPALPINVOKE.OpalStatusIncomingCall_extras_set(swigCPtr, OpalMIME.getCPtr(value));
    } 
    get {
      global::System.IntPtr cPtr = OPALPINVOKE.OpalStatusIncomingCall_extras_get(swigCPtr);
      OpalMIME ret = (cPtr == global::System.IntPtr.Zero) ? null : new OpalMIME(cPtr, false);
      return ret;
    } 
  }

  public string remoteIdentity {
    set {
      OPALPINVOKE.OpalStatusIncomingCall_remoteIdentity_set(swigCPtr, value);
    } 
    get {
      string ret = OPALPINVOKE.OpalStatusIncomingCall_remoteIdentity_get(swigCPtr);
      return ret;
    } 
  }

  public OpalStatusIncomingCall() : this(OPALPINVOKE.new_OpalStatusIncomingCall(), true) {
  }

}
