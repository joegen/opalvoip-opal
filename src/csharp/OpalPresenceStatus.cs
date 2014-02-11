/* ----------------------------------------------------------------------------
 * This file was automatically generated by SWIG (http://www.swig.org).
 * Version 2.0.9
 *
 * Do not make changes to this file unless you know what you are doing--modify
 * the SWIG interface file instead.
 * ----------------------------------------------------------------------------- */


using System;
using System.Runtime.InteropServices;

public class OpalPresenceStatus : IDisposable {
  private HandleRef swigCPtr;
  protected bool swigCMemOwn;

  internal OpalPresenceStatus(IntPtr cPtr, bool cMemoryOwn) {
    swigCMemOwn = cMemoryOwn;
    swigCPtr = new HandleRef(this, cPtr);
  }

  internal static HandleRef getCPtr(OpalPresenceStatus obj) {
    return (obj == null) ? new HandleRef(null, IntPtr.Zero) : obj.swigCPtr;
  }

  ~OpalPresenceStatus() {
    Dispose();
  }

  public virtual void Dispose() {
    lock(this) {
      if (swigCPtr.Handle != IntPtr.Zero) {
        if (swigCMemOwn) {
          swigCMemOwn = false;
          OPALPINVOKE.delete_OpalPresenceStatus(swigCPtr);
        }
        swigCPtr = new HandleRef(null, IntPtr.Zero);
      }
      GC.SuppressFinalize(this);
    }
  }

  public string entity {
    set {
      OPALPINVOKE.OpalPresenceStatus_entity_set(swigCPtr, value);
    } 
    get {
      string ret = OPALPINVOKE.OpalPresenceStatus_entity_get(swigCPtr);
      return ret;
    } 
  }

  public string target {
    set {
      OPALPINVOKE.OpalPresenceStatus_target_set(swigCPtr, value);
    } 
    get {
      string ret = OPALPINVOKE.OpalPresenceStatus_target_get(swigCPtr);
      return ret;
    } 
  }

  public string service {
    set {
      OPALPINVOKE.OpalPresenceStatus_service_set(swigCPtr, value);
    } 
    get {
      string ret = OPALPINVOKE.OpalPresenceStatus_service_get(swigCPtr);
      return ret;
    } 
  }

  public string contact {
    set {
      OPALPINVOKE.OpalPresenceStatus_contact_set(swigCPtr, value);
    } 
    get {
      string ret = OPALPINVOKE.OpalPresenceStatus_contact_get(swigCPtr);
      return ret;
    } 
  }

  public string capabilities {
    set {
      OPALPINVOKE.OpalPresenceStatus_capabilities_set(swigCPtr, value);
    } 
    get {
      string ret = OPALPINVOKE.OpalPresenceStatus_capabilities_get(swigCPtr);
      return ret;
    } 
  }

  public OpalPresenceStates state {
    set {
      OPALPINVOKE.OpalPresenceStatus_state_set(swigCPtr, (int)value);
    } 
    get {
      OpalPresenceStates ret = (OpalPresenceStates)OPALPINVOKE.OpalPresenceStatus_state_get(swigCPtr);
      return ret;
    } 
  }

  public string activities {
    set {
      OPALPINVOKE.OpalPresenceStatus_activities_set(swigCPtr, value);
    } 
    get {
      string ret = OPALPINVOKE.OpalPresenceStatus_activities_get(swigCPtr);
      return ret;
    } 
  }

  public string note {
    set {
      OPALPINVOKE.OpalPresenceStatus_note_set(swigCPtr, value);
    } 
    get {
      string ret = OPALPINVOKE.OpalPresenceStatus_note_get(swigCPtr);
      return ret;
    } 
  }

  public string infoType {
    set {
      OPALPINVOKE.OpalPresenceStatus_infoType_set(swigCPtr, value);
    } 
    get {
      string ret = OPALPINVOKE.OpalPresenceStatus_infoType_get(swigCPtr);
      return ret;
    } 
  }

  public string infoData {
    set {
      OPALPINVOKE.OpalPresenceStatus_infoData_set(swigCPtr, value);
    } 
    get {
      string ret = OPALPINVOKE.OpalPresenceStatus_infoData_get(swigCPtr);
      return ret;
    } 
  }

  public OpalPresenceStatus() : this(OPALPINVOKE.new_OpalPresenceStatus(), true) {
  }

}
