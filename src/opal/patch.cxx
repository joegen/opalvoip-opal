/*
 * patch.cxx
 *
 * Media stream patch thread.
 *
 * Open Phone Abstraction Library (OPAL)
 * Formally known as the Open H323 project.
 *
 * Copyright (c) 2001 Equivalence Pty. Ltd.
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
 * The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * Contributor(s): ______________________________________.
 *
 */

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "patch.h"
#endif


#include <opal_config.h>

#include <opal/patch.h>
#include <opal/mediastrm.h>
#include <opal/endpoint.h>
#include <opal/transcoders.h>
#include <rtp/rtpconn.h>

#if OPAL_VIDEO
#include <codec/vidcodec.h>
#endif

#define PTraceModule() "Patch"

#define new PNEW


/////////////////////////////////////////////////////////////////////////////

OpalMediaPatch::OpalMediaPatch(OpalMediaStream & src)
  : PSafeObject(m_instrumentedMutex)
  , m_source(src)
  , m_bypassToPatch(NULL)
  , m_bypassFromPatch(NULL)
  , m_patchThread(NULL)
#if OPAL_STATISTICS
  , m_patchThreadId(PNullThreadIdentifier)
#endif
  , m_transcoderChanged(false)
{
  PTRACE_CONTEXT_ID_FROM(src);

  PTRACE(5, "Created media patch " << this << ", session " << src.GetSessionID());
  src.SetPatch(this);
  m_source.SafeReference();
}


OpalMediaPatch::~OpalMediaPatch()
{
  StopThread();
  m_source.SafeDereference();
  PTRACE(5, "Destroyed media patch " << this);
}


void OpalMediaPatch::PrintOn(ostream & strm) const
{
  strm << GetClass() << '[' << this << "] " << m_source;

  P_INSTRUMENTED_LOCK_READ_ONLY(return);

  if (m_sinks.GetSize() > 0) {
    strm << " -> ";
    if (m_sinks.GetSize() == 1)
      strm << *m_sinks.front().m_stream;
    else {
      PINDEX i = 0;
      for (PList<Sink>::const_iterator s = m_sinks.begin(); s != m_sinks.end(); ++s,++i) {
        if (i > 0)
          strm << ", ";
        strm << "sink[" << i << "]=" << *s->m_stream;
      }
    }
  }
}


bool OpalMediaPatch::CanStart() const
{
  if (!m_source.IsOpen()) {
    PTRACE(4, "Delaying patch start till source stream open: " << *this);
    return false;
  }

  if (m_sinks.IsEmpty()) {
    PTRACE(4, "Delaying patch start till have sink stream: " << *this);
    return false;
  }

  for (PList<Sink>::const_iterator s = m_sinks.begin(); s != m_sinks.end(); ++s) {
    if (!s->m_stream->IsOpen()) {
      PTRACE(4, "Delaying patch start till sink stream open: " << *this);
      return false;
    }
  }

  PSafePtr<OpalRTPConnection> connection = dynamic_cast<OpalRTPConnection *>(&m_source.GetConnection());
  if (connection == NULL)
    connection = m_source.GetConnection().GetOtherPartyConnectionAs<OpalRTPConnection>();
  if (connection == NULL) {
    PTRACE(4, "Allow patch start as connection not RTP: " << *this);
    return true;
	}

  OpalMediaSession * session = connection->GetMediaSession(m_source.GetSessionID());
  if (session == NULL) {
    PTRACE(4, "Allow patch start as does not have session " << session->GetSessionID() << ": " << *this);
    return true;
	}

  if (session->IsOpen())
    return true;

  // Special case for some gateway modes, really weird place to put it, but this is a good time
  if (dynamic_cast<OpalDummySession *>(session) != NULL && session->Open(PString::Empty(), session->GetRemoteAddress()))
    return true;

  PTRACE(4, "Delaying patch start till session " << session->GetSessionID() << " open: " << *this);
  return false;
}


void OpalMediaPatch::Start()
{
  PWaitAndSignal m(m_patchThreadMutex);
	
  if(m_patchThread != NULL && !m_patchThread->IsTerminated()) {
    PTRACE(5, "Already started thread " << m_patchThread->GetThreadName());
    return;
  }

  delete m_patchThread;
  m_patchThread = NULL;

  if (CanStart()) {
    PString threadName = m_source.GetPatchThreadName();
    if (threadName.IsEmpty() && !m_sinks.empty())
      threadName = m_sinks.front().m_stream->GetPatchThreadName();
    if (threadName.IsEmpty())
      threadName = "Media Patch";
    m_patchThread = new PThreadObj<OpalMediaPatch>(*this, &OpalMediaPatch::Main, false, threadName, PThread::HighPriority);
    PTRACE_CONTEXT_ID_TO(m_patchThread);
    PThread::Yield();
    PTRACE(4, "Starting thread " << m_patchThread->GetThreadName());
  }
}


void OpalMediaPatch::StopThread()
{
  PThread::WaitAndDelete(m_patchThread, 10000, &m_patchThreadMutex);
}


