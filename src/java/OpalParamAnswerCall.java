/* ----------------------------------------------------------------------------
 * This file was automatically generated by SWIG (http://www.swig.org).
 * Version 2.0.9
 *
 * Do not make changes to this file unless you know what you are doing--modify
 * the SWIG interface file instead.
 * ----------------------------------------------------------------------------- */

package org.opalvoip.opal;

public class OpalParamAnswerCall {
  private long swigCPtr;
  protected boolean swigCMemOwn;

  protected OpalParamAnswerCall(long cPtr, boolean cMemoryOwn) {
    swigCMemOwn = cMemoryOwn;
    swigCPtr = cPtr;
  }

  protected static long getCPtr(OpalParamAnswerCall obj) {
    return (obj == null) ? 0 : obj.swigCPtr;
  }

  protected void finalize() {
    delete();
  }

  public synchronized void delete() {
    if (swigCPtr != 0) {
      if (swigCMemOwn) {
        swigCMemOwn = false;
        OPALJNI.delete_OpalParamAnswerCall(swigCPtr);
      }
      swigCPtr = 0;
    }
  }

  public void setCallToken(String value) {
    OPALJNI.OpalParamAnswerCall_callToken_set(swigCPtr, this, value);
  }

  public String getCallToken() {
    return OPALJNI.OpalParamAnswerCall_callToken_get(swigCPtr, this);
  }

  public void setOverrides(OpalParamProtocol value) {
    OPALJNI.OpalParamAnswerCall_overrides_set(swigCPtr, this, OpalParamProtocol.getCPtr(value), value);
  }

  public OpalParamProtocol getOverrides() {
    long cPtr = OPALJNI.OpalParamAnswerCall_overrides_get(swigCPtr, this);
    return (cPtr == 0) ? null : new OpalParamProtocol(cPtr, false);
  }

  public void setWithMedia(long value) {
    OPALJNI.OpalParamAnswerCall_withMedia_set(swigCPtr, this, value);
  }

  public long getWithMedia() {
    return OPALJNI.OpalParamAnswerCall_withMedia_get(swigCPtr, this);
  }

  public OpalParamAnswerCall() {
    this(OPALJNI.new_OpalParamAnswerCall(), true);
  }

}
