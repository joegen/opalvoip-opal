/* ----------------------------------------------------------------------------
 * This file was automatically generated by SWIG (http://www.swig.org).
 * Version 2.0.9
 *
 * Do not make changes to this file unless you know what you are doing--modify
 * the SWIG interface file instead.
 * ----------------------------------------------------------------------------- */


using System;
using System.Runtime.InteropServices;

public class OpalParamProtocol : IDisposable {
  private HandleRef swigCPtr;
  protected bool swigCMemOwn;

  internal OpalParamProtocol(IntPtr cPtr, bool cMemoryOwn) {
    swigCMemOwn = cMemoryOwn;
    swigCPtr = new HandleRef(this, cPtr);
  }

  internal static HandleRef getCPtr(OpalParamProtocol obj) {
    return (obj == null) ? new HandleRef(null, IntPtr.Zero) : obj.swigCPtr;
  }

  ~OpalParamProtocol() {
    Dispose();
  }

  public virtual void Dispose() {
    lock(this) {
      if (swigCPtr.Handle != IntPtr.Zero) {
        if (swigCMemOwn) {
          swigCMemOwn = false;
          OPALPINVOKE.delete_OpalParamProtocol(swigCPtr);
        }
        swigCPtr = new HandleRef(null, IntPtr.Zero);
      }
      GC.SuppressFinalize(this);
    }
  }

  public string prefix {
    set {
      OPALPINVOKE.OpalParamProtocol_prefix_set(swigCPtr, value);
    } 
    get {
      string ret = OPALPINVOKE.OpalParamProtocol_prefix_get(swigCPtr);
      return ret;
    } 
  }

  public string userName {
    set {
      OPALPINVOKE.OpalParamProtocol_userName_set(swigCPtr, value);
    } 
    get {
      string ret = OPALPINVOKE.OpalParamProtocol_userName_get(swigCPtr);
      return ret;
    } 
  }

  public string displayName {
    set {
      OPALPINVOKE.OpalParamProtocol_displayName_set(swigCPtr, value);
    } 
    get {
      string ret = OPALPINVOKE.OpalParamProtocol_displayName_get(swigCPtr);
      return ret;
    } 
  }

  public OpalProductDescription product {
    set {
      OPALPINVOKE.OpalParamProtocol_product_set(swigCPtr, OpalProductDescription.getCPtr(value));
    } 
    get {
      IntPtr cPtr = OPALPINVOKE.OpalParamProtocol_product_get(swigCPtr);
      OpalProductDescription ret = (cPtr == IntPtr.Zero) ? null : new OpalProductDescription(cPtr, false);
      return ret;
    } 
  }

  public string interfaceAddresses {
    set {
      OPALPINVOKE.OpalParamProtocol_interfaceAddresses_set(swigCPtr, value);
    } 
    get {
      string ret = OPALPINVOKE.OpalParamProtocol_interfaceAddresses_get(swigCPtr);
      return ret;
    } 
  }

  public OpalUserInputModes userInputMode {
    set {
      OPALPINVOKE.OpalParamProtocol_userInputMode_set(swigCPtr, (int)value);
    } 
    get {
      OpalUserInputModes ret = (OpalUserInputModes)OPALPINVOKE.OpalParamProtocol_userInputMode_get(swigCPtr);
      return ret;
    } 
  }

  public string defaultOptions {
    set {
      OPALPINVOKE.OpalParamProtocol_defaultOptions_set(swigCPtr, value);
    } 
    get {
      string ret = OPALPINVOKE.OpalParamProtocol_defaultOptions_get(swigCPtr);
      return ret;
    } 
  }

  public OpalParamProtocol() : this(OPALPINVOKE.new_OpalParamProtocol(), true) {
  }

}
