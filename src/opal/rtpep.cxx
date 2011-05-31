/*
 * rtpep.cxx
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
#pragma implementation "rtpep.h"
#endif

#include <opal/buildopts.h>

#include <opal/rtpep.h>
#include <opal/rtpconn.h>


OpalRTPEndPoint::OpalRTPEndPoint(OpalManager & manager,     ///<  Manager of all endpoints.
                       const PCaselessString & prefix,      ///<  Prefix for URL style address strings
                                      unsigned attributes)  ///<  Bit mask of attributes endpoint has
  : OpalEndPoint(manager, prefix, attributes)
#ifdef OPAL_ZRTP
    , zrtpEnabled(manager.GetZRTPEnabled())
#endif
{
}

OpalRTPEndPoint::~OpalRTPEndPoint()
{
}


PBoolean OpalRTPEndPoint::IsRTPNATEnabled(OpalConnection & conn, 
                                const PIPSocket::Address & localAddr, 
                                const PIPSocket::Address & peerAddr,
                                const PIPSocket::Address & sigAddr,
                                                PBoolean   incoming)
{
  return GetManager().IsRTPNATEnabled(conn, localAddr, peerAddr, sigAddr, incoming);
}


OpalMediaFormatList OpalRTPEndPoint::GetMediaFormats() const
{
  return manager.GetCommonMediaFormats(true, false);
}


#ifdef OPAL_ZRTP

bool OpalRTPEndPoint::GetZRTPEnabled() const
{ 
  return zrtpEnabled; 
}

#endif


static RTP_UDP * GetRTPFromStream(const OpalMediaStream & stream)
{
  const OpalRTPMediaStream * rtpStream = dynamic_cast<const OpalRTPMediaStream *>(&stream);
  if (rtpStream == NULL)
    return NULL;

  return dynamic_cast<RTP_UDP *>(&rtpStream->GetRtpSession());
}


void OpalRTPEndPoint::OnClosedMediaStream(const OpalMediaStream & stream)
{
  CheckEndLocalRTP(stream.GetConnection(), GetRTPFromStream(stream));

  OpalEndPoint::OnClosedMediaStream(stream);
}


bool OpalRTPEndPoint::OnLocalRTP(OpalConnection & connection1,
                                 OpalConnection & connection2,
                                 unsigned         sessionID,
                                 bool             opened) const
{
  return manager.OnLocalRTP(connection1, connection2, sessionID, opened);
}


bool OpalRTPEndPoint::CheckForLocalRTP(const OpalRTPMediaStream & stream)
{
  PWaitAndSignal mutex(inUseFlag);

  RTP_UDP * rtp = GetRTPFromStream(stream);
  if (rtp == NULL)
    return false;

  OpalConnection & connection = stream.GetConnection();

  if (!PIPSocket::IsLocalHost(rtp->GetRemoteAddress())) {
    PTRACE(5, "RTPEp\tSession " << stream.GetSessionID() << ", "
              "remote RTP address " << rtp->GetRemoteAddress() << " not local.");
    CheckEndLocalRTP(connection, rtp);
    return false;
  }

  WORD localPort = rtp->GetLocalDataPort();
  LocalRtpInfoMap::iterator itLocal = m_connectionsByRtpLocalPort.find(localPort);
  if (!PAssert(itLocal != m_connectionsByRtpLocalPort.end(), PLogicError))
    return false;

  WORD remotePort = rtp->GetRemoteDataPort();
  LocalRtpInfoMap::iterator itRemote = m_connectionsByRtpLocalPort.find(remotePort);
  if (itRemote == m_connectionsByRtpLocalPort.end()) {
    PTRACE(4, "RTPEp\tSession " << stream.GetSessionID() << ", "
              "remote RTP port " << remotePort << " not this process.");
    return false;
  }

  bool result;
  bool cached = itRemote->second.m_previousResult >= 0;
  if (cached)
    result = itRemote->second.m_previousResult != 0;
  else {
    result = OnLocalRTP(connection, itRemote->second.m_connection, rtp->GetSessionID(), true);
    itLocal->second.m_previousResult = result;
    itRemote->second.m_previousResult = result;
  }

  PTRACE(3, "RTPEp\tSession " << stream.GetSessionID() << ", "
            "RTP ports " << localPort << " and " << remotePort
         << ' ' << (cached ? "cached" : "flagged") << " as "
         << (result ? "bypassed" : "normal"));

  return result;
}


void OpalRTPEndPoint::CheckEndLocalRTP(OpalConnection & connection, RTP_UDP * rtp)
{
  if (rtp == NULL)
    return;

  PWaitAndSignal mutex(inUseFlag);

  LocalRtpInfoMap::iterator it = m_connectionsByRtpLocalPort.find(rtp->GetLocalDataPort());
  if (it == m_connectionsByRtpLocalPort.end() || it->second.m_previousResult < 0)
    return;

  PTRACE(5, "RTPEp\tSession " << rtp->GetSessionID() << ", "
            "local RTP port " << it->first << " cache cleared.");
  it->second.m_previousResult = -1;

  it = m_connectionsByRtpLocalPort.find(rtp->GetRemoteDataPort());
  if (it == m_connectionsByRtpLocalPort.end() || it->second.m_previousResult < 0)
    return;

  PTRACE(5, "RTPEp\tSession " << rtp->GetSessionID() << ", "
            "remote RTP port " << it->first << " is local, ending bypass.");
  it->second.m_previousResult = -1;
  OnLocalRTP(connection, it->second.m_connection, rtp->GetSessionID(), false);
}


void OpalRTPEndPoint::SetConnectionByRtpLocalPort(RTP_Session * session, OpalConnection * connection)
{
  RTP_UDP * rtp = dynamic_cast<RTP_UDP *>(session);
  if (rtp == NULL)
    return;

  WORD localPort = rtp->GetLocalDataPort();
  inUseFlag.Wait();
  if (connection == NULL) {
    LocalRtpInfoMap::iterator it = m_connectionsByRtpLocalPort.find(localPort);
    if (it != m_connectionsByRtpLocalPort.end()) {
      PTRACE(4, "RTPEp\tSession " << rtp->GetSessionID() << ", "
                "forgetting local RTP port " << localPort << " on connection " << it->second.m_connection);
      m_connectionsByRtpLocalPort.erase(it);
    }
  }
  else {
    std::pair<LocalRtpInfoMap::iterator, bool> insertResult =
                m_connectionsByRtpLocalPort.insert(LocalRtpInfoMap::value_type(localPort, *connection));
    PTRACE_IF(4, insertResult.second, "RTPEp\tSession " << rtp->GetSessionID() << ", "
              "remembering local RTP port " << localPort << " on connection " << *connection);
  }
  inUseFlag.Signal();
}

