/*
 * channels.cxx
 *
 * H.323 protocol handler
 *
 * Open H323 Library
 *
 * Copyright (c) 1998-2000 Equivalence Pty. Ltd.
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
 * Portions of this code were written with the assisance of funding from
 * Vovida Networks, Inc. http://www.vovida.com.
 *
 * Contributor(s): ______________________________________.
 *
 * $Id$
 */

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "channels.h"
#endif

#include <h323/channels.h>

#include <opal/transports.h>

#if OPAL_VIDEO
#include <codec/vidcodec.h>
#endif

#include <h323/h323ep.h>
#include <h323/h323con.h>
#include <h323/h323rtp.h>
#include <h323/transaddr.h>
#include <h323/h323pdu.h>


#define new PNEW


#define	MAX_PAYLOAD_TYPE_MISMATCHES 8
#define RTP_TRACE_DISPLAY_RATE 16000 // 2 seconds


#if PTRACING

ostream & operator<<(ostream & out, H323Channel::Directions dir)
{
  static const char * const DirNames[H323Channel::NumDirections] = {
    "IsBidirectional", "IsTransmitter", "IsReceiver"
  };

  if (dir < H323Channel::NumDirections && DirNames[dir] != NULL)
    out << DirNames[dir];
  else
    out << "Direction<" << (unsigned)dir << '>';

  return out;
}

#endif


/////////////////////////////////////////////////////////////////////////////

H323ChannelNumber::H323ChannelNumber(unsigned num, BOOL fromRem)
{
  PAssert(num < 0x10000, PInvalidParameter);
  number = num;
  fromRemote = fromRem;
}


PObject * H323ChannelNumber::Clone() const
{
  return new H323ChannelNumber(number, fromRemote);
}


PINDEX H323ChannelNumber::HashFunction() const
{
  PINDEX hash = (number%17) << 1;
  if (fromRemote)
    hash++;
  return hash;
}


void H323ChannelNumber::PrintOn(ostream & strm) const
{
  strm << (fromRemote ? 'R' : 'T') << '-' << number;
}


PObject::Comparison H323ChannelNumber::Compare(const PObject & obj) const
{
#ifndef PASN_LEANANDMEAN
  PAssert(PIsDescendant(&obj, H323ChannelNumber), PInvalidCast);
#endif
  const H323ChannelNumber & other = (const H323ChannelNumber &)obj;
  if (number < other.number)
    return LessThan;
  if (number > other.number)
    return GreaterThan;
  if (fromRemote && !other.fromRemote)
    return LessThan;
  if (!fromRemote && other.fromRemote)
    return GreaterThan;
  return EqualTo;
}


H323ChannelNumber & H323ChannelNumber::operator++(int)
{
  number++;
  return *this;
}


/////////////////////////////////////////////////////////////////////////////

H323Channel::H323Channel(H323Connection & conn, const H323Capability & cap)
  : endpoint(conn.GetEndPoint()),
    connection(conn)
{
  capability = (H323Capability *)cap.Clone();
  bandwidthUsed = 0;
  terminating = FALSE;
  opened = FALSE;
  paused = TRUE;
}


H323Channel::~H323Channel()
{
  connection.SetBandwidthUsed(bandwidthUsed, 0);

  delete capability;
}


void H323Channel::PrintOn(ostream & strm) const
{
  strm << number;
}


unsigned H323Channel::GetSessionID() const
{
  return 0;
}


BOOL H323Channel::GetMediaTransportAddress(OpalTransportAddress & /*data*/,
                                           OpalTransportAddress & /*control*/) const
{
  return FALSE;
}


void H323Channel::Close()
{
  if (!opened || terminating)
    return;

  terminating = TRUE;

  // Signal to the connection that this channel is on the way out
  connection.OnClosedLogicalChannel(*this);

  PTRACE(4, "LogChan\tCleaned up " << number);
}


BOOL H323Channel::OnReceivedPDU(const H245_OpenLogicalChannel & /*pdu*/,
                                unsigned & /*errorCode*/)
{
  return TRUE;
}


BOOL H323Channel::OnReceivedAckPDU(const H245_OpenLogicalChannelAck & /*pdu*/)
{
  return TRUE;
}


void H323Channel::OnSendOpenAck(const H245_OpenLogicalChannel & /*pdu*/,
                                H245_OpenLogicalChannelAck & /* pdu*/) const
{
}


void H323Channel::OnFlowControl(long PTRACE_PARAM(bitRateRestriction))
{
  PTRACE(3, "LogChan\tOnFlowControl: " << bitRateRestriction);
}


