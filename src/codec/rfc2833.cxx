/*
 * rfc2833.cxx
 *
 * Open Phone Abstraction Library (OPAL)
 * Formally known as the Open H323 project.
 *
 * Copyright (c) 2001 Equivalence Pty. Ltd.
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
 * The Original Code is Open Phone Abstraction Library.
 *
 * The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * Contributor(s): ______________________________________.
 *
 */

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "rfc2833.h"
#endif

#include <codec/rfc2833.h>
#include <rtp/rtpconn.h>
#include <rtp/rtp.h>

static const char RFC2833Table1Events[] = "0123456789*#ABCD!                Y   X";
static const char NSEEvents[] = "XY";
static PINDEX NSECodeBase = 192;

#define PTraceModule() "RFC2833"
#define new PNEW


///////////////////////////////////////////////////////////////////////////////

const PCaselessString & OpalRFC288EventsName() { static PConstCaselessString s("Events"); return s; }

OpalRFC2833EventsMask::OpalRFC2833EventsMask(bool defaultValue)
  : std::vector<bool>(NumEvents, defaultValue)
{
}


OpalRFC2833EventsMask::OpalRFC2833EventsMask(const char * defaultValues)
  : std::vector<bool>(NumEvents)
{
  PStringStream strm(defaultValues);
  strm >> *this;
}


OpalRFC2833EventsMask & OpalRFC2833EventsMask::operator&=(const OpalRFC2833EventsMask & other)
{
  iterator lhs;
  const_iterator rhs;
  for (lhs = begin(), rhs = other.begin(); lhs != end() && rhs != other.end(); ++lhs,++rhs)
    *lhs = *lhs && *rhs;
  return *this;
}


ostream & operator<<(ostream & strm, const OpalRFC2833EventsMask & mask)
{
  PINDEX last = mask.size()-1;
  PINDEX i = 0;
  bool needComma = false;
  while (i < last) {
    if (!mask[i])
      i++;
    else {
      PINDEX start = i++;
      while (mask[i])
        i++;
      if (needComma)
        strm << ',';
      else
        needComma = true;
      strm << start;
      if (i > start+1)
        strm << '-' << (i-1);
    }
  }

  return  strm;
}


istream & operator>>(istream & strm, OpalRFC2833EventsMask & mask)
{
  mask.assign(OpalRFC2833EventsMask::NumEvents, false);

  for (;;) {
    unsigned eventCode;
    strm >> eventCode;
    if (eventCode >= mask.size())
      return strm;

    if (strm.fail())
      return strm;

    strm >> ws;
    switch (strm.peek()) {
      default :
        mask[eventCode] = true;
        return strm;

      case ',' :
        mask[eventCode] = true;
        break;

      case '-' :
        strm.ignore(1);

        unsigned endCode;
        strm >> endCode;
        if (strm.fail())
          return strm;

        while (eventCode <= endCode)
          mask[eventCode++] = true;

        strm >> ws;
        if (strm.peek() != ',')
          return strm;
    }

    strm.ignore(1);
  }
}


#if OPAL_SDP
static void AddEventsOption(OpalMediaFormatInternal & mediaFormat, const char * defaultValues, const char * fmtpDefaults)
#else
#define AddEventsOption(mf,dv,fd) AddEventsOption2(mf,dv)
static void AddEventsOption2(OpalMediaFormatInternal & mediaFormat, const char * defaultValues)
#endif
{
  OpalRFC288EventsOption * option = new OpalRFC288EventsOption(OpalRFC288EventsName(),
                                                               false,
                                                               OpalMediaOption::IntersectionMerge,
                                                               OpalRFC2833EventsMask(defaultValues));
#if OPAL_SDP
  option->SetFMTPName("FMTP");
  option->SetFMTPDefault(fmtpDefaults);
#endif
  mediaFormat.AddOption(option);
}


