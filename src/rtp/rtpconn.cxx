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
 */

#include <ptlib.h>

#ifdef P_USE_PRAGMA
#pragma implementation "rtpconn.h"
#endif

#include <opal_config.h>

#include <rtp/rtpconn.h>
#include <rtp/rtpep.h>
#include <opal/manager.h>
#include <codec/rfc2833.h>
#include <t38/t38proto.h>
#include <opal/patch.h>

#if OPAL_VIDEO
#include <codec/vidcodec.h>
#endif

#include <algorithm>

#define PTraceModule() "RTPCon"

#define new PNEW


///////////////////////////////////////////////////////////////////////////////

OpalRTPConnection::OpalRTPConnection(OpalCall & call,
                             OpalRTPEndPoint  & ep,
                                const PString & token,
                                   unsigned int options,
                                StringOptions * stringOptions)
  : OpalConnection(call, ep, token, options, stringOptions)
  , m_remoteBehindNAT(false)
{
  m_rfc2833Handler = new OpalRFC2833Proto(PCREATE_NOTIFIER(OnUserInputInlineRFC2833), OpalRFC2833);
  PTRACE_CONTEXT_ID_TO(m_rfc2833Handler);

#if OPAL_T38_CAPABILITY
  m_ciscoNSEHandler = new OpalRFC2833Proto(PCREATE_NOTIFIER(OnUserInputInlineRFC2833), OpalCiscoNSE);
  PTRACE_CONTEXT_ID_TO(m_ciscoNSEHandler);
#endif
}

OpalRTPConnection::~OpalRTPConnection()
{
  delete m_rfc2833Handler;
#if OPAL_T38_CAPABILITY
  delete m_ciscoNSEHandler;
#endif
}


void OpalRTPConnection::OnReleased()
{
  for (SessionMap::iterator it = m_sessions.begin(); it != m_sessions.end(); ++it)
    it->second->Close();

  OpalConnection::OnReleased();

  if (m_rfc2833Handler != NULL)
    m_rfc2833Handler->UseRTPSession(false, NULL);
#if OPAL_T38_CAPABILITY
  if (m_ciscoNSEHandler != NULL)
    m_ciscoNSEHandler->UseRTPSession(false, NULL);
#endif

  m_sessions.RemoveAll();
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

  unsigned defaultSessionId = mediaType->GetDefaultSessionId();
  if (defaultSessionId >= nextSessionId ||
      GetMediaStream(defaultSessionId,  isSource) != NULL ||
      GetMediaStream(defaultSessionId, !isSource) != NULL)
    return nextSessionId;

  return defaultSessionId;
}


OpalMediaCryptoSuite::KeyExchangeModes OpalRTPConnection::GetMediaCryptoKeyExchangeModes() const
{
  return OpalMediaCryptoSuite::KeyExchangeModes::All();
}


