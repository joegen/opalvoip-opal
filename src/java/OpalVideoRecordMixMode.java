/* ----------------------------------------------------------------------------
 * This file was automatically generated by SWIG (http://www.swig.org).
 * Version 1.3.40
 *
 * Do not make changes to this file unless you know what you are doing--modify
 * the SWIG interface file instead.
 * ----------------------------------------------------------------------------- */

package org.opalvoip;

public final class OpalVideoRecordMixMode {
  public final static OpalVideoRecordMixMode OpalSideBySideLetterbox = new OpalVideoRecordMixMode("OpalSideBySideLetterbox");
  public final static OpalVideoRecordMixMode OpalSideBySideScaled = new OpalVideoRecordMixMode("OpalSideBySideScaled");
  public final static OpalVideoRecordMixMode OpalStackedPillarbox = new OpalVideoRecordMixMode("OpalStackedPillarbox");
  public final static OpalVideoRecordMixMode OpalStackedScaled = new OpalVideoRecordMixMode("OpalStackedScaled");

  public final int swigValue() {
    return swigValue;
  }

  public String toString() {
    return swigName;
  }

  public static OpalVideoRecordMixMode swigToEnum(int swigValue) {
    if (swigValue < swigValues.length && swigValue >= 0 && swigValues[swigValue].swigValue == swigValue)
      return swigValues[swigValue];
    for (int i = 0; i < swigValues.length; i++)
      if (swigValues[i].swigValue == swigValue)
        return swigValues[i];
    throw new IllegalArgumentException("No enum " + OpalVideoRecordMixMode.class + " with value " + swigValue);
  }

  private OpalVideoRecordMixMode(String swigName) {
    this.swigName = swigName;
    this.swigValue = swigNext++;
  }

  private OpalVideoRecordMixMode(String swigName, int swigValue) {
    this.swigName = swigName;
    this.swigValue = swigValue;
    swigNext = swigValue+1;
  }

  private OpalVideoRecordMixMode(String swigName, OpalVideoRecordMixMode swigEnum) {
    this.swigName = swigName;
    this.swigValue = swigEnum.swigValue;
    swigNext = this.swigValue+1;
  }

  private static OpalVideoRecordMixMode[] swigValues = { OpalSideBySideLetterbox, OpalSideBySideScaled, OpalStackedPillarbox, OpalStackedScaled };
  private static int swigNext = 0;
  private final int swigValue;
  private final String swigName;
}

