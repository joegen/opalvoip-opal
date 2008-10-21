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

class OpalVideoRateController
{
  public:
    OpalVideoRateController();
    void Reset();
    void Open(unsigned targetBitRate, unsigned windowSizeInMs = 5000, unsigned rcMaxConsecutiveFramesSkip = 5);
    bool SkipFrame();
    void AddFrame(PInt64 sizeInBytes, int packetPacketCount);
    void AddFrame(PInt64 sizeInBytes, int packetPacketCount, PInt64 now);

  protected:
    unsigned byteRate;
    unsigned historySizeInMs;
    unsigned maxConsecutiveFramesSkip;
    PInt64  targetHistorySize;
    PInt64  startTime;
    PInt64  inputFrameCount;
    PInt64  outputFrameCount;

    unsigned consecutiveFramesSkipped;

    PInt64 now;
    PInt64 lastReport;

    struct FrameInfo {
      PInt64  time;
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

        PInt64 bytes;
        int packets;

    };

    FrameInfoList frameHistory;
    FrameInfoList packetHistory;
};

extern double OpalCalcSNR(const BYTE * src1, const BYTE * src2, PINDEX dataLen);

#endif // OPAL_RATE_CONTROL_H
