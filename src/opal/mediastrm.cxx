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
 * Revision 1.2014  2002/02/13 02:33:15  robertj
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

#include <opal/mediastrm.h>

#include <opal/patch.h>
#include <lids/lid.h>
#include <rtp/rtp.h>


#define new PNEW


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
  if (frameTime != 0 && format.GetClockRate() == OpalMediaFormat::AudioClockRate)
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
  unsigned frames = (size + mediaFormat.GetFrameSize() - 1)/mediaFormat.GetFrameSize();
  return frames*mediaFormat.GetFrameTime();
}


BOOL OpalMediaStream::ReadPacket(RTP_DataFrame & packet)
{
  unsigned oldTimestamp = timestamp;

  PINDEX lastReadCount;
  if (!ReadData(packet.GetPayloadPtr(), defaultDataSize, lastReadCount))
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
  patchThread = patch;
}


///////////////////////////////////////////////////////////////////////////////

OpalNullMediaStream::OpalNullMediaStream(BOOL isSourceStream, unsigned sessionID)
  : OpalMediaStream(isSourceStream, sessionID)
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

OpalRTPMediaStream::OpalRTPMediaStream(BOOL isSourceStream,
                                       RTP_Session & rtp)
  : OpalMediaStream(isSourceStream, rtp.GetSessionID()),
    rtpSession(rtp)
{
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
  if (!rtpSession.ReadBufferedData(timestamp, packet))
    return FALSE;

  timestamp = packet.GetTimestamp();
  return TRUE;
}


BOOL OpalRTPMediaStream::WritePacket(RTP_DataFrame & packet)
{
  timestamp = packet.GetTimestamp();
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


BOOL OpalLineMediaStream::ReadData(BYTE * buffer, PINDEX size, PINDEX & length)
{
  length = 0;

  if (IsSink())
    return FALSE;

  if (useDeblocking) {
    line.SetReadFrameSize(size);
    if (line.ReadBlock(buffer, size)) {
      length = size;
      return TRUE;
    }
  }
  else {
    if (line.ReadFrame(buffer, length)) {
      // In the case of G.723.1 remember the last SID frame we sent and send
      // it again if the hardware sends us a CNG frame.
      if (mediaFormat.GetPayloadType() == RTP_DataFrame::G7231) {
        switch (length) {
          case 1 : // CNG frame
            memcpy(buffer, lastSID, 4);
            length = 4;
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

  return FALSE;
}


BOOL OpalLineMediaStream::WriteData(const BYTE * buffer, PINDEX length, PINDEX & written)
{
  written = 0;

  if (IsSource())
    return FALSE;

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
        buffer = silenceBuffer.GetPointer(line.GetWriteFrameSize());
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
        buffer = silenceBuffer.GetPointer(line.GetWriteFrameSize()); // Fills with zeros
        length = silenceBuffer.GetSize();
        break;
    }
  }

  if (useDeblocking) {
    line.SetWriteFrameSize(length);
    if (line.WriteBlock(buffer, length)) {
      written = length;
      return TRUE;
    }
  }
  else {
    if (line.WriteFrame(buffer, length, written))
      return TRUE;
  }

  DWORD osError = line.GetDevice().GetErrorNumber();
  PTRACE_IF(1, osError != 0, "Media\tLID write frame error: " << line.GetDevice().GetErrorText());

  return FALSE;
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

  return TRUE;
}


BOOL OpalLineMediaStream::Close()
{
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
}


OpalRawMediaStream::~OpalRawMediaStream()
{
  if (autoDelete)
    delete channel;
}


BOOL OpalRawMediaStream::ReadData(BYTE * buffer, PINDEX size, PINDEX & length)
{
  length = 0;

  if (channel == NULL)
    return FALSE;

  if (!channel->Read(buffer, size))
    return FALSE;

  length = channel->GetLastReadCount();
  return TRUE;
}


BOOL OpalRawMediaStream::WriteData(const BYTE * buffer, PINDEX length, PINDEX & written)
{
  written = 0;

  if (channel == NULL)
    return FALSE;

  if (!channel->Write(buffer, length))
    return FALSE;

  written = channel->GetLastWriteCount();
  return TRUE;
}


BOOL OpalRawMediaStream::Close()
{
  OpalMediaStream::Close();

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
