/*
 * mediastrm.cxx
 *
 * Media Stream classes
 *
 * Open H323 Library
 *
 * Copyright (c) 1998-2001 Equivalence Pty. Ltd.
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
 * Contributor(s): ________________________________________.
 *
 * $Log: mediastrm.cxx,v $
 * Revision 1.2052  2006/12/08 05:39:29  csoutheren
 * Remove warnings under Windows
 *
 * Revision 2.50  2006/11/01 06:57:23  csoutheren
 * Fixed usage of YUV frame header
 *
 * Revision 2.49  2006/10/31 04:14:10  csoutheren
 * Fixed problem with uninitialised variable when getting video frames
 *
 * Revision 2.48  2006/10/11 01:15:15  csoutheren
 * Applied 1552449 - Always use max RTP buffer size
 * Thanks to Simon Zwahlen
 *
 * Revision 2.47  2006/10/10 07:18:18  csoutheren
 * Allow compilation with and without various options
 *
 * Revision 2.46  2006/08/29 08:47:43  rjongbloed
 * Added functions to get average audio signal level from audio streams in
 *   suitable connection types.
 *
 * Revision 2.45  2006/08/28 00:06:10  csoutheren
 * Applied 1545107 - MediaStream - safe access to patch for adding a filter
 * Thanks to Drazen Dimoti
 *
 * Revision 2.44  2006/07/14 05:24:50  csoutheren
 * Applied 1509232 - Fix for a bug in OpalMediaPatch::Close method
 * Thanks to Borko Jandras
 *
 * Revision 2.43  2006/07/14 04:22:43  csoutheren
 * Applied 1517397 - More Phobos stability fix
 * Thanks to Dinis Rosario
 *
 * Revision 2.42  2006/07/09 10:18:28  csoutheren
 * Applied 1517393 - Opal T.38
 * Thanks to Drazen Dimoti
 *
 * Revision 2.41  2006/06/30 04:49:54  csoutheren
 * Applied 1509240 -  Fix for OpalMediaStream::Close deadlock
 * Thanks to Borko Jandras
 *
 * Revision 2.40  2006/06/21 04:54:15  csoutheren
 * Fixed build with latest PWLib
 *
 * Revision 2.39  2005/12/30 14:30:02  dsandras
 * Removed the assumption that the jitter will contain a 8 kHz signal.
 *
 * Revision 2.38  2005/09/06 12:44:49  rjongbloed
 * Many fixes to finalise the video processing: merging remote media
 *
 * Revision 2.37  2005/09/04 06:23:39  rjongbloed
 * Added OpalMediaCommand mechanism (via PNotifier) for media streams
 *   and media transcoders to send commands back to remote.
 *
 * Revision 2.36  2005/08/31 13:19:25  rjongbloed
 * Added mechanism for controlling media (especially codecs) including
 *   changing the OpalMediaFormat option list (eg bit rate) and a completely
 *   new OpalMediaCommand abstraction for things like video fast update.
 *
 * Revision 2.35  2005/08/24 10:16:33  rjongbloed
 * Fix checking of payload type when mediaFormat is not an RTP media but internal.
 *
 * Revision 2.34  2005/08/20 07:35:11  rjongbloed
 * Correctly set grabber size to size of video frame in OpalMediaFormat
 * Set video RTP timestamps to value dirived from real time clock.
 *
 * Revision 2.33  2005/08/04 08:46:21  rjongbloed
 * Fixed video output stream to allow for empty/missing packets
 *
 * Revision 2.32  2005/07/24 07:41:27  rjongbloed
 * Fixed various video media stream issues.
 *
 * Revision 2.31  2005/07/14 08:54:35  csoutheren
 * Fixed transposition of parameters in OpalNULLStream constructor
 *
 * Revision 2.30  2005/04/10 21:16:11  dsandras
 * Added support to put an OpalMediaStream on pause.
 *
 * Revision 2.29  2005/03/12 00:33:28  csoutheren
 * Fixed problems with STL compatibility on MSVC 6
 * Fixed problems with video streams
 * Thanks to Adrian Sietsma
 *
 * Revision 2.28  2004/10/02 11:50:58  rjongbloed
 * Fixed RTP media stream so assures RTP session is open before starting.
 *
 * Revision 2.27  2004/08/14 07:56:43  rjongbloed
 * Major revision to utilise the PSafeCollection classes for the connections and calls.
 *
 * Revision 2.26  2004/05/24 13:36:30  rjongbloed
 * Fixed not transmitting RTP packets with zero length payload which
 *   is rather important for silence suppression, thanks Ted Szoczei
 *
 * Revision 2.25  2004/05/02 05:18:45  rjongbloed
 * More logging
 *
 * Revision 2.24  2004/03/25 11:53:42  rjongbloed
 * Fixed size of RTP data frame buffer when reading from RTP, needs to be big
 *   enough for anything that can be received. Pointed out by Ted Szoczei
 *
 * Revision 2.23  2004/03/22 11:32:42  rjongbloed
 * Added new codec type for 16 bit Linear PCM as must distinguish between the internal
 *   format used by such things as the sound card and the RTP payload format which
 *   is always big endian.
 *
 * Revision 2.22  2004/02/15 04:31:07  rjongbloed
 * Added trace log to sound card stream for what read/write size and buffers are used.
 *
 * Revision 2.21  2004/02/09 13:12:28  rjongbloed
 * Fixed spin problem when closing channel, realted to not outputting silence
 *   frames to the sound card when nothing coming out of jitter buffer.
 *
 * Revision 2.20  2004/01/18 15:35:21  rjongbloed
 * More work on video support
 *
 * Revision 2.19  2003/06/02 02:58:07  rjongbloed
 * Moved LID specific media stream class to LID source file.
 * Added assurance media stream is open when Start() is called.
 *
 * Revision 2.18  2003/04/08 02:46:29  robertj
 * Fixed incorrect returned buffer size on video read, thanks Guilhem Tardy.
 * Fixed missing set of corrct colour format for video grabber/display when
 *   starting vide media stream, thanks Guilhem Tardy.
 *
 * Revision 2.17  2003/03/17 10:27:00  robertj
 * Added video support.
 *
 * Revision 2.16  2003/01/07 06:00:43  robertj
 * Fixed MSVC warnings.
 *
 * Revision 2.15  2002/11/10 11:33:20  robertj
 * Updated to OpenH323 v1.10.3
 *
 * Revision 2.14  2002/04/15 08:48:14  robertj
 * Fixed problem with mismatched payload type being propagated.
 * Fixed correct setting of jitter buffer size in RTP media stream.
 *
 * Revision 2.13  2002/02/13 02:33:15  robertj
 * Added ability for media patch (and transcoders) to handle multiple RTP frames.
 * Removed media stream being descended from PChannel, not really useful.
 *
 * Revision 2.12  2002/02/11 07:42:39  robertj
 * Added media bypass for streams between compatible protocols.
 *
 * Revision 2.11  2002/01/22 05:10:44  robertj
 * Removed payload mismatch detection from RTP media stream.
 * Added function to get media patch from media stream.
 *
 * Revision 2.10  2002/01/14 02:24:33  robertj
 * Added ability to turn jitter buffer off in media stream to allow for patches
 *   that do not require it.
 *
 * Revision 2.9  2001/11/15 06:58:17  robertj
 * Changed default read size for media stream to 50ms (if is audio), fixes
 *   overly small packet size for G.711
 *
 * Revision 2.8  2001/10/15 04:32:14  robertj
 * Added delayed start of media patch threads.
 *
 * Revision 2.7  2001/10/04 05:43:44  craigs
 * Changed to start media patch threads in Paused state
 *
 * Revision 2.6  2001/10/04 00:42:48  robertj
 * Removed GetMediaFormats() function as is not useful.
 *
 * Revision 2.5  2001/10/03 06:42:32  craigs
 * Changed to remove WIN32isms from error return values
 *
 * Revision 2.4  2001/10/03 05:53:25  robertj
 * Update to new PTLib channel error system.
 *
 * Revision 2.3  2001/08/21 01:12:10  robertj
 * Fixed propagation of sound channel buffers through media stream.
 *
 * Revision 2.2  2001/08/17 08:34:28  robertj
 * Fixed not setting counts in read/write of sound channel.
 *
 * Revision 2.1  2001/08/01 05:45:34  robertj
 * Made OpalMediaFormatList class global to help with documentation.
 *
 * Revision 2.0  2001/07/27 15:48:25  robertj
 * Conversion of OpenH323 to Open Phone Abstraction Library (OPAL)
 *
 */

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "mediastrm.h"
#endif

