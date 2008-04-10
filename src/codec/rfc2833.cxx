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
#include <opal/rtpconn.h>
#include <rtp/rtp.h>

static const char RFC2833Table1Events[] = "0123456789*#ABCD!";
const char * OpalDefaultNTEString = "0-15,32-49";

#if OPAL_T38FAX
const char * OpalDefaultNSEString = "192,193";
#endif

#define new PNEW


///////////////////////////////////////////////////////////////////////////////

OpalRFC2833Info::OpalRFC2833Info(char t, unsigned d, unsigned ts)
{
  tone = t;
  duration = d;
  timestamp = ts;
}


///////////////////////////////////////////////////////////////////////////////

OpalRFC2833Proto::OpalRFC2833Proto(OpalRTPConnection & _conn, const PNotifier & rx)
  : conn(_conn), receiveNotifier(rx),
#ifdef _MSC_VER
#pragma warning(disable:4355)
#endif
    receiveHandler(PCREATE_NOTIFIER(ReceivedPacket))
#ifdef _MSC_VER
#pragma warning(default:4355)
#endif
{
  PTRACE(4, "RFC2833\tHandler created");

  payloadType = RTP_DataFrame::IllegalPayloadType;
  receiveTimer.SetNotifier(PCREATE_NOTIFIER(ReceiveTimeout));

  receiveState  = ReceiveIdle;
  transmitState = TransmitIdle;

  asyncTransmitTimer.SetNotifier(PCREATE_NOTIFIER(AsyncTimeout));
  rtpSession = NULL;

  tonesReceived = 0;
}

OpalRFC2833Proto::~OpalRFC2833Proto()
{
  if (rtpSession != NULL)
    conn.ReleaseSession(1);
}

PBoolean OpalRFC2833Proto::SendToneAsync(char tone, unsigned duration)
{
  PWaitAndSignal m(mutex);

  // find an audio session in the current connection to send the packet on
  if (rtpSession == NULL) {
    rtpSession = conn.UseSession(OpalMediaFormat::DefaultAudioSessionID);
    if (rtpSession == NULL) {
      PTRACE(4, "RFC2833\tNo RTP session suitable for RFC2833");
      return PFalse;
    }
  }

  // if not ' ', then start a new tone
  // if ' ', then update last tone sent
  if (tone != ' ') {
    if (!BeginTransmit(tone))
      return PFalse;
  }

  // reset the duration and retransmit timers
  asyncDurationTimer = duration;
  asyncTransmitTimer.RunContinuous(30);

  // send the current frame
  SendAsyncFrame();

  return PTrue;
}

void OpalRFC2833Proto::SendAsyncFrame()
{
  // be thread safe
  PWaitAndSignal m(mutex);

  // if transmittter is ever in this state, then stop the duration timer
  if (transmitState == TransmitIdle) {
    asyncDurationTimer.Stop();
    return;
  }

  RTP_DataFrame frame;
  frame.SetPayloadType(payloadType);
  frame.SetPayloadSize(4);

  BYTE * payload = frame.GetPayloadPtr();
  payload[0] = transmitCode; // tone
  payload[1] = (7 & 0x3f);   // Volume

  // set end bit if sending last three packets
  switch (transmitState) {
    case TransmitActive:
      // if the duration has ended, then go into ending state
      if (!asyncDurationTimer.IsRunning()) 
        transmitState = TransmitEnding1;

      // set duration to time since start of time
      if (asyncStart != PTimeInterval(0)) 
        transmitDuration = (PTimer::Tick() - asyncStart).GetInterval() * 8;
      else {
        transmitDuration = 0;
        frame.SetMarker(PTrue);
        asyncStart = PTimer::Tick();
      }
      break;
    case TransmitEnding1:
      transmitDuration = (PTimer::Tick() - asyncStart).GetInterval() * 8;
      payload[1] |= 0x80;
      transmitState = TransmitEnding2;
      break;
    case TransmitEnding2:
      payload[1] |= 0x80;
      transmitState = TransmitEnding3;
      break;
    case TransmitEnding3:
      payload[1] |= 0x80;
      transmitState = TransmitIdle;
      asyncTransmitTimer.Stop();
      break;
    default:
      PAssertAlways("RFC2833\tunknown transmit state");
      return;
  }

  // set tone duration
  payload[2] = (BYTE)(transmitDuration >> 8);
  payload[3] = (BYTE) transmitDuration;

  if (rtpSession != NULL) {
    if (!rewriteTransmitTimestamp)
      frame.SetTimestamp(transmitTimestamp);
    rtpSession->WriteOOBData(frame, rewriteTransmitTimestamp);
    if (rewriteTransmitTimestamp) {
      transmitTimestamp        = frame.GetTimestamp();
      rewriteTransmitTimestamp = false;
    } 
  }

  PTRACE(4, "RFC2833\tSending " << ((payload[1] & 0x80) ? "end" : "tone") << ":tone=" << (unsigned)transmitCode << ",dur=" << transmitDuration << ",ts=" << frame.GetTimestamp() << ",mkr=" << frame.GetMarker());
}

