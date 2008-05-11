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
 * $Revision: 19424 $
 * $Author: csoutheren $
 * $Date: 2008-02-08 17:24:10 +1100 (Fri, 08 Feb 2008) $
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
#include <h224/h224handler.h>

#if OPAL_VIDEO
#include <codec/vidcodec.h>
#endif

#ifdef HAS_LIBZRTP

#define BUILD_ZRTP_MUTEXES 1

#ifdef _WIN32
#define ZRTP_PLATFORM ZP_WIN32
#endif

#ifdef P_LINUX
#define ZRTP_PLATFORM ZP_LINUX
#endif

#include <zrtp.h>
#include <zrtp/opalzrtp.h>
#include <rtp/zrtpudp.h>
#endif


OpalRTPConnection::OpalRTPConnection(OpalCall & call,
                             OpalRTPEndPoint  & ep,
                                const PString & token,
                                   unsigned int options,
               OpalConnection::StringOptions * _stringOptions)
  : OpalConnection(call, ep, token, options, _stringOptions)
  , securityData(NULL)
  , remoteIsNAT(false)
{
  rfc2833Handler  = new OpalRFC2833Proto(*this, PCREATE_NOTIFIER(OnUserInputInlineRFC2833));
  ciscoNSEHandler = new OpalRFC2833Proto(*this, PCREATE_NOTIFIER(OnUserInputInlineCiscoNSE));

  securityMode = ep.GetDefaultSecurityMode();

#if OPAL_RTP_AGGREGATE
  switch (options & RTPAggregationMask) {
    case RTPAggregationDisable:
      useRTPAggregation = PFalse;
      break;
    case RTPAggregationEnable:
      useRTPAggregation = PTrue;
      break;
    default:
      useRTPAggregation = ep.UseRTPAggregation();
  }
#endif

  // if this is the second connection in this call, then we are making an outgoing H.323/SIP call
  // so, get the autoStart info from the other connection
  PSafePtr<OpalConnection> conn  = call.GetConnection(0);
  if (conn != NULL) 
    m_rtpSessions.Initialise(*this, conn->GetStringOptions());
  else
    m_rtpSessions.Initialise(*this, stringOptions);
}

OpalRTPConnection::~OpalRTPConnection()
{
  delete rfc2833Handler;
  delete ciscoNSEHandler;
}


RTP_Session * OpalRTPConnection::GetSession(unsigned sessionID) const
{
  return m_rtpSessions.GetSession(sessionID);
}


RTP_Session * OpalRTPConnection::UseSession(const OpalTransport & transport, unsigned sessionID, const OpalMediaType & mediaType, RTP_QOS * rtpqos)
{
  RTP_Session * rtpSession = m_rtpSessions.GetSession(sessionID);
  if (rtpSession == NULL) {
    rtpSession = CreateSession(transport, sessionID, rtpqos);
    m_rtpSessions.AddSession(rtpSession, mediaType);
  }

  return rtpSession;
}


