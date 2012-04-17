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


#include <opal/buildopts.h>

#include <opal/patch.h>
#include <opal/mediastrm.h>
#include <opal/transcoders.h>

#if OPAL_VIDEO
#include <codec/vidcodec.h>
#endif

#define new PNEW


/////////////////////////////////////////////////////////////////////////////

OpalMediaPatch::OpalMediaPatch(OpalMediaStream & src)
  : source(src)
  , m_bypassToPatch(NULL)
  , m_bypassFromPatch(NULL)
  , patchThread(NULL)
{
  PTRACE_CONTEXT_ID_FROM(src);

  PTRACE(5, "Patch\tCreated media patch " << this << ", session " << src.GetSessionID());
  src.SetPatch(this);
}


OpalMediaPatch::~OpalMediaPatch()
{
  StopThread();
  PTRACE(5, "Patch\tDestroyed media patch " << this);
}


void OpalMediaPatch::PrintOn(ostream & strm) const
{
  strm << "Patch[" << this << "] " << source;

  if (!LockReadOnly())
    return;

  if (sinks.GetSize() > 0) {
    strm << " -> ";
    if (sinks.GetSize() == 1)
      strm << *sinks.front().stream;
    else {
      PINDEX i = 0;
      for (PList<Sink>::const_iterator s = sinks.begin(); s != sinks.end(); ++s,++i) {
        if (i > 0)
          strm << ", ";
        strm << "sink[" << i << "]=" << *s->stream;
      }
    }
  }

  UnlockReadOnly();
}

void OpalMediaPatch::Start()
{
  PWaitAndSignal m(patchThreadMutex);
	
  if(patchThread != NULL) {
    PTRACE(5, "Media\tAlready started thread " << patchThread->GetThreadName());
    return;
  }

  if (!source.IsOpen()) {
    PTRACE(4, "Media\tDelaying thread starting till source stream open");
    return;
  }

  if (sinks.IsEmpty()) {
    PTRACE(4, "Media\tDelaying thread starting till have sink stream");
    return;
  }

  for (PList<Sink>::iterator s = sinks.begin(); s != sinks.end(); ++s) {
    if (!s->stream->IsOpen()) {
      PTRACE(4, "Media\tDelaying thread starting till sink stream open");
      return;
    }
  }

  patchThread = new Thread(*this);
  patchThread->Resume();
  PThread::Yield();
  PTRACE(4, "Media\tStarting thread " << patchThread->GetThreadName());
}


