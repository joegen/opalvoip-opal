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

H323_RTPChannel::H323_RTPChannel(H323Connection & conn,
                                 const H323Capability & cap,
                                 Directions direction,
                                 OpalMediaSession & session)
  : H323_RealTimeChannel(conn, cap, direction)
  , m_session(session)
{
  PTRACE(3, "H323RTP\t" << (receiver ? "Receiver" : "Transmitter")
         << " created using session " << GetSessionID());
}


H323_RTPChannel::~H323_RTPChannel()
{
  // Finished with the RTP session, this will delete the session if it is no
  // longer referenced by any logical channels.
  connection.ReleaseMediaSession(GetSessionID());
}


unsigned H323_RTPChannel::GetSessionID() const
{
  return m_session.GetSessionID();
}


bool H323_RTPChannel::SetSessionID(unsigned sessionID)
{
  unsigned oldSessionID = GetSessionID();
  if (sessionID == oldSessionID)
    return true;

  return connection.ChangeSessionID(oldSessionID, sessionID);
}


PBoolean H323_RTPChannel::GetMediaTransportAddress(OpalTransportAddress & data,
                                                   OpalTransportAddress & control) const
{
  const OpalRTPSession & rtpSession = dynamic_cast<const OpalRTPSession &>(m_session);
  data = rtpSession.GetRemoteAddress(true);
  control = rtpSession.GetRemoteAddress(false);
  return true;
}


PBoolean H323_RTPChannel::OnSendingPDU(H245_H2250LogicalChannelParameters & param) const
{
  PTRACE(3, "RTP\tOnSendingPDU");

  param.IncludeOptionalField(H245_H2250LogicalChannelParameters::e_mediaGuaranteedDelivery);
  param.m_mediaGuaranteedDelivery = false;

  // unicast must have mediaControlChannel
  H323TransportAddress mediaControlAddress(m_session.GetLocalAddress(false));
  param.IncludeOptionalField(H245_H2250LogicalChannelParameters::e_mediaControlChannel);
  mediaControlAddress.SetPDU(param.m_mediaControlChannel);

  if (GetDirection() == H323Channel::IsReceiver) {
    // set mediaChannel
    H323TransportAddress mediaAddress(m_session.GetLocalAddress(true));
    param.IncludeOptionalField(H245_H2250LogicalChannelAckParameters::e_mediaChannel);
    mediaAddress.SetPDU(param.m_mediaChannel);
  }

  if (GetDirection() != H323Channel::IsReceiver) {
    // Set flag for we are going to stop sending audio on silence
    if (m_mediaFormat.GetMediaType() == OpalMediaType::Audio()) {
      param.IncludeOptionalField(H245_H2250LogicalChannelParameters::e_silenceSuppression);
      param.m_silenceSuppression = (connection.GetEndPoint().GetManager().GetSilenceDetectParams().m_mode != OpalSilenceDetector::NoSilenceDetection);
    }
  }

  // Set dynamic payload type, if is one
  RTP_DataFrame::PayloadTypes rtpPayloadType = GetDynamicRTPPayloadType();
  if (rtpPayloadType >= RTP_DataFrame::DynamicBase && rtpPayloadType < RTP_DataFrame::IllegalPayloadType) {
    param.IncludeOptionalField(H245_H2250LogicalChannelParameters::e_dynamicRTPPayloadType);
    param.m_dynamicRTPPayloadType = (int)rtpPayloadType;
  }

  // Set the media packetization field if have an option to describe it.
  param.m_mediaPacketization.SetTag(H245_H2250LogicalChannelParameters_mediaPacketization::e_rtpPayloadType);
  if (H323SetRTPPacketization(param.m_mediaPacketization, m_mediaFormat, rtpPayloadType))
    param.IncludeOptionalField(H245_H2250LogicalChannelParameters::e_mediaPacketization);

  return H323_RealTimeChannel::OnSendingPDU(param);
}


PBoolean H323_RTPChannel::OnSendingAltPDU(H245_ArrayOf_GenericInformation & alternate) const
{
  PTRACE ( 4, "H323RTP\tOnSendingAltPDU");
#if OPAL_H460
  return connection.OnSendingOLCGenericInformation(GetSessionID(), alternate, false);
#else
  return false;
#endif
}


void H323_RTPChannel::OnSendOpenAck(H245_H2250LogicalChannelAckParameters & param) const
{
  PTRACE(3, "RTP\tOnSendingAckPDU");

  // set mediaControlChannel
  H323TransportAddress mediaControlAddress(m_session.GetLocalAddress(false));
  param.IncludeOptionalField(H245_H2250LogicalChannelAckParameters::e_mediaControlChannel);
  mediaControlAddress.SetPDU(param.m_mediaControlChannel);

  // set mediaChannel
  H323TransportAddress mediaAddress(m_session.GetLocalAddress(true));
  param.IncludeOptionalField(H245_H2250LogicalChannelAckParameters::e_mediaChannel);
  mediaAddress.SetPDU(param.m_mediaChannel);

  // Set dynamic payload type, if is one
  int rtpPayloadType = GetDynamicRTPPayloadType();
  if (rtpPayloadType >= RTP_DataFrame::DynamicBase && rtpPayloadType < RTP_DataFrame::IllegalPayloadType) {
    param.IncludeOptionalField(H245_H2250LogicalChannelAckParameters::e_dynamicRTPPayloadType);
    param.m_dynamicRTPPayloadType = rtpPayloadType;
  }

  H323_RealTimeChannel::OnSendOpenAck(param);
}