void H323Channel::OnMiscellaneousCommand(const H245_MiscellaneousCommand_type & type)
{
  PTRACE(3, "LogChan\tOnMiscellaneousCommand: chan=" << number << ", type=" << type.GetTagName());
  OpalMediaStream * mediaStream = GetMediaStream();
  if (mediaStream == NULL)
    return;

  switch (type.GetTag()) {
#if OPAL_VIDEO
    case H245_MiscellaneousCommand_type::e_videoFastUpdatePicture :
      mediaStream->ExecuteCommand(OpalVideoUpdatePicture());
      break;

    case H245_MiscellaneousCommand_type::e_videoFastUpdateGOB :
      {
        const H245_MiscellaneousCommand_type_videoFastUpdateGOB & vfuGOB = type;
        mediaStream->ExecuteCommand(OpalVideoUpdatePicture(vfuGOB.m_firstGOB, -1, vfuGOB.m_numberOfGOBs));
      }
      break;

    case H245_MiscellaneousCommand_type::e_videoFastUpdateMB :
      {
        const H245_MiscellaneousCommand_type_videoFastUpdateMB & vfuMB = type;
        mediaStream->ExecuteCommand(OpalVideoUpdatePicture(vfuMB.m_firstGOB, vfuMB.m_firstMB, vfuMB.m_numberOfMBs));
      }
      break;
#endif
  }
}


void H323Channel::OnMiscellaneousIndication(const H245_MiscellaneousIndication_type & PTRACE_PARAM(type))
{
  PTRACE(3, "LogChan\tOnMiscellaneousIndication: chan=" << number
         << ", type=" << type.GetTagName());
}


void H323Channel::OnJitterIndication(DWORD PTRACE_PARAM(jitter),
                                     int   PTRACE_PARAM(skippedFrameCount),
                                     int   PTRACE_PARAM(additionalBuffer))
{
  PTRACE(3, "LogChan\tOnJitterIndication:"
            " jitter=" << jitter <<
            " skippedFrameCount=" << skippedFrameCount <<
            " additionalBuffer=" << additionalBuffer);
}


BOOL H323Channel::SetBandwidthUsed(unsigned bandwidth)
{
  PTRACE(3, "LogChan\tBandwidth requested/used = "
         << bandwidth/10 << '.' << bandwidth%10 << '/'
         << bandwidthUsed/10 << '.' << bandwidthUsed%10
         << " kb/s");
  if (!connection.SetBandwidthUsed(bandwidthUsed, bandwidth)) {
    bandwidthUsed = 0;
    return FALSE;
  }

  bandwidthUsed = bandwidth;
  return TRUE;
}


BOOL H323Channel::Open()
{
  if (opened)
    return TRUE;

  // Give the connection (or endpoint) a chance to do something with
  // the opening of the codec.
  if (!connection.OnStartLogicalChannel(*this)) {
    PTRACE(1, "LogChan\t" << (GetDirection() == IsReceiver ? "Receive" : "Transmit")
           << " open failed (OnStartLogicalChannel fail)");
    return FALSE;
  }

  opened = TRUE;
  return TRUE;
}


/////////////////////////////////////////////////////////////////////////////

H323UnidirectionalChannel::H323UnidirectionalChannel(H323Connection & conn,
                                                     const H323Capability & cap,
                                                     Directions direction)
  : H323Channel(conn, cap),
    receiver(direction == IsReceiver)
{
  mediaStream = NULL;
}

H323UnidirectionalChannel::~H323UnidirectionalChannel()
{
  if (!connection.RemoveMediaStream(mediaStream)) {
    delete mediaStream;
    mediaStream = NULL;
  }
}


H323Channel::Directions H323UnidirectionalChannel::GetDirection() const
{
  return receiver ? IsReceiver : IsTransmitter;
}


BOOL H323UnidirectionalChannel::SetInitialBandwidth()
{
  return SetBandwidthUsed(capability->GetMediaFormat().GetBandwidth()/100);
}


BOOL H323UnidirectionalChannel::Open()
{
  if (opened)
    return TRUE;

  if (PAssertNULL(mediaStream) == NULL)
    return FALSE;

  if (!mediaStream->Open()) {
    PTRACE(1, "LogChan\t" << (GetDirection() == IsReceiver ? "Receive" : "Transmit")
           << " open failed (OpalMediaStream::Open fail)");
    return FALSE;
  }

  if (!H323Channel::Open())
    return FALSE;

  if (!mediaStream->IsSource())
    return TRUE;

  return connection.OnOpenMediaStream(*mediaStream);
}


BOOL H323UnidirectionalChannel::Start()
{
  if (!Open())
    return FALSE;

  if (!mediaStream->Start())
    return FALSE;

  //mediaStream->SetCommandNotifier(PCREATE_NOTIFIER(OnMediaCommand));  // TODO: HERE

  paused = FALSE;
  return TRUE;
}


void H323UnidirectionalChannel::Close()
{
  if (terminating)
    return;

  PTRACE(4, "H323RTP\tCleaning up media stream on " << number);

  // If we have source media stream close it
  if (mediaStream != NULL)
    mediaStream->Close();

  H323Channel::Close();
}


