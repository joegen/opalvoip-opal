/*
 * pcapfile.h
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

#ifndef PTLIB_PCAPFILE_H
#define PTLIB_PCAPFILE_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <rtp/rtp.h>
#include <opal/mediafmt.h>
#include <ptlib/sockets.h>


/**Class for a reading RTP from an Ethernet Capture (PCAP) file.
 */
class OpalPCAPFile : public PFile
{
    PCLASSINFO(OpalPCAPFile, PFile);
  public:
    OpalPCAPFile();

    bool Open(const PFilePath & filename);
    bool Restart();

    void PrintOn(ostream & strm) const;

    bool ReadRawPacket(PBYTEArray & payload);
    int GetDataLink(PBYTEArray & payload);
    int GetIP(PBYTEArray & payload);
    int GetUDP(PBYTEArray & payload);
    int GetRTP(RTP_DataFrame & rtp);

    const PTime & GetPacketTime() const { return m_packetTime; }
    const PIPSocket::Address & GetSrcIP() const { return m_packetSrcIP; }
    const PIPSocket::Address & GetDstIP() const { return m_packetDstIP; }
    unsigned IsFragmentated() const { return m_fragmentated; }
    WORD GetSrcPort() const { return m_packetSrcPort; }
    WORD GetDstPort() const { return m_packetDstPort; }

    void SetFilterSrcIP(
      const PIPSocket::Address & ip
    ) { m_filterSrcIP = ip; }
    const PIPSocket::Address & GetFilterSrcIP() const { return m_filterSrcIP; }

    void SetFilterDstIP(
      const PIPSocket::Address & ip
    ) { m_filterDstIP = ip; }
    const PIPSocket::Address & GetFilterDstIP() const { return m_filterDstIP; }

    void SetFilterSrcPort(
      WORD port
    ) { m_filterSrcPort = port; }
    WORD GetFilterSrcPort() const { return m_filterSrcPort; }

    void SetFilterDstPort(
      WORD port
    ) { m_filterDstPort = port; }
    WORD GetFilterDstPort() const { return m_filterDstPort; }


    struct DiscoveredRTPInfo {
      DiscoveredRTPInfo();

      PIPSocketAddressAndPort     m_addr[2];
      RTP_DataFrame::PayloadTypes m_payload[2];
      bool                        m_found[2];

      DWORD m_ssrc[2];
      WORD  m_seq[2];
      DWORD m_ts[2];

      unsigned m_ssrc_matches[2];
      unsigned m_seq_matches[2];
      unsigned m_ts_matches[2];

      RTP_DataFrame m_firstFrame[2];

      PString m_type[2];
      PString m_format[2];

      size_t m_index[2];
    };
    class DiscoveredRTPMap : public PObject, public std::map<std::string, DiscoveredRTPInfo>
    {
        PCLASSINFO(DiscoveredRTPMap, PObject);
      public:
        void PrintOn(ostream & strm) const;
    };

    bool DiscoverRTP(DiscoveredRTPMap & discoveredRTPMap);

    bool SetFilters(
      const DiscoveredRTPInfo & rtp,
      int dir,
      const PString & format = PString::Empty()
    );
    bool SetFilters(
      const DiscoveredRTPMap & rtp,
      size_t index,
      const PString & format = PString::Empty()
    );

    bool SetPayloadMap(
      RTP_DataFrame::PayloadTypes pt,
      const OpalMediaFormat & format
    );

    OpalMediaFormat GetMediaFormat(const RTP_DataFrame & rtp) const;

  protected:
    PINDEX GetNetworkLayerHeaderSize();

    struct FileHeader { 
      DWORD magic_number;   /* magic number */
      WORD  version_major;  /* major version number */
      WORD  version_minor;  /* minor version number */
      DWORD thiszone;       /* GMT to local correction */
      DWORD sigfigs;        /* accuracy of timestamps */
      DWORD snaplen;        /* max length of captured packets, in octets */
      DWORD network;        /* data link type */
    };

    struct RecordHeader { 
      DWORD ts_sec;         /* timestamp seconds */
      DWORD ts_usec;        /* timestamp microseconds */
      DWORD incl_len;       /* number of octets of packet saved in file */
      DWORD orig_len;       /* actual length of packet */
    };


    FileHeader m_fileHeader;
    bool       m_otherEndian;
    PBYTEArray m_rawPacket;
    PTime      m_packetTime;

    PIPSocket::Address m_filterSrcIP;
    PIPSocket::Address m_filterDstIP;
    PIPSocket::Address m_packetSrcIP;
    PIPSocket::Address m_packetDstIP;

    PBYTEArray m_fragments;
    bool       m_fragmentated;
    unsigned   m_fragmentProto;

    WORD m_filterSrcPort;
    WORD m_filterDstPort;
    WORD m_packetSrcPort;
    WORD m_packetDstPort;

    std::map<RTP_DataFrame::PayloadTypes, OpalMediaFormat> m_payloadType2mediaFormat;
};


#endif // PTLIB_PCAPFILE_H


// End Of File ///////////////////////////////////////////////////////////////
