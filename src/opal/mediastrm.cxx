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
 * Revision 1.2011  2002/01/14 02:24:33  robertj
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

#include <opal/mediastrm.h>

#include <opal/patch.h>
#include <lids/lid.h>
#include <rtp/rtp.h>


#define new PNEW

#define	MAX_PAYLOAD_TYPE_MISMATCHES 8


///////////////////////////////////////////////////////////////////////////////

OpalMediaStream::OpalMediaStream(BOOL isSourceStream, unsigned id)
{
  isSource = isSourceStream;
  sessionID = id;
  timestamp = 0;
  marker = TRUE;
  patchThread = NULL;
}


OpalMediaStream::~OpalMediaStream()
{
  Close();
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


BOOL OpalMediaStream::Open(const OpalMediaFormat & format)
{
  PTRACE(3, "Media\tOpening " << format
         << " as " << (IsSource() ? "source" : "sink"));

  mediaFormat = format;

  // Set default frame size to 50ms of audio, otherwise just one frame
  unsigned frameTime = format.GetFrameTime();
  if (frameTime != 0 && format.GetTimeUnits() == OpalMediaFormat::AudioTimeUnits)
    defaultDataSize = ((400+frameTime-1)/frameTime)*format.GetFrameSize();
  else
    defaultDataSize = format.GetFrameSize();

  return TRUE;
}


BOOL OpalMediaStream::Start()
{
  if (patchThread != NULL && patchThread->IsSuspended())
    patchThread->Resume();
  return TRUE;
}


BOOL OpalMediaStream::Close()
{
  if (patchThread != NULL) {
    OpalMediaPatch * patch = patchThread;
    patchThread = NULL;

    if (IsSink())
      patch->RemoveSink(this);
    else {
      patch->Close();
      delete patch;
    }
  }

  return TRUE;
}


inline static unsigned CalculateTimestamp(PINDEX size, const OpalMediaFormat & mediaFormat)
{
  unsigned frames = (size + mediaFormat.GetFrameSize() - 1)/mediaFormat.GetFrameSize();
  return frames*mediaFormat.GetFrameTime();
}


BOOL OpalMediaStream::ReadPacket(RTP_DataFrame & packet)
{
  unsigned oldTimestamp = timestamp;

  if (!Read(packet.GetPayloadPtr(), defaultDataSize))
    return FALSE;

  // If the Write() function did not change the timestamp then use the default
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
  int size = packet.GetPayloadSize();

  if (size == 0) {
    timestamp += CalculateTimestamp(1, mediaFormat);
    packet.SetTimestamp(timestamp);
    return Write(NULL, 0);
  }

  marker = packet.GetMarker();
  const BYTE * ptr = packet.GetPayloadPtr();

  while (size > 0) {
    unsigned oldTimestamp = timestamp;

    if (!Write(ptr, size))
      return FALSE;

    // If the Write() function did not change the timestamp then use the default
    // method of fixed frame times and sizes.
    if (oldTimestamp == timestamp)
      timestamp += CalculateTimestamp(lastWriteCount, mediaFormat);

    size -= lastWriteCount;
    ptr += lastWriteCount;
  }

  PTRACE_IF(1, size < 0, "Media\tRTP payload size too small, short " << -size << " bytes.");

  packet.SetTimestamp(timestamp);

  return TRUE;
}


BOOL OpalMediaStream::SetDataSize(PINDEX dataSize)
{
  defaultDataSize = dataSize;
  return TRUE;
}


void OpalMediaStream::EnableJitterBuffer() const
{
}


void OpalMediaStream::SetPatch(OpalMediaPatch * patch)
{
  patchThread = patch;
}


///////////////////////////////////////////////////////////////////////////////

OpalRTPMediaStream::OpalRTPMediaStream(BOOL isSourceStream, RTP_Session & rtp)
  : OpalMediaStream(isSourceStream, rtp.GetSessionID()),
    rtpSession(rtp)
{
  os_handle = 1; // RTP session is always open
}


BOOL OpalRTPMediaStream::Open(const OpalMediaFormat & format)
{
  if (!OpalMediaStream::Open(format))
    return FALSE;

  consecutiveMismatches = 0;
  mismatchPayloadType = mediaFormat.GetPayloadType();
  return TRUE;
}


BOOL OpalRTPMediaStream::Close()
{
  PTRACE(3, "Media\tClosing RTP for " << *this);

  // Break any I/O blocks and wait for the thread that uses this object to
  // terminate before we allow it to be deleted.
  rtpSession.Close(IsSource());
  return OpalMediaStream::Close();
}


BOOL OpalRTPMediaStream::Read(void * buf, PINDEX len)
{
  RTP_DataFrame packet(len);
  if (!ReadPacket(packet))
    return FALSE;

  lastReadCount = packet.GetPayloadSize();
  if (lastReadCount > len)
    lastReadCount = len;
  memcpy(buf, packet.GetPayloadPtr(), lastReadCount);
  timestamp = packet.GetTimestamp();
  marker = packet.GetMarker();
  return TRUE;
}


BOOL OpalRTPMediaStream::Write(const void * buf, PINDEX len)
{
  RTP_DataFrame packet(len);
  memcpy(packet.GetPayloadPtr(), buf, len);
  packet.SetTimestamp(timestamp);
  packet.SetMarker(marker);
  return WritePacket(packet);
}


BOOL OpalRTPMediaStream::ReadPacket(RTP_DataFrame & packet)
{
  if (!rtpSession.ReadBufferedData(timestamp, packet))
    return FALSE;

  timestamp = packet.GetTimestamp();

  // Empty packet indicates silence (jitter buffer underrun)
  if (packet.GetPayloadSize() == 0)
    return TRUE;

  // Have data, confirm payload type
  if (packet.GetPayloadType() == mismatchPayloadType) {
    consecutiveMismatches = 0;
    return TRUE;
  }

  // Ignore first few in the case of some pathalogical conditions by some stacks
  if (consecutiveMismatches < MAX_PAYLOAD_TYPE_MISMATCHES) {
    packet.SetPayloadSize(0);
    consecutiveMismatches++;
    return TRUE;
  }

  // After a few RTP packets with the wrong payload type, we start using it
  // anyway as some stacks out there just put in the wrong value
  PTRACE(1, "Media\tRTP payload type mismatch: expected "
           << mediaFormat.GetPayloadType() << ", got " << packet.GetPayloadType()
           << ". Ignoring from now on.");
  mismatchPayloadType = packet.GetPayloadType();

  return TRUE;
}


BOOL OpalRTPMediaStream::WritePacket(RTP_DataFrame & packet)
{
  packet.SetPayloadType(mediaFormat.GetPayloadType());
  return rtpSession.WriteData(packet);
}


BOOL OpalRTPMediaStream::IsSynchronous() const
{
  return FALSE;
}


void OpalRTPMediaStream::EnableJitterBuffer() const
{
  rtpSession.SetJitterBufferSize(150);
}


///////////////////////////////////////////////////////////////////////////////

OpalLineMediaStream::OpalLineMediaStream(BOOL isSourceStream, unsigned id, OpalLine & ln)
  : OpalMediaStream(isSourceStream, id),
    line(ln)
{
  useDeblocking = FALSE;
  missedCount = 0;
  lastSID[0] = 2;
  lastFrameWasSignal = TRUE;
}


BOOL OpalLineMediaStream::Read(void * buffer, PINDEX length)
{
  lastReadCount = 0;

  if (IsSink())
    return SetErrorValues(Miscellaneous, EINVAL);

  if (useDeblocking) {
    line.SetReadFrameSize(length);
    if (line.ReadBlock(buffer, length)) {
      lastReadCount = length;
      return TRUE;
    }
  }
  else {
    if (line.ReadFrame(buffer, lastReadCount)) {
      // In the case of G.723.1 remember the last SID frame we sent and send
      // it again if the hardware sends us a CNG frame.
      if (mediaFormat.GetPayloadType() == RTP_DataFrame::G7231) {
        switch (lastReadCount) {
          case 1 : // CNG frame
            memcpy(buffer, lastSID, 4);
            lastReadCount = 4;
            lastFrameWasSignal = FALSE;
            break;
          case 4 :
            if ((*(BYTE *)buffer&3) == 2)
              memcpy(lastSID, buffer, 4);
            lastFrameWasSignal = FALSE;
            break;
          default :
            lastFrameWasSignal = TRUE;
        }
      }
      return TRUE;
    }
  }

  DWORD osError = line.GetDevice().GetErrorNumber();
  PTRACE_IF(1, osError != 0, "Media\tDevice read frame error: " << line.GetDevice().GetErrorText());

  return SetErrorValues(Miscellaneous, osError, LastReadError);
}


BOOL OpalLineMediaStream::Write(const void * buffer, PINDEX length)
{
  lastWriteCount = 0;

  if (IsSource())
    return SetErrorValues(Miscellaneous, EINVAL);

  // Check for writing silence
  PBYTEArray silenceBuffer;
  if (length != 0)
    missedCount = 0;
  else {
    switch (mediaFormat.GetPayloadType()) {
      case RTP_DataFrame::G7231 :
        if (missedCount++ < 4) {
          static const BYTE g723_erasure_frame[24] = { 0xff, 0xff, 0xff, 0xff };
          buffer = g723_erasure_frame;
          length = 24;
        }
        else {
          static const BYTE g723_cng_frame[4] = { 3 };
          buffer = g723_cng_frame;
          length = 1;
        }
        break;

      case RTP_DataFrame::PCMU :
      case RTP_DataFrame::PCMA :
        buffer = (const void *)silenceBuffer.GetPointer(line.GetWriteFrameSize());
        length = silenceBuffer.GetSize();
        memset((void *)buffer, 0xff, length);
        break;

      case RTP_DataFrame::G729 :
        if (mediaFormat.Find('B') != P_MAX_INDEX) {
          static const BYTE g729_sid_frame[2] = { 1 };
          buffer = g729_sid_frame;
          length = 2;
          break;
        }
        // Else fall into default case

      default :
        buffer = (const void *)silenceBuffer.GetPointer(line.GetWriteFrameSize()); // Fills with zeros
        length = silenceBuffer.GetSize();
        break;
    }
  }

  if (useDeblocking) {
    line.SetWriteFrameSize(length);
    if (line.WriteBlock(buffer, length)) {
      lastWriteCount = length;
      return TRUE;
    }
  }
  else {
    if (line.WriteFrame(buffer, length, lastWriteCount))
      return TRUE;
  }

  DWORD osError = line.GetDevice().GetErrorNumber();
  PTRACE_IF(1, osError != 0, "Media\tLID write frame error: " << line.GetDevice().GetErrorText());

  return SetErrorValues(Miscellaneous, osError, LastWriteError);
}


BOOL OpalLineMediaStream::Open(const OpalMediaFormat & format)
{
  if (!OpalMediaStream::Open(format))
    return FALSE;

  if (IsSource()) {
    if (!line.SetReadFormat(mediaFormat))
      return FALSE;
    useDeblocking = mediaFormat.GetFrameSize() != line.GetReadFrameSize();
  }
  else {
    if (!line.SetWriteFormat(mediaFormat))
      return FALSE;
    useDeblocking = mediaFormat.GetFrameSize() != line.GetWriteFrameSize();
  }

  PTRACE(3, "Media\tStream set to " << mediaFormat << ", frame size: rd="
         << line.GetReadFrameSize() << " wr="
         << line.GetWriteFrameSize() << ", "
         << (useDeblocking ? "needs" : "no") << " reblocking.");

  os_handle = 1;
  return TRUE;
}


BOOL OpalLineMediaStream::Close()
{
  os_handle = -1;

  if (IsSource())
    line.StopReadCodec();
  else
    line.StopWriteCodec();

  return OpalMediaStream::Close();
}


BOOL OpalLineMediaStream::IsSynchronous() const
{
  return TRUE;
}


///////////////////////////////////////////////////////////////////////////////

OpalRawMediaStream::OpalRawMediaStream(BOOL isSourceStream, unsigned id, PChannel * chan, BOOL autoDel)
  : OpalMediaStream(isSourceStream, id)
{
  channel = chan;
  autoDelete = autoDel;

  os_handle = channel->IsOpen() ? 1 : -1;
}


OpalRawMediaStream::~OpalRawMediaStream()
{
  if (autoDelete)
    delete channel;
}


BOOL OpalRawMediaStream::Read(void * buf, PINDEX len)
{
  lastReadCount = 0;

  if (channel == NULL)
    return FALSE;

  if (!channel->Read(buf, len))
    return FALSE;

  lastReadCount = channel->GetLastReadCount();
  return TRUE;
}


BOOL OpalRawMediaStream::Write(const void * buf, PINDEX len)
{
  lastWriteCount = 0;

  if (channel == NULL)
    return FALSE;

  if (!channel->Write(buf, len))
    return FALSE;

  lastWriteCount = channel->GetLastWriteCount();
  return TRUE;
}


BOOL OpalRawMediaStream::Close()
{
  OpalMediaStream::Close();

  os_handle = -1;

  if (channel == NULL)
    return FALSE;

  return channel->Close();
}


///////////////////////////////////////////////////////////////////////////////

OpalFileMediaStream::OpalFileMediaStream(BOOL isSourceStream,
                                         unsigned id,
                                         PFile * file,
                                         BOOL autoDel)
  : OpalRawMediaStream(isSourceStream, id, file, autoDel)
{
}


OpalFileMediaStream::OpalFileMediaStream(BOOL isSourceStream,
                                         unsigned id,
                                         const PFilePath & path)
  : OpalRawMediaStream(isSourceStream,
                       id,
                       new PFile(path, isSourceStream ? PFile::ReadOnly : PFile::WriteOnly),
                       TRUE)
{
}


BOOL OpalFileMediaStream::IsSynchronous() const
{
  return FALSE;
}


///////////////////////////////////////////////////////////////////////////////

OpalAudioMediaStream::OpalAudioMediaStream(BOOL isSourceStream,
                                           unsigned id,
                                           PINDEX buffers,
                                           PSoundChannel * channel,
                                           BOOL autoDel)
  : OpalRawMediaStream(isSourceStream, id, channel, autoDel)
{
  soundChannelBuffers = buffers;
}


OpalAudioMediaStream::OpalAudioMediaStream(BOOL isSourceStream,
                                           unsigned id,
                                           PINDEX buffers,
                                           const PString & deviceName)
  : OpalRawMediaStream(isSourceStream,
                       id,
                       new PSoundChannel(deviceName,
                                         isSource ? PSoundChannel::Recorder
                                                  : PSoundChannel::Player,
                                         1, 8000, 16),
                       FALSE)
{
  soundChannelBuffers = buffers;
}


BOOL OpalAudioMediaStream::SetDataSize(PINDEX dataSize)
{
  return ((PSoundChannel *)channel)->SetBuffers(dataSize, soundChannelBuffers);
}


BOOL OpalAudioMediaStream::IsSynchronous() const
{
  return TRUE;
}


///////////////////////////////////////////////////////////////////////////////

OpalVideoMediaStream::OpalVideoMediaStream(BOOL isSourceStream,
                                           unsigned sessionID,
                                           PVideoDevice & dev)
  : OpalMediaStream(isSourceStream, sessionID),
    device(dev)
{
}


BOOL OpalVideoMediaStream::Read(void * /*buf*/, PINDEX /*len*/)
{
  return FALSE;
}


BOOL OpalVideoMediaStream::Write(const void * /*buf*/, PINDEX /*len*/)
{
  return FALSE;
}


// End of file ////////////////////////////////////////////////////////////////
