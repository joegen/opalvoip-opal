/*
 * pcapfile.cxx
 *
 * Ethernet capture (PCAP) file declaration
 *
 * Portable Tools Library
 *
 * Copyright (C) 2011 Vox Lucida Pty. Ltd.
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
 * The Original Code is Portable Tools Library.
 *
 * The Initial Developer of the Original Code is Vox Lucida
 *
 * All Rights Reserved.
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "pcapfile.h"
#endif

#include <rtp/pcapfile.h>



void Reverse(char * ptr, size_t sz)
{
  char * top = ptr+sz-1;
  while (ptr < top) {
    char t = *ptr;
    *ptr = *top;
    *top = t;
    ptr++;
    top--;
  }
}

#define REVERSE(p) Reverse((char *)&p, sizeof(p))


///////////////////////////////////////////////////////////////////////////////

OpalPCAPFile::OpalPCAPFile()
{
  OpalMediaFormatList list = OpalMediaFormat::GetAllRegisteredMediaFormats();
  for (PINDEX i = 0; i < list.GetSize(); i++) {
    if (list[i].GetPayloadType() < RTP_DataFrame::DynamicBase)
      m_payloadType2mediaFormat[list[i].GetPayloadType()] = list[i];
  }
}


bool OpalPCAPFile::InternalOpen(OpenMode mode, OpenOptions opts, PFileInfo::Permissions permissions)
{
  PAssert(mode != PFile::ReadWrite, PInvalidParameter);

  if (!PFile::InternalOpen(mode, opts, permissions))
    return false;

  if (mode == PFile::WriteOnly) {
    m_fileHeader.magic_number = 0xa1b2c3d4;
    m_fileHeader.version_major = 2;
    m_fileHeader.version_minor = 4;
    m_fileHeader.thiszone = PTime().GetTimeZone();
    m_fileHeader.sigfigs = 0;
    m_fileHeader.snaplen = 65535;
    m_fileHeader.network = 1;
    if (Write(&m_fileHeader, sizeof(m_fileHeader)))
      return true;
    PTRACE(1, "PCAPFile\tCould not write header to \"" << GetFilePath() << '"');
    return false;
  }

  if (!Read(&m_fileHeader, sizeof(m_fileHeader))) {
    PTRACE(1, "PCAPFile\tCould not read header from \"" << GetFilePath() << '"');
    return false;
  }

  if (m_fileHeader.magic_number == 0xa1b2c3d4)
    m_rawPacket.m_otherEndian = false;
  else if (m_fileHeader.magic_number == 0xd4c3b2a1)
    m_rawPacket.m_otherEndian = true;
  else {
    PTRACE(1, "PCAPFile\tFile \"" << GetFilePath() << "\" is not a PCAP file, bad magic number.");
    return false;
  }

  if (m_rawPacket.m_otherEndian) {
    REVERSE(m_fileHeader.version_major);
    REVERSE(m_fileHeader.version_minor);
    REVERSE(m_fileHeader.thiszone);
    REVERSE(m_fileHeader.sigfigs);
    REVERSE(m_fileHeader.snaplen);
    REVERSE(m_fileHeader.network);
  }

  return true;
}


bool OpalPCAPFile::Restart()
{
  if (SetPosition(sizeof(m_fileHeader)))
    return true;

  PTRACE(2, "PCAPFile\tCould not seek beginning of \"" << GetFilePath() << '"');
  return false;
}


void OpalPCAPFile::PrintOn(ostream & strm) const
{
  strm << "PCAP v" << m_fileHeader.version_major << '.' << m_fileHeader.version_minor
                   << " file \"" << GetFilePath() << '"';
}


bool OpalPCAPFile::WriteFrame(const PEthSocket::Frame & frame)
{
  RecordHeader header;
  header.ts_sec  = (uint32_t)frame.GetTimestamp().GetTimeInSeconds();
  header.ts_usec = frame.GetTimestamp().GetMicrosecond();
  header.incl_len = header.orig_len = frame.GetSize();
  PWaitAndSignal mutex(m_writeMutex);
  return Write(&header, sizeof(header)) && frame.Write(*this);
}


bool OpalPCAPFile::WriteRTP(const RTP_DataFrame & rtp, WORD port)
{
  PEthSocket::Frame packet;
  memcpy(packet.CreateUDP(PIPSocketAddressAndPort(GetFilterSrcIP(), port),
                          PIPSocketAddressAndPort(GetFilterDstIP(), port),
                          rtp.GetPacketSize()),
         rtp, rtp.GetPacketSize());
  return WriteFrame(packet);
}


bool OpalPCAPFile::Frame::Read(PChannel & channel, PINDEX)
{
  PreRead();

  RecordHeader recordHeader;
  if (!channel.Read(&recordHeader, sizeof(recordHeader))) {
    PTRACE(1, "PCAPFile\tTruncated file \"" << channel.GetName() << '"');
    return false;
  }

  if (m_otherEndian) {
    REVERSE(recordHeader.ts_sec);
    REVERSE(recordHeader.ts_usec);
    REVERSE(recordHeader.incl_len);
    REVERSE(recordHeader.orig_len);
  }

  m_timestamp.SetTimestamp(recordHeader.ts_sec, recordHeader.ts_usec);

  if (!channel.Read(m_rawData.GetPointer(recordHeader.incl_len), recordHeader.incl_len)) {
    PTRACE(1, "PCAPFile\tTruncated file \"" << channel.GetName() << '"');
    return false;
  }

  m_rawSize = recordHeader.incl_len;
  return true;
}


int OpalPCAPFile::GetDataLink(PBYTEArray & payload)
{
  return m_rawPacket.Read(*this) ? m_rawPacket.GetDataLink(payload) : -1;
}


int OpalPCAPFile::GetIP(PBYTEArray & payload)
{
  if (!m_rawPacket.Read(*this))
    return -1;

  PIPSocket::Address src, dst;
  int type = m_rawPacket.GetIP(payload, src, dst);
  if (type < 0)
    return -1;

  m_packetSrc.SetAddress(src);
  m_packetDst.SetAddress(dst);

  return (m_filterSrc.GetAddress().IsValid() && m_filterSrc.GetAddress() != src) ||
         (m_filterDst.GetAddress().IsValid() || m_filterDst.GetAddress() == dst) ? -1 : type;
}


int OpalPCAPFile::GetTCP(PBYTEArray & payload)
{
  return m_rawPacket.Read(*this) &&
         m_rawPacket.GetTCP(payload, m_packetSrc, m_packetDst) &&
         m_packetSrc.MatchWildcard(m_filterSrc) &&
         m_packetDst.MatchWildcard(m_filterDst)
         ? payload.GetSize() : -1;
}


int OpalPCAPFile::GetUDP(PBYTEArray & payload)
{
  return m_rawPacket.Read(*this) &&
         m_rawPacket.GetUDP(payload, m_packetSrc, m_packetDst) &&
         m_packetSrc.MatchWildcard(m_filterSrc) &&
         m_packetDst.MatchWildcard(m_filterDst)
         ? payload.GetSize() : -1;
}


int OpalPCAPFile::GetRTP(RTP_DataFrame & rtp)
{
  int packetLength = GetUDP(rtp);
  if (packetLength < 0)
    return -1;

  if (!rtp.SetPacketSize(packetLength))
    return -1;

  if (rtp.GetVersion() != 2)
    return -1;

  RTP_DataFrame::PayloadTypes pt = rtp.GetPayloadType();
  if (pt >= RTP_DataFrame::StartConflictRTCP && pt <= RTP_DataFrame::EndConflictRTCP)
    return -1;

  return pt;
}


static const PConstString Unknown("Unknown");

OpalPCAPFile::DiscoveredRTPInfo::DirInfo::DirInfo()
  : m_found(false)
  , m_ssrc(0)
  , m_seq(0)
  , m_ts(0)
  , m_ssrc_matches(0)
  , m_seq_matches(0)
  , m_ts_matches(0)
  , m_type(Unknown)
  , m_format(Unknown)
  , m_index(0)
{
}


void OpalPCAPFile::DiscoveredRTPMap::PrintOn(ostream & strm) const
{
  // display matches
  const_iterator iter;
  for (iter = begin(); iter != end(); ++iter) {
    const DiscoveredRTPInfo & info = iter->second;
    for (int dir = 0; dir < 2; ++dir) {
      if (info.m_direction[dir].m_found) {
        if (info.m_direction[dir].m_payload != info.m_direction[dir].m_firstFrames.front().GetPayloadType())
          strm << "Mismatched payload types" << endl;
        strm << info.m_direction[dir].m_index << " : " << info.m_direction[dir].m_addr
                                  << " -> " << info.m_direction[1-dir].m_addr
                                  << ", " << info.m_direction[dir].m_payload
                                  << " " << info.m_direction[dir].m_type
                                  << " " << info.m_direction[dir].m_format << endl;
      }
    }
  }
}


bool OpalPCAPFile::DiscoverRTP(DiscoveredRTPMap & discoveredRTPMap)
{
  if (!Restart())
    return false;

  unsigned packetCount = 0;
  while (!IsEndOfFile()) {
    ++packetCount;

    RTP_DataFrame rtp;
    if (GetRTP(rtp) < 0)
      continue;

    // determine if reverse or forward session
    bool dir = GetSrcIP() >  GetDstIP() ||
              (GetSrcIP() == GetDstIP() && GetSrcPort() > GetDstPort()) ? 1 : 0;

    ostringstream keyStrm;
    if (dir == 0)
      keyStrm << GetSrcIP() << ':' << GetSrcPort() << '|' << GetDstIP() << ':' << GetDstPort();
    else
      keyStrm << GetDstIP() << ':' << GetDstPort() << '|' << GetSrcIP() << ':' << GetSrcPort();
    string key = keyStrm.str();

    // see if we have identified this potential session before
    DiscoveredRTPMap::iterator iter;
    if ((iter = discoveredRTPMap.find(key)) == discoveredRTPMap.end()) {
      iter = discoveredRTPMap.insert(make_pair(key, DiscoveredRTPInfo())).first;
      DiscoveredRTPInfo::DirInfo & info = iter->second.m_direction[dir];

      info.m_addr.SetAddress(GetSrcIP());
      info.m_addr.SetPort(GetSrcPort());
      iter->second.m_direction[1-dir].m_addr.SetAddress(GetDstIP());
      iter->second.m_direction[1-dir].m_addr.SetPort(GetDstPort());

      info.m_payload  = rtp.GetPayloadType();
      info.m_seq      = rtp.GetSequenceNumber();
      info.m_ts       = rtp.GetTimestamp();

      info.m_ssrc     = rtp.GetSyncSource();
      info.m_seq      = rtp.GetSequenceNumber();
      info.m_ts       = rtp.GetTimestamp();

      info.m_found    = true;
    }
    else if (iter->second.m_direction[dir].m_found) {
      DiscoveredRTPInfo::DirInfo & info = iter->second.m_direction[dir];

      WORD seq = rtp.GetSequenceNumber();
      DWORD ts = rtp.GetTimestamp();
      DWORD ssrc = rtp.GetSyncSource();
      if (info.m_ssrc == ssrc)
        ++info.m_ssrc_matches;
      if ((info.m_seq+1) == seq)
        ++info.m_seq_matches;
      info.m_seq = seq;
      if ((info.m_ts+1) < ts)
        ++info.m_ts_matches;
      info.m_ts = ts;
    }
    else {
      DiscoveredRTPInfo::DirInfo & info = iter->second.m_direction[dir];
      info.m_addr.SetAddress(GetSrcIP());
      info.m_addr.SetPort(GetSrcPort());
      iter->second.m_direction[1-dir].m_addr.SetAddress(GetDstIP());
      iter->second.m_direction[1-dir].m_addr.SetPort(GetDstPort());

      info.m_payload  = rtp.GetPayloadType();
      info.m_seq      = rtp.GetSequenceNumber();
      info.m_ts       = rtp.GetTimestamp();

      info.m_ssrc     = rtp.GetSyncSource();
      info.m_seq      = rtp.GetSequenceNumber();
      info.m_ts       = rtp.GetTimestamp();

      info.m_found    = true;
    }

    if (iter->second.m_direction[dir].m_firstFrames.size() < 100) {
      RTP_DataFrame * newRTP = new RTP_DataFrame(rtp);
      newRTP->MakeUnique();
      iter->second.m_direction[dir].m_firstFrames.Append(newRTP);
    }
  }

  if (!Restart())
    return false;

  OpalMediaFormatList formats = OpalMediaFormat::GetAllRegisteredMediaFormats();

#if OPAL_VIDEO
  struct
  {
    OpalVideoFormat m_format;
    PBYTEArray      m_context;
  } VideoCodecs[] = {
    { OPAL_H263 },
    { OPAL_H263plus },
    { OPAL_MPEG4 },
    { OPAL_H264 },
    { OPAL_VP8 },
  };
#endif

  size_t index = 1;
  DiscoveredRTPMap::iterator iter = discoveredRTPMap.begin();
  while (iter != discoveredRTPMap.end()) {
    DiscoveredRTPInfo & info = iter->second;
    if (
          (
            info.m_direction[0].m_found
            &&
            (
              info.m_direction[0].m_seq_matches > 5 ||
              info.m_direction[0].m_ts_matches > 5 ||
              info.m_direction[0].m_ssrc_matches > 5
            )
          ) 
          ||
          (
            info.m_direction[1].m_found
            &&
            (
              info.m_direction[1].m_seq_matches > 5 ||
              info.m_direction[1].m_ts_matches > 5 ||
              info.m_direction[1].m_ssrc_matches > 5
            )
          )
        )
    {
      for (int dir = 0; dir < 2; ++dir) {
        DiscoveredRTPInfo::DirInfo & dirinfo = info.m_direction[dir];
        if (!dirinfo.m_firstFrames.IsEmpty()) {
          dirinfo.m_index = index++;

          RTP_DataFrame::PayloadTypes pt = dirinfo.m_firstFrames.front().GetPayloadType();

          // look for known audio types
          if (pt <= RTP_DataFrame::Cisco_CN) {
            OpalMediaFormatList::const_iterator it = formats.FindFormat(pt, OpalMediaFormat::AudioClockRate);
            if (it != formats.end()) {
              dirinfo.m_type   = it->GetMediaType();
              dirinfo.m_format = it->GetName();
            }
          }

#if OPAL_VIDEO
          // look for known video types
          else if (pt <= RTP_DataFrame::LastKnownPayloadType) {
            OpalMediaFormatList::const_iterator r;
            if ((r = formats.FindFormat(pt, OpalMediaFormat::VideoClockRate)) != formats.end()) {
              dirinfo.m_type   = r->GetMediaType();
              dirinfo.m_format = r->GetName();
            }
          }
          else {
            // try and identify media by inspection
            for (PINDEX i = 0; i < PARRAYSIZE(VideoCodecs); ++i) {
              RTP_DataFrameList::iterator rtp;
              for (rtp = dirinfo.m_firstFrames.begin(); rtp != dirinfo.m_firstFrames.end(); ++rtp) {
                if (VideoCodecs[i].m_format.GetVideoFrameType(rtp->GetPayloadPtr(), rtp->GetPayloadSize(), VideoCodecs[i].m_context) == OpalVideoFormat::e_IntraFrame)
                  break;
              }
              if (rtp != dirinfo.m_firstFrames.end()) {
                dirinfo.m_type = OpalMediaType::Video();
                dirinfo.m_format = VideoCodecs[i].m_format.GetName();
                break;
              }
            }
          }
#endif // OPAL_VIDEO
        }
      }
      ++iter;
    }
    else
      discoveredRTPMap.erase(iter++);
  }

  return true;
}


bool OpalPCAPFile::SetFilters(const DiscoveredRTPInfo & info, int dir, const PString & format)
{
  if (!SetPayloadMap(info.m_direction[dir].m_payload, format.IsEmpty() ? info.m_direction[dir].m_format : format))
    return false;

  m_filterSrc = info.m_direction[dir].m_addr;
  m_filterDst = info.m_direction[1-dir].m_addr;
  return true;
}


bool OpalPCAPFile::SetFilters(const DiscoveredRTPMap & discoveredRTPMap, size_t index, const PString & format)
{
  for (DiscoveredRTPMap::const_iterator iter = discoveredRTPMap.begin();
                                        iter != discoveredRTPMap.end(); ++iter) {
    const OpalPCAPFile::DiscoveredRTPInfo & info = iter->second;
    if (info.m_direction[0].m_index == index)
      return SetFilters(info, 0, format);
    if (info.m_direction[1].m_index == index)
      return SetFilters(info, 1, format);
  }

  return false;
}


bool OpalPCAPFile::SetPayloadMap(RTP_DataFrame::PayloadTypes pt, const OpalMediaFormat & format)
{
  if (!format.IsTransportable())
    return false;

  m_payloadType2mediaFormat[pt] = format;
  m_payloadType2mediaFormat[pt].SetPayloadType(pt);
  return true;
}


OpalMediaFormat OpalPCAPFile::GetMediaFormat(const RTP_DataFrame & rtp) const
{
  std::map<RTP_DataFrame::PayloadTypes, OpalMediaFormat>::const_iterator iter =
                                          m_payloadType2mediaFormat.find(rtp.GetPayloadType());
  return iter != m_payloadType2mediaFormat.end() ? iter->second : OpalMediaFormat();
}


// End Of File ///////////////////////////////////////////////////////////////