vector<bool> OpalRTPConnection::CreateAllMediaSessions()
{
  OpalMediaTypeList allMediaTypes = GetLocalMediaFormats().GetMediaTypes();
  allMediaTypes.PrioritiseAudioVideo();

  OpalMediaCryptoSuite::KeyExchangeModes keyExchangeModes = GetMediaCryptoKeyExchangeModes();
  const PStringArray cryptoSuites = GetMediaCryptoSuites();
  PTRACE(4, "Creating media sessions: media=" << setfill(',') << allMediaTypes << " - crypto=" << cryptoSuites);

  vector<bool> openedMediaSessions(allMediaTypes.size()*cryptoSuites.GetSize()+1);

  for (OpalMediaTypeList::iterator iterMediaType = allMediaTypes.begin(); iterMediaType != allMediaTypes.end(); ++iterMediaType) {
    const OpalMediaType & mediaType = *iterMediaType;
    if (!PAssert(mediaType.GetDefinition() != NULL, PLogicError))
      continue;

    if (GetAutoStart(mediaType) == OpalMediaType::DontOffer) {
      PTRACE(4, "Not offering " << mediaType);
      continue;
    }

    for (PINDEX csIdx = 0; csIdx < cryptoSuites.GetSize(); ++csIdx) {
      PCaselessString cryptoSuiteName = cryptoSuites[csIdx];
      if (cryptoSuiteName == OpalMediaCryptoSuite::ClearText() && !(keyExchangeModes & OpalMediaCryptoSuite::e_AllowClear)) {
        PTRACE(4, "Skipping " << cryptoSuiteName << " as secure media required.");
        continue;
      }
      if (cryptoSuiteName != OpalMediaCryptoSuite::ClearText() && keyExchangeModes == OpalMediaCryptoSuite::e_AllowClear) {
        PTRACE(4, "Skipping " << cryptoSuiteName << " as non-secure media required.");
        continue;
      }

      OpalMediaCryptoSuite * cryptoSuite = OpalMediaCryptoSuiteFactory::CreateInstance(cryptoSuiteName);
      if (cryptoSuite == NULL) {
        PTRACE(1, "Cannot create crypto suite for " << cryptoSuiteName);
        continue;
      }

      PCaselessString sessionType = mediaType->GetMediaSessionType();
      if (!cryptoSuite->ChangeSessionType(sessionType, keyExchangeModes)) {
        PTRACE(3, "Cannot use crypto suite " << cryptoSuiteName << " with media type " << mediaType);
        continue;
      }

      OpalMediaSession * session = NULL;

      unsigned nextSessionId = 1;
      for (SessionMap::const_iterator it = m_sessions.begin(); it != m_sessions.end(); ++it) {
        if (it->second->GetMediaType() == mediaType && it->second->GetSessionType() == sessionType) {
          session = it->second;
          PTRACE_IF(4, !openedMediaSessions[session->GetSessionID()],
                    "Session " << session->GetSessionID() << " for " << mediaType << " and " << sessionType << " already created.");
          break;
        }
        if (nextSessionId <= it->first)
          nextSessionId = it->first+1;
      }

      if (session == NULL && (session = UseMediaSession(nextSessionId, mediaType, sessionType)) == NULL) {
        PTRACE(1, "Could not create RTP session for media type " << mediaType);
        continue;
      }

      session->OfferCryptoSuite(cryptoSuiteName);

      openedMediaSessions[session->GetSessionID()] = true;
    }
  }

  return openedMediaSessions;
}


OpalMediaSession * OpalRTPConnection::GetMediaSession(unsigned sessionID) const
{
  SessionMap::const_iterator it = m_sessions.find(sessionID);
  return it != m_sessions.end() ? it->second : NULL;
}


OpalMediaSession * OpalRTPConnection::FindSessionByMediaType(const OpalMediaType & mediaType) const
{
  for (SessionMap::const_iterator it = m_sessions.begin(); it != m_sessions.end(); ++it) {
    if (it->second->GetMediaType() == mediaType)
      return &*it->second;
  }
  return NULL;
}


OpalMediaSession * OpalRTPConnection::FindSessionByLocalPort(WORD port) const
{
  for (SessionMap::const_iterator it = m_sessions.begin(); it != m_sessions.end(); ++it) {
    OpalRTPSession * rtp = dynamic_cast<OpalRTPSession * >(&*it->second);
    if (rtp != NULL && (rtp->GetLocalDataPort() == port || rtp->GetLocalControlPort() == port))
      return rtp;
  }

  return NULL;
}


OpalMediaSession * OpalRTPConnection::UseMediaSession(unsigned sessionId,
                                                      const OpalMediaType & mediaType,
                                                      const PString & sessionType)
{
  SessionMap::iterator it = m_sessions.find(sessionId);
  if (it != m_sessions.end()) {
    PTRACE(4, "Found existing " << it->second->GetMediaType() << " session " << sessionId);
    return it->second;
  }

  OpalMediaSession * session = CreateMediaSession(sessionId, mediaType, sessionType);
  if (session != NULL)
    m_sessions.SetAt(sessionId, session);

  return session;
}


