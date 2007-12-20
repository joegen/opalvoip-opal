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

#include <opal/patch.h>

#include <opal/mediastrm.h>
#include <opal/transcoders.h>


#define new PNEW


/////////////////////////////////////////////////////////////////////////////

OpalMediaPatch::OpalMediaPatch(OpalMediaStream & src)
: source(src)
{
  src.SetPatch(this);
  patchThread = NULL;
}


OpalMediaPatch::~OpalMediaPatch()
{
  PWaitAndSignal m(patchThreadMutex);
  inUse.Wait();
  delete patchThread;
  PTRACE(4, "Patch\tMedia patch thread " << *this << " destroyed.");
}


void OpalMediaPatch::PrintOn(ostream & strm) const
{
  strm << "Patch " << source;

  // Have timed mutex so avoid deadlocks in PTRACE(), it is nice to
  // get all the sinks in the PrintOn, we don't HAVE to have it.
  if (inUse.Try()) {

    if (sinks.GetSize() > 0) {
      strm << " -> ";
      if (sinks.GetSize() == 1)
        strm << *sinks[0].stream;
      else {
        for (PINDEX i = 0; i < sinks.GetSize(); i++) {
          if (i > 0)
            strm << ", ";
          strm << "sink[" << i << "]=" << *sinks[i].stream;
        }
      }
    }

    inUse.Signal();
  } 
}

void OpalMediaPatch::Start()
{
  PWaitAndSignal m(patchThreadMutex);
	
  if(patchThread != NULL) 
    return;
	
  patchThread = new Thread(*this);
  patchThread->Resume();
  PThread::Yield();
  PTRACE(4, "Media\tStarting thread " << patchThread->GetThreadName());
}


void OpalMediaPatch::Close()
{
  PTRACE(3, "Patch\tClosing media patch " << *this);

  inUse.Wait();
  filters.RemoveAll();
  source.Close();

  while (sinks.GetSize() > 0) {
    OpalMediaStreamPtr stream = sinks[0].stream;
    inUse.Signal();
    if (!stream->Close()) {
      // The only way we can get here is if the sink is in the proccess of being closed
      // but is blocked on the inUse mutex waiting to remove the sink from this patch.
      // Se we unlock it, and wait for it to do it in the other thread.
      Sleep(10);
    }
    inUse.Wait();
  }

  PTRACE(4, "Patch\tWaiting for media patch thread to stop " << *this);
  if (patchThread != NULL && !patchThread->IsSuspended()) {
    inUse.Signal();
    PAssert(patchThread->WaitForTermination(10000), "Media patch thread not terminated.");
    return;
  }
  
  inUse.Signal();
}


PBoolean OpalMediaPatch::AddSink(const OpalMediaStreamPtr & stream, const RTP_DataFrame::PayloadMapType & rtpMap)
{
  PWaitAndSignal mutex(inUse);

  if (PAssertNULL(stream) == NULL)
    return PFalse;

  PAssert(stream->IsSink(), "Attempt to set source stream as sink!");

  if (!stream->SetPatch(this)) {
    PTRACE(2, "Patch\tCould not set patch in stream " << *stream);
    return PFalse;
  }

  Sink * sink = new Sink(*this, stream, rtpMap);
  sinks.Append(sink);

  // Find the media formats than can be used to get from source to sink
  OpalMediaFormat sourceFormat = source.GetMediaFormat();
  OpalMediaFormat destinationFormat = stream->GetMediaFormat();

  PTRACE(5, "Patch\tAddSink\n"
            "Source format:\n" << setw(-1) << sourceFormat << "\n"
            "Destination format:\n" << setw(-1) << destinationFormat);

  if ((sourceFormat == destinationFormat) && ((sourceFormat.GetDefaultSessionID() == OpalMediaFormat::DefaultDataSessionID) || (source.GetDataSize() <= stream->GetDataSize()))) {
    PTRACE(3, "Patch\tAdded direct media stream sink " << *stream);
    return PTrue;
  }

  PString id = stream->GetID();
  sink->primaryCodec = OpalTranscoder::Create(sourceFormat, destinationFormat, (const BYTE *)id, id.GetLength());
  if (sink->primaryCodec != NULL) {
    PTRACE(4, "Patch\tCreated primary codec " << sourceFormat << "/" << destinationFormat << " with ID " << id);
    sink->primaryCodec->SetRTPPayloadMap(rtpMap);
    sink->primaryCodec->SetMaxOutputSize(stream->GetDataSize());

    if (!stream->SetDataSize(sink->primaryCodec->GetOptimalDataFrameSize(PFalse))) {
      PTRACE(1, "Patch\tSink stream " << *stream << " cannot support data size "
              << sink->primaryCodec->GetOptimalDataFrameSize(PFalse));
      return PFalse;
    }

    PTRACE(3, "Patch\tAdded media stream sink " << *stream
           << " using transcoder " << *sink->primaryCodec);
  }
  else {
    OpalMediaFormat intermediateFormat;
    if (!OpalTranscoder::FindIntermediateFormat(sourceFormat, destinationFormat,
                                                intermediateFormat)) {
      PTRACE(1, "Patch\tCould find compatible media format for " << *stream);
      return PFalse;
    }

    sink->primaryCodec = OpalTranscoder::Create(sourceFormat, intermediateFormat, (const BYTE *)id, id.GetLength());
    sink->secondaryCodec = OpalTranscoder::Create(intermediateFormat, destinationFormat, (const BYTE *)id, id.GetLength());

    PTRACE(4, "Patch\tCreated two stage codec " << sourceFormat << "/" << intermediateFormat << "/" << destinationFormat << " with ID " << id);

    sink->secondaryCodec->SetMaxOutputSize(sink->stream->GetDataSize());

    if (!stream->SetDataSize(sink->secondaryCodec->GetOptimalDataFrameSize(PFalse))) {
      PTRACE(1, "Patch\tSink stream " << *stream << " cannot support data size "
              << sink->secondaryCodec->GetOptimalDataFrameSize(PFalse));
      return PFalse;
    }

    PTRACE(3, "Patch\tAdded media stream sink " << *stream
           << " using transcoders " << *sink->primaryCodec
           << " and " << *sink->secondaryCodec);
  }

  source.SetDataSize(sink->primaryCodec->GetOptimalDataFrameSize(PTrue));
  return PTrue;
}