const OpalMediaFormat & GetOpalRFC2833()
{
  struct OpalRFC2833MediaFormatInternal : public OpalMediaFormatInternal {
    OpalRFC2833MediaFormatInternal()
      : OpalMediaFormatInternal(OPAL_RFC2833,
                                "userinput",
                                (RTP_DataFrame::PayloadTypes)101,  // Set to this for Cisco compatibility
                                "telephone-event",
                                true,   // Needs jitter
                                32*(1000/50), // bits/sec  (32 bits every 50ms)
                                4,      // bytes/frame
                                10*8,   // 10 millisecond
                                OpalMediaFormat::AudioClockRate)
    {
      SetOptionString(OpalMediaFormat::DescriptionOption(), "RFC 2833 - telephone event");
      AddEventsOption(*this, "0-16,32,36", "0-15");  // Support DTMF 0-9,*,#,A-D & hookflash, CNG, CED
    }
  };
  static OpalMediaFormatStatic<OpalMediaFormat> RFC2833(new OpalRFC2833MediaFormatInternal);
  return RFC2833;
}

const OpalMediaFormat & GetOpalCiscoNSE()
{
  struct OpalCiscoNSEMediaFormatInternal : public OpalMediaFormatInternal {
    OpalCiscoNSEMediaFormatInternal()
      : OpalMediaFormatInternal(OPAL_CISCONSE,
                                "userinput",
                                (RTP_DataFrame::PayloadTypes)100,  // Set to this for Cisco compatibility
                                "NSE",
                                true,   // Needs jitter
                                32*(1000/50), // bits/sec  (32 bits every 50ms)
                                4,      // bytes/frame
                                10*8,   // 10 millisecond
                                OpalMediaFormat::AudioClockRate)
    {
      SetOptionString(OpalMediaFormat::DescriptionOption(), "Cisco NSE - fax event");
      AddEventsOption(*this, "192,193", "192,193");
    }
  };
  static OpalMediaFormatStatic<OpalMediaFormat> CiscoNSE(new OpalCiscoNSEMediaFormatInternal);
  return CiscoNSE;
}

///////////////////////////////////////////////////////////////////////////////

OpalRFC2833Info::OpalRFC2833Info(char t, unsigned d, unsigned ts)
{
  tone = t;
  duration = d;
  timestamp = ts;
}


///////////////////////////////////////////////////////////////////////////////

OpalRFC2833Proto::OpalRFC2833Proto(const PNotifier & rx, const OpalMediaFormat & fmt)
  : m_baseMediaFormat(fmt)
  , m_txPayloadType(RTP_DataFrame::IllegalPayloadType)
  , m_rxPayloadType(RTP_DataFrame::IllegalPayloadType)
  , m_receiveNotifier(rx)
  , m_receiveHandler(PCREATE_RTPDataNotifier(ReceivedPacket))
  , m_rtpSession(NULL)
  , m_transmitState(TransmitIdle)
  , m_transmitTimestamp(0)
  , m_transmitStartTime(0)
  , m_transmitCode('\0')
  , m_transmitDuration(0)
  , m_receiveIdle(true)
  , m_receivedTone('\0')
  , m_receivedTimestamp(0)
  , m_receivedDuration(0)
{
  PTRACE(4, "Handler created");

  m_receiveTimer.SetNotifier(PCREATE_NOTIFIER(ReceiveTimeout), "RFC2833-Rx");
  m_transmitUpdateTimer.SetNotifier(PCREATE_NOTIFIER(TransmitTimeout), "RFC2833-TxUp");
  m_transmitDurationTimer.SetNotifier(PCREATE_NOTIFIER(TransmitTimeout), "RFC2833-TxDur");

  m_rxEvents.assign(16, true);
  m_rxEvents.resize(256);
  m_txEvents = m_rxEvents;
}


OpalRFC2833Proto::~OpalRFC2833Proto()
{
}


void OpalRFC2833Proto::UseRTPSession(bool rx, OpalRTPSession * rtpSession)
{
  if (rx) {
    if (rtpSession != NULL) {
      PTRACE(3, "Setting receive handler " << m_receiveHandler.GetObject()
             << " in RTP session " << rtpSession << " for " << m_baseMediaFormat);
      rtpSession->AddDataNotifier(50, m_receiveHandler);
    }
  }
  else {
    PTRACE(3, "Using RTP session " << rtpSession << " on " << m_baseMediaFormat);
    m_transmitMutex.Wait();
    m_rtpSession = rtpSession;
    m_transmitMutex.Signal();
  }
}


