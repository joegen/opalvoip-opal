/* ----------------------------------------------------------------------------
 * This file was automatically generated by SWIG (http://www.swig.org).
 * Version 2.0.9
 *
 * Do not make changes to this file unless you know what you are doing--modify
 * the SWIG interface file instead.
 * ----------------------------------------------------------------------------- */


using System;
using System.Runtime.InteropServices;

public class OpalStatusRegistration : IDisposable {
  private HandleRef swigCPtr;
  protected bool swigCMemOwn;

  internal OpalStatusRegistration(IntPtr cPtr, bool cMemoryOwn) {
    swigCMemOwn = cMemoryOwn;
    swigCPtr = new HandleRef(this, cPtr);
  }

  internal static HandleRef getCPtr(OpalStatusRegistration obj) {
    return (obj == null) ? new HandleRef(null, IntPtr.Zero) : obj.swigCPtr;
  }

  ~OpalStatusRegistration() {
    Dispose();
  }

  public virtual void Dispose() {
    lock(this) {
      if (swigCPtr.Handle != IntPtr.Zero) {
        if (swigCMemOwn) {
          swigCMemOwn = false;
          OPALPINVOKE.delete_OpalStatusRegistration(swigCPtr);
        }
        swigCPtr = new HandleRef(null, IntPtr.Zero);
      }
      GC.SuppressFinalize(this);
    }
  }

  public string protocol {
    set {
      OPALPINVOKE.OpalStatusRegistration_protocol_set(swigCPtr, value);
    } 
    get {
      string ret = OPALPINVOKE.OpalStatusRegistration_protocol_get(swigCPtr);
      return ret;
    } 
  }

  public string serverName {
    set {
      OPALPINVOKE.OpalStatusRegistration_serverName_set(swigCPtr, value);
    } 
    get {
      string ret = OPALPINVOKE.OpalStatusRegistration_serverName_get(swigCPtr);
      return ret;
    } 
  }

  public string error {
    set {
      OPALPINVOKE.OpalStatusRegistration_error_set(swigCPtr, value);
    } 
    get {
      string ret = OPALPINVOKE.OpalStatusRegistration_error_get(swigCPtr);
      return ret;
    } 
  }

  public OpalRegistrationStates status {
    set {
      OPALPINVOKE.OpalStatusRegistration_status_set(swigCPtr, (int)value);
    } 
    get {
      OpalRegistrationStates ret = (OpalRegistrationStates)OPALPINVOKE.OpalStatusRegistration_status_get(swigCPtr);
      return ret;
    } 
  }

  public OpalProductDescription product {
    set {
      OPALPINVOKE.OpalStatusRegistration_product_set(swigCPtr, OpalProductDescription.getCPtr(value));
    } 
    get {
      IntPtr cPtr = OPALPINVOKE.OpalStatusRegistration_product_get(swigCPtr);
      OpalProductDescription ret = (cPtr == IntPtr.Zero) ? null : new OpalProductDescription(cPtr, false);
      return ret;
    } 
  }

  public OpalStatusRegistration() : this(OPALPINVOKE.new_OpalStatusRegistration(), true) {
  }

}
