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
 */

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "mediastrm.h"
#endif

#include <opal_config.h>

#include <opal/mediastrm.h>
#include <opal/mediasession.h>

#if OPAL_VIDEO
#include <ptlib/videoio.h>
#include <codec/vidcodec.h>
#include <ptlib/vconvert.h>
#endif

#include <opal/patch.h>
#include <opal/endpoint.h>
#include <opal/call.h>
#include <lids/lid.h>
#include <ptlib/sound.h>


#define PTraceModule() "Media"
#define new PNEW


///////////////////////////////////////////////////////////////////////////////

OpalMediaStream::OpalMediaStream(OpalConnection & conn, const OpalMediaFormat & fmt, unsigned _sessionID, PBoolean isSourceStream)
  : m_connection(conn)
  , m_sessionID(_sessionID)
  , m_sequenceNumber(0)
  , m_identifier(conn.GetCall().GetToken() + psprintf("_%u", m_sessionID))
  , m_mediaFormat(fmt)
  , m_paused(false)
  , m_isSource(isSourceStream)
  , m_isOpen(false)
  , m_defaultDataSize(m_mediaFormat.GetFrameSize()*m_mediaFormat.GetOptionInteger(OpalAudioFormat::TxFramesPerPacketOption(), 1))
  , m_timestamp(0)
  , m_marker(true)
  , m_payloadType(m_mediaFormat.GetPayloadType())
  , m_frameTime(m_mediaFormat.GetFrameTime())
  , m_frameSize(m_mediaFormat.GetFrameSize())
{
  PTRACE_CONTEXT_ID_FROM(conn);
  PTRACE_CONTEXT_ID_TO(m_identifier);

  if (m_defaultDataSize == 0)
    m_defaultDataSize = m_connection.GetEndPoint().GetManager().GetMaxRtpPayloadSize();

  m_connection.SafeReference();
  PTRACE(5, "Created " << (IsSource() ? "Source" : "Sink") << ' ' << this);
}


OpalMediaStream::~OpalMediaStream()
{
  Close();
  m_connection.SafeDereference();
  PTRACE(5, "Destroyed " << (IsSource() ? "Source" : "Sink") << ' ' << this);
}


void OpalMediaStream::PrintOn(ostream & strm) const
{
  strm << GetClass() << '[' << this << "],"
       << (IsSource() ? "Source" : "Sink")
       << ',' << m_mediaFormat << ',' << m_sessionID;
}


PString OpalMediaStream::GetPatchThreadName() const
{
  return PString::Empty(); // Use default
}


OpalMediaFormat OpalMediaStream::GetMediaFormat() const
{
  return m_mediaFormat;
}


bool OpalMediaStream::SetMediaFormat(const OpalMediaFormat & newMediaFormat)
{
  if (!PAssert(newMediaFormat.IsValid(), PInvalidParameter))
    return false;

  OpalMediaFormat oldMediaFormat;
  OpalMediaPatchPtr mediaPatch;
  {
    P_INSTRUMENTED_LOCK_READ_WRITE(return false);

    if (m_mediaFormat == newMediaFormat)
      return true;

    PTRACE(4, "Switch media format from " << m_mediaFormat << " to " << newMediaFormat << " on " << *this);

    oldMediaFormat = m_mediaFormat;
    m_mediaFormat = newMediaFormat;

    // We make referenced copy of pointer so can't be deleted out from under us
    mediaPatch = m_mediaPatch;
  }

  // Easy if we haven't done the patch yet
  if (mediaPatch == NULL)
    return true;

  // Find transcoders for new media format pair (source/sink)
  if (mediaPatch->ResetTranscoders())
    return true;

  {
    // Couldn't switch, put it back
    P_INSTRUMENTED_LOCK_READ_WRITE(return false);
    m_mediaFormat = oldMediaFormat;
  }

  mediaPatch->ResetTranscoders();

  return false;
}


bool OpalMediaStream::UpdateMediaFormat(const OpalMediaFormat & newMediaFormat, bool mergeOnly)
{
  // We make referenced copy of pointer so can't be deleted out from under us
  OpalMediaPatchPtr mediaPatch = m_mediaPatch;

  OpalMediaFormat adjustedMediaFormat;
  if (!mergeOnly)
    adjustedMediaFormat = newMediaFormat;
  else {
    adjustedMediaFormat = m_mediaFormat;
    if (!adjustedMediaFormat.Merge(newMediaFormat, true))
      return false;
  }

  return mediaPatch != NULL ? mediaPatch->UpdateMediaFormat(adjustedMediaFormat)
                            : InternalUpdateMediaFormat(adjustedMediaFormat);
}


bool OpalMediaStream::InternalUpdateMediaFormat(const OpalMediaFormat & newMediaFormat)
{
  if (!m_mediaFormat.Update(newMediaFormat))
    return false;

  PTRACE(4, "Media format updated on " << *this);
  m_payloadType = m_mediaFormat.GetPayloadType();
  m_frameTime = m_mediaFormat.GetFrameTime();
  m_frameSize = m_mediaFormat.GetFrameSize();
  return true;
}


bool OpalMediaStream::ExecuteCommand(const OpalMediaCommand & command) const
{
  // We make referenced copy of pointer so can't be deleted out from under us
  OpalMediaPatchPtr mediaPatch = m_mediaPatch;

  return mediaPatch != NULL && mediaPatch->ExecuteCommand(command);

}


bool OpalMediaStream::InternalExecuteCommand(const OpalMediaCommand & command)
{
  if (IsSink()) {
    PTRACE(4, "Ended processing ExecuteCommand \"" << command << "\" on " << *this);
    return false;
  }

  PTRACE(4, "Passing on ExecuteCommand \"" << command << "\" on " << *this << " to " << m_connection);
  return m_connection.OnMediaCommand(*this, command);
}


PBoolean OpalMediaStream::Open()
{
  m_isOpen = true;
  return true;
}


bool OpalMediaStream::IsOpen() const
{
  return m_isOpen;
}


bool OpalMediaStream::IsEstablished() const
{
  return true;
}