#include <opal/buildopts.h>

#include <opal/mediastrm.h>

#if OPAL_VIDEO
#include <ptlib/videoio.h>
#include <codec/vidcodec.h>
#endif

#include <ptlib/sound.h>
#include <opal/patch.h>
#include <lids/lid.h>
#include <rtp/rtp.h>
#include <opal/transports.h>

#define MAX_PAYLOAD_TYPE_MISMATCHES 10


#define new PNEW


///////////////////////////////////////////////////////////////////////////////

OpalMediaStream::OpalMediaStream(const OpalMediaFormat & fmt, unsigned id, BOOL isSourceStream)
  : mediaFormat(fmt)
{
  isSource = isSourceStream;
  sessionID = id;
  isOpen = FALSE;

  // Set default frame size to 50ms of audio, otherwise just one frame
  unsigned frameTime = mediaFormat.GetFrameTime();
  if (frameTime != 0 && mediaFormat.GetClockRate() == OpalMediaFormat::AudioClockRate)
    SetDataSize(((400+frameTime-1)/frameTime)*mediaFormat.GetFrameSize());
  else
    SetDataSize(mediaFormat.GetFrameSize());

  timestamp = 0;
  marker = TRUE;
  paused = FALSE;
  mismatchedPayloadTypes = 0;
  patchThread = NULL;
}


