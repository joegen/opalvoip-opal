/* ----------------------------------------------------------------------------
 * This file was automatically generated by SWIG (http://www.swig.org).
 * Version 3.0.5
 *
 * Do not make changes to this file unless you know what you are doing--modify
 * the SWIG interface file instead.
 * ----------------------------------------------------------------------------- */

package org.opalvoip.opal;

public class OpalStatusCallCleared {
  private long swigCPtr;
  protected boolean swigCMemOwn;

  protected OpalStatusCallCleared(long cPtr, boolean cMemoryOwn) {
    swigCMemOwn = cMemoryOwn;
    swigCPtr = cPtr;
  }

  protected static long getCPtr(OpalStatusCallCleared obj) {
    return (obj == null) ? 0 : obj.swigCPtr;
  }

  protected void finalize() {
    delete();
  }

  public synchronized void delete() {
    if (swigCPtr != 0) {
      if (swigCMemOwn) {
        swigCMemOwn = false;
        OPALJNI.delete_OpalStatusCallCleared(swigCPtr);
      }
      swigCPtr = 0;
    }
  }

  public void setCallToken(String value) {
    OPALJNI.OpalStatusCallCleared_callToken_set(swigCPtr, this, value);
  }

  public String getCallToken() {
    return OPALJNI.OpalStatusCallCleared_callToken_get(swigCPtr, this);
  }

  public void setReason(String value) {
    OPALJNI.OpalStatusCallCleared_reason_set(swigCPtr, this, value);
  }

  public String getReason() {
    return OPALJNI.OpalStatusCallCleared_reason_get(swigCPtr, this);
  }

  public OpalStatusCallCleared() {
    this(OPALJNI.new_OpalStatusCallCleared(), true);
  }

}