PBoolean OpalMediaStream::Start()
{
  if (!Open())
    return false;

  // We make referenced copy of pointer so can't be deleted out from under us
  OpalMediaPatchPtr mediaPatch = m_mediaPatch;

  if (mediaPatch == NULL)
    return false;

  if (IsPaused()) {
    PTRACE(4, "Starting (paused) stream " << *this);
    return false;
  }

  PTRACE(4, "Starting stream " << *this);
  mediaPatch->Start();
  return true;
}


PBoolean OpalMediaStream::Close()
{
  if (!m_isOpen.exchange(false))
    return false;

  PTRACE(4, "Closing stream " << *this);

  {
    P_INSTRUMENTED_LOCK_READ_WRITE(return false);
    InternalClose();
  }

  m_connection.OnClosedMediaStream(*this);
  SetPatch(NULL);

  PTRACE(5, "Closed stream " << *this);

  m_connection.RemoveMediaStream(*this);
  // Don't do anything after above as object may be deleted

  return true;
}


PBoolean OpalMediaStream::WritePackets(RTP_DataFrameList & packets)
{
  for (RTP_DataFrameList::iterator packet = packets.begin(); packet != packets.end(); ++packet) {
    if (!WritePacket(*packet))
      return false;
  }

  return true;
}


PBoolean OpalMediaStream::ReadPacket(RTP_DataFrame & packet)
{
  if (!IsOpen())
    return false;

  unsigned oldTimestamp = m_timestamp;
  WORD oldSeqNumber = m_sequenceNumber;

  packet.MakeUnique();

  // We do the following to make sure that the buffer size is large enough,
  // in case something in previous loop adjusted it
  PINDEX maxSize = GetDataSize();
  packet.SetPayloadSize(maxSize);
  packet.SetPayloadSize(0);

  PINDEX lastReadCount;
  if (!ReadData(packet.GetPayloadPtr(), maxSize, lastReadCount))
    return false;

  if (oldSeqNumber == m_sequenceNumber)
    m_sequenceNumber++;

  packet.SetPayloadType(m_payloadType);
  packet.SetPayloadSize(lastReadCount);
  packet.SetTimestamp(oldTimestamp); // Beginning of frame
  packet.SetMarker(m_marker);
  packet.SetSequenceNumber(oldSeqNumber);
  m_marker = false;

  return true;
}


PBoolean OpalMediaStream::WritePacket(RTP_DataFrame & packet)
{
  if (!IsOpen())
    return false;

  m_timestamp = packet.GetTimestamp();

  int size = packet.GetPayloadSize();
  if (size == 0) {
    PINDEX dummy;
    if (!WriteData(NULL, 0, dummy)) {
      PTRACE(2, "WriteData (silence) failed");
      return false;
    }
  }
  else {
    m_marker = packet.GetMarker();
    const BYTE * ptr = packet.GetPayloadPtr();

    while (size > 0) {
      PINDEX written;
      if (!WriteData(ptr, size, written)) {
        PTRACE(2, "WriteData failed, size=" << size << ", written=" << written);
        return false;
      }
      size -= written;
      ptr += written;
    }

    PTRACE_IF(1, size < 0, "RTP payload size too small, short " << -size << " bytes.");
  }

  packet.SetTimestamp(m_timestamp);

  return true;
}


PBoolean OpalMediaStream::ReadData(BYTE * buffer, PINDEX size, PINDEX & length)
{
  if (!IsOpen()) {
    length = 0;
    return false;
  }

  RTP_DataFrame packet(size);
  if (!ReadPacket(packet)) {
    length = 0;
    return false;
  }

  length = packet.GetPayloadSize();
  if (length > size)
    length = size;
  memcpy(buffer, packet.GetPayloadPtr(), length);
  m_timestamp = packet.GetTimestamp();
  m_marker = packet.GetMarker();
  return true;
}


PBoolean OpalMediaStream::WriteData(const BYTE * buffer, PINDEX length, PINDEX & written)
{
  if (!IsOpen()) {
    written = 0;
    return false;
  }

  written = length;
  RTP_DataFrame packet(length);
  memcpy(packet.GetPayloadPtr(), buffer, length);
  packet.SetPayloadType(m_payloadType);
  packet.SetTimestamp(m_timestamp);
  packet.SetMarker(m_marker);
  return WritePacket(packet);
}


PBoolean OpalMediaStream::PushPacket(RTP_DataFrame & packet)
{
  // We make referenced copy of pointer so can't be deleted out from under us
  OpalMediaPatchPtr mediaPatch = m_mediaPatch;

  // OpalMediaPatch::PushFrame() might block, do outside of mutex
  return mediaPatch != NULL && mediaPatch->PushFrame(packet);
}


PBoolean OpalMediaStream::SetDataSize(PINDEX dataSize, PINDEX /*frameTime*/)
{
  if (dataSize <= 0) {
    PTRACE(4, "Ignoring set data size of " << dataSize << ", leaving as " << GetDataSize() << " on " << *this);
    return false;
  }

  PTRACE_IF(4, GetDataSize() != dataSize, "Set data size from "
            << GetDataSize() << " to " << dataSize << " on " << *this);
  m_defaultDataSize = dataSize;
  return true;
}


PBoolean OpalMediaStream::RequiresPatchThread(OpalMediaStream * /*stream*/) const
{
  return RequiresPatchThread();
}


PBoolean OpalMediaStream::RequiresPatchThread() const
{
  return true;
}


bool OpalMediaStream::RequireMediaTransportThread(OpalMediaStream & /*stream*/) const
{
  return true;
}


bool OpalMediaStream::EnableJitterBuffer(bool enab)
{
  if (!IsOpen())
    return false;

  PTRACE(4, (enab ? "En" : "Dis") << "abling jitter buffer on " << *this);
  OpalJitterBuffer::Init init(m_connection.GetEndPoint().GetManager(), m_mediaFormat.GetTimeUnits());
  if (!enab)
    init.m_minJitterDelay = init.m_maxJitterDelay = 0;
  return InternalSetJitterBuffer(init);
}


bool OpalMediaStream::InternalSetJitterBuffer(const OpalJitterBuffer::Init &)
{
  return false;
}


