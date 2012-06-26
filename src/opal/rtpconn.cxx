/*
 * rtpconn.cxx
 *
 * Connection abstraction
 *
 * Open Phone Abstraction Library (OPAL)
 *
 * Copyright (C) 2007 Post Increment
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
 * The Initial Developer of the Original Code is Post Increment
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#include <ptlib.h>

#ifdef P_USE_PRAGMA
#pragma implementation "rtpconn.h"
#endif

#include <opal/buildopts.h>

#include <opal/rtpconn.h>
#include <opal/rtpep.h>
#include <opal/manager.h>
#include <codec/rfc2833.h>
#include <t38/t38proto.h>
#include <opal/patch.h>

#if OPAL_VIDEO
#include <codec/vidcodec.h>
#endif


#define new PNEW


OpalRTPConnection::OpalRTPConnection(OpalCall & call,
                             OpalRTPEndPoint  & ep,
                                const PString & token,
                                   unsigned int options,
                                StringOptions * stringOptions)
  : OpalConnection(call, ep, token, options, stringOptions)
  , remoteIsNAT(false)
{
  rfc2833Handler = new OpalRFC2833Proto(PCREATE_NOTIFIER(OnUserInputInlineRFC2833), OpalRFC2833);
  PTRACE_CONTEXT_ID_TO(rfc2833Handler);

#if OPAL_T38_CAPABILITY
  ciscoNSEHandler = new OpalRFC2833Proto(PCREATE_NOTIFIER(OnUserInputInlineRFC2833), OpalCiscoNSE);
  PTRACE_CONTEXT_ID_TO(ciscoNSEHandler);
#endif
}

OpalRTPConnection::~OpalRTPConnection()
{
  delete rfc2833Handler;
#if OPAL_T38_CAPABILITY
  delete ciscoNSEHandler;
#endif
}


void OpalRTPConnection::OnReleased()
{
  OpalConnection::OnReleased();

  for (SessionMap::iterator it = m_sessions.begin(); it != m_sessions.end(); ++it) {
    OpalRTPSession * rtp = dynamic_cast<OpalRTPSession * >(it->second);
    if (rtp != NULL && (rtp->GetPacketsSent() != 0 || rtp->GetPacketsReceived() != 0))
      rtp->SendBYE();
  }
}


unsigned OpalRTPConnection::GetNextSessionID(const OpalMediaType & mediaType, bool isSource)
{
  unsigned nextSessionId = 1;

  for (SessionMap::iterator it = m_sessions.begin(); it != m_sessions.end(); ++it) {
    if (nextSessionId <= it->first)
      nextSessionId = it->first+1;
  }

  if (GetMediaStream(mediaType, isSource) != NULL)
    return nextSessionId;

  OpalMediaStreamPtr mediaStream = GetMediaStream(mediaType, !isSource);
  if (mediaStream != NULL)
    return mediaStream->GetSessionID();

  unsigned defaultSessionId = mediaType.GetDefinition()->GetDefaultSessionId();
  if (defaultSessionId >= nextSessionId ||
      GetMediaStream(defaultSessionId,  isSource) != NULL ||
      GetMediaStream(defaultSessionId, !isSource) != NULL)
    return nextSessionId;

  return defaultSessionId;
}


OpalMediaTypeList OpalRTPConnection::CreateAllMediaSessions()
{
  OpalMediaTypeList openedMediaTypes;
  OpalMediaTypeList allMediaTypes = OpalMediaType::GetList();

  for (OpalMediaTypeList::iterator iterMediaType = allMediaTypes.begin(); iterMediaType != allMediaTypes.end(); ++iterMediaType) {
    const OpalMediaType & mediaType = *iterMediaType;
    if (GetAutoStart(mediaType) == OpalMediaType::DontOffer) {
      PTRACE(4, "RTPCon\tNot offerring " << mediaType);
      continue;
    }

    // See if any media formats of this session id, so don't create unused RTP session
    if (!m_localMediaFormats.HasType(mediaType)) {
      PTRACE(3, "RTPCon\tNo media formats of type " << mediaType << ", not creating media sesion");
      continue;
    }

    unsigned sessionId = GetNextSessionID(mediaType, true);
    if (sessionId == 0)
      sessionId = GetNextSessionID(mediaType, false);
    OpalMediaSession * session = UseMediaSession(sessionId, mediaType);
    if (session == NULL) {
      PTRACE(1, "RTPCon\tCould not create RTP session for media type " << mediaType);
      continue;
    }

    CheckForMediaBypass(*session);

    openedMediaTypes.push_back(mediaType);
    ++sessionId;
  }

  return openedMediaTypes;
}


OpalMediaSession * OpalRTPConnection::GetMediaSession(unsigned sessionID) const
{
  SessionMap::const_iterator it = m_sessions.find(sessionID);
  return it != m_sessions.end() ? it->second : NULL;
}


OpalMediaSession * OpalRTPConnection::FindSessionByLocalPort(WORD port) const
{
  for (SessionMap::const_iterator it = m_sessions.begin(); it != m_sessions.end(); ++it) {
    OpalRTPSession * rtp = dynamic_cast<OpalRTPSession * >(it->second);
    if (rtp != NULL && (rtp->GetLocalDataPort() == port || rtp->GetLocalControlPort() == port))
      return rtp;
  }

  return NULL;
}


OpalMediaSession * OpalRTPConnection::UseMediaSession(unsigned sessionId, const OpalMediaType & mediaType)
{
  SessionMap::iterator it = m_sessions.find(sessionId);
  if (it != m_sessions.end()) {
    it->second->Use();
    return it->second;
  }

  OpalMediaTypeDefinition * def = mediaType.GetDefinition();
  if (def == NULL) {
    PTRACE(1, "RTPCon\tNo definition for media type " << mediaType);
    return NULL;
  }

  OpalMediaSession * session = def->CreateMediaSession(*this, sessionId);
  if (session == NULL) {
    PTRACE(1, "RTPCon\tMedia definition cannot create session for " << mediaType);
    return NULL;
  }

  m_sessions[sessionId] = session;

  return session;
}


bool OpalRTPConnection::GetMediaTransportAddresses(const OpalMediaType & mediaType,
                                             OpalTransportAddressArray & transports) const
{
  // Can only do media bypass if we have fast connect
  for (SessionMap::const_iterator session = m_sessions.begin(); session != m_sessions.end(); ++session) {
    if (session->second->GetMediaType() == mediaType) {
      OpalTransportAddress address = session->second->GetRemoteMediaAddress();
      if (!address.IsEmpty()) {
        transports.AppendAddress(address);
        PTRACE(3, "RTPCon\tGetMediaTransport for " << mediaType << " found " << setfill(',') << address);
        return true;
      }
    }
  }

  return OpalConnection::GetMediaTransportAddresses(mediaType, transports);
}


bool OpalRTPConnection::ChangeSessionID(unsigned fromSessionID, unsigned toSessionID)
{
  if (m_sessions.find(toSessionID) != m_sessions.end()) {
    PTRACE(2, "RTPCon\tAttempt to renumber session " << fromSessionID << " to existing session ID " << toSessionID);
    return false;
  }

  SessionMap::iterator it = m_sessions.find(fromSessionID);
  if (it == m_sessions.end()) {
    PTRACE(2, "RTPCon\tAttempt to renumber unknown session " << fromSessionID << " to session ID " << toSessionID);
    return false;
  }

  PTRACE(3, "RTPCon\tChanging session ID " << fromSessionID << " to " << toSessionID);

  m_sessions[toSessionID] = it->second;
  m_sessions.erase(it);

  for (OpalMediaStreamPtr stream(mediaStreams, PSafeReference); stream != NULL; ++stream) {
    if (stream->GetSessionID() == fromSessionID) {
      stream->SetSessionID(toSessionID);
      OpalMediaPatch * patch = stream->GetPatch();
      if (patch != NULL) {
        patch->GetSource().SetSessionID(toSessionID);
        OpalMediaStreamPtr otherStream;
        for (PINDEX i = 0; (otherStream = patch->GetSink(i)) != NULL; ++i)
          otherStream->SetSessionID(toSessionID);
      }
    }
  }

  return true;
}


void OpalRTPConnection::ReplaceMediaSession(unsigned sessionId, OpalMediaSession * mediaSession)
{
  SessionMap::iterator it = m_sessions.find(sessionId);
  if (it == m_sessions.end()) {
    m_sessions[sessionId] = mediaSession;
    return;
  }

  OpalMediaSession::Transport transport = it->second->DetachTransport();
  mediaSession->AttachTransport(transport);
  delete it->second;
  it->second = mediaSession;

  OpalRTPSession * rtpSession = dynamic_cast<OpalRTPSession *>(mediaSession);
  if (rtpSession == NULL || !rtpSession->IsAudio())
    return;

  if (rfc2833Handler != NULL)
    rfc2833Handler->UseRTPSession(false, rtpSession);

#if OPAL_T38_CAPABILITY
  if (ciscoNSEHandler != NULL)
    ciscoNSEHandler->UseRTPSession(false, rtpSession);
#endif
}


void OpalRTPConnection::ReleaseMediaSession(unsigned sessionID)
{
  SessionMap::iterator it = m_sessions.find(sessionID);
  if (it == m_sessions.end()) {
    PTRACE(2, "RTPCon\tAttempt to release unknown session " << sessionID);
    return;
  }

  if (it->second->Release())
    return;

  OpalRTPSession * rtpSession = dynamic_cast<OpalRTPSession *>(it->second);
  if (rtpSession != NULL && rtpSession->IsAudio()) {
    if (rfc2833Handler != NULL)
      rfc2833Handler->UseRTPSession(false, NULL);

#if OPAL_T38_CAPABILITY
    if (ciscoNSEHandler != NULL)
      ciscoNSEHandler->UseRTPSession(false, NULL);
#endif
  }

  delete it->second;
  m_sessions.erase(it);
}


bool OpalRTPConnection::SetSessionQoS(OpalRTPSession * /*session*/)
{
  return true;
}


