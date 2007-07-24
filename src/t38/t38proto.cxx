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
 * $Log: t38proto.cxx,v $
 * Revision 1.2023  2007/07/24 12:57:55  rjongbloed
 * Made sure all integer OpalMediaOptions are unsigned so is compatible with H.245 generic capabilities.
 *
 * Revision 2.21  2007/07/19 03:48:53  csoutheren
 * Ensure fake padding RTP packets are handled correctly
 *
 * Revision 2.20  2007/07/16 05:52:30  csoutheren
 * Set payload type and SSRC for fake RTP packets
 * Set PDU size before populating RTP fields to avoid memory overrun
 *
 * Revision 2.19  2007/06/29 06:59:59  rjongbloed
 * Major improvement to the "product info", normalising H.221 and User-Agent mechanisms.
 *
 * Revision 2.18  2007/05/15 01:48:57  csoutheren
 * Fix T.38 compile problems on Linux
 *
 * Revision 2.17  2007/05/13 23:51:44  dereksmithies
 * If t38 is disabled, minimise the work done to compile this file.
 *
 * Revision 2.16  2007/05/10 05:34:23  csoutheren
 * Ensure fax transmission works with reasonable size audio blocks
 *
 * Revision 2.15  2007/05/01 05:33:57  rjongbloed
 * Fixed compiler warning
 *
 * Revision 2.14  2007/04/10 05:15:54  rjongbloed
 * Fixed issue with use of static C string variables in DLL environment,
 *   must use functional interface for correct initialisation.
 *
 * Revision 2.13  2007/03/30 02:18:24  csoutheren
 * Fix obvious error (thanks to Robert)
 *
 * Revision 2.12  2007/03/29 23:13:52  rjongbloed
 * Fixed MSVC warning
 *
 * Revision 2.11  2007/03/29 08:31:36  csoutheren
 * Fix media formats for T.38 endpoint
 *
 * Revision 2.10  2007/03/29 05:20:17  csoutheren
 * Implement T.38 and fax
 *
 * Revision 2.9  2007/01/18 12:49:22  csoutheren
 * Add ability to disable T.38 in compile
 *
 * Revision 2.8  2005/02/21 12:20:08  rjongbloed
 * Added new "options list" to the OpalMediaFormat class.
 *
 * Revision 2.7  2003/01/07 04:39:53  robertj
 * Updated to OpenH323 v1.11.2
 *
 * Revision 2.6  2002/11/10 11:33:20  robertj
 * Updated to OpenH323 v1.10.3
 *
 * Revision 2.5  2002/09/04 06:01:49  robertj
 * Updated to OpenH323 v1.9.6
 *
 * Revision 2.4  2002/02/11 09:32:13  robertj
 * Updated to openH323 v1.8.0
 *
 * Revision 2.3  2002/01/22 05:21:54  robertj
 * Added RTP encoding name string to media format database.
 * Changed time units to clock rate in Hz.
 *
 * Revision 2.2  2002/01/14 06:35:59  robertj
 * Updated to OpenH323 v1.7.9
 *
 * Revision 2.1  2001/08/01 05:05:26  robertj
 * Major changes to H.323 capabilities, uses OpalMediaFormat for base name.
 *
 * Revision 2.0  2001/07/27 15:48:25  robertj
 * Conversion of OpenH323 to Open Phone Abstraction Library (OPAL)
 *
 * Revision 1.17  2002/12/19 01:49:08  robertj
 * Fixed incorrect setting of optional fields in pre-corrigendum packet
 *   translation function, thanks Vyacheslav Frolov
 *
 * Revision 1.16  2002/12/06 04:18:02  robertj
 * Fixed GNU warning
 *
 * Revision 1.15  2002/12/02 04:08:02  robertj
 * Turned T.38 Originate inside out, so now has WriteXXX() functions that can
 *   be call ed in different thread contexts.
 *
 * Revision 1.14  2002/12/02 00:37:19  robertj
 * More implementation of T38 base library code, some taken from the t38modem
 *   application by Vyacheslav Frolov, eg redundent frames.
 *
 * Revision 1.13  2002/11/21 06:40:00  robertj
 * Changed promiscuous mode to be three way. Fixes race condition in gkserver
 *   which can cause crashes or more PDUs to be sent to the wrong place.
 *
 * Revision 1.12  2002/09/25 05:20:40  robertj
 * Fixed warning on no trace version.
 *
 * Revision 1.11  2002/08/05 10:03:48  robertj
 * Cosmetic changes to normalise the usage of pragma interface/implementation.
 *
 * Revision 1.10  2002/02/09 04:39:05  robertj
 * Changes to allow T.38 logical channels to use single transport which is
 *   now owned by the OpalT38Protocol object instead of H323Channel.
 *
 * Revision 1.9  2002/01/01 23:27:50  craigs
 * Added CleanupOnTermination functions
 * Thanks to Vyacheslav Frolov
 *
 * Revision 1.8  2001/12/22 22:18:07  craigs
 * Canged to ignore subsequent PDUs with identical sequence numbers
 *
 * Revision 1.7  2001/12/22 01:56:51  robertj
 * Cleaned up code and allowed for repeated sequence numbers.
 *
 * Revision 1.6  2001/12/19 09:15:43  craigs
 * Added changes from Vyacheslav Frolov
 *
 * Revision 1.5  2001/12/14 08:36:36  robertj
 * More implementation of T.38, thanks Adam Lazur
 *
 * Revision 1.4  2001/11/11 23:18:53  robertj
 * MSVC warnings removed.
 *
 * Revision 1.3  2001/11/11 23:07:52  robertj
 * Some clean ups after T.38 commit, thanks Adam Lazur
 *
 * Revision 1.2  2001/11/09 05:39:54  craigs
 * Added initial T.38 support thanks to Adam Lazur
 *
 * Revision 1.1  2001/07/17 04:44:32  robertj
 * Partial implementation of T.120 and T.38 logical channels.
 *
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
    FALSE, // No jitter for data
    1440, // 100's bits/sec
    0,
    0,
    0);
  return opalT38;
}

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
                    TRUE,
                    8*frameSize*AudioClockRate/frameTime,  // bits per second = 8*frameSize * framesPerSecond
                    frameSize,
                    frameTime,
                    clockRate,
                    timeStamp)
{
  AddOption(new OpalMediaOptionUnsigned(OpalAudioFormat::RxFramesPerPacketOption(), false, OpalMediaOption::MinMerge, rxFrames, 1, maxFrames));
  AddOption(new OpalMediaOptionUnsigned(OpalAudioFormat::TxFramesPerPacketOption(), false, OpalMediaOption::MinMerge, txFrames, 1, maxFrames));
}

