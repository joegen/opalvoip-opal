/* ----------------------------------------------------------------------------
 * This file was automatically generated by SWIG (http://www.swig.org).
 * Version 2.0.9
 *
 * Do not make changes to this file unless you know what you are doing--modify
 * the SWIG interface file instead.
 * ----------------------------------------------------------------------------- */

package org.opalvoip.opal;

public class OpalPresenceStatus {
  private long swigCPtr;
  protected boolean swigCMemOwn;

  protected OpalPresenceStatus(long cPtr, boolean cMemoryOwn) {
    swigCMemOwn = cMemoryOwn;
    swigCPtr = cPtr;
  }

  protected static long getCPtr(OpalPresenceStatus obj) {
    return (obj == null) ? 0 : obj.swigCPtr;
  }

  protected void finalize() {
    delete();
  }

  public synchronized void delete() {
    if (swigCPtr != 0) {
      if (swigCMemOwn) {
        swigCMemOwn = false;
        OPALJNI.delete_OpalPresenceStatus(swigCPtr);
      }
      swigCPtr = 0;
    }
  }

  public void setEntity(String value) {
    OPALJNI.OpalPresenceStatus_entity_set(swigCPtr, this, value);
  }

  public String getEntity() {
    return OPALJNI.OpalPresenceStatus_entity_get(swigCPtr, this);
  }

  public void setTarget(String value) {
    OPALJNI.OpalPresenceStatus_target_set(swigCPtr, this, value);
  }

  public String getTarget() {
    return OPALJNI.OpalPresenceStatus_target_get(swigCPtr, this);
  }

  public void setService(String value) {
    OPALJNI.OpalPresenceStatus_service_set(swigCPtr, this, value);
  }

  public String getService() {
    return OPALJNI.OpalPresenceStatus_service_get(swigCPtr, this);
  }

  public void setContact(String value) {
    OPALJNI.OpalPresenceStatus_contact_set(swigCPtr, this, value);
  }

  public String getContact() {
    return OPALJNI.OpalPresenceStatus_contact_get(swigCPtr, this);
  }

  public void setCapabilities(String value) {
    OPALJNI.OpalPresenceStatus_capabilities_set(swigCPtr, this, value);
  }

  public String getCapabilities() {
    return OPALJNI.OpalPresenceStatus_capabilities_get(swigCPtr, this);
  }

  public void setState(OpalPresenceStates value) {
    OPALJNI.OpalPresenceStatus_state_set(swigCPtr, this, value.swigValue());
  }

  public OpalPresenceStates getState() {
    return OpalPresenceStates.swigToEnum(OPALJNI.OpalPresenceStatus_state_get(swigCPtr, this));
  }

  public void setActivities(String value) {
    OPALJNI.OpalPresenceStatus_activities_set(swigCPtr, this, value);
  }

  public String getActivities() {
    return OPALJNI.OpalPresenceStatus_activities_get(swigCPtr, this);
  }

  public void setNote(String value) {
    OPALJNI.OpalPresenceStatus_note_set(swigCPtr, this, value);
  }

  public String getNote() {
    return OPALJNI.OpalPresenceStatus_note_get(swigCPtr, this);
  }

  public void setInfoType(String value) {
    OPALJNI.OpalPresenceStatus_infoType_set(swigCPtr, this, value);
  }

  public String getInfoType() {
    return OPALJNI.OpalPresenceStatus_infoType_get(swigCPtr, this);
  }

  public void setInfoData(String value) {
    OPALJNI.OpalPresenceStatus_infoData_set(swigCPtr, this, value);
  }

  public String getInfoData() {
    return OPALJNI.OpalPresenceStatus_infoData_get(swigCPtr, this);
  }

  public OpalPresenceStatus() {
    this(OPALJNI.new_OpalPresenceStatus(), true);
  }

}