OpalMediaStream::~OpalMediaStream()
{
  Close();
  PWaitAndSignal deleteLock(deleteMutex);
}


void OpalMediaStream::PrintOn(ostream & strm) const
{
  strm << GetClass() << '-';
  if (isSource)
    strm << "Source";
  else
    strm << "Sink";
  strm << '-' << mediaFormat;
}


OpalMediaFormat OpalMediaStream::GetMediaFormat() const
{
  return mediaFormat;
}


BOOL OpalMediaStream::UpdateMediaFormat(const OpalMediaFormat & mediaFormat)
{
  PWaitAndSignal mutex(patchMutex);

  if (patchThread == NULL)
    return FALSE;

  // If we are source, then update the sink side, and vice versa
  return patchThread->UpdateMediaFormat(mediaFormat, IsSink());
}


BOOL OpalMediaStream::ExecuteCommand(const OpalMediaCommand & command)
{
  PWaitAndSignal mutex(patchMutex);

  if (patchThread == NULL)
    return FALSE;

  return patchThread->ExecuteCommand(command, IsSink());
}


void OpalMediaStream::SetCommandNotifier(const PNotifier & notifier)
{
  PWaitAndSignal mutex(patchMutex);

  if (patchThread != NULL)
    patchThread->SetCommandNotifier(notifier, IsSink());

  commandNotifier = notifier;
}


BOOL OpalMediaStream::Open()
{
  isOpen = TRUE;
  return TRUE;
}


BOOL OpalMediaStream::Start()
{
  if (!Open())
    return FALSE;

  patchMutex.Wait();
  if (patchThread != NULL && patchThread->IsSuspended()) {
    patchThread->Resume();
    PThread::Yield(); // This is so the thread name below is initialised.
    PTRACE(4, "Media\tStarting thread " << patchThread->GetThreadName());
  }
  patchMutex.Signal();

  return TRUE;
}


BOOL OpalMediaStream::Close()
{
  if (!isOpen)
    return FALSE;

  PTRACE(4, "Media\tClosing stream " << *this);

  patchMutex.Wait();

  isOpen = FALSE;

  if (patchThread != NULL) {
    PTRACE(4, "Media\tDisconnecting " << *this << " from patch thread " << *patchThread);
    OpalMediaPatch * patch = patchThread;
    patchThread = NULL;

    if (IsSink())
      patch->RemoveSink(this);
	
    patchMutex.Signal();

    if (IsSource()) {
      patch->Close();
      delete patch;
    }
  }
  else
    patchMutex.Signal();

  return TRUE;
}


BOOL OpalMediaStream::WritePackets(RTP_DataFrameList & packets)
{
  for (PINDEX i = 0; i < packets.GetSize(); i++) {
    if (!WritePacket(packets[i]))
      return FALSE;
  }

  return TRUE;
}


inline static unsigned CalculateTimestamp(PINDEX size, const OpalMediaFormat & mediaFormat)
{
  unsigned frameTime = mediaFormat.GetFrameTime();
  PINDEX frameSize = mediaFormat.GetFrameSize();

  if (frameSize == 0)
    return frameTime;

  unsigned frames = (size + frameSize - 1) / frameSize;
  return frames*frameTime;
}