/////////////////////////////////////////////////////////////////////////////

OpalT38Protocol::OpalT38Protocol()
{
  transport = NULL;
  autoDeleteTransport = FALSE;
  corrigendumASN = TRUE;
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


void OpalT38Protocol::SetTransport(H323Transport * t, BOOL autoDelete)
{
  if (transport != t) {
    if (autoDeleteTransport)
      delete transport;

    transport = t;
  }

  autoDeleteTransport = autoDelete;
}


BOOL OpalT38Protocol::Originate()
{
  PTRACE(3, "T38\tOriginate, transport=" << *transport);

  // Application would normally override this. The default just sends
  // a "heartbeat".
  while (WriteIndicator(T38_Type_of_msg_t30_indicator::e_no_signal))
    PThread::Sleep(500);

  return FALSE;
}


BOOL OpalT38Protocol::WritePacket(const T38_IFPPacket & ifp)
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
    return FALSE;
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

  return TRUE;
}


BOOL OpalT38Protocol::WriteIndicator(unsigned indicator)
{
  T38_IFPPacket ifp;

  ifp.SetTag(T38_Type_of_msg::e_t30_indicator);
  T38_Type_of_msg_t30_indicator & ind = ifp.m_type_of_msg;
  ind.SetValue(indicator);

  return WritePacket(ifp);
}


