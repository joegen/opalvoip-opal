/*
 * rtp.cxx
 *
 * RTP protocol handler
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
 * $Log: rtp.cxx,v $
 * Revision 1.2007  2002/02/11 09:32:13  robertj
 * Updated to openH323 v1.8.0
 *
 * Revision 2.5  2002/01/14 06:35:58  robertj
 * Updated to OpenH323 v1.7.9
 *
 * Revision 2.4  2002/01/14 02:24:06  robertj
 * Fixed possible divide by zero error in statistics.
 *
 * Revision 2.3  2001/12/07 08:52:28  robertj
 * Used new const PWaitAndSignal
 *
 * Revision 2.2  2001/11/14 06:20:40  robertj
 * Changed sending of control channel reports to be timer based.
 *
 * Revision 2.1  2001/10/05 00:22:14  robertj
 * Updated to PWLib 1.2.0 and OpenH323 1.7.0
 *
 * Revision 2.0  2001/07/27 15:48:25  robertj
 * Conversion of OpenH323 to Open Phone Abstraction Library (OPAL)
 *
 * Revision 1.71  2002/02/09 02:33:49  robertj
 * Improved payload type docuemntation and added Cisco CN.
 *
 * Revision 1.70  2001/12/20 04:34:56  robertj
 * Fixed display of some of the unknown RTP types.
 *
 * Revision 1.69  2001/09/12 07:48:05  robertj
 * Fixed various problems with tracing.
 *
 * Revision 1.68  2001/09/11 00:21:24  robertj
 * Fixed missing stack sizes in endpoint for cleaner thread and jitter thread.
 *
 * Revision 1.67  2001/09/10 08:24:04  robertj
 * Fixed setting of destination RTP address so works with endpoints that
 *   do not get the OLC packets quite right.
 *
 * Revision 1.66  2001/07/11 03:23:54  robertj
 * Bug fixed where every 65536 PDUs the session thinks it is the first PDU again.
 *
 * Revision 1.65  2001/07/06 06:32:24  robertj
 * Added flag and checks for RTP data having specific SSRC.
 * Changed transmitter IP address check so is based on first received
 *    PDU instead of expecting it to come from the host we are sending to.
 *
 * Revision 1.64  2001/06/04 13:43:44  robertj
 * Fixed I/O block breaker code if RTP session is bound to INADDR_ANY interface.
 *
 * Revision 1.63  2001/06/04 11:37:50  robertj
 * Added thread safe enumeration functions of RTP sessions.
 * Added member access functions to UDP based RTP sessions.
 *
 * Revision 1.62  2001/05/24 01:12:23  robertj
 * Fixed sender report timestamp field, thanks Johan Gnosspelius
 *
 * Revision 1.61  2001/05/08 05:28:02  yurik
 * No ifdef _WIN32_WCE anymore - 3+ version of  SDK allows it
 *
 * Revision 1.60  2001/04/24 06:15:50  robertj
 * Added work around for strange Cisco bug which suddenly starts sending
 *   RTP packets beginning at a difference sequence number base.
 *
 * Revision 1.59  2001/04/02 23:58:24  robertj
 * Added jitter calculation to RTP session.
 * Added trace of statistics.
 *
 * Revision 1.58  2001/03/20 07:24:05  robertj
 * Changed RTP SDES to have simple host name instead of attempting a
 *   reverse DNS lookup on IP address as badly configured DNS can cause
 *   a 30 second delay before audio is sent.
 *
 * Revision 1.57  2001/03/15 05:45:45  robertj
 * Changed, yet again, the setting of RTP info to allow for Cisco idosyncracies.
 *
 * Revision 1.56  2001/02/21 08:08:26  robertj
 * Added more debugging.
 *
 * Revision 1.55  2001/02/09 05:13:56  craigs
 * Added pragma implementation to (hopefully) reduce the executable image size
 * under Linux
 *
 * Revision 1.54  2001/01/28 07:06:59  yurik
 * WinCE-port - avoid using of IP_TOS
 *
 * Revision 1.53  2000/12/18 08:59:20  craigs
 * Added ability to set ports
 *
 * Revision 1.52  2000/09/25 22:27:40  robertj
 * Fixed uninitialised variable for lastRRSequenceNumber
 *
 * Revision 1.51  2000/09/25 01:53:20  robertj
 * Fixed MSVC warnings.
 *
 * Revision 1.50  2000/09/25 01:44:13  robertj
 * Fixed possible race condition on shutdown of RTP session with jitter buffer.
 *
 * Revision 1.49  2000/09/22 00:32:34  craigs
 * Added extra logging
 * Fixed problems with no fastConnect with tunelling
 *
 * Revision 1.48  2000/09/21 02:06:07  craigs
 * Added handling for endpoints that return conformant, but useless, RTP address
 * and port numbers
 *
 * Revision 1.47  2000/08/17 00:42:39  robertj
 * Fixed RTP Goodbye message reason string parsing, thanks Thien Nguyen.
 *
 * Revision 1.46  2000/05/30 10:35:41  robertj
 * Fixed GNU compiler warning.
 *
 * Revision 1.45  2000/05/30 06:52:26  robertj
 * Fixed problem with Cisco restarting sequence numbers when changing H.323 logical channels.
 *
 * Revision 1.44  2000/05/23 12:57:37  robertj
 * Added ability to change IP Type Of Service code from applications.
 *
 * Revision 1.43  2000/05/12 00:27:35  robertj
 * Fixed bug in UseSession() that caused asserts on BSD and possible race condition everywhere.
 *
 * Revision 1.42  2000/05/04 11:52:35  robertj
 * Added Packets Too Late statistics, requiring major rearrangement of jitter
 *    buffer code, not also changes semantics of codec Write() function slightly.
 *
 * Revision 1.41  2000/05/02 04:32:27  robertj
 * Fixed copyright notice comment.
 *
 * Revision 1.40  2000/05/01 01:01:49  robertj
 * Added flag for what to do with out of orer packets (use if jitter, don't if not).
 *
 * Revision 1.39  2000/04/30 03:55:09  robertj
 * Improved the RTCP messages, epecially reports
 *
 * Revision 1.38  2000/04/28 12:56:39  robertj
 * Fixed transmission of SDES record in RTCP channel.
 *
 * Revision 1.37  2000/04/19 01:50:05  robertj
 * Improved debugging messages.
 *
 * Revision 1.36  2000/04/13 18:07:39  robertj
 * Fixed missing mutex release causing possible deadlocks.
 *
 * Revision 1.35  2000/04/10 17:40:05  robertj
 * Fixed debug output of RTP payload types to allow for unknown numbers.
 *
 * Revision 1.34  2000/04/05 04:09:24  robertj
 * Fixed portability problem with max() macro.
 *
 * Revision 1.33  2000/04/05 03:17:32  robertj
 * Added more RTP statistics gathering and H.245 round trip delay calculation.
 *
 * Revision 1.32  2000/04/03 18:15:44  robertj
 * Added "fractional" part of RTCP status NTP timestamp field.
 *
 * Revision 1.31  2000/03/23 02:54:57  robertj
 * Added sending of SDES control packets.
 *
 * Revision 1.30  2000/03/20 20:53:42  robertj
 * Fixed problem with being able to reopen for reading an RTP_Session (Cisco compatibilty)
 *
 * Revision 1.29  2000/03/04 12:32:23  robertj
 * Added setting of TOS field in IP header to get poor mans QoS on some routers.
 *
 * Revision 1.28  2000/02/29 13:00:13  robertj
 * Added extra statistic display for RTP packets out of order.
 *
 * Revision 1.27  2000/02/29 02:13:56  robertj
 * Fixed RTP receive of both control and data, ignores ECONNRESET/ECONNREFUSED errors.
 *
 * Revision 1.26  2000/02/27 10:56:24  robertj
 * Fixed error in code allowing non-consecutive RTP port numbers coming from broken stacks, thanks Vassili Leonov.
 *
 * Revision 1.25  2000/02/17 12:07:44  robertj
 * Used ne wPWLib random number generator after finding major problem in MSVC rand().
 *
 * Revision 1.24  2000/01/29 07:10:20  robertj
 * Fixed problem with RTP transmit to host that is not yet ready causing the
 *   receive side to die with ECONNRESET, thanks Ian MacDonald
 *
 * Revision 1.23  2000/01/20 05:57:46  robertj
 * Added extra flexibility in receiving incorrectly formed OpenLogicalChannel PDU's
 *
 * Revision 1.22  2000/01/14 00:31:40  robertj
 * Bug fix for RTP port allocation (MSVC optimised version), Thanks to Ian MacDonald.
 *
 * Revision 1.21  1999/12/30 09:14:49  robertj
 * Changed payload type functions to use enum.
 *
 * Revision 1.20  1999/12/23 23:02:36  robertj
 * File reorganision for separating RTP from H.323 and creation of LID for VPB support.
 *
 * Revision 1.19  1999/11/29 04:50:11  robertj
 * Added adaptive threshold calculation to silence detection.
 *
 * Revision 1.18  1999/11/22 00:09:59  robertj
 * Fixed error in RTP transmit rewrite for ternary state, didn't transmit at all!
 *
 * Revision 1.17  1999/11/20 05:35:48  robertj
 * Fixed possibly I/O block in RTP read loops.
 *
 * Revision 1.16  1999/11/19 13:13:31  robertj
 * Fixed Windows 95 compatibility issue in shutdown of RTP reading.
 *
 * Revision 1.15  1999/11/19 10:23:16  robertj
 * Fixed local binding of socket to correct address on multi-homes systems.
 *
 * Revision 1.14  1999/11/19 09:17:15  robertj
 * Fixed problems with aycnhronous shut down of logical channels.
 *
 * Revision 1.13  1999/11/17 03:49:51  robertj
 * Added RTP statistics display.
 *
 * Revision 1.12  1999/11/14 11:41:07  robertj
 * Added access functions to RTP statistics.
 *
 * Revision 1.11  1999/10/18 23:55:11  robertj
 * Fixed bug in setting contributing sources, length put into wrong spot in PDU header
 *
 * Revision 1.10  1999/09/21 14:10:04  robertj
 * Removed warnings when no tracing enabled.
 *
 * Revision 1.9  1999/09/05 00:58:37  robertj
 * Removed requirement that OpenLogicalChannalAck sessionId match original OLC.
 *
 * Revision 1.8  1999/09/03 02:17:50  robertj
 * Added more debugging
 *
 * Revision 1.7  1999/08/31 12:34:19  robertj
 * Added gatekeeper support.
 *
 * Revision 1.6  1999/07/16 02:13:54  robertj
 * Slowed down the control channel statistics packets.
 *
 * Revision 1.5  1999/07/13 09:53:24  robertj
 * Fixed some problems with jitter buffer and added more debugging.
 *
 * Revision 1.4  1999/07/09 06:09:52  robertj
 * Major implementation. An ENORMOUS amount of stuff added everywhere.
 *
 * Revision 1.3  1999/06/22 13:49:40  robertj
 * Added GSM support and further RTP protocol enhancements.
 *
 * Revision 1.2  1999/06/14 08:42:52  robertj
 * Fixed bug in dropped packets display was negative.
 *
 * Revision 1.1  1999/06/14 06:12:25  robertj
 * Changes for using RTP sessions correctly in H323 Logical Channel context
 *
 */

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "rtp.h"
#endif