bool OpalMediaStream::InternalSetPaused(bool pause, bool fromUser, bool fromPatch)
{
  // We make referenced copy of pointer so can't be deleted out from under us
  OpalMediaPatchPtr mediaPatch = m_mediaPatch;

  // If we are source, then update the sink side, and vice versa
  if (!fromPatch && mediaPatch != NULL)
    return mediaPatch->InternalSetPaused(pause, fromUser);

  if (m_paused.exchange(pause) == pause)
    return false;

  PTRACE(3, (pause ? "Paused" : "Resumed") << " stream " << *this);

  if (fromUser) {
    P_INSTRUMENTED_LOCK_READ_WRITE(return false);
    m_connection.OnPauseMediaStream(*this, pause);
  }

  return true;
}


OpalMediaPatchPtr OpalMediaStream::InternalSetPatchPart1(OpalMediaPatch * newPatch)
{
  OpalMediaPatchPtr oldPatch = m_mediaPatch.Set(newPatch);
  if (newPatch == oldPatch)
    return NULL;

#if PTRACING
  if (PTrace::CanTrace(4) && (newPatch != NULL || oldPatch != NULL)) {
    ostream & trace = PTRACE_BEGIN(4);
    if (newPatch == NULL)
      trace << "Removing patch " << *oldPatch;
    else if (oldPatch == NULL)
      trace << "Adding patch " << *newPatch;
    else
      trace << "Overwriting patch " << *oldPatch << " with " << *newPatch;
    trace << " on stream " << *this << PTrace::End;
  }
#endif

  return oldPatch;
}


void OpalMediaStream::InternalSetPatchPart2(const OpalMediaPatchPtr & oldPatch)
{
  if (oldPatch != NULL) {
    if (IsSink())
      oldPatch->RemoveSink(*this);
    else
      oldPatch->Close();
  }
}


PBoolean OpalMediaStream::SetPatch(OpalMediaPatch * patch)
{
  InternalSetPatchPart2(InternalSetPatchPart1(patch));
  return true;
}


void OpalMediaStream::AddFilter(const PNotifier & filter, const OpalMediaFormat & stage) const
{
  // We make referenced copy of pointer so can't be deleted out from under us
  OpalMediaPatchPtr mediaPatch = m_mediaPatch;

  if (mediaPatch != NULL)
    mediaPatch->AddFilter(filter, stage);
}


PBoolean OpalMediaStream::RemoveFilter(const PNotifier & filter, const OpalMediaFormat & stage) const
{
  // We make referenced copy of pointer so can't be deleted out from under us
  OpalMediaPatchPtr mediaPatch = m_mediaPatch;

  return mediaPatch != NULL && mediaPatch->RemoveFilter(filter, stage);
}


#if OPAL_STATISTICS
void OpalMediaStream::GetStatistics(OpalMediaStatistics & statistics, bool fromPatch) const
{
  if (statistics.m_mediaFormat.IsEmpty() || m_mediaFormat.IsTransportable()) {
    statistics.m_mediaType = m_mediaFormat.GetMediaType();
    statistics.m_mediaFormat = m_mediaFormat.GetName();
  }

  // We make referenced copy of pointer so can't be deleted out from under us
  OpalMediaPatchPtr mediaPatch = m_mediaPatch;

  if (mediaPatch != NULL && !fromPatch)
    mediaPatch->GetStatistics(statistics, IsSink());
}
#endif // OPAL_STATISTICS


void OpalMediaStream::OnStartMediaPatch() 
{ 
  // We make referenced copy of pointer so can't be deleted out from under us
  OpalMediaPatchPtr mediaPatch = m_mediaPatch;

  if (mediaPatch != NULL && IsSource())
    m_connection.OnStartMediaPatch(*mediaPatch);
}


void OpalMediaStream::OnStopMediaPatch(OpalMediaPatch & patch)
{ 
  m_connection.OnStopMediaPatch(patch);
}


bool OpalMediaStream::SetMediaPassThrough(OpalMediaStream & otherStream, bool bypass)
{
  if (IsSink())
    return otherStream.SetMediaPassThrough(*this, bypass);

  OpalMediaPatchPtr sourcePatch = GetPatch();
  if (sourcePatch == NULL) {
    PTRACE(2, "SetMediaPassThrough could not complete as source patch does not exist");
    return false;
  }

  OpalMediaPatchPtr sinkPatch = otherStream.GetPatch();
  if (sinkPatch == NULL) {
    PTRACE(2, "SetMediaPassThrough could not complete as sink patch does not exist");
    return false;
  }

  if (GetMediaFormat() != otherStream.GetMediaFormat()) {
    PTRACE(3, "SetMediaPassThrough could not complete as different formats: "
           << GetMediaFormat() << "!=" << otherStream.GetMediaFormat());
    return false;
  }

  // Note SetBypassPatch() will do PTRACE() on status.
  return sourcePatch->SetBypassPatch(bypass ? sinkPatch : NULL);
}


void OpalMediaStream::PrintDetail(ostream & strm, const char * prefix, Details details) const
{
  if (prefix == NULL)
    strm << (IsSource() ? 'R' : 'S');
  else
    strm << prefix << (IsSource() ? " r" : " s");
  strm << (IsSource() ? "eceiving from " : "ending to ") << m_connection.GetPrefixName()
       << ", session " << GetSessionID()
       << ", " << m_mediaFormat;

  if ((details & DetailAudio) && !IsSource() && m_mediaFormat.GetMediaType() == OpalMediaType::Audio())
    strm << ", " << m_mediaFormat.GetOptionInteger(OpalAudioFormat::TxFramesPerPacketOption())*m_mediaFormat.GetFrameTime() / m_mediaFormat.GetTimeUnits() << "ms";

#if OPAL_VIDEO
  OpalVideoFormat::ContentRole contentRole = m_mediaFormat.GetOptionEnum(OpalVideoFormat::ContentRoleOption(), OpalVideoFormat::eNoRole);
  if (contentRole != OpalVideoFormat::eNoRole) {
    PString roleStr = OpalVideoFormat::ContentRoleToString(contentRole);
    roleStr.Delete(0,1);
    strm << ", " << roleStr << " video";
  }
#endif

  if (details & DetailEOL)
    strm << endl;
}


///////////////////////////////////////////////////////////////////////////////

