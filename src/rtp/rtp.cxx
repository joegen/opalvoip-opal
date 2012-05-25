/*
 * rtp.cxx
 *
 * RTP protocol handler
 *
 * Open H323 Library
 *
 * Copyright (c) 1998-2000 Equivalence Pty. Ltd.
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
 * The Original Code is Open H323 Library.
 *
 * The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * Portions of this code were written with the assisance of funding from
 * Vovida Networks, Inc. http://www.vovida.com.
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "rtp.h"
#endif

#include <opal/buildopts.h>

#include <rtp/rtp.h>

#include <rtp/jitter.h>

#include <rtp/metrics.h>

#include <ptclib/random.h>
#include <ptclib/pstun.h>
#include <opal/rtpconn.h>

#include <algorithm>

#define new PNEW

#define BAD_TRANSMIT_TIME_MAX 10    //  maximum of seconds of transmit fails before session is killed

const unsigned SecondsFrom1900to1970 = (70*365+17)*24*60*60U;

#define RTP_VIDEO_RX_BUFFER_SIZE 0x100000 // 1Mb
#define RTP_AUDIO_RX_BUFFER_SIZE 0x4000   // 16kb
#define RTP_DATA_TX_BUFFER_SIZE  0x2000   // 8kb
#define RTP_CTRL_BUFFER_SIZE     0x1000   // 4kb


PFACTORY_CREATE(PFactory<RTP_Encoding>, RTP_Encoding, "rtp/avp", false);


/////////////////////////////////////////////////////////////////////////////

RTP_DataFrame::RTP_DataFrame(PINDEX payloadSz, PINDEX bufferSz)
  : PBYTEArray(std::max(bufferSz, MinHeaderSize+payloadSz))
  , m_headerSize(MinHeaderSize)
  , m_payloadSize(payloadSz)
  , m_paddingSize(0)
{
  theArray[0] = '\x80'; // Default to version 2
  theArray[1] = '\x7f'; // Default to MaxPayloadType
}


RTP_DataFrame::RTP_DataFrame(const BYTE * data, PINDEX len, PBoolean dynamic)
  : PBYTEArray(data, len, dynamic)
  , m_headerSize(MinHeaderSize)
  , m_payloadSize(0)
  , m_paddingSize(0)
{
  SetPacketSize(len);
}


bool RTP_DataFrame::SetPacketSize(PINDEX sz)
{
  if (sz < RTP_DataFrame::MinHeaderSize) {
    PTRACE(2, "RTP\tInvalid RTP packet, "
              "smaller than minimum header size, " << sz << " < " << RTP_DataFrame::MinHeaderSize);
    m_payloadSize = m_paddingSize = 0;
    return false;
  }

  m_headerSize = MinHeaderSize + 4*GetContribSrcCount();

  if (GetExtension())
    m_headerSize += (GetExtensionSizeDWORDs()+1)*4;

  if (sz < m_headerSize) {
    PTRACE(2, "RTP\tInvalid RTP packet, "
              "smaller than indicated header size, " << sz << " < " << m_headerSize);
    m_payloadSize = m_paddingSize = 0;
    return false;
  }

  if (!GetPadding()) {
    m_payloadSize = sz - m_headerSize;
    return true;
  }

  /* We do this as some systems send crap at the end of the packet, giving
     incorrect results for the padding size. So we do a sanity check that
     the indicating padding size is not larger than the payload itself. Not
     100% accurate, but you do whatever you can.
   */
  PINDEX pos = sz;
  do {
    if (pos-- <= m_headerSize) {
      PTRACE(2, "RTP\tInvalid RTP packet, padding indicated but not enough data, "
                "size=" << sz << ", header=" << m_headerSize);
      m_payloadSize = m_paddingSize = 0;
      return false;
    }

    m_paddingSize = theArray[pos] & 0xff;
  } while (m_paddingSize > (pos-m_headerSize));

  m_payloadSize = pos - m_headerSize - 1;

  return true;
}


void RTP_DataFrame::SetExtension(PBoolean ext)
{
  if (ext)
    theArray[0] |= 0x10;
  else
    theArray[0] &= 0xef;
}


void RTP_DataFrame::SetMarker(PBoolean m)
{
  if (m)
    theArray[1] |= 0x80;
  else
    theArray[1] &= 0x7f;
}


void RTP_DataFrame::SetPayloadType(PayloadTypes t)
{
  PAssert(t <= 0x7f, PInvalidParameter);

  theArray[1] &= 0x80;
  theArray[1] |= t;
}


DWORD RTP_DataFrame::GetContribSource(PINDEX idx) const
{
  PAssert(idx < GetContribSrcCount(), PInvalidParameter);
  return ((PUInt32b *)&theArray[MinHeaderSize])[idx];
}


void RTP_DataFrame::SetContribSource(PINDEX idx, DWORD src)
{
  PAssert(idx <= 15, PInvalidParameter);

  if (idx >= GetContribSrcCount()) {
    BYTE * oldPayload = GetPayloadPtr();
    theArray[0] &= 0xf0;
    theArray[0] |= idx+1;
    m_headerSize += 4;
    PINDEX sz = m_payloadSize + m_paddingSize;
    SetMinSize(m_headerSize+sz);
    memmove(GetPayloadPtr(), oldPayload, sz);
  }

  ((PUInt32b *)&theArray[MinHeaderSize])[idx] = src;
}


int RTP_DataFrame::GetExtensionType() const
{
  if (GetExtension())
    return *(PUInt16b *)&theArray[MinHeaderSize + 4*GetContribSrcCount()];

  return -1;
}


void RTP_DataFrame::SetExtensionType(int type)
{
  if (type < 0)
    SetExtension(false);
  else {
    if (!GetExtension())
      SetExtensionSizeDWORDs(0);
    *(PUInt16b *)&theArray[MinHeaderSize + 4*GetContribSrcCount()] = (WORD)type;
  }
}


PINDEX RTP_DataFrame::GetExtensionSizeDWORDs() const
{
  if (GetExtension())
    return *(PUInt16b *)&theArray[MinHeaderSize + 4*GetContribSrcCount() + 2];

  return 0;
}


PBoolean RTP_DataFrame::SetExtensionSizeDWORDs(PINDEX sz)
{
  m_headerSize = MinHeaderSize + 4*GetContribSrcCount() + (sz+1)*4;
  if (!SetMinSize(m_headerSize+m_payloadSize+m_paddingSize))
    return false;

  SetExtension(true);
  *(PUInt16b *)&theArray[MinHeaderSize + 4*GetContribSrcCount() + 2] = (WORD)sz;
  return true;
}


BYTE * RTP_DataFrame::GetExtensionPtr() const
{
  if (GetExtension())
    return (BYTE *)&theArray[MinHeaderSize + 4*GetContribSrcCount() + 4];

  return NULL;
}


bool RTP_DataFrame::SetPayloadSize(PINDEX sz)
{
  m_payloadSize = sz;
  return SetMinSize(m_headerSize+m_payloadSize+m_paddingSize);
}


bool RTP_DataFrame::SetPaddingSize(PINDEX sz)
{
  m_paddingSize = sz;
  return SetMinSize(m_headerSize+m_payloadSize+m_paddingSize);
}


void RTP_DataFrame::PrintOn(ostream & strm) const
{
  strm <<  "V="  << GetVersion()
       << " X="  << GetExtension()
       << " M="  << GetMarker()
       << " PT=" << GetPayloadType()
       << " SN=" << GetSequenceNumber()
       << " TS=" << GetTimestamp()
       << " SSRC=" << hex << GetSyncSource() << dec
       << " size=" << GetPayloadSize()
       << '\n';

  int csrcCount = GetContribSrcCount();
  for (int csrc = 0; csrc < csrcCount; csrc++)
    strm << "  CSRC[" << csrc << "]=" << GetContribSource(csrc) << '\n';

  if (GetExtension())
    strm << "  Header Extension Type: " << GetExtensionType() << '\n'
         << hex << setfill('0') << PBYTEArray(GetExtensionPtr(), GetExtensionSizeDWORDs()*4, false) << setfill(' ') << dec << '\n';

  strm << hex << setfill('0') << PBYTEArray(GetPayloadPtr(), GetPayloadSize(), false) << setfill(' ') << dec;
}


#if PTRACING
static const char * const PayloadTypesNames[RTP_DataFrame::LastKnownPayloadType] = {
  "PCMU",
  "FS1016",
  "G721",
  "GSM",
  "G723",
  "DVI4_8k",
  "DVI4_16k",
  "LPC",
  "PCMA",
  "G722",
  "L16_Stereo",
  "L16_Mono",
  "G723",
  "CN",
  "MPA",
  "G728",
  "DVI4_11k",
  "DVI4_22k",
  "G729",
  "CiscoCN",
  NULL, NULL, NULL, NULL, NULL,
  "CelB",
  "JPEG",
  NULL, NULL, NULL, NULL,
  "H261",
  "MPV",
  "MP2T",
  "H263",
  NULL, NULL, NULL,
  "T38"
};

ostream & operator<<(ostream & o, RTP_DataFrame::PayloadTypes t)
{
  if ((PINDEX)t < PARRAYSIZE(PayloadTypesNames) && PayloadTypesNames[t] != NULL)
    o << PayloadTypesNames[t];
  else
    o << "[pt=" << (int)t << ']';
  return o;
}

#endif


/////////////////////////////////////////////////////////////////////////////

RTP_ControlFrame::RTP_ControlFrame(PINDEX sz)
  : PBYTEArray(sz)
{
  compoundOffset = 0;
  payloadSize = 0;
}

void RTP_ControlFrame::Reset(PINDEX size)
{
  SetSize(size);
  compoundOffset = 0;
  payloadSize = 0;
}


void RTP_ControlFrame::SetCount(unsigned count)
{
  PAssert(count < 32, PInvalidParameter);
  theArray[compoundOffset] &= 0xe0;
  theArray[compoundOffset] |= count;
}


void RTP_ControlFrame::SetFbType(unsigned type, PINDEX fciSize)
{
  PAssert(type < 32, PInvalidParameter);
  theArray[compoundOffset] &= 0xe0;
  theArray[compoundOffset] |= type;
  SetPayloadSize(fciSize+8);
}


void RTP_ControlFrame::SetPayloadType(unsigned t)
{
  PAssert(t < 256, PInvalidParameter);
  theArray[compoundOffset+1] = (BYTE)t;
}

PINDEX RTP_ControlFrame::GetCompoundSize() const 
{ 
  // transmitted length is the offset of the last compound block
  // plus the compound length of the last block
  return compoundOffset + *(PUInt16b *)&theArray[compoundOffset+2]*4;
}

void RTP_ControlFrame::SetPayloadSize(PINDEX sz)
{
  payloadSize = sz;

  // compound size is in words, rounded up to nearest word
  PINDEX compoundSize = (payloadSize + 3) & ~3;
  PAssert(compoundSize <= 0xffff, PInvalidParameter);

  // make sure buffer is big enough for previous packets plus packet header plus payload
  SetMinSize(compoundOffset + 4 + 4*(compoundSize));

  // put the new compound size into the packet (always at offset 2)
  *(PUInt16b *)&theArray[compoundOffset+2] = (WORD)(compoundSize / 4);
}

BYTE * RTP_ControlFrame::GetPayloadPtr() const 
{ 
  // payload for current packet is always one DWORD after the current compound start
  if ((GetPayloadSize() == 0) || ((compoundOffset + 4) >= GetSize()))
    return NULL;
  return (BYTE *)(theArray + compoundOffset + 4); 
}

PBoolean RTP_ControlFrame::ReadNextPacket()
{
  // skip over current packet
  compoundOffset += GetPayloadSize() + 4;

  // see if another packet is feasible
  if (compoundOffset + 4 > GetSize())
    return false;

  // check if payload size for new packet is legal
  return compoundOffset + GetPayloadSize() + 4 <= GetSize();
}