void OpalMediaPatch::Close()
{
  PTRACE(3, "Closing media patch " << *this);

  if (!LockReadWrite(P_DEBUG_LOCATION))
    return;

  if (m_bypassFromPatch != NULL)
    m_bypassFromPatch->SetBypassPatch(NULL);
  else
    SetBypassPatch(NULL);

  m_filters.RemoveAll();
  if (m_source.GetPatch() == this) {
    UnlockReadWrite(P_DEBUG_LOCATION);
    m_source.Close();
    if (!LockReadWrite(P_DEBUG_LOCATION))
      return;
  }

  while (m_sinks.GetSize() > 0) {
    OpalMediaStreamPtr stream = m_sinks.front().m_stream;
    UnlockReadWrite(P_DEBUG_LOCATION);
    if (stream == NULL || !stream->Close()) {
      // The only way we can get here is if the sink is in the proccess of being closed
      // but is blocked on the mutex waiting to remove the sink from this patch.
      // Se we unlock it, and wait for it to do it in the other thread.
      PThread::Sleep(10);
    }
    if (!LockReadWrite(P_DEBUG_LOCATION))
      return;
  }
  UnlockReadWrite(P_DEBUG_LOCATION);

  StopThread();
}


PBoolean OpalMediaPatch::AddSink(const OpalMediaStreamPtr & sinkStream)
{
  P_INSTRUMENTED_LOCK_READ_WRITE();

  if (PAssertNULL(sinkStream) == NULL)
    return false;

  PAssert(sinkStream->IsSink(), "Attempt to set source stream as sink!");

  if (!sinkStream->SetPatch(this)) {
    PTRACE(2, "Could not set patch in stream " << *sinkStream);
    return false;
  }

  Sink * sink = new Sink(*this, sinkStream);
  m_sinks.Append(sink);
  if (!sink->CreateTranscoders())
    return false;

  EnableJitterBuffer();
  return true;
}


bool OpalMediaPatch::ResetTranscoders()
{
  P_INSTRUMENTED_LOCK_READ_WRITE();

  for (PList<Sink>::iterator s = m_sinks.begin(); s != m_sinks.end(); ++s) {
    if (!s->CreateTranscoders())
      return false;
    m_transcoderChanged = true;
  }

  return true;
}


static bool SetStreamDataSize(OpalMediaStream & stream, OpalTranscoder & codec)
{
  OpalMediaFormat format = stream.IsSource() ? codec.GetOutputFormat() : codec.GetInputFormat();
  PINDEX size = codec.GetOptimalDataFrameSize(stream.IsSource());
  if (stream.SetDataSize(size, format.GetFrameTime()*stream.GetMediaFormat().GetClockRate()/format.GetClockRate()))
    return true;

  PTRACE(1, "Stream " << stream << " cannot support data size " << size);
  return false;
}