OpalMediaStreamPacing::OpalMediaStreamPacing(const OpalMediaFormat & mediaFormat)
  : m_timeOnMarkers(mediaFormat.GetMediaType() != OpalMediaType::Audio())
  , m_frameTime(mediaFormat.GetFrameTime())
  , m_frameSize(mediaFormat.GetFrameSize())
  , m_timeUnits(mediaFormat.GetTimeUnits())
  /* No more than 1 second of "slip", reset timing if thread does not execute
     the timeout for a whole second. Prevents too much bunching up of data
     in "bad" conditions, that really should not happen. */
  , m_delay(1000)
  , m_previousDelay(m_frameTime/m_timeUnits)
{
  PAssert(m_timeOnMarkers || m_frameSize > 0, PInvalidParameter);
  PTRACE(4, "Pacing " << mediaFormat << ", time=" << m_frameTime << " (" << (m_frameTime/m_timeUnits) << "ms), "
            "size=" << m_frameSize << ", time " << (m_timeOnMarkers ? "on markers" : "every packet"));
}


void OpalMediaStreamPacing::Pace(bool generated, PINDEX bytes, bool & marker)
{
  unsigned timeToWait = m_frameTime;

  if (!m_timeOnMarkers)
    timeToWait *= (bytes + m_frameSize - 1) / m_frameSize;
  else {
    if (generated)
      marker = true;
    else if (!marker)
      return;
  }

  unsigned msToWait = timeToWait/m_timeUnits;
  if (msToWait == 0)
    msToWait = m_previousDelay;
  else
    m_previousDelay = msToWait;

  PTRACE(m_throttleLog, "Pacing delay: " << timeToWait
         << " (" << msToWait << "ms), "
            "time=" << m_frameTime << " (" << (m_frameTime/m_timeUnits) << "ms), "
            "bytes=" << bytes << ", size=" << m_frameSize
         << m_throttleLog);
  m_delay.Delay(msToWait);
}


bool OpalMediaStreamPacing::UpdateMediaFormat(const OpalMediaFormat & mediaFormat)
{
  m_frameTime = mediaFormat.GetFrameTime();
  m_frameSize = mediaFormat.GetFrameSize();
  m_timeUnits = mediaFormat.GetTimeUnits();
  PTRACE(4, "Pacing updated: " << mediaFormat << ",  time=" << m_frameTime
         << " (" << (m_frameTime/m_timeUnits) << "ms), size=" << m_frameSize);
  return true;
}


///////////////////////////////////////////////////////////////////////////////

OpalNullMediaStream::OpalNullMediaStream(OpalConnection & conn,
                                         const OpalMediaFormat & mediaFormat,
                                         unsigned sessionID,
                                         bool isSource,
                                         bool isSyncronous)
  : OpalMediaStream(conn, mediaFormat, sessionID, isSource)
  , OpalMediaStreamPacing(mediaFormat)
  , m_isSynchronous(isSyncronous)
  , m_requiresPatchThread(isSyncronous)
{
}


OpalNullMediaStream::OpalNullMediaStream(OpalConnection & conn,
                                         const OpalMediaFormat & mediaFormat,
                                         unsigned sessionID,
                                         bool isSource,
                                         bool usePacingDelay,
                                         bool requiresPatchThread)
  : OpalMediaStream(conn, mediaFormat, sessionID, isSource)
  , OpalMediaStreamPacing(mediaFormat)
  , m_isSynchronous(usePacingDelay)
  , m_requiresPatchThread(requiresPatchThread)
{
}


PBoolean OpalNullMediaStream::ReadData(BYTE * buffer, PINDEX size, PINDEX & length)
{
  if (!IsOpen())
    return false;

  memset(buffer, 0, size);
  length = size;
  m_timestamp += OpalMediaStream::m_frameTime;

  if (m_isSynchronous)
    Pace(true, size, m_marker);
  return true;
}


PBoolean OpalNullMediaStream::WriteData(const BYTE * /*buffer*/, PINDEX length, PINDEX & written)
{
  if (!IsOpen())
    return false;

  written = length != 0 ? length : GetDataSize();

  if (m_isSynchronous)
    Pace(false, written, m_marker);
  return true;
}


bool OpalNullMediaStream::InternalSetPaused(bool pause, bool fromUser, bool fromPatch)
{
  if (!OpalMediaStream::InternalSetPaused(pause, fromUser, fromPatch))
    return false;

  // If coming out of pause, restart pacing delay
  if (!pause)
    m_delay.Restart();

  return true;
}


PBoolean OpalNullMediaStream::RequiresPatchThread() const
{
  return m_requiresPatchThread;
}


PBoolean OpalNullMediaStream::IsSynchronous() const
{
  return m_isSynchronous;
}


bool OpalNullMediaStream::InternalUpdateMediaFormat(const OpalMediaFormat & newMediaFormat)
{
  return OpalMediaStream::InternalUpdateMediaFormat(newMediaFormat) &&
         OpalMediaStreamPacing::UpdateMediaFormat(m_mediaFormat); // use the newly adjusted mediaFormat
}


///////////////////////////////////////////////////////////////////////////////

OpalRawMediaStream::OpalRawMediaStream(OpalConnection & conn,
                                       const OpalMediaFormat & mediaFormat,
                                       unsigned sessionID,
                                       PBoolean isSource,
                                       PChannel * chan, PBoolean autoDelete)
  : OpalMediaStream(conn, mediaFormat, sessionID, isSource)
  , m_channel(chan)
  , m_autoDelete(autoDelete)
  , m_silence(10*sizeof(short)*mediaFormat.GetTimeUnits()) // At least 10ms
  , m_averageSignalSum(0)
  , m_averageSignalSamples(0)
{
}


OpalRawMediaStream::~OpalRawMediaStream()
{
  Close();

  if (m_autoDelete)
    delete m_channel;
  m_channel = NULL;
}


