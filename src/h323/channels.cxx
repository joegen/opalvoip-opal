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
 * $Log: channels.cxx,v $
 * Revision 1.2005  2001/08/17 08:32:17  robertj
 * Update from OpenH323
 *
 * Revision 2.3  2001/08/13 05:10:39  robertj
 * Updates from OpenH323 v1.6.0 release.
 *
 * Revision 2.2  2001/08/01 05:17:45  robertj
 * Made OpalMediaFormatList class global to help with documentation.
 *
 * Revision 2.1  2001/07/30 01:40:01  robertj
 * Fixed GNU C++ warnings.
 *
 * Revision 2.0  2001/07/27 15:48:25  robertj
 * Conversion of OpenH323 to Open Phone Abstraction Library (OPAL)
 *
 * Revision 1.96  2001/08/16 06:34:42  robertj
 * Plugged memory leak if using trace level 5.
 *
 * Revision 1.95  2001/08/10 01:34:41  robertj
 * Fixed problem with incorrect timestamp if codec returns more than one
 *    frame in read, thanks Lee Kirchhoff.
 *
 * Revision 1.94  2001/08/06 05:36:00  robertj
 * Fixed GNU warnings.
 *
 * Revision 1.93  2001/08/06 03:08:56  robertj
 * Fission of h323.h to h323ep.h & h323con.h, h323.h now just includes files.
 *
 * Revision 1.92  2001/07/24 02:26:44  robertj
 * Added start for handling reverse channels.
 *
 * Revision 1.91  2001/07/17 04:44:31  robertj
 * Partial implementation of T.120 and T.38 logical channels.
 *
 * Revision 1.90  2001/07/12 07:28:41  yurik
 * WinCE fix: Sleep(0) in Main to get system chance to digest
 *
 * Revision 1.89  2001/06/15 07:20:35  robertj
 * Moved OnClosedLogicalChannel() to be after channels threads halted.
 *
 * Revision 1.88  2001/06/02 01:35:32  robertj
 * Added thread names.
 *
 * Revision 1.87  2001/05/31 06:29:48  robertj
 * Changed trace of RTP mismatch so only displays for first n packets then
 *   does not dump messages any more. Was exactly the opposite.
 *
 * Revision 1.86  2001/04/20 02:32:07  robertj
 * Improved logging of bandwith, used more intuitive units.
 *
 * Revision 1.85  2001/04/02 04:12:53  robertj
 * Fixed trace output from packet transmit timing.
 *
 * Revision 1.84  2001/03/23 05:38:30  robertj
 * Added PTRACE_IF to output trace if a conditional is TRUE.
 *
 * Revision 1.83  2001/02/09 05:13:55  craigs
 * Added pragma implementation to (hopefully) reduce the executable image size
 * under Linux
 *
 * Revision 1.82  2001/02/07 05:04:45  robertj
 * Improved codec read analysis debug output.
 *
 * Revision 1.81  2001/02/06 07:40:46  robertj
 * Added debugging for timing of codec read.
 *
 * Revision 1.80  2001/01/25 07:27:16  robertj
 * Major changes to add more flexible OpalMediaFormat class to normalise
 *   all information about media types, especially codecs.
 *
 * Revision 1.79  2000/12/20 00:50:42  robertj
 * Fixed MSVC compatibility issues (No trace).
 *
 * Revision 1.78  2000/12/19 22:33:44  dereks
 * Adjust so that the video channel is used for reading/writing raw video
 * data, which better modularizes the video codec.
 *
 * Revision 1.77  2000/12/17 22:45:36  robertj
 * Set media stream threads to highest unprivileged priority.
 *
 * Revision 1.76  2000/11/24 10:52:50  robertj
 * Modified the ReadFrame/WriteFrame functions to allow for variable length codecs.
 * Added support for G.729 annex B packetisation scheme in RTP.
 * Fixed bug in fast started G.711 codec not working in one direction.
 *
 * Revision 1.75  2000/10/24 00:00:09  robertj
 * Improved memory hogging hash function for logical channels.
 *
 * Revision 1.74  2000/10/19 04:05:01  robertj
 * Added compare function for logical channel numbers, thanks Yuriy Ershov.
 *
 * Revision 1.73  2000/09/23 06:54:44  robertj
 * Prevented call of OnClose call back if channel was never opened.
 *
 * Revision 1.72  2000/09/22 01:35:49  robertj
 * Added support for handling LID's that only do symmetric codecs.
 *
 * Revision 1.71  2000/09/22 00:32:33  craigs
 * Added extra logging
 * Fixed problems with no fastConnect with tunelling
 *
 * Revision 1.70  2000/09/20 01:50:21  craigs
 * Added ability to set jitter buffer on a per-connection basis
 *
 * Revision 1.69  2000/09/14 23:03:45  robertj
 * Increased timeout on asserting because of driver lockup
 *
 * Revision 1.68  2000/08/31 08:15:40  robertj
 * Added support for dynamic RTP payload types in H.245 OpenLogicalChannel negotiations.
 *
 * Revision 1.67  2000/08/30 06:33:01  craigs
 * Add fix to ignore small runs of consectuive mismatched payload types
 *
 * Revision 1.66  2000/08/25 01:10:28  robertj
 * Added assert if various thrads ever fail to terminate.
 *
 * Revision 1.65  2000/08/21 02:50:28  robertj
 * Fixed race condition if close call just as slow start media channels are opening.
 *
 * Revision 1.64  2000/07/14 14:04:49  robertj
 * Clarified a debug message.
 *
 * Revision 1.63  2000/07/14 12:47:36  robertj
 * Added clarification to some logging messags.
 *
 * Revision 1.62  2000/07/13 16:05:47  robertj
 * Removed time critical priority as it can totally slag a Win98 system.
 * Fixed trace message displaying mismatched codecs in RTP packet around the wrong way.
 *
 * Revision 1.61  2000/07/11 11:15:52  robertj
 * Fixed bug when terminating RTP receiver and not also terminating transmitter.
 *
 * Revision 1.60  2000/06/23 02:04:01  robertj
 * Increased the priority of the media channels, only relevent for Win32 at this time.
 *
 * Revision 1.59  2000/06/15 01:46:15  robertj
 * Added channel pause (aka mute) functions.
 *
 * Revision 1.58  2000/05/18 12:10:50  robertj
 * Removed all Sleep() calls in codec as timing innacuracies make it unusable. All
 *    codec implementations must thus have timing built into them, usually using I/O.
 *
 * Revision 1.57  2000/05/11 23:54:25  craigs
 * Fixed the Windows fix with another Linux fix. But it worked OK on the Alpha!
 *
 * Revision 1.56  2000/05/11 09:56:46  robertj
 * Win32 compatibility and addition of some extra debugging on codec timing.
 *
 * Revision 1.55  2000/05/11 02:27:18  robertj
 * Added "fail safe" timer sleep on codec writes when on output of jitter buffer.
 *
 * Revision 1.54  2000/05/04 11:52:34  robertj
 * Added Packets Too Late statistics, requiring major rearrangement of jitter
 *    buffer code, not also changes semantics of codec Write() function slightly.
 *
 * Revision 1.53  2000/05/02 04:32:26  robertj
 * Fixed copyright notice comment.
 *
 * Revision 1.52  2000/05/01 01:01:49  robertj
 * Added flag for what to do with out of orer packets (use if jitter, don't if not).
 *
 * Revision 1.51  2000/04/28 13:01:44  robertj
 * Fixed problem with adjusting tx/rx frame counts in capabilities during fast start.
 *
 * Revision 1.50  2000/04/10 19:45:49  robertj
 * Changed RTP data receive tp be more forgiving, will process packet even if payload type is wrong.
 *
 * Revision 1.49  2000/03/31 20:04:28  robertj
 * Fixed log message for start/end of transmitted talk burst.
 *
 * Revision 1.48  2000/03/29 04:36:38  robertj
 * Improved some trace logging messages.
 *
 * Revision 1.47  2000/03/22 01:31:36  robertj
 * Fixed transmitter loop so codec can return multiple frames (crash in G.711 mode)
 *
 * Revision 1.46  2000/03/21 03:58:00  robertj
 * Fixed stuffed up RTP transmit loop after previous change.
 *
 * Revision 1.45  2000/03/21 03:06:49  robertj
 * Changes to make RTP TX of exact numbers of frames in some codecs.
 *
 * Revision 1.44  2000/03/20 20:59:28  robertj
 * Fixed possible buffer overrun problem in RTP_DataFrames
 *
 * Revision 1.43  2000/02/24 00:34:25  robertj
 * Fixed possible endless loop on channel abort, thanks Yura Ershov
 *
 * Revision 1.42  2000/02/04 05:11:19  craigs
 * Updated for new Makefiles and for new video transmission code
 *
 * Revision 1.41  2000/01/13 04:03:45  robertj
 * Added video transmission
 *
 * Revision 1.40  2000/01/08 06:52:10  robertj
 * Removed invalid assert
 *
 * Revision 1.39  1999/12/23 23:02:35  robertj
 * File reorganision for separating RTP from H.323 and creation of LID for VPB support.
 *
 * Revision 1.38  1999/11/22 01:37:31  robertj
 * Fixed channel closure so OnClosedLogicalChannel() only called if channel was actually started.
 *
 * Revision 1.37  1999/11/20 05:35:26  robertj
 * Extra debugging
 *
 * Revision 1.36  1999/11/20 00:53:47  robertj
 * Fixed ability to have variable sized frames in single RTP packet under G.723.1
 *
 * Revision 1.35  1999/11/19 09:06:25  robertj
 * Changed to close down logical channel if get a transmit codec error.
 *
 * Revision 1.34  1999/11/11 23:28:46  robertj
 * Added first cut silence detection algorithm.
 *
 * Revision 1.33  1999/11/06 11:01:37  robertj
 * Extra debugging.
 *
 * Revision 1.32  1999/11/06 05:37:44  robertj
 * Complete rewrite of termination of connection to avoid numerous race conditions.
 *
 * Revision 1.31  1999/11/01 00:47:46  robertj
 * Added close of logical channel on write error
 *
 * Revision 1.30  1999/10/30 12:38:24  robertj
 * Added more tracing of channel threads.
 *
 * Revision 1.29  1999/10/08 09:59:03  robertj
 * Rewrite of capability for sending multiple audio frames
 *
 * Revision 1.28  1999/10/08 04:58:37  robertj
 * Added capability for sending multiple audio frames in single RTP packet
 *
 * Revision 1.27  1999/09/23 07:25:12  robertj
 * Added open audio and video function to connection and started multi-frame codec send functionality.
 *
 * Revision 1.26  1999/09/21 14:09:02  robertj
 * Removed warnings when no tracing enabled.
 *
 * Revision 1.25  1999/09/18 13:24:38  craigs
 * Added ability to disable jitter buffer
 * Added ability to access entire RTP packet in codec Write
 *
 * Revision 1.24  1999/09/08 04:05:48  robertj
 * Added support for video capabilities & codec, still needs the actual codec itself!
 *
 * Revision 1.23  1999/09/03 02:17:50  robertj
 * Added more debugging
 *
 * Revision 1.22  1999/08/31 12:34:18  robertj
 * Added gatekeeper support.
 *
 * Revision 1.21  1999/07/16 15:02:51  robertj
 * Changed jitter buffer to throw away old packets if jitter exceeded.
 *
 * Revision 1.20  1999/07/16 00:51:03  robertj
 * Some more debugging of fast start.
 *
 * Revision 1.19  1999/07/15 14:45:35  robertj
 * Added propagation of codec open error to shut down logical channel.
 * Fixed control channel start up bug introduced with tunnelling.
 *
 * Revision 1.18  1999/07/15 09:04:31  robertj
 * Fixed some fast start bugs
 *
 * Revision 1.17  1999/07/14 06:04:04  robertj
 * Fixed setting of channel number in fast start.
 *
 * Revision 1.16  1999/07/13 09:53:24  robertj
 * Fixed some problems with jitter buffer and added more debugging.
 *
 * Revision 1.15  1999/07/13 02:50:58  craigs
 * Changed semantics of SetPlayDevice/SetRecordDevice, only descendent
 *    endpoint assumes PSoundChannel devices for audio codec.
 *
 * Revision 1.14  1999/07/10 03:01:48  robertj
 * Removed debugging.
 *
 * Revision 1.13  1999/07/09 06:09:49  robertj
 * Major implementation. An ENORMOUS amount of stuff added everywhere.
 *
 * Revision 1.12  1999/06/25 14:19:40  robertj
 * Fixed termination race condition in logical channel tear down.
 *
 * Revision 1.11  1999/06/24 13:32:45  robertj
 * Fixed ability to change sound device on codec and fixed NM3 G.711 compatibility
 *
 * Revision 1.10  1999/06/22 13:49:40  robertj
 * Added GSM support and further RTP protocol enhancements.
 *
 * Revision 1.9  1999/06/14 05:15:55  robertj
 * Changes for using RTP sessions correctly in H323 Logical Channel context
 *
 * Revision 1.8  1999/06/13 12:41:14  robertj
 * Implement logical channel transmitter.
 * Fixed H245 connect on receiving call.
 *
 * Revision 1.7  1999/06/09 06:18:00  robertj
 * GCC compatibiltiy.
 *
 * Revision 1.6  1999/06/09 05:26:19  robertj
 * Major restructuring of classes.
 *
 * Revision 1.5  1999/06/07 00:54:30  robertj
 * Displayed error on SetOption for buffer size
 *
 * Revision 1.4  1999/06/06 06:06:36  robertj
 * Changes for new ASN compiler and v2 protocol ASN files.
 *
 * Revision 1.3  1999/04/26 06:14:46  craigs
 * Initial implementation for RTP decoding and lots of stuff
 * As a whole, these changes are called "First Noise"
 *
 * Revision 1.2  1999/02/25 03:26:02  robertj
 * BeOS compatibility
 *
 * Revision 1.1  1999/01/16 01:31:09  robertj
 * Initial revision
 *
 */

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "channels.h"
#endif

#include <h323/channels.h>

#include <opal/transports.h>
#include <h323/h323ep.h>
#include <h323/h323con.h>
#include <h323/h323rtp.h>
#include <h323/transaddr.h>
#include <asn/h245.h>


#define new PNEW


#define	MAX_PAYLOAD_TYPE_MISMATCHES 8
#define RTP_TRACE_DISPLAY_RATE 16000 // 2 seconds


#if !PTRACING // Stuff to remove unused parameters warning
#define PTRACE_bitRateRestriction
#define PTRACE_type
#define PTRACE_jitter
#define PTRACE_skippedFrameCount
#define PTRACE_additionalBuffer
#define PTRACE_direction
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
  PAssert(IsDescendant(Class()), PInvalidCast);
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
    connection(conn),
    capability(cap)
{
  bandwidthUsed = 0;
  terminating = FALSE;
  opened = FALSE;
  paused = TRUE;
}


H323Channel::~H323Channel()
{
  connection.UseBandwidth(bandwidthUsed, FALSE);
}


void H323Channel::PrintOn(ostream & strm) const
{
  strm << number;
}


unsigned H323Channel::GetSessionID() const
{
  return 0;
}


void H323Channel::CleanUpOnTermination()
{
  if (!opened || terminating)
    return;

  terminating = TRUE;

  // Signal to the connection that this channel is on the way out
  connection.OnClosedLogicalChannel(*this);

  PTRACE(3, "LogChan\tCleaned up " << number);
}


BOOL H323Channel::IsRunning() const
{
  return opened && !paused;
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


void H323Channel::OnFlowControl(long PTRACE_bitRateRestriction)
{
  PTRACE(3, "LogChan\tOnFlowControl: " << PTRACE_bitRateRestriction);
}


void H323Channel::OnMiscellaneousCommand(const H245_MiscellaneousCommand_type & PTRACE_type)
{
  PTRACE(3, "LogChan\tOnMiscellaneousCommand: chan=" << number
         << ", type=" << PTRACE_type.GetTagName());
}


void H323Channel::OnMiscellaneousIndication(const H245_MiscellaneousIndication_type & PTRACE_type)
{
  PTRACE(3, "LogChan\tOnMiscellaneousIndication: chan=" << number
         << ", type=" << PTRACE_type.GetTagName());
}


void H323Channel::OnJitterIndication(DWORD PTRACE_jitter,
                                     int   PTRACE_skippedFrameCount,
                                     int   PTRACE_additionalBuffer)
{
  PTRACE(3, "LogChan\tOnJitterIndication:"
            " jitter=" << PTRACE_jitter <<
            " skippedFrameCount=" << PTRACE_skippedFrameCount <<
            " additionalBuffer=" << PTRACE_additionalBuffer);
}


BOOL H323Channel::SetBandwidthUsed(unsigned bandwidth)
{
  PTRACE(3, "LogChan\tBandwidth requested/used = "
         << bandwidth/10 << '.' << bandwidth%10 << '/'
         << bandwidthUsed/10 << '.' << bandwidthUsed%10
         << " kb/s");
  connection.UseBandwidth(bandwidthUsed, TRUE);
  bandwidthUsed = 0;

  if (!connection.UseBandwidth(bandwidth, FALSE))
    return FALSE;

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


H323Channel::Directions H323UnidirectionalChannel::GetDirection() const
{
  return receiver ? IsReceiver : IsTransmitter;
}


BOOL H323UnidirectionalChannel::SetInitialBandwidth()
{
  return SetBandwidthUsed(capability.GetMediaFormat().GetBandwidth()/100);
}


BOOL H323UnidirectionalChannel::Open()
{
  if (opened)
    return TRUE;

  if (PAssertNULL(mediaStream) == NULL)
    return FALSE;

  OpalMediaFormat mediaFormat = capability.GetMediaFormat();
  if (mediaFormat.IsEmpty()) {
    PTRACE(1, "LogChan\t" << (GetDirection() == IsReceiver ? "Receive" : "Transmit")
           << " open failed (Invalid OpalMediaFormat)");
    return FALSE;
  }

  if (!mediaStream->Open(mediaFormat)) {
    PTRACE(1, "LogChan\t" << (GetDirection() == IsReceiver ? "Receive" : "Transmit")
           << " open failed (OpalMediaStream::Open fail)");
    return FALSE;
  }

  return H323Channel::Open();
}


BOOL H323UnidirectionalChannel::Start()
{
  if (!Open())
    return FALSE;

  if (mediaStream->IsSource()) {
    if (!connection.OnOpenMediaStream(*mediaStream))
      return FALSE;
  }

  paused = FALSE;
  return TRUE;
}


void H323UnidirectionalChannel::CleanUpOnTermination()
{
  if (terminating)
    return;

  PTRACE(3, "H323RTP\tCleaning up media stream on " << number);

  // If we have source medai stream close it
  if (mediaStream != NULL) {
    mediaStream->Close();
    delete mediaStream;
  }

  H323Channel::CleanUpOnTermination();
}


OpalMediaStream * H323UnidirectionalChannel::GetMediaStream() const
{
  return mediaStream;
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

H323_RTPChannel::H323_RTPChannel(H323Connection & conn,
                                 const H323Capability & cap,
                                 Directions direction,
                                 RTP_Session & r)
  : H323UnidirectionalChannel(conn, cap, direction),
    rtpSession(r),
    rtpCallbacks(*(H323_RTP_Session *)r.GetUserData())
{
  rtpPayloadType = capability.GetMediaFormat().GetPayloadType();
  mediaStream = new OpalRTPMediaStream(receiver, rtpSession);
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


BOOL H323_RTPChannel::OnSendingPDU(H245_OpenLogicalChannel & open) const
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

    return rtpCallbacks.OnSendingPDU(*this, open.m_reverseLogicalChannelParameters.m_multiplexParameters);
  }
  else {
    // Set the communications information for unicast IPv4
    open.m_forwardLogicalChannelParameters.m_multiplexParameters.SetTag(
                H245_OpenLogicalChannel_forwardLogicalChannelParameters_multiplexParameters
                    ::e_h2250LogicalChannelParameters);

    return rtpCallbacks.OnSendingPDU(*this, open.m_forwardLogicalChannelParameters.m_multiplexParameters);
  }
}


void H323_RTPChannel::OnSendOpenAck(const H245_OpenLogicalChannel & open,
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

  rtpCallbacks.OnSendingAckPDU(*this, param);

  PTRACE(2, "H323RTP\tSending open logical channel ACK: sessionID=" << sessionID);
}


BOOL H323_RTPChannel::OnReceivedPDU(const H245_OpenLogicalChannel & open,
                                    unsigned & errorCode)
{
  if (receiver)
    number = H323ChannelNumber(open.m_forwardLogicalChannelNumber, TRUE);

  PTRACE(3, "H323RTP\tOnReceivedPDU for channel: " << number);

  BOOL reverse = open.HasOptionalField(H245_OpenLogicalChannel::e_reverseLogicalChannelParameters);
  const H245_DataType & dataType = reverse ? open.m_reverseLogicalChannelParameters.m_dataType
                                           : open.m_forwardLogicalChannelParameters.m_dataType;

  if (!((H323Capability&)capability).OnReceivedPDU(dataType, receiver)) {
    PTRACE(1, "H323RTP\tOnly H.225.0 multiplex supported");
    errorCode = H245_OpenLogicalChannelReject_cause::e_dataTypeNotSupported;
    return FALSE;
  }

  if (reverse) {
    if (open.m_reverseLogicalChannelParameters.m_multiplexParameters.GetTag() ==
             H245_OpenLogicalChannel_reverseLogicalChannelParameters_multiplexParameters::e_h2250LogicalChannelParameters)
      return rtpCallbacks.OnReceivedPDU(*this,
                                        open.m_reverseLogicalChannelParameters.m_multiplexParameters,
                                        errorCode);
  }
  else {
    if (open.m_forwardLogicalChannelParameters.m_multiplexParameters.GetTag() ==
             H245_OpenLogicalChannel_forwardLogicalChannelParameters_multiplexParameters::e_h2250LogicalChannelParameters)
      return rtpCallbacks.OnReceivedPDU(*this,
                                        open.m_forwardLogicalChannelParameters.m_multiplexParameters,
                                        errorCode);
  }

  PTRACE(1, "H323RTP\tOnly H.225.0 multiplex supported");
  errorCode = H245_OpenLogicalChannelReject_cause::e_unsuitableReverseParameters;
  return FALSE;
}


BOOL H323_RTPChannel::OnReceivedAckPDU(const H245_OpenLogicalChannelAck & ack)
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

  return rtpCallbacks.OnReceivedAckPDU(*this, ack.m_forwardMultiplexAckParameters);
}


BOOL H323_RTPChannel::SetDynamicRTPPayloadType(int newType)
{
  PTRACE(1, "H323RTP\tSetting dynamic RTP payload type: " << newType);

  // This is "no change"
  if (newType == -1)
    return TRUE;

  // Check for illegal type
  if (newType < RTP_DataFrame::DynamicBase || newType > RTP_DataFrame::MaxPayloadType)
    return FALSE;

  // Check for overwriting "known" type
  if (rtpPayloadType < RTP_DataFrame::DynamicBase)
    return FALSE;

  rtpPayloadType = (RTP_DataFrame::PayloadTypes)newType;
  PTRACE(3, "H323RTP\tSetting dynamic payload type to " << rtpPayloadType);
  return TRUE;
}


/////////////////////////////////////////////////////////////////////////////

H323DataChannel::H323DataChannel(H323Connection & conn,
                                 const H323Capability & cap,
                                 Directions dir)
  : H323UnidirectionalChannel(conn, cap, dir)
{
  listener = NULL;
  transport = NULL;
  separateReverseChannel = 0;
}


H323DataChannel::~H323DataChannel()
{
  delete transport;
  delete listener;
}


void H323DataChannel::CleanUpOnTermination()
{
  if (terminating)
    return;

  PTRACE(3, "LogChan\tCleaning up data channel " << number);

  // Break any I/O blocks and wait for the thread that uses this object to
  // terminate before we allow it to be deleted.
  if (listener != NULL)
    listener->Close();
  if (transport != NULL)
    transport->Close();

  H323UnidirectionalChannel::CleanUpOnTermination();
}


BOOL H323DataChannel::OnSendingPDU(H245_OpenLogicalChannel & open) const
{
  PTRACE(3, "LogChan\tOnSendingPDU");

  open.m_forwardLogicalChannelNumber = (unsigned)number;

  open.m_forwardLogicalChannelParameters.m_multiplexParameters.SetTag(
              H245_OpenLogicalChannel_forwardLogicalChannelParameters_multiplexParameters
                  ::e_h2250LogicalChannelParameters);
  H245_H2250LogicalChannelParameters & fparam = open.m_forwardLogicalChannelParameters.m_multiplexParameters;
  fparam.m_sessionID = capability.GetDefaultSessionID();

  if (separateReverseChannel)
    return TRUE;

  open.IncludeOptionalField(H245_OpenLogicalChannel::e_reverseLogicalChannelParameters);
  open.m_reverseLogicalChannelParameters.IncludeOptionalField(
              H245_OpenLogicalChannel_reverseLogicalChannelParameters::e_multiplexParameters);
  open.m_reverseLogicalChannelParameters.m_multiplexParameters.SetTag(
              H245_OpenLogicalChannel_reverseLogicalChannelParameters_multiplexParameters
                  ::e_h2250LogicalChannelParameters);
  H245_H2250LogicalChannelParameters & rparam = open.m_reverseLogicalChannelParameters.m_multiplexParameters;
  rparam.m_sessionID = capability.GetDefaultSessionID();

  return capability.OnSendingPDU(open.m_reverseLogicalChannelParameters.m_dataType);
}


void H323DataChannel::OnSendOpenAck(const H245_OpenLogicalChannel & /*open*/,
                                    H245_OpenLogicalChannelAck & ack) const
{
  if (listener == NULL && transport == NULL) {
    PTRACE(2, "LogChan\tOnSendOpenAck without a listener or transport");
    return;
  }

  PTRACE(3, "LogChan\tOnSendOpenAck");

  H245_H2250LogicalChannelParameters * param;

  if (separateReverseChannel) {
    ack.IncludeOptionalField(H245_OpenLogicalChannelAck::e_forwardMultiplexAckParameters);
    ack.m_forwardMultiplexAckParameters.SetTag(
              H245_OpenLogicalChannelAck_forwardMultiplexAckParameters::e_h2250LogicalChannelAckParameters);
    param = (H245_H2250LogicalChannelParameters*)&ack.m_forwardMultiplexAckParameters.GetObject();
  }
  else {
    ack.IncludeOptionalField(H245_OpenLogicalChannelAck::e_reverseLogicalChannelParameters);
    ack.m_reverseLogicalChannelParameters.m_multiplexParameters.SetTag(
              H245_OpenLogicalChannelAck_reverseLogicalChannelParameters_multiplexParameters
                  ::e_h2250LogicalChannelParameters);
    param = (H245_H2250LogicalChannelParameters*)
                &ack.m_reverseLogicalChannelParameters.m_multiplexParameters.GetObject();
  }

  H323TransportAddress address;
  if (listener != NULL)
    address = listener->GetLocalAddress(connection.GetControlChannel().GetLocalAddress());
  else
    address = transport->GetLocalAddress();

  param->IncludeOptionalField(H245_H2250LogicalChannelParameters::e_mediaChannel);
  address.SetPDU(param->m_mediaChannel);
}


BOOL H323DataChannel::OnReceivedPDU(const H245_OpenLogicalChannel & open,
                                    unsigned & errorCode)
{
  number = H323ChannelNumber(open.m_forwardLogicalChannelNumber, TRUE);

  PTRACE(3, "LogChan\tOnReceivedPDU for bidirectional channel: " << number);

  if (!CreateListener()) {
    PTRACE(1, "LogChan\tCould not create listener");
    errorCode = H245_OpenLogicalChannelReject_cause::e_unspecified;
    return FALSE;
  }

  if (separateReverseChannel &&
      open.HasOptionalField(H245_OpenLogicalChannel::e_reverseLogicalChannelParameters)) {
    errorCode = H245_OpenLogicalChannelReject_cause::e_unsuitableReverseParameters;
    PTRACE(2, "LogChan\tOnReceivedPDU has unexpected reverse parameters");
    return FALSE;
  }

  return TRUE;
}


BOOL H323DataChannel::OnReceivedAckPDU(const H245_OpenLogicalChannelAck & ack)
{
  PTRACE(3, "LogChan\tOnReceivedAckPDU");

  const H245_TransportAddress * address;

  if (separateReverseChannel) {
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

    if (ack.HasOptionalField(H245_OpenLogicalChannelAck::e_reverseLogicalChannelParameters))
      reverseChannel = H323ChannelNumber(ack.m_reverseLogicalChannelParameters.m_reverseLogicalChannelNumber, TRUE);
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

  if (!transport->SetRemoteAddress(H323TransportAddress(*address))) {
    PTRACE(1, "LogChan\tCould not set remote transport address");
    return FALSE;
  }

  if (!transport->Connect()) {
    PTRACE(1, "LogChan\tCould not connect to remote transport address");
    return FALSE;
  }

  return TRUE;
}


BOOL H323DataChannel::CreateListener()
{
  if (listener == NULL) {
    listener = connection.GetControlChannel().GetLocalAddress().CreateCompatibleListener(connection.GetEndPoint());
    if (listener == NULL)
      return FALSE;
  }

  return listener->Open(NULL);
}


BOOL H323DataChannel::CreateTransport()
{
  if (transport == NULL)
    transport = connection.GetControlChannel().GetLocalAddress().CreateTransport(connection.GetEndPoint());
  return transport != NULL;
}


/////////////////////////////////////////////////////////////////////////////
