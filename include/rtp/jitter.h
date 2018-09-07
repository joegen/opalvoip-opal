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
 */

#ifndef OPAL_RTP_JITTER_H
#define OPAL_RTP_JITTER_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <opal_config.h>

#include <opal/mediatype.h>
#include <rtp/rtp.h>


class OpalManager;


///////////////////////////////////////////////////////////////////////////////

/**This is an Abstract jitter buffer. 
  */
class OpalJitterBuffer : public PObject
{
    PCLASSINFO(OpalJitterBuffer, PObject);
  public:
    struct Params
    {
      unsigned m_minJitterDelay;      ///< Minimum delay in milliseconds
      unsigned m_maxJitterDelay;      ///< Maximum delay in milliseconds
      unsigned m_currentJitterDelay;  ///< Current/initial delay in milliseconds
      unsigned m_jitterGrowTime;      ///< Amount to increase jitter delay by when get "late" packet
      unsigned m_jitterShrinkPeriod;  ///< Deadband of low jitter before shrink delay
      unsigned m_jitterShrinkTime;    ///< Amount to reduce buffer delay
      unsigned m_silenceShrinkPeriod; ///< Reduce jitter delay is silent for this long
      unsigned m_silenceShrinkTime;   ///< Amount to shrink jitter delay by if consistently silent
      unsigned m_jitterDriftPeriod;   ///< Time over which repeated undeflows cause packet to be dropped
      unsigned m_overrunFactor;       ///< Multiplier on JB length (in packets) before throwing away packets

      Params(
        unsigned minJitterDelay = 40,
        unsigned maxJitterDelay = 250
      )
        : m_minJitterDelay(minJitterDelay)
        , m_maxJitterDelay(maxJitterDelay)
        , m_currentJitterDelay(m_minJitterDelay)
        , m_jitterGrowTime(10)
        , m_jitterShrinkPeriod(1000)
        , m_jitterShrinkTime(5)
        , m_silenceShrinkPeriod(5000)
        , m_silenceShrinkTime(20)
        , m_jitterDriftPeriod(500)
        , m_overrunFactor(2)
      { }
    };

    /// Initialisation information
    struct Init : Params
    {
      Init(
        const OpalManager & manager,
        unsigned timeUnits
      );
      Init(
        const OpalMediaType & mediaType,
        unsigned minJitterDelay,
        unsigned maxJitterDelay,
        unsigned timeUnits = 8,
        PINDEX packetSize = 2048
        ) : Params(minJitterDelay, maxJitterDelay)
          , m_mediaType(mediaType)
          , m_timeUnits(timeUnits)
          , m_packetSize(packetSize)
      { }

      OpalMediaType m_mediaType;
      unsigned      m_timeUnits;           ///< Time units, usually 8 or 16
      PINDEX        m_packetSize;          ///< Max RTP packet size
    };

    /**@name Construction */
    //@{
    /**Constructor for this jitter buffer. The size of this buffer can be
       altered later with the SetDelay method
       */
    OpalJitterBuffer(
      const Init & init  ///< Initialisation information
    );

    /** Destructor, which closes this down and deletes the internal list of frames
      */
    virtual ~OpalJitterBuffer();

    // Create an appropriate jitter buffer for the media type
    static OpalJitterBuffer * Create(
      const OpalMediaType & mediaType,
      const Init & init  ///< Initialisation information
    );
    //@}

  /**@name Operations */
  //@{
    /**Set the maximum delay the jitter buffer will operate to.
      */
    virtual void SetDelay(
      const Init & init  ///< Initialisation information
    );

    /**Close jitter buffer.
      */
    virtual void Close() = 0;

    /**Restart jitter buffer.
      */
    virtual void Restart() = 0;

    /**Write data frame from the RTP channel.
      */
    virtual bool WriteData(
      const RTP_DataFrame & frame,        ///< Frame to feed into jitter buffer
      const PTimeInterval & tick = PTimer::Tick() ///< Real time tick for packet arrival
    ) = 0;