#include <rtp/rtp.h>

#include <ptclib/random.h>
#include <rtp/jitter.h>


#define new PNEW

#if !PTRACING // Stuff to remove unised parameters warning
#define PTRACE_sender
#define PTRACE_reports
#define PTRACE_count
#define PTRACE_src
#define PTRACE_description
#define PTRACE_reason
#define PTRACE_type
#define PTRACE_subtype
#define PTRACE_size
#define PTRACE_name
#define PTRACE_port
#endif


const unsigned SecondsFrom1900to1970 = (70*365+17)*24*60*60U;

#define UDP_BUFFER_SIZE 32768


/////////////////////////////////////////////////////////////////////////////

RTP_DataFrame::RTP_DataFrame(PINDEX sz)
  : PBYTEArray(12+sz)
{
  payloadSize = sz;
  theArray[0] = '\x80';
}


void RTP_DataFrame::SetExtension(BOOL ext)
{
  if (ext)
    theArray[0] |= 0x10;
  else
    theArray[0] &= 0xef;
}


void RTP_DataFrame::SetMarker(BOOL m)
{
  if (m)
    theArray[1] |= 0x80;
  else
    theArray[1] &= 0x7f;
}


void RTP_DataFrame::SetPayloadType(PayloadTypes t)
{
  PAssert(t <= 0x7f, PInvalidParameter);

  theArray[1] &= 0x80;
  theArray[1] |= t;
}


DWORD RTP_DataFrame::GetContribSource(PINDEX idx) const
{
  PAssert(idx < GetContribSrcCount(), PInvalidParameter);
  return ((PUInt32b *)&theArray[12])[idx];
}


