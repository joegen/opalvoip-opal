/*
 * rtp_stream.cxx
 *
 * Media Stream classes
 *
 * Open Phone Abstraction Library (OPAL)
 *
 * Copyright (C) 2014 Vox Lucida Pty. Ltd.
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
 * Contributor(s): ________________________________________.
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "rtp_stream.h"
#endif

#include <opal_config.h>

#include <rtp/rtpep.h>
#include <rtp/rtpconn.h>
#include <rtp/rtp_stream.h>
#include <rtp/rtp_session.h>
#include <opal/patch.h>

#if OPAL_VIDEO
#include <codec/vidcodec.h>
#endif


#define PTraceModule() "RTPStream"
#define new PNEW


///////////////////////////////////////////////////////////////////////////////

OpalRTPMediaStream::OpalRTPMediaStream(OpalRTPConnection & conn,
                                   const OpalMediaFormat & mediaFormat,
                                                  PBoolean isSource,
                                          OpalRTPSession & rtp)
  : OpalMediaStream(conn, mediaFormat, rtp.GetSessionID(), isSource)
  , m_rtpSession(rtp)
  , m_rewriteHeaders(true)
  , m_syncSource(0)
  , m_jitterBuffer(NULL)
  , m_receiveNotifier(PCREATE_RTPDataNotifier(OnReceivedPacket))
{
  /* If we are a source then we should set our buffer size to the max
     practical UDP packet size. This means we have a buffer that can accept
     whatever the RTP sender throws at us. For sink, we set it to the
     maximum size based on MTU (or other criteria). */
  m_defaultDataSize = isSource ? conn.GetEndPoint().GetManager().GetMaxRtpPacketSize() : conn.GetMaxRtpPayloadSize();

  m_rtpSession.SafeReference();

  PTRACE(5, "Using RTP media session " << &rtp);
}


OpalRTPMediaStream::~OpalRTPMediaStream()
{
  Close();

  delete m_jitterBuffer;

  m_rtpSession.SafeDereference();
}


PBoolean OpalRTPMediaStream::Open()
{
  if (m_isOpen)
    return true;

  if (IsSource()) {
    delete m_jitterBuffer;
    m_jitterBuffer = OpalJitterBuffer::Create(OpalJitterBuffer::Init(mediaFormat.GetMediaType()));
    m_rtpSession.SetJitterBuffer(m_jitterBuffer, m_syncSource);
    m_rtpSession.AddDataNotifier(100, m_receiveNotifier);
    PTRACE(4, "Opening source stream " << *this << " jb=" << *m_jitterBuffer);
  }
  else {
    PTRACE(4, "Opening sink stream " << *this);
  }

#if OPAL_VIDEO
  m_forceIntraFrameFlag = mediaFormat.GetMediaType() == OpalMediaType::Video();
  m_forceIntraFrameTimer = 500;
#endif

  return OpalMediaStream::Open();
}


bool OpalRTPMediaStream::IsOpen() const
{
  return OpalMediaStream::IsOpen() && m_rtpSession.IsOpen();
}


void OpalRTPMediaStream::OnStartMediaPatch()
{
  // Make sure a RTCP packet goes out as early as possible, helps with issues
  // to do with ICE, DTLS, NAT etc.
  if (IsSink()) {
    while (IsOpen() && m_rtpSession.SendReport(true) == OpalRTPSession::e_IgnorePacket) {
      PTRACE(m_throttleSendReport, m_rtpSession << "initial send report write delayed.");
      PThread::Sleep(20);
    }
  }

  OpalMediaStream::OnStartMediaPatch();
}


void OpalRTPMediaStream::InternalClose()
{
  // Break any I/O blocks and wait for the thread that uses this object to
  // terminate before we allow it to be deleted.
  if (m_jitterBuffer != NULL) {
    m_rtpSession.RemoveDataNotifier(m_receiveNotifier);
    m_rtpSession.SetJitterBuffer(NULL, m_syncSource);
    m_jitterBuffer->Close();
    // Don't delete m_jitterBuffer as read thread may still be running
  }
}


bool OpalRTPMediaStream::InternalSetPaused(bool pause, bool fromUser, bool fromPatch)
{
  if (!OpalMediaStream::InternalSetPaused(pause, fromUser, fromPatch))
    return false; // Had not changed

  if (IsSource()) {
    // We make referenced copy of pointer so can't be deleted out from under us
    OpalMediaPatchPtr mediaPatch = m_mediaPatch;
    if (mediaPatch != NULL)
      mediaPatch->EnableJitterBuffer(!pause);
  }

  return true;
}