void H323UnidirectionalChannel::OnMiscellaneousCommand(const H245_MiscellaneousCommand_type & type)
{
  H323Channel::OnMiscellaneousCommand(type);

  if (mediaStream == NULL)
    return;

#if OPAL_VIDEO
  switch (type.GetTag())
  {
    case H245_MiscellaneousCommand_type::e_videoFreezePicture :
      mediaStream->ExecuteCommand(OpalVideoFreezePicture());
      break;

    case H245_MiscellaneousCommand_type::e_videoFastUpdatePicture:
      mediaStream->ExecuteCommand(OpalVideoUpdatePicture());
      break;

    case H245_MiscellaneousCommand_type::e_videoFastUpdateGOB :
    {
      const H245_MiscellaneousCommand_type_videoFastUpdateGOB & fuGOB = type;
      mediaStream->ExecuteCommand(OpalVideoUpdatePicture(fuGOB.m_firstGOB, -1, fuGOB.m_numberOfGOBs));
    }
    break;

    case H245_MiscellaneousCommand_type::e_videoFastUpdateMB :
    {
      const H245_MiscellaneousCommand_type_videoFastUpdateMB & vfuMB = type;
      mediaStream->ExecuteCommand(OpalVideoUpdatePicture(vfuMB.HasOptionalField(H245_MiscellaneousCommand_type_videoFastUpdateMB::e_firstGOB) ? (int)vfuMB.m_firstGOB : -1,
                                                         vfuMB.HasOptionalField(H245_MiscellaneousCommand_type_videoFastUpdateMB::e_firstMB)  ? (int)vfuMB.m_firstMB  : -1,
                                                         vfuMB.m_numberOfMBs));
    }
    break;

    case H245_MiscellaneousCommand_type::e_videoTemporalSpatialTradeOff :
      mediaStream->ExecuteCommand(OpalTemporalSpatialTradeOff((const PASN_Integer &)type));
      break;
    default:
      break;
  }
#endif
}


void H323UnidirectionalChannel::OnMediaCommand(
#if OPAL_VIDEO
  OpalMediaCommand & command, 
#else
  OpalMediaCommand & , 
#endif
  INT)
{
#if OPAL_VIDEO
  if (PIsDescendant(&command, OpalVideoUpdatePicture)) {
    H323ControlPDU pdu;
    const OpalVideoUpdatePicture & updatePicture = (const OpalVideoUpdatePicture &)command;

    if (updatePicture.GetNumBlocks() < 0)
      pdu.BuildMiscellaneousCommand(GetNumber(), H245_MiscellaneousCommand_type::e_videoFastUpdatePicture);
    else if (updatePicture.GetFirstMB() < 0) {
      H245_MiscellaneousCommand_type_videoFastUpdateGOB & fuGOB = 
            pdu.BuildMiscellaneousCommand(GetNumber(), H245_MiscellaneousCommand_type::e_videoFastUpdateGOB).m_type;
      fuGOB.m_firstGOB = updatePicture.GetFirstGOB();
      fuGOB.m_numberOfGOBs = updatePicture.GetNumBlocks();
    }
    else {
      H245_MiscellaneousCommand_type_videoFastUpdateMB & vfuMB =
            pdu.BuildMiscellaneousCommand(GetNumber(), H245_MiscellaneousCommand_type::e_videoFastUpdateMB).m_type;
      if (updatePicture.GetFirstGOB() >= 0) {
        vfuMB.IncludeOptionalField(H245_MiscellaneousCommand_type_videoFastUpdateMB::e_firstGOB);
        vfuMB.m_firstGOB = updatePicture.GetFirstGOB();
      }
      if (updatePicture.GetFirstMB() >= 0) {
        vfuMB.IncludeOptionalField(H245_MiscellaneousCommand_type_videoFastUpdateMB::e_firstMB);
        vfuMB.m_firstMB = updatePicture.GetFirstMB();
      }
      vfuMB.m_numberOfMBs = updatePicture.GetNumBlocks();
    }

    connection.WriteControlPDU(pdu);
    return;
  }
#endif
}


OpalMediaStream * H323UnidirectionalChannel::GetMediaStream(BOOL deleted) const
{
  OpalMediaStream * t = mediaStream;
  if (deleted)
    mediaStream = NULL;
  return t;
}


/////////////////////////////////////////////////////////////////////////////

H323BidirectionalChannel::H323BidirectionalChannel(H323Connection & conn,
                                                   const H323Capability & cap)
  : H323Channel(conn, cap)
{
}


H323Channel::Directions H323BidirectionalChannel::GetDirection() const
{
  return IsBidirectional;
}


BOOL H323BidirectionalChannel::Start()
{
  return TRUE;
}


/////////////////////////////////////////////////////////////////////////////

H323_RealTimeChannel::H323_RealTimeChannel(H323Connection & connection,
                                           const H323Capability & capability,
                                           Directions direction)
  : H323UnidirectionalChannel(connection, capability, direction)
{
  rtpPayloadType = capability.GetMediaFormat().GetPayloadType();
}