RTP_Session * OpalRTPConnection::CreateSession(const OpalTransport & transport,
                                                            unsigned sessionID,
                                                           RTP_QOS * rtpqos)
{
  // We only support RTP over UDP at this point in time ...
  if (!transport.IsCompatibleTransport("ip$127.0.0.1")) 
    return NULL;

  PIPSocket::Address localAddress;
 
  transport.GetLocalAddress().GetIpAddress(localAddress);

  OpalManager & manager = GetEndPoint().GetManager();

  PIPSocket::Address remoteAddress;
  transport.GetRemoteAddress().GetIpAddress(remoteAddress);
  PSTUNClient * stun = manager.GetSTUN(remoteAddress);

  OpalMediaType mediaType = OpalMediaTypeDefinition::GetMediaTypeForSessionId(sessionID);
  OpalMediaTypeDefinition * def = mediaType.GetDefinition();
  if (def == NULL) {
    PTRACE(1, "OpalCon\tNo definition for media type " << mediaType);
    return NULL;
  }

  // create an RTP session
  RTP_UDP          * rtpSession    = NULL;
  OpalSecurityMode * securityParms = NULL;

  if (!securityMode.IsEmpty()) {
    securityParms = PFactory<OpalSecurityMode>::CreateInstance(securityMode);
    if (securityParms == NULL) {
      PTRACE(1, "OpalCon\tSecurity mode " << securityMode << " unknown");
      return NULL;
    }
    PTRACE(1, "OpalCon\tCreating security mode " << securityMode);
  }

  rtpSession = def->CreateRTPSession(*this,
#if OPAL_RTP_AGGREGATE
                             useRTPAggregation ? endpoint.GetRTPAggregator() : NULL, 
#endif
                             securityParms, sessionID, remoteIsNAT);  
  if (rtpSession == NULL) {
    if (securityParms == NULL) {
      PTRACE(1, "OpalCon\tCannot create RTP session " << sessionID);
    } else {
      PTRACE(1, "OpalCon\tCannot create RTP session " << sessionID << " with security mode " << securityMode);
      delete securityParms;
    }
    return NULL;
  }

  WORD firstPort = manager.GetRtpIpPortPair();
  WORD nextPort = firstPort;
  while (!rtpSession->Open(localAddress, nextPort, nextPort, manager.GetRtpIpTypeofService(), stun, rtpqos)) {
    nextPort = manager.GetRtpIpPortPair();
    if (nextPort == firstPort) {
      PTRACE(1, "OpalCon\tNo ports available for RTP session " << sessionID << " for " << *this);
      delete rtpSession;
      return NULL;
    }
  }

  localAddress = rtpSession->GetLocalAddress();
  if (manager.TranslateIPAddress(localAddress, remoteAddress)){
    rtpSession->SetLocalAddress(localAddress);
  }
  
  return rtpSession;
}

