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

OpalMediaStream::OpalMediaStream(OpalConnection & connection, const OpalMediaFormat & fmt, unsigned _sessionID, PBoolean isSourceStream)
  : mediaFormat(fmt), 
    sessionID(_sessionID), 
    paused(PFalse), 
    isSource(isSourceStream), 
    isOpen(PFalse), 
    defaultDataSize(0),
    timestamp(0), 
    marker(PTrue),
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


PBoolean OpalMediaStream::UpdateMediaFormat(const OpalMediaFormat & newMediaFormat)
{
  PWaitAndSignal mutex(patchMutex);

  // If we are source, then update the sink side, and vice versa
  if (mediaPatch != NULL) {
    if (!mediaPatch->UpdateMediaFormat(newMediaFormat, IsSink())) {
      PTRACE(2, "Media\tPatch did not allow media format update of " << *this);
      return PFalse;
    }
  }

  mediaFormat = newMediaFormat;

  PTRACE(4, "Media\tMedia format updated on " << *this);

  return PTrue;
}


PBoolean OpalMediaStream::ExecuteCommand(const OpalMediaCommand & command)
{
  PWaitAndSignal mutex(patchMutex);

  if (mediaPatch == NULL)
    return PFalse;

  return mediaPatch->ExecuteCommand(command, IsSink());
}


void OpalMediaStream::SetCommandNotifier(const PNotifier & notifier)
{
  PWaitAndSignal mutex(patchMutex);

  if (mediaPatch != NULL)
    mediaPatch->SetCommandNotifier(notifier, IsSink());

  commandNotifier = notifier;
}


PBoolean OpalMediaStream::Open()
{
  isOpen = PTrue;
  return PTrue;
}


PBoolean OpalMediaStream::Start()
{
  if (!Open())
    return PFalse;

  patchMutex.Wait();
  if (mediaPatch != NULL) {
    mediaPatch->Start();
  }
  patchMutex.Signal();

  return PTrue;
}


PBoolean OpalMediaStream::Close()
{
  if (!isOpen)
    return PFalse;

  PTRACE(4, "Media\tClosing stream " << *this);

  patchMutex.Wait();

  isOpen = PFalse;

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

  return PTrue;
}


PBoolean OpalMediaStream::WritePackets(RTP_DataFrameList & packets)
{
  for (PINDEX i = 0; i < packets.GetSize(); i++) {
    if (!WritePacket(packets[i]))
      return PFalse;
  }

  return PTrue;
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


PBoolean OpalMediaStream::ReadPacket(RTP_DataFrame & packet)
{
  unsigned oldTimestamp = timestamp;

  PINDEX lastReadCount;
  if (!ReadData(packet.GetPayloadPtr(), defaultDataSize, lastReadCount))
    return PFalse;

  // If the ReadData() function did not change the timestamp then use the default
  // method or fixed frame times and sizes.
  if (oldTimestamp == timestamp)
    timestamp += CalculateTimestamp(lastReadCount, mediaFormat);

  packet.SetPayloadType(mediaFormat.GetPayloadType());
  packet.SetPayloadSize(lastReadCount);
  packet.SetTimestamp(oldTimestamp); // Beginning of frame
  packet.SetMarker(marker);
  marker = PFalse;

  return PTrue;
}


PBoolean OpalMediaStream::WritePacket(RTP_DataFrame & packet)
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
      return PFalse;

    // If the Write() function did not change the timestamp then use the default
    // method of fixed frame times and sizes.
    if (oldTimestamp == timestamp)
      timestamp += CalculateTimestamp(size, mediaFormat);

    size -= written;
    ptr += written;
  }

  PTRACE_IF(1, size < 0, "Media\tRTP payload size too small, short " << -size << " bytes.");

  packet.SetTimestamp(timestamp);

  return PTrue;
}


PBoolean OpalMediaStream::ReadData(BYTE * buffer, PINDEX size, PINDEX & length)
{
  RTP_DataFrame packet(size);
  if (!ReadPacket(packet))
    return PFalse;

  length = packet.GetPayloadSize();
  if (length > size)
    length = size;
  memcpy(buffer, packet.GetPayloadPtr(), length);
  timestamp = packet.GetTimestamp();
  marker = packet.GetMarker();
  return PTrue;
}