PBoolean RTP_ControlFrame::StartNewPacket()
{
  // allocate storage for new packet header
  if (!SetMinSize(compoundOffset + 4))
    return false;

  theArray[compoundOffset] = '\x80'; // Set version 2
  theArray[compoundOffset+1] = 0;    // Set payload type to illegal
  theArray[compoundOffset+2] = 0;    // Set payload size to zero
  theArray[compoundOffset+3] = 0;

  // payload is now zero bytes
  payloadSize = 0;
  SetPayloadSize(payloadSize);

  return true;
}

void RTP_ControlFrame::EndPacket()
{
  // all packets must align to DWORD boundaries
  while (((4 + payloadSize) & 3) != 0) {
    theArray[compoundOffset + 4 + payloadSize - 1] = 0;
    ++payloadSize;
  }

  compoundOffset += 4 + payloadSize;
  payloadSize = 0;
}

void RTP_ControlFrame::StartSourceDescription(DWORD src)
{
  // extend payload to include SSRC + END
  SetPayloadSize(payloadSize + 4 + 1);  
  SetPayloadType(RTP_ControlFrame::e_SourceDescription);
  SetCount(GetCount()+1); // will be incremented automatically

  // get ptr to new item SDES
  BYTE * payload = GetPayloadPtr();
  *(PUInt32b *)payload = src;
  payload[4] = e_END;
}


void RTP_ControlFrame::AddSourceDescriptionItem(unsigned type, const PString & data)
{
  // get ptr to new item, remembering that END was inserted previously
  BYTE * payload = GetPayloadPtr() + payloadSize - 1;

  // length of new item
  PINDEX dataLength = data.GetLength();

  // add storage for new item (note that END has already been included)
  SetPayloadSize(payloadSize + 1 + 1 + dataLength);

  // insert new item
  payload[0] = (BYTE)type;
  payload[1] = (BYTE)dataLength;
  memcpy(payload+2, (const char *)data, dataLength);

  // insert new END
  payload[2+dataLength] = (BYTE)e_END;
}


void RTP_ControlFrame::ReceiverReport::SetLostPackets(unsigned packets)
{
  lost[0] = (BYTE)(packets >> 16);
  lost[1] = (BYTE)(packets >> 8);
  lost[2] = (BYTE)packets;
}


///////////////////////////////////////////////////////////////////////////////

#if OPAL_STATISTICS

OpalMediaStatistics::OpalMediaStatistics()
  : m_totalBytes(0)
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
  , m_jitterBufferDelay(0)

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

void RTP_UserData::OnTxStatistics(const RTP_Session & /*session*/) const
{
}


void RTP_UserData::OnRxStatistics(const RTP_Session & /*session*/) const
{
}

void RTP_UserData::SessionFailing(RTP_Session & /*session*/)
{
}

#if OPAL_VIDEO
void RTP_UserData::OnRxIntraFrameRequest(const RTP_Session & /*session*/) const
{
}

void RTP_UserData::OnTxIntraFrameRequest(const RTP_Session & /*session*/) const
{
}
#endif


/////////////////////////////////////////////////////////////////////////////

#if P_CONFIG_FILE
static PTimeInterval GetDefaultOutOfOrderWaitTime()
{
  static PTimeInterval ooowt(PConfig(PConfig::Environment).GetInteger("OPAL_RTP_OUT_OF_ORDER_TIME", 100));
  return ooowt;
}
#else
#define GetDefaultOutOfOrderWaitTime() (100)
#endif

RTP_Session::RTP_Session(const Params & params)
  : m_timeUnits(params.isAudio ? 8 : 90)
  , canonicalName(PProcess::Current().GetUserName())
  , toolName(PProcess::Current().GetName())
  , lastSRTimestamp(0)
  , lastSRReceiveTime(0)
  , outOfOrderWaitTime(GetDefaultOutOfOrderWaitTime())
  , firstPacketSent(0)
  , firstPacketReceived(0)
#if OPAL_RTCP_XR
  , m_metrics(NULL)
#endif
  , m_reportTimer(0, 12)  // Seconds
  , failed(false)
{
  PAssert(params.id > 0, PInvalidParameter);
  sessionID = params.id;
  isAudio = params.isAudio;

  userData = params.userData;
  autoDeleteUserData = params.autoDelete;

  ignorePayloadTypeChanges = true;
  syncSourceOut = PRandom::Number();

  timeStampOffs = 0;
  oobTimeStampBaseEstablished = false;
  lastSentPacketTime = PTimer::Tick();

  syncSourceIn = 0;
  allowAnySyncSource = true;
  allowOneSyncSourceChange = false;
  allowRemoteTransmitAddressChange = false;
  allowSequenceChange = false;
  txStatisticsInterval = 100;  // Number of data packets between tx reports
  rxStatisticsInterval = 100;  // Number of data packets between rx reports
  lastSentSequenceNumber = (WORD)PRandom::Number();
  expectedSequenceNumber = 0;
  lastRRSequenceNumber = 0;
  resequenceOutOfOrderPackets = true;
  senderReportsReceived = 0;
  consecutiveOutOfOrderPackets = 0;

  ClearStatistics();

  lastReceivedPayloadType = RTP_DataFrame::IllegalPayloadType;

  closeOnBye = false;
  byeSent    = false;

  lastSentTimestamp = 0;  // should be calculated, but we'll settle for initialising it

  m_encodingHandler = NULL;
  SetEncoding(params.encoding);

  m_reportTimer.SetNotifier(PCREATE_NOTIFIER(SendReport));
}


RTP_Session::~RTP_Session()
{
  m_reportTimer.Stop(true);

#if OPAL_RTCP_XR
  delete m_metrics;
#endif

#if PTRACING
  PTime now;
  int sentDuration = (now-firstPacketSent).GetSeconds();
  if (sentDuration == 0)
    sentDuration = 1;
  int receiveDuration = (now-firstPacketReceived).GetSeconds();
  if (receiveDuration == 0)
    receiveDuration = 1;
 #endif
  PTRACE_IF(3, packetsSent != 0 || packetsReceived != 0,
      "RTP\tSession " << sessionID << ", final statistics:\n"
      "    firstPacketSent    = " << firstPacketSent << "\n"
      "    packetsSent        = " << packetsSent << "\n"
      "    octetsSent         = " << octetsSent << "\n"
      "    bitRateSent        = " << (8*octetsSent/sentDuration) << "\n"
      "    averageSendTime    = " << averageSendTime << "\n"
      "    maximumSendTime    = " << maximumSendTime << "\n"
      "    minimumSendTime    = " << minimumSendTime << "\n"
      "    packetsLostByRemote= " << packetsLostByRemote << "\n"
      "    jitterLevelOnRemote= " << jitterLevelOnRemote << "\n"
      "    firstPacketReceived= " << firstPacketReceived << "\n"
      "    packetsReceived    = " << packetsReceived << "\n"
      "    octetsReceived     = " << octetsReceived << "\n"
      "    bitRateReceived    = " << (8*octetsReceived/receiveDuration) << "\n"
      "    packetsLost        = " << packetsLost << "\n"
      "    packetsTooLate     = " << GetPacketsTooLate() << "\n"
      "    packetOverruns     = " << GetPacketOverruns() << "\n"
      "    packetsOutOfOrder  = " << packetsOutOfOrder << "\n"
      "    averageReceiveTime = " << averageReceiveTime << "\n"
      "    maximumReceiveTime = " << maximumReceiveTime << "\n"
      "    minimumReceiveTime = " << minimumReceiveTime << "\n"
      "    averageJitter      = " << GetAvgJitterTime() << "\n"
      "    maximumJitter      = " << GetMaxJitterTime()
   );
  if (autoDeleteUserData)
    delete userData;
  delete m_encodingHandler;
}


void RTP_Session::ClearStatistics()
{
  firstPacketSent.SetTimestamp(0);
  packetsSent = 0;
  rtcpPacketsSent = 0;
  octetsSent = 0;
  firstPacketReceived.SetTimestamp(0);
  packetsReceived = 0;
  octetsReceived = 0;
  packetsLost = 0;
  packetsLostByRemote = 0;
  packetsOutOfOrder = 0;
  averageSendTime = 0;
  maximumSendTime = 0;
  minimumSendTime = 0;
  averageReceiveTime = 0;
  maximumReceiveTime = 0;
  minimumReceiveTime = 0;
  jitterLevel = 0;
  maximumJitterLevel = 0;
  jitterLevelOnRemote = 0;
  markerRecvCount = 0;
  markerSendCount = 0;

  txStatisticsCount = 0;
  rxStatisticsCount = 0;
  averageSendTimeAccum = 0;
  maximumSendTimeAccum = 0;
  minimumSendTimeAccum = 0xffffffff;
  averageReceiveTimeAccum = 0;
  maximumReceiveTimeAccum = 0;
  minimumReceiveTimeAccum = 0xffffffff;
  packetsLostSinceLastRR = 0;
  lastTransitTime = 0;
}


void RTP_Session::SendBYE()
{
  {
    PWaitAndSignal mutex(dataMutex);
    if (byeSent)
      return;

    byeSent = true;
  }

  RTP_ControlFrame report;
  InsertReportPacket(report);

  static char const ReasonStr[] = "Session ended";
  static size_t ReasonLen = sizeof(ReasonStr);

  // insert BYE
  report.StartNewPacket();
  report.SetPayloadType(RTP_ControlFrame::e_Goodbye);
  report.SetPayloadSize(4+1+ReasonLen);  // length is SSRC + ReasonLen + reason

  BYTE * payload = report.GetPayloadPtr();

  // one SSRC
  report.SetCount(1);
  *(PUInt32b *)payload = syncSourceOut;

  // insert reason
  payload[4] = (BYTE)ReasonLen;
  memcpy((char *)(payload+5), ReasonStr, ReasonLen);

  report.EndPacket();
  WriteControl(report);
}

PString RTP_Session::GetCanonicalName() const
{
  PWaitAndSignal mutex(m_reportMutex);
  PString s = canonicalName;
  s.MakeUnique();
  return s;
}


void RTP_Session::SetCanonicalName(const PString & name)
{
  PWaitAndSignal mutex(m_reportMutex);
  canonicalName = name;
  canonicalName.MakeUnique();
}


PString RTP_Session::GetToolName() const
{
  PWaitAndSignal mutex(m_reportMutex);
  PString s = toolName;
  s.MakeUnique();
  return s;
}


void RTP_Session::SetToolName(const PString & name)
{
  PWaitAndSignal mutex(m_reportMutex);
  toolName = name;
  toolName.MakeUnique();
}


void RTP_Session::SetUserData(RTP_UserData * data, PBoolean autoDelete)
{
  if (autoDeleteUserData)
    delete userData;
  userData = data;
  autoDeleteUserData = autoDelete;
}


void RTP_Session::SetJitterBufferSize(unsigned minJitterDelay,
                                      unsigned maxJitterDelay,
                                      unsigned timeUnits,
                                        PINDEX packetSize)
{
  if (timeUnits > 0)
    m_timeUnits = timeUnits;

  if (minJitterDelay == 0 && maxJitterDelay == 0) {
    PTRACE_IF(4, m_jitterBuffer != NULL, "RTP\tSwitching off jitter buffer " << *m_jitterBuffer);
    m_jitterBuffer.SetNULL();
  }
  else {
    resequenceOutOfOrderPackets = false;
    FlushData();
    if (m_jitterBuffer != NULL) {
      PTRACE(4, "RTP\tSetting jitter buffer time from " << minJitterDelay << " to " << maxJitterDelay);
      m_jitterBuffer->SetDelay(minJitterDelay, maxJitterDelay, packetSize);
    }
    else {
      m_jitterBuffer = new RTP_JitterBuffer(*this, minJitterDelay, maxJitterDelay, m_timeUnits, packetSize);
      PTRACE(4, "RTP\tCreated RTP jitter buffer " << *m_jitterBuffer);
    }
  }
}


