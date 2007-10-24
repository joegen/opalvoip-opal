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
 * Revision 2.52  2007/09/05 13:43:04  csoutheren
 * Applied 1748637 - RTP payload translation
 * Thanks to Borko Jandras
 *
 * Revision 2.51  2007/09/05 13:23:39  csoutheren
 * Applied 1704162 - Opal mediastrm.cxx added IsOpen check to SetPatch
 * Thanks to Drazen Dimoti
 *
 * Revision 2.50  2007/09/03 06:40:36  rjongbloed
 * Assured data frame has zero payload size so if source media stream does not
 *   alter it the default will be to ignore the frame.
 *
 * Revision 2.49  2007/08/17 07:39:56  rjongbloed
 * Removed extraneous piece of code and fixed formatting.
 *
 * Revision 2.48  2007/08/01 08:37:57  csoutheren
 * Add function to lock sink transcoders so options can be set
 *
 * Revision 2.47  2007/05/23 14:39:31  dsandras
 * Removed annoying output.
 *
 * Revision 2.46  2007/04/04 02:12:01  rjongbloed
 * Reviewed and adjusted PTRACE log levels
 *   Now follows 1=error,2=warn,3=info,4+=debug
 *
 * Revision 2.45  2007/04/02 05:51:33  rjongbloed
 * Tidied some trace logs to assure all have a category (bit before a tab character) set.
 *
 * Revision 2.44  2007/03/29 08:30:21  csoutheren
 * Avoid problem with T.38 codecs
 *
 * Revision 2.43  2007/03/29 05:22:42  csoutheren
 * Add extra logging
 *
 * Revision 2.42  2007/03/01 03:23:00  csoutheren
 * Ignore packets with no payload emitted by jitter buffer when no input available
 *
 * Revision 2.41  2007/02/10 21:50:36  dsandras
 * Fixed potential deadlock if ReadPacket takes time to return or does not
 * return. Thanks to Hannes Friederich for the proposal and the SUN Team
 * for the bug report (Ekiga #404904).
 *
 * Revision 2.40  2007/02/05 19:43:17  dsandras
 * Added additional mutex to prevent temporary deadlock when nothing is
 * received on the remote media stream during the establishment phase.
 *
 * Revision 2.39  2007/01/25 11:48:11  hfriederich
 * OpalMediaPatch code refactorization.
 * Split into OpalMediaPatch (using a thread) and OpalPassiveMediaPatch
 * (not using a thread). Also adds the possibility for source streams
 * to push frames down to the sink streams instead of having a patch
 * thread around.
 *
 * Revision 2.38  2006/12/08 05:13:10  csoutheren
 * Applied 1603783 - To allow media streams to handle more then one patch
 * Thanks to jmatela
 *
 * Revision 2.37  2006/10/06 05:33:12  hfriederich
 * Fix RFC2833 for SIP connections
 *
 * Revision 2.36  2006/07/24 14:03:40  csoutheren
 * Merged in audio and video plugins from CVS branch PluginBranch
 *
 * Revision 2.35  2006/07/14 05:24:50  csoutheren
 * Applied 1509232 - Fix for a bug in OpalMediaPatch::Close method
 * Thanks to Borko Jandras
 *
 * Revision 2.34  2006/07/14 04:22:43  csoutheren
 * Applied 1517397 - More Phobos stability fix
 * Thanks to Dinis Rosario
 *
 * Revision 2.33  2006/07/04 00:48:14  csoutheren
 * New version of patch 1509246
 *
 * Revision 2.32  2006/06/30 09:20:37  dsandras
 * Fixed wrong assertion triggering.
 *
 * Revision 2.31  2006/06/30 07:36:37  csoutheren
 * Applied 1495026 - Avoid deadlock if mediaPatchThread has never been started
 * Thanks to mturconi
 *
 * Revision 2.30  2006/06/30 05:33:26  csoutheren
 * Applied 1509251 - Locking rearrangement in OpalMediaPatch::Main
 * Thanks to Borko Jandras
 *
 * Revision 2.29  2006/06/30 05:23:47  csoutheren
 * Applied 1509246 - Fix sleeping in OpalMediaPatch::Main
 * Thanks to Borko Jandras
 *
 * Revision 2.28  2006/06/30 01:33:43  csoutheren
 * Add function to get patch sink media format
 *
 * Revision 2.27  2006/06/28 11:29:07  csoutheren
 * Patch 1456858 - Add mutex to transaction dictionary and other stability patches
 * Thanks to drosario
 *
 * Revision 2.26  2006/06/27 12:08:01  csoutheren
 * Patch 1455568 - RFC2833 patch
 * Thanks to Boris Pavacic
 *
 * Revision 2.25  2006/06/03 12:42:36  shorne
 * Fix compile error on MSVC6
 *
 * Revision 2.24  2006/05/07 15:33:54  dsandras
 * Reverted the last part of the patch.
 *
 * Revision 2.23  2006/05/07 14:03:04  dsandras
 * Reverted patch 2.21 which could cause some deadlocks with H.323.
 *
 * Revision 2.22  2006/04/09 12:12:54  rjongbloed
 * Changed the media format option merging to include the transcoder formats.
 *
 * Revision 2.21  2006/03/20 10:37:47  csoutheren
 * Applied patch #1453753 - added locking on media stream manipulation
 * Thanks to Dinis Rosario
 *
 * Revision 2.20  2006/02/02 07:02:58  csoutheren
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
    OpalMediaStream * stream = sinks[0].stream;
    stream->GetDeleteMutex().Wait();
    inUse.Signal();
    stream->RemovePatch(this);
    inUse.Wait();
    stream->GetDeleteMutex().Signal();
    RemoveSink(stream);
  }

  PTRACE(4, "Patch\tWaiting for media patch thread to stop " << *this);
  if (patchThread != NULL && !patchThread->IsSuspended()) {
    inUse.Signal();
    PAssert(patchThread->WaitForTermination(10000), "Media patch thread not terminated.");
    return;
  }
  
  inUse.Signal();
}


