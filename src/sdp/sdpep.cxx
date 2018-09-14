/*
 * sdpep.cxx
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
 * The Initial Developer of the Original Code is Post Increment
 *
 * Contributor(s): ______________________________________.
 *
 */

#include <ptlib.h>

#ifdef P_USE_PRAGMA
#pragma implementation "sdpep.h"
#endif

#include <sdp/sdpep.h>

#if OPAL_SDP

#include <rtp/rtpconn.h>
#include <rtp/rtp_stream.h>
#include <rtp/dtls_srtp_session.h>
#include <codec/rfc2833.h>
#include <opal/transcoders.h>
#include <opal/patch.h>
#include <ptclib/random.h>
#include <sdp/ice.h>


#define PTraceModule() "SDP-EP"


///////////////////////////////////////////////////////////////////////////////

const PCaselessString & OpalSDPEndPoint::ContentType() { static PConstCaselessString const s("application/sdp"); return s; }

OpalSDPEndPoint::OpalSDPEndPoint(OpalManager & manager,     ///<  Manager of all endpoints.
                       const PCaselessString & prefix,      ///<  Prefix for URL style address strings
                                  Attributes   attributes)  ///<  Bit mask of attributes endpoint has
  : OpalRTPEndPoint(manager, prefix, attributes)
  , m_holdTimeout(0, 40) // Seconds
{
}


OpalSDPEndPoint::~OpalSDPEndPoint()
{
}


PStringList OpalSDPEndPoint::GetAvailableStringOptions() const
{
  static char const * const StringOpts[] = {
    OPAL_OPT_OFFER_SDP_PTIME,
    OPAL_OPT_OFFER_RTCP_FB,
    OPAL_OPT_FORCE_RTCP_FB,
    OPAL_OPT_SUPPRESS_UDP_TLS,
    OPAL_OPT_RTCP_MUX,
    OPAL_OPT_OFFER_REDUCED_SIZE_RTCP,
    OPAL_OPT_SUPPRESS_UDP_TLS,
    #ifdef OPAL_ICE
      OPAL_OPT_OFFER_ICE,
    #endif
    OPAL_OPT_AV_BUNDLE,
    OPAL_OPT_INACTIVE_AUDIO_FLOW,
    OPAL_OPT_MULTI_SSRC
  };

  PStringList list = OpalRTPEndPoint::GetAvailableStringOptions();
  list += PStringList(PARRAYSIZE(StringOpts), StringOpts, true);
  return list;
}


SDPSessionDescription * OpalSDPEndPoint::CreateSDP(time_t sessionId, unsigned version, const OpalTransportAddress & address)
{
  return new SDPSessionDescription(sessionId, version, address);
}


///////////////////////////////////////////////////////////////////////////////

OpalSDPConnection::OpalSDPConnection(OpalCall & call,
                             OpalSDPEndPoint  & ep,
                                const PString & token,
                                   unsigned int options,
                                StringOptions * stringOptions)
  : OpalRTPConnection(call, ep, token, options, stringOptions)
  , m_endpoint(ep)
  , m_offerPending(false)
  , m_sdpSessionId(PTime().GetTimeInSeconds())
  , m_sdpVersion(0)
  , m_holdToRemote(eHoldOff)
  , m_holdFromRemote(false)
{
}


OpalSDPConnection::~OpalSDPConnection()
{
}


OpalMediaFormatList OpalSDPConnection::GetMediaFormats() const
{
  // Need to limit the media formats to what the other side provided in it's offer
  if (!m_activeFormatList.IsEmpty()) {
    PTRACE(4, "Using offered media format list: " << setfill(',') << m_activeFormatList);
    return m_activeFormatList;
  }

  if (!m_remoteFormatList.IsEmpty()) {
    PTRACE(4, "Using remote media format list: " << setfill(',') << m_remoteFormatList);
    return m_remoteFormatList;
  }

  return OpalMediaFormatList();
}


bool OpalSDPConnection::HoldRemote(bool placeOnHold)
{
  PSafeLockReadWrite lock(*this);
  if (!lock.IsLocked())
    return false;

  switch (m_holdToRemote) {
    case eHoldOff :
    case eRetrieveInProgress :
      if (!placeOnHold) {
        PTRACE(4, "Hold off request ignored as not on hold for " << *this);
        return true;
      }
      break;

    case eHoldOn :
    case eHoldInProgress :
      if (placeOnHold) {
        PTRACE(4, "Hold on request ignored as already on hold fir " << *this);
        return true;
      }
      break;
  }

  HoldState origState = m_holdToRemote;

  switch (m_holdToRemote) {
    case eHoldOff :
      m_holdToRemote = eHoldInProgress;
      break;

    case eHoldOn :
      m_holdToRemote = eRetrieveInProgress;
      break;

    case eRetrieveInProgress :
    case eHoldInProgress :
      PTRACE(4, "Hold " << (placeOnHold ? "on" : "off") << " request deferred as in progress for " << *this);
      GetEndPoint().GetManager().QueueDecoupledEvent(new PSafeWorkArg1<OpalSDPConnection, bool>(
                                         this, placeOnHold, &OpalSDPConnection::RetryHoldRemote));
      return true;
  }

  if (OnHoldStateChanged(placeOnHold))
    return true;

  m_holdToRemote = origState;
  return false;
}


void OpalSDPConnection::RetryHoldRemote(bool placeOnHold)
{
  HoldState progressState = placeOnHold ? eRetrieveInProgress : eHoldInProgress;
  PSimpleTimer failsafe(m_endpoint.GetHoldTimeout());
  while (m_holdToRemote == progressState) {
    PThread::Sleep(100);

    if (IsReleased() || failsafe.HasExpired()) {
      PTRACE(3, "Hold " << (placeOnHold ? "on" : "off") << " request failed for " << *this);
      return;
    }

    PTRACE(5, "Hold " << (placeOnHold ? "on" : "off") << " request still in progress for " << *this);
  }

  HoldRemote(placeOnHold);
}


PBoolean OpalSDPConnection::IsOnHold(bool fromRemote) const
{
  return fromRemote ? m_holdFromRemote : (m_holdToRemote >= eHoldOn);
}


bool OpalSDPConnection::GetOfferSDP(SDPSessionDescription & offer, bool offerOpenMediaStreamsOnly)
{
  if (m_offerPending.exchange(true)) {
    PTRACE(2, "Outgoing offer pending, cannot send another offer.");
    return false;
  }

  if (GetPhase() == UninitialisedPhase) {
    InternalSetAsOriginating();
    SetPhase(SetUpPhase);
    OnApplyStringOptions();
  }

  return OnSendOfferSDP(offer, offerOpenMediaStreamsOnly);
}


PString OpalSDPConnection::GetOfferSDP(bool offerOpenMediaStreamsOnly)
{
  std::auto_ptr<SDPSessionDescription> sdp(CreateSDP(PString::Empty()));
  PTRACE_CONTEXT_ID_TO(sdp.get());
  return sdp.get() != NULL && GetOfferSDP(*sdp, offerOpenMediaStreamsOnly) ? sdp->Encode() : PString::Empty();
}


bool OpalSDPConnection::AnswerOfferSDP(const SDPSessionDescription & offer, SDPSessionDescription & answer)
{
  if (m_offerPending) {
    PTRACE(2, "Outgoing offer pending, cannot handle incoming offer.");
    return false;
  }

  if (GetPhase() == UninitialisedPhase) {
    SetPhase(SetUpPhase);
    OnApplyStringOptions();
    if (!OnIncomingConnection(0, NULL))
      return false;
  }

  return OnSendAnswerSDP(offer, answer);
}


