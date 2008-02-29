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
 * $Log: patch.cxx,v $
 * Revision 1.2009  2004/05/17 13:24:18  rjongbloed
 * Added silence suppression.
 *
 * Revision 2.7  2004/04/25 02:53:29  rjongbloed
 * Fixed GNU 3.4 warnings
 *
 * Revision 2.6  2004/02/15 04:34:08  rjongbloed
 * Fixed correct setting of write data size on sick stream. Important for current
 *   output of silence frames and adjustment of sound card buffers.
 * Fixed correct propagation of timestamp values from source to sink media
 *   stream and back from sink to source stream.
 *
 * Revision 2.5  2004/01/18 15:35:21  rjongbloed
 * More work on video support
 *
 * Revision 2.4  2003/03/17 10:27:00  robertj
 * Added video support.
 *
 * Revision 2.3  2002/03/07 02:25:52  craigs
 * Patch threads now take notice of failed writes by removing the offending sink from the list
 *
 * Revision 2.2  2002/01/22 05:13:15  robertj
 * Added filter functions to media patch.
 *
 * Revision 2.1  2002/01/14 02:19:03  robertj
 * Added ability to turn jitter buffer off in media stream to allow for patches
 *   that do not require it.
 *
 * Revision 2.0  2001/07/27 15:48:25  robertj
 * Conversion of OpenH323 to Open Phone Abstraction Library (OPAL)
 *
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
  : PThread(10000,
            NoAutoDeleteThread,
            HighestPriority,
            "Media Patch:%x"),
    source(src)
{
  src.SetPatch(this);
}


OpalMediaPatch::~OpalMediaPatch()
{
  PTRACE(3, "Patch\tMedia patch thread " << *this << " destroyed.");
}


void OpalMediaPatch::PrintOn(ostream & strm) const
{
  strm << "Patch " << source;

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
}


void OpalMediaPatch::Main()
{
  PTRACE(3, "Patch\tThread started for " << *this);

  PINDEX i;

  inUse.Wait();
  for (i = 0; i < sinks.GetSize(); i++) {
    if (sinks[i].stream->IsSynchronous()) {
      source.EnableJitterBuffer();
      break;
    }
  }
  inUse.Signal();

  RTP_DataFrame sourceFrame(source.GetDataSize());
  while (source.ReadPacket(sourceFrame)) {
    inUse.Wait();

    FilterFrame(sourceFrame, source.GetMediaFormat());

    for (i = 0; i < sinks.GetSize(); i++) {
      if (!sinks[i].WriteFrame(sourceFrame))
        sinks.RemoveAt(i--);  // Got write error, remove from sink list
    }

    PINDEX len = sinks.GetSize();
    inUse.Signal();
    if (len == 0)
      break;
  }

  PTRACE(3, "Patch\tThread ended for " << *this);
}


void OpalMediaPatch::Close()
{
  PTRACE(3, "Patch\tClosing media patch " << *this);

  source.Close();

  // This relies on the channel close doing a RemoveSink() call
  while (sinks.GetSize() > 0)
    sinks[0].stream->Close();

  PTRACE(3, "Patch\tWaiting for media patch thread to stop " << *this);
  PAssert(WaitForTermination(10000), "Media patch thread not terminated.");
}


BOOL OpalMediaPatch::AddSink(OpalMediaStream * stream)
{
  if (PAssertNULL(stream) == NULL)
    return FALSE;

  PAssert(stream->IsSink(), "Attempt to set source stream as sink!");

  PWaitAndSignal mutex(inUse);

  Sink * sink = new Sink(*this, stream);
  sinks.Append(sink);

  stream->SetPatch(this);

  // Find the media formats than can be used to get from source to sink
  OpalMediaFormat sourceFormat = source.GetMediaFormat();
  OpalMediaFormat destinationFormat = stream->GetMediaFormat();

  if (sourceFormat == destinationFormat && source.GetDataSize() <= stream->GetDataSize()) {
    PTRACE(3, "Patch\tAdded direct media stream sink " << *stream);
    return TRUE;
  }

  sink->primaryCodec = OpalTranscoder::Create(sourceFormat, destinationFormat);
  if (sink->primaryCodec != NULL) {
    if (!stream->SetDataSize(sink->primaryCodec->GetOptimalDataFrameSize(FALSE))) {
      PTRACE(2, "Patch\tSink stream " << *stream << " cannot support data size "
              << sink->primaryCodec->GetOptimalDataFrameSize(FALSE));
      return FALSE;
    }

    PTRACE(3, "Patch\tAdded media stream sink " << *stream
           << " using transcoder " << *sink->primaryCodec);
  }
  else {
    OpalMediaFormat intermediateFormat;
    if (!OpalTranscoder::FindIntermediateFormat(sourceFormat, destinationFormat,
                                                intermediateFormat)) {
      PTRACE(2, "Patch\tCould find compatible media format for " << *stream);
      return FALSE;
    }

    sink->primaryCodec = OpalTranscoder::Create(sourceFormat, intermediateFormat);
    sink->secondaryCodec = OpalTranscoder::Create(intermediateFormat, destinationFormat);
    if (!stream->SetDataSize(sink->secondaryCodec->GetOptimalDataFrameSize(FALSE))) {
      PTRACE(2, "Patch\tSink stream " << *stream << " cannot support data size "
              << sink->secondaryCodec->GetOptimalDataFrameSize(FALSE));
      return FALSE;
    }

    PTRACE(3, "Patch\tAdded media stream sink " << *stream
           << " using transcoders " << *sink->primaryCodec
           << " and " << *sink->secondaryCodec);
  }

  source.SetDataSize(sink->primaryCodec->GetOptimalDataFrameSize(TRUE));
  return TRUE;
}