PBoolean OpalRTPConnection::IsRTPNATEnabled(const PIPSocket::Address & localAddr, 
                                            const PIPSocket::Address & peerAddr,
                                            const PIPSocket::Address & sigAddr,
                                                              PBoolean incoming)
{
  return static_cast<OpalRTPEndPoint &>(endpoint).IsRTPNATEnabled(*this, localAddr, peerAddr, sigAddr, incoming);
}


bool OpalRTPConnection::OnMediaCommand(OpalMediaStream & stream, const OpalMediaCommand & command)
{
  if (OpalConnection::OnMediaCommand(stream, command))
    return true; // Already handled

  unsigned sessionID = stream.GetSessionID();
  OpalRTPSession * session = dynamic_cast<OpalRTPSession *>(GetMediaSession(sessionID));
  if (session == NULL)
    return false;

  OpalVideoFormat::RTCPFeedback rtcp_fb = stream.GetMediaFormat().GetOptionEnum(OpalVideoFormat::RTCPFeedbackOption(),
                                                                                OpalVideoFormat::e_NoRTCPFb);
  if (rtcp_fb == OpalVideoFormat::e_NoRTCPFb) {
    OpalMediaStreamPtr source = GetMediaStream(sessionID, true);
    if (source != NULL)
      rtcp_fb = source->GetMediaFormat().GetOptionEnum(OpalVideoFormat::RTCPFeedbackOption(), OpalVideoFormat::e_NoRTCPFb);
  }

  const OpalMediaFlowControl * flow = dynamic_cast<const OpalMediaFlowControl *>(&command);
  if (flow != NULL) {
    if (rtcp_fb & OpalVideoFormat::e_TMMBR) {
      session->SendFlowControl(flow->GetMaxBitRate());
      return true;
    }
    PTRACE(3, "RTPCon\tRemote not capable of flow control (TMMBR)");
    return false;
  }

#if OPAL_VIDEO

  const OpalTemporalSpatialTradeOff * tsto = dynamic_cast<const OpalTemporalSpatialTradeOff *>(&command);
  if (tsto != NULL) {
    if (rtcp_fb & OpalVideoFormat::e_TSTR) {
      session->SendTemporalSpatialTradeOff(tsto->GetTradeOff());
      return true;
    }
    PTRACE(3, "RTPCon\tRemote not capable of Temporal/Spatial Tradeoff (TSTR)");
    return false;
  }

  if (PIsDescendant(&command, OpalVideoUpdatePicture)) {
    if (m_rtcpIntraFrameRequestTimer.IsRunning()) {
      PTRACE(4, "RTPCon\tRecent RTCP FIR was sent, not sending another");
      return true;
    }

    bool has_AVPF_PLI = rtcp_fb & OpalVideoFormat::e_PLI;
    bool has_AVPF_FIR = rtcp_fb & OpalVideoFormat::e_FIR;

    if (has_AVPF_PLI && has_AVPF_FIR)
      session->SendIntraFrameRequest(false, PIsDescendant(&command, OpalVideoPictureLoss));
    else if (has_AVPF_PLI)
      session->SendIntraFrameRequest(false, true);  // More common, use RFC4585 PLI
    else if (has_AVPF_PLI)
      session->SendIntraFrameRequest(false, false); // Unusual, but possible, use RFC5104 FIR
    else
      session->SendIntraFrameRequest(true, false);  // Fall back to RFC2032

    m_rtcpIntraFrameRequestTimer.SetInterval(0, 1);

#if OPAL_STATISTICS
    m_VideoUpdateRequestsSent++;
#endif

    return true;
  }
#endif // OPAL_VIDEO

  return false;
}