    /**Read a data frame from the jitter buffer.
       This function never blocks. If no data is available, an RTP packet
       with zero payload size is returned.
      */
    virtual bool ReadData(
      RTP_DataFrame & frame,              ///<  Frame to extract from jitter buffer
      const PTimeInterval & timeout = PMaxTimeInterval  ///< Time out for read
      PTRACE_PARAM(, const PTimeInterval & tick = PMaxTimeInterval)
    ) = 0;

    /**Get current delay for jitter buffer.
      */
    virtual RTP_Timestamp GetCurrentJitterDelay() const { return 0; }

    /**Get average packet time for incoming data.
      */
    virtual RTP_Timestamp GetPacketTime() const { return 0; }

    /**Get time units.
      */
    unsigned GetTimeUnits() const { return m_timeUnits; }
    
    /**Get minimum delay for jitter buffer.
      */
    RTP_Timestamp GetMinJitterDelay() const { return m_minJitterDelay; }
    
    /**Get maximum delay for jitter buffer.
      */
    RTP_Timestamp GetMaxJitterDelay() const { return m_maxJitterDelay; }

    /**Get total number received packets too late to go into jitter buffer.
      */
    unsigned GetPacketsTooLate() const { return m_packetsTooLate; }

    /**Get total number received packets that overran the jitter buffer.
      */
    unsigned GetBufferOverruns() const { return m_bufferOverruns; }
  //@}

  protected:
    unsigned      m_timeUnits;
    PINDEX        m_packetSize;
    RTP_Timestamp m_minJitterDelay;      ///< Minimum jitter delay in timestamp units
    RTP_Timestamp m_maxJitterDelay;      ///< Maximum jitter delay in timestamp units
    unsigned      m_packetsTooLate;
    unsigned      m_bufferOverruns;

    class Analyser;
    Analyser * m_analyser;
};


typedef PParamFactory<OpalJitterBuffer, OpalJitterBuffer::Init, OpalMediaType> OpalJitterBufferFactory;


/**This is an Audio jitter buffer.
  */
class OpalAudioJitterBuffer : public OpalJitterBuffer
{
  PCLASSINFO(OpalAudioJitterBuffer, OpalJitterBuffer);

  public:
  /**@name Construction */
  //@{
    /**Constructor for this jitter buffer. The size of this buffer can be
       altered later with the SetDelay method
      */
    OpalAudioJitterBuffer(
      const Init & init  ///< Initialisation information
    );
    
    /** Destructor, which closes this down and deletes the internal list of frames
      */
    virtual ~OpalAudioJitterBuffer();
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
    /**Set the maximum delay the jitter buffer will operate to.
      */
    virtual void SetDelay(
      const Init & init  ///< Initialisation information
    );

    /**Reset jitter buffer.
      */
    virtual void Close();

    /**Restart jitter buffer.
      */
    virtual void Restart();

    /**Write data frame from the RTP channel.
      */
    virtual bool WriteData(
      const RTP_DataFrame & frame,        ///< Frame to feed into jitter buffer
      const PTimeInterval & tick = PTimer::Tick() ///< Real time tick for packet arrival
    );

    /**Read a data frame from the jitter buffer.
       This function never blocks. If no data is available, an RTP packet
       with zero payload size is returned.
      */
    virtual bool ReadData(
      RTP_DataFrame & frame,              ///<  Frame to extract from jitter buffer
      const PTimeInterval & timeout = PMaxTimeInterval  ///< Time out for read
      PTRACE_PARAM(, const PTimeInterval & tick = PMaxTimeInterval)
    );

    /**Get current delay for jitter buffer.
      */
    virtual RTP_Timestamp GetCurrentJitterDelay() const;

    /**Get average packet time for incoming data.
      */
    virtual RTP_Timestamp GetPacketTime() const;

    /**Get maximum consecutive marker bits before buffer starts to ignore them.
      */
    unsigned GetMaxConsecutiveMarkerBits() const { return m_maxConsecutiveMarkerBits; }

    /**Set maximum consecutive marker bits before buffer starts to ignore them.
      */
    void SetMaxConsecutiveMarkerBits(unsigned max) { m_maxConsecutiveMarkerBits = max; }
  //@}

