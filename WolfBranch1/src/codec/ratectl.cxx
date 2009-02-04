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
  byteRate = bitRateHistorySizeInMs = maxConsecutiveFramesSkip = 0;
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
      (_outputFrameTime != targetOutputFrameTime) ||
      (_historySizeInMs != bitRateHistorySizeInMs) ||
      (_maxConsecutiveFramesSkip != maxConsecutiveFramesSkip) ||
      (newByteRate != byteRate) 
      ) {

    PTRACE(3, "RateController\tNew paramaters: "
           << "bitrate=" << _targetBitRate
           << ", window=" << _historySizeInMs
           << ", frame time=" << _outputFrameTime << "(rate=" << (((2*90000) + _outputFrameTime) / (2 *_outputFrameTime)) << ")"
           << ", max skipped frames=" << _maxConsecutiveFramesSkip);

    // set the new parameters
    byteRate                 = newByteRate;
    bitRateHistorySizeInMs   = _historySizeInMs;
    maxConsecutiveFramesSkip = _maxConsecutiveFramesSkip;
    targetBitRateHistorySize = (byteRate * bitRateHistorySizeInMs) / 1000;
    targetOutputFrameTime    = _outputFrameTime;

    Reset();
  }
}

void OpalVideoRateController::Reset()
{
  bitRateHistory.reset();
  frameRateHistory.reset();

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

  // report every second
  bool reporting = (now - lastReport) > 1000;
  if (reporting)
    lastReport = now;

  if (consecutiveFramesSkipped > maxConsecutiveFramesSkip) {
    PTRACE(5, "RateController\tAllowing frame to avoid too many consecutive skipped frames");
    return false;
  }

  if (CheckFrameRate(reporting) || CheckBitRate(reporting)) {
    consecutiveFramesSkipped++;
    return true;
  }

  return false;
}

bool OpalVideoRateController::CheckFrameRate(bool reporting)
{
  // flush histories older than window
  bitRateHistory.remove_older_than(now, bitRateHistorySizeInMs);
  frameRateHistory.remove_older_than(now, PACKET_HISTORY_SIZE * 1000);

  // the first one is always free
  if (frameRateHistory.size() == 0) {
    PTRACE(5, "RateController\tHistory too small for frame rate control");
    return false;
  }

  PTRACE_IF(3, reporting, "RateController\tReport:Total frames:in=" << inputFrameCount
                          << ",out=" << outputFrameCount
                          << ",dropped=" << (inputFrameCount - outputFrameCount)
                          << "(" << (inputFrameCount < 1 ? 0 : ((inputFrameCount - outputFrameCount) * 100 / inputFrameCount)) << "%)");

  // if maintaining a frame rate, check to see if frame should be dropped
  // to make this decision, check what the rate would be if this frame was not dropped
  if (targetOutputFrameTime > 0) {
  
    // if the frame history covers zero time, it is doubtful that outputting this
    // frame will result in a valid frame rate
    PInt64 frameRateHistoryDuration = now - frameRateHistory.begin()->time;
    if (now == 0) {
      PTRACE(5, "RateController\tDropping frame as frame history has zero length");
      return true;
    }
  
    // output report now we have useful statistics
    PTRACE_IF(3, reporting, "RateController\tReport:"
                 "in="      << ((inputFrameCount-0)  * 1000) / (now - startTime) << " fps,"
                 "out="     << ((outputFrameCount-0) * 1000) / (now - startTime) << " fps,"
                 "target="  << ((90000 + targetOutputFrameTime/2) / targetOutputFrameTime) << " fps,"
                 "history=" << frameRateHistoryDuration << "ms " << frameRateHistory.size() << " frames");

    PInt64 targetFrameHistorySize = frameRateHistoryDuration * 90;

    if (((frameRateHistory.size()-1) * targetOutputFrameTime) > targetFrameHistorySize) {
      PTRACE(3, "RateController\tSkipping frame to enforce frame rate");
      return true;
    }
  }

  return false;
}

bool OpalVideoRateController::CheckBitRate(bool reporting)
{
  bitRateHistory.remove_older_than(now, bitRateHistorySizeInMs);

  // need to have at least 2 frames of history to make any useful predictions
  // and the history must span a non-trivial time window
  PInt64 bitRateHistoryDuration = 0;
  if ((bitRateHistory.size() < 2) || (bitRateHistoryDuration = bitRateHistory.back().time - bitRateHistory.front().time) < 10 || outputFrameCount < 2) {
    PTRACE(3, "RateController\thistory too small for bit rate control");
    return false;
  }

  // calculate average payload and packets per frame
  PInt64 averagePayloadSize     = bitRateHistory.bytes / frameRateHistory.size();
  PInt64 averagePacketsPerFrame = bitRateHistory.packets / frameRateHistory.size();
  if (averagePacketsPerFrame < 1)
    averagePacketsPerFrame = 1;

  // show some statistics
  PTRACE_IF(3, reporting, "RateController\tReport:"
                          "payload=" << (bitRateHistory.bytes * 8 * 1000) / bitRateHistoryDuration << ","
                          "udp="     << ((bitRateHistory.packets * UDP_OVERHEAD + bitRateHistory.bytes) * 8 * 1000) / bitRateHistoryDuration << " bps,"
                          "target="  << (byteRate*8) << ","
                          "history=" << bitRateHistoryDuration << "ms," << bitRateHistory.size() << " frames," << bitRateHistory.packets << " packets"
              );

  // use these to rate limit on payload size
  PInt64 avgPacketSize = averagePayloadSize;
  PInt64 historySize   = bitRateHistory.bytes;

  // Use these to rate limit based on UDP packet size
  //PInt64 avgPacketSize += averagePacketsPerFrame * UDP_OVERHEAD;
  //PInt64 historySize   += bitRateHistory.packets * UDP_OVERHEAD;
  
  // allow the packet if the expected history size with this packet is less 
  // than the target history size 
  if ((historySize + avgPacketSize) <= targetBitRateHistorySize) {
    return false;
  }

  PTRACE(3, "RateController\tSkipping frame to enforce bit rate");
  return true;
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

  bitRateHistory.push(info);
  frameRateHistory.push(info);

  consecutiveFramesSkipped = 0;
}