BOOL H323_RealTimeChannel::OnSendingPDU(H245_OpenLogicalChannel & open) const
{
  PTRACE(3, "H323RTP\tOnSendingPDU");

  open.m_forwardLogicalChannelNumber = (unsigned)number;

  if (open.HasOptionalField(H245_OpenLogicalChannel::e_reverseLogicalChannelParameters)) {
    open.m_reverseLogicalChannelParameters.IncludeOptionalField(
            H245_OpenLogicalChannel_reverseLogicalChannelParameters::e_multiplexParameters);
    // Set the communications information for unicast IPv4
    open.m_reverseLogicalChannelParameters.m_multiplexParameters.SetTag(
                H245_OpenLogicalChannel_reverseLogicalChannelParameters_multiplexParameters
                    ::e_h2250LogicalChannelParameters);

    return OnSendingPDU(open.m_reverseLogicalChannelParameters.m_multiplexParameters);
  }
  else {
    // Set the communications information for unicast IPv4
    open.m_forwardLogicalChannelParameters.m_multiplexParameters.SetTag(
                H245_OpenLogicalChannel_forwardLogicalChannelParameters_multiplexParameters
                    ::e_h2250LogicalChannelParameters);

    return OnSendingPDU(open.m_forwardLogicalChannelParameters.m_multiplexParameters);
  }
}


void H323_RealTimeChannel::OnSendOpenAck(const H245_OpenLogicalChannel & open,
                                         H245_OpenLogicalChannelAck & ack) const
{
  PTRACE(3, "H323RTP\tOnSendOpenAck");

  // set forwardMultiplexAckParameters option
  ack.IncludeOptionalField(H245_OpenLogicalChannelAck::e_forwardMultiplexAckParameters);

  // select H225 choice
  ack.m_forwardMultiplexAckParameters.SetTag(
	  H245_OpenLogicalChannelAck_forwardMultiplexAckParameters::e_h2250LogicalChannelAckParameters);

  // get H225 parms
  H245_H2250LogicalChannelAckParameters & param = ack.m_forwardMultiplexAckParameters;

  // set session ID
  param.IncludeOptionalField(H245_H2250LogicalChannelAckParameters::e_sessionID);
  const H245_H2250LogicalChannelParameters & openparam =
                          open.m_forwardLogicalChannelParameters.m_multiplexParameters;
  unsigned sessionID = openparam.m_sessionID;
  param.m_sessionID = sessionID;

  OnSendOpenAck(param);

  PTRACE(3, "H323RTP\tSending open logical channel ACK: sessionID=" << sessionID);
}


BOOL H323_RealTimeChannel::OnReceivedPDU(const H245_OpenLogicalChannel & open,
                                         unsigned & errorCode)
{
  if (receiver)
    number = H323ChannelNumber(open.m_forwardLogicalChannelNumber, TRUE);

  PTRACE(3, "H323RTP\tOnReceivedPDU for channel: " << number);

  BOOL reverse = open.HasOptionalField(H245_OpenLogicalChannel::e_reverseLogicalChannelParameters);
  const H245_DataType & dataType = reverse ? open.m_reverseLogicalChannelParameters.m_dataType
                                           : open.m_forwardLogicalChannelParameters.m_dataType;

  if (!capability->OnReceivedPDU(dataType, receiver)) {
    PTRACE(1, "H323RTP\tData type not supported");
    errorCode = H245_OpenLogicalChannelReject_cause::e_dataTypeNotSupported;
    return FALSE;
  }

  if (reverse) {
    if (open.m_reverseLogicalChannelParameters.m_multiplexParameters.GetTag() ==
             H245_OpenLogicalChannel_reverseLogicalChannelParameters_multiplexParameters::e_h2250LogicalChannelParameters)
      return OnReceivedPDU(open.m_reverseLogicalChannelParameters.m_multiplexParameters, errorCode);
  }
  else {
    if (open.m_forwardLogicalChannelParameters.m_multiplexParameters.GetTag() ==
             H245_OpenLogicalChannel_forwardLogicalChannelParameters_multiplexParameters::e_h2250LogicalChannelParameters)
      return OnReceivedPDU(open.m_forwardLogicalChannelParameters.m_multiplexParameters, errorCode);
  }

  PTRACE(1, "H323RTP\tOnly H.225.0 multiplex supported");
  errorCode = H245_OpenLogicalChannelReject_cause::e_unsuitableReverseParameters;
  return FALSE;
}


BOOL H323_RealTimeChannel::OnReceivedAckPDU(const H245_OpenLogicalChannelAck & ack)
{
  PTRACE(3, "H323RTP\tOnReceiveOpenAck");

  if (!ack.HasOptionalField(H245_OpenLogicalChannelAck::e_forwardMultiplexAckParameters)) {
    PTRACE(1, "H323RTP\tNo forwardMultiplexAckParameters");
    return FALSE;
  }

  if (ack.m_forwardMultiplexAckParameters.GetTag() !=
            H245_OpenLogicalChannelAck_forwardMultiplexAckParameters::e_h2250LogicalChannelAckParameters) {
    PTRACE(1, "H323RTP\tOnly H.225.0 multiplex supported");
    return FALSE;
  }

  return OnReceivedAckPDU(ack.m_forwardMultiplexAckParameters);
}


BOOL H323_RealTimeChannel::SetDynamicRTPPayloadType(int newType)
{
  PTRACE(4, "H323RTP\tAttempting to set dynamic RTP payload type: " << newType);

  // This is "no change"
  if (newType == -1)
    return TRUE;

  // Check for illegal type
  if (newType < RTP_DataFrame::DynamicBase || newType >= RTP_DataFrame::IllegalPayloadType)
    return FALSE;

  // Check for overwriting "known" type
  if (rtpPayloadType < RTP_DataFrame::DynamicBase)
    return FALSE;

  rtpPayloadType = (RTP_DataFrame::PayloadTypes)newType;
  PTRACE(3, "H323RTP\tSet dynamic payload type to " << rtpPayloadType);
  return TRUE;
}