void RTP_DataFrame::SetContribSource(PINDEX idx, DWORD src)
{
  PAssert(idx <= 15, PInvalidParameter);

  if (idx >= GetContribSrcCount()) {
    BYTE * oldPayload = GetPayloadPtr();
    theArray[0] &= 0xf0;
    theArray[0] |= idx+1;
    SetSize(GetHeaderSize()+payloadSize);
    memmove(GetPayloadPtr(), oldPayload, payloadSize);
  }

  ((PUInt32b *)&theArray[12])[idx] = src;
}


BOOL RTP_DataFrame::SetPayloadSize(PINDEX sz)
{
  payloadSize = sz;
  return SetMinSize(GetHeaderSize()+payloadSize);
}


#if PTRACING
static const char * const PayloadTypesNames[RTP_DataFrame::LastKnownPayloadType] = {
  "PCMU",
  "FS1016",
  "G721",
  "GSM",
  "G7231",
  "DVI4_8k",
  "DVI4_16k",
  "LPC",
  "PCMA",
  "G722",
  "L16_Stereo",
  "L16_Mono",
  "G723",
  "CN",
  "MPA",
  "G728",
  "DVI4_11k",
  "DVI4_22k",
  "G729",
  "CiscoCN",
  NULL, NULL, NULL, NULL, NULL,
  "CelB",
  "JPEG",
  NULL, NULL, NULL, NULL,
  "H261",
  "MPV",
  "MP2T",
  "H263"
};

ostream & operator<<(ostream & o, RTP_DataFrame::PayloadTypes t)
{
  if ((PINDEX)t < PARRAYSIZE(PayloadTypesNames) && PayloadTypesNames[t] != NULL)
    o << PayloadTypesNames[t];
  else
    o << "[pt=" << (int)t << ']';
  return o;
}

#endif


/////////////////////////////////////////////////////////////////////////////

RTP_ControlFrame::RTP_ControlFrame(PINDEX sz)
{
  SetPayloadSize(sz);
  theArray[0] = '\x80';
}


void RTP_ControlFrame::SetCount(unsigned count)
{
  PAssert(count < 32, PInvalidParameter);
  theArray[0] &= 0xe0;
  theArray[0] |= count;
}


void RTP_ControlFrame::SetPayloadType(unsigned t)
{
  PAssert(t < 256, PInvalidParameter);
  theArray[1] = (BYTE)t;
}


void RTP_ControlFrame::SetPayloadSize(PINDEX sz)
{
  sz = (sz+3)/4;
  PAssert(sz <= 0xffff, PInvalidParameter);

  SetMinSize(4*(sz+1));
  *(PUInt16b *)&theArray[2] = (WORD)sz;
}


RTP_ControlFrame::SourceDescription & RTP_ControlFrame::AddSourceDescription(DWORD src)
{
  SetPayloadType(RTP_ControlFrame::e_SourceDescription);

  PINDEX index = GetCount();
  SetCount(index+1);

  PINDEX originalPayloadSize = index != 0 ? GetPayloadSize() : 0;
  SetPayloadSize(originalPayloadSize+sizeof(SourceDescription));
  SourceDescription & sdes = *(SourceDescription *)(GetPayloadPtr()+originalPayloadSize);
  sdes.src = src;
  sdes.item[0].type = e_END;
  return sdes;
}


RTP_ControlFrame::SourceDescription::Item &
        RTP_ControlFrame::AddSourceDescriptionItem(SourceDescription & sdes,
                                                   unsigned type,
                                                   const PString & data)
{
  PINDEX dataLength = data.GetLength();
  SetPayloadSize(GetPayloadSize()+sizeof(SourceDescription::Item)+dataLength-1);

  SourceDescription::Item * item = sdes.item;
  while (item->type != e_END)
    item = item->GetNextItem();

  item->type = (BYTE)type;
  item->length = (BYTE)dataLength;
  memcpy(item->data, (const char *)data, item->length);

  item->GetNextItem()->type = e_END;
  return *item;
}


void RTP_ControlFrame::ReceiverReport::SetLostPackets(unsigned packets)
{
  lost[0] = (BYTE)(packets >> 16);
  lost[1] = (BYTE)(packets >> 8);
  lost[2] = (BYTE)packets;
}


/////////////////////////////////////////////////////////////////////////////

void RTP_UserData::OnTxStatistics(const RTP_Session & /*session*/) const
{
}


void RTP_UserData::OnRxStatistics(const RTP_Session & /*session*/) const
{
}


/////////////////////////////////////////////////////////////////////////////

RTP_Session::RTP_Session(unsigned id, RTP_UserData * data)
  : senderReportTimer(0, 10),   // Ten seconds between tx reports
    receiverReportTimer(0, 10)  // Ten seconds between rx reports
{
  PAssert(id > 0 && id < 256, PInvalidParameter);
  sessionID = (BYTE)id;

  referenceCount = 1;
  userData = data;
  jitter = NULL;

  ignoreOtherSources = TRUE;
  ignoreOutOfOrderPackets = TRUE;
  syncSourceOut = PRandom::Number();
  syncSourceIn = 0;
  lastSentSequenceNumber = (WORD)PRandom::Number();
  expectedSequenceNumber = 0;
  lastRRSequenceNumber = 0;
  consecutiveOutOfOrderPackets = 0;

  packetsSent = 0;
  octetsSent = 0;
  packetsReceived = 0;
  octetsReceived = 0;
  packetsLost = 0;
  packetsOutOfOrder = 0;
  averageSendTime = 0;
  maximumSendTime = 0;
  minimumSendTime = 0;
  averageReceiveTime = 0;
  maximumReceiveTime = 0;
  minimumReceiveTime = 0;
  jitterLevel = 0;

  senderReportCount = 0;
  receiverReportCount = 0;
  averageSendTimeAccum = 0;
  maximumSendTimeAccum = 0;
  minimumSendTimeAccum = 0xffffffff;
  averageReceiveTimeAccum = 0;
  maximumReceiveTimeAccum = 0;
  minimumReceiveTimeAccum = 0xffffffff;
  lastTransitTime = 0;
}


