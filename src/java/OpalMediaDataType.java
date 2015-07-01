/* ----------------------------------------------------------------------------
 * This file was automatically generated by SWIG (http://www.swig.org).
 * Version 3.0.5
 *
 * Do not make changes to this file unless you know what you are doing--modify
 * the SWIG interface file instead.
 * ----------------------------------------------------------------------------- */

package org.opalvoip.opal;

public final class OpalMediaDataType {
  public final static OpalMediaDataType OpalMediaDataNoChange = new OpalMediaDataType("OpalMediaDataNoChange");
  public final static OpalMediaDataType OpalMediaDataPayloadOnly = new OpalMediaDataType("OpalMediaDataPayloadOnly");
  public final static OpalMediaDataType OpalMediaDataWithHeader = new OpalMediaDataType("OpalMediaDataWithHeader");

  public final int swigValue() {
    return swigValue;
  }

  public String toString() {
    return swigName;
  }

  public static OpalMediaDataType swigToEnum(int swigValue) {
    if (swigValue < swigValues.length && swigValue >= 0 && swigValues[swigValue].swigValue == swigValue)
      return swigValues[swigValue];
    for (int i = 0; i < swigValues.length; i++)
      if (swigValues[i].swigValue == swigValue)
        return swigValues[i];
    throw new IllegalArgumentException("No enum " + OpalMediaDataType.class + " with value " + swigValue);
  }

  private OpalMediaDataType(String swigName) {
    this.swigName = swigName;
    this.swigValue = swigNext++;
  }

  private OpalMediaDataType(String swigName, int swigValue) {
    this.swigName = swigName;
    this.swigValue = swigValue;
    swigNext = swigValue+1;
  }

  private OpalMediaDataType(String swigName, OpalMediaDataType swigEnum) {
    this.swigName = swigName;
    this.swigValue = swigEnum.swigValue;
    swigNext = this.swigValue+1;
  }

  private static OpalMediaDataType[] swigValues = { OpalMediaDataNoChange, OpalMediaDataPayloadOnly, OpalMediaDataWithHeader };
  private static int swigNext = 0;
  private final int swigValue;
  private final String swigName;
}