BOOL OpalMediaStream::ReadPacket(RTP_DataFrame & packet)
{
  unsigned oldTimestamp = timestamp;

  PINDEX lastReadCount;
  if (!ReadData(packet.GetPayloadPtr(), defaultDataSize, lastReadCount))
    return FALSE;

  // If the ReadData() function did not change the timestamp then use the default
  // method or fixed frame times and sizes.
  if (oldTimestamp == timestamp)
    timestamp += CalculateTimestamp(lastReadCount, mediaFormat);

  packet.SetPayloadType(mediaFormat.GetPayloadType());
  packet.SetPayloadSize(lastReadCount);
  packet.SetTimestamp(oldTimestamp); // Beginning of frame
  packet.SetMarker(marker);
  marker = FALSE;

  return TRUE;
}


BOOL OpalMediaStream::WritePacket(RTP_DataFrame & packet)
{
  timestamp = packet.GetTimestamp();
  int size = paused?0:packet.GetPayloadSize();

  if (paused)
    packet.SetPayloadSize(0);
  
  if (size > 0 && mediaFormat.GetPayloadType() != RTP_DataFrame::MaxPayloadType) {
    if (packet.GetPayloadType() == mediaFormat.GetPayloadType()) {
      PTRACE_IF(2, mismatchedPayloadTypes > 0,
                "H323RTP\tPayload type matched again " << mediaFormat.GetPayloadType());
      mismatchedPayloadTypes = 0;
    }
    else {
      mismatchedPayloadTypes++;
      if (mismatchedPayloadTypes < MAX_PAYLOAD_TYPE_MISMATCHES) {
        PTRACE(2, "Media\tRTP data with mismatched payload type,"
                  " is " << packet.GetPayloadType() << 
                  " expected " << mediaFormat.GetPayloadType() <<
                  ", ignoring packet.");
        size = 0;
      }
      else {
        PTRACE_IF(2, mismatchedPayloadTypes == MAX_PAYLOAD_TYPE_MISMATCHES,
                  "Media\tRTP data with consecutive mismatched payload types,"
                  " is " << packet.GetPayloadType() << 
                  " expected " << mediaFormat.GetPayloadType() <<
                  ", ignoring payload type from now on.");
      }
    }
  }

  if (size == 0) {
    timestamp += CalculateTimestamp(1, mediaFormat);
    packet.SetTimestamp(timestamp);
    PINDEX dummy;
    return WriteData(NULL, 0, dummy);
  }

  marker = packet.GetMarker();
  const BYTE * ptr = packet.GetPayloadPtr();

  while (size > 0) {
    unsigned oldTimestamp = timestamp;

    PINDEX written;
    if (!WriteData(ptr, size, written))
      return FALSE;

    // If the Write() function did not change the timestamp then use the default
    // method of fixed frame times and sizes.
    if (oldTimestamp == timestamp)
      timestamp += CalculateTimestamp(size, mediaFormat);

    size -= written;
    ptr += written;
  }

  PTRACE_IF(1, size < 0, "Media\tRTP payload size too small, short " << -size << " bytes.");

  packet.SetTimestamp(timestamp);

  return TRUE;
}


BOOL OpalMediaStream::ReadData(BYTE * buffer, PINDEX size, PINDEX & length)
{
  RTP_DataFrame packet(size);
  if (!ReadPacket(packet))
    return FALSE;

  length = packet.GetPayloadSize();
  if (length > size)
    length = size;
  memcpy(buffer, packet.GetPayloadPtr(), length);
  timestamp = packet.GetTimestamp();
  marker = packet.GetMarker();
  return TRUE;
}


BOOL OpalMediaStream::WriteData(const BYTE * buffer, PINDEX length, PINDEX & written)
{
  written = length;
  RTP_DataFrame packet(length);
  memcpy(packet.GetPayloadPtr(), buffer, length);
  packet.SetPayloadType(mediaFormat.GetPayloadType());
  packet.SetTimestamp(timestamp);
  packet.SetMarker(marker);
  return WritePacket(packet);
}


BOOL OpalMediaStream::SetDataSize(PINDEX dataSize)
{
  if (dataSize <= 0)
    return FALSE;

  defaultDataSize = dataSize;
  return TRUE;
}


BOOL OpalMediaStream::RequiresPatchThread() const
{
  return TRUE;
}


void OpalMediaStream::EnableJitterBuffer() const
{
}


void OpalMediaStream::SetPatch(OpalMediaPatch * patch)
{
  patchMutex.Wait();
  patchThread = patch;
  patchMutex.Signal();
}


