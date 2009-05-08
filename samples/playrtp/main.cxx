/*
 * main.cxx
 *
 * OPAL application source file for playing RTP from a PCAP file
 *
 * Main program entry point.
 *
 * Copyright (c) 2007 Equivalence Pty. Ltd.
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
 * The Original Code is Portable Windows Library.
 *
 * The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#include "precompile.h"
#include "main.h"


PCREATE_PROCESS(PlayRTP);



struct DiscoveredRTPInfo {
  DiscoveredRTPInfo()
  { 
    m_found[0] = m_found[1] = false;
    m_ssrc_matches[0] = m_ssrc_matches[1] = 0;
    m_seq_matches[0]  = m_seq_matches[1]  = 0;
    m_ts_matches[0]   = m_ts_matches[1]   = 0;
    m_index[0] = m_index[1] = 0;
  }

  PIPSocketAddressAndPort m_addr[2];
  RTP_DataFrame::PayloadTypes m_payload[2];
  BOOL m_found[2];

  DWORD m_ssrc[2];
  WORD  m_seq[2];
  DWORD m_ts[2];

  unsigned m_ssrc_matches[2];
  unsigned m_seq_matches[2];
  unsigned m_ts_matches[2];

  RTP_DataFrame * m_firstFrame[2];

  PString m_type[2];
  PString m_format[2];

  int m_index[2];
};

typedef std::map<std::string, DiscoveredRTPInfo> DiscoveredRTPMap;
DiscoveredRTPMap discoveredRTPMap;

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

#define REVERSE(p) Reverse((char *)p, sizeof(p))

bool IdentifyMediaType(const RTP_DataFrame & rtp, PString & type, PString & format)
{
  OpalMediaFormatList formats = OpalMediaFormat::GetAllRegisteredMediaFormats();

  RTP_DataFrame::PayloadTypes pt = rtp.GetPayloadType();

  type   = "Unknown";
  format = "Unknown";

  // look for known audio types
  if (pt <= RTP_DataFrame::Cisco_CN) {
    OpalMediaFormatList::const_iterator r;
    if ((r = formats.FindFormat(pt, OpalMediaFormat::AudioClockRate)) != formats.end()) {
      type   = r->GetMediaType();
      format = r->GetName();
      return true;
    }
  }

  // look for known video types
  if (pt <= RTP_DataFrame::LastKnownPayloadType) {
    OpalMediaFormatList::const_iterator r;
    if ((r = formats.FindFormat(pt, OpalMediaFormat::VideoClockRate)) != formats.end()) {
      type   = r->GetMediaType();
      format = r->GetName();
      return true;
    }
  }

  // try and identify media by inspection
  if (rtp.GetPayloadSize() > 10) {
    const BYTE * data = rtp.GetPayloadPtr();

    // second byte = 0x42 - H.264
    if (data[1] == 0x42) {
      type = OpalMediaType::Video();
      OpalMediaFormatList::const_iterator r = formats.FindFormat("*h.264*");
      if (r != formats.end())
        format = r->GetName();
    }

    //cout << hex << (int)data[0] << " " << (int)data[1] << " " << (int)data[2] << dec << endl;
  }

  return false;
}

void DisplaySessions()
{
  // display matches
  DiscoveredRTPMap::iterator r;
  int index = 1;
  for (r = discoveredRTPMap.begin(); r != discoveredRTPMap.end(); ++r) {
    DiscoveredRTPInfo & info = r->second;
    for (int dir = 0; dir < 2; ++dir) {
      if (info.m_found[dir]) {
#if 0
        if (info.m_seq_matches[dir] > 5 &&
            info.m_ts_matches[dir] > 5 &&
            info.m_ssrc_matches[dir] > 5) {
#endif
      {
          info.m_index[dir] = index++;

          PString type, format;
          IdentifyMediaType(*info.m_firstFrame[dir], info.m_type[dir], info.m_format[dir]);

          if (info.m_payload[dir] != info.m_firstFrame[dir]->GetPayloadType()) {
            cout << "Mismatched payload types" << endl;
          }
          cout << info.m_index[dir] << " : " << info.m_addr[dir].AsString() 
                                    << " -> " << info.m_addr[1-dir].AsString() 
                                    << ", " << info.m_payload[dir] 
                                    << " " << info.m_type[dir]
                                    << " " << info.m_format[dir] << endl;
        }
      }
    }
  }
}

PlayRTP::PlayRTP()
  : PProcess("OPAL Audio/Video Codec Tester", "PlayRTP", 1, 0, ReleaseCode, 0)
  , m_srcIP(PIPSocket::GetDefaultIpAny())
  , m_dstIP(PIPSocket::GetDefaultIpAny())
  , m_srcPort(0)
  , m_dstPort(0)
  , m_transcoder(NULL)
  , m_player(NULL)
  , m_display(NULL)
{
  OpalMediaFormatList list = OpalMediaFormat::GetAllRegisteredMediaFormats();
  for (PINDEX i = 0; i < list.GetSize(); i++) {
    if (list[i].GetPayloadType() < RTP_DataFrame::DynamicBase)
      m_payloadType2mediaFormat[list[i].GetPayloadType()] = list[i];
  }
}


PlayRTP::~PlayRTP()
{
  delete m_transcoder;
  delete m_player;
}


void PlayRTP::Main()
{
  PArgList & args = GetArguments();

  args.Parse("h-help."
             "m-mapping:"
             "S-src-ip:"
             "D-dst-ip:"
             "s-src-port:"
             "d-dst-port:"
             "A-audio-driver:"
             "a-audio-device:"
             "V-video-driver:"
             "v-video-device:"
             "p-singlestep."
             "i-info."
             "f-find."
#if PTRACING
             "o-output:"             "-no-output."
             "t-trace."              "-no-trace."
#endif
             , FALSE);

#if PTRACING
  PTrace::Initialise(args.GetOptionCount('t'),
                     args.HasOption('o') ? (const char *)args.GetOptionString('o') : NULL,
         PTrace::Blocks | PTrace::Timestamp | PTrace::Thread | PTrace::FileAndLine);
#endif

  if (args.HasOption('h') || args.GetCount() == 0) {
    PError << "usage: " << GetFile().GetTitle() << " [ options ] filename [ filename ... ]\n"
              "\n"
              "Available options are:\n"
              "  --help                   : print this help message.\n"
              "  -m or --mapping N=fmt    : Set mapping of payload type to format, eg 101=H.264\n"
              "  -S or --src-ip addr      : Source IP address, default is any\n"
              "  -D or --dst-ip addr      : Destination IP address, default is any\n"
              "  -s or --src-port N       : Source UDP port, default is any\n"
              "  -d or --dst-port N       : Destination UDP port, default is any\n"
              "  -A or --audio-driver drv : Audio player driver.\n"
              "  -a or --audio-device dev : Audio player device.\n"
              "  -V or --video-driver drv : Video display driver to use.\n"
              "  -v or --video-device dev : Video display device to use.\n"
              "  -p or --singlestep       : Single step through input data.\n"
              "  -i or --info             : Display per-frame information.\n"
              "  -f or --find             : find RTP sessions.\n"
#if PTRACING
              "  -o or --output file     : file name for output of log messages\n"       
              "  -t or --trace           : degree of verbosity in error log (more times for more detail)\n"     
#endif
              "\n"
              "e.g. " << GetFile().GetTitle() << " conversation.pcap\n\n";
    return;
  }

  if (args.HasOption('f')) {
    Find(args[0]);
    if (discoveredRTPMap.size() == 0) {
      cerr << "error: no RTP sessions found" << endl;
      return;
    }
    cout << "Found " << discoveredRTPMap.size() << " sessions:\n" << endl;
    DisplaySessions();
    return;
  }

  if (!args.HasOption('m')) {
    Find(args[0]);
    if (discoveredRTPMap.size() == 0) {
      cerr << "error: no RTP sessions found - please use -m/-S/-D option to specify session manually" << endl;
      return;
    }

    cout << "Select one of the following sessions:\n" << endl;
    DisplaySessions();

    int num;
    for (;;) {
      cout << "Select (1-" << discoveredRTPMap.size()*2 << ") ? " << flush;
      PString line;
      cin >> line;
      line = line.Trim();
      num = line.AsInteger();
      if (num > 0 && num <= discoveredRTPMap.size()*2)
        break;
      cout << "Session " << num << " is not valid" << endl;
    }

    DiscoveredRTPMap::iterator r;
    bool found = false;
    for (r = discoveredRTPMap.begin(); r != discoveredRTPMap.end(); ++r) {
      DiscoveredRTPInfo & info = r->second;
      if (info.m_index[0] == num) {
        OpalMediaFormat mf = info.m_format[0];
        mf.SetPayloadType(info.m_payload[0]);
        m_payloadType2mediaFormat[info.m_payload[0]] = mf;
        m_srcIP = info.m_addr[0].GetAddress();
        m_dstIP = info.m_addr[1].GetAddress();
        m_srcPort = info.m_addr[0].GetPort();
        m_dstPort = info.m_addr[1].GetPort();
        found = true;
      }
      else if (info.m_index[1] == num) {
        OpalMediaFormat mf = info.m_format[1];
        mf.SetPayloadType(info.m_payload[1]);
        m_payloadType2mediaFormat[info.m_payload[1]] = mf;
        m_srcIP = info.m_addr[1].GetAddress();
        m_dstIP = info.m_addr[0].GetAddress();
        m_srcPort = info.m_addr[1].GetPort();
        m_dstPort = info.m_addr[0].GetPort();
        found = true;
      }
    }
    if (!found) {
      cout << "Session " << num << " not valid" << endl;
      return;
    }
  }

  else {
    PStringArray mappings = args.GetOptionString('m').Lines();
    for (PINDEX i = 0; i < mappings.GetSize(); i++) {
      const PString & mapping = mappings[i];
      PINDEX equal = mapping.Find('=');
      if (equal == P_MAX_INDEX) {
        cout << "Invalid syntax for mapping \"" << mapping << '"' << endl;
        continue;
      }

      RTP_DataFrame::PayloadTypes pt = (RTP_DataFrame::PayloadTypes)mapping.Left(equal).AsUnsigned();
      if (pt > RTP_DataFrame::MaxPayloadType) {
        cout << "Invalid payload type for mapping \"" << mapping << '"' << endl;
        continue;
      }

      OpalMediaFormat mf = mapping.Mid(equal+1);
      if (!mf.IsTransportable()) {
        cout << "Invalid media format for mapping \"" << mapping << '"' << endl;
        continue;
      }

      mf.SetPayloadType(pt);
      m_payloadType2mediaFormat[pt] = mf;
    }

    m_srcIP = args.GetOptionString('S', m_srcIP.AsString());
    m_dstIP = args.GetOptionString('D', m_dstIP.AsString());

    m_srcPort = PIPSocket::GetPortByService("udp", args.GetOptionString('s'));
    m_dstPort = PIPSocket::GetPortByService("udp", args.GetOptionString('d', "5000"));
  }

  m_singleStep = args.HasOption('p');
  m_info       = args.HasOption('i');

  // Audio player
  {
    PString driverName = args.GetOptionString('A');
    PString deviceName = args.GetOptionString('a');
    PStringList devices = PSoundChannel::GetDriversDeviceNames("*", PSoundChannel::Player);
    m_player = PSoundChannel::CreateOpenedChannel(driverName, deviceName, PSoundChannel::Player);
    if (m_player == NULL) {
      if (!driverName.IsEmpty() || !deviceName.IsEmpty()) {
        cerr << "Cannot use ";
        if (driverName.IsEmpty() && deviceName.IsEmpty())
          cerr << "default ";
        cerr << "audio player";
        if (!driverName)
          cerr << ", driver \"" << driverName << '"';
        if (!deviceName)
          cerr << ", device \"" << deviceName << '"';
        cerr << ", must be one of:\n";
        PStringList devices = PSoundChannel::GetDriversDeviceNames("*", PSoundChannel::Player);
        for (PINDEX i = 0; i < devices.GetSize(); i++)
          cerr << "   " << devices[i] << '\n';
        cerr << endl;
        return;
      }
      else {
        PString lastTried = "default";
        PINDEX i = 0;
        for (i = 0;m_player == NULL && i < devices.GetSize();++i) {
          deviceName = devices[i];
          if (deviceName *= lastTried) {
            ++i;
            continue;
          }
          cerr << "Cannot use " << lastTried << " audio device - trying " << deviceName << endl;
          m_player = PSoundChannel::CreateOpenedChannel(driverName, deviceName, PSoundChannel::Player);
        }
        if (m_player == NULL) {
          cerr << "Unable to find available sound device" << endl;
          return;
        }
      }
    }

    cout << "Audio Player ";
    if (!driverName.IsEmpty())
      cout << "driver \"" << driverName << "\" and ";
    cout << "device \"" << m_player->GetName() << "\" opened." << endl;
  }

  // Video display
  PString driverName = args.GetOptionString('V');
  PString deviceName = args.GetOptionString('v');
  m_display = PVideoOutputDevice::CreateOpenedDevice(driverName, deviceName, FALSE);
  if (m_display == NULL) {
    cerr << "Cannot use ";
    if (driverName.IsEmpty() && deviceName.IsEmpty())
      cerr << "default ";
    cerr << "video display";
    if (!driverName)
      cerr << ", driver \"" << driverName << '"';
    if (!deviceName)
      cerr << ", device \"" << deviceName << '"';
    cerr << ", must be one of:\n";
    PStringList devices = PVideoOutputDevice::GetDriversDeviceNames("*");
    for (PINDEX i = 0; i < devices.GetSize(); i++)
      cerr << "   " << devices[i] << '\n';
    cerr << endl;
    return;
  }

  m_display->SetColourFormatConverter(OpalYUV420P);

  cout << "Display ";
  if (!driverName.IsEmpty())
    cout << "driver \"" << driverName << "\" and ";
  cout << "device \"" << m_display->GetDeviceName() << "\" opened." << endl;

  for (PINDEX i = 0; i < args.GetCount(); i++)
    Play(args[i]);
}


struct pcap_hdr_s { 
  DWORD magic_number;   /* magic number */
  WORD  version_major;  /* major version number */
  WORD  version_minor;  /* minor version number */
  DWORD thiszone;       /* GMT to local correction */
  DWORD sigfigs;        /* accuracy of timestamps */
  DWORD snaplen;        /* max length of captured packets, in octets */
  DWORD network;        /* data link type */
} pcap_hdr;

