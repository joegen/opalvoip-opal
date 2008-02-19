/*
 * t38proto.cxx
 *
 * T.38 protocol handler
 *
 * Open Phone Abstraction Library
 *
 * Copyright (c) 1998-2002 Equivalence Pty. Ltd.
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
 * Contributor(s): Vyacheslav Frolov.
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "t38proto.h"
#endif

#include <opal/buildopts.h>

#if OPAL_T38FAX

#include <ptlib/pipechan.h>

#include <t38/t38proto.h>

#include <opal/mediastrm.h>
#include <opal/patch.h>

#include <h323/transaddr.h>
#include <asn/t38.h>

//#define USE_SEQ

#define new PNEW

namespace PWLibStupidLinkerHacks {
  int t38Loader;
};

#define SPANDSP_AUDIO_SIZE    320

static PAtomicInteger faxCallIndex;

const OpalMediaFormat & GetOpalT38()
{
  static const OpalMediaFormat opalT38(
    OPAL_T38,
    OpalMediaFormat::DefaultDataSessionID,
    RTP_DataFrame::IllegalPayloadType,
    "t38",
    PFalse, // No jitter for data
    1440, // 100's bits/sec
    0,
    0,
    0);
  return opalT38;
}

#if OPAL_AUDIO

const OpalFaxAudioFormat & GetOpalPCM16Fax() 
{
  static const OpalFaxAudioFormat opalPCM16Fax(OPAL_PCM16_FAX, RTP_DataFrame::MaxPayloadType, "", 16, 8,  240, 30, 256,  8000);
  return opalPCM16Fax;
}


OpalFaxAudioFormat::OpalFaxAudioFormat(const char * fullName,
                                 RTP_DataFrame::PayloadTypes rtpPayloadType,
                                 const char * encodingName,
                                 PINDEX   frameSize,
                                 unsigned frameTime,
                                 unsigned rxFrames,
                                 unsigned txFrames,
                                 unsigned maxFrames,
				                         unsigned clockRate,
                                   time_t timeStamp)
  : OpalMediaFormat(fullName,
                    OpalMediaFormat::DefaultDataSessionID,
                    rtpPayloadType,
                    encodingName,
                    PTrue,
                    8*frameSize*AudioClockRate/frameTime,  // bits per second = 8*frameSize * framesPerSecond
                    frameSize,
                    frameTime,
                    clockRate,
                    timeStamp)
{
  AddOption(new OpalMediaOptionUnsigned(OpalAudioFormat::RxFramesPerPacketOption(), false, OpalMediaOption::MinMerge, rxFrames, 1, maxFrames));
  AddOption(new OpalMediaOptionUnsigned(OpalAudioFormat::TxFramesPerPacketOption(), false, OpalMediaOption::MinMerge, txFrames, 1, maxFrames));
}

#endif

/////////////////////////////////////////////////////////////////////////////

OpalT38Protocol::OpalT38Protocol()
{
  transport = NULL;
  autoDeleteTransport = PFalse;
  corrigendumASN = PTrue;
  indicatorRedundancy = 0;
  lowSpeedRedundancy = 0;
  highSpeedRedundancy = 0;
  lastSentSequenceNumber = -1;
}


OpalT38Protocol::~OpalT38Protocol()
{
  if (autoDeleteTransport)
    delete transport;
}

void OpalT38Protocol::Close()
{
  transport->Close();
}


void OpalT38Protocol::SetTransport(H323Transport * t, PBoolean autoDelete)
{
  if (transport != t) {
    if (autoDeleteTransport)
      delete transport;

    transport = t;
  }

  autoDeleteTransport = autoDelete;
}


PBoolean OpalT38Protocol::Originate()
{
  PTRACE(3, "T38\tOriginate, transport=" << *transport);

  // Application would normally override this. The default just sends
  // a "heartbeat".
  while (WriteIndicator(T38_Type_of_msg_t30_indicator::e_no_signal))
    PThread::Sleep(500);

  return PFalse;
}


PBoolean OpalT38Protocol::WritePacket(const T38_IFPPacket & ifp)
{
  T38_UDPTLPacket udptl;

#if 0
  // If there are redundant frames saved from last time, put them in
  if (!redundantIFPs.IsEmpty()) {
    udptl.m_error_recovery.SetTag(T38_UDPTLPacket_error_recovery::e_secondary_ifp_packets);
    T38_UDPTLPacket_error_recovery_secondary_ifp_packets & secondary = udptl.m_error_recovery;
    secondary.SetSize(redundantIFPs.GetSize());
    for (PINDEX i = 0; i < redundantIFPs.GetSize(); i++)
      secondary[i].SetValue(redundantIFPs[i]);
  }
#endif

  // Encode the current ifp, but need to do stupid things as there are two
  // versions of the ASN out there, completely incompatible.
  if (corrigendumASN || !ifp.HasOptionalField(T38_IFPPacket::e_data_field))
    udptl.m_primary_ifp_packet.EncodeSubType(ifp);
  else {
    T38_PreCorrigendum_IFPPacket old_ifp;

    old_ifp.m_type_of_msg = ifp.m_type_of_msg;

    old_ifp.IncludeOptionalField(T38_IFPPacket::e_data_field);

    PINDEX count = ifp.m_data_field.GetSize();
    old_ifp.m_data_field.SetSize(count);

    for (PINDEX i = 0 ; i < count; i++) {
      old_ifp.m_data_field[i].m_field_type = ifp.m_data_field[i].m_field_type;
      if (ifp.m_data_field[i].HasOptionalField(T38_Data_Field_subtype::e_field_data)) {
        old_ifp.m_data_field[i].IncludeOptionalField(T38_Data_Field_subtype::e_field_data);
        old_ifp.m_data_field[i].m_field_data = ifp.m_data_field[i].m_field_data;
      }
    }

    udptl.m_primary_ifp_packet.PASN_OctetString::EncodeSubType(old_ifp);
  }

  lastSentSequenceNumber = (lastSentSequenceNumber + 1) & 0xffff;
  udptl.m_seq_number = lastSentSequenceNumber;

  PPER_Stream rawData;
  udptl.Encode(rawData);

#if PTRACING
  if (PTrace::CanTrace(4)) {
    PTRACE(4, "T38\tSending PDU:\n  "
           << setprecision(2) << ifp << "\n "
           << setprecision(2) << udptl << "\n "
           << setprecision(2) << rawData);
  }
  else {
    PTRACE(3, "T38\tSending PDU:"
              " seq=" << lastSentSequenceNumber <<
              " type=" << ifp.m_type_of_msg.GetTagName());
  }
#endif

  if (!transport->WritePDU(rawData)) {
    PTRACE(1, "T38\tWritePacket error: " << transport->GetErrorText());
    return PFalse;
  }

  // Calculate the level of redundency for this data phase
  PINDEX maxRedundancy;
  if (ifp.m_type_of_msg.GetTag() == T38_Type_of_msg::e_t30_indicator)
    maxRedundancy = indicatorRedundancy;
  else if ((T38_Type_of_msg_data)ifp.m_type_of_msg  == T38_Type_of_msg_data::e_v21)
    maxRedundancy = lowSpeedRedundancy;
  else
    maxRedundancy = highSpeedRedundancy;

  // Push down the current ifp into redundant data
  if (maxRedundancy > 0)
    redundantIFPs.InsertAt(0, new PBYTEArray(udptl.m_primary_ifp_packet.GetValue()));

  // Remove redundant data that are surplus to requirements
  while (redundantIFPs.GetSize() > maxRedundancy)
    redundantIFPs.RemoveAt(maxRedundancy);

  return PTrue;
}


PBoolean OpalT38Protocol::WriteIndicator(unsigned indicator)
{
  T38_IFPPacket ifp;

  ifp.SetTag(T38_Type_of_msg::e_t30_indicator);
  T38_Type_of_msg_t30_indicator & ind = ifp.m_type_of_msg;
  ind.SetValue(indicator);

  return WritePacket(ifp);
}


PBoolean OpalT38Protocol::WriteMultipleData(unsigned mode,
                                        PINDEX count,
                                        unsigned * type,
                                        const PBYTEArray * data)
{
  T38_IFPPacket ifp;

  ifp.SetTag(T38_Type_of_msg::e_data);
  T38_Type_of_msg_data & datamode = ifp.m_type_of_msg;
  datamode.SetValue(mode);

  ifp.IncludeOptionalField(T38_IFPPacket::e_data_field);
  ifp.m_data_field.SetSize(count);
  for (PINDEX i = 0; i < count; i++) {
    ifp.m_data_field[i].m_field_type.SetValue(type[i]);
    ifp.m_data_field[i].m_field_data.SetValue(data[i]);
  }

  return WritePacket(ifp);
}


PBoolean OpalT38Protocol::WriteData(unsigned mode, unsigned type, const PBYTEArray & data)
{
  return WriteMultipleData(mode, 1, &type, &data);
}


PBoolean OpalT38Protocol::Answer()
{
  PTRACE(3, "T38\tAnswer, transport=" << *transport);

  // Should probably get this from the channel open negotiation, but for
  // the time being just accept from whoever sends us something first
  transport->SetPromiscuous(H323Transport::AcceptFromAnyAutoSet);

  int consecutiveBadPackets = 0;
  int expectedSequenceNumber = 0;	// 16 bit
  PBoolean firstPacket = PTrue;

  for (;;) {
    PPER_Stream rawData;
    if (!transport->ReadPDU(rawData)) {
      PTRACE(1, "T38\tError reading PDU: " << transport->GetErrorText(PChannel::LastReadError));
      return PFalse;
    }

    /* when we get the first packet, set the RemoteAddress and then turn off
     * promiscuous listening */
    if (firstPacket) {
      PTRACE(3, "T38\tReceived first packet, remote=" << transport->GetRemoteAddress());
      transport->SetPromiscuous(H323Transport::AcceptFromRemoteOnly);
      firstPacket = PFalse;
    }

    // Decode the PDU
    T38_UDPTLPacket udptl;
    if (udptl.Decode(rawData))
      consecutiveBadPackets = 0;
    else {
      consecutiveBadPackets++;
      PTRACE(2, "T38\tRaw data decode failure:\n  "
             << setprecision(2) << rawData << "\n  UDPTL = "
             << setprecision(2) << udptl);
      if (consecutiveBadPackets > 3) {
        PTRACE(1, "T38\tRaw data decode failed multiple times, aborting!");
        return PFalse;
      }
      continue;
    }

    T38_IFPPacket ifp;
    if (!udptl.m_primary_ifp_packet.DecodeSubType(ifp)) {
      PTRACE(2, "T38\tUDPTLPacket decode failure:\n  "
             << setprecision(2) << rawData << "\n  UDPTL = "
             << setprecision(2) << udptl << "\n  ifp = "
             << setprecision(2) << ifp);
      continue;
    }

    unsigned receivedSequenceNumber = udptl.m_seq_number;

