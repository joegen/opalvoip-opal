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
  std::pair<LocalRtpInfoMap::iterator, bool> insertResult =
              m_connectionsByRtpLocalPort.insert(LocalRtpInfoMap::value_type(localPort, connection));
  PTRACE_IF(4, insertResult.second, "RTPEp\tSession " << stream.GetSessionID() << ", "
            "remembering local RTP port " << localPort << " on connection " << connection);

  WORD remotePort = rtp->GetRemoteDataPort();
  LocalRtpInfoMap::iterator it = m_connectionsByRtpLocalPort.find(remotePort);
  if (it == m_connectionsByRtpLocalPort.end()) {
    PTRACE(5, "RTPEp\tSession " << stream.GetSessionID() << ", "
              "remote RTP port " << remotePort << " not peviously remembered, searching.");

    /* Not already cached so search all RTP connections for if it is there
       To avoid deadlocks take a snapshot of the active list at an instant
       of time and iterate over that. The other connection MUST already be
       there or it isn't a hairpinned call. */
    ConnectionDict snapshot(connectionsActive);
    for (PSafePtr<OpalRTPConnection> connection(snapshot, PSafeReadOnly); connection != NULL; ++connection) {
      if (connection->FindSessionByLocalPort(remotePort) != NULL) {
        PTRACE(4, "RTPEp\tSession " << stream.GetSessionID() << ", "
                  "remembering remote RTP port " << remotePort << " on connection " << *connection);
        it = m_connectionsByRtpLocalPort.insert(LocalRtpInfoMap::value_type(remotePort, *connection)).first;
        break;
      }
    }
    if (it == m_connectionsByRtpLocalPort.end()) {
      PTRACE(4, "RTPEp\tSession " << stream.GetSessionID() << ", "
                "remote RTP port " << remotePort << " not this process.");
      return false;
    }
  }
  else {
    PTRACE(5, "RTPEp\tSession " << stream.GetSessionID() << ", "
              "remote RTP port " << remotePort << " already remembered.");
  }

  bool notCached = it->second.m_previousResult < 0;
  if (notCached) {
    it->second.m_previousResult = OnLocalRTP(connection, it->second.m_connection, rtp->GetSessionID(), true);
    if (insertResult.second)
      insertResult.first->second.m_previousResult = it->second.m_previousResult;
  }

  PTRACE(3, "RTPEp\tSession " << stream.GetSessionID() << ", "
            "RTP ports " << localPort << " and " << remotePort
         << ' ' << (notCached ? "flagged" : "cached") << " as "
         << (it->second.m_previousResult != 0 ? "bypassed" : "normal"));

  return it->second.m_previousResult != 0;
}


void OpalRTPEndPoint::CheckEndLocalRTP(OpalConnection & connection, RTP_UDP * rtp)
{
  if (rtp == NULL)
    return;

  LocalRtpInfoMap::iterator it = m_connectionsByRtpLocalPort.find(rtp->GetLocalDataPort());
  if (it == m_connectionsByRtpLocalPort.end())
    return;

  PTRACE(5, "RTPEp\tSession " << rtp->GetSessionID() << ", "
            "local RTP port " << it->first << " removed.");
  m_connectionsByRtpLocalPort.erase(it);

  it = m_connectionsByRtpLocalPort.find(rtp->GetRemoteDataPort());
  if (it == m_connectionsByRtpLocalPort.end())
    return;

  PTRACE(5, "RTPEp\tSession " << rtp->GetSessionID() << ", "
            "remote RTP port " << it->first << " is local, ending bypass.");
  it->second.m_previousResult = -1;
  OnLocalRTP(connection, it->second.m_connection, rtp->GetSessionID(), false);
}
