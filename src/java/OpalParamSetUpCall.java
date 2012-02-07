/* ----------------------------------------------------------------------------
 * This file was automatically generated by SWIG (http://www.swig.org).
 * Version 1.3.40
 *
 * Do not make changes to this file unless you know what you are doing--modify
 * the SWIG interface file instead.
 * ----------------------------------------------------------------------------- */

package org.opalvoip;

public class OpalParamSetUpCall {
  private long swigCPtr;
  protected boolean swigCMemOwn;

  protected OpalParamSetUpCall(long cPtr, boolean cMemoryOwn) {
    swigCMemOwn = cMemoryOwn;
    swigCPtr = cPtr;
  }

  protected static long getCPtr(OpalParamSetUpCall obj) {
    return (obj == null) ? 0 : obj.swigCPtr;
  }

  protected void finalize() {
    delete();
  }

  public synchronized void delete() {
    if (swigCPtr != 0) {
      if (swigCMemOwn) {
        swigCMemOwn = false;
        OPALJNI.delete_OpalParamSetUpCall(swigCPtr);
      }
      swigCPtr = 0;
    }
  }

  public void setM_partyA(String value) {
    OPALJNI.OpalParamSetUpCall_m_partyA_set(swigCPtr, this, value);
  }

  public String getM_partyA() {
    return OPALJNI.OpalParamSetUpCall_m_partyA_get(swigCPtr, this);
  }

  public void setM_partyB(String value) {
    OPALJNI.OpalParamSetUpCall_m_partyB_set(swigCPtr, this, value);
  }

  public String getM_partyB() {
    return OPALJNI.OpalParamSetUpCall_m_partyB_get(swigCPtr, this);
  }

  public void setM_callToken(String value) {
    OPALJNI.OpalParamSetUpCall_m_callToken_set(swigCPtr, this, value);
  }

  public String getM_callToken() {
    return OPALJNI.OpalParamSetUpCall_m_callToken_get(swigCPtr, this);
  }

  public void setM_alertingType(String value) {
    OPALJNI.OpalParamSetUpCall_m_alertingType_set(swigCPtr, this, value);
  }

  public String getM_alertingType() {
    return OPALJNI.OpalParamSetUpCall_m_alertingType_get(swigCPtr, this);
  }

  public void setM_protocolCallId(String value) {
    OPALJNI.OpalParamSetUpCall_m_protocolCallId_set(swigCPtr, this, value);
  }

  public String getM_protocolCallId() {
    return OPALJNI.OpalParamSetUpCall_m_protocolCallId_get(swigCPtr, this);
  }

  public void setM_overrides(OpalParamProtocol value) {
    OPALJNI.OpalParamSetUpCall_m_overrides_set(swigCPtr, this, OpalParamProtocol.getCPtr(value), value);
  }

  public OpalParamProtocol getM_overrides() {
    long cPtr = OPALJNI.OpalParamSetUpCall_m_overrides_get(swigCPtr, this);
    return (cPtr == 0) ? null : new OpalParamProtocol(cPtr, false);
  }

  public OpalParamSetUpCall() {
    this(OPALJNI.new_OpalParamSetUpCall(), true);
  }

}
