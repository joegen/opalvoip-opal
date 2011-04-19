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
  rfc2833Handler  = new OpalRFC2833Proto(*this, PCREATE_NOTIFIER(OnUserInputInlineRFC2833), OpalRFC2833);
#if OPAL_T38_CAPABILITY
  ciscoNSEHandler = new OpalRFC2833Proto(*this, PCREATE_NOTIFIER(OnUserInputInlineCiscoNSE), OpalCiscoNSE);
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
    delete it->second;
  }
  m_sessions.clear();
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
    PTRACE(1, "RTP\tMedia definition cannot create session for " << mediaType);
    return NULL;
  }

  m_sessions[sessionId] = session;
  return session;
}


bool OpalRTPConnection::ChangeSessionID(unsigned fromSessionID, unsigned toSessionID)
{
  if (m_sessions.find(toSessionID) != m_sessions.end()) {
    PTRACE(2, "RTP\tAttempt to renumber session " << fromSessionID << " to existing session ID " << toSessionID);
    return false;
  }

  SessionMap::iterator it = m_sessions.find(fromSessionID);
  if (it == m_sessions.end()) {
    PTRACE(2, "RTP\tAttempt to renumber unknown session " << fromSessionID << " to session ID " << toSessionID);
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


void OpalRTPConnection::ReleaseMediaSession(unsigned sessionID)
{
  SessionMap::iterator it = m_sessions.find(sessionID);
  if (it == m_sessions.end()) {
    PTRACE(2, "RTP\tAttempt to release unknown session " << sessionID);
    return;
  }

  if (it->second->Release())
    return;

#ifdef HAS_LIBZRTP
  //check is security mode ZRTP
  if (0 == securityMode.Find("ZRTP")) {
    OpalRTPSession *session = GetSession(sessionID);
    if (NULL != session){
      OpalZrtp_UDP *zsession = (OpalZrtp_UDP*)session;
      if (NULL != zsession->zrtpStream){
        ::zrtp_stop_stream(zsession->zrtpStream);
        zsession->zrtpStream = NULL;
      }
    }
  }

  printf("release session %i\n", sessionID);
#endif

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


#if OPAL_VIDEO
void OpalRTPConnection::OnMediaCommand(OpalMediaCommand & command, INT sessionID)
{
  OpalRTPSession * session = dynamic_cast<OpalRTPSession *>(GetMediaSession(sessionID));
  if (session == NULL)
    return;

  PCaselessString rtcp_fb;
  OpalMediaStreamPtr videoStream = GetMediaStream(sessionID, true);
  if (videoStream != NULL)
    rtcp_fb = videoStream->GetMediaFormat().GetOptionString("RTCP-FB");

  if (PIsDescendant(&command, OpalVideoUpdatePicture)) {
    bool no_AVPF_PLI = rtcp_fb.Find("pli") == P_MAX_INDEX;
    bool no_AVPF_FIR = rtcp_fb.Find("fir") == P_MAX_INDEX;

    if (no_AVPF_PLI && no_AVPF_FIR)
      session->SendIntraFrameRequest(true, false);  // Fall back to RFC2032
    else if (no_AVPF_PLI)
      session->SendIntraFrameRequest(false, false); // Unusual, but possible, use RFC5104 FIR
    else if (no_AVPF_FIR)
      session->SendIntraFrameRequest(false, true);  // More common, use RFC4585 PLI
    else
      session->SendIntraFrameRequest(false, PIsDescendant(&command, OpalVideoPictureLoss));

#if OPAL_STATISTICS
    m_VideoUpdateRequestsSent++;
#endif
  }
  else if (PIsDescendant(&command, OpalTemporalSpatialTradeOff) &&
           rtcp_fb.Find("tstr") != P_MAX_INDEX)
    session->SendTemporalSpatialTradeOff(dynamic_cast<OpalTemporalSpatialTradeOff &>(command).GetTradeOff());
}
#else
void OpalRTPConnection::OnMediaCommand(OpalMediaCommand & /*command*/, INT /*extra*/)
{
}
#endif

void OpalRTPConnection::AttachRFC2833HandlerToPatch(PBoolean isSource, OpalMediaPatch & patch)
{
  if (isSource) {
    OpalRTPMediaStream * mediaStream = dynamic_cast<OpalRTPMediaStream *>(&patch.GetSource());
    if (mediaStream != NULL) {
      OpalRTPSession & rtpSession = mediaStream->GetRtpSession();
      if (rfc2833Handler != NULL) {
        PTRACE(3, "RTPCon\tAdding RFC2833 receive handler");
        rtpSession.AddFilter(rfc2833Handler->GetReceiveHandler());
      }
#if OPAL_T38_CAPABILITY
      if (ciscoNSEHandler != NULL) {
        PTRACE(3, "RTPCon\tAdding Cisco NSE receive handler");
        rtpSession.AddFilter(ciscoNSEHandler->GetReceiveHandler());
      }
#endif
    }
  }
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


PBoolean OpalRTPConnection::GetMediaInformation(unsigned sessionID,
                                         MediaInformation & info) const
{
  if (!mediaTransportAddresses.Contains(sessionID)) {
    PTRACE(2, "RTPCon\tGetMediaInformation for session " << sessionID << " - no channel.");
    return PFalse;
  }

  const OpalTransportAddress & address = mediaTransportAddresses[sessionID];

  PIPSocket::Address ip;
  WORD port;
  if (address.GetIpAndPort(ip, port)) {
    info.data    = OpalTransportAddress(ip, (WORD)(port&0xfffe));
    info.control = OpalTransportAddress(ip, (WORD)(port|0x0001));
  }
  else
    info.data = info.control = address;

  info.rfc2833 = rfc2833Handler->GetPayloadType();
  PTRACE(3, "RTPCon\tGetMediaInformation for session " << sessionID
         << " data=" << info.data << " rfc2833=" << info.rfc2833);
  return PTrue;
}

PBoolean OpalRTPConnection::IsMediaBypassPossible(unsigned) const
{
  return true;
}

OpalMediaStream * OpalRTPConnection::CreateMediaStream(const OpalMediaFormat & mediaFormat,
                                                       unsigned sessionID,
                                                       PBoolean isSource)
{
  if (ownerCall.IsMediaBypassPossible(*this, sessionID))
    return new OpalNullMediaStream(*this, mediaFormat, sessionID, isSource);

  for (OpalMediaStreamPtr mediaStream(mediaStreams, PSafeReference); mediaStream != NULL; ++mediaStream) {
    if (mediaStream->GetSessionID() == sessionID && mediaStream->IsSource() == isSource && !mediaStream->IsOpen())
      return mediaStream;
  }

  OpalMediaSession * mediaSession = UseMediaSession(sessionID, mediaFormat.GetMediaType());
  if (mediaSession == NULL) {
    PTRACE(1, "RTPCon\tUnable to create media stream for session " << sessionID);
    return NULL;
  }

  return mediaSession->CreateMediaStream(mediaFormat, sessionID, isSource);
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

  if (patch.GetSource().GetMediaFormat().GetMediaType() == OpalMediaType::Audio())
    AttachRFC2833HandlerToPatch(isSource, patch);
}

void OpalRTPConnection::OnUserInputInlineRFC2833(OpalRFC2833Info & info, INT type)
{
  // trigger on start of tone only
  if (type == 0)
    OnUserInputTone(info.GetTone(), info.GetDuration() > 0 ? info.GetDuration()/8 : 100);
}

void OpalRTPConnection::OnUserInputInlineCiscoNSE(OpalRFC2833Info & info, INT type)
{
  // trigger on start of tone only
  if (type == 0)
    OnUserInputTone(info.GetTone(), info.GetDuration() > 0 ? info.GetDuration()/8 : 100);
}


/////////////////////////////////////////////////////////////////////////////

OpalRTPConnection::SessionMap::SessionMap()
{
}

OpalRTPConnection::SessionMap::~SessionMap()
{
  for (iterator it = begin(); it != end(); ++it)
    delete it->second;
}


void OpalRTPConnection::SessionMap::operator=(SessionMap & other)
{
  map<unsigned, OpalMediaSession *>::operator=(other);
  other.clear();
}


/////////////////////////////////////////////////////////////////////////////