void OpalMediaStream::AddFilter(const PNotifier & Filter, const OpalMediaFormat & Stage)
{
  PWaitAndSignal Lock(patchMutex);
  if (patchThread != NULL) patchThread->AddFilter(Filter, Stage);
}


BOOL OpalMediaStream::RemoveFilter(const PNotifier & Filter, const OpalMediaFormat & Stage)
{
  PWaitAndSignal Lock(patchMutex);

  if (patchThread != NULL) return patchThread->RemoveFilter(Filter, Stage);

  return FALSE;
}

void OpalMediaStream::RemovePatch(OpalMediaPatch * /*patch*/ ) 
{ 
  SetPatch(NULL); 
}

///////////////////////////////////////////////////////////////////////////////

OpalNullMediaStream::OpalNullMediaStream(const OpalMediaFormat & mediaFormat,
                                         unsigned sessionID,
                                         BOOL isSource)
  : OpalMediaStream(mediaFormat, sessionID, isSource)
{
}


BOOL OpalNullMediaStream::ReadData(BYTE * /*buffer*/, PINDEX /*size*/, PINDEX & /*length*/)
{
  return FALSE;
}


BOOL OpalNullMediaStream::WriteData(const BYTE * /*buffer*/, PINDEX /*length*/, PINDEX & /*written*/)
{
  return FALSE;
}


BOOL OpalNullMediaStream::RequiresPatchThread() const
{
  return FALSE;
}


BOOL OpalNullMediaStream::IsSynchronous() const
{
  return FALSE;
}


///////////////////////////////////////////////////////////////////////////////

OpalRTPMediaStream::OpalRTPMediaStream(const OpalMediaFormat & mediaFormat,
                                       BOOL isSource,
                                       RTP_Session & rtp,
                                       unsigned minJitter,
                                       unsigned maxJitter)
  : OpalMediaStream(mediaFormat, rtp.GetSessionID(), isSource),
    rtpSession(rtp),
    minAudioJitterDelay(minJitter),
    maxAudioJitterDelay(maxJitter)
{
}


BOOL OpalRTPMediaStream::Open()
{
  if (isOpen)
    return TRUE;

  rtpSession.Reopen(IsSource());

  return OpalMediaStream::Open();
}


BOOL OpalRTPMediaStream::Close()
{
  PTRACE(3, "Media\tClosing RTP for " << *this);

  // Break any I/O blocks and wait for the thread that uses this object to
  // terminate before we allow it to be deleted.
  rtpSession.Close(IsSource());
  return OpalMediaStream::Close();
}


BOOL OpalRTPMediaStream::ReadPacket(RTP_DataFrame & packet)
{
  if (IsSink()) {
    PTRACE(1, "Media\tTried to read from sink media stream");
    return FALSE;
  }

  if (!rtpSession.ReadBufferedData(timestamp, packet))
    return FALSE;

  timestamp = packet.GetTimestamp();
  return TRUE;
}


BOOL OpalRTPMediaStream::WritePacket(RTP_DataFrame & packet)
{
  if (paused)
    packet.SetPayloadSize(0);
  
  if (IsSource()) {
    PTRACE(1, "Media\tTried to write to source media stream");
    return FALSE;
  }

  timestamp = packet.GetTimestamp();

  if (packet.GetPayloadSize() == 0)
    return TRUE;

  return rtpSession.WriteData(packet);
}

BOOL OpalRTPMediaStream::SetDataSize(PINDEX dataSize)
{
  if (dataSize <= 0)
    return FALSE;

  defaultDataSize = dataSize;

  /* If we are a source then we should set our buffer size to the max
     practical UDP packet size. This means we have a buffer that can accept
     whatever the RTP sender throws at us. For sink, we just clamp it to that
     maximum size. */
  if (IsSource() || defaultDataSize > RTP_DataFrame::MaxEthernetPayloadSize)
    defaultDataSize = RTP_DataFrame::MaxEthernetPayloadSize;

  return TRUE;
}

BOOL OpalRTPMediaStream::IsSynchronous() const
{
  return FALSE;
}


void OpalRTPMediaStream::EnableJitterBuffer() const
{
  if (mediaFormat.NeedsJitterBuffer())
    rtpSession.SetJitterBufferSize(minAudioJitterDelay*mediaFormat.GetTimeUnits(),
                                   maxAudioJitterDelay*mediaFormat.GetTimeUnits(),
				   mediaFormat.GetTimeUnits());
}