void OpalMediaPatch::RemoveSink(OpalMediaStream * stream)
{
  if (PAssertNULL(stream) == NULL)
    return;

  PTRACE(3, "Patch\tRemoving media stream sink " << *stream);

  PWaitAndSignal mutex(inUse);

  for (PINDEX i = 0; i < sinks.GetSize(); i++) {
    if (sinks[i].stream == stream) {
      sinks.RemoveAt(i);
      return;
    }
  }
}


OpalMediaPatch::Sink::Sink(OpalMediaPatch & p, OpalMediaStream * s)
  : patch(p)
{
  stream = s;
  primaryCodec = NULL;
  secondaryCodec = NULL;
  intermediateFrames.Append(new RTP_DataFrame);
  finalFrames.Append(new RTP_DataFrame);
}


OpalMediaPatch::Sink::~Sink()
{
  delete primaryCodec;
  delete secondaryCodec;
}


void OpalMediaPatch::AddFilter(const PNotifier & filter, const OpalMediaFormat & stage)
{
  inUse.Wait();
  filters.Append(new Filter(filter, stage));
  inUse.Signal();
}


BOOL OpalMediaPatch::RemoveFilter(const PNotifier & filter, const OpalMediaFormat & stage)
{
  PWaitAndSignal mutex(inUse);

  for (PINDEX i = 0; i < filters.GetSize(); i++) {
    if (filters[i].notifier == filter && filters[i].stage == stage) {
      filters.RemoveAt(i);
      return TRUE;
    }
  }

  return FALSE;
}


void OpalMediaPatch::FilterFrame(RTP_DataFrame & frame,
                                 const OpalMediaFormat & mediaFormat)
{
  for (PINDEX f = 0; f < filters.GetSize(); f++) {
    Filter & filter = filters[f];
    if (filter.stage.IsEmpty() || filter.stage == mediaFormat)
      filter.notifier(frame, (INT)this);
  }
}


BOOL OpalMediaPatch::Sink::WriteFrame(RTP_DataFrame & sourceFrame)
{
  if (primaryCodec == NULL || sourceFrame.GetPayloadSize() == 0)
    return stream->WritePacket(sourceFrame);

  if (!primaryCodec->ConvertFrames(sourceFrame, intermediateFrames)) {
    PTRACE(1, "Patch\tMedia conversion (primary) failed");
    return FALSE;
  }

  for (PINDEX i = 0; i < intermediateFrames.GetSize(); i++) {
    RTP_DataFrame & intermediateFrame = intermediateFrames[i];
    patch.FilterFrame(intermediateFrame, primaryCodec->GetOutputFormat());
    if (secondaryCodec == NULL) {
      if (!stream->WritePacket(intermediateFrame))
        return FALSE;
      sourceFrame.SetTimestamp(intermediateFrame.GetTimestamp());
    }
    else {
      if (!secondaryCodec->ConvertFrames(intermediateFrame, finalFrames)) {
        PTRACE(1, "Patch\tMedia conversion (secondary) failed");
        return FALSE;
      }

      for (PINDEX f = 0; f < finalFrames.GetSize(); f++) {
        RTP_DataFrame & finalFrame = finalFrames[f];
        patch.FilterFrame(finalFrame, secondaryCodec->GetOutputFormat());
        if (!stream->WritePacket(finalFrame))
          return FALSE;
        sourceFrame.SetTimestamp(finalFrame.GetTimestamp());
      }
    }
  }

  return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
