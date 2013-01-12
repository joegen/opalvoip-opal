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
 * $Revision$
 * $Author$
 * $Date$
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


static void AddEventsOption(OpalMediaFormat & mediaFormat,
                            const char * defaultValues,
                            const char * fmtpDefaults)
{
  OpalRFC288EventsOption * option = new OpalRFC288EventsOption(OpalRFC288EventsName(),
                                                               false,
                                                               OpalMediaOption::IntersectionMerge,
                                                               OpalRFC2833EventsMask(defaultValues));
#if OPAL_SIP
  option->SetFMTPName("FMTP");
  option->SetFMTPDefault(fmtpDefaults);
#endif
  mediaFormat.AddOption(option);
}


const OpalMediaFormat & GetOpalRFC2833()
{
  static struct OpalRFC2833MediaFormat : public OpalMediaFormat {
    OpalRFC2833MediaFormat()
      : OpalMediaFormat(OPAL_RFC2833,
                        "userinput",
                        (RTP_DataFrame::PayloadTypes)101,  // Set to this for Cisco compatibility
                        "telephone-event",
                        true,   // Needs jitter
                        32*(1000/50), // bits/sec  (32 bits every 50ms)
                        4,      // bytes/frame
                        10*8,   // 10 millisecond
                        OpalMediaFormat::AudioClockRate)
    {
      AddEventsOption(*this, "0-16,32,36", "0-15");  // Support DTMF 0-9,*,#,A-D & hookflash, CNG, CED
    }
  } const RFC2833;
  return RFC2833;
}

const OpalMediaFormat & GetOpalCiscoNSE()
{
  static struct OpalCiscoNSEMediaFormat : public OpalMediaFormat {
    OpalCiscoNSEMediaFormat()
      : OpalMediaFormat(OPAL_CISCONSE,
                        "userinput",
                        (RTP_DataFrame::PayloadTypes)100,  // Set to this for Cisco compatibility
                        "NSE",
                        true,   // Needs jitter
                        32*(1000/50), // bits/sec  (32 bits every 50ms)
                        4,      // bytes/frame
                        10*8,   // 10 millisecond
                        OpalMediaFormat::AudioClockRate)
    {
      AddEventsOption(*this, "192,193", "192,193");
    }
  } const CiscoNSE;
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
  , m_receiveHandler(PCREATE_RTPFilterNotifier(ReceivedPacket))
  , m_receiveState(ReceiveIdle)
  , m_receivedTone('\0')
  , m_tonesReceived(0)
  , m_previousReceivedTimestamp(0)
  , m_transmitState(TransmitIdle)
  , m_rtpSession(NULL)
  , m_transmitTimestamp(0)
  , m_rewriteTransmitTimestamp(false)
  , m_transmitCode('\0')
  , m_transmitDuration(0)
{
  PTRACE(4, "RFC2833\tHandler created");

  m_receiveTimer.SetNotifier(PCREATE_NOTIFIER(ReceiveTimeout));
  m_asyncTransmitTimer.SetNotifier(PCREATE_NOTIFIER(AsyncTimeout));
  m_asyncDurationTimer.SetNotifier(PCREATE_NOTIFIER(AsyncTimeout));

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
      PTRACE(3, "RTPCon\tSetting receive handler for " << m_baseMediaFormat);
      rtpSession->AddFilter(m_receiveHandler);
    }
  }
  else {
    PTRACE(3, "RTPCon\tSetting RTP session 0x" << rtpSession << " on " << m_baseMediaFormat);
    m_sendMutex.Wait();
    m_rtpSession = rtpSession;
    m_sendMutex.Signal();
  }
}


PBoolean OpalRFC2833Proto::SendToneAsync(char tone, unsigned duration)
{
  PWaitAndSignal mutex(m_sendMutex);

  // convert tone to correct code
  PINDEX code = ASCIIToRFC2833(tone, m_txEvents[NSECodeBase]);
  if (code == P_MAX_INDEX || !m_txEvents[code])
    return false; // Silently ignore illegal tone values for this instance

  // if transmittter is ever in this state, then stop the duration timer
  if (m_txPayloadType == RTP_DataFrame::IllegalPayloadType) {
    PTRACE(2, "RFC2833\tNo payload type, cannot send packet for " << m_baseMediaFormat);
    return false;
  }

  // if same tone as last time and still transmitting, just extend the time
  if (m_transmitState == TransmitIdle || (tone != ' ' && code != m_transmitCode)) {
    // kick off the transmitter
    m_transmitCode             = (BYTE)code;
    m_transmitState            = TransmitActive;
    m_rewriteTransmitTimestamp = true;
    m_asyncStart               = 0;

    // Starting so cannot have zero duration
    if (duration == 0)
      duration = 90;
  }

  if (duration == 0)
    m_transmitState = TransmitEnding1;
  else {
    // reset the duration and retransmit timers
    m_asyncDurationTimer = duration;
    m_asyncTransmitTimer.RunContinuous(30);
  }

  // send the current frame
  SendAsyncFrame();

  return true;
}


