/* ----------------------------------------------------------------------------
 * This file was automatically generated by SWIG (http://www.swig.org).
 * Version 2.0.9
 *
 * Do not make changes to this file unless you know what you are doing--modify
 * the SWIG interface file instead.
 * ----------------------------------------------------------------------------- */

package org.opalvoip.opal;

public class OpalStatusTransferCall {
  private long swigCPtr;
  protected boolean swigCMemOwn;

  protected OpalStatusTransferCall(long cPtr, boolean cMemoryOwn) {
    swigCMemOwn = cMemoryOwn;
    swigCPtr = cPtr;
  }

  protected static long getCPtr(OpalStatusTransferCall obj) {
    return (obj == null) ? 0 : obj.swigCPtr;
  }

  protected void finalize() {
    delete();
  }

  public synchronized void delete() {
    if (swigCPtr != 0) {
      if (swigCMemOwn) {
        swigCMemOwn = false;
        OPALJNI.delete_OpalStatusTransferCall(swigCPtr);
      }
      swigCPtr = 0;
    }
  }

  public void setCallToken(String value) {
    OPALJNI.OpalStatusTransferCall_callToken_set(swigCPtr, this, value);
  }

  public String getCallToken() {
    return OPALJNI.OpalStatusTransferCall_callToken_get(swigCPtr, this);
  }

  public void setProtocolCallId(String value) {
    OPALJNI.OpalStatusTransferCall_protocolCallId_set(swigCPtr, this, value);
  }

  public String getProtocolCallId() {
    return OPALJNI.OpalStatusTransferCall_protocolCallId_get(swigCPtr, this);
  }

  public void setResult(String value) {
    OPALJNI.OpalStatusTransferCall_result_set(swigCPtr, this, value);
  }

  public String getResult() {
    return OPALJNI.OpalStatusTransferCall_result_get(swigCPtr, this);
  }

  public void setInfo(String value) {
    OPALJNI.OpalStatusTransferCall_info_set(swigCPtr, this, value);
  }

  public String getInfo() {
    return OPALJNI.OpalStatusTransferCall_info_get(swigCPtr, this);
  }

  public OpalStatusTransferCall() {
    this(OPALJNI.new_OpalStatusTransferCall(), true);
  }

}