PINDEX OpalRFC2833Proto::ASCIIToRFC2833(char tone)
{
  const char * theChar = strchr(RFC2833Table1Events, tone);
  if (theChar == NULL) {
    PTRACE(1, "RFC2833\tInvalid tone character.");
    return P_MAX_INDEX;
  }

  return (PINDEX)(theChar-RFC2833Table1Events);
}

char OpalRFC2833Proto::RFC2833ToASCII(PINDEX rfc2833)
{
  PASSERTINDEX(rfc2833);
  if (rfc2833 < (PINDEX)sizeof(RFC2833Table1Events)-1)
    return RFC2833Table1Events[rfc2833];
  return '\0';
}


PBoolean OpalRFC2833Proto::BeginTransmit(char tone)
{
  PWaitAndSignal m(mutex);

  // convert tone to correct code
  PINDEX code = ASCIIToRFC2833(tone);
  if (code == P_MAX_INDEX) {
    PTRACE(1, "RFC2833\tInvalid tone character.");
    return PFalse;
  }

  // kick off the transmitter
  transmitCode             = (BYTE)code;
  transmitState            = TransmitActive;
  rewriteTransmitTimestamp = true;
  asyncStart               = 0;
  return PTrue;
}

void OpalRFC2833Proto::AsyncTimeout(PTimer &, INT)
{
  SendAsyncFrame();
}

void OpalRFC2833Proto::OnStartReceive(char tone, unsigned timestamp)
{
  ++tonesReceived;
  previousReceivedTimestamp = timestamp;
  OnStartReceive(tone);
  OpalRFC2833Info info(tone, timestamp);
  receiveNotifier(info, 0);
}

void OpalRFC2833Proto::OnStartReceive(char)
{
}


void OpalRFC2833Proto::OnEndReceive(char tone, unsigned duration, unsigned timestamp)
{
  receiveTimer.Stop(false);
  receiveState = ReceiveIdle;
  OpalRFC2833Info info(tone, duration, timestamp);
  receiveNotifier(info, 1);
}


void OpalRFC2833Proto::ReceivedPacket(RTP_DataFrame & frame, INT)
{
  if (frame.GetPayloadType() != payloadType || frame.GetPayloadSize() == 0)
    return;

  PWaitAndSignal m(mutex);

  if (frame.GetPayloadSize() < 4) {
    PTRACE(2, "RFC2833\tIgnoring packet size " << frame.GetPayloadSize() << " - too small.");
    return;
  }

  const BYTE * payload = frame.GetPayloadPtr();

  BYTE tone = RFC2833ToASCII(payload[0]);
  if (tone == '\0') {
    PTRACE(2, "RFC2833\tIgnoring packet " << payload[0] << " - unsupported event.");
    return;
  }
  unsigned duration  = ((payload[2] <<8) + payload[3]) / 8;
  unsigned timeStamp = frame.GetTimestamp();
  unsigned volume    = (payload[1] & 0x3f);

  // RFC 2833 says to ignore below -55db
  if (volume > 55) {
    PTRACE(2, "RFC2833\tIgnoring packet " << payload[0] << " with volume -" << volume << "db");
    return;
  }

  PTRACE(4, "RFC2833\tReceived " << ((payload[1] & 0x80) ? "end" : "tone") << ":tone=" << (unsigned)tone << ",dur=" << duration << ",vol=" << volume << ",ts=" << timeStamp << ",mkr=" << frame.GetMarker());

  // the only safe way to detect a new tone is the timestamp
  // because the packet with the marker bit could go missing and 
  // because some endpoints (*cough* Kapanga *cough*) send multiple marker bits
  bool newTone = (tonesReceived == 0) || (timeStamp != previousReceivedTimestamp);

  // if new tone, end any current tone and start new one
  if (newTone) {

    // finish any existing tone
    if (receiveState == ReceiveActive) 
      OnEndReceive(receivedTone, duration, timeStamp);

    // do callback for new tone
    OnStartReceive(tone, timeStamp);

    // setup for new tone
    receivedTone = tone;
    receiveTimer = 200;
    receiveState = ReceiveActive;
  }

  // if end of active tone, do callback and change to idle 
  // no else, so this works for single packet tones too
  if ((receiveState == ReceiveActive) && ((payload[1]&0x80) != 0)) 
    OnEndReceive(receivedTone, duration, timeStamp);
}


void OpalRFC2833Proto::ReceiveTimeout(PTimer &, INT)
{
  PTRACE(3, "RFC2833\tTimeout occurred while receiving " << (unsigned)receivedTone);

  PWaitAndSignal m(mutex);

  if (receiveState != ReceiveIdle) {
    receiveTimer.Stop();
    receiveState = ReceiveIdle;
    //OnEndReceive(receivedTone, 0, 0);
  }
}

/////////////////////////////////////////////////////////////////////////////
