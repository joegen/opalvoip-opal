/*
 * jitter.cxx
 *
 * Jitter buffer support
 *
 * Open H323 Library
 *
 * Copyright (c) 1998-2000 Equivalence Pty. Ltd.
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

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "jitter.h"
#endif

#include <opal_config.h>

#include <rtp/jitter.h>

#include <rtp/metrics.h>
#include <opal/manager.h>


#define PTraceModule() "Jitter"

const unsigned AverageFrameTimePackets = 4;
const unsigned MaxConsecutiveOverflows = 10;

#define ANALYSER_TRACE_LEVEL     5

#define COMMON_TRACE_INFO ": ts=" << requiredTimestamp << " (" << playOutTimestamp << "), dT=" << removalDelta << ", size=" << m_frames.size()
#define COMMON_TRACE_DELAY " delay=" << m_currentJitterDelay << " (" << (m_currentJitterDelay/m_timeUnits) << "ms)"
#define PTRACE_J(level, arg) PTRACE(std::min(sm_EveryPacketLogLevel,(unsigned)(level)), arg)

#if PTRACING
ostream & operator<<(ostream & strm, const OpalAudioJitterBuffer::AdjustResult adjusted)
{
  static const char * adjustments[] = {
    "unchanged from",
    "decreased to",
    "increased to",
    "reached minimum",
    "reached maximum"
  };

  return strm << adjustments[adjusted];
}
#endif // PTRACING


#if !PTRACING && !defined(NO_ANALYSER)
#define NO_ANALYSER 1
#endif

#ifdef NO_ANALYSER
  #define ANALYSE(inout, time, extra)
#else

  #define ANALYSE(inout, time, extra) \
    if (PTrace::CanTrace(ANALYSER_TRACE_LEVEL)) \
      m_analyser->inout(tick, time, m_frames.size(), extra)

  class OpalJitterBuffer::Analyser : public PObject
  {
      PCLASSINFO(Analyser, PObject);

    protected:
      struct Info {
        Info() : time(0), depth(0), extra("") { }
        RTP_Timestamp time;
        PTimeInterval tick;
        size_t        depth;
        const char *  extra;
      };
      std::vector<Info> in, out;
      std::vector<Info>::size_type inPos, outPos;

    public:
      Analyser()
        : inPos(0)
        , outPos(0)
      {
        std::vector<Info>::size_type size;
#if P_CONFIG_FILE
        PConfig env(PConfig::Environment);
        size = env.GetInteger("OPAL_JITTER_ANALYSER_SIZE", 1000);
#else
        size = 1000;
#endif
        if (size > 100000)
          size = 100000;
        in.resize(size);
        out.resize(size);
      }

      void Reset()
      {
        inPos = outPos = 0;
      }

      bool HasData() const { return inPos > 0 || outPos > 0; }

      void In(const PTimeInterval & tick, RTP_Timestamp time, size_t depth, const char * extra)
      {
        if (inPos == 0)
          in[inPos++].tick = tick;

        if (inPos < in.size()) {
          in[inPos].tick = tick;
          in[inPos].time = time;
          in[inPos].depth = depth;
          in[inPos++].extra = extra;
        }
      }

      void Out(const PTimeInterval & tick, RTP_Timestamp time, size_t depth, const char * extra)
      {
        if (inPos == 0) // Don't do anything until the first received packet. Removes a lot of noise.
          return;

        if (outPos == 0)
          out[outPos++].tick = tick;

        if (outPos < out.size()) {
          out[outPos].tick = tick;
          if (time == 0 && outPos > 0)
            out[outPos].time = out[outPos-1].time;
          else
            out[outPos].time = time;
          out[outPos].depth = depth;
          out[outPos++].extra = extra;
        }
      }

      void PrintOn(ostream & strm) const
      {
        if (inPos == 0 && outPos == 0)
          return;

        strm << "Input samples: " << inPos << " Output samples: " << outPos << "\n"
                "Dir\tRTPTime\tInDiff\tOutDiff\tInMode\tOutMode\t"
                "InDepth\tOuDepth\tInTick\tInDelta\tOutTick\tODelta\tIODelay\n";
        std::vector<Info>::size_type ix = 1;
        std::vector<Info>::size_type ox = 1;
        while (ix < inPos || ox < outPos) {
          while (ix < inPos && (ox >= outPos || in[ix].time < out[ox].time)) {
            strm << "In\t"
                 << in[ix].time << '\t'
                 << (int)(in[ix].time - in[ix-1].time) << "\t"
                    "\t"
                 << in[ix].extra << "\t"
                    "\t"
                 << in[ix].depth << "\t"
                    "\t"
                 << (in[ix].tick - in[0].tick) << '\t'
                 << (in[ix].tick - in[ix-1].tick) << "\t"
                    "\t"
                    "\t"
                    "\t"
                    "\n";
            ix++;
          }

          while (ox < outPos && (ix >= inPos || out[ox].time < in[ix].time)) {
            strm << "Out\t"
                 << out[ox].time << "\t"
                    "\t"
                 << (int)(out[ox].time - out[ox-1].time) << "\t"
                    "\t"
                 << out[ox].extra << "\t"
                    "\t"
                 << out[ox].depth << "\t"
                    "\t"
                    "\t"
                 << (out[ox].tick - out[0].tick) << '\t'
                 << (out[ox].tick - out[ox-1].tick) << "\t"
                    "\t"
                    "\n";
            ox++;
          }

          while (ix < inPos && ox < outPos && in[ix].time == out[ox].time) {
            strm << "I/O\t"
                 << in[ix].time << '\t'
                 << (int)(in[ix].time - in[ix-1].time) << '\t'
                 << (int)(out[ox].time - out[ox-1].time) << '\t'
                 << in[ix].extra << '\t'
                 << out[ox].extra << '\t'
                 << in[ix].depth << '\t'
                 << out[ox].depth << '\t'
                 << (in[ix].tick - in[0].tick) << '\t'
                 << (in[ix].tick - in[ix-1].tick) << '\t'
                 << (out[ox].tick - out[0].tick) << '\t'
                 << (out[ox].tick - out[ox-1].tick) << '\t'
                 << (out[ox].tick - in[ix].tick)
                 << '\n';
            ox++;
            ix++;
          }
        }
      }
  };

#endif

#define new PNEW


/////////////////////////////////////////////////////////////////////////////

OpalJitterBuffer::Init::Init(const OpalManager & manager, unsigned timeUnits)
  : Params(manager.GetJitterParameters())
  , m_timeUnits(timeUnits)
  , m_packetSize(manager.GetMaxRtpPacketSize())
{
}

OpalJitterBuffer::OpalJitterBuffer(const Init & init)
  : m_timeUnits(init.m_timeUnits)
  , m_packetSize(init.m_packetSize)
  , m_minJitterDelay(init.m_minJitterDelay*m_timeUnits)
  , m_maxJitterDelay(init.m_maxJitterDelay*m_timeUnits)
  , m_packetsTooLate(0)
  , m_bufferOverruns(0)
#ifdef NO_ANALYSER
  , m_analyser(NULL)
#else
  , m_analyser(new Analyser)
#endif
{
}


OpalJitterBuffer::~OpalJitterBuffer()
{
#ifndef NO_ANALYSER
  delete m_analyser;
#endif
}


OpalJitterBuffer * OpalJitterBuffer::Create(const OpalMediaType & mediaType, const Init & init)
{
  OpalJitterBuffer * jb = OpalJitterBufferFactory::CreateInstance(mediaType, init);
  if (jb == NULL)
    jb = new OpalNonJitterBuffer(init);
  return jb;
}


void OpalJitterBuffer::SetDelay(const Init & init)
{
  PAssert(m_timeUnits == init.m_timeUnits, PInvalidParameter);
  m_packetSize     = init.m_packetSize;
  m_minJitterDelay = init.m_minJitterDelay*m_timeUnits;
  m_maxJitterDelay = init.m_maxJitterDelay*m_timeUnits;
}


/////////////////////////////////////////////////////////////////////////////

PFACTORY_CREATE(OpalJitterBufferFactory, OpalAudioJitterBuffer, OpalMediaType::Audio());

#if PTRACING
unsigned OpalAudioJitterBuffer::sm_EveryPacketLogLevel = 6;
#endif

OpalAudioJitterBuffer::OpalAudioJitterBuffer(const Init & init)
  : OpalJitterBuffer(init)
  , m_jitterGrowTime(init.m_jitterGrowTime*m_timeUnits)
  , m_jitterShrinkPeriod(init.m_jitterShrinkPeriod*m_timeUnits)
  , m_jitterShrinkTime(-(int)init.m_jitterShrinkTime*m_timeUnits)
  , m_silenceShrinkPeriod(init.m_silenceShrinkPeriod*m_timeUnits)
  , m_silenceShrinkTime(-(int)init.m_silenceShrinkTime*m_timeUnits)
  , m_jitterDriftPeriod(init.m_jitterDriftPeriod*m_timeUnits)
  , m_overrunFactor(init.m_overrunFactor)
  , m_closed(false)
  , m_currentJitterDelay(init.m_minJitterDelay*m_timeUnits)
  , m_consecutiveMarkerBits(0)
  , m_maxConsecutiveMarkerBits(10)
  , m_consecutiveLatePackets(0)
  , m_consecutiveOverflows(0)
  , m_consecutiveEmpty(0)
  , m_lastSyncSource(0)
#if PTRACING
  , m_lastRemoveTick(PTimer::Tick())
#endif
{
  InternalReset();
  PTRACE_J(4, "Audio buffer created:" << *this);
}


OpalAudioJitterBuffer::~OpalAudioJitterBuffer()
{
  PTRACE_J(4, "Audio buffer destroyed:" << *this);
}


void OpalAudioJitterBuffer::Close()
{
  m_closed = true;
  m_frameCount.Signal();
#ifndef NO_ANALYSER
  PTRACE_IF(ANALYSER_TRACE_LEVEL, m_analyser->HasData(), "Buffer analysis: " << *this << '\n' << *m_analyser);
  m_analyser->Reset();
#endif
}


void OpalAudioJitterBuffer::Restart()
{
  m_bufferMutex.Wait();

  InternalReset();
  m_closed = false;

  m_bufferMutex.Signal();
}


void OpalAudioJitterBuffer::PrintOn(ostream & strm) const
{
  strm << "this=" << (void *)this
       << " packets=" << m_frames.size()
       <<   " rate=" << m_timeUnits << "kHz"
       <<  " delay=" << (m_minJitterDelay/m_timeUnits) << '-'
                     << (m_currentJitterDelay/m_timeUnits) << '-'
                     << (m_maxJitterDelay/m_timeUnits) << "ms"
           " frame=" << (m_packetTime/m_timeUnits) << "ms"
            " grow=" << (m_jitterGrowTime/m_timeUnits) << "ms"
          " shrink=" << (-m_jitterShrinkTime/m_timeUnits) << "ms"
                " (" << (m_jitterShrinkPeriod/m_timeUnits) << "ms)";
}


void OpalAudioJitterBuffer::SetDelay(const Init & init)
{
  m_bufferMutex.Wait();

  OpalJitterBuffer::SetDelay(init);

  m_currentJitterDelay = init.m_currentJitterDelay*m_timeUnits;
  if (m_currentJitterDelay < (int)m_minJitterDelay)
    m_currentJitterDelay = m_minJitterDelay;
  else if (m_currentJitterDelay > (int)m_maxJitterDelay)
    m_currentJitterDelay = m_maxJitterDelay;

  m_jitterGrowTime = init.m_jitterGrowTime*m_timeUnits;
  m_jitterShrinkPeriod = init.m_jitterShrinkPeriod*m_timeUnits;
  m_jitterShrinkTime = -(int)init.m_jitterShrinkTime*m_timeUnits;
  m_silenceShrinkPeriod = init.m_silenceShrinkPeriod*m_timeUnits;
  m_silenceShrinkTime = -(int)init.m_silenceShrinkTime*m_timeUnits;
  m_jitterDriftPeriod = init.m_jitterDriftPeriod*m_timeUnits;
  m_overrunFactor = init.m_overrunFactor;

  PTRACE_J(3, "Delays set to " << *this);

  InternalReset();

  m_bufferMutex.Signal();
}


void OpalAudioJitterBuffer::InternalReset()
{
  m_frameTimeCount    = 0;
  m_frameTimeSum      = 0;
  m_packetTime = 0;
  m_lastSequenceNum   = USHRT_MAX;
  m_lastTimestamp     = UINT_MAX;
  m_lastBufferSize    = 0;
  m_bufferStaticTime  = 0;
  m_bufferLowTime     = 0;
  m_bufferEmptiedTime = 0;

  m_consecutiveLatePackets = 0;
  m_consecutiveOverflows   = 0;
  m_consecutiveEmpty       = 0;

  m_synchronisationState = e_SynchronisationStart;

  m_frames.clear();
}


RTP_Timestamp OpalAudioJitterBuffer::GetCurrentJitterDelay() const
{
  PWaitAndSignal mutex(m_bufferMutex);
  return m_currentJitterDelay;
}


RTP_Timestamp OpalAudioJitterBuffer::GetPacketTime() const
{
  PWaitAndSignal mutex(m_bufferMutex);
  return m_packetTime;
}


PBoolean OpalAudioJitterBuffer::WriteData(const RTP_DataFrame & frame, const PTimeInterval & tick)
{
  if (m_closed)
    return false;

  if (frame.GetSize() < RTP_DataFrame::MinHeaderSize) {
    PTRACE_J(2, "Writing invalid RTP data frame.");
    return true; // Don't abort, but ignore
  }

  PWaitAndSignal mutex(m_bufferMutex);

  RTP_Timestamp timestamp = frame.GetTimestamp();
  RTP_SequenceNumber currentSequenceNum = frame.GetSequenceNumber();
  RTP_SyncSourceId newSyncSource = frame.GetSyncSource();

  // Avoid issues with constant delay offset caused by initial in rush of packets
  if (m_lastSyncSource == 0 && (m_lastInsertTick == 0 || (tick - m_lastInsertTick) < 10)) {
    PTRACE_J(4, "Flushing initial audio packet:"
                " SSRC=" << RTP_TRACE_SRC(newSyncSource) <<
                " sn=" << currentSequenceNum <<
                " ts=" << timestamp);
    m_lastInsertTick = tick;
    return true;
  }

  // Check for remote switching media senders, they shouldn't do this but do anyway
  if (newSyncSource != m_lastSyncSource) {
    PTRACE_IF(2, m_lastSyncSource != 0, "Buffer reset due to SSRC change from "
              << RTP_TRACE_SRC(m_lastSyncSource) << " to " << RTP_TRACE_SRC(newSyncSource)
              << " at sn=" << currentSequenceNum);
    InternalReset();
    m_packetsTooLate = m_bufferOverruns = 0; // Reset these stats for new SSRC
    m_lastSyncSource = newSyncSource;
  }


  /*Deal with naughty systems that send continuous marker bits, thus we
    cannot use it to determine start of talk burst, and the need to refill
    the jitter buffer */
  if (m_consecutiveMarkerBits < m_maxConsecutiveMarkerBits) {
    if (frame.GetMarker()) {
      m_consecutiveMarkerBits++;
      InternalReset();

      // Have been told there is explicit silence by marker, take opportunity
      // to reduce the current jitter delay.
      PTRACE_PARAM(AdjustResult adjusted =) AdjustCurrentJitterDelay(m_silenceShrinkTime);
      PTRACE_J(adjusted == e_ReachedMinimum ? 2 : 3,
               "Start talk burst: ts=" << timestamp << ", " << adjusted << COMMON_TRACE_DELAY);
    }
    else
      m_consecutiveMarkerBits = 0;
  }
  else {
    if (m_consecutiveMarkerBits == m_maxConsecutiveMarkerBits) {
      PTRACE_J(2, "Every packet has Marker bit, ignoring them from this client!");
      m_consecutiveMarkerBits++;
    }
  }


  /*Calculate the time between packets. While not actually dictated by any
    standards, this is invariably a constant. We just need to allow for if we
    are unlucky at the start and get a missing packet, in which case we are
    twice as big (or more) as we should be. Se we make sure we do not have
    a missing packet by inspecting sequence numbers. */
  if (m_lastSequenceNum != USHRT_MAX) {
    if (timestamp < m_lastTimestamp) {
      PTRACE_J(3, "Timestamps abruptly changed from " << m_lastTimestamp << " to " << timestamp << ", resynching");
      InternalReset();
    }
    else if (m_lastSequenceNum+1 == currentSequenceNum) {
      RTP_Timestamp delta = timestamp - m_lastTimestamp;
      PTRACE_IF(std::min(sm_EveryPacketLogLevel,5U), m_packetTime == 0, "Wait frame time :"
                     " ts=" << timestamp << ","
                  " delta=" << delta << " (" << (delta/m_timeUnits) << "ms),"
                     " sn=" << currentSequenceNum);

      /* Average the deltas. Most systems are "normalised" and the timestamp
          goes up by a constant on consecutive packets. Not not all. */
      m_frameTimeSum += delta;
      if (++m_frameTimeCount > AverageFrameTimePackets) {
        int newFrameTime = (int)(m_frameTimeSum/m_frameTimeCount);
        m_frameTimeSum = 0;
        m_frameTimeCount = 0;

        // If new average changed by more than millisecond, start using it.
        if (std::abs(newFrameTime - (int)m_packetTime) >= (int)m_timeUnits) {
          m_packetTime = newFrameTime;
          PTRACE_PARAM(AdjustResult adjusted =) AdjustCurrentJitterDelay(0);
          PTRACE_J(4, "Frame time set  : "
                      "ts=" << timestamp << ", "
                      "size=" << m_frames.size() << ", "
                      "time=" << newFrameTime << " (" << (newFrameTime/m_timeUnits) << "ms), " <<
                      adjusted << COMMON_TRACE_DELAY);
        }
      }
    }
    else {
      PTRACE_J(4, "Lost packet(s), resetting frame time average, sn=" << currentSequenceNum);
      m_frameTimeSum = 0;
      m_frameTimeCount = 0;
    }
  }
  m_lastSequenceNum = currentSequenceNum;
  m_lastTimestamp = timestamp;


  /* Fail safe for infinite queueing, for example, if other thread is not
     taking stuff out.  Also checks for abrupt changes in timestamp values, can
     happen when remote is swapping media sources */
  FrameMap::iterator oldestFrame = m_frames.begin();
  if (oldestFrame != m_frames.end()) {
    RTP_Timestamp delta = timestamp - oldestFrame->second.GetTimestamp();
    if (delta < (m_maxJitterDelay > 0 ? (m_maxJitterDelay*2) : (m_timeUnits*1000)))
      m_consecutiveOverflows = 0;
    else {
      ANALYSE(In, timestamp, "Overflow");
      PTRACE_J(4, "Buffer overflow : ts=" << timestamp << ", delta=" << delta << ", size=" << m_frames.size());
      if (++m_consecutiveOverflows > (m_packetTime == 0 ? AverageFrameTimePackets : MaxConsecutiveOverflows)) {
        PTRACE_J(2, "Consecutive overflow packets, resynching");
        InternalReset();
      }
      return true;
    }
  }


  // Add to buffer
  pair<FrameMap::iterator,bool> result = m_frames.insert(FrameMap::value_type(timestamp, frame));
  if (result.second) {
    ANALYSE(In, timestamp, m_synchronisationState != e_SynchronisationDone ? "PreBuf" : "");
    PTRACE_IF(sm_EveryPacketLogLevel, m_maxJitterDelay > 0, "Inserted packet :"
           " ts=" << timestamp << ","
           " dT=" << (tick - m_lastInsertTick) << ","
           " payload=" << frame.GetPayloadSize() << ","
           " size=" << m_frames.size());
    m_lastInsertTick = tick;
    m_frameCount.Signal();
  }
  else {
    PTRACE_J(2, "Attempt to insert two RTP packets with same timestamp: " << timestamp);
  }

  return true;
}