OpalMediaSession * OpalRTPConnection::CreateMediaSession(unsigned sessionId,
                                                         const OpalMediaType & mediaType,
                                                         const PString & sessionType)
{
  OpalMediaSession::Init init(*this, sessionId, mediaType, m_remoteBehindNAT);

  {
    OpalTransportAddressArray transports;
    PSafePtr<OpalConnection> otherConnection = GetOtherPartyConnection();
    if (otherConnection != NULL && otherConnection->GetMediaTransportAddresses(*this, sessionId, mediaType, transports)) {
      // Make sure we do not include any transcoded format combinations
      m_localMediaFormats.Remove(PString('@')+mediaType);
      m_localMediaFormats += otherConnection->GetMediaFormats();
      PTRACE(4, "Created dummy " << mediaType << " session " << sessionId
             << " using formats " << setfill(',') << m_localMediaFormats << " on " << *this);
      return new OpalDummySession(init, transports);
    }
  }

  PString actualSessionType = sessionType;
  if (actualSessionType.IsEmpty())
    actualSessionType = mediaType->GetMediaSessionType();

  OpalMediaSession * session = OpalMediaSessionFactory::CreateInstance(actualSessionType, init);
  if (session == NULL) {
    PTRACE(1, "Cannot create session for " << actualSessionType << " on " << *this);
    return NULL;
  }

  session->SetStringOptions(GetStringOptions());

  PTRACE(4, "Created " << mediaType << " session " << sessionId << " using " << actualSessionType << " on " << *this);
  return session;
}


bool OpalRTPConnection::GetMediaTransportAddresses(OpalConnection & otherConnection,
                                                         unsigned   sessionId,
                                              const OpalMediaType & mediaType,
                                        OpalTransportAddressArray & transports) const
{
  if (!OpalConnection::GetMediaTransportAddresses(otherConnection, sessionId, mediaType, transports))
    return false;

  if (!transports.IsEmpty())
    return true;

  SessionMap::const_iterator session = m_sessions.find(sessionId);
  if (session == m_sessions.end() || session->second->GetMediaType() != mediaType) {
    for (session = m_sessions.begin(); session != m_sessions.end(); ++session) {
      if (session->second->GetMediaType() == mediaType)
        break;
    }
    if (session == m_sessions.end()) {
      PTRACE(4, "GetMediaTransportAddresses of " << mediaType << " had no session for " << otherConnection << " on " << *this);
      return true;
    }
  }

  if (transports.SetAddressPair(session->second->GetRemoteAddress(true), session->second->GetRemoteAddress(false)))
    PTRACE(3, "GetMediaTransportAddresses of " << mediaType << " found session addresses "
           << setfill(',') << transports << " for " << otherConnection << " on " << *this);
  else
    PTRACE(4, "GetMediaTransportAddresses of " << mediaType << " had no transports in sessions for " << otherConnection << " on " << *this);

  return true;
}


#if OPAL_RTP_FEC
OpalMediaFormatList OpalRTPConnection::NegotiateFECMediaFormats(OpalMediaSession & session)
{
  OpalMediaFormatList fecFormats;

  OpalRTPSession * rtpSession = dynamic_cast<OpalRTPSession *>(&session);
  if (rtpSession != NULL) {
    OpalMediaFormatList remoteFormats = GetMediaFormats();

    for (OpalMediaFormatList::iterator it = m_localMediaFormats.begin(); it != m_localMediaFormats.end(); ++it) {
      if (it->GetMediaType() == OpalFEC::MediaType() && it->GetOptionString(OpalFEC::MediaTypeOption()) == session.GetMediaType()) {
        OpalMediaFormatList::const_iterator fmt = remoteFormats.FindFormat(*it);
        if (fmt != remoteFormats.end()) {
          fecFormats += *fmt;

          if (fmt->GetName().NumCompare(OPAL_REDUNDANT_PREFIX) == EqualTo)
            rtpSession->SetRedundencyPayloadType(fmt->GetPayloadType());
          else if (fmt->GetName().NumCompare(OPAL_ULP_FEC_PREFIX) == EqualTo)
            rtpSession->SetUlpFecPayloadType(fmt->GetPayloadType());

          PTRACE(4, "Accepted redundant format " << *fmt << ", pt=" << fmt->GetPayloadType());
        }
      }
    }
  }

  return fecFormats;
}
#endif // OPAL_RTP_FEC