bool OpalMediaPatch::Sink::CreateTranscoders()
{
  delete m_primaryCodec;
  m_primaryCodec = NULL;
  delete m_secondaryCodec;
  m_secondaryCodec = NULL;

  // Find the media formats than can be used to get from source to sink
  OpalMediaFormat sourceFormat = m_patch.m_source.GetMediaFormat();
  OpalMediaFormat destinationFormat = m_stream->GetMediaFormat();

  PTRACE(5, "AddSink\n"
            "Source format:\n" << setw(-1) << sourceFormat << "\n"
            "Destination format:\n" << setw(-1) << destinationFormat);

  if (sourceFormat == destinationFormat) {
    PINDEX framesPerPacket = destinationFormat.GetOptionInteger(OpalAudioFormat::TxFramesPerPacketOption(),
                                  sourceFormat.GetOptionInteger(OpalAudioFormat::TxFramesPerPacketOption(), 1));
    PINDEX packetSize = sourceFormat.GetFrameSize()*framesPerPacket;
    PINDEX packetTime = sourceFormat.GetFrameTime()*framesPerPacket;
    m_patch.m_source.SetDataSize(packetSize, packetTime);
    m_stream->SetDataSize(packetSize, packetTime);
    m_stream->InternalUpdateMediaFormat(m_stream->GetMediaFormat());
    m_patch.m_source.InternalUpdateMediaFormat(m_patch.m_source.GetMediaFormat());
#if OPAL_STATISTICS
    m_audioFormat = sourceFormat;
#if OPAL_VIDEO
    m_videoFormat = sourceFormat;
#endif // OPAL_VIDEO
#endif // OPAL_STATISTICS
    PTRACE(3, "Changed to direct media on " << m_patch);
    return true;
  }

  PString id = m_stream->GetID();
  m_primaryCodec = OpalTranscoder::Create(sourceFormat, destinationFormat, (const BYTE *)id, id.GetLength());
  if (m_primaryCodec != NULL) {
    PTRACE_CONTEXT_ID_TO(m_primaryCodec);
    PTRACE(4, "Created primary codec " << sourceFormat << "->" << destinationFormat << " with ID " << id);

    if (!SetStreamDataSize(*m_stream, *m_primaryCodec))
      return false;
    m_primaryCodec->SetMaxOutputSize(m_stream->GetDataSize());
    m_primaryCodec->SetSessionID(m_patch.m_source.GetSessionID());
    m_primaryCodec->SetCommandNotifier(PCREATE_NOTIFIER_EXT(&m_patch, OpalMediaPatch, InternalOnMediaCommand1));

    if (!SetStreamDataSize(m_patch.m_source, *m_primaryCodec))
      return false;
    m_patch.m_source.InternalUpdateMediaFormat(m_primaryCodec->GetInputFormat());
    m_stream->InternalUpdateMediaFormat(m_primaryCodec->GetOutputFormat());

    PTRACE(3, "Added media stream sink " << *m_stream
           << " using transcoder " << *m_primaryCodec << ", data size=" << m_stream->GetDataSize());
    return true;
  }

  PTRACE(4, "Creating two stage transcoders for " << sourceFormat << "->" << destinationFormat << " with ID " << id);
  OpalMediaFormat intermediateFormat;
  if (!OpalTranscoder::FindIntermediateFormat(sourceFormat, destinationFormat, intermediateFormat)) {
    PTRACE(1, "Could find compatible media format for " << *m_stream);
    return false;
  }

  if (intermediateFormat.GetMediaType() == OpalMediaType::Audio()) {
    // try prepare intermediateFormat for correct frames to frames transcoding
    // so we need make sure that tx frames time of destinationFormat be equal 
    // to tx frames time of intermediateFormat (all this does not produce during
    // Merge phase in FindIntermediateFormat)
    int destinationPacketTime = destinationFormat.GetFrameTime()*destinationFormat.GetOptionInteger(OpalAudioFormat::TxFramesPerPacketOption(), 1);
    if ((destinationPacketTime % intermediateFormat.GetFrameTime()) != 0) {
      PTRACE(1, "Could produce without buffered media format converting (which not implemented yet) for " << *m_stream);
      return false;
    }
    intermediateFormat.AddOption(new OpalMediaOptionUnsigned(OpalAudioFormat::TxFramesPerPacketOption(),
                                                              true,
                                                              OpalMediaOption::NoMerge,
                                                              destinationPacketTime/intermediateFormat.GetFrameTime()),
                                  true);
  }

  m_primaryCodec = OpalTranscoder::Create(sourceFormat, intermediateFormat, (const BYTE *)id, id.GetLength());
  m_secondaryCodec = OpalTranscoder::Create(intermediateFormat, destinationFormat, (const BYTE *)id, id.GetLength());
  if (m_primaryCodec == NULL || m_secondaryCodec == NULL)
    return false;

  PTRACE_CONTEXT_ID_TO(m_primaryCodec);
  PTRACE_CONTEXT_ID_TO(m_secondaryCodec);
  PTRACE(3, "Created two stage codec " << sourceFormat << "/" << intermediateFormat << "/" << destinationFormat << " with ID " << id);

  m_primaryCodec->SetMaxOutputSize(m_secondaryCodec->GetOptimalDataFrameSize(true));
  m_primaryCodec->SetSessionID(m_patch.m_source.GetSessionID());
  m_primaryCodec->SetCommandNotifier(PCREATE_NOTIFIER_EXT(&m_patch, OpalMediaPatch, InternalOnMediaCommand1));
  m_primaryCodec->UpdateMediaFormats(OpalMediaFormat(), m_secondaryCodec->GetInputFormat());

  if (!SetStreamDataSize(*m_stream, *m_secondaryCodec))
    return false;
  m_secondaryCodec->SetMaxOutputSize(m_stream->GetDataSize());
  m_secondaryCodec->SetSessionID(m_patch.m_source.GetSessionID());
  m_secondaryCodec->SetCommandNotifier(PCREATE_NOTIFIER_EXT(&m_patch, OpalMediaPatch, InternalOnMediaCommand1));
  m_secondaryCodec->UpdateMediaFormats(m_primaryCodec->GetInputFormat(), OpalMediaFormat());

  if (!SetStreamDataSize(m_patch.m_source, *m_primaryCodec))
    return false;
  m_patch.m_source.InternalUpdateMediaFormat(m_primaryCodec->GetInputFormat());
  m_stream->InternalUpdateMediaFormat(m_secondaryCodec->GetOutputFormat());

  PTRACE(3, "Added media stream sink " << *m_stream
          << " using transcoders " << *m_primaryCodec
          << " and " << *m_secondaryCodec << ", data size=" << m_stream->GetDataSize());
  return true;
}


void OpalMediaPatch::RemoveSink(const OpalMediaStream & stream)
{
  PTRACE(3, "Removing sink " << stream << " from " << *this);

  bool closeSource = false;

  {
    P_INSTRUMENTED_LOCK_READ_WRITE(return);

    for (PList<Sink>::iterator s = m_sinks.begin(); s != m_sinks.end(); ++s) {
      if (s->m_stream == &stream) {
        m_sinks.erase(s);
        PTRACE(5, "Removed sink " << stream << " from " << *this);
        break;
      }
    }

    if (m_sinks.IsEmpty()) {
      closeSource = true;
      if (m_bypassFromPatch != NULL)
        m_bypassFromPatch->SetBypassPatch(NULL);
    }
  }

  if (closeSource  && m_source.GetPatch() == this)
    m_source.Close();
}


