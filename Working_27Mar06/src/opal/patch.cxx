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
 * Revision 1.2021  2006/02/02 07:02:58  csoutheren
 * Added RTP payload map to transcoders and connections to allow remote SIP endpoints
 * to change the payload type used for outgoing RTP.
 *
 * Revision 2.19  2005/12/30 14:33:12  dsandras
 * Added support for Packet Loss Concealment frames for framed codecs supporting it similarly to what was done for OpenH323.
 *
 * Revision 2.18  2005/12/21 20:39:15  dsandras
 * Prevent recursion when executing a command on a stream.
 *
 * Revision 2.17  2005/11/25 21:02:19  dsandras
 * Remove the filters when closing the OpalMediaPatch.
 *
 * Revision 2.16  2005/10/20 20:28:18  dsandras
 * Avoid the thread to keep the priority for a too long time.
 *
 * Revision 2.15  2005/09/06 12:44:49  rjongbloed
 * Many fixes to finalise the video processing: merging remote media
 *
 * Revision 2.14  2005/09/04 06:23:39  rjongbloed
 * Added OpalMediaCommand mechanism (via PNotifier) for media streams
 *   and media transcoders to send commands back to remote.
 *
 * Revision 2.13  2005/08/31 13:19:25  rjongbloed
 * Added mechanism for controlling media (especially codecs) including
 *   changing the OpalMediaFormat option list (eg bit rate) and a completely
 *   new OpalMediaCommand abstraction for things like video fast update.
 *
 * Revision 2.12  2005/07/24 07:42:29  rjongbloed
 * Fixed various video media stream issues.
 *
 * Revision 2.11  2004/08/16 09:53:48  rjongbloed
 * Fixed possible deadlock in PTRACE output of media patch.
 *
 * Revision 2.10  2004/08/15 10:10:28  rjongbloed
 * Fixed possible deadlock when closing media patch
 *
 * Revision 2.9  2004/08/14 07:56:43  rjongbloed
 * Major revision to utilise the PSafeCollection classes for the connections and calls.
 *
 * Revision 2.8  2004/05/17 13:24:18  rjongbloed
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

  // Have timed mutex so avoid deadlocks in PTRACE(), it is nice to
  // get all the sinks in the PrintOn, we don't HAVE to have it.
  if (inUse.Wait(20)) {

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


void OpalMediaPatch::Main()
{
  PTRACE(3, "Patch\tThread started for " << *this);

  PINDEX i;

  inUse.Wait();
  if (!source.IsSynchronous()) {
    for (i = 0; i < sinks.GetSize(); i++) {
      if (sinks[i].stream->IsSynchronous()) {
        source.EnableJitterBuffer();
        break;
      }
    }
  }
  inUse.Signal();

  RTP_DataFrame sourceFrame(source.GetDataSize());
  while (source.ReadPacket(sourceFrame)) {
    inUse.Wait();

    FilterFrame(sourceFrame, source.GetMediaFormat());

    for (i = 0; i < sinks.GetSize(); i++)
      sinks[i].WriteFrame(sourceFrame);  // Got write error, remove from sink list

    PINDEX len = sinks.GetSize();
    inUse.Signal();
    Sleep(5); // Permit to another thread to take the mutex
    if (len == 0)
      break;
  }

  PTRACE(3, "Patch\tThread ended for " << *this);
}


void OpalMediaPatch::Close()
{
  PTRACE(3, "Patch\tClosing media patch " << *this);

  filters.RemoveAll();
  source.Close();

  // This relies on the channel close doing a RemoveSink() call
  inUse.Wait();
  while (sinks.GetSize() > 0) {
    OpalMediaStream * stream = sinks[0].stream;
    inUse.Signal();
    stream->Close();
    inUse.Wait();
  }
  inUse.Signal();

  PTRACE(3, "Patch\tWaiting for media patch thread to stop " << *this);
  PAssert(WaitForTermination(10000), "Media patch thread not terminated.");
}


BOOL OpalMediaPatch::AddSink(OpalMediaStream * stream, const RTP_DataFrame::PayloadMapType & rtpMap)
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
    sink->primaryCodec->SetRTPPayloadMap(rtpMap);
    sink->primaryCodec->SetMaxOutputSize(stream->GetDataSize());

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

    sink->secondaryCodec->SetMaxOutputSize(sink->stream->GetDataSize());

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
  writeSuccessful = true;
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


BOOL OpalMediaPatch::UpdateMediaFormat(const OpalMediaFormat & mediaFormat, BOOL fromSink)
{
  PWaitAndSignal mutex(inUse);

  if (fromSink)
    return source.UpdateMediaFormat(mediaFormat);

  BOOL atLeastOne = FALSE;
  for (PINDEX i = 0; i < sinks.GetSize(); i++)
    atLeastOne = sinks[i].UpdateMediaFormat(mediaFormat) || atLeastOne;

  return atLeastOne;
}


BOOL OpalMediaPatch::ExecuteCommand(const OpalMediaCommand & command, BOOL fromSink)
{
  PWaitAndSignal mutex(inUse);

  if (fromSink)
    return source.ExecuteCommand(command);

  BOOL atLeastOne = FALSE;
  for (PINDEX i = 0; i < sinks.GetSize(); i++)
    atLeastOne = sinks[i].ExecuteCommand(command) || atLeastOne;

  return atLeastOne;
}


void OpalMediaPatch::SetCommandNotifier(const PNotifier & notifier, BOOL fromSink)
{
  PWaitAndSignal mutex(inUse);

  if (fromSink)
    source.SetCommandNotifier(notifier);
  else {
    for (PINDEX i = 0; i < sinks.GetSize(); i++)
      sinks[i].SetCommandNotifier(notifier);
  }
}


bool OpalMediaPatch::Sink::UpdateMediaFormat(const OpalMediaFormat & mediaFormat)
{
  if (secondaryCodec != NULL)
    return secondaryCodec->UpdateOutputMediaFormat(mediaFormat);

  if (primaryCodec != NULL)
    return primaryCodec->UpdateOutputMediaFormat(mediaFormat);

  return stream->UpdateMediaFormat(mediaFormat);
}


bool OpalMediaPatch::Sink::ExecuteCommand(const OpalMediaCommand & command)
{
  BOOL atLeastOne = FALSE;

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

  if (primaryCodec == NULL)
    return writeSuccessful = stream->WritePacket(sourceFrame);

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


/////////////////////////////////////////////////////////////////////////////
