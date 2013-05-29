/* ----------------------------------------------------------------------------
 * This file was automatically generated by SWIG (http://www.swig.org).
 * Version 2.0.9
 *
 * Do not make changes to this file unless you know what you are doing--modify
 * the SWIG interface file instead.
 * ----------------------------------------------------------------------------- */

package org.opalvoip.opal;

public class OpalParamSetUserData {
  private long swigCPtr;
  protected boolean swigCMemOwn;

  protected OpalParamSetUserData(long cPtr, boolean cMemoryOwn) {
    swigCMemOwn = cMemoryOwn;
    swigCPtr = cPtr;
  }

  protected static long getCPtr(OpalParamSetUserData obj) {
    return (obj == null) ? 0 : obj.swigCPtr;
  }

  protected void finalize() {
    delete();
  }

  public synchronized void delete() {
    if (swigCPtr != 0) {
      if (swigCMemOwn) {
        swigCMemOwn = false;
        OPALJNI.delete_OpalParamSetUserData(swigCPtr);
      }
      swigCPtr = 0;
    }
  }

  public void setCallToken(String value) {
    OPALJNI.OpalParamSetUserData_callToken_set(swigCPtr, this, value);
  }

  public String getCallToken() {
    return OPALJNI.OpalParamSetUserData_callToken_get(swigCPtr, this);
  }

  public void setUserData(SWIGTYPE_p_void value) {
    OPALJNI.OpalParamSetUserData_userData_set(swigCPtr, this, SWIGTYPE_p_void.getCPtr(value));
  }

  public SWIGTYPE_p_void getUserData() {
    long cPtr = OPALJNI.OpalParamSetUserData_userData_get(swigCPtr, this);
    return (cPtr == 0) ? null : new SWIGTYPE_p_void(cPtr, false);
  }

  public OpalParamSetUserData() {
    this(OPALJNI.new_OpalParamSetUserData(), true);
  }

}
