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
 * $Revision$
 * $Author$
 * $Date$
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

#define new PNEW


/////////////////////////////////////////////////////////////////////////////

OpalMediaPatch::OpalMediaPatch(OpalMediaStream & src)
  : m_source(src)
  , m_bypassToPatch(NULL)
  , m_bypassFromPatch(NULL)
  , m_patchThread(NULL)
  , m_transcoderChanged(false)
{
  PTRACE_CONTEXT_ID_FROM(src);

  PTRACE(5, "Patch\tCreated media patch " << this << ", session " << src.GetSessionID());
  src.SetPatch(this);
  m_source.SafeReference();
}


OpalMediaPatch::~OpalMediaPatch()
{
  StopThread();
  m_source.SafeDereference();
  PTRACE(5, "Patch\tDestroyed media patch " << this);
}


void OpalMediaPatch::PrintOn(ostream & strm) const
{
  strm << GetClass() << '[' << this << "] " << m_source;

  if (!LockReadOnly())
    return;

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

  UnlockReadOnly();
}


bool OpalMediaPatch::CanStart() const
{
  if (!m_source.IsOpen()) {
    PTRACE(4, "Media\tDelaying patch start till source stream open: " << *this);
    return false;
  }

  if (m_sinks.IsEmpty()) {
    PTRACE(4, "Media\tDelaying patch start till have sink stream: " << *this);
    return false;
  }

  for (PList<Sink>::const_iterator s = m_sinks.begin(); s != m_sinks.end(); ++s) {
    if (!s->m_stream->IsOpen()) {
      PTRACE(4, "Media\tDelaying patch start till sink stream open: " << *this);
      return false;
    }
  }

  PSafePtr<OpalRTPConnection> connection = dynamic_cast<OpalRTPConnection *>(&m_source.GetConnection());
  if (connection == NULL)
    connection = m_source.GetConnection().GetOtherPartyConnectionAs<OpalRTPConnection>();
  if (connection == NULL) {
    PTRACE(4, "Media\tAllow patch start as connection not RTP: " << *this);
    return true;
	}

  OpalMediaSession * session = connection->GetMediaSession(m_source.GetSessionID());
  if (session == NULL) {
    PTRACE(4, "Media\tAllow patch start as does not have session " << session->GetSessionID() << ": " << *this);
    return true;
	}

  if (session->IsOpen())
    return true;

  PTRACE(4, "Media\tDelaying patch start till session " << session->GetSessionID() << " open: " << *this);
  return false;
}


void OpalMediaPatch::Start()
{
  PWaitAndSignal m(m_patchThreadMutex);
	
  if(m_patchThread != NULL) {
    PTRACE(5, "Media\tAlready started thread " << m_patchThread->GetThreadName());
    return;
  }

  if (CanStart()) {
    m_patchThread = new PThreadObj<OpalMediaPatch>(*this, &OpalMediaPatch::Main, false, "Media Patch", PThread::HighPriority);
    PTRACE_CONTEXT_ID_TO(m_patchThread);
    PThread::Yield();
    PTRACE(4, "Media\tStarting thread " << m_patchThread->GetThreadName());
  }
}


void OpalMediaPatch::StopThread()
{
  m_patchThreadMutex.Wait();
  PThread * thread = m_patchThread;
  m_patchThread = NULL;
  m_patchThreadMutex.Signal();

  if (thread == NULL)
    return;

  if (!thread->IsSuspended()) {
    PTRACE(4, "Patch\tWaiting for media patch thread to stop " << *this);
    PAssert(thread->WaitForTermination(10000), "Media patch thread not terminated.");
  }

  delete thread;
}


void OpalMediaPatch::Close()
{
  PTRACE(3, "Patch\tClosing media patch " << *this);

  if (!LockReadWrite())
    return;

  if (m_bypassFromPatch != NULL)
    m_bypassFromPatch->SetBypassPatch(NULL);
  else
    SetBypassPatch(NULL);

  m_filters.RemoveAll();
  if (m_source.GetPatch() == this) {
    UnlockReadWrite();
    m_source.Close();
    if (!LockReadWrite())
      return;
  }

  while (m_sinks.GetSize() > 0) {
    OpalMediaStreamPtr stream = m_sinks.front().m_stream;
    UnlockReadWrite();
    if (stream == NULL || !stream->Close()) {
      // The only way we can get here is if the sink is in the proccess of being closed
      // but is blocked on the mutex waiting to remove the sink from this patch.
      // Se we unlock it, and wait for it to do it in the other thread.
      PThread::Sleep(10);
    }
    if (!LockReadWrite())
      return;
  }
  UnlockReadWrite();

  StopThread();
}


