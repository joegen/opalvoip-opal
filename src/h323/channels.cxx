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
 * $Revision$
 * $Author$
 * $Date$
 */

#include <ptlib.h>

#include <opal/buildopts.h>
#if OPAL_H323

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


#define MAX_PAYLOAD_TYPE_MISMATCHES 8
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

H323ChannelNumber::H323ChannelNumber(unsigned num, PBoolean fromRem)
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
  opened = PFalse;
  paused = PTrue;
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


bool H323Channel::SetSessionID(unsigned /*sessionID*/)
{
  return true;
}


PBoolean H323Channel::GetMediaTransportAddress(OpalTransportAddress & /*data*/,
                                               OpalTransportAddress & /*control*/) const
{
  return PFalse;
}


void H323Channel::Close()
{
  if (opened && m_terminating++ == 0)
    InternalClose();
}


void H323Channel::InternalClose()
{
  // Signal to the connection that this channel is on the way out
  connection.OnClosedLogicalChannel(*this);

  PTRACE(4, "LogChan\tCleaned up " << number);
}


OpalMediaStreamPtr H323Channel::GetMediaStream() const
{
  return NULL;
}


void H323Channel::SetMediaStream(OpalMediaStreamPtr)
{
}


PBoolean H323Channel::OnReceivedPDU(const H245_OpenLogicalChannel & /*pdu*/,
                                unsigned & /*errorCode*/)
{
  return PTrue;
}


PBoolean H323Channel::OnReceivedAckPDU(const H245_OpenLogicalChannelAck & /*pdu*/)
{
  return PTrue;
}


void H323Channel::OnSendOpenAck(const H245_OpenLogicalChannel & /*pdu*/,
                                H245_OpenLogicalChannelAck & /* pdu*/) const
{
}


void H323Channel::OnFlowControl(long bitRateRestriction)
{
  PTRACE(3, "LogChan\tOnFlowControl: " << bitRateRestriction);

  OpalMediaStreamPtr stream = GetMediaStream();
  if (stream != NULL) {
    long bitsPerSecond = bitRateRestriction*100;
    OpalMediaFormat format = stream->GetMediaFormat();
    format.SetOptionInteger(OpalMediaFormat::MaxBitRateOption(), bitsPerSecond);
    if (format.GetOptionInteger(OpalMediaFormat::TargetBitRateOption()) > bitsPerSecond)
      format.SetOptionInteger(OpalMediaFormat::TargetBitRateOption(), bitsPerSecond);
    stream->UpdateMediaFormat(format);
  }
}


void H323Channel::OnMiscellaneousCommand(const H245_MiscellaneousCommand_type & type)
{
  PTRACE(3, "LogChan\tOnMiscellaneousCommand: chan=" << number << ", type=" << type.GetTagName());
  OpalMediaStreamPtr mediaStream = GetMediaStream();
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
    case H245_MiscellaneousCommand_type::e_videoFastUpdateMB :
      mediaStream->ExecuteCommand(OpalVideoPictureLoss());
      break;

    case H245_MiscellaneousCommand_type::e_videoTemporalSpatialTradeOff :
      mediaStream->ExecuteCommand(OpalTemporalSpatialTradeOff((const PASN_Integer &)type));
      break;

    default:
      break;
  }
#endif
}


