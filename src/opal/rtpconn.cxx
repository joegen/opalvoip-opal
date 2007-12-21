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
 * $Revision: 19146 $
 * $Author: csoutheren $
 * $Date: 2007-12-21 11:57:11 +1100 (Fri, 21 Dec 2007) $
 */

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "connection.h"
#endif

#include <opal/buildopts.h>

#include <opal/rtpconn.h>
#include <opal/endpoint.h>
#include <codec/rfc2833.h>
#include <t38/t38proto.h>
#include <opal/patch.h>


OpalRTPConnection::OpalRTPConnection(OpalCall & call,
                                OpalEndPoint  & ep,
                                const PString & token,
                                   unsigned int options,
               OpalConnection::StringOptions * _stringOptions)
  : OpalConnection(call, ep, token, options, _stringOptions)
{
  rfc2833Handler  = new OpalRFC2833Proto(*this, PCREATE_NOTIFIER(OnUserInputInlineRFC2833));
#if OPAL_T38FAX
  ciscoNSEHandler = new OpalRFC2833Proto(*this, PCREATE_NOTIFIER(OnUserInputInlineCiscoNSE));
#endif

  // if this is the second connection in this call, then we are making an outgoing H.323/SIP call
  // so, get the autoStart info from the other connection
  PSafePtr<OpalConnection> conn  = call.GetConnection(0);
  if (conn != NULL) 
    channelStartInfoMap.Initialise(*this, conn->GetStringOptions());
  else
    channelStartInfoMap.Initialise(*this, stringOptions);
}

OpalRTPConnection::~OpalRTPConnection()
{
  delete rfc2833Handler;
#if OPAL_T38FAX
  delete ciscoNSEHandler;
#endif
}

RTP_Session * OpalRTPConnection::GetSession(unsigned sessionID) const
{
  return rtpSessions.GetSession(sessionID);
}

RTP_Session * OpalRTPConnection::UseSession(unsigned sessionID)
{
  return rtpSessions.UseSession(sessionID);
}

RTP_Session * OpalRTPConnection::UseSession(const OpalTransport & transport,
                                         unsigned sessionID,
                                         RTP_QOS * rtpqos)
{
  RTP_Session * rtpSession = rtpSessions.UseSession(sessionID);
  if (rtpSession == NULL) {
    rtpSession = CreateSession(transport, sessionID, rtpqos);
    rtpSessions.AddSession(rtpSession);
  }

  return rtpSession;
}


void OpalRTPConnection::ReleaseSession(unsigned sessionID,
                                    PBoolean clearAll)
{
  rtpSessions.ReleaseSession(sessionID, clearAll);
}


RTP_Session * OpalRTPConnection::CreateSession(const OpalTransport & transport,
                                            unsigned sessionID,
                                            RTP_QOS * rtpqos)
{
  // We only support RTP over UDP at this point in time ...
  if (!transport.IsCompatibleTransport("ip$127.0.0.1"))
    return NULL;

  // We support video, audio and T38 over IP
  if (sessionID != OpalMediaFormat::DefaultAudioSessionID && 
      sessionID != OpalMediaFormat::DefaultVideoSessionID 
#if OPAL_T38FAX
      && sessionID != OpalMediaFormat::DefaultDataSessionID
#endif
      && sessionID != 5
      )
    return NULL;

  PIPSocket::Address localAddress;
  transport.GetLocalAddress().GetIpAddress(localAddress);

  OpalManager & manager = GetEndPoint().GetManager();

  PIPSocket::Address remoteAddress;
  transport.GetRemoteAddress().GetIpAddress(remoteAddress);
  PSTUNClient * stun = manager.GetSTUN(remoteAddress);

  // create an (S)RTP session or T38 pseudo-session as appropriate
  RTP_UDP * rtpSession = NULL;

#if OPAL_T38FAX
  if (sessionID == OpalMediaFormat::DefaultDataSessionID) {
    rtpSession = new T38PseudoRTP(NULL, sessionID, remoteIsNAT);
  }
  else
#endif

  if (!securityMode.IsEmpty()) {
    OpalSecurityMode * parms = PFactory<OpalSecurityMode>::CreateInstance(securityMode);
    if (parms == NULL) {
      PTRACE(1, "RTPCon\tSecurity mode " << securityMode << " unknown");
      return NULL;
    }
    rtpSession = parms->CreateRTPSession(
                  useRTPAggregation ? endpoint.GetRTPAggregator() : NULL, 
                  sessionID, remoteIsNAT);
    if (rtpSession == NULL) {
      PTRACE(1, "RTPCon\tCannot create RTP session for security mode " << securityMode);
      delete parms;
      return NULL;
    }
  }
  else
  {
    rtpSession = new RTP_UDP(
                   useRTPAggregation ? endpoint.GetRTPAggregator() : NULL, 
                   sessionID, remoteIsNAT);
  }

  WORD firstPort = manager.GetRtpIpPortPair();
  WORD nextPort = firstPort;
  while (!rtpSession->Open(localAddress,
                           nextPort, nextPort,
                           manager.GetRtpIpTypeofService(),
                           stun,
                           rtpqos)) {
    nextPort = manager.GetRtpIpPortPair();
    if (nextPort == firstPort) {
      PTRACE(1, "RTPCon\tNo ports available for RTP session " << sessionID << " for " << *this);
      delete rtpSession;
      return NULL;
    }
  }

  localAddress = rtpSession->GetLocalAddress();
  if (manager.TranslateIPAddress(localAddress, remoteAddress))
    rtpSession->SetLocalAddress(localAddress);
  return rtpSession;
}

