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



template <typename T> const T & Get(const PBYTEArray & p, PINDEX off)
{
  return *(const T *)(((const BYTE *)p)+off);
}


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
  : m_otherEndian(false)
  , m_filterSrcIP(PIPSocket::GetInvalidAddress())
  , m_filterDstIP(PIPSocket::GetInvalidAddress())
  , m_fragmentated(false)
  , m_fragmentProto(0)
  , m_filterSrcPort(0)
  , m_filterDstPort(0)
  , m_packetSrcPort(0)
  , m_packetDstPort(0)
{
  OpalMediaFormatList list = OpalMediaFormat::GetAllRegisteredMediaFormats();
  for (PINDEX i = 0; i < list.GetSize(); i++) {
    if (list[i].GetPayloadType() < RTP_DataFrame::DynamicBase)
      m_payloadType2mediaFormat[list[i].GetPayloadType()] = list[i];
  }
}


bool OpalPCAPFile::Open(const PFilePath & filename)
{
  if (!PFile::Open(filename, PFile::ReadOnly))
    return false;

  if (!Read(&m_fileHeader, sizeof(m_fileHeader))) {
    PTRACE(1, "PCAPFile\tCould not read header from \"" << filename << '"');
    return false;
  }

  if (m_fileHeader.magic_number == 0xa1b2c3d4)
    m_otherEndian = false;
  else if (m_fileHeader.magic_number == 0xd4c3b2a1)
    m_otherEndian = true;
  else {
    PTRACE(1, "PCAPFile\tFile \"" << filename << "\" is not a PCAP file, bad magic number.");
    return false;
  }

  if (m_otherEndian) {
    REVERSE(m_fileHeader.version_major);
    REVERSE(m_fileHeader.version_minor);
    REVERSE(m_fileHeader.thiszone);
    REVERSE(m_fileHeader.sigfigs);
    REVERSE(m_fileHeader.snaplen);
    REVERSE(m_fileHeader.network);
  }

  if (GetNetworkLayerHeaderSize() == 0) {
    PTRACE(1, "PCAPFile\tUnsupported Data Link Layer " << m_fileHeader.network << " in file \"" << filename << '"');
    return false;
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


bool OpalPCAPFile::ReadRawPacket(PBYTEArray & payload)
{
  if (m_fragmentated) {
    m_fragments.SetSize(0);
    m_fragmentated = false;
  }

  RecordHeader recordHeader;
  if (!Read(&recordHeader, sizeof(recordHeader))) {
    PTRACE(1, "PCAPFile\tTruncated file \"" << GetFilePath() << '"');
    return false;
  }

  if (m_otherEndian) {
    REVERSE(recordHeader.ts_sec);
    REVERSE(recordHeader.ts_usec);
    REVERSE(recordHeader.incl_len);
    REVERSE(recordHeader.orig_len);
  }

  m_packetTime.SetTimestamp(recordHeader.ts_sec, recordHeader.ts_usec);

  if (!Read(m_rawPacket.GetPointer(recordHeader.incl_len), recordHeader.incl_len)) {
    PTRACE(1, "PCAPFile\tTruncated file \"" << GetFilePath() << '"');
    return false;
  }

  payload.Attach(m_rawPacket, recordHeader.incl_len);
  return true;
}


PINDEX OpalPCAPFile::GetNetworkLayerHeaderSize()
{
  switch (m_fileHeader.network) {
    case 1 : // DLT_EN10MB - Ethernet (10Mb)
      return 14;

    case 113 : // DLT_LINUX_SLL - Linux cooked sockets
      return 16;
  }

  return 0;
}


int OpalPCAPFile::GetDataLink(PBYTEArray & payload)
{
  PBYTEArray dataLink;
  if (!ReadRawPacket(dataLink))
    return -1;

  PINDEX headerLength = GetNetworkLayerHeaderSize();
  payload.Attach(&dataLink[headerLength], dataLink.GetSize()-headerLength);
  return Get<PUInt16b>(dataLink, headerLength-2); // Next protocol layer
}


int OpalPCAPFile::GetIP(PBYTEArray & payload)
{
  PBYTEArray ip;
  if (GetDataLink(ip) != 0x800) // IPv4
    return -1;

  PINDEX headerLength = (ip[0]&0xf)*4; // low 4 bits in DWORDS, is this in bytes
  payload.Attach(&ip[headerLength], ip.GetSize()-headerLength);

  m_packetSrcIP = PIPSocket::Address(4, ip+12);
  if (m_filterSrcIP.IsValid() && m_filterSrcIP != m_packetSrcIP)
    return -1;

  m_packetDstIP = PIPSocket::Address(4, ip+16);
  if (m_filterDstIP.IsValid() && m_filterDstIP != m_packetDstIP)
    return -1;

  // Check for fragmentation
  bool isFragment = (ip[6] & 0x20) != 0;
  int fragmentOffset = (((ip[6]&0x1f)<<8)+ip[7])*8;
  PINDEX fragmentsSize = m_fragments.GetSize();
  if (!isFragment && fragmentsSize == 0)
    return ip[9]; // Next protocol layer

  if (fragmentsSize != fragmentOffset) {
    PTRACE(2, "PCAPFile\tMissing IP fragment in \"" << GetFilePath() << '"');
    m_fragments.SetSize(0);
    return -1;
  }

  if (fragmentsSize == 0)
    m_fragmentProto = ip[9]; // Next protocol layer

  m_fragments.Concatenate(payload);

  if (isFragment)
    return -1;

  payload.Attach(m_fragments, m_fragments.GetSize());
  m_fragmentated = true;

  return m_fragmentProto; // Next protocol layer
}


int OpalPCAPFile::GetUDP(PBYTEArray & payload)
{
  PBYTEArray udp;
  if (GetIP(udp) != 0x11)
    return -1;

  if (udp.GetSize() < 8)
    return -1;

  m_packetSrcPort = Get<PUInt16b>(udp, 0);
  if (m_filterSrcPort != 0 && m_filterSrcPort != m_packetSrcPort)
    return -1;

  m_packetDstPort = Get<PUInt16b>(udp, 2);
  if (m_filterDstPort != 0 && m_filterDstPort != m_packetDstPort)
    return -1;

  int payloadLength = udp.GetSize() - 8;
  payload.Attach(&udp[8], payloadLength);
  return payloadLength;
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

  return rtp.GetPayloadType();
}


OpalPCAPFile::DiscoveredRTPInfo::DiscoveredRTPInfo()
{
  m_found[0] = m_found[1] = false;
  m_ssrc_matches[0] = m_ssrc_matches[1] = 0;
  m_seq_matches[0]  = m_seq_matches[1]  = 0;
  m_ts_matches[0]   = m_ts_matches[1]   = 0;
  m_index[0] = m_index[1] = 0;
  m_format[0] = m_format[1] = m_type[0] = m_type[1] = "Unknown";
}


void OpalPCAPFile::DiscoveredRTPMap::PrintOn(ostream & strm) const
{
  // display matches
  const_iterator iter;
  for (iter = begin(); iter != end(); ++iter) {
    const DiscoveredRTPInfo & info = iter->second;
    for (int dir = 0; dir < 2; ++dir) {
      if (info.m_found[dir]) {
        if (info.m_payload[dir] != info.m_firstFrame[dir].GetPayloadType())
          strm << "Mismatched payload types" << endl;
        strm << info.m_index[dir] << " : " << info.m_addr[dir].AsString() 
                                  << " -> " << info.m_addr[1-dir].AsString() 
                                  << ", " << info.m_payload[dir] 
                                  << " " << info.m_type[dir]
                                  << " " << info.m_format[dir] << endl;
      }
    }
  }
}


bool OpalPCAPFile::DiscoverRTP(DiscoveredRTPMap & discoveredRTPMap)
{
  if (!Restart())
    return false;

  while (!IsEndOfFile()) {
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
    DiscoveredRTPMap::iterator r;
    if ((r = discoveredRTPMap.find(key)) == discoveredRTPMap.end()) {
      DiscoveredRTPInfo & info = discoveredRTPMap[key];
      info.m_addr[dir].SetAddress(GetSrcIP());
      info.m_addr[dir].SetPort(GetSrcPort());
      info.m_addr[1 - dir].SetAddress(GetDstIP());
      info.m_addr[1 - dir].SetPort(GetDstPort());

      info.m_payload[dir]  = rtp.GetPayloadType();
      info.m_seq[dir]      = rtp.GetSequenceNumber();
      info.m_ts[dir]       = rtp.GetTimestamp();

      info.m_ssrc[dir]     = rtp.GetSyncSource();
      info.m_seq[dir]      = rtp.GetSequenceNumber();
      info.m_ts[dir]       = rtp.GetTimestamp();

      info.m_found[dir]    = true;

      info.m_firstFrame[dir] = rtp;
      info.m_firstFrame[dir].MakeUnique();
    }
    else {
      DiscoveredRTPInfo & info = r->second;
      if (info.m_found[dir]) {
        WORD seq = rtp.GetSequenceNumber();
        DWORD ts = rtp.GetTimestamp();
        DWORD ssrc = rtp.GetSyncSource();
        if (info.m_ssrc[dir] == ssrc)
          ++info.m_ssrc_matches[dir];
        if ((info.m_seq[dir]+1) == seq)
          ++info.m_seq_matches[dir];
        info.m_seq[dir] = seq;
        if ((info.m_ts[dir]+1) < ts)
          ++info.m_ts_matches[dir];
        info.m_ts[dir] = ts;
      }
      else {
        info.m_addr[dir].SetAddress(GetSrcIP());
        info.m_addr[dir].SetPort(GetSrcPort());
        info.m_addr[1 - dir].SetAddress(GetDstIP());
        info.m_addr[1 - dir].SetPort(GetDstPort());

        info.m_payload[dir]  = rtp.GetPayloadType();
        info.m_seq[dir]      = rtp.GetSequenceNumber();
        info.m_ts[dir]       = rtp.GetTimestamp();

        info.m_ssrc[dir]     = rtp.GetSyncSource();
        info.m_seq[dir]      = rtp.GetSequenceNumber();
        info.m_ts[dir]       = rtp.GetTimestamp();

        info.m_found[dir]    = true;

        info.m_firstFrame[dir] = rtp;
        info.m_firstFrame[dir].MakeUnique();
      }
    }
  }

  if (!Restart())
    return false;

  OpalMediaFormatList formats = OpalMediaFormat::GetAllRegisteredMediaFormats();

  size_t index = 1;
  DiscoveredRTPMap::iterator iter = discoveredRTPMap.begin();
  while (iter != discoveredRTPMap.end()) {
    DiscoveredRTPInfo & info = iter->second;
    if (
          (
            info.m_found[0]
            &&
            (
              info.m_seq_matches[0] > 5 ||
              info.m_ts_matches[0] > 5 ||
              info.m_ssrc_matches[0] > 5
            )
          ) 
          ||
          (
            info.m_found[1]
            &&
            (
              info.m_seq_matches[1] > 5 ||
              info.m_ts_matches[1] > 5 ||
              info.m_ssrc_matches[1] > 5
            )
          )
        )
    {
      for (int dir = 0; dir < 2; ++dir) {
        if (!info.m_firstFrame[dir].IsEmpty()) {
          info.m_index[dir] = index++;

          RTP_DataFrame::PayloadTypes pt = info.m_firstFrame[dir].GetPayloadType();

          // look for known audio types
          if (pt <= RTP_DataFrame::Cisco_CN) {
            OpalMediaFormatList::const_iterator r;
            if ((r = formats.FindFormat(pt, OpalMediaFormat::AudioClockRate)) != formats.end()) {
              info.m_type[dir]   = r->GetMediaType();
              info.m_format[dir] = r->GetName();
            }
          }

          // look for known video types
          else if (pt <= RTP_DataFrame::LastKnownPayloadType) {
            OpalMediaFormatList::const_iterator r;
            if ((r = formats.FindFormat(pt, OpalMediaFormat::VideoClockRate)) != formats.end()) {
              info.m_type[dir]   = r->GetMediaType();
              info.m_format[dir] = r->GetName();
            }
          }
          else {
            // try and identify media by inspection
            PINDEX size = info.m_firstFrame[dir].GetPayloadSize();
            if (size > 6) {
              const BYTE * data = info.m_firstFrame[dir].GetPayloadPtr();

              OpalMediaFormatList::const_iterator selectedFormat;

              // xxx00111 01000010 xxxx0000 - H.264
              if ((data[0]&0x1f) == 7 && data[1] == 0x42 && (data[2]&0x0f) == 0)
                selectedFormat = formats.FindFormat("*h.264*");

              // 00000000 00000000 00011011 - MPEG4
              else if (data[0] == 0x00 && data[1] == 0x00 && data[2] == 0x01)
                selectedFormat = formats.FindFormat("*mpeg4*");

              // xxxxx100 00000000 100000xx  - RFC4629/H.263+
              else if ((data[0]&0x07) == 4 && data[1] == 0x00 && (data[2]&0xfc) == 0x80) {
                selectedFormat = formats.FindFormat("*263P*");
                if (selectedFormat == formats.end()) {
                  selectedFormat = formats.FindFormat("*263+*");
                  if (selectedFormat == formats.end())
                    selectedFormat = formats.FindFormat("*263*");
                }
              }

              if (selectedFormat != formats.end()) {
                info.m_type[dir] = OpalMediaType::Video();
                info.m_format[dir] = selectedFormat->GetName();
              }
            }
          }
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
  if (!SetPayloadMap(info.m_payload[dir], format.IsEmpty() ? info.m_format[dir] : format))
    return false;

  m_filterSrcIP = info.m_addr[dir].GetAddress();
  m_filterDstIP = info.m_addr[1 - dir].GetAddress();
  m_filterSrcPort = info.m_addr[dir].GetPort();
  m_filterDstPort = info.m_addr[1 - dir].GetPort();
  return true;
}


bool OpalPCAPFile::SetFilters(const DiscoveredRTPMap & discoveredRTPMap, size_t index, const PString & format)
{
  for (DiscoveredRTPMap::const_iterator iter = discoveredRTPMap.begin();
                                        iter != discoveredRTPMap.end(); ++iter) {
    const OpalPCAPFile::DiscoveredRTPInfo & info = iter->second;
    if (info.m_index[0] == index)
      return SetFilters(info, 0, format);
    if (info.m_index[1] == index)
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
