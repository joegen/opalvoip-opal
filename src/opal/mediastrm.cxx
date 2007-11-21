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
 * $Revision$
 * $Author$
 * $Date$
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
#include <opal/connection.h>
#include <opal/call.h>

#define MAX_PAYLOAD_TYPE_MISMATCHES 10


#define new PNEW


///////////////////////////////////////////////////////////////////////////////

OpalMediaStream::OpalMediaStream(OpalConnection & connection, const OpalMediaFormat & fmt, unsigned _sessionID, BOOL isSourceStream)
  : mediaFormat(fmt), 
    sessionID(_sessionID), 
    paused(FALSE), 
    isSource(isSourceStream), 
    isOpen(FALSE), 
    defaultDataSize(0),
    timestamp(0), 
    marker(TRUE),
    mismatchedPayloadTypes(0),
    mediaPatch(NULL)
{
  // Set default frame size to 50ms of audio, otherwise just one frame
  unsigned frameTime = mediaFormat.GetFrameTime();
  if (sessionID == OpalMediaFormat::DefaultAudioSessionID && 
      frameTime != 0 && 
      mediaFormat.GetClockRate() == OpalMediaFormat::AudioClockRate)
    SetDataSize(((400+frameTime-1)/frameTime)*mediaFormat.GetFrameSize());
  else
    SetDataSize(mediaFormat.GetFrameSize());

  PString tok = connection.GetCall().GetToken();

  id = connection.GetCall().GetToken() + psprintf("_%i", sessionID);
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


BOOL OpalMediaStream::UpdateMediaFormat(const OpalMediaFormat & newMediaFormat)
{
  PWaitAndSignal mutex(patchMutex);

  // If we are source, then update the sink side, and vice versa
  if (mediaPatch != NULL) {
    if (!mediaPatch->UpdateMediaFormat(newMediaFormat, IsSink())) {
      PTRACE(2, "Media\tPatch did not allow media format update of " << *this);
      return FALSE;
    }
  }

  mediaFormat = newMediaFormat;

  PTRACE(4, "Media\tMedia format updated on " << *this);

  return TRUE;
}


BOOL OpalMediaStream::ExecuteCommand(const OpalMediaCommand & command)
{
  PWaitAndSignal mutex(patchMutex);

  if (mediaPatch == NULL)
    return FALSE;

  return mediaPatch->ExecuteCommand(command, IsSink());
}


void OpalMediaStream::SetCommandNotifier(const PNotifier & notifier)
{
  PWaitAndSignal mutex(patchMutex);

  if (mediaPatch != NULL)
    mediaPatch->SetCommandNotifier(notifier, IsSink());

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
  if (mediaPatch != NULL) {
    mediaPatch->Start();
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

  if (mediaPatch != NULL) {
    PTRACE(4, "Media\tDisconnecting " << *this << " from patch thread " << *mediaPatch);
    OpalMediaPatch * patch = mediaPatch;
    mediaPatch = NULL;

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
  
  if (size > 0 && mediaFormat.IsTransportable()) {
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


BOOL OpalMediaStream::PushPacket(RTP_DataFrame & packet)
{
  if(mediaPatch == NULL) {
    return FALSE;
  }
	
  return mediaPatch->PushFrame(packet);
}


BOOL OpalMediaStream::SetDataSize(PINDEX dataSize)
{
  if (dataSize <= 0)
    return FALSE;

  defaultDataSize = dataSize;
  return TRUE;
}


BOOL OpalMediaStream::RequiresPatch() const
{
  return TRUE;
}


BOOL OpalMediaStream::RequiresPatchThread() const
{
  return TRUE;
}


void OpalMediaStream::EnableJitterBuffer() const
{
}


BOOL OpalMediaStream::SetPatch(OpalMediaPatch * patch)
{
  PWaitAndSignal m(patchMutex);
  if (IsOpen()) {
    mediaPatch = patch;
    return TRUE;
  }
  return FALSE;
}


void OpalMediaStream::AddFilter(const PNotifier & Filter, const OpalMediaFormat & Stage)
{
  PWaitAndSignal Lock(patchMutex);
  if (mediaPatch != NULL) mediaPatch->AddFilter(Filter, Stage);
}


BOOL OpalMediaStream::RemoveFilter(const PNotifier & Filter, const OpalMediaFormat & Stage)
{
  PWaitAndSignal Lock(patchMutex);

  if (mediaPatch != NULL) return mediaPatch->RemoveFilter(Filter, Stage);

  return FALSE;
}

void OpalMediaStream::RemovePatch(OpalMediaPatch * /*patch*/ ) 
{ 
  SetPatch(NULL); 
}

///////////////////////////////////////////////////////////////////////////////

OpalNullMediaStream::OpalNullMediaStream(OpalConnection & conn,
                                        const OpalMediaFormat & mediaFormat,
                                         unsigned sessionID,
                                         BOOL isSource)
  : OpalMediaStream(conn, mediaFormat, sessionID, isSource)
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


BOOL OpalNullMediaStream::RequiresPatch() const
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

OpalRTPMediaStream::OpalRTPMediaStream(OpalConnection & conn,
                                      const OpalMediaFormat & mediaFormat,
                                       BOOL isSource,
                                       RTP_Session & rtp,
                                       unsigned minJitter,
                                       unsigned maxJitter)
  : OpalMediaStream(conn, mediaFormat, rtp.GetSessionID(), isSource),
    rtpSession(rtp),
    minAudioJitterDelay(minJitter),
    maxAudioJitterDelay(maxJitter)
{
  defaultDataSize = RTP_DataFrame::MaxEthernetPayloadSize;
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

#if OPAL_VIDEO
void OpalRTPMediaStream::OnPatchStart()
{
    OpalMediaStream::OnPatchStart();
    // We add the command notifer here so that the OpalPluginVideoTranscoder 
    // patch gets its command notifier updated as well.  This lets us catch 
    // OpalVideoUpdatePicture commands from the decoder
    if(sessionID == OpalMediaFormat::DefaultVideoSessionID && isSource)
      SetCommandNotifier(PCREATE_NOTIFIER(OnMediaCommand));
}

void OpalRTPMediaStream::OnMediaCommand(OpalMediaCommand &command, INT /*extra*/)
{
  if (PIsDescendant(&command, OpalVideoUpdatePicture))
    rtpSession.SendIntraFrameRequest();
}
#endif


///////////////////////////////////////////////////////////////////////////////

OpalRawMediaStream::OpalRawMediaStream(OpalConnection & conn,
                                     const OpalMediaFormat & mediaFormat,
                                       unsigned sessionID,
                                       BOOL isSource,
                                       PChannel * chan, BOOL autoDel)
  : OpalMediaStream(conn, mediaFormat, sessionID, isSource)
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
  PTRACE(3, "Media\tClosing raw media stream " << *this);
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

OpalFileMediaStream::OpalFileMediaStream(OpalConnection & conn,
                                         const OpalMediaFormat & mediaFormat,
                                         unsigned sessionID,
                                         BOOL isSource,
                                         PFile * file,
                                         BOOL autoDel)
  : OpalRawMediaStream(conn, mediaFormat, sessionID, isSource, file, autoDel)
{
}


OpalFileMediaStream::OpalFileMediaStream(OpalConnection & conn,
                                         const OpalMediaFormat & mediaFormat,
                                         unsigned sessionID,
                                         BOOL isSource,
                                         const PFilePath & path)
  : OpalRawMediaStream(conn, mediaFormat, sessionID, isSource,
                       new PFile(path, isSource ? PFile::ReadOnly : PFile::WriteOnly),
                       TRUE)
{
}


BOOL OpalFileMediaStream::IsSynchronous() const
{
  return FALSE;
}

BOOL OpalFileMediaStream::ReadData(
  BYTE * data,      ///<  Data buffer to read to
  PINDEX size,      ///<  Size of buffer
  PINDEX & length   ///<  Length of data actually read
)
{
  if (!OpalRawMediaStream::ReadData(data, size, length))
    return FALSE;

  // only delay if audio
  if (sessionID == 1)
    fileDelay.Delay(length/16);

  return TRUE;
}

/**Write raw media data to the sink media stream.
   The default behaviour writes to the PChannel object.
  */
BOOL OpalFileMediaStream::WriteData(
  const BYTE * data,   ///<  Data to write
  PINDEX length,       ///<  Length of data to read.
  PINDEX & written     ///<  Length of data actually written
)
{
  if (!OpalRawMediaStream::WriteData(data, length, written))
    return FALSE;

  // only delay if audio
  if (sessionID == 1)
    fileDelay.Delay(written/16);

  return TRUE;
}

///////////////////////////////////////////////////////////////////////////////

#if OPAL_AUDIO
#if P_AUDIO

OpalAudioMediaStream::OpalAudioMediaStream(OpalConnection & conn,
                                           const OpalMediaFormat & mediaFormat,
                                           unsigned sessionID,
                                           BOOL isSource,
                                           PINDEX buffers,
                                           PSoundChannel * channel,
                                           BOOL autoDel)
  : OpalRawMediaStream(conn, mediaFormat, sessionID, isSource, channel, autoDel)
{
  soundChannelBuffers = buffers;
}


OpalAudioMediaStream::OpalAudioMediaStream(OpalConnection & conn,
                                           const OpalMediaFormat & mediaFormat,
                                           unsigned sessionID,
                                           BOOL isSource,
                                           PINDEX buffers,
                                           const PString & deviceName)
  : OpalRawMediaStream(conn, mediaFormat, sessionID, isSource,
                       PSoundChannel::CreateOpenedChannel(PString::Empty(),
                                                          deviceName,
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

OpalVideoMediaStream::OpalVideoMediaStream(OpalConnection & conn,
                                          const OpalMediaFormat & mediaFormat,
                                           unsigned sessionID,
                                           PVideoInputDevice * in,
                                           PVideoOutputDevice * out,
                                           BOOL del)
  : OpalMediaStream(conn, mediaFormat, sessionID, in != NULL),
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

  unsigned width = mediaFormat.GetOptionInteger(OpalVideoFormat::FrameWidthOption(), PVideoFrameInfo::QCIFWidth);
  unsigned height = mediaFormat.GetOptionInteger(OpalVideoFormat::FrameHeightOption(), PVideoFrameInfo::QCIFHeight);

  if (inputDevice != NULL) {
    if (!inputDevice->SetColourFormatConverter(mediaFormat)) {
      PTRACE(1, "Media\tCould not set colour format in grabber to " << mediaFormat);
      return FALSE;
    }
    if (!inputDevice->SetFrameSizeConverter(width, height)) {
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
    if (!outputDevice->SetFrameSizeConverter(width, height)) {
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

  PINDEX bytesReturned = size - sizeof(OpalVideoTranscoder::FrameHeader);
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

OpalUDPMediaStream::OpalUDPMediaStream(OpalConnection & conn,
                                      const OpalMediaFormat & mediaFormat,
                                       unsigned sessionID,
                                       BOOL isSource,
                                       OpalTransportUDP & transport)
  : OpalMediaStream(conn, mediaFormat, sessionID, isSource),
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
    PTRACE(2, "Media\tRead on UDP transport failed: "
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
      PTRACE(2, "Media\tWrite on UDP transport failed: "
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