#if PTRACING
    if (PTrace::CanTrace(5)) {
      PTRACE(4, "T38\tReceived UDPTL packet:\n  "
             << setprecision(2) << rawData << "\n  "
             << setprecision(2) << udptl);
    }
    if (PTrace::CanTrace(4)) {
      PTRACE(4, "T38\tReceived UDPTL packet:\n  " << setprecision(2) << udptl);
    }
    else {
      PTRACE(3, "T38\tReceived UDPTL packet: seq=" << receivedSequenceNumber);
    }
#endif

    // Calculate the number of lost packets, if the number lost is really
    // really big then it means it is actually a packet arriving out of order
    int lostPackets = (receivedSequenceNumber - expectedSequenceNumber)&0xffff;
    if (lostPackets > 32767) {
      PTRACE(3, "T38\tIgnoring out of order packet");
      continue;
    }

    expectedSequenceNumber = (WORD)(receivedSequenceNumber+1);

    // See if this is the expected packet
    if (lostPackets > 0) {
      // Not what was expected, see if we have enough redundant data
      if (udptl.m_error_recovery.GetTag() == T38_UDPTLPacket_error_recovery::e_secondary_ifp_packets) {
        T38_UDPTLPacket_error_recovery_secondary_ifp_packets & secondary = udptl.m_error_recovery;
        int nRedundancy = secondary.GetSize();
        if (lostPackets >= nRedundancy) {
          if (!HandlePacketLost(lostPackets - nRedundancy)) {
            PTRACE(1, "T38\tHandle packet failed, aborting answer");
            return PFalse;
          }
          lostPackets = nRedundancy;
        }
        while (lostPackets > 0) {
          if (!HandleRawIFP(secondary[lostPackets++])) {
            PTRACE(1, "T38\tHandle packet failed, aborting answer");
            return PFalse;
          }
        }
      }
      else {
        if (!HandlePacketLost(lostPackets)) {
          PTRACE(1, "T38\tHandle lost packet, aborting answer");
          return PFalse;
        }
      }
    }

    if (!HandleRawIFP(udptl.m_primary_ifp_packet)) {
      PTRACE(1, "T38\tHandle packet failed, aborting answer");
      return PFalse;
    }
  }
}


