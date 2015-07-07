//------------------------------------------------------------------------------
// <auto-generated />
//
// This file was automatically generated by SWIG (http://www.swig.org).
// Version 3.0.5
//
// Do not make changes to this file unless you know what you are doing--modify
// the SWIG interface file instead.
//------------------------------------------------------------------------------


public class OpalParamRegistration : global::System.IDisposable {
  private global::System.Runtime.InteropServices.HandleRef swigCPtr;
  protected bool swigCMemOwn;

  internal OpalParamRegistration(global::System.IntPtr cPtr, bool cMemoryOwn) {
    swigCMemOwn = cMemoryOwn;
    swigCPtr = new global::System.Runtime.InteropServices.HandleRef(this, cPtr);
  }

  internal static global::System.Runtime.InteropServices.HandleRef getCPtr(OpalParamRegistration obj) {
    return (obj == null) ? new global::System.Runtime.InteropServices.HandleRef(null, global::System.IntPtr.Zero) : obj.swigCPtr;
  }

  ~OpalParamRegistration() {
    Dispose();
  }

  public virtual void Dispose() {
    lock(this) {
      if (swigCPtr.Handle != global::System.IntPtr.Zero) {
        if (swigCMemOwn) {
          swigCMemOwn = false;
          OPALPINVOKE.delete_OpalParamRegistration(swigCPtr);
        }
        swigCPtr = new global::System.Runtime.InteropServices.HandleRef(null, global::System.IntPtr.Zero);
      }
      global::System.GC.SuppressFinalize(this);
    }
  }

  public string protocol {
    set {
      OPALPINVOKE.OpalParamRegistration_protocol_set(swigCPtr, value);
    } 
    get {
      string ret = OPALPINVOKE.OpalParamRegistration_protocol_get(swigCPtr);
      return ret;
    } 
  }

  public string identifier {
    set {
      OPALPINVOKE.OpalParamRegistration_identifier_set(swigCPtr, value);
    } 
    get {
      string ret = OPALPINVOKE.OpalParamRegistration_identifier_get(swigCPtr);
      return ret;
    } 
  }

  public string hostName {
    set {
      OPALPINVOKE.OpalParamRegistration_hostName_set(swigCPtr, value);
    } 
    get {
      string ret = OPALPINVOKE.OpalParamRegistration_hostName_get(swigCPtr);
      return ret;
    } 
  }

  public string authUserName {
    set {
      OPALPINVOKE.OpalParamRegistration_authUserName_set(swigCPtr, value);
    } 
    get {
      string ret = OPALPINVOKE.OpalParamRegistration_authUserName_get(swigCPtr);
      return ret;
    } 
  }

  public string password {
    set {
      OPALPINVOKE.OpalParamRegistration_password_set(swigCPtr, value);
    } 
    get {
      string ret = OPALPINVOKE.OpalParamRegistration_password_get(swigCPtr);
      return ret;
    } 
  }

  public string adminEntity {
    set {
      OPALPINVOKE.OpalParamRegistration_adminEntity_set(swigCPtr, value);
    } 
    get {
      string ret = OPALPINVOKE.OpalParamRegistration_adminEntity_get(swigCPtr);
      return ret;
    } 
  }

  public uint timeToLive {
    set {
      OPALPINVOKE.OpalParamRegistration_timeToLive_set(swigCPtr, value);
    } 
    get {
      uint ret = OPALPINVOKE.OpalParamRegistration_timeToLive_get(swigCPtr);
      return ret;
    } 
  }

  public uint restoreTime {
    set {
      OPALPINVOKE.OpalParamRegistration_restoreTime_set(swigCPtr, value);
    } 
    get {
      uint ret = OPALPINVOKE.OpalParamRegistration_restoreTime_get(swigCPtr);
      return ret;
    } 
  }

  public string eventPackage {
    set {
      OPALPINVOKE.OpalParamRegistration_eventPackage_set(swigCPtr, value);
    } 
    get {
      string ret = OPALPINVOKE.OpalParamRegistration_eventPackage_get(swigCPtr);
      return ret;
    } 
  }

  public string attributes {
    set {
      OPALPINVOKE.OpalParamRegistration_attributes_set(swigCPtr, value);
    } 
    get {
      string ret = OPALPINVOKE.OpalParamRegistration_attributes_get(swigCPtr);
      return ret;
    } 
  }

  public OpalParamRegistration() : this(OPALPINVOKE.new_OpalParamRegistration(), true) {
  }

}