bool H323Channel::OnMediaCommand(const OpalMediaCommand & command)
{
#if OPAL_VIDEO
  if (PIsDescendant(&command, OpalVideoUpdatePicture)) {
    H323ControlPDU pdu;
    pdu.BuildMiscellaneousCommand(GetNumber(), H245_MiscellaneousCommand_type::e_videoFastUpdatePicture);
    connection.WriteControlPDU(pdu);
    return true;
  }
#endif
  return false;
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


PBoolean H323Channel::SetBandwidthUsed(unsigned bandwidth)
{
  PTRACE(3, "LogChan\tBandwidth requested/used = "
         << bandwidth/10 << '.' << bandwidth%10 << '/'
         << bandwidthUsed/10 << '.' << bandwidthUsed%10
         << " kb/s");
  if (!connection.SetBandwidthUsed(bandwidthUsed, bandwidth)) {
    bandwidthUsed = 0;
    return PFalse;
  }

  bandwidthUsed = bandwidth;
  return PTrue;
}


PBoolean H323Channel::Open()
{
  if (opened)
    return PTrue;

  // Give the connection (or endpoint) a chance to do something with
  // the opening of the codec.
  if (!connection.OnStartLogicalChannel(*this)) {
    PTRACE(1, "LogChan\t" << (GetDirection() == IsReceiver ? "Receive" : "Transmit")
           << " open failed (OnStartLogicalChannel fail)");
    return PFalse;
  }

  opened = PTrue;
  return PTrue;
}


/////////////////////////////////////////////////////////////////////////////

H323UnidirectionalChannel::H323UnidirectionalChannel(H323Connection & conn,
                                                     const H323Capability & cap,
                                                     Directions direction)
  : H323Channel(conn, cap)
  , receiver(direction == IsReceiver)
  , m_mediaFormat(capability->GetMediaFormat())
{
}

H323UnidirectionalChannel::~H323UnidirectionalChannel()
{
}


H323Channel::Directions H323UnidirectionalChannel::GetDirection() const
{
  return receiver ? IsReceiver : IsTransmitter;
}


PBoolean H323UnidirectionalChannel::SetInitialBandwidth()
{
  return SetBandwidthUsed(m_mediaFormat.GetBandwidth()/100);
}


PBoolean H323UnidirectionalChannel::Open()
{
  if (opened)
    return true;

  if (m_mediaStream == NULL) {
    m_mediaStream = connection.CreateMediaStream(m_mediaFormat, GetSessionID(), receiver);

    OpalCall & call = connection.GetCall();
    OpalMediaType mediaType = m_mediaFormat.GetMediaType();

    bool ok;
    if (GetDirection() == IsReceiver)
      ok = call.OpenSourceMediaStreams(connection, mediaType, GetSessionID(), m_mediaFormat);
    else {
      PSafePtr<OpalConnection> otherConnection = call.GetOtherPartyConnection(connection);
      ok = otherConnection != NULL && call.OpenSourceMediaStreams(*otherConnection, mediaType, GetSessionID(), m_mediaFormat);
    }

    if (!ok) {
      PTRACE(1, "LogChan\t" << (GetDirection() == IsReceiver ? "Receive" : "Transmit")
             << " open failed (OpalMediaStream::Open fail)");
      return false;
    }
  }

  m_mediaFormat = m_mediaStream->GetMediaFormat();
  capability->UpdateMediaFormat(m_mediaFormat);
  return H323Channel::Open();
}


PBoolean H323UnidirectionalChannel::Start()
{
  if (!Open())
    return PFalse;

  if (m_mediaStream == NULL || !m_mediaStream->Start())
    return PFalse;

  paused = PFalse;
  return PTrue;
}


void H323UnidirectionalChannel::InternalClose()
{
  PTRACE(4, "H323RTP\tCleaning up media stream on " << number);

  // If we have source media stream close it
  if (m_mediaStream != NULL) {
    connection.CloseMediaStream(*m_mediaStream);
    connection.RemoveMediaStream(*m_mediaStream);
    m_mediaStream.SetNULL();
  }

  H323Channel::InternalClose();
}


OpalMediaStreamPtr H323UnidirectionalChannel::GetMediaStream() const
{
  return m_mediaStream;
}


void H323UnidirectionalChannel::SetMediaStream(OpalMediaStreamPtr mediaStream)
{
  m_mediaStream = mediaStream;
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


PBoolean H323BidirectionalChannel::Start()
{
  return PTrue;
}


/////////////////////////////////////////////////////////////////////////////

H323_RealTimeChannel::H323_RealTimeChannel(H323Connection & connection,
                                           const H323Capability & capability,
                                           Directions direction)
  : H323UnidirectionalChannel(connection, capability, direction)
{
  rtpPayloadType = capability.GetMediaFormat().GetPayloadType();
}


PBoolean H323_RealTimeChannel::OnSendingPDU(H245_OpenLogicalChannel & open) const
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

    // NOTE Two callbacks?  This may be supposed to fit somewhere else
    connection.OnSendH245_OpenLogicalChannel(open, PFalse);
    return OnSendingPDU(open.m_reverseLogicalChannelParameters.m_multiplexParameters);
  }
  else {
    // Set the communications information for unicast IPv4
    open.m_forwardLogicalChannelParameters.m_multiplexParameters.SetTag(
                H245_OpenLogicalChannel_forwardLogicalChannelParameters_multiplexParameters
                    ::e_h2250LogicalChannelParameters);

    if (OnSendingAltPDU(open.m_genericInformation))
        open.IncludeOptionalField(H245_OpenLogicalChannel::e_genericInformation);

    connection.OnSendH245_OpenLogicalChannel(open, PTrue);
    return OnSendingPDU(open.m_forwardLogicalChannelParameters.m_multiplexParameters);
  }
}


