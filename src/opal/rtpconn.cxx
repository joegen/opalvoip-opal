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

#ifdef __GNUC__
#pragma implementation "connection.h"
#endif

#include <opal/buildopts.h>

#include <opal/rtpconn.h>
#include <opal/rtpep.h>
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
#if OPAL_H224
  , h224Handler(NULL)
#endif
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

#if 0
  // if this is the second connection in this call, then we are making an outgoing H.323/SIP call
  // so, get the autoStart info from the other connection
  PSafePtr<OpalConnection> conn  = call.GetConnection(0);
  if (conn != NULL) 
    channelInfoMap.Initialise(*this, conn->GetStringOptions());
  else
    channelInfoMap.Initialise(*this, stringOptions);
#endif
}

OpalRTPConnection::~OpalRTPConnection()
{
  delete rfc2833Handler;
  delete ciscoNSEHandler;
#if OPAL_H224
  delete h224Handler;
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
      ) {
    return NULL;
  }

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

  rtpSessions.ReleaseSession(sessionID, clearAll);
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
    RTP_Session * session = rtpSessions.GetSession(OpalMediaFormat::DefaultVideoSessionID);
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

#if OPAL_H224

OpalH224Handler * OpalRTPConnection::CreateH224ProtocolHandler(unsigned sessionID)
{
  if(h224Handler == NULL)
    h224Handler = endpoint.CreateH224ProtocolHandler(*this, sessionID);
	
  return h224Handler;
}

OpalH281Handler * OpalRTPConnection::CreateH281ProtocolHandler(OpalH224Handler & h224Handler)
{
  return endpoint.CreateH281ProtocolHandler(h224Handler);
}

#endif

/////////////////////////////////////////////////////////////////////////////

RTP_SessionManager::RTP_SessionManager()
{
}


RTP_SessionManager::RTP_SessionManager(const RTP_SessionManager & sm)
: sessions(sm.sessions)
{
}


RTP_SessionManager & RTP_SessionManager::operator=(const RTP_SessionManager & sm)
{
  PWaitAndSignal m1(mutex);
  PWaitAndSignal m2(sm.mutex);
  sessions = sm.sessions;
  return *this;
}


RTP_Session * RTP_SessionManager::UseSession(unsigned sessionID)
{
  PWaitAndSignal m(mutex);

  RTP_Session * session = sessions.GetAt(sessionID);
  if (session == NULL) 
    return NULL; 
  
  PTRACE(3, "RTP\tFound existing session " << sessionID);
  session->IncrementReference();

  return session;
}


void RTP_SessionManager::AddSession(RTP_Session * session)
{
  PWaitAndSignal m(mutex);
  
  if (session != NULL) {
    PTRACE(3, "RTP\tAdding session " << *session);
    sessions.SetAt(session->GetSessionID(), session);
  }
}


void RTP_SessionManager::ReleaseSession(unsigned sessionID,
                                        PBoolean clearAll)
{
  PTRACE(3, "RTP\tReleasing session " << sessionID);

  mutex.Wait();

  while (sessions.Contains(sessionID)) {
    RTP_Session * session = &sessions[sessionID];
    if (session->DecrementReference()) {
      PTRACE(3, "RTP\tDeleting session " << sessionID);
      session->Close(PTrue);
      session->SetJitterBufferSize(0, 0);
      sessions.SetAt(sessionID, NULL);
    }
    if (!clearAll)
      break;
  }

  mutex.Signal();
}


RTP_Session * RTP_SessionManager::GetSession(unsigned sessionID) const
{
  PWaitAndSignal wait(mutex);
  if (!sessions.Contains(sessionID))
    return NULL;

  PTRACE(3, "RTP\tFound existing session " << sessionID);
  return &sessions[sessionID];
}

/////////////////////////////////////////////////////////////////////////////

