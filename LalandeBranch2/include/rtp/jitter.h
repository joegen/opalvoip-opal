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
 * $Revision$
 * $Author$
 * $Date$
 */

#ifndef OPAL_RTP_JITTER_H
#define OPAL_RTP_JITTER_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <opal/buildopts.h>

#include <rtp/rtp.h>


class RTP_JitterBuffer;
class RTP_JitterBufferAnalyser;


///////////////////////////////////////////////////////////////////////////////
/**This is an Abstract jitter buffer, which can be used simply in any
   application. The user is required to use a descendant of this class, and
   provide a "OnReadPacket" method, so that network packets can be placed in
   this class instance */
class OpalJitterBuffer : public PSafeObject
{
  PCLASSINFO(OpalJitterBuffer, PSafeObject);

  public:
    /**Constructor for this jitter buffer. The size of this buffer can be
       altered later with the SetDelay method */
    OpalJitterBuffer(
      unsigned minJitterDelay, ///<  Minimum delay in RTP timestamp units
      unsigned maxJitterDelay, ///<  Maximum delay in RTP timestamp units
      unsigned timeUnits = 8,  ///<  Time units, usually 8 or 16
      PINDEX stackSize = 30000 ///<  Stack size for jitter thread
    );
    
    /**Destructor, which closes this down and deletes the internal list of frames */
    virtual ~OpalJitterBuffer();

    /**Report the statistics for this jitter instance */
    void PrintOn(ostream & strm  ) const;

    /**This method is where this OpalJitterBuffer collects data from the
       outside world.  A descendant class of OpalJitterBuffer will override
       this method

    @return PTrue on successful read, PFalse on faulty read. */
    virtual PBoolean OnReadPacket    (
      RTP_DataFrame & frame,  ///<  Frame read from the RTP session
      PBoolean loop               ///<  If PTrue, loop as long as data is available, if PFalse, only process once
    ) = 0;

//    PINDEX GetSize() const { return bufferSize; }
    /**Set the maximum delay the jitter buffer will operate to.
      */
    void SetDelay(
      unsigned minJitterDelay, ///<  Minimum delay in RTP timestamp units
      unsigned maxJitterDelay  ///<  Maximum delay in RTP timestamp units
    );

    void UseImmediateReduction(PBoolean state) { doJitterReductionImmediately = state; }

    /**Read a data frame from the RTP channel.
       Any control frames received are dispatched to callbacks and are not
       returned by this function. It will block until a data frame is
       available or an error occurs.
      */
    virtual PBoolean ReadData(
      RTP_DataFrame & frame   ///<  Frame read from the RTP session
    );

    /**Get current delay for jitter buffer.
      */
    DWORD GetJitterTime() const { return currentJitterTime; }

    /**Get time units.
      */
    unsigned GetTimeUnits() const { return timeUnits; }
    
    /**Get total number received packets too late to go into jitter buffer.
      */
    DWORD GetPacketsTooLate() const { return packetsTooLate; }

    /**Get total number received packets that overran the jitter buffer.
      */
    DWORD GetBufferOverruns() const { return bufferOverruns; }

    /**Get maximum consecutive marker bits before buffer starts to ignore them.
      */
    DWORD GetMaxConsecutiveMarkerBits() const { return maxConsecutiveMarkerBits; }

    /**Set maximum consecutive marker bits before buffer starts to ignore them.
      */
    void SetMaxConsecutiveMarkerBits(DWORD max) { maxConsecutiveMarkerBits = max; }

    /**Start jitter thread
      */
    virtual void Resume();

    PDECLARE_NOTIFIER(PThread, OpalJitterBuffer, JitterThreadMain);

    PBoolean WaitForTermination(const PTimeInterval & t)
    { 
      if (jitterThread == NULL) 
        return PTrue;
      shuttingDown = true;
      return jitterThread->WaitForTermination(t); 
    }

    bool IsEmpty() { return jitterBuffer.size() == 0; }

  protected:
    void Start(unsigned _minJitterTime, unsigned _maxJitterTime);

    PINDEX        bufferSize;
    DWORD         minJitterTime;
    DWORD         maxJitterTime;
    DWORD         maxConsecutiveMarkerBits;

    unsigned      timeUnits;
    DWORD         currentJitterTime;
    DWORD         packetsTooLate;
    unsigned      bufferOverruns;
    unsigned      consecutiveBufferOverruns;
    DWORD         consecutiveMarkerBits;
    PTimeInterval consecutiveEarlyPacketStartTime;
    DWORD         lastWriteTimestamp;
    PTimeInterval lastWriteTick;
    DWORD         jitterCalc;
    DWORD         targetJitterTime;
    unsigned      jitterCalcPacketCount;
    bool          doJitterReductionImmediately;

    class Entry : public RTP_DataFrame
    {
      public:
        Entry() : RTP_DataFrame(0, 512) { } // Allocate enough for 250ms of L16 audio, zero payload size
        PTimeInterval tick;
    };

    class FrameQueue : public std::deque<Entry *>
    {
      public:
        void resize(size_type _Newsize)
        { 
          while (size() < (size_t)_Newsize)
            push_back(new Entry);
          while (size() > (size_t)_Newsize) {
            delete front();
            pop_front();
          }
        }

        ~FrameQueue()
        { resize(0); }
    };

    FrameQueue freeFrames;
    FrameQueue jitterBuffer;
    inline Entry * GetNewest(bool pop) { Entry * e = jitterBuffer.back(); if (pop) jitterBuffer.pop_back(); return e; }
    inline Entry * GetOldest(bool pop) { Entry * e = jitterBuffer.front(); if (pop) jitterBuffer.pop_front(); return e; }

    Entry * currentFrame;    // storage of current frame

    PMutex bufferMutex;
    bool   shuttingDown;
    bool   preBuffering;
    bool   firstReadData;

    RTP_JitterBufferAnalyser * analyser;

    PThread * jitterThread;
    PINDEX    jitterStackSize;

    PBoolean Init(Entry * & currentReadFrame, PBoolean & markerWarning);
    PBoolean PreRead(Entry * & currentReadFrame, PBoolean & markerWarning);
    PBoolean OnRead(Entry * & currentReadFrame, PBoolean & markerWarning, PBoolean loop);
    void DeInit(Entry * & currentReadFrame, PBoolean & markerWarning);
};

/////////////////////////////////////////////////////////////////////////////
/**A descendant of the OpalJitterBuffer that reads RTP_DataFrame instances
   from the RTP_Sessions */
class RTP_JitterBuffer : public OpalJitterBuffer
{
    PCLASSINFO(RTP_JitterBuffer, OpalJitterBuffer);

 public:
    RTP_JitterBuffer(
      RTP_Session & session,   ///<  Associated RTP session tor ead data from
      unsigned minJitterDelay, ///<  Minimum delay in RTP timestamp units
      unsigned maxJitterDelay, ///<  Maximum delay in RTP timestamp units
      unsigned timeUnits = 8,  ///<  Time units, usually 8 or 16
      PINDEX stackSize = 30000 ///<  Stack size for jitter thread
    );

    /**This class instance collects data from the outside world in this
       method.

    @return PTrue on successful read, PFalse on faulty read. */
    virtual PBoolean OnReadPacket(
      RTP_DataFrame & frame,  ///<  Frame read from the RTP session
      PBoolean loop           ///<  If PTrue, loop as long as data is available, if PFalse, only process once
    );

 protected:
   /**This class extracts data from the outside world by reading from this session variable */
   RTP_Session & session;
};

#endif // OPAL_RTP_JITTER_H


/////////////////////////////////////////////////////////////////////////////