BOOL OpalT38Protocol::WriteMultipleData(unsigned mode,
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


BOOL OpalT38Protocol::WriteData(unsigned mode, unsigned type, const PBYTEArray & data)
{
  return WriteMultipleData(mode, 1, &type, &data);
}


BOOL OpalT38Protocol::Answer()
{
  PTRACE(3, "T38\tAnswer, transport=" << *transport);

  // Should probably get this from the channel open negotiation, but for
  // the time being just accept from whoever sends us something first
  transport->SetPromiscuous(H323Transport::AcceptFromAnyAutoSet);

  int consecutiveBadPackets = 0;
  int expectedSequenceNumber = 0;	// 16 bit
  BOOL firstPacket = TRUE;

  for (;;) {
    PPER_Stream rawData;
    if (!transport->ReadPDU(rawData)) {
      PTRACE(1, "T38\tError reading PDU: " << transport->GetErrorText(PChannel::LastReadError));
      return FALSE;
    }

    /* when we get the first packet, set the RemoteAddress and then turn off
     * promiscuous listening */
    if (firstPacket) {
      PTRACE(3, "T38\tReceived first packet, remote=" << transport->GetRemoteAddress());
      transport->SetPromiscuous(H323Transport::AcceptFromRemoteOnly);
      firstPacket = FALSE;
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
        return FALSE;
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
            return FALSE;
          }
          lostPackets = nRedundancy;
        }
        while (lostPackets > 0) {
          if (!HandleRawIFP(secondary[lostPackets++])) {
            PTRACE(1, "T38\tHandle packet failed, aborting answer");
            return FALSE;
          }
        }
      }
      else {
        if (!HandlePacketLost(lostPackets)) {
          PTRACE(1, "T38\tHandle lost packet, aborting answer");
          return FALSE;
        }
      }
    }

    if (!HandleRawIFP(udptl.m_primary_ifp_packet)) {
      PTRACE(1, "T38\tHandle packet failed, aborting answer");
      return FALSE;
    }
  }
}


BOOL OpalT38Protocol::HandleRawIFP(const PASN_OctetString & pdu)
{
  T38_IFPPacket ifp;

  if (corrigendumASN) {
    if (pdu.DecodeSubType(ifp))
      return HandlePacket(ifp);

    PTRACE(2, "T38\tIFP decode failure:\n  " << setprecision(2) << ifp);
    return TRUE;
  }

  T38_PreCorrigendum_IFPPacket old_ifp;
  if (!pdu.DecodeSubType(old_ifp)) {
    PTRACE(2, "T38\tPre-corrigendum IFP decode failure:\n  " << setprecision(2) << old_ifp);
    return TRUE;
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


BOOL OpalT38Protocol::HandlePacket(const T38_IFPPacket & ifp)
{
  if (ifp.m_type_of_msg.GetTag() == T38_Type_of_msg::e_t30_indicator)
    return OnIndicator((T38_Type_of_msg_t30_indicator)ifp.m_type_of_msg);

  for (PINDEX i = 0; i < ifp.m_data_field.GetSize(); i++) {
    if (!OnData((T38_Type_of_msg_data)ifp.m_type_of_msg,
                ifp.m_data_field[i].m_field_type,
                ifp.m_data_field[i].m_field_data.GetValue()))
      return FALSE;
  }
  return TRUE;
}


BOOL OpalT38Protocol::OnIndicator(unsigned indicator)
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

  return TRUE;
}


BOOL OpalT38Protocol::OnCNG()
{
  return TRUE;
}


BOOL OpalT38Protocol::OnCED()
{
  return TRUE;
}


BOOL OpalT38Protocol::OnPreamble()
{
  return TRUE;
}


BOOL OpalT38Protocol::OnTraining(unsigned /*indicator*/)
{
  return TRUE;
}


BOOL OpalT38Protocol::OnData(unsigned /*mode*/,
                             unsigned /*type*/,
                             const PBYTEArray & /*data*/)
{
  return TRUE;
}


#if PTRACING
#define PTRACE_nLost nLost
#else
#define PTRACE_nLost
#endif

BOOL OpalT38Protocol::HandlePacketLost(unsigned PTRACE_nLost)
{
  PTRACE(2, "T38\tHandlePacketLost, n=" << PTRACE_nLost);
  /* don't handle lost packets yet */
  return TRUE;
}

/////////////////////////////////////////////////////////////////////////////

T38PseudoRTP::T38PseudoRTP(PHandleAggregator * _aggregator, unsigned _id, BOOL _remoteIsNAT)
  : RTP_UDP(_aggregator, _id, _remoteIsNAT)
{
  PTRACE(4, "RTP_T38\tPseudoRTP session created with NAT flag set to " << remoteIsNAT);

  corrigendumASN = TRUE;
  consecutiveBadPackets = 0;

  reportTimer = 20;

  lastIFP.SetSize(0);
}

T38PseudoRTP::~T38PseudoRTP()
{
  PTRACE(4, "RTP_T38\tPseudoRTP session destroyed");
}

BOOL T38PseudoRTP::SetRemoteSocketInfo(PIPSocket::Address address, WORD port, BOOL isDataPort)
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

BOOL T38PseudoRTP::WriteData(RTP_DataFrame & frame)
{
  if (shutdownWrite) {
    PTRACE(3, "RTP_T38\tSession " << sessionID << ", Write shutdown.");
    shutdownWrite = FALSE;
    return FALSE;
  }

  // Trying to send a PDU before we are set up!
  if (!remoteAddress.IsValid() || remoteDataPort == 0)
    return TRUE;

  switch (OnSendData(frame)) {
    case e_ProcessPacket :
      break;
    case e_IgnorePacket :
      return TRUE;
    case e_AbortTransport :
      return FALSE;
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
        return FALSE;
    }
  }

  return TRUE;
}