PBoolean OpalRawMediaStream::ReadData(BYTE * buffer, PINDEX size, PINDEX & length)
{
  if (!IsOpen())
    return false;

  length = 0;

  if (IsSink()) {
    PTRACE(1, "Tried to read from sink media stream");
    return false;
  }

  PWaitAndSignal mutex(m_channelMutex);

  if (!IsOpen() || m_channel == NULL)
    return false;

  if (buffer == NULL || size == 0)
    return m_channel->Read(buffer, size);

  unsigned consecutiveZeroReads = 0;
  while (size > 0) {
    if (!m_channel->Read(buffer, size)) {
#if PTRACING
      if (PTrace::CanTrace(2)) {
        switch (m_channel->GetErrorCode(PChannel::LastReadError)) {
          case PChannel::NotOpen:
            break;
          case PChannel::NoError:
            PTRACE_BEGIN(3) << "Raw channel end of file" << PTrace::End;
            break;
          default:
            PTRACE_BEGIN(2) << "Raw channel read error: " << m_channel->GetErrorText(PChannel::LastReadError) << PTrace::End;
        }
      }
#endif
      return false;
    }

    PINDEX lastReadCount = m_channel->GetLastReadCount();
    if (lastReadCount != 0)
      consecutiveZeroReads = 0;
    else if (++consecutiveZeroReads > 10) {
      PTRACE(1, "Raw channel returned success with zero data multiple consecutive times, aborting.");
      return false;
    }

    CollectAverage(buffer, lastReadCount);

    m_timestamp += lastReadCount / sizeof(short);
    buffer += lastReadCount;
    length += lastReadCount;
    size -= lastReadCount;
  }

  return true;
}


PBoolean OpalRawMediaStream::WriteData(const BYTE * buffer, PINDEX length, PINDEX & written)
{
  if (!IsOpen()) {
    PTRACE(1, "Tried to write to closed media stream");
    return false;
  }

  written = 0;

  if (IsSource()) {
    PTRACE(1, "Tried to write to source media stream");
    return false;
  }
  
  PWaitAndSignal mutex(m_channelMutex);

  if (!IsOpen() || m_channel == NULL) {
    PTRACE(1, "Tried to write to media stream with no channel");
    return false;
  }

  /* We make the assumption that the remote is sending the same sized packets
     all the time and does not suddenly switch from 30ms to 20ms, even though
     this is technically legal. We have never yet seen it, and even if it did
     it doesn't hurt the silence insertion algorithm very much.

     So, silence buffer is set to be the largest chunk of audio the remote has
     ever sent to us. Then when they stop sending, (we get length==0) we just
     keep outputting that number of bytes to the raw channel until the remote
     starts up again.
     */

  if (buffer != NULL && length != 0)
    m_silence.SetMinSize(length);
  else {
    length = m_silence.GetSize();
    buffer = m_silence;
    PTRACE(6, "Playing silence " << length << " bytes");
  }

  if (!m_channel->Write(buffer, length)) {
    PTRACE_IF(2, m_channel->GetErrorCode(PChannel::LastWriteError) != PChannel::NotOpen,
              "Channel write error: " << m_channel->GetErrorText(PChannel::LastWriteError));
    return false;
  }

  written = m_channel->GetLastWriteCount();
  CollectAverage(buffer, written);
  return true;
}


bool OpalRawMediaStream::SetChannel(PChannel * chan, bool autoDelete)
{
  if (chan == NULL || !chan->IsOpen()) {
    if (autoDelete)
      delete chan;
    return false;
  }

  m_channelMutex.Wait();

  PChannel * channelToDelete = m_autoDelete ? m_channel : NULL;
  m_channel = chan;
  m_autoDelete = autoDelete;

  SetDataSize(GetDataSize(), m_frameTime);

  m_channelMutex.Signal();

  delete channelToDelete; // Outside mutex

  PTRACE(4, "Set raw media channel to \"" << m_channel->GetName() << '"');
  return true;
}


void OpalRawMediaStream::InternalClose()
{
  if (m_channel != NULL)
    m_channel->Close();
}


unsigned OpalRawMediaStream::GetAverageSignalLevel()
{
  PWaitAndSignal mutex(m_averagingMutex);

  if (m_averageSignalSamples == 0)
    return UINT_MAX;

  unsigned average = (unsigned)(m_averageSignalSum/m_averageSignalSamples);
  m_averageSignalSum = average;
  m_averageSignalSamples = 1;
  return average;
}


