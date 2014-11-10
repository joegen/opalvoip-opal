/* ----------------------------------------------------------------------------
 * This file was automatically generated by SWIG (http://www.swig.org).
 * Version 2.0.9
 *
 * Do not make changes to this file unless you know what you are doing--modify
 * the SWIG interface file instead.
 * ----------------------------------------------------------------------------- */


using System;
using System.Runtime.InteropServices;

public class OpalStatusUserInput : IDisposable {
  private HandleRef swigCPtr;
  protected bool swigCMemOwn;

  internal OpalStatusUserInput(IntPtr cPtr, bool cMemoryOwn) {
    swigCMemOwn = cMemoryOwn;
    swigCPtr = new HandleRef(this, cPtr);
  }

  internal static HandleRef getCPtr(OpalStatusUserInput obj) {
    return (obj == null) ? new HandleRef(null, IntPtr.Zero) : obj.swigCPtr;
  }

  ~OpalStatusUserInput() {
    Dispose();
  }

  public virtual void Dispose() {
    lock(this) {
      if (swigCPtr.Handle != IntPtr.Zero) {
        if (swigCMemOwn) {
          swigCMemOwn = false;
          OPALPINVOKE.delete_OpalStatusUserInput(swigCPtr);
        }
        swigCPtr = new HandleRef(null, IntPtr.Zero);
      }
      GC.SuppressFinalize(this);
    }
  }

  public string callToken {
    set {
      OPALPINVOKE.OpalStatusUserInput_callToken_set(swigCPtr, value);
    } 
    get {
      string ret = OPALPINVOKE.OpalStatusUserInput_callToken_get(swigCPtr);
      return ret;
    } 
  }

  public string userInput {
    set {
      OPALPINVOKE.OpalStatusUserInput_userInput_set(swigCPtr, value);
    } 
    get {
      string ret = OPALPINVOKE.OpalStatusUserInput_userInput_get(swigCPtr);
      return ret;
    } 
  }

  public uint duration {
    set {
      OPALPINVOKE.OpalStatusUserInput_duration_set(swigCPtr, value);
    } 
    get {
      uint ret = OPALPINVOKE.OpalStatusUserInput_duration_get(swigCPtr);
      return ret;
    } 
  }

  public OpalStatusUserInput() : this(OPALPINVOKE.new_OpalStatusUserInput(), true) {
  }

}