BOOL OpalMediaPatch::AddSink(OpalMediaStream * stream, const RTP_DataFrame::PayloadMapType & rtpMap)
{
  PWaitAndSignal mutex(inUse);

  if (PAssertNULL(stream) == NULL)
    return FALSE;

  PAssert(stream->IsSink(), "Attempt to set source stream as sink!");

  if (!stream->SetPatch(this))
    return FALSE;

  Sink * sink = new Sink(*this, stream, rtpMap);
  sinks.Append(sink);

  // Find the media formats than can be used to get from source to sink
  OpalMediaFormat sourceFormat = source.GetMediaFormat();
  OpalMediaFormat destinationFormat = stream->GetMediaFormat();

  if ((sourceFormat == destinationFormat) && ((sourceFormat.GetDefaultSessionID() == OpalMediaFormat::DefaultDataSessionID) || (source.GetDataSize() <= stream->GetDataSize()))) {
    PTRACE(3, "Patch\tAdded direct media stream sink " << *stream);
    return TRUE;
  }

  PString id = stream->GetID();
  sink->primaryCodec = OpalTranscoder::Create(sourceFormat, destinationFormat, (const BYTE *)id, id.GetLength());
  if (sink->primaryCodec != NULL) {
    PTRACE(4, "Patch\tCreated primary codec " << sourceFormat << "/" << destinationFormat << " with ID " << id);
    sink->primaryCodec->SetRTPPayloadMap(rtpMap);
    sink->primaryCodec->SetMaxOutputSize(stream->GetDataSize());

    if (!stream->SetDataSize(sink->primaryCodec->GetOptimalDataFrameSize(FALSE))) {
      PTRACE(1, "Patch\tSink stream " << *stream << " cannot support data size "
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
      PTRACE(1, "Patch\tCould find compatible media format for " << *stream);
      return FALSE;
    }

    sink->primaryCodec = OpalTranscoder::Create(sourceFormat, intermediateFormat, (const BYTE *)id, id.GetLength());
    sink->secondaryCodec = OpalTranscoder::Create(intermediateFormat, destinationFormat, (const BYTE *)id, id.GetLength());

    PTRACE(4, "Patch\tCreated two stage codec " << sourceFormat << "/" << intermediateFormat << "/" << destinationFormat << " with ID " << id);

    sink->secondaryCodec->SetMaxOutputSize(sink->stream->GetDataSize());

    if (!stream->SetDataSize(sink->secondaryCodec->GetOptimalDataFrameSize(FALSE))) {
      PTRACE(1, "Patch\tSink stream " << *stream << " cannot support data size "
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


OpalMediaPatch::Sink::Sink(OpalMediaPatch & p, OpalMediaStream * s, const RTP_DataFrame::PayloadMapType & m)
  : patch(p)
{
  stream = s;
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
  PWaitAndSignal mutex(inUse);
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

void OpalMediaPatch::Main()
{
  PTRACE(4, "Patch\tThread started for " << *this);
  PINDEX i;
	
  inUse.Wait();
  source.OnPatchStart();
  BOOL isSynchronous = source.IsSynchronous();
  if (!source.IsSynchronous()) {
    for (i = 0; i < sinks.GetSize(); i++) {
      if (sinks[i].stream->IsSynchronous()) {
        source.EnableJitterBuffer();
        isSynchronous = TRUE;
        break;
      }
    }
  }
	
  inUse.Signal();
  RTP_DataFrame sourceFrame(source.GetDataSize());
  RTP_DataFrame emptyFrame(source.GetDataSize());
	
  while (source.IsOpen()) {
    sourceFrame.SetPayloadSize(0); 
    if (!source.ReadPacket(sourceFrame))
      break;
 
    inUse.Wait();
		
    if(!source.IsOpen() || sinks.GetSize() == 0) {
      inUse.Signal();
      break;
    }
		
    PINDEX len = sinks.GetSize();
		
    if (sourceFrame.GetPayloadSize() > 0)
      DispatchFrame(sourceFrame);
		
    inUse.Signal();
		
    if (!isSynchronous || !sourceFrame.GetPayloadSize())
      PThread::Sleep(5); // Don't starve the CPU
#if !defined(WIN32)
    else
      PThread::Sleep(5); // Permit to another thread to take the mutex
#endif
		
    if (len == 0)
      break;
		
    // make a new, clean frame, so that silence frame won't confuse RFC2833 handler
    sourceFrame = emptyFrame;
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


BOOL OpalPassiveMediaPatch::PushFrame(RTP_DataFrame & frame)
{
  DispatchFrame(frame);
  return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