RTP_Session::SendReceiveStatus T38PseudoRTP::OnSendControl(RTP_ControlFrame & /*frame*/, PINDEX & /*len*/)
{
  return e_IgnorePacket; // Non fatal error, just ignore
}

RTP_Session::SendReceiveStatus T38PseudoRTP::ReadDataPDU(RTP_DataFrame & frame)
{
  SendReceiveStatus status = ReadDataOrControlPDU(*dataSocket, frame, TRUE);
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

BOOL T38PseudoRTP::ReadData(RTP_DataFrame & frame, BOOL loop)
{
  do {
    int selectStatus = WaitForPDU(*dataSocket, *controlSocket, reportTimer);

    if (shutdownRead) {
      PTRACE(3, "T38_RTP\tSession " << sessionID << ", Read shutdown.");
      shutdownRead = FALSE;
      return FALSE;
    }

    switch (selectStatus) {
      case -2 :
        if (ReadControlPDU() == e_AbortTransport)
          return FALSE;
        break;

      case -3 :
        if (ReadControlPDU() == e_AbortTransport)
          return FALSE;
        // Then do -1 case

      case -1 :
        switch (ReadDataPDU(frame)) {
          case e_ProcessPacket :
            if (!shutdownRead)
              return TRUE;
          case e_IgnorePacket :
            break;
          case e_AbortTransport :
            return FALSE;
        }
        break;

      case 0 :
        // for timeouts, send a "fake" payload of one byte of 0xff to keep the T.38 engine emitting PCM
        frame.SetPayloadSize(1);
        frame.GetPayloadPtr()[0] = 0xff;
        return TRUE;

      case PSocket::Interrupted:
        PTRACE(3, "T38_RTP\tSession " << sessionID << ", Interrupted.");
        return FALSE;

      default :
        PTRACE(1, "T38_RTP\tSession " << sessionID << ", Select error: "
                << PChannel::GetErrorText((PChannel::Errors)selectStatus));
        return FALSE;
    }
  } while (loop);

  frame.SetSize(0);
  return TRUE;
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

OpalFaxMediaStream::OpalFaxMediaStream(OpalConnection & conn, const OpalMediaFormat & mediaFormat, unsigned sessionID, BOOL isSource, const PString & _token, const PString & _filename)
  : OpalMediaStream(conn, mediaFormat, sessionID, isSource), sessionToken(_token), filename(_filename)
{
  faxCallInfo = NULL;
  SetDataSize(300);
}

BOOL OpalFaxMediaStream::Open()
{
  if (sessionToken.IsEmpty()) {
    PTRACE(1, "T38\tCannot open unknown media stream");
    return FALSE;
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
        return FALSE;
      }
      faxCallInfoMap.insert(OpalFaxCallInfoMap_T::value_type((const char *)sessionToken, faxCallInfo));
    }
  }

  return OpalMediaStream::Open();
}

BOOL OpalFaxMediaStream::Start()
{
  PWaitAndSignal m(infoMutex);
  if (faxCallInfo == NULL) {
    PTRACE(1, "Fax\tCannot start unknown media stream");
    return FALSE;
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
      return FALSE;
    }

    if (!faxCallInfo->spanDSP.Execute()) {
      PTRACE(1, "Fax\tCannot execute SpanDSP");
      return FALSE;
    }
  }

  return TRUE;
}