PBoolean OpalRTPConnection::SendUserInputTone(char tone, unsigned duration)
{
  if (GetRealSendUserInputMode() == SendUserInputAsRFC2833) {
    if (
#if OPAL_T38_CAPABILITY
        ciscoNSEHandler->SendToneAsync(tone, duration) ||
#endif
         rfc2833Handler->SendToneAsync(tone, duration))
      return true;

    PTRACE(2, "RTPCon\tCould not send tone '" << tone << "' via RFC2833.");
  }

  return OpalConnection::SendUserInputTone(tone, duration);
}


OpalMediaStream * OpalRTPConnection::CreateMediaStream(const OpalMediaFormat & mediaFormat,
                                                       unsigned sessionID,
                                                       PBoolean isSource)
{
  for (OpalMediaStreamPtr mediaStream(mediaStreams, PSafeReference); mediaStream != NULL; ++mediaStream) {
    if (mediaStream->GetSessionID() == sessionID && mediaStream->IsSource() == isSource && !mediaStream->IsOpen())
      return mediaStream;
  }

  OpalMediaSession * mediaSession = UseMediaSession(sessionID, mediaFormat.GetMediaType());
  if (mediaSession == NULL) {
    PTRACE(1, "RTPCon\tUnable to create media stream for session " << sessionID);
    return NULL;
  }

  CheckForMediaBypass(*mediaSession);

  return mediaSession->CreateMediaStream(mediaFormat, sessionID, isSource);
}