void OpalRTPConnection::ReleaseSession(unsigned sessionID, PBoolean clearAll/* = PFalse */)
{
#ifdef HAS_LIBZRTP
  //check is security mode ZRTP
  if (0 == securityMode.Find("ZRTP")) {
    RTP_Session *session = GetSession(sessionID);
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

  m_rtpSessions.ReleaseSession(sessionID, clearAll);
}


PBoolean OpalRTPConnection::IsRTPNATEnabled(const PIPSocket::Address & localAddr, 
                                            const PIPSocket::Address & peerAddr,
                                            const PIPSocket::Address & sigAddr,
                                                              PBoolean incoming)
{
  return static_cast<OpalRTPEndPoint &>(endpoint).IsRTPNATEnabled(*this, localAddr, peerAddr, sigAddr, incoming);
}

void * OpalRTPConnection::GetSecurityData()
{
  return securityData;
}
 
void OpalRTPConnection::SetSecurityData(void *data)
{
  securityData = data;
}

#if OPAL_VIDEO
void OpalRTPConnection::OnMediaCommand(OpalMediaCommand & command, INT /*extra*/)
{
  if (PIsDescendant(&command, OpalVideoUpdatePicture)) {
    RTP_Session * session = m_rtpSessions.GetSession(OpalMediaFormat::DefaultVideoSessionID);
    if (session != NULL)
      session->SendIntraFrameRequest();
#ifdef OPAL_STATISTICS
    m_VideoUpdateRequestsSent++;
#endif
  }
}
#else
void OpalRTPConnection::OnMediaCommand(OpalMediaCommand & /*command*/, INT /*extra*/)
{
}
#endif

void OpalRTPConnection::AttachRFC2833HandlerToPatch(PBoolean isSource, OpalMediaPatch & patch)
{
  if (rfc2833Handler != NULL) {
    if(isSource) {
      PTRACE(3, "OpalCon\tAdding RFC2833 receive handler");
      OpalMediaStream & mediaStream = patch.GetSource();
      patch.AddFilter(rfc2833Handler->GetReceiveHandler(), mediaStream.GetMediaFormat());
    } 
  }

  if (ciscoNSEHandler != NULL) {
    if(isSource) {
      PTRACE(3, "OpalCon\tAdding Cisco NSE receive handler");
      OpalMediaStream & mediaStream = patch.GetSource();
      patch.AddFilter(ciscoNSEHandler->GetReceiveHandler(), mediaStream.GetMediaFormat());
    } 
  }
}

PBoolean OpalRTPConnection::SendUserInputTone(char tone, unsigned duration)
{
  if (duration == 0)
    duration = 180;

  return rfc2833Handler->SendToneAsync(tone, duration);
}

PBoolean OpalRTPConnection::GetMediaInformation(unsigned sessionID,
                                         MediaInformation & info) const
{
  if (!mediaTransportAddresses.Contains(sessionID)) {
    PTRACE(2, "OpalCon\tGetMediaInformation for session " << sessionID << " - no channel.");
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
  PTRACE(3, "OpalCon\tGetMediaInformation for session " << sessionID
         << " data=" << info.data << " rfc2833=" << info.rfc2833);
  return PTrue;
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
  if (ownerCall.IsMediaBypassPossible(*this, sessionID))
    return new OpalNullMediaStream(*this, mediaFormat, sessionID, isSource);

  RTP_Session * session = GetSession(sessionID);
  if (session == NULL) {
    PTRACE(1, "H323\tCreateMediaStream could not find session " << sessionID);
    return NULL;
  }

  return new OpalRTPMediaStream(*this, mediaFormat, isSource, *session,
                                GetMinAudioJitterDelay(),
                                GetMaxAudioJitterDelay());
}

void OpalRTPConnection::OnPatchMediaStream(PBoolean isSource, OpalMediaPatch & patch)
{
  OpalConnection::OnPatchMediaStream(isSource, patch);
  if (patch.GetSource().GetMediaFormat().GetMediaType() == OpalMediaType::Audio()) {
    AttachRFC2833HandlerToPatch(isSource, patch);
#if P_DTMF
    if (detectInBandDTMF && isSource) {
      patch.AddFilter(PCREATE_NOTIFIER(OnUserInputInBandDTMF), OPAL_PCM16);
    }
#endif
  }

  patch.SetCommandNotifier(PCREATE_NOTIFIER(OnMediaCommand), !isSource);
}

void OpalRTPConnection::OnUserInputInlineRFC2833(OpalRFC2833Info & info, INT type)
{
  // trigger on end of tone
  if (type == 0)
    OnUserInputTone(info.GetTone(), info.GetDuration()/8);
}

void OpalRTPConnection::OnUserInputInlineCiscoNSE(OpalRFC2833Info & /*info*/, INT)
{
  cout << "Received NSE event" << endl;
  //if (!info.IsToneStart())
  //  OnUserInputTone(info.GetTone(), info.GetDuration()/8);
}

/////////////////////////////////////////////////////////////////////////////

OpalMediaSession::OpalMediaSession(const OpalMediaType & _mediaType)
  : mediaType(_mediaType)
  , autoStartReceive(true)
  , autoStartTransmit(true)
  , sessionId(0)
  , rtpSession(NULL)
{
}


OpalMediaSession::OpalMediaSession(const OpalMediaSession & _obj)
  : mediaType(_obj.mediaType)
  , autoStartReceive(_obj.autoStartReceive)
  , autoStartTransmit(_obj.autoStartTransmit)
  , sessionId(_obj.sessionId)
  , rtpSession(_obj.rtpSession)
{
}

/////////////////////////////////////////////////////////////////////////////

OpalRTPSessionManager::OpalRTPSessionManager()
{
  m_initialised = false;
  m_cleanupOnDelete = true;
}

OpalRTPSessionManager::~OpalRTPSessionManager()
{
  if (m_cleanupOnDelete) {
    while (sessions.GetSize() > 0) {
      RTP_Session * session = sessions.GetDataAt(0).rtpSession;
      if (session != NULL) {
        PTRACE(3, "RTP\tDeleting session " << session->GetSessionID());
        session->Close(PTrue);
        session->SetJitterBufferSize(0, 0);
        delete session;
      }
      sessions.RemoveAt(sessions.GetKeyAt(0));
    }
  }
}

void OpalRTPSessionManager::CopyFromMaster(const OpalRTPSessionManager & from)
{
  PWaitAndSignal m1(m_mutex);
  PWaitAndSignal m2(from.m_mutex);

  PAssert(sessions.GetSize() == 0, "Cannot copy from master RTP session list to non-empty list");

  for (PINDEX i = 0; i < sessions.GetSize(); ++i) {
    PAssert(sessions.GetDataAt(i).rtpSession == NULL, "Cannot copy from master RTP session list after sockets have been created");
  }

  sessions = from.sessions;
  sessions.MakeUnique();
}

void OpalRTPSessionManager::CopyToMaster(OpalRTPSessionManager & from)
{
  PWaitAndSignal m1(m_mutex);
  PWaitAndSignal m2(from.m_mutex);

  PAssert(from.sessions.GetSize() != 0, "Cannot copy from empty list to master RTP session list");

  for (PINDEX i = 0; i < sessions.GetSize(); ++i) {
    if (sessions.GetDataAt(i).rtpSession != NULL) {
      PTRACE(2, "RTP\tCannot copy to master RTP session list after sockets have been created");
      return;
    }
  }

  sessions = from.sessions;
  from.m_cleanupOnDelete = false;
}


void OpalRTPSessionManager::AddSession(RTP_Session * rtpSession, const OpalMediaType & mediaType)
{
  PWaitAndSignal m(m_mutex);
  
  if (rtpSession != NULL) {
    OpalMediaSession * session = sessions.GetAt(rtpSession->GetSessionID());
    if (session == NULL) {
      OpalMediaSession * s = new OpalMediaSession(mediaType);
      s->rtpSession = rtpSession;
      sessions.Insert(POrdinalKey(rtpSession->GetSessionID()), s);
      PTRACE(3, "RTP\tCreating new session " << *rtpSession);
    }
    else
    {
      PAssert(session->rtpSession == NULL, "Cannot add already existing session");
      session->rtpSession = rtpSession;
    }
  }
}


void OpalRTPSessionManager::ReleaseSession(unsigned sessionID, PBoolean /*clearAll*/)
{
  PTRACE(3, "RTP\tReleasing session " << sessionID);
}


RTP_Session * OpalRTPSessionManager::GetSession(unsigned sessionID) const
{
  PWaitAndSignal wait(m_mutex);

  OpalMediaSession * session;
  if (((session = sessions.GetAt(sessionID)) == NULL) || (session->rtpSession == NULL)) {
    PTRACE(3, "RTP\tCannot find session " << sessionID);
    return NULL;
  }

  PTRACE(3, "RTP\tFound existing session " << sessionID);
  return session->rtpSession;
}


void OpalRTPSessionManager::Initialise(OpalRTPConnection & conn, OpalConnection::StringOptions * stringOptions)
{
  PWaitAndSignal m(m_mutex);

  // make function idempotent
  if (m_initialised)
    return;

  m_initialised = true;

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
        OpalMediaSession info(mediaType);
        bool autoStartReceive  = true;
        bool autoStartTransmit = true;
        if (colon != P_MAX_INDEX) {
          PStringArray tokens = line.Mid(colon+1).Tokenise(";", FALSE);
          PINDEX j;
          for (j = 0; j < tokens.GetSize(); ++j) {
            if (tokens[i] *= "no") {
              autoStartReceive  = false;
              autoStartTransmit = false;
            }
          }
        }
        AutoStartSession(def->GetDefaultSessionId(), mediaType, autoStartReceive, autoStartTransmit);
      }
    }
  }

  // set old video and audio auto start if not already set
