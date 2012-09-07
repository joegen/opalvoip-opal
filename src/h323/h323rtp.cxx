/*
 * h323rtp.cxx
 *
 * H.323 RTP protocol handler
 *
 * Open H323 Library
 *
 * Copyright (c) 1999-2000 Equivalence Pty. Ltd.
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.0 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
 * the License for the specific language governing rights and limitations
 * under the License.
 *
 * The Original Code is Open H323 Library.
 *
 * The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#include <ptlib.h>

#include <opal/buildopts.h>
#if OPAL_H323

#ifdef __GNUC__
#pragma implementation "h323rtp.h"
#endif

#include <h323/h323rtp.h>

#include <opal/manager.h>
#include <h323/h323ep.h>
#include <h323/h323con.h>
#include <h323/h323pdu.h>
#include <h323/transaddr.h>
#include <asn/h225.h>
#include <asn/h245.h>


#define new PNEW


/////////////////////////////////////////////////////////////////////////////

PBoolean H323RTPSession::OnSendingPDU(const H323_RTPChannel & channel,
                                    H245_H2250LogicalChannelParameters & param) const
{
  PTRACE(3, "RTP\tOnSendingPDU");

  param.IncludeOptionalField(H245_H2250LogicalChannelParameters::e_mediaGuaranteedDelivery);
  param.m_mediaGuaranteedDelivery = false;

  // unicast must have mediaControlChannel
  H323TransportAddress mediaControlAddress(GetLocalAddress(false));
  param.IncludeOptionalField(H245_H2250LogicalChannelParameters::e_mediaControlChannel);
  mediaControlAddress.SetPDU(param.m_mediaControlChannel);

  if (channel.GetDirection() == H323Channel::IsReceiver) {
    // set mediaChannel
    H323TransportAddress mediaAddress(GetLocalAddress(true));
    param.IncludeOptionalField(H245_H2250LogicalChannelAckParameters::e_mediaChannel);
    mediaAddress.SetPDU(param.m_mediaChannel);
  }

  return true;
}


PBoolean H323RTPSession::OnSendingAltPDU(const H323_RTPChannel &,
                                         H245_ArrayOf_GenericInformation &) const
{
  return false; // NOTE what was this supposed to do again?;
}


void H323RTPSession::OnSendingAckPDU(const H323_RTPChannel & channel,
                                   H245_H2250LogicalChannelAckParameters & param) const
{
  PTRACE(3, "RTP\tOnSendingAckPDU");

  // set mediaControlChannel
  H323TransportAddress mediaControlAddress(GetLocalAddress(false));
  param.IncludeOptionalField(H245_H2250LogicalChannelAckParameters::e_mediaControlChannel);
  mediaControlAddress.SetPDU(param.m_mediaControlChannel);

  // set mediaChannel
  H323TransportAddress mediaAddress(GetLocalAddress(true));
  param.IncludeOptionalField(H245_H2250LogicalChannelAckParameters::e_mediaChannel);
  mediaAddress.SetPDU(param.m_mediaChannel);

  // Set dynamic payload type, if is one
  int rtpPayloadType = channel.GetDynamicRTPPayloadType();
  if (rtpPayloadType >= RTP_DataFrame::DynamicBase && rtpPayloadType < RTP_DataFrame::IllegalPayloadType) {
    param.IncludeOptionalField(H245_H2250LogicalChannelAckParameters::e_dynamicRTPPayloadType);
    param.m_dynamicRTPPayloadType = rtpPayloadType;
  }
}


PBoolean H323RTPSession::ExtractTransport(const H245_TransportAddress & pdu,
                                    PBoolean isDataPort,
                                    unsigned & errorCode)
{
  if (pdu.GetTag() != H245_TransportAddress::e_unicastAddress) {
    PTRACE(1, "RTP_UDP\tOnly unicast supported at this time");
    errorCode = H245_OpenLogicalChannelReject_cause::e_multicastChannelNotAllowed;
    return false;
  }

  if (!isDataPort && GetRemoteDataPort() != 0)
    return true; // Ignore

  H323TransportAddress transAddr = pdu;

  if (SetRemoteAddress(transAddr, isDataPort))
    return true;

  PTRACE(1, "RTP_UDP\tIllegal IP address/port in transport address.");
  return false;
}


PBoolean H323RTPSession::OnReceivedPDU(H323_RTPChannel & channel,
                                 const H245_H2250LogicalChannelParameters & param,
                                 unsigned & errorCode)
{
  bool ok = false;

  if (param.HasOptionalField(H245_H2250LogicalChannelParameters::e_mediaControlChannel)) {
    if (!ExtractTransport(param.m_mediaControlChannel, false, errorCode)) {
      PTRACE(1, "RTP_UDP\tFailed to extract mediaControl transport for " << channel);
      return false;
    }
    ok = true;
  }

  if (param.HasOptionalField(H245_H2250LogicalChannelParameters::e_mediaChannel)) {
    if (ok && channel.GetDirection() == H323Channel::IsReceiver)
      PTRACE(2, "RTP_UDP\tIgnoring media transport for " << channel);
    else if (!ExtractTransport(param.m_mediaChannel, true, errorCode)) {
      PTRACE(1, "RTP_UDP\tFailed to extract media transport for " << channel);
      return false;
    }
    ok = true;
  }

  if (ok)
    return true;

  PTRACE(1, "RTP_UDP\tNo mediaChannel or mediaControlChannel specified for " << channel);

  if (GetSessionID() == H323Capability::DefaultDataSessionID)
    return true;

  errorCode = H245_OpenLogicalChannelReject_cause::e_unspecified;
  return false;
}


PBoolean H323RTPSession::OnReceivedAckPDU(H323_RTPChannel & channel,
                                    const H245_H2250LogicalChannelAckParameters & param)
{
  if (!param.HasOptionalField(H245_H2250LogicalChannelAckParameters::e_sessionID)) {
    PTRACE(1, "RTP_UDP\tNo session specified");
  }
  
  unsigned errorCode;

  if (param.HasOptionalField(H245_H2250LogicalChannelAckParameters::e_mediaControlChannel)) {
    if (!ExtractTransport(param.m_mediaControlChannel, false, errorCode))
      return false;
  }
  else {
    PTRACE(1, "RTP_UDP\tNo mediaControlChannel specified");
    if (GetSessionID() != H323Capability::DefaultDataSessionID)
      return false;
  }

  if (!param.HasOptionalField(H245_H2250LogicalChannelAckParameters::e_mediaChannel)) {
    PTRACE(1, "RTP_UDP\tNo mediaChannel specified");
    return false;
  }

  return ExtractTransport(param.m_mediaChannel, true, errorCode);
}


void H323RTPSession::OnSendRasInfo(H225_RTPSession & info)
{
  info.m_sessionId = GetSessionID();
  info.m_ssrc = GetSyncSourceOut();
  info.m_cname = GetCanonicalName();

  H323TransportAddress lda(GetLocalAddress(true));
  lda.SetPDU(info.m_rtpAddress.m_recvAddress);
  H323TransportAddress rda(GetRemoteAddress(true));
  rda.SetPDU(info.m_rtpAddress.m_sendAddress);

  H323TransportAddress lca(GetLocalAddress(false));
  lca.SetPDU(info.m_rtcpAddress.m_recvAddress);
  H323TransportAddress rca(GetRemoteAddress(false));
  rca.SetPDU(info.m_rtcpAddress.m_sendAddress);
}


PBoolean H323RTPSession::OnReceivedAckAltPDU(H323_RTPChannel & channel,
                                             const H245_ArrayOf_GenericInformation & alternate)
{
#if OPAL_H260
  return dynamic_cast<H323Connection*>(&m_connection)->OnReceiveOLCGenericInformation(channel.GetSessionID(), alternate);
#else
  return false;
#endif
}


// NOTE look at diff here.... lda/rda etc  incompatibilities?
#if 0
  const H323Transport & transport = connection.GetControlChannel();
 
   transport.SetUpTransportPDU(info.m_rtpAddress.m_recvAddress, rtp.GetLocalDataPort());
   H323TransportAddress ta1(rtp.GetRemoteAddress(), rtp.GetRemoteDataPort());
   ta1.SetPDU(info.m_rtpAddress.m_sendAddress);
 
   transport.SetUpTransportPDU(info.m_rtcpAddress.m_recvAddress, rtp.GetLocalControlPort());
   H323TransportAddress ta2(rtp.GetRemoteAddress(), rtp.GetRemoteDataPort());
   ta2.SetPDU(info.m_rtcpAddress.m_sendAddress);
  }
#endif


#if 0 // NOTE P_QOS?
PBoolean H323_RTP_UDP::WriteTransportCapPDU(H245_TransportCapability & cap, 
                                            const H323_RTPChannel & channel) const
{
  PQoS & qos = rtp.GetQOS();
  cap.IncludeOptionalField(H245_TransportCapability::e_qOSCapabilities);
  H245_ArrayOf_QOSCapability & QoSs = cap.m_qOSCapabilities;

  H245_QOSCapability Cap = H245_QOSCapability();
  Cap.IncludeOptionalField(H245_QOSCapability::e_localQoS);
  PASN_Boolean & localqos = Cap.m_localQoS;
  localqos.SetValue(TRUE);

  Cap.IncludeOptionalField(H245_QOSCapability::e_dscpValue);
  PASN_Integer & dscp = Cap.m_dscpValue;
  dscp = qos.GetDSCP();

  if (PUDPSocket::SupportQoS(rtp.GetLocalAddress())) {        
    Cap.IncludeOptionalField(H245_QOSCapability::e_rsvpParameters);
    H245_RSVPParameters & rsvp = Cap.m_rsvpParameters; 

    if (channel.GetDirection() == H323Channel::IsReceiver) {   /// If Reply don't have to send body
      rtp.EnableGQoS(TRUE);
      return TRUE;
    }
    rsvp.IncludeOptionalField(H245_RSVPParameters::e_qosMode); 
    H245_QOSMode & mode = rsvp.m_qosMode;
    if (qos.GetServiceType() == SERVICETYPE_GUARANTEED) 
      mode.SetTag(H245_QOSMode::e_guaranteedQOS); 
    else 
      mode.SetTag(H245_QOSMode::e_controlledLoad); 

    rsvp.IncludeOptionalField(H245_RSVPParameters::e_tokenRate); 
    rsvp.m_tokenRate = qos.GetTokenRate();
    rsvp.IncludeOptionalField(H245_RSVPParameters::e_bucketSize);
    rsvp.m_bucketSize = qos.GetTokenBucketSize();
    rsvp.HasOptionalField(H245_RSVPParameters::e_peakRate);
    rsvp.m_peakRate = qos.GetPeakBandwidth();
  }
  QoSs.SetSize(1);
  QoSs[0] = Cap;
  return TRUE;
}


void H323_RTP_UDP::ReadTransportCapPDU(const H245_TransportCapability & cap,
  H323_RTPChannel & channel)
{
  if (!cap.HasOptionalField(H245_TransportCapability::e_qOSCapabilities)) 
    return;

  const H245_ArrayOf_QOSCapability QoSs = cap.m_qOSCapabilities;
  for (PINDEX i =0; i < QoSs.GetSize(); i++) {
    PQoS & qos = rtp.GetQOS();
    const H245_QOSCapability & QoS = QoSs[i];
    //        if (QoS.HasOptionalField(H245_QOSCapability::e_localQoS)) {
    //           PASN_Boolean & localqos = QoS.m_localQoS;
    //        }
    if (QoS.HasOptionalField(H245_QOSCapability::e_dscpValue)) {
      const PASN_Integer & dscp = QoS.m_dscpValue;
      qos.SetDSCP(dscp);
    }

    if (PUDPSocket::SupportQoS(rtp.GetLocalAddress())) {
      if (!QoS.HasOptionalField(H245_QOSCapability::e_rsvpParameters)) {
        PTRACE(4,"TRANS\tDisabling GQoS");
        rtp.EnableGQoS(FALSE);  
        return;
      }

      const H245_RSVPParameters & rsvp = QoS.m_rsvpParameters; 
      if (channel.GetDirection() != H323Channel::IsReceiver) {
        rtp.EnableGQoS(TRUE);
        return;
      }      
      if (rsvp.HasOptionalField(H245_RSVPParameters::e_qosMode)) {
        const H245_QOSMode & mode = rsvp.m_qosMode;
        if (mode.GetTag() == H245_QOSMode::e_guaranteedQOS) {
          qos.SetWinServiceType(SERVICETYPE_GUARANTEED);
          qos.SetDSCP(PQoS::guaranteedDSCP);
        } else {
          qos.SetWinServiceType(SERVICETYPE_CONTROLLEDLOAD);
          qos.SetDSCP(PQoS::controlledLoadDSCP);
        }
      }
      if (rsvp.HasOptionalField(H245_RSVPParameters::e_tokenRate)) 
        qos.SetAvgBytesPerSec(rsvp.m_tokenRate);
      if (rsvp.HasOptionalField(H245_RSVPParameters::e_bucketSize))
        qos.SetMaxFrameBytes(rsvp.m_bucketSize);
      if (rsvp.HasOptionalField(H245_RSVPParameters::e_peakRate))
        qos.SetPeakBytesPerSec(rsvp.m_peakRate);    
    }
  }
}
#endif
#endif // OPAL_H323

/////////////////////////////////////////////////////////////////////////////