unsigned RTP_Session::GetJitterBufferSize() const
{
  JitterBufferPtr jitter = m_jitterBuffer; // Increase reference count
  return jitter != NULL ? jitter->GetCurrentJitterDelay() : 0;
}


PBoolean RTP_Session::ReadBufferedData(RTP_DataFrame & frame)
{
  JitterBufferPtr jitter = m_jitterBuffer; // Increase reference count
  if (jitter != NULL)
    return jitter->ReadData(frame);

  if (m_outOfOrderPackets.empty())
    return ReadData(frame);

  unsigned sequenceNumber = m_outOfOrderPackets.back().GetSequenceNumber();
  if (sequenceNumber != expectedSequenceNumber) {
    PTRACE(5, "RTP\tSession " << sessionID << ", ssrc=" << syncSourceIn
           << ", still out of order packets, next "
           << sequenceNumber << " expected " << expectedSequenceNumber);
    return ReadData(frame);
  }

  frame = m_outOfOrderPackets.back();
  m_outOfOrderPackets.pop_back();
  expectedSequenceNumber = (WORD)(sequenceNumber + 1);

  PTRACE(5, "RTP\tSession " << sessionID << ", ssrc=" << syncSourceIn << ", resequenced "
         << (m_outOfOrderPackets.empty() ? "last" : "next") << " out of order packet " << sequenceNumber);
  return true;
}


void RTP_Session::FlushData()
{
}


void RTP_Session::SetTxStatisticsInterval(unsigned packets)
{
  txStatisticsInterval = PMAX(packets, 2);
  txStatisticsCount = 0;
  averageSendTimeAccum = 0;
  maximumSendTimeAccum = 0;
  minimumSendTimeAccum = 0xffffffff;
}


void RTP_Session::SetRxStatisticsInterval(unsigned packets)
{
  rxStatisticsInterval = PMAX(packets, 2);
  rxStatisticsCount = 0;
  averageReceiveTimeAccum = 0;
  maximumReceiveTimeAccum = 0;
  minimumReceiveTimeAccum = 0xffffffff;
}


void RTP_Session::AddReceiverReport(RTP_ControlFrame::ReceiverReport & receiver)
{
  receiver.ssrc = syncSourceIn;
  receiver.SetLostPackets(GetPacketsLost()+GetPacketsTooLate());

  if (expectedSequenceNumber > lastRRSequenceNumber)
    receiver.fraction = (BYTE)((packetsLostSinceLastRR<<8)/(expectedSequenceNumber - lastRRSequenceNumber));
  else
    receiver.fraction = 0;
  packetsLostSinceLastRR = 0;

  receiver.last_seq = lastRRSequenceNumber;
  lastRRSequenceNumber = expectedSequenceNumber;

  receiver.jitter = jitterLevel >> JitterRoundingGuardBits; // Allow for rounding protection bits
  
  if (senderReportsReceived > 0) {
    // Calculate the last SR timestamp
    PUInt32b lsr_ntp_sec  = (DWORD)(lastSRTimestamp.GetTimeInSeconds()+SecondsFrom1900to1970); // Convert from 1970 to 1900
    PUInt32b lsr_ntp_frac = lastSRTimestamp.GetMicrosecond()*4294; // Scale microseconds to "fraction" from 0 to 2^32
    receiver.lsr = (((lsr_ntp_sec << 16) & 0xFFFF0000) | ((lsr_ntp_frac >> 16) & 0x0000FFFF));
  
    // Calculate the delay since last SR
    PTime now;
    delaySinceLastSR = now - lastSRReceiveTime;
    receiver.dlsr = (DWORD)(delaySinceLastSR.GetMilliSeconds()*65536/1000);
  }
  else {
    receiver.lsr = 0;
    receiver.dlsr = 0;
  }
  
  PTRACE(3, "RTP\tSession " << sessionID << ", SentReceiverReport:"
            " ssrc=" << receiver.ssrc
         << " fraction=" << (unsigned)receiver.fraction
         << " lost=" << receiver.GetLostPackets()
         << " last_seq=" << receiver.last_seq
         << " jitter=" << receiver.jitter
         << " lsr=" << receiver.lsr
         << " dlsr=" << receiver.dlsr);
} 


RTP_Session::SendReceiveStatus RTP_Session::OnSendData(RTP_DataFrame & frame)
{
  return EncodingLock(*this)->OnSendData(frame);
}


RTP_Session::SendReceiveStatus RTP_Session::Internal_OnSendData(RTP_DataFrame & frame)
{
  PWaitAndSignal mutex(dataMutex);

  PTimeInterval tick = PTimer::Tick();  // Timestamp set now

  frame.SetSequenceNumber(++lastSentSequenceNumber);
  frame.SetSyncSource(syncSourceOut);

  // special handling for first packet
  if (packetsSent == 0) {

    // establish timestamp offset
    if (oobTimeStampBaseEstablished)  {
      timeStampOffs = oobTimeStampOutBase - frame.GetTimestamp() + ((PTimer::Tick() - oobTimeStampBase).GetInterval() * 8);
      frame.SetTimestamp(frame.GetTimestamp() + timeStampOffs);
    }
    else {
      oobTimeStampBaseEstablished = true;
      timeStampOffs               = 0;
      oobTimeStampOutBase         = frame.GetTimestamp();
      oobTimeStampBase            = PTimer::Tick();
    }

    firstPacketSent.SetCurrentTime();

    // display stuff
    PTRACE(3, "RTP\tSession " << sessionID << ", first sent data:"
              " ver=" << frame.GetVersion()
           << " pt=" << frame.GetPayloadType()
           << " psz=" << frame.GetPayloadSize()
           << " m=" << frame.GetMarker()
           << " x=" << frame.GetExtension()
           << " seq=" << frame.GetSequenceNumber()
           << " ts=" << frame.GetTimestamp()
           << " src=" << hex << frame.GetSyncSource()
           << " ccnt=" << frame.GetContribSrcCount() << dec);
  }

  else {
    // set timestamp
    DWORD ts = frame.GetTimestamp() + timeStampOffs;
    frame.SetTimestamp(ts);

    // reset OOB timestamp every marker bit
    if (frame.GetMarker()) {
      oobTimeStampOutBase = ts;
      oobTimeStampBase    = PTimer::Tick();
    }

    // Only do statistics on subsequent packets
    if ( ! (isAudio && frame.GetMarker()) ) {
      DWORD diff = (tick - lastSentPacketTime).GetInterval();

      averageSendTimeAccum += diff;
      if (diff > maximumSendTimeAccum)
        maximumSendTimeAccum = diff;
      if (diff < minimumSendTimeAccum)
        minimumSendTimeAccum = diff;
      txStatisticsCount++;
    }
  }

  lastSentPacketTime = tick;

  octetsSent += frame.GetPayloadSize();
  packetsSent++;

  if (frame.GetMarker())
    markerSendCount++;

  // Call the statistics call-back on the first PDU with total count == 1
  if (packetsSent == 1 && userData != NULL)
    userData->OnTxStatistics(*this);

  if (txStatisticsCount < txStatisticsInterval)
    return e_ProcessPacket;

  txStatisticsCount = 0;

  averageSendTime = averageSendTimeAccum/txStatisticsInterval;
  maximumSendTime = maximumSendTimeAccum;
  minimumSendTime = minimumSendTimeAccum;

  averageSendTimeAccum = 0;
  maximumSendTimeAccum = 0;
  minimumSendTimeAccum = 0xffffffff;

  PTRACE(3, "RTP\tSession " << sessionID << ", transmit statistics: "
   " packets=" << packetsSent <<
   " octets=" << octetsSent <<
   " avgTime=" << averageSendTime <<
   " maxTime=" << maximumSendTime <<
   " minTime=" << minimumSendTime
  );

  if (userData != NULL)
    userData->OnTxStatistics(*this);

  return e_ProcessPacket;
}

RTP_Session::SendReceiveStatus RTP_Session::OnSendControl(RTP_ControlFrame & frame, PINDEX & len)
{
  return EncodingLock(*this)->OnSendControl(frame, len);
}

#if OPAL_VIDEO
RTP_Session::SendReceiveStatus RTP_Session::Internal_OnSendControl(RTP_ControlFrame & frame, PINDEX & /*len*/)
{
  rtcpPacketsSent++;

  if(frame.GetPayloadType() == RTP_ControlFrame::e_IntraFrameRequest && userData != NULL)
    userData->OnTxIntraFrameRequest(*this);

  return e_ProcessPacket;
}
#else
RTP_Session::SendReceiveStatus RTP_Session::Internal_OnSendControl(RTP_ControlFrame & /*frame*/, PINDEX & /*len*/)
{
  rtcpPacketsSent++;
  return e_ProcessPacket;
}
#endif


RTP_Session::SendReceiveStatus RTP_Session::OnReceiveData(RTP_DataFrame & frame)
{
  return EncodingLock(*this)->OnReceiveData(frame);
}