PBoolean OpalMediaStream::WriteData(const BYTE * buffer, PINDEX length, PINDEX & written)
{
  written = length;
  RTP_DataFrame packet(length);
  memcpy(packet.GetPayloadPtr(), buffer, length);
  packet.SetPayloadType(mediaFormat.GetPayloadType());
  packet.SetTimestamp(timestamp);
  packet.SetMarker(marker);
  return WritePacket(packet);
}


PBoolean OpalMediaStream::PushPacket(RTP_DataFrame & packet)
{
  if(mediaPatch == NULL) {
    return PFalse;
  }
	
  return mediaPatch->PushFrame(packet);
}


PBoolean OpalMediaStream::SetDataSize(PINDEX dataSize)
{
  if (dataSize <= 0)
    return PFalse;

  defaultDataSize = dataSize;
  return PTrue;
}


PBoolean OpalMediaStream::RequiresPatch() const
{
  return PTrue;
}


PBoolean OpalMediaStream::RequiresPatchThread() const
{
  return PTrue;
}


void OpalMediaStream::EnableJitterBuffer() const
{
}


PBoolean OpalMediaStream::SetPatch(OpalMediaPatch * patch)
{
  PWaitAndSignal m(patchMutex);
  if (IsOpen()) {
    mediaPatch = patch;
    return PTrue;
  }
  return PFalse;
}


void OpalMediaStream::AddFilter(const PNotifier & Filter, const OpalMediaFormat & Stage)
{
  PWaitAndSignal Lock(patchMutex);
  if (mediaPatch != NULL) mediaPatch->AddFilter(Filter, Stage);
}


PBoolean OpalMediaStream::RemoveFilter(const PNotifier & Filter, const OpalMediaFormat & Stage)
{
  PWaitAndSignal Lock(patchMutex);

  if (mediaPatch != NULL) return mediaPatch->RemoveFilter(Filter, Stage);

  return PFalse;
}

void OpalMediaStream::RemovePatch(OpalMediaPatch * /*patch*/ ) 
{ 
  SetPatch(NULL); 
}

///////////////////////////////////////////////////////////////////////////////

OpalNullMediaStream::OpalNullMediaStream(OpalConnection & conn,
                                        const OpalMediaFormat & mediaFormat,
                                         unsigned sessionID,
                                         PBoolean isSource)
  : OpalMediaStream(conn, mediaFormat, sessionID, isSource)
{
}


PBoolean OpalNullMediaStream::ReadData(BYTE * /*buffer*/, PINDEX /*size*/, PINDEX & /*length*/)
{
  return PFalse;
}


PBoolean OpalNullMediaStream::WriteData(const BYTE * /*buffer*/, PINDEX /*length*/, PINDEX & /*written*/)
{
  return PFalse;
}


PBoolean OpalNullMediaStream::RequiresPatch() const
{
	return PFalse;
}


PBoolean OpalNullMediaStream::RequiresPatchThread() const
{
  return PFalse;
}


PBoolean OpalNullMediaStream::IsSynchronous() const
{
  return PFalse;
}


///////////////////////////////////////////////////////////////////////////////