void OpalRTPConnection::AttachRFC2833HandlerToPatch(PBoolean isSource, OpalMediaPatch & patch)
{
  if (rfc2833Handler != NULL) {
    if(isSource) {
      PTRACE(3, "RTPCon\tAdding RFC2833 receive handler");
      OpalMediaStream & mediaStream = patch.GetSource();
      patch.AddFilter(rfc2833Handler->GetReceiveHandler(), mediaStream.GetMediaFormat());
    } 
  }

#if OPAL_T38FAX
  if (ciscoNSEHandler != NULL) {
    if(isSource) {
      PTRACE(3, "RTPCon\tAdding Cisco NSE receive handler");
      OpalMediaStream & mediaStream = patch.GetSource();
      patch.AddFilter(ciscoNSEHandler->GetReceiveHandler(), mediaStream.GetMediaFormat());
    } 
  }
#endif
}


PBoolean OpalRTPConnection::GetMediaInformation(unsigned sessionID,
                                         MediaInformation & info) const
{
  if (!mediaTransportAddresses.Contains(sessionID)) {
    PTRACE(2, "RTPCon\tGetMediaInformation for session " << sessionID << " - no channel.");
    return PFalse;
  }

  OpalTransportAddress & address = mediaTransportAddresses[sessionID];

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

PBoolean OpalRTPConnection::SendUserInputTone(char tone, unsigned duration)
{
  if (duration == 0)
    duration = 180;

  return rfc2833Handler->SendToneAsync(tone, duration);
}

PBoolean OpalRTPConnection::IsRTPNATEnabled(const PIPSocket::Address & localAddr, 
                                            const PIPSocket::Address & peerAddr,
                                            const PIPSocket::Address & sigAddr,
                                                              PBoolean incoming)
{
  return endpoint.IsRTPNATEnabled(*this, localAddr, peerAddr, sigAddr, incoming);
}

PBoolean OpalRTPConnection::IsMediaBypassPossible(unsigned sessionID) const
{
  return sessionID == OpalMediaFormat::DefaultAudioSessionID ||
         sessionID == OpalMediaFormat::DefaultVideoSessionID;
}



OpalMediaStream * OpalRTPConnection::CreateMediaStream(const OpalMediaFormat & mediaFormat,
                                                    unsigned sessionID,
                                                    PBoolean isSource)
{
  // Use a NULL stream if media is bypassing us, 
  if (ownerCall.IsMediaBypassPossible(*this, sessionID)) {
    PTRACE(3, "RTPCon\tBypassing media for session " << sessionID);
    return new OpalNullMediaStream(*this, mediaFormat, sessionID, isSource);
  }

  // if no RTP sessions matching this session ID, then nothing to do
  RTP_Session * session = GetSession(sessionID);
  if (session == NULL) {
    PTRACE(1, "RTPCon\tCreateMediaStream could not find session " << sessionID);
    return NULL;
  }

  return new OpalRTPMediaStream(*this, mediaFormat, isSource, *session,
                                GetMinAudioJitterDelay(),
                                GetMaxAudioJitterDelay());
}


void OpalRTPConnection::OnPatchMediaStream(PBoolean isSource, OpalMediaPatch & patch)
{
  OpalConnection::OnPatchMediaStream(isSource, patch);
  if (patch.GetSource().GetSessionID() == OpalMediaFormat::DefaultAudioSessionID) {
    AttachRFC2833HandlerToPatch(isSource, patch);
#if P_DTMF
    if (detectInBandDTMF && isSource) {
      patch.AddFilter(PCREATE_NOTIFIER(OnUserInputInBandDTMF), OPAL_PCM16);
    }
#endif
  }
}

bool OpalRTPConnection::CanAutoStartMedia(const OpalMediaType & mediaType, bool rx)
{
  return channelStartInfoMap.CanAutoStartMedia(mediaType, rx);
}


/////////////////////////////////////////////////////////////////////////////

OpalRTPConnection::ChannelStartInfo::ChannelStartInfo(const OpalMediaType & _mediaType)
  : mediaType(_mediaType)
{
  autoStartReceive = autoStartTransmit = false;
  assigned                     = false;
  protocolSpecificSessionId    = 0;
  channelId                    = 0;
}

/////////////////////////////////////////////////////////////////////////////

OpalRTPConnection::ChannelStartInfoMap::ChannelStartInfoMap()
{
  initialised = false;
}

unsigned OpalRTPConnection::ChannelStartInfoMap::AddChannel(OpalRTPConnection::ChannelStartInfo & info)
{
  PWaitAndSignal m(mutex);
  initialised = true;

  if ((info.channelId == 0) || (find(info.channelId) != end())) {
    unsigned i = 10;
    while (find(info.channelId) != end())
      ++i;
    info.channelId = i;
  }

  insert(value_type(info.channelId, info));

  return info.channelId;
}

bool OpalRTPConnection::ChannelStartInfoMap::CanAutoStartMedia(const OpalMediaType & mediaType, bool rx)
{
  PWaitAndSignal m(mutex);

  iterator r;
  for (r = begin(); r != end(); ++r) {
    if (r->second.mediaType == mediaType) {
      if (rx)
        return r->second.autoStartReceive;
      else
        return r->second.autoStartTransmit;
    }
  }

  return false;
}

OpalRTPConnection::ChannelStartInfo * OpalRTPConnection::ChannelStartInfoMap::AssignAndLockChannel(const OpalMediaType & mediaType, bool assigned)
{
  mutex.Wait();

  iterator r;
  for (r = begin(); r != end(); ++r) {
    ChannelStartInfo & info = r->second;
    if ((info.assigned == assigned) && (info.mediaType == mediaType)) 
      return &r->second;
  }

  mutex.Signal();
  return NULL;
}


void OpalRTPConnection::ChannelStartInfoMap::Initialise(OpalRTPConnection & conn, OpalConnection::StringOptions * stringOptions)
{
  PWaitAndSignal m(mutex);

  // make function idempotent
  if (initialised)
    return;
  initialised = true;

  // see if stringoptions contains AutoStart option
  if (stringOptions != NULL && stringOptions->Contains("autostart")) {

    // get autostart option as lines
    PStringArray lines = (*stringOptions)("autostart").Lines();
    PINDEX i;
    for (i = 0; i < lines.GetSize(); ++i) {
      PString line = lines[i];
      PINDEX colon = line.Find(':');
      OpalMediaType mediaType = line.Left(colon);

      // see if media type is known, and if it is, enable it
      OpalMediaTypeDefinition * def = mediaType.GetDefinition();
      if (def != NULL) {
        ChannelStartInfo info(mediaType);
        info.autoStartReceive  = true;
        info.autoStartTransmit = true;
        info.channelId         = def->GetPreferredSessionId();
        if (colon != P_MAX_INDEX) {
          PStringArray tokens = line.Mid(colon+1).Tokenise(";", FALSE);
          PINDEX j;
          for (j = 0; j < tokens.GetSize(); ++j) {
            if (tokens[i] *= "no") {
              info.autoStartReceive  = false;
              info.autoStartTransmit = false;
            }
          }
        }
        AddChannel(info);
      }
    }
  }

  // set old video and audio auto start if not already set
  OpalManager & mgr = conn.GetCall().GetManager();
#if OPAL_VIDEO
  SetOldOptions(1, OpalMediaType::Audio(), true,                                true);
#endif
#if OPAL_VIDEO
  SetOldOptions(2, OpalMediaType::Video(), mgr.CanAutoStartReceiveVideo(), mgr.CanAutoStartTransmitVideo());
#endif
}


void OpalRTPConnection::ChannelStartInfoMap::SetOldOptions(unsigned channelId, const OpalMediaType & mediaType, bool rx, bool tx)
{
  iterator r;
  for (r = begin(); r != end(); ++r) {
    if ((r->second.mediaType == mediaType) && (r->second.channelId == channelId)) 
      break;
  }
  if (r == end()) {
    ChannelStartInfo info(mediaType);
    info.channelId         = channelId;
    info.autoStartReceive  = rx;
    info.autoStartTransmit = tx;
    AddChannel(info);
  }
}


/////////////////////////////////////////////////////////////////////////////