bool OpalRTPConnection::ChangeSessionID(unsigned fromSessionID, unsigned toSessionID)
{
  SessionMap::iterator from = m_sessions.find(fromSessionID);
  if (from == m_sessions.end()) {
    PTRACE(2, "Attempt to renumber unknown session " << fromSessionID << " to session ID " << toSessionID);
    return false;
  }

  SessionMap::iterator to = m_sessions.find(toSessionID);
  if (to != m_sessions.end()) {
    PTRACE(2, "Attempt to renumber session " << fromSessionID << " to existing session ID " << toSessionID);

    m_sessions.erase(from);
  }
  else {
    PTRACE(3, "Changing session ID " << fromSessionID << " to " << toSessionID);

    PSafePtr<OpalMediaSession> session = from->second;
    m_sessions.DisallowDeleteObjects();
    m_sessions.erase(from);
    m_sessions.AllowDeleteObjects();
    m_sessions.SetAt(toSessionID, session);
  }

  for (StreamDict::iterator it = m_mediaStreams.begin(); it != m_mediaStreams.end(); ++it) {
    OpalMediaStreamPtr stream = it->second;
    if (stream.SetSafetyMode(PSafeReadWrite) && stream->GetSessionID() == fromSessionID) {
      stream->SetSessionID(toSessionID);
      OpalMediaPatchPtr patch = stream->GetPatch();
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
  {
    SessionMap::iterator it = m_sessions.find(sessionId);
    if (it == m_sessions.end()) {
      PTRACE(4, "Replacing empty session " << sessionId);
      m_sessions.SetAt(sessionId, mediaSession);
      return;
    }

    OpalTransportAddress remoteMedia = it->second->GetRemoteAddress(true);
    OpalTransportAddress remoteCtrl = it->second->GetRemoteAddress(false);

    mediaSession->AttachTransport(it->second->DetachTransport());

    mediaSession->SetRemoteAddress(remoteMedia, true);
    mediaSession->SetRemoteAddress(remoteCtrl, false);
    PTRACE(4, "Replacing session " << sessionId << " media=" << remoteMedia << " ctrl=" << remoteCtrl);
  }

  OpalRTPSession * rtpSession = dynamic_cast<OpalRTPSession *>(mediaSession);
  if (rtpSession != NULL && rtpSession->IsAudio()) {
    if (m_rfc2833Handler != NULL)
      m_rfc2833Handler->UseRTPSession(false, rtpSession);

#if OPAL_T38_CAPABILITY
    if (m_ciscoNSEHandler != NULL)
      m_ciscoNSEHandler->UseRTPSession(false, rtpSession);
#endif
  }

  m_sessions.SetAt(sessionId, mediaSession);
}


void OpalRTPConnection::ReleaseMediaSession(unsigned sessionID)
{
  SessionMap::iterator it = m_sessions.find(sessionID);
  if (it == m_sessions.end()) {
    PTRACE(2, "Attempt to release unknown session " << sessionID);
    return;
  }

  PTRACE(3, "Released session " << sessionID);
  it->second->Close();

  OpalRTPSession * rtpSession = dynamic_cast<OpalRTPSession *>(&*it->second);
  if (rtpSession != NULL && rtpSession->IsAudio()) {
    if (m_rfc2833Handler != NULL)
      m_rfc2833Handler->UseRTPSession(false, NULL);

#if OPAL_T38_CAPABILITY
    if (m_ciscoNSEHandler != NULL)
      m_ciscoNSEHandler->UseRTPSession(false, NULL);
#endif
  }

  m_sessions.erase(it);
}


#if OPAL_VIDEO
void OpalRTPConnection::AddAudioVideoGroup(const PString & id)
{
  for (SessionMap::iterator it = m_sessions.begin(); it != m_sessions.end(); ++it) {
    if (it->second->GetMediaType() == OpalMediaType::Audio() || it->second->GetMediaType() == OpalMediaType::Video())
      it->second->AddGroup(id, it->second->GetMediaType(), false);
  }
}
#endif // OPAL_VIDEO


bool OpalRTPConnection::SetSessionQoS(OpalRTPSession * /*session*/)
{
  return true;
}


void OpalRTPConnection::DetermineRTPNAT(const OpalTransport & transport, const OpalTransportAddress & signalAddr)
{
  if (m_remoteBehindNAT) {
    PTRACE(4, "Already determined is behind NAT");
    return;
  }

  if (m_stringOptions.GetBoolean(OPAL_OPT_DISABLE_NAT)) {
    PTRACE(4, "NAT detection has been disabled");
    return;
  }

  PIPSocket::Address localAddr, peerAddr, sigAddr;

  transport.GetLocalAddress().GetIpAddress(localAddr);
  transport.GetRemoteAddress().GetIpAddress(peerAddr);
  signalAddr.GetIpAddress(sigAddr);

  if (dynamic_cast<OpalRTPEndPoint &>(m_endpoint).IsRTPNATEnabled(*this, localAddr, peerAddr, sigAddr, !IsOriginating())) {
    PTRACE(4, "Determined is behind NAT");
    m_remoteBehindNAT = true;
    for (SessionMap::const_iterator it = m_sessions.begin(); it != m_sessions.end(); ++it)
      it->second->SetRemoteBehindNAT();
  }
}


bool OpalRTPConnection::OnMediaCommand(OpalMediaStream & stream, const OpalMediaCommand & command)
{
  if (stream.IsSource() != (&stream.GetConnection() == this)) {
    PTRACE(5, "OnMediaCommand \"" << command << "\" on " << stream << " not processed for " << *this);
    return OpalConnection::OnMediaCommand(stream, command);
  }

  PTRACE(5, "OnMediaCommand \"" << command << "\" on " << stream << " processed for " << *this);

  unsigned sessionID = stream.GetSessionID();
  OpalRTPSession * session = dynamic_cast<OpalRTPSession *>(GetMediaSession(sessionID));
  if (session == NULL)
    return OpalConnection::OnMediaCommand(stream, command);

#if OPAL_VIDEO

  const OpalMediaFlowControl * flow = dynamic_cast<const OpalMediaFlowControl *>(&command);
  if (flow != NULL && session->SendFlowControl(flow->GetMaxBitRate(), 0, false, flow->GetSyncSource()) == OpalRTPSession::e_ProcessPacket)
    return true;

  const OpalTemporalSpatialTradeOff * tsto = dynamic_cast<const OpalTemporalSpatialTradeOff *>(&command);
  if (tsto != NULL && session->SendTemporalSpatialTradeOff(tsto->GetTradeOff(), tsto->GetSyncSource()) == OpalRTPSession::e_ProcessPacket)
    return true;

  const OpalVideoUpdatePicture * vup = dynamic_cast<const OpalVideoUpdatePicture *>(&command);
  if (vup != NULL) {
    unsigned options = m_stringOptions.GetInteger(OPAL_OPT_VIDUP_METHODS, OPAL_OPT_VIDUP_METHOD_DEFAULT);
    if ((options&(OPAL_OPT_VIDUP_METHOD_RTCP | OPAL_OPT_VIDUP_METHOD_PLI | OPAL_OPT_VIDUP_METHOD_FIR)) != 0) {
      if (PIsDescendant(&command, OpalVideoPictureLoss))
        options |= OPAL_OPT_VIDUP_METHOD_PREFER_PLI;
      if (session->SendIntraFrameRequest(options, vup->GetSyncSource()) == OpalRTPSession::e_ProcessPacket)
        return true;
    }

    PTRACE(4, "RTCP Intra-Frame Request disabled in string options");
  }
#endif // OPAL_VIDEO

  return OpalConnection::OnMediaCommand(stream, command);
}


PBoolean OpalRTPConnection::SendUserInputTone(char tone, unsigned duration)
{
  if (GetRealSendUserInputMode() == SendUserInputAsRFC2833) {
    if (
#if OPAL_T38_CAPABILITY
        m_ciscoNSEHandler->SendToneAsync(tone, duration) ||
#endif
         m_rfc2833Handler->SendToneAsync(tone, duration))
      return true;

    PTRACE(2, "Could not send tone '" << tone << "' via RFC2833.");
  }

  return OpalConnection::SendUserInputTone(tone, duration);
}


OpalMediaStream * OpalRTPConnection::CreateMediaStream(const OpalMediaFormat & mediaFormat,
                                                       unsigned sessionID,
                                                       PBoolean isSource)
{
  OpalMediaSession * mediaSession = UseMediaSession(sessionID, mediaFormat.GetMediaType());
  if (mediaSession != NULL)
    return mediaSession->CreateMediaStream(mediaFormat, sessionID, isSource);

  PTRACE(1, "Unable to create media stream for session " << sessionID);
  return NULL;
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
#if OPAL_RTP_FEC
    if (!mediaFormats.IsEmpty()) {
      mediaFormats += OpalRedundantAudio;
      mediaFormats += OpalUlpFecAudio;
 #if OPAL_VIDEO
      mediaFormats += OpalRedundantVideo;
      mediaFormats += OpalUlpFecVideo;
 #endif
    }
#endif
  }

  OpalConnection::AdjustMediaFormats(local, otherConnection, mediaFormats);

  /* After we have adjusted/filtered/mutilated the media formats list, add/remove
     "rtx" capabilities for any media formats indicating they do NACK. */ 
  if (otherConnection == NULL) {
    if (local) {
      OpalMediaFormatList rtxList;
      for (OpalMediaFormatList::iterator fmt = mediaFormats.begin(); fmt != mediaFormats.end(); ++fmt) {
        if (fmt->GetOptionEnum(OpalMediaFormat::RTCPFeedbackOption(), OpalMediaFormat::e_NoRTCPFb)&OpalMediaFormat::e_NACK) {
          OpalMediaFormat rtx = OpalRtx::GetMediaFormat(fmt->GetMediaType());
          rtx.SetOptionInteger(OpalMediaFormat::ClockRateOption(), fmt->GetClockRate());
          rtx.SetOptionInteger(OpalRtx::AssociatedPayloadTypeOption(), fmt->GetPayloadType());
          PTRACE(5, "Adding rtx media format for " << *fmt << ":\n" << setw(-1) << rtx);
          rtxList += rtx;
        }
      }

      // This may filter the just added "rtx" right back out again
      rtxList.Remove(m_stringOptions(OPAL_OPT_REMOVE_CODEC).Lines());
      rtxList.Remove(GetEndPoint().GetManager().GetMediaFormatMask());
      mediaFormats += rtxList;
    }
    else {
      OpalMediaFormatList::iterator fmt = mediaFormats.begin();
      while (fmt != mediaFormats.end()) {
        const OpalMediaFormat rtx = OpalRtx::GetMediaFormat(fmt->GetMediaType());
        if (rtx == *fmt) {
          RTP_DataFrame::PayloadTypes apt = fmt->GetOptionPayloadType(OpalRtx::AssociatedPayloadTypeOption());
          if (apt != RTP_DataFrame::IllegalPayloadType) {
            bool remove = true;
            for (OpalMediaFormatList::iterator it = mediaFormats.begin(); it != mediaFormats.end(); ++it) {
              if (it->GetPayloadType() == apt) {
                remove = false;
                break;
              }
            }
            if (remove) {
              PTRACE(5, "Removing format " << *fmt << ", apt=" << apt);
              mediaFormats.erase(fmt++);
              continue;
            }
          }
        }

        ++fmt;
      }
    }
  }
}


void OpalRTPConnection::OnPatchMediaStream(PBoolean isSource, OpalMediaPatch & patch)
{
  OpalConnection::OnPatchMediaStream(isSource, patch);

  if (patch.GetSource().GetMediaFormat().GetMediaType() != OpalMediaType::Audio())
    return;

  OpalRTPSession * rtpSession = dynamic_cast<OpalRTPSession *>(GetMediaSession(patch.GetSource().GetSessionID()));
  if (rtpSession == NULL)
    return;

  if (m_rfc2833Handler != NULL)
    m_rfc2833Handler->UseRTPSession(isSource, rtpSession);

#if OPAL_T38_CAPABILITY
  if (m_ciscoNSEHandler != NULL)
    m_ciscoNSEHandler->UseRTPSession(isSource, rtpSession);
#endif
}

void OpalRTPConnection::OnUserInputInlineRFC2833(OpalRFC2833Info & info, P_INT_PTR)
{
  GetEndPoint().GetManager().QueueDecoupledEvent(
          new PSafeWorkArg2<OpalConnection, char, unsigned>(
                 this, info.GetTone(),
                 info.GetDuration(),
                 &OpalConnection::OnUserInputTone));
}


bool OpalRTPConnection::GarbageCollection()
{
  return m_sessions.DeleteObjectsToBeRemoved() & OpalConnection::GarbageCollection();
}


/////////////////////////////////////////////////////////////////////////////
