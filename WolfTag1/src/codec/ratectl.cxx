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

//
// size of history used for calcluating average packet size
//
#define PACKET_HISTORY_SIZE   5

#define new PNEW


//
//  This file implements a video rate controller that seeks to maintain a constant bit rate 
//  by indicating when encoded video frames should be dropped
//
//  The instantaneous bit rate is monitored by calculating the total number of bytes that have been 
//  transmitted over the past few seconds. This decision to drop a frame is based on whether the 
//  the actual transmitted count is less, or more, then the number of bytes that would have been 
//  transmitted if the target bit rate had been maintained. 
//
//  The size of this history used to calculate the current bit rate is set when the rate 
//  controller is opened. Experience shows that a history of 5000ms seems to work well.
//
//  The decision to drop a frame is made before the frame is encoded. The rate controller predicts
//  the probable size of the encoded frame by looking at the previous 5 encoded frames. Experiments
//  were done with longer histories (for example the same history used for the bit rate calculation)
//  but it was found that this tended to over-estimate the probable frame size due to the inclusion 
//  of occasional I-frames.
//
//  The maximum number of consecutive dropped frames is also set when the rate controller is opened. 
//
//  The bit rate calculations take into account the 28 bytes of IP and UDP overhead on every RTP packet
//  This can make a big difference when small video packets are being transmitted
//
//  Additionally, this code can also enforce an output frame rate
//

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

void OpalVideoRateController::Open(unsigned _targetBitRate, 
                                        int _outputFrameTime,
                                   unsigned _historySizeInMs, 
                                   unsigned _maxConsecutiveFramesSkip)
{
  // convert bit rate to byte rate
  unsigned newByteRate = (_targetBitRate + 7) / 8; 

  // if any paramaters have changed, reset the rate controller
  if (
      (_outputFrameTime != outputFrameTime) ||
      (_historySizeInMs != historySizeInMs) ||
      (_maxConsecutiveFramesSkip != maxConsecutiveFramesSkip) ||
      (newByteRate != byteRate) 
      ) {

    PTRACE(3, "Patch\tSet new paramaters for rate controller : " 
           << "bitrate=" << _targetBitRate
           << ", window=" << _historySizeInMs
           << ", frame rate=" << _outputFrameTime);

    // set the new parameters
    byteRate                 = newByteRate;
    historySizeInMs          = _historySizeInMs;
    maxConsecutiveFramesSkip = _maxConsecutiveFramesSkip;
    targetHistorySize        = (byteRate * historySizeInMs) / 1000;
    outputFrameTime          = _outputFrameTime;

    Reset();
  }
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
  if (range < 200)
    return false;

  // calculate average payload and packets per frame
  PInt64 averagePayloadSize     = packetHistory.bytes / packetHistory.size();
  PInt64 averagePacketsPerFrame = packetHistory.packets / packetHistory.size();
  if (averagePacketsPerFrame < 1)
    averagePacketsPerFrame = 1;
  PInt64 avgPacketSize = averagePacketsPerFrame * UDP_OVERHEAD + averagePayloadSize;

  // show some statistics
  if ((now - lastReport) > 1000) {
    PTRACE(5, "RateController\n"
              "Frame rate  : in=" << (inputFrameCount * 1000) / (now - startTime) << ",out=" << (outputFrameCount * 1000) / (now - startTime) << "\n"
              "Total frames: in=" << inputFrameCount << ",out=" << outputFrameCount << ",dropped=" << (inputFrameCount - outputFrameCount) << "(" << (inputFrameCount - outputFrameCount) * 100 / inputFrameCount << "%)\n"
              "Output frame: avg packet size" << avgPacketSize << ",avg packet per frame=" << averagePacketsPerFrame << " packets\n"
              "Bit rate    : total=" << ((frameHistory.packets * UDP_OVERHEAD + frameHistory.bytes) * 8 * 1000) / range << " bps,payload=" << (frameHistory.bytes * 8 * 1000) / range << "," << range << "ms," << frameHistory.size() << ",frames," << frameHistory.packets << " packets\n"
              );
    lastReport = now;
  }

  // if maintaining a frame rate, check to see if frame should be dropped
  if (outputFrameTime > 0) {
    PInt64 desiredFrameCount = ((range * 90) + (outputFrameTime/2)) / outputFrameTime;
    cout << "desiredFrameCount = " << desiredFrameCount << ",actual=" << frameHistory.size() << endl;
    if ((frameHistory.size()+1) > desiredFrameCount)
      return true;
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
  if (packetHistory.size() > PACKET_HISTORY_SIZE) 
    packetHistory.pop();
}


