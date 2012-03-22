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
 * $Revision$
 * $Author$
 * $Date$
 */

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "jitter.h"
#endif

#include <opal/buildopts.h>

#include <rtp/jitter.h>


const unsigned JitterRoundingGuardBits = 4;


#if !PTRACING && !defined(NO_ANALYSER)
#define NO_ANALYSER 1
#endif

#ifdef NO_ANALYSER
  #define ANALYSE(inout, time, extra)
#else
  #define ANALYSER_TRACE_LEVEL 5

  #define ANALYSE(inout, time, extra) \
    if (PTrace::CanTrace(ANALYSER_TRACE_LEVEL)) \
      m_analyser->inout(tick, time, m_frames.size(), extra)

  class RTP_JitterBufferAnalyser : public PObject
  {
      PCLASSINFO(RTP_JitterBufferAnalyser, PObject);

    protected:
      struct Info {
        Info() : time(0), depth(0), extra("") { }
        DWORD         time;
        PTimeInterval tick;
        size_t        depth;
        const char *  extra;
      };
      std::vector<Info> in, out;
      std::vector<Info>::size_type inPos, outPos;

    public:
      RTP_JitterBufferAnalyser()
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

      void In(PTimeInterval tick, DWORD time, size_t depth, const char * extra)
      {
        if (tick == 0)
          tick = PTimer::Tick();

        if (inPos == 0)
          in[inPos++].tick = tick;

        if (inPos < in.size()) {
          in[inPos].tick = tick;
          in[inPos].time = time;
          in[inPos].depth = depth;
          in[inPos++].extra = extra;
        }
      }

      void Out(PTimeInterval tick, DWORD time, size_t depth, const char * extra)
      {
        if (tick == 0)
          tick = PTimer::Tick();

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
        strm << "Input samples: " << inPos << " Output samples: " << outPos << "\n"
                "Dir\tRTPTime\tInDiff\tOutDiff\tInMode\tOutMode\t"
                "InDepth\tOutDep\tInTick\tInDelay\tOutTick\tOutDel\tIODelay\tTotalDelay\n";
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
                 << (out[ox].tick - in[ix].tick) << '\t'
                 << ((out[ox].tick - in[1].tick) - PTimeInterval((in[ix].time-in[1].time)/8))
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

OpalJitterBuffer::OpalJitterBuffer(unsigned minJitter,
                                   unsigned maxJitter,
                                   unsigned units,
                                     PINDEX packetSize)
  : m_timeUnits(units)
  , m_jitterGrowTime(10*units) // 10 milliseconds @ 8kHz
  , m_jitterShrinkPeriod(2000*units) // 2 seconds @ 8kHz
  , m_jitterShrinkTime(-5*units) // 5 milliseconds @ 8kHz
  , m_silenceShrinkPeriod(5000*units) // 5 seconds @ 8kHz
  , m_silenceShrinkTime(-20*units) // 20 milliseconds @ 8kHz
  , m_jitterDriftPeriod(500*units) // 0.5 second @ 8kHz
  , m_maxConsecutiveMarkerBits(10)
#ifdef NO_ANALYSER
  , m_analyser(NULL)
#else
  , m_analyser(new RTP_JitterBufferAnalyser)
#endif
{
  SetDelay(minJitter, maxJitter, packetSize);

  PTRACE(4, "Jitter\tBuffer created:" << *this);
}


OpalJitterBuffer::~OpalJitterBuffer()
{
#ifndef NO_ANALYSER
  PTRACE(ANALYSER_TRACE_LEVEL, "Jitter\tBuffer analysis: " << *this << '\n' << *m_analyser);
  delete m_analyser;
#endif

  PTRACE(4, "Jitter\tBuffer destroyed:" << *this);
}


void OpalJitterBuffer::PrintOn(ostream & strm) const
{
  strm << "this=" << (void *)this
       << " packets=" << m_frames.size()
       << " delay=" << (m_minJitterDelay/m_timeUnits) << '-'
                    << (m_currentJitterDelay/m_timeUnits) << '-'
                    << (m_maxJitterDelay/m_timeUnits) << "ms";
}


void OpalJitterBuffer::SetDelay(unsigned minJitterDelay, unsigned maxJitterDelay, PINDEX packetSize)
{
  m_bufferMutex.Wait();

  m_minJitterDelay     = minJitterDelay;
  m_maxJitterDelay     = maxJitterDelay;
  m_currentJitterDelay = minJitterDelay;
  m_packetSize         = packetSize;

  PTRACE(3, "Jitter\tDelays set to " << *this);

  m_packetsTooLate        = 0;
  m_bufferOverruns        = 0;
  m_consecutiveMarkerBits = 0;
  m_lastSyncSource        = 0;

  Reset();

  m_bufferMutex.Signal();
}


void OpalJitterBuffer::Reset()
{
  m_bufferMutex.Wait();

  m_averageFrameTime  = 0;
  m_lastTimestamp     = UINT_MAX;
  m_bufferFilledTime  = 0;
  m_bufferLowTime     = 0;
  m_bufferEmptiedTime = 0;
  m_timestampDelta    = 0;

  m_consecutiveLatePackets = 0;

  m_synchronisationState = e_SynchronisationStart;

  m_frames.clear();

  m_bufferMutex.Signal();
}


PBoolean OpalJitterBuffer::WriteData(const RTP_DataFrame & frame, const PTimeInterval & PTRACE_PARAM(tick))
{
  if (frame.GetSize() < RTP_DataFrame::MinHeaderSize) {
    PTRACE(2, "Jitter\tWriting invalid RTP data frame.");
    return true; // Don't abort, but ignore
  }

  PWaitAndSignal mutex(m_bufferMutex);

  DWORD timestamp = frame.GetTimestamp();

  /*Check for catastrophic failure, nothing removing packets from buffer! */
  if (m_frames.size() > 100) {
    PTRACE(2, "Jitter\tNothing being removed from buffer, aborting!");
    return false;
  }


  /*Deal with naughty systems that send continuous marker bits, thus we
    cannot use it to determine start of talk burst, and the need to refill
    the jitter buffer */
  if (m_consecutiveMarkerBits < m_maxConsecutiveMarkerBits) {
    if (frame.GetMarker()) {
      m_consecutiveMarkerBits++;
      Reset();

      // Have been told there is explicit silence by marker, take opportunity
      // to reduce the current jitter delay.
      AdjustCurrentJitterDelay(m_silenceShrinkTime);
      PTRACE(3, "Jitter\tStart talk burst: ts=" << timestamp << ", decreasing delay="
             << m_currentJitterDelay << " (" << (m_currentJitterDelay/m_timeUnits) << "ms)");
    }
    else
      m_consecutiveMarkerBits = 0;
  }
  else {
    if (m_consecutiveMarkerBits == m_maxConsecutiveMarkerBits) {
      PTRACE(2, "Jitter\tEvery packet has Marker bit, ignoring them from this client!");
      m_consecutiveMarkerBits++;
    }
  }


  /*Calculate the time between packets. While not actually dictated by any
    standards, this is invariably a constant. We just need to allow for if we
    are unlucky at the start and get a missing packet, in which case we are
    twice as big (or more) as we should be. The smallest distance between
    consecutive packets will be the frame time interval. */
  if (m_lastTimestamp != UINT_MAX) {
    int newFrameTime = timestamp - m_lastTimestamp;

    /* Check for naughty systems that have discontinuous timestamps, that is
       time has gone backwards or it has been 10 minutes since last packet,
       or an out of order packet was hanging in a router for 2 seconds.
       Neither likely! */
    if (newFrameTime < -16000 || newFrameTime > (m_averageFrameTime == 0 ? 4000 : 4800000)) {
      PTRACE(3, "Jitter\tTimestamps abruptly changed from "
              << m_lastTimestamp << " to " << timestamp << ", resynching");
      Reset();
    }
    else if (m_averageFrameTime == 0 || m_averageFrameTime > (DWORD)newFrameTime) {
      m_averageFrameTime = newFrameTime;
      PTRACE(4, "Jitter\tAverage frame time set to " << newFrameTime << " (" << (newFrameTime/m_timeUnits) << "ms)");
      AdjustCurrentJitterDelay(0);
    }
  }
  m_lastTimestamp = timestamp;



  if (frame.GetSyncSource() != m_lastSyncSource) {
    Reset();
    m_lastSyncSource = frame.GetSyncSource();
    PTRACE(4, "Jitter\tBuffer reset due to SSRC change.");
  }


  // Add to buffer
  pair<FrameMap::iterator,bool> result = m_frames.insert(FrameMap::value_type(timestamp, frame));
  if (result.second) {
    ANALYSE(In, timestamp, m_synchronisationState != e_SynchronisationDone ? "PreBuf" : "");
    PTRACE(6, "Jitter\tReceived packet : ts=" << timestamp);
  }
  else {
    PTRACE(2, "Jitter\tAttempt to insert two RTP packets with same timestamp: " << timestamp);
  }

  return true;
}


DWORD OpalJitterBuffer::CalculateRequiredTimestamp(DWORD playOutTimestamp) const
{
  DWORD timestamp = playOutTimestamp + m_timestampDelta;
  return timestamp > (DWORD)m_currentJitterDelay ? (timestamp - m_currentJitterDelay) : 0;
}


bool OpalJitterBuffer::AdjustCurrentJitterDelay(int delta)
{
  int minJitterDelay = max(m_minJitterDelay, 2*m_averageFrameTime);
  int maxJitterDelay = max(m_minJitterDelay, m_maxJitterDelay);

  if (delta < 0 && m_currentJitterDelay <= minJitterDelay)
    return false;
  if (delta > 0 && m_currentJitterDelay >= maxJitterDelay)
    return false;

  m_currentJitterDelay += delta;
  if (m_currentJitterDelay < minJitterDelay)
    m_currentJitterDelay = minJitterDelay;
  else if (m_currentJitterDelay > maxJitterDelay)
    m_currentJitterDelay = maxJitterDelay;

  return true;
}


#define COMMON_TRACE_INFO ": ts=" << requiredTimestamp << " (" << playOutTimestamp << "), size=" << m_frames.size()

PBoolean OpalJitterBuffer::ReadData(RTP_DataFrame & frame, const PTimeInterval & PTRACE_PARAM(tick))
{
  // Default response is an empty frame, ie silence
  frame.SetPayloadSize(0);

  PWaitAndSignal mutex(m_bufferMutex);

  // Now we get the timestamp the caller wants
  DWORD playOutTimestamp = frame.GetTimestamp();
  DWORD requiredTimestamp = CalculateRequiredTimestamp(playOutTimestamp);

  if (m_frames.empty()) {
    /*We ran the buffer down to empty, so have no data to play, play silence.
      This happens if packet is too late or completely missing. A too late
      packet will be picked up by later code.
     */
    PTRACE_IF(6, m_synchronisationState == e_SynchronisationDone, "Jitter\tBuffer is empty " COMMON_TRACE_INFO);
    ANALYSE(Out, requiredTimestamp, "Empty");

    if ((playOutTimestamp - m_bufferEmptiedTime) > m_silenceShrinkPeriod &&
         AdjustCurrentJitterDelay(m_silenceShrinkTime)) {
      PTRACE(4, "Jitter\tLong silence    " COMMON_TRACE_INFO << ", decreasing delay="
             << m_currentJitterDelay << " (" << (m_currentJitterDelay/m_timeUnits) << "ms)");
      m_bufferEmptiedTime = playOutTimestamp;
    }

    return true;
  }
  m_bufferEmptiedTime = playOutTimestamp;

  size_t framesInBuffer = m_averageFrameTime > 0 ? m_currentJitterDelay/m_averageFrameTime : 2;
  if (framesInBuffer < 2)
    framesInBuffer = 2;

  /* Check for buffer low (less than half full) for prologed period, then that generally
     means we have a sample clock drift problem, that is we have a clock of 8.01kHz and
     the remote has 7.99kHz so gradually the buffer drains as we take things out faster
     than they arrive. */
  if (m_bufferLowTime == 0 || m_frames.size() > framesInBuffer/2)
    m_bufferLowTime = playOutTimestamp;
  else if ((playOutTimestamp - m_bufferLowTime) > m_jitterDriftPeriod) {
    m_bufferLowTime = playOutTimestamp;
    PTRACE(4, "Jitter\tClock underrun  " COMMON_TRACE_INFO << " <= " << framesInBuffer << "/2");
    m_timestampDelta -= m_averageFrameTime;
    ANALYSE(Out, requiredTimestamp, "Drift");
    return true;
  }

  /* Check for buffer full (or nearly so) and count them. If full for a while
     then it is time to reduce the size of the jitter buffer */
  if (m_bufferFilledTime == 0 || m_frames.size() < framesInBuffer)
    m_bufferFilledTime = playOutTimestamp;
  else if ((playOutTimestamp - m_bufferFilledTime) > m_jitterShrinkPeriod) {
    m_bufferFilledTime = playOutTimestamp;

    bool adjusted = AdjustCurrentJitterDelay(m_jitterShrinkTime);
    PTRACE(4, "Jitter\tPackets on time " COMMON_TRACE_INFO << ", "
           << (adjusted ? "decreasing" : "cannot decrease") << " delay="
           << m_currentJitterDelay << " (" << (m_currentJitterDelay/m_timeUnits) << "ms)");
    if (adjusted)
      m_synchronisationState = e_SynchronisationShrink;
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
      PTRACE(5, "Jitter\tSynchronising   " COMMON_TRACE_INFO);
      // Do next state

    case e_SynchronisationFill :
      /* Now see if we have buffered enough yet */
      if (requiredTimestamp < oldestFrame->first) {
        /* Nope, play out some silence */
        ANALYSE(Out, oldestFrame->first, "PreBuf");
        return true;
      }

      /* Buffer now full, return the oldest frame. From now on the caller will
         call ReadData() with the next timestamp it wants which will be the
         timstamp of this oldest packet, plus the amount of audio data that
         is in it. */
      m_synchronisationState = e_SynchronisationDone;
      PTRACE(5, "Jitter\tSynchronise done" COMMON_TRACE_INFO << ", delay="
             << m_currentJitterDelay << " (" << (m_currentJitterDelay/m_timeUnits) << "ms)");
      break;

    case e_SynchronisationDone :
      // Get rid of all the frames that are too late
      while (requiredTimestamp >= oldestFrame->first + m_averageFrameTime) {
        if (++m_consecutiveLatePackets > 10) {
          PTRACE(3, "Jitter\tToo many late   " COMMON_TRACE_INFO);
          Reset();
          return true;
        }

        // Packets late, need a bigger jitter buffer
#if PTRACING
        bool adjusted =
#endif
        AdjustCurrentJitterDelay(m_jitterGrowTime);
        PTRACE(4, "Jitter\tPacket too late " COMMON_TRACE_INFO
                  << ", oldest=" << oldestFrame->first << ", "
                  << (adjusted ? "increasing" : "cannot increase") << " delay="
                  << m_currentJitterDelay << " (" << (m_currentJitterDelay/m_timeUnits) << "ms)");
        ANALYSE(Out, oldestFrame->first, "Late");
        m_frames.erase(oldestFrame);
        ++m_packetsTooLate;

        if (m_frames.empty()) {
          PTRACE(5, "Jitter\tBuffer emptied  " COMMON_TRACE_INFO);
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
      if (m_frames.size() < framesInBuffer*2)
        break;

      PTRACE(4, "Jitter\tClock overrun   " COMMON_TRACE_INFO << " >= " << framesInBuffer << "*2");
      m_timestampDelta += m_averageFrameTime;
      // Do next case

    case e_SynchronisationShrink :
      requiredTimestamp = CalculateRequiredTimestamp(playOutTimestamp);
      if (requiredTimestamp >= oldestFrame->first + m_averageFrameTime) {
        ANALYSE(Out, oldestFrame->first, "Shrink");
        m_frames.erase(oldestFrame);
        ++m_bufferOverruns;
        oldestFrame = m_frames.begin();
        PAssert(oldestFrame != m_frames.end(), PLogicError);
      }

      m_synchronisationState = e_SynchronisationDone;
      break;
  }

  /* Now we see if it time for our oldest packet, or that there is a missing
     packet (not arrived yet) in buffer. Can't wait for it, return no data.
     If the packet subsequently DOES arrive, it will get picked up by the
     too late section above. */
  if (requiredTimestamp < oldestFrame->first) {
    if (oldestFrame->first - requiredTimestamp > 8000) {
      PTRACE(3, "Jitter\tToo far in ahead" COMMON_TRACE_INFO);
      Reset();
    }
    else {
      PTRACE(4, "Jitter\tPacket not ready" COMMON_TRACE_INFO << ", oldest=" << oldestFrame->first);
      ANALYSE(Out, requiredTimestamp, "Wait");
    }
    return true;
  }

  // Finally can return the frame we have
  ANALYSE(Out, oldestFrame->first, "");
  PTRACE(6, "Jitter\tDelivered packet" COMMON_TRACE_INFO);
  frame = oldestFrame->second;
  m_frames.erase(oldestFrame);
  frame.SetTimestamp(playOutTimestamp);
  m_consecutiveLatePackets = 0;
  return true;
}


/////////////////////////////////////////////////////////////////////////////

OpalJitterBufferThread::OpalJitterBufferThread(unsigned minJitterDelay,
                                               unsigned maxJitterDelay,
                                               unsigned timeUnits,
                                                 PINDEX packetSize)
  : OpalJitterBuffer(minJitterDelay, maxJitterDelay, timeUnits, packetSize)
  , m_jitterThread(NULL)
  , m_running(true)
{
}


OpalJitterBufferThread::~OpalJitterBufferThread()
{
  WaitForThreadTermination(); // Failsafe, should be called from derived class dtor
}


void OpalJitterBufferThread::StartThread()
{
  m_bufferMutex.Wait();

  if (m_jitterThread == NULL) {
    m_jitterThread = PThread::Create(PCREATE_NOTIFIER(JitterThreadMain), "RTP Jitter");
    m_jitterThread->Resume();
  }

  m_bufferMutex.Signal();
}


void OpalJitterBufferThread::WaitForThreadTermination()
{
  m_running = false;

  m_bufferMutex.Wait();
  PThread * jitterThread = m_jitterThread;
  m_jitterThread = NULL;
  m_bufferMutex.Signal();

  if (jitterThread != NULL) {
    PTRACE(3, "Jitter\tWaiting for thread " << jitterThread->GetThreadName() << " on jitter buffer " << *this);
    PAssert(jitterThread->WaitForTermination(10000), "Jitter buffer thread did not terminate");
    delete jitterThread;
  } 
}


PBoolean OpalJitterBufferThread::ReadData(RTP_DataFrame & frame)
{
  if (m_running)
    return OpalJitterBuffer::ReadData(frame);

  PTRACE(3, "Jitter\tShutting down " << *this);
  return false;
}


void OpalJitterBufferThread::JitterThreadMain(PThread &, INT)
{
  PTRACE(4, "Jitter\tReceive thread started: " << *this);

  while (m_running) {
    RTP_DataFrame frame((PINDEX)0, m_packetSize);

    // Keep reading from the RTP transport frames
    if (!OnReadPacket(frame) || !WriteData(frame))
      m_running = false;
  }

  PTRACE(4, "Jitter\tReceive thread finished: " << *this);
}


/////////////////////////////////////////////////////////////////////////////

RTP_JitterBuffer::RTP_JitterBuffer(OpalRTPSession & session,
                                   unsigned         minJitterDelay,
                                   unsigned         maxJitterDelay,
                                   unsigned         time,
                                   PINDEX           packetSize)
  : OpalJitterBufferThread(minJitterDelay, maxJitterDelay, time, packetSize),
    m_session(session)
{
  PTRACE_CONTEXT_ID_FROM(session);

  StartThread();
}


RTP_JitterBuffer::~RTP_JitterBuffer()
{
  PTRACE(4, "Jitter\tDestroying jitter buffer " << *this);

  m_running = false;
  bool reopen = m_session.Shutdown(true);

  WaitForThreadTermination();

  if (reopen)
    m_session.Restart(true);
}


PBoolean RTP_JitterBuffer::OnReadPacket(RTP_DataFrame & frame)
{
  if (!m_session.InternalReadData(frame))
    return false;

#if OPAL_RTCP_XR
  m_session.GetMetrics().SetJitterDelay(GetCurrentJitterDelay()/GetTimeUnits());
#endif

  PTRACE(6, "Jitter\tOnReadPacket: Frame from network, timestamp " << frame.GetTimestamp());
  return true;
}


/////////////////////////////////////////////////////////////////////////////
