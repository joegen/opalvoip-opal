/*
 * t38proto.cxx
 *
 * T.38 protocol handler
 *
 * Open Phone Abstraction Library
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
 * Contributor(s): ______________________________________.
 *
 * $Log: t38proto.cxx,v $
 * Revision 1.2003  2002/01/14 06:35:59  robertj
 * Updated to OpenH323 v1.7.9
 *
 * Revision 2.1  2001/08/01 05:05:26  robertj
 * Major changes to H.323 capabilities, uses OpalMediaFormat for base name.
 *
 * Revision 2.0  2001/07/27 15:48:25  robertj
 * Conversion of OpenH323 to Open Phone Abstraction Library (OPAL)
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

#include <t38/t38proto.h>

#include <h323/transaddr.h>
#include <asn/t38.h>


#define new PNEW


OpalMediaFormat const OpalT38Protocol::MediaFormat("T.38",
                                                   OpalMediaFormat::DefaultDataSessionID,
                                                   RTP_DataFrame::MaxPayloadType,
                                                   FALSE, // No jitter for data
                                                   1440); // 100's bits/sec


/////////////////////////////////////////////////////////////////////////////

OpalT38Protocol::OpalT38Protocol()
{
}

OpalT38Protocol::~OpalT38Protocol()
{
}

void OpalT38Protocol::Close()
{
}

BOOL OpalT38Protocol::Originate(OpalTransport & transport)
{
  PTRACE(3, "T38\tOriginate, transport=" << transport);

  WORD seq = 0;	// 16 bit
  T38_IFPPacket ifp;

  while (PreparePacket(ifp)) {
    T38_UDPTLPacket udptl;

    udptl.m_seq_number = seq;
    udptl.m_primary_ifp_packet.EncodeSubType(ifp);

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
                " seq=" << seq <<
                " type=" << ifp.m_type_of_msg.GetTagName());
    }
#endif

    if(!transport.WritePDU(rawData) ) {
      PTRACE(1, "T38\tOriginate - WritePDU ERROR: " << transport.GetErrorText());
      break;
    }

    seq++;      // 16 bit
  }

  return FALSE;
}


BOOL OpalT38Protocol::Answer(OpalTransport & transport)
{
  PTRACE(3, "T38\tAnswer, transport=" << transport);

  /* HACK HACK HACK -- need to figure out how to get the remote address
   * properly here */
  transport.SetPromiscuous(TRUE);

  int consecutiveBadPackets = 0;
  WORD expectedSequenceNumber = 0;	// 16 bit

  for (;;) {
    PPER_Stream rawData;
    if (!transport.ReadPDU(rawData)) {
      PTRACE(1, "T38\tError reading PDU: " << transport.GetErrorText(PChannel::LastReadError));
      break;
    }

    /* when we get the first packet, set the RemoteAddress and then turn off
     * promiscuous listening */
    if (expectedSequenceNumber == 0) {
      PTRACE(3, "T38\tReceived first packet, remote=" << transport.GetRemoteAddress());
      transport.SetPromiscuous(FALSE);
    }

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
        break;
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

    WORD receivedSequenceNumber = (WORD)udptl.m_seq_number;	// 16 bit

#if PTRACING
    if (PTrace::CanTrace(4)) {
      PTRACE(4, "T38\tReceived PDU:\n  "
             << setprecision(2) << rawData << "\n  UDPTL = "
             << setprecision(2) << udptl << "\n  ifp = "
             << setprecision(2) << ifp);
    }
    else {
      PTRACE(3, "T38\tReceived PDU:"
                " seq=" << receivedSequenceNumber <<
                " type=" << ifp.m_type_of_msg.GetTagName());
    }
#endif

    if (receivedSequenceNumber < expectedSequenceNumber) {
      PTRACE(3, "T38\tRepeated packet");
      continue;
    } else if (receivedSequenceNumber > expectedSequenceNumber) {
      if (!HandlePacketLost(receivedSequenceNumber - expectedSequenceNumber))
        break;
      expectedSequenceNumber = (WORD)(receivedSequenceNumber+1);
    }
    else
      expectedSequenceNumber++;

    if (!HandlePacket(ifp))
      break;
  }

  return FALSE;
}


BOOL OpalT38Protocol::HandlePacket(const T38_IFPPacket & /*pdu*/)
{
  return TRUE;
}


BOOL OpalT38Protocol::PreparePacket(T38_IFPPacket & pdu)
{
  pdu.m_type_of_msg = T38_Type_of_msg::e_t30_indicator;
  pdu.m_type_of_msg = T38_Type_of_msg_t30_indicator::e_cng;
  // pdu.m_data_field = new T38_Data_Field();

  /* sleep for a second and loop... this is only for debugging and should be
   * removed */
  PThread::Sleep(1000);
  return TRUE;
}


BOOL OpalT38Protocol::HandlePacketLost(unsigned nLost)
{
  PTRACE(2, "T38\tHandlePacketLost, n=" << nLost);
  /* don't handle lost packets yet */
  return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