void OpalRFC2833Proto::SendAsyncFrame()
{
  if (m_rtpSession == NULL) {
    PTRACE(2, "RFC2833\tNo RTP session suitable for " << m_baseMediaFormat);
    m_transmitState = TransmitIdle;
  }

  // if transmittter is ever in this state, then stop the duration timer
  if (m_txPayloadType == RTP_DataFrame::IllegalPayloadType) {
    PTRACE(2, "RFC2833\tNo payload type to send packet for " << m_baseMediaFormat);
    m_transmitState = TransmitIdle;
  }

  if (m_transmitState == TransmitIdle) {
    m_asyncDurationTimer.Stop(false);
    return;
  }

  RTP_DataFrame frame(4);
  frame.SetPayloadType(m_txPayloadType);

  BYTE * payload = frame.GetPayloadPtr();
  payload[0] = m_transmitCode; // tone
  payload[1] = (7 & 0x3f);     // Volume

  // set end bit if sending last three packets
  switch (m_transmitState) {
    case TransmitActive:
      // if the duration has ended, then go into ending state
      if (m_asyncDurationTimer.IsRunning()) {
        // set duration to time since start of time
        if (m_asyncStart != PTimeInterval(0)) 
          m_transmitDuration = (PTimer::Tick() - m_asyncStart).GetInterval() * 8;
        else {
          m_transmitDuration = 0;
          frame.SetMarker(true);
          m_asyncStart = PTimer::Tick();
        }
        break;
      }

      m_transmitState = TransmitEnding1;
      m_asyncTransmitTimer.RunContinuous(5); // Output the three end packets a bit quicker.
      // Do next case

    case TransmitEnding1:
      payload[1] |= 0x80;
      m_transmitDuration = (PTimer::Tick() - m_asyncStart).GetInterval() * 8;
      m_transmitState = TransmitEnding2;
      break;

    case TransmitEnding2:
      payload[1] |= 0x80;
      m_transmitState = TransmitEnding3;
      break;

    case TransmitEnding3:
      payload[1] |= 0x80;
      m_transmitState = TransmitIdle;
      m_asyncTransmitTimer.Stop(false);
      break;

    default:
      PAssertAlways("RFC2833\tUnknown transmit state.");
      m_transmitState = TransmitIdle;
      return;
  }

  // set tone duration
  payload[2] = (BYTE)(m_transmitDuration >> 8);
  payload[3] = (BYTE) m_transmitDuration;

  if (!m_rewriteTransmitTimestamp)
    frame.SetTimestamp(m_transmitTimestamp);

  if (!m_rtpSession->WriteOOBData(frame, m_rewriteTransmitTimestamp)) {
    PTRACE(3, "RFC2833\tRTP session transmission stopped for " << m_baseMediaFormat);
    // Abort further transmission
    m_transmitState = TransmitIdle;
    m_asyncDurationTimer.Stop(false);
  }

  if (m_rewriteTransmitTimestamp) {
    m_transmitTimestamp        = frame.GetTimestamp();
    m_rewriteTransmitTimestamp = false;
  } 

  PTRACE(frame.GetMarker() ? 3 : 4,
         "RFC2833\tSent " << ((payload[1] & 0x80) ? "end" : "tone") << ": "
         "code=" << (unsigned)m_transmitCode << ", "
         "dur=" << m_transmitDuration << ", "
         "ts=" << frame.GetTimestamp() << ", "
         "mkr=" << frame.GetMarker() << ", "
         "pt=" << m_txPayloadType <<
         " for " << m_baseMediaFormat);
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
  PTRACE(4, "RFC2833\tSet tx pt=" << m_txPayloadType << ","
            " events=\"" << m_txEvents << "\""
            " for " << m_baseMediaFormat);
}