PString OpalSDPConnection::AnswerOfferSDP(const PString & offer)
{
  if (GetPhase() == UninitialisedPhase) {
    SetPhase(SetUpPhase);
    OnApplyStringOptions();
    if (!OnIncomingConnection(0, NULL))
      return PString::Empty();
  }

  std::auto_ptr<SDPSessionDescription> sdpIn(CreateSDP(offer));
  std::auto_ptr<SDPSessionDescription> sdpOut(CreateSDP(PString::Empty()));
  if (sdpIn.get() == NULL || sdpOut.get() == NULL)
    return PString::Empty();

  PTRACE_CONTEXT_ID_TO(*sdpIn);
  PTRACE_CONTEXT_ID_TO(*sdpOut);

  if (!OnSendAnswerSDP(*sdpIn, *sdpOut))
    return PString::Empty();

  SetConnected();
  return sdpOut->Encode();
}


bool OpalSDPConnection::HandleAnswerSDP(const SDPSessionDescription & answer)
{
  if (!m_offerPending.exchange(false)) {
    PTRACE(1, "Did not send an offer before handling answer");
    return false;
  }

  bool dummy;
  if (!OnReceivedAnswerSDP(answer, dummy))
    return false;

  InternalOnConnected();
  return true;
}


bool OpalSDPConnection::HandleAnswerSDP(const PString & answer)
{
  std::auto_ptr<SDPSessionDescription> sdp(CreateSDP(answer));
  PTRACE_CONTEXT_ID_TO(sdp.get());
  return sdp.get() != NULL && HandleAnswerSDP(*sdp);
}


SDPSessionDescription * OpalSDPConnection::CreateSDP(const PString & sdpStr)
{
  if (sdpStr.IsEmpty())
    return m_endpoint.CreateSDP(m_sdpSessionId, ++m_sdpVersion, OpalTransportAddress(GetMediaInterface(), 0, OpalTransportAddress::UdpPrefix()));

  OpalMediaFormatList formats = GetLocalMediaFormats();
  if (formats.IsEmpty())
    formats = OpalMediaFormat::GetAllRegisteredMediaFormats();

  SDPSessionDescription * sdpPtr = m_endpoint.CreateSDP(0, 0, OpalTransportAddress());
  PTRACE_CONTEXT_ID_TO(*sdpPtr);

  sdpPtr->SetStringOptions(m_stringOptions);

  if (sdpPtr->Decode(sdpStr, formats))
    return sdpPtr;

  delete sdpPtr;
  return NULL;
}


bool OpalSDPConnection::SetActiveMediaFormats(const OpalMediaFormatList & formats)
{
  if (formats.IsEmpty()) {
    PTRACE(3, "No known media formats in remotes SDP.");
    return false;
  }

  // get the remote media formats
  m_activeFormatList = formats;

  // Remove anything we never offered
  while (!m_activeFormatList.IsEmpty() && m_localMediaFormats.FindFormat(m_activeFormatList.front()) == m_localMediaFormats.end())
    m_activeFormatList.RemoveHead();

  if (!m_activeFormatList.IsEmpty())
    AdjustMediaFormats(false, NULL, m_activeFormatList);

  if (m_activeFormatList.IsEmpty()) {
    PTRACE(3, "All media formats in remotes SDP have been removed.");
    return false;
  }

  // Remember the initial set of media formats remote has told us about
  if (m_remoteFormatList.IsEmpty()) {
    m_remoteFormatList = formats;
    m_remoteFormatList.MakeUnique();
  }

  return true;
}


struct OpalSDPConnection::BundleMergeInfo
{
    vector<bool> m_allowPauseRecvMediaStream;
    vector<bool> m_allowPauseSendMediaStream;
    OpalMediaTransportPtr m_transport;
    RTP_SyncSourceArray m_ssrcs;

    BundleMergeInfo(size_t mediaDescriptionCount)
      : m_allowPauseRecvMediaStream(mediaDescriptionCount+1, true)
      , m_allowPauseSendMediaStream(mediaDescriptionCount+1, true)
    { }

    void RemoveSessionSSRCs(SessionMap & sessions)
    {
      if (m_ssrcs.empty())
        return;

      for (SessionMap::iterator session = sessions.begin(); session != sessions.end(); ++session) {
        OpalRTPSession * rtpSession = dynamic_cast<OpalRTPSession *>(&*session->second);
        if (rtpSession == NULL)
          continue;

        RTP_SyncSourceArray ssrcs = rtpSession->GetSyncSources(OpalRTPSession::e_Receiver);
        for (RTP_SyncSourceArray::const_iterator ssrc = ssrcs.begin(); ssrc != ssrcs.end(); ++ssrc) {
          if (std::find(m_ssrcs.begin(), m_ssrcs.end(), *ssrc) == m_ssrcs.end())
            rtpSession->RemoveSyncSource(*ssrc PTRACE_PARAM(, "clearing non-confirmed SSRC in bundle"));
        }
      }
    }
};


OpalMediaSession * OpalSDPConnection::SetUpMediaSession(const unsigned   sessionId,
                                                   const OpalMediaType & mediaType,
                                             const SDPMediaDescription & mediaDescription,
                                                  OpalTransportAddress & localAddress,
                                                       BundleMergeInfo & bundleMergeInfo)
{
  if (mediaDescription.GetPort() == 0) {
    PTRACE(2, "Received disabled/missing media description for " << mediaType);

    /* Some remotes return all of the media detail (a= lines) in SDP even though
       port is zero indicating the media is not to be used. So don't return these
       bogus media formats from SDP to the "remote media format list". */
    m_remoteFormatList.Remove(PString('@')+mediaType);
    return NULL;
  }

  // Create the OpalMediaSession if required
  OpalMediaSession * session = UseMediaSession(sessionId, mediaType, mediaDescription.GetSessionType());
  if (session == NULL)
    return NULL;

  OpalTransportAddress remoteMediaAddress;
#if OPAL_ICE
  if (mediaDescription.HasICE())
    remoteMediaAddress = GetRemoteMediaAddress();
  else
#endif
  {
    remoteMediaAddress = mediaDescription.GetMediaAddress();
    PTRACE_IF(3, session->IsOpen() && session->GetRemoteAddress() != remoteMediaAddress,
            "Remote changed IP address: " << session->GetRemoteAddress() << " -> " << remoteMediaAddress);
  }

  // Once before opening
  if (!mediaDescription.ToSession(session, bundleMergeInfo.m_ssrcs))
    return NULL;

  bool bundled = session->IsGroupMember(OpalMediaSession::GetBundleGroupId());
  if (bundled && bundleMergeInfo.m_transport != NULL)
    session->AttachTransport(bundleMergeInfo.m_transport);

  if (!session->Open(GetMediaInterface(), remoteMediaAddress))
    return NULL;

  if (bundled && bundleMergeInfo.m_transport == NULL)
    bundleMergeInfo.m_transport = session->GetTransport();

  // And again after
  if (!mediaDescription.ToSession(session, bundleMergeInfo.m_ssrcs))
    return NULL;

  dynamic_cast<OpalRTPEndPoint &>(m_endpoint).CheckEndLocalRTP(*this, dynamic_cast<OpalRTPSession *>(session));
  localAddress = session->GetLocalAddress();
  return session;
}