void OpalRTPConnection::CheckForMediaBypass(OpalMediaSession & session)
{
  if (session.IsExternalTransport())
    return;

  PSafePtr<OpalConnection> otherConnection = GetOtherPartyConnection();
  if (otherConnection == NULL)
    return;

  const OpalMediaType & mediaType = session.GetMediaType();
  if (!endpoint.GetManager().AllowMediaBypass(*this, *otherConnection, mediaType))
    return;

  OpalTransportAddressArray transports;
  if (!otherConnection->GetMediaTransportAddresses(mediaType, transports))
    return;

  // Make sure we do not include any transcoded format combinations
  m_localMediaFormats.Remove(PString('@')+mediaType);
  m_localMediaFormats += otherConnection->GetMediaFormats();
  session.SetExternalTransport(transports);
}


void OpalRTPConnection::AdjustMediaFormats(bool   local,
                           const OpalConnection * otherConnection,
                            OpalMediaFormatList & mediaFormats) const
{
  if (otherConnection == NULL && local) {
    OpalMediaFormatList::iterator fmt = mediaFormats.begin();
    while (fmt != mediaFormats.end()) {
      if (fmt->IsTransportable())
        ++fmt;
      else
        mediaFormats -= *fmt++;
    }
  }

  OpalConnection::AdjustMediaFormats(local, otherConnection, mediaFormats);
}


void OpalRTPConnection::OnPatchMediaStream(PBoolean isSource, OpalMediaPatch & patch)
{
  OpalConnection::OnPatchMediaStream(isSource, patch);

  if (patch.GetSource().GetMediaFormat().GetMediaType() != OpalMediaType::Audio())
    return;

  OpalRTPSession * rtpSession = dynamic_cast<OpalRTPSession *>(GetMediaSession(patch.GetSource().GetSessionID()));
  if (rtpSession == NULL)
    return;

  if (rfc2833Handler != NULL)
    rfc2833Handler->UseRTPSession(isSource, rtpSession);

#if OPAL_T38_CAPABILITY
  if (ciscoNSEHandler != NULL)
    ciscoNSEHandler->UseRTPSession(isSource, rtpSession);
#endif
}

void OpalRTPConnection::OnUserInputInlineRFC2833(OpalRFC2833Info & info, INT type)
{
  // trigger on start of tone only
  if (type == 0)
    GetEndPoint().GetManager().QueueUserInput(this, info.GetTone(), info.GetDuration() > 0 ? info.GetDuration()/8 : 100);
}

/////////////////////////////////////////////////////////////////////////////

OpalRTPConnection::SessionMap::SessionMap()
  : m_deleteSessions(true)
{
}

OpalRTPConnection::SessionMap::~SessionMap()
{
  if (m_deleteSessions) {
    for (iterator it = begin(); it != end(); ++it)
      delete it->second;
  }
}


void OpalRTPConnection::SessionMap::Assign(SessionMap & other, bool move)
{
  map<unsigned, OpalMediaSession *>::operator=(other);
  m_deleteSessions = move;
  if (move)
    other.clear();
}


/////////////////////////////////////////////////////////////////////////////