RTP_Session::SendReceiveStatus RTP_Session::Internal_OnReceiveData(RTP_DataFrame & frame)
{
  // Check that the PDU is the right version
  if (frame.GetVersion() != RTP_DataFrame::ProtocolVersion)
    return e_IgnorePacket; // Non fatal error, just ignore

  // Check if expected payload type
  if (lastReceivedPayloadType == RTP_DataFrame::IllegalPayloadType)
    lastReceivedPayloadType = frame.GetPayloadType();

  if (lastReceivedPayloadType != frame.GetPayloadType() && !ignorePayloadTypeChanges) {

    PTRACE(4, "RTP\tSession " << sessionID << ", got payload type "
           << frame.GetPayloadType() << ", but was expecting " << lastReceivedPayloadType);
    return e_IgnorePacket;
  }

  // Check for if a control packet rather than data packet.
  if (frame.GetPayloadType() > RTP_DataFrame::MaxPayloadType)
    return e_IgnorePacket; // Non fatal error, just ignore

  PTimeInterval tick = PTimer::Tick();  // Get timestamp now

  // Have not got SSRC yet, so grab it now
  if (syncSourceIn == 0)
    syncSourceIn = frame.GetSyncSource();

  // Check packet sequence numbers
  if (packetsReceived == 0) {
    firstPacketReceived.SetCurrentTime();
    PTRACE(3, "RTP\tSession " << sessionID << ", first receive data:"
              " ver=" << frame.GetVersion()
           << " pt=" << frame.GetPayloadType()
           << " psz=" << frame.GetPayloadSize()
           << " m=" << frame.GetMarker()
           << " x=" << frame.GetExtension()
           << " seq=" << frame.GetSequenceNumber()
           << " ts=" << frame.GetTimestamp()
           << " src=" << hex << frame.GetSyncSource()
           << " ccnt=" << frame.GetContribSrcCount() << dec);

#if OPAL_RTCP_XR
    delete m_metrics; // Should be NULL, but just in case ...
    m_metrics = RTCP_XR_Metrics::Create(frame);
#endif

    if ((frame.GetPayloadType() == RTP_DataFrame::T38) &&
        (frame.GetSequenceNumber() >= 0x8000) &&
         (frame.GetPayloadSize() == 0)) {
      PTRACE(4, "RTP\tSession " << sessionID << ", ignoring left over audio packet from switch to T.38");
      return e_IgnorePacket; // Non fatal error, just ignore
    }

    expectedSequenceNumber = (WORD)(frame.GetSequenceNumber() + 1);
  }
  else {
    if (frame.GetSyncSource() != syncSourceIn) {
      if (allowAnySyncSource) {
        PTRACE(2, "RTP\tSession " << sessionID << ", SSRC changed from " << hex << frame.GetSyncSource() << " to " << syncSourceIn << dec);
        syncSourceIn = frame.GetSyncSource();
        allowSequenceChange = true;
      } 
      else if (allowOneSyncSourceChange) {
        PTRACE(2, "RTP\tSession " << sessionID << ", allowed one SSRC change from SSRC=" << hex << syncSourceIn << " to =" << dec << frame.GetSyncSource() << dec);
        syncSourceIn = frame.GetSyncSource();
        allowSequenceChange = true;
        allowOneSyncSourceChange = false;
      }
      else {
        PTRACE(2, "RTP\tSession " << sessionID << ", packet from SSRC=" << hex << frame.GetSyncSource() << " ignored, expecting SSRC=" << syncSourceIn << dec);
        return e_IgnorePacket; // Non fatal error, just ignore
      }
    }

    WORD sequenceNumber = frame.GetSequenceNumber();
    if (sequenceNumber == expectedSequenceNumber) {
      expectedSequenceNumber++;
      consecutiveOutOfOrderPackets = 0;

      if (!m_outOfOrderPackets.empty()) {
        PTRACE(5, "RTP\tSession " << sessionID << ", ssrc=" << syncSourceIn
               << ", received out of order packet " << sequenceNumber);
        outOfOrderPacketTime = tick;
        packetsOutOfOrder++;
      }

      /* For audio we do not do statistics on start of talk burst (marker bit)
         as that could be a substantial time and is not really "jitter".
         For video we measure jitter between whoile frames which is indicated
         by the marker bit being on. */
      if (frame.GetMarker() != isAudio) {
        DWORD diff = (tick - lastReceivedPacketTime).GetInterval();

        averageReceiveTimeAccum += diff;
        if (diff > maximumReceiveTimeAccum)
          maximumReceiveTimeAccum = diff;
        if (diff < minimumReceiveTimeAccum)
          minimumReceiveTimeAccum = diff;
        rxStatisticsCount++;

        // As per RFC3550 Appendix 8
        diff *= GetJitterTimeUnits(); // Convert to timestamp units
        long variance = diff - lastTransitTime;
        lastTransitTime = diff;
        if (variance < 0)
          variance = -variance;
        jitterLevel += variance - ((jitterLevel+(1<<(JitterRoundingGuardBits-1))) >> JitterRoundingGuardBits);
        if (jitterLevel > maximumJitterLevel)
          maximumJitterLevel = jitterLevel;
      }

      if (frame.GetMarker())
        markerRecvCount++;
    }
    else if (allowSequenceChange) {
      expectedSequenceNumber = (WORD) (sequenceNumber + 1);
      allowSequenceChange = false;
      m_outOfOrderPackets.clear();
      PTRACE(2, "RTP\tSession " << sessionID << ", ssrc=" << syncSourceIn
             << ", adjusting sequence numbers to expect " << expectedSequenceNumber);
    }
    else if (sequenceNumber < expectedSequenceNumber) {
#if OPAL_RTCP_XR
      if (m_metrics != NULL) m_metrics->OnPacketDiscarded();
#endif

      // Check for Cisco bug where sequence numbers suddenly start incrementing
      // from a different base.
      if (++consecutiveOutOfOrderPackets > 10) {
        expectedSequenceNumber = (WORD)(sequenceNumber + 1);
        PTRACE(2, "RTP\tSession " << sessionID << ", ssrc=" << syncSourceIn
               << ", abnormal change of sequence numbers, adjusting to expect " << expectedSequenceNumber);
      }
      else {
        PTRACE(2, "RTP\tSession " << sessionID << ", ssrc=" << syncSourceIn
               << ", incorrect sequence, got " << sequenceNumber << " expected " << expectedSequenceNumber);

        if (resequenceOutOfOrderPackets)
          return e_IgnorePacket; // Non fatal error, just ignore

        packetsOutOfOrder++;
      }
    }
    else if (resequenceOutOfOrderPackets &&
                (m_outOfOrderPackets.empty() || (tick - outOfOrderPacketTime) < outOfOrderWaitTime)) {
      if (m_outOfOrderPackets.empty())
        outOfOrderPacketTime = tick;
      // Maybe packet lost, maybe out of order, save for now
      SaveOutOfOrderPacket(frame);
      return e_IgnorePacket;
    }
    else {
      if (!m_outOfOrderPackets.empty()) {
        // Give up on the packet, probably never coming in. Save current and switch in
        // the lowest numbered packet.
        SaveOutOfOrderPacket(frame);

        for (;;) {
          if (m_outOfOrderPackets.empty())
            return e_IgnorePacket;

          frame = m_outOfOrderPackets.back();
          m_outOfOrderPackets.pop_back();

          sequenceNumber = frame.GetSequenceNumber();
          if (sequenceNumber >= expectedSequenceNumber)
            break;

          PTRACE(2, "RTP\tSession " << sessionID << ", ssrc=" << syncSourceIn
                 << ", incorrect sequence after re-ordering, got "
                 << sequenceNumber << " expected " << expectedSequenceNumber);
        }

        outOfOrderPacketTime = tick;
      }

      unsigned dropped = sequenceNumber - expectedSequenceNumber;
      packetsLost += dropped;
      packetsLostSinceLastRR += dropped;
      PTRACE(2, "RTP\tSession " << sessionID << ", ssrc=" << syncSourceIn
             << ", " << dropped << " packet(s) missing at " << sequenceNumber);
      expectedSequenceNumber = (WORD)(sequenceNumber + 1);
      consecutiveOutOfOrderPackets = 0;
#if OPAL_RTCP_XR
      if (m_metrics != NULL) m_metrics->OnPacketLost(dropped);
#endif
    }
  }

  lastReceivedPacketTime = tick;

  octetsReceived += frame.GetPayloadSize();
  packetsReceived++;
  
#if OPAL_RTCP_XR
  if (m_metrics != NULL) m_metrics->OnPacketReceived();
#endif

  // Call the statistics call-back on the first PDU with total count == 1
  if (packetsReceived == 1 && userData != NULL)
    userData->OnRxStatistics(*this);

  if (rxStatisticsCount >= rxStatisticsInterval) {

    rxStatisticsCount = 0;

    averageReceiveTime = averageReceiveTimeAccum/rxStatisticsInterval;
    maximumReceiveTime = maximumReceiveTimeAccum;
    minimumReceiveTime = minimumReceiveTimeAccum;

    averageReceiveTimeAccum = 0;
    maximumReceiveTimeAccum = 0;
    minimumReceiveTimeAccum = 0xffffffff;

    PTRACE(4, "RTP\tSession " << sessionID << ", receive statistics:"
              " packets=" << packetsReceived <<
              " octets=" << octetsReceived <<
              " lost=" << packetsLost <<
              " tooLate=" << GetPacketsTooLate() <<
              " order=" << packetsOutOfOrder <<
              " avgTime=" << averageReceiveTime <<
              " maxTime=" << maximumReceiveTime <<
              " minTime=" << minimumReceiveTime <<
              " jitter=" << GetAvgJitterTime() <<
              " maxJitter=" << GetMaxJitterTime());

    if (userData != NULL)
      userData->OnRxStatistics(*this);
  }

  SendReceiveStatus status = e_ProcessPacket;
  for (list<FilterNotifier>::iterator filter = m_filters.begin(); filter != m_filters.end(); ++filter) 
    (*filter)(frame, status);

  return status;
}


void RTP_Session::SaveOutOfOrderPacket(RTP_DataFrame & frame)
{
  WORD sequenceNumber = frame.GetSequenceNumber();

  PTRACE(m_outOfOrderPackets.empty() ? 2 : 5,
         "RTP\tSession " << sessionID << ", ssrc=" << syncSourceIn << ", "
         << (m_outOfOrderPackets.empty() ? "first" : "next") << " out of order packet, got "
         << sequenceNumber << " expected " << expectedSequenceNumber);

  std::list<RTP_DataFrame>::iterator it;
  for (it  = m_outOfOrderPackets.begin(); it != m_outOfOrderPackets.end(); ++it) {
    if (sequenceNumber > it->GetSequenceNumber())
      break;
  }

  m_outOfOrderPackets.insert(it, frame);
  frame.MakeUnique();
}


PBoolean RTP_Session::InsertReportPacket(RTP_ControlFrame & report)
{
  report.StartNewPacket();

  // No packets sent yet, so only set RR
  if (packetsSent == 0) {

    // Send RR as we are not transmitting
    report.SetPayloadType(RTP_ControlFrame::e_ReceiverReport);

    // if no packets received, put in an empty report
    if (packetsReceived == 0) {
      report.SetPayloadSize(sizeof(PUInt32b));  // length is SSRC 
      report.SetCount(0);

      // add the SSRC to the start of the payload
      *(PUInt32b *)report.GetPayloadPtr() = syncSourceOut;
    }
    else {
      report.SetPayloadSize(sizeof(PUInt32b) + sizeof(RTP_ControlFrame::ReceiverReport));  // length is SSRC of packet sender plus RR
      report.SetCount(1);
      BYTE * payload = report.GetPayloadPtr();

      // add the SSRC to the start of the payload
      *(PUInt32b *)payload = syncSourceOut;

      // add the RR after the SSRC
      AddReceiverReport(*(RTP_ControlFrame::ReceiverReport *)(payload+sizeof(PUInt32b)));
    }
  }
  else {
    // send SR and RR
    report.SetPayloadType(RTP_ControlFrame::e_SenderReport);
    report.SetPayloadSize(sizeof(PUInt32b) + sizeof(RTP_ControlFrame::SenderReport));  // length is SSRC of packet sender plus SR
    report.SetCount(0);
    BYTE * payload = report.GetPayloadPtr();

    // add the SSRC to the start of the payload
    *(PUInt32b *)payload = syncSourceOut;

    // add the SR after the SSRC
    RTP_ControlFrame::SenderReport * sender = (RTP_ControlFrame::SenderReport *)(payload+sizeof(PUInt32b));
    PTime now;
    sender->ntp_sec  = (DWORD)(now.GetTimeInSeconds()+SecondsFrom1900to1970); // Convert from 1970 to 1900
    sender->ntp_frac = now.GetMicrosecond()*4294; // Scale microseconds to "fraction" from 0 to 2^32
    sender->rtp_ts   = lastSentTimestamp;
    sender->psent    = packetsSent;
    sender->osent    = octetsSent;

    PTRACE(3, "RTP\tSession " << sessionID << ", SentSenderReport:"
              " ssrc=" << syncSourceOut
           << " ntp=" << sender->ntp_sec << '.' << sender->ntp_frac
           << " rtp=" << sender->rtp_ts
           << " psent=" << sender->psent
           << " osent=" << sender->osent);

    if (syncSourceIn != 0) {
      report.SetPayloadSize(sizeof(PUInt32b) + sizeof(RTP_ControlFrame::SenderReport) + sizeof(RTP_ControlFrame::ReceiverReport));
      report.SetCount(1);
      AddReceiverReport(*(RTP_ControlFrame::ReceiverReport *)(payload+sizeof(PUInt32b)+sizeof(RTP_ControlFrame::SenderReport)));
    }
  }

  report.EndPacket();
  return true;
}


void RTP_Session::SendReport(PTimer&, INT)
{
  PWaitAndSignal mutex(m_reportMutex);

  // Have not got anything yet, do nothing
  if (packetsSent == 0 && packetsReceived == 0)
    return;

  RTP_ControlFrame report;
  InsertReportPacket(report);

  // Add the SDES part to compound RTCP packet
  PTRACE(3, "RTP\tSession " << sessionID << ", sending SDES: " << canonicalName);
  report.StartNewPacket();

  report.SetCount(0); // will be incremented automatically
  report.StartSourceDescription  (syncSourceOut);
  report.AddSourceDescriptionItem(RTP_ControlFrame::e_CNAME, canonicalName);
  report.AddSourceDescriptionItem(RTP_ControlFrame::e_TOOL, toolName);
  report.EndPacket();
  
#if OPAL_RTCP_XR
  //Generate and send RTCP-XR packet
  if (m_metrics != NULL) m_metrics->InsertExtendedReportPacket(sessionID, syncSourceOut, m_jitterBuffer, report);
#endif

  WriteControl(report);
}


