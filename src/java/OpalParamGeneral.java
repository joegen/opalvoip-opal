/* ----------------------------------------------------------------------------
 * This file was automatically generated by SWIG (http://www.swig.org).
 * Version 1.3.40
 *
 * Do not make changes to this file unless you know what you are doing--modify
 * the SWIG interface file instead.
 * ----------------------------------------------------------------------------- */

package org.opalvoip;

public class OpalParamGeneral {
  private long swigCPtr;
  protected boolean swigCMemOwn;

  protected OpalParamGeneral(long cPtr, boolean cMemoryOwn) {
    swigCMemOwn = cMemoryOwn;
    swigCPtr = cPtr;
  }

  protected static long getCPtr(OpalParamGeneral obj) {
    return (obj == null) ? 0 : obj.swigCPtr;
  }

  protected void finalize() {
    delete();
  }

  public synchronized void delete() {
    if (swigCPtr != 0) {
      if (swigCMemOwn) {
        swigCMemOwn = false;
        OPALJNI.delete_OpalParamGeneral(swigCPtr);
      }
      swigCPtr = 0;
    }
  }

  public void setM_audioRecordDevice(String value) {
    OPALJNI.OpalParamGeneral_m_audioRecordDevice_set(swigCPtr, this, value);
  }

  public String getM_audioRecordDevice() {
    return OPALJNI.OpalParamGeneral_m_audioRecordDevice_get(swigCPtr, this);
  }

  public void setM_audioPlayerDevice(String value) {
    OPALJNI.OpalParamGeneral_m_audioPlayerDevice_set(swigCPtr, this, value);
  }

  public String getM_audioPlayerDevice() {
    return OPALJNI.OpalParamGeneral_m_audioPlayerDevice_get(swigCPtr, this);
  }

  public void setM_videoInputDevice(String value) {
    OPALJNI.OpalParamGeneral_m_videoInputDevice_set(swigCPtr, this, value);
  }

  public String getM_videoInputDevice() {
    return OPALJNI.OpalParamGeneral_m_videoInputDevice_get(swigCPtr, this);
  }

  public void setM_videoOutputDevice(String value) {
    OPALJNI.OpalParamGeneral_m_videoOutputDevice_set(swigCPtr, this, value);
  }

  public String getM_videoOutputDevice() {
    return OPALJNI.OpalParamGeneral_m_videoOutputDevice_get(swigCPtr, this);
  }

  public void setM_videoPreviewDevice(String value) {
    OPALJNI.OpalParamGeneral_m_videoPreviewDevice_set(swigCPtr, this, value);
  }

  public String getM_videoPreviewDevice() {
    return OPALJNI.OpalParamGeneral_m_videoPreviewDevice_get(swigCPtr, this);
  }

  public void setM_mediaOrder(String value) {
    OPALJNI.OpalParamGeneral_m_mediaOrder_set(swigCPtr, this, value);
  }

  public String getM_mediaOrder() {
    return OPALJNI.OpalParamGeneral_m_mediaOrder_get(swigCPtr, this);
  }

  public void setM_mediaMask(String value) {
    OPALJNI.OpalParamGeneral_m_mediaMask_set(swigCPtr, this, value);
  }

  public String getM_mediaMask() {
    return OPALJNI.OpalParamGeneral_m_mediaMask_get(swigCPtr, this);
  }

  public void setM_autoRxMedia(String value) {
    OPALJNI.OpalParamGeneral_m_autoRxMedia_set(swigCPtr, this, value);
  }

  public String getM_autoRxMedia() {
    return OPALJNI.OpalParamGeneral_m_autoRxMedia_get(swigCPtr, this);
  }

  public void setM_autoTxMedia(String value) {
    OPALJNI.OpalParamGeneral_m_autoTxMedia_set(swigCPtr, this, value);
  }

  public String getM_autoTxMedia() {
    return OPALJNI.OpalParamGeneral_m_autoTxMedia_get(swigCPtr, this);
  }

  public void setM_natRouter(String value) {
    OPALJNI.OpalParamGeneral_m_natRouter_set(swigCPtr, this, value);
  }

  public String getM_natRouter() {
    return OPALJNI.OpalParamGeneral_m_natRouter_get(swigCPtr, this);
  }

  public void setM_stunServer(String value) {
    OPALJNI.OpalParamGeneral_m_stunServer_set(swigCPtr, this, value);
  }

  public String getM_stunServer() {
    return OPALJNI.OpalParamGeneral_m_stunServer_get(swigCPtr, this);
  }

  public void setM_tcpPortBase(long value) {
    OPALJNI.OpalParamGeneral_m_tcpPortBase_set(swigCPtr, this, value);
  }

  public long getM_tcpPortBase() {
    return OPALJNI.OpalParamGeneral_m_tcpPortBase_get(swigCPtr, this);
  }

  public void setM_tcpPortMax(long value) {
    OPALJNI.OpalParamGeneral_m_tcpPortMax_set(swigCPtr, this, value);
  }