static bool SetNxECapabilities(OpalRFC2833Proto * handler,
                      const OpalMediaFormatList & localMediaFormats,
                      const OpalMediaFormatList & remoteMediaFormats,
                          const OpalMediaFormat & baseMediaFormat,
                            SDPMediaDescription * localMedia = NULL)
{
  OpalMediaFormatList::const_iterator remFmt = remoteMediaFormats.FindFormat(baseMediaFormat);
  if (remFmt == remoteMediaFormats.end()) {
    // Not in remote list, disable transmitter
    handler->SetTxMediaFormat(OpalMediaFormat());
    return false;
  }

  OpalMediaFormatList::const_iterator localFmt = localMediaFormats.FindFormat(baseMediaFormat);
  if (localFmt == localMediaFormats.end()) {
    // Not in our local list, disable transmitter
    handler->SetTxMediaFormat(OpalMediaFormat());
    return true;
  }

  // Merge remotes format into ours.
  // Note if this is our initial offer remote is the same as local.
  OpalMediaFormat adjustedFormat = *localFmt;
  adjustedFormat.Merge(*remFmt, true);

  handler->SetTxMediaFormat(adjustedFormat);

  if (localMedia != NULL) {
    // Set the receive handler to what we are sending to remote in our SDP
    handler->SetRxMediaFormat(adjustedFormat);
    SDPMediaFormat * fmt = localMedia->CreateSDPMediaFormat();
    if (fmt != NULL) {
      fmt->FromMediaFormat(adjustedFormat);
      localMedia->AddSDPMediaFormat(fmt);
    }
  }

  return true;
}


bool OpalSDPConnection::PauseOrCloseMediaStream(OpalMediaStreamPtr & stream, bool changed, bool paused)
{
  if (stream == NULL)
    return false;

  if (!stream->IsOpen()) {
    PTRACE(4, "Answer SDP, stream closed " << *stream);
    stream.SetNULL();
    return false;
  }

  if (!changed) {
    OpalMediaFormatList::const_iterator fmt = m_activeFormatList.FindFormat(stream->GetMediaFormat());
    if (fmt != m_activeFormatList.end() && stream->UpdateMediaFormat(*fmt, true)) {
      if (paused &&
          m_stringOptions.GetBoolean(OPAL_OPT_INACTIVE_AUDIO_FLOW) &&
          stream->IsSource() &&
          stream->GetMediaFormat().GetMediaType() == OpalMediaType::Audio())
      {
        PTRACE(4, "Answer SDP change pause ignored on stream " << *stream);
        return true;
      }
      PTRACE(4, "Answer SDP change needs to " << (paused ? "pause" : "resume") << " stream " << *stream);
      stream->InternalSetPaused(paused, false, false);
      return !paused;
    }
    PTRACE(4, "Answer SDP (format change) needs to close stream " << *stream);
  }
  else {
    PTRACE(4, "Answer SDP (type change) needs to close stream " << *stream);
  }

  OpalMediaPatchPtr patch = stream->GetPatch();
  if (patch != NULL)
    patch->GetSource().Close();
  stream.SetNULL();
  return false;
}


bool OpalSDPConnection::OnSendOfferSDP(SDPSessionDescription & sdpOut, bool offerOpenMediaStreamsOnly)
{
  bool sdpOK = false;

  if (offerOpenMediaStreamsOnly && !m_mediaStreams.IsEmpty()) {
    PTRACE(3, "Offering only current media streams");
    for (SessionMap::iterator it = m_sessions.begin(); it != m_sessions.end(); ++it) {
      if (OnSendOfferSDPSession(it->first, sdpOut, true))
        sdpOK = true;
      else
        sdpOut.AddMediaDescription(it->second->CreateSDPMediaDescription());
    }
  }
  else {
    PTRACE(3, "Offering all configured media:\n    " << setfill(',') << m_localMediaFormats << setfill(' '));

    if (m_remoteFormatList.IsEmpty()) {
      // Need to fake the remote formats with everything we do,
      // so parts of the offering work correctly
      m_remoteFormatList = GetLocalMediaFormats();
      m_remoteFormatList.MakeUnique();
      AdjustMediaFormats(false, NULL, m_remoteFormatList);
    }

    // Create media sessions based on available media types and make sure audio and video are first two sessions
    vector<bool> sessions = CreateAllMediaSessions();

#if OPAL_VIDEO
    if (m_stringOptions.GetBoolean(OPAL_OPT_AV_BUNDLE))
      AddAudioVideoGroup();
#endif

    OpalMediaTransportPtr bundledTransport;
    for (vector<bool>::size_type sessionId = 1; sessionId < sessions.size(); ++sessionId) {
      if (sessions[sessionId]) {
        OpalMediaSession * session = GetMediaSession(sessionId);

        bool bundled = session->IsGroupMember(OpalMediaSession::GetBundleGroupId());
        if (bundled && bundledTransport != NULL)
          session->AttachTransport(bundledTransport);

        if (OnSendOfferSDPSession(sessionId, sdpOut, false)) {
          sdpOK = true;

          if (bundled && bundledTransport == NULL)
            bundledTransport = session->GetTransport();
        }
        else
          ReleaseMediaSession(sessionId);
      }
    }
  }

  return sdpOK && !sdpOut.GetMediaDescriptions().IsEmpty();
}


bool OpalSDPConnection::OnSendOfferSDPSession(unsigned   sessionId,
                                              SDPSessionDescription & sdp,
                                              bool   offerOpenMediaStreamOnly)
{
  OpalMediaSession * mediaSession = GetMediaSession(sessionId);
  if (mediaSession == NULL) {
    PTRACE(1, "Could not create RTP session " << sessionId);
    return false;
  }

  OpalMediaType mediaType = mediaSession->GetMediaType();
  if (!m_localMediaFormats.HasType(mediaType)) {
    PTRACE(2, "No formats for RTP session " << sessionId << " of type " << mediaType << " in " << setfill(',') << m_localMediaFormats);
    return false;
  }

  if (m_stringOptions.GetBoolean(OPAL_OPT_RTCP_MUX)) {
    OpalRTPSession * rtpSession = dynamic_cast<OpalRTPSession *>(mediaSession);
    if (rtpSession != NULL)
      rtpSession->SetSinglePortRx();
  }

  if (!mediaSession->Open(GetMediaInterface(), GetRemoteMediaAddress())) {
    PTRACE(1, "Could not open RTP session " << sessionId << " for media type " << mediaType);
    return false;
  }

  if (sdp.GetDefaultConnectAddress().IsEmpty())
    sdp.SetDefaultConnectAddress(mediaSession->GetLocalAddress());

  if (!m_stringOptions.GetBoolean(OPAL_OPT_MULTI_SSRC) && mediaSession->IsGroupMember(OpalMediaSession::GetBundleGroupId())) {
    OpalRTPSession * rtpSession = dynamic_cast<OpalRTPSession *>(mediaSession);
    if (rtpSession != NULL) {
      RTP_SyncSourceArray ssrcs = rtpSession->GetSyncSources(OpalRTPSession::e_Sender);
      size_t count = 0;
      for (RTP_SyncSourceArray::iterator ssrc = ssrcs.begin(); ssrc != ssrcs.end(); ++ssrc) {
        if (!rtpSession->GetMediaStreamId(*ssrc, OpalRTPSession::e_Sender).IsEmpty())
          ++count;
      }
      PTRACE(4, "Bundled session has msid for " << count << " of " << ssrcs.size() << " SSRCs");
      if (count > 0) {
        for (RTP_SyncSourceArray::iterator ssrc = ssrcs.begin(); ssrc != ssrcs.end(); ++ssrc) {
          if (!rtpSession->GetMediaStreamId(*ssrc, OpalRTPSession::e_Sender).IsEmpty()) {
            SDPMediaDescription * localMedia = mediaSession->CreateSDPMediaDescription();
            PTRACE_CONTEXT_ID_TO(localMedia);
            if (!OnSendOfferSDPSession(mediaSession, localMedia, offerOpenMediaStreamOnly, *ssrc))
              return false;

            sdp.AddMediaDescription(localMedia);
          }
        }
        return true;
      }
    }
  }

  SDPMediaDescription * localMedia = mediaSession->CreateSDPMediaDescription();
  PTRACE_CONTEXT_ID_TO(localMedia);
  if (!OnSendOfferSDPSession(mediaSession, localMedia, offerOpenMediaStreamOnly, 0))
    return false;

  sdp.AddMediaDescription(localMedia);
  return true;
}