RTP_Session::~RTP_Session()
{
  PTRACE_IF(2, packetsSent != 0 || packetsReceived != 0,
            "RTP\tFinal statistics:\n"
            "    packetsSent       = " << packetsSent << "\n"
            "    octetsSent        = " << octetsSent << "\n"
            "    averageSendTime   = " << averageSendTime << "\n"
            "    maximumSendTime   = " << maximumSendTime << "\n"
            "    minimumSendTime   = " << minimumSendTime << "\n"
            "    packetsReceived   = " << packetsReceived << "\n"
            "    octetsReceived    = " << octetsReceived << "\n"
            "    packetsLost       = " << packetsLost << "\n"
            "    packetsTooLate    = " << GetPacketsTooLate() << "\n"
            "    packetsOutOfOrder = " << packetsOutOfOrder << "\n"
            "    averageReceiveTime= " << averageReceiveTime << "\n"
            "    maximumReceiveTime= " << maximumReceiveTime << "\n"
            "    minimumReceiveTime= " << minimumReceiveTime << "\n"
            "    jitter            = " << (jitterLevel >> 7)
            );
  delete userData;
  delete jitter;
}


void RTP_Session::SetUserData(RTP_UserData * data)
{
  delete userData;
  userData = data;
}


void RTP_Session::SetJitterBufferSize(unsigned jitterDelay, PINDEX stackSize)
{
  if (jitterDelay == 0) {
    delete jitter;
    jitter = NULL;
  }
  else if (jitter != NULL)
    jitter->SetDelay(jitterDelay);
  else {
    SetIgnoreOutOfOrderPackets(FALSE);
    jitter = new RTP_JitterBuffer(*this, jitterDelay, stackSize);
  }
}


BOOL RTP_Session::ReadBufferedData(DWORD timestamp, RTP_DataFrame & frame)
{
  if (jitter != NULL)
    return jitter->ReadData(timestamp, frame);
  else
    return ReadData(frame);
}


void RTP_Session::SetSenderReportTime(const PTimeInterval & time)
{
  PAssert(time > 100, PInvalidParameter);

  senderReportTimer = time;
  senderReportCount = 0;
  averageSendTimeAccum = 0;
  maximumSendTimeAccum = 0;
  minimumSendTimeAccum = 0xffffffff;
}


void RTP_Session::SetReceiverReportTime(const PTimeInterval & time)
{
  PAssert(time > 100, PInvalidParameter);

  receiverReportTimer = time;
  receiverReportCount = 0;
  averageReceiveTimeAccum = 0;
  maximumReceiveTimeAccum = 0;
  minimumReceiveTimeAccum = 0xffffffff;
}


void RTP_Session::SetReceiverReport(RTP_ControlFrame::ReceiverReport & receiver)
{
  receiver.ssrc = syncSourceIn;
  receiver.fraction = (BYTE)(256*packetsLost/packetsReceived);
  receiver.SetLostPackets(packetsLost);
  receiver.last_seq = lastRRSequenceNumber;
  lastRRSequenceNumber = expectedSequenceNumber;
  receiver.jitter = jitterLevel >> 4; // Allow for rounding protection bits

  // The following have not been calculated yet.
  receiver.lsr = 0;
  receiver.dlsr = 0;

  PTRACE(3, "RTP\tSentReceiverReport:"
            " ssrc=" << receiver.ssrc
         << " fraction=" << (unsigned)receiver.fraction
         << " lost=" << receiver.GetLostPackets()
         << " last_seq=" << receiver.last_seq
         << " jitter=" << receiver.jitter
         << " lsr=" << receiver.lsr
         << " dlsr=" << receiver.dlsr);
}


RTP_Session::SendReceiveStatus RTP_Session::OnSendData(RTP_DataFrame & frame)
{
  PTimeInterval tick = PTimer::Tick();  // Timestamp set now

  frame.SetSequenceNumber(++lastSentSequenceNumber);
  frame.SetSyncSource(syncSourceOut);

  if (packetsSent == 0) {
    // First packet sent
    PString description = PProcess::Current().GetUserName() + '@' + GetLocalHostName();

    PTRACE(2, "RTP\tSending SDES: " << description);

    RTP_ControlFrame pdu;
    RTP_ControlFrame::SourceDescription & sdes = pdu.AddSourceDescription(syncSourceOut);
    pdu.AddSourceDescriptionItem(sdes, RTP_ControlFrame::e_CNAME, description);
    pdu.AddSourceDescriptionItem(sdes, RTP_ControlFrame::e_TOOL, PProcess::Current().GetName());
    if (!WriteControl(pdu))
      return e_AbortTransport;
  }
  else if (!frame.GetMarker()) {
    // Only do statistics on subsequent packets
    DWORD diff = (tick - lastSentPacketTime).GetInterval();

    averageSendTimeAccum += diff;
    if (diff > maximumSendTimeAccum)
      maximumSendTimeAccum = diff;
    if (diff < minimumSendTimeAccum)
      minimumSendTimeAccum = diff;
    senderReportCount++;
  }

  lastSentPacketTime = tick;

  octetsSent += frame.GetPayloadSize();
  packetsSent++;

  if (senderReportTimer.IsRunning() || senderReportCount == 0)
    return e_ProcessPacket;

  averageSendTime = averageSendTimeAccum/senderReportCount;
  maximumSendTime = maximumSendTimeAccum;
  minimumSendTime = minimumSendTimeAccum;

  averageSendTimeAccum = 0;
  maximumSendTimeAccum = 0;
  minimumSendTimeAccum = 0xffffffff;

  PTRACE(2, "RTP\tTransmit statistics: "
            " packets=" << packetsSent <<
            " octets=" << octetsSent <<
            " avgTime=" << averageSendTime <<
            " maxTime=" << maximumSendTime <<
            " minTime=" << minimumSendTime
            );

  senderReportTimer.Reset();
  senderReportCount = 0;

  if (userData != NULL)
    userData->OnTxStatistics(*this);

  RTP_ControlFrame report;
  report.SetPayloadType(RTP_ControlFrame::e_SenderReport);
  report.SetPayloadSize(sizeof(RTP_ControlFrame::SenderReport));

  RTP_ControlFrame::SenderReport * sender =
                            (RTP_ControlFrame::SenderReport *)report.GetPayloadPtr();
  sender->ssrc = syncSourceOut;
  PTime now;
  sender->ntp_sec = now.GetTimeInSeconds()+SecondsFrom1900to1970; // Convert from 1970 to 1900
  sender->ntp_frac = now.GetMicrosecond()*4294; // Scale microseconds to "fraction" from 0 to 2^32
  sender->rtp_ts = frame.GetTimestamp();
  sender->psent = packetsSent;
  sender->osent = octetsSent;

  PTRACE(3, "RTP\tSentSenderReport: "
              " ssrc=" << sender->ssrc
           << " ntp=" << sender->ntp_sec << '.' << sender->ntp_frac
           << " rtp=" << sender->rtp_ts
           << " psent=" << sender->psent
           << " osent=" << sender->osent);

  if (syncSourceIn != 0) {
    report.SetPayloadSize(sizeof(RTP_ControlFrame::SenderReport) +
                          sizeof(RTP_ControlFrame::ReceiverReport));
    report.SetCount(1);
    SetReceiverReport(*(RTP_ControlFrame::ReceiverReport *)&sender[1]);
  }

  return WriteControl(report) ? e_ProcessPacket : e_AbortTransport;
}