/////////////////////////////////////////////////////////////////////////////

H323_RTPChannel::H323_RTPChannel(H323Connection & conn,
                                 const H323Capability & cap,
                                 Directions direction,
                                 RTP_Session & r)
  : H323_RealTimeChannel(conn, cap, direction),
    rtpSession(r),
    rtpCallbacks(*(H323_RTP_Session *)r.GetUserData())
{
  // If we are the receiver of RTP data then we create a source media stream
  mediaStream = conn.CreateMediaStream(capability->GetMediaFormat(), GetSessionID(), receiver);
  PTRACE(3, "H323RTP\t" << (receiver ? "Receiver" : "Transmitter")
         << " created using session " << GetSessionID());
}


H323_RTPChannel::~H323_RTPChannel()
{
  // Finished with the RTP session, this will delete the session if it is no
  // longer referenced by any logical channels.
  connection.ReleaseSession(GetSessionID());
}


unsigned H323_RTPChannel::GetSessionID() const
{
  return rtpSession.GetSessionID();
}


BOOL H323_RTPChannel::OnSendingPDU(H245_H2250LogicalChannelParameters & param) const
{
  return rtpCallbacks.OnSendingPDU(*this, param);
}


void H323_RTPChannel::OnSendOpenAck(H245_H2250LogicalChannelAckParameters & param) const
{
  rtpCallbacks.OnSendingAckPDU(*this, param);
}


BOOL H323_RTPChannel::OnReceivedPDU(const H245_H2250LogicalChannelParameters & param,
                                    unsigned & errorCode)
{
  return rtpCallbacks.OnReceivedPDU(*this, param, errorCode);
}


BOOL H323_RTPChannel::OnReceivedAckPDU(const H245_H2250LogicalChannelAckParameters & param)
{
  return rtpCallbacks.OnReceivedAckPDU(*this, param);
}


/////////////////////////////////////////////////////////////////////////////

H323_ExternalRTPChannel::H323_ExternalRTPChannel(H323Connection & connection,
                                                 const H323Capability & capability,
                                                 Directions direction,
                                                 unsigned id)
  : H323_RealTimeChannel(connection, capability, direction)
{
  Construct(connection, id);
}


H323_ExternalRTPChannel::H323_ExternalRTPChannel(H323Connection & connection,
                                                 const H323Capability & capability,
                                                 Directions direction,
                                                 unsigned id,
                                                 const H323TransportAddress & data,
                                                 const H323TransportAddress & control)
  : H323_RealTimeChannel(connection, capability, direction),
    externalMediaAddress(data),
    externalMediaControlAddress(control)
{
  Construct(connection, id);
}


H323_ExternalRTPChannel::H323_ExternalRTPChannel(H323Connection & connection,
                                                 const H323Capability & capability,
                                                 Directions direction,
                                                 unsigned id,
                                                 const PIPSocket::Address & ip,
                                                 WORD dataPort)
  : H323_RealTimeChannel(connection, capability, direction),
    externalMediaAddress(ip, dataPort),
    externalMediaControlAddress(ip, (WORD)(dataPort+1))
{
  Construct(connection, id);
}

void H323_ExternalRTPChannel::Construct(H323Connection & conn, unsigned id)
{
  mediaStream = new OpalNullMediaStream(conn, capability->GetMediaFormat(), id, receiver);
  sessionID = id;

  PTRACE(3, "H323RTP\tExternal " << (receiver ? "receiver" : "transmitter")
         << " created using session " << GetSessionID());
}


unsigned H323_ExternalRTPChannel::GetSessionID() const
{
  return sessionID;
}


BOOL H323_ExternalRTPChannel::GetMediaTransportAddress(OpalTransportAddress & data,
                                                       OpalTransportAddress & control) const
{
  data = remoteMediaAddress;
  control = remoteMediaControlAddress;

  if (data.IsEmpty() && control.IsEmpty())
    return FALSE;

  PIPSocket::Address ip;
  WORD port;
  if (data.IsEmpty()) {
    if (control.GetIpAndPort(ip, port))
      data = OpalTransportAddress(ip, (WORD)(port-1));
  }
  else if (control.IsEmpty()) {
    if (data.GetIpAndPort(ip, port))
      control = OpalTransportAddress(ip, (WORD)(port+1));
  }

  return TRUE;
}


BOOL H323_ExternalRTPChannel::Start()
{
  PSafePtr<OpalConnection> otherParty = connection.GetCall().GetOtherPartyConnection(connection);
  if (otherParty == NULL)
    return FALSE;

  OpalConnection::MediaInformation info;
  if (!otherParty->GetMediaInformation(sessionID, info))
    return FALSE;

  externalMediaAddress = info.data;
  externalMediaControlAddress = info.control;
  return Open();
}


void H323_ExternalRTPChannel::Receive()
{
  // Do nothing
}


void H323_ExternalRTPChannel::Transmit()
{
  // Do nothing
}