void OpalRFC2833Proto::SetRxMediaFormat(const OpalMediaFormat & mediaFormat)
{
  SetXxMediaFormat(mediaFormat, m_rxPayloadType, m_rxEvents);
  PTRACE(4, "RFC2833\tSet rx pt=" << m_rxPayloadType << ","
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

  PTRACE2(1, NULL, "RFC2833\tInvalid tone character '" << tone << "'.");
  return P_MAX_INDEX;
}


char OpalRFC2833Proto::RFC2833ToASCII(PINDEX rfc2833, bool hasNSE)
{
  PASSERTINDEX(rfc2833);

  if (hasNSE && rfc2833 >= NSECodeBase && rfc2833 < NSECodeBase+(PINDEX)sizeof(NSEEvents)-1)
    return NSEEvents[rfc2833-NSECodeBase];

  if (rfc2833 >= 0 && rfc2833 < (PINDEX)sizeof(RFC2833Table1Events)-1)
    return RFC2833Table1Events[rfc2833];

  return '\0';
}


void OpalRFC2833Proto::AsyncTimeout(PTimer & timer, P_INT_PTR)
{
  if (m_sendMutex.Try()) {
    SendAsyncFrame();
    m_sendMutex.Signal();
  }
  else
    timer = 2; // Try again in a couple of milliseconds
}


void OpalRFC2833Proto::OnStartReceive(char tone, unsigned timestamp)
{
  ++m_tonesReceived;
  m_previousReceivedTimestamp = timestamp;
  OnStartReceive(tone);
  OpalRFC2833Info info(tone, 0, timestamp);
  m_receiveNotifier(info, 0);
}


void OpalRFC2833Proto::OnStartReceive(char)
{
}


void OpalRFC2833Proto::OnEndReceive(char tone, unsigned duration, unsigned timestamp)
{
  m_receiveState = ReceiveIdle;
  m_receiveTimer.Stop(false);
  OpalRFC2833Info info(tone, duration, timestamp);
  m_receiveNotifier(info, 1);
}


void OpalRFC2833Proto::ReceivedPacket(RTP_DataFrame & frame, OpalRTPSession::SendReceiveStatus & status)
{
  if (frame.GetPayloadType() != m_rxPayloadType || frame.GetPayloadSize() == 0)
    return;

  status = OpalRTPSession::e_IgnorePacket;

  PWaitAndSignal mutex(m_receiveMutex);

  if (frame.GetPayloadSize() < 4) {
    PTRACE(2, "RFC2833\tIgnoring packet size " << frame.GetPayloadSize() << " - too small for " << m_baseMediaFormat);
    return;
  }

  const BYTE * payload = frame.GetPayloadPtr();

  char tone = RFC2833ToASCII(payload[0], m_rxEvents[NSECodeBase]);
  if (tone == '\0') {
    PTRACE(2, "RFC2833\tIgnoring packet with code " << payload[0] << " - unsupported event for " << m_baseMediaFormat);
    return;
  }
  unsigned duration  = ((payload[2] <<8) + payload[3]) / 8;
  unsigned timeStamp = frame.GetTimestamp();
  unsigned volume    = (payload[1] & 0x3f);

  // RFC 2833 says to ignore below -55db
  if (volume > 55) {
    PTRACE(2, "RFC2833\tIgnoring packet " << (unsigned)payload[0] << " with volume -" << volume << "db for " << m_baseMediaFormat);
    return;
  }

  PTRACE(4, "RFC2833\tReceived " << ((payload[1] & 0x80) ? "end" : "tone") << ": code='" << (unsigned)payload[0]
         << "', dur=" << duration << ", vol=" << volume << ", ts=" << timeStamp << ", mkr=" << frame.GetMarker()
         << " for " << m_baseMediaFormat);

  // the only safe way to detect a new tone is the timestamp
  // because the packet with the marker bit could go missing and 
  // because some endpoints (*cough* Kapanga *cough*) send multiple marker bits
  bool newTone = (m_tonesReceived == 0) || (timeStamp != m_previousReceivedTimestamp);

  // if new tone, end any current tone and start new one
  if (!newTone) {
    if (m_receiveState == ReceiveActive)
      m_receiveTimer = 200;
    else
      m_receiveTimer.Stop(false);
  }
  else {
    m_receiveTimer.Stop(false);

    // finish any existing tone
    if (m_receiveState == ReceiveActive) 
      OnEndReceive(m_receivedTone, duration, m_previousReceivedTimestamp);

    // do callback for new tone
    OnStartReceive(tone, timeStamp);

    // setup for new tone
    m_receivedTone = tone;
    m_receiveTimer = 200;
    m_receiveState = ReceiveActive;
  }

  // if end of active tone, do callback and change to idle 
  // no else, so this works for single packet tones too
  if ((m_receiveState == ReceiveActive) && ((payload[1]&0x80) != 0)) 
    OnEndReceive(m_receivedTone, duration, timeStamp);
}


void OpalRFC2833Proto::ReceiveTimeout(PTimer & timer, P_INT_PTR)
{
  if (!m_receiveMutex.Try()) {
    timer = 2; // Try again in a couple of milliseconds
    return;
  }

  PTRACE(3, "RFC2833\tTimeout occurred while receiving " << (unsigned)m_receivedTone
         << " for " << m_baseMediaFormat);

  if (m_receiveState != ReceiveIdle) {
    m_receiveState = ReceiveIdle;
    //OnEndReceive(receivedTone, 0, 0);
  }

  m_receiveMutex.Signal();

  m_receiveTimer.Stop(false);
}


/////////////////////////////////////////////////////////////////////////////
