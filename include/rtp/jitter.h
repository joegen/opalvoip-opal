/*
 * jitter.h
 *
 * Jitter buffer support
 *
 * Open H323 Library
 *
 * Copyright (c) 1999-2001 Equivalence Pty. Ltd.
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
 * $Log: jitter.h,v $
 * Revision 1.2002  2001/10/05 00:22:13  robertj
 * Updated to PWLib 1.2.0 and OpenH323 1.7.0
 *
 * Revision 2.0  2001/07/27 15:48:24  robertj
 * Conversion of OpenH323 to Open Phone Abstraction Library (OPAL)
 *
 * Revision 1.8  2001/09/11 00:21:21  robertj
 * Fixed missing stack sizes in endpoint for cleaner thread and jitter thread.
 *
 * Revision 1.7  2001/02/09 05:16:24  robertj
 * Added #pragma interface for GNU C++.
 *
 * Revision 1.6  2000/05/25 02:26:12  robertj
 * Added ignore of marker bits on broken clients that sets it on every RTP packet.
 *
 * Revision 1.5  2000/05/04 11:49:21  robertj
 * Added Packets Too Late statistics, requiring major rearrangement of jitter buffer code.
 *
 * Revision 1.4  2000/05/02 04:32:24  robertj
 * Fixed copyright notice comment.
 *
 * Revision 1.3  2000/04/30 03:56:14  robertj
 * More instrumentation to analyse jitter buffer operation.
 *
 * Revision 1.2  2000/03/20 20:51:13  robertj
 * Fixed possible buffer overrun problem in RTP_DataFrames
 *
 * Revision 1.1  1999/12/23 23:02:35  robertj
 * File reorganision for separating RTP from H.323 and creation of LID for VPB support.
 *
 */

#ifndef __RTP_JITTER_H
#define __RTP_JITTER_H

#ifdef __GNUC__
#pragma interface
#endif


#include <rtp/rtp.h>


class RTP_JitterBufferAnalyser;


///////////////////////////////////////////////////////////////////////////////

class RTP_JitterBuffer : public PThread
{
  PCLASSINFO(RTP_JitterBuffer, PThread);

  public:
    RTP_JitterBuffer(
      RTP_Session & session,   /// Associated RTP session tor ead data from
      unsigned jitterDelay,    /// Delay in RTP timestamp units
      PINDEX stackSize = 30000 /// Stack size for jitter thread
    );
    ~RTP_JitterBuffer();

//    PINDEX GetSize() const { return bufferSize; }
    /**Set the maximum delay the jitter buffer will operate to.
      */
    void SetDelay(
      unsigned delay    /// Delay in RTP timestamp units
    );

    /**Read a data frame from the RTP channel.
       Any control frames received are dispatched to callbacks and are not
       returned by this function. It will block until a data frame is
       available or an error occurs.
      */
    virtual BOOL ReadData(
      DWORD timestamp,        /// Timestamp to read from buffer.
      RTP_DataFrame & frame   /// Frame read from the RTP session
    );

    /**Get total number received packets too late to go into jitter buffer.
      */
    DWORD GetPacketsTooLate() const { return packetsTooLate; }

    /**Get maximum consecutive marker bits before buffer starts to ignore them.
      */
    DWORD GetMaxConsecutiveMarkerBits() const { return maxConsecutiveMarkerBits; }

    /**Set maximum consecutive marker bits before buffer starts to ignore them.
      */
    void SetMaxConsecutiveMarkerBits(DWORD max) { maxConsecutiveMarkerBits = max; }


  protected:
    virtual void Main();

    class Entry : public RTP_DataFrame
    {
      public:
        Entry * next;
        Entry * prev;
    };

    RTP_Session & session;
    PINDEX        bufferSize;
    DWORD         maxJitterTime;
    DWORD         maxConsecutiveMarkerBits;

    unsigned currentDepth;
    DWORD    packetsTooLate;
    DWORD    consecutiveMarkerBits;

    Entry * oldestFrame;
    Entry * newestFrame;
    Entry * freeFrames;
    Entry * currentWriteFrame;

    PMutex bufferMutex;
    BOOL   shuttingDown;
    BOOL   preBuffering;

    RTP_JitterBufferAnalyser * analyser;
};


#endif // __RTP_JITTER_H


/////////////////////////////////////////////////////////////////////////////