BOOL OpalFaxMediaStream::ReadPacket(RTP_DataFrame & packet)
{
  // it is possible for ReadPacket to be called before the media stream has been opened, so deal with that case
  PWaitAndSignal m(infoMutex);
  if ((faxCallInfo == NULL) || !faxCallInfo->spanDSP.IsRunning()) {

    // return silence
    packet.SetPayloadSize(0);

  } else {

    packet.SetSize(2048);

    if (faxCallInfo->spanDSPPort > 0) {
      if (!faxCallInfo->socket.Read(packet.GetPointer()+RTP_DataFrame::MinHeaderSize, packet.GetSize()-RTP_DataFrame::MinHeaderSize))
        return FALSE;
    } else{ 
      if (!faxCallInfo->socket.ReadFrom(packet.GetPointer()+RTP_DataFrame::MinHeaderSize, packet.GetSize()-RTP_DataFrame::MinHeaderSize, 
                                        faxCallInfo->spanDSPAddr, faxCallInfo->spanDSPPort))
        return FALSE;
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

  return TRUE;
}

BOOL OpalFaxMediaStream::WritePacket(RTP_DataFrame & packet)
{
  PWaitAndSignal m(infoMutex);
  if ((faxCallInfo == NULL) || !faxCallInfo->spanDSP.IsRunning()) {
   
    // return silence
    packet.SetPayloadSize(0);

  } else {

    if (!faxCallInfo->spanDSP.IsRunning()) {
      PTRACE(1, "Fax\tspandsp ended");
      return FALSE;
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
            return FALSE;
          }
          len = sizeof(writeBuffer);
        }
        else {
          len = sizeof(writeBuffer) - writeBufferLen;
          memcpy(writeBuffer + writeBufferLen, ptr, len);
          if (!faxCallInfo->socket.WriteTo(writeBuffer, sizeof(writeBuffer), faxCallInfo->spanDSPAddr, faxCallInfo->spanDSPPort)) {
            PTRACE(1, "Fax\tSocket write error - " << faxCallInfo->socket.GetErrorText(PChannel::LastWriteError));
            return FALSE;
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
          return FALSE;
        }
        writeBufferLen = 0;
      }
    }
  }

  return TRUE;
}