OpalRTPMediaStream::OpalRTPMediaStream(OpalConnection & conn,
                                      const OpalMediaFormat & mediaFormat,
                                       PBoolean isSource,
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


PBoolean OpalRTPMediaStream::Open()
{
  if (isOpen)
    return PTrue;

  rtpSession.Reopen(IsSource());

  return OpalMediaStream::Open();
}


PBoolean OpalRTPMediaStream::Close()
{
  PTRACE(3, "Media\tClosing RTP for " << *this);

  // Break any I/O blocks and wait for the thread that uses this object to
  // terminate before we allow it to be deleted.
  rtpSession.Close(IsSource());
  return OpalMediaStream::Close();
}


PBoolean OpalRTPMediaStream::ReadPacket(RTP_DataFrame & packet)
{
  if (IsSink()) {
    PTRACE(1, "Media\tTried to read from sink media stream");
    return PFalse;
  }

  if (!rtpSession.ReadBufferedData(timestamp, packet))
    return PFalse;

  timestamp = packet.GetTimestamp();
  return PTrue;
}


PBoolean OpalRTPMediaStream::WritePacket(RTP_DataFrame & packet)
{
  if (paused)
    packet.SetPayloadSize(0);
  
  if (IsSource()) {
    PTRACE(1, "Media\tTried to write to source media stream");
    return PFalse;
  }

  timestamp = packet.GetTimestamp();

  if (packet.GetPayloadSize() == 0)
    return PTrue;

  return rtpSession.WriteData(packet);
}

PBoolean OpalRTPMediaStream::SetDataSize(PINDEX dataSize)
{
  if (dataSize <= 0)
    return PFalse;

  defaultDataSize = dataSize;

  /* If we are a source then we should set our buffer size to the max
     practical UDP packet size. This means we have a buffer that can accept
     whatever the RTP sender throws at us. For sink, we just clamp it to that
     maximum size. */
  if (IsSource() || defaultDataSize > RTP_DataFrame::MaxEthernetPayloadSize)
    defaultDataSize = RTP_DataFrame::MaxEthernetPayloadSize;

  return PTrue;
}

PBoolean OpalRTPMediaStream::IsSynchronous() const
{
  return PFalse;
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
                                       PBoolean isSource,
                                       PChannel * chan, PBoolean autoDel)
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


PBoolean OpalRawMediaStream::ReadData(BYTE * buffer, PINDEX size, PINDEX & length)
{
  length = 0;

  if (IsSink()) {
    PTRACE(1, "Media\tTried to read from sink media stream");
    return PFalse;
  }

  PWaitAndSignal m(channel_mutex);
  if (!IsOpen() ||
      channel == NULL)
    return PFalse;

  if (!channel->Read(buffer, size))
    return PFalse;

  length = channel->GetLastReadCount();
  CollectAverage(buffer, length);
  return PTrue;
}


PBoolean OpalRawMediaStream::WriteData(const BYTE * buffer, PINDEX length, PINDEX & written)
{
  written = 0;

  if (IsSource()) {
    PTRACE(1, "Media\tTried to write to source media stream");
    return PFalse;
  }
  
  PWaitAndSignal m(channel_mutex);
  if (!IsOpen() ||
      channel == NULL)
    return PFalse;

  if (buffer != NULL && length != 0) {
    if (!channel->Write(buffer, length))
      return PFalse;
    written = channel->GetLastWriteCount();
    CollectAverage(buffer, written);
  }
  else {
    PBYTEArray silence(defaultDataSize);
    if (!channel->Write(silence, defaultDataSize))
      return PFalse;
    written = channel->GetLastWriteCount();
    CollectAverage(silence, written);
  }

  return PTrue;
}


PBoolean OpalRawMediaStream::Close()
{
  PTRACE(3, "Media\tClosing raw media stream " << *this);
  if (!OpalMediaStream::Close())
    return PFalse;

  PWaitAndSignal m(channel_mutex);
  if (channel == NULL)
    return PFalse;

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
                                         PBoolean isSource,
                                         PFile * file,
                                         PBoolean autoDel)
  : OpalRawMediaStream(conn, mediaFormat, sessionID, isSource, file, autoDel)
{
}


OpalFileMediaStream::OpalFileMediaStream(OpalConnection & conn,
                                         const OpalMediaFormat & mediaFormat,
                                         unsigned sessionID,
                                         PBoolean isSource,
                                         const PFilePath & path)
  : OpalRawMediaStream(conn, mediaFormat, sessionID, isSource,
                       new PFile(path, isSource ? PFile::ReadOnly : PFile::WriteOnly),
                       PTrue)
{
}


PBoolean OpalFileMediaStream::IsSynchronous() const
{
  return PFalse;
}

PBoolean OpalFileMediaStream::ReadData(
  BYTE * data,      ///<  Data buffer to read to
  PINDEX size,      ///<  Size of buffer
  PINDEX & length   ///<  Length of data actually read
)
{
  if (!OpalRawMediaStream::ReadData(data, size, length))
    return PFalse;

  // only delay if audio
  if (sessionID == 1)
    fileDelay.Delay(length/16);

  return PTrue;
}

/**Write raw media data to the sink media stream.
   The default behaviour writes to the PChannel object.
  */
PBoolean OpalFileMediaStream::WriteData(
  const BYTE * data,   ///<  Data to write
  PINDEX length,       ///<  Length of data to read.
  PINDEX & written     ///<  Length of data actually written
)
{
  if (!OpalRawMediaStream::WriteData(data, length, written))
    return PFalse;

  // only delay if audio
  if (sessionID == 1)
    fileDelay.Delay(written/16);

  return PTrue;
}

///////////////////////////////////////////////////////////////////////////////

#if OPAL_AUDIO
#if P_AUDIO

OpalAudioMediaStream::OpalAudioMediaStream(OpalConnection & conn,
                                           const OpalMediaFormat & mediaFormat,
                                           unsigned sessionID,
                                           PBoolean isSource,
                                           PINDEX buffers,
                                           PSoundChannel * channel,
                                           PBoolean autoDel)
  : OpalRawMediaStream(conn, mediaFormat, sessionID, isSource, channel, autoDel)
{
  soundChannelBuffers = buffers;
}


OpalAudioMediaStream::OpalAudioMediaStream(OpalConnection & conn,
                                           const OpalMediaFormat & mediaFormat,
                                           unsigned sessionID,
                                           PBoolean isSource,
                                           PINDEX buffers,
                                           const PString & deviceName)
  : OpalRawMediaStream(conn, mediaFormat, sessionID, isSource,
                       PSoundChannel::CreateOpenedChannel(PString::Empty(),
                                                          deviceName,
                                                          isSource ? PSoundChannel::Recorder
                                                                   : PSoundChannel::Player,
                                                          1, mediaFormat.GetClockRate(), 16),
                       PTrue)
{
  soundChannelBuffers = buffers;
}


PBoolean OpalAudioMediaStream::SetDataSize(PINDEX dataSize)
{
  PTRACE(3, "Media\tAudio " << (IsSource() ? "source" : "sink") << " data size set to  "
         << dataSize << " bytes and " << soundChannelBuffers << " buffers.");
  return OpalMediaStream::SetDataSize(dataSize) &&
         ((PSoundChannel *)channel)->SetBuffers(dataSize, soundChannelBuffers);
}


PBoolean OpalAudioMediaStream::IsSynchronous() const
{
  return PTrue;
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
                                           PBoolean del)
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


PBoolean OpalVideoMediaStream::SetDataSize(PINDEX dataSize)
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


PBoolean OpalVideoMediaStream::Open()
{
  if (isOpen)
    return PTrue;

  unsigned width = mediaFormat.GetOptionInteger(OpalVideoFormat::FrameWidthOption(), PVideoFrameInfo::QCIFWidth);
  unsigned height = mediaFormat.GetOptionInteger(OpalVideoFormat::FrameHeightOption(), PVideoFrameInfo::QCIFHeight);

  if (inputDevice != NULL) {
    if (!inputDevice->SetColourFormatConverter(mediaFormat)) {
      PTRACE(1, "Media\tCould not set colour format in grabber to " << mediaFormat);
      return PFalse;
    }
    if (!inputDevice->SetFrameSizeConverter(width, height)) {
      PTRACE(1, "Media\tCould not set frame size in grabber to " << width << 'x' << height << " in " << mediaFormat);
      return PFalse;
    }
    if (!inputDevice->Start()) {
      PTRACE(1, "Media\tCould not start video grabber");
      return PFalse;
    }
    lastGrabTime = PTimer::Tick();
  }

  if (outputDevice != NULL) {
    if (!outputDevice->SetColourFormatConverter(mediaFormat)) {
      PTRACE(1, "Media\tCould not set colour format in video display to " << mediaFormat);
      return PFalse;
    }
    if (!outputDevice->SetFrameSizeConverter(width, height)) {
      PTRACE(1, "Media\tCould not set frame size in video display to " << width << 'x' << height << " in " << mediaFormat);
      return PFalse;
    }
    if (!outputDevice->Start()) {
      PTRACE(1, "Media\tCould not start video display device");
      return PFalse;
    }
  }

  SetDataSize(0); // Gets set to minimum of device buffer requirements

  return OpalMediaStream::Open();
}


PBoolean OpalVideoMediaStream::ReadData(BYTE * data, PINDEX size, PINDEX & length)
{
  if (IsSink()) {
    PTRACE(1, "Media\tTried to read from sink media stream");
    return PFalse;
  }

  if (inputDevice == NULL) {
    PTRACE(1, "Media\tTried to read from video display device");
    return PFalse;
  }

  if (size < inputDevice->GetMaxFrameBytes()) {
    PTRACE(1, "Media\tTried to read with insufficient buffer size");
    return PFalse;
  }

  unsigned width, height;
  inputDevice->GetFrameSize(width, height);
  OpalVideoTranscoder::FrameHeader * frame = (OpalVideoTranscoder::FrameHeader *)PAssertNULL(data);
  frame->x = frame->y = 0;
  frame->width = width;
  frame->height = height;

  PINDEX bytesReturned = size - sizeof(OpalVideoTranscoder::FrameHeader);
  if (!inputDevice->GetFrameData((BYTE *)OPAL_VIDEO_FRAME_DATA_PTR(frame), &bytesReturned))
    return PFalse;

  PTimeInterval currentGrabTime = PTimer::Tick();
  timestamp += ((lastGrabTime - currentGrabTime)*1000/OpalMediaFormat::VideoClockRate).GetInterval();
  lastGrabTime = currentGrabTime;

  marker = PTrue;
  length = bytesReturned + sizeof(PluginCodec_Video_FrameHeader);

  if (outputDevice == NULL)
    return PTrue;

  return outputDevice->SetFrameData(0, 0, width, height, OPAL_VIDEO_FRAME_DATA_PTR(frame), PTrue);
}


PBoolean OpalVideoMediaStream::WriteData(const BYTE * data, PINDEX length, PINDEX & written)
{
  if (IsSource()) {
    PTRACE(1, "Media\tTried to write to source media stream");
    return PFalse;
  }

  if (outputDevice == NULL) {
    PTRACE(1, "Media\tTried to write to video capture device");
    return PFalse;
  }

  // Assume we are writing the exact amount (check?)
  written = length;

  // Check for missing packets, we do nothing at this level, just ignore it
  if (data == NULL)
    return PTrue;

  const OpalVideoTranscoder::FrameHeader * frame = (const OpalVideoTranscoder::FrameHeader *)data;
  outputDevice->SetFrameSize(frame->width, frame->height);
  return outputDevice->SetFrameData(frame->x, frame->y,
                                    frame->width, frame->height,
                                    OPAL_VIDEO_FRAME_DATA_PTR(frame), marker);
}


PBoolean OpalVideoMediaStream::IsSynchronous() const
{
  return IsSource();
}

#endif // OPAL_VIDEO

///////////////////////////////////////////////////////////////////////////////

OpalUDPMediaStream::OpalUDPMediaStream(OpalConnection & conn,
                                      const OpalMediaFormat & mediaFormat,
                                       unsigned sessionID,
                                       PBoolean isSource,
                                       OpalTransportUDP & transport)
  : OpalMediaStream(conn, mediaFormat, sessionID, isSource),
    udpTransport(transport)
{}

PBoolean OpalUDPMediaStream::ReadPacket(RTP_DataFrame & Packet)
{
  Packet.SetPayloadType(mediaFormat.GetPayloadType());
  Packet.SetPayloadSize(0);

  if (IsSink()) {
    PTRACE(1, "Media\tTried to read from sink media stream");
    return PFalse;
  }

  PBYTEArray rawData;
  if (!udpTransport.ReadPDU(rawData)) {
    PTRACE(2, "Media\tRead on UDP transport failed: "
       << udpTransport.GetErrorText() << " transport: " << udpTransport);
    return PFalse;
  }

  if (rawData.GetSize() > 0) {
    Packet.SetPayloadSize(rawData.GetSize());
    memcpy(Packet.GetPayloadPtr(), rawData.GetPointer(), rawData.GetSize());
  }

  return PTrue;
}

PBoolean OpalUDPMediaStream::WritePacket(RTP_DataFrame & Packet)
{
  if (IsSource()) {
    PTRACE(1, "Media\tTried to write to source media stream");
    return PFalse;
  }

  if (Packet.GetPayloadSize() > 0) {
    if (!udpTransport.Write(Packet.GetPayloadPtr(), Packet.GetPayloadSize())) {
      PTRACE(2, "Media\tWrite on UDP transport failed: "
         << udpTransport.GetErrorText() << " transport: " << udpTransport);
      return PFalse;
    }
  }

  return PTrue;
}

PBoolean OpalUDPMediaStream::IsSynchronous() const
{
  return PFalse;
}

PBoolean OpalUDPMediaStream::Close()
{
  if (!OpalMediaStream::Close())
    return PFalse;

  return udpTransport.Close();
}

// End of file ////////////////////////////////////////////////////////////////