  public long getM_tcpPortMax() {
    return OPALJNI.OpalParamGeneral_m_tcpPortMax_get(swigCPtr, this);
  }

  public void setM_udpPortBase(long value) {
    OPALJNI.OpalParamGeneral_m_udpPortBase_set(swigCPtr, this, value);
  }

  public long getM_udpPortBase() {
    return OPALJNI.OpalParamGeneral_m_udpPortBase_get(swigCPtr, this);
  }

  public void setM_udpPortMax(long value) {
    OPALJNI.OpalParamGeneral_m_udpPortMax_set(swigCPtr, this, value);
  }

  public long getM_udpPortMax() {
    return OPALJNI.OpalParamGeneral_m_udpPortMax_get(swigCPtr, this);
  }

  public void setM_rtpPortBase(long value) {
    OPALJNI.OpalParamGeneral_m_rtpPortBase_set(swigCPtr, this, value);
  }

  public long getM_rtpPortBase() {
    return OPALJNI.OpalParamGeneral_m_rtpPortBase_get(swigCPtr, this);
  }

  public void setM_rtpPortMax(long value) {
    OPALJNI.OpalParamGeneral_m_rtpPortMax_set(swigCPtr, this, value);
  }

  public long getM_rtpPortMax() {
    return OPALJNI.OpalParamGeneral_m_rtpPortMax_get(swigCPtr, this);
  }

  public void setM_rtpTypeOfService(long value) {
    OPALJNI.OpalParamGeneral_m_rtpTypeOfService_set(swigCPtr, this, value);
  }

  public long getM_rtpTypeOfService() {
    return OPALJNI.OpalParamGeneral_m_rtpTypeOfService_get(swigCPtr, this);
  }

  public void setM_rtpMaxPayloadSize(long value) {
    OPALJNI.OpalParamGeneral_m_rtpMaxPayloadSize_set(swigCPtr, this, value);
  }

  public long getM_rtpMaxPayloadSize() {
    return OPALJNI.OpalParamGeneral_m_rtpMaxPayloadSize_get(swigCPtr, this);
  }

  public void setM_minAudioJitter(long value) {
    OPALJNI.OpalParamGeneral_m_minAudioJitter_set(swigCPtr, this, value);
  }

  public long getM_minAudioJitter() {
    return OPALJNI.OpalParamGeneral_m_minAudioJitter_get(swigCPtr, this);
  }

  public void setM_maxAudioJitter(long value) {
    OPALJNI.OpalParamGeneral_m_maxAudioJitter_set(swigCPtr, this, value);
  }

  public long getM_maxAudioJitter() {
    return OPALJNI.OpalParamGeneral_m_maxAudioJitter_get(swigCPtr, this);
  }

  public void setM_silenceDetectMode(OpalSilenceDetectMode value) {
    OPALJNI.OpalParamGeneral_m_silenceDetectMode_set(swigCPtr, this, value.swigValue());
  }

  public OpalSilenceDetectMode getM_silenceDetectMode() {
    return OpalSilenceDetectMode.swigToEnum(OPALJNI.OpalParamGeneral_m_silenceDetectMode_get(swigCPtr, this));
  }

  public void setM_silenceThreshold(long value) {
    OPALJNI.OpalParamGeneral_m_silenceThreshold_set(swigCPtr, this, value);
  }

  public long getM_silenceThreshold() {
    return OPALJNI.OpalParamGeneral_m_silenceThreshold_get(swigCPtr, this);
  }

  public void setM_signalDeadband(long value) {
    OPALJNI.OpalParamGeneral_m_signalDeadband_set(swigCPtr, this, value);
  }

  public long getM_signalDeadband() {
    return OPALJNI.OpalParamGeneral_m_signalDeadband_get(swigCPtr, this);
  }

  public void setM_silenceDeadband(long value) {
    OPALJNI.OpalParamGeneral_m_silenceDeadband_set(swigCPtr, this, value);
  }

  public long getM_silenceDeadband() {
    return OPALJNI.OpalParamGeneral_m_silenceDeadband_get(swigCPtr, this);
  }

  public void setM_silenceAdaptPeriod(long value) {
    OPALJNI.OpalParamGeneral_m_silenceAdaptPeriod_set(swigCPtr, this, value);
  }

  public long getM_silenceAdaptPeriod() {
    return OPALJNI.OpalParamGeneral_m_silenceAdaptPeriod_get(swigCPtr, this);
  }

  public void setM_echoCancellation(OpalEchoCancelMode value) {
    OPALJNI.OpalParamGeneral_m_echoCancellation_set(swigCPtr, this, value.swigValue());
  }

  public OpalEchoCancelMode getM_echoCancellation() {
    return OpalEchoCancelMode.swigToEnum(OPALJNI.OpalParamGeneral_m_echoCancellation_get(swigCPtr, this));
  }