RTP_Session::SendReceiveStatus RTP_Session::OnReceiveData(const RTP_DataFrame & frame)
{
  // Check that the PDU is the right version
  if (frame.GetVersion() != RTP_DataFrame::ProtocolVersion)
    return e_IgnorePacket; // Non fatal error, just ignore

  // Check for if a control packet rather than data packet.
  if (frame.GetPayloadType() > RTP_DataFrame::MaxPayloadType)
    return e_IgnorePacket; // Non fatal error, just ignore

  PTimeInterval tick = PTimer::Tick();  // Get timestamp now

  // Check packet sequence numbers
  if (packetsReceived == 0) {
    expectedSequenceNumber = (WORD)(frame.GetSequenceNumber() + 1);
    syncSourceIn = frame.GetSyncSource();
    PTRACE(2, "RTP\tFirst data:"
              " ver=" << frame.GetVersion()
           << " pt=" << frame.GetPayloadType()
           << " psz=" << frame.GetPayloadSize()
           << " m=" << frame.GetMarker()
           << " x=" << frame.GetExtension()
           << " seq=" << frame.GetSequenceNumber()
           << " ts=" << frame.GetTimestamp()
           << " src=" << frame.GetSyncSource()
           << " ccnt=" << frame.GetContribSrcCount());
  }
  else {
    if (ignoreOtherSources && frame.GetSyncSource() != syncSourceIn) {
      PTRACE(2, "RTP\tPacket from SSRC=" << frame.GetSyncSource()
             << " ignored, expecting SSRC=" << syncSourceIn);
      return e_IgnorePacket; // Non fatal error, just ignore
    }

    WORD sequenceNumber = frame.GetSequenceNumber();
    if (sequenceNumber == expectedSequenceNumber) {
      expectedSequenceNumber++;
      consecutiveOutOfOrderPackets = 0;
      // Only do statistics on packets after first received in talk burst
      if (!frame.GetMarker()) {
        DWORD diff = (tick - lastReceivedPacketTime).GetInterval();

        averageReceiveTimeAccum += diff;
        if (diff > maximumReceiveTimeAccum)
          maximumReceiveTimeAccum = diff;
        if (diff < minimumReceiveTimeAccum)
          minimumReceiveTimeAccum = diff;
        receiverReportCount++;

        // The following has the implicit assumption that something that has jitter
        // is an audio codec and thus is in 8kHz timestamp units.
        diff *= 8;
        long variance = diff - lastTransitTime;
        lastTransitTime = diff;
        if (variance < 0)
          variance = -variance;
        jitterLevel += variance - ((jitterLevel+8) >> 4);
      }
    }
    else if (sequenceNumber < expectedSequenceNumber) {
      PTRACE(3, "RTP\tOut of order packet, received "
             << sequenceNumber << " expected " << expectedSequenceNumber
             << " ssrc=" << syncSourceIn);
      packetsOutOfOrder++;

      // Check for Cisco bug where sequence numbers suddenly start incrementing
      // from a different base.
      if (++consecutiveOutOfOrderPackets > 10) {
        expectedSequenceNumber = (WORD)(sequenceNumber + 1);
        PTRACE(1, "RTP\tAbnormal change of sequence numbers, adjusting to expect "
               << expectedSequenceNumber << " ssrc=" << syncSourceIn);
      }

      if (ignoreOutOfOrderPackets)
        return e_IgnorePacket; // Non fatal error, just ignore
    }
    else {
      unsigned dropped = sequenceNumber - expectedSequenceNumber;
      packetsLost += dropped;
      PTRACE(3, "RTP\tDropped " << dropped << " packet(s) at " << sequenceNumber
             << ", ssrc=" << syncSourceIn);
      expectedSequenceNumber = (WORD)(sequenceNumber + 1);
      consecutiveOutOfOrderPackets = 0;
    }
  }

  lastReceivedPacketTime = tick;

  octetsReceived += frame.GetPayloadSize();
  packetsReceived++;

  if (receiverReportTimer.IsRunning() || receiverReportCount == 0)
    return e_ProcessPacket;

  averageReceiveTime = averageReceiveTimeAccum/receiverReportCount;
  maximumReceiveTime = maximumReceiveTimeAccum;
  minimumReceiveTime = minimumReceiveTimeAccum;

  averageReceiveTimeAccum = 0;
  maximumReceiveTimeAccum = 0;
  minimumReceiveTimeAccum = 0xffffffff;
  
  PTRACE(2, "RTP\tReceive statistics: "
            " packets=" << packetsReceived <<
            " octets=" << octetsReceived <<
            " lost=" << packetsLost <<
            " tooLate=" << GetPacketsTooLate() <<
            " order=" << packetsOutOfOrder <<
            " avgTime=" << averageReceiveTime <<
            " maxTime=" << maximumReceiveTime <<
            " minTime=" << minimumReceiveTime <<
            " jitter=" << (jitterLevel >> 7)
            );

  receiverReportTimer.Reset();
  receiverReportCount = 0;

  if (userData != NULL)
    userData->OnRxStatistics(*this);

  // If sending data, RR is sent in an SR
  if (packetsSent != 0)
    return e_ProcessPacket;

  // Send RR as we are not transmitting
  RTP_ControlFrame report;
  report.SetPayloadType(RTP_ControlFrame::e_ReceiverReport);
  report.SetPayloadSize(4+sizeof(RTP_ControlFrame::ReceiverReport));
  report.SetCount(1);

  PUInt32b * payload = (PUInt32b *)report.GetPayloadPtr();
  *payload = syncSourceOut;
  SetReceiverReport(*(RTP_ControlFrame::ReceiverReport *)&payload[1]);

  return WriteControl(report) ? e_ProcessPacket : e_AbortTransport;
}