PBoolean OpalMediaPatch::AddSink(const OpalMediaStreamPtr & sinkStream)
{
  PSafeLockReadWrite mutex(*this);

  if (PAssertNULL(sinkStream) == NULL)
    return false;

  PAssert(sinkStream->IsSink(), "Attempt to set source stream as sink!");

  if (!sinkStream->SetPatch(this)) {
    PTRACE(2, "Patch\tCould not set patch in stream " << *sinkStream);
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
  PSafeLockReadWrite mutex(*this);

  for (PList<Sink>::iterator s = m_sinks.begin(); s != m_sinks.end(); ++s) {
    if (!s->CreateTranscoders())
      return false;
  }

  m_transcoderChanged = true;
  return true;
}


static bool SetStreamDataSize(OpalMediaStream & stream, OpalTranscoder & codec)
{
  OpalMediaFormat format = stream.IsSource() ? codec.GetOutputFormat() : codec.GetInputFormat();
  PINDEX size = codec.GetOptimalDataFrameSize(stream.IsSource());
  if (stream.SetDataSize(size, format.GetFrameTime()*stream.GetMediaFormat().GetClockRate()/format.GetClockRate()))
    return true;

  PTRACE(1, "Patch\tStream " << stream << " cannot support data size " << size);
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

  PTRACE(5, "Patch\tAddSink\n"
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
#if OPAL_VIDEO
    if (sourceFormat.GetMediaType() == OpalMediaType::Video())
      m_videoFormat = sourceFormat;
#endif // OPAL_VIDEO
    PTRACE(3, "Patch\tAdded direct media stream sink " << *m_stream);
    return true;
  }

  PString id = m_stream->GetID();
  m_primaryCodec = OpalTranscoder::Create(sourceFormat, destinationFormat, (const BYTE *)id, id.GetLength());
  if (m_primaryCodec != NULL) {
    PTRACE_CONTEXT_ID_TO(m_primaryCodec);
    PTRACE(4, "Patch\tCreated primary codec " << sourceFormat << "->" << destinationFormat << " with ID " << id);

    if (!SetStreamDataSize(*m_stream, *m_primaryCodec))
      return false;
    m_primaryCodec->SetMaxOutputSize(m_stream->GetDataSize());
    m_primaryCodec->SetSessionID(m_patch.m_source.GetSessionID());
    m_primaryCodec->SetCommandNotifier(PCREATE_NOTIFIER_EXT(&m_patch, OpalMediaPatch, InternalOnMediaCommand1));

    if (!SetStreamDataSize(m_patch.m_source, *m_primaryCodec))
      return false;
    m_patch.m_source.InternalUpdateMediaFormat(m_primaryCodec->GetInputFormat());
    m_stream->InternalUpdateMediaFormat(m_primaryCodec->GetOutputFormat());

    PTRACE(3, "Patch\tAdded media stream sink " << *m_stream
           << " using transcoder " << *m_primaryCodec << ", data size=" << m_stream->GetDataSize());
    return true;
  }

  PTRACE(4, "Patch\tCreating two stage transcoders for " << sourceFormat << "->" << destinationFormat << " with ID " << id);
  OpalMediaFormat intermediateFormat;
  if (!OpalTranscoder::FindIntermediateFormat(sourceFormat, destinationFormat, intermediateFormat)) {
    PTRACE(1, "Patch\tCould find compatible media format for " << *m_stream);
    return false;
  }

  if (intermediateFormat.GetMediaType() == OpalMediaType::Audio()) {
    // try prepare intermediateFormat for correct frames to frames transcoding
    // so we need make sure that tx frames time of destinationFormat be equal 
    // to tx frames time of intermediateFormat (all this does not produce during
    // Merge phase in FindIntermediateFormat)
    int destinationPacketTime = destinationFormat.GetFrameTime()*destinationFormat.GetOptionInteger(OpalAudioFormat::TxFramesPerPacketOption(), 1);
    if ((destinationPacketTime % intermediateFormat.GetFrameTime()) != 0) {
      PTRACE(1, "Patch\tCould produce without buffered media format converting (which not implemented yet) for " << *m_stream);
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
  PTRACE(3, "Patch\tCreated two stage codec " << sourceFormat << "/" << intermediateFormat << "/" << destinationFormat << " with ID " << id);

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

  PTRACE(3, "Patch\tAdded media stream sink " << *m_stream
          << " using transcoders " << *m_primaryCodec
          << " and " << *m_secondaryCodec << ", data size=" << m_stream->GetDataSize());
  return true;
}


void OpalMediaPatch::RemoveSink(const OpalMediaStream & stream)
{
  PTRACE(3, "Patch\tRemoving sink " << stream << " from " << *this);

  bool closeSource = false;

  if (!LockReadWrite())
    return;

  for (PList<Sink>::iterator s = m_sinks.begin(); s != m_sinks.end(); ++s) {
    if (s->m_stream == &stream) {
      m_sinks.erase(s);
      PTRACE(5, "Patch\tRemoved sink " << stream << " from " << *this);
      break;
    }
  }

  if (m_sinks.IsEmpty()) {
    closeSource = true;
    if (m_bypassFromPatch != NULL)
      m_bypassFromPatch->SetBypassPatch(NULL);
  }

  UnlockReadWrite();

  if (closeSource  && m_source.GetPatch() == this)
    m_source.Close();
}


OpalMediaStreamPtr OpalMediaPatch::GetSink(PINDEX i) const
{
  PSafeLockReadOnly mutex(*this);
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
  if (!LockReadOnly())
    return NULL;

  if (i >= m_sinks.GetSize()) {
    UnLockSinkTranscoder();
    return NULL;
  }

  Sink & sink = m_sinks[i];
  if (sink.m_secondaryCodec != NULL) 
    return sink.m_secondaryCodec;

  if (sink.m_primaryCodec != NULL)
    return sink.m_primaryCodec;

  UnLockSinkTranscoder();

  return NULL;
}


void OpalMediaPatch::UnLockSinkTranscoder() const
{
  UnlockReadOnly();
}


#if OPAL_STATISTICS
void OpalMediaPatch::GetStatistics(OpalMediaStatistics & statistics, bool fromSink) const
{
  if (!LockReadOnly())
    return;

  if (fromSink)
    m_source.GetStatistics(statistics, true);

  if (!m_sinks.IsEmpty())
    m_sinks.front().GetStatistics(statistics, !fromSink);

  UnlockReadOnly();
}


void OpalMediaPatch::Sink::GetStatistics(OpalMediaStatistics & statistics, bool fromSource) const
{
  if (fromSource)
    m_stream->GetStatistics(statistics, true);

  if (m_primaryCodec != NULL)
    m_primaryCodec->GetStatistics(statistics);

  if (m_secondaryCodec != NULL)
    m_secondaryCodec->GetStatistics(statistics);

#if OPAL_VIDEO
  if (m_videoFrames > 0 || statistics.m_totalFrames == 0)
    statistics.m_totalFrames = m_videoFrames;
  if (m_keyFrames > 0 || statistics.m_keyFrames == 0)
    statistics.m_keyFrames = m_keyFrames;
#endif
}
#endif // OPAL_STATISTICS


OpalMediaPatch::Sink::Sink(OpalMediaPatch & p, const OpalMediaStreamPtr & s)
  : m_patch(p)
  , m_stream(s)
  , m_primaryCodec(NULL)
  , m_secondaryCodec(NULL)
  , m_writeSuccessful(true)
#if OPAL_VIDEO
  , m_rateController(NULL)
  , m_videoFrames(0)
  , m_keyFrames(0)
#endif
{
  PTRACE_CONTEXT_ID_FROM(p);

#if OPAL_VIDEO
  SetRateControlParameters(m_stream->GetMediaFormat());
#endif

  PTRACE(3, "Patch\tCreated Sink for " << p);
}


OpalMediaPatch::Sink::~Sink()
{
  delete m_primaryCodec;
  delete m_secondaryCodec;
#if OPAL_VIDEO
  delete m_rateController;
#endif
}


void OpalMediaPatch::AddFilter(const PNotifier & filter, const OpalMediaFormat & stage)
{
  PSafeLockReadWrite mutex(*this);

  if (m_source.GetMediaFormat().GetMediaType() != stage.GetMediaType())
    return;

  // ensures that a filter is added only once
  for (PList<Filter>::iterator f = m_filters.begin(); f != m_filters.end(); ++f) {
    if (f->m_notifier == filter && f->m_stage == stage) {
      PTRACE(4, "OpalCon\tFilter already added for stage " << stage);
      return;
    }
  }
  m_filters.Append(new Filter(filter, stage));
}


PBoolean OpalMediaPatch::RemoveFilter(const PNotifier & filter, const OpalMediaFormat & stage)
{
  PSafeLockReadWrite mutex(*this);

  for (PList<Filter>::iterator f = m_filters.begin(); f != m_filters.end(); ++f) {
    if (f->m_notifier == filter && f->m_stage == stage) {
      m_filters.erase(f);
      return true;
    }
  }

  PTRACE(4, "OpalCon\tNo filter to remove for stage " << stage);
  return false;
}


void OpalMediaPatch::FilterFrame(RTP_DataFrame & frame,
                                 const OpalMediaFormat & mediaFormat)
{
  if (!LockReadOnly())
    return;

  for (PList<Filter>::iterator f = m_filters.begin(); f != m_filters.end(); ++f) {
    if (f->m_stage.IsEmpty() || f->m_stage == mediaFormat)
      f->m_notifier(frame, (P_INT_PTR)this);
  }

  UnlockReadOnly();
}


bool OpalMediaPatch::UpdateMediaFormat(const OpalMediaFormat & mediaFormat)
{
  PSafeLockReadOnly mutex(*this);

  bool atLeastOne = m_source.InternalUpdateMediaFormat(mediaFormat);

  for (PList<Sink>::iterator s = m_sinks.begin(); s != m_sinks.end(); ++s) {
    if (s->UpdateMediaFormat(mediaFormat)) {
      m_source.InternalUpdateMediaFormat(s->m_stream->GetMediaFormat());
      atLeastOne = true;
    }
  }

  PTRACE_IF(2, !atLeastOne, "Patch\tCould not update media format for any stream/transcoder in " << *this);
  return atLeastOne;
}


PBoolean OpalMediaPatch::ExecuteCommand(const OpalMediaCommand & command)
{
  bool atLeastOne = false;

  if (!LockReadOnly())
    return false;

  PSafePtr<OpalMediaPatch> fromPatch;
  if (m_bypassFromPatch != NULL) // Don't use tradic ?: as GNU doesn't like it
    fromPatch = m_bypassFromPatch;
  else
    fromPatch = this;

  PSafePtr<OpalMediaPatch> toPatch;
  if (m_bypassToPatch != NULL) // Don't use tradic ?: as GNU doesn't like it
    toPatch = m_bypassToPatch;
  else
    toPatch = this;

  UnlockReadOnly();


  if (fromPatch.SetSafetyMode(PSafeReadOnly)) {
    atLeastOne = fromPatch->m_source.InternalExecuteCommand(command);
    fromPatch.SetSafetyMode(PSafeReference);
  }

  if (toPatch.SetSafetyMode(PSafeReadOnly)) {
    for (PList<Sink>::iterator s = toPatch->m_sinks.begin(); s != toPatch->m_sinks.end(); ++s) {
      if (s->ExecuteCommand(command))
        atLeastOne = true;
    }
    toPatch.SetSafetyMode(PSafeReference);
  }

#if PTRACING
  if (PTrace::CanTrace(5)) {
    ostream & trace = PTrace::Begin(5, __FILE__, __LINE__, this);
    trace << "Patch\tExecute" << (atLeastOne ? "d" : "fail for ")
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
  PSafeLockReadOnly mutex(*this);

  bool atLeastOne = m_source.InternalSetPaused(pause, fromUser, true);

  for (PList<Sink>::iterator s = m_sinks.begin(); s != m_sinks.end(); ++s) {
    if (s->m_stream->InternalSetPaused(pause, fromUser, true))
      atLeastOne = true;
  }

  return atLeastOne;
}


bool OpalMediaPatch::OnStartMediaPatch()
{
  m_source.OnStartMediaPatch();

  if (m_source.IsSynchronous())
    return false;

  return EnableJitterBuffer();
}


bool OpalMediaPatch::EnableJitterBuffer(bool enab)
{
  PSafeLockReadOnly mutex(*this);

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
  PTRACE(4, "Patch\tThread started for " << *this);
	
  bool asynchronous = OnStartMediaPatch();
  PAdaptiveDelay asynchPacing;
  PThread::Times lastThreadTimes;
  PTimeInterval lastTick;

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

    sourceFrame.MakeUnique();
    sourceFrame.SetPayloadType(m_source.GetMediaFormat().GetPayloadType());

    // We do the following to make sure that the buffer size is large enough,
    // in case something in previous loop adjusted it
    sourceFrame.SetPayloadSize(m_source.GetDataSize());
    sourceFrame.SetPayloadSize(0);

    if (!m_source.ReadPacket(sourceFrame)) {
      PTRACE(4, "Patch\tThread ended because source read failed");
      break;
    }
 
    if (!DispatchFrame(sourceFrame)) {
      PTRACE(4, "Patch\tThread ended because all sink writes failed");
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
#if P_CONFIG_FILE
    static const unsigned SampleTimeMS = PConfig(PConfig::Environment).GetInteger("OPAL_MEDIA_PATCH_CPU_CHECK", 1000);
#else
    static const unsigned SampleTimeMS = 1000;
#endif
    static const unsigned ThresholdPercent = 90;
    if (PTimer::Tick() - lastTick > SampleTimeMS) {
      PThread::Times threadTimes;
      if (PThread::Current()->GetTimes(threadTimes)) {
        PTRACE(5, "Patch\tCPU for " << *this << " is " << threadTimes);
        if ((threadTimes.m_user - lastThreadTimes.m_user + threadTimes.m_kernel - lastThreadTimes.m_kernel)
                                      > (threadTimes.m_real - lastThreadTimes.m_real)*ThresholdPercent/100) {
          PTRACE(2, "Patch\tGreater that 90% CPU usage for " << *this);
          PThread::Sleep(SampleTimeMS*(100-ThresholdPercent)/100);
        }
        lastThreadTimes = threadTimes;
      }
      lastTick = PTimer::Tick();
    }
  }

  m_source.OnStopMediaPatch(*this);

  if (m_sinks.IsEmpty())
   m_source.GetConnection().GetEndPoint().GetManager().QueueDecoupledEvent(
                new PSafeWorkArg1<OpalConnection, OpalMediaStreamPtr, bool>(&m_source.GetConnection(),
                                                        &m_source, &OpalConnection::CloseMediaStream));

  PTRACE(4, "Patch\tThread ended for " << *this);
}


bool OpalMediaPatch::SetBypassPatch(const OpalMediaPatchPtr & patch)
{
  PSafeLockReadWrite mutex(*this);

  if (!PAssert(m_bypassFromPatch == NULL, PLogicError))
    return false; // Can't be both!

  if (m_bypassToPatch == patch)
    return true; // Already set

  PTRACE(4, "Patch\tSetting media patch bypass to " << patch << " on " << *this);

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
  if (!LockReadOnly())
    return false;

  if (m_transcoderChanged) {
    m_transcoderChanged = false;
    UnlockReadOnly();
    PTRACE(3, "Patch\tIgnoring source data with transcoder change on " << *this);
    return true;
  }

  if (m_bypassFromPatch != NULL) {
    PTRACE(3, "Patch\tMedia patch bypass started by " << *m_bypassFromPatch << " on " << *this);
    UnlockReadOnly();
    m_bypassEnded.Wait();
    PTRACE(4, "Patch\tMedia patch bypass ended on " << *this);
    return true;
  }

  FilterFrame(frame, m_source.GetMediaFormat());

  OpalMediaPatchPtr patch = m_bypassToPatch;
  if (patch == NULL)
    patch = this;

  UnlockReadOnly();

  bool written = false;
  PSafeLockReadOnly guard(*patch);
  for (PList<Sink>::iterator s = patch->m_sinks.begin(); s != patch->m_sinks.end(); ++s) {
    if (s->WriteFrame(frame, patch != this))
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

#if OPAL_VIDEO
    SetRateControlParameters(m_stream->GetMediaFormat());
#endif

  PTRACE(3, "Patch\tUpdated Sink: format=" << mediaFormat << " ok=" << ok);
  return ok;
}


bool OpalMediaPatch::Sink::ExecuteCommand(const OpalMediaCommand & command)
{
  bool atLeastOne = m_stream->InternalExecuteCommand(command);

  if (m_secondaryCodec != NULL)
    atLeastOne = m_secondaryCodec->ExecuteCommand(command) || atLeastOne;

  if (m_primaryCodec != NULL)
    atLeastOne = m_primaryCodec->ExecuteCommand(command) || atLeastOne;

  return atLeastOne;
}


#if OPAL_VIDEO
void OpalMediaPatch::Sink::SetRateControlParameters(const OpalMediaFormat & mediaFormat)
{
  if ((mediaFormat.GetMediaType() == OpalMediaType::Video()) && mediaFormat != OpalYUV420P) {
    m_rateController = NULL;
    PString rc = mediaFormat.GetOptionString(OpalVideoFormat::RateControllerOption());
    if (!rc.IsEmpty()) {
      m_rateController = PFactory<OpalVideoRateController>::CreateInstance(rc);
      if (m_rateController != NULL) {   
        PTRACE(3, "Patch\tCreated " << rc << " rate controller");
      }
      else {
        PTRACE(3, "Patch\tCould not create " << rc << " rate controller");
      }
    }
  }

  if (m_rateController != NULL) 
    m_rateController->Open(mediaFormat);
}


bool OpalMediaPatch::Sink::RateControlExceeded(bool & forceIFrame)
{
  if ((m_rateController == NULL) || !m_rateController->SkipFrame(forceIFrame)) 
    return false;

  PTRACE(4, "Patch\tRate controller skipping frame.");
  return true;
}

#endif


bool OpalMediaPatch::Sink::WriteFrame(RTP_DataFrame & sourceFrame, bool bypassing)
{
  if (!m_writeSuccessful)
    return false;
  
  if (m_stream->IsPaused())
    return true;

#if OPAL_VIDEO
  if (m_rateController != NULL) {
    bool forceIFrame = false;
    bool s = RateControlExceeded(forceIFrame);
    if (forceIFrame)
      m_stream->ExecuteCommand(OpalVideoUpdatePicture());
    if (s) {
      if (m_secondaryCodec == NULL) {
        bool wasIFrame = false;
        if (m_rateController->Pop(m_intermediateFrames, wasIFrame, false)) {
        PTRACE(3, "RC returned " << m_intermediateFrames.GetSize() << " packets");
          for (RTP_DataFrameList::iterator interFrame = m_intermediateFrames.begin(); interFrame != m_intermediateFrames.end(); ++interFrame) {
            m_patch.FilterFrame(*interFrame, m_primaryCodec->GetOutputFormat());
            if (!m_stream->WritePacket(*interFrame))
              return (m_writeSuccessful = false);
            sourceFrame.SetTimestamp(interFrame->GetTimestamp());
            continue;
          }
          m_intermediateFrames.RemoveAll();
        }
      }
      return true;
    }
  }
#endif // OPAL_VIDEO

  if (bypassing || m_primaryCodec == NULL) {
#if OPAL_VIDEO
    if (m_videoFormat.IsValid()) {
      switch (m_videoFormat.GetVideoFrameType(sourceFrame.GetPayloadPtr(), sourceFrame.GetPayloadSize(), m_keyFrameDetectContext)) {
      case OpalVideoFormat::e_IntraFrame :
          ++m_videoFrames;
          ++m_keyFrames;
          PTRACE(4, "Patch\tI-Frame detected: SSRC=" << RTP_TRACE_SRC(sourceFrame.GetSyncSource())
                 << ", ts=" << sourceFrame.GetTimestamp() << ", total=" << m_videoFrames << ", key=" << m_keyFrames << ", on " << m_patch);
          break;

        case OpalVideoFormat::e_InterFrame :
          ++m_videoFrames;
          PTRACE(5, "Patch\tP-Frame detected SSRC=" << RTP_TRACE_SRC(sourceFrame.GetSyncSource())
                 << ", ts=" << sourceFrame.GetTimestamp() << ", total=" << m_videoFrames << ", key=" << m_keyFrames << ", on " << m_patch);
          break;

        default :
          break;
      }
    }
#endif // OPAL_VIDEO

    m_writeSuccessful = m_stream->WritePacket(sourceFrame);
    if (!m_writeSuccessful)
      return false;

    PTRACE_IF(6, bypassing, "Patch\tBypassed packet "
                         << " M="  << sourceFrame.GetMarker()
                         << " PT=" << sourceFrame.GetPayloadType()
                         << " SN=" << sourceFrame.GetSequenceNumber()
                         << " TS=" << sourceFrame.GetTimestamp()
                         << " SSRC=" << RTP_TRACE_SRC(sourceFrame.GetSyncSource())
                         << " P-SZ=" << sourceFrame.GetPayloadSize());
    return true;
  }

  if (!m_primaryCodec->ConvertFrames(sourceFrame, m_intermediateFrames)) {
    PTRACE(1, "Patch\tMedia conversion (primary) failed");
    return false;
  }

#if OPAL_VIDEO
  if (m_secondaryCodec == NULL && m_rateController != NULL) {
    PTRACE(4, "Patch\tPushing " << m_intermediateFrames.GetSize() << " packet into RC");
    m_rateController->Push(m_intermediateFrames, ((OpalVideoTranscoder *)m_primaryCodec)->WasLastFrameIFrame());
    bool wasIFrame = false;
    if (m_rateController->Pop(m_intermediateFrames, wasIFrame, false)) {
      PTRACE(4, "Patch\tPulled " << m_intermediateFrames.GetSize() << " frames from RC");
      for (RTP_DataFrameList::iterator interFrame = m_intermediateFrames.begin(); interFrame != m_intermediateFrames.end(); ++interFrame) {
        m_patch.FilterFrame(*interFrame, m_primaryCodec->GetOutputFormat());
        if (!m_stream->WritePacket(*interFrame))
          return (m_writeSuccessful = false);
        if (m_primaryCodec == NULL)
          return true;
        m_primaryCodec->CopyTimestamp(sourceFrame, *interFrame, false);
        continue;
      }
      m_intermediateFrames.RemoveAll();
    }
  }
  else 
#endif // OPAL_VIDEO
  for (RTP_DataFrameList::iterator interFrame = m_intermediateFrames.begin(); interFrame != m_intermediateFrames.end(); ++interFrame) {
    m_patch.FilterFrame(*interFrame, m_primaryCodec->GetOutputFormat());

    if (m_secondaryCodec == NULL) {
      if (!m_stream->WritePacket(*interFrame))
        return (m_writeSuccessful = false);
      if (m_primaryCodec == NULL)
        return true;
      m_primaryCodec->CopyTimestamp(sourceFrame, *interFrame, false);
      continue;
    }

    if (!m_secondaryCodec->ConvertFrames(*interFrame, m_finalFrames)) {
      PTRACE(1, "Patch\tMedia conversion (secondary) failed");
      return false;
    }

    for (RTP_DataFrameList::iterator finalFrame = m_finalFrames.begin(); finalFrame != m_finalFrames.end(); ++finalFrame) {
      m_patch.FilterFrame(*finalFrame, m_secondaryCodec->GetOutputFormat());
      if (!m_stream->WritePacket(*finalFrame))
        return (m_writeSuccessful = false);
      if (m_secondaryCodec == NULL)
        return true;
      m_secondaryCodec->CopyTimestamp(sourceFrame, *finalFrame, false);
    }
  }

#if OPAL_VIDEO
  //if (rcEnabled)
  //  rateController.AddFrame(totalPayloadSize, frameCount);
#endif

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
    PTRACE(4, "Patch\tPassive media patch started: " << *this);
    OnStartMediaPatch();
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