OpalMediaStreamPtr OpalMediaPatch::GetSink(PINDEX i) const
{
  P_INSTRUMENTED_LOCK_READ_ONLY();
  return i < m_sinks.GetSize() ? m_sinks[i].m_stream : OpalMediaStreamPtr();
}


OpalMediaFormat OpalMediaPatch::GetSinkFormat(PINDEX i) const
{
  OpalMediaFormat fmt;

  OpalTranscoder * xcoder = GetAndLockSinkTranscoder(i);
  if (xcoder != NULL) {
    fmt = xcoder->GetOutputFormat();
    UnLockSinkTranscoder();
  }

  return fmt;
}


OpalTranscoder * OpalMediaPatch::GetAndLockSinkTranscoder(PINDEX i) const
{
  if (!LockReadOnly(P_DEBUG_LOCATION))
    return NULL;

  if (i >= m_sinks.GetSize()) {
    UnlockReadOnly(P_DEBUG_LOCATION);
    return NULL;
  }

  Sink & sink = m_sinks[i];
  if (sink.m_secondaryCodec != NULL) 
    return sink.m_secondaryCodec;

  if (sink.m_primaryCodec != NULL)
    return sink.m_primaryCodec;

  UnlockReadOnly(P_DEBUG_LOCATION);

  return NULL;
}


void OpalMediaPatch::UnLockSinkTranscoder() const
{
  UnlockReadOnly(P_DEBUG_LOCATION);
}


#if OPAL_STATISTICS
void OpalMediaPatch::GetStatistics(OpalMediaStatistics & statistics, bool fromSink) const
{
  P_INSTRUMENTED_LOCK_READ_ONLY(return);

  statistics.m_threadIdentifier = m_patchThreadId;

  if (fromSink)
    m_source.GetStatistics(statistics, true);

  if (!m_sinks.IsEmpty())
    m_sinks.front().GetStatistics(statistics, !fromSink);
}


void OpalMediaPatch::Sink::GetStatistics(OpalMediaStatistics & statistics, bool fromSource) const
{
  if (fromSource)
    m_stream->GetStatistics(statistics, true);

  {
    PWaitAndSignal mutex(m_statsMutex);
    AudioStatsMap::const_iterator itAud = m_audioStatistics.find(statistics.m_SSRC);
    if (itAud != m_audioStatistics.end()) {
      //statistics.m_silent = itAud->second.m_silent;
      statistics.m_FEC = itAud->second.m_FEC;
    }

#if OPAL_VIDEO
    VideoStatsMap::const_iterator itVid = m_videoStatistics.find(statistics.m_SSRC);
    if (itVid != m_videoStatistics.end())
      statistics.OpalVideoStatistics::operator=(itVid->second);
#endif
  }

  if (m_primaryCodec != NULL)
    m_primaryCodec->GetStatistics(statistics);

  if (m_secondaryCodec != NULL)
    m_secondaryCodec->GetStatistics(statistics);
}
#endif // OPAL_STATISTICS


OpalMediaPatch::Sink::Sink(OpalMediaPatch & p, const OpalMediaStreamPtr & s)
  : m_patch(p)
  , m_stream(s)
  , m_primaryCodec(NULL)
  , m_secondaryCodec(NULL)
{
  PTRACE_CONTEXT_ID_FROM(p);

  PTRACE(3, "Created Sink for " << p);
}


OpalMediaPatch::Sink::~Sink()
{
  delete m_primaryCodec;
  delete m_secondaryCodec;
}


void OpalMediaPatch::AddFilter(const PNotifier & filter, const OpalMediaFormat & stage)
{
  P_INSTRUMENTED_LOCK_READ_WRITE();

  if (m_source.GetMediaFormat().GetMediaType() != stage.GetMediaType())
    return;

  // ensures that a filter is added only once
  for (PList<Filter>::iterator f = m_filters.begin(); f != m_filters.end(); ++f) {
    if (f->m_notifier == filter && f->m_stage == stage) {
      PTRACE(4, "Filter already added for stage " << stage);
      return;
    }
  }
  m_filters.Append(new Filter(filter, stage));
}


PBoolean OpalMediaPatch::RemoveFilter(const PNotifier & filter, const OpalMediaFormat & stage)
{
  P_INSTRUMENTED_LOCK_READ_WRITE();

  for (PList<Filter>::iterator f = m_filters.begin(); f != m_filters.end(); ++f) {
    if (f->m_notifier == filter && f->m_stage == stage) {
      m_filters.erase(f);
      return true;
    }
  }

  PTRACE(4, "No filter to remove for stage " << stage);
  return false;
}