bool OpalRTPMediaStream::InternalExecuteCommand(const OpalMediaCommand & command)
{
#if OPAL_VIDEO
  if (dynamic_cast<const OpalVideoUpdatePicture *>(&command) != NULL) {
    if (m_videoUpdateThrottleTimer.IsRunning()) {
      PTRACE(4, "Throttled " << command);
      return false;
    }

    PTimeInterval throttleTime = m_videoUpdateThrottleTime;
    if (throttleTime == 0) {
      throttleTime = m_rtpSession.GetRoundTripTime()*2;
      if (throttleTime == 0)
        throttleTime.SetInterval(0, 1);
    }
    m_videoUpdateThrottleTimer = throttleTime;
  }
#endif

  return OpalMediaStream::InternalExecuteCommand(command);
}


void OpalRTPMediaStream::OnReceivedPacket(OpalRTPSession &, OpalRTPSession::Data & data)
{
  if (m_jitterBuffer != NULL)
    m_jitterBuffer->WriteData(data.m_frame);
}


PBoolean OpalRTPMediaStream::ReadPacket(RTP_DataFrame & packet)
{
  if (!IsOpen()) {
    PTRACE(4, "Read from closed stream");
    return false;
  }

  if (IsSink()) {
    PTRACE(1, "Tried to read from sink media stream");
    return false;
  }

  if (PAssertNULL(m_jitterBuffer) == NULL || !m_jitterBuffer->ReadData(packet))
    return false;

#if OPAL_VIDEO
  if (packet.GetDiscontinuity() > 0 && mediaFormat.GetMediaType() == OpalMediaType::Video()) {
    PTRACE(3, "Automatically requesting video update due to " << packet.GetDiscontinuity() << " missing packets.");
    ExecuteCommand(OpalVideoPictureLoss(packet.GetSequenceNumber(), packet.GetTimestamp(), 0, packet.GetSyncSource()));
  }
#endif

  timestamp = packet.GetTimestamp();
  return true;
}


PBoolean OpalRTPMediaStream::WritePacket(RTP_DataFrame & packet)
{
  if (!IsOpen()) {
    PTRACE(4, "Write to closed stream");
    return false;
  }

  if (IsSource()) {
    PTRACE(1, "Tried to write to source media stream");
    return false;
  }

#if OPAL_VIDEO
  /* This is to allow for remote systems that are not quite ready to receive
     video immediately after the stream is set up. Specification says they
     MUST, but ... sigh. So, they sometime miss the first few packets which
     contains Intra-Frame and then, though further failure of implementation,
     do not subsequently ask for a new Intra-Frame using one of the several
     mechanisms available. Net result, no video. It won't hurt to send another
     Intra-Frame, so we do. Thus, interoperability is improved! */
  if (m_forceIntraFrameFlag && m_forceIntraFrameTimer.HasExpired()) {
    PTRACE(3, "Forcing I-Frame after start up in case remote does not ask");
    ExecuteCommand(OpalVideoUpdatePicture());
    m_forceIntraFrameFlag = false;
  }
#endif

  timestamp = packet.GetTimestamp();

  if (m_rewriteHeaders && packet.GetPayloadSize() == 0
#if OPAL_VIDEO
          && (!packet.GetMarker() || GetMediaFormat().GetMediaType() != OpalMediaType::Video())
#endif
      )
    return true; // Ignore empty packets, except for video with marker, which can plausibly be empty

  if (m_syncSource != 0)
    packet.SetSyncSource(m_syncSource);

  while (IsOpen()) {
    switch (m_rtpSession.WriteData(packet, m_rewriteHeaders ? OpalRTPSession::e_RewriteHeader : OpalRTPSession::e_RewriteSSRC)) {
      case OpalRTPSession::e_AbortTransport :
        return false;

      case OpalRTPSession::e_ProcessPacket :
        return true;

      case OpalRTPSession::e_IgnorePacket :
        PTRACE(m_throttleWriteData, m_rtpSession << "write data delayed.");
        PThread::Sleep(20);
        break;
    }
  }

  return false;
}


PBoolean OpalRTPMediaStream::SetDataSize(PINDEX PTRACE_PARAM(dataSize), PINDEX /*frameTime*/)
{
  PTRACE(3, "Data size cannot be changed to " << dataSize << ", fixed at " << GetDataSize());
  return true;
}


PString OpalRTPMediaStream::GetPatchThreadName() const
{
  return PSTRSTRM((IsSource() ? 'R' : 'T') << "x " << GetMediaFormat().GetMediaType());
}