RTP_Timestamp OpalAudioJitterBuffer::CalculateRequiredTimestamp(RTP_Timestamp playOutTimestamp) const
{
  RTP_Timestamp timestamp = playOutTimestamp + m_timestampDelta;
  return timestamp > (RTP_Timestamp)m_currentJitterDelay ? (timestamp - m_currentJitterDelay) : 0;
}


OpalAudioJitterBuffer::AdjustResult OpalAudioJitterBuffer::AdjustCurrentJitterDelay(int delta)
{
  int minJitterDelay = max(m_minJitterDelay, 2*m_packetTime);
  int maxJitterDelay = max(m_minJitterDelay, m_maxJitterDelay);

  if (delta < 0 && m_currentJitterDelay <= minJitterDelay)
    return e_Unchanged;
  if (delta > 0 && m_currentJitterDelay >= maxJitterDelay)
    return e_Unchanged;

  m_currentJitterDelay += delta;
  if (m_currentJitterDelay < minJitterDelay) {
    m_currentJitterDelay = minJitterDelay;
    return e_ReachedMinimum;
  }

  if (m_currentJitterDelay > maxJitterDelay) {
    m_currentJitterDelay = maxJitterDelay;
    return e_ReachedMaximum;
  }

  return delta < 0 ? e_Decreased : e_Increased;
}