struct pcaprec_hdr_s { 
    DWORD ts_sec;         /* timestamp seconds */
    DWORD ts_usec;        /* timestamp microseconds */
    DWORD incl_len;       /* number of octets of packet saved in file */
    DWORD orig_len;       /* actual length of packet */
} pcaprec_hdr;


void PlayRTP::Find(const PFilePath & filename)
{
  PFile pcap;
  if (!pcap.Open(filename, PFile::ReadOnly)) {
    cout << "Could not open file \"" << filename << '"' << endl;
    return;
  }

  if (!pcap.Read(&pcap_hdr, sizeof(pcap_hdr))) {
    cout << "Could not read header from \"" << filename << '"' << endl;
    return;
  }

  bool fileOtherEndian;
  if (pcap_hdr.magic_number == 0xa1b2c3d4)
    fileOtherEndian = false;
  else if (pcap_hdr.magic_number == 0xd4c3b2a1)
    fileOtherEndian = true;
  else {
    cout << "File \"" << filename << "\" is not a PCAP file, bad magic number." << endl;
    return;
  }

  if (fileOtherEndian) {
    REVERSE(pcap_hdr.version_major);
    REVERSE(pcap_hdr.version_minor);
    REVERSE(pcap_hdr.thiszone);
    REVERSE(pcap_hdr.sigfigs);
    REVERSE(pcap_hdr.snaplen);
    REVERSE(pcap_hdr.network);
  }

  cout << "Playing PCAP v" << pcap_hdr.version_major << '.' << pcap_hdr.version_minor << " file \"" << filename << '"' << endl;

  PBYTEArray packetData(pcap_hdr.snaplen); // Every packet is smaller than this
  PBYTEArray fragments;

  while (!pcap.IsEndOfFile()) {

    if (!pcap.Read(&pcaprec_hdr, sizeof(pcaprec_hdr))) {
      cout << "Truncated file \"" << filename << '"' << endl;
      return;
    }

    if (fileOtherEndian) {
      REVERSE(pcaprec_hdr.ts_sec);
      REVERSE(pcaprec_hdr.ts_usec);
      REVERSE(pcaprec_hdr.incl_len);
      REVERSE(pcaprec_hdr.orig_len);
    }

    if (!pcap.Read(packetData.GetPointer(pcaprec_hdr.incl_len), pcaprec_hdr.incl_len)) {
      cout << "Truncated file \"" << filename << '"' << endl;
      return;
    }

    const BYTE * packet = packetData;
    switch (pcap_hdr.network) {
      case 1 :
        if (*(PUInt16b *)(packet+12) != 0x800)
          continue; // Not IP, next packet

        packet += 14; // Skip Data Link Layer Header
        break;

      default :
        cout << "Unsupported Data Link Layer in file \"" << filename << '"' << endl;
        return;
    }

    // Check for fragmentation bit
    packet += 6;
    bool isFragment = (*packet & 0x20) != 0;
    int fragmentOffset = (((packet[0]&0x1f)<<8)+packet[1])*8;

    // Skip first bit of IP header
    packet += 3;
    if (*packet != 0x11)
      continue; // Not UDP

    PIPSocketAddressAndPort rtpSrc;
    PIPSocketAddressAndPort rtpDst;

    packet += 3;
    rtpSrc.SetAddress(PIPSocket::Address(4, packet));
    //if (!m_srcIP.IsAny() && m_srcIP != PIPSocket::Address(4, packet))
    //  continue; // Not specified source IP address

    packet += 4;
    rtpDst.SetAddress(PIPSocket::Address(4, packet));
    //if (!m_dstIP.IsAny() && m_dstIP != PIPSocket::Address(4, packet))
    //  continue; // Not specified destination IP address

    // On to the UDP header
    packet += 4;

    // As we are past IP header, handle fragmentation now
    PINDEX fragmentsSize = fragments.GetSize();
    if (isFragment || fragmentsSize > 0) {
      if (fragmentsSize != fragmentOffset) {
        cout << "Missing IP fragment in \"" << filename << '"' << endl;
        fragments.SetSize(0);
        continue;
      }

      fragments.Concatenate(PBYTEArray(packet, pcaprec_hdr.incl_len - (packet - packetData), false));

      if (isFragment)
        continue;

      packetData = fragments;
      pcaprec_hdr.incl_len = packetData.GetSize();
      fragments.SetSize(0);
      packet = packetData;
    }

    // Check UDP ports
    //if (m_srcPort != 0 && m_srcPort != *(PUInt16b *)packet)
    //  continue;
    rtpSrc.SetPort(*(PUInt16b *)packet);

    packet += 2;
    //if (m_dstPort != 0 && m_dstPort != *(PUInt16b *)packet)
    //  continue;
    rtpDst.SetPort(*(PUInt16b *)packet);

    // On to (probably) RTP header
    packet += 6;

    // see if this is an RTP packet
    int rtpLen = pcaprec_hdr.incl_len - (packet - packetData);

    // must be at least this long
    if (rtpLen < RTP_DataFrame::MinHeaderSize)
      continue;

    // must have version number 2
    RTP_DataFrame rtp(packet, rtpLen, FALSE);
    if (rtp.GetVersion() != 2)
      continue;

    // determine if reverse or forward session
    bool reverse;
    if (rtpSrc.GetAddress() != rtpDst.GetAddress())
      reverse = rtpSrc.GetAddress() > rtpDst.GetAddress();
    else
      reverse = rtpSrc.GetPort() > rtpDst.GetPort();

    PString key;
    if (reverse)
      key = rtpDst.AsString() + "|" + rtpSrc.AsString();
    else
      key = rtpSrc.AsString() + "|" + rtpDst.AsString();

    std::string k(key);

    // see if we have identified this potential session before
    DiscoveredRTPMap::iterator r;
    if ((r = discoveredRTPMap.find(k)) == discoveredRTPMap.end()) {
      DiscoveredRTPInfo info;
      int dir = reverse ? 1 : 0;
      info.m_found  [dir]  = true;
      info.m_addr[dir]     = rtpSrc;
      info.m_addr[1 - dir] = rtpDst;
      info.m_payload[dir]  = rtp.GetPayloadType();
      info.m_seq[dir]      = rtp.GetSequenceNumber();
      info.m_ts[dir]       = rtp.GetTimestamp();

      info.m_ssrc[dir]     = rtp.GetSyncSource();
      info.m_seq[dir]      = rtp.GetSequenceNumber();
      info.m_ts[dir]       = rtp.GetTimestamp();

      info.m_firstFrame[dir] = new RTP_DataFrame(rtp.GetPointer(), rtp.GetSize());

      discoveredRTPMap.insert(DiscoveredRTPMap::value_type(k, info));
    }
    else {
      DiscoveredRTPInfo & info = r->second;
      int dir = reverse ? 1 : 0;
      if (!info.m_found[dir]) {
        info.m_found  [dir]  = true;
        info.m_addr[dir]     = rtpSrc;
        info.m_addr[1 - dir] = rtpDst;
        info.m_payload[dir]  = rtp.GetPayloadType();
        info.m_seq[dir]      = rtp.GetSequenceNumber();
        info.m_ts[dir]       = rtp.GetTimestamp();

        info.m_ssrc[dir]     = rtp.GetSyncSource();
        info.m_seq[dir]      = rtp.GetSequenceNumber();
        info.m_ts[dir]       = rtp.GetTimestamp();

        info.m_firstFrame[dir] = new RTP_DataFrame(rtp.GetPointer(), rtp.GetSize());
      }
      else
      {
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
    }
  }

  // display matches
  DiscoveredRTPMap::iterator r = discoveredRTPMap.begin();
  while (r != discoveredRTPMap.end()) {
    DiscoveredRTPInfo & info = r->second;
    if (
        (info.m_found[0] && 
         info.m_seq_matches[0] > 5 &&
         info.m_ts_matches[0] > 5 &&
         info.m_ssrc_matches[0] > 5
         ) 
         ||
        (info.m_found[1] && 
         info.m_seq_matches[1] > 5 &&
         info.m_ts_matches[1] > 5 &&
         info.m_ssrc_matches[1] > 5
         ) 
         )
    {
        ++r;
    }
    else {
      discoveredRTPMap.erase(r);
      r = discoveredRTPMap.begin();
    }
  }

}


void PlayRTP::Play(const PFilePath & filename)
{
  PFile pcap;
  if (!pcap.Open(filename, PFile::ReadOnly)) {
    cout << "Could not open file \"" << filename << '"' << endl;
    return;
  }

  if (!pcap.Read(&pcap_hdr, sizeof(pcap_hdr))) {
    cout << "Could not read header from \"" << filename << '"' << endl;
    return;
  }

  bool fileOtherEndian;
  if (pcap_hdr.magic_number == 0xa1b2c3d4)
    fileOtherEndian = false;
  else if (pcap_hdr.magic_number == 0xd4c3b2a1)
    fileOtherEndian = true;
  else {
    cout << "File \"" << filename << "\" is not a PCAP file, bad magic number." << endl;
    return;
  }

  if (fileOtherEndian) {
    REVERSE(pcap_hdr.version_major);
    REVERSE(pcap_hdr.version_minor);
    REVERSE(pcap_hdr.thiszone);
    REVERSE(pcap_hdr.sigfigs);
    REVERSE(pcap_hdr.snaplen);
    REVERSE(pcap_hdr.network);
  }

  cout << "Playing PCAP v" << pcap_hdr.version_major << '.' << pcap_hdr.version_minor << " file \"" << filename << '"' << endl;

  PBYTEArray packetData(pcap_hdr.snaplen); // Every packet is smaller than this
  PBYTEArray fragments;

  RTP_DataFrame::PayloadTypes rtpStreamPayloadType = RTP_DataFrame::IllegalPayloadType;
  RTP_DataFrame::PayloadTypes lastUnsupportedPayloadType = RTP_DataFrame::IllegalPayloadType;
  DWORD lastTimeStamp = 0;

  while (!pcap.IsEndOfFile()) {
    if (!pcap.Read(&pcaprec_hdr, sizeof(pcaprec_hdr))) {
      cout << "Truncated file \"" << filename << '"' << endl;
      return;
    }

    if (fileOtherEndian) {
      REVERSE(pcaprec_hdr.ts_sec);
      REVERSE(pcaprec_hdr.ts_usec);
      REVERSE(pcaprec_hdr.incl_len);
      REVERSE(pcaprec_hdr.orig_len);
    }

    if (!pcap.Read(packetData.GetPointer(pcaprec_hdr.incl_len), pcaprec_hdr.incl_len)) {
      cout << "Truncated file \"" << filename << '"' << endl;
      return;
    }

    const BYTE * packet = packetData;
    switch (pcap_hdr.network) {
      case 1 :
        if (*(PUInt16b *)(packet+12) != 0x800)
          continue; // Not IP, next packet

        packet += 14; // Skip Data Link Layer Header
        break;

      default :
        cout << "Unsupported Data Link Layer in file \"" << filename << '"' << endl;
        return;
    }

    // Check for fragmentation bit
    packet += 6;
    bool isFragment = (*packet & 0x20) != 0;
    int fragmentOffset = (((packet[0]&0x1f)<<8)+packet[1])*8;

    // Skip first bit of IP header
    packet += 3;
    if (*packet != 0x11)
      continue; // Not UDP

    packet += 3;
    if (!m_srcIP.IsAny() && m_srcIP != PIPSocket::Address(4, packet))
      continue; // Not specified source IP address

    packet += 4;
    if (!m_dstIP.IsAny() && m_dstIP != PIPSocket::Address(4, packet))
      continue; // Not specified destination IP address

    // On to the UDP header
    packet += 4;

    // As we are past IP header, handle fragmentation now
    PINDEX fragmentsSize = fragments.GetSize();
    if (isFragment || fragmentsSize > 0) {
      if (fragmentsSize != fragmentOffset) {
        cout << "Missing IP fragment in \"" << filename << '"' << endl;
        fragments.SetSize(0);
        continue;
      }

      fragments.Concatenate(PBYTEArray(packet, pcaprec_hdr.incl_len - (packet - packetData), false));

      if (isFragment)
        continue;

      packetData = fragments;
      pcaprec_hdr.incl_len = packetData.GetSize();
      fragments.SetSize(0);
      packet = packetData;
    }

    // Check UDP ports
    if (m_srcPort != 0 && m_srcPort != *(PUInt16b *)packet)
      continue;

    packet += 2;
    if (m_dstPort != 0 && m_dstPort != *(PUInt16b *)packet)
      continue;

    // On to (probably) RTP header
    packet += 6;
    RTP_DataFrame rtp(packet, pcaprec_hdr.incl_len - (packet - packetData), FALSE);

    if (rtp.GetVersion() != 2)
      continue;

    if (rtpStreamPayloadType != rtp.GetPayloadType()) {
      if (rtpStreamPayloadType != RTP_DataFrame::IllegalPayloadType) {
        cout << "Payload type changed in mid file \"" << filename << '"' << endl;
        return;
      }
      rtpStreamPayloadType = rtp.GetPayloadType();
    }

    if (m_transcoder == NULL) {
      if (m_payloadType2mediaFormat.find(rtpStreamPayloadType) == m_payloadType2mediaFormat.end()) {
        if (lastUnsupportedPayloadType != rtpStreamPayloadType) {
          cout << "Unsupported Payload Type " << rtpStreamPayloadType << " in file \"" << filename << '"' << endl;
          lastUnsupportedPayloadType = rtpStreamPayloadType;
        }
        rtpStreamPayloadType = RTP_DataFrame::IllegalPayloadType;
        continue;
      }

      OpalMediaFormat srcFmt = m_payloadType2mediaFormat[rtpStreamPayloadType];
      OpalMediaFormat dstFmt;
      if (srcFmt.GetMediaType() == OpalMediaType::Audio())
        dstFmt = OpalPCM16;
      else if (srcFmt.GetMediaType() == OpalMediaType::Video()) {
          dstFmt = OpalYUV420P;
          m_display->Start();
      }
      else {
        cout << "Unsupported Media Type " << srcFmt.GetMediaType() << " in file \"" << filename << '"' << endl;
        return;
      }

      m_transcoder = OpalTranscoder::Create(srcFmt, dstFmt);
      if (m_transcoder == NULL) {
        cout << "No transcoder for " << srcFmt << " in file \"" << filename << '"' << endl;
        return;
      }

      cout << "Decoding " << srcFmt << " from file \"" << filename << '"' << endl;
      m_transcoder->SetCommandNotifier(PCREATE_NOTIFIER(OnTranscoderCommand));
      lastTimeStamp = rtp.GetTimestamp();
    }

    const OpalMediaFormat & inputFmt = m_transcoder->GetInputFormat();

    if (rtp.GetTimestamp() != lastTimeStamp) {
      unsigned msecs = (rtp.GetTimestamp() - lastTimeStamp)/inputFmt.GetTimeUnits();
      if (msecs < 3000) 
        PThread::Sleep(msecs);
      else 
        cout << "ignoring timestamp jump > 3 seconds" << endl;
      lastTimeStamp = rtp.GetTimestamp();
    }

    if (m_singleStep) 
      cout << "Input packet of length " << rtp.GetPayloadSize() << (rtp.GetMarker() ? " with MARKER" : "") << " -> ";
    else if (m_info)
      cout << "ssrc=" << hex << rtp.GetSyncSource() << dec << ",ts=" << rtp.GetTimestamp() << ",seq = " << rtp.GetSequenceNumber() << endl;


    RTP_DataFrameList output;
    if (!m_transcoder->ConvertFrames(rtp, output)) {
      cout << "Error decoding file \"" << filename << '"' << endl;
      return;
    }

    if (output.GetSize() == 0) {
      if (m_singleStep) 
        cout << "no frame" << endl;
    }
    else for (PINDEX i = 0; i < output.GetSize(); i++) {
      if (m_singleStep)
        cout << output.GetSize() << " packets" << endl;
      const RTP_DataFrame & data = output[i];
      if (inputFmt.GetMediaType() == OpalMediaType::Audio())
        m_player->Write(data.GetPayloadPtr(), data.GetPayloadSize());
      else {
        const OpalVideoTranscoder::FrameHeader * frame = (const OpalVideoTranscoder::FrameHeader *)data.GetPayloadPtr();
        m_display->SetFrameSize(frame->width, frame->height);
        m_display->SetFrameData(frame->x, frame->y,
                                frame->width, frame->height,
                                OPAL_VIDEO_FRAME_DATA_PTR(frame), data.GetMarker());
      }
    }
    if (m_singleStep) {
      char ch;
      cin >> ch;
    }

  }

  delete m_transcoder;
  m_transcoder = NULL;
}


void PlayRTP::OnTranscoderCommand(OpalMediaCommand & command, INT /*extra*/)
{
  if (PIsDescendant(&command, OpalVideoUpdatePicture))
    cout << "Decoder error in received stream." << endl;
}


// End of File ///////////////////////////////////////////////////////////////