void OpalMediaPatch::RemoveSink(const OpalMediaStreamPtr & stream)
{
  if (PAssertNULL(stream) == NULL)
    return;

  PTRACE(3, "Patch\tRemoving media stream sink " << *stream);

  inUse.Wait();

  for (PINDEX i = 0; i < sinks.GetSize(); i++) {
    if (sinks[i].stream == stream) {
      sinks.RemoveAt(i);
      if (!sinks.IsEmpty())
        break;

      inUse.Signal();
      source.Close();
      return;
    }
  }

  inUse.Signal();
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
  inUse.Wait();

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
  inUse.Signal();
}


OpalMediaPatch::Sink::Sink(OpalMediaPatch & p, const OpalMediaStreamPtr & s, const RTP_DataFrame::PayloadMapType & m)
  : patch(p)
  , stream(s)
{
  payloadTypeMap = m;
  primaryCodec = NULL;
  secondaryCodec = NULL;
  intermediateFrames.Append(new RTP_DataFrame);
  finalFrames.Append(new RTP_DataFrame);
  writeSuccessful = true;
}


OpalMediaPatch::Sink::~Sink()
{
  delete primaryCodec;
  delete secondaryCodec;
}


void OpalMediaPatch::AddFilter(const PNotifier & filter, const OpalMediaFormat & stage)
{
  PWaitAndSignal mutex(inUse);
  
  // ensures that a filter is added only once
  for (PINDEX i = 0; i < filters.GetSize(); i++) {
    if (filters[i].notifier == filter && filters[i].stage == stage) {
	  return;
    }
  }
  filters.Append(new Filter(filter, stage));
}


PBoolean OpalMediaPatch::RemoveFilter(const PNotifier & filter, const OpalMediaFormat & stage)
{
  PWaitAndSignal mutex(inUse);

  for (PINDEX i = 0; i < filters.GetSize(); i++) {
    if (filters[i].notifier == filter && filters[i].stage == stage) {
      filters.RemoveAt(i);
      return PTrue;
    }
  }

  return PFalse;
}


void OpalMediaPatch::FilterFrame(RTP_DataFrame & frame,
                                 const OpalMediaFormat & mediaFormat)
{
  PWaitAndSignal mutex(inUse);
  for (PINDEX f = 0; f < filters.GetSize(); f++) {
    Filter & filter = filters[f];
    if (filter.stage.IsEmpty() || filter.stage == mediaFormat)
      filter.notifier(frame, (INT)this);
  }
}


PBoolean OpalMediaPatch::UpdateMediaFormat(const OpalMediaFormat & mediaFormat, PBoolean fromSink)
{
  PWaitAndSignal mutex(inUse);

  if (fromSink)
    return source.UpdateMediaFormat(mediaFormat);

  PBoolean atLeastOne = PFalse;
  for (PINDEX i = 0; i < sinks.GetSize(); i++)
    atLeastOne = sinks[i].UpdateMediaFormat(mediaFormat) || atLeastOne;

  return atLeastOne;
}


PBoolean OpalMediaPatch::ExecuteCommand(const OpalMediaCommand & command, PBoolean fromSink)
{
  PWaitAndSignal mutex(inUse);

  if (fromSink)
    return source.ExecuteCommand(command);

  PBoolean atLeastOne = PFalse;
  for (PINDEX i = 0; i < sinks.GetSize(); i++)
    atLeastOne = sinks[i].ExecuteCommand(command) || atLeastOne;

  return atLeastOne;
}


void OpalMediaPatch::SetCommandNotifier(const PNotifier & notifier, PBoolean fromSink)
{
  PWaitAndSignal mutex(inUse);

  if (fromSink)
    source.SetCommandNotifier(notifier);
  else {
    for (PINDEX i = 0; i < sinks.GetSize(); i++)
      sinks[i].SetCommandNotifier(notifier);
  }
}