PBoolean OpalAudioJitterBuffer::ReadData(RTP_DataFrame & frame, const PTimeInterval & PTRACE_PARAM(, const PTimeInterval & tick))
{
  // Default response is an empty frame, ie silence with possible comfort noise
  frame.SetPayloadType(RTP_DataFrame::CN);
  frame.SetPayloadSize(0);

  if (m_maxJitterDelay == 0) {
    m_currentJitterDelay = 0;
    m_frameCount.Wait(); // Go synchronous
    PWaitAndSignal mutex(m_bufferMutex);
    if (m_frames.empty()) {
        // Must have been reset, clear the semaphore.
        while (m_frameCount.Wait(0))
            ;
    }
    else {
      FrameMap::iterator oldestFrame = m_frames.begin();
      frame = oldestFrame->second;
      m_frames.erase(oldestFrame);
    }
    return !m_closed;
  }

  PWaitAndSignal mutex(m_bufferMutex);

  if (m_closed)
    return false;

#if PTRACING
  PTimeInterval removalDelta;
  if (tick == PMaxTimeInterval) {
    PTimeInterval now = PTimer::Tick();
    removalDelta = now - m_lastRemoveTick;
    m_lastRemoveTick = now;
  }
  else {
    removalDelta = tick - m_lastRemoveTick;
    m_lastRemoveTick = tick;
  }
#endif

  // Now we get the timestamp the caller wants
  const RTP_Timestamp playOutTimestamp = frame.GetTimestamp();
  RTP_Timestamp requiredTimestamp = CalculateRequiredTimestamp(playOutTimestamp);

  if (m_frames.empty()) {
    /*We ran the buffer down to empty, so have no data to play, play silence.
      This happens if packet is too late or completely missing. A too late
      packet will be picked up by later code.
     */
    ANALYSE(Out, requiredTimestamp, "Empty");

    if ((playOutTimestamp - m_bufferEmptiedTime) > m_silenceShrinkPeriod) {
      AdjustResult adjusted = AdjustCurrentJitterDelay(m_silenceShrinkTime);
      if (adjusted != e_Unchanged) {
        PTRACE_J(adjusted == e_ReachedMinimum ? 2 : 4,
                 "Long silence    " COMMON_TRACE_INFO << ", " << adjusted << COMMON_TRACE_DELAY);
        m_bufferEmptiedTime = playOutTimestamp;
      }
    }

    ++m_consecutiveEmpty;
    PTRACE_IF(2, m_consecutiveEmpty == 100, "Always empty    " COMMON_TRACE_INFO);
    PTRACE_J(m_consecutiveEmpty < 100 && m_synchronisationState == e_SynchronisationDone ? 4U : 100U,
           "Buffer is empty " COMMON_TRACE_INFO);
    return true;
  }
  m_consecutiveEmpty  = 0;
  m_bufferEmptiedTime = playOutTimestamp;

  size_t maxFramesInBuffer;
  if (m_packetTime == 0) {
    m_synchronisationState = e_SynchronisationStart; // Can't start until we have an average packet time
    maxFramesInBuffer = 1000000;                     // Disable clock overrun check later in code as well
  }
  else {
    maxFramesInBuffer = m_currentJitterDelay/m_packetTime;
    if (maxFramesInBuffer < 2)
      maxFramesInBuffer = 2;

    int currentFramesInBuffer = m_frames.size(); // Must be signed int for later abs()

    /* Check for buffer low (one packet) for prologed period, then that generally
       means we have a sample clock drift problem, that is we have a clock of 8.01kHz and
       the remote has 7.99kHz so gradually the buffer drains as we take things out faster
       than they arrive. */
    if (m_bufferLowTime == 0 || currentFramesInBuffer > 1)
      m_bufferLowTime = playOutTimestamp;
    else if ((playOutTimestamp - m_bufferLowTime) > m_jitterDriftPeriod) {
      m_bufferLowTime = playOutTimestamp;
      PTRACE_J(4, "Clock underrun  " COMMON_TRACE_INFO);
      m_timestampDelta -= m_packetTime;
      ANALYSE(Out, requiredTimestamp, "Drift");
      return true;
    }

    /* Check for buffer has been consistently the same size. If so for a while
       then it is time to reduce the size of the jitter buffer as packets are
       arriving with little jitter. */
    if (m_bufferStaticTime == 0 || std::abs(currentFramesInBuffer - m_lastBufferSize) > 1)
      m_bufferStaticTime = playOutTimestamp;
    else if ((playOutTimestamp - m_bufferStaticTime) > m_jitterShrinkPeriod) {
      m_bufferStaticTime = playOutTimestamp;

      AdjustResult adjusted = AdjustCurrentJitterDelay(m_jitterShrinkTime);
      PTRACE_J(adjusted == e_ReachedMinimum ? 2 : 4,
               "Packets on time " COMMON_TRACE_INFO << ", " << adjusted << COMMON_TRACE_DELAY);
      if (adjusted != e_Unchanged && currentFramesInBuffer > 1)
        m_synchronisationState = e_SynchronisationShrink;
    }
    m_lastBufferSize = currentFramesInBuffer;
  }

  // Get the oldest packet
  FrameMap::iterator oldestFrame = m_frames.begin();
  PAssert(oldestFrame != m_frames.end(), PLogicError);

  // Check current buffer state and act accordingly
  switch (m_synchronisationState) {
    case e_SynchronisationStart :
      /* First packet of talk burst, re-calculate the timestamp delta */
      m_timestampDelta = oldestFrame->first - playOutTimestamp;
      requiredTimestamp = CalculateRequiredTimestamp(playOutTimestamp);
      m_synchronisationState = e_SynchronisationFill;
      PTRACE_J(5, "Synchronising   " COMMON_TRACE_INFO << ", oldest=" << oldestFrame->first);
      ANALYSE(Out, oldestFrame->first, "PreBuf");
      return true;

    case e_SynchronisationFill :
      /* Now see if we have buffered enough yet */
      if (requiredTimestamp < oldestFrame->first) {
        PTRACE(sm_EveryPacketLogLevel, "Pre-buffering   " COMMON_TRACE_INFO << ", oldest=" << oldestFrame->first);
        /* Nope, play out some silence */
        ANALYSE(Out, oldestFrame->first, "PreBuf");
        return true;
      }

      /* Buffer now full, return the oldest frame. From now on the caller will
         call ReadData() with the next timestamp it wants which will be the
         timstamp of this oldest packet, plus the amount of audio data that
         is in it. */
      m_synchronisationState = e_SynchronisationDone;
      PTRACE_J(4, "Synchronise done" COMMON_TRACE_INFO << "," COMMON_TRACE_DELAY);
      break;

    case e_SynchronisationDone :
      // Get rid of all the frames that are too late
      while (requiredTimestamp >= oldestFrame->first + m_packetTime) {
        if (++m_consecutiveLatePackets > 10) {
          PTRACE_J(3, "Too many late   " COMMON_TRACE_INFO);
          InternalReset();
          return true;
        }

        // Packets late, need a bigger jitter buffer
        PTRACE_PARAM(AdjustResult adjusted =) AdjustCurrentJitterDelay(m_jitterGrowTime);
        PTRACE_J(adjusted == e_ReachedMaximum ? 2 : 3,
                 "Packet too late " COMMON_TRACE_INFO << ", "
                 "oldest=" << oldestFrame->first << ", " <<
                 adjusted << COMMON_TRACE_DELAY);
        ANALYSE(Out, oldestFrame->first, "Late");
        m_bufferStaticTime = playOutTimestamp;
        m_frames.erase(oldestFrame);
        ++m_packetsTooLate;

        if (m_frames.empty()) {
          PTRACE(sm_EveryPacketLogLevel, "Buffer emptied  " COMMON_TRACE_INFO);
          ANALYSE(Out, requiredTimestamp, "Emptied");
          return true;
        }

        requiredTimestamp = CalculateRequiredTimestamp(playOutTimestamp);

        oldestFrame = m_frames.begin();
        PAssert(oldestFrame != m_frames.end(), PLogicError);
      }

      /* Check for buffer overfull due to clock mismatch. It is possible for the remote
         to have a clock of 8.01kHz and the receiver 7.99kHz so gradually the remote
         sends more data than we take out over time, gradually building up in the
         jitter buffer. So, drop a frame every now and then. */
      if (m_frames.size() <= maxFramesInBuffer*m_overrunFactor)
        break;

      PTRACE(m_overrunFactor < 10 ? std::min(sm_EveryPacketLogLevel,4U) : 2,
             "Clock overrun   " COMMON_TRACE_INFO << " greater than "
             << maxFramesInBuffer*m_overrunFactor << " (" << maxFramesInBuffer << '*' << m_overrunFactor << ')');
      m_timestampDelta += m_packetTime;
      // Do next case

    case e_SynchronisationShrink :
      m_synchronisationState = e_SynchronisationDone;
      requiredTimestamp = CalculateRequiredTimestamp(playOutTimestamp);
      while (requiredTimestamp >= oldestFrame->first + m_packetTime) {
        ANALYSE(Out, oldestFrame->first, "Shrink");
        PTRACE(sm_EveryPacketLogLevel, "Dropping packet " COMMON_TRACE_INFO << ", actual-ts=" << oldestFrame->first);
        m_frames.erase(oldestFrame);
        ++m_bufferOverruns;

        if (m_frames.empty()) {
          PTRACE(sm_EveryPacketLogLevel, "Buffer emptied  " COMMON_TRACE_INFO);
          ANALYSE(Out, requiredTimestamp, "Emptied");
          return true;
        }

        oldestFrame = m_frames.begin();
      }
      break;
  }

  /* Now we see if it time for our oldest packet, or that there is a missing
     packet (not arrived yet) in buffer. Can't wait for it, return no data.
     If the packet subsequently DOES arrive, it will get picked up by the
     too late section above. */
  if (requiredTimestamp < oldestFrame->first) {
    if (oldestFrame->first - requiredTimestamp > m_timeUnits*1000) {
      PTRACE_J(3, "Too far in ahead" COMMON_TRACE_INFO);
      InternalReset();
    }
    else {
      PTRACE(sm_EveryPacketLogLevel, "Packet not ready" COMMON_TRACE_INFO << ", oldest=" << oldestFrame->first);
      ANALYSE(Out, requiredTimestamp, "Wait");
    }
    return true;
  }

  // Finally can return the frame we have
  ANALYSE(Out, oldestFrame->first, "");
  frame = oldestFrame->second;
  PTRACE(sm_EveryPacketLogLevel, "Delivered packet" COMMON_TRACE_INFO
         << ", payload=" << frame.GetPayloadSize() << ", actual-ts=" << frame.GetTimestamp());
  m_frames.erase(oldestFrame);
  frame.SetTimestamp(playOutTimestamp);
  m_consecutiveLatePackets = 0;
  return true;
}


/////////////////////////////////////////////////////////////////////////////

OpalNonJitterBuffer::OpalNonJitterBuffer(const Init & init)
  : OpalJitterBuffer(init)
{
}


void OpalNonJitterBuffer::Close()
{
  m_queue.Close(true);
}


void OpalNonJitterBuffer::Restart()
{
  m_queue.Restart();
}


bool OpalNonJitterBuffer::WriteData(const RTP_DataFrame & frame, const PTimeInterval &)
{
  return m_queue.Enqueue(frame);
}


bool OpalNonJitterBuffer::ReadData(RTP_DataFrame & frame, const PTimeInterval & timeout PTRACE_PARAM(, const PTimeInterval &))
{
  if (!m_queue.Dequeue(frame, timeout))
      frame.SetPayloadSize(0);
  return m_queue.IsOpen();
}


/////////////////////////////////////////////////////////////////////////////