///////////////////////////////////////////////////////////////////////////////

OpalRawMediaStream::OpalRawMediaStream(const OpalMediaFormat & mediaFormat,
                                       unsigned sessionID,
                                       BOOL isSource,
                                       PChannel * chan, BOOL autoDel)
  : OpalMediaStream(mediaFormat, sessionID, isSource)
{
  channel = chan;
  autoDelete = autoDel;
  averageSignalSum = 0;
  averageSignalSamples = 0;
}


OpalRawMediaStream::~OpalRawMediaStream()
{
  PWaitAndSignal m(channel_mutex);
  if (autoDelete)
    delete channel;
  channel = NULL;
}


BOOL OpalRawMediaStream::ReadData(BYTE * buffer, PINDEX size, PINDEX & length)
{
  length = 0;

  if (IsSink()) {
    PTRACE(1, "Media\tTried to read from sink media stream");
    return FALSE;
  }

  PWaitAndSignal m(channel_mutex);
  if (!IsOpen() ||
      channel == NULL)
    return FALSE;

  if (!channel->Read(buffer, size))
    return FALSE;

  length = channel->GetLastReadCount();
  CollectAverage(buffer, length);
  return TRUE;
}


BOOL OpalRawMediaStream::WriteData(const BYTE * buffer, PINDEX length, PINDEX & written)
{
  written = 0;

  if (IsSource()) {
    PTRACE(1, "Media\tTried to write to source media stream");
    return FALSE;
  }
  
  PWaitAndSignal m(channel_mutex);
  if (!IsOpen() ||
      channel == NULL)
    return FALSE;

  if (buffer != NULL && length != 0) {
    if (!channel->Write(buffer, length))
      return FALSE;
    written = channel->GetLastWriteCount();
    CollectAverage(buffer, written);
  }
  else {
    PBYTEArray silence(defaultDataSize);
    if (!channel->Write(silence, defaultDataSize))
      return FALSE;
    written = channel->GetLastWriteCount();
    CollectAverage(silence, written);
  }

  return TRUE;
}


BOOL OpalRawMediaStream::Close()
{
  PTRACE(1, "Media\tClosing raw media stream " << *this);
  if (!OpalMediaStream::Close())
    return FALSE;

  PWaitAndSignal m(channel_mutex);
  if (channel == NULL)
    return FALSE;

  return channel->Close();
}


unsigned OpalRawMediaStream::GetAverageSignalLevel()
{
  PWaitAndSignal m(channel_mutex);

  if (averageSignalSamples == 0)
    return UINT_MAX;

  unsigned average = (unsigned)(averageSignalSum/averageSignalSamples);
  averageSignalSum = average;
  averageSignalSamples = 1;
  return average;
}


void OpalRawMediaStream::CollectAverage(const BYTE * buffer, PINDEX size)
{
  size = size/2;
  averageSignalSamples += size;
  const short * pcm = (const short *)buffer;
  while (size-- > 0) {
    averageSignalSum += PABS(*pcm);
    pcm++;
  }
}


///////////////////////////////////////////////////////////////////////////////

OpalFileMediaStream::OpalFileMediaStream(const OpalMediaFormat & mediaFormat,
                                         unsigned sessionID,
                                         BOOL isSource,
                                         PFile * file,
                                         BOOL autoDel)
  : OpalRawMediaStream(mediaFormat, sessionID, isSource, file, autoDel)
{
}


OpalFileMediaStream::OpalFileMediaStream(const OpalMediaFormat & mediaFormat,
                                         unsigned sessionID,
                                         BOOL isSource,
                                         const PFilePath & path)
  : OpalRawMediaStream(mediaFormat, sessionID, isSource,
                       new PFile(path, isSource ? PFile::ReadOnly : PFile::WriteOnly),
                       TRUE)
{
}


BOOL OpalFileMediaStream::IsSynchronous() const
{
  return FALSE;
}


///////////////////////////////////////////////////////////////////////////////

#if OPAL_AUDIO
#if P_AUDIO

OpalAudioMediaStream::OpalAudioMediaStream(const OpalMediaFormat & mediaFormat,
                                           unsigned sessionID,
                                           BOOL isSource,
                                           PINDEX buffers,
                                           PSoundChannel * channel,
                                           BOOL autoDel)
  : OpalRawMediaStream(mediaFormat, sessionID, isSource, channel, autoDel)
{
  soundChannelBuffers = buffers;
}