PBoolean H323_RealTimeChannel::OnSendingPDU(H245_H2250LogicalChannelParameters & param) const
{
  // If we are master (or a standard ID) then we can set the session ID, otherwise zero
  unsigned sessionID = GetSessionID();
  if (connection.IsH245Master() || sessionID <= H323Capability::DefaultDataSessionID)
    param.m_sessionID = sessionID;

  return true;
}


void H323_RealTimeChannel::OnSendOpenAck(H245_H2250LogicalChannelAckParameters & /*param*/) const
{
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


PBoolean H323_RealTimeChannel::OnReceivedPDU(const H245_OpenLogicalChannel & open,
                                         unsigned & errorCode)
{
  if (receiver)
    number = H323ChannelNumber(open.m_forwardLogicalChannelNumber, PTrue);

  PTRACE(3, "H323RTP\tOnReceivedPDU for channel: " << number);

  PBoolean reverse = open.HasOptionalField(H245_OpenLogicalChannel::e_reverseLogicalChannelParameters);
  const H245_DataType & dataType = reverse ? open.m_reverseLogicalChannelParameters.m_dataType
                                           : open.m_forwardLogicalChannelParameters.m_dataType;

  if (!capability->OnReceivedPDU(dataType, receiver)) {
    PTRACE(1, "H323RTP\tData type not supported");
    errorCode = H245_OpenLogicalChannelReject_cause::e_dataTypeNotSupported;
    return PFalse;
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
  return PFalse;
}


PBoolean H323_RealTimeChannel::OnReceivedPDU(const H245_H2250LogicalChannelParameters & param,
                                          unsigned & errorCode)
{
  unsigned sessionID = param.m_sessionID;

  if (connection.IsH245Master()) {
    if (sessionID == 0)
      return true;
  }
  else {
    if (sessionID != 0)
      SetSessionID(sessionID);
  }

  if (sessionID != GetSessionID()) {
    PTRACE(1, "H323RTP\tOpen of " << *this << " with invalid session: " << param.m_sessionID);
    errorCode = H245_OpenLogicalChannelReject_cause::e_invalidSessionID;
    return false;
  }

  return true;
}


PBoolean H323_RealTimeChannel::OnReceivedAckPDU(const H245_OpenLogicalChannelAck & ack)
{
  PTRACE(3, "H323RTP\tOnReceiveOpenAck");

  if (!ack.HasOptionalField(H245_OpenLogicalChannelAck::e_forwardMultiplexAckParameters)) {
    PTRACE(1, "H323RTP\tNo forwardMultiplexAckParameters");
    return PFalse;
  }

  if (ack.m_forwardMultiplexAckParameters.GetTag() !=
            H245_OpenLogicalChannelAck_forwardMultiplexAckParameters::e_h2250LogicalChannelAckParameters) {
    PTRACE(1, "H323RTP\tOnly H.225.0 multiplex supported");
    return PFalse;
  }

  if (ack.HasOptionalField(H245_OpenLogicalChannel::e_genericInformation))
    OnReceivedAckAltPDU(ack.m_genericInformation);

  return OnReceivedAckPDU(ack.m_forwardMultiplexAckParameters);
}


PBoolean H323_RealTimeChannel::OnReceivedAckPDU(const H245_H2250LogicalChannelAckParameters & param)
{
  unsigned sessionID = 0;
  if (param.HasOptionalField(H245_H2250LogicalChannelAckParameters::e_sessionID))
    sessionID = param.m_sessionID;

  if (connection.IsH245Master() || sessionID <= H323Capability::DefaultDataSessionID) {
    PTRACE_IF(2, sessionID != 0 && sessionID != GetSessionID(),
              "LogChan\tAck contains invalid session ID " << param.m_sessionID << ", ignoring");
    return true;
  }

  if (sessionID == 0) {
    PTRACE(2, "LogChan\tNo session ID in ACK from master!");
    return false;
  }

  return SetSessionID(sessionID);
}


PBoolean H323_RealTimeChannel::SetDynamicRTPPayloadType(int newType)
{
  PTRACE(4, "H323RTP\tAttempting to set dynamic RTP payload type: " << newType);

  // This is "no change"
  if (newType == -1)
    return PTrue;

  // Check for illegal type
  if (newType < RTP_DataFrame::DynamicBase || newType >= RTP_DataFrame::IllegalPayloadType)
    return PFalse;

  // Check for overwriting "known" type
  if (rtpPayloadType < RTP_DataFrame::DynamicBase)
    return PFalse;

  rtpPayloadType = (RTP_DataFrame::PayloadTypes)newType;

  m_mediaFormat.SetPayloadType(rtpPayloadType);
  if (m_mediaStream != NULL)
    m_mediaStream->UpdateMediaFormat(m_mediaFormat);

  PTRACE(3, "H323RTP\tSet dynamic payload type to " << rtpPayloadType);
  return PTrue;
}


/////////////////////////////////////////////////////////////////////////////

H323_RTPChannel::H323_RTPChannel(H323Connection & conn,
                                 const H323Capability & cap,
                                 Directions direction,
                                 H323SessionPDUHandler & session)
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
  data = rtpSession.GetRemoteMediaAddress();
  control = rtpSession.GetRemoteControlAddress();
  return true;
}


PBoolean H323_RTPChannel::OnSendingPDU(H245_H2250LogicalChannelParameters & param) const
{
  if (!m_session.OnSendingPDU(*this, param))
    return false;

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
  return connection.OnSendingOLCGenericInformation(GetSessionID(), alternate, false);
}


void H323_RTPChannel::OnSendOpenAck(H245_H2250LogicalChannelAckParameters & param) const
{
  m_session.OnSendingAckPDU(*this, param);
  H323_RealTimeChannel::OnSendOpenAck(param);
}


PBoolean H323_RTPChannel::OnReceivedPDU(const H245_H2250LogicalChannelParameters & param,
                                    unsigned & errorCode)
{
  if (!m_session.OnReceivedPDU(*this, param, errorCode))
    return false;

  if (param.HasOptionalField(H245_H2250LogicalChannelParameters::e_dynamicRTPPayloadType))
    SetDynamicRTPPayloadType(param.m_dynamicRTPPayloadType);

  // Update actual media packetization if present
  if (param.HasOptionalField(H245_H2250LogicalChannelParameters::e_mediaPacketization) &&
      param.m_mediaPacketization.GetTag() == H245_H2250LogicalChannelParameters_mediaPacketization::e_rtpPayloadType) {
    m_mediaFormat.SetMediaPacketizations(H323GetRTPPacketization(param.m_mediaPacketization));
    if (m_mediaStream != NULL)
      m_mediaStream->UpdateMediaFormat(m_mediaFormat);
  }

  return H323_RealTimeChannel::OnReceivedPDU(param, errorCode);
}


PBoolean H323_RTPChannel::OnReceivedAckPDU(const H245_H2250LogicalChannelAckParameters & param)
{
  return m_session.OnReceivedAckPDU(*this, param) && H323_RealTimeChannel::OnReceivedAckPDU(param);
}


PBoolean H323_RTPChannel::OnReceivedAckAltPDU(const H245_ArrayOf_GenericInformation & alternate)
{ 
  return connection.OnReceiveOLCGenericInformation(GetSessionID(), alternate);
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
  autoDeleteListener = PTrue;
  transport = NULL;
  autoDeleteTransport = PTrue;
  separateReverseChannel = PFalse;
}


H323DataChannel::~H323DataChannel()
{
  if (autoDeleteListener)
    delete listener;
  if (autoDeleteTransport)
    delete transport;
}


void H323DataChannel::InternalClose()
{
  PTRACE(4, "LogChan\tCleaning up data channel " << number);

  // Break any I/O blocks and wait for the thread that uses this object to
  // terminate before we allow it to be deleted.
  if (listener != NULL)
    listener->Close();
  if (transport != NULL)
    transport->Close();

  H323UnidirectionalChannel::InternalClose();
}


unsigned H323DataChannel::GetSessionID() const
{
  return sessionID;
}


PBoolean H323DataChannel::OnSendingPDU(H245_OpenLogicalChannel & open) const
{
  PTRACE(3, "LogChan\tOnSendingPDU for channel: " << number);

  open.m_forwardLogicalChannelNumber = (unsigned)number;

  open.m_forwardLogicalChannelParameters.m_multiplexParameters.SetTag(
              H245_OpenLogicalChannel_forwardLogicalChannelParameters_multiplexParameters
                  ::e_h2250LogicalChannelParameters);
  H245_H2250LogicalChannelParameters & fparam = open.m_forwardLogicalChannelParameters.m_multiplexParameters;
  fparam.m_sessionID = GetSessionID();

  if (separateReverseChannel)
    return PTrue;

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

  H323TransportAddress address;
  param->IncludeOptionalField(H245_H2250LogicalChannelAckParameters::e_mediaChannel);
  if (listener != NULL)
    address = listener->GetLocalAddress(connection.GetControlChannel().GetRemoteAddress());
  else
    address = transport->GetLocalAddress();

  address.SetPDU(param->m_mediaChannel);
}


PBoolean H323DataChannel::OnReceivedPDU(const H245_OpenLogicalChannel & open,
                                    unsigned & errorCode)
{
  number = H323ChannelNumber(open.m_forwardLogicalChannelNumber, PTrue);

  PTRACE(3, "LogChan\tOnReceivedPDU for data channel: " << number);

  if (!CreateListener()) {
    PTRACE(1, "LogChan\tCould not create listener");
    errorCode = H245_OpenLogicalChannelReject_cause::e_unspecified;
    return PFalse;
  }

  if (separateReverseChannel &&
      open.HasOptionalField(H245_OpenLogicalChannel::e_reverseLogicalChannelParameters)) {
    errorCode = H245_OpenLogicalChannelReject_cause::e_unsuitableReverseParameters;
    PTRACE(1, "LogChan\tOnReceivedPDU has unexpected reverse parameters");
    return PFalse;
  }

  if (!capability->OnReceivedPDU(open.m_forwardLogicalChannelParameters.m_dataType, receiver)) {
    PTRACE(1, "H323RTP\tData type not supported");
    errorCode = H245_OpenLogicalChannelReject_cause::e_dataTypeNotSupported;
    return PFalse;
  }

  return PTrue;
}


PBoolean H323DataChannel::OnReceivedAckPDU(const H245_OpenLogicalChannelAck & ack)
{
  PTRACE(3, "LogChan\tOnReceivedAckPDU");

  const H245_TransportAddress * address;

  if (separateReverseChannel) {
      PTRACE(3, "LogChan\tseparateReverseChannels");
    if (!ack.HasOptionalField(H245_OpenLogicalChannelAck::e_forwardMultiplexAckParameters)) {
      PTRACE(1, "LogChan\tNo forwardMultiplexAckParameters");
      return PFalse;
    }

    if (ack.m_forwardMultiplexAckParameters.GetTag() !=
              H245_OpenLogicalChannelAck_forwardMultiplexAckParameters::e_h2250LogicalChannelAckParameters) {
      PTRACE(1, "LogChan\tOnly H.225.0 multiplex supported");
      return PFalse;
    }

    const H245_H2250LogicalChannelAckParameters & param = ack.m_forwardMultiplexAckParameters;

    if (!param.HasOptionalField(H245_H2250LogicalChannelAckParameters::e_mediaChannel)) {
      PTRACE(1, "LogChan\tNo media channel address provided");
      return PFalse;
    }

    address = &param.m_mediaChannel;

    if (ack.HasOptionalField(H245_OpenLogicalChannelAck::e_reverseLogicalChannelParameters)) {
      PTRACE(3, "LogChan\treverseLogicalChannelParameters set");
      reverseChannel = H323ChannelNumber(ack.m_reverseLogicalChannelParameters.m_reverseLogicalChannelNumber, PTrue);
    }
  }
  else {
    if (!ack.HasOptionalField(H245_OpenLogicalChannelAck::e_reverseLogicalChannelParameters)) {
      PTRACE(1, "LogChan\tNo reverseLogicalChannelParameters");
      return PFalse;
    }

    if (ack.m_reverseLogicalChannelParameters.m_multiplexParameters.GetTag() !=
              H245_OpenLogicalChannelAck_reverseLogicalChannelParameters_multiplexParameters
                              ::e_h2250LogicalChannelParameters) {
      PTRACE(1, "LogChan\tOnly H.225.0 multiplex supported");
      return PFalse;
    }

    const H245_H2250LogicalChannelParameters & param = ack.m_reverseLogicalChannelParameters.m_multiplexParameters;

    if (!param.HasOptionalField(H245_H2250LogicalChannelParameters::e_mediaChannel)) {
      PTRACE(1, "LogChan\tNo media channel address provided");
      return PFalse;
    }

    address = &param.m_mediaChannel;
  }

  if (!CreateTransport()) {
    PTRACE(1, "LogChan\tCould not create transport");
    return PFalse;
  }

  if (!transport->ConnectTo(H323TransportAddress(*address))) {
    PTRACE(1, "LogChan\tCould not connect to remote transport address: " << *address);
    return PFalse;
  }

  return PTrue;
}


PBoolean H323DataChannel::CreateListener()
{
  if (listener == NULL) {
    listener = connection.GetControlChannel().GetLocalAddress().CreateListener(
                          connection.GetEndPoint(), OpalTransportAddress::HostOnly);
    if (listener == NULL)
      return PFalse;

    PTRACE(3, "LogChan\tCreated listener for data channel: " << *listener);
  }

  return listener->Open(NULL);
}


PBoolean H323DataChannel::CreateTransport()
{
  if (transport == NULL) {
    transport = connection.GetControlChannel().GetLocalAddress().CreateTransport(
                          connection.GetEndPoint(), OpalTransportAddress::HostOnly);
    if (transport == NULL)
      return PFalse;

    PTRACE(3, "LogChan\tCreated transport for data channel: " << *transport);
  }

  return transport != NULL;
}


#endif // OPAL_H323

/////////////////////////////////////////////////////////////////////////////