PBoolean OpalT38Protocol::HandleRawIFP(const PASN_OctetString & pdu)
{
  T38_IFPPacket ifp;

  if (corrigendumASN) {
    if (pdu.DecodeSubType(ifp))
      return HandlePacket(ifp);

    PTRACE(2, "T38\tIFP decode failure:\n  " << setprecision(2) << ifp);
    return PTrue;
  }

  T38_PreCorrigendum_IFPPacket old_ifp;
  if (!pdu.DecodeSubType(old_ifp)) {
    PTRACE(2, "T38\tPre-corrigendum IFP decode failure:\n  " << setprecision(2) << old_ifp);
    return PTrue;
  }

  ifp.m_type_of_msg = old_ifp.m_type_of_msg;

  if (old_ifp.HasOptionalField(T38_IFPPacket::e_data_field)) {
    ifp.IncludeOptionalField(T38_IFPPacket::e_data_field);
    PINDEX count = old_ifp.m_data_field.GetSize();
    ifp.m_data_field.SetSize(count);
    for (PINDEX i = 0 ; i < count; i++) {
      ifp.m_data_field[i].m_field_type = old_ifp.m_data_field[i].m_field_type;
      if (old_ifp.m_data_field[i].HasOptionalField(T38_Data_Field_subtype::e_field_data)) {
        ifp.m_data_field[i].IncludeOptionalField(T38_Data_Field_subtype::e_field_data);
        ifp.m_data_field[i].m_field_data = old_ifp.m_data_field[i].m_field_data;
      }
    }
  }

  return HandlePacket(ifp);
}


PBoolean OpalT38Protocol::HandlePacket(const T38_IFPPacket & ifp)
{
  if (ifp.m_type_of_msg.GetTag() == T38_Type_of_msg::e_t30_indicator)
    return OnIndicator((T38_Type_of_msg_t30_indicator)ifp.m_type_of_msg);

  for (PINDEX i = 0; i < ifp.m_data_field.GetSize(); i++) {
    if (!OnData((T38_Type_of_msg_data)ifp.m_type_of_msg,
                ifp.m_data_field[i].m_field_type,
                ifp.m_data_field[i].m_field_data.GetValue()))
      return PFalse;
  }
  return PTrue;
}


PBoolean OpalT38Protocol::OnIndicator(unsigned indicator)
{
  switch (indicator) {
    case T38_Type_of_msg_t30_indicator::e_no_signal :
      break;

    case T38_Type_of_msg_t30_indicator::e_cng :
      return OnCNG();

    case T38_Type_of_msg_t30_indicator::e_ced :
      return OnCED();

    case T38_Type_of_msg_t30_indicator::e_v21_preamble :
      return OnPreamble();

    case T38_Type_of_msg_t30_indicator::e_v27_2400_training :
    case T38_Type_of_msg_t30_indicator::e_v27_4800_training :
    case T38_Type_of_msg_t30_indicator::e_v29_7200_training :
    case T38_Type_of_msg_t30_indicator::e_v29_9600_training :
    case T38_Type_of_msg_t30_indicator::e_v17_7200_short_training :
    case T38_Type_of_msg_t30_indicator::e_v17_7200_long_training :
    case T38_Type_of_msg_t30_indicator::e_v17_9600_short_training :
    case T38_Type_of_msg_t30_indicator::e_v17_9600_long_training :
    case T38_Type_of_msg_t30_indicator::e_v17_12000_short_training :
    case T38_Type_of_msg_t30_indicator::e_v17_12000_long_training :
    case T38_Type_of_msg_t30_indicator::e_v17_14400_short_training :
    case T38_Type_of_msg_t30_indicator::e_v17_14400_long_training :
      return OnTraining(indicator);

    default:
      break;
  }

  return PTrue;
}


PBoolean OpalT38Protocol::OnCNG()
{
  return PTrue;
}


PBoolean OpalT38Protocol::OnCED()
{
  return PTrue;
}


PBoolean OpalT38Protocol::OnPreamble()
{
  return PTrue;
}


PBoolean OpalT38Protocol::OnTraining(unsigned /*indicator*/)
{
  return PTrue;
}


PBoolean OpalT38Protocol::OnData(unsigned /*mode*/,
                             unsigned /*type*/,
                             const PBYTEArray & /*data*/)
{
  return PTrue;
}


#if PTRACING
#define PTRACE_nLost nLost
#else
#define PTRACE_nLost
#endif

PBoolean OpalT38Protocol::HandlePacketLost(unsigned PTRACE_nLost)
{
  PTRACE(2, "T38\tHandlePacketLost, n=" << PTRACE_nLost);
  /* don't handle lost packets yet */
  return PTrue;
}

/////////////////////////////////////////////////////////////////////////////

T38PseudoRTP::T38PseudoRTP(
#if OPAL_RTP_AGGREGATE
                           PHandleAggregator * _aggregator, 
#endif
                           unsigned _id, PBoolean _remoteIsNAT)
  : RTP_UDP(
#if OPAL_RTP_AGGREGATE
  _aggregator, 
#endif
  _id, _remoteIsNAT)
{
  PTRACE(4, "RTP_T38\tPseudoRTP session created with NAT flag set to " << remoteIsNAT);

  corrigendumASN = PTrue;
  consecutiveBadPackets = 0;

  reportTimer = 20;

  lastIFP.SetSize(0);
}

T38PseudoRTP::~T38PseudoRTP()
{
  PTRACE(4, "RTP_T38\tPseudoRTP session destroyed");
}

PBoolean T38PseudoRTP::SetRemoteSocketInfo(PIPSocket::Address address, WORD port, PBoolean isDataPort)
{
  return RTP_UDP::SetRemoteSocketInfo(address, port, isDataPort);
}


