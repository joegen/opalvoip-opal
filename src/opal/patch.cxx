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
 * Revision 1.2002  2002/01/14 02:19:03  robertj
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

  RTP_DataFrame sourceFrame, intermediateFrame, finalFrame;
  while (source.ReadPacket(sourceFrame)) {
    inUse.Wait();

    for (i = 0; i < sinks.GetSize(); i++) {
      Sink & sink = sinks[i];

      RTP_DataFrame * sinkFrame;
      if (sink.primaryCodec == NULL || sourceFrame.GetPayloadSize() == 0)
        sinkFrame = &sourceFrame;
      else {
        sinkFrame = &intermediateFrame;
        if (sink.primaryCodec->Convert(sourceFrame, intermediateFrame)) {
          if (sink.secondaryCodec != NULL) {
            sinkFrame = &finalFrame;
            if (!sink.secondaryCodec->Convert(intermediateFrame, finalFrame)) {
              PTRACE(1, "Patch\tMedia conversion failed");
              finalFrame.SetPayloadSize(0);
            }
          }
        }
        else {
          PTRACE(1, "Patch\tMedia conversion failed");
          intermediateFrame.SetPayloadSize(0);
        }
      }

      sinkFrame->SetTimestamp(sourceFrame.GetTimestamp());
      sink.stream->WritePacket(*sinkFrame);
      sourceFrame.SetTimestamp(sinkFrame->GetTimestamp());
    }

    inUse.Signal();
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
  PAssertNULL(stream);
  PAssert(stream->IsSink(), "Attempt to set source stream as sink!");

  PWaitAndSignal mutex(inUse);

  Sink * sink = new Sink(stream);
  sinks.Append(sink);

  stream->SetPatch(this);

  // Find the media formats than can be used to get from source to sink
  OpalMediaFormat sourceFormat = source.GetMediaFormat();
  OpalMediaFormat destinationFormat = stream->GetMediaFormat();

  if (sourceFormat == destinationFormat) {
    PTRACE(3, "Patch\tAdded direct media stream sink " << *stream);
    return TRUE;
  }

  sink->primaryCodec = OpalTranscoder::Create(sourceFormat, destinationFormat);
  if (sink->primaryCodec != NULL) {
    stream->SetDataSize(sink->primaryCodec->GetOptimalDataFrameSize(FALSE));
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
    stream->SetDataSize(sink->secondaryCodec->GetOptimalDataFrameSize(FALSE));
    PTRACE(3, "Patch\tAdded media stream sink " << *stream
           << " using transcoders " << *sink->primaryCodec
           << " and " << *sink->secondaryCodec);
  }

  source.SetDataSize(sink->primaryCodec->GetOptimalDataFrameSize(TRUE));
  return TRUE;
}


void OpalMediaPatch::RemoveSink(OpalMediaStream * stream)
{
  PAssertNULL(stream);
  PTRACE(3, "Patch\tRemoving media stream sink " << *stream);

  PWaitAndSignal mutex(inUse);

  for (PINDEX i = 0; i < sinks.GetSize(); i++) {
    if (sinks[i].stream == stream) {
      sinks.RemoveAt(i);
      return;
    }
  }
}


OpalMediaPatch::Sink::Sink(OpalMediaStream * s)
{
  stream = s;
  primaryCodec = NULL;
  secondaryCodec = NULL;
}


OpalMediaPatch::Sink::~Sink()
{
  delete primaryCodec;
  delete secondaryCodec;
}


/////////////////////////////////////////////////////////////////////////////