#if OPAL_STATISTICS
void RTP_Session::GetStatistics(OpalMediaStatistics & statistics, bool receiver) const
{
  statistics.m_totalBytes        = receiver ? GetOctetsReceived()     : GetOctetsSent();
  statistics.m_totalPackets      = receiver ? GetPacketsReceived()    : GetPacketsSent();
  statistics.m_packetsLost       = receiver ? GetPacketsLost()        : GetPacketsLostByRemote();
  statistics.m_packetsOutOfOrder = receiver ? GetPacketsOutOfOrder()  : 0;
  statistics.m_packetsTooLate    = receiver ? GetPacketsTooLate()     : 0;
  statistics.m_packetOverruns    = receiver ? GetPacketOverruns()     : 0;
  statistics.m_minimumPacketTime = receiver ? GetMinimumReceiveTime() : GetMinimumSendTime();
  statistics.m_averagePacketTime = receiver ? GetAverageReceiveTime() : GetAverageSendTime();
  statistics.m_maximumPacketTime = receiver ? GetMaximumReceiveTime() : GetMaximumSendTime();
  statistics.m_averageJitter     = receiver ? GetAvgJitterTime()      : GetJitterTimeOnRemote();
  statistics.m_maximumJitter     = receiver ? GetMaxJitterTime()      : 0;
  statistics.m_jitterBufferDelay = receiver ? GetJitterBufferDelay()  : 0;
}
#endif


RTP_Session::ReceiverReportArray
RTP_Session::BuildReceiverReportArray(const RTP_ControlFrame & frame, PINDEX offset)
{
  RTP_Session::ReceiverReportArray reports;

  const RTP_ControlFrame::ReceiverReport * rr = (const RTP_ControlFrame::ReceiverReport *)(frame.GetPayloadPtr()+offset);
  for (PINDEX repIdx = 0; repIdx < (PINDEX)frame.GetCount(); repIdx++) {
    RTP_Session::ReceiverReport * report = new RTP_Session::ReceiverReport;
    report->sourceIdentifier = rr->ssrc;
    report->fractionLost = rr->fraction;
    report->totalLost = rr->GetLostPackets();
    report->lastSequenceNumber = rr->last_seq;
    report->jitter = rr->jitter;
    report->lastTimestamp = (PInt64)(DWORD)rr->lsr;
    report->delay = ((PInt64)rr->dlsr << 16)/1000;
    reports.SetAt(repIdx, report);
#if OPAL_RTCP_XR
    if (m_metrics != NULL) m_metrics->OnRxSenderReport(rr->lsr, rr->dlsr);
#endif
    rr++;
  }

  return reports;
}


RTP_Session::SendReceiveStatus RTP_Session::OnReceiveControl(RTP_ControlFrame & frame)
{
  do {
    BYTE * payload = frame.GetPayloadPtr();
    PINDEX size = frame.GetPayloadSize(); 
    if ((payload == NULL) || (size == 0) || ((payload + size) > (frame.GetPointer() + frame.GetSize()))){
      /* TODO: 1.shall we test for a maximum size ? Indeed but what's the value ? *
               2. what's the correct exit status ? */
      PTRACE(2, "RTP\tSession " << sessionID << ", OnReceiveControl invalid frame");
      break;
    }

    switch (frame.GetPayloadType()) {
      case RTP_ControlFrame::e_SenderReport :
        if (size >= (PINDEX)(sizeof(PUInt32b)+sizeof(RTP_ControlFrame::SenderReport)+frame.GetCount()*sizeof(RTP_ControlFrame::ReceiverReport))) {
          SenderReport sender;
          sender.sourceIdentifier = *(const PUInt32b *)payload;
          const RTP_ControlFrame::SenderReport & sr = *(const RTP_ControlFrame::SenderReport *)(payload+sizeof(PUInt32b));
          sender.realTimestamp = PTime(sr.ntp_sec-SecondsFrom1900to1970, sr.ntp_frac/4294);

          // Save the receive time
          lastSRTimestamp = sender.realTimestamp;
          lastSRReceiveTime.SetCurrentTime();
          senderReportsReceived++;

          sender.rtpTimestamp = sr.rtp_ts;
          sender.packetsSent = sr.psent;
          sender.octetsSent = sr.osent;

          OnRxSenderReport(sender, BuildReceiverReportArray(frame, sizeof(PUInt32b)+sizeof(RTP_ControlFrame::SenderReport)));
        }
        else {
          PTRACE(2, "RTP\tSession " << sessionID << ", SenderReport packet truncated");
        }
        break;

      case RTP_ControlFrame::e_ReceiverReport :
        if (size >= (PINDEX)(sizeof(PUInt32b)+frame.GetCount()*sizeof(RTP_ControlFrame::ReceiverReport)))
          OnRxReceiverReport(*(const PUInt32b *)payload, BuildReceiverReportArray(frame, sizeof(PUInt32b)));
        else {
          PTRACE(2, "RTP\tSession " << sessionID << ", ReceiverReport packet truncated");
        }
        break;

      case RTP_ControlFrame::e_SourceDescription :
        if (size >= (PINDEX)(frame.GetCount()*sizeof(RTP_ControlFrame::SourceDescription))) {
          SourceDescriptionArray descriptions;
          const RTP_ControlFrame::SourceDescription * sdes = (const RTP_ControlFrame::SourceDescription *)payload;
          PINDEX srcIdx;
          for (srcIdx = 0; srcIdx < (PINDEX)frame.GetCount(); srcIdx++) {
            descriptions.SetAt(srcIdx, new SourceDescription(sdes->src));
            const RTP_ControlFrame::SourceDescription::Item * item = sdes->item;
            PINDEX uiSizeCurrent = 0;   /* current size of the items already parsed */
            while ((item != NULL) && (item->type != RTP_ControlFrame::e_END)) {
              descriptions[srcIdx].items.SetAt(item->type, PString(item->data, item->length));
              uiSizeCurrent += item->GetLengthTotal();
              PTRACE(4,"RTP\tSession " << sessionID << ", SourceDescription item " << item << ", current size = " << uiSizeCurrent);
              
              /* avoid reading where GetNextItem() shall not */
              if (uiSizeCurrent >= size){
                PTRACE(4,"RTP\tSession " << sessionID << ", SourceDescription end of items");
                item = NULL;
                break;
              }

              item = item->GetNextItem();
            }

            /* RTP_ControlFrame::e_END doesn't have a length field, so do NOT call item->GetNextItem()
               otherwise it reads over the buffer */
            if (item == NULL || 
                item->type == RTP_ControlFrame::e_END || 
                (sdes = (const RTP_ControlFrame::SourceDescription *)item->GetNextItem()) == NULL)
              break;
          }
          OnRxSourceDescription(descriptions);
        }
        else {
          PTRACE(2, "RTP\tSession " << sessionID << ", SourceDescription packet truncated");
        }
        break;

      case RTP_ControlFrame::e_Goodbye :
        if (size >= 4) {
          PString str;
          PINDEX count = frame.GetCount()*4;
    
          if (size > count) {
            if (size >= (PINDEX)(payload[count] + sizeof(DWORD) /*SSRC*/ + sizeof(unsigned char) /* length */))
              str = PString((const char *)(payload+count+1), payload[count]);
            else {
              PTRACE(2, "RTP\tSession " << sessionID << ", Goodbye packet invalid");
            }
          }

          PDWORDArray sources(count);
          for (PINDEX i = 0; i < count; i++)
            sources[i] = ((const PUInt32b *)payload)[i];
          OnRxGoodbye(sources, str);
        }
        else {
          PTRACE(2, "RTP\tSession " << sessionID << ", Goodbye packet truncated");
        }

        if (closeOnBye) {
          PTRACE(3, "RTP\tSession " << sessionID << ", Goodbye packet closing transport");
          return e_AbortTransport;
        }
        break;

      case RTP_ControlFrame::e_ApplDefined :
        if (size >= 4) {
          PString str((const char *)(payload+4), 4);
          OnRxApplDefined(str, frame.GetCount(), *(const PUInt32b *)payload,
          payload+8, frame.GetPayloadSize()-8);
        }
        else {
          PTRACE(2, "RTP\tSession " << sessionID << ", ApplDefined packet truncated");
        }
        break;

#if OPAL_RTCP_XR
      case RTP_ControlFrame::e_ExtendedReport :
        if (size >= (PINDEX)(sizeof(PUInt32b)+frame.GetCount()*sizeof(RTP_ControlFrame::ExtendedReport)))
          OnRxExtendedReport(*(const PUInt32b *)payload, RTCP_XR_Metrics::BuildExtendedReportArray(frame, sizeof(PUInt32b)));
        else {
          PTRACE(2, "RTP\tSession " << sessionID << ", ReceiverReport packet truncated");
        }
        break;
#endif

  #if OPAL_VIDEO
      case RTP_ControlFrame::e_IntraFrameRequest :
        PTRACE(4, "RTP\tSession " << sessionID << ", received RFC2032 FIR");
        if(userData != NULL)
          userData->OnRxIntraFrameRequest(*this);
        break;

      case RTP_ControlFrame::e_PayloadSpecificFeedBack :
        switch (frame.GetFbType()) {
          case RTP_ControlFrame::e_PictureLossIndication :
            PTRACE(4, "RTP\tSession " << sessionID << ", received RFC5104 PLI");
            if(userData != NULL)
              userData->OnRxIntraFrameRequest(*this);
            break;

          case RTP_ControlFrame::e_FullIntraRequest :
            PTRACE(4, "RTP\tSession " << sessionID << ", received RFC5104 FIR");
            if(userData != NULL)
              userData->OnRxIntraFrameRequest(*this);
            break;

          default :
            PTRACE(2, "RTP\tSession " << sessionID << ", Unknown Payload Specific feedback type: " << frame.GetFbType());
        }
        break;
  #endif

      default :
        PTRACE(2, "RTP\tSession " << sessionID << ", Unknown control payload type: " << frame.GetPayloadType());
    }
  } while (frame.ReadNextPacket());

  return e_ProcessPacket;
}


void RTP_Session::OnRxSenderReport(const SenderReport & PTRACE_PARAM(sender), const ReceiverReportArray & reports)
{
#if PTRACING
  if (PTrace::CanTrace(3)) {
    ostream & strm = PTrace::Begin(3, __FILE__, __LINE__);
    strm << "RTP\tSession " << sessionID << ", OnRxSenderReport: " << sender << '\n';
    for (PINDEX i = 0; i < reports.GetSize(); i++)
      strm << "  RR: " << reports[i] << '\n';
    strm << PTrace::End;
  }
#endif
  OnReceiverReports(reports);
}


void RTP_Session::OnRxReceiverReport(DWORD PTRACE_PARAM(src), const ReceiverReportArray & reports)
{
#if PTRACING
  if (PTrace::CanTrace(3)) {
    ostream & strm = PTrace::Begin(2, __FILE__, __LINE__);
    strm << "RTP\tSession " << sessionID << ", OnReceiverReport: ssrc=" << src << '\n';
    for (PINDEX i = 0; i < reports.GetSize(); i++)
      strm << "  RR: " << reports[i] << '\n';
    strm << PTrace::End;
  }
#endif
  OnReceiverReports(reports);
}


void RTP_Session::OnReceiverReports(const ReceiverReportArray & reports)
{
  for (PINDEX i = 0; i < reports.GetSize(); i++) {
    if (reports[i].sourceIdentifier == syncSourceOut) {
      packetsLostByRemote = reports[i].totalLost;
      jitterLevelOnRemote = reports[i].jitter;
      break;
    }
  }
}