RTP_Session::SendReceiveStatus T38PseudoRTP::OnSendData(RTP_DataFrame & frame)
{
  if (frame.GetPayloadSize() == 0)
    return e_IgnorePacket;

  PINDEX plLen = frame.GetPayloadSize();

  // reformat the raw T.38 data as an UDPTL packet
  T38_UDPTLPacket udptl;
  udptl.m_seq_number = frame.GetSequenceNumber();
  udptl.m_primary_ifp_packet.SetValue(frame.GetPayloadPtr(), plLen);

  udptl.m_error_recovery.SetTag(T38_UDPTLPacket_error_recovery::e_secondary_ifp_packets);
  T38_UDPTLPacket_error_recovery_secondary_ifp_packets & secondary = udptl.m_error_recovery;
  T38_UDPTLPacket_error_recovery_secondary_ifp_packets & redundantPackets = secondary;
  if (lastIFP.GetSize() == 0)
    redundantPackets.SetSize(0);
  else {
    redundantPackets.SetSize(1);
    T38_UDPTLPacket_error_recovery_secondary_ifp_packets_subtype & redundantPacket = redundantPackets[0];
    redundantPacket.SetValue(lastIFP, lastIFP.GetSize());
  }

  lastIFP = udptl.m_primary_ifp_packet;

  PPER_Stream rawData;
  udptl.Encode(rawData);
  rawData.CompleteEncoding();

#if PTRACING
  if (PTrace::CanTrace(4)) {
    PTRACE(4, "RTP_T38\tSending PDU:\n  "
           << setprecision(2) << udptl << "\n "
           << setprecision(2) << rawData);
  }
#endif

#if 0
  // Calculate the level of redundency for this data phase
  PINDEX maxRedundancy;
  if (ifp.m_type_of_msg.GetTag() == T38_Type_of_msg::e_t30_indicator)
    maxRedundancy = indicatorRedundancy;
  else if ((T38_Type_of_msg_data)ifp.m_type_of_msg  == T38_Type_of_msg_data::e_v21)
    maxRedundancy = lowSpeedRedundancy;
  else
    maxRedundancy = highSpeedRedundancy;

  // Push down the current ifp into redundant data
  if (maxRedundancy > 0)
    redundantIFPs.InsertAt(0, new PBYTEArray(udptl.m_primary_ifp_packet.GetValue()));

  // Remove redundant data that are surplus to requirements
  while (redundantIFPs.GetSize() > maxRedundancy)
    redundantIFPs.RemoveAt(maxRedundancy);
#endif

  // copy the UDPTL into the RTP packet
  frame.SetSize(rawData.GetSize());
  memcpy(frame.GetPointer(), rawData.GetPointer(), rawData.GetSize());

PTRACE(1, "T38_RTP\tWriting RTP T.38 seq " << udptl.m_seq_number << " of size " << plLen << " as T.38 UDPTL size " << frame.GetSize());

  return e_ProcessPacket;
}

PBoolean T38PseudoRTP::WriteData(RTP_DataFrame & frame)
{
  if (shutdownWrite) {
    PTRACE(3, "RTP_T38\tSession " << sessionID << ", Write shutdown.");
    shutdownWrite = PFalse;
    return PFalse;
  }

  // Trying to send a PDU before we are set up!
  if (!remoteAddress.IsValid() || remoteDataPort == 0)
    return PTrue;

  switch (OnSendData(frame)) {
    case e_ProcessPacket :
      break;
    case e_IgnorePacket :
      return PTrue;
    case e_AbortTransport :
      return PFalse;
  }

  while (!dataSocket->WriteTo(frame.GetPointer(),
                             frame.GetSize(),
                             remoteAddress, remoteDataPort)) {
    switch (dataSocket->GetErrorNumber()) {
      case ECONNRESET :
      case ECONNREFUSED :
        PTRACE(2, "RTP_T38\tSession " << sessionID << ", data port on remote not ready.");
        break;

      default:
        PTRACE(1, "RTP_T38\tSession " << sessionID
               << ", Write error on data port ("
               << dataSocket->GetErrorNumber(PChannel::LastWriteError) << "): "
               << dataSocket->GetErrorText(PChannel::LastWriteError));
        return PFalse;
    }
  }

  return PTrue;
}


RTP_Session::SendReceiveStatus T38PseudoRTP::OnSendControl(RTP_ControlFrame & /*frame*/, PINDEX & /*len*/)
{
  return e_IgnorePacket; // Non fatal error, just ignore
}

RTP_Session::SendReceiveStatus T38PseudoRTP::ReadDataPDU(RTP_DataFrame & frame)
{
  SendReceiveStatus status = ReadDataOrControlPDU(*dataSocket, frame, PTrue);
  if (status != e_ProcessPacket)
    return status;

  // Check received PDU is big enough
  PINDEX pduSize = dataSocket->GetLastReadCount();
  frame.SetSize(pduSize);

  return OnReceiveData(frame);
}

RTP_Session::SendReceiveStatus T38PseudoRTP::OnReceiveData(RTP_DataFrame & frame)
{
  PTRACE(4, "T38_RTP\tReading raw T.38 of size " << frame.GetSize());

  if ((frame.GetPayloadSize() == 1) && (frame.GetPayloadPtr()[0] == 0xff)) {
    // allow fake timing frames to pass through
  }
  else {
    PPER_Stream rawData(frame.GetPointer(), frame.GetSize());

    // Decode the PDU
    T38_UDPTLPacket udptl;
    if (udptl.Decode(rawData))
      consecutiveBadPackets = 0;
    else {
      consecutiveBadPackets++;
      PTRACE(2, "RTP_T38\tRaw data decode failure:\n  "
             << setprecision(2) << rawData << "\n  UDPTL = "
             << setprecision(2) << udptl);
      if (consecutiveBadPackets > 100) {
        PTRACE(1, "RTP_T38\tRaw data decode failed multiple times, aborting!");
        return e_AbortTransport;
      }
      return e_IgnorePacket;
    }

    PASN_OctetString & ifp = udptl.m_primary_ifp_packet;
    frame.SetPayloadSize(ifp.GetDataLength());

    memcpy(frame.GetPayloadPtr(), ifp.GetPointer(), ifp.GetDataLength());
    frame.SetSequenceNumber((WORD)(udptl.m_seq_number & 0xffff));
    PTRACE(4, "T38_RTP\tT38 decode :\n  " << setprecision(2) << udptl);
  }

  frame[0] = 0x80;
  frame.SetPayloadType((RTP_DataFrame::PayloadTypes)96);
  frame.SetSyncSource(syncSourceIn);

  PTRACE(3, "T38_RTP\tReading RTP payload size " << frame.GetPayloadSize());

  return RTP_UDP::OnReceiveData(frame);
}