bool OpalSDPConnection::OnSendOfferSDPSession(OpalMediaSession * mediaSession,
                                              SDPMediaDescription * localMedia,
                                              bool offerOpenMediaStreamOnly,
                                              RTP_SyncSourceId ssrc)
{
  OpalMediaType mediaType = mediaSession->GetMediaType();
  if (localMedia == NULL) {
    PTRACE(2, "Can't create SDP media description for media type " << mediaType);
    return false;
  }

  localMedia->SetStringOptions(m_stringOptions);

  if (offerOpenMediaStreamOnly) {
    unsigned sessionId = mediaSession->GetSessionID();
    OpalMediaStreamPtr recvStream = GetMediaStream(sessionId, true);
    OpalMediaStreamPtr sendStream = GetMediaStream(sessionId, false);
    if (recvStream != NULL) {
      OpalMediaFormat rxFormat = recvStream->GetMediaFormat();
      OpalMediaFormatList::const_iterator it = m_localMediaFormats.FindFormat(rxFormat);
      if (it != m_localMediaFormats.end())
        localMedia->AddMediaFormat(*it);
      else {
        PTRACE(2, "Could not find media format " << rxFormat << " from stream " << *recvStream << " in local media formats.");
        localMedia->AddMediaFormat(rxFormat);
      }
    }
    else if (sendStream != NULL)
      localMedia->AddMediaFormat(sendStream->GetMediaFormat());
    else
      localMedia->AddMediaFormats(m_localMediaFormats, mediaType);

    if (sendStream != NULL) {
      // Add in the "rtx" capabilities corressponding to the selected codec
      RTP_DataFrame::PayloadTypes pt = sendStream->GetMediaFormat().GetPayloadType();
      OpalMediaFormatList::const_iterator it;
      while ((it = m_localMediaFormats.FindFormat(OpalRtx::GetName(mediaType), it)) != m_localMediaFormats.end()) {
        if (it->GetOptionPayloadType(OpalRtx::AssociatedPayloadTypeOption()) == pt) {
          localMedia->AddMediaFormat(*it);
          break;
        }
      }
    }

    bool sending = !m_holdFromRemote         && sendStream != NULL && sendStream->IsOpen();
    bool recving =  m_holdToRemote < eHoldOn && recvStream != NULL && recvStream->IsOpen();

    if (sending && recving)
      localMedia->SetDirection(SDPMediaDescription::SendRecv);
    else if (recving)
      localMedia->SetDirection(SDPMediaDescription::RecvOnly);
    else if (sending)
      localMedia->SetDirection(SDPMediaDescription::SendOnly);
    else
      localMedia->SetDirection(SDPMediaDescription::Inactive);

#if PAUSE_WITH_EMPTY_ADDRESS
    if (m_holdToRemote >= eHoldOn) {
      OpalTransportAddress addr = localMedia->GetTransportAddress();
      PIPSocket::Address dummy;
      WORD port;
      addr.GetIpAndPort(dummy, port);
      OpalTransportAddress newAddr("0.0.0.0", port, addr.GetProtoPrefix());
      localMedia->SetTransportAddress(newAddr);
      localMedia->SetDirection(SDPMediaDescription::Undefined);
      sdp.SetDefaultConnectAddress(newAddr);
    }
#endif
  }
  else {
    localMedia->AddMediaFormats(m_localMediaFormats, mediaType);
    localMedia->SetDirection((SDPMediaDescription::Direction)(3&(unsigned)GetAutoStart(mediaType)));
  }

  localMedia->FromSession(mediaSession, NULL, ssrc);

  if (mediaType == OpalMediaType::Audio()) {
    // Set format if we have an RTP payload type for RFC2833 and/or NSE
    // Must be after other codecs, as Mediatrix gateways barf if RFC2833 is first
    SetNxECapabilities(m_rfc2833Handler, m_localMediaFormats, m_remoteFormatList, OpalRFC2833, localMedia);
#if OPAL_T38_CAPABILITY
    SetNxECapabilities(m_ciscoNSEHandler, m_localMediaFormats, m_remoteFormatList, OpalCiscoNSE, localMedia);
#endif
  }

#if OPAL_SRTP
  if (GetMediaCryptoKeyExchangeModes()&OpalMediaCryptoSuite::e_SecureSignalling) {
    OpalMediaCryptoKeyList keys;
    OpalMediaCryptoKeyInfo * txKey = mediaSession->IsCryptoSecured(false);
    if (txKey != NULL)
      keys.Append(txKey->CloneAs<OpalMediaCryptoKeyInfo>());
    else
      keys = mediaSession->GetOfferedCryptoKeys();
    localMedia->SetCryptoKeys(keys);
  }
#endif

#if OPAL_RTP_FEC
  if (GetAutoStart(OpalFEC::MediaType()) != OpalMediaType::DontOffer) {
    OpalMediaFormat redundantMediaFormat;
    for (OpalMediaFormatList::iterator it = m_localMediaFormats.begin(); it != m_localMediaFormats.end(); ++it) {
      if (it->GetMediaType() == OpalFEC::MediaType() && it->GetOptionString(OpalFEC::MediaTypeOption()) == mediaType) {
        if (it->GetName().NumCompare(OPAL_REDUNDANT_PREFIX) == EqualTo)
          redundantMediaFormat = *it;
        else
          localMedia->AddMediaFormat(*it);
      }
    }

    if (redundantMediaFormat.IsValid()) {
      // Calculate the fmtp for red
      PStringStream fmtp;
      OpalMediaFormatList formats = localMedia->GetMediaFormats();
      for (OpalMediaFormatList::iterator it = formats.begin(); it != formats.end(); ++it) {
        if (it->IsTransportable() && *it != redundantMediaFormat) {
          if (!fmtp.IsEmpty())
            fmtp << '/';
          fmtp << (unsigned)it->GetPayloadType();
        }
      }
      redundantMediaFormat.SetOptionString("FMTP", fmtp);
      localMedia->AddMediaFormat(redundantMediaFormat);
    }
  }
#endif // OPAL_RTP_FEC

  return true;
}