void OpalMediaPatch::FilterFrame(RTP_DataFrame & frame, const OpalMediaFormat & mediaFormat)
{
  // Should already be locked for read

  for (PList<Filter>::iterator f = m_filters.begin(); f != m_filters.end(); ++f) {
    if (f->m_stage.IsEmpty() || f->m_stage == mediaFormat)
      f->m_notifier(frame, (P_INT_PTR)this);
  }
}


bool OpalMediaPatch::UpdateMediaFormat(const OpalMediaFormat & mediaFormat)
{
  P_INSTRUMENTED_LOCK_READ_ONLY();

  bool atLeastOne = m_source.InternalUpdateMediaFormat(mediaFormat);

  for (PList<Sink>::iterator s = m_sinks.begin(); s != m_sinks.end(); ++s) {
    if (s->UpdateMediaFormat(mediaFormat)) {
      m_source.InternalUpdateMediaFormat(s->m_stream->GetMediaFormat());
      atLeastOne = true;
    }
  }

  PTRACE_IF(2, !atLeastOne, "Could not update media format for any stream/transcoder in " << *this);
  return atLeastOne;
}


PBoolean OpalMediaPatch::ExecuteCommand(const OpalMediaCommand & command)
{
  bool atLeastOne = false;
  PSafePtr<OpalMediaPatch> fromPatch, toPatch;

  {
    P_INSTRUMENTED_LOCK_READ_ONLY(return false);

    if (m_bypassFromPatch != NULL) // Don't use tradic ?: as GNU doesn't like it
      fromPatch = m_bypassFromPatch;
    else
      fromPatch = this;

    if (m_bypassToPatch != NULL) // Don't use tradic ?: as GNU doesn't like it
      toPatch = m_bypassToPatch;
    else
      toPatch = this;
  }

  if (fromPatch.SetSafetyMode(PSafeReadOnly)) {
    atLeastOne = fromPatch->m_source.InternalExecuteCommand(command);
    fromPatch.SetSafetyMode(PSafeReference);
  }

  if (toPatch.SetSafetyMode(PSafeReadOnly)) {
    for (PList<Sink>::iterator s = toPatch->m_sinks.begin(); s != toPatch->m_sinks.end(); ++s) {
      if (s->ExecuteCommand(command, atLeastOne))
        atLeastOne = true;
    }
    toPatch.SetSafetyMode(PSafeReference);
  }

#if PTRACING
  if (PTrace::CanTrace(5)) {
    ostream & trace = PTRACE_BEGIN(5);
    trace << "Execute" << (atLeastOne ? "d" : " cancelled for ")
          << " command \"" << command << '"';
    if (fromPatch != this)
      trace << " bypassing " << *fromPatch << " to " << *this;
    else if (toPatch != this)
      trace << " bypassing " << *this << " to " << *toPatch;
    else
      trace << " on " << *this;
    trace << PTrace::End;
  }
#endif

  return atLeastOne;
}


void OpalMediaPatch::InternalOnMediaCommand1(OpalMediaCommand & command, P_INT_PTR)
{
  m_source.GetConnection().GetEndPoint().GetManager().QueueDecoupledEvent(new PSafeWorkArg1<OpalMediaPatch, OpalMediaCommand *>(
              this, command.CloneAs<OpalMediaCommand>(), &OpalMediaPatch::InternalOnMediaCommand2));
}


void OpalMediaPatch::InternalOnMediaCommand2(OpalMediaCommand * command)
{
  m_source.ExecuteCommand(*command);
  delete command;
}


bool OpalMediaPatch::InternalSetPaused(bool pause, bool fromUser)
{
  P_INSTRUMENTED_LOCK_READ_ONLY();

  bool atLeastOne = m_source.InternalSetPaused(pause, fromUser, true);

  for (PList<Sink>::iterator s = m_sinks.begin(); s != m_sinks.end(); ++s) {
    if (s->m_stream->InternalSetPaused(pause, fromUser, true))
      atLeastOne = true;
  }

  return atLeastOne;
}


bool OpalMediaPatch::OnStartMediaPatch()
{
  P_INSTRUMENTED_LOCK_READ_ONLY();

  m_source.OnStartMediaPatch();

  for (PList<Sink>::iterator s = m_sinks.begin(); s != m_sinks.end(); ++s)
    s->m_stream->OnStartMediaPatch();

  if (m_source.IsSynchronous())
    return false;

  return EnableJitterBuffer();
}


bool OpalMediaPatch::EnableJitterBuffer(bool enab)
{
  P_INSTRUMENTED_LOCK_READ_ONLY();

  if (m_bypassToPatch != NULL)
    enab = false;

  PList<Sink>::iterator s;
  for (s = m_sinks.begin(); s != m_sinks.end(); ++s) {
    if (s->m_stream->EnableJitterBuffer(enab)) {
      m_source.EnableJitterBuffer(false);
      return true;
    }
  }

  for (s = m_sinks.begin(); s != m_sinks.end(); ++s) {
    if (m_source.EnableJitterBuffer(enab && s->m_stream->IsSynchronous()))
      return true;
  }

  return false;
}