OpalAudioMediaStream::OpalAudioMediaStream(const OpalMediaFormat & mediaFormat,
                                           unsigned sessionID,
                                           BOOL isSource,
                                           PINDEX buffers,
                                           const PString & deviceName)
  : OpalRawMediaStream(mediaFormat, sessionID, isSource,
                       new PSoundChannel(deviceName,
                                         isSource ? PSoundChannel::Recorder
                                                  : PSoundChannel::Player,
                                         1, mediaFormat.GetClockRate(), 16),
                       TRUE)
{
  soundChannelBuffers = buffers;
}


BOOL OpalAudioMediaStream::SetDataSize(PINDEX dataSize)
{
  PTRACE(3, "Media\tAudio " << (IsSource() ? "source" : "sink") << " data size set to  "
         << dataSize << " bytes and " << soundChannelBuffers << " buffers.");
  return OpalMediaStream::SetDataSize(dataSize) &&
         ((PSoundChannel *)channel)->SetBuffers(dataSize, soundChannelBuffers);
}


BOOL OpalAudioMediaStream::IsSynchronous() const
{
  return TRUE;
}

#endif // P_AUDIO

#endif // OPAL_AUDIO

///////////////////////////////////////////////////////////////////////////////

#if OPAL_VIDEO

OpalVideoMediaStream::OpalVideoMediaStream(const OpalMediaFormat & mediaFormat,
                                           unsigned sessionID,
                                           PVideoInputDevice * in,
                                           PVideoOutputDevice * out,
                                           BOOL del)
  : OpalMediaStream(mediaFormat, sessionID, in != NULL),
    inputDevice(in),
    outputDevice(out),
    autoDelete(del)
{
  PAssert(in != NULL || out != NULL, PInvalidParameter);
}


OpalVideoMediaStream::~OpalVideoMediaStream()
{
  if (autoDelete) {
    delete inputDevice;
    delete outputDevice;
  }
}


BOOL OpalVideoMediaStream::SetDataSize(PINDEX dataSize)
{
  if (inputDevice != NULL) {
    PINDEX minDataSize = inputDevice->GetMaxFrameBytes();
    if (dataSize < minDataSize)
      dataSize = minDataSize;
  }
  if (outputDevice != NULL) {
    PINDEX minDataSize = outputDevice->GetMaxFrameBytes();
    if (dataSize < minDataSize)
      dataSize = minDataSize;
  }

  return OpalMediaStream::SetDataSize(sizeof(PluginCodec_Video_FrameHeader) + dataSize); 
}


BOOL OpalVideoMediaStream::Open()
{
  if (isOpen)
    return TRUE;

  unsigned width = mediaFormat.GetOptionInteger(OpalVideoFormat::FrameWidthOption, 176);
  unsigned height = mediaFormat.GetOptionInteger(OpalVideoFormat::FrameHeightOption, 144);

  if (inputDevice != NULL) {
    if (!inputDevice->SetColourFormatConverter(mediaFormat)) {
      PTRACE(1, "Media\tCould not set colour format in grabber to " << mediaFormat);
      return FALSE;
    }
    if (!inputDevice->SetFrameSizeConverter(width, height, FALSE)) {
      PTRACE(1, "Media\tCould not set frame size in grabber to " << width << 'x' << height << " in " << mediaFormat);
      return FALSE;
    }
    if (!inputDevice->Start()) {
      PTRACE(1, "Media\tCould not start video grabber");
      return FALSE;
    }
    lastGrabTime = PTimer::Tick();
  }

  if (outputDevice != NULL) {
    if (!outputDevice->SetColourFormatConverter(mediaFormat)) {
      PTRACE(1, "Media\tCould not set colour format in video display to " << mediaFormat);
      return FALSE;
    }
    if (!outputDevice->SetFrameSizeConverter(width, height, FALSE)) {
      PTRACE(1, "Media\tCould not set frame size in video display to " << width << 'x' << height << " in " << mediaFormat);
      return FALSE;
    }
    if (!outputDevice->Start()) {
      PTRACE(1, "Media\tCould not start video display device");
      return FALSE;
    }
  }

  SetDataSize(0); // Gets set to minimum of device buffer requirements

  return OpalMediaStream::Open();
}