static RTP_Session::ReceiverReportArray
                BuildReceiverReportArray(const RTP_ControlFrame & frame, PINDEX offset)
{
  RTP_Session::ReceiverReportArray reports;

  const RTP_ControlFrame::ReceiverReport * rr = (const RTP_ControlFrame::ReceiverReport *)(frame.GetPayloadPtr()+offset);
  for (PINDEX repIdx = 0; repIdx < (PINDEX)frame.GetCount(); repIdx++) {
    RTP_Session::ReceiverReport * report = new RTP_Session::ReceiverReport;
    report->sourceIdentifier = rr->ssrc;
    report->fractionLost = rr->fraction;
    report->totalLost = rr->GetLostPackets();
    report->lastSequenceNumber = rr->last_seq;
    report->jitter = rr->jitter;
    report->lastTimestamp = (PInt64)(DWORD)rr->lsr;
    report->delay = ((PInt64)rr->dlsr << 16)/1000;
    reports.SetAt(repIdx, report);
    rr++;
  }

  return reports;
}


RTP_Session::SendReceiveStatus RTP_Session::OnReceiveControl(const RTP_ControlFrame & frame)
{
  BYTE * payload = frame.GetPayloadPtr();
  unsigned size = frame.GetPayloadSize();

  switch (frame.GetPayloadType()) {
    case RTP_ControlFrame::e_SenderReport :
      if (size >= sizeof(RTP_ControlFrame::SenderReport)) {
        SenderReport sender;
        const RTP_ControlFrame::SenderReport & sr = *(const RTP_ControlFrame::SenderReport *)payload;
        sender.sourceIdentifier = sr.ssrc;
        sender.realTimestamp = PTime(sr.ntp_sec-SecondsFrom1900to1970, sr.ntp_frac/4294);
        sender.rtpTimestamp = sr.rtp_ts;
        sender.packetsSent = sr.psent;
        sender.octetsSent = sr.osent;
        OnRxSenderReport(sender,
              BuildReceiverReportArray(frame, sizeof(RTP_ControlFrame::SenderReport)));
      }
      else {
        PTRACE(2, "RTP\tSenderReport packet truncated");
      }
      break;

    case RTP_ControlFrame::e_ReceiverReport :
      if (size >= 4)
        OnRxReceiverReport(*(const PUInt32b *)payload,
                                    BuildReceiverReportArray(frame, sizeof(PUInt32b)));
      else {
        PTRACE(2, "RTP\tReceiverReport packet truncated");
      }
      break;

    case RTP_ControlFrame::e_SourceDescription :
      if (size >= frame.GetCount()*sizeof(RTP_ControlFrame::SourceDescription)) {
        SourceDescriptionArray descriptions;
        const RTP_ControlFrame::SourceDescription * sdes = (const RTP_ControlFrame::SourceDescription *)payload;
        for (PINDEX srcIdx = 0; srcIdx < (PINDEX)frame.GetCount(); srcIdx++) {
          descriptions.SetAt(srcIdx, new SourceDescription(sdes->src));
          const RTP_ControlFrame::SourceDescription::Item * item = sdes->item;
          while (item->type != RTP_ControlFrame::e_END) {
            descriptions[srcIdx].items.SetAt(item->type, PString(item->data, item->length));
            item = item->GetNextItem();
          }
          sdes = (const RTP_ControlFrame::SourceDescription *)item->GetNextItem();
        }
        OnRxSourceDescription(descriptions);
      }
      else {
        PTRACE(2, "RTP\tSourceDescription packet truncated");
      }
      break;

    case RTP_ControlFrame::e_Goodbye :
      if (size >= 4) {
        PString str;
        unsigned count = frame.GetCount()*4;
        if (size > count)
          str = PString((const char *)(payload+count+1), payload[count]);
        PDWORDArray sources(count);
        for (PINDEX i = 0; i < (PINDEX)count; i++)
          sources[i] = ((const PUInt32b *)payload)[i];
        OnRxGoodbye(sources, str);
      }
      else {
        PTRACE(2, "RTP\tGoodbye packet truncated");
      }
      break;

    case RTP_ControlFrame::e_ApplDefined :
      if (size >= 4) {
        PString str((const char *)(payload+4), 4);
        OnRxApplDefined(str, frame.GetCount(), *(const PUInt32b *)payload,
                        payload+8, frame.GetPayloadSize()-8);
      }
      else {
        PTRACE(2, "RTP\tApplDefined packet truncated");
      }
      break;

    default :
      PTRACE(2, "RTP\tUnknown control payload type: " << frame.GetPayloadType());
  }

  return e_ProcessPacket;
}


void RTP_Session::OnRxSenderReport(const SenderReport & PTRACE_sender,
                                   const ReceiverReportArray & PTRACE_reports)
{
#if PTRACING
  PTRACE(3, "RTP\tOnRxSenderReport: " << PTRACE_sender);
  for (PINDEX i = 0; i < PTRACE_reports.GetSize(); i++)
    PTRACE(3, "RTP\tOnRxSenderReport RR: " << PTRACE_reports[i]);
#endif
}


void RTP_Session::OnRxReceiverReport(DWORD PTRACE_src,
                                     const ReceiverReportArray & PTRACE_reports)
{
#if PTRACING
  PTRACE(3, "RTP\tOnReceiverReport: ssrc=" << PTRACE_src);
  for (PINDEX i = 0; i < PTRACE_reports.GetSize(); i++)
    PTRACE(3, "RTP\tOnReceiverReport RR: " << PTRACE_reports[i]);
#endif
}


void RTP_Session::OnRxSourceDescription(const SourceDescriptionArray & PTRACE_description)
{
#if PTRACING
  for (PINDEX i = 0; i < PTRACE_description.GetSize(); i++)
    PTRACE(3, "RTP\tOnSourceDescription: " << PTRACE_description[i]);
#endif
}


void RTP_Session::OnRxGoodbye(const PDWORDArray & PTRACE_src,
                              const PString & PTRACE_reason)
{
  PTRACE(3, "RTP\tOnGoodbye: \"" << PTRACE_reason << "\" srcs=" << PTRACE_src);
}


void RTP_Session::OnRxApplDefined(const PString & PTRACE_type,
                                  unsigned PTRACE_subtype, DWORD PTRACE_src,
                                  const BYTE * /*data*/, PINDEX PTRACE_size)
{
  PTRACE(3, "RTP\tOnApplDefined: \"" << PTRACE_type << "\"-" << PTRACE_subtype
                          << " " << PTRACE_src << " [" << PTRACE_size << ']');
}