bool OpalSDPConnection::OnSendAnswerSDP(const SDPSessionDescription & sdpOffer, SDPSessionDescription & sdpOut, bool transfer)
{
  SetActiveMediaFormats(sdpOffer.GetMediaFormats());

  size_t sessionCount = sdpOffer.GetMediaDescriptions().GetSize();
  vector<SDPMediaDescription *> sdpMediaDescriptions(sessionCount+1);
  size_t sessionId;

  BundleMergeInfo bundleMergeInfo(sessionCount);
#if OPAL_SRTP
  PStringArray cryptoSuites = GetMediaCryptoSuites();
  bool hasClearText = cryptoSuites.GetValuesIndex(OpalMediaCryptoSuite::ClearText()) != P_MAX_INDEX;
  if (cryptoSuites.GetSize() > (hasClearText ? 1 : 0)) {
    for (sessionId = 1; sessionId <= sessionCount; ++sessionId) {
      SDPMediaDescription * incomingMedia = sdpOffer.GetMediaDescriptionByIndex(sessionId);
      if (PAssert(incomingMedia != NULL, PLogicError) && incomingMedia->IsSecure())
        sdpMediaDescriptions[sessionId] = OnSendAnswerSDPSession(incomingMedia,
                                                                 sessionId,
                                                                 transfer,
                                                                 sdpOffer.GetDirection(sessionId),
                                                                 bundleMergeInfo);
    }
  }
  if (hasClearText) {
    for (sessionId = 1; sessionId <= sessionCount; ++sessionId) {
      SDPMediaDescription * incomingMedia = sdpOffer.GetMediaDescriptionByIndex(sessionId);
      if (PAssert(incomingMedia != NULL, PLogicError) && !incomingMedia->IsSecure())
        sdpMediaDescriptions[sessionId] = OnSendAnswerSDPSession(incomingMedia,
                                                                 sessionId,
                                                                 transfer,
                                                                 sdpOffer.GetDirection(sessionId),
                                                                 bundleMergeInfo);
    }
  }
#else
  for (sessionId = 1; sessionId <= sessionCount; ++sessionId) {
    SDPMediaDescription * incomingMedia = sdpOffer.GetMediaDescriptionByIndex(sessionId);
    if (PAssert(incomingMedia != NULL, "SDP Media description list changed"))
      sdpMediaDescriptions[sessionId] = OnSendAnswerSDPSession(incomingMedia,
                                                               sessionId,
                                                               transfer,
                                                               sdpOffer.GetDirection(sessionId),
                                                               bundleMergeInfo);
  }
#endif // OPAL_SRTP

  bundleMergeInfo.RemoveSessionSSRCs(m_sessions);

  // Fill in refusal for media sessions we didn't like
  bool gotNothing = true;
  for (sessionId = 1; sessionId <= sessionCount; ++sessionId) {
    SDPMediaDescription * incomingMedia = sdpOffer.GetMediaDescriptionByIndex(sessionId);
    if (!PAssert(incomingMedia != NULL, PLogicError))
      return false;

    SDPMediaDescription * mediaDescription = sdpMediaDescriptions[sessionId];
    OpalMediaSession * mediaSession = GetMediaSession(sessionId);
    if (mediaDescription != NULL && mediaSession != NULL)
      gotNothing = false;
    else {
      if (mediaSession == NULL) {
        OpalMediaSession::Init init(*this, sessionId, incomingMedia->GetMediaType(), m_remoteBehindNAT);
        PStringArray tokens(4);
        tokens[0] = incomingMedia->GetSDPMediaType();
        tokens[1] = '0';
        tokens[2] = incomingMedia->GetSDPTransportType();
        tokens[3] = incomingMedia->GetSDPPortList();
        mediaSession = new OpalDummySession(init, tokens);
        m_sessions.SetAt(sessionId, mediaSession);
      }
      if (mediaDescription == NULL)
        mediaDescription = mediaSession->CreateSDPMediaDescription();
    }

    mediaDescription->FromSession(mediaSession, incomingMedia, 0);
    sdpOut.AddMediaDescription(mediaDescription);
  }

  if (gotNothing) {
    PTRACE(3, "Could not match any SDP media descriptions on " << *this);
    return false;
  }

  m_activeFormatList = OpalMediaFormatList(); // Don't do RemoveAll() in case of references

  /* Shut down any media that is in a session not mentioned in answer.
      While the SIP/SDP specification says this shouldn't happen, it does
      anyway so we need to deal. */
  for (StreamDict::iterator it = m_mediaStreams.begin(); it != m_mediaStreams.end(); ++it) {
    OpalMediaStreamPtr stream = it->second;
    if (stream != NULL) {
      unsigned session = stream->GetSessionID();
      if (session > sessionCount || sdpMediaDescriptions[session] == NULL)
        stream->Close();
    }
  }

  bool holdFromRemote = sdpOffer.IsHold();
  if (m_holdFromRemote != holdFromRemote) {
    PTRACE(3, "Remote " << (holdFromRemote ? "" : "retrieve from ") << "hold detected");
    m_holdFromRemote = holdFromRemote;
    OnHold(true, holdFromRemote);
  }

  StartMediaStreams();

  return true;
}