PBoolean OpalRFC2833Proto::SendToneAsync(char tone, unsigned milliseconds)
{
  PWaitAndSignal mutex(m_transmitMutex);

  if (m_txPayloadType == RTP_DataFrame::IllegalPayloadType) {
    PTRACE(2, "No payload type, cannot send packet for " << m_baseMediaFormat);
    return false;
  }

  if (milliseconds > 65535 / m_baseMediaFormat.GetTimeUnits()) {
    PTRACE(2, "Cannot play tone of " << milliseconds << "ms for " << m_baseMediaFormat);
    return false;
  }

  // convert tone to correct code, if space, use last one
  PINDEX code = tone == ' ' ? m_transmitCode : ASCIIToRFC2833(tone, m_txEvents[NSECodeBase]);
  if (code == P_MAX_INDEX || !m_txEvents[code])
    return false; // Silently ignore illegal tone values for this instance

  // if same tone as last time and still transmitting, just extend the time
  if (m_transmitState == TransmitIdle || code != m_transmitCode) {
    // Starting, so cannot have zero duration
    if (milliseconds == 0)
      milliseconds = 90;

    // kick off the transmitter
    m_transmitCode = (BYTE)code;
    m_transmitState = TransmitActive;
    m_transmitStartTime = 0;
  }

  if (milliseconds == 0)
    m_transmitState = TransmitEnding1;
  else {
    // reset the duration and retransmit timers
    m_transmitDurationTimer = milliseconds;
    m_transmitUpdateTimer.RunContinuous(20);
  }

  // send the current frame
  return InternalTransmitFrame();
}


WORD OpalRFC2833Proto::GetTimestampSince(const PTime & when) const
{
  return (WORD)(when.GetElapsed().GetMilliSeconds() * m_baseMediaFormat.GetTimeUnits());
}


bool OpalRFC2833Proto::AbortTransmit()
{
  m_transmitState = TransmitIdle;
  m_transmitDurationTimer.Stop(false);
  m_transmitUpdateTimer.Stop(false);
  return false;
}


bool OpalRFC2833Proto::InternalTransmitFrame()
{
  if (m_rtpSession == NULL) {
    PTRACE(2, "No RTP session suitable for " << m_baseMediaFormat);
    return AbortTransmit();
  }

  // if transmittter is ever in this state, then stop the duration timer
  if (m_txPayloadType == RTP_DataFrame::IllegalPayloadType) {
    PTRACE(2, "No payload type to send packet for " << m_baseMediaFormat);
    return AbortTransmit();
  }

  if (m_transmitState == TransmitIdle)
    return AbortTransmit();

  RTP_DataFrame frame(4);
  frame.SetPayloadType(m_txPayloadType);

  BYTE * payload = frame.GetPayloadPtr();
  payload[0] = m_transmitCode; // tone
  payload[1] = (7 & 0x3f);     // Volume
  if (m_transmitState != TransmitActive)
    payload[1] |= 0x80;

  // set end bit if sending last three packets
  switch (m_transmitState) {
    case TransmitActive:
      // set duration to time since start of time
      if (m_transmitStartTime.IsValid()) 
        m_transmitDuration = GetTimestampSince(m_transmitStartTime);
      else {
        frame.SetMarker(true);
        m_transmitDuration = 0;
        m_transmitStartTime.SetCurrentTime();
        m_transmitTimestamp = m_rtpSession->GetLastSentTimestamp() + GetTimestampSince(m_rtpSession->GetLastSentNetTime());
      }
      break;

    case TransmitEnding1:
      m_transmitDuration = GetTimestampSince(m_transmitStartTime);
      m_transmitState = TransmitEnding2;
      break;

    case TransmitEnding2:
      m_transmitState = TransmitEnding3;
      break;

    case TransmitEnding3:
      m_transmitState = TransmitIdle;
      m_transmitUpdateTimer.Stop(false);
      break;

    default:
      PAssertAlways("Unknown transmit state.");
      return AbortTransmit();
  }

  // set tone duration
  payload[2] = (BYTE)(m_transmitDuration >> 8);
  payload[3] = (BYTE) m_transmitDuration;

  frame.SetTimestamp(m_transmitTimestamp);

  if (!m_rtpSession->WriteData(frame)) {
    PTRACE(3, "RTP session transmission stopped for " << m_baseMediaFormat);
    return AbortTransmit();
  }

  PTRACE(frame.GetMarker() ? 3 : 4,
         "Sent " << ((payload[1] & 0x80) ? "end" : "tone") << ": "
         "code=" << (unsigned)m_transmitCode << ", "
         "dur=" << m_transmitDuration << ", "
         "ts=" << frame.GetTimestamp() << ", "
         "mkr=" << frame.GetMarker() << ", "
         "pt=" << m_txPayloadType <<
         " for " << m_baseMediaFormat);
  return true;
}


