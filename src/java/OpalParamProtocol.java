/* ----------------------------------------------------------------------------
 * This file was automatically generated by SWIG (http://www.swig.org).
 * Version 1.3.35
 *
 * Do not make changes to this file unless you know what you are doing--modify
 * the SWIG interface file instead.
 * ----------------------------------------------------------------------------- */

package org.opalvoip;

public class OpalParamProtocol {
  private long swigCPtr;
  protected boolean swigCMemOwn;

  protected OpalParamProtocol(long cPtr, boolean cMemoryOwn) {
    swigCMemOwn = cMemoryOwn;
    swigCPtr = cPtr;
  }

  protected static long getCPtr(OpalParamProtocol obj) {
    return (obj == null) ? 0 : obj.swigCPtr;
  }

  protected void finalize() {
    delete();
  }

  public synchronized void delete() {
    if(swigCPtr != 0 && swigCMemOwn) {
      swigCMemOwn = false;
      exampleJNI.delete_OpalParamProtocol(swigCPtr);
    }
    swigCPtr = 0;
  }

  public void setM_prefix(String value) {
    exampleJNI.OpalParamProtocol_m_prefix_set(swigCPtr, this, value);
  }

  public String getM_prefix() {
    return exampleJNI.OpalParamProtocol_m_prefix_get(swigCPtr, this);
  }

  public void setM_userName(String value) {
    exampleJNI.OpalParamProtocol_m_userName_set(swigCPtr, this, value);
  }

  public String getM_userName() {
    return exampleJNI.OpalParamProtocol_m_userName_get(swigCPtr, this);
  }

  public void setM_displayName(String value) {
    exampleJNI.OpalParamProtocol_m_displayName_set(swigCPtr, this, value);
  }

  public String getM_displayName() {
    return exampleJNI.OpalParamProtocol_m_displayName_get(swigCPtr, this);
  }

  public void setM_product(OpalProductDescription value) {
    exampleJNI.OpalParamProtocol_m_product_set(swigCPtr, this, OpalProductDescription.getCPtr(value), value);
  }

  public OpalProductDescription getM_product() {
    long cPtr = exampleJNI.OpalParamProtocol_m_product_get(swigCPtr, this);
    return (cPtr == 0) ? null : new OpalProductDescription(cPtr, false);
  }

  public void setM_interfaceAddresses(String value) {
    exampleJNI.OpalParamProtocol_m_interfaceAddresses_set(swigCPtr, this, value);
  }

  public String getM_interfaceAddresses() {
    return exampleJNI.OpalParamProtocol_m_interfaceAddresses_get(swigCPtr, this);
  }

  public OpalParamProtocol() {
    this(exampleJNI.new_OpalParamProtocol(), true);
  }

}