void RTP_Session::ReceiverReport::PrintOn(ostream & strm) const
{
  strm << "ssrc=" << sourceIdentifier
       << " fraction=" << fractionLost
       << " lost=" << totalLost
       << " last_seq=" << lastSequenceNumber
       << " jitter=" << jitter
       << " lsr=" << lastTimestamp
       << " dlsr=" << delay;
}


void RTP_Session::SenderReport::PrintOn(ostream & strm) const
{
  strm << "ssrc=" << sourceIdentifier
       << " ntp=" << realTimestamp.AsString("yyyy/M/d-h:m:s.uuuu")
       << " rtp=" << rtpTimestamp
       << " psent=" << packetsSent
       << " osent=" << octetsSent;
}


void RTP_Session::SourceDescription::PrintOn(ostream & strm) const
{
  static const char * const DescriptionNames[RTP_ControlFrame::NumDescriptionTypes] = {
    "END", "CNAME", "NAME", "EMAIL", "PHONE", "LOC", "TOOL", "NOTE", "PRIV"
  };

  strm << "ssrc=" << sourceIdentifier;
  for (PINDEX i = 0; i < items.GetSize(); i++) {
    strm << "\n  item[" << i << "]: type=";
    unsigned typeNum = items.GetKeyAt(i);
    if (typeNum < PARRAYSIZE(DescriptionNames))
      strm << DescriptionNames[typeNum];
    else
      strm << typeNum;
    strm << " data=\""
         << items.GetDataAt(i)
         << '"';
  }
}


DWORD RTP_Session::GetPacketsTooLate() const
{
  return jitter != NULL ? jitter->GetPacketsTooLate() : 0;
}


/////////////////////////////////////////////////////////////////////////////

RTP_SessionManager::RTP_SessionManager()
{
  enumerationIndex = P_MAX_INDEX;
}


RTP_Session * RTP_SessionManager::UseSession(unsigned sessionID)
{
  mutex.Wait();

  RTP_Session * session = sessions.GetAt(sessionID);
  if (session == NULL)
    return NULL;

  PTRACE(3, "RTP\tFound existing session " << sessionID);
  session->IncrementReference();

  mutex.Signal();
  return session;
}


void RTP_SessionManager::AddSession(RTP_Session * session)
{
  PAssertNULL(session);

  PTRACE(2, "RTP\tAdding session " << *session);
  sessions.SetAt(session->GetSessionID(), session);
  mutex.Signal();
}


void RTP_SessionManager::ReleaseSession(unsigned sessionID)
{
  PTRACE(2, "RTP\tReleasing session " << sessionID);

  mutex.Wait();

  if (sessions.Contains(sessionID)) {
    if (sessions[sessionID].DecrementReference()) {
      PTRACE(3, "RTP\tDeleting session " << sessionID);
      sessions[sessionID].SetJitterBufferSize(0);
      sessions.SetAt(sessionID, NULL);
    }
  }

  mutex.Signal();
}


RTP_Session * RTP_SessionManager::GetSession(unsigned sessionID) const
{
  PWaitAndSignal wait(mutex);
  if (!sessions.Contains(sessionID))
    return NULL;

  PTRACE(3, "RTP\tFound existing session " << sessionID);
  return &sessions[sessionID];
}


RTP_Session * RTP_SessionManager::First()
{
  mutex.Wait();

  enumerationIndex = 0;
  return Next();
}


RTP_Session * RTP_SessionManager::Next()
{
  if (enumerationIndex < sessions.GetSize())
    return &sessions.GetDataAt(enumerationIndex++);

  Exit();
  return NULL;
}


void RTP_SessionManager::Exit()
{
  enumerationIndex = P_MAX_INDEX;
  mutex.Signal();
}


/////////////////////////////////////////////////////////////////////////////

static void SetMinBufferSize(PUDPSocket & sock, int buftype)
{
  int sz = 0;
  if (sock.GetOption(buftype, sz)) {
    if (sz >= UDP_BUFFER_SIZE)
      return;
  }

  if (!sock.SetOption(buftype, UDP_BUFFER_SIZE)) {
    PTRACE(1, "RTP_UDP\tSetOption(" << buftype << ") failed: " << sock.GetErrorText());
  }
}


RTP_UDP::RTP_UDP(unsigned id)
  : RTP_Session(id),
    remoteAddress(0),
    remoteTransmitAddress(0)
{
  remoteDataPort = 0;
  remoteControlPort = 0;
  shutdownRead = FALSE;
  shutdownWrite = FALSE;
}


RTP_UDP::~RTP_UDP()
{
  Close(TRUE);
  Close(FALSE);
}


BOOL RTP_UDP::Open(PIPSocket::Address _localAddress, WORD portBase, WORD portMax, BYTE tos)
{
  // save local address 
  localAddress = _localAddress;

  localDataPort    = (WORD)(portBase&0xfffe);
  localControlPort = (WORD)(localDataPort + 1);

  while (!dataSocket.Listen(localAddress,    1, localDataPort) ||
         !controlSocket.Listen(localAddress, 1, localControlPort)) {
    dataSocket.Close();
    controlSocket.Close();
    if ((localDataPort > portMax) || (localDataPort > 0xfffd))
      return FALSE; // If it ever gets to here the OS has some SERIOUS problems!
    localDataPort    += 2;
    localControlPort += 2;
  }

#ifndef __BEOS__

  // Set the IP Type Of Service field for prioritisation of media UDP packets
  // through some Cisco routers and Linux boxes
  if (!dataSocket.SetOption(IP_TOS, tos, IPPROTO_IP)) {
    PTRACE(1, "RTP_UDP\tCould not set TOS field in IP header: " << dataSocket.GetErrorText());
  }

  // Increase internal buffer size on media UDP sockets
  SetMinBufferSize(dataSocket,    SO_RCVBUF);
  SetMinBufferSize(dataSocket,    SO_SNDBUF);
  SetMinBufferSize(controlSocket, SO_RCVBUF);
  SetMinBufferSize(controlSocket, SO_SNDBUF);
#endif

  shutdownRead = FALSE;
  shutdownWrite = FALSE;

  PTRACE(2, "RTP_UDP\tSession " << sessionID
         << " created: data=" << dataSocket.GetPort()
         << " control=" << controlSocket.GetPort()
         << " ssrc=" << syncSourceOut);
  return TRUE;
}


