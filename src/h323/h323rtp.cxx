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
 * $Log: h323rtp.cxx,v $
 * Revision 1.2003  2001/10/05 00:22:14  robertj
 * Updated to PWLib 1.2.0 and OpenH323 1.7.0
 *
 * Revision 2.1  2001/08/13 05:10:40  robertj
 * Updates from OpenH323 v1.6.0 release.
 *
 * Revision 2.0  2001/07/27 15:48:25  robertj
 * Conversion of OpenH323 to Open Phone Abstraction Library (OPAL)
 *
 * Revision 1.16  2001/10/02 02:06:23  robertj
 * Fixed CIsco IOS compatibility, yet again!.
 *
 * Revision 1.15  2001/10/02 01:53:53  robertj
 * Fixed CIsco IOS compatibility, again.
 *
 * Revision 1.14  2001/08/06 03:08:57  robertj
 * Fission of h323.h to h323ep.h & h323con.h, h323.h now just includes files.
 *
 * Revision 1.13  2001/02/09 05:13:56  craigs
 * Added pragma implementation to (hopefully) reduce the executable image size
 * under Linux
 *
 * Revision 1.12  2001/01/25 07:27:16  robertj
 * Major changes to add more flexible OpalMediaFormat class to normalise
 *   all information about media types, especially codecs.
 *
 * Revision 1.11  2000/12/18 08:59:20  craigs
 * Added ability to set ports
 *
 * Revision 1.10  2000/09/22 00:32:34  craigs
 * Added extra logging
 * Fixed problems with no fastConnect with tunelling
 *
 * Revision 1.9  2000/08/31 08:15:41  robertj
 * Added support for dynamic RTP payload types in H.245 OpenLogicalChannel negotiations.
 *
 * Revision 1.8  2000/08/23 14:27:04  craigs
 * Added prototype support for Microsoft GSM codec
 *
 * Revision 1.7  2000/07/12 13:06:49  robertj
 * Removed test for sessionID in OLC, just trace a warning instead of abandoning connection.
 *
 * Revision 1.6  2000/07/11 19:36:43  robertj
 * Fixed silenceSuppression field in OLC only to be included on transmitter.
 *
 * Revision 1.5  2000/05/23 12:57:37  robertj
 * Added ability to change IP Type Of Service code from applications.
 *
 * Revision 1.4  2000/05/02 04:32:27  robertj
 * Fixed copyright notice comment.
 *
 * Revision 1.3  2000/04/05 03:17:32  robertj
 * Added more RTP statistics gathering and H.245 round trip delay calculation.
 *
 * Revision 1.2  2000/01/20 05:57:46  robertj
 * Added extra flexibility in receiving incorrectly formed OpenLogicalChannel PDU's
 *
 * Revision 1.1  1999/12/23 23:02:36  robertj
 * File reorganision for separating RTP from H.323 and creation of LID for VPB support.
 *
 */

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "h323rtp.h"
#endif

#include <h323/h323rtp.h>

#include <opal/manager.h>
#include <h323/h323ep.h>
#include <h323/h323con.h>
#include <h323/transaddr.h>
#include <asn/h225.h>
#include <asn/h245.h>


#define new PNEW


/////////////////////////////////////////////////////////////////////////////

H323_RTP_Session::H323_RTP_Session(const H323Connection & conn)
  : connection(conn)
{
}


void H323_RTP_Session::OnTxStatistics(const RTP_Session & session) const
{
  connection.OnRTPStatistics(session);
}


void H323_RTP_Session::OnRxStatistics(const RTP_Session & session) const
{
  connection.OnRTPStatistics(session);
}


/////////////////////////////////////////////////////////////////////////////

H323_RTP_UDP::H323_RTP_UDP(const H323Connection & conn, RTP_UDP & rtp_udp)
  : H323_RTP_Session(conn),
    rtp(rtp_udp)
{
  const OpalTransport & transport = connection.GetControlChannel();
  PIPSocket::Address localAddress;
  WORD dummy;
  transport.GetLocalAddress().GetIpAndPort(localAddress, dummy);

  const H323EndPoint & endpoint = connection.GetEndPoint();
  rtp.Open(localAddress,
           endpoint.GetManager().GetRtpIpPortBase(),
           endpoint.GetManager().GetRtpIpPortMax(),
           endpoint.GetManager().GetRtpIpTypeofService());
}


