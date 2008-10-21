/*
 * ratectl.h
 *
 * Video rate controller
 *
 * Open Phone Abstraction Library (OPAL)
 *
 * Copyright (C) 2007 Matthias Schneider
 * Copyright (C) 2008 Post Increment
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
 * The Initial Developer of the Original Code is Matthias Schneider 
 *
 * Contributor(s): Post Increment
 *
 * $Revision: 21293 $
 * $Author: rjongbloed $
 * $Date: 2008-10-13 10:24:41 +1100 (Mon, 13 Oct 2008) $
 */

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "ratectl.h"
#endif

#include <opal/buildopts.h>
#include <codec/ratectl.h>

//
// 20 bytes for nominal TCP header
// 8 bytes for nominal UDP header
// 
#define UDP_OVERHEAD  (20 + 8)

#define new PNEW


/////////////////////////////////////////////////////////////////////////////

static int udiff(unsigned int const subtrahend, unsigned int const subtractor)
{
  return subtrahend - subtractor;
}


static double square(double const arg)
{
  return(arg*arg);
}

double OpalCalcSNR(const BYTE * src1, const BYTE * src2, PINDEX dataLen)
{
  double diff2 = 0.0;
  for (PINDEX i = 0; i < dataLen; ++i)
    diff2 += square(udiff(*src1++, *src2++));

  double const snr = diff2 / dataLen / 255;

  return snr;
}

/////////////////////////////////////////////////////////////////////////////

OpalVideoRateController::OpalVideoRateController()
{
  lastReport = 0;
  byteRate = historySizeInMs = maxConsecutiveFramesSkip = 0;
  Reset();
}

void OpalVideoRateController::Reset()
{
  frameHistory.reset();
  packetHistory.reset();

  consecutiveFramesSkipped = 0;
  startTime                = PTimer::Tick().GetMilliSeconds();
  inputFrameCount          = 0;
  outputFrameCount         = 0;
}

void OpalVideoRateController::Open(unsigned _targetBitRate, 
                                   unsigned _historySizeInMs, 
                                   unsigned _maxConsecutiveFramesSkip)
{
  unsigned newByteRate = (_targetBitRate + 7) / 8; 

  // if any paramaters have changed, reset the rate controller
  if (
      (_historySizeInMs != historySizeInMs) ||
      (_maxConsecutiveFramesSkip != maxConsecutiveFramesSkip) ||
      (newByteRate != byteRate) 
      ) {

    PTRACE(3, "Patch\tSet new paramaters for rate controller : " 
           << "bitrate=" << _targetBitRate
           << ", window=" << _historySizeInMs);

    // set the new parameters
    byteRate                 = newByteRate;
    historySizeInMs          = _historySizeInMs;
    maxConsecutiveFramesSkip = _maxConsecutiveFramesSkip;
    targetHistorySize        = (byteRate * historySizeInMs) / 1000;

    Reset();
  }
}

bool OpalVideoRateController::SkipFrame()
{
  inputFrameCount++;

  // get "now"
  now = PTimer::Tick().GetMilliSeconds();

  // flush history older than window
  while (
         (frameHistory.size() != 0) && 
         ((now - frameHistory.begin()->time) > historySizeInMs)
         ) 
    frameHistory.pop();

  // need to have at least 2 frames of history to make any useful predictions
  // and the history must span a non-trivial time window
  if (frameHistory.size() < 2)
    return false;
  PInt64 range = now - frameHistory.begin()->time;
  if (range < 20)
    return false;

  PInt64 averagePayloadSize     = packetHistory.bytes / packetHistory.size();
  PInt64 averagePacketsPerFrame = packetHistory.packets / packetHistory.size();
  if (averagePacketsPerFrame < 1)
    averagePacketsPerFrame = 1;

  PInt64 avgPacketSize = averagePacketsPerFrame * UDP_OVERHEAD + averagePayloadSize;

  if ((now - lastReport) > 1000) {
    PTRACE(5, "RateController\n"
              "Frame rate  : in=" << (inputFrameCount * 1000) / (now - startTime) << ",out=" << (outputFrameCount * 1000) / (now - startTime) << "\n"
              "Total frames: in=" << inputFrameCount << ",out=" << outputFrameCount << ",dropped=" << (inputFrameCount - outputFrameCount) << "(" << (inputFrameCount - outputFrameCount) * 100 / inputFrameCount << "%)\n"
              "Output frame: avg packet size" << avgPacketSize << ",avg packet per frame=" << averagePacketsPerFrame << " packets\n"
              "Bit rate    : total=" << ((frameHistory.packets * UDP_OVERHEAD + frameHistory.bytes) * 8 * 1000) / range << " bps,payload=" << (frameHistory.bytes * 8 * 1000) / range << "," << range << "ms," << frameHistory.size() << ",frames," << frameHistory.packets << " packets\n"
              );
    lastReport = now;
  }

  // calculate history size, scaled to one second of data
  // note that the variable "historyInBytes" is the current total history size, and thus must be scaled to
  // before comparison to the "targetHistorySize" which is always bytes per second
  PInt64 scaledHistorySize = ((frameHistory.packets * UDP_OVERHEAD + frameHistory.bytes) * historySizeInMs) / range;

  // allow the packet if the expected history size with this packet is less 
  // than half the target history size plus half the average packet size
  // if it is likely this frame will fit, then allow it
  if ((scaledHistorySize + (avgPacketSize / 2)) <= targetHistorySize) {
    consecutiveFramesSkipped = 0;
    return false;
  }

  // see if max consecutive frames has been reached
  if (++consecutiveFramesSkipped <= maxConsecutiveFramesSkip) 
    return true;

  PTRACE(5, "Patch\tAllowing bit rate to be exceeded after " << consecutiveFramesSkipped-1 << " skipped frames");
  
  consecutiveFramesSkipped = 0;
  return false;
}

void OpalVideoRateController::AddFrame(PInt64 totalPayloadSize, int packetCount, PInt64 _now)
{
  now = _now;
  AddFrame(totalPayloadSize, packetCount);
}


void OpalVideoRateController::AddFrame(PInt64 totalPayloadSize, int packetCount)
{
  outputFrameCount++;

  FrameInfo info;
  info.time             = now;
  info.totalPayloadSize = totalPayloadSize;
  info.packetCount      = packetCount;

  frameHistory.push(info);
  packetHistory.push(info);
  if (packetHistory.size() > 5) 
    packetHistory.pop();
}


