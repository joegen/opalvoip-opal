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
#include <ptlib/safecoll.h>


class RTP_JitterBuffer;
class RTP_JitterBufferAnalyser;
class OpalRTPSession;


///////////////////////////////////////////////////////////////////////////////
/**This is an Abstract jitter buffer, which can be used simply in any
   application. The user is required to use a descendant of this class, and
   provide a "OnReadPacket" method, so that network packets can be placed in
   this class instance */
class OpalJitterBuffer : public PSafeObject
{
  PCLASSINFO(OpalJitterBuffer, PSafeObject);

  public:
  /**@name Construction */
  //@{
    /**Constructor for this jitter buffer. The size of this buffer can be
       altered later with the SetDelay method
      */
    OpalJitterBuffer(
      unsigned minJitterDelay, ///<  Minimum delay in RTP timestamp units
      unsigned maxJitterDelay, ///<  Maximum delay in RTP timestamp units
      unsigned timeUnits = 8,  ///<  Time units, usually 8 or 16
      PINDEX packetSize = 2048 ///<  Max RTP packet size
    );
    
    /** Destructor, which closes this down and deletes the internal list of frames
      */
    virtual ~OpalJitterBuffer();
  //@}

  /**@name Overrides from PObject */
  //@{
    /**Report the statistics for this jitter instance */
    void PrintOn(
      ostream & strm
    ) const;
  //@}

  /**@name Operations */
  //@{
    /// Start jitter buffer.
    virtual void Start() { }

    /**Set the maximum delay the jitter buffer will operate to.
      */
    void SetDelay(
      unsigned minJitterDelay, ///<  Minimum delay in RTP timestamp units
      unsigned maxJitterDelay, ///<  Maximum delay in RTP timestamp units
      PINDEX packetSize = 2048 ///<  Max RTP packet size
    );

    /**Reset jitter buffer.
      */
    void Reset();

    /**Write data frame from the RTP channel.
      */
    virtual PBoolean WriteData(
      const RTP_DataFrame & frame,   ///< Frame to feed into jitter buffer
      const PTimeInterval & tick = 0 ///< Real time tick for packet arrival
    );

    /**Read a data frame from the jitter buffer.
       This function never blocks. If no data is available, an RTP packet
       with zero payload size is returned.
      */
    virtual PBoolean ReadData(
      RTP_DataFrame & frame,  ///<  Frame to extract from jitter buffer
      const PTimeInterval & tick = 0 ///< Real time tick for packet removal
    );

    /**Get current delay for jitter buffer.
      */
    DWORD GetCurrentJitterDelay() const { return m_currentJitterDelay; }
    
    /**Get minimum delay for jitter buffer.
      */
    DWORD GetMinJitterDelay() const { return m_minJitterDelay; }
    
    /**Get maximum delay for jitter buffer.
      */
    DWORD GetMaxJitterDelay() const { return m_maxJitterDelay; }

    /**Get time units.
      */
    unsigned GetTimeUnits() const { return m_timeUnits; }
    
    /**Get total number received packets too late to go into jitter buffer.
      */
    DWORD GetPacketsTooLate() const { return m_packetsTooLate; }

    /**Get total number received packets that overran the jitter buffer.
      */
    DWORD GetBufferOverruns() const { return m_bufferOverruns; }

    /**Get maximum consecutive marker bits before buffer starts to ignore them.
      */
    DWORD GetMaxConsecutiveMarkerBits() const { return m_maxConsecutiveMarkerBits; }

    /**Set maximum consecutive marker bits before buffer starts to ignore them.
      */
    void SetMaxConsecutiveMarkerBits(DWORD max) { m_maxConsecutiveMarkerBits = max; }
  //@}

  protected:
    DWORD CalculateRequiredTimestamp(DWORD playOutTimestamp) const;
    bool AdjustCurrentJitterDelay(int delta);

    unsigned m_timeUnits;
    PINDEX   m_packetSize;
    DWORD    m_minJitterDelay;      ///< Minimum jitter delay in timestamp units
    DWORD    m_maxJitterDelay;      ///< Maximum jitter delay in timestamp units
    int      m_jitterGrowTime;      ///< Amount to increase jitter delay by when get "late" packet
    DWORD    m_jitterShrinkPeriod;  ///< Period (in timestamp units) over which buffer is
                                    ///< consistently filled before shrinking
    int      m_jitterShrinkTime;    ///< Amount to shrink jitter delay by if consistently filled
    DWORD    m_silenceShrinkPeriod; ///< Reduce jitter delay is silent for this long
    int      m_silenceShrinkTime;   ///< Amount to shrink jitter delay by if consistently silent
    DWORD    m_jitterDriftPeriod;

    int      m_currentJitterDelay;
    DWORD    m_packetsTooLate;
    DWORD    m_bufferOverruns;
    DWORD    m_consecutiveMarkerBits;
    DWORD    m_maxConsecutiveMarkerBits;
    DWORD    m_consecutiveLatePackets;

    DWORD    m_frameTimeSum;
    unsigned m_frameTimeCount;
    DWORD    m_averageFrameTime;
    DWORD    m_lastTimestamp;
    DWORD    m_lastSyncSource;
    DWORD    m_bufferFilledTime;
    DWORD    m_bufferLowTime;
    DWORD    m_bufferEmptiedTime;
    int      m_timestampDelta;

    enum {
      e_SynchronisationStart,
      e_SynchronisationFill,
      e_SynchronisationShrink,
      e_SynchronisationDone
    } m_synchronisationState;

    typedef std::map<DWORD, RTP_DataFrame> FrameMap;
    FrameMap m_frames;
    PMutex   m_bufferMutex;

    RTP_JitterBufferAnalyser * m_analyser;
};


/**A descendant of the OpalJitterBuffer that starts a thread to read
   from something continuously and feed it into the jitter buffer.
  */
class OpalJitterBufferThread : public OpalJitterBuffer
{
    PCLASSINFO(OpalJitterBufferThread, OpalJitterBuffer);
 public:
    OpalJitterBufferThread(
      unsigned minJitterDelay, ///<  Minimum delay in RTP timestamp units
      unsigned maxJitterDelay, ///<  Maximum delay in RTP timestamp units
      unsigned timeUnits = 8,  ///<  Time units, usually 8 or 16
      PINDEX packetSize = 2048 ///<  Max RTP packet size
    );
    ~OpalJitterBufferThread();

    /// Start jiter buffer
    virtual void Start();

    /**Read a data frame from the jitter buffer.
       This function never blocks. If no data is available, an RTP packet
       with zero payload size is returned.

       Override of base class so can terminate caller when shutting down.
      */
    virtual PBoolean ReadData(
      RTP_DataFrame & frame,  ///<  Frame to extract from jitter buffer
      const PTimeInterval & tick = 0 ///< Real time tick for packet removal
    );

    /**This class instance collects data from the outside world in this
       method.

       @return true on successful read, false on faulty read. */
    virtual PBoolean OnReadPacket(
      RTP_DataFrame & frame   ///<  Frame read from the RTP session
    ) = 0;

  protected:
    PDECLARE_NOTIFIER(PThread, OpalJitterBufferThread, JitterThreadMain);

    /// Internal function to be called from derived class destructor
    void WaitForThreadTermination();

    PThread * m_jitterThread;
    bool      m_running;
};


#endif // OPAL_RTP_JITTER_H


/////////////////////////////////////////////////////////////////////////////