void OpalMediaPatch::StopThread()
{
  patchThreadMutex.Wait();
  PThread * thread = patchThread;
  patchThread = NULL;
  patchThreadMutex.Signal();

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

  filters.RemoveAll();
  if (source.GetPatch() == this)
    source.Close();

  while (sinks.GetSize() > 0) {
    OpalMediaStreamPtr stream = sinks.front().stream;
    UnlockReadWrite();
    if (!stream->Close()) {
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
  sinks.Append(sink);
  if (!sink->CreateTranscoders())
    return false;

  EnableJitterBuffer();
  return true;
}


bool OpalMediaPatch::ResetTranscoders()
{
  PSafeLockReadWrite mutex(*this);

  for (PList<Sink>::iterator s = sinks.begin(); s != sinks.end(); ++s) {
    if (!s->CreateTranscoders())
      return false;
  }

  return true;
}


bool OpalMediaPatch::Sink::CreateTranscoders()
{
  delete primaryCodec;
  primaryCodec =  NULL;
  delete secondaryCodec;
  secondaryCodec = NULL;

  // Find the media formats than can be used to get from source to sink
  OpalMediaFormat sourceFormat = patch.source.GetMediaFormat();
  OpalMediaFormat destinationFormat = stream->GetMediaFormat();

  PTRACE(5, "Patch\tAddSink\n"
            "Source format:\n" << setw(-1) << sourceFormat << "\n"
            "Destination format:\n" << setw(-1) << destinationFormat);

  if (sourceFormat == destinationFormat) {
    PINDEX framesPerPacket = destinationFormat.GetOptionInteger(OpalAudioFormat::TxFramesPerPacketOption(),
                                  sourceFormat.GetOptionInteger(OpalAudioFormat::TxFramesPerPacketOption(), 1));
    PINDEX packetSize = sourceFormat.GetFrameSize()*framesPerPacket;
    PINDEX packetTime = sourceFormat.GetFrameTime()*framesPerPacket;
    patch.source.SetDataSize(packetSize, packetTime);
    stream->SetDataSize(packetSize, packetTime);
    stream->InternalUpdateMediaFormat(stream->GetMediaFormat());
    patch.source.InternalUpdateMediaFormat(patch.source.GetMediaFormat());
    PTRACE(3, "Patch\tAdded direct media stream sink " << *stream);
    return true;
  }

  PString id = stream->GetID();
  primaryCodec = OpalTranscoder::Create(sourceFormat, destinationFormat, (const BYTE *)id, id.GetLength());
  if (primaryCodec != NULL) {
    PTRACE_CONTEXT_ID_TO(primaryCodec);
    PTRACE(4, "Patch\tCreated primary codec " << sourceFormat << "->" << destinationFormat << " with ID " << id);

    if (!stream->SetDataSize(primaryCodec->GetOptimalDataFrameSize(false), sourceFormat.GetFrameTime())) {
      PTRACE(1, "Patch\tSink stream " << *stream << " cannot support data size "
              << primaryCodec->GetOptimalDataFrameSize(PFalse));
      return false;
    }
    primaryCodec->SetMaxOutputSize(stream->GetDataSize());
    primaryCodec->SetSessionID(patch.source.GetSessionID());
    primaryCodec->SetCommandNotifier(PCREATE_NOTIFIER_EXT(&patch, OpalMediaPatch, OnMediaCommand));

    patch.source.SetDataSize(primaryCodec->GetOptimalDataFrameSize(true), destinationFormat.GetFrameTime());
    patch.source.InternalUpdateMediaFormat(primaryCodec->GetInputFormat());
    stream->InternalUpdateMediaFormat(primaryCodec->GetOutputFormat());

    PTRACE(3, "Patch\tAdded media stream sink " << *stream
           << " using transcoder " << *primaryCodec << ", data size=" << stream->GetDataSize());
    return true;
  }

  PTRACE(4, "Patch\tCreating two stage transcoders for " << sourceFormat << "->" << destinationFormat << " with ID " << id);
  OpalMediaFormat intermediateFormat;
  if (!OpalTranscoder::FindIntermediateFormat(sourceFormat, destinationFormat, intermediateFormat)) {
    PTRACE(1, "Patch\tCould find compatible media format for " << *stream);
    return false;
  }

  if (intermediateFormat.GetMediaType() == OpalMediaType::Audio()) {
    // try prepare intermediateFormat for correct frames to frames transcoding
    // so we need make sure that tx frames time of destinationFormat be equal 
    // to tx frames time of intermediateFormat (all this does not produce during
    // Merge phase in FindIntermediateFormat)
    int destinationPacketTime = destinationFormat.GetFrameTime()*destinationFormat.GetOptionInteger(OpalAudioFormat::TxFramesPerPacketOption(), 1);
    if ((destinationPacketTime % intermediateFormat.GetFrameTime()) != 0) {
      PTRACE(1, "Patch\tCould produce without buffered media format converting (which not implemented yet) for " << *stream);
      return false;
    }
    intermediateFormat.AddOption(new OpalMediaOptionUnsigned(OpalAudioFormat::TxFramesPerPacketOption(),
                                                              true,
                                                              OpalMediaOption::NoMerge,
                                                              destinationPacketTime/intermediateFormat.GetFrameTime()),
                                  true);
  }

  primaryCodec = OpalTranscoder::Create(sourceFormat, intermediateFormat, (const BYTE *)id, id.GetLength());
  secondaryCodec = OpalTranscoder::Create(intermediateFormat, destinationFormat, (const BYTE *)id, id.GetLength());
  if (primaryCodec == NULL || secondaryCodec == NULL)
    return false;

  PTRACE_CONTEXT_ID_TO(primaryCodec);
  PTRACE_CONTEXT_ID_TO(secondaryCodec);
  PTRACE(3, "Patch\tCreated two stage codec " << sourceFormat << "/" << intermediateFormat << "/" << destinationFormat << " with ID " << id);

  primaryCodec->SetMaxOutputSize(secondaryCodec->GetOptimalDataFrameSize(true));
  primaryCodec->SetSessionID(patch.source.GetSessionID());
  primaryCodec->SetCommandNotifier(PCREATE_NOTIFIER_EXT(&patch, OpalMediaPatch, OnMediaCommand));
  primaryCodec->UpdateMediaFormats(OpalMediaFormat(), secondaryCodec->GetInputFormat());

  if (!stream->SetDataSize(secondaryCodec->GetOptimalDataFrameSize(false), sourceFormat.GetFrameTime())) {
    PTRACE(1, "Patch\tSink stream " << *stream << " cannot support data size "
            << secondaryCodec->GetOptimalDataFrameSize(PFalse));
    return false;
  }
  secondaryCodec->SetMaxOutputSize(stream->GetDataSize());
  secondaryCodec->SetSessionID(patch.source.GetSessionID());
  secondaryCodec->SetCommandNotifier(PCREATE_NOTIFIER_EXT(&patch, OpalMediaPatch, OnMediaCommand));
  secondaryCodec->UpdateMediaFormats(primaryCodec->GetInputFormat(), OpalMediaFormat());

  patch.source.SetDataSize(primaryCodec->GetOptimalDataFrameSize(true), destinationFormat.GetFrameTime());
  patch.source.InternalUpdateMediaFormat(primaryCodec->GetInputFormat());
  stream->InternalUpdateMediaFormat(secondaryCodec->GetOutputFormat());

  PTRACE(3, "Patch\tAdded media stream sink " << *stream
          << " using transcoders " << *primaryCodec
          << " and " << *secondaryCodec << ", data size=" << stream->GetDataSize());
  return true;
}


void OpalMediaPatch::RemoveSink(const OpalMediaStreamPtr & stream)
{
  if (PAssertNULL(stream) == NULL)
    return;

  PTRACE(3, "Patch\tRemoving media stream sink " << *stream);

  bool closeSource = false;

  if (!LockReadWrite())
    return;

  for (PList<Sink>::iterator s = sinks.begin(); s != sinks.end(); ++s) {
    if (s->stream == stream) {
      sinks.erase(s);
      PTRACE(5, "Patch\tRemoved media stream sink " << *stream);
      break;
    }
  }

  if (sinks.IsEmpty()) {
    closeSource = true;
    if (m_bypassFromPatch != NULL)
      m_bypassFromPatch->SetBypassPatch(NULL);
  }

  UnlockReadWrite();

  if (closeSource  && source.GetPatch() == this)
    source.Close();
}


OpalMediaStreamPtr OpalMediaPatch::GetSink(PINDEX i) const
{
  PSafeLockReadOnly mutex(*this);
  return i < sinks.GetSize() ? sinks[i].stream : OpalMediaStreamPtr();
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

  if (i >= sinks.GetSize()) {
    UnLockSinkTranscoder();
    return NULL;
  }

  Sink & sink = sinks[i];
  if (sink.secondaryCodec != NULL) 
    return sink.secondaryCodec;

  if (sink.primaryCodec != NULL)
    return sink.primaryCodec;

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
    source.GetStatistics(statistics, true);

  if (!sinks.IsEmpty())
    sinks.front().GetStatistics(statistics, !fromSink);

  UnlockReadOnly();
}


void OpalMediaPatch::Sink::GetStatistics(OpalMediaStatistics & statistics, bool fromSource) const
{
  if (fromSource)
    stream->GetStatistics(statistics, true);

  if (primaryCodec != NULL)
    primaryCodec->GetStatistics(statistics);

  if (secondaryCodec != NULL)
    secondaryCodec->GetStatistics(statistics);
}
#endif


OpalMediaPatch::Sink::Sink(OpalMediaPatch & p, const OpalMediaStreamPtr & s)
  : patch(p)
  , stream(s)
  , primaryCodec(NULL)
  , secondaryCodec(NULL)
  , writeSuccessful(true)
  , m_lastPayloadType(RTP_DataFrame::IllegalPayloadType)
  , m_consecutivePayloadTypeMismatches(0)
#if OPAL_VIDEO
  , rateController(NULL)
#endif
{
  PTRACE_CONTEXT_ID_FROM(p);

#if OPAL_VIDEO
  SetRateControlParameters(stream->GetMediaFormat());
#endif

  PTRACE(3, "Patch\tCreated Sink: format=" << stream->GetMediaFormat());
}


OpalMediaPatch::Sink::~Sink()
{
  delete primaryCodec;
  delete secondaryCodec;
#if OPAL_VIDEO
  delete rateController;
#endif
}


void OpalMediaPatch::AddFilter(const PNotifier & filter, const OpalMediaFormat & stage)
{
  PSafeLockReadWrite mutex(*this);

  if (source.GetMediaFormat().GetMediaType() != stage.GetMediaType())
    return;

  // ensures that a filter is added only once
  for (PList<Filter>::iterator f = filters.begin(); f != filters.end(); ++f) {
    if (f->notifier == filter && f->stage == stage) {
      PTRACE(3, "OpalCon\tFilter already added for stage " << stage);
      return;
    }
  }
  filters.Append(new Filter(filter, stage));
}


PBoolean OpalMediaPatch::RemoveFilter(const PNotifier & filter, const OpalMediaFormat & stage)
{
  PSafeLockReadWrite mutex(*this);

  for (PList<Filter>::iterator f = filters.begin(); f != filters.end(); ++f) {
    if (f->notifier == filter && f->stage == stage) {
      filters.erase(f);
      return true;
    }
  }

  PTRACE(3, "OpalCon\tNo filter to remove for stage " << stage);
  return false;
}


void OpalMediaPatch::FilterFrame(RTP_DataFrame & frame,
                                 const OpalMediaFormat & mediaFormat)
{
  if (!LockReadOnly())
    return;

  for (PList<Filter>::iterator f = filters.begin(); f != filters.end(); ++f) {
    if (f->stage.IsEmpty() || f->stage == mediaFormat)
      f->notifier(frame, (INT)this);
  }

  UnlockReadOnly();
}


bool OpalMediaPatch::UpdateMediaFormat(const OpalMediaFormat & mediaFormat)
{
  PSafeLockReadOnly mutex(*this);

  bool atLeastOne = source.InternalUpdateMediaFormat(mediaFormat);

  for (PList<Sink>::iterator s = sinks.begin(); s != sinks.end(); ++s) {
    if (s->UpdateMediaFormat(mediaFormat)) {
      source.InternalUpdateMediaFormat(s->stream->GetMediaFormat());
      atLeastOne = true;
    }
  }

  PTRACE_IF(2, !atLeastOne, "Patch\tCould not update media format for any stream/transcoder in " << *this);
  return atLeastOne;
}


PBoolean OpalMediaPatch::ExecuteCommand(const OpalMediaCommand & command, PBoolean fromSink)
{
  PSafeLockReadOnly mutex(*this);

  if (fromSink) {
    OpalMediaPatch * patch = m_bypassToPatch != NULL ? m_bypassToPatch : this;
    PTRACE(5, "Patch\tExecute command \"" << command << "\" "
           << (m_bypassToPatch != NULL ? "bypassed" : "normally") << ' ' << *this);
    return patch->source.ExecuteCommand(command);
  }

  bool atLeastOne = false;

  for (PList<Sink>::iterator s = sinks.begin(); s != sinks.end(); ++s) {
    if (s->ExecuteCommand(command))
      atLeastOne = true;
  }

  return atLeastOne;
}


void OpalMediaPatch::OnMediaCommand(OpalMediaCommand & command, INT)
{
  source.ExecuteCommand(command);
}


bool OpalMediaPatch::SetPaused(bool pause)
{
  PSafeLockReadOnly mutex(*this);

  bool atLeastOne = source.SetPaused(pause, true);

  for (PList<Sink>::iterator s = sinks.begin(); s != sinks.end(); ++s) {
    if (s->stream->SetPaused(pause, true))
      atLeastOne = true;
  }

  if (!pause)
    Start();

  return atLeastOne;
}


bool OpalMediaPatch::OnStartMediaPatch()
{
  source.OnStartMediaPatch();

  if (source.IsSynchronous())
    return false;

  return EnableJitterBuffer();
}


bool OpalMediaPatch::EnableJitterBuffer()
{
  PSafeLockReadOnly mutex(*this);

  bool enab = m_bypassToPatch == NULL;

  PList<Sink>::iterator s;
  for (s = sinks.begin(); s != sinks.end(); ++s) {
    if (s->stream->EnableJitterBuffer(enab)) {
      source.EnableJitterBuffer(false);
      return false;
    }
  }

  for (s = sinks.begin(); s != sinks.end(); ++s) {
    if (s->stream->IsSynchronous() && source.EnableJitterBuffer(enab))
      return false;
  }

  return true;
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

  while (source.IsOpen()) {
    if (source.IsPaused()) {
      PThread::Sleep(100);
      continue;
    }

    sourceFrame.MakeUnique();
    sourceFrame.SetPayloadType(source.GetMediaFormat().GetPayloadType());

    // We do the following to make sure that the buffer size is large enough,
    // in case something in previous loop adjusted it
    sourceFrame.SetPayloadSize(source.GetDataSize());
    sourceFrame.SetPayloadSize(0);

    if (!source.ReadPacket(sourceFrame)) {
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

  source.OnStopMediaPatch(*this);

  PTRACE(4, "Patch\tThread ended for " << *this);
}


bool OpalMediaPatch::SetBypassPatch(OpalMediaPatch * patch)
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

    m_bypassToPatch->m_bypassFromPatch = NULL;
    m_bypassToPatch->m_bypassEnded.Signal();
  }

  if (patch != NULL) {
    if (!PAssert(patch->m_bypassFromPatch == NULL, PLogicError))
      return false;

    patch->m_bypassFromPatch = this;
  }

  m_bypassToPatch = patch;

#if OPAL_VIDEO
  OpalMediaFormat format = source.GetMediaFormat();
  if (format.IsTransportable() && format.GetMediaType() == OpalMediaType::Video())
    source.ExecuteCommand(OpalVideoUpdatePicture());
  else
#endif
    EnableJitterBuffer();

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

  if (m_bypassFromPatch != NULL) {
    PTRACE(3, "Patch\tMedia patch bypass started by " << *m_bypassFromPatch << " on " << *this);
    UnlockReadOnly();
    m_bypassEnded.Wait();
    PTRACE(4, "Patch\tMedia patch bypass ended on " << *this);
    return true;
  }

  FilterFrame(frame, source.GetMediaFormat());

  bool written = false;

  if (m_bypassToPatch == NULL) {
    for (PList<Sink>::iterator s = sinks.begin(); s != sinks.end(); ++s) {
      if (s->WriteFrame(frame))
        written = true;
    }
    UnlockReadOnly();
  }
  else {
    PSafePtr<OpalMediaPatch> bypassToPatch = m_bypassToPatch;
    UnlockReadOnly();

    PSafeLockReadOnly guard(*bypassToPatch);
    for (PList<Sink>::iterator s = bypassToPatch->sinks.begin(); s != bypassToPatch->sinks.end(); ++s) {
      if (s->stream->WritePacket(frame))
        written = true;
    }
  }

  return written;
}


bool OpalMediaPatch::Sink::UpdateMediaFormat(const OpalMediaFormat & mediaFormat)
{
  bool ok;

  if (primaryCodec == NULL)
    ok = stream->InternalUpdateMediaFormat(mediaFormat);
  else if (secondaryCodec != NULL && secondaryCodec->GetOutputFormat() == mediaFormat)
    ok = secondaryCodec->UpdateMediaFormats(OpalMediaFormat(), mediaFormat) &&
         stream->InternalUpdateMediaFormat(secondaryCodec->GetOutputFormat());
  else if (primaryCodec->GetOutputFormat() == mediaFormat)
    ok = primaryCodec->UpdateMediaFormats(OpalMediaFormat(), mediaFormat) &&
         stream->InternalUpdateMediaFormat(primaryCodec->GetOutputFormat());
  else
    ok = primaryCodec->UpdateMediaFormats(mediaFormat, OpalMediaFormat()) &&
         stream->InternalUpdateMediaFormat(primaryCodec->GetInputFormat());

#if OPAL_VIDEO
    SetRateControlParameters(stream->GetMediaFormat());
#endif

  PTRACE(3, "Patch\tUpdated Sink: format=" << mediaFormat << " ok=" << ok);
  return ok;
}


bool OpalMediaPatch::Sink::ExecuteCommand(const OpalMediaCommand & command)
{
  PBoolean atLeastOne = PFalse;

  if (secondaryCodec != NULL)
    atLeastOne = secondaryCodec->ExecuteCommand(command) || atLeastOne;

  if (primaryCodec != NULL)
    atLeastOne = primaryCodec->ExecuteCommand(command) || atLeastOne;

  return atLeastOne;
}


bool OpalMediaPatch::Sink::CannotTranscodeFrame(OpalTranscoder & codec, RTP_DataFrame & frame)
{
  RTP_DataFrame::PayloadTypes pt = frame.GetPayloadType();

  if (!codec.AcceptEmptyPayload() && frame.GetPayloadSize() == 0) {
    frame.SetPayloadType(codec.GetPayloadType(false));
    return true;
  }

  if (!codec.AcceptComfortNoise()) {
    if (pt == RTP_DataFrame::CN || pt == RTP_DataFrame::Cisco_CN) {
      PTRACE(4, "Patch\tRemoving comfort noise frame with payload type " << pt);
      frame.SetPayloadSize(0);   // remove the payload because the transcoder has indicated it won't understand it
      frame.SetPayloadType(codec.GetPayloadType(true));
      return true;
    }
  }

  if ((pt != codec.GetPayloadType(true)) && !codec.AcceptOtherPayloads()) {
    if (pt != m_lastPayloadType) {
      m_consecutivePayloadTypeMismatches = 0;
      m_lastPayloadType = pt;
    }
    else if (++m_consecutivePayloadTypeMismatches > 10) {
      PTRACE(2, "Patch\tConsecutive mismatched payload type, was expecting " 
             << codec.GetPayloadType(true) << ", now using " << pt);
      OpalMediaFormat fmt = codec.GetInputFormat();
      fmt.SetPayloadType(pt);
      codec.UpdateMediaFormats(fmt, OpalMediaFormat());
      return false;
    }
    PTRACE(4, "Patch\tRemoving frame with mismatched payload type " << pt << " - should be " << codec.GetPayloadType(true));
    frame.SetPayloadSize(0);   // remove the payload because the transcoder has indicated it won't understand it
    frame.SetPayloadType(codec.GetPayloadType(true)); // Reset pt so if get silence frames from jitter buffer, they don't cause errors
    return true;
  }

  return false;
}


#if OPAL_VIDEO
void OpalMediaPatch::Sink::SetRateControlParameters(const OpalMediaFormat & mediaFormat)
{
  if ((mediaFormat.GetMediaType() == OpalMediaType::Video()) && mediaFormat != OpalYUV420P) {
    rateController = NULL;
    PString rc = mediaFormat.GetOptionString(OpalVideoFormat::RateControllerOption());
    if (rc.IsEmpty() && mediaFormat.GetOptionBoolean(OpalVideoFormat::RateControlEnableOption()))
      rc = "Standard";
    if (!rc.IsEmpty()) {
      rateController = PFactory<OpalVideoRateController>::CreateInstance(rc);
      if (rateController != NULL) {   
        PTRACE(3, "Patch\tCreated " << rc << " rate controller");
      }
      else {
        PTRACE(3, "Patch\tCould not create " << rc << " rate controller");
      }
    }
  }

  if (rateController != NULL) 
    rateController->Open(mediaFormat);
}


bool OpalMediaPatch::Sink::RateControlExceeded(bool & forceIFrame)
{
  if ((rateController == NULL) || !rateController->SkipFrame(forceIFrame)) 
    return false;

  PTRACE(4, "Patch\tRate controller skipping frame.");
  return true;
}

#endif


bool OpalMediaPatch::Sink::WriteFrame(RTP_DataFrame & sourceFrame)
{
  if (!writeSuccessful)
    return false;
  
  if (stream->IsPaused())
    return true;

#if OPAL_VIDEO
  if (rateController != NULL) {
    bool forceIFrame = false;
    bool s = RateControlExceeded(forceIFrame);
    if (forceIFrame)
      stream->ExecuteCommand(OpalVideoUpdatePicture());
    if (s) {
      if (secondaryCodec == NULL) {
        bool wasIFrame = false;
        if (rateController->Pop(intermediateFrames, wasIFrame, false)) {
        PTRACE(3, "RC returned " << intermediateFrames.GetSize() << " packets");
          for (RTP_DataFrameList::iterator interFrame = intermediateFrames.begin(); interFrame != intermediateFrames.end(); ++interFrame) {
            patch.FilterFrame(*interFrame, primaryCodec->GetOutputFormat());
            if (!stream->WritePacket(*interFrame))
              return (writeSuccessful = false);
            sourceFrame.SetTimestamp(interFrame->GetTimestamp());
            continue;
          }
          intermediateFrames.RemoveAll();
        }
      }
      return true;
    }
  }
#endif

  if (primaryCodec == NULL)
    return (writeSuccessful = stream->WritePacket(sourceFrame));

  if (CannotTranscodeFrame(*primaryCodec, sourceFrame))
    return (writeSuccessful = stream->WritePacket(sourceFrame));

  if (!primaryCodec->ConvertFrames(sourceFrame, intermediateFrames)) {
    PTRACE(1, "Patch\tMedia conversion (primary) failed");
    return false;
  }

#if OPAL_VIDEO
  if (secondaryCodec == NULL && rateController != NULL) {
    PTRACE(4, "Patch\tPushing " << intermediateFrames.GetSize() << " packet into RC");
    rateController->Push(intermediateFrames, ((OpalVideoTranscoder *)primaryCodec)->WasLastFrameIFrame());
    bool wasIFrame = false;
    if (rateController->Pop(intermediateFrames, wasIFrame, false)) {
      PTRACE(4, "Patch\tPulled " << intermediateFrames.GetSize() << " frames from RC");
      for (RTP_DataFrameList::iterator interFrame = intermediateFrames.begin(); interFrame != intermediateFrames.end(); ++interFrame) {
        patch.FilterFrame(*interFrame, primaryCodec->GetOutputFormat());
        if (!stream->WritePacket(*interFrame))
          return (writeSuccessful = false);
        if (primaryCodec == NULL)
          return true;
        primaryCodec->CopyTimestamp(sourceFrame, *interFrame, false);
        continue;
      }
      intermediateFrames.RemoveAll();
    }
  }
  else 
#endif
  for (RTP_DataFrameList::iterator interFrame = intermediateFrames.begin(); interFrame != intermediateFrames.end(); ++interFrame) {
    patch.FilterFrame(*interFrame, primaryCodec->GetOutputFormat());

    if (secondaryCodec == NULL) {
      if (!stream->WritePacket(*interFrame))
        return (writeSuccessful = false);
      if (primaryCodec == NULL)
        return true;
      primaryCodec->CopyTimestamp(sourceFrame, *interFrame, false);
      continue;
    }

    if (CannotTranscodeFrame(*secondaryCodec, *interFrame)) {
      if (!stream->WritePacket(*interFrame))
        return (writeSuccessful = false);
      continue;
    }

    if (!secondaryCodec->ConvertFrames(*interFrame, finalFrames)) {
      PTRACE(1, "Patch\tMedia conversion (secondary) failed");
      return false;
    }

    for (RTP_DataFrameList::iterator finalFrame = finalFrames.begin(); finalFrame != finalFrames.end(); ++finalFrame) {
      patch.FilterFrame(*finalFrame, secondaryCodec->GetOutputFormat());
      if (!stream->WritePacket(*finalFrame))
        return (writeSuccessful = false);
      if (secondaryCodec == NULL)
        return true;
      secondaryCodec->CopyTimestamp(sourceFrame, *finalFrame, false);
    }
  }

#if OPAL_VIDEO
  //if (rcEnabled)
  //  rateController.AddFrame(totalPayloadSize, frameCount);
#endif

  return true;
}


OpalMediaPatch::Thread::Thread(OpalMediaPatch & p)
  : PThread(65536,  //16*4kpage size
            NoAutoDeleteThread,
            HighPriority,
            "Media Patch"),
    patch(p)
{
  PTRACE_CONTEXT_ID_FROM(p);
}


/////////////////////////////////////////////////////////////////////////////


OpalPassiveMediaPatch::OpalPassiveMediaPatch(OpalMediaStream & source)
  : OpalMediaPatch(source)
{
}


void OpalPassiveMediaPatch::Start()
{
  OnStartMediaPatch();
}