PBoolean OpalRTPMediaStream::IsSynchronous() const
{
  // Sinks never block
  if (!IsSource())
    return false;

  // Source will bock if no jitter buffer, either not needed ...
  if (!mediaFormat.NeedsJitterBuffer())
    return true;

  // ... or is disabled
  if (connection.GetMaxAudioJitterDelay() == 0)
    return true;

  // Finally, are asynchonous if external or in RTP bypass mode. These are the
  // same conditions as used when not creating patch thread at all.
  return RequiresPatchThread();
}


PBoolean OpalRTPMediaStream::RequiresPatchThread() const
{
  return !dynamic_cast<OpalRTPEndPoint &>(connection.GetEndPoint()).CheckForLocalRTP(*this);
}


bool OpalRTPMediaStream::InternalSetJitterBuffer(const OpalJitterBuffer::Init & init)
{
  if (!IsOpen() || IsSink() || !RequiresPatchThread() || m_jitterBuffer == NULL)
    return false;

  PTRACE_IF(4, init.m_maxJitterDelay == 0, "Jitter", "Switching off jitter buffer " << *m_jitterBuffer);
  m_jitterBuffer->SetDelay(init);

  return true;
}


bool OpalRTPMediaStream::InternalUpdateMediaFormat(const OpalMediaFormat & newMediaFormat)
{
  return OpalMediaStream::InternalUpdateMediaFormat(newMediaFormat) &&
         m_rtpSession.UpdateMediaFormat(mediaFormat); // use the newly adjusted mediaFormat
}


PBoolean OpalRTPMediaStream::SetPatch(OpalMediaPatch * patch)
{
  if (!IsOpen() || IsSink())
    return OpalMediaStream::SetPatch(patch);

  if (m_jitterBuffer == NULL)
    return false;

  OpalMediaPatchPtr oldPatch = InternalSetPatchPart1(patch);
  m_jitterBuffer->Close();
  InternalSetPatchPart2(oldPatch);
  m_jitterBuffer->Restart();
  return true;
}


#if OPAL_STATISTICS
void OpalRTPMediaStream::GetStatistics(OpalMediaStatistics & statistics, bool fromPatch) const
{
  OpalMediaStream::GetStatistics(statistics, fromPatch);
  m_rtpSession.GetStatistics(statistics, IsSource(), m_syncSource);

  if (m_jitterBuffer != NULL) {
    statistics.m_packetsTooLate    = m_jitterBuffer->GetPacketsTooLate();
    statistics.m_packetOverruns    = m_jitterBuffer->GetBufferOverruns();
    statistics.m_jitterBufferDelay = m_jitterBuffer->GetCurrentJitterDelay()/m_jitterBuffer->GetTimeUnits();
  }
  else {
    statistics.m_packetsTooLate    = 0;
    statistics.m_packetOverruns    = 0;
    statistics.m_jitterBufferDelay = 0;
  }
}
#endif


void OpalRTPMediaStream::PrintDetail(ostream & strm, const char * prefix, Details details) const
{
  OpalMediaStream::PrintDetail(strm, prefix, details - DetailEOL);

#if OPAL_PTLIB_NAT
  if ((details & DetailNAT) && m_rtpSession.IsOpen()) {
    PString sockName = m_rtpSession.GetDataSocket().GetName();
    if (sockName.NumCompare("udp") != PObject::EqualTo)
      strm << ", " << sockName.Left(sockName.Find(':'));
  }
#endif // OPAL_PTLIB_NAT

#if OPAL_SRTP
  if ((details & DetailSecured) && m_rtpSession.IsCryptoSecured(IsSource()))
    strm << ", " << "secured";
#endif // OPAL_SRTP

#if OPAL_RTP_FEC
  if ((details & DetailFEC) && m_rtpSession.GetUlpFecPayloadType() != RTP_DataFrame::IllegalPayloadType)
    strm << ", " << "error correction";
#endif // OPAL_RTP_FEC

  if (details & DetailAddresses) {
    strm << "\n  media=" << m_rtpSession.GetRemoteAddress(true) << "<if=" << m_rtpSession.GetLocalAddress(true) << '>';
    if (!m_rtpSession.GetRemoteAddress(false).IsEmpty())
      strm << "\n  control=" << m_rtpSession.GetRemoteAddress(false) << "<if=" << m_rtpSession.GetLocalAddress(false) << '>';
  }

  if (details & DetailEOL)
    strm << endl;
}


// End of file ////////////////////////////////////////////////////////////////
