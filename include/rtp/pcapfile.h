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

#if P_PCAP

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

    int GetDataLink(PBYTEArray & payload);
    int GetIP(PBYTEArray & payload);
    int GetUDP(PBYTEArray & payload);
    int GetRTP(RTP_DataFrame & rtp);

    const PTime & GetPacketTime() const { return m_rawPacket.GetTimestamp(); }
    const PIPSocket::Address & GetSrcIP() const { return m_packetSrc.GetAddress(); }
    const PIPSocket::Address & GetDstIP() const { return m_packetDst.GetAddress(); }
    unsigned IsFragmentated() const { return m_rawPacket.IsFragmentated(); }
    WORD GetSrcPort() const { return m_packetSrc.GetPort(); }
    WORD GetDstPort() const { return m_packetDst.GetPort(); }

    void SetFilterSrcIP(
      const PIPSocket::Address & ip
    ) { m_filterSrc.SetAddress(ip); }
    const PIPSocket::Address & GetFilterSrcIP() const { return m_filterSrc.GetAddress(); }

    void SetFilterDstIP(
      const PIPSocket::Address & ip
    ) { m_filterDst.SetAddress(ip); }
    const PIPSocket::Address & GetFilterDstIP() const { return m_filterDst.GetAddress(); }

    void SetFilterSrcPort(
      WORD port
    ) { m_filterSrc.SetPort(port); }
    WORD GetFilterSrcPort() const { return m_filterSrc.GetPort(); }

    void SetFilterDstPort(
      WORD port
    ) { m_filterDst.SetPort(port); }
    WORD GetFilterDstPort() const { return m_filterDst.GetPort(); }


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

    class Frame : public PEthSocket::Frame {
      public:
        Frame() : m_otherEndian(false) { }

        virtual bool Read(
          PChannel & channel,
          PINDEX packetSize = P_MAX_INDEX
        );

        bool m_otherEndian;
    };
    Frame m_rawPacket;

    PIPSocketAddressAndPort m_filterSrc;
    PIPSocketAddressAndPort m_filterDst;
    PIPSocketAddressAndPort m_packetSrc;
    PIPSocketAddressAndPort m_packetDst;

    std::map<RTP_DataFrame::PayloadTypes, OpalMediaFormat> m_payloadType2mediaFormat;
};

#endif // P_PCAP

#endif // PTLIB_PCAPFILE_H


// End Of File ///////////////////////////////////////////////////////////////