void OpalMediaPatch::Main()
{
  PTRACE(4, "Thread started for " << *this);
	
#if OPAL_STATISTICS
  m_patchThreadId = PThread::GetCurrentThreadId();
#endif

  bool asynchronous = OnStartMediaPatch();
  PAdaptiveDelay asynchPacing;
  PThread::Times lastThreadTimes;
  const unsigned CheckCPUTimeMS =
#if P_CONFIG_FILE
                                  PConfig(PConfig::Environment).GetInteger("OPAL_MEDIA_PATCH_CPU_CHECK",
#else
                                  (
#endif
                                   2000);
  static const int ThresholdPercent = 90;
  PTRACE_THROTTLE(ThrottleCPU, 3, 30000, 5);


  /* Note the RTP frame is outside loop so that a) it is more efficient
     for memory usage, the buffer is only ever increased and not allocated
     on the heap ever time, and b) the timestamp value embedded into the
     sourceFrame is needed for correct operation of the jitter buffer and
     silence frames. It is adjusted by DispatchFrame (really Sink::WriteFrame)
     each time and passed back in to source.Read() (and eventually the JB) so
     it knows where it is up to in extracting data from the JB. */
  RTP_DataFrame sourceFrame(0);

  while (m_source.IsOpen()) {
    if (m_source.IsPaused()) {
      PThread::Sleep(100);
      if (m_patchThread == NULL)
        break; // Shutting down
      continue;
    }

    if (!m_source.ReadPacket(sourceFrame)) {
      PTRACE(4, "Thread ended because source read failed on " << *this);
      break;
    }
 
    if (!DispatchFrame(sourceFrame)) {
      PTRACE(4, "Thread ended because all sink writes failed on " << *this);
      break;
    }
 
    if (asynchronous)
      asynchPacing.Delay(10);

    /* Don't starve the CPU if we have idle frames and the no source or
       destination is synchronous. Note that performing a Yield is not good
       enough, as the media patch threads are high priority and will consume
       all available CPU if allowed. Also just doing a sleep each time around
       the loop slows down video where you get clusters of packets thrown at
       us, want to clear them as quickly as possible out of the UDP OS buffers
       or we overflow and lose some. Best compromise is to every X ms, sleep
       for X/10 ms so can not use more than about 90% of CPU. */
    int percentage = PThread::GetPercentageCPU(lastThreadTimes, CheckCPUTimeMS);
    if (percentage >= 0) {
      if (percentage < ThresholdPercent)
        PTRACE(ThrottleCPU, "CPU for " << *this << " since start is " << lastThreadTimes);
      else {
        PTRACE(2, "Greater than " << ThresholdPercent << "% CPU usage for " << *this);
        PThread::Sleep(CheckCPUTimeMS*(100-ThresholdPercent)/100);
      }
    }
  }

  m_source.OnStopMediaPatch(*this);

  if (m_sinks.IsEmpty() && m_source.GetPatch() == this) {
    PTRACE(4, "Closing source media stream as no sinks in " << *this);
    m_source.GetConnection().GetEndPoint().GetManager().QueueDecoupledEvent(
                new PSafeWorkArg1<OpalConnection, OpalMediaStreamPtr, bool>(&m_source.GetConnection(),
                                                        &m_source, &OpalConnection::CloseMediaStream));
  }

  PTRACE(4, "Thread ended for " << *this);
}


bool OpalMediaPatch::SetBypassPatch(const OpalMediaPatchPtr & patch)
{
  P_INSTRUMENTED_LOCK_READ_WRITE();

  if (!PAssert(m_bypassFromPatch == NULL, PLogicError))
    return false; // Can't be both!

  if (m_bypassToPatch == patch)
    return true; // Already set

  PTRACE(4, "Setting media patch bypass to " << patch << " on " << *this);

  if (m_bypassToPatch != NULL) {
    if (!PAssert(m_bypassToPatch->m_bypassFromPatch == this, PLogicError))
      return false;

    m_bypassToPatch->m_bypassFromPatch.SetNULL();
    m_bypassToPatch->m_bypassEnded.Signal();

    if (patch == NULL)
      m_bypassToPatch->EnableJitterBuffer();
  }

  if (patch != NULL) {
    if (!PAssert(patch->m_bypassFromPatch == NULL, PLogicError))
      return false;

    patch->m_bypassFromPatch = this;
  }

  m_bypassToPatch = patch;

#if OPAL_VIDEO
  OpalMediaFormat format = m_source.GetMediaFormat();
  if (format.IsTransportable() && format.GetMediaType() == OpalMediaType::Video())
    m_source.ExecuteCommand(OpalVideoUpdatePicture());
#endif

  if (patch == NULL)
    EnableJitterBuffer();
  else {
    EnableJitterBuffer(false);
    patch->EnableJitterBuffer(false);
  }

  return true;
}


PBoolean OpalMediaPatch::PushFrame(RTP_DataFrame & frame)
{
  return DispatchFrame(frame);
}


bool OpalMediaPatch::DispatchFrame(RTP_DataFrame & frame)
{
  if (!LockReadOnly(P_DEBUG_LOCATION))
    return false;

  if (m_bypassFromPatch != NULL) {
    PTRACE(3, "Media patch bypass started by " << *m_bypassFromPatch << " on " << *this);
    UnlockReadOnly(P_DEBUG_LOCATION);
    m_bypassEnded.Wait();
    PTRACE(4, "Media patch bypass ended on " << *this);
    return true;
  }

  FilterFrame(frame, m_source.GetMediaFormat());

  OpalMediaPatchPtr patch = m_bypassToPatch;
  if (patch == NULL) {
    bool result = DispatchFrameLocked(frame, false);
    UnlockReadOnly(P_DEBUG_LOCATION);
    return result;
  }

  UnlockReadOnly(P_DEBUG_LOCATION);

  P_INSTRUMENTED_LOCK_READ_ONLY();
  return lock.IsLocked() && patch->DispatchFrameLocked(frame, true);
}


bool OpalMediaPatch::DispatchFrameLocked(RTP_DataFrame & frame, bool bypassing)
{
  if (m_transcoderChanged) {
    m_transcoderChanged = false;
    PTRACE(3, "Ignoring source data with transcoder change on " << *this);
    return true;
  }

  if (m_sinks.empty()) {
    PTRACE(2, "No sinks available on " << *this);
    return false;
  }

  bool written = false;
  for (PList<Sink>::iterator s = m_sinks.begin(); s != m_sinks.end(); ++s) {
    if (s->WriteFrame(frame, bypassing))
      written = true;
  }

  return written;
}


bool OpalMediaPatch::Sink::UpdateMediaFormat(const OpalMediaFormat & mediaFormat)
{
  bool ok;

  if (m_primaryCodec == NULL)
    ok = m_stream->InternalUpdateMediaFormat(mediaFormat);
  else if (m_secondaryCodec == NULL)
    ok = m_primaryCodec->UpdateMediaFormats(mediaFormat, mediaFormat) &&
         m_stream->InternalUpdateMediaFormat(m_primaryCodec->GetOutputFormat());
  else
    ok = m_primaryCodec->UpdateMediaFormats(mediaFormat, mediaFormat) &&
         m_secondaryCodec->UpdateMediaFormats(m_primaryCodec->GetOutputFormat(), m_primaryCodec->GetOutputFormat()) &&
         m_stream->InternalUpdateMediaFormat(m_secondaryCodec->GetOutputFormat());

  PTRACE(3, "Updated Sink: format=" << mediaFormat << " ok=" << ok);
  return ok;
}


bool OpalMediaPatch::Sink::ExecuteCommand(const OpalMediaCommand & command, bool atLeastOne)
{
  atLeastOne = m_stream->InternalExecuteCommand(command) || atLeastOne;

  if (m_secondaryCodec != NULL)
    atLeastOne = m_secondaryCodec->ExecuteCommand(command) || atLeastOne;

  if (m_primaryCodec != NULL)
    atLeastOne = m_primaryCodec->ExecuteCommand(command) || atLeastOne;

#if OPAL_VIDEO && OPAL_STATISTICS
  if (atLeastOne) {
    const OpalVideoUpdatePicture * update = dynamic_cast<const OpalVideoUpdatePicture *>(&command);
    if (update != NULL) {
      bool full = dynamic_cast<const OpalVideoPictureLoss *>(&command) == NULL;
      PWaitAndSignal mutex(m_statsMutex);
      m_videoStatistics[0].IncrementUpdateCount(full);
      if (update->GetSyncSource() != 0)
        m_videoStatistics[update->GetSyncSource()].IncrementUpdateCount(full);
    }
  }
#endif

  return atLeastOne;
}


bool OpalMediaPatch::Sink::WriteFrame(RTP_DataFrame & sourceFrame, bool bypassing)
{
  if (m_stream->IsPaused())
    return true;

  if (bypassing || m_primaryCodec == NULL) {
#if OPAL_STATISTICS
    OpalAudioFormat::FrameType audioFrameType;
    if (m_audioFormat.IsValid())
      audioFrameType = m_audioFormat.GetFrameType(sourceFrame.GetPayloadPtr(), sourceFrame.GetPayloadSize(), m_audioFrameDetector);

#if OPAL_VIDEO
    // Must be done before the WritePacket() which could encrypt the packet
    OpalVideoFormat::FrameType videoFrameType;
    if (m_videoFormat.IsValid())
      videoFrameType = m_videoFormat.GetFrameType(sourceFrame.GetPayloadPtr(), sourceFrame.GetPayloadSize(), m_videoFrameDetector);
    else
      videoFrameType = OpalVideoFormat::e_UnknownFrameType;
#endif // OPAL_VIDEO
#endif // OPAL_STATISTICS

    if (!m_stream->WritePacket(sourceFrame))
      return false;

#if OPAL_STATISTICS
    RTP_SyncSourceId ssrc;
    if (audioFrameType != OpalAudioFormat::e_UnknownFrameType) {
      PWaitAndSignal mutex(m_statsMutex);

      AudioStats & allStats = m_audioStatistics[0];
      AudioStats * ssrcStats = (ssrc = sourceFrame.GetSyncSource()) != 0 ? &m_audioStatistics[ssrc] : NULL;

      if (audioFrameType&OpalAudioFormat::e_SilenceFrame) {
        ++allStats.m_silent;
        if (ssrcStats)
          ++ssrcStats->m_silent;
      }

      if (audioFrameType&OpalAudioFormat::e_FECFrame) {
        ++allStats.m_FEC;
        if (ssrcStats)
          ++ssrcStats->m_FEC;
      }
    }

#if OPAL_VIDEO
    switch (videoFrameType) {
      case OpalVideoFormat::e_IntraFrame :
        m_statsMutex.Wait();
        m_videoStatistics[0].IncrementFrames(true);
        if ((ssrc = sourceFrame.GetSyncSource()) != 0)
          m_videoStatistics[ssrc].IncrementFrames(true);
        PTRACE(4, "I-Frame detected: SSRC=" << RTP_TRACE_SRC(ssrc)
                << ", ts=" << sourceFrame.GetTimestamp() << ", total=" << m_videoStatistics[ssrc].m_totalFrames
                << ", key=" << m_videoStatistics[ssrc].m_keyFrames
                << ", req=" << m_videoStatistics[ssrc].m_lastUpdateRequestTime << ", on " << m_patch);
        m_statsMutex.Signal();
        break;

      case OpalVideoFormat::e_InterFrame :
        m_statsMutex.Wait();
        m_videoStatistics[0].IncrementFrames(false);
        if ((ssrc = sourceFrame.GetSyncSource()) != 0)
          m_videoStatistics[ssrc].IncrementFrames(false);
        PTRACE(5, "P-Frame detected: SSRC=" << RTP_TRACE_SRC(ssrc)
                << ", ts=" << sourceFrame.GetTimestamp() << ", total=" << m_videoStatistics[ssrc].m_totalFrames
                << ", key=" << m_videoStatistics[ssrc].m_keyFrames << ", on " << m_patch);
        m_statsMutex.Signal();
        break;

      default :
        break;
    }
#endif // OPAL_VIDEO
#endif // OPAL_STATISTICS

    PTRACE_IF(6, bypassing, "Bypassed packet " << setw(1) << sourceFrame);
    return true;
  }

  if (!m_primaryCodec->ConvertFrames(sourceFrame, m_intermediateFrames)) {
    PTRACE(1, "Media conversion (primary) failed");
    return false;
  }

  for (RTP_DataFrameList::iterator interFrame = m_intermediateFrames.begin(); interFrame != m_intermediateFrames.end(); ++interFrame) {
    m_patch.FilterFrame(*interFrame, m_primaryCodec->GetOutputFormat());

    if (m_secondaryCodec == NULL) {
      if (!m_stream->WritePacket(*interFrame))
        return false;
      if (m_primaryCodec == NULL)
        return true;
      m_primaryCodec->CopyTimestamp(sourceFrame, *interFrame, false);
      continue;
    }

    if (!m_secondaryCodec->ConvertFrames(*interFrame, m_finalFrames)) {
      PTRACE(1, "Media conversion (secondary) failed");
      return false;
    }

    for (RTP_DataFrameList::iterator finalFrame = m_finalFrames.begin(); finalFrame != m_finalFrames.end(); ++finalFrame) {
      m_patch.FilterFrame(*finalFrame, m_secondaryCodec->GetOutputFormat());
      if (!m_stream->WritePacket(*finalFrame))
        return false;
      if (m_secondaryCodec == NULL)
        return true;
      m_secondaryCodec->CopyTimestamp(sourceFrame, *finalFrame, false);
    }
  }

#if OPAL_VIDEO && OPAL_STATISTICS
  OpalVideoTranscoder * videoCodec = dynamic_cast<OpalVideoTranscoder *>(m_primaryCodec);
  if (videoCodec != NULL && !m_intermediateFrames.IsEmpty()) {
    PWaitAndSignal mutex(m_statsMutex);
    m_videoStatistics[0].IncrementFrames(videoCodec->WasLastFrameIFrame());
  }
#endif // OPAL_VIDEO && OPAL_STATISTICS

  return true;
}


/////////////////////////////////////////////////////////////////////////////

OpalPassiveMediaPatch::OpalPassiveMediaPatch(OpalMediaStream & source)
  : OpalMediaPatch(source)
  , m_started(false)
{
}


void OpalPassiveMediaPatch::Start()
{
  if (m_started)
    return;

  if (CanStart()) {
    m_started = true;
    PTRACE(4, "Passive media patch started: " << *this);
    m_source.GetConnection().GetEndPoint().GetManager().QueueDecoupledEvent(new PSafeWorkNoArg<OpalMediaPatch, bool>(this, &OpalMediaPatch::OnStartMediaPatch));
  }
}


void OpalPassiveMediaPatch::Close()
{
  OpalMediaPatch::Close();

  if (m_started) {
    m_started = false;
    m_source.OnStopMediaPatch(*this);
  }
}
