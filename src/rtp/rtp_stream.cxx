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


/////////////////////////////////////////////////////////////////////////////
/**A descendant of the OpalJitterBuffer that reads RTP_DataFrame instances
   from the OpalRTPSession
  */
class RTP_JitterBuffer : public OpalJitterBufferThread
{
    PCLASSINFO(RTP_JitterBuffer, OpalJitterBufferThread);
  public:
    RTP_JitterBuffer(OpalRTPSession & session, const Init & init)
      : OpalJitterBufferThread(init)
      , m_session(session)
    {
      PTRACE_CONTEXT_ID_FROM(m_session);
    }


    ~RTP_JitterBuffer()
    {
      PTRACE(4, "Jitter", "Destroying jitter buffer " << *this);

      m_running = false;
      bool reopen = m_session.Shutdown(true);

      WaitForThreadTermination();

      if (reopen)
        m_session.Restart(true);
    }


    virtual PBoolean OnReadPacket(RTP_DataFrame & frame)
    {
      if (!m_session.ReadData(frame))
        return false;

      PTRACE(6, "Jitter", "OnReadPacket: Frame from network, timestamp " << frame.GetTimestamp());
      return true;
   }

 protected:
   /**This class extracts data from the outside world by reading from this session variable */
   OpalRTPSession & m_session;
};


#define new PNEW


///////////////////////////////////////////////////////////////////////////////

OpalRTPMediaStream::OpalRTPMediaStream(OpalRTPConnection & conn,
                                   const OpalMediaFormat & mediaFormat,
                                                  PBoolean isSource,
                                          OpalRTPSession & rtp)
  : OpalMediaStream(conn, mediaFormat, rtp.GetSessionID(), isSource)
  , m_rtpSession(rtp)
  , m_syncSource(0)
{
  /* If we are a source then we should set our buffer size to the max
     practical UDP packet size. This means we have a buffer that can accept
     whatever the RTP sender throws at us. For sink, we set it to the
     maximum size based on MTU (or other criteria). */
  m_defaultDataSize = isSource ? conn.GetEndPoint().GetManager().GetMaxRtpPacketSize() : conn.GetMaxRtpPayloadSize();

  m_rtpSession.SafeReference();

  PTRACE(5, "Media\tCreated RTP media session, RTP=" << &rtp);
}


OpalRTPMediaStream::~OpalRTPMediaStream()
{
  Close();
  m_rtpSession.SafeDereference();
}


PBoolean OpalRTPMediaStream::Open()
{
  if (m_isOpen)
    return true;

  m_rtpSession.Restart(IsSource());

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
  if (IsSink())
    m_rtpSession.SendReport(true);

  OpalMediaStream::OnStartMediaPatch();
}


void OpalRTPMediaStream::InternalClose()
{
  // Break any I/O blocks and wait for the thread that uses this object to
  // terminate before we allow it to be deleted.
  m_rtpSession.SetJitterBuffer(m_jitterBuffer, m_syncSource);
  m_jitterBuffer.SetNULL();
  m_rtpSession.Shutdown(IsSource());
}


bool OpalRTPMediaStream::InternalSetPaused(bool pause, bool fromUser, bool fromPatch)
{
  if (!OpalMediaStream::InternalSetPaused(pause, fromUser, fromPatch))
    return false;

  // If coming out of pause, reopen the RTP session, even though it is probably
  // still open, to make sure any pending error/statistic conditions are reset.
  if (!pause)
    m_rtpSession.Restart(IsSource());

  if (IsSource()) {
    // We make referenced copy of pointer so can't be deleted out from under us
    OpalMediaPatchPtr mediaPatch = m_mediaPatch;
    if (mediaPatch != NULL)
      mediaPatch->EnableJitterBuffer(!pause);
  }

  return true;
}


PBoolean OpalRTPMediaStream::ReadPacket(RTP_DataFrame & packet)
{
  if (!IsOpen())
    return false;

  if (IsSink()) {
    PTRACE(1, "Media\tTried to read from sink media stream");
    return false;
  }

  if (!(m_jitterBuffer != NULL ? m_jitterBuffer->ReadData(packet) : m_rtpSession.ReadData(packet)))
    return false;

#if OPAL_VIDEO
  if (packet.GetDiscontinuity() > 0 && mediaFormat.GetMediaType() == OpalMediaType::Video()) {
    PTRACE(3, "Media\tAutomatically requiring video update due to " << packet.GetDiscontinuity() << " missing packets.");
    ExecuteCommand(OpalVideoPictureLoss(packet.GetSequenceNumber(), packet.GetTimestamp()));
  }
#endif

  timestamp = packet.GetTimestamp();
  return true;
}


PBoolean OpalRTPMediaStream::WritePacket(RTP_DataFrame & packet)
{
  if (!IsOpen())
    return false;

  if (IsSource()) {
    PTRACE(1, "Media\tTried to write to source media stream");
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
    PTRACE(3, "Media\tForcing I-Frame after start up in case remote does not ask");
    ExecuteCommand(OpalVideoUpdatePicture());
    m_forceIntraFrameFlag = false;
  }
#endif

  timestamp = packet.GetTimestamp();

  OpalMediaPatchPtr mediaPatch = m_mediaPatch;
  if (mediaPatch != NULL && mediaPatch->IsBypassed())
    return m_rtpSession.WriteData(packet);

  if (packet.GetPayloadSize() == 0)
    return true;

  packet.SetPayloadType(m_payloadType);
  return m_rtpSession.WriteData(packet);
}


PBoolean OpalRTPMediaStream::SetDataSize(PINDEX PTRACE_PARAM(dataSize), PINDEX /*frameTime*/)
{
  PTRACE(3, "Media\tRTP data size cannot be changed to " << dataSize << ", fixed at " << GetDataSize());
  return true;
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
  if (!IsOpen() || IsSink() || !RequiresPatchThread())
    return false;

  if (init.m_maxJitterDelay == 0) {
    PTRACE_IF(4, m_jitterBuffer != NULL, "Jitter", "Switching off jitter buffer " << *m_jitterBuffer);
    m_jitterBuffer.SetNULL();
    return false;
  }

  if (m_jitterBuffer != NULL)
    m_jitterBuffer->SetDelay(init);
  else {
    m_jitterBuffer = new RTP_JitterBuffer(m_rtpSession, init);
    PTRACE(4, "Jitter", "Created RTP jitter buffer " << *m_jitterBuffer);
  }

  m_rtpSession.SetJitterBuffer(m_jitterBuffer, m_syncSource);
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

  OpalMediaPatchPtr oldPatch = InternalSetPatchPart1(patch);
  m_rtpSession.Shutdown(true);
  InternalSetPatchPart2(oldPatch);
  m_rtpSession.Restart(true);
  return true;
}


#if OPAL_STATISTICS
void OpalRTPMediaStream::GetStatistics(OpalMediaStatistics & statistics, bool fromPatch) const
{
  m_rtpSession.GetStatistics(statistics, IsSource());

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

  OpalMediaStream::GetStatistics(statistics, fromPatch);
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