void OpalRawMediaStream::CollectAverage(const BYTE * buffer, PINDEX size)
{
  PWaitAndSignal mutex(m_averagingMutex);

  size = size/2;
  m_averageSignalSamples += size;
  const short * pcm = (const short *)buffer;
  while (size-- > 0) {
    m_averageSignalSum += PABS(*pcm);
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
  , OpalMediaStreamPacing(mediaFormat)
{
}


OpalFileMediaStream::OpalFileMediaStream(OpalConnection & conn,
                                         const OpalMediaFormat & mediaFormat,
                                         unsigned sessionID,
                                         PBoolean isSource,
                                         const PFilePath & path)
  : OpalRawMediaStream(conn, mediaFormat, sessionID, isSource,
                       new PFile(path, isSource ? PFile::ReadOnly : PFile::WriteOnly),
                       true)
  , OpalMediaStreamPacing(mediaFormat)
{
}


PBoolean OpalFileMediaStream::IsSynchronous() const
{
  return false;
}


PBoolean OpalFileMediaStream::ReadData(BYTE * data, PINDEX size, PINDEX & length)
{
  if (!OpalRawMediaStream::ReadData(data, size, length))
    return false;

  Pace(true, size, m_marker);
  return true;
}


/**Write raw media data to the sink media stream.
   The default behaviour writes to the PChannel object.
  */
PBoolean OpalFileMediaStream::WriteData(const BYTE * data, PINDEX length, PINDEX & written)
{
  if (!OpalRawMediaStream::WriteData(data, length, written))
    return false;

  Pace(false, written, m_marker);
  return true;
}


///////////////////////////////////////////////////////////////////////////////

#if OPAL_PTLIB_AUDIO

OpalAudioMediaStream::OpalAudioMediaStream(OpalConnection & conn,
                                           const OpalMediaFormat & mediaFormat,
                                           unsigned sessionID,
                                           PBoolean isSource,
                                           PINDEX buffers,
                                           unsigned bufferTime,
                                           PSoundChannel * channel,
                                           PBoolean autoDel)
  : OpalRawMediaStream(conn, mediaFormat, sessionID, isSource, channel, autoDel)
  , m_soundChannelBuffers(buffers)
  , m_soundChannelBufferTime(bufferTime)
{
}


OpalAudioMediaStream::OpalAudioMediaStream(OpalConnection & conn,
                                           const OpalMediaFormat & mediaFormat,
                                           unsigned sessionID,
                                           PBoolean isSource,
                                           PINDEX buffers,
                                           unsigned bufferTime,
                                           const PString & deviceName)
  : OpalRawMediaStream(conn, mediaFormat, sessionID, isSource,
                       PSoundChannel::CreateOpenedChannel(PString::Empty(),
                                                          deviceName,
                                                          isSource ? PSoundChannel::Recorder
                                                                   : PSoundChannel::Player,
                                                          1, mediaFormat.GetClockRate(), 16),
                       true)
  , m_soundChannelBuffers(buffers)
  , m_soundChannelBufferTime(bufferTime)
{
}


PBoolean OpalAudioMediaStream::SetDataSize(PINDEX dataSize, PINDEX frameTime)
{
  /* For efficiency reasons we will not accept a packet size that is too small.
      We move it up to the next even multiple of the minimum, which has a danger
      of the remote not sending an even number of our multiplier, but 10ms seems
      universally done by everyone out there. */
  const unsigned MinBufferTimeMilliseconds = 10;

  unsigned timePerMS = m_mediaFormat.GetTimeUnits();
  unsigned channels = m_mediaFormat.GetOptionInteger(OpalAudioFormat::ChannelsOption(), 1);
  PINDEX frameSize = frameTime*channels*sizeof(short);
  unsigned frameMilliseconds = (frameTime+timePerMS-1)/timePerMS;

  if (frameMilliseconds == 0) {
    frameSize = MinBufferTimeMilliseconds*timePerMS*channels*sizeof(short);
    frameMilliseconds = MinBufferTimeMilliseconds;
  }
  else if (frameMilliseconds < MinBufferTimeMilliseconds) {
    PINDEX minFrameCount = (MinBufferTimeMilliseconds+frameMilliseconds-1)/frameMilliseconds;
    frameSize = minFrameCount*frameTime*channels*sizeof(short);
    frameMilliseconds = (minFrameCount*frameTime+timePerMS-1)/timePerMS;
  }

  // Calculate number of sound buffers from global system settings
  PINDEX bufferCount = (m_soundChannelBufferTime+frameMilliseconds-1)/frameMilliseconds;
  if (bufferCount < m_soundChannelBuffers)
    bufferCount = m_soundChannelBuffers;
  if (IsSource())
    bufferCount = std::max(bufferCount, (dataSize + frameSize - 1) / frameSize);

  PINDEX adjustedSize = (dataSize + frameSize - 1) / frameSize * frameSize;
  PTRACE(3, "Audio " << (IsSource() ? "source" : "sink") << " "
            "data size set to " << adjustedSize << " (" << dataSize << "), "
            "frameTime=" << frameTime << ", "
            "clock=" << m_mediaFormat.GetClockRate() << ", "
            "buffers=" << bufferCount << 'x' << frameSize);

  return OpalMediaStream::SetDataSize(adjustedSize, frameTime) &&
         ((PSoundChannel *)m_channel)->SetBuffers(frameSize, bufferCount);
}


PBoolean OpalAudioMediaStream::IsSynchronous() const
{
  return true;
}

#endif // OPAL_PTLIB_AUDIO


///////////////////////////////////////////////////////////////////////////////

#if OPAL_VIDEO

OpalVideoMediaStream::OpalVideoMediaStream(OpalConnection & conn,
                                          const OpalMediaFormat & mediaFormat,
                                           unsigned sessionID,
                                           PVideoInputDevice * in,
                                           PVideoOutputDevice * out,
                                           PBoolean delIn,
                                           PBoolean delOut)
  : OpalMediaStream(conn, mediaFormat, sessionID, in != NULL)
  , m_inputDevice(in)
  , m_watermarkDevice(NULL)
  , m_outputDevice(out)
  , m_autoDeleteInput(delIn)
  , m_autoDeleteWatermark(false)
  , m_autoDeleteOutput(delOut)
  , m_needKeyFrame(false)
{
  PAssert(in != NULL || out != NULL, PInvalidParameter);
}


OpalVideoMediaStream::~OpalVideoMediaStream()
{
  Close();

  if (m_autoDeleteInput)
    delete m_inputDevice;

  if (m_autoDeleteOutput)
    delete m_outputDevice;
}


PBoolean OpalVideoMediaStream::SetDataSize(PINDEX dataSize, PINDEX frameTime)
{
  if (m_inputDevice != NULL) {
    PINDEX minDataSize = m_inputDevice->GetMaxFrameBytes();
    if (dataSize < minDataSize)
      dataSize = minDataSize;
  }
  if (m_outputDevice != NULL) {
    PINDEX minDataSize = m_outputDevice->GetMaxFrameBytes();
    if (dataSize < minDataSize)
      dataSize = minDataSize;
  }

  return OpalMediaStream::SetDataSize(sizeof(PluginCodec_Video_FrameHeader) + dataSize, frameTime);
}


void OpalVideoMediaStream::SetVideoInputDevice(PVideoInputDevice * device, bool autoDelete)
{
  PWaitAndSignal mutex(m_devicesMutex);

  if (m_autoDeleteInput)
    delete m_inputDevice;

  m_inputDevice = device;
  m_autoDeleteInput = autoDelete;

  InternalAdjustDevices();

  if (!m_inputDevice->Start()) {
    PTRACE(1, "Could not start video grabber");
  }
  m_lastGrabTime = PTimer::Tick();
}


void OpalVideoMediaStream::SetVideoOutputDevice(PVideoOutputDevice * device, bool autoDelete)
{
  PWaitAndSignal mutex(m_devicesMutex);

  if (m_autoDeleteOutput)
    delete m_outputDevice;
  m_outputDevice = device;
  m_autoDeleteOutput = autoDelete;
  InternalAdjustDevices();
}


void OpalVideoMediaStream::SetVideoWatermarkDevice(PVideoInputDevice * device, bool autoDelete)
{
  PWaitAndSignal mutex(m_devicesMutex);

  if (m_autoDeleteWatermark)
    delete m_watermarkDevice;

  m_watermarkDevice = device;
  m_autoDeleteWatermark = autoDelete;

  InternalAdjustDevices();

  if (!m_watermarkDevice->Start()) {
    PTRACE(1, "Could not start video grabber");
  }
}


bool OpalVideoMediaStream::InternalUpdateMediaFormat(const OpalMediaFormat & newMediaFormat)
{
  PWaitAndSignal mutex(m_devicesMutex);

  return OpalMediaStream::InternalUpdateMediaFormat(newMediaFormat) && InternalAdjustDevices();
}


bool OpalVideoMediaStream::InternalAdjustDevices()
{
  PVideoFrameInfo video(m_mediaFormat.GetOptionInteger(OpalVideoFormat::FrameWidthOption(), PVideoFrameInfo::QCIFWidth),
                        m_mediaFormat.GetOptionInteger(OpalVideoFormat::FrameHeightOption(), PVideoFrameInfo::QCIFHeight),
                        m_mediaFormat.GetName(),
                       (m_mediaFormat.GetClockRate()+m_mediaFormat.GetFrameTime()/2)/m_mediaFormat.GetFrameTime());

  if (m_inputDevice != NULL) {
    if (!m_inputDevice->SetFrameInfoConverter(video))
      return false;
  }

  if (m_outputDevice != NULL) {
    if (!m_outputDevice->SetFrameInfoConverter(video))
      return false;
  }

  if (m_watermarkDevice != NULL) {
    // Don't use SetFrameInfoConverter s do not want to change resolution
    if (!m_watermarkDevice->SetColourFormatConverter(m_mediaFormat.GetName()))
      return false;
    if (!m_watermarkDevice->SetFrameRate(video.GetFrameRate()))
      return false;
  }

  PTRACE(4, "Adjusted video devices to " << video << " on " << *this);
  return true;
}


PBoolean OpalVideoMediaStream::Open()
{
  if (m_isOpen)
    return true;

  P_INSTRUMENTED_LOCK_READ_WRITE(return false);

  InternalAdjustDevices();

  if (m_inputDevice != NULL) {
    if (!m_inputDevice->Start()) {
      PTRACE(1, "Could not start video grabber");
      return false;
    }
    m_lastGrabTime = PTimer::Tick();
  }

  SetDataSize(1, 1); // Gets set to minimum of device buffer requirements

  return OpalMediaStream::Open();
}


void OpalVideoMediaStream::InternalClose()
{
  if (m_inputDevice != NULL) {
    if (m_autoDeleteInput)
      m_inputDevice->Close();
    else
      m_inputDevice->Stop();
  }

  if (m_outputDevice != NULL) {
    if (m_autoDeleteOutput)
      m_outputDevice->Close();
    else
      m_outputDevice->Stop();
  }
}


bool OpalVideoMediaStream::InternalExecuteCommand(const OpalMediaCommand & command)
{
  if (IsSource() && PIsDescendant(&command, OpalVideoUpdatePicture)) {
    PTRACE_IF(3, !m_needKeyFrame, "Key frame forced in video stream");
    m_needKeyFrame = true; // Reset when I-Frame is sent
  }

  return OpalMediaStream::InternalExecuteCommand(command);
}


PBoolean OpalVideoMediaStream::ReadData(BYTE * data, PINDEX size, PINDEX & length)
{
  if (!IsOpen())
    return false;

  if (IsSink()) {
    PTRACE(1, "Tried to read from sink media stream");
    return false;
  }

  PWaitAndSignal mutex(m_devicesMutex);

  if (m_inputDevice == NULL) {
    PTRACE(1, "Tried to read from video display device");
    return false;
  }

  if (size < m_inputDevice->GetMaxFrameBytes()) {
    PTRACE(1, "Tried to read with insufficient buffer size - " << size << " < " << m_inputDevice->GetMaxFrameBytes());
    return false;
  }

  unsigned width, height;
  m_inputDevice->GetFrameSize(width, height);
  OpalVideoTranscoder::FrameHeader * frame = (OpalVideoTranscoder::FrameHeader *)PAssertNULL(data);
  frame->x = frame->y = 0;
  frame->width = width;
  frame->height = height;

  bool keyFrame = m_needKeyFrame;
  PINDEX bytesReturned = size - sizeof(OpalVideoTranscoder::FrameHeader);
  BYTE * frameData = OpalVideoFrameDataPtr(frame);
  if (!m_inputDevice->GetFrameData(frameData, &bytesReturned, keyFrame)) {
    PTRACE(2, "Failed to grab frame from " << m_inputDevice->GetDeviceName());
    return false;
  }

  // If it gave us a key frame, stop asking.
  if (keyFrame)
    m_needKeyFrame = false;

  PTimeInterval currentGrabTime = PTimer::Tick();
  m_timestamp += (int)((currentGrabTime - m_lastGrabTime).GetMilliSeconds()*OpalMediaFormat::VideoClockRate/1000);
  m_lastGrabTime = currentGrabTime;

  m_marker = true;
  length = bytesReturned;
  if (length > 0)
    length += sizeof(PluginCodec_Video_FrameHeader);

  ApplyWatermark(width, height, frameData);

  if (m_outputDevice == NULL)
    return true;

  if (!m_outputDevice->Start()) {
    PTRACE(1, "Could not start video display device");
    if (m_autoDeleteOutput)
      delete m_outputDevice;
    m_outputDevice = NULL;
    return false;
  }

  return m_outputDevice->SetFrameData(0, 0, width, height, frameData, true);
}


PBoolean OpalVideoMediaStream::WriteData(const BYTE * data, PINDEX length, PINDEX & written)
{
  if (!IsOpen())
    return false;

  if (IsSource()) {
    PTRACE(1, "Tried to write to source media stream");
    return false;
  }

  PWaitAndSignal mutex(m_devicesMutex);

  if (m_outputDevice == NULL) {
    PTRACE(1, "Tried to write to video capture device");
    return false;
  }

  // Assume we are writing the exact amount (check?)
  written = length;

  // Check for missing packets, we do nothing at this level, just ignore it
  if (data == NULL)
    return true;

  const OpalVideoTranscoder::FrameHeader * frame = (const OpalVideoTranscoder::FrameHeader *)data;

  if (!m_outputDevice->SetFrameSize(frame->width, frame->height)) {
    PTRACE(1, "Could not resize video display device to " << frame->width << 'x' << frame->height);
    return false;
  }

  ApplyWatermark(frame->width, frame->height, OpalVideoFrameDataPtr(frame));

  if (!m_outputDevice->Start()) {
    PTRACE(1, "Could not start video display device");
    return false;
  }

  PVideoOutputDevice::FrameData frameData;
  frameData.x = frame->x;
  frameData.y = frame->y;
  frameData.width = frame->width;
  frameData.height = frame->height;
  frameData.pixels = OpalVideoFrameDataPtr(frame);
  frameData.partialFrame = !m_marker;
  frameData.timestamp = m_timestamp * 1000LL / m_mediaFormat.GetTimeUnits();
  if (!m_outputDevice->SetFrameData(frameData))
    return false;

  if (frameData.keyFrameNeeded)
    ExecuteCommand(OpalVideoUpdatePicture());

  return true;
}


PBoolean OpalVideoMediaStream::IsSynchronous() const
{
  return IsSource();
}


void OpalVideoMediaStream::ApplyWatermark(unsigned frameWidth, unsigned frameHeight, BYTE * frameData)
{
  if (m_watermarkDevice == NULL)
    return;

  if (m_watermarkDevice->GetColourFormat() != OpalYUV420P)
    return;

  if (!m_watermarkDevice->GetFrameData(m_watermarkData.GetPointer(m_watermarkDevice->GetMaxFrameBytes()))) {
    PTRACE(2, "Failed to grab frame from " << m_watermarkDevice->GetDeviceName());
    return;
  }

  unsigned waterWidth, waterHeight;
  m_watermarkDevice->GetFrameSize(waterWidth, waterHeight);

  unsigned w,h;
  if (waterWidth > waterHeight) {
    w = std::min(waterWidth, frameWidth);
    h = std::min(waterHeight, frameHeight/4)&~1; // No more than a quarter of height, and even
  }
  else {
    w = std::min(waterWidth, frameWidth/4)&~1; // No more than a quarter of width, and even
    h = std::min(waterHeight, frameHeight);
  }

  PColourConverter::CopyYUV420P(0, 0, waterWidth, waterHeight, waterWidth, waterHeight, m_watermarkData,
                                frameWidth-w, frameHeight-h, w, h, frameWidth, frameHeight, frameData);
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
{
}

OpalUDPMediaStream::~OpalUDPMediaStream()
{
  Close();
}


PBoolean OpalUDPMediaStream::ReadPacket(RTP_DataFrame & packet)
{
  packet.SetPayloadType(m_payloadType);
  packet.SetPayloadSize(0);

  if (IsSink()) {
    PTRACE(1, "Tried to read from sink media stream");
    return false;
  }

  PBYTEArray rawData;
  if (!udpTransport.ReadPDU(rawData)) {
    PTRACE(2, "Read on UDP transport failed: "
       << udpTransport.GetErrorText() << " transport: " << udpTransport);
    return false;
  }

  if (rawData.GetSize() > 0)
    packet.SetPayload(rawData, rawData.GetSize());

  return true;
}


PBoolean OpalUDPMediaStream::WritePacket(RTP_DataFrame & Packet)
{
  if (IsSource()) {
    PTRACE(1, "Tried to write to source media stream");
    return false;
  }

  if (Packet.GetPayloadSize() > 0) {
    if (!udpTransport.Write(Packet.GetPayloadPtr(), Packet.GetPayloadSize())) {
      PTRACE(2, "Write on UDP transport failed: "
         << udpTransport.GetErrorText() << " transport: " << udpTransport);
      return false;
    }
  }

  return true;
}


PBoolean OpalUDPMediaStream::IsSynchronous() const
{
  return false;
}


void OpalUDPMediaStream::InternalClose()
{
  udpTransport.Close();
}


///////////////////////////////////////////////////////////////////////////////

OpalMediaCommand::OpalMediaCommand(const OpalMediaType & mediaType, unsigned sessionID, unsigned ssrc)
  : m_mediaType(mediaType)
  , m_sessionID(sessionID)
  , m_ssrc(ssrc)
{
}


void OpalMediaCommand::PrintOn(ostream & strm) const
{
  strm << GetName();
}


PObject::Comparison OpalMediaCommand::Compare(const PObject & obj) const
{
  return GetName().Compare(PDownCast(const OpalMediaCommand, &obj)->GetName());
}


void * OpalMediaCommand::GetPlugInData() const
{
  return NULL;
}


unsigned * OpalMediaCommand::GetPlugInSize() const
{
  return NULL;
}


OpalMediaFlowControl::OpalMediaFlowControl(OpalBandwidth bitRate,
                                           const OpalMediaType & mediaType,
                                           unsigned sessionID,
                                           unsigned ssrc)
  : OpalMediaCommand(mediaType, sessionID, ssrc)
  , m_bitRate(bitRate)
  , m_ssrcs(1, ssrc)
{
}


OpalMediaFlowControl::OpalMediaFlowControl(OpalBandwidth bitRate,
                                           const OpalMediaType & mediaType,
                                           unsigned sessionID,
                                           const RTP_SyncSourceArray & ssrcs)
  : OpalMediaCommand(mediaType, sessionID, ssrcs.empty() ? 0 : ssrcs[0])
  , m_bitRate(bitRate)
  , m_ssrcs(ssrcs)
{
}


PString OpalMediaFlowControl::GetName() const
{
  return "Flow Control";
}


OpalMediaPacketLoss::OpalMediaPacketLoss(unsigned packetLoss,
                                         const OpalMediaType & mediaType,
                                         unsigned sessionID,
                                         unsigned ssrc)
  : OpalMediaCommand(mediaType, sessionID, ssrc)
  , m_packetLoss(packetLoss)
{
}


PString OpalMediaPacketLoss::GetName() const
{
  return "Packet Loss";
}


OpalMediaMaxPayload::OpalMediaMaxPayload(unsigned payloadSize,
                                         const OpalMediaType & mediaType,
                                         unsigned sessionID,
                                         unsigned ssrc)
  : OpalMediaCommand(mediaType, sessionID, ssrc)
  , m_payloadSize(payloadSize)
{
}


PString OpalMediaMaxPayload::GetName() const
{
  return "Max Payload Size";
}


// End of file ////////////////////////////////////////////////////////////////