PBoolean H323_RTPChannel::OnReceivedPDU(const H245_H2250LogicalChannelParameters & param,
                                    unsigned & errorCode)
{
  bool ok = false;

  if (param.HasOptionalField(H245_H2250LogicalChannelParameters::e_mediaControlChannel)) {
    if (!ExtractTransport(param.m_mediaControlChannel, false, errorCode)) {
      PTRACE(1, "RTP_UDP\tFailed to extract mediaControl transport for " << *this);
      return false;
    }
    ok = true;
  }

  if (param.HasOptionalField(H245_H2250LogicalChannelParameters::e_mediaChannel)) {
    if (ok && GetDirection() == H323Channel::IsReceiver)
      PTRACE(2, "RTP_UDP\tIgnoring media transport for " << *this);
    else if (!ExtractTransport(param.m_mediaChannel, true, errorCode)) {
      PTRACE(1, "RTP_UDP\tFailed to extract media transport for " << *this);
      return false;
    }
    ok = true;
  }

  if (!ok) {
    PTRACE(1, "RTP_UDP\tNo mediaChannel or mediaControlChannel specified for " << *this);

    if (GetSessionID() != H323Capability::DefaultDataSessionID) {
      errorCode = H245_OpenLogicalChannelReject_cause::e_unspecified;
      return false;
    }
  }

  // Override default payload type if indicated by remote
  RTP_DataFrame::PayloadTypes rtpPayloadType = m_mediaFormat.GetPayloadType();
  if (param.HasOptionalField(H245_H2250LogicalChannelParameters::e_dynamicRTPPayloadType))
    rtpPayloadType = (RTP_DataFrame::PayloadTypes)param.m_dynamicRTPPayloadType.GetValue();

  // Update actual media packetization if present
  PString mediaPacketization;
  if (param.HasOptionalField(H245_H2250LogicalChannelParameters::e_mediaPacketization) &&
      param.m_mediaPacketization.GetTag() == H245_H2250LogicalChannelParameters_mediaPacketization::e_rtpPayloadType)
    mediaPacketization = H323GetRTPPacketization(param.m_mediaPacketization);

  // Hack for H.263 compatibility, default to RFC2190 if not told otherwise
  if (mediaPacketization.IsEmpty() && m_mediaFormat == OPAL_H263)
    mediaPacketization = "RFC2190";

  m_mediaFormat.SetPayloadType(rtpPayloadType);
  m_mediaFormat.SetMediaPacketizations(mediaPacketization);
  if (m_mediaStream != NULL)
    m_mediaStream->UpdateMediaFormat(m_mediaFormat);

  return H323_RealTimeChannel::OnReceivedPDU(param, errorCode);
}


PBoolean H323_RTPChannel::OnReceivedAckPDU(const H245_H2250LogicalChannelAckParameters & param)
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

  return ExtractTransport(param.m_mediaChannel, true, errorCode) && H323_RealTimeChannel::OnReceivedAckPDU(param);
}


PBoolean H323_RTPChannel::OnReceivedAckAltPDU(const H245_ArrayOf_GenericInformation & alternate)
{ 
#if OPAL_H460
  return connection.OnReceiveOLCGenericInformation(GetSessionID(), alternate);
#else
  return false;
#endif
}


bool H323_RTPChannel::ExtractTransport(const H245_TransportAddress & pdu,
                                    bool isDataPort,
                                    unsigned & errorCode)
{
  if (pdu.GetTag() != H245_TransportAddress::e_unicastAddress) {
    PTRACE(1, "RTP_UDP\tOnly unicast supported at this time");
    errorCode = H245_OpenLogicalChannelReject_cause::e_multicastChannelNotAllowed;
    return false;
  }

  if (!isDataPort && !m_session.GetRemoteAddress().IsEmpty())
    return true; // Ignore

  H323TransportAddress transAddr = pdu;

  if (m_session.SetRemoteAddress(transAddr, isDataPort))
    return true;

  PTRACE(1, "RTP_UDP\tIllegal IP address/port in transport address.");
  return false;
}


void H323_RTPChannel::OnSendRasInfo(H225_RTPSession & info)
{
  info.m_sessionId = m_session.GetSessionID();

  OpalRTPSession * rtpSession = dynamic_cast<OpalRTPSession *>(&m_session);
  if (rtpSession != NULL) {
    info.m_ssrc = rtpSession->GetSyncSourceOut();
    info.m_cname = rtpSession->GetCanonicalName();
  }

  H323TransportAddress lda(m_session.GetLocalAddress(true));
  lda.SetPDU(info.m_rtpAddress.m_recvAddress);
  H323TransportAddress rda(m_session.GetRemoteAddress(true));
  rda.SetPDU(info.m_rtpAddress.m_sendAddress);

  H323TransportAddress lca(m_session.GetLocalAddress(false));
  lca.SetPDU(info.m_rtcpAddress.m_recvAddress);
  H323TransportAddress rca(m_session.GetRemoteAddress(false));
  rca.SetPDU(info.m_rtcpAddress.m_sendAddress);
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
