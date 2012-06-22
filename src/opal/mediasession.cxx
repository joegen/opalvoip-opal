/*
 * mediasession.cxx
 *
 * Media session abstraction
 *
 * Open Phone Abstraction Library (OPAL)
 *
 * Copyright (C) 2010 Vox Lucida Pty. Ltd.
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
 * The Initial Developer of the Original Code is Vox Lucida Pty. Ltd.
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#include <ptlib.h>

#ifdef P_USE_PRAGMA
#pragma implementation "mediasession.h"
#endif

#include <opal/buildopts.h>

#include <opal/mediasession.h>
#include <opal/connection.h>


#define new PNEW


///////////////////////////////////////////////////////////////////////////////

#if OPAL_STATISTICS

OpalMediaStatistics::OpalMediaStatistics()
  : m_startTime(0)
  , m_totalBytes(0)
  , m_totalPackets(0)
  , m_packetsLost(0)
  , m_packetsOutOfOrder(0)
  , m_packetsTooLate(0)
  , m_packetOverruns(0)
  , m_minimumPacketTime(0)
  , m_averagePacketTime(0)
  , m_maximumPacketTime(0)

    // Audio
  , m_averageJitter(0)
  , m_maximumJitter(0)

    // Video
  , m_totalFrames(0)
  , m_keyFrames(0)
{
}

#if OPAL_FAX
OpalMediaStatistics::Fax::Fax()
  : m_result(OpalMediaStatistics::FaxNotStarted)
  , m_phase(' ')
  , m_bitRate(9600)
  , m_compression(FaxCompressionUnknown)
  , m_errorCorrection(false)
  , m_txPages(-1)
  , m_rxPages(-1)
  , m_totalPages(0)
  , m_imageSize(0)
  , m_resolutionX(0)
  , m_resolutionY(0)
  , m_pageWidth(0)
  , m_pageHeight(0)
  , m_badRows(0)
  , m_mostBadRows(0)
  , m_errorCorrectionRetries(0)
{
}

ostream & operator<<(ostream & strm, OpalMediaStatistics::FaxCompression compression)
{
  static const char * const Names[] = { "N/A", "T.4 1d", "T.4 2d", "T.6" };
  if (compression >= 0 && compression < PARRAYSIZE(Names))
    strm << Names[compression];
  else
    strm << (unsigned)compression;
  return strm;
}
#endif

#endif

/////////////////////////////////////////////////////////////////////////////

OpalMediaSession::OpalMediaSession(OpalConnection & connection, unsigned sessionId, const OpalMediaType & mediaType)
  : m_connection(connection)
  , m_sessionId(sessionId)
  , m_mediaType(mediaType)
  , m_isExternalTransport(false)
  , m_referenceCount(1)
{
  PTRACE_CONTEXT_ID_FROM(connection);
}

bool OpalMediaSession::Open(const PString &)
{
  return true;
}


bool OpalMediaSession::Close()
{
  return true;
}


bool OpalMediaSession::SetRemoteMediaAddress(const OpalTransportAddress &)
{
  return true;
}


OpalTransportAddress OpalMediaSession::GetRemoteControlAddress() const
{
  return OpalTransportAddress();
}


bool OpalMediaSession::SetRemoteControlAddress(const OpalTransportAddress &)
{
  return true;
}


void OpalMediaSession::AttachTransport(Transport &)
{
}


OpalMediaSession::Transport OpalMediaSession::DetachTransport()
{
  return Transport();
}


void OpalMediaSession::SetExternalTransport(const OpalTransportAddressArray & PTRACE_PARAM(transports))
{
  PTRACE(3, "Media\tSetting external transport to " << setfill(',') << transports);
  m_isExternalTransport = true;
}


#if OPAL_STATISTICS
void OpalMediaSession::GetStatistics(OpalMediaStatistics &, bool) const
{
}
#endif

/////////////////////////////////////////////////////////////////////////////