BOOL H323_ExternalRTPChannel::OnSendingPDU(H245_H2250LogicalChannelParameters & param) const
{
  param.m_sessionID = sessionID;

  param.IncludeOptionalField(H245_H2250LogicalChannelParameters::e_mediaGuaranteedDelivery);
  param.m_mediaGuaranteedDelivery = FALSE;

  param.IncludeOptionalField(H245_H2250LogicalChannelParameters::e_silenceSuppression);
  param.m_silenceSuppression = FALSE;

  // unicast must have mediaControlChannel
  param.IncludeOptionalField(H245_H2250LogicalChannelParameters::e_mediaControlChannel);
  externalMediaControlAddress.SetPDU(param.m_mediaControlChannel);

  if (receiver) {
    // set mediaChannel
    param.IncludeOptionalField(H245_H2250LogicalChannelAckParameters::e_mediaChannel);
    externalMediaAddress.SetPDU(param.m_mediaChannel);
  }

  return TRUE;
}


void H323_ExternalRTPChannel::OnSendOpenAck(H245_H2250LogicalChannelAckParameters & param) const
{
  // set mediaControlChannel
  param.IncludeOptionalField(H245_H2250LogicalChannelAckParameters::e_mediaControlChannel);
  externalMediaControlAddress.SetPDU(param.m_mediaControlChannel);

  // set mediaChannel
  param.IncludeOptionalField(H245_H2250LogicalChannelAckParameters::e_mediaChannel);
  externalMediaAddress.SetPDU(param.m_mediaChannel);
}


BOOL H323_ExternalRTPChannel::OnReceivedPDU(const H245_H2250LogicalChannelParameters & param,
                                          unsigned & errorCode)
{
  // Only support a single audio session
  if (param.m_sessionID != sessionID) {
    PTRACE(1, "LogChan\tOpen for invalid session: " << param.m_sessionID);
    errorCode = H245_OpenLogicalChannelReject_cause::e_invalidSessionID;
    return FALSE;
  }

  if (!param.HasOptionalField(H245_H2250LogicalChannelParameters::e_mediaControlChannel)) {
    PTRACE(1, "LogChan\tNo mediaControlChannel specified");
    errorCode = H245_OpenLogicalChannelReject_cause::e_unspecified;
    return FALSE;
  }

  remoteMediaControlAddress = param.m_mediaControlChannel;
  if (remoteMediaControlAddress.IsEmpty())
    return FALSE;

  if (param.HasOptionalField(H245_H2250LogicalChannelParameters::e_mediaChannel)) {
    remoteMediaAddress = param.m_mediaChannel;
    if (remoteMediaAddress.IsEmpty())
      return FALSE;
  }
  else {
    PIPSocket::Address addr;
    WORD port;
    if (!remoteMediaControlAddress.GetIpAndPort(addr, port))
      return FALSE;
    remoteMediaAddress = OpalTransportAddress(addr, port-1);
  }

  unsigned id = param.m_sessionID;
  if (!remoteMediaAddress.IsEmpty() && (connection.GetMediaTransportAddresses().GetAt(id) == NULL))
    connection.GetMediaTransportAddresses().SetAt(id, new OpalTransportAddress(remoteMediaAddress));

  return TRUE;
}


BOOL H323_ExternalRTPChannel::OnReceivedAckPDU(const H245_H2250LogicalChannelAckParameters & param)
{
   if (param.HasOptionalField(H245_H2250LogicalChannelAckParameters::e_sessionID) && (param.m_sessionID != sessionID)) {
     PTRACE(2, "LogChan\tAck for invalid session: " << param.m_sessionID);
  }


  if (!param.HasOptionalField(H245_H2250LogicalChannelAckParameters::e_mediaControlChannel)) {
    PTRACE(1, "LogChan\tNo mediaControlChannel specified");
    return FALSE;
  }

  remoteMediaControlAddress = param.m_mediaControlChannel;
  if (remoteMediaControlAddress.IsEmpty())
    return FALSE;

  if (!param.HasOptionalField(H245_H2250LogicalChannelAckParameters::e_mediaChannel)) {
    PTRACE(1, "LogChan\tNo mediaChannel specified");
    return FALSE;
  }

  remoteMediaAddress = param.m_mediaChannel;
  if (remoteMediaAddress.IsEmpty())
    return FALSE;

  unsigned id = param.m_sessionID;
  if (!remoteMediaAddress.IsEmpty() && (connection.GetMediaTransportAddresses().GetAt(id) == NULL))
    connection.GetMediaTransportAddresses().SetAt(id, new OpalTransportAddress(remoteMediaAddress));

  return TRUE;
}


void H323_ExternalRTPChannel::SetExternalAddress(const H323TransportAddress & data,
                                                 const H323TransportAddress & control)
{
  externalMediaAddress = data;
  externalMediaControlAddress = control;

  if (data.IsEmpty() || control.IsEmpty()) {
    PIPSocket::Address ip;
    WORD port;
    if (data.GetIpAndPort(ip, port))
      externalMediaControlAddress = H323TransportAddress(ip, (WORD)(port+1));
    else if (control.GetIpAndPort(ip, port))
      externalMediaAddress = H323TransportAddress(ip, (WORD)(port-1));
  }
}