PBoolean T38PseudoRTP::ReadData(RTP_DataFrame & frame, PBoolean loop)
{
  do {
    int selectStatus = WaitForPDU(*dataSocket, *controlSocket, reportTimer);

    if (shutdownRead) {
      PTRACE(3, "T38_RTP\tSession " << sessionID << ", Read shutdown.");
      shutdownRead = PFalse;
      return PFalse;
    }

    switch (selectStatus) {
      case -2 :
        if (ReadControlPDU() == e_AbortTransport)
          return PFalse;
        break;

      case -3 :
        if (ReadControlPDU() == e_AbortTransport)
          return PFalse;
        // Then do -1 case

      case -1 :
        switch (ReadDataPDU(frame)) {
          case e_ProcessPacket :
            if (!shutdownRead)
              return PTrue;
          case e_IgnorePacket :
            break;
          case e_AbortTransport :
            return PFalse;
        }
        break;

      case 0 :
        // for timeouts, send a "fake" payload of one byte of 0xff to keep the T.38 engine emitting PCM
        frame.SetPayloadSize(1);
        frame.GetPayloadPtr()[0] = 0xff;
        return PTrue;

      case PSocket::Interrupted:
        PTRACE(3, "T38_RTP\tSession " << sessionID << ", Interrupted.");
        return PFalse;

      default :
        PTRACE(1, "T38_RTP\tSession " << sessionID << ", Select error: "
                << PChannel::GetErrorText((PChannel::Errors)selectStatus));
        return PFalse;
    }
  } while (loop);

  frame.SetSize(0);
  return PTrue;
}

int T38PseudoRTP::WaitForPDU(PUDPSocket & dataSocket, PUDPSocket & controlSocket, const PTimeInterval &)
{
  // wait for no longer than 20ms so audio gets correctly processed
  return PSocket::Select(dataSocket, controlSocket, 20);
}


/////////////////////////////////////////////////////////////////////////////

static PMutex faxMapMutex;
typedef std::map<std::string, OpalFaxCallInfo *> OpalFaxCallInfoMap_T;

static OpalFaxCallInfoMap_T faxCallInfoMap;

OpalFaxCallInfo::OpalFaxCallInfo()
{ 
  refCount = 1; 
  spanDSPPort = 0; 
}

/////////////////////////////////////////////////////////////////////////////

OpalFaxMediaStream::OpalFaxMediaStream(OpalConnection & conn, const OpalMediaFormat & mediaFormat, unsigned sessionID, PBoolean isSource, const PString & _token, const PString & _filename, PBoolean _receive)
  : OpalMediaStream(conn, mediaFormat, sessionID, isSource), sessionToken(_token), filename(_filename), receive(_receive)
{
  faxCallInfo = NULL;
  SetDataSize(300);
}

PBoolean OpalFaxMediaStream::Open()
{
  if (sessionToken.IsEmpty()) {
    PTRACE(1, "T38\tCannot open unknown media stream");
    return PFalse;
  }

  PWaitAndSignal m2(faxMapMutex);
  PWaitAndSignal m(infoMutex);

  if (faxCallInfo == NULL) {
    OpalFaxCallInfoMap_T::iterator r = faxCallInfoMap.find(sessionToken);
    if (r != faxCallInfoMap.end()) {
      faxCallInfo = r->second;
      ++faxCallInfo->refCount;
    } else {
      faxCallInfo = new OpalFaxCallInfo();
      if (!faxCallInfo->socket.Listen()) {
        PTRACE(1, "Fax\tCannot open listening socket for SpanDSP");
        return PFalse;
      }
      faxCallInfo->socket.SetReadTimeout(1000);
      faxCallInfoMap.insert(OpalFaxCallInfoMap_T::value_type((const char *)sessionToken, faxCallInfo));
    }
  }

  return OpalMediaStream::Open();
}

PBoolean OpalFaxMediaStream::Start()
{
  PWaitAndSignal m(infoMutex);
  if (faxCallInfo == NULL) {
    PTRACE(1, "Fax\tCannot start unknown media stream");
    return PFalse;
  }

  // reset the output buffer
  writeBufferLen = 0;

  // only open pipe channel once
  if (!faxCallInfo->spanDSP.IsOpen()) {

    // create the command line for spandsp_util
    PString cmdLine = GetSpanDSPCommandLine(*faxCallInfo);
    PTRACE(1, "Fax\tExecuting '" << cmdLine << "'");

    // open connection to spandsp
    if (!faxCallInfo->spanDSP.Open(cmdLine, PPipeChannel::ReadWriteStd)) {
      PTRACE(1, "Fax\tCannot open SpanDSP");
      return PFalse;
    }

    if (!faxCallInfo->spanDSP.Execute()) {
      PTRACE(1, "Fax\tCannot execute SpanDSP");
      return PFalse;
    }
  }

  return PTrue;
}

PBoolean OpalFaxMediaStream::ReadPacket(RTP_DataFrame & packet)
{
  // it is possible for ReadPacket to be called before the media stream has been opened, so deal with that case
  PWaitAndSignal m(infoMutex);
  if ((faxCallInfo == NULL) || !faxCallInfo->spanDSP.IsRunning()) {

    // return silence
    packet.SetPayloadSize(0);

  } else {

    packet.SetSize(2048);

    if (faxCallInfo->spanDSPPort > 0) {
      if (!faxCallInfo->socket.Read(packet.GetPointer()+RTP_DataFrame::MinHeaderSize, packet.GetSize()-RTP_DataFrame::MinHeaderSize)) {
        faxCallInfo->socket.Close();
        return PFalse;
      }
    } else{ 
      if (!faxCallInfo->socket.ReadFrom(packet.GetPointer()+RTP_DataFrame::MinHeaderSize, packet.GetSize()-RTP_DataFrame::MinHeaderSize, faxCallInfo->spanDSPAddr, faxCallInfo->spanDSPPort)) {
        faxCallInfo->socket.Close();
        return PFalse;
      }
    }

    PINDEX len = faxCallInfo->socket.GetLastReadCount();
    packet.SetPayloadType(RTP_DataFrame::MaxPayloadType);
    packet.SetPayloadSize(len);

#if WRITE_PCM_FILE
    static int file = _open("t38_audio_in.pcm", _O_BINARY | _O_CREAT | _O_TRUNC | _O_WRONLY, _S_IREAD | _S_IWRITE);
    if (file >= 0) {
      if (_write(file, packet.GetPointer()+RTP_DataFrame::MinHeaderSize, len) < len) {
        cerr << "cannot write output PCM data to file" << endl;
        file = -1;
      }
    }
#endif    
  }

  return PTrue;
}