BOOL OpalFaxMediaStream::Close()
{
  BOOL stat = OpalMediaStream::Close();

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

BOOL OpalFaxMediaStream::IsSynchronous() const
{
  return TRUE;
}

PString OpalFaxMediaStream::GetSpanDSPCommandLine(OpalFaxCallInfo & info)
{
  PStringStream cmdline;

  PIPSocket::Address dummy; WORD port;
  info.socket.GetLocalAddress(dummy, port);

  cmdline << "spandsp_util -m tiff_to_fax -n '" << filename << "' -f 127.0.0.1:" << port;

  return cmdline;
}

/////////////////////////////////////////////////////////////////////////////

/**This class describes a media stream that transfers data to/from a T.38 session
  */
OpalT38MediaStream::OpalT38MediaStream(
      OpalConnection & conn,
      const OpalMediaFormat & mediaFormat, ///<  Media format for stream
      unsigned sessionID, 
      BOOL isSource,                       ///<  Is a source stream
      const PString & token,               ///<  token used to match incoming/outgoing streams
      const PString & _filename
    )
  : OpalFaxMediaStream(conn, mediaFormat, sessionID, isSource, token, _filename)
{
}

PString OpalT38MediaStream::GetSpanDSPCommandLine(OpalFaxCallInfo & info)
{
  PStringStream cmdline;

  PIPSocket::Address dummy; WORD port;
  info.socket.GetLocalAddress(dummy, port);

  cmdline << "spandsp_util -V 0 -m tiff_to_t38 -n '" << filename << "' -t 127.0.0.1:" << port;

  return cmdline;
}

BOOL OpalT38MediaStream::ReadPacket(RTP_DataFrame & packet)
{
  if (faxCallInfo == NULL)
    return FALSE;

  if (!faxCallInfo->spanDSP.IsRunning()) {
    PTRACE(1, "Fax\tspandsp ended");
    return FALSE;
  }

  packet.SetSize(2048);

  if (faxCallInfo->spanDSPPort > 0) {
    if (!faxCallInfo->socket.Read(packet.GetPointer(), packet.GetSize()))
      return FALSE;
  } else{ 
    if (!faxCallInfo->socket.ReadFrom(packet.GetPointer(), packet.GetSize(), faxCallInfo->spanDSPAddr, faxCallInfo->spanDSPPort))
      return FALSE;
  }

  PINDEX len = faxCallInfo->socket.GetLastReadCount();
  if (len < RTP_DataFrame::MinHeaderSize)
    return FALSE;

  packet.SetSize(len);
  packet.SetPayloadSize(len - RTP_DataFrame::MinHeaderSize);

//PTRACE(1, "Fax\tT.38 Read RTP payload size = " << packet.GetPayloadSize());

  return TRUE;
}

BOOL OpalT38MediaStream::WritePacket(RTP_DataFrame & packet)
{
  if (faxCallInfo == NULL)
    return FALSE;

  if (!faxCallInfo->spanDSP.IsRunning()) {
    PTRACE(1, "Fax\tspandsp ended");
    return FALSE;
  }

//PTRACE(1, "Fax\tT.38 Write RTP payload size = " << packet.GetPayloadSize());

  if (faxCallInfo->spanDSPPort > 0) {
    PTRACE(1, "Fax\tT.38 Write RTP packet size = " << packet.GetHeaderSize() + packet.GetPayloadSize() <<" to " << faxCallInfo->spanDSPAddr << ":" << faxCallInfo->spanDSPPort);
    if (!faxCallInfo->socket.WriteTo(packet.GetPointer(), packet.GetHeaderSize() + packet.GetPayloadSize(), faxCallInfo->spanDSPAddr, faxCallInfo->spanDSPPort)) {
      PTRACE(1, "T38_UDP\tSocket write error - " << faxCallInfo->socket.GetErrorText(PChannel::LastWriteError));
      return FALSE;
    }
  }

  return TRUE;
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

OpalFaxConnection * OpalFaxEndPoint::CreateConnection(OpalCall & call, const PString & filename, void * /*userData*/, OpalConnection::StringOptions * stringOptions)
{
  return new OpalFaxConnection(call, *this, filename, MakeToken(), stringOptions);
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

BOOL OpalFaxEndPoint::MakeConnection(OpalCall & call,
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
  if (!PFile::Exists(filename)) {
    return FALSE;
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

  PSafePtr<OpalFaxConnection> connection = PSafePtrCast<OpalConnection, OpalFaxConnection>(GetConnectionWithLock(MakeToken()));
  if (connection != NULL)
    return FALSE;

  connection = CreateConnection(call, filename, userData, stringOptions);
  if (connection == NULL)
    return FALSE;

  connectionsActive.SetAt(connection->GetToken(), connection);

  return TRUE;
}

void OpalFaxEndPoint::OnPatchMediaStream(const OpalFaxConnection & /*connection*/, BOOL /*isSource*/, OpalMediaPatch & /*patch*/)
{
}

/////////////////////////////////////////////////////////////////////////////

OpalFaxConnection::OpalFaxConnection(OpalCall & call, OpalFaxEndPoint & ep, const PString & _filename, const PString & _token, OpalConnection::StringOptions * stringOptions)
  : OpalConnection(call, ep, _token), endpoint(ep), filename(_filename)
{
  PTRACE(3, "FAX\tCreated FAX connection with token '" << callToken << "'");
  phase = SetUpPhase;

  forceFaxAudio = (stringOptions == NULL) || stringOptions->Contains("Force-Fax-Audio");
}

OpalFaxConnection::~OpalFaxConnection()
{
  PTRACE(3, "FAX\tDeleted FAX connection.");
}

OpalMediaStream * OpalFaxConnection::CreateMediaStream(const OpalMediaFormat & mediaFormat, unsigned sessionID, BOOL isSource)
{
  // if creating an audio session, use a NULL stream
  if (sessionID == OpalMediaFormat::DefaultAudioSessionID) {
    if (forceFaxAudio && (mediaFormat == OpalPCM16))
      return new OpalFaxMediaStream(*this, mediaFormat, sessionID, isSource, GetToken(), filename);
    else
      return new OpalNullMediaStream(*this, mediaFormat, sessionID, isSource);
  }

  // if creating a data stream, see what type it is
  else if (!forceFaxAudio && (sessionID == OpalMediaFormat::DefaultDataSessionID)) {
    if (mediaFormat == OpalPCM16Fax)
      return new OpalFaxMediaStream(*this, mediaFormat, sessionID, isSource, GetToken(), filename);
  }

  // else call ancestor
  return NULL;
}

BOOL OpalFaxConnection::SetUpConnection()
{
  // Check if we are A-Party in thsi call, so need to do things differently
  if (ownerCall.GetConnection(0) == this) {
    phase = SetUpPhase;
    if (!OnIncomingConnection(0, NULL)) {
      Release(EndedByCallerAbort);
      return FALSE;
    }

    PTRACE(2, "FAX\tOutgoing call routed to " << ownerCall.GetPartyB() << " for " << *this);
    if (!ownerCall.OnSetUp(*this)) {
      Release(EndedByNoAccept);
      return FALSE;
    }

    return TRUE;
  }

  {
    PSafePtr<OpalConnection> otherConn = ownerCall.GetOtherPartyConnection(*this);
    if (otherConn == NULL)
      return FALSE;

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

  return TRUE;
}


BOOL OpalFaxConnection::SetAlerting(const PString & calleeName, BOOL)
{
  PTRACE(3, "Fax\tSetAlerting(" << calleeName << ')');
  phase = AlertingPhase;
  remotePartyName = calleeName;
  //return endpoint.OnShowOutgoing(*this);
  return TRUE;
}


BOOL OpalFaxConnection::SetConnected()
{
  PTRACE(3, "Fax\tSetConnected()");

  {
    PSafeLockReadWrite safeLock(*this);
    if (!safeLock.IsLocked())
      return FALSE;

    if (mediaStreams.IsEmpty())
      return TRUE;

    phase = EstablishedPhase;
  }

  OnEstablished();

  return TRUE;
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

void OpalFaxConnection::OnPatchMediaStream(BOOL isSource, OpalMediaPatch & patch)
{
  endpoint.OnPatchMediaStream(*this, isSource, patch);
}


BOOL OpalFaxConnection::OpenSourceMediaStream(const OpalMediaFormatList & mediaFormats, unsigned sessionID)
{
#if OPAL_VIDEO
  if (sessionID == OpalMediaFormat::DefaultVideoSessionID)
    return FALSE;
#endif

  return OpalConnection::OpenSourceMediaStream(mediaFormats, sessionID);
}


OpalMediaStream * OpalFaxConnection::OpenSinkMediaStream(OpalMediaStream & source)
{
#if OPAL_VIDEO
  if (source.GetSessionID() == OpalMediaFormat::DefaultVideoSessionID)
    return NULL;
#endif

  return OpalConnection::OpenSinkMediaStream(source);
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

  formats += OpalPCM16;        
  formats += OpalT38;        

  return formats;
}

OpalFaxConnection * OpalT38EndPoint::CreateConnection(OpalCall & call, const PString & filename, void * /*userData*/, OpalConnection::StringOptions * stringOptions)
{
  return new OpalT38Connection(call, *this, filename, MakeToken(), stringOptions);
}

PString OpalT38EndPoint::MakeToken()
{
  return psprintf("T38Connection_%i", ++faxCallIndex);
}

/////////////////////////////////////////////////////////////////////////////

OpalT38Connection::OpalT38Connection(OpalCall & call, OpalT38EndPoint & ep, const PString & _filename, const PString & _token, OpalConnection::StringOptions * stringOptions)
  : OpalFaxConnection(call, ep, _filename, _token, stringOptions)
{
  PTRACE(3, "FAX\tCreated T.38 connection with token '" << callToken << "'");
  forceFaxAudio = FALSE;
}

OpalMediaStream * OpalT38Connection::CreateMediaStream(const OpalMediaFormat & mediaFormat, unsigned sessionID, BOOL isSource)
{
  // if creating an audio session, use a NULL stream
  if (sessionID == OpalMediaFormat::DefaultAudioSessionID) 
    return new OpalNullMediaStream(*this, mediaFormat, sessionID, isSource);

  // if creating a data stream, see what type it is
  else if ((sessionID == OpalMediaFormat::DefaultDataSessionID) && (mediaFormat == OpalT38))
    return new OpalT38MediaStream(*this, mediaFormat, sessionID, isSource, GetToken(), filename);

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


