/* ----------------------------------------------------------------------------
 * This file was automatically generated by SWIG (http://www.swig.org).
 * Version 2.0.9
 *
 * Do not make changes to this file unless you know what you are doing--modify
 * the SWIG interface file instead.
 * ----------------------------------------------------------------------------- */

package org.opalvoip.opal;

public final class OpalPresenceStates {
  public final static OpalPresenceStates OpalPresenceAuthRequest = new OpalPresenceStates("OpalPresenceAuthRequest", OPALJNI.OpalPresenceAuthRequest_get());
  public final static OpalPresenceStates OpalPresenceError = new OpalPresenceStates("OpalPresenceError", OPALJNI.OpalPresenceError_get());
  public final static OpalPresenceStates OpalPresenceForbidden = new OpalPresenceStates("OpalPresenceForbidden", OPALJNI.OpalPresenceForbidden_get());
  public final static OpalPresenceStates OpalPresenceNone = new OpalPresenceStates("OpalPresenceNone", OPALJNI.OpalPresenceNone_get());
  public final static OpalPresenceStates OpalPresenceUnchanged = new OpalPresenceStates("OpalPresenceUnchanged", OPALJNI.OpalPresenceUnchanged_get());
  public final static OpalPresenceStates OpalPresenceAvailable = new OpalPresenceStates("OpalPresenceAvailable");
  public final static OpalPresenceStates OpalPresenceUnavailable = new OpalPresenceStates("OpalPresenceUnavailable");

  public final int swigValue() {
    return swigValue;
  }

  public String toString() {
    return swigName;
  }

  public static OpalPresenceStates swigToEnum(int swigValue) {
    if (swigValue < swigValues.length && swigValue >= 0 && swigValues[swigValue].swigValue == swigValue)
      return swigValues[swigValue];
    for (int i = 0; i < swigValues.length; i++)
      if (swigValues[i].swigValue == swigValue)
        return swigValues[i];
    throw new IllegalArgumentException("No enum " + OpalPresenceStates.class + " with value " + swigValue);
  }

  private OpalPresenceStates(String swigName) {
    this.swigName = swigName;
    this.swigValue = swigNext++;
  }

  private OpalPresenceStates(String swigName, int swigValue) {
    this.swigName = swigName;
    this.swigValue = swigValue;
    swigNext = swigValue+1;
  }

  private OpalPresenceStates(String swigName, OpalPresenceStates swigEnum) {
    this.swigName = swigName;
    this.swigValue = swigEnum.swigValue;
    swigNext = this.swigValue+1;
  }

  private static OpalPresenceStates[] swigValues = { OpalPresenceAuthRequest, OpalPresenceError, OpalPresenceForbidden, OpalPresenceNone, OpalPresenceUnchanged, OpalPresenceAvailable, OpalPresenceUnavailable };
  private static int swigNext = 0;
  private final int swigValue;
  private final String swigName;
}