void OpalRFC2833Proto::TransmitTimeout(PTimer & timer, P_INT_PTR)
{
  if (!m_transmitMutex.Try()) {
    timer = 2; // Try again in a couple of milliseconds
    return;
  }

  if (m_transmitState == TransmitActive && &timer == &m_transmitDurationTimer) {
    m_transmitState = TransmitEnding1;
    timer = 5;
  }

  InternalTransmitFrame();
  m_transmitMutex.Signal();
}


OpalMediaFormat OpalRFC2833Proto::GetTxMediaFormat() const
{
  OpalMediaFormat format = m_baseMediaFormat;
  format.SetPayloadType(m_txPayloadType);
  OpalRFC288EventsOption * opt = format.FindOptionAs<OpalRFC288EventsOption>(OpalRFC288EventsName());
  if (PAssertNULL(opt) != NULL)
    opt->SetValue(m_txEvents);
  return format;
}


OpalMediaFormat OpalRFC2833Proto::GetRxMediaFormat() const
{
  OpalMediaFormat format = m_baseMediaFormat;
  format.SetPayloadType(m_rxPayloadType);
  OpalRFC288EventsOption * opt = format.FindOptionAs<OpalRFC288EventsOption>(OpalRFC288EventsName());
  if (PAssertNULL(opt) != NULL)
    opt->SetValue(m_rxEvents);
  return format;
}


static void SetXxMediaFormat(const OpalMediaFormat & mediaFormat,
                             RTP_DataFrame::PayloadTypes & payloadType,
                             OpalRFC2833EventsMask & events)
{
  if (mediaFormat.IsValid()) {
    payloadType = mediaFormat.GetPayloadType();
    OpalRFC288EventsOption * opt = mediaFormat.FindOptionAs<OpalRFC288EventsOption>(OpalRFC288EventsName());
    if (PAssertNULL(opt) != NULL)
      events = opt->GetValue();
  }
  else {
    payloadType = RTP_DataFrame::IllegalPayloadType;
    events = OpalRFC2833EventsMask();
  }
}


void OpalRFC2833Proto::SetTxMediaFormat(const OpalMediaFormat & mediaFormat)
{
  SetXxMediaFormat(mediaFormat, m_txPayloadType, m_txEvents);
  PTRACE(4, "Set tx pt=" << m_txPayloadType << ","
            " events=\"" << m_txEvents << "\""
            " for " << m_baseMediaFormat);
}


void OpalRFC2833Proto::SetRxMediaFormat(const OpalMediaFormat & mediaFormat)
{
  SetXxMediaFormat(mediaFormat, m_rxPayloadType, m_rxEvents);
  PTRACE(4, "Set rx pt=" << m_rxPayloadType << ","
            " events=\"" << m_rxEvents << "\""
            " for " << m_baseMediaFormat);
}


PINDEX OpalRFC2833Proto::ASCIIToRFC2833(char tone, bool hasNSE)
{
  const char * theChar;
  int upperTone = toupper(tone);

  if (hasNSE && (theChar = strchr(NSEEvents, upperTone)) != NULL)
    return (PINDEX)(NSECodeBase+theChar-NSEEvents);

  if ((theChar = strchr(RFC2833Table1Events, upperTone)) != NULL) 
    return (PINDEX)(theChar-RFC2833Table1Events);

  PTRACE2(1, NULL, "Invalid tone character '" << tone << "'.");
  return P_MAX_INDEX;
}