BOOL H323_ExternalRTPChannel::GetRemoteAddress(PIPSocket::Address & ip,
                                               WORD & dataPort) const
{
  if (!remoteMediaControlAddress) {
    if (remoteMediaControlAddress.GetIpAndPort(ip, dataPort)) {
      dataPort--;
      return TRUE;
    }
  }

  if (!remoteMediaAddress)
    return remoteMediaAddress.GetIpAndPort(ip, dataPort);

  return FALSE;
}


/////////////////////////////////////////////////////////////////////////////

H323DataChannel::H323DataChannel(H323Connection & conn,
                                 const H323Capability & cap,
                                 Directions dir,
                                 unsigned id)
  : H323UnidirectionalChannel(conn, cap, dir)
{
  sessionID = id;
  listener = NULL;
  autoDeleteListener = TRUE;
  transport = NULL;
  autoDeleteTransport = TRUE;
  separateReverseChannel = FALSE;
}


H323DataChannel::~H323DataChannel()
{
  if (autoDeleteListener)
    delete listener;
  if (autoDeleteTransport)
    delete transport;
}


void H323DataChannel::Close()
{
  if (terminating)
    return;

  PTRACE(4, "LogChan\tCleaning up data channel " << number);

  // Break any I/O blocks and wait for the thread that uses this object to
  // terminate before we allow it to be deleted.
  if (listener != NULL)
    listener->Close();
  if (transport != NULL)
    transport->Close();

  H323UnidirectionalChannel::Close();
}


unsigned H323DataChannel::GetSessionID() const
{
  return sessionID;
}


BOOL H323DataChannel::OnSendingPDU(H245_OpenLogicalChannel & open) const
{
  PTRACE(3, "LogChan\tOnSendingPDU for channel: " << number);

  open.m_forwardLogicalChannelNumber = (unsigned)number;

  open.m_forwardLogicalChannelParameters.m_multiplexParameters.SetTag(
              H245_OpenLogicalChannel_forwardLogicalChannelParameters_multiplexParameters
                  ::e_h2250LogicalChannelParameters);
  H245_H2250LogicalChannelParameters & fparam = open.m_forwardLogicalChannelParameters.m_multiplexParameters;
  fparam.m_sessionID = GetSessionID();

  if (separateReverseChannel)
    return TRUE;

  open.IncludeOptionalField(H245_OpenLogicalChannel::e_reverseLogicalChannelParameters);
  open.m_reverseLogicalChannelParameters.IncludeOptionalField(
              H245_OpenLogicalChannel_reverseLogicalChannelParameters::e_multiplexParameters);
  open.m_reverseLogicalChannelParameters.m_multiplexParameters.SetTag(
              H245_OpenLogicalChannel_reverseLogicalChannelParameters_multiplexParameters
                  ::e_h2250LogicalChannelParameters);
  H245_H2250LogicalChannelParameters & rparam = open.m_reverseLogicalChannelParameters.m_multiplexParameters;
  rparam.m_sessionID = GetSessionID();

  return capability->OnSendingPDU(open.m_reverseLogicalChannelParameters.m_dataType);
}


void H323DataChannel::OnSendOpenAck(const H245_OpenLogicalChannel & /*open*/,
                                    H245_OpenLogicalChannelAck & ack) const
{
  if (listener == NULL && transport == NULL) {
    PTRACE(2, "LogChan\tOnSendOpenAck without a listener or transport");
    return;
  }

  PTRACE(3, "LogChan\tOnSendOpenAck for channel: " << number);

  H245_H2250LogicalChannelAckParameters * param;

  if (separateReverseChannel) {
    ack.IncludeOptionalField(H245_OpenLogicalChannelAck::e_forwardMultiplexAckParameters);
    ack.m_forwardMultiplexAckParameters.SetTag(
              H245_OpenLogicalChannelAck_forwardMultiplexAckParameters::e_h2250LogicalChannelAckParameters);
    param = (H245_H2250LogicalChannelAckParameters*)&ack.m_forwardMultiplexAckParameters.GetObject();
  }
  else {
    ack.IncludeOptionalField(H245_OpenLogicalChannelAck::e_reverseLogicalChannelParameters);
    ack.m_reverseLogicalChannelParameters.m_multiplexParameters.SetTag(
              H245_OpenLogicalChannelAck_reverseLogicalChannelParameters_multiplexParameters
                  ::e_h2250LogicalChannelParameters);
    param = (H245_H2250LogicalChannelAckParameters*)
                &ack.m_reverseLogicalChannelParameters.m_multiplexParameters.GetObject();
  }

  unsigned session = GetSessionID();
  if (session != 0) {
    param->IncludeOptionalField(H245_H2250LogicalChannelAckParameters::e_sessionID);
    param->m_sessionID = GetSessionID();
  }

  H323TransportAddress address;
  param->IncludeOptionalField(H245_H2250LogicalChannelAckParameters::e_mediaChannel);
  if (listener != NULL)
    address = listener->GetLocalAddress(connection.GetControlChannel().GetLocalAddress());
  else
    address = transport->GetLocalAddress();

  address.SetPDU(param->m_mediaChannel);
}