  protected:
    void InternalReset();
    RTP_Timestamp CalculateRequiredTimestamp(RTP_Timestamp playOutTimestamp) const;
    enum AdjustResult {
      e_Unchanged,
      e_Decreased,
      e_Increased,
      e_ReachedMinimum,
      e_ReachedMaximum
    };
    friend ostream & operator<<(ostream & strm, const AdjustResult adjusted);
    AdjustResult AdjustCurrentJitterDelay(int delta);

    int           m_jitterGrowTime;      ///< Amount to increase jitter delay by when get "late" packet
    RTP_Timestamp m_jitterShrinkPeriod;  ///< Period (in timestamp units) over which buffer is
                                    ///< consistently filled before shrinking
    int           m_jitterShrinkTime;    ///< Amount to shrink jitter delay by if consistently filled
    RTP_Timestamp m_silenceShrinkPeriod; ///< Reduce jitter delay is silent for this long
    int           m_silenceShrinkTime;   ///< Amount to shrink jitter delay by if consistently silent
    RTP_Timestamp m_jitterDriftPeriod;
    unsigned      m_overrunFactor;

    bool     m_closed;
    int      m_currentJitterDelay;
    unsigned m_consecutiveMarkerBits;
    unsigned m_maxConsecutiveMarkerBits;
    unsigned m_consecutiveLatePackets;
    unsigned m_consecutiveOverflows;
    unsigned m_consecutiveEmpty;

    unsigned           m_frameTimeCount;
    uint64_t           m_frameTimeSum;
    RTP_Timestamp      m_packetTime;
    RTP_SequenceNumber m_lastSequenceNum;
    RTP_Timestamp      m_lastTimestamp;
    RTP_SyncSourceId   m_lastSyncSource;
    int                m_lastBufferSize;
    RTP_Timestamp      m_bufferStaticTime;
    RTP_Timestamp      m_bufferLowTime;
    RTP_Timestamp      m_bufferEmptiedTime;
    int                m_timestampDelta;

    enum {
      e_SynchronisationStart,
      e_SynchronisationFill,
      e_SynchronisationShrink,
      e_SynchronisationDone
    } m_synchronisationState;

    typedef std::map<RTP_Timestamp, RTP_DataFrame> FrameMap;
    FrameMap   m_frames;
    PDECLARE_MUTEX(m_bufferMutex);
    PSemaphore m_frameCount;

    PTimeInterval m_lastInsertTick;
#if PTRACING
    PTimeInterval m_lastRemoveTick;
  public:
    static unsigned sm_EveryPacketLogLevel;
#endif
};


/// Null jitter buffer, just a simpple queue
class OpalNonJitterBuffer : public OpalJitterBuffer
{
    PCLASSINFO(OpalNonJitterBuffer, OpalJitterBuffer);
  public:
  /**@name Construction */
  //@{
    /**Constructor for this jitter buffer. The size of this buffer can be
       altered later with the SetDelay method
      */
    OpalNonJitterBuffer(
      const Init & init  ///< Initialisation information
    );
  //@}

  /**@name Operations */
  //@{
    /**Reset jitter buffer.
      */
    virtual void Close();

    /**Restart jitter buffer.
      */
    virtual void Restart();

    /**Write data frame from the RTP channel.
      */
    virtual bool WriteData(
      const RTP_DataFrame & frame,        ///< Frame to feed into jitter buffer
      const PTimeInterval & tick = PTimer::Tick() ///< Real time tick for packet arrival
    );

    /**Read a data frame from the jitter buffer.
       This function never blocks. If no data is available, an RTP packet
       with zero payload size is returned.
      */
    virtual bool ReadData(
      RTP_DataFrame & frame,              ///<  Frame to extract from jitter buffer
      const PTimeInterval & timeout = PMaxTimeInterval  ///< Time out for read
      PTRACE_PARAM(, const PTimeInterval & tick = PMaxTimeInterval)
    );
  //@}

  protected:
    PSyncQueue<RTP_DataFrame> m_queue;
};


#endif // OPAL_RTP_JITTER_H


/////////////////////////////////////////////////////////////////////////////