BOOL H323_RTP_UDP::OnSendingPDU(const H323_RTPChannel & channel,
                                H245_H2250LogicalChannelParameters & param) const
{
  PTRACE(3, "RTP\tOnSendingPDU");

  param.m_sessionID = rtp.GetSessionID();

  param.IncludeOptionalField(H245_H2250LogicalChannelParameters::e_mediaGuaranteedDelivery);
  param.m_mediaGuaranteedDelivery = FALSE;

  if (channel.GetDirection() != H323Channel::IsReceiver) {
    param.IncludeOptionalField(H245_H2250LogicalChannelParameters::e_silenceSuppression);
    param.m_silenceSuppression = TRUE;
  }

  // unicast must have mediaControlChannel
  H323TransportAddress mediaControlAddress(rtp.GetLocalAddress(), rtp.GetLocalControlPort());
  param.IncludeOptionalField(H245_H2250LogicalChannelParameters::e_mediaControlChannel);
  mediaControlAddress.SetPDU(param.m_mediaControlChannel);

  if (channel.GetDirection() == H323Channel::IsReceiver) {
    // set mediaChannel
    H323TransportAddress mediaAddress(rtp.GetLocalAddress(), rtp.GetLocalDataPort());
    param.IncludeOptionalField(H245_H2250LogicalChannelAckParameters::e_mediaChannel);
    mediaAddress.SetPDU(param.m_mediaChannel);
  }

  // Set dynamic payload type, if is one
  int rtpPayloadType = channel.GetDynamicRTPPayloadType();
  if (rtpPayloadType >= RTP_DataFrame::DynamicBase && rtpPayloadType < RTP_DataFrame::MaxPayloadType) {
    param.IncludeOptionalField(H245_H2250LogicalChannelParameters::e_dynamicRTPPayloadType);
    param.m_dynamicRTPPayloadType = rtpPayloadType;
  }

  return TRUE;
}


void H323_RTP_UDP::OnSendingAckPDU(const H323_RTPChannel & channel,
                                   H245_H2250LogicalChannelAckParameters & param) const
{
  PTRACE(3, "RTP\tOnSendingAckPDU");

  // set mediaControlChannel
  H323TransportAddress mediaControlAddress(rtp.GetLocalAddress(), rtp.GetLocalControlPort());
  param.IncludeOptionalField(H245_H2250LogicalChannelAckParameters::e_mediaControlChannel);
  mediaControlAddress.SetPDU(param.m_mediaControlChannel);

  // set mediaChannel
  H323TransportAddress mediaAddress(rtp.GetLocalAddress(), rtp.GetLocalDataPort());
  param.IncludeOptionalField(H245_H2250LogicalChannelAckParameters::e_mediaChannel);
  mediaAddress.SetPDU(param.m_mediaChannel);

  // Set dynamic payload type, if is one
  int rtpPayloadType = channel.GetDynamicRTPPayloadType();
  if (rtpPayloadType >= RTP_DataFrame::DynamicBase && rtpPayloadType < RTP_DataFrame::MaxPayloadType) {
    param.IncludeOptionalField(H245_H2250LogicalChannelAckParameters::e_dynamicRTPPayloadType);
    param.m_dynamicRTPPayloadType = rtpPayloadType;
  }
}


static BOOL ExtractTransport(const H245_TransportAddress & pdu,
                             RTP_UDP & rtp,
                             BOOL isDataPort,
                             unsigned & errorCode)
{
  if (pdu.GetTag() != H245_TransportAddress::e_unicastAddress) {
    PTRACE(1, "RTP_UDP\tOnly unicast supported at this time");
    errorCode = H245_OpenLogicalChannelReject_cause::e_multicastChannelNotAllowed;
    return FALSE;
  }

  const H245_UnicastAddress & unicast = pdu;
  if (unicast.GetTag() != H245_UnicastAddress::e_iPAddress) {
    PTRACE(1, "RTP_UDP\tLogic error, UDP must be IP unicast");
    errorCode = H245_OpenLogicalChannelReject_cause::e_unspecified;
    return FALSE;
  }

  const H245_UnicastAddress_iPAddress & addr = unicast;

  PIPSocket::Address ipnum(addr.m_network[0],
                           addr.m_network[1],
                           addr.m_network[2],
                           addr.m_network[3]);

  return rtp.SetRemoteSocketInfo(ipnum,
                                 (WORD)(unsigned)addr.m_tsapIdentifier,
                                 isDataPort);
}