SDPMediaDescription * OpalSDPConnection::OnSendAnswerSDPSession(SDPMediaDescription * incomingMedia,
                                                                unsigned   sessionId,
                                                                bool   transfer,
                                                                SDPMediaDescription::Direction   otherSidesDir,
                                                                BundleMergeInfo & bundleMergeInfo)
{
  OpalMediaType mediaType = incomingMedia->GetMediaType();

  // See if any media formats of this session id, so don't create unused RTP session
  if (!m_activeFormatList.HasType(mediaType)) {
    PTRACE(3, "No media formats of type " << mediaType << ", not adding SDP");
    return NULL;
  }

  if (!PAssert(mediaType.GetDefinition() != NULL, PString("Unusable media type \"") + mediaType + '"'))
    return NULL;

  #if OPAL_SRTP
  OpalMediaCryptoKeyList keys = incomingMedia->GetCryptoKeys();
  if (!keys.IsEmpty() && !(GetMediaCryptoKeyExchangeModes()&OpalMediaCryptoSuite::e_SecureSignalling)) {
    PTRACE(2, "No secure signaling, cannot use SDES crypto for " << mediaType << " session " << sessionId);
    keys.RemoveAll();
    incomingMedia->SetCryptoKeys(keys);
  }

  // If not a match already, or if we already have another, secure version, of the media session
  if (GetMediaSession(sessionId) == NULL) {
    for (SessionMap::const_iterator it = m_sessions.begin(); it != m_sessions.end(); ++it) {
      if (it->second->GetSessionID() != sessionId &&
          it->second->GetMediaType() == mediaType &&
          (
            it->second->GetSessionType() == OpalSRTPSession::RTP_SAVP() ||
            it->second->GetSessionType() == OpalDTLSSRTPSession::RTP_DTLS_SAVPF()
          ) &&
          it->second->IsOpen())
      {
        PTRACE(3, "Not creating " << mediaType << " media session, already secure.");
        return NULL;
      }
    }
  }
#endif // OPAL_SRTP

  // Create new media session
  OpalTransportAddress localAddress;
  OpalMediaSession * mediaSession = SetUpMediaSession(sessionId, mediaType, *incomingMedia, localAddress, bundleMergeInfo);
  if (mediaSession == NULL)
    return NULL;

  bool replaceSession = false;

  // For fax for example, we have to switch the media session according to mediaType
  if (mediaSession->GetMediaType() != mediaType) {
    PTRACE(3, "Replacing " << mediaSession->GetMediaType() << " session " << sessionId << " with " << mediaType);
#if OPAL_T38_CAPABILITY
    if (mediaType == OpalMediaType::Fax()) {
      if (!OnSwitchingFaxMediaStreams(true)) {
        PTRACE(2, "Switch to T.38 refused for " << *this);
        return NULL;
      }
    }
    else if (mediaSession->GetMediaType() == OpalMediaType::Fax()) {
      if (!OnSwitchingFaxMediaStreams(false)) {
        PTRACE(2, "Switch from T.38 refused for " << *this);
        return NULL;
      }
    }
#endif // OPAL_T38_CAPABILITY

    mediaSession = CreateMediaSession(sessionId, mediaType, incomingMedia->GetSessionType());
    if (mediaSession == NULL) {
      PTRACE(2, "Could not create session for " << mediaType);
      return NULL;
    }

    // Set flag to force media stream close
    replaceSession = true;
  }

  // construct a new media session list 
  std::auto_ptr<SDPMediaDescription> localMedia(mediaSession->CreateSDPMediaDescription());
  if (localMedia.get() == NULL) {
    if (replaceSession)
      delete mediaSession; // Still born so can delete, not used anywhere
    PTRACE(1, "Could not create SDP media description for media type " << mediaType);
    return NULL;
  }
  PTRACE_CONTEXT_ID_TO(*localMedia);

  /* Make sure SDP transport type in preply is same as in offer. This is primarily
     a workaround for broken implementations, esecially with respect to feedback
     (AVPF) and DTLS (UDP/TLS/SAFP) */
  localMedia->SetSDPTransportType(incomingMedia->GetSDPTransportType());

  // Get SDP string options through
  localMedia->SetStringOptions(m_stringOptions);

#if OPAL_SRTP
  if (!keys.IsEmpty()) {// SDES
    OpalMediaCryptoKeyInfo * rxKey = mediaSession->IsCryptoSecured(true);
    OpalMediaCryptoKeyInfo * txKey = mediaSession->IsCryptoSecured(false);
    if (txKey != NULL && rxKey != NULL && keys.GetValuesIndex(*rxKey) != P_MAX_INDEX) {
      keys.RemoveAll();
      keys.Append(txKey->CloneAs<OpalMediaCryptoKeyInfo>());
    }
    else {
      // Set rx key from the other side SDP, which it's tx key
      if (!mediaSession->ApplyCryptoKey(keys, true)) {
        PTRACE(2, "Incompatible crypto suite(s) for " << mediaType << " session " << sessionId);
        return NULL;
      }

      // Use symmetric keys, generate a cloneof the remotes tx key for out yx key
      txKey = keys.front().CloneAs<OpalMediaCryptoKeyInfo>();
      if (PAssertNULL(txKey) == NULL)
        return NULL;

      // But with a different value
      txKey->Randomise();

      keys.RemoveAll();
      keys.Append(txKey);
      if (!mediaSession->ApplyCryptoKey(keys, false)) {
        PTRACE(2, "Unexpected error with crypto suite(s) for " << mediaType << " session " << sessionId);
        return NULL;
      }
    }
    localMedia->SetCryptoKeys(keys);
  }
#endif // OPAL_SRTP

  if (GetPhase() < ConnectedPhase) {
    // If processing initial offer and video, obey the auto-start flags
    OpalMediaType::AutoStartMode autoStart = GetAutoStart(mediaType);
    if ((autoStart&OpalMediaType::Transmit) == 0)
      otherSidesDir = (otherSidesDir&SDPMediaDescription::SendOnly) != 0 ? SDPMediaDescription::SendOnly : SDPMediaDescription::Inactive;
    if ((autoStart&OpalMediaType::Receive) == 0)
      otherSidesDir = (otherSidesDir&SDPMediaDescription::RecvOnly) != 0 ? SDPMediaDescription::RecvOnly : SDPMediaDescription::Inactive;
    PTRACE(4, "Answering initial offer for media type " << mediaType << ", directions=" << otherSidesDir << ", autoStart=" << autoStart);
  }
  else {
    PTRACE(4, "Answering offer for media type " << mediaType << ", directions=" << otherSidesDir);
  }

  SDPMediaDescription::Direction newDirection = SDPMediaDescription::Inactive;

  // Check if we had a stream and the remote has either changed the codec or
  // changed the direction of the stream
  OpalMediaStreamPtr sendStream = GetMediaStream(sessionId, false);
  if (PauseOrCloseMediaStream(sendStream, replaceSession, (otherSidesDir&SDPMediaDescription::RecvOnly) == 0))
    newDirection = SDPMediaDescription::SendOnly;

  OpalMediaStreamPtr recvStream = GetMediaStream(sessionId, true);
  if (PauseOrCloseMediaStream(recvStream, replaceSession,
                              m_holdToRemote >= eHoldOn || (otherSidesDir&SDPMediaDescription::SendOnly) == 0))
    newDirection = newDirection != SDPMediaDescription::Inactive ? SDPMediaDescription::SendRecv : SDPMediaDescription::RecvOnly;

  // See if we need to do a session switcharoo, but must be after stream closing
  if (replaceSession)
    ReplaceMediaSession(sessionId, mediaSession);

  /* After (possibly) closing streams, we now open them again if necessary,
     OpenSourceMediaStreams will just return true if they are already open.
     We open tx (other party source) side first so we follow the remote
     endpoints preferences. */
  if (!incomingMedia->GetMediaAddress().IsEmpty()) {
    PSafePtr<OpalConnection> otherParty = GetOtherPartyConnection();
    if (otherParty != NULL && sendStream == NULL) {
      if ((sendStream = GetMediaStream(sessionId, false)) == NULL) {
        PTRACE(5, "Opening tx " << mediaType << " stream from offer SDP");
        if (m_ownerCall.OpenSourceMediaStreams(*otherParty,
                                             mediaType,
                                             sessionId,
                                             OpalMediaFormat(),
#if OPAL_VIDEO
                                             incomingMedia->GetContentRole(),
#endif
                                             transfer,
                                             (otherSidesDir&SDPMediaDescription::RecvOnly) == 0))
          sendStream = GetMediaStream(sessionId, false);
      }

      if ((otherSidesDir&SDPMediaDescription::RecvOnly) != 0) {
        if (sendStream == NULL) {
          PTRACE(4, "Did not open required tx " << mediaType << " stream.");
          return NULL;
        }
        newDirection = newDirection != SDPMediaDescription::Inactive ? SDPMediaDescription::SendRecv
                                                                     : SDPMediaDescription::SendOnly;
      }
    }

    if (sendStream != NULL) {
      // In case is new offer and remote has tweaked the streams paramters, we need to merge them
      sendStream->UpdateMediaFormat(*m_activeFormatList.FindFormat(sendStream->GetMediaFormat()), true);
    }

    if (recvStream == NULL) {
      if ((recvStream = GetMediaStream(sessionId, true)) == NULL) {
        PTRACE(5, "Opening rx " << mediaType << " stream from offer SDP");
        if (m_ownerCall.OpenSourceMediaStreams(*this,
                                             mediaType,
                                             sessionId,
                                             OpalMediaFormat(),
#if OPAL_VIDEO
                                             incomingMedia->GetContentRole(),
#endif
                                             transfer,
                                             (otherSidesDir&SDPMediaDescription::SendOnly) == 0))
          recvStream = GetMediaStream(sessionId, true);
      }

      if ((otherSidesDir&SDPMediaDescription::SendOnly) != 0) {
        if (recvStream == NULL) {
          PTRACE(4, "Did not open required rx " << mediaType << " stream.");
          return NULL;
        }
        newDirection = newDirection != SDPMediaDescription::Inactive ? SDPMediaDescription::SendRecv
                                                                     : SDPMediaDescription::RecvOnly;
      }
    }

    if (recvStream != NULL) {
      OpalMediaFormat adjustedMediaFormat = *m_activeFormatList.FindFormat(recvStream->GetMediaFormat());

      // If we are sendrecv we will receive the same payload type as we transmit.
      if (newDirection == SDPMediaDescription::SendRecv)
        adjustedMediaFormat.SetPayloadType(sendStream->GetMediaFormat().GetPayloadType());

      recvStream->UpdateMediaFormat(adjustedMediaFormat, true);
    }
  }

  // Now we build the reply, setting "direction" as appropriate for what we opened.
  localMedia->SetDirection(newDirection);
  if (sendStream != NULL)
    localMedia->AddMediaFormat(sendStream->GetMediaFormat());
  else if (recvStream != NULL)
    localMedia->AddMediaFormat(recvStream->GetMediaFormat());
  else {
    // Add all possible formats
    bool empty = true;
    for (OpalMediaFormatList::iterator remoteFormat = m_remoteFormatList.begin(); remoteFormat != m_remoteFormatList.end(); ++remoteFormat) {
      if (remoteFormat->GetMediaType() == mediaType) {
        for (OpalMediaFormatList::iterator localFormat = m_localMediaFormats.begin(); localFormat != m_localMediaFormats.end(); ++localFormat) {
          if (localFormat->GetMediaType() == mediaType) {
            OpalMediaFormat intermediateFormat;
            if (OpalTranscoder::FindIntermediateFormat(*localFormat, *remoteFormat, intermediateFormat)) {
              localMedia->AddMediaFormat(*remoteFormat);
              empty = false;
              break;
            }
          }
        }
      }
    }

    // RFC3264 says we MUST have an entry, but it should have port zero
    if (empty) {
      localMedia->AddMediaFormat(m_activeFormatList.front());
      localMedia->FromSession(NULL, NULL, 0);
    }
    else {
      // We can do the media type but choose not to at this time
      localMedia->SetDirection(SDPMediaDescription::Inactive);
    }
  }

  FinaliseRtx(recvStream, localMedia.get());

  if (mediaType == OpalMediaType::Audio()) {
    // Set format if we have an RTP payload type for RFC2833 and/or NSE
    SetNxECapabilities(m_rfc2833Handler, m_localMediaFormats, m_activeFormatList, OpalRFC2833, localMedia.get());
#if OPAL_T38_CAPABILITY
    SetNxECapabilities(m_ciscoNSEHandler, m_localMediaFormats, m_activeFormatList, OpalCiscoNSE, localMedia.get());
#endif
  }

#if OPAL_T38_CAPABILITY
  m_ownerCall.ResetSwitchingT38();
#endif

#if OPAL_RTP_FEC
  OpalMediaFormatList fec = NegotiateFECMediaFormats(*mediaSession);
  for (OpalMediaFormatList::iterator it = fec.begin(); it != fec.end(); ++it)
    localMedia->AddMediaFormat(*it);
#endif

	PTRACE(4, "Answered offer for media type " << mediaType << ' ' << localMedia->GetMediaAddress());
  return localMedia.release();
}