void RTP_UDP::Close(BOOL reading)
{
  if (reading) {
    if (!shutdownRead) {
      PTRACE(3, "RTP_UDP\tSession " << sessionID << ", Shutting down read.");
      shutdownRead = TRUE;
      PIPSocket::Address addr;
      controlSocket.GetLocalAddress(addr);
      if (addr == INADDR_ANY)
        PIPSocket::GetHostAddress(addr);
      dataSocket.WriteTo("", 1, addr, controlSocket.GetPort());
    }
  }
  else {
    PTRACE(3, "RTP_UDP\tSession " << sessionID << ", Shutting down write.");
    shutdownWrite = TRUE;
  }
}


PString RTP_UDP::GetLocalHostName()
{
  return PIPSocket::GetHostName();
}


BOOL RTP_UDP::SetRemoteSocketInfo(PIPSocket::Address address, WORD port, BOOL isDataPort)
{
  PTRACE(3, "RTP_UDP\tSetRemoteSocketInfo: "
         << (isDataPort ? "data" : "control") << " channel, "
            "new=" << address << ':' << port << ", "
            "local=" << localAddress << ':' << localDataPort << '-' << localControlPort << ", "
            "remote=" << remoteAddress << ':' << remoteDataPort << '-' << remoteControlPort);

  if (localAddress == address && (isDataPort ? localDataPort : localControlPort) == port)
    return TRUE;

  remoteAddress = address;

  if (isDataPort) {
    remoteDataPort = port;
    if (remoteControlPort == 0)
      remoteControlPort = (WORD)(port + 1);
  }
  else {
    remoteControlPort = port;
    if (remoteDataPort == 0)
      remoteDataPort = (WORD)(port - 1);
  }

  return remoteAddress != 0 && port != 0;
}


BOOL RTP_UDP::ReadData(RTP_DataFrame & frame)
{
  while (!shutdownRead) {
    int selectStatus = PSocket::Select(dataSocket, controlSocket);
    if (shutdownRead)
      break;

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

      case PSocket::Interrupted:
        PTRACE(3, "RTP_UDP\tSession " << sessionID << ", Interrupted.");
        return FALSE;

      default :
        PTRACE(1, "RTP_UDP\tSession " << sessionID << ", Select error: "
                << PChannel::GetErrorText((PChannel::Errors)selectStatus));
        return FALSE;
    }
  }

  PTRACE(3, "RTP_UDP\tSession " << sessionID << ", Read shutdown.");
  shutdownRead = FALSE;
  return FALSE;
}


RTP_Session::SendReceiveStatus RTP_UDP::ReadDataOrControlPDU(PUDPSocket & socket,
                                                             PBYTEArray & frame,
                                                             const char * PTRACE_name)
{
  PIPSocket::Address addr;
  WORD port;

  if (socket.ReadFrom(frame.GetPointer(), frame.GetSize(), addr, port)) {
    if (ignoreOtherSources) {
      if (remoteTransmitAddress == 0)
        remoteTransmitAddress = addr;
      else if (remoteTransmitAddress != addr) {
        PTRACE(1, "RTP_UDP\t" << PTRACE_name << " PDU from incorrect host, "
                  " is " << addr << " should be " << remoteTransmitAddress);
        return RTP_Session::e_IgnorePacket;
      }
    }

    return RTP_Session::e_ProcessPacket;
  }

  switch (socket.GetErrorNumber()) {
    case ECONNRESET :
    case ECONNREFUSED :
      PTRACE(2, "RTP_UDP\t" << PTRACE_name << " port on remote not ready.");
      return RTP_Session::e_IgnorePacket;

    default:
      PTRACE(1, "RTP_UDP\t" << PTRACE_name << " read error ("
             << socket.GetErrorNumber(PChannel::LastReadError) << "): "
             << socket.GetErrorText(PChannel::LastReadError));
      return RTP_Session::e_AbortTransport;
  }
}


RTP_Session::SendReceiveStatus RTP_UDP::ReadDataPDU(RTP_DataFrame & frame)
{
  SendReceiveStatus status = ReadDataOrControlPDU(dataSocket, frame, "Data");
  if (status != e_ProcessPacket)
    return status;

  // Check received PDU is big enough
  PINDEX pduSize = dataSocket.GetLastReadCount();
  if (pduSize < RTP_DataFrame::MinHeaderSize || pduSize < frame.GetHeaderSize()) {
    PTRACE(2, "RTP_UDP\tSession " << sessionID
           << ", Received data packet too small: " << pduSize << " bytes");
    return e_IgnorePacket;
  }

  frame.SetPayloadSize(pduSize - frame.GetHeaderSize());
  return OnReceiveData(frame);
}


RTP_Session::SendReceiveStatus RTP_UDP::ReadControlPDU()
{
  RTP_ControlFrame frame(2048);

  SendReceiveStatus status = ReadDataOrControlPDU(controlSocket, frame, "Control");
  if (status != e_ProcessPacket)
    return status;

  PINDEX pduSize = controlSocket.GetLastReadCount();
  if (pduSize < 4 || pduSize < 4+frame.GetPayloadSize()) {
    PTRACE(2, "RTP_UDP\tSession " << sessionID
           << ", Received control packet too small: " << pduSize << " bytes");
    return e_IgnorePacket;
  }

  return OnReceiveControl(frame);
}


BOOL RTP_UDP::WriteData(RTP_DataFrame & frame)
{
  if (shutdownWrite) {
    PTRACE(3, "RTP_UDP\tSession " << sessionID << ", Write shutdown.");
    shutdownWrite = FALSE;
    return FALSE;
  }

  switch (OnSendData(frame)) {
    case e_ProcessPacket :
      break;
    case e_IgnorePacket :
      return TRUE;
    case e_AbortTransport :
      return FALSE;
  }

  if (dataSocket.WriteTo(frame.GetPointer(),
                         frame.GetHeaderSize()+frame.GetPayloadSize(),
                         remoteAddress, remoteDataPort))
    return TRUE;

  PTRACE(1, "RTP_UDP\tSession " << sessionID
         << ", Write error on data port ("
         << dataSocket.GetErrorNumber(PChannel::LastWriteError) << "): "
         << dataSocket.GetErrorText(PChannel::LastWriteError));
  return FALSE;
}


BOOL RTP_UDP::WriteControl(RTP_ControlFrame & frame)
{
  if (controlSocket.WriteTo(frame.GetPointer(), frame.GetPayloadSize()+4,
                            remoteAddress, remoteControlPort))
    return TRUE;

  PTRACE(1, "RTP_UDP\tSession " << sessionID
         << ", Write error on control port ("
         << controlSocket.GetErrorNumber(PChannel::LastWriteError) << "): "
         << controlSocket.GetErrorText(PChannel::LastWriteError));
  return FALSE;
}


/////////////////////////////////////////////////////////////////////////////