BOOL H323_RTP_UDP::OnReceivedPDU(H323_RTPChannel & channel,
                                 const H245_H2250LogicalChannelParameters & param,
                                 unsigned & errorCode)
{
  if (param.m_sessionID != rtp.GetSessionID()) {
    PTRACE(1, "RTP_UDP\tOpen of " << channel << " with invalid session: " << param.m_sessionID);
    errorCode = H245_OpenLogicalChannelReject_cause::e_invalidSessionID;
    return FALSE;
  }

  BOOL ok = FALSE;

  if (param.HasOptionalField(H245_H2250LogicalChannelParameters::e_mediaControlChannel)) {
    if (!ExtractTransport(param.m_mediaControlChannel, rtp, FALSE, errorCode)) {
      PTRACE(1, "RTP_UDP\tFailed to extract mediaControl transport for " << channel);
      return FALSE;
    }
    ok = TRUE;
  }

  if (param.HasOptionalField(H245_H2250LogicalChannelParameters::e_mediaChannel)) {
    if (ok && channel.GetDirection() == H323Channel::IsReceiver)
      PTRACE(3, "RTP_UDP\tIgnoring media transport for " << channel);
    else if (!ExtractTransport(param.m_mediaChannel, rtp, TRUE, errorCode)) {
      PTRACE(1, "RTP_UDP\tFailed to extract media transport for " << channel);
      return FALSE;
    }
    ok = TRUE;
  }

  if (param.HasOptionalField(H245_H2250LogicalChannelParameters::e_dynamicRTPPayloadType))
    channel.SetDynamicRTPPayloadType(param.m_dynamicRTPPayloadType);

  if (ok)
    return TRUE;

  PTRACE(1, "RTP_UDP\tNo mediaChannel or mediaControlChannel specified for " << channel);
  errorCode = H245_OpenLogicalChannelReject_cause::e_unspecified;
  return FALSE;
}


BOOL H323_RTP_UDP::OnReceivedAckPDU(H323_RTPChannel & channel,
                                    const H245_H2250LogicalChannelAckParameters & param)
{
  if (!param.HasOptionalField(H245_H2250LogicalChannelAckParameters::e_sessionID)) {
    PTRACE(1, "RTP_UDP\tNo session specified");
  }

  if (param.m_sessionID != rtp.GetSessionID()) {
    PTRACE(1, "RTP_UDP\tAck for invalid session: " << param.m_sessionID);
  }

  if (!param.HasOptionalField(H245_H2250LogicalChannelAckParameters::e_mediaControlChannel)) {
    PTRACE(1, "RTP_UDP\tNo mediaControlChannel specified");
    return FALSE;
  }

  unsigned errorCode;
  if (!ExtractTransport(param.m_mediaControlChannel, rtp, FALSE, errorCode))
    return FALSE;

  if (!param.HasOptionalField(H245_H2250LogicalChannelAckParameters::e_mediaChannel)) {
    PTRACE(1, "RTP_UDP\tNo mediaChannel specified");
    return FALSE;
  }

  if (!ExtractTransport(param.m_mediaChannel, rtp, TRUE, errorCode))
    return FALSE;

  if (param.HasOptionalField(H245_H2250LogicalChannelAckParameters::e_dynamicRTPPayloadType))
    channel.SetDynamicRTPPayloadType(param.m_dynamicRTPPayloadType);

  return TRUE;
}


void H323_RTP_UDP::OnSendRasInfo(H225_RTPSession & info)
{
  info.m_sessionId = rtp.GetSessionID();
  info.m_ssrc = rtp.GetSyncSourceOut();

  H323TransportAddress lda(rtp.GetLocalAddress(), rtp.GetLocalDataPort());
  lda.SetPDU(info.m_rtpAddress.m_recvAddress);
  H323TransportAddress rda(rtp.GetRemoteAddress(), rtp.GetRemoteDataPort());
  rda.SetPDU(info.m_rtpAddress.m_sendAddress);

  H323TransportAddress lca(rtp.GetLocalAddress(), rtp.GetLocalControlPort());
  lca.SetPDU(info.m_rtcpAddress.m_recvAddress);
  H323TransportAddress rca(rtp.GetRemoteAddress(), rtp.GetRemoteDataPort());
  rca.SetPDU(info.m_rtcpAddress.m_sendAddress);
}


/////////////////////////////////////////////////////////////////////////////