char OpalRFC2833Proto::RFC2833ToASCII(PINDEX rfc2833, bool hasNSE)
{
  PASSERTINDEX(rfc2833);

  if (hasNSE && rfc2833 >= NSECodeBase && rfc2833 < NSECodeBase+(PINDEX)sizeof(NSEEvents)-1)
    return NSEEvents[rfc2833-NSECodeBase];

  if (
#ifndef __GNUC__
      rfc2833 >= 0 &&
#endif
      rfc2833 < (PINDEX)sizeof(RFC2833Table1Events)-1)
    return RFC2833Table1Events[rfc2833];

  return '\0';
}


void OpalRFC2833Proto::OnStartReceive(char tone, unsigned timestamp)
{
  m_receiveIdle = false;
  m_receivedTone = tone;
  m_receivedTimestamp = timestamp;
  m_receiveTimer = 200;

  OpalRFC2833Info info(tone, 0, timestamp);
  m_receiveNotifier(info, 0);
}


void OpalRFC2833Proto::OnEndReceive()
{
  m_receiveIdle = true;
  m_receiveTimer.Stop(false);

  OpalRFC2833Info info(m_receivedTone, m_receivedDuration/m_baseMediaFormat.GetTimeUnits(), m_receivedTimestamp);
  m_receiveNotifier(info, 1);
}


void OpalRFC2833Proto::ReceivedPacket(OpalRTPSession &, OpalRTPSession::Data & data)
{
  // We have to have some payload, and we accept both the correctly negotiated rx payload type,
  // and the tx payload type in case the remote has misinterpreted the specification and thinks
  // the SDP is for what it sends and not what it receives. Idiots.
  if (data.m_frame.GetPayloadSize() == 0 ||
                (data.m_frame.GetPayloadType() != m_rxPayloadType && data.m_frame.GetPayloadType() != m_txPayloadType))
    return;

  data.m_status = OpalRTPSession::e_IgnorePacket;

  PWaitAndSignal mutex(m_receiveMutex);

  if (data.m_frame.GetPayloadSize() < 4) {
    PTRACE(2, "Ignoring packet size " << data.m_frame.GetPayloadSize() << " - too small for " << m_baseMediaFormat);
    return;
  }

  const BYTE * payload = data.m_frame.GetPayloadPtr();

  char tone = RFC2833ToASCII(payload[0], m_rxEvents[NSECodeBase]);
  if (tone == '\0') {
    PTRACE(2, "Ignoring packet with code " << payload[0] << " - unsupported event for " << m_baseMediaFormat);
    return;
  }

  // RFC 2833 says to ignore below -55db
  unsigned volume = (payload[1] & 0x3f);
  if (volume > 55) {
    PTRACE(2, "Ignoring packet " << (unsigned)payload[0] << " with volume -" << volume << "db for " << m_baseMediaFormat);
    return;
  }

  bool     endTone = (payload[1] & 0x80) != 0;
  m_receivedDuration = (WORD)((payload[2] << 8) + payload[3]);
  unsigned timestamp = data.m_frame.GetTimestamp();
  PTRACE(4, "Received " << (endTone ? "end" : "tone") << ": code='" << (unsigned)payload[0]
         << "', dur=" << m_receivedDuration << ", vol=" << volume << ", ts=" << timestamp << ", mkr=" << data.m_frame.GetMarker()
         << " for " << m_baseMediaFormat);

  if (m_receivedTimestamp == timestamp) {
    if (m_receiveIdle)
      return; // Ignore duplicate END tones
  }
  else {
    if (m_receiveIdle)
      OnStartReceive(tone, timestamp); // do callback for new tone
    else
      endTone = true; // As per RFC2833/RFC4733, if timestamp changes, it's the end of the tone too
  }

  if (endTone)
    OnEndReceive();
  else
    m_receiveTimer = 200; // Extend timeout
}


void OpalRFC2833Proto::ReceiveTimeout(PTimer & timer, P_INT_PTR)
{
  if (!m_receiveMutex.Try()) {
    timer = 2; // Try again in a couple of milliseconds
    return;
  }

  PTRACE(3, "Timeout occurred while receiving " << (unsigned)m_receivedTone
         << " for " << m_baseMediaFormat);

  if (!m_receiveIdle)
    OnEndReceive();

  m_receiveMutex.Signal();
}


/////////////////////////////////////////////////////////////////////////////
