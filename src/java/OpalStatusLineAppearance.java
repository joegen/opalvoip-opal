/* ----------------------------------------------------------------------------
 * This file was automatically generated by SWIG (http://www.swig.org).
 * Version 2.0.9
 *
 * Do not make changes to this file unless you know what you are doing--modify
 * the SWIG interface file instead.
 * ----------------------------------------------------------------------------- */

package org.opalvoip.opal;

public class OpalStatusLineAppearance {
  private long swigCPtr;
  protected boolean swigCMemOwn;

  protected OpalStatusLineAppearance(long cPtr, boolean cMemoryOwn) {
    swigCMemOwn = cMemoryOwn;
    swigCPtr = cPtr;
  }

  protected static long getCPtr(OpalStatusLineAppearance obj) {
    return (obj == null) ? 0 : obj.swigCPtr;
  }

  protected void finalize() {
    delete();
  }

  public synchronized void delete() {
    if (swigCPtr != 0) {
      if (swigCMemOwn) {
        swigCMemOwn = false;
        OPALJNI.delete_OpalStatusLineAppearance(swigCPtr);
      }
      swigCPtr = 0;
    }
  }

  public void setLine(String value) {
    OPALJNI.OpalStatusLineAppearance_line_set(swigCPtr, this, value);
  }

  public String getLine() {
    return OPALJNI.OpalStatusLineAppearance_line_get(swigCPtr, this);
  }

  public void setState(OpalLineAppearanceStates value) {
    OPALJNI.OpalStatusLineAppearance_state_set(swigCPtr, this, value.swigValue());
  }

  public OpalLineAppearanceStates getState() {
    return OpalLineAppearanceStates.swigToEnum(OPALJNI.OpalStatusLineAppearance_state_get(swigCPtr, this));
  }

  public void setAppearance(int value) {
    OPALJNI.OpalStatusLineAppearance_appearance_set(swigCPtr, this, value);
  }

  public int getAppearance() {
    return OPALJNI.OpalStatusLineAppearance_appearance_get(swigCPtr, this);
  }

  public void setCallId(String value) {
    OPALJNI.OpalStatusLineAppearance_callId_set(swigCPtr, this, value);
  }

  public String getCallId() {
    return OPALJNI.OpalStatusLineAppearance_callId_get(swigCPtr, this);
  }

  public void setPartyA(String value) {
    OPALJNI.OpalStatusLineAppearance_partyA_set(swigCPtr, this, value);
  }

  public String getPartyA() {
    return OPALJNI.OpalStatusLineAppearance_partyA_get(swigCPtr, this);
  }

  public void setPartyB(String value) {
    OPALJNI.OpalStatusLineAppearance_partyB_set(swigCPtr, this, value);
  }

  public String getPartyB() {
    return OPALJNI.OpalStatusLineAppearance_partyB_get(swigCPtr, this);
  }

  public OpalStatusLineAppearance() {
    this(OPALJNI.new_OpalStatusLineAppearance(), true);
  }

}