  public void setM_audioBuffers(long value) {
    OPALJNI.OpalParamGeneral_m_audioBuffers_set(swigCPtr, this, value);
  }

  public long getM_audioBuffers() {
    return OPALJNI.OpalParamGeneral_m_audioBuffers_get(swigCPtr, this);
  }

  public void setM_mediaReadData(SWIGTYPE_p_f_p_q_const__char_p_q_const__char_p_q_const__char_p_void_p_void_int__int value) {
    OPALJNI.OpalParamGeneral_m_mediaReadData_set(swigCPtr, this, SWIGTYPE_p_f_p_q_const__char_p_q_const__char_p_q_const__char_p_void_p_void_int__int.getCPtr(value));
  }

  public SWIGTYPE_p_f_p_q_const__char_p_q_const__char_p_q_const__char_p_void_p_void_int__int getM_mediaReadData() {
    long cPtr = OPALJNI.OpalParamGeneral_m_mediaReadData_get(swigCPtr, this);
    return (cPtr == 0) ? null : new SWIGTYPE_p_f_p_q_const__char_p_q_const__char_p_q_const__char_p_void_p_void_int__int(cPtr, false);
  }

  public void setM_mediaWriteData(SWIGTYPE_p_f_p_q_const__char_p_q_const__char_p_q_const__char_p_void_p_void_int__int value) {
    OPALJNI.OpalParamGeneral_m_mediaWriteData_set(swigCPtr, this, SWIGTYPE_p_f_p_q_const__char_p_q_const__char_p_q_const__char_p_void_p_void_int__int.getCPtr(value));
  }

  public SWIGTYPE_p_f_p_q_const__char_p_q_const__char_p_q_const__char_p_void_p_void_int__int getM_mediaWriteData() {
    long cPtr = OPALJNI.OpalParamGeneral_m_mediaWriteData_get(swigCPtr, this);
    return (cPtr == 0) ? null : new SWIGTYPE_p_f_p_q_const__char_p_q_const__char_p_q_const__char_p_void_p_void_int__int(cPtr, false);
  }

  public void setM_mediaDataHeader(OpalMediaDataType value) {
    OPALJNI.OpalParamGeneral_m_mediaDataHeader_set(swigCPtr, this, value.swigValue());
  }

  public OpalMediaDataType getM_mediaDataHeader() {
    return OpalMediaDataType.swigToEnum(OPALJNI.OpalParamGeneral_m_mediaDataHeader_get(swigCPtr, this));
  }

  public void setM_messageAvailable(SWIGTYPE_p_f_p_q_const__OpalMessage__int value) {
    OPALJNI.OpalParamGeneral_m_messageAvailable_set(swigCPtr, this, SWIGTYPE_p_f_p_q_const__OpalMessage__int.getCPtr(value));
  }

  public SWIGTYPE_p_f_p_q_const__OpalMessage__int getM_messageAvailable() {
    long cPtr = OPALJNI.OpalParamGeneral_m_messageAvailable_get(swigCPtr, this);
    return (cPtr == 0) ? null : new SWIGTYPE_p_f_p_q_const__OpalMessage__int(cPtr, false);
  }

  public void setM_mediaOptions(String value) {
    OPALJNI.OpalParamGeneral_m_mediaOptions_set(swigCPtr, this, value);
  }

  public String getM_mediaOptions() {
    return OPALJNI.OpalParamGeneral_m_mediaOptions_get(swigCPtr, this);
  }

  public void setM_audioBufferTime(long value) {
    OPALJNI.OpalParamGeneral_m_audioBufferTime_set(swigCPtr, this, value);
  }

  public long getM_audioBufferTime() {
    return OPALJNI.OpalParamGeneral_m_audioBufferTime_get(swigCPtr, this);
  }

  public void setM_manualAlerting(long value) {
    OPALJNI.OpalParamGeneral_m_manualAlerting_set(swigCPtr, this, value);
  }

  public long getM_manualAlerting() {
    return OPALJNI.OpalParamGeneral_m_manualAlerting_get(swigCPtr, this);
  }

  public void setM_mediaTiming(OpalMediaTiming value) {
    OPALJNI.OpalParamGeneral_m_mediaTiming_set(swigCPtr, this, value.swigValue());
  }

  public OpalMediaTiming getM_mediaTiming() {
    return OpalMediaTiming.swigToEnum(OPALJNI.OpalParamGeneral_m_mediaTiming_get(swigCPtr, this));
  }

  public void setM_videoSourceTiming(OpalMediaTiming value) {
    OPALJNI.OpalParamGeneral_m_videoSourceTiming_set(swigCPtr, this, value.swigValue());
  }

  public OpalMediaTiming getM_videoSourceTiming() {
    return OpalMediaTiming.swigToEnum(OPALJNI.OpalParamGeneral_m_videoSourceTiming_get(swigCPtr, this));
  }

  public OpalParamGeneral() {
    this(OPALJNI.new_OpalParamGeneral(), true);
  }

}