void RTP_Session::OnRxSourceDescription(const SourceDescriptionArray & PTRACE_PARAM(description))
{
#if PTRACING
  if (PTrace::CanTrace(3)) {
    ostream & strm = PTrace::Begin(3, __FILE__, __LINE__);
    strm << "RTP\tSession " << sessionID << ", OnSourceDescription: " << description.GetSize() << " entries";
    for (PINDEX i = 0; i < description.GetSize(); i++)
      strm << "\n  " << description[i];
    strm << PTrace::End;
  }
#endif
}


void RTP_Session::OnRxGoodbye(const PDWORDArray & PTRACE_PARAM(src), const PString & PTRACE_PARAM(reason))
{
  PTRACE(3, "RTP\tSession " << sessionID << ", OnGoodbye: \"" << reason << "\" srcs=" << src);
}


void RTP_Session::OnRxApplDefined(const PString & PTRACE_PARAM(type),
          unsigned PTRACE_PARAM(subtype), DWORD PTRACE_PARAM(src),
          const BYTE * /*data*/, PINDEX PTRACE_PARAM(size))
{
  PTRACE(3, "RTP\tSession " << sessionID << ", OnApplDefined: \""
         << type << "\"-" << subtype << " " << src << " [" << size << ']');
}


void RTP_Session::ReceiverReport::PrintOn(ostream & strm) const
{
  strm << "ssrc=" << sourceIdentifier
       << " fraction=" << fractionLost
       << " lost=" << totalLost
       << " last_seq=" << lastSequenceNumber
       << " jitter=" << jitter
       << " lsr=" << lastTimestamp
       << " dlsr=" << delay;
}


void RTP_Session::SenderReport::PrintOn(ostream & strm) const
{
  strm << "ssrc=" << sourceIdentifier
       << " ntp=" << realTimestamp.AsString("yyyy/M/d-h:m:s.uuuu")
       << " rtp=" << rtpTimestamp
       << " psent=" << packetsSent
       << " osent=" << octetsSent;
}


void RTP_Session::SourceDescription::PrintOn(ostream & strm) const
{
  static const char * const DescriptionNames[RTP_ControlFrame::NumDescriptionTypes] = {
    "END", "CNAME", "NAME", "EMAIL", "PHONE", "LOC", "TOOL", "NOTE", "PRIV"
  };

  strm << "ssrc=" << sourceIdentifier;
  for (PINDEX i = 0; i < items.GetSize(); i++) {
    strm << "\n  item[" << i << "]: type=";
    unsigned typeNum = items.GetKeyAt(i);
    if (typeNum < PARRAYSIZE(DescriptionNames))
      strm << DescriptionNames[typeNum];
    else
      strm << typeNum;
    strm << " data=\""
      << items.GetDataAt(i)
      << '"';
  }
}


DWORD RTP_Session::GetPacketsTooLate() const
{
  JitterBufferPtr jitter = m_jitterBuffer; // Increase reference count
  return jitter != NULL ? jitter->GetPacketsTooLate() : 0;
}


DWORD RTP_Session::GetPacketOverruns() const
{
  JitterBufferPtr jitter = m_jitterBuffer; // Increase reference count
  return jitter != NULL ? jitter->GetBufferOverruns() : 0;
}


PBoolean RTP_Session::WriteOOBData(RTP_DataFrame &, bool)
{
  return true;
}


void RTP_Session::AddFilter(const FilterNotifier & filter)
{
  // ensures that a filter is added only once
  if (find(m_filters.begin(), m_filters.end(), filter) == m_filters.end())
    m_filters.push_back(filter);
}


/////////////////////////////////////////////////////////////////////////////

static void SetMinBufferSize(PUDPSocket & sock, int buftype, int bufsz)
{
  int sz = 0;
  if (!sock.GetOption(buftype, sz)) {
    PTRACE(1, "RTP_UDP\tGetOption(" << sock.GetHandle() << ',' << buftype << ") failed: " << sock.GetErrorText());
    return;
  }

  // Already big enough
  if (sz >= bufsz)
    return;

  for (; bufsz >= 1024; bufsz /= 2) {
    // Set to new size
    if (!sock.SetOption(buftype, bufsz)) {
      PTRACE(1, "RTP_UDP\tSetOption(" << sock.GetHandle() << ',' << buftype << ',' << bufsz << ") failed: " << sock.GetErrorText());
      continue;
    }

    // As some stacks lie about setting the buffer size, we double check.
    if (!sock.GetOption(buftype, sz)) {
      PTRACE(1, "RTP_UDP\tGetOption(" << sock.GetHandle() << ',' << buftype << ") failed: " << sock.GetErrorText());
      return;
    }

    if (sz >= bufsz) {
      PTRACE(4, "RTP_UDP\tSetOption(" << sock.GetHandle() << ',' << buftype << ',' << bufsz << ") succeeded.");
      return;
    }

    PTRACE(1, "RTP_UDP\tSetOption(" << sock.GetHandle() << ',' << buftype << ',' << bufsz << ") failed, even though it said it succeeded!");
  }
}


RTP_UDP::RTP_UDP(const Params & params)
  : RTP_Session(params),
    remoteAddress(0),
    remoteTransmitAddress(0),
    remoteIsNAT(params.remoteIsNAT)
{
  PTRACE(4, "RTP_UDP\tSession " << sessionID << ", created with NAT flag set to " << remoteIsNAT);
  remoteDataPort    = 0;
  remoteControlPort = 0;
  shutdownRead      = false;
  shutdownWrite     = false;
  dataSocket        = NULL;
  controlSocket     = NULL;
  appliedQOS        = false;
  localHasNAT       = false;
  badTransmitCounter = 0;

  timerWriteDataIdle.SetNotifier(PCREATE_NOTIFIER(OnWriteDataIdle));
}


RTP_UDP::~RTP_UDP()
{
  timerWriteDataIdle.Stop();
  Close(true);
  Close(false);

  // We need to do this to make sure that the sockets are not
  // deleted before select decides there is no more data coming
  // over them and exits the reading thread.
  SetJitterBufferSize(0, 0);

  delete dataSocket;
  delete controlSocket;
}


void RTP_UDP::ApplyQOS(const PIPSocket::Address & addr)
{
  if (controlSocket != NULL)
    controlSocket->SetSendAddress(addr,GetRemoteControlPort());
  if (dataSocket != NULL)
    dataSocket->SetSendAddress(addr,GetRemoteDataPort());
  appliedQOS = true;
}


PBoolean RTP_UDP::ModifyQOS(RTP_QOS * rtpqos)
{
  PBoolean retval = false;

  if (rtpqos == NULL)
    return retval;

  if (controlSocket != NULL)
    retval = controlSocket->ModifyQoSSpec(&(rtpqos->ctrlQoS));
    
  if (dataSocket != NULL)
    retval &= dataSocket->ModifyQoSSpec(&(rtpqos->dataQoS));

  appliedQOS = false;
  return retval;
}

PBoolean RTP_UDP::Open(PIPSocket::Address transportLocalAddress,
                   WORD portBase, WORD portMax,
                   BYTE tos,
                   PNatMethod * natMethod,
                   RTP_QOS * rtpQos)
{
  PWaitAndSignal mutex(dataMutex);

  m_firstControl = true;

  // save local address 
  localAddress = transportLocalAddress;

  localDataPort    = (WORD)(portBase&0xfffe);
  localControlPort = (WORD)(localDataPort + 1);

  delete dataSocket;
  delete controlSocket;
  dataSocket = NULL;
  controlSocket = NULL;

  byeSent = false;

  PQoS * dataQos = NULL;
  PQoS * ctrlQos = NULL;
  if (rtpQos != NULL) {
    dataQos = &(rtpQos->dataQoS);
    ctrlQos = &(rtpQos->ctrlQoS);
  }

  // allow for special case of portBase == 0 or portMax == 0, which indicates a shared RTP session
  if ((portBase != 0) || (portMax != 0)) {
    PIPSocket::Address bindingAddress = localAddress;
    if (natMethod != NULL && natMethod->IsAvailable(localAddress)) {
      switch (natMethod->GetRTPSupport()) {
        case PNatMethod::RTPIfSendMedia :
          /* This NAT variant will work if we send something out through the
             NAT port to "open" it so packets can then flow inward. We set
             this flag to make that happen as soon as we get the remotes IP
             address and port to send to.
            */
          localHasNAT = true;
          // Then do case for full cone support and create STUN sockets

        case PNatMethod::RTPSupported :
          if (natMethod->CreateSocketPair(dataSocket, controlSocket, localAddress)) {
            PTRACE(4, "RTP\tSession " << sessionID << ", " << natMethod->GetName() << " created STUN RTP/RTCP socket pair.");
            dataSocket->GetLocalAddress(localAddress, localDataPort);
            controlSocket->GetLocalAddress(localAddress, localControlPort);
          }
          else {
            PTRACE(2, "RTP\tSession " << sessionID << ", " << natMethod->GetName()
                   << " could not create RTP/RTCP socket pair; trying to create individual sockets.");
            if (natMethod->CreateSocket(dataSocket, localAddress) && natMethod->CreateSocket(controlSocket, localAddress)) {
              dataSocket->GetLocalAddress(localAddress, localDataPort);
              controlSocket->GetLocalAddress(localAddress, localControlPort);
            }
            else {
              delete dataSocket;
              delete controlSocket;
              dataSocket = NULL;
              controlSocket = NULL;
              PTRACE(2, "RTP\tSession " << sessionID << ", " << natMethod->GetName()
                     << " could not create RTP/RTCP sockets individually either, using normal sockets.");
            }
          }
          break;

        default :
          /* We canot use NAT traversal method (e.g. STUN) to create sockets
             in the remaining modes as the NAT router will then not let us
             talk to the real RTP destination. All we can so is bind to the
             local interface the NAT is on and hope the NAT router is doing
             something sneaky like symmetric port forwarding. */
          PTRACE(2, "RTP\tSession " << sessionID << ", " << natMethod->GetName()
                  << " cannot create RTP/RTCP socket pair; creating individual sockets.");
          natMethod->GetInterfaceAddress(bindingAddress);
          break;
      }
    }

    if (dataSocket == NULL || controlSocket == NULL) {
      dataSocket = new PUDPSocket(dataQos);
      controlSocket = new PUDPSocket(ctrlQos);
      while (!   dataSocket->Listen(bindingAddress, 1, localDataPort) ||
             !controlSocket->Listen(bindingAddress, 1, localControlPort)) {
        dataSocket->Close();
        controlSocket->Close();
        if ((localDataPort > portMax) || (localDataPort > 0xfffd))
          return false; // If it ever gets to here the OS has some SERIOUS problems!
        localDataPort    += 2;
        localControlPort += 2;
      }
    }

#   ifndef __BEOS__
    // Set the IP Type Of Service field for prioritisation of media UDP packets
    // through some Cisco routers and Linux boxes
    if (!dataSocket->SetOption(IP_TOS, tos, IPPROTO_IP)) {
      PTRACE(1, "RTP_UDP\tSession " << sessionID << ", could not set TOS field in IP header: " << dataSocket->GetErrorText());
    }

    // Increase internal buffer size on media UDP sockets
    SetMinBufferSize(*dataSocket,    SO_RCVBUF, isAudio ? RTP_AUDIO_RX_BUFFER_SIZE : RTP_VIDEO_RX_BUFFER_SIZE);
    SetMinBufferSize(*dataSocket,    SO_SNDBUF, RTP_DATA_TX_BUFFER_SIZE);
    SetMinBufferSize(*controlSocket, SO_RCVBUF, RTP_CTRL_BUFFER_SIZE);
    SetMinBufferSize(*controlSocket, SO_SNDBUF, RTP_CTRL_BUFFER_SIZE);
#   endif
  }

  shutdownRead = false;
  shutdownWrite = false;

  if (canonicalName.Find('@') == P_MAX_INDEX)
    canonicalName += '@' + GetLocalHostName();

  PTRACE(3, "RTP_UDP\tSession " << sessionID << " created: "
         << localAddress << ':' << localDataPort << '-' << localControlPort
         << " ssrc=" << syncSourceOut);

  m_reportTimer.RunContinuous(m_reportTimer.GetResetTime());
  return true;
}