bool OpalSDPConnection::OnReceivedAnswerSDP(const SDPSessionDescription & sdp, bool & multipleFormats)
{
  SetActiveMediaFormats(sdp.GetMediaFormats());

  unsigned mediaDescriptionCount = sdp.GetMediaDescriptions().GetSize();

  bool ok = false;
  BundleMergeInfo bundleMergeInfo(mediaDescriptionCount);
  for (unsigned index = 1; index <= mediaDescriptionCount; ++index) {
    SDPMediaDescription * mediaDescription = sdp.GetMediaDescriptionByIndex(index);
    if (PAssertNULL(mediaDescription) == NULL)
      return false;

    unsigned sessionId;
    if (!mediaDescription->IsGroupMember(OpalMediaSession::GetBundleGroupId()))
      sessionId = index;
    else {
      /* When using BUNDLE, sessionId not 1 to 1 with media description any
         more, so need to try and match it up by SDP "mid" attribute. */
      sessionId = 0;
      for (SessionMap::iterator it = m_sessions.begin(); it != m_sessions.end(); ++it) {
        if (it->second->IsGroupMember(OpalMediaSession::GetBundleGroupId()) &&
            mediaDescription->GetGroupMediaId(OpalMediaSession::GetBundleGroupId()).NumCompare(
                                it->second->GetGroupMediaId(OpalMediaSession::GetBundleGroupId())) == EqualTo) {
          sessionId = it->first;
          break;
        }
      }
      if (sessionId == 0) {
        PTRACE(3, "Could not match mid=\"" << mediaDescription->GetGroupMediaId(OpalMediaSession::GetBundleGroupId()) << "\""
                  " to any session in " << OpalMediaSession::GetBundleGroupId());
        return false;
      }
    }

    if (OnReceivedAnswerSDPSession(mediaDescription, sessionId, sdp.GetDirection(index), multipleFormats, bundleMergeInfo))
      ok = true;
    else {
      OpalMediaStreamPtr stream;
      if ((stream = GetMediaStream(sessionId, false)) != NULL)
        stream->Close();
      if ((stream = GetMediaStream(sessionId, true)) != NULL)
        stream->Close();
    }
  }

  bundleMergeInfo.RemoveSessionSSRCs(m_sessions);

  m_activeFormatList = OpalMediaFormatList(); // Don't do RemoveAll() in case of references

  /* Shut down any media that is in a session not mentioned in answer to our offer.
     While the SIP/SDP specification says this shouldn't happen, it does
     anyway so we need to deal. */
  for (StreamDict::iterator it = m_mediaStreams.begin(); it != m_mediaStreams.end(); ++it) {
    OpalMediaStreamPtr stream = it->second;
    if (stream != NULL && stream->GetSessionID() > mediaDescriptionCount)
      stream->Close();
  }

  if (ok)
    StartMediaStreams();

  return ok;
}


