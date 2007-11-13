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
 * Revision 2.11  2007/01/09 23:04:41  dereksmithies
 * Abstract out the jitterbuffer. This enables the use of the jitter buffer
 * in quite different circumstances (such as iax2 and test programs).
 *
 * Revision 2.10  2006/11/20 07:27:47  csoutheren
 * Fix compilation problem on Linux
 *
 * Revision 2.9  2006/11/20 03:37:12  csoutheren
 * Allow optional inclusion of RTP aggregation
 *
 * Revision 2.8  2005/12/30 14:29:15  dsandras
 * Removed the assumption that the jitter will contain a 8 kHz signal.
 *
 * Revision 2.7  2005/11/30 13:35:26  csoutheren
 * Changed tags for Doxygen
 *
 * Revision 2.6  2004/02/19 10:47:01  rjongbloed
 * Merged OpenH323 version 1.13.1 changes.
 *
 * Revision 2.5  2002/11/10 11:33:17  robertj
 * Updated to OpenH323 v1.10.3
 *
 * Revision 2.4  2002/09/16 02:52:35  robertj
 * Added #define so can select if #pragma interface/implementation is used on
 *   platform basis (eg MacOS) rather than compiler, thanks Robert Monaghan.
 *
 * Revision 2.3  2002/09/04 06:01:47  robertj
 * Updated to OpenH323 v1.9.6
 *
 * Revision 2.2  2002/04/15 08:50:14  robertj
 * Added buffer reset on excess buffer overruns.
 *
 * Revision 2.1  2001/10/05 00:22:13  robertj
 * Updated to PWLib 1.2.0 and OpenH323 1.7.0
 *
 * Revision 2.0  2001/07/27 15:48:24  robertj
 * Conversion of OpenH323 to Open Phone Abstraction Library (OPAL)
 *
 * Revision 1.13  2003/10/28 22:38:31  dereksmithies
 * Rework of jitter buffer. Many thanks to Henry Harrison of Alice Street.
 *
 * Revision 1.12ACC1.0 6th October 2003 henryh
 * Complete change to adaptive algorithm 
 *
 * Revision 1.12  2002/10/31 00:32:39  robertj
 * Enhanced jitter buffer system so operates dynamically between minimum and
 *   maximum values. Altered API to assure app writers note the change!
 *
 * Revision 1.11  2002/09/16 01:14:15  robertj
 * Added #define so can select if #pragma interface/implementation is used on
 *   platform basis (eg MacOS) rather than compiler, thanks Robert Monaghan.
 *
 * Revision 1.10  2002/09/03 05:40:18  robertj
 * Normalised the multi-include header prevention ifdef/define symbol.
 * Added buffer reset on excess buffer overruns.
 * Added ability to get buffer overruns for statistics display.
 *
 * Revision 1.9  2002/08/05 10:03:47  robertj
 * Cosmetic changes to normalise the usage of pragma interface/implementation.
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

#ifndef __OPAL_JITTER_H
#define __OPAL_JITTER_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <rtp/rtp.h>

class RTP_JitterBuffer;
class RTP_JitterBufferAnalyser;


///////////////////////////////////////////////////////////////////////////////
/**This is an Abstract jitter buffer, which can be used simply in any
   application. The user is required to use a descendant of this class, and
   provide a "OnReadPacket" method, so that network packets can be placed in
   this class instance */
class OpalJitterBuffer : public PObject
{
  PCLASSINFO(OpalJitterBuffer, PObject);

  public:
#if OPAL_RTP_AGGREGATE
    friend class RTP_AggregatedHandle;
#endif
    /**Constructor for this jitter buffer. The size of this buffer can be
       altered later with the SetDelay method */
    OpalJitterBuffer(
      unsigned minJitterDelay, ///<  Minimum delay in RTP timestamp units
      unsigned maxJitterDelay, ///<  Maximum delay in RTP timestamp units
      unsigned timeUnits = 8,  ///<  Time units, usually 8 or 16
      PINDEX stackSize = 30000 ///<  Stack size for jitter thread
    );
    
    /**Destructor, which closes this down and deletes the internal list of frames */
    ~OpalJitterBuffer();

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
      DWORD timestamp,        ///<  Timestamp to read from buffer.
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
    void Resume(PHandleAggregator * aggregator = NULL);

    PDECLARE_NOTIFIER(PThread, OpalJitterBuffer, JitterThreadMain);

    PBoolean WaitForTermination(const PTimeInterval & t)
    { return (jitterThread == NULL) ? PTrue : jitterThread->WaitForTermination(t); }

  protected:
    class Entry : public RTP_DataFrame
    {
      public:
        Entry * next;
        Entry * prev;
        PTimeInterval tick;
    };

    PINDEX        bufferSize;
    DWORD         minJitterTime;
    DWORD         maxJitterTime;
    DWORD         maxConsecutiveMarkerBits;

    unsigned timeUnits;
    unsigned currentDepth;
    DWORD    currentJitterTime;
    DWORD    packetsTooLate;
    unsigned bufferOverruns;
    unsigned consecutiveBufferOverruns;
    DWORD    consecutiveMarkerBits;
    PTimeInterval    consecutiveEarlyPacketStartTime;
    DWORD    lastWriteTimestamp;
    PTimeInterval lastWriteTick;
    DWORD    jitterCalc;
    DWORD    targetJitterTime;
    unsigned jitterCalcPacketCount;
    PBoolean     doJitterReductionImmediately;
    PBoolean     doneFreeTrash;

    Entry * oldestFrame;
    Entry * newestFrame;
    Entry * freeFrames;
    Entry * currentWriteFrame;

    PMutex bufferMutex;
    PBoolean   shuttingDown;
    PBoolean   preBuffering;
    PBoolean   doneFirstWrite;

    RTP_JitterBufferAnalyser * analyser;

    PThread * jitterThread;
    PINDEX    jitterStackSize;

    RTP_AggregatedHandle * aggregratedHandle;

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
    virtual PBoolean OnReadPacket    (
	RTP_DataFrame & frame,  ///<  Frame read from the RTP session
	PBoolean loop               ///<  If PTrue, loop as long as data is available, if PFalse, only process once
	) ;

 protected:
	/**This class extracts data from the outside world by reading from this session variable */
   RTP_Session & session;
};



#endif // __OPAL_JITTER_H


/////////////////////////////////////////////////////////////////////////////
