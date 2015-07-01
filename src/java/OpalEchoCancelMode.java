/* ----------------------------------------------------------------------------
 * This file was automatically generated by SWIG (http://www.swig.org).
 * Version 3.0.5
 *
 * Do not make changes to this file unless you know what you are doing--modify
 * the SWIG interface file instead.
 * ----------------------------------------------------------------------------- */

package org.opalvoip.opal;

public final class OpalEchoCancelMode {
  public final static OpalEchoCancelMode OpalEchoCancelNoChange = new OpalEchoCancelMode("OpalEchoCancelNoChange");
  public final static OpalEchoCancelMode OpalEchoCancelDisabled = new OpalEchoCancelMode("OpalEchoCancelDisabled");
  public final static OpalEchoCancelMode OpalEchoCancelEnabled = new OpalEchoCancelMode("OpalEchoCancelEnabled");

  public final int swigValue() {
    return swigValue;
  }

  public String toString() {
    return swigName;
  }

  public static OpalEchoCancelMode swigToEnum(int swigValue) {
    if (swigValue < swigValues.length && swigValue >= 0 && swigValues[swigValue].swigValue == swigValue)
      return swigValues[swigValue];
    for (int i = 0; i < swigValues.length; i++)
      if (swigValues[i].swigValue == swigValue)
        return swigValues[i];
    throw new IllegalArgumentException("No enum " + OpalEchoCancelMode.class + " with value " + swigValue);
  }

  private OpalEchoCancelMode(String swigName) {
    this.swigName = swigName;
    this.swigValue = swigNext++;
  }

  private OpalEchoCancelMode(String swigName, int swigValue) {
    this.swigName = swigName;
    this.swigValue = swigValue;
    swigNext = swigValue+1;
  }

  private OpalEchoCancelMode(String swigName, OpalEchoCancelMode swigEnum) {
    this.swigName = swigName;
    this.swigValue = swigEnum.swigValue;
    swigNext = this.swigValue+1;
  }

  private static OpalEchoCancelMode[] swigValues = { OpalEchoCancelNoChange, OpalEchoCancelDisabled, OpalEchoCancelEnabled };
  private static int swigNext = 0;
  private final int swigValue;
  private final String swigName;
}