bool OpalSDPConnection::OnReceivedAnswerSDPSession(const SDPMediaDescription * mediaDescription,
                                                   unsigned sessionId,
                                                   SDPMediaDescription::Direction otherSidesDir,
                                                   bool & multipleFormats,
                                                   BundleMergeInfo & bundleMergeInfo)
{
  if (!PAssert(mediaDescription != NULL, "SDP Media description list changed"))
    return false;

  OpalMediaType mediaType = mediaDescription->GetMediaType();
  
  PTRACE(4, "Processing received SDP media description for " << mediaType);

  /* Get the media the remote has answered to our offer. Remove the media
     formats we do not support, in case the remote is insane and replied
     with something we did not actually offer. */
  if (!m_activeFormatList.HasType(mediaType)) {
    PTRACE(2, "Could not find supported media formats in SDP media description for session " << sessionId);
    return false;
  }

  // Set up the media session, e.g. RTP
  OpalTransportAddress localAddress;
  OpalMediaSession * mediaSession = SetUpMediaSession(sessionId, mediaType, *mediaDescription, localAddress, bundleMergeInfo);
  if (mediaSession == NULL)
    return false;

#if OPAL_SRTP
  OpalMediaCryptoKeyList keys = mediaDescription->GetCryptoKeys();
  if (!keys.IsEmpty()) {
    // Set our rx keys to remotes tx keys indicated in SDP
    if (!mediaSession->ApplyCryptoKey(keys, true)) {
      PTRACE(2, "Incompatible crypto suite(s) for " << mediaType << " session " << sessionId);
      return false;
    }

    if (!mediaSession->IsCryptoSecured(false)) {
      // Now match up the tag number on our offered keys
      OpalMediaCryptoKeyList & offeredKeys = mediaSession->GetOfferedCryptoKeys();
      OpalMediaCryptoKeyList::iterator it;
      for (it = offeredKeys.begin(); it != offeredKeys.end(); ++it) {
        if (it->GetTag() == keys.front().GetTag())
          break;
      }
      if (it == offeredKeys.end()) {
        PTRACE(2, "Remote selected crypto suite(s) we did not offer for " << mediaType << " session " << sessionId);
        return false;
      }

      keys.RemoveAll();
      keys.Append(&*it);

      offeredKeys.DisallowDeleteObjects(); // Can't have in two lists and both dispose of pointer
      offeredKeys.erase(it);
      offeredKeys.AllowDeleteObjects();
      offeredKeys.RemoveAll();

      if (!mediaSession->ApplyCryptoKey(keys, false)) {
        PTRACE(2, "Incompatible crypto suite(s) for " << mediaType << " session " << sessionId);
        return false;
      }
    }
  }
#endif // OPAL_SRTP

  // Check if we had a stream and the remote has either changed the codec or
  // changed the direction of the stream
  OpalMediaStreamPtr sendStream = GetMediaStream(sessionId, false);
  bool sendDisabled = bundleMergeInfo.m_allowPauseSendMediaStream[sessionId] && (otherSidesDir&SDPMediaDescription::RecvOnly) == 0;
  bundleMergeInfo.m_allowPauseSendMediaStream[sessionId] = sendDisabled;
  PauseOrCloseMediaStream(sendStream, false, sendDisabled);

  OpalMediaStreamPtr recvStream = GetMediaStream(sessionId, true);
  bool recvDisabled = bundleMergeInfo.m_allowPauseRecvMediaStream[sessionId] && (otherSidesDir&SDPMediaDescription::SendOnly) == 0;
  bundleMergeInfo.m_allowPauseRecvMediaStream[sessionId] = recvDisabled;
  PauseOrCloseMediaStream(recvStream, false, recvDisabled);

  /* After (possibly) closing streams, we now open them again if necessary,
     OpenSourceMediaStreams will just return true if they are already open.
     We open tx (other party source) side first so we follow the remote
     endpoints preferences. */
  if (sendStream == NULL) {
    PSafePtr<OpalConnection> otherParty = GetOtherPartyConnection();
    if (otherParty == NULL)
      return false;

    PTRACE(5, "Opening tx " << mediaType << " stream from answer SDP");
    if (m_ownerCall.OpenSourceMediaStreams(*otherParty,
                                          mediaType,
                                          sessionId,
                                          OpalMediaFormat(),
#if OPAL_VIDEO
                                          mediaDescription->GetContentRole(),
#endif
                                          false,
                                          sendDisabled))
      sendStream = GetMediaStream(sessionId, false);
    if (!sendDisabled && sendStream == NULL && !otherParty->IsOnHold(true))
      OnMediaStreamOpenFailed(false);
  }

  if (recvStream == NULL) {
    PTRACE(5, "Opening rx " << mediaType << " stream from answer SDP");
    if (m_ownerCall.OpenSourceMediaStreams(*this,
                                         mediaType,
                                         sessionId,
                                         OpalMediaFormat(),
#if OPAL_VIDEO
                                         mediaDescription->GetContentRole(),
#endif
                                         false,
                                         recvDisabled))
      recvStream = GetMediaStream(sessionId, true);
    if (!recvDisabled && recvStream == NULL)
      OnMediaStreamOpenFailed(true);
  }

  FinaliseRtx(sendStream, NULL);

  PINDEX maxFormats = 1;
  if (mediaType == OpalMediaType::Audio()) {
    if (SetNxECapabilities(m_rfc2833Handler, m_localMediaFormats, m_activeFormatList, OpalRFC2833))
      ++maxFormats;
#if OPAL_T38_CAPABILITY
    if (SetNxECapabilities(m_ciscoNSEHandler, m_localMediaFormats, m_activeFormatList, OpalCiscoNSE))
      ++maxFormats;
#endif
  }

  if (mediaDescription->GetSDPMediaFormats().GetSize() > maxFormats)
    multipleFormats = true;

#if OPAL_RTP_FEC
  NegotiateFECMediaFormats(*mediaSession);
#endif

	PTRACE_IF(3, otherSidesDir == SDPMediaDescription::Inactive, "No streams opened as " << mediaType << " inactive");
  return true;
}


void OpalSDPConnection::FinaliseRtx(const OpalMediaStreamPtr & stream, SDPMediaDescription * sdp)
{
  if (stream == NULL)
    return;

  OpalRTPSession * rtpSession = dynamic_cast<OpalRTPSession *>(GetMediaSession(stream->GetSessionID()));
  if (rtpSession == NULL)
    return;

  // Make sure rtx has correct PT
  RTP_DataFrame::PayloadTypes primaryPT = stream->GetMediaFormat().GetPayloadType();
  RTP_DataFrame::PayloadTypes newPT = RTP_DataFrame::IllegalPayloadType;
  PString rtxName = OpalRtx::GetName(rtpSession->GetMediaType());
  OpalMediaFormatList remoteFormats = GetMediaFormats();
  for (OpalMediaFormatList::iterator it = remoteFormats.begin(); it != remoteFormats.end(); ++it) {
    if (it->GetName() == rtxName && it->GetOptionPayloadType(OpalRtx::AssociatedPayloadTypeOption()) == primaryPT) {
      newPT = it->GetPayloadType();
      if (sdp != NULL)
        sdp->AddMediaFormat(*it);
      break;
    }
  }

  if (newPT == RTP_DataFrame::IllegalPayloadType) {
    PTRACE(4, "No RTX present for stream " << *stream);
    return;
  }

  PTRACE(4, "Finalising RTX as " << newPT << " for primary " << primaryPT << " on stream " << *stream);

  OpalRTPSession::Direction dir = stream->IsSource() ? OpalRTPSession::e_Receiver : OpalRTPSession::e_Sender;

  // Adjust the session SSRCs
  RTP_SyncSourceArray ssrcs = rtpSession->GetSyncSources(dir);
  for (RTP_SyncSourceArray::iterator it = ssrcs.begin(); it != ssrcs.end(); ++it) {
    RTP_SyncSourceId primarySSRC = *it;
    RTP_SyncSourceId rtxSSRC = rtpSession->GetRtxSyncSource(primarySSRC, dir, true);
    if (rtxSSRC != 0)
      rtpSession->EnableSyncSourceRtx(primarySSRC, dir == OpalRTPSession::e_Receiver ? primaryPT : newPT, rtxSSRC);
  }
}


bool OpalSDPConnection::OnHoldStateChanged(bool)
{
  return true;
}


void OpalSDPConnection::OnMediaStreamOpenFailed(bool)
{
}

#endif // OPAL_SDP


///////////////////////////////////////////////////////////////////////////////