#if OPAL_AUDIO
  SetOldOptions(1, OpalMediaType::Audio(), true, true);
#endif
#if OPAL_VIDEO
  OpalManager & mgr = conn.GetCall().GetManager();
  SetOldOptions(2, OpalMediaType::Video(), mgr.CanAutoStartReceiveVideo(), mgr.CanAutoStartTransmitVideo());
#endif
}

void OpalRTPSessionManager::SetOldOptions(unsigned preferredSessionIndex, const OpalMediaType & mediaType, bool rx, bool tx)
{
  PINDEX i;
  for (i = 0; i < sessions.GetSize(); ++i) {
    if (sessions.GetDataAt(i).mediaType == mediaType)
      break;
  }
  if (i == sessions.GetSize()) 
    AutoStartSession(preferredSessionIndex, mediaType, rx, tx);
}


unsigned OpalRTPSessionManager::AutoStartSession(unsigned sessionID, const OpalMediaType & mediaType, bool autoStartReceive, bool autoStartTransmit)
{
  PWaitAndSignal m(m_mutex);
  m_initialised = true;

  if ((sessionID == 0) || (sessions.Contains(sessionID))) {
    unsigned i = 1;
    while (sessions.Contains(i))
      ++i;
    sessionID = i;
  }

  OpalMediaSession * s = new OpalMediaSession(mediaType);
  s->sessionId         = sessionID;
  s->autoStartReceive  = autoStartReceive;
  s->autoStartTransmit = autoStartTransmit;
  sessions.Insert(POrdinalKey(sessionID), s);

  return sessionID;
}

/////////////////////////////////////////////////////////////////////////////