void RTP_UDP::Reopen(PBoolean reading)
{
  PWaitAndSignal mutex(dataMutex);

  if (reading)
    shutdownRead = false;
  else
    shutdownWrite = false;

  badTransmitCounter = 0;
  m_reportTimer.RunContinuous(m_reportTimer.GetResetTime());

  PTRACE(3, "RTP_UDP\tSession " << sessionID << " reopened for " << (reading ? "reading" : "writing"));
}


bool RTP_UDP::Close(PBoolean reading)
{
  if (reading) {
    {
      PWaitAndSignal mutex(dataMutex);

      if (shutdownRead) {
        PTRACE(4, "RTP_UDP\tSession " << sessionID << ", read already shut down .");
        return false;
      }

      PTRACE(3, "RTP_UDP\tSession " << sessionID << ", Shutting down read.");

      syncSourceIn = 0;
      shutdownRead = true;

      if (dataSocket != NULL && controlSocket != NULL) {
        PIPSocket::Address addr;
        controlSocket->GetLocalAddress(addr);
        if (addr.IsAny())
          PIPSocket::GetHostAddress(addr);
        dataSocket->WriteTo("", 1, addr, controlSocket->GetPort());
      }
    }

    SetJitterBufferSize(0, 0); // Kill jitter buffer too, but outside mutex
  }
  else {
    if (shutdownWrite) {
      PTRACE(4, "RTP_UDP\tSession " << sessionID << ", write already shut down .");
      return false;
    }

    PTRACE(3, "RTP_UDP\tSession " << sessionID << ", shutting down write.");
    shutdownWrite = true;
  }

  if (shutdownRead && shutdownWrite)
    m_reportTimer.Stop(false);

  return true;
}


PString RTP_UDP::GetLocalHostName()
{
  return PIPSocket::GetHostName();
}


PBoolean RTP_UDP::SetRemoteSocketInfo(PIPSocket::Address address, WORD port, PBoolean isDataPort)
{
  if (remoteIsNAT) {
    PTRACE(2, "RTP_UDP\tSession " << sessionID << ", ignoring remote socket info as remote is behind NAT");
    return true;
  }

  if (!PAssert(address.IsValid() && port != 0,PInvalidParameter))
    return false;

  PTRACE(3, "RTP_UDP\tSession " << sessionID << ", SetRemoteSocketInfo: "
         << (isDataPort ? "data" : "control") << " channel, "
            "new=" << address << ':' << port << ", "
            "local=" << localAddress << ':' << localDataPort << '-' << localControlPort << ", "
            "remote=" << remoteAddress << ':' << remoteDataPort << '-' << remoteControlPort);

  if (localAddress == address && remoteAddress == address && (isDataPort ? localDataPort : localControlPort) == port)
    return true;
  
  remoteAddress = address;
  
  allowOneSyncSourceChange = true;
  allowRemoteTransmitAddressChange = true;
  allowSequenceChange = packetsReceived != 0;

  if (isDataPort) {
    remoteDataPort = port;
    if (remoteControlPort == 0 || allowRemoteTransmitAddressChange)
      remoteControlPort = (WORD)(port + 1);
  }
  else {
    remoteControlPort = port;
    if (remoteDataPort == 0 || allowRemoteTransmitAddressChange)
      remoteDataPort = (WORD)(port - 1);
  }

  if (!appliedQOS)
      ApplyQOS(remoteAddress);

  if (localHasNAT) {
    // If have Port Restricted NAT on local host then send a datagram
    // to remote to open up the port in the firewall for return data.
    static const BYTE dummy[1] = { 0 };
    WriteDataOrControlPDU(dummy, sizeof(dummy), true);
    WriteDataOrControlPDU(dummy, sizeof(dummy), false);
    PTRACE(2, "RTP_UDP\tSession " << sessionID << ", sending empty datagrams to open local Port Restricted NAT");
  }

  return true;
}


PBoolean RTP_UDP::ReadData(RTP_DataFrame & frame)
{
  return EncodingLock(*this)->ReadData(frame);
}

PBoolean RTP_UDP::Internal_ReadData(RTP_DataFrame & frame)
{
  for (;;) {
    int selectStatus = WaitForPDU(*dataSocket, *controlSocket, PMaxTimeInterval);

    {
      PWaitAndSignal mutex(dataMutex);
      if (shutdownRead) {
        PTRACE(3, "RTP_UDP\tSession " << sessionID << ", Read shutdown.");
        return false;
      }
    }

    switch (selectStatus) {
      case -2 :
        if (ReadControlPDU() == e_AbortTransport)
          return false;
        break;

      case -3 :
        if (ReadControlPDU() == e_AbortTransport)
          return false;
        // Then do -1 case

      case -1 :
        switch (ReadDataPDU(frame)) {
          case e_ProcessPacket :
            if (!shutdownRead) {
              switch (OnReceiveData(frame)) {
                case e_ProcessPacket :
                  return true;
                case e_IgnorePacket :
                  break;
                case e_AbortTransport :
                  return false;
              }
            }
          case e_IgnorePacket :
            break;
          case e_AbortTransport :
            return false;
        }
        break;

      case 0 :
        switch (OnReadTimeout(frame)) {
          case e_ProcessPacket :
            if (!shutdownRead)
              return true;
          case e_IgnorePacket :
            break;
          case e_AbortTransport :
            return false;
        }
        break;

      case PSocket::Interrupted:
        PTRACE(2, "RTP_UDP\tSession " << sessionID << ", Interrupted.");
        return false;

      default :
        PTRACE(1, "RTP_UDP\tSession " << sessionID << ", Select error: "
                << PChannel::GetErrorText((PChannel::Errors)selectStatus));
        return false;
    }
  }
}


void RTP_UDP::FlushData()
{
  if (dataSocket == NULL)
    return;

  PTimeInterval oldTimeout = dataSocket->GetReadTimeout();
  dataSocket->SetReadTimeout(0);

  PINDEX count = 0;
  BYTE buffer[2000];
  while (dataSocket->Read(buffer, sizeof(buffer)))
    ++count;

  dataSocket->SetReadTimeout(oldTimeout);

  PTRACE_IF(3, count > 0, "RTP_UDP\tSession " << sessionID << ", flushed "
            << count << " RTP data packets before activating jitter buffer");
}


int RTP_UDP::WaitForPDU(PUDPSocket & dataSocket, PUDPSocket & controlSocket, const PTimeInterval & timeout)
{
  return EncodingLock(*this)->WaitForPDU(dataSocket, controlSocket, timeout);
}

int RTP_UDP::Internal_WaitForPDU(PUDPSocket & dataSocket, PUDPSocket & controlSocket, const PTimeInterval & timeout)
{
  return PSocket::Select(dataSocket, controlSocket, timeout);
}

RTP_Session::SendReceiveStatus RTP_UDP::ReadDataOrControlPDU(BYTE * framePtr,
                                                             PINDEX frameSize,
                                                             PBoolean fromDataChannel)
{
#if PTRACING
  const char * channelName = fromDataChannel ? "Data" : "Control";
#endif
  PUDPSocket & socket = *(fromDataChannel ? dataSocket : controlSocket);
  PIPSocket::Address addr;
  WORD port;

  if (socket.ReadFrom(framePtr, frameSize, addr, port)) {
    // If remote address never set from higher levels, then try and figure
    // it out from the first packet received.
    if (!remoteAddress.IsValid()) {
      remoteAddress = addr;
      PTRACE(4, "RTP\tSession " << sessionID << ", set remote address from first "
             << channelName << " PDU from " << addr << ':' << port);
    }
    if (fromDataChannel) {
      if (remoteDataPort == 0)
        remoteDataPort = port;
    }
    else {
      if (remoteControlPort == 0)
        remoteControlPort = port;
    }

    if (!remoteTransmitAddress.IsValid())
      remoteTransmitAddress = addr;
    else if (allowRemoteTransmitAddressChange && remoteAddress == addr) {
      remoteTransmitAddress = addr;
      allowRemoteTransmitAddressChange = false;
    }
    else if (remoteTransmitAddress != addr && !allowRemoteTransmitAddressChange) {
      PTRACE(2, "RTP_UDP\tSession " << sessionID << ", "
             << channelName << " PDU from incorrect host, "
                " is " << addr << " should be " << remoteTransmitAddress);
      return RTP_Session::e_IgnorePacket;
    }

    if (remoteAddress.IsValid() && !appliedQOS) 
      ApplyQOS(remoteAddress);

    badTransmitCounter = 0;

    return RTP_Session::e_ProcessPacket;
  }

  switch (socket.GetErrorNumber(PChannel::LastReadError)) {
    case ECONNRESET :
    case ECONNREFUSED :
      PTRACE(2, "RTP_UDP\tSession " << sessionID << ", " << channelName << " port on remote not ready.");
      if (++badTransmitCounter == 1) 
        badTransmitStart = PTime();
      else {
        if (badTransmitCounter < 5 || (PTime()- badTransmitStart).GetSeconds() < BAD_TRANSMIT_TIME_MAX)
          return RTP_Session::e_IgnorePacket;
        PTRACE(2, "RTP_UDP\tSession " << sessionID << ", " << channelName << " " << BAD_TRANSMIT_TIME_MAX << " seconds of transmit fails - informing connection");
        userData->SessionFailing(*this);
      }
      return RTP_Session::e_IgnorePacket;

    case EMSGSIZE :
      PTRACE(2, "RTP_UDP\tSession " << sessionID << ", " << channelName
             << " read packet too large for buffer of " << frameSize << " bytes.");
      return RTP_Session::e_IgnorePacket;

    case EAGAIN :
      PTRACE(4, "RTP_UDP\tSession " << sessionID << ", " << channelName
             << " read packet interrupted.");
      // Shouldn't happen, but it does.
      return RTP_Session::e_IgnorePacket;

    default:
      PTRACE(1, "RTP_UDP\tSession " << sessionID << ", " << channelName
             << " read error (" << socket.GetErrorNumber(PChannel::LastReadError) << "): "
             << socket.GetErrorText(PChannel::LastReadError));
      return RTP_Session::e_AbortTransport;
  }
}


RTP_Session::SendReceiveStatus RTP_UDP::ReadDataPDU(RTP_DataFrame & frame)
{
  return EncodingLock(*this)->ReadDataPDU(frame);
}

RTP_Session::SendReceiveStatus RTP_UDP::Internal_ReadDataPDU(RTP_DataFrame & frame)
{
  SendReceiveStatus status = ReadDataOrControlPDU(frame.GetPointer(), frame.GetSize(), true);
  if (status != e_ProcessPacket)
    return status;

  // Check received PDU is big enough
  return frame.SetPacketSize(dataSocket->GetLastReadCount()) ? e_ProcessPacket : e_IgnorePacket;
}


bool RTP_UDP::WriteDataPDU(RTP_DataFrame & frame)
{
  if (!EncodingLock(*this)->WriteDataPDU(frame))
      return false;

  PWaitAndSignal mutex(dataMutex);
  EncodingLock(*this)->SetWriteDataIdleTimer(timerWriteDataIdle);

  return true;
}


RTP_Session::SendReceiveStatus RTP_UDP::OnReadTimeout(RTP_DataFrame & frame)
{
  return EncodingLock(*this)->OnReadTimeout(frame);
}

RTP_Session::SendReceiveStatus RTP_UDP::Internal_OnReadTimeout(RTP_DataFrame & /*frame*/)
{
  return e_IgnorePacket;
}


