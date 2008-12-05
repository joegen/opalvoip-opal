/* ----------------------------------------------------------------------------
 * This file was automatically generated by SWIG (http://www.swig.org).
 * Version 1.3.36
 *
 * Do not make changes to this file unless you know what you are doing--modify
 * the SWIG interface file instead.
 * ----------------------------------------------------------------------------- */

package org.opalvoip;

class exampleJNI {
  public final static native long OpalInitialise(long[] jarg1, String jarg2);
  public final static native long OpalSendMessage(long jarg1, long jarg2);
  public final static native int OPAL_C_API_VERSION_get();
  public final static native String OPAL_INITIALISE_FUNCTION_get();
  public final static native void OpalShutDown(long jarg1);
  public final static native String OPAL_SHUTDOWN_FUNCTION_get();
  public final static native long OpalGetMessage(long jarg1, long jarg2);
  public final static native String OPAL_GET_MESSAGE_FUNCTION_get();
  public final static native String OPAL_SEND_MESSAGE_FUNCTION_get();
  public final static native void OpalFreeMessage(long jarg1);
  public final static native String OPAL_FREE_MESSAGE_FUNCTION_get();
  public final static native String OPAL_PREFIX_H323_get();
  public final static native String OPAL_PREFIX_SIP_get();
  public final static native String OPAL_PREFIX_IAX2_get();
  public final static native String OPAL_PREFIX_PCSS_get();
  public final static native String OPAL_PREFIX_LOCAL_get();
  public final static native String OPAL_PREFIX_POTS_get();
  public final static native String OPAL_PREFIX_PSTN_get();
  public final static native String OPAL_PREFIX_IVR_get();
  public final static native String OPAL_PREFIX_ALL_get();
  public final static native void OpalParamGeneral_m_audioRecordDevice_set(long jarg1, OpalParamGeneral jarg1_, String jarg2);
  public final static native String OpalParamGeneral_m_audioRecordDevice_get(long jarg1, OpalParamGeneral jarg1_);
  public final static native void OpalParamGeneral_m_audioPlayerDevice_set(long jarg1, OpalParamGeneral jarg1_, String jarg2);
  public final static native String OpalParamGeneral_m_audioPlayerDevice_get(long jarg1, OpalParamGeneral jarg1_);
  public final static native void OpalParamGeneral_m_videoInputDevice_set(long jarg1, OpalParamGeneral jarg1_, String jarg2);
  public final static native String OpalParamGeneral_m_videoInputDevice_get(long jarg1, OpalParamGeneral jarg1_);
  public final static native void OpalParamGeneral_m_videoOutputDevice_set(long jarg1, OpalParamGeneral jarg1_, String jarg2);
  public final static native String OpalParamGeneral_m_videoOutputDevice_get(long jarg1, OpalParamGeneral jarg1_);
  public final static native void OpalParamGeneral_m_videoPreviewDevice_set(long jarg1, OpalParamGeneral jarg1_, String jarg2);
  public final static native String OpalParamGeneral_m_videoPreviewDevice_get(long jarg1, OpalParamGeneral jarg1_);
  public final static native void OpalParamGeneral_m_mediaOrder_set(long jarg1, OpalParamGeneral jarg1_, String jarg2);
  public final static native String OpalParamGeneral_m_mediaOrder_get(long jarg1, OpalParamGeneral jarg1_);
  public final static native void OpalParamGeneral_m_mediaMask_set(long jarg1, OpalParamGeneral jarg1_, String jarg2);
  public final static native String OpalParamGeneral_m_mediaMask_get(long jarg1, OpalParamGeneral jarg1_);
  public final static native void OpalParamGeneral_m_autoRxMedia_set(long jarg1, OpalParamGeneral jarg1_, String jarg2);
  public final static native String OpalParamGeneral_m_autoRxMedia_get(long jarg1, OpalParamGeneral jarg1_);
  public final static native void OpalParamGeneral_m_autoTxMedia_set(long jarg1, OpalParamGeneral jarg1_, String jarg2);
  public final static native String OpalParamGeneral_m_autoTxMedia_get(long jarg1, OpalParamGeneral jarg1_);
  public final static native void OpalParamGeneral_m_natRouter_set(long jarg1, OpalParamGeneral jarg1_, String jarg2);
  public final static native String OpalParamGeneral_m_natRouter_get(long jarg1, OpalParamGeneral jarg1_);
  public final static native void OpalParamGeneral_m_stunServer_set(long jarg1, OpalParamGeneral jarg1_, String jarg2);
  public final static native String OpalParamGeneral_m_stunServer_get(long jarg1, OpalParamGeneral jarg1_);
  public final static native void OpalParamGeneral_m_tcpPortBase_set(long jarg1, OpalParamGeneral jarg1_, long jarg2);
  public final static native long OpalParamGeneral_m_tcpPortBase_get(long jarg1, OpalParamGeneral jarg1_);
  public final static native void OpalParamGeneral_m_tcpPortMax_set(long jarg1, OpalParamGeneral jarg1_, long jarg2);
  public final static native long OpalParamGeneral_m_tcpPortMax_get(long jarg1, OpalParamGeneral jarg1_);
  public final static native void OpalParamGeneral_m_udpPortBase_set(long jarg1, OpalParamGeneral jarg1_, long jarg2);
  public final static native long OpalParamGeneral_m_udpPortBase_get(long jarg1, OpalParamGeneral jarg1_);
  public final static native void OpalParamGeneral_m_udpPortMax_set(long jarg1, OpalParamGeneral jarg1_, long jarg2);
  public final static native long OpalParamGeneral_m_udpPortMax_get(long jarg1, OpalParamGeneral jarg1_);
  public final static native void OpalParamGeneral_m_rtpPortBase_set(long jarg1, OpalParamGeneral jarg1_, long jarg2);
  public final static native long OpalParamGeneral_m_rtpPortBase_get(long jarg1, OpalParamGeneral jarg1_);
  public final static native void OpalParamGeneral_m_rtpPortMax_set(long jarg1, OpalParamGeneral jarg1_, long jarg2);
  public final static native long OpalParamGeneral_m_rtpPortMax_get(long jarg1, OpalParamGeneral jarg1_);
  public final static native void OpalParamGeneral_m_rtpTypeOfService_set(long jarg1, OpalParamGeneral jarg1_, long jarg2);
  public final static native long OpalParamGeneral_m_rtpTypeOfService_get(long jarg1, OpalParamGeneral jarg1_);
  public final static native void OpalParamGeneral_m_rtpMaxPayloadSize_set(long jarg1, OpalParamGeneral jarg1_, long jarg2);
  public final static native long OpalParamGeneral_m_rtpMaxPayloadSize_get(long jarg1, OpalParamGeneral jarg1_);
  public final static native void OpalParamGeneral_m_minAudioJitter_set(long jarg1, OpalParamGeneral jarg1_, long jarg2);
  public final static native long OpalParamGeneral_m_minAudioJitter_get(long jarg1, OpalParamGeneral jarg1_);
  public final static native void OpalParamGeneral_m_maxAudioJitter_set(long jarg1, OpalParamGeneral jarg1_, long jarg2);
  public final static native long OpalParamGeneral_m_maxAudioJitter_get(long jarg1, OpalParamGeneral jarg1_);
  public final static native void OpalParamGeneral_m_silenceDetectMode_set(long jarg1, OpalParamGeneral jarg1_, int jarg2);
  public final static native int OpalParamGeneral_m_silenceDetectMode_get(long jarg1, OpalParamGeneral jarg1_);
  public final static native void OpalParamGeneral_m_silenceThreshold_set(long jarg1, OpalParamGeneral jarg1_, long jarg2);
  public final static native long OpalParamGeneral_m_silenceThreshold_get(long jarg1, OpalParamGeneral jarg1_);
  public final static native void OpalParamGeneral_m_signalDeadband_set(long jarg1, OpalParamGeneral jarg1_, long jarg2);
  public final static native long OpalParamGeneral_m_signalDeadband_get(long jarg1, OpalParamGeneral jarg1_);
  public final static native void OpalParamGeneral_m_silenceDeadband_set(long jarg1, OpalParamGeneral jarg1_, long jarg2);
  public final static native long OpalParamGeneral_m_silenceDeadband_get(long jarg1, OpalParamGeneral jarg1_);
  public final static native void OpalParamGeneral_m_silenceAdaptPeriod_set(long jarg1, OpalParamGeneral jarg1_, long jarg2);
  public final static native long OpalParamGeneral_m_silenceAdaptPeriod_get(long jarg1, OpalParamGeneral jarg1_);
  public final static native void OpalParamGeneral_m_echoCancellation_set(long jarg1, OpalParamGeneral jarg1_, int jarg2);
  public final static native int OpalParamGeneral_m_echoCancellation_get(long jarg1, OpalParamGeneral jarg1_);
  public final static native void OpalParamGeneral_m_audioBuffers_set(long jarg1, OpalParamGeneral jarg1_, long jarg2);
  public final static native long OpalParamGeneral_m_audioBuffers_get(long jarg1, OpalParamGeneral jarg1_);
  public final static native void OpalParamGeneral_m_mediaReadData_set(long jarg1, OpalParamGeneral jarg1_, long jarg2);
  public final static native long OpalParamGeneral_m_mediaReadData_get(long jarg1, OpalParamGeneral jarg1_);
  public final static native void OpalParamGeneral_m_mediaWriteData_set(long jarg1, OpalParamGeneral jarg1_, long jarg2);
  public final static native long OpalParamGeneral_m_mediaWriteData_get(long jarg1, OpalParamGeneral jarg1_);
  public final static native void OpalParamGeneral_m_mediaDataHeader_set(long jarg1, OpalParamGeneral jarg1_, int jarg2);
  public final static native int OpalParamGeneral_m_mediaDataHeader_get(long jarg1, OpalParamGeneral jarg1_);
  public final static native void OpalParamGeneral_m_messageAvailable_set(long jarg1, OpalParamGeneral jarg1_, long jarg2);
  public final static native long OpalParamGeneral_m_messageAvailable_get(long jarg1, OpalParamGeneral jarg1_);
  public final static native long new_OpalParamGeneral();
  public final static native void delete_OpalParamGeneral(long jarg1);
  public final static native void OpalProductDescription_m_vendor_set(long jarg1, OpalProductDescription jarg1_, String jarg2);
  public final static native String OpalProductDescription_m_vendor_get(long jarg1, OpalProductDescription jarg1_);
  public final static native void OpalProductDescription_m_name_set(long jarg1, OpalProductDescription jarg1_, String jarg2);
  public final static native String OpalProductDescription_m_name_get(long jarg1, OpalProductDescription jarg1_);
  public final static native void OpalProductDescription_m_version_set(long jarg1, OpalProductDescription jarg1_, String jarg2);
  public final static native String OpalProductDescription_m_version_get(long jarg1, OpalProductDescription jarg1_);
  public final static native void OpalProductDescription_m_t35CountryCode_set(long jarg1, OpalProductDescription jarg1_, long jarg2);
  public final static native long OpalProductDescription_m_t35CountryCode_get(long jarg1, OpalProductDescription jarg1_);
  public final static native void OpalProductDescription_m_t35Extension_set(long jarg1, OpalProductDescription jarg1_, long jarg2);
  public final static native long OpalProductDescription_m_t35Extension_get(long jarg1, OpalProductDescription jarg1_);
  public final static native void OpalProductDescription_m_manufacturerCode_set(long jarg1, OpalProductDescription jarg1_, long jarg2);
  public final static native long OpalProductDescription_m_manufacturerCode_get(long jarg1, OpalProductDescription jarg1_);
  public final static native long new_OpalProductDescription();
  public final static native void delete_OpalProductDescription(long jarg1);
  public final static native void OpalParamProtocol_m_prefix_set(long jarg1, OpalParamProtocol jarg1_, String jarg2);
  public final static native String OpalParamProtocol_m_prefix_get(long jarg1, OpalParamProtocol jarg1_);
  public final static native void OpalParamProtocol_m_userName_set(long jarg1, OpalParamProtocol jarg1_, String jarg2);
  public final static native String OpalParamProtocol_m_userName_get(long jarg1, OpalParamProtocol jarg1_);
  public final static native void OpalParamProtocol_m_displayName_set(long jarg1, OpalParamProtocol jarg1_, String jarg2);
  public final static native String OpalParamProtocol_m_displayName_get(long jarg1, OpalParamProtocol jarg1_);
  public final static native void OpalParamProtocol_m_product_set(long jarg1, OpalParamProtocol jarg1_, long jarg2, OpalProductDescription jarg2_);
  public final static native long OpalParamProtocol_m_product_get(long jarg1, OpalParamProtocol jarg1_);
  public final static native void OpalParamProtocol_m_interfaceAddresses_set(long jarg1, OpalParamProtocol jarg1_, String jarg2);
  public final static native String OpalParamProtocol_m_interfaceAddresses_get(long jarg1, OpalParamProtocol jarg1_);
  public final static native long new_OpalParamProtocol();
  public final static native void delete_OpalParamProtocol(long jarg1);
  public final static native String OPAL_MWI_EVENT_PACKAGE_get();
  public final static native String OPAL_LINE_APPEARANCE_EVENT_PACKAGE_get();
  public final static native void OpalParamRegistration_m_protocol_set(long jarg1, OpalParamRegistration jarg1_, String jarg2);
  public final static native String OpalParamRegistration_m_protocol_get(long jarg1, OpalParamRegistration jarg1_);
  public final static native void OpalParamRegistration_m_identifier_set(long jarg1, OpalParamRegistration jarg1_, String jarg2);
  public final static native String OpalParamRegistration_m_identifier_get(long jarg1, OpalParamRegistration jarg1_);
  public final static native void OpalParamRegistration_m_hostName_set(long jarg1, OpalParamRegistration jarg1_, String jarg2);
  public final static native String OpalParamRegistration_m_hostName_get(long jarg1, OpalParamRegistration jarg1_);
  public final static native void OpalParamRegistration_m_authUserName_set(long jarg1, OpalParamRegistration jarg1_, String jarg2);
  public final static native String OpalParamRegistration_m_authUserName_get(long jarg1, OpalParamRegistration jarg1_);
  public final static native void OpalParamRegistration_m_password_set(long jarg1, OpalParamRegistration jarg1_, String jarg2);
  public final static native String OpalParamRegistration_m_password_get(long jarg1, OpalParamRegistration jarg1_);
  public final static native void OpalParamRegistration_m_adminEntity_set(long jarg1, OpalParamRegistration jarg1_, String jarg2);
  public final static native String OpalParamRegistration_m_adminEntity_get(long jarg1, OpalParamRegistration jarg1_);
  public final static native void OpalParamRegistration_m_timeToLive_set(long jarg1, OpalParamRegistration jarg1_, long jarg2);
  public final static native long OpalParamRegistration_m_timeToLive_get(long jarg1, OpalParamRegistration jarg1_);
  public final static native void OpalParamRegistration_m_restoreTime_set(long jarg1, OpalParamRegistration jarg1_, long jarg2);
  public final static native long OpalParamRegistration_m_restoreTime_get(long jarg1, OpalParamRegistration jarg1_);
  public final static native void OpalParamRegistration_m_eventPackage_set(long jarg1, OpalParamRegistration jarg1_, String jarg2);
  public final static native String OpalParamRegistration_m_eventPackage_get(long jarg1, OpalParamRegistration jarg1_);
  public final static native long new_OpalParamRegistration();
  public final static native void delete_OpalParamRegistration(long jarg1);
  public final static native void OpalStatusRegistration_m_protocol_set(long jarg1, OpalStatusRegistration jarg1_, String jarg2);
  public final static native String OpalStatusRegistration_m_protocol_get(long jarg1, OpalStatusRegistration jarg1_);
  public final static native void OpalStatusRegistration_m_serverName_set(long jarg1, OpalStatusRegistration jarg1_, String jarg2);
  public final static native String OpalStatusRegistration_m_serverName_get(long jarg1, OpalStatusRegistration jarg1_);
  public final static native void OpalStatusRegistration_m_error_set(long jarg1, OpalStatusRegistration jarg1_, String jarg2);
  public final static native String OpalStatusRegistration_m_error_get(long jarg1, OpalStatusRegistration jarg1_);
  public final static native void OpalStatusRegistration_m_status_set(long jarg1, OpalStatusRegistration jarg1_, int jarg2);
  public final static native int OpalStatusRegistration_m_status_get(long jarg1, OpalStatusRegistration jarg1_);
  public final static native void OpalStatusRegistration_m_product_set(long jarg1, OpalStatusRegistration jarg1_, long jarg2, OpalProductDescription jarg2_);
  public final static native long OpalStatusRegistration_m_product_get(long jarg1, OpalStatusRegistration jarg1_);
  public final static native long new_OpalStatusRegistration();
  public final static native void delete_OpalStatusRegistration(long jarg1);
  public final static native void OpalParamSetUpCall_m_partyA_set(long jarg1, OpalParamSetUpCall jarg1_, String jarg2);
  public final static native String OpalParamSetUpCall_m_partyA_get(long jarg1, OpalParamSetUpCall jarg1_);
  public final static native void OpalParamSetUpCall_m_partyB_set(long jarg1, OpalParamSetUpCall jarg1_, String jarg2);
  public final static native String OpalParamSetUpCall_m_partyB_get(long jarg1, OpalParamSetUpCall jarg1_);
  public final static native void OpalParamSetUpCall_m_callToken_set(long jarg1, OpalParamSetUpCall jarg1_, String jarg2);
  public final static native String OpalParamSetUpCall_m_callToken_get(long jarg1, OpalParamSetUpCall jarg1_);
  public final static native long new_OpalParamSetUpCall();
  public final static native void delete_OpalParamSetUpCall(long jarg1);
  public final static native void OpalStatusIncomingCall_m_callToken_set(long jarg1, OpalStatusIncomingCall jarg1_, String jarg2);
  public final static native String OpalStatusIncomingCall_m_callToken_get(long jarg1, OpalStatusIncomingCall jarg1_);
  public final static native void OpalStatusIncomingCall_m_localAddress_set(long jarg1, OpalStatusIncomingCall jarg1_, String jarg2);
  public final static native String OpalStatusIncomingCall_m_localAddress_get(long jarg1, OpalStatusIncomingCall jarg1_);
  public final static native void OpalStatusIncomingCall_m_remoteAddress_set(long jarg1, OpalStatusIncomingCall jarg1_, String jarg2);
  public final static native String OpalStatusIncomingCall_m_remoteAddress_get(long jarg1, OpalStatusIncomingCall jarg1_);
  public final static native void OpalStatusIncomingCall_m_remotePartyNumber_set(long jarg1, OpalStatusIncomingCall jarg1_, String jarg2);
  public final static native String OpalStatusIncomingCall_m_remotePartyNumber_get(long jarg1, OpalStatusIncomingCall jarg1_);
  public final static native void OpalStatusIncomingCall_m_remoteDisplayName_set(long jarg1, OpalStatusIncomingCall jarg1_, String jarg2);
  public final static native String OpalStatusIncomingCall_m_remoteDisplayName_get(long jarg1, OpalStatusIncomingCall jarg1_);
  public final static native void OpalStatusIncomingCall_m_calledAddress_set(long jarg1, OpalStatusIncomingCall jarg1_, String jarg2);
  public final static native String OpalStatusIncomingCall_m_calledAddress_get(long jarg1, OpalStatusIncomingCall jarg1_);
  public final static native void OpalStatusIncomingCall_m_calledPartyNumber_set(long jarg1, OpalStatusIncomingCall jarg1_, String jarg2);
  public final static native String OpalStatusIncomingCall_m_calledPartyNumber_get(long jarg1, OpalStatusIncomingCall jarg1_);
  public final static native void OpalStatusIncomingCall_m_product_set(long jarg1, OpalStatusIncomingCall jarg1_, long jarg2, OpalProductDescription jarg2_);
  public final static native long OpalStatusIncomingCall_m_product_get(long jarg1, OpalStatusIncomingCall jarg1_);
  public final static native long new_OpalStatusIncomingCall();
  public final static native void delete_OpalStatusIncomingCall(long jarg1);
  public final static native void OpalStatusMediaStream_m_callToken_set(long jarg1, OpalStatusMediaStream jarg1_, String jarg2);
  public final static native String OpalStatusMediaStream_m_callToken_get(long jarg1, OpalStatusMediaStream jarg1_);
  public final static native void OpalStatusMediaStream_m_identifier_set(long jarg1, OpalStatusMediaStream jarg1_, String jarg2);
  public final static native String OpalStatusMediaStream_m_identifier_get(long jarg1, OpalStatusMediaStream jarg1_);
  public final static native void OpalStatusMediaStream_m_type_set(long jarg1, OpalStatusMediaStream jarg1_, String jarg2);
  public final static native String OpalStatusMediaStream_m_type_get(long jarg1, OpalStatusMediaStream jarg1_);
  public final static native void OpalStatusMediaStream_m_format_set(long jarg1, OpalStatusMediaStream jarg1_, String jarg2);
  public final static native String OpalStatusMediaStream_m_format_get(long jarg1, OpalStatusMediaStream jarg1_);
  public final static native void OpalStatusMediaStream_m_state_set(long jarg1, OpalStatusMediaStream jarg1_, int jarg2);
  public final static native int OpalStatusMediaStream_m_state_get(long jarg1, OpalStatusMediaStream jarg1_);
  public final static native long new_OpalStatusMediaStream();
  public final static native void delete_OpalStatusMediaStream(long jarg1);
  public final static native void OpalParamSetUserData_m_callToken_set(long jarg1, OpalParamSetUserData jarg1_, String jarg2);
  public final static native String OpalParamSetUserData_m_callToken_get(long jarg1, OpalParamSetUserData jarg1_);
  public final static native void OpalParamSetUserData_m_userData_set(long jarg1, OpalParamSetUserData jarg1_, long jarg2);
  public final static native long OpalParamSetUserData_m_userData_get(long jarg1, OpalParamSetUserData jarg1_);
  public final static native long new_OpalParamSetUserData();
  public final static native void delete_OpalParamSetUserData(long jarg1);
  public final static native void OpalStatusUserInput_m_callToken_set(long jarg1, OpalStatusUserInput jarg1_, String jarg2);
  public final static native String OpalStatusUserInput_m_callToken_get(long jarg1, OpalStatusUserInput jarg1_);
  public final static native void OpalStatusUserInput_m_userInput_set(long jarg1, OpalStatusUserInput jarg1_, String jarg2);
  public final static native String OpalStatusUserInput_m_userInput_get(long jarg1, OpalStatusUserInput jarg1_);
  public final static native void OpalStatusUserInput_m_duration_set(long jarg1, OpalStatusUserInput jarg1_, long jarg2);
  public final static native long OpalStatusUserInput_m_duration_get(long jarg1, OpalStatusUserInput jarg1_);
  public final static native long new_OpalStatusUserInput();
  public final static native void delete_OpalStatusUserInput(long jarg1);
  public final static native void OpalStatusMessageWaiting_m_party_set(long jarg1, OpalStatusMessageWaiting jarg1_, String jarg2);
  public final static native String OpalStatusMessageWaiting_m_party_get(long jarg1, OpalStatusMessageWaiting jarg1_);
  public final static native void OpalStatusMessageWaiting_m_type_set(long jarg1, OpalStatusMessageWaiting jarg1_, String jarg2);
  public final static native String OpalStatusMessageWaiting_m_type_get(long jarg1, OpalStatusMessageWaiting jarg1_);
  public final static native void OpalStatusMessageWaiting_m_extraInfo_set(long jarg1, OpalStatusMessageWaiting jarg1_, String jarg2);
  public final static native String OpalStatusMessageWaiting_m_extraInfo_get(long jarg1, OpalStatusMessageWaiting jarg1_);
  public final static native long new_OpalStatusMessageWaiting();
  public final static native void delete_OpalStatusMessageWaiting(long jarg1);
  public final static native void OpalStatusLineAppearance_m_line_set(long jarg1, OpalStatusLineAppearance jarg1_, String jarg2);
  public final static native String OpalStatusLineAppearance_m_line_get(long jarg1, OpalStatusLineAppearance jarg1_);
  public final static native void OpalStatusLineAppearance_m_state_set(long jarg1, OpalStatusLineAppearance jarg1_, int jarg2);
  public final static native int OpalStatusLineAppearance_m_state_get(long jarg1, OpalStatusLineAppearance jarg1_);
  public final static native void OpalStatusLineAppearance_m_appearance_set(long jarg1, OpalStatusLineAppearance jarg1_, int jarg2);
  public final static native int OpalStatusLineAppearance_m_appearance_get(long jarg1, OpalStatusLineAppearance jarg1_);
  public final static native void OpalStatusLineAppearance_m_callId_set(long jarg1, OpalStatusLineAppearance jarg1_, String jarg2);
  public final static native String OpalStatusLineAppearance_m_callId_get(long jarg1, OpalStatusLineAppearance jarg1_);
  public final static native void OpalStatusLineAppearance_m_partyA_set(long jarg1, OpalStatusLineAppearance jarg1_, String jarg2);
  public final static native String OpalStatusLineAppearance_m_partyA_get(long jarg1, OpalStatusLineAppearance jarg1_);
  public final static native void OpalStatusLineAppearance_m_partyB_set(long jarg1, OpalStatusLineAppearance jarg1_, String jarg2);
  public final static native String OpalStatusLineAppearance_m_partyB_get(long jarg1, OpalStatusLineAppearance jarg1_);
  public final static native long new_OpalStatusLineAppearance();
  public final static native void delete_OpalStatusLineAppearance(long jarg1);
  public final static native void OpalStatusCallCleared_m_callToken_set(long jarg1, OpalStatusCallCleared jarg1_, String jarg2);
  public final static native String OpalStatusCallCleared_m_callToken_get(long jarg1, OpalStatusCallCleared jarg1_);
  public final static native void OpalStatusCallCleared_m_reason_set(long jarg1, OpalStatusCallCleared jarg1_, String jarg2);
  public final static native String OpalStatusCallCleared_m_reason_get(long jarg1, OpalStatusCallCleared jarg1_);
  public final static native long new_OpalStatusCallCleared();
  public final static native void delete_OpalStatusCallCleared(long jarg1);
  public final static native int OpalCallEndedWithQ931Code_get();
  public final static native void OpalParamCallCleared_m_callToken_set(long jarg1, OpalParamCallCleared jarg1_, String jarg2);
  public final static native String OpalParamCallCleared_m_callToken_get(long jarg1, OpalParamCallCleared jarg1_);
  public final static native void OpalParamCallCleared_m_reason_set(long jarg1, OpalParamCallCleared jarg1_, int jarg2);
  public final static native int OpalParamCallCleared_m_reason_get(long jarg1, OpalParamCallCleared jarg1_);
  public final static native long new_OpalParamCallCleared();
  public final static native void delete_OpalParamCallCleared(long jarg1);
  public final static native void OpalMessage_m_type_set(long jarg1, int jarg2);
  public final static native int OpalMessage_m_type_get(long jarg1);
  public final static native long OpalMessage_m_param_get(long jarg1);
  public final static native long new_OpalMessage();
  public final static native void delete_OpalMessage(long jarg1);
  public final static native void OpalMessage_m_param_m_commandError_set(long jarg1, OpalMessage_m_param jarg1_, String jarg2);
  public final static native String OpalMessage_m_param_m_commandError_get(long jarg1, OpalMessage_m_param jarg1_);
  public final static native void OpalMessage_m_param_m_general_set(long jarg1, OpalMessage_m_param jarg1_, long jarg2, OpalParamGeneral jarg2_);
  public final static native long OpalMessage_m_param_m_general_get(long jarg1, OpalMessage_m_param jarg1_);
  public final static native void OpalMessage_m_param_m_protocol_set(long jarg1, OpalMessage_m_param jarg1_, long jarg2, OpalParamProtocol jarg2_);
  public final static native long OpalMessage_m_param_m_protocol_get(long jarg1, OpalMessage_m_param jarg1_);
  public final static native void OpalMessage_m_param_m_registrationInfo_set(long jarg1, OpalMessage_m_param jarg1_, long jarg2, OpalParamRegistration jarg2_);
  public final static native long OpalMessage_m_param_m_registrationInfo_get(long jarg1, OpalMessage_m_param jarg1_);
  public final static native void OpalMessage_m_param_m_registrationStatus_set(long jarg1, OpalMessage_m_param jarg1_, long jarg2, OpalStatusRegistration jarg2_);
  public final static native long OpalMessage_m_param_m_registrationStatus_get(long jarg1, OpalMessage_m_param jarg1_);
  public final static native void OpalMessage_m_param_m_callSetUp_set(long jarg1, OpalMessage_m_param jarg1_, long jarg2, OpalParamSetUpCall jarg2_);
  public final static native long OpalMessage_m_param_m_callSetUp_get(long jarg1, OpalMessage_m_param jarg1_);
  public final static native void OpalMessage_m_param_m_callToken_set(long jarg1, OpalMessage_m_param jarg1_, String jarg2);
  public final static native String OpalMessage_m_param_m_callToken_get(long jarg1, OpalMessage_m_param jarg1_);
  public final static native void OpalMessage_m_param_m_incomingCall_set(long jarg1, OpalMessage_m_param jarg1_, long jarg2, OpalStatusIncomingCall jarg2_);
  public final static native long OpalMessage_m_param_m_incomingCall_get(long jarg1, OpalMessage_m_param jarg1_);
  public final static native void OpalMessage_m_param_m_userInput_set(long jarg1, OpalMessage_m_param jarg1_, long jarg2, OpalStatusUserInput jarg2_);
  public final static native long OpalMessage_m_param_m_userInput_get(long jarg1, OpalMessage_m_param jarg1_);
  public final static native void OpalMessage_m_param_m_messageWaiting_set(long jarg1, OpalMessage_m_param jarg1_, long jarg2, OpalStatusMessageWaiting jarg2_);
  public final static native long OpalMessage_m_param_m_messageWaiting_get(long jarg1, OpalMessage_m_param jarg1_);
  public final static native void OpalMessage_m_param_m_lineAppearance_set(long jarg1, OpalMessage_m_param jarg1_, long jarg2, OpalStatusLineAppearance jarg2_);
  public final static native long OpalMessage_m_param_m_lineAppearance_get(long jarg1, OpalMessage_m_param jarg1_);
  public final static native void OpalMessage_m_param_m_callCleared_set(long jarg1, OpalMessage_m_param jarg1_, long jarg2, OpalStatusCallCleared jarg2_);
  public final static native long OpalMessage_m_param_m_callCleared_get(long jarg1, OpalMessage_m_param jarg1_);
  public final static native void OpalMessage_m_param_m_clearCall_set(long jarg1, OpalMessage_m_param jarg1_, long jarg2, OpalParamCallCleared jarg2_);
  public final static native long OpalMessage_m_param_m_clearCall_get(long jarg1, OpalMessage_m_param jarg1_);
  public final static native void OpalMessage_m_param_m_mediaStream_set(long jarg1, OpalMessage_m_param jarg1_, long jarg2, OpalStatusMediaStream jarg2_);
  public final static native long OpalMessage_m_param_m_mediaStream_get(long jarg1, OpalMessage_m_param jarg1_);
  public final static native void OpalMessage_m_param_m_setUserData_set(long jarg1, OpalMessage_m_param jarg1_, long jarg2, OpalParamSetUserData jarg2_);
  public final static native long OpalMessage_m_param_m_setUserData_get(long jarg1, OpalMessage_m_param jarg1_);
  public final static native long new_OpalMessage_m_param();
  public final static native void delete_OpalMessage_m_param(long jarg1);
}
