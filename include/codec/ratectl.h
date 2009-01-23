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

#ifndef OPAL_RATE_CONTROL_H
#define OPAL_RATE_CONTROL_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <list>

#include <opal/buildopts.h>

//
//  This file implements a video rate controller that seeks to maintain a constant bit rate 
//  by indicating when encoded video frames should be dropped
//
//  To use the rate controller, open it with the appropriate parameters. 
//
//  Before encoding a potential output frame, use the SkipFrame function to determine if the 
//  frame should be skipped. If the frame is not skipped, encode the frame and then call AddFrame
//  with the parameters of the final data.
//


class OpalVideoRateController
{
  public:
    OpalVideoRateController();

    /** Open the rate controller with the specific parameters
      */
    void Open(
      unsigned targetBitRate,                  ///< target bit rate to acheive
      int outputFrameTime = -1,                ///< output frame time (90000 / rate), or -1 to not limit frame rate
      unsigned windowSizeInMs = 5000,          ///< size of history used for calculating output bit rate
      unsigned maxConsecutiveFramesSkip = 5    ///< maximum number of consecutive frames to skip
    );

    /** Determine if the next frame should be skipped
      */
    bool SkipFrame();

    /** Add information about an encoded frame 
      */
    void AddFrame(
      PInt64 sizeInBytes,                      ///< total payload size in bytes, including all RTP headers
      int packetPacketCount                    ///< total number of RTP packets sent
    );

  protected:
    bool CheckFrameRate(bool reporting);
    bool CheckBitRate(bool reporting);

    void Reset();
    void AddFrame(
      PInt64 sizeInBytes, 
      int packetPacketCount, 
      PInt64 now);

    unsigned byteRate;
    unsigned bitRateHistorySizeInMs;
    unsigned maxConsecutiveFramesSkip;
    int targetOutputFrameTime;

    PInt64  targetBitRateHistorySize;
    PInt64  startTime;
    PInt64  inputFrameCount;
    PInt64  outputFrameCount;

    unsigned consecutiveFramesSkipped;

    PInt64 now;
    PInt64 lastReport;

    struct FrameInfo {
      PInt64 time;
      PInt64 totalPayloadSize;
      int packetCount;
    };

    class FrameInfoList : public std::list<FrameInfo> 
    {
      public:
        FrameInfoList()
        { reset(); }

        void reset()
        {
          resize(0);
          bytes = packets = 0;
        }

        void push(const FrameInfo & info)
        {
          bytes   += info.totalPayloadSize;
          packets += info.packetCount;
          push_back(info);
        }

        void pop()
        {
          bytes   -= front().totalPayloadSize;
          packets -= front().packetCount;
          pop_front();
        }

        void remove_older_than(PInt64 now, PInt64 age)
        {
          while ((size() != 0) && ((now - begin()->time) > age))
            pop();
        }

        PInt64 bytes;
        int packets;

    };

    FrameInfoList bitRateHistory;
    FrameInfoList frameRateHistory;
};

extern double OpalCalcSNR(const BYTE * src1, const BYTE * src2, PINDEX dataLen);

#endif // OPAL_RATE_CONTROL_H