PBoolean OpalFaxMediaStream::WritePacket(RTP_DataFrame & packet)
{
  PWaitAndSignal m(infoMutex);
  if ((faxCallInfo == NULL) || !faxCallInfo->spanDSP.IsRunning()) {
   
    // return silence
    packet.SetPayloadSize(0);

  } else {

    if (!faxCallInfo->spanDSP.IsRunning()) {
      PTRACE(1, "Fax\tspandsp ended");
      return PFalse;
    }

#if WRITE_PCM_FILE
    static int file = _open("t38_audio_out.pcm", _O_BINARY | _O_CREAT | _O_TRUNC | _O_WRONLY, _S_IREAD | _S_IWRITE);
    if (file >= 0) {
      PINDEX len = packet.GetPayloadSize();
      if (_write(file, packet.GetPointer()+RTP_DataFrame::MinHeaderSize, len) < len) {
        cerr << "cannot write output PCM data to file" << endl;
        file = -1;
      }
    }
#endif

    if (faxCallInfo->spanDSPPort > 0) {

      PINDEX size = packet.GetPayloadSize();
      BYTE * ptr = packet.GetPayloadPtr();

      // if there is more data than spandsp can accept, break it down
      while ((writeBufferLen + size) >= (PINDEX)sizeof(writeBuffer)) {
        PINDEX len;
        if (writeBufferLen == 0) {
          if (!faxCallInfo->socket.WriteTo(ptr, sizeof(writeBuffer), faxCallInfo->spanDSPAddr, faxCallInfo->spanDSPPort)) {
            PTRACE(1, "Fax\tSocket write error - " << faxCallInfo->socket.GetErrorText(PChannel::LastWriteError));
            return PFalse;
          }
          len = sizeof(writeBuffer);
        }
        else {
          len = sizeof(writeBuffer) - writeBufferLen;
          memcpy(writeBuffer + writeBufferLen, ptr, len);
          if (!faxCallInfo->socket.WriteTo(writeBuffer, sizeof(writeBuffer), faxCallInfo->spanDSPAddr, faxCallInfo->spanDSPPort)) {
            PTRACE(1, "Fax\tSocket write error - " << faxCallInfo->socket.GetErrorText(PChannel::LastWriteError));
            return PFalse;
          }
        }
        ptr += len;
        size -= len;
        writeBufferLen = 0;
      }

      // copy remaining data into buffer
      if (size > 0) {
        memcpy(writeBuffer + writeBufferLen, ptr, size);
        writeBufferLen += size;
      }

      if (writeBufferLen == sizeof(writeBuffer)) {
        if (!faxCallInfo->socket.WriteTo(writeBuffer, sizeof(writeBuffer), faxCallInfo->spanDSPAddr, faxCallInfo->spanDSPPort)) {
          PTRACE(1, "Fax\tSocket write error - " << faxCallInfo->socket.GetErrorText(PChannel::LastWriteError));
          return PFalse;
        }
        writeBufferLen = 0;
      }
    }
  }

  return PTrue;
}

PBoolean OpalFaxMediaStream::Close()
{
  PBoolean stat = OpalMediaStream::Close();

  PWaitAndSignal m2(faxMapMutex);

  {
    if (faxCallInfo == NULL || sessionToken.IsEmpty()) {
      PTRACE(1, "Fax\tCannot close unknown media stream");
      return stat;
    }

    // shutdown whatever is running
    faxCallInfo->socket.Close();
    faxCallInfo->spanDSP.Close();

    OpalFaxCallInfoMap_T::iterator r = faxCallInfoMap.find(sessionToken);
    if (r == faxCallInfoMap.end()) {
      PTRACE(1, "Fax\tError: media stream not found in T38 session list");
      PWaitAndSignal m(infoMutex);
      faxCallInfo = NULL;
      return stat;
    }

    if (r->second != faxCallInfo) {
      PTRACE(1, "Fax\tError: session list does not match local ptr");
      PWaitAndSignal m(infoMutex);
      faxCallInfo = NULL;
      return stat;
    }

    else if (faxCallInfo->refCount == 0) {
      PTRACE(1, "Fax\tError: media stream has incorrect reference count");
      PWaitAndSignal m(infoMutex);
      faxCallInfo = NULL;
      return stat;
    }

    if (--faxCallInfo->refCount > 0) {
      PWaitAndSignal m(infoMutex);
      faxCallInfo = NULL;
      PTRACE(1, "Fax\tClosed fax media stream");
      return stat;
    }
  }

  // remove info from map
  faxCallInfoMap.erase(sessionToken);

  // delete the object
  {
    PWaitAndSignal m(infoMutex);
    faxCallInfo = NULL;
  }

  delete faxCallInfo;

  return stat;
}

PBoolean OpalFaxMediaStream::IsSynchronous() const
{
  return PTrue;
}

PString OpalFaxMediaStream::GetSpanDSPCommandLine(OpalFaxCallInfo & info)
{
  PStringStream cmdline;

  PIPSocket::Address dummy; WORD port;
  info.socket.GetLocalAddress(dummy, port);

  cmdline << "spandsp_util -m ";
  if (receive)
    cmdline << "fax_to_tiff";
  else
    cmdline << "tiff_to_fax";
  cmdline << " -n '" << filename << "' -f 127.0.0.1:" << port;

  return cmdline;
}

/////////////////////////////////////////////////////////////////////////////

/**This class describes a media stream that transfers data to/from a T.38 session
  */
OpalT38MediaStream::OpalT38MediaStream(
      OpalConnection & conn,
      const OpalMediaFormat & mediaFormat, ///<  Media format for stream
      unsigned sessionID, 
      PBoolean isSource,                       ///<  Is a source stream
      const PString & token,               ///<  token used to match incoming/outgoing streams
      const PString & _filename,
      PBoolean _receive
    )
  : OpalFaxMediaStream(conn, mediaFormat, sessionID, isSource, token, _filename, _receive)
{
}

PString OpalT38MediaStream::GetSpanDSPCommandLine(OpalFaxCallInfo & info)
{
  PStringStream cmdline;

  PIPSocket::Address dummy; WORD port;
  info.socket.GetLocalAddress(dummy, port);

  cmdline << "spandsp_util -V 0 -m ";
  if (receive)
    cmdline << "fax_to_tiff";
  else
    cmdline << "tiff_to_fax";
  cmdline << " -n '" << filename << "' -t 127.0.0.1:" << port;

  return cmdline;
}