BOOL H323DataChannel::OnReceivedPDU(const H245_OpenLogicalChannel & open,
                                    unsigned & errorCode)
{
  number = H323ChannelNumber(open.m_forwardLogicalChannelNumber, TRUE);

  PTRACE(3, "LogChan\tOnReceivedPDU for data channel: " << number);

  if (!CreateListener()) {
    PTRACE(1, "LogChan\tCould not create listener");
    errorCode = H245_OpenLogicalChannelReject_cause::e_unspecified;
    return FALSE;
  }

  if (separateReverseChannel &&
      open.HasOptionalField(H245_OpenLogicalChannel::e_reverseLogicalChannelParameters)) {
    errorCode = H245_OpenLogicalChannelReject_cause::e_unsuitableReverseParameters;
    PTRACE(1, "LogChan\tOnReceivedPDU has unexpected reverse parameters");
    return FALSE;
  }

  if (!capability->OnReceivedPDU(open.m_forwardLogicalChannelParameters.m_dataType, receiver)) {
    PTRACE(1, "H323RTP\tData type not supported");
    errorCode = H245_OpenLogicalChannelReject_cause::e_dataTypeNotSupported;
    return FALSE;
  }

  return TRUE;
}


BOOL H323DataChannel::OnReceivedAckPDU(const H245_OpenLogicalChannelAck & ack)
{
  PTRACE(3, "LogChan\tOnReceivedAckPDU");

  const H245_TransportAddress * address;

  if (separateReverseChannel) {
      PTRACE(3, "LogChan\tseparateReverseChannels");
    if (!ack.HasOptionalField(H245_OpenLogicalChannelAck::e_forwardMultiplexAckParameters)) {
      PTRACE(1, "LogChan\tNo forwardMultiplexAckParameters");
      return FALSE;
    }

    if (ack.m_forwardMultiplexAckParameters.GetTag() !=
              H245_OpenLogicalChannelAck_forwardMultiplexAckParameters::e_h2250LogicalChannelAckParameters) {
      PTRACE(1, "LogChan\tOnly H.225.0 multiplex supported");
      return FALSE;
    }

    const H245_H2250LogicalChannelAckParameters & param = ack.m_forwardMultiplexAckParameters;

    if (!param.HasOptionalField(H245_H2250LogicalChannelAckParameters::e_mediaChannel)) {
      PTRACE(1, "LogChan\tNo media channel address provided");
      return FALSE;
    }

    address = &param.m_mediaChannel;

    if (ack.HasOptionalField(H245_OpenLogicalChannelAck::e_reverseLogicalChannelParameters)) {
      PTRACE(3, "LogChan\treverseLogicalChannelParameters set");
      reverseChannel = H323ChannelNumber(ack.m_reverseLogicalChannelParameters.m_reverseLogicalChannelNumber, TRUE);
    }
  }
  else {
    if (!ack.HasOptionalField(H245_OpenLogicalChannelAck::e_reverseLogicalChannelParameters)) {
      PTRACE(1, "LogChan\tNo reverseLogicalChannelParameters");
      return FALSE;
    }

    if (ack.m_reverseLogicalChannelParameters.m_multiplexParameters.GetTag() !=
              H245_OpenLogicalChannelAck_reverseLogicalChannelParameters_multiplexParameters
                              ::e_h2250LogicalChannelParameters) {
      PTRACE(1, "LogChan\tOnly H.225.0 multiplex supported");
      return FALSE;
    }

    const H245_H2250LogicalChannelParameters & param = ack.m_reverseLogicalChannelParameters.m_multiplexParameters;

    if (!param.HasOptionalField(H245_H2250LogicalChannelParameters::e_mediaChannel)) {
      PTRACE(1, "LogChan\tNo media channel address provided");
      return FALSE;
    }

    address = &param.m_mediaChannel;
  }

  if (!CreateTransport()) {
    PTRACE(1, "LogChan\tCould not create transport");
    return FALSE;
  }

  if (!transport->ConnectTo(H323TransportAddress(*address))) {
    PTRACE(1, "LogChan\tCould not connect to remote transport address: " << *address);
    return FALSE;
  }

  return TRUE;
}


BOOL H323DataChannel::CreateListener()
{
  if (listener == NULL) {
    listener = connection.GetControlChannel().GetLocalAddress().CreateListener(
                          connection.GetEndPoint(), OpalTransportAddress::HostOnly);
    if (listener == NULL)
      return FALSE;

    PTRACE(3, "LogChan\tCreated listener for data channel: " << *listener);
  }

  return listener->Open(NULL);
}


BOOL H323DataChannel::CreateTransport()
{
  if (transport == NULL) {
    transport = connection.GetControlChannel().GetLocalAddress().CreateTransport(
                          connection.GetEndPoint(), OpalTransportAddress::HostOnly);
    if (transport == NULL)
      return FALSE;

    PTRACE(3, "LogChan\tCreated transport for data channel: " << *transport);
  }

  return transport != NULL;
}


/////////////////////////////////////////////////////////////////////////////
