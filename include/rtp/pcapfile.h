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
 */

#ifndef PTLIB_PCAPFILE_H
#define PTLIB_PCAPFILE_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <rtp/rtp.h>
#include <opal/mediafmt.h>
#include <ptlib/sockets.h>


class OpalTranscoder;


/**Class for a reading RTP from an Ethernet Capture (PCAP) file.
 */
class OpalPCAPFile : public PFile
{
    PCLASSINFO(OpalPCAPFile, PFile);
  public:
    OpalPCAPFile();

    bool Restart();

    void PrintOn(ostream & strm) const;

    bool WriteFrame(const PEthSocket::Frame & frame);
    bool WriteRTP(const RTP_DataFrame & rtp, WORD port = 5000);

    int GetDataLink(PBYTEArray & payload);
    int GetIP(PBYTEArray & payload);
    int GetTCP(PBYTEArray & payload);
    int GetUDP(PBYTEArray & payload);
    int GetRTP(RTP_DataFrame & rtp);

    struct DecodeContext
    {
      OpalTranscoder   * m_transcoder;
      RTP_SequenceNumber m_lastSequenceNumber;
      RTP_SyncSourceId   m_lastSSRC;
      off_t              m_lastPacketsFilePosition;
      DecodeContext()
        : m_transcoder(NULL)
        , m_lastSequenceNumber(0)
        , m_lastSSRC(0)
        , m_lastPacketsFilePosition(0)
      { }
      ~DecodeContext();
    };
    int GetDecodedRTP(RTP_DataFrame & decodedRTP, DecodeContext & context);
    int DecodeRTP(RTP_DataFrame & encodedRTP, RTP_DataFrame & decodedRTP, DecodeContext & context);


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


    struct DiscoveredRTPKey : PObject
    {
      PIPAddressAndPort m_src;
      PIPAddressAndPort m_dst;
      RTP_SyncSourceId  m_ssrc;

      DiscoveredRTPKey();
      Comparison Compare(const PObject & obj) const;
      void PrintOn(ostream & strm) const;
    };

    struct DiscoveredRTPInfo : DiscoveredRTPKey {
      RTP_DataFrame::PayloadTypes m_payloadType;
      OpalMediaFormat             m_mediaFormat;

      DiscoveredRTPInfo();
      DiscoveredRTPInfo(const DiscoveredRTPKey & key);
      void PrintOn(ostream & strm) const;
    };

    typedef PArray<DiscoveredRTPInfo> DiscoveredRTP;

    struct Progress
    {
      Progress(off_t length)
        : m_fileLength(length)
        , m_filePosition(0)
        , m_packets(0)
        , m_abort(false)
      { }

      off_t    m_fileLength;
      off_t    m_filePosition;
      unsigned m_packets;
      bool     m_abort;
    };
    typedef PNotifierTemplate<Progress &> ProgressNotifier;

    bool DiscoverRTP(DiscoveredRTP & discoveredRTP, const ProgressNotifier & progressNotifier = NULL);

    bool SetFilters(
      const DiscoveredRTPInfo & discoveredRTP,
      const PString & format = PString::Empty()
    );

    typedef std::map<RTP_DataFrame::PayloadTypes, OpalMediaFormat> PayloadMap;

    void SetPayloadMap(
      const PayloadMap & payloadMap,
      bool overwrite = false
    );
    bool SetPayloadMap(
      RTP_DataFrame::PayloadTypes pt,
      const OpalMediaFormat & format
    );

    OpalMediaFormat GetMediaFormat(const RTP_DataFrame & rtp) const;

  protected:
    bool InternalOpen(OpenMode mode, OpenOptions opt, PFileInfo::Permissions permissions);
    int InternalDecodeRTP(RTP_DataFrame & encodedRTP, RTP_DataFrame & decodedRTP, DecodeContext & context);

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
    PDECLARE_MUTEX(m_writeMutex);

    PIPSocketAddressAndPort m_filterSrc;
    PIPSocketAddressAndPort m_filterDst;
    RTP_SyncSourceId        m_filterSSRC;
    PIPSocketAddressAndPort m_packetSrc;
    PIPSocketAddressAndPort m_packetDst;

    PayloadMap m_payloadType2mediaFormat;

    struct DiscoveryInfo;
    typedef std::map<DiscoveredRTPKey, DiscoveryInfo> DiscoveryMap;
};


#endif // PTLIB_PCAPFILE_H


// End Of File ///////////////////////////////////////////////////////////////