PBoolean OpalT38MediaStream::ReadPacket(RTP_DataFrame & packet)
{
  if (faxCallInfo == NULL)
    return PFalse;

  if (!faxCallInfo->spanDSP.IsRunning()) {
    PTRACE(1, "Fax\tspandsp ended");
    return PFalse;
  }

  packet.SetSize(2048);

  if (faxCallInfo->spanDSPPort > 0) {
    if (!faxCallInfo->socket.Read(packet.GetPointer(), packet.GetSize()))
      return PFalse;
  } else{ 
    if (!faxCallInfo->socket.ReadFrom(packet.GetPointer(), packet.GetSize(), faxCallInfo->spanDSPAddr, faxCallInfo->spanDSPPort))
      return PFalse;
  }

  PINDEX len = faxCallInfo->socket.GetLastReadCount();
  if (len < RTP_DataFrame::MinHeaderSize)
    return PFalse;

  packet.SetSize(len);
  packet.SetPayloadSize(len - RTP_DataFrame::MinHeaderSize);

//PTRACE(1, "Fax\tT.38 Read RTP payload size = " << packet.GetPayloadSize());

  return PTrue;
}

PBoolean OpalT38MediaStream::WritePacket(RTP_DataFrame & packet)
{
  if (faxCallInfo == NULL)
    return PFalse;

  if (!faxCallInfo->spanDSP.IsRunning()) {
    PTRACE(1, "Fax\tspandsp ended");
    return PFalse;
  }

//PTRACE(1, "Fax\tT.38 Write RTP payload size = " << packet.GetPayloadSize());

  if (faxCallInfo->spanDSPPort > 0) {
    PTRACE(1, "Fax\tT.38 Write RTP packet size = " << packet.GetHeaderSize() + packet.GetPayloadSize() <<" to " << faxCallInfo->spanDSPAddr << ":" << faxCallInfo->spanDSPPort);
    if (!faxCallInfo->socket.WriteTo(packet.GetPointer(), packet.GetHeaderSize() + packet.GetPayloadSize(), faxCallInfo->spanDSPAddr, faxCallInfo->spanDSPPort)) {
      PTRACE(1, "T38_UDP\tSocket write error - " << faxCallInfo->socket.GetErrorText(PChannel::LastWriteError));
      return PFalse;
    }
  }

  return PTrue;
}

/////////////////////////////////////////////////////////////////////////////

OpalFaxEndPoint::OpalFaxEndPoint(OpalManager & mgr, const char * prefix)
  : OpalEndPoint(mgr, prefix, CanTerminateCall)
{
  PTRACE(3, "Fax\tCreated Fax endpoint");
}

OpalFaxEndPoint::~OpalFaxEndPoint()
{
  PTRACE(3, "Fax\tDeleted Fax endpoint.");
}

OpalFaxConnection * OpalFaxEndPoint::CreateConnection(OpalCall & call, const PString & filename, PBoolean receive, void * /*userData*/, OpalConnection::StringOptions * stringOptions)
{
  return new OpalFaxConnection(call, *this, filename, receive, MakeToken(), stringOptions);
}

OpalMediaFormatList OpalFaxEndPoint::GetMediaFormats() const
{
  OpalMediaFormatList formats;

  //formats += OpalPCM16;        

  return formats;
}

PString OpalFaxEndPoint::MakeToken()
{
  return psprintf("FaxConnection_%i", ++faxCallIndex);
}

void OpalFaxEndPoint::AcceptIncomingConnection(const PString & token)
{
  PSafePtr<OpalFaxConnection> connection = PSafePtrCast<OpalConnection, OpalFaxConnection>(GetConnectionWithLock(token, PSafeReadOnly));
  if (connection != NULL)
    connection->AcceptIncoming();
}

PBoolean OpalFaxEndPoint::MakeConnection(OpalCall & call,
                                const PString & remoteParty,
                                         void * userData,
                                 unsigned int /*options*/,
                OpalConnection::StringOptions * stringOptions)
{
  // First strip of the prefix if present
  PINDEX prefixLength = 0;
  if (remoteParty.Find(GetPrefixName()+":") == 0)
    prefixLength = GetPrefixName().GetLength()+1;

  PString filename = remoteParty.Mid(prefixLength);

  PBoolean receive = PFalse;

  PINDEX options = filename.Find(";");
  if (options != P_MAX_INDEX) {
    PString str = filename.Mid(options+1);
    filename = filename.Left(options);
    receive = str *= "receive";
  }

  if (!receive && !PFile::Exists(filename)) {
    return PFalse;
  }

/*
  PString playDevice;
  PString recordDevice;
  PINDEX separator = remoteParty.FindOneOf("\n\t\\", prefixLength);
  if (separator == P_MAX_INDEX)
    playDevice = recordDevice = remoteParty.Mid(prefixLength);
  else {
    playDevice = remoteParty(prefixLength+1, separator-1);
    recordDevice = remoteParty.Mid(separator+1);
  }

  if (!SetDeviceName(playDevice, PSoundChannel::Player, playDevice))
    playDevice = soundChannelPlayDevice;
  if (!SetDeviceName(recordDevice, PSoundChannel::Recorder, recordDevice))
    recordDevice = soundChannelRecordDevice;

*/

  return AddConnection(CreateConnection(call, filename, receive, userData, stringOptions));
}

void OpalFaxEndPoint::OnPatchMediaStream(const OpalFaxConnection & /*connection*/, PBoolean /*isSource*/, OpalMediaPatch & /*patch*/)
{
}

/////////////////////////////////////////////////////////////////////////////

OpalFaxConnection::OpalFaxConnection(OpalCall & call, OpalFaxEndPoint & ep, const PString & _filename, PBoolean _receive, const PString & _token, OpalConnection::StringOptions * stringOptions)
  : OpalConnection(call, ep, _token), endpoint(ep), filename(_filename), receive(_receive)
{
  PTRACE(3, "FAX\tCreated FAX connection with token '" << callToken << "'");
  phase = SetUpPhase;

  forceFaxAudio = (stringOptions == NULL) || stringOptions->Contains("Force-Fax-Audio");
}

OpalFaxConnection::~OpalFaxConnection()
{
  PTRACE(3, "FAX\tDeleted FAX connection.");
}

OpalMediaStream * OpalFaxConnection::CreateMediaStream(const OpalMediaFormat & mediaFormat, unsigned sessionID, PBoolean isSource)
{
  // if creating an audio session, use a NULL stream
  if (sessionID == OpalMediaFormat::DefaultAudioSessionID) {
    if (forceFaxAudio && (mediaFormat == OpalPCM16))
      return new OpalFaxMediaStream(*this, mediaFormat, sessionID, isSource, GetToken(), filename, receive);
    else
      return new OpalNullMediaStream(*this, mediaFormat, sessionID, isSource);
  }

  // if creating a data stream, see what type it is
  else if (!forceFaxAudio && (sessionID == OpalMediaFormat::DefaultDataSessionID)) {
    if (mediaFormat == OpalPCM16Fax)
      return new OpalFaxMediaStream(*this, mediaFormat, sessionID, isSource, GetToken(), filename, receive);
  }

  // else call ancestor
  return NULL;
}