BOOL OpalVideoMediaStream::ReadData(BYTE * data, PINDEX size, PINDEX & length)
{
  if (IsSink()) {
    PTRACE(1, "Media\tTried to read from sink media stream");
    return FALSE;
  }

  if (inputDevice == NULL) {
    PTRACE(1, "Media\tTried to read from video display device");
    return FALSE;
  }

  if (size < inputDevice->GetMaxFrameBytes()) {
    PTRACE(1, "Media\tTried to read with insufficient buffer size");
    return FALSE;
  }

  unsigned width, height;
  inputDevice->GetFrameSize(width, height);
  OpalVideoTranscoder::FrameHeader * frame = (OpalVideoTranscoder::FrameHeader *)PAssertNULL(data);
  frame->x = frame->y = 0;
  frame->width = width;
  frame->height = height;

  PINDEX bytesReturned = size;
  if (!inputDevice->GetFrameData((BYTE *)OPAL_VIDEO_FRAME_DATA_PTR(frame), &bytesReturned))
    return FALSE;

  PTimeInterval currentGrabTime = PTimer::Tick();
  timestamp += ((lastGrabTime - currentGrabTime)*1000/OpalMediaFormat::VideoClockRate).GetInterval();
  lastGrabTime = currentGrabTime;

  marker = TRUE;
  length = bytesReturned + sizeof(PluginCodec_Video_FrameHeader);

  if (outputDevice == NULL)
    return TRUE;

  return outputDevice->SetFrameData(0, 0, width, height, OPAL_VIDEO_FRAME_DATA_PTR(frame), TRUE);
}


BOOL OpalVideoMediaStream::WriteData(const BYTE * data, PINDEX length, PINDEX & written)
{
  if (IsSource()) {
    PTRACE(1, "Media\tTried to write to source media stream");
    return FALSE;
  }

  if (outputDevice == NULL) {
    PTRACE(1, "Media\tTried to write to video capture device");
    return FALSE;
  }

  // Assume we are writing the exact amount (check?)
  written = length;

  // Check for missing packets, we do nothing at this level, just ignore it
  if (data == NULL)
    return TRUE;

  const OpalVideoTranscoder::FrameHeader * frame = (const OpalVideoTranscoder::FrameHeader *)data;
  outputDevice->SetFrameSize(frame->width, frame->height);
  return outputDevice->SetFrameData(frame->x, frame->y,
                                    frame->width, frame->height,
                                    OPAL_VIDEO_FRAME_DATA_PTR(frame), marker);
}


BOOL OpalVideoMediaStream::IsSynchronous() const
{
  return IsSource();
}

#endif // OPAL_VIDEO

///////////////////////////////////////////////////////////////////////////////

OpalUDPMediaStream::OpalUDPMediaStream(const OpalMediaFormat & mediaFormat,
                                       unsigned sessionID,
                                       BOOL isSource,
                                       OpalTransportUDP & transport)
  : OpalMediaStream(mediaFormat, sessionID, isSource),
    udpTransport(transport)
{}

BOOL OpalUDPMediaStream::ReadPacket(RTP_DataFrame & Packet)
{
  Packet.SetPayloadType(mediaFormat.GetPayloadType());
  Packet.SetPayloadSize(0);

  if (IsSink()) {
    PTRACE(1, "Media\tTried to read from sink media stream");
    return FALSE;
  }

  PBYTEArray rawData;
  if (!udpTransport.ReadPDU(rawData)) {
    PTRACE(3, "Read on UDP transport failed: "
       << udpTransport.GetErrorText() << " transport: " << udpTransport);
    return FALSE;
  }

  if (rawData.GetSize() > 0) {
    Packet.SetPayloadSize(rawData.GetSize());
    memcpy(Packet.GetPayloadPtr(), rawData.GetPointer(), rawData.GetSize());
  }

  return TRUE;
}

BOOL OpalUDPMediaStream::WritePacket(RTP_DataFrame & Packet)
{
  if (IsSource()) {
    PTRACE(1, "Media\tTried to write to source media stream");
    return FALSE;
  }

  if (Packet.GetPayloadSize() > 0) {
    if (!udpTransport.Write(Packet.GetPayloadPtr(), Packet.GetPayloadSize())) {
      PTRACE(3, "Media\tWrite on UDP transport failed: "
         << udpTransport.GetErrorText() << " transport: " << udpTransport);
      return FALSE;
    }
  }

  return TRUE;
}

BOOL OpalUDPMediaStream::IsSynchronous() const
{
  return FALSE;
}

BOOL OpalUDPMediaStream::Close()
{
  if (!OpalMediaStream::Close())
    return FALSE;

  return udpTransport.Close();
}

// End of file ////////////////////////////////////////////////////////////////