RTP_Session::SendReceiveStatus RTP_UDP::ReadControlPDU()
{
  RTP_ControlFrame frame(2048);

  SendReceiveStatus status = ReadDataOrControlPDU(frame.GetPointer(), frame.GetSize(), false);
  if (status != e_ProcessPacket)
    return status;

  PINDEX pduSize = controlSocket->GetLastReadCount();
  if (pduSize < 4 || pduSize < 4+frame.GetPayloadSize()) {
    PTRACE_IF(2, pduSize != 1 || !m_firstControl, "RTP_UDP\tSession " << sessionID
              << ", Received control packet too small: " << pduSize << " bytes");
    return e_IgnorePacket;
  }

  m_firstControl = false;
  frame.SetSize(pduSize);
  return OnReceiveControl(frame);
}


PBoolean RTP_UDP::WriteOOBData(RTP_DataFrame & frame, bool rewriteTimeStamp)
{
  PWaitAndSignal m(dataMutex);

  // set timestamp offset if not already set
  // otherwise offset timestamp
  if (!oobTimeStampBaseEstablished) {
    oobTimeStampBaseEstablished = true;
    oobTimeStampBase            = PTimer::Tick();
    if (rewriteTimeStamp)
      oobTimeStampOutBase = PRandom::Number();
    else
      oobTimeStampOutBase = frame.GetTimestamp();
  }

  // set new timestamp
  if (rewriteTimeStamp) 
    frame.SetTimestamp(oobTimeStampOutBase + ((PTimer::Tick() - oobTimeStampBase).GetInterval() * 8));

  // write the data
  return EncodingLock(*this)->WriteData(frame, true);
}

PBoolean RTP_UDP::WriteData(RTP_DataFrame & frame)
{
  return EncodingLock(*this)->WriteData(frame, false);
}


PBoolean RTP_UDP::Internal_WriteData(RTP_DataFrame & frame)
{
  {
    PWaitAndSignal mutex(dataMutex);
    if (shutdownWrite) {
      PTRACE(3, "RTP_UDP\tSession " << sessionID << ", write shutdown.");
      return false;
    }
  }

  // Trying to send a PDU before we are set up!
  if (!remoteAddress.IsValid() || remoteDataPort == 0)
    return true;

  switch (OnSendData(frame)) {
    case e_ProcessPacket :
      break;
    case e_IgnorePacket :
      return true;
    case e_AbortTransport :
      return false;
  }

  return WriteDataPDU(frame);
}


void RTP_UDP::OnWriteDataIdle(PTimer &, INT)
{
  {
    PWaitAndSignal mutex(dataMutex);
    if (shutdownWrite) {
      PTRACE(3, "RTP_UDP\tSession " << sessionID << ", write shutdown.");
      return;
    }
  }

  // Trying to send a PDU before we are set up!
  if (!remoteAddress.IsValid() || remoteDataPort == 0)
    return;

  EncodingLock(*this)->OnWriteDataIdle();

  PWaitAndSignal mutex(dataMutex);
  EncodingLock(*this)->SetWriteDataIdleTimer(timerWriteDataIdle);
}


void RTP_UDP::SetEncoding(const PString & newEncoding)
{
  dataMutex.Wait();
  timerWriteDataIdle.Stop(false);
  dataMutex.Signal();

  RTP_Session::SetEncoding(newEncoding);
}


PBoolean RTP_UDP::WriteControl(RTP_ControlFrame & frame)
{
  // Trying to send a PDU before we are set up!
  if (!remoteAddress.IsValid() || remoteControlPort == 0 || controlSocket == NULL)
    return true;

  PINDEX len = frame.GetCompoundSize();
  switch (OnSendControl(frame, len)) {
    case e_ProcessPacket :
      break;
    case e_IgnorePacket :
      return true;
    case e_AbortTransport :
      return false;
  }

  return WriteDataOrControlPDU(frame.GetPointer(), len, false);
}


bool RTP_UDP::WriteDataOrControlPDU(const BYTE * framePtr, PINDEX frameSize, bool toDataChannel)
{
  PUDPSocket & socket = *(toDataChannel ? dataSocket : controlSocket);
  WORD port = toDataChannel ? remoteDataPort : remoteControlPort;
  int retry = 0;

  while (!socket.WriteTo(framePtr, frameSize, remoteAddress, port)) {
    switch (socket.GetErrorNumber()) {
      case ECONNRESET :
      case ECONNREFUSED :
        break;

      default:
        PTRACE(1, "RTP_UDP\tSession " << sessionID
               << ", write (" << frameSize << " bytes) error on "
               << (toDataChannel ? "data" : "control") << " port ("
               << socket.GetErrorNumber(PChannel::LastWriteError) << "): "
               << socket.GetErrorText(PChannel::LastWriteError));
        return false;
    }

    if (++retry >= 10)
      break;
  }

  PTRACE_IF(2, retry > 0, "RTP_UDP\tSession " << sessionID << ", " << (toDataChannel ? "data" : "control")
            << " port on remote not ready " << retry << " time" << (retry > 1 ? "s" : "")
            << (retry < 10 ? "" : ", data never sent"));
  return true;
}


void RTP_Session::SendIntraFrameRequest(bool rfc2032, bool pictureLoss)
{
  PTRACE(3, "RTP\tSession " << sessionID << ", SendIntraFrameRequest using "
         << (rfc2032 ? "RFC2032" : (pictureLoss ? "RFC4585 PLI" : "RFC5104 FIR")));

  // Create packet
  RTP_ControlFrame request;
  InsertReportPacket(request);

  request.StartNewPacket();

  if (rfc2032) {
    // Create packet
    request.SetPayloadType(RTP_ControlFrame::e_IntraFrameRequest);
    request.SetPayloadSize(4);
    // Insert SSRC
    request.SetCount(1);
    BYTE * payload = request.GetPayloadPtr();
    *(PUInt32b *)payload = syncSourceOut;
  }
  else {
    request.SetPayloadType(RTP_ControlFrame::e_PayloadSpecificFeedBack);
    if (pictureLoss)
      request.SetFbType(RTP_ControlFrame::e_PictureLossIndication, 0);
    else {
      request.SetFbType(RTP_ControlFrame::e_FullIntraRequest, sizeof(RTP_ControlFrame::FbFIR));
      RTP_ControlFrame::FbFIR * fir = (RTP_ControlFrame::FbFIR *)request.GetPayloadPtr();
      fir->requestSSRC = syncSourceIn;
    }
    RTP_ControlFrame::FbFCI * fci = (RTP_ControlFrame::FbFCI *)request.GetPayloadPtr();
    fci->senderSSRC = syncSourceOut;
  }

  // Send it
  request.EndPacket();
  WriteControl(request);
}


void RTP_Session::SendTemporalSpatialTradeOff(unsigned tradeOff)
{
  PTRACE(3, "RTP\tSession " << sessionID << ", SendTemporalSpatialTradeOff " << tradeOff);

  RTP_ControlFrame request;
  InsertReportPacket(request);

  request.StartNewPacket();
  request.SetPayloadType(RTP_ControlFrame::e_PayloadSpecificFeedBack);
  request.SetFbType(RTP_ControlFrame::e_TemporalSpatialTradeOffRequest, sizeof(RTP_ControlFrame::FbTSTO));
  RTP_ControlFrame::FbTSTO * tsto = (RTP_ControlFrame::FbTSTO *)request.GetPayloadPtr();
  tsto->requestSSRC = syncSourceIn;
  tsto->tradeOff = (BYTE)tradeOff;

  // Send it
  request.EndPacket();
  WriteControl(request);
}


void RTP_Session::SetEncoding(const PString & newEncoding)
{
  {
    PWaitAndSignal m(m_encodingMutex);

    if (newEncoding == m_encoding)
      return;

    RTP_Encoding * newHandler = PFactory<RTP_Encoding>::CreateInstance(newEncoding);
    if (newHandler == NULL) {
      PTRACE(2, "RTP\tUnable to identify new RTP format '" << newEncoding << "' - retaining old format '" << m_encoding << "'");
      return;
    }

    if (m_encodingHandler != NULL) {
      --m_encodingHandler->refCount;
      if (m_encodingHandler->refCount == 0)
        delete m_encodingHandler;
      m_encodingHandler = NULL;
    }

    PTRACE_IF(2, !m_encoding.IsEmpty(), "RTP\tChanged RTP session format from '" << m_encoding << "' to '" << newEncoding << "'");

    m_encoding  = newEncoding;
    m_encodingHandler = newHandler;
  }

  ClearStatistics();

  EncodingLock(*this)->OnStart(*this);
}

/////////////////////////////////////////////////////////////////////////////

RTP_Session::EncodingLock::EncodingLock(RTP_Session & _session)
  : session(_session)
{
  PWaitAndSignal m(session.m_encodingMutex);

  m_encodingHandler = session.m_encodingHandler;
  ++m_encodingHandler->refCount;
}

RTP_Session::EncodingLock::~EncodingLock()
{
  PWaitAndSignal m(session.m_encodingMutex);

  --m_encodingHandler->refCount;
  if (m_encodingHandler->refCount == 0)
    delete m_encodingHandler;
}


/////////////////////////////////////////////////////////////////////////////

RTP_Encoding::RTP_Encoding()
{
  refCount = 1;
}

RTP_Encoding::~RTP_Encoding()
{
  OnFinish();
}


void RTP_Encoding::OnStart(RTP_Session & _rtpSession)
{
  //rtpSession = &_rtpSession;
  rtpUDP = (RTP_UDP *)&_rtpSession;
}

void RTP_Encoding::OnFinish()
{
}

RTP_Session::SendReceiveStatus RTP_Encoding::OnSendData(RTP_DataFrame & frame)
{
  return rtpUDP->Internal_OnSendData(frame);
}

PBoolean RTP_Encoding::WriteData(RTP_DataFrame & frame, bool)
{
  return rtpUDP->Internal_WriteData(frame);
}

RTP_Session::SendReceiveStatus RTP_Encoding::OnSendControl(RTP_ControlFrame & frame, PINDEX & len)
{
  return rtpUDP->Internal_OnSendControl(frame, len);
}

bool RTP_Encoding::WriteDataPDU(RTP_DataFrame & frame)
{
  return rtpUDP->WriteDataOrControlPDU(frame.GetPointer(), frame.GetHeaderSize()+frame.GetPayloadSize(), true);
}

RTP_Session::SendReceiveStatus RTP_Encoding::ReadDataPDU(RTP_DataFrame & frame)
{
  return rtpUDP->Internal_ReadDataPDU(frame);
}

RTP_Session::SendReceiveStatus RTP_Encoding::OnReceiveData(RTP_DataFrame & frame)
{
  return rtpUDP->Internal_OnReceiveData(frame);
}

RTP_Session::SendReceiveStatus RTP_Encoding::OnReadTimeout(RTP_DataFrame & frame)
{
  return rtpUDP->Internal_OnReadTimeout(frame);
}

PBoolean RTP_Encoding::ReadData(RTP_DataFrame & frame)
{
  return rtpUDP->Internal_ReadData(frame);
}

int RTP_Encoding::WaitForPDU(PUDPSocket & dataSocket, PUDPSocket & controlSocket, const PTimeInterval & t)
{
  return rtpUDP->Internal_WaitForPDU(dataSocket, controlSocket, t);
}

/////////////////////////////////////////////////////////////////////////////

SecureRTP_UDP::SecureRTP_UDP(const Params & params)
  : RTP_UDP(params)
{
  securityParms = NULL;
}

SecureRTP_UDP::~SecureRTP_UDP()
{
  delete securityParms;
}

void SecureRTP_UDP::SetSecurityMode(OpalSecurityMode * newParms)
{ 
  if (securityParms != NULL)
    delete securityParms;
  securityParms = newParms; 
}
  
OpalSecurityMode * SecureRTP_UDP::GetSecurityParms() const
{ 
  return securityParms; 
}

/////////////////////////////////////////////////////////////////////////////