void OpalMediaPatch::Main()
{
  PTRACE(4, "Patch\tThread started for " << *this);
  PINDEX i;
	
  inUse.Wait();
  source.OnPatchStart();
  PBoolean isSynchronous = source.IsSynchronous();
  if (!source.IsSynchronous()) {
    for (i = 0; i < sinks.GetSize(); i++) {
      if (sinks[i].stream->IsSynchronous()) {
        source.EnableJitterBuffer();
        isSynchronous = PTrue;
        break;
      }
    }
  }
	
  inUse.Signal();

  RTP_DataFrame sourceFrame(source.GetDataSize());
	
  while (source.IsOpen()) {
    sourceFrame.SetPayloadSize(0); 
    if (!source.ReadPacket(sourceFrame))
      break;
 
    inUse.Wait();
    DispatchFrame(sourceFrame);
    inUse.Signal();

    // Don't starve the CPU if we have empty frames
    if (!isSynchronous || sourceFrame.GetPayloadSize() == 0)
      PThread::Sleep(5);
  }

  PTRACE(4, "Patch\tThread ended for " << *this);
}


void OpalMediaPatch::DispatchFrame(RTP_DataFrame & frame)
{
  FilterFrame(frame, source.GetMediaFormat());    
	
  PINDEX len = sinks.GetSize();
  for (PINDEX i = 0; i < len; i++)
    sinks[i].WriteFrame(frame);
}


bool OpalMediaPatch::Sink::UpdateMediaFormat(const OpalMediaFormat & mediaFormat)
{
  if (primaryCodec == NULL)
    return stream->UpdateMediaFormat(mediaFormat);

  if (secondaryCodec != NULL && secondaryCodec->GetOutputFormat() == mediaFormat)
    return secondaryCodec->UpdateMediaFormats(OpalMediaFormat(), mediaFormat);

  if (primaryCodec->GetOutputFormat() == mediaFormat)
    return primaryCodec->UpdateMediaFormats(OpalMediaFormat(), mediaFormat);
  else
    return primaryCodec->UpdateMediaFormats(mediaFormat, OpalMediaFormat());
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


void OpalMediaPatch::Sink::SetCommandNotifier(const PNotifier & notifier)
{
  if (secondaryCodec != NULL)
    secondaryCodec->SetCommandNotifier(notifier);

  if (primaryCodec != NULL)
    primaryCodec->SetCommandNotifier(notifier);
}


bool OpalMediaPatch::Sink::WriteFrame(RTP_DataFrame & sourceFrame)
{
  if (!writeSuccessful)
    return false;

  if (primaryCodec == NULL) {
    RTP_DataFrame::PayloadMapType::iterator r = payloadTypeMap.find(sourceFrame.GetPayloadType());
    if (r != payloadTypeMap.end())
      sourceFrame.SetPayloadType(r->second);
    return writeSuccessful = stream->WritePacket(sourceFrame);
  }

  if (!primaryCodec->ConvertFrames(sourceFrame, intermediateFrames)) {
    PTRACE(1, "Patch\tMedia conversion (primary) failed");
    return false;
  }

  if (sourceFrame.GetPayloadSize() == 0)
    return writeSuccessful = stream->WritePacket(sourceFrame);

  for (PINDEX i = 0; i < intermediateFrames.GetSize(); i++) {
    RTP_DataFrame & intermediateFrame = intermediateFrames[i];
    patch.FilterFrame(intermediateFrame, primaryCodec->GetOutputFormat());
    if (secondaryCodec == NULL) {
      if (!stream->WritePacket(intermediateFrame))
        return writeSuccessful = false;
      sourceFrame.SetTimestamp(intermediateFrame.GetTimestamp());
    }
    else {
      if (!secondaryCodec->ConvertFrames(intermediateFrame, finalFrames)) {
        PTRACE(1, "Patch\tMedia conversion (secondary) failed");
        return false;
      }

      for (PINDEX f = 0; f < finalFrames.GetSize(); f++) {
        RTP_DataFrame & finalFrame = finalFrames[f];
        patch.FilterFrame(finalFrame, secondaryCodec->GetOutputFormat());
        if (!stream->WritePacket(finalFrame))
          return writeSuccessful = false;
        sourceFrame.SetTimestamp(finalFrame.GetTimestamp());
      }
    }
  }

  return true;
}

OpalMediaPatch::Thread::Thread(OpalMediaPatch & p)
: PThread(65536,  //16*4kpage size
  NoAutoDeleteThread,
  HighestPriority,
  "Media Patch:%x"),
  patch(p)
{
}


/////////////////////////////////////////////////////////////////////////////


OpalPassiveMediaPatch::OpalPassiveMediaPatch(OpalMediaStream & source)
: OpalMediaPatch(source)
{
}


void OpalPassiveMediaPatch::Start()
{
  source.OnPatchStart();
}


PBoolean OpalPassiveMediaPatch::PushFrame(RTP_DataFrame & frame)
{
  DispatchFrame(frame);
  return PTrue;
}


/////////////////////////////////////////////////////////////////////////////