PBoolean OpalFaxConnection::SetUpConnection()
{
  // Check if we are A-Party in thsi call, so need to do things differently
  if (ownerCall.GetConnection(0) == this) {
    phase = SetUpPhase;
    if (!OnIncomingConnection(0, NULL)) {
      Release(EndedByCallerAbort);
      return PFalse;
    }

    PTRACE(2, "FAX\tOutgoing call routed to " << ownerCall.GetPartyB() << " for " << *this);
    if (!ownerCall.OnSetUp(*this)) {
      Release(EndedByNoAccept);
      return PFalse;
    }

    return PTrue;
  }

  {
    PSafePtr<OpalConnection> otherConn = ownerCall.GetOtherPartyConnection(*this);
    if (otherConn == NULL)
      return PFalse;

    remotePartyName    = otherConn->GetRemotePartyName();
    remotePartyAddress = otherConn->GetRemotePartyAddress();
    remoteProductInfo  = otherConn->GetRemoteProductInfo();
  }

  PTRACE(3, "FAX\tSetUpConnection(" << remotePartyName << ')');
  phase = AlertingPhase;
  //endpoint.OnShowIncoming(*this);
  OnAlerting();

  phase = ConnectedPhase;
  OnConnected();

  if (!mediaStreams.IsEmpty()) {
    phase = EstablishedPhase;
    OnEstablished();
  }

  return PTrue;
}


PBoolean OpalFaxConnection::SetAlerting(const PString & calleeName, PBoolean)
{
  PTRACE(3, "Fax\tSetAlerting(" << calleeName << ')');
  phase = AlertingPhase;
  remotePartyName = calleeName;
  //return endpoint.OnShowOutgoing(*this);
  return PTrue;
}


PBoolean OpalFaxConnection::SetConnected()
{
  PTRACE(3, "Fax\tSetConnected()");

  {
    PSafeLockReadWrite safeLock(*this);
    if (!safeLock.IsLocked())
      return PFalse;

    if (mediaStreams.IsEmpty())
      return PTrue;

    phase = EstablishedPhase;
  }

  OnEstablished();

  return PTrue;
}


OpalMediaFormatList OpalFaxConnection::GetMediaFormats() const
{
  OpalMediaFormatList formats;

  if (forceFaxAudio)
    formats += OpalPCM16;        
  else
    formats += OpalPCM16Fax;        

  return formats;
}

void OpalFaxConnection::AdjustMediaFormats(OpalMediaFormatList & mediaFormats) const
{
  endpoint.AdjustMediaFormats(*this, mediaFormats);
  if (forceFaxAudio)
    mediaFormats.Remove(PStringArray(OpalT38));
}

void OpalFaxConnection::OnPatchMediaStream(PBoolean isSource, OpalMediaPatch & patch)
{
  endpoint.OnPatchMediaStream(*this, isSource, patch);
}


void OpalFaxConnection::AcceptIncoming()
{
  if (!LockReadOnly())
    return;

  if (phase != AlertingPhase) {
    UnlockReadOnly();
    return;
  }

  LockReadWrite();
  phase = ConnectedPhase;
  UnlockReadWrite();
  UnlockReadOnly();

  OnConnected();

  if (!LockReadOnly())
    return;

  if (mediaStreams.IsEmpty()) {
    UnlockReadOnly();
    return;
  }

  LockReadWrite();
  phase = EstablishedPhase;
  UnlockReadWrite();
  UnlockReadOnly();

  OnEstablished();
}

/////////////////////////////////////////////////////////////////////////////

OpalT38EndPoint::OpalT38EndPoint(OpalManager & manager, const char * prefix)
  : OpalFaxEndPoint(manager, prefix)
{
}

OpalMediaFormatList OpalT38EndPoint::GetMediaFormats() const
{
  OpalMediaFormatList formats;

  formats += OpalPCM16;        // need this so we get lead-in to T.38 sessions
  formats += OpalT38;        

  return formats;
}

OpalFaxConnection * OpalT38EndPoint::CreateConnection(OpalCall & call, const PString & filename, PBoolean receive, void * /*userData*/, OpalConnection::StringOptions * stringOptions)
{
  return new OpalT38Connection(call, *this, filename, receive, MakeToken(), stringOptions);
}

PString OpalT38EndPoint::MakeToken()
{
  return psprintf("T38Connection_%i", ++faxCallIndex);
}

/////////////////////////////////////////////////////////////////////////////

OpalT38Connection::OpalT38Connection(OpalCall & call, OpalT38EndPoint & ep, const PString & _filename, PBoolean _receive, const PString & _token, OpalConnection::StringOptions * stringOptions)
  : OpalFaxConnection(call, ep, _filename, _receive, _token, stringOptions)
{
  PTRACE(3, "FAX\tCreated T.38 connection with token '" << callToken << "'");
  forceFaxAudio = PFalse;
}

OpalMediaStream * OpalT38Connection::CreateMediaStream(const OpalMediaFormat & mediaFormat, unsigned sessionID, PBoolean isSource)
{
  // if creating an audio session, use a NULL stream
  if (sessionID == OpalMediaFormat::DefaultAudioSessionID) 
    return new OpalNullMediaStream(*this, mediaFormat, sessionID, isSource);

  // if creating a data stream, see what type it is
  else if ((sessionID == OpalMediaFormat::DefaultDataSessionID) && (mediaFormat == OpalT38))
    return new OpalT38MediaStream(*this, mediaFormat, sessionID, isSource, GetToken(), filename, receive);

  return NULL;
}

void OpalT38Connection::AdjustMediaFormats(OpalMediaFormatList & mediaFormats) const
{
  endpoint.AdjustMediaFormats(*this, mediaFormats);
  mediaFormats.Remove(PStringArray(OpalPCM16Fax));
}

OpalMediaFormatList OpalT38Connection::GetMediaFormats() const
{
  OpalMediaFormatList formats;

  formats += OpalPCM16;        
  formats += OpalT38;        

  return formats;
}


/////////////////////////////////////////////////////////////////////////////

#endif // OPAL_T38FAX


